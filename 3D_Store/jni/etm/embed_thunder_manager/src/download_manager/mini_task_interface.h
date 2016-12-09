#if !defined(__MINI_TASK_H_20100420)
#define __MINI_TASK_H_20100420

#ifdef __cplusplus
extern "C" 
{
#endif

#include "em_common/em_define.h"
#include "em_common/em_errcode.h"

#include "em_common/em_list.h"
#include "em_common/em_map.h"
#include "settings/em_settings.h"
//#include "connect_manager/resource.h"
//#include "data_manager/data_manager_interface.h"
//#include "asyn_frame/msg.h"
//#include "asyn_frame/notice.h"


/************************************************************************/
/*                    STRUCT DEFINE                                     */
/************************************************************************/

/* 创建微型任务时的输入参数 (比如上传或下载几KB 的一块数据，一个小文件，一张小图片或网页等)*/
typedef struct t_em_mini_task
{
	char* _url;					/* 只支持"http://" 开头的url  */
	_u32 _url_len;
	
	BOOL _is_file;					/* 是否为文件，TRUE为_file_path,_file_path_len,_file_name,_file_name_len有效,_data和_data_len无效；FALSE则相反*/
	
	char _file_path[MAX_FILE_PATH_LEN]; 				/* _is_file=TRUE时, 需要上传或下载的文件存储路径，必须真实存在 */
	_u32 _file_path_len;			/* _is_file=TRUE时, 需要上传或下载的文件存储路径的长度*/
	char _file_name[MAX_FILE_NAME_BUFFER_LEN];			/* _is_file=TRUE时, 需要上传或下载的文件名*/
	_u32 _file_name_len; 			/* _is_file=TRUE时, 需要上传或下载的文件名长度*/
	
	_u8 * _send_data;			/* _is_file=FALSE时,指向需要上传的数据*/
	_u32  _send_data_len;			/* _is_file=FALSE时,需要上传的数据长度 */
	
	void * _recv_buffer;			/* 用于接收服务器返回的响应数据的缓存，下载时如果_is_file=TRUE,此参数无效*/
	_u32  _recv_buffer_size;		/* _recv_buffer 缓存大小，下载时如果_is_file=TRUE,此参数无效 */
	
	char * _send_header;			/* 用于发送http头部 */
	_u32  _send_header_len;		/* _send_header的大小	 */
	
	void *_callback_fun;			/* 任务完成后的回调函数 : typedef int32 ( *ETM_NOTIFY_MINI_TASK_FINISHED)(void * user_data, int32 result,void * recv_buffer,u32  recvd_len,void * p_header,u8 * send_data); */
	void * _user_data;			/* 用户参数 */
	_u32  _timeout;				/* 超时,单位秒*/
	_u32  _mini_id;				/* id */
	BOOL  _gzip;					/* 是否接受或发送压缩文件 */
} EM_MINI_TASK;

typedef struct t_mini_http_header
{
	_u32 	_status_code;
	char 	_last_modified[64];  
	char 	_cookie[1024]; 
	char 	_content_encoding[32];	
	_u64 	_content_length;
} MINI_HTTP_HEADER;

 typedef _int32 ( *EM_NOTIFY_MINI_TASK_FINISHED)(void * user_data, _int32 result,void * recv_buffer,_u32  recvd_len,void * p_header,_u8 * send_data);

//typedef enum t_mini_task_type
//{
//	MT_GET = 0, 				
//	MT_POST
//} MINI_TASK_TYPE;
typedef enum t_mini_task_state
{
	MTS_RUNNING = 0, 				
	MTS_FINISHED
} MINI_TASK_STATE;

typedef struct t_mini_task
{
	_u32 _task_id;
	//MINI_TASK_TYPE _type;
	MINI_TASK_STATE _state;
	_int32 _status_code;
	_int32 _failed_code;
	_u64 _data_recved;
	_u32 _file_id;
	_u32 _start_time;
	MINI_HTTP_HEADER _header;
	EM_MINI_TASK _task_info;
	BOOL _is_file;
} MINI_TASK;
#if 0
typedef enum t_http_resp_type
{
	HRT_STATUS_CODE = 0, 				
	HRT_HEADER,
	HRT_BODY,
	HRT_FINISHED
} HTTP_RESP_TYPE;

/* Structure for http response header       */
typedef struct 
{       
	char* 	_header_buffer; 			  	/*  指向从socket中接受到的响应包的起始地址 */
	_u32 	_header_buffer_length; 	  	/* 接受响应的内存长度，注意不是响应包的字节数 */
	_u32	_header_buffer_end_pos;  	/* 接受到的响应包的字节数，应<= _header_buffer_length*/
	
	_u32  _status_code;
	BOOL 	_is_binary_file;				/* 如果"Content-Type"的值为"text/html"则此值为true,否则为false */ 
	BOOL _is_chunked;/* "Transfer-Encoding" */ 
	char*	_location;  					/* "Location"，指向重定向url在_header_buffer中的起始地址，它的长度为_location_len */
	_u64 	_entity_length;				/* "Content-Range" ,整个文件的大小，若果为0且_has_entity_length为false，则服务器没有发回这一字段*/ 
	_u64 	_content_length;				/* "Content-Length" ,整个响应包中数据部分的大小,若果为0且_has_content_length为false，则服务器没有发回这一字段 */ 
	BOOL _is_support_range;   /* "Content-Range" */
	char* 	_server_return_name;		/* "Content-Disposition" ，指向文件名在_header_buffer中的起始地址，它的长度为_server_return_name_len */
	BOOL _is_support_long_connect; /*"Connection" */
	char* 	_last_modified;  				/* 指向last_modified的值在_header_buffer中的起始地址，它的长度为_last_modified_len */
	char* 	_cookie;   					/*  指向cookie的值在_header_buffer中的起始地址，它的长度为_cookie_len */
	char* 	_content_encoding;			/*  指向Content-Encoding的值在_header_buffer中的起始地址，它的长度为_content_encoding_len */

	char* 	_body_start_pos; 			/*  响应包中也有可能接受到小块数据，这个指针指向数据块位于_header_buffer中的起始地址，它的长度为_body_len。注意：这是一块_u8类型的数据，不一定是字符串，拷贝时不能用strcpy函数 ，要用memcpy*/
	_u32 	_body_len; 					/*   响应包中附带接受到的数据的长度 */
	
	BOOL 	_has_entity_length;			/* 如果为false，则服务器没有发回这一字段 */
	BOOL 	_has_content_length; 		/* 如果为false，则服务器没有发回这一字段 */
	_u32 	_location_len; /* the length of _location */
	_u32 	_server_return_name_len; /* the length of _server_return_name */
	_u32 	_last_modified_len; /* the length of _last_modified */
	_u32 	_cookie_len; /* the length of _cookie */
	_u32 	_content_encoding_len; /* the length of _content_encoding */

	BOOL 	_is_vnd_wap;  /*  just for cmwap */
}MINI_HTTP_RESPN_HEADER;
#endif

typedef enum t_em_http_cb_type
{
	EMHC_NOTIFY_RESPN=0, 
	EMHC_GET_SEND_DATA, 	// Just for POST
	EMHC_NOTIFY_SENT_DATA, 	// Just for POST
	EMHC_GET_RECV_BUFFER,
	EMHC_PUT_RECVED_DATA,
	EMHC_NOTIFY_FINISHED
} EM_HTTP_CB_TYPE;

typedef struct t_em_http_call_back
{
	_u32 _http_id;
	void * _user_data;			/* 用户参数 */
	EM_HTTP_CB_TYPE  _type;
	void * _header;				/* _type==EMHC_NOTIFY_RESPN时有效,指向http响应头*/
	
	_u8 ** _send_data;			/* _type==EMHC_GET_SEND_DATA时有效, 数据分步上传时,指向需要上传的数据*/
	_u32  * _send_data_len;		/* 需要上传的数据长度 */
	
	_u8 * _sent_data;			/* _type==EMHC_NOTIFY_SENT_DATA时有效, 数据分步上传时,指向已经上传的数据*/
	_u32   _sent_data_len;		/* 已经上传的数据长度 */
	
	void ** _recv_buffer;			/* _type==EMHC_GET_RECV_BUFFER时有效, 数据分步下载时,指向用于接收数据的缓存*/
	_u32  * _recv_buffer_len;		/* 缓存大小 */
	
	_u8 * _recved_data;			/* _type==EMHC_PUT_RECVED_DATA时有效, 数据分步下载时,指向已经收到的数据*/
	_u32  _recved_data_len;		/* 已经收到的数据长度 */

	_int32 _result;					/* _type==EMHC_NOTIFY_FINISHED时有效, 0为成功*/
} EM_HTTP_CALL_BACK;
typedef _int32 ( *EM_HTTP_CALL_BACK_FUNCTION)(EM_HTTP_CALL_BACK * p_em_http_call_back); 

/* HTTP会话接口的输入参数 */
typedef struct t_em_http_get
{
	char* _url;					/* 只支持"http://" 开头的url  */
	_u32 _url_len;
	
	char * _ref_url;				/* 引用页面URL*/
	_u32  _ref_url_len;		
	
	char * _cookie;			
	_u32  _cookie_len;		
	
	_u64  _range_from;			/* RANGE 起始位置*/
	_u64  _range_to;			/* RANGE 结束位置*/
	
	BOOL  _accept_gzip;			/* 是否接受压缩文件 */
	
	void * _recv_buffer;			/* 用于接收服务器返回的响应数据的缓存*/
	_u32  _recv_buffer_size;		/* _recv_buffer 缓存大小*/
	
	void *_callback_fun;			/* 任务完成后的回调函数 : typedef int32 ( *EM_HTTP_CALL_BACK_FUNCTION)(EM_HTTP_CALL_BACK * p_em_http_call_back); */
	void * _user_data;			/* 用户参数 */
	_u32  _timeout;				/* 超时,单位秒,等于0时180秒超时*/
} EM_HTTP_GET;
typedef struct t_em_http_post
{
	char* _url;					/* 只支持"http://" 开头的url  */
	_u32 _url_len;
	
	char * _ref_url;				/* 引用页面URL*/
	_u32  _ref_url_len;		
	
	char * _cookie;			
	_u32  _cookie_len;		
	
	_u64  _content_len;			/* Content-Length:*/
	
	BOOL  _send_gzip;			/* 是否发送压缩数据*/
	BOOL  _accept_gzip;			/* 是否接受压缩数据*/
	
	_u8 * _send_data;				/* 指向需要上传的数据*/
	_u32  _send_data_len;			/* 需要上传的数据大小*/
	
	void * _recv_buffer;			/* 用于接收服务器返回的响应数据的缓存*/
	_u32  _recv_buffer_size;		/* _recv_buffer 缓存大小*/

	void *_callback_fun;			/* 任务完成后的回调函数 : typedef int32 ( *ETM_HTTP_CALL_BACK_FUNCTION)(ETM_HTTP_CALL_BACK * p_http_call_back_param); */
	void * _user_data;			/* 用户参数 */
	_u32  _timeout;				/* 超时,单位秒,等于0时180秒超时*/
} EM_HTTP_POST;
typedef struct t_em_http_get_file
{
	char* _url;					/* 只支持"http://" 开头的url  */
	_u32 _url_len;
	
	char * _ref_url;				/* 引用页面URL*/
	_u32  _ref_url_len;		
	
	char * _cookie;			
	_u32  _cookie_len;		
	
	_u64  _range_from;			/* RANGE 起始位置,【暂不支持】*/
	_u64  _range_to;			/* RANGE 结束位置,【暂不支持】*/
	
	BOOL  _accept_gzip;			/* 是否接受压缩文件 */
	
	char _file_path[MAX_FILE_PATH_LEN]; 				/* _is_file=TRUE时, 需要上传或下载的文件存储路径，必须真实存在 */
	_u32 _file_path_len;			/* _is_file=TRUE时, 需要上传或下载的文件存储路径的长度*/
	char _file_name[MAX_FILE_NAME_BUFFER_LEN];			/* _is_file=TRUE时, 需要上传或下载的文件名*/
	_u32 _file_name_len; 			/* _is_file=TRUE时, 需要上传或下载的文件名长度*/
	
	void *_callback_fun;			/* 任务完成后的回调函数 : typedef int32 ( *ETM_HTTP_CALL_BACK_FUNCTION)(ETM_HTTP_CALL_BACK * p_http_call_back_param); */
	void * _user_data;			/* 用户参数 */
	_u32  _timeout;				/* 超时,单位秒,等于0时180秒超时*/
} EM_HTTP_GET_FILE;

typedef struct t_em_http_post_file
{
	char* _url;					/* 只支持"http://" 开头的url  */
	_u32 _url_len;
	
	char * _ref_url;				/* 引用页面URL*/
	_u32  _ref_url_len;		
	
	char * _cookie;			
	_u32  _cookie_len;		
	
	_u64  _range_from;			/* RANGE 起始位置,【暂不支持】*/
	_u64  _range_to;			/* RANGE 结束位置,【暂不支持】*/
	
	BOOL  _accept_gzip;			/* 是否接受压缩文件 */
	
	char _file_path[MAX_FILE_PATH_LEN]; 				/* _is_file=TRUE时, 需要上传或下载的文件存储路径，必须真实存在 */
	_u32 _file_path_len;			/* _is_file=TRUE时, 需要上传或下载的文件存储路径的长度*/
	char _file_name[MAX_FILE_NAME_BUFFER_LEN];			/* _is_file=TRUE时, 需要上传或下载的文件名*/
	_u32 _file_name_len; 			/* _is_file=TRUE时, 需要上传或下载的文件名长度*/
	
	void *_callback_fun;			/* 任务完成后的回调函数 : typedef int32 ( *ETM_HTTP_CALL_BACK_FUNCTION)(ETM_HTTP_CALL_BACK * p_http_call_back_param); */
	void * _user_data;			/* 用户参数 */
	_u32  _timeout;				/* 超时,单位秒,等于0时180秒超时*/
} EM_HTTP_POST_FILE;

/*--------------------------------------------------------------------------*/
/*                Interfaces provid for external program				        */
/*--------------------------------------------------------------------------*/

_int32 init_mini_task_module(void);
_int32 uninit_mini_task_module(void);
_int32 mini_id_comp(void *E1, void *E2);
_int32 mini_clear(void);
void mini_scheduler(void);
_int32 em_get_mini_file_from_url(void* p_param);
_int32 em_post_mini_file_to_url(void* p_param);
_int32 em_cancel_mini_task(void* p_param);

_int32 em_get_mini_file_from_url_impl( EM_MINI_TASK * p_task_param);
_int32 em_post_mini_file_to_url_impl(EM_MINI_TASK * p_task_param);
_int32 mini_delete_task(MINI_TASK * p_task );

_int32 mini_task_malloc(MINI_TASK **pp_slip);
_int32 mini_task_free(MINI_TASK * p_slip);

_int32 mini_limit_speed(void);
_int32 mini_unlimit_speed(void);


_int32 em_http_get_impl(EM_HTTP_GET * p_http_get ,_u32 * http_id,_int32 priority);
_int32 em_http_get(void* p_param);
_int32 em_http_get_file_impl(EM_HTTP_GET_FILE * p_http_get_file ,_u32 * http_id);
_int32 em_http_get_file(void* p_param);
_int32 em_http_post_impl(EM_HTTP_POST * p_http_post ,_u32 * http_id);
_int32 em_http_post(void* p_param);
_int32 em_http_post_file_impl(EM_HTTP_POST_FILE * p_http_post_file ,_u32 * http_id);
_int32 em_http_post_file(void* p_param);
_int32 em_http_cancel(void* p_param);

_int32 em_query_shub_by_url(void* p_param);
_int32 em_cancel_query_shub(void* p_param);

#ifdef __cplusplus
}
#endif


#endif // !defined(__DOWNLOAD_TASK_H_20080918)
