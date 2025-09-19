/*
 * YOS
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#ifndef _Y_MUTEX_H_
#define _Y_MUTEX_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ymutex {
	volatile int owner;
};

void ymutex_init(struct ymutex *mutex);

/*
 * Block until got mutex
 *
 * mutexを取得するまでブロックされます
 */
void ymutex_lock(struct ymutex *mutex);

/*
 * Try to get mutex
 * Return 0 if got mutex, or other value returns
 *
 * mutexを取得試行します
 * 0を戻る場合、mutexを取得できたことになります
 * その他の値を戻る場合、mutexを取得できないことになります
 */
int ymutex_try_lock(struct ymutex *mutex);

/*
 * Release mutex if it is acquired by current
 * Return 0 if released mutex, or other value returns
 *
 * 取得したmutexを解放します
 * 0を戻る場合、mutexを解放したことになります
 * その他の値を戻る場合、mutexを解放できないことになります
 */
int ymutex_unlock(struct ymutex *mutex);

#ifdef __cplusplus
}
#endif
#endif
