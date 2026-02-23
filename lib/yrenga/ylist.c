/*
 * YRenga
 *
 * YList Source File
 *
 * Copyright(C) 2020 Ashibananon(Yuan).
 *
 */

#include <stdio.h>
#include "ylist.h"

int YListInit(struct YList *list, void *node_list, uint32_t node_size, uint32_t node_count)
{
	return -1;
}

int YListDestroy(struct YList *list)
{
	return -1;
}

uint32_t YListGetNodeSize(struct YList *list)
{
	return 0;
}

uint32_t YListGetNodeTotalCount(struct YList *list)
{
	return 0;
}

uint32_t YListGetNodeCurrentCount(struct YList *list)
{
	return 0;
}

void *YListGetNodeAt(struct YList *list, uint32_t index)
{
	return NULL;
}

int YListInsertNodeAt(struct YList *list, uint32_t index, void *node)
{
	return -1;
}

int YListRemoveNodeAt(struct YList *list, uint32_t index)
{
	return -1;
}
