/*
 * YOS
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#include <stdio.h>
#include <string.h>

#include "yfs_data.h"
#include "yfs_ops_lfs.h"

int yfs_register(struct yfs_data *yfs,
				enum yfs_fs_type fs_type,
				enum yfs_media_type media_type,
				unsigned long block_size,
				unsigned long block_count,
				int format_when_mount_err,
				const char *media_name,
				struct yfs_ops *ops)
{
	int ret = -1;

	if (fs_type <= YFS_FS_TYPE_UNKNOWN || fs_type >= YFS_FS_TYPE_MAX ||
		yfs == NULL || block_size <= 0 || block_count <= 0 || ops == NULL) {
		goto para_err;
	}
	if (media_type <= YFS_MEDIA_TYPE_INVALID
		|| media_type >= YFS_MEDIA_TYPE_MAX) {
		goto para_err;
	}
	if (yfs->ops != NULL) {
		goto already_registered_err;
	}

	memset(yfs, 0x00, sizeof(struct yfs_data));

	yfs->media_type = media_type;
	yfs->block_size = block_size;
	yfs->block_count = block_count;

	yfs->format_when_mount_err = format_when_mount_err;

	if (media_name != NULL) {
		strncpy(yfs->media_name, media_name, sizeof(yfs->media_name));
		yfs->media_name[sizeof(yfs->media_name) - 1] = '\0';
	}

	yfs->fs_type = fs_type;
	yfs->ops = ops;

	ret = 0;

already_registered_err:
para_err:
	return ret;
}


int yfs_unregister(struct yfs_data *yfs)
{
	if (yfs == NULL) {
		return -1;
	}

	yfs->ops = NULL;
	yfs->media_name[0] = '\0';

	return 0;
}

int yfs_init(struct yfs_data *yfs)
{
	if (yfs == NULL) {
		return -1;
	}
	if (yfs->ops == NULL || yfs->ops->yfs_load == NULL) {
		return -2;
	}

	if (yfs->ops->yfs_load(yfs) == 0) {
		return 0;
	} else {
		return -3;
	}
}

int yfs_deinit(struct yfs_data *yfs)
{
	if (yfs == NULL) {
		return -1;
	}
	if (yfs->ops == NULL || yfs->ops->yfs_unload == NULL) {
		return -2;
	}

	if (yfs->ops->yfs_unload(yfs) == 0) {
		return 0;
	} else {
		return -1;
	}
}

int yfs_get_status(struct yfs_data *yfs, struct yfs_fs_stat *stat)
{
	if (yfs == NULL || yfs->ops == NULL || yfs->ops->yfs_stat == NULL) {
		return -1;
	}

	return yfs->ops->yfs_stat(yfs, stat);
}

int yfs_fopen(struct yfs_data *yfs, struct yfs_file *yfile, char *filename, enum yfs_open_flags flags)
{
	if (yfs == NULL || yfs->ops == NULL || yfs->ops->yfopen == NULL) {
		return -1;
	}

	return yfs->ops->yfopen(filename, flags, yfile);
}

int yfs_fclose(struct yfs_data *yfs, struct yfs_file *yfile)
{
	if (yfs == NULL || yfs->ops == NULL || yfs->ops->yfclose == NULL) {
		return -1;
	}

	return yfs->ops->yfclose(yfile);
}

uint32_t yfs_fwrite(struct yfs_data *yfs, struct yfs_file *yfile, void *src, uint32_t size)
{
	if (yfs == NULL || yfs->ops == NULL || yfs->ops->yfwrite == NULL) {
		return 0;
	}

	return yfs->ops->yfwrite(yfile, src, size);
}

uint32_t yfs_fread(struct yfs_data *yfs, struct yfs_file *yfile, void *dst, uint32_t size)
{
	if (yfs == NULL || yfs->ops == NULL || yfs->ops->yfread == NULL) {
		return 0;
	}

	return yfs->ops->yfread(yfile, dst, size);
}

int yfs_fseek(struct yfs_data *yfs, struct yfs_file *yfile, int64_t offset, enum yfs_whence_flags whence)
{
	if (yfs == NULL || yfs->ops == NULL || yfs->ops->yfseek == NULL) {
		return -1;
	}

	return yfs->ops->yfseek(yfile, offset, whence);
}

int64_t yfs_ftell(struct yfs_data *yfs, struct yfs_file *yfile)
{
	if (yfs == NULL || yfs->ops == NULL || yfs->ops->yftell == NULL) {
		return -1;
	}

	return yfs->ops->yftell(yfile);
}

int yfs_fsync(struct yfs_data *yfs, struct yfs_file *yfile)
{
	if (yfs == NULL || yfs->ops == NULL || yfs->ops->yfsync == NULL) {
		return -1;
	}

	return yfs->ops->yfsync(yfile);
}

int yfs_feof(struct yfs_data *yfs, struct yfs_file *yfile, int *is_eof)
{
	if (yfs == NULL || yfs->ops == NULL || yfs->ops->yfeof == NULL) {
		return -1;
	}

	return yfs->ops->yfeof(yfile, is_eof);
}

int yfs_fremove(struct yfs_data *yfs, char *path)
{
	if (yfs == NULL || yfs->ops == NULL || yfs->ops->yfremove == NULL) {
		return -1;
	}

	return yfs->ops->yfremove(path);
}

int yfs_fmove(struct yfs_data *yfs, char *oldpath, char *newpath)
{
	if (yfs == NULL || yfs->ops == NULL || yfs->ops->yfmove == NULL) {
		return -1;
	}

	return yfs->ops->yfmove(oldpath, newpath);
}

int yfs_fstat(struct yfs_data *yfs, char *path, struct yfs_file_info *finfo)
{
	if (yfs == NULL || yfs->ops == NULL || yfs->ops->yfstat == NULL) {
		return -1;
	}

	return yfs->ops->yfstat(path, finfo);
}

int yfs_ftrunc(struct yfs_data *yfs, struct yfs_file *yfile, uint32_t size)
{
	if (yfs == NULL || yfs->ops == NULL || yfs->ops->yftrunc == NULL) {
		return -1;
	}

	return yfs->ops->yftrunc(yfile, size);
}

int yfs_fsize(struct yfs_data *yfs, struct yfs_file *yfile, uint32_t *size)
{
	if (yfs == NULL || yfs->ops == NULL || yfs->ops->yfsize == NULL) {
		return 0;
	}

	return yfs->ops->yfsize(yfile, size);
}

int yfs_diropen(struct yfs_data *yfs, struct yfs_dir *ydir, char *path)
{
	if (yfs == NULL || yfs->ops == NULL || yfs->ops->ydiropen == NULL) {
		return -1;
	}

	return yfs->ops->ydiropen(path, ydir);
}

int yfs_dirclose(struct yfs_data *yfs, struct yfs_dir *ydir)
{
	if (yfs == NULL || yfs->ops == NULL || yfs->ops->ydirclose == NULL) {
		return -1;
	}

	return yfs->ops->ydirclose(ydir);
}

int yfs_dirmake(struct yfs_data *yfs, char *path)
{
	if (yfs == NULL || yfs->ops == NULL || yfs->ops->ydirmake == NULL) {
		return -1;
	}

	return yfs->ops->ydirmake(path);
}

int yfs_dirread(struct yfs_data *yfs, struct yfs_dir *ydir, struct yfs_file_info *finfo)
{
	if (yfs == NULL || yfs->ops == NULL || yfs->ops->ydirread == NULL) {
		return -1;
	}

	return yfs->ops->ydirread(ydir, finfo);
}
