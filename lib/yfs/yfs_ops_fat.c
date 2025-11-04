/*
 * YOS
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#include "yfs_ops_fat.h"

#if (YFS_IS_WITH_FAT == 1)
#include "../ff16/source/ff.h"
#include "../ff16/source/ffconf.h"

static FATFS _fatfs;

static int yfs_load_fat(struct yfs_data *data)
{
	int ret = -1;
	if (data == NULL) {
		goto para_err;
	}

	FRESULT fret = f_mount(&_fatfs, data->media_name, 1);
	if (fret != FR_OK) {
		YFS_DBG("f_mount returned %d\n", fret);
		goto mount_err;
	}

	data->lower_fs_obj = &_fatfs;

	ret = 0;

mount_err:
para_err:
	return ret;
}

static int yfs_unload_fat(struct yfs_data *data)
{
	int ret = -1;
	if (data == NULL) {
		goto para_err;
	}

	FRESULT fret = f_mount(NULL, data->media_name, 1);
	if (fret != FR_OK) {
		YFS_DBG("umount returned %d\n", fret);
		goto unmount_err;
	}

	data->lower_fs_obj = NULL;

	ret = 0;

unmount_err:
para_err:
	return ret;
}

static int yfs_stat_fat(struct yfs_data *data, struct yfs_fs_stat *stat)
{
	int ret = -1;
	if (data == NULL || data->lower_fs_obj == NULL) {
		goto para_err;
	}

	if (stat != NULL) {
		stat->type = data->fs_type;
		stat->sub_type = ((FATFS *)data->lower_fs_obj)->fs_type;
		stat->block_size = ((FATFS *)data->lower_fs_obj)->csize * FF_MAX_SS;
		stat->block_total = ((FATFS *)data->lower_fs_obj)->n_fatent;
		stat->block_allocated = ((FATFS *)data->lower_fs_obj)->last_clst;
		stat->file_name_max_bytes = FF_LFN_BUF;
		stat->file_size_max_bytes = 0;
		strncpy(stat->mount_point, data->media_name, sizeof(stat->mount_point));
		stat->mount_point[sizeof(stat->mount_point) - 1] = '\0';
	}

	ret = 0;

fat_not_ready:
para_err:
	return ret;
}

static int yfs_ops_fat_fopen(char *filename, enum yfs_open_flags flags, struct yfs_file *yfile)
{
	int ret = -1;
	if (filename == NULL || yfile == NULL) {
		goto para_err;
	}

	FIL *fp;
	BYTE mode = 0;
	if (flags & YFS_O_RDONLY) {
		mode |= FA_READ;
	}
	if (flags & YFS_O_WRONLY) {
		mode |= FA_WRITE;
	}
	if (flags & YFS_O_CREAT) {
		mode |= (FA_CREATE_NEW);
	}
	if (flags & YFS_O_EXCL) {
		mode |= FA_CREATE_NEW;
	}
	if (flags & YFS_O_TRUNC) {
		mode |= (FA_CREATE_NEW | FA_CREATE_ALWAYS);
	}
	if (flags & YFS_O_APPEND) {
		mode |= FA_OPEN_APPEND;
	}

	fp = (FIL *)malloc(sizeof(FIL));
	if (fp == NULL) {
		goto fp_malloc_err;
	}

	FRESULT fret = f_open(fp, filename, mode);
	if (fret != FR_OK) {
		goto fopen_err;
	}

	if (flags & YFS_O_TRUNC) {
		fret = f_truncate(fp);
		if (fret != FR_OK) {
			goto trunc_err;
		}
	}

	yfile->data = fp;
	strncpy(yfile->fname, filename, sizeof(yfile->fname));
	yfile->fname[sizeof(yfile->fname) - 1] = '\0';

	ret = 0;

	return ret;

trunc_err:
fopen_err:
	free(fp);
fp_malloc_err:
para_err:
	return ret;
}

static int yfs_ops_fat_fclose(struct yfs_file *yfile)
{
	int ret = -1;
	if (yfile == NULL || yfile->data == NULL) {
		goto para_err;
	}

	FRESULT fret = f_close(yfile->data);
	if (fret == FR_OK) {
		free(yfile->data);
		yfile->data = NULL;
		yfile->fname[0] = '\0';
		ret = 0;
	}

para_err:
	return ret;
}

static uint32_t yfs_ops_fat_fread(struct yfs_file *yfile, void *dst, uint32_t size)
{
	uint32_t bytes_read = 0;
	if (yfile == NULL || yfile->data == NULL || dst == NULL || size == 0) {
		goto para_err;
	}

	FRESULT fret = f_read(yfile->data, dst, size, &bytes_read);
	if (fret != FR_OK) {
		/* Read error */
	}

para_err:
	return bytes_read;
}

static uint32_t yfs_ops_fat_fwrite(struct yfs_file *yfile, void *src, uint32_t size)
{
	uint32_t bytes_written = 0;
	if (yfile == NULL || yfile->data == NULL || src == NULL || size == 0) {
		goto para_err;
	}

	FRESULT fret = f_write(yfile->data, src, size, &bytes_written);
	if (fret != FR_OK) {
		/* Write error */
	}

para_err:
	return bytes_written;
}

static int yfs_ops_fat_fseek(struct yfs_file *yfile, uint32_t offset, enum yfs_whence_flags whence)
{
	int ret = -1;
	if (yfile == NULL || yfile->data == NULL) {
		goto para_err;
	}

	FSIZE_t pos;
	FRESULT fret;
	FILINFO finfo;
	fret = f_stat(yfile->fname, &finfo);
	if (fret != FR_OK) {
		goto file_stat_err;
	}

	if (whence == YFS_SEEK_SET) {
		pos = offset;
	} else if (whence == YFS_SEEK_CUR) {
		goto not_support;
	} else if (whence == YFS_SEEK_END) {
		pos = finfo.fsize - offset;
	} else {
		goto para_err_2;
	}

	fret = f_lseek(yfile->data, pos);
	if (fret == FR_OK) {
		ret = 0;
	}

para_err_2:
not_support:
file_stat_err:
para_err:
	return ret;
}

static int yfs_ops_fat_fsync(struct yfs_file *yfile)
{
	int ret = -1;
	if (yfile == NULL || yfile->data == NULL) {
		goto para_err;
	}

	FRESULT fret = f_sync(yfile->data);
	if (fret == FR_OK) {
		ret = 0;
	}

para_err:
	return ret;
}

static int yfs_ops_fat_feof(struct yfs_file *yfile, int *iseof)
{
	int ret = -1;
	if (yfile == NULL || yfile->data == NULL) {
		goto para_err;
	}

	if (iseof != NULL) {
		if (f_eof((FIL *)(yfile->data))) {
			*iseof = 1;
		} else {
			*iseof = 0;
		}
	}

	ret = 0;

para_err:
	return ret;
}

static int yfs_ops_fat_fremove(char *path)
{
	int ret = -1;
	if (path == NULL) {
		goto para_err;
	}

	FRESULT fret = f_unlink(path);
	if (fret == FR_OK) {
		ret = 0;
	}

para_err:
	return ret;
}

static int yfs_ops_fat_fmove(char *oldpath, char *newpath)
{
	int ret = -1;
	if (oldpath == NULL || newpath == NULL) {
		goto para_err;
	}

	FRESULT fret = f_rename(oldpath, newpath);
	if (fret == FR_OK) {
		ret = 0;
	}

para_err:
	return ret;
}

static int yfs_ops_fat_fstat(char *path, struct yfs_file_info *finfo)
{
	int ret = -1;
	if (path == NULL || finfo == NULL) {
		goto para_err;
	}

	finfo->filename[0] = '\0';
	finfo->type = YFS_FILE_TYPE_UNKNOWN;
	finfo->size = 0;

	FILINFO fatfinfo;
	FRESULT fret = f_stat(path, &fatfinfo);
	if (fret == FR_OK) {
		/* Regular file */
		strncpy(finfo->filename, fatfinfo.fname, sizeof(finfo->filename));
		finfo->filename[sizeof(finfo->filename) - 1] = '\0';

		finfo->size = fatfinfo.fsize;

		if (fatfinfo.fattrib == AM_DIR) {
			finfo->type = YFS_FILE_TYPE_FOLDER;
		} else {
			finfo->type = YFS_FILE_TYPE_REGULAR;
		}

		ret = 0;
	}

para_err:
	return ret;
}

static int yfs_ops_fat_ftrunc(struct yfs_file *yfile, uint32_t size)
{
	int ret = -1;
	if (yfile == NULL || yfile->data == NULL) {
		goto para_err;
	}

	FRESULT fret = f_truncate(yfile->data);
	if (fret == FR_OK) {
		ret = 0;
	}

para_err:
	return ret;
}

static int yfs_ops_fat_fsize(struct yfs_file *yfile, uint32_t *size)
{
	int ret = -1;
	if (yfile == NULL) {
		goto para_err;
	}

	struct yfs_file_info yfinfo;
	int yret = yfs_ops_fat_fstat(yfile->fname, &yfinfo);
	if (yret == 0) {
		if (yfinfo.type == YFS_FILE_TYPE_REGULAR) {
			if (size != NULL) {
				*size = yfinfo.size;
			}

			ret = 0;
		}
	}

para_err:
	return ret;
}

static int yfs_ops_fat_diropen(char *path, struct yfs_dir *ydir)
{
	int ret = -1;
	if (path == NULL || ydir == NULL) {
		goto para_err;
	}

	DIR *fat_dir = (DIR *)malloc(sizeof(DIR));
	if (fat_dir == NULL) {
		goto fat_dir_malloc_err;
	}
	FRESULT fret = f_opendir(fat_dir, path);
	if (fret != FR_OK) {
		goto fat_open_dir_err;
	}

	ydir->data = fat_dir;
	ret = 0;

	return ret;

fat_open_dir_err:
	free(fat_dir);
fat_dir_malloc_err:
para_err:
	return ret;
}

static int yfs_ops_fat_dirclose(struct yfs_dir *ydir)
{
	int ret = -1;
	if (ydir == NULL || ydir->data == NULL) {
		goto para_err;
	}

	FRESULT fret = f_closedir(ydir->data);
	if (fret == FR_OK) {
		free(ydir->data);
		ydir->data = NULL;
		ret = 0;
	}

para_err:
	return ret;
}

static int yfs_ops_fat_dirmake(char *path)
{
	int ret = -1;
	if (path == NULL) {
		goto para_err;
	}

	FRESULT fret = f_mkdir(path);
	if (fret == FR_OK) {
		ret = 0;
	}

para_err:
	return ret;
}

static int yfs_ops_fat_dirread(struct yfs_dir *ydir, struct yfs_file_info *finfo)
{
	int ret = -1;
	if (ydir == NULL || ydir->data == NULL) {
		goto para_err;
	}

	FILINFO fat_file_info;
	FRESULT fret = f_readdir(ydir->data, &fat_file_info);
	if (fret == FR_OK) {
		if (fat_file_info.fname[0] == '\0') {
			goto end_of_dir;
		}
		if (finfo != NULL) {
			strncpy(finfo->filename, fat_file_info.fname, sizeof(finfo->filename));
			finfo->filename[sizeof(finfo->filename) - 1] = '\0';

			finfo->size = fat_file_info.fsize;

			if (fat_file_info.fattrib == AM_DIR) {
				finfo->type = YFS_FILE_TYPE_FOLDER;
			} else {
				finfo->type = YFS_FILE_TYPE_REGULAR;
			}
		}

		ret = 0;
	}

end_of_dir:
para_err:
	return ret;
}

static struct yfs_ops _yfs_ops_fat = {
	.yfs_load = yfs_load_fat,
	.yfs_unload = yfs_unload_fat,
	.yfs_stat = yfs_stat_fat,
	.yfopen = yfs_ops_fat_fopen,
	.yfclose = yfs_ops_fat_fclose,
	.yfread = yfs_ops_fat_fread,
	.yfwrite = yfs_ops_fat_fwrite,
	.yfseek = yfs_ops_fat_fseek,
	.yfsync = yfs_ops_fat_fsync,
	.yfeof = yfs_ops_fat_feof,
	.yfremove = yfs_ops_fat_fremove,
	.yfmove = yfs_ops_fat_fmove,
	.yfstat = yfs_ops_fat_fstat,
	.yftrunc = yfs_ops_fat_ftrunc,
	.yfsize = yfs_ops_fat_fsize,
	.ydiropen = yfs_ops_fat_diropen,
	.ydirclose = yfs_ops_fat_dirclose,
	.ydirmake = yfs_ops_fat_dirmake,
	.ydirread = yfs_ops_fat_dirread
};

struct yfs_ops *yfs_ops_fat = &_yfs_ops_fat;

#endif
