/*
 * YRenga
 *
 * YRingBuffer Source File
 *
 * Copyright(C) 2020 Ashibananon(Yuan).
 *
 */

#include <stdio.h>
#include <string.h>
#include "yringbuffer.h"

#if (YRENGA_WITH_HEAP_OPERATIONS == 1)
struct YRingBuffer *YRingBufferNew(uint32_t size_of_item, uint32_t total_item_count)
{
	struct YRingBuffer *rb = NULL;
	if (size_of_item <= 0 || total_item_count <= 0) {
		goto size_err;
	}

	rb = (struct YRingBuffer *)malloc(sizeof(struct YRingBuffer));
	if (rb == NULL) {
		goto malloc_err;
	}

	rb->data = malloc(size_of_item * total_item_count);
	if (rb->data == NULL) {
		goto rb_data_err;
	}
	rb->size_of_item = size_of_item;
	rb->size = total_item_count;
	rb->start = rb->end = 0;
	rb->current_len = 0;

	return rb;

rb_data_err:
	free(rb);
	rb = NULL;
malloc_err:
size_err:
	return rb;
}

void YRingBufferDelete(struct YRingBuffer *rb)
{
	if (rb != NULL) {
		if (rb->data != NULL) {
			free(rb->data);
		}
		free(rb);
	}
}
#endif

static void _YRingBufferEnterCritical(struct YRingBuffer *rb)
{
	if (rb != NULL && rb->enter_critical != NULL) {
		rb->enter_critical();
	}
}

static void _YRingBufferLeaveCritical(struct YRingBuffer *rb)
{
	if (rb != NULL && rb->leave_critical != NULL) {
		rb->leave_critical();
	}
}

int YRingBufferInit(struct YRingBuffer *rb, void *data,
					uint32_t size_of_item, uint32_t total_item_count,
					void (*func_enter_critical)(void), void (*func_leave_critical)(void))
{
	int ret = -1;
	if (rb == NULL || data == NULL || size_of_item <= 0 || total_item_count <= 0) {
		goto para_err;
	}

	rb->data = data;
	rb->size_of_item = size_of_item;
	rb->size = total_item_count;
	rb->start = rb->end = 0;
	rb->current_len = 0;
	rb->taken_item_num = 0;
	rb->enter_critical = func_enter_critical;
	rb->leave_critical = func_leave_critical;

	ret = 0;

para_err:
	return ret;
}

void YRingBufferDestory(struct YRingBuffer *rb)
{
	if (rb == NULL) {
		return;
	}

	rb->data = NULL;
	rb->size_of_item = 0;
	rb->size = 0;
	rb->start = rb->end = 0;
	rb->current_len = 0;
	rb->taken_item_num = 0;
	rb->enter_critical = NULL;
	rb->leave_critical = NULL;
}

uint32_t YRingBufferPutItems(struct YRingBuffer *rb, void *data, uint32_t item_count, int drop_if_full)
{
	uint32_t ret;

	_YRingBufferEnterCritical(rb);
	ret = YRingBufferPutItemsInCritical(rb, data, item_count, drop_if_full);
	_YRingBufferLeaveCritical(rb);

	return ret;
}

uint32_t YRingBufferPutItemsInCritical(struct YRingBuffer *rb, void *data, uint32_t item_count, int drop_if_full)
{
	if (rb == NULL || rb->data == NULL || data == NULL) {
		return 0;
	}

	if (rb->blank_item_taken_num > 0) {
		return 0;
	}

	uint32_t cnt = 0;
	while (cnt < item_count) {
		if (rb->end == rb->start) {
			if (rb->current_len == rb->size) {
				/* Full */
				if (drop_if_full) {
					//((char *)(rb->data))[rb->end++] = ((char *)data)[cnt++];
					memcpy((char *)rb->data + rb->end * rb->size_of_item,
							(char *)data + cnt * rb->size_of_item,
							rb->size_of_item);
					rb->end++;
					cnt++;
					rb->start++;
				} else {
					break;
				}
			} else {
				/* Empty */
				//((char *)(rb->data))[rb->end++] = ((char *)data)[cnt++];
				memcpy((char *)rb->data + rb->end * rb->size_of_item,
						(char *)data + cnt * rb->size_of_item,
						rb->size_of_item);
				rb->end++;
				cnt++;
				rb->current_len++;
			}
		} else {
			//((char *)(rb->data))[rb->end++] = ((char *)data)[cnt++];
			memcpy((char *)rb->data + rb->end * rb->size_of_item,
					(char *)data + cnt * rb->size_of_item,
					rb->size_of_item);
			rb->end++;
			cnt++;
			rb->current_len++;
		}

		if (rb->start >= rb->size) {
			rb->start = 0;
		}
		if (rb->end >= rb->size) {
			rb->end = 0;
		}
	}

	return cnt;
}

uint32_t YRingBufferGetItems(struct YRingBuffer *rb, void *data, uint32_t item_count)
{
	uint32_t ret;

	_YRingBufferEnterCritical(rb);
	ret = YRingBufferGetItemsInCritical(rb, data, item_count);
	_YRingBufferLeaveCritical(rb);

	return ret;
}

uint32_t YRingBufferGetItemsInCritical(struct YRingBuffer *rb, void *data, uint32_t item_count)
{
	if (rb == NULL || rb->data == NULL || data == NULL) {
		return 0;
	}

	uint32_t cnt = 0;
	while (cnt < item_count) {
		if (rb->end == rb->start) {
			if (rb->current_len == rb->size) {
				/* Full */
				//((char *)data)[cnt++] = ((char *)(rb->data))[rb->start++];
				memcpy((char *)data + cnt * rb->size_of_item,
						(char *)rb->data + rb->start * rb->size_of_item,
						rb->size_of_item);
				cnt++;
				rb->start++;
				rb->current_len--;
				if (rb->taken_item_num > 0) {
					rb->taken_item_num--;
				}
			} else {
				/* Empty */
				break;
			}
		} else {
			//((char *)data)[cnt++] = ((char *)(rb->data))[rb->start++];
			memcpy((char *)data + cnt * rb->size_of_item,
					(char *)rb->data + rb->start * rb->size_of_item,
					rb->size_of_item);
			cnt++;
			rb->start++;
			rb->current_len--;
			if (rb->taken_item_num > 0) {
				rb->taken_item_num--;
			}
		}

		if (rb->start >= rb->size) {
			rb->start = 0;
		}
	}

	return cnt;
}

void *YRingBufferTakeAnItemAddress(struct YRingBuffer *rb)
{
	void *ret;

	_YRingBufferEnterCritical(rb);
	ret = YRingBufferTakeAnItemAddressInCritical(rb);
	_YRingBufferLeaveCritical(rb);

	return ret;
}

void *YRingBufferTakeAnItemAddressInCritical(struct YRingBuffer *rb)
{
	void *ret = NULL;
	int32_t item_index = -1;
	if (rb == NULL || rb->data == NULL) {
		goto para_err;
	}

	if (rb->taken_item_num >= rb->current_len) {
		goto no_more_can_be_taken;
	}

	if (rb->end == rb->start) {
		if (rb->current_len == rb->size) {
			/* Full */
			item_index = rb->start + rb->taken_item_num;
			rb->taken_item_num++;
		} else {
			/* Empty */
		}
	} else {
		item_index = rb->start + rb->taken_item_num;
		rb->taken_item_num++;
	}

	if (item_index >= 0) {
		if (item_index >= rb->size) {
			item_index -= rb->size;
		}

		ret = (char *)rb->data
			+ item_index * rb->size_of_item;
	}

no_more_can_be_taken:
para_err:
	return ret;
}

int YRingBufferReturnTheItemAddress(struct YRingBuffer *rb)
{
	int ret;

	_YRingBufferEnterCritical(rb);
	ret = YRingBufferReturnTheItemAddressInCritical(rb);
	_YRingBufferLeaveCritical(rb);

	return ret;
}

int YRingBufferReturnTheItemAddressInCritical(struct YRingBuffer *rb)
{
	int ret = -1;
	if (rb == NULL || rb->data == NULL) {
		goto para_err;
	}

	if (rb->taken_item_num == 0) {
		goto no_item_is_taken;
	}

	do {
		if (rb->end == rb->start) {
			if (rb->current_len == rb->size) {
				/* Full */
				rb->start++;
				rb->current_len--;
				rb->taken_item_num--;
				ret = 0;
			} else {
				/* Empty */
				break;
			}
		} else {
			rb->start++;
			rb->current_len--;
			rb->taken_item_num--;
			ret = 0;
		}

		if (rb->start >= rb->size) {
			rb->start = 0;
		}
	} while (0);

no_item_is_taken:
para_err:
	return ret;
}

uint32_t YRingBufferGetTakenItemNumber(struct YRingBuffer *rb)
{
	uint32_t ret;

	_YRingBufferEnterCritical(rb);
	ret = YRingBufferGetTakenItemNumberInCritical(rb);
	_YRingBufferLeaveCritical(rb);

	return ret;
}

uint32_t YRingBufferGetTakenItemNumberInCritical(struct YRingBuffer *rb)
{
	uint32_t ret = 0;
	if (rb == NULL) {
		goto para_err;
	}

	ret = rb->taken_item_num;

para_err:
	return ret;
}

void *YRingBufferTakeAnBlankItemAddress(struct YRingBuffer *rb, int force_on_full)
{
	void *ret;

	_YRingBufferEnterCritical(rb);
	ret = YRingBufferTakeAnBlankItemAddressInCritical(rb, force_on_full);
	_YRingBufferLeaveCritical(rb);

para_err:
	return ret;
}

void *YRingBufferTakeAnBlankItemAddressInCritical(struct YRingBuffer *rb, int force_on_full)
{
	void *ret = NULL;
	if (rb == NULL) {
		goto para_err;
	}
	if (rb->blank_item_taken_num > 0) {
		goto cannot_take_more_blank_addr;
	}

	do {
		if (rb->end == rb->start) {
			if (rb->current_len == rb->size) {
				/* Full */
				if (force_on_full) {
					//((char *)(rb->data))[rb->end++] = ((char *)data)[cnt++];
					ret = (char *)rb->data + rb->end * rb->size_of_item;
					rb->start++;
					rb->current_len--;
					rb->blank_item_taken_num++;
				} else {
					break;
				}
			} else {
				/* Empty */
				//((char *)(rb->data))[rb->end++] = ((char *)data)[cnt++];
				ret = (char *)rb->data + rb->end * rb->size_of_item;
				rb->blank_item_taken_num++;
			}
		} else {
			//((char *)(rb->data))[rb->end++] = ((char *)data)[cnt++];
			ret = (char *)rb->data + rb->end * rb->size_of_item;
			rb->blank_item_taken_num++;
		}

		if (rb->start >= rb->size) {
			rb->start = 0;
		}
		if (rb->end >= rb->size) {
			rb->end = 0;
		}
	} while (0);

cannot_take_more_blank_addr:
para_err:
	return ret;
}

int YRingBufferReturnTheBlankItemAddress(struct YRingBuffer *rb)
{
	int ret;
	_YRingBufferEnterCritical(rb);
	ret = YRingBufferReturnTheBlankItemAddressInCritical(rb);
	_YRingBufferLeaveCritical(rb);

	return ret;
}

int YRingBufferReturnTheBlankItemAddressInCritical(struct YRingBuffer *rb)
{
	int ret = -1;
	if (rb == NULL) {
		goto para_err;
	}

	if (rb->blank_item_taken_num == 0) {
		goto no_taken_item;
	}

#if 0
	if (((char *)rb->data + rb->end * rb->size_of_item) != addr) {
		goto bad_addr;
	}
#endif

	do {
		if (rb->end == rb->start) {
			if (rb->current_len == rb->size) {
				/* Full */
				rb->end++;
				rb->blank_item_taken_num--;
				ret = 0;
			} else {
				/* Empty */
				rb->end++;
				rb->current_len++;
				rb->blank_item_taken_num--;
				ret = 0;
			}
		} else {
			rb->end++;
			rb->current_len++;
			rb->blank_item_taken_num--;
			ret = 0;
		}

		if (rb->start >= rb->size) {
			rb->start = 0;
		}
		if (rb->end >= rb->size) {
			rb->end = 0;
		}
	} while (0);

bad_addr:
no_taken_item:
para_err:
	return ret;
}

uint32_t YRingBufferGetItemSize(struct YRingBuffer *rb)
{
	uint32_t ret;

	_YRingBufferEnterCritical(rb);
	ret = YRingBufferGetItemSizeInCritical(rb);
	_YRingBufferLeaveCritical(rb);

	return ret;
}

uint32_t YRingBufferGetItemSizeInCritical(struct YRingBuffer *rb)
{
	if (rb == NULL) {
		return 0;
	}

	return rb->size_of_item;
}

uint32_t YRingBufferGetCurrentItemCount(struct YRingBuffer *rb)
{
	uint32_t ret;

	_YRingBufferEnterCritical(rb);
	ret = YRingBufferGetCurrentItemCountInCritical(rb);
	_YRingBufferLeaveCritical(rb);

	return ret;
}

uint32_t YRingBufferGetCurrentItemCountInCritical(struct YRingBuffer *rb)
{
	if (rb == NULL) {
		return 0;
	}

	return rb->current_len;
}

uint32_t YRingBufferGetItemCapacity(struct YRingBuffer *rb)
{
	uint32_t ret;

	_YRingBufferEnterCritical(rb);
	ret = YRingBufferGetItemCapacityInCritical(rb);
	_YRingBufferLeaveCritical(rb);

	return ret;
}

uint32_t YRingBufferGetItemCapacityInCritical(struct YRingBuffer *rb)
{
	if (rb == NULL) {
		return 0;
	}

	return rb->size;
}

void YRingBufferClear(struct YRingBuffer *rb)
{
	_YRingBufferEnterCritical(rb);
	YRingBufferClearInCritical(rb);
	_YRingBufferLeaveCritical(rb);
}

void YRingBufferClearInCritical(struct YRingBuffer *rb)
{
	if (rb == NULL) {
		return;
	}

	memset(rb->data, 0x00, rb->size_of_item * rb->size);
	rb->start = 0;
	rb->end = 0;
	rb->current_len = 0;
	rb->taken_item_num = 0;
	rb->blank_item_taken_num = 0;
}
