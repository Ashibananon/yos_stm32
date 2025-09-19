/*
 * YOS
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#include <libopencm3/cm3/cortex.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/rcc.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "yos_core.h"
#include "ystack.h"
#include "ymutex.h"
#include "common_def.h"

static struct yos_task _all_tasks[YOS_MAX_TASK_COUNT];
static void *volatile stack_bp_for_next_task;
static volatile int task_id_for_next;

static volatile int _CURRENT_TASK_ID;
static struct yos_task *volatile _CURRENT_TASK = NULL;
static volatile uint32_t _tmp_sp;

/*
 * Task shell function
 *
 * タスクシェル関数です
 */
static void _yos_task_shell(int task_id)
{
	YOS_DBG("enter yos task shell(id=%d)\n", task_id);

	if (task_id < 0 || task_id >= YOS_MAX_TASK_COUNT) {
		goto resched;
	}
	struct yos_task *task = _all_tasks + task_id;
	int task_ret = -1;
	if (task->task_func != NULL) {
		task->status = YOS_TASK_STATUS_RUNNING;
		task_ret = task->task_func(task->data);
	}
	task->status = YOS_TASK_STATUS_EXITED;

resched:
	schedule();
}

static void _yos_start_error(void)
{
	while (1) {
	}
}

#define _DUMMY_PSR_VALUE				0x01000000
static void _yos_init_task_stack(int task_id, struct yos_task *task)
{
	uint32_t *sp = (uint32_t *)(task->bp);

	/* PSR */
	*(--sp) = _DUMMY_PSR_VALUE;
	/* PC */
	*(--sp) = (uint32_t)&_yos_task_shell;
	/* LR */
	*(--sp) = (uint32_t)&_yos_start_error;
	/* R12 */
	*(--sp) = 0x12;
	/* R3 */
	*(--sp) = 0x03;
	/* R2 */
	*(--sp) = 0x02;
	/* R1 */
	*(--sp) = 0x01;
	/* R0 */
	*(--sp) = task_id;
	/* R11 */
	*(--sp) = 0x11;
	/* R10 */
	*(--sp) = 0x10;
	/* R9 */
	*(--sp) = 0x09;
	/* R8 */
	*(--sp) = 0x08;
	/* R7 */
	*(--sp) = 0x07;
	/* R6 */
	*(--sp) = 0x06;
	/* R5 */
	*(--sp) = 0x05;
	/* R4 */
	*(--sp) = 0x04;

	task->sp = (void *)sp;
}

/*
 * Find the next runnable task one by one.
 *
 * 順番に次の実行できるタスクを探します
 */
static int _find_next_task_to_run(void)
{
	struct yos_task *_the_next_task;
	int the_next_task_id = (_CURRENT_TASK_ID + 1) % YOS_MAX_TASK_COUNT;
	while (the_next_task_id != _CURRENT_TASK_ID) {
		_the_next_task = _all_tasks + the_next_task_id;
		if (_the_next_task->status == YOS_TASK_STATUS_CREATED
			|| _the_next_task->status == YOS_TASK_STATUS_RUNNING) {
			/*
			 * Found!
			 *
			 * 見つけた！
			 */
			break;
		} else if (_the_next_task->status == YOS_TASK_STATUS_EXITED) {
			_the_next_task->status = YOS_TASK_STATUS_INVALID;
		}

		the_next_task_id = (the_next_task_id + 1) % YOS_MAX_TASK_COUNT;
	}

	return the_next_task_id;
}

static void _update_task_block_ticks_irq(void)
{
	struct yos_task *_the_task;
	int i = 0;
	while (i < YOS_MAX_TASK_COUNT) {
		_the_task = _all_tasks + i;
		if (_the_task->status == YOS_TASK_STATUS_WAITING) {
			if (_the_task->block_ticks > 0) {
				_the_task->block_ticks--;
			}
			if (_the_task->block_ticks == 0) {
				_the_task->status = YOS_TASK_STATUS_RUNNING;
			}
		}

		i++;
	}
}

static void _global_timer_interrupt_enable(int enable)
{
	if (enable) {
		systick_interrupt_enable();
		systick_counter_enable();
	} else {
		systick_counter_disable();
		systick_interrupt_disable();
	}
}

static void _global_timer_start(void)
{
	systick_clear();
	_global_timer_interrupt_enable(1);
}

static void _global_timer_stop(void)
{
	_global_timer_interrupt_enable(0);
}

#define YOS_AHB_FREQ_HZ			72000000
static int _global_timer_init(void)
{
	int ret = 0;
	if (systick_set_frequency(YOS_TICK_HZ, YOS_AHB_FREQ_HZ)) {
		_global_timer_start();
		ret = 0;
	} else {
		ret = -1;
	}

	return ret;
}

static void _global_timer_deinit(void)
{
	_global_timer_stop();
}

static void yos_clock_setup(void)
{
	//rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
}

static void _make_pendsv(void)
{
	SCB_ICSR |= SCB_ICSR_PENDSVSET;
}

static volatile int next_task_id;
static struct yos_task *volatile next_task;
static volatile int is_systick_trigger_by_int = 1;

void sys_tick_handler(void)
{
	if (is_systick_trigger_by_int) {
		_update_task_block_ticks_irq();
	} else {
		is_systick_trigger_by_int = 1;
	}

	/*
	 * schedule tasks
	 *
	 * タスクをスケジュールします
	 */
	next_task_id = _find_next_task_to_run();
	if (next_task_id == _CURRENT_TASK_ID) {
		return;
	} else {
		next_task = _all_tasks + next_task_id;
		if (next_task->status == YOS_TASK_STATUS_CREATED) {
			_yos_init_task_stack(next_task_id, next_task);
		} else if (next_task->status == YOS_TASK_STATUS_RUNNING) {
		} else if (next_task->status == YOS_TASK_STATUS_EXITED) {
			next_task->status = YOS_TASK_STATUS_INVALID;
			/*
			 * Invalid task
			 *
			 * 不可能なタスク
			 */
			return;
		} else {
			/*
			 * Invalid task
			 *
			 * 不可能なタスク
			 */
			return;
		}
	}

	_make_pendsv();
}

__attribute__((naked)) void pend_sv_handler(void)
{
	//__asm__ __volatile__ ("CPSID I\n");

	SAVE_PSP
	SAVE_REGS_TO_STACK
	/*
	 * Save current SP
	 *
	 * 「今」動いているタスクのSPを保存します
	 */
	__asm__ __volatile__ (
		"mov %0, r0"
		: "=r"(_tmp_sp)
		:
		:
	);
	_CURRENT_TASK->sp = (void *)_tmp_sp;

	SAVE_LR_TO_STACK

#if (YOS_RECORD_STACK_USAGE == 1)
	if ((uint32_t)(_CURRENT_TASK->min_sp_by_now) > (uint32_t)(_CURRENT_TASK->sp)) {
		_CURRENT_TASK->min_sp_by_now = _CURRENT_TASK->sp;
	}
#endif

	_CURRENT_TASK = next_task;
	_CURRENT_TASK_ID = next_task_id;
	_CURRENT_TASK->status = YOS_TASK_STATUS_RUNNING;

	RESTORE_LR_FROM_STACK

	_tmp_sp = (uint32_t)_CURRENT_TASK->sp;
	/*
	 * Restore next SP
	 *
	 * 次の動くタスクのSPを回復します
	 */
	__asm__ __volatile__ (
		"ldr r0, %0"
		:
		: "m"(_tmp_sp)
		:
	);

	RESTORE_REGS_FROM_STACK
	RESTORE_PSP

	__asm__ __volatile__ (
	//	"CPSIE I							\n\t"
		"bx lr								\n\t"
	);
}

int yos_create_task(int (*task_func)(void *task_data), void *data,
						uint16_t stack_size, char *name)
{
	int task_id;
	if (task_func == NULL || stack_size == 0 || task_id_for_next >= YOS_MAX_TASK_COUNT) {
		return -1;
	}

	task_id = task_id_for_next;
	struct yos_task *this_task = _all_tasks + task_id;
	this_task->task_func = task_func;
	this_task->data = data;
	this_task->bp = stack_bp_for_next_task;
	this_task->sp = this_task->bp;
#if (YOS_RECORD_STACK_USAGE == 1)
	this_task->min_sp_by_now = this_task->sp;
#endif
	this_task->stack_size = stack_size;
	this_task->status = YOS_TASK_STATUS_CREATED;
	this_task->block_ticks = 0;
	if (name != NULL) {
		strncpy(this_task->name, name, sizeof(this_task->name));
	} else {
		snprintf(this_task->name, sizeof(this_task->name), "Tsk%03d", task_id);
	}
	this_task->name[sizeof(this_task->name) - 1] = '\0';

	YOS_DBG("create task[%s], id=%d, bp=0x%04X, ss=%d\n",
			this_task->name, task_id, this_task->bp, this_task->stack_size);

	task_id_for_next++;
	stack_bp_for_next_task -= stack_size;

	return task_id;
}

int yos_delete_task(int task_id)
{
	/*
	 * Deleting a task is not supported right now, since a memory management
	 * function is not implemented.
	 *
	 * タスクの削除は利用できません。
	 * メモリー管理機能は実現されていないためです。
	 */
	return -1;
}

#if (YOS_DEBUG_MSG_OUTPUT == 1)
#define YOS_IDLE_TASK_STACK_SIZE		512
#else
#define YOS_IDLE_TASK_STACK_SIZE		128
#endif

#define YOS_IDLE_TASK_NAME				"yosidle"
/*
 * YOS Idle task
 * Nothing has to be done right now
 *
 * The idle task makes sure that there is always a runnable task.
 *
 * アイドルタスクです。
 * 何もしないままになります
 *
 * アイドルタスクということは、いつでも動けるタスクが存在することを保証します。
 */
static int _yos_idle_task(void *para)
{
	uint32_t i = 0;
	YOS_DBG("_yos_idle_task is running\n");
	while (1) {
		i++;
	}

	return 0;
}

void yos_init(void)
{
	stack_bp_for_next_task = (void *)YOS_PROCESS_STACK_ADDRESS;
	task_id_for_next = 0;

	int i = 0;
	while (i < YOS_MAX_TASK_COUNT) {
		_all_tasks[i].status = YOS_TASK_STATUS_INVALID;

		i++;
	}

	_CURRENT_TASK_ID = yos_create_task(_yos_idle_task,
										NULL,
										YOS_IDLE_TASK_STACK_SIZE,
										YOS_IDLE_TASK_NAME);

	YOS_DBG("yos_create_task returned %d\n", _CURRENT_TASK_ID);
}

void yos_start(void)
{
	int first_task_id = _find_next_task_to_run();
	YOS_DBG("enter yos_start, tid=%d\n", first_task_id);
	if (first_task_id < 0 || first_task_id >= YOS_MAX_TASK_COUNT) {
		return;
	}

	yos_clock_setup();
	if (_global_timer_init() != 0) {
		YOS_DBG("global timer init failed\n");
		return;
	}

	cm_disable_interrupts();

	_CURRENT_TASK_ID = first_task_id;
	_CURRENT_TASK = _all_tasks + first_task_id;

	next_task = _CURRENT_TASK;
	next_task_id = _CURRENT_TASK_ID;

	_yos_init_task_stack(_CURRENT_TASK_ID, _CURRENT_TASK);

	__asm__ __volatile__ (
		"ldr r0, %0								\n\t"
		:
		: "m"(_CURRENT_TASK->sp)
		:
	);
	RESTORE_REGS_FROM_STACK
	RESTORE_PSP
	__asm__ __volatile__ (
		"mov r0, #0x02							\n\t"
		"msr control, r0						\n\t"
		"isb									\n\t"
		"CPSIE I								\n\t"
	);

	schedule();

	/*
	 * This function will not return to its caller(e.g. main) but
	 * to the _yos_task_shell with first_task_id as parameter.
	 * All the stacks by the end of this point will be gone.
	 *
	 * この関数は呼び出し元に戻りません。
	 * 代わりに、「_yos_task_shell」に戻るようになります。
	 * それに、ここまでのスタックは全部なくなります。
	 */
}

static void _schedule_irq(void)
{
	is_systick_trigger_by_int = 0;
	sys_tick_handler();
}

void yos_task_delay(uint16_t ticks)
{
	cm_disable_interrupts();
	_CURRENT_TASK->status = YOS_TASK_STATUS_WAITING;
	_CURRENT_TASK->block_ticks = ticks;
	_schedule_irq();
	cm_enable_interrupts();
}

void yos_task_msleep(uint16_t ms)
{
	yos_task_delay(ms < _TASK_SWITCH_INTERVAL_MS ? 1 : ms / _TASK_SWITCH_INTERVAL_MS);
}

#define DEBUG_SCHEDULE_WITH_DELAY	0
void schedule(void)
{
#if (DEBUG_SCHEDULE_WITH_DELAY == 1)
	yos_task_delay(0);
#else
	cm_disable_interrupts();
	_schedule_irq();
	cm_enable_interrupts();
#endif
}


int yos_get_task_info(int task_id, struct yos_task_info *task_info)
{
	cm_disable_interrupts();

	int ret = -1;
	if (task_id < 0 || task_id >= YOS_MAX_TASK_COUNT) {
		goto para_err;
	}

	struct yos_task *this_task = _all_tasks + task_id;
	if (this_task->status == YOS_TASK_STATUS_CREATED
		|| this_task->status == YOS_TASK_STATUS_RUNNING
		|| this_task->status == YOS_TASK_STATUS_WAITING
		|| this_task->status == YOS_TASK_STATUS_EXITED) {
		if (task_info != NULL) {
			task_info->id = task_id;
			task_info->status = this_task->status;
			task_info->stack_size = this_task->stack_size;
#if (YOS_RECORD_STACK_USAGE == 1)
			task_info->stack_max_reached_size =
					(uint32_t)(this_task->bp) - (uint32_t)(this_task->min_sp_by_now);
#else
			task_info->stack_max_reached_size = 0;
#endif
			strcpy(task_info->name, this_task->name);
		}

		ret = 0;
	} else {
		ret = -1;
	}

para_err:
	cm_enable_interrupts();

	return ret;
}


#define _YMUTEX_OWNER_NONE		-1

void ymutex_init(struct ymutex *mutex)
{
	if (mutex == NULL) {
		return;
	}

	mutex->owner = _YMUTEX_OWNER_NONE;
}

void ymutex_lock(struct ymutex *mutex)
{
	if (mutex == NULL) {
		return;
	}

	while (1) {
		if (ymutex_try_lock(mutex) == 0) {
			break;
		}
		schedule();
	}
}

int ymutex_try_lock(struct ymutex *mutex)
{
	int ret = -1;
	if (mutex == NULL) {
		return ret;
	}

	cm_disable_interrupts();
	if (mutex->owner < 0) {
		mutex->owner = _CURRENT_TASK_ID;
		ret = 0;
	}
	cm_enable_interrupts();

	return ret;
}

int ymutex_unlock(struct ymutex *mutex)
{
	int ret = -1;
	if (mutex == NULL) {
		return ret;
	}

	cm_disable_interrupts();
	if (mutex->owner == _CURRENT_TASK_ID) {
		mutex->owner = _YMUTEX_OWNER_NONE;
		ret = 0;
	}
	cm_enable_interrupts();

	return ret;
}
