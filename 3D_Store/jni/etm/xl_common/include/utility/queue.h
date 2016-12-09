#ifndef _SD_QUEUE_H_00138F8F2E70_200806260949
#define _SD_QUEUE_H_00138F8F2E70_200806260949

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/errcode.h"
#include "utility/define.h"
#include "utility/string.h"

#define MAX_QUEUE_VALUE	(0x7fff)
#define MAX_QUEUE_SIZE	(MAX_QUEUE_VALUE)

#define MIN_QUEUE_CAPACITY (2)

typedef struct {
	volatile _u16 _add_ref;
	volatile _u16 _sub_ref;
} QUEUE_INT;

#define QINT_ADD(int_data, addend)	(int_data._add_ref += addend)
#define QINT_SUB(int_data, subtrahend)	(int_data._sub_ref += subtrahend)
#define QINT_VALUE(int_data)	(_int16)(int_data._add_ref - int_data._sub_ref)
/* call in write thread(push) */
#define QINT_SET_VALUE_1(int_data, value)	(int_data._add_ref = int_data._sub_ref + value)
/* call in read thread(pop) */
#define QINT_SET_VALUE_2(int_data, value)	(int_data._sub_ref = int_data._add_ref - value)


typedef struct t_queue_node{
	void *_data;
	struct t_queue_node *_nxt_node;
}QUEUE_NODE, *pQUEUE_NODE;

typedef struct {
	/* its next node is the first node can be used*/
	QUEUE_NODE *_queue_head;
	/* its next node is the first free node*/
	QUEUE_NODE *_queue_tail;
	QUEUE_INT _queue_size;
	QUEUE_INT _queue_actual_capacity;
	QUEUE_INT _queue_capacity;

	/* optimation for alloc node, need not too accurate */
	_u16 _empty_count;
	_u16 _full_count;
} QUEUE;

/**/
#define QUEUE_AJUST_THRESHOLD	(10)
/* must > 1 */
#define QUEUE_REDUCE_TIMES		(2)
/* do not use parenthess, must > 1 */
#define QUEUE_ENLARGE_TIMES		3 / 2



_int32 queue_alloctor_init(void);
_int32 queue_alloctor_uninit(void);


_int32 queue_init(QUEUE *queue, _u32 capacity);

_int32 queue_uninit(QUEUE *queue);


/* @Simple Function@
 * Return : the size of queue
 */
_u32 queue_size(QUEUE *queue);


_int32 queue_push(QUEUE *queue, void *data);

/* get the last data-ptr of queue, can used for change queue-data directly in some special case. 
   it should be used with caution. */
_int32 queue_get_tail_ptr(QUEUE *queue, void **tail_ptr);

/* peek head, not popped */
_int32 queue_peek(QUEUE *queue, void **data);

/*
 * Return immediately, even if there were not any node popped.
 * If no node popped, the *data will be NULL.
 */
_int32 queue_pop(QUEUE *queue, void **data);


/* functions below are prepared for no-lock malloc */

/* when the queue full, return immediately with QUEUE_NO_ROOM */
_int32 queue_push_without_alloc(QUEUE *queue, void *data);

/**/
_int32 queue_pop_without_dealloc(QUEUE *queue, void **data);


/* recycle node up to capacity or queue_size.
 * it be used to recycle free-node as queue_pop_without_dealloc() can not do.
 */
_int32 queue_recycle(QUEUE *queue);

/* alloc new node to keep capacity
 * used to alloc new node as queue_push_without_alloc() can not do it.
 */
_int32 queue_reserved(QUEUE *queue, _u32 capacity);

/* check queue for optimazation of memory */
_int32 queue_check_full(QUEUE *queue);
_int32 queue_check_empty(QUEUE *queue);

#ifdef __cplusplus
}
#endif

#endif
