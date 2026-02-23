/*
 * YOS
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#include <stdio.h>
#include "yqueue.h"

int yqueue_init(struct yqueue *q, void *buffer, uint32_t item_size, uint32_t length)
{
	int ret = -1;
	if (q == NULL || buffer == NULL || item_size == 0 || length == 0) {
		goto para_err;
	}

	ymutex_init(&q->mutex);
	if (YRingBufferInit(&q->rb, buffer, item_size, length, NULL, NULL) != 0) {
		goto ringbuffer_err;
	}

	ret = 0;
	return ret;

ringbuffer_err:
	ymutex_deinit(&q->mutex);
para_err:
	return ret;
}

void yqueue_deinit(struct yqueue *q)
{
	if (q == NULL) {
		goto para_err;
	}

	ymutex_deinit(&q->mutex);
	YRingBufferDestory(&q->rb);

para_err:
	return;
}

uint32_t yqueue_send_items(struct yqueue *q, void *items, uint32_t item_count)
{
	uint32_t cnt = 0;
	if (q == NULL || items == NULL || item_count == 0) {
		goto para_err;
	}

	ymutex_lock(&q->mutex);
	cnt = YRingBufferPutItems(&q->rb, items, item_count, 0);
	ymutex_unlock(&q->mutex);

para_err:
	return cnt;
}

uint32_t yqueue_receive_items(struct yqueue *q, void *items, uint32_t item_count)
{
	uint32_t cnt = 0;
	if (q == NULL || items == NULL || item_count == 0) {
		goto para_err;
	}

	ymutex_lock(&q->mutex);
	cnt = YRingBufferGetItems(&q->rb, items, item_count);
	ymutex_unlock(&q->mutex);

para_err:
	return cnt;
}

uint32_t yqueue_try_send_items(struct yqueue *q, void *items, uint32_t item_count)
{
	uint32_t cnt = 0;
	if (q == NULL || items == NULL || item_count == 0) {
		goto para_err;
	}

	if (ymutex_try_lock(&q->mutex) == 0) {
		cnt = YRingBufferPutItems(&q->rb, items, item_count, 0);
		ymutex_unlock(&q->mutex);
	}

para_err:
	return cnt;
}

uint32_t yqueue_try_receive_items(struct yqueue *q, void *items, uint32_t item_count)
{
	uint32_t cnt = 0;
	if (q == NULL || items == NULL || item_count == 0) {
		goto para_err;
	}

	if (ymutex_try_lock(&q->mutex) == 0) {
		cnt = YRingBufferGetItems(&q->rb, items, item_count);
		ymutex_unlock(&q->mutex);
	}

para_err:
	return cnt;
}
