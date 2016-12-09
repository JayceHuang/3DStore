#include "em_common/em_errcode.h"
#include "em_asyn_frame/em_msg_list.h"
#include "em_asyn_frame/em_notice_queue.h"
#include "em_asyn_frame/em_notice.h"
#include "em_common/em_mempool.h"
#include "em_common/em_string.h"
#include "em_interface/em_task.h"
#if 0 //def _ANDROID_LINUX
#include "android_interface/sd_android_utility.h"
#endif /* _ANDROID_LINUX */

#define EM_LOGID EM_LOGID_MSGLIST	
#include "em_common/em_logger.h"

static NOTICE_QUEUE g_em_msg_queue;

static QUEUE g_em_thread_msg_queue;

#ifndef _MSG_POLL
static SEVENT_HANDLE g_em_thread_msg_signal;
#endif

/* avoid more than one thread call queue_push */
static TASK_LOCK g_em_thread_msg_lock;

#define MIN_MSG_QUEUE_CAPATICY	(16)

_int32 em_msg_queue_init(_int32 *waitable_handle)
{
	_int32 ret_val = SUCCESS;
	ret_val = em_notice_queue_init(&g_em_msg_queue, MIN_MSG_QUEUE_CAPATICY);
	CHECK_VALUE(ret_val);

	if(waitable_handle)
		*waitable_handle = g_em_msg_queue._waitable_handle;

	ret_val = em_queue_init(&g_em_thread_msg_queue, MIN_THREAD_MSG_COUNT);
	CHECK_VALUE(ret_val);

	/* pre-alloc node for thread_msg_queue */
	ret_val = em_queue_reserved(&g_em_thread_msg_queue, MIN_THREAD_MSG_COUNT);
	CHECK_VALUE(ret_val);

#ifndef _MSG_POLL
	ret_val = em_init_simple_event(&g_em_thread_msg_signal);
	CHECK_VALUE(ret_val);
#endif

	/* thread-msg */
	ret_val = em_init_task_lock(&g_em_thread_msg_lock);
	CHECK_VALUE(ret_val);

	return ret_val;
}

_int32 em_msg_queue_uninit(void)
{
	_int32 ret_val = SUCCESS;

	ret_val = em_uninit_task_lock(&g_em_thread_msg_lock);
	CHECK_VALUE(ret_val);

#ifndef _MSG_POLL
	ret_val = em_uninit_simple_event(&g_em_thread_msg_signal);
	CHECK_VALUE(ret_val);
#endif

	ret_val = em_queue_uninit(&g_em_thread_msg_queue);
	CHECK_VALUE(ret_val);

	ret_val = em_notice_queue_uninit(&g_em_msg_queue);
	CHECK_VALUE(ret_val);

	return ret_val;
}

_int32 em_pop_msginfo_node(MSG **msg)
{
	return em_pop_notice_node(&g_em_msg_queue, (void**)msg);
}

_int32 em_push_msginfo_node(MSG *msg)
{
	_int32 ret_val=SUCCESS;
	EM_LOG_DEBUG("queue_capacity(msglist)   size:%d, capacity:%d, actual_capacity:%d",
		QINT_VALUE(g_em_msg_queue._data_queue._queue_size), 
		QINT_VALUE(g_em_msg_queue._data_queue._queue_capacity),
		QINT_VALUE(g_em_msg_queue._data_queue._queue_actual_capacity));

	ret_val= em_push_notice_node(&g_em_msg_queue, msg);
	CHECK_VALUE(ret_val);
#if defined( __SYMBIAN32__)
	em_symbian_scheduler();
#endif
	return ret_val;
}

_int32 em_pop_msginfo_node_from_other_thread(THREAD_MSG **msg)
{
	_int32 ret_val = SUCCESS;

	*msg = NULL;
	em_task_lock(&g_em_thread_msg_lock);
	ret_val = em_queue_pop(&g_em_thread_msg_queue, (void**)msg);

	if(*msg)
	{
		#if 0 //def _ANDROID_LINUX
		if(sd_android_utility_is_xiaomi())
		{
			//EM_LOG_URGENT("pop:*msg=0x%X,handle=0x%X,param=0x%X!", *msg,(*msg)->_handle,(*msg)->_param);
			sd_sleep(1);
			sd_assert((*msg)->_handle>1000);
			sd_assert((*msg)->_param>1000);
		}
		#endif /* _ANDROID_LINUX  */
		/*check full since this queue push without alloc */
		ret_val = em_queue_check_full(&g_em_thread_msg_queue);
		CHECK_VALUE(ret_val);

#ifndef _MSG_POLL
		/* signal empty */
		ret_val = em_signal_sevent_handle(&g_em_thread_msg_signal);
		CHECK_VALUE(ret_val);
#endif

		/* pop success */
		ret_val = em_reset_notice(g_em_msg_queue._waitable_handle);
	}
	em_task_unlock(&g_em_thread_msg_lock);

	return ret_val;
}

_int32 em_push_msginfo_node_in_other_thread(THREAD_MSG *msg)
{
	_int32 ret_val = SUCCESS;

	em_task_lock(&g_em_thread_msg_lock);

	while((ret_val = em_queue_push_without_alloc(&g_em_thread_msg_queue, msg)) == QUEUE_NO_ROOM)
	{
		/* wait for room */
#ifdef _MSG_POLL
		em_wait_instance();
#else
		em_wait_sevent_handle(&g_em_thread_msg_signal);
#endif
	}

	/* use the same notice-handle with normal_msg */
	ret_val = em_notice(g_em_msg_queue._notice_handle);

	em_task_unlock(&g_em_thread_msg_lock);

	CHECK_VALUE(ret_val);

	return ret_val;
}

BOOL em_msg_has_new_msg(void)
{
       if(em_queue_size(&g_em_msg_queue._data_queue) > 0
	   	|| em_queue_size(&g_em_thread_msg_queue) > 0)
       {
             return TRUE;
       }
	else
	{
	       return FALSE;
	}
}


