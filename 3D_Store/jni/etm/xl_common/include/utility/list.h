#ifndef _SD_LIST_H_00138F8F2E70_200806260949
#define _SD_LIST_H_00138F8F2E70_200806260949

#ifdef __cplusplus
extern "C" 
{
#endif

/* NO Thread-Safe */

#include "utility/errcode.h"
#include "utility/define.h"
#include "utility/string.h"

typedef struct t_list_node{
	void *_data;
	struct t_list_node *_pre_node;
	struct t_list_node *_nxt_node;
}LIST_NODE, *pLIST_NODE;

typedef struct tagLIST{
	LIST_NODE _list_nil; /* not use pointer avoid to calling list_uninit() */
	_u32 _list_size;
} LIST;

typedef pLIST_NODE LIST_ITERATOR;

#define LIST_BEGIN(list)		((list)._list_nil._nxt_node)
#define LIST_END(list)			(&(list)._list_nil)
#define LIST_NEXT(iterator)		((iterator)->_nxt_node)

#define LIST_RBEGIN(list)		((list)._list_nil._pre_node)
#define LIST_PRE(iterator)		((iterator)->_pre_node)

#define	LIST_VALUE(iterator)	((iterator)->_data)

typedef _int32 (*data_comparator)(void *E1, void *E2);

_int32 list_alloctor_init(void);
_int32 list_alloctor_uninit(void);

/* @Simple Function@
 * Return : void
 */
void list_init(LIST *list);

/* @Simple Function@
 * Return : the size of list
 */
_u32 list_size(const LIST *list);


_int32 list_push(LIST *list, void *data);

/*
 * Return immediately, even if there were not any node popped.
 * If no node popped, the *data will be NULL.
 */
_int32 list_pop(LIST *list, void **data);

_int32 list_insert(LIST *list, void *data, LIST_ITERATOR insert_before);
_int32 list_erase(LIST *list, LIST_ITERATOR erase_node);


/* @Simple Function@
 * swap the head&tail of two LIST
 * Return : void
 */
void list_swap(LIST *list1, LIST *list2);


/* @Simple Function@
 * cat list2 to list1, and clear the list2
 * Return : void
 */
void list_cat_and_clear(LIST *list1, LIST *list2);


/* clear all node of list, the caller is responsable for destruction of elements */
_int32 list_clear(LIST *list);

#ifdef __cplusplus
}
#endif

#endif
