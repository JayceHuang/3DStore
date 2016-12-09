

/*****************************************************************************
 *
 * Filename: 			bt_sub_file_manager.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2009/03/21
 *	
 * Purpose:				Basic bt_data_manager struct.
 *
 *****************************************************************************/

#if !defined(__BT_SUB_FILE_MANAGER_20090321)
#define __BT_SUB_FILE_MANAGER_20090321

#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"
#ifdef ENABLE_BT 

#include "utility/map.h"
#include "utility/range.h"
#include "utility/speed_calculator.h"

#include "utility/define_const_num.h"

#include "bt_download/bt_task/bt_task_interface.h"
//#include "bt_download/bt_data_manager/bt_tmp_file.h"


struct tagBT_DATA_MANAGER;
struct _tag_file_info;
struct _tag_range_data_buffer_list;
struct tagBT_SUB_FILE;

typedef enum tagBT_SUB_FILE_TYPE
{
	IDLE_SUB_FILE_TYPE,
	COMMON_SUB_FILE_TYPE,
	SPEEDUP_SUB_FILE_TYPE
} BT_SUB_FILE_TYPE;

typedef enum tagSUB_FILE_INFO_STATUS
{
	FILE_INFO_OPENING,
	FILE_INFO_OPENED,
	FILE_INFO_CLOSING,
	FILE_INFO_CLOSED,
		
	//FILE_INFO_OPEN = 1,
	//FILE_OPEN = 2,
	//FILE_CLOSING = 3,
	//FILE_CHECK_RETRY = 5,
	//FILE_CLOSE = 6,
	//FILE_INFO_CLOSE = 8,
	//BT_SUB_FILE_DESTORY = 9
} SUB_FILE_INFO_STATUS;

typedef struct tagBT_FILE_MANAGER
{
	MAP _bt_sub_file_map;
	//_u32 _cur_speedup_file_num;
	//_u32 _cur_common_file_num;
	_u32 _cur_file_info_num;
	struct tagBT_DATA_MANAGER *_bt_data_manager_ptr;
	struct tagTORRENT_PARSER *_torrent_parser_ptr;
	char _data_path[MAX_FILE_PATH_LEN];
	_u32 _data_path_len;
	_u64 _file_total_download_bytes;
	_u64 _file_total_writed_bytes;
	_u64 _file_total_size;
	_u64 _need_download_size;
	_u64 _cur_downloading_file_size;
	BOOL _close_need_call_back;
	BOOL _is_closing;
	_u32 _cur_range_step;
	struct tagBT_SUB_FILE *_big_sub_file_ptr;
	BOOL _is_start_finished_task;
	BOOL _is_report_task;
	_u64 _server_dl_bytes;
	_u64 _peer_dl_bytes;
	_u64 _cdn_dl_bytes;
	_u64 _partner_cdn_dl_bytes;
	RANGE_LIST _3part_cid_list;
    _u64 _origin_dl_bytes;  // 资源类型为 BT_RESOURCE_TYPE
    _u64 _assist_peer_dl_bytes;
    _u64 _lixian_dl_bytes;
	_u64 _normal_cdn_bytes;
} BT_FILE_MANAGER;

typedef struct tagBT_SUB_FILE
{
	_u32 _file_index;
	enum BT_FILE_STATUS _file_status;
	struct _tag_file_info *_file_info_ptr;
	enum tagBT_SUB_FILE_TYPE _sub_file_type;
	BT_FILE_MANAGER *_bt_file_manager_ptr;
	_u64 _file_size;
	_u64 _file_download_bytes;
	_u64 _file_writed_bytes;
	_u32 _err_code;
	BOOL _has_bcid;
	BOOL _is_speedup_failed;
	BOOL _waited_success_close_file;
	SUB_FILE_INFO_STATUS _sub_file_info_status;
	SPEED_CALCULATOR _file_download_speed;
	_u32 _idle_ticks;
	_u32 _start_time;//(/s)
	_u32 _downloading_time;//(/s)
	_u64 _server_dl_bytes;
	_u64 _peer_dl_bytes;
	_u64 _cdn_dl_bytes;
	_u64 _origion_dl_bytes;
	_u64 _lixian_dl_bytes;
	_u64 _partner_cdn_dl_bytes;
	_u64 _normal_cdn_bytes;
	BOOL _is_ever_vod_file;
    _u32 _sub_task_start_time;
    _u32 _first_zero_speed_duration;
    _u32 _last_stat_time;
    _u32 _other_zero_speed_duraiton;
    _u32 _percent100_time;
    BOOL _is_continue_task;
} BT_SUB_FILE;


/*****************************************************************************
 * public function for task platform.
 *****************************************************************************/

void bfm_init_module(void);

_int32 bfm_init_struct( BT_FILE_MANAGER *p_bt_file_manager, 
	struct tagBT_DATA_MANAGER *p_bt_data_manager, struct tagTORRENT_PARSER *p_torrent_parser,
	_u32 *p_need_download_file_array, _u32 need_download_file_num,
	char *p_data_path, _u32 data_path_len );

_int32 bfm_uninit_struct( BT_FILE_MANAGER *p_bt_file_manager );

_int32 bfm_set_cid( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, _u8 cid[CID_SIZE] );

BOOL bfm_get_cid( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, _u8 cid[CID_SIZE] );
BOOL bfm_get_bcids( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, _u32 *p_bcid_num, _u8 **pp_bcid_buffer );

_int32 bfm_set_hub_return_info( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, _int32 gcid_type,  
	unsigned char block_cid[], _u32 block_count, _u32 block_size, _u64 filesize );

_int32 bfm_set_hub_return_info2( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, _int32 gcid_type,  
								unsigned char block_cid[], _u32 block_count, _u32 block_size );

_int32 bfm_set_gcid( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, _u8 gcid[CID_SIZE] );

enum BT_FILE_STATUS bfm_get_file_status ( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index );


_u32  bfm_get_total_file_percent( BT_FILE_MANAGER *p_bt_file_manager );
_u32  bfm_get_sub_file_percent( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index );
_u32 bfm_get_downloading_time( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index );
_u32 bfm_get_sub_task_start_time(BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index);


_u64  bfm_get_total_file_download_data_size( BT_FILE_MANAGER *p_bt_file_manager );
_u64  bfm_get_sub_file_download_data_size( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index );

_u64  bfm_get_total_file_writed_data_size( BT_FILE_MANAGER *p_bt_file_manager );
_u64  bfm_get_sub_file_writed_data_size( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index );

void bfm_get_sub_file_stat_data(BT_FILE_MANAGER *p_bt_file_manager
                                , _u32 file_index
                                , _u32* first_zero_duration
                                , _u32* other_zero_duration
                                , _u32* percent100_time);

BOOL bfm_is_file_info_opening( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index );

BOOL bfm_bcid_is_valid( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index );

//void bfm_erase_err_range( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, RANGE *p_range );


BOOL bfm_get_shub_gcid( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, _u8 gcid[CID_SIZE] );
BOOL bfm_get_calc_gcid( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, _u8 gcid[CID_SIZE] );

_int32 bfm_notify_create_speedup_file_info( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index );

_int32 bfm_notify_delete_speedup_file_info( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index );

_int32 bfm_notify_delete_range_data( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, RANGE *p_range );

_u32  bfm_get_sub_file_block_size( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index );



/*****************************************************************************
 * public function for pipe.
 *****************************************************************************/

_int32 bfm_put_data( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, const RANGE *p_range, 
	char **pp_data_buffer, _u32 data_len,_u32 buffer_len, RESOURCE *p_resource  );

_int32  bfm_read_data_buffer( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, 
	RANGE_DATA_BUFFER *p_data_buffer, notify_read_result p_call_back, void *p_user );


_int32 bfm_notify_flush_data( BT_FILE_MANAGER *p_bt_file_manager );

_u64 bfm_get_total_file_size( BT_FILE_MANAGER *p_bt_file_manager );

_u64 bfm_get_sub_file_size( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index );

_u64 bfm_get_file_size( BT_FILE_MANAGER *p_bt_file_manager );

enum tagPIPE_FILE_TYPE bfm_get_sub_file_type( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index );

_int32 bfm_get_sub_file_cache_data( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, const RANGE *p_sub_file_range, struct _tag_range_data_buffer_list *p_ret_range_buffer );

BOOL bfm_range_is_check( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, RANGE *p_sub_file_range );
BOOL bfm_range_is_can_read( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, RANGE *p_sub_file_range );

BOOL bfm_range_is_cached( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, RANGE *p_sub_file_range );

void bfm_get_dl_size( BT_FILE_MANAGER *p_bt_file_manager, _u64 *p_server_dl_bytes, _u64 *p_peer_dl_bytes, _u64 *p_cdn_dl_bytes, _u64* p_origion_dl_bytes );

_u64 bfm_get_normal_cdn_size(BT_FILE_MANAGER *p_bt_file_manager);

void bfm_get_sub_file_dl_size( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, _u64 *p_server_dl_bytes, _u64 *p_peer_dl_bytes, _u64 *p_cdn_dl_bytes, _u64 *p_lixian_dl_bytes, _u64* p_origion_dl_bytes );

_u64 bfm_get_sub_file_normal_cdn_size(BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index);

BOOL bfm_is_waited_success_close_file(BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index);

void bfm_get_assist_peer_dl_size( BT_FILE_MANAGER *p_bt_file_manager, _u64 *p_assist_peer_dl_bytes);



/*****************************************************************************
 * Interface for dap
 *****************************************************************************/

BOOL  bfm_range_is_recv( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, RANGE *p_range );
RANGE_LIST *bfm_get_resved_range_list( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index );

BOOL  bfm_range_is_write( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, RANGE *p_range );


RANGE_LIST *bfm_get_checked_range_list( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index );

/*****************************************************************************
 * set bt sub file err status.
 *****************************************************************************/

_int32 bfm_set_bt_sub_file_err_code( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, _int32 err_code );

/*****************************************************************************
 * register file_info call back.
 *****************************************************************************/

void  bfm_notify_file_create_result( void *p_user_data, struct _tag_file_info *p_file_info, _int32 creat_result );

void  bfm_notify_file_close_result( void *p_user_data, struct _tag_file_info *p_file_info, _int32 close_result );

void bfm_notify_file_block_check_result( void *p_user_data, struct _tag_file_info *p_file_info, RANGE *p_range, BOOL  check_result );

void bfm_notify_file_result( void* p_user_data, struct _tag_file_info *p_file_info, _u32 file_result );

_int32 bfm_handle_file_success( BT_FILE_MANAGER *p_bt_file_manager, BT_SUB_FILE *p_bt_sub_file );

_int32 bfm_handle_file_failture( BT_FILE_MANAGER *p_bt_file_manager, BT_SUB_FILE *p_bt_sub_file, _int32 err_code );


/*****************************************************************************
 * private function.
 *****************************************************************************/

_int32 bfm_create_bt_sub_file_struct( BT_FILE_MANAGER *p_bt_file_manager, struct tagTORRENT_PARSER *p_torrent_parser,
	_u32 *p_need_download_file_array, _u32 need_download_file_num );
_int32 bfm_init_file_info( BT_SUB_FILE *p_bt_sub_file, struct tagTORRENT_PARSER *p_torrent_parser, char *p_data_path, _u32 data_path_len );

_int32 bfm_bt_sub_file_map_compare( void *E1, void *E2 );
_int32 bfm_stop_all_bt_sub_file_struct( BT_FILE_MANAGER *p_bt_file_manager );

_int32 bfm_destroy_bt_sub_file_struct( BT_FILE_MANAGER *p_bt_file_manager );

_int32 bfm_start_create_common_file_info( BT_FILE_MANAGER *p_bt_file_manager );
BOOL bfm_select_file_download( BT_FILE_MANAGER *p_bt_file_manager );

_int32 bfm_start_single_file_info( BT_FILE_MANAGER *p_bt_file_manager, BT_SUB_FILE *p_bt_sub_file );
void bfm_add_file_downloading_range( BT_FILE_MANAGER *p_bt_file_manager, BT_SUB_FILE *p_bt_sub_file );

_int32 bfm_create_file_info( BT_FILE_MANAGER *p_bt_file_manager, BT_SUB_FILE *p_bt_sub_file );

_int32 bfm_open_continue_file_info( BT_SUB_FILE *p_bt_sub_file, struct _tag_file_info *p_file_info, char* cfg_file_path );

_int32 bfm_uninit_file_info( BT_FILE_MANAGER *p_bt_file_manager, BT_SUB_FILE *p_bt_sub_file );

_int32 bfm_dispatch_common_file( BT_FILE_MANAGER *p_bt_file_manager );

_int32 bfm_stop_sub_file( BT_FILE_MANAGER *p_bt_file_manager, BT_SUB_FILE *p_bt_sub_file );

BOOL bfm_is_slow_bt_sub_file( BT_SUB_FILE *p_bt_sub_file );

_int32 bfm_get_bt_sub_file_ptr( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, BT_SUB_FILE **pp_bt_sub_file );

_int32 bfm_get_file_info_ptr( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, struct _tag_file_info **pp_file_info );


_u32 bfm_get_max_file_info_num( BT_FILE_MANAGER *p_bt_file_manager );

void bfm_notify_range_check_result( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, RANGE *p_range, BOOL  check_result );
void bfm_nofity_file_close_success( BT_FILE_MANAGER *p_bt_file_manager );
void bfm_nofity_file_download_success( BT_FILE_MANAGER *p_bt_file_manager );
void bfm_nofity_file_download_failture( BT_FILE_MANAGER *p_bt_file_manager, _int32 err_code );

void bfm_enter_file_info_status( BT_SUB_FILE *p_bt_sub_file, enum tagSUB_FILE_INFO_STATUS status );
void bfm_enter_file_status( BT_SUB_FILE *p_bt_sub_file, enum BT_FILE_STATUS status );
void bfm_enter_file_err_code( BT_SUB_FILE *p_bt_sub_file, _int32 err_code );

void bfm_handle_file_err_code( BT_FILE_MANAGER *p_bt_file_manager );

_int32 bfm_notify_can_close( const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid );

_int32 bfm_vod_only_open_file_info( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index );

_int32 bfm_get_file_err_code( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index );

BOOL bfm_update_big_file_downloading_range( BT_FILE_MANAGER *p_bt_file_manager );

BOOL bfm_add_no_need_check_range( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, RANGE *p_sub_range );

#ifdef EMBED_REPORT
struct tagFILE_INFO_REPORT_PARA *bfm_get_report_para( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index );
void bfm_handle_err_data_report( BT_FILE_MANAGER *p_bt_file_manager, _u32 file_index, RESOURCE *p_res, _u32 data_len );
#endif

_int32 bfm_stop_report_running_sub_file( BT_FILE_MANAGER *p_bt_file_manager );

#endif /* #ifdef ENABLE_BT */



#ifdef __cplusplus
}
#endif

#endif // !defined(__BT_SUB_FILE_MANAGER_20090321)


