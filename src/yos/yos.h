/*
 * YOS
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#ifndef _YOS_H_
#define _YOS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define YOS_DEBUG_MSG_OUTPUT		1

#define YOS_RECORD_STACK_USAGE		1

#define YOS_TICK_HZ					100

/*
 * Maxium length of a task name, terminating '\0' character included
 *
 * タスク名の最大長さ（バイト）、ターミネーター「\0」は含まれます
 */
#define YOS_TASK_NAME_MAX_LENGTH	8

/*
 * Maxium number of tasks can be run on YOS, idle task is always there
 *
 * YOSで動けるタスクの最大数、アイドルタスクはいつでも存在しています
 */
#define YOS_MAX_TASK_COUNT			8

#define U16_HIGH_BYTE(u16)		((uint8_t)(((uint16_t)(u16)) >> 8))
#define U16_LOW_BYTE(u16)		((uint8_t)(((uint16_t)(u16)) & 0xFF))
#define U8HL_TO_U16(u8h, u8l)	((uint16_t)((((uint16_t)(u8h)) << 8) | (uint8_t)(u8l)))


/*
 * Task status changing diagram(May differ from the real implementation)
 *
 * (start)        (call yos_create_task)(first time being scheduled)
 * [Invalid] ---> [Created] ------------------> [Running]
 *     ^                                         |     ^
 *     |                                         |     |
 *     |                          (delay called) |     | (delay time over and scheduled to run again)
 *     |                                         |     |
 *     |                                         V     |
 *     | (next time being scheduled)            [Waiting]
 *     |                                            |
 *     ---------- [Exited] <<------------------------
 *                            (task function returns)
 *                            (or yos_delete_task called)
 *
 *
 * タスク状態の遷移図（本当の実現と異なる場合があります）
 *
 * (開始)         (yos_create_taskを呼び出す)(初めにスゲージュル)
 * [無効   ] ---> [新規    ] ------------------> [動いている]
 *     ^                                         |     ^
 *     |                                         |     |
 *     |                        (delay呼び出した) |     | (delayタイムアップ且つ次の動けるタスクとして選ばれる)
 *     |                                         |     |
 *     |                                         V     |
 *     | (次にスゲージュル)                      [待ち合わせ]
 *     |                                            |
 *     ---------- [終了  ] <<------------------------
 *                            (タスク関数戻った)
 *                            (またyos_delete_taskは呼び出した)
 */
enum yos_task_status {
	/*
	 * Not being as a task
	 *
	 * 無効タスク
	 */
	YOS_TASK_STATUS_INVALID = -1,

	/*
	 * Task is created but has not been scheduled to run yet
	 *
	 * 新規タスク、一度もスゲージュルされて動くことはない
	 */
	YOS_TASK_STATUS_CREATED,

	/*
	 * Task is running now, or runnable
	 *
	 * タスクは動いている、また動ける
	 */
	YOS_TASK_STATUS_RUNNING,

	/*
	 * Task is waiting for the wait-time over
	 * (e.g After yos_task_delay or yos_task_msleep is called)
	 *
	 * タスクは待ち合わせている
	 * （例：yos_task_delayまたはyos_task_msleepを呼び出した）
	 */
	YOS_TASK_STATUS_WAITING,

	/*
	 * Task function is returned but its resource(stack, etc) has not
	 * been taken back yet
	 * (Not implemented yet)
	 *
	 * タスク関数は戻ったが関連するリソースはまだ回収されていません
	 * （まだ実現していません）
	 */
	YOS_TASK_STATUS_EXITED
};

/*
 * YOS init
 * This must be called first to use YOS
 *
 * YOS初期化
 * YOSを利用にはこの関数は一番で呼び出すことは必要です
 */
void yos_init(void);

/*
 * Create a task to run on YOS
 *
 * task_func shall be a funciton that never returns,
 * and stack_size shall be set properly
 *
 * Task id will be returned if task is created successfully,
 * or a minus return value means that some error occured.
 *
 *
 * YOSタスクを登録する関数です
 *
 * タスク関数には無限ループにするようにしてください。
 * そしてスタックサイズも適当に設定してください。
 *
 * 登録成功の場合、0また正数をタスクIDとして戻ります。
 * 登録失敗の場合、負数を戻ります。
 */
int yos_create_task(int (*task_func)(void *task_data), void *data,
						uint16_t stack_size, char *name);

/*
 * Delete a task.
 * Not implemented and don't call this function
 *
 * タスクを削除する関数です
 * 実現していないため、呼び出さないでください。
 */
int yos_delete_task(int task_id);

/*
 * Start YOS
 *
 * Notice that this function will not return to the caller, and
 * all the stacks by now will be gone.
 *
 * YOSを開始します
 *
 * この関数は呼び出し元に戻りません、それにここまでのすべてのスタック
 * を失うことをご注意ください。
 */
void yos_start(void);

/*
 * Delay current task for specified ticks
 *
 * タスクを指定するtickの間に待ち合わせます
 */
void yos_task_delay(uint16_t ticks);

/*
 * Sleep for at least miliseconds.
 *
 * Note that the real sleep time may not be as exact as the value
 * specified, since it is implemented by task scheduler.
 *
 * For an accurate timing, please use timer function instead.
 *
 *
 * タスクを指定する時間（ミリ秒）で待ち合わせます
 *
 * 精確に指定される時間で待ち合わせることはありません、スゲージュルされる
 * ことのためです。
 *
 * 精確な時間計算には、timer関数をご利用ください
 */
void yos_task_msleep(uint16_t ms);

/*
 * Give up the current executing chance
 *
 * 今回の動くチャンスを放棄します
 */
void schedule(void);


struct yos_task_info {
	int id;
	enum yos_task_status status;
	uint16_t stack_size;
	uint16_t stack_max_reached_size;
	char name[YOS_TASK_NAME_MAX_LENGTH];
};

/*
 * Get task info by task id
 *
 * If returns 0, the task info will be stored in the space pointed by
 * [task_info]. Any other return values means error.
 *
 *
 * タスク情報を取得します
 *
 * 0を戻る場合、タスク情報は「task_info」でポイントしている領域に保存されます。
 * その他の値を戻る場合、エラーが発生したこととなります。
 */
int yos_get_task_info(int task_id, struct yos_task_info *task_info);

#if (YOS_DEBUG_MSG_OUTPUT == 1)
#include "../../lib/cmdline/basic_io.h"
#define YOS_DBG(...)		basic_io_printf("[YOS]"__VA_ARGS__)
#else
#define YOS_DBG(...)
#endif

#ifdef __cplusplus
}
#endif
#endif
