#if !defined(__DOWNLOAD_TASK_IMPL_H_20090922)
#define __DOWNLOAD_TASK_IMPL_H_20090922

#ifdef __cplusplus
extern "C" 
{
#endif

#include "em_common/em_define.h"
#include "download_manager/download_task_interface.h"
#include "download_manager/download_task_store.h"

//#include "task_manager/task_manager.h"
//#include "common/errcode.h"
//#include "common/settings.h"
//#include "connect_manager/resource.h"
//#include "data_manager/data_manager_interface.h"
//#include "asyn_frame/msg.h"
//#include "asyn_frame/notice.h"

/************************************************************************/
/*                    STRUCT DEFINE                                     */
/************************************************************************/

/*--------------------------------------------------------------------------*/
/*                Interfaces provid for task_manager				        */
/*--------------------------------------------------------------------------*/
_int32 dt_start_task_impl(EM_TASK * p_task);

_int32 dt_start_bt_task(EM_TASK * p_task);
_int32 dt_start_p2sp_task(EM_TASK * p_task);

_int32 dt_start_task_tag(EM_TASK * p_task,_int32 ret_value,_u8* user_data,_u32 user_data_len,const char * path);
_int32 dt_stop_task_impl(EM_TASK * p_task);

_int32 dt_delete_task_impl(EM_TASK * p_task);
_int32 dt_recover_task_impl(EM_TASK * p_task);

_int32 dt_destroy_task_impl(EM_TASK * p_task,BOOL delete_file);

_int32 dt_rename_task_impl(EM_TASK * p_task,char * new_name,_u32 new_name_len);

_int32 dt_add_resource_to_task_impl(EM_TASK * p_task,EM_RES* res);
_int32 dt_add_resource_to_task(EM_TASK * p_task,_u8* user_data,_u32 user_data_len);
_int32 dt_get_task_user_data_impl(EM_TASK * p_task,void * data_buffer,	_u32 buffer_len);
_int32 dt_get_task_common_user_data(_u8 * user_data,	_u32 user_data_len,_u8 ** common_user_data,_u32 * common_user_data_len);
EM_RES * dt_get_resource_from_user_data_impl(_u8 * user_data,_u32 user_data_len);
EM_RES * dt_get_resource_from_user_data(_u8 * user_data,_u32 user_data_len,_int32 index);
_int32 dt_add_server_resource_impl(EM_TASK * p_task,EM_SERVER_RES * p_resource);
_int32 dt_add_peer_resource_impl(EM_TASK * p_task, EM_PEER_RES * p_resource);
_int32 dt_get_peer_resource_impl(EM_TASK * p_task, EM_PEER_RES * p_resource);
_int32  dt_save_task_user_data_to_file(EM_TASK * p_task,_u8 * user_data,_u32  user_data_len);
_u16 dt_get_sizeof_extra_item(USER_DATA_ITEM_TYPE item_type);
_int32 dt_get_task_next_extra_item_pos(USER_DATA_ITEM_TYPE item_type,_u8 * user_data,	_u32 user_data_len,_u8 ** extra_item_pos);
_int32 dt_get_task_extra_item_pos(USER_DATA_ITEM_TYPE item_type,_u8 * user_data,	_u32 user_data_len,_u8 ** extra_item_pos);
_int32 dt_set_task_extra_item(EM_TASK * p_task,USER_DATA_ITEM_TYPE item_type,void * extra_item);
_int32 dt_get_task_extra_item(EM_TASK * p_task,USER_DATA_ITEM_TYPE item_type,void * extra_item);
#ifdef ENABLE_LIXIAN
_int32 dt_set_lixian_task_id_impl(EM_TASK * p_task,_u32 file_index,_u64 lixian_id);
_int32 dt_get_lixian_task_id_impl(EM_TASK * p_task,_u32 file_index,_u64 * p_lixian_id);
_int32 dt_get_task_lixian_id_pos(USER_DATA_ITEM_TYPE item_type,void * extra_item,_u8 * user_data,	_u32 user_data_len,_u8 ** extra_item_pos);
_int32 dt_set_task_lixian_mode_impl(EM_TASK * p_task,BOOL is_lixian);
_int32 dt_is_lixian_task_impl(EM_TASK * p_task,BOOL * is_lixian);
#endif /* ENABLE_LIXIAN */
/////////////////////// ¹ã¸æ////////////////////////////////
_int32 dt_set_task_ad_mode_impl(EM_TASK * p_task,BOOL is_ad);
_int32 dt_is_ad_task_impl(EM_TASK * p_task,BOOL * is_lixian);
//////////////////////  vod  //////////////////////////////////
_int32 dt_vod_get_download_mode_impl(_u8 * user_data, _u32 user_data_len, _u8 ** download_mode);
_int32 dt_vod_set_download_mode_impl(EM_TASK * p_task);
_int32 dt_set_task_download_mode(_u32 task_id,BOOL is_download,_u32 file_retain_time);
_int32 dt_get_task_download_mode(_u32 task_id,BOOL* is_download,_u32* file_retain_time);
_int32 dt_stop_vod_task_impl( EM_TASK * p_task );
_int32 dt_set_vod_cache_path_impl( const char *cache_path  );
char * dt_get_vod_cache_path_impl( void  );
char *dt_get_vod_cache_path_for_lan(void);
#if defined(ENABLE_NULL_PATH)
_int32 dt_et_get_task_info_impl(_u32 task_id, EM_TASK_INFO * p_em_task_info);
#endif
#ifdef ENABLE_HSC
_int32 dt_get_hsc_mode_impl(_u8 * user_data, _u32 user_data_len, _u8 ** hsc_mode);
_int32 dt_set_hsc_mode_impl(EM_TASK * p_task);
#endif /* ENABLE_HSC */

#ifdef __cplusplus
}
#endif


#endif // !defined(__DOWNLOAD_TASK_IMPL_H_20090922)
