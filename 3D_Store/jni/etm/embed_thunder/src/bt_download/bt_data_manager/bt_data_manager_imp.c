#include "utility/define.h"
#ifdef ENABLE_BT 


#include "bt_data_manager_imp.h"
#include "bt_data_manager_interface.h"

#include "bt_piece_checker.h"
#include "data_manager/file_info.h"
#include "data_manager/data_receive.h"
#include "../bt_task/bt_task_interface.h"
#include "utility/utility.h"
#include "utility/define_const_num.h"
#ifdef UPLOAD_ENABLE
#include "upload_manager/upload_manager.h"
#endif

#include "utility/logid.h"
#define LOGID LOGID_BT_DOWNLOAD
#include "utility/logger.h"

#include "reporter/reporter_interface.h"

static void bdm_file_manager_delete_error_block(BT_DATA_MANAGER *p_bt_data_manager
    , RANGE* padding_range);


_int32 bdm_handle_add_range( BT_DATA_MANAGER *p_bt_data_manager, const RANGE *p_range, RESOURCE *p_resource )
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "bdm_handle_add_range.range:[%u,%u], resource:0x%x",
		p_range->_index, p_range->_num, p_resource );

	ret_val = brdi_add_recved_range( &p_bt_data_manager->_range_download_info, p_range );
	CHECK_VALUE( ret_val );

	ret_val = put_range_record( &p_bt_data_manager->_range_manager, p_resource, p_range );

	CHECK_VALUE( ret_val );
	
	return SUCCESS;
}

_int32 bdm_handle_del_range( BT_DATA_MANAGER *p_bt_data_manager, const RANGE *p_range, RESOURCE *p_resource )
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "bdm_handle_del_range.range:[%u,%u], resource:0x%x",
		p_range->_index, p_range->_num, p_resource );

	ret_val = brdi_del_recved_range( &p_bt_data_manager->_range_download_info, p_range );
	CHECK_VALUE( ret_val );

	ret_val = range_manager_erase_range( &p_bt_data_manager->_range_manager, p_range, NULL );

	CHECK_VALUE( ret_val );
	
	return SUCCESS;
}

/*
_int32 bdm_try_to_check_piece( BT_DATA_MANAGER *p_bt_data_manager, const LIST *p_piece_info_list )
{
	_int32 ret_val = SUCCESS;
	LIST_ITERATOR cur_node = LIST_BEGIN( *p_piece_info_list );
	PIECE_RANGE_INFO *p_piece_range_info = NULL;
	RANGE_DATA_BUFFER_LIST range_data_buffer_list;
	_u32 file_index = 0;
	
	LOG_DEBUG( "bdm_try_to_check_piece.p_piece_info_list:0x%x",
		p_piece_info_list );
	
	buffer_list_init( &range_data_buffer_list );
	
	while( cur_node != LIST_END( *p_piece_info_list ) )
	{
		p_piece_range_info = ( PIECE_RANGE_INFO * )LIST_VALUE( cur_node );
		file_index = p_piece_range_info->_file_index;
		LOG_DEBUG( "bdm_try_to_check_piece.piece_index:%u, file_index:%u, padding_range:[%u,%u], padding_exacting_range:[%llu, %llu]",
			p_piece_range_info->_piece_index, p_piece_range_info->_file_index,
			p_piece_range_info->_padding_range._index, p_piece_range_info->_padding_range._num,
			p_piece_range_info->_padding_exact_range._pos, p_piece_range_info->_padding_exact_range._length );
		if( !bfm_range_is_cached( &p_bt_data_manager->_bt_file_manager, file_index, &p_piece_range_info->_padding_range ) )
		{
			cur_node = LIST_NEXT( cur_node );
			continue;
		}
		
		ret_val = bdm_get_cache_data_buffer( p_bt_data_manager, file_index, &p_piece_range_info->_padding_range, &range_data_buffer_list );
		CHECK_VALUE( ret_val );
		
		if( buffer_list_size( &range_data_buffer_list ) != 0 )
		{
			ret_val = bdm_handle_cache_piece_hash( p_bt_data_manager, p_piece_range_info, &range_data_buffer_list );
			sd_assert( ret_val == SUCCESS );
			
			list_clear( &range_data_buffer_list._data_list );		
		}		

		cur_node = LIST_NEXT( cur_node );
	}

	return SUCCESS;
}



_int32 bdm_handle_cache_piece_hash( BT_DATA_MANAGER *p_bt_data_manager, PIECE_RANGE_INFO *p_piece_range_info, RANGE_DATA_BUFFER_LIST *p_ret_range_buffer )
{
	_int32 ret_val = SUCCESS;
	_u8 piece_hash[20] = {0};
	LOG_DEBUG( " bdm_handle_cache_piece_hash");
	ret_val = bpc_check_buffer_piece( &p_bt_data_manager->_bt_piece_check, &p_piece_range_info->_padding_exact_range, p_ret_range_buffer, piece_hash );
	sd_assert( ret_val == SUCCESS );

	ret_val = bpc_verify_piece_hash( &p_bt_data_manager->_bt_piece_check, p_piece_range_info->_piece_index, piece_hash );	
	
	if( ret_val == SUCCESS )
	{
		bpc_set_piece_ok( &p_bt_data_manager->_bt_piece_check, p_piece_range_info->_piece_index, TRUE );
		bdm_notify_piece_check_finished( p_bt_data_manager, p_piece_range_info->_piece_index, TRUE );
	}
	else if( ret_val == BT_PIECE_CHECK_FAILED )
	{
		bpc_set_piece_ok( &p_bt_data_manager->_bt_piece_check, p_piece_range_info->_piece_index, FALSE );
		bdm_notify_piece_check_finished( p_bt_data_manager, p_piece_range_info->_piece_index, FALSE );
	}
	else 
	{
		bpc_set_piece_ok( &p_bt_data_manager->_bt_piece_check, p_piece_range_info->_piece_index, FALSE );
		bdm_notify_download_failed( p_bt_data_manager, ret_val );
	}	
	return SUCCESS;
}
*/
_int32 bdm_get_cache_data_buffer( BT_DATA_MANAGER *p_bt_data_manager, _u32 file_index, const RANGE *p_sub_file_range, RANGE_DATA_BUFFER_LIST *p_ret_range_buffer )
{
	LOG_DEBUG( "bdm_get_cache_data_buffer.file_index:%u, range:[%u,%u], p_ret_range_buffer:0x%x",
		file_index, p_sub_file_range->_index, p_sub_file_range->_num, p_ret_range_buffer );
	return bfm_get_sub_file_cache_data( &p_bt_data_manager->_bt_file_manager, file_index,
		p_sub_file_range, p_ret_range_buffer );
}

BOOL bdm_is_range_check_ok( BT_DATA_MANAGER *p_bt_data_manager, _u32 file_index, RANGE *p_sub_file_range )
{
	
	LOG_DEBUG( "bdm_is_range_check_ok." );
	return bfm_range_is_check( &p_bt_data_manager->_bt_file_manager, 
		file_index, p_sub_file_range  );
}

BOOL bdm_is_range_can_read( BT_DATA_MANAGER *p_bt_data_manager, _u32 file_index, RANGE *p_sub_file_range )
{
	
	LOG_DEBUG( "bdm_is_range_check_ok." );
	return bfm_range_is_can_read( &p_bt_data_manager->_bt_file_manager, 
		file_index, p_sub_file_range  );
}


/*
_int32 bdm_start_check_piece( BT_DATA_MANAGER *p_bt_data_manager )
{
	_int32 ret_val = SUCCESS;
	
	LOG_DEBUG( "bdm_start_check_piece." );

	
	bdm_try_to_add_check_piece( p_bt_data_manager );
	if( !bpc_has_piece_need_to_check( &p_bt_data_manager->_bt_piece_check ) ) return SUCCESS;
	if( bpc_has_piece_checking( &p_bt_data_manager->_bt_piece_check ) ) return SUCCESS;

	ret_val = bpc_start_check_next_piece( &p_bt_data_manager->_bt_piece_check );
	if( ret_val != SUCCESS )
	{
		bdm_notify_download_failed( p_bt_data_manager, ret_val );
	}
	
	return SUCCESS;
}

_int32 bdm_try_to_add_check_piece( BT_DATA_MANAGER *p_bt_data_manager )
{
	_u32 piece_index = 0;
	_u32 piece_num = 0;
	_int32 ret_val = SUCCESS;
	
	LOG_DEBUG( "bdm_try_to_add_check_piece." );
	if( bpc_has_piece_need_to_check( &p_bt_data_manager->_bt_piece_check ) ) return SUCCESS;
	
	piece_num = bpc_get_piece_num( &p_bt_data_manager->_bt_piece_check );
	
	for( ; piece_index < piece_num; piece_index++ )
	{
		if( bpc_is_piece_resv( &p_bt_data_manager->_bt_piece_check, piece_index )
			&& bpc_is_piece_need_check( &p_bt_data_manager->_bt_piece_check, piece_index )
			&& !bpc_is_piece_ok( &p_bt_data_manager->_bt_piece_check, piece_index )
			&& !bpc_has_piece_checking( &p_bt_data_manager->_bt_piece_check )
			&& bdm_bt_piece_is_write( p_bt_data_manager, piece_index ) )
		{
			ret_val = bpc_add_check_piece( &p_bt_data_manager->_bt_piece_check, piece_index );
			sd_assert( ret_val == SUCCESS );
		}
	}
	return SUCCESS;
}


_int32 bdm_notify_piece_check_finished( BT_DATA_MANAGER *p_bt_data_manager, _u32 piece_index, BOOL is_success )
{
	_int32 ret_val = SUCCESS;
	BT_RANGE bt_range;
	LIST read_range_info_list;
	LIST_ITERATOR cur_node = NULL;
	READ_RANGE_INFO *p_read_range_info = NULL;
	
	LOG_DEBUG( " bdm_notify_piece_check_finished, piece_index:%u", piece_index );
	ret_val = bpc_get_bt_range( &p_bt_data_manager->_bt_piece_check, piece_index, &bt_range );
	sd_assert( ret_val == SUCCESS );

	list_init( &read_range_info_list );
	
	ret_val = brs_bt_range_to_read_range_info_list( &p_bt_data_manager->_bt_range_switch, &bt_range, &read_range_info_list );
	sd_assert( ret_val == SUCCESS );
	bpc_set_piece_ok( &p_bt_data_manager->_bt_piece_check, piece_index, TRUE );

	cur_node = LIST_BEGIN( read_range_info_list );
	while( cur_node != LIST_END( read_range_info_list ) )
	{
		p_read_range_info = ( READ_RANGE_INFO * )LIST_VALUE( cur_node );
		bdm_notify_range_check_finished( p_bt_data_manager, p_read_range_info->_file_index, &p_read_range_info->_padding_range, is_success );
		if( is_success )
		{
			bdm_check_if_download_success( p_bt_data_manager, p_read_range_info->_file_index );
		}		
		cur_node = LIST_NEXT( cur_node );
	}	

	brs_release_read_range_info_list( &read_range_info_list );

	
	ret_val = bdm_start_check_piece( p_bt_data_manager );
	if( ret_val != SUCCESS )
	{
		bdm_notify_download_failed( p_bt_data_manager, ret_val );
	}

	return SUCCESS;
}
*/

void bdm_notify_sub_file_range_check_finished( BT_DATA_MANAGER *p_bt_data_manager, _u32 file_index, RANGE *p_range, BOOL is_success )
{
	_int32 ret_val = SUCCESS;
	RANGE padding_range;
#ifdef UPLOAD_ENABLE
	_u8 gcid[CID_SIZE]={0};
#endif
	
	LOG_DEBUG( " bdm_notify_sub_file_range_check_finished, file_index:%d,  block_range :(%u,%u) , is_success:%d",
		file_index, p_range->_index, p_range->_num, is_success );

	ret_val = brs_file_range_to_padding_range( &p_bt_data_manager->_bt_range_switch, file_index, p_range, &padding_range );
	sd_assert( ret_val == SUCCESS );
	
	bdm_notify_padding_range_check_finished( p_bt_data_manager, &padding_range, is_success, FALSE );

#ifdef UPLOAD_ENABLE
	if(bfm_get_shub_gcid( &p_bt_data_manager->_bt_file_manager, file_index, gcid ))
	{
		ulm_notify_have_block( gcid);
	}
	//dt_notify_have_block( p_bt_data_manager->_task_ptr, file_index );
#endif

}

void bdm_checker_notify_check_finished( BT_DATA_MANAGER *p_bt_data_manager, _u32 piece_index, RANGE *p_padding_range, BOOL is_success )
{
    
#ifdef ENABLE_BT_PROTOCOL
	_int32 ret_val = SUCCESS;
#endif
	BOOL is_piece_ok = FALSE;
	LOG_DEBUG( " bdm_checker_notify_check_finished, piece_index:%u, block_range :(%u,%u) , is_success:%d",
		piece_index, p_padding_range->_index, p_padding_range->_num, is_success );

	is_piece_ok = bitmap_at( &p_bt_data_manager->_piece_bitmap, piece_index );
	bitmap_set( &p_bt_data_manager->_piece_bitmap, piece_index, is_success );
	bitmap_print(&p_bt_data_manager->_piece_bitmap);
	bdm_notify_padding_range_check_finished( p_bt_data_manager, p_padding_range, is_success, TRUE );
	if( is_success && !is_piece_ok )
	{
      
#ifdef ENABLE_BT_PROTOCOL
		ret_val = bt_notify_have_piece( p_bt_data_manager->_task_ptr, piece_index );
		sd_assert( ret_val == SUCCESS );	
#endif
        
	}
}

void bdm_notify_padding_range_check_finished( BT_DATA_MANAGER *p_bt_data_manager, RANGE *p_padding_range, BOOL is_success, BOOL is_piece_check )
{
	_int32 ret_val = SUCCESS;
	
	LOG_DEBUG( " bdm_notify_padding_range_check_finished, block_range :(%u,%u) , is_success:%d",
		p_padding_range->_index, p_padding_range->_num, is_success );
		
	if( is_success )
	{
		bdm_check_range_success( p_bt_data_manager, p_padding_range );
	}
	else
	{
		ret_val = bdm_check_range_failure( p_bt_data_manager, p_padding_range, is_piece_check );		  
		sd_assert( ret_val == SUCCESS );
	}
}

void bdm_check_range_success( BT_DATA_MANAGER *p_bt_data_manager, RANGE *p_range )
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG("bdm_check_range_success, block_range :(%u,%u) check success .", p_range->_index, p_range->_num );		  
	   
    ret_val = correct_manager_erase_error_block( &p_bt_data_manager->_correct_manager, p_range ); 
	sd_assert( ret_val == SUCCESS );

	range_manager_erase_range( &p_bt_data_manager->_range_manager, p_range, NULL );
	sd_assert( ret_val == SUCCESS );

	//bdm_handle_range_related_piece( p_bt_data_manager, p_range );
}


_int32 bdm_check_range_failure( BT_DATA_MANAGER *p_bt_data_manager, RANGE *p_range, BOOL is_piece_check )
{
    LIST res_list;
    _int32 ret_val = SUCCESS;  

     RESOURCE * r_res = NULL;
     LIST_ITERATOR it_res = NULL;	
	
    _int32 correct_manager_ret_val = SUCCESS;  
	LIST_ITERATOR cur_node = NULL;
	LIST padding_range_file_index_list;
	SUB_FILE_PADDING_RANGE_INFO *p_range_info = NULL;
	RANGE sub_file_range;
	
#ifdef EMBED_REPORT
	_u32 file_index = 0;
#endif

	LOG_DEBUG("bdm_check_range_failure, range :(%u, %u) check failure .", p_range->_index, p_range->_num);			
		
    list_init(&res_list);
    list_init(&padding_range_file_index_list);

	get_res_from_range( &p_bt_data_manager->_range_manager, p_range, &res_list );		  

	/*
	0   no error
	-1  peer/server  correct
	-2  one resource  correct 
	-3  origin correct
	-4  can not correct  
	*/						

      out_put_res_list(&res_list);   

	if(list_size(&res_list) == 1)
	{
	       it_res = LIST_BEGIN(res_list);
	       r_res = (RESOURCE*) LIST_VALUE(it_res);
		   
#ifdef EMBED_REPORT
			if( !is_piece_check )
			{
				ret_val = brs_padding_range_to_sub_file_range( &p_bt_data_manager->_bt_range_switch, p_range, &file_index, &sub_file_range );
				if( ret_val == SUCCESS )
				{
					bfm_handle_err_data_report( &p_bt_data_manager->_bt_file_manager, file_index, r_res, sub_file_range._num * get_data_unit_size());
				}
			}
#endif

		if(range_is_all_from_res(&p_bt_data_manager->_range_manager, r_res, p_range) == FALSE)
		{
		       LOG_DEBUG("bdm_check_range_failure range_is_all_from_res,  res:0x%x , range (%u,%u) ret FALSE.", r_res, p_range->_index, p_range->_num); 
 
		       correct_manager_clear_res_list(&res_list );	 
		}
	}
 	
	
	correct_manager_ret_val = correct_manager_add_error_block( &p_bt_data_manager->_correct_manager, p_range, &res_list );		 
	LOG_DEBUG("bdm_check_range_failure, range :(%u, %u) correct_manager_ret_val:%d .", 
		p_range->_index, p_range->_num, correct_manager_ret_val );			

	sd_assert(correct_manager_ret_val != -3 ); 
	if( correct_manager_ret_val == -3 )
	{
		bdm_notify_download_failed( p_bt_data_manager, BT_DATA_MANAGER_LOGIC_ERROR );
		return SUCCESS;
	}

	ret_val = brs_get_padding_range_file_index_list( &p_bt_data_manager->_bt_range_switch
	        , p_range
	        , &padding_range_file_index_list );
	sd_assert( ret_val == SUCCESS );
	
	cur_node = LIST_BEGIN( padding_range_file_index_list );
	while( cur_node != LIST_END( padding_range_file_index_list ) )
	{
		p_range_info = ( SUB_FILE_PADDING_RANGE_INFO * )LIST_VALUE( cur_node );
		if( bfm_bcid_is_valid( &p_bt_data_manager->_bt_file_manager, p_range_info->_file_index )
			&& is_piece_check )
		{
			cur_node = LIST_NEXT( cur_node );
			continue;
		}
		
		ret_val = brs_padding_range_to_file_range( &p_bt_data_manager->_bt_range_switch, p_range_info->_file_index,
			&p_range_info->_padding_range, &sub_file_range );
		sd_assert( ret_val == SUCCESS );
		if( ret_val != SUCCESS )
		{
			cur_node = LIST_NEXT( cur_node );
			continue;
		}
		
		if( is_piece_check )
		{
			bfm_notify_delete_range_data( &p_bt_data_manager->_bt_file_manager, p_range_info->_file_index, &sub_file_range );
		}
		ret_val = brdi_del_recved_range( &p_bt_data_manager->_range_download_info, &p_range_info->_padding_range );
		sd_assert( ret_val == SUCCESS );	
		
		if( correct_manager_ret_val == -4 )
		{
			if( !bfm_add_no_need_check_range( &p_bt_data_manager->_bt_file_manager, p_range_info->_file_index, &sub_file_range) )
			{
				bfm_set_bt_sub_file_err_code( &p_bt_data_manager->_bt_file_manager, p_range_info->_file_index, BT_DATA_MANAGER_CANNOT_CORRECT);
			}
		}
		cur_node = LIST_NEXT( cur_node );
	}
	
	brs_release_padding_range_file_index_list( &padding_range_file_index_list );

	correct_manager_clear_res_list( &res_list );
	
	range_manager_erase_range( &p_bt_data_manager->_range_manager, p_range, NULL );

	//bt出现99%下不完问题，校验失败的块没加到brdi，导致得不到未完成块，p_bt_file_manager->_cur_range_step增加，减去之前的下载范围
	//ret_val = brdi_add_cur_need_download_range_list( &p_bt_data_manager->_range_download_info, p_range );
	//sd_assert( ret_val == SUCCESS );

	return SUCCESS;
}


_int32 bdm_read_data( BT_DATA_MANAGER *p_bt_data_manager, _u32 file_index, RANGE_DATA_BUFFER *p_range_data_buffer, notify_read_result p_call_back, void *p_user )
{
	_int32 ret_val = SUCCESS;
	//RANGE padding_range;
	LOG_DEBUG( " bdm_read_data " );
	ret_val =  bfm_read_data_buffer( &p_bt_data_manager->_bt_file_manager, file_index, 
		p_range_data_buffer, p_call_back, p_user );	

	/*
	if( ret_val != SUCCESS )
	{
		brs_file_range_to_padding_range( &p_bt_data_manager->_bt_range_switch, file_index,
			&p_range_data_buffer->_range, &padding_range );	
		if( bt_is_tmp_file_range( &p_bt_data_manager->_bt_tmp_file, &padding_range ) 
			&& bt_is_tmp_range_valid( &p_bt_data_manager->_bt_tmp_file, &padding_range ) )
		{
			ret_val = bt_read_tmp_file( &p_bt_data_manager->_bt_tmp_file, &padding_range, p_range_data_buffer, p_call_back, p_user );
		}
	}
	*/
	return ret_val;

}

_int32 bdm_check_if_download_success( BT_DATA_MANAGER *p_bt_data_manager, _u32 file_index )
{
	_int32 ret_val = SUCCESS;
	BOOL is_success = TRUE;
	//RANGE padding_range;
	_u32 start_piece = 0;
	_u32 end_piece = 0;
	//_u32 piece_index= 0;
	
	LOG_DEBUG( " bdm_check_if_download_success:file_index:%u ", file_index );

	/*
	ret_val = brs_get_padding_range_from_file_index( &p_bt_data_manager->_bt_range_switch, file_index, & padding_range );
	sd_assert( ret_val == SUCCESS );
	*/	
	
	ret_val = brs_get_piece_range_from_file_index( &p_bt_data_manager->_bt_range_switch, file_index, &start_piece, &end_piece );
	sd_assert( ret_val == SUCCESS );

	/*
	for( piece_index = start_piece; piece_index <= end_piece; piece_index++ )
	{
		is_success = bitmap_at( &p_bt_data_manager->_piece_bitmap, piece_index );

		if( !is_success ) break;
	}
	*/

	bitmap_print(&p_bt_data_manager->_piece_bitmap);
	
	is_success = bitmap_range_is_all_set( &p_bt_data_manager->_piece_bitmap, start_piece, end_piece );

	LOG_DEBUG("bdm_check_if_download_success: file_index:%u, start_piece:%u, end_piece:%u, is_success:%d", 
		file_index, start_piece, end_piece, is_success );

	if( is_success )
	{
		bfm_set_bt_sub_file_err_code( &p_bt_data_manager->_bt_file_manager, file_index, SUCCESS );
		return SUCCESS;
	}

	return SUCCESS;

}

/*
BOOL bdm_check_if_file_can_close( BT_DATA_MANAGER *p_bt_data_manager, _u32 file_index )
{
	_u32 start_piece = 0;
	_u32 end_piece = 0;
	_int32 ret_val = SUCCESS;
	BOOL can_close = TRUE;
	_u32 piece_index = 0;

	ret_val = brs_get_piece_range_from_file_index( &p_bt_data_manager->_bt_range_switch, file_index, &start_piece, &end_piece );
	sd_assert( ret_val == SUCCESS );
	
	for( piece_index = start_piece; piece_index <= end_piece; piece_index++ )
	{
		if( bpc_is_piece_need_check( &p_bt_data_manager->_bt_piece_check, piece_index ) )
		{
			if( !bpc_is_piece_ok( &p_bt_data_manager->_bt_piece_check, piece_index ) )
			{
				can_close = FALSE;
			}
		}
	}
    LOG_DEBUG("bdm_check_if_file_can_close: file_index:%u, can_close:%u", 
		file_index, can_close );	

	return can_close;

}
*/

void bdm_notify_download_failed( BT_DATA_MANAGER *p_bt_data_manager, _int32 err_code )
{
	LOG_DEBUG( "bdm_notify_download_failed:err:%d", err_code );
	p_bt_data_manager->_data_status_code = err_code;
}

void bdm_notify_task_download_success( BT_DATA_MANAGER *p_bt_data_manager )
{
	LOG_DEBUG( "bdm_notify_task_download_success" );
	p_bt_data_manager->_data_status_code = DATA_SUCCESS;	
}

void bdm_notify_del_tmp_file( BT_DATA_MANAGER *p_bt_data_manager )
{
	LOG_DEBUG( "bdm_notify_del_tmp_file" );
	bt_checker_del_tmp_file( &p_bt_data_manager->_bt_piece_check );
}

/*
BOOL bdm_bt_piece_is_write( BT_DATA_MANAGER *p_bt_data_manager, _u32 piece_index )
{
	_int32 ret_val = SUCCESS;
	BT_RANGE bt_range;
	LIST read_range_info_list;
	LIST_ITERATOR cur_node = NULL;
	READ_RANGE_INFO *p_read_range_info = NULL;
	BOOL is_write = TRUE;
	LOG_DEBUG( "bdm_bt_piece_is_write. piece_index:%d", piece_index );
	
	list_init( &read_range_info_list );
	
	ret_val = bpc_get_bt_range( &p_bt_data_manager->_bt_piece_check, piece_index, &bt_range );
	sd_assert( ret_val == SUCCESS );
	
	ret_val = brs_bt_range_to_read_range_info_list( &p_bt_data_manager->_bt_range_switch, &bt_range, &read_range_info_list );
	sd_assert( ret_val == SUCCESS );

	cur_node = LIST_BEGIN( read_range_info_list );
	while( cur_node != LIST_END( read_range_info_list ) )
	{
		p_read_range_info = ( READ_RANGE_INFO * )LIST_VALUE( cur_node );
		LOG_DEBUG( "bdm_bt_piece_is_write. piece_index:%d, file_index:%d,padding_range:[%u, %u]", 
			piece_index, p_read_range_info->_file_index , p_read_range_info->_padding_range._index, p_read_range_info->_padding_range._num );

		is_write = bdm_range_is_write_in_disk( p_bt_data_manager, p_read_range_info->_file_index, &p_read_range_info->_padding_range, p_read_range_info->_is_tmp_file );
		if( !is_write ) break;
		cur_node = LIST_NEXT( cur_node );
	}
	brs_release_read_range_info_list( &read_range_info_list );	
	LOG_DEBUG( "bdm_bt_piece_is_write. piece_index:%d, is_write:%d", piece_index, is_write );
	
    return is_write;
}
*/

void bdm_file_manager_notify_add_file_range( BT_DATA_MANAGER *p_bt_data_manager, _u32 file_index, RANGE *p_sub_range )
{
	RANGE padding_range;
	_int32 ret_val = SUCCESS;

	LOG_DEBUG( "bdm_file_manager_notify_add_file_range. file_index:%d, sub_range:[%u,%u]", 
		file_index, p_sub_range->_index, p_sub_range->_num );
	
	ret_val = brs_file_range_to_padding_range( &p_bt_data_manager->_bt_range_switch, file_index, p_sub_range, &padding_range );
	sd_assert( ret_val == SUCCESS );

	ret_val = brdi_add_cur_need_download_range_list( &p_bt_data_manager->_range_download_info, &padding_range );
	sd_assert( ret_val == SUCCESS );
	
	/*-----------------------------------*/
#ifdef ENABLE_BT_PROTOCOL
	bt_update_pipe_can_download_range(p_bt_data_manager->_task_ptr);
#endif
}

void bdm_file_manager_notify_start_file( BT_DATA_MANAGER *p_bt_data_manager, _u32 file_index, RANGE_LIST *p_sub_file_recv_range_list, BOOL has_bcid )
{
	RANGE padding_range;
	RANGE resv_padding_range;
	_int32 ret_val = SUCCESS;
	RANGE_LIST_ITEROATOR cur_range_node = NULL;
	RANGE_LIST tmp_valid_list;
	RANGE_LIST overlap_list;
	RANGE_LIST file_range_list;

	LOG_DEBUG( "bdm_file_manager_notify_start_file. file_index:%d, has_bcid:%d", file_index, has_bcid );

	ret_val = brs_get_padding_range_from_file_index( &p_bt_data_manager->_bt_range_switch, file_index, &padding_range );
	sd_assert( ret_val == SUCCESS );

	//ret_val = brdi_add_cur_need_download_range_list( &p_bt_data_manager->_range_download_info, &padding_range );
	//sd_assert( ret_val == SUCCESS );

	ret_val = bt_checker_add_need_check_file( &p_bt_data_manager->_bt_piece_check, file_index );
	sd_assert( ret_val == SUCCESS );

	range_list_get_head_node( p_sub_file_recv_range_list, &cur_range_node );
	while( cur_range_node != NULL )
	{	
		resv_padding_range._index = 0;
		resv_padding_range._num = 0;
		ret_val = brs_file_range_to_padding_range( &p_bt_data_manager->_bt_range_switch, file_index, &cur_range_node->_range, &resv_padding_range );
		sd_assert( ret_val == SUCCESS );	

		ret_val = brdi_add_recved_range( &p_bt_data_manager->_range_download_info, &resv_padding_range );
		sd_assert( ret_val == SUCCESS );	

		ret_val = bt_checker_recv_range( &p_bt_data_manager->_bt_piece_check, &resv_padding_range );
		sd_assert( ret_val == SUCCESS );		
		
		range_list_get_next_node( p_sub_file_recv_range_list, cur_range_node, &cur_range_node );
	}

	range_list_init( &tmp_valid_list );
	range_list_init( &overlap_list );
	range_list_init( &file_range_list );
	
	bt_checker_get_tmp_file_valid_range( &p_bt_data_manager->_bt_piece_check, &tmp_valid_list );
	range_list_add_range( &file_range_list, &padding_range, NULL, NULL );
	LOG_DEBUG( "tmp_valid_list:" );
	out_put_range_list( &tmp_valid_list );

	get_range_list_overlap_list( &tmp_valid_list, &file_range_list, &overlap_list );
	LOG_DEBUG( "overlap_list:" );
	out_put_range_list( &overlap_list );

	range_list_add_range_list( &p_bt_data_manager->_need_read_tmp_range_list, &overlap_list );

	range_list_get_head_node( &overlap_list, &cur_range_node );
	while( cur_range_node != NULL )
	{	
		ret_val = brdi_add_recved_range( &p_bt_data_manager->_range_download_info, &cur_range_node->_range );
		sd_assert( ret_val == SUCCESS );	

		range_list_get_next_node( &overlap_list, cur_range_node, &cur_range_node );
	}

	range_list_clear( &tmp_valid_list );
	range_list_clear( &overlap_list );
	range_list_clear( &file_range_list );
	bdm_start_read_tmp_file_range( p_bt_data_manager );
	
}

void bdm_file_manager_notify_delete_file_range( BT_DATA_MANAGER *p_bt_data_manager, _u32 file_index, RANGE *p_sub_range )
{
	RANGE padding_range;
	_int32 ret_val = SUCCESS;

	LOG_DEBUG( "bdm_file_manager_notify_delete_file_range. file_index:%d, sub_range:[%u,%u]", 
		file_index, p_sub_range->_index, p_sub_range->_num );
	sd_assert( brdi_is_all_resv(&p_bt_data_manager->_range_download_info) );
	
	ret_val = brs_file_range_to_padding_range( &p_bt_data_manager->_bt_range_switch, file_index, p_sub_range, &padding_range );
	sd_assert( ret_val == SUCCESS );
	
	ret_val = brdi_del_cur_need_download_range_list( &p_bt_data_manager->_range_download_info, &padding_range );
	sd_assert( ret_val == SUCCESS );
	
	//ret_val = brdi_del_recved_range( &p_bt_data_manager->_range_download_info, &padding_range );
	//sd_assert( ret_val == SUCCESS );
}

void bdm_file_manager_delete_error_block(BT_DATA_MANAGER *p_bt_data_manager
    , RANGE* padding_range)
{
    LIST_ITERATOR it=LIST_BEGIN(p_bt_data_manager->_correct_manager._error_ranages._error_block_list);
    LIST_ITERATOR it_end = LIST_END(p_bt_data_manager->_correct_manager._error_ranages._error_block_list);
    LIST_ITERATOR tmp_it = NULL;
    while(it != it_end)
    {
        ERROR_BLOCK* err_block_ptr = (ERROR_BLOCK*)LIST_VALUE(it);
        if (err_block_ptr->_r._index >= padding_range->_index 
            && err_block_ptr->_r._index + err_block_ptr->_r._num <= padding_range->_index + padding_range->_num )
        {
            tmp_it = it;
            it = LIST_NEXT(it);
            list_erase(&p_bt_data_manager->_correct_manager._error_ranages._error_block_list, tmp_it);
        }
        else
        {
            it = LIST_NEXT(it);
        }
    }
}

void bdm_file_manager_notify_stop_file( BT_DATA_MANAGER *p_bt_data_manager, _u32 file_index, BOOL is_ever_vod_file )
{
	RANGE padding_range;
	_int32 ret_val = SUCCESS;
#ifdef EMBED_REPORT
	VOD_REPORT_PARA vod_para;
#endif

	LOG_DEBUG( "bdm_file_manager_notify_stop_file. file_index:%d", file_index );
	ret_val = brs_get_padding_range_from_file_index( &p_bt_data_manager->_bt_range_switch, file_index, &padding_range );
	sd_assert( ret_val == SUCCESS );
	
	brdi_adjust_resv_range_list( &p_bt_data_manager->_range_download_info );

	ret_val = brdi_del_cur_need_download_range_list( &p_bt_data_manager->_range_download_info, &padding_range );
	sd_assert( ret_val == SUCCESS );
	
	ret_val = brdi_del_recved_range( &p_bt_data_manager->_range_download_info, &padding_range );
	sd_assert( ret_val == SUCCESS );

	bdm_erase_range_related_piece( p_bt_data_manager, file_index );

	bt_checker_erase_need_check_range( &p_bt_data_manager->_bt_piece_check, &padding_range );

    bdm_file_manager_delete_error_block(p_bt_data_manager, &padding_range);


    
	if( !is_ever_vod_file )
	{
#ifdef EMBED_REPORT
		sd_memset( &vod_para, 0, sizeof( VOD_REPORT_PARA ) );
		emb_reporter_bt_stop_file( p_bt_data_manager->_task_ptr, file_index, &vod_para, FALSE );
#endif
        
	}
	//bpc_stop_download_file( &p_bt_data_manager->_bt_piece_check, file_index );
}

void bdm_notify_task_stop_success( BT_DATA_MANAGER *p_bt_data_manager )
{
	_int32 ret_val = SUCCESS;
	ret_val = bprm_uninit_list( &p_bt_data_manager->_bt_pipe_reader_mgr_list);
	sd_assert( ret_val == SUCCESS );

	bt_notify_file_closing_result_event( p_bt_data_manager->_task_ptr );	
}

void bdm_notify_report_task_success( BT_DATA_MANAGER *p_bt_data_manager )
{
	LOG_DEBUG("bdm_notify_report_task_success." );	
	bt_notify_report_task_success( p_bt_data_manager->_task_ptr );	
}


BOOL bdm_range_is_write_in_disk( BT_DATA_MANAGER *p_bt_data_manager, _u32 file_index, RANGE *p_range )
{
	BOOL ret_bool = TRUE;
	//RANGE padding_range;
	
	LOG_DEBUG("bdm_range_is_write_in_disk:file_index;%d, range:[%u,%u].",
		file_index, p_range->_index, p_range->_num );
	
	ret_bool = bfm_range_is_write( &p_bt_data_manager->_bt_file_manager, file_index, p_range );

/*
	if( !ret_bool && is_tmp_file )
	{
		brs_file_range_to_padding_range( &p_bt_data_manager->_bt_range_switch, file_index,
			p_range, &padding_range );	
		ret_bool = bt_is_tmp_range_valid( &p_bt_data_manager->_bt_tmp_file, &padding_range );
	}
	*/
	LOG_DEBUG("bdm_range_is_write_in_disk:%d.", ret_bool);
	return ret_bool;
}

/*
void bdm_handle_range_related_piece( BT_DATA_MANAGER *p_bt_data_manager, const RANGE *p_padding_range )
{
	_int32 ret_val = SUCCESS;
	_u32 first_piece = 0;
	_u32 last_piece = 0;
	_u32 piece_index = 0;
	RANGE piece_padding_range;
	LIST range_info_list;
	LIST_ITERATOR cur_node = NULL;
	BOOL is_bcid_check_ok = TRUE;
	READ_RANGE_INFO *p_read_range_info = NULL;
	
	LOG_DEBUG("bdm_handle_range_related_piece: range:[%u,%u]",
		p_padding_range->_index, p_padding_range->_num );
	
	piece_padding_range._index=0;
	piece_padding_range._num=0;
	
	ret_val = brs_range_to_extra_piece( &p_bt_data_manager->_bt_range_switch, p_padding_range, &first_piece, &last_piece );
	sd_assert( ret_val == SUCCESS );

	list_init( &range_info_list );
	for( piece_index = first_piece; piece_index <= last_piece; piece_index++ )
	{
		is_bcid_check_ok = TRUE;
		ret_val = brs_piece_to_range_info_list( &p_bt_data_manager->_bt_range_switch, piece_index, &piece_padding_range, &range_info_list );
		sd_assert( ret_val == SUCCESS );
		LOG_DEBUG("***bdm_handle_range_related_piece: piece_index:%u, piece_padding_range:[%u, %u]", 
			piece_index, piece_padding_range._index, piece_padding_range._num );		
		if( bt_checker_is_in_check_range( &p_bt_data_manager->_bt_piece_check, &piece_padding_range ) )
		{
			LOG_DEBUG("!!!bdm_handle_range_related_piece: piece_padding_range in checker range");		
			continue;
		}
		cur_node = LIST_BEGIN( range_info_list );
		while( cur_node != LIST_END( range_info_list ) )
		{
			p_read_range_info = ( READ_RANGE_INFO *)LIST_VALUE( cur_node );
			if( !bfm_range_is_check( &p_bt_data_manager->_bt_file_manager, p_read_range_info->_file_index, &p_read_range_info->_padding_range ) )
			{
				is_bcid_check_ok = FALSE;
				break;
			}
			cur_node = LIST_NEXT( cur_node );
		}
		if( is_bcid_check_ok )
		{
			LOG_DEBUG("***bdm_handle_range_related_piece: set_piece_ok: piece_index:%u", piece_index );		
			bitmap_set( &p_bt_data_manager->_piece_bitmap, piece_index, TRUE );
			ret_val = bt_notify_have_piece( p_bt_data_manager->_task_ptr, piece_index );
			sd_assert( ret_val == SUCCESS );
		}
		brs_release_read_range_info_list( &range_info_list );
	}
	
}
*/

void bdm_erase_range_related_piece( BT_DATA_MANAGER *p_bt_data_manager, _u32 file_index )
{
	_int32 ret_val = SUCCESS;
	_u32 first_piece = 0;
	_u32 last_piece = 0;
	_u32 piece_index = 0;
	
	LOG_DEBUG("bdm_erase_range_related_piece: file_index:%u", file_index );

	ret_val = brs_get_inner_piece_range_from_file_index( &p_bt_data_manager->_bt_range_switch, file_index, &first_piece, &last_piece );
	if( ret_val != SUCCESS )
	{
		return;
	}

	for( piece_index = first_piece; piece_index <= last_piece; piece_index++ )
	{
		bitmap_set( &p_bt_data_manager->_piece_bitmap, piece_index, FALSE );
	}
	bitmap_print(&p_bt_data_manager->_piece_bitmap);
}

/*
void bdm_check_if_range_related_piece_received( BT_DATA_MANAGER *p_bt_data_manager, const RANGE *p_padding_range )
{
	_int32 ret_val = SUCCESS;
	_u32 first_piece = 0;
	_u32 last_piece = 0;
	_u32 piece_index = 0;
	
	LOG_DEBUG("bdm_check_if_range_related_piece_received: range:[%u,%u]",
		p_padding_range->_index, p_padding_range->_num );

	ret_val = brs_range_to_extra_piece( &p_bt_data_manager->_bt_range_switch, p_padding_range, &first_piece, &last_piece );
	sd_assert( ret_val == SUCCESS );

	for( piece_index = first_piece; piece_index <= last_piece; piece_index++ )
	{
		if( bdm_bt_piece_is_resv( p_bt_data_manager, piece_index ) )

		{
			bpc_set_piece_resv( &p_bt_data_manager->_bt_piece_check, piece_index, TRUE );
		}
	}
}



void bdm_set_range_related_piece_resv( BT_DATA_MANAGER *p_bt_data_manager, const RANGE *p_padding_range )
{
	_int32 ret_val = SUCCESS;
	_u32 first_piece = 0;
	_u32 last_piece = 0;
	_u32 piece_index = 0;
	
	LOG_DEBUG("bdm_check_if_range_related_piece_received: range:[%u,%u]",
		p_padding_range->_index, p_padding_range->_num );

	ret_val = brs_padding_range_to_bt_range( &p_bt_data_manager->_bt_range_switch, p_padding_range, &first_piece, &last_piece );
	sd_assert( ret_val == SUCCESS );

	for( piece_index = first_piece; piece_index <= last_piece; piece_index++ )
	{
		bpc_set_piece_resv( &p_bt_data_manager->_bt_piece_check, piece_index, TRUE );
	}
}


BOOL bdm_bt_piece_is_resv( BT_DATA_MANAGER *p_bt_data_manager, _u32 piece_index )
{
	_int32 ret_val = SUCCESS;
	BT_RANGE bt_range;
	RANGE padding_range;
	BOOL is_resv = FALSE;

	bt_range._pos = piece_index * bpc_get_piece_size( &p_bt_data_manager->_bt_piece_check );
	bt_range._length = bpc_get_target_piece_size( &p_bt_data_manager->_bt_piece_check, piece_index );
	LOG_DEBUG( "bdm_bt_piece_is_write. piece_index:%d, is_resv:%d", piece_index, is_resv );

	ret_val = brs_bt_range_to_extra_padding_range( &p_bt_data_manager->_bt_range_switch, &bt_range, &padding_range );
	if( ret_val != SUCCESS )
	{
		is_resv = FALSE;
	}
	else
	{
		is_resv = brdi_is_padding_range_resv( &p_bt_data_manager->_range_download_info, &padding_range );
	}

	LOG_DEBUG( "bdm_bt_piece_is_resv. piece_index:%d, is_resv:%d",
		piece_index, is_resv );
	return is_resv;
}


BOOL bdm_is_file_need_piece_check( BT_DATA_MANAGER *p_bt_data_manager, _u32 file_index )
{
	BT_SUB_FILE *p_bt_sub_file = NULL;
	_int32 ret_val = SUCCESS;
	_int32 ret_bool = FALSE;
	
	ret_val = bfm_get_bt_sub_file_ptr( &p_bt_data_manager->_bt_file_manager, file_index, &p_bt_sub_file );
	//sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS )
	{
		ret_bool = FALSE;
	}
	else
	{
		ret_bool = !p_bt_sub_file->_has_bcid;
	}
	LOG_DEBUG( "bdm_bt_piece_is_resv. file_index:%d, ret_bool:%d",
		file_index, ret_bool );

	return ret_bool;
	
}
*/

void bdm_query_hub_for_single_file( BT_DATA_MANAGER *p_bt_data_manager, _u32 file_index )
{
	LOG_DEBUG( "bdm_query_hub_for_single_file. file_index:%d", file_index );
	bt_query_hub_for_single_file( p_bt_data_manager->_task_ptr, file_index );
}

BOOL bdm_file_range_is_cached( BT_DATA_MANAGER *p_bt_data_manager, _u32 file_index, RANGE *p_sub_file_range )
{
	return bfm_range_is_cached( &p_bt_data_manager->_bt_file_manager, file_index, p_sub_file_range );
}

BOOL bdm_padding_range_is_resv( BT_DATA_MANAGER *p_bt_data_manager, RANGE *p_padding_range )
{
	return brdi_is_padding_range_resv( &p_bt_data_manager->_range_download_info, p_padding_range );
}

_u32 bdm_get_vod_file_index( BT_DATA_MANAGER *p_bt_data_manager )
{
	LOG_DEBUG( "bdm_get_vod_file_index:%d", p_bt_data_manager->_dap_file_index );
	return p_bt_data_manager->_dap_file_index;	
}

void bdm_start_read_tmp_file_range( BT_DATA_MANAGER *p_bt_data_manager )
{
	RANGE_LIST_ITEROATOR cur_range_node = NULL;
	_int32 ret_val = SUCCESS;
	RANGE_DATA_BUFFER *p_range_data_buffer = NULL;
	_u32 buffer_len = get_data_unit_size();
	
	LOG_DEBUG( "bdm_start_read_tmp_file_range:is_reading:%d",
		p_bt_data_manager->_is_reading_tmp_range );
	
	LOG_DEBUG( "need_read_tmp_range_list:" );	
	out_put_range_list( &p_bt_data_manager->_need_read_tmp_range_list );
	if( p_bt_data_manager->_is_reading_tmp_range ) return;
	if( range_list_size( &p_bt_data_manager->_need_read_tmp_range_list ) == 0 ) return;

	ret_val = alloc_range_data_buffer_node( &p_range_data_buffer );	 
	if( ret_val != SUCCESS )
	{
		LOG_ERROR( "alloc_range_data_buffer_node err." );
		return;
	}

	ret_val = dm_get_buffer_from_data_buffer( &buffer_len, (_int8**)&(p_range_data_buffer->_data_ptr) ); 			 
	if( ret_val != SUCCESS || buffer_len != get_data_unit_size() )
	{
		LOG_DEBUG( "sd_malloc data_buffer err." );
		free_range_data_buffer_node( p_range_data_buffer );		
		return;	   
	}
	range_list_get_head_node( &p_bt_data_manager->_need_read_tmp_range_list, &cur_range_node );

	p_range_data_buffer->_range._index = cur_range_node->_range._index;
	p_range_data_buffer->_range._num = MIN( 1, cur_range_node->_range._num );
	if( cur_range_node->_range._num == 0 )
	{
		sd_assert( FALSE );
		range_list_delete_range( &p_bt_data_manager->_need_read_tmp_range_list, &p_range_data_buffer->_range, NULL, NULL );
		dm_free_buffer_to_data_buffer( buffer_len, (_int8**)&(p_range_data_buffer->_data_ptr) );
		free_range_data_buffer_node( p_range_data_buffer );	
		return;	   		
	}
	p_range_data_buffer->_buffer_length = buffer_len;
	p_range_data_buffer->_data_length = buffer_len;
	
	LOG_DEBUG( "bdm_start_read_tmp_file_range:range:[%u,%u]",
		p_range_data_buffer->_range._index, p_range_data_buffer->_range._num );	
	
	ret_val = bt_checker_read_tmp_file( &p_bt_data_manager->_bt_piece_check
            , p_range_data_buffer
            , bdm_read_tmp_file_call_back
            , p_bt_data_manager );
	if( ret_val != SUCCESS )
	{
		LOG_DEBUG( "bt_checker_read_tmp_file err:%d", ret_val );
		dm_free_buffer_to_data_buffer( buffer_len, (_int8**)&(p_range_data_buffer->_data_ptr) );
		free_range_data_buffer_node( p_range_data_buffer );	
		return;	   
	}
	p_bt_data_manager->_is_reading_tmp_range = TRUE;
	
}

_int32 bdm_read_tmp_file_call_back(struct tagTMP_FILE *p_file_struct, void *p_user, RANGE_DATA_BUFFER *p_data_buffer, _int32 read_result, _u32 read_len )
{
	_int32 ret_val = SUCCESS;
	_u32 file_index = 0;
	RANGE file_range;
	BT_DATA_MANAGER *p_bt_data_manager = (BT_DATA_MANAGER *)p_user;
	//RANGE_LIST_ITEROATOR cur_range_node = NULL;

	LOG_DEBUG( "bdm_read_tmp_file_call_back:range:[%u,%u], ret:%d",
		p_data_buffer->_range._index, p_data_buffer->_range._num, ret_val );	
	
	p_bt_data_manager->_is_reading_tmp_range = FALSE;

	if( read_result != SUCCESS )
	{
		brdi_del_recved_range( &p_bt_data_manager->_range_download_info, &p_data_buffer->_range );
		dm_free_buffer_to_data_buffer( p_data_buffer->_buffer_length, (_int8**)&(p_data_buffer->_data_ptr) );

		free_range_data_buffer_node( p_data_buffer ); 

		return SUCCESS;
	}
	
	//range_list_get_head_node( &p_bt_data_manager->_need_read_tmp_range_list, &cur_range_node );
	//sd_assert( cur_range_node->_range._index == p_data_buffer->_range._index );
	
	//sd_assert( cur_range_node->_range._num >= p_data_buffer->_range._num );

	range_list_delete_range( &p_bt_data_manager->_need_read_tmp_range_list, &p_data_buffer->_range, NULL, NULL );
	
	ret_val = brs_padding_range_to_sub_file_range( &p_bt_data_manager->_bt_range_switch
        , &p_data_buffer->_range
        , &file_index
        , &file_range );
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS )
	{		
		LOG_ERROR( " brs_padding_range_to_sub_file_range err:%d", ret_val );
		dm_free_buffer_to_data_buffer( p_data_buffer->_buffer_length, &p_data_buffer->_data_ptr );
		return SUCCESS;
	}
	ret_val = bfm_put_data( &p_bt_data_manager->_bt_file_manager
        , file_index
        , &file_range
        , &p_data_buffer->_data_ptr
        , p_data_buffer->_data_length
        , p_data_buffer->_buffer_length
        , NULL );
	if( ret_val != SUCCESS )
	{		
		LOG_ERROR( " bfm_put_data err:%d", ret_val );
		dm_free_buffer_to_data_buffer( p_data_buffer->_buffer_length, &p_data_buffer->_data_ptr );
	}
	
	ret_val = bt_checker_recv_range( &p_bt_data_manager->_bt_piece_check, &p_data_buffer->_range );
	sd_assert( ret_val == SUCCESS );	
	
	free_range_data_buffer_node( p_data_buffer ); 
	
	bdm_start_read_tmp_file_range( p_bt_data_manager );
	return SUCCESS;
	
}

void bdm_notify_tmp_range_write_ok( BT_DATA_MANAGER *p_bt_data_manager, RANGE *p_padding_range )
{
	LOG_DEBUG( " bdm_notify_tmp_range_write_ok:range:[%u,%u]", p_padding_range->_index, p_padding_range->_num );
	if( brdi_is_padding_range_resv( &p_bt_data_manager->_range_download_info, p_padding_range ) ) return;
	if( correct_manager_is_relate_error_block( &p_bt_data_manager->_correct_manager, p_padding_range ) ) return;
	range_list_add_range( &p_bt_data_manager->_need_read_tmp_range_list, p_padding_range, NULL, NULL );
}


void bdm_report_single_file( BT_DATA_MANAGER *p_bt_data_manager, _u32 file_index, BOOL is_success )
{
	LOG_DEBUG( " bdm_report_single_file:file_index:%u, is_success:%d", file_index, is_success );
	bt_report_single_file( p_bt_data_manager->_task_ptr, file_index, is_success );
}


#endif /* #ifdef ENABLE_BT */

