#ifndef _SD_MSG_LIST_H_00138F8F2E70_200806241435
#define _SD_MSG_LIST_H_00138F8F2E70_200806241435

#ifdef __cplusplus
extern "C" 
{
#endif


#include "utility/queue.h"
#include "asyn_frame/msg.h"
#include "asyn_frame/device.h"

/*
 *
 */
_int32 msg_queue_init(_int32 *waitable_handle);

/*
 *
 */
_int32 msg_queue_uninit(void);


/*
 * Return immediately, even if there were not any msg popped.
 * If no msg popped, the *msg will be NULL.
 */
_int32 pop_msginfo_node(MSG **msg);


_int32 push_msginfo_node(MSG *msg);


/*thread msg*/
_int32 pop_msginfo_node_from_other_thread(THREAD_MSG **msg);
_int32 push_msginfo_node_in_other_thread(THREAD_MSG *msg);




#ifdef __cplusplus
}
#endif

#endif
