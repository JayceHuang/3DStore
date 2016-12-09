/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/10/31
-----------------------------------------------------------------------------------------------------------*/
#ifndef _EMULE_SERVER_DEVICE_H_
#define _EMULE_SERVER_DEVICE_H_
#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL

#include "emule/emule_server/emule_server.h"
#include "emule/emule_transfer_layer/emule_socket_device.h"
#include "platform/sd_socket.h"

typedef struct tagEMULE_SERVER_DEVICE
{
	EMULE_SOCKET_DEVICE*	_socket_device;
	char*					_buffer;
	_u32					_buffer_len;
	_u32					_buffer_offset;
	EMULE_SERVER*			_server;
	BOOL					_can_query;		//登录成功并获得id_change后，该值才为TRUE,表示可以查询资源了

}EMULE_SERVER_DEVICE;

_int32 emule_server_device_connect(EMULE_SERVER_DEVICE* server_device);

_int32 emule_server_device_connect_callback(void* user_data, _int32 errcode);

_int32 emule_server_device_extend_recv_buffer(EMULE_SERVER_DEVICE* server_device, _u32 len);

_int32 emule_server_device_recv(EMULE_SERVER_DEVICE* device, _int32 len);

_int32 emule_server_device_recv_callback(void* user_data, char* buffer, _int32 len, _int32 errcode);

_int32 emule_server_device_send(EMULE_SERVER_DEVICE* server_device, char* buffer, _int32 len);

_int32 emule_server_device_send_callback(void* user_data, char* buffer, _int32 len, _int32 errcode);

_int32 emule_server_device_close(EMULE_SERVER_DEVICE* server_device);

_int32 emule_server_device_close_callback(void* user_data);

_int32 emule_server_device_notify_error(void* user_data, _int32 errcode);

#endif
#endif /* ENABLE_EMULE */
#endif


