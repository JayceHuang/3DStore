#ifndef _SD_DEVICE_REACTOR_H_00138F8F2E70_200807092108
#define _SD_DEVICE_REACTOR_H_00138F8F2E70_200807092108

#ifdef __cplusplus
extern "C" 
{
#endif

#include "asyn_frame/notice_queue.h"
#include "asyn_frame/msg.h"
#include "utility/list.h"
#include "platform/sd_task.h"

/* MSG._cur_reactor_status */
#define REACTOR_STATUS_READY		(0)
#define REACTOR_STATUS_REGISTERING	(1)
#define REACTOR_STATUS_REGISTERED	(2)
#define REACTOR_STATUS_CANCEL		(3)
#define REACTOR_STATUS_TIMEOUT		(4)

#define REACTOR_READY(status)					(status == REACTOR_STATUS_READY)
#define REACTOR_REGISTERED(status)		(status == REACTOR_STATUS_REGISTERING || status == REACTOR_STATUS_REGISTERED)
#define REACTOR_UNREGISTERED(status)	(status == REACTOR_STATUS_CANCEL || status == REACTOR_STATUS_TIMEOUT)

/* MSG._op_errcode */
#define REACTOR_E_UNREGISTER	(-1)

/**/
#define MIN_REACTOR_QUEUE_SIZE		(16)

#define INVALID_REACTOR_MSG  (-1)

#define FS_IN_QUEUE_COUNT (1) 

typedef struct {
	NOTICE_QUEUE _in_queue;
	NOTICE_QUEUE _out_queue;
	SEVENT_HANDLE _out_queue_signal;
	LIST _abortive_msg; /* the msg that is unregistered while in _in_queue */

	TASK_LOCK _in_queue_lock; /* can be removed? */ 

	NOTICE_QUEUE _fs_in_queue[FS_IN_QUEUE_COUNT];
} DEVICE_REACTOR;


_int32 device_reactor_init(DEVICE_REACTOR *reactor);
_int32 device_reactor_uninit(DEVICE_REACTOR *reactor);


_int32 register_event(DEVICE_REACTOR *reactor, MSG *msg, void **msg_pos_ptr /*record the queue-node-ptr that store this msg*/);
_int32 register_event_by_thread(DEVICE_REACTOR *reactor, MSG *msg, void **msg_pos_ptr,  int thread_idx);
/* 
 * unregister a event while the opeartion has not completed but timeout, or be cancelled.
 * if this operation is unregistered successfully, a cancelled-msg will be noticed
 */
_int32 unregister_event(DEVICE_REACTOR *reactor, MSG *msg, _int32 reason);

/* try to unregister from reactor->_in_queue first; and even if failed to find event in _in_queue, it would not generate a new cancel-msg */
_int32 unregister_event_immediately(DEVICE_REACTOR *reactor, MSG *msg, _int32 reason, void **msg_pos_ptr);

_int32 check_register(MSG *msg);
_int32 check_unregister(MSG *msg);

_int32 pop_register_event(DEVICE_REACTOR *reactor, MSG **msg);
_int32 pop_register_event_with_lock(DEVICE_REACTOR *reactor, MSG **msg);


/* notice a completed event, simultaneously unregister this event */
_int32 notice_complete_event(DEVICE_REACTOR *reactor, _int16 op_errcode, MSG *msg);

_int32 pop_complete_event(DEVICE_REACTOR *reactor, MSG **msg);

#ifdef __cplusplus
}
#endif

#endif
