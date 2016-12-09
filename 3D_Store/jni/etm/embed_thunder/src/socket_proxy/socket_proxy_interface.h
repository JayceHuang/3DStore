#ifndef _SOCKET_PROXY_INTERFACE_H_
#define _SOCKET_PROXY_INTERFACE_H_

#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"
#include "platform/sd_socket.h"
#include "asyn_frame/msg.h"
#include "asyn_frame/device.h"

#define DNS_DOMAIN_MAX_IP_NUM 32

/*dns common cache*/
typedef struct
{
	char* _host_name;
	_u32 _port;
	_u32 _ip;
	_u32 _start_time;
}DNS_COMMON_CACHE;

typedef struct
{
	_u32 _hash;
	char _host_name[MAX_HOST_NAME_LEN];
	_u32 _ip_count;
	_u32 _ip[DNS_DOMAIN_MAX_IP_NUM];
}DNS_DOMAIN;

typedef _int32 (*socket_proxy_connect_handler)(_int32 errcode, _u32 pending_op_count, void* user_data);
typedef _int32 (*socket_proxy_accept_handler)(_int32 errcode, _u32 pending_op_count, SOCKET conn_sock, void* user_data);
typedef _int32 (*socket_proxy_send_handler)(_int32 errcode, _u32 pending_op_count, const char* buffer, _u32 had_send, void* user_data);
typedef _int32 (*socket_proxy_sendto_handler)(_int32 errcode, _u32 pending_op_count, const char* buffer, _u32 had_send, void* user_data);
typedef _int32 (*socket_proxy_recv_handler)(_int32 errcode, _u32 pending_op_count, char* buffer, _u32 had_recv, void* user_data);
typedef _int32 (*socket_proxy_recvfrom_handler)(_int32 errcode, _u32 pending_op_count, char* buffer, _u32 had_recv, SD_SOCKADDR* from, void* user_data);

_int32	init_socket_proxy_module(void);
_int32	uninit_socket_proxy_module(void);

_int32 socket_proxy_create(SOCKET* sock, _int32 type);
_int32 socket_proxy_create_http_client(SOCKET* sock, _int32 type);
_int32 socket_proxy_create_browser(SOCKET* sock, _int32 type);

_int32 socket_proxy_bind(SOCKET sock, SD_SOCKADDR* addr);
_int32 socket_proxy_listen(SOCKET sock, _int32 backlog);
_int32 socket_proxy_accept(SOCKET sock, socket_proxy_accept_handler callback_handler, void* user_data);

_int32 socket_proxy_connect(SOCKET sock, SD_SOCKADDR* addr, socket_proxy_connect_handler callback_handler, void* user_data);
_int32 socket_proxy_connect_with_timeout(SOCKET sock, SD_SOCKADDR* addr, _u32 connect_timeout, socket_proxy_connect_handler callback_handler, void* user_data);
_int32 socket_proxy_connect_by_domain(SOCKET sock, const char* host, _u16 port, socket_proxy_connect_handler callback_handler, void* user_data);
_int32 socket_proxy_connect_by_proxy(SOCKET sock, const char* host, _u16 port, socket_proxy_connect_handler callback_handler, void* user_data);

_int32 socket_proxy_send(SOCKET sock, const char* buffer, _u32 len, socket_proxy_send_handler callback_handler, void* user_data);
_int32 socket_proxy_send_in_speed_limit(SOCKET sock, const char* buffer, _u32 len, socket_proxy_send_handler callback_handler, void* user_data);

_int32 socket_proxy_send_function(SOCKET sock, const char* buffer, _u32 len, socket_proxy_send_handler callback_handler, void* user_data);
_int32 socket_proxy_recv(SOCKET sock, char* buffer, _u32 len, socket_proxy_recv_handler callback_handler, void* user_data);
_int32 socket_proxy_recv_function(SOCKET sock, char* buffer, _u32 len, socket_proxy_recv_handler callback_handler, void* user_data, BOOL is_one_shot);
_int32 socket_proxy_uncomplete_recv(SOCKET sock, char* buffer, _u32 len, socket_proxy_recv_handler callback_handler, void* user_data);

_int32 socket_proxy_sendto(SOCKET sock, const char* buffer, _u32 len, SD_SOCKADDR* addr, socket_proxy_sendto_handler callback_handler, void* user_data);
_int32 socket_proxy_sendto_in_speed_limit(SOCKET sock, const char* buffer, _u32 len, SD_SOCKADDR* addr, socket_proxy_sendto_handler callback_handler, void* user_data);
_int32 socket_proxy_sendto_by_domain(SOCKET sock, const char* buffer, _u32 len, const char* host, _u16 port, socket_proxy_sendto_handler callback_handler, void* user_data);

_int32 socket_proxy_recvfrom(SOCKET sock, char* buffer, _u32 len, socket_proxy_recvfrom_handler callback_handler, void* user_data);
_int32 socket_proxy_cancel(SOCKET sock, _int32 type);

_int32 socket_proxy_peek_op_count(SOCKET sock, _int32 type, _u32* count);

_int32 socket_proxy_close(SOCKET sock);

_int32 socket_proxy_send_browser(SOCKET sock, const char* buffer, _u32 len, socket_proxy_send_handler callback_handler, void* user_data);
_int32 socket_proxy_recv_browser(SOCKET sock, char* buffer, _u32 len, socket_proxy_recv_handler callback_handler, void* user_data);
_int32 socket_proxy_uncomplete_recv_browser(SOCKET sock, char* buffer, _u32 len, socket_proxy_recv_handler callback_handler, void* user_data);

_int32 dns_add_domain_ip(const char * host_name, const char *ip_str);
_int32 dns_clear_domain_map(void);

void socket_proxy_set_speed_limit(_u32 download_limit_speed, _u32 upload_limit_speed);
void socket_proxy_get_speed_limit(_u32* download_limit_speed, _u32* upload_limit_speed);

_u32 socket_proxy_get_p2s_recv_block_size(void);

_u32 socket_proxy_speed_limit_get_download_speed(void);
_u32 socket_proxy_speed_limit_get_max_download_speed(void);
_u32 socket_proxy_speed_limit_get_upload_speed(void);

#ifdef _ENABLE_SSL
#include <openssl/bio.h>
_int32 socket_proxy_connect_ssl(SOCKET sock, BIO* pbio, 
	socket_proxy_connect_handler callback_handler, void* user_data);
_int32 socket_proxy_send_ssl(SOCKET sock, BIO* pbio, char* buffer, _u32 len,
	socket_proxy_send_handler callback_handler, void* user_data);
_int32 socket_proxy_recv_ssl(SOCKET sock, BIO* pbio, char* buffer, _u32 len,
	socket_proxy_recv_handler callback_handler, void* user_data);
#endif

extern void sl_get_speed_limit(_u32* download_limit_speed, _u32* upload_limit_speed);

#ifdef __cplusplus
}
#endif
#endif

