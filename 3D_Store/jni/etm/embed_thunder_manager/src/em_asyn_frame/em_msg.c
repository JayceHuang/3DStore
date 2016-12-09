#include "em_asyn_frame/em_msg.h"
#include "em_asyn_frame/em_msg_list.h"
#include "em_asyn_frame/em_device.h"
#include "em_asyn_frame/em_msg_alloctor.h"
#include "em_common/em_string.h"
#include "em_common/em_errcode.h"
#include "em_common/em_mempool.h"
#ifdef _ANDROID_LINUX
#include "platform/android_interface/sd_android_utility.h"
#endif /* _ANDROID_LINUX */


#define EM_LOGID EM_LOGID_ASYN_FRAME
#include "em_common/em_logger.h"


static _u32 g_em_msgid_generator = INVALID_MSG_ID;
static SEVENT_HANDLE * gp_em_handle=NULL;
static TASK_LOCK * p_post_lock = NULL;
static _int32 * gp_em_result = NULL;
#if defined(MACOS)&&(!defined(_MSG_POLL))
static SEVENT_HANDLE g_em_handle;
#endif

#define EM_ALLOC_NEW_MSGID	(++g_em_msgid_generator == INVALID_MSG_ID ? ++g_em_msgid_generator : g_em_msgid_generator)

_int32 em_init_post_msg(void)
{
	_int32 ret_val =	em_malloc(sizeof(TASK_LOCK),(void**)&p_post_lock);
	CHECK_VALUE(ret_val);
	em_memset(p_post_lock,0x00,sizeof(TASK_LOCK));
	ret_val =	em_init_task_lock(p_post_lock);
	if(ret_val!=SUCCESS)
	{
		EM_SAFE_DELETE(p_post_lock);
	}
#if defined(MACOS)&&(!defined(_MSG_POLL))
	ret_val = em_init_simple_event(&g_em_handle);
	if(ret_val!=SUCCESS)
	{
		em_uninit_task_lock(p_post_lock);
		EM_SAFE_DELETE(p_post_lock);
	}
#endif
	return ret_val;
}
_int32 em_uninit_post_msg(void)
{
#if defined(MACOS)&&(!defined(_MSG_POLL))
	em_uninit_simple_event(&g_em_handle);
#endif
	em_uninit_task_lock(p_post_lock);
	EM_SAFE_DELETE(p_post_lock);
	return SUCCESS;
}

_int32 em_post_message(const MSG_INFO *msg_info, msg_handler handler, _int16 notice_count, _u32 timeout, _u32 *msgid)
{
	_int32 ret_val = SUCCESS;
	MSG *pmsg = NULL;

	/* check paramter */
	if(!handler)
	{
		return INVALID_MSG_HANDLER;
	}
	if(!VALIDATE_DEVICE(msg_info->_device_type, msg_info->_operation_type))
	{
		return INVALID_OPERATION_TYPE;
	}

	ret_val = em_msg_alloc(&pmsg);
	CHECK_VALUE(ret_val);

	/* init msg */
	em_memset(pmsg, 0, sizeof(MSG));

	pmsg->_handler = handler;
	pmsg->_notice_count_left = notice_count;
	pmsg->_timeout = timeout;
	ret_val = em_memcpy(&pmsg->_msg_info, msg_info, sizeof(MSG_INFO));
	pmsg->_msg_info._pending_op_count = 0;
	pmsg->_msg_cannelled = 0;

	CHECK_VALUE(ret_val);

	ret_val = em_alloc_and_copy_para(&pmsg->_msg_info, msg_info);
	CHECK_VALUE(ret_val);

	/* allocate msg id */
	pmsg->_msg_id = EM_ALLOC_NEW_MSGID;

	if(msgid)
		*msgid = pmsg->_msg_id;

	EM_LOG_DEBUG("ready to put msg(id:%d) to in-queue:(handler: 0x%x, notice_count:%d, timeout: %u",
		pmsg->_msg_id, handler, notice_count, timeout);

	ret_val = em_push_msginfo_node(pmsg);
	CHECK_VALUE(ret_val);

	return ret_val;
}

_int32 em_post_message_from_other_thread(thread_msg_handle handler, void *args)
{
	_int32 ret_val = SUCCESS;
	THREAD_MSG *pmsg = NULL;

	/* check paramter */
	if(!handler)
	{
		return INVALID_MSG_HANDLER;
	}

	ret_val = em_msg_thread_alloc(&pmsg);
	CHECK_VALUE(ret_val);

	pmsg->_handle = handler;
	pmsg->_param = args;
	
	#if 0 //def _ANDROID_LINUX
	if(sd_android_utility_is_xiaomi())
	{
		//EM_LOG_URGENT("post:pnewmsg=0x%X,handle=0x%X,param=0x%X!", pmsg,pmsg->_handle,pmsg->_param);
		sd_sleep(1);
		sd_assert(pmsg->_handle>1000);
		sd_assert(pmsg->_param>1000);
	}
	#endif /* _ANDROID_LINUX */
	ret_val = em_push_msginfo_node_in_other_thread(pmsg);
	CHECK_VALUE(ret_val);

	return ret_val;
}

_int32 em_cancel_message_by_msgid(_u32 msg_id)
{
	_int32 ret_val = SUCCESS;
	MSG *pmsg = NULL;

	ret_val = em_msg_alloc(&pmsg);
	CHECK_VALUE(ret_val);

	/* init msg */
	em_memset(pmsg, 0, sizeof(MSG));

	pmsg->_handler = (msg_handler)CANCEL_MSG_BY_MSGID;
	pmsg->_msg_id = msg_id;

	EM_LOG_DEBUG("ready to put a cancel-msg about msg(id:%d) to in-queue", pmsg->_msg_id);
	ret_val = em_push_msginfo_node(pmsg);
	CHECK_VALUE(ret_val);

	return ret_val;
}

_int32 em_cancel_message_by_device_id(_u32 device_id, _u32 device_type)
{
	_int32 ret_val = SUCCESS;
	return ret_val;
}

_int32 em_post_function(thread_msg_handle handler, void *args,SEVENT_HANDLE * handle,_int32 * result)
{
	int ret_val = 0;

	em_task_lock(p_post_lock);
	
#if defined( __SYMBIAN32__)
	gp_em_handle = handle;
	ret_val=handler(args);
	CHECK_VALUE(ret_val);
#else
	if(gp_em_handle!=NULL)
	{
		EM_LOG_ERROR("WARNNING:em_post_function:ETM_BUSY:0x%X!", gp_em_handle);
		CHECK_VALUE( ETM_BUSY);
	}

#if defined(MACOS)&&(!defined(_MSG_POLL))
	sd_assert(handle->_value == 0);
	em_memcpy(handle,&g_em_handle,sizeof(SEVENT_HANDLE));
#else
	ret_val = em_init_simple_event(handle);
	CHECK_VALUE(ret_val);
#endif
	gp_em_handle = handle;
	gp_em_result = result;
	
	ret_val = em_post_message_from_other_thread(handler, args);
	CHECK_VALUE(ret_val);

	ret_val = em_wait_sevent_handle(handle);
	CHECK_VALUE(ret_val);
	
#if defined(MACOS)&&(!defined(_MSG_POLL))
	//sd_assert(handle->_value == 0);
	em_memset(handle,0x00,sizeof(SEVENT_HANDLE));
#else
	ret_val = em_uninit_simple_event(handle);
	CHECK_VALUE(ret_val);
#endif
#endif
	gp_em_handle = NULL;
	gp_em_result = NULL;
	em_task_unlock(p_post_lock);
	
	return (*result);
}
_int32 em_post_function_unlock(thread_msg_handle handler, void *args,SEVENT_HANDLE * handle,_int32 * result)
{
	int ret_val = 0;
	
#if defined( __SYMBIAN32__)
	ret_val = handler(args);
#else
	ret_val = em_post_message_from_other_thread(handler, args);
#endif
	CHECK_VALUE(ret_val);
	return SUCCESS;
}
_int32 em_force_signal_sevent(void)
{
	EM_LOG_URGENT("ETM_BROKEN_SIGNALR:em_force_signal_sevent:gp_em_handle=0x%X,gp_em_result=0x%X!", gp_em_handle,gp_em_result);
	if(gp_em_result)
	{
		*gp_em_result = ETM_BROKEN_SIGNAL;
	}
	
	if(gp_em_handle)
	{
		return signal_sevent_handle(gp_em_handle);
	}
	else
	{
		return SUCCESS;
	}
}
_int32 em_start_timer(msg_handler handler, _int16 notice_count, _u32 timeout, _u32 user_data1, void *user_data2, _u32 *timer_handle)
{
	_int32 ret_val = SUCCESS;
	MSG_INFO msg_info;

	msg_info._device_id = user_data1;
	msg_info._device_type = DEVICE_COMMON;
	msg_info._msg_priority = MSG_PRIORITY_NORMAL;
	msg_info._operation_parameter = NULL;
	msg_info._operation_type = OP_COMMON;
	msg_info._pending_op_count = 0;
	msg_info._user_data = user_data2;

	ret_val = em_post_message(&msg_info, handler, notice_count, timeout, timer_handle);

	return ret_val;
}

_int32 em_cancel_timer(_u32 timer_handle)
{
	return em_cancel_message_by_msgid(timer_handle);
}
