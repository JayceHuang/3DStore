#ifndef _SD_ASYN_FRAME_H_00138F8F2E70_200806251344
#define _SD_ASYN_FRAME_H_00138F8F2E70_200806251344

#ifdef __cplusplus
extern "C" 
{
#endif

#include "asyn_frame/msg_list.h"
#include "asyn_frame/device.h"


typedef _int32 (*init_handler)(void*);
typedef _int32 (*uninit_handler)(void*);


_int32 start_asyn_frame(init_handler h_init, void *init_arg, uninit_handler h_uninit, void *uninit_arg);

_int32 stop_asyn_frame(void);

BOOL is_asyn_frame_running(void);

/* only support socket-fd, device_type must be DEVICE_SOCKET_TCP || DEVICE_SOCKET_UDP */
_int32 peek_operation_count_by_device_id(_u32 device_id, _u32 device_type, _u32 *count);

BOOL asyn_frame_is_suspend();
_int32 asyn_frame_suspend();
_int32 asyn_frame_resume();


#ifdef __cplusplus
}
#endif

#endif
