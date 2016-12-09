/*****************************************************************************
 *
 * Filename: 			bt_magnet_task.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2010/08/09
 *	
 * Purpose:				bt_magnet_task logic.
 *
 *****************************************************************************/

#if !defined(__BT_MAGNET_TASK_20100809)
#define __BT_MAGNET_TASK_20100809

#ifdef _cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#ifdef ENABLE_BT 
#include "utility/list.h"
#include "download_task/download_task.h"
#include "bt_download/bt_magnet/bt_magnet_data_manager.h"


struct  _tag_p2sp_task;

typedef enum tagMAGNET_TASK_STATUS
{
    MAGNET_TASK_RUNNING,
    MAGNET_TASK_SUCCESS,
    MAGNET_TASK_FAIL
} MAGNET_TASK_STATUS;

typedef struct tagBT_MAGNET_TASK
{
	enum TASK_TYPE _task_type;
    LIST _tracker_url;
    _u32 _task_id;
    _u8 _info_hash[INFO_HASH_LEN];
	BOOL _bManual;

    MAGNET_TASK_STATUS _task_status;

    enum TASK_RES_QUERY_STATE _res_query_bt_tracker_state;
    enum TASK_RES_QUERY_STATE _res_query_bt_dht_state;
    _u32 _query_bt_tracker_timer_id;
    _u32 _task_update_timer_id;
    
    struct  _tag_p2sp_task *_p2sp_task_ptr;
    CONNECT_MANAGER *_connect_manager_ptr;
    BT_MAGNET_DATA_MANAGER _data_manager;
	BOOL _tried_commit_to_lixian;
} BT_MAGNET_TASK;

typedef struct tagBT_MAGNET_TASK_MANAGER
{
    LIST _bt_magnet_task;
    _u32 _id_count;
} BT_MAGNET_TASK_MANAGER;

/* ui public platform */
_int32 bm_create_task( char *url, char *file_path, char * file_name, BOOL bManual, BT_MAGNET_TASK **pp_task , TASK* task);

_int32 bm_destory_task( BT_MAGNET_TASK *p_task );

_int32 bm_get_task_status( BT_MAGNET_TASK *p_task, MAGNET_TASK_STATUS *p_task_status );

_int32 bm_get_seed_full_path( BT_MAGNET_TASK *p_task, char *p_file_full_path_buffer, _u32 buffer_len );

_int32 bm_get_data_path( BT_MAGNET_TASK *p_task, char *p_data_path_buffer, _u32 buffer_len );

/* magnet pipe public platform */
_int32 bm_init_task( BT_MAGNET_TASK *p_task, char url[], char file_path[], char file_name[] ,BOOL bManual);

_int32 bm_uninit_task( BT_MAGNET_TASK *p_task );

/* private platform */
_int32 bmt_start( BT_MAGNET_TASK *p_bt_magnet_task, TASK* task);

_int32 bmt_stop( BT_MAGNET_TASK *p_bt_magnet_task );

_int32 bmt_update(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid);

_int32 bm_parse_url( char url[], _u32 url_len, _u8 infohash_buffer[], _u32 buffer_len,
    LIST *p_tracker_list );

_int32 bmt_res_query_bt_track( BT_MAGNET_TASK *p_bt_magnet_task );

_int32 bmt_bt_tracker_callback( void* user_data,_int32 errcode );

_int32 bmt_query_bt_tracker_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid);

_int32 bmt_stop_track( BT_MAGNET_TASK *p_bt_magnet_task );

_int32 bmt_res_query_dht( BT_MAGNET_TASK *p_bt_magnet_task );

_int32 bmt_stop_dht( BT_MAGNET_TASK *p_bt_magnet_task );

_int32 bmt_add_bt_resource( void* user_data, _u32 host_ip, _u16 port );

_int32 bmt_start_p2sp_download( BT_MAGNET_TASK *p_bt_magnet_task );

_int32 bmt_stop_p2sp_download( BT_MAGNET_TASK *p_bt_magnet_task );

_int32 bmt_start_create_pipes( BT_MAGNET_TASK *p_bt_magnet_task );

_int32 bmt_notify_torrent_ok( BT_MAGNET_TASK *p_bt_magnet_task, char *p_file_name );

BOOL bmt_verify_is_torrent_ok( BT_MAGNET_TASK *p_bt_magnet_task, char *p_file_name  );

_int32 bmt_generate_torrent_file( BT_MAGNET_TASK *p_bt_magnet_task, char *p_file_name );

#endif /* #ifdef ENABLE_BT */

#ifdef _cplusplus
}
#endif

#endif // !defined(__BT_MAGNET_TASK_20100809)

