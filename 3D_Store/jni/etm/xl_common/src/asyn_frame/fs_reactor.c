#include "asyn_frame/fs_reactor.h"
#include "asyn_frame/msg.h"
#include "asyn_frame/device_handler.h"
#include "platform/sd_task.h"
#include "platform/sd_fs.h"
#include "asyn_frame/device_reactor.h"

/* log */
#define LOGID LOGID_FS_REACTOR
#include "utility/logger.h"

#include "utility/thread.h"

#ifdef SET_PRIORITY
#include <sys/time.h>
#include <sys/resource.h>
#endif

static TASK_LOCK g_fs_thread_queue_lock;

static DEVICE_REACTOR g_fs_reactor;
static _int32 g_fs_waitable_container[FS_IN_QUEUE_COUNT];

static DEVICE_REACTOR g_fs_slow_op_reactor;
static _int32 g_fs_slow_op_waitable_container;

/* thread error handler */
static _int32 g_fs_reactor_error_exitcode = 0;
#define EXIT(code)	{g_fs_reactor_error_exitcode = code; goto ERROR_EXIT;}

#define FS_OP_THREAD_COUNT	(FS_IN_QUEUE_COUNT)

static THREAD_STATUS g_fs_reactor_thread_status[FS_OP_THREAD_COUNT + 1];
static _int32 g_fs_reactor_thread_id[FS_OP_THREAD_COUNT+1];

void fs_handler(void *args);

/**** some tools-function about thread-idx ***/
static DEVICE_REACTOR* get_fs_reactor_ptr(_int32 thread_idx)
{
	if (thread_idx == FS_OP_THREAD_COUNT)
	{
	    return &g_fs_slow_op_reactor;
	}
	else
	{
	    return &g_fs_reactor;
	}
}

static _int32 get_fs_waitable_container(_int32 thread_idx)
{
	if (thread_idx == FS_OP_THREAD_COUNT)
	{
	    return g_fs_slow_op_waitable_container;
	}
	else
	{
	    return g_fs_waitable_container[thread_idx];
	}
}

static _int32 lock_thread_queue(_int32 thread_idx)
{
	_int32 ret_val = SUCCESS;
	if (thread_idx < FS_OP_THREAD_COUNT)
	{
		ret_val = sd_task_lock(&g_fs_thread_queue_lock);
		CHECK_VALUE(ret_val);
	}
	return ret_val;
}

static _int32 unlock_thread_queue(_int32 thread_idx)
{
	_int32 ret_val = SUCCESS;
	if (thread_idx < FS_OP_THREAD_COUNT)
	{
		ret_val = sd_task_unlock(&g_fs_thread_queue_lock);
		CHECK_VALUE(ret_val);
	}
	return ret_val;
}

static _int32 pop_fs_msg(_int32 thread_idx, MSG **msg)
{
	if (thread_idx == FS_OP_THREAD_COUNT)
	{
	    return pop_register_event_with_lock(&g_fs_slow_op_reactor, msg);
	}
	else
	{
	    return pop_notice_node_without_dealloc(&(g_fs_reactor._fs_in_queue[thread_idx]), (void**)msg);
	}
}

/************************************************************************/

_int32 init_fs_reactor(_int32 *waitable_handle, _int32 *slow_op_waitable_handle)
{
	_int32 ret_val = device_reactor_init(&g_fs_reactor);
	CHECK_VALUE(ret_val);

	if (waitable_handle)
	{
	    *waitable_handle = g_fs_reactor._out_queue._waitable_handle;
	}

	ret_val = sd_init_task_lock(&g_fs_thread_queue_lock);
	CHECK_VALUE(ret_val);

    _int32 index = 0;
	for (index = 0; index < FS_IN_QUEUE_COUNT; index++)
	{
	    ret_val = create_waitable_container(&g_fs_waitable_container[index]);
	    CHECK_VALUE(ret_val);
	    ret_val = add_notice_handle(g_fs_waitable_container[index], g_fs_reactor._fs_in_queue[index]._waitable_handle);
	    CHECK_VALUE(ret_val);
	}
	
	/* start thread-pool */
	for (index = 0; index < FS_OP_THREAD_COUNT; index++)
	{
	    g_fs_reactor_thread_status[index] = INIT;
	    ret_val = sd_create_task(fs_handler, 32 * 1024, (void*)index, &g_fs_reactor_thread_id[index]);
	    CHECK_VALUE(ret_val);

	    /* wait for thread running */
	    while (g_fs_reactor_thread_status[index] == INIT)
	    {
	        sd_sleep(20);
	    }
	}

	/****** init slow-op reactor *******/
	ret_val = device_reactor_init(&g_fs_slow_op_reactor);
	CHECK_VALUE(ret_val);

    if (slow_op_waitable_handle)
    {
        *slow_op_waitable_handle = g_fs_slow_op_reactor._out_queue._waitable_handle;
    }

    ret_val = create_waitable_container(&g_fs_slow_op_waitable_container);
	CHECK_VALUE(ret_val);

	ret_val = add_notice_handle(g_fs_slow_op_waitable_container, g_fs_slow_op_reactor._in_queue._waitable_handle);
	CHECK_VALUE(ret_val);

	g_fs_reactor_thread_status[index] = INIT; 

	/* start a slow-op thread, index == FS_OP_THREAD_COUNT at this time */
	ret_val = sd_create_task(fs_handler, 32 * 1024, (void*)index, &g_fs_reactor_thread_id[index]);
	CHECK_VALUE(ret_val);

	/* wait for thread running */
	while (g_fs_reactor_thread_status[index] == INIT)
	{
	    sd_sleep(20);
	}

	return ret_val;
}

_int32 uninit_fs_reactor(void)
{
	_int32 ret_val = SUCCESS;

	/*stop all thread*/
	_int32 index = 0;
	for (; index < FS_OP_THREAD_COUNT; index++)
	{
		STOP_THREAD(g_fs_reactor_thread_status[index]);		
	}
	
	/* pop all remain msg */
	MSG *pmsg = NULL;
	do 
	{
		/* mem leak */
		ret_val = pop_complete_event(&g_fs_reactor, &pmsg);
		CHECK_VALUE(ret_val);
	} while(pmsg);

	for (index = 0; index < FS_OP_THREAD_COUNT; index++)
	{
		wait_thread(g_fs_reactor_thread_status + index, g_fs_reactor._fs_in_queue[index]._notice_handle);
		sd_finish_task(g_fs_reactor_thread_id[index]);
		g_fs_reactor_thread_id[index] = 0;
	}
	
	/* destory other variables */
	for (index = 0; index < FS_IN_QUEUE_COUNT; index++)
	{
		ret_val = destory_waitable_container(g_fs_waitable_container[index]);
		CHECK_VALUE(ret_val);
	}

	ret_val = sd_uninit_task_lock(&g_fs_thread_queue_lock);
	CHECK_VALUE(ret_val);

	ret_val = device_reactor_uninit(&g_fs_reactor);
	CHECK_VALUE(ret_val);


	/******* uninit slow-op reactor *********/

	/*stop slop-op thread*/
	STOP_THREAD(g_fs_reactor_thread_status[FS_OP_THREAD_COUNT]);

	/* pop all remain msg */
	do 
	{
		/* mem leak */
		ret_val = pop_complete_event(&g_fs_slow_op_reactor, &pmsg);
		CHECK_VALUE(ret_val);
	} while(pmsg);

	wait_thread(g_fs_reactor_thread_status + FS_OP_THREAD_COUNT, g_fs_slow_op_reactor._in_queue._notice_handle);

	sd_finish_task(g_fs_reactor_thread_id[FS_OP_THREAD_COUNT]);
	g_fs_reactor_thread_id[FS_OP_THREAD_COUNT] = 0;

	/* destory other variables */
	ret_val = destory_waitable_container(g_fs_slow_op_waitable_container);
	CHECK_VALUE(ret_val);

	ret_val = device_reactor_uninit(&g_fs_slow_op_reactor);
	CHECK_VALUE(ret_val);
	return ret_val;
}

_int32 register_fs_event(MSG *msg)
{
	_int32 ret_val = SUCCESS;

	if (is_a_pending_op(msg))
	{
		LOG_DEBUG("the fs-msg(id:%d) is a slow-op msg, so register to slow-op-reactor", msg->_msg_id);
		ret_val = register_event(&g_fs_slow_op_reactor, msg, (void*)&msg->_inner_data);
	}
	else
	{
		int thread_idx = msg->_msg_info._device_id % FS_IN_QUEUE_COUNT;
		LOG_DEBUG("the fs-msg(id:%d) is a normal fs-msg, register to fs-reactor", msg->_msg_id);
		ret_val = register_event_by_thread(&g_fs_reactor, msg, NULL, thread_idx);
	}

	return ret_val;
}

_int32 unregister_fs(MSG *msg, _int32 reason)
{
	_int32 ret_val = SUCCESS;

	if (is_a_pending_op(msg))
	{
		ret_val = unregister_event_immediately(&g_fs_reactor, msg, reason, (void**)msg->_inner_data);
	}
	else
	{
		/* change staus of msg */
		ret_val = check_unregister(msg);
		CHECK_VALUE(ret_val);

		/* change staus of msg */
		msg->_cur_reactor_status = (_u8)reason;
	}	

	return ret_val;
}

_int32 get_complete_fs_msg(MSG **msg)
{
	*msg = NULL;

	_int32 ret_val = pop_complete_event(&g_fs_slow_op_reactor, msg);
	CHECK_VALUE(ret_val);

//	LOG_DEBUG("get a slow complete fs-msg(id:%d)", (*msg) ? ((*msg)->_msg_id) : 0);

	if (NULL == *msg)
	{
		ret_val = pop_complete_event(&g_fs_reactor, msg);
		CHECK_VALUE(ret_val);
//		LOG_DEBUG("get a normal complete fs-msg(id:%d)", (*msg) ? ((*msg)->_msg_id) : 0);
	}

	return ret_val;
}

static _int32 fs_handler_run(_int32 thread_idx, DEVICE_REACTOR *fs_reactor_ptr, _int32 fs_waitable_container)
{
    _int32 noticed_handle = 0;
	_int32 ret_val = wait_for_notice(fs_waitable_container, 1, &noticed_handle, WAIT_INFINITE);
	CHECK_THREAD_VALUE(ret_val);

	ret_val = reset_notice(noticed_handle);
	CHECK_THREAD_VALUE(ret_val);

	ret_val = lock_thread_queue(thread_idx);
	CHECK_THREAD_VALUE(ret_val);
		
	/* get new msg */
	MSG *pmsg = NULL;
	ret_val = pop_fs_msg(thread_idx, &pmsg);
	CHECK_THREAD_VALUE(ret_val);

	ret_val = unlock_thread_queue(thread_idx);
	CHECK_THREAD_VALUE(ret_val);

	/* handle this msg */
	while (pmsg && IS_THREAD_RUN(g_fs_reactor_thread_status[thread_idx]))
	{
	    /* distinguish whether if open-file or enlarge-file diretly on op-open*/
	    BOOL completed = TRUE;
	    _int32 handle_errno = SUCCESS;
	    do
	    {
	        handle_errno = handle_msg(pmsg, &completed);
	    } while(!completed && REACTOR_REGISTERED(pmsg->_cur_reactor_status) && IS_THREAD_RUN(g_fs_reactor_thread_status[thread_idx]));


	    LOG_DEBUG("complete(or unregistered) a fs operation(%d) of fileid(%d) about msg(id:%d) of reactor-status(%d), ready to get lock to notice",
	        pmsg->_msg_info._operation_type, pmsg->_msg_info._device_id, pmsg->_msg_id, pmsg->_cur_reactor_status);

	    ret_val = lock_thread_queue(thread_idx);
	    CHECK_THREAD_VALUE(ret_val);

	    LOG_DEBUG("ready to notice about msg(id:%d)", pmsg->_msg_id);
	    /*ignore completed*/
	    ret_val = notice_complete_event(fs_reactor_ptr, (_int16)handle_errno, pmsg);
	    CHECK_THREAD_VALUE(ret_val);

	    LOG_DEBUG("ready to pop next msg since already complete to notice");
	    /* try to get new msg */
	    ret_val = pop_fs_msg(thread_idx, &pmsg);
	    CHECK_THREAD_VALUE(ret_val);

	    ret_val = unlock_thread_queue(thread_idx);
	    CHECK_THREAD_VALUE(ret_val);
	    /*  end of lock queue  */
	    LOG_DEBUG("free lock and ready to handle next msg(id:%d)", (pmsg?pmsg->_msg_id:0));
	}
    return ret_val;	
ERROR_EXIT:
	LOG_ERROR("ERROR OCCUR in fs-reactor thread(%u): %d\n", sd_get_self_taskid(), g_fs_reactor_error_exitcode);
	set_critical_error(g_fs_reactor_error_exitcode);
	finished_thread(g_fs_reactor_thread_status + thread_idx);
    return ret_val;
}

void fs_handler(void *args)
{
    sd_ignore_signal();
#ifdef SET_PRIORITY
    setpriority(PRIO_PROCESS, 0, 5);
#endif

    _int32 thread_idx = (_int32)args;
    DEVICE_REACTOR *fs_reactor_ptr = get_fs_reactor_ptr(thread_idx);
	_int32 fs_waitable_container = get_fs_waitable_container(thread_idx);

    LOG_DEBUG("fs_handler, begin, index=%d.", thread_idx);
    RUN_THREAD(g_fs_reactor_thread_status[thread_idx]);
    BEGIN_THREAD_LOOP(g_fs_reactor_thread_status[thread_idx])
    fs_handler_run(thread_idx, fs_reactor_ptr, fs_waitable_container);
    END_THREAD_LOOP(g_fs_reactor_thread_status[thread_idx]);

    finished_thread(g_fs_reactor_thread_status + thread_idx);
    LOG_DEBUG("fs reactor handler(%u) exit normally", sd_get_self_taskid());
}
