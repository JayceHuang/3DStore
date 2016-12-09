#include "asyn_frame/device_reactor.h"
#define LOGID LOGID_ASYN_FRAME
#include "utility/logger.h"

_int32 device_reactor_init(DEVICE_REACTOR *reactor)
{
    _int32 ret_val = notice_queue_init(&reactor->_in_queue, MIN_REACTOR_QUEUE_SIZE);
    CHECK_VALUE(ret_val);
    
    ret_val = notice_queue_init(&reactor->_out_queue, MIN_REACTOR_QUEUE_SIZE);
    CHECK_VALUE(ret_val);
    
    int i = 0;
    for (; i < FS_IN_QUEUE_COUNT; i++)
	{
		ret_val = notice_queue_init(&reactor->_fs_in_queue[i], MIN_REACTOR_QUEUE_SIZE);
		CHECK_VALUE(ret_val);
	}

	/* pre-alloc node for _out_queue */
	ret_val = queue_reserved(&reactor->_out_queue._data_queue, MIN_REACTOR_QUEUE_SIZE);
	CHECK_VALUE(ret_val);

	ret_val = sd_init_task_lock(&reactor->_in_queue_lock);
	CHECK_VALUE(ret_val);

	list_init(&reactor->_abortive_msg);

	ret_val = init_simple_event(&reactor->_out_queue_signal);
	CHECK_VALUE(ret_val);

	return ret_val;
}

_int32 device_reactor_uninit(DEVICE_REACTOR *reactor)
{
	_int32 ret_val = uninit_simple_event(&reactor->_out_queue_signal);
	CHECK_VALUE(ret_val);

	ret_val = sd_uninit_task_lock(&reactor->_in_queue_lock);
	CHECK_VALUE(ret_val);

	ret_val = notice_queue_uninit(&reactor->_in_queue);
	CHECK_VALUE(ret_val);

    int i = 0;
	for (; i < FS_IN_QUEUE_COUNT; ++i)
	{
	    notice_queue_uninit(&reactor->_fs_in_queue[i]);
	}

	ret_val = notice_queue_uninit(&reactor->_out_queue);
	CHECK_VALUE(ret_val);

	return ret_val;
}

/* check status.
 * Before callback-funtion was called, the operation of cancel maybe not complete.
 * And only ONE register-operation is allowed simultaneously.
 */
_int32 check_register(MSG *msg)
{
	if (!REACTOR_READY(msg->_cur_reactor_status) || (msg->_op_count > 0))
	{
		return REACTOR_LOGIC_ERROR;
	}
	return SUCCESS;
}

_int32 check_unregister(MSG *msg)
{
	if (!REACTOR_REGISTERED(msg->_cur_reactor_status))
	{
		return REACTOR_LOGIC_ERROR;
	}
	return SUCCESS;
}

static _int32 register_event_to_queue(MSG *msg, void **msg_pos_ptr, NOTICE_QUEUE *queue)
{
    _int32 ret = check_register(msg);
    CHECK_VALUE(ret);

    /* change staus of msg*/
    msg->_cur_reactor_status = REACTOR_STATUS_REGISTERING;
    
    msg->_op_count ++;
    ret = push_notice_node(queue, msg);
    /* when failed to push queue(NOT failed to notice), reset the status. */
    if (SUCCESS != ret)
    {
        msg->_op_count --;
        return ret;
    }

    if (NULL != msg_pos_ptr)
    {
        ret = queue_get_tail_ptr(&(queue->_data_queue), msg_pos_ptr);
        CHECK_VALUE(ret);
    }

    /*check empty since this queue pop without dealloc */
	ret = queue_check_empty(&(queue->_data_queue));
	CHECK_VALUE(ret);

	return ret;
}

_int32 register_event(DEVICE_REACTOR *reactor, MSG *msg, void **msg_pos_ptr)
{
    return register_event_to_queue(msg, msg_pos_ptr, &(reactor->_in_queue));
}

_int32 register_event_by_thread(DEVICE_REACTOR *reactor, MSG *msg, void **msg_pos_ptr,  int thread_idx)
{
    return register_event_to_queue(msg, msg_pos_ptr, &(reactor->_fs_in_queue[thread_idx]));
}

_int32 unregister_event(DEVICE_REACTOR *reactor, MSG *msg, _int32 reason)
{
	_int32 ret_val = check_unregister(msg);
	CHECK_VALUE(ret_val);

	/* change staus of msg */
	msg->_cur_reactor_status = (_u8)reason;
	msg->_op_count ++;

	ret_val = push_notice_node(&reactor->_in_queue, msg);
	/* when failed to push queue(NOT failed to notice), reset the status. */
	if (ret_val != SUCCESS)
	{
		msg->_op_count --;
		return ret_val;
	}

	/*check empty since this queue pop without dealloc */
	ret_val = queue_check_empty(&reactor->_in_queue._data_queue);
	CHECK_VALUE(ret_val);

	return ret_val;
}

_int32 unregister_event_immediately(DEVICE_REACTOR *reactor, MSG *msg, _int32 reason, void **msg_pos_ptr)
{
	_int32 ret_val = check_unregister(msg);
	CHECK_VALUE(ret_val);

	if (msg->_cur_reactor_status == REACTOR_STATUS_REGISTERING)
	{
		/* can remove this lock ??? */
		ret_val = sd_task_lock(&reactor->_in_queue_lock);
		CHECK_VALUE(ret_val);

		if (msg->_cur_reactor_status == REACTOR_STATUS_REGISTERING)
		{
			sd_assert(*msg_pos_ptr == msg);
			*msg_pos_ptr = (void*)INVALID_REACTOR_MSG;
			list_push(&reactor->_abortive_msg, msg);
		}

		ret_val = sd_task_unlock(&reactor->_in_queue_lock);
		CHECK_VALUE(ret_val);
	}

	/* change staus of msg */
	msg->_cur_reactor_status = (_u8)reason;

	return ret_val;
}

_int32 pop_complete_event(DEVICE_REACTOR *reactor, MSG **msg)
{
	*msg = NULL;
	_int32 ret_val = list_pop(&reactor->_abortive_msg, (void**)msg);
	CHECK_VALUE(ret_val);

	if (NULL == (*msg))
	{
	    ret_val = pop_notice_node(&reactor->_out_queue, (void**)msg);
	    CHECK_VALUE(ret_val);
	}

/* NOTICE:  
 *  reset this status in asyn_frame.c
 *	(*msg)->_cur_reactor_status = REACTOR_STATUS_READY; 
 */
	if (*msg)
	{
		(*msg)->_op_count --;

		/*check full since this queue push without alloc */
		ret_val = queue_check_full(&reactor->_out_queue._data_queue);
		CHECK_VALUE(ret_val);

		/* signal empty */
		ret_val = signal_sevent_handle(&reactor->_out_queue_signal);
		CHECK_VALUE(ret_val);
	}

	return ret_val;
}

_int32 notice_complete_event(DEVICE_REACTOR *reactor, _int16 op_errcode, MSG *msg)
{
	_int32 ret_val = SUCCESS;

	msg->_op_errcode = op_errcode;

	while((ret_val = push_notice_node_without_alloc(&reactor->_out_queue, msg)) == QUEUE_NO_ROOM)
	{
		/* wait for room */
		wait_sevent_handle(&reactor->_out_queue_signal);
	}

	LOG_DEBUG("notice_complete_event: msg(id:%d) op(%d) of device_id(%d) ,op_errcode=%d ", 
	    msg->_msg_id,msg->_msg_info._operation_type, msg->_msg_info._device_id,op_errcode);
	return ret_val;
}

_int32 pop_register_event(DEVICE_REACTOR *reactor, MSG **msg)
{
	_int32 ret_val = pop_notice_node_without_dealloc(&reactor->_in_queue, (void**)msg);
	CHECK_VALUE(ret_val);
	return ret_val;
}

_int32 pop_register_event_with_lock(DEVICE_REACTOR *reactor, MSG **msg)
{
	_int32 ret_val = SUCCESS;
	BOOL retry_pop = FALSE;

	do
	{
		retry_pop = FALSE;

		ret_val = sd_task_lock(&reactor->_in_queue_lock);
		CHECK_VALUE(ret_val);

		ret_val = pop_notice_node_without_dealloc(&reactor->_in_queue, (void**)msg);
		CHECK_VALUE(ret_val);

		if(*msg)
		{
			if((*msg) == (void*)INVALID_REACTOR_MSG)
				retry_pop = TRUE;
			else
			{
				/* assert now, will improved in future*/
				sd_assert((*msg)->_cur_reactor_status == REACTOR_STATUS_REGISTERING);


				if((*msg)->_cur_reactor_status == REACTOR_STATUS_REGISTERING)
					(*msg)->_cur_reactor_status = REACTOR_STATUS_REGISTERED;
			}
		}

		ret_val = sd_task_unlock(&reactor->_in_queue_lock);
		CHECK_VALUE(ret_val);

	} while(retry_pop);

	return ret_val;
}
