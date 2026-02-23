/*
 * YRenga
 *
 * YRingBuffer Header File
 *
 * Copyright(C) 2020 Ashibananon(Yuan).
 *
 */

#ifndef _Y_RING_BUFFER_H_
#define _Y_RING_BUFFER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "yrenga.h"

struct YRingBuffer {
	/* Buffer */
	void *data;
	/* size of item */
	uint32_t size_of_item;
	/* Total number of items the buffer can hold */
	uint32_t size;
	/* Position of the first item */
	uint32_t volatile start;
	/* Position for the next coming item */
	uint32_t volatile end;
	/* Current number of items in the buffer */
	uint32_t volatile current_len;
	/* How many items are taken */
	uint32_t volatile taken_item_num;
	uint32_t volatile blank_item_taken_num;
	/* Function that enters critical section */
	void (*enter_critical)(void);
	/* Function that leaves critical section */
	void (*leave_critical)(void);
};

#if (YRENGA_WITH_HEAP_OPERATIONS == 1)
struct YRingBuffer *YRingBufferNew(uint32_t size_of_item, uint32_t total_item_count);
void YRingBufferDelete(struct YRingBuffer *rb);
#endif

/*
 * Init Ring Buffer with given parameters
 *     [rb]                      The Ring Buffer obj that to be initialized
 *     [data]                    Memory area to store all the items
 *     [size_of_item]            Size of item
 *     [total_item_count]        Total count of items that the Ring Buffer can hold
 *
 * Note: This function does NOT use heap memory allocation functions like malloc, so that
 *       [data] parameter should be pointing to a memory area large enough for all of the
 *       items in advance.
 *
 * Return 0 if successfully initialized, otherwise means error.
 *
 * 環状バッファを各パラメーターによって初期化します
 *     [rb]                      初期化する環状バッファ
 *     [data]                    全部の項目を格納するメモリ領域
 *     [size_of_item]            項目のサイズ
 *     [total_item_count]        環状バッファに格納される項目の最大数
 *
 * ノート：この関数はmallocなどのヒープメモリ領域の割り振り関数を利用しません。あるいは「data」パラメーター
 * が指定するメモリ領域は全部数の項目を格納できて、予め確保するのは必要です。
 *
 * 初期化できたら0を返却します、それ以外の場合はエラーです
 */
int YRingBufferInit(struct YRingBuffer *rb, void *data,
					uint32_t size_of_item, uint32_t total_item_count,
					void (*func_enter_critical)(void), void (*func_leave_critical)(void));
void YRingBufferDestory(struct YRingBuffer *rb);

uint32_t YRingBufferPutItems(struct YRingBuffer *rb, void *data, uint32_t item_count, int drop_if_full);
uint32_t YRingBufferPutItemsInCritical(struct YRingBuffer *rb, void *data, uint32_t item_count, int drop_if_full);

uint32_t YRingBufferGetItems(struct YRingBuffer *rb, void *data, uint32_t item_count);
uint32_t YRingBufferGetItemsInCritical(struct YRingBuffer *rb, void *data, uint32_t item_count);

void *YRingBufferTakeAnItemAddress(struct YRingBuffer *rb);
void *YRingBufferTakeAnItemAddressInCritical(struct YRingBuffer *rb);
int YRingBufferReturnTheItemAddress(struct YRingBuffer *rb);
int YRingBufferReturnTheItemAddressInCritical(struct YRingBuffer *rb);
uint32_t YRingBufferGetTakenItemNumber(struct YRingBuffer *rb);
uint32_t YRingBufferGetTakenItemNumberInCritical(struct YRingBuffer *rb);

void *YRingBufferTakeAnBlankItemAddress(struct YRingBuffer *rb, int force_on_full);
void *YRingBufferTakeAnBlankItemAddressInCritical(struct YRingBuffer *rb, int force_on_full);
int YRingBufferReturnTheBlankItemAddress(struct YRingBuffer *rb);
int YRingBufferReturnTheBlankItemAddressInCritical(struct YRingBuffer *rb);


uint32_t YRingBufferGetItemSize(struct YRingBuffer *rb);
uint32_t YRingBufferGetItemSizeInCritical(struct YRingBuffer *rb);

uint32_t YRingBufferGetCurrentItemCount(struct YRingBuffer *rb);
uint32_t YRingBufferGetCurrentItemCountInCritical(struct YRingBuffer *rb);

uint32_t YRingBufferGetItemCapacity(struct YRingBuffer *rb);
uint32_t YRingBufferGetItemCapacityInCritical(struct YRingBuffer *rb);

void YRingBufferClear(struct YRingBuffer *rb);
void YRingBufferClearInCritical(struct YRingBuffer *rb);

#ifdef __cplusplus
}
#endif
#endif /* RING_BUF_H_ */
