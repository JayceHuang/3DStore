#ifndef _EMULE_INTERFACE_H_
#define _EMULE_INTERFACE_H_

#ifdef ENABLE_EMULE
#include "utility/define.h"
#include "asyn_frame/notice.h"

#include "emule/emule_pipe/emule_pipe_interface.h"
#include "emule/emule_utility/emule_ed2k_link.h"
#include "emule/emule_data_manager/emule_data_manager.h"
#include "download_task/download_task.h"

struct tagEMULE_DATA_MANAGER;
struct tagRESOURCE;

typedef struct tagTM_EMULE_PARAM
{
	SEVENT_HANDLE	_handle;
	_int32			_result;
	_u32*			_task_id;	
	char				_ed2k_link[MAX_ED2K_LEN + 1];
	char				_file_path[MAX_FILE_PATH_LEN + 2];
	char				_file_name[MAX_FILE_NAME_LEN + 1];
} TM_EMULE_PARAM;

typedef struct tagEMULE_TASK
{
	TASK			_task;
	struct tagEMULE_DATA_MANAGER* _data_manager;
	RES_QUERY_PARA	_query_param;
	LIST				_peer_source_list;
    
 	enum TASK_RES_QUERY_STATE _res_query_emulehub_state;
 	enum TASK_RES_QUERY_STATE _res_query_shub_state;
	enum TASK_RES_QUERY_STATE _res_query_phub_state;
	enum TASK_RES_QUERY_STATE _res_query_tracker_state;
	enum TASK_RES_QUERY_STATE _res_query_partner_cdn_state; 
    enum TASK_RES_QUERY_STATE _res_query_dphub_state;
#ifdef ENABLE_HSC
	enum TASK_RES_QUERY_STATE _res_query_vip_hub_state; 
#endif /* ENABLE_HSC */
	enum TASK_RES_QUERY_STATE _res_query_normal_cdn_state;
	_int32 _res_query_normal_cdn_cnt;
	enum TASK_RES_QUERY_STATE _res_query_emule_tracker_state;
	_u32 _last_query_stamp;
    _u32 _last_query_emule_tracker_stamp;
    _u32 _query_shub_times_sn;
    _u32 _query_shub_newres_times_sn;
    _u32 _res_query_shub_retry_count;
    BOOL _is_need_report;

#ifdef UPLOAD_ENABLE
	BOOL   _is_add_rc_ok;  // add record to upload manager
#endif

    DPHUB_QUERY_CONTEXT     _dpub_query_context;

    _u32 _requery_emule_tracker_timer;
}EMULE_TASK;

typedef struct tagEMULE_RESOURCE
{
	RESOURCE		_base_res;
	_u32			_server_ip;
	_u32			_client_id;
	_u16			_client_port;
	_u16			_server_port;
	_u8				_user_id[USER_ID_SIZE];
}EMULE_RESOURCE;

_int32 emule_init_modular(void);

_int32 emule_uninit_modular(void);

_int32 emule_create_task(TM_EMULE_PARAM* param, TASK** task);

_int32 emule_start_task(TASK* task);

_int32 emule_stop_task(TASK* task);

_int32 emule_delete_task(TASK* task);

BOOL emule_can_abandon_resource(void* data_manager, struct tagRESOURCE* res);

BOOL emule_is_task_exist_by_gcid(TASK* task, const _u8* gcid);

_int32 emule_update_task_info(TASK* task);

void emule_get_peer_hash_value(_u32 ip, _u16 port, _u32* result);

_int32 emule_resource_create(RESOURCE** res, _u32 ip, _u16 port, _u32 server_ip, _u16 server_port);
_int32 emule_resource_destroy(RESOURCE** resource);

#ifdef _CONNECT_DETAIL
_int32 emule_pipe_get_peer_pipe_info(DATA_PIPE* pipe, PEER_PIPE_INFO* info);
#endif

_int32 emule_get_task_file_name(TASK* p_task, char *file_name_buffer, _int32* buffer_size);

void emule_set_enable( BOOL is_enable );
_int32 emule_get_task_gcid(TASK *p_task, _u8 *gcid);
_int32 emule_get_task_cid(TASK *p_task, _u8 *cid);

_int32 emule_set_notify_event_callback( void * call_ptr );

_int32 emule_cancel_emule_tracker_timer(TASK *task);

BOOL compare_emule_resource(EMULE_RESOURCE *res1, EMULE_RESOURCE *res2);

BOOL emule_is_my_task(TASK *p_task, _u8 file_id[FILE_ID_SIZE], const _u8* gcid);

_int32 emule_nat_server_notask_hook(const _int32 task_state);

_int32 emule_just_query_file_info(void* user_data, const char * url);
_int32 emule_just_query_file_info_resp(void* user_data,_int32 result,_u8 * cid,_u8* gcid,_u32 gcid_level,_u32 control_flag);


#endif
#endif


