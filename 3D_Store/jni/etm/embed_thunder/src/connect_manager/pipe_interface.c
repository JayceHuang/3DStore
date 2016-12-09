#include "utility/logid.h"
#define LOGID LOGID_CONNECT_MANAGER
#include "utility/logger.h"

#include "pipe_interface.h"

#include "data_pipe.h"
#include "connect_manager/pipe_function_table.h"
#include "data_manager/data_receive.h"
#include "data_manager/data_buffer.h"
#ifdef UPLOAD_ENABLE
#include "upload_manager/upload_manager.h"
#endif


void pi_init_pipe_interface(PIPE_INTERFACE *p_pipe_interface, 
    enum tagPIPE_INTERFACE_TYPE pipe_interface_type, 
	_u32 file_index, void *range_switch_handler, 
	void **pp_fuct_table)
{
	p_pipe_interface->_pipe_interface_type = pipe_interface_type;
	p_pipe_interface->_file_index = file_index;
	p_pipe_interface->_range_switch_handler = range_switch_handler;
	p_pipe_interface->_func_table_ptr = pp_fuct_table;
}
     
_u32 pi_get_file_index( PIPE_INTERFACE *p_pipe_interface )
{
	if( p_pipe_interface == NULL ) 
	{
		LOG_DEBUG( "pi_get_file_index:MAX_U32"  );		
		return MAX_U32;	
	}
	
	LOG_DEBUG( "pi_get_file_index:%u", p_pipe_interface->_file_index  );		
	return p_pipe_interface->_file_index;
}


_int32 pi_pipe_change_range( struct tagDATA_PIPE *p_data_pipe, RANGE *p_range, BOOL cancel_flag )
{
	PIPE_INTERFACE *p_pipe_interface = p_data_pipe->_p_pipe_interface;
	change_range_handler p_change_range_handler = NULL;

	if( p_pipe_interface == NULL ) 
	{
		return SUCCESS;	
	}
	p_change_range_handler = (change_range_handler)( (p_pipe_interface->_func_table_ptr)[ CHANGE_PIPE_RANGE_FUNC ] );
	sd_assert( p_change_range_handler != NULL );
	if( p_change_range_handler == NULL ) return SUCCESS;
	
	LOG_DEBUG( "pi_pipe_change_range:p_range:[%u,%u], cancel_flag:%u, speed:%u .", p_range->_index, p_range->_num, cancel_flag,  p_data_pipe->_dispatch_info._speed);		
	return p_change_range_handler( p_data_pipe, p_range, cancel_flag );
}

_int32 pi_pipe_get_dispatcher_range(struct tagDATA_PIPE *p_data_pipe, const RANGE *p_dispatcher_range, void *p_pipe_range)
{
	PIPE_INTERFACE *p_pipe_interface = p_data_pipe->_p_pipe_interface;
	get_dispatcher_range_handler p_get_dispatcher_range_handler = NULL;

	if (p_pipe_interface == NULL) 
	{
		return SUCCESS;	
	}
	p_get_dispatcher_range_handler = (get_dispatcher_range_handler)(p_pipe_interface->_func_table_ptr)[GET_DISPATCHER_RANGE_FUNC];

	sd_assert(p_get_dispatcher_range_handler != NULL);
	sd_assert(p_dispatcher_range != NULL);
	
	if( p_get_dispatcher_range_handler == NULL)
	{
	    return SUCCESS;
	}
	LOG_DEBUG("pi_pipe_get_dispatcher_range:p_dispatcher_range:[%u,%u]", p_dispatcher_range->_index, p_dispatcher_range->_num);
	
	return p_get_dispatcher_range_handler(p_data_pipe, p_dispatcher_range, p_pipe_range);
}

_int32 pi_pipe_set_dispatcher_range(struct tagDATA_PIPE *p_data_pipe, const void *p_pipe_range, RANGE *p_dispatcher_range)
{
	_int32 ret_val = SUCCESS;
	PIPE_INTERFACE *p_pipe_interface = p_data_pipe->_p_pipe_interface;
	set_dispatcher_range_handler p_set_dispatcher_range_handler = NULL;

	if (p_pipe_interface == NULL) 
	{
		return SUCCESS;	
	}	
	p_set_dispatcher_range_handler = (set_dispatcher_range_handler )(p_pipe_interface->_func_table_ptr)[SET_DISPATCHER_RANGE_FUNC];
	sd_assert( p_set_dispatcher_range_handler != NULL );
	if (p_set_dispatcher_range_handler == NULL)
	{
	    return SUCCESS;
	}

	ret_val = p_set_dispatcher_range_handler(p_data_pipe, p_pipe_range, p_dispatcher_range);
	LOG_DEBUG( "pi_pipe_set_dispatcher_range:p_dispatcher_range:[%u,%u]", p_dispatcher_range->_index, p_dispatcher_range->_num );		
	return ret_val;
}

_u64 pi_get_file_size(struct tagDATA_PIPE *p_data_pipe)
{
	PIPE_INTERFACE *p_pipe_interface = p_data_pipe->_p_pipe_interface;
	get_file_size_handler p_get_file_size_handler = NULL;
#ifdef UPLOAD_ENABLE
	_int32 ret_val= SUCCESS;
	_u64  file_size=0;
#endif

	if (p_pipe_interface == NULL) 
	{
#ifdef UPLOAD_ENABLE
		ret_val = ulm_get_file_size(p_data_pipe, &file_size);
		sd_assert(ret_val==SUCCESS);
		return file_size;
#else
		return 0;
#endif
	}
	
	p_get_file_size_handler = (get_file_size_handler )p_pipe_interface->_func_table_ptr[GET_FILE_SIZE_FUNC];
	sd_assert(p_get_file_size_handler != NULL);
	if (p_get_file_size_handler == NULL) 
	{
	    return 0;
	}
	return p_get_file_size_handler(p_data_pipe);
}

_int32 pi_set_file_size(struct tagDATA_PIPE *p_data_pipe, _u64 filesize)
{
	PIPE_INTERFACE *p_pipe_interface = p_data_pipe->_p_pipe_interface;
	set_file_size_handler p_set_file_size_handler = NULL;

	if (p_pipe_interface == NULL)
	{
		return SUCCESS;	
	}
	
	p_set_file_size_handler = (set_file_size_handler)p_pipe_interface->_func_table_ptr[SET_FILE_SIZE_FUNC];
	LOG_DEBUG("pi_set_file_size: pipe_ptr:0x%x, filesize:%u", p_data_pipe, filesize);		

	sd_assert(p_set_file_size_handler != NULL);
	if (p_set_file_size_handler == NULL)
	{
	    return SUCCESS;
	}
	
	return p_set_file_size_handler( p_data_pipe, filesize );
}

_int32 pi_get_data_buffer( struct tagDATA_PIPE *p_data_pipe,
	char **pp_data_buffer, _u32 *p_data_len )
{
	PIPE_INTERFACE *p_pipe_interface = p_data_pipe->_p_pipe_interface;
	get_data_buffer_handler p_get_data_buffer_handler = NULL;
	_int32 ret_val = SUCCESS;

	if( p_pipe_interface == NULL ) 
	{
		ret_val = dm_get_buffer_from_data_buffer( p_data_len, pp_data_buffer );
		return ret_val;	
	}

	p_get_data_buffer_handler = (get_data_buffer_handler )p_pipe_interface->_func_table_ptr[ GET_DATA_BUFFER_FUNC ];

	LOG_DEBUG( "pi_get_data_buffer:pipe_ptr:0x%x, data_len:%u ", p_data_pipe, *p_data_len );		

	sd_assert( p_get_data_buffer_handler != NULL );
	if( p_get_data_buffer_handler == NULL ) return SUCCESS;
	return p_get_data_buffer_handler( p_data_pipe, pp_data_buffer, p_data_len );	
}

_int32 pi_free_data_buffer( struct tagDATA_PIPE *p_data_pipe,
	char **pp_data_buffer, _u32 data_len)
{
	PIPE_INTERFACE *p_pipe_interface = p_data_pipe->_p_pipe_interface;
	free_data_buffer_handler p_free_data_buffer_handler = NULL;
	
	LOG_DEBUG( " enter pi_free_data_buffer(0x%X,%u)..." ,p_data_pipe,p_data_pipe->_id);
	if( p_pipe_interface == NULL ) 
	{
		dm_free_buffer_to_data_buffer( data_len, pp_data_buffer );
		return SUCCESS;	
	}
	p_free_data_buffer_handler = (free_data_buffer_handler )p_pipe_interface->_func_table_ptr[ FREE_DATA_BUFFER_FUNC ];
	LOG_DEBUG( "pi_free_data_buffer:pipe_ptr:0x%x, data_len:%u ", p_data_pipe, data_len );		

	sd_assert( p_free_data_buffer_handler != NULL );
	if( p_free_data_buffer_handler == NULL ) return SUCCESS;
	return p_free_data_buffer_handler( p_data_pipe, pp_data_buffer, data_len );	
}

_int32 pi_put_data( struct tagDATA_PIPE *p_data_pipe, 
	const RANGE *p_range, char **pp_data_buffer, _u32 data_len,_u32 buffer_len,
	struct tagRESOURCE *p_resource )
{
	PIPE_INTERFACE *p_pipe_interface = p_data_pipe->_p_pipe_interface;
	put_data_handler p_put_data_handler = NULL;
	
	if( p_pipe_interface == NULL ) 
	{
		return SUCCESS;	
	}
	
	p_put_data_handler = (put_data_handler )p_pipe_interface->_func_table_ptr[ PUT_DATA_FUNC ];
    LOG_DEBUG( "pi_put_data [dispatch_stat] pipe 0x%x recv %u_%u, range:[%u,%u], data_len:%u, buffer_len:%u ", 
        p_data_pipe, p_range->_index, p_range->_num, p_range->_index, p_range->_num, data_len, buffer_len  );		

	sd_assert( p_put_data_handler != NULL );
	if( p_put_data_handler == NULL ) return SUCCESS;
	return p_put_data_handler( p_data_pipe, p_range, 
		pp_data_buffer, data_len, buffer_len, p_resource);
}

_int32  pi_pipe_read_data_buffer( struct tagDATA_PIPE *p_data_pipe,  
	RANGE_DATA_BUFFER *p_data_buffer, notify_read_result p_call_back, void *p_user )
{
	PIPE_INTERFACE *p_pipe_interface = p_data_pipe->_p_pipe_interface;
	read_data_handler p_read_data_handler = NULL;

	if( p_pipe_interface == NULL ) 
	{
		return SUCCESS;	
	}

	p_read_data_handler = (read_data_handler )p_pipe_interface->_func_table_ptr[ READ_DATA_FUNC ];
	LOG_DEBUG( "pi_pipe_read_data_buffer:pipe_ptr:0x%x, range:[%u,%u], data_len:%u, buffer_len:%u ", 
		p_data_pipe, p_data_buffer->_range._index, p_data_buffer->_range._num, p_data_buffer->_data_length, p_data_buffer->_buffer_length  );		

	sd_assert( p_read_data_handler != NULL );
	if( p_read_data_handler == NULL ) return SUCCESS;
	return p_read_data_handler( p_data_pipe, p_data_buffer, 
		p_call_back, p_user );
}

_int32 pi_notify_dispatch_data_finish(struct tagDATA_PIPE *p_data_pipe)
{
	PIPE_INTERFACE *p_pipe_interface = p_data_pipe->_p_pipe_interface;
	notify_dispatch_data_finish_handler p_notify_dispatch_data_finish_handler = NULL;

	if( p_pipe_interface == NULL ) 
	{
		return SUCCESS;	
	}
	
	p_notify_dispatch_data_finish_handler = (notify_dispatch_data_finish_handler )p_pipe_interface->_func_table_ptr[ NOTIFY_DISPATCH_DATA_FINISHED_FUNC ];
	LOG_DEBUG( "pi_notify_dispatch_data_finish, pipe_ptr:0x%x ", p_data_pipe  );		

	sd_assert( p_notify_dispatch_data_finish_handler != NULL );
	if( p_notify_dispatch_data_finish_handler == NULL ) return SUCCESS;
	return p_notify_dispatch_data_finish_handler( p_data_pipe ); 
}

PIPE_FILE_TYPE pi_get_file_type(struct tagDATA_PIPE *p_data_pipe)
{
	PIPE_INTERFACE *p_pipe_interface = p_data_pipe->_p_pipe_interface;
	get_file_type_handler p_get_file_type_handler = NULL;

	if( p_pipe_interface == NULL ) 
	{
		return SUCCESS;	
	}
	p_get_file_type_handler = (get_file_type_handler )p_pipe_interface->_func_table_ptr[ GET_FILE_TYPE_FUNC ];

	LOG_DEBUG( "pi_get_file_type, pipe_ptr:0x%x ", p_data_pipe  );		

	sd_assert( p_get_file_type_handler != NULL );
	if( p_get_file_type_handler == NULL ) return SUCCESS;
	return ( PIPE_FILE_TYPE )p_get_file_type_handler( p_data_pipe ); 
}

RANGE_LIST* pi_get_checked_range_list(struct tagDATA_PIPE *p_data_pipe)
{
	PIPE_INTERFACE *p_pipe_interface = p_data_pipe->_p_pipe_interface;
	get_checked_range_list_handler p_get_checked_range_list_handler = NULL;

	if( p_pipe_interface == NULL ) 
	{
		return NULL;	
	}
	p_get_checked_range_list_handler = (get_checked_range_list_handler )p_pipe_interface->_func_table_ptr[ GET_CHECKED_RANGE_LIST_FUNC ];

	LOG_DEBUG( "pi_get_checked_range_list, pipe_ptr:0x%x ", p_data_pipe  );		

	sd_assert( p_get_checked_range_list_handler != NULL );
	if( p_get_checked_range_list_handler == NULL ) return SUCCESS;
	return p_get_checked_range_list_handler( p_data_pipe ); 	
}

PIPE_INTERFACE_TYPE pi_get_pipe_interface_type(struct tagDATA_PIPE *p_data_pipe)
{
	sd_assert(p_data_pipe->_p_pipe_interface != NULL);
	if (p_data_pipe->_p_pipe_interface == NULL) 
	{
	    return UNKNOWN_PIPE_INTERFACE_TYPE;
	}
    LOG_DEBUG("pi_get_pipe_interface_type, p_data_pipe->_p_pipe_interface->_pipe_interface_type = %d.",
        p_data_pipe->_p_pipe_interface->_pipe_interface_type);
	return p_data_pipe->_p_pipe_interface->_pipe_interface_type;
}

