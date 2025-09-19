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

#define USER_TIMER_MAX_COUNT			8

int user_timer_init(void);
int user_timer_deinit(void);


int user_timer_create(uint32_t timeout_ms, int auto_restart,
					void (*on_timeout)(void *para), void *timeout_para);
int user_timer_pause(int timer_id);
int user_timer_restore(int timer_id);
int user_timer_reset(int timer_id, uint32_t timeout_ms);
uint32_t user_timer_get_remaining_ms(int timer_id);
int user_timer_destroy(int timer_id);


#ifdef __cplusplus
}
#endif
#endif
