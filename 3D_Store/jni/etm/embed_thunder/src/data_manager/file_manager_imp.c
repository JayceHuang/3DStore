#include "utility/define.h"
//#ifndef XUNLEI_MODE

#include "file_manager_imp.h"

#include "utility/string.h"
#include "utility/mempool.h"
#include "utility/logid.h"
#include "utility/utility.h"
#define LOGID LOGID_FILE_MANAGER
#include "utility/logger.h"
#include "platform/sd_fs.h"


_int32 fm_create_and_init_struct( const char *p_filename, const char *p_filepath, _u64 file_size, TMP_FILE **pp_file_struct )
{
	_int32 ret_val = SUCCESS;
	TMP_FILE *p_file_struct = NULL;

	ret_val = tmp_file_malloc_wrap( &p_file_struct );
	CHECK_VALUE( ret_val );
	
	LOG_DEBUG( "fm_init_struct: file_struct ptr:0x%x, file name:%s, file path:%s", 
		p_file_struct, p_filename, p_filepath );
			
	p_file_struct->_file_name_len = sd_strlen( p_filename );
	ret_val = sd_memcpy( p_file_struct->_file_name, p_filename, p_file_struct->_file_name_len );
	CHECK_VALUE( ret_val );
	
    p_file_struct->_file_name[p_file_struct->_file_name_len] = '\0';

	p_file_struct->_file_path_len = sd_strlen( p_filepath );	
	ret_val = sd_memcpy( p_file_struct->_file_path, p_filepath, p_file_struct->_file_path_len );
	CHECK_VALUE( ret_val );
	
    p_file_struct->_file_path[p_file_struct->_file_path_len] = '\0';

	p_file_struct->_device_id = 0;
	p_file_struct->_block_size = 0;
	p_file_struct->_file_size = file_size;	
	p_file_struct->_block_num = 0;		
	p_file_struct->_last_block_size = 0;	
	
	list_init( &(p_file_struct->_asyn_read_block_list) );
	list_init( &(p_file_struct->_syn_read_block_list) );
	list_init( &(p_file_struct->_write_block_list) );
	
	p_file_struct->_close_para_ptr = NULL;	
	p_file_struct->_block_index_array._index_array_ptr = NULL;	
	
	p_file_struct->_is_closing = FALSE;
	p_file_struct->_is_known_block_size = FALSE;
	p_file_struct->_is_file_size_valid = FALSE;
	p_file_struct->_is_file_created = FALSE;
	p_file_struct->_is_order_mode = FALSE;  /* TRUE -> FALSE by zyq @20101224 for ipadkankan  */
	p_file_struct->_is_writing = FALSE;	
	p_file_struct->_is_asyn_reading = FALSE;	

	p_file_struct->_open_msg_id = INVALID_MSG_ID;
	p_file_struct->_is_opening = FALSE;

    p_file_struct->_write_error = SUCCESS;

	*pp_file_struct = p_file_struct;
	return SUCCESS;
}

_int32 fm_handle_create_file( TMP_FILE *p_file_struct, void *p_user, notify_file_create p_call_back )
{
	OP_PARA_FS_OPEN open_para;
	_int32 ret_val = SUCCESS;
	char file_full_path[MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN];
	MSG_FILE_CREATE_PARA *p_user_para = NULL;

	LOG_DEBUG( "fm_handle_create_file. user_ptr:0x%x, call_back_ptr:0x%x.", p_user, p_call_back);
	
	ret_val = msg_file_create_para_malloc_wrap( &p_user_para );// delete in call_back function
	CHECK_VALUE( ret_val );
	p_user_para->_callback_func_ptr = (void*)p_call_back;
	p_user_para->_file_struct_ptr = p_file_struct;
	p_user_para->_user_ptr = p_user;
	
	ret_val = sd_memcpy( file_full_path, p_file_struct->_file_path, p_file_struct->_file_path_len);
	CHECK_VALUE( ret_val );
	ret_val = sd_memcpy( file_full_path + p_file_struct->_file_path_len, p_file_struct->_file_name, p_file_struct->_file_name_len );
	CHECK_VALUE( ret_val );
	
	open_para._filepath = file_full_path;
	open_para._filepath_buffer_len = p_file_struct->_file_path_len + p_file_struct->_file_name_len;
	sd_assert( open_para._filepath_buffer_len < MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN );
	open_para._open_option = O_DEVICE_FS_CREATE;
	open_para._file_size = p_file_struct->_block_size;//p_file_struct->_file_size;

	p_file_struct->_is_opening = TRUE;
	ret_val = fm_op_open( &open_para,  p_user_para, &p_file_struct->_open_msg_id );
	CHECK_VALUE( ret_val );

	return SUCCESS;
}

_int32 fm_op_open( OP_PARA_FS_OPEN *p_fso_para, MSG_FILE_CREATE_PARA *p_user_para, _u32 *msg_id )
{
	MSG_INFO msginfo = {};
	_u32 msgid;
	LOG_DEBUG( "fm_op_open." );
	
	msginfo._device_id = 0;
	msginfo._device_type = DEVICE_FS;
	msginfo._msg_priority = MSG_PRIORITY_NORMAL;
	msginfo._operation_type = OP_OPEN;
	msginfo._user_data = p_user_para;
	msginfo._operation_parameter = p_fso_para;
	
	post_message(&msginfo, fm_callback, NOTICE_ONCE, 10000, &msgid);
	return SUCCESS;
}

_int32 fm_cancel_open_msg(TMP_FILE *p_file_struct)
{
	_int32 ret_val = SUCCESS;
	
	LOG_DEBUG( "fm_cancel_open_msg. msd_id:%d", p_file_struct->_open_msg_id );
	sd_assert( p_file_struct->_is_opening );
	
	ret_val = cancel_message_by_msgid( p_file_struct->_open_msg_id );
	sd_assert( ret_val == SUCCESS );
	return SUCCESS;
}

_int32 fm_generate_block_list( TMP_FILE *p_file_struct
    , RANGE_DATA_BUFFER *p_range_buffer
    , _u16 op_type, void *buffer_para, void *p_call_back, void *p_user )
{
	_int32 ret_val = SUCCESS;
	_u32 range_num = 0;
	_u64 range_pos = 0;
	_u32 block_size, data_unit_size;
	_u32 end_block_index = 0, begin_block_index = 0, cur_block_index = 0;
	_u32 last_block_length = 0;

	data_unit_size = get_data_unit_size();
	range_num = p_range_buffer->_range._num;
	range_pos = p_range_buffer->_range._index;
	if( range_pos * data_unit_size + p_range_buffer->_data_length != p_file_struct->_file_size ) 
	{
		sd_assert( p_range_buffer->_data_length == range_num * data_unit_size );
	}
	sd_assert( range_num != 0 );
	
	block_size = p_file_struct->_block_size;

	sd_assert( block_size % data_unit_size == 0 );
	sd_assert( block_size / data_unit_size != 0 );
	
	LOG_DEBUG( "##########fm_generate_block_list, range_buffer_ptr:0x%x, range pos:%llu, range length:%u, range_op_type:%u", 
		p_range_buffer, range_pos * data_unit_size, range_num * data_unit_size, (_u32)op_type);
		
	begin_block_index = range_pos / ( block_size / data_unit_size );
	if( ( range_pos + range_num ) % ( block_size / data_unit_size ) == 0 )
	{
		end_block_index = ( range_pos + range_num ) / ( block_size / data_unit_size ) - 1;
	}
	else
	{
		end_block_index = ( range_pos + range_num ) / ( block_size / data_unit_size );
	}

	LOG_DEBUG( "fm_generate_block_list, begin_block_index:%u, end_block_index:%u############", 
		begin_block_index, end_block_index );

	cur_block_index = begin_block_index;
	while( cur_block_index <= end_block_index )
	{
		BLOCK_DATA_BUFFER *p_block_data_buffer = NULL;
		ret_val = block_data_buffer_malloc_wrap( &p_block_data_buffer );
		CHECK_VALUE( ret_val );
		
		p_block_data_buffer->_block_index = cur_block_index;
		if( cur_block_index != end_block_index )//muti blocks generate normal blocks
		{	
			if( cur_block_index == begin_block_index && range_pos % (block_size/data_unit_size) != 0 )
			{
				p_block_data_buffer->_block_offset = (_u32)(range_pos * data_unit_size % block_size );
				p_block_data_buffer->_data_length = block_size - p_block_data_buffer->_block_offset;
			}
			else
			{
				p_block_data_buffer->_block_offset = 0;
				p_block_data_buffer->_data_length = block_size;
			}
			p_block_data_buffer->_is_range_buffer_end = FALSE;
		}	
		else if(  begin_block_index != end_block_index )//muti blocks generate end block
		{
			p_block_data_buffer->_block_offset = 0;
			p_block_data_buffer->_data_length = (_u64) (range_pos * data_unit_size + p_range_buffer->_data_length) % block_size;
			p_block_data_buffer->_is_range_buffer_end = TRUE;
		}
		else  //single blocks
		{
			p_block_data_buffer->_block_offset = (_u64)( range_pos * data_unit_size ) % block_size;
			p_block_data_buffer->_data_length = p_range_buffer->_data_length;
			p_block_data_buffer->_is_range_buffer_end = TRUE;
		}
		LOG_DEBUG( "generate blocks: _block_data_buffer_ptr:0x%x, block_index:%u, _block_offset:%u, _data_length:%u ", p_block_data_buffer,
			cur_block_index, p_block_data_buffer->_block_offset, p_block_data_buffer->_data_length);
	
		p_block_data_buffer->_data_ptr = p_range_buffer->_data_ptr + last_block_length;
		last_block_length = p_block_data_buffer->_data_length;
		
		p_block_data_buffer->_is_call_back = FALSE;
		p_block_data_buffer->_is_tmp_block = FALSE;
		p_block_data_buffer->_try_num = 0;
		p_block_data_buffer->_operation_type = op_type;
		p_block_data_buffer->_buffer_para_ptr = (void *)buffer_para;
		p_block_data_buffer->_user_ptr = p_user;
		p_block_data_buffer->_call_back_ptr = (void *)p_call_back;
		
		p_block_data_buffer->_range_buffer = (void *)p_range_buffer;
		
		if( op_type == FM_OP_BLOCK_WRITE )
		{
			LOG_DEBUG( "fm_generate_block_list, block_buffer_ptr:0x%x, op_type = FM_OP_BLOCK_WRITE ", p_block_data_buffer );
			ret_val = list_push( &p_file_struct->_write_block_list, (void*)p_block_data_buffer );	
			CHECK_VALUE( ret_val );
		}
		else if ( op_type == FM_OP_BLOCK_NORMAL_READ )
		{
			LOG_DEBUG( "fm_generate_block_list, block_buffer_ptr:0x%x, op_type = FM_OP_BLOCK_NORMAL_READ ", p_block_data_buffer );
			ret_val = list_push( &p_file_struct->_asyn_read_block_list, (void*)p_block_data_buffer );	
			CHECK_VALUE( ret_val );
		}
		else if( op_type == FM_OP_BLOCK_SYN_READ )
		{
			LOG_DEBUG( "fm_generate_block_list, block_buffer_ptr:0x%x, op_type = FM_OP_BLOCK_SYN_READ ", p_block_data_buffer );
			ret_val = list_push( &p_file_struct->_syn_read_block_list, (void*)p_block_data_buffer );
			CHECK_VALUE( ret_val );
		}	
		cur_block_index++;
	}

	return SUCCESS;
}

_int32 fm_handle_write_block_list( TMP_FILE *p_file_struct )
{
	_int32 ret_val = SUCCESS;
	LIST_ITERATOR cur_node;
	BLOCK_DATA_BUFFER *p_block_data_buffer = NULL;
	notify_write_result p_write_call_back = NULL;
	
	if( list_size( &p_file_struct->_write_block_list ) == 0 )
	{
		p_file_struct->_is_writing = FALSE;
		return SUCCESS;
	}

	cur_node = LIST_BEGIN( p_file_struct->_write_block_list );
	p_block_data_buffer = (BLOCK_DATA_BUFFER *)LIST_VALUE( cur_node );
	LOG_DEBUG( "fm_handle_write_block_list. " );
	if(p_block_data_buffer->_is_cancel)
	{
		LOG_DEBUG( "fm_handle_write_block_list:CANCELED:operation_type = %d,is_call_back=%d. " ,p_block_data_buffer->_operation_type,p_block_data_buffer->_is_call_back);
		list_erase( &p_file_struct->_write_block_list, cur_node );	
		sd_assert(p_block_data_buffer->_operation_type == FM_OP_BLOCK_WRITE);
		if(p_block_data_buffer->_is_call_back && p_block_data_buffer->_operation_type == FM_OP_BLOCK_WRITE)
		{
			p_write_call_back = (notify_write_result)p_block_data_buffer->_call_back_ptr;
			p_write_call_back( p_file_struct, p_block_data_buffer->_user_ptr, p_block_data_buffer->_buffer_para_ptr, FM_WRITE_CLOSE_EVENT );
			
		}
		block_data_buffer_free_wrap( p_block_data_buffer );
		/* 处理下一个块 */
		fm_handle_write_block_list( p_file_struct );
		return SUCCESS;
	}
	
	if( p_block_data_buffer->_operation_type == FM_OP_BLOCK_WRITE )
	{
		ret_val = fm_try_to_write_block_data( p_file_struct, p_block_data_buffer );
		if( ret_val != SUCCESS )
		{
			list_erase( &p_file_struct->_write_block_list, cur_node );		
			LOG_ERROR( "fm_write_callback exceed try num.errode:%u.", ret_val);	
			if(p_block_data_buffer->_is_call_back)
			{
				p_write_call_back = (notify_write_result)p_block_data_buffer->_call_back_ptr;
				p_write_call_back( p_file_struct, p_block_data_buffer->_user_ptr, p_block_data_buffer->_buffer_para_ptr, ret_val );
				
			}
			block_data_buffer_free_wrap( p_block_data_buffer );
			/* 处理下一个块 */
			fm_handle_write_block_list( p_file_struct );
		}
		return SUCCESS;
	}
	if( p_block_data_buffer->_operation_type == FM_OP_BLOCK_TMP_READ )
	{
		p_file_struct->_block_index_array._index_array_ptr[p_block_data_buffer->_block_index]._is_valid = FALSE;
		ret_val = fm_asyn_handle_block_data( p_file_struct, p_block_data_buffer, p_block_data_buffer->_block_index );
		CHECK_VALUE( ret_val );
	}
	return SUCCESS;
}

_int32 fm_handle_order_write_block_list(  TMP_FILE *p_file_struct )
{
	_int32 ret_val = SUCCESS;
	BLOCK_DATA_BUFFER *p_block_data_buffer = NULL;
	LIST_ITERATOR cur_node = LIST_BEGIN( p_file_struct->_write_block_list );
	p_block_data_buffer = (BLOCK_DATA_BUFFER *)LIST_VALUE( cur_node );
	LOG_DEBUG( "fm_handle_order_write_block_list. call_back ptr:0x%x, user_ptr:0x%x " );

	ret_val = fm_asyn_handle_block_data( p_file_struct, p_block_data_buffer, p_block_data_buffer->_block_index );
	CHECK_VALUE( ret_val );

	return SUCCESS;
}
void fm_printf_block_index_array( TMP_FILE *p_file_struct, BLOCK_DATA_BUFFER *p_block_data_buffer )
{
#ifdef _DEBUG
	_u32 block_index = 0;
	DATA_BLOCK_INDEX_ARRAY * block_index_array = &p_file_struct->_block_index_array;
	
	LOG_DEBUG( "fm_printf_block_index_array. path[%u]=%s,name[%u]=%s", 
		p_file_struct->_file_path_len,p_file_struct->_file_path,p_file_struct->_file_name_len,p_file_struct->_file_name);
#ifdef LINUX
	printf( "\n **********\n fm_printf_block_index_array. path[%u]=%s,name[%u]=%s\n", 
		p_file_struct->_file_path_len,p_file_struct->_file_path,p_file_struct->_file_name_len,p_file_struct->_file_name);
#endif /* LINUX */

	LOG_DEBUG( "fm_printf_block_index_array. write_mode=%d,file_created=%d,size_valid=%d,file_size=%llu,block_num=%u,block_size=%u,last_block_size=%u,device_id=%u,new_file_size=%llu", 
		p_file_struct->_write_mode,p_file_struct->_is_file_created,p_file_struct->_is_file_size_valid,p_file_struct->_file_size,p_file_struct->_block_num,p_file_struct->_block_size,p_file_struct->_last_block_size,p_file_struct->_device_id,p_file_struct->_new_file_size);

#ifdef LINUX
	printf( "\n fm_printf_block_index_array. write_mode=%d,file_created=%d,size_valid=%d,file_size=%llu,block_num=%u,block_size=%u,last_block_size=%u,device_id=%u,new_file_size=%llu\n", 
		p_file_struct->_write_mode,p_file_struct->_is_file_created,p_file_struct->_is_file_size_valid,p_file_struct->_file_size,p_file_struct->_block_num,p_file_struct->_block_size,p_file_struct->_last_block_size,p_file_struct->_device_id,p_file_struct->_new_file_size);
#endif /* LINUX */

	LOG_DEBUG( "fm_printf_block_index_array. opening=%d,closing=%d,reopening=%d,asyn_reading=%d,writing=%d,asyn_read_blocks=%u,syn_read_blocks=%u,write_blocks=%u", 
		p_file_struct->_is_opening,p_file_struct->_is_closing,p_file_struct->_is_reopening,p_file_struct->_is_asyn_reading,p_file_struct->_is_writing,list_size(&p_file_struct->_asyn_read_block_list),list_size(&p_file_struct->_syn_read_block_list),list_size(&p_file_struct->_write_block_list));

#ifdef LINUX
	printf( "\n fm_printf_block_index_array. opening=%d,closing=%d,reopening=%d,asyn_reading=%d,writing=%d,asyn_read_blocks=%u,syn_read_blocks=%u,write_blocks=%u\n", 
		p_file_struct->_is_opening,p_file_struct->_is_closing,p_file_struct->_is_reopening,p_file_struct->_is_asyn_reading,p_file_struct->_is_writing,list_size(&p_file_struct->_asyn_read_block_list),list_size(&p_file_struct->_syn_read_block_list),list_size(&p_file_struct->_write_block_list));
#endif /* LINUX */

	LOG_DEBUG( "fm_printf_block_index_array. total_index_num=%u,valid_index_num=%u,extra_block_index=%u,empty_block_index=%u", 
		block_index_array->_total_index_num,block_index_array->_valid_index_num,block_index_array->_extra_block_index,block_index_array->_empty_block_index);

#ifdef LINUX
	printf( "\n fm_printf_block_index_array. total_index_num=%u,valid_index_num=%u,extra_block_index=%u,empty_block_index=%u\n", 
		block_index_array->_total_index_num,block_index_array->_valid_index_num,block_index_array->_extra_block_index,block_index_array->_empty_block_index);
#endif /* LINUX */

	LOG_DEBUG( "fm_printf_block_index_array. block_index=%u,block_offset=%u,data_length=%u,try_num=%u,is_tmp_block=%d", 
		p_block_data_buffer->_block_index,p_block_data_buffer->_block_offset,p_block_data_buffer->_data_length,p_block_data_buffer->_try_num,p_block_data_buffer->_is_tmp_block);

#ifdef LINUX
	printf( "\n fm_printf_block_index_array. block_index=%u,block_offset=%u,data_length=%u,try_num=%u,is_tmp_block=%d\n", 
		p_block_data_buffer->_block_index,p_block_data_buffer->_block_offset,p_block_data_buffer->_data_length,p_block_data_buffer->_try_num,p_block_data_buffer->_is_tmp_block);
#endif /* LINUX */


	for( ; block_index < block_index_array->_valid_index_num; block_index++ )
	{
		LOG_DEBUG( "fm_printf_block_index_array. index_array[%u]:logic_block_index=%u, is_valid:%d",
			block_index,block_index_array->_index_array_ptr[block_index]._logic_block_index,  block_index_array->_index_array_ptr[block_index]._is_valid );
#ifdef LINUX
		printf( "\n fm_printf_block_index_array. index_array[%u]:logic_block_index=%u, is_valid:%d\n",
			block_index,block_index_array->_index_array_ptr[block_index]._logic_block_index,  block_index_array->_index_array_ptr[block_index]._is_valid );
#endif /* LINUX */
	}
	
	LOG_DEBUG( "fm_printf_block_index_array. end!");

#ifdef LINUX
	printf( "\n fm_printf_block_index_array.end!\n**********\n");
#endif /* LINUX */
#endif /*  debug */	
	return;
}

_int32 fm_try_to_write_block_data( TMP_FILE *p_file_struct, BLOCK_DATA_BUFFER *p_block_data_buffer )
{
	_int32 ret_val = SUCCESS;
	BOOL b_exist = FALSE;
	_u32 disk_block;
	_u32 block_index = p_block_data_buffer->_block_index;
	_u32 valid_block_index_num = p_file_struct->_block_index_array._valid_index_num;
	_u32 block_size = p_file_struct->_block_size;
	//DATA_BLOCK_INDEX_ITEM  *data_block_index_array = p_file_struct->_block_index_array._index_array_ptr;

	LOG_DEBUG( "########fm_try_to_write_block_data: logic block index:%u, valid index num:%u, block size:%u, block_offset:%d, data_length:%d",
		block_index, valid_block_index_num, block_size, p_block_data_buffer->_block_offset, p_block_data_buffer->_data_length );

	b_exist = fm_get_cfg_disk_block_index( &p_file_struct->_block_index_array, p_block_data_buffer->_block_index, &disk_block );

	if( b_exist && p_block_data_buffer->_operation_type == FM_OP_BLOCK_WRITE ) // multi dispatch or write the part block
	{
		LOG_DEBUG( "block exist,fm_try_to_write_block_data: write block index again:%u!!", block_index );
		ret_val = fm_asyn_handle_block_data( p_file_struct, p_block_data_buffer, disk_block );		
		CHECK_VALUE( ret_val );
		return SUCCESS;
	}
		
	//if( block_index >= valid_block_index_num )
	{
		//update the _block_index_array, then write block data to the array end
		LOG_DEBUG( "fm_try_to_write_block_data: write data to the file end!!" );
		if(p_file_struct->_block_index_array._valid_index_num>=p_file_struct->_block_index_array._total_index_num)
		{
			/* 不应该出现这种情况!  */
			fm_printf_block_index_array( p_file_struct, p_block_data_buffer );
			sd_assert(p_file_struct->_block_index_array._valid_index_num<p_file_struct->_block_index_array._total_index_num);
			return FM_BLOCK_MALLOC_ERROR;
		}
		
		ret_val = fm_set_block_index_array( &p_file_struct->_block_index_array, valid_block_index_num, block_index );
		CHECK_VALUE(ret_val);
		if( block_index == p_file_struct->_block_num - 1 )//handle temp last block
		{
			_u32 data_length = p_block_data_buffer->_data_length;
			sd_assert( p_block_data_buffer->_block_offset < p_file_struct->_last_block_size );
			p_block_data_buffer->_data_length = MIN( data_length, p_file_struct->_last_block_size - p_block_data_buffer->_block_offset );
		}
		ret_val = fm_asyn_handle_block_data( p_file_struct, p_block_data_buffer, valid_block_index_num );
		CHECK_VALUE( ret_val );
	}
	#if 0
	else if( !data_block_index_array[block_index]._is_valid )
	{
		//update the _block_index_array, then write block data to the correct position
		LOG_DEBUG( "fm_try_to_write_block_data: write data to the correct pos!!" );
		data_block_index_array[block_index]._logic_block_index = block_index;
		data_block_index_array[block_index]._is_valid = TRUE;
		ret_val = fm_asyn_handle_block_data( p_file_struct, p_block_data_buffer, block_index );		
		CHECK_VALUE( ret_val );
	}
	else
	{
		//update the block_index_array, then write_block_data_list
		BLOCK_DATA_BUFFER *p_block_write_buffer = NULL;
		BLOCK_DATA_BUFFER *p_block_read_buffer = NULL;
		void *p_data_buffer = NULL;
		_u32 logic_block_index; 
		RANGE_DATA_BUFFER_LIST *p_buffer_list;
		LIST_ITERATOR cur_node, next_node;

		LOG_DEBUG( "fm_try_to_write_block_data: need temp read!!" );
		p_buffer_list = (RANGE_DATA_BUFFER_LIST *)p_block_data_buffer->_buffer_para_ptr;
		logic_block_index = data_block_index_array[block_index]._logic_block_index;
	
		ret_val = sd_malloc( block_size, &p_data_buffer );
		sd_assert( ret_val == SUCCESS );
		if( ret_val != SUCCESS )
		{
			LOG_ERROR( "fm_try_to_write_block_data, malloc error:%d.", ret_val );
			return FM_BLOCK_MALLOC_ERROR;
		}
		
        //read data to the buffer
        ret_val = fm_generate_tmp_write_block( p_file_struct, block_index, FALSE, p_buffer_list, p_data_buffer, &p_block_read_buffer );
        CHECK_VALUE( ret_val );
        p_block_read_buffer->_call_back_ptr = p_block_data_buffer->_call_back_ptr;
        p_block_read_buffer->_user_ptr = p_block_data_buffer->_user_ptr;
     
		//write the buffer data, transfer the range list end
		ret_val = fm_generate_tmp_write_block( p_file_struct, logic_block_index, TRUE, p_buffer_list, p_data_buffer, &p_block_write_buffer );
		CHECK_VALUE( ret_val );		
		if( p_block_data_buffer->_is_call_back )
		{
			p_block_data_buffer->_is_call_back = FALSE;
			p_block_write_buffer->_is_call_back = TRUE;
		}
                
		p_block_write_buffer->_data_length = p_block_read_buffer->_data_length;
		p_block_write_buffer->_call_back_ptr = p_block_data_buffer->_call_back_ptr;
		p_block_write_buffer->_user_ptr = p_block_data_buffer->_user_ptr;
		
		cur_node = LIST_BEGIN( p_file_struct->_write_block_list );
		next_node = LIST_NEXT( cur_node );
		sd_assert( LIST_VALUE( cur_node ) == p_block_data_buffer );

		//take the read buffer as the first block operation, the write buffer as the third block operation
		list_insert( &(p_file_struct->_write_block_list), (void*)p_block_write_buffer, next_node );
			
		list_insert( &(p_file_struct->_write_block_list), (void*)p_block_read_buffer, cur_node );
		
		fm_handle_write_block_list( p_file_struct );
	}
	#endif
	return SUCCESS;
}

_int32 fm_set_block_index_array( DATA_BLOCK_INDEX_ARRAY *block_index_array, _u32 disk_block_index, _u32 logic_block_index )
{
	_u32 block_index = 0;
	LOG_DEBUG( "fm_set_block_index_array: disk_block_index:%u, logic_block_index:%u.",
		disk_block_index, logic_block_index );

	/* 加强校验 */
	for( ; block_index < block_index_array->_valid_index_num; block_index++ )
	{
		if( block_index_array->_index_array_ptr[block_index]._logic_block_index == logic_block_index 
		&& block_index_array->_index_array_ptr[block_index]._is_valid == TRUE )
		{
			LOG_ERROR( "fm_set_block_index_array. logic_block_index:%u, want disk_block_index:%u,alreay disk_block_index:%u", logic_block_index, disk_block_index,block_index );
			sd_assert(FALSE);
			return FM_BLOCK_MALLOC_ERROR;
		}
	}
		
	block_index_array->_index_array_ptr[disk_block_index]._logic_block_index = logic_block_index;	
	block_index_array->_index_array_ptr[disk_block_index]._is_valid = TRUE;		
	block_index_array->_valid_index_num++;	
	
	return SUCCESS;
}

_int32 fm_generate_tmp_write_block(TMP_FILE *p_file_struct, _u32 disk_block_index, BOOL is_write, RANGE_DATA_BUFFER_LIST *p_buffer_list, char *p_data_buffer, BLOCK_DATA_BUFFER **pp_block_data_buffer )
{
	_int32 ret_val = SUCCESS;
	_u32 block_size = 0;
	BLOCK_DATA_BUFFER *p_block_buffer = NULL;		
	block_size = p_file_struct->_block_size;
	LOG_DEBUG( "fm_generate_tmp_write_block: disk_block_index:%u, is_write:%u, block size:%u ", disk_block_index, is_write, block_size );
		
	ret_val = block_data_buffer_malloc_wrap( &p_block_buffer );
	CHECK_VALUE( ret_val );	
	
    if( is_write == TRUE )
    {
		p_block_buffer->_data_length = block_size; 
		p_block_buffer->_operation_type = FM_OP_BLOCK_WRITE;
    } 
    else
    { 
		_u64 tmp_filesize = fm_get_tmp_filesize( p_file_struct );
		p_block_buffer->_operation_type = FM_OP_BLOCK_TMP_READ;	
		if( disk_block_index * block_size + block_size > tmp_filesize )
		{   
		sd_assert(disk_block_index * block_size<=tmp_filesize);
			p_block_buffer->_data_length = tmp_filesize - disk_block_index * block_size;
		}
		else
		{
			p_block_buffer->_data_length = block_size;   
		}
    }
    LOG_DEBUG( "fm_generate_tmp_write_block: disk_block_index:%u, is_write:%u, block size:%u, tmp opt length:%u ", disk_block_index, is_write, block_size, p_block_buffer->_data_length );	

    p_block_buffer->_block_index = disk_block_index;	
	p_block_buffer->_block_offset = 0;

	p_block_buffer->_data_ptr = p_data_buffer;
	p_block_buffer->_buffer_para_ptr = p_buffer_list;
	p_block_buffer->_is_call_back = FALSE;	
	p_block_buffer->_is_tmp_block = TRUE;	
	p_block_buffer->_try_num = 0;	
	
	*pp_block_data_buffer = p_block_buffer;
	return SUCCESS;
}

//handle the read and write operation for the block data
_int32 fm_asyn_handle_block_data( TMP_FILE *p_file_struct, BLOCK_DATA_BUFFER *p_block_data_buffer, _u32 block_disk_index )
{
	OP_PARA_FS_RW rw_para;
	MSG_FILE_RW_PARA *p_user_para;
	_int32 ret_val;

	LOG_DEBUG( "fm_asyn_handle_block_data: block_disk_index:%u , p_block_data_buffer:0x%x.", block_disk_index, p_block_data_buffer );

	ret_val = msg_file_rw_para_slab_malloc_wrap( &p_user_para );
	CHECK_VALUE( ret_val );
	
	p_user_para->_file_struct_ptr = p_file_struct;	
	
	p_user_para->_callback_func_ptr = p_block_data_buffer->_call_back_ptr;
	p_user_para->_user_ptr = p_block_data_buffer->_user_ptr;
	p_user_para->_rw_para_ptr = p_block_data_buffer;
	p_user_para->_op_type = p_block_data_buffer->_operation_type;
	
	rw_para._buffer = p_block_data_buffer->_data_ptr;
	rw_para._expect_size = p_block_data_buffer->_data_length;	
	rw_para._operating_offset = ( _u64 )block_disk_index  * p_file_struct->_block_size + p_block_data_buffer->_block_offset;
	rw_para._operated_size = 0;
	
	LOG_DEBUG( "fm_asyn_handle_block_data: buffer:0x%x, expect_size:%u, operating_offset:%llu, _operated_size = %u.", 
		p_block_data_buffer->_data_ptr, rw_para._expect_size, rw_para._operating_offset, rw_para._operated_size );
	
	if( p_block_data_buffer->_operation_type == FM_OP_BLOCK_WRITE )
	{
		LOG_DEBUG( "fm_asyn_handle_block_data: block_disk_index:%u, block_offset:%d, operation_type = FM_OP_BLOCK_WRITE.", 
			block_disk_index, p_block_data_buffer->_block_offset );	
		ret_val = fm_op_rw( p_file_struct->_device_id, OP_WRITE, &rw_para, p_user_para );
		CHECK_VALUE( ret_val );
	}
	else if( p_block_data_buffer->_operation_type == FM_OP_BLOCK_TMP_READ || p_block_data_buffer->_operation_type == FM_OP_BLOCK_NORMAL_READ )
	{
		LOG_DEBUG( "fm_asyn_handle_block_data: block_disk_index:%u, operation_type = OP_READ.", block_disk_index );	
		ret_val = fm_op_rw( p_file_struct->_device_id, OP_READ, &rw_para, p_user_para );
		CHECK_VALUE( ret_val );
	}
	
	return SUCCESS;
}

_int32 fm_op_rw( _u32 device_id, _u16 op, OP_PARA_FS_RW *p_frw_para, MSG_FILE_RW_PARA *p_user_para )
{
	MSG_INFO msg_info = {};
	_u32 msgid;
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "fm_op_rw .p_user_para:0x%x.", p_user_para );
	
	msg_info._device_id = device_id;
	msg_info._device_type = DEVICE_FS;
	msg_info._msg_priority = MSG_PRIORITY_NORMAL;
	msg_info._operation_parameter = p_frw_para;
	msg_info._operation_type = op;
	msg_info._user_data = p_user_para;

	ret_val = post_message(&msg_info, fm_callback, NOTICE_ONCE, 10000, &msgid);
	CHECK_VALUE( ret_val );

	return SUCCESS;
}


//handle normal read
_int32 fm_handle_asyn_read_block_list( TMP_FILE *p_file_struct )
{
	_int32 ret_val = SUCCESS;
	_u32 block_disk_index;
	BOOL ret_bool = FALSE;
	BLOCK_DATA_BUFFER *p_block_data_buffer = NULL;
	LIST_ITERATOR cur_node;
	_u32 asyn_read_list_size = 0;
	notify_read_result p_read_call_back;
	asyn_read_list_size = list_size( &p_file_struct->_asyn_read_block_list );
	
	LOG_DEBUG( "fm_handle_asyn_read_block_list.list_size:%d ", asyn_read_list_size );	
	if( asyn_read_list_size == 0 )
	{
		p_file_struct->_is_asyn_reading = FALSE;
		return SUCCESS;
	}
	
	cur_node = LIST_BEGIN( p_file_struct->_asyn_read_block_list );
	p_block_data_buffer = (BLOCK_DATA_BUFFER *)LIST_VALUE( cur_node );
	
	LOG_DEBUG( "fm_handle_asyn_read_block_list. block_ptr:0x%x", p_block_data_buffer );	

	if(p_block_data_buffer->_is_cancel)
	{
		list_erase( &p_file_struct->_asyn_read_block_list, cur_node );	
		if( p_block_data_buffer->_is_call_back )
		{
			LOG_DEBUG( "fm_read_callback return FM_READ_CLOSE_EVENT." );
			p_read_call_back = (notify_read_result)p_block_data_buffer->_call_back_ptr;
			p_read_call_back( p_file_struct, p_block_data_buffer->_user_ptr, p_block_data_buffer->_buffer_para_ptr, FM_READ_CLOSE_EVENT, p_block_data_buffer->_data_length );
		}	
		block_data_buffer_free_wrap( p_block_data_buffer );
		fm_handle_asyn_read_block_list(p_file_struct );
		return SUCCESS;
	}
	
	ret_bool = fm_get_cfg_disk_block_index( &p_file_struct->_block_index_array, p_block_data_buffer->_block_index, &block_disk_index );
	if( !ret_bool ) return FM_LOGIC_ERROR;
	
	ret_val = fm_asyn_handle_block_data( p_file_struct, p_block_data_buffer, block_disk_index );
	CHECK_VALUE( ret_val );

	return SUCCESS;
}

_int32 fm_handle_syn_read_block_list( TMP_FILE *p_file_struct )
{
	_int32 ret_val = SUCCESS;
	BOOL ret_bool = FALSE;
	_u32 file_id;
	BLOCK_DATA_BUFFER *p_block_data_buffer = NULL;	
	LIST_ITERATOR cur_node, next_node;

	LOG_DEBUG( "fm_handle_syn_read_block_list." );	
	
	ret_val = get_syn_op_file_id( p_file_struct, &file_id );
	CHECK_VALUE( ret_val );
	
	cur_node = LIST_BEGIN( p_file_struct->_syn_read_block_list );

	while( cur_node != LIST_END( p_file_struct->_syn_read_block_list ) )
	{
		_u32 block_disk_index;
		p_block_data_buffer = (BLOCK_DATA_BUFFER *)LIST_VALUE( cur_node );
		ret_bool = fm_get_cfg_disk_block_index( &p_file_struct->_block_index_array, p_block_data_buffer->_block_index, &block_disk_index );
		if( !ret_bool ) return FM_LOGIC_ERROR;
		ret_val = fm_syn_handle_block_data( p_file_struct, p_block_data_buffer, block_disk_index, file_id);
		CHECK_VALUE( ret_val );
		ret_val = block_data_buffer_free_wrap( p_block_data_buffer );
		CHECK_VALUE( ret_val );
		next_node = LIST_NEXT( cur_node );
		list_erase( &p_file_struct->_syn_read_block_list, cur_node );
		cur_node = next_node;
	}
	ret_val = sd_close_ex( file_id );
	CHECK_VALUE( ret_val );
	
	return SUCCESS;
}

_int32 fm_syn_handle_block_data( TMP_FILE *p_file_struct, BLOCK_DATA_BUFFER *p_block_data_buffer, _u32 block_disk_index, _u32 file_id )
{
	_int32 ret_val;
	_u32 read_len;//????

	LOG_DEBUG( "fm_syn_handle_block_data. block_disk_index: %u, file_id: %u", block_disk_index, file_id );	
	ret_val = sd_setfilepos( file_id, block_disk_index  *p_file_struct->_block_size + p_block_data_buffer->_block_offset );
	CHECK_VALUE( ret_val );

	ret_val = sd_read( file_id, p_block_data_buffer->_data_ptr, p_block_data_buffer->_data_length, &read_len );
	CHECK_VALUE( ret_val );

	LOG_DEBUG( "fm_syn_handle_block_data return. data_length: %u, read_len: %u", p_block_data_buffer->_data_length, read_len );	
	return SUCCESS;
}

_int32 get_syn_op_file_id( TMP_FILE *p_file_struct, _u32 *file_id )
{
	char file_full_path[MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN];
	_int32 ret_val = SUCCESS;
	
	LOG_DEBUG( "get_syn_op_file_id." );	
	ret_val = sd_memset( file_full_path, 0, MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN );
	CHECK_VALUE( ret_val );
	
	ret_val = sd_memcpy( file_full_path, p_file_struct->_file_path, p_file_struct->_file_path_len);
	CHECK_VALUE( ret_val );
	
	ret_val = sd_memcpy( file_full_path + p_file_struct->_file_path_len, p_file_struct->_file_name, p_file_struct->_file_name_len );
	CHECK_VALUE( ret_val );

	sd_assert( sd_strlen( file_full_path ) != 0 );
	ret_val = sd_open_ex( file_full_path, O_FS_RDWR, file_id );
	LOG_DEBUG( "sd_open_ex ret_val:%d.", ret_val );	

	CHECK_VALUE( ret_val );
	return SUCCESS;
}
void fm_cancel_list_rw_op( LIST *p_list )
{
	LIST_ITERATOR cur_node, end_node;
	cur_node = LIST_BEGIN( *p_list );
	end_node = LIST_END( *p_list );
	LOG_DEBUG( "fm_cancel_list_rw_op ." );

	while( cur_node != end_node )
	{
		BLOCK_DATA_BUFFER *p_list_rw_item = (BLOCK_DATA_BUFFER *)LIST_VALUE( cur_node );
		p_list_rw_item->_is_cancel = TRUE;
		cur_node = LIST_NEXT( cur_node );
	}
}

_int32 fm_handle_close_file( TMP_FILE *p_file_struct )
{	
	_int32 list_size = 0;
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "fm_handle_close_file:asyn_read_block_list=%u,syn_read_block_list=%u,write_block_list=%u." ,
		p_file_struct->_asyn_read_block_list._list_size,p_file_struct->_syn_read_block_list._list_size,p_file_struct->_write_block_list._list_size);	
	
	list_size = list_size + p_file_struct->_asyn_read_block_list._list_size;
	list_size = list_size + p_file_struct->_syn_read_block_list._list_size;
	list_size = list_size + p_file_struct->_write_block_list._list_size;
	if( list_size != 0 )
	{
		LOG_DEBUG( "fm_handle_close_file. list_size is not zero!" );	
		if(dm_get_enable_fast_stop()&& p_file_struct->_file_size >= MAX_FILE_SIZE_NEED_FLUSH_DATA_B4_CLOSE)
		{
			fm_cancel_list_rw_op( &p_file_struct->_write_block_list );
		}
		fm_cancel_list_rw_op( &p_file_struct->_asyn_read_block_list );	
		return SUCCESS;
	}

	if( p_file_struct->_is_opening )
	{
		ret_val = fm_cancel_open_msg( p_file_struct );
		sd_assert( ret_val == SUCCESS );
		return SUCCESS;
	}
	LOG_DEBUG( "fm implement close file ." );
	sd_assert( p_file_struct->_close_para_ptr != NULL );
	ret_val = fm_op_close( p_file_struct->_device_id, p_file_struct->_close_para_ptr );
	CHECK_VALUE( ret_val );
	return SUCCESS;
}

_int32 fm_op_close( _u32 device_id , MSG_FILE_CLOSE_PARA *p_close_rara )
{
	MSG_INFO msg_info = {};
	_u32 msgid;
	LOG_DEBUG( "fm_op_close ." );
	
	msg_info._device_id = device_id;
	msg_info._device_type = DEVICE_FS;
	msg_info._msg_priority = MSG_PRIORITY_NORMAL;
	msg_info._operation_parameter = NULL;
	msg_info._operation_type = OP_CLOSE;
	msg_info._user_data = p_close_rara;

	post_message(&msg_info, fm_callback, NOTICE_ONCE, 10000, &msgid);
	return SUCCESS;
}

_int32 fm_callback(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid)
{
	_u16 device_type = msg_info->_device_type;

	_u16 operation_type = msg_info->_operation_type;
	_int32 ret_val = SUCCESS;
	TMP_FILE *p_file_struct;
	BOOL is_closing = FALSE;
	LOG_DEBUG( "fm_callback." );	
	if(errcode!=SUCCESS)
	{
		LOG_ERROR( "fm_callback.%d",errcode );
	}
	sd_assert( device_type == DEVICE_FS );
	if( operation_type == OP_OPEN )//handle open operation
	{
		MSG_FILE_CREATE_PARA *p_msg_para = (MSG_FILE_CREATE_PARA *)msg_info->_user_data;
		p_file_struct = p_msg_para->_file_struct_ptr;
		is_closing = p_file_struct->_is_closing;
		p_file_struct->_is_file_created = TRUE;	
		
		p_file_struct->_is_opening = FALSE;
		ret_val = fm_create_callback( msg_info, errcode );
		CHECK_VALUE( ret_val );
		ret_val = msg_file_create_para_free_wrap( msg_info->_user_data );
		CHECK_VALUE( ret_val );	
	}
	else if( operation_type == OP_READ )
	{	
		MSG_FILE_RW_PARA *p_msg_para = (MSG_FILE_RW_PARA *)msg_info->_user_data;	
		p_file_struct = p_msg_para->_file_struct_ptr;
		is_closing = p_file_struct->_is_closing;
		
		if( p_msg_para->_op_type == FM_OP_BLOCK_TMP_READ )
		{
			ret_val = fm_write_callback( msg_info, errcode );
			CHECK_VALUE( ret_val );
		}
		else if( p_msg_para->_op_type == FM_OP_BLOCK_NORMAL_READ )
		{
			ret_val = fm_read_callback( msg_info, errcode );
			CHECK_VALUE( ret_val );			
		}
		ret_val = msg_file_rw_para_slab_free_wrap( msg_info->_user_data );
		CHECK_VALUE( ret_val );	
	}
	else if( operation_type == OP_WRITE )
	{	
		MSG_FILE_RW_PARA *p_msg_para = (MSG_FILE_RW_PARA *)msg_info->_user_data;	
		p_file_struct = p_msg_para->_file_struct_ptr;
		is_closing = p_file_struct->_is_closing;
		
		ret_val = fm_write_callback( msg_info, errcode );
		CHECK_VALUE( ret_val );
		ret_val = msg_file_rw_para_slab_free_wrap( msg_info->_user_data );
		CHECK_VALUE( ret_val );
	}
	else if( operation_type == OP_CLOSE )
	{
		ret_val = fm_close_callback( msg_info, errcode );
		CHECK_VALUE( ret_val );
		is_closing = FALSE;
		ret_val = msg_file_close_para_free_wrap( msg_info->_user_data );
		CHECK_VALUE( ret_val );
	}

	if( is_closing )
	{
		ret_val = fm_handle_close_file( p_file_struct );
		CHECK_VALUE( ret_val );
	}
	return SUCCESS;
}

_int32 fm_create_callback( const MSG_INFO *msg_info, _int32 errcode )
{
	MSG_FILE_CREATE_PARA *p_user_para = NULL;
	TMP_FILE *p_file_struct = NULL;
	notify_file_create p_create_call_back = NULL;
	_int32 ret_val = 0;
	LOG_DEBUG( "fm_create_callback." );

	p_user_para = (MSG_FILE_CREATE_PARA *)msg_info->_user_data;
	p_file_struct = p_user_para->_file_struct_ptr;
	p_create_call_back = (notify_file_create)p_user_para->_callback_func_ptr;
	p_file_struct->_device_id = msg_info->_device_id;
	LOG_DEBUG( "fm_create_callback:errcode=%d.filename=%s/%s,filesize=%llu",errcode,p_file_struct->_file_path,p_file_struct->_file_name,p_file_struct->_file_size);

	//if error, not retry
	if( p_file_struct->_is_closing )
	{
		ret_val = p_create_call_back( p_file_struct, p_user_para->_user_ptr, FM_CREAT_CLOSE_EVENT );
		CHECK_VALUE( ret_val );			
	}
	else
	{
		ret_val = p_create_call_back( p_file_struct, p_user_para->_user_ptr, errcode );
		CHECK_VALUE( ret_val );	
	}

	return SUCCESS;
}

_int32 fm_close_callback( const MSG_INFO *msg_info, _int32 errcode )
{
	MSG_FILE_CLOSE_PARA *p_user_para = NULL;
	TMP_FILE *p_file_struct = NULL;
	notify_file_close p_callback_func = NULL;
	_int32 ret_val = SUCCESS;

	p_user_para = msg_info->_user_data;
	p_file_struct = p_user_para->_file_struct_ptr;
	p_callback_func = p_user_para->_callback_func_ptr;
	
	LOG_DEBUG( "fm_close_callback:errcode=%d.filename=%s/%s,filesize=%llu",errcode,p_file_struct->_file_path,p_file_struct->_file_name,p_file_struct->_file_size);
	ret_val = p_callback_func( p_file_struct, p_user_para->_user_ptr, errcode );//file_info_notify_file_close
	CHECK_VALUE( ret_val );

	SAFE_DELETE( p_file_struct->_block_index_array._index_array_ptr ); 
	if( p_file_struct != NULL )
	{
		ret_val = tmp_file_free_wrap( p_file_struct );
		CHECK_VALUE( ret_val );
	}
	
	return SUCCESS;
}

_int32 fm_write_callback( const MSG_INFO *msg_info, _int32 errcode )
{
	_int32 ret_val = SUCCESS;
	MSG_FILE_RW_PARA *p_user_para = (MSG_FILE_RW_PARA *)msg_info->_user_data;
	TMP_FILE *p_file_struct = p_user_para->_file_struct_ptr;
	OP_PARA_FS_RW *p_rw_para = NULL;
	RANGE_DATA_BUFFER *p_data_buffer = NULL;
	
	//handle block read and block write
	notify_write_result p_write_call_back = (notify_write_result)p_user_para->_callback_func_ptr;
	BLOCK_DATA_BUFFER *p_block_data_buffer = p_user_para->_rw_para_ptr;

	//check the read block list
	BLOCK_DATA_BUFFER *p_list_block_item = NULL;
	LIST_ITERATOR cur_node = LIST_BEGIN( p_file_struct->_write_block_list );
	p_list_block_item = (BLOCK_DATA_BUFFER *)LIST_VALUE( cur_node );
	sd_assert( p_list_block_item == p_block_data_buffer );

	//check asyn write op
	p_rw_para = ( OP_PARA_FS_RW * )msg_info->_operation_parameter;

	p_data_buffer = (RANGE_DATA_BUFFER *)p_block_data_buffer->_range_buffer;
	
	LOG_DEBUG( "fm_write_callback:errcode=%d.filename=%s/%s,filesize=%llu,offset=%llu,expect_size=%u,operated_size=%u",
		errcode,p_file_struct->_file_path,p_file_struct->_file_name,p_file_struct->_file_size,p_rw_para->_operating_offset,p_rw_para->_expect_size,p_rw_para->_operated_size);
	

	
	if( errcode == SUCCESS )
	{
		sd_assert( p_rw_para->_expect_size == p_rw_para->_operated_size );
       	ret_val = drop_buffer_from_range_buffer( p_data_buffer );
		sd_assert( ret_val == SUCCESS );
		list_erase( &p_file_struct->_write_block_list, cur_node );	
		if( p_block_data_buffer->_is_call_back )
		{
			if( !p_file_struct->_is_closing )
			{
				LOG_DEBUG( "fm_write_callback return SUCCESS." );
				p_write_call_back( p_file_struct, p_user_para->_user_ptr, p_block_data_buffer->_buffer_para_ptr, SUCCESS );	 //notify_write_data_result
			}
			else
			{
				LOG_DEBUG( "fm_write_callback return FM_WRITE_CLOSE_EVENT." );
				p_write_call_back( p_file_struct, p_user_para->_user_ptr, p_block_data_buffer->_buffer_para_ptr, FM_WRITE_CLOSE_EVENT );
			}
			range_data_buffer_list_free_wrap( p_block_data_buffer->_buffer_para_ptr );
		}
		
		if( p_block_data_buffer->_is_tmp_block && msg_info->_operation_type == OP_WRITE )
		{
			//delete the temp buffer used in store the temp read block
			SAFE_DELETE( p_block_data_buffer->_data_ptr );
		}	
		block_data_buffer_free_wrap( p_block_data_buffer );	
	}
	else
	{
		p_block_data_buffer->_try_num++;
		LOG_DEBUG( "fm_write_callback error, errcode:%d.", errcode );
		if(( p_block_data_buffer->_try_num == FM_ERR_TRY_TIME ||p_block_data_buffer->_is_cancel)
			#ifdef MACOS
			||(errcode==28)  //No space left on device
			#endif
		)
		{
			list_erase( &p_file_struct->_write_block_list, cur_node );		
			LOG_ERROR( "fm_write_callback exceed try num.errode:%u.", errcode);			
			p_write_call_back( p_file_struct, p_user_para->_user_ptr, p_block_data_buffer->_buffer_para_ptr, errcode );
			block_data_buffer_free_wrap( p_block_data_buffer );
			//return SUCCESS;
		}	
	}
	
	fm_handle_write_block_list( p_file_struct );
	return SUCCESS;
}

_int32 fm_read_callback( const MSG_INFO *msg_info, _int32 errcode )
{
	MSG_FILE_RW_PARA *p_user_para = (MSG_FILE_RW_PARA *)msg_info->_user_data;		
	TMP_FILE *p_file_struct = p_user_para->_file_struct_ptr;
	OP_PARA_FS_RW *p_rw_para = NULL;
	
	//handle normal read
	notify_read_result p_read_call_back = (notify_read_result)p_user_para->_callback_func_ptr;
	BLOCK_DATA_BUFFER *p_block_data_buffer = (BLOCK_DATA_BUFFER *)p_user_para->_rw_para_ptr;
	RANGE_DATA_BUFFER *p_range_data_buffer = p_block_data_buffer->_buffer_para_ptr;
	RANGE_DATA_BUFFER *p_data_buffer = p_block_data_buffer->_buffer_para_ptr;
	//check the read block list
	LIST_ITERATOR cur_node = LIST_BEGIN( p_file_struct->_asyn_read_block_list );
	BLOCK_DATA_BUFFER *p_list_block_item = (BLOCK_DATA_BUFFER *)LIST_VALUE( cur_node );
	LOG_DEBUG( "fm_read_callback. p_list_block_item:0x%x, p_block_data_buffer:0x%x, p_user_para:0x%x/n",
		p_list_block_item, p_block_data_buffer, p_user_para );
	sd_assert( p_list_block_item == p_block_data_buffer );
	
	//check asyn read op
	p_rw_para = ( OP_PARA_FS_RW * )msg_info->_operation_parameter;
	//sd_assert( p_rw_para->_expect_size == p_rw_para->_operated_size );
	
	LOG_DEBUG( "fm_read_callback:errcode=%d.filename=%s/%s,filesize=%llu,offset=%llu,expect_size=%u,operated_size=%u",
		errcode,p_file_struct->_file_path,p_file_struct->_file_name,p_file_struct->_file_size,p_rw_para->_operating_offset,p_rw_para->_expect_size,p_rw_para->_operated_size);
	if( errcode == SUCCESS )
	{
		list_erase( &p_file_struct->_asyn_read_block_list, cur_node );	
		if( p_block_data_buffer->_is_call_back )
		{
			if( !p_file_struct->_is_closing )
			{
				LOG_DEBUG( "fm_read_callback return SUCCESS." );
				p_read_call_back( p_file_struct, p_user_para->_user_ptr, p_block_data_buffer->_buffer_para_ptr, SUCCESS, p_data_buffer->_data_length );		//notify_read_data_result
			}
			else
			{
				LOG_DEBUG( "fm_read_callback return FM_READ_CLOSE_EVENT." );
				p_read_call_back( p_file_struct, p_user_para->_user_ptr, p_block_data_buffer->_buffer_para_ptr, FM_READ_CLOSE_EVENT, p_data_buffer->_data_length );
			}
		}	
		block_data_buffer_free_wrap( p_block_data_buffer );
	}
	else
	{
		p_block_data_buffer->_try_num++;
		if(( p_block_data_buffer->_try_num == FM_ERR_TRY_TIME ||p_block_data_buffer->_is_cancel)
			#ifdef MACOS
			||(errcode==28)  //No space left on device
			#endif
		)
		{
			list_erase( &p_file_struct->_asyn_read_block_list, cur_node );	
			p_read_call_back( p_file_struct, p_user_para->_user_ptr, p_range_data_buffer, errcode, p_data_buffer->_data_length);
			block_data_buffer_free_wrap( p_block_data_buffer );
			LOG_ERROR( "fm_read_callback exceed try num." );
			//return SUCCESS;
		}		
	}
	
	fm_handle_asyn_read_block_list( p_file_struct );
	return SUCCESS;
}


BOOL fm_get_cfg_disk_block_index( DATA_BLOCK_INDEX_ARRAY *block_index_array , _u32 logic_block_index, _u32 *disk_block_index )
{
	_u32 block_index = 0;
	LOG_DEBUG( "fm_get_cfg_disk_block_index. logic_block_index:%u", logic_block_index );

	for( ; block_index < block_index_array->_valid_index_num; block_index++ )
	{
		if( block_index_array->_index_array_ptr[block_index]._logic_block_index == logic_block_index 
		&& block_index_array->_index_array_ptr[block_index]._is_valid == TRUE )
		{
			*disk_block_index = block_index;
			LOG_DEBUG( "fm_get_cfg_disk_block_index. logic_block_index:%u, disk_block_index:%u", logic_block_index, *disk_block_index );

			return TRUE;
		}
	}
	LOG_DEBUG( "fm_get_cfg_disk_block_index failed. " );
	return FALSE;
}

BOOL fm_check_cfg_block_index_array( TMP_FILE *p_file_struct )
{
	//File size in cfg must be the actual file size.
	_u64 disk_file_size, file_size;
	_u32 valid_block_num, total_block_num;
	_u32 file_id;
	BOOL is_check_ok;
	_int32 ret_val = SUCCESS;
	char full_name[MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN];
	
	LOG_DEBUG( "fm_check_cfg_block_index_array begin." );

    ret_val = sd_memcpy( full_name,p_file_struct->_file_path, p_file_struct->_file_path_len );
	sd_assert( ret_val == SUCCESS );

	ret_val = sd_memcpy( full_name + p_file_struct->_file_path_len ,p_file_struct->_file_name, p_file_struct->_file_name_len );
	sd_assert( ret_val == SUCCESS );
	
    full_name[p_file_struct->_file_path_len + p_file_struct->_file_name_len] = '\0';

	ret_val = sd_open_ex( full_name, O_FS_RDWR, &file_id ); 
	sd_assert( ret_val == SUCCESS );

	sd_filesize( file_id, &disk_file_size );
	sd_assert( ret_val == SUCCESS );

	ret_val = sd_close_ex( file_id ); 
	sd_assert( ret_val == SUCCESS );
	
	valid_block_num = p_file_struct->_block_index_array._valid_index_num;
	total_block_num = p_file_struct->_block_index_array._total_index_num;	
	file_size = valid_block_num * p_file_struct->_block_size;
	if(file_size>p_file_struct->_file_size) file_size=p_file_struct->_file_size;
	//if( disk_file_size  <= file_size )
	if( disk_file_size  >= file_size )    /* modify <= to >= by zyq @20101221 for ipadkankan */
	{
		is_check_ok = TRUE;
	}
	else
	{
		if( disk_file_size+p_file_struct->_block_size> file_size ) 
		{
			is_check_ok = TRUE;
		}
		else
		{
			is_check_ok = FALSE;
		}
	}
	LOG_DEBUG( "fm_check_cfg_block_index_array return %u.", (_u32)is_check_ok );
	return is_check_ok;
}


_u64 fm_get_tmp_filesize( TMP_FILE *p_file_struct )
{
    _u64  filesize = 0;
    
	_int32 ret_val = sd_filesize( p_file_struct->_device_id, &filesize );
	CHECK_VALUE(ret_val);
	
	LOG_DEBUG( "fm_get_tmp_filesize, p_file_struct:0x%x ,get fileid: %u, filesize:%llu, ret_val:%d .", p_file_struct, p_file_struct->_device_id, filesize, ret_val );

	return filesize;
}

//#endif  /* !XUNLEI_MODE */

