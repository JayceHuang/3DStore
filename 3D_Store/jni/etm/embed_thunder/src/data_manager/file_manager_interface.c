#include "utility/define.h"
#include "utility/logid.h"
#define LOGID LOGID_FILE_MANAGER
#include "utility/logger.h"
#include "platform/sd_fs.h"

#include "file_manager_interface.h"
#include "file_manager_imp.h"
#include "file_manager_interface_xl.h"

_int32 init_file_manager_module(void)
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "init_file_manager_module." );
	
	ret_val = init_tmp_file_slabs();
	CHECK_VALUE( ret_val );

	ret_val = init_msg_file_create_para_slabs();	
	CHECK_VALUE( ret_val );

	ret_val = init_msg_file_rw_para_slabs();
	CHECK_VALUE( ret_val );

	ret_val = init_msg_file_close_para_slabs();
	CHECK_VALUE( ret_val );

	ret_val = init_block_data_buffer_slabs();
	CHECK_VALUE( ret_val );
	
	ret_val = init_range_data_buffer_list_slabs();
	CHECK_VALUE( ret_val );

	ret_val = init_file_manager_module_xl();
	CHECK_VALUE( ret_val );
	return ret_val;
}

_int32 uninit_file_manager_module(void)
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "uninit_file_manager_module." );

	uninit_file_manager_module_xl();
	
	ret_val = uninit_tmp_file_slabs();
	CHECK_VALUE( ret_val );

	ret_val = uninit_msg_file_create_para_slabs();	
	CHECK_VALUE( ret_val );

	ret_val = uninit_msg_file_rw_para_slabs();
	CHECK_VALUE( ret_val );

	ret_val = uninit_msg_file_close_para_slabs();
	CHECK_VALUE( ret_val );

	ret_val = uninit_block_data_buffer_slabs();
	CHECK_VALUE( ret_val );
	
	ret_val = uninit_range_data_buffer_list_slabs();
	CHECK_VALUE( ret_val );
	return ret_val;
}

_int32 fm_create_file_struct( const char *p_file_name, const char *p_file_path, _u64 file_size, void *p_user, notify_file_create p_call_back, TMP_FILE **pp_file_struct ,_u32 write_mode)
{
	//check argument
	_int32 ret_val = SUCCESS;

	if( p_file_name == NULL || p_file_path == NULL || p_user == NULL || p_call_back == NULL )
		return INVALID_ARGUMENT;
	LOG_DEBUG( "fm_create_file_struct. write_mode = %d", write_mode );

	if((FILE_WRITE_MODE)write_mode == FWM_RANGE)
	{
		ret_val =  fm_create_file_struct_xl( p_file_name,p_file_path,  file_size, p_user,  p_call_back,pp_file_struct,write_mode );
		CHECK_VALUE( ret_val );
	}
	else
	{
		ret_val = fm_create_and_init_struct( p_file_name, p_file_path, file_size, pp_file_struct);
		CHECK_VALUE( ret_val );
		(*pp_file_struct)->_write_mode = write_mode;
		ret_val = fm_handle_create_file( *pp_file_struct, p_user, p_call_back );
		CHECK_VALUE( ret_val );
	}
	return SUCCESS;
}

_int32 fm_file_write_buffer_list( struct tagTMP_FILE *p_file_struct
        , RANGE_DATA_BUFFER_LIST *p_buffer_list
        , notify_write_result p_call_back
        , void *p_user )
{
	_int32 ret_val = SUCCESS;	
	RANGE_DATA_BUFFER_LIST *p_tmp_buffer_list = NULL;
	LIST_ITERATOR cur_node, rbegin_node;
	BLOCK_DATA_BUFFER *p_block_data_buffer = NULL;
	_u32 write_list_size = 0;
	LOG_DEBUG( "fm_file_write_buffer_list. buffer list ptr:0x%x, call_back_ptr:0x%x, user_ptr:0x%x ", 
		p_buffer_list, p_call_back, p_user);

	if(p_file_struct->_write_mode == FWM_RANGE)
	{
		ret_val =  fm_file_write_buffer_list_xl(p_file_struct, p_buffer_list,  p_call_back, p_user);
		CHECK_VALUE( ret_val );
	}
	else
	{
		ret_val = range_data_buffer_list_malloc_wrap( &p_tmp_buffer_list );
		CHECK_VALUE( ret_val );
		list_init( &p_tmp_buffer_list->_data_list ); 
		list_swap( &p_buffer_list->_data_list, &p_tmp_buffer_list->_data_list);
		
		cur_node = LIST_BEGIN( p_tmp_buffer_list->_data_list );
		sd_assert( cur_node != NULL );

		if( p_file_struct->_block_size == 0 && p_file_struct->_is_order_mode )
		{
			p_file_struct->_block_size = get_data_unit_size() * 8;
		}
		
		while( cur_node != LIST_END( p_tmp_buffer_list->_data_list ) )
		{
			ret_val = fm_generate_block_list( p_file_struct
			        , (RANGE_DATA_BUFFER *)LIST_VALUE(cur_node)
			        , FM_OP_BLOCK_WRITE
			        , p_tmp_buffer_list
			        , p_call_back
			        , p_user );
			CHECK_VALUE( ret_val );	
			cur_node = LIST_NEXT( cur_node );	
		}

		rbegin_node = LIST_RBEGIN( p_file_struct->_write_block_list );
		p_block_data_buffer = LIST_VALUE( rbegin_node );
		p_block_data_buffer->_is_call_back = TRUE;

		write_list_size = list_size( &p_file_struct->_write_block_list );
		LOG_DEBUG( "fm_file_write_buffer_list. _write_block_list size:%d ", write_list_size );	
		if( write_list_size == 0 || p_file_struct->_is_writing == FALSE )
		{
			p_file_struct->_is_writing = TRUE;	
			if( !p_file_struct->_is_order_mode )
			{
				LOG_DEBUG( "fm_file_write_buffer_list in order_mode " );
				ret_val = fm_handle_write_block_list( p_file_struct );
			}
			else
			{
				LOG_DEBUG( "fm_file_write_buffer_list in order_mode " );
				ret_val = fm_handle_order_write_block_list( p_file_struct );
			}
			CHECK_VALUE( ret_val );		
		}
	}
	return SUCCESS;
}

_int32 fm_file_asyn_read_buffer( struct tagTMP_FILE *p_file_struct, RANGE_DATA_BUFFER *p_data_buffer, notify_read_result p_call_back, void *p_user )
{
	_int32 ret_val = SUCCESS;
	LIST_ITERATOR rbegin_node;
	BLOCK_DATA_BUFFER *p_block_data_buffer = NULL;
	_u32 asyn_read_list_size = 0;
	LOG_DEBUG( "fm_file_asyn_read_buffer. data_buffer ptr:0x%x, call_back_ptr:0x%x, user_ptr:0x%x ", 
		p_data_buffer, p_call_back, p_user);
		
	if(p_file_struct->_write_mode == FWM_RANGE)
	{
		ret_val =  fm_file_asyn_read_buffer_xl(p_file_struct,p_data_buffer, p_call_back,p_user);
		CHECK_VALUE( ret_val );
	}
	else
	{
		ret_val = fm_generate_block_list( p_file_struct, p_data_buffer, FM_OP_BLOCK_NORMAL_READ, p_data_buffer, p_call_back, p_user );
		CHECK_VALUE( ret_val );

		rbegin_node = LIST_RBEGIN( p_file_struct->_asyn_read_block_list );
		p_block_data_buffer = LIST_VALUE( rbegin_node );
		p_block_data_buffer->_is_call_back = TRUE;
		
		asyn_read_list_size = list_size( &p_file_struct->_asyn_read_block_list );
		if( asyn_read_list_size == 0 || p_file_struct->_is_asyn_reading == FALSE )
		{
			p_file_struct->_is_asyn_reading = TRUE;
			ret_val = fm_handle_asyn_read_block_list( p_file_struct );
			CHECK_VALUE( ret_val );  //zyq remark!!!!
		}
	}
	return SUCCESS;
}

_int32  fm_file_syn_read_buffer( struct tagTMP_FILE *p_file_struct, RANGE_DATA_BUFFER *p_data_buffer )
{
	_int32 ret_val = SUCCESS;
	_u32 syn_read_list_size = 0;
	
	LOG_DEBUG( "fm_file_syn_read_buffer. data_buffer ptr:0x%x. ", p_data_buffer );	
	if(p_file_struct->_write_mode == FWM_RANGE)
	{
		ret_val =  fm_file_syn_read_buffer_xl(p_file_struct,p_data_buffer);
		//CHECK_VALUE( ret_val );
		if(ret_val != SUCCESS)
		    return ret_val;
	}
	else
	{
    	ret_val = fm_generate_block_list( p_file_struct, p_data_buffer, FM_OP_BLOCK_SYN_READ, NULL, NULL, NULL );
    	CHECK_VALUE( ret_val );

    	ret_val = fm_handle_syn_read_block_list( p_file_struct );
    	CHECK_VALUE( ret_val );	
    	
    	syn_read_list_size = list_size( &p_file_struct->_syn_read_block_list );
    	sd_assert( syn_read_list_size == 0 );
	}
	return SUCCESS;
}

_int32 fm_close( struct tagTMP_FILE *p_file_struct , notify_file_close p_call_back, void *p_user )
{
	_int32 ret_val =SUCCESS;
	LOG_DEBUG( "fm_close." );	

	if(p_file_struct->_write_mode == FWM_RANGE)
	{
		ret_val =  fm_close_xl( p_file_struct ,  p_call_back, p_user );
		CHECK_VALUE( ret_val );
	}
	else
	{
	p_file_struct->_is_closing = TRUE;
	ret_val = msg_file_close_para_malloc_wrap( &p_file_struct->_close_para_ptr );
	CHECK_VALUE( ret_val );

	p_file_struct->_close_para_ptr->_file_struct_ptr = p_file_struct;
	p_file_struct->_close_para_ptr->_user_ptr = p_user;
	p_file_struct->_close_para_ptr->_callback_func_ptr = p_call_back;

	ret_val = fm_handle_close_file( p_file_struct );
	CHECK_VALUE( ret_val );
	}
	return SUCCESS;
}

_int32 fm_file_change_name( struct tagTMP_FILE *p_file_struct, const char *p_new_file_name )
{
    _int32 ret_val = SUCCESS;
	_int32 new_file_len = 0;

	if(p_file_struct->_write_mode == FWM_RANGE)
	{
		ret_val =  fm_file_change_name_xl( p_file_struct, p_new_file_name );
		CHECK_VALUE( ret_val );
	}
	else
	{
	char old_full_name[MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN];
	char new_full_name[MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN];

	LOG_DEBUG( "fm_file_change_name. new file name:%s.", p_new_file_name );	

    ret_val = sd_memcpy( old_full_name,p_file_struct->_file_path, p_file_struct->_file_path_len );
	CHECK_VALUE( ret_val );

	ret_val = sd_memcpy( old_full_name + p_file_struct->_file_path_len ,p_file_struct->_file_name, p_file_struct->_file_name_len );
	CHECK_VALUE( ret_val );
	
    old_full_name[p_file_struct->_file_path_len + p_file_struct->_file_name_len] = '\0';

	new_file_len = sd_strlen( p_new_file_name);

	ret_val = sd_memcpy( new_full_name, p_file_struct->_file_path, p_file_struct->_file_path_len );
	CHECK_VALUE( ret_val );

	ret_val = sd_memcpy( new_full_name + p_file_struct->_file_path_len , p_new_file_name, new_file_len );
	CHECK_VALUE( ret_val );
	
    new_full_name[p_file_struct->_file_path_len + new_file_len] = '\0';
  
	LOG_DEBUG( "fm_file_change_name. old_full_name:%s ,new_full_file name:%s.", old_full_name,  new_full_name);	

	ret_val = sd_rename_file( old_full_name, new_full_name );
	CHECK_VALUE( ret_val );

	p_file_struct->_file_name_len = sd_strlen( p_new_file_name);
	ret_val = sd_strncpy( p_file_struct->_file_name, p_new_file_name, p_file_struct->_file_name_len );
	p_file_struct->_file_name[ p_file_struct->_file_name_len ] = '\0';
	}
	return SUCCESS;
}

_int32 fm_load_cfg_block_index_array( struct tagTMP_FILE *p_file_struct, _u32 cfg_file_id )
{
	_int32 ret_val = SUCCESS;
	_u32 read_len = 0, total_index_num = 0;
	DATA_BLOCK_INDEX_ARRAY *p_block_index_array = NULL;
	_u32 block_index = 0,i = 0;

	LOG_DEBUG( "fm_load_cfg_block_index_array." );	
	
	if(p_file_struct->_write_mode == FWM_RANGE)
	{
		ret_val =  fm_load_cfg_block_index_array_xl( p_file_struct,  cfg_file_id );
		CHECK_VALUE( ret_val );
	}
	else
	{
	sd_assert( p_file_struct != NULL );
    p_block_index_array = &p_file_struct->_block_index_array;
	ret_val = sd_read( cfg_file_id, (char *)&total_index_num, sizeof(_u32), &read_len );
	CHECK_VALUE( ret_val );
	ret_val = sd_read( cfg_file_id, (char *)(&p_block_index_array->_valid_index_num), sizeof(_u32), &read_len );
	CHECK_VALUE( ret_val );
	ret_val = sd_read( cfg_file_id, (char *)(&p_block_index_array->_extra_block_index), sizeof(_u32), &read_len );
	CHECK_VALUE( ret_val );
	ret_val = sd_read( cfg_file_id, (char *)(&p_block_index_array->_empty_block_index), sizeof(_u32), &read_len );
	CHECK_VALUE( ret_val );

	sd_assert( total_index_num == p_block_index_array->_total_index_num );
	if( total_index_num != p_block_index_array->_total_index_num )
	{
		LOG_ERROR( "fm cfg total index num error." );
		return FM_CFG_FILE_ERROR;
	}
		
	if( p_block_index_array->_valid_index_num > p_block_index_array->_total_index_num )
	{
		LOG_ERROR( "fm cfg _valid_index_num error." );
		sd_assert(FALSE);
		return FM_CFG_FILE_ERROR;
	}
		
	for( ; block_index < p_block_index_array->_total_index_num; block_index++ )
	{
		DATA_BLOCK_INDEX_ITEM *p_index_item = &p_block_index_array->_index_array_ptr[block_index];
		ret_val = sd_read( cfg_file_id, (char *)(&p_index_item->_logic_block_index), sizeof(_u32), &read_len );
		CHECK_VALUE( ret_val );
		ret_val = sd_read( cfg_file_id, (char *)(&p_index_item->_is_valid), sizeof(_u32), &read_len );
        	CHECK_VALUE( ret_val );
		if(p_index_item->_is_valid)
		{
			/* 加强校验 */
			for(i=0 ; i < block_index; i++ )
			{
				if( p_block_index_array->_index_array_ptr[i]._logic_block_index == p_index_item->_logic_block_index 
				&& p_block_index_array->_index_array_ptr[i]._is_valid  )
				{
					LOG_ERROR( "fm cfg _logic_block_index error:%d." ,p_index_item->_logic_block_index);
					sd_assert(FALSE);
					return FM_CFG_FILE_ERROR;
				}
			}
		}
        	LOG_DEBUG( "fm_load_cfg_block_index_array: disk_block_index: %u, logic_block_index:%u, is_valid: %u.",
        	block_index, p_index_item->_logic_block_index, p_index_item->_is_valid );
	}
	if( !fm_check_cfg_block_index_array( p_file_struct ) )
	{
		LOG_ERROR( "fm_check_cfg_block_index_array error." );
		return FM_CFG_FILE_ERROR;
	}	
	}
	return SUCCESS;
}

_int32 fm_flush_cfg_block_index_array( struct tagTMP_FILE *p_file_struct, _u32 cfg_file_id )
{
	_int32 ret_val = SUCCESS;
	if(p_file_struct->_write_mode == FWM_RANGE)
	{
		ret_val =  fm_flush_cfg_block_index_array_xl( p_file_struct,  cfg_file_id );
		CHECK_VALUE( ret_val );
	}
	else
	{
	_u32 write_len;
	DATA_BLOCK_INDEX_ARRAY *p_block_index_array = NULL;
	_u32 block_index = 0;
	char  buffer[1024];
	_u32 buffer_pos=0,buffer_len=1024;
	
	LOG_DEBUG( "fm_flush_cfg_block_index_array." );
	if( p_file_struct->_is_order_mode ) return SUCCESS;
	
	sd_assert( p_file_struct != NULL );
    p_block_index_array = &p_file_struct->_block_index_array;
	sd_assert(p_block_index_array->_total_index_num>=p_block_index_array->_valid_index_num);
	if(p_block_index_array->_total_index_num<p_block_index_array->_valid_index_num)
	{
		return FM_CFG_FILE_ERROR;
	}
	ret_val = sd_write_save_to_buffer(cfg_file_id, buffer, buffer_len,&buffer_pos,  (char *)(&p_block_index_array->_total_index_num), sizeof(_u32));
	CHECK_VALUE( ret_val );
	ret_val = sd_write_save_to_buffer(cfg_file_id, buffer, buffer_len,&buffer_pos,  (char *)(&p_block_index_array->_valid_index_num), sizeof(_u32));
	CHECK_VALUE( ret_val );
	ret_val = sd_write_save_to_buffer(cfg_file_id, buffer, buffer_len,&buffer_pos,  (char *)(&p_block_index_array->_extra_block_index), sizeof(_u32));
	CHECK_VALUE( ret_val );
	ret_val = sd_write_save_to_buffer(cfg_file_id, buffer, buffer_len,&buffer_pos,  (char *)(&p_block_index_array->_empty_block_index), sizeof(_u32));
	CHECK_VALUE( ret_val );
	for( ; block_index < p_block_index_array->_total_index_num; block_index++ )
	{
		DATA_BLOCK_INDEX_ITEM *p_index_item = &p_block_index_array->_index_array_ptr[block_index];
		ret_val = sd_write_save_to_buffer(cfg_file_id, buffer, buffer_len,&buffer_pos, (char *)(&p_index_item->_logic_block_index), sizeof(_u32));
		CHECK_VALUE( ret_val );
		ret_val = sd_write_save_to_buffer(cfg_file_id, buffer, buffer_len,&buffer_pos, (char *)(&p_index_item->_is_valid), sizeof(BOOL));
        CHECK_VALUE( ret_val );
        LOG_DEBUG( "fm_flush_cfg_block_index_array: disk_block_index: %u, logic_block_index:%u, is_valid: %u.",
        	block_index, p_index_item->_logic_block_index, p_index_item->_is_valid );
	}
	
	if(buffer_pos!=0)
	{
		ret_val =sd_write(cfg_file_id,buffer, buffer_pos, &write_len);
		CHECK_VALUE( ret_val );
		sd_assert(buffer_pos==write_len);
	}
	}
	return SUCCESS;
}

BOOL fm_file_is_created( struct tagTMP_FILE *p_file_struct )
{
	LOG_DEBUG( "fm_file_is_created. result:%u.", (_u32)p_file_struct->_is_file_created );
	if( p_file_struct->_is_file_created )
		return TRUE;
	else return FALSE;
}

BOOL fm_filesize_is_valid( struct tagTMP_FILE *p_file_struct )
{
	LOG_DEBUG( "fm_filesize_is_valid. result:%u.", (_u32)p_file_struct->_is_file_size_valid );
	if( p_file_struct->_is_file_size_valid )
		return TRUE;
	else return FALSE;
}


_int32 fm_init_file_info( struct tagTMP_FILE *p_file_struct, _u64 file_size, _u32 block_size )
{
	_int32 ret_val = SUCCESS;
	_int32 total_index_num = 0;
	DATA_BLOCK_INDEX_ITEM *p_block_index_arry = NULL;
	LOG_DEBUG( "fm_init_file_info. file_size:%llu, block_size:%u.p_file_struct->write_mode = %d"
        , file_size, block_size, p_file_struct->_write_mode);

	if(p_file_struct->_write_mode == FWM_RANGE)
	{
		ret_val =  fm_init_file_info_xl( p_file_struct,  file_size,  block_size  );
		CHECK_VALUE( ret_val );
	}
	else
	{
	if ( p_file_struct->_block_size != 0 )
	{
		if( p_file_struct->_block_size != block_size )
		{
			sd_assert( FALSE );
		}
		else
		{
			LOG_DEBUG( "fm_init_file_info again, do nothing." );
			return SUCCESS;
		}
	}
	
	p_file_struct->_file_size = file_size;
	p_file_struct->_block_size = block_size;
	total_index_num = ( file_size + block_size - 1 ) / block_size;

  	ret_val = sd_malloc( sizeof(DATA_BLOCK_INDEX_ITEM) * total_index_num, (void **)&(p_block_index_arry) );
	CHECK_VALUE( ret_val );
	
	ret_val = sd_memset( (void*)p_block_index_arry, 0, sizeof(DATA_BLOCK_INDEX_ITEM) * total_index_num );
	CHECK_VALUE( ret_val );
	
	p_file_struct->_block_index_array._index_array_ptr = p_block_index_arry;	
	p_file_struct->_block_index_array._total_index_num = total_index_num;
	p_file_struct->_block_index_array._valid_index_num = 0;
	p_file_struct->_block_index_array._extra_block_index= MAX_U32;
	p_file_struct->_block_index_array._empty_block_index= MAX_U32;
	p_file_struct->_is_order_mode = FALSE;	
	p_file_struct->_block_num = total_index_num;	
	p_file_struct->_last_block_size = file_size - block_size * (total_index_num - 1);	
	}
	return SUCCESS;
}

_int32 fm_change_file_size( struct tagTMP_FILE *p_file_struct, _u64 file_size, void *p_user, notify_file_create p_call_back )
{
	_int32 ret_val;
	LOG_DEBUG( "fm_change_file_size. file_size:%llu.", file_size );
	if( file_size == p_file_struct->_file_size ) return SUCCESS;
	if(p_file_struct->_write_mode == FWM_RANGE)
	{
		ret_val =  fm_change_file_size_xl( p_file_struct,  file_size, p_user,  p_call_back );
		CHECK_VALUE( ret_val );
	}
	else
	{
sd_assert(FALSE);
	}
	return SUCCESS;
}

