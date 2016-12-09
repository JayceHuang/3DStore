

#if !defined(__BT_ACCELERATE_H_20090317)
#define __BT_ACCELERATE_H_20090317
#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"
#ifdef ENABLE_BT
#include "download_task/download_task.h"
#include "bt_download/bt_task/bt_task.h"


typedef struct tagBT_FILE_TASK_INFO
{
	enum BT_FILE_STATUS _file_status;
	//char file_suffix[MAX_SUFFIX_LEN];
	_u32  _res_query_timer_id;
	_u32 _last_query_stamp;
	_int32 _query_shub_times_sn;
	_int32 _query_shub_newres_times_sn;
	_int32 _res_query_retry_count;
	_int32     _error_code;
	enum TASK_RES_QUERY_STATE _res_query_shub_state;
	enum TASK_RES_QUERY_STATE _res_query_phub_state;
	enum TASK_RES_QUERY_STATE _res_query_tracker_state;
	enum TASK_RES_QUERY_STATE _res_query_partner_cdn_state;
    enum TASK_RES_QUERY_STATE _res_query_dphub_state;
    enum TASK_SHUB_QUERY_STEP _res_query_shub_step;
#ifdef ENABLE_HSC
	enum TASK_RES_QUERY_STATE _res_query_vip_hub_state;
#endif
	enum TASK_RES_QUERY_STATE _res_query_normal_cdn_state;
	_int32 _res_query_normal_cdn_cnt;
	BOOL   _is_gcid_ok;
	BOOL   _is_bcid_ok;
	BOOL   _is_add_rc_ok;

	/* Others need by reporter  */
	_int32 _origin_url_ip;
	_int32	_download_status; 
	_int32	_insert_course;
	BOOL is_shub_return_file_size; 
	_int32 _control_flag;
	RES_QUERY_PARA _res_query_para;

    DPHUB_QUERY_CONTEXT     _dpub_query_context;

} BT_FILE_TASK_INFO;



_int32 bt_start_accelerate( BT_TASK *p_bt_task,BT_FILE_INFO *p_file_info, _u32 file_index, BOOL* is_start_succ );
 _int32 bt_start_next_accelerate( BT_TASK *p_bt_task);
  _int32 bt_update_accelerate_sub( BT_TASK *p_bt_task,BT_FILE_TASK_INFO *p_file_task_info,_u32 file_index );
  _int32 bt_update_accelerate( BT_TASK *p_bt_task );
_int32 bt_stop_accelerate( BT_TASK *p_bt_task, _u32 file_index );
 _int32 bt_clear_accelerate( BT_TASK *p_bt_task );
 _int32 bt_stop_accelerate_sub( BT_TASK *p_bt_task ,BT_FILE_TASK_INFO *p_file_task_info, _u32 file_index );
_int32 file_task_info_map_compare( void *E1, void *E2 );
_int32 file_index_set_compare( void *E1, void *E2 );

_int32 bt_start_res_query_accelerate(BT_TASK * p_bt_task ,_u32 file_index,BT_FILE_TASK_INFO * p_file_task);
_int32 bt_stop_res_query_accelerate(BT_FILE_TASK_INFO *p_file_task_info);
_int32 bt_res_query_shub_callback(void* user_data,_int32 errcode, _u8 result, _u64 file_size, _u8* cid, _u8* gcid, _u32 gcid_level, _u8* bcid, _u32 bcid_size, _u32 block_size,_u32 retry_interval,char * file_suffix,_int32 control_flag);
_int32 bt_query_only_res_shub_callback(void* user_data,_int32 errcode, _u8 result, _u64 file_size, _u8* cid, _u8* gcid, _u32 gcid_level, _u8* bcid, _u32 bcid_size, _u32 block_size,_u32 retry_interval,char * file_suffix,_int32 control_flag);
_int32 bt_res_query_phub_callback(void* user_data,_int32 errcode, _u8 result, _u32 retry_interval);
_int32 bt_res_query_dphub_callback(void* user_data,_int32 errcode, _u16 result, _u32 retry_interval, BOOL is_continued);

#ifdef ENABLE_HSC
_int32 bt_res_query_vip_hub_callback(void* user_data,_int32 errcode, _u8 result, _u32 retry_interval);
#endif
#ifdef ENABLE_CDN
_int32 bt_res_query_cdn_manager_callback(_int32 errcode, void* user_data, _u8 result, BOOL is_end, char* host, _u16 port);
#endif /* ENABLE_CDN */
_int32 bt_res_query_tracker_callback(void* user_data,_int32 errcode, _u8 result, _u32 retry_interval,_u32 query_stamp);
_int32 bt_res_query_partner_cdn_callback(void* user_data,_int32 errcode, _u8 result, _u32 retry_interval);
_int32 bt_handle_query_accelerate_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid);
_int32 bt_file_task_failure_exit( BT_TASK *p_bt_task,_u32 file_index,_int32 err_code  );
_int32 bt_handle_wait_dphub_root_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid);

_int32 bt_ajust_accelerate_list( BT_TASK *p_bt_task);

#endif /* ENABLE_BT */

#ifdef __cplusplus
}
#endif

#endif // !defined(__BT_ACCELERATE_H_20090317)




