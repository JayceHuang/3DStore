#ifndef _SOCKET_PROXY_ENCAP_H_
#define _SOCKET_PROXY_ENCAP_H_

#ifdef __cplusplus
extern "C" 
{
#endif

/* -------------------------------------------------------------------------------- */

#include "socket_proxy_encap_http_client.h"

/* proxy socket 封装类型: 无封装。 */
#define  SOCKET_ENCAP_TYPE_NONE (0)

/* proxy socket 封装类型: 用http client方式收发包。 */
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

/* 初始化 */
_int32 init_socket_encap();

/* 反初始化 */
_int32 uninit_socket_encap();

/* 关联一个新socket的属性 */
_int32 insert_socket_encap_prop(_u32 sock_num, SOCKET_ENCAP_PROP_S* p_encap_prop);

/* 删除一个socket关联的属性 */
_int32 remove_socket_encap_prop(_u32 sock_num);

/* 获得一个socket关联的属性 */
_int32 get_socket_encap_prop_by_sock(_u32 sock_num, SOCKET_ENCAP_PROP_S** pp_encap_prop);

/* -------------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif
#endif

