#ifndef _SOCKET_PROXY_ENCAP_H_
#define _SOCKET_PROXY_ENCAP_H_

#ifdef __cplusplus
extern "C" 
{
#endif

/* -------------------------------------------------------------------------------- */

#include "socket_proxy_encap_http_client.h"

/* proxy socket ��װ����: �޷�װ�� */
#define  SOCKET_ENCAP_TYPE_NONE (0)

/* proxy socket ��װ����: ��http client��ʽ�շ����� */
#define  SOCKET_ENCAP_TYPE_HTTP_CLIENT (0x1)

typedef union _tag_socket_encap_state_s
{
	SOCKET_ENCAP_HTTP_CLIENT_S _http_client;
} SOCKET_ENCAP_STATE_S;

typedef struct _tag_socket_encap_prop_s
{
	_u32 _encap_type;
	SOCKET_ENCAP_STATE_S _encap_state;
} SOCKET_ENCAP_PROP_S;

/* ��ʼ�� */
_int32 init_socket_encap();

/* ����ʼ�� */
_int32 uninit_socket_encap();

/* ����һ����socket������ */
_int32 insert_socket_encap_prop(_u32 sock_num, SOCKET_ENCAP_PROP_S* p_encap_prop);

/* ɾ��һ��socket���������� */
_int32 remove_socket_encap_prop(_u32 sock_num);

/* ���һ��socket���������� */
_int32 get_socket_encap_prop_by_sock(_u32 sock_num, SOCKET_ENCAP_PROP_S** pp_encap_prop);

/* -------------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif
#endif

