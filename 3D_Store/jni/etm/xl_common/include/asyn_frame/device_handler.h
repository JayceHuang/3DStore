#ifndef _SD_DEVICE_HANDLER_H_00138F8F2E70_200807191437
#define _SD_DEVICE_HANDLER_H_00138F8F2E70_200807191437
#ifdef __cplusplus
extern "C" 
{
#endif

#include "asyn_frame/device.h"

_int32 handle_msg(MSG *msg, BOOL *completed);

/* some socket statics info */
_u32 socket_tcp_device_recved_bytes(void);
_u32 socket_udp_device_recved_bytes(void);

_u32 socket_tcp_device_send_bytes(void);
_u32 socket_udp_device_send_bytes(void);

_int32 get_network_flow(_u32 * download,_u32 * upload);

#ifdef __cplusplus
}
#endif
#endif
