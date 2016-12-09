#include "utility/define.h"
#ifdef _DK_QUERY

#include "routing_table.h"
#include "ping_handler.h"
#include "find_node_handler.h"
#include "dk_setting.h"
#include "dk_socket_handler.h"
#include "dk_define.h"

#include "platform/sd_fs.h"
#include "utility/utility.h"

#include "utility/logid.h"
#define LOGID LOGID_DK_QUERY
#include "utility/logger.h"
#include "utility/mempool.h"

static ROUTING_TABLE *g_routing_table_ptr[DK_TYPE_COUNT] = { NULL, NULL };


_int32 dk_routing_table_create( _u32 dk_type )
{
	_int32 ret_val = SUCCESS;
	ROUTING_TABLE *p_routing_table = NULL;
	
	LOG_DEBUG( "dk_routing_table_create" );
	
	if( NULL != g_routing_table_ptr[ dk_type ]
		|| dk_type >= DK_TYPE_COUNT )
	{
		sd_assert( FALSE );
		return -1;
	}
	ret_val = sd_malloc( sizeof(ROUTING_TABLE), (void **)&p_routing_table );
	if( ret_val != SUCCESS )
	{
		goto ErrHandler;
	}
	
	g_routing_table_ptr[dk_type] = p_routing_table;


	//init _bucket_para
	p_routing_table->_bucket_para._can_split_max_distance = dk_can_split_max_distance();
	p_routing_table->_bucket_para._dk_type = dk_type;

	if( dk_type == DHT_TYPE )
	{
        p_routing_table->_bucket_para._k = dht_bucket_k();
		p_routing_table->_bucket_para._max_level = dht_bucket_max_level();
		p_routing_table->_bucket_para._min_level = dht_bucket_min_level();
	}
	else if( dk_type == KAD_TYPE )
	{
        p_routing_table->_bucket_para._k = kad_bucket_k();
		p_routing_table->_bucket_para._max_level = kad_bucket_max_level();
		p_routing_table->_bucket_para._min_level = kad_bucket_min_level();
	}
	else
	{
		sd_assert( FALSE );
		return -1;
	}

	ret_val = create_k_bucket_without_index( NULL
            , &p_routing_table->_bucket_para
            , &p_routing_table->_root_ptr );
	if( ret_val != SUCCESS )
	{
		goto ErrHandler1;
	}
	
	ret_val = k_distance_init( &p_routing_table->_root_ptr->_bucket_index, 1 );	
	if( ret_val != SUCCESS )
	{
		goto ErrHandler2;
	}
	
	ret_val = ping_handler_init( &p_routing_table->_ping_handler, dk_type );	
	if( ret_val != SUCCESS )
	{
		goto ErrHandler2;
	}	
	list_init( &p_routing_table->_find_node_list );
	p_routing_table->_local_node_id_ptr = NULL;
	p_routing_table->_time_ticks = 0;
	p_routing_table->_ping_root_nodes_times = 0;
	p_routing_table->_dk_type = dk_type;
	p_routing_table->_status = RT_IDLE;

    rt_handle_root_node( p_routing_table );
    
	rt_update( p_routing_table );
	return SUCCESS;


ErrHandler2:
	destrory_k_bucket( &p_routing_table->_root_ptr );
	
ErrHandler1:
	SAFE_DELETE( g_routing_table_ptr[dk_type] );
	
ErrHandler:
	return ret_val;
}

_int32 dk_routing_table_destory( _u32 dk_type )
{
	ROUTING_TABLE *p_routing_table = rt_ptr( dk_type );
	LIST_ITERATOR cur_node = NULL;
	FIND_NODE_HANDLER *p_find_node_handler = NULL;
	
	LOG_DEBUG( "dk_routing_table_destory, dk_type:%d", dk_type );
	
	if( p_routing_table == NULL ) return SUCCESS;

	rt_save_to_cfg( p_routing_table );
	
	k_distance_destory( &p_routing_table->_local_node_id_ptr ); 

	ping_handler_uninit( &p_routing_table->_ping_handler );
	
	cur_node = LIST_BEGIN( p_routing_table->_find_node_list );
	while( cur_node != LIST_END( p_routing_table->_find_node_list ) )
	{
		p_find_node_handler = (FIND_NODE_HANDLER *)LIST_VALUE( cur_node );
        fnh_uninit( p_find_node_handler );
        fnh_destory( p_find_node_handler );
		cur_node = LIST_NEXT( cur_node );
	}

	list_clear( &p_routing_table->_find_node_list );
	
	destrory_k_bucket( &p_routing_table->_root_ptr );
	
	SAFE_DELETE( g_routing_table_ptr[dk_type] );

	return SUCCESS;
}

ROUTING_TABLE *rt_ptr( _u32 dk_type )
{
	sd_assert( dk_type < DK_TYPE_COUNT );
	sd_assert( g_routing_table_ptr[ dk_type ] != NULL );

	return g_routing_table_ptr[ dk_type ];
}

void rt_update( ROUTING_TABLE *p_routing_table )
{
    _u32 rout_num = kb_get_node_num( p_routing_table->_root_ptr );
	
	p_routing_table->_time_ticks++;
	LOG_DEBUG( "rt_update, p_routing_table:0x%x, status:%d, time_ticks:%u, rout_num:%u", 
        p_routing_table, p_routing_table->_status, p_routing_table->_time_ticks, rout_num );
	if( p_routing_table->_status == RT_IDLE )
	{
        if( rout_num > dk_min_node_num()
            && rout_num < dk_avg_node_num() )
        {
            rt_find_myself( p_routing_table );
            p_routing_table->_status = RT_INIT;
        }
	}
    
    rt_handle_root_node( p_routing_table );
	rt_update_ping_handler( p_routing_table );
    if( rout_num < dk_avg_node_num() )
    {
        if( rout_num > dk_min_node_num() ) 
        {
            rt_add_find_node_list( p_routing_table );
        }
    }
    else
    {
        p_routing_table->_status = RT_RUNNING;
    }
    rt_update_find_node_list( p_routing_table );    
    
}

void rt_update_ping_handler( ROUTING_TABLE *p_routing_table )
{
	_int32 ret_val = SUCCESS;
	LIST ping_node_list;
	LIST_ITERATOR cur_node = NULL;
	K_NODE *p_k_node = NULL;
	KAD_NODE *p_kad_node = NULL;
	_u32 ping_tick_cycle = 0;
    
#ifdef _LOGGER
    char addr[24] = {0};
#endif	

	LOG_DEBUG( "rt_update_ping_handler, p_routing_table:0x%x, _ping_time_ticks:%d", 
        p_routing_table, p_routing_table->_time_ticks );
	
	ping_tick_cycle = dk_get_ping_tick_cycle();
	if( ping_tick_cycle == 0 )
	{
		sd_assert( FALSE );
		return;
	}
    
	if( p_routing_table->_time_ticks % ping_tick_cycle == 0 ) 
	{

		list_init( &ping_node_list );
		
		kb_get_old_node_list( p_routing_table->_root_ptr, dk_once_ping_num(), &ping_node_list );

		cur_node = LIST_BEGIN( ping_node_list );

		if( p_routing_table->_dk_type == DHT_TYPE )
		{
			while( cur_node != LIST_END( ping_node_list ) )
			{
				p_k_node = (K_NODE *)LIST_VALUE( cur_node );
#ifdef _LOGGER
                sd_inet_ntoa( p_k_node->_peer_addr._ip, addr, 24);
                LOG_DEBUG( "rt_update_ping_handler, ping_k_node: p_k_node:0x%x, ip:%s, port:%u", 
                    p_k_node, addr, p_k_node->_peer_addr._port );
#endif	
				ret_val = ph_ping_node( &p_routing_table->_ping_handler, 
					p_k_node->_peer_addr._ip, p_k_node->_peer_addr._port, 0);
				if( ret_val != SUCCESS ) return;
				cur_node = LIST_NEXT( cur_node );
			}	
		}
		else if( p_routing_table->_dk_type == KAD_TYPE )
		{
			while( cur_node != LIST_END( ping_node_list ) )
			{
				p_kad_node = (KAD_NODE *)LIST_VALUE( cur_node );
				p_k_node = &p_kad_node->_k_node;
#ifdef _LOGGER
                sd_inet_ntoa( p_k_node->_peer_addr._ip, addr, 24);
                LOG_DEBUG( "rt_update_ping_handler, ping_k_node: p_k_node:0x%x, ip:%s, port:%u, version:%d", 
                    p_k_node, addr, p_k_node->_peer_addr._port, p_kad_node->_version );
#endif	                
				ret_val = ph_ping_node( &p_routing_table->_ping_handler, 
					p_k_node->_peer_addr._ip, p_k_node->_peer_addr._port, p_kad_node->_version );
				if( ret_val != SUCCESS ) return;
				cur_node = LIST_NEXT( cur_node );
			}	
		}
		else
		{
			sd_assert( FALSE );
		}
		
		list_clear( &ping_node_list );
	}

	ph_update( &p_routing_table->_ping_handler );
	
}

void rt_handle_root_node( ROUTING_TABLE *p_routing_table )
{
	LOG_DEBUG( "rt_handle_root_node, p_routing_table:0x%x, status:%d", 
        p_routing_table, p_routing_table->_status );

    if( p_routing_table->_time_ticks % dk_root_node_interval() != 0 ) return;

    if( p_routing_table->_status != RT_IDLE )
    {
        rt_save_to_cfg( p_routing_table );
        return;
    }

    if( p_routing_table->_ping_root_nodes_times < dk_ping_root_node_max_times() )
    {
     	rt_load_from_cfg( p_routing_table );

    	if( p_routing_table->_dk_type == DHT_TYPE )
    	{
    		ph_ping_dht_boot_host( &p_routing_table->_ping_handler );
    	}   
        p_routing_table->_ping_root_nodes_times++;
    }
}

_int32 rt_find_myself( ROUTING_TABLE *p_routing_table )
{
	_int32 ret_val = SUCCESS;
	FIND_NODE_HANDLER *p_find_node_handler = NULL;
	_u8 *p_target = NULL;
	_u32 target_len = 0;
	
	LOG_DEBUG( "rt_find_myself, p_routing_table:0x%x", p_routing_table );
	
	ret_val = fnh_create( &p_find_node_handler );
	CHECK_VALUE( ret_val );

	p_target = k_distance_get_bit_buffer( p_routing_table->_local_node_id_ptr );
	target_len = k_distance_buffer_len( p_routing_table->_local_node_id_ptr );
	ret_val = fnh_init( p_routing_table->_dk_type, p_find_node_handler, p_target, target_len, KAD_FIND_NODE );
	if( ret_val != SUCCESS )
	{
		fnh_destory( p_find_node_handler );
	}
	
	list_push( &p_routing_table->_find_node_list, p_find_node_handler );
	return SUCCESS;
}

void rt_update_find_node_list( ROUTING_TABLE *p_routing_table )
{
	FIND_NODE_HANDLER *p_find_node_handler = NULL;
	LIST_ITERATOR cur_node = NULL;
	LIST_ITERATOR next_node = NULL;
	FIND_NODE_STATUS status;

	LOG_DEBUG( "rt_update_find_node_list, p_routing_table:0x%x", p_routing_table );
	
	cur_node = LIST_BEGIN( p_routing_table->_find_node_list );
	while( cur_node != LIST_END( p_routing_table->_find_node_list ) )
	{
		p_find_node_handler = (FIND_NODE_HANDLER *)LIST_VALUE( cur_node );
		fnh_update( p_find_node_handler );
		status = fnh_get_status( p_find_node_handler );
		if( status == FIND_NODE_FINISHED )
		{
            fnh_uninit( p_find_node_handler );
			fnh_destory( p_find_node_handler );
			next_node = LIST_NEXT( cur_node );
			list_erase( &p_routing_table->_find_node_list, cur_node );
			cur_node = next_node;
			continue;
		}
		cur_node = LIST_NEXT( cur_node );
	}
	return;

}

_int32 rt_add_find_node_list( ROUTING_TABLE *p_routing_table )
{
	_int32 ret_val = SUCCESS;
	LIST empty_bucket_list;
	_u32 find_empty_bucket_cycle = 0;
	_u32 find_empty_bucket_num = 0;
	LIST_ITERATOR cur_node = NULL;
	FIND_NODE_HANDLER *p_find_node_handler = NULL;
	K_BUCKET *p_bucket = NULL;
	find_empty_bucket_cycle = dk_find_empty_bucket_cycle();
    
#ifdef _PACKET_DEBUG
    char test_buffer[1024];
    _u8 *p_target_id = NULL;
#endif
	
	LOG_DEBUG( "rt_add_find_node_list begin: routing_table:0x%x, find_node_list_size:%u, _time_ticks:%d",
        p_routing_table, list_size( &p_routing_table->_find_node_list ), p_routing_table->_time_ticks );
	
	if( find_empty_bucket_cycle == 0 )
	{
		sd_assert( FALSE );
		return -1;
	}
	
	if( p_routing_table->_time_ticks % find_empty_bucket_cycle != find_empty_bucket_cycle/2 
		|| list_size( &p_routing_table->_find_node_list ) >= dk_empty_bucket_find_node_max_num() )
		return SUCCESS;
		
	list_init( &empty_bucket_list );
	find_empty_bucket_num = dk_empty_bucket_find_node_max_num() - list_size( &p_routing_table->_find_node_list );
	kb_get_bucket_list( p_routing_table->_root_ptr, &find_empty_bucket_num, TRUE, &empty_bucket_list );		

#ifdef _PACKET_DEBUG
    p_target_id = k_distance_get_bit_buffer( p_routing_table->_local_node_id_ptr );
    sd_memset(test_buffer,0,1024);
    str2hex( (char *)p_target_id, KAD_ID_LEN, test_buffer, 1024);
    
    LOG_DEBUG( "rt_add_find_node_list, local _id:%s, find_empty_bucket_num:%d, empty_bucket_list_size:%d", 
        test_buffer, find_empty_bucket_num, list_size(&empty_bucket_list) );
#endif

	cur_node = LIST_BEGIN( empty_bucket_list );

	while( cur_node != LIST_END( empty_bucket_list ) )
	{
		ret_val = fnh_create( &p_find_node_handler );
		CHECK_VALUE( ret_val );
		
		ret_val = fnh_init( p_routing_table->_dk_type, p_find_node_handler, NULL, 0, KAD_FIND_NODE );
		if( ret_val != SUCCESS )
		{
			fnh_destory( p_find_node_handler );
		}

		p_bucket = LIST_VALUE( cur_node );
		ret_val = kb_get_bucket_find_node_id( p_bucket, &p_find_node_handler->_target );

        if( ret_val != SUCCESS
            || rt_is_target_exist(p_routing_table, &p_find_node_handler->_target) )
        {
			fnh_uninit( p_find_node_handler );
			fnh_destory( p_find_node_handler );
            cur_node = LIST_NEXT( cur_node );
            continue;
        }
		list_push( &p_routing_table->_find_node_list, p_find_node_handler );
        
#ifdef _PACKET_DEBUG
        p_target_id = k_distance_get_bit_buffer( &p_find_node_handler->_target );
        sd_memset(test_buffer,0,1024);
        str2hex( (char *)p_target_id, KAD_ID_LEN, test_buffer, 1024);
        
        LOG_DEBUG( "rt_add_find_node_list, target_id:%s, p_find_node_handler:0x%x, _find_node_list size:%d", 
            test_buffer, p_find_node_handler, list_size(&p_routing_table->_find_node_list) );
#endif

		cur_node = LIST_NEXT( cur_node );
	}
    list_clear( &empty_bucket_list );
	LOG_DEBUG( "rt_add_find_node_list end: routing_table:0x%x, find_node_list_size:%u",
        p_routing_table, list_size( &p_routing_table->_find_node_list ) );
		
	return SUCCESS;	
}

BOOL rt_is_target_exist( ROUTING_TABLE *p_routing_table, K_NODE_ID *p_target )
{
	FIND_NODE_HANDLER *p_find_node_handler = NULL;
	LIST_ITERATOR cur_node = LIST_BEGIN( p_routing_table->_find_node_list );
    
    _int32 ret_val = SUCCESS, compare_ret = 0;

    while( cur_node != LIST_END( p_routing_table->_find_node_list ) )
    {
        p_find_node_handler = (FIND_NODE_HANDLER *)LIST_VALUE( cur_node );
        
        ret_val = k_distance_compare( &p_find_node_handler->_target, p_target, &compare_ret );
        if( ret_val == SUCCESS && compare_ret == 0 ) return TRUE;
        cur_node = LIST_NEXT( cur_node );
    }
    return FALSE;
}

_int32 rt_add_rout_node( ROUTING_TABLE *p_routing_table, K_NODE *p_node )
{
	_int32 ret_val = SUCCESS;
#ifdef _LOGGER
    char addr[24] = {0};

    sd_inet_ntoa( p_node->_peer_addr._ip, addr, 24);
	LOG_DEBUG( "rt_add_rout_node, p_routing_table:0x%x, p_k_node:0x%x, ip:%s, port:%u", 
        p_routing_table, p_node, addr, p_node->_peer_addr._port );
#endif	

	ret_val = kb_add_node( p_routing_table->_root_ptr, p_node );
    if( ret_val != SUCCESS )
    {
        LOG_DEBUG( "rt_add_rout_node failed!!!!!!, p_node:0x%x, err:%d", p_node, ret_val );
        return ret_val;
    }

	LOG_DEBUG( "rt_add_rout_node end, p_routing_table:0x%x, node_num:%u", 
        p_routing_table, kb_get_node_num(p_routing_table->_root_ptr) );    
    return SUCCESS;
}


_int32 rt_get_nearest_nodes( ROUTING_TABLE *p_routing_table, const K_NODE_ID *p_k_node_id, LIST *p_node_list, _u32 *p_node_num )	
{
#ifdef _LOGGER
    //k_bucket_print_node_info(p_routing_table->_root_ptr);
#endif
	return kb_get_nearst_node( p_routing_table->_root_ptr, p_k_node_id, p_node_list, p_node_num );
}

K_NODE_ID *rt_get_local_id( ROUTING_TABLE *p_routing_table )
{
	LOG_DEBUG( "rt_get_local_id" );
	return p_routing_table->_local_node_id_ptr;
}


_int32 rt_load_from_cfg( ROUTING_TABLE *p_routing_table )
{
	_int32 ret_val = SUCCESS;
	_u32 file_id = INVALID_FILE_ID;
	char *p_file_path = NULL;
	_u8 *p_node_id_buffer = NULL;
	_u32 read_size = 0;
	_u32 ip = 0;
	_u16 port = 0;
	_u16 local_port = 0;
	_u32 total_node_num = 0;
	_u32 node_index = 0;
	_u32 id_len = 0;
	_u8 version = 0;
    
#ifdef _LOGGER
    char addr[24] = {0};
#endif	

	LOG_DEBUG( "rt_load_from_cfg" );
	
	local_port = sh_get_udp_port(sh_ptr(p_routing_table->_dk_type));

	if( p_routing_table->_dk_type == DHT_TYPE )
	{
		p_file_path = dht_cfg_path();
		id_len = DHT_ID_LEN;
	}
	else if( p_routing_table->_dk_type == KAD_TYPE )
	{
		p_file_path = kad_cfg_path();
		id_len = KAD_ID_LEN;
	}
	else
	{
		sd_assert( FALSE );
		return -1;
	}

	ret_val = sd_malloc( id_len, (void **)&p_node_id_buffer );
	CHECK_VALUE( ret_val ); 

    LOG_DEBUG("dhtkad cfg path : %s", p_file_path);

	ret_val = sd_open_ex( p_file_path, O_FS_CREATE, &file_id );
	if( ret_val == SUCCESS )
	{
		ret_val = sd_read( file_id, (char *)p_node_id_buffer, id_len, &read_size );
	}

	if( read_size == 0 || ret_val != SUCCESS )
	{
		ret_val = k_node_generate_id( p_node_id_buffer, id_len );
		LOG_ERROR("k_node_generate_id ret = %d", ret_val);
		if( ret_val != SUCCESS ) goto ErrHandler1;
	}

	ret_val = k_distance_create_with_char_buffer( p_node_id_buffer, id_len, &p_routing_table->_local_node_id_ptr );
	LOG_ERROR("k_distance_create_with_char_buffer ret = %d", ret_val);
	if( ret_val != SUCCESS ) goto ErrHandler1;
 
    SAFE_DELETE( p_node_id_buffer );
    
	ret_val = sd_read( file_id, (char *)&total_node_num, sizeof( total_node_num ), &read_size );
	LOG_ERROR("sd_read total_node_num ret = %d", ret_val);
	if( ret_val != SUCCESS || read_size == 0 )
	{
		goto ErrHandler2;
	}		
	LOG_DEBUG( "rt_load_from_cfg, node_num:%u", total_node_num );

	for( ; node_index < total_node_num; node_index++ )
	{
		version = 0;
		ret_val = sd_read( file_id, (char *)&ip, sizeof( ip ), &read_size );
		if( ret_val != SUCCESS || read_size == 0 )
		{
			goto ErrHandler2;
		}	
		ret_val = sd_read( file_id, (char *)&port, sizeof( port), &read_size );
		if( ret_val != SUCCESS || read_size == 0 )
		{
			goto ErrHandler2;
		}
		if( p_routing_table->_dk_type == KAD_TYPE )
		{
			ret_val = sd_read( file_id, (char *)&version, sizeof( version), &read_size );
			if( ret_val != SUCCESS || read_size == 0 )
			{
				goto ErrHandler2;
			}
		}
        
#ifdef _LOGGER
        sd_inet_ntoa( ip, addr, 24);
    	LOG_DEBUG( "rt_load_from_cfg, p_routing_table:0x%x, ip:%s, port:%u, version:%u", 
            p_routing_table, addr, port, version );
#endif			

		ret_val = ph_ping_node( &p_routing_table->_ping_handler, ip, port, version );
		if( ret_val != SUCCESS )
		{
			goto ErrHandler2;
		}	

	}
	
	sd_close_ex( file_id );
    
	return SUCCESS;
	
ErrHandler2:
	//k_distance_destory( &p_routing_table->_local_node_id_ptr );	
	LOG_ERROR("rt_load_from_cfg ErrHandler2");
	sd_close_ex( file_id );
	return ret_val;
	
ErrHandler1:
	sd_close_ex( file_id );
	LOG_ERROR("rt_load_from_cfg ErrHandler1");
	
	SAFE_DELETE( p_node_id_buffer );
	
	return ret_val;
		
}

_int32 rt_save_to_cfg( ROUTING_TABLE *p_routing_table )
{
	_int32 ret_val = SUCCESS;
	_u32 file_id = INVALID_FILE_ID;
	char *p_file_path = NULL;
	char  buffer[1024];
	_u32 buffer_pos = 0, buffer_len = 1024;
	_u8 *p_local_peer_id = NULL;
	LIST node_list;
    _u32 save_max_num = 0;
	_u32 node_num = 0;
	LIST_ITERATOR cur_node = NULL;
	_u32 id_len = 0;
	K_NODE *p_k_node = NULL;
	KAD_NODE *p_kad_node = NULL;
    _u32 write_len = 0;
#ifdef _LOGGER
    char addr[24] = {0};
#endif	

	LOG_DEBUG( "rt_save_to_cfg begin" );
	
	if( p_routing_table->_dk_type == DHT_TYPE )
	{
		p_file_path = dht_cfg_path();
		id_len = DHT_ID_LEN;
		node_num = dht_peer_save_num();
	}
	else if( p_routing_table->_dk_type == KAD_TYPE )
	{
		p_file_path = kad_cfg_path();
		id_len = KAD_ID_LEN;
		node_num = kad_peer_save_num();
	}
	else
	{
		sd_assert( FALSE );
		return -1;
	}
	
	ret_val = sd_open_ex( p_file_path, O_FS_CREATE, &file_id );
 	if( ret_val != SUCCESS )
 	{
 		goto ErrHandler;
 	}

	p_local_peer_id = k_distance_get_bit_buffer( p_routing_table->_local_node_id_ptr );
	
	ret_val = sd_write_save_to_buffer( file_id, buffer, buffer_len, &buffer_pos, (char *)p_local_peer_id, id_len );
	if( ret_val != SUCCESS ) goto ErrHandler1;
    
    save_max_num = node_num;

	list_init( &node_list );
	ret_val = kb_get_nearst_node( p_routing_table->_root_ptr, p_routing_table->_local_node_id_ptr,
		&node_list, &node_num );
	if( ret_val != SUCCESS ) goto ErrHandler1;

    sd_assert( save_max_num >= node_num );
    node_num = save_max_num - node_num;
    sd_assert( node_num == list_size( &node_list ) );
	LOG_DEBUG( "rt_save_to_cfg, node_num:%u", node_num );

	ret_val = sd_write_save_to_buffer( file_id, buffer, buffer_len, &buffer_pos, (char *)&node_num, sizeof(node_num) );
	if( ret_val != SUCCESS ) goto ErrHandler1;

	cur_node = LIST_BEGIN( node_list );
	while( cur_node != LIST_END( node_list ))
	{
		p_k_node = ( K_NODE *)LIST_VALUE( cur_node );

		ret_val = sd_write_save_to_buffer( file_id, buffer, buffer_len, &buffer_pos, (char *)&p_k_node->_peer_addr._ip, sizeof(_u32) );
		if( ret_val != SUCCESS ) goto ErrHandler1;
	
		ret_val = sd_write_save_to_buffer( file_id, buffer, buffer_len, &buffer_pos, (char *)&p_k_node->_peer_addr._port, sizeof(_u16) );
		if( ret_val != SUCCESS ) goto ErrHandler1;
		
		if( p_routing_table->_dk_type == KAD_TYPE )
		{
			p_kad_node = ( KAD_NODE *)p_k_node;
			ret_val = sd_write_save_to_buffer( file_id, buffer, buffer_len, &buffer_pos, (char *)&p_kad_node->_version, sizeof(p_kad_node->_version) );
			if( ret_val != SUCCESS ) goto ErrHandler1;
		}		
        
#ifdef _LOGGER
        sd_inet_ntoa( p_k_node->_peer_addr._ip, addr, 24);
    	LOG_DEBUG( "rt_save_to_cfg, p_routing_table:0x%x, ip:%s, port:%u", 
            p_routing_table, addr, p_k_node->_peer_addr._port );
#endif				

		cur_node = LIST_NEXT( cur_node );
	}
	list_clear( &node_list );

    if( buffer_pos != 0 )
	{
		ret_val = sd_write( file_id,buffer, buffer_pos, &write_len );
		if( ret_val != SUCCESS ) goto ErrHandler1;
		sd_assert( buffer_pos == write_len );
	}
    
	LOG_DEBUG( "rt_save_to_cfg end" );
ErrHandler1:
	sd_close_ex( file_id );
ErrHandler:
	return ret_val;	
}

_int32 rt_ping_node( _u32 dk_type, _u32 ip, _u16 port, _u8 version, BOOL is_force_ping )
{
 	ROUTING_TABLE *p_routing_table = g_routing_table_ptr[ dk_type ];
#ifdef _LOGGER
    char addr[24] = {0};
#endif	    

    if( p_routing_table == NULL ) return ROUTING_TABLE_NULL;
    
#ifdef _LOGGER
    sd_inet_ntoa( ip, addr, 24);
    LOG_DEBUG( "rt_ping_node, is_force_ping:%d, _status:%d, ip:%s, port:%u", 
        is_force_ping, p_routing_table->_status, addr, port );
#endif	  

	if( p_routing_table == NULL )
	{
		CHECK_VALUE( -1 );
	}	

    if( !is_force_ping && p_routing_table->_status == RT_RUNNING ) return SUCCESS;
    return ph_ping_node( &p_routing_table->_ping_handler, ip, port, version );
}

_int32 rt_distance_calc( _u32 dk_type, const K_NODE_ID *p_k_node_id, K_DISTANCE *p_k_distance )
{
	ROUTING_TABLE *p_routing_table = rt_ptr( dk_type );
	K_NODE_ID *p_local_node_id = NULL;
	
	//LOG_DEBUG( "rt_distance_calc" );
	
	if( p_routing_table == NULL )
	{
		CHECK_VALUE( -1 );
	}	
	
	p_local_node_id = p_routing_table->_local_node_id_ptr;
	
	if( p_local_node_id == NULL ) 
	{
		CHECK_VALUE( -1 );
	}
	
	return k_distance_xor( p_local_node_id, p_k_node_id, p_k_distance );
}

BOOL rt_is_ready( _u32 dk_type )
{
    BOOL ret_bool = FALSE;
	ROUTING_TABLE *p_routing_table = rt_ptr( dk_type );
	
	if( p_routing_table == NULL )
	{
		sd_assert( FALSE );
        return FALSE;
	}	
	if( p_routing_table->_status == RT_RUNNING )
	{
        ret_bool = TRUE;
	}
    LOG_DEBUG( "rt_is_ready, p_routing_table:0x%x, is_ready:%d", p_routing_table, ret_bool );
    
    return ret_bool;
}

#endif

