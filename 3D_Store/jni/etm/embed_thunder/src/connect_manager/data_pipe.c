#include "utility/logid.h"
#define LOGID LOGID_CONNECT_MANAGER
#include "utility/logger.h"
#include "utility/time.h"
#include "data_pipe.h"
#include "resource.h"
#include "pipe_interface.h"

#include "data_manager/data_manager_interface.h"

#ifdef ENABLE_BT 
#include "bt_download/bt_task/bt_task_interface.h"
#include "bt_download/bt_data_manager/bt_data_manager_interface.h"
#include "bt_download/bt_data_manager/bt_data_manager.h"

#endif /* #ifdef ENABLE_BT */


void init_pipe_info(DATA_PIPE *p_data_pipe, 
					PIPE_TYPE type_info, 
					RESOURCE *p_res, 
					void *p_data_manager)
{  
	p_data_pipe->_data_pipe_type = type_info;
	p_data_pipe->_p_resource = p_res;
	p_data_pipe->_p_data_manager = p_data_manager;
	p_data_pipe->_p_pipe_interface = NULL;

	p_data_pipe->_dispatch_info._time_stamp = 0;
    
	p_data_pipe->_dispatch_info._speed = 0;	
	p_data_pipe->_dispatch_info._score = 0;	
	p_data_pipe->_dispatch_info._max_speed = 0;
	p_data_pipe->_dispatch_info._try_to_filter_time = 0;
	
	p_data_pipe->_dispatch_info._is_support_range = TRUE;
	p_data_pipe->_dispatch_info._is_support_long_connect = TRUE;	
    //p_data_pipe->_dispatch_info._pipe_state = PIPE_IDLE;
    dp_set_state(p_data_pipe, PIPE_IDLE);
	p_data_pipe->_dispatch_info._cancel_down_range = FALSE;
	p_data_pipe->_dispatch_info._is_global_filted = FALSE;
	p_data_pipe->_dispatch_info._global_wrap_ptr = NULL;
	p_data_pipe->_dispatch_info._is_err_get_buffer = FALSE;

	p_data_pipe->_dispatch_info._down_range._index = 0;	
	p_data_pipe->_dispatch_info._down_range._num = 0;	
	
    p_data_pipe->_dispatch_info._correct_error_range._index = 0;	
    p_data_pipe->_dispatch_info._correct_error_range._num = 0;		

	range_list_init( &p_data_pipe->_dispatch_info._can_download_ranges );
	range_list_init( &p_data_pipe->_dispatch_info._uncomplete_ranges );
	p_data_pipe->_dispatch_info._last_recv_data_time = MAX_U32;
	
	p_data_pipe->_is_user_free = FALSE;
	p_data_pipe->_dispatch_info._pipe_create_time = 0;
	sd_time_ms(&p_data_pipe->_dispatch_info._pipe_create_time);
}

void uninit_pipe_info(DATA_PIPE *p_data_pipe)
{
	range_list_clear(&p_data_pipe->_dispatch_info._can_download_ranges);
	range_list_clear(&p_data_pipe->_dispatch_info._uncomplete_ranges);
}

void pipe_set_err_get_buffer(DATA_PIPE *p_data_pipe, BOOL is_err)
{
	p_data_pipe->_dispatch_info._is_err_get_buffer = is_err;
}

BOOL pipe_is_err_get_buffer(DATA_PIPE *p_data_pipe)
{
	return p_data_pipe->_dispatch_info._is_err_get_buffer;
}

void dp_set_pipe_interface(DATA_PIPE *p_data_pipe, struct tagPIPE_INTERFACE *p_pipe_interface)
{
	p_data_pipe->_p_pipe_interface = p_pipe_interface;
}

_u32 dp_get_pipe_file_index(DATA_PIPE *p_data_pipe)
{
	return pi_get_file_index(p_data_pipe->_p_pipe_interface);
}

 PIPE_STATE dp_get_state(DATA_PIPE *p_data_pipe)
{
	return p_data_pipe->_dispatch_info._pipe_state;
}

 void dp_set_state(DATA_PIPE *p_data_pipe, PIPE_STATE state)
 {
     LOG_DEBUG("[dispatch_stat] pipe 0x%x set_status %d", p_data_pipe, state);
     p_data_pipe->_dispatch_info._pipe_state = state;
 }

 #ifdef UPLOAD_ENABLE
PEER_UPLOAD_STATE dp_get_upload_state(DATA_PIPE* p_data_pipe)
 {
 	return p_data_pipe->_dispatch_info._pipe_upload_state;
 }	
#endif

_int32 dp_get_download_range(DATA_PIPE *p_data_pipe, RANGE *p_download_range)
{
	p_download_range->_index = 0;
	p_download_range->_num = 0;
		
	if (p_data_pipe->_dispatch_info._down_range._num == 0)
	{
		return SUCCESS;
	}
	
	return pi_pipe_get_dispatcher_range(p_data_pipe, &p_data_pipe->_dispatch_info._down_range, (void *)p_download_range);
}

_int32 dp_set_download_range(DATA_PIPE *p_data_pipe, const RANGE *p_download_range)
{
	sd_assert(p_data_pipe->_data_pipe_type != BT_PIPE);

	LOG_DEBUG("dp_set_download_range.range:[%u,%u]", p_download_range->_index, p_download_range->_num);
	return pi_pipe_set_dispatcher_range(p_data_pipe, (void *)p_download_range, &p_data_pipe->_dispatch_info._down_range);
}

_int32 dp_clear_download_range(DATA_PIPE *p_data_pipe)
{
 	RANGE range;
	range._index = 0;
	range._num = 0;
	
	LOG_DEBUG( "dp_clear_download_range" );
	return dp_set_download_range(p_data_pipe, &range);
}

#ifdef ENABLE_BT
_int32 dp_get_bt_download_range(DATA_PIPE *p_data_pipe, RANGE *p_download_range)
{
	sd_assert( p_data_pipe->_data_pipe_type == BT_PIPE );
	p_download_range->_index = p_data_pipe->_dispatch_info._down_range._index;
	p_download_range->_num = p_data_pipe->_dispatch_info._down_range._num;
	return SUCCESS;
}

_int32 dp_set_bt_download_range(DATA_PIPE *p_data_pipe, const RANGE *p_download_range)
{
	sd_assert( p_data_pipe->_data_pipe_type == BT_PIPE );

	LOG_DEBUG( "dp_set_bt_download_range.range:[%u,%u]", p_download_range->_index, p_download_range->_num );
	
	p_data_pipe->_dispatch_info._down_range._index = p_download_range->_index;
	p_data_pipe->_dispatch_info._down_range._num = p_download_range->_num;
	return SUCCESS;
}

_int32 dp_clear_bt_download_range(DATA_PIPE *p_data_pipe)
{
	p_data_pipe->_dispatch_info._down_range._index = 0;
	p_data_pipe->_dispatch_info._down_range._num = 0;
	LOG_DEBUG( "dp_clear_download_range" );
	
	return SUCCESS;
}

_int32 dp_switch_range_2_bt_range(DATA_PIPE *p_data_pipe, const RANGE *p_range, BT_RANGE *p_bt_range)
{
	sd_assert( p_data_pipe->_data_pipe_type == BT_PIPE );
	
	LOG_DEBUG( "dp_switch_range_2_bt_range.range:[%u,%u]", p_range->_index, p_range->_num );
	
	return pi_pipe_get_dispatcher_range( p_data_pipe, p_range, p_bt_range );	
}
#endif

_int32 dp_add_can_download_ranges(DATA_PIPE *p_data_pipe, RANGE *p_can_download_range)
{
	_int32 ret_val = SUCCESS;
	RANGE dispatcher_range;
	dispatcher_range._index = 0;
	dispatcher_range._num = 0;
	sd_assert( p_data_pipe->_data_pipe_type != BT_PIPE );
	ret_val =  pi_pipe_set_dispatcher_range(p_data_pipe, (void *)p_can_download_range, &dispatcher_range);
	sd_assert(ret_val == SUCCESS);
	
	ret_val = range_list_add_range(&p_data_pipe->_dispatch_info._can_download_ranges, &dispatcher_range, NULL, NULL);
	CHECK_VALUE(ret_val);
	
	return SUCCESS;
}

#ifdef ENABLE_BT
_int32 dp_add_bt_can_download_ranges(DATA_PIPE *p_data_pipe, BT_RANGE *p_can_download_range)
{
	_int32 ret_val = SUCCESS;
	RANGE dispatcher_range;
	dispatcher_range._index = 0;
	dispatcher_range._num = 0;
	sd_assert(p_data_pipe->_data_pipe_type == BT_PIPE);
	ret_val =  pi_pipe_set_dispatcher_range(p_data_pipe, (void *)p_can_download_range, &dispatcher_range);
	sd_assert( ret_val == SUCCESS );

	if (bdm_range_is_in_need_range((struct tagBT_DATA_MANAGER *)(p_data_pipe->_p_data_manager), &dispatcher_range) == FALSE)
	{
		return SUCCESS;
	}
	
	ret_val = range_list_add_range(&p_data_pipe->_dispatch_info._can_download_ranges, &dispatcher_range, NULL, NULL);
	CHECK_VALUE( ret_val );
	
	return SUCCESS;

}
#endif

_u32 dp_get_can_download_ranges_list_size(DATA_PIPE *p_data_pipe)
{
	return range_list_size(&p_data_pipe->_dispatch_info._can_download_ranges);
}

_int32 dp_clear_can_download_ranges_list(DATA_PIPE *p_data_pipe)
{
	return range_list_clear(&p_data_pipe->_dispatch_info._can_download_ranges);
}

_int32 dp_add_uncomplete_ranges(DATA_PIPE *p_data_pipe, const RANGE *p_uncomplete_range)
{
	_int32 ret_val = SUCCESS;
	RANGE dispatcher_range;
	dispatcher_range._index = 0;
	dispatcher_range._num = 0;
	sd_assert(p_data_pipe->_data_pipe_type != BT_PIPE);
	ret_val =  pi_pipe_set_dispatcher_range(p_data_pipe, (void *)p_uncomplete_range, &dispatcher_range);
	sd_assert(ret_val == SUCCESS);
	ret_val = range_list_add_range( &p_data_pipe->_dispatch_info._uncomplete_ranges, &dispatcher_range, NULL, NULL );
	CHECK_VALUE( ret_val );
	
	return SUCCESS;
}

#ifdef ENABLE_BT
_int32 dp_add_bt_uncomplete_ranges(DATA_PIPE *p_data_pipe, const BT_RANGE *p_uncomplete_range)
{
	_int32 ret_val = SUCCESS;
	RANGE dispatcher_range;
	dispatcher_range._index = 0;
	dispatcher_range._num = 0;
	sd_assert(p_data_pipe->_data_pipe_type == BT_PIPE);
	ret_val =  pi_pipe_set_dispatcher_range(p_data_pipe, (void *)p_uncomplete_range, &dispatcher_range);
	sd_assert(ret_val == SUCCESS);
	ret_val = range_list_add_range(&p_data_pipe->_dispatch_info._uncomplete_ranges, &dispatcher_range, NULL, NULL);
	CHECK_VALUE( ret_val );
	
	return SUCCESS;
}
#endif

_u32 dp_get_uncomplete_ranges_list_size(DATA_PIPE *p_data_pipe)
{
	return range_list_size(&p_data_pipe->_dispatch_info._uncomplete_ranges);
}

_int32 dp_clear_uncomplete_ranges_list(DATA_PIPE *p_data_pipe)
{
	return range_list_clear(&p_data_pipe->_dispatch_info._uncomplete_ranges);
}

_int32 dp_get_uncomplete_ranges_head_range(DATA_PIPE *p_data_pipe, RANGE *p_head_range)
{
	_int32 ret_val = SUCCESS;
	RANGE_LIST_ITEROATOR  range_node = NULL;
	
	sd_assert(p_data_pipe->_data_pipe_type != BT_PIPE);
	p_head_range->_index = 0;
	p_head_range->_num= 0;	
    
	ret_val = range_list_get_head_node(&p_data_pipe->_dispatch_info._uncomplete_ranges, &range_node);
	CHECK_VALUE( ret_val );

	if (NULL == range_node)
	{
		return SUCCESS;
	}
	
	return pi_pipe_get_dispatcher_range(p_data_pipe, &range_node->_range, (void *)p_head_range);
}

#ifdef ENABLE_BT
_int32 dp_get_bt_uncomplete_ranges_head_range(DATA_PIPE *p_data_pipe, BT_RANGE *p_head_range)
{
	_int32 ret_val = SUCCESS;
	RANGE_LIST_ITEROATOR  range_node = NULL;
	sd_assert(p_data_pipe->_data_pipe_type == BT_PIPE);
	
	ret_val = range_list_get_head_node( &p_data_pipe->_dispatch_info._uncomplete_ranges, &range_node );
	CHECK_VALUE(ret_val);
	
	return pi_pipe_get_dispatcher_range(p_data_pipe, &range_node->_range, (void *)p_head_range);
}
#endif

_int32 dp_delete_uncomplete_ranges( DATA_PIPE *p_data_pipe, RANGE *p_range )
{
    _int32 ret_val = SUCCESS;
    RANGE dispatcher_range;
    sd_assert( p_data_pipe->_data_pipe_type != BT_PIPE );

    ret_val =  pi_pipe_set_dispatcher_range( p_data_pipe, 
        (void *)p_range, &dispatcher_range );
    sd_assert( ret_val == SUCCESS );

    LOG_DEBUG( "dp_delete_uncomplete_ranges.range:[%u,%u], dispatcher_range:[%u,%u]",
        p_range->_index, p_range->_num, dispatcher_range._index, dispatcher_range._num  );	


    ret_val = range_list_delete_range( &p_data_pipe->_dispatch_info._uncomplete_ranges, &dispatcher_range, NULL, NULL );
    CHECK_VALUE( ret_val );

    LOG_DEBUG( "dp_delete_uncomplete_ranges and _uncomplete_ranges:"  );		
    out_put_range_list( &p_data_pipe->_dispatch_info._uncomplete_ranges); 

    return SUCCESS;
}

#ifdef ENABLE_BT
_int32 dp_delete_bt_uncomplete_ranges(DATA_PIPE *p_data_pipe, BT_RANGE *p_range)
{
	_int32 ret_val = SUCCESS;
	RANGE dispatcher_range;
	dispatcher_range._index = 0;
	dispatcher_range._num = 0;
	sd_assert(p_data_pipe->_data_pipe_type == BT_PIPE);
	ret_val =  pi_pipe_set_dispatcher_range(p_data_pipe, (void *)p_range, &dispatcher_range);
	sd_assert(ret_val == SUCCESS);
	ret_val = range_list_delete_range( &p_data_pipe->_dispatch_info._uncomplete_ranges, &dispatcher_range, NULL, NULL );
	CHECK_VALUE( ret_val );
	
	return SUCCESS;
}
#endif

_int32 dp_adjust_uncomplete_2_can_download_ranges(DATA_PIPE *p_data_pipe)
{

	return range_list_adjust( &p_data_pipe->_dispatch_info._uncomplete_ranges,
		&p_data_pipe->_dispatch_info._can_download_ranges );
}

void *dp_get_task_ptr(DATA_PIPE *p_data_pipe)
{
	PIPE_INTERFACE *p_pipe_interface = p_data_pipe->_p_pipe_interface;
	struct _tag_data_manager *p_common_data_manager = NULL;
    
#ifdef ENABLE_BT
	struct tagBT_DATA_MANAGER *p_bt_data_manager = NULL;
#endif

	if (p_pipe_interface == NULL) 
	{
		return NULL;	
	}
	
	if ( p_pipe_interface->_pipe_interface_type == COMMON_PIPE_INTERFACE_TYPE )
	{
		p_common_data_manager = (struct _tag_data_manager *)p_data_pipe->_p_data_manager;
		if( p_common_data_manager == NULL )
		{
			return NULL;
		}
		else
		{
			return (void *)p_common_data_manager->_task_ptr;
		}
	}
#ifdef ENABLE_BT
	else
	{
		p_bt_data_manager = (struct tagBT_DATA_MANAGER *)p_data_pipe->_p_data_manager;
		if( p_bt_data_manager == NULL )
		{
			return NULL;
		}
		else
		{
			return (void *)p_bt_data_manager->_task_ptr;
		}
	}
	
#endif /* #ifdef ENABLE_BT */

	return NULL;
}

