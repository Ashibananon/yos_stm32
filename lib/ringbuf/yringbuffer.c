/*
 * YRenga
 *
 * YRingBuffer Source File
 *
 * Copyright(C) 2020 Ashibananon(Yuan).
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "yringbuffer.h"

struct YRingBuffer *YRingBufferNew(unsigned long size)
{
	struct YRingBuffer *rb = NULL;
	if (size <= 0) {
		goto size_err;
	}

	rb = (struct YRingBuffer *)malloc(sizeof(struct YRingBuffer));
	if (rb == NULL) {
		goto malloc_err;
	}

	rb->data = malloc(size);
	if (rb->data == NULL) {
		goto rb_data_err;
	}
	rb->size = size;
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

void YRingBufferInit(struct YRingBuffer *rb, void *data, unsigned long size)
{
	if (rb == NULL || data == NULL || size <= 0) {
		return;
	}

	rb->data = data;
	rb->size = size;
	rb->start = rb->end = 0;
	rb->current_len = 0;
}

void YRingBufferDestory(struct YRingBuffer *rb)
{
	if (rb == NULL) {
		return;
	}

	rb->data = NULL;
	rb->size = 0;
	rb->start = rb->end = 0;
	rb->current_len = 0;
}

unsigned long YRingBufferPutData(struct YRingBuffer *rb, void *data, unsigned long len, int drop_if_full)
{
	if (rb == NULL || rb->data == NULL || data == NULL) {
		return 0;
	}

	unsigned long cnt = 0;
	while (cnt < len) {
		if (rb->end == rb->start) {
			if (rb->current_len == rb->size) {
				/* Full */
				if (drop_if_full) {
					((char *)(rb->data))[rb->end++] = ((char *)data)[cnt++];
					rb->start++;
				} else {
					break;
				}
			} else {
				/* Empty */
				((char *)(rb->data))[rb->end++] = ((char *)data)[cnt++];
				rb->current_len++;
			}
		} else {
			((char *)(rb->data))[rb->end++] = ((char *)data)[cnt++];
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


unsigned long YRingBufferGetData(struct YRingBuffer *rb, void *data, unsigned long len)
{
	if (rb == NULL || rb->data == NULL || data == NULL) {
		return 0;
	}

	unsigned long cnt = 0;
	while (cnt < len) {
		if (rb->end == rb->start) {
			if (rb->current_len == rb->size) {
				/* Full */
				((char *)data)[cnt++] = ((char *)(rb->data))[rb->start++];
				rb->current_len--;
			} else {
				/* Empty */
				break;
			}
		} else {
			((char *)data)[cnt++] = ((char *)(rb->data))[rb->start++];
			rb->current_len--;
		}

		if (rb->start >= rb->size) {
			rb->start = 0;
		}
	}

	return cnt;
}


unsigned long YRingBufferGetCurrentLen(struct YRingBuffer *rb)
{
	if (rb == NULL) {
		return 0;
	}

	return rb->current_len;
}

unsigned long YRingBufferGetSize(struct YRingBuffer *rb)
{
	if (rb == NULL) {
		return 0;
	}

	return rb->size;
}

void YRingBufferClear(struct YRingBuffer *rb)
{
	if (rb == NULL) {
		return;
	}

	rb->start = 0;
	rb->end = 0;
	rb->current_len = 0;
}
