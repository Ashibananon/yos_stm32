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
	char buf[BASIC_IO_PRINTF_BUFFER_SIZE];
	va_list ag;
	va_start(ag, msg);
	vsnprintf(buf, sizeof(buf), msg, ag);
	va_end(ag);
	buf[sizeof(buf) - 1] = '\0';

	return basic_io_write(buf, strlen(buf), 1);
}
