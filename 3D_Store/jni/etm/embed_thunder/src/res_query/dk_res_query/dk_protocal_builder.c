#include "utility/define.h"
#ifdef _DK_QUERY

#include "dk_protocal_builder.h"
#include "bt_download/torrent_parser/bencode.h"
#include "dht_recv_packet_handler.h"
#include "routing_table.h"
#include "kad_packet_handler.h"

#include "utility/logid.h"
#define LOGID LOGID_DK_QUERY
#include "utility/logger.h"

#include "utility/errcode.h"

#include "utility/mempool.h"
#include "utility/define_const_num.h"

#include "utility/utility.h"

#define DHT_TOKEN_LEN 10

static _u32 g_dht_token_id = 10000;

_int32 dht_build_ping_cmd( NODE_INFO *p_node_info, char **pp_cmd_buffer, _u32 *p_buffer_len )
{
	_int32 ret_val = SUCCESS;

	BC_DICT *p_bc_dict = NULL;
	BC_DICT *p_a_dict = NULL;
	
	LOG_DEBUG( "dht_build_ping_cmd" );

	ret_val = bc_dict_create( &p_a_dict );
	if( ret_val != SUCCESS ) goto ErrHandle;
	
	ret_val = dht_build_set_local_id( p_a_dict );
	if( ret_val != SUCCESS ) goto ErrHandle;

	ret_val = dht_get_query_dict( "ping", p_a_dict, &p_bc_dict );
	if( ret_val != SUCCESS ) goto ErrHandle;
	
	ret_val = dht_create_buffer_from_dict( p_bc_dict, pp_cmd_buffer, p_buffer_len );
	
	bc_dict_uninit( (BC_OBJ *)p_bc_dict );

#ifdef _PACKET_DEBUG
	{
		char test_buffer[1024];
		sd_memset(test_buffer,0,1024);
		sd_memcpy(test_buffer,*pp_cmd_buffer, MIN( *p_buffer_len,1024));
		LOG_DEBUG( "dht_build_ping_cmd:*buffer[%u]=%s",*p_buffer_len, test_buffer);
	}
#endif 

	return ret_val;

ErrHandle:
	bc_dict_uninit( (BC_OBJ *)p_a_dict );
	return ret_val;	
}

_int32 dht_build_find_node_cmd( NODE_INFO *p_invalid_para, K_NODE_ID *p_node_id, _u8 invalid_para, char **pp_cmd_buffer, _u32 *p_buffer_len, _u32 *p_key )
{
	_int32 ret_val = SUCCESS;
	
	BC_DICT *p_bc_dict = NULL;
	BC_DICT *p_a_dict = NULL;
	BC_STR *p_info_hash_str = NULL;
	
	LOG_DEBUG( "dht_build_find_node_cmd" );

	ret_val = bc_dict_create( &p_a_dict );
	if( ret_val != SUCCESS ) goto ErrHandle;

	ret_val = bc_str_create_with_value( (char *)k_distance_get_bit_buffer(p_node_id), k_distance_buffer_len(p_node_id), &p_info_hash_str );
	if( ret_val != SUCCESS ) goto ErrHandle;


	ret_val = bc_dict_set_value( p_a_dict, "target", (BC_OBJ *)p_info_hash_str  );
	if( ret_val != SUCCESS ) goto ErrHandle;

	
	ret_val = dht_build_set_local_id( p_a_dict );
	if( ret_val != SUCCESS ) goto ErrHandle;
	
	
	ret_val = dht_get_query_dict( "find_node", p_a_dict, &p_bc_dict );
	if( ret_val != SUCCESS ) goto ErrHandle;

	ret_val = dht_create_buffer_from_dict( p_bc_dict, pp_cmd_buffer, p_buffer_len );

	bc_dict_uninit( (BC_OBJ *)p_bc_dict );
	*p_key = dht_get_cur_packet_id();
	
#ifdef _PACKET_DEBUG
	{
		char test_buffer[1024];
		sd_memset(test_buffer,0,1024);
		sd_memcpy(test_buffer,*pp_cmd_buffer, MIN( *p_buffer_len,1024));
		LOG_DEBUG( "dht_build_find_node_cmd:*buffer[%u]=%s",*p_buffer_len, test_buffer);
	}
#endif 

	return ret_val;

ErrHandle:
	bc_dict_uninit( (BC_OBJ *)p_a_dict );
	return ret_val;	

}

_int32 dht_build_get_peers_cmd( NODE_INFO *p_invalid_para, K_NODE_ID *p_node_id, _u8 invalid_para, char **pp_cmd_buffer, _u32 *p_buffer_len, _u32 *p_key )
{
	_int32 ret_val = SUCCESS;
	BC_DICT *p_bc_dict = NULL;
	BC_DICT *p_a_dict = NULL;
	BC_STR *p_info_hash_str = NULL;
	
	LOG_DEBUG( "dht_build_get_peers_cmd" );

	ret_val = bc_dict_create( &p_a_dict );
	if( ret_val != SUCCESS ) goto ErrHandle;

	ret_val = bc_str_create_with_value( (char *)k_distance_get_bit_buffer(p_node_id), k_distance_buffer_len(p_node_id), &p_info_hash_str );
	if( ret_val != SUCCESS ) goto ErrHandle;

	ret_val = bc_dict_set_value( p_a_dict, "info_hash", (BC_OBJ *)p_info_hash_str  );
	if( ret_val != SUCCESS ) goto ErrHandle1;
	
	ret_val = dht_build_set_local_id( p_a_dict );
	if( ret_val != SUCCESS ) goto ErrHandle;

	ret_val = dht_get_query_dict( "get_peers", p_a_dict, &p_bc_dict );
	if( ret_val != SUCCESS ) goto ErrHandle;

	ret_val = dht_create_buffer_from_dict( p_bc_dict, pp_cmd_buffer, p_buffer_len );
	bc_dict_uninit( (BC_OBJ *)p_bc_dict );
	*p_key = dht_get_cur_packet_id();

#ifdef _PACKET_DEBUG
	{
		char test_buffer[1024];
		sd_memset(test_buffer,0,1024);
        
		sd_memcpy(test_buffer,*pp_cmd_buffer, MIN( *p_buffer_len,1024) );
		LOG_DEBUG( "dht_build_get_peers_cmd:*buffer[%u]=%s",*p_buffer_len, test_buffer);
	}
#endif 	
	return ret_val;

ErrHandle1:
	bc_str_uninit( (BC_OBJ *)p_info_hash_str );
ErrHandle:
	bc_dict_uninit( (BC_OBJ *)p_a_dict );
	return ret_val;	

}

_int32 dht_build_announce_cmd( _u8 infohash[DHT_ID_LEN], _u16 port, 
	char *p_token, _u32 token_len, char **pp_cmd_buffer, _u32 *p_buffer_len )
{
	_int32 ret_val = SUCCESS;
	BC_DICT *p_bc_dict = NULL;
	BC_DICT *p_a_dict = NULL;
	BC_STR *p_info_hash_str = NULL;
	BC_INT *p_port_int = NULL;
	BC_STR *p_token_str = NULL;
	
	LOG_DEBUG( "dht_build_announce_cmd" );

	ret_val = bc_dict_create( &p_a_dict );
	if( ret_val != SUCCESS ) goto ErrHandle;

	ret_val = bc_str_create_with_value( (char *)infohash, DHT_ID_LEN, &p_info_hash_str );
	if( ret_val != SUCCESS ) goto ErrHandle;

	ret_val = bc_dict_set_value( p_a_dict, "info_hash", (BC_OBJ *)p_info_hash_str  );
	if( ret_val != SUCCESS ) goto ErrHandle1;
	
	ret_val = bc_int_create_with_value( port, &p_port_int );
	if( ret_val != SUCCESS ) goto ErrHandle;

	ret_val = bc_dict_set_value( p_a_dict, "port", (BC_OBJ *)p_port_int  );
	if( ret_val != SUCCESS ) goto ErrHandle2;
	
	ret_val = bc_str_create_with_value( p_token, token_len, &p_token_str );
	if( ret_val != SUCCESS ) goto ErrHandle;

	ret_val = bc_dict_set_value( p_a_dict, "token", (BC_OBJ *)p_token_str  );
	if( ret_val != SUCCESS ) goto ErrHandle3;

	ret_val = dht_build_set_local_id( p_a_dict );
	if( ret_val != SUCCESS ) goto ErrHandle;

	ret_val = dht_get_query_dict( "announce_peer", p_a_dict, &p_bc_dict );
	if( ret_val != SUCCESS ) goto ErrHandle;


	ret_val = dht_create_buffer_from_dict( p_bc_dict, pp_cmd_buffer, p_buffer_len );
	bc_dict_uninit( (BC_OBJ *)p_bc_dict );
	
#ifdef _PACKET_DEBUG
	{
		char test_buffer[1024];
		sd_memset(test_buffer,0,1024);
		sd_memcpy(test_buffer,*pp_cmd_buffer, MIN( *p_buffer_len,1024));
		LOG_DEBUG( "dht_build_announce_cmd:*buffer[%u]=%s",*p_buffer_len, test_buffer);
	}
#endif 	

	return ret_val;

ErrHandle3:
	bc_str_uninit( (BC_OBJ *)p_token_str );
	bc_dict_uninit( (BC_OBJ *)p_a_dict );
	return ret_val;

ErrHandle2:
	bc_int_uninit( (BC_OBJ *)p_port_int );
	bc_dict_uninit( (BC_OBJ *)p_a_dict );
	return ret_val;

ErrHandle1:
	bc_str_uninit( (BC_OBJ *)p_info_hash_str );
	bc_dict_uninit( (BC_OBJ *)p_a_dict );
	return ret_val;

ErrHandle:
	bc_dict_uninit( (BC_OBJ *)p_a_dict );
	return ret_val;	

}

_int32 dht_get_query_dict( const char *p_q, BC_DICT *p_a_dict, BC_DICT **pp_bc_dict )
{
	_int32 ret_val = SUCCESS;
	BC_DICT *p_bc_dict = NULL;
	BC_STR *p_t_str = NULL;
	BC_STR *p_y_str = NULL;
	BC_STR *p_q_str = NULL;
	char packet_id[DHT_PACKET_ID_BUFFER_LEN];
	
	LOG_DEBUG( "dht_get_query_dict, p_q:%s", p_q );

	ret_val = dht_get_packet_id( packet_id );
	CHECK_VALUE( ret_val );
	
	ret_val = bc_dict_create( &p_bc_dict );
	CHECK_VALUE( ret_val );

	ret_val = bc_str_create_with_value( packet_id, sd_strlen(packet_id), &p_t_str );
	if( ret_val != SUCCESS ) goto ErrHandle;

	ret_val = bc_dict_set_value( p_bc_dict, "t", (BC_OBJ *)p_t_str  );
	if( ret_val != SUCCESS ) goto ErrHandle1;
	
	ret_val = bc_str_create_with_value( "q", 1, &p_y_str );
	if( ret_val != SUCCESS ) goto ErrHandle;

	ret_val = bc_dict_set_value( p_bc_dict, "y", (BC_OBJ *)p_y_str  );
	if( ret_val != SUCCESS ) goto ErrHandle2;


	ret_val = bc_str_create_with_value( p_q, sd_strlen(p_q), &p_q_str );
	if( ret_val != SUCCESS ) goto ErrHandle;

	ret_val = bc_dict_set_value( p_bc_dict, "q", (BC_OBJ *)p_q_str  );
	if( ret_val != SUCCESS ) goto ErrHandle3;

	ret_val = bc_dict_set_value( p_bc_dict, "a", (BC_OBJ *)p_a_dict  );
	if( ret_val != SUCCESS ) goto ErrHandle;

	*pp_bc_dict = p_bc_dict;

	return SUCCESS;
	
ErrHandle3:
	bc_str_uninit( (BC_OBJ *)p_q_str );
	bc_dict_uninit( (BC_OBJ *)p_bc_dict );	
	return ret_val; 
	
ErrHandle2:
	bc_str_uninit( (BC_OBJ *)p_y_str );
	bc_dict_uninit( (BC_OBJ *)p_bc_dict );	
	return ret_val; 

ErrHandle1:
	bc_str_uninit( (BC_OBJ *)p_t_str );
	bc_dict_uninit( (BC_OBJ *)p_bc_dict );	
	return ret_val;	
	
ErrHandle:
	bc_dict_uninit( (BC_OBJ *)p_bc_dict );
	return ret_val;		
}

_int32 dht_build_ping_or_announce_resp_cmd( char *p_packet_id, _u32 packet_len, char **pp_cmd_buffer, _u32 *p_buffer_len )
{
	_int32 ret_val = SUCCESS;
	BC_DICT *p_bc_dict = NULL;

	BC_DICT *p_r_dict = NULL;
	
	LOG_DEBUG( "dht_build_ping_or_announce_resp_cmd" );

	ret_val = bc_dict_create( &p_r_dict );
	if( ret_val != SUCCESS ) goto ErrHandle;
	
	ret_val = dht_build_set_local_id( p_r_dict );
	if( ret_val != SUCCESS ) goto ErrHandle;

	ret_val = dht_get_resp_dict( p_packet_id, packet_len, p_r_dict, &p_bc_dict );
	if( ret_val != SUCCESS ) goto ErrHandle;

	ret_val = dht_create_buffer_from_dict( p_bc_dict, pp_cmd_buffer, p_buffer_len );
	bc_dict_uninit( (BC_OBJ *)p_bc_dict );
	
#ifdef _PACKET_DEBUG
	{
		char test_buffer[1024];
		sd_memset(test_buffer,0,1024);
		sd_memcpy(test_buffer,*pp_cmd_buffer, MIN( *p_buffer_len,1024));
		LOG_DEBUG( "dht_build_ping_or_announce_resp_cmd:*buffer[%u]=%s",*p_buffer_len, test_buffer);
	}
#endif 	

	return ret_val;

ErrHandle:
	bc_dict_uninit( (BC_OBJ *)p_r_dict );
	return ret_val;
	
}

_int32 dht_build_find_node_resp_cmd( char *p_packet_id, _u32 packet_len, 
	char *p_nodes, _u32 nodes_str_len, char **pp_cmd_buffer, _u32 *p_buffer_len )
{
	_int32 ret_val = SUCCESS;
	BC_DICT *p_bc_dict = NULL;
	BC_DICT *p_r_dict = NULL;
	BC_STR *p_nodes_str = NULL;
	
	LOG_DEBUG( "dht_build_find_node_resp_cmd" );

	ret_val = bc_dict_create( &p_r_dict );
	if( ret_val != SUCCESS ) goto ErrHandle;
	
	ret_val = dht_build_set_local_id( p_r_dict );
	if( ret_val != SUCCESS ) goto ErrHandle;

	ret_val = bc_str_create_with_value( p_nodes, nodes_str_len, &p_nodes_str );
	if( ret_val != SUCCESS ) goto ErrHandle;
	
	ret_val = bc_dict_set_value( p_r_dict, "nodes", (BC_OBJ *)p_nodes_str  );
	if( ret_val != SUCCESS ) goto ErrHandle1;

	ret_val = dht_get_resp_dict( p_packet_id, packet_len, p_r_dict, &p_bc_dict );
	if( ret_val != SUCCESS ) goto ErrHandle;

	ret_val = dht_create_buffer_from_dict( p_bc_dict, pp_cmd_buffer, p_buffer_len );
	bc_dict_uninit( (BC_OBJ *)p_bc_dict );

#ifdef _PACKET_DEBUG
	{
		char test_buffer[1024];
		sd_memset(test_buffer,0,1024);
		sd_memcpy(test_buffer,*pp_cmd_buffer, MIN( *p_buffer_len,1024));
		LOG_DEBUG( "dht_build_find_node_resp_cmd:*buffer[%u]=%s",*p_buffer_len, test_buffer);
	}
#endif 	

	return ret_val;

ErrHandle1:
	bc_str_uninit( (BC_OBJ *)p_nodes_str );
	
ErrHandle:
	bc_dict_uninit( (BC_OBJ *)p_r_dict );
	return ret_val;
}

_int32 dht_build_get_peers_resp_cmd( char *p_packet_id, _u32 packet_len,
	const char* p_info_key, char *p_info_value, _u32 info_len, char **pp_cmd_buffer, _u32 *p_buffer_len )
{
	_int32 ret_val = SUCCESS;
	BC_DICT *p_bc_dict = NULL;
	BC_DICT *p_r_dict = NULL;
	BC_STR *p_bc_info_str = NULL;
	BC_STR *p_bc_token_str = NULL;
	char token[DHT_TOKEN_LEN];
	
	LOG_DEBUG( "dht_build_get_peers_resp_cmd" );

	sd_memset( token, 0, DHT_TOKEN_LEN );
	sd_u32_to_str( g_dht_token_id, token, DHT_TOKEN_LEN );
	g_dht_token_id++;
	
	ret_val = bc_dict_create( &p_r_dict );
	if( ret_val != SUCCESS ) goto ErrHandle;
	
	ret_val = dht_build_set_local_id( p_r_dict );
	if( ret_val != SUCCESS ) goto ErrHandle;

	ret_val = bc_str_create_with_value( p_info_value, info_len, &p_bc_info_str );
	if( ret_val != SUCCESS ) goto ErrHandle;

	ret_val = bc_dict_set_value( p_r_dict, p_info_key, (BC_OBJ *)p_bc_info_str  );
	if( ret_val != SUCCESS ) goto ErrHandle1;

	ret_val = bc_str_create_with_value( token, DHT_TOKEN_LEN, &p_bc_token_str );
	if( ret_val != SUCCESS ) goto ErrHandle;

	ret_val = bc_dict_set_value( p_r_dict, "token", (BC_OBJ *)p_bc_token_str  );
	if( ret_val != SUCCESS ) goto ErrHandle2;

	ret_val = dht_get_resp_dict( p_packet_id, packet_len, p_r_dict, &p_bc_dict );
	if( ret_val != SUCCESS ) goto ErrHandle;

	ret_val = dht_create_buffer_from_dict( p_bc_dict, pp_cmd_buffer, p_buffer_len );
	bc_dict_uninit( (BC_OBJ *)p_bc_dict );

#ifdef _PACKET_DEBUG
	{
		char test_buffer[1024];
		sd_memset(test_buffer,0,1024);
		sd_memcpy(test_buffer,*pp_cmd_buffer, MIN( *p_buffer_len,1024));
		LOG_DEBUG( "dht_build_get_peers_resp_cmd:*buffer[%u]=%s",*p_buffer_len, test_buffer);
	}
#endif 	

	return ret_val;

ErrHandle2:
	bc_str_uninit( (BC_OBJ *)p_bc_token_str );
	bc_dict_uninit( (BC_OBJ *)p_r_dict );
	return ret_val;

ErrHandle1:
	bc_str_uninit( (BC_OBJ *)p_bc_info_str );
	bc_dict_uninit( (BC_OBJ *)p_r_dict );
	return ret_val;

ErrHandle:
	bc_dict_uninit( (BC_OBJ *)p_r_dict );
	return ret_val;

}

_int32 dht_get_resp_dict( char *p_packet_id, _u32 packet_len, BC_DICT *p_r_dict, BC_DICT **pp_bc_dict )
{
	_int32 ret_val = SUCCESS;
	BC_DICT *p_bc_dict = NULL;
	BC_STR *p_t_str = NULL;
	BC_STR *p_y_str = NULL;
	
	LOG_DEBUG( "dht_get_resp_dict" );

	ret_val = bc_dict_create( &p_bc_dict );
	CHECK_VALUE( ret_val );

	ret_val = bc_str_create_with_value( p_packet_id, packet_len, &p_t_str );
	if( ret_val != SUCCESS ) goto ErrHandle;

	ret_val = bc_dict_set_value( p_bc_dict, "t", (BC_OBJ *)p_t_str  );
	if( ret_val != SUCCESS ) goto ErrHandle1;

	ret_val = bc_str_create_with_value( "r", 1, &p_y_str );
	if( ret_val != SUCCESS ) goto ErrHandle;

	ret_val = bc_dict_set_value( p_bc_dict, "y", (BC_OBJ *)p_y_str  );
	if( ret_val != SUCCESS ) goto ErrHandle2;

	ret_val = bc_dict_set_value( p_bc_dict, "r", (BC_OBJ *)p_r_dict  );
	if( ret_val != SUCCESS ) goto ErrHandle;

	*pp_bc_dict = p_bc_dict;

	return SUCCESS;
	
ErrHandle2:
	bc_str_uninit( (BC_OBJ *)p_y_str );
	bc_dict_uninit( (BC_OBJ *)p_bc_dict );
	return ret_val;		
		
ErrHandle1:
	bc_str_uninit( (BC_OBJ *)p_t_str );
	bc_dict_uninit( (BC_OBJ *)p_bc_dict );
	return ret_val;		
	
ErrHandle:
	bc_dict_uninit( (BC_OBJ *)p_bc_dict );
	return ret_val;	
}


_int32 dht_build_set_local_id( BC_DICT *p_dict )
{
	_int32 ret_val = SUCCESS;
	K_NODE_ID *p_k_node_id = NULL;
	BC_STR *p_id_str = NULL;
	
	LOG_DEBUG( "dht_build_set_local_id" );

	p_k_node_id = rt_get_local_id( rt_ptr(DHT_TYPE) );
	ret_val = bc_str_create_with_value( (char *)k_distance_get_bit_buffer(p_k_node_id),
		k_distance_buffer_len(p_k_node_id), &p_id_str );
	if( ret_val != SUCCESS )
	{
		return ret_val;
	}

	ret_val = bc_dict_set_value( p_dict, "id", (BC_OBJ *)p_id_str  );
	if( ret_val != SUCCESS )
	{
		bc_str_uninit( (BC_OBJ *)p_id_str );
		return ret_val;
	}	
	return SUCCESS;	
}

_int32 dht_create_buffer_from_dict( BC_DICT *p_bc_dict, char **pp_cmd_buffer, _u32 *p_buffer_len )
{
	_int32 ret_val = SUCCESS;
	char packet_buffer[ DK_PACKET_MAX_LEN ];
	_u32 packet_len = 0;
	
	LOG_DEBUG( "dht_create_buffer_from_dict" );

	ret_val = bc_dict_to_str( (BC_OBJ *)p_bc_dict, packet_buffer, DK_PACKET_MAX_LEN, &packet_len );
	if( ret_val != SUCCESS )
	{
		return ret_val;
	}
	//需要修改bc_dict_to_str的buffer不够情况下返回错误码!!!!!

	ret_val = dk_get_cmd_buffer( packet_buffer, packet_len, pp_cmd_buffer, p_buffer_len );
	CHECK_VALUE( ret_val );

	return SUCCESS;	
}

#ifdef ENABLE_EMULE

_int32 kad_build_ping_cmd( NODE_INFO *p_node_info, char **pp_cmd_buffer, _u32 *p_buffer_len )
{
	_int32 ret_val = SUCCESS;
    KAD_NODE_INFO *p_kad_node_info = (KAD_NODE_INFO *)p_node_info;
	char packet_buffer[ DK_PACKET_MAX_LEN ];
    char *p_tmp_buffer = packet_buffer;
	_u32 packet_buffer_len = DK_PACKET_MAX_LEN; 
    BOOL is_boot_query = !rt_is_ready( KAD_TYPE );
    
	//LOG_DEBUG( "kad_build_ping_cmd_common" );

	ret_val = kad_fill_query_ping_buffer( &p_tmp_buffer, &packet_buffer_len, p_kad_node_info->_version, is_boot_query );
	CHECK_VALUE( ret_val );   

    sd_assert( packet_buffer_len < DK_PACKET_MAX_LEN );
    packet_buffer_len = DK_PACKET_MAX_LEN - packet_buffer_len;
    
	ret_val = dk_get_cmd_buffer( packet_buffer, packet_buffer_len, pp_cmd_buffer, p_buffer_len );
	CHECK_VALUE( ret_val );

	return SUCCESS;
}

_int32 kad_build_find_node_cmd( NODE_INFO *p_node_info, K_NODE_ID *p_node_id, _u8 cmd_type, char **pp_cmd_buffer, _u32 *p_buffer_len, _u32 *p_packet_key )
{
	_int32 ret_val = SUCCESS;
    
    KAD_NODE_INFO *p_kad_node_info = (KAD_NODE_INFO *)p_node_info;
	char packet_buffer[ DK_PACKET_MAX_LEN ];
    char *p_tmp_buffer = packet_buffer;
	_u32 packet_buffer_len = DK_PACKET_MAX_LEN; 
    _u8 *p_key = k_distance_get_bit_buffer( p_node_id );
    _u32 key_len = k_distance_buffer_len( p_node_id );

	LOG_DEBUG( "kad_build_find_node_cmd" );
    if( p_kad_node_info->_node_id_ptr == NULL )
    {
        CHECK_VALUE( KAD_NODE_ID_ERR );
    }
	ret_val = kad_fill_query_find_node_buffer( &p_tmp_buffer, &packet_buffer_len, 
        p_kad_node_info->_version, cmd_type, p_key, key_len, p_kad_node_info->_node_id_ptr );
	CHECK_VALUE( ret_val );   

    sd_assert( packet_buffer_len < DK_PACKET_MAX_LEN );
    packet_buffer_len = DK_PACKET_MAX_LEN - packet_buffer_len;
    
	ret_val = dk_get_cmd_buffer( packet_buffer, packet_buffer_len, pp_cmd_buffer, p_buffer_len );
	CHECK_VALUE( ret_val ); 
	
    *p_packet_key = dk_calc_key( KAD_ID_LEN, 
        KAD_ID_LEN, p_key, key_len );
    
	return SUCCESS;

}

_int32 kad_build_find_source_cmd( _u32 ip, _u16 port, _u8 verion, K_NODE_ID *p_node_id, _u64 file_size, char **pp_cmd_buffer, _u32 *p_buffer_len, _u32 *p_packet_key  )
{
	_int32 ret_val = SUCCESS;
	char packet_buffer[ DK_PACKET_MAX_LEN ];
    char *p_tmp_buffer = packet_buffer;
	_u32 packet_buffer_len = DK_PACKET_MAX_LEN; 
    _u8 *p_key = k_distance_get_bit_buffer( p_node_id );
    _u32 key_len = k_distance_buffer_len( p_node_id );
    
	LOG_DEBUG( "kad_build_find_source_cmd" );
    
	ret_val = kad_fill_query_find_source_buffer( &p_tmp_buffer, &packet_buffer_len, 
        verion, file_size, p_key, key_len );
	CHECK_VALUE( ret_val ); 
    
    sd_assert( packet_buffer_len < DK_PACKET_MAX_LEN );
    packet_buffer_len = DK_PACKET_MAX_LEN - packet_buffer_len;
    
	ret_val = dk_get_cmd_buffer( packet_buffer, packet_buffer_len, pp_cmd_buffer, p_buffer_len );
	CHECK_VALUE( ret_val ); 
    
    *p_packet_key = dk_calc_key( DK_PACKET_MAX_LEN, DK_PACKET_MAX_LEN, p_key, key_len );
	return SUCCESS;
}

_int32 kad_build_announce_source_cmd( _u8 verion, K_NODE_ID *p_node_id,
	struct tagEMULE_TAG_LIST *p_tag_list, char **pp_cmd_buffer, _u32 *p_buffer_len )
{
	_int32 ret_val = SUCCESS;
	char packet_buffer[ DK_PACKET_MAX_LEN ];
    char *p_tmp_buffer = packet_buffer;
	_u32 packet_buffer_len = DK_PACKET_MAX_LEN; 
    _u8 *p_key = k_distance_get_bit_buffer( p_node_id );
    _u32 key_len = k_distance_buffer_len( p_node_id );
    
	LOG_DEBUG( "kad_build_announce_source_cmd" );
    
	ret_val = kad_fill_query_announce_souce_buffer( &p_tmp_buffer, &packet_buffer_len, 
        verion, p_key, key_len, p_tag_list );
	CHECK_VALUE( ret_val ); 
    
    sd_assert( packet_buffer_len < DK_PACKET_MAX_LEN );
    packet_buffer_len = DK_PACKET_MAX_LEN - packet_buffer_len;
    
	ret_val = dk_get_cmd_buffer( packet_buffer, packet_buffer_len, pp_cmd_buffer, p_buffer_len );
	CHECK_VALUE( ret_val ); 	
	return SUCCESS;
}
#endif

_int32 dk_get_cmd_buffer( char *p_cmd, _u32 cmd_len, char **pp_cmd_buffer, _u32 *p_cmd_buffer_len )
{
	_int32 ret_val = SUCCESS;
	ret_val = sd_malloc( cmd_len, (void **)pp_cmd_buffer );
	CHECK_VALUE( ret_val );

	ret_val = sd_memcpy( *pp_cmd_buffer, p_cmd, cmd_len );
	if( ret_val != SUCCESS )
	{
		SAFE_DELETE( *pp_cmd_buffer );
		CHECK_VALUE( ret_val );
	}
	*p_cmd_buffer_len = cmd_len;    
	
	return SUCCESS;    
}
#endif

