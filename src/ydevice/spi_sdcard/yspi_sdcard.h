/*
 * YOS
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#ifndef _YOS_SDCARD_H_
#define _YOS_SDCARD_H_

#ifdef __cplusplus
extern "C" {
#endif

#define YOS_SD_DEBUG_MSG_OUTPUT		0
#define YOS_SD_LIBRARY_MSG_OUTPUT	1

#include <stdint.h>
#include "../../lib/cmdline/basic_io.h"

struct ysdcard_info {
	uint8_t type;
	uint32_t sector_size;
	uint32_t sector_num;
	uint32_t erase_block_size;
};

int ysdcard_init(void);
int ysdcard_deinit(void);
int ysdcard_is_ready(void);

int ysdcard_get_sdcard_info(struct ysdcard_info *sdinfo);
int ysdcard_read(uint32_t sector, uint32_t offset, uint8_t *buf, uint32_t read_len);
int ysdcard_write(uint32_t sector, uint32_t offset, uint8_t *data, uint32_t write_len);


#if (YOS_SD_DEBUG_MSG_OUTPUT == 1)
#define YSD_DBG(...)						basic_io_printf("[YSD]"__VA_ARGS__)
#define YSD_DBG_DUMP_DATA(data, len)		basic_io_dump_hex(data, len, 16, " ", 1, " => ");
#else
#define YSD_DBG(...)
#define YSD_DBG_DUMP_DATA(data, len)
#endif

#ifdef __cplusplus
}
#endif
#endif
