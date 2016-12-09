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

/* socket http client ��װ�� */
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
	BOOL _recv_enable;	// ��ǰ�Ƿ��ܽ���RECV�����������Ҫ�ȷ�һ��HTTP GET��
	_u32 _recv_state;	// 0-����http header��1-����http body.
	char _recv_header_buf[SOCKET_ENCAP_HTTP_CLIENT_HEADER_BUF_SIZE + 1];	// "+1"��Ϊ��sd_strstr()���жϵ�'\0'��
	char *_p_recv_data;		// �趨��ǰ��������BUF
	_u32 _recv_data_size;	// �趨��ǰ��������BUF ��С��
	char *_p_recv_body_data;	// �ϲ㴫������ݽ���BUF
	_u32 _recv_body_data_size;	// �ϲ㴫������ݽ���BUF ��С��
	BOOL _recv_one_shot;	// �ϲ㴫���one_shotֵ��
	_u32 _recv_keyword_remain_in_buf;	// �ϴν�������header buf�б����µĹؼ��ִ�С��
	_u32 _recv_body_remain_in_buf;	//��header buf�л�δ��ȡ���body��С��
	_u32 _recv_http_body_total;	// ��http �н�������body buf �ĳ��ȡ�
	_u32 _recv_http_body_remain;	// http ʣ���body��������������header buf�л�δ��ȡ���body��С����
	_u32 _recv_body_len;	// �����ε�recv�����У����յ�����copy���ϲ�� buf �е�������
	socket_proxy_recv_handler _callback_handler_recv;	// �ݴ��ϲ����õ�recv�ص�������
	SOCKET_ENCAP_HTTP_CLIENT_CALLBACK_DATA_S _callback_recv_data;	// �ݴ�socket�Լ��ϲ����õ�recv user_data��

	_u32 _send_type;	// 0-��ǰû��send������1-��ǰִ���ϲ���õ�send������2-��ǰִ��HTTP GET��send������
	_u32 _send_state;	// 0-����http header��1-����http body.
	char _send_header_buf[SOCKET_ENCAP_HTTP_CLIENT_SEND_HEADER_BUF_SIZE];	// send header buf.
	_u32 _send_header_total_len;	// HEADER���ܳ��ȡ�
	_u32 _sent_header_len;	// �ѷ�����HEADER�ĳ��ȡ�
	_u32 _sent_body_len;	// �ѷ�����BODY�ĳ��ȡ�
	char *_p_send_body_data;		// �ϲ㴫������ݷ���BUF
	_u32 _send_body_data_size;	// �ϲ㴫������ݷ���BUF ��С��
	socket_proxy_send_handler _callback_handler_send;	// �ݴ��ϲ����õ�send�ص�������
	SOCKET_ENCAP_HTTP_CLIENT_CALLBACK_DATA_S _callback_send_data;	// �ݴ�socket�Լ��ϲ����õ�send user_data��

	BOOL _http_protocal_need_recv;	// �Ƿ����ݴ��recv ���á�
	SOCKET_ENCAP_HTTP_CLIENT_BAK_RECV_OP_S _bak_op_recv;
	
	BOOL _http_protocal_need_send;	// �Ƿ����ݴ��send ���á�
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

