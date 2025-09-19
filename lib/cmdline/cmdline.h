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
#define CMDLINE_NAME					"STM32"
#define CMDLINE_MARK					"> "
#define CMDLINE_EXIT_CMD_NAME			"exit"
#define CMDLINE_HELP_CMD_NAME			"help"

#define CMDLINE_OUTPUT_VERBOSE			1

void do_cmdline(void);


/*
 * Check if [c] is a hex char, i.e. [0-9a-fA-F]
 * Return 1 if it is a hex char, otherwise 0 is returned.
 *
 * If [c] is a hex char, and [d] is not NULL, the hex value is set to [*d].
 *
 * 「c」は16進数文字（つまり[0-9a-fA-F]）になるかどうかをチェックします
 * 16進数文字の場合、1を戻ります
 * その他の場合、0を戻ります
 *
 * 「c」は16進数文字の場合、それに「d」はNULLではないなら、16進数の値に変換して
 * 「d」のポイントしている領域に保存されます
 */
int is_hex_char(char c, unsigned char *d);


/*
 * Check if the value of [hex] is between 0x0-0xF.
 * Return 1 if it is between 0x0-0xF, otherwise 0 is returned.
 *
 * If [hex] is between 0x0-0xF, and [c] is not NULL, the Character value is set to [*c]
 *
 * 「hex」の値は0x0-0xFの範囲内になることをチェックします
 * 0x0-0xFの範囲内の場合、1を戻ります
 * その他の場合、0を戻ります
 *
 * 「hex」の値は0x0-0xFの範囲内の場合、それに「c」はNULLではないなら、16進数文字に
 * 変換して、「c」のポイントしている領域に保存されます
 */
int is_one_digit_hex(unsigned char hex, char *c);

#ifdef __cplusplus
}
#endif
#endif
