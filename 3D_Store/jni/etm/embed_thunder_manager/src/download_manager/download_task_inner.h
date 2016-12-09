#if !defined(__DOWNLOAD_TASK_INNER_H_20090924)
#define __DOWNLOAD_TASK_INNER_H_20090924

#ifdef __cplusplus
extern "C" 
{
#endif

#include "em_common/em_define.h"

#include "download_manager/download_task_interface.h"
#include "et_interface/et_interface.h"
#include "torrent_parser/torrent_parser_interface.h"


/************************************************************************/
/*                    STRUCT DEFINE                                     */
/************************************************************************/
_int32 dt_id_comp(void *E1, void *E2);

_int32 dt_eigenvalue_comp(void *E1, void *E2);
_int32 dt_file_name_eigenvalue_comp(void *E1, void *E2);

////////////////////////////////////////////////////////////////////

_int32 dt_get_bt_sub_file_array_index(EM_TASK * p_task,_u16 file_index,_u16 *array_index);

_int32 dt_generate_eigenvalue(CREATE_TASK * p_create_param,_u8 * eigenvalue);

_int32 dt_generate_file_name_eigenvalue(char * file_path,_u32 path_len,char * file_name,_u32 name_len,_u32 * eigenvalue);
//_int32 dt_get_bt_eigenvalue(char * seed_file,_u8 * eigenvalue);

_int32 dt_get_url_eigenvalue(char * url,_u32 url_len,_u8 * eigenvalue);
_int32 dt_get_cid_eigenvalue(char * cid,_u8 * eigenvalue);
_int32 dt_get_file_eigenvalue(CREATE_TASK * p_create_param,_u8 * eigenvalue);

_int32 dt_check_and_sort_bt_file_index(_u32 *  src_need_dl_file_index_array,_u32 src_need_dl_file_num,_u32 file_total_num,_u16 ** pp_need_dl_file_index_array,_u16 * p_need_dl_file_num);
_int32 dt_get_all_bt_file_index(TORRENT_SEED_INFO *p_seed_info,_u16 ** pp_need_dl_file_index_array,_u16 * p_need_dl_file_num);
_int32 dt_init_task_info(CREATE_TASK * p_create_param,TASK_INFO ** pp_task_info ,_u32* p_task_id, BOOL create_by_cfg );

_int32 dt_uninit_task_info(TASK_INFO * p_task_info);
_int32 dt_init_bt_task_info(CREATE_TASK * p_create_param,TASK_INFO ** pp_task_info,_u8 *eigenvalue,_u32* p_task_id );
_int32 dt_uninit_bt_task_info(EM_BT_TASK * p_bt_task);

char * dt_get_file_name_from_url(char * url,_u32 url_length);

_int32 dt_init_p2sp_task_info(CREATE_TASK * p_create_param,TASK_INFO ** pp_task_info,_u8 *eigenvalue,_u32* p_task_id, BOOL create_by_cfg);
_int32 dt_uninit_p2sp_task_info(EM_P2SP_TASK * p_p2sp_task);

_int32 dt_init_task(TASK_INFO * p_task_info, EM_TASK ** pp_task);
_int32 dt_uninit_task(EM_TASK * p_task);

BOOL dt_is_task_exist(EM_TASK_TYPE type,_u8 * eigenvalue,_u32 * task_id);

_int32 dt_add_task_eigenvalue(EM_TASK_TYPE type,_u8 * eigenvalue,_u32 task_id);
_int32 dt_remove_task_eigenvalue(EM_TASK_TYPE type,_u8 * eigenvalue);
_int32 dt_get_task_tcid_from_et(EM_TASK * p_task);
_int32  dt_set_p2sp_task_tcid(EM_TASK * p_task,_u8 * tcid);
_int32  dt_set_p2sp_task_url(EM_TASK * p_task,char * new_url,_u32 url_len);

_int32 dt_get_task_file_name_from_et(EM_TASK * p_task);

_int32  dt_set_task_change(EM_TASK * p_task,_u32 change_flag);

_int32  dt_set_task_downloaded_size(EM_TASK * p_task,_u64 downloaded_size,BOOL save_dled_size);
_int32  dt_set_task_name(EM_TASK * p_task,char * new_name,_u32 name_len);
_int32  dt_set_p2sp_task_name(EM_TASK * p_task,char * new_name,_u32 name_len);
_int32  dt_set_bt_task_name(EM_TASK * p_task,char * new_name,_u32 name_len);
char * dt_get_task_file_path(EM_TASK * p_task);
char * dt_get_task_file_name(EM_TASK * p_task);
_int32  dt_set_task_file_size(EM_TASK * p_task,_u64 file_size);
_int32  dt_set_task_start_time(EM_TASK * p_task,_u32 start_time);
_int32  dt_set_task_finish_time(EM_TASK * p_task,_u32 finish_time);
_int32  dt_set_task_failed_code(EM_TASK * p_task,_u32 failed_code);

_int32 dt_set_task_state(EM_TASK * p_task,TASK_STATE new_state);

TASK_STATE dt_get_task_state(EM_TASK * p_task);

EM_TASK_TYPE dt_get_task_type(EM_TASK * p_task);
////////////////////////////////////

_int32 dt_update_bt_sub_file_info(EM_TASK * p_task);
_int32  dt_update_bt_running_file_in_cache(EM_TASK * p_task);
_int32  dt_init_bt_running_file(EM_TASK * p_task);

_int32  dt_update_bt_running_file(EM_TASK * p_task);
_int32  dt_find_next_bt_running_file(EM_TASK * p_task);

_int32 dt_correct_task_path(EM_TASK * p_task, char * path,_u32  path_len);
_int32 dt_check_task_free_disk(EM_TASK * p_task,char * path);


#ifdef __cplusplus
}
#endif


#endif // !defined(__DOWNLOAD_TASK_INNER_H_20090924)
