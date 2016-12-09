#include "utility/define.h"
#ifdef _DK_QUERY

#include "dht_recv_packet_handler.h"
#include "dk_socket_handler.h"
#include "routing_table.h"
#include "utility/utility.h"
#include "dk_setting.h"
#include "utility/bytebuffer.h"
#include "utility/mempool.h"

#include "utility/logid.h"
#define LOGID LOGID_DK_QUERY
#include "utility/logger.h"

static _u32 g_dht_send_packet_id = 10000000;
#define MAX_NODES_INFO_LEN 512

_int32 dht_recv_handle_recv_packet( _u32 ip, _u16 port, char *p_buffer, _u32 data_len )
{
	_int32 ret_val = SUCCESS;
	BC_DICT *p_dict = NULL;
	BC_PARSER *p_bc_parser = NULL;
	
	BC_OBJ *p_bc_obj = NULL;
	BC_OBJ *p_bc_value = NULL;
	const char *p_key = "y";
	
	BC_STR *p_bc_str_value = NULL;
	
#ifdef _LOGGER
	char addr[24] = {0};
	ret_val = sd_inet_ntoa(ip, addr, 24);
	sd_assert( ret_val == SUCCESS ); 

 	LOG_DEBUG( "dht_recv_handle_recv_packet: host_ip = %s, port = %u, data_len:%u", addr, port, data_len );

    {
        char test_buffer[1024];
        sd_memset(test_buffer,0,1024);
        str2hex( p_buffer, data_len, test_buffer, 1024);
        LOG_DEBUG( "dht_recv_handle_recv_packet:*buffer[%u]=%s" ,(_u32)data_len, test_buffer);
    }

#endif

#ifdef _PACKET_DEBUG
	{
		char test_buffer[1024];
		sd_memset(test_buffer,0,1024);
		sd_memcpy(test_buffer,p_buffer,1024);
		LOG_DEBUG( "dht_recv_handle_recv_packet:*buffer[%u]=%s",data_len, test_buffer);
	}
#endif /* _DEBUG */

	
	ret_val = bc_parser_create( p_buffer, data_len, data_len, &p_bc_parser);
	if( ret_val != SUCCESS )
	{
		return ret_val;
	}

	ret_val = bc_parser_str( p_bc_parser, &p_bc_obj );
	if( ret_val != SUCCESS )
	{
        //sd_assert( FALSE );
		LOG_ERROR("dht_recv_handle_recv_packet, bc_dict_from_str failed, errcode = %d.", ret_val );
		bc_parser_destory(p_bc_parser);
		return ret_val;
	}
	
	//sd_assert( bc_get_type( p_bc_obj ) == DICT_TYPE );
	p_dict = (BC_DICT* )p_bc_obj;
	
	bc_parser_destory(p_bc_parser);

	if( bc_get_type( p_bc_obj ) != DICT_TYPE )
	{
		p_bc_obj->p_uninit_call_back(p_bc_obj);
		return -1;
	}


	ret_val = bc_dict_get_value( p_dict, p_key, (BC_OBJ **)&p_bc_value );
	
	if( ret_val != SUCCESS )
	{
		LOG_ERROR("dht_recv_handle_recv_packet, bc_dict_get_value failed, errcode = %d.", ret_val );
		bc_dict_uninit((BC_OBJ*)p_dict);
		return ret_val;
	}
	sd_assert( bc_get_type( p_bc_value ) == STR_TYPE );
	if( bc_get_type( p_bc_value ) != STR_TYPE )
	{
		LOG_ERROR("dht_recv_handle_recv_packet, bc_get_type failed, errcode = -1." );
		bc_dict_uninit((BC_OBJ*)p_dict);
		return -1;	
	}
	p_bc_str_value = (BC_STR *)p_bc_value;

	if( sd_strcmp( p_bc_str_value->_str_ptr, "q" ) == 0 )
	{
		ret_val = dht_on_query( ip, port, p_dict );
	}
	else if( sd_strcmp( p_bc_str_value->_str_ptr, "r" ) == 0 )
	{
		ret_val = dht_on_response( ip, port, p_dict );
	}	
	else if( sd_strcmp( p_bc_str_value->_str_ptr, "e" ) == 0 )
	{
		ret_val = dht_on_err( ip, port, p_dict );
	}	
	else
	{
		sd_assert( FALSE );
	}
	if( ret_val != SUCCESS )
	{
		LOG_ERROR("dht_recv_handle_recv_packet, process failed, errcode = %d.", ret_val );
	}
	bc_dict_uninit((BC_OBJ*)p_dict);
	return SUCCESS;	
}

_int32 dht_on_query( _u32 ip, _u16 port, BC_DICT *p_dict_data )
{
	BC_OBJ *p_bc_value = NULL;
	BC_STR *p_bc_str_value = NULL;
	char *p_key = "q";
	_int32 ret_val = SUCCESS;
	BC_DICT *p_a_dict = NULL;
	BC_STR *p_id_str = NULL;
	K_NODE *p_node = NULL;	
	
 	LOG_DEBUG( "dht_on_query" );

	ret_val = bc_dict_get_value( p_dict_data, p_key, (BC_OBJ **)&p_bc_value );
	CHECK_VALUE( ret_val );

	if( bc_get_type( p_bc_value ) != STR_TYPE )
	{
		CHECK_VALUE( -1 );
	}

	p_bc_str_value = ( BC_STR * )p_bc_value;
	if( sd_strcmp( p_bc_str_value->_str_ptr, "ping" ) == 0
		|| sd_strcmp( p_bc_str_value->_str_ptr, "announce_peer" ) == 0 )
	{
		dht_on_query_ping_or_announce( ip, port, p_dict_data );
	}
	else if( sd_strcmp( p_bc_str_value->_str_ptr, "find_node" ) == 0 )
	{
		dht_on_query_find_node( ip, port, p_dict_data, FALSE );
	}
	else if( sd_strcmp( p_bc_str_value->_str_ptr, "get_peers" ) == 0 )
	{
		dht_on_query_find_node( ip, port, p_dict_data, TRUE );
	}
	else
	{
		//FIXIT: bt protocol is too old, and some new fields are not support.
		sd_assert( FALSE );
		return SUCCESS;
	}
	
	ret_val = bc_dict_get_value( p_dict_data, "a", (BC_OBJ **)&p_a_dict );
	CHECK_VALUE( ret_val );
	
	ret_val = bc_dict_get_value( p_a_dict, "id", (BC_OBJ **)&p_id_str );
	CHECK_VALUE( ret_val );

	ret_val = k_node_create( (_u8 *)p_id_str->_str_ptr, p_id_str->_str_len, ip, port, &p_node );
	CHECK_VALUE( ret_val );
	
	ret_val = rt_add_rout_node( rt_ptr(DHT_TYPE), p_node );
	if( ret_val != SUCCESS )
	{
		k_node_destory( p_node );
	}	
	
	return SUCCESS;	
}


_int32 dht_on_query_ping_or_announce( _u32 ip, _u16 port, BC_DICT *p_dict_data )
{
	_int32 ret_val = SUCCESS;
	char *p_buffer = NULL;
	_u32 buffer_len = 0;
	BC_STR *p_t = NULL;
	
 	LOG_DEBUG( "dht_on_query_ping_or_announce" );
	
	ret_val = bc_dict_get_value( p_dict_data, "t", (BC_OBJ **)&p_t );
	CHECK_VALUE( ret_val );

	ret_val = dht_build_ping_or_announce_resp_cmd( p_t->_str_ptr, p_t->_str_len,
		&p_buffer, &buffer_len );
	if( ret_val != SUCCESS )
	{
		sd_assert( FALSE );
		return SUCCESS;
	}
	
	ret_val = sh_send_packet( sh_ptr(DHT_TYPE), ip, port, p_buffer, buffer_len, NULL, 0 );
	if( ret_val != SUCCESS )
	{
		//sd_assert( FALSE );
		SAFE_DELETE( p_buffer );
	}

	return SUCCESS;
}


_int32 dht_on_query_find_node( _u32 ip, _u16 port, BC_DICT *p_dict_data, BOOL is_get_peers )
{
	_int32 ret_val = SUCCESS;
	BC_DICT *p_a = NULL;
	BC_STR *p_t = NULL;
	BC_STR *p_target = NULL;
	K_NODE_ID target_id;
	LIST near_node_list;
	_u32 node_num = 0;
	LIST_ITERATOR cur_node = NULL;
	K_NODE *p_k_node = NULL;
	_u8 nodes_info[ MAX_NODES_INFO_LEN ];
	_u8 *p_nodes_pos = nodes_info;
	_u32 nodes_info_len = 0;
	char *p_buffer = NULL;
	_u32 buffer_len = MAX_NODES_INFO_LEN;	
	_u8 *p_node_id = NULL;
	_u32 node_id_len = 0;
    _u32 h_ip = 0;
	
 	LOG_DEBUG( "dht_on_query_find_node" );
	
	ret_val = bc_dict_get_value( p_dict_data, "a", (BC_OBJ **)&p_a );
	CHECK_VALUE( ret_val );
    if( p_a == NULL )
    {
        sd_assert( FALSE );
        return SUCCESS;
    }

	ret_val = bc_dict_get_value( p_dict_data, "t", (BC_OBJ **)&p_t );
	CHECK_VALUE( ret_val );	
    if( p_t == NULL )
    {
        sd_assert( FALSE );
        return SUCCESS;
    }

    if( is_get_peers )
    {
        ret_val = bc_dict_get_value( p_a, "info_hash", (BC_OBJ **)&p_target );
    }
    else
    {
        ret_val = bc_dict_get_value( p_a, "target", (BC_OBJ **)&p_target );
    }
	CHECK_VALUE( ret_val );	
    if( p_target == NULL )
    {
        sd_assert( FALSE );
        return SUCCESS;
    }

	ret_val = k_distance_init_with_char_buffer( &target_id, (_u8 *)p_target->_str_ptr, p_target->_str_len );
	CHECK_VALUE( ret_val );	

	list_init( &near_node_list );

	node_num = dht_get_nearest_node_max_num();

	ret_val = rt_get_nearest_nodes( rt_ptr(DHT_TYPE), &target_id, &near_node_list, &node_num  );
	k_distance_uninit( &target_id );
	if( ret_val != SUCCESS )
	{
		sd_assert( FALSE );
		return SUCCESS;
	}

	sd_memset( nodes_info, 0, MAX_NODES_INFO_LEN );
	cur_node = LIST_BEGIN( near_node_list );
	while( cur_node != LIST_END( near_node_list ) )
	{
		p_k_node = (K_NODE *)LIST_VALUE( cur_node );
		
		p_node_id = k_distance_get_bit_buffer( &p_k_node->_node_id );
		node_id_len = k_distance_buffer_len( &p_k_node->_node_id );
		
		ret_val = sd_set_bytes( (char **)&p_nodes_pos, (_int32 *)&buffer_len, (char *)p_node_id, node_id_len );
		CHECK_VALUE( ret_val );
        
		h_ip = sd_ntohl( p_k_node->_peer_addr._ip );
        
		ret_val = sd_set_int32_to_bg( (char **)&p_nodes_pos, (_int32 *)&buffer_len, h_ip );
		CHECK_VALUE( ret_val ); 
		
		ret_val = sd_set_int16_to_bg( (char **)&p_nodes_pos, (_int32 *)&buffer_len, p_k_node->_peer_addr._port );
		CHECK_VALUE( ret_val ); 
		
		cur_node = LIST_END( near_node_list );
	}
    list_clear( &near_node_list );
	if( buffer_len > MAX_NODES_INFO_LEN )
	{
		sd_assert( FALSE );
		return -1;
	}

	if( is_get_peers )
	{
		ret_val = dht_build_get_peers_resp_cmd( p_t->_str_ptr, p_t->_str_len,
		"nodes", (char *)nodes_info, nodes_info_len, &p_buffer, &buffer_len );
		if( ret_val != SUCCESS )
		{
			sd_assert( FALSE );
			return SUCCESS;
		}	
	}
	else
	{
		ret_val = dht_build_find_node_resp_cmd( p_t->_str_ptr, p_t->_str_len,
		(char *)nodes_info, nodes_info_len, &p_buffer, &buffer_len );
		if( ret_val != SUCCESS )
		{
			sd_assert( FALSE );
			return SUCCESS;
		}	
	}

	ret_val = sh_send_packet( sh_ptr(DHT_TYPE), ip, port, p_buffer, buffer_len, NULL, 0 );
	if( ret_val != SUCCESS )
	{
		SAFE_DELETE( p_buffer );
	}

	return SUCCESS;
}

_int32 dht_on_response( _u32 ip, _u16 port, BC_DICT *p_dict_data )
{
	_int32 ret_val = SUCCESS;
    BC_OBJ *p_bc_obj = NULL;
	BC_STR *p_bc_t_str = NULL;
	BC_INT *p_bc_t_int = NULL;
	DK_PROTOCAL_HANDLER *p_protocal_handler = NULL;
	BC_DICT *p_r_dict = NULL;
	_u64 key = 0;
	BC_STR *p_id_str = NULL;
	K_NODE *p_node = NULL;
	
    
#ifdef _DEBUG
    char addr[24] = {0};
    char test_buffer[1024];
#endif

 	LOG_DEBUG( "dht_on_response" );

	ret_val = bc_dict_get_value( p_dict_data, "t", &p_bc_obj );
    if( ret_val != SUCCESS ) return ret_val;

    if( p_bc_obj->_bc_type == STR_TYPE )
    {
        p_bc_t_str = (BC_STR *)p_bc_obj;
        ret_val = sd_str_to_u64( p_bc_t_str->_str_ptr, p_bc_t_str->_str_len, &key );
        if( ret_val != SUCCESS ) return ret_val;
    }
    else if( p_bc_obj->_bc_type == INT_TYPE )
    {
        p_bc_t_int = (BC_INT *)p_bc_obj;
        key = p_bc_t_int->_value;
    }	

	ret_val = sh_get_resp_handler( sh_ptr(DHT_TYPE), (_u32)key, &p_protocal_handler );
	if( ret_val != SUCCESS ) return ret_val;
	
	ret_val = bc_dict_get_value( p_dict_data, "r", (BC_OBJ **)&p_r_dict );
    if( ret_val != SUCCESS ) return ret_val;
	
	ret_val = bc_dict_get_value( p_r_dict, "id", (BC_OBJ **)&p_id_str );
    if( ret_val != SUCCESS ) return ret_val;
    
#ifdef _DEBUG
    sd_memset(test_buffer,0,1024);
    str2hex( (char *)p_id_str->_str_ptr, p_id_str->_str_len, test_buffer, 1024);
    sd_inet_ntoa( ip, addr, 24);
    LOG_DEBUG( "dht_on_response, ip:%s, port:%d, id:%s", addr, port, test_buffer );
#endif

	ret_val = k_node_create( (_u8 *)p_id_str->_str_ptr, p_id_str->_str_len, ip, port, &p_node );
	CHECK_VALUE( ret_val );

	if( p_protocal_handler != NULL && p_protocal_handler->_response_handler != NULL )
	{
		ret_val = p_protocal_handler->_response_handler( p_protocal_handler, ip, port, 0, p_r_dict );
		sd_assert( ret_val == SUCCESS );
	}	

	ret_val = rt_add_rout_node( rt_ptr(DHT_TYPE), p_node );
	if( ret_val != SUCCESS )
	{
		k_node_destory( p_node );
		return ret_val;
	}	
	
	return SUCCESS;		
}

_int32 dht_on_err( _u32 ip, _u16 port, BC_DICT *p_dict_data )
{
 	LOG_DEBUG( "dht_on_err" );
    //sd_assert( FALSE );
	return SUCCESS;
}

_int32 dht_add_parser_node( _u32 ip, _u16 port, BC_DICT *p_dict_data )
{
	_int32 ret_val = SUCCESS;
	BC_DICT *p_r_dict = NULL;
	BC_STR *p_id_str = NULL;
	K_NODE *p_node = NULL;
	
 	LOG_DEBUG( "dht_add_parser_node" );

	ret_val = bc_dict_get_value( p_dict_data, "r", (BC_OBJ **)&p_r_dict );
	CHECK_VALUE( ret_val );
	
	ret_val = bc_dict_get_value( p_r_dict, "id", (BC_OBJ **)&p_id_str );
	CHECK_VALUE( ret_val );

	ret_val = k_node_create( (_u8 *)p_id_str->_str_ptr, p_id_str->_str_len, ip, port, &p_node );
	CHECK_VALUE( ret_val );	

	ret_val = rt_add_rout_node( rt_ptr(DHT_TYPE), p_node );
	if( ret_val != SUCCESS )
	{
		k_node_destory( p_node );
	}	
	return SUCCESS;
}

_u32 dht_get_cur_packet_id( void )
{
 	LOG_DEBUG( "dht_get_cur_packet_id:%u", g_dht_send_packet_id );
	return g_dht_send_packet_id;
}


_int32 dht_get_packet_id( char packet_id[DHT_PACKET_ID_BUFFER_LEN] )
{
	g_dht_send_packet_id++;
	
 	LOG_DEBUG( "dht_get_packet_id:%u", g_dht_send_packet_id );

	sd_memset( packet_id, 0, DHT_PACKET_ID_BUFFER_LEN );
	sd_u32_to_str( g_dht_send_packet_id, packet_id, DHT_PACKET_ID_BUFFER_LEN );
	
	return SUCCESS;
}

#endif

