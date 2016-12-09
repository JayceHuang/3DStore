#include "utility/define.h"
#ifdef ENABLE_BT 


#include "bt_data_manager.h"
#include "bt_data_manager_interface.h"
#include "bt_data_manager_imp.h"
#include "torrent_parser/torrent_parser_interface.h"

#include "../bt_magnet/bt_magnet_task.h"

#include "bt_range_switch.h"
#include "connect_manager/pipe_interface.h"
#include "data_manager/file_info.h"
#include "data_manager/data_buffer.h"

#include "download_task/download_task.h"

#include "dispatcher/dispatcher_interface_info.h"
#include "bt_piece_checker.h"
#include "utility/utility.h"
#include "utility/string.h"


#include "utility/logid.h"
#define LOGID LOGID_BT_DOWNLOAD
#include "utility/logger.h"

const _u32  BT_VOD_ONCE_RANGE_NUM =  32;


_int32 bdm_init_module( void )
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "bdm_init_module. " );
	
	ret_val = brs_init_module();
	CHECK_VALUE( ret_val );
	
	ret_val = init_file_info_slabs();
	CHECK_VALUE( ret_val );

	ret_val = init_bt_data_reader_slabs();
	CHECK_VALUE( ret_val );

	ret_val = init_bpr();
	CHECK_VALUE( ret_val );
	
	bfm_init_module();

	return SUCCESS;
}

_int32 bdm_uninit_module( void )
{
	
	LOG_DEBUG( "bdm_uninit_module. " );
	
	uninit_bpr();
	
	uninit_bt_data_reader_slabs();
	
	uninit_file_info_slabs();	
	
	brs_uninit_module();
	
	return SUCCESS;
}

_int32 bdm_init_struct( struct tagBT_DATA_MANAGER *p_bt_data_manager, 
	struct _tag_task *p_task, struct tagTORRENT_PARSER *p_torrent_parser,
	_u32 *p_need_download_file_array, _u32 need_download_file_num, 
	char *p_data_path, _u32 data_path_len )
{
	_int32 ret_val = SUCCESS;
	_u8 *p_piece_hash = NULL;
	unsigned char *p_info_hash = NULL;
	_u32 piece_hash_len = 0;
	_u32 piece_num = 0;
	const char *tmp_sign = ".tmp";
	const char *p_path_sign = "/";
	char tmp_file_name[ MAX_FILE_NAME_LEN ];
	char tmp_file_path[ MAX_FILE_PATH_LEN ];
	char *p_title_name = NULL;

	LOG_DEBUG( "bdm_init_struct. " );
	p_bt_data_manager->_task_ptr = p_task;
	p_bt_data_manager->_IS_VOD_MOD = FALSE;
	p_bt_data_manager->_start_pos_index= 0;
	p_bt_data_manager->_conrinue_times = 0;
	p_bt_data_manager->_dap_file_index = MAX_U32;
	p_bt_data_manager->_is_reading_tmp_range = FALSE;
	
	piece_num = tp_get_piece_num( p_torrent_parser );
	bitmap_init( &p_bt_data_manager->_piece_bitmap );
	
	ret_val = bitmap_resize( &p_bt_data_manager->_piece_bitmap, piece_num );
	CHECK_VALUE( ret_val );
	
	range_list_init( &p_bt_data_manager->_need_read_tmp_range_list );
	
	p_bt_data_manager->_data_status_code = DATA_DOWNING;

	ret_val = tp_get_file_info_hash( p_torrent_parser, &p_info_hash );
	CHECK_VALUE( ret_val );
	
    str2hex((char*)p_info_hash, CID_SIZE, tmp_file_name, CID_SIZE*2);
	if( CID_SIZE*2 >= MAX_FILE_NAME_LEN ) return BT_DATA_MANAGER_LOGIC_ERROR;
	ret_val = sd_strncpy( tmp_file_name + CID_SIZE*2, tmp_sign, MAX_FILE_NAME_LEN - CID_SIZE*2 );
	tmp_file_name[ CID_SIZE*2 + sd_strlen( tmp_sign )] = '\0';

	LOG_DEBUG( "bdm_init_struct. p_data_path:%s",p_data_path );

	ret_val = tp_get_piece_hash( p_torrent_parser, &p_piece_hash, &piece_hash_len );
	if( ret_val != SUCCESS )
	{
		goto ErrHandle;
	}

	ret_val = brs_init_struct( &p_bt_data_manager->_bt_range_switch, p_torrent_parser,
		p_need_download_file_array, need_download_file_num );
    CHECK_VALUE( ret_val );

    ret_val = init_correct_manager( &p_bt_data_manager->_correct_manager );
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS )
	{
		goto ErrHandle0;
	}
	
	ret_val = init_range_manager( &p_bt_data_manager->_range_manager );
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS )
	{
		goto ErrHandle1;
	}

	ret_val = brdi_init_stuct( &p_bt_data_manager->_range_download_info );
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS )
	{
		goto ErrHandle2;
	}

	sd_memset( tmp_file_path, 0, MAX_FILE_PATH_LEN );
	if( data_path_len >= MAX_FILE_PATH_LEN )
	{
		ret_val = FILE_PATH_TOO_LONG;
		goto ErrHandle3;
	}
	ret_val = sd_strncpy( tmp_file_path, p_data_path, MAX_FILE_PATH_LEN );
	if( ret_val != SUCCESS )
	{
		goto ErrHandle3;
	}

	if( tp_has_dir(p_torrent_parser) )
	{
		ret_val = tp_get_seed_title_name( p_torrent_parser, &p_title_name );
		if( ret_val != SUCCESS )
		{
			goto ErrHandle3;
		}		
		ret_val = sd_strncpy( tmp_file_path + data_path_len, p_title_name, MAX_FILE_PATH_LEN );
		if( ret_val != SUCCESS )
		{
			goto ErrHandle3;
		}	
		ret_val = sd_strncpy( tmp_file_path + sd_strlen(tmp_file_path), p_path_sign, MAX_FILE_PATH_LEN );
		if( ret_val != SUCCESS )
		{
			goto ErrHandle3;
		}			
	}
	
	LOG_DEBUG( "bdm_init_struct. tmp_data_path:%s, tmp_name:%s",
		tmp_file_path, tmp_file_name );	
	
	ret_val = bt_init_piece_checker( &p_bt_data_manager->_bt_piece_check, 
		p_bt_data_manager, p_piece_hash, 
		tmp_file_path, tmp_file_name );
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS )
	{
		goto ErrHandle3;
	}

	ret_val = bfm_init_struct( &p_bt_data_manager->_bt_file_manager, 
		p_bt_data_manager, p_torrent_parser, 
		p_need_download_file_array, need_download_file_num,
		p_data_path, data_path_len );

	if( ret_val != SUCCESS )
	{
		goto ErrHandle4;
	}

	ret_val = bprm_init_list( &p_bt_data_manager->_bt_pipe_reader_mgr_list);
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS )
	{
		goto ErrHandle5;
	}	

	return SUCCESS;

ErrHandle5:
	bfm_uninit_struct( &p_bt_data_manager->_bt_file_manager );

ErrHandle4:
  	bt_uninit_piece_checker( &p_bt_data_manager->_bt_piece_check );

ErrHandle3:
	brdi_uninit_stuct( &p_bt_data_manager->_range_download_info );

ErrHandle2:
    unit_range_manager( &p_bt_data_manager->_range_manager );

ErrHandle1:
    unit_correct_manager( &p_bt_data_manager->_correct_manager );

ErrHandle0:
	brs_uninit_struct( &p_bt_data_manager->_bt_range_switch );
	
	range_list_clear( &p_bt_data_manager->_need_read_tmp_range_list );
	
ErrHandle:
	bitmap_uninit( &p_bt_data_manager->_piece_bitmap );
	return ret_val;
}

_int32 bdm_uninit_struct( struct tagBT_DATA_MANAGER *p_bt_data_manager )
{
	_int32 ret_val = SUCCESS;
	
	LOG_DEBUG( " bdm_uninit_struct");
	
	p_bt_data_manager->_start_pos_index= 0;
	p_bt_data_manager->_conrinue_times = 0;
	p_bt_data_manager->_data_status_code = DATA_UNINITED;
	bdm_set_vod_mode( p_bt_data_manager, FALSE );
	
	ret_val = bfm_uninit_struct( &p_bt_data_manager->_bt_file_manager );
	sd_assert( ret_val == SUCCESS );
	
	ret_val = bt_uninit_piece_checker( &p_bt_data_manager->_bt_piece_check );
	sd_assert( ret_val == SUCCESS );
	
	ret_val = brdi_uninit_stuct( &p_bt_data_manager->_range_download_info );
	sd_assert( ret_val == SUCCESS );

	ret_val = unit_range_manager( &p_bt_data_manager->_range_manager );
	sd_assert( ret_val == SUCCESS );

	ret_val = unit_correct_manager( &p_bt_data_manager->_correct_manager );
	sd_assert( ret_val == SUCCESS );
	
	//ret_val = bprm_uninit_list( &p_bt_data_manager->_bt_pipe_reader_mgr_list);
	//sd_assert( ret_val == SUCCESS );
	
	ret_val = brs_uninit_struct( &p_bt_data_manager->_bt_range_switch );
	sd_assert( ret_val == SUCCESS );

	range_list_clear( &p_bt_data_manager->_need_read_tmp_range_list );
	
	bitmap_uninit( &p_bt_data_manager->_piece_bitmap );

	return SUCCESS;	
}

_int32 bdm_set_cid( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index, _u8 cid[CID_SIZE])
{
	LOG_DEBUG( " bdm_set_cid");
	return bfm_set_cid( &p_bt_data_manager->_bt_file_manager, file_index, cid);	
}

BOOL bdm_get_cid( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index, _u8 cid[CID_SIZE])
{
	LOG_DEBUG( " bdm_get_cid");
	return bfm_get_cid( &p_bt_data_manager->_bt_file_manager, file_index, cid );	
}

BOOL bdm_get_bcids( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index, _u32 *p_bcid_num, _u8 **pp_bcid_buffer )
{
	LOG_DEBUG( " bdm_get_bcids");
	return bfm_get_bcids( &p_bt_data_manager->_bt_file_manager, file_index, p_bcid_num, pp_bcid_buffer );	
}

_int32 bdm_set_hub_return_info2( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index, _int32 gcid_type,  
							   unsigned char block_cid[], _u32 block_count, _u32 block_size)
{
	return bfm_set_hub_return_info2( &p_bt_data_manager->_bt_file_manager, file_index, gcid_type,
		block_cid, block_count, block_size );	
}

_int32 bdm_set_hub_return_info( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index, _int32 gcid_type,  
	unsigned char block_cid[], _u32 block_count, _u32 block_size, _u64 filesize )
{
	_int32 ret_val = SUCCESS;
	RANGE padding_range;
	LOG_DEBUG( " bdm_set_hub_return_info");
	ret_val = bfm_set_hub_return_info( &p_bt_data_manager->_bt_file_manager, file_index, gcid_type,
		block_cid, block_count, block_size, filesize );	
	sd_assert( ret_val == SUCCESS );

	ret_val = brs_get_padding_range_from_file_index( &p_bt_data_manager->_bt_range_switch, file_index, &padding_range );
	CHECK_VALUE( ret_val );
	bt_checker_erase_need_check_range( &p_bt_data_manager->_bt_piece_check, &padding_range );

	//bpc_stop_download_file( &p_bt_data_manager->_bt_piece_check, file_index );
	return SUCCESS;
}

_int32 bdm_set_gcid( struct tagBT_DATA_MANAGER* p_bt_data_manager, _u32 file_index, _u8 gcid[CID_SIZE])
{
	LOG_DEBUG( " bdm_set_gcid");
	return bfm_set_gcid( &p_bt_data_manager->_bt_file_manager, file_index, gcid );	
}

_int32 bdm_get_status_code( struct tagBT_DATA_MANAGER *p_bt_data_manager )
{
	LOG_DEBUG( " bdm_get_status_code:%d", p_bt_data_manager->_data_status_code);
	return p_bt_data_manager->_data_status_code;	
}

enum BT_FILE_STATUS bdm_get_file_status ( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index )
{
	LOG_DEBUG( " bdm_get_file_status,file_index:%u", file_index );
	return bfm_get_file_status( &p_bt_data_manager->_bt_file_manager, file_index );	
}

_int32 bdm_get_file_err_code ( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index )
{
	LOG_DEBUG( " bdm_get_file_err_code,file_index:%u", file_index );
	return bfm_get_file_err_code( &p_bt_data_manager->_bt_file_manager, file_index );	
	
}

_u32  bdm_get_total_file_percent( struct tagBT_DATA_MANAGER *p_bt_data_manager )
{
	return bfm_get_total_file_percent( &p_bt_data_manager->_bt_file_manager );	
}

_u32  bdm_get_sub_file_percent( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index )
{
	return bfm_get_sub_file_percent( &p_bt_data_manager->_bt_file_manager, file_index );	
}

_u32  bdm_get_file_download_time( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index )
{
	return bfm_get_downloading_time( &p_bt_data_manager->_bt_file_manager, file_index );	
}

_u32 bdm_get_sub_task_start_time(struct tagBT_DATA_MANAGER* p_bt_data_manager, _u32 file_index)
{
	return bfm_get_sub_task_start_time(&p_bt_data_manager->_bt_file_manager, file_index);
}

_u64  bdm_get_total_file_download_data_size( struct tagBT_DATA_MANAGER *p_bt_data_manager)
{
	return bfm_get_total_file_download_data_size( &p_bt_data_manager->_bt_file_manager );	
}

_u64  bdm_get_sub_file_download_data_size( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index )
{
	return bfm_get_sub_file_download_data_size( &p_bt_data_manager->_bt_file_manager, file_index );	
}

_u64  bdm_get_total_file_writed_data_size( struct tagBT_DATA_MANAGER *p_bt_data_manager )
{
	return bfm_get_total_file_writed_data_size( &p_bt_data_manager->_bt_file_manager );	
}

_u64  bdm_get_sub_file_writed_data_size( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index )
{
	return bfm_get_sub_file_writed_data_size( &p_bt_data_manager->_bt_file_manager, file_index ); 
}

void bdm_get_sub_file_stat_data(struct tagBT_DATA_MANAGER* p_bt_data_manager
                                , _u32 file_index
                                , _u32* first_zero_duration
                                , _u32* other_zero_duration
                                , _u32* percent100_time)
{
    bfm_get_sub_file_stat_data(&p_bt_data_manager->_bt_file_manager
        , file_index
        , first_zero_duration
        , other_zero_duration
        , percent100_time);
}

BOOL bdm_is_file_info_opening(  struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index )
{
	return bfm_is_file_info_opening( &p_bt_data_manager->_bt_file_manager, file_index ); 
}

_u32 bdm_get_file_block_size( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index )
{
	return bfm_get_sub_file_block_size( &p_bt_data_manager->_bt_file_manager, file_index ); 
}

_int32 bdm_shub_no_result( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index )
{
	LOG_URGENT( " bdm_shub_no_result, file_index = %d", file_index);
	//未完成
	return SUCCESS;
}

BOOL bdm_bcid_is_valid( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index )
{
	LOG_DEBUG( "bdm_bcid_is_valid, file_index = %d", file_index);
	return bfm_bcid_is_valid( &p_bt_data_manager->_bt_file_manager, file_index ); 
}


BOOL bdm_get_shub_gcid( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index, _u8 gcid[CID_SIZE] )
{
	LOG_DEBUG( "bdm_get_shub_gcid, file_index = %d", file_index);
	return bfm_get_shub_gcid( &p_bt_data_manager->_bt_file_manager, file_index, gcid ); 
}

BOOL bdm_get_calc_gcid( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index, _u8 gcid[CID_SIZE] )
{
	LOG_DEBUG( "bdm_get_calc_gcid, file_index = %d", file_index);
	return bfm_get_calc_gcid( &p_bt_data_manager->_bt_file_manager, file_index, gcid ); 
}

_int32 bdm_notify_start_speedup_sub_file( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index )
{
	_int32 ret_val = SUCCESS;
	
	LOG_DEBUG( "bdm_notify_start_speedup_sub_file, file_index = %d", file_index);

	ret_val = bfm_notify_create_speedup_file_info( &p_bt_data_manager->_bt_file_manager, file_index );

	//bpc_stop_download_file( &p_bt_data_manager->_bt_piece_check, file_index );
	
	return ret_val;		
}

_int32 bdm_notify_stop_speedup_sub_file( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index )
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( " bdm_notify_start_speedup_sub_file");
	
	ret_val = bfm_notify_delete_speedup_file_info( &p_bt_data_manager->_bt_file_manager, file_index );
	sd_assert( ret_val == SUCCESS );
	
	return SUCCESS;		
}

void bdm_on_time( struct tagBT_DATA_MANAGER *p_bt_data_manager )
{
	
	LOG_DEBUG( " bdm_on_time");
	//bdm_start_check_piece( p_bt_data_manager );
	bfm_handle_file_err_code( &p_bt_data_manager->_bt_file_manager );
	bdm_start_read_tmp_file_range( p_bt_data_manager );
	bfm_dispatch_common_file( &p_bt_data_manager->_bt_file_manager );
	LOG_DEBUG( "end of  bdm_on_time");
}

_int32 bdm_get_data_path( struct tagBT_DATA_MANAGER *p_bt_data_manager, char **p_data_path, _u32 *p_path_len )
{
	*p_data_path = (p_bt_data_manager->_bt_file_manager._data_path);
	*p_path_len = p_bt_data_manager->_bt_file_manager._data_path_len;
	return SUCCESS;
}

struct tagBT_RANGE_SWITCH *bdm_get_range_switch( struct tagBT_DATA_MANAGER *p_bt_data_manager )
{
	
	LOG_DEBUG( " bdm_get_range_switch");
	return &p_bt_data_manager->_bt_range_switch;
}

BOOL bdm_can_abandon_resource( void *p_data_manager, RESOURCE *p_res )
{
    struct tagBT_DATA_MANAGER *p_bt_data_manager = ( struct tagBT_DATA_MANAGER * )p_data_manager;
	sd_assert( p_bt_data_manager != NULL );

    LOG_DEBUG("bdm_can_abandon_resource,struct tagBT_DATA_MANAGER:0x%x  ,RESOURCE is:0x%x .", p_data_manager, p_res );	  

	if( p_bt_data_manager == NULL )
	{
		return TRUE;
	}
		
	if( p_res == NULL )
	{
		return TRUE;	
	}

	correct_manager_erase_abandon_res( &p_bt_data_manager->_correct_manager,  p_res );
/*
	if( range_manager_has_resource( &p_bt_data_manager->_range_manager, p_res ) )
	{
	    LOG_DEBUG("bdm_can_abandon_resource,DATA_MANAGER:0x%x  , abandon RESOURCE is:0x%x  failure.", p_bt_data_manager, p_res );	  
	    return FALSE;
	}
	else
	{
	    LOG_DEBUG("bdm_can_abandon_resource,DATA_MANAGER:0x%x  , abandon RESOURCE is:0x%x success.", p_bt_data_manager, p_res );	  
	    return TRUE;
	}
*/
	range_manager_erase_resource( &p_bt_data_manager->_range_manager, p_res );
	return TRUE;	
}

_int32 bdm_get_dispatcher_data_info( struct tagBT_DATA_MANAGER *p_bt_data_manager, struct _tag_ds_data_intereface *p_data_info )
{
	p_data_info->_get_file_size = bdm_dispatcher_get_file_size;               
	p_data_info->_is_only_using_origin_res = bdm_is_only_using_origin_res;  
	p_data_info->_is_origin_resource =  bdm_is_origin_resource;        
	p_data_info->_get_priority_range = bdm_get_priority_range;        
	p_data_info->_get_uncomplete_range = bdm_get_uncomplete_range ;     
	p_data_info->_get_error_range_block_list = bdm_get_error_range_block_list ;
	p_data_info->_get_ds_is_vod_mode = bdm_dispatcher_ds_is_vod_mod;
	LOG_DEBUG( " bdm_get_dispatcher_data_info");

	p_data_info->_own_ptr = (void*)p_bt_data_manager;

	return SUCCESS;
}

_u64 bdm_dispatcher_get_file_size( void *p_data_manager )
{
	struct tagBT_DATA_MANAGER *p_bt_data_manager = ( struct tagBT_DATA_MANAGER * )p_data_manager;
	sd_assert( p_bt_data_manager != NULL );
	if( p_bt_data_manager == NULL ) return SUCCESS;
	LOG_DEBUG( " bdm_dispatcher_get_file_size");

	return bdm_get_file_size( p_bt_data_manager );	
}

BOOL bdm_is_only_using_origin_res( void *p_data_manager )
{
	struct tagBT_DATA_MANAGER *p_bt_data_manager = ( struct tagBT_DATA_MANAGER * )p_data_manager;
	sd_assert( p_bt_data_manager != NULL );
	if( p_bt_data_manager == NULL ) return FALSE;
	LOG_DEBUG( " bdm_is_only_using_origin_res");

	return FALSE;	
}	

BOOL bdm_is_origin_resource( void *p_data_manager, RESOURCE *p_res )
{	
	struct tagBT_DATA_MANAGER *p_bt_data_manager = ( struct tagBT_DATA_MANAGER * )p_data_manager;
	sd_assert( p_bt_data_manager != NULL );
	if( p_bt_data_manager == NULL ) return FALSE;
	
	LOG_DEBUG( " bdm_is_origin_resource");
	return FALSE;	
}

_int32 bdm_get_uncomplete_range( void *p_data_manager, RANGE_LIST *p_un_complete_range_list )
{
	_int32 ret_val = SUCCESS;
	_u64 file_size = 0;	
	RANGE full_r ; 	
	RANGE start_r;  
	RANGE_LIST *recv_list = NULL;
	RANGE_LIST *p_need_download_list = NULL;
	RANGE_LIST piece_check_need_download_list;
	//_u32 p2sp_view_pos = 0;
	struct tagBT_DATA_MANAGER *p_bt_data_manager = ( struct tagBT_DATA_MANAGER * )p_data_manager;
	sd_assert( p_bt_data_manager != NULL );
	if( p_bt_data_manager == NULL ) return SUCCESS;

	LOG_DEBUG("bdm_get_uncomplete_range, is_vod_mode:%d. !!!!!!!!!!!!!!!!!!!!!!!!!!!!!",
		p_bt_data_manager->_IS_VOD_MOD );
	
	file_size = bfm_get_total_file_size( &p_bt_data_manager->_bt_file_manager );	 
	if( file_size == 0 )  return SUCCESS;

	recv_list = brdi_get_recved_range_list( &p_bt_data_manager->_range_download_info );
	if( recv_list == NULL )
	{
		sd_assert(FALSE);
	}

	//ret_val = brs_get_padding_range_from_file_index( &p_bt_data_manager->_bt_range_switch, p_bt_data_manager->_dap_file_index, &full_r );;

	LOG_DEBUG("bdm_get_uncomplete_range , before clear, un_complete_range_list:.");	 
	out_put_range_list( p_un_complete_range_list ); 
	range_list_clear( p_un_complete_range_list ); 
	

	LOG_DEBUG("bdm_get_uncomplete_range, is_vod_mode:%d. _start_pos_index:%u, _dap_file_index:%u .",
		p_bt_data_manager->_IS_VOD_MOD, p_bt_data_manager->_start_pos_index,
		p_bt_data_manager->_dap_file_index );

	if(p_bt_data_manager->_IS_VOD_MOD == TRUE 
		&& p_bt_data_manager->_start_pos_index != MAX_U32
		&& p_bt_data_manager->_dap_file_index != MAX_U32 )
	{    
		ret_val = brs_get_padding_range_from_file_index( &p_bt_data_manager->_bt_range_switch, p_bt_data_manager->_dap_file_index, &full_r );;
		sd_assert( ret_val == SUCCESS );
		
		do
		{
			start_r._index = p_bt_data_manager->_start_pos_index;
			start_r._num = ( p_bt_data_manager->_conrinue_times + 1 ) * BT_VOD_ONCE_RANGE_NUM;	
			sd_assert( full_r._index <= start_r._index );
			if( start_r._index + start_r._num > full_r._index + full_r._num )
			{
				start_r._num =  full_r._index + full_r._num - start_r._index;        
			}
			LOG_DEBUG("bdm_get_uncomplete_range , before calc, continue times :%u._start_pos_index:%u", 
				p_bt_data_manager->_conrinue_times, p_bt_data_manager->_start_pos_index );	  		   

			range_list_add_range( p_un_complete_range_list, &start_r, NULL,NULL );

			LOG_DEBUG("bdm_get_uncomplete_range , before calc, un_complete_range_list:.");	 
			out_put_range_list( p_un_complete_range_list); 
			LOG_DEBUG("bdm_get_uncomplete_range , before calc, _total_receive_range:.");	 
			out_put_range_list(recv_list); 

			range_list_delete_range_list( p_un_complete_range_list, recv_list );

			LOG_DEBUG("bdm_get_uncomplete_range , after calc, un_complete_range_list:.");	 
			out_put_range_list( p_un_complete_range_list ); 
			LOG_DEBUG("bdm_get_uncomplete_range , after calc, _total_receive_range:.");	 
			out_put_range_list( recv_list );    	

			if( p_un_complete_range_list->_node_size != 0 )
			{
				break;
			}
			else 
			{
				if(start_r._index + start_r._num >= full_r._index + full_r._num )
				{
					p_bt_data_manager->_start_pos_index = MAX_U32;  

					range_list_add_range( p_un_complete_range_list, &full_r, NULL,NULL );

					LOG_DEBUG("bdm_get_uncomplete_range , before calc, un_complete_range_list:.");	 
					out_put_range_list( p_un_complete_range_list ); 
					LOG_DEBUG("bdm_get_uncomplete_range , before calc, _total_receive_range:.");	 
					out_put_range_list(recv_list); 

					ret_val =  range_list_delete_range_list( p_un_complete_range_list, recv_list );

					LOG_DEBUG("bdm_get_uncomplete_range , after calc, un_complete_range_list:.");	 
					out_put_range_list( p_un_complete_range_list ); 
					LOG_DEBUG("bdm_get_uncomplete_range , after calc, _total_receive_range:.");	 
					out_put_range_list(recv_list);    	
					return  ret_val;				 
				}
				else
				{
					//ret_val = brs_get_file_p2sp_pos( &p_bt_data_manager->_bt_range_switch, p_bt_data_manager->_dap_file_index, &p2sp_view_pos );
					//sd_assert( ret_val == SUCCESS );
					//if( ret_val != SUCCESS ) return SUCCESS;
					p_bt_data_manager->_start_pos_index = start_r._index + start_r._num;
					p_bt_data_manager->_conrinue_times++;	  
					ret_val = 1;	   	  
					continue;
				}
			}
		}while(TRUE);

	}
	else
	{
		p_need_download_list = brdi_get_need_download_range_list( &p_bt_data_manager->_range_download_info );
		range_list_add_range_list( p_un_complete_range_list, p_need_download_list );	


		LOG_DEBUG("bdm_get_uncomplete_range , before calc, un_complete_range_list:.");	 
		out_put_range_list( p_un_complete_range_list ); 
		
		LOG_DEBUG("bdm_get_uncomplete_range , before calc, _total_receive_range:.");	 
		out_put_range_list(recv_list); 

		ret_val =  range_list_delete_range_list( p_un_complete_range_list, recv_list );

		LOG_DEBUG("bdm_get_uncomplete_range , after calc, un_complete_range_list:.");	 
		out_put_range_list( p_un_complete_range_list ); 
		
		if( range_list_size( p_un_complete_range_list ) == 0
			&& bfm_update_big_file_downloading_range( &p_bt_data_manager->_bt_file_manager ) )
		{
			p_need_download_list = brdi_get_need_download_range_list( &p_bt_data_manager->_range_download_info );
			range_list_add_range_list( p_un_complete_range_list, p_need_download_list );	

			LOG_DEBUG("bdm_get_uncomplete_range , before calc, un_complete_range_list:.");	 
			out_put_range_list( p_un_complete_range_list ); 
			
			LOG_DEBUG("bdm_get_uncomplete_range , before calc, _total_receive_range:.");	 
			out_put_range_list(recv_list); 

			ret_val =  range_list_delete_range_list( p_un_complete_range_list, recv_list);

			LOG_DEBUG("bdm_get_uncomplete_range , after calc, un_complete_range_list:.");	 
			out_put_range_list( p_un_complete_range_list ); 
		}

		range_list_init( &piece_check_need_download_list );

		bt_checker_get_need_download_range( &p_bt_data_manager->_bt_piece_check, &piece_check_need_download_list );
		LOG_DEBUG("bdm_get_uncomplete_range , piece_check_need_download_range, piece_check_need_download_list:.");	 
		out_put_range_list( &piece_check_need_download_list ); 

		LOG_DEBUG("bdm_get_uncomplete_range , after calc piece_check_need_download_range, un_complete_range_list:.");	 
		range_list_add_range_list( p_un_complete_range_list, &piece_check_need_download_list );	

#ifndef DISPATCH_RANDOM_MODE
		bdm_filter_uncomplete_rangelist(p_bt_data_manager,  p_un_complete_range_list);
#endif
		out_put_range_list( p_un_complete_range_list ); 
		
		range_list_clear( &piece_check_need_download_list );
		
	}	 

	return ret_val;	

}

_int32 bdm_get_priority_range( void *p_data_manager, RANGE_LIST *p_priority_range_list )
{
	_int32 ret_val = SUCCESS;
	RANGE_LIST *recv_list = NULL;	
	struct tagBT_DATA_MANAGER *p_bt_data_manager = ( struct tagBT_DATA_MANAGER * )p_data_manager;
	sd_assert( p_bt_data_manager != NULL );
	if( p_bt_data_manager == NULL ) return SUCCESS;

	if( p_bt_data_manager->_correct_manager._prority_ranages._node_size == 0 )
	{
	LOG_DEBUG( "bdm_get_priority_range, but  priority_range is 0." );
	return ret_val;
	}

	//out_put_range_list( &p_bt_data_manager->_correct_manager._prority_ranages );

	range_list_cp_range_list( &p_bt_data_manager->_correct_manager._prority_ranages, p_priority_range_list );

	//out_put_range_list( p_priority_range_list );

	recv_list = brdi_get_recved_range_list( &p_bt_data_manager->_range_download_info );
	if( recv_list == NULL )
	{
		sd_assert(FALSE);
	}

	//out_put_range_list( recv_list );   

	ret_val = range_list_delete_range_list( p_priority_range_list, recv_list );

	if( p_priority_range_list->_node_size == 0 )
	{
		correct_manager_clear_prority_range_list( &p_bt_data_manager->_correct_manager );
		ret_val = 1; 
	}
	
    LOG_DEBUG( "bdm_get_priority_range, uncomplete priority_range is:" );
	out_put_range_list( p_priority_range_list );

	return ret_val;

}

_int32 bdm_get_error_range_block_list( void *p_data_manager, struct _tag_error_block_list **pp_error_block_list )
{
#ifdef _LOGGER
	LIST_ITERATOR cur_node = NULL;
	struct _tag_error_block *p_err_block = NULL;
#endif

	struct tagBT_DATA_MANAGER *p_bt_data_manager = ( struct tagBT_DATA_MANAGER * )p_data_manager;
	sd_assert( p_bt_data_manager != NULL );
	if( p_bt_data_manager == NULL ) return SUCCESS;

	*pp_error_block_list = NULL;

	LOG_DEBUG( "bdm_get_error_range_block_list ." ); 
	
#ifdef _LOGGER
	cur_node = LIST_BEGIN( p_bt_data_manager->_correct_manager._error_ranages._error_block_list );
	while( cur_node != LIST_END( p_bt_data_manager->_correct_manager._error_ranages._error_block_list ) )
	{
		p_err_block = ( struct _tag_error_block *)LIST_VALUE( cur_node );
		
		LOG_DEBUG( "bdm_get_error_range_block_list.range:[%u, %u], err_count:%u",
			p_err_block->_r._index, p_err_block->_r._num, p_err_block->_error_count ); 
		cur_node = LIST_NEXT( cur_node );
	}

#endif


	if( list_size(&p_bt_data_manager->_correct_manager._error_ranages._error_block_list) == 0 )
	{
		LOG_DEBUG("bdm_get_error_range_block_list,  _correct_manage._error_ranages._error_block_list size is 0.");    
		return SUCCESS;
	}

	*pp_error_block_list = &p_bt_data_manager->_correct_manager._error_ranages;
	return SUCCESS;
}

BOOL bdm_dispatcher_ds_is_vod_mod( void *p_data_manager )
{
	struct tagBT_DATA_MANAGER *p_bt_data_manager = ( struct tagBT_DATA_MANAGER * )p_data_manager;
	sd_assert( p_bt_data_manager != NULL );
	if( p_bt_data_manager == NULL ) return FALSE;
	LOG_DEBUG( " bdm_dispatcher_ds_is_vod_mod");

	return bdm_is_vod_mode( p_bt_data_manager );	
}


_int32 bdm_get_data_buffer( struct tagBT_DATA_MANAGER * p_bt_data_manager, char **pp_data_buffer, _u32 *p_data_len )
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( " bdm_get_data_buffer");

	ret_val = dm_get_buffer_from_data_buffer( p_data_len, pp_data_buffer );

	if(ret_val == SUCCESS)
	{
		LOG_DEBUG("bdm_get_data_buffer, data_buffer:0x%x, data_len:%u.", *pp_data_buffer, *p_data_len );
	}
	else if( ret_val == DATA_BUFFER_IS_FULL || ret_val== OUT_OF_MEMORY )
	{
		LOG_DEBUG("p_bt_data_manager : 0x%x  , bdm_get_data_buffer failue, so flush data,  data_buffer:0x%x, data_len:%u.", 
			p_bt_data_manager, *pp_data_buffer, *p_data_len ); 

        // 缓冲区满了不强刷磁盘，会导致很多回调
		if(p_bt_data_manager != NULL)
		{
			bfm_notify_flush_data( &p_bt_data_manager->_bt_file_manager );
		}
		
	}

	return ret_val;	
}

_int32 bdm_free_data_buffer( struct tagBT_DATA_MANAGER * p_bt_data_manager, char **pp_data_buffer, _u32 data_len )
{
    _int32 ret_val = SUCCESS;

	LOG_DEBUG("bdm_free_data_buffer,  data_buffer:0x%x, data_len:%u.", *pp_data_buffer, data_len);
	   
    ret_val = dm_free_buffer_to_data_buffer( data_len, pp_data_buffer );

	return ret_val;

}

BOOL bdm_is_piece_ok( struct tagBT_DATA_MANAGER * p_bt_data_manager, _u32 piece_index )
{
	return bitmap_at( &p_bt_data_manager->_piece_bitmap, piece_index );
}

_int32 bdm_speedup_pipe_put_data( struct tagBT_DATA_MANAGER* p_bt_data_manager
        , _u32 file_index
        , const RANGE *p_range
        , char **pp_data_buffer
        , _u32 data_len
        , _u32 buffer_len
		, DATA_PIPE *p_data_pipe
        , RESOURCE *p_resource )
{
	_int32 ret_val = SUCCESS;
	RANGE padding_range;
	BOOL is_checker_put_ok = FALSE;
	BOOL is_success = FALSE;
	_u64 file_size = 0;

	ret_val = brs_file_range_to_padding_range( &p_bt_data_manager->_bt_range_switch
            , file_index
            , p_range
            , &padding_range );
	if( ret_val != SUCCESS )
	{		
		LOG_ERROR( " bfm_put_data err:%d", ret_val );
		dm_free_buffer_to_data_buffer( buffer_len, pp_data_buffer );
		return SUCCESS;
	}
	
	LOG_DEBUG( " bdm_speedup_pipe_put_data, file_index:%u, range:[%u,%u], radding_range:[%u,%u], resource:0x%x",
		file_index, p_range->_index, p_range->_num, padding_range._index, padding_range._num, p_resource );

	ret_val = brs_get_file_size( &p_bt_data_manager->_bt_range_switch, file_index, &file_size );
	CHECK_VALUE( ret_val );
	
	if( data_len != p_range->_num * get_data_unit_size()
		&& (_u64)p_range->_index * get_data_unit_size() + data_len != file_size )
	{
		LOG_ERROR( " bfm_put_data err:invalid data" );
		dm_free_buffer_to_data_buffer( buffer_len, pp_data_buffer );
		return SUCCESS;
	}

	ret_val = bt_checker_put_data( &p_bt_data_manager->_bt_piece_check
            , &padding_range
            , *pp_data_buffer
            , data_len
            , buffer_len );
	if( ret_val != SUCCESS )
	{
		LOG_ERROR( " bt_checker_put_data err:%d", ret_val );
	}
	else
	{
		is_checker_put_ok = TRUE;
	}	
	LOG_DEBUG("[dispatch_stat] range %u_%u check 0x%x %d.", p_range->_index, p_range->_num, p_data_pipe, is_checker_put_ok);

	ret_val = bdm_handle_add_range( p_bt_data_manager, &padding_range, p_resource );
	sd_assert( ret_val == SUCCESS );
	
	ret_val = bfm_put_data( &p_bt_data_manager->_bt_file_manager, file_index, p_range, pp_data_buffer, data_len, buffer_len, p_resource );
	if( ret_val != SUCCESS )
	{
		LOG_ERROR( " bfm_put_data err:%d", ret_val );
		dm_free_buffer_to_data_buffer( buffer_len, pp_data_buffer );
		if( ret_val == FIL_INFO_RECVED_DATA )
		{
			is_success = TRUE;
		}
	}
	else
	{
		is_success = TRUE;
	}

	if( !is_success )
	{
		//ret_val = bdm_handle_del_range( p_bt_data_manager, &padding_range, p_resource );

		ret_val = brdi_del_recved_range( &p_bt_data_manager->_range_download_info, &padding_range );
		sd_assert( ret_val == SUCCESS );	
	}
	
	if( !is_checker_put_ok && !is_success )
	{
		ret_val = range_manager_erase_range( &p_bt_data_manager->_range_manager, &padding_range, NULL );
		sd_assert( ret_val == SUCCESS );	
	}	
	
	if( is_checker_put_ok )
	{
		ret_val = bt_checker_recv_range( &p_bt_data_manager->_bt_piece_check, &padding_range );
		sd_assert( ret_val == SUCCESS );		
	}

	/*
	bdm_check_if_range_related_piece_received( p_bt_data_manager, 
		&padding_range );
	*/
	return SUCCESS;	
}

_int32 bdm_bt_pipe_put_data( struct tagBT_DATA_MANAGER* p_bt_data_manager, const RANGE *p_range, 
                            char **pp_data_buffer, _u32 data_len,_u32 buffer_len, DATA_PIPE *p_data_pipe, RESOURCE *p_resource )
{
    _int32 ret_val = SUCCESS;
    _u32 file_index = 0;
    RANGE sub_file_range;
    BOOL is_success = FALSE;
    BOOL is_notify_checker_success = FALSE;
    //LIST piece_info_list;
    //BOOL is_tmp_file = FALSE;
    LOG_DEBUG( " bdm_bt_pipe_put_data, range:[%u,%u],resouce:0x%x", 
        p_range->_index, p_range->_num, p_resource );

    ret_val = brs_padding_range_to_sub_file_range( &p_bt_data_manager->_bt_range_switch, p_range
        , &file_index, &sub_file_range );
    sd_assert( ret_val == SUCCESS );
    if( ret_val != SUCCESS )
    {		
        LOG_ERROR( " bfm_put_data err:%d", ret_val );
        dm_free_buffer_to_data_buffer( buffer_len, pp_data_buffer );
        return SUCCESS;
    }

    ret_val = bt_checker_put_data( &p_bt_data_manager->_bt_piece_check, p_range
        , *pp_data_buffer, data_len, buffer_len );
    if( ret_val == SUCCESS )
    {
        is_notify_checker_success = TRUE;
    }
    LOG_DEBUG("[dispatch_stat] range %u_%u check 0x%x %d.", p_range->_index, p_range->_num, p_data_pipe, is_notify_checker_success);

    //必须先加range,因为bfm_put_data可能会回调减range
    ret_val = bdm_handle_add_range( p_bt_data_manager, p_range, p_resource );
    sd_assert( ret_val == SUCCESS );

    ret_val = bfm_put_data( &p_bt_data_manager->_bt_file_manager, file_index
        , &sub_file_range, pp_data_buffer, data_len, buffer_len, p_resource );
    if( ret_val != SUCCESS )
    {		
        LOG_ERROR( " bfm_put_data err:%d", ret_val );
        dm_free_buffer_to_data_buffer( buffer_len, pp_data_buffer );
        if( ret_val == FIL_INFO_RECVED_DATA )
        {
            is_success = TRUE;
        }
    }
    else
    {
        is_success = TRUE;
        is_notify_checker_success = TRUE;
    }

    if( !is_success )
    {
        //ret_val = bdm_handle_del_range( p_bt_data_manager, p_range, p_resource );
        //sd_assert( ret_val == SUCCESS );	
        ret_val = brdi_del_recved_range( &p_bt_data_manager->_range_download_info, p_range );
        sd_assert( ret_val == SUCCESS );	
    }

    if( is_notify_checker_success )
    {
        ret_val = bt_checker_recv_range( &p_bt_data_manager->_bt_piece_check, p_range );
        sd_assert( ret_val == SUCCESS );		
    }
    if( !is_notify_checker_success && !is_success )
    {
        //若进行piece校验,file_info关闭,range只加到piecehash里面,不能将range擦除,
        //否则纠错认为没有从这个资源完整的下回过这块,一直会去从这个资源去下这块
        ret_val = range_manager_erase_range( &p_bt_data_manager->_range_manager, p_range, NULL );
        sd_assert( ret_val == SUCCESS );		
    }

    return SUCCESS;
}

_int32  bdm_asyn_read_data_buffer( struct tagBT_DATA_MANAGER* p_bt_data_manager, RANGE_DATA_BUFFER *p_data_buffer, 
	notify_read_result p_call_back, void *p_user, _u32 file_index )
{
	
	LOG_DEBUG( " bdm_asyn_read_data_buffer");
	return bdm_read_data( p_bt_data_manager, file_index, 
		p_data_buffer, p_call_back, p_user );
}
_int32  bdm_read_range_info_list_reorder(  LIST *p_read_range_info_list)
{
	LIST list_tmp;
	LIST_ITERATOR p_list_node=NULL;
	_int32 ret_val=SUCCESS;
	_u32 ListSize = list_size(p_read_range_info_list),ListSize2=0,file_index=0;
	READ_RANGE_INFO * p_read_range_info = NULL,*p_read_range_info_2=NULL;// (READ_RANGE_INFO * )LIST_VALUE(p_list_node);

	LOG_DEBUG( " bdm_read_range_info_list_reorder:ListSize=%u",ListSize);
	
	if(ListSize<2) return SUCCESS;
	
	ListSize2 = ListSize;
	list_init( &(list_tmp));
	list_cat_and_clear(&(list_tmp), p_read_range_info_list);

	ret_val=list_pop(&(list_tmp), (void**)&p_read_range_info);
	CHECK_VALUE(ret_val);
	file_index=p_read_range_info->_file_index;
	ret_val=list_push(p_read_range_info_list, p_read_range_info);
	CHECK_VALUE(ret_val);
	
	while(--ListSize!=0)
	{
		ret_val=list_pop(&(list_tmp),(void**) &p_read_range_info);
		CHECK_VALUE(ret_val);
		if(p_read_range_info->_file_index<file_index)
		{
			//ret_val=list_push(p_read_range_info_list, p_read_range_info);
			ret_val=list_insert(p_read_range_info_list, p_read_range_info, LIST_BEGIN(*p_read_range_info_list));
			CHECK_VALUE(ret_val);
			file_index=p_read_range_info->_file_index;
		}
		else
		{
			for(p_list_node = LIST_BEGIN(*p_read_range_info_list); p_list_node != LIST_END(*p_read_range_info_list); p_list_node = LIST_NEXT(p_list_node))
			{
				p_read_range_info_2 = (READ_RANGE_INFO * )LIST_VALUE(p_list_node);
				/* Check the  p_read_range_info  ...*/
				if(p_read_range_info_2->_file_index>p_read_range_info->_file_index)
				{
					break;
				}
			}

			if(p_list_node!=LIST_END(*p_read_range_info_list))
			{
				p_list_node=LIST_PRE(p_list_node);
			}
			else
			{
				p_list_node=LIST_RBEGIN(*p_read_range_info_list);
			}
			
			p_read_range_info_2 = (READ_RANGE_INFO * )LIST_VALUE(p_list_node);
			while((p_read_range_info_2->_file_index==p_read_range_info->_file_index)&&
				(p_read_range_info_2->_padding_exact_range._pos >= p_read_range_info->_padding_exact_range._pos))
			{
				sd_assert(p_read_range_info_2->_padding_exact_range._pos > p_read_range_info->_padding_exact_range._pos);
				if(p_read_range_info_2->_padding_exact_range._pos == p_read_range_info->_padding_exact_range._pos)
				{
					if(p_read_range_info_2->_padding_exact_range._length<= p_read_range_info->_padding_exact_range._length)
					{
						sd_assert(FALSE);
						break;
					}
				}
				p_list_node=LIST_PRE(p_list_node);
				if(p_list_node==LIST_END(*p_read_range_info_list))
				{
					p_read_range_info_2 = (READ_RANGE_INFO * )LIST_VALUE(LIST_BEGIN(*p_read_range_info_list));
					sd_assert(p_read_range_info_2->_file_index==p_read_range_info->_file_index);
					break;
				}
				p_read_range_info_2 = (READ_RANGE_INFO * )LIST_VALUE(p_list_node);
			}
				
			ret_val=list_insert(p_read_range_info_list, p_read_range_info, LIST_NEXT(p_list_node));
			CHECK_VALUE(ret_val);
		}
	}
	sd_assert(ListSize2==list_size(p_read_range_info_list));
	sd_assert(0==list_size( &(list_tmp)));
	LOG_DEBUG( " bdm_read_range_info_list_reorder end SUCCESS");
	return SUCCESS;
}
/*
_int32  bdm_cache_data_list_reorder(  LIST *p_cache_data_list)
{
	LIST list_tmp;
	LIST_ITERATOR p_list_node=NULL;
	_int32 ret_val=SUCCESS;
	_u32 ListSize = list_size(p_cache_data_list),ListSize2=0,range_index=0;
	//READ_RANGE_INFO *  p_read_range_info = NULL,p_read_range_info_2=NULL;// (READ_RANGE_INFO * )LIST_VALUE(p_list_node);
	RANGE_DATA_BUFFER *p_range_data_buffer=NULL,*p_range_data_buffer_2=NULL;

	LOG_DEBUG( " bdm_cache_data_list_reorder:ListSize=%u",ListSize);
	ListSize2 = ListSize;
	list_init( &(list_tmp));
	list_cat_and_clear(&(list_tmp), p_cache_data_list);

	ret_val=list_pop(&(list_tmp), (void**)&p_range_data_buffer);
	CHECK_VALUE(ret_val);
	range_index=p_range_data_buffer->_range._index;
	ret_val=list_push(p_cache_data_list, p_range_data_buffer);
	CHECK_VALUE(ret_val);
	
	while(--ListSize!=0)
	{
		ret_val=list_pop(&(list_tmp), (void**)&p_range_data_buffer);
		CHECK_VALUE(ret_val);
		if(p_range_data_buffer->_range._index<range_index)
		{
			//ret_val=list_push(p_cache_data_list, p_range_data_buffer);
			ret_val=list_insert(p_cache_data_list, p_range_data_buffer, LIST_BEGIN(*p_cache_data_list));
			CHECK_VALUE(ret_val);
			range_index=p_range_data_buffer->_range._index;
		}
		else
		{
			for(p_list_node = LIST_BEGIN(*p_cache_data_list); p_list_node != LIST_END(*p_cache_data_list); p_list_node = LIST_NEXT(p_list_node))
			{
				p_range_data_buffer_2 = (RANGE_DATA_BUFFER * )LIST_VALUE(p_list_node);
				// Check the  p_read_range_info  ...
				if(p_range_data_buffer_2->_range._index>p_range_data_buffer->_range._index)
				{
					break;
				}
			}

			if(p_list_node!=LIST_END(*p_cache_data_list))
			{
				p_list_node=LIST_PRE(p_list_node);
				
			}
			else
			{
				p_list_node=LIST_RBEGIN(*p_cache_data_list);
			}
			
			p_range_data_buffer_2 = (RANGE_DATA_BUFFER * )LIST_VALUE(p_list_node);
			while((p_range_data_buffer_2->_range._index==p_range_data_buffer->_range._index)&&
				(p_range_data_buffer_2->_data_length >= p_range_data_buffer->_data_length))
			{
				sd_assert(p_range_data_buffer_2->_data_length > p_range_data_buffer->_data_length);
				if(p_range_data_buffer_2->_data_length == p_range_data_buffer->_data_length)
				{
						sd_assert(FALSE);
						break;
				}
				p_list_node=LIST_PRE(p_list_node);
				if(p_list_node!=LIST_END(*p_cache_data_list))
				{
					sd_assert(FALSE);
					break;
				}
				p_range_data_buffer_2 = (RANGE_DATA_BUFFER * )LIST_VALUE(p_list_node);
			}
				
			ret_val=list_insert(p_cache_data_list, p_range_data_buffer, LIST_NEXT(p_list_node));
			CHECK_VALUE(ret_val);
		}
	}
	sd_assert(ListSize2==list_size(p_cache_data_list));
	sd_assert(0==list_size( &(list_tmp)));
	return SUCCESS;
}
*/
_int32  bdm_bt_pipe_read_data_buffer( struct tagBT_DATA_MANAGER* p_bt_data_manager, const BT_RANGE *p_bt_range, 
	char *p_data_buffer, _u32 buffer_len, bt_pipe_read_result p_call_back, void *p_user )
{
	_int32 ret_val = SUCCESS;
	LIST read_range_info_list,*read_range_info_list_for_file=NULL;
	READ_RANGE_INFO * p_read_range_info=NULL;
	LIST_ITERATOR p_list_node=NULL;
	RANGE_DATA_BUFFER_LIST range_data_buffer_list;
	READ_RANGE_INFO_FOR_FILE * p_read_range_info_for_file=NULL;
	char *data_buffer=p_data_buffer;
	_u32 total_buffer_len=buffer_len,total_data_len=0,data_len=0,data_len_read_from_cache=0,data_len_read_from_fm=0;
	RANGE one_padding_range_to_read;//piece在padding后所处子文件中的range
	EXACT_RANGE padding_exact_range;//piece在padding后所处的在子文件内的精确range
	_u32 index=0;
	_u64 des_pos=0,src_pos=0;
	_u32 des_data_len=0;

	LOG_DEBUG( " bdm_bt_pipe_read_data_buffer:list_size of _bt_pipe_reader_mgr_list = %u,p_bt_range[%llu,%llu],bt_pipe=0x%X",list_size(&(p_bt_data_manager->_bt_pipe_reader_mgr_list)) ,p_bt_range->_pos,p_bt_range->_length,p_user);

	/* Check if the number of pipe reader get to the max number  */
	//if(list_size(&(p_bt_data_manager->_bt_pipe_reader_mgr_list))>=MAX_NUM_OF_PIPE_READER)
	//	return BT_PIPE_READER_FULL;
	
	list_init( &(read_range_info_list));
	ret_val = sd_malloc(sizeof(LIST), (void**)&read_range_info_list_for_file);  /* It should be released by BT_PIPE_READER_MANAGER */
	if(ret_val!=SUCCESS) return ret_val;
	list_init( read_range_info_list_for_file);
	list_init( &(range_data_buffer_list._data_list));
	/* Check if all the data in  p_bt_range is valid */
	ret_val = brs_bt_range_to_read_range_info_list( &(p_bt_data_manager->_bt_range_switch), (BT_RANGE *)p_bt_range, &(read_range_info_list) );
	CHECK_VALUE( ret_val );

	if(0!=list_size( &(read_range_info_list)))
	{
		/* Make sure the read_range_info_list is in the order of  file index and start positon */
		if(1<list_size( &(read_range_info_list)))
		{
			ret_val = bdm_read_range_info_list_reorder(  &(read_range_info_list));
			CHECK_VALUE( ret_val );
		} 

		for(p_list_node = LIST_BEGIN(read_range_info_list); p_list_node != LIST_END(read_range_info_list); p_list_node = LIST_NEXT(p_list_node))
		{
			p_read_range_info = (READ_RANGE_INFO * )LIST_VALUE(p_list_node);
			/* Check the  p_read_range_infois valid ...*/
			if(FALSE==bdm_is_range_can_read( p_bt_data_manager, p_read_range_info->_file_index, &(p_read_range_info->_padding_range)))
			{
				/* Some data is not valid */
				ret_val=-1;
				LOG_DEBUG( " bdm_bt_pipe_read_data_buffer:Some data is not valid" );
				goto ErrorHanle;
			}
		}
	}	
	else
	{
		LOG_DEBUG( " bdm_bt_pipe_read_data_buffer:brs_bt_range_to_read_range_info_list return list size ==0" );
		return -1;
	}

	/* OK all the data is valid,ready to read data from cache or file manager to buffer */
	LOG_DEBUG( " bdm_bt_pipe_read_data_buffer:OK all the data is valid,ready to read data from cache or file manager to buffer:list_size=%u",list_size( &(read_range_info_list)) );

	/* first read data from cache to buffer */
	if(0!=list_size( &(read_range_info_list)))
	{
		data_len=total_buffer_len;
		
		for(p_list_node = LIST_BEGIN(read_range_info_list); p_list_node != LIST_END(read_range_info_list); p_list_node = LIST_NEXT(p_list_node))
		{
			p_read_range_info = (READ_RANGE_INFO * )LIST_VALUE(p_list_node);
			
			des_pos=p_read_range_info->_padding_exact_range._pos;
	 		des_data_len=p_read_range_info->_padding_exact_range._length;			
			one_padding_range_to_read._num=1;

			LOG_DEBUG( " bdm_bt_pipe_read_data_buffer:p_read_range_info->_padding_range._num=%u",p_read_range_info->_padding_range._num );
			
			for(index=0;index<p_read_range_info->_padding_range._num;index++)
			{
				one_padding_range_to_read._index=p_read_range_info->_padding_range._index+index;
				if(TRUE==bdm_range_is_cache( p_bt_data_manager, p_read_range_info->_file_index, &one_padding_range_to_read ))
				{
					LOG_DEBUG( " bdm_bt_pipe_read_data_buffer:one_padding_range_to_read[%u,1] is found in cache",one_padding_range_to_read._index);
					list_init( &(range_data_buffer_list._data_list));
					ret_val = bdm_get_cache_data_buffer( p_bt_data_manager, p_read_range_info->_file_index, &one_padding_range_to_read, &range_data_buffer_list);
					sd_assert(ret_val==SUCCESS);

					data_len=total_buffer_len;
					sd_assert(des_data_len<=data_len);
					//if(des_data_len>data_len)
					//	des_data_len=data_len;

					ret_val = bdm_read_cache_data_to_buffer( des_pos, des_data_len,&(range_data_buffer_list._data_list),data_buffer,&data_len);
					if(ret_val!=SUCCESS) goto ErrorHanle;
					data_buffer+=data_len;
					sd_assert(total_buffer_len>=data_len);
					total_buffer_len-=data_len;
					data_len_read_from_cache+=data_len;
					des_pos+=data_len;
					sd_assert(des_data_len>=data_len);
					des_data_len-=data_len;
					
					ret_val = list_clear( &(range_data_buffer_list._data_list));
					if(ret_val!=SUCCESS) goto ErrorHanle;
				
				}
				else
				{
					/* The data is not found in cache,should be read from file manager */
					data_len=total_buffer_len;
					LOG_DEBUG( " bdm_bt_pipe_read_data_buffer:one_padding_range_to_read[%u,1] is not found in cache,need read from file",one_padding_range_to_read._index);
					
					ret_val = sd_malloc(sizeof(READ_RANGE_INFO_FOR_FILE), (void**)&p_read_range_info_for_file);
					if(ret_val!=SUCCESS) goto ErrorHanle;
					sd_memset(p_read_range_info_for_file,0,sizeof(READ_RANGE_INFO_FOR_FILE));


					p_read_range_info_for_file->data_buffer = data_buffer;
					p_read_range_info_for_file->_file_index= p_read_range_info->_file_index;
					p_read_range_info_for_file->_padding_range._index= one_padding_range_to_read._index;
					p_read_range_info_for_file->_padding_range._num= one_padding_range_to_read._num;

					padding_exact_range._pos=des_pos;

					src_pos = get_data_pos_from_index(one_padding_range_to_read._index);
					sd_assert(src_pos<=des_pos);
					padding_exact_range._length=get_data_unit_size()-(des_pos-src_pos);
					if(padding_exact_range._length>des_data_len)
					{
						padding_exact_range._length=des_data_len;
					}
					
					ret_val = brs_sub_file_exact_range_to_bt_range(&(p_bt_data_manager->_bt_range_switch),p_read_range_info->_file_index,&padding_exact_range,&(p_read_range_info_for_file->bt_range));
					if(ret_val!=SUCCESS) 
					{
						SAFE_DELETE(p_read_range_info_for_file);
						goto ErrorHanle;
					}

					ret_val = list_push( read_range_info_list_for_file,(void*)p_read_range_info_for_file);
					if(ret_val!=SUCCESS) 
					{
						SAFE_DELETE(p_read_range_info_for_file);
						goto ErrorHanle;
					}
					
					data_len=padding_exact_range._length;
					data_buffer+=data_len;
					sd_assert(total_buffer_len>=data_len);
					total_buffer_len-=data_len;
					des_pos+=data_len;
					sd_assert(des_data_len>=data_len);
					des_data_len-=data_len;
				
				}
			}

		}
	}	
	sd_assert(p_bt_range->_length>=data_len_read_from_cache);
	sd_assert(data_len_read_from_cache>=data_len_read_from_fm);
	total_data_len=p_bt_range->_length-(data_len_read_from_cache-data_len_read_from_fm);
	LOG_DEBUG( " bdm_bt_pipe_read_data_buffer:total_data_len=%llu,data_len_read_from_cache=%u,data_len_read_from_file=%u" ,p_bt_range->_length,data_len_read_from_cache,total_data_len);
	if(total_data_len!=0)
	{
		/* then read the rest data from file manager to buffer */
		ret_val = bdm_read_file_data_to_buffer( p_bt_data_manager,p_bt_range, p_data_buffer, p_bt_range->_length,total_data_len,p_call_back, p_user,  &read_range_info_list_for_file);
		if(ret_val!=SUCCESS) goto ErrorHanle;

		brs_release_read_range_info_list( &(read_range_info_list));
	}
	else
	{
		brs_release_read_range_info_list( &(read_range_info_list));
		p_call_back( p_user, (BT_RANGE *)p_bt_range, p_data_buffer,0, p_bt_range->_length );
	}
	
	LOG_DEBUG( " bdm_bt_pipe_read_data_buffer:SUCCESS" );
	return SUCCESS;	
	
ErrorHanle:
	if(read_range_info_list_for_file!=NULL)
	{
		if(0!=list_size( read_range_info_list_for_file))
		{
			for(p_list_node = LIST_BEGIN(*read_range_info_list_for_file); p_list_node != LIST_END(*read_range_info_list_for_file); p_list_node = LIST_NEXT(p_list_node))
			{
				p_read_range_info_for_file = (READ_RANGE_INFO_FOR_FILE * )LIST_VALUE(p_list_node);
				SAFE_DELETE(p_read_range_info_for_file);
			}
				list_clear( read_range_info_list_for_file);
		}	
		SAFE_DELETE(read_range_info_list_for_file);
	}
	if(0!=list_size( &(range_data_buffer_list._data_list)))
		list_clear( &(range_data_buffer_list._data_list));
	
	brs_release_read_range_info_list( &(read_range_info_list));

	return ret_val;
}

/* Added by zyq @ 20090327 */
_int32  bdm_bt_pipe_clear_read_data_buffer( struct tagBT_DATA_MANAGER* p_bt_data_manager,  void *p_user )
{
	LIST_ITERATOR p_list_node=NULL;
	BT_PIPE_READER_MANAGER * p_pipe_reader_mgr=NULL;

	LOG_DEBUG( " bdm_bt_pipe_clear_read_data_buffer:list_size of _bt_pipe_reader_mgr_list = %u,bt_pipe=0x%X",list_size(&(p_bt_data_manager->_bt_pipe_reader_mgr_list)) ,p_user);

	if(0!=list_size(&(p_bt_data_manager->_bt_pipe_reader_mgr_list)))
	{
			for(p_list_node = LIST_BEGIN(p_bt_data_manager->_bt_pipe_reader_mgr_list); p_list_node != LIST_END(p_bt_data_manager->_bt_pipe_reader_mgr_list); p_list_node = LIST_NEXT(p_list_node))
			{
				p_pipe_reader_mgr = (BT_PIPE_READER_MANAGER * )LIST_VALUE(p_list_node);
				if(p_pipe_reader_mgr->_p_user==p_user)
				{
					bprm_clear(p_pipe_reader_mgr);
				}
			}
	}
	
	LOG_DEBUG( " bdm_bt_pipe_clear_read_data_buffer:SUCCESS" );
	return SUCCESS;	
	
}
_int32  bdm_read_cache_data_to_buffer( _u64 des_pos,_u32 des_data_len , LIST * cache_data_list,char *data_buffer,_u32 *data_len)
{
	RANGE_DATA_BUFFER * p_range_data_buffer=(RANGE_DATA_BUFFER * )LIST_VALUE(LIST_BEGIN(*cache_data_list));
	_u64 src_pos=0;
	_u32 src_data_len=0;
	char *des_buffer=data_buffer,*src_buffer=NULL;

	LOG_DEBUG( " bdm_read_cache_data_to_buffer:list_size=%u",list_size( cache_data_list) );

	sd_assert(list_size( cache_data_list) ==1);
	
	//if(des_data_len>*data_len)
	//	des_data_len=*data_len;

	*data_len=0;

	src_pos = get_data_pos_from_index(p_range_data_buffer->_range._index);
	LOG_DEBUG( " bdm_read_cache_data_to_buffer 1:des_pos=%llu,src_pos=%llu,src_data_len=%u,range(%u,%u)",des_pos,src_pos, p_range_data_buffer->_data_length,p_range_data_buffer->_range._index,p_range_data_buffer->_range._num);
	src_data_len=p_range_data_buffer->_data_length;
	sd_assert(src_pos<=des_pos);
	sd_assert(src_pos+src_data_len>des_pos);
	
	src_data_len-=(des_pos-src_pos);
	if(src_data_len>des_data_len)
	{
		src_data_len=des_data_len;
	}
	LOG_DEBUG( " bdm_read_cache_data_to_buffer 2:des_pos=%llu,src_pos=%llu,src_data_len=%u,des_data_len=%u,(*data_len)=%u",des_pos,src_pos,src_data_len, des_data_len,(*data_len));
	src_buffer=p_range_data_buffer->_data_ptr+(des_pos-src_pos);
	sd_memcpy(des_buffer, src_buffer, src_data_len);
	(*data_len)=src_data_len;
	
	sd_assert((*data_len)!=0);
	LOG_DEBUG( " bdm_read_cache_data_to_buffer end SUCCESS! " );
	return SUCCESS;
}

_int32  bdm_read_file_data_to_buffer( struct tagBT_DATA_MANAGER* p_bt_data_manager, const BT_RANGE *p_bt_range,char *p_data_buffer,_u32 total_data_len,_u32 data_len,
	 bt_pipe_read_result p_call_back, void *p_user, LIST ** read_range_info_list_for_file)
{
	_int32 ret_val = SUCCESS;

	BT_PIPE_READER_MANAGER *p_pipe_reader_mgr=NULL;

	LOG_DEBUG( " bdm_read_file_data_to_buffer" );
	
	ret_val = bpr_pipe_reader_mgr_malloc_wrap(&p_pipe_reader_mgr);
	CHECK_VALUE( ret_val );

	ret_val = bprm_init_struct( p_pipe_reader_mgr ,p_bt_data_manager,p_bt_range, p_data_buffer,total_data_len,data_len,(void*) p_call_back, p_user,*read_range_info_list_for_file);
	*read_range_info_list_for_file=NULL;
	if(ret_val!=SUCCESS) goto ErrorHanle;

	ret_val = bprm_add_read_range( p_pipe_reader_mgr );
	if(ret_val!=SUCCESS) goto ErrorHanle;

	ret_val = list_push(&(p_bt_data_manager->_bt_pipe_reader_mgr_list),(void*)p_pipe_reader_mgr);
	if(ret_val!=SUCCESS) goto ErrorHanle;
	
return SUCCESS;
	
ErrorHanle:

	bprm_uninit_struct(p_pipe_reader_mgr );

	bpr_pipe_reader_mgr_free_wrap(p_pipe_reader_mgr);
	//CHECK_VALUE( ret_val );
	return ret_val;
}
/* zyq adding end  @ 20090327 */

_u64 bdm_get_sub_file_size( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index )
{
	
	LOG_DEBUG("bdm_get_sub_file_size .");
	return bfm_get_sub_file_size( &p_bt_data_manager->_bt_file_manager, file_index ); 
}

_u64 bdm_get_file_size( struct tagBT_DATA_MANAGER *p_bt_data_manager )
{
	
	LOG_DEBUG("bdm_get_file_size .");
	return bfm_get_file_size( &p_bt_data_manager->_bt_file_manager );	
}

_int32 bdm_set_sub_file_size( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index, _u64 file_size )
{
	_u64 torrent_file_size = bfm_get_sub_file_size( &p_bt_data_manager->_bt_file_manager, file_index ); 
	LOG_DEBUG("bdm_set_sub_file_size .");
	if( file_size == torrent_file_size )
	{
		return SUCCESS;
	}
	else
	{
		return SD_INVALID_FILE_SIZE;
	}
}

enum tagPIPE_FILE_TYPE bdm_get_sub_file_type( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index )
{
	LOG_DEBUG("bdm_get_sub_file_type .");
	return bfm_get_sub_file_type( &p_bt_data_manager->_bt_file_manager, file_index ); 
}

_int32 bdm_notify_dispatch_data_finish( struct tagBT_DATA_MANAGER* p_bt_data_manager, DATA_PIPE *ptr_data_pipe )
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG("bdm_notify_dispatch_data_finish .");
	ret_val = dt_dispatch_at_pipe( p_bt_data_manager->_task_ptr, ptr_data_pipe );
	sd_assert( ret_val == SUCCESS );
	return SUCCESS;	
}

_u32 bdm_get_total_piece_num( struct tagBT_DATA_MANAGER* p_bt_data_manager )
{
	LOG_DEBUG("bdm_get_total_piece_num .");
	return brs_get_piece_num( &p_bt_data_manager->_bt_range_switch );
}

_u32 bdm_get_piece_size( struct tagBT_DATA_MANAGER* p_bt_data_manager )
{
	LOG_DEBUG("bdm_get_piece_size .");
	return brs_get_piece_size( &p_bt_data_manager->_bt_range_switch );
}

_int32 bdm_get_complete_bitmap( struct tagBT_DATA_MANAGER* p_bt_data_manager, struct tagBITMAP **pp_bitmap )
{
	LOG_DEBUG("bdm_get_complete_bitmap .");
	bitmap_print(&p_bt_data_manager->_piece_bitmap);
	*pp_bitmap = &p_bt_data_manager->_piece_bitmap;	
	return SUCCESS;	
}

_int32 bdm_get_info_hash( struct tagBT_DATA_MANAGER* p_bt_data_manager, unsigned char **pp_info_hash )
{
	LOG_DEBUG("bdm_get_info_hash .");
	
	return tp_get_file_info_hash( p_bt_data_manager->_bt_file_manager._torrent_parser_ptr, pp_info_hash );	
}

/*-----------------------------------*/
BOOL bdm_range_is_in_need_range(struct tagBT_DATA_MANAGER* p_bt_data_manager, RANGE* p_range)
{
	return  range_list_is_relevant(&p_bt_data_manager->_range_download_info._cur_need_download_range, p_range);
}

_int32 bdm_set_emerge_rangelist( struct tagBT_DATA_MANAGER* p_bt_data_manager, _u32 file_index, RANGE_LIST *em_range_list )
{
	RANGE_LIST_ITEROATOR range_iter = NULL;
	RANGE_LIST_ITEROATOR range_first_iter = NULL;
	_int32 ret_val = SUCCESS;
	_u32 p2sp_view_pos = 0;
	RANGE tmp_range;
	if( em_range_list == NULL ) return SUCCESS;

	LOG_DEBUG("bdm_set_emerge_rangelist ...,file_index:%u", file_index );
	ret_val = brs_get_file_p2sp_pos( &p_bt_data_manager->_bt_range_switch, file_index, &p2sp_view_pos );
	CHECK_VALUE( ret_val );
	
	if( p_bt_data_manager->_dap_file_index != file_index )
	{	
		ret_val = bfm_vod_only_open_file_info( &p_bt_data_manager->_bt_file_manager, file_index );
		if( ret_val != SUCCESS )
		{
			return ret_val;
		}
		p_bt_data_manager->_start_pos_index = p2sp_view_pos;
	}
	
	p_bt_data_manager->_dap_file_index = file_index;

	correct_manager_clear_prority_range_list( &p_bt_data_manager->_correct_manager );

	LOG_DEBUG("bdm_set_emerge_rangelist em_range_list,file_index:%u, _start_pos_index:%u", 
		file_index, p_bt_data_manager->_start_pos_index );

	out_put_range_list(em_range_list);
	range_list_get_head_node( em_range_list, &range_first_iter );

	range_iter = range_first_iter;
	while( range_iter != NULL )
	{
		tmp_range._index = range_iter->_range._index;
		tmp_range._num = range_iter->_range._num;
		tmp_range._index += p2sp_view_pos;
		LOG_DEBUG("bdm_set_emerge_rangelist ...,file_index:%u, add_prority_range:[%u,%u]", 
			file_index, tmp_range._index, tmp_range._num );
		
		correct_manager_add_prority_range( &p_bt_data_manager->_correct_manager, &tmp_range );	  
		
		range_list_get_next_node( em_range_list, range_iter, &range_iter );
	}

	sd_assert( p_bt_data_manager->_IS_VOD_MOD );

	if( range_first_iter != NULL )
	{
		p_bt_data_manager->_start_pos_index = range_first_iter->_range._index + range_first_iter->_range._num + p2sp_view_pos;
		LOG_DEBUG("bdm_set_emerge_rangelist em_range_list,file_index:%u, _start_pos_index:%u", 
			file_index, p_bt_data_manager->_start_pos_index );

		dt_start_dispatcher_immediate( p_bt_data_manager->_task_ptr );		  
	}

	p_bt_data_manager->_conrinue_times = 0;

	return SUCCESS;

}

_int32 bdm_get_range_data_buffer( struct tagBT_DATA_MANAGER* p_bt_data_manager, _u32 file_index, RANGE need_r, RANGE_DATA_BUFFER_LIST *ret_range_buffer )
{
	LOG_DEBUG("bdm_get_range_data_buffer .");
	return bfm_get_sub_file_cache_data( &p_bt_data_manager->_bt_file_manager, file_index,
		&need_r, ret_range_buffer );
}

BOOL  bdm_range_is_recv( struct tagBT_DATA_MANAGER* p_bt_data_manager, _u32 file_index, RANGE *p_range )
{
	return bfm_range_is_recv( &p_bt_data_manager->_bt_file_manager, file_index, p_range );
}

BOOL  bdm_range_is_cache( struct tagBT_DATA_MANAGER* p_bt_data_manager, _u32 file_index, RANGE *p_range )
{
	LOG_DEBUG("bdm_range_is_cache .");
	return bdm_file_range_is_cached( p_bt_data_manager, file_index, p_range );
}

BOOL  bdm_range_is_write( struct tagBT_DATA_MANAGER* p_bt_data_manager, _u32 file_index, RANGE *p_range )
{
	BOOL ret_bool = FALSE;
	
	ret_bool = bdm_range_is_write_in_disk( p_bt_data_manager, file_index, p_range );
	
	LOG_DEBUG("bdm_range_is_write:%d.", ret_bool);
	return ret_bool;
}

RANGE_LIST *bdm_get_resv_range_list( struct tagBT_DATA_MANAGER* p_bt_data_manager, _u32 file_index  )
{
	return bfm_get_resved_range_list( &p_bt_data_manager->_bt_file_manager, file_index );
}

RANGE_LIST *bdm_get_check_range_list( struct tagBT_DATA_MANAGER* p_bt_data_manager, _u32 file_index  )
{
	return bfm_get_checked_range_list( &p_bt_data_manager->_bt_file_manager, file_index );
}

BOOL bdm_is_vod_mode( struct tagBT_DATA_MANAGER *p_bt_data_manager )
{
	LOG_DEBUG( "bdm_is_vod_mode:%d", p_bt_data_manager->_IS_VOD_MOD );
	return p_bt_data_manager->_IS_VOD_MOD;
}

void bdm_set_vod_mode( struct tagBT_DATA_MANAGER* p_bt_data_manager, BOOL is_vod_mode )
{
	LOG_DEBUG("bdm_set_vod_mode:%d.", is_vod_mode);
	p_bt_data_manager->_IS_VOD_MOD = is_vod_mode;
	if( !is_vod_mode )
	{
		p_bt_data_manager->_dap_file_index = MAX_U32;
	}
}

_int32 bdm_get_file_name( struct tagBT_DATA_MANAGER* p_bt_data_manager, _u32 file_index, char **pp_file_name )
{
	if( p_bt_data_manager->_bt_file_manager._torrent_parser_ptr == SUCCESS )
	{
		return BT_DATA_MANAGER_LOGIC_ERROR;
	}
	return tp_get_file_name_ptr( p_bt_data_manager->_bt_file_manager._torrent_parser_ptr, file_index, pp_file_name );
}

void bdm_get_dl_bytes( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u64 *p_server_dl_bytes, _u64 *p_peer_dl_bytes, _u64 *p_cdn_dl_bytes, _u64* p_origion_dl_bytes )
{
	LOG_DEBUG("bdm_get_dl_bytes .");
	bfm_get_dl_size( &p_bt_data_manager->_bt_file_manager, p_server_dl_bytes, p_peer_dl_bytes, p_cdn_dl_bytes, p_origion_dl_bytes ); 	
}

void bdm_get_sub_file_dl_bytes( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index, _u64 *p_server_dl_bytes, _u64 *p_peer_dl_bytes, _u64 *p_cdn_dl_bytes, _u64 *p_lixian_dl_bytes, _u64* p_origion_dl_bytes)
{
	LOG_DEBUG("bdm_get_sub_file_dl_bytes .");
	bfm_get_sub_file_dl_size( &p_bt_data_manager->_bt_file_manager, file_index, p_server_dl_bytes, p_peer_dl_bytes, p_cdn_dl_bytes,p_lixian_dl_bytes, p_origion_dl_bytes ); 
	
}

_u64 bdm_get_sub_file_normal_cdn_bytes(struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index)
{
	LOG_DEBUG("bdm_get_sub_file_normal_cdn_bytes.");
	return bfm_get_sub_file_normal_cdn_size(&p_bt_data_manager->_bt_file_manager, file_index);
}

_u64 bdm_get_normal_cdn_down_bytes(struct tagBT_DATA_MANAGER *p_bt_data_manager)
{
	LOG_DEBUG("bdm_get_normal_cdn_down_bytes.");
	return bfm_get_normal_cdn_size(&p_bt_data_manager->_bt_file_manager);
}

_int32 bdm_stop_report_running_sub_file( struct tagBT_DATA_MANAGER *p_bt_data_manager)
{
	return bfm_stop_report_running_sub_file(&p_bt_data_manager->_bt_file_manager);
}

BOOL bdm_is_waited_success_close_file( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index )
{
	LOG_DEBUG("bdm_is_waited_success_close_file .");
	return bfm_is_waited_success_close_file( &p_bt_data_manager->_bt_file_manager, file_index ); 
}

void bdm_set_del_tmp_flag( struct tagBT_DATA_MANAGER *p_bt_data_manager )
{
	LOG_DEBUG("bdm_set_del_tmp_flag .");
    bt_checker_del_tmp_file( &p_bt_data_manager->_bt_piece_check );
}

void bdm_subfile_range(void * p_data_manager, _u32 file_index ,const RANGE* big_range, RANGE* sub_range)
{
	_u64 p_file_size = 0;
	struct tagBT_DATA_MANAGER *p_bt_data_manager = ( struct tagBT_DATA_MANAGER * )p_data_manager;
	_int32 ret =  brs_get_file_size(&p_bt_data_manager->_bt_range_switch, file_index, &p_file_size);
	sd_assert(ret == SUCCESS );
	_u32 file_range_num = (p_file_size + get_data_unit_size()- 1) / get_data_unit_size();

	_u32  file_p2sp_pos = 0;
	ret = brs_get_file_p2sp_pos(&p_bt_data_manager->_bt_range_switch,  file_index, &file_p2sp_pos);
	sd_assert(ret == SUCCESS);
	_u64 end_pos = file_p2sp_pos + file_range_num;
	sd_assert(big_range->_index < end_pos);
	
	if(big_range->_index > file_p2sp_pos)
	{
		sub_range->_index = big_range->_index;
	}
	else
	{
		sub_range->_index = file_p2sp_pos;
	}

	if(big_range->_index + big_range->_num > end_pos)
	{
		sub_range->_num = end_pos - sub_range->_index ;
	}
	else
	{
		sub_range->_num = big_range->_index + big_range->_num - sub_range->_index ;
	}

	LOG_DEBUG("bdm_subfile_range file_index:%u, big_range(%u,%u) sub_range(%u,%u)", file_index, big_range->_index, big_range->_num, sub_range->_index, sub_range->_num);
}

void bdm_filter_uncomplete_rangelist(void * p_data_manager, RANGE_LIST * p_un_complete_range_list)
{
	LOG_DEBUG("###########bdm_filter_uncomplete_rangelist");

	const static  _u32 const_max_assgin_range_num_per_subfile = 32*1024*1024/1024/16;
	const static  _u32 const_min_assign_range_num = 0;

	struct tagBT_DATA_MANAGER *p_bt_data_manager = ( struct tagBT_DATA_MANAGER * )p_data_manager;
	sd_assert( p_bt_data_manager != NULL );
		
	RANGE_LIST_NODE* cur_node = NULL;

	struct _filter_file_index_info
	{
		_u32 _cur_filter_index;
		_u32 _has_assign_range_num;
	};

	out_put_range_list(p_un_complete_range_list);

	struct _filter_file_index_info filter_file_info;
	filter_file_info._cur_filter_index = -1;
	filter_file_info._has_assign_range_num = 0;
	
	
	RANGE_LIST_NODE* next_node = NULL;
	RANGE_LIST_NODE* prev_node = NULL;
	for(cur_node = p_un_complete_range_list->_head_node; cur_node != NULL; cur_node = next_node)
	{
		prev_node = cur_node->_prev_node;
		next_node = cur_node->_next_node;
		
	
		_u32 p_range_first_file_index = 0;
		_u32 p_range_last_file_index = 0;
		RANGE r =  cur_node->_range;

		_int32 ret = brs_search_file_index(&p_bt_data_manager->_bt_range_switch,  (_u64)r._index,  (_u64)r._num,  brs_get_file_padding_pos,  &p_range_first_file_index, &p_range_last_file_index);		
		sd_assert(ret == SUCCESS);
		sd_assert(p_range_first_file_index <= p_range_last_file_index);

		if(p_range_first_file_index == p_range_last_file_index)
		{
			if(filter_file_info._cur_filter_index == -1  || filter_file_info._cur_filter_index != p_range_first_file_index)
			{
				filter_file_info._cur_filter_index = p_range_first_file_index;
				filter_file_info._has_assign_range_num = 0;
			}			

			_u32 max_assign_num = const_max_assgin_range_num_per_subfile - filter_file_info._has_assign_range_num;
			sd_assert(max_assign_num <= const_max_assgin_range_num_per_subfile);

			_u32 real_assign_num = 0;
			if(max_assign_num < const_min_assign_range_num)
			{
				//  太小了，不分配小块，避免太多的分散块
				range_list_erase(p_un_complete_range_list,  cur_node);
				cur_node = NULL;		
			}
			else
			{
				real_assign_num = r._num > max_assign_num ?( max_assign_num) :  r._num;
				cur_node->_range._num = real_assign_num;
			}

			filter_file_info._has_assign_range_num += real_assign_num;
			
			LOG_DEBUG("bdm_filter_uncomplete_rangelist same file max_assign_num:%u, real_assign_num:%u, filter(%u,%u), sub_range(%u, %u)",
				max_assign_num, real_assign_num, filter_file_info._cur_filter_index, filter_file_info._has_assign_range_num, r._index, r._num);

			continue;
		}

		range_list_erase(p_un_complete_range_list,  cur_node);
		cur_node = NULL;		
	
		while(p_range_first_file_index <= p_range_last_file_index)
		{
		
			if(filter_file_info._cur_filter_index == -1 )
			{
				filter_file_info._cur_filter_index = p_range_first_file_index;
				filter_file_info._has_assign_range_num = 0;
			}

			if(filter_file_info._cur_filter_index != p_range_first_file_index)
			{
				// range一定是排好顺序的
				sd_assert(filter_file_info._cur_filter_index < p_range_first_file_index);

				filter_file_info._cur_filter_index = p_range_first_file_index;
				filter_file_info._has_assign_range_num = 0;
			}



			_u32 max_assign_num = const_max_assgin_range_num_per_subfile - filter_file_info._has_assign_range_num;
			sd_assert(max_assign_num <= const_max_assgin_range_num_per_subfile);

			if(max_assign_num < const_min_assign_range_num)
			{
				LOG_DEBUG("bdm_filter_uncomplete_rangelist not assign more because max_assign_num:%u too min,const_min_assign_range_num:%u", max_assign_num,
					const_min_assign_range_num);
			}
			else	
			{

				RANGE sub_range;
				bdm_subfile_range(p_bt_data_manager, filter_file_info._cur_filter_index, &r, &sub_range);


				_u32 real_assign_num = 0;
			
				real_assign_num = sub_range._num > max_assign_num ? max_assign_num :  sub_range._num;

			
				if(real_assign_num > 0)
				{
					sub_range._num = real_assign_num;

					// 循环过程中修改链表要特别小心
					// 这里加进去不可能影响现有的列表顺序，下一个节点位置肯定不会被改变
					range_list_add_range(p_un_complete_range_list, &sub_range, prev_node, NULL);
				}

				filter_file_info._has_assign_range_num += real_assign_num;
		
				LOG_DEBUG("bdm_filter_uncomplete_rangelist max_assign_num:%u, real_assign_num:%u, filter(%u,%u), sub_range(%u, %u)",
					max_assign_num, real_assign_num, filter_file_info._cur_filter_index, filter_file_info._has_assign_range_num, sub_range._index, sub_range._num);
			}
			
			p_range_first_file_index++;
		}
		
	}


	out_put_range_list(p_un_complete_range_list);

	LOG_DEBUG("###########bdm_filter_uncomplete_rangelist exit");
}


#ifdef EMBED_REPORT
struct tagFILE_INFO_REPORT_PARA *bdm_get_report_para( struct tagBT_DATA_MANAGER *p_bt_data_manager, _u32 file_index )
{
	return bfm_get_report_para( &p_bt_data_manager->_bt_file_manager, file_index );
}
#endif 

#endif /* #ifdef ENABLE_BT */

