/*
 * YRenga
 *
 * YRenga Common Header File
 *
 * Copyright(C) 2020 Ashibananon(Yuan).
 *
 */

#ifndef _Y_RENGA_H_
#define _Y_RENGA_H_

#ifdef __cplusplus
extern "C" {
#endif

#define YRENGA_WITH_HEAP_OPERATIONS			0

#if (YRENGA_WITH_HEAP_OPERATIONS == 1)
#include <stdlib.h>
#endif


#ifdef __cplusplus
}
#endif

#endif
