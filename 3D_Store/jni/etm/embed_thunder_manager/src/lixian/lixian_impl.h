#if !defined(__LIXIAN_IMPL_H_20111029)
#define __LIXIAN_IMPL_H_20111029
/*----------------------------------------------------------------------------------------------------------
author:		Zeng yuqing
created:	2011/11/02
-----------------------------------------------------------------------------------------------------------*/


#ifdef __cplusplus
extern "C" 
{
#endif

#include "em_common/em_define.h"

#ifdef ENABLE_LIXIAN_ETM

#include "lixian/lixian_interface.h"
#include "et_interface/et_interface.h"
#include "asyn_frame/msg.h"
#include "em_common/em_utility.h"

/************************************************************************/
/*                    STRUCT DEFINE                                     */
/************************************************************************/

////////////////////////////////////////////////////
/* walbox �������� */
typedef struct t_lx_base
{
	_u64 _userid;	
	char _user_name[MAX_USER_NAME_LEN];
	char _user_name_old[MAX_USER_NAME_LEN];
	char _session_id[MAX_SESSION_ID_LEN];			
	_int32 _vip_level;					
	_int32 _net;			//���� ��0����
	_u32 _local_ip;
	_int32 _from;      		// 0 ҳ�棻1�ͻ��ˣ� 2 �ƶ��豸
	_int32 _section_type; //��������,0������;1����ͨ;10��С��Ӫ��;999��δ֪
} LX_BASE;

/* ���������������ṹ��,ȫ��Ψһ */
typedef struct t_lx_manager
{
	LX_BASE	_base;
	char _cookie[MAX_URL_LEN];
	char _download_cookie[MAX_URL_LEN];
	_u32 _jump_key_len;
	char _jump_key[LX_JUMPKEY_MAX_SIZE];

	_u32 _timer_id;	// ��ʱ����
	_u32 _cmd_protocal_seq;

	LIST _action_list;		// LX_PT

	_u32 _max_task_num;			/* ���û����߿ռ��ڿɴ��������������*/
	_u32 _current_task_num;			/* ���û����߿ռ��ڵ����������*/
	_u64 _total_space;				/* �û������ܿռ䣬��λ�ֽ�  */
	_u64 _available_space;				/* �û����߿��ÿռ䣬��λ�ֽ� */
	
	//_u32 _task_num;					/* _task_array ��������� */
	//LX_TASK_INFO * _task_array;		/* �����б�,����Ϊ_task_num*sizeof(LX_TASK_INFO)   */

	MAP _task_map;
	MAP _task_list_action_map;
	LIST _task_list;
	_u32 _update_task_timer_id;	// ��ʱ����
} LX_MANAGER;

/* ����ͷ��ʽ */
typedef struct _t_lx_cmd_header
{
	_u32 _version;       //Э��汾��
	_u32 _seq;           //���к�
	_u32 _len;           //����ȣ�������������ͷ�ĳ���
	_u32 _thunder_flag; //�ͻ��˰汾��ʶ
	_u16 _compress_flag;//ѹ����־
	_u16 _cmd_type;      //�������ͱ�ʶ
} LX_CMD_HEADER;

////////////////////////////////////////////////////

/* �����ض������Ľṹ�� */

/*  task list */
typedef struct t_lx_pt_task_ls
{
	LX_PT _action;			
	/* req */
	LX_GET_TASK_LIST _req;					

	/* resp */
	LX_TASK_LIST _resp;
} LX_PT_TASK_LS;

typedef struct _t_lx_new_get_task_list_req
{
	LX_CMD_HEADER _cmd_header;
	_u32 _jump_key_len;
	char _jump_key[LX_JUMPKEY_MAX_SIZE];
	_u64 _userid;
	_u8 _vip_level;

	LX_GET_TASK_LIST _task_list; 
}LX_NEW_GET_TASK_LIST_REQ;

typedef struct t_lx_pt_task_ls_new
{
	LX_PT _action;			
	/* req */
	LX_NEW_GET_TASK_LIST_REQ _req;					

	/* resp */
	LX_TASK_LIST _resp;
} LX_PT_TASK_LS_NEW;

/*  bt task sub file list */
typedef struct t_lx_pt_bt_ls
{
	LX_PT _action;			
	/* req */
	LX_GET_BT_FILE_LIST _req;					

	/* resp */
	LX_FILE_LIST _resp;
} LX_PT_BT_LS;

/*  screenshot download picture */
typedef struct t_lx_dl_file
{
	LX_PT  _action;		

	_int32 _gcid_index;
	
	char  _url[MAX_URL_LEN];
	//char  _ref_url[MAX_URL_LEN];
	//char  _cookie[MAX_URL_LEN];

	_int32 _section_type;
	
	LX_PT * _ss_action;		/* ָ�� ���ڵ�LX_PT_SS */
} LX_DL_FILE;

/*  screenshot */
typedef struct t_lx_pt_screenshot
{
	LX_PT _action;			
	/* req */
	LX_GET_SCREENSHOT _req;					

	/* resp */
	LX_SCREENSHOT_RESUTL _resp;
	_u32 _need_dl_num;  /* ʵ����Ҫ������������ͼƬ��gcid���� */
	_u32 _dling_num;  	/* �������ص�ͼƬ���� */
	_u32 _dled_num;  	/* ��������ص�ͼƬ���� */
	LX_DL_FILE * _dl_array;
} LX_PT_SS;

/*  vod url */
typedef struct t_lx_pt_vod_url
{
	LX_PT _action;			
	/* req */
	LX_GET_VOD _req;					

	/* resp */
	LX_GET_VOD_RESULT _resp;
} LX_PT_VOD_URL;

typedef struct _t_lx_get_task_list_req
{
	LX_CMD_HEADER _cmd_header;
	_u32 _jump_key_len;
	char _jump_key[LX_JUMPKEY_MAX_SIZE];
	_u64 _userid;
	_u8 _vip_level;
	_u64 _commit_time;
	_u64 _task_id;
	_u32 _flag;
	_u32 _retry_count;
	LX_GET_TASK_LIST _task_list; 
}LX_GET_TASK_LIST_REQ;

typedef struct t_lx_pt_task_id_ls
{
	LX_PT _action;			
	/* req */
	LX_GET_TASK_LIST_REQ _req;					

	/* resp */
	LX_TASK_LIST _resp;
} LX_PT_TASK_ID_LS;

/*  �ύ���������*/
typedef struct _t_lx_commit_task_req
{
	LX_CMD_HEADER _cmd_header;
	_u32 _jump_key_len;
	char _jump_key[LX_JUMPKEY_MAX_SIZE];
	_u64 _userid;
	_u8 _vip_level;
	LX_CREATE_TASK_INFO _task_info; 
}LX_COMMIT_TASK_REQ;

/*  task list */
typedef struct t_lx_pt_commit_task
{
	LX_PT _action;			
	/* req */
	LX_COMMIT_TASK_REQ _req;					
	
	/* resp */
	LX_CREATE_TASK_RESULT _resp;
} LX_PT_COMMIT_TASK;

/*  �ύBT���������*/
typedef struct _t_lx_commit_bt_task_req
{
	LX_CMD_HEADER _cmd_header;
	_u32 _jump_key_len;
	char _jump_key[LX_JUMPKEY_MAX_SIZE];
	_u64 _userid;
	_u8 _vip_level;	
	_u8 _info_hash[INFO_HASH_LEN];	
	char _bt_title_name[MAX_FILE_NAME_LEN];
	LX_CREATE_BT_TASK_INFO _task_info; 
}LX_COMMIT_BT_TASK_REQ;
/* bt task list */
typedef struct t_lx_pt_commit_bt_task
{
	LX_PT _action;			
	/* req */
	LX_COMMIT_BT_TASK_REQ _req;	
	/* resp */
	LX_CREATE_TASK_RESULT _resp;
} LX_PT_COMMIT_BT_TASK;

typedef struct _t_lx_delete_task_cmd
{
	LX_CMD_HEADER _cmd_header;
	_u32 _jump_key_len;
	char _jump_key[LX_JUMPKEY_MAX_SIZE];
	_u64 _userid;
	_u8 _vip_level;
	_u8 _flag;
	_u64 _task_id;

	void * _user_data;							/* �û����� */
	LX_DELETE_TASK_CALLBACK _callback_fun;		
}LX_DELETE_TASK_REQ;


typedef struct t_lx_pt_delete_task
{
	LX_PT _action;			
	/* req */
	LX_DELETE_TASK_REQ _req;					
	/* resp */
	LX_DELETE_TASK_RESULT _resp;
} LX_PT_DELETE_TASK;

/* ����ɾ������ */
typedef struct _t_lx_delete_tasks_cmd
{
	LX_CMD_HEADER _cmd_header;
	_u32 _jump_key_len;
	char _jump_key[LX_JUMPKEY_MAX_SIZE];
	_u64 _userid;
	_u8 _vip_level;
	_u8 _flag;
	_u32 _task_num;
	_u64 *_p_task_ids;

	void * _user_data;							/* �û����� */
	LX_DELETE_TASKS_CALLBACK _callback_fun;		
}LX_DELETE_TASKS_REQ;


typedef struct t_lx_pt_delete_tasks
{
	LX_PT _action;			
	/* req */
	LX_DELETE_TASKS_REQ _req;					
	/* resp */
	LX_DELETE_TASKS_RESULT _resp;
} LX_PT_DELETE_TASKS;

typedef struct _t_lx_delay_task_cmd
{
	LX_CMD_HEADER _cmd_header;
	_u32 _jump_key_len;
	char _jump_key[LX_JUMPKEY_MAX_SIZE];
	_u64 _userid;
	_u8 _vip_level;
	_u64 _task_id;
	
	void * _user_data;							/* �û����� */
	LX_DELAY_TASK_CALLBACK _callback_fun;		
}LX_DELAY_TASK_REQ;

typedef struct t_lx_pt_delay_task
{
	LX_PT _action;			
	/* req */
	LX_DELAY_TASK_REQ _req;					
	/* resp */
	LX_DELAY_TASK_RESULT _resp;
} LX_PT_DELAY_TASK;

//////////////////////////  ���񾫼��ѯ���ֽṹ  //////////////////////////////////
typedef struct _t_lx_miniquery_task_cmd
{
	LX_CMD_HEADER _cmd_header;
	_u32 _jump_key_len;
	char _jump_key[LX_JUMPKEY_MAX_SIZE];
	_u64 _userid;
	_u8 _vip_level;
	_u64 _task_id;
	
	void * _user_data;							/* �û����� */
	LX_MINIQUERY_TASK_CALLBACK _callback_fun;		
}LX_MINIQUERY_TASK_REQ;

typedef struct t_lx_pt_miniquery_task
{
	LX_PT _action;			
	/* req */
	LX_MINIQUERY_TASK_REQ _req;					
	/* resp */
	LX_MINIQUERY_TASK_RESULT _resp;
} LX_PT_MINIQUERY_TASK;


typedef struct _t_lx_get_user_info_task_cmd
{
    LX_CMD_HEADER _cmd_header;
    _u32 _jump_key_len;
    char _jump_key[LX_JUMPKEY_MAX_SIZE];
    _u64 _userid;
    _u8 _vip_level;
        
    void * _user_data;							/* �û����� */
    LX_LOGIN_CALLBACK _callback_fun;		
}LX_GET_USER_INFO_TASK_REQ;
    
typedef struct t_lx_pt_get_user_info_task
{
    LX_PT _action;			
    /* req */
    LX_GET_USER_INFO_TASK_REQ _req;					
    /* resp */
    LX_LOGIN_RESULT _resp;
} LX_PT_GET_USER_INFO_TASK;

typedef struct _t_lx_query_task_info_task_cmd
{
    LX_CMD_HEADER _cmd_header;
    _u32 _jump_key_len;
    char _jump_key[LX_JUMPKEY_MAX_SIZE];
    _u64 _userid;
    _u64 *_task_id;
	_u32 _task_num;
        
    void * _user_data;							/* �û����� */
    LX_QUERY_TASK_INFO_CALLBACK _callback_fun;		
}LX_QUERY_TASK_INFO_REQ;
/*  query task info */
typedef struct t_lx_pt_query_task_info
{
	LX_PT _action;			
	/* req */
	LX_QUERY_TASK_INFO_REQ _req;					
	/* resp */
	LX_QUERY_TASK_INFO_RESULT _resp;
} LX_PT_QUERY_TASK_INFO;

typedef struct _t_lx_batch_query_task_info_task_cmd
{
    LX_CMD_HEADER _cmd_header;
    _u32 _jump_key_len;
    char _jump_key[LX_JUMPKEY_MAX_SIZE];
    _u64 _userid;
    _u64 *_task_id;
	_u32 _task_num;
	_u32 _offset;
	_u64 _last_commit_time;
	_u64 _last_task_id;
        
    void * _user_data;							/* �û����� */
    LX_GET_TASK_LIST_CALLBACK _callback_fun;		
}LX_BATCH_QUERY_TASK_INFO_REQ;
/*  query task info */
typedef struct t_lx_pt_batch_query_task_info
{
	LX_PT _action;			
	/* req */
	LX_BATCH_QUERY_TASK_INFO_REQ _req;					
	/* resp */
	LX_TASK_LIST _resp;
} LX_PT_BATCH_QUERY_TASK_INFO;

	
/* get downloadable url */
typedef struct t_lx_get_dl_url
{
	LX_PT  _action;	
	
	void* _user_data;
	void* _callback_fun; 
	char  _url[MAX_URL_LEN];
	BOOL _is_canceled;
} LX_PT_GET_DL_URL;

/* save downloadable url in cache*/
typedef struct t_lx_dl_url
{
	_u32 _url_num;	
	EM_DOWNLOADABLE_URL* _dl_url_array;
} LX_DL_URL;

/* get regex*/
typedef struct t_lx_get_regex
{
	LX_PT _action;

	void* _user_data;
	char  _url[MAX_URL_LEN];
	BOOL _is_canceled;

	LIST* _website_list;
	EM_DETECT_SPECIAL_WEB *_detect_website;
	EM_DETECT_REGEX * _detect_rule;
}LX_PT_GET_REGEX;

/* get detect string*/
typedef struct t_lx_get_detect_string
{
	LX_PT _action;

	void* _user_data;
	char  _url[MAX_URL_LEN];
	BOOL _is_canceled;

	LIST* _website_list;
	EM_DETECT_SPECIAL_WEB *_detect_website;
	EM_DETECT_STRING* _detect_rule;
}LX_PT_GET_DETECT_STRING;


/*  �ύ���������*/
typedef struct _t_lx_get_od_or_del_req
{
	LX_CMD_HEADER _cmd_header;
	_u32 _jump_key_len;
	char _jump_key[LX_JUMPKEY_MAX_SIZE];
	_u64 _userid;
	_u8 _vip_level;
	LX_GET_OD_OR_DEL_TASK_LIST _list_info; 
}LX_GET_OD_OR_DEL_TASK_REQ;
/*  task list */
typedef struct t_lx_pt_get_od_or_del_task
{
	LX_PT _action;			
	/* req */
	LX_GET_OD_OR_DEL_TASK_REQ _req;					
	
	/* resp */
	LX_OD_OR_DEL_TASK_LIST_RESULT _resp;
} LX_PT_OD_OR_DEL_TASK;

typedef struct _t_lx_free_strategy_req
{
	_u32 _jump_key_len;
	char _jump_key[LX_JUMPKEY_MAX_SIZE];
	_u64 _userid;
	
	void *_user_data;
	LX_FREE_STRATEGY_CALLBACK _callback_fun;
}LX_FREE_STRATEGY_REQ;
typedef struct t_lx_pt_free_strategy
{
	LX_PT _action;		
	/* req */
	LX_FREE_STRATEGY_REQ _req;		
	/* resp */
	LX_FREE_STRATEGY_RESULT _resp;
} LX_PT_FREE_STRATEGY;

typedef struct _t_lx_free_experience_member_req
{
	_u64 _userid;
	LX_CS_AC_TYPE _type;
	void *_user_data;
	LX_FREE_EXPERIENCE_MEMBER_CALLBACK _callback_fun;
}LX_FREE_EXPERIENCE_MEMBER_REQ;
typedef struct t_lx_pt_free_experience_member
{
	LX_PT _action;		
	/* req */
	LX_FREE_EXPERIENCE_MEMBER_REQ _req;		
	/* resp */
	LX_FREE_EXPERIENCE_MEMBER_RESULT _resp;
} LX_PT_FREE_EXPERIENCE_MEMBER;

typedef struct t_lx_pt_get_high_speed_flux
{
	LX_PT _action;		
	/* req */
	LX_GET_HIGH_SPEED_FLUX _req;		
	/* resp */
	LX_GET_HIGH_SPEED_FLUX_RESULT _resp;
} LX_PT_GET_HIGH_SPEED_FLUX;

typedef struct _t_lx_take_off_flux_req
{
	LX_CMD_HEADER _cmd_header;
	_u32 _userid;
	_u32 _jump_key_len;
	char _jump_key[LX_JUMPKEY_MAX_SIZE];
	char _peer_id[MAX_OS_LEN];
	_u32 _business_flag;
	_u64 _main_taskid;
	_u64 _file_id;
	
	void *_user_data;
	LX_TAKE_OFF_FLUX_CALLBACK _callback_fun;
}LX_TAKE_OFF_FLUX_REQ;
typedef struct t_lx_pt_take_off_flux
{
	LX_PT _action;		
	/* req */
	LX_TAKE_OFF_FLUX_REQ _req;		
	/* resp */
	LX_TAKE_OFF_FLUX_RESULT _resp;
} LX_PT_TAKE_OFF_FLUX;

/*--------------------------------------------------------------------------*/
/*                Interfaces provid for other module				        */
/*--------------------------------------------------------------------------*/

_int32 lx_init_mgr(void);
_int32 lx_uninit_mgr(void);

/* �˳����߿ռ� */
_int32 lx_logout(void);

_int32 lx_set_base(_u64 user_id,const char * user_name,const char * old_user_name,_int32 vip_level,const char * session_id);

_int32 lx_set_sessionid(const char * session_id);
LX_MANAGER* lx_get_manager(void);
LX_BASE * lx_get_base(void);

BOOL lx_is_logined(void);

_int32 lx_set_download_cookie(const char * cookie);

_int32 lx_set_section_type(_int32 type);

_int32 lx_get_section_type(_int32 type);

_int32 lx_get_aes_key( char aes_key[MAX_SESSION_ID_LEN]);
_int32 lx_get_session_id( char sessionid[MAX_SESSION_ID_LEN]);
const char * lx_get_server_url(void);

_int32 lx_get_cookie_impl(char* cookie_buffer);

_u32 lx_get_cmd_protocal_seq(void);

_int32 lx_set_jumpkey(char* jumpkey, _u32 jumpkey_len);
_int32 lx_get_jumpkey(char* jumpkey_buffer, _u32* len);

_int32 lx_set_user_lixian_info(_u32 max_task_num,_u32 current_task_num,_u64 total_space,_u64 available_space);

/* ��ȡ�û����߿ռ���Ϣ*/
_int32 lx_get_space(_u64 * p_max_space,_u64 * p_available_space);
_int32 lx_get_task_num(_u32 * p_max_task_num,_u32 * p_current_task_num);

/////////////////////////////////////////////////////////////////////////////////////


_int32 lx_add_action_to_list(LX_PT * p_action);
_int32 lx_remove_action_from_list(LX_PT * p_action);
LX_PT *  lx_get_action_from_list(_u32 action_id);

_int32 lx_check_action_in_list(LX_PT * p_action);

_int32 lx_mini_http_post_callback(ET_HTTP_CALL_BACK * p_http_call_back_param);

_int32 lx_post_req(LX_PT * p_action,_u32 * p_http_id,_int32 priority);


_int32 lx_mini_http_get_callback(ET_HTTP_CALL_BACK * p_http_call_back_param);


_int32 lx_get_file_req(LX_PT * p_dl);

/*--------------------------------------------------------------------------*/
/*                Internal functions				        */
/*--------------------------------------------------------------------------*/
_int32 lx_start_dispatch(void);
_int32 lx_stop_dispatch(void);
_int32 lx_dispatch_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid);

_int32 lx_handle_action_list(void);

_int32 lx_action_finished(LX_PT *  p_action);

//////////////////////////////////////////////////////////
_int32 lx_cancel_action(LX_PT* p_action);

_int32 lx_clear_action_list(void);
_int32 lx_clear_action_list_except_sniff(void);

/* ����list xml�ļ��Ĵ���ļ��� */
_int32  lx_make_xml_file_store_dir( void);
_int32  lx_get_xml_file_store_path( char * xml_file_path_buffer);

/*  bt ���ļ�*/
_int32 lx_file_id_comp(void *E1, void *E2);
_int32 lx_add_file_to_map(MAP * p_map,LX_FILE_INFO * p_file);
LX_FILE_INFO * lx_get_file_from_map(MAP * p_map,_u64 file_id);
_int32 lx_get_file_array_from_map(MAP * p_map,LX_FILE_TYPE file_type,_int32 file_status,_u32 offset,_u32 file_num,LX_FILE_INFO ** pp_file_array);
_int32 lx_remove_file_from_map(MAP * p_map,_u64 file_id);
_int32 lx_clear_file_map(MAP * p_map);


///// ��ȡbt���������ļ�id �б�,*buffer_len�ĵ�λ��sizeof(_u64)
_int32 lx_get_bt_sub_file_ids(_u64 task_id,_int32 state,_u64 * id_array_buffer,_u32 *buffer_len);


/////��ȡ����BT������ĳ�����ļ�����Ϣ
_int32 lx_get_bt_sub_file_info(_u64 task_id, _u64 file_id, LX_FILE_INFO *p_file_info);


/* �����б� */
_int32 lx_malloc_ex_task(LX_TASK_INFO_EX ** pp_task);
_int32 lx_release_ex_task(LX_TASK_INFO_EX * p_task);

_int32 lx_task_id_comp(void *E1, void *E2);

_int32 lx_add_task_to_map(LX_TASK_INFO_EX * p_task);
_int32 lx_add_new_create_task_to_map(LX_TASK_INFO_EX * p_task);

LX_TASK_INFO_EX * lx_get_task_from_map(_u64 task_id);

_int32 lx_get_task_array_from_map(LX_FILE_TYPE file_type,_int32 file_status,_u32 offset,_u32 task_num,LX_TASK_INFO ** pp_task_array);
_int32 lx_remove_task_from_map(_u64 task_id);
_int32 lx_clear_task_map(void);
_u32 lx_num_of_task_in_map(void);
_u32 lx_num_of_task_in_list(void);
_int32 lx_get_all_task_id(_u64 * id_array_buffer, _u64 last_task_id);

_int32 lx_action_offset_comp(void *E1, void *E2);

_u32 lx_num_of_task_list_action_in_map_new(void);
_int32 lx_add_task_list_action_to_map_new(LX_PT_TASK_LS_NEW * p_action);
LX_PT_TASK_LS_NEW * lx_get_task_list_action_from_map_new(_u32 offset);
_int32 lx_remove_task_list_action_from_map_new(_u32 offset);
_int32 lx_clear_task_list_action_map_new(void);

_u32 lx_num_of_task_list_action_in_map(void);
_int32 lx_add_task_list_action_to_map(LX_PT_TASK_LS * p_action);
LX_PT_TASK_LS * lx_get_task_list_action_from_map(_u32 offset);
_int32 lx_remove_task_list_action_from_map(_u32 offset);
_int32 lx_clear_task_list_action_map(void);


///// ��������״̬��ȡ��������id �б�,stateȡֵ0:ȫ����1:��ѯ��������������2:���,*buffer_len�ĵ�λ��sizeof(_u64)
_int32 lx_get_task_ids_by_state( _int32 state,_u64 * id_array_buffer,_u32 *buffer_len);

/* ��ȡ����������Ϣ */
_int32 lx_get_task_info(_u64 task_id,LX_TASK_INFO * p_info);

/* ����״̬���� */
_int32 lx_start_update_task_dispatch(void);
_int32 lx_stop_update_task_dispatch(void);

_int32 lx_update_task_dispatch_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid);

_int32 lx_get_bt_task_lists(_int32 file_status);

_int32 lx_clear_task_list_not_free(void);


/***************************************************************************************************************************/
/***************************************************************************************************************************/
/***************************************************************************************************************************/
/* ����ҳ����̽�ɹ����ص�URL */

_int32 lx_url_key_comp(void *E1, void *E2);
_int32 lx_add_url_to_map(const char * url,_u32 url_num,EM_DOWNLOADABLE_URL * p_dl_url);
LX_DL_URL * lx_get_url_from_map(const char * url);
_int32 lx_clear_url_map(void);

_int32 lx_add_downloadable_url_to_map(EM_DOWNLOADABLE_URL * p_dl_url);
EM_DOWNLOADABLE_URL * lx_get_downloadable_url_from_map(_u32 url_id);
_int32 lx_clear_downloadable_url_map(void);

_int32 lx_set_downloadable_url_status_to_lixian(char *url);

	/* �ڱ������ؿ��в����Ƿ��Ѵ�����ͬ���� */
_int32 lx_get_downloadable_url_status_in_local(EM_DOWNLOADABLE_URL * p_dl_url);
	/* �����߿ռ��в����Ƿ��Ѵ�����ͬ���� */
_int32 lx_get_downloadable_url_status_in_lixian(EM_DOWNLOADABLE_URL * p_dl_url);

_int32 lx_get_downloadable_url_status(EM_DOWNLOADABLE_URL * p_dl_url);

///// ��ȡ��ҳ�пɹ�����URL��id �б�,*buffer_len�ĵ�λ��sizeof(u32)
_int32 lx_get_downloadable_url_ids( const char* url,_u32 * id_array_buffer,_u32 *buffer_len);

/* ��ȡ�ɹ�����URL ��Ϣ */
_int32 lx_get_downloadable_url_info(_u32 url_id,EM_DOWNLOADABLE_URL * p_info);

///// ����URL ,cid,gicd ��BT���ӵ�info_hash ���Ҷ�Ӧ����������id
_int32 lx_get_task_id_by_eigenvalue( EM_EIGENVALUE * p_eigenvalue ,_u64 * task_id);
///// ����gicd ���Ҷ�Ӧ����������id
_int32 lx_get_task_id_by_gcid( char * p_gcid ,_u64 * task_id, _u64 * file_id);

#endif /* ENABLE_LIXIAN */

#ifdef __cplusplus
}
#endif


#endif // !defined(__LIXIAN_IMPL_H_20111029)

