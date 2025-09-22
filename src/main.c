/*
 * YOS
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#include <libopencm3/stm32/rcc.h>
#include <stdint.h>
#include <stdio.h>
#include "ydevice/yiic.h"
#include "ydevice/yusart.h"
#include "yos/yos.h"
#include "yos/ytimer.h"
#include "../lib/AVR_aht20/src/aht20.h"
#include "../lib/cmdline/basic_io.h"
#include "../lib/cmdline/cmdline.h"
#include "../lib/ssd1306xled/ssd1306xled/ssd1306xled.h"
#include "../lib/ssd1306xled/ssd1306xled/yos_ssd1306_font.h"


#define HAS_AHT20_SENSOR		1
#define HAS_SSD1306_OLED		0


static void system_clock_setup(void)
{
	rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_3V3_84MHZ]);
}

static int _cmdline_task(void *para)
{
	while (1) {
		do_cmdline();
	}

	return 0;
}

#if (HAS_AHT20_SENSOR == 1)
static int8_t _temperature = 0;
static uint8_t _humidity = 0;
static void _ath20_event_cb(int8_t temperature, uint8_t humidity, enum AHT20_STATUS status)
{
	if (status == AHT20_SUCCESS) {
		_temperature = temperature;
		_humidity = humidity;
	}
}

static int _aht20_task(void *para)
{
	basic_io_printf("aht20 task begins\n");
	aht20_init();
	basic_io_printf("aht20 init done\n");
	register_aht20_event_callback(_ath20_event_cb);
	while (1) {
		aht20_trigger_measurement();
		yos_task_msleep(100);
		aht20_event();

		yos_task_msleep(1000);
		basic_io_printf("aht20 task last read: Temp=%d, Humidity=%d%%\n", _temperature, _humidity);
	}

	return 0;
}
#endif

#if (HAS_SSD1306_OLED == 1)
static int _oled_task(void *para)
{
	uint8_t dd, hh, mm, ss;
	dd = hh = mm = ss = 0;
	char buf[16];
	int invert = 0;

	basic_io_printf("oled task starts\n");

	ssd1306_init();
	basic_io_printf("oled ssd1306_init done\n");
	ssd1306_clear();
	basic_io_printf("oled clear done\n");

	yos_ssd1306_puts(YOS_SSD1306_FONT_6X8, 0, 0, "YOS OLED");
	while (1) {
		if (ss == 60) {
			ss = 0;
			mm++;
			if (mm == 60) {
				mm = 0;
				hh++;
				if (hh == 24) {
					hh = 0;
					dd++;
				}
			}
		}

		snprintf(buf, sizeof(buf), "%03u D", dd);
		yos_ssd1306_puts(YOS_SSD1306_FONT_6X8,
						yos_ssd1306_char_col_to_pixel(YOS_SSD1306_FONT_6X8, 0),
						2, buf);

		snprintf(buf, sizeof(buf), "%02u H", hh);
		yos_ssd1306_puts(YOS_SSD1306_FONT_6X8,
						yos_ssd1306_char_col_to_pixel(YOS_SSD1306_FONT_6X8, 6),
						2, buf);

		snprintf(buf, sizeof(buf), "%02u M", mm);
		yos_ssd1306_puts(YOS_SSD1306_FONT_6X8,
						yos_ssd1306_char_col_to_pixel(YOS_SSD1306_FONT_6X8, 11),
						2, buf);

		snprintf(buf, sizeof(buf), "%02u S", ss);
		yos_ssd1306_puts(YOS_SSD1306_FONT_6X8,
						yos_ssd1306_char_col_to_pixel(YOS_SSD1306_FONT_6X8, 16),
						2, buf);

#if (HAS_AHT20_SENSOR == 1)
		snprintf(buf, sizeof(buf), "T: %3d", _temperature);
		yos_ssd1306_puts(YOS_SSD1306_FONT_6X8,
						yos_ssd1306_char_col_to_pixel(YOS_SSD1306_FONT_6X8, 0),
						4, buf);

		snprintf(buf, sizeof(buf), "H: %3u%%", _humidity);
		yos_ssd1306_puts(YOS_SSD1306_FONT_6X8,
						yos_ssd1306_char_col_to_pixel(YOS_SSD1306_FONT_6X8, 8),
						4, buf);
#endif

		if (ss == 0) {
			ssd1306_display_invert(invert);
			invert = !invert;
		}

		yos_task_msleep(1000);
		ss++;
	}

	return 0;
}
#endif

int main(void)
{
	system_clock_setup();
	user_timer_init();

	if (basic_io_init(yusart_io_operations) != 0) {
		return -1;
	}

	if (yiic_master_init(100000) != 0) {
		basic_io_printf("IIC master init failed\n");
		return -1;
	}

	basic_io_printf("-------------\n");
	basic_io_printf("YOS starts on STM32\n");

	yos_init();

	if (yos_create_task(_cmdline_task, NULL, 1024, "cmdtask") < 0) {
		basic_io_printf("Failed to create cmdline task\n");
		return -1;
	}

#if (HAS_SSD1306_OLED == 1)
	if (yos_create_task(_oled_task, NULL, 1024, "oledtak") < 0) {
		basic_io_printf("Failed to create oled task\n");
		return -1;
	}
#endif

#if (HAS_AHT20_SENSOR == 1)
	if (yos_create_task(_aht20_task, NULL, 1024, "ahttsk") < 0 ) {
		basic_io_printf("Failed to create aht20 task\n");
		return -1;
	}
#endif

	yos_start();

	/*
	 * Not going here
	 *
	 * ここに着くことはありません
	 */
	return 0;
}
