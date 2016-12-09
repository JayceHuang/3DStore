#include "utility/define.h"
#ifdef _DK_QUERY

#include "get_peers_handler.h"
#include "dk_task.h"
#include "dk_setting.h"
#include "find_node_handler.h"
#include "dk_define.h"

#include "utility/mempool.h"

#include "utility/bytebuffer.h"
#include "dk_manager.h"
#include "utility/utility.h"

#include "utility/logid.h"
#define LOGID LOGID_DK_QUERY
#include "utility/logger.h"

_int32 get_peers_init( GET_PEERS_HANDLER *p_get_peers_handler, _u8 *p_target, _u32 target_len, void *p_user )
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "get_peers_init.p_get_peers_handler:0x%x", p_get_peers_handler);

	ret_val = fnh_init( DHT_TYPE, &p_get_peers_handler->_find_node, p_target, target_len, DHT_FIND_SOURCE );
	CHECK_VALUE( ret_val );

	find_node_set_build_cmd_handler( &p_get_peers_handler->_find_node, dht_build_get_peers_cmd );
	find_node_set_response_handler( &p_get_peers_handler->_find_node, get_peers_response_handler );

	p_get_peers_handler->_user_ptr = p_user;
	return SUCCESS;
}

_int32 get_peers_update( GET_PEERS_HANDLER *p_get_peers_handler )
{	
	LOG_DEBUG( "get_peers_update.p_get_peers_handler:0x%x", p_get_peers_handler);
	
	fnh_update( &p_get_peers_handler->_find_node );
		
	return SUCCESS;
}
	
_int32 get_peers_uninit( GET_PEERS_HANDLER *p_get_peers_handler )
{
	LOG_DEBUG( "get_peers_uninit.p_get_peers_handler:0x%x", p_get_peers_handler);

	fnh_uninit( &p_get_peers_handler->_find_node );
	
	p_get_peers_handler->_user_ptr = NULL;

	return SUCCESS;
}

_int32 get_peers_response_handler( DK_PROTOCAL_HANDLER *p_handler, _u32 ip, _u32 port, _u32 invalid_para, void *p_data )
{
	_int32 ret_val = SUCCESS;
	GET_PEERS_HANDLER *p_get_peers_handler = (GET_PEERS_HANDLER *)p_handler;
	BC_DICT *p_r_dict = ( BC_DICT * )p_data;
	BC_STR *p_nodes_str = NULL;
	BC_STR *p_token_str = NULL;
	BC_OBJ *p_values_obj = NULL;
	BC_LIST *p_values_list = NULL;
    char *p_tmp_str = NULL;
    
	_u32 find_ip = 0;
	_u16 find_port = 0;
	_u32 str_len = 0;
	_u32 node_num = 0;
	_u32 node_index = 0;
	LIST_ITERATOR cur_node = NULL;
    
#ifdef _DEBUG
    char addr[24] = {0};
    sd_inet_ntoa( ip, addr, 24);
    LOG_DEBUG( "get_peers_response_handler.p_handler:0x%x, ip:%s, port:%d", 
        p_handler, addr, port );
#endif

	LOG_DEBUG( "get_peers_response_handler.p_get_peers_handler:0x%x", p_get_peers_handler);

	ret_val = bc_dict_get_value( p_r_dict, "nodes", (BC_OBJ **)&p_nodes_str );
	CHECK_VALUE( ret_val );
    
	if( p_nodes_str != NULL )
	{
		return dht_find_node_response_handler( p_handler, ip, port, 0, p_data );
	}
	
	ret_val = bc_dict_get_value( p_r_dict, "values", (BC_OBJ **)&p_values_obj );
	CHECK_VALUE( ret_val );
    if( p_values_obj == NULL )
    {
        ret_val = bc_dict_get_value( p_r_dict, "value", (BC_OBJ **)&p_values_obj );
        CHECK_VALUE( ret_val );
    }

	if( p_values_obj == NULL || p_values_obj->_bc_type != LIST_TYPE )
	{
		//sd_assert( FALSE );
		return SUCCESS;
	}
	ret_val = bc_dict_get_value( p_r_dict, "token", (BC_OBJ **)&p_token_str );
	CHECK_VALUE( ret_val );
	
	p_values_list = (BC_LIST *)p_values_obj;

	cur_node = LIST_BEGIN( p_values_list->_list );
	while( cur_node != LIST_END( p_values_list->_list ) )
	{
        node_index = 0;
		p_nodes_str = ( BC_STR *)LIST_VALUE( cur_node );

		str_len = p_nodes_str->_str_len;
		if( str_len % DHT_COMPACT_ADDR_SIZE != 0 )
		{
			//sd_assert( FALSE );
			return SUCCESS; 
		}

		node_num = str_len / DHT_COMPACT_ADDR_SIZE;
        
        p_tmp_str = p_nodes_str->_str_ptr;
        
		while( node_index < node_num )
		{
			ret_val = sd_get_int32_from_bg( &p_tmp_str, (_int32 *)&str_len, (_int32 *)&find_ip );
			CHECK_VALUE( ret_val );	
            
			find_ip= sd_htonl( find_ip );
            
			ret_val = sd_get_int16_from_bg( &p_tmp_str, (_int32 *)&str_len, (_int16 *)&find_port);
			CHECK_VALUE( ret_val );	

			ret_val = dht_res_nofity( p_get_peers_handler->_user_ptr, find_ip, find_port );
			CHECK_VALUE( ret_val );	
			
			node_index++;
		}
		sd_assert( str_len == 0 );		

		cur_node = LIST_NEXT( cur_node );
	}
    
    find_node_remove_node_info( (FIND_NODE_HANDLER *)p_handler, ip, port );
	
	get_peers_announce( p_get_peers_handler, ip, 
		port, p_token_str->_str_ptr, p_token_str->_str_len );

	return SUCCESS;	

}

_int32 get_peers_announce( GET_PEERS_HANDLER *p_get_peers_handler, _u32 ip, _u16 port, char *p_token, _u32 token_len )
{
	_int32 ret_val = SUCCESS;
	char *p_cmd_buffer = NULL;
	_u32 buffer_len = 0;
    _u8 *p_target_id = NULL;
    
#ifdef _DEBUG
    char addr[24] = {0};

    sd_inet_ntoa( ip, addr, 24);
    LOG_DEBUG( "get_peers_announce.p_get_peers_handler:0x%x, ip:%s, port:%d", 
        p_get_peers_handler, addr, port );
#endif	

    p_target_id = k_distance_get_bit_buffer( &p_get_peers_handler->_find_node._target );
	ret_val = dht_build_announce_cmd( p_target_id, sh_get_udp_port(sh_ptr(DHT_TYPE)),
		p_token, token_len, &p_cmd_buffer, &buffer_len );
	CHECK_VALUE( ret_val );
	
	ret_val = sh_send_packet( sh_ptr(DHT_TYPE), ip, port, p_cmd_buffer, buffer_len, NULL, 0 );
	if( ret_val != SUCCESS ) 
	{
		SAFE_DELETE( p_cmd_buffer );
	}
	
	return SUCCESS;
}

FIND_NODE_STATUS get_peers_get_status( GET_PEERS_HANDLER *p_get_peers_handler )
{
	LOG_DEBUG( "get_peers_get_status.p_get_peers_handler:0x%x", p_get_peers_handler);
	return fnh_get_status( &p_get_peers_handler->_find_node );
}

_int32 get_peers_add_node( GET_PEERS_HANDLER *p_get_peers_handler, _u32 ip, _u16 port )
{
	LOG_DEBUG( "get_peers_add_node begin.p_get_peers_handler:0x%x", p_get_peers_handler);
    if( fnh_get_status( &p_get_peers_handler->_find_node ) != FIND_NODE_RUNNING )
    {
        return fnh_add_node( &p_get_peers_handler->_find_node, ip, port, 0, NULL, 0 );
    }
    
	LOG_DEBUG( "get_peers_add_node end.p_get_peers_handler:0x%x", p_get_peers_handler);
    return SUCCESS;
}

#endif

