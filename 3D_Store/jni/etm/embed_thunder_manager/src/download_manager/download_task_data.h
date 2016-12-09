#if !defined(__DOWNLOAD_TASK_DATA_H_20090924)
#define __DOWNLOAD_TASK_DATA_H_20090924

#ifdef __cplusplus
extern "C" 
{
#endif

#include "em_common/em_define.h"

#include "download_manager/download_task_interface.h"

/************************************************************************/
/*                    STRUCT DEFINE                                     */
/************************************************************************/
/*--------------------------------------------------------------------------*/
/*                Interfaces provid for external program				        */
/*--------------------------------------------------------------------------*/

#ifdef ENABLE_ETM_MEDIA_CENTER
typedef struct tagTASK_TG_ID
{
	_u32 _task_id;
	_u64 _group_id;
	_int32 _task_state;
} TASK_TG_ID;
#endif


/*--------------------------------------------------------------------------*/
/*                Internal functions				        */
/*--------------------------------------------------------------------------*/
_int32 dt_init_slabs(void);
_int32 dt_uninit_slabs(void);
_int32 dt_init(void);

_int32 dt_uninit(void);
_int32 dt_load_tasks_from_file(void);

_int32 dt_save_tasks(void);
_int32 dt_have_task_changed(void);
_int32 dt_have_task_waiting_stop(void);
_int32 dt_have_task_failed(void);
_int32 dt_report_to_remote(void);

_int32 dt_load_order_list(void);
_int32 dt_save_order_list(void);

_int32 dt_get_total_task_num(void);
_int32 dt_save_total_task_num(void);
_int32 dt_set_need_notify_state_changed(BOOL is_need);
BOOL dt_is_need_notify_state_changed(void);
BOOL dt_is_running_tasks_loaded(void);
_int32 dt_load_running_tasks(void);
_int32 dt_save_running_tasks(BOOL clear);

_int32 dt_stop_tasks(void);

_int32 dt_task_info_malloc(TASK_INFO **pp_slip);

_int32 dt_task_info_free(TASK_INFO * p_slip);

_int32 dt_task_malloc(EM_TASK **pp_slip);

_int32 dt_task_free(EM_TASK * p_slip);

_int32 dt_bt_task_malloc(EM_BT_TASK **pp_slip);

_int32 dt_bt_task_free(EM_BT_TASK * p_slip);
_int32 dt_p2sp_task_malloc(EM_P2SP_TASK **pp_slip);
_int32 dt_p2sp_task_free(EM_P2SP_TASK * p_slip);
_int32 dt_bt_running_file_malloc(BT_RUNNING_FILE **pp_slip);

_int32 dt_bt_running_file_free(BT_RUNNING_FILE * p_slip);
_int32 dt_bt_running_file_safe_delete(EM_TASK * p_task);

////////////////////////////////////////////////////////////////////

_int32 dt_reset_task_id(void);
_u32 dt_create_task_id(BOOL is_remote);
_int32 dt_decrease_task_id(void);
BOOL dt_is_local_task(EM_TASK * p_task);
EM_TASK * dt_get_task_from_map(_u32 task_id);
_int32 dt_add_task_to_map(EM_TASK * p_task);

_int32 dt_remove_task_from_map(EM_TASK * p_task);
_int32 dt_clear_task_map(void);
_u32 dt_get_task_num_in_map(void);

BOOL dt_is_url_task_exist(_u8 * eigenvalue,_u32 * task_id);
BOOL dt_is_tcid_task_exist(_u8 * eigenvalue,_u32 * task_id);
BOOL dt_is_kankan_task_exist(_u8 * eigenvalue,_u32 * task_id);
BOOL dt_is_bt_task_exist(_u8 * eigenvalue,_u32 * task_id);
BOOL dt_is_file_exist(_u32  eigenvalue);
BOOL dt_is_file_task_exist(_u8 * eigenvalue,_u32 * task_id);
_int32 dt_add_file_task_eigenvalue(_u8 * eigenvalue,_u32 task_id);
_int32 dt_remove_file_task_eigenvalue(_u8 * eigenvalue);

_int32 dt_add_bt_task_eigenvalue(_u8 * eigenvalue,_u32 task_id);
_int32 dt_add_url_task_eigenvalue(_u8 * eigenvalue,_u32 task_id);

_int32 dt_add_tcid_task_eigenvalue(_u8 * eigenvalue,_u32 task_id);

_int32 dt_add_kankan_task_eigenvalue(_u8 * eigenvalue,_u32 task_id);
_int32 dt_remove_bt_task_eigenvalue(_u8 * eigenvalue);
_int32 dt_remove_url_task_eigenvalue(_u8 * eigenvalue);
_int32 dt_remove_tcid_task_eigenvalue(_u8 * eigenvalue);
_int32 dt_remove_kankan_task_eigenvalue(_u8 * eigenvalue);
_int32 dt_add_file_name_eigenvalue(_u32  eigenvalue,_u32 task_id);
_int32 dt_remove_file_name_eigenvalue(_u32  eigenvalue);
_int32 dt_clear_eigenvalue(void);

_int32 dt_add_task_to_order_list(EM_TASK * p_task);
_int32 dt_remove_oldest_task_from_order_list(void);
_int32 dt_remove_task_from_order_list(EM_TASK * p_task);
_int32 dt_clear_order_list(void);
_int32 dt_add_task_info_to_cache(TASK_INFO * p_task_info);
_int32 dt_remove_task_info_from_cache(TASK_INFO * p_task_info);
_int32 dt_clear_cache(void);

_int32 dt_add_file_path_to_cache(EM_TASK * p_task,const char * path,_u32 path_len);
char * dt_get_file_path_from_cache(EM_TASK * p_task);
_int32 dt_clear_file_path_cache(void);

_int32 dt_add_file_name_to_cache(EM_TASK * p_task,const char * file_name,_u32 name_len);
char * dt_get_file_name_from_cache(EM_TASK * p_task);
_int32 dt_remove_file_name_from_cache(EM_TASK * p_task);
_int32 dt_clear_file_name_cache(void);

_int32 dt_stop_over_running_task(void);
_int32 dt_set_max_running_task(_u32 max_tasks);
BOOL dt_is_running_task_full(void);
_int32 dt_add_running_task(EM_TASK * p_task);
_int32 dt_remove_running_task(EM_TASK * p_task);

_int32 dt_update_running_task(void);
_int32 dt_get_running_et_task_id(_u32 etm_task_id,_u32 * et_task_id);


_int32 dt_get_running_task(_u32 task_id,RUNNING_STATUS *p_status);
_int32 dt_get_download_speed(_u32 * speed);
_int32 dt_get_upload_speed(_u32 * speed);

_int32 dt_clear_dead_task(void);
_int32 dt_start_next_task(void);
/* 停掉最近启动的那个任务,并将它置为等待状态 */
_int32 dt_stop_the_latest_task( void );
/* 停掉另一个下载任务,并将它置为等待状态 */
_int32 dt_stop_the_other_download_task( _u32 task_id );

_int32 dt_stop_all_waiting_tasks_impl(void);
_int32 dt_have_task_waitting(void);
_u32 dt_get_running_task_num(void );
EM_TASK *  dt_get_pri_task(void);

////////////////////////////////////////////////////////////////////
_int32 dt_get_pri_id_list_impl(_u32 * id_array_buffer,_u32 * buffer_len);
_int32 dt_pri_level_up_impl(_u32 task_id,_u32 steps);

_int32 dt_pri_level_down_impl(_u32 task_id,_u32 steps);

_int32 dt_get_task_id_by_state_impl(TASK_STATE state,_u32 * id_array_buffer,_u32 *buffer_len,BOOL is_local);
_int32 dt_get_all_task_ids_impl(_u32 * id_array_buffer,_u32 *buffer_len);
_int32 dt_get_all_task_total_file_size_impl(_u64 * p_total_file_size);
_int32 dt_get_all_task_need_space_impl(_u64 * p_need_space);
#ifdef ENABLE_ETM_MEDIA_CENTER
_int32 dt_get_all_task_ids_and_group_id_impl( TASK_TG_ID *tg_array_buffer,_u32 *num, BOOL is_success_task );
#endif

//////////////////////  lan  //////////////////////////////////
BOOL dt_is_lan_task(EM_TASK * p_task);
_int32 dt_increase_running_lan_task_num(void);
_int32 dt_decrease_running_lan_task_num(void);
_u32 dt_get_running_lan_task_num( void );
_int32 dt_increase_waiting_lan_task_num(void);
_int32 dt_decrease_waiting_lan_task_num(void);
_u32 dt_get_waiting_lan_task_num( void );

//////////////////////  vod  //////////////////////////////////
_int32 dt_set_current_vod_task_id(_u32 task_id);
_u32 dt_get_current_vod_task_id(void);
_int32 dt_set_reading_success_file(BOOL is_reading_success_file);
BOOL dt_is_reading_success_file(void);

_int32 dt_destroy_vod_task( EM_TASK * p_task );
BOOL dt_is_vod_task(EM_TASK * p_task);
BOOL dt_is_vod_task_no_disk(EM_TASK * p_task);
_u32 dt_create_vod_task_id(void);
_int32 dt_decrease_vod_task_id(void);
_int32 dt_increase_vod_task_num(EM_TASK * p_task);
_int32 dt_decrease_vod_task_num(EM_TASK * p_task);
_int32 dt_reset_vod_task_num(void);
_u32 dt_get_vod_task_num( void );
_int32 dt_increase_running_vod_task_num(void);
_int32 dt_decrease_running_vod_task_num(void);
_u32 dt_get_running_vod_task_num( void );
_int32 dt_increase_used_vod_cache_size(EM_TASK * p_task);
_int32 dt_decrease_used_vod_cache_size(EM_TASK * p_task);
_int32 dt_reset_used_vod_cache_size(void);
_u32 dt_get_used_vod_cache_size( void );
_int32  dt_set_vod_cache_size_impl( _u32 cache_size );
_u32  dt_get_vod_cache_size_impl( void );
BOOL dt_check_enough_vod_cache_free_size( void );
_int32 dt_clear_vod_cache_impl(BOOL clear_all);
_int32 dt_load_task_vod_extra_mode(void);
_int32 dt_correct_all_tasks_path(void);
BOOL dt_can_failed_task_restart(EM_TASK * p_task);

#ifdef ENABLE_NULL_PATH 
_int32 dt_get_no_disk_vod_task_num();
void dt_inc_no_disk_vod_task_num();
void dt_dec_no_disk_vod_task_num();
#endif

/* 在所有任务中查找是否有相同cid的任务存在 */
BOOL dt_is_same_cid_task_exist(char * tcid);

#ifdef __cplusplus
}
#endif


#endif // !defined(__DOWNLOAD_TASK_DATA_H_20090924)
