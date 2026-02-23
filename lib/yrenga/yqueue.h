/*
 * YOS
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#ifndef _Y_STACK_H_
#define _Y_STACK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "../../src/yos/ymutex.h"
#include "yringbuffer.h"

struct yqueue {
	struct ymutex mutex;
	struct YRingBuffer rb;
};

/*
 * Initialize a queue with given parameters
 * Return 0 if succeeds
 */
int yqueue_init(struct yqueue *q, void *buffer, uint32_t item_size, uint32_t length);
void yqueue_deinit(struct yqueue *q);

/*
 * Send [item_count] of [items] to queue
 * Return the real count of items that successfully sent to the queue
 */
uint32_t yqueue_send_items(struct yqueue *q, void *items, uint32_t item_count);

/*
 * Receive [item_count] of [items] from queue
 * Return the real count of items that successfully received from the queue
 */
uint32_t yqueue_receive_items(struct yqueue *q, void *items, uint32_t item_count);

/*
 * Try to send [item_count] of [items] to queue
 * Return the real count of items that successfully sent to the queue
 */
uint32_t yqueue_try_send_items(struct yqueue *q, void *items, uint32_t item_count);

/*
 * Try to receive [item_count] of [items] from queue
 * Return the real count of items that successfully received from the queue
 */
uint32_t yqueue_try_receive_items(struct yqueue *q, void *items, uint32_t item_count);

#ifdef __cplusplus
}
#endif
#endif
