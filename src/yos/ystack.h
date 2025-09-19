/*
 * YOS
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#ifndef _Y_STACK_H_
#define _Y_STACK_H_

#include "common_def.h"

#ifdef __cplusplus
extern "C" {
#endif

#define YOS_SRAM_BASE_ADDRESS				0x20000000
#define YOS_SRAM_END_ADDRESS				(YOS_SRAM_BASE_ADDRESS + SRAM_SIZE)

#define YOS_MAIN_STACK_SIZE					(1024 * 1)
#define YOS_PROCESS_STACK_ADDRESS			(YOS_SRAM_END_ADDRESS - YOS_MAIN_STACK_SIZE)

#define SAVE_PSP												\
	__asm__ __volatile__ (										\
		"mrs r0, psp									\n\t"	\
		"isb											\n\t"	\
	);


#define SAVE_REGS_TO_STACK										\
	__asm__ __volatile__ (										\
		"stmdb r0!, {r4-r11}							\n\t"	\
	);


#define SAVE_LR_TO_STACK										\
	__asm__ __volatile__ (										\
		"push {lr}										\n\t"	\
	);


#define RESTORE_LR_FROM_STACK									\
	__asm__ __volatile__ (										\
		"pop {lr}										\n\t"	\
	);


#define RESTORE_REGS_FROM_STACK									\
	__asm__ __volatile__ (										\
		"ldmia r0!, {r4-r11}							\n\t"	\
	);


#define RESTORE_PSP												\
	__asm__ __volatile__ (										\
		"msr psp, r0									\n\t"	\
		"isb											\n\t"	\
	);


#ifdef __cplusplus
}
#endif
#endif
