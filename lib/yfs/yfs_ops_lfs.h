/*
 * YOS
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#ifndef _YFS_OPS_LFS_H_
#define _YFS_OPS_LFS_H_

#include "yfs.h"

#if (YFS_IS_WITH_LFS == 1)

#if (HAS_LOGGER == 1)
#include "../cmdline/logger.h"
#define YFS_LOG_ENABLE				1
#endif


#define DEFAULT_BLOCK_SIZE				4096
#define DEFAULT_READ_SIZE				4096
#define DEFAULT_PROG_SIZE				4096
#define DEFAULT_CACHE_SIZE				4096
#define DEFAULT_LOOKAHEAD_SIZE			4096

#define DEFAULT_BLOCK_CYCLES			100

#ifdef __cplusplus
extern "C" {
#endif

int yfs_register_lfs(struct yfs_data *yfs);

#if (YFS_LOG_ENABLE == 1)
 #define YFS_LOG(...)		LoggerLog("[YFS]" __VA_ARGS__)
#else
 #define YFS_LOG(...)
#endif

#ifdef __cplusplus
}
#endif
#endif
#endif
