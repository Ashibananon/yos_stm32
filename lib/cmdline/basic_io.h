/*
 * Basic IO
 *
 * Basic IO Header File
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#ifndef _BASIC_IO_H_
#define _BASIC_IO_H_

#include <stdint.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BASIC_IO_TEXT_END_MARK	'\n'


struct basic_io_port_operations {
	/*
	 * Return 0 if init ok, or init is failed
	 *
	 * 0を戻る場合、初期化できました
	 * その他の値を戻る場合、初期化できませんでした
	 */
	int (*basic_io_port_init)(void);

	/*
	 * Return 0 if deinit ok, or init is failed
	 *
	 * 0を戻る場合、終了できました
	 * その他の値を戻る場合、終了できませんでした
	 */
	int (*basic_io_port_deinit)(void);

	/*
	 * Return non-zero(TRUE) means it can read, or zero(FALSE) means no data available
	 *
	 * 0以外の値（TRUE）を戻る場合、読み込めることになります
	 * 0（FALSE）を戻る場合、今は読み込めないことになります
	 */
	int (*basic_io_port_can_read)(void);

	/*
	 * Return 0 if one byte is read and saved to b, other return values means error
	 * If no data available, it will return with a non-zero value, i.e. it will not block
	 *
	 * 0を戻る場合、一バイトのデータは読み込んで「b」のポイントしている領域に保存されています
	 * その他の値を戻る場合、読み込みエラーになります
	 * この関数は読み込めるデータはなくても返却します、つまりブロックしません
	 */
	int (*basic_io_port_read_byte_no_block)(uint8_t *b);

	/*
	 * Return non-zero(TRUE) means it can write, or zero(FALSE) means cannot write
	 *
	 * 0以外の値（TRUE）を戻る場合、書き込めることになります
	 * 0（FALSE)を戻る場合、いま書き込めないことになります
	 */
	int (*basic_io_port_can_write)(void);

	/*
	 * Return 0 if one byte(b) is written, other return values means error
	 * If cannot write, it will return with a non-zero value, i.e. it will not block
	 *
	 * 0を戻る場合、一バイトのデータ「b」を書き込みましたことになります
	 * その他の値を戻る場合、書き込みエラーになります
	 * この関数は書き込めなくても返却します、つまりブロックしません
	 */
	int (*basic_io_port_write_byte_no_block)(uint8_t b);
};


/*
 * Return 0 if init ok
 *
 * 0を戻る場合、初期化できたことになります
 */
int basic_io_init(struct basic_io_port_operations *ops);


/*
 * Return 0 if deinit ok
 *
 * 0を戻る場合、終了できたことになります
 */
int basic_io_deinit(void);


/*
 * Return non-zero(TRUE) means it can read, or zero(FALSE) means no data available
 *
 * 0以外の値（TRUE）を戻る場合、読み込めることになります
 * その他の値を戻る場合、読み込めないことになります
 */
int basic_io_has_data_to_read(void);


/*
 * Return 0 if one byte is read and saved to byte_read, other return values means error
 * If no data available, it will return with a non-zero value, i.e. it will not block
 *
 * 0を戻る場合、一バイトのデータは読み込んで「byte_read」のポイントしている領域に保存されています
 * 0以外の値を戻る場合、読み込みエラーになります
 * 読み込めるデータはなくても、この関数は戻ります、つまりブロックしません
 */
int basic_io_read_byte(uint8_t *byte_read);


/* Read data and save to buf with length of buf_len
 * Return the bytes read and saved to buf.
 *
 * If error occured, minus value is returned.
 * This function will not block.
 *
 * 「buf_len」バイト分のデータを読み込んで、そして「buf」のポイントしている領域に保存します
 * 実際に読み込んで保存されたバイト数を戻ります
 *
 * エラーが発生した場合、負数を戻ります
 * この関数はブロックしません
 */
int32_t basic_io_read(char *buf, uint16_t buf_len);


/* Read data and save to buf until EOL or
 * the bytes count reached buf_len.
 *
 * Return the bytes read and save to buf.
 *
 * If EOL is read before the bytes count reaches buf_len,
 * the EOL will be replaced with '\0' and return.
 *
 * If error occured, minus value is returned.
 *
 * This function will block until EOL read or
 * the bytes count reached buf_len.
 *
 * 「buf_len」バイト分のデータを読み込んで、そして「buf」のポイントしている領域に保存します
 * もし途中でEOLを読み込んだら、すぐに戻ります
 *
 * 実際に読み込んで保存されたバイト数を戻ります
 *
 * EOLを読み込んだ場合、EOLは'\0'として保存されます
 *
 * エラーが発生した場合、負数を戻ります
 *
 * この関数はEOLを読み込むまで、または「buf_len」で指定するバイト数を読み込むまで戻りません
 * つまり、この関数はブロックすることをご注意ください
 */
int32_t basic_io_readline(char *buf, uint16_t buf_len);


/* Write data with byte count of data_len,
 * Return the real byte count that written.
 *
 * If the paramater block is non-zero(TRUE),
 * it will not return until all data_len bytes
 * data has been sent unless error occurs.
 *
 * If block is zero(FALSE), it will return
 * immediately if it is not able to write. And
 * a retry might be needed.
 *
 * If error occured, minus value is returned.
 *
 * 「data」のポイントしている領域から「data_len」バイト分のデータを書き込みます
 * 実際の書き込んだバイト数を戻ります
 *
 * パラメーター「block」は0以外の値の場合（TRUE）、全部「data_len」バイトのデータ
 * を書き込んだまで戻りません、エラーが発生した場合除外です
 *
 * パラメーター「block」は0の場合（FALSE）、もし途中で書き込めない状態になったらすぐに戻ります
 * この場合は続いて呼び出して残ったデータを書き込むようになるでしょう
 *
 * エラーが発生した場合、負数を戻ります
 */
int32_t basic_io_write(char *data, uint16_t data_len, int block);


#define BASIC_IO_PRINTF_BUFFER_SIZE			256
int32_t basic_io_printf(const char *msg, ...);

#ifdef __cplusplus
}
#endif
#endif
