// ================================= 头文件包含顺序 =================================
#include "zf_common_typedef.h"
#include "zf_common_headfile.h"
#include "zf_driver_flash.h"
#include "imu_datalogger.h"
#include "flash.h"
#include <stdint.h>
#include <string.h>

// ================================= 内部状态与常量 =================================
static flash_log_info_t g_flash_info = {0};

// 【核心机制】：通过整除计算每页最多能完整存放多少条记录。
// 页末尾不足一条记录的空间会被自动舍弃，从数学原理上彻底杜绝跨页断层的 Bug！
#define RECORDS_PER_PAGE  (FLASH_PAGE_LENGTH / LOG_RECORD_WORDS)

// ================================= 内部辅助函数 =================================
// 更新科目三专用区域的元数据页（物理页64）
static void update_meta_data(void) {
    // 1. 完全使用官方例程 API：清空官方自带的 flash_union_buffer 缓冲区为 0xFF
    flash_buffer_clear();

    // 2. 利用官方联合体存入头信息
    flash_union_buffer[0].uint32_type = FLASH_LOG_HEADER_MAGIC;
    flash_union_buffer[1].uint32_type = g_flash_info.log_count;

    // 3. 完全使用官方例程 API：一键将缓冲区写入 Flash
    flash_write_page_from_buffer(FLASH_DRIVER_SECTOR_NUM, FLASH_LOG_HEADER_PAGE);
}

// ================================= 对外接口实现 =================================

uint8_t flash_log_init(void) {
    g_flash_info.initialized = 0;
    g_flash_info.total_pages = FLASH_LOG_DATA_PAGE_COUNT;

    // 检查科目三Header页；空白时只格式化科目三的物理页64~127。
    if (flash_check(FLASH_DRIVER_SECTOR_NUM, FLASH_LOG_HEADER_PAGE) == 0) {
        flash_log_clear_all();
    }

    // 1. 完全使用官方例程 API：一键将 Flash 数据读入官方缓冲区 flash_union_buffer
    flash_read_page_to_buffer(FLASH_DRIVER_SECTOR_NUM, FLASH_LOG_HEADER_PAGE);

    // 校验官方缓冲区里的 Magic Number
    if (flash_union_buffer[0].uint32_type == FLASH_LOG_HEADER_MAGIC &&
        flash_union_buffer[1].uint32_type <= FLASH_MAX_LOG_RECORDS) {
        g_flash_info.log_count = flash_union_buffer[1].uint32_type;
    } else {
        // 如果 Flash 是乱码或数据非法，执行"软格式化"清空
        flash_log_clear_all();
    }

    g_flash_info.initialized = 1;
    return FLASH_OK;
}

uint8_t flash_log_store_batch(imu_encoder_log_t *log_buffer, uint32_t count) {
    if (!g_flash_info.initialized || log_buffer == NULL || count == 0) return FLASH_ERR;

    uint32_t records_written = 0;

    // 批量处理，一次循环处理一页
    while (records_written < count) {
        if (g_flash_info.log_count >= FLASH_MAX_LOG_RECORDS) break;

        // 科目三Header在物理页64，路径点从物理页65开始
        uint32_t current_page = FLASH_LOG_FIRST_DATA_PAGE +
                                (g_flash_info.log_count / RECORDS_PER_PAGE);
        uint32_t index_in_page = g_flash_info.log_count % RECORDS_PER_PAGE;

        // 计算当前这页还能存几条
        uint32_t records_to_write_this_page = RECORDS_PER_PAGE - index_in_page;

        if (records_to_write_this_page > (count - records_written)) {
            records_to_write_this_page = count - records_written;
        }

        if (g_flash_info.log_count + records_to_write_this_page > FLASH_MAX_LOG_RECORDS) {
            records_to_write_this_page = FLASH_MAX_LOG_RECORDS - g_flash_info.log_count;
        }

        // --- 核心合并逻辑：操作官方缓冲区 ---
        // 1. 如果是在本页追加数据，把本页已有的旧数据读入官方缓冲区
        if (index_in_page > 0) {
            flash_read_page_to_buffer(FLASH_DRIVER_SECTOR_NUM, current_page);
        } else {
            // 如果是全新的一页，直接调用官方清空接口
            flash_buffer_clear();
        }

        // 2. 将要写的新结构体拷贝入官方缓冲区对应的偏移位置
        // 注意：flash_union_buffer 是联合体数组，我们直接取目标偏移的地址进行内存拷贝，完美兼容！
        uint32_t word_offset = index_in_page * LOG_RECORD_WORDS;
        memcpy(&flash_union_buffer[word_offset],
               &log_buffer[records_written],
               records_to_write_this_page * sizeof(imu_encoder_log_t));

        // 3. 官方 API：将修改好的整个缓冲区写回 Flash
        flash_write_page_from_buffer(FLASH_DRIVER_SECTOR_NUM, current_page);

        g_flash_info.log_count += records_to_write_this_page;
        records_written += records_to_write_this_page;
    }

    if (records_written > 0) {
        update_meta_data();
    }

    return FLASH_OK;
}

uint32_t flash_log_read_batch(imu_encoder_log_t *log_buffer, uint32_t start_index, uint32_t count) {
    if (!g_flash_info.initialized || log_buffer == NULL || count == 0) return 0;
    if (start_index >= g_flash_info.log_count) return 0;

    uint32_t records_read = 0;
    uint32_t actual_count = count;

    if (start_index + count > g_flash_info.log_count) {
        actual_count = g_flash_info.log_count - start_index;
    }

    while (records_read < actual_count) {
        uint32_t current_idx = start_index + records_read;
        uint32_t current_page = FLASH_LOG_FIRST_DATA_PAGE +
                                (current_idx / RECORDS_PER_PAGE);
        uint32_t index_in_page = current_idx % RECORDS_PER_PAGE;

        uint32_t records_to_read_this_page = RECORDS_PER_PAGE - index_in_page;
        if (records_to_read_this_page > (actual_count - records_read)) {
            records_to_read_this_page = actual_count - records_read;
        }

        // 官方 API：直接提取整页到 flash_union_buffer 中
        flash_read_page_to_buffer(FLASH_DRIVER_SECTOR_NUM, current_page);

        // 从官方缓冲区中解包对应偏移位置的数据
        uint32_t word_offset = index_in_page * LOG_RECORD_WORDS;
        memcpy(&log_buffer[records_read],
               &flash_union_buffer[word_offset],
               records_to_read_this_page * sizeof(imu_encoder_log_t));

        records_read += records_to_read_this_page;
    }

    return records_read;
}

/*uint8_t flash_log_clear_all(void) {
    g_flash_info.log_count = 0;
    update_meta_data();
    return FLASH_OK;
}*/
// 科目三返航用：反向读取 Flash 中的路径点
// Flash 原始顺序：
// Flash[0]               = 发车区起点
// Flash[1]
// ...
// Flash[log_count - 1]   = 停车区 final_yaw 参考点
//
// 调用：
// flash_log_read_reverse_batch(buf, flash_log_get_count() - 1, flash_log_get_count());
//
// 读出结果：
// buf[0] = 停车区 final_yaw 参考点
// buf[1] = 停车前一个点
// buf[2] = 再前一个点
// ...
uint32_t flash_log_read_reverse_batch(imu_encoder_log_t *log_buffer, uint32_t start_index, uint32_t count)
{
    if (!g_flash_info.initialized || log_buffer == NULL || count == 0)
    {
        return 0;
    }

    if (g_flash_info.log_count == 0)
    {
        return 0;
    }

    // 如果 start_index 超过已有记录范围，自动从最后一条开始读
    if (start_index >= g_flash_info.log_count)
    {
        start_index = g_flash_info.log_count - 1;
    }

    uint32_t records_read = 0;

    for (uint32_t i = 0; i < count; i++)
    {
        // 防止 uint32_t 下溢
        if (start_index < i)
        {
            break;
        }

        uint32_t read_index = start_index - i;

        if (flash_log_read_batch(&log_buffer[records_read], read_index, 1) == 1)
        {
            records_read++;
        }
        else
        {
            break;
        }
    }

    return records_read;
}

uint8_t flash_log_clear_all(void) {
    // 只擦除科目三专用的物理页64~127，不触碰科目一、二的物理页0~63。
    for(uint32_t p = FLASH_LOG_FIRST_PAGE; p <= FLASH_LOG_LAST_PAGE; p++) {
        flash_erase_page(FLASH_DRIVER_SECTOR_NUM, p);
    }

    // 3. 重置内存变量
    g_flash_info.log_count = 0;

    // 4. 重新写回干净的 Header
    update_meta_data();

    return FLASH_OK;
}

uint32_t flash_log_get_count(void) {
    return g_flash_info.log_count;
}


// ================================= 保持 flash.h 声明的底层安全接口实现 =================================

uint8_t flash_safe_erase_page(uint32_t page) {
    if (page >= FLASH_LOG_REGION_PAGE_COUNT) return FLASH_ERR;
    flash_erase_page(FLASH_DRIVER_SECTOR_NUM, FLASH_LOG_FIRST_PAGE + page);
    return FLASH_OK;
}

uint8_t flash_safe_write_page(uint32_t page, const uint32_t *data, uint32_t word_count) {
    if (page >= FLASH_LOG_REGION_PAGE_COUNT ||
        data == NULL ||
        word_count == 0 ||
        word_count > EEPROM_PAGE_LENGTH) {
        return FLASH_ERR;
    }
    flash_write_page(FLASH_DRIVER_SECTOR_NUM,
                     FLASH_LOG_FIRST_PAGE + page,
                     (const uint32 *)data,
                     (uint16)word_count);
    return FLASH_OK;
}

uint8_t flash_safe_read_page(uint32_t page, uint32_t *data, uint32_t word_count) {
    if (page >= FLASH_LOG_REGION_PAGE_COUNT ||
        data == NULL ||
        word_count == 0 ||
        word_count > EEPROM_PAGE_LENGTH) {
        return FLASH_ERR;
    }
    flash_read_page(FLASH_DRIVER_SECTOR_NUM,
                    FLASH_LOG_FIRST_PAGE + page,
                    (uint32 *)data,
                    (uint16)word_count);
    return FLASH_OK;
}

// ================================= 自测函数 =================================
uint8_t test_flash_module(void) {
    imu_encoder_log_t test_data;
    memset(&test_data, 0x00, sizeof(imu_encoder_log_t));
    test_data.timestamp_ms = 99999;
    test_data.main_encoder_cnt = 88888888;

    if (flash_log_store_batch(&test_data, 1) == FLASH_OK) {
        imu_encoder_log_t read_data;
        if (flash_log_read_batch(&read_data, g_flash_info.log_count - 1, 1) == 1) {
            if (read_data.timestamp_ms == 99999 && read_data.main_encoder_cnt == 88888888) {
                return FLASH_OK;
            }
        }
    }
    return FLASH_ERR;
}
