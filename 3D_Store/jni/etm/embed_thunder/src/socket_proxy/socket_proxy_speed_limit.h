#ifndef _SOCKET_PROXY_SPEED_LIMIT_H_
#define	_SOCKET_PROXY_SPEED_LIMIT_H_

#ifdef __cplusplus
extern "C" 
{
#endif
#include "platform/sd_socket.h"
#include "asyn_frame/msg.h"

typedef struct tagSOCKET_RECV_REQUEST
{
	SOCKET	_sock;
	_u16	_type;
	BOOL	_is_cancel;
	char*	_buffer;
	_u32	_len;
	void*	_callback;
	void*	_user_data;
	BOOL	_oneshot;
}SOCKET_RECV_REQUEST;

typedef struct tagSOCKET_SEND_REQUEST
{
	SOCKET	_sock;
	_u16	_type;
	BOOL	_is_cancel;
	const char*	_buffer;
	_u32	_len;
	SD_SOCKADDR _addr;
	void*	_callback;
	void*	_user_data;
	BOOL    _is_data;
	
}SOCKET_SEND_REQUEST;

_int32 init_socket_proxy_speed_limit(void);

_int32 uninit_socket_proxy_speed_limit(void);

void sl_set_speed_limit(_u32 download_limit_speed, _u32 upload_limit_speed);

void sl_get_speed_limit(_u32* download_limit_speed, _u32* upload_limit_speed);

_u32 sl_get_p2s_recv_block_size(void);

_u32 speed_limit_get_download_speed(void);

_u32 speed_limit_get_max_download_speed(void);

_u32 speed_limit_get_upload_speed(void);

_int32 speed_limit_peek_op_count(SOCKET sock, _u16 type, _u32* speed_limit_count);

_int32 speed_limit_cancel_request(SOCKET sock, _u16 type);

_int32 speed_limit_timeout_handler(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid);

_int32 speed_limit_add_recv_request(SOCKET sock, _u16 type, char* buffer, _u32 len, void* callback, void* user_data, BOOL oneshot);

_int32 speed_limit_add_send_request_data_item(SOCKET sock
    , _u16 type, const char* buffer, _u32 len, SD_SOCKADDR* addr, void* callback, void* user_data);

void speed_limit_update_request(void);

void speed_limit_update_send_request(void);

#ifdef __cplusplus
}
#endif

#endif

