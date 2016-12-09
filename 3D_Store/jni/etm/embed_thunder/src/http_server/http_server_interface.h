
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
	_int32 _post_type;  // 0:����post;1:����post��ͨ�ļ�;2:����post Э������;3:�ȴ�UI������Ӧ�ļ�;4:����postЭ���������Ӧ; -1 :δ��ʼ��
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
/////ָ����������������������ؽӿ�
//������Э������
typedef enum t_server_type
{
         ST_HTTP=0, 
         ST_FTP            //�ݲ�֧�֣���Ҫ�����Ժ���չ��
} SERVER_TYPE;

//��������Ϣ
typedef struct t_erver_info
{
         SERVER_TYPE _type;
         _u32 _ip;
         _u32 _port;
         char _description[256];  // ����������������"version=xl7.xxx&pc_name; file_num=999; peerid=864046003239850V; ip=192.168.x.x; tcp_port=8080; udp_port=10373; peer_capability=61546"
} SERVER_INFO;

//�ҵ��������Ļص��������Ͷ���
typedef _int32 ( *FIND_SERVER_CALL_BACK_FUNCTION)(SERVER_INFO * p_server);

//�������֪ͨ�ص��������Ͷ��壬result����0Ϊ�ɹ�������ֵΪʧ��
typedef _int32 ( *SEARCH_FINISHED_CALL_BACK_FUNCTION)(_int32 result);

//�����������
typedef struct t_search_server
{
         SERVER_TYPE _type;
         _u32 _ip_from;           /* ��ʼip����������"192.168.1.1"Ϊ3232235777����0��ʾֻɨ���뱾��ipͬһ���������� */
         _u32 _ip_to;               /* ����ip����������"192.168.1.254"Ϊ3232236030����0��ʾֻɨ���뱾��ipͬһ����������*/
         _int32 _ip_step;         /* ɨ�貽��*/
         
         _u32 _port_from;         /*��ʼ�˿ڣ�������*/
         _u32 _port_to;           /* �����˿ڣ�������*/
         _int32 _port_step;    /* �˿�ɨ�貽��*/
         
         FIND_SERVER_CALL_BACK_FUNCTION _find_server_callback_fun;             /* ÿ�ҵ�һ����������et�ͻص�һ�¸ú��������÷���������Ϣ����UI */
         SEARCH_FINISHED_CALL_BACK_FUNCTION _search_finished_callback_fun;     /* ������� */
} SEARCH_SERVER;

#define SEARCH_CONNECT_TIMEOUT			3000	// 3s
#define SEARCH_CONNECT_MAX_NUM			64		//ÿ����������󴴽�������
#define SEARCH_CACHE_ADDRESS_MAX_NUM	20		//��������ַ��

/* ����״̬ */
typedef enum t_search_state
{
	SEARCH_IDLE = 0,
	SEARCH_RUNNING, 
	SEARCH_FINISHED, 
	SEARCH_STOPPING
} SEARCH_STATE;

//��ǰ��ʼ������Ϣ
typedef struct t_search_info
{
	_u32 _start_ip;           /* ��ʼip����������"192.168.1.1"Ϊ3232235777����0��ʾֻɨ���뱾��ipͬһ���������� */
	_u32 _end_ip;               /* ����ip����������"192.168.1.254"Ϊ3232236030����0��ʾֻɨ���뱾��ipͬһ����������*/
	_u32 _current_ip;
	_u32 _start_port;
	_u32 _end_port;
	_u32 _used_fd;
	_u32 _connected_num;
	SEARCH_STATE _search_state;
	FIND_SERVER_CALL_BACK_FUNCTION _find_server_callback;
	SEARCH_FINISHED_CALL_BACK_FUNCTION _search_finished_callback;
}SEARCH_INFO;
 
//��һ������Ϣ
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

// ����Ŀ�������ַ��Ϣ
typedef struct t_cache_address
{
	_u32 _ip;
	_u32 _port;
	BOOL _notified; /* �Ƿ��Ѿ�֪ͨ�����棬��ҪΪ�˱��⽫һ��server ���֪ͨ������ -- Added by zyq @ 20120714 */
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
/////17 �ֻ�������ص�http server

#ifdef ENABLE_ASSISTANT	
/* �ͻ��˷�����xml ����ص����� */
typedef struct t_ma_req
{
         _u32 _ma_id;			/* ��ʶid,������Ӧ��ʱ����Ҫ�õ� */
         char _req_file[1024];  /* xml�����ļ��Ĵ�ŵ�ַ*/
} MA_REQ;
typedef _int32 ( *ASSISTANT_INCOM_REQ_CALLBACK)(MA_REQ * p_req_file);

/* ����xml ��Ӧ�Ļص����� */
typedef struct t_ma_resp_ret
{
	_u32 _ma_id;			/* ��ʶid,������Ӧ��ʱ����Ҫ�õ� */
	void * _user_data;	/* ԭ�����ص��û����� */
	_int32 _result;			/* "��Ӧ�ļ�" �ķ��ͽ�� */
} MA_RESP_RET;
typedef _int32 ( *ASSISTANT_SEND_RESP_CALLBACK)(MA_RESP_RET * p_result);

/* �����ؿ����ûص����� */
_int32 assistant_set_callback_func_impl(ASSISTANT_INCOM_REQ_CALLBACK req_callback,ASSISTANT_SEND_RESP_CALLBACK resp_callback);

/* ������Ӧ */
_int32 assistant_send_resp_file_impl(_u32 ma_id,const char * resp_file,void * user_data);

/* ȡ���첽���� */
_int32 assistant_cancel_impl(_u32 ma_id);

/* �����ؿ����ûص����� */
_int32 assistant_set_callback_func(void * _param );

/* ������Ӧ */
_int32 assistant_send_resp_file(void * _param );

/* ȡ���첽���� */
_int32 assistant_cancel(void * _param );
#endif	/* ENABLE_ASSISTANT */



BOOL http_server_is_file_exist(char * file_name);
_int32 http_server_handle_get_local_file(HTTP_SERVER_ACCEPT_SOCKET_DATA* data, char * file_name,_u64 file_pos);
_int32 http_server_handle_http_complete_recv_callback(_int32 errcode, _u32 pending_op_count, char* buffer, _u32 had_recv, void* user_data);
_int32 http_server_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid);


#ifdef ENABLE_HTTP_SERVER


#define HS_ERR_200_OK (200)    //
#define HS_ERR_206_PARTIAL_CONTENT (206)    //206 Partial Content

#define HS_ERR_304_NOT_MODIFIED (304)    //δ�޸� �� δ��Ԥ���޸��ĵ���  

#define HS_ERR_400_BAD_REQUEST (400)    //400  �������� �� ���������﷨���⣬������������  
#define HS_ERR_401_UNAUTHORIZED (401)    //401  δ��Ȩ �� δ��Ȩ�ͻ����������ݡ�  
#define HS_ERR_403_FORBIDDEN (403)    //403  ��ֹ �� ��ʹ����ȨҲ����Ҫ���ʡ�  
#define HS_ERR_404_NOT_FOUND (404)    //404  �Ҳ��� �� �������Ҳ�����������Դ���ĵ������ڡ�  
#define HS_ERR_405_METHOD_NOT_ALLOWED (405)    //405  ��Դ����ֹ
#define HS_ERR_406_NOT_ACCEPTABLE (406)    //406 �޷�����
#define HS_ERR_408_REQUEST_TIME_OUT (408)    //408 ����ʱ
#define HS_ERR_409_CONFLICT (409)    //409 ��ͻ
#define HS_ERR_410_GONE (410)    //410 ��Զ������
#define HS_ERR_411_LENGTH_REQUIRED (411)    //411
#define HS_ERR_413_REQUEST_ENTITY_TOO_LARGE (413)    //413 ����ʵ��̫��
#define HS_ERR_414_REQUEST_URI_TOO_LARGE (414)    //414 ����URI̫��
#define HS_ERR_415_UNSUPPORTED_MEDIA_TYPE (415)    //415  �������Ͳ���֧�� �� �������ܾ�����������Ϊ��֧������ʵ��ĸ�ʽ��  
#define HS_ERR_416_REQUESTED_RANGE_NOT_SATISFIABLE (416)    //416 ������ķ�Χ�޷�����
#define HS_ERR_417_EXPECTATION_FAILED (417)    //417 ִ��ʧ��

#define HS_ERR_500_INTERNAL_SERVER_ERROR (500)    //500  �ڲ����� �� ��Ϊ��������������������������  
#define HS_ERR_501_NOT_IMPLEMENTED (501)    //501  δִ�� �� ��������֧������Ĺ��ߡ�  
#define HS_ERR_502_BAD_GETEWAY (502)    //502  �������� �� ���������յ��������η���������Ч��Ӧ��  
#define HS_ERR_503_SERVICE_UNAVAILABLE (503)    //503  �޷���÷��� �� ������ʱ���ػ�ά�����������޷���������
#define HS_ERR_504_GATEWAY_TIME_OUT (504)    //504
#define HS_ERR_505_HTTP_VERSION_NOT_SUPPORTED (505)    //505

#define HS_MAX_HS_INFO_LEN (1024)
#define HS_MAX_USER_NAME_LEN (64)
#define HS_MAX_PASSWORD_LEN (64)
#define HS_MAX_ENCRYPT_KEY_LEN (64)
#define HS_MAX_CONTENT_TYPE_LEN (64)
#define HS_MAX_FILE_NAME_LEN (512)		/* �ļ�����󳤶� */
#define HS_MAX_FILE_PATH_LEN (512)		/* �ļ�·��(�������ļ���)��󳤶� */
#define HS_MAX_TD_CFG_SUFFIX_LEN (8)		/* Ѹ��������ʱ�ļ��ĺ�׺��󳤶� */
#define HS_MAX_URL_LEN (1024)		/* URL ��󳤶� */
#define HS_MAX_COOKIE_LEN HS_MAX_URL_LEN
#define HS_MAX_FILE_LEN_STRING 17
#define HS_MAX_DIR_LIST_NUM 0xff
#define MAX_ENCRYPT_KEY_LEN (64)
#define HS_TIME_STR_LEN (64)
#define FILENAME_EXTENSION ".tmp"
/* �������� */
typedef enum t_hs_ac_type
{
	HST_RESP_SENT = 1600, 				/* ��Ӧ�ѷ���,����������Ӧ����֮�󶼻�������ص�*/	
	
	HST_REQ_SERVER_INFO , 				/* �пͻ��������ѯ��������Ϣ */	
	HST_RESP_SERVER_INFO ,				/* ��ͻ��˷��ͱ�http ��������Ϣ */	
	
	HST_REQ_BIND, 						/* �пͻ�������� */	
	HST_RESP_BIND, 						/* ��ͻ��˷��Ͱ���Ӧ */	
	
	HST_REQ_GET_FILE, 					/* �пͻ������������ļ�*/	
	HST_RESP_GET_FILE, 				/* �����ؿ���Ȩ�����ļ�*/	
	HST_NOTIFY_TRANSFER_FILE_PROGRESS, 	/* �����ļ�����֪ͨ*/	

	HST_REQ_POST_FILE, 				/* �пͻ��������ϴ��ļ�*/	
	HST_RESP_POST_FILE, 				/* �����ؿ���Ȩ�����ļ�*/	

	HST_REQ_GET, 						/* �пͻ��˷���δ֪��GET �������(���ļ�����body)*/	
	HST_RESP_GET, 						/* ��ͻ��˷���δ֪��GET ������Ӧ(���ļ�����body) */	

	HST_REQ_POST, 						/* �пͻ��˷���δ֪��POST �������(���ļ�����body)*/	
	HST_RESP_POST, 						/* ��ͻ��˷���δ֪��POST ������Ӧ(���ļ�����body) */	


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
	_u32    _file_index;   //���ļ���ȡ����ʱ������ļ�id(�㲥bt����ʱ��Ϊbt�������������)
	
	char 	_buffer[SEND_MOVIE_BUFFER_LENGTH_LOOG]; // ���ͻ�������ݵĻ�����
	_u32	_buffer_len;         // ���ͻ���տͻ������ݵĳ���
	_u32	_buffer_offset;     // �Ѿ����ջ��͵��ֽ���
	_u64    _fetch_file_pos;    // ����ʱΪ�ļ���ȡ��λ�ã�����ʱΪ��ʼд���ļ���λ��
	_u64    _fetch_file_length;// ����ʱΪʣ����Ҫ���͵Ĵ�С������ʱΪʣ����Ҫ���յĳ���
	_u64    _range_from;    // �ļ��б��Ự��Ҫ����Ŀ�ʼλ��
	_u64    _range_to;// �ļ��б��Ự��Ҫ����Ľ���λ��
	_u64    _file_size;// �ļ��ܴ�С
	_u32      _fetch_time_ms;
	
	//HS_FLV * _flv;
	
	//_int32 _post_type;  // 0:����post;1:����post��ͨ�ļ�;2:����post Э������;3:�ȴ�UI������Ӧ�ļ�;4:����postЭ���������Ӧ; -1 :δ��ʼ��
	_int32 _errcode;
	//void * _user_data;

	BOOL _is_canceled;
	BOOL _need_cb;			/* �ر�actionʱ�Ƿ���Ҫ�ص�֪ͨ���� */

	HS_PATH _req_path;
	char _ui_resp_file_path[MAX_LONG_FULL_PATH_BUFFER_LEN];

	void * _action_ptr;
	BOOL  _is_gzip;						/* �����ļ�ʱ�Ƿ�ѹ���ļ� */
	char  _encrypt_key[MAX_ENCRYPT_KEY_LEN];		/* �����ļ�ʱ�Ƿ����, ���ؿ⽫���ø�key���ļ����ݽ��м��ܣ�Ϊ���򲻼���*/
}HS_AC_DATA;
	
/* �������� */
typedef struct t_hs_ac_lite
{
	_u32 _ac_size;		/* ���������ṹ��Ĵ�С,���_type= ECT_NB_LOGIN,��_cb_size=sizeof(ETM_NB_CB)�����_type= ECT_NB_GET_FILE_LS,��_cb_size=sizeof(ENB_FILE_LIST_CB)+_file_num*ENB_MAX_REAL_NAME_LEN */
	HS_AC_TYPE _type;
	_u32 _action_id;		/* ����id,������Ӧ��ʱ����Ҫ���Ӧ������Ķ���idһ�� */
	void * _user_data;	/*	�û�����,��������󣬸�ֵΪ��*/
	_int32 _result;			/*	0:�ɹ�������ֵ:ʧ��*/
} HS_AC_LITE;

typedef struct t_hs_ac
{
	HS_AC_LITE _action;			/*	0:�ɹ�������ֵ:ʧ��*/

	HS_AC_DATA _data;
} HS_AC;

/* �ͻ��������ѯ��������Ϣ */	
typedef struct t_hs_incom_req_server_info
{
	HS_AC_LITE _ac;						/* EHS_AC_TYPE _type = EAT_HS_REQ_SERVER_INFO */
	char _client_info[HS_MAX_HS_INFO_LEN];	/* �ͻ�����Ϣ */
} HS_INCOM_REQ_SERVER_INFO;

/*  ��ͻ��˷��ͱ�http ��������Ϣ */
typedef struct t_hs_resp_server_info
{
	HS_AC_LITE _ac;						/* EHS_AC_TYPE _type = EAT_HS_RESP_SERVER_INFO */
	char _server_info[HS_MAX_HS_INFO_LEN];	/* ��http ��������Ϣ  */
} HS_RESP_SERVER_INFO;

/////////////////////////////////////////////////////////////////////////////

/* �ͻ�������� */	
typedef struct t_hs_incom_req_bind
{
	HS_AC_LITE _ac;								/* EHS_AC_TYPE _type = EAT_HS_REQ_BIND */
	char _user_name[HS_MAX_USER_NAME_LEN];	
	char _password[HS_MAX_PASSWORD_LEN];		
} HS_INCOM_REQ_BIND;

/*  ��ͻ��˷��Ͱ󶨽�� */
typedef struct t_hs_resp_bind
{
	HS_AC_LITE _ac;						/* EHS_AC_TYPE _type = EAT_HS_RESP_BIND,_result=0��200 ok/400 �ܾ� */
} HS_RESP_BIND;

/////////////////////////////////////////////////////////////////////////////

/* �ͻ������������ļ� */	
typedef struct t_hs_incom_req_get_file
{
	HS_AC_LITE _ac;								/* EHS_AC_TYPE _type = EAT_HS_REQ_GET_FILE */
	char _user_name[HS_MAX_USER_NAME_LEN];	
	char _file_path[HS_MAX_FILE_PATH_LEN];		/* http server �е���ʵ·�� */
	char _file_name[HS_MAX_FILE_NAME_LEN];		/* ����ļ���Ϊ�գ����ʾ��ȡ·���µ��ļ��б� */	
	char _cookie[HS_MAX_COOKIE_LEN];
	
	_u64  _range_from;			/* RANGE ��ʼλ��*/
	_u64  _range_to;			/* RANGE ����λ��*/
	
	BOOL  _accept_gzip;			/* �Ƿ����ѹ���ļ� */
} HS_INCOM_REQ_GET_FILE;

/*  �����ؿ���Ȩ�����ļ�*/
typedef struct t_hs_resp_get_file
{
	HS_AC_LITE _ac;						/* EHS_AC_TYPE _type = EAT_HS_RESP_GET_FILE ,_result = 0��200��206Ϊ��������,����ܾ�*/
	BOOL  _is_gzip;						/* �����ļ�ʱ�Ƿ�ѹ���ļ� */
	char  _encrypt_key[HS_MAX_ENCRYPT_KEY_LEN];		/* �����ļ�ʱ�Ƿ����, ���ؿ⽫���ø�key���ļ����ݽ��м��ܣ�Ϊ���򲻼���*/
	//char _content_type[ETM_MAX_CONTENT_TYPE_LEN];
}HS_RESP_GET_FILE;

/* �����ļ�����֪ͨ*/	
typedef struct t_hs_notify_transfer_file_progress
{
	HS_AC_LITE _ac;								/* EHS_AC_TYPE _type = EAT_HS_NOTIFY_TRANSFER_FILE_PROGRESS */
	
	_u64  _file_size;								/* �ļ��ܴ�С*/
	_u64  _range_size;							/* ��������Ҫ���͵����ݴ�С*/
	_u64  _transfer_size;						/* �Ѵ���Ĵ�С*/
} HS_NOTIFY_TRANSFER_FILE_PROGRESS;

/////////////////////////////////////////////////////////////////////////////

/* �ͻ��������ϴ��ļ� */	
typedef struct t_hs_incom_req_post_file
{
	HS_AC_LITE _ac;								/* EHS_AC_TYPE _type = EAT_HS_REQ_POST_FILE */
	char _user_name[HS_MAX_USER_NAME_LEN];	
	char _file_path[HS_MAX_FILE_PATH_LEN];		/* http server �е���ʵ·�� */
	char _file_name[HS_MAX_FILE_NAME_LEN];		/* ����ļ���Ϊ�գ����ʾ��Ҫ�����ļ��� */	
	char _cookie[HS_MAX_COOKIE_LEN];
	char  _encrypt_key[HS_MAX_ENCRYPT_KEY_LEN];		/* �ļ��Ƿ����, ���ؿ⽫���ø�key���ļ����ݽ��н��ܣ�Ϊ�����������*/
	_u64  _file_size;				/* Content-Length:*/
	
	_u64  _range_from;			/* RANGE ��ʼλ��*/
	_u64  _range_to;			/* RANGE ����λ��*/
	
	BOOL  _is_gzip;			/* �ļ������Ƿ���ѹ��*/
} HS_INCOM_REQ_POST_FILE;

/*  �����ؿ���Ȩ�����ļ�*/
typedef struct t_hs_resp_post_file
{
	HS_AC_LITE _ac;						/* EHS_AC_TYPE _type = EAT_HS_RESP_POST_FILE ,_result = 0��200��206Ϊ��������,����ܾ�*/
	BOOL _is_break_transfer;           /* �Ƿ�ϵ�����*/
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
	_u32 _action_id_num;			/* ��1��ʼ�ۼӣ���������action_id */
	LIST _path_list;    				/* ��Ŀ¼�µ�����Ŀ¼�б�, HS_PATH */
	MAP _actions;					/* ������еĶ�����HS_AC */
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

/* ����ʺţ�Ŀǰ���ֻ֧��һ�� */
_int32 http_server_add_account_handle( void *_param );
/* ��http �������������·�����ļ� 
����:real_path="/mnt/sdcard/tddownload",virtual_path="my_dir",�����ͻ��tddownload����ļ���ӳ�䵽http ��������Ŀ¼��my_dir��,
need_auth��ʾ���ʸ��ļ����ļ���ʱ�Ƿ���Ҫ�û�������֤*/
_int32 http_server_add_path_handle( void *_param );
/* ��ȡhttp server�е��ļ��б�
����virtual_path="./",path_array_buffer = buffer[10][ETM_MAX_FILE_NAME_LEN],*path_num =  10,��ʾ��ȡ��Ŀ¼��ǰʮ���ļ���Ŀ¼������*/
_int32 http_server_get_path_list_handle( void *_param );
/* �����ؿ����ûص����� */
_int32 http_server_set_callback_func_handle( void *_param );
/* ������Ӧ */
_int32 http_server_send_resp_handle( void *_param );
/* ȡ��ĳ���첽���� */
_int32 http_server_cancel_handle( void *_param );


_int32 hs_id_comp(void *E1, void *E2);


#endif /* ENABLE_HTTP_SERVER */

#ifdef __cplusplus
}
#endif

#endif  /*__HTTP_SERVER_INTERFACE_20090222*/
