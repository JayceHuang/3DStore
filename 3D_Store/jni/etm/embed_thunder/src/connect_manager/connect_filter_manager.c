#include "connect_filter_manager.h"

#include "utility/logid.h"
#define LOGID LOGID_CONNECT_MANAGER
#include "utility/logger.h"
#include "utility/utility.h"
#include "utility/time.h"
#include "utility/settings.h"
#include "connect_manager_imp.h"
#include "connect_manager_setting.h"
#include "global_connect_dispatch.h"
#include "global_connect_manager.h"

static void cm_max_speed_res(struct tagCONNECT_MANAGER *p_connect_manager
    , _u32* max_speed
    , RESOURCE** pres_max);

void cm_init_filter_manager( CONNECT_FILTER_MANAGER *p_connect_filter_manager )
{
	LOG_DEBUG( "cm_init_filter_manager." );
	p_connect_filter_manager->_cur_total_speed = 0;
	p_connect_filter_manager->_cur_speed_lower_limit = 0;	
	p_connect_filter_manager->_cur_filter_dispatch_ms_time = 0;
	p_connect_filter_manager->_idle_ticks = 0;
}

_int32 cm_filter_pipes( struct tagCONNECT_MANAGER *p_connect_manager )
{
	_int32 ret_val = SUCCESS;
	_u32 cur_total_speed = p_connect_manager->_cur_server_speed + p_connect_manager->_cur_peer_speed;

	LOG_DEBUG( "cm_filter_pipes!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!." );
	ret_val = cm_init_filter_para( &p_connect_manager->_connect_filter_manager, cur_total_speed );
	CHECK_VALUE( ret_val );
	
	//cm_order_using_pipes( p_connect_manager );
	//CHECK_VALUE( ret_val );	
	
	cm_do_filter_dispatch( p_connect_manager );
	CHECK_VALUE( ret_val );		
	
	return SUCCESS;
}

_int32 cm_order_using_pipes( struct tagCONNECT_MANAGER *p_connect_manager )
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "cm_order_using_pipes." );

	ret_val = cm_order_using_server_pipes( p_connect_manager );
	CHECK_VALUE( ret_val );

	ret_val = cm_order_using_peer_pipes( p_connect_manager );
	CHECK_VALUE( ret_val );	
	
	return SUCCESS;	
}

_int32 cm_order_using_server_pipes( struct tagCONNECT_MANAGER *p_connect_manager )
{
	_int32 ret_val = SUCCESS;
	PIPE_LIST speed_order_server_pipe_list;	

	#ifdef _LOGGER
	LIST_ITERATOR cur_node, end_node;
	DATA_PIPE *p_order_pipe = NULL;
	#endif
	
	LOG_DEBUG( "cm_order_using_server_pipes. connect_manager ptr:0x%x.", p_connect_manager );

	list_init( &speed_order_server_pipe_list._data_pipe_list );
	
	ret_val = cm_get_order_list( &(p_connect_manager->_working_server_pipe_list._data_pipe_list), &speed_order_server_pipe_list._data_pipe_list, 
		cm_get_normal_dispatch_pipe_score,
		cm_is_normal_iter_valid,
		NULL );
	CHECK_VALUE( ret_val );
	
	list_swap( &(p_connect_manager->_working_server_pipe_list._data_pipe_list), &(speed_order_server_pipe_list._data_pipe_list) );

	//print the order result: must to descend order 
	
	#ifdef _LOGGER
	cur_node = LIST_BEGIN( p_connect_manager->_working_server_pipe_list._data_pipe_list );
	end_node = LIST_END( p_connect_manager->_working_server_pipe_list._data_pipe_list );	
	while( cur_node != end_node )
	{
		p_order_pipe = LIST_VALUE( cur_node );
		LOG_DEBUG( "server: cm_get_speed_order_list.order list pipe speed:%u, score:%u, max_speed:%u, pipe_ptr:0x%x, resource_ptr:0x%x.", 
			p_order_pipe->_dispatch_info._speed, p_order_pipe->_dispatch_info._score, p_order_pipe->_dispatch_info._max_speed, p_order_pipe, p_order_pipe->_p_resource );
		cur_node = LIST_NEXT( cur_node );
	}
	#endif

	return SUCCESS;
}

_int32 cm_order_using_peer_pipes( struct tagCONNECT_MANAGER *p_connect_manager )
{
	_int32 ret_val = SUCCESS;
	PIPE_LIST speed_order_peer_pipe_list;	
	
	#ifdef _LOGGER
	LIST_ITERATOR cur_node, end_node;
	DATA_PIPE *p_order_pipe= NULL;
	#endif
	
	LOG_DEBUG( "cm_order_using_peer_pipes. connect_manager ptr:0x%x.", p_connect_manager );

	list_init( &speed_order_peer_pipe_list._data_pipe_list );


	ret_val = cm_get_order_list( &(p_connect_manager->_working_peer_pipe_list._data_pipe_list), 
		&speed_order_peer_pipe_list._data_pipe_list,
		cm_get_normal_dispatch_pipe_score,
		cm_is_normal_iter_valid, 
		NULL );
	CHECK_VALUE( ret_val );	

	list_swap( &(p_connect_manager->_working_peer_pipe_list._data_pipe_list), &(speed_order_peer_pipe_list._data_pipe_list) );

	#ifdef _LOGGER
	cur_node = LIST_BEGIN( p_connect_manager->_working_peer_pipe_list._data_pipe_list );
	end_node = LIST_END( p_connect_manager->_working_peer_pipe_list._data_pipe_list );	
	while( cur_node != end_node )
	{
		p_order_pipe = LIST_VALUE( cur_node );
		LOG_DEBUG( "peer: cm_get_speed_order_list.order list pipe speed:%u, score:%u, max_speed:%u, pipe_ptr:0x%x, resource_ptr:0x%x.", 
			p_order_pipe->_dispatch_info._speed, p_order_pipe->_dispatch_info._score, p_order_pipe->_dispatch_info._max_speed, p_order_pipe, p_order_pipe->_p_resource );
		cur_node = LIST_NEXT( cur_node );
	}
	#endif

	return SUCCESS;
}

_int32 gcm_order_global_using_pipes(void)
{
	_int32 ret_val = SUCCESS;
	LIST speed_order_list;	
	
	#ifdef _LOGGER
	LIST_ITERATOR cur_node, end_node;
	PIPE_WRAP *p_pipe_wrap = NULL;
	DATA_PIPE *p_order_pipe = NULL;
	#endif
	
	LOG_DEBUG( "gcm_order_global_using_pipes." );

	list_init( &speed_order_list );

	ret_val = cm_get_order_list( &gcm_get_ptr()->_using_pipe_list, 
		&speed_order_list, 
		cm_get_global_dispatch_pipe_score,
		cm_is_pipe_wrap_iter_valid,
		cm_free_pipe_wrap_iterator );
	CHECK_VALUE( ret_val );	
	
	list_swap( &gcm_get_ptr()->_using_pipe_list, &speed_order_list );

	#ifdef _LOGGER
	cur_node = LIST_BEGIN( gcm_get_ptr()->_using_pipe_list );
	end_node = LIST_END( gcm_get_ptr()->_using_pipe_list );	
	while( cur_node != end_node )
	{
		p_pipe_wrap = LIST_VALUE( cur_node );
		sd_assert( !p_pipe_wrap->_is_deleted_by_pipe );
		p_order_pipe = p_pipe_wrap->_data_pipe_ptr;
		sd_assert( p_pipe_wrap == p_order_pipe->_dispatch_info._global_wrap_ptr );
		
		LOG_DEBUG( "gcm_order_global_using_pipes.order list pipe speed:%u, score:%u, max_speed:%u, pipe_ptr:0x%x, pipe_type:%d, resource_ptr:0x%x.pipe_state:%u, wrap_ptr:0x%x", 
			p_order_pipe->_dispatch_info._speed, p_order_pipe->_dispatch_info._score, 
			p_order_pipe->_dispatch_info._max_speed, p_order_pipe,
			p_order_pipe->_data_pipe_type, p_order_pipe->_p_resource,
			p_order_pipe->_dispatch_info._pipe_state, p_pipe_wrap );
		cur_node = LIST_NEXT( cur_node );
	}
	#endif

	return SUCCESS;
}

_int32 gcm_order_global_candidate_res(void)
{
	_int32 ret_val = SUCCESS;
	LIST speed_order_list;	
	
	#ifdef _LOGGER
	LIST_ITERATOR cur_node, end_node;
	RESOURCE *p_res = NULL;
	RES_WRAP *p_res_wrap = NULL;

	#endif
	
	LOG_DEBUG( "gcm_order_global_candidate_res." );

	list_init( &speed_order_list );

	ret_val = cm_get_order_list( &gcm_get_ptr()->_candidate_res_list,
		&speed_order_list,
		cm_get_global_dispatch_res_max_speed,
		cm_is_res_wrap_iter_valid,
		cm_free_res_wrap_iterator );
	CHECK_VALUE( ret_val );	
	
	list_swap( &gcm_get_ptr()->_candidate_res_list, &speed_order_list );

	#ifdef _LOGGER
	cur_node = LIST_BEGIN( gcm_get_ptr()->_candidate_res_list );
	end_node = LIST_END( gcm_get_ptr()->_candidate_res_list );	
	while( cur_node != end_node )
	{
		p_res_wrap = LIST_VALUE( cur_node );
		sd_assert( !p_res_wrap->_is_deleted );

		p_res = p_res_wrap->_res_ptr;
		LOG_DEBUG( "gcm_order_global_candidate_res.order list max_speed:%u, res_ptr:0x%x.", 
			p_res->_max_speed, p_res );
		cur_node = LIST_NEXT( cur_node );
	}
	#endif
	
	return SUCCESS;
}

BOOL cm_is_normal_iter_valid( LIST_ITERATOR list_iter )
{	
	LOG_DEBUG( "cm_is_normal_iter_valid." );
	return TRUE;
}

BOOL cm_is_res_wrap_iter_valid( LIST_ITERATOR list_iter )
{
	RES_WRAP *p_res_wrap = LIST_VALUE( list_iter );
	LOG_DEBUG( "cm_is_res_wrap_iter_valid.res_wrap:0x%x, _is_deleted_by_pipe:%d",
		p_res_wrap, p_res_wrap->_is_deleted );
	
	return !p_res_wrap->_is_deleted;
}

_u32 cm_get_normal_dispatch_pipe_score( LIST_ITERATOR list_iter )
{
	DATA_PIPE *p_pipe = LIST_VALUE( list_iter );
	LOG_DEBUG( "cm_get_normal_dispatch_pipe_score.pipe_ptr:0x%x, speed:%d",
		p_pipe, p_pipe->_dispatch_info._speed );

	return p_pipe->_dispatch_info._score;
}

BOOL cm_is_pipe_wrap_iter_valid( LIST_ITERATOR list_iter )
{
	PIPE_WRAP *p_pipe_wrap = LIST_VALUE( list_iter );
	LOG_DEBUG( "cm_is_pipe_wrap_iter_valid.pipe_wrap:0x%x, pipe_ptr:0x%x, _is_deleted_by_pipe:%d",
		p_pipe_wrap, p_pipe_wrap->_data_pipe_ptr, p_pipe_wrap->_is_deleted_by_pipe );
	
	return !p_pipe_wrap->_is_deleted_by_pipe;
}

_int32 cm_free_pipe_wrap_iterator( LIST_ITERATOR list_iter )
{
	PIPE_WRAP *p_pipe_wrap = LIST_VALUE( list_iter );
	LOG_DEBUG( "cm_free_pipe_wrap_iterator.pipe_wrap:0x%x, pipe_ptr:0x%x", p_pipe_wrap, p_pipe_wrap->_data_pipe_ptr );
	
	gcm_free_pipe_wrap( p_pipe_wrap );
	return SUCCESS;
}

_int32 cm_free_res_wrap_iterator( LIST_ITERATOR list_iter )
{
	RES_WRAP *p_res_wrap = LIST_VALUE( list_iter );
	LOG_DEBUG( "cm_free_res_wrap_iterator.res_wrap:0x%x ", p_res_wrap );
	
	gcm_free_res_wrap( p_res_wrap );
	return SUCCESS;
}

_u32 cm_get_global_dispatch_pipe_score( LIST_ITERATOR list_iter )
{
	PIPE_WRAP *p_pipe_wrap = LIST_VALUE( list_iter );
	DATA_PIPE *p_pipe = p_pipe_wrap->_data_pipe_ptr;
	
	sd_assert( !p_pipe_wrap->_is_deleted_by_pipe );
	//LOG_DEBUG( "cm_get_global_dispatch_pipe_speed.pipe:0x%x, speed:%u ", p_pipe, p_pipe->_dispatch_info._speed );

	//return p_pipe->_dispatch_info._speed;	
	return p_pipe->_dispatch_info._score;	
}

_u32 cm_get_global_dispatch_res_max_speed( LIST_ITERATOR list_iter )
{
	RES_WRAP *p_res_wrap = LIST_VALUE( list_iter );
	RESOURCE *p_res = p_res_wrap->_res_ptr;
	
	//LOG_DEBUG( "cm_get_global_dispatch_res_max_speed.res:0x%x, speed:%u ", p_res, p_res->_max_speed );
	return p_res->_max_speed;
}

_int32 cm_get_order_list( LIST *p_src_list, 
	LIST *p_order_list, 
	cm_get_key_value p_get_value, 
	cm_is_iter_valid p_is_valid,
	cm_free_iterator p_free )
{
	_int32 ret_val = SUCCESS;

	LIST_ITERATOR src_cur_node = LIST_BEGIN( *p_src_list );
	LIST_ITERATOR src_end_node = LIST_END( *p_src_list );
	LIST_ITERATOR src_next_node;
	
	LIST_ITERATOR order_next_node, order_cur_node, order_begin_node;		

	LOG_DEBUG( "cm_get_order_list." );

	while( src_cur_node != src_end_node && !p_is_valid( src_cur_node ) )
	{
		sd_assert( p_free != NULL );
		ret_val = p_free( src_cur_node );
		CHECK_VALUE( ret_val );

		src_next_node = LIST_NEXT( src_cur_node );

		list_erase( p_src_list, src_cur_node );
		src_cur_node = src_next_node;
	}
	
	if( list_size( p_src_list ) != 0 )//handle first src pipe
	{
		ret_val = list_push( p_order_list, LIST_VALUE( src_cur_node ) );
		CHECK_VALUE( ret_val );	
		LOG_DEBUG( "cm_get_order_list. src speed:%u.", p_get_value(src_cur_node) );
		
		src_next_node = LIST_NEXT( src_cur_node );

		ret_val = list_erase( p_src_list, src_cur_node );
		CHECK_VALUE( ret_val );

		src_cur_node = src_next_node;
	}
	else 
	{
		return SUCCESS;
	}

	while( src_cur_node != src_end_node )
	{
		if( !p_is_valid( src_cur_node ) )
		{
			sd_assert( p_free != NULL );
			ret_val = p_free( src_cur_node );
			CHECK_VALUE( ret_val );
			
			src_next_node = LIST_NEXT( src_cur_node );

			list_erase( p_src_list, src_cur_node );
			src_cur_node = src_next_node;
			continue;
		}
		
		order_cur_node = LIST_RBEGIN( *p_order_list );
		order_begin_node = LIST_BEGIN( *p_order_list );

		while( order_cur_node != order_begin_node )//insert pipe to the dest
		{
			order_next_node = LIST_PRE( order_cur_node );
			if( p_get_value(src_cur_node) < p_get_value(order_cur_node) )
			{
				ret_val = list_insert( p_order_list, LIST_VALUE( src_cur_node ), LIST_NEXT(order_cur_node) );
				CHECK_VALUE( ret_val );
				break;	
			}
			
			if( p_get_value(src_cur_node) > p_get_value(order_cur_node) 
				&& p_get_value(src_cur_node) <= p_get_value(order_next_node) )
			{
				ret_val = list_insert( p_order_list, LIST_VALUE( src_cur_node ), order_cur_node );
				CHECK_VALUE( ret_val );
				break;
			}
			order_cur_node = LIST_PRE( order_cur_node );
		}

		if( order_cur_node == order_begin_node )//insert pipe to the dest end
		{
			if( p_get_value(src_cur_node) > p_get_value(order_begin_node) )
			{
				ret_val = list_insert( p_order_list, LIST_VALUE( src_cur_node ), order_begin_node );
				CHECK_VALUE( ret_val );
			}
			else
			{
				ret_val = list_insert( p_order_list, LIST_VALUE( src_cur_node ), LIST_NEXT( order_begin_node ) );
			    CHECK_VALUE( ret_val );			
			}
		}
		
		LOG_DEBUG( "cm_get_order_list. src speed:%u.", p_get_value(src_cur_node) );
		
		src_next_node = LIST_NEXT( src_cur_node );
		
		ret_val = list_erase( p_src_list, src_cur_node );
		CHECK_VALUE( ret_val );

		src_cur_node = src_next_node;
	}

	return SUCCESS;
}

_int32 cm_filter_speed_list( struct tagCONNECT_MANAGER *p_connect_manager
    , _u32 max_speed
    , RESOURCE* pres
	, PIPE_LIST *p_pipe_list
	, RESOURCE_LIST *p_using_res_list
	, RESOURCE_LIST *p_candidate_res_list
	, BOOL is_server )
{
	_int32 ret_val = SUCCESS;

	LIST_ITERATOR begin_node, pre_node, cur_node;
	DATA_PIPE *p_data_pipe;
	CONNECT_FILTER_MANAGER *p_connect_filter_manager= NULL;
	_u32 pipe_num = 0;
	_u32 idle_res_size = 0;
	_u32 filter_pipe_num = 0;

	RESOURCE *p_resource = NULL;
	pipe_num = list_size( &p_pipe_list->_data_pipe_list );
	LOG_DEBUG( "filter pipe begin!!!!!: pipe list size:%d.", pipe_num );
	
	if( list_size( &p_pipe_list->_data_pipe_list ) == 0 )  
	{
		return SUCCESS;
	}
	
	begin_node = LIST_BEGIN( p_pipe_list->_data_pipe_list );
	cur_node = LIST_RBEGIN( p_pipe_list->_data_pipe_list );
	p_connect_filter_manager = &p_connect_manager->_connect_filter_manager;
	
	while( cur_node != begin_node )
	{
		idle_res_size = 0;
		p_data_pipe = LIST_VALUE( cur_node );
        LOG_DEBUG( "filter pipe ptr:0x%x, pipe_state:%d, cm_pipes_num_low_limit:%u ", 
        p_data_pipe,
        p_data_pipe->_dispatch_info._pipe_state,
        cm_pipes_num_low_limit() );
        if( p_connect_manager->_cur_pipe_num <= cm_pipes_num_low_limit() )
        {
            return SUCCESS;
        }

		if( p_data_pipe->_dispatch_info._pipe_state != PIPE_DOWNLOADING 
			|| cm_is_new_pipe( p_connect_filter_manager, p_data_pipe ) )
		{
            LOG_DEBUG("cm_filter_speed_list p_data_pipe->_dispatch_info._pipe_state = %d");
			cur_node = LIST_PRE( cur_node );
			continue;
		}
		if( is_server )
		{
			idle_res_size = list_size( &p_connect_manager->_idle_server_res_list._res_list );
		}
		else
		{
			idle_res_size = list_size( &p_connect_manager->_idle_peer_res_list._res_list );
		}
		//if( cm_is_lower_speed_pipe( p_connect_filter_manager, idle_res_size, p_data_pipe ) )
		if( cm_is_lower_speed_pipe( p_connect_filter_manager, p_data_pipe, max_speed, pres) )
		{
			p_resource = NULL;
			p_resource = p_data_pipe->_p_resource;
			if( is_server )
			{
				p_connect_manager->_cur_server_speed -= p_data_pipe->_dispatch_info._speed;	
			}
			else
			{
				p_connect_manager->_cur_peer_speed -= p_data_pipe->_dispatch_info._speed;					
			}
			
			ret_val = cm_destroy_single_pipe( p_connect_manager, p_data_pipe );
			CHECK_VALUE( ret_val );
			
			if( p_resource->_pipe_num == 0 )
			{
				LOG_DEBUG( "cm_move_res from using list to candidate list: resource:0x%x\n", p_resource );
							
				ret_val = cm_move_res( p_using_res_list, p_candidate_res_list, p_resource );
				CHECK_VALUE( ret_val );	
			}
			
			pre_node = LIST_PRE( cur_node );

			ret_val = list_erase( &p_pipe_list->_data_pipe_list, cur_node );
			CHECK_VALUE( ret_val );
			
			cur_node = pre_node;	
		}
		else
		{
			LOG_DEBUG( "cm_filter_speed_list end: pipe list size:%d .", list_size( &p_pipe_list->_data_pipe_list ) );
			//return SUCCESS;
			cur_node = LIST_PRE(cur_node);
			continue;
		}
	}
	LOG_DEBUG( "filter pipe end!!!!!: pipe list size:%d.", list_size( &p_pipe_list->_data_pipe_list ) );
	filter_pipe_num = pipe_num - list_size( &p_pipe_list->_data_pipe_list );
	pipe_num = list_size( &p_pipe_list->_data_pipe_list );
	LOG_DEBUG( "filter pipe end!!!!!: pipe list size:%d. filter_pipe_num:%d", pipe_num, filter_pipe_num );
	return SUCCESS;
}

void cm_max_speed_res(struct tagCONNECT_MANAGER *p_connect_manager
    , _u32* max_speed
    , RESOURCE** pres_max)
{
    _u32 res_max_speed = 0;
    //RESOURCE* pres = NULL;
    LIST_ITERATOR cur = LIST_BEGIN(p_connect_manager->_using_server_res_list._res_list);
    LIST_ITERATOR end = LIST_END(p_connect_manager->_using_server_res_list._res_list);
    if (list_size(&(p_connect_manager->_using_server_res_list._res_list)) != 0)
    {
        while(cur != end)
        {
            RESOURCE* cur_value = LIST_VALUE(cur);
            if(cur_value->_max_speed > res_max_speed)
            {
                res_max_speed = cur_value->_max_speed;
                *pres_max = cur_value;
            }
            cur = LIST_NEXT(cur);
        }
    }

    
    cur = LIST_BEGIN(p_connect_manager->_using_peer_res_list._res_list);
    end = LIST_END(p_connect_manager->_using_peer_res_list._res_list);
    if (list_size(&(p_connect_manager->_using_peer_res_list._res_list)) != 0)
    {
        while(cur != end)
        {
            RESOURCE* cur_value = LIST_VALUE(cur);
            if(cur_value->_max_speed > res_max_speed)
            {
                res_max_speed = cur_value->_max_speed;
                *pres_max = cur_value;
            }
            cur = LIST_NEXT(cur);
        }
    }
    *max_speed = res_max_speed;
    LOG_DEBUG("res_max_speed = %d, res = 0x%x", *max_speed, *pres_max);
}

_int32 cm_do_filter_dispatch( struct tagCONNECT_MANAGER *p_connect_manager )
{
	_int32 ret_val = SUCCESS;	
	
	LOG_DEBUG( "cm_do_filter_dispatch begin. connect_manager ptr:0x%x. cur_server_speed:%d, cur_peer_speed:%d, server_working_pipe_list_size:%d, peer_working_pipe_list_size:%d.",
		p_connect_manager,
		p_connect_manager->_cur_server_speed, 
		p_connect_manager->_cur_peer_speed,		
		list_size(&p_connect_manager->_working_server_pipe_list._data_pipe_list),
		list_size(&p_connect_manager->_working_peer_pipe_list._data_pipe_list) );

    _u32 max_speed = 0;
    RESOURCE* pres_max = NULL;
    cm_max_speed_res(p_connect_manager, &max_speed, &pres_max);


    // 总pipe数少于5个,pipe太少没必要进行废弃
    if(list_size(&p_connect_manager->_working_peer_pipe_list._data_pipe_list) + list_size(&p_connect_manager->_working_server_pipe_list._data_pipe_list) <= cm_pipes_num_low_limit() )
	{
	    LOG_DEBUG("working pipe less then upper limit");
	    return SUCCESS;
	}
	
	ret_val = cm_filter_speed_list( p_connect_manager,
	    max_speed,
	    pres_max,
		&p_connect_manager->_working_server_pipe_list,
		&p_connect_manager->_using_server_res_list,
		&p_connect_manager->_candidate_server_res_list,
		TRUE);
	CHECK_VALUE( ret_val );	

	ret_val = cm_filter_speed_list( p_connect_manager,
        max_speed,
	    pres_max,
		&p_connect_manager->_working_peer_pipe_list,
		&p_connect_manager->_using_peer_res_list,
		&p_connect_manager->_candidate_peer_res_list,
		FALSE);
	CHECK_VALUE( ret_val );	


	LOG_DEBUG( "cm_do_server_filter_list end. connect_manager ptr:0x%x. cur_server_speed:%d, cur_peer_speed:%d, server_working_pipe_list_size:%d, peer_working_pipe_list_size:%d.",
		p_connect_manager,
		p_connect_manager->_cur_server_speed, 
		p_connect_manager->_cur_peer_speed,		
		list_size(&p_connect_manager->_working_server_pipe_list._data_pipe_list),
		list_size(&p_connect_manager->_working_peer_pipe_list._data_pipe_list) );
	return SUCCESS;
}

_int32 cm_init_filter_para( CONNECT_FILTER_MANAGER *p_connect_filter_manager, _u32 cur_total_speed  )
{
	_int32 ret_val = SUCCESS;
	_u32 cur_ms_time = 0;
	LOG_DEBUG( "cm_init_filter_para. cur_total_speed:%d", cur_total_speed );

	p_connect_filter_manager->_cur_total_speed = cur_total_speed;
	p_connect_filter_manager->_cur_speed_lower_limit = cur_total_speed * cm_task_speed_filter_ratio() / 100;

	ret_val = sd_time_ms( &cur_ms_time );
	CHECK_VALUE( ret_val );	
	p_connect_filter_manager->_cur_filter_dispatch_ms_time = cur_ms_time;
	
	LOG_DEBUG( "cm_init_filter_para end: task speed limit:%d. ",
 		p_connect_filter_manager->_cur_speed_lower_limit );

	return SUCCESS;	
}

BOOL cm_is_lower_speed_pipe( CONNECT_FILTER_MANAGER *p_connect_filter_manager
        , DATA_PIPE *p_data_pipe 
        , _u32 max_speed
        , RESOURCE* pres)
{
	_u32 cur_total_speed, pipe_speed,pipe_max_speed, now_time;
	BOOL is_slow = FALSE;
	cur_total_speed = p_connect_filter_manager->_cur_total_speed;
	pipe_speed = p_data_pipe->_dispatch_info._speed;
	pipe_max_speed = p_data_pipe->_dispatch_info._max_speed;
	sd_assert( cur_total_speed >= pipe_speed );
	sd_time_ms(&now_time);
	// cm_filt_speed 默认配置是10K/s
	//if( pipe_speed < cm_filt_speed() &&  pipe_max_speed < cm_filt_speed())
	//{
	//if( (cur_total_speed - pipe_speed) > p_connect_filter_manager->_cur_speed_lower_limit 
	//    && pipe_max_speed < 100*1024)
	if( pipe_max_speed*10 < max_speed && pipe_max_speed < 20*1024 && p_data_pipe->_p_resource != pres)
	{
        if( p_data_pipe->_dispatch_info._try_to_filter_time < cm_filt_max_speed_time() )
        {
        	p_data_pipe->_dispatch_info._try_to_filter_time++;
		    is_slow = FALSE;
        }
        else
        {
            is_slow = TRUE;
        }
	}	
	else
	{
		is_slow = FALSE;
	}
	LOG_DEBUG( "cm_is_lower_speed_pipe, pipe ptr:0x%x, pipe_speed:%u, pipe_max_speed:%d,try_to_filter_time:%d, cur_total_speed:%u, is_slow:%d", 
			p_data_pipe, pipe_speed, p_data_pipe->_dispatch_info._max_speed,p_data_pipe->_dispatch_info._try_to_filter_time , cur_total_speed, is_slow );
	return is_slow;
}

BOOL cm_is_new_pipe( CONNECT_FILTER_MANAGER *p_connect_filter_manager, DATA_PIPE *p_data_pipe )
{	
	_u32 exist_time;
	BOOL ret_bool;
	//exist_time = p_connect_filter_manager->_cur_filter_dispatch_ms_time - p_data_pipe->_dispatch_info._time_stamp;
	exist_time = TIME_SUBZ( p_connect_filter_manager->_cur_filter_dispatch_ms_time, p_data_pipe->_dispatch_info._pipe_create_time );

	if( exist_time < cm_pipe_speed_test_time() )
	{
		ret_bool = TRUE;
	}
	else
	{
		ret_bool = FALSE;
	}	
	LOG_DEBUG( "cm_is_new_pipe, _cur_filter_dispatch_ms_time:%u, time_stamp:%u",
		p_connect_filter_manager->_cur_filter_dispatch_ms_time, p_data_pipe->_dispatch_info._time_stamp );
	
	LOG_DEBUG( "cm_is_new_pipe, pipe ptr:0x%x, pipe time:%u, testing_millisecond:%u, return:%d",
		p_data_pipe, exist_time, cm_pipe_speed_test_time(), ret_bool);
	
	return ret_bool;
}

_int32 gcm_do_global_filter_dispatch(void)
{
    _u32 filter_pipe_num = 0;
	_u32 filter_pipe_max_num_limit = 0;
    _u32 idle_res_num = 0;
	PIPE_WRAP *p_pipe_wrap = NULL;
	DATA_PIPE *p_data_pipe = NULL;
	BOOL is_slow = FALSE;
	_u32 pipe_low_limit = 0;
	
    LIST_ITERATOR cur_node = LIST_BEGIN( gcm_get_ptr()->_connect_manager_ptr_list );
    LIST_ITERATOR begin_node = NULL;

    while( cur_node != LIST_END(gcm_get_ptr()->_connect_manager_ptr_list) )
    {
        CONNECT_MANAGER *p_connect_manager = LIST_VALUE( cur_node );
        sd_assert( p_connect_manager->_cm_level != UNKNOWN_CM_LEVEL );
        if( p_connect_manager->_cm_level != INIT_CM_LEVEL )
        {
            idle_res_num += list_size(&(p_connect_manager->_idle_peer_res_list._res_list));
			idle_res_num += list_size(&(p_connect_manager->_idle_server_res_list._res_list));
        }
        cur_node = LIST_NEXT( cur_node );
    }

	sd_assert( gcm_get_ptr()->_global_max_pipe_num >= gcm_get_ptr()->_global_max_filter_pipe_num );
	pipe_low_limit = gcm_get_ptr()->_global_max_pipe_num - gcm_get_ptr()->_global_max_filter_pipe_num;
	if( gcm_get_ptr()->_global_cur_pipe_num > pipe_low_limit )
	{
		filter_pipe_max_num_limit = gcm_get_ptr()->_global_cur_pipe_num - pipe_low_limit;
	}
	
	filter_pipe_max_num_limit += idle_res_num / 2;

	cur_node = LIST_RBEGIN( gcm_get_ptr()->_using_pipe_list );
	begin_node = LIST_BEGIN( gcm_get_ptr()->_using_pipe_list );
	LOG_DEBUG( "gcm_do_global_filter_dispatch, list_size:%d, idle_res_num:%d, cur_pipe_num:%d, filter_pipe_max_num_limit:%d, pipe_low_limit:%d!!!!!!!!!! ",
        list_size( &gcm_get_ptr()->_using_pipe_list ), 
        idle_res_num, 
        gcm_get_ptr()->_global_cur_pipe_num, 
        filter_pipe_max_num_limit, pipe_low_limit );

    while( cur_node != begin_node )
	{
		p_pipe_wrap = LIST_VALUE( cur_node );
		sd_assert( !p_pipe_wrap->_is_deleted_by_pipe );
		p_data_pipe = p_pipe_wrap->_data_pipe_ptr;
        
        p_data_pipe->_dispatch_info._is_global_filted = FALSE;
        
        LOG_DEBUG( "gcm_do_global_filter_dispatch start, pipe_ptr:0x%x, is_filted:%d, state:%d, score:%d",
            p_data_pipe, p_data_pipe->_dispatch_info._is_global_filted, p_data_pipe->_dispatch_info._pipe_state, p_data_pipe->_dispatch_info._score );
        
 		if( ( p_data_pipe->_dispatch_info._pipe_state != PIPE_DOWNLOADING 
			&& p_data_pipe->_dispatch_info._pipe_state != PIPE_CHOKED )
          || cm_is_new_pipe( &gcm_get_ptr()->_filter_manager, p_data_pipe ) )
		{
			cur_node = LIST_PRE( cur_node );
			continue;
		}
		
		is_slow = FALSE;//cm_is_lower_speed_pipe( &gcm_get_ptr()->_filter_manager, p_data_pipe );

		if( ( is_slow && p_data_pipe->_dispatch_info._score == 0 )
			|| ( is_slow && p_data_pipe->_dispatch_info._score > 0 && filter_pipe_max_num_limit > 0 ) )
		{
			p_data_pipe->_dispatch_info._is_global_filted = TRUE;
            filter_pipe_num++;
			if( filter_pipe_max_num_limit > 0 )
			{
				filter_pipe_max_num_limit--;	
			}
			else
			{
				filter_pipe_max_num_limit = 0;
			}
			
			LOG_DEBUG( "gcm_do_global_filter_dispatch, pipe_ptr:0x%x, is_filted:%d, res_ptr:0x%x, filter_pipe_num:%d, filter_pipe_max_num_limit:%d",
				p_data_pipe, p_data_pipe->_dispatch_info._is_global_filted, p_data_pipe->_p_resource, filter_pipe_num, filter_pipe_max_num_limit );

			gcm_get_ptr()->_filter_manager._cur_total_speed -= p_data_pipe->_dispatch_info._speed;
		}
		
		if( p_data_pipe->_dispatch_info._score > 0 && filter_pipe_max_num_limit == 0 )
		{
			LOG_DEBUG( "gcm_do_global_filter_dispatch break, filter_pipe_num:%d, _score:%d, filter_pipe_max_num_limit:%d", 
				filter_pipe_num, p_data_pipe->_dispatch_info._score, filter_pipe_max_num_limit );
			break;
		}
        
		cur_node = LIST_PRE( cur_node );
	}
	
	LOG_DEBUG( "gcm_do_global_filter_dispatch end, filter_pipe_num:%d, filter_pipe_max_num_limit:%d", 
		filter_pipe_num, filter_pipe_max_num_limit );
	return SUCCESS;
}

