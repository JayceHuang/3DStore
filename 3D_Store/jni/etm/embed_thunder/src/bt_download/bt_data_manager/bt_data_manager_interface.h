

/*****************************************************************************
 *
 * Filename: 			bt_data_manager_interface.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2009/02/24
 *	
 * Purpose:				bt_data_manager_interface.
 *
 *****************************************************************************/

#if !defined(__BT_DATA_MANAGER_INTERFACE_20090224)
#define __BT_DATA_MANAGER_INTERFACE_20090224

#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"

#ifdef ENABLE_BT 
#include "data_manager/file_manager_interface.h"
#include "bt_download/bt_data_manager/bt_pipe_reader.h"
#include "connect_manager/data_pipe.h"
struct tagBT_DATA_MANAGER;
struct tagTORRENT_PARSER;
struct tagBT_RANGE_SWITCH;
struct tagBITMAP;
struct _tag_task;
struct _tag_error_block_list;
struct _tag_ds_data_intereface;
//struct tagREAD_RANGE_INFO;
#include "bt_download/bt_task/bt_task.h"

typedef _int32 (*bt_pipe_read_result) ( void *p_user, BT_RANGE *p_bt_range, char *p_data_buffer, _int32 read_result, _u32 read_len );


_int32 bdm_init_module( void );

_int32 bdm_uninit_module( void );

/*****************************************************************************
 * Interface for bt task
 *****************************************************************************/

_int32 bdm_init_struct( struct tagBT_DATA_MANAGER *p_bt_data_manager, 
	struct _tag_task *p_task, struct tagTORRENT_PARSER *p_torrent_parser,
	_u32 *p_need_download_file_array, _u32 need_download_file_num, 
	char *p_data_path, _u32 data_path_len );

_int32 bdm_uninit_struct( struct tagBT_DATA_MANAGER *p_bt_data_manager );

_int32 bdm_set_cid( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index, _u8 cid[CID_SIZE] );

BOOL bdm_get_cid( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index, _u8 cid[CID_SIZE] );
BOOL bdm_get_bcids( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index, _u32 *p_bcid_num, _u8 **pp_bcid_buffer );

_int32 bdm_set_hub_return_info( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index, _int32 gcid_type,  
	unsigned char block_cid[], _u32 block_count, _u32 block_size, _u64 filesize );
_int32 bdm_set_hub_return_info2( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index, _int32 gcid_type,  
								unsigned char block_cid[], _u32 block_count, _u32 block_size);

_int32 bdm_set_gcid( struct tagBT_DATA_MANAGER* p_bt_data_manager, _u32 file_index, _u8 gcid[CID_SIZE] );

_int32 bdm_get_status_code( struct tagBT_DATA_MANAGER *p_bt_data_manager );

enum BT_FILE_STATUS bdm_get_file_status ( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index );
_int32 bdm_get_file_err_code ( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index );

_u32  bdm_get_total_file_percent( struct tagBT_DATA_MANAGER *p_bt_data_manager );
_u32  bdm_get_sub_file_percent( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index );
_u32  bdm_get_file_download_time( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index );
_u32 bdm_get_sub_task_start_time(struct tagBT_DATA_MANAGER* p_bt_data_manager, _u32 file_index);

_u64  bdm_get_total_file_download_data_size( struct tagBT_DATA_MANAGER *p_bt_data_manager);
_u64  bdm_get_sub_file_download_data_size( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index );

_u64  bdm_get_total_file_writed_data_size( struct tagBT_DATA_MANAGER *p_bt_data_manager );
_u64  bdm_get_sub_file_writed_data_size( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index );

void bdm_get_sub_file_stat_data(struct tagBT_DATA_MANAGER* p_bt_data_manager
                                , _u32 file_index
                                , _u32* first_zero_duration
                                , _u32* other_zero_duration
                                , _u32* percent100_time);

BOOL bdm_is_file_info_opening(  struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index );

_u32 bdm_get_file_block_size( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index );

_int32 bdm_shub_no_result( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index );

BOOL bdm_bcid_is_valid( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index );

BOOL bdm_get_shub_gcid( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index, _u8 gcid[CID_SIZE] );
BOOL bdm_get_calc_gcid( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index, _u8 gcid[CID_SIZE] );

_int32 bdm_notify_start_speedup_sub_file( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index );

_int32 bdm_notify_stop_speedup_sub_file( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index );

void bdm_on_time( struct tagBT_DATA_MANAGER *p_bt_data_manager );

_int32 bdm_get_data_path( struct tagBT_DATA_MANAGER *p_bt_data_manager, char **p_data_path, _u32 *p_path_len );
	
/*****************************************************************************
 * Interface for connect_manager
 *****************************************************************************/

struct tagBT_RANGE_SWITCH *bdm_get_range_switch( struct tagBT_DATA_MANAGER *p_bt_data_manager );

BOOL bdm_can_abandon_resource( void *p_data_manager, RESOURCE *p_res );


/*****************************************************************************
 * Interface for dispatcher
 *****************************************************************************/
_int32 bdm_get_dispatcher_data_info( struct tagBT_DATA_MANAGER *p_bt_data_manager, struct _tag_ds_data_intereface *p_data_info );
_u64 bdm_dispatcher_get_file_size( void *p_data_manager );
BOOL bdm_is_only_using_origin_res( void *p_data_manager );	
BOOL bdm_is_origin_resource( void *p_data_manager, RESOURCE *p_res );
_int32 bdm_get_uncomplete_range( void *p_data_manager, RANGE_LIST *p_un_complete_range_list );
_int32 bdm_get_priority_range( void *p_data_manager, RANGE_LIST *p_priority_range_list );
_int32 bdm_get_error_range_block_list( void *p_data_manager, struct _tag_error_block_list **pp_error_block_list );
BOOL bdm_dispatcher_ds_is_vod_mod( void *p_data_manager );


/*****************************************************************************
 * Interface for bt_data_pipe
 *****************************************************************************/
_int32 bdm_get_data_buffer( struct tagBT_DATA_MANAGER * p_bt_data_manager, char **pp_data_buffer, _u32 *p_data_len );
_int32 bdm_free_data_buffer( struct tagBT_DATA_MANAGER * p_bt_data_manager, char **pp_data_buffer, _u32 data_len );
BOOL bdm_is_piece_ok( struct tagBT_DATA_MANAGER * p_bt_data_manager, _u32 piece_index );

_int32 bdm_speedup_pipe_put_data( struct tagBT_DATA_MANAGER* p_bt_data_manager, _u32 file_index, const RANGE *p_range, 
                                 char **pp_data_buffer, _u32 data_len,_u32 buffer_len, DATA_PIPE *p_data_pipe, RESOURCE *p_resource );

_int32 bdm_bt_pipe_put_data( struct tagBT_DATA_MANAGER* p_bt_data_manager, const RANGE *p_range, 
                            char **pp_data_buffer, _u32 data_len,_u32 buffer_len, DATA_PIPE *p_data_pipe, RESOURCE *p_resource );


_int32  bdm_asyn_read_data_buffer( struct tagBT_DATA_MANAGER* p_bt_data_manager, RANGE_DATA_BUFFER *p_data_buffer, 
	notify_read_result p_call_back, void *p_user, _u32 file_index );


_int32  bdm_bt_pipe_read_data_buffer( struct tagBT_DATA_MANAGER* p_bt_data_manager, const BT_RANGE *p_bt_range, 
	char *p_data_buffer, _u32 buffer_len, bt_pipe_read_result p_call_back, void *p_user );

/* Added by zyq @ 20090327 */
_int32  bdm_bt_pipe_clear_read_data_buffer( struct tagBT_DATA_MANAGER* p_bt_data_manager,  void *p_user );
_int32  bdm_read_cache_data_to_buffer( _u64 des_pos,_u32 des_data_len , LIST * cache_data_list,char *data_buffer,_u32 *data_len);
_int32  bdm_read_file_data_to_buffer( struct tagBT_DATA_MANAGER* p_bt_data_manager, const BT_RANGE *p_bt_range,char *p_data_buffer,_u32 total_data_len,_u32 data_len,
	 bt_pipe_read_result p_call_back, void *p_user, LIST ** read_range_info_list_for_file);
/* zyq adding end  @ 20090327 */


_u64 bdm_get_sub_file_size( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index );
_u64 bdm_get_file_size( struct tagBT_DATA_MANAGER *p_bt_data_manager );

_int32 bdm_set_sub_file_size( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index, _u64 file_size );

enum tagPIPE_FILE_TYPE bdm_get_sub_file_type( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index );

_int32 bdm_notify_dispatch_data_finish( struct tagBT_DATA_MANAGER* p_bt_data_manager, DATA_PIPE *ptr_data_pipe );
_u32 bdm_get_total_piece_num( struct tagBT_DATA_MANAGER* p_bt_data_manager );
_u32 bdm_get_piece_size( struct tagBT_DATA_MANAGER* p_bt_data_manager );
_int32 bdm_get_complete_bitmap( struct tagBT_DATA_MANAGER* p_bt_data_manager, struct tagBITMAP **pp_bitmap );
_int32 bdm_get_info_hash( struct tagBT_DATA_MANAGER* p_bt_data_manager, unsigned char **pp_info_hash );

/*-----------------------------------*/
BOOL bdm_range_is_in_need_range(struct tagBT_DATA_MANAGER* p_bt_data_manager, RANGE* p_range);


/*****************************************************************************
 * Interface for dap
 *****************************************************************************/

_int32 bdm_set_emerge_rangelist( struct tagBT_DATA_MANAGER* p_bt_data_manager, _u32 file_index, RANGE_LIST *em_range_list );

_int32 bdm_get_range_data_buffer( struct tagBT_DATA_MANAGER* p_bt_data_manager, _u32 file_index, RANGE need_r, RANGE_DATA_BUFFER_LIST *ret_range_buffer );
BOOL  bdm_range_is_recv( struct tagBT_DATA_MANAGER* p_bt_data_manager, _u32 file_index, RANGE *p_range );
BOOL  bdm_range_is_cache( struct tagBT_DATA_MANAGER* p_bt_data_manager, _u32 file_index, RANGE *p_range );

BOOL  bdm_range_is_write( struct tagBT_DATA_MANAGER* p_bt_data_manager, _u32 file_index, RANGE *p_range );
RANGE_LIST *bdm_get_resv_range_list( struct tagBT_DATA_MANAGER* p_bt_data_manager, _u32 file_index  );
void bdm_set_vod_mode( struct tagBT_DATA_MANAGER* p_bt_data_manager, BOOL is_vod_mode );
_int32 bdm_get_file_name( struct tagBT_DATA_MANAGER* p_bt_data_manager, _u32 file_index, char **pp_file_name );

RANGE_LIST *bdm_get_check_range_list( struct tagBT_DATA_MANAGER* p_bt_data_manager, _u32 file_index  );
BOOL bdm_is_vod_mode( struct tagBT_DATA_MANAGER *p_bt_data_manager );

void bdm_get_dl_bytes( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u64 *p_server_dl_bytes, _u64 *p_peer_dl_bytes, _u64 *p_cdn_dl_bytes , _u64* p_origion_dl_bytes);

void bdm_get_sub_file_dl_bytes( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index, _u64 *p_server_dl_bytes, _u64 *p_peer_dl_bytes, _u64 *p_cdn_dl_bytes, _u64 *p_lixian_dl_bytes ,_u64* p_origion_dl_bytes);

_u64 bdm_get_sub_file_normal_cdn_bytes(struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index);

BOOL bdm_is_waited_success_close_file( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index );

void bdm_set_del_tmp_flag( struct tagBT_DATA_MANAGER *p_bt_data_manager );

void bdm_filter_uncomplete_rangelist( void *p_data_manager, RANGE_LIST *p_un_complete_range_list );

_u64 bdm_get_normal_cdn_down_bytes(struct tagBT_DATA_MANAGER *p_bt_data_manager);

#ifdef EMBED_REPORT
struct tagFILE_INFO_REPORT_PARA *bdm_get_report_para( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index );
#endif 

_int32 bdm_stop_report_running_sub_file( struct tagBT_DATA_MANAGER *p_bt_data_manager);

#endif /* #ifdef ENABLE_BT */

#ifdef __cplusplus
}
#endif

#endif // !defined(__BT_DATA_MANAGER_INTERFACE_20090224)


