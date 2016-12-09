#ifndef _TCP_INTERFACE_H_
#define	_TCP_INTERFACE_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include "platform/sd_socket.h"
#include "p2p_transfer_layer/tcp/tcp_device.h"

struct tagPTL_DEVICE;

_int32 init_tcp_modular(void);

_int32 uninit_tcp_modular(void);

_int32 tcp_device_create(TCP_DEVICE** tcp, SOCKET sock, struct tagPTL_DEVICE* device);

_int32 tcp_device_close(TCP_DEVICE* tcp);

_int32 tcp_device_connect(TCP_DEVICE* tcp, _u32 ip, _u16 port);

_int32 tcp_device_connect_callback(_int32 errcode, _u32 pending_op_count, void* user_data);

_int32 tcp_device_send(TCP_DEVICE* tcp, char* buffer, _u32 len);
_int32 tcp_device_send_in_speed_limit(TCP_DEVICE* tcp, char* cmd_buffer, _u32 len);


_int32 tcp_device_send_callback(_int32 errcode, _u32 pending_op_count, const char* buffer, _u32 had_send, void* user_data);

_int32 tcp_device_recv_cmd(TCP_DEVICE* tcp, char* buffer, _u32 len);

_int32 tcp_device_recv_cmd_callback(_int32 errcode, _u32 pending_op_count, char* tmp_buffer, _u32 had_recv, void* user_data);

_int32 tcp_device_recv_data(TCP_DEVICE* tcp, char* buffer, _u32 len);

_int32 tcp_device_recv_data_callback(_int32 errcode, _u32 pending_op_count, char* buffer, _u32 had_recv, void* user_data);

_int32 tcp_device_recv_discard_data(TCP_DEVICE* tcp, char* buffer, _u32 len);

_int32 tcp_device_recv_discard_data_callback(_int32 errcode, _u32 pending_op_count, char* buffer, _u32 had_recv, void* user_data);

_int32 tcp_device_handle_cancel_msg(TCP_DEVICE* tcp, _u32 pending_op_count);

#ifdef __cplusplus
}
#endif

#endif

