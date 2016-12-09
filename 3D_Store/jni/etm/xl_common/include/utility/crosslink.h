/*
 * crosslink_t.h
 *
 *  Created on: 2010-12-25
 *      Author: lihai
 */

#ifndef _SD_CROSSLINK_H_00138F8F2E70_200806260949
#define _SD_CROSSLINK_H_00138F8F2E70_200806260949

#define COL_LEFT 0
#define COL_RIGHT 1
#define ROW_TOP 2
#define ROW_BOTTOM 3

#ifdef __cplusplus
extern "C" 
{
#endif

/* NO Thread-Safe */

#include "utility/errcode.h"
#include "utility/define.h"
#include "utility/string.h"

typedef struct t_crosslink_node{
	void *_data;
	struct t_crosslink_node *left_sibiling;
	struct t_crosslink_node *right_sibiling;
	struct t_crosslink_node *up_sibiling;
	struct t_crosslink_node *down_sibiling;
}CROSSLINK_NODE, *pCROSSLINK_NODE;

typedef struct tagCROSSLINK{
	pCROSSLINK_NODE head;
	pCROSSLINK_NODE tail;
	_int32 row;
	_int32 col;
} CROSSLINK;

typedef pCROSSLINK_NODE CROSSLINK_ITERATOR;

#define CROSSLINK_VALUE(iterator)  ((iterator)->_data)
#define CROSSLINK_HEAD(crosslink) ((crosslink)->head)
#define CROSSLINK_TAIL(crosslink) ((crosslink)->tail)

#define CROSSLINK_UP(iterator)			((iterator)->up_sibiling)
#define CROSSLINK_DOWN(iterator)		((iterator)->down_sibiling)
#define CROSSLINK_LEFT(iterator)		((iterator)->left_sibiling)
#define CROSSLINK_RIGHT(iterator)		((iterator)->right_sibiling)

_int32 crosslink_alloctor_init(void);
_int32 crosslink_alloctor_uninit(void);

/* @Simple Function@
 * Return : void
 */
void crosslink_init(CROSSLINK *crosslink, _int32 row, _int32 col);
void crosslink_destroy(CROSSLINK *crosslink);

/* @Simple Function@
 * Return : the size of crosslink
 */
_u32 crosslink_row(const CROSSLINK *crosslink);
_u32 crosslink_col(const CROSSLINK *crosslink);

CROSSLINK_ITERATOR get_crosslink_cell(CROSSLINK *crosslink, _int32 row, _int32 col);

_int32 crosslink_push(CROSSLINK *crosslink, void *data, _int32 row, _int32 col);

/*
 * Return immediately, even if there were not any node popped.
 * If no node popped, the *data will be NULL.
 */
_int32 crosslink_pop(CROSSLINK *crosslink, void **data, _int32 row, _int32 col);

_int32 crosslink_insert(CROSSLINK *crosslink, void *data, CROSSLINK_ITERATOR insert_before);
_int32 crosslink_erase(CROSSLINK *crosslink, CROSSLINK_ITERATOR erase_node);


/* clear all node of crosslink, the caller is responsable for destruction of elements */
_int32 crosslink_clear(CROSSLINK *crosslink);

_int32 crosslink_add_row(CROSSLINK *crosslink, _int32 pos/*ROW_TOP,ROW_BOTTOM*/);
_int32 crosslink_add_col(CROSSLINK *crosslink, _int32 pos/*COL_LEFT,COL_RIGHT*/);

_int32 crosslink_delete_row(CROSSLINK *crosslink, _int32 row);
_int32 crosslink_delete_col(CROSSLINK *crosslink, _int32 col);

#ifdef __cplusplus
}
#endif

#endif
