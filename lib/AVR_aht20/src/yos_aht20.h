#ifndef _YOS_AHT20_H_
#define _YOS_AHT20_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "../../../src/yos/yos.h"

/*
 * Host MCU Type
 * 0: AVR(ATmega328P)
 * 1: STM32
 */
#define HOST_TYPE			1

#if (HOST_TYPE == 0)
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#elif (HOST_TYPE == 1)
#define PROGMEM
#define pgm_read_byte(address_short)		(*((uint8_t *)address_short))
//void _delay_ms(double __ms);
#define _delay_ms(ms)						yos_task_msleep(ms)
#else
#error "SSD1306 needs to specify a host type"
#endif

#ifdef __cplusplus
}
#endif
#endif
