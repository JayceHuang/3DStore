#include "utility/list.h"
#include "utility/url.h"
#include "utility/utility.h"
#include "utility/logid.h"
#define LOGID LOGID_CONNECT_MANAGER
#include "utility/logger.h"
#include "utility/define.h"
#include "utility/local_ip.h"
#include "utility/peer_capability.h"


#include "connect_manager_imp.h"

#include "data_manager/data_manager_interface.h"
#include "http_data_pipe/http_data_pipe_interface.h"
#include "ftp_data_pipe/ftp_data_pipe_interface.h"
#include "p2p_data_pipe/p2p_pipe_interface.h"
#include "p2p_data_pipe/p2p_data_pipe.h"

#include "global_connect_manager.h"
#include "global_connect_dispatch.h"
#include "p2p_transfer_layer/ptl_interface.h"
#include "platform/sd_network.h"

#include "download_task/download_task.h"

#ifdef ENABLE_BT
#include "bt_download/bt_utility/bt_utility.h"
#include "bt_download/bt_data_pipe/bt_pipe_interface.h"
#endif /* #ifdef ENABLE_BT  */

#ifdef ENABLE_EMULE
#include "emule/emule_interface.h"
#endif

static _int32 cm_destroy_pipe_list_owned_by_res(CONNECT_MANAGER *p_connect_manager
        , PIPE_LIST *p_pipe_list
        , RESOURCE *p_res 
        , BOOL is_connecting_list);
        
static _int32 cm_destroy_server_pipes_owned_by_res(CONNECT_MANAGER *p_connect_manager
            , RESOURCE *p_res );
	
_int32 cm_init_struct(CONNECT_MANAGER* p_connect_manager
    , void *p_data_manager, can_destory_resource_call_back p_call_back )
{
	_int32 ret_val = SUCCESS;
	
	LOG_DEBUG( "cm_init_struct.ptr:0x%x.", p_connect_manager );
	
	p_connect_manager->_cur_res_num = 0;	
	p_connect_manager->_cur_pipe_num = 0;
	p_connect_manager->_max_pipe_num = 0;
	p_connect_manager->_is_only_using_origin_server = TRUE;//cm_is_only_using_origin_server();
	p_connect_manager->_is_idle_status = FALSE;
	p_connect_manager->_origin_res_ptr = NULL;
	p_connect_manager->_status_ticks = 0;
	p_connect_manager->_res_type_switch = 0;
	p_connect_manager->_data_manager_ptr = p_data_manager;
 	p_connect_manager->_is_need_new_server_res = TRUE;
	p_connect_manager->_is_need_new_peer_res = TRUE;	
	p_connect_manager->_is_fast_speed = FALSE;
	p_connect_manager->_cur_peer_speed = 0;
	p_connect_manager->_cur_peer_upload_speed = 0;
	p_connect_manager->_cur_server_speed = 0;
	p_connect_manager->_max_server_speed = 0;
	p_connect_manager->_max_peer_speed = 0;
	p_connect_manager->_clear_server_hash_map_ticks = 0;
	p_connect_manager->_clear_peer_hash_map_ticks = 0;
	p_connect_manager->_idle_assign_num = 0;
	p_connect_manager->_retry_assign_num = 0;	
	p_connect_manager->_is_err_get_buffer_status = FALSE;	
	p_connect_manager->_call_back_ptr = p_call_back;	

	ret_val = sd_memset(&p_connect_manager->_pipe_interface, 0, sizeof(PIPE_INTERFACE));
	sd_assert( ret_val == SUCCESS );
		
#ifdef _CONNECT_DETAIL
	p_connect_manager->_server_destroy_num = 0;
	p_connect_manager->_peer_destroy_num = 0;
	sd_memset(&p_connect_manager->_peer_pipe_info, 0, sizeof(p_connect_manager->_peer_pipe_info));
	sd_memset(&p_connect_manager->_server_pipe_info, 0, sizeof( p_connect_manager->_server_pipe_info));
#endif   
	
	ret_val = sd_time_ms(&p_connect_manager->_start_time);
	CHECK_VALUE( ret_val ); 
	
	list_init( &(p_connect_manager->_idle_server_res_list._res_list) );
	list_init( &(p_connect_manager->_idle_peer_res_list._res_list) );	
	
	list_init( &(p_connect_manager->_using_server_res_list._res_list) );
	list_init( &(p_connect_manager->_using_peer_res_list._res_list) );	

	list_init( &(p_connect_manager->_candidate_server_res_list._res_list) );	
	list_init( &(p_connect_manager->_candidate_peer_res_list._res_list) );	

	list_init( &(p_connect_manager->_retry_server_res_list._res_list) );	
	list_init( &(p_connect_manager->_retry_peer_res_list._res_list) );	
	
	list_init( &(p_connect_manager->_discard_server_res_list._res_list) );	
	list_init( &(p_connect_manager->_discard_peer_res_list._res_list) );	

	
	list_init( &(p_connect_manager->_connecting_server_pipe_list._data_pipe_list) );	
	list_init( &(p_connect_manager->_connecting_peer_pipe_list._data_pipe_list) );	
	
	list_init( &(p_connect_manager->_working_server_pipe_list._data_pipe_list) );
	list_init( &(p_connect_manager->_working_peer_pipe_list._data_pipe_list) );

	cm_init_filter_manager( &p_connect_manager->_connect_filter_manager );
	map_init( &p_connect_manager->_server_hash_value_map, cm_value_compare );
	map_init( &p_connect_manager->_peer_hash_value_map, cm_value_compare );

	map_init( &p_connect_manager->_destoryed_server_res_hash_value_map, cm_value_compare );
	map_init( &p_connect_manager->_destoryed_peer_res_hash_value_map, cm_value_compare );

	map_init( &p_connect_manager->_sub_connect_manager_map, cm_value_compare );

#ifdef ENABLE_CDN
	list_init( &(p_connect_manager->_cdn_res_list._res_list) );
	list_init( &(p_connect_manager->_cdn_pipe_list._data_pipe_list) );	
	#ifndef MOBILE_PHONE
	/* MAC 迅雷默认打开CDN做测试 */
	p_connect_manager->_cdn_mode=TRUE;
	#else
	p_connect_manager->_cdn_mode=FALSE;
	#endif /* defined(MACOS) && !defined(MOBILE_PHONE) */
	p_connect_manager->_max_cdn_speed=0;
	p_connect_manager->_cdn_ok=FALSE;
#endif
	p_connect_manager->_pause_pipes = FALSE;
#ifdef EMBED_REPORT
	sd_memset( &p_connect_manager->_report_para, 0, sizeof(CM_REPORT_PARA) );
	range_list_init(&(p_connect_manager->_report_para._res_normal_cdn_use_time_list));
#endif

#if defined(ASSISTANT_PROJ) || defined(ENABLE_ETM_MEDIA_CENTER)
	/* 手机助手项目默认打开高速通道 */
	p_connect_manager->enable_high_speed_channel = TRUE;
#else
	p_connect_manager->enable_high_speed_channel = FALSE;
#endif

    p_connect_manager->_last_dispatcher_time = 0;
    
	return SUCCESS;
}

_int32 cm_uninit_struct( CONNECT_MANAGER *p_connect_manager )
{
	_int32 ret_val = SUCCESS;
	
	LOG_DEBUG( "cm_uninit_struct begin. connect_manager ptr:0x%x, cur pipe num:%u, cur res num:%u",
		p_connect_manager, p_connect_manager->_cur_pipe_num, p_connect_manager->_cur_res_num );

	ret_val = cm_destroy_all_pipes( p_connect_manager );
	CHECK_VALUE( ret_val );
	
	ret_val = cm_destroy_all_ress( p_connect_manager );
	CHECK_VALUE( ret_val );
	
	LOG_DEBUG( "cm_uninit_struct end.cur pipe num:%u, cur res num:%u",
		p_connect_manager->_cur_pipe_num, p_connect_manager->_cur_res_num );

	sd_assert( p_connect_manager->_cur_pipe_num == 0 );
	sd_assert( p_connect_manager->_cur_res_num == 0 );	
	map_clear( &p_connect_manager->_server_hash_value_map );
	map_clear( &p_connect_manager->_peer_hash_value_map );	
	
	map_clear( &p_connect_manager->_destoryed_server_res_hash_value_map );
	map_clear( &p_connect_manager->_destoryed_peer_res_hash_value_map );	

#ifdef EMBED_REPORT   
	ret_val = range_list_clear(&(p_connect_manager->_report_para._res_normal_cdn_use_time_list));
#endif

	return SUCCESS;

}

_int32 cm_value_compare(void *E1, void *E2)
{
	_u32 value1 = (_u32)E1;
	_u32 value2 = (_u32)E2;
	return value1 > value2 ? 1 : ( value1 < value2 ? -1 : 0 );  
}

BOOL cm_is_server_res_exist( CONNECT_MANAGER* p_connect_manager, char *url, _u32 url_size, _u32 *p_hash_value )
{
	_int32 ret_val = SUCCESS;
	_u32 key = 0;
	MAP_ITERATOR cur_node;
	BOOL ret_bool = FALSE;

	ret_val = sd_get_url_hash_value( url, url_size, &key );
	CHECK_VALUE( ret_val );
	*p_hash_value = key;
	
	ret_val = map_find_iterator( &p_connect_manager->_server_hash_value_map, (void *)key, &cur_node );
	CHECK_VALUE( ret_val );
	if( cur_node != MAP_END( p_connect_manager->_server_hash_value_map ) )
	{
		ret_bool = TRUE;
	}
	
	ret_val = map_find_iterator( &p_connect_manager->_destoryed_server_res_hash_value_map, (void *)key, &cur_node );
	CHECK_VALUE( ret_val );
	if( cur_node != MAP_END( p_connect_manager->_destoryed_server_res_hash_value_map ) )
	{
		ret_bool = TRUE;
	}
	return ret_bool;
}

void cm_adjust_url_format( char *p_url ,_u32 url_size)
{
	_u32 index = 0;
	if( p_url == NULL ) return;
	//LOG_DEBUG( "cm_adjust_url_format begin p_url:%s.", p_url);
	
	while( p_url[index] != '\0' && p_url[index] != ':' && index<url_size )
	{
	  if( p_url[index]>='A' && p_url[index]<='Z' )   
	  p_url[index]+=32;   
	  index++;   
	}
	
	//LOG_DEBUG( "cm_adjust_url_format   end p_url:%s.", p_url);
}

BOOL cm_is_enable_server_res( CONNECT_MANAGER* p_connect_manager
        , enum SCHEMA_TYPE url_type, BOOL is_origin )
{
	if ( !cm_is_enable_server_download() ) return FALSE;

	//if ( !cm_is_use_multires(p_connect_manager) && !is_origin ) return FALSE;

	if( (( url_type == HTTP )||( url_type == HTTPS )) && !cm_is_enable_http_download() )
	{
		return FALSE;
	}
	else if( url_type == FTP && !cm_is_enable_ftp_download() )
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}
/*
BOOL cm_is_passive_peer_res_exist( CONNECT_MANAGER *p_connect_manager, RESOURCE *p_res )
{
	if( cm_search_peer_res_from_list( &(p_connect_manager->_idle_peer_res_list), p_res ) ) return TRUE;
	
	if( cm_search_peer_res_from_list( &(p_connect_manager->_using_peer_res_list), p_res ) ) return TRUE;
	
	if( cm_search_peer_res_from_list( &(p_connect_manager->_candidate_peer_res_list), p_res ) ) return TRUE;
	
	if( cm_search_peer_res_from_list( &(p_connect_manager->_retry_peer_res_list), p_res ) ) return TRUE;
	
	if( cm_search_peer_res_from_list( &(p_connect_manager->_discard_peer_res_list), p_res ) ) return TRUE;

	return FALSE;
}
*/
	
BOOL cm_is_active_peer_res_exist( CONNECT_MANAGER *p_connect_manager, char *p_peer_id, _u32 peer_id_len, _u32 host_ip, _u16 port, _u32 *p_hash_value )
{
	_int32 ret_val = SUCCESS;
	_u32 key = 0;
	MAP_ITERATOR cur_node;
	BOOL ret_bool = FALSE;

	ret_val = get_peer_hash_value( p_peer_id, peer_id_len, host_ip, port, &key );
	CHECK_VALUE( ret_val );
	*p_hash_value = key;
	
	ret_val = map_find_iterator( &p_connect_manager->_peer_hash_value_map, (void *)key, &cur_node );
	CHECK_VALUE( ret_val );
	if( cur_node != MAP_END( p_connect_manager->_peer_hash_value_map ) )
	{
		ret_bool = TRUE;
	}
	
	ret_val = map_find_iterator( &p_connect_manager->_destoryed_peer_res_hash_value_map, (void *)key, &cur_node );
	CHECK_VALUE( ret_val );
	if( cur_node != MAP_END( p_connect_manager->_destoryed_peer_res_hash_value_map ) )
	{
		ret_bool = TRUE;
	}
	return ret_bool;
}


BOOL cm_is_enable_peer_res( CONNECT_MANAGER *p_connect_manager, _u32 peer_capability )
{
	PTL_CONNECT_TYPE connect_type;
	
	if ( !cm_is_enable_peer_download() ) return FALSE;
	if ( !cm_is_enable_p2p_download() ) return FALSE;

	connect_type = ptl_get_connect_type( peer_capability );
	if( connect_type == INVALID_CONN )
	{
		return FALSE;
	}
	else if( connect_type == ACTIVE_TCP_CONN && !cm_is_enable_p2p_tcp() )
	{
		return FALSE;
	}
	else if( connect_type == SAME_NAT_CONN && !cm_is_enable_p2p_same_nat() )
	{
		return FALSE;
	}
	else if( connect_type == ACTIVE_TCP_BROKER_CONN && !cm_is_enable_p2p_tcp_broker() )
	{
		return FALSE;
	}
	else if( connect_type == ACTIVE_UDP_BROKER_CONN && !cm_is_enable_p2p_udp_broker() )
	{
		return FALSE;
	}	
	else if( connect_type == ACTIVE_UDT_CONN && !cm_is_enable_p2p_udt() )
	{
		return FALSE;
	}	
	else if( connect_type == ACTIVE_PUNCH_HOLE_CONN && !cm_is_enable_punch_hole() )
	{
		return FALSE;
	}		
	else
	{
		return TRUE;
	}
	
}

#ifdef ENABLE_BT 
BOOL cm_is_enable_bt_res( CONNECT_MANAGER *p_connect_manager )
{
	BOOL is_enable = cm_is_enable_bt_download();
	
	LOG_DEBUG( "cm_is_enable_bt_res:%d", is_enable );
	return is_enable;
}

BOOL cm_is_bt_res_exist( CONNECT_MANAGER *p_connect_manager, _u32 host_ip, _u16 port, _u8 info_hash[20], _u32 *p_hash_value )
{
	_int32 ret_val;
	_u32 key = 0;
	MAP_ITERATOR cur_node;
	BOOL ret_bool = FALSE;
	
	sd_assert( p_connect_manager->_pipe_interface._pipe_interface_type == BT_PIPE_INTERFACE_TYPE 
		|| p_connect_manager->_pipe_interface._pipe_interface_type == BT_MAGNET_INTERFACE_TYPE);

	bt_get_peer_hash_value( host_ip, port, info_hash, &key );
	*p_hash_value = key;

	ret_val = map_find_iterator( &p_connect_manager->_peer_hash_value_map, (void *)key, &cur_node );
	CHECK_VALUE( ret_val );
	if( cur_node != MAP_END( p_connect_manager->_peer_hash_value_map ) )
	{
		ret_bool = TRUE;
	}
	
	ret_val = map_find_iterator( &p_connect_manager->_destoryed_peer_res_hash_value_map, (void *)key, &cur_node );
	CHECK_VALUE( ret_val );
	if( cur_node != MAP_END( p_connect_manager->_destoryed_peer_res_hash_value_map ) )
	{
		ret_bool = TRUE;
	}
	return ret_bool;

}
#endif /* #ifdef ENABLE_BT  */

#ifdef ENABLE_EMULE
BOOL cm_is_emule_res_exist( CONNECT_MANAGER *p_connect_manager, _u32 ip, _u16 port, _u32 *p_hash_value )
{
	_int32 ret_val;
	_u32 key = 0;
	MAP_ITERATOR cur_node;
	BOOL ret_bool = FALSE;
    
	sd_assert( p_connect_manager->_pipe_interface._pipe_interface_type == EMULE_PIPE_INTERFACE_TYPE );

    LOG_DEBUG("cm_is_emule_res_exist, ip = %u, port = %hu.", ip, port);

	emule_get_peer_hash_value(ip, port, &key);
	*p_hash_value = key;

	ret_val = map_find_iterator( &p_connect_manager->_peer_hash_value_map, (void *)key, &cur_node );
	CHECK_VALUE( ret_val );
	if( cur_node != MAP_END( p_connect_manager->_peer_hash_value_map ) )
	{
        LOG_DEBUG("find in p_connect_manager->_peer_hash_value_map");
		ret_bool = TRUE;
	}
	
	ret_val = map_find_iterator( &p_connect_manager->_destoryed_peer_res_hash_value_map, (void *)key, &cur_node );
	CHECK_VALUE( ret_val );
	if( cur_node != MAP_END( p_connect_manager->_destoryed_peer_res_hash_value_map ) )
	{
        LOG_DEBUG("find in p_connect_manager->_destoryed_peer_res_hash_value_map");
		ret_bool = TRUE;
	}
	return ret_bool;

}

#endif /* #ifdef ENABLE_EMULE  */

_int32 cm_remove_peer_res_from_list( CONNECT_MANAGER *p_connect_manager, RESOURCE_LIST *p_res_list, RESOURCE *p_res )
{
	_int32 ret_val = SUCCESS;
	LIST_ITERATOR cur_node = LIST_BEGIN( p_res_list->_res_list );
	LIST_ITERATOR end_node = LIST_END( p_res_list->_res_list );
	LOG_DEBUG( "cm_remove_peer_res_from_list." );	
	if(NULL != p_res)
		LOG_DEBUG( "cm_remove_peer_res_from_list, res_type = %d", p_res->_resource_type);	
	while( cur_node != end_node )
	{
		RESOURCE *p_list_res = LIST_VALUE(cur_node);
		if( is_peer_res_equal( p_res, p_list_res ) )
		{
			LOG_DEBUG( "cm_remove_peer_res_from_list.erase res:0x%x.", p_list_res );	
			if( p2p_resource_copy_lack_info(p_res, p_list_res) )
			{
				p_res->_max_retry_time = p_list_res->_max_retry_time;
				LOG_DEBUG( "cm_remove_peer_res_from_list. _max_retry_time:%u.", p_res->_max_retry_time );	
			}

			ret_val = list_erase( &p_res_list->_res_list, cur_node );
			CHECK_VALUE( ret_val );
			sd_assert( p_res->_pipe_num == 0 );
			ret_val = cm_destroy_res( p_connect_manager, p_list_res );
			CHECK_VALUE( ret_val );
			return SUCCESS;
		}
		cur_node = LIST_NEXT( cur_node );
	}
	return SUCCESS;	
}

_int32 cm_handle_passive_peer( CONNECT_MANAGER *p_connect_manager, RESOURCE *p_peer_res )
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "cm_handle_passive_peer.resource:0x%x", p_peer_res );	

	//if( cm_search_peer_res_from_list( &(p_connect_manager->_using_peer_res_list), p_peer_res ) ) return CM_ADD_PASSIVE_PEER_EXIST;

	ret_val = cm_handle_pasive_valid_peer( p_connect_manager, p_peer_res );
	if( ret_val != SUCCESS ) return ret_val;

	ret_val = cm_remove_peer_res_from_list( p_connect_manager, &(p_connect_manager->_idle_peer_res_list), p_peer_res );
	CHECK_VALUE( ret_val );
	
	ret_val = cm_remove_peer_res_from_list( p_connect_manager, &(p_connect_manager->_candidate_peer_res_list), p_peer_res );
	CHECK_VALUE( ret_val );
	
	ret_val = cm_remove_peer_res_from_list( p_connect_manager, &(p_connect_manager->_retry_peer_res_list), p_peer_res );
	CHECK_VALUE( ret_val );
	
	ret_val = cm_remove_peer_res_from_list( p_connect_manager, &(p_connect_manager->_discard_peer_res_list), p_peer_res );
	CHECK_VALUE( ret_val );
	
	return SUCCESS;
}

_int32 cm_handle_pasive_valid_peer( CONNECT_MANAGER *p_connect_manager, RESOURCE *p_peer_res )
{
	LIST_ITERATOR cur_node = NULL;
	DATA_PIPE *p_pipe = NULL;
	
	LOG_DEBUG( "cm_handle_pasive_valid_peer.resource:0x%x", p_peer_res );	

	cur_node = LIST_BEGIN( p_connect_manager->_connecting_peer_pipe_list._data_pipe_list );
	while( cur_node != LIST_END( p_connect_manager->_connecting_peer_pipe_list._data_pipe_list ) )
	{
		p_pipe = LIST_VALUE( cur_node );
		if( is_peer_res_equal( p_pipe->_p_resource, p_peer_res ) )
		{
			//若发现pipe被choke状态,若本pipe发生错误,将下次重连时间随机加大几S(最大8S),用来防止两个盒子同时对连连不上
	
			if( p_pipe->_dispatch_info._pipe_state == PIPE_CHOKED )
			{
				p_pipe->_p_resource->_retry_time_interval = sd_rand() % 9;
			}			
			if( p_pipe->_dispatch_info._pipe_state >= PIPE_CHOKED )
			{
				return CM_ADD_PASSIVE_PEER_EXIST;
			}
		}
		cur_node = LIST_NEXT( cur_node );
	}

	cur_node = LIST_BEGIN( p_connect_manager->_working_peer_pipe_list._data_pipe_list );
	while( cur_node != LIST_END( p_connect_manager->_working_peer_pipe_list._data_pipe_list ) )
	{
		p_pipe = LIST_VALUE( cur_node );
		if( is_peer_res_equal( p_pipe->_p_resource, p_peer_res ) )
		{
			if( p_pipe->_dispatch_info._pipe_state == PIPE_CHOKED )
			{
				p_pipe->_p_resource->_retry_time_interval = sd_rand() % 9;
			}			
			if( p_pipe->_dispatch_info._pipe_state >= PIPE_CHOKED )
			{
				return CM_ADD_PASSIVE_PEER_EXIST;
			}
		}
		cur_node = LIST_NEXT( cur_node );
	}	
	return SUCCESS;
}

_int32 out_put_range_list_ex(const char* peerid, const char* rangetype, RANGE_LIST* range_list)
{
	RANGE_LIST_NODE*  cur_node = range_list->_head_node;
	if(range_list->_node_size == 0)
	{
		LOG_DEBUG("%s,%s range_list size is 0, so return.", peerid, rangetype);
		return SUCCESS;
	}

	LOG_DEBUG("%s,%s range_list size is %u .", peerid, rangetype,  range_list->_node_size);

	while(cur_node != NULL)
	{
		LOG_DEBUG("%s,%s  range_list node is (%u, %u).",
			peerid, rangetype, cur_node->_range._index, cur_node->_range._num);
		cur_node = cur_node->_next_node;
	}

	return SUCCESS;
}


void cm_update_list_pipe_speed( PIPE_LIST *p_pipe_list, _u32 *p_total_speed, BOOL *is_err_get_buffer )
{
	LIST_ITERATOR cur_node = LIST_BEGIN( p_pipe_list->_data_pipe_list );
	LIST_ITERATOR end_node = LIST_END( p_pipe_list->_data_pipe_list );
	BOOL is_err_get_buffer_pipe = FALSE;
	*p_total_speed = 0;
	
	*is_err_get_buffer = TRUE;

	while( cur_node != end_node )
	{
		DATA_PIPE *p_data_pipe = LIST_VALUE( cur_node );
		is_err_get_buffer_pipe = FALSE;
		
		if( p_data_pipe->_dispatch_info._pipe_state == PIPE_DOWNLOADING )
		{
			if( pipe_is_err_get_buffer( p_data_pipe ) )
			{
				is_err_get_buffer_pipe = TRUE;
			}
            cm_update_data_pipe_speed( p_data_pipe );
		}
		else
		{
			p_data_pipe->_dispatch_info._speed = 0;
		}

		if( *is_err_get_buffer && !is_err_get_buffer_pipe )
		{
			*is_err_get_buffer = FALSE;
		}
		LOG_DEBUG( "update_pipe_speed:pipe_ptr:0x%x, speed:%u. pipe_state:%d, is_err_get_buffer_pipe:%d",
			p_data_pipe, p_data_pipe->_dispatch_info._speed, p_data_pipe->_dispatch_info._pipe_state, is_err_get_buffer_pipe );
		
		if(p_data_pipe->_data_pipe_type == P2P_PIPE && p_data_pipe->_p_resource->_resource_type == PEER_RESOURCE)
		{
			P2P_RESOURCE* peer_res = (P2P_RESOURCE* )p_data_pipe->_p_resource;

			LOG_DEBUG( "update_pipe_speed:peerid:%s, pipe_ptr:0x%x, speed:%u. pipe_state:%d, is_err_get_buffer_pipe:%d",
				peer_res->_peer_id,	p_data_pipe, p_data_pipe->_dispatch_info._speed, p_data_pipe->_dispatch_info._pipe_state, is_err_get_buffer_pipe );

			out_put_range_list_ex(peer_res->_peer_id, "update_pipe_speed _can_download_ranges",  &p_data_pipe->_dispatch_info._can_download_ranges);
			out_put_range_list_ex(peer_res->_peer_id, "update_pipe_speed _uncomplete_ranges",  &p_data_pipe->_dispatch_info._uncomplete_ranges);

			LOG_DEBUG( "update_pipe_speed:peerid:%s, assign(%u,%u), down (%u,%u)",
				peer_res->_peer_id,
                p_data_pipe->_dispatch_info._correct_error_range._index, p_data_pipe->_dispatch_info._correct_error_range._num,
				p_data_pipe->_dispatch_info._down_range._index, p_data_pipe->_dispatch_info._down_range._num);

		}


		p_data_pipe->_dispatch_info._max_speed = MAX( p_data_pipe->_dispatch_info._max_speed, p_data_pipe->_dispatch_info._speed );

		cur_node = LIST_NEXT( cur_node );
		*p_total_speed += p_data_pipe->_dispatch_info._speed;
	}
}


void cm_update_list_pipe_upload_speed( PIPE_LIST *p_pipe_list, _u32 *p_total_speed )
{
	
#ifdef _CONNECT_DETAIL

	LIST_ITERATOR cur_node = LIST_BEGIN( p_pipe_list->_data_pipe_list );
	LIST_ITERATOR end_node = LIST_END( p_pipe_list->_data_pipe_list );
	_u32 pipe_speed = 0;
		
	*p_total_speed = 0;

	while( cur_node != end_node )
	{
		DATA_PIPE *p_data_pipe = LIST_VALUE( cur_node );
		
		if( p_data_pipe->_dispatch_info._pipe_state == PIPE_DOWNLOADING )
		{
			if( p_data_pipe->_data_pipe_type == BT_PIPE )
			{
#ifdef UPLOAD_ENABLE
#ifdef ENABLE_BT
#ifdef ENABLE_BT_PROTOCOL

				bt_pipe_get_upload_speed( p_data_pipe, &pipe_speed );
#endif
#endif
#endif

			}
			else if( p_data_pipe->_data_pipe_type == P2P_PIPE )
			{
#ifdef UPLOAD_ENABLE
				p2p_pipe_get_upload_speed( p_data_pipe, &pipe_speed );
#endif
			}
		}

		LOG_DEBUG( "cm_update_list_pipe_upload_speed:pipe_ptr:0x%x, speed:%u. pipe_state:%d, pipe_type:%d",
			p_data_pipe, pipe_speed, p_data_pipe->_dispatch_info._pipe_state, p_data_pipe->_data_pipe_type );
		
		cur_node = LIST_NEXT( cur_node );
		*p_total_speed += pipe_speed;
	}
	
#endif

}

void cm_update_list_pipe_score( PIPE_LIST *p_pipe_list, _u32 *p_total_speed )
{
	LIST_ITERATOR cur_node = LIST_BEGIN( p_pipe_list->_data_pipe_list );
	LIST_ITERATOR end_node = LIST_END( p_pipe_list->_data_pipe_list );
	_u32 cur_time_ms = 0;
	_int32 ret_val = SUCCESS;
	
	ret_val = sd_time_ms( &cur_time_ms );
	sd_assert( ret_val == SUCCESS );
	LOG_DEBUG( "cm_update_list_pipe_score begin:list_ptr:0x%x.", p_pipe_list );
			
	*p_total_speed = 0;
    if (list_size(&(p_pipe_list->_data_pipe_list)) != 0)
    {
        while( cur_node != end_node )
        {
            DATA_PIPE *p_data_pipe = LIST_VALUE( cur_node );

            if( p_data_pipe->_dispatch_info._pipe_state == PIPE_DOWNLOADING )
            {
                cm_update_data_pipe_speed( p_data_pipe );
                p_data_pipe->_dispatch_info._score = p_data_pipe->_dispatch_info._speed;
            }	
            else if( p_data_pipe->_dispatch_info._pipe_state == PIPE_CHOKED )
            {
                p_data_pipe->_dispatch_info._score =  cm_get_choke_pipe_score( p_data_pipe, cur_time_ms );
                p_data_pipe->_dispatch_info._speed = 0;
            }

            LOG_DEBUG( "update_pipe_score:pipe_ptr:0x%x, speed:%u. pipe_state:%d, pipe_score:%u",
                p_data_pipe, p_data_pipe->_dispatch_info._speed, p_data_pipe->_dispatch_info._pipe_state, p_data_pipe->_dispatch_info._score );
            
            p_data_pipe->_dispatch_info._max_speed = MAX( p_data_pipe->_dispatch_info._max_speed, p_data_pipe->_dispatch_info._speed );
            p_data_pipe->_p_resource->_max_speed = MAX( p_data_pipe->_dispatch_info._max_speed, p_data_pipe->_p_resource->_max_speed );

            cur_node = LIST_NEXT( cur_node );
            *p_total_speed += p_data_pipe->_dispatch_info._speed;
        }
    }
	
	LOG_DEBUG( "cm_update_list_pipe_score begin:list_ptr:0x%x. total_speed:%d",
		p_pipe_list, *p_total_speed );
}

_u32 cm_get_choke_pipe_score( DATA_PIPE *p_data_pipe, _u32 cur_time_ms )
{	
	PIPE_TYPE data_pipe_type = p_data_pipe->_data_pipe_type;
	_u8 res_level = 0;
	_u32 res_score = 0;
	_u32 time_intervel = 0;
	_u32 sub_speed = 0;
	_u32 speed_span = cm_choke_res_speed_span();

	BOOL is_ever_unchoke = TRUE;
	_u32 init_res_score = 0;

	sd_assert( data_pipe_type == P2P_PIPE || data_pipe_type == BT_PIPE || data_pipe_type == EMULE_PIPE );
	if( data_pipe_type == P2P_PIPE )
	{
		res_level = p2p_get_res_level( p_data_pipe->_p_resource );
		is_ever_unchoke = p2p_is_ever_unchoke( p_data_pipe );
	}
#ifdef ENABLE_BT  
#ifdef ENABLE_BT_PROTOCOL
	else if( data_pipe_type == BT_PIPE )
	{
		res_level = cm_choke_res_level_standard();
		is_ever_unchoke = bt_pipe_is_ever_unchoke( p_data_pipe );
	}
#endif /* #ifdef ENABLE_BT  */
#endif /* #ifdef ENABLE_BT  */


	if( is_ever_unchoke )
	{
		init_res_score = p_data_pipe->_dispatch_info._max_speed / speed_span * speed_span;
		LOG_DEBUG( "cm_get_choke_pipe_score:pipe_ptr:0x%x, is_ever_unchoke:%d. res_max_speed:%d, res_score:%d",
			p_data_pipe, is_ever_unchoke, p_data_pipe->_p_resource->_max_speed, res_score );
	}
	else
	{
		init_res_score = cm_excellent_choke_res_speed();
	}

	//sd_assert( cur_time_ms >= p_data_pipe->_dispatch_info._time_stamp );
	time_intervel = TIME_SUBZ( cur_time_ms, p_data_pipe->_dispatch_info._time_stamp );

	
	if( res_level >= cm_choke_res_level_standard()
		|| is_ever_unchoke )//8
	{
		sub_speed = time_intervel / cm_choke_res_time_span() * speed_span;//20s. 20K
		if( init_res_score > sub_speed )//60K
		{
			res_score = init_res_score - sub_speed;
		}
		else
		{
			res_score = 0;
		}
	}
	else
	{
		if( time_intervel < cm_choke_res_time_span() )
		{
			res_score = speed_span;//20s
		}			
		else
		{
			res_score = 0;
		}
	}
	LOG_DEBUG( "cm_get_choke_pipe_score:pipe_ptr:0x%x, is_ever_unchoke:%d. res_level:%u, res_score:%d, cur_time_ms:%u, _time_stamp:%u,time_intervel:%d ",
		p_data_pipe, is_ever_unchoke, (_u32)res_level, res_score, cur_time_ms, p_data_pipe->_dispatch_info._time_stamp, time_intervel );
	
	return res_score;
	
}

void cm_update_data_pipe_speed( DATA_PIPE *p_data_pipe )
{
	_int32 ret_val = SUCCESS;
	PIPE_TYPE data_pipe_type = p_data_pipe->_data_pipe_type;

	if( data_pipe_type == HTTP_PIPE )
	{
		ret_val = http_pipe_get_speed( p_data_pipe );
		sd_assert( ret_val == SUCCESS );
	}
	else if( data_pipe_type == FTP_PIPE )
	{
		ret_val = ftp_pipe_get_speed( p_data_pipe );
		sd_assert( ret_val == SUCCESS );
	}
	else if( data_pipe_type == P2P_PIPE )
	{
		ret_val = p2p_pipe_get_speed( p_data_pipe );
		sd_assert( ret_val == SUCCESS );
	}
#ifdef ENABLE_BT
#ifdef ENABLE_BT_PROTOCOL
	else if( data_pipe_type == BT_PIPE )
	{
		ret_val = bt_pipe_get_speed( p_data_pipe );
		sd_assert( ret_val == SUCCESS );
	}
#endif
#endif

#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL
	else if(data_pipe_type == EMULE_PIPE)
	{
		ret_val = emule_pipe_get_speed(p_data_pipe);
		sd_assert(ret_val == SUCCESS);
	}
#endif
#endif
	LOG_DEBUG( "cm_update_data_pipe_speed, data_pipe_type:%d, get_data_speed:%d",
		data_pipe_type, p_data_pipe->_dispatch_info._speed);
}


void cm_set_res_type_switch( CONNECT_MANAGER *p_connect_manager )
{
	_u32 _idle_server_res_num = list_size(&p_connect_manager->_idle_server_res_list._res_list);
	_u32 _idle_peer_res_num = list_size(&p_connect_manager->_idle_peer_res_list._res_list);
	if( _idle_server_res_num > _idle_peer_res_num )
	{
		p_connect_manager->_res_type_switch = 0;
	}

	LOG_DEBUG( "cm_set_res_type_switch, server_res_num:%d, peer_res_num:%d, switch:%d.",
		_idle_server_res_num, _idle_peer_res_num, p_connect_manager->_res_type_switch );
}

_int32 cm_update_connect_status( CONNECT_MANAGER *p_connect_manager )
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "cm_update_connect_status. connect_manager:0x%x", p_connect_manager );
	
	if(cm_is_pause_pipes(p_connect_manager))
	{
		/* 暂停下载情况下不更新pipe ! by zyq @20110117  */
		return ret_val;
	}
	
	ret_val = cm_destroy_all_discard_ress( p_connect_manager );
	CHECK_VALUE( ret_val );	
	
	ret_val = cm_update_server_connect_status( p_connect_manager );
	CHECK_VALUE( ret_val );	
	
	ret_val = cm_update_peer_connect_status( p_connect_manager );

	cm_check_is_fast_speed( p_connect_manager );
	
	cm_update_idle_status( p_connect_manager );	
	CHECK_VALUE( ret_val );	
	return SUCCESS;
}

_int32 cm_update_server_connect_status( CONNECT_MANAGER *p_connect_manager )
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "cm_update_server_connect_status." );
	
	ret_val = cm_update_server_pipe_list( p_connect_manager );
	CHECK_VALUE( ret_val );	

    cm_update_server_pipe_score( p_connect_manager );
	cm_update_server_hash_map( p_connect_manager );

	if( list_size(&p_connect_manager->_idle_server_res_list._res_list) > cm_need_idle_server_res_num() )
	{
		cm_set_need_new_server_res( p_connect_manager, FALSE );
	}
	else
	{
		cm_set_need_new_server_res( p_connect_manager, TRUE );
	}

	return SUCCESS;
}

void cm_update_server_hash_map( CONNECT_MANAGER *p_connect_manager )
{
	_u32 server_pipe_num = 0;
	p_connect_manager->_max_server_speed = MAX( p_connect_manager->_max_server_speed, p_connect_manager->_cur_server_speed );
	LOG_DEBUG( "cm_update_server_hash_map. _max_server_speed:%d, server_speed:%d, _clear_server_hash_map_ticks:%d",
		p_connect_manager->_max_server_speed, p_connect_manager->_cur_server_speed, p_connect_manager->_clear_server_hash_map_ticks );

	if( p_connect_manager->_max_server_speed > cm_clear_hash_map_speed_ratio() * p_connect_manager->_cur_server_speed )
	{
		p_connect_manager->_clear_server_hash_map_ticks++;
		if( p_connect_manager->_clear_server_hash_map_ticks > cm_clear_hash_map_ticks() )
		{
			LOG_DEBUG( "cm_update_server_hash_map. map_clear, map_size:%u",
				map_size(&p_connect_manager->_destoryed_server_res_hash_value_map) );
			map_clear( &p_connect_manager->_destoryed_server_res_hash_value_map );
			p_connect_manager->_clear_server_hash_map_ticks = 0;

			if( p_connect_manager->_cur_res_num < cm_discard_res_use_limit() )
			{
				cm_create_pipes_from_server_res_list( p_connect_manager, 
					&p_connect_manager->_discard_server_res_list, 
					cm_discard_res_max_use_num(),
					&server_pipe_num );
			}
		}
	}
	else
	{
		p_connect_manager->_clear_server_hash_map_ticks = 0;
	}		
}

void cm_update_server_pipe_speed( CONNECT_MANAGER *p_connect_manager, BOOL *is_server_err_get_buffer )
{
	_u32 total_speed = 0;
   	cm_update_list_pipe_speed( &p_connect_manager->_working_server_pipe_list, &total_speed, is_server_err_get_buffer );
	p_connect_manager->_cur_server_speed = total_speed;
	LOG_DEBUG( "cm_update_server_pipe_speed. server_speed:%d, is_server_err_get_buffer:%d", total_speed, *is_server_err_get_buffer );
}

void cm_update_server_pipe_score( CONNECT_MANAGER *p_connect_manager )
{
	_u32 total_speed = 0;
   	cm_update_list_pipe_score( &p_connect_manager->_working_server_pipe_list, &total_speed );
	p_connect_manager->_cur_server_speed = total_speed;
	LOG_DEBUG( "cm_update_server_pipe_score. server_speed:%d", total_speed );
}


_int32 cm_create_server_pipes( CONNECT_MANAGER *p_connect_manager )
{
	_u32 new_server_pipe_num = 0;
	_u32 server_pipe_num = 0;
	_u32 created_pipe_num = 0;
	_u32 idle_res_num = 0;
	
	_int32 ret_val = SUCCESS;
    LOG_DEBUG( "cm_create_server_pipes begin, connect_manager_ptr:0x%x.", p_connect_manager );

	new_server_pipe_num = cm_get_new_server_pipe_num( p_connect_manager );

	if( new_server_pipe_num == 0 )  return SUCCESS;	
	LOG_DEBUG( "cm_create_server_pipes from idle list begin, idle resource num, resource num = %u.", 
		list_size(&p_connect_manager->_idle_server_res_list._res_list) );
	ret_val = cm_create_pipes_from_server_res_list( p_connect_manager, 
		&p_connect_manager->_idle_server_res_list, 
		new_server_pipe_num - created_pipe_num,
		&server_pipe_num );
	CHECK_VALUE( ret_val );
	created_pipe_num += server_pipe_num;
	LOG_DEBUG( "cm_create_server_pipes from idle list end, created pipe num = %u.", server_pipe_num );

	sd_assert( created_pipe_num <= new_server_pipe_num);
	
	if( created_pipe_num >= new_server_pipe_num )  return SUCCESS;	
	idle_res_num = cm_idle_res_num( p_connect_manager );
	if( new_server_pipe_num - created_pipe_num < idle_res_num ) return SUCCESS;
		
	LOG_DEBUG( "cm_create_server_pipes from using list begin, using resource num = %u.", 
		list_size(&p_connect_manager->_using_server_res_list._res_list) );
	ret_val = cm_create_pipes_from_server_using_pipes( p_connect_manager, 
		new_server_pipe_num - created_pipe_num,
		&server_pipe_num );
	CHECK_VALUE( ret_val );
	created_pipe_num += server_pipe_num;
	LOG_DEBUG( "cm_create_server_pipes from using list end, created pipe num = %u.", server_pipe_num );

	sd_assert( created_pipe_num <= new_server_pipe_num);	
	if( created_pipe_num >= new_server_pipe_num )  return SUCCESS;	
	LOG_DEBUG( "cm_create_server_pipes from candidate list begin, candidate resource num  = %u.", 
		list_size(&p_connect_manager->_candidate_server_res_list._res_list) );
	ret_val = cm_create_pipes_from_server_res_list( p_connect_manager, 
		&p_connect_manager->_candidate_server_res_list, 
		new_server_pipe_num - created_pipe_num,
		&server_pipe_num );
	CHECK_VALUE( ret_val );
	created_pipe_num += server_pipe_num;
	LOG_DEBUG( "cm_create_server_pipes from candidate list end, created pipe num = %u.", server_pipe_num );
	
	sd_assert( created_pipe_num <= new_server_pipe_num);
	if( created_pipe_num >= new_server_pipe_num )  return SUCCESS;	
	LOG_DEBUG( "cm_create_server_pipes from retry list begin, retry resource num = %u.", 
		list_size(&p_connect_manager->_retry_server_res_list._res_list) );
	ret_val = cm_create_pipes_from_server_res_list( p_connect_manager, 
		&p_connect_manager->_retry_server_res_list, 
		new_server_pipe_num - created_pipe_num,
		&server_pipe_num );
	CHECK_VALUE( ret_val );
	created_pipe_num += server_pipe_num;
	LOG_DEBUG( "cm_create_server_pipes from retry list end, created pipe num = %u.", server_pipe_num );

	LOG_DEBUG( "cm_create_server_pipes total create pipes num:%u. connect_manager_ptr:0x%x", 
     created_pipe_num, p_connect_manager );
	return SUCCESS;
}

_u32 cm_get_new_server_pipe_num( CONNECT_MANAGER *p_connect_manager )
{
	_u32 new_server_pipe_num, connecting_server_pipe_num, connecting_pipe_num, server_pipe_num;
	LOG_DEBUG( "cm_get_new_server_pipe_num begin. connect_manager_ptr:0x%x", p_connect_manager );

	if( cm_max_pipe_num() <= p_connect_manager->_cur_pipe_num )
	{
		LOG_DEBUG( "cm_get_new_server_pipe_num : 0, max_pipe_num:%d, cur_pipe_num:%d.",
			cm_max_pipe_num(), p_connect_manager->_cur_pipe_num );	
		return 0;
	}
	
	new_server_pipe_num = cm_max_pipe_num() - p_connect_manager->_cur_pipe_num;
	
	LOG_DEBUG( "cm_get_new_server_pipe_num limit by total pipes num:%u.", new_server_pipe_num );	

	connecting_server_pipe_num = list_size( &(p_connect_manager->_connecting_server_pipe_list._data_pipe_list) );

    if( cm_max_connecting_server_pipe_num() <  connecting_server_pipe_num )
    {
        LOG_ERROR( "cm_get_new_server_pipe_num warn:cm_max_connecting_server_pipe_num,%u. connecting_server_pipe_num:%u",
            cm_max_connecting_server_pipe_num(),  connecting_server_pipe_num );    
        return 0;
    }


    new_server_pipe_num = MIN( new_server_pipe_num, cm_max_connecting_server_pipe_num() - connecting_server_pipe_num );
	LOG_DEBUG( "cm_get_new_server_pipe_num:cm_max_connecting_server_pipe_num:%u, new_server_pipe_num:%u.connecting_server_pipe_num:%u.", 
     cm_max_connecting_server_pipe_num(), new_server_pipe_num, connecting_server_pipe_num );	

    sd_assert(cm_max_connecting_server_pipe_num() >= connecting_server_pipe_num );
	LOG_DEBUG( "cm_get_new_server_pipe_num limit by connecting server pipes num:%u.", cm_max_connecting_server_pipe_num() - connecting_server_pipe_num );	
	
	server_pipe_num = list_size( &(p_connect_manager->_working_server_pipe_list._data_pipe_list) ) + connecting_server_pipe_num;

	new_server_pipe_num = MIN( new_server_pipe_num, cm_max_server_pipe_num() - server_pipe_num );
	sd_assert(cm_max_server_pipe_num() >= server_pipe_num );
	LOG_DEBUG( "cm_get_new_server_pipe_num limit by server pipes num:%u.", cm_max_server_pipe_num() - server_pipe_num );	

	connecting_pipe_num = list_size( &(p_connect_manager->_connecting_peer_pipe_list._data_pipe_list) ) + connecting_server_pipe_num;
    if( cm_max_connecting_pipe_num() <  connecting_pipe_num )
    {
        LOG_ERROR( "cm_get_new_server_pipe_num warn:cm_max_connecting_pipe_num,%u. connecting_pipe_num:%u",
            cm_max_connecting_pipe_num(),  connecting_pipe_num );    
        return 0;
    }

    new_server_pipe_num = MIN( new_server_pipe_num, cm_max_connecting_pipe_num() - connecting_pipe_num );
	LOG_DEBUG( "cm_get_new_server_pipe_num:cm_max_connecting_peer_pipe_num:%u, new_server_pipe_num:%u.connecting_pipe_num:%u.", 
     cm_max_connecting_pipe_num(), new_server_pipe_num, connecting_pipe_num );	

	sd_assert(cm_max_connecting_pipe_num() >= connecting_pipe_num );
	LOG_DEBUG( "cm_get_new_server_pipe_num limit by connecting pipes num:%u.", cm_max_connecting_pipe_num() - connecting_pipe_num );	
	
	LOG_DEBUG( "cm_get_new_server_pipe_num end new_server_pipe_num:%u.connect_manager ptr:0x%x", new_server_pipe_num, p_connect_manager );
	return new_server_pipe_num;	
}


_int32 cm_create_pipes_from_server_res_list( CONNECT_MANAGER *p_connect_manager
        , RESOURCE_LIST *p_res_list, _u32 max_pipe_num, _u32 *p_create_pipe_num )
{
    _u32  server_pipe_num = 0;
    RESOURCE *p_res = NULL;
    _int32 ret_val = SUCCESS;

    LIST_ITERATOR next_node, cur_node, end_node;
    cur_node = LIST_BEGIN( p_res_list->_res_list );
    end_node = LIST_END( p_res_list->_res_list );

    LOG_DEBUG( "cm_create_pipes_from_server_res_list:max_pipe_num = %u. ", max_pipe_num);
    if (list_size(&(p_res_list->_res_list)) != 0)
    {
        while( cur_node != end_node && server_pipe_num < max_pipe_num )
        {
            p_res = LIST_VALUE( cur_node );
            sd_assert( p_res->_resource_type == HTTP_RESOURCE || p_res->_resource_type == FTP_RESOURCE );
            if( ( !cm_is_use_multires(p_connect_manager) )
                && (p_res!=p_connect_manager->_origin_res_ptr) )
            {
                //LOG_URGENT( "cm_create_pipes_from_server_res_list, thi is not origin server, res : %X. ", p_res);
                cur_node = LIST_NEXT( cur_node );
                continue;
            }
            sd_assert( p_res->_pipe_num == 0 );
            LOG_DEBUG( "cm_create_pipes_from_server_res_list. resource:0x%x", p_res );
            ret_val = cm_create_single_server_pipe( p_connect_manager, p_res );

            if( ret_val == SUCCESS )
            {
                LOG_DEBUG( "push resource:0x%x into _using_server_res_list, resource type = %d, resource list ptr:0x%x", p_res, p_res->_resource_type, &p_connect_manager->_using_server_res_list );
                ret_val = list_push( &(p_connect_manager->_using_server_res_list._res_list), p_res );
                CHECK_VALUE( ret_val );
                server_pipe_num++;
            }
            else
            {
                sd_assert( p_res->_pipe_num == 0 );
                ret_val = list_push( &(p_connect_manager->_discard_server_res_list._res_list), p_res );
                CHECK_VALUE( ret_val );
            }

            next_node = LIST_NEXT( cur_node );
            ret_val = list_erase( &p_res_list->_res_list, cur_node );
            CHECK_VALUE( ret_val );
            cur_node = next_node;
        }

        *p_create_pipe_num = server_pipe_num;
    }
    return SUCCESS;
}

_u32 cm_get_max_pipe_num_each_server( CONNECT_MANAGER *p_connect_manager)
{
	_u32 max_pipe = cm_max_pipe_num_each_server();

	return max_pipe;
}

_u32 cm_is_origin_mode(CONNECT_MANAGER* p_connect_manager)
{
    if(p_connect_manager->_main_task_ptr == NULL)
    {
        //CHECK_VALUE(1);
        return 0;
    }
    else
    {
        _u32 ret_val = (p_connect_manager->_main_task_ptr->_working_mode ? 0 : 1 );
        LOG_DEBUG("cm_is_origin_mode, mode : %d, ret=%d", p_connect_manager->_main_task_ptr->_working_mode, ret_val);
        return ret_val;
    }
}

_u32 cm_is_use_multires(CONNECT_MANAGER* p_connect_manager)
{
    if(p_connect_manager->_main_task_ptr == NULL)
    {
        //CHECK_VALUE(1);
        return 1;
    }
    else
    {
    	LOG_DEBUG("cm_is_use_multires,(p_connect_manager->_main_task_ptr->_working_mode & S_USE_NORMAL_MULTI_RES )=%d", (p_connect_manager->_main_task_ptr->_working_mode & S_USE_NORMAL_MULTI_RES ));
        _u32 ret_val = ((p_connect_manager->_main_task_ptr->_working_mode & S_USE_NORMAL_MULTI_RES )?1 : 0 );
        LOG_DEBUG("cm_is_use_multires, mode : %d, ret=%d", p_connect_manager->_main_task_ptr->_working_mode, ret_val);

        if(ret_val == 1)    // 使用多资源的情况下再判断是否小文件模式
        {
            ret_val = ( (p_connect_manager->_main_task_ptr->_working_mode & S_SMALL_FILE_MODE )?0 : 1 ); 
        }

        return ret_val;
    }
}


_u32 cm_is_use_pcres(CONNECT_MANAGER* p_connect_manager)
{
    if(p_connect_manager->_main_task_ptr == NULL)
    {
        return 1;
    }
    else
    {
        _u32 ret_val = ((p_connect_manager->_main_task_ptr->_working_mode & S_USE_PC_RES)? 1 : 0 );
        LOG_DEBUG("cm_use_pc_res, mode : %d, ret=%d", p_connect_manager->_main_task_ptr->_working_mode, ret_val);
        return ret_val;
    }
}

_u32 cm_set_use_pcres(CONNECT_MANAGER * p_connect_manager, BOOL isuse)
{
	LOG_DEBUG("cm_set_use_pcres, isuse :%d", isuse);
	_u32 ret_val = 0;
    if(p_connect_manager->_main_task_ptr == NULL)
    {
        return 0;
    }
    else
    {
    	p_connect_manager->_main_task_ptr->_working_mode |= (isuse ? S_USE_PC_RES : 0);
        LOG_DEBUG("cm_set_use_pcres, mode : %d, ret=%d", p_connect_manager->_main_task_ptr->_working_mode, ret_val);
        return ret_val;
    }
}


_u32 cm_create_new_pipes(CONNECT_MANAGER *p_connect_manager, _u32 sum)
{
    _u32 new_pipe_num = sum;
    _u32 created_pipe_num = 0;
    _u32 server_pipe_num = 0;
    _u32 peer_pipe_num = 0;
    //_u32 idle_res_num = 0;

    _int32 ret_val = SUCCESS;
    LOG_DEBUG( "cm_create_new_pipes from peers begin, connect_manager_ptr:0x%x.", p_connect_manager );

    if( new_pipe_num == 0 )  return created_pipe_num;

    /*
    LOG_DEBUG( "cm_create_server_pipes from using list begin, using resource num = %u.",
        list_size(&p_connect_manager->_using_server_res_list._res_list) );
    ret_val = cm_create_pipes_from_server_using_pipes( p_connect_manager,
        new_pipe_num - created_pipe_num,
        &server_pipe_num );
    CHECK_VALUE( ret_val );
    created_pipe_num += server_pipe_num;
    LOG_DEBUG( "cm_create_server_pipes from using list end, created pipe num = %u.", server_pipe_num );

    sd_assert( created_pipe_num <= new_pipe_num);
    if( created_pipe_num >= new_pipe_num )  return created_pipe_num;
    */

    LOG_DEBUG("cm_create_new_pipes new peer count : %d", list_size(&p_connect_manager->_idle_peer_res_list._res_list));
    // create peer
    ret_val = cm_create_pipes_from_peer_res_list( p_connect_manager,
              &p_connect_manager->_idle_peer_res_list,
              FALSE,
              new_pipe_num- created_pipe_num,
              &peer_pipe_num );
    CHECK_VALUE( ret_val );
    created_pipe_num += peer_pipe_num;
    LOG_DEBUG( "cm_create_active_peer_pipes from idle list end, created pipe num = %u.", peer_pipe_num );

    // create server again
    sd_assert( created_pipe_num <= new_pipe_num);
    if( created_pipe_num >= new_pipe_num )  return created_pipe_num;
    LOG_DEBUG( "cm_create_server_pipes from candidate list begin, candidate resource num  = %u.",
               list_size(&p_connect_manager->_candidate_server_res_list._res_list) );
    ret_val = cm_create_pipes_from_server_res_list( p_connect_manager,
              &p_connect_manager->_candidate_server_res_list,
              new_pipe_num - created_pipe_num,
              &server_pipe_num );
    CHECK_VALUE( ret_val );
    created_pipe_num += server_pipe_num;
    LOG_DEBUG( "cm_create_server_pipes from candidate list end, created pipe num = %u.", server_pipe_num );

    sd_assert( created_pipe_num <= new_pipe_num);
    if( created_pipe_num >= new_pipe_num )  return created_pipe_num;
    LOG_DEBUG( "cm_create_server_pipes from retry list begin, retry resource num = %u.",
               list_size(&p_connect_manager->_retry_server_res_list._res_list) );
    ret_val = cm_create_pipes_from_server_res_list( p_connect_manager,
              &p_connect_manager->_retry_server_res_list,
              new_pipe_num - created_pipe_num,
              &server_pipe_num );
    CHECK_VALUE( ret_val );
    created_pipe_num += server_pipe_num;
    LOG_DEBUG( "cm_create_server_pipes from retry list end, created pipe num = %u.", server_pipe_num );

    // create peer again
    LOG_DEBUG( "cm_create_active_peer_pipes from candidate list begin , candidate resource num = %u.",
               list_size(&p_connect_manager->_candidate_peer_res_list._res_list) );
    if( created_pipe_num >= new_pipe_num )  return created_pipe_num;
    ret_val = cm_create_pipes_from_peer_res_list( p_connect_manager,
              &p_connect_manager->_candidate_peer_res_list,
              FALSE,
              new_pipe_num - created_pipe_num,
              &peer_pipe_num );
    CHECK_VALUE( ret_val );
    created_pipe_num += peer_pipe_num;
    LOG_DEBUG( "cm_create_active_peer_pipes from candidate list end , created pipe num = %u.", peer_pipe_num );

    sd_assert( created_pipe_num <= new_pipe_num);
    if( created_pipe_num >= new_pipe_num )  return created_pipe_num;
    LOG_DEBUG( "cm_create_active_peer_pipes from retry list begin, retry resource num = %u.",
               list_size(&p_connect_manager->_retry_peer_res_list._res_list) );
    ret_val = cm_create_pipes_from_peer_res_list( p_connect_manager,
              &p_connect_manager->_retry_peer_res_list,
              TRUE,
              new_pipe_num - created_pipe_num,
              &peer_pipe_num );
    CHECK_VALUE( ret_val );
    created_pipe_num += peer_pipe_num;
    LOG_DEBUG( "cm_create_active_peer_pipes from retry list  end, created pipe num = %u.", peer_pipe_num );

//  LOG_DEBUG( "cm_create_server_pipes total create pipes num:%u. connect_manager_ptr:0x%x",
//     created_pipe_num, p_connect_manager );
//  LOG_DEBUG( "cm_create_active_peer_pipes total create pipes num:%u.connect_manager_ptr:0x%x ",
//     created_pipe_num, p_connect_manager );

    LOG_DEBUG( "cm_create_pipes total create pipes num:%u.connect_manager_ptr:0x%x ",
               created_pipe_num, p_connect_manager );
    return created_pipe_num;


}


_int32 cm_destroy_not_support_range_speed_up_res(CONNECT_MANAGER* p_connect_manager)
{
    LOG_DEBUG("cm_destroy_not_support_range_speed_up_res enter, using server_res list size = %u"
            , list_size(&(p_connect_manager->_using_server_res_list._res_list)));
    //_int32 ret_val = SUCCESS;
    DATA_PIPE *p_data_pipe = NULL;
    RESOURCE *p_res = NULL;

    LIST_ITERATOR cur_node, next_node;
    cur_node = LIST_BEGIN( p_connect_manager->_working_server_pipe_list._data_pipe_list );
    
    while( cur_node != LIST_END( p_connect_manager->_working_server_pipe_list._data_pipe_list) )
    {
        p_data_pipe = LIST_VALUE( cur_node );
        p_res = p_data_pipe->_p_resource;
        LOG_DEBUG( "pipe:0x%x, pipe_speed:%u resource:0x%x, resource's pipe num:%u,type=%d,_max_speed=%u,cnting_pipe_num=%u,support-range=%d",
                   p_data_pipe, p_data_pipe->_dispatch_info._speed, p_res, p_res->_pipe_num ,p_res->_resource_type,p_res->_max_speed
                   ,p_res->_connecting_pipe_num,http_resource_is_support_range( p_res ));

        if( ( ( p_res->_resource_type == HTTP_RESOURCE && !http_resource_is_support_range( p_res ) )
            || ( p_res->_resource_type == FTP_RESOURCE && !ftp_resource_is_support_range( p_res ) ) )
            && p_res != p_connect_manager->_origin_res_ptr )
        {
            cm_destroy_single_pipe(p_connect_manager, p_data_pipe);
            next_node = LIST_NEXT( cur_node );
            list_erase(&p_connect_manager->_working_server_pipe_list._data_pipe_list, cur_node);

            if(0 == p_res->_pipe_num)
            {
            	   cm_move_res(&p_connect_manager->_using_server_res_list
                    , &p_connect_manager->_discard_server_res_list
                    , p_res );
            }
            
            cur_node = next_node;
            continue;
        }

        cur_node = LIST_NEXT( cur_node );
    }    
    return SUCCESS;
}

_int32 cm_create_pipes_from_server_using_pipes( CONNECT_MANAGER *p_connect_manager
        , _u32 max_pipe_num
        , _u32 *p_create_pipe_num )
{
    _int32 ret_val = SUCCESS;
    _u32  server_pipe_num = 0;
    DATA_PIPE *p_data_pipe = NULL;
    RESOURCE *p_res = NULL;

    _u32 max_pipe_speed = MAX(p_connect_manager->_max_server_speed,
                              p_connect_manager->_max_peer_speed);
    _u32 speed_limit = max_pipe_speed/2;
    LOG_DEBUG("max_pipe_speed :%d, speed_limit = %d", max_pipe_speed, speed_limit);

    LIST_ITERATOR cur_node, end_node;
    cur_node = LIST_BEGIN( p_connect_manager->_working_server_pipe_list._data_pipe_list );
    end_node = LIST_END( p_connect_manager->_working_server_pipe_list._data_pipe_list );

    while( cur_node != end_node && server_pipe_num < max_pipe_num)
    {
        p_data_pipe = LIST_VALUE( cur_node );
        p_res = p_data_pipe->_p_resource;
        LOG_DEBUG( "pipe:0x%x, pipe_speed:%u resource:0x%x, resource's pipe num:%u,type=%d,_max_speed=%u,cnting_pipe_num=%u,support-range=%d,max_pipe_num=%u,max_pipes_each_res=%d",
                   p_data_pipe, p_data_pipe->_dispatch_info._speed, p_res, p_res->_pipe_num ,p_res->_resource_type,p_res->_max_speed,p_res->_connecting_pipe_num,http_resource_is_support_range( p_res ),max_pipe_num, cm_get_max_pipe_num_each_server(p_connect_manager));
        sd_assert( p_res->_resource_type == HTTP_RESOURCE || p_res->_resource_type == FTP_RESOURCE );
        if( p_res->_pipe_num >= cm_get_max_pipe_num_each_server(p_connect_manager)
            //|| p_res->_connecting_pipe_num >= 1
            || p_res->_max_speed < speed_limit )
        {
            cur_node = LIST_NEXT( cur_node );
            continue;
        }
        if( ( p_res->_resource_type == HTTP_RESOURCE && !http_resource_is_support_range( p_res ) )
            || ( p_res->_resource_type == FTP_RESOURCE && !ftp_resource_is_support_range( p_res ) ) )
        {
            cur_node = LIST_NEXT( cur_node );
            continue;
        }

        while( p_res->_pipe_num < cm_get_max_pipe_num_each_server(p_connect_manager)
               && server_pipe_num < max_pipe_num )
        {
            LOG_DEBUG( "cm_create_pipes_from_server_using_pipes. pipe:0x%x, pipe_speed:%u resource:0x%x, resource's pipe num:%u ",
                       p_data_pipe, p_data_pipe->_dispatch_info._speed, p_res, p_res->_pipe_num );
            ret_val = cm_create_single_server_pipe( p_connect_manager, p_res );// if error, do nothing
            if( ret_val == SUCCESS )
            {
                server_pipe_num++;
            }
            else
            {
                break;
            }
        }

        cur_node = LIST_NEXT( cur_node );
    }
    *p_create_pipe_num = server_pipe_num;
    return SUCCESS;
}

_int32 cm_create_single_server_pipe( CONNECT_MANAGER *p_connect_manager, RESOURCE *p_res )
{
	_int32 ret_val = SUCCESS;
	DATA_PIPE *p_pipe = NULL;
	RANGE can_download_range;
	RANGE padding_can_download_range;
	_u64 file_size = 0;
	LOG_DEBUG( "cm_create_single_server_pipe, res_type = %d", p_res->_resource_type );

	if( p_res->_resource_type == HTTP_RESOURCE )
	{
		LOG_DEBUG( "cm create http pipe. resource:0x%x", p_res );
		ret_val = http_pipe_create( p_connect_manager->_data_manager_ptr, p_res, &p_pipe );
	   	if( ret_val != SUCCESS )
	   	{
			LOG_ERROR( "cm create http pipe error. error code:%d.", ret_val );
			return ret_val;
	   	}
		
		dp_set_pipe_interface( p_pipe, &p_connect_manager->_pipe_interface );

		p_connect_manager->_cur_pipe_num++;
		p_res->_pipe_num++;	
	
		ret_val = gcm_register_pipe( p_connect_manager, p_pipe );
		CHECK_VALUE( ret_val );		
		
		LOG_DEBUG( "cm open http server pipe. resource:0x%x, pipe:0x%x", p_res, p_pipe );
		ret_val = http_pipe_open( p_pipe );
		if( ret_val != SUCCESS )
		{
			LOG_ERROR( "cm open http pipe error. error code:%d.", ret_val );
			cm_destroy_single_pipe( p_connect_manager, p_pipe );
			return ret_val;
		}
		p_res->_connecting_pipe_num++;		
	}
	else if( p_res->_resource_type == FTP_RESOURCE )
	{
		LOG_DEBUG( "cm create ftp pipe. resource:0x%x", p_res );
		ret_val = ftp_pipe_create( p_connect_manager->_data_manager_ptr, p_res, &p_pipe );
	   	if( ret_val != SUCCESS )
	   	{
			LOG_ERROR( "cm create ftp pipe error. error code:%d.", ret_val );
			return ret_val;
	   	}

		dp_set_pipe_interface( p_pipe, &p_connect_manager->_pipe_interface );

		p_connect_manager->_cur_pipe_num++;
		p_res->_pipe_num++;	
		
		ret_val = gcm_register_pipe( p_connect_manager, p_pipe );
		CHECK_VALUE( ret_val );	
		
		LOG_DEBUG( "cm open ftp server pipe. resource:0x%x, pipe:0x%x", p_res, p_pipe );			
		ret_val = ftp_pipe_open( p_pipe );
		if( ret_val != SUCCESS )
		{
			LOG_ERROR( "cm open server pipe error. error code:%d.", ret_val );
			cm_destroy_single_pipe( p_connect_manager, p_pipe );
			return ret_val;
		}
		p_res->_connecting_pipe_num++;
	} 

	if( p_connect_manager->_pipe_interface._pipe_interface_type == SPEEDUP_PIPE_INTERFACE_TYPE )
	{
		file_size = pi_get_file_size( p_pipe );
		can_download_range  = pos_length_to_range( 0, file_size, file_size );

		ret_val = pi_pipe_set_dispatcher_range( p_pipe, (void *)&can_download_range, &padding_can_download_range );
		ret_val = range_list_add_range( &p_pipe->_dispatch_info._can_download_ranges, &padding_can_download_range, NULL, NULL );
		CHECK_VALUE( ret_val );
	}

    LOG_DEBUG("after cm_create_single_server_pipe, cur_pipe_num = %d", p_connect_manager->_cur_pipe_num );

	ret_val = list_push( &p_connect_manager->_connecting_server_pipe_list._data_pipe_list, p_pipe );
    LOG_DEBUG( "cm_create_single_server_pipe. _connecting_server_pipe_list:0x%x, list_size:%u, [dispatch_stat] pipe 0x%x create 0x%x (resource).",
        &p_connect_manager->_connecting_server_pipe_list._data_pipe_list, 
        list_size(&p_connect_manager->_connecting_server_pipe_list._data_pipe_list),
        p_pipe, p_res);
	CHECK_VALUE( ret_val );

	return SUCCESS;
}

_int32 cm_update_peer_connect_status( CONNECT_MANAGER *p_connect_manager )
{
	_int32 ret_val = SUCCESS;

	LOG_DEBUG( "cm_update_peer_connect_status." );
	ret_val = cm_update_peer_pipe_list( p_connect_manager );//must before cm_update_peer_pipe_score
	CHECK_VALUE( ret_val );
	
    cm_update_peer_pipe_score( p_connect_manager );
	cm_update_peer_hash_map( p_connect_manager );

	if( list_size(&p_connect_manager->_idle_peer_res_list._res_list) > cm_need_idle_peer_res_num()
		|| list_size(&p_connect_manager->_retry_peer_res_list._res_list) > cm_need_retry_peer_res_num() )
	{
		cm_set_need_new_peer_res( p_connect_manager, FALSE );
	}
	else
	{
		cm_set_need_new_peer_res( p_connect_manager, TRUE );
	}

	return SUCCESS;
}

void cm_update_peer_hash_map( CONNECT_MANAGER *p_connect_manager )
{
	_u32 peer_pipe_num = 0;
	p_connect_manager->_max_peer_speed = MAX( p_connect_manager->_max_peer_speed, p_connect_manager->_cur_peer_speed );
	LOG_DEBUG( "cm_update_peer_hash_map. _max_peer_speed:%d, peer_speed:%d, _clear_peer_hash_map_ticks:%d",
		p_connect_manager->_max_peer_speed, p_connect_manager->_cur_peer_speed, p_connect_manager->_clear_peer_hash_map_ticks );

	if( p_connect_manager->_max_peer_speed > cm_clear_hash_map_speed_ratio() * p_connect_manager->_cur_peer_speed )
	{
		p_connect_manager->_clear_peer_hash_map_ticks++;
		if( p_connect_manager->_clear_peer_hash_map_ticks > cm_clear_hash_map_ticks() )
		{
			LOG_DEBUG( "cm_update_peer_hash_map. map_clear, map_size:%u",
				map_size(&p_connect_manager->_destoryed_peer_res_hash_value_map) );

			map_clear( &p_connect_manager->_destoryed_peer_res_hash_value_map );
			p_connect_manager->_clear_peer_hash_map_ticks = 0;

			if( p_connect_manager->_cur_res_num < cm_discard_res_use_limit() )
			{
				cm_create_pipes_from_peer_res_list( p_connect_manager, 
				&p_connect_manager->_discard_peer_res_list, 
				FALSE,
				cm_discard_res_max_use_num(),
				&peer_pipe_num );			
			}
		}
	}
	else
	{
		p_connect_manager->_clear_peer_hash_map_ticks = 0;
	}	
}

void cm_update_peer_pipe_speed( CONNECT_MANAGER *p_connect_manager, BOOL *is_peer_err_get_buffer )
{
	_u32 total_speed = 0;	
	_u32 total_upload_speed = 0;
    cm_update_list_pipe_speed( &p_connect_manager->_working_peer_pipe_list, &total_speed, is_peer_err_get_buffer );
	p_connect_manager->_cur_peer_speed = total_speed;

	cm_update_list_pipe_upload_speed( &p_connect_manager->_working_peer_pipe_list, &total_upload_speed );
	p_connect_manager->_cur_peer_upload_speed = total_upload_speed;

	LOG_DEBUG( "cm_update_peer_pipe_speed. peer_speed:%u, peed_upload_speed:%u, is_peer_err_get_buffer:%d", 
		total_speed, total_upload_speed, *is_peer_err_get_buffer );
}

void cm_update_peer_pipe_score( CONNECT_MANAGER *p_connect_manager )
{
	_u32 total_speed = 0;	
    cm_update_list_pipe_score( &p_connect_manager->_working_peer_pipe_list, &total_speed );
	p_connect_manager->_cur_peer_speed = total_speed;
	LOG_DEBUG( "cm_update_peer_pipe_score. peer_speed:%d", total_speed );
}

_int32 cm_create_active_peer_pipes( CONNECT_MANAGER *p_connect_manager )
{
	_u32 new_peer_pipe_num = 0;
	_u32 peer_pipe_num = 0;
	_u32 created_pipe_num = 0;
	_int32 ret_val = SUCCESS;
	_u32 idle_res_num = 0;
	
	if (!cm_is_use_multires(p_connect_manager) ) return SUCCESS;		
	
    LOG_DEBUG( "cm_create_active_peer_pipes begin, connect_manager_ptr:0x%x.", p_connect_manager );

    new_peer_pipe_num = cm_get_new_peer_pipe_num( p_connect_manager );

	if( new_peer_pipe_num == 0 )  return SUCCESS;	
	LOG_DEBUG( "cm_create_active_peer_pipes from idle list begin, idle resource num = %u.", 
		list_size(&p_connect_manager->_idle_peer_res_list._res_list) );	
	ret_val = cm_create_pipes_from_peer_res_list( p_connect_manager, 
		&p_connect_manager->_idle_peer_res_list, 
		FALSE,
		new_peer_pipe_num - created_pipe_num,
		&peer_pipe_num );
	CHECK_VALUE( ret_val );
	created_pipe_num += peer_pipe_num;
	LOG_DEBUG( "cm_create_active_peer_pipes from idle list end, created pipe num = %u.", peer_pipe_num );

	sd_assert( created_pipe_num <= new_peer_pipe_num);
	if( created_pipe_num >= new_peer_pipe_num )  return SUCCESS;	
	idle_res_num = cm_idle_res_num( p_connect_manager );
	if( new_peer_pipe_num - created_pipe_num < idle_res_num ) return SUCCESS;
	
	LOG_DEBUG( "cm_create_active_peer_pipes from candidate list begin , candidate resource num = %u.", 
		list_size(&p_connect_manager->_candidate_peer_res_list._res_list) );	
	ret_val = cm_create_pipes_from_peer_res_list( p_connect_manager, 
		&p_connect_manager->_candidate_peer_res_list, 
		FALSE,
		new_peer_pipe_num - created_pipe_num,
		&peer_pipe_num );
	CHECK_VALUE( ret_val );
	created_pipe_num += peer_pipe_num;
	LOG_DEBUG( "cm_create_active_peer_pipes from candidate list end , created pipe num = %u.", peer_pipe_num );

	sd_assert( created_pipe_num <= new_peer_pipe_num);
	if( created_pipe_num >= new_peer_pipe_num )  return SUCCESS;	
	LOG_DEBUG( "cm_create_active_peer_pipes from retry list begin, retry resource num = %u.", 
		list_size(&p_connect_manager->_retry_peer_res_list._res_list) );	
	ret_val = cm_create_pipes_from_peer_res_list( p_connect_manager, 
		&p_connect_manager->_retry_peer_res_list, 
		TRUE,
		new_peer_pipe_num - created_pipe_num,
		&peer_pipe_num );
	CHECK_VALUE( ret_val );
	created_pipe_num += peer_pipe_num;
	LOG_DEBUG( "cm_create_active_peer_pipes from retry list  end, created pipe num = %u.", peer_pipe_num );

	LOG_DEBUG( "cm_create_active_peer_pipes total create pipes num:%u.connect_manager_ptr:0x%x ",
     created_pipe_num, p_connect_manager );

	return SUCCESS;
}

_u32 cm_get_new_peer_pipe_num( CONNECT_MANAGER *p_connect_manager )
{
	_u32 new_peer_pipe_num, connecting_peer_pipe_num, connecting_pipe_num, peer_pipe_num;
    _u32 max_pipe_num = 0;
	LOG_DEBUG( "cm_get_new_peer_pipe_num begin. connect_manager_ptr:0x%x", p_connect_manager );

    if( p_connect_manager->_pipe_interface._pipe_interface_type == BT_MAGNET_INTERFACE_TYPE )
    {
        max_pipe_num = cm_magnet_max_pipe_num();
    }
    else
    {
        max_pipe_num = cm_max_pipe_num();
    }
    
	if( max_pipe_num <= p_connect_manager->_cur_pipe_num )
	{
		LOG_DEBUG( "cm_get_new_peer_pipe_num : 0, max_pipe_num:%d, cur_pipe_num:%d.",
			max_pipe_num, p_connect_manager->_cur_pipe_num );	
		return 0;
	}
	new_peer_pipe_num = max_pipe_num - p_connect_manager->_cur_pipe_num;
	
	LOG_DEBUG( "cm_get_new_peer_pipe_num limit by total pipes num:%u.", new_peer_pipe_num );	

	connecting_peer_pipe_num = list_size( &(p_connect_manager->_connecting_peer_pipe_list._data_pipe_list) );

    if( cm_max_connecting_peer_pipe_num() <  connecting_peer_pipe_num )
    {
        LOG_ERROR( "cm_get_new_peer_pipe_num warn:cm_max_connecting_peer_pipe_num,%u. connecting_peer_pipe_num:%u",
            cm_max_connecting_peer_pipe_num(),  connecting_peer_pipe_num );    
        return 0;
    }

    new_peer_pipe_num = MIN( new_peer_pipe_num, cm_max_connecting_peer_pipe_num() - connecting_peer_pipe_num );
	LOG_DEBUG( "cm_get_new_peer_pipe_num:cm_max_connecting_peer_pipe_num:%u, new_peer_pipe_num:%u.connecting_peer_pipe_num:%u.", 
     cm_max_connecting_peer_pipe_num(), new_peer_pipe_num, connecting_peer_pipe_num );	


    sd_assert(cm_max_connecting_peer_pipe_num() >= connecting_peer_pipe_num );
	LOG_DEBUG( "cm_get_new_peer_pipe_num limit by connecting peer pipes num:%u.", cm_max_connecting_peer_pipe_num() - connecting_peer_pipe_num );	
		
	peer_pipe_num = list_size( &(p_connect_manager->_working_peer_pipe_list._data_pipe_list) ) + connecting_peer_pipe_num;
	new_peer_pipe_num = MIN( new_peer_pipe_num, cm_max_peer_pipe_num() - peer_pipe_num );
	sd_assert(cm_max_peer_pipe_num() >= peer_pipe_num );
	LOG_DEBUG( "cm_get_new_peer_pipe_num limit by peer pipes num:%u.", cm_max_peer_pipe_num() - peer_pipe_num );	

	connecting_pipe_num = list_size( &(p_connect_manager->_connecting_server_pipe_list._data_pipe_list) ) + connecting_peer_pipe_num;
    if( cm_max_connecting_pipe_num() <  connecting_pipe_num )
    {
        LOG_ERROR( "cm_get_new_peer_pipe_num warn:cm_max_connecting_pipe_num,%u. connecting_pipe_num:%u",
            cm_max_connecting_pipe_num(),  connecting_pipe_num );    
        return 0;
    }

    new_peer_pipe_num = MIN( new_peer_pipe_num, cm_max_connecting_pipe_num() - connecting_pipe_num );
	sd_assert(cm_max_connecting_pipe_num() >= connecting_pipe_num  );
	LOG_DEBUG( "cm_get_new_peer_pipe_num limit by connecting pipes num:%u.", cm_max_connecting_pipe_num() - connecting_pipe_num );	
	LOG_DEBUG( "cm_get_new_peer_pipe_num limit:cur all connecting pipes num (for detail):%u.", connecting_pipe_num );	
		
	LOG_DEBUG( "cm_get_new_peer_pipe_num end, new_peer_pipe_num:%u. connect_manager_ptr:0x%x", new_peer_pipe_num, p_connect_manager );
	return new_peer_pipe_num;	
}

_int32 cm_create_pipes_from_peer_res_list( CONNECT_MANAGER *p_connect_manager, RESOURCE_LIST *p_res_list, BOOL is_retry_list, _u32 max_pipe_num, _u32 *p_create_pipe_num )
{
	_u32  peer_pipe_num = 0;
	_int32 ret_val = SUCCESS;
	RESOURCE *p_res = NULL;
    _u32 cur_time = 0;

    LOG_DEBUG( "cm_create_pipes_from_peer_res_list _is_only_using_origin_server:%d",cm_is_origin_mode(p_connect_manager));

    if(!cm_is_use_multires(p_connect_manager))
    {
        LOG_DEBUG( "cm_create_pipes_from_peer_res_list.resource: _is_only_using_origin_server is true");
        *p_create_pipe_num = 0;
        return SUCCESS;
    }

	LIST_ITERATOR next_node;
	LIST_ITERATOR cur_node = LIST_BEGIN( p_res_list->_res_list );
	LIST_ITERATOR end_node = LIST_END( p_res_list->_res_list );
	LOG_DEBUG( "cm_create_pipes_from_peer_res_list:max_pipe_num=%u. ",max_pipe_num );
	if( is_retry_list ) 
	{
      ret_val = sd_time_ms( &cur_time );
      CHECK_VALUE( ret_val );
	}
    
	while( cur_node != end_node && peer_pipe_num < max_pipe_num )
	{
		p_res = LIST_VALUE( cur_node );
		sd_assert( p_res->_resource_type == PEER_RESOURCE || p_res->_resource_type == BT_RESOURCE_TYPE || p_res->_resource_type == EMULE_RESOURCE_TYPE);		
		sd_assert( p_res->_pipe_num == 0 );
		LOG_DEBUG( "cm_create_pipes_from_peer_res_list. resource:0x%x", p_res );
        if( is_retry_list )
        {
            sd_assert( p_res->_retry_time_interval != 0 );
            sd_assert( p_res->_last_failture_time != 0 );
            LOG_DEBUG( "cm_create_pipes_from_peer_res_retry_list.resource:0x%x, cur_time:%d, _last_failture_time:%d, _retry_time_interval:%d, failture_time:%d ",
                p_res, cur_time, p_res->_last_failture_time, p_res->_retry_time_interval, cur_time - p_res->_last_failture_time );
            
            if( TIME_SUBZ( cur_time, p_res->_last_failture_time ) < p_res->_retry_time_interval * 1000 )
            {
                cur_node = LIST_NEXT( cur_node );
                continue;  
            }  
            LOG_DEBUG( "cm_create_pipes_from_peer_res_retry_list start. resource:0x%x", p_res );  
        }
        
		ret_val = cm_create_single_active_peer_pipe( p_connect_manager, p_res );
		if( ret_val == SUCCESS )
		{
			LOG_DEBUG( "push resource:0x%x into _using_peer_res_list, resource type = %d, p_connect_manager = 0x%x, resource list ptr:0x%x", 
				p_res, p_res->_resource_type, p_connect_manager, &p_connect_manager->_using_peer_res_list );
			ret_val = list_push( &(p_connect_manager->_using_peer_res_list._res_list), p_res ); 
			CHECK_VALUE( ret_val );
			peer_pipe_num++;
		}
		else
		{
			sd_assert( p_res->_pipe_num == 0 );
			ret_val = list_push( &(p_connect_manager->_discard_peer_res_list._res_list), p_res ); 
			CHECK_VALUE( ret_val );			
		}

		next_node = LIST_NEXT( cur_node );
		ret_val = list_erase( &p_res_list->_res_list, cur_node );
		CHECK_VALUE( ret_val );
		cur_node = next_node;	
	}

	*p_create_pipe_num = peer_pipe_num;
	return SUCCESS;
}

_int32 cm_create_single_active_peer_pipe( CONNECT_MANAGER *p_connect_manager, RESOURCE *p_res )
{
	_int32 ret_val = SUCCESS;
	DATA_PIPE *p_pipe = NULL;

	LOG_DEBUG( "cm_create_single_active_peer_pipe, res_type = %d", p_res->_resource_type);
	sd_assert( p_res->_resource_type == PEER_RESOURCE || p_res->_resource_type == BT_RESOURCE_TYPE || p_res->_resource_type == EMULE_RESOURCE_TYPE);

	LOG_DEBUG( "cm create peer pipe. resource:0x%x", p_res );	

	if( p_res->_resource_type == PEER_RESOURCE )
	{
		P2P_RESOURCE* p2p_res = (P2P_RESOURCE* )p_res;

		if(p_connect_manager->_main_task_ptr != NULL)
		{
			if(cm_is_cdn_peer_res_exist_by_peerid(&p_connect_manager->_main_task_ptr->_connect_manager
				, p2p_res->_peer_id))
			{
				LOG_DEBUG( "cm_create_single_active_peer_pipee. resource:0x%x, has cdn res just return ...", p_res );	
				return -1;
			}
		}
		
	
		ret_val = p2p_pipe_create( p_connect_manager->_data_manager_ptr, p_res, &p_pipe, NULL );
	   	if( ret_val != SUCCESS )
	   	{
			LOG_ERROR( "cm create peer pipe error. error code:%d.", ret_val );
			return ret_val;
	   	}		
	   	LOG_DEBUG("p2p_pipe_create success, pipe : 0x%x, res_level = %d, res_priority = %d"
	   	    , p_pipe, p2p_get_res_level(p_res), p2p_get_res_priority(p_res) );
	}
	
#ifdef ENABLE_BT  
#ifdef ENABLE_BT_PROTOCOL

	else if( p_res->_resource_type == BT_RESOURCE_TYPE)
	{
		ret_val = bt_pipe_create( p_connect_manager->_data_manager_ptr, p_res, &p_pipe );
	   	if( ret_val != SUCCESS )
	   	{
			LOG_ERROR( "cm create bt pipe error. error code:%d.", ret_val );
			return ret_val;
	   	}		
        if( p_connect_manager->_pipe_interface._pipe_interface_type == BT_MAGNET_INTERFACE_TYPE )
        {
            bt_pipe_set_magnet_mode(p_pipe);
        }
	}
#endif /* #ifdef ENABLE_BT  */
#endif /* #ifdef ENABLE_BT  */

#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL

	else if(p_res->_resource_type == EMULE_RESOURCE_TYPE)
	{
		ret_val = emule_pipe_create(&p_pipe, p_connect_manager->_data_manager_ptr, p_res);
		if(ret_val != SUCCESS)
		{
			LOG_ERROR("cm create emule pipe error, errcode = %d.", ret_val);
			return ret_val;
		}
	}
#endif
#endif

    if( p_connect_manager->_pipe_interface._pipe_interface_type != BT_MAGNET_INTERFACE_TYPE )
    {
	dp_set_pipe_interface( p_pipe, &p_connect_manager->_pipe_interface );
    }

	p_connect_manager->_cur_pipe_num++;
	p_res->_pipe_num++;	

	ret_val = gcm_register_pipe( p_connect_manager, p_pipe );
	CHECK_VALUE( ret_val );
	
	LOG_DEBUG( "cm open peer pipe. resource:0x%x, pipe:0x%x", p_res, p_pipe );
	if( p_res->_resource_type == PEER_RESOURCE )
	{
		ret_val = p2p_pipe_open(  p_pipe );
	}
#ifdef ENABLE_BT  
#ifdef ENABLE_BT_PROTOCOL

	else if( p_res->_resource_type == BT_RESOURCE_TYPE )
	{
		ret_val = bt_pipe_open( p_pipe );	
	}
#endif /* #ifdef ENABLE_BT  */	
#endif /* #ifdef ENABLE_BT  */	

#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL
	else if(p_res->_resource_type == EMULE_RESOURCE_TYPE)
	{
		ret_val = emule_pipe_open(p_pipe);
	}
#endif
#endif

	if( ret_val != SUCCESS )
	{
		LOG_ERROR( "cm open peer pipe error. error code:%d.", ret_val );
		cm_destroy_single_pipe( p_connect_manager, p_pipe );
		return ret_val;	
	}
    
	p_res->_connecting_pipe_num++;	
	ret_val = list_push( &p_connect_manager->_connecting_peer_pipe_list._data_pipe_list, p_pipe );
	CHECK_VALUE( ret_val );

    LOG_DEBUG( "cm_create_single_active_peer_pipe. _connecting_peer_pipe_list_ptr:0x%x, list_size:%u, [dispatch_stat] pipe 0x%x create 0x%x (resource).",
        &p_connect_manager->_connecting_peer_pipe_list._data_pipe_list, 
        list_size(&p_connect_manager->_connecting_peer_pipe_list._data_pipe_list), 
        p_pipe, p_res);
    
	return SUCCESS;
}

_u32 cm_idle_res_num( CONNECT_MANAGER *p_connect_manager )
{
	_u32 idle_res_num = 0;
	idle_res_num = list_size( &p_connect_manager->_idle_server_res_list._res_list );
	LOG_DEBUG( "cm_idle_res_num. server_idle_res num:%d", idle_res_num );
	idle_res_num += list_size( &p_connect_manager->_idle_peer_res_list._res_list );
	LOG_DEBUG( "cm_idle_res_num. total_idle_res num:%d", idle_res_num );
	return idle_res_num;
}

_u32 cm_retry_res_num( CONNECT_MANAGER *p_connect_manager )
{
	_u32 retry_res_num = 0;
	retry_res_num = list_size( &p_connect_manager->_retry_server_res_list._res_list );
	LOG_DEBUG( "cm_retry_res_num. server_retry_res_num:%d", retry_res_num );
	retry_res_num += list_size( &p_connect_manager->_retry_peer_res_list._res_list );
	LOG_DEBUG( "cm_retry_res_num. total_retry_res_num:%d", retry_res_num );
	return retry_res_num;
}

void cm_update_idle_status( CONNECT_MANAGER *p_connect_manager )
{
	_u32 total_speed = 0;

	total_speed = p_connect_manager->_cur_server_speed + p_connect_manager->_cur_peer_speed;
	if( total_speed == 0 
#ifdef ENABLE_CDN
		&& p_connect_manager->_max_cdn_speed == 0
#endif
		&& cm_idle_res_num( p_connect_manager ) == 0 
		&& cm_retry_res_num( p_connect_manager ) == 0 		
		&& !p_connect_manager->_is_err_get_buffer_status )
	{
		p_connect_manager->_status_ticks++;
	}
	else
	{
		p_connect_manager->_status_ticks = 0;
	}
	LOG_DEBUG( "cm_update_idle_status ticks:%d._is_err_get_buffer_status:%d, total_speed:%d", 
		p_connect_manager->_status_ticks, p_connect_manager->_is_err_get_buffer_status, total_speed );	
	if( p_connect_manager->_status_ticks > cm_status_idle_ticks() )
	{
		p_connect_manager->_is_idle_status = TRUE;
	}
}

_int32 cm_update_server_pipe_list( CONNECT_MANAGER *p_connect_manager )
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "##########cm_update_server_pipe_list." );

	LOG_DEBUG( "###cm_update_server_working_pipes to handle failture pipe." );
	ret_val = cm_update_failture_pipes( p_connect_manager,
		&p_connect_manager->_working_server_pipe_list,
		&p_connect_manager->_using_server_res_list,
		&p_connect_manager->_retry_server_res_list,
		&p_connect_manager->_discard_server_res_list,
		FALSE );
	CHECK_VALUE( ret_val );	

	LOG_DEBUG( "###cm_update_server_connecting_pipes to handle failture pipe." );	
	ret_val = cm_update_failture_pipes( p_connect_manager,
		&p_connect_manager->_connecting_server_pipe_list,
		&p_connect_manager->_using_server_res_list,
		&p_connect_manager->_retry_server_res_list,
		&p_connect_manager->_discard_server_res_list,
		TRUE );
	CHECK_VALUE( ret_val );	
	
	//cm_update_failture_pipes must before cm_update_to_connecting_pipes
	LOG_DEBUG( "###cm_update_to_connecting_pipes." );	
	ret_val = cm_update_to_connecting_pipes( p_connect_manager, &p_connect_manager->_connecting_server_pipe_list,
		&p_connect_manager->_working_server_pipe_list );
	CHECK_VALUE( ret_val );	
	
	return SUCCESS;
}

_int32 cm_update_peer_pipe_list( CONNECT_MANAGER *p_connect_manager )
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "##########cm_update_peer_pipe_list" );
	
	LOG_DEBUG( "###cm_update_peer_working_pipes to handle failture pipe." );
	ret_val = cm_update_failture_pipes( p_connect_manager,
		&p_connect_manager->_working_peer_pipe_list,
		&p_connect_manager->_using_peer_res_list,
		&p_connect_manager->_retry_peer_res_list,
		&p_connect_manager->_discard_peer_res_list,
		FALSE );
	CHECK_VALUE( ret_val );	
	
	LOG_DEBUG( "###cm_update_peer_connecting_pipes to handle failture pipe." );	
	ret_val = cm_update_failture_pipes( p_connect_manager,
		&p_connect_manager->_connecting_peer_pipe_list,
		&p_connect_manager->_using_peer_res_list,
		&p_connect_manager->_retry_peer_res_list,
		&p_connect_manager->_discard_peer_res_list,
		TRUE );
	CHECK_VALUE( ret_val );	
	
	//cm_update_failture_pipes must before cm_update_to_connecting_pipes
	LOG_DEBUG( "###cm_update_to_connecting_pipes." );	
	ret_val = cm_update_to_connecting_pipes( p_connect_manager, &p_connect_manager->_connecting_peer_pipe_list,
		&p_connect_manager->_working_peer_pipe_list );
	CHECK_VALUE( ret_val );	
	
	return SUCCESS;	
}

_int32 cm_update_failture_pipes( CONNECT_MANAGER *p_connect_manager,
	PIPE_LIST *p_pipe_list, 
	RESOURCE_LIST *p_using_res_list,
	RESOURCE_LIST *p_retry_res_list,
	RESOURCE_LIST *p_discard_res_list,
	BOOL is_connecting_list )
{
	_int32 ret_val = SUCCESS;
	
	LIST_ITERATOR next_node;
	LIST_ITERATOR cur_node = LIST_BEGIN( p_pipe_list->_data_pipe_list );
	LIST_ITERATOR end_node = LIST_END( p_pipe_list->_data_pipe_list );

	LOG_DEBUG( "cm_update_failture_pipes begin. connect_manager_ptr:0x%x, target pipe list num = %u", 
        p_connect_manager, list_size(&p_pipe_list->_data_pipe_list) );
    if (list_size(&(p_pipe_list->_data_pipe_list)) != 0)
    {
        while( cur_node != end_node )
        {
            DATA_PIPE *p_data_pipe = LIST_VALUE( cur_node );
            RESOURCE *p_res = p_data_pipe->_p_resource;

			if (p_data_pipe->_dispatch_info._pipe_state > PIPE_FAILURE 
               #ifdef _DEBUG
                            && p_data_pipe->_dispatch_info._old_state > PIPE_FAILURE
               #endif
                            ) {
					next_node = LIST_NEXT( cur_node );
					list_erase( &p_pipe_list->_data_pipe_list, cur_node );
					cur_node = next_node;
					continue;
			}
                
            if( p_data_pipe->_dispatch_info._pipe_state != PIPE_FAILURE 
				&& (p_res != NULL && p_res->_dispatch_state != ABANDON ))
            {
                cur_node = LIST_NEXT( cur_node );
                continue;
            }
          
            
            LOG_DEBUG( "cm_destroy_single_pipe: _pipe_state:%d, resource_state:%d", 
                p_data_pipe->_dispatch_info._pipe_state, 
                p_res->_dispatch_state );		
            ret_val = cm_destroy_single_pipe( p_connect_manager, p_data_pipe );
            CHECK_VALUE( ret_val );

            next_node = LIST_NEXT( cur_node );
            list_erase( &p_pipe_list->_data_pipe_list, cur_node );
            cur_node = next_node;
			if (p_res == NULL)
			{
				sd_assert( p_res->_pipe_num != 0 );
				continue;
			}
            if( is_connecting_list )
            {
                p_res->_connecting_pipe_num--;
            }

            if( p_res->_dispatch_state == ABANDON )
            {
                if( p_res->_pipe_num == 0 )
                {
                    LOG_DEBUG( "cm_move_res to discard list: resource:0x%x", p_res );		
                    ret_val = cm_move_res( p_using_res_list, p_discard_res_list, p_res );
                    CHECK_VALUE( ret_val );
                }
            }
            else 
            {
                if( p_res->_pipe_num == 0 && p_res->_retry_times < p_res->_max_retry_time )
                {
                    LOG_DEBUG( "cm_move_res to retry list: resource:0x%x, retry_times:%u, max_retry_time:%u ",
                        p_res, p_res->_retry_times, p_res->_max_retry_time );
                    
                    cm_calc_res_retry_score( p_retry_res_list, p_res ); 

                    ret_val = cm_move_res_to_order_list( p_using_res_list, p_retry_res_list, p_res );
                    CHECK_VALUE( ret_val );	

                }
                if( p_res->_pipe_num == 0 && p_res->_retry_times >= p_res->_max_retry_time )
                {
                    LOG_DEBUG( "cm_move_res to discard list: resource:0x%x, retry_times:%u, max_retry_time:%u ",
                        p_res, p_res->_retry_times, p_res->_max_retry_time );
                    ret_val = cm_move_res( p_using_res_list, p_discard_res_list, p_res );
                    CHECK_VALUE( ret_val );	
                }
                
                //p_res->_retry_times++;
                cm_calc_res_retry_para( p_connect_manager, p_res ); 

                ret_val = sd_time_ms(&p_res->_last_failture_time);
                CHECK_VALUE( ret_val ); 
            }	
        }
    }
	LOG_DEBUG( "cm_update_failture_pipes end. target pipe list num = %u", list_size(&p_pipe_list->_data_pipe_list) );

	return SUCCESS;		
}


_int32 cm_create_special_pipes(CONNECT_MANAGER* p_connect_manager)
{
    LIST_ITERATOR cur = LIST_BEGIN(p_connect_manager->_idle_peer_res_list._res_list);
    LIST_ITERATOR end = LIST_END(p_connect_manager->_idle_peer_res_list._res_list);
    LIST_ITERATOR tmp = NULL;
    RESOURCE* res = NULL;
    _u8 capability = 0;
    _u8 from = 0;
    _int32 ret = SUCCESS;
    if (list_size(&(p_connect_manager->_idle_peer_res_list._res_list)) != 0)
    {
        while(cur != end)
        {
            res = LIST_VALUE(cur);

        //BT and EM task also push into the _idle_peer_res_list._res_list	
        if(res->_resource_type != PEER_RESOURCE)
            {
            cur = LIST_NEXT(cur);
            continue;
        }
            
        capability = p2p_get_capability(res);
            from = p2p_get_from(res);
            if( is_same_nat(capability) )
            {
                LOG_DEBUG("same nat peer created, res = 0x%x", res);
                ret = cm_create_single_active_peer_pipe(p_connect_manager, res);
                if(ret==SUCCESS)
                {
                    list_push(&p_connect_manager->_using_peer_res_list._res_list, res);
                }
                else
                {
                    list_push(&p_connect_manager->_discard_peer_res_list._res_list, res);
                }
                tmp = cur;
                cur = LIST_NEXT(cur);
                list_erase(&p_connect_manager->_idle_peer_res_list._res_list, tmp);
            }
            else
            {
                cur = LIST_NEXT(cur);
            }
        }
    }
    return SUCCESS;
}


void cm_calc_res_retry_para( CONNECT_MANAGER *p_connect_manager, RESOURCE *p_res )
{
	
   /* if( p_res->_resource_type == PEER_RESOURCE )
    {
        p_res->_max_retry_time = cm_max_peer_res_retry_times();
    }
    else if( p_res->_resource_type == HTTP_RESOURCE || p_res->_resource_type == FTP_RESOURCE )
    {
        p_res->_max_retry_time = cm_max_server_res_retry_times();
    } 
    else
    {
        sd_assert( FALSE );
    }
    */

    sd_assert( p_res->_err_code != SUCCESS );
   
    switch( p_res->_err_code )
    {
        case ERR_P2P_UPLOAD_OVER_MAX:
            p_res->_retry_time_interval = cm_pipe_retry_interval_time();
            p_res->_max_retry_time = MAX_U32;
            break;
		case HTTP_ERR_503_LIMIT_EXCEEDED:
			p_res->_retry_times++;			
			p_res->_retry_time_interval = p_res->_retry_times * cm_pipe_retry_interval_time();
            p_res->_max_retry_time = cm_max_res_retry_times() * 3;
            break;
        default:
            if(p_res->_resource_type == HTTP_RESOURCE || p_res->_resource_type == FTP_RESOURCE)
            {
                if (p_res->_max_speed == 0)
                {
                    p_res->_retry_times++;	
                }
            }
            else
            {
                p_res->_retry_times++;
            }
            p_res->_retry_time_interval += cm_pipe_retry_interval_time();
			p_res->_max_retry_time = cm_max_res_retry_times();
			break;
    }

    if( p_res == p_connect_manager->_origin_res_ptr )
    {
       p_res->_max_retry_time = MAX_U32; //cm_max_orgin_res_retry_times();
       if(p_res->_retry_time_interval > 120) // 原始资源最大10s重试一次
           p_res->_retry_time_interval = 120;
    }    	
	
	LOG_DEBUG( "cm_calc_res_retry_para.res:0x%x, _retry_time_interval =%u, _max_retry_time:%d, _retry_times:%d, err_code:%u",
     p_res, p_res->_retry_time_interval, p_res->_max_retry_time, p_res->_retry_times, p_res->_err_code );
}

void cm_calc_res_retry_score( RESOURCE_LIST *p_retry_res_list,RESOURCE *p_res )
{
	LIST_ITERATOR cur_node = NULL;
	RESOURCE *p_list_res = NULL;
	_u32 score_temp = 0;

	if( 0 == list_size( &p_retry_res_list->_res_list ) )
	{
		p_res->_score = cm_retry_res_init_score();
	}
	else
	{
		cur_node = LIST_RBEGIN( p_retry_res_list->_res_list );
		p_list_res = LIST_VALUE( cur_node );
		p_res->_score = p_list_res->_score + 1;		
	}

	score_temp = p_res->_max_speed / cm_retry_res_score_ratio();
	
	if( p_res->_score > score_temp )
	{
		p_res->_score -= score_temp;
	}
	else
	{
		p_res->_score = 0;
	}
	
	LOG_DEBUG( "cm_calc_res_retry_score.res:0x%x, score =%u, _max_speed:%d",
		p_res, p_res->_score, p_res->_max_speed );
}

_int32 cm_update_to_connecting_pipes( CONNECT_MANAGER *p_connect_manager, PIPE_LIST *p_connecting_pipe_list, PIPE_LIST *p_working_pipe_list )
{
	_int32 ret_val = SUCCESS;
	LIST_ITERATOR next_node;
	LIST_ITERATOR cur_node = LIST_BEGIN( p_connecting_pipe_list->_data_pipe_list );
	LIST_ITERATOR end_node = LIST_END( p_connecting_pipe_list->_data_pipe_list );

	LOG_DEBUG( "cm_update_to_connecting_pipes begin. connect_manager_ptr:0x%x, connecting_pipe_list ptr:0x%x, connecting pipe num = %u, working pipe num = %u", 
        p_connect_manager,
        &p_connecting_pipe_list->_data_pipe_list,
		list_size(&p_connecting_pipe_list->_data_pipe_list),
		list_size(&p_working_pipe_list->_data_pipe_list) );
    if (list_size(&(p_connecting_pipe_list->_data_pipe_list)) != 0)
    {
        while( cur_node != end_node )
        {
            DATA_PIPE *p_data_pipe = LIST_VALUE( cur_node );
            LOG_DEBUG( "connecting_pipes. pipe state = %u, pipe_ptr:0x%x, resource_ptr:0x%x.", 
                p_data_pipe->_dispatch_info._pipe_state, p_data_pipe, p_data_pipe->_p_resource );

            sd_assert( p_data_pipe->_dispatch_info._pipe_state != PIPE_IDLE
                && p_data_pipe->_dispatch_info._pipe_state != PIPE_FAILURE );
            
            if( p_data_pipe->_dispatch_info._pipe_state != PIPE_CONNECTING )
            {
                next_node = LIST_NEXT( cur_node );
                ret_val = list_erase( &(p_connecting_pipe_list->_data_pipe_list), cur_node );
                CHECK_VALUE( ret_val );
                
#ifdef EMBED_REPORT
                if( p_data_pipe->_p_resource->_retry_times == 0
                    && p_data_pipe->_p_resource->_dispatch_state == NEW )
                {
                    cm_valid_res_report_para( p_connect_manager, p_data_pipe->_p_resource );
                }
#endif
                p_data_pipe->_p_resource->_dispatch_state = VALID;
                p_data_pipe->_p_resource->_retry_times = 0;
                p_data_pipe->_p_resource->_retry_time_interval = 0;
                LOG_DEBUG( "connecting_pipes. _retry_time_interval = %u, pipe_ptr:0x%x, resource_ptr:0x%x.", 
                    p_data_pipe->_p_resource->_retry_time_interval, p_data_pipe, p_data_pipe->_p_resource );

                ret_val = list_push( &(p_working_pipe_list->_data_pipe_list), p_data_pipe );
                CHECK_VALUE( ret_val );
                cur_node = next_node;
                p_data_pipe->_p_resource->_connecting_pipe_num--;
                LOG_DEBUG( "move pipe from connecting list to working list. pipe state = %u, pipe_ptr:0x%x, resource_ptr:0x%x.", 
                    p_data_pipe->_dispatch_info._pipe_state, p_data_pipe, p_data_pipe->_p_resource );

                ret_val = gcm_register_working_pipe( p_connect_manager, p_data_pipe );
                CHECK_VALUE( ret_val );
                
                continue;
            }

            cur_node = LIST_NEXT( cur_node );
        }
    }
	
	cur_node = LIST_BEGIN( p_working_pipe_list->_data_pipe_list );
	end_node = LIST_END( p_working_pipe_list->_data_pipe_list );
	if (list_size(&(p_working_pipe_list->_data_pipe_list)) !=0)
    {
        while( cur_node != end_node )
        {
            DATA_PIPE *p_data_pipe = LIST_VALUE( cur_node );
            LOG_DEBUG( "working_pipes. pipe state = %u, pipe_ptr:0x%x, resource_ptr:0x%x.", 
                p_data_pipe->_dispatch_info._pipe_state, p_data_pipe, p_data_pipe->_p_resource );
            sd_assert( p_data_pipe->_dispatch_info._pipe_state != PIPE_IDLE
                && p_data_pipe->_dispatch_info._pipe_state != PIPE_FAILURE );
            
            if( p_data_pipe->_dispatch_info._pipe_state == PIPE_CONNECTING )
            {
                next_node = LIST_NEXT( cur_node );
                ret_val = list_erase( &(p_working_pipe_list->_data_pipe_list), cur_node );
                CHECK_VALUE( ret_val );

                ret_val = list_push( &(p_connecting_pipe_list->_data_pipe_list), p_data_pipe );
                CHECK_VALUE( ret_val );
                cur_node = next_node;
                p_data_pipe->_p_resource->_connecting_pipe_num++;
                
                LOG_DEBUG( "move pipe from working list to connecting list. pipe state = %u, pipe_ptr:0x%x, resource_ptr:0x%x.", 
                    p_data_pipe->_dispatch_info._pipe_state, p_data_pipe, p_data_pipe->_p_resource );
                continue;
            }
            cur_node = LIST_NEXT( cur_node );
        }
    }

	LOG_DEBUG( "cm_update_to_connecting_pipes end. connect_manager_ptr:0x%x, connecting pipe num = %u, working pipe num = %u",
		p_connect_manager, list_size(&p_connecting_pipe_list->_data_pipe_list), list_size(&p_working_pipe_list->_data_pipe_list) );
	return SUCCESS;		
}

_int32 cm_move_res( RESOURCE_LIST *p_src_res_list, RESOURCE_LIST *p_dest_res_list, RESOURCE *p_target_res )
{
	_int32 ret_val = SUCCESS;
	LIST_ITERATOR cur_node = LIST_BEGIN( p_src_res_list->_res_list );
	LIST_ITERATOR end_node = LIST_END( p_src_res_list->_res_list );
	LOG_DEBUG( "cm_move_res: resource:0x%x", p_target_res );

    if (list_size(&(p_src_res_list->_res_list)) != 0)
    {
        while( cur_node != end_node )
        {
            RESOURCE *p_res = LIST_VALUE( cur_node );
            if( p_res == p_target_res )
            {
                LOG_DEBUG( "push_res: resource:0x%x, resource list ptr:0x%x", p_target_res, p_dest_res_list );
                ret_val = list_push( &p_dest_res_list->_res_list, p_res );
                CHECK_VALUE( ret_val );	
                
                LOG_DEBUG( "erase_res: resource:0x%x, resource list ptr:0x%x", p_src_res_list, p_dest_res_list );
                ret_val = list_erase( &p_src_res_list->_res_list, cur_node );
                CHECK_VALUE( ret_val );

                return SUCCESS;
            }
            cur_node = LIST_NEXT( cur_node );
        }
    }
	sd_assert( FALSE );
	return SUCCESS;
}

_int32 cm_move_res_to_order_list( RESOURCE_LIST *p_src_res_list, RESOURCE_LIST *p_dest_res_list, RESOURCE *p_target_res )
{
	_int32 ret_val = SUCCESS;
	LIST_ITERATOR src_cur_node = LIST_BEGIN( p_src_res_list->_res_list );
	LIST_ITERATOR src_end_node = LIST_END( p_src_res_list->_res_list );
	LIST_ITERATOR dest_cur_node = LIST_BEGIN( p_dest_res_list->_res_list );
	LIST_ITERATOR dest_end_node = LIST_END( p_dest_res_list->_res_list );
    RESOURCE *p_src_res = NULL;
    RESOURCE *p_dest_res = NULL;
	LOG_DEBUG( "cm_move_res_to_order_list: resource:0x%x", p_target_res );

	while( src_cur_node != src_end_node )
	{
		p_src_res = LIST_VALUE( src_cur_node );

		if( p_src_res == p_target_res )
		{
  			LOG_DEBUG( "erase_res: resource:0x%x, resource list ptr:0x%x", p_src_res_list, p_dest_res_list );
			ret_val = list_erase( &p_src_res_list->_res_list, src_cur_node );
			CHECK_VALUE( ret_val );  

            while( dest_cur_node != dest_end_node )
            {
                p_dest_res = LIST_VALUE( dest_cur_node );
                if( p_src_res->_score < p_dest_res->_score )
                {
                    ret_val = list_insert( &p_dest_res_list->_res_list, (void *)p_target_res, dest_cur_node );
                    break;
                }
                dest_cur_node = LIST_NEXT( dest_cur_node );
            }
            
			if( dest_cur_node ==  dest_end_node )
			{
                LOG_DEBUG( "push_res: resource:0x%x, resource list ptr:0x%x", p_target_res, p_dest_res_list );
                ret_val = list_insert( &p_dest_res_list->_res_list, (void *)p_target_res, dest_cur_node );
    			CHECK_VALUE( ret_val );  
			}
			return SUCCESS;
		}
		src_cur_node = LIST_NEXT( src_cur_node );
	}	
	sd_assert( FALSE );
	return SUCCESS;
}

_int32 cm_destroy_all_discard_ress( CONNECT_MANAGER *p_connect_manager )
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "cm_destroy_all_discard_ress." );

	LOG_DEBUG( "cm_destroy_discard_server_ress. list_size:%d", list_size(&p_connect_manager->_discard_server_res_list._res_list) );	
	ret_val = cm_destroy_discard_res_list( p_connect_manager
	        , &(p_connect_manager->_discard_server_res_list) );
	CHECK_VALUE( ret_val );	
	
	LOG_DEBUG( "cm_destroy_discard_peer_ress. list_size:%d", list_size(&p_connect_manager->_discard_peer_res_list._res_list) );	
	ret_val = cm_destroy_discard_res_list( p_connect_manager
	        , &(p_connect_manager->_discard_peer_res_list) );
	CHECK_VALUE( ret_val );	
	
	return SUCCESS;
}

_int32 cm_destroy_all_ress( CONNECT_MANAGER *p_connect_manager )
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "cm_destroy_all_ress." );

	ret_val = cm_destroy_all_server_ress( p_connect_manager );
	CHECK_VALUE( ret_val );
	
	ret_val = cm_destroy_all_peer_ress( p_connect_manager );
	CHECK_VALUE( ret_val );

#ifdef ENABLE_CDN
	ret_val = cm_destroy_all_cdn_ress( p_connect_manager );
	CHECK_VALUE( ret_val );
#endif
	return SUCCESS;
}

_int32 cm_destroy_all_server_ress( CONNECT_MANAGER *p_connect_manager )
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "cm_destroy_all_server_ress." );

	LOG_DEBUG( "cm_destroy_idle_server_ress. list_size:%d", list_size(&p_connect_manager->_idle_server_res_list._res_list) );	
	ret_val = cm_destroy_res_list( p_connect_manager, &(p_connect_manager->_idle_server_res_list) );
	CHECK_VALUE( ret_val );

	LOG_DEBUG( "cm_destroy_using_server_ress. list_size:%d", list_size(&p_connect_manager->_using_server_res_list._res_list) );
	ret_val = cm_destroy_res_list( p_connect_manager, &(p_connect_manager->_using_server_res_list) );
	CHECK_VALUE( ret_val );

	LOG_DEBUG( "cm_destroy_candidate_server_ress. list_size:%d", list_size(&p_connect_manager->_candidate_server_res_list._res_list) );
	ret_val = cm_destroy_res_list( p_connect_manager, &(p_connect_manager->_candidate_server_res_list) );
	CHECK_VALUE( ret_val );
	
	LOG_DEBUG( "cm_destroy_retry_server_ress. list_size:%d", list_size(&p_connect_manager->_retry_server_res_list._res_list) );	
	ret_val = cm_destroy_res_list( p_connect_manager, &(p_connect_manager->_retry_server_res_list) );
	CHECK_VALUE( ret_val );
	
	LOG_DEBUG( "cm_destroy_discard_server_ress. list_size:%d", list_size(&p_connect_manager->_discard_server_res_list._res_list) );	
	ret_val = cm_destroy_res_list( p_connect_manager, &(p_connect_manager->_discard_server_res_list) );
	CHECK_VALUE( ret_val );	
	
	return SUCCESS;
}

_int32 cm_destroy_all_peer_ress( CONNECT_MANAGER *p_connect_manager )
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "cm_destroy_all_peer_ress." );
	
	LOG_DEBUG( "cm_destroy_idle_peer_ress. list_size:%d", list_size(&p_connect_manager->_idle_peer_res_list._res_list) );	
	ret_val = cm_destroy_res_list( p_connect_manager, &(p_connect_manager->_idle_peer_res_list) );
	CHECK_VALUE( ret_val );
	
	LOG_DEBUG( "cm_destroy_using_peer_ress. list_size:%d", list_size(&p_connect_manager->_using_peer_res_list._res_list) );	
	ret_val = cm_destroy_res_list( p_connect_manager, &(p_connect_manager->_using_peer_res_list) );
	CHECK_VALUE( ret_val );
	
	LOG_DEBUG( "cm_destroy_candidate_peer_ress. list_size:%d", list_size(&p_connect_manager->_candidate_peer_res_list._res_list) );	
	ret_val = cm_destroy_res_list( p_connect_manager, &(p_connect_manager->_candidate_peer_res_list) );
	CHECK_VALUE( ret_val );
	
	LOG_DEBUG( "cm_destroy_retry_peer_ress. list_size:%d", list_size(&p_connect_manager->_retry_peer_res_list._res_list) );	
	ret_val = cm_destroy_res_list( p_connect_manager, &(p_connect_manager->_retry_peer_res_list) );
	CHECK_VALUE( ret_val );
	
	LOG_DEBUG( "cm_destroy_discard_peer_ress. list_size:%d", list_size(&p_connect_manager->_discard_peer_res_list._res_list) );	
	ret_val = cm_destroy_res_list( p_connect_manager, &(p_connect_manager->_discard_peer_res_list) );
	CHECK_VALUE( ret_val );	
	
	return SUCCESS;
}

_int32 cm_destroy_discard_res_list( CONNECT_MANAGER *p_connect_manager
        , RESOURCE_LIST *p_res_list )
{
	_int32 ret_val = SUCCESS;
	LIST_ITERATOR cur_node, end_node, next_node;
	LOG_DEBUG( "cm_destroy_discard_res_list. list_ptr:0x%x", p_res_list );

	cur_node = LIST_BEGIN( p_res_list->_res_list );
	end_node = LIST_END( p_res_list->_res_list );
    if (list_size(&(p_res_list->_res_list)) != 0)
    {
        while( cur_node != end_node )
        {
            RESOURCE *p_res = LIST_VALUE( cur_node );
            BOOL can_destroy = TRUE;
            if( p_connect_manager->_call_back_ptr != NULL )
            {
                can_destroy = p_connect_manager->_call_back_ptr( p_connect_manager->_data_manager_ptr, p_res );
            }
            LOG_DEBUG( "cm_destroy_discard_res_list. res_ptr:0x%x", p_res );

            if( p_res == p_connect_manager->_origin_res_ptr || !can_destroy ) 
            {
                cur_node = LIST_NEXT( cur_node );
                continue;
            }

            // 先删除res所拥有的相关pipe
            ret_val = cm_destroy_server_pipes_owned_by_res(p_connect_manager, p_res);
            CHECK_VALUE( ret_val );
            
            ret_val = cm_destroy_res( p_connect_manager, p_res );
            CHECK_VALUE( ret_val );
            
            next_node = LIST_NEXT( cur_node );
            
            ret_val = list_erase( &p_res_list->_res_list, cur_node );
            CHECK_VALUE( ret_val );

            cur_node = next_node;
        }
    }
	
	return SUCCESS;	
}

_int32 cm_destroy_res_list( CONNECT_MANAGER *p_connect_manager
        , RESOURCE_LIST *p_res_list )
{
	_int32 ret_val = SUCCESS;
	LIST_ITERATOR cur_node, end_node, next_node;
	LOG_DEBUG( "cm_destroy_res_list. list_ptr:0x%x", p_res_list );

	cur_node = LIST_BEGIN( p_res_list->_res_list );
	end_node = LIST_END( p_res_list->_res_list );

	while( cur_node != end_node )
	{
		RESOURCE *p_res = LIST_VALUE( cur_node );

		ret_val = cm_destroy_res( p_connect_manager, p_res );
		CHECK_VALUE( ret_val );
		
		next_node = LIST_NEXT( cur_node );
		
		ret_val = list_erase( &p_res_list->_res_list, cur_node );
		CHECK_VALUE( ret_val );

		cur_node = next_node;
	}	
	
	return SUCCESS;	
}

_int32 cm_destroy_res( CONNECT_MANAGER *p_connect_manager, RESOURCE *p_res )
{
	_int32 ret_val = SUCCESS;
	RES_WRAP *p_res_wrap = NULL;
	
    LOG_DEBUG( "cm_destroy_res,[dispatch_stat] resource 0x%x delete", p_res);
	sd_assert( p_res->_pipe_num == 0 );
	sd_assert( p_res->_connecting_pipe_num == 0 );
	if( p_res == p_connect_manager->_origin_res_ptr )
	{
		p_connect_manager->_origin_res_ptr = NULL;
	}

	p_res_wrap = (RES_WRAP *)p_res->_global_wrap_ptr;
	if( p_res_wrap != NULL )
	{
		p_res_wrap->_is_deleted = TRUE;		
	}	

	if( p_res->_resource_type == HTTP_RESOURCE )
	{
		cm_move_hash_map_res( &p_connect_manager->_server_hash_value_map, 
			&p_connect_manager->_destoryed_server_res_hash_value_map, p_res );

		ret_val = http_resource_destroy( &p_res );
		CHECK_VALUE( ret_val );
        
#ifdef _CONNECT_DETAIL
        p_connect_manager->_server_destroy_num++;
#endif
       
	}
	else if( p_res->_resource_type == FTP_RESOURCE )
	{
		cm_move_hash_map_res( &p_connect_manager->_server_hash_value_map, 
			&p_connect_manager->_destoryed_server_res_hash_value_map, p_res );

		ret_val = ftp_resource_destroy( &p_res );
		CHECK_VALUE( ret_val );	
        
#ifdef _CONNECT_DETAIL        
        p_connect_manager->_server_destroy_num++;    
#endif        
	}
	else if( p_res->_resource_type == PEER_RESOURCE )
	{
		cm_move_hash_map_res( &p_connect_manager->_peer_hash_value_map, 
			&p_connect_manager->_destoryed_peer_res_hash_value_map, p_res );
		
		ret_val = p2p_resource_destroy( &p_res );
		CHECK_VALUE( ret_val );	
#ifdef _CONNECT_DETAIL        
        p_connect_manager->_peer_destroy_num++;    
#endif      
	}


#ifdef ENABLE_BT  
#ifdef ENABLE_BT_PROTOCOL

	else if( p_res->_resource_type == BT_RESOURCE_TYPE )
	{
		cm_move_hash_map_res( &p_connect_manager->_peer_hash_value_map, 
			&p_connect_manager->_destoryed_peer_res_hash_value_map, p_res );
				
		ret_val = bt_resource_destroy( &p_res );
		CHECK_VALUE( ret_val );	
#ifdef _CONNECT_DETAIL        
        p_connect_manager->_peer_destroy_num++;    
#endif      
	}	
#endif      
    
#endif /* #ifdef ENABLE_BT  */
	
#ifdef ENABLE_EMULE  
    else if( p_res->_resource_type == EMULE_RESOURCE_TYPE )
    {
        cm_move_hash_map_res( &p_connect_manager->_peer_hash_value_map, 
            &p_connect_manager->_destoryed_peer_res_hash_value_map, p_res );
                
        ret_val = emule_resource_destroy( &p_res );
        CHECK_VALUE( ret_val ); 
#ifdef _CONNECT_DETAIL        
        p_connect_manager->_peer_destroy_num++;    
#endif      
    }   
#endif /* #ifdef ENABLE_EMULE  */


	p_connect_manager->_cur_res_num--;
	gcm_sub_res_num();
	return SUCCESS;
}

_int32 cm_move_hash_map_res( MAP *p_source_map, MAP *p_dest_map, RESOURCE *p_res )
{
	_int32 ret_val = SUCCESS;
	MAP_ITERATOR cur_node = MAP_BEGIN(*p_source_map);
	PAIR map_pair;
	LOG_DEBUG( "cm_move_hash_map_res, source_map:0x%x, dest_map:0x%x, res:0x%x",
		p_source_map, p_dest_map, p_res );
	while( cur_node != MAP_END(*p_source_map) )
	{
		if( p_res == MAP_VALUE(cur_node) )
		{
			map_pair._key = MAP_KEY( cur_node );
			map_pair._value = p_res;
			ret_val = map_insert_node( p_dest_map, &map_pair );
			sd_assert( ret_val == SUCCESS );
			map_erase_iterator( p_source_map, cur_node );
			return SUCCESS;
		}
		cur_node = MAP_NEXT( *p_source_map, cur_node );
	}
	return SUCCESS;
}

_int32 cm_destroy_all_pipes( CONNECT_MANAGER* p_connect_manager )
{
	_int32 ret_val = SUCCESS;

	LOG_DEBUG( "cm_destroy_all_pipes" );
	ret_val = cm_destroy_all_server_pipes( p_connect_manager );
	CHECK_VALUE( ret_val );
	
	ret_val = cm_destroy_all_peer_pipes( p_connect_manager );
	CHECK_VALUE( ret_val );

#ifdef ENABLE_CDN
	ret_val = cm_destroy_all_cdn_pipes( p_connect_manager );
	CHECK_VALUE( ret_val );
#endif

	return SUCCESS;
}


void cm_destroy_all_pipes_fileter(CONNECT_MANAGER *v_connect_manager, distroy_filter_fun fun)
{
	
	LOG_DEBUG( "cm_destroy_all_pipes_fileter" );

	if(v_connect_manager->_main_task_ptr == NULL)
	{
		LOG_DEBUG( "cm_destroy_all_pipes_fileter RETURN ... MAIN TASK PTR IS NULL" );
		return;
	}

	LOG_DEBUG( "cm_destroy_all_pipes_fileter main begin" );
	CONNECT_MANAGER * p_connect_manager = &v_connect_manager->_main_task_ptr->_connect_manager;

	cm_destroy_pipe_list( p_connect_manager, &(p_connect_manager->_working_peer_pipe_list), FALSE , fun);
	cm_destroy_pipe_list( p_connect_manager, &(p_connect_manager->_connecting_peer_pipe_list), TRUE , fun);

	MAP_ITERATOR map_node = NULL;
	CONNECT_MANAGER *p_sub_connect_manager = NULL;

	map_node = MAP_BEGIN( p_connect_manager->_sub_connect_manager_map );
	while( map_node != MAP_END( p_connect_manager->_sub_connect_manager_map ) )
	{
		LOG_DEBUG( "cm_destroy_all_pipes_fileter sub begin" );
		p_sub_connect_manager = MAP_VALUE( map_node );
		cm_destroy_pipe_list( p_sub_connect_manager, &(p_sub_connect_manager->_working_peer_pipe_list), FALSE , fun);
		cm_destroy_pipe_list( p_sub_connect_manager, &(p_sub_connect_manager->_connecting_peer_pipe_list), TRUE , fun);		
		map_node = MAP_NEXT(p_connect_manager->_sub_connect_manager_map, map_node);
	}
}

_int32 cm_destroy_all_pipes_except_cdn( CONNECT_MANAGER* p_connect_manager )
{
	_int32 ret_val = SUCCESS;

	LOG_DEBUG( "cm_destroy_all_pipes" );
	ret_val = cm_destroy_all_server_pipes( p_connect_manager );
	CHECK_VALUE( ret_val );
	
	ret_val = cm_destroy_all_peer_pipes( p_connect_manager );
	CHECK_VALUE( ret_val );

	return SUCCESS;
}

/* 销毁除了局域网http pipe 外的所有pipe */
_int32 cm_destroy_all_pipes_except_lan( CONNECT_MANAGER* p_connect_manager )
{
	_int32 ret_val = SUCCESS;

	LOG_DEBUG( "cm_destroy_all_pipes" );
	ret_val = cm_destroy_all_server_pipes_except_lan( p_connect_manager );
	CHECK_VALUE( ret_val );
	
	ret_val = cm_destroy_all_peer_pipes( p_connect_manager );
	CHECK_VALUE( ret_val );

#ifdef ENABLE_CDN
	ret_val = cm_destroy_all_cdn_pipes( p_connect_manager );
	CHECK_VALUE( ret_val );
#endif

	/* 除局域网 资源以外,把所有其他正在使用的资源都移到候选资源列表中,等到下一次调度再使用 */
	cm_move_using_res_to_candidate_except_lan(p_connect_manager);

	return SUCCESS;
}

/* 销毁除了局域网http pipe 外的所有pipe */
_int32 cm_destroy_all_server_pipes_except_lan( CONNECT_MANAGER* p_connect_manager )
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "cm_destroy_all_server_pipes" );
	
	ret_val = cm_destroy_pipe_list_except_lan( p_connect_manager, &(p_connect_manager->_working_server_pipe_list), FALSE );
	CHECK_VALUE( ret_val );

	ret_val = cm_destroy_pipe_list_except_lan( p_connect_manager, &(p_connect_manager->_connecting_server_pipe_list), TRUE );
	CHECK_VALUE( ret_val );
	
	return SUCCESS;
}

_int32 cm_destroy_all_server_pipes( CONNECT_MANAGER* p_connect_manager )
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "cm_destroy_all_server_pipes" );
	
	ret_val = cm_destroy_pipe_list( p_connect_manager, &(p_connect_manager->_working_server_pipe_list), FALSE , NULL);
	CHECK_VALUE( ret_val );

	ret_val = cm_destroy_pipe_list( p_connect_manager, &(p_connect_manager->_connecting_server_pipe_list), TRUE , NULL);
	CHECK_VALUE( ret_val );
	
	return SUCCESS;
}

_int32 cm_destroy_all_peer_pipes( CONNECT_MANAGER* p_connect_manager )
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "cm_destroy_all_peer_pipes" );

	ret_val = cm_destroy_pipe_list( p_connect_manager, &(p_connect_manager->_working_peer_pipe_list), FALSE , NULL);
	CHECK_VALUE( ret_val );

	ret_val = cm_destroy_pipe_list( p_connect_manager, &(p_connect_manager->_connecting_peer_pipe_list), TRUE , NULL);
	CHECK_VALUE( ret_val );
	
	return SUCCESS;
}

/* 销毁除了局域网http pipe 外的所有pipe */
_int32 cm_destroy_pipe_list_except_lan( CONNECT_MANAGER *p_connect_manager, PIPE_LIST *p_pipe_list, BOOL is_connecting_list )
{
	_int32 ret_val = SUCCESS;
	LIST_ITERATOR cur_node, end_node, next_node;
	cur_node = LIST_BEGIN( p_pipe_list->_data_pipe_list );
	end_node = LIST_END( p_pipe_list->_data_pipe_list );
	LOG_DEBUG( "cm_destroy_pipe_list" );

	while( cur_node != end_node )
	{
		DATA_PIPE *p_data_pipe = LIST_VALUE( cur_node );
		if(p_data_pipe->_data_pipe_type == HTTP_PIPE)
		{
			HTTP_DATA_PIPE * http_pipe = (HTTP_DATA_PIPE *)p_data_pipe;
			HTTP_SERVER_RESOURCE *	p_http_server_resource = http_pipe->_p_http_server_resource;
			sd_assert(p_http_server_resource!=NULL);
			sd_assert(sd_strlen(p_http_server_resource->_url._host)!=0);
			if(sd_check_if_in_nat_host(p_http_server_resource->_url._host))
			{
				cur_node = LIST_NEXT( cur_node );
				continue;
			}
		}
		
		if( is_connecting_list )
		{
			p_data_pipe->_p_resource->_connecting_pipe_num--;
		}
		ret_val = cm_destroy_single_pipe( p_connect_manager, p_data_pipe);
		CHECK_VALUE( ret_val );

		next_node = LIST_NEXT( cur_node );

		ret_val = list_erase( &p_pipe_list->_data_pipe_list, cur_node );
		CHECK_VALUE( ret_val );

		cur_node = next_node;
	}
	
	return SUCCESS;		
}


_int32 cm_destroy_pipe_list( CONNECT_MANAGER *p_connect_manager, PIPE_LIST *p_pipe_list, BOOL is_connecting_list, distroy_filter_fun fun)
{
	_int32 ret_val = SUCCESS;
	LIST_ITERATOR cur_node, end_node, next_node;
	cur_node = LIST_BEGIN( p_pipe_list->_data_pipe_list );
	end_node = LIST_END( p_pipe_list->_data_pipe_list );
	LOG_DEBUG( "cm_destroy_pipe_list" );

	while( cur_node != end_node )
	{
		DATA_PIPE *p_data_pipe = LIST_VALUE( cur_node );

		if(fun != NULL && !fun(p_connect_manager, p_data_pipe))
		{
			// 过滤条件如果返回false，则不删除
			next_node = LIST_NEXT( cur_node );
			cur_node = next_node;
			continue;
		}
		
		if( is_connecting_list )
		{
			p_data_pipe->_p_resource->_connecting_pipe_num--;
		}
		ret_val = cm_destroy_single_pipe( p_connect_manager, p_data_pipe);
		CHECK_VALUE( ret_val );

		next_node = LIST_NEXT( cur_node );

		ret_val = list_erase( &p_pipe_list->_data_pipe_list, cur_node );
		CHECK_VALUE( ret_val );

		cur_node = next_node;
	}
	
	return SUCCESS;		
}

_int32 cm_destroy_single_pipe( CONNECT_MANAGER *p_connect_manager, DATA_PIPE *p_data_pipe )
{
	_int32 ret_val = SUCCESS;
	RESOURCE *p_res = p_data_pipe->_p_resource;	
	PIPE_WRAP *p_pipe_wrap = (PIPE_WRAP *)p_data_pipe->_dispatch_info._global_wrap_ptr;

    if( p_res == NULL )
    {
        sd_assert( FALSE );
        return SUCCESS;
    }
    
	p_res->_max_speed = MAX( p_res->_max_speed, p_data_pipe->_dispatch_info._max_speed );
	if( p_pipe_wrap != NULL )
	{
		p_pipe_wrap->_is_deleted_by_pipe = TRUE;		
	}	
	
    LOG_DEBUG( "cm_destroy_single_pipe: [dispatch_stat] pipe 0x%x delete, pipe_tpye:%d, pipe speed:%u, pipe_max_speed:%u, res_ptr:0x%x.res_retry time:%d, is_global_filtered:%d, pipe_wrap:0x%x ", 
        p_data_pipe, p_data_pipe->_data_pipe_type, p_data_pipe->_dispatch_info._speed, 
        p_data_pipe->_dispatch_info._max_speed, p_data_pipe->_p_resource, 
        p_data_pipe->_p_resource->_retry_times, p_data_pipe->_dispatch_info._is_global_filted,
        p_pipe_wrap );

	ret_val = gcm_unregister_pipe( p_connect_manager, p_data_pipe);
	CHECK_VALUE( ret_val );
	
	if( p_data_pipe->_data_pipe_type == HTTP_PIPE )
	{
		ret_val = http_pipe_close( p_data_pipe );
		CHECK_VALUE( ret_val );
	}
	else if( p_data_pipe->_data_pipe_type == FTP_PIPE )
	{
		ret_val = ftp_pipe_close( p_data_pipe );
		CHECK_VALUE( ret_val );
	}
	else if( p_data_pipe->_data_pipe_type == P2P_PIPE )
	{
		ret_val = p2p_pipe_close( p_data_pipe );
		CHECK_VALUE( ret_val );
	}
#ifdef ENABLE_BT 
#ifdef ENABLE_BT_PROTOCOL
	else if( p_data_pipe->_data_pipe_type == BT_PIPE )
	{
		ret_val = bt_pipe_close( p_data_pipe );
		CHECK_VALUE( ret_val );
	}	
	
#endif 
#endif /* #ifdef ENABLE_BT  */
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL
	else if(p_data_pipe->_data_pipe_type == EMULE_PIPE)
	{
		ret_val = emule_pipe_close(p_data_pipe);
		CHECK_VALUE(ret_val);
	}
#endif
#endif
	p_res->_pipe_num--;
	p_connect_manager->_cur_pipe_num--;	
	LOG_DEBUG("cm_destroy_single_pipe, p_connect_manager->_cur_pipe_num = %d", p_connect_manager->_cur_pipe_num);

	if( p_pipe_wrap != NULL )
	{
		LOG_DEBUG( "cm_destroy_single_pipe end:pipe_wrap:0x%x, _is_deleted_by_pipe:%d", 
			p_pipe_wrap, p_pipe_wrap->_is_deleted_by_pipe );
	}

	return SUCCESS;	
}

_int32 cm_discard_all_server_res_except( CONNECT_MANAGER *p_connect_manager, RESOURCE *p_res )
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "cm_discard_all_server_res_except. resoure_ptr:0x%x.", p_res );

	ret_val = cm_move_res_list_except_res( p_connect_manager, &p_connect_manager->_idle_server_res_list, &p_connect_manager->_discard_server_res_list, p_res );
	CHECK_VALUE( ret_val );
	
	ret_val = cm_move_res_list_except_res( p_connect_manager, &p_connect_manager->_using_server_res_list, &p_connect_manager->_discard_server_res_list, p_res );
	CHECK_VALUE( ret_val );
	
	ret_val = cm_move_res_list_except_res( p_connect_manager, &p_connect_manager->_candidate_server_res_list, &p_connect_manager->_discard_server_res_list, p_res );
	CHECK_VALUE( ret_val );
	
	ret_val = cm_move_res_list_except_res( p_connect_manager, &p_connect_manager->_retry_server_res_list, &p_connect_manager->_discard_server_res_list, p_res );
	CHECK_VALUE( ret_val );
	return SUCCESS;	
}

_int32 cm_discard_all_peer_res( CONNECT_MANAGER *p_connect_manager )
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "cm_discard_all_peer_res." );

	ret_val = cm_move_res_list_except_res( p_connect_manager, &p_connect_manager->_idle_peer_res_list, &p_connect_manager->_discard_peer_res_list, NULL );
	CHECK_VALUE( ret_val );
	
	ret_val = cm_move_res_list_except_res( p_connect_manager, &p_connect_manager->_using_peer_res_list, &p_connect_manager->_discard_peer_res_list, NULL );
	CHECK_VALUE( ret_val );
	
	ret_val = cm_move_res_list_except_res( p_connect_manager, &p_connect_manager->_candidate_peer_res_list, &p_connect_manager->_discard_peer_res_list, NULL );
	CHECK_VALUE( ret_val );
	
	ret_val = cm_move_res_list_except_res( p_connect_manager, &p_connect_manager->_retry_peer_res_list, &p_connect_manager->_discard_peer_res_list, NULL );
	CHECK_VALUE( ret_val );
	return SUCCESS;	
}

/* 除局域网 资源以外,把所有其他正在使用的资源都移到候选资源列表中,等到下一次调度再使用 */
_int32 cm_move_using_res_to_candidate_except_lan(CONNECT_MANAGER *p_connect_manager )
{
	_int32 ret_val = SUCCESS;
	RESOURCE *p_except_res = NULL;
	
	if(p_connect_manager->_origin_res_ptr && p_connect_manager->_origin_res_ptr->_resource_type ==HTTP_RESOURCE)
	{
		HTTP_SERVER_RESOURCE * p_http_res = (HTTP_SERVER_RESOURCE *)p_connect_manager->_origin_res_ptr;
		if(sd_check_if_in_nat_host(p_http_res->_url._host))
		{
			p_except_res = p_connect_manager->_origin_res_ptr;
		}
	}
	ret_val = cm_move_using_res_to_candidate_except_res( p_connect_manager,  p_except_res );
	CHECK_VALUE( ret_val );
	return SUCCESS;
}
_int32 cm_move_using_res_to_candidate_except_res(CONNECT_MANAGER *p_connect_manager ,RESOURCE *p_except_res)
{
	_int32 ret_val = SUCCESS;
	ret_val = cm_move_using_res_list_to_candidate_res_list_except_res( p_connect_manager, &p_connect_manager->_using_peer_res_list, &p_connect_manager->_candidate_peer_res_list, p_except_res );
	CHECK_VALUE( ret_val );
	ret_val = cm_move_using_res_list_to_candidate_res_list_except_res( p_connect_manager, &p_connect_manager->_using_server_res_list, &p_connect_manager->_candidate_server_res_list, p_except_res );
	CHECK_VALUE( ret_val );
	return SUCCESS;
}

_int32 cm_move_using_res_list_to_candidate_res_list_except_res( CONNECT_MANAGER *p_connect_manager, RESOURCE_LIST *p_using_res_list, RESOURCE_LIST *p_candidate_res_list, RESOURCE *p_except_res )
{
	_int32 ret_val = SUCCESS;
	LIST_ITERATOR cur_node, end_node, next_node;
	LOG_DEBUG( "cm_move_using_res_list_to_candidate_res_list_except_res. p_using_res_list:0x%x, p_dest_res_list:0x%x.", p_using_res_list, p_candidate_res_list );

	cur_node = LIST_BEGIN( p_using_res_list->_res_list );
	end_node = LIST_END( p_using_res_list->_res_list );

	while( cur_node != end_node )
	{
		RESOURCE *p_res = LIST_VALUE( cur_node );
		if( p_res == p_except_res )
		{
			cur_node = LIST_NEXT( cur_node );
			continue;
		}
	
		next_node = LIST_NEXT( cur_node );

		ret_val = list_push( &p_candidate_res_list->_res_list, p_res );
		CHECK_VALUE( ret_val );
		
		gcm_register_candidate_res( p_connect_manager,  p_res );
		
		ret_val = list_erase( &p_using_res_list->_res_list, cur_node );
		CHECK_VALUE( ret_val );

		cur_node = next_node;
	}	
	return SUCCESS;	
}

_int32 cm_move_res_list_except_res( CONNECT_MANAGER *p_connect_manager
    , RESOURCE_LIST *p_src_res_list
    , RESOURCE_LIST *p_dest_res_list
    , RESOURCE *p_except_res )
{
	_int32 ret_val = SUCCESS;
	LIST_ITERATOR cur_node, end_node, next_node;
	LOG_DEBUG( "cm_move_res_list. p_src_res_list:0x%x, p_dest_res_list:0x%x.", p_src_res_list, p_dest_res_list );

	cur_node = LIST_BEGIN( p_src_res_list->_res_list );
	end_node = LIST_END( p_src_res_list->_res_list );

	while( cur_node != end_node )
	{
		RESOURCE *p_res = LIST_VALUE( cur_node );
		if( p_res == p_except_res )
		{
			cur_node = LIST_NEXT( cur_node );
			continue;
		}
	
		next_node = LIST_NEXT( cur_node );

		ret_val = list_push( &p_dest_res_list->_res_list, p_res );
		CHECK_VALUE( ret_val );
		
		ret_val = list_erase( &p_src_res_list->_res_list, cur_node );
		CHECK_VALUE( ret_val );

		cur_node = next_node;
	}	
	return SUCCESS;	
}

_int32 cm_destroy_server_pipes_except( CONNECT_MANAGER *p_connect_manager, RESOURCE *p_res )
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "cm_destroy_server_pipes_except" );
	
	ret_val = cm_destroy_pipe_list_except_owned_by_res( p_connect_manager, &(p_connect_manager->_working_server_pipe_list), p_res, FALSE );
	CHECK_VALUE( ret_val );

	ret_val = cm_destroy_pipe_list_except_owned_by_res( p_connect_manager, &(p_connect_manager->_connecting_server_pipe_list), p_res, TRUE );
	CHECK_VALUE( ret_val );
	
	return SUCCESS;
}

_int32 cm_destroy_pipe_list_except_owned_by_res( CONNECT_MANAGER *p_connect_manager
        , PIPE_LIST *p_pipe_list
        , RESOURCE *p_res
        , BOOL is_connecting_list )
{
	_int32 ret_val = SUCCESS;
	LIST_ITERATOR cur_node, end_node, next_node;
	cur_node = LIST_BEGIN( p_pipe_list->_data_pipe_list );
	end_node = LIST_END( p_pipe_list->_data_pipe_list );
	LOG_DEBUG( "cm_destroy_pipe_list_except_owned_by_res, resource_ptr:0x%x.", p_res );

	while( cur_node != end_node )
	{
		DATA_PIPE *p_data_pipe = LIST_VALUE( cur_node );
		if( p_data_pipe->_p_resource == p_res )
		{
			cur_node = LIST_NEXT( cur_node );
			continue;
		}
		if( is_connecting_list )
		{
			p_data_pipe->_p_resource->_connecting_pipe_num--;
		}
		ret_val = cm_destroy_single_pipe( p_connect_manager, p_data_pipe);
		CHECK_VALUE( ret_val );

		next_node = LIST_NEXT( cur_node );

		ret_val = list_erase( &p_pipe_list->_data_pipe_list, cur_node );
		CHECK_VALUE( ret_val );

		cur_node = next_node;

	}
	
	return SUCCESS;		
}

_int32 cm_destroy_server_pipes_owned_by_res( CONNECT_MANAGER *p_connect_manager
            , RESOURCE *p_res )
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "cm_destroy_server_pipes_owned_by_res" );
	
	ret_val = cm_destroy_pipe_list_owned_by_res( p_connect_manager
	        , &(p_connect_manager->_working_server_pipe_list)
	        , p_res
	        , FALSE );
	CHECK_VALUE( ret_val );

	ret_val = cm_destroy_pipe_list_owned_by_res( p_connect_manager
	        , &(p_connect_manager->_connecting_server_pipe_list)
	        , p_res
	        , TRUE );

	ret_val = cm_destroy_pipe_list_owned_by_res( p_connect_manager
	        , &(p_connect_manager->_working_peer_pipe_list)
	        , p_res
	        , FALSE );
	CHECK_VALUE( ret_val );

	ret_val = cm_destroy_pipe_list_owned_by_res( p_connect_manager
	        , &(p_connect_manager->_connecting_peer_pipe_list)
	        , p_res
	        , TRUE );
	        
	CHECK_VALUE( ret_val );
	
	return SUCCESS;
}

_int32 cm_destroy_pipe_list_owned_by_res( CONNECT_MANAGER *p_connect_manager
        , PIPE_LIST *p_pipe_list
        , RESOURCE *p_res 
        , BOOL is_connecting_list)
{
	_int32 ret_val = SUCCESS;
	LIST_ITERATOR cur_node, end_node, next_node;
	cur_node = LIST_BEGIN( p_pipe_list->_data_pipe_list );
	end_node = LIST_END( p_pipe_list->_data_pipe_list );
	LOG_DEBUG( "cm_destroy_pipe_list_owned_by_res, resource_ptr:0x%x.", p_res );

	while( cur_node != end_node )
	{
		DATA_PIPE *p_data_pipe = LIST_VALUE( cur_node );
		if( p_data_pipe->_p_resource != p_res )
		{
			cur_node = LIST_NEXT( cur_node );
			continue;
		}
		if( is_connecting_list )
		{
			p_data_pipe->_p_resource->_connecting_pipe_num--;
		}
		ret_val = cm_destroy_single_pipe( p_connect_manager, p_data_pipe);
		CHECK_VALUE( ret_val );

		next_node = LIST_NEXT( cur_node );

		ret_val = list_erase( &p_pipe_list->_data_pipe_list, cur_node );
		CHECK_VALUE( ret_val );

		cur_node = next_node;

	}
	
	return SUCCESS;		
}


BOOL cm_get_excellent_filename_from_res_list( CONNECT_MANAGER *p_connect_manager, RESOURCE_LIST *p_res_list, 
	char *p_str_excellent_name, _u32 excellent_name_buffer_len, _u32 *p_excellent_name_len, 
	char *p_str_normal_name, _u32 normal_name_buffer_len, _u32 *p_normal_name_len )
{

	return FALSE;
}


BOOL cm_get_excellent_filename_from_orgin_res( CONNECT_MANAGER *p_connect_manager, RESOURCE *p_orgin_res, 
	char *p_str_excellent_name, _u32 excellent_name_buffer_len, _u32 *p_excellent_name_len, 
	char *p_str_normal_name, _u32 normal_name_buffer_len, _u32 *p_normal_name_len )
{

	return FALSE;
}


void cm_set_need_new_server_res( CONNECT_MANAGER *p_connect_manager, BOOL is_need_new_res )
{
	LOG_DEBUG( "cm_set_need_new_server_res.is_need_new_res:%d", is_need_new_res );
	p_connect_manager->_is_need_new_server_res = is_need_new_res;	
}

void cm_set_need_new_peer_res( CONNECT_MANAGER *p_connect_manager, BOOL is_need_new_res )
{
	LOG_DEBUG( "cm_set_need_new_peer_res.is_need_new_res:%d", is_need_new_res );
	p_connect_manager->_is_need_new_peer_res = is_need_new_res;	
}


void cm_check_is_fast_speed( CONNECT_MANAGER *p_connect_manager )
{
	_u32 total_speed = p_connect_manager->_cur_server_speed + p_connect_manager->_cur_peer_speed;

	if( total_speed > cm_refuse_more_res_speed_limit() )
	{
		p_connect_manager->_is_fast_speed = TRUE;
	}
	else
	{
		p_connect_manager->_is_fast_speed = FALSE;
	}
	LOG_DEBUG( "check_is_fast_speed.cur_speed:%u, is_fast_speed:%d",
		total_speed, p_connect_manager->_is_fast_speed );
}

_int32 cm_get_sub_connect_manager(CONNECT_MANAGER *p_connect_manager, _u32 file_index, CONNECT_MANAGER **pp_sub_connect_manager)
{
	MAP_ITERATOR map_node = NULL;

	if( file_index == INVALID_FILE_INDEX )
	{
		*pp_sub_connect_manager = p_connect_manager;
		return SUCCESS;
	}

	map_find_iterator( &p_connect_manager->_sub_connect_manager_map, (void *)file_index, &map_node );
	if( map_node == MAP_END( p_connect_manager->_sub_connect_manager_map ) )
	{
		return CM_LOGIC_ERROR;
	}

	*pp_sub_connect_manager = MAP_VALUE( map_node );
	
	LOG_DEBUG( "cm_get_sub_connect_manager.p_connect_manager:0x%x, file_index:%u, p_sub_connect_manager:0x%x ",
		p_connect_manager, file_index, *pp_sub_connect_manager );
	return SUCCESS;
}


#ifdef _LOGGER

void cm_print_info( CONNECT_MANAGER *p_connect_manager )
{
    LOG_DEBUG( "[vod_dispatch_analysis] print_cm_info:ptr:0s%x,cnting_server:%u,cnting_peer:%u,working_server:%u,working_peer:%u,res_num=%u,pipe_num=%u,using_origin=%d,is_idle=%d,is_fast=%d,server_speed=%u/%u,peer_speed=%u/%u,upload_speed=%u,cdn_pipe=%u,pause_pipes=%d",
        p_connect_manager,
        list_size(&p_connect_manager->_connecting_server_pipe_list._data_pipe_list) ,
        list_size(&p_connect_manager->_connecting_peer_pipe_list._data_pipe_list) ,
        list_size(&p_connect_manager->_working_server_pipe_list._data_pipe_list),
        list_size(&p_connect_manager->_working_peer_pipe_list._data_pipe_list) ,

        p_connect_manager->_cur_res_num,
        p_connect_manager->_cur_pipe_num,
        cm_is_origin_mode(p_connect_manager),
        p_connect_manager->_is_idle_status,
        p_connect_manager->_is_fast_speed,
        p_connect_manager->_cur_server_speed,
        p_connect_manager->_max_server_speed,
        p_connect_manager->_cur_peer_speed,
        p_connect_manager->_max_peer_speed,
        p_connect_manager->_cur_peer_upload_speed,
        list_size(&p_connect_manager->_cdn_pipe_list._data_pipe_list) ,
        p_connect_manager->_pause_pipes );

    LOG_DEBUG("[vod_dispatch_analysis] cm_print_info >> _idle_server_res_list:");
    cm_print_server_res_list_info( p_connect_manager, &p_connect_manager->_idle_server_res_list );
    LOG_DEBUG("[vod_dispatch_analysis] cm_print_info >> _using_server_res_list:");
    cm_print_server_res_list_info( p_connect_manager, &p_connect_manager->_using_server_res_list );
    LOG_DEBUG("[vod_dispatch_analysis] cm_print_info >> _candidate_server_res_list:");
    cm_print_server_res_list_info( p_connect_manager, &p_connect_manager->_candidate_server_res_list );
    LOG_DEBUG("[vod_dispatch_analysis] cm_print_info >> _retry_server_res_list:");
    cm_print_server_res_list_info( p_connect_manager, &p_connect_manager->_retry_server_res_list );
    LOG_DEBUG("[vod_dispatch_analysis] cm_print_info >> _discard_server_res_list:");
    cm_print_server_res_list_info( p_connect_manager, &p_connect_manager->_discard_server_res_list );
    LOG_DEBUG("[vod_dispatch_analysis] cm_print_info >> _idle_peer_res_list:");
    cm_print_peer_res_list_info( p_connect_manager, &p_connect_manager->_idle_peer_res_list );
    LOG_DEBUG("[vod_dispatch_analysis] cm_print_info >> _using_peer_res_list:");
    cm_print_peer_res_list_info( p_connect_manager, &p_connect_manager->_using_peer_res_list );
    LOG_DEBUG("[vod_dispatch_analysis] cm_print_info >> _candidate_peer_res_list:");
    cm_print_peer_res_list_info( p_connect_manager, &p_connect_manager->_candidate_peer_res_list);
    LOG_DEBUG("[vod_dispatch_analysis] cm_print_info >> _retry_peer_res_list:");
    cm_print_peer_res_list_info( p_connect_manager, &p_connect_manager->_retry_peer_res_list );
    LOG_DEBUG("[vod_dispatch_analysis] cm_print_info >> _discard_peer_res_list:");
    cm_print_peer_res_list_info( p_connect_manager, &p_connect_manager->_discard_peer_res_list );
}


void cm_print_server_res_list_info( CONNECT_MANAGER *p_connect_manager, RESOURCE_LIST *p_res_list )
{
    output_resource_list(p_res_list);
}

void cm_print_peer_res_list_info( CONNECT_MANAGER *p_connect_manager, RESOURCE_LIST *p_res_list )
{
    output_resource_list(p_res_list);
}

#endif


#ifdef _CONNECT_DETAIL

void cm_update_peer_pipe_info( CONNECT_MANAGER * p_connect_manager )
{
    _u32 pipe_index = 0;
    _int32 ret_val = SUCCESS;
    LIST_ITERATOR cur_node = LIST_BEGIN( p_connect_manager->_working_peer_pipe_list._data_pipe_list );

    sd_memset( &p_connect_manager->_peer_pipe_info, 0, sizeof( p_connect_manager->_peer_pipe_info ) );

    while( cur_node != LIST_END( p_connect_manager->_working_peer_pipe_list._data_pipe_list )
        && pipe_index < PEER_INFO_NUM )
    {
        DATA_PIPE *p_data_pipe = LIST_VALUE( cur_node );
		if( p_data_pipe->_dispatch_info._pipe_state != PIPE_DOWNLOADING )
		{
			cur_node = LIST_NEXT( cur_node );
			continue;
		}
		if( p_data_pipe->_data_pipe_type == P2P_PIPE )
		{
			ret_val = p2p_pipe_get_peer_pipe_info( p_data_pipe, &p_connect_manager->_peer_pipe_info._pipe_info_list[pipe_index] );
		}
#ifdef ENABLE_BT
#ifdef ENABLE_BT_PROTOCOL

		else if( p_data_pipe->_data_pipe_type == BT_PIPE )
		{
			ret_val = bt_pipe_get_peer_pipe_info( p_data_pipe, &p_connect_manager->_peer_pipe_info._pipe_info_list[pipe_index] );
		}
#endif
        
#endif

#ifdef ENABLE_EMULE
		else if(p_data_pipe->_data_pipe_type == EMULE_PIPE)
		{
			ret_val = emule_pipe_get_peer_pipe_info(p_data_pipe, &p_connect_manager->_peer_pipe_info._pipe_info_list[pipe_index]);
		}
#endif
        sd_assert( ret_val == SUCCESS );		

        pipe_index++;                
        cur_node = LIST_NEXT( cur_node );
    }

     p_connect_manager->_peer_pipe_info._pipe_info_num = pipe_index;
}

void cm_update_server_pipe_info( CONNECT_MANAGER * p_connect_manager )
{
    sd_memset( &p_connect_manager->_server_pipe_info, 0, sizeof( p_connect_manager->_server_pipe_info ) );
}
#endif


#ifdef ENABLE_CDN
/***************************************************************************************************/
/*   Internal functions   */
/***************************************************************************************************/
_int32 cm_create_cdn_pipes( CONNECT_MANAGER *p_connect_manager )
{

	LOG_DEBUG( "cm_create_cdn_pipes");

	return cm_update_cdn_pipes( p_connect_manager );

}

_int32 cm_create_single_cdn_pipe( CONNECT_MANAGER *p_connect_manager, RESOURCE *p_res )
{
	_int32 ret_val = SUCCESS;
	DATA_PIPE *p_pipe = NULL;

	LOG_DEBUG( "cm_create_single_cdn_pipe:resource:0x%x,pipe_num=%u", p_res,p_res->_pipe_num);
	
	if( p_res->_resource_type == PEER_RESOURCE)
	{
		P2P_RESOURCE* p2pres = (P2P_RESOURCE*)(p_res);
		LOG_DEBUG( "cm_create_single_cdn_pipe:resource:0x%x,pipe_num=%u, peerid=%s", p_res,p_res->_pipe_num, p2pres->_peer_id);
		ret_val = p2p_pipe_create( p_connect_manager->_data_manager_ptr, p_res, &p_pipe, NULL );
	}
	else
	if( p_res->_resource_type == HTTP_RESOURCE)
	{
		ret_val = http_pipe_create( p_connect_manager->_data_manager_ptr, p_res, &p_pipe);
	}
	else
	if( p_res->_resource_type == FTP_RESOURCE)
	{
		ret_val = ftp_pipe_create( p_connect_manager->_data_manager_ptr, p_res, &p_pipe);
	}
	else
	{
		sd_assert(FALSE);
		return SUCCESS;
	}
	
	if( ret_val != SUCCESS )
	{
		LOG_ERROR( "cm create  pipe error. error code:%d.", ret_val );
		return ret_val;
	}		
	
	dp_set_pipe_interface( p_pipe, &p_connect_manager->_pipe_interface );

	p_res->_pipe_num++;	
	
	LOG_DEBUG( "cm open  pipe. resource:0x%x, pipe:0x%x", p_res, p_pipe );

	if( p_res->_resource_type == PEER_RESOURCE)
	{
		ret_val = p2p_pipe_open( p_pipe );
	}
	else
	if( p_res->_resource_type == HTTP_RESOURCE)
	{
		ret_val = http_pipe_open(  p_pipe );
	}
	else
	if( p_res->_resource_type == FTP_RESOURCE)
	{
		ret_val = ftp_pipe_open(  p_pipe );
	}

	if( ret_val != SUCCESS )
	{
		LOG_ERROR( "cm open  pipe error. error code:%d.", ret_val );
		
#ifdef EMBED_REPORT
		p_connect_manager->_report_para._cdn_connect_failed_times++;
#endif
		cm_destroy_single_cdn_pipe( p_connect_manager, p_pipe );
		return ret_val;	
	}
    
	ret_val = list_push( &(p_connect_manager->_cdn_pipe_list._data_pipe_list), p_pipe );
	if( ret_val != SUCCESS )
	{
		LOG_ERROR( "cm list_push peer pipe error. error code:%d.", ret_val );
		cm_destroy_single_cdn_pipe( p_connect_manager, p_pipe );
		return ret_val;	
	}

    LOG_DEBUG( "cm_create_single_cdn_pipe. _data_pipe_list:0x%x, list_size:%u, [dispatch_stat] pipe 0x%x create 0x%x (resource).",
        &(p_connect_manager->_cdn_pipe_list._data_pipe_list), 
        list_size(&(p_connect_manager->_cdn_pipe_list._data_pipe_list)), 
        p_pipe, p_res);

	// backup the assist pipe
	TASK *p_task = (TASK*)(p_connect_manager->_main_task_ptr);
	if(p_task && ((P2P_RESOURCE*)p_res)->_from == P2P_FROM_ASSIST_DOWNLOAD)
	{
		// This pipe cannot be closed by others forever
		p_connect_manager->_main_task_ptr->_p_assist_pipe = p_pipe;
	}

	return SUCCESS;
}

_int32 cm_create_single_normal_cdn_pipe( CONNECT_MANAGER *p_connect_manager, RESOURCE *p_res )
{
	_int32 ret_val = SUCCESS;
	DATA_PIPE *p_pipe = NULL;

	LOG_DEBUG( "cm_create_single_normal_cdn_pipe:resource:0x%x,pipe_num=%u", p_res,p_res->_pipe_num);

	P2P_RESOURCE* p2pres = (P2P_RESOURCE*)(p_res);
	LOG_DEBUG( "cm_create_single_cdn_pipe:resource:0x%x,pipe_num=%u, peerid=%s", 
		p_res,p_res->_pipe_num, p2pres->_peer_id);
	ret_val = p2p_pipe_create( p_connect_manager->_data_manager_ptr, p_res, &p_pipe, NULL );

	if(ret_val != SUCCESS)
	{
		LOG_ERROR( "cm create  pipe error. error code:%d.", ret_val );
		return ret_val;
	}		

	dp_set_pipe_interface( p_pipe, &p_connect_manager->_pipe_interface );

	p_res->_pipe_num++;	

	LOG_DEBUG( "cm open  pipe. resource:0x%x, pipe:0x%x", p_res, p_pipe );

	ret_val = p2p_pipe_open( p_pipe );
	if (ret_val == SUCCESS) 
	{
		_u32 now_time = 0;

		sd_time(&now_time);

		LOG_DEBUG("p2p pipe open success.");

		p_res->_open_normal_cdn_pipe_time = now_time;
		p_res->_close_normal_cdn_pipe_time = 0;


		if (p2p_get_from(p_res) == P2P_FROM_NORMAL_CDN) 
		{
			p_connect_manager->_main_task_ptr->_last_open_normal_cdn_time = now_time;

			#ifdef EMBED_REPORT
			if (p_connect_manager->_report_para._res_normal_cdn_start_time == 0)
			{
				p_connect_manager->_report_para._res_normal_cdn_start_time = now_time;
			}

			if (p_connect_manager->_report_para._res_normal_cdn_first_from_task_start == 0)
			{
				p_connect_manager->_report_para._res_normal_cdn_first_from_task_start = 
					now_time - p_connect_manager->_main_task_ptr->_start_time;
			}

			LOG_DEBUG("now open normal cdn pipe, now time:%u, start to open pipe time:%u, "
				"start to close time:%u, last open pipe time in task:%u, first from:%u.", now_time, 
				p_connect_manager->_report_para._res_normal_cdn_start_time,
				p_connect_manager->_report_para._res_normal_cdn_close_time,
				p_connect_manager->_main_task_ptr->_last_open_normal_cdn_time,
				p_connect_manager->_report_para._res_normal_cdn_first_from_task_start);
			#endif
		}
	}
			
	if( ret_val != SUCCESS )
	{
		LOG_ERROR( "cm open  pipe error. error code:%d.", ret_val );

#ifdef EMBED_REPORT
		p_connect_manager->_report_para._cdn_connect_failed_times++;
#endif
		cm_destroy_single_normal_cdn_pipe( p_connect_manager, p_pipe );
		return ret_val;	
	}

	ret_val = list_push( &(p_connect_manager->_cdn_pipe_list._data_pipe_list), p_pipe );
	if( ret_val != SUCCESS )
	{
		LOG_ERROR( "cm list_push peer pipe error. error code:%d.", ret_val );
		cm_destroy_single_normal_cdn_pipe( p_connect_manager, p_pipe );
		return ret_val;	
	}

	LOG_DEBUG( "_data_pipe_list:0x%x, list_size:%u, [dispatch_stat] pipe 0x%x create 0x%x (resource).",
		&(p_connect_manager->_cdn_pipe_list._data_pipe_list), 
		list_size(&(p_connect_manager->_cdn_pipe_list._data_pipe_list)), 
		p_pipe, p_res);

	return ret_val;
}

_int32 cm_destroy_all_cdn_ress( CONNECT_MANAGER *p_connect_manager )
{
	_int32 ret_val = SUCCESS;
	LIST_ITERATOR cur_node, end_node, next_node;
	LOG_DEBUG( "cm_destroy_all_cdn_ress");

	cur_node = LIST_BEGIN( p_connect_manager->_cdn_res_list._res_list );
	end_node = LIST_END( p_connect_manager->_cdn_res_list._res_list);

	while( cur_node != end_node )
	{
		RESOURCE *p_res = LIST_VALUE( cur_node );

		ret_val = cm_destroy_cdn_res( p_connect_manager, p_res );
		CHECK_VALUE( ret_val );
		
		next_node = LIST_NEXT( cur_node );
		
		ret_val = list_erase( &( p_connect_manager->_cdn_res_list._res_list), cur_node );
		CHECK_VALUE( ret_val );

		cur_node = next_node;
	}	
	
	return SUCCESS;	
}

_int32 cm_destroy_cdn_res( CONNECT_MANAGER *p_connect_manager, RESOURCE *p_res )
{
	_int32 ret_val = SUCCESS;
    LOG_DEBUG( "cm_destroy_cdn_res,[dispatch_stat] resource 0x%x delete", p_res);
	sd_assert( p_res->_pipe_num == 0 );
	sd_assert( p_res->_connecting_pipe_num == 0 );
	sd_assert( p_res != p_connect_manager->_origin_res_ptr );

	if( p_res->_resource_type == HTTP_RESOURCE )
	{
		ret_val = http_resource_destroy( &p_res );
		CHECK_VALUE( ret_val );
        
	}
	else if( p_res->_resource_type == FTP_RESOURCE )
	{
		ret_val = ftp_resource_destroy( &p_res );
		CHECK_VALUE( ret_val );	
        
	}
	else if( p_res->_resource_type == PEER_RESOURCE )
	{
		ret_val = p2p_resource_destroy( &p_res );
		CHECK_VALUE( ret_val );	
	}


#ifdef ENABLE_BT  
#ifdef ENABLE_BT_PROTOCOL

	else if( p_res->_resource_type == BT_RESOURCE_TYPE )
	{
		ret_val = bt_resource_destroy( &p_res );
		CHECK_VALUE( ret_val );	
	}	
    
#endif /* #ifdef ENABLE_BT  */
#endif /* #ifdef ENABLE_BT  */
	
	return SUCCESS;
}

_int32 cm_destroy_all_cdn_pipes( CONNECT_MANAGER* p_connect_manager )
{
	_int32 ret_val = SUCCESS;
	LIST_ITERATOR cur_node, end_node, next_node;
	cur_node = LIST_BEGIN( p_connect_manager->_cdn_pipe_list._data_pipe_list );
	end_node = LIST_END( p_connect_manager->_cdn_pipe_list._data_pipe_list );
	LOG_DEBUG( "cm_destroy_pipe_list" );

	while( cur_node != end_node )
	{
		DATA_PIPE *p_data_pipe = LIST_VALUE( cur_node );

		ret_val = cm_destroy_single_cdn_pipe( p_connect_manager, p_data_pipe);
		CHECK_VALUE( ret_val );

		next_node = LIST_NEXT( cur_node );

		ret_val = list_erase( &(p_connect_manager->_cdn_pipe_list._data_pipe_list ), cur_node );
		CHECK_VALUE( ret_val );

		cur_node = next_node;
	}
	
	return SUCCESS;		
}

_int32 cm_close_all_cdn_manager_pipes( CONNECT_MANAGER* p_connect_manager )
{
	_int32 ret_val = SUCCESS;
	P2P_RESOURCE* peer_resource = NULL;
	LIST_ITERATOR cur_node, end_node, next_node;
	cur_node = LIST_BEGIN( p_connect_manager->_cdn_pipe_list._data_pipe_list );
	end_node = LIST_END( p_connect_manager->_cdn_pipe_list._data_pipe_list );
	LOG_DEBUG( "cm_close_all_cdn_manager_pipes" );

	while( cur_node != end_node )
	{
		DATA_PIPE *p_data_pipe = LIST_VALUE( cur_node );
		peer_resource = (P2P_RESOURCE*)p_data_pipe->_p_resource;
		if((p_data_pipe->_p_resource->_resource_type ==PEER_RESOURCE)&&(peer_resource->_from ==P2P_FROM_CDN))
		{
			ret_val = cm_destroy_single_cdn_pipe( p_connect_manager, p_data_pipe);
			CHECK_VALUE( ret_val );

			next_node = LIST_NEXT( cur_node );

			ret_val = list_erase( &(p_connect_manager->_cdn_pipe_list._data_pipe_list ), cur_node );
			CHECK_VALUE( ret_val );
		}
		else
		{
			next_node = LIST_NEXT( cur_node );
		}
		cur_node = next_node;
	}
	
	return SUCCESS;		
}

_int32 cm_destroy_single_cdn_pipe( CONNECT_MANAGER *p_connect_manager, DATA_PIPE *p_data_pipe )
{
    _int32 ret_val = SUCCESS;
    RESOURCE *p_res = p_data_pipe->_p_resource;	

    LOG_DEBUG( "cm_destroy_single_pipe:[dispatch_stat]pipe 0x%x delete, pipe_tpye:%d, pipe speed:%u, pipe_max_speed:%u, res_ptr:0x%x.res_retry time:%d, is_global_filtered:%d ", 
        p_data_pipe, p_data_pipe->_data_pipe_type, p_data_pipe->_dispatch_info._speed, 
        p_data_pipe->_dispatch_info._max_speed, p_data_pipe->_p_resource, 
        p_data_pipe->_p_resource->_retry_times, p_data_pipe->_dispatch_info._is_global_filted );

	if((p_data_pipe->_p_resource->_resource_type == PEER_RESOURCE) 
		&& p2p_get_from(p_res) == P2P_FROM_NORMAL_CDN)
	{
		P2P_RESOURCE* p2pres = (P2P_RESOURCE*)p_data_pipe->_p_resource;
		LOG_DEBUG( "cm_destroy_single_pipe: pipe_tpye:%d, pipe_ptr:0x%x,peerid=%s",
			p_data_pipe->_data_pipe_type, p_data_pipe, p2pres->_peer_id);

		return cm_destroy_single_normal_cdn_pipe(p_connect_manager, p_data_pipe);
	}

    if( p_data_pipe->_data_pipe_type == HTTP_PIPE )
    {
        ret_val = http_pipe_close( p_data_pipe );
        CHECK_VALUE( ret_val );
    }
    else if( p_data_pipe->_data_pipe_type == FTP_PIPE )
    {
        ret_val = ftp_pipe_close( p_data_pipe );
        CHECK_VALUE( ret_val );
    }
    else if( p_data_pipe->_data_pipe_type == P2P_PIPE )
    {
        ret_val = p2p_pipe_close( p_data_pipe );
        CHECK_VALUE( ret_val );		
    }
#ifdef ENABLE_BT 
#ifdef ENABLE_BT_PROTOCOL
    else if( p_data_pipe->_data_pipe_type == BT_PIPE )
    {
        ret_val = bt_pipe_close( p_data_pipe );
        CHECK_VALUE( ret_val );
    }	

#endif /* #ifdef ENABLE_BT  */
#endif /* #ifdef ENABLE_BT  */
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL
    else if(p_data_pipe->_data_pipe_type == EMULE_PIPE)
    {
        ret_val = emule_pipe_close(p_data_pipe);
        CHECK_VALUE(ret_val);
    }
#endif
#endif
    p_res->_pipe_num--;

    return SUCCESS;	
}

_int32 cm_stat_normal_cdn_use_time_helper(CONNECT_MANAGER *p_connect_manager, BOOL is_destroy)
{
	LOG_DEBUG("p_connect_manager:0x%x, is_destroy:%d.", p_connect_manager, is_destroy);

#ifdef EMBED_REPORT
	if (p_connect_manager->_report_para._res_normal_cdn_start_time != 0) {
		RANGE _time_span;
		_u32 now_time = 0;
		sd_time(&now_time);

		_u32 _use_time = now_time - p_connect_manager->_report_para._res_normal_cdn_start_time;

		if (is_destroy) {
			p_connect_manager->_report_para._res_normal_cdn_close_time = now_time;
			p_connect_manager->_report_para._res_normal_cdn_start_time = 0;
		}
		
		p_connect_manager->_report_para._res_normal_cdn_sum_use_time += _use_time;

		// 记录一个使用时间段到list中					
		_time_span._index = p_connect_manager->_report_para._res_normal_cdn_start_time;
		_time_span._num = _use_time;		

		_int32 tmp_ret = range_list_add_range(&(p_connect_manager->_report_para._res_normal_cdn_use_time_list), 
			&_time_span, NULL, NULL);

		LOG_DEBUG("cur connect manager:0x%x, time span:(%u, %u).", p_connect_manager,
			_time_span._index, _time_span._num);
	}
#endif
	return SUCCESS;
}

_int32 cm_destroy_single_normal_cdn_pipe(CONNECT_MANAGER *p_connect_manager, DATA_PIPE *p_data_pipe)
{
	_int32 ret_val = SUCCESS;
	RESOURCE *p_res = p_data_pipe->_p_resource;	
	_u32 now_time = 0;
	sd_time(&now_time);

	p_res->_close_normal_cdn_pipe_time = now_time;
	p_res->_last_pipe_connect_time = now_time - p_res->_open_normal_cdn_pipe_time;

	ret_val = cm_stat_normal_cdn_use_time_helper(p_connect_manager, TRUE);

	p_connect_manager->_main_task_ptr->_last_close_normal_cdn_time = now_time;

	LOG_DEBUG("now will close normal cdn pipe, now time:%u, last time:%u, last close pipe in task.", 
		now_time, p_connect_manager->_main_task_ptr->_last_close_normal_cdn_time);
	
	ret_val = p2p_pipe_close(p_data_pipe);
	CHECK_VALUE( ret_val );	

	p_res->_pipe_num--;

	return ret_val;
}

BOOL cm_is_cdn_pipe_idle(DATA_PIPE * p_pipe)
{
    _u32 now_time = 0;
    sd_time_ms(&now_time);
    if((p_pipe->_dispatch_info._pipe_state==PIPE_DOWNLOADING)
        &&(p_pipe->_dispatch_info._last_recv_data_time!=MAX_U32))
    {
        if((p_pipe->_dispatch_info._speed==0)
            &&(TIME_SUBZ(now_time, p_pipe->_dispatch_info._last_recv_data_time) > cm_pipe_speed_test_time()) )
        {
            LOG_ERROR( "cm_is_cdn_pipe_idle:last_recv_data_time=%u,now_time =%u",p_pipe->_dispatch_info._last_recv_data_time,now_time);
            return TRUE;
        }
    }
    return FALSE;
}

_int32 cm_update_cdn_pipes( CONNECT_MANAGER *p_connect_manager )
{
	_int32 ret_val = SUCCESS;
	LIST_ITERATOR cur_node, end_node,next_node;
	RESOURCE *p_res=NULL;
	P2P_RESOURCE* peer_resource = NULL;
	DATA_PIPE *p_data_pipe=NULL;

	LOG_DEBUG("cm_update_cdn_pipes:_cdn_mode=%d,res num=%u,pipes num=%u, enable_high_speed_channel:%d ",
			p_connect_manager->_cdn_mode,list_size(&p_connect_manager->_cdn_res_list._res_list),
			list_size(&p_connect_manager->_cdn_pipe_list._data_pipe_list),
			p_connect_manager->enable_high_speed_channel);
	
	cur_node = LIST_BEGIN( p_connect_manager->_cdn_pipe_list._data_pipe_list );
	end_node = LIST_END( p_connect_manager->_cdn_pipe_list._data_pipe_list );
    
	if (list_size(&(p_connect_manager->_cdn_pipe_list._data_pipe_list)) != 0)
    {
		LOG_DEBUG("there are some cdn pipes, size:%u.", 
			list_size(&(p_connect_manager->_cdn_pipe_list._data_pipe_list)));

        while( cur_node != end_node )
        {
            p_data_pipe = (DATA_PIPE *)LIST_VALUE( cur_node );

            BOOL is_from_pc = ((P2P_FROM_ASSIST_DOWNLOAD == cm_get_resource_from(p_data_pipe->_p_resource)) ? TRUE:FALSE);
            
            if(p_data_pipe->_dispatch_info._pipe_state==PIPE_DOWNLOADING)
            {
                cm_update_data_pipe_speed( p_data_pipe );
                
#if defined(MOBILE_PHONE)
            if(sd_get_net_type()<NT_3G)
            {
                if(!p_connect_manager->_cdn_ok) p_connect_manager->_cdn_ok = TRUE;
            }
#endif /* MOBILE_PHONE */
                
                if( p_data_pipe->_p_resource->_retry_times == 0
                    && p_data_pipe->_p_resource->_dispatch_state == NEW )
                {
#ifdef EMBED_REPORT
                    cm_valid_res_report_para( p_connect_manager, p_data_pipe->_p_resource );
#endif     
                    p_data_pipe->_p_resource->_dispatch_state = VALID;
                }
            }
            else
            {
                p_data_pipe->_dispatch_info._speed = 0;
            }
            
			///////////////////////////////////
			// 只是日志的功能？？TODO
            if(p_data_pipe->_p_resource->_resource_type == PEER_RESOURCE)
            {
                if(p_data_pipe->_data_pipe_type == P2P_PIPE)
                {
                    P2P_RESOURCE* peer_res = (P2P_RESOURCE* )p_data_pipe->_p_resource;

                    LOG_DEBUG("cm_update_cdn_pipes:peerid:%s, pipe_ptr:0x%x, speed:%u. pipe_state:%d",
                        peer_res->_peer_id,	p_data_pipe, p_data_pipe->_dispatch_info._speed, 
						p_data_pipe->_dispatch_info._pipe_state );
#ifdef _LOGGER
                    out_put_range_list_ex(peer_res->_peer_id, 
						"cm_update_cdn_pipes _can_download_ranges",  
						&p_data_pipe->_dispatch_info._can_download_ranges);
                    out_put_range_list_ex(peer_res->_peer_id, 
						"cm_update_cdn_pipes _uncomplete_ranges",  
						&p_data_pipe->_dispatch_info._uncomplete_ranges);
#endif

                    LOG_DEBUG( "cm_update_cdn_pipes:peerid:%s, assign(%u,%u), down (%u,%u)",
                        peer_res->_peer_id,
                        p_data_pipe->_dispatch_info._correct_error_range._index, 
						p_data_pipe->_dispatch_info._correct_error_range._num,
                        p_data_pipe->_dispatch_info._down_range._index, 
						p_data_pipe->_dispatch_info._down_range._num);

                }
                LOG_DEBUG("cm_update_cdn_pipes:p_data_pipe=0x%X,_pipe_state=%d,_speed=%u,err_code=%d,peer_id=%s",
					p_data_pipe,p_data_pipe->_dispatch_info._pipe_state,
					p_data_pipe->_dispatch_info._speed,
					p_data_pipe->_p_resource->_err_code,
					((P2P_RESOURCE*)p_data_pipe->_p_resource)->_peer_id);
            }
            else
            {
                LOG_ERROR( "cm_update_cdn_pipes:p_data_pipe=0x%X,_pipe_state=%d,_speed=%u,err_code=%d",
					p_data_pipe,p_data_pipe->_dispatch_info._pipe_state,
					p_data_pipe->_dispatch_info._speed,p_data_pipe->_p_resource->_err_code);
            }
			///////////////////////////////////

            
            if((p_data_pipe->_dispatch_info._pipe_state == PIPE_FAILURE) || 
			   (cm_is_cdn_pipe_idle(p_data_pipe) && !is_from_pc))
            {
				LOG_URGENT("prepare to deal with failure pipe:0x%x."
                    , p_data_pipe);

#ifdef EMBED_REPORT
				p_connect_manager->_report_para._cdn_connect_failed_times++;
#endif			
                p_data_pipe->_p_resource->_retry_times++;

                ret_val = cm_destroy_single_cdn_pipe( p_connect_manager, p_data_pipe);
                CHECK_VALUE( ret_val );

                next_node = LIST_NEXT( cur_node );

                ret_val = list_erase( &(p_connect_manager->_cdn_pipe_list._data_pipe_list ), cur_node );
                CHECK_VALUE( ret_val );

				cur_node = next_node;
            }
            else
            {
                if(p_data_pipe->_dispatch_info._pipe_state != PIPE_DOWNLOADING)
                {
                    p_data_pipe->_dispatch_info._speed=0;
                }

#ifdef ENABLE_HSC
                if(p_connect_manager->enable_high_speed_channel != TRUE)
                {
                    if(p_data_pipe->_p_resource->_resource_type == PEER_RESOURCE)
                    {
                        peer_resource = (P2P_RESOURCE*)(p_data_pipe->_p_resource);
                        if(peer_resource->_from == P2P_FROM_VIP_HUB)
                        {
                            LOG_DEBUG("on cm_update_cdn_pipes destroy hsc_data_pipe,cm:0x%x, res:0x%x, pipe:0x%x",p_connect_manager, peer_resource, p_data_pipe);
                            ret_val = cm_destroy_single_cdn_pipe( p_connect_manager, p_data_pipe);
                            CHECK_VALUE( ret_val );
                
                            next_node = LIST_NEXT( cur_node );
                
                            ret_val = list_erase( &(p_connect_manager->_cdn_pipe_list._data_pipe_list ), cur_node );
                            CHECK_VALUE( ret_val );

                            cur_node = next_node;
                            continue;
                        }
                    }
                }
#endif /* ENABLE_HSC */

                cur_node = LIST_NEXT( cur_node );
            }
        } // end while( cur_node != end_node )
    }
	else
	{
		if(p_connect_manager->_cdn_ok) p_connect_manager->_cdn_ok = FALSE;
	}

	cur_node = LIST_BEGIN( p_connect_manager->_cdn_res_list._res_list );
	end_node = LIST_END( p_connect_manager->_cdn_res_list._res_list);
    if (list_size(&(p_connect_manager->_cdn_res_list._res_list)) != 0)
    {
		LOG_DEBUG("there are some cdn resources.");
		int open=cm_can_open_normal_cdn_pipe(p_connect_manager) >= 0 ? 1 : 0;
        RESOURCE * p_zero_cdn_res=NULL;
        while( cur_node != end_node )
        {
            p_res =(RESOURCE *) LIST_VALUE( cur_node );

            BOOL is_from_pc = ((P2P_FROM_ASSIST_DOWNLOAD == cm_get_resource_from(p_res)) ? TRUE:FALSE);
            
            if(p_res->_resource_type == PEER_RESOURCE)
            {
                P2P_RESOURCE* res = (P2P_RESOURCE*)p_res;

                LOG_DEBUG( "cm_update_cdn_pipes:peerid:%s, pipe num:%d", res->_peer_id, p_res->_pipe_num);                
            }
            
            if((p_connect_manager->_cdn_mode == TRUE ) &&
			   (p_res->_pipe_num < 4) &&
			   (p_res->_resource_type == HTTP_RESOURCE))
            {
				ret_val = cm_create_single_cdn_pipe(p_connect_manager, p_res);
            }
            else
            {
                if(p_res->_pipe_num == 0)
                {
                    if( p_res->_resource_type == PEER_RESOURCE)
                    {  
                        peer_resource = (P2P_RESOURCE*)p_res;
                        if(p_res->_err_code != ERR_P2P_HANDSHAKE_RESP_FAIL)
                        {
                            if(peer_resource->_from == P2P_FROM_CDN) 
                            {
                                if(p_connect_manager->_cdn_mode==TRUE )
                                {
                                    ret_val = cm_create_single_cdn_pipe( p_connect_manager, p_res );
                                    //CHECK_VALUE( ret_val );
                                }
                            }
#ifdef ENABLE_HSC
                            else if(peer_resource->_from == P2P_FROM_VIP_HUB)
                            {
                                if(p_connect_manager->enable_high_speed_channel == TRUE)
                                {
                                    LOG_DEBUG("on cm_update_cdn_pipes creating hsc_data_pipe,cm:0x%x, res:0x%x",p_connect_manager, p_res);
                                    ret_val = cm_create_single_cdn_pipe( p_connect_manager, p_res );
                                }
                            }
#endif /* ENABLE_HSC */
							else if (peer_resource->_from == P2P_FROM_NORMAL_CDN)
							{
								if( 1==open) 
								{
									if(NULL==p_zero_cdn_res)
									{
										p_zero_cdn_res= p_res;
									}
									else
									{
										if(p_zero_cdn_res->_retry_times > p_res->_retry_times)
										{
											p_zero_cdn_res= p_res;
										}
									}
								}
							}
                            else if(p_connect_manager->_cdn_mode == TRUE)
                            {
                                ret_val = cm_create_single_cdn_pipe( p_connect_manager, p_res );
                            }
                            else if(is_from_pc)
                            {
                                ret_val = cm_create_single_cdn_pipe( p_connect_manager, p_res );
                            }
                        }
                    }
                    else if(p_connect_manager->_cdn_mode == TRUE)
                    {
                        ret_val = cm_create_single_cdn_pipe( p_connect_manager, p_res );	 
                    }
                }
            }
            cur_node = LIST_NEXT( cur_node );
        }
		if (NULL!=p_zero_cdn_res)
		{
			LOG_DEBUG("prepare for open normal cdn pipe.");
			ret_val = cm_create_single_normal_cdn_pipe(p_connect_manager, p_zero_cdn_res);
		}

    }

	return ret_val;
}

#ifdef ENABLE_HSC
_int32 cm_check_high_speed_channel_flag(const CONNECT_MANAGER *p_connect_manager, const _u32 file_index)
{
    _int32 ret_val = SUCCESS;
    CONNECT_MANAGER *p_sub_connection_manager = NULL;
    sd_assert(p_connect_manager);
	ret_val = cm_get_sub_connect_manager((CONNECT_MANAGER*)p_connect_manager, file_index, &p_sub_connection_manager);
    if((ret_val == SUCCESS) && p_sub_connection_manager)
    {
        LOG_DEBUG("on cm_check_high_speed_channel_flag connect_manager:0x%x, file_index:%u, result:%d", p_connect_manager, file_index, p_sub_connection_manager->enable_high_speed_channel);
        return p_sub_connection_manager->enable_high_speed_channel;
    }
    LOG_DEBUG("on cm_check_high_speed_channel_flag connect_manager:0x%x, file_index:%u, result:0", p_connect_manager, file_index);
    return FALSE;
}
#endif //ENABLE_HSC

BOOL cm_is_cdn_peer_res_exist( CONNECT_MANAGER *p_connect_manager, _u32 host_ip, _u16 port )
{
	//_int32 ret_val;
	LIST_ITERATOR p_list_node=NULL ;
	RESOURCE *p_cdn_res=NULL;
	

	for(p_list_node = LIST_BEGIN(p_connect_manager->_cdn_res_list._res_list); p_list_node != LIST_END(p_connect_manager->_cdn_res_list._res_list); p_list_node = LIST_NEXT(p_list_node))
	{
		p_cdn_res = (RESOURCE * )LIST_VALUE(p_list_node);

		if(p_cdn_res->_resource_type==PEER_RESOURCE)
		{
			if((((P2P_RESOURCE*)p_cdn_res)->_ip==host_ip)&&(((P2P_RESOURCE*)p_cdn_res)->_tcp_port==(_u32)port))
			{
			 	return TRUE;
			}
		}
	}
	
	return FALSE;
	
}

BOOL cm_is_cdn_peer_res_exist_by_peerid(void * v_connect_manager, const  char* peerid)
{
	LIST_ITERATOR p_list_node=NULL ;
	RESOURCE *p_cdn_res=NULL;

	BOOL ret = FALSE;

	CONNECT_MANAGER *p_connect_manager = (CONNECT_MANAGER *)v_connect_manager;
	

	for(p_list_node = LIST_BEGIN(p_connect_manager->_cdn_res_list._res_list); p_list_node != LIST_END(p_connect_manager->_cdn_res_list._res_list); p_list_node = LIST_NEXT(p_list_node))
	{
		p_cdn_res = (RESOURCE * )LIST_VALUE(p_list_node);

		if(p_cdn_res->_resource_type==PEER_RESOURCE)
		{
			if(strncmp(((P2P_RESOURCE*)p_cdn_res)->_peer_id, peerid,12)==0)
			{
			 	ret =  TRUE;
				break;
			}
		}
	}

	LOG_DEBUG( "cm_is_cdn_peer_res_exist_by_peerid:peerid:%s, ret=%d", peerid, ret);
	
	return ret;	
}


BOOL cm_filter_pipes_res_exist_cdnres(CONNECT_MANAGER* p_connect_manager, DATA_PIPE * data_pipe)
{
	BOOL ret = FALSE;
	if(data_pipe->_p_resource->_resource_type == PEER_RESOURCE)
	{
		P2P_RESOURCE* res = (P2P_RESOURCE*)data_pipe->_p_resource;

		LOG_DEBUG( "cm_is_cdn_peer_res_exist_by_peerid:peerid:%s", res->_peer_id);

		if(p2p_get_from(data_pipe->_p_resource) == P2P_FROM_ASSIST_DOWNLOAD)
		{
			LOG_DEBUG( "cm_is_cdn_peer_res_exist_by_peerid:peerid:%s from assit...", res->_peer_id);
		}
		else if(cm_is_cdn_peer_res_exist_by_peerid(p_connect_manager, res->_peer_id))
		{
			ret = TRUE;
		}
	}

	LOG_DEBUG( "cm_is_cdn_peer_res_exist_by_peerid:ret:%d", ret);

	return ret;
}

BOOL cm_filter_pipes_low_speed(CONNECT_MANAGER *manager, DATA_PIPE* data_pipe)
{
	BOOL ret = FALSE;
	if(data_pipe->_p_resource->_resource_type == PEER_RESOURCE)
	{
		P2P_RESOURCE* res = (P2P_RESOURCE*)data_pipe->_p_resource;

		LOG_DEBUG( "cm_filter_pipes_lowspeed:peerid:%s, current_speed:%d, pipe_speed:%d", 
			res->_peer_id, manager->_cur_peer_speed, data_pipe->_dispatch_info._speed);

		if(manager->_cur_peer_speed * 0.05 > data_pipe->_dispatch_info._speed)
		{
			ret = TRUE;
		}
	}


	LOG_DEBUG( "cm_filter_pipes_lowspeed:ret:%d", ret);

	return ret;
}

_int32 cm_destroy_normal_cdn_res(CONNECT_MANAGER *p_connect_manager, RESOURCE *p_res)
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "[dispatch_stat]normal cdn resource 0x%x delete", p_res);
	sd_assert( p_res->_pipe_num == 0 );
	sd_assert( p_res->_connecting_pipe_num == 0 );
	sd_assert( p_res != p_connect_manager->_origin_res_ptr );
	sd_assert( p_res->_resource_type == PEER_RESOURCE );

	if( p_res->_resource_type == PEER_RESOURCE )
	{
		ret_val = p2p_resource_destroy( &p_res );
		CHECK_VALUE( ret_val );	
	}

	return ret_val;
}

_int32 cm_destroy_normal_cdn_pipes(CONNECT_MANAGER *p_connect_manager)
{
	_int32 ret_val = SUCCESS;
	LIST_ITERATOR cur_node, end_node, next_node;

	cur_node = LIST_BEGIN(p_connect_manager->_cdn_pipe_list._data_pipe_list);
	end_node = LIST_END(p_connect_manager->_cdn_pipe_list._data_pipe_list);

	LOG_DEBUG("prepare to destroy normal cdn pipes of connect manager:0x%x.", p_connect_manager);

	while(cur_node != end_node)
	{
		DATA_PIPE *p_data_pipe = LIST_VALUE(cur_node);

		if (p_data_pipe->_p_resource->_resource_type == PEER_RESOURCE)
		{
			if (p2p_get_from(p_data_pipe->_p_resource) == P2P_FROM_NORMAL_CDN)
			{
				ret_val = cm_destroy_single_normal_cdn_pipe(p_connect_manager, p_data_pipe);
				CHECK_VALUE(ret_val);

				next_node = LIST_NEXT(cur_node);

				ret_val = list_erase( &(p_connect_manager->_cdn_pipe_list._data_pipe_list ), cur_node );
				CHECK_VALUE(ret_val);

				cur_node = next_node;

				continue;
			}
		}

		cur_node = LIST_NEXT(cur_node);		
	}

	return ret_val;
}

#endif /* ENABLE_CDN */

_u8 cm_get_resource_from(RESOURCE * res)
{
	_u8 ret = P2P_FROM_UNKNOWN;
	if(res->_resource_type == PEER_RESOURCE)
	{
		ret =  p2p_get_from(res);
	}

	return ret;
}

#ifdef EMBED_REPORT
void cm_add_peer_res_report_para( CONNECT_MANAGER *p_connect_manager, _u8 capability, _u8 from )
{
	CM_REPORT_PARA *p_report_para = &p_connect_manager->_report_para;
	BOOL is_loacl_nate = sd_is_in_nat(sd_get_local_ip());
	
	p_report_para->_res_peer_total++;

	switch(from)
	{
		case P2P_FROM_TRACKER:
		{
			p_report_para->_active_tracker_res_total++;
			break;
		}
		case P2P_FROM_HUB:
		{
			p_report_para->_hub_res_total++;
			break;
		}
		case P2P_FROM_VIP_HUB:
		{
			p_report_para->_res_cdn_total++;
			break;
		}
		case P2P_FROM_PARTNER_CDN:
		{
			p_report_para->_res_partner_cdn_total++;
			break;
		}
		case P2P_FROM_NORMAL_CDN:
		{
			p_report_para->_res_normal_cdn_total++;
			break;
		}
		default:
			break;
	}

	if( is_loacl_nate )
	{
		if( !is_nated(capability) )
		{
			p_report_para->_res_n2i_total++;
		}	
		else
		{
			p_report_para->_res_n2n_total++;
			if( is_same_nat(capability) )
			{
				p_report_para->_res_n2s_total++;
			}
		}
	}
	else
	{
		if( !is_nated(capability) )
		{
			p_report_para->_res_i2i_total++;
		}
		else
		{
			p_report_para->_res_i2n_total++;
		}
	}	
}

void cm_valid_res_report_para( CONNECT_MANAGER *p_connect_manager, RESOURCE *p_res )
{
	CM_REPORT_PARA *p_report_para = &p_connect_manager->_report_para;
	BOOL is_loacl_nate = sd_is_in_nat(sd_get_local_ip());
	_u8 capability = 0;
	_u8 from = 0;

	if( p_res->_resource_type == HTTP_RESOURCE
		|| p_res->_resource_type == FTP_RESOURCE )
	{
		p_report_para->_res_server_valid++;
		return;
	}	
	
	if(p_res->_resource_type != PEER_RESOURCE)
	{
		return;
	}	
	
	capability = p2p_get_capability( p_res );
	from = p2p_get_from( p_res );

	switch(from)
	{
		case P2P_FROM_TRACKER:
		{
			p_report_para->_active_tracker_res_valid++;
			break;
		}
		case P2P_FROM_HUB:
		{
			p_report_para->_hub_res_valid++;
			break;
		}
		case P2P_FROM_VIP_HUB:
		{
			p_report_para->_res_cdn_valid++;
			break;
		}
		case P2P_FROM_PARTNER_CDN:
		{
			p_report_para->_res_partner_cdn_valid++;
			break;
		}
		case P2P_FROM_NORMAL_CDN:
		{
			p_report_para->_res_normal_cdn_valid++;
			break;
		}
		default:
			break;
	}

	if( is_loacl_nate )
	{
		if( !is_nated(capability) )
		{
			p_report_para->_res_n2i_valid++;
		}	
		else
		{
			p_report_para->_res_n2n_valid++;
			if( is_same_nat(capability) )
			{
				p_report_para->_res_n2s_valid++;
			}
		}
	}
	else
	{
		if( !is_nated(capability) )
		{
			p_report_para->_res_i2i_valid++;
		}
		else
		{
			p_report_para->_res_i2n_valid++;
		}
	}		
}

#endif



