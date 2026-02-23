/*
 * YOS
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#include <stdarg.h>
#include <stdio.h>
#include <libopencm3/stm32/gpio.h>
#include "../../../lib/sd-spi-driver/inc/sd_spi_driver.h"
#include "../../yos/yos.h"
#include "../../yos/ytimer.h"
#include "../yspi.h"
#include "yspi_sdcard.h"

#define YSDCARD_SPI_CS_GPIO_PORT			GPIOA
#define YSDCARD_SPI_CS_GPIO_NUM				GPIO4
#define YSDCARD_SPI_CS_VALID_VALUE			YSPI_CS_VALID_ON_LOW

#if (DEFAULT_SPI_USE_DMA == 1)
#define YSDCARD_SPI_SPEED_HIGH				(YSPI_FREQUENCY_MIN * 32)
#else
#define YSDCARD_SPI_SPEED_HIGH				(YSPI_FREQUENCY_MIN * 16)
#endif
#define YSDCARD_SPI_SPEED_LOW				(YSPI_FREQUENCY_MIN)

#define YSDCARD_CARD_NAME					"sdcard0"

static struct yspi_ctrl *_sdcard_spi_ctrl = NULL;
static struct yspi_device _sdcard_spi_dev;

static void _on_spi_event(enum YSPI_DEVICE_EVENT evt)
{
	switch (evt) {
	case YSPI_DEVICE_EVENT_TX_COMPLETE:
		break;
	default:
		break;
	}
}

static int yspi_sd_control(struct sd_card* card, enum sd_user_ctrl ctrl)
{
	int ret = -1;
	if (card == NULL) {
		goto para_err;
	}
	switch (ctrl) {
	case Sd_User_Ctrl_Init_Hardware:
		if (yspi_device_init(&_sdcard_spi_dev, YSDCARD_SPI_CS_GPIO_PORT, YSDCARD_SPI_CS_GPIO_NUM,
							YSDCARD_SPI_CS_VALID_VALUE, _on_spi_event) == 0) {
			ret = 0;
		}
		break;
	case Sd_User_Ctrl_Deinit_Hardware:
		if (yspi_device_deinit(&_sdcard_spi_dev) == 0) {
			ret = 0;
		}
		break;
	case Sd_User_Ctrl_Select_Card:
		if (yspi_device_select(&_sdcard_spi_dev) == 0) {
			ret = 0;
		}
		break;
	case Sd_User_Ctrl_Deselect_Card:
		if (yspi_device_unselect(&_sdcard_spi_dev) == 0) {
			ret = 0;
		}
		break;
	case Sd_User_Ctrl_Take_Bus:
		if (yspi_trans_begin(_sdcard_spi_ctrl, &_sdcard_spi_dev) == 0) {
			ret = 0;
		}
		break;
	case Sd_User_Ctrl_Release_Bus:
		if (yspi_trans_end(_sdcard_spi_ctrl, &_sdcard_spi_dev) == 0) {
			ret = 0;
		}
		break;
	case Sd_User_Ctrl_Set_Low_Speed:
		if (yspi_master_set_speed(_sdcard_spi_ctrl, YSDCARD_SPI_SPEED_LOW) == 0) {
			ret = 0;
		}
		break;
	case Sd_User_Ctrl_Set_High_Speed:
		if (yspi_master_set_speed(_sdcard_spi_ctrl, YSDCARD_SPI_SPEED_HIGH) == 0) {
			ret = 0;
		}
		break;
	default:
		break;
	}

para_err:
	return ret;
}

static int yspi_sd_transfer(struct sd_card* card, struct sd_spi_buf* tx, struct sd_spi_buf* rx)
{
	int ret = -1;
	YSD_DBG("Enter yspi_sd_transfer(card=0x%08X, tx=0x%08X, rx=0x%08X)\n",
			card, tx, rx);
	if (card == NULL) {
		goto para_err;
	}

	if (tx != NULL) {
		tx->used = yspi_send_and_receive(_sdcard_spi_ctrl,tx->data, NULL, tx->size, 0xFF);
		if (tx->used == tx->size) {
			ret = 0;
		}
		YSD_DBG("yspi_sd_transfer: %d bytes sent\n", tx->used);
		YSD_DBG_DUMP_DATA(tx->data, tx->used);
	}

	if (rx != NULL) {
		rx->used = yspi_send_and_receive(_sdcard_spi_ctrl, NULL, rx->data, rx->size, 0xFF);
		if (rx->used == rx->size) {
			ret = 0;
		}
		YSD_DBG("yspi_sd_transfer: %d bytes received\n", rx->used);
		YSD_DBG_DUMP_DATA(rx->data, rx->used);
	}

	ret = 0;

para_err:
	YSD_DBG("Leave yspi_sd_transfer, ret=%d\n", ret);
	return ret;
}

static void yspi_sd_delay_us(struct sd_card* card, uint32_t us)
{
	if (card == NULL) {
		goto para_err;
	}

	uint32_t t;
	if (us % 1000 == 0) {
		t = us / 1000;
	} else {
		t = us / 1000 + 1;
	}

	YSD_DBG("yspi_sd_delay_us: us[%d] to sleep %d ms\n", us, t);
	yos_task_msleep(t);

para_err:
	return;
}

static void yspi_sd_print(struct sd_card* card, const char* format, ...)
{
#if (YOS_SD_LIBRARY_MSG_OUTPUT == 1)
	if (card == NULL || format == NULL) {
		return;
	}

	va_list ag;
	va_start(ag, format);
	basic_io_vprintf(format, ag);
	va_end(ag);
#endif
}

static struct sd_spi_interface yspi_sd_operators = {
	.control = yspi_sd_control,
	.transfer = yspi_sd_transfer,
	.delay_us = yspi_sd_delay_us
};

static struct sd_debug_interface yspi_sd_debug_operatiors = {
	.print = yspi_sd_print
};

struct sd_card card0 = SD_CARD_OBJ_INIT(YSDCARD_CARD_NAME, &yspi_sd_operators, &yspi_sd_debug_operatiors);

static struct sd_card *yspi_sd_card = NULL;
int ysdcard_init(void)
{
	int ret = -1;

	if (_sdcard_spi_ctrl != NULL) {
		/* Alread inited, just return */
		return 0;
	}
	_sdcard_spi_ctrl = YSPI_1_CTRL;

	enum sd_error sdret = sd_spi_lib_init();
	if (sdret != Sd_Err_OK) {
		YSD_DBG("sd_spi_lib_init returned %d\n", sdret);
		goto sd_lib_init_err;
	}

	yspi_sd_card = sd_card_find(YSDCARD_CARD_NAME);
	if (yspi_sd_card == NULL) {
		YSD_DBG("sd_card_find(%s) returned NULL\n", YSDCARD_CARD_NAME);
		goto sdcard_not_found;
	}

	sdret = sd_card_init(yspi_sd_card);
	if (sdret != Sd_Err_OK) {
		YSD_DBG("sd_card_init(yspi_sd_card=0x%08X) returned %d\n", yspi_sd_card, sdret);
		goto sdcard_init_err;
	}

	ret = 0;

	return ret;

sdcard_init_err:
	yspi_sd_card = NULL;
sdcard_not_found:
	/* call sd_spi_lib_deinit if it exists */
sd_lib_init_err:
	return ret;
}

int ysdcard_deinit(void)
{
	int ret = -1;
	enum sd_error sdret;
	if (yspi_sd_card == NULL) {
		goto sd_not_inited;
	}

	sdret = sd_card_deinit(yspi_sd_card);
	if (sdret != Sd_Err_OK) {
		YSD_DBG("sd_card_deinit returned %d\n", sdret);
	}

	yspi_sd_card = NULL;

	_sdcard_spi_ctrl = NULL;

	ret = 0;
sd_not_inited:
	return ret;
}

int ysdcard_is_ready(void)
{
	return (yspi_sd_card != NULL);
}

int ysdcard_get_sdcard_info(struct ysdcard_info *sdinfo)
{
	int ret = -1;
	if (yspi_sd_card == NULL || sdinfo == NULL) {
		goto para_err;
	}

	sdinfo->type = sd_card_get_type(yspi_sd_card);
	sdinfo->sector_size = sd_card_get_block_size(yspi_sd_card);
	if (sdinfo->sector_size != 0) {
		sdinfo->sector_num = sd_card_get_capacity(yspi_sd_card) / sdinfo->sector_size;
	} else {
		sdinfo->sector_num = 0;
	}
	sdinfo->erase_block_size = sd_card_get_erase_size(yspi_sd_card);

	ret = 0;

para_err:
	return ret;
}

int ysdcard_read(uint32_t sector, uint32_t offset, uint8_t *buf, uint32_t read_len)
{
	int ret = -1;
	YSD_DBG("Enter ysdcard_read(sector=%d, offset=%d, buf=0x%08X, len=%d)\n",
			sector, offset, buf, read_len);
	if (yspi_sd_card == NULL || buf == NULL) {
		goto para_err;
	}

	uint32_t block_size = sd_card_get_block_size(yspi_sd_card);
	enum sd_error sdret = sd_card_read(yspi_sd_card, sector * block_size, buf, read_len);
	if (sdret != Sd_Err_OK) {
		YSD_DBG("sd_card_read returned %d\n", sdret);
		goto ysd_read_err;
	}

	ret = 0;

ysd_read_err:
para_err:
	YSD_DBG("Leave ysdcard_read, retv=%d\n", ret);
	return ret;
}

int ysdcard_write(uint32_t sector, uint32_t offset, uint8_t *data, uint32_t write_len)
{
	int ret = -1;
	YSD_DBG("Enter ysdcard_write(sector=%d, offset=%d, data=0x%08X, len=%d)\n",
			sector, offset, data, write_len);
	if (yspi_sd_card == NULL || data == NULL) {
		goto para_err;
	}

	uint32_t block_size = sd_card_get_block_size(yspi_sd_card);
	enum sd_error sdret = sd_card_write(yspi_sd_card, sector * block_size, data, write_len);
	if (sdret != Sd_Err_OK) {
		YSD_DBG("sd_card_write returned %d\n", sdret);
		goto ysd_write_err;
	}

	ret = 0;

ysd_write_err:
para_err:
	YSD_DBG("Leave ysdcard_write retv=%d\n", ret);
	return ret;
}
