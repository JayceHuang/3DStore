#ifndef __HTTP_MINI_INTERFACE_H_20100701__
#define __HTTP_MINI_INTERFACE_H_20100701__

#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"
#include "utility/errcode.h"

#if 0
typedef enum t_http_resp_type
{
	HRT_STATUS_CODE = 0, 				
	HRT_HEADER,
	HRT_BODY,
	HRT_FINISHED
} HTTP_RESP_TYPE;

typedef struct t_http_resp_body
{
	const _u8 * _data;
	_u32 _data_len;
} HTTP_RESP_BODY;


typedef _int32 ( *http_resp_callback)(_u32 http_id,HTTP_RESP_TYPE type, void * resp);

typedef struct t_http_param
{
	const char * _url;
	_u32 _url_len;
	
	const _u8 * _data;	/* just for POST */
	_u32 _data_len;		/* just for POST */
	
	const char * _send_header;		/* 用于发送http头部 */
	_u32  _send_header_len;		/* _send_header的大小	 */
	
	BOOL  _gzip;					/* 是否接受或发送压缩文件 */
	http_resp_callback   resp_callback;
} HTTP_PARAM;
#endif
/* HTTP会话接口的输入参数 */
typedef struct t_http_param
{
	char* _url;					/* 只支持"http://" 开头的url  */
	_u32 _url_len;
	
	char * _ref_url;				/* 引用页面URL*/
	_u32  _ref_url_len;		
	
	char * _cookie;			
	_u32  _cookie_len;		
	
	_u64  _range_from;			/* RANGE 起始位置*/
	_u64  _range_to;			/* RANGE 结束位置*/
	
	_u64  _content_len;			/* Content-Length:*/
	
	BOOL  _send_gzip;			/* 是否发送压缩数据*/
	BOOL  _accept_gzip;			/* 是否接受压缩文件 */
	
	_u8 * _send_data;				/* 指向需要上传的数据*/
	_u32  _send_data_len;			/* 需要上传的数据大小*/

	void * _recv_buffer;			/* 用于接收服务器返回的响应数据的缓存*/
	_u32  _recv_buffer_size;		/* _recv_buffer 缓存大小*/
	
	void *_callback_fun;			/* 任务完成后的回调函数 : typedef int32 ( *ET_HTTP_CALL_BACK_FUNCTION)(ET_HTTP_CALL_BACK * p_http_call_back_param); */
	void * _user_data;			/* 用户参数 */
	_u32  _timeout;				/* 超时,单位秒,等于0时180秒超时*/
	_int32 _priority;				/* -1:低优先级,不会暂停下载任务; 0:普通优先级,会暂停下载任务;1:高优先级,会暂停所有其他的网络连接  */
} HTTP_PARAM;
/* HTTP会话接口的回调函数参数*/
typedef enum t_http_cb_type
{
	EHC_NOTIFY_RESPN=0, 
	EHC_GET_SEND_DATA, 	// Just for POST
	EHC_NOTIFY_SENT_DATA, 	// Just for POST
	EHC_GET_RECV_BUFFER,
	EHC_PUT_RECVED_DATA,
	EHC_NOTIFY_FINISHED
} HTTP_CB_TYPE;
typedef struct t_http_call_back
{
	_u32 _http_id;
	void * _user_data;			/* 用户参数 */
	HTTP_CB_TYPE _type;
	void * _header;				/* _type==EHCT_NOTIFY_RESPN时有效,指向http响应头,可用etm_get_http_header_value获取里面的详细信息*/
	
	_u8 ** _send_data;			/* _type==EHCT_GET_SEND_DATA时有效, 数据分步上传时,指向需要上传的数据*/
	_u32  * _send_data_len;		/* 需要上传的数据长度 */
	
	_u8 * _sent_data;			/* _type==EHCT_NOTIFY_SENT_DATA时有效, 数据分步上传时,指向已经上传的数据*/
	_u32   _sent_data_len;		/* 已经上传的数据长度 */
	
	void ** _recv_buffer;			/* _type==EHCT_GET_RECV_BUFFER时有效, 数据分步下载时,指向用于接收数据的缓存*/
	_u32  * _recv_buffer_len;		/* 缓存大小 */
	
	_u8 * _recved_data;			/* _type==EHCT_PUT_RECVED_DATA时有效, 数据分步下载时,指向已经收到的数据*/
	_u32  _recved_data_len;		/* 已经收到的数据长度 */

	_int32 _result;					/* _type==EHCT_NOTIFY_FINISHED时有效, 0为成功*/
} HTTP_CALL_BACK;
typedef _int32 ( *HTTP_CALL_BACK_FUNCTION)(HTTP_CALL_BACK * p_http_call_back_param); 


 _int32 init_mini_http(void);
 _int32 uninit_mini_http(void);
 
 _int32 sd_http_clear(void);

 _int32 sd_http_get(HTTP_PARAM * param,_u32 * http_id);

 _int32 sd_http_post(HTTP_PARAM * param,_u32 * http_id);

 _int32 sd_http_close(_u32 http_id);

_int32 sd_http_cancel(_u32 http_id);

#ifdef __cplusplus
}
#endif

#endif // __HTTP_MINI_INTERFACE_H_20100701__
