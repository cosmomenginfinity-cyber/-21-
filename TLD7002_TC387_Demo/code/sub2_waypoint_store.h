#ifndef SUB2_WAYPOINT_STORE_H
#define SUB2_WAYPOINT_STORE_H

#include "zf_common_typedef.h"
#include "sub2_navigation.h"

/*
 * The zf_driver_flash sector_num argument is a compatibility placeholder;
 * the physical 4 KB erase sector is selected by page_num. Subject 1 path
 * logs own pages 0..63. Subject 2 waypoints are fixed at page 64, so the
 * records cannot be erased by flash_log_clear_all().
 */
#define SUB2_WAYPOINT_FLASH_PAGE 64U
#define SUB2_WAYPOINT_FLASH_MAGIC 0x53554232U /* "SUB2" */

void flash_save_sub2_waypoints(void);
void flash_load_sub2_waypoints(void);
uint8_t flash_clear_sub2_waypoints(void);

#endif
