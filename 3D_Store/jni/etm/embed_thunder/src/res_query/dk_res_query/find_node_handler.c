#include "utility/define.h"
#ifdef _DK_QUERY

#include "find_node_handler.h"
#include "dk_socket_handler.h"
#include "routing_table.h"
#include "dk_setting.h"

#include "utility/define_const_num.h"
#include "utility/mempool.h"
#include "utility/bytebuffer.h"
#include "utility/utility.h"

#include "utility/logid.h"
#define LOGID LOGID_DK_QUERY
#include "utility/logger.h"

static SLAB* g_find_node_handler_slab = NULL;

_int32 fnh_create( FIND_NODE_HANDLER **pp_find_node_handler )
{
	LOG_DEBUG( "fnh_create." );
	return find_node_handler_malloc_wrap( pp_find_node_handler );
}

_int32 fnh_init( _u32 dk_type, FIND_NODE_HANDLER *p_find_node_handler, _u8 *p_target, _u32 target_len, _u8 find_type )
{
	_int32 ret_val = SUCCESS;
	
	LOG_DEBUG( "fnh_init.p_find_node_handler:0x%x, dk_type:%u, find_type:%u", p_find_node_handler, dk_type, find_type );
	
	p_find_node_handler->_dk_type = dk_type;
	p_find_node_handler->_find_type = find_type;
	p_find_node_handler->_idle_ticks = 0;
	p_find_node_handler->_resp_node_num = 0;

	if( dk_type == DHT_TYPE )
	{
		p_find_node_handler->_protocal_handler._response_handler = dht_find_node_response_handler;
		p_find_node_handler->_cmd_builder = dht_build_find_node_cmd;
		p_find_node_handler->_node_creater = dht_node_info_creater;
		p_find_node_handler->_node_destoryer = dht_node_info_destoryer;
		p_find_node_handler->_get_nearest_node_max_num = dht_get_nearest_node_max_num();
		p_find_node_handler->_filter_low_limit = dht_node_filter_low_limit();
        
	}
    
#ifdef ENABLE_EMULE
	else if( dk_type == KAD_TYPE )
	{
		p_find_node_handler->_protocal_handler._response_handler = kad_find_node_response_handler;
		p_find_node_handler->_cmd_builder = kad_build_find_node_cmd;
		p_find_node_handler->_node_creater = kad_node_info_creater;
		p_find_node_handler->_node_destoryer = kad_node_info_destoryer;
		p_find_node_handler->_get_nearest_node_max_num = kad_get_nearest_node_max_num();
		p_find_node_handler->_filter_low_limit = kad_node_filter_low_limit();
	}
#endif
	else
	{
		sd_assert( FALSE );
	}

	if( target_len != 0 )
	{
		ret_val = k_distance_init_with_char_buffer( &p_find_node_handler->_target, p_target, target_len );
		if( ret_val != SUCCESS )
		{
			find_node_handler_free_wrap( p_find_node_handler );
			return ret_val;
		}		
	}
	else
	{
		k_distance_init( &p_find_node_handler->_target, 0 );
		if( ret_val != SUCCESS )
		{
			find_node_handler_free_wrap( p_find_node_handler );
			return ret_val;
		}	
	}

	list_init( &p_find_node_handler->_unqueryed_node_info_list );
	
	set_init( &p_find_node_handler->_near_node_key, find_node_set_compare );
	p_find_node_handler->_status = FIND_NODE_INIT;
	p_find_node_handler->_nearst_dist_ptr = NULL;
	p_find_node_handler->_middle_dist_ptr = NULL;
	p_find_node_handler->_near_node_count = 0;
#ifdef _LOGGER
	p_find_node_handler->_nearst_ip = 0;
    p_find_node_handler->_near_id_ptr = NULL;
#endif  
	return SUCCESS;	
}

_int32 fnh_uninit( FIND_NODE_HANDLER *p_find_node_handler )
{
	LIST_ITERATOR cur_node = NULL;
	NODE_INFO *p_node_info = NULL;
	LOG_DEBUG( "fnh_uninit.p_find_node_handler:0x%x, _status:%u, find_type:%u",
		p_find_node_handler, p_find_node_handler->_status, p_find_node_handler->_find_type );

	if( p_find_node_handler->_status == FIND_NODE_FINISHED ) return SUCCESS;
	
#ifdef _DEBUG
    k_distance_print( &p_find_node_handler->_target );
#endif

	k_distance_uninit( &p_find_node_handler->_target );

	cur_node = LIST_BEGIN( p_find_node_handler->_unqueryed_node_info_list );
	while( cur_node != LIST_END( p_find_node_handler->_unqueryed_node_info_list ) )
	{
		p_node_info = (NODE_INFO*)LIST_VALUE( cur_node );
		p_find_node_handler->_node_destoryer( p_node_info );
        
		cur_node = LIST_NEXT( cur_node );
	}
    list_clear( &p_find_node_handler->_unqueryed_node_info_list );
	set_clear( &p_find_node_handler->_near_node_key );
	if( p_find_node_handler->_nearst_dist_ptr != NULL )
	{
		k_distance_destory( &p_find_node_handler->_nearst_dist_ptr );
	}
 	if( p_find_node_handler->_middle_dist_ptr != NULL )
	{
		k_distance_destory( &p_find_node_handler->_middle_dist_ptr );
	}   
	p_find_node_handler->_near_node_count = 0;
#ifdef _LOGGER
	p_find_node_handler->_nearst_ip = 0;
    SAFE_DELETE( p_find_node_handler->_near_id_ptr );
#endif      
	p_find_node_handler->_status = FIND_NODE_FINISHED;
    
	p_find_node_handler->_idle_ticks = 0;
	p_find_node_handler->_resp_node_num = 0;
    
	sh_clear_resp_handler( sh_ptr(p_find_node_handler->_dk_type), &p_find_node_handler->_protocal_handler );
	return SUCCESS;	
}

_int32 fnh_destory( FIND_NODE_HANDLER *p_find_node_handler )
{
	LOG_DEBUG( "fnh_destory.p_find_node_handler:0x%x", p_find_node_handler);
	find_node_handler_free_wrap( p_find_node_handler );
	return SUCCESS;
}

void find_node_set_build_cmd_handler( FIND_NODE_HANDLER *p_find_node_handler, find_node_cmd_builder cmd_builder )
{
	LOG_DEBUG( "find_node_set_build_cmd_handler.p_find_node_handler:0x%x", p_find_node_handler);
	p_find_node_handler->_cmd_builder = cmd_builder;
}

void find_node_set_response_handler( FIND_NODE_HANDLER *p_find_node_handler, dk_protocal_response_handler resp_handler)
{
	LOG_DEBUG( "find_node_set_response_handler.p_find_node_handler:0x%x", p_find_node_handler);
	p_find_node_handler->_protocal_handler._response_handler = resp_handler;
}

_int32 dht_find_node_response_handler( DK_PROTOCAL_HANDLER *p_handler, _u32 ip, _u32 port, _u32 invalid_para, void *p_data )
{
	_int32 ret_val = SUCCESS;
	FIND_NODE_HANDLER *p_find_node_handler = (FIND_NODE_HANDLER *)p_handler;
	BC_DICT *p_r_dict = ( BC_DICT * )p_data;
	BC_STR *p_nodes_str = NULL;
	_u32 find_ip = 0;
	_u16 find_port = 0;
	_u32 str_len = 0;
	_u32 node_num = 0;
	_u32 node_index = 0;
	_u8 node_id[DHT_ID_LEN];
    char *p_tmp_str = NULL;
    
#ifdef _DEBUG
    char addr[24] = {0};
    sd_inet_ntoa( ip, addr, 24);
    LOG_DEBUG( "dht_find_node_response_handler.p_find_node_handler:0x%x, ip:%s, port:%d", 
        p_find_node_handler, addr, port );
#endif
    sd_assert( p_find_node_handler->_status != FIND_NODE_FINISHED );

	ret_val = bc_dict_get_value( p_r_dict, "nodes", (BC_OBJ **)&p_nodes_str );
	CHECK_VALUE( ret_val );	

    if( p_nodes_str == NULL ) return SUCCESS;
    
	str_len = p_nodes_str->_str_len;
    p_tmp_str = p_nodes_str->_str_ptr;
    
	if( str_len % DHT_COMPACT_NODE_SIZE != 0 )
	{
		//sd_assert( FALSE );
		return SUCCESS; 
	}

	node_num = str_len / DHT_COMPACT_NODE_SIZE;
	while( node_index < node_num )
	{
		sd_memset( node_id, 0, DHT_ID_LEN );
		ret_val = sd_get_bytes( &p_tmp_str, (_int32 *)&str_len, (char *)node_id, DHT_ID_LEN );
		CHECK_VALUE( ret_val );

		ret_val = sd_get_int32_from_bg( &p_tmp_str, (_int32 *)&str_len, (_int32 *)&find_ip );
		CHECK_VALUE( ret_val );	
		
		ret_val = sd_get_int16_from_bg( &p_tmp_str, (_int32 *)&str_len, (_int16 *)&find_port);
		CHECK_VALUE( ret_val );	
        
        find_ip  = sd_htonl( find_ip );
        rt_ping_node( DHT_TYPE, find_ip, find_port, 0, FALSE );

		ret_val = fnh_handler_new_peer( p_find_node_handler, find_ip, find_port, 0, node_id, DHT_ID_LEN );
		CHECK_VALUE( ret_val );	
		
		node_index++;
	}
	sd_assert( str_len == 0 );
    
    find_node_remove_node_info( p_find_node_handler, ip, port );
	return SUCCESS;	
}

_int32 kad_find_node_response_handler( DK_PROTOCAL_HANDLER *p_handler, _u32 ip, _u32 port, _u32 version, void *p_data )
{
	_int32 ret_val = SUCCESS;
	FIND_NODE_HANDLER *p_find_node_handler = (FIND_NODE_HANDLER *)p_handler;
    LIST *p_node_list = (LIST *)p_data;
    LIST_ITERATOR cur_node = NULL;
    KAD_NODE *p_kad_node = NULL;
    K_NODE *p_k_node = NULL;
    _u8 *p_id = NULL;
    _u32 id_len = 0;
    
#ifdef _LOGGER
    char addr[24] = {0};
    char addr_1[24] = {0};
    sd_inet_ntoa( ip, addr, 24);
    LOG_DEBUG( "kad_find_node_response_handler.p_find_node_handler:0x%x, ip:%s, port:%d, find_num:%d", 
        p_find_node_handler, addr, port, list_size(p_node_list) );
#endif
    sd_assert( p_find_node_handler->_status != FIND_NODE_FINISHED );

    cur_node = LIST_BEGIN( *p_node_list );
    while( cur_node != LIST_END( *p_node_list ) )
    {
        p_kad_node = LIST_VALUE( cur_node );
        p_k_node = (K_NODE *)p_kad_node;
        p_id = k_distance_get_bit_buffer( &p_k_node->_node_id );
        id_len = k_distance_buffer_len( &p_k_node->_node_id );
        
#ifdef _LOGGER
    sd_inet_ntoa( p_k_node->_peer_addr._ip, addr_1, 24);
    LOG_DEBUG( "kad_find_node_response_handler.p_find_node_handler:0x%x, ip:%s, port:%d, version:%d kad find node!!!: ip:%s, port:%d", 
        p_find_node_handler, addr, port, version, addr_1, p_k_node->_peer_addr._port );
#endif        
		ret_val = fnh_handler_new_peer( p_find_node_handler, p_k_node->_peer_addr._ip, p_k_node->_peer_addr._port, 
          p_kad_node->_version, p_id, id_len );
		CHECK_VALUE( ret_val );	
        
        cur_node = LIST_NEXT( cur_node );
    }
    find_node_remove_node_info( p_find_node_handler, ip, port );
	return SUCCESS;	
}

_int32 fnh_handler_new_peer( FIND_NODE_HANDLER *p_find_node_handler,
	_u32 ip, _u16 port, _u8 version, _u8 *p_id, _u32 id_len )
{
	_int32 ret_val = SUCCESS;
	K_DISTANCE *p_distance = NULL;
	K_NODE_ID *p_k_node_id = NULL;
    _int32 comp_result = 0;
    _u32 near_node_size = set_size( &p_find_node_handler->_near_node_key );
    _u32 filter_cycle = dk_filter_cycle();
    
#ifdef _LOGGER
    char addr_1[24] = {0};
    char addr_2[24] = {0};
    char test_buffer[1024];
    _u8 *p_target_id = NULL;
    _u8 *p_nearst_id = NULL;
    
    _u32 queryed_node_size = 0;

    _u64 dist_value = 0;
    _u64 near_dist_value = 0;

    sd_inet_ntoa(ip, addr_1, 24);
    sd_inet_ntoa(p_find_node_handler->_nearst_ip, addr_2, 24);
    LOG_DEBUG( "fnh_handler_new_peer print nearest info!!!, p_find_node_handler:0x%x, _nearst_dist_ptr:0x%x, "
               "nearst_ip:%s, new ip:%s, port:%u", 
        p_find_node_handler, p_find_node_handler->_nearst_dist_ptr, addr_2,  addr_1, port );

    p_target_id = k_distance_get_bit_buffer( &p_find_node_handler->_target );
    sd_memset(test_buffer,0,1024);
    str2hex( (char *)p_target_id, KAD_ID_LEN, test_buffer, 1024);

    LOG_DEBUG( "fnh_handler_new_peer print nearest info!!!, p_find_node_handler:0x%x, target_id:%s", 
        p_find_node_handler, test_buffer );

    if( p_find_node_handler->_nearst_dist_ptr != NULL )
    {
        sd_memset(test_buffer,0,1024);
        str2hex( (char *)p_find_node_handler->_near_id_ptr, KAD_ID_LEN, test_buffer, 1024);
        
        LOG_DEBUG( "fnh_handler_new_peer print nearest info!!!, p_find_node_handler:0x%x, nearst_id:%s", 
            p_find_node_handler, test_buffer );
        
        p_nearst_id = k_distance_get_bit_buffer( p_find_node_handler->_nearst_dist_ptr );
        sd_memset(test_buffer,0,1024);
        str2hex( (char *)p_nearst_id, KAD_ID_LEN, test_buffer, 1024);
                
        LOG_DEBUG( "fnh_handler_new_peer print nearest info!!!, p_find_node_handler:0x%x, nearstdis:%s", 
            p_find_node_handler, test_buffer );       
    }
    
    sd_memset(test_buffer,0,1024);
    str2hex( (char *)p_id, KAD_ID_LEN, test_buffer, 1024);
    LOG_DEBUG( "fnh_handler_new_peer print nearest info!!!, p_find_node_handler:0x%x, addnew_id:%s", 
        p_find_node_handler, test_buffer );

#endif	

    if( near_node_size > dk_find_node_max_num() ) return SUCCESS;

    p_find_node_handler->_idle_ticks = 0;

	ret_val = k_distance_create( &p_distance );
	CHECK_VALUE( ret_val );	

	ret_val = k_distance_create_with_char_buffer( p_id, id_len, &p_k_node_id );
	if( ret_val != SUCCESS )
	{
        sd_assert( FALSE );
		k_distance_destory( &p_distance );
		return ret_val;	
	}
	
	ret_val = k_distance_xor( &p_find_node_handler->_target, p_k_node_id, p_distance );
	CHECK_VALUE( ret_val );	
    
#ifdef _LOGGER
    LOG_DEBUG( "fnh_handler_new_peer begin print distance and _nearst_dist_ptr, p_find_node_handler:0x%x, ip:%s, port:%u, print distance:!!!", 
        p_find_node_handler, addr_1, port );
    /*k_distance_print(p_distance);
    if( p_find_node_handler->_nearst_dist_ptr != NULL )
    {
        k_distance_print(p_find_node_handler->_nearst_dist_ptr);
    }
    */
#endif	

	k_distance_destory( &p_k_node_id );

	if( p_find_node_handler->_nearst_dist_ptr == NULL )
	{
        ret_val = fnh_add_node_with_id( p_find_node_handler, ip,  port, version, p_id, id_len );
        if( ret_val != SUCCESS )
        {
            sd_assert( FALSE );
            k_distance_destory( &p_distance );
            return ret_val;
        }
		p_find_node_handler->_nearst_dist_ptr = p_distance;	
#ifdef _LOGGER
        p_find_node_handler->_nearst_ip = ip;
        sd_malloc( id_len, (void **)&p_find_node_handler->_near_id_ptr );
        sd_memcpy( p_find_node_handler->_near_id_ptr, p_id, id_len);
#endif	
        return SUCCESS;
	}

    sd_assert( p_find_node_handler->_nearst_dist_ptr != NULL );
    ret_val = k_distance_compare( p_distance, p_find_node_handler->_nearst_dist_ptr, &comp_result );
    if( ret_val != SUCCESS )
    {   
        sd_assert( FALSE );
        k_distance_destory( &p_distance );
        return ret_val;	
    }   
    
    
#ifdef _LOGGER
    dist_value = k_distance_get_value(p_distance);
    if( p_find_node_handler->_nearst_dist_ptr != NULL )
    {
        near_dist_value = k_distance_get_value(p_find_node_handler->_nearst_dist_ptr);
    }
    queryed_node_size = set_size( &p_find_node_handler->_near_node_key );
    LOG_DEBUG( "fnh_handler_new_peer, p_find_node_handler:0x%x, comp_result:%d, dist_value:%llu,near_dist_value:%llu, queryed_node_size:%u,_resp_node_num:%u, filter_low_limit:%u", 
         p_find_node_handler, comp_result, dist_value, near_dist_value, queryed_node_size, p_find_node_handler->_resp_node_num, p_find_node_handler->_filter_low_limit );
#endif	
	//if( p_find_node_handler->_resp_node_num > dk_find_node_filter_low_limit()
    if( p_find_node_handler->_resp_node_num > p_find_node_handler->_filter_low_limit
        && comp_result > 0 )
	{
     	LOG_DEBUG( "fnh_handler_new_peer, p_find_node_handler:0x%x, too far add_failed!!! ip:%s, port:%u, _resp_node_num:%u", 
            p_find_node_handler, addr_1, port, p_find_node_handler->_resp_node_num );       
		k_distance_destory( &p_distance );
		return SUCCESS;	
	}

    ret_val = fnh_add_node_with_id( p_find_node_handler, ip,  port, version, p_id, id_len );
    if( ret_val != SUCCESS )
    {
        sd_assert( FALSE );
		k_distance_destory( &p_distance );
        return ret_val;
    }
#ifdef _LOGGER
        LOG_DEBUG( "fnh_handler_new_peer, p_find_node_handler:0x%x, ip:%s, port:%u, print distance:!!!", 
            p_find_node_handler, addr_1, port );
#endif	

	if( comp_result >= 0 )
	{
		k_distance_destory( &p_distance );
        return SUCCESS; 
	}  
    
    sd_assert( filter_cycle != 0 );
    p_find_node_handler->_near_node_count++;
    
    if( p_find_node_handler->_near_node_count % ( 2 * filter_cycle )== 0 )
    {
        if( p_find_node_handler->_nearst_dist_ptr != NULL )
        {
            k_distance_destory( &p_find_node_handler->_nearst_dist_ptr );
        }
        ret_val = k_distance_create( &p_find_node_handler->_nearst_dist_ptr );
        CHECK_VALUE( ret_val );	
        sd_assert( p_find_node_handler->_middle_dist_ptr != NULL );
        LOG_DEBUG( "fnh_handler_new_peer, set nearst dist!!!, _near_node_count:%d", p_find_node_handler->_near_node_count );
        k_distance_copy_construct( p_find_node_handler->_middle_dist_ptr, p_find_node_handler->_nearst_dist_ptr );
    }  
    
    if( p_find_node_handler->_near_node_count % filter_cycle  == 0 )
    {
        if( p_find_node_handler->_middle_dist_ptr != NULL )
        {
            k_distance_destory( &p_find_node_handler->_middle_dist_ptr );
        }
        LOG_DEBUG( "fnh_handler_new_peer, set middle dist!!!, _near_node_count:%d", p_find_node_handler->_near_node_count );
		p_find_node_handler->_middle_dist_ptr = p_distance;
    }
    else
    {
        k_distance_destory( &p_distance );
    }
    
#ifdef _LOGGER
    p_find_node_handler->_nearst_ip = ip;
#endif	

	return SUCCESS;	
}

void fnh_update( FIND_NODE_HANDLER *p_find_node_handler )
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "fnh_update.p_find_node_handler:0x%x, status:%d", 
     p_find_node_handler, p_find_node_handler->_status );

	switch( p_find_node_handler->_status )
	{
		case FIND_NODE_INIT:
		{
			ret_val = fnh_find_target_from_rt( p_find_node_handler );
			if( ret_val != SUCCESS ) 
			{
                fnh_find( p_find_node_handler );
                return;
			}
			p_find_node_handler->_status = FIND_NODE_RUNNING;
		}
		case FIND_NODE_RUNNING:
		{
			fnh_find( p_find_node_handler );
		}
		case FIND_NODE_FINISHED:
			break;
		default:
			sd_assert( FALSE );
			break;
	}
}

_int32 fnh_find_target_from_rt( FIND_NODE_HANDLER *p_find_node_handler )
{
	_int32 ret_val = SUCCESS;
	LIST nearest_node_list;
	LIST_ITERATOR cur_node = NULL;
	_u32 max_node_num = p_find_node_handler->_get_nearest_node_max_num;
	K_NODE *p_k_node = NULL;
	KAD_NODE *p_kad_node = NULL;
	K_NODE_ID *p_k_node_id = NULL;
    _u8 version = 0;
    _u8 *p_id = NULL;
    _u32 id_len = 0;
	
	LOG_DEBUG( "fnh_find_target_from_rt begin.p_find_node_handler:0x%x", p_find_node_handler);
	
	list_init( &nearest_node_list );
	
	ret_val = rt_get_nearest_nodes( rt_ptr(p_find_node_handler->_dk_type), &p_find_node_handler->_target, &nearest_node_list, &max_node_num );
	CHECK_VALUE( ret_val );

	cur_node = LIST_BEGIN( nearest_node_list );
	while( cur_node != LIST_END( nearest_node_list ) )
	{
		p_k_node = LIST_VALUE( cur_node );
		p_k_node_id = &p_k_node->_node_id;
        p_id = k_distance_get_bit_buffer(p_k_node_id);
        id_len = k_distance_buffer_len(p_k_node_id);
        
        if( p_find_node_handler->_dk_type == KAD_TYPE )
        {
            p_kad_node = ( KAD_NODE *)p_k_node;
            version = p_kad_node->_version;
        }
 		ret_val = fnh_add_node_with_id( p_find_node_handler, p_k_node->_peer_addr._ip, 
           p_k_node->_peer_addr._port, version, p_id, id_len );
		if( ret_val != SUCCESS )
		{
			list_clear( &nearest_node_list );
			return ret_val;
		}

		cur_node = LIST_NEXT( cur_node );
	}
	
	list_clear( &nearest_node_list );
    
	LOG_DEBUG( "fnh_find_target_from_rt end.p_find_node_handler:0x%x, num:%u, _resp_node_num:%u", 
        p_find_node_handler, max_node_num, p_find_node_handler->_resp_node_num );
    //if( list_size(&p_find_node_handler->_unqueryed_node_info_list) < dk_find_node_from_rt_low_limit()
    //    && !rt_is_ready(p_find_node_handler->_dk_type) )
    if( p_find_node_handler->_resp_node_num < dk_find_node_from_rt_low_limit()
        && !rt_is_ready(p_find_node_handler->_dk_type) )      
    {
        LOG_DEBUG( "fnh_find_target_from_rt end.p_find_node_handler:0x%x, unqueryed_node num is lack:%u", 
            p_find_node_handler, list_size(&p_find_node_handler->_unqueryed_node_info_list) );
        return -1;
    }
	return SUCCESS;	
}

_int32 fnh_add_node_with_id( FIND_NODE_HANDLER *p_find_node_handler, _u32 ip, _u16 port, _u8 version, 
                            _u8 *p_id, _u32 id_len )
{
    _int32 ret_val = SUCCESS;
	_u32 key = 0;
    SET_ITERATOR cur_node = NULL;
    
#ifdef _DEBUG
    char addr[24] = {0};
    sd_inet_ntoa(ip, addr, 24);
    LOG_DEBUG( "fnh_add_node with_id.p_find_node_handler:0x%x, ip:%s, port:%d", 
        p_find_node_handler, addr, port );
#endif

    key = dk_calc_key( ip, port, p_id, id_len );

	ret_val = set_find_iterator( &p_find_node_handler->_near_node_key, (void *)key, &cur_node );
	CHECK_VALUE( ret_val );	

	if( cur_node != SET_END( p_find_node_handler->_near_node_key ) )
    {
        
#ifdef _DEBUG
     	LOG_DEBUG( "fnh_add_node with_id, p_find_node_handler:0x%x, ip:%s, port:%u, key:%u exist!!!", 
            p_find_node_handler, addr, port, key );  
#endif
        return SUCCESS; 
    }
    
    ret_val = fnh_add_node( p_find_node_handler, ip,  port, version, p_id, id_len );
    if( ret_val != SUCCESS )
    {
        sd_assert( FALSE );
        return ret_val;
    }
#ifdef _DEBUG
 	LOG_DEBUG( "fnh_add_node with_id, p_find_node_handler:0x%x, ip:%s, port:%u, key:%u success!!!", 
        p_find_node_handler, addr, port, key );      
#endif
    set_insert_node( &p_find_node_handler->_near_node_key, (void *)key );  
    return SUCCESS;
}

_int32 fnh_add_node( FIND_NODE_HANDLER *p_find_node_handler, _u32 ip, _u16 port, _u8 version, _u8 *p_id, _u32 id_len  )
{
	NODE_INFO *p_node_info = NULL;
    _int32 ret_val = SUCCESS;

#ifdef _DEBUG
    char addr[24] = {0};
    sd_inet_ntoa(ip, addr, 24);
    LOG_DEBUG( "fnh_add_node.p_find_node_handler:0x%x, ip:%s, port:%d", 
        p_find_node_handler, addr, port );
#endif
    
	ret_val = p_find_node_handler->_node_creater( ip, port, version, p_id, id_len, &p_node_info );
	CHECK_VALUE( ret_val );

	ret_val = list_push( &p_find_node_handler->_unqueryed_node_info_list, (void *)p_node_info );
	if( ret_val != SUCCESS )
	{
		p_find_node_handler->_node_destoryer( p_node_info );
		return ret_val;   
	}
    return SUCCESS;
}

void fnh_find( FIND_NODE_HANDLER *p_find_node_handler )
{
	_int32 ret_val = SUCCESS;
	NODE_INFO *p_node_info = NULL;
	_u32 find_num = 0;
	char *p_buffer = NULL;
	_u32 buffer_len = 0;
	DK_SOCKET_HANDLER *p_dk_socket_handler = NULL;
    LIST_ITERATOR cur_node = NULL;
	_u32 key = 0;
    _u32 unquery_node_num = list_size( &p_find_node_handler->_unqueryed_node_info_list);

#ifdef _LOGGER
    char addr[24] = {0};
    _u8 version = 0;
    KAD_NODE_INFO *p_kad_node_info = NULL;
    
    _u32 near_node_size = set_size( &p_find_node_handler->_near_node_key );

	LOG_DEBUG( "fnh_find.p_find_node_handler:0x%x, unquery_node_num:%d, near_node_size:%u, resp_num:%u ticks:%u", 
        p_find_node_handler, unquery_node_num, near_node_size, p_find_node_handler->_resp_node_num, p_find_node_handler->_idle_ticks );
#endif	

	p_find_node_handler->_idle_ticks++;
	if( p_find_node_handler->_idle_ticks > dk_find_node_idle_count() )
	{
        LOG_DEBUG( "fnh_find.fnh_uninit!!!" );
		fnh_uninit( p_find_node_handler );
		return;
	}    
    
    if( unquery_node_num == 0 )
    {
        return;
    }
    p_find_node_handler->_idle_ticks = 0;

	p_dk_socket_handler = sh_ptr( p_find_node_handler->_dk_type );
	while( find_num < MIN( dk_once_find_node_num(), unquery_node_num ) )
	{
		ret_val = list_pop( &p_find_node_handler->_unqueryed_node_info_list, (void **)&p_node_info );
		if( ret_val != SUCCESS || p_node_info == NULL )
		{
			sd_assert( FALSE );
			return;
		}
        
#ifdef _LOGGER
        sd_inet_ntoa(p_node_info->_peer_addr._ip, addr, 24);
        LOG_DEBUG( "fnh_find_node, p_find_node_handler:0x%x, p_node_info:0x%x, ip:%s, port:%u", 
            p_find_node_handler, p_node_info, addr, p_node_info->_peer_addr._port ); 
#endif	    
        
		ret_val = p_find_node_handler->_cmd_builder( p_node_info, &p_find_node_handler->_target,
			p_find_node_handler->_find_type, &p_buffer, &buffer_len, &key );
		if( ret_val != SUCCESS )
		{
			sd_assert( FALSE );
			return;
		}

		ret_val = sh_send_packet( p_dk_socket_handler, p_node_info->_peer_addr._ip,
			p_node_info->_peer_addr._port, p_buffer, buffer_len, &p_find_node_handler->_protocal_handler, key );
        if( ret_val == DK_SIMI_FILE_ID )
        {
            LOG_DEBUG( "fnh_find DK_SIMI_FILE_ID.fnh_uninit!!!" );
            p_find_node_handler->_node_destoryer( p_node_info );
            fnh_uninit( p_find_node_handler );
            return;
        }
        else if( ret_val != SUCCESS ) 
		{
			SAFE_DELETE( p_buffer );
            cur_node = LIST_BEGIN( p_find_node_handler->_unqueryed_node_info_list );
            list_insert( &p_find_node_handler->_unqueryed_node_info_list, (void *)p_node_info, cur_node );
            LOG_DEBUG( "fnh_find.p_find_node_handler:0x%x, sh_send_packet failed!! ", p_find_node_handler );
			return;
		}

        p_node_info->_try_times++;
        
        if( p_node_info->_try_times < dk_find_node_retry_times() )
        {
            ret_val = list_push( &p_find_node_handler->_unqueryed_node_info_list, (void *)p_node_info );
            if( ret_val != SUCCESS )
            {
                sd_assert( FALSE );
                p_find_node_handler->_node_destoryer( p_node_info );
                return;   
            }
        }   
        else
        {
            
#ifdef _LOGGER
            p_kad_node_info = (KAD_NODE_INFO *)p_node_info;
            if( p_find_node_handler->_dk_type == KAD_TYPE )
            {
                version = p_kad_node_info->_version;
            }

            LOG_DEBUG( "find_node_remove_node, p_find_node_handler:0x%x, try exceed max time p_node_info:0x%x, ip:%s, port:%u, version:%d, retry_time:%u", 
                p_find_node_handler, p_node_info, addr, p_node_info->_peer_addr._port, version, p_node_info->_try_times ); 
#endif	   
                p_find_node_handler->_node_destoryer( p_node_info );
        }

		find_num++;
	}
	
}

FIND_NODE_STATUS fnh_get_status( FIND_NODE_HANDLER *p_find_node_handler )
{
	LOG_DEBUG( "fnh_get_status.p_find_node_handler:0x%x", p_find_node_handler);
	return p_find_node_handler->_status;
}

_int32 find_node_remove_node_info( FIND_NODE_HANDLER *p_find_node_handler, _u32 net_ip, _u16 host_port )
{
    LIST_ITERATOR cur_node = LIST_BEGIN( p_find_node_handler->_unqueryed_node_info_list );
    LIST_ITERATOR next_node = NULL;

    NODE_INFO *p_node_info = NULL;
    
#ifdef _LOGGER
    char addr[24] = {0};
    KAD_NODE_INFO *p_kad_node_info = NULL;
    char test_buffer[1024];

    sd_inet_ntoa(net_ip, addr, 24);
	LOG_DEBUG( "find_node_remove_node, had response,p_find_node_handler:0x%x, ip:%s, port:%u, resp_node_num:%u", 
        p_find_node_handler, addr, host_port, p_find_node_handler->_resp_node_num );
#endif	

    while( cur_node != LIST_END( p_find_node_handler->_unqueryed_node_info_list ) )
    {
        p_node_info = ( NODE_INFO * )LIST_VALUE( cur_node );
        next_node = LIST_NEXT( cur_node );
        
        if( p_node_info->_peer_addr._ip == net_ip
            && p_node_info->_peer_addr._port == host_port )
        {
            
#ifdef _LOGGER
            if( p_find_node_handler->_dk_type == KAD_TYPE )
            {
                p_kad_node_info = (KAD_NODE_INFO *)p_node_info;
                sd_memset(test_buffer,0,1024);
                str2hex( (char *)p_kad_node_info->_node_id_ptr, KAD_ID_LEN, test_buffer, 1024);
                LOG_DEBUG( "find_node_remove_node success!!!, had response,p_find_node_handler:0x%x, ip:%s, port:%u, version:%u, id:%s", 
                    p_find_node_handler, addr, host_port, p_kad_node_info->_version, test_buffer );
            }
#endif	
            p_find_node_handler->_node_destoryer( p_node_info );
            list_erase( &p_find_node_handler->_unqueryed_node_info_list, cur_node );
            p_find_node_handler->_resp_node_num++;
        }
        cur_node = next_node;
    }
    return SUCCESS;
}

void find_node_clear_idle_ticks( FIND_NODE_HANDLER *p_find_node_handler )
{
    p_find_node_handler->_idle_ticks = 0;
}


//find_node_handler malloc/free wrap
_int32 init_find_node_handler_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_find_node_handler_slab == NULL )
	{              
		ret_val = mpool_create_slab( sizeof( FIND_NODE_HANDLER ), MIN_FIND_NODE_HANDLER, 0, &g_find_node_handler_slab );
		CHECK_VALUE( ret_val );
	}
	return ret_val;
}

_int32 uninit_find_node_handler_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_find_node_handler_slab != NULL )
	{              
		ret_val = mpool_destory_slab( g_find_node_handler_slab );
		CHECK_VALUE( ret_val );
		g_find_node_handler_slab = NULL;
	}
	return ret_val;
}

_int32 find_node_handler_malloc_wrap(FIND_NODE_HANDLER **pp_node)
{
	_int32 ret_val = mpool_get_slip( g_find_node_handler_slab, (void**)pp_node );
	CHECK_VALUE( ret_val );
	LOG_DEBUG( "find_node_handler_malloc_wrap:0x%x ", *pp_node );			
	return SUCCESS;
}

_int32 find_node_handler_free_wrap(FIND_NODE_HANDLER *p_node)
{
	_int32 ret_val = SUCCESS;
	sd_assert( p_node != NULL );
	if( p_node == NULL ) return SUCCESS;
	LOG_DEBUG( "find_node_handler_free_wrap:0x%x ", p_node );		
	ret_val = mpool_free_slip( g_find_node_handler_slab, (void*)p_node );
	CHECK_VALUE( ret_val );	
	
	return SUCCESS;
}


_int32 find_node_set_compare( void *E1, void *E2 )
{
	_u32 value1 = (_u32)E1;
	_u32 value2 = (_u32)E2;
	LOG_DEBUG( "find_node_set_compare:value1=%u,value2=%u",value1,value2 );	
	return value1 > value2 ? 1 : ( value1 < value2 ? -1 : 0 );  
}

#endif
