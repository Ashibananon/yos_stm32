#ifndef _YOS_SSD_1306_H_
#define _YOS_SSD_1306_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*
 * Host MCU Type
 * 0: AVR(ATmega328P)
 * 1: STM32
 */
#define HOST_TYPE			1

#if (HOST_TYPE == 0)
#include <avr/io.h>
#include <avr/pgmspace.h>
#elif (HOST_TYPE == 1)
#define PROGMEM
#define pgm_read_byte(address_short)		(*((uint8_t *)address_short))
#else
#error "SSD1306 needs to specify a host type"
#endif

#ifdef __cplusplus
}
#endif
#endif
