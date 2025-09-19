/*
 * YOS
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#include <libopencm3/cm3/cortex.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ytimer.h"

#define DEFAULT_USER_TIMER				TIM4
#define DEFAULT_USER_TIMER_RCC			RCC_TIM4
#define DEFAULT_USER_TIMER_IRQ			NVIC_TIM4_IRQ
#define DEFAULT_USER_TIMER_RST			RST_TIM4
#define DEFAULT_USER_TIMER_IRS			tim4_isr

static struct user_timer {
	void (*on_timeout)(void *para);
	void *para;
	uint32_t remaining_ms;
	uint32_t init_ms;
	uint8_t auto_restart;
	uint8_t is_paused;
	uint8_t is_used;
} _user_timer_list[USER_TIMER_MAX_COUNT];

static void _user_timer_list_check_in_irq(void)
{
	int i = 0;
	struct user_timer *ut;
	while (i < USER_TIMER_MAX_COUNT) {
		ut = _user_timer_list + i;
		if (ut->is_used) {
			if (!(ut->is_paused)) {
				if (ut->remaining_ms == 0) {
					if (ut->on_timeout != NULL) {
						ut->on_timeout(ut->para);
					}

					if (ut->auto_restart) {
						ut->remaining_ms = ut->init_ms;
					} else {
						ut->is_used = 0;
					}
				} else {
					ut->remaining_ms--;
				}
			}
		}

		i++;
	}
}

static void _user_timer_interrupt_enable(int enable)
{
	if (enable) {
		timer_enable_irq(DEFAULT_USER_TIMER, TIM_DIER_CC1IE);
	} else {
		timer_disable_irq(DEFAULT_USER_TIMER, TIM_DIER_CC1IE);
	}
}

static void _user_timer_list_init(void)
{
	memset(&_user_timer_list, 0x00, sizeof(_user_timer_list));
}

int user_timer_init(void)
{
	cm_disable_interrupts();

	_user_timer_list_init();

	rcc_periph_clock_enable(DEFAULT_USER_TIMER_RCC);
	rcc_periph_reset_pulse(DEFAULT_USER_TIMER_RST);

	timer_set_mode(DEFAULT_USER_TIMER, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
	timer_set_prescaler(DEFAULT_USER_TIMER, 1);

	timer_disable_preload(DEFAULT_USER_TIMER);
	timer_continuous_mode(DEFAULT_USER_TIMER);
	timer_set_period(DEFAULT_USER_TIMER, 36000 - 1);
	//timer_set_oc_value(DEFAULT_USER_TIMER, TIM_OC1, 36000 - 1);
	timer_enable_counter(DEFAULT_USER_TIMER);

	/* Enable interrupt */
	_user_timer_interrupt_enable(1);
	nvic_enable_irq(DEFAULT_USER_TIMER_IRQ);
	cm_enable_interrupts();

	return 0;
}

int user_timer_deinit(void)
{
	cm_disable_interrupts();
	_user_timer_interrupt_enable(0);
	timer_disable_counter(DEFAULT_USER_TIMER);
	nvic_disable_irq(DEFAULT_USER_TIMER_IRQ);
	rcc_periph_clock_disable(DEFAULT_USER_TIMER_RCC);
	cm_enable_interrupts();

	return 0;
}

//ISR(TIMER2_OVF_vect)
void DEFAULT_USER_TIMER_IRS(void)
{
	if (timer_get_flag(DEFAULT_USER_TIMER, TIM_SR_CC1IF)) {
		timer_clear_flag(DEFAULT_USER_TIMER, TIM_SR_CC1IF);
		_user_timer_list_check_in_irq();
	}
}

int user_timer_create(uint32_t timeout_ms, int auto_restart,
	void (*on_timeout)(void *para), void *timeout_para)
{
	int timer_id = -1;
	int i = 0;

	struct user_timer *ut;
	_user_timer_interrupt_enable(0);
	while (i < USER_TIMER_MAX_COUNT) {
		ut = _user_timer_list + i;
		if (!(ut->is_used)) {
			ut->on_timeout = on_timeout;
			ut->para = timeout_para;
			ut->remaining_ms = timeout_ms;
			ut->init_ms = timeout_ms;
			ut->auto_restart = auto_restart;
			ut->is_paused = 0;
			ut->is_used = 1;

			timer_id = i;
			break;
		}

		i++;
	}
	_user_timer_interrupt_enable(1);

	return timer_id;
}

static int _user_timer_pause_set(int timer_id, uint8_t pause)
{
	if (timer_id < 0 || timer_id > USER_TIMER_MAX_COUNT) {
		return -1;
	}

	struct user_timer *ut;
	_user_timer_interrupt_enable(0);
	ut = _user_timer_list + timer_id;
	if (ut->is_used) {
		ut->is_paused = pause;
	}
	_user_timer_interrupt_enable(1);

	return 0;
}

int user_timer_pause(int timer_id)
{
	return _user_timer_pause_set(timer_id, 1);
}

int user_timer_restore(int timer_id)
{
	return _user_timer_pause_set(timer_id, 0);
}

int user_timer_reset(int timer_id, uint32_t timeout_ms)
{
	if (timer_id < 0 || timer_id > USER_TIMER_MAX_COUNT) {
		return -1;
	}

	struct user_timer *ut;
	_user_timer_interrupt_enable(0);
	ut = _user_timer_list + timer_id;
	if (ut->is_used) {
		ut->remaining_ms = timeout_ms;
		ut->init_ms = timeout_ms;
	}
	_user_timer_interrupt_enable(1);

	return 0;
}

uint32_t user_timer_get_remaining_ms(int timer_id)
{
	if (timer_id < 0 || timer_id > USER_TIMER_MAX_COUNT) {
		return 0;
	}

	uint32_t remaining = 0;
	struct user_timer *ut;
	_user_timer_interrupt_enable(0);
	ut = _user_timer_list + timer_id;
	if (ut->is_used) {
		remaining = ut->remaining_ms;
	}
	_user_timer_interrupt_enable(1);

	return remaining;
}

int user_timer_destroy(int timer_id)
{
	if (timer_id < 0 || timer_id > USER_TIMER_MAX_COUNT) {
		return -1;
	}

	struct user_timer *ut;
	_user_timer_interrupt_enable(0);
	ut = _user_timer_list + timer_id;
	if (ut->is_used) {
		//ut->on_timeout = NULL;
		//ut->para = NULL;
		//ut->remaining_ms = 0;
		//ut->auto_restart = 0;
		//ut->is_paused = 0;
		ut->is_used = 0;
	}
	_user_timer_interrupt_enable(1);

	return 0;
}
