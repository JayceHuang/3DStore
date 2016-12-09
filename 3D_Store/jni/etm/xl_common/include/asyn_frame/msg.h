#ifndef _SD_MSG_H_00138F8F2E70_200806051401
#define _SD_MSG_H_00138F8F2E70_200806051401

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#include "platform/sd_task.h"

/************************************************************************/
/*     Used by asyn_frame                                               */
/************************************************************************/

#define CANCEL_MSG_BY_MSGID		(-1)
#define CANCEL_MSG_BY_DEVICEID	(-2)

#define VALID_HANDLE(handle)	((handle) != CANCEL_MSG_BY_MSGID && (handle) != CANCEL_MSG_BY_DEVICEID)


/************************************************************************/
/*              Used by caller                                          */
/************************************************************************/

#define INVALID_MSG_ID		(0)

#define MSG_PRIORITY_URGERNT	(1)
#define MSG_PRIORITY_NORMAL		(2)
#define MSG_PRIORITY_IDLE		(3)


/* REASON > 0 : ERRCODE */
#define MSG_SUCC		(0)
#define MSG_TIMEOUT		(-1)
#define MSG_CANCELLED	(-2)

/* notice count */
#define NOTICE_ONCE			(1)
#define NOTICE_INFINITE		(-1)


typedef _int32 (*thread_msg_handle)(void *param);

typedef struct {
	thread_msg_handle _handle;
	void *_param;
} THREAD_MSG;


typedef struct {
	_u32 _device_id;
	_u16 _msg_priority;
	_u16 _device_type;
	_u16 _operation_type;
	_u16 _pending_op_count;
	void *_operation_parameter;
	void *_user_data;
#ifdef _ENABLE_SSL
	_u32  _ssl_magicnum;
	void* _pbio;
#endif
} MSG_INFO, *pMSG_INFO;


/* 
 *	NOTICE: all parameters are readonly!
 *          all parameters will be destoryed when the callback-handler returned!
 *          user must duplicate the parameter that will be used later
 *  Parameter:
 *          errcode: MSG_SUCC || MSG_TIMEOUT || MSG_CANCELLED || errcode 
 */
typedef _int32 (*msg_handler)(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid);


/* the first member must be _handler, used for @FIND_NODE@ in socket_reactor.c later*/
typedef struct {
	msg_handler _handler; /* when cancel a msg, this member will be CANCEL_MSG_BY_MSGID || CANCEL_MSG_BY_DEVICEID */
	MSG_INFO _msg_info;

	_u32 _msg_id;

	_u32 _timeout;
	_int16 _notice_count_left;

	/* used for record timer-index, a simple optimazation of erasing a msg from timer.
	   will be improved in future */
	_int16 _timeout_index;

	/* communication between asyn_frame and reactors.
	 * defined here avoid allocating memory twice
	 * value of _cur_reactor_status is defined in device_reactor.h
	 * ONLY when _op_count == 0, this object can be desturcted.
	 */
	_u8 _op_count;
	_u8 _cur_reactor_status : 7;
	_u8 _msg_cannelled      : 1; /* 0: normal     1: cannelled*/

	/* _op_errcode:     SUCESS(0)/REACTOR_E_UNREGISTER(-1)/other operation-errcode(>0) */
	_int16	_op_errcode;

	void *_inner_data;
	_u32 _timestamp;
} MSG;

_int32 init_post_msg(TASK_LOCK **task_lock);
_int32 uninit_post_msg(TASK_LOCK **task_lock);


/*
 * Parameter:   
 *         timeout : ms
 *		   msgid:  NULL is allowed
 *
 * OPTIMIZATION: if the spending of copying MSG_INFO too much, 
 *               we can provide the allocating platform of inner node so that we can pass pointer.
 */
_int32 post_message(const MSG_INFO *msg_info, msg_handler handler, _int16 notice_count, _u32 timeout, _u32 *msgid);

/*
 *	Add lock for case of thread-safe
 */
_int32 post_message_from_other_thread(thread_msg_handle handler, void *args);


_int32 cancel_message_by_msgid(_u32 msg_id);
_int32 cancel_message_by_device_id(_u32 device_id, _u32 device_type);

/* Some platform about timer
 * Simplified version of post_message() & cancel_message_by_msgid()
 * timer_handle can be NULL
 * in callback funtion, user_data1 is MSG_INFO._device_id; user_data2 is MSG_INFO._user_data
 * timeout : ms
 */
_int32 start_timer(msg_handler handler, _int16 notice_count, _u32 timeout, _u32 user_data1, void *user_data2, _u32 *timer_handle);
_int32 cancel_timer(_u32 timer_handle);


#ifdef __cplusplus
}
#endif

#endif
