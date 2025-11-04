/*
 * YOS
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#ifndef _YFS_H_
#define _YFS_H_

#define YFS_IS_WITH_LFS		0
#define YFS_IS_WITH_FAT		1

#include "yfs_data.h"

#define YFS_DBG_MSG_OUTPUT	1

#ifdef __cplusplus
extern "C" {
#endif

extern struct yfs_data *YFS_Data;

int yfs_start(void);
int yfs_end(void);
int yfs_status(struct yfs_fs_stat *yfs_stat);

#if (YFS_DBG_MSG_OUTPUT == 1)
#include "../cmdline/basic_io.h"

#define YFS_DBG(...)		basic_io_printf("[YFS]" __VA_ARGS__)
#else
#define YFS_DBG(...)
#endif

#ifdef __cplusplus
}
#endif

#endif
