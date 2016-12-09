#include "utility/logid.h"
#define LOGID LOGID_CONNECT_MANAGER
#include "utility/logger.h"
#include "utility/mempool.h"
#include "utility/list.h"
#include "utility/settings.h"

#include "global_connect_manager.h"

#include "connect_manager.h"
#include "connect_filter_manager.h"
#include "connect_manager_imp.h"
#include "connect_manager_setting.h"
#include "connect_manager_interface.h"
#include "global_connect_dispatch.h"

static GLOBAL_CONNECT_MANAGER *g_global_connect_manager = NULL;
static BOOL g_pause_global_pipes = FALSE;
static BOOL g_pause_all_global_pipes = FALSE;

_int32 gcm_init_struct(void)
{
	_int32 ret_val;

    if (NULL == g_global_connect_manager)
    {
        ret_val = sd_malloc(sizeof(GLOBAL_CONNECT_MANAGER), (void **)&g_global_connect_manager);
        CHECK_VALUE( ret_val );
    }
	LOG_DEBUG("gcm_init_struct, gcm ptr:0x%x", g_global_connect_manager);

	g_pause_global_pipes = FALSE;
	g_pause_all_global_pipes = FALSE;
		
	list_init(&g_global_connect_manager->_connect_manager_ptr_list);
	list_init(&g_global_connect_manager->_using_pipe_list);
	list_init(&g_global_connect_manager->_candidate_res_list);
	
	g_global_connect_manager->_global_max_pipe_num = cm_global_max_pipe_num();
	g_global_connect_manager->_global_test_speed_pipe_num = cm_global_test_speed_pipe_num();
	g_global_connect_manager->_global_max_filter_pipe_num = cm_global_max_filter_pipe_num();
    
	g_global_connect_manager->_global_cur_pipe_num = 0;
	g_global_connect_manager->_global_max_connecting_pipe_num = cm_global_max_connecting_pipe_num();

	g_global_connect_manager->_assign_max_pipe_num = 0;
	g_global_connect_manager->_is_use_global_strategy = cm_is_use_global_strategy();
	g_global_connect_manager->_global_dispatch_num = 0;	
	g_global_connect_manager->_global_switch_num = 0;	
	g_global_connect_manager->_global_start_ticks = 0;	
	g_global_connect_manager->_total_res_num = 0;	

	cm_init_filter_manager( &g_global_connect_manager->_filter_manager );
	ret_val = init_pipe_wrap_slabs();
	CHECK_VALUE( ret_val );
	
	ret_val = init_res_wrap_slabs();
	CHECK_VALUE( ret_val );
	return SUCCESS;
}

_int32 gcm_uninit_struct(void)
{
	_int32 ret_val = SUCCESS;
	LIST_ITERATOR cur_node, next_node;
	sd_assert( list_size(&g_global_connect_manager->_connect_manager_ptr_list ) == 0 );

	LOG_DEBUG( "gcm_uninit_struct" );
	cur_node = LIST_BEGIN( g_global_connect_manager->_using_pipe_list );
	while( cur_node != LIST_END( g_global_connect_manager->_using_pipe_list ) )
	{
		PIPE_WRAP *p_pipe_wrap = LIST_VALUE( cur_node );
		ret_val = gcm_free_pipe_wrap( p_pipe_wrap );
		CHECK_VALUE( ret_val );
		next_node = LIST_NEXT( cur_node );

		list_erase( &g_global_connect_manager->_using_pipe_list, cur_node );
		cur_node = next_node;
	}


	cur_node = LIST_BEGIN( g_global_connect_manager->_candidate_res_list );
	while( cur_node != LIST_END( g_global_connect_manager->_candidate_res_list ) )
	{
		RES_WRAP *p_res_wrap = LIST_VALUE( cur_node );
		ret_val = gcm_free_res_wrap( p_res_wrap );
		CHECK_VALUE( ret_val );
		next_node = LIST_NEXT( cur_node );

		list_erase( &g_global_connect_manager->_candidate_res_list, cur_node );
		cur_node = next_node;
	}

	ret_val = uninit_pipe_wrap_slabs();
	CHECK_VALUE( ret_val );
	
	ret_val = uninit_res_wrap_slabs();
	CHECK_VALUE( ret_val );
	
	SAFE_DELETE( g_global_connect_manager );
	return SUCCESS;	
}

_int32 gcm_set_global_strategy( BOOL is_using )
{
	LOG_DEBUG( "gcm_set_global_strategy:%d", is_using );
	g_global_connect_manager->_is_use_global_strategy = is_using;
	return SUCCESS;
}

BOOL gcm_is_global_strategy(void)
{
	LOG_DEBUG( "gcm_is_global_strategy:%d", g_global_connect_manager->_is_use_global_strategy );

	return g_global_connect_manager->_is_use_global_strategy;
}

_int32 gcm_set_max_pipe_num( _u32 max_pipe_num )
{
	LOG_DEBUG( "gcm_set_max_pipe_num:%u", max_pipe_num );

	g_global_connect_manager->_global_max_pipe_num = max_pipe_num;
	settings_set_int_item( "connect_manager.global_max_pipe_num", (_int32)max_pipe_num );
	
	if( max_pipe_num <= g_global_connect_manager->_global_test_speed_pipe_num )
	{
		g_global_connect_manager->_global_test_speed_pipe_num = max_pipe_num / 3;
	}
	
	g_global_connect_manager->_global_max_filter_pipe_num = max_pipe_num / 3;
	//g_global_connect_manager->_global_pipe_low_limit = cm_global_max_pipe_num()/3;
    
	return SUCCESS;
}

_u32 gcm_get_mak_pipe_num(void)
{
	return g_global_connect_manager->_global_max_pipe_num;
}

_int32 gcm_set_max_connecting_pipe_num( _u32 max_connecting_pipe_num )
{
	LOG_DEBUG( "gcm_set_max_pipe_num:%u", max_connecting_pipe_num );

	g_global_connect_manager->_global_max_connecting_pipe_num = max_connecting_pipe_num;
	return SUCCESS;
}
#ifdef _DEBUG
void gcm_testing(void)
{
	//_int32 ret_val;
	LIST *pipes_list=&(g_global_connect_manager->_using_pipe_list);
	LIST_ITERATOR cur_node, end_node;
	PIPE_WRAP *p_pipe_wrap = NULL;
	DATA_PIPE *p_order_pipe = NULL;
	cur_node = LIST_BEGIN( *pipes_list );
	end_node = LIST_END( *pipes_list );	
	while( cur_node != end_node )
	{
		p_pipe_wrap = LIST_VALUE( cur_node );
		if( TRUE==p_pipe_wrap->_is_deleted_by_pipe )
		{
			cur_node = LIST_NEXT( cur_node );
			continue;
		}
		p_order_pipe = p_pipe_wrap->_data_pipe_ptr;
		sd_assert( p_pipe_wrap == p_order_pipe->_dispatch_info._global_wrap_ptr );
		
		LOG_DEBUG( "gcm_testing_1._using_pipe_list speed:%u,_old_speed=%u, score:%u,_old_score=%u, max_speed:%u, pipe_ptr:0x%x, pipe_type:%d, resource_ptr:0x%x.pipe_state:%u, wrap_ptr:0x%x", 
			p_order_pipe->_dispatch_info._speed,p_order_pipe->_dispatch_info._old_speed, p_order_pipe->_dispatch_info._score, p_order_pipe->_dispatch_info._old_score, 
			p_order_pipe->_dispatch_info._max_speed, p_order_pipe,
			p_order_pipe->_data_pipe_type, p_order_pipe->_p_resource,
			p_order_pipe->_dispatch_info._pipe_state, p_pipe_wrap );

// //1. 某个pipe被choke，但是score却在增大
//pipe状态在一个调度周期内发生变化,比如2s内收到unchoke和choke,状态从choke到unchoke再到choke,
//调度没来的及更新,导致界面看到一直被choke但是score增加

		if(p_order_pipe->_dispatch_info._pipe_state==PIPE_CHOKED)
		{
			sd_assert(p_order_pipe->_dispatch_info._speed<=p_order_pipe->_dispatch_info._old_speed);
/*			if((p_order_pipe->_dispatch_info._old_score!=0)&&(p_order_pipe->_dispatch_info._old_state==PIPE_CHOKED))
			{
				sd_assert(p_order_pipe->_dispatch_info._score<=p_order_pipe->_dispatch_info._old_score);
			} 
*/	
		}

		//确定downloading状态下pipe的score和speed是一样的
		if(p_order_pipe->_dispatch_info._pipe_state==PIPE_DOWNLOADING)
		{
			sd_assert(p_order_pipe->_dispatch_info._speed == p_order_pipe->_dispatch_info._score);
		}

		//pipe在downloading下,1s后速度下降150k会导致速度波动很大
/*
		if(p_order_pipe->_dispatch_info._pipe_state==PIPE_DOWNLOADING
			&& p_order_pipe->_dispatch_info._old_state==PIPE_DOWNLOADING
			&& p_order_pipe->_dispatch_info._old_speed > p_order_pipe->_dispatch_info._speed)
		{
			sd_assert( p_order_pipe->_dispatch_info._old_speed < p_order_pipe->_dispatch_info._speed + 150 * 1024 );
		}
*/
		p_order_pipe->_dispatch_info._old_state = p_order_pipe->_dispatch_info._pipe_state;
		p_order_pipe->_dispatch_info._old_speed=p_order_pipe->_dispatch_info._speed;
		p_order_pipe->_dispatch_info._old_score=p_order_pipe->_dispatch_info._score;

		
// //2. 某个pipe速度>0，但score却 == 0
		if(p_order_pipe->_dispatch_info._speed>0)
		{
			sd_assert(p_order_pipe->_dispatch_info._score!=0);
		}
		
// //3.切换任务时 导致一个任务速度下降 
		//sd_assert( !p_pipe_wrap->_is_deleted_by_pipe );
		
		cur_node = LIST_NEXT( cur_node );
	}
}
#endif

_int32 gcm_do_connect_dispatch(void)
{
    LIST_ITERATOR cur_node = NULL;
    CONNECT_MANAGER *p_connect_manager = NULL;
    _int32 ret_val;

    cur_node = LIST_BEGIN( gcm_get_ptr()->_connect_manager_ptr_list );

    LOG_DEBUG( "gcm_do_connect_dispatch enter ... " );

    while ( cur_node != LIST_END( gcm_get_ptr()->_connect_manager_ptr_list  ) )
    {
        p_connect_manager = LIST_VALUE( cur_node );

        ret_val = cm_create_pipes( p_connect_manager );
        if( ret_val != SUCCESS)
        {
            return ret_val;
        }

        cur_node = LIST_NEXT( cur_node );
    }
    LOG_DEBUG( "gcm_do_connect_dispatch exit ... " );
    return SUCCESS;
}

_u32 gcm_get_all_pipes_num()
{
	LIST_ITERATOR cur_node = NULL;
	CONNECT_MANAGER *p_connect_manager = NULL;
	_u32 inuse_pipes_count = 0;
	
	cur_node = LIST_BEGIN( gcm_get_ptr()->_connect_manager_ptr_list );
	
	LOG_DEBUG( "gcm_get_all_pipes_num enter ... " );

	while( cur_node != LIST_END( gcm_get_ptr()->_connect_manager_ptr_list  ) )
	{
		p_connect_manager = LIST_VALUE( cur_node );

		inuse_pipes_count += cm_get_connecting_server_pipe_num( p_connect_manager );
		inuse_pipes_count += cm_get_working_server_pipe_num(p_connect_manager);
		inuse_pipes_count += cm_get_connecting_peer_pipe_num(p_connect_manager);
		inuse_pipes_count += cm_get_working_peer_pipe_num(p_connect_manager);
		cur_node = LIST_NEXT( cur_node );
	}
	LOG_DEBUG( "gcm_get_all_pipes_num exit ..., all pipes count : %d", inuse_pipes_count);
	return inuse_pipes_count;
}

_int32 gcm_register_connect_manager( struct tagCONNECT_MANAGER *p_connect_manager )
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "gcm_register_connect_manager, connect_ptr:0x%x", p_connect_manager );

	p_connect_manager->_cm_level = INIT_CM_LEVEL;
	ret_val = list_push( &g_global_connect_manager->_connect_manager_ptr_list, (void *)p_connect_manager );
	CHECK_VALUE( ret_val );
	
	ret_val = gcm_calc_global_connect_level();
	CHECK_VALUE( ret_val );	
	return SUCCESS;
}

_int32 gcm_unregister_connect_manager( struct tagCONNECT_MANAGER *p_connect_manager )
{
	_int32 ret_val;
	LIST_ITERATOR cur_node = LIST_BEGIN( g_global_connect_manager->_connect_manager_ptr_list );
	LOG_DEBUG( "gcm_unregister_connect_manager, connect_ptr:0x%x", p_connect_manager );

	while( cur_node != LIST_END( g_global_connect_manager->_connect_manager_ptr_list ) )
	{
		struct tagCONNECT_MANAGER *p_target = LIST_VALUE( cur_node );
		if( p_target == p_connect_manager )
		{
			ret_val = list_erase( &g_global_connect_manager->_connect_manager_ptr_list, cur_node );
			CHECK_VALUE( ret_val );
			return SUCCESS;;
		}
		cur_node = LIST_NEXT( cur_node );
	}
	sd_assert( FALSE );
	return SUCCESS;
}

/* 销毁所有pipe */
_int32 pause_all_global_pipes(void)
{
	//_int32 ret_val;
	#if 0
	LIST_ITERATOR cur_node = LIST_BEGIN( g_global_connect_manager->_connect_manager_ptr_list );
	LOG_URGENT( "pause_all_global_pipes, connect_manager_size:%d", list_size(&g_global_connect_manager->_connect_manager_ptr_list) );

	if(g_pause_all_global_pipes) return SUCCESS;
	
	while( cur_node != LIST_END( g_global_connect_manager->_connect_manager_ptr_list ) )
	{
		struct tagCONNECT_MANAGER *p_target = LIST_VALUE( cur_node );
		print_cm_info(p_target);
		cm_destroy_all_pipes(p_target);
		
		/* 把所有正在使用的资源都移到候选资源列表中,等到下一次调度再使用 */
		cm_move_using_res_to_candidate_except_res(p_target,NULL);
		
		cur_node = LIST_NEXT( cur_node );
	}
	g_pause_all_global_pipes = TRUE;
	#endif
	return SUCCESS;
}
_int32 resume_all_global_pipes(void)
{
#if 0
	//_int32 ret_val;
	LIST_ITERATOR cur_node = LIST_BEGIN( g_global_connect_manager->_connect_manager_ptr_list );
	LOG_URGENT( "resume_all_global_pipes, connect_manager_size:%d", list_size(&g_global_connect_manager->_connect_manager_ptr_list) );

	if(!g_pause_all_global_pipes&&!g_pause_global_pipes) return SUCCESS;

	g_pause_global_pipes = FALSE;
	g_pause_all_global_pipes = FALSE;
	
	while( cur_node != LIST_END( g_global_connect_manager->_connect_manager_ptr_list ) )
	{
		struct tagCONNECT_MANAGER *p_target = LIST_VALUE( cur_node );
		cm_create_pipes(p_target);
		cur_node = LIST_NEXT( cur_node );
	}
#endif	
	return SUCCESS;
}

/* 销毁除了局域网http pipe 外的所有pipe */
_int32 pause_global_pipes(void)
{
	//_int32 ret_val;
	LIST_ITERATOR cur_node = LIST_BEGIN( g_global_connect_manager->_connect_manager_ptr_list );
	LOG_ERROR( "pause_global_pipes, list_size:%d", list_size(&g_global_connect_manager->_connect_manager_ptr_list) );

	if(g_pause_global_pipes||g_pause_all_global_pipes) return SUCCESS;
	
	while( cur_node != LIST_END( g_global_connect_manager->_connect_manager_ptr_list ) )
	{
		struct tagCONNECT_MANAGER *p_target = LIST_VALUE( cur_node );
		cm_destroy_all_pipes_except_lan(p_target);
		cur_node = LIST_NEXT( cur_node );
	}
	g_pause_global_pipes = TRUE;
	return SUCCESS;
}

_int32 resume_global_pipes(void)
{
	//_int32 ret_val;
	LIST_ITERATOR cur_node = LIST_BEGIN( g_global_connect_manager->_connect_manager_ptr_list );
	LOG_ERROR( "resume_global_pipes, list_size:%d", list_size(&g_global_connect_manager->_connect_manager_ptr_list) );

	if(g_pause_all_global_pipes)
	{
		/* 在所有pipe被暂停的情况下不能用这个接口续传pipe，得用resume_all_global_pipes */
		return -1;
	}
	
	if(!g_pause_global_pipes) return SUCCESS;
	
	g_pause_global_pipes = FALSE;
	
	while( cur_node != LIST_END( g_global_connect_manager->_connect_manager_ptr_list ) )
	{
		struct tagCONNECT_MANAGER *p_target = LIST_VALUE( cur_node );
		cm_create_pipes(p_target);
		cur_node = LIST_NEXT( cur_node );
	}
	
	return SUCCESS;
}
BOOL is_pause_global_pipes(void)
{
	if( g_pause_global_pipes||g_pause_all_global_pipes)
		return TRUE;
	else
		return FALSE;
}
_int32 gcm_register_pipe( struct tagCONNECT_MANAGER *p_connect_manager, DATA_PIPE *p_data_pipe )
{
	LOG_DEBUG( "gcm_register_pipe begin , connect_manager_ptr:0x%x, pipe_ptr:0x%x, cm_level:%d", 
		p_connect_manager, p_data_pipe, p_connect_manager->_cm_level );

	if( !g_global_connect_manager->_is_use_global_strategy 
		|| p_connect_manager->_cm_level == INIT_CM_LEVEL )
	{
		return SUCCESS;
	}
	g_global_connect_manager->_global_cur_pipe_num++;
    
	LOG_DEBUG( "gcm_register_pipe end, connect_manager_ptr:0x%x, pipe_ptr:0x%x, _global_cur_pipe_num:%d, connect_cur_pipe_num:%d", 
		p_connect_manager, p_data_pipe, g_global_connect_manager->_global_cur_pipe_num, p_connect_manager->_cur_pipe_num );
	//sd_assert( g_global_connect_manager->_global_cur_pipe_num <= g_global_connect_manager->_global_max_pipe_num );

	return SUCCESS;
}

_int32 gcm_unregister_pipe( struct tagCONNECT_MANAGER *p_connect_manager, DATA_PIPE *p_data_pipe )
{
	LOG_DEBUG( "gcm_unregister_pipe , connect_manager_ptr:0x%x, pipe_ptr:0x%x, cm_level:%d, connect_cur_pipe_num:%d", 
		p_connect_manager, p_data_pipe, p_connect_manager->_cm_level, p_connect_manager->_cur_pipe_num  );
	if( !g_global_connect_manager->_is_use_global_strategy 
		|| p_connect_manager->_cm_level == INIT_CM_LEVEL )
	{
		return SUCCESS;
	}
    sd_assert( g_global_connect_manager->_global_cur_pipe_num > 0 );
	if( g_global_connect_manager->_global_cur_pipe_num == 0 ) return SUCCESS;
	g_global_connect_manager->_global_cur_pipe_num--;	

    
	LOG_DEBUG( "gcm_unregister_pipe end, connect_manager_ptr:0x%x, pipe_ptr:0x%x, _global_cur_pipe_num:%d", 
		p_connect_manager, p_data_pipe, g_global_connect_manager->_global_cur_pipe_num );

	return SUCCESS;
}

_int32 gcm_register_working_pipe( struct tagCONNECT_MANAGER *p_connect_manager, DATA_PIPE *p_data_pipe )
{
	_int32 ret_val;
	PIPE_WRAP *p_pipe_wrap = NULL;
	LOG_DEBUG( "gcm_register_working_pipe begin, connect_manager_ptr:0x%x, pipe_ptr:0x%x, cm_level:%d", 
		p_connect_manager, p_data_pipe, p_connect_manager->_cm_level );

	if( !g_global_connect_manager->_is_use_global_strategy 
		|| p_connect_manager->_cm_level == INIT_CM_LEVEL )
	{
		return SUCCESS;
	}
	if( p_data_pipe->_dispatch_info._global_wrap_ptr != NULL ) return SUCCESS;
	
	ret_val = gcm_malloc_pipe_wrap( &p_pipe_wrap );
	CHECK_VALUE( ret_val );

	p_pipe_wrap->_data_pipe_ptr = p_data_pipe;
	p_pipe_wrap->_is_deleted_by_pipe = FALSE;
	p_data_pipe->_dispatch_info._global_wrap_ptr = (void *)p_pipe_wrap;
	
    LOG_DEBUG( "gcm_register_working_pipe end ,pipe_ptr:0x%x , pipe_wrap_ptr:0x%x, _global_cur_pipe_num:%u",
		p_data_pipe, p_pipe_wrap, g_global_connect_manager->_global_cur_pipe_num );

	ret_val = list_push( &g_global_connect_manager->_using_pipe_list, p_pipe_wrap );
	CHECK_VALUE( ret_val );
	return SUCCESS;
}

_int32 gcm_register_candidate_res( struct tagCONNECT_MANAGER *p_connect_manager, RESOURCE *p_res )
{
	_int32 ret_val;
	RES_WRAP *p_res_wrap = NULL;

	LOG_DEBUG( "gcm_register_candidate_res, res_ptr:0x%x", p_res );

	if( !g_global_connect_manager->_is_use_global_strategy 
	|| p_connect_manager->_cm_level == INIT_CM_LEVEL )
	{
		return SUCCESS;
	}
	
	ret_val = gcm_malloc_res_wrap( &p_res_wrap );
	CHECK_VALUE( ret_val );

	p_res_wrap->_res_ptr = p_res;
	p_res_wrap->_is_deleted = FALSE;
	p_res->_global_wrap_ptr = (void *)p_res_wrap;
	ret_val = list_push( &g_global_connect_manager->_candidate_res_list, p_res_wrap );
	CHECK_VALUE( ret_val );
	return SUCCESS;
}

BOOL gcm_is_need_more_res(void)
{
	BOOL is_need = TRUE;
	is_need = ( g_global_connect_manager->_total_res_num <  cm_max_res_num() );
    LOG_DEBUG( "gcm_is_need_more_res:%d, total_res_num:%d, max_res_num:%d", 
		is_need, g_global_connect_manager->_total_res_num, cm_max_res_num() );

	return is_need;
}

void gcm_add_res_num(void)
{
	g_global_connect_manager->_total_res_num++;
}
void gcm_sub_res_num(void)
{
	sd_assert( g_global_connect_manager->_total_res_num > 0 );
	g_global_connect_manager->_total_res_num--;
}

_u32 gcm_get_res_num(void)
{
	return g_global_connect_manager->_total_res_num;
}

_u32 gcm_get_pipe_num(void)
{
	_u32 pipe_num = 0;
	struct tagCONNECT_MANAGER *p_connect_manager = NULL;
	LIST_ITERATOR cur_node = LIST_BEGIN( g_global_connect_manager->_connect_manager_ptr_list );

	while( cur_node != LIST_END( g_global_connect_manager->_connect_manager_ptr_list ) )
	{
		p_connect_manager = LIST_VALUE( cur_node );
		pipe_num += p_connect_manager->_cur_pipe_num;

		cur_node = LIST_NEXT( cur_node );
	}
	LOG_DEBUG( "gcm_get_pipe_num, pipe_num:%u", pipe_num );

	return pipe_num;
}

BOOL gcm_is_pipe_full(void)
{
	return g_global_connect_manager->_global_cur_pipe_num >= g_global_connect_manager->_global_max_pipe_num;
}

struct tagGLOBAL_CONNECT_MANAGER* gcm_get_ptr(void)
{
	return g_global_connect_manager;
}

