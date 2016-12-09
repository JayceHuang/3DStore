#include "asyn_frame/msg.h"
#include "asyn_frame/msg_list.h"
#include "asyn_frame/device.h"
#include "asyn_frame/msg_alloctor.h"
#include "utility/string.h"
#include "utility/errcode.h"
#include "utility/mempool.h"
#include "utility/logid.h"
#define LOGID LOGID_ASYN_FRAME
#include "utility/logger.h"


static _u32 g_msgid_generator = INVALID_MSG_ID;

#define ALLOC_NEW_MSGID	(++g_msgid_generator == INVALID_MSG_ID ? ++g_msgid_generator : g_msgid_generator)

_int32 init_post_msg(TASK_LOCK **task_lock)
{
    LOG_DEBUG("in task_lock : 0x%x", *task_lock);
    sd_assert(NULL == *task_lock);
    _int32 ret_val = sd_malloc(sizeof(TASK_LOCK), (void**)task_lock);
	CHECK_VALUE(ret_val);
	sd_memset(*task_lock, 0, sizeof(TASK_LOCK));	
	ret_val = sd_init_task_lock(*task_lock);
	if (ret_val != SUCCESS)
	{
		SAFE_DELETE(*task_lock);
	}
    LOG_DEBUG("out task_lock : 0x%x", *task_lock);

	return ret_val;
}

_int32 uninit_post_msg(TASK_LOCK **task_lock)
{
    LOG_DEBUG("in task_lock : 0x%x", *task_lock);
	sd_uninit_task_lock(*task_lock);
	SAFE_DELETE(*task_lock);
    LOG_DEBUG("out task_lock : 0x%x", *task_lock);
    
	return SUCCESS;
}

_int32 post_message(const MSG_INFO *msg_info, msg_handler handler, _int16 notice_count, _u32 timeout, _u32 *msgid)
{
	/* check paramter */
	if(!handler)
	{
		return INVALID_MSG_HANDLER;
	}
	if(!VALIDATE_DEVICE(msg_info->_device_type, msg_info->_operation_type))
	{
		return INVALID_OPERATION_TYPE;
	}

	MSG *pmsg = NULL;
	_int32 ret_val = msg_alloc(&pmsg);
	CHECK_VALUE(ret_val);
	sd_memset(pmsg, 0, sizeof(MSG));

	pmsg->_handler = handler;
	pmsg->_notice_count_left = notice_count;
	pmsg->_timeout = timeout;
	ret_val = sd_memcpy(&pmsg->_msg_info, msg_info, sizeof(MSG_INFO));
	pmsg->_msg_info._pending_op_count = 0;
	pmsg->_msg_cannelled = 0;

	CHECK_VALUE(ret_val);

	ret_val = alloc_and_copy_para(&pmsg->_msg_info, msg_info);
	CHECK_VALUE(ret_val);

	/* allocate msg id */
	pmsg->_msg_id = ALLOC_NEW_MSGID;

	if (msgid)
	{
	    *msgid = pmsg->_msg_id;
	}

	LOG_DEBUG("ready to put msg(id:%d) to in-queue:(handler: 0x%x, notice_count:%d, timeout: %u",
		pmsg->_msg_id, handler, notice_count, timeout);

	ret_val = push_msginfo_node(pmsg);
	CHECK_VALUE(ret_val);

	return ret_val;
}

_int32 post_message_from_other_thread(thread_msg_handle handler, void *args)
{
	if (!handler)
	{
		return INVALID_MSG_HANDLER;
	}

    THREAD_MSG *pmsg = NULL;
	_int32 ret_val = msg_thread_alloc(&pmsg);
	CHECK_VALUE(ret_val);

	pmsg->_handle = handler;
	pmsg->_param = args;

	ret_val = push_msginfo_node_in_other_thread(pmsg);
	CHECK_VALUE(ret_val);

	return ret_val;
}

_int32 cancel_message_by_msgid(_u32 msg_id)
{
	MSG *pmsg = NULL;
	_int32 ret_val = msg_alloc(&pmsg);
	CHECK_VALUE(ret_val);
	sd_memset(pmsg, 0, sizeof(MSG));

	pmsg->_handler = (msg_handler)CANCEL_MSG_BY_MSGID;
	pmsg->_msg_id = msg_id;

	LOG_DEBUG("ready to put a cancel-msg about msg(id:%d) to in-queue", pmsg->_msg_id);
	ret_val = push_msginfo_node(pmsg);
	CHECK_VALUE(ret_val);

	return ret_val;
}

_int32 cancel_message_by_device_id(_u32 device_id, _u32 device_type)
{
	MSG *pmsg = NULL;
	_int32 ret_val = msg_alloc(&pmsg);
	CHECK_VALUE(ret_val);
	sd_memset(pmsg, 0, sizeof(MSG));

	pmsg->_handler = (msg_handler)CANCEL_MSG_BY_DEVICEID;
	pmsg->_msg_id = INVALID_MSG_ID;
	pmsg->_msg_info._device_id = device_id;
	pmsg->_msg_info._device_type = device_type;

	LOG_DEBUG("ready to put a cancel-msg of socket id(%d) to in-queue", pmsg->_msg_info._device_id);
	ret_val = push_msginfo_node(pmsg);
	CHECK_VALUE(ret_val);

	return ret_val;
}

_int32 start_timer(msg_handler handler, _int16 notice_count, _u32 timeout, _u32 user_data1, void *user_data2, _u32 *timer_handle)
{
	MSG_INFO msg_info = {};
	msg_info._device_id = user_data1;
	msg_info._device_type = DEVICE_COMMON;
	msg_info._msg_priority = MSG_PRIORITY_NORMAL;
	msg_info._operation_parameter = NULL;
	msg_info._operation_type = OP_COMMON;
	msg_info._pending_op_count = 0;
	msg_info._user_data = user_data2;

	return post_message(&msg_info, handler, notice_count, timeout, timer_handle);
}

_int32 cancel_timer(_u32 timer_handle)
{
	return cancel_message_by_msgid(timer_handle);
}
