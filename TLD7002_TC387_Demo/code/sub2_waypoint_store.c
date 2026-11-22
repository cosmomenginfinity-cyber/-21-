#include "sub2_waypoint_store.h"
#include "flash.h"
#include "zf_driver_flash.h"
#include <string.h>

void flash_save_sub2_waypoints(void)
{
    /* Subject 1 path logs use pages 0..63; Subject 2 is fixed at page 64. */
    flash_erase_page(FLASH_SECTOR_NUM, SUB2_WAYPOINT_FLASH_PAGE);
    flash_buffer_clear();
    flash_union_buffer[0].uint32_type = SUB2_WAYPOINT_FLASH_MAGIC;
    flash_union_buffer[1].uint32_type = (uint32)WP_NUM;
    memcpy(&flash_union_buffer[2], g_sub2_waypoints, sizeof(g_sub2_waypoints));
    flash_write_page_from_buffer(FLASH_SECTOR_NUM, SUB2_WAYPOINT_FLASH_PAGE);
}

void flash_load_sub2_waypoints(void)
{
    if (flash_check(FLASH_SECTOR_NUM, SUB2_WAYPOINT_FLASH_PAGE) == 0)
    {
        memset(g_sub2_waypoints, 0, sizeof(g_sub2_waypoints));
        return;
    }

    flash_read_page_to_buffer(FLASH_SECTOR_NUM, SUB2_WAYPOINT_FLASH_PAGE);
    if (flash_union_buffer[0].uint32_type != SUB2_WAYPOINT_FLASH_MAGIC ||
        flash_union_buffer[1].uint32_type != (uint32)WP_NUM)
    {
        memset(g_sub2_waypoints, 0, sizeof(g_sub2_waypoints));
        return;
    }

    memcpy(g_sub2_waypoints, &flash_union_buffer[2], sizeof(g_sub2_waypoints));
}

uint8_t flash_clear_sub2_waypoints(void)
{
    flash_erase_page(FLASH_SECTOR_NUM, SUB2_WAYPOINT_FLASH_PAGE);
    memset(g_sub2_waypoints, 0, sizeof(g_sub2_waypoints));
    return FLASH_OK;
}
