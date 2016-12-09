/*****************************************************************************
 *
 * Filename: 			dk_socket_handler.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2009/10/15
 *	
 * Purpose:				socket管理和收发包控制.
 *****************************************************************************/


#if !defined(__dk_socket_handler_20091015)
#define __dk_socket_handler_20091015

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#ifdef _DK_QUERY
#include "socket_proxy_interface.h"
#include "platform/sd_socket.h"
#include "utility/map.h"
#include "utility/list.h"
#include "res_query/dk_res_query/dk_protocal_handler.h"

#define DK_MAX_PACKET_LEN ( 4 * 1024 )

typedef _int32 (*dk_recv_packet_handler) ( _u32 ip, _u16 port, char *p_buffer, _u32 data_len );

struct tagDK_SOCKET_HANDLER;

typedef struct tagDK_REQUEST_NODE
{
	_u32 _ip;
	_u16 _port;
	char *_buffer;
	_u32 _buffer_len;
	_u32 _retry_times;
	struct tagDK_SOCKET_HANDLER *_dk_socket_handler_ptr;
}DK_REQUEST_NODE;


typedef struct tagDK_SOCKET_HANDLER
{
	SOCKET _udp_socket;
	_u32 _udp_port;
	MAP _resp_handler_map;//key:_request_id  value:DK_REQUEST_NODE //hash(ip,port,node_id,key,hander_ptr)
	LIST _request_list;
	DK_REQUEST_NODE *_cur_resquest_node_ptr;
	char *_recv_buffer_ptr;
    _u32 _dk_type;
    /* 传给内部的地址统一为:ip使用网络字节序,port使用主机字节序 */
	dk_recv_packet_handler _recv_packet_handler;//dht 和 kad分别注册收数据处理回调函数进来
}DK_SOCKET_HANDLER;

_int32 sh_create( _u32 dk_type );
_int32 sh_try_destory( _u32 dk_type );
_int32 sh_destory( DK_SOCKET_HANDLER *p_dk_socket_handler );

_int32 sh_request_id_comparator(void *E1, void *E2);

_int32 sh_send_packet_by_domain( DK_SOCKET_HANDLER *p_dk_socket_handler, 
	char *p_host, _u16 port, char *p_buffer, _u32 data_len );

//p_protocal_handler为NULL 表示对收包不需要处理
_int32 sh_send_packet( DK_SOCKET_HANDLER *p_dk_socket_handler, _u32 ip, _u16 port, 
	char *p_buffer, _u32 data_len, DK_PROTOCAL_HANDLER *p_protocal_handler, _u32 packet_key );

_int32 sh_handle_request_list( DK_SOCKET_HANDLER *p_dk_socket_handler );

_int32 sh_udp_sendto_callback( _int32 errcode, _u32 pending_op_count, const char *buffer, _u32 had_send, void *user_data );
_int32 sh_udp_recvfrom_callback( _int32 errcode, _u32 pending_op_count, char *buffer, _u32 had_recv, SD_SOCKADDR *from, void *user_data );

DK_SOCKET_HANDLER *sh_ptr( _u32 dk_type );
_int32 sh_get_resp_handler( DK_SOCKET_HANDLER *p_dk_socket_handler, _u32 key, DK_PROTOCAL_HANDLER **pp_protocal_handler );
_int32 sh_clear_resp_handler( DK_SOCKET_HANDLER *p_dk_socket_handler, DK_PROTOCAL_HANDLER *p_protocal_handler );

_u16 sh_get_udp_port( DK_SOCKET_HANDLER *p_dk_socket_handler );

//dk_request_node malloc/free
_int32 init_dk_request_node_slabs(void);
_int32 uninit_dk_request_node_slabs(void);
_int32 dk_request_node_malloc_wrap(DK_REQUEST_NODE **pp_node);
_int32 dk_request_node_init( DK_REQUEST_NODE *p_node, _u32 ip, _u16 port, 
	char *p_buffer, _u32 buffer_len, DK_SOCKET_HANDLER *p_dk_socket_handler );
_int32 dk_request_node_uninit( DK_REQUEST_NODE *p_node );
_int32 dk_request_node_free_wrap(DK_REQUEST_NODE *p_node);

#endif

#ifdef __cplusplus
}
#endif

#endif // !defined(__dk_socket_handler_20091015)






