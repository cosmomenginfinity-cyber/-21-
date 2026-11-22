#ifndef __FLASH_H__
#define __FLASH_H__

// 基础类型依赖
#include "zf_common_typedef.h"
#include "zf_common_headfile.h"

// 业务结构体依赖（imu_encoder_log_t）
#include "imu_datalogger.h"

// ================================= 核心宏定义 =================================
#define FLASH_LOG_HEADER_MAGIC      0x55AA55AAU
#define FLASH_PAGE_LENGTH           256U            // 每页实际用于日志的32位字数（1024字节）
#define FLASH_OK                    1
#define FLASH_ERR                   0

/*
 * TC387逐飞Flash驱动中sector_num固定为0且不参与地址计算，
 * 真正的物理区域由page_num（0~127）决定。
 *
 * 科目一、二保留：物理页 0~63（按人的编号习惯为第1~64页）
 * 科目三独占：    物理页64~127（按人的编号习惯为第65~128页）
 *   物理页64：科目三Header
 *   物理页65~127：科目三路径点
 */
#define FLASH_DRIVER_SECTOR_NUM     0U
#define FLASH_LOG_FIRST_PAGE        64U
#define FLASH_LOG_HEADER_PAGE       FLASH_LOG_FIRST_PAGE
#define FLASH_LOG_FIRST_DATA_PAGE   (FLASH_LOG_HEADER_PAGE + 1U)
#define FLASH_LOG_LAST_PAGE         127U
#define FLASH_LOG_REGION_PAGE_COUNT (FLASH_LOG_LAST_PAGE - FLASH_LOG_FIRST_PAGE + 1U)
#define FLASH_LOG_DATA_PAGE_COUNT   (FLASH_LOG_LAST_PAGE - FLASH_LOG_FIRST_DATA_PAGE + 1U)

#if FLASH_LOG_FIRST_PAGE > FLASH_LOG_LAST_PAGE
#error "Invalid subject3 Flash page range"
#endif

#if FLASH_LOG_LAST_PAGE >= EEPROM_PAGE_NUM
#error "Subject3 Flash page range exceeds TC387 DFlash"
#endif

#if FLASH_PAGE_LENGTH > EEPROM_PAGE_LENGTH
#error "Subject3 log page length exceeds one TC387 DFlash page"
#endif

// 自动计算单条记录字数（对齐到4字节）
#define LOG_RECORD_WORDS            ((sizeof(imu_encoder_log_t) + 3U) / 4U)
#define FLASH_RECORDS_PER_PAGE      (FLASH_PAGE_LENGTH / LOG_RECORD_WORDS)

// 按结构体大小和科目三可用数据页数自动计算真实容量。
#define FLASH_MAX_LOG_RECORDS       (FLASH_LOG_DATA_PAGE_COUNT * FLASH_RECORDS_PER_PAGE)

// ================================= Flash 状态结构体 =================================
typedef struct
{
    uint8_t  initialized;
    uint32_t log_count;
    uint32_t total_pages;
} flash_log_info_t;

// ================================= 对外接口声明 =================================
uint8_t flash_log_init(void);
uint8_t flash_log_store_batch(imu_encoder_log_t *log_buffer, uint32_t count);
uint32_t flash_log_read_batch(imu_encoder_log_t *log_buffer, uint32_t start_index, uint32_t count);
uint8_t flash_log_clear_all(void);
uint32_t flash_log_get_count(void);
uint8_t test_flash_module(void);
// 科目三返航用：反向读取 Flash 中的路径点
// 例如：
// start_index = flash_log_get_count() - 1
// count       = flash_log_get_count()
//
// 读出结果：
// log_buffer[0] = 最后一条记录，停车区 final_yaw 参考点
// log_buffer[1] = 倒数第二条记录
// log_buffer[2] = 倒数第三条记录
// ...
uint32_t flash_log_read_reverse_batch(
    imu_encoder_log_t *log_buffer,
    uint32_t start_index,
    uint32_t count
);
// 底层页操作。page为科目三区域内的逻辑页号0~63，内部映射到物理页64~127。
uint8_t flash_safe_erase_page(uint32_t page);
uint8_t flash_safe_write_page(uint32_t page, const uint32_t *data, uint32_t word_count);
uint8_t flash_safe_read_page(uint32_t page, uint32_t *data, uint32_t word_count);

#endif // __FLASH_H__
