#include "asyn_frame/notice_queue.h"
#include "utility/errcode.h"

_int32 notice_queue_init(NOTICE_QUEUE *queue, _int32 min_queue_count)
{
	_int32 ret_val = queue_init(&queue->_data_queue, min_queue_count);
	CHECK_VALUE(ret_val);

	ret_val = create_notice_handle(&queue->_notice_handle, &queue->_waitable_handle);
	CHECK_VALUE(ret_val);

	return ret_val;
}

_int32 notice_queue_uninit(NOTICE_QUEUE *queue)
{
	_int32 ret_val = destory_notice_handle(queue->_notice_handle, queue->_waitable_handle);
	CHECK_VALUE(ret_val);

	ret_val = queue_uninit(&queue->_data_queue);
	CHECK_VALUE(ret_val);

	return ret_val;
}

_int32 pop_notice_node(NOTICE_QUEUE *queue, void **data)
{
    *data = NULL;
	_int32 ret_val = queue_pop(&queue->_data_queue, data);
	CHECK_VALUE(ret_val);

	if (NULL != *data)/* pop success */
	{
		ret_val = reset_notice(queue->_waitable_handle);
		CHECK_VALUE(ret_val);
	}

	return ret_val;
}

_int32 push_notice_node(NOTICE_QUEUE *queue, void *data)
{
	_int32 ret_val = queue_push(&queue->_data_queue, data);
	CHECK_VALUE(ret_val);

	ret_val = notice_impl(queue->_notice_handle);
	CHECK_VALUE(ret_val);

	return ret_val;
}

_int32 pop_notice_node_without_dealloc(NOTICE_QUEUE *queue, void **data)
{
    *data = NULL;
	_int32 ret_val = queue_pop_without_dealloc(&queue->_data_queue, data);
	CHECK_VALUE(ret_val);

	if (*data)/* pop success */
	{
		ret_val = reset_notice(queue->_waitable_handle);
		CHECK_VALUE(ret_val);
	}

	return ret_val;
}

_int32 push_notice_node_without_alloc(NOTICE_QUEUE *queue, void *data)
{
	_int32 ret_val = queue_push_without_alloc(&queue->_data_queue, data);

	if (ret_val != QUEUE_NO_ROOM)
	{
		CHECK_VALUE(ret_val);

		ret_val = notice_impl(queue->_notice_handle);
		CHECK_VALUE(ret_val);
	}

	return ret_val;
}
