#include "utility/define.h"
//#ifdef XUNLEI_MODE

#include "file_manager_interface.h"
#include "file_manager_interface_xl.h"
#include "platform/sd_fs.h"
#include "utility/logid.h"
#define LOGID LOGID_FILE_MANAGER
#include "utility/logger.h"
#include "file_manager_imp_xl.h"

_int32 init_file_manager_module_xl()
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "init_file_manager_module." );

	ret_val = init_rw_data_buffer_slabs();
	CHECK_VALUE( ret_val );
	
	//ret_val = init_range_data_buffer_list_slabs();
	//CHECK_VALUE( ret_val );

	fm_init_setting();

	return ret_val;
}

_int32 uninit_file_manager_module_xl()
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "uninit_file_manager_module." );

	//ret_val = uninit_tmp_file_slabs();
	//CHECK_VALUE( ret_val );

	//ret_val = uninit_msg_file_create_para_slabs();	
	//CHECK_VALUE( ret_val );

	//ret_val = uninit_msg_file_rw_para_slabs();
	//CHECK_VALUE( ret_val );

	//ret_val = uninit_msg_file_close_para_slabs();
	//CHECK_VALUE( ret_val );

	ret_val = uninit_rw_data_buffer_slabs();
	CHECK_VALUE( ret_val );
	
	//ret_val = uninit_range_data_buffer_list_slabs();
	//CHECK_VALUE( ret_val );
	return ret_val;
}

_int32 fm_create_file_struct_xl( const char *p_file_name, const char *p_file_path, _u64 file_size, void *p_user, notify_file_create p_call_back, TMP_FILE **pp_file_struct ,_u32 write_mode)
{
	//check argument
	_int32 ret_val = SUCCESS;
	if( p_file_name == NULL || p_file_path == NULL || p_user == NULL || p_call_back == NULL  )
		return INVALID_ARGUMENT;
	LOG_DEBUG( "fm_create_file_struct." );
	
	ret_val = fm_create_and_init_struct_xl( p_file_name, p_file_path, file_size, pp_file_struct);
	CHECK_VALUE( ret_val );
	(*pp_file_struct)->_write_mode = (FILE_WRITE_MODE)write_mode;
	ret_val = fm_handle_create_file_xl( *pp_file_struct, p_user, p_call_back );
	CHECK_VALUE( ret_val );

	return SUCCESS;
}

_int32 fm_init_file_info_xl( struct tagTMP_FILE *p_file_struct, _u64 file_size, _u32 block_size )
{
	LOG_DEBUG( "fm_init_file_info. file_size:%llu, block_size:%u.", file_size, block_size );
	if( p_file_struct->_file_size == file_size ) return SUCCESS;
	
	p_file_struct->_file_size = file_size;

	return SUCCESS;
}

_int32 fm_change_file_size_xl( struct tagTMP_FILE *p_file_struct, _u64 file_size, void *p_user, notify_file_create p_call_back )
{
	_int32 ret_val;
	LOG_DEBUG( "fm_change_file_size. file_size:%llu.", file_size );
	if( file_size == p_file_struct->_file_size ) return SUCCESS;

	p_file_struct->_new_file_size = file_size;
	
	fm_cancel_list_rw_op_xl( &p_file_struct->_write_range_list );
	fm_cancel_list_rw_op_xl( &p_file_struct->_asyn_read_range_list );

	p_file_struct->_is_reopening = TRUE;
	ret_val = fm_generate_range_list( p_file_struct, NULL, NULL, (void *)p_call_back, p_user, OP_OPEN );
	CHECK_VALUE( ret_val );
	
	ret_val = fm_handle_write_range_list( p_file_struct );
	CHECK_VALUE( ret_val );		
	return SUCCESS;
}

_int32 fm_file_write_buffer_list_xl( struct tagTMP_FILE *p_file_struct
    , RANGE_DATA_BUFFER_LIST *p_buffer_list
    , notify_write_result p_call_back
    , void *p_user )
{
	_int32 ret_val = SUCCESS;	
	RANGE_DATA_BUFFER_LIST *p_tmp_buffer_list = NULL;
	LIST_ITERATOR cur_node, end_node, rbegin_node;
	RW_DATA_BUFFER *p_rw_data_buffer = NULL;
	_u32 write_list_size = 0;

	sd_assert( p_file_struct->_is_closing == FALSE );
	if( p_file_struct->_is_closing ) return FM_LOGIC_ERROR;
	ret_val = range_data_buffer_list_malloc_wrap( &p_tmp_buffer_list );
	CHECK_VALUE( ret_val );
	list_init( &p_tmp_buffer_list->_data_list ); 
	list_swap( &p_buffer_list->_data_list, &p_tmp_buffer_list->_data_list);
	
	cur_node = LIST_BEGIN( p_tmp_buffer_list->_data_list );
	end_node = LIST_END( p_tmp_buffer_list->_data_list );
	sd_assert( list_size( &p_tmp_buffer_list->_data_list ) != 0 );
	write_list_size = list_size( &p_file_struct->_write_range_list );
	
	LOG_DEBUG( "fm_file_write_buffer_list. buffer list ptr:0x%x, call_back_ptr:0x%x, user_ptr:0x%x , write_list_size:%u, tmp_buffer_list:0x%x, file_struct:0x%x", 
		p_buffer_list, p_call_back, p_user, write_list_size, p_tmp_buffer_list, p_file_struct );
	
	while( cur_node != end_node )
	{
		RANGE_DATA_BUFFER *p_range_buffer = LIST_VALUE(cur_node);
		sd_assert( p_range_buffer != NULL );
		ret_val = fm_generate_range_list( p_file_struct
		    , p_range_buffer
		    , p_tmp_buffer_list
		    , (void*)p_call_back
		    , p_user
		    , OP_WRITE );
		CHECK_VALUE( ret_val );	
		cur_node = LIST_NEXT( cur_node );	
	}

	rbegin_node = LIST_RBEGIN( p_file_struct->_write_range_list );
	p_rw_data_buffer = LIST_VALUE( rbegin_node );
	p_rw_data_buffer->_is_call_back = TRUE;

	LOG_DEBUG( "fm_file_write_buffer_list. write_list_size:%d ", write_list_size );	

	ret_val = fm_handle_write_range_list( p_file_struct );
	CHECK_VALUE( ret_val );		

	return SUCCESS;
}

_int32 fm_file_asyn_read_buffer_xl( struct tagTMP_FILE *p_file_struct, RANGE_DATA_BUFFER *p_data_buffer, notify_read_result p_call_back, void *p_user )
{
	_int32 ret_val = SUCCESS;
	LIST_ITERATOR rbegin_node;
	RW_DATA_BUFFER *p_rw_data_buffer = NULL;
	_u32 asyn_read_list_size = 0;
	
	sd_assert( p_file_struct->_is_closing == FALSE );
	if( p_file_struct->_is_closing ) return FM_LOGIC_ERROR;
	asyn_read_list_size = list_size( &p_file_struct->_asyn_read_range_list );
		
	LOG_DEBUG( "fm_file_asyn_read_buffer. data_buffer ptr:0x%x, call_back_ptr:0x%x, user_ptr:0x%x, read list size:%u ", 
		p_data_buffer, p_call_back, p_user, asyn_read_list_size );
	sd_assert( p_data_buffer != NULL );
	ret_val = fm_generate_range_list( p_file_struct, p_data_buffer, (void *)p_data_buffer, (void*)p_call_back, p_user, OP_READ );
	CHECK_VALUE( ret_val );

	rbegin_node = LIST_RBEGIN( p_file_struct->_asyn_read_range_list );
	p_rw_data_buffer = LIST_VALUE( rbegin_node );
	p_rw_data_buffer->_is_call_back = TRUE;
	
	ret_val = fm_handle_asyn_read_range_list( p_file_struct );
	CHECK_VALUE( ret_val );

	return SUCCESS;
}

_int32  fm_file_syn_read_buffer_xl( struct tagTMP_FILE *p_file_struct, RANGE_DATA_BUFFER *p_data_buffer )
{
	_int32 ret_val = SUCCESS;
	_u32 file_id = 0;
	_u32 read_len;
	char file_full_path[MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN];
	_u64 file_pos = 0;

	LOG_DEBUG( "fm_file_syn_read_buffer. data_buffer ptr:0x%x. ", p_data_buffer );	
		
	ret_val = fm_get_file_full_path( p_file_struct, file_full_path, MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN );
	CHECK_VALUE( ret_val );
	
	sd_assert( sd_strlen( file_full_path ) != 0 );
	
	ret_val = sd_open_ex( file_full_path, O_FS_RDWR, &file_id );
	LOG_DEBUG( "sd_open_ex ret_val:%d, file_full_path:%s.", ret_val, file_full_path);
	CHECK_VALUE( ret_val );

	file_pos = (_u64)p_data_buffer->_range._index * (_u64)get_data_unit_size();
	ret_val = sd_setfilepos( file_id, file_pos );
	CHECK_VALUE( ret_val );

	ret_val = sd_read( file_id, p_data_buffer->_data_ptr, p_data_buffer->_data_length, &read_len );
	CHECK_VALUE( ret_val );
    LOG_DEBUG("fm_file_syn_read_buffer_xl, sd_read need_read_data_len = %d, real read len = %d"
        , p_data_buffer->_data_length, read_len);
	sd_assert( p_data_buffer->_data_length == read_len );

	ret_val = sd_close_ex( file_id );
	LOG_DEBUG( "sd_close_ex ret_val:%d.", ret_val );	
	
	LOG_DEBUG( "fm_file_syn_read_buffer return. data pos: %llu, data_length: %u, read_len: %u",
		file_pos, p_data_buffer->_data_length, read_len );	
	return SUCCESS;
}

_int32 fm_close_xl( struct tagTMP_FILE *p_file_struct , notify_file_close p_call_back, void *p_user )
{
	_int32 ret_val =SUCCESS;
	LOG_DEBUG( "fm_close." );	

	sd_assert( p_file_struct->_is_closing == FALSE );
	if( p_file_struct->_is_closing ) return FM_LOGIC_ERROR;
	p_file_struct->_is_closing = TRUE;
	ret_val = msg_file_close_para_malloc_wrap( &p_file_struct->_close_para_ptr );
	CHECK_VALUE( ret_val );

	p_file_struct->_close_para_ptr->_file_struct_ptr = p_file_struct;
	p_file_struct->_close_para_ptr->_user_ptr = p_user;
	p_file_struct->_close_para_ptr->_callback_func_ptr = p_call_back;
	
	if(dm_get_enable_fast_stop() && p_file_struct->_file_size >= MAX_FILE_SIZE_NEED_FLUSH_DATA_B4_CLOSE)
	{
		fm_cancel_list_rw_op_xl( &p_file_struct->_write_range_list );
	}
	fm_cancel_list_rw_op_xl( &p_file_struct->_asyn_read_range_list );	
	ret_val = fm_handle_close_file_xl( p_file_struct );
	CHECK_VALUE( ret_val );
	return SUCCESS;
}

_int32 fm_file_change_name_xl( struct tagTMP_FILE *p_file_struct, const char *p_new_file_name )
{
    _int32 ret_val = SUCCESS;
	_int32 new_file_len = 0;

	char new_full_name[MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN];
	char old_full_name[MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN];
	
	ret_val = fm_get_file_full_path( p_file_struct, old_full_name, MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN );
	CHECK_VALUE( ret_val );
	
	LOG_DEBUG( "fm_file_change_name. new file name:%s.", p_new_file_name );	

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
	
	return SUCCESS;
}

_int32 fm_load_cfg_block_index_array_xl( struct tagTMP_FILE *p_file_struct, _u32 cfg_file_id )
{
	return SUCCESS;
}

_int32 fm_flush_cfg_block_index_array_xl( struct tagTMP_FILE *p_file_struct, _u32 cfg_file_id )
{
	return SUCCESS;
}

#if 0
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
#endif
//#endif
