#if !defined(__CMD_PROXY_20100402)
#define __CMD_PROXY_20100402

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/list.h"
#include "utility/define.h"
#include "utility/rw_sharebrd.h"
#include "platform/sd_socket.h"


typedef enum tagCMD_PROXY_TYPE
{
    TCP_CMD_PROXY,
        UDP_CMD_PROXY
} CMD_PROXY_TYPE;

typedef enum tagCMD_PROXY_SOCKET_STATE
{
    CMD_PROXY_SOCKET_IDLE,
        CMD_PROXY_SOCKET_CREATING,
        CMD_PROXY_SOCKET_WORKING,
        CMD_PROXY_SOCKET_CLOSEING,
        CMD_PROXY_SOCKET_CLOSED,
        CMD_PROXY_SOCKET_ERR
} CMD_PROXY_SOCKET_STATE;

typedef struct tagCMD_PROXY_RECV_INFO
{
    _u32 _recv_cmd_id;
    char* _buffer;
    _u32 _cmd_len;
}CMD_PROXY_RECV_INFO;


typedef struct tagCMD_INFO
{
	char*		_cmd_buffer_ptr;			//命令指针
	_u32		_cmd_len;			//命令长度
	_u32		_retry_times;
    BOOL        _is_format;
}CMD_INFO;

typedef struct tagCMD_PROXY
{
    _u32            _proxy_id;
    _u32            _socket_type;
	_u32            _attribute; 
    
    CMD_PROXY_SOCKET_STATE _socket_state;
	SOCKET			_socket;			//套接字

    _u32            _ip;
    _u16            _port;
    char*           _host;
    
    //send
	LIST			_cmd_list;			
	CMD_INFO*	    _last_cmd;			/*last command which is processing*/

    //recv
	char*			_recv_buffer;		//接收缓冲区
	_u32			_recv_buffer_len;	//接收缓冲区的长度
	_u32			_had_recv_len;		//已经接收到
	
    _u32            _recv_cmd_type;
    _u32            _recv_cmd_id;
    LIST            _recv_cmd_list;   //CMD_PROXY_RECV_INFO node
 
	_u32			_timeout_id;		/*when failed, it will start a timer and retry later*/ 
    BOOL            _close_flag;
    _int32          _err_code;
} CMD_PROXY;

_int32 cmd_proxy_create( _u32 type, _u32 attribute, CMD_PROXY **pp_cmd_proxy );
_int32 cmd_proxy_destroy( CMD_PROXY *p_cmd_proxy );

_int32 cmd_proxy_create_socket( CMD_PROXY *p_cmd_proxy );
_int32 cmd_proxy_connect_callback( _int32 errcode, _u32 pending_op_count, void* user_data );

_int32 cmd_proxy_send_update( CMD_PROXY *p_cmd_proxy );
_int32 cmd_proxy_send_last_cmd( CMD_PROXY *p_cmd_proxy );
_int32 cmd_proxy_send_callback( _int32 errcode, _u32 pending_op_count, const char* buffer, _u32 had_send, void* user_data );

_int32 cmd_proxy_recv_update( CMD_PROXY *p_cmd_proxy );
_int32 cmd_proxy_add_recv_cmd( CMD_PROXY *p_cmd_proxy, char *p_recv_cmd, _u32 cmd_len );
_int32 cmd_proxy_parse_recv_cmd( CMD_PROXY *p_cmd_proxy, char *buffer, _u32 had_recv );
_int32 cmd_proxy_recv_callback(_int32 errcode, _u32 pending_op_count, char* buffer, _u32 had_recv, void* user_data);
_int32 cmd_proxy_get_old_recv_info( CMD_PROXY *p_cmd_proxy, _u32 *p_recv_cmd_id, _u32 *p_data_buffer_len );

_int32 cmd_proxy_get_recv_info( CMD_PROXY *p_cmd_proxy, _u32 recv_cmd_id, _u32 data_buffer_len, CMD_PROXY_RECV_INFO **pp_recv_info );

void cmd_proxy_uninit_recv_info( CMD_PROXY_RECV_INFO *p_recv_info );
_int32 cmd_proxy_try_close_socket( CMD_PROXY *p_cmd_proxy );
_int32 cmd_proxy_final_close( CMD_PROXY *p_cmd_proxy );
_int32 cmd_proxy_free_cmd_info( CMD_INFO *p_cmd_info );

_int32 cmd_proxy_format_cmd( CMD_PROXY *p_cmd_proxy, CMD_INFO *p_cmd_info );
_int32 cmd_proxy_extend_recv_buffer( CMD_PROXY *p_cmd_proxy, _u32 total_len );
_int32 cmd_proxy_handle_err( CMD_PROXY *p_cmd_proxy, _int32 err_code );

_int32 cmd_proxy_enter_socket_state( CMD_PROXY *p_cmd_proxy, CMD_PROXY_SOCKET_STATE state );

_int32 cmd_proxy_build_http_header(char* buffer, _u32* len, _u32 data_len, char *addr, _u16 port );

#ifdef __cplusplus
}
#endif

#endif // !defined(__CMD_PROXY_20100402)



