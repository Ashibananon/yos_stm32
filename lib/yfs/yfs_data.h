/*
 * YOS
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#ifndef _YFS_DATA_H_
#define _YFS_DATA_H_

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define YFS_MEDIA_NAME_MAX_LENGTH	32
#define YFS_FILE_NAME_MAX_LENGTH	256

enum yfs_fs_type {
	YFS_FS_TYPE_UNKNOWN = -1,
	YFS_FS_TYPE_FAT,

	/* Little FS */
	YFS_FS_TYPE_LFS,
	YFS_FS_TYPE_MAX
};

enum yfs_file_type {
	YFS_FILE_TYPE_UNKNOWN = -1,
	YFS_FILE_TYPE_REGULAR,
	YFS_FILE_TYPE_FOLDER,
	YFS_FILE_TYPE_MAX
};

struct yfs_fs_stat {
	enum yfs_fs_type type;
	int sub_type;
	uint32_t block_size;
	uint32_t block_total;
	uint32_t block_allocated;
	uint32_t file_name_max_bytes;
	uint32_t file_size_max_bytes;
	char mount_point[YFS_MEDIA_NAME_MAX_LENGTH];
};

struct yfs_file_info {
	char filename[YFS_FILE_NAME_MAX_LENGTH];
	uint32_t size;
	enum yfs_file_type type;
};

enum yfs_open_flags {
	YFS_O_RDONLY = 1,		 // Open a file as read only
	YFS_O_WRONLY = 2,		 // Open a file as write only
	YFS_O_RDWR   = 3,		 // Open a file as read and write
	YFS_O_CREAT  = 0x0100,	// Create a file if it does not exist
	YFS_O_EXCL   = 0x0200,	// Fail if a file already exists
	YFS_O_TRUNC  = 0x0400,	// Truncate the existing file to zero size
	YFS_O_APPEND = 0x0800,	// Move to end of file on every write
};

enum yfs_whence_flags {
	YFS_SEEK_SET = 0,   // Seek relative to an absolute position
	YFS_SEEK_CUR = 1,   // Seek relative to the current file position
	YFS_SEEK_END = 2,   // Seek relative to the end of the file
};


struct yfs_data;

struct yfs_file {
	void *data;
	char fname[YFS_FILE_NAME_MAX_LENGTH];
};

struct yfs_dir {
	void *data;
};

struct yfs_ops {
	/*
	 * YFS load lower FS
	 * Return
	 * 0: OK
	 * Other: Error
	 */
	int (*yfs_load)(struct yfs_data *data);

	/*
	 * YFS unload lower FS
	 * Return
	 * 0: OK
	 * Other: Error
	 */
	int (*yfs_unload)(struct yfs_data *data);

	/*
	 * YFS get FS stat info
	 * Return
	 * 0: OK
	 * Other: Error
	 */
	int (*yfs_stat)(struct yfs_data *data, struct yfs_fs_stat *stat);

	/*
	 * YFS open file
	 * Return
	 * 0: OK
	 * Other: Error
	 */
	int (*yfopen)(char *filename, enum yfs_open_flags flags, struct yfs_file *yfile);

	/*
	 * YFS close file
	 * Return
	 * 0: OK
	 * Other: Error
	 */
	int (*yfclose)(struct yfs_file *yfile);

	/*
	 * YFS read file
	 * Return the bytes read from file
	 */
	uint32_t (*yfread)(struct yfs_file *yfile, void *dst, uint32_t size);

	/*
	 * YFS write file
	 * Return the bytes written to file
	 */
	uint32_t (*yfwrite)(struct yfs_file *yfile, void *src, uint32_t size);

	/*
	 * YFS seek file
	 * Return
	 * 0: OK
	 * Other: Error
	 *
	 * NOTE: If [whence] is YFS_SEEK_END, it will seek back [offset] bytes from EOF
	 */
	int (*yfseek)(struct yfs_file *yfile, int64_t offset, enum yfs_whence_flags whence);

	/*
	 * YFS get file pointer position
	 *
	 * Return: the file pointer position, or a minus value on error
	 */
	int64_t (*yftell)(struct yfs_file *yfile);

	/*
	 * YFS sync file
	 * Return
	 * 0: OK
	 * Other: Error
	 */
	int (*yfsync)(struct yfs_file *yfile);

	/*
	 * YFS check EOF
	 * Return
	 * 0: OK
	 * Other: Error
	 */
	int (*yfeof)(struct yfs_file *yfile, int *iseof);

	/*
	 * YFS remove file
	 * Return
	 * 0: OK
	 * Other: Error
	 */
	int (*yfremove)(char *path);

	/*
	 * YFS move file
	 * Return
	 * 0: OK
	 * Other: Error
	 */
	int (*yfmove)(char *oldpath, char *newpath);

	/*
	 * YFS get file stat
	 * Return
	 * 0: OK
	 * Other: Error
	 */
	int (*yfstat)(char *path, struct yfs_file_info *finfo);

	/*
	 * YFS trunc file to specified size
	 * Return
	 * 0: OK
	 * Other: Error
	 */
	int (*yftrunc)(struct yfs_file *yfile, uint32_t size);

	/*
	 * YFS get file size
	 * Return:
	 * 0: OK
	 * Other: Error
	 */
	int (*yfsize)(struct yfs_file *yfile, uint32_t *size);

	/*
	 * YFS open directory
	 * Return
	 * 0: OK
	 * Other: Error
	 */
	int (*ydiropen)(char *path, struct yfs_dir *ydir);

	/*
	 * YFS close directory
	 * Return
	 * 0: OK
	 * Other: Error
	 */
	int (*ydirclose)(struct yfs_dir *ydir);

	/*
	 * YFS make directory
	 * Return
	 * 0: OK
	 * Other: Error
	 */
	int (*ydirmake)(char *path);

	/*
	 * YFS read directory
	 * Return
	 * 0: OK
	 * Other: Error
	 */
	int (*ydirread)(struct yfs_dir *ydir, struct yfs_file_info *finfo);
};

enum yfs_media_type {
	YFS_MEDIA_TYPE_INVALID = -1,
	YFS_MEDIA_TYPE_FLASH_DISK,
	YFS_MEDIA_TYPE_REGULAR_FILE,
	YFS_MEDIA_TYPE_MAX
};

struct yfs_data {
	enum yfs_fs_type fs_type;
	enum yfs_media_type media_type;
	uint32_t block_size;
	uint32_t block_count;
	int format_when_mount_err;

	char media_name[YFS_MEDIA_NAME_MAX_LENGTH];
	void *media_handle;

	struct yfs_ops *ops;
	void *lower_fs_obj;
};

int yfs_register(struct yfs_data *yfs,
				enum yfs_fs_type fs_type,
				enum yfs_media_type media_type,
				unsigned long block_size,
				unsigned long block_count,
				int format_when_mount_err,
				const char *media_name,
				struct yfs_ops *ops);

int yfs_unregister(struct yfs_data *yfs);

int yfs_init(struct yfs_data *yfs);
int yfs_deinit(struct yfs_data *yfs);
int yfs_get_status(struct yfs_data *yfs, struct yfs_fs_stat *stat);

int yfs_fopen(struct yfs_data *yfs, struct yfs_file *yfile, char *filename, enum yfs_open_flags flags);
int yfs_fclose(struct yfs_data *yfs, struct yfs_file *yfile);
uint32_t yfs_fwrite(struct yfs_data *yfs, struct yfs_file *yfile, void *src, uint32_t size);
uint32_t yfs_fread(struct yfs_data *yfs, struct yfs_file *yfile, void *dst, uint32_t size);
int yfs_fseek(struct yfs_data *yfs, struct yfs_file *yfile, int64_t offset, enum yfs_whence_flags whence);
int64_t yfs_ftell(struct yfs_data *yfs, struct yfs_file *yfile);
int yfs_fsync(struct yfs_data *yfs, struct yfs_file *yfile);
int yfs_feof(struct yfs_data *yfs, struct yfs_file *yfile, int *iseof);
int yfs_fremove(struct yfs_data *yfs, char *path);
int yfs_fmove(struct yfs_data *yfs, char *oldpath, char *newpath);
int yfs_fstat(struct yfs_data *yfs, char *path, struct yfs_file_info *finfo);
int yfs_ftrunc(struct yfs_data *yfs, struct yfs_file *yfile, uint32_t size);
int yfs_fsize(struct yfs_data *yfs, struct yfs_file *yfile, uint32_t *size);

int yfs_diropen(struct yfs_data *yfs, struct yfs_dir *ydir, char *path);
int yfs_dirclose(struct yfs_data *yfs, struct yfs_dir *ydir);
int yfs_dirmake(struct yfs_data *yfs, char *path);
int yfs_dirread(struct yfs_data *yfs, struct yfs_dir *ydir, struct yfs_file_info *finfo);


#ifdef __cplusplus
}
#endif
#endif
