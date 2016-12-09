#ifndef _SD_SOCKET_REACTOR_H_00138F8F2E70_200807091038
#define _SD_SOCKET_REACTOR_H_00138F8F2E70_200807091038

#ifdef __cplusplus
extern "C" 
{
#endif

#include "asyn_frame/msg.h"
#include "asyn_frame/device_reactor.h"

_int32 init_socket_reactor(_int32 *waitable_handle);
_int32 uninit_socket_reactor(void);

_int32 register_socket_event(MSG *msg);
_int32 unregister_socket(MSG *msg, _int32 reason);

_int32 get_complete_socket_msg(MSG **msg);

_int32 cancel_socket(_int32 socket_id);

_int32 peek_op_count(_int32 socket_id, _u32 *count);

#ifdef __cplusplus
}
#endif

#endif
