/*
 * YOS
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#ifndef _YOS_TIMER_H_
#define _YOS_TIMER_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define YTIMER_MAX_COUNT				8

#define YTIMER_ID_INVALID				-1

int ytimer_init(void);
int ytimer_deinit(void);


int ytimer_create(uint32_t timeout_ms, int auto_restart,
					void (*on_timeout)(void *para), void *timeout_para);
int ytimer_pause(int timer_id);
int ytimer_restore(int timer_id);
int ytimer_reset(int timer_id, uint32_t timeout_ms);
uint32_t ytimer_get_remaining_ms(int timer_id);
int ytimer_destroy(int timer_id);

#ifdef __cplusplus
}
#endif
#endif
