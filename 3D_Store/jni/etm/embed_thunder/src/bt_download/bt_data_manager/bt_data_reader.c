#include "utility/define.h"
#ifdef ENABLE_BT 


#include "bt_data_reader.h"
#include "bt_range_switch.h"
#include "torrent_parser/torrent_parser_interface.h"

#include "utility/define_const_num.h"
#include "utility/range.h"
#include "utility/utility.h"
#include "utility/mempool.h"

#include "data_manager/file_info.h"

#include "utility/logid.h"
#define LOGID LOGID_BT_DOWNLOAD
#include "utility/logger.h"

static SLAB* g_bt_data_reader_slab = NULL;

#define MAX_READ_TIMES 3


_int32 bdr_init_struct( BT_DATA_READER *p_bt_data_reader, struct tagBT_RANGE_SWITCH *p_range_switcher,
	input_data_handler p_input_hander, output_data_handler p_out_put_hander, 
	read_data_result_handler p_read_data_call_back, void *p_call_back_user )
{
	
	list_init( &p_bt_data_reader->_read_range_info_list );
	
	LOG_DEBUG( "bdr_init_struct." );
	p_bt_data_reader->_input_hander_ptr = p_input_hander;

	p_bt_data_reader->_output_hander_ptr = p_out_put_hander;
	
	p_bt_data_reader->_read_data_call_back_ptr = p_read_data_call_back;
	
	p_bt_data_reader->_call_back_user_ptr = p_call_back_user;
	
	p_bt_data_reader->_cur_read_range_info_ptr = NULL;
	
	p_bt_data_reader->_cur_read_times = 0;
		
	p_bt_data_reader->_finished_len = 0;
		
	p_bt_data_reader->_range_data_buffer_ptr = NULL;
	
	p_bt_data_reader->_range_switcher_ptr = p_range_switcher;

	list_init( &p_bt_data_reader->_read_range_info_list );

	return SUCCESS;
}

_int32 bdr_uninit_struct( BT_DATA_READER *p_bt_data_reader )
{
	
	LOG_DEBUG( "bdr_uninit_struct." );
	
	bdr_read_clear( p_bt_data_reader );
	
	return SUCCESS;
}


_int32 bdr_read_bt_range( BT_DATA_READER *p_bt_data_reader, BT_RANGE *p_bt_range )
{
	_int32 ret_val = SUCCESS;
	sd_assert( p_bt_data_reader->_range_data_buffer_ptr == NULL );
	
	LOG_DEBUG( "bdr_read_bt_range." );

	if( p_bt_data_reader->_range_data_buffer_ptr != NULL )
	{
		bdr_read_clear( p_bt_data_reader );
	}

	ret_val = alloc_range_data_buffer_node( &p_bt_data_reader->_range_data_buffer_ptr );	 
	if( ret_val != SUCCESS )
	{
		LOG_DEBUG( "alloc_range_data_buffer_node err." );
		return BT_READ_GET_BUFFER_FAIL;
	}

	ret_val = sd_malloc( bdr_get_read_length(), (void**)&(p_bt_data_reader->_range_data_buffer_ptr->_data_ptr) ); 			 
	if( ret_val != SUCCESS )
	{
		
		LOG_DEBUG( "sd_malloc data_buffer err." );
		free_range_data_buffer_node( p_bt_data_reader->_range_data_buffer_ptr );		
		p_bt_data_reader->_range_data_buffer_ptr = NULL;
		return BT_READ_GET_BUFFER_FAIL;	   
	}
	
	LOG_DEBUG( "bdr_read_bt_range begin." );

	list_init( &p_bt_data_reader->_read_range_info_list );
	ret_val = brs_bt_range_to_read_range_info_list( p_bt_data_reader->_range_switcher_ptr,
		p_bt_range, &p_bt_data_reader->_read_range_info_list );
	CHECK_VALUE( ret_val );

	sd_assert( list_size( &p_bt_data_reader->_read_range_info_list ) != 0 );
	ret_val = bdr_handle_new_read_range_info( p_bt_data_reader );
	
	LOG_DEBUG( "bdr_read_bt_range end." );

	return ret_val;
}

_int32 bdr_handle_new_read_range_info( BT_DATA_READER *p_bt_data_reader )
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "bdr_handle_new_read_range_info." );
	
	if( list_size( &p_bt_data_reader->_read_range_info_list ) == 0 )
	{
		return bdr_read_success( p_bt_data_reader );
	}

	ret_val = list_pop( &p_bt_data_reader->_read_range_info_list, (void **)&p_bt_data_reader->_cur_read_range_info_ptr );
	CHECK_VALUE( ret_val );
	
	ret_val = bdr_handle_uncomplete_range_info( p_bt_data_reader );
	
	return ret_val;
}


_int32 bdr_handle_uncomplete_range_info( BT_DATA_READER *p_bt_data_reader )
{
	_int32 ret_val = SUCCESS;
	READ_RANGE_INFO *p_read_range_info = p_bt_data_reader->_cur_read_range_info_ptr;
	RANGE *p_read_range = &p_bt_data_reader->_range_data_buffer_ptr->_range;
	_u32 read_max_num = bdr_get_max_read_num();
	_u32 read_max_length = bdr_get_read_length();
	_u32 read_num = 0;
	
	sd_assert( read_max_num != 0 );
	if( read_max_num == 0 ) return BT_DATA_MANAGER_LOGIC_ERROR;

	read_num = MIN( p_read_range_info->_padding_range._num, read_max_num );
	sd_assert( read_num != 0 );

	p_read_range->_index = p_read_range_info->_padding_range._index;	
	p_read_range->_num = read_num;

	p_bt_data_reader->_range_data_buffer_ptr->_buffer_length = p_read_range->_num * get_data_unit_size();
	
	//data_length = p_read_range_info->_padding_exact_range._length + p_read_range_info->_padding_exact_range._pos;
	//sd_assert( data_length > p_read_range_info->_padding_range._index * get_data_unit_size() );
	//data_length -= p_read_range_info->_padding_range._index * get_data_unit_size();
	
	p_bt_data_reader->_range_data_buffer_ptr->_data_length = MIN( p_read_range_info->_range_length, read_max_length );

	LOG_DEBUG( "bdr_handle_uncomplete_range_info.file_index:%u, file_range:[%u,%u], buffer_ptr:0x%x, bufer_len:%u,data_len:%u",
		p_read_range_info->_file_index, p_read_range->_index, p_read_range->_num, 
		p_bt_data_reader->_range_data_buffer_ptr->_data_ptr, 
		p_bt_data_reader->_range_data_buffer_ptr->_buffer_length,
		p_bt_data_reader->_range_data_buffer_ptr->_data_length );

	ret_val = p_bt_data_reader->_input_hander_ptr( p_bt_data_reader->_call_back_user_ptr, 
		p_read_range_info->_file_index, p_bt_data_reader->_range_data_buffer_ptr, bdr_notify_read_result );

	return ret_val;	
}

_int32 bdr_notify_read_result( struct tagTMP_FILE *p_file_struct, void *p_user, RANGE_DATA_BUFFER *p_range_data_buffer, _int32 read_result, _u32 read_len )
{
	_int32 ret_val = SUCCESS;
	
	BT_DATA_READER *p_bt_data_reader = ( BT_DATA_READER * )p_user;
	READ_RANGE_INFO *p_read_range_info = p_bt_data_reader->_cur_read_range_info_ptr;
	char *p_data_buffer = p_range_data_buffer->_data_ptr;
	RANGE *p_range = &p_range_data_buffer->_range;
	_u64 range_start_pos = 0;
	_u32 buffer_offset = 0;
	_u32 buffer_data_len = 0;
	LIST_ITERATOR cur_node = NULL;

	LOG_DEBUG( "bdr_notify_read_result. buffer_ptr:0x%x, bufer_len:%u. call_back_buffer_ptr:0x%x, read_len:%u, read_result:%d",
		p_bt_data_reader->_range_data_buffer_ptr->_data_ptr, p_bt_data_reader->_range_data_buffer_ptr->_buffer_length,
		p_range_data_buffer->_data_ptr, read_len, read_result);
	

	sd_assert( p_bt_data_reader->_range_data_buffer_ptr == p_range_data_buffer );
    if( read_result != SUCCESS || read_len == 0 )
    {
		p_bt_data_reader->_cur_read_times++;
		if( p_bt_data_reader->_cur_read_times > MAX_READ_TIMES )
		{
			return bdr_read_failed( p_bt_data_reader );
		}
		if( read_result == FM_READ_CLOSE_EVENT )
		{
			return bdr_read_failed( p_bt_data_reader );
		}
		
		ret_val = bdr_handle_uncomplete_range_info( p_bt_data_reader );
		if( ret_val != SUCCESS )
		{
			bdr_read_failed( p_bt_data_reader );
		}

		return SUCCESS;
    }
	
	p_bt_data_reader->_cur_read_times = 0;
	sd_assert( p_range->_index == p_read_range_info->_padding_range._index );
	range_start_pos = (_u64)get_data_unit_size() * p_read_range_info->_padding_range._index;
	sd_assert( p_read_range_info->_padding_exact_range._pos >= range_start_pos );
	if( p_read_range_info->_padding_exact_range._pos < range_start_pos )
	{
		return BT_DATA_MANAGER_LOGIC_ERROR;
	}
	buffer_offset = p_read_range_info->_padding_exact_range._pos - range_start_pos;

	sd_assert( read_len >= buffer_offset );
	if( read_len < buffer_offset )
	{
		return BT_DATA_MANAGER_LOGIC_ERROR;
	}

	LOG_DEBUG( "bdr_notify_read_result begin.file_index:%d,padding_range:[%u,%u],exact_file_range:[%llu,%llu], call_back_range:[%u,%u], buffer_offset:%u ",
		p_read_range_info->_file_index, 
		p_read_range_info->_padding_range._index, p_read_range_info->_padding_range._num, 
		p_read_range_info->_padding_exact_range._pos, p_read_range_info->_padding_exact_range._length,
		p_range_data_buffer->_range._index, p_range_data_buffer->_range._num,
		buffer_offset );

	buffer_data_len = MIN( p_read_range_info->_padding_exact_range._length, read_len - buffer_offset );
	ret_val = p_bt_data_reader->_output_hander_ptr( p_bt_data_reader->_call_back_user_ptr, 
		 p_bt_data_reader->_finished_len, p_data_buffer + buffer_offset, buffer_data_len );
	CHECK_VALUE( ret_val );	
	p_bt_data_reader->_finished_len += buffer_data_len;

	p_read_range_info->_padding_range._index += p_range->_num;
	sd_assert( p_read_range_info->_padding_range._num >= p_range->_num );
	p_read_range_info->_padding_range._num -= p_range->_num;
	p_read_range_info->_padding_exact_range._pos += buffer_data_len;
	sd_assert( p_read_range_info->_padding_exact_range._length >= buffer_data_len );
	p_read_range_info->_padding_exact_range._length -= buffer_data_len; 
	p_read_range_info->_range_length -= read_len; 

	if( p_read_range_info->_padding_exact_range._length == 0 )
	{
		sd_assert( p_read_range_info->_padding_range._num == 0 );
		brs_release_read_range_info( p_bt_data_reader->_cur_read_range_info_ptr );
		p_bt_data_reader->_cur_read_range_info_ptr = NULL;
	}	
	else
	{
		LOG_DEBUG( "bdr_notify_read_result read_range_info unfinished.file_index:%d,padding_range:[%u,%u],exact_file_range:[%llu,%llu], call_back_range:[%u,%u] ",
			p_read_range_info->_file_index, 
			p_read_range_info->_padding_range._index, p_read_range_info->_padding_range._num, 
			p_read_range_info->_padding_exact_range._pos, p_read_range_info->_padding_exact_range._length,
			p_range_data_buffer->_range._index, p_range_data_buffer->_range._num );
		
		cur_node = LIST_BEGIN( p_bt_data_reader->_read_range_info_list );
		list_insert( &p_bt_data_reader->_read_range_info_list, (void *)p_bt_data_reader->_cur_read_range_info_ptr, cur_node );

		p_bt_data_reader->_cur_read_range_info_ptr = NULL;
	}
	
	ret_val = bdr_handle_new_read_range_info( p_bt_data_reader );
	sd_assert( ret_val == SUCCESS ); 

 	return SUCCESS;
	
}

_int32 bdr_read_success( BT_DATA_READER *p_bt_data_reader )
{
	_u32 finished_len = p_bt_data_reader->_finished_len;
	
	LOG_DEBUG( "bdr_read_success.finished_len:%u", finished_len );
	bdr_read_clear( p_bt_data_reader );

	p_bt_data_reader->_read_data_call_back_ptr( p_bt_data_reader->_call_back_user_ptr, SUCCESS, finished_len );
	return SUCCESS;
}


_int32 bdr_read_failed( BT_DATA_READER *p_bt_data_reader )
{
	_u32 finished_len = p_bt_data_reader->_finished_len;
	LOG_DEBUG( "bdr_read_failed.finished_len:%u", finished_len );

	bdr_read_clear( p_bt_data_reader );

	p_bt_data_reader->_read_data_call_back_ptr( p_bt_data_reader->_call_back_user_ptr, BT_DATA_MANAGER_READ_DATA_ERROR, finished_len );

	return SUCCESS;
}

void bdr_read_clear( BT_DATA_READER *p_bt_data_reader )
{
	
	LOG_DEBUG( "bdr_read_clear." );
	if( p_bt_data_reader->_cur_read_range_info_ptr != NULL )
	{
		brs_release_read_range_info( p_bt_data_reader->_cur_read_range_info_ptr );
		p_bt_data_reader->_cur_read_range_info_ptr = NULL;
	}
	brs_release_read_range_info_list( &p_bt_data_reader->_read_range_info_list );
	
	p_bt_data_reader->_cur_read_times = 0;
		
	p_bt_data_reader->_finished_len = 0;
	
	if( p_bt_data_reader->_range_data_buffer_ptr != NULL )
	{
		SAFE_DELETE( p_bt_data_reader->_range_data_buffer_ptr->_data_ptr );
		free_range_data_buffer_node( p_bt_data_reader->_range_data_buffer_ptr );		
		p_bt_data_reader->_range_data_buffer_ptr = NULL;
	}
}


_int32 bdr_cancel( BT_DATA_READER *p_bt_data_reader, BT_RANGE bt_range )
{
	LOG_DEBUG("bdr_cancel .");
	return SUCCESS;
}

_u32 bdr_get_max_read_num(void)
{
	sd_assert( get_data_unit_size() != 0 );
//	return DATA_CHECK_MAX_READ_LENGTH / get_data_unit_size();
	return get_read_length() / get_data_unit_size();	
}

_u32 bdr_get_read_length(void)
{
	sd_assert( get_read_length() != 0 );
	return get_read_length();
}

//bt_data_reader malloc/free
_int32 init_bt_data_reader_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_bt_data_reader_slab == NULL )
	{              
		ret_val = mpool_create_slab( sizeof( BT_DATA_READER ), MIN_BT_DATA_READER, 0, &g_bt_data_reader_slab );
		CHECK_VALUE( ret_val );
	}
	return ret_val;
}

_int32 uninit_bt_data_reader_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_bt_data_reader_slab != NULL )
	{              
		ret_val = mpool_destory_slab( g_bt_data_reader_slab );
		CHECK_VALUE( ret_val );
		g_bt_data_reader_slab = NULL;
	}
	return ret_val;
}

_int32 bt_data_reader_malloc_wrap(struct tagBT_DATA_READER **pp_node)
{
	return mpool_get_slip( g_bt_data_reader_slab, (void**)pp_node );
}

_int32 bt_data_reader_free_wrap(struct tagBT_DATA_READER *p_node)
{
	sd_assert( p_node != NULL );
	if( p_node == NULL ) return SUCCESS;
	return mpool_free_slip( g_bt_data_reader_slab, (void*)p_node );
}



#endif /* #ifdef ENABLE_BT */

