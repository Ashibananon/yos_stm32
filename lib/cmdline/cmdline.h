/*
 * Cmdline
 *
 * Cmdline Header File
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#ifndef _CMDLINE_H_
#define _CMDLINE_H_

#ifdef __cplusplus
extern "C" {
#endif

#define CMDLINE_MAX_LENGTH				256
#define CMDLINE_BLANK_CHARS				" \t\r\n"
#define CMDLINE_QUOTE_CHARS				"\"\'"
#define CMDLINE_MAX_PARA_CNT			10
#define CMDLINE_NAME					"yos_stm32"
#define CMDLINE_MARK					"> "
#define CMDLINE_EXIT_CMD_NAME			"exit"
#define CMDLINE_HELP_CMD_NAME			"help"

#define BASIC_IO_TEXT_END_MARK			'\n'

#define CMDLINE_OUTPUT_VERBOSE			1

#define CMDLINE_SUPPORT_YFS				1


void do_cmdline(void);


#ifdef __cplusplus
}
#endif
#endif
