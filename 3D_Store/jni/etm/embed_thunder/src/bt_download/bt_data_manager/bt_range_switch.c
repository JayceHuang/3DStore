#include "utility/define.h"
#ifdef ENABLE_BT 


#include "bt_range_switch.h"
#include "torrent_parser/torrent_parser_interface.h"
#include "utility/range.h"
#include "utility/mempool.h"
#include "utility/utility.h"

#include "utility/logid.h"
#define LOGID LOGID_BT_DOWNLOAD
#include "utility/logger.h"

#ifdef _TEST
#include "test_bt_range_switch.h"
#endif


static SLAB* g_piece_range_info_slab = NULL;
static SLAB* g_read_range_info_slab = NULL;
static SLAB* g_sub_file_padding_range_info_slab = NULL;

_int32 brs_init_module( void )
{
	_int32 ret_val = SUCCESS;
	
	LOG_DEBUG("brs_init_module" );
	ret_val = init_piece_range_info_slabs();
	CHECK_VALUE( ret_val );

	ret_val = init_read_range_info_slabs();
	CHECK_VALUE( ret_val );	

	ret_val = init_sub_file_padding_range_info_slabs();
	CHECK_VALUE( ret_val );	
	return SUCCESS;
}

_int32 brs_uninit_module( void )
{
	
	LOG_DEBUG("brs_uninit_module" );
	uninit_piece_range_info_slabs();

	uninit_read_range_info_slabs();
	
	uninit_sub_file_padding_range_info_slabs();
	return SUCCESS;
}

_int32 brs_init_struct( BT_RANGE_SWITCH *p_bt_range_switch, struct tagTORRENT_PARSER *p_torrent_parser,
	_u32 *p_need_download_file_array, _u32 need_download_file_num )
{
	TORRENT_FILE_INFO *p_torrent_file_info = NULL;
	FILE_RANGE_INFO *p_file_range_info = NULL;
	_u32 file_index = 0;
	_u32 index = 0;
	_int32 ret_val = SUCCESS;
	_u32 next_file_p2sp_view_pos = 0;
	p_bt_range_switch->_file_num = tp_get_seed_file_num( p_torrent_parser );
	p_bt_range_switch->_piece_size = tp_get_piece_size( p_torrent_parser );


	sd_assert( p_bt_range_switch->_piece_size != 0);
	if( p_bt_range_switch->_piece_size == 0 ) return BT_DATA_MANAGER_LOGIC_ERROR;
	p_bt_range_switch->_file_size = tp_get_file_total_size( p_torrent_parser );
	if( p_bt_range_switch->_file_size % p_bt_range_switch->_piece_size != 0 )
	{
		p_bt_range_switch->_last_piece_size = p_bt_range_switch->_file_size % p_bt_range_switch->_piece_size;
		p_bt_range_switch->_piece_num = p_bt_range_switch->_file_size / p_bt_range_switch->_piece_size + 1;
	}
	else
	{
		p_bt_range_switch->_last_piece_size = p_bt_range_switch->_piece_size;
		p_bt_range_switch->_piece_num = p_bt_range_switch->_file_size / p_bt_range_switch->_piece_size;
	}

	LOG_DEBUG("brs_init_struct, file_num:%u, file_size:%llu, piece_num:%u, piece_size:%u, last_piece_size:%u ",
		p_bt_range_switch->_file_num, p_bt_range_switch->_file_size, 
		p_bt_range_switch->_piece_num, p_bt_range_switch->_piece_size,
		p_bt_range_switch->_last_piece_size );

	sd_assert( p_bt_range_switch->_file_num > 0 );
	if( p_bt_range_switch->_file_num == 0 )
	{
		return BT_SEED_LACK_FILE;
	}
	sd_assert( get_data_unit_size() != 0 );
	if( get_data_unit_size() == 0 )
	{
		return BT_DATA_MANAGER_LOGIC_ERROR;
	}	
	
	ret_val = sd_malloc( p_bt_range_switch->_file_num * sizeof( FILE_RANGE_INFO ), (void**)&(p_bt_range_switch->_file_range_info_array) );
	CHECK_VALUE( ret_val );

	for( file_index = 0; file_index < p_bt_range_switch->_file_num; file_index++ )
	{
		ret_val = tp_get_file_info( p_torrent_parser, file_index, &p_torrent_file_info );
		if( ret_val != SUCCESS && ret_val != BT_FILE_INDEX_NOT_DOWNLOAD )
		{
			SAFE_DELETE( p_bt_range_switch->_file_range_info_array );
			return ret_val;
		}
		
		p_file_range_info = &p_bt_range_switch->_file_range_info_array[ file_index ];

		sd_assert( p_torrent_file_info->_file_index == file_index );

		p_file_range_info->_file_index = p_torrent_file_info->_file_index;
		p_file_range_info->_file_offset = p_torrent_file_info->_file_offset;
		p_file_range_info->_file_size = p_torrent_file_info->_file_size;
		p_file_range_info->_pos_p2sp_view = next_file_p2sp_view_pos;

		if( p_file_range_info->_file_size % get_data_unit_size() == 0 )
		{
			p_file_range_info->_file_range_num = p_file_range_info->_file_size / get_data_unit_size();
			p_file_range_info->_right_padding_size = 0;
		}
		else
		{
			p_file_range_info->_file_range_num = p_file_range_info->_file_size / get_data_unit_size() + 1;
			p_file_range_info->_right_padding_size = get_data_unit_size() - p_file_range_info->_file_size % get_data_unit_size();
		}
		next_file_p2sp_view_pos  = p_file_range_info->_pos_p2sp_view + p_file_range_info->_file_range_num;

		
		p_file_range_info->_is_need_download = FALSE;
		for( index = 0; index < need_download_file_num; index++ )
		{
			if( p_need_download_file_array[ index ] == file_index )
			{
				p_file_range_info->_is_need_download = TRUE;
				break;
			}
		}
		LOG_DEBUG("brs_init_struct, file_index:%u, file_offset:%llu, file_size:%llu, pos_p2sp_view:%u, right_padding_size:%u, file_range_num:%u, file_need_download:%d",
			p_file_range_info->_file_index, p_file_range_info->_file_offset, p_file_range_info->_file_size,
			p_file_range_info->_pos_p2sp_view, p_file_range_info->_right_padding_size, 
			p_file_range_info->_file_range_num, p_file_range_info->_is_need_download );

	}

#ifdef _TEST
	//brs_test( p_bt_range_switch );
#endif

	return SUCCESS;
}

_int32 brs_uninit_struct( BT_RANGE_SWITCH *p_bt_range_switch )
{
	LOG_DEBUG("brs_uninit_struct" );	
	SAFE_DELETE( p_bt_range_switch->_file_range_info_array );
	return SUCCESS;
}

_int32 brs_padding_range_to_bt_range( const BT_RANGE_SWITCH *p_bt_range_switch, const RANGE *p_padding_range, BT_RANGE *p_bt_range )
{
	_u32 range_first_file_index = 0;
	_u32 range_last_file_index = 0;
	_u32 file_index = 0;
	_int32 ret_val = SUCCESS;
	_u32 file_pos_p2sp_view = 0;
	_u32 last_file_end = 0;

	ret_val = brs_search_file_index( p_bt_range_switch, p_padding_range->_index, p_padding_range->_num, brs_get_file_padding_pos, &range_first_file_index, &range_last_file_index );
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS )
	{
		return BT_DATA_MANAGER_LOGIC_ERROR;
	}
	
	file_pos_p2sp_view = p_bt_range_switch->_file_range_info_array[range_first_file_index]._pos_p2sp_view;

	sd_assert( p_padding_range->_index >= file_pos_p2sp_view );
	if( p_padding_range->_index < file_pos_p2sp_view )
	{
		return BT_DATA_MANAGER_LOGIC_ERROR;
	}
	p_bt_range->_pos = (_u64)get_data_unit_size() * ( p_padding_range->_index - file_pos_p2sp_view ) + p_bt_range_switch->_file_range_info_array[range_first_file_index]._file_offset;
	p_bt_range->_length = (_u64)get_data_unit_size() * p_padding_range->_num;

	sd_assert( range_first_file_index <= range_last_file_index );
	if( range_first_file_index > range_last_file_index )
	{
		return BT_DATA_MANAGER_LOGIC_ERROR;
	}
	
	for( file_index = range_first_file_index; file_index < range_last_file_index; file_index++ )
	{
		sd_assert( p_bt_range->_length >= p_bt_range_switch->_file_range_info_array[file_index]._right_padding_size );
		p_bt_range->_length -= p_bt_range_switch->_file_range_info_array[file_index]._right_padding_size;
	}
	
	last_file_end = p_bt_range_switch->_file_range_info_array[range_last_file_index]._pos_p2sp_view;
	last_file_end += p_bt_range_switch->_file_range_info_array[range_last_file_index]._file_range_num;
	if( p_padding_range->_index + p_padding_range->_num == last_file_end  )
	{
		p_bt_range->_length -= p_bt_range_switch->_file_range_info_array[range_last_file_index]._right_padding_size;
	}

	LOG_DEBUG("brs_padding_range_to_bt_range end: file_pos_p2sp_view:%u, padding_range:[%u,%u], bt_range[%llu,%llu]",
		file_pos_p2sp_view,
		p_padding_range->_index, p_padding_range->_num,
		p_bt_range->_pos, p_bt_range->_length
	 );	
	return SUCCESS;
}


_int32 brs_range_to_extra_piece( const BT_RANGE_SWITCH *p_bt_range_switch, const RANGE *p_padding_range, _u32 *p_first_piece, _u32 *p_last_piece )
{
	BT_RANGE bt_range;
	_int32 ret_val = SUCCESS;
	
	ret_val = brs_padding_range_to_bt_range( p_bt_range_switch, p_padding_range, &bt_range );
	CHECK_VALUE( ret_val );

	if (p_bt_range_switch->_piece_size > 0) {
	*p_first_piece = bt_range._pos / p_bt_range_switch->_piece_size;
	*p_last_piece = ( bt_range._pos + bt_range._length - 1 ) / p_bt_range_switch->_piece_size;
	}

	LOG_DEBUG("brs_range_to_extra_piece: padding_range:[%u,%u], first_piece:%u, last_piece:%u",
		p_padding_range->_index, p_padding_range->_num,
		*p_first_piece, *p_last_piece );	

	return SUCCESS;
}


_int32 brs_padding_range_to_file_range( const BT_RANGE_SWITCH *p_bt_range_switch, _u32 file_index, const RANGE *p_padding_range, RANGE *p_file_range )
{
	_u32 file_pos_p2sp_view = p_bt_range_switch->_file_range_info_array[file_index]._pos_p2sp_view;

	if( p_padding_range->_index < file_pos_p2sp_view )
	{
		return BT_DATA_MANAGER_LOGIC_ERROR;
	}
	p_file_range->_index = p_padding_range->_index - file_pos_p2sp_view;
	p_file_range->_num = p_padding_range->_num;
	
	LOG_DEBUG("brs_padding_range_to_file_range end: file_index:%d, file_pos_p2sp_view:%u, padding_range:[%u,%u], file_range[%u,%u]",
		file_index, file_pos_p2sp_view,
		p_padding_range->_index, p_padding_range->_num,	
		p_file_range->_index, p_file_range->_num
	 );
		
	return SUCCESS;
}

//将bt_range转成bt_range包含的最大padding_range
_int32 brs_bt_range_to_padding_range( const BT_RANGE_SWITCH *p_bt_range_switch,
	const BT_RANGE *p_bt_range, RANGE *p_padding_range )
{
	_u32 range_first_file_index = 0;
	_u32 range_last_file_index = 0;
	_int32 ret_val = SUCCESS;
	_u64 file_pos_bt_view = 0;
	_u32 file_pos_p2sp_view = 0;
	_u64 bt_range_end = 0;
	
	_u64 exact_range_end = 0;
	_u32 lack_size = 0;
	_u64 tmp_size = 0;
	
	ret_val = brs_search_file_index( p_bt_range_switch, p_bt_range->_pos, p_bt_range->_length, brs_get_file_bt_pos, &range_first_file_index, &range_last_file_index );
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS )
	{
		return BT_DATA_MANAGER_LOGIC_ERROR;
	}
	
	LOG_DEBUG("brs_bt_range_to_padding_range begin: first_file_index:%u, last_file_index:%u, bt_range:[%llu,%llu]",
		range_first_file_index, range_last_file_index,
		p_bt_range->_pos, p_bt_range->_length );
	
	file_pos_bt_view = p_bt_range_switch->_file_range_info_array[range_first_file_index]._file_offset;
	file_pos_p2sp_view = p_bt_range_switch->_file_range_info_array[range_first_file_index]._pos_p2sp_view;

	sd_assert( p_bt_range->_pos >= file_pos_bt_view );
	if( p_bt_range->_pos < file_pos_bt_view )
	{
		return BT_DATA_MANAGER_LOGIC_ERROR;
	}
	
	lack_size = ( p_bt_range->_pos - file_pos_bt_view ) % get_data_unit_size();

	if( lack_size == 0 )
	{
		p_padding_range->_index = ( p_bt_range->_pos - file_pos_bt_view ) / (_u64)get_data_unit_size() + file_pos_p2sp_view;
	}
	else
	{
		p_padding_range->_index = ( p_bt_range->_pos - file_pos_bt_view ) / (_u64)get_data_unit_size() + file_pos_p2sp_view + 1;
	}

	file_pos_bt_view = p_bt_range_switch->_file_range_info_array[range_last_file_index]._file_offset;
	file_pos_p2sp_view = p_bt_range_switch->_file_range_info_array[range_last_file_index]._pos_p2sp_view;
	bt_range_end = p_bt_range->_pos + p_bt_range->_length - 1;
	sd_assert( bt_range_end >= file_pos_bt_view );
	if( bt_range_end < file_pos_bt_view )
	{
		return BT_DATA_MANAGER_LOGIC_ERROR;
	}
	exact_range_end = bt_range_end - file_pos_bt_view + (_u64)file_pos_p2sp_view * get_data_unit_size();

	if( exact_range_end > (_u64)file_pos_p2sp_view * get_data_unit_size() + p_bt_range_switch->_file_range_info_array[range_last_file_index]._file_size - 1 )
	{
		return BT_RANGE_TOO_BIG;
	}

	//如果是文件最后一块,需要加上padding
	if( exact_range_end >= (_u64)file_pos_p2sp_view * get_data_unit_size() + p_bt_range_switch->_file_range_info_array[range_last_file_index]._file_size - 1 )
	{
		exact_range_end += p_bt_range_switch->_file_range_info_array[range_last_file_index]._right_padding_size;
	}
	
	if( exact_range_end < (_u64)p_padding_range->_index * get_data_unit_size() )
	{
		p_padding_range->_num= 0;
		LOG_DEBUG("brs_bt_range_to_padding_range BT_RANGE_TOO_SMALL" );
		return BT_RANGE_TOO_SMALL;
	}	
	tmp_size = exact_range_end - (_u64)p_padding_range->_index * get_data_unit_size() + 1;
	if( tmp_size < get_data_unit_size() )
	{
	
		LOG_DEBUG("brs_bt_range_to_padding_range BT_RANGE_TOO_SMALL" );
		return BT_RANGE_TOO_SMALL;
	}	
	else
	{
		p_padding_range->_num = tmp_size / (_u64)get_data_unit_size();
	}

	
	LOG_DEBUG("brs_bt_range_to_padding_range end: range_first_file_index:%d, range_last_file_index:%u, bt_range:[%llu,%llu], padding_range[%u,%u]",
		range_first_file_index, range_last_file_index,
		p_bt_range->_pos, p_bt_range->_length,
		p_padding_range->_index, p_padding_range->_num );
	
	return SUCCESS;
}

_int32 brs_piece_to_extra_padding_range( const BT_RANGE_SWITCH *p_bt_range_switch, _u32 piece_index, RANGE *p_padding_range )
{
	BT_RANGE bt_range;
	bt_range._pos = (_u64) piece_index * p_bt_range_switch->_piece_size;
	if( piece_index != p_bt_range_switch->_piece_num - 1 )
	{
		bt_range._length = p_bt_range_switch->_piece_size;
	}
	else
	{
		bt_range._length = p_bt_range_switch->_last_piece_size;
	}
	return brs_bt_range_to_extra_padding_range( p_bt_range_switch, &bt_range, p_padding_range );
}

//将bt_range转成包含bt_range的最小padding_range
_int32 brs_bt_range_to_extra_padding_range( const BT_RANGE_SWITCH *p_bt_range_switch,
	const BT_RANGE *p_bt_range, RANGE *p_padding_range )
{
	_u32 range_first_file_index = 0;
	_u32 range_last_file_index = 0;
	_int32 ret_val = SUCCESS;
	_u64 file_pos_bt_view = 0;
	_u32 file_pos_p2sp_view = 0;
	_u64 bt_range_end = 0;
	
	_u64 exact_range_end = 0;
	_u64 lack_size = 0;
	_u64 tmp_size = 0;

	if( p_bt_range->_length == 0 )
	{
		LOG_DEBUG("brs_bt_range_to_extra_padding_range BT_RANGE_TOO_SMALL" );
		return BT_RANGE_TOO_SMALL;
	}		
	
	ret_val = brs_search_file_index( p_bt_range_switch, p_bt_range->_pos, p_bt_range->_length, brs_get_file_bt_pos, &range_first_file_index, &range_last_file_index );
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS )
	{
		return BT_DATA_MANAGER_LOGIC_ERROR;
	}

	LOG_DEBUG("brs_bt_range_to_extra_padding_range begin: first_file_index:%u, last_file_index:%u, bt_range:[%llu,%llu]",
		range_first_file_index, range_last_file_index,
		p_bt_range->_pos, p_bt_range->_length );
	
	file_pos_bt_view = p_bt_range_switch->_file_range_info_array[range_first_file_index]._file_offset;
	file_pos_p2sp_view = p_bt_range_switch->_file_range_info_array[range_first_file_index]._pos_p2sp_view;

	sd_assert( p_bt_range->_pos >= file_pos_bt_view );
	if( p_bt_range->_pos < file_pos_bt_view )
	{
		return BT_DATA_MANAGER_LOGIC_ERROR;
	}
	
	p_padding_range->_index = ( p_bt_range->_pos - file_pos_bt_view ) / (_u64)get_data_unit_size() + file_pos_p2sp_view ;

	file_pos_bt_view = p_bt_range_switch->_file_range_info_array[range_last_file_index]._file_offset;
	file_pos_p2sp_view = p_bt_range_switch->_file_range_info_array[range_last_file_index]._pos_p2sp_view;

	//bt_range_end = p_bt_range->_pos + p_bt_range->_length;
	bt_range_end = p_bt_range->_pos + p_bt_range->_length - 1;
	
	sd_assert( bt_range_end >= file_pos_bt_view );
	if( bt_range_end < file_pos_bt_view )
	{
		return BT_DATA_MANAGER_LOGIC_ERROR;
	}
	exact_range_end = bt_range_end - file_pos_bt_view + ((_u64)file_pos_p2sp_view) * get_data_unit_size();
	sd_assert( exact_range_end >= (_u64)p_padding_range->_index * get_data_unit_size() );
	if( exact_range_end < ((_u64)p_padding_range->_index) * (_u64)get_data_unit_size() )
	{
		p_padding_range->_num= 0;
		LOG_DEBUG("brs_bt_range_to_extra_padding_range BT_DATA_MANAGER_LOGIC_ERROR" );
		return BT_DATA_MANAGER_LOGIC_ERROR;
	}	
	tmp_size = exact_range_end - (_u64)p_padding_range->_index * (_u64)get_data_unit_size() + 1;

	lack_size = (_u64)tmp_size % (_u64)get_data_unit_size();

	if( lack_size == 0 )
	{
		p_padding_range->_num = tmp_size /(_u64)get_data_unit_size();
	}
	else
	{
		p_padding_range->_num = tmp_size /(_u64)get_data_unit_size() + 1;
	}
	
	LOG_DEBUG("brs_bt_range_to_extra_padding_range end: range_first_file_index:%d, range_last_file_index:%u, bt_range:[%llu,%llu], padding_range[%u,%u]",
		range_first_file_index, range_last_file_index,
		p_bt_range->_pos, p_bt_range->_length,
		p_padding_range->_index, p_padding_range->_num );
	
	return SUCCESS;


}


_int32 brs_file_range_to_padding_range( const BT_RANGE_SWITCH *p_bt_range_switch, _u32 file_index, 
    const RANGE *p_file_range, RANGE *p_padding_range )
{
	_u32 file_pos_p2sp_view = p_bt_range_switch->_file_range_info_array[file_index]._pos_p2sp_view;
	p_padding_range->_index = file_pos_p2sp_view + p_file_range->_index;
	p_padding_range->_num =  p_file_range->_num;
	
	LOG_DEBUG("brs_file_range_to_padding_range: file_index:%d, file_pos_p2sp_view:%u, file_range:[%u,%u], padding_range[%u,%u]",
		file_index, file_pos_p2sp_view,
		p_file_range->_index, p_file_range->_num,
		p_padding_range->_index, p_padding_range->_num );		
	return SUCCESS;
}

_int32 brs_search_file_index( const BT_RANGE_SWITCH *p_bt_range_switch, _u64 range_pos, _u64 range_length, brs_get_file_pos p_get_file_pos, _u32 *p_range_first_file_index, _u32 *p_range_last_file_index )
{
	_u32 file_num = 0;
	_u32 small_file_index = 0;
	_u32 big_file_index = 0;
	_u32 middle_file_index = 0; 
	_u64 middle_file_index_pos = 0;
	_u64 small_file_index_pos = 0;
	_u64 big_file_index_pos = 0;
	_u32 file_index = 0;
	_u64 file_index_pos = 0;
	
	LOG_DEBUG("brs_search_file_index begin: range_pos:%llu, range_length:%llu",
		range_pos, range_length );

	if( p_bt_range_switch->_file_num == 0 )
	{
		return BT_SEED_LACK_FILE;
	}
	file_num = p_bt_range_switch->_file_num;
	big_file_index = file_num - 1;
	
	middle_file_index = ( small_file_index + big_file_index ) / 2;

	while( small_file_index < big_file_index 
		&& small_file_index != middle_file_index 
		&& big_file_index != middle_file_index )
	{
		middle_file_index_pos = p_get_file_pos( &p_bt_range_switch->_file_range_info_array[ middle_file_index ] );
		if( middle_file_index_pos <= range_pos )
		{
			small_file_index = middle_file_index;
		}
		else //if( middle_file_index_pos > range_pos )
		{
			big_file_index = middle_file_index;
		}
		middle_file_index = ( small_file_index + big_file_index ) / 2;
	}

	small_file_index_pos = p_get_file_pos( &p_bt_range_switch->_file_range_info_array[ small_file_index ] );
	big_file_index_pos = p_get_file_pos( &p_bt_range_switch->_file_range_info_array[ big_file_index ] );

	sd_assert( small_file_index_pos <= range_pos );
	sd_assert( middle_file_index == small_file_index );

	if( big_file_index_pos <= range_pos )
	{
		*p_range_first_file_index = big_file_index;
	}
	else
	{
		*p_range_first_file_index = small_file_index;
	}

	for( file_index = *p_range_first_file_index + 1; file_index < p_bt_range_switch->_file_num; file_index++ )
	{
		file_index_pos = p_get_file_pos( &p_bt_range_switch->_file_range_info_array[ file_index ] );
		if( file_index_pos >= range_pos + range_length )
		{
			break;
		}
	}
	*p_range_last_file_index = file_index - 1;

	LOG_DEBUG("brs_search_file_index end: first_file_index:%u, last_file_index:%u",
		*p_range_first_file_index, *p_range_last_file_index );	
	return SUCCESS;	
}

_int32 brs_piece_search_file_index( const BT_RANGE_SWITCH *p_bt_range_switch, _u32 piece_index, _u32 *p_first_file, _u32 *p_last_file )
{
	_u64 pos = 0;
	_u64 length = 0;
	pos = (_u64)p_bt_range_switch->_piece_size * piece_index;
	if( piece_index != p_bt_range_switch->_piece_num - 1 )
	{
		length = p_bt_range_switch->_piece_size;
	}
	else
	{
		length = p_bt_range_switch->_last_piece_size;
	}
	return brs_search_file_index( p_bt_range_switch, pos, length, brs_get_file_bt_pos, p_first_file, p_last_file );
}

/*
_int32 brs_padding_range_search_file_index( const BT_RANGE_SWITCH *p_bt_range_switch, RANGE *p_padding_range, _u32 *p_first_file, _u32 *p_last_file )
{
	return brs_search_file_index( p_bt_range_switch, p_padding_range->_index, p_padding_range->_num, brs_get_file_padding_pos, p_first_file, p_last_file );
}
*/
_int32 brs_get_padding_range_file_index_list( const BT_RANGE_SWITCH *p_bt_range_switch, RANGE *p_padding_range, LIST *p_list )
{
	_u32 first_file_index = 0;
	_u32 last_file_index = 0;
	_u32 file_index = 0;
	_int32 ret_val = SUCCESS;
	RANGE tmp_range;
	_u32 range_num = 0;
	_u32 temp_num = 0;
	SUB_FILE_PADDING_RANGE_INFO *p_range_info = NULL;
	ret_val = brs_search_file_index( p_bt_range_switch, p_padding_range->_index, p_padding_range->_num, brs_get_file_padding_pos, &first_file_index, &last_file_index );
	CHECK_VALUE( ret_val );
	tmp_range._index = p_padding_range->_index;
	tmp_range._num = p_padding_range->_num;
	LOG_DEBUG("brs_get_padding_range_file_index_list: p_padding_range:[%u, %u]",
		 p_padding_range->_index, p_padding_range->_num );

	for( file_index = first_file_index; file_index <= last_file_index; file_index++ )
	{
		ret_val = sub_file_padding_range_info_malloc_wrap( &p_range_info );
		sd_assert( ret_val == SUCCESS );
		if( ret_val != SUCCESS ) break;
		
		p_range_info->_file_index = file_index;
		p_range_info->_padding_range._index = tmp_range._index;
		
		sd_assert( tmp_range._num > 0 );
		if( tmp_range._num == 0 )
		{
			ret_val = BT_DATA_MANAGER_LOGIC_ERROR;
			break;
		}
		temp_num = p_bt_range_switch->_file_range_info_array[ file_index ]._pos_p2sp_view +
			p_bt_range_switch->_file_range_info_array[ file_index ]._file_range_num;
		sd_assert( temp_num > tmp_range._index );
		temp_num -= tmp_range._index;
			
		range_num = MIN( tmp_range._num, temp_num );
		p_range_info->_padding_range._num = range_num;
		tmp_range._index += range_num;
		sd_assert( tmp_range._num >= range_num );
		tmp_range._num -= range_num;
		ret_val = list_push( p_list, (void *)p_range_info );
		sd_assert( ret_val == SUCCESS );
		if( ret_val != SUCCESS ) break;
		LOG_DEBUG("brs_get_padding_range_file_index_list: file_index:%u, p_padding_range:[%u, %u]",
			 file_index, p_range_info->_padding_range._index, p_range_info->_padding_range._num );

	}
	
	if( ret_val != SUCCESS )
	{
		brs_release_padding_range_file_index_list( p_list );
	}
	return ret_val;
}

void brs_release_padding_range_file_index_list( LIST *p_list )
{
	LIST_ITERATOR cur_node = NULL;
	SUB_FILE_PADDING_RANGE_INFO *p_range_info = NULL;
	LOG_DEBUG("brs_get_padding_range_file_index_list: list:0x%x", p_list );

	cur_node = LIST_BEGIN( *p_list );
	while( cur_node != LIST_END( *p_list ) )
	{
		p_range_info = LIST_VALUE( cur_node );
		sub_file_padding_range_info_free_wrap( p_range_info );
		cur_node = LIST_NEXT( cur_node );
	}
	list_clear( p_list );
}

_u64 brs_get_file_padding_pos( FILE_RANGE_INFO *p_file_range_info )
{
	// LOG_DEBUG("brs_get_file_padding_pos :%u", p_file_range_info->_pos_p2sp_view );	
	return p_file_range_info->_pos_p2sp_view;
}

_u64 brs_get_file_bt_pos( FILE_RANGE_INFO *p_file_range_info )
{
	// LOG_DEBUG("brs_get_file_bt_pos :%u", p_file_range_info->_file_offset );	
	return p_file_range_info->_file_offset;
}

//padding 必须在一个文件内
_int32 brs_padding_range_to_sub_file_range( const BT_RANGE_SWITCH *p_bt_range_switch, const RANGE *p_padding_range, _u32 *p_file_index, RANGE *p_sub_file_range )
{
	_u32 range_first_file_index = 0;
	_u32 range_last_file_index = 0;
	_int32 ret_val = SUCCESS;
	FILE_RANGE_INFO *p_file_range_info = NULL;


	sd_assert( p_bt_range_switch->_piece_num > 0 );

	LOG_DEBUG("brs_padding_range_to_sub_file_range: target padding_range:[%u, %u]",
		p_padding_range->_index, p_padding_range->_num );

	ret_val = brs_search_file_index( p_bt_range_switch, p_padding_range->_index, p_padding_range->_num, brs_get_file_padding_pos, &range_first_file_index, &range_last_file_index );
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS )
	{
		return BT_DATA_MANAGER_LOGIC_ERROR;
	}	
	sd_assert( range_first_file_index == range_last_file_index );
	if( range_first_file_index != range_last_file_index )
	{
		LOG_DEBUG("brs_padding_range_to_sub_file_range: range_first_file_index:%u, range_last_file_index:%u",
			range_first_file_index, range_last_file_index );	
		return BT_DATA_MANAGER_LOGIC_ERROR;
	}
	*p_file_index = range_first_file_index;
	p_file_range_info = &(p_bt_range_switch->_file_range_info_array[*p_file_index]);

	sd_assert( p_padding_range->_index >= p_file_range_info->_pos_p2sp_view );

	p_sub_file_range->_index = p_padding_range->_index - p_file_range_info->_pos_p2sp_view;
	
	p_sub_file_range->_num = p_padding_range->_num;
	
	LOG_DEBUG("brs_padding_range_to_sub_file_range: target padding_range:[%u, %u], file_index:%u, p_sub_file_range:[%u,%u]",
		p_padding_range->_index, p_padding_range->_num, *p_file_index, p_sub_file_range->_index, p_sub_file_range->_num );
	return SUCCESS;
}

//p_range不能跨文件!!!
//需要去掉!!!!!!!
_int32 brs_pipe_put_range_get_info( const BT_RANGE_SWITCH *p_bt_range_switch, const RANGE *p_padding_range, _u32 *p_file_index, BOOL *p_is_tmp_file, RANGE *p_sub_file_range, LIST *p_piece_info_list )
{
	PIECE_RANGE_INFO *p_piece_range_info = NULL;
	_u32 range_first_file_index = 0;
	_u32 range_last_file_index = 0;
	_int32 ret_val = SUCCESS;
	FILE_RANGE_INFO *p_file_range_info = NULL;
	_u64 rang_bt_begin_pos = 0;
	_u64 rang_bt_end_pos = 0;
	_u32 begin_piece_index = 0;
	_u32 end_piece_index = 0;
	_u32 piece_index = 0;
	_u64 piece_begin_pos = 0;// piece头在全文件的位置(精确到bytes)
	_u64 piece_end_pos = 0;// piece尾在全文件的位置(精确到bytes)
	_u64 piece_begin_sub_file_pos = 0;//piece头在子文件的位置(精确到bytes)
	_u64 piece_end_sub_file_pos = 0;//piece尾在子文件的位置(精确到bytes)
	_u32 piece_end_sub_range_pos = 0;//piece尾在子文件的位置(精确到range)
	

	sd_assert( p_bt_range_switch->_piece_num > 0 );

	LOG_DEBUG("brs_pipe_put_range_get_info: target padding_range:[%u, %u]",
		p_padding_range->_index, p_padding_range->_num );

	ret_val = brs_search_file_index( p_bt_range_switch, p_padding_range->_index, p_padding_range->_num, brs_get_file_padding_pos, &range_first_file_index, &range_last_file_index );
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS )
	{
		return BT_DATA_MANAGER_LOGIC_ERROR;
	}	
	sd_assert( range_first_file_index == range_last_file_index );
	if( range_first_file_index != range_last_file_index )
	{
		LOG_DEBUG("brs_pipe_put_range_get_info: range_first_file_index:%u, range_last_file_index:%u",
			range_first_file_index, range_last_file_index );	
		return BT_DATA_MANAGER_LOGIC_ERROR;
	}
	*p_file_index = range_first_file_index;
	p_file_range_info = &(p_bt_range_switch->_file_range_info_array[*p_file_index]);
	*p_is_tmp_file = !p_file_range_info->_is_need_download;

	sd_assert( p_padding_range->_index >= p_file_range_info->_pos_p2sp_view );

	p_sub_file_range->_index = p_padding_range->_index - p_file_range_info->_pos_p2sp_view;
	
	p_sub_file_range->_num = p_padding_range->_num;
	

	rang_bt_begin_pos = p_file_range_info->_file_offset + (_u64)p_sub_file_range->_index * get_data_unit_size();
	rang_bt_end_pos = rang_bt_begin_pos + (_u64)p_padding_range->_num * get_data_unit_size() - 1;

	begin_piece_index = rang_bt_begin_pos / p_bt_range_switch->_piece_size;
	end_piece_index = rang_bt_end_pos  / p_bt_range_switch->_piece_size;

	LOG_DEBUG("brs_pipe_put_range_get_info: target padding_range:[%u, %u], file_index:%u, p_sub_file_range:[%u,%u], begin_piece_index:%u, end_piece_index:%u",
		p_padding_range->_index, p_padding_range->_num, *p_file_index, p_sub_file_range->_index, p_sub_file_range->_num,
		begin_piece_index, end_piece_index );

	//挑选和p_range在同一文件中的piece
	for( piece_index = begin_piece_index; piece_index <= end_piece_index && piece_index < p_bt_range_switch->_piece_num; piece_index++ )
	{
		
		piece_begin_pos = (_u64)piece_index * p_bt_range_switch->_piece_size;
 		LOG_DEBUG("brs_pipe_put_range_get_info: piece_index:%u", piece_index );
		
		if( piece_index == p_bt_range_switch->_piece_num - 1
			&& piece_begin_pos < p_file_range_info->_file_offset )
		{
			LOG_DEBUG("brs_pipe_put_range_get_info: invalid last piece:%u, piece_begin_pos:%llu, file_offset:%llu", 
				piece_index, piece_begin_pos, p_file_range_info->_file_offset );
			continue;
		}
		else if( piece_begin_pos < p_file_range_info->_file_offset
			|| piece_begin_pos + p_bt_range_switch->_piece_size > ( p_file_range_info->_file_offset + p_file_range_info->_file_size ) )
		{
			LOG_DEBUG("brs_pipe_put_range_get_info: invalid piece:%u, piece_begin_pos:%llu, file_offset:%llu, file_size:%llu", 
				piece_index, piece_begin_pos, p_file_range_info->_file_offset, p_file_range_info->_file_size );
			continue;
		}
		ret_val = piece_range_info_malloc_wrap( &p_piece_range_info );
		CHECK_VALUE( ret_val );
		p_piece_range_info->_piece_index = piece_index;
		p_piece_range_info->_file_index = *p_file_index;

		sd_assert( piece_begin_pos >= p_file_range_info->_file_offset );
		
		piece_begin_sub_file_pos = piece_begin_pos - p_file_range_info->_file_offset;
		p_piece_range_info->_padding_range._index = piece_begin_sub_file_pos / get_data_unit_size();

		if( piece_index == p_bt_range_switch->_piece_num - 1 )
		{
			piece_end_pos = p_bt_range_switch->_file_size - 1;
			p_piece_range_info->_padding_exact_range._length = p_bt_range_switch->_last_piece_size;
		}
		else 
		{
			piece_end_pos = (_u64)( piece_index + 1 ) * p_bt_range_switch->_piece_size - 1;
			p_piece_range_info->_padding_exact_range._length = p_bt_range_switch->_piece_size;
		}
		
		sd_assert( piece_end_pos > p_file_range_info->_file_offset );
		piece_end_sub_file_pos = piece_end_pos - p_file_range_info->_file_offset;
		piece_end_sub_range_pos = piece_end_sub_file_pos / get_data_unit_size();

		sd_assert( piece_end_sub_range_pos >= p_piece_range_info->_padding_range._index );
		p_piece_range_info->_padding_range._num = piece_end_sub_range_pos - p_piece_range_info->_padding_range._index + 1;

		p_piece_range_info->_padding_exact_range._pos = piece_begin_sub_file_pos;

		LOG_DEBUG("brs_pipe_put_range_get_info: piece index:%u:[%llu,%llu,%llu], _padding_range:[%u,%u,%u], padding_exact_range:[%llu,%llu,%llu]", 
			piece_index, piece_begin_pos, piece_end_pos - piece_begin_pos, piece_end_pos,
			p_piece_range_info->_padding_range._index, p_piece_range_info->_padding_range._num, p_piece_range_info->_padding_range._index + p_piece_range_info->_padding_range._num,
			p_piece_range_info->_padding_exact_range._pos, p_piece_range_info->_padding_exact_range._length, p_piece_range_info->_padding_exact_range._pos + p_piece_range_info->_padding_exact_range._length
			);
		list_push( p_piece_info_list, (void *)p_piece_range_info );

	}

	return SUCCESS;
}


void brs_release_piece_range_info_list( LIST *p_piece_info_list )
{
	LIST_ITERATOR cur_node = LIST_BEGIN( *p_piece_info_list );
	PIECE_RANGE_INFO *p_piece_range_info = NULL;
	
	LOG_DEBUG("brs_release_piece_range_info_list: p_piece_info_list:0x%x", p_piece_info_list );
	while( cur_node != LIST_END( *p_piece_info_list ) )
	{
		p_piece_range_info = ( PIECE_RANGE_INFO * )LIST_VALUE( cur_node );
		piece_range_info_free_wrap( p_piece_range_info );
		cur_node = LIST_NEXT( cur_node );
	}
	list_clear( p_piece_info_list );
}

//转换成包含bt_range的最小range
_int32 brs_bt_range_to_read_range_info_list( const BT_RANGE_SWITCH *p_bt_range_switch, BT_RANGE *p_bt_range, LIST *p_read_range_info_list )
{
	_u32 range_first_file_index = 0;
	_u32 range_last_file_index = 0;
	_u32 file_index = 0;
	_int32 ret_val = SUCCESS;
	_u64 file_pos_bt_view = 0;
	_u32 file_pos_p2sp_view = 0;
	//_u64 last_file_pos_bt_view = 0;
	//_u32 last_file_pos_p2sp_view = 0;
	READ_RANGE_INFO *p_read_range_info = NULL;
	_u64 padding_range_sub_file_offset = 0;
	
	_u64 padding_range_end_sub_file_pos = 0;
	_u64 bt_range_pos = p_bt_range->_pos;
	
	_u64 file_size = 0;
	
#ifdef _LOGGER
	_u32 file_range_num = 0;
#endif

	ret_val = brs_search_file_index( p_bt_range_switch, p_bt_range->_pos, p_bt_range->_length, brs_get_file_bt_pos, &range_first_file_index, &range_last_file_index );
	sd_assert( ret_val == SUCCESS );
	if( ret_val != SUCCESS )
	{
		return BT_DATA_MANAGER_LOGIC_ERROR;
	}	

	LOG_DEBUG("brs_bt_range_to_read_range_info_list begin:bt_range:[%llu,%llu] p_read_range_info_list:0x%x, range_first_file_index:%u,range_last_file_index:%u", 
		p_bt_range->_pos, p_bt_range->_length, p_read_range_info_list, range_first_file_index, range_last_file_index );

	sd_assert( p_bt_range->_pos + p_bt_range->_length <= p_bt_range_switch->_file_size );
	if( p_bt_range->_pos + p_bt_range->_length > p_bt_range_switch->_file_size )
	{
		return BT_RANGE_TOO_BIG;
	}
	
	for( file_index = range_first_file_index; file_index <= range_last_file_index; file_index++ )
	{
		file_pos_bt_view = p_bt_range_switch->_file_range_info_array[file_index]._file_offset;
		file_pos_p2sp_view = p_bt_range_switch->_file_range_info_array[file_index]._pos_p2sp_view;
		file_size = p_bt_range_switch->_file_range_info_array[file_index]._file_size;
		if( file_size == 0 ) continue;

		ret_val = read_range_info_malloc_wrap( &p_read_range_info );
		if( ret_val != SUCCESS )
		{
			break;
		}
				
		p_read_range_info->_file_index = file_index;
		sd_assert( bt_range_pos >= file_pos_bt_view );

		padding_range_sub_file_offset = MAX( file_pos_bt_view, bt_range_pos ) - file_pos_bt_view;
		p_read_range_info->_padding_range._index = padding_range_sub_file_offset / get_data_unit_size();

		p_read_range_info->_padding_exact_range._pos = bt_range_pos - file_pos_bt_view;
		
		sd_assert( p_bt_range->_pos + p_bt_range->_length > bt_range_pos );
		sd_assert( file_pos_bt_view + file_size > bt_range_pos );
		p_read_range_info->_padding_exact_range._length = MIN( p_bt_range->_pos + p_bt_range->_length - bt_range_pos, file_pos_bt_view + file_size - bt_range_pos );

		sd_assert( p_read_range_info->_padding_exact_range._length > 0 );
		padding_range_end_sub_file_pos = ( bt_range_pos + p_read_range_info->_padding_exact_range._length - file_pos_bt_view - 1 ) / get_data_unit_size();

		sd_assert( padding_range_end_sub_file_pos >= p_read_range_info->_padding_range._index );
		p_read_range_info->_padding_range._num = padding_range_end_sub_file_pos - p_read_range_info->_padding_range._index + 1;

		bt_range_pos += p_read_range_info->_padding_exact_range._length;

		LOG_DEBUG("brs_bt_range_to_read_range_info_list: file_index:%u, file_pos_p2sp_view:%u: file_range_num:%u, exact_range_length:%llu, exact_range_pos:%llu ", 
			file_index, file_pos_p2sp_view, p_bt_range_switch->_file_range_info_array[file_index]._file_range_num,
			p_read_range_info->_padding_exact_range._length, p_read_range_info->_padding_exact_range._pos );


		if( ( p_read_range_info->_padding_range._index + p_read_range_info->_padding_range._num )== p_bt_range_switch->_file_range_info_array[file_index]._file_range_num )
		{
			sd_assert( file_size > (_u64)p_read_range_info->_padding_range._index * get_data_unit_size() );
			p_read_range_info->_range_length = file_size - (_u64)p_read_range_info->_padding_range._index * get_data_unit_size();
		}
		else
		{
			p_read_range_info->_range_length = (_u64)p_read_range_info->_padding_range._num * get_data_unit_size();
		}
		
		if( p_bt_range_switch->_file_range_info_array[file_index]._is_need_download )
		{
			p_read_range_info->_is_tmp_file = FALSE;
		}
		else
		{
			p_read_range_info->_is_tmp_file = TRUE;
		}
		
#ifdef _LOGGER
		file_range_num = p_bt_range_switch->_file_range_info_array[file_index]._file_range_num;

		LOG_DEBUG("brs_bt_range_to_read_range_info_list: file_index:%u: file_bt_range:[%llu,%llu,%llu],file_padding_range:[%u,%u,%u], _padding_range:[%u,%u,%u], padding_exact_range:[%llu,%llu,%llu], range_length:%u, bt_range_pos:%llu, _is_tmp_file:%d", 
			file_index, 
			file_pos_bt_view, file_size, file_pos_bt_view + file_size,
			file_pos_p2sp_view, file_range_num, file_pos_p2sp_view + file_range_num,
			p_read_range_info->_padding_range._index, p_read_range_info->_padding_range._num, p_read_range_info->_padding_range._index + p_read_range_info->_padding_range._num,
			p_read_range_info->_padding_exact_range._pos, p_read_range_info->_padding_exact_range._length, p_read_range_info->_padding_exact_range._pos + p_read_range_info->_padding_exact_range._length,
			p_read_range_info->_range_length, bt_range_pos,
			p_read_range_info->_is_tmp_file );
#endif
		list_push( p_read_range_info_list, (void *)p_read_range_info );
	}
	
	if( ret_val != SUCCESS )
	{
		brs_release_read_range_info_list( p_read_range_info_list );
	}

	return ret_val;
}

//需要去掉!!!!!!!
_int32 brs_piece_to_range_info_list( const BT_RANGE_SWITCH *p_bt_range_switch, _u32 piece_index, RANGE *p_padding_range, LIST *p_read_range_info_list )
{
	BT_RANGE bt_range;
	_int32 ret_val = SUCCESS;
	READ_RANGE_INFO *p_read_range_info = NULL;
	LIST_ITERATOR cur_node = NULL;
	bt_range._pos = (_u64) piece_index * p_bt_range_switch->_piece_size;
	if( piece_index != p_bt_range_switch->_piece_num - 1 )
	{
		bt_range._length = p_bt_range_switch->_piece_size;
	}
	else
	{
		bt_range._length = p_bt_range_switch->_last_piece_size;
	}
	ret_val = brs_bt_range_to_read_range_info_list( p_bt_range_switch, &bt_range, p_read_range_info_list);
	CHECK_VALUE( ret_val );

	cur_node = LIST_BEGIN( *p_read_range_info_list );
	p_read_range_info = ( READ_RANGE_INFO *)LIST_VALUE( cur_node );
	p_padding_range->_index = p_bt_range_switch->_file_range_info_array[ p_read_range_info->_file_index ]._pos_p2sp_view;
	p_padding_range->_index += p_read_range_info->_padding_range._index;

	while( cur_node != LIST_END( *p_read_range_info_list ) )
	{
		p_read_range_info = ( READ_RANGE_INFO *)LIST_VALUE( cur_node );
		p_padding_range->_num += p_read_range_info->_padding_range._num;
		cur_node = LIST_NEXT( cur_node );
	}
	return SUCCESS;
	
}

void brs_release_read_range_info_list( LIST *p_read_range_info_list )
{
	LIST_ITERATOR cur_node = LIST_BEGIN( *p_read_range_info_list );
	READ_RANGE_INFO *p_read_range_info = NULL;
	
	LOG_DEBUG("*********brs_release_read_range_info_list: p_read_range_info_list:0x%x", p_read_range_info_list );
	while( cur_node != LIST_END( *p_read_range_info_list ) )
	{
		p_read_range_info = ( READ_RANGE_INFO * )LIST_VALUE( cur_node );
		brs_release_read_range_info( p_read_range_info );
		cur_node = LIST_NEXT( cur_node );
	}
	list_clear( p_read_range_info_list );
	
}

void brs_release_read_range_info( READ_RANGE_INFO *p_read_range_info )
{
	LOG_DEBUG("brs_release_read_range_info: p_read_range_info:0x%x", p_read_range_info );
	read_range_info_free_wrap( p_read_range_info );
}

_int32 brs_get_padding_range_from_file_index( const BT_RANGE_SWITCH *p_bt_range_switch, _u32 file_index, RANGE *p_range )
{
	if( file_index >= p_bt_range_switch->_file_num ) return BT_DATA_MANAGER_LOGIC_ERROR;
	
	p_range->_index = p_bt_range_switch->_file_range_info_array[file_index]._pos_p2sp_view;
	p_range->_num = p_bt_range_switch->_file_range_info_array[file_index]._file_range_num;
	LOG_DEBUG("brs_get_padding_range_from_file_index: file_index:%u, file_range:[%u,%u]", 
		file_index, p_range->_index, p_range->_num );
	return SUCCESS;
}

_int32 brs_get_inner_piece_range_from_file_index( const BT_RANGE_SWITCH *p_bt_range_switch, _u32 file_index, _u32 *p_begin_piece_index, _u32 *p_end_piece_index )
{
	_u64 file_begin_bt_pos = 0;
	_u64 file_end_bt_pos = 0;
	_u64 file_size = 0;
	_u32 begin_piece_index = 0;
	_u32 end_piece_index = 0;
	
	LOG_DEBUG("brs_get_inner_piece_range_from_file_index: file_index:%u", 
		file_index );	
	
	if( file_index >= p_bt_range_switch->_file_num ) return BT_DATA_MANAGER_LOGIC_ERROR;

	file_begin_bt_pos = p_bt_range_switch->_file_range_info_array[file_index]._file_offset;

	sd_assert( p_bt_range_switch->_piece_size != 0 );
	if( p_bt_range_switch->_piece_size == 0 ) return BT_DATA_MANAGER_LOGIC_ERROR;

	file_size = p_bt_range_switch->_file_range_info_array[file_index]._file_size;
	if( file_size == 0 )
	{
		return BT_FILE_TOO_SMALL;
	}

	begin_piece_index = file_begin_bt_pos / p_bt_range_switch->_piece_size;

	if( file_begin_bt_pos % p_bt_range_switch->_piece_size != 0 ) 
	{
		begin_piece_index++;
	}

	file_end_bt_pos = file_begin_bt_pos + p_bt_range_switch->_file_range_info_array[file_index]._file_size -1;
	
	end_piece_index = file_end_bt_pos  / p_bt_range_switch->_piece_size;

	if( end_piece_index == 0 && ( file_end_bt_pos + 1 ) != p_bt_range_switch->_piece_size )
	{
		return BT_FILE_TOO_SMALL;
	}
	if( ( file_end_bt_pos + 1 ) % p_bt_range_switch->_piece_size != 0 ) 
	{
		sd_assert( end_piece_index > 0 );
		if( end_piece_index == 0 ) return BT_DATA_MANAGER_LOGIC_ERROR;
		end_piece_index--;
	}
	if( begin_piece_index > end_piece_index )
	{
		return BT_FILE_TOO_SMALL;
	}

	*p_begin_piece_index = begin_piece_index;
	*p_end_piece_index = end_piece_index;
	
	LOG_DEBUG("brs_get_inner_piece_range_from_file_index: file_index:%u, begin_piece:%u, end_piece:%u", 
		file_index, *p_begin_piece_index, *p_end_piece_index );	

	return SUCCESS;
}

_int32 brs_get_piece_range_from_file_index( const BT_RANGE_SWITCH *p_bt_range_switch, _u32 file_index, _u32 *p_begin_piece_index, _u32 *p_end_piece_index )
{
	_u64 file_begin_bt_pos = 0;
	_u64 file_end_bt_pos = 0;
	_u64 file_size = 0;
	
	if( file_index >= p_bt_range_switch->_file_num ) return BT_DATA_MANAGER_LOGIC_ERROR;

	file_begin_bt_pos = p_bt_range_switch->_file_range_info_array[file_index]._file_offset;

	sd_assert( p_bt_range_switch->_piece_size != 0 );
	if( p_bt_range_switch->_piece_size == 0 ) return BT_DATA_MANAGER_LOGIC_ERROR;

	file_size = p_bt_range_switch->_file_range_info_array[file_index]._file_size;
	if( file_size == 0 )
	{
		return BT_FILE_TOO_SMALL;
	}

	*p_begin_piece_index = file_begin_bt_pos / p_bt_range_switch->_piece_size ;

	file_end_bt_pos = file_begin_bt_pos + p_bt_range_switch->_file_range_info_array[file_index]._file_size -1 ;
	
	*p_end_piece_index = file_end_bt_pos  / p_bt_range_switch->_piece_size ;
	return SUCCESS;
}

_u32 brs_get_piece_num( const BT_RANGE_SWITCH *p_bt_range_switch )
{
	LOG_DEBUG( "brs_get_piece_num:%d", p_bt_range_switch->_piece_num );
	return p_bt_range_switch->_piece_num;
}

_u32 brs_get_piece_size( const BT_RANGE_SWITCH *p_bt_range_switch )
{
	
	LOG_DEBUG( "brs_get_piece_size:%d", p_bt_range_switch->_piece_size );
	return p_bt_range_switch->_piece_size;
}

_u32 brs_get_target_piece_size( const BT_RANGE_SWITCH *p_bt_range_switch, _u32 piece_index )
{
	_u32 piece_size = 0;
	sd_assert( p_bt_range_switch->_piece_num > 0 );
	if( piece_index != p_bt_range_switch->_piece_num - 1 )
	{
		piece_size = p_bt_range_switch->_piece_size;
	}
	else
	{
		piece_size = p_bt_range_switch->_last_piece_size;
	}
	LOG_DEBUG( "brs_get_target_piece_size:%d", piece_size );
	return piece_size;
}

_int32 brs_sub_file_exact_range_to_bt_range( const BT_RANGE_SWITCH *p_bt_range_switch, _u32 file_index, const EXACT_RANGE *p_exact_range, BT_RANGE *p_bt_range )
{
	sd_assert( file_index < p_bt_range_switch->_file_num );
	if( file_index >= p_bt_range_switch->_file_num ) return BT_DATA_MANAGER_LOGIC_ERROR;
	p_bt_range->_pos = p_bt_range_switch->_file_range_info_array[file_index]._file_offset + p_exact_range->_pos;
	p_bt_range->_length = p_exact_range->_length;
	return SUCCESS;
}

_int32 brs_get_file_p2sp_pos( const BT_RANGE_SWITCH *p_bt_range_switch, _u32 file_index, _u32 *p_p2sp_pos )
{
	sd_assert( file_index < p_bt_range_switch->_file_num );
	if( file_index >= p_bt_range_switch->_file_num ) return BT_ERROR_FILE_INDEX;
	*p_p2sp_pos = p_bt_range_switch->_file_range_info_array[file_index]._pos_p2sp_view;
	return SUCCESS;
}

_int32 brs_get_file_size( const BT_RANGE_SWITCH *p_bt_range_switch, _u32 file_index, _u64 *p_file_size )
{
	sd_assert( file_index < p_bt_range_switch->_file_num );
	if( file_index >= p_bt_range_switch->_file_num ) return BT_ERROR_FILE_INDEX;
	*p_file_size = p_bt_range_switch->_file_range_info_array[file_index]._file_size;
	return SUCCESS;
}


//piece_range_info malloc/free
_int32 init_piece_range_info_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_piece_range_info_slab == NULL )
	{              
		ret_val = mpool_create_slab( sizeof( PIECE_RANGE_INFO ), MIN_PIECE_RANGE_INFO, 0, &g_piece_range_info_slab );
		CHECK_VALUE( ret_val );
	}
	return ret_val;
}

_int32 uninit_piece_range_info_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_piece_range_info_slab != NULL )
	{              
		ret_val = mpool_destory_slab( g_piece_range_info_slab );
		CHECK_VALUE( ret_val );
		g_piece_range_info_slab = NULL;
	}
	return ret_val;
}

_int32 piece_range_info_malloc_wrap(struct tagPIECE_RANGE_INFO **pp_node)
{
	return mpool_get_slip( g_piece_range_info_slab, (void**)pp_node );
}

_int32 piece_range_info_free_wrap(struct tagPIECE_RANGE_INFO *p_node)
{
	sd_assert( p_node != NULL );
	if( p_node == NULL ) return SUCCESS;
	return mpool_free_slip( g_piece_range_info_slab, (void*)p_node );
}


//read_range_info malloc/free
_int32 init_read_range_info_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_read_range_info_slab == NULL )
	{              
		ret_val = mpool_create_slab( sizeof( READ_RANGE_INFO ), MIN_READ_RANGE_INFO, 0, &g_read_range_info_slab );
		CHECK_VALUE( ret_val );
	}
	return ret_val;
}

_int32 uninit_read_range_info_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_read_range_info_slab != NULL )
	{              
		ret_val = mpool_destory_slab( g_read_range_info_slab );
		CHECK_VALUE( ret_val );
		g_read_range_info_slab = NULL;
	}
	return ret_val;
}

_int32 read_range_info_malloc_wrap(struct tagREAD_RANGE_INFO **pp_node)
{
	return mpool_get_slip( g_read_range_info_slab, (void**)pp_node );
}

_int32 read_range_info_free_wrap(struct tagREAD_RANGE_INFO *p_node)
{
	sd_assert( p_node != NULL );
	if( p_node == NULL ) return SUCCESS;
	return mpool_free_slip( g_read_range_info_slab, (void*)p_node );
}

//sub_file_padding_range_info malloc/free
_int32 init_sub_file_padding_range_info_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_sub_file_padding_range_info_slab == NULL )
	{              
		ret_val = mpool_create_slab( sizeof( SUB_FILE_PADDING_RANGE_INFO ), MIN_SUB_FILE_PADDING_RANGE_INFO, 0, &g_sub_file_padding_range_info_slab );
		CHECK_VALUE( ret_val );
	}
	return ret_val;
}

_int32 uninit_sub_file_padding_range_info_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_sub_file_padding_range_info_slab != NULL )
	{              
		ret_val = mpool_destory_slab( g_sub_file_padding_range_info_slab );
		CHECK_VALUE( ret_val );
		g_sub_file_padding_range_info_slab = NULL;
	}
	return ret_val;
}

_int32 sub_file_padding_range_info_malloc_wrap(struct tagSUB_FILE_PADDING_RANGE_INFO **pp_node)
{
	return mpool_get_slip( g_sub_file_padding_range_info_slab, (void**)pp_node );
}

_int32 sub_file_padding_range_info_free_wrap(struct tagSUB_FILE_PADDING_RANGE_INFO *p_node)
{
	sd_assert( p_node != NULL );
	if( p_node == NULL ) return SUCCESS;
	return mpool_free_slip( g_sub_file_padding_range_info_slab, (void*)p_node );
}

#endif /* #ifdef ENABLE_BT */

