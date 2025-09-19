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

#if (CMDLINE_SUPPORT_LFS == 1)
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
	return basic_io_readline(buf, len);
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

int is_hex_char(char c, unsigned char *d)
{
	int ret = 1;
	unsigned char dd;
	switch (c) {
	case '0':
		dd = 0x0;
		break;
	case '1':
		dd = 0x1;
		break;
	case '2':
		dd = 0x2;
		break;
	case '3':
		dd = 0x3;
		break;
	case '4':
		dd = 0x4;
		break;
	case '5':
		dd = 0x5;
		break;
	case '6':
		dd = 0x6;
		break;
	case '7':
		dd = 0x7;
		break;
	case '8':
		dd = 0x8;
		break;
	case '9':
		dd = 0x9;
		break;
	case 'a':
	case 'A':
		dd = 0xA;
		break;
	case 'b':
	case 'B':
		dd = 0xB;
		break;
	case 'c':
	case 'C':
		dd = 0xC;
		break;
	case 'd':
	case 'D':
		dd = 0xD;
		break;
	case 'e':
	case 'E':
		dd = 0xE;
		break;
	case 'f':
	case 'F':
		dd = 0xF;
		break;
	default:
		ret = 0;
		break;
	}

	if (d != NULL) {
		*d = dd;
	}

	return ret;
}

int is_one_digit_hex(unsigned char hex, char *c)
{
	int ret = 1;
	char cc = '\0';
	switch (hex) {
	case 0x0:
		cc = '0';
		break;
	case 0x1:
		cc = '1';
		break;
	case 0x2:
		cc = '2';
		break;
	case 0x3:
		cc = '3';
		break;
	case 0x4:
		cc = '4';
		break;
	case 0x5:
		cc = '5';
		break;
	case 0x6:
		cc = '6';
		break;
	case 0x7:
		cc = '7';
		break;
	case 0x8:
		cc = '8';
		break;
	case 0x9:
		cc = '9';
		break;
	case 0xA:
		cc = 'A';
		break;
	case 0xB:
		cc = 'B';
		break;
	case 0xC:
		cc = 'C';
		break;
	case 0xD:
		cc = 'D';
		break;
	case 0xE:
		cc = 'E';
		break;
	case 0xF:
		cc = 'F';
		break;
	default:
		ret = 0;
		break;
	}

	if (c != NULL) {
		*c = cc;
	}

	return ret;
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


static int _cmd_sys_info(int argc, char **argv)
{
	_cmd_printf("MCU: %s Max Freq: %d Hz\n", MCU_NAME, MCU_MAX_FREQ);
	_cmd_printf("Flash: %d Bytes\n", FLASH_SIZE);
	_cmd_printf("RAM: %d Bytes\n", SRAM_SIZE);
	_cmd_printf("sz char=%d\n", sizeof(char));
	_cmd_printf("sz short=%d\n", sizeof(short));
	_cmd_printf("sz int=%d\n", sizeof(int));
	_cmd_printf("sz long=%d\n", sizeof(long));
	_cmd_printf("sz float=%d\n", sizeof(float));
	_cmd_printf("sz double=%d\n", sizeof(double));
	_cmd_printf("sz void *=%d\n", sizeof(void *));

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

static int _cmd_tasks_info(int argc, char **argv)
{
	_cmd_printf("ID    ST      SS     MSS    NAME\n");
	struct yos_task_info ti;
	int i = 0;
	while (i < YOS_MAX_TASK_COUNT) {
		if (yos_get_task_info(i, &ti) == 0) {
			_cmd_printf("%03d   %2d    %4d    %4d    %s\n",
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
	CMD_INFO_ITEM(_cmd_sys_info, "si", "Show system info"),
	CMD_INFO_ITEM(_cmd_sleep, "sleep", "Sleep given ms"),
#if (CMDLINE_SUPPORT_STEPMOTOR == 1)
	CMD_INFO_ITEM(_cmd_step_motor, "sm", "Step Motor test"),
#endif
	CMD_INFO_ITEM(_cmd_tasks_info, "ts", "Show tasks info"),
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
