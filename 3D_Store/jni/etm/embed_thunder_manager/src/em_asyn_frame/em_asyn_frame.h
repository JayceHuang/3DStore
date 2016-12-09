#ifndef _EM_ASYN_FRAME_H_00138F8F2E70_200806251344
#define _EM_ASYN_FRAME_H_00138F8F2E70_200806251344

#ifdef __cplusplus
extern "C" 
{
#endif

#include "em_asyn_frame/em_msg_list.h"
#include "em_asyn_frame/em_device.h"

typedef _int32 (*init_handler)(void*);
typedef _int32 (*uninit_handler)(void*);

_int32 em_start_asyn_frame(init_handler h_init, void *init_arg, uninit_handler h_uninit, void *uninit_arg);

_int32 em_stop_asyn_frame(void);

/* only support socket-fd, device_type must be DEVICE_SOCKET_TCP || DEVICE_SOCKET_UDP */
_int32 em_peek_operation_count_by_device_id(_u32 device_id, _u32 device_type, _u32 *count);

#if defined(__SYMBIAN32__)
_int32 em_asyn_frame_handler(void *arglist );
BOOL em_asyn_frame_has_msg_to_do(void);
#endif /* __SYMBIAN32__ */

#ifdef __cplusplus
}
#endif

#endif
