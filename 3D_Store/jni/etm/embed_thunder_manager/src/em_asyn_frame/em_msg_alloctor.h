#ifndef _EM_MSG_ALLOCTOR_H_00138F8F2E70_200806252159
#define _EM_MSG_ALLOCTOR_H_00138F8F2E70_200806252159

#ifdef __cplusplus
extern "C" 
//{
#endif

#include "em_asyn_frame/em_device.h"

_int32 em_msg_alloctor_init(void);
_int32 em_msg_alloctor_uninit(void);

_int32 em_msg_alloc(MSG **msg);
_int32 em_msg_dealloc(MSG *msg);


/* thread msg */
_int32 em_msg_thread_alloc(THREAD_MSG **ppmsg);
_int32 em_msg_thread_dealloc(THREAD_MSG *pmsg);

#ifdef __cplusplus
//}
#endif

#endif
