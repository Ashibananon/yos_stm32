/*
 * YRenga
 *
 * YList Header File
 *
 * Copyright(C) 2020 Ashibananon(Yuan).
 *
 */

#ifndef _Y_LIST_H_
#define _Y_LIST_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "yrenga.h"

struct YList {
	void *node_list;
	uint32_t node_size;
	uint32_t node_total_count;
	uint32_t node_current_count;
};

/*
 * Initialize a list with given parameters
 * Return 0 if success. Otherwise error
 *
 * パラメーターを指定してリストを初期化します
 * 返却値0は成功です、それ以外はエラーです
 */
int YListInit(struct YList *list, void *node_list, uint32_t node_size, uint32_t node_count);

/*
 * Destroy a list
 * Return 0 if success. Otherwise error
 *
 * リストを廃棄します
 * 返却値0は成功です、それ以外はエラーです
 */
int YListDestroy(struct YList *list);

/*
 * Get node size of a list
 *
 * リストの項目サイズを取得します
 */
uint32_t YListGetNodeSize(struct YList *list);

/*
 * Get node total count of a list
 *
 * リストの項目総数を取得します
 */
uint32_t YListGetNodeTotalCount(struct YList *list);

/*
 * Get node current count of a list
 *
 * リストの現在項目数を取得します
 */
uint32_t YListGetNodeCurrentCount(struct YList *list);

/*
 * Get node at the given index
 * NULL is returned if error occurs
 *
 * 指定するindexでの項目を取得します
 * エラーが発生した場合NULLが返却値となります
 */
void *YListGetNodeAt(struct YList *list, uint32_t index);

/*
 * Insert a new [node] to the given [list] at the position specified by [index]
 * Return 0 if success. Otherwise error
 *
 * リスト「list」に新たな項目「node」を「index」のところに挿入します
 * 成功した場合0を返却します、それ以外はエラーです
 */
int YListInsertNodeAt(struct YList *list, uint32_t index, void *node);

/*
 * Remove the node of the given index
 * Return 0 if success. Otherwise error
 *
 * 指定するindexでの項目を削除します
 * 成功した場合0を返却します、それ以外はエラーです
 */
int YListRemoveNodeAt(struct YList *list, uint32_t index);

#ifdef __cplusplus
}
#endif
#endif
