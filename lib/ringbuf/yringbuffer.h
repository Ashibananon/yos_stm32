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

struct YRingBuffer {
	void *data;
	unsigned long size;
	unsigned long start;
	unsigned long end;
	unsigned long current_len;
};

struct YRingBuffer *YRingBufferNew(unsigned long size);
void YRingBufferDelete(struct YRingBuffer *rb);

void YRingBufferInit(struct YRingBuffer *rb, void *data, unsigned long size);
void YRingBufferDestory(struct YRingBuffer *rb);

unsigned long YRingBufferPutData(struct YRingBuffer *rb, void *data, unsigned long len, int drop_if_full);
unsigned long YRingBufferGetData(struct YRingBuffer *rb, void *data, unsigned long len);

unsigned long YRingBufferGetCurrentLen(struct YRingBuffer *rb);
unsigned long YRingBufferGetSize(struct YRingBuffer *rb);

void YRingBufferClear(struct YRingBuffer *rb);

#ifdef __cplusplus
}
#endif
#endif /* RING_BUF_H_ */
