/*
 * YOS
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if (YFS_ON_ESP32 == 1)
#include "esp_partition.h"
#endif

#include "yfs.h"

#if (YFS_IS_WITH_LFS == 1)
#include "../lib/littlefs/lfs.h"

#include "yfs_data.h"
#include "yfs_ops_lfs.h"

struct yfs_lfs_data {
	struct lfs lfs_obj;
	struct lfs_config lfs_cfg;
	struct yfs_data *yfs;
};

static struct yfs_lfs_data yldata;

static int yfs_ops_lfs_read_from_file(const struct lfs_config *c,
					lfs_block_t block,
					lfs_off_t off,
					void *buffer,
					lfs_size_t size)
{
	if (fseek((FILE *)(yldata.yfs->media_handle), block * c->block_size + off, SEEK_SET) == 0) {
		if (fread(buffer, size, 1, (FILE *)(yldata.yfs->media_handle)) > 0) {
			return 0;
		}
	}

	return -1;
}

static int yfs_ops_lfs_read_from_disk(const struct lfs_config *c,
					lfs_block_t block,
					lfs_off_t off,
					void *buffer,
					lfs_size_t size)
{
	if (esp_partition_read((esp_partition_t *)(yldata.yfs->media_handle),
							block * c->block_size + off,
							buffer,
							size) != ESP_OK) {
		/* Partition read error */
		return -1;
	}

	return 0;
}

static int yfs_ops_lfs_prog_to_file(const struct lfs_config *c,
					lfs_block_t block,
					lfs_off_t off,
					const void *buffer,
					lfs_size_t size)
{
	if (fseek((FILE *)(yldata.yfs->media_handle), block * c->block_size + off, SEEK_SET) == 0) {
		if (fwrite(buffer, size, 1, (FILE *)(yldata.yfs->media_handle)) > 0) {
			return 0;
		}
	}

	return -1;
}

static int yfs_ops_lfs_prog_to_disk(const struct lfs_config *c,
					lfs_block_t block,
					lfs_off_t off,
					const void *buffer,
					lfs_size_t size)
{
	if (esp_partition_write((esp_partition_t *)(yldata.yfs->media_handle),
							block * c->block_size + off,
							buffer,
							size) != ESP_OK) {
		/* Partition write error */
		return -1;
	}

	return 0;
}


static int yfs_ops_lfs_erase_on_file(const struct lfs_config *c, lfs_block_t block)
{
	unsigned long b, e;
	const char erase_data[] = {0x01, 0x10};
	if (fseek((FILE *)(yldata.yfs->media_handle), block * c->block_size, SEEK_SET) == 0) {
		b = 0;
		e = 0;
		while (b < c->block_size) {
			if (e >= sizeof(erase_data) / sizeof(erase_data[0])) {
				e = 0;
			}
			fwrite(erase_data + e, sizeof(erase_data[0]), 1, (FILE *)(yldata.yfs->media_handle));
			b++;
			e++;
		}

		return 0;
	}

	return -1;
}

static int yfs_ops_lfs_erase_on_disk(const struct lfs_config *c, lfs_block_t block)
{
	if (esp_partition_erase_range((esp_partition_t *)(yldata.yfs->media_handle),
								block * c->block_size,
								c->block_size) != ESP_OK) {
		/* Partition erase error */
		return -1;
	}

	return 0;
}


static int yfs_ops_lfs_sync_to_file(const struct lfs_config *c)
{
	if (fflush((FILE *)(yldata.yfs->media_handle)) == 0) {
		return 0;
	}

	return -1;
}

static int yfs_ops_lfs_sync_to_disk(const struct lfs_config *c)
{
	/* Do nothing for sync disk partition */
	return 0;
}


static int yfs_load_lfs(struct yfs_data *data)
{
	if (data == NULL) {
		return -1;
	}

	memset(&yldata, 0x00, sizeof(yldata));
	yldata.lfs_cfg.block_size = data->block_size;
	yldata.lfs_cfg.block_count = data->block_count;
	yldata.lfs_cfg.read_size = DEFAULT_READ_SIZE;
	yldata.lfs_cfg.prog_size = DEFAULT_PROG_SIZE;
	yldata.lfs_cfg.cache_size = DEFAULT_CACHE_SIZE;
	yldata.lfs_cfg.block_cycles = DEFAULT_BLOCK_CYCLES;
	yldata.lfs_cfg.lookahead_size = DEFAULT_LOOKAHEAD_SIZE;
	if (data->media_type == YFS_MEDIA_TYPE_REGULAR_FILE) {
		yldata.lfs_cfg.read = yfs_ops_lfs_read_from_file;
		yldata.lfs_cfg.prog = yfs_ops_lfs_prog_to_file;
		yldata.lfs_cfg.erase = yfs_ops_lfs_erase_on_file;
		yldata.lfs_cfg.sync = yfs_ops_lfs_sync_to_file;
	} else if (data->media_type == YFS_MEDIA_TYPE_FLASH_DISK) {
		yldata.lfs_cfg.read = yfs_ops_lfs_read_from_disk;
		yldata.lfs_cfg.prog = yfs_ops_lfs_prog_to_disk;
		yldata.lfs_cfg.erase = yfs_ops_lfs_erase_on_disk;
		yldata.lfs_cfg.sync = yfs_ops_lfs_sync_to_disk;
	}

	yldata.yfs = data;

	if (data->media_type == YFS_MEDIA_TYPE_FLASH_DISK) {
		yldata.yfs->media_handle = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
															ESP_PARTITION_SUBTYPE_DATA_LITTLEFS,
															yldata.yfs->media_name);
		if (yldata.yfs->media_handle == NULL) {
			YFS_LOG("Media %s not found\n", yldata.yfs->media_name);
			return -2;
		}
	} else if (data->media_type == YFS_MEDIA_TYPE_REGULAR_FILE) {
		yldata.yfs->media_handle = fopen(yldata.yfs->media_name, "r+");
		if (yldata.yfs->media_handle == NULL) {
			YFS_LOG("Open file system data file[%s] failed\n", yldata.yfs->media_name);
			return -2;
		}
	}

	if (lfs_mount(&yldata.lfs_obj, &yldata.lfs_cfg) < 0) {
		YFS_LOG("Mount lfs failed\n");
		if (data->format_when_mount_err) {
			YFS_LOG("Format when mount err enabled, try format and mount...\n");
			if (lfs_format(&yldata.lfs_obj, &yldata.lfs_cfg) < 0) {
				YFS_LOG("Format lfs failed\n");
				goto mount_err;
			} else {
				YFS_LOG("Format lfs OK, try mount...\n");
				if (lfs_mount(&yldata.lfs_obj, &yldata.lfs_cfg) < 0) {
					YFS_LOG("Mount lfs failed again\n");
					goto mount_err;
				} else {
					YFS_LOG("Mount lfs OK\n");
				}
			}
		} else {
			goto mount_err;
		}
	}

	data->is_ready = 1;

	return 0;

mount_err:
	if (data->media_type == YFS_MEDIA_TYPE_FLASH_DISK) {
		/* Partition always exists so that no CLOSE is needed */
		yldata.yfs->media_handle = NULL;
	} else if (data->media_type == YFS_MEDIA_TYPE_REGULAR_FILE) {
		fclose((FILE *)(yldata.yfs->media_handle));
		yldata.yfs->media_handle = NULL;
	}
	return -3;
}


static int yfs_unload_lfs(struct yfs_data *data)
{
	int ret = 0;
	if (data == NULL) {
		return -1;
	}

	if (lfs_unmount(&yldata.lfs_obj) < 0) {
		YFS_LOG("Umount lfs failed\n");
		ret = -2;
	} else {
		data->is_ready = 0;
		if (data->media_type == YFS_MEDIA_TYPE_FLASH_DISK) {
			/* Partition always exists so that no CLOSE is needed */
			yldata.yfs->media_handle = NULL;
		} else if (data->media_type == YFS_MEDIA_TYPE_REGULAR_FILE) {
			if (fclose((FILE *)(yldata.yfs->media_handle)) != 0) {
				YFS_LOG("Close file system data file[%s] failed\n", yldata.yfs->media_name);
				ret = -3;
			}
		}
	}

	return ret;
}

static int yfs_fs_get_stat(struct yfs_fs_stat *stat)
{
	struct lfs_fsinfo fsinfo;
	if (lfs_fs_stat(&yldata.lfs_obj, &fsinfo) >= 0) {
		lfs_ssize_t fs_size = lfs_fs_size(&yldata.lfs_obj);
		if (stat != NULL) {
			stat->type = YFS_FS_TYPE_LFS;
			stat->block_size = fsinfo.block_size;
			stat->block_total = fsinfo.block_count;
			stat->file_name_max_bytes = fsinfo.name_max;
			stat->file_size_max_bytes = fsinfo.file_max;
			if (fs_size >= 0) {
				stat->block_allocated = fs_size;
			} else {
				stat->block_allocated = 0;
			}
		}
		return 0;
	} else {
		return -2;
	}
}

static int yfs_ops_lfs_fopen(char *filename, enum yfs_open_flags flags, struct yfs_file *yfile)
{
	if (filename == NULL || yfile == NULL) {
		return -1;
	}

	int lfs_flags = 0;
	if ((flags & YFS_O_RDONLY) == YFS_O_RDONLY) {
		lfs_flags |= LFS_O_RDONLY;
	}
	if ((flags & YFS_O_WRONLY) == YFS_O_WRONLY) {
		lfs_flags |= LFS_O_WRONLY;
	}
	if ((flags & YFS_O_RDWR) == YFS_O_RDWR) {
		lfs_flags |= LFS_O_RDWR;
	}
	if ((flags & YFS_O_CREAT) == YFS_O_CREAT) {
		lfs_flags |= LFS_O_CREAT;
	}
	if ((flags & YFS_O_EXCL) == YFS_O_EXCL) {
		lfs_flags |= LFS_O_EXCL;
	}
	if ((flags & YFS_O_TRUNC) == YFS_O_TRUNC) {
		lfs_flags |= LFS_O_TRUNC;
	}
	if ((flags & YFS_O_APPEND) == YFS_O_APPEND) {
		lfs_flags |= LFS_O_APPEND;
	}

	lfs_file_t *file = (lfs_file_t *)malloc(sizeof(lfs_file_t));
	if (file == NULL) {
		YFS_LOG("Allocate memory for file[%s] failed\n", filename);
		return -2;
	}

	yfile->data = file;

	if (lfs_file_open(&yldata.lfs_obj, file, filename, lfs_flags) >= 0) {
		return 0;
	} else {
		free(yfile->data);
		yfile->data = NULL;

		return -3;
	}
}


static int yfs_ops_lfs_fclose(struct yfs_file *yfile)
{
	int ret;
	if (yfile == NULL) {
		return -1;
	}
	lfs_file_t *file = (lfs_file_t *)yfile->data;
	if (file != NULL) {
		ret = lfs_file_close(&yldata.lfs_obj, file);
		if (ret >= 0) {
			free(file);
			yfile->data = NULL;
			ret = 0;
		}
	} else {
		return -2;
	}

	return ret;
}


static int yfs_ops_lfs_fread(struct yfs_file *yfile,
							void *dst,
							unsigned long size)
{
	if (yfile == NULL || dst == NULL) {
		return -1;
	}
	lfs_file_t *file = (lfs_file_t *)yfile->data;
	if (file == NULL) {
		return -2;
	}

	return lfs_file_read(&yldata.lfs_obj, file, dst, size);
}


static int yfs_ops_lfs_fwrite(struct yfs_file *yfile, void *src, unsigned long size)
{
	if (yfile == NULL || src == NULL) {
		return -1;
	}
	lfs_file_t *file = (lfs_file_t *)yfile->data;
	if (file == NULL) {
		return -2;
	}

	return lfs_file_write(&yldata.lfs_obj, file, src, size);
}


static int yfs_ops_lfs_fseek(struct yfs_file *yfile,
							unsigned long offset,
							enum yfs_whence_flags whence) {
	if (yfile == NULL) {
		return -1;
	}
	lfs_file_t *file = (lfs_file_t *)yfile->data;
	if (file == NULL) {
		return -2;
	}

	int lfs_whence;
	if (whence == YFS_SEEK_SET) {
		lfs_whence = LFS_SEEK_SET;
	} else if (whence == YFS_SEEK_CUR) {
		lfs_whence = LFS_SEEK_CUR;
	} else if (whence == YFS_SEEK_END) {
		lfs_whence = LFS_SEEK_END;
	} else {
		YFS_LOG("Invalid parameter for whence[%d]\n", whence);
		return -3;
	}

	return lfs_file_seek(&yldata.lfs_obj, file, offset, lfs_whence);
}


static int yfs_ops_lfs_fsync(struct yfs_file *yfile)
{
	if (yfile == NULL) {
		return -1;
	}
	lfs_file_t *file = (lfs_file_t *)yfile->data;
	if (file == NULL) {
		return -2;
	}

	return lfs_file_sync(&yldata.lfs_obj, file) < 0 ? -3 : 0;
}


static int yfs_ops_lfs_feof(struct yfs_file *yfile, int *iseof)
{
	if (yfile == NULL || iseof == NULL) {
		return -1;
	}
	lfs_file_t *file = (lfs_file_t *)yfile->data;
	if (file == NULL) {
		return -2;
	}

	lfs_soff_t current_pos = lfs_file_tell(&yldata.lfs_obj, file);
	lfs_soff_t file_size = lfs_file_size(&yldata.lfs_obj, file);
	if (current_pos >= 0 && file_size >= 0) {
		if (current_pos == file_size) {
			*iseof = 1;
		} else {
			*iseof = 0;
		}
	} else {
		return -3;
	}

	return 0;
}

static int yfs_ops_lfs_fremove(char *path)
{
	if (path == NULL) {
		return -1;
	}

	return lfs_remove(&yldata.lfs_obj, path) < 0 ? -2 : 0;
}


static int yfs_ops_lfs_fmove(char *oldpath, char *newpath)
{
	if (oldpath == NULL || newpath == NULL) {
		return -1;
	}

	return lfs_rename(&yldata.lfs_obj, oldpath, newpath) < 0 ? -2 : 0;
}


static int yfs_ops_lfs_fstat(char *path, struct yfs_file_info *finfo)
{
	if (path == NULL || finfo == NULL) {
		return -1;
	}

	struct lfs_info info;
	finfo->filename[0] = '\0';
	finfo->size = 0;
	finfo->type = YFS_FILE_TYPE_UNKNOWN;
	int ret = lfs_stat(&yldata.lfs_obj, path, &info);
	if (ret >= 0) {
		finfo->size = info.size;
		if (info.type == LFS_TYPE_REG) {
			finfo->type = YFS_FILE_TYPE_REGULAR;
		} else if (info.type == LFS_TYPE_DIR) {
			finfo->type = YFS_FILE_TYPE_FOLDER;
		} else {
			finfo->type = YFS_FILE_TYPE_UNKNOWN;
		}

		strcpy(finfo->filename, info.name);
		ret = 0;
	}

	return ret;
}

static int yfs_ops_lfs_ftrunc(struct yfs_file *yfile, unsigned long size)
{
	if (yfile == NULL) {
		return -1;
	}
	lfs_file_t *file = (lfs_file_t *)yfile->data;
	if (file == NULL) {
		return -2;
	}

	return lfs_file_truncate(&yldata.lfs_obj, file, size) < 0 ? -3 : 0;
}

static int yfs_ops_lfs_fsize(struct yfs_file *yfile)
{
	if (yfile == NULL) {
		return -1;
	}
	lfs_file_t *file = (lfs_file_t *)yfile->data;
	if (file == NULL) {
		return -2;
	}

	return lfs_file_size(&yldata.lfs_obj, file);
}

static int yfs_ops_lfs_diropen(char *path, struct yfs_dir *ydir)
{
	if (path == NULL || ydir == NULL) {
		return -1;
	}

	lfs_dir_t *dir = (lfs_dir_t *)malloc(sizeof(lfs_dir_t));
	if (dir == NULL) {
		return -2;
	}
	ydir->data = dir;

	if (lfs_dir_open(&yldata.lfs_obj, dir, path) >= 0) {
		return 0;
	} else {
		free(ydir->data);
		ydir->data = NULL;
		return -3;
	}
}


static int yfs_ops_lfs_dirclose(struct yfs_dir *ydir)
{
	int ret;
	if (ydir == NULL) {
		return -1;
	}
	lfs_dir_t *dir = (lfs_dir_t *)ydir->data;
	if (dir != NULL) {
		ret = lfs_dir_close(&yldata.lfs_obj, dir);
		if (ret >= 0) {
			free(dir);
			ydir->data = NULL;
			ret = 0;
		}
	} else {
		return -2;
	}

	return ret;
}


static int yfs_ops_lfs_dirmake(char *path)
{
	if (path == NULL) {
		return -1;
	}

	return lfs_mkdir(&yldata.lfs_obj, path) < 0 ? -2 : 0;
}


static int yfs_ops_lfs_dirread(struct yfs_dir *ydir, struct yfs_file_info *finfo)
{
	int ret;
	if (ydir == NULL || finfo == NULL) {
		return -1;
	}
	lfs_dir_t *dir = (lfs_dir_t *)ydir->data;
	if (dir == NULL) {
		return -2;
	}

	struct lfs_info info;
	ret = lfs_dir_read(&yldata.lfs_obj, dir, &info);
	if (ret >= 0) {
		finfo->size = info.size;

		if (info.type == LFS_TYPE_REG) {
			finfo->type = YFS_FILE_TYPE_REGULAR;
		} else if (info.type == LFS_TYPE_DIR) {
			finfo->type = YFS_FILE_TYPE_FOLDER;
		} else {
			finfo->type = YFS_FILE_TYPE_UNKNOWN;
		}

		strcpy(finfo->filename, info.name);
	}

	return ret;
}


static struct yfs_ops yfs_ops_lfs = {
	.yfs_load = yfs_load_lfs,
	.yfs_unload = yfs_unload_lfs,
	.yfs_stat = yfs_fs_get_stat,

	.yfopen = yfs_ops_lfs_fopen,
	.yfclose = yfs_ops_lfs_fclose,
	.yfread = yfs_ops_lfs_fread,
	.yfwrite = yfs_ops_lfs_fwrite,
	.yfseek = yfs_ops_lfs_fseek,
	.yfsync = yfs_ops_lfs_fsync,
	.yfeof = yfs_ops_lfs_feof,
	.yfremove = yfs_ops_lfs_fremove,
	.yfmove = yfs_ops_lfs_fmove,
	.yfstat = yfs_ops_lfs_fstat,
	.yftrunc = yfs_ops_lfs_ftrunc,
	.yfsize = yfs_ops_lfs_fsize,

	.ydiropen = yfs_ops_lfs_diropen,
	.ydirclose = yfs_ops_lfs_dirclose,
	.ydirmake = yfs_ops_lfs_dirmake,
	.ydirread = yfs_ops_lfs_dirread
};


int yfs_register_lfs(struct yfs_data *yfs)
{
	if (yfs == NULL) {
		return -1;
	}

	yfs->ops = &yfs_ops_lfs;
	return 0;
}

#endif
