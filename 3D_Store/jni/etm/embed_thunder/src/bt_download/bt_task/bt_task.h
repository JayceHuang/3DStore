/*****************************************************************************
 *
 * Filename: 			bt_task.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2008/09/20
 *	
 * Purpose:				Basic bt_task_imp struct.
 *
 *****************************************************************************/

#if !defined(__BT_TASK_20080920)
#define __BT_TASK_20080920

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#ifdef ENABLE_BT

#include "download_task/download_task.h"
#include "bt_download/bt_task/bt_task_interface.h"
#include "torrent_parser/torrent_parser_interface.h"
#include "bt_download/bt_data_manager/bt_data_manager_interface.h"
#include "bt_download/bt_data_manager/bt_data_manager.h"
#include "utility/map.h"
#include "utility/list.h"
#include "utility/rw_sharebrd.h"

 struct tagBT_FILE_TASK_INFO;

struct tagBT_MAGNET_TASK;
typedef struct tagBT_TASK
{
	TASK _task;
	BT_DATA_MANAGER _data_manager;  
	struct tagTORRENT_PARSER *_torrent_parser_ptr;
	MAP _file_info_map; /* struct tagBT_FILE_INFO *_file_info_ptr; */
	MAP _file_task_info_map;
	LIST _query_para_list;
	LIST * _tracker_url_list;
	BOOL _is_deleting_task;
	enum TASK_RES_QUERY_STATE _res_query_bt_tracker_state;
	enum TASK_RES_QUERY_STATE _res_query_bt_dht_state;
	_u32  _info_query_timer_id;
	_u32 _info_query_times;
	_u32  _query_bt_tracker_timer_id;
	_u32  _query_dht_timer_id;

	RW_SHAREBRD *_brd_ptr;
	void * _file_info_for_user;
	_u32 _file_num;

	struct tagBT_MAGNET_TASK *_magnet_task_ptr;
    _u32 _encoding_switch_mode;
    BOOL _is_magnet_task;
} BT_TASK;

struct  tag_BT_FILE_INFO_POOL;

_int32 bt_check_task_para( BT_TASK *p_bt_task, struct tagTM_BT_TASK *p_task_para );

_int32 bt_update_task_info( BT_TASK *p_bt_task );

_int32 bt_init_file_info( BT_TASK *p_bt_task, struct tagTM_BT_TASK *p_task_para );
_int32 bt_uninit_file_info( BT_TASK *p_bt_task );
_int32 bt_update_file_info( BT_TASK *p_bt_task );
_int32 bt_update_file_info_sub(  BT_TASK *p_bt_task,BT_FILE_INFO *p_file_info );

_int32 bt_init_tracker_url_list( BT_TASK *p_bt_task );
_int32 bt_clear_tracker_url_list( BT_TASK *p_bt_task );

_int32 bt_delete_tmp_file( BT_TASK *p_bt_task );
	
_int32 init_bt_file_info_slabs(void);
_int32 uninit_bt_file_info_slabs(void);
_int32 bt_file_info_malloc_wrap(BT_FILE_INFO **pp_node);
_int32 bt_file_info_free_wrap(BT_FILE_INFO *p_node);

_int32 init_bt_file_task_info_slabs(void);
_int32 uninit_bt_file_task_info_slabs(void);
_int32 bt_file_task_info_malloc_wrap(struct tagBT_FILE_TASK_INFO **pp_node);
_int32 bt_file_task_info_free_wrap(struct tagBT_FILE_TASK_INFO *p_node);

_int32 init_bt_query_para_slabs(void);
_int32 uninit_bt_query_para_slabs(void);
_int32 bt_query_para_malloc_wrap(RES_QUERY_PARA **pp_node);
_int32 bt_query_para_free_wrap(RES_QUERY_PARA *p_node);

_int32 init_bt_task_slabs(void);
_int32 uninit_bt_task_slabs(void);
_int32 bt_task_malloc_wrap(BT_TASK **pp_node);
_int32 bt_task_free_wrap(BT_TASK *p_node);

_int32 bt_file_info_for_user_malloc_wrap(void **pp_file_info_for_user,_u32 number,_u32 *file_index_array);
_int32 bt_file_info_for_user_free_wrap(void *p_file_info_for_user);
_int32 bt_file_info_for_user_write( BT_TASK *p_bt_task,BT_FILE_INFO* p_file_info );
_int32 bt_file_info_for_user_read(struct  tag_BT_FILE_INFO_POOL* p_bt_file_info_pool,_u32 file_index, void *p_file_info );

#ifdef UPLOAD_ENABLE
_int32 bt_add_record_to_upload_manager(TASK* p_task ,_u32 file_index);
#endif

#endif /* #ifdef ENABLE_BT */

#ifdef __cplusplus
}
#endif

#endif // !defined(__BT_TASK_20080920)



