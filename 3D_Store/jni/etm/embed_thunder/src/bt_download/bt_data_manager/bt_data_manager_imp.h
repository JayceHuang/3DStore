

/*****************************************************************************
 *
 * Filename: 			bt_data_manager_imp.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2009/02/24
 *	
 * Purpose:				bt_data_manager implement.
 *
 *****************************************************************************/

#if !defined(__BT_DATA_MANAGER_IMP_20090224)
#define __BT_DATA_MANAGER_IMP_20090224

#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"
#ifdef ENABLE_BT 

#include "bt_download/bt_data_manager/bt_data_manager.h"
#include "bt_download/bt_data_manager/bt_piece_checker.h"


_int32 bdm_handle_add_range( BT_DATA_MANAGER *p_bt_data_manager, const RANGE *p_range, RESOURCE *p_resource );
_int32 bdm_handle_del_range( BT_DATA_MANAGER *p_bt_data_manager, const RANGE *p_range, RESOURCE *p_resource );

//_int32 bdm_try_to_check_piece( BT_DATA_MANAGER *p_bt_data_manager, const LIST *p_piece_info_list );
//_int32 bdm_handle_cache_piece_hash( BT_DATA_MANAGER *p_bt_data_manager, PIECE_RANGE_INFO *p_piece_range_info, RANGE_DATA_BUFFER_LIST *p_ret_range_buffer );
_int32 bdm_get_cache_data_buffer( BT_DATA_MANAGER *p_bt_data_manager, _u32 file_index, const RANGE *p_sub_file_range, RANGE_DATA_BUFFER_LIST *p_ret_range_buffer );

BOOL bdm_is_range_check_ok( BT_DATA_MANAGER *p_bt_data_manager, _u32 file_index, RANGE *p_sub_file_range );
BOOL bdm_is_range_can_read( BT_DATA_MANAGER *p_bt_data_manager, _u32 file_index, RANGE *p_sub_file_range );

//_int32 bdm_start_check_piece( BT_DATA_MANAGER *p_bt_data_manager );

//_int32 bdm_try_to_add_check_piece( BT_DATA_MANAGER *p_bt_data_manager );

//_int32 bdm_notify_piece_check_finished( BT_DATA_MANAGER *p_bt_data_manager, _u32 piece_index, BOOL is_success );

void bdm_notify_sub_file_range_check_finished( BT_DATA_MANAGER *p_bt_data_manager, _u32 file_index, RANGE *p_range, BOOL is_success );

void bdm_checker_notify_check_finished( BT_DATA_MANAGER *p_bt_data_manager, _u32 piece_index, RANGE *p_padding_range, BOOL is_success );

void bdm_notify_padding_range_check_finished( BT_DATA_MANAGER *p_bt_data_manager, RANGE *p_padding_range, BOOL is_success, BOOL is_piece_check );

void bdm_check_range_success( BT_DATA_MANAGER *p_bt_data_manager, RANGE *p_range );
_int32 bdm_check_range_failure( BT_DATA_MANAGER *p_bt_data_manager, RANGE *p_range, BOOL is_piece_check );

void bdm_notify_erase_range( BT_DATA_MANAGER *p_bt_data_manager, RANGE *p_range );

_int32 bdm_read_data( BT_DATA_MANAGER *p_bt_data_manager, _u32 file_index, RANGE_DATA_BUFFER *p_range_data_buffer, notify_read_result p_call_back, void *p_user );

_int32 bdm_check_if_download_success( BT_DATA_MANAGER *p_bt_data_manager, _u32 file_index );
//BOOL bdm_check_if_file_can_close( BT_DATA_MANAGER *p_bt_data_manager, _u32 file_index );

void bdm_notify_download_failed( BT_DATA_MANAGER *p_bt_data_manager, _int32 err_code );

void bdm_notify_task_download_success( BT_DATA_MANAGER *p_bt_data_manager );
void bdm_notify_del_tmp_file( BT_DATA_MANAGER *p_bt_data_manager );


//BOOL bdm_bt_piece_is_write( BT_DATA_MANAGER *p_bt_data_manager, _u32 piece_index );

void bdm_file_manager_notify_add_file_range( BT_DATA_MANAGER *p_bt_data_manager, _u32 file_index, RANGE *p_sub_range );

void bdm_file_manager_notify_start_file( BT_DATA_MANAGER *p_bt_data_manager, _u32 file_index, RANGE_LIST *p_sub_file_recv_range_list, BOOL has_bcid );
void bdm_file_manager_notify_delete_file_range( BT_DATA_MANAGER *p_bt_data_manager, _u32 file_index, RANGE *p_sub_range );

void bdm_file_manager_notify_stop_file( BT_DATA_MANAGER *p_bt_data_manager, _u32 file_index, BOOL is_ever_vod_file );

void bdm_notify_task_stop_success( BT_DATA_MANAGER *p_bt_data_manager );
void bdm_notify_report_task_success( BT_DATA_MANAGER *p_bt_data_manager );

BOOL bdm_range_is_write_in_disk( BT_DATA_MANAGER *p_bt_data_manager, _u32 file_index, RANGE *p_range );

//void bdm_handle_range_related_piece( BT_DATA_MANAGER *p_bt_data_manager, const RANGE *p_padding_range );
void bdm_erase_range_related_piece( BT_DATA_MANAGER *p_bt_data_manager, _u32 file_index );

//void bdm_check_if_range_related_piece_received( BT_DATA_MANAGER *p_bt_data_manager, const RANGE *p_padding_range );
//BOOL bdm_bt_piece_is_resv( BT_DATA_MANAGER *p_bt_data_manager, _u32 piece_index );

//BOOL bdm_is_file_need_piece_check( BT_DATA_MANAGER *p_bt_data_manager, _u32 file_index );
void bdm_query_hub_for_single_file( BT_DATA_MANAGER *p_bt_data_manager, _u32 file_index );

BOOL bdm_file_range_is_cached( BT_DATA_MANAGER *p_bt_data_manager, _u32 file_index, RANGE *p_sub_file_range );

BOOL bdm_padding_range_is_resv( BT_DATA_MANAGER *p_bt_data_manager, RANGE *p_padding_range );

_u32 bdm_get_vod_file_index( BT_DATA_MANAGER *p_bt_data_manager );

void bdm_start_read_tmp_file_range( BT_DATA_MANAGER *p_bt_data_manager );
_int32 bdm_read_tmp_file_call_back(struct tagTMP_FILE *p_file_struct, void *p_user, RANGE_DATA_BUFFER *p_data_buffer, _int32 read_result, _u32 read_len );
void bdm_notify_tmp_range_write_ok( BT_DATA_MANAGER *p_bt_data_manager, RANGE *p_padding_range );

void bdm_report_single_file( BT_DATA_MANAGER *p_bt_data_manager, _u32 file_index, BOOL is_success );
#endif /* #ifdef ENABLE_BT */

#ifdef __cplusplus
}
#endif

#endif // !defined(__BT_DATA_MANAGER_IMP_20090224)


