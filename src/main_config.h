/*
 * YOS
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#ifndef _MAIN_CONFIG_H_
#define _MAIN_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#define HAS_CMDLINE								1
#define HAS_AHT20_SENSOR						1
#define HAS_SSD1306_OLED						1
#define HAS_SPI_SDCARD_MODULE					1


#if (HAS_CMDLINE == 1)
#include "../lib/cmdline/basic_io.h"
#include "../lib/cmdline/cmdline.h"
#endif

#if (HAS_AHT20_SENSOR == 1)
#include "../lib/AVR_aht20/src/aht20.h"
#endif

#if (HAS_SSD1306_OLED == 1)
#include "../lib/ssd1306xled/ssd1306xled/ssd1306xled.h"
#include "../lib/ssd1306xled/ssd1306xled/yos_ssd1306_font.h"
#endif

#if (HAS_SPI_SDCARD_MODULE == 1)
#include "ydevice/spi_sdcard/yspi_sdcard.h"
#endif

#ifdef __cplusplus
}
#endif
#endif
