/*
 * YOS
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <stdio.h>
#include "../lib/cmdline/basic_io.h"
#include "../lib/cmdline/cmdline.h"
#include "ydevice/yusart.h"
#include "yos/yos.h"
#include "yos/ytimer.h"

static void system_clock_setup(void)
{
	rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);

	/* Enable clocks for GPIO port A (for GPIO_USART1_TX) and USART1. */
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_USART1);
}

static int _cmdline_task(void *para)
{
	while (1) {
		do_cmdline();
	}

	return 0;
}

int main(void)
{
	system_clock_setup();
	user_timer_init();

	if (basic_io_init(yusart_io_operations) != 0) {
		return -1;
	}

	basic_io_printf("-------------\n");
	basic_io_printf("YOS starts on STM32\n");

	yos_init();

	if (yos_create_task(_cmdline_task, NULL, 1024, "cmdtask") < 0) {
		return -1;
	}

	yos_start();

	/*
	 * Not going here
	 *
	 * ここに着くことはありません
	 */
	return 0;
}
