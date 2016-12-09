/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/11/27
-----------------------------------------------------------------------------------------------------------*/
#ifndef _EMULE_SOCKET_DEVICE_H_
#define _EMULE_SOCKET_DEVICE_H_
#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL

#include "emule/emule_transfer_layer/emule_server_encryptor.h"
#include "emule/emule_utility/emule_define.h"
#include "platform/sd_socket.h"

typedef enum tagEMULE_SOCKET_TYPE { EMULE_TCP_TYPE = 0,	EMULE_UDT_TYPE } EMULE_SOCKET_TYPE;

typedef struct tagEMULE_SOCKET_DEVICE
{
	EMULE_SOCKET_TYPE			_type;
	void*						_device;
	void*						_user_data;
	void**						_callback_table;
	EMULE_SERVER_ENCRYPTOR* 	_encryptor;
}EMULE_SOCKET_DEVICE;

typedef _int32 (*emule_socket_device_notify_connect)(void* user_data, _int32 errcode);
typedef _int32 (*emule_socket_device_notify_send)(void* user_data, char* buffer, _int32 len, _int32 errcode);
typedef _int32 (*emule_socket_device_notify_recv_cmd)(void* user_data, char* buffer, _int32 len, _int32 errcode);
typedef _int32 (*emule_socket_device_notify_recv_data)(void* user_data, char* buffer, _int32 len, _int32 errcode);
typedef _int32 (*emule_socket_device_notify_close)(void* user_data);
typedef _int32 (*emule_socket_device_notify_error)(void* user_data, _int32 errcode);

_int32 emule_socket_device_create(EMULE_SOCKET_DEVICE** socket_device, EMULE_SOCKET_TYPE type, EMULE_SERVER_ENCRYPTOR* encrytor, void** callback_table, void* user_data);

_int32 emule_socket_device_connect(EMULE_SOCKET_DEVICE* socket_device, _u32 ip, _u16 port, _u8 conn_id[USER_ID_SIZE]);

_int32 emule_socket_device_connect_callback(EMULE_SOCKET_DEVICE* socket_device, _int32 errcode);

_int32 emule_socket_device_send(EMULE_SOCKET_DEVICE* socket_device, char* buffer, _int32 len);
_int32 emule_socket_device_send_in_speed_limit(EMULE_SOCKET_DEVICE* socket_device, char* buffer, _int32 len);


_int32 emule_socket_device_send_callback(EMULE_SOCKET_DEVICE* socket_device, char* buffer, _int32 len, _int32 errcode);

_int32 emule_socket_device_recv(EMULE_SOCKET_DEVICE* socket_device, char* buffer, _int32 len, BOOL data);

_int32 emule_socket_device_recv_callback(EMULE_SOCKET_DEVICE* socket_device, char* buffer, _int32 len, BOOL data, _int32 errcode);

_int32 emule_socket_device_close(EMULE_SOCKET_DEVICE* socket_device);

_int32 emule_socket_device_close_callback(EMULE_SOCKET_DEVICE* socket_device);

_int32 emule_socket_device_destroy(EMULE_SOCKET_DEVICE** socket_device);

_int32 emule_socket_device_error(EMULE_SOCKET_DEVICE* socket_device, _int32 errcode);

#endif
#endif /* ENABLE_EMULE */
#endif


