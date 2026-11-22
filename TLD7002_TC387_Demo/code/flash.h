    #ifndef __FLASH_H__
#define __FLASH_H__

// 基础类型依赖
#include "zf_common_typedef.h"
#include "zf_common_headfile.h"

// 业务结构体依赖（imu_encoder_log_t）
#include "imu_datalogger.h"

// ================================= 核心宏定义 =================================
#define FLASH_LOG_HEADER_MAGIC      0x55AA55AAU
#define FLASH_PAGE_LENGTH           256U            // 每页 32位字数（1024字节）
#define FLASH_MAX_PAGE_NUM          63U             // 路径数据页 1..63；第64页属于科目二
#define FLASH_OK                    1
#define FLASH_ERR                   0
#define FLASH_SECTOR_NUM            0

// 自动计算单条记录字数（对齐到 4 字节）
#define LOG_RECORD_WORDS            ((sizeof(imu_encoder_log_t) + 3U) / 4U)
#define FLASH_RECORDS_PER_PAGE      (FLASH_PAGE_LENGTH / LOG_RECORD_WORDS)

// 第0页保存Header，实际路径容量严格限制在第1..63页，不能写入科目二第64页。
#define FLASH_MAX_LOG_RECORDS       (FLASH_MAX_PAGE_NUM * FLASH_RECORDS_PER_PAGE)

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
// 底层页操作（使用 uint32_t * 接口，因为 Flash 是按字操作的）
uint8_t flash_safe_erase_page(uint32_t page);
uint8_t flash_safe_write_page(uint32_t page, const uint32_t *data, uint32_t word_count);
uint8_t flash_safe_read_page(uint32_t page, uint32_t *data, uint32_t word_count);

#endif // __FLASH_H__
