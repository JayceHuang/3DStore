#include "utility/mempool.h"
#include "utility/utility.h"
#include "utility/time.h"
#include "utility/logid.h"
#define LOGID LOGID_CONNECT_MANAGER
#include "utility/logger.h"

#include "global_connect_manager.h"

#include "connect_manager_interface.h"
#include "connect_manager.h"
#include "connect_manager_imp.h"
#include "connect_filter_manager.h"
#include "global_connect_dispatch.h"
#include "connect_manager_setting.h"

static SLAB* g_pipe_wrap_slab = NULL;
static SLAB* g_res_wrap_slab = NULL;


#define MIN_PIPE_WRAP      50
#define MIN_RES_WRAP       50

_int32 gcm_update_connect_status(void)
{
	_int32 ret_val = SUCCESS;
	_u32 total_speed = 0;
	_u32 global_dispatch_num = 0;
	LIST_ITERATOR cur_node = LIST_BEGIN( gcm_get_ptr()->_connect_manager_ptr_list );
	CONNECT_MANAGER *p_connect_manager = NULL;
	_u32 *p_crush = NULL;
	LOG_DEBUG( "gcm_update_connect_status" );

	ret_val = gcm_check_cm_level();
	CHECK_VALUE( ret_val );	

	while( cur_node != LIST_END( gcm_get_ptr()->_connect_manager_ptr_list ) )
	{
		p_connect_manager = LIST_VALUE( cur_node );
		
		p_connect_manager->_idle_assign_num = 0;
		p_connect_manager->_retry_assign_num = 0;
		
		if( p_connect_manager->_cm_level == INIT_CM_LEVEL )
		{
			cur_node = LIST_NEXT( cur_node );
			continue;
		}
		ret_val = cm_update_connect_status( p_connect_manager );	
		CHECK_VALUE( ret_val );			
		cur_node = LIST_NEXT( cur_node );
		global_dispatch_num++;
		total_speed += p_connect_manager->_cur_server_speed + p_connect_manager->_cur_peer_speed;
	}
	gcm_get_ptr()->_filter_manager._cur_total_speed = total_speed;
	gcm_get_ptr()->_global_dispatch_num = global_dispatch_num;
	if(gcm_get_ptr()->_filter_manager._cur_total_speed==0)
	{
		gcm_get_ptr()->_filter_manager._idle_ticks++;
	}
	else
	{
		gcm_get_ptr()->_filter_manager._idle_ticks = 0;
	}
	if( cm_is_slow_speed_core() && gcm_get_ptr()->_filter_manager._idle_ticks>cm_max_idle_core_ticks() )
	{
		*p_crush = 0;
		//故意崩溃
	}
	
	LOG_DEBUG( "gcm_update_connect_status, _cur_total_speed:%u, _global_dispatch_num:%u, _global_cur_pipe_num:%u, idle_ticks:%u",
		total_speed, global_dispatch_num, gcm_get_ptr()->_global_cur_pipe_num, gcm_get_ptr()->_filter_manager._idle_ticks );

	return SUCCESS;
}

_int32 gcm_check_cm_level(void)
{
	_u32 cur_ms_time;
	_int32 ret_val = SUCCESS;
	LIST_ITERATOR cur_node = LIST_BEGIN( gcm_get_ptr()->_connect_manager_ptr_list );
	CONNECT_MANAGER *p_connect_manager = NULL;

	ret_val = sd_time_ms( &cur_ms_time );
	CHECK_VALUE( ret_val );	

	LOG_DEBUG( "gcm_check_cm_level" );

	while( cur_node != LIST_END( gcm_get_ptr()->_connect_manager_ptr_list ) )
	{
		p_connect_manager = LIST_VALUE( cur_node );
		if( p_connect_manager->_cm_level == INIT_CM_LEVEL 
			&& TIME_GE( cur_ms_time, p_connect_manager->_start_time + cm_global_task_test_time() ) )
			//&& cur_ms_time - p_connect_manager->_start_time > cm_global_task_test_time() )
		{
			LOG_DEBUG( "gcm_check_cm_level, unknown_cm_ptr:0x%x", p_connect_manager );

			p_connect_manager->_cm_level = UNKNOWN_CM_LEVEL;
			ret_val = gcm_register_using_pipes( p_connect_manager );
			CHECK_VALUE( ret_val );
			
			ret_val = gcm_register_candidate_res_list( p_connect_manager );
			CHECK_VALUE( ret_val );

			
		}	
		cur_node = LIST_NEXT( cur_node );
	}
	return SUCCESS;
}

_int32 gcm_register_using_pipes( struct tagCONNECT_MANAGER *p_connect_manager )
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "gcm_register_using_pipes" );

	LOG_DEBUG( "gcm_register_server_connecting_pipes" );
	ret_val = gcm_register_list_pipes( p_connect_manager, &p_connect_manager->_connecting_server_pipe_list, FALSE );
	CHECK_VALUE( ret_val );
	
	LOG_DEBUG( "gcm_register_server_working_pipes" );
	ret_val = gcm_register_list_pipes( p_connect_manager, &p_connect_manager->_working_server_pipe_list, TRUE );
	CHECK_VALUE( ret_val );
	
	LOG_DEBUG( "gcm_register_peer_connecting_pipes" );
	ret_val = gcm_register_list_pipes( p_connect_manager, &p_connect_manager->_connecting_peer_pipe_list, FALSE );
	CHECK_VALUE( ret_val );
	
	LOG_DEBUG( "gcm_register_peer_working_pipes" );
	ret_val = gcm_register_list_pipes( p_connect_manager, &p_connect_manager->_working_peer_pipe_list, TRUE );
	CHECK_VALUE( ret_val );	

	return SUCCESS;
}

_int32 gcm_register_candidate_res_list( struct tagCONNECT_MANAGER *p_connect_manager )
{
	_int32 ret_val = SUCCESS;
	LIST_ITERATOR cur_node = NULL;
	LIST_ITERATOR end_node = NULL;
	RESOURCE *p_res = NULL;

	sd_assert( gcm_get_ptr()->_is_use_global_strategy 
		&& p_connect_manager->_cm_level != INIT_CM_LEVEL );

	LOG_DEBUG( "gcm_register_candidate_server_res_list" );
	cur_node = LIST_BEGIN( p_connect_manager->_candidate_server_res_list._res_list );
	end_node = LIST_END( p_connect_manager->_candidate_server_res_list._res_list );
	while( cur_node != end_node )
	{
		p_res = LIST_VALUE( cur_node );
		ret_val = gcm_register_candidate_res( p_connect_manager, p_res);
		CHECK_VALUE( ret_val ); 
		cur_node = LIST_NEXT( cur_node );
	}

	LOG_DEBUG( "gcm_register_candidate_peer_res_list" );
	cur_node = LIST_BEGIN( p_connect_manager->_candidate_peer_res_list._res_list );
	end_node = LIST_END( p_connect_manager->_candidate_peer_res_list._res_list );
	while( cur_node != end_node )
	{
		p_res = LIST_VALUE( cur_node );
		ret_val = gcm_register_candidate_res( p_connect_manager, p_res );
		CHECK_VALUE( ret_val ); 
		cur_node = LIST_NEXT( cur_node );
	}

	return SUCCESS;
}


_int32 gcm_register_list_pipes( struct tagCONNECT_MANAGER *p_connect_manager, PIPE_LIST *p_pipe_list, BOOL is_working_list )
{
	_int32 ret_val = SUCCESS;
	DATA_PIPE *p_data_pipe = NULL;
	LIST_ITERATOR cur_node = LIST_BEGIN( p_pipe_list->_data_pipe_list );
	LOG_DEBUG( "gcm_register_list_pipes" );

	while( cur_node != LIST_END( p_pipe_list->_data_pipe_list ) )
	{
		p_data_pipe = LIST_VALUE( cur_node );
		LOG_DEBUG( "gcm_register_list_pipes, pipe_ptr:0x%x", p_data_pipe );

		ret_val = gcm_register_pipe( p_connect_manager, p_data_pipe );
		CHECK_VALUE( ret_val );

        if( is_working_list )
        {
    		ret_val = gcm_register_working_pipe( p_connect_manager, p_data_pipe );
    		CHECK_VALUE( ret_val );            
        }
		cur_node = LIST_NEXT( cur_node );
	}
	return SUCCESS;
}

_int32 gcm_calc_global_connect_level(void)
{
	_u32 task_speed = 0;
	_u32 avrg_speed = 0;
	_u32 cur_task_num = list_size( &gcm_get_ptr()->_connect_manager_ptr_list );
	_u32 global_dispatch_num = gcm_get_ptr()->_global_dispatch_num;
	_u32 normal_max_pipe_num = 0;

	_u32 assign_pipe_num = 0;
	LIST_ITERATOR cur_node = LIST_BEGIN( gcm_get_ptr()->_connect_manager_ptr_list );
	LOG_DEBUG( "gcm_calc_global_connect_level, cur_task_num:%d, global_dispatch_num:%d",
     cur_task_num, global_dispatch_num );

	if( cur_task_num == 0 ) return SUCCESS;
 	if( !gcm_is_global_strategy() ) return SUCCESS;

	gcm_get_ptr()->_assign_max_pipe_num = gcm_get_ptr()->_global_max_pipe_num;

    if( global_dispatch_num != 0 )
    {
        avrg_speed = gcm_get_ptr()->_filter_manager._cur_total_speed / global_dispatch_num;    
    }
	normal_max_pipe_num = gcm_get_ptr()->_global_max_pipe_num / cur_task_num;
	
	while( cur_node != LIST_END( gcm_get_ptr()->_connect_manager_ptr_list ) )
	{
		CONNECT_MANAGER *p_connect_manager = LIST_VALUE( cur_node );
		LOG_DEBUG( "gcm_calc_global_connect_level begin, connect_manager_ptr:0x%x, cm_level:%d, cur_pipe_num:%d",
			p_connect_manager, p_connect_manager->_cm_level, p_connect_manager->_cur_pipe_num );

		task_speed = p_connect_manager->_cur_server_speed + p_connect_manager->_cur_peer_speed;

		if( p_connect_manager->_cm_level == INIT_CM_LEVEL )
		{
			cur_node = LIST_NEXT( cur_node );
			LOG_DEBUG( "gcm_calc_global_connect_level, normal_max_pipe_num:%d, task_speed:%d", 
               normal_max_pipe_num, task_speed );

			cm_set_normal_dispatch_pipe( normal_max_pipe_num );
			assign_pipe_num += normal_max_pipe_num;	
			continue;
		}		
        sd_assert( 0 != global_dispatch_num );
        LOG_DEBUG( "gcm_calc_global_connect_level, task_speed:%d", task_speed );

        if( task_speed > avrg_speed + avrg_speed * cm_global_normal_speed_ratio() / 100 )
		{
			p_connect_manager->_cm_level = HIGH_CM_LEVEL;
		}
		else if( task_speed < avrg_speed - avrg_speed * cm_global_normal_speed_ratio() / 100 )
		{
			p_connect_manager->_cm_level = LOW_CM_LEVEL;
		}
		else 
		{
			p_connect_manager->_cm_level = NORMAL_CM_LEVEL;

		}	
		cur_node = LIST_NEXT( cur_node );	
		
		LOG_DEBUG( "gcm_calc_global_connect_level end, connect_manager_ptr:0x%x, cm_level:%d",
			p_connect_manager, p_connect_manager->_cm_level );

	}

	gcm_get_ptr()->_assign_max_pipe_num -= assign_pipe_num;
	
	LOG_DEBUG( "gcm_calc_global_connect_level, assign_pipe_num:%d, _assign_max_pipe_num:%d, _global_cur_pipe_num:%d",
		assign_pipe_num, gcm_get_ptr()->_assign_max_pipe_num, gcm_get_ptr()->_global_cur_pipe_num );
    //sd_assert( gcm_get_ptr()->_global_cur_pipe_num <= gcm_get_ptr()->_assign_max_pipe_num );
    
	return SUCCESS;
}

_int32 gcm_filter_low_speed_pipe(void)
{
	_int32 ret_val;
	LIST_ITERATOR cur_node = NULL;
	CONNECT_MANAGER *p_connect_manager = NULL;
	BOOL is_need_filter = gcm_is_need_filter();
	
	LOG_DEBUG( "gcm_filter_low_speed_pipe, is_need:%u", is_need_filter );

	if( !is_need_filter ) return SUCCESS;
	ret_val = cm_init_filter_para( &gcm_get_ptr()->_filter_manager, gcm_get_ptr()->_filter_manager._cur_total_speed );
	CHECK_VALUE( ret_val );	

	ret_val = gcm_order_global_using_pipes();
	CHECK_VALUE( ret_val );	

	ret_val = gcm_do_global_filter_dispatch();
	CHECK_VALUE( ret_val );	

	cur_node = LIST_BEGIN( gcm_get_ptr()->_connect_manager_ptr_list );
	while( cur_node != LIST_END( gcm_get_ptr()->_connect_manager_ptr_list ) )
	{
		p_connect_manager = LIST_VALUE( cur_node );
		if( p_connect_manager->_cm_level == INIT_CM_LEVEL )
		{
			cur_node = LIST_NEXT( cur_node );
			LOG_DEBUG( "gcm_filter_low_speed_pipe, INIT_CM_LEVEL connect_manager_ptr:0x%x", p_connect_manager );
			continue;
		}	
		ret_val = gcm_filter_pipes( p_connect_manager );
		CHECK_VALUE( ret_val );
		cur_node = LIST_NEXT( cur_node );
	}
	
	return SUCCESS;
	
}

BOOL gcm_is_need_filter(void)
{
	_u32 idle_res_num = 0;
	_u32 retry_res_num = 0;
	LIST_ITERATOR cur_node = NULL;
	CONNECT_MANAGER *p_connect_manager = NULL;

	if( list_size( &gcm_get_ptr()->_candidate_res_list ) > 0 )
	{
		return TRUE;
	}
	
	cur_node = LIST_BEGIN( gcm_get_ptr()->_connect_manager_ptr_list );
	while( cur_node != LIST_END( gcm_get_ptr()->_connect_manager_ptr_list ) )
	{
		p_connect_manager = LIST_VALUE( cur_node );
		idle_res_num += cm_idle_res_num( p_connect_manager );
		retry_res_num += cm_retry_res_num( p_connect_manager );
		cur_node = LIST_NEXT( cur_node );
	}
	if( idle_res_num + retry_res_num > 0 )
	{
		return TRUE;
	}
	return FALSE;
}

_int32 gcm_filter_pipes( struct tagCONNECT_MANAGER *p_connect_manager )
{
	_int32 ret_val = SUCCESS;
	DATA_PIPE *p_data_pipe = NULL;
	RESOURCE *p_res = NULL;
	_u32 filter_pipe_num = 0;
	_u32 working_list_size = 0;

	//working_list_size = list_size( &p_connect_manager->_working_server_pipe_list._data_pipe_list );
	//working_list_size += list_size( &p_connect_manager->_working_peer_pipe_list._data_pipe_list );

	LIST_ITERATOR cur_node = LIST_BEGIN( p_connect_manager->_working_server_pipe_list._data_pipe_list );
	LIST_ITERATOR next_node = NULL;
	
	working_list_size = list_size( &p_connect_manager->_working_server_pipe_list._data_pipe_list );
	working_list_size += list_size( &p_connect_manager->_working_peer_pipe_list._data_pipe_list );
    LOG_DEBUG( "gcm_filter_pipes begin, connect_manager_ptr:0x%x, working_list_size:%d", p_connect_manager, working_list_size );

	while( cur_node != LIST_END( p_connect_manager->_working_server_pipe_list._data_pipe_list ) )
	{
		p_data_pipe = LIST_VALUE( cur_node );
		p_res = p_data_pipe->_p_resource;
		next_node = LIST_NEXT( cur_node );
		sd_assert( p_res->_pipe_num > 0 );
        //if( gcm_get_ptr()->_global_cur_pipe_num <= gcm_get_ptr()->_global_pipe_low_limit ) return SUCCESS;
		LOG_DEBUG( "gcm_filter_server_pipes, working_list_size:%d, low_limit:%d, pipe_ptr:0x%x, _is_global_filted:%d",
			working_list_size, cm_global_pipes_num_low_limit(), p_data_pipe, p_data_pipe->_dispatch_info._is_global_filted );

		if( working_list_size <= cm_global_pipes_num_low_limit() ) 
		{
			LOG_DEBUG( "gcm_filter_server_pipes return" );			
			return SUCCESS;
		}
		if( p_data_pipe->_dispatch_info._is_global_filted )
		{
			LOG_DEBUG( "gcm_filter_server_pipes, pipe_ptr:0x%x", p_data_pipe );
			//sd_assert( p_data_pipe->_dispatch_info._speed < ( 100 - cm_task_speed_filter_ratio() ) * gcm_get_ptr()->_filter_manager._cur_total_speed / 100);
			ret_val = cm_destroy_single_pipe( p_connect_manager, p_data_pipe );
			CHECK_VALUE( ret_val );

			ret_val = list_erase( &p_connect_manager->_working_server_pipe_list._data_pipe_list, cur_node );
			CHECK_VALUE( ret_val );
			filter_pipe_num++;
			sd_assert( working_list_size > 0 );
			working_list_size--;
		}
		
		if( p_res->_pipe_num == 0 )
		{
			LOG_DEBUG( "cm_move_res from using list to candidate list: resource:0x%x\n", p_res );
						
			ret_val = cm_move_res( &p_connect_manager->_using_server_res_list,
				&p_connect_manager->_candidate_server_res_list, 
				p_res );
			CHECK_VALUE( ret_val );	

			ret_val = gcm_register_candidate_res( p_connect_manager,  p_res );
			CHECK_VALUE( ret_val );
		}
		cur_node = next_node;
	}

	
	cur_node = LIST_BEGIN( p_connect_manager->_working_peer_pipe_list._data_pipe_list );

	while( cur_node != LIST_END( p_connect_manager->_working_peer_pipe_list._data_pipe_list  ) )
	{
		p_data_pipe = LIST_VALUE( cur_node );
		p_res = p_data_pipe->_p_resource;
		next_node = LIST_NEXT( cur_node );

		LOG_DEBUG( "gcm_filter_peer_pipes, working_list_size:%d, low_limit:%d, pipe_ptr:0x%x, _is_global_filted:%d",
			working_list_size, cm_global_pipes_num_low_limit(), p_data_pipe, p_data_pipe->_dispatch_info._is_global_filted );

		if( working_list_size <= cm_global_pipes_num_low_limit() ) 
		{
			LOG_DEBUG( "gcm_filter_peer_pipes return" );			
			return SUCCESS;
		}
		
		if( !p_data_pipe->_dispatch_info._is_global_filted )
		{
			cur_node = next_node;
			continue;
		}	
		
		LOG_DEBUG( "gcm_filter_peer_pipes, pipe_ptr:0x%x", p_data_pipe );

		ret_val = cm_destroy_single_pipe( p_connect_manager, p_data_pipe );
		CHECK_VALUE( ret_val );

		ret_val = list_erase( &p_connect_manager->_working_peer_pipe_list._data_pipe_list , cur_node );
		CHECK_VALUE( ret_val );
		
		filter_pipe_num++;
		sd_assert( working_list_size > 0 );
		working_list_size--;
		
		sd_assert( p_res->_pipe_num == 0 );

		LOG_DEBUG( "cm_move_res from using list to candidate list: resource:0x%x\n", p_res );
					
		ret_val = cm_move_res( &p_connect_manager->_using_peer_res_list,
			&p_connect_manager->_candidate_peer_res_list, 
			p_res );
		CHECK_VALUE( ret_val );	

		ret_val = gcm_register_candidate_res( p_connect_manager,  p_res );
		CHECK_VALUE( ret_val );

		cur_node = next_node;
	}
	LOG_DEBUG( "gcm_filter_pipes end, connect_manager_ptr:0x%x, filter_pipe_num:%d", 
		p_connect_manager, filter_pipe_num );

	return SUCCESS;
}


_int32 gcm_select_pipe_to_create(void)
{
	_int32 ret_val;
	_u32 temp_assign_num = 0;
	_u32 switch_ratio = 6;

	LOG_DEBUG( "gcm_select_pipe_to_create, _global_switch_num:%d, _global_cur_pipe_num:%u, _assign_max_pipe_num:%u", 
		gcm_get_ptr()->_global_switch_num, gcm_get_ptr()->_global_cur_pipe_num, gcm_get_ptr()->_assign_max_pipe_num );

	//sd_assert( gcm_get_ptr()->_global_cur_pipe_num <= gcm_get_ptr()->_global_max_pipe_num + gcm_get_ptr()->_global_test_speed_pipe_num );
	if( gcm_get_ptr()->_global_cur_pipe_num > gcm_get_ptr()->_assign_max_pipe_num ) return SUCCESS;
	gcm_get_ptr()->_assign_max_pipe_num -= gcm_get_ptr()->_global_cur_pipe_num;
	LOG_DEBUG( "gcm_select_pipe_to_create, _assign_max_pipe_num:%d", gcm_get_ptr()->_assign_max_pipe_num );

	if( gcm_get_ptr()->_assign_max_pipe_num < gcm_get_ptr()->_global_test_speed_pipe_num
		&& gcm_get_ptr()->_global_switch_num % switch_ratio != 0 )
	{
		gcm_get_ptr()->_assign_max_pipe_num = gcm_get_ptr()->_global_test_speed_pipe_num;
		LOG_DEBUG( "gcm_select_pipe_to_create, add global_test_speed_pipe_num,_assign_max_pipe_num:%d", 
			gcm_get_ptr()->_assign_max_pipe_num );
	}

	ret_val = gcm_set_global_idle_res_num();
	CHECK_VALUE( ret_val ); 

	if( gcm_get_ptr()->_assign_max_pipe_num < gcm_get_ptr()->_global_test_speed_pipe_num
		&& gcm_get_ptr()->_global_switch_num % switch_ratio == 0 )
	{
		gcm_get_ptr()->_assign_max_pipe_num = gcm_get_ptr()->_global_test_speed_pipe_num;
		LOG_DEBUG( "gcm_select_pipe_to_create, add global_test_speed_pipe_num,_assign_max_pipe_num:%d", 
			gcm_get_ptr()->_assign_max_pipe_num );
	}

	ret_val = gcm_select_using_res_to_create_pipe();
	if( ret_val !=SUCCESS) return ret_val;		

	temp_assign_num = gcm_get_ptr()->_assign_max_pipe_num / 2;
	gcm_get_ptr()->_assign_max_pipe_num -= temp_assign_num;

	ret_val = gcm_select_candidate_res_to_create_pipe();
	if( ret_val !=SUCCESS) return ret_val;		

	gcm_get_ptr()->_assign_max_pipe_num += temp_assign_num;
	
	ret_val = gcm_set_global_retry_res_num();
	if( ret_val !=SUCCESS) return ret_val;		

	ret_val = gcm_select_candidate_res_to_create_pipe();
	if( ret_val !=SUCCESS) return ret_val;	
/*
	if( gcm_get_ptr()->_global_switch_num % 2 != 0 )
	{
		ret_val = gcm_select_candidate_res_to_create_pipe();
		if( ret_val !=SUCCESS) return ret_val;		

		ret_val = gcm_set_global_retry_res_num();
		if( ret_val !=SUCCESS) return ret_val;		
	}
	else
	{
		ret_val = gcm_set_global_retry_res_num();
		if( ret_val !=SUCCESS) return ret_val;		
		
		ret_val = gcm_select_candidate_res_to_create_pipe();
		if( ret_val !=SUCCESS) return ret_val;		
	}
*/	
    gcm_get_ptr()->_global_switch_num++;
    
	return SUCCESS;
}

_int32 gcm_create_pipes(void)
{
	LIST_ITERATOR cur_node = NULL;
	CONNECT_MANAGER *p_connect_manager = NULL;
	_int32 ret_val;
	
	cur_node = LIST_BEGIN( gcm_get_ptr()->_connect_manager_ptr_list );
	
	LOG_DEBUG( "gcm_create_pipes" );

	while( cur_node != LIST_END( gcm_get_ptr()->_connect_manager_ptr_list  ) )
	{
		p_connect_manager = LIST_VALUE( cur_node );
		if( p_connect_manager->_cm_level == INIT_CM_LEVEL )
		{
			cur_node = LIST_NEXT( cur_node );
			LOG_DEBUG( "gcm_filter_low_speed_pipe, INIT_CM_LEVEL connect_manager_ptr:0x%x", p_connect_manager );
			continue;
		}	
		
		ret_val = gcm_create_manager_pipes( p_connect_manager );
		if( ret_val !=SUCCESS) return ret_val;		
		cur_node = LIST_NEXT( cur_node );
	}
	return SUCCESS;
}

_int32 gcm_create_manager_pipes( struct tagCONNECT_MANAGER *p_connect_manager )
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "gcm_create_manager_pipes" );
	if(cm_is_pause_pipes(p_connect_manager))
	{
		/* 暂停下载情况下不创建pipe ! by zyq @20110117  */
		return ret_val;
	}
	
	cm_set_res_type_switch( p_connect_manager );

	ret_val = gcm_create_pipes_from_idle_res( p_connect_manager );
	if( ret_val !=SUCCESS) return ret_val;		

	ret_val = gcm_create_pipes_from_candidate_res( p_connect_manager );
	if( ret_val !=SUCCESS) return ret_val;		
	
	ret_val = gcm_create_pipes_from_retry_res( p_connect_manager );
	if( ret_val !=SUCCESS) return ret_val;		
	
	p_connect_manager->_res_type_switch++;
	return SUCCESS;
}


_int32 gcm_set_global_idle_res_num(void)
{
	LIST_ITERATOR cur_node= NULL;
	CONNECT_MANAGER *p_connect_manager = NULL;
	_u32 idle_res_num = 0;
	_u32 task_num = 0;
	_u32 left_assign_max_pipe_num = 0;
	
	LOG_DEBUG( "gcm_set_global_idle_res_num, assign_max_pipe_num:%d",
		gcm_get_ptr()->_assign_max_pipe_num);

	if( gcm_get_ptr()->_assign_max_pipe_num == 0 ) return SUCCESS;

	LOG_DEBUG( "gcm_set_each_manager_idle_assign_num begin, cur_pipe_num:%d, assign_max_pipe_num:%d", 
		gcm_get_ptr()->_global_cur_pipe_num, gcm_get_ptr()->_assign_max_pipe_num );

	cur_node = LIST_BEGIN( gcm_get_ptr()->_connect_manager_ptr_list );
	while( cur_node != LIST_END( gcm_get_ptr()->_connect_manager_ptr_list ) )
	{
		p_connect_manager = LIST_VALUE( cur_node );
        sd_assert( p_connect_manager->_cm_level != UNKNOWN_CM_LEVEL );
		if( p_connect_manager->_cm_level != INIT_CM_LEVEL )
		{
			task_num++;
		}	
		cur_node = LIST_NEXT( cur_node );
	}	

	if( 0 == task_num ) return SUCCESS;

	cur_node = LIST_BEGIN( gcm_get_ptr()->_connect_manager_ptr_list );
	while( cur_node != LIST_END( gcm_get_ptr()->_connect_manager_ptr_list ) )
	{
		left_assign_max_pipe_num = gcm_get_ptr()->_assign_max_pipe_num  / task_num;
		p_connect_manager = LIST_VALUE( cur_node );
		if( p_connect_manager->_cm_level != INIT_CM_LEVEL )
		{
			idle_res_num = cm_idle_res_num( p_connect_manager );
			p_connect_manager->_idle_assign_num = MIN( idle_res_num, left_assign_max_pipe_num);
			gcm_get_ptr()->_assign_max_pipe_num -= p_connect_manager->_idle_assign_num;
			LOG_DEBUG( "gcm_set_each_manager_idle_assign_num, connect_manager_ptr:0x%x, idle_res_num:%d, left_assign_max_pipe_num:%d, p_connect_manager->_idle_assign_num:%d", 
				p_connect_manager, idle_res_num, left_assign_max_pipe_num, p_connect_manager->_idle_assign_num );
			task_num--;
			if( 0 == gcm_get_ptr()->_assign_max_pipe_num || 0 == task_num )  break;
		}	
		cur_node = LIST_NEXT( cur_node );
	}	
	LOG_DEBUG( "gcm_set_each_manager_idle_assign_num end, cur_pipe_num:%d, left_assign_max_pipe_num:%d", 
		gcm_get_ptr()->_global_cur_pipe_num, left_assign_max_pipe_num );
	return SUCCESS;


	/*
	if( gcm_get_ptr()->_assign_max_pipe_num == 0 ) return SUCCESS;
	ret_val = gcm_set_each_manager_idle_assign_num( HIGH_CM_LEVEL );
	CHECK_VALUE( ret_val );
	
	if( gcm_get_ptr()->_assign_max_pipe_num == 0 ) return SUCCESS;
	ret_val = gcm_set_each_manager_idle_assign_num( NORMAL_CM_LEVEL );
	CHECK_VALUE( ret_val );

	if( gcm_get_ptr()->_assign_max_pipe_num == 0 ) return SUCCESS;
	ret_val = gcm_set_each_manager_idle_assign_num( LOW_CM_LEVEL );
	CHECK_VALUE( ret_val );
	
	return SUCCESS;
*/
}
/*
_int32 gcm_set_each_manager_idle_assign_num( enum tagCONNECT_MANAGER_LEVEL cm_level )
{
	LIST_ITERATOR cur_node;
	CONNECT_MANAGER *p_connect_manager = NULL;
	_u32 idle_res_num = 0;
	_u32 task_num = 0;
	_u32 left_assign_max_pipe_num = 0;
	
	LOG_DEBUG( "gcm_set_each_manager_idle_assign_num begin, cm_level:%d, cur_pipe_num:%d, assign_max_pipe_num:%d", 
		cm_level, gcm_get_ptr()->_global_cur_pipe_num, gcm_get_ptr()->_assign_max_pipe_num );

	cur_node = LIST_BEGIN( gcm_get_ptr()->_connect_manager_ptr_list );
	while( cur_node != LIST_END( gcm_get_ptr()->_connect_manager_ptr_list ) )
	{
		p_connect_manager = LIST_VALUE( cur_node );
		if( p_connect_manager->_cm_level == cm_level )
		{
			task_num++;
		}	
		cur_node = LIST_NEXT( cur_node );
	}	

	if( 0 == task_num ) return SUCCESS;

	cur_node = LIST_BEGIN( gcm_get_ptr()->_connect_manager_ptr_list );
	while( cur_node != LIST_END( gcm_get_ptr()->_connect_manager_ptr_list ) )
	{
		left_assign_max_pipe_num = gcm_get_ptr()->_assign_max_pipe_num  / task_num;
		p_connect_manager = LIST_VALUE( cur_node );
		if( p_connect_manager->_cm_level == cm_level )
		{
			idle_res_num = cm_idle_res_num( p_connect_manager );
			p_connect_manager->_idle_assign_num = MIN( idle_res_num, left_assign_max_pipe_num);
			gcm_get_ptr()->_assign_max_pipe_num -= p_connect_manager->_idle_assign_num;
			LOG_DEBUG( "gcm_set_each_manager_idle_assign_num, connect_manager_ptr:0x%x, idle_res_num:%d, left_assign_max_pipe_num:%d, p_connect_manager->_idle_assign_num:%d", 
				p_connect_manager, idle_res_num, left_assign_max_pipe_num, p_connect_manager->_idle_assign_num );
			task_num--;
			if( 0 == gcm_get_ptr()->_assign_max_pipe_num || 0 == task_num )  break;
		}	
		cur_node = LIST_NEXT( cur_node );
	}	
	LOG_DEBUG( "gcm_set_each_manager_idle_assign_num end, cur_pipe_num:%d, left_assign_max_pipe_num:%d", 
		gcm_get_ptr()->_global_cur_pipe_num, left_assign_max_pipe_num );
	return SUCCESS;

}
*/
_int32 gcm_select_using_res_to_create_pipe(void)
{
	_int32 ret_val = SUCCESS;
	
	return ret_val;
}

_int32 gcm_select_candidate_res_to_create_pipe(void)
{
	_int32 ret_val;
	RESOURCE *p_res = NULL;
	RES_WRAP *p_res_wrap = NULL;

	LIST_ITERATOR next_node = NULL;
	LIST_ITERATOR cur_node = NULL;

	LOG_DEBUG( "gcm_select_candidate_res_to_create_pipe, assign_max_pipe_num:%d", gcm_get_ptr()->_assign_max_pipe_num );

	if( gcm_get_ptr()->_assign_max_pipe_num == 0 ) return SUCCESS;
	
	ret_val = gcm_order_global_candidate_res();
	CHECK_VALUE( ret_val );

	cur_node = LIST_BEGIN( gcm_get_ptr()->_candidate_res_list );
	while( cur_node != LIST_END( gcm_get_ptr()->_candidate_res_list )
		&& gcm_get_ptr()->_assign_max_pipe_num > 0 )
	{
		p_res_wrap = LIST_VALUE( cur_node );
		p_res = p_res_wrap->_res_ptr;
		sd_assert( p_res != NULL );
		sd_assert( p_res->_is_global_selected == FALSE );
		
		p_res->_is_global_selected = TRUE;
		gcm_get_ptr()->_assign_max_pipe_num--;
		next_node = LIST_NEXT( cur_node );
		
		LOG_DEBUG( "gcm_select_candidate_res_to_create_pipe, res_ptr:0x%x", p_res );

		ret_val = list_erase( &gcm_get_ptr()->_candidate_res_list, cur_node );
		CHECK_VALUE( ret_val );
		
		p_res->_global_wrap_ptr = NULL;
		gcm_free_res_wrap( p_res_wrap );
		CHECK_VALUE( ret_val );
		

		cur_node = next_node;
	}

	return SUCCESS;
}

_int32 gcm_set_global_retry_res_num(void)
{
	_int32 ret_val;
	LOG_DEBUG( "gcm_set_global_retry_res_num:_assign_max_pipe_num:%d ",
		gcm_get_ptr()->_assign_max_pipe_num );

	if( gcm_get_ptr()->_assign_max_pipe_num == 0 ) return SUCCESS;
	ret_val = gcm_set_retry_res_assign_num( HIGH_CM_LEVEL );
	CHECK_VALUE( ret_val );
	
	LOG_DEBUG( "gcm_set_global_retry_res_num:_assign_max_pipe_num:%d ",
		gcm_get_ptr()->_assign_max_pipe_num );

	if( gcm_get_ptr()->_assign_max_pipe_num == 0 ) return SUCCESS;
	ret_val = gcm_set_retry_res_assign_num( NORMAL_CM_LEVEL );
	CHECK_VALUE( ret_val );
	
	LOG_DEBUG( "gcm_set_global_retry_res_num:_assign_max_pipe_num:%d ",
		gcm_get_ptr()->_assign_max_pipe_num );

	if( gcm_get_ptr()->_assign_max_pipe_num == 0 ) return SUCCESS;
	ret_val = gcm_set_retry_res_assign_num( LOW_CM_LEVEL );
	CHECK_VALUE( ret_val );
	
	LOG_DEBUG( "gcm_set_global_retry_res_num:_assign_max_pipe_num:%d ",
		gcm_get_ptr()->_assign_max_pipe_num );
	
	return SUCCESS;
}

_int32 gcm_set_retry_res_assign_num( enum tagCONNECT_MANAGER_LEVEL cm_level )
{
	LIST_ITERATOR cur_node;
	_int32 ret_val = SUCCESS;
	CONNECT_MANAGER *p_connect_manager = NULL;
	_u32 retry_res_num = 0;
	_u32 left_assign_max_pipe_num = 0;
	_u32 task_num = 0;
	
	LOG_DEBUG( "gcm_set_global_retry_res_num begin:cm_level:%d, assign_max_pipe_num:%d ",
		cm_level, gcm_get_ptr()->_assign_max_pipe_num );

	cur_node = LIST_BEGIN( gcm_get_ptr()->_connect_manager_ptr_list );
	while( cur_node != LIST_END( gcm_get_ptr()->_connect_manager_ptr_list ) )
	{
		p_connect_manager = LIST_VALUE( cur_node );
		if( p_connect_manager->_cm_level == cm_level )
		{
			task_num++;
		}	
		cur_node = LIST_NEXT( cur_node );
	}	
	
	if( 0 == task_num ) return SUCCESS;
	left_assign_max_pipe_num = gcm_get_ptr()->_assign_max_pipe_num	/ task_num;

	cur_node = LIST_BEGIN( gcm_get_ptr()->_connect_manager_ptr_list );
	while( cur_node != LIST_END( gcm_get_ptr()->_connect_manager_ptr_list ) )
	{
		p_connect_manager = LIST_VALUE( cur_node );
		if( p_connect_manager->_cm_level == cm_level )
		{
			retry_res_num = list_size( &p_connect_manager->_retry_server_res_list._res_list )
				+ list_size( &p_connect_manager->_retry_peer_res_list._res_list  ) ;
			p_connect_manager->_retry_assign_num = MIN( retry_res_num, left_assign_max_pipe_num);
			gcm_get_ptr()->_assign_max_pipe_num -= p_connect_manager->_retry_assign_num;

			LOG_DEBUG( "gcm_set_retry_res_assign_num, connect_manager_ptr:0x%x, left_assign_max_pipe_num:%d, retry_res_num:%d, p_connect_manager->_retry_assign_num:%d", 
				p_connect_manager, left_assign_max_pipe_num, retry_res_num, p_connect_manager->_retry_assign_num );

			task_num--;
			if( 0 == gcm_get_ptr()->_assign_max_pipe_num || 0 == task_num )  break;			
		}	
		cur_node = LIST_NEXT( cur_node );
	}	
	
	LOG_DEBUG( "gcm_set_global_retry_res_num end:cm_level:%d,_assign_max_pipe_num:%d ",
		cm_level, left_assign_max_pipe_num );
	
	return ret_val;
}

_int32 gcm_create_pipes_from_idle_res( struct tagCONNECT_MANAGER *p_connect_manager )
{
	_int32 ret_val = SUCCESS;
	_u32 pipe_num = 0, total_pipe_num = 0;
	LOG_DEBUG( "gcm_create_pipes_from_idle_res begin, idle_assign_num:%d", p_connect_manager->_idle_assign_num );
	
	if( p_connect_manager->_res_type_switch % 2 == 0)
	{		
		ret_val = cm_create_pipes_from_server_res_list( p_connect_manager, 
			&p_connect_manager->_idle_server_res_list, 
			p_connect_manager->_idle_assign_num,
			&pipe_num );
		if( ret_val !=SUCCESS) return ret_val;		
		total_pipe_num += pipe_num;

		ret_val = cm_create_pipes_from_peer_res_list( p_connect_manager, 
			&p_connect_manager->_idle_peer_res_list, 
			FALSE,
			p_connect_manager->_idle_assign_num - pipe_num,
			&pipe_num );
		if( ret_val !=SUCCESS) return ret_val;		
		total_pipe_num += pipe_num;
	}
	else
	{
		ret_val = cm_create_pipes_from_peer_res_list( p_connect_manager, 
			&p_connect_manager->_idle_peer_res_list, 
			FALSE,
			p_connect_manager->_idle_assign_num,
			&pipe_num );
		if( ret_val !=SUCCESS) return ret_val;		
		total_pipe_num += pipe_num;	
		
		ret_val = cm_create_pipes_from_server_res_list( p_connect_manager, 
			&p_connect_manager->_idle_server_res_list, 
			p_connect_manager->_idle_assign_num - pipe_num,
			&pipe_num );
		if( ret_val !=SUCCESS) return ret_val;		
		total_pipe_num += pipe_num;
	}
	LOG_DEBUG( "gcm_create_pipes_from_idle_res end, idle_assign_num:%d, created_pipe_num:%d",
		p_connect_manager->_idle_assign_num, total_pipe_num );
	return SUCCESS;

}


_int32 gcm_create_pipes_from_retry_res( struct tagCONNECT_MANAGER *p_connect_manager )
{
	_int32 ret_val = SUCCESS;
	_u32 pipe_num = 0, total_pipe_num = 0;
	LOG_DEBUG( "gcm_create_pipes_from_retry_res begin, retry_assign_num:%d", p_connect_manager->_retry_assign_num );
	
	if( p_connect_manager->_res_type_switch % 2 == 0)
	{		
		ret_val = cm_create_pipes_from_server_res_list( p_connect_manager, 
			&p_connect_manager->_retry_server_res_list, 
			p_connect_manager->_retry_assign_num,
			&pipe_num );
		if( ret_val !=SUCCESS) return ret_val;		
		total_pipe_num += pipe_num;

		ret_val = cm_create_pipes_from_peer_res_list( p_connect_manager, 
			&p_connect_manager->_retry_peer_res_list, 
			TRUE,
			p_connect_manager->_retry_assign_num - pipe_num,
			&pipe_num );
		if( ret_val !=SUCCESS) return ret_val;		
		total_pipe_num += pipe_num;
	}
	else
	{
		ret_val = cm_create_pipes_from_peer_res_list( p_connect_manager, 
			&p_connect_manager->_retry_peer_res_list,
			TRUE,
			p_connect_manager->_retry_assign_num,
			&pipe_num );
		if( ret_val !=SUCCESS) return ret_val;		
		total_pipe_num += pipe_num;	
		
		ret_val = cm_create_pipes_from_server_res_list( p_connect_manager, 
			&p_connect_manager->_retry_server_res_list, 
			p_connect_manager->_retry_assign_num - pipe_num,
			&pipe_num );
		if( ret_val !=SUCCESS) return ret_val;		
		total_pipe_num += pipe_num;
	}
	LOG_DEBUG( "gcm_create_pipes_from_retry_res end, retry_assign_num:%d, created_pipe_num:%d",
		p_connect_manager->_retry_assign_num, total_pipe_num );
	return SUCCESS;
}

_int32 gcm_create_pipes_from_candidate_res( struct tagCONNECT_MANAGER *p_connect_manager )
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "gcm_create_pipes_from_candidate_res" );

	if( p_connect_manager->_res_type_switch % 2 == 0)
	{		
		ret_val = gcm_create_pipes_from_candidate_server_res_list( p_connect_manager );
		if( ret_val !=SUCCESS) return ret_val;		

		ret_val = gcm_create_pipes_from_candidate_peer_res_list( p_connect_manager );
		if( ret_val !=SUCCESS) return ret_val;		
	}
	else
	{
		ret_val = gcm_create_pipes_from_candidate_peer_res_list( p_connect_manager );
		if( ret_val !=SUCCESS) return ret_val;		
		
		ret_val = gcm_create_pipes_from_candidate_server_res_list( p_connect_manager );
		if( ret_val !=SUCCESS) return ret_val;		
	
	}
	return SUCCESS;
	
}

_int32 gcm_create_pipes_from_candidate_server_res_list( struct tagCONNECT_MANAGER *p_connect_manager )
{
	_int32 ret_val = SUCCESS;
	RESOURCE *p_res = NULL;
	LIST_ITERATOR cur_node = LIST_BEGIN( p_connect_manager->_candidate_server_res_list._res_list );
	LIST_ITERATOR next_node = NULL;
	LOG_DEBUG( "gcm_create_pipes_from_candidate_server_res_list. _candidate_server_res_list size:%u ",
		list_size(&p_connect_manager->_candidate_server_res_list._res_list ) );
	
	while( cur_node != LIST_END( p_connect_manager->_candidate_server_res_list._res_list ) )
	{
		p_res = LIST_VALUE( cur_node );
		sd_assert( p_res->_resource_type == HTTP_RESOURCE || p_res->_resource_type == FTP_RESOURCE );
		sd_assert( p_res->_pipe_num == 0 );
		LOG_DEBUG( "gcm_create_pipes_from_candidate_server_res_list. resource:0x%x, _is_global_selected:%d", 
			p_res, p_res->_is_global_selected );
		next_node = LIST_NEXT( cur_node );

		if( !p_res->_is_global_selected ) 
		{
			cur_node = next_node;	
			continue;
		}

		ret_val = cm_create_single_server_pipe( p_connect_manager, p_res );
		if( ret_val !=SUCCESS) return ret_val;
		p_res->_is_global_selected = FALSE;

		if( ret_val == SUCCESS )
		{
			LOG_DEBUG( "gcm push resource:0x%x into _using_server_res_list, resource type = %d, resource list ptr:0x%x", p_res, p_res->_resource_type, &p_connect_manager->_using_server_res_list );
			ret_val = list_push( &(p_connect_manager->_using_server_res_list._res_list), p_res ); 
			CHECK_VALUE( ret_val );
		}
		else
		{
			LOG_ERROR( "cm_create_single_server_pipe error res:0x%x", p_res );

			sd_assert( p_res->_pipe_num == 0 );
			ret_val = list_push( &(p_connect_manager->_discard_server_res_list._res_list), p_res ); 
			CHECK_VALUE( ret_val );			
		}
		ret_val = list_erase( &p_connect_manager->_candidate_server_res_list._res_list, cur_node );		
		CHECK_VALUE( ret_val );

		cur_node = next_node;	
	}
	
	return SUCCESS;
}

_int32 gcm_create_pipes_from_candidate_peer_res_list( struct tagCONNECT_MANAGER *p_connect_manager )
{
	_int32 ret_val = SUCCESS;
	RESOURCE *p_res = NULL;
	LIST_ITERATOR cur_node = LIST_BEGIN( p_connect_manager->_candidate_peer_res_list._res_list );
	LIST_ITERATOR next_node = NULL;
	LOG_DEBUG( "gcm_create_pipes_from_candidate_peer_res_list. _candidate_peer_res_list size:%u ",
		list_size(&p_connect_manager->_candidate_peer_res_list._res_list ) );

	while( cur_node != LIST_END( p_connect_manager->_candidate_peer_res_list._res_list ) )
	{
		p_res = LIST_VALUE( cur_node );
//		sd_assert( p_res->_resource_type == PEER_RESOURCE || p_res->_resource_type == BT_RESOURCE_TYPE );
		sd_assert( p_res->_pipe_num == 0 );
		
		LOG_DEBUG( "gcm_create_pipes_from_candidate_peer_res_list. resource:0x%x, _is_global_selected:%d", 
			p_res, p_res->_is_global_selected );
		next_node = LIST_NEXT( cur_node );

		if( !p_res->_is_global_selected ) 
		{
			cur_node = next_node;	
			continue;
		}

		ret_val = cm_create_single_active_peer_pipe( p_connect_manager, p_res );
		if( ret_val != SUCCESS) return ret_val;
		p_res->_is_global_selected = FALSE;

		if( ret_val == SUCCESS )
		{
			LOG_DEBUG( "gcm push resource:0x%x into _using_peer_res_list, resource type = %d, resource list ptr:0x%x", p_res, p_res->_resource_type, &p_connect_manager->_using_peer_res_list );
			ret_val = list_push( &(p_connect_manager->_using_peer_res_list._res_list), p_res ); 
			CHECK_VALUE( ret_val );
		}
		else
		{
			sd_assert( p_res->_pipe_num == 0 );

			LOG_ERROR( "cm_create_single_active_peer_pipe error res:0x%x", p_res );

			sd_assert( p_res->_pipe_num == 0 );
			ret_val = list_push( &(p_connect_manager->_discard_peer_res_list._res_list), p_res ); 
			CHECK_VALUE( ret_val );			
		}
			
		ret_val = list_erase( &p_connect_manager->_candidate_peer_res_list._res_list, cur_node );
		CHECK_VALUE( ret_val );
		
		cur_node = next_node;	
	}
	
	return SUCCESS;
}

//pipe_wrap malloc/free wrap
_int32 init_pipe_wrap_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_pipe_wrap_slab == NULL )
	{              
		ret_val = mpool_create_slab( sizeof( PIPE_WRAP ), MIN_PIPE_WRAP, 0, &g_pipe_wrap_slab );
		CHECK_VALUE( ret_val );
	}
	return ret_val;
}

_int32 uninit_pipe_wrap_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_pipe_wrap_slab != NULL )
	{              
		ret_val = mpool_destory_slab( g_pipe_wrap_slab );
		CHECK_VALUE( ret_val );
		g_pipe_wrap_slab = NULL;
	}
	return ret_val;
}

_int32 gcm_malloc_pipe_wrap(PIPE_WRAP **pp_node)
{
 	return mpool_get_slip( g_pipe_wrap_slab, (void**)pp_node );
}

_int32 gcm_free_pipe_wrap(PIPE_WRAP *p_node)
{
	sd_assert( p_node != NULL );
	if( p_node == NULL ) return SUCCESS;
	return mpool_free_slip( g_pipe_wrap_slab, (void*)p_node );
}

//res_wrap malloc/free wrap
_int32 init_res_wrap_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_res_wrap_slab == NULL )
	{              
		ret_val = mpool_create_slab( sizeof( RES_WRAP ), MIN_RES_WRAP, 0, &g_res_wrap_slab );
		CHECK_VALUE( ret_val );
	}
	return ret_val;
}

_int32 uninit_res_wrap_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_res_wrap_slab != NULL )
	{              
		ret_val = mpool_destory_slab( g_res_wrap_slab );
		CHECK_VALUE( ret_val );
		g_res_wrap_slab = NULL;
	}
	return ret_val;
}

_int32 gcm_malloc_res_wrap(RES_WRAP **pp_node)
{
 	return mpool_get_slip( g_res_wrap_slab, (void**)pp_node );
}

_int32 gcm_free_res_wrap(RES_WRAP *p_node)
{
	sd_assert( p_node != NULL );
	if( p_node == NULL ) return SUCCESS;
	return mpool_free_slip( g_res_wrap_slab, (void*)p_node );
}
