#ifndef _SOCKET_PROXY_ENCAP_HTTP_CLIENT_H_
#define _SOCKET_PROXY_ENCAP_HTTP_CLIENT_H_

#ifdef __cplusplus
extern "C" 
{
#endif

/* -------------------------------------------------------------------------------- */

#include "utility/define.h"
#include "utility/errcode.h"

#include "socket_proxy_interface.h"

/* -------------------------------------------------------------------------------- */

/* socket http client 封装。 */
#define  SOCKET_ENCAP_HTTP_PROTOCAL_SEND_NONE (0)
#define  SOCKET_ENCAP_HTTP_PROTOCAL_SEND_CALL (0x1)
#define  SOCKET_ENCAP_HTTP_PROTOCAL_SEND_GET (0x2)

#define  SOCKET_ENCAP_STATE_HTTP_CLIENT_HEADER (0x0)
#define  SOCKET_ENCAP_STATE_HTTP_CLIENT_BODY (0x1)

#define  SOCKET_ENCAP_HTTP_CLIENT_HEADER_BUF_SIZE (128-1)
#define  SOCKET_ENCAP_HTTP_CLIENT_SEND_HEADER_BUF_SIZE (256)

typedef struct _tag_socket_encap_http_client_callback_data_s
{
	SOCKET _sock;
	void * _user_data;
}SOCKET_ENCAP_HTTP_CLIENT_CALLBACK_DATA_S;

typedef struct _tag_socket_encap_http_client_bak_recv_op_s
{
	char * _buffer;
	_int32 _size;
	socket_proxy_recv_handler _callback_handler;
	SOCKET_ENCAP_HTTP_CLIENT_CALLBACK_DATA_S _callback_data;
	BOOL _one_shot;
}SOCKET_ENCAP_HTTP_CLIENT_BAK_RECV_OP_S;

typedef struct _tag_socket_encap_http_client_bak_send_op_s
{
	char * _buffer;
	_int32 _size;
	socket_proxy_send_handler _callback_handler;
	SOCKET_ENCAP_HTTP_CLIENT_CALLBACK_DATA_S _callback_data;
	BOOL _one_shot;
}SOCKET_ENCAP_HTTP_CLIENT_BAK_SNED_OP_S;

typedef struct _tag_socket_encap_http_client_s
{
	BOOL _recv_enable;	// 当前是否能接收RECV，如果不能需要先发一个HTTP GET。
	_u32 _recv_state;	// 0-接收http header，1-接收http body.
	char _recv_header_buf[SOCKET_ENCAP_HTTP_CLIENT_HEADER_BUF_SIZE + 1];	// "+1"是为了sd_strstr()能判断到'\0'。
	char *_p_recv_data;		// 设定当前接收数据BUF
	_u32 _recv_data_size;	// 设定当前接收数据BUF 大小。
	char *_p_recv_body_data;	// 上层传入的数据接收BUF
	_u32 _recv_body_data_size;	// 上层传入的数据接收BUF 大小。
	BOOL _recv_one_shot;	// 上层传入的one_shot值。
	_u32 _recv_keyword_remain_in_buf;	// 上次解析后，在header buf中保留下的关键字大小。
	_u32 _recv_body_remain_in_buf;	//在header buf中还未读取完的body大小。
	_u32 _recv_http_body_total;	// 从http 中解析出的body buf 的长度。
	_u32 _recv_http_body_remain;	// http 剩余的body数量（包括了在header buf中还未读取完的body大小）。
	_u32 _recv_body_len;	// （本次的recv请求中）已收到并已copy到上层的 buf 中的数量。
	socket_proxy_recv_handler _callback_handler_recv;	// 暂存上层设置的recv回调函数。
	SOCKET_ENCAP_HTTP_CLIENT_CALLBACK_DATA_S _callback_recv_data;	// 暂存socket以及上层设置的recv user_data。

	_u32 _send_type;	// 0-当前没有send操作，1-当前执行上层调用的send操作，2-当前执行HTTP GET的send操作。
	_u32 _send_state;	// 0-发送http header，1-发送http body.
	char _send_header_buf[SOCKET_ENCAP_HTTP_CLIENT_SEND_HEADER_BUF_SIZE];	// send header buf.
	_u32 _send_header_total_len;	// HEADER的总长度。
	_u32 _sent_header_len;	// 已发出的HEADER的长度。
	_u32 _sent_body_len;	// 已发出的BODY的长度。
	char *_p_send_body_data;		// 上层传入的数据发送BUF
	_u32 _send_body_data_size;	// 上层传入的数据发送BUF 大小。
	socket_proxy_send_handler _callback_handler_send;	// 暂存上层设置的send回调函数。
	SOCKET_ENCAP_HTTP_CLIENT_CALLBACK_DATA_S _callback_send_data;	// 暂存socket以及上层设置的send user_data。

	BOOL _http_protocal_need_recv;	// 是否有暂存的recv 调用。
	SOCKET_ENCAP_HTTP_CLIENT_BAK_RECV_OP_S _bak_op_recv;
	
	BOOL _http_protocal_need_send;	// 是否有暂存的send 调用。
	SOCKET_ENCAP_HTTP_CLIENT_BAK_SNED_OP_S _bak_op_send;
} SOCKET_ENCAP_HTTP_CLIENT_S;

_int32 socket_encap_http_client_recv_handler(_int32 errcode, _u32 pending_op_count,
	char* buffer, _u32 had_recv, void* user_data);

_int32 socket_encap_http_client_send_handler(_int32 errcode, _u32 pending_op_count,
	const char* buffer, _u32 had_send, void* user_data);

_int32 socket_encap_http_client_send(SOCKET sock, const char* buffer, _u32 len,
	socket_proxy_send_handler callback_handler, void* user_data);

_int32 socket_encap_http_client_recv(SOCKET sock, char* buffer, _u32 len,
	socket_proxy_recv_handler callback_handler, void* user_data);

_int32 socket_encap_http_client_uncomplete_recv(SOCKET sock, char* buffer, _u32 len,
	socket_proxy_recv_handler callback_handler, void* user_data);

_int32 socket_encap_http_client_send_http_get(SOCKET sock, char* buffer, _u32 len,
	socket_proxy_send_handler callback_handler, void* user_data);

/* -------------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif
#endif

