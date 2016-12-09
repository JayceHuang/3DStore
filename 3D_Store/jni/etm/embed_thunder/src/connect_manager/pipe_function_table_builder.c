#include "utility/logid.h"
#define LOGID LOGID_CONNECT_MANAGER
#include "utility/logger.h"

#include "pipe_function_table_builder.h"
#include "pipe_interface.h"
#include "data_pipe.h"

#include "http_data_pipe/http_data_pipe_interface.h"
#include "ftp_data_pipe/ftp_data_pipe_interface.h"
#include "p2p_data_pipe/p2p_pipe_interface.h"
#include "data_manager/data_manager_interface.h"

#ifdef ENABLE_BT
#include "bt_download/bt_task/bt_task_interface.h"
#include "bt_download/bt_data_manager/bt_range_switch.h"
#include "bt_download/bt_data_manager/bt_data_manager_interface.h"
#include "bt_download/bt_data_pipe/bt_pipe_interface.h"

#endif /* #ifdef ENABLE_BT  */

void build_common_pipe_function_table(void)
{
	void **p_func_table = pft_get_common_pipe_function_table();
	
	pft_register_change_range_handler(p_func_table, common_pipe_change_range_handle);
	pft_register_get_dispatcher_range_handler(p_func_table, common_pipe_get_dispatcher_range);
	pft_register_set_dispatcher_range_handler(p_func_table, common_pipe_set_dispatcher_range);
	pft_register_get_file_size_handler(p_func_table, common_pipe_get_file_size);
	pft_register_set_file_size_handler(p_func_table, common_pipe_set_file_size);	
	pft_register_get_data_buffer_handler(p_func_table, common_pipe_get_data_buffer_handler);	
	pft_register_free_data_buffer_handler(p_func_table, common_pipe_free_data_buffer_handler);	
	pft_register_put_data_handler(p_func_table, common_pipe_put_data_handler);
	pft_register_read_data_handler(p_func_table, common_pipe_read_data_handler);	
	pft_register_notify_dispatch_data_finish_handler(p_func_table, common_pipe_notify_dispatch_data_finish_handler);	
	pft_register_get_file_type_handler(p_func_table, common_pipe_get_file_type_handler);
	pft_register_get_checked_range_list_handler(p_func_table, common_pipe_get_checked_range_list_handler);
}

#ifdef ENABLE_BT 

void build_speedup_pipe_function_table(void)
{	
	void **p_func_table = pft_get_speedup_pipe_function_table();
	
	pft_register_change_range_handler(p_func_table, speedup_pipe_change_range_handle);	
	pft_register_get_dispatcher_range_handler(p_func_table, speedup_pipe_get_dispatcher_range);	
	pft_register_set_dispatcher_range_handler(p_func_table, speedup_pipe_set_dispatcher_range);	
	pft_register_get_file_size_handler(p_func_table, speedup_pipe_get_file_size);	
	pft_register_set_file_size_handler(p_func_table, speedup_pipe_set_file_size);
	pft_register_get_data_buffer_handler(p_func_table, speedup_pipe_get_data_buffer_handler);	
	pft_register_free_data_buffer_handler(p_func_table, speedup_pipe_free_data_buffer_handler);	
	pft_register_put_data_handler(p_func_table, speedup_pipe_put_data_handler);	
	pft_register_read_data_handler(p_func_table, speedup_pipe_read_data_handler);
	pft_register_notify_dispatch_data_finish_handler(p_func_table, speedup_pipe_notify_dispatch_data_finish_handler);
	pft_register_get_file_type_handler(p_func_table, speedup_pipe_get_file_type_handler);
	pft_register_get_checked_range_list_handler(p_func_table, speedup_pipe_get_checked_range_list_handler);
}

void build_bt_pipe_function_table(void)
{
	void **p_func_table = pft_get_bt_pipe_function_table();
	
	pft_register_change_range_handler( p_func_table, bt_pipe_change_range_handle );
	
	pft_register_get_dispatcher_range_handler( p_func_table, bt_pipe_get_dispatcher_range );
	
	pft_register_set_dispatcher_range_handler( p_func_table, bt_pipe_set_dispatcher_range );
	
	
	pft_register_get_file_size_handler( p_func_table, bt_pipe_get_file_size );
	
	pft_register_set_file_size_handler( p_func_table, bt_pipe_set_file_size );
	
	pft_register_get_data_buffer_handler( p_func_table, bt_pipe_get_data_buffer_handler );
	
	
	pft_register_free_data_buffer_handler( p_func_table, bt_pipe_free_data_buffer_handler );
	
	pft_register_put_data_handler( p_func_table, bt_pipe_put_data_handler );

	pft_register_read_data_handler( p_func_table, bt_pipe_read_data_handler );
	
	pft_register_notify_dispatch_data_finish_handler( p_func_table, bt_pipe_notify_dispatch_data_finish_handler );
	
	pft_register_get_file_type_handler( p_func_table, bt_pipe_notify_get_file_type_handler );

	pft_register_get_checked_range_list_handler( p_func_table, bt_pipe_get_checked_range_list_handler );

}
#endif /* #ifdef ENABLE_BT  */

void build_globla_pipe_function_table(void)
{
	build_common_pipe_function_table();

#ifdef ENABLE_BT 
	build_speedup_pipe_function_table();
	build_bt_pipe_function_table();
#endif /* #ifdef ENABLE_BT  */
}

void clear_globla_pipe_function_table(void)
{
	void **p_func_table = pft_get_common_pipe_function_table();
	pft_clear_func_table( p_func_table );

#ifdef ENABLE_BT 
	p_func_table = pft_get_speedup_pipe_function_table();
	pft_clear_func_table( p_func_table );
	
	p_func_table = pft_get_bt_pipe_function_table();
	pft_clear_func_table( p_func_table );
#endif /* #ifdef ENABLE_BT  */
}

_int32 common_pipe_change_range_handle( struct tagDATA_PIPE *p_data_pipe, const RANGE *p_range, BOOL cancel_flag )
{
	_int32 ret_val = SUCCESS;
	PIPE_TYPE pipe_type = p_data_pipe->_data_pipe_type;
	LOG_DEBUG( "common_pipe_change_range_handler. pipe_ptr:0x%x, assign range(%u,%u), cancel_flag:%d",
		p_data_pipe, p_range->_index, p_range->_num, cancel_flag );	

	if( pipe_type == P2P_PIPE )
	{
 		LOG_DEBUG( "common_pipe_change_range to p2p pipe." ); 
		ret_val = p2p_pipe_change_ranges( p_data_pipe, p_range, cancel_flag );
	}
	else if( pipe_type == HTTP_PIPE )
	{
 		LOG_DEBUG( "common_pipe_change_range to http pipe." ); 
		ret_val = http_pipe_change_ranges( p_data_pipe, p_range );
	}
	else if( pipe_type == FTP_PIPE )
	{
		LOG_DEBUG( "common_pipe_change_range to ftp pipe." ); 
		ret_val = ftp_pipe_change_ranges( p_data_pipe, p_range );
	}
    /*
#ifdef ENABLE_BT
	else if( pipe_type == BT_PIPE )
	{
		LOG_DEBUG( "common_pipe_change_range to ftp pipe." ); 
		ret_val = bt_pipe_change_ranges( p_data_pipe, p_range, cancel_flag );
	}
#endif
*/
	else
	{
		sd_assert( FALSE );
	}
	return ret_val;
}


_int32 common_pipe_get_dispatcher_range( struct tagDATA_PIPE *p_data_pipe, const RANGE *p_dispatcher_range, void *p_pipe_range )
{
	RANGE *p_file_range = (RANGE *) p_pipe_range;
#ifdef _LOGGER
	PIPE_INTERFACE *p_pipe_interface = p_data_pipe->_p_pipe_interface;
	sd_assert( p_pipe_interface != NULL );
	sd_assert( p_pipe_interface->_range_switch_handler == NULL );
#endif

	sd_assert( p_dispatcher_range != NULL );

	p_file_range->_index = p_dispatcher_range->_index;
	p_file_range->_num = p_dispatcher_range->_num;
	
	LOG_DEBUG( "common_pipe_get_dispatcher_range. pipe_ptr:0x%x, assign range(%u,%u)",
		p_data_pipe, p_file_range->_index, p_file_range->_num );	

	return SUCCESS;
}

_int32 common_pipe_set_dispatcher_range( struct tagDATA_PIPE *p_data_pipe, const void *p_pipe_range, RANGE *p_dispatcher_range )
{
	RANGE *p_common_range = (RANGE *) p_pipe_range;
#ifdef _LOGGER
	PIPE_INTERFACE *p_pipe_interface = p_data_pipe->_p_pipe_interface;
	sd_assert( p_pipe_interface != NULL );
	sd_assert( p_pipe_interface->_range_switch_handler == NULL );
#endif
	
	sd_assert( p_data_pipe->_data_pipe_type != BT_PIPE );

	p_dispatcher_range->_index = p_common_range->_index;
	p_dispatcher_range->_num = p_common_range->_num;	

	LOG_DEBUG( "common_pipe_set_dispatcher_range. pipe_ptr:0x%x, assign range(%u,%u)",
		p_data_pipe, p_dispatcher_range->_index, p_dispatcher_range->_num );	
	
	return SUCCESS;
}

_u64 common_pipe_get_file_size( struct tagDATA_PIPE *p_data_pipe )
{
	DATA_MANAGER *p_common_data_manager = (DATA_MANAGER *)p_data_pipe->_p_data_manager;
	_u64 file_size = 0;
	sd_assert( p_data_pipe->_data_pipe_type != BT_PIPE );
	file_size = dm_get_file_size( p_common_data_manager );
	
	LOG_DEBUG( "common_pipe_get_file_size, pipe_ptr:0x%x, file_size:%llu", 
		p_data_pipe, file_size  );	
	
	return file_size;
}

_int32 common_pipe_set_file_size( struct tagDATA_PIPE *p_data_pipe, _u64 filesize)
{
	DATA_MANAGER *p_common_data_manager = (DATA_MANAGER *)p_data_pipe->_p_data_manager;
	sd_assert(p_data_pipe->_data_pipe_type != BT_PIPE);

	BOOL force = ((p_data_pipe->_data_pipe_type == FTP_PIPE) ? TRUE: FALSE);
	LOG_DEBUG("common_pipe_set_file_size, pipe_ptr:0x%x, file_size:%llu, force=%d", 
		p_data_pipe, filesize , force);
	
	return dm_set_file_size(p_common_data_manager, filesize, force);
}


_int32 common_pipe_get_data_buffer_handler( struct tagDATA_PIPE *p_data_pipe, 
	char **pp_data_buffer, _u32 *p_data_len )
{
	DATA_MANAGER *p_common_data_manager = (DATA_MANAGER *)p_data_pipe->_p_data_manager;
	sd_assert( p_data_pipe->_data_pipe_type != BT_PIPE );
	LOG_DEBUG( "common_pipe_get_data_buffer_handler, pipe_ptr:0x%x, data_len:%u", 
		p_data_pipe, *p_data_len  );	
	return dm_get_data_buffer( p_common_data_manager, pp_data_buffer, p_data_len );	
}


_int32 common_pipe_free_data_buffer_handler( struct tagDATA_PIPE *p_data_pipe, 
	char **pp_data_buffer, _u32 data_len) 
{
	DATA_MANAGER *p_common_data_manager = (DATA_MANAGER *)p_data_pipe->_p_data_manager;
	sd_assert( p_data_pipe->_data_pipe_type != BT_PIPE );
	LOG_DEBUG( "common_pipe_get_data_buffer_handler, pipe_ptr:0x%x, data_len:%u", 
		p_data_pipe, data_len  );	

	return dm_free_data_buffer( p_common_data_manager, pp_data_buffer, data_len ); 
}


_int32 common_pipe_put_data_handler( struct tagDATA_PIPE *p_data_pipe,  
	const RANGE *p_range, char **pp_data_buffer, _u32 data_len,_u32 buffer_len,
	RESOURCE *p_resource ) 
{
	DATA_MANAGER *p_common_data_manager = (DATA_MANAGER *)p_data_pipe->_p_data_manager;
	sd_assert( p_data_pipe->_data_pipe_type != BT_PIPE );
	LOG_DEBUG( "common_pipe_put_data_handler:pipe_ptr:0x%x, range:[%u,%u], data_len:%u, buffer_len:%u ,speed:%u .", 
		p_data_pipe, p_range->_index, p_range->_num, data_len, buffer_len , p_data_pipe->_dispatch_info._speed);		

    return dm_put_data( p_common_data_manager, p_range, pp_data_buffer, data_len, buffer_len, p_data_pipe, p_resource ); 
}


_int32 common_pipe_read_data_handler( struct tagDATA_PIPE *p_data_pipe, RANGE_DATA_BUFFER *p_data_buffer, 
	notify_read_result p_call_back, void *p_user ) 
{
	DATA_MANAGER *p_common_data_manager = (DATA_MANAGER *)p_data_pipe->_p_data_manager;
	sd_assert( p_data_pipe->_data_pipe_type != BT_PIPE );
	LOG_DEBUG( "common_pipe_read_data_handler:pipe_ptr:0x%x, range:[%u,%u], data_len:%u, buffer_len:%u ", 
		p_data_pipe, p_data_buffer->_range._index, p_data_buffer->_range._num, p_data_buffer->_data_length, p_data_buffer->_buffer_length  );		

	return dm_asyn_read_data_buffer( p_common_data_manager, p_data_buffer, p_call_back, p_user ); 
}

_int32 common_pipe_notify_dispatch_data_finish_handler( struct tagDATA_PIPE *p_data_pipe ) 
{
	DATA_MANAGER *p_common_data_manager = (DATA_MANAGER *)p_data_pipe->_p_data_manager;
	sd_assert( p_data_pipe->_data_pipe_type != BT_PIPE );
	LOG_DEBUG( "common_pipe_notify_dispatch_data_finish_handler, pipe_ptr:0x%x ", p_data_pipe  );		

	return dm_notify_dispatch_data_finish( p_common_data_manager, p_data_pipe ); 
}


_int32 common_pipe_get_file_type_handler( struct tagDATA_PIPE *p_data_pipe ) 
{
	_int32 ret_val;
	DATA_MANAGER *p_common_data_manager = (DATA_MANAGER *)p_data_pipe->_p_data_manager;
	sd_assert( p_data_pipe->_data_pipe_type != BT_PIPE );
	LOG_DEBUG( "common_pipe_get_file_type_handler, pipe_ptr:0x%x ", p_data_pipe  );		

	ret_val = (_int32)dm_get_file_type( p_common_data_manager ); 
	return ret_val;
}

RANGE_LIST* common_pipe_get_checked_range_list_handler( struct tagDATA_PIPE *p_data_pipe ) 
{
	DATA_MANAGER *p_common_data_manager = (DATA_MANAGER *)p_data_pipe->_p_data_manager;
	sd_assert( p_data_pipe->_data_pipe_type != BT_PIPE );
	LOG_DEBUG( "common_pipe_get_file_type_handler, pipe_ptr:0x%x ", p_data_pipe  );		

	return dm_get_checked_range_list( p_common_data_manager );
}

#ifdef ENABLE_BT 

_int32 speedup_pipe_change_range_handle( struct tagDATA_PIPE *p_data_pipe, const RANGE *p_range, BOOL cancel_flag )
{
	_int32 ret_val = SUCCESS;
	RANGE file_range;
	PIPE_INTERFACE *p_pipe_interface =( PIPE_INTERFACE * )p_data_pipe->_p_pipe_interface;

	sd_assert( p_pipe_interface != NULL );
	sd_assert( p_pipe_interface->_range_switch_handler != NULL );
	
	if( p_pipe_interface == NULL 
		|| p_pipe_interface->_range_switch_handler == NULL )
	{
		return SUCCESS;
	}	

	ret_val = brs_padding_range_to_file_range( (BT_RANGE_SWITCH *)p_pipe_interface->_range_switch_handler, 
		p_pipe_interface->_file_index, p_range, &file_range );
	if( ret_val != SUCCESS )
	{
		file_range._index = 0;
		file_range._num = 0;
	}
	
	LOG_DEBUG( "speedup_pipe_change_range_handle. pipe_ptr:0x%x, p_range(%u,%u), file_range:[%u, %u]",
		p_data_pipe, p_range->_index, p_range->_num, file_range._index, file_range._num );	
		

	ret_val = common_pipe_change_range_handle( p_data_pipe, &file_range, cancel_flag );
	CHECK_VALUE( ret_val );
	return SUCCESS;
}

_int32 speedup_pipe_get_dispatcher_range( struct tagDATA_PIPE *p_data_pipe, const RANGE *p_dispatcher_range, void *p_pipe_range )
{
	_int32 ret_val = SUCCESS;
	RANGE *p_file_range = (RANGE *) p_pipe_range;
	PIPE_INTERFACE *p_pipe_interface = p_data_pipe->_p_pipe_interface;
	
	sd_assert( p_pipe_interface != NULL );
	sd_assert( p_pipe_interface->_range_switch_handler != NULL );

	ret_val = brs_padding_range_to_file_range( p_pipe_interface->_range_switch_handler, 
		p_pipe_interface->_file_index, p_dispatcher_range, p_file_range );
	if( ret_val != SUCCESS )
	{
		p_file_range->_index = 0;
		p_file_range->_num = 0;
	}
	
	LOG_DEBUG( "speedup_pipe_get_dispatcher_range. pipe_ptr:0x%x, p_dispatcher_range(%u,%u), file_range:[%u, %u]",
		p_data_pipe, p_dispatcher_range->_index, p_dispatcher_range->_num, p_file_range->_index, p_file_range->_num );	
		

	return SUCCESS;
}

_int32 speedup_pipe_set_dispatcher_range( struct tagDATA_PIPE *p_data_pipe, const void *p_pipe_range, RANGE *p_dispatcher_range )
{
	_int32 ret_val = SUCCESS;
	RANGE *p_file_range = (RANGE *) p_pipe_range;
	PIPE_INTERFACE *p_pipe_interface = p_data_pipe->_p_pipe_interface;
	
	sd_assert( p_data_pipe->_data_pipe_type != BT_PIPE );
	sd_assert( p_pipe_interface != NULL );
	sd_assert( p_pipe_interface->_range_switch_handler != NULL );
	
	
	ret_val = brs_file_range_to_padding_range( p_pipe_interface->_range_switch_handler, 
		p_pipe_interface->_file_index, p_file_range, p_dispatcher_range  );
	sd_assert( ret_val == SUCCESS );
	
	LOG_DEBUG( "speedup_pipe_set_dispatcher_range:file_index:%u, p_file_range:[%u,%u], p_dispatcher_range:[%u,%u]",
		p_pipe_interface->_file_index, 
		p_file_range->_index, p_file_range->_num,
		p_dispatcher_range->_index, p_dispatcher_range->_num );	

	return SUCCESS;
}

_u64 speedup_pipe_get_file_size( struct tagDATA_PIPE *p_data_pipe )
{
	struct tagBT_DATA_MANAGER *p_bt_data_manager = (struct tagBT_DATA_MANAGER *)p_data_pipe->_p_data_manager;
	_u32 file_index = 0;
	_u64 file_size = 0;
	PIPE_INTERFACE *p_pipe_interface = p_data_pipe->_p_pipe_interface;
	sd_assert( p_data_pipe->_data_pipe_type != BT_PIPE );
	sd_assert( p_pipe_interface != NULL );

	file_index = pi_get_file_index( p_pipe_interface );
	file_size =  bdm_get_sub_file_size( p_bt_data_manager, file_index );
	LOG_DEBUG( "speedup_pipe_get_file_size: pipe_ptr:0x%x, filesize:%u", p_data_pipe, file_size );		

	return file_size;
}

_int32 speedup_pipe_set_file_size( struct tagDATA_PIPE *p_data_pipe, _u64 filesize)
{
	struct tagBT_DATA_MANAGER *p_bt_data_manager = (struct tagBT_DATA_MANAGER *)p_data_pipe->_p_data_manager;
	_u32 file_index = 0;
	PIPE_INTERFACE *p_pipe_interface = p_data_pipe->_p_pipe_interface;
	sd_assert( p_data_pipe->_data_pipe_type != BT_PIPE );
	sd_assert( p_pipe_interface != NULL );

	file_index = pi_get_file_index( p_pipe_interface );
	
	LOG_DEBUG( "speedup_pipe_set_file_size: pipe_ptr:0x%x, filesize:%u", p_data_pipe, filesize );		
	return bdm_set_sub_file_size( p_bt_data_manager, file_index, filesize );	
}


_int32 speedup_pipe_get_data_buffer_handler( struct tagDATA_PIPE *p_data_pipe, 
	char **pp_data_buffer, _u32 *p_data_len ) 
{
	struct tagBT_DATA_MANAGER *p_bt_data_manager = (struct tagBT_DATA_MANAGER *)p_data_pipe->_p_data_manager;
	sd_assert( p_data_pipe->_data_pipe_type != BT_PIPE );
	LOG_DEBUG( "speedup_pipe_get_data_buffer_handler:pipe_ptr:0x%x, data_len:%u ", p_data_pipe, *p_data_len );		

	return bdm_get_data_buffer( p_bt_data_manager, pp_data_buffer, p_data_len );	
}

_int32 speedup_pipe_free_data_buffer_handler( struct tagDATA_PIPE *p_data_pipe, 
	char **pp_data_buffer, _u32 data_len) 
{
	struct tagBT_DATA_MANAGER *p_bt_data_manager = (struct tagBT_DATA_MANAGER *)p_data_pipe->_p_data_manager;
	sd_assert( p_data_pipe->_data_pipe_type != BT_PIPE );
	LOG_DEBUG( "speedup_pipe_free_data_buffer_handler:pipe_ptr:0x%x, data_len:%u ", p_data_pipe, data_len );		

	return bdm_free_data_buffer( p_bt_data_manager, pp_data_buffer, data_len );	
}

_int32 speedup_pipe_put_data_handler( struct tagDATA_PIPE *p_data_pipe,  
	const RANGE *p_range, char **pp_data_buffer, _u32 data_len,_u32 buffer_len,
	RESOURCE *p_resource ) 
{
	struct tagBT_DATA_MANAGER *p_bt_data_manager = (struct tagBT_DATA_MANAGER *)p_data_pipe->_p_data_manager;
	_u32 file_index = 0;
	PIPE_INTERFACE *p_pipe_interface = p_data_pipe->_p_pipe_interface;
	
	sd_assert( p_data_pipe->_data_pipe_type != BT_PIPE );
	sd_assert( p_pipe_interface != NULL );
	LOG_DEBUG( "speedup_pipe_put_data_handler:pipe_ptr:0x%x, range:[%u,%u], data_len:%u, buffer_len:%u ", 
		p_data_pipe, p_range->_index, p_range->_num, data_len, buffer_len  );		

	file_index = pi_get_file_index( p_pipe_interface );
    return bdm_speedup_pipe_put_data( p_bt_data_manager, file_index, p_range, pp_data_buffer, data_len, buffer_len, p_data_pipe, p_resource ); 
}

_int32 speedup_pipe_read_data_handler(  struct tagDATA_PIPE *p_data_pipe, RANGE_DATA_BUFFER *p_data_buffer, 
	notify_read_result p_call_back, void *p_user  )
{
	struct tagBT_DATA_MANAGER *p_bt_data_manager = (struct tagBT_DATA_MANAGER *)p_data_pipe->_p_data_manager;
	_u32 file_index = 0;
	PIPE_INTERFACE *p_pipe_interface = p_data_pipe->_p_pipe_interface;
	
	sd_assert( p_data_pipe->_data_pipe_type != BT_PIPE );
	sd_assert( p_pipe_interface != NULL );
	LOG_DEBUG( "speedup_pipe_read_data_handler:pipe_ptr:0x%x, range:[%u,%u], data_len:%u, buffer_len:%u ", 
		p_data_pipe, p_data_buffer->_range._index, p_data_buffer->_range._num, p_data_buffer->_data_length, p_data_buffer->_buffer_length  );		

	file_index = pi_get_file_index( p_pipe_interface );
	return bdm_asyn_read_data_buffer( p_bt_data_manager, p_data_buffer , p_call_back, p_user, file_index ); 	
}

_int32 speedup_pipe_notify_dispatch_data_finish_handler( struct tagDATA_PIPE *p_data_pipe ) 
{
	struct tagBT_DATA_MANAGER *p_bt_data_manager = (struct tagBT_DATA_MANAGER *)p_data_pipe->_p_data_manager;

	sd_assert( p_data_pipe->_data_pipe_type != BT_PIPE );
	LOG_DEBUG( "speedup_pipe_notify_dispatch_data_finish_handler, pipe_ptr:0x%x ", p_data_pipe  );		

	return bdm_notify_dispatch_data_finish( p_bt_data_manager, p_data_pipe ); 
}

_int32 speedup_pipe_get_file_type_handler( struct tagDATA_PIPE *p_data_pipe ) 
{
	struct tagBT_DATA_MANAGER *p_bt_data_manager = (struct tagBT_DATA_MANAGER *)p_data_pipe->_p_data_manager;
	_u32 file_index = 0;
	PIPE_INTERFACE *p_pipe_interface = p_data_pipe->_p_pipe_interface;
	_int32 ret_val = 0;
	sd_assert( p_data_pipe->_data_pipe_type != BT_PIPE );
	sd_assert( p_pipe_interface != NULL );

	file_index = pi_get_file_index( p_pipe_interface );
	
	ret_val = ( PIPE_FILE_TYPE )bdm_get_sub_file_type( p_bt_data_manager, file_index ); 
	LOG_DEBUG( "speedup_pipe_get_file_type_handler, pipe_ptr:0x%x, file_type:%u ", p_data_pipe, ret_val  );		

	return ret_val;
}


RANGE_LIST* speedup_pipe_get_checked_range_list_handler( struct tagDATA_PIPE *p_data_pipe ) 
{
	struct tagBT_DATA_MANAGER *p_bt_data_manager = (struct tagBT_DATA_MANAGER *)p_data_pipe->_p_data_manager;
	_u32 file_index = 0;
	PIPE_INTERFACE *p_pipe_interface = p_data_pipe->_p_pipe_interface;
	sd_assert( p_data_pipe->_data_pipe_type != BT_PIPE );
	sd_assert( p_pipe_interface != NULL );

	file_index = pi_get_file_index( p_pipe_interface );
	
	LOG_DEBUG( "speedup_pipe_get_checked_range_list_handler, pipe_ptr:0x%x ", p_data_pipe  );		

	return bdm_get_check_range_list( p_bt_data_manager, file_index );
}

_int32 bt_pipe_change_range_handle( struct tagDATA_PIPE *p_data_pipe, const RANGE *p_range, BOOL cancel_flag )
{
    
#ifdef ENABLE_BT_PROTOCOL
	_int32 ret_val = SUCCESS;
	PIPE_INTERFACE *p_pipe_interface =( PIPE_INTERFACE * )p_data_pipe->_p_pipe_interface;

	sd_assert( p_pipe_interface != NULL );
	sd_assert( p_pipe_interface->_range_switch_handler != NULL );

	if( p_pipe_interface == NULL 
		|| p_pipe_interface->_range_switch_handler == NULL )
	{
		return SUCCESS;
	}

	sd_assert( p_data_pipe->_data_pipe_type == BT_PIPE );

	ret_val = bt_pipe_change_ranges( p_data_pipe, p_range, cancel_flag );
	CHECK_VALUE( ret_val );
	LOG_DEBUG( "bt_pipe_change_range_handle. pipe_ptr:0x%x, p_range(%u,%u)",
		p_data_pipe, p_range->_index, p_range->_num );	
#endif			
	return SUCCESS;	
}

_int32 bt_pipe_get_dispatcher_range( struct tagDATA_PIPE *p_data_pipe, const RANGE *p_dispatcher_range, void *p_pipe_range )
{
	_int32 ret_val = SUCCESS;
	BT_RANGE *p_bt_range = (BT_RANGE *) p_pipe_range;
	PIPE_INTERFACE *p_pipe_interface = p_data_pipe->_p_pipe_interface;
	
	sd_assert( p_pipe_interface != NULL );
	sd_assert( p_pipe_interface->_range_switch_handler != NULL );
	LOG_DEBUG( "bt_pipe_get_dispatcher_range"  );		

	ret_val = brs_padding_range_to_bt_range( p_pipe_interface->_range_switch_handler, p_dispatcher_range, p_bt_range );
	sd_assert( ret_val == SUCCESS );

	LOG_DEBUG( "bt_pipe_get_dispatcher_range. pipe_ptr:0x%x, p_dispatcher_range(%u,%u), p_bt_range:[%llu, %u]",
		p_data_pipe, p_dispatcher_range->_index, p_dispatcher_range->_num, p_bt_range->_pos, p_bt_range->_length );	
		

	return SUCCESS;
}

_int32 bt_pipe_set_dispatcher_range( struct tagDATA_PIPE *p_data_pipe, const void *p_pipe_range, RANGE *p_dispatcher_range )
{
	_int32 ret_val = SUCCESS;
	BT_RANGE *p_bt_range = (BT_RANGE *) p_pipe_range;
	PIPE_INTERFACE *p_pipe_interface = p_data_pipe->_p_pipe_interface;
	
	
	sd_assert( p_pipe_interface != NULL );
	sd_assert( p_pipe_interface->_range_switch_handler != NULL );
	sd_assert( p_data_pipe->_data_pipe_type == BT_PIPE );

	ret_val = brs_bt_range_to_padding_range( p_pipe_interface->_range_switch_handler, p_bt_range, p_dispatcher_range );
	//sd_assert( ret_val == SUCCESS );
	LOG_ERROR( "brs_bt_range_to_padding_range err:%d", ret_val );
	if( ret_val != SUCCESS )
	{
		p_dispatcher_range->_index = 0;
		p_dispatcher_range->_num = 0;
	}

	LOG_DEBUG( "bt_pipe_set_dispatcher_range:pipe_ptr:0x%x, p_bt_range:[%llu,%llu], p_dispatcher_range:[%u,%u]",
		p_data_pipe, p_bt_range->_pos, p_bt_range->_length,
		p_dispatcher_range->_index, p_dispatcher_range->_num );	

	return SUCCESS;
}

_u64 bt_pipe_get_file_size( struct tagDATA_PIPE *p_data_pipe )
{
	struct tagBT_DATA_MANAGER *p_bt_data_manager = (struct tagBT_DATA_MANAGER *)p_data_pipe->_p_data_manager;

	sd_assert( p_data_pipe->_data_pipe_type == BT_PIPE );
	
	LOG_DEBUG( "bt_pipe_get_file_size, pipe_ptr:0x%x", p_data_pipe  );	
	
	return bdm_get_file_size( p_bt_data_manager);
}

_int32 bt_pipe_set_file_size( struct tagDATA_PIPE *p_data_pipe, _u64 filesize)
{
	sd_assert( p_data_pipe->_data_pipe_type == BT_PIPE );
	
	LOG_DEBUG( "bt_pipe_get_file_size, pipe_ptr:0x%x, file_size:%llu", p_data_pipe, filesize  );	

	sd_assert( FALSE );
	
	return SUCCESS;
}

_int32 bt_pipe_get_data_buffer_handler( struct tagDATA_PIPE *p_data_pipe, 
	char **pp_data_buffer, _u32 *p_data_len ) 
{
	struct tagBT_DATA_MANAGER *p_bt_data_manager = (struct tagBT_DATA_MANAGER *)p_data_pipe->_p_data_manager;
	sd_assert( p_data_pipe->_data_pipe_type == BT_PIPE );
	LOG_DEBUG( "bt_pipe_get_data_buffer_handler:pipe_ptr:0x%x, data_len:%u ", p_data_pipe, *p_data_len );		

	return bdm_get_data_buffer( p_bt_data_manager, pp_data_buffer, p_data_len );	
}

_int32 bt_pipe_free_data_buffer_handler( struct tagDATA_PIPE *p_data_pipe, 
	char **pp_data_buffer, _u32 data_len) 
{
	struct tagBT_DATA_MANAGER *p_bt_data_manager = (struct tagBT_DATA_MANAGER *)p_data_pipe->_p_data_manager;
	sd_assert( p_data_pipe->_data_pipe_type == BT_PIPE );
	LOG_DEBUG( "pi_free_data_buffer:pipe_ptr:0x%x, data_len:%u ", p_data_pipe, data_len );		

	return bdm_free_data_buffer( p_bt_data_manager, pp_data_buffer, data_len ); 
}

_int32 bt_pipe_put_data_handler( struct tagDATA_PIPE *p_data_pipe,  
	const RANGE *p_range, char **pp_data_buffer, _u32 data_len,_u32 buffer_len,
	RESOURCE *p_resource ) 
{
	struct tagBT_DATA_MANAGER *p_bt_data_manager = (struct tagBT_DATA_MANAGER *)p_data_pipe->_p_data_manager;
	sd_assert( p_data_pipe->_data_pipe_type == BT_PIPE );
	
	LOG_DEBUG( "bt_pipe_put_data_handler:pipe_ptr:0x%x, range:[%u,%u], data_len:%u, buffer_len:%u ", 
		p_data_pipe, p_range->_index, p_range->_num, data_len, buffer_len  );	
	
    return bdm_bt_pipe_put_data( p_bt_data_manager, p_range, pp_data_buffer, data_len, buffer_len, p_data_pipe, p_resource ); 
}

_int32 bt_pipe_read_data_handler( struct tagDATA_PIPE *p_data_pipe, RANGE_DATA_BUFFER *p_data_buffer, 
	notify_read_result p_call_back, void *p_user )
{
	sd_assert( p_data_pipe->_data_pipe_type == BT_PIPE );
	LOG_DEBUG( "pi_pipe_read_data_buffer:pipe_ptr:0x%x, range:[%u,%u], data_len:%u, buffer_len:%u ", 
		p_data_pipe, p_data_buffer->_range._index, p_data_buffer->_range._num, p_data_buffer->_data_length, p_data_buffer->_buffer_length  );		
	
	sd_assert( FALSE );

	return SUCCESS;
}


_int32 bt_pipe_notify_dispatch_data_finish_handler( struct tagDATA_PIPE *p_data_pipe ) 
{
	struct tagBT_DATA_MANAGER *p_bt_data_manager = (struct tagBT_DATA_MANAGER *)p_data_pipe->_p_data_manager;

	sd_assert( p_data_pipe->_data_pipe_type == BT_PIPE );
	LOG_DEBUG( "pi_notify_dispatch_data_finish, pipe_ptr:0x%x ", p_data_pipe  );		

	return bdm_notify_dispatch_data_finish( p_bt_data_manager, p_data_pipe ); 
}

_int32 bt_pipe_notify_get_file_type_handler( struct tagDATA_PIPE *p_data_pipe ) 
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "pi_get_file_type, pipe_ptr:0x%x ", p_data_pipe  );		

	sd_assert( p_data_pipe->_data_pipe_type == BT_PIPE );
	return ret_val;
}

RANGE_LIST* bt_pipe_get_checked_range_list_handler( struct tagDATA_PIPE *p_data_pipe ) 
{
	LOG_DEBUG( "bt_pipe_get_checked_range_list_handler, pipe_ptr:0x%x ", p_data_pipe  );		

	sd_assert( p_data_pipe->_data_pipe_type == BT_PIPE );
	sd_assert( FALSE );
	return NULL;
}

#endif /* #ifdef ENABLE_BT  */


