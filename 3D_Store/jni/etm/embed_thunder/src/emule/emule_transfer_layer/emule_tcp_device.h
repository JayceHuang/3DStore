/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/11/4
-----------------------------------------------------------------------------------------------------------*/
#ifndef _EMULE_TCP_DEVICE_H_
#define _EMULE_TCP_DEVICE_H_
#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL

#include "platform/sd_socket.h"

struct tagEMULE_SOCKET_DEVICE;

typedef struct tagEMULE_TCP_DEVICE
{
	SOCKET							_sock;
	struct tagEMULE_SOCKET_DEVICE*	_socket_device;
}EMULE_TCP_DEVICE;

_int32 emule_tcp_device_create(EMULE_TCP_DEVICE** tcp_device);

_int32 emule_tcp_device_close(EMULE_TCP_DEVICE* tcp_device);

_int32 emule_tcp_device_destroy(EMULE_TCP_DEVICE* tcp_device);

_int32 emule_tcp_device_connect(EMULE_TCP_DEVICE* tcp_device, _u32 ip, _u16 port);

_int32 emule_tcp_device_connect_callback(_int32 errcode, _u32 pending_op_count, void* user_data);

_int32 emule_tcp_device_send(EMULE_TCP_DEVICE* tcp_device, char* buffer, _int32 len);
_int32 emule_tcp_device_send_in_speed_limit(EMULE_TCP_DEVICE* tcp_device, char* buffer, _int32 len);


_int32 emule_tcp_device_send_callback(_int32 errcode, _u32 pending_op_count, const char* buffer, _u32 had_send, void* user_data);

_int32 emule_tcp_device_recv(EMULE_TCP_DEVICE* tcp_device, char* buffer, _int32 len, BOOL is_data);

_int32 emule_tcp_device_recv_cmd_callback(_int32 errcode, _u32 pending_op_count, char* buffer, _u32 had_recv, void* user_data);

_int32 emule_tcp_device_recv_data_callback(_int32 errcode, _u32 pending_op_count, char* buffer, _u32 had_recv, void* user_data);

#endif
#endif /* ENABLE_EMULE */
#endif


