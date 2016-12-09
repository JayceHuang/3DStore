#ifndef _SOCKET_PROXY_INTERFACE_IMP_H_
#define _SOCKET_PROXY_INTERFACE_IMP_H_

#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"
#include "platform/sd_socket.h"
#include "asyn_frame/msg.h"
#include "asyn_frame/device.h"
#include "socket_proxy_interface.h"

struct tagSOCKET_RECV_REQUEST;
struct tagSOCKET_SEND_REQUEST;

_int32 socket_proxy_recv_impl(struct tagSOCKET_RECV_REQUEST* request);
_int32 socket_proxy_recvfrom_impl(struct tagSOCKET_RECV_REQUEST* request);
_int32 socket_proxy_send_impl(struct tagSOCKET_SEND_REQUEST* request);
_int32 socket_proxy_sendto_impl(struct tagSOCKET_SEND_REQUEST * request);

#ifdef __cplusplus
}
#endif
#endif

