#if !defined(__SCHEDULER_H_20090925)
#define __SCHEDULER_H_20090925

#ifdef __cplusplus
extern "C" 
{
#endif

#include "em_common/em_define.h"
#include "em_common/em_list.h"
#include "em_common/em_errcode.h"
#include "em_asyn_frame/em_msg.h"
#include "em_asyn_frame/em_notice.h"

#include "settings/em_settings.h"
#include "download_manager/download_task_interface.h"
#include "download_manager/mini_task_interface.h"

#include "platform/sd_network.h"

/* Pointer of the task_manager,only one task_manager when program is running */
typedef _int32 ( *EM_NOTIFY_TASK_STATE_CHANGED)(_u32 task_id, EM_TASK_INFO * p_task_info);


typedef struct t_em_torrent_file_info
{
	_u32 _file_index;
	char *_file_name;
	_u32 _file_name_len;
	char *_file_path;
	_u32 _file_path_len;
	_u64 _file_offset;
	_u64 _file_size;
} EM_TORRENT_FILE_INFO;
typedef struct t_em_torrent_seed_info
{
	char _title_name[MAX_FILE_NAME_LEN];
	_u32 _title_name_len;
	_u64 _file_total_size;
	_u64 _file_num;
	_u32 _encoding;								/* 种子文件编码格式，GBK = 0, UTF_8 = 1, BIG5 = 2 */
	_u8 _info_hash[20];
	EM_TORRENT_FILE_INFO *file_info_array_ptr;
} EM_TORRENT_SEED_INFO;


typedef _int32 ( *EM_NOTIFY_NET_CONNECT_RESULT)(_u32 iap_id,_int32 result,_u32 net_type);

/* Structures for etm 
typedef struct t_etm
{
	ETM_TASK_MANAGER _task_mgr;
	ETM_TREE_MANAGER _tree_mgr;

	//NOTICE_QUEUE g_msg_queue;

	//QUEUE g_thread_msg_queue;

	//SEVENT_HANDLE g_thread_msg_signal;
	_u32  _update_info_time_stamp;
	u32 _license_result;
}ETM;

*/
/* 上报状态 */
typedef enum t_em_http_report_state
{
	EHRS_WAITING=0, 
	EHRS_RUNNING, 
	EHRS_PAUSED,
	EHRS_SUCCESS, 
	EHRS_FAILED
} EM_HTTP_REPORT_STATE;
typedef struct t_em_http_report_info
{
	_u32 _http_id;
	_u32 _node_id;
	_u32 _errcode;
	_u32 _retry_num;
	EM_HTTP_REPORT_STATE _state;
	char _url[MAX_URL_LEN];
	char _recv_buffer[16384];
} EM_HTTP_REPORT_INFO;

/*--------------------------------------------------------------------------*/
/*                Structures for posting functions				        */
/*--------------------------------------------------------------------------*/
///////////////////////////////////////////////////////////////////////////////////////


#ifdef ENABLE_ETM_MEDIA_CENTER
typedef struct tagEM_SYS_VOLUME_INFO
{
	_u32 _volume_index;
	char  _volume_name[MAX_SYS_VOLUME_LEN];
	char  _volume_path[MAX_FILE_PATH_LEN];
} EM_SYS_VOLUME_INFO;
#endif

/*--------------------------------------------------------------------------*/
/*                Internal functions				        */
/*--------------------------------------------------------------------------*/
_int32 em_init( void * _param );
_int32 em_uninit( void * _param );

/* 下载库全局基础模块的初始化 */
_int32 em_basic_init( void  );
_int32 em_basic_uninit(void);

/* 下载库功能模块的初始化 */
_int32 em_sub_module_init( void  );
_int32 em_sub_module_uninit(void);

/* 下载库独立模块的初始化 */
_int32 em_other_module_init( void  );
_int32 em_other_module_uninit(void);

/* task_manager 模块的初始化 */
_int32 em_init_task_manager( void * _param );
_int32 em_uninit_task_manager(void);

_int32 em_init_sys_path_info();
_int32 em_uninit_sys_path_info();

_int32 em_post_next(msg_handler handler,_u32 timeout);
_int32 em_do_next(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid);
_int32 em_clear(void* p_param);
_int32 em_scheduler(void);
 _int32 em_scheduler_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid);
char * em_get_system_path(void);
BOOL em_is_et_running(void);
BOOL em_is_license_ok(void);
#ifdef ENABLE_ETM_MEDIA_CENTER
BOOL em_is_multi_disk(void);
#endif
BOOL em_is_task_auto_start(void);
void em_user_set_netok(BOOL is_net_ok);
BOOL em_is_user_netok(void);

BOOL em_is_net_ok(BOOL start_connect);
_int32 em_set_network_cnt_notify_callback(void* p_param);
_int32 em_init_network_impl(_u32 iap_id,EM_NOTIFY_NET_CONNECT_RESULT call_back_func);
_int32 em_init_network(void* p_param);
_int32 em_uninit_network_impl(BOOL user_abort);
_int32 em_uninit_network(void* p_param);
_int32 	em_get_network_status(void * p_param);
_int32 	em_get_network_iap(void * p_param);
_int32 	em_get_network_iap_from_ui(void * p_param);
_int32 	em_get_network_iap_name(void * p_param);
_int32 em_get_network_flow(_u32 * download,_u32 * upload);
_int32 	em_set_peerid(void * p_param);
_int32 	em_get_peerid(void * p_param);
_int32 em_set_net_type(void* p_param);
_int32 em_notify_init_network(_u32 iap_id,_int32 result,_u32 net_type);



_int32 em_set_customed_interfaces(_int32 fun_idx, void *fun_ptr);
_int32 em_notify_license_result(_u32  result, _u32  expire_time);
_int32 	em_set_license(void * p_param);
_int32 	em_get_license(void * p_param);
_int32 	em_get_license_result(void * p_param);
_int32 	em_set_default_encoding_mode(void * p_param);
_int32 	em_get_default_encoding_mode(void * p_param);
_int32 	em_set_download_path(void * p_param);
_int32 	em_get_download_path(void * p_param);
_int32  em_get_download_path_imp(char *download_path);
#ifdef ENABLE_ETM_MEDIA_CENTER
_int32  em_get_download_volume_path(char *download_path);
_int32  em_set_download_path_volume(char *download_volume);
_int32  em_get_all_disk_volume( LIST *p_volume_list );
_int32  em_get_default_path_space( _u32 *p_size );
_int32  em_get_disk_download_path( char *p_path );

#endif

_int32 em_set_task_state_changed_callback(void* p_param);
#ifdef ENABLE_ETM_MEDIA_CENTER
void em_set_task_state_changed_callback_ptr(EM_NOTIFY_TASK_STATE_CHANGED callback_function_ptr);
#endif
_int32 em_notify_task_state_changed(_u32 task_id, EM_TASK_INFO * p_task_info);
_int32 em_load_default_settings(void* p_param);
_int32 em_set_max_tasks(void* p_param);
_int32 em_get_max_tasks(void* p_param);
_int32 em_set_download_limit_speed(void* p_param);
_int32 em_get_download_limit_speed(void* p_param);
_int32 em_set_upload_limit_speed(void* p_param);
_int32 em_get_upload_limit_speed(void* p_param);
_int32 em_set_auto_limit_speed(void* p_param);
_int32 em_get_auto_limit_speed(void* p_param);
_int32 em_set_max_task_connection(void* p_param);
_int32 em_get_max_task_connection(void* p_param);
_int32 em_set_task_auto_start(void* p_param);
_int32 em_get_task_auto_start(void* p_param);
_int32 	em_get_torrent_seed_info( void * p_param);
_int32 	em_release_torrent_seed_info( void * p_param);
_int32 	em_extract_ed2k_url( void * p_param);
_int32 em_set_download_piece_size(void* p_param);
_int32 em_get_download_piece_size(void* p_param);
_int32 em_set_ui_version(void* p_param);
_int32 em_get_business_type(_int32 product_id);
/* 上报相关 */
_int32 em_reporter_mobile_user_action_to_file(void* p_param);
_int32 em_reporter_mobile_enable_user_action_report(void* p_param);
_int32 em_http_report(void* p_param);
_int32 em_set_username_password(char *name, char *password);
_int32 em_set_uid(_u64 uid);
#ifdef ENABLE_TEST_NETWORK
void em_main_login_server_report_handler(void* arg);
void em_vip_login_server_report_handler(void* arg);
void em_portal_server_report_handler(void* arg);
void em_ipad_interface_server_report_handler(void* arg);
#endif

#ifdef ENABLE_PROGRESS_BACK_REPORT
void em_download_progress_back_report_handler(void* arg);
#endif

_int32 em_http_etm_inner_report(const char* first_u, const char* str_begin_from_u1);
_int32 em_http_report_impl(char* data,_u32 data_len);
_int32 em_http_report_by_url(void* p_param);
_int32 em_http_report_resp(EM_HTTP_CALL_BACK* param);
_int32 em_http_report_save_to_file(char* data, _u32 data_len);
_int32 em_http_report_from_file(void);
_int32 em_http_report_add_action_to_list(EM_HTTP_REPORT_INFO * p_action);
_int32 em_http_report_handle_action_list(BOOL clear_all);
_int32 em_http_report_clear_action_list(void);

_int32 em_set_prompt_tone_mode(void* p_param);
_int32 em_get_prompt_tone_mode(void* p_param);
_int32 em_set_p2p_mode(void* p_param);
_int32 em_get_p2p_mode(void* p_param);
_int32 em_set_cdn_mode(void* p_param);
_int32 em_set_host_ip(void* p_param);
_int32 em_clear_host_ip(void* p_param);

_int32 em_set_max_tasks_impl(_u32 max_tasks);

_int32 	em_start_et( void );
_int32 	em_set_et_license( void );
_int32 em_set_et_license_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid);
_int32 	em_set_et_config( void );
_int32 	em_stop_et( void );
_int32 	em_restart_et( void );
_int32 	em_start_et_sub_step( void );
_int32 	em_stop_et_sub_step( void );

#if defined(ENABLE_NULL_PATH)
_int32 	em_force_stop_et( void );
#endif

_int32 em_save_license(const char * license);
_int32 em_load_license(char * license_buffer );

/* drm */
_int32 	em_set_certificate_path(void * p_param);
_int32 	em_open_drm_file(void * p_param);
_int32 	em_is_certificate_ok(void * p_param);
//_int32 	em_read_drm_file(void * p_param);
_int32 	em_read_drm_file(_u32 drm_id, char *p_buffer, _u32 size, 
    _u64 file_pos, _u32 *p_read_size );

_int32 	em_close_drm_file(void * p_param);
_int32 em_set_openssl_rsa_interface(_int32 fun_count, void *fun_ptr_table);

#ifdef _LOGGER
_int32 em_log_reload_initialize();
_int32 em_log_reload_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid);
#endif

#ifdef ENABLE_ETM_MEDIA_CENTER
_int32 em_start_remote_control();
_int32 em_upgrade_initialze();
_int32 em_upgrade_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid);
_int32 em_down_upgrade_files(BOOL wait);
#define MEDIA_CENTER_WEBUI_PATH	"/usr/local/etc/webui"
#define FIRST_TIME_UPGRADE_CHECK_DELAY 120
#define MEDIA_CENTER_UPGRADE_SCRIPT	MEDIA_CENTER_WEBUI_PATH"/auto_upgrade.sh"
#endif

/* 预占1MB 的空间用于确保程序能顺利启动 */
_int32 em_ensure_free_disk( char *system_path);


_int32 em_start_search_server(void* p_param);
_int32 em_stop_search_server(void* p_param);
_int32 em_restart_search_server(void* p_param);

//////////////////////////////////////////////////////////////
/////17 手机助手相关的http server

/* 网下载库设置回调函数 */
_int32 em_assistant_set_callback_func(void* p_param);

/* 启动和关闭http server */
_int32 em_start_http_server(void* p_param);
_int32 em_stop_http_server(void* p_param);
_int32 em_http_server_add_account(void* p_param);
_int32 em_http_server_add_path(void* p_param);
_int32 em_http_server_get_path_list(void* p_param);
_int32 em_http_server_set_callback_func(void* p_param);
_int32 em_http_server_send_resp(void* p_param);
_int32 em_http_server_cancel(void* p_param);


/* 发送响应 */
_int32 em_assistant_send_resp_file(void* p_param);

/* 取消异步操作 */
_int32 em_assistant_cancel(void* p_param);


/* et 通知etm强制调度 */
_int32 em_scheduler_by_et(void* p_param);

#ifdef __cplusplus
}
#endif


#endif // !defined(__SCHEDULER_H_20090925)
