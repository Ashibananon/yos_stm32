/*
 * YOS
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#include "yfs_data.h"
#include "yfs.h"

#if (YFS_IS_WITH_LFS == 1)
#define FS_BLOCK_SIZE		4096
#define FS_BLOCK_COUNT		752
#define FS_MEDIA_NAME		"pobdata"
#elif (YFS_IS_WITH_FAT == 1)
#define FS_BLOCK_SIZE		4096
#define FS_BLOCK_COUNT		752
#define FS_MEDIA_NAME		""
#define FS_MOUNT_PATH		"/sd"
#include "yfs_ops_fat.h"
#endif

static struct yfs_data yfs = {0};

struct yfs_data *YFS_Data = NULL;

int yfs_start(void)
{
#if (YFS_IS_WITH_LFS == 1)
	if (yfs_register(&yfs, YFS_FS_TYPE_LFS, YFS_MEDIA_TYPE_FLASH_DISK,
					FS_BLOCK_SIZE, FS_BLOCK_COUNT, 1, FS_MEDIA_NAME) == 0) {
		if (yfs_init(&yfs) == 0) {
			YFS_Data = &yfs;
			return 0;
		}
	}
#elif (YFS_IS_WITH_FAT == 1)
	if (yfs_register(&yfs, YFS_FS_TYPE_FAT, YFS_MEDIA_TYPE_FLASH_DISK,
					FS_BLOCK_SIZE, FS_BLOCK_COUNT, 0, FS_MOUNT_PATH, yfs_ops_fat) == 0) {
		if (yfs_init(&yfs) == 0) {
			YFS_Data = &yfs;
			return 0;
		}
	}
#endif

	return -1;
}

int yfs_end(void)
{
	if (yfs_deinit(&yfs) != 0) {
		YFS_DBG("yfs_deinit error\n");
	}

	if (yfs_unregister(&yfs) != 0) {
		YFS_DBG("yfs_unregister error\n");
	}

	YFS_Data = NULL;

	return 0;
}

int yfs_status(struct yfs_fs_stat *yfs_stat)
{
	int ret = -1;
	if (yfs_get_status(YFS_Data, yfs_stat) == 0) {
		ret = 0;
	}

	return ret;
}