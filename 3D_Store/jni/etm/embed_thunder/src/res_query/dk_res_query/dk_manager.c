#include "utility/define.h"
#ifdef _DK_QUERY

#include "dk_manager.h"
#include "dk_manager_interface.h"
#include "routing_table.h"
#include "dk_task.h"
#include "dht_task.h"
#include "kad_task.h"

#include "k_node.h"
#include "dk_setting.h"
#include "find_node_handler.h"


#include "asyn_frame/msg.h"

#include "utility/logid.h"
#define LOGID LOGID_DK_QUERY
#include "utility/logger.h"

#include "utility/mempool.h"
#include "utility/utility.h"


static DK_MANAGER *g_dk_manager[DK_TYPE_COUNT] = { NULL, NULL };
static dht_res_handler g_dht_res_handler = NULL;
static kad_res_handler g_kad_res_handler = NULL;

_int32 dk_init_module()
{
	_int32 ret_val = SUCCESS;
	
	LOG_DEBUG( "dk_init_module" );
	
	ret_val = dk_setting_init();
	CHECK_VALUE( ret_val );
	
	ret_val = dk_task_set_logic_func( DHT_TYPE, dht_task_logic_create, dht_task_logic_init, 
		dht_task_logic_update, dht_task_logic_uninit, dht_task_logic_destory );
	if( ret_val != SUCCESS ) goto ErrHandler;
    
#ifdef ENABLE_EMULE
	ret_val = dk_task_set_logic_func( KAD_TYPE, kad_task_logic_create, kad_task_logic_init, 
		kad_task_logic_update, kad_task_logic_uninit, kad_task_logic_destory );
#endif

	if( ret_val != SUCCESS ) goto ErrHandler;

	ret_val = k_node_module_init();
	if( ret_val != SUCCESS ) goto ErrHandler1;

	ret_val = init_find_node_handler_slabs();
	if( ret_val != SUCCESS ) goto ErrHandler2;

	ret_val = init_k_bucket_slabs();
	if( ret_val != SUCCESS ) goto ErrHandler3;	
    
	ret_val = init_dk_request_node_slabs();
	if( ret_val != SUCCESS ) goto ErrHandler4;	

	
	return SUCCESS;
ErrHandler4:
	uninit_k_bucket_slabs();
    
ErrHandler3:
	uninit_find_node_handler_slabs();			
ErrHandler2:
	k_node_module_uninit();		
ErrHandler1:
	dk_task_set_logic_func( DHT_TYPE, NULL, NULL, NULL, NULL, NULL );
	dk_task_set_logic_func( KAD_TYPE, NULL, NULL, NULL, NULL, NULL );
ErrHandler:
	dk_setting_uninit();
	return ret_val;
}

_int32 dk_uninit_module()
{
	LOG_DEBUG( "dk_uninit_module" );
    
    dk_manager_destroy( DHT_TYPE );
    dk_manager_destroy( KAD_TYPE );

    uninit_dk_request_node_slabs();
	uninit_k_bucket_slabs();
	uninit_find_node_handler_slabs();	
	k_node_module_uninit();
	
	dk_task_set_logic_func( DHT_TYPE, NULL, NULL, NULL, NULL, NULL );
	dk_task_set_logic_func( KAD_TYPE, NULL, NULL, NULL, NULL, NULL );
	
	dk_setting_uninit();
    g_dht_res_handler = NULL;
    g_kad_res_handler = NULL;
	return SUCCESS;
}

_int32 dht_register_res_handler( dht_res_handler res_handler )
{
	sd_assert( g_dht_res_handler == NULL );
	g_dht_res_handler = res_handler;
	
	LOG_DEBUG( "dht_register_res_handler:0x%x", res_handler );
	
	return SUCCESS;
}

_int32 dht_res_nofity( void *p_user, _u32 ip, _u16 port )
{
	
#ifdef _LOGGER
    char addr[24] = {0};

    sd_inet_ntoa(ip, addr, 24);
	LOG_DEBUG( "dht_res_nofity, p_user:0x%x, ip:%s, port:%u", 
        p_user, addr, port );
#endif	

	if( g_dht_res_handler == NULL )
	{
		sd_assert( FALSE );
		return SUCCESS;
	}
	
	g_dht_res_handler( p_user, ip, port );
	return SUCCESS;
}

_int32 kad_res_nofity( void *p_user, _u8 id[KAD_ID_LEN], struct tagEMULE_TAG_LIST *p_tag_list )
{

	LOG_DEBUG( "kad_res_nofity ", p_user );
    
#ifndef _DK_TEST
	if( g_kad_res_handler == NULL )
	{
		sd_assert( FALSE );
		return SUCCESS;
	}
	
	g_kad_res_handler( p_user, id, p_tag_list );
#endif    

	return SUCCESS;
}

_int32 kad_register_res_handler( kad_res_handler res_handler )
{
	sd_assert( g_kad_res_handler == NULL );
	g_kad_res_handler = res_handler;	
	
	LOG_DEBUG( "kad_register_res_handler:0x%x", res_handler );
	return SUCCESS;
}

_int32 dk_register_qeury( _u8 *p_key, _u32 key_len, void *p_user, _u64 file_size, _u32 dk_type )
{
	_int32 ret_val = SUCCESS;
	DK_TASK *p_dk_task = NULL;
#ifdef ENABLE_EMULE
    KAD_TASK *p_kad_task = NULL;
#endif
	DK_MANAGER *p_manager = NULL;
	LIST_ITERATOR cur_node = NULL;

	LOG_DEBUG( "dk_register_qeury begin, p_key:0x%x, key_len:%u, p_user:0x%x, dk_type:%u",
		p_key, key_len, p_user, dk_type );
	if( g_dk_manager[dk_type] == NULL )
	{
		ret_val = dk_manager_create( dk_type );
        if( ret_val != SUCCESS ) return SUCCESS;
	}

	p_manager = g_dk_manager[dk_type];
	sd_assert( p_manager != SUCCESS);
	
	cur_node = LIST_BEGIN( p_manager->_task_list );
	
	while( cur_node != LIST_END( p_manager->_task_list ) )
	{
		p_dk_task = (DK_TASK *)LIST_VALUE( cur_node );
		if( p_dk_task->_user_ptr == p_user )
		{
			break;
		}
		if( p_dk_task->_target_len != key_len )
		{
            sd_assert( FALSE );
			break;
		}        
		if( sd_data_cmp(p_dk_task->_target_ptr, p_key, (_int32)key_len ) )
		{
			break;
		}        
		cur_node = LIST_NEXT( cur_node );
	}	

	if( cur_node == LIST_END( p_manager->_task_list ) )
	{
		ret_val = dk_create_task( dk_type, p_key, key_len, p_user, &p_dk_task );
		CHECK_VALUE( ret_val );	
		
		ret_val = list_push( &p_manager->_task_list, p_dk_task );
		if( ret_val != SUCCESS )
		{
			dk_task_destory( p_dk_task );
		}
	}
    else
    {
        return DK_TASK_EXIST;
    }

	ret_val = dk_task_start( p_dk_task );
	CHECK_VALUE( ret_val );
    
#ifdef ENABLE_EMULE
    if( dk_type == KAD_TYPE )
    {
        p_kad_task = (KAD_TASK *)p_dk_task;
        kad_task_set_file_size( p_kad_task, file_size );
    }   
#endif
    LOG_DEBUG( "dk_register_qeury end, p_key:0x%x, key_len:%u, p_user:0x%x, dk_type:%u",
		p_key, key_len, p_user, dk_type );

	return SUCCESS;
}

_int32 dk_unregister_qeury( void *p_user, _u32 dk_type )
{
	DK_MANAGER *p_manager = NULL;
	DK_TASK *p_dk_task = NULL;
	LIST_ITERATOR cur_node = NULL;
	
	p_manager = g_dk_manager[dk_type];
	if( p_manager == NULL) return SUCCESS;
	
	LOG_DEBUG( "dk_unregister_qeury: p_user:0x%x, dk_type:%u", p_user, dk_type );
	
	cur_node = LIST_BEGIN( p_manager->_task_list );
	while( cur_node != LIST_END( p_manager->_task_list ) )
	{
		p_dk_task = (DK_TASK *)LIST_VALUE( cur_node );
		if( p_dk_task->_user_ptr == p_user )
		{
			dk_task_destory( p_dk_task );
			list_erase( &p_manager->_task_list, cur_node );
			return SUCCESS;
		}
		cur_node = LIST_NEXT( cur_node );
	}	
	//sd_assert( FALSE );
	return SUCCESS;

}

_int32 dht_add_routing_nodes( void *p_user, LIST *p_node_list )
{
	DK_MANAGER *p_manager = NULL;
	DHT_TASK *p_dht_task = NULL;
	LIST_ITERATOR cur_node = NULL;
    K_PEER_ADDR *p_node = NULL;
	
	p_manager = g_dk_manager[DHT_TYPE];
    if( p_manager == NULL ) return SUCCESS;
	
	LOG_DEBUG( "dht_add_routing_nodes: p_user:0x%x", p_user );
	
	cur_node = LIST_BEGIN( p_manager->_task_list );
	while( cur_node != LIST_END( p_manager->_task_list ) )
	{
		p_dht_task = (DHT_TASK *)LIST_VALUE( cur_node );
		if( p_dht_task->_dk_task._user_ptr == p_user )
		{
            break;
		}
		cur_node = LIST_NEXT( cur_node );
	}	
    
	if( cur_node == LIST_END( p_manager->_task_list ) )
	{
        sd_assert( FALSE );
        return SUCCESS;
	}
    
	cur_node = LIST_BEGIN( *p_node_list );
	while( cur_node != LIST_END( *p_node_list ) )
	{
        p_node = ( K_PEER_ADDR *)LIST_VALUE( cur_node );
        rt_ping_node( DHT_TYPE, p_node->_ip, p_node->_port, 0, TRUE );

        dht_add_node( p_dht_task, p_node->_ip, p_node->_port );

		cur_node = LIST_NEXT( cur_node );
	}	    

	return SUCCESS;
}

_int32 kad_add_routing_node( _u32 ip, _u16 kad_port, _u8 version )
{
    return rt_ping_node( KAD_TYPE, ip, kad_port, version, TRUE );
}

_int32 dk_manager_create( _u32 dk_type )
{
	_int32 ret_val = SUCCESS;
	DK_MANAGER *p_manager = NULL;
	
	LOG_DEBUG( "dk_manager_create, dk_type:%u", dk_type );
	
	if( NULL != g_dk_manager[ dk_type ]
		|| dk_type >= DK_TYPE_COUNT )
	{
		sd_assert( FALSE );
		return -1;
	}	
	
	ret_val = sd_malloc( sizeof( DK_MANAGER ), (void **)&p_manager );
	CHECK_VALUE( ret_val );

	ret_val = sh_create( dk_type );
	if( ret_val != SUCCESS ) goto ErrHandler;
	
	ret_val = dk_routing_table_create( dk_type );
	if( ret_val != SUCCESS ) goto ErrHandler1;

	list_init( &p_manager->_task_list );
	g_dk_manager[ dk_type ] = p_manager;
	
	ret_val = start_timer( dk_manager_time_out, NOTICE_INFINITE, dk_time_out_interval(), 0, p_manager, 
        &p_manager->_time_out_id );
	if( ret_val != SUCCESS ) goto ErrHandler2;
	
	p_manager->_dk_type = dk_type;
	p_manager->_idle_ticks = 0;
    
	return SUCCESS;
	
ErrHandler2:
	dk_routing_table_destory( dk_type );
ErrHandler1:
	sh_try_destory( dk_type );	
ErrHandler:
	SAFE_DELETE( p_manager );
	return ret_val;
}

_int32 dk_manager_destroy( _u32 dk_type )
{
	DK_TASK *p_dk_task = NULL;
	LIST_ITERATOR cur_node = NULL;
	DK_MANAGER *p_manager = NULL;
	
	LOG_DEBUG( "dk_manager_destroy, dk_type:%u", dk_type );
	p_manager = g_dk_manager[ dk_type ];

    if( p_manager == NULL ) return SUCCESS;

	dk_routing_table_destory( dk_type );
	sh_try_destory( dk_type );

	cur_node = LIST_BEGIN( p_manager->_task_list );
	while( cur_node != LIST_END( p_manager->_task_list ) )
	{
		p_dk_task = (DK_TASK *)LIST_VALUE( cur_node );
		dk_task_destory( p_dk_task );
		cur_node = LIST_NEXT( cur_node );
	}
    
    if( p_manager->_time_out_id != INVALID_MSG_ID )
    {
        cancel_timer( p_manager->_time_out_id );
	    p_manager->_time_out_id = INVALID_MSG_ID;
    }

    SAFE_DELETE( g_dk_manager[ dk_type ] );
	return SUCCESS;	
}

_int32 dk_manager_time_out(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid )
{
	DK_MANAGER *p_dk_manager = (DK_MANAGER *)msg_info->_user_data;
	DK_TASK *p_dk_task = NULL;
	LIST_ITERATOR cur_node = NULL;
	
	LOG_DEBUG( "dk_manager_time_out, errcode:%u", errcode );

	if( errcode == MSG_CANCELLED )
	{
	      LOG_DEBUG("dk_manager_time_out, MSG_CANCELLED" );  
	      return SUCCESS;
	}

    if( p_dk_manager->_idle_ticks == 0 )//没有任务不需要更新
    {
        rt_update( rt_ptr(p_dk_manager->_dk_type) );
    }
	
	cur_node = LIST_BEGIN( p_dk_manager->_task_list );
	while( cur_node != LIST_END( p_dk_manager->_task_list ) )
	{
		p_dk_task = (DK_TASK *)LIST_VALUE( cur_node );
		dk_task_timeout( p_dk_task );
		cur_node = LIST_NEXT( cur_node );
	}
    if( cur_node == LIST_BEGIN( p_dk_manager->_task_list ) )
    {
        p_dk_manager->_idle_ticks++;
    }
    else
    {
        p_dk_manager->_idle_ticks = 0;
    }
    
	LOG_DEBUG( "dk_manager_time_out, manager_ptr:0x%x, type:%d, _idle_ticks:%u, task_list_size:%u, ", 
        p_dk_manager, p_dk_manager->_dk_type, p_dk_manager->_idle_ticks, list_size( &p_dk_manager->_task_list) );
    
    if( p_dk_manager->_idle_ticks > dk_manager_idle_count() )
    {
        dk_manager_destroy( p_dk_manager->_dk_type );
    }
	
	return SUCCESS;	
}

#endif

