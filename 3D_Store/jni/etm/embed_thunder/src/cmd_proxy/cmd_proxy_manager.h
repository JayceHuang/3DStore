#if !defined(__CMD_PROXY_MANAGER_20100402)
#define __CMD_PROXY_MANAGER_20100402

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/list.h"
#include "utility/define.h"
#include "asyn_frame/notice.h"
#include "cmd_proxy/cmd_proxy.h"

struct tagCMD_PROXY;

typedef struct tagCMD_PROXY_MANAGER
{
    LIST _cmd_proxy_list;// CMD_PROXY nodes
} CMD_PROXY_MANAGER;



typedef struct tagPM_CREATE_CMD_PROXY
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_u32 _ip;
	_u16 _port;
	_u32 _attribute; 
	_u32 *_cmd_proxy_id_ptr;
} PM_CREATE_CMD_PROXY;

typedef struct tagPM_CREATE_CMD_PROXY_BY_DOMAIN
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	const char* _host;
	_u16 _port;
	_u32 _attribute; 
	_u32 *_cmd_proxy_id_ptr;
} PM_CREATE_CMD_PROXY_BY_DOMAIN;

typedef struct tagPM_CMD_PROXY_GET_INFO
{
	SEVENT_HANDLE _handle;
    _u32 _cmd_proxy_id;
	_int32 _result;
    _int32 *_errcode;
    _u32 *_recv_cmd_id;
    _u32 *_data_len;
} PM_CMD_PROXY_GET_INFO;

typedef struct tagPM_CMD_PROXY_SEND
{
	SEVENT_HANDLE _handle;
	_int32 _result;
    _u32 _cmd_proxy_id;
    const char* _buffer; 
    _u32 _len;
} PM_CMD_PROXY_SEND;

typedef struct tagPM_CMD_PROXY_GET_RECV_INFO
{
	SEVENT_HANDLE _handle;
	_int32 _result;
    _u32 _cmd_proxy_id;
    _u32 _recv_cmd_id;
    char *_data_buffer;
    _u32 _data_buffer_len;
} PM_CMD_PROXY_GET_RECV_INFO;

typedef struct tagPM_CMD_PROXY_CLOSE
{
	SEVENT_HANDLE _handle;
	_int32 _result;
    _u32 _cmd_proxy_id;
} PM_CMD_PROXY_CLOSE;

typedef struct tagGET_PEER_ID
{
	SEVENT_HANDLE _handle;
	_int32 _result;
    char   *_buffer;
    _u32   _buffer_len;
} PM_GET_PEER_ID;

_int32 init_cmd_proxy_module( void );
_int32 uninit_cmd_proxy_module( void );

_int32 pm_create_cmd_proxy( void *p_param );
_int32 pm_create_cmd_proxy_by_domain( void *p_param );

_int32 pm_cmd_proxy_get_info( void *p_param );

//support tcp
_int32 pm_cmd_proxy_send( void *p_param );
_int32 pm_cmd_proxy_get_recv_info( void *p_param );

_int32 pm_cmd_proxy_close( void *p_param );
void pm_get_cmd_proxy( _u32 cmd_proxy_id, struct tagCMD_PROXY **pp_cmd_proxy, BOOL is_remove );
_int32 pm_get_peer_id( void *p_param );


#ifdef __cplusplus
}
#endif

#endif // !defined(__CMD_PROXY_MANAGER_20100402)



