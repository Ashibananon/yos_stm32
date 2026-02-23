/*
 * Basic IO
 *
 * Basic IO Source File
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#include <stdio.h>
#include <string.h>
#include "basic_io.h"


static struct basic_io_port_operations *volatile _bipo = NULL;

int basic_io_init(struct basic_io_port_operations *ops)
{
	if (ops == NULL) {
		return -1;
	}
	_bipo = ops;

	if (_bipo->basic_io_port_init != NULL) {
		return _bipo->basic_io_port_init();
	} else {
		return 0;
	}
}

int basic_io_deinit(void)
{
	int ret = -1;
	if (_bipo == NULL) {
		return ret;
	}

	if (_bipo->basic_io_port_deinit != NULL) {
		ret = _bipo->basic_io_port_deinit();
	} else {
		ret = 0;
	}
	_bipo = NULL;

	return ret;
}

int basic_io_has_data_to_read(void)
{
	if (_bipo == NULL || _bipo->basic_io_port_can_read == NULL) {
		return 0;
	}

	return _bipo->basic_io_port_can_read();
}

int basic_io_read_byte(uint8_t *byte_read)
{
	if (_bipo == NULL || _bipo->basic_io_port_read_byte_no_block == NULL) {
		return -1;
	}

	return (_bipo->basic_io_port_read_byte_no_block(byte_read));
}

int32_t basic_io_read(char *buf, uint16_t buf_len)
{
	if (buf == NULL) {
		return -1;
	}

	uint16_t cnt = 0;
	while (cnt < buf_len) {
		if (basic_io_read_byte(buf + cnt) == 0) {
			cnt++;
		} else {
			break;
		}
	}

	return cnt;
}

/* To be delete */
#if 0
int32_t basic_io_readline(char *buf, uint16_t buf_len)
{
	if (buf == NULL) {
		return -1;
	}

	uint16_t _byte_read = 0;
	char byte;
	int result = -1;
	while (_byte_read < buf_len) {
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
			continue;
		}
	}

	if (_byte_read == buf_len) {
		*(buf + _byte_read - 1) = '\0';
	}

	return _byte_read;
}
#endif

int32_t basic_io_write(char *data, uint16_t data_len, int block)
{
	if (_bipo == NULL || _bipo->basic_io_port_write_byte_no_block == NULL || data == NULL) {
		return -1;
	}

	uint16_t byte_written = 0;
	while (byte_written < data_len) {
		if (_bipo->basic_io_port_write_byte_no_block(*(data + byte_written)) == 0) {
			byte_written++;
		} else {
			/* error */
			if (!block) {
				break;
			}
		}
	}

	return byte_written;
}

int32_t basic_io_printf(const char *msg, ...)
{
	va_list ag;
	va_start(ag, msg);
	uint32_t ret = basic_io_vprintf(msg, ag);
	va_end(ag);

	return ret;
}

int32_t basic_io_vprintf(const char *msg, va_list ag)
{
	char buf[BASIC_IO_PRINTF_BUFFER_SIZE];
	vsnprintf(buf, sizeof(buf), msg, ag);
	buf[sizeof(buf) - 1] = '\0';

	return basic_io_write(buf, strlen(buf), 1);
}

uint32_t basic_io_dump_hex(void *data, uint32_t data_len,
							uint8_t data_per_line, const char *spilter,
							int dump_to_char, const char *data_char_spliter)
{
	uint32_t dumped = 0;
	if (data == NULL || data_per_line == 0) {
		goto para_err;
	}

	int col = 0, line = 0;
	uint32_t total_line = data_len / data_per_line;
	uint8_t remaining = data_len % data_per_line;
	uint8_t current_data;
	uint8_t left;

	basic_io_printf("Dump %d bytes data from 0x%08X starts\n", data_len, data);
	for (line = 0; line < total_line; line++) {
		for (col = 0; col < data_per_line; col++) {
			current_data = *((uint8_t *)data + line * data_per_line + col);
			basic_io_printf("%02X", current_data);
			dumped++;
			if (spilter != NULL) {
				basic_io_printf("%s", spilter);
			}
		}
		if (dump_to_char) {
			if (data_char_spliter != NULL) {
				basic_io_printf("%s", data_char_spliter);
			}
			for (col = 0; col < data_per_line; col++) {
				current_data = *((uint8_t *)data + line * data_per_line + col);
				basic_io_printf("%c", can_display_char(current_data) ? current_data : '.');
				if (spilter != NULL) {
					basic_io_printf("%s", spilter);
				}
			}
		}
		basic_io_printf("\n");
	}

	for (left = 0; left < remaining; left++) {
		current_data = *((uint8_t *)data + line * data_per_line + col + left);
		basic_io_printf("%02X", current_data);
		dumped++;
		if (spilter != NULL) {
			basic_io_printf("%s", spilter);
		}
	}
	if (remaining > 0 && dump_to_char) {
		int spliter_len = spilter != NULL ? strlen(spilter) : 0;
		int space = (data_per_line - remaining) * (2 + spliter_len);
		int i;
		for (i = 0; i < space; i++) {
			basic_io_printf("%c", BASIC_IO_SPACE_CHAR);
		}
		if (data_char_spliter != NULL) {
			basic_io_printf("%s", data_char_spliter);
		}
		for (left = 0; left < remaining; left++) {
			current_data = *((uint8_t *)data + line * data_per_line + col + left);
			basic_io_printf("%c", can_display_char(current_data) ? current_data : '.');
			if (spilter != NULL) {
				basic_io_printf("%s", spilter);
			}
		}
	}
	basic_io_printf("\n");
	basic_io_printf("Dump %d bytes data from 0x%08X ends\n", dumped, data);

para_err:
	return dumped;
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

int can_display_char(char c)
{
	return (c >= 0x21 && c <= 0x7E);
}

int is_string_end_with(char *str, char *end_str)
{
	int ret = 0;
	if (str == NULL || end_str == NULL) {
		goto para_err;
	}

	if (strcmp(str + strlen(str) - strlen(end_str), end_str) == 0) {
		ret = 1;
	}

para_err:
	return ret;
}
