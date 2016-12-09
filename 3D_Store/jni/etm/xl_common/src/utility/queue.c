#include "utility/queue.h"
#include "utility/utility.h"
#include "utility/errcode.h"
#include "utility/mempool.h"

static SLAB *gp_queueslab = NULL;

_int32 queue_alloctor_init(void)
{
	_int32 ret_val = SUCCESS;
	if (NULL == gp_queueslab)
	{
		ret_val = mpool_create_slab(sizeof(QUEUE_NODE), MIN_QUEUE_MEMORY, 0, &gp_queueslab);
		CHECK_VALUE(ret_val);
	}
	return ret_val;
}

_int32 queue_alloctor_uninit(void)
{
	_int32 ret_val = SUCCESS;
	if (NULL != gp_queueslab)
	{
		ret_val = mpool_destory_slab(gp_queueslab);
		CHECK_VALUE(ret_val);
		gp_queueslab = NULL;
	}
	return ret_val;
}

_int32 queue_init(QUEUE *queue, _u32 capacity)
{
    if (capacity < MIN_QUEUE_CAPACITY)
    {
        capacity = MIN_QUEUE_CAPACITY;
    }

	sd_memset(queue, 0, sizeof(QUEUE));
	QINT_SET_VALUE_1(queue->_queue_capacity, capacity);

    QUEUE_NODE *free_node = NULL;
	/* pre-alloc two node */
	_int32 ret_val = mpool_get_slip(gp_queueslab, (void**)&free_node);
	CHECK_VALUE(ret_val);
    sd_memset(free_node, 0, sizeof(QUEUE_NODE));
	queue->_queue_head = free_node;

	ret_val = mpool_get_slip(gp_queueslab, (void**)&free_node);
	CHECK_VALUE(ret_val);
    sd_memset(free_node, 0, sizeof(QUEUE_NODE));
	queue->_queue_tail = free_node;

	queue->_queue_head->_nxt_node = queue->_queue_tail;
	queue->_queue_tail->_nxt_node = queue->_queue_head;

	queue->_empty_count = queue->_full_count = 0;

	return ret_val;
}

_int32 queue_uninit(QUEUE *queue)
{
    _int32 ret_val = SUCCESS;
    
	_int32 count = 0;
	QUEUE_NODE *node = (QUEUE_NODE*)queue->_queue_head->_nxt_node;
	QUEUE_NODE *pre_node = NULL;

	/* free all free node, excluding head & tail */
	for (; count < QINT_VALUE(queue->_queue_size); count++)
	{
		pre_node = node;
		node = (QUEUE_NODE*)node->_nxt_node;
		ret_val = mpool_free_slip(gp_queueslab, pre_node);
		CHECK_VALUE(ret_val);
	}

	/* free head & tail */
	ret_val = mpool_free_slip(gp_queueslab, (void*)queue->_queue_head);
	CHECK_VALUE(ret_val);

	ret_val = mpool_free_slip(gp_queueslab, (void*)queue->_queue_tail);
	CHECK_VALUE(ret_val);

	return ret_val;
}

_u32 queue_size(QUEUE *queue)
{
	sd_assert(queue != NULL);
	return QINT_VALUE(queue->_queue_size);
}

_int32 queue_push(QUEUE *queue, void *node)
{
	_int32 ret_val = SUCCESS;
	QUEUE_NODE *free_node = NULL;

	sd_assert(queue != NULL);

	if (QINT_VALUE(queue->_queue_size) >= QINT_VALUE(queue->_queue_actual_capacity))
	{
		/* allocate new node */
		ret_val = mpool_get_slip(gp_queueslab, (void**)&free_node);
		CHECK_VALUE(ret_val);

		/* init node */
		sd_memset(free_node, 0, sizeof(QUEUE_NODE));
		free_node->_data = node;
		free_node->_nxt_node = queue->_queue_tail->_nxt_node;
		queue->_queue_tail->_nxt_node = free_node;
		QINT_ADD(queue->_queue_actual_capacity, 1);
	}

	queue->_queue_tail->_nxt_node->_data = node;
	queue->_queue_tail = queue->_queue_tail->_nxt_node;

	QINT_ADD(queue->_queue_size, 1);

	return ret_val;
}

_int32 queue_get_tail_ptr(QUEUE *queue, void **tail_ptr)
{
	*tail_ptr = (void*)(&queue->_queue_tail->_data);
	return SUCCESS;
}

_int32 queue_peek(QUEUE *queue, void **data)
{
	*data = NULL;
	if (QINT_VALUE(queue->_queue_size) > 0)
	{
	    *data = (void*)queue->_queue_head->_nxt_node->_nxt_node->_data; /* skip a SENTINEL node */
	}
	return SUCCESS;
}

_int32 queue_pop(QUEUE *queue, void **node)
{
	_int32 ret_val = SUCCESS;
	*node = NULL;

	if (QINT_VALUE(queue->_queue_size) > 0)
	{
		QUEUE_NODE *free_node = (QUEUE_NODE*)queue->_queue_head->_nxt_node;
		*node = (void*)free_node->_nxt_node->_data; /* skip a SENTINEL node */
        free_node->_nxt_node->_data = NULL;
        
		if (QINT_VALUE(queue->_queue_size) > QINT_VALUE(queue->_queue_capacity) 
			|| QINT_VALUE(queue->_queue_actual_capacity) > QINT_VALUE(queue->_queue_capacity))
		{
			queue->_queue_head->_nxt_node = free_node->_nxt_node;

			/* free the extra node */
			ret_val = mpool_free_slip(gp_queueslab, free_node);
			CHECK_VALUE(ret_val);

			QINT_SUB(queue->_queue_actual_capacity, 1);
		}
		else
		{
			queue->_queue_head = free_node;
		}

		QINT_SUB(queue->_queue_size, 1);
	}

	return ret_val;
}

_int32 queue_push_without_alloc(QUEUE *queue, void *data)
{
	_int32 ret_val = SUCCESS;

	sd_assert(queue != NULL);

	if (QINT_VALUE(queue->_queue_size) >= QINT_VALUE(queue->_queue_actual_capacity))
	{
		return QUEUE_NO_ROOM;
	}

	queue->_queue_tail->_nxt_node->_data = data;
	queue->_queue_tail = queue->_queue_tail->_nxt_node;

	QINT_ADD(queue->_queue_size, 1);

	return ret_val;
}

_int32 queue_pop_without_dealloc(QUEUE *queue, void **data)
{
    _int32 ret_val = SUCCESS;
    *data = NULL;

    if (QINT_VALUE(queue->_queue_size) > 0)
	{
		QUEUE_NODE *free_node = (QUEUE_NODE*)queue->_queue_head->_nxt_node;
		*data = (void*)free_node->_nxt_node->_data; /* skip a SENTINEL node */
        free_node->_nxt_node->_data = NULL;
		queue->_queue_head = free_node;
		QINT_SUB(queue->_queue_size, 1);
	}

	return ret_val;
}

_int32 queue_recycle(QUEUE *queue)
{
	_int32 ret_val = SUCCESS;
	_int32 blow = MAX(QINT_VALUE(queue->_queue_size), QINT_VALUE(queue->_queue_capacity));
	_int32 uppper = QINT_VALUE(queue->_queue_actual_capacity);
	QUEUE_NODE *free_node = NULL;

	for(; blow < uppper; blow++)
	{
		free_node = (QUEUE_NODE*)queue->_queue_tail->_nxt_node;
		queue->_queue_tail->_nxt_node = free_node->_nxt_node;

		/* free this node */
		ret_val = mpool_free_slip(gp_queueslab, free_node);
		CHECK_VALUE(ret_val);

		QINT_SUB(queue->_queue_actual_capacity, 1);
	}

	return ret_val;
}

/* alloc new node to keep capacity */
_int32 queue_reserved(QUEUE *queue, _u32 capacity)
{
	_int32 ret_val = SUCCESS;
	_u32 cur_size = QINT_VALUE(queue->_queue_actual_capacity);
	QUEUE_NODE *free_node = NULL;

	if (capacity < MIN_QUEUE_CAPACITY)
	{
	    capacity = MIN_QUEUE_CAPACITY;
	}

	for(; cur_size < capacity; cur_size++)
	{
		/* allocate new node */
		ret_val = mpool_get_slip(gp_queueslab, (void**)&free_node);
		CHECK_VALUE(ret_val);

		/* init node */
		sd_memset(free_node, 0, sizeof(QUEUE_NODE));

		free_node->_nxt_node = queue->_queue_head->_nxt_node;
		queue->_queue_head->_nxt_node = free_node;

		queue->_queue_head = free_node;

		QINT_ADD(queue->_queue_actual_capacity, 1);
	}

	QINT_SET_VALUE_1(queue->_queue_capacity, capacity);

	return ret_val;
}

_int32 queue_check_full(QUEUE *queue)
{
	_int32 ret_val = SUCCESS;
	_u16 new_size = 0;

	if(QINT_VALUE(queue->_queue_actual_capacity) == 0 || QINT_VALUE(queue->_queue_size) >= QINT_VALUE(queue->_queue_actual_capacity) - 1)
	{
		queue->_empty_count = 0;

		if(queue->_full_count++ > QUEUE_AJUST_THRESHOLD)
		{
			new_size = QINT_VALUE(queue->_queue_actual_capacity) * QUEUE_ENLARGE_TIMES;
			if(new_size <= QINT_VALUE(queue->_queue_actual_capacity))
				new_size = QINT_VALUE(queue->_queue_actual_capacity) + 1;

			ret_val = queue_reserved(queue, new_size);
			CHECK_VALUE(ret_val);

			queue->_full_count = 0;
		}
	}
	else if(QINT_VALUE(queue->_queue_size) * QUEUE_REDUCE_TIMES < QINT_VALUE(queue->_queue_actual_capacity))
	{/* avoid the capacity always increased */
		queue->_full_count = 0;

		if(queue->_empty_count++ > QUEUE_AJUST_THRESHOLD)
		{
			new_size = QINT_VALUE(queue->_queue_actual_capacity) / QUEUE_REDUCE_TIMES;
                     if(new_size < MIN_QUEUE_CAPACITY)
                        new_size = MIN_QUEUE_CAPACITY;

			/* operation in read thread(pop) */
			QINT_SET_VALUE_2(queue->_queue_capacity, new_size);
			queue->_empty_count = 0;
		}
	}
	else
	{
		queue->_full_count = 0;
		queue->_empty_count = 0;
	}

	return ret_val;
}

_int32 queue_check_empty(QUEUE *queue)
{
	_int32 ret_val = SUCCESS;
	_u16 new_size = 0;

	if(QINT_VALUE(queue->_queue_size) * QUEUE_REDUCE_TIMES < QINT_VALUE(queue->_queue_actual_capacity))
	{
		if(queue->_empty_count++ > QUEUE_AJUST_THRESHOLD)
		{
			new_size = QINT_VALUE(queue->_queue_actual_capacity) / QUEUE_REDUCE_TIMES;

                     if(new_size < MIN_QUEUE_CAPACITY)
                        new_size = MIN_QUEUE_CAPACITY;

			/* operate in write thread(push) */
			QINT_SET_VALUE_1(queue->_queue_capacity, new_size);
			ret_val = queue_recycle(queue);
			CHECK_VALUE(ret_val);

			queue->_empty_count = 0;
		}
	}
	else
	{
		queue->_empty_count = 0;
	}

	return ret_val;
}
