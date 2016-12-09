/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/10/13
-----------------------------------------------------------------------------------------------------------*/
#ifndef _EMULE_PIPE_DEVICE_H_
#define _EMULE_PIPE_DEVICE_H_
#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL

#include "emule/emule_transfer_layer/emule_socket_device.h"
#include "platform/sd_socket.h"
#include "asyn_frame/msg.h"
#include "utility/speed_calculator.h"

struct tagEMULE_DATA_PIPE;
struct tagEMULE_RESOURCE;

typedef struct tagEMULE_PIPE_DEVICE
{
	EMULE_SOCKET_DEVICE*		_socket_device;
	struct tagEMULE_DATA_PIPE*	_pipe;
	char*						_recv_buffer;
	_u32						_recv_buffer_len;
	_u32						_recv_buffer_offset;
	char*						_data_buffer;
	_u32						_data_buffer_len;
	_u32						_data_buffer_offset;
	_u64						_data_pos;
	_u32						_data_len;
	BOOL						_valid_data;
	_u32						_get_buffer_timeout_id;
	SPEED_CALCULATOR			_download_speed;
	BOOL						_passive;
}EMULE_PIPE_DEVICE;
void emule_pipe_device_module_init();
void emule_pipe_device_module_uninit();

void** emule_pipe_device_get_callback_table(void);

_int32 emule_pipe_device_create(EMULE_PIPE_DEVICE** pipe_device);

_int32 emule_pipe_device_close(EMULE_PIPE_DEVICE* pipe_device);

_int32 emule_pipe_device_close_callback(void* user_data);

_int32 emule_pipe_device_destroy(EMULE_PIPE_DEVICE* pipe_device);

_int32 emule_pipe_device_connect(EMULE_PIPE_DEVICE* pipe_device, struct tagEMULE_RESOURCE* res);

_int32 emule_pipe_device_connect_callback(void* user_data, _int32 errcode);

_int32 emule_pipe_device_send(EMULE_PIPE_DEVICE* pipe_device, char* buffer, _int32 len);
_int32 emule_pipe_device_send_data(EMULE_PIPE_DEVICE* pipe_device, char* buffer, _int32 len, void *p_user );

_int32 emule_pipe_device_send_callback(void* user_data, char* buffer, _int32 len, _int32 errcode);

BOOL emule_pipe_device_can_send();

void emule_pipe_remove_user( void *p_target );

_int32 emule_pipe_device_recv_cmd(EMULE_PIPE_DEVICE* pipe_device, _u32 len);

_int32 emule_pipe_device_recv_cmd_callback(void* user_data, char* buffer, _int32 len, _int32 errcode);

_int32 emule_pipe_device_recv_data(EMULE_PIPE_DEVICE* pipe_device, _u32 len);

_int32 emule_pipe_device_recv_data_callback(void* user_data, char* buffer, _int32 len, _int32 errcode);

_int32 emule_pipe_device_notify_error(void* user_data, _int32 errcode);

_int32 emule_pipe_device_extend_recv_buffer(EMULE_PIPE_DEVICE* pipe_device, _u32 len);

_int32 emule_pipe_device_get_buffer_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid);

#endif
#endif /* ENABLE_EMULE */
#endif


