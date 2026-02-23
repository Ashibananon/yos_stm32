/*
 * Cmdline
 *
 * Cmdline Source File
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include "cmdline.h"
#include "basic_io.h"
#include "../../src/yos/yos.h"
#include "../../src/yos/common_def.h"
#include "../../src/main_config.h"

#if (CMDLINE_SUPPORT_YFS == 1)
#include "../yfs/yfs.h"
#include "../yfs/yfs_data.h"
#endif

static char *_ARGV[CMDLINE_MAX_PARA_CNT];
static volatile int _last_cmd_ret = 0;

static void _cmd_printf(const char *msg, ...)
{
	char buf[CMDLINE_MAX_LENGTH];
	va_list ag;
	va_start(ag, msg);
	vsnprintf(buf, sizeof(buf), msg, ag);
	va_end(ag);
	buf[sizeof(buf) - 1] = '\0';

	basic_io_write(buf, strlen(buf), 1);
}

static int _cmd_read_line(char *buf, uint16_t len)
{
	if (buf == NULL) {
		return -1;
	}

	uint16_t _byte_read = 0;
	char byte;
	int result = -1;
	while (_byte_read < len) {
		result = basic_io_read_byte(buf + _byte_read);
		if (result == 0) {
			byte = *(buf + _byte_read);
			_byte_read++;
			if (byte == BASIC_IO_TEXT_END_MARK) {
				*(buf + _byte_read - 1) = '\0';
				break;
			}
		} else {
			/* Error occurs */
			yos_task_delay(10);
			continue;
		}
	}

	if (_byte_read == len) {
		*(buf + _byte_read - 1) = '\0';
	}

	return _byte_read;
}

static int _search_char_in_string(const char c, const char *str)
{
	if (str == NULL) {
		return -1;
	}

	int i = 0;
	int found = 0;
	while (str[i] != '\0') {
		if (c == str[i]) {
			found = 1;
			break;
		}

		i++;
	}

	if (found) {
		return i;
	} else {
		return -1;
	}
}

static int _is_blank_char(char c)
{
	return _search_char_in_string(c, CMDLINE_BLANK_CHARS) >= 0;
}

static int _is_quote_char(char c)
{
	return _search_char_in_string(c, CMDLINE_QUOTE_CHARS) >= 0;
}

static void _make_args(char *cmdline, int *argc, char **argv)
{
	if (cmdline == NULL || argc == NULL || argv == NULL) {
		return;
	}

	*argc = 0;
	int cmdlen = strlen(cmdline);
	int pos = 0;
	int cur_token_start;
	int cur_token_end;
	char cur_quote_c;
	while (pos < cmdlen && cmdline[pos] != '\0' && *argc < CMDLINE_MAX_PARA_CNT) {
		if (_is_blank_char(cmdline[pos])) {
			pos++;
		} else if (_is_quote_char(cmdline[pos])) {
			cur_quote_c = cmdline[pos];
			pos++;
			cur_token_start = pos;
			cur_token_end = _search_char_in_string(cur_quote_c, cmdline + pos);
			if (cur_token_end >= 0) {
				pos += cur_token_end;
			} else {
				while (cmdline[pos] != '\0') {
					pos++;
				}
			}
			cur_token_end = pos;
			cmdline[cur_token_end] = '\0';

			argv[*argc] = cmdline + cur_token_start;
			(*argc)++;

			pos++;
		} else {
			cur_token_start = pos;
			pos++;
			while (pos < cmdlen && !_is_blank_char(cmdline[pos])) {
				pos++;
			}
			cur_token_end = pos;
			cmdline[cur_token_end] = '\0';

			argv[*argc] = cmdline + cur_token_start;
			(*argc)++;

			pos++;
		}
	}
}

typedef int (*_CMDLINE_CMD_FUNC)(int argc, char **argv);
struct _cmd_info {
	_CMDLINE_CMD_FUNC func;
	char *cmd;
	char *desc;
};

static int _cmd_exit(int argc, char **argv)
{
#if (CMDLINE_OUTPUT_VERBOSE != 0)
	_cmd_printf("Bye\n");
#endif
	return 0;
}

static int _cmd_echo(int argc, char **argv)
{
	int i;

#if (CMDLINE_OUTPUT_VERBOSE != 0)
	_cmd_printf("Echo got %d paramaters:\n", argc);
#endif

	for (i = 0; i < argc; i++) {
		_cmd_printf("[%2d]\t[%s]\n", i, argv[i]);
	}

#if (CMDLINE_OUTPUT_VERBOSE != 0)
	_cmd_printf("Last cmd ret: [%d]\n", _last_cmd_ret);
#endif

	return 0;
}

#if (HAS_SPI_SDCARD_MODULE == 1)
/*
 * Usage:
 *		sd info		Show sd information
 *		sd init		Initialize sd card
 *		sd deinit	Deinitialize sd card
 */
static void _cmd_sdcard_usage(char *cmdname)
{
	_cmd_printf("Usage:\n");
	_cmd_printf("%s info      Show sd info\n", cmdname);
	_cmd_printf("%s init      Init sd\n", cmdname);
	_cmd_printf("%s deinit    Deinit sd\n", cmdname);
}
static int _cmd_sdcard(int argc, char **argv)
{
	int ret = -1;
	struct ysdcard_info sdi;
	if (argc == 2) {
		if (strcmp("info", argv[1]) == 0) {
			if (ysdcard_get_sdcard_info(&sdi) == 0) {
				_cmd_printf("got sd info:\n");
				_cmd_printf("  Type: %d\n", sdi.type);
				_cmd_printf("  Sector Size: %u\n", sdi.sector_size);
				_cmd_printf("  Sector Number: %u\n", sdi.sector_num);
				_cmd_printf("  Erase block size: %u\n", sdi.erase_block_size);
				ret = 0;
			} else {
				_cmd_printf("get sd info error\n");
			}
		} else if (strcmp("init", argv[1]) == 0) {
			if (ysdcard_init() == 0) {
				_cmd_printf("init sd OK\n");
				ret = 0;
			} else {
				_cmd_printf("init sd error\n");
			}
		} else if (strcmp("deinit", argv[1]) == 0) {
			if (ysdcard_deinit() == 0) {
				_cmd_printf("deinit sd OK\n");
				ret = 0;
			} else {
				_cmd_printf("deinit sd error\n");
			}
		} else {
			goto usage;
		}
	} else {
		goto usage;
	}

	return ret;

usage:
	_cmd_sdcard_usage(argv[0]);
	return ret;
}
#endif

static int _cmd_sys_info(int argc, char **argv)
{
	uint32_t chip_id_address = 0x1FFF7A10;
	char chip_id[12];
	int i;
	for (i = 0; i < sizeof(chip_id) / sizeof(chip_id[0]); i++) {
		*(chip_id + i) = *((char *)chip_id_address + i);
	}

	uint32_t flash_size_address = 0x1FFF7A22;
	uint16_t flash_size = *((uint16_t *)flash_size_address);

	_cmd_printf("Chip info:\n");
	_cmd_printf("  chip id:\n");
	basic_io_dump_hex(chip_id, sizeof(chip_id), 16, " ", 1, " => ");
	_cmd_printf("  Flash size reads: %d KB\n", flash_size);
	_cmd_printf("  MCU: %s Max Freq: %d Hz\n", MCU_NAME, MCU_MAX_FREQ);
	_cmd_printf("  Flash: %d Bytes\n", FLASH_SIZE);
	_cmd_printf("  RAM: %d Bytes\n", SRAM_SIZE);

	_cmd_printf("Data info:\n");
	_cmd_printf("  sz char=%d\n", sizeof(char));
	_cmd_printf("  sz short=%d\n", sizeof(short));
	_cmd_printf("  sz int=%d\n", sizeof(int));
	_cmd_printf("  sz long=%d\n", sizeof(long));
	_cmd_printf("  sz long long=%d\n", sizeof(long long));
	_cmd_printf("  sz float=%d\n", sizeof(float));
	_cmd_printf("  sz double=%d\n", sizeof(double));
	_cmd_printf("  sz void *=%d\n", sizeof(void *));

	return 0;
}


static int _cmd_sleep(int argc, char **argv)
{
	uint16_t ms_value;
	if (argc == 2) {
		ms_value = atoi(argv[1]);
		_cmd_printf("sleep %u ms\n", ms_value);
		yos_task_msleep(ms_value);
		_cmd_printf("%u ms slept\n", ms_value);
	}

	return 0;
}

#if (CMDLINE_SUPPORT_STEPMOTOR == 1)
/* usage: sm dir pulse_sets_number
 *
 * dir		0			Clockwise
 * 			1			Counter-Clockwise
 */
static int _cmd_step_motor(int argc, char **argv)
{
	uint8_t dir;
	uint16_t pulse_set_number;
	int ret = -1;
	if (argc == 3) {
		dir = atoi(argv[1]);
		pulse_set_number = atoi(argv[2]);
		ret = ysm_rotate(dir == 0 ? YSM_DIRECTION_CLOCKWISE : YSM_DIRECTION_COUNTER_CLOCKWISE,
						pulse_set_number);
	} else {
		/* TODO: Show cmd hints */
	}

	return ret;
}
#endif

#if (CMDLINE_SUPPORT_YFS == 1)
/*
 * Usage: yfs { start | end | status }
 */
static int yfs_mount(int argc, char **argv)
{
	int ret = -1;
	if (argc != 2) {
		goto usage;
	}

	struct yfs_fs_stat yfs_stat;
	if (strcmp(argv[1], "start") == 0) {
		if (yfs_start() == 0) {
			_cmd_printf("YFS started OK\n");
			ret = 0;
		} else {
			_cmd_printf("YFS started failed\n");
		}
	} else if (strcmp(argv[1], "end") == 0) {
		if (yfs_end() == 0) {
			_cmd_printf("YFS ended OK\n");
			ret = 0;
		} else {
			_cmd_printf("YFS ended failed\n");
		}
	} else if (strcmp(argv[1], "status") == 0) {
		if (yfs_status(&yfs_stat) == 0) {
			_cmd_printf("YFS status:\n");
			_cmd_printf("  type: %d\n", yfs_stat.type);
			_cmd_printf("  subtype: %d\n", yfs_stat.sub_type);
			_cmd_printf("  block size: %d\n", yfs_stat.block_size);
			_cmd_printf("  block count: %d\n", yfs_stat.block_total);
			_cmd_printf("  block allocated: %d\n", yfs_stat.block_allocated);
			_cmd_printf("  max filename length: %d\n", yfs_stat.file_name_max_bytes);
			_cmd_printf("  max file size: %d\n", yfs_stat.file_size_max_bytes);
			_cmd_printf("  mount point: %s\n", yfs_stat.mount_point);
			ret = 0;
		} else {
			_cmd_printf("YFS get status failed\n");
		}
	} else {
		goto usage;
	}

	return ret;
usage:
	_cmd_printf("Usage: %s { start | end | status }\n", argv[0]);
	return ret;
}

static int yfs_cmd_ls(int argc, char **argv)
{
	int ret;
	int fret;
	struct yfs_dir dirent;
	struct yfs_file_info fi;
	struct yfs_file fp;
	char *fname;
	char path[YFS_FILE_NAME_MAX_LENGTH];

	if (argc == 2) {
		snprintf(path, sizeof(path), "%s", argv[1]);
	} else {
		snprintf(path, sizeof(path), "/");
	}

	fret = yfs_diropen(YFS_Data, &dirent, path);
	if (fret != 0) {
		/* No such directory, try if it is a normal file */
		fret = yfs_fopen(YFS_Data, &fp, path, YFS_O_RDONLY);
		if (fret == 0) {
			yfs_fstat(YFS_Data, path, &fi);
			_cmd_printf("T %12s %s\n", "Size", "Name");
			_cmd_printf("- %12ld %s\n", fi.size, path);
			yfs_fclose(YFS_Data, &fp);
			ret = 0;
		} else {
			_cmd_printf("%s: open (%s) error(%d)\n", argv[0], path, fret);
			ret = -1;
		}
	} else {
		_cmd_printf("Files on %s:\n", path);
		_cmd_printf("T %12s %s\n", "Size", "Name");
		while (1) {
			fret = yfs_dirread(YFS_Data, &dirent, &fi);
			if (fret < 0) {
				break;
			} else {
				fname = fi.filename;
				if (fi.type == YFS_FILE_TYPE_REGULAR) {
					_cmd_printf("- %12ld %s\n", fi.size, fname);
				} else if (fi.type == YFS_FILE_TYPE_FOLDER) {
					_cmd_printf("d %12s %s\n", "", fname);
				} else {
					_cmd_printf("? %12s %s\n", "", fname);
				}
			}
		}

		yfs_dirclose(YFS_Data, &dirent);
		ret = 0;
	}

	return ret;
}

static int yfs_cmd_cat(int argc, char **argv)
{
	int ret;
	if (argc != 3) {
		_cmd_printf("Usage: %s mode filepath\n", argv[0]);
		_cmd_printf(" mode: b binary\n");
		_cmd_printf("       t text\n");
		ret = -1;
		goto fcat_cmd_error;
	}

	int mode = -1; /*0 binary, 1 text*/
	if (strcmp("b", argv[1]) == 0) {
		mode = 0;
	} else if (strcmp("t", argv[1]) == 0) {
		mode = 1;
	} else {
		_cmd_printf("Bad mode\n");
		ret = -2;
		goto fcat_mode_error;
	}

	int fret;
	struct yfs_file file;
	struct yfs_file_info fi;
	long bytes_to_read, bytes_read;
	unsigned char buf[256];
	long i;
	fret = yfs_fopen(YFS_Data, &file, argv[2], YFS_O_RDONLY);
	if (fret != 0) {
		_cmd_printf("Open file [%s] failed(%d)\n", argv[2], fret);
		ret = -3;
		goto fcat_open_error;
	}
	fret = yfs_fstat(YFS_Data, argv[2], &fi);
	bytes_to_read = (long)fi.size;

	int iseof = 0;
	/*_cmd_printf("%d bytes data to read\n", bytes_to_read);*/
	do {
		if (yfs_feof(YFS_Data, &file, &iseof) == 0) {
			if (iseof) {
				break;
			}
		} else {
			_cmd_printf("Read file info[%s] failed\n", argv[2]);
			ret = -4;
			goto eof_err;
		}
		bytes_read = yfs_fread(YFS_Data, &file, buf, sizeof(buf));
		if (bytes_read > 0) {
				bytes_to_read -= bytes_read;

				for (i = 0; i < bytes_read; i++) {
					if (mode == 0) {			/* Binary */
						_cmd_printf("%02X", buf[i]);
					} else if (mode == 1) {	 /* Text */
						_cmd_printf("%c", buf[i]);
					}
				}
		} else {
			_cmd_printf("File [%s] read error(%d)\n", argv[2], bytes_read);
			ret = -5;
			goto fcat_read_error;
		}
	} while (!iseof);

	ret = 0;

eof_err:
	_cmd_printf("\n");
fcat_read_error:
	yfs_fclose(YFS_Data, &file);
fcat_open_error:
fcat_mode_error:
fcat_cmd_error:
	return ret;
}

static int HexStr2Bytes(char *str, int len, void *data)
{
	if (str == NULL || len < 0 || data == NULL) {
		return -1;
	}

	int i, fh, fl;
	unsigned char ch, cl;
	int idx;
	i = 0;
	while (i < len) {
		idx = i / 2;
		fh = is_hex_char(*(str + i), &ch);
		i++;
		fl = is_hex_char(*(str + i), &cl);
		i++;
		if (!fh || !fl) {
			return -1;
		}
		*((char *)data + idx) = (ch << 4 | cl);
	}

	return 0;
}

static int yfs_cmd_append(int argc, char **argv)
{
	int ret = -1;
	char mode;
	char *filename, *data;
	long data_len;
	if (argc != 4) {
		_cmd_printf("Usage: %s mode file data\n", argv[0]);
		_cmd_printf("   mode   ab: append binary\n");
		_cmd_printf("          at: append text\n");
		_cmd_printf("          tb: truncate and write binary\n");
		_cmd_printf("          tt: truncate and write text\n");
		_cmd_printf("   file   file to which to append/write\n");
		_cmd_printf("   data   Text data for text mode\n");
		_cmd_printf("          Hex data for binary mode\n");
		return -1;
	}

	if (strcmp("ab", argv[1]) == 0) {
		mode = 'b';
	} else if (strcmp("at", argv[1]) == 0) {
		mode = 't';
	} else if (strcmp("tb", argv[1]) == 0) {
		mode = 'B';
	} else if (strcmp("tt", argv[1]) == 0) {
		mode = 'T';
	} else {
		_cmd_printf("Usage: %s Bad mode\n", argv[0]);
		return -2;
	}

	filename = argv[2];
	data = argv[3];
	data_len = strlen(data);

	struct yfs_file file;
	int fm = 0;
	long bytes_written, bytes2write = 0;
	int fret = yfs_fopen(YFS_Data, &file, filename, YFS_O_RDONLY);
	if (fret == 0) {
		/* File exists */
		yfs_fclose(YFS_Data, &file);
		fm = YFS_O_WRONLY;
	} else {
		/* File does not exists */
		fm = YFS_O_WRONLY | YFS_O_CREAT;
	}

	fret = yfs_fopen(YFS_Data, &file, filename, fm);
	if (fret != 0) {
		_cmd_printf("%s: Open file [%s] error(%d)\n", argv[0], filename, fret);
		return -3;
	}

	if (mode == 'B' || mode == 'T') {
		fret = yfs_ftrunc(YFS_Data, &file, 0);
		if (fret != 0) {
			_cmd_printf("%s: Truncate file [%s] error(%d)\n", argv[0], filename, fret);
			ret = -4;
			goto seek_err;
		}
	} else if (mode == 'b' || mode == 't') {
		fret = yfs_fseek(YFS_Data, &file, 0, YFS_SEEK_END);
		if (fret < 0) {
			_cmd_printf("%s: Seek file [%s] error(%d)\n", argv[0], filename, fret);
			ret = -4;
			goto seek_err;
		}
	}

	long i;
	char *bs = NULL;
	char *bin = NULL;
	if (mode == 't' || mode == 'T') {
		bs = data;
		bytes2write = data_len;
	} else if (mode == 'b' || mode == 'B') {
		if (data_len % 2 != 0) {
			_cmd_printf("%s: Binary data length error.\n", argv[0]);
			ret = -5;
			goto bin_data_err;
		}
		for (i = 0; i < data_len; i++) {
			if (!is_hex_char(*(data + i), NULL)) {
				_cmd_printf("%s: Binary data format error at (%ld).\n", argv[0], i);
				ret = -5;
				goto bin_data_err;
			}
		}

		bin = (char *)malloc((data_len / 2) * sizeof(char));
		if (bin == NULL) {
			_cmd_printf("%s: Allocate memory (%ld bytes) failed.\n", argv[0], data_len / 2);
			ret = -6;
			goto malloc_err;
		}

		if (HexStr2Bytes(data, data_len, bin) != 0) {
			_cmd_printf("%s: Convert bin data failed.\n", argv[0]);
			ret = -7;
			goto bin_cvt_err;
		}
		bs = bin;
		bytes2write = data_len / 2;
	}

	long bytes;
	bytes_written = 0;
	while (bytes_written < bytes2write) {
		bytes = yfs_fwrite(YFS_Data, &file, bs + bytes_written, bytes2write - bytes_written);
		if (bytes > 0) {
			bytes_written += bytes;
		} else {
			_cmd_printf("%s: write data failed(%d)\n", argv[0], fret);
			ret = -8;
			goto write_err;
		}
	}

	_cmd_printf("%s: write %ld bytes to file[%s]\n", argv[0], bytes_written, filename);

write_err:
bin_cvt_err:
	if (bin != NULL) {
		free(bin);
	}
malloc_err:
bin_data_err:
seek_err:
	yfs_fclose(YFS_Data, &file);

	return ret;
}

static int yfs_cmd_mkdir(int argc, char **argv)
{
	if (argc != 2) {
		_cmd_printf("Usage: %s dir_name\n", argv[0]);
		return -1;
	}

	int fret = yfs_dirmake(YFS_Data, argv[1]);
	if (fret == 0) {
		_cmd_printf("%s: [%s] done\n", argv[0], argv[1]);
		return 0;
	} else {
		_cmd_printf("%s: [%s] error(%d)\n", argv[0], argv[1], fret);
		return -2;
	}
}

static int yfs_cmd_mv(int argc, char **argv)
{
	if (argc != 3) {
		_cmd_printf("Usage: %s old new\n", argv[0]);
		return -1;
	}

	int fret = yfs_fmove(YFS_Data, argv[1], argv[2]);
	if (fret == 0) {
		_cmd_printf("%s: [%s] -> [%s] done\n", argv[0], argv[1], argv[2]);
		return 0;
	} else {
		_cmd_printf("%s: [%s] -> [%s] error(%d)\n", argv[0], argv[1], argv[2], fret);
		return -2;
	}
}

static int yfs_cmd_rm(int argc, char **argv)
{
	if (argc != 2) {
		_cmd_printf("Usage: %s name/path\n", argv[0]);
		return -1;
	}

	int fret = yfs_fremove(YFS_Data, argv[1]);
	if (fret == 0) {
		_cmd_printf("%s: [%s] done\n", argv[0], argv[1]);
		return 0;
	} else {
		_cmd_printf("%s: [%s] error(%d)\n", argv[0], argv[1], fret);
		return -2;
	}
}

static int yfs_cmd_touch(int argc, char **argv)
{
	if (argc != 2) {
		_cmd_printf("Usage: %s filename\n", argv[0]);
		return -1;
	}

	struct yfs_file file;
	int fret = yfs_fopen(YFS_Data, &file, argv[1], YFS_O_RDONLY);
	if (fret == 0) {
		/* File already exists */
		yfs_fclose(YFS_Data, &file);
		_cmd_printf("%s: file [%s] already exists\n", argv[0], argv[1]);
		return -2;
	} else {
		fret = yfs_fopen(YFS_Data, &file, argv[1], YFS_O_CREAT | YFS_O_WRONLY);
		if (fret == 0) {
			yfs_fclose(YFS_Data, &file);
			_cmd_printf("%s: file [%s] done\n", argv[0], argv[1]);
			return 0;
		} else {
			_cmd_printf("%s: file [%s] failed(%d)\n", argv[0], argv[1], fret);
			return -3;
		}
	}
}

static int yfs_cmd_copy(int argc, char **argv)
{
	int ret;
	int fret_src, fret_dst;
	struct yfs_file f_src;
	struct yfs_file f_dst;
	unsigned char buf[4096];
	uint32_t bytes_to_copy;
	uint32_t bytes_copied;
	uint32_t bytes_read;
	uint32_t bytes_written;
	uint32_t bytes_written_once;

	if (argc != 3) {
		_cmd_printf("Usage: %s src dest\n", argv[0]);
		ret = -1;
		goto fcp_arg_err;
	}

	fret_src = yfs_fopen(YFS_Data, &f_src, argv[1], YFS_O_RDONLY);
	if (fret_src != 0) {
		_cmd_printf("%s: open src [%s] error(%d)\n", argv[0], argv[1], fret_src);
		ret = -4;
		goto fcp_open_err_src;
	}

	fret_dst = yfs_fopen(YFS_Data, &f_dst, argv[2], YFS_O_WRONLY | YFS_O_CREAT);
	if (fret_dst != 0) {
		_cmd_printf("%s: open dst [%s] error(%d)\n", argv[0], argv[2], fret_dst);
		ret = -5;
		goto fcp_open_err_dst;
	}

	fret_dst = yfs_ftrunc(YFS_Data, &f_dst, 0);
	if (fret_dst != 0) {
		_cmd_printf("%s: trunc dst [%s] error(%d)\n", argv[0], argv[2], fret_dst);
		ret = -6;
		goto fcp_trunc_dst_err;
	}

	int fret = yfs_fsize(YFS_Data, &f_src, &bytes_to_copy);
	if (fret != 0) {
		_cmd_printf("%s: get src file size failed(%d)\n", argv[0], fret);
		goto src_size_err;
	}

	bytes_copied = 0;
	_cmd_printf("%s: %d bytes to copy\n", argv[0], bytes_to_copy);
	while (bytes_to_copy > 0) {
		bytes_read = yfs_fread(YFS_Data, &f_src, buf, sizeof(buf));
		if (bytes_read > 0) {
			bytes_written = 0;
			while (bytes_written < bytes_read) {
				bytes_written_once = yfs_fwrite(YFS_Data, &f_dst,
									buf + bytes_written,
									bytes_read - bytes_written);
				if (bytes_written_once > 0) {
						bytes_written += bytes_written_once;
				} else {
					_cmd_printf("%s: write [%s] error(%d)\n", argv[0], argv[2], fret_dst);
					ret = -7;
					goto fcp_write_dst_err;
				}
			}

			bytes_to_copy -= bytes_read;
			bytes_copied += bytes_read;
		} else {
			_cmd_printf("%s: read [%s] error(%d)\n", argv[0], argv[1], fret_src);
			ret = -8;
			goto fcp_read_src_err;
		}
	}

	_cmd_printf("%s: %d bytes copied\n", argv[0], bytes_copied);
	ret = 0;

fcp_write_dst_err:
fcp_read_src_err:
src_size_err:
fcp_trunc_dst_err:
	yfs_fclose(YFS_Data, &f_dst);
fcp_open_err_dst:
	yfs_fclose(YFS_Data, &f_src);
fcp_open_err_src:
fcp_arg_err:
	return ret;
}


static int yfs_cmd_dd(int argc, char **argv)
{
	int ret;
	if (argc != 6) {
		goto fdd_usage;
	}

	long iskip = strtol(argv[1], NULL, 16);
	long oseek = strtol(argv[2], NULL, 16);
	char *ifile = argv[3];
	char *ofile = argv[4];
	long size = strtol(argv[5], NULL, 16);
	if (iskip < 0 || oseek < 0 || size < 0) {
		_cmd_printf("%s: Parameter error. Please check whether iskip/oseek/size is non-negative value\n");
		return -2;
	}

	struct yfs_file *ifp = NULL, *ofp = NULL;
	int freti, freto;
	int if_is_regular, of_is_regular;
	unsigned char bin_vstart = 0x00, bin_vend;
	if (strcmp("?b", ifile) == 0) {
		bin_vstart = 0x00;
		bin_vend = 0xFF;
		if_is_regular = 0;
	} else if (strcmp("?t", ifile) == 0) {
		/* Text characters are from 0x20(space) to 0x7E(~) */
		bin_vstart = 0x20; // ' ' (space)
		bin_vend = 0x7E;  // '~'
		if_is_regular = 0;
	} else if (strcmp("?0", ifile) == 0) {
		bin_vstart = 0x00;
		bin_vend = 0x00;
		if_is_regular = 0;
	} else {
		if_is_regular = 1;
		ifp = (struct yfs_file *)malloc(sizeof(struct yfs_file));
		if (ifp == NULL) {
			_cmd_printf("%s: Allocate memory for input file failed\n", argv[0]);
			return -3;
		}

		freti = yfs_fopen(YFS_Data, ifp, ifile, YFS_O_RDONLY);
		if (freti != 0) {
			_cmd_printf("%s: Open input file[%s] failed(%d)\n", argv[0], ifile, freti);
			ret = -4;
			goto ifp_open_err;
		}
		freti = yfs_fseek(YFS_Data, ifp, iskip, YFS_SEEK_SET);
		if (freti != 0) {
			_cmd_printf("%s: Seek input file[%s] for %ld bytes failed(%d)\n", argv[0], ifile, iskip, freti);
			ret = -5;
			goto ifp_seek_err;
		}
	}

	if (strcmp("?b", ofile) == 0) {
		of_is_regular = 0;
	} else if (strcmp("?t", ofile) == 0) {
		of_is_regular = 0;
	} else {
		of_is_regular = 1;
		ofp = (struct yfs_file *)malloc(sizeof(struct yfs_file));
		if (ofp == NULL) {
			_cmd_printf("%s: Allocate memory for output file failed\n", argv[0]);
			ret = -6;
			goto ofp_malloc_err;
		}
		freto = yfs_fopen(YFS_Data, ofp, ofile, YFS_O_WRONLY);
		if (freto != 0) {
			freto = yfs_fopen(YFS_Data, ofp, ofile, YFS_O_WRONLY | YFS_O_CREAT);
			if (freto != 0) {
				_cmd_printf("%s: Open output file[%s] failed(%d)\n", argv[0], ofile, freto);
				ret = -7;
				goto ofp_open_err;
			}
		}
		freto = yfs_fseek(YFS_Data, ofp, oseek, YFS_SEEK_SET);
		if (freto != 0) {
			_cmd_printf("%s: Seek output file[%s] for %ld bytes failed(%d)\n", argv[0], ofile, oseek, freto);
			ret = -8;
			goto ofp_seek_err;
		}
	}

	/* copy data */
	long bytes_copied = 0;
	unsigned char buf[256], cc;
	long read_size = size > sizeof(buf) ? sizeof(buf) : size;
	int i;
	long bytes_written, bytes_read, total_bytes_written;
	cc = bin_vstart;
	int iseof = 0;
	while (bytes_copied < size) {
		if (if_is_regular) {
			if (yfs_feof(YFS_Data, ifp, &iseof) == 0) {
				if (iseof) {
					_cmd_printf("\n%s: EOF of input file[%s] occured, stop\n", argv[0], ifile);
					ret = -9;
					goto ifp_eof_err;
				}
			} else {
				_cmd_printf("\n%s: Get EOF info of input file[%s] failed, stop\n", argv[0], ifile);
				ret = -10;
				goto ifp_eof_err;
			}
			bytes_read = yfs_fread(YFS_Data, ifp, buf, read_size);
			if (bytes_read > 0) {
				if (of_is_regular) {
					total_bytes_written = 0;
					while (total_bytes_written < bytes_read) {
						bytes_written = yfs_fwrite(YFS_Data, ofp, buf + total_bytes_written,
										bytes_read - total_bytes_written);
						if (bytes_written > 0) {
							total_bytes_written += bytes_written;
						} else {
							_cmd_printf("%s: Write output file[%s] failed(%d)\n", argv[0], ofile, freto);
							ret = -11;
							goto ofp_write_err_1;
						}
					}
				} else {
					for (i = 0; i < bytes_read; i++) {
						if (strcmp("?b", ofile) == 0) {
							_cmd_printf("%02X", buf[i]);
						} else if (strcmp("?t", ofile) == 0) {
							_cmd_printf("%c", buf[i]);
						}
					}
				}

				bytes_copied += bytes_read;
			} else {
				_cmd_printf("%s: Read input file[%s] failed(%d)\n", argv[0], ifile, freti);
				ret = -12;
				goto ifp_read_err;
			}
		} else {
			if (of_is_regular) {
				bytes_written = yfs_fwrite(YFS_Data, ofp, &cc, 1);
				if (bytes_written > 0) {
					bytes_copied += bytes_written;
					cc++;
					if (cc > bin_vend) {
						cc = bin_vstart;
					}
				} else {
					_cmd_printf("%s: Write output file[%s] failed(%d)\n", argv[0], ofile, freto);
					ret = -13;
					goto ofp_write_err_2;
				}
			} else {
				if (strcmp("?b", ofile) == 0) {
					_cmd_printf("%02X", cc);
				} else if (strcmp("?t", ofile) == 0) {
					_cmd_printf("%c", cc);
				}
				bytes_copied++;
				cc++;
				if (cc > bin_vend) {
					cc = bin_vstart;
				}
			}
		}
	}

	_cmd_printf("\n%s: %ld bytes copied to file[%s]\n", argv[0], bytes_copied, ofile);
	ret = 0;

ofp_write_err_2:
ifp_read_err:
ofp_write_err_1:
ifp_eof_err:
ofp_seek_err:
	if (of_is_regular) {
		yfs_fclose(YFS_Data, ofp);
	}
ofp_open_err:
	if (of_is_regular && ofp != NULL) {
		free(ofp);
	}
ofp_malloc_err:
ifp_seek_err:
	if (if_is_regular) {
		yfs_fclose(YFS_Data, ifp);
	}
ifp_open_err:
	if (if_is_regular && ifp != NULL) {
		free(ifp);
	}
	return ret;

fdd_usage:
	_cmd_printf("Usage: %s iskip oseek if of size\n", argv[0]);
	_cmd_printf("   iskip   How many bytes(in hex) to skip from input file\n");
	_cmd_printf("   oseek   How many bytes(in hex) to seek for output file\n");
	_cmd_printf("   if      Input file. Some special data files can be specified like below:\n");
	_cmd_printf("       ?b  generate binary data from 0x00 to 0xFF sequentially\n");
	_cmd_printf("       ?t  generate text data from 0x20(space) to 0x7E(~) sequentially\n");
	_cmd_printf("       ?0  generate zero data (0x00) for all bytes\n");
	_cmd_printf("   of      Output file. Some special data files can be specified like below:\n");
	_cmd_printf("       ?b  output to *stdout* in binary mode\n");
	_cmd_printf("       ?t  output to *stdout* in text mode\n");
	_cmd_printf("   size    How many bytes(in hex) of data to copy\n");
	return -1;
}
#endif

#if (HAS_AUDIO_MODULE == 1)
/*
 * Usage: yaudio {play | stop | status} audio_file
 */
static void yaudio_cmd_usage(char *cmd)
{
	_cmd_printf("Usage: %s {play | record | pause | stop | status}\n", cmd);
	_cmd_printf("       %s set <audio_file>\n", cmd);
	_cmd_printf("       %s volume <volume_l> <volume_r>\n", cmd);
}

static int yaudio_cmd(int argc, char **argv)
{
	if (argc == 2) {
		if (strcmp(argv[1], "play") == 0) {
			if (yaudio_play() == 0) {
				_cmd_printf("Send play cmd ok\n");
			} else {
				_cmd_printf("Send play cmd failed\n");
			}
		} else if (strcmp(argv[1], "record") == 0) {
			if (yaudio_record() == 0) {
				_cmd_printf("Send record cmd ok\n");
			} else {
				_cmd_printf("Send record cmd failed\n");
			}
		} else if (strcmp(argv[1], "pause") == 0) {
			if (yaudio_pause() == 0) {
				_cmd_printf("Send pause cmd ok\n");
			} else {
				_cmd_printf("Send pause cmd failed\n");
			}
		} else if (strcmp(argv[1], "stop") == 0) {
			if (yaudio_stop() == 0) {
				_cmd_printf("Send stop cmd ok\n");
			} else {
				_cmd_printf("Send stop cmd failed\n");
			}
		} else if (strcmp(argv[1], "status") == 0) {
			struct yaudio_player _yp;
			if (yaudio_get_status(&_yp) == 0) {
				_cmd_printf("YAudio Player status:\n");
				_cmd_printf("  Running: %s\n", _yp.is_running ? "Yes" : "No");
				_cmd_printf("  Status: %d\n", _yp.status);
				_cmd_printf("  Volume: (%d, %d)\n", _yp.volume_l, _yp.volume_r);
				_cmd_printf("  Total duration: %d ms\n", _yp.audio_file_duration_ms);
				_cmd_printf("  Played duration: %d ms\n", _yp.audio_file_played_ms);
				_cmd_printf("  Audio File: [%s]\n", _yp.audio_file);
				_cmd_printf("  Sample Rate: [%d]\n", _yp.sampling_rate);
				_cmd_printf("  Sample Cnt: [%u][%u]\n",
								(uint32_t)(_yp.sample_num >> 32),
								(uint32_t)(_yp.sample_num & 0xFFFFFFFF));
				_cmd_printf("  Sample Played: [%u][%u]\n",
								(uint32_t)(_yp.sample_played >> 32),
								(uint32_t)(_yp.sample_played & 0xFFFFFFFF));
				_cmd_printf("  Channels: [%d]\n", _yp.channel);
				_cmd_printf("  Bit depth: [%d]\n", _yp.audio_bit_depth);
			} else {
				_cmd_printf("Get audio player status failed\n");
			}
		} else {
			goto usage;
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "set") == 0) {
			if (yaudio_set_audio_file(argv[2]) == 0) {
				_cmd_printf("Send cmd to set audio file [%s] ok\n",
							argv[2]);
			} else {
				_cmd_printf("Send cmd to set audio file [%s] failed\n",
							argv[2]);
			}
		} else {
			goto usage;
		}
	} else if (argc == 4) {
		if (strcmp(argv[1], "volume") == 0) {
			uint8_t vl = atoi(argv[2]);
			uint8_t vr = atoi(argv[3]);
			if (yaudio_change_volume(vl, vr) == 0) {
				_cmd_printf("Send cmd to change volume to (%d, %d) ok\n",
							vl, vr);
			} else {
				_cmd_printf("Send cmd to change volume to (%d, %d) failed\n",
							vl, vr);
			}
		} else {
			goto usage;
		}
	} else {
		goto usage;
	}

	return 0;

usage:
	yaudio_cmd_usage(argv[0]);

	return -1;
}
#endif

static int _cmd_tasks_info(int argc, char **argv)
{
	_cmd_printf("%3s   %2s      %6s      %6s    %s\n",
				"ID", "ST", "SS", "MSS", "NAME");
	_cmd_printf("-----------------------------------------------\n");
	struct yos_task_info ti;
	int i = 0;
	while (i < YOS_MAX_TASK_COUNT) {
		if (yos_get_task_info(i, &ti) == 0) {
			_cmd_printf("%03d   %2d      %6d      %6d    %s\n",
					ti.id, ti.status, ti.stack_size, ti.stack_max_reached_size, ti.name);
		}

		i++;
	}
	return 0;
}

static int _cmd_help(int argc, char **argv);

#if (CMDLINE_OUTPUT_VERBOSE == 0)
#define CMD_INFO_ITEM(func, name, tip)		{func, name, NULL}
#else
#define CMD_INFO_ITEM(func, name, tip)		{func, name, tip}
#endif

static struct _cmd_info _cmd_list[] = {
	CMD_INFO_ITEM(_cmd_help, CMDLINE_HELP_CMD_NAME, "Show cmd info briefly"),
	CMD_INFO_ITEM(_cmd_echo, "echo", "Echo cmdline info"),

#if (HAS_SPI_SDCARD_MODULE == 1)
	CMD_INFO_ITEM(_cmd_sdcard, "sd", "SD card cmds"),
#endif

	CMD_INFO_ITEM(_cmd_sys_info, "si", "Show system info"),
	CMD_INFO_ITEM(_cmd_sleep, "sleep", "Sleep given ms"),

#if (CMDLINE_SUPPORT_STEPMOTOR == 1)
	CMD_INFO_ITEM(_cmd_step_motor, "sm", "Step Motor test"),
#endif

	CMD_INFO_ITEM(_cmd_tasks_info, "ts", "Show tasks info"),

#if (CMDLINE_SUPPORT_YFS == 1)
	CMD_INFO_ITEM(yfs_mount, "yfs", "mount yfs"),
	CMD_INFO_ITEM(yfs_cmd_ls, "yls", "list files of file system"),
	CMD_INFO_ITEM(yfs_cmd_cat, "ycat", "display file contents"),
	CMD_INFO_ITEM(yfs_cmd_append, "yapp", "append data to the end of file"),
	CMD_INFO_ITEM(yfs_cmd_mkdir, "ymkdir", "make directory"),
	CMD_INFO_ITEM(yfs_cmd_mv, "ymv", "move file/directory"),
	CMD_INFO_ITEM(yfs_cmd_rm, "yrm", "remove file/directory"),
	CMD_INFO_ITEM(yfs_cmd_copy, "ycp", "copy file"),
	CMD_INFO_ITEM(yfs_cmd_dd, "ydd", "data copy"),
	CMD_INFO_ITEM(yfs_cmd_touch, "ytouch", "create an empty file"),
#endif

#if (HAS_AUDIO_MODULE == 1)
	CMD_INFO_ITEM(yaudio_cmd, "yaudio", "play audio file"),
#endif

	CMD_INFO_ITEM(_cmd_exit, CMDLINE_EXIT_CMD_NAME, "Exit cmdline")
};

static struct _cmd_info *_find_cmd_item_by_name(char *name)
{
	struct _cmd_info *item = NULL;

	if (name == NULL) {
		return item;
	}

	int i;
	int cnt = sizeof(_cmd_list) / sizeof(_cmd_list[0]);
	for (i = 0; i < cnt; i++) {
		if (strcmp(_cmd_list[i].cmd, name) == 0) {
			item = _cmd_list + i;
			break;
		}
	}

	return item;
}

static int _cmd_help(int argc, char **argv)
{
	int ret = 0;
	int i;
	int cnt = sizeof(_cmd_list) / sizeof(_cmd_list[0]);
	if (argc == 1) {
		for (i = 0; i < cnt; i++) {
			_cmd_printf("%16s\t%s\n",
						_cmd_list[i].cmd,
						_cmd_list[i].desc == NULL ? "" : _cmd_list[i].desc);
		}
	} else if (argc == 2) {
		struct _cmd_info *item = _find_cmd_item_by_name(argv[1]);
		if (item != NULL) {
			_cmd_printf("%16s\t%s\n", item->cmd, item->desc == NULL ? "" : item->desc);
		} else {
#if (CMDLINE_OUTPUT_VERBOSE == 0)
			_cmd_printf("unknown cmd\n");
#else
			_cmd_printf("%s Error: command %s not found\n", argv[0], argv[1]);
#endif
			ret = -1;
		}
	} else {
#if (CMDLINE_OUTPUT_VERBOSE != 0)
		_cmd_printf("Usage:\n");
		_cmd_printf("     %s\n", argv[0]);
		_cmd_printf("             show all commands\n");
		_cmd_printf("     %s [cmd]\n", argv[0]);
		_cmd_printf("             show [cmd] info\n");
#endif
		ret = -2;
	}

	return ret;
}

static void _clear_args(int argc, char **argv)
{
}


static int _do_cmds(int argc, char **argv)
{
	if (argc <= 0) {
		return 1;
	}

	struct _cmd_info *item = _find_cmd_item_by_name(argv[0]);
	if (item != NULL) {
		if (item->func != NULL) {
			_last_cmd_ret = item->func(argc, argv);

			if (item->func == _cmd_exit) {
				return 0;
			}
		}
	} else {
#if (CMDLINE_OUTPUT_VERBOSE == 0)
		_cmd_printf("bad cmd\n");
#else
		_cmd_printf("Command %s not found\nEnter [%s] to see all supported commands\n",
					argv[0], CMDLINE_HELP_CMD_NAME);
#endif
	}
	return 1;
}

static char _cmdline[CMDLINE_MAX_LENGTH];
void do_cmdline()
{
#if (CMDLINE_OUTPUT_VERBOSE != 0)
	_cmd_printf("\n%s cmdline started.\n", CMDLINE_NAME);
	_cmd_printf("Input [%s] to show all available commands,\n", CMDLINE_HELP_CMD_NAME);
	_cmd_printf("or [%s] to exit %s cmdline.\n", CMDLINE_EXIT_CMD_NAME, CMDLINE_NAME);
#endif
	int argc;
	char **argv = _ARGV;
	int cmd_flag = 1;
	while (cmd_flag) {
		_cmd_printf("%s%s", CMDLINE_NAME, CMDLINE_MARK);
		if (_cmd_read_line(_cmdline, sizeof(_cmdline)) >= 0) {
			_make_args(_cmdline, &argc, argv);
			cmd_flag = _do_cmds(argc, argv);
			_clear_args(argc, argv);
		} else {
#if (CMDLINE_OUTPUT_VERBOSE == 0)
			_cmd_printf("\nERROR\n");
#else
			_cmd_printf("\n* ERROR occured on reading cmdline, exit.*\n");
#endif
			cmd_flag = 0;
		}
	}
}
