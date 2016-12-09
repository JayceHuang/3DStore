#ifndef _EM_MSG_H_00138F8F2E70_200806051401
#define _EM_MSG_H_00138F8F2E70_200806051401

#ifdef __cplusplus
extern "C" 
{
#endif

#include "em_common/em_define.h"
#include "em_asyn_frame/em_notice.h"

#include "asyn_frame/msg.h"


typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32	_result;
}POST_PARA_0;

typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32	_result;
	void *	_para1;
}POST_PARA_1;

typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32	_result;
	void *	_para1;
	void *	_para2;
}POST_PARA_2;

typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32	_result;
	void *	_para1;
	void *	_para2;
	void *	_para3;
}POST_PARA_3;
typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32	_result;
	void *	_para1;
	void *	_para2;
	void *	_para3;
	void *	_para4;
}POST_PARA_4;
typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32	_result;
	void *	_para1;
	void *	_para2;
	void *	_para3;
	void *	_para4;
	void *	_para5;
}POST_PARA_5;
typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32	_result;
	void *	_para1;
	void *	_para2;
	void *	_para3;
	void *	_para4;
	void *	_para5;
	void *	_para6;
}POST_PARA_6;
typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32	_result;
	void *	_para1;
	void *	_para2;
	void *	_para3;
	void *	_para4;
	void *	_para5;
	void *	_para6;
	void *	_para7;
}POST_PARA_7;
typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32	_result;
	void *	_para1;
	void *	_para2;
	void *	_para3;
	void *	_para4;
	void *	_para5;
	void *	_para6;
	void *	_para7;
	void *	_para8;
}POST_PARA_8;

_int32 em_init_post_msg(void);
_int32 em_uninit_post_msg(void);

/*
 * Parameter:   
 *         timeout : ms
 *		   msgid:  NULL is allowed
 *
 * OPTIMIZATION: if the spending of copying MSG_INFO too much, 
 *               we can provide the allocating interface of inner node so that we can pass pointer.
 */
_int32 em_post_message(const MSG_INFO *msg_info, msg_handler handler, _int16 notice_count, _u32 timeout, _u32 *msgid);

/*
 *	Add lock for case of thread-safe
 */
_int32 em_post_message_from_other_thread(thread_msg_handle handler, void *args);


_int32 em_cancel_message_by_msgid(_u32 msg_id);
_int32 em_cancel_message_by_device_id(_u32 device_id, _u32 device_type);

_int32 em_post_function(thread_msg_handle handler, void *args,SEVENT_HANDLE * handle,_int32 * result);
_int32 em_post_function_unlock(thread_msg_handle handler, void *args,SEVENT_HANDLE * handle,_int32 * result);
_int32 em_force_signal_sevent(void);
/* Some interface about timer
 * Simplified version of post_message() & cancel_message_by_msgid()
 * timer_handle can be NULL
 * in callback funtion, user_data1 is MSG_INFO._device_id; user_data2 is MSG_INFO._user_data
 * timeout : ms
 */
_int32 em_start_timer(msg_handler handler, _int16 notice_count, _u32 timeout, _u32 user_data1, void *user_data2, _u32 *timer_handle);
_int32 em_cancel_timer(_u32 timer_handle);


#ifdef __cplusplus
}
#endif

#endif
