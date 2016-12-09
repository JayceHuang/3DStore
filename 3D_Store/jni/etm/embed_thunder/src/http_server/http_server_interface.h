
#if !defined(__HTTP_SERVER_INTERFACE_20090222)
#define __HTTP_SERVER_INTERFACE_20090222

#ifdef __cplusplus
extern "C" 
{
#endif

#include "platform/sd_socket.h"
#include "asyn_frame/notice.h"
#include "utility/define.h"
#include "utility/range.h"
#include "asyn_frame/msg.h"

typedef enum  tagHTTP_SERVER_STATE 
{
	HTTP_SERVER_STATE_IDLE, 
	HTTP_SERVER_STATE_GOT_REQUEST, 
	HTTP_SERVER_STATE_SENDING_HEADER, 
	HTTP_SERVER_STATE_SENDING_BODY, 
	HTTP_SERVER_STATE_RECVING_BODY,
	HTTP_SERVER_STATE_SENDING_ERR 
} HTTP_SERVER_STATE;

typedef struct tagHTTP_SERVER_ACCEPT_SOCKET_DATA
{
	SOCKET	_sock;
	char*	_buffer;
	_u32	_buffer_len;
	_u32	_buffer_offset;
	_u64    _fetch_file_pos;
	_u64    _fetch_file_length;
	_u32    _task_id;
	_u32    _file_index;
	_u32      _fetch_time_ms;
	HTTP_SERVER_STATE _state;
	BOOL 	_is_flv_seek;
	#ifdef ENABLE_FLASH_PLAYER
	//BOOL _is_header_sent;
	_u64    _real_file_size;
	_u64    _virtual_file_size;  //data->_fetch_file_length 
	_u64    _real_file_start_pos;
	_u64    _real_file_pos;
	_u64    _virtual_file_pos;  //data->_fetch_file_pos
	#endif /* ENABLE_FLASH_PLAYER */
	_int32 _post_type;  // 0:不是post;1:接受post普通文件;2:接受post 协议命令;3:等待UI传入响应文件;4:发送post协议命令的响应; -1 :未初始化
	_int32 _errcode;
	void * _user_data;

	BOOL _is_canceled;
}HTTP_SERVER_ACCEPT_SOCKET_DATA;


#define	RECV_HTTP_HEAD_BUFFER_LEN		1024
#define SEND_MOVIE_BUFFER_LENGTH          (1024*4*4)
#define SEND_MOVIE_BUFFER_LENGTH_LOOG          (1024*4*4*4)

_int32 init_vod_http_server_module(_u16 * port);

_int32 uninit_vod_http_server_module(void);

_int32 force_close_http_server_module_res(void);

_int32 http_vod_server_start(_u16 * port);

_int32 http_vod_server_stop(void);


typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_u16 * _port;
}HTTP_SERVER_START;


typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32 _result;
}HTTP_SERVER_STOP;

_int32 http_server_start_handle(_u16 * port);
_int32 http_server_stop_handle();

/**************************************************************/
//////////////////////////////////////////////////////////////
/////指定网段内搜索服务器的相关接口
//服务器协议类型
typedef enum t_server_type
{
         ST_HTTP=0, 
         ST_FTP            //暂不支持，主要用于以后扩展用
} SERVER_TYPE;

//服务器信息
typedef struct t_erver_info
{
         SERVER_TYPE _type;
         _u32 _ip;
         _u32 _port;
         char _description[256];  // 服务器描述，比如"version=xl7.xxx&pc_name; file_num=999; peerid=864046003239850V; ip=192.168.x.x; tcp_port=8080; udp_port=10373; peer_capability=61546"
} SERVER_INFO;

//找到服务器的回调函数类型定义
typedef _int32 ( *FIND_SERVER_CALL_BACK_FUNCTION)(SERVER_INFO * p_server);

//搜索完毕通知回调函数类型定义，result等于0为成功，其他值为失败
typedef _int32 ( *SEARCH_FINISHED_CALL_BACK_FUNCTION)(_int32 result);

//搜索输入参数
typedef struct t_search_server
{
         SERVER_TYPE _type;
         _u32 _ip_from;           /* 初始ip，本地序，如"192.168.1.1"为3232235777，填0表示只扫描与本地ip同一子网的主机 */
         _u32 _ip_to;               /* 结束ip，本地序，如"192.168.1.254"为3232236030，填0表示只扫描与本地ip同一子网的主机*/
         _int32 _ip_step;         /* 扫描步进*/
         
         _u32 _port_from;         /*初始端口，本地序*/
         _u32 _port_to;           /* 结束端口，本地序*/
         _int32 _port_step;    /* 端口扫描步进*/
         
         FIND_SERVER_CALL_BACK_FUNCTION _find_server_callback_fun;             /* 每找到一个服务器，et就回调一下该函数，将该服务器的信息传给UI */
         SEARCH_FINISHED_CALL_BACK_FUNCTION _search_finished_callback_fun;     /* 搜索完毕 */
} SEARCH_SERVER;

#define SEARCH_CONNECT_TIMEOUT			3000	// 3s
#define SEARCH_CONNECT_MAX_NUM			64		//每次搜索的最大创建连接数
#define SEARCH_CACHE_ADDRESS_MAX_NUM	20		//缓存最大地址数

/* 搜索状态 */
typedef enum t_search_state
{
	SEARCH_IDLE = 0,
	SEARCH_RUNNING, 
	SEARCH_FINISHED, 
	SEARCH_STOPPING
} SEARCH_STATE;

//当前开始搜索信息
typedef struct t_search_info
{
	_u32 _start_ip;           /* 初始ip，本地序，如"192.168.1.1"为3232235777，填0表示只扫描与本地ip同一子网的主机 */
	_u32 _end_ip;               /* 结束ip，本地序，如"192.168.1.254"为3232236030，填0表示只扫描与本地ip同一子网的主机*/
	_u32 _current_ip;
	_u32 _start_port;
	_u32 _end_port;
	_u32 _used_fd;
	_u32 _connected_num;
	SEARCH_STATE _search_state;
	FIND_SERVER_CALL_BACK_FUNCTION _find_server_callback;
	SEARCH_FINISHED_CALL_BACK_FUNCTION _search_finished_callback;
}SEARCH_INFO;
 
//单一连接信息
typedef struct t_single_connection
{
	SOCKET _sockfd;
	_u32 _ip;
	_u32 _port;
	_int32 _result;
	_u32 _had_recv_len;
	char* _recv_buf;
	FIND_SERVER_CALL_BACK_FUNCTION _callback_fun;
}SINGLE_CONNECTION;

// 缓存目标机器地址信息
typedef struct t_cache_address
{
	_u32 _ip;
	_u32 _port;
	BOOL _notified; /* 是否已经通知过界面，主要为了避免将一个server 多次通知给界面 -- Added by zyq @ 20120714 */
}CACHE_ADDRESS;

_int32 http_server_start_search_server( void * _param );
_int32 http_server_stop_search_server( void * _param );
_int32 http_server_restart_search_server( void * _param);
_int32 http_server_start_cache_poll(void);
_int32 http_server_start_local_poll(_u32 start_ip, _u32 end_ip, _u32 port);
_int32 http_server_single_connect(char* ip_addr, _u16 port, FIND_SERVER_CALL_BACK_FUNCTION callback_fun);
_int32 http_server_single_connect_callback(_int32 errcode, _u32 pending_op_count, void* user_data);
_int32 http_server_build_query_cmd(char** buffer, _u32* len, SINGLE_CONNECTION* p_connect);
_int32 http_server_single_send_callback(_int32 errcode, _u32 pending_op_count, const char* buffer, _u32 had_send, void* user_data);
_int32 http_server_single_recv_callback(_int32 errcode, _u32 pending_op_count, char* buffer, _u32 had_recv, void* user_data);
_int32 http_server_search_finished_callback();

//////////////////////////////////////////////////////////////
/////17 手机助手相关的http server

#ifdef ENABLE_ASSISTANT	
/* 客户端发来的xml 请求回调函数 */
typedef struct t_ma_req
{
         _u32 _ma_id;			/* 标识id,发送响应的时候需要用到 */
         char _req_file[1024];  /* xml请求文件的存放地址*/
} MA_REQ;
typedef _int32 ( *ASSISTANT_INCOM_REQ_CALLBACK)(MA_REQ * p_req_file);

/* 发送xml 响应的回调函数 */
typedef struct t_ma_resp_ret
{
	_u32 _ma_id;			/* 标识id,发送响应的时候需要用到 */
	void * _user_data;	/* 原样返回的用户参数 */
	_int32 _result;			/* "响应文件" 的发送结果 */
} MA_RESP_RET;
typedef _int32 ( *ASSISTANT_SEND_RESP_CALLBACK)(MA_RESP_RET * p_result);

/* 网下载库设置回调函数 */
_int32 assistant_set_callback_func_impl(ASSISTANT_INCOM_REQ_CALLBACK req_callback,ASSISTANT_SEND_RESP_CALLBACK resp_callback);

/* 发送响应 */
_int32 assistant_send_resp_file_impl(_u32 ma_id,const char * resp_file,void * user_data);

/* 取消异步操作 */
_int32 assistant_cancel_impl(_u32 ma_id);

/* 网下载库设置回调函数 */
_int32 assistant_set_callback_func(void * _param );

/* 发送响应 */
_int32 assistant_send_resp_file(void * _param );

/* 取消异步操作 */
_int32 assistant_cancel(void * _param );
#endif	/* ENABLE_ASSISTANT */



BOOL http_server_is_file_exist(char * file_name);
_int32 http_server_handle_get_local_file(HTTP_SERVER_ACCEPT_SOCKET_DATA* data, char * file_name,_u64 file_pos);
_int32 http_server_handle_http_complete_recv_callback(_int32 errcode, _u32 pending_op_count, char* buffer, _u32 had_recv, void* user_data);
_int32 http_server_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid);


#ifdef ENABLE_HTTP_SERVER


#define HS_ERR_200_OK (200)    //
#define HS_ERR_206_PARTIAL_CONTENT (206)    //206 Partial Content

#define HS_ERR_304_NOT_MODIFIED (304)    //未修改 — 未按预期修改文档。  

#define HS_ERR_400_BAD_REQUEST (400)    //400  错误请求 — 请求中有语法问题，或不能满足请求。  
#define HS_ERR_401_UNAUTHORIZED (401)    //401  未授权 — 未授权客户机访问数据。  
#define HS_ERR_403_FORBIDDEN (403)    //403  禁止 — 即使有授权也不需要访问。  
#define HS_ERR_404_NOT_FOUND (404)    //404  找不到 — 服务器找不到给定的资源；文档不存在。  
#define HS_ERR_405_METHOD_NOT_ALLOWED (405)    //405  资源被禁止
#define HS_ERR_406_NOT_ACCEPTABLE (406)    //406 无法接受
#define HS_ERR_408_REQUEST_TIME_OUT (408)    //408 请求超时
#define HS_ERR_409_CONFLICT (409)    //409 冲突
#define HS_ERR_410_GONE (410)    //410 永远不可用
#define HS_ERR_411_LENGTH_REQUIRED (411)    //411
#define HS_ERR_413_REQUEST_ENTITY_TOO_LARGE (413)    //413 请求实体太长
#define HS_ERR_414_REQUEST_URI_TOO_LARGE (414)    //414 请求URI太长
#define HS_ERR_415_UNSUPPORTED_MEDIA_TYPE (415)    //415  介质类型不受支持 — 服务器拒绝服务请求，因为不支持请求实体的格式。  
#define HS_ERR_416_REQUESTED_RANGE_NOT_SATISFIABLE (416)    //416 所请求的范围无法满足
#define HS_ERR_417_EXPECTATION_FAILED (417)    //417 执行失败

#define HS_ERR_500_INTERNAL_SERVER_ERROR (500)    //500  内部错误 — 因为意外情况，服务器不能完成请求。  
#define HS_ERR_501_NOT_IMPLEMENTED (501)    //501  未执行 — 服务器不支持请求的工具。  
#define HS_ERR_502_BAD_GETEWAY (502)    //502  错误网关 — 服务器接收到来自上游服务器的无效响应。  
#define HS_ERR_503_SERVICE_UNAVAILABLE (503)    //503  无法获得服务 — 由于临时过载或维护，服务器无法处理请求。
#define HS_ERR_504_GATEWAY_TIME_OUT (504)    //504
#define HS_ERR_505_HTTP_VERSION_NOT_SUPPORTED (505)    //505

#define HS_MAX_HS_INFO_LEN (1024)
#define HS_MAX_USER_NAME_LEN (64)
#define HS_MAX_PASSWORD_LEN (64)
#define HS_MAX_ENCRYPT_KEY_LEN (64)
#define HS_MAX_CONTENT_TYPE_LEN (64)
#define HS_MAX_FILE_NAME_LEN (512)		/* 文件名最大长度 */
#define HS_MAX_FILE_PATH_LEN (512)		/* 文件路径(不包括文件名)最大长度 */
#define HS_MAX_TD_CFG_SUFFIX_LEN (8)		/* 迅雷下载临时文件的后缀最大长度 */
#define HS_MAX_URL_LEN (1024)		/* URL 最大长度 */
#define HS_MAX_COOKIE_LEN HS_MAX_URL_LEN
#define HS_MAX_FILE_LEN_STRING 17
#define HS_MAX_DIR_LIST_NUM 0xff
#define MAX_ENCRYPT_KEY_LEN (64)
#define HS_TIME_STR_LEN (64)
#define FILENAME_EXTENSION ".tmp"
/* 动作类型 */
typedef enum t_hs_ac_type
{
	HST_RESP_SENT = 1600, 				/* 响应已发送,以下所有响应发送之后都会有这个回调*/	
	
	HST_REQ_SERVER_INFO , 				/* 有客户端请求查询服务器信息 */	
	HST_RESP_SERVER_INFO ,				/* 向客户端发送本http 服务器信息 */	
	
	HST_REQ_BIND, 						/* 有客户端请求绑定 */	
	HST_RESP_BIND, 						/* 向客户端发送绑定响应 */	
	
	HST_REQ_GET_FILE, 					/* 有客户端请求下载文件*/	
	HST_RESP_GET_FILE, 				/* 向下载库授权传输文件*/	
	HST_NOTIFY_TRANSFER_FILE_PROGRESS, 	/* 发送文件进度通知*/	

	HST_REQ_POST_FILE, 				/* 有客户端请求上传文件*/	
	HST_RESP_POST_FILE, 				/* 向下载库授权接受文件*/	

	HST_REQ_GET, 						/* 有客户端发送未知的GET 请求过来(用文件传递body)*/	
	HST_RESP_GET, 						/* 向客户端发送未知的GET 请求响应(用文件传递body) */	

	HST_REQ_POST, 						/* 有客户端发送未知的POST 请求过来(用文件传递body)*/	
	HST_RESP_POST, 						/* 向客户端发送未知的POST 请求响应(用文件传递body) */	


} HS_AC_TYPE;
typedef struct t_hs_path
{
	char _real_path[MAX_LONG_FULL_PATH_BUFFER_LEN];   
	char _virtual_path[MAX_LONG_FULL_PATH_BUFFER_LEN];
	BOOL _need_auth;
} HS_PATH;

typedef struct t_hs_ac_data
{
	HTTP_SERVER_STATE _state;
	SOCKET	_sock;
	_u32    _task_id;
	_u32    _file_index;   //打开文件读取数据时保存的文件id(点播bt任务时，为bt子任务的索引号)
	
	char 	_buffer[SEND_MOVIE_BUFFER_LENGTH_LOOG]; // 发送或接收数据的缓冲区
	_u32	_buffer_len;         // 发送或接收客户端数据的长度
	_u32	_buffer_offset;     // 已经接收或发送的字节数
	_u64    _fetch_file_pos;    // 发送时为文件读取的位置，接收时为起始写入文件的位置
	_u64    _fetch_file_length;// 发送时为剩余需要发送的大小，接收时为剩余需要接收的长度
	_u64    _range_from;    // 文件中本会话需要传输的开始位置
	_u64    _range_to;// 文件中本会话需要传输的结束位置
	_u64    _file_size;// 文件总大小
	_u32      _fetch_time_ms;
	
	//HS_FLV * _flv;
	
	//_int32 _post_type;  // 0:不是post;1:接受post普通文件;2:接受post 协议命令;3:等待UI传入响应文件;4:发送post协议命令的响应; -1 :未初始化
	_int32 _errcode;
	//void * _user_data;

	BOOL _is_canceled;
	BOOL _need_cb;			/* 关闭action时是否需要回调通知界面 */

	HS_PATH _req_path;
	char _ui_resp_file_path[MAX_LONG_FULL_PATH_BUFFER_LEN];

	void * _action_ptr;
	BOOL  _is_gzip;						/* 发送文件时是否压缩文件 */
	char  _encrypt_key[MAX_ENCRYPT_KEY_LEN];		/* 发送文件时是否加密, 下载库将会用该key对文件内容进行加密，为空则不加密*/
}HS_AC_DATA;
	
/* 基本动作 */
typedef struct t_hs_ac_lite
{
	_u32 _ac_size;		/* 整个动作结构体的大小,如果_type= ECT_NB_LOGIN,则_cb_size=sizeof(ETM_NB_CB)，如果_type= ECT_NB_GET_FILE_LS,则_cb_size=sizeof(ENB_FILE_LIST_CB)+_file_num*ENB_MAX_REAL_NAME_LEN */
	HS_AC_TYPE _type;
	_u32 _action_id;		/* 动作id,发送响应的时候需要与对应的请求的动作id一致 */
	void * _user_data;	/*	用户参数,如果是请求，该值为空*/
	_int32 _result;			/*	0:成功；其他值:失败*/
} HS_AC_LITE;

typedef struct t_hs_ac
{
	HS_AC_LITE _action;			/*	0:成功；其他值:失败*/

	HS_AC_DATA _data;
} HS_AC;

/* 客户端请求查询服务器信息 */	
typedef struct t_hs_incom_req_server_info
{
	HS_AC_LITE _ac;						/* EHS_AC_TYPE _type = EAT_HS_REQ_SERVER_INFO */
	char _client_info[HS_MAX_HS_INFO_LEN];	/* 客户端信息 */
} HS_INCOM_REQ_SERVER_INFO;

/*  向客户端发送本http 服务器信息 */
typedef struct t_hs_resp_server_info
{
	HS_AC_LITE _ac;						/* EHS_AC_TYPE _type = EAT_HS_RESP_SERVER_INFO */
	char _server_info[HS_MAX_HS_INFO_LEN];	/* 本http 服务器信息  */
} HS_RESP_SERVER_INFO;

/////////////////////////////////////////////////////////////////////////////

/* 客户端请求绑定 */	
typedef struct t_hs_incom_req_bind
{
	HS_AC_LITE _ac;								/* EHS_AC_TYPE _type = EAT_HS_REQ_BIND */
	char _user_name[HS_MAX_USER_NAME_LEN];	
	char _password[HS_MAX_PASSWORD_LEN];		
} HS_INCOM_REQ_BIND;

/*  向客户端发送绑定结果 */
typedef struct t_hs_resp_bind
{
	HS_AC_LITE _ac;						/* EHS_AC_TYPE _type = EAT_HS_RESP_BIND,_result=0或200 ok/400 拒绝 */
} HS_RESP_BIND;

/////////////////////////////////////////////////////////////////////////////

/* 客户端请求下载文件 */	
typedef struct t_hs_incom_req_get_file
{
	HS_AC_LITE _ac;								/* EHS_AC_TYPE _type = EAT_HS_REQ_GET_FILE */
	char _user_name[HS_MAX_USER_NAME_LEN];	
	char _file_path[HS_MAX_FILE_PATH_LEN];		/* http server 中的真实路径 */
	char _file_name[HS_MAX_FILE_NAME_LEN];		/* 如果文件名为空，则表示获取路径下的文件列表 */	
	char _cookie[HS_MAX_COOKIE_LEN];
	
	_u64  _range_from;			/* RANGE 起始位置*/
	_u64  _range_to;			/* RANGE 结束位置*/
	
	BOOL  _accept_gzip;			/* 是否接受压缩文件 */
} HS_INCOM_REQ_GET_FILE;

/*  向下载库授权传输文件*/
typedef struct t_hs_resp_get_file
{
	HS_AC_LITE _ac;						/* EHS_AC_TYPE _type = EAT_HS_RESP_GET_FILE ,_result = 0或200或206为接受请求,否则拒绝*/
	BOOL  _is_gzip;						/* 发送文件时是否压缩文件 */
	char  _encrypt_key[HS_MAX_ENCRYPT_KEY_LEN];		/* 发送文件时是否加密, 下载库将会用该key对文件内容进行加密，为空则不加密*/
	//char _content_type[ETM_MAX_CONTENT_TYPE_LEN];
}HS_RESP_GET_FILE;

/* 传输文件进度通知*/	
typedef struct t_hs_notify_transfer_file_progress
{
	HS_AC_LITE _ac;								/* EHS_AC_TYPE _type = EAT_HS_NOTIFY_TRANSFER_FILE_PROGRESS */
	
	_u64  _file_size;								/* 文件总大小*/
	_u64  _range_size;							/* 该请求需要发送的内容大小*/
	_u64  _transfer_size;						/* 已传输的大小*/
} HS_NOTIFY_TRANSFER_FILE_PROGRESS;

/////////////////////////////////////////////////////////////////////////////

/* 客户端请求上传文件 */	
typedef struct t_hs_incom_req_post_file
{
	HS_AC_LITE _ac;								/* EHS_AC_TYPE _type = EAT_HS_REQ_POST_FILE */
	char _user_name[HS_MAX_USER_NAME_LEN];	
	char _file_path[HS_MAX_FILE_PATH_LEN];		/* http server 中的真实路径 */
	char _file_name[HS_MAX_FILE_NAME_LEN];		/* 如果文件名为空，则表示需要创建文件夹 */	
	char _cookie[HS_MAX_COOKIE_LEN];
	char  _encrypt_key[HS_MAX_ENCRYPT_KEY_LEN];		/* 文件是否加密, 下载库将会用该key对文件内容进行解密，为空则无需解密*/
	_u64  _file_size;				/* Content-Length:*/
	
	_u64  _range_from;			/* RANGE 起始位置*/
	_u64  _range_to;			/* RANGE 结束位置*/
	
	BOOL  _is_gzip;			/* 文件内容是否已压缩*/
} HS_INCOM_REQ_POST_FILE;

/*  向下载库授权接收文件*/
typedef struct t_hs_resp_post_file
{
	HS_AC_LITE _ac;						/* EHS_AC_TYPE _type = EAT_HS_RESP_POST_FILE ,_result = 0或200或206为接受请求,否则拒绝*/
	BOOL _is_break_transfer;           /* 是否断点续传*/
} HS_RESP_POST_FILE;



typedef enum  t_hs_state 
{
	HS_IDLE, 
	HS_RUNNING, 
	HS_STOPPING 
} HS_STATE;



typedef struct t_hs_account
{
	char _user_name[MAX_USER_NAME_LEN];   
	char _password[MAX_PASSWORD_LEN];
	BOOL _auth;
} HS_ACCOUNT;

typedef _int32 ( *HTTP_SERVER_CALLBACK)(HS_AC_LITE * p_param);

typedef struct t_http_server_manager
{
	HS_STATE _state;
	SOCKET _accept_sock;
	_u32 _port;
	HS_ACCOUNT _account;
	_u32 _action_id_num;			/* 从1开始累加，用于生成action_id */
	LIST _path_list;    				/* 根目录下的虚拟目录列表, HS_PATH */
	MAP _actions;					/* 存放所有的动作，HS_AC */
	_u32 _resp_timer_id;
	
	_u32 _pasued_posting_records_file_id;

	HTTP_SERVER_CALLBACK _http_server_callback_function;
} HTTP_SERVER_MANAGER;



_int32 http_server_add_account( const  char * name,const char * password );

_int32 http_server_add_path(const  char * real_path,const char * virtual_path,BOOL need_auth);

_int32 http_server_get_path_list(const char * virtual_path,char ** path_array_buffer,_u32 * path_num);

_int32 http_server_set_callback_func(HTTP_SERVER_CALLBACK callback);

_int32 http_server_send_resp(HS_AC_LITE * p_param);

_int32 http_server_cancel(_u32 action_id);

/* 添加帐号，目前最多只支持一个 */
_int32 http_server_add_account_handle( void *_param );
/* 往http 服务器添加虚拟路径或文件 
比如:real_path="/mnt/sdcard/tddownload",virtual_path="my_dir",这样就会把tddownload这个文件夹映射到http 服务器根目录的my_dir中,
need_auth表示访问该文件或文件夹时是否需要用户密码认证*/
_int32 http_server_add_path_handle( void *_param );
/* 获取http server中的文件列表
比如virtual_path="./",path_array_buffer = buffer[10][ETM_MAX_FILE_NAME_LEN],*path_num =  10,表示获取根目录下前十个文件或目录的名字*/
_int32 http_server_get_path_list_handle( void *_param );
/* 往下载库设置回调函数 */
_int32 http_server_set_callback_func_handle( void *_param );
/* 发送响应 */
_int32 http_server_send_resp_handle( void *_param );
/* 取消某个异步操作 */
_int32 http_server_cancel_handle( void *_param );


_int32 hs_id_comp(void *E1, void *E2);


#endif /* ENABLE_HTTP_SERVER */

#ifdef __cplusplus
}
#endif

#endif  /*__HTTP_SERVER_INTERFACE_20090222*/
