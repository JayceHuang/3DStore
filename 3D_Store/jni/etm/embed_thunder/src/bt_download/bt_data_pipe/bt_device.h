/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2009/02/26
-----------------------------------------------------------------------------------------------------------*/
#ifndef _BT_SOCKET_DEVICE_H_
#define	_BT_SOCKET_DEVICE_H_

#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"
#ifdef ENABLE_BT 
#ifdef ENABLE_BT_PROTOCOL

#include "bt_download/bt_data_pipe/bt_data_pipe.h"
#include "platform/sd_socket.h"
#include "utility/speed_calculator.h"

typedef struct tagBT_DEVICE
{
	SOCKET	_sock;
	BT_DATA_PIPE* _bt_pipe;
	char*	_recv_buffer;
	_u32	_recv_buffer_len;
	_u32	_recv_buffer_offset;
	SPEED_CALCULATOR _speed_calculator;

	SPEED_CALCULATOR _upload_speed_calculator;

}BT_DEVICE;

_int32 bt_create_device(BT_DEVICE** device, BT_DATA_PIPE* bt_pipe);

_int32 bt_close_device(BT_DEVICE* device);

_int32 bt_destroy_device(BT_DEVICE* device);

_int32 bt_device_connect(BT_DEVICE* device, _u32 ip, _u16 port);

_int32 bt_device_connect_callback(_int32 errcode, _u32 pending_op_count, void* user_data);

_int32 bt_device_send(BT_DEVICE* device, char* buffer, _u32 len);
_int32 bt_device_send_in_speed_limit(BT_DEVICE* device, char* buffer, _u32 len);


_int32 bt_device_send_callback(_int32 errcode, _u32 pending_op_count, const char* buffer, _u32 had_send, void* user_data);


_int32 bt_device_send_data(BT_DEVICE* device, char* buffer, _u32 len);

_int32 bt_device_send_data_callback(_int32 errcode, _u32 pending_op_count, const char* buffer, _u32 had_send, void* user_data);


_int32 bt_device_recv(BT_DEVICE* device, _u32 len);

_int32 bt_device_recv_callback(_int32 errcode, _u32 pending_op_count, char* buffer, _u32 had_recv, void* user_data);

_int32 bt_device_recv_data(BT_DEVICE* device, char* buffer, _u32 len);

_int32 bt_device_recv_data_callback(_int32 errcode, _u32 pending_op_count, char* buffer, _u32 had_recv, void* user_data);

_int32 bt_device_recv_discard_data(BT_DEVICE* device, char* buffer, _u32 len);

_int32 bt_device_recv_discard_data_callback(_int32 errcode, _u32 pending_op_count, char* buffer, _u32 had_recv, void* user_data);

_int32 bt_device_handle_error(_int32 errcode, _u32 pending_op_count, BT_DEVICE* device);
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif
