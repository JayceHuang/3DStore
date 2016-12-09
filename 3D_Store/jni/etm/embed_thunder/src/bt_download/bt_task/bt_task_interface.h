
/*****************************************************************************
 *
 * Filename: 			bt_task_interface.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2008/09/20
 *	
 * Purpose:				Basic bt_task_interface struct.
 *
 *****************************************************************************/

#if !defined(__BT_TASK_INTERFACE_20080920)
#define __BT_TASK_INTERFACE_20080920

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"

#ifdef ENABLE_BT

#include "asyn_frame/notice.h"
#include "download_task/download_task.h"


enum BT_FILE_STATUS { BT_FILE_IDLE, BT_FILE_DOWNLOADING,  BT_FILE_FINISHED, BT_FILE_FAILURE };
//enum BT_HUB_QUERY_RESULT { BT_HUB_QUERY_IDLE, BT_HUB_QUERY_WORKING, BT_HUB_QUERY_SUCCESS, BT_HUB_QUERY_FAILTURE };
enum BT_ACCELERATE_STATE { BT_NO_ACCELERATE=0, BT_ACCELERATING, BT_ACCELERATED, BT_ACCELERATE_FAILED,BT_ACCELERATE_END };

typedef struct  t_et_bt_file_info
{
	_u32 _file_index;
	_u64 _file_size;	
	_u64 _downloaded_data_size; 		 /*已下载的数据大小*/  
	_u64 _written_data_size; 			 /*已写进磁盘的数据大小*/  
	_u32 _file_percent;	
	_u32 _file_status;
	_u32 _query_result;
	_u32 _sub_task_err_code;
	BOOL _has_record;
	_u32 _accelerate_state;
}ET_BT_FILE_INFO;  /*  This struct just for user */

typedef struct tagBT_FILE_INFO
{
	_u32 _file_index;
	char *_file_name_str;
	_u32 _file_name_len;
	_u64 _file_size;	
	_u64 _downloaded_data_size; 		 /*已下载的数据大小*/  
	_u64 _written_data_size; 			 /*已写进磁盘的数据大小*/  
	//BOOL _is_need_download;	
	_u32 _file_percent;	
	enum BT_FILE_STATUS _file_status;
	enum TASK_RES_QUERY_STATE _query_result;
	_u32 _sub_task_err_code;
	_int32 _control_flag;
	BOOL _has_record;
	enum BT_ACCELERATE_STATE _accelerate_state;
	BOOL _need_update_to_user;
} BT_FILE_INFO;

typedef struct tagTM_BT_TASK
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	char _seed_full_path[MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN];
	_u32 _seed_full_path_len;
	char _data_path[MAX_FILE_PATH_LEN];
	_u32 _data_path_len;
	_u32 *_download_file_index_array;
	_u32 _file_num;
	_u32 _encoding_switch_mode;
	_u32 *_task_id;	
} TM_BT_TASK;

typedef struct tagTM_BT_MAGNET_TASK
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	char _url[MAX_BT_MAGNET_LEN];
    _u32 _url_len;
	char _data_path[MAX_FILE_PATH_LEN];
	_u32 _data_path_len;
	char _data_name[MAX_FILE_NAME_LEN];
	_u32 _data_name_len;
	BOOL _bManual;
	_u32 _encoding_switch_mode;
	_u32 *_task_id;	
} TM_BT_MAGNET_TASK;

struct  tag_BT_FILE_INFO_POOL;

_int32 init_bt_download_module(void);
_int32 uninit_bt_download_module(void);
void bt_set_enable( BOOL is_enable );
_int32 bt_create_task( TASK **pp_task );
_int32 bt_init_task( TM_BT_TASK *p_task_para, TASK *p_task, BOOL *is_wait_close );
_int32 bt_init_magnet_task( TM_BT_MAGNET_TASK *p_task_para, TASK *p_task );
_int32 bt_init_magnet_core_task( TASK *p_task );

_int32 bt_start_task( struct _tag_task *p_task );
_int32 bt_start_task_impl( struct _tag_task *p_task );
_int32 bt_stop_task( struct _tag_task *p_task );
_int32 bt_stop_task_impl( struct _tag_task *p_task );
_int32 bt_update_info( struct _tag_task *p_task );
_int32 bt_update_info_impl( struct _tag_task *p_task );
_int32 bt_delete_task( struct _tag_task *p_task );
_int32 bt_delete_task_impl( struct _tag_task *p_task );
BOOL bt_is_task_exist_by_info_hash(struct _tag_task *p_task,const _u8* info_hash);
BOOL bt_is_task_exist_by_gcid( struct _tag_task *p_task, const _u8* gcid);
BOOL bt_is_task_exist_by_cid( struct _tag_task *p_task, const _u8* cid);
CONNECT_MANAGER *  bt_get_task_connect_manager( struct _tag_task *p_task, const _u8* gcid,_u32 * file_index);
_int32 bt_get_file_info(  struct  tag_BT_FILE_INFO_POOL* p_bt_file_info_pool, _u32 file_index, void *p_file_info );
_int32 bt_get_download_file_index( struct _tag_task *p_task, _u32 *buffer_len, _u32 *file_index_buffer );
_int32 bt_get_task_file_path_and_name(TASK* p_task, _u32 file_index,char *file_path_buffer, _int32 *file_path_buffer_size, char *file_name_buffer, _int32* file_name_buffer_size);
void bt_get_file_info_pool(TASK* p_task, struct  tag_BT_FILE_INFO_POOL* p_bt_file_info_pool );

/* Interfaces for BT data manager  */
_int32 bt_notify_file_closing_result_event(TASK* p_task);
_int32 bt_notify_report_task_success(TASK* p_task);

_int32 bt_query_hub_for_single_file( TASK* p_task,_u32 file_index );

_int32 bt_get_tmp_file_path_name(TASK* p_task, char* tmp_file_path, char* tmp_file_name);

#ifdef ENABLE_BT_PROTOCOL
_int32 bt_notify_have_piece(TASK* p_task, _u32 piece_index );
_int32 bt_update_pipe_can_download_range( TASK* p_task);
#endif

_int32 bt_report_single_file( TASK* p_task,_u32 file_index, BOOL is_success );

_int32 bt_get_sub_file_tcid( struct _tag_task *p_task, _u32 file_index, _u8 cid[CID_SIZE]);
_int32 bt_get_sub_file_gcid( struct _tag_task *p_task, _u32 file_index, _u8 gcid[CID_SIZE]);
_int32 bt_get_sub_file_name( struct _tag_task *p_task, _u32 file_index, char * name_buffer,_u32 * buffer_size);

_int32 bt_get_seed_title_name(TASK* p_task, char *file_name_buffer, _int32* buffer_size);
#endif /* #ifdef ENABLE_BT */

#ifdef __cplusplus
}
#endif

#endif // !defined(__BT_TASK_INTERFACE_20080920)

