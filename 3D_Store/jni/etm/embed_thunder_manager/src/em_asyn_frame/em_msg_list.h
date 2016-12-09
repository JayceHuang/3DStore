#ifndef _EM_MSG_LIST_H_00138F8F2E70_200806241435
#define _EM_MSG_LIST_H_00138F8F2E70_200806241435

#ifdef __cplusplus
extern "C" 
//{
#endif


#include "em_common/em_queue.h"
#include "em_asyn_frame/em_msg.h"
#include "em_asyn_frame/em_device.h"

/*
 *
 */
_int32 em_msg_queue_init(_int32 *waitable_handle);

/*
 *
 */
_int32 em_msg_queue_uninit(void);


/*
 * Return immediately, even if there were not any msg popped.
 * If no msg popped, the *msg will be NULL.
 */
_int32 em_pop_msginfo_node(MSG **msg);


_int32 em_push_msginfo_node(MSG *msg);


/*thread msg*/
_int32 em_pop_msginfo_node_from_other_thread(THREAD_MSG **msg);
_int32 em_push_msginfo_node_in_other_thread(THREAD_MSG *msg);

BOOL em_msg_has_new_msg(void);

#ifdef __cplusplus
//}
#endif

#endif
