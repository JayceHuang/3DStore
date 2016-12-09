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

/* ����΢������ʱ��������� (�����ϴ������ؼ�KB ��һ�����ݣ�һ��С�ļ���һ��СͼƬ����ҳ��)*/
typedef struct t_em_mini_task
{
	char* _url;					/* ֻ֧��"http://" ��ͷ��url  */
	_u32 _url_len;
	
	BOOL _is_file;					/* �Ƿ�Ϊ�ļ���TRUEΪ_file_path,_file_path_len,_file_name,_file_name_len��Ч,_data��_data_len��Ч��FALSE���෴*/
	
	char _file_path[MAX_FILE_PATH_LEN]; 				/* _is_file=TRUEʱ, ��Ҫ�ϴ������ص��ļ��洢·����������ʵ���� */
	_u32 _file_path_len;			/* _is_file=TRUEʱ, ��Ҫ�ϴ������ص��ļ��洢·���ĳ���*/
	char _file_name[MAX_FILE_NAME_BUFFER_LEN];			/* _is_file=TRUEʱ, ��Ҫ�ϴ������ص��ļ���*/
	_u32 _file_name_len; 			/* _is_file=TRUEʱ, ��Ҫ�ϴ������ص��ļ�������*/
	
	_u8 * _send_data;			/* _is_file=FALSEʱ,ָ����Ҫ�ϴ�������*/
	_u32  _send_data_len;			/* _is_file=FALSEʱ,��Ҫ�ϴ������ݳ��� */
	
	void * _recv_buffer;			/* ���ڽ��շ��������ص���Ӧ���ݵĻ��棬����ʱ���_is_file=TRUE,�˲�����Ч*/
	_u32  _recv_buffer_size;		/* _recv_buffer �����С������ʱ���_is_file=TRUE,�˲�����Ч */
	
	char * _send_header;			/* ���ڷ���httpͷ�� */
	_u32  _send_header_len;		/* _send_header�Ĵ�С	 */
	
	void *_callback_fun;			/* ������ɺ�Ļص����� : typedef int32 ( *ETM_NOTIFY_MINI_TASK_FINISHED)(void * user_data, int32 result,void * recv_buffer,u32  recvd_len,void * p_header,u8 * send_data); */
	void * _user_data;			/* �û����� */
	_u32  _timeout;				/* ��ʱ,��λ��*/
	_u32  _mini_id;				/* id */
	BOOL  _gzip;					/* �Ƿ���ܻ���ѹ���ļ� */
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
	char* 	_header_buffer; 			  	/*  ָ���socket�н��ܵ�����Ӧ������ʼ��ַ */
	_u32 	_header_buffer_length; 	  	/* ������Ӧ���ڴ泤�ȣ�ע�ⲻ����Ӧ�����ֽ��� */
	_u32	_header_buffer_end_pos;  	/* ���ܵ�����Ӧ�����ֽ�����Ӧ<= _header_buffer_length*/
	
	_u32  _status_code;
	BOOL 	_is_binary_file;				/* ���"Content-Type"��ֵΪ"text/html"���ֵΪtrue,����Ϊfalse */ 
	BOOL _is_chunked;/* "Transfer-Encoding" */ 
	char*	_location;  					/* "Location"��ָ���ض���url��_header_buffer�е���ʼ��ַ�����ĳ���Ϊ_location_len */
	_u64 	_entity_length;				/* "Content-Range" ,�����ļ��Ĵ�С������Ϊ0��_has_entity_lengthΪfalse���������û�з�����һ�ֶ�*/ 
	_u64 	_content_length;				/* "Content-Length" ,������Ӧ�������ݲ��ֵĴ�С,����Ϊ0��_has_content_lengthΪfalse���������û�з�����һ�ֶ� */ 
	BOOL _is_support_range;   /* "Content-Range" */
	char* 	_server_return_name;		/* "Content-Disposition" ��ָ���ļ�����_header_buffer�е���ʼ��ַ�����ĳ���Ϊ_server_return_name_len */
	BOOL _is_support_long_connect; /*"Connection" */
	char* 	_last_modified;  				/* ָ��last_modified��ֵ��_header_buffer�е���ʼ��ַ�����ĳ���Ϊ_last_modified_len */
	char* 	_cookie;   					/*  ָ��cookie��ֵ��_header_buffer�е���ʼ��ַ�����ĳ���Ϊ_cookie_len */
	char* 	_content_encoding;			/*  ָ��Content-Encoding��ֵ��_header_buffer�е���ʼ��ַ�����ĳ���Ϊ_content_encoding_len */

	char* 	_body_start_pos; 			/*  ��Ӧ����Ҳ�п��ܽ��ܵ�С�����ݣ����ָ��ָ�����ݿ�λ��_header_buffer�е���ʼ��ַ�����ĳ���Ϊ_body_len��ע�⣺����һ��_u8���͵����ݣ���һ�����ַ���������ʱ������strcpy���� ��Ҫ��memcpy*/
	_u32 	_body_len; 					/*   ��Ӧ���и������ܵ������ݵĳ��� */
	
	BOOL 	_has_entity_length;			/* ���Ϊfalse���������û�з�����һ�ֶ� */
	BOOL 	_has_content_length; 		/* ���Ϊfalse���������û�з�����һ�ֶ� */
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
	void * _user_data;			/* �û����� */
	EM_HTTP_CB_TYPE  _type;
	void * _header;				/* _type==EMHC_NOTIFY_RESPNʱ��Ч,ָ��http��Ӧͷ*/
	
	_u8 ** _send_data;			/* _type==EMHC_GET_SEND_DATAʱ��Ч, ���ݷֲ��ϴ�ʱ,ָ����Ҫ�ϴ�������*/
	_u32  * _send_data_len;		/* ��Ҫ�ϴ������ݳ��� */
	
	_u8 * _sent_data;			/* _type==EMHC_NOTIFY_SENT_DATAʱ��Ч, ���ݷֲ��ϴ�ʱ,ָ���Ѿ��ϴ�������*/
	_u32   _sent_data_len;		/* �Ѿ��ϴ������ݳ��� */
	
	void ** _recv_buffer;			/* _type==EMHC_GET_RECV_BUFFERʱ��Ч, ���ݷֲ�����ʱ,ָ�����ڽ������ݵĻ���*/
	_u32  * _recv_buffer_len;		/* �����С */
	
	_u8 * _recved_data;			/* _type==EMHC_PUT_RECVED_DATAʱ��Ч, ���ݷֲ�����ʱ,ָ���Ѿ��յ�������*/
	_u32  _recved_data_len;		/* �Ѿ��յ������ݳ��� */

	_int32 _result;					/* _type==EMHC_NOTIFY_FINISHEDʱ��Ч, 0Ϊ�ɹ�*/
} EM_HTTP_CALL_BACK;
typedef _int32 ( *EM_HTTP_CALL_BACK_FUNCTION)(EM_HTTP_CALL_BACK * p_em_http_call_back); 

/* HTTP�Ự�ӿڵ�������� */
typedef struct t_em_http_get
{
	char* _url;					/* ֻ֧��"http://" ��ͷ��url  */
	_u32 _url_len;
	
	char * _ref_url;				/* ����ҳ��URL*/
	_u32  _ref_url_len;		
	
	char * _cookie;			
	_u32  _cookie_len;		
	
	_u64  _range_from;			/* RANGE ��ʼλ��*/
	_u64  _range_to;			/* RANGE ����λ��*/
	
	BOOL  _accept_gzip;			/* �Ƿ����ѹ���ļ� */
	
	void * _recv_buffer;			/* ���ڽ��շ��������ص���Ӧ���ݵĻ���*/
	_u32  _recv_buffer_size;		/* _recv_buffer �����С*/
	
	void *_callback_fun;			/* ������ɺ�Ļص����� : typedef int32 ( *EM_HTTP_CALL_BACK_FUNCTION)(EM_HTTP_CALL_BACK * p_em_http_call_back); */
	void * _user_data;			/* �û����� */
	_u32  _timeout;				/* ��ʱ,��λ��,����0ʱ180�볬ʱ*/
} EM_HTTP_GET;
typedef struct t_em_http_post
{
	char* _url;					/* ֻ֧��"http://" ��ͷ��url  */
	_u32 _url_len;
	
	char * _ref_url;				/* ����ҳ��URL*/
	_u32  _ref_url_len;		
	
	char * _cookie;			
	_u32  _cookie_len;		
	
	_u64  _content_len;			/* Content-Length:*/
	
	BOOL  _send_gzip;			/* �Ƿ���ѹ������*/
	BOOL  _accept_gzip;			/* �Ƿ����ѹ������*/
	
	_u8 * _send_data;				/* ָ����Ҫ�ϴ�������*/
	_u32  _send_data_len;			/* ��Ҫ�ϴ������ݴ�С*/
	
	void * _recv_buffer;			/* ���ڽ��շ��������ص���Ӧ���ݵĻ���*/
	_u32  _recv_buffer_size;		/* _recv_buffer �����С*/

	void *_callback_fun;			/* ������ɺ�Ļص����� : typedef int32 ( *ETM_HTTP_CALL_BACK_FUNCTION)(ETM_HTTP_CALL_BACK * p_http_call_back_param); */
	void * _user_data;			/* �û����� */
	_u32  _timeout;				/* ��ʱ,��λ��,����0ʱ180�볬ʱ*/
} EM_HTTP_POST;
typedef struct t_em_http_get_file
{
	char* _url;					/* ֻ֧��"http://" ��ͷ��url  */
	_u32 _url_len;
	
	char * _ref_url;				/* ����ҳ��URL*/
	_u32  _ref_url_len;		
	
	char * _cookie;			
	_u32  _cookie_len;		
	
	_u64  _range_from;			/* RANGE ��ʼλ��,���ݲ�֧�֡�*/
	_u64  _range_to;			/* RANGE ����λ��,���ݲ�֧�֡�*/
	
	BOOL  _accept_gzip;			/* �Ƿ����ѹ���ļ� */
	
	char _file_path[MAX_FILE_PATH_LEN]; 				/* _is_file=TRUEʱ, ��Ҫ�ϴ������ص��ļ��洢·����������ʵ���� */
	_u32 _file_path_len;			/* _is_file=TRUEʱ, ��Ҫ�ϴ������ص��ļ��洢·���ĳ���*/
	char _file_name[MAX_FILE_NAME_BUFFER_LEN];			/* _is_file=TRUEʱ, ��Ҫ�ϴ������ص��ļ���*/
	_u32 _file_name_len; 			/* _is_file=TRUEʱ, ��Ҫ�ϴ������ص��ļ�������*/
	
	void *_callback_fun;			/* ������ɺ�Ļص����� : typedef int32 ( *ETM_HTTP_CALL_BACK_FUNCTION)(ETM_HTTP_CALL_BACK * p_http_call_back_param); */
	void * _user_data;			/* �û����� */
	_u32  _timeout;				/* ��ʱ,��λ��,����0ʱ180�볬ʱ*/
} EM_HTTP_GET_FILE;

typedef struct t_em_http_post_file
{
	char* _url;					/* ֻ֧��"http://" ��ͷ��url  */
	_u32 _url_len;
	
	char * _ref_url;				/* ����ҳ��URL*/
	_u32  _ref_url_len;		
	
	char * _cookie;			
	_u32  _cookie_len;		
	
	_u64  _range_from;			/* RANGE ��ʼλ��,���ݲ�֧�֡�*/
	_u64  _range_to;			/* RANGE ����λ��,���ݲ�֧�֡�*/
	
	BOOL  _accept_gzip;			/* �Ƿ����ѹ���ļ� */
	
	char _file_path[MAX_FILE_PATH_LEN]; 				/* _is_file=TRUEʱ, ��Ҫ�ϴ������ص��ļ��洢·����������ʵ���� */
	_u32 _file_path_len;			/* _is_file=TRUEʱ, ��Ҫ�ϴ������ص��ļ��洢·���ĳ���*/
	char _file_name[MAX_FILE_NAME_BUFFER_LEN];			/* _is_file=TRUEʱ, ��Ҫ�ϴ������ص��ļ���*/
	_u32 _file_name_len; 			/* _is_file=TRUEʱ, ��Ҫ�ϴ������ص��ļ�������*/
	
	void *_callback_fun;			/* ������ɺ�Ļص����� : typedef int32 ( *ETM_HTTP_CALL_BACK_FUNCTION)(ETM_HTTP_CALL_BACK * p_http_call_back_param); */
	void * _user_data;			/* �û����� */
	_u32  _timeout;				/* ��ʱ,��λ��,����0ʱ180�볬ʱ*/
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
