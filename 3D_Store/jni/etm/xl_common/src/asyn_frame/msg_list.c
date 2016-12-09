#include "utility/errcode.h"
#include "asyn_frame/msg_list.h"
#include "asyn_frame/notice_queue.h"
#include "asyn_frame/notice.h"
#include "utility/mempool.h"
#include "utility/string.h"
#include "platform/sd_task.h"

#define LOGID LOGID_MSGLIST	
#include "utility/logger.h"

static NOTICE_QUEUE g_msg_notice_queue;

static QUEUE g_thread_msg_queue;

static SEVENT_HANDLE g_thread_msg_signal;

/* avoid more than one thread call queue_push */
static TASK_LOCK g_thread_msg_lock;

#define MIN_MSG_QUEUE_CAPATICY	(16)

_int32 msg_queue_init(_int32 *waitable_handle)
{
	_int32 ret_val = SUCCESS;
	ret_val = notice_queue_init(&g_msg_notice_queue, MIN_MSG_QUEUE_CAPATICY);
	CHECK_VALUE(ret_val);

	if (waitable_handle)
	{
	    *waitable_handle = g_msg_notice_queue._waitable_handle;
	}

	ret_val = queue_init(&g_thread_msg_queue, MIN_THREAD_MSG_COUNT);
	CHECK_VALUE(ret_val);

	/* pre-alloc node for thread_msg_queue */
	ret_val = queue_reserved(&g_thread_msg_queue, MIN_THREAD_MSG_COUNT);
	CHECK_VALUE(ret_val);

	ret_val = init_simple_event(&g_thread_msg_signal);
	CHECK_VALUE(ret_val);

	/* thread-msg */
	ret_val = sd_init_task_lock(&g_thread_msg_lock);
	CHECK_VALUE(ret_val);

	return ret_val;
}

_int32 msg_queue_uninit(void)
{
	_int32 ret_val = sd_uninit_task_lock(&g_thread_msg_lock);
	CHECK_VALUE(ret_val);

	ret_val = uninit_simple_event(&g_thread_msg_signal);
	CHECK_VALUE(ret_val);

	ret_val = queue_uninit(&g_thread_msg_queue);
	CHECK_VALUE(ret_val);

	ret_val = notice_queue_uninit(&g_msg_notice_queue);
	CHECK_VALUE(ret_val);

	return ret_val;
}

_int32 pop_msginfo_node(MSG **msg)
{
	return pop_notice_node(&g_msg_notice_queue, (void**)msg);
}

_int32 push_msginfo_node(MSG *msg)
{
	LOG_DEBUG("queue_capacity(msglist)   size:%d, capacity:%d, actual_capacity:%d",
		QINT_VALUE(g_msg_notice_queue._data_queue._queue_size), 
		QINT_VALUE(g_msg_notice_queue._data_queue._queue_capacity),
		QINT_VALUE(g_msg_notice_queue._data_queue._queue_actual_capacity));

	_int32 ret_val = push_notice_node(&g_msg_notice_queue, msg);
	CHECK_VALUE(ret_val);

	return ret_val;
}

_int32 pop_msginfo_node_from_other_thread(THREAD_MSG **msg)
{
	_int32 ret_val = SUCCESS;

	sd_task_lock(&g_thread_msg_lock);

	*msg = NULL;
	ret_val = queue_pop(&g_thread_msg_queue, (void**)msg);

	if(*msg)
	{
		/*check full since this queue push without alloc */
		ret_val = queue_check_full(&g_thread_msg_queue);
		if (SUCCESS!=ret_val)
		{
			sd_task_unlock(&g_thread_msg_lock);              
			sd_assert(FALSE);
			return ret_val;
		}


		/* signal empty */
		ret_val = signal_sevent_handle(&g_thread_msg_signal);
		if (SUCCESS!=ret_val)
		{
			sd_task_unlock(&g_thread_msg_lock);              
			sd_assert(FALSE);
			return ret_val;
		}


		/* pop success */
		ret_val = reset_notice(g_msg_notice_queue._waitable_handle);
	}
	else
	{
		ret_val = signal_sevent_handle(&g_thread_msg_signal);
		if (SUCCESS!=ret_val)
		{
			sd_task_unlock(&g_thread_msg_lock);              
			sd_assert(FALSE);
			return ret_val;
		}
	}
	sd_task_unlock(&g_thread_msg_lock);

	return ret_val;
}

_int32 push_msginfo_node_in_other_thread(THREAD_MSG *msg)
{
	_int32 ret_val = SUCCESS;

	sd_task_lock(&g_thread_msg_lock);

	while ((ret_val = queue_push_without_alloc(&g_thread_msg_queue, msg)) == QUEUE_NO_ROOM)
	{
		sd_task_unlock(&g_thread_msg_lock);
		wait_sevent_handle(&g_thread_msg_signal);
		sd_task_lock(&g_thread_msg_lock);
	}

	/* use the same notice-handle with normal_msg */
	ret_val = notice_impl(g_msg_notice_queue._notice_handle);

	sd_task_unlock(&g_thread_msg_lock);

	CHECK_VALUE(ret_val);

	return ret_val;
}


