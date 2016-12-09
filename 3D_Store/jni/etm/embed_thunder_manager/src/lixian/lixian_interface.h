#if !defined(__LIXIAN_INTERFACE_H_20111028)
#define __LIXIAN_INTERFACE_H_20111028

#ifdef __cplusplus
extern "C" 
{
#endif

#include "em_common/em_define.h"

#ifdef ENABLE_LIXIAN_ETM
#include "em_common/em_errcode.h"
#include "em_asyn_frame/em_asyn_frame.h"
#include "download_manager/download_task_interface.h"

#include "asyn_frame/msg.h"
#include "em_common/em_list.h"
#include "em_common/em_map.h"
#include "settings/em_settings.h"
#include "em_common/em_utility.h"

/************************************************************************/
/*                    STRUCT DEFINE                                     */
/************************************************************************/
#define   LX_LIST_XML_FILE_PATH "lixian_list_xml_file"
#define   LX_TEMP_FILE_NAME "lixian_temp.xml"
#define   LX_DEFAULT_DETECT_XML_NAME "detect_string_config.xml"

#define 	LX_DETECT_REGEX_CONFIG "http://wireless.yun.vip.runmit.com/detect_regex_config.xml"//"http://192.168.108.122/detect_regex_config.xml"//
#define   LX_DETECT_STRING_CONFIG "http://wireless.yun.vip.runmit.com/detect_string_config.xml"//"http://192.168.108.122/detect_string_config.xml"//

#define MAX_GET_TASK_LIST_REQ_NUM 5
#define MAX_GET_TASK_NUM 100
#define MAX_RETRY_GET_LIST_NUM 1

#define MAX_SESSION_ID_LEN 128
#define	MAX_POST_REQ_BUFFER_LEN		(16*1024)	// 16KB
#define	MAX_POST_RESP_BUFFER_LEN	(16*1024)	// 16KB

#define	LX_DISPATCH_TIMEOUT 			20	// 20 ms
#define	LX_UPDATE_TASK_DISPATCH_TIMEOUT 			(20*1000)	// 20 s
#define	LX_DEFAULT_HTTP_POST_TIMEOUT 			10
#define	LX_DEFAULT_HTTP_GET_TIMEOUT 			10
#define	LX_HTTP_LOGOUT_TIMEOUT 			2
#define	LX_HTTP_LIST_TIMEOUT 			20
#define	LX_JUMPKEY_MAX_SIZE			512
#define	LX_MAX_COOKIE_LEN			128

#define	LX_MAX_TASK_NUM_EACH_REQ			(1500)

#define	LX_CMD_PROTOCAL_VERSION  14
#define 	LX_CMD_TYPE_NEW_COMMIT_REQ 0x0c 	//12 ��ͨ�������ύ�������񵽷�����
#define 	LX_CMD_TYPE_DELETE_REQ 	0x0a 	//10 ɾ�����߷���������

#define LX_CMD_TYPE_NEW_BT_COMMIT_REQ 0x0d   // 13 BT�������ύ�������񵽷�����
#define LX_CMD_TYPE_NEW_BT_COMMIT_RESP 0x8d  //141

#define LX_CMD_TYPE_DELAYTASK_REQ			0x14  //20��������
#define LX_CMD_TYPE_DELAYTASK_RESP			0x94

#define LX_CMD_TYPE_TASKMINI_REQ 0x0E	 //14���񾫼��ѯ
#define LX_CMD_TYPE_TASKMINI_RESP 0x8E
    
#define LX_CMD_TYPE_GETUSERINFO_REQ			0x10  //��ȡ�û���Ϣ
#define LX_CMD_TYPE_GETUSERINFO_RESP		0x90    

#define LX_CMD_TYPE_HISTORYTASKLIST_REQ		0x11  //17 ��ѯ��ɾ��������ѹ�������
#define LX_CMD_TYPE_HISTORYTASKLIST_RESP	0x91

#define LX_CMD_TYPE_QUERYTASK_REQ  0x03				// ��ͨ����ˢ����������״̬(10sһ��)
#define LX_CMD_TYPE_QUERYTASK_RESP 0x83

#define LX_CMD_TYPE_BT_QUERYTASK_REQ	0x07   //7 BT�������ѯ
#define LX_CMD_TYPE_BT_QUERYTASK_RESP	0x87

#define LX_CMD_TYPE_QUERY_SOME_TASKID_REQ	0x0F  //15����TaskID��ѯ
#define LX_CMD_TYPE_QUERY_SOME_TASKID_RESP	0x8F

#define LX_CMD_TYPE_GET_TASK_LIST_REQ	0x23  //��ȡ���������б� 35
#define LX_CMD_TYPE_GET_TASK_LIST_RESP	0xA3

//
#define LX_HS_CMD_TYPE_TASK_OFF_FLUX_REQ	0x09  //��ȡ��������
#define LX_HS_CMD_TYPE_TASK_OFF_FLUX_RESP	0x89
#define LX_BUSINESS_TYPE  17
#define GET_HIGH_SPEED_FLUX_SEQUENCE  8888
/* Э������ */
typedef enum t_lx_protocol_type
{
	LPT_IDLE=0, 					
	LPT_TASK_LS , 					
	LPT_BT_LS , 					
	LPT_SCREEN_SHOT , 					
	LPT_VOD_URL, 				
	LPT_DL_PIC, 	
	LPT_CRE_TASK,     /*������������*/
	LPT_DEL_TASK,
	LPT_DEL_TASKS,
	LPT_CRE_BT_TASK, /*����BT��������*/
	LPT_DELAY_TASK,   /*��������*/
	LPT_MINIQUERY_TASK,   /*���񾫼��ѯ*/
	LPT_GET_DL_URL,
    LPT_GET_USER_INFO,
    LPT_GET_REGEX,
    LPT_GET_DETECT_STRING,
    LPT_GET_OD_OR_DEL_LS,
    LPT_QUERY_TASK_INFO,    /*��ѯ������Ϣ*/
    LPT_QUERY_BT_TASK_INFO,    /*��ѯBT������Ϣ*/
    LPT_BINARY_TASK_LS,
    LPT_BATCH_QUERY_TASK_INFO,
    LPT_FREE_STRATEGY,
    LPT_TASK_LS_NEW,
    LPT_TAKE_OFF_FLUX,
    LPT_GET_HIGH_SPEED_FLUX,
    LPT_FREE_EXPERIENCE_MEMBER,
    LPT_GET_EXPERIENCE_MEMBER_REMAIN_TIME,
} LX_PT_TYPE;

/* Э��״̬ */
typedef enum t_lx_protocol_state
{
	LPS_WAITING=0, 
	LPS_RUNNING, 
	LPS_PAUSED,
	LPS_SUCCESS, 
	LPS_FAILED
} LX_PT_STATE;

/* �ļ������Ļ���ṹ�� */
typedef struct t_lx_protocol
{
	LX_PT_TYPE _type;		/* Э������ */
	LX_PT_STATE _state;		/* Э��״̬ */
	_u32 _action_id; //http_id or task_id
	//void * _user_data;
	//void * _action_callback_ptr;
	BOOL _is_compress;
	BOOL _is_aes;
	char _aes_key[MAX_SESSION_ID_LEN];

	_int32 _error_code;		/* ���_stateΪFAILED ,�����Ŵ�����*/
	_int32 _resp_status;		

	char  _req_buffer[MAX_POST_REQ_BUFFER_LEN];		/* ����ƴ��xml������ڴ� */
	_u32 _req_buffer_len;	/* �ڴ泤�� */
	_u32 _req_data_len;		/* �ڴ�ʵ��ʹ�ó��ȣ���xml������ֽ��� */

	char  _resp_buffer[MAX_POST_RESP_BUFFER_LEN];		/* ��������xml��Ӧ���ڴ� */
	_u32 _resp_buffer_len;	/* �ڴ泤�� */
	_u32 _resp_data_len;		/* �ڴ�ʵ��ʹ�ó��ȣ���xml��Ӧ���ֽ��� */

	char _file_path[MAX_FULL_PATH_BUFFER_LEN];
	_u32 _file_id;

	_u64 _resp_content_len; /* ������������Ӧ��body ��С */
	_u32 _retry_num;          /* ����������Ӧʱ�����Դ��� */
} LX_PT;



/*--------------------------------------------------------------------------*/
/*                Interfaces provid for other module				        */
/*--------------------------------------------------------------------------*/
_int32 init_lixian_module(void);
_int32 uninit_lixian_module(void);

_int32 lx_zlib_uncompress( _u8 *p_out_buffer, _int32 *p_out_len, const _u8 *p_in_buffer, _int32 in_len );
/////14 ���߿ռ���ؽӿ�
//	ע��:����char *�ַ�����ΪUTF8��ʽ!

/* �����ؿ�����Ѹ���û��ʺ���Ϣ */
_int32 lx_set_user_info(_u64 user_id,const char * user_name,const char * old_user_name,_int32 vip_level,const char * session_id);

_int32 lx_set_sessionid_info(const char * session_id);
/* �����������񷵻������Ϣ*/
typedef struct t_lx_login_result
{
	_u32 _action_id;
	void * _user_data;							/* �û����� */
	_int32 _result;								/*	0:�ɹ�������ֵ:ʧ��*/

	/* �û����*/
	_u8 _user_type;								/* �û����� */
	_u8 _vip_level;								/* �û�vip�ȼ� */
	_u64 _available_space;					/* ���ÿռ� */
	_u64 _max_space;							/* �û��ռ� */
	_u64 _max_task_num;						/* �û���������� */
	_u64 _current_task_num;					/* ��ǰ������ */

}LX_LOGIN_RESULT;

typedef _int32 ( *LX_LOGIN_CALLBACK)(LX_LOGIN_RESULT * p_result);
/* ���ߵ�¼����*/
typedef struct t_lx_login_info
{
	_u64 _user_id;
	char * _new_user_name;
	char * _old_user_name;
	_int32 _vip_level;
	char * _session_id;

	char* _jumpkey;
	_u32 _jumpkey_len;
	
	void * _user_data;		
	LX_LOGIN_CALLBACK _callback_fun;
} LX_LOGIN_INFO;

/* �������߿ռ����֮ǰ��Ҫ���Ƚ������ߵ�¼ */
_int32 lx_login(void* user_data, LX_LOGIN_CALLBACK callback_fun, _u32 * p_action_id);

_int32 lx_has_list_cache(_u64 userid, BOOL * is_cache);
/* ������������ */
typedef enum t_lx_task_type
{
	LXT_UNKNOWN = 0, 	 //������thunder://��ͷ������
	LXT_HTTP , 					
	LXT_FTP,				
	LXT_BT,			// ����
	LXT_EMULE,				
	LXT_BT_ALL,				
	LXT_BT_FILE,				
	LXT_SHOP				
} LX_TASK_TYPE;

/* ��������״̬*/
typedef enum t_lx_task_state
{
	LXS_WAITTING = 1, 				
	LXS_RUNNING = 2, 					
	LXS_PAUSED =4, 					
	LXS_SUCCESS = 8,				
	LXS_FAILED = 16,
	LXS_OVERDUE = 32,
	LXS_DELETED =64,
} LX_TASK_STATE;

/* ���������ļ����� */
typedef enum t_lx_file_type
{
	LXF_UNKNOWN = 0, 				
	LXF_VEDIO , 					
	LXF_PIC , 					
	LXF_AUDIO , 					
	LXF_ZIP , 		/* ����ѹ���ĵ�,��zip,rar,tar.gz... */
	LXF_TXT , 					
	LXF_APP,		/* �������,��exe,apk,dmg,rpm... */
	LXF_OTHER					
} LX_FILE_TYPE;


/* ����������Ϣ */
typedef struct t_lx_task_info
{
	_u64 _task_id;
	LX_TASK_TYPE _type; 						
	LX_TASK_STATE _state;
	char _name[MAX_FILE_NAME_BUFFER_LEN];		/* ��������,utf8����*/
	_u64 _size;								/* �����ļ��Ĵ�С,BT����ʱΪ�������ļ����ܴ�С,��λbyte*/
	_int32 _progress;							/* �����������������,��Ϊ���ؽ���*/

	char _file_suffix[16]; 						/* ��BT������ļ���׺��,��: txt,exe,flv,avi,mp3...*/
	_u8 _cid[20];								/* ��BT������ļ�cid ,bt����Ϊinfo hash */
	_u8 _gcid[20];								/* ��BT������ļ�gcid */
	BOOL _vod;								/* ��BT������ļ��Ƿ�ɵ㲥 */
	char _url[MAX_URL_LEN];					/* ��BT���������URL,utf8����*/
	char _cookie[MAX_URL_LEN];			/* ��BT���������URL ��Ӧ��cookie */

	_u32 _sub_file_num;						/* BT�����б�ʾ���������ļ�����(����BT���������ļ���) */
	_u32 _finished_file_num;					/* BT�����б�ʾ�������������(ʧ�ܻ�ɹ�)�����ļ�����*/

	_u32 _left_live_time;						/* ʣ������ */
	char _origin_url[MAX_URL_LEN];				/* ��BT�����ԭʼ����URL*/

	_u32 _origin_url_len;						/* ��BT�����ԭʼ����URL ����*/
	_u32 _origin_url_hash;					/* ��BT�����ԭʼ����URL ��hashֵ*/
} LX_TASK_INFO;

/* �����MAP �е�����������Ϣ */
typedef struct t_lx_task_info_ex
{
	_u64 _task_id;
	LX_TASK_TYPE _type; 						
	LX_TASK_STATE _state;
	char _name[MAX_FILE_NAME_BUFFER_LEN];		/* ��������,utf8����*/
	_u64 _size;								/* �����ļ��Ĵ�С,BT����ʱΪ�������ļ����ܴ�С,��λbyte*/
	_int32 _progress;							/* �����������������,��Ϊ���ؽ���*/

	char _file_suffix[16]; 						/* ��BT������ļ���׺��,��: txt,exe,flv,avi,mp3...*/
	_u8 _cid[20];								/* ��BT������ļ�cid ,bt����Ϊinfo hash */
	_u8 _gcid[20];								/* ��BT������ļ�gcid */
	BOOL _vod;								/* ��BT������ļ��Ƿ�ɵ㲥 */
	char _url[MAX_URL_LEN];				/* ��BT���������URL,utf8����*/
	char _cookie[MAX_URL_LEN];			/* ��BT���������URL ��Ӧ��cookie */

	_u32 _sub_file_num;						/* BT�����б�ʾ���������ļ�����(����BT���������ļ���) */
	_u32 _finished_file_num;					/* BT�����б�ʾ�������������(ʧ�ܻ�ɹ�)�����ļ�����*/

	_u32 _left_live_time;						/* ʣ������ */

	char _origin_url[MAX_URL_LEN];				/* ��BT�����ԭʼ����URL*/

	_u32 _origin_url_len;						/* ��BT�����ԭʼ����URL ����*/
	_u32 _origin_url_hash;					/* ��BT�����ԭʼ����URL ��hashֵ*/

	MAP _bt_sub_files;						/* BT��������ļ�MAP*/
} LX_TASK_INFO_EX;

/* ��ȡ�б���Ӧ,�ڴ������ؿ��ͷ�,UI��Ҫ����һ�����л���UI�߳��д��� */
typedef struct t_lx_task_list
{
	_u32 _action_id;
	void * _user_data;				/* �û����� */
	_int32 _result;						/*	0:�ɹ�,����ֵ:ʧ��	*/
	_u32 _total_task_num;			/* ���û����߿ռ��ڵ����������*/
	_u32 _task_num;					/* �˴ν�����õ�������� */
	_u64 _total_space;				/* �û������ܿռ䣬��λ�ֽ�  */
	_u64 _available_space;				/* �û����߿��ÿռ䣬��λ�ֽ� */
	LX_TASK_INFO * _task_array;		/* �����б�,����Ϊ_task_num*sizeof(LX_TASK_INFO)   */
} LX_TASK_LIST;
typedef _int32 ( *LX_GET_TASK_LIST_CALLBACK)(LX_TASK_LIST * p_task_list);

/* ��ȡ���߿ռ������б���������*/
typedef struct t_lx_get_task_ls
{
	LX_FILE_TYPE _file_type;			/*  Ϊָ����ȡ�����ض����͵��ļ��������б�*/
	_int32 _file_status;				/*  Ϊָ�������ض�״̬���ļ��������б�,0:ȫ����1:��ѯ��������������2:���*/
	_int32 _offset;						/*  ��ʾ���������̫��ʱ������ȡ��ƫ��ֵ,��0��ʼ*/
	_int32 _max_task_num;			/*  Ϊ�˴λ�ȡ����������   */
	
	void * _user_data;				/* �û����� */
	LX_GET_TASK_LIST_CALLBACK _callback;
} LX_GET_TASK_LIST;

/* ��ȡ���߿ռ������б�
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/

_int32 lx_batch_query_task_info(_u64 *task_id, _u32 num, void* user_data, LX_GET_TASK_LIST_CALLBACK callback_fun, _u64 commit_time, _u64 last_task_id, _u32 offset, _u32 * p_action_id);
_int32 lx_batch_query_task_info_resp(LX_PT * p_action);
_int32 lx_get_task_list_binary_req(LX_GET_TASK_LIST * p_param,_u32 * p_action_id,_int32 priority, _u64 commit_time, _u64 task_id);
_int32 lx_get_task_list_binary_resp(LX_PT * p_action);
_int32 lx_get_task_list(LX_GET_TASK_LIST * p_param,_u32 * p_action_id,_int32 priority);
_int32 lx_get_task_list_req(LX_GET_TASK_LIST * p_param,_u32 * p_action_id,_int32 priority, _u32 retry_num);
_int32 lx_get_task_list_resp(LX_PT * p_action);
_int32 lx_get_task_list_new_req(LX_GET_TASK_LIST * p_param,_u32 * p_action_id,_int32 priority, _u32 retry_num);
_int32 lx_get_task_list_new_resp(LX_PT * p_action);
  _int32 lx_cancel_get_task_list(LX_PT * p_action);

typedef struct t_lx_od_or_del_task_list_result
{
	_u32 _action_id;
	void * _user_data;				/* �û����� */
	_int32 _result;						/*	0:�ɹ�,����ֵ:ʧ��	*/
	_u32 _task_num;					/* �˴ν�����õ�������� */
} LX_OD_OR_DEL_TASK_LIST_RESULT;
typedef _int32 ( *LX_GET_OD_OR_DEL_TASK_LIST_CALLBACK)(LX_OD_OR_DEL_TASK_LIST_RESULT * p_task_list);
/* ��ȡ���߿ռ���ڻ�����ɾ�������б���������*/
typedef struct t_lx_od_or_del_get_task_ls
{
	_int32 _task_type;				/*  0:��ɾ����1:����*/
	_int32 _page_offset;			/*  ��ʾ���������̫��ʱ������ȡ��ƫ��ֵ,��0��ʼ*/
	_int32 _max_task_num;			/*  ÿҳ�������� */
	
	void * _user_data;				/* �û����� */
	LX_GET_OD_OR_DEL_TASK_LIST_CALLBACK _callback;
} LX_GET_OD_OR_DEL_TASK_LIST;
_int32 lx_get_overdue_or_deleted_task_list(LX_GET_OD_OR_DEL_TASK_LIST * p_param,_u32 * p_action_id,_int32 priority);
_int32 lx_get_overdue_or_deleted_task_list_req(LX_GET_OD_OR_DEL_TASK_LIST * p_param,_u32 * p_action_id,_int32 priority);
_int32 lx_get_overdue_or_deleted_task_list_resp(LX_PT * p_action);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* ��ȡ���߿ռ�BT ��������ļ��б�*/


/* BT�������ļ���Ϣ */
typedef struct t_lx_file_info
{
	_u64 _file_id; 						
	LX_TASK_STATE _state;
	char _name[MAX_FILE_NAME_BUFFER_LEN];		/* �ļ���,utf8����*/
	
	_u64 _size;								/* �ļ���С,��λbyte*/
	_int32 _progress;							/* ����ļ�����������,��Ϊ���ؽ���*/

	char _file_suffix[16]; 						/* �ļ���׺��,��: txt,exe,flv,avi,mp3...*/
	_u8 _cid[20];								/* �ļ�cid */
	_u8 _gcid[20];								/* �ļ�gcid */
	BOOL _vod;								/* �ļ��Ƿ�ɵ㲥 */
	char _url[MAX_URL_LEN];				/* �ļ�����������URL,utf8����*/
	char _cookie[MAX_URL_LEN];			/* �ļ�������URL ��Ӧ��cookie */
	_u32 _file_index;						/* BT ���ļ���bt�����е��ļ����,��0��ʼ */
} LX_FILE_INFO;

/* ��ȡ�б���Ӧ,�ڴ������ؿ��ͷ�,UI��Ҫ����һ�����л���UI�߳��д��� */
typedef struct t_lx_file_list
{
	_u32 _action_id;
	_u64 _task_id;
	void * _user_data;				/* �û����� */
	_int32 _result;						/*	0:�ɹ�,����ֵ:ʧ��	*/
	_u32 _total_file_num;				/* ��BT �����ڵ����ļ�����*/
	_u32 _file_num;					/* �˴ν�����õ��ļ����� */
	LX_FILE_INFO * _file_array;		/* �ļ��б�,����Ϊ_file_num*sizeof(LX_FILE_INFO)   */
} LX_FILE_LIST;
typedef _int32 ( *LX_GET_FILE_LIST_CALLBACK)(LX_FILE_LIST * p_file_list);

/* ��ȡ���߿ռ�BT ��������ļ��б���������*/
typedef struct t_lx_get_bt_file_ls
{
	_u64 _task_id;					/* BT ����id */
	LX_FILE_TYPE _file_type;			/*  Ϊָ����ȡbt�������ض����͵��ļ�*/
	_int32 _file_status;				/*  Ϊָ��bt�������ض�״̬���ļ�,0:ȫ����1:��ѯ��������������2:���*/
	_int32 _offset;						/*  ��ʾ�����ļ�����̫��ʱ������ȡ��ƫ��ֵ,��0��ʼ*/
	_int32 _max_file_num;				/*  Ϊ�˴λ�ȡ����ļ�����   */
	
	void * _user_data;				/* �û����� */
	LX_GET_FILE_LIST_CALLBACK _callback;
} LX_GET_BT_FILE_LIST;

/* ��ȡ���߿ռ�BT ��������ļ��б�
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
_int32 lx_get_bt_task_file_list(LX_GET_BT_FILE_LIST * p_param,_u32 * p_action_id,_int32 priority);
_int32 lx_get_bt_task_file_list_req(LX_GET_BT_FILE_LIST * p_param,_u32 * p_action_id,_int32 priority);

 _int32 lx_get_bt_task_file_list_resp(LX_PT * p_action);

 _int32 lx_cancel_get_bt_task_file_list(LX_PT * p_action);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/* ��ȡ��Ƶ��ͼ,��ͼΪjpg��ʽ */
typedef struct t_lx_screenshot_result
{
	_u32 _action_id;
	void * _user_data;							/* �û����� */
	char  _store_path[MAX_FILE_PATH_LEN]; 	/* �������� ��ͼ���·��*/
	BOOL _is_big;
	_u32 _file_num;
	_u8 * _gcid_array;
	_int32 * _result_array;							/*	���ļ��Ľ�ͼ��ȡ���:0 �ɹ�,����ֵ:ʧ��	*/
} LX_SCREENSHOT_RESUTL;
typedef _int32 ( *LX_GET_SCREENSHOT_CALLBACK)(LX_SCREENSHOT_RESUTL * p_screenshot_result);

/* ��ȡ��Ƶ��ͼ���������*/
typedef struct t_lx_get_screenshot
{
	_u32 _file_num;								/* file_num ��Ҫ��ȡ��ͼ���ļ����� */
	_u8 * _gcid_array;							/* gcid_array ��Ÿ�����Ҫ��ȡ��ͼ���ļ�gcid,��Ϊ20byte�����ִ� */
	
	BOOL _is_big;								/* is_big �Ƿ��ȡ��ͼ��TRUEΪ��ȡ 256*192�Ľ�ͼ,FALSEΪ��ȡ128*96	�Ľ�ͼ */
	const char * _store_path;						/* store_pathΪ��ͼ���·��,�����д,���ؿ⽫�Ѹ�gcid��Ӧ���ļ���ͼ���ص���Ŀ¼��,����gcid��Ϊ�ļ���,��ͼΪjpg��׺ */
	BOOL _overwrite;
	
	void * _user_data;							/* �û����� */
	LX_GET_SCREENSHOT_CALLBACK _callback;
} LX_GET_SCREENSHOT;

/* ��ȡ��Ƶ��ͼ,��ͼΪjpg��ʽ
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
_int32 lx_get_screenshot(LX_GET_SCREENSHOT * p_param,_u32 * p_action_id);

 _int32 lx_get_screenshot_resp(LX_PT * p_action);

 _int32 lx_cancel_screenshot(LX_PT * p_action);

/* ����ͼƬ */
_int32 lx_dowanload_pic(LX_PT * p_action);
 _int32 lx_dowanload_pic_resp(LX_PT * p_action);
_int32 lx_cancel_download_pic(LX_PT * p_action,BOOL need_cancel_ss);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* ��ȡ�ļ����Ƶ㲥��URL */

/* ��"��Ƶָ��"������ѯ����ͬһ����Ӱ��������ѡ�㲥URL �������Ϣ*/
typedef struct t_lx_fp_info
{
	_u64 _size;								/* ��ѡ�㲥��Դ���ļ���С,��λbyte*/
	_u8 _cid[20];								/* ��ѡ�㲥��Դ���ļ�cid */
	_u8 _gcid[20];								/* ��ѡ�㲥��Դ���ļ�gcid */
	_u32 _video_time;							/* ��ѡ�㲥��Դ����Ƶʱ�䳤��,��λ: s*/
	_u32 _bit_rate;							/* ��ѡ�㲥��Դ�����ʡ��������ݲ��ṩ�ò�����*/
	char _vod_url[MAX_URL_LEN];    		/* ��ѡ�㲥��Դ�ĵ㲥URL*/
	char _screenshot_url[MAX_URL_LEN];    	/* ��ѡ�㲥��Դ�Ľ�ͼURL*/
} LX_FP_INFO;

/*�Ƶ㲥�����Ϣ*/
typedef struct t_lx_get_vod_result
{
	_u32 _action_id;
	void * _user_data;							/* �û����� */
       _int32 _result;									/*	0:�ɹ�������ֵ:ʧ��*/

	//_u64 _size;							
	_u8 _cid[20];							
	_u8 _gcid[20];							
	//char _sheshou_cid[64];    						/*����cid,���ڲ�ѯ��Ļ*/
	_u32 _video_time;								/*��Ƶʱ�䳤��,��λ: s*/

	_u32 _normal_bit_rate;						/* �������ʡ��������ݲ��ṩ�ò�����*/
	_u64 _normal_size;							/* ������Ƶ�ļ��Ĵ�С */
	char _normal_vod_url[MAX_URL_LEN];    	/* ����㲥URL*/
	
	_u32 _high_bit_rate;							/* �������ʡ��������ݲ��ṩ�ò�����*/
	_u64 _high_size;							/* ������Ƶ�ļ��Ĵ�С */
	char _high_vod_url[MAX_URL_LEN];    		/* ����㲥URL*/

	/* ���ֲ�ͬ���ʸ�ʽ */
	_u32 _fluency_bit_rate_1;							/* �������ʡ��������ݲ��ṩ�ò�����*/
	_u64 _fluency_size_1;							/* ������Ƶ�ļ��Ĵ�С */
	char _fluency_vod_url_1[MAX_URL_LEN];    		/* �����㲥URL*/

	_u32 _fluency_bit_rate_2;							/* �������ʡ��������ݲ��ṩ�ò�����*/
	_u64 _fluency_size_2;							/* ������Ƶ�ļ��Ĵ�С */
	char _fluency_vod_url_2[MAX_URL_LEN];    		/* �����㲥URL*/
	
	_u32 _fpfile_num;								/* ��"��Ƶָ��"������ѯ����ͬһ����Ӱ��������ѡ�㲥URL �ĸ���*/
	LX_FP_INFO * _fp_array;						/* ������ѡ�㲥URL ����*/
} LX_GET_VOD_RESULT;
typedef _int32 ( *LX_GET_VOD_URL_CALLBACK)(LX_GET_VOD_RESULT * p_result);

/* ��ȡ��Ƶ�ļ����Ƶ㲥URL ���������*/
typedef struct t_lx_get_vod
{
	_u32 _device_width;
	_u32 _device_height;
	_u32 _video_type;							/*	��Ƶ��ʽ: 0:flv(���ݾɰ汾);  1:flv;  2:ts;  3:����;  4: mp4  */
	
	_u64 _size;								/* �ļ���С,��λbyte*/
	_u8 _cid[20];								
	_u8 _gcid[20];								
	//BOOL _is_after_trans;					/* �Ƿ���ת�����ļ�*/
	_u32 _max_fpfile_num;					/* ��"��Ƶָ��"������ѯͬһ����Ӱ��������ѡ�㲥URL ��������*/

	void * _user_data;						/* �û����� */
	LX_GET_VOD_URL_CALLBACK _callback_fun;
} LX_GET_VOD;

/* ��ȡ��Ƶ�ļ����Ƶ㲥URL
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
_int32 lx_get_vod_url(LX_GET_VOD * p_param,_u32 * p_action_id);
 _int32 lx_get_vod_url_resp(LX_PT * p_action);
 _int32 lx_cancel_get_vod_url(LX_PT * p_action);

/* �����������񷵻������Ϣ*/
typedef struct t_lx_create_task_result
{
	_u32 _action_id;
	void * _user_data;							/* �û����� */
	_int32 _result;									/*	0:�ɹ�������ֵ:ʧ��*/

	/* �û����*/
	_u64 _available_space;							/* ���ÿռ� */
	_u64 _max_space;								/* �û��ռ� */
	_u64 _max_task_num;							/* �û���������� */
	_u64 _current_task_num;						/* ��ǰ������ */

	/* �۷����*/
	BOOL _is_goldbean_converted;					/* �Ƿ�ת���˽�*/
	_u64 _goldbean_total_num;					/* ���ܶ�*/
	_u64 _goldbean_need_num;					/* ��Ҫ�Ľ�����*/
	_u64 _goldbean_get_space;					/* ת��������õĿռ��С*/
	_u64 _silverbean_total_num;					/* ��������*/
	_u64 _silverbaen_need_num;					/* ��Ҫ����������*/

	/* �������*/
	_u64 _task_id;
	_u64 _file_size;       
	LX_TASK_STATE _status;  /*�ļ����������״̬*/
	LX_TASK_TYPE  _type;
	_u32 _progress;								/*������ȣ�����100��Ϊ�ٷֱȣ�10000��100%*/
} LX_CREATE_TASK_RESULT;

typedef _int32 ( *LX_CREATE_TASK_CALLBACK)(LX_CREATE_TASK_RESULT * p_result);

/* ���������������*/
typedef struct t_lx_create_task_info
{
	char _url[MAX_URL_LEN];
	char _ref_url[MAX_URL_LEN];
	char _task_name[MAX_FILE_PATH_LEN];
	_u8 _cid[20];			
	_u8 _gcid[20];
	_u64 _file_size;
	BOOL _is_auto_charge;					/* ���ռ䲻�����н�ʱ�Ƿ��Զ��۽� */

	void * _user_data;						/* �û����� */
	LX_CREATE_TASK_CALLBACK _callback_fun;
} LX_CREATE_TASK_INFO;

/* �����߿ռ��ύ��������*/
_int32 lx_create_task(LX_CREATE_TASK_INFO * p_param, _u32 * p_action_id, _u32 retry_num);
 _int32 lx_create_task_resp(LX_PT * p_action);

_int32 lx_create_task_again(_u64 task_id, void* user_data ,LX_CREATE_TASK_CALLBACK callback, _u32 * p_action_id);


/* ����bt�����������*/
typedef struct t_lx_create_bt_task_info
{
	char* _title;							/* ����ı���,utf8����,�255�ֽ�,��NULL��ʾֱ����torrent�����title ���ô��������infohash������*/
	char* _seed_file_full_path; 			/* ��torrent��������ʱ�������������������ļ�(*.torrent) ȫ·�� */
	char* _magnet_url;					/* �ô�������������ʱ��������Ǵ�����*/
	_u32 _file_num;						/* ��Ҫ���ص����ļ�����*/
	_u32* _download_file_index_array; 	/* ��Ҫ���ص����ļ�����б�ΪNULL ��ʾ����������Ч�����ļ�*/

	BOOL _is_auto_charge;				/* ���ռ䲻�����н�ʱ�Ƿ��Զ��۽� */
	
	void * _user_data;		
	LX_CREATE_TASK_CALLBACK _callback_fun;
}LX_CREATE_BT_TASK_INFO;

 _int32 lx_create_bt_task(LX_CREATE_BT_TASK_INFO * p_param, _u32 * p_action_id, _u32 retry_num);
_int32 lx_create_bt_task_resp(LX_PT * p_action);


/* ɾ���������񷵻������Ϣ*/
typedef struct t_lx_delete_task_result
{
	_u32 _action_id;
	void * _user_data;							/* �û����� */
	_int32 _result;									/*	0:�ɹ�������ֵ:ʧ��*/
} LX_DELETE_TASK_RESULT;

typedef _int32 ( *LX_DELETE_TASK_CALLBACK)(LX_DELETE_TASK_RESULT * p_result);

/* �����߿ռ�ɾ����������*/
_int32 lx_delete_task(_u64 task_id, void* user_data, LX_DELETE_TASK_CALLBACK callback_fun, _u32 * p_action_id, _u32 retry_num);
_int32 lx_delete_overdue_task(_u64 task_id, void* user_data, LX_DELETE_TASK_CALLBACK callback_fun, _u32 * p_action_id, _u32 retry_num);

 _int32 lx_delete_task_resp(LX_PT * p_action);

/* ����ɾ���������񷵻������Ϣ*/
typedef struct t_lx_delete_tasks_result
{
	_u32 _action_id;
	void * _user_data;							/* �û����� */
	_u32 _task_num;
	_u64 * _p_task_ids;
	_int32 *_p_results;									/*	0:�ɹ�������ֵ:ʧ��*/
} LX_DELETE_TASKS_RESULT;

typedef _int32 ( *LX_DELETE_TASKS_CALLBACK)(LX_DELETE_TASKS_RESULT * p_result);

/* �����߿ռ�����ɾ����������*/
_int32 lx_delete_tasks(_u32 task_num,_u64 * p_task_ids,BOOL is_overdue, void* user_data, LX_DELETE_TASKS_CALLBACK callback_fun, _u32 * p_action_id);
_int32 lx_delete_tasks_resp(LX_PT * p_action);

/* �����������񷵻������Ϣ*/
typedef struct t_lx_delay_task_result
{
	_u32 _action_id;
	void * _user_data;							/* �û����� */
	_int32 _result;									/*	0:�ɹ�������ֵ:ʧ��*/
	_u32 _left_live_time;							/*  ʣ������ */
} LX_DELAY_TASK_RESULT;

typedef _int32 ( *LX_DELAY_TASK_CALLBACK)(LX_DELAY_TASK_RESULT * p_result);
_int32 lx_delay_task(_u64 task_id, void* user_data, LX_DELAY_TASK_CALLBACK callback_fun, _u32 * p_action_id, _u32 retry_num);
_int32 lx_delay_task_resp(LX_PT * p_action);

/* �����ѯ�������񷵻������Ϣ*/
typedef struct t_lx_miniquery_task_result
{
	_u32 _action_id;
	void * _user_data;							/* �û����� */
	_int32 _result;									/*	0:�ɹ�������ֵ:ʧ��*/
	_u32 _state;
	_u64 _commit_time;									/*  �ύʱ�� */
	_u32 _left_live_time;                     /*  ʣ������ */
} LX_MINIQUERY_TASK_RESULT;

typedef _int32 ( *LX_MINIQUERY_TASK_CALLBACK)(LX_MINIQUERY_TASK_RESULT * p_result);
_int32 lx_miniquery_task(_u64 task_id, void* user_data, LX_MINIQUERY_TASK_CALLBACK callback_fun, _u32 * p_action_id);
_int32 lx_miniquery_task_resp(LX_PT * p_action);


/* ��ѯ����������Ϣ���ص�����, ��ȡ�������б�ELX_TASK_INFO�ڴ������ؿ��ͷ�,UI��Ҫ����һ�����л���UI�߳��д��� */
typedef struct t_lx_query_task_info_result
{
	_u32 _action_id;
	void * _user_data;						/* �û����� */
	_int32 _result;							/*	0:�ɹ�������ֵ:ʧ��*/
	//LX_TASK_STATE _download_status;			/*	����״̬*/
	//_int32 _progress;	
} LX_QUERY_TASK_INFO_RESULT;
typedef _int32 ( *LX_QUERY_TASK_INFO_CALLBACK)(LX_QUERY_TASK_INFO_RESULT * p_result);
_int32 lx_query_task_info(_u64 task_id, void* user_data, LX_QUERY_TASK_INFO_CALLBACK callback_fun, _u32 * p_action_id);
_int32 lx_query_task_info_resp(LX_PT * p_action);
_int32 lx_query_bt_task_info(_u64 *task_id, _u32 num, void* user_data, LX_QUERY_TASK_INFO_CALLBACK callback_fun, _u32 * p_action_id);
_int32 lx_query_bt_task_info_resp(LX_PT * p_action);

typedef struct t_lx_get_user_info_task_result
{
    _u32 _action_id;
    void * _user_data;							
    _int32 _result;									
    _u32 _max_task_num;
	_u64 _max_store;
    _u64 _vip_store;
    _u64 _buy_store;
    _u8 _buy_num_task;
    _u8 _buy_num_connection;
    _u32 _buy_bandwidth;
    _u32 _buy_task_live_time;
    char _expire_data[MAX_DATE_LEN];
    _u64 _available_space;
    _u32 _total_num;
    _u32 _history_task_total_num;
    _u32 _suspending_num;
    _u32 _downloading_num;
    _u32 _waiting_num;
    _u32 _complete_num;
    _u64 _store_period;
    char _cookie[MAX_URL_LEN];
    _u8 _vip_level;
    _u8 _user_type;
    _u64 _goldbean_num;									
	_u64 _silverbean_num;					
    
} LX_GET_USER_INFO_TASK_RESULT;
    
typedef _int32 ( *LX_GET_USER_INFO_TASK_CALLBACK)(LX_GET_USER_INFO_TASK_RESULT * p_result);
/* ��ȡ�û�������Ϣ */
_int32 lx_get_user_info_req(void* user_data, LX_LOGIN_CALLBACK callback_fun, _u32* p_action_id);
_int32 lx_get_user_info_resp(LX_PT * p_action);

/* ��ȡ�㲥ʱ��cookie */
_int32 lx_get_cookie(char * cookie_buffer);

/* ȡ��ĳ���첽���� */
_int32 lx_cancel(_u32 action_id);

typedef struct t_lx_free_strategy_result
{
	_u32 _action_id;
	void * _user_data;					/* �û����� */
	_int32 _result;					/*	0:�ɹ���-1: ��ʱ; ����ֵ:ʧ��*/
	_u64 _user_id;						/*  �û�id */
	BOOL _is_active_stage;			/*  �Ƿ��ڻ���� */
	_u32 _free_play_time;				/*  �Բ�ʱ�� */
} LX_FREE_STRATEGY_RESULT;
typedef _int32 ( *LX_FREE_STRATEGY_CALLBACK)(LX_FREE_STRATEGY_RESULT * p_result);
_int32 lx_get_free_strategy(_u64 user_id, const char *session_id, void* user_data, LX_FREE_STRATEGY_CALLBACK callback_fun, _u32 * p_action_id);
_int32 lx_get_free_strategy_resp(LX_PT * p_action);

typedef enum t_lx_cloudserver_type
{
	LX_CS_RESP_GET_FREE_DAY = 1800, 	
	LX_CS_RESP_GET_REMAIN_DAY
} LX_CS_AC_TYPE;
typedef struct t_lx_free_experience_member_result
{
	_u32 _action_id;
	void * _user_data;				/* �û����� */
	_int32 _result;				/*	0:�ɹ���-1: ��ʱ; ����ֵ:ʧ��*/
	_u64 _user_id;					/*  �û�id */
	LX_CS_AC_TYPE _type;
	_u32 _free_time;				/*  ������� */
	BOOL _is_first_use;			/*  �Ƿ��һ��ʹ�� */
} LX_FREE_EXPERIENCE_MEMBER_RESULT;
typedef _int32 ( *LX_FREE_EXPERIENCE_MEMBER_CALLBACK)(LX_FREE_EXPERIENCE_MEMBER_RESULT * p_result);
_int32 lx_get_free_experience_member(_u32 type, _u64 user_id, void* user_data, LX_FREE_EXPERIENCE_MEMBER_CALLBACK callback_fun, _u32 * p_action_id);
_int32 lx_get_free_experience_member_resp(LX_PT * p_action);

typedef struct t_lx_get_high_speed_flux_result
{
	_u32 _action_id;
	void * _user_data;					/* �û����� */
	_int32 _result;					/*	0:�ɹ���-1: ��ʱ; ����ֵ:ʧ��*/
} LX_GET_HIGH_SPEED_FLUX_RESULT;
typedef _int32 ( *LX_GET_HIGH_SPEED_FLUX_CALLBACK)(LX_GET_HIGH_SPEED_FLUX_RESULT * p_result);
typedef struct t_lx_get_high_speed_flux
{
	_u64  _user_id;     /* �û�id */
	_u64  _flux;		  /* ��Ҫ��ȡ�������� */
	_u32  _valid_time; /* ��ȡ��������Чʱ�� */
	_u32  _property;   /* 0Ϊ��ͨ��������1Ϊ��Ա������ */
	void* _user_data;   /* �û����� */
	LX_GET_HIGH_SPEED_FLUX_CALLBACK _callback_fun;  /* �ӿڻص����� */
} LX_GET_HIGH_SPEED_FLUX;
_int32 lx_get_high_speed_flux(LX_GET_HIGH_SPEED_FLUX *p_param, _u32 * p_action_id);
_int32 lx_get_high_speed_flux_resp(LX_PT * p_action);

typedef struct t_lx_take_off_flux_result
{
	_u32 _action_id;
	void * _user_data;					/* �û����� */
	_int32 _result;					/*	0:�ɹ���-1: ��ʱ; ����ֵ:ʧ��*/
	_u64 _all_flux;					/*   */
	_u64 _remain_flux;				   /*   */
	_u64 _task_id;					  /*   */
	_u64 _file_id;					  /*   */
} LX_TAKE_OFF_FLUX_RESULT;
typedef _int32 ( *LX_TAKE_OFF_FLUX_CALLBACK)(LX_TAKE_OFF_FLUX_RESULT * p_result);
_int32 lx_take_off_flux_from_high_speed(_u64 task_id, _u64 file_id, void* user_data, LX_TAKE_OFF_FLUX_CALLBACK callback_fun, _u32 * p_action_id);
_int32 lx_take_off_flux_from_high_speed_resp(LX_PT * p_action);

/* ����ҳ�л�ÿɹ����ص�url  */
typedef struct t_lx_downloadable_url_result
{
	_u32 _action_id;
	void * _user_data;						/* �û����� */
	char _url[MAX_URL_LEN];				/* ����ʱ�������ؿⱻ��̽��URL */
	_int32 _result;								/*	0:�ɹ�������ֵ:ʧ��,������ֶ���Ч*/
	_u32 _url_num;							/*  ���ֿɹ����ص�URL ���� */
} LX_DOWNLOADABLE_URL_RESULT;

typedef _int32 ( *LX_DOWNLOADABLE_URL_CALLBACK)(LX_DOWNLOADABLE_URL_RESULT * p_result);
_int32 lx_get_downloadable_url_from_webpage(const char* url,void* user_data,LX_DOWNLOADABLE_URL_CALLBACK callback_fun , _u32 * p_action_id);
_int32 lx_get_downloadable_url_from_webpage_req(const char* url,void* user_data,LX_DOWNLOADABLE_URL_CALLBACK callback_fun , _u32 * p_action_id);
_int32 lx_get_downloadable_url_from_webpage_resp(LX_PT * p_action);
 _int32 lx_cancel_get_downloadable_url_from_webpage(LX_PT * p_action);
 _int32 lx_get_downloadable_url_from_webpage_file(const char* url,const char * source_file_path,_u32 * p_url_num);

// /*��ȡ������̽������ҳ������ʽ*/
_int32 lx_get_detect_regex(void* user_data);
_int32 lx_get_detect_regex_resp(LX_PT * p_action);

// /*��ȡ������̽������ҳ��λ�ַ���*/
_int32 lx_get_detect_string(void* user_data);
_int32 lx_get_detect_string_resp(LX_PT * p_action);
/*--------------------------------------------------------------------------*/
/*                Interfaces provid for external program				        */
/*--------------------------------------------------------------------------*/

/////14 ���߿ռ���ؽӿ�
//	ע��:����char *�ַ�����ΪUTF8��ʽ!

/* �����ؿ�����Ѹ���û��ʺ���Ϣ */
_int32 lixian_set_user_info(void * p_param);

_int32 lixian_set_sessionid(void * p_param);

/* ��ȡ�û���Ϣ������¼���߿ռ� */
_int32 lixian_login(void * p_param);
_int32 lixian_has_list_cache(void * p_param);
/* ��ȡ���߿ռ������б�
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
_int32 lixian_get_task_list(void * p_param);

_int32 lixian_get_overdue_or_deleted_task_list(void * p_param);

///// ��������״̬��ȡ��������id �б�,stateȡֵ0:ȫ����1:��ѯ��������������2:���,*buffer_len�ĵ�λ��sizeof(_u64)
_int32 lixian_get_task_ids_by_state(void * p_param);

/* ��ȡ����������Ϣ */
_int32 lixian_get_task_info(void * p_param);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* ��ȡ���߿ռ�BT ��������ļ��б�
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
_int32 lixian_get_bt_task_file_list(void * p_param);

///// ��ȡbt���������ļ�id �б�,*buffer_len�ĵ�λ��sizeof(u32)
_int32 lixian_get_bt_sub_file_ids(void * p_param);

/////��ȡ����BT������ĳ�����ļ�����Ϣ
_int32 lixian_get_bt_sub_file_info(void * p_param);
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* ��ȡ��Ƶ��ͼ,��ͼΪjpg��ʽ
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
_int32 lixian_get_screenshot(void * p_param);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* ��ȡ��Ƶ�ļ����Ƶ㲥URL
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
_int32 lixian_get_vod_url(void * p_param);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* �����߿ռ��ύ��������
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
_int32 lixian_create_task(void * p_param);

/* ���ѹ��ڵ����������߿ռ������ύ��������
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
_int32 lixian_create_task_again(void * p_param);

/* �����߿ռ��ύbt��������
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
_int32 lixian_create_bt_task(void * p_param);

/* �����߿ռ��ύ������������,ʹ�øýӿ�ǰ�����ù�jump key*/
_int32 lixian_create_tasks(void * p_param);
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* �����߿ռ�ɾ����������
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
_int32 lixian_delete_task(void * p_param);
_int32 lixian_delete_overdue_task(void * p_param);

/* �����߿ռ�����ɾ����������
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
_int32 lixian_delete_tasks(void * p_param);

/* �����߿ռ�������������
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
_int32 lixian_delay_task(void * p_param);

/* ���߿ռ������ѯ
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
_int32 lixian_query_task_info(void * p_param);

_int32 lixian_query_bt_task_info(void * p_param);

/* ���߿ռ侫�������ѯ
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
_int32 lixian_miniquery_task(void * p_param);


_int32 lixian_get_user_info_task(void * p_param);
    
///// ����URL ,cid,gicd ��BT���ӵ�info_hash ���Ҷ�Ӧ����������id
_int32 lixian_get_task_id_by_eigenvalue( void * p_param );
///// ����gicd ���Ҷ�Ӧ����������id
_int32 lixian_get_task_id_by_gcid( void * p_param );
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* �����ؿ�����jump key*/
_int32 lixian_set_jumpkey(void * p_param);

/* ��ȡ�㲥ʱ��cookie */
_int32 lixian_get_cookie(void * p_param);

/* ��ȡ�û����߿ռ���Ϣ*/
_int32 lixian_get_space(void * p_param);

/* �˳����߿ռ� */
_int32 lixian_logout(void * p_param);

/* ��������ģ���Э�飬��ʱд������ģ����� */
_int32 lixian_get_free_strategy(void * p_param);

_int32 lixian_get_free_experience_member(void * p_param);

_int32 lixian_get_experience_member_remain_time(void * p_param);

_int32 lixian_get_high_speed_flux(void * p_param);

_int32 lixian_take_off_flux_from_high_speed(void * p_param);

/* ȡ��ĳ���첽���� */
_int32 lixian_cancel(void * p_param);

/* ����ҳ�л�ÿɹ����ص�url  */
_int32 lixian_get_downloadable_url_from_webpage(void * p_param);

/* ���Ѿ���ȡ������ҳԴ�ļ�����̽�ɹ����ص�url ,ע��,�����ͬ���ӿڣ�����Ҫ���罻��  */
_int32 lixian_get_downloadable_url_from_webpage_file(void * p_param);

///// ��ȡ��ҳ�пɹ�����URL��id �б�,*buffer_len�ĵ�λ��sizeof(u32)
_int32 lixian_get_downloadable_url_ids(void * p_param);

/* ��ȡ�ɹ�����URL ��Ϣ */
_int32 lixian_get_downloadable_url_info(void * p_param);

/*--------------------------------------------------------------------------*/
/*                Internal functions				        */
/*--------------------------------------------------------------------------*/
_int32 lx_check_if_pic_exist(LX_PT * p_action);
_int32 lx_set_screenshot_result(LX_PT * p_action,_int32 result);
BOOL lx_check_is_all_gcid_parse_failed(LX_PT * p_action);

 _int32 em_update_regex_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid);
 _int32 em_update_detect_string_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid);


#endif /* ENABLE_LIXIAN */

#ifdef __cplusplus
}
#endif

#endif // !defined(__LIXIAN_INTERFACE_H_20111028)

