/*
 * YOS
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#ifndef _YOS_CORE_H_
#define _YOS_CORE_H_

#include <stdint.h>
#include "yos.h"

#ifdef __cplusplus
extern "C" {
#endif

#if (YOS_TICK_HZ > 1000)
#error "YOS HZ cannot exceed 1000!"
#endif

#define _TASK_SWITCH_INTERVAL_MS		(1000 / YOS_TICK_HZ)

struct yos_task {
	int (*task_func)(void *task_data);
	void *data;
	void *bp;
	void *sp;
#if (YOS_RECORD_STACK_USAGE == 1)
	void *min_sp_by_now;
#endif
	uint16_t stack_size;
	enum yos_task_status status;
	uint16_t block_ticks;
	char name[YOS_TASK_NAME_MAX_LENGTH];
};


#ifdef __cplusplus
}
#endif

#endif
