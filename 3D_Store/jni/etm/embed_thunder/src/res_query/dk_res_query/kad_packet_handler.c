#include "utility/define.h"
#ifdef _DK_QUERY
#ifdef ENABLE_EMULE
#include "utility/list.h"
#include "utility/local_ip.h"
#include "utility/mempool.h"
#include "utility/utility.h"
#include "utility/logid.h"
#define LOGID LOGID_DK_QUERY
#include "utility/logger.h"
#include "platform/sd_zlib.h"

#include "kad_packet_handler.h"
#include "dk_define.h"
#include "utility/bytebuffer.h"
#include "k_node.h"
#include "routing_table.h"
#include "dk_socket_handler.h"

#include "dk_protocal_handler.h"
#include "dk_setting.h"
#include "emule/emule_utility/emule_tag_list.h"
#include "emule/emule_utility/emule_memory_slab.h"

_int32 kad_fill_query_ping_buffer( char **pp_buffer, _u32 *p_data_len, _u8 version, BOOL is_query_boot )
{
    _int32 ret_val = SUCCESS;
	_u8 protocol = OP_KADEMLIAHEADER;
	_u8 opcode = 0;  
    
	LOG_DEBUG( "kad_fill_query_ping_buffer,version:%d, is_query_boot:%d", 
        version, is_query_boot );

    if( is_query_boot )
    {
        if( version >= KADEMLIA_VERSION2_47a )
        {
            opcode = KADEMLIA2_BOOTSTRAP_REQ;
        }
        else
        {
            opcode = KADEMLIA_BOOTSTRAP_REQ;
        }
    }
    else
    {
        if( version >= KADEMLIA_VERSION2_47a )
        {
            opcode = KADEMLIA2_HELLO_REQ;
        }
        else
        {
            opcode = KADEMLIA_HELLO_REQ;
        }       
    }
    
    ret_val = sd_set_int8( pp_buffer, (_int32 *)p_data_len, (_int8)protocol );
    CHECK_VALUE( ret_val ); 
    
    ret_val = sd_set_int8( pp_buffer, (_int32 *)p_data_len, (_int8)opcode );
    CHECK_VALUE( ret_val );  

    if( version < KADEMLIA_VERSION2_47a )
    {
        ret_val = kad_fill_my_detail( pp_buffer, p_data_len, FALSE );
        CHECK_VALUE( ret_val ); 
    }
    else if( !is_query_boot )
    {
        ret_val = kad_fill_my_detail( pp_buffer, p_data_len, TRUE );
        CHECK_VALUE( ret_val ); 
    }
	return SUCCESS;       
}

_int32 kad_fill_query_find_node_buffer( char **pp_buffer, _u32 *p_data_len, 
    _u8 version, _u8 find_type, _u8 *p_key, _u32 key_len, _u8 *p_id )
{
    _int32 ret_val = SUCCESS;
	_u8 protocol = OP_KADEMLIAHEADER;
	_u8 opcode = 0;  
    _u8 node_type = 0;
    
	LOG_DEBUG( "kad_fill_query_find_node_buffer,version:%d, find_type:%d", 
        version, find_type );

    if( version >= KADEMLIA_VERSION2_47a )
    {
        opcode = KADEMLIA2_REQ;
    }
    else
    {
        opcode = KADEMLIA_REQ;
    }

 	switch(find_type)
	{
        case KAD_FIND_NODE:
        case KAD_FIND_NODECOMPLETE:
            node_type = KADEMLIA_FIND_NODE;
            break;
        case KAD_FIND_KEYWORD:
        case KAD_FIND_FINDSOURCE:
        case KAD_FIND_NOTES:
            node_type = KADEMLIA_FIND_VALUE;
            break;
        case KAD_FIND_FINDBUDDY:
        case KAD_FIND_STOREFILE:
        case KAD_FIND_STOREKEYWORD:
        case KAD_FIND_STORENOTES:
            node_type = KADEMLIA_STORE;
            break;
        default:
            return DK_LOGIC_ERR;
            break;
	}
    ret_val = sd_set_int8( pp_buffer, (_int32 *)p_data_len, (_int8)protocol );
    CHECK_VALUE( ret_val ); 
    
    ret_val = sd_set_int8( pp_buffer, (_int32 *)p_data_len, (_int8)opcode );
    CHECK_VALUE( ret_val );  

    ret_val = sd_set_int8( pp_buffer, (_int32 *)p_data_len, (_int8)node_type );
    CHECK_VALUE( ret_val ); 
    
    ret_val = kad_set_bytes( pp_buffer, (_int32 *)p_data_len, (char *)p_key, key_len );
    CHECK_VALUE( ret_val ); 
    
    ret_val = kad_set_bytes( pp_buffer, (_int32 *)p_data_len, (char *)p_id, KAD_ID_LEN );
    CHECK_VALUE( ret_val );  

	return SUCCESS;       
}

_int32 kad_fill_query_find_source_buffer( char **pp_buffer, _u32 *p_data_len, 
    _u8 version, _u64 file_size, _u8 *p_key, _u32 key_len )
{
    _int32 ret_val = SUCCESS;
	_u8 protocol = OP_KADEMLIAHEADER;
	_u8 opcode = 0;  
    _u16 pos = 0;
    _u8 restrictive = 1;
    
	LOG_DEBUG( "kad_fill_query_find_source_buffer,version:%d, file_size:%llu", 
        version, file_size );

    if( version >= KADEMLIA_VERSION2_47a )
    {
        opcode = KADEMLIA2_SEARCH_SOURCE_REQ;
    }
    else
    {
        opcode = KADEMLIA_SEARCH_REQ;
    }
    
    ret_val = sd_set_int8( pp_buffer, (_int32 *)p_data_len, (_int8)protocol );
    CHECK_VALUE( ret_val ); 
    
    ret_val = sd_set_int8( pp_buffer, (_int32 *)p_data_len, (_int8)opcode );
    CHECK_VALUE( ret_val );  
    
    ret_val = kad_set_bytes( pp_buffer, (_int32 *)p_data_len, (char *)p_key, key_len );
    CHECK_VALUE( ret_val ); 
    
    if( version >= KADEMLIA_VERSION2_47a )
    {
        ret_val = sd_set_int16_to_lt( pp_buffer, (_int32 *)p_data_len, pos );
        CHECK_VALUE( ret_val ); 
        
        ret_val = sd_set_int64_to_lt( pp_buffer, (_int32 *)p_data_len, file_size );
        CHECK_VALUE( ret_val );    
    }
    else
    {
        ret_val = sd_set_int8( pp_buffer, (_int32 *)p_data_len, restrictive );
        CHECK_VALUE( ret_val ); 
    }    

	return SUCCESS;       
}


_int32 kad_fill_query_announce_souce_buffer( char **pp_buffer, _u32 *p_data_len, 
    _u8 version, _u8 *p_key, _u32 key_len, EMULE_TAG_LIST *p_tag_list )
{
    _int32 ret_val = SUCCESS;
	_u8 protocol = OP_KADEMLIAHEADER;
	_u8 opcode = 0;  
    _u16 temp  = 1;
    
    K_NODE_ID *p_k_node_id = rt_get_local_id( rt_ptr(KAD_TYPE) );
    _u8 *p_id_buffer = k_distance_get_bit_buffer( p_k_node_id );
    _u32 id_buffer_len = k_distance_buffer_len( p_k_node_id );
    
	LOG_DEBUG( "kad_fill_query_announce_souce_buffer,version:%d", version );

    if( version >= KADEMLIA_VERSION2_47a )
    {
        opcode = KADEMLIA2_PUBLISH_SOURCE_REQ;
    }
    else
    {
        opcode = KADEMLIA_PUBLISH_REQ;
    }
    
    ret_val = sd_set_int8( pp_buffer, (_int32 *)p_data_len, (_int8)protocol );
    CHECK_VALUE( ret_val ); 
    
    ret_val = sd_set_int8( pp_buffer, (_int32 *)p_data_len, (_int8)opcode );
    CHECK_VALUE( ret_val );  
    
    ret_val = kad_set_bytes( pp_buffer, (_int32 *)p_data_len, (char *)p_key, key_len );
    CHECK_VALUE( ret_val ); 
    
    if( version < KADEMLIA_VERSION2_47a )
    {
        ret_val = sd_set_int16_to_lt( pp_buffer, (_int32 *)p_data_len, temp );
        CHECK_VALUE( ret_val ); 
    }    
    
    ret_val = kad_set_bytes( pp_buffer, (_int32 *)p_data_len, (char *)p_id_buffer, id_buffer_len );
    CHECK_VALUE( ret_val ); 

    ret_val = emule_tag_list_to_buffer( p_tag_list, pp_buffer, (_int32 *)p_data_len );
    CHECK_VALUE( ret_val ); 

	return SUCCESS;       

}

_int32 kad_fill_query_call_back_buffer( char **pp_buffer, _u32 *p_data_len,
    _u8 buddy_hash[KAD_ID_LEN], _u8 file_id[KAD_ID_LEN], _u16 port )
{
    _int32 ret_val = SUCCESS;
	_u8 protocol = OP_KADEMLIAHEADER; //回应OP_EMULEPROT
	_u8 opcode = KADEMLIA_CALLBACK_REQ; 
    
	LOG_DEBUG( "kad_fill_query_call_back_buffer,port:%d", port );
    
    ret_val = sd_set_int8( pp_buffer, (_int32 *)p_data_len, (_int8)protocol );
    CHECK_VALUE( ret_val ); 
    
    ret_val = sd_set_int8( pp_buffer, (_int32 *)p_data_len, (_int8)opcode );
    CHECK_VALUE( ret_val );  
    
    ret_val = kad_set_bytes( pp_buffer, (_int32 *)p_data_len, (char *)buddy_hash, KAD_ID_LEN );
    CHECK_VALUE( ret_val );     

    ret_val = kad_set_bytes( pp_buffer, (_int32 *)p_data_len, (char *)file_id, KAD_ID_LEN );
    CHECK_VALUE( ret_val );  
    
    ret_val = sd_set_int16_to_lt( pp_buffer, (_int32 *)p_data_len, port );
    CHECK_VALUE( ret_val );     
    
	return SUCCESS;       
}

_int32 kad_recv_handle_recv_packet( _u32 ip, _u16 port, char *p_buffer, _u32 data_len )
{
    _int32 ret_val = SUCCESS;
	_u8 protocol = 0;
	_u8 opcode   = 0;
    char *p_zlib_buffer = NULL;
    _u32 zlib_buffer_len = 0;

#ifdef _LOGGER
    char addr[24] = {0};
    ret_val = sd_inet_ntoa(ip, addr, 24);
    sd_assert( ret_val == SUCCESS ); 
#endif

#ifdef _PACKET_DEBUG
    char test_buffer[1024];
    sd_memset(test_buffer,0,1024);
    str2hex( p_buffer, data_len, test_buffer, 1024);
    LOG_DEBUG( "kad_recv_handle_recv_packet:*hex_buffer[%u]=%s" ,(_u32)data_len, test_buffer);

    sd_memset(test_buffer,0,1024);
    sd_memcpy(test_buffer,p_buffer,1024);
    LOG_DEBUG( "kad_recv_handle_recv_packet:*buffer[%u]=%s",data_len, test_buffer);
#endif 

    ret_val = sd_get_int8( &p_buffer, (_int32 *)&data_len, (_int8 *)&protocol );
    CHECK_VALUE( ret_val );
    
    ret_val = sd_get_int8( &p_buffer, (_int32 *)&data_len, (_int8 *)&opcode );
    CHECK_VALUE( ret_val );
    
#ifdef _LOGGER
    LOG_DEBUG( "kad_recv_handle_recv_packet: host_ip = %s, port = %u, data_len:%u, protocol:%d, opcode:%d", 
        addr, port, data_len, protocol, opcode );
#endif

 	if ( OP_KADEMLIAPACKEDPROT == protocol )
	{
		//if ( !package.uncompress(data+2,len-2,0) )
		//{
		//	return KAD_PROTOCAL_ERR;
		//}
		//sd_assert( FALSE );
		ret_val = sd_zlib_uncompress( p_buffer, data_len, &p_zlib_buffer, &zlib_buffer_len );
        if( ret_val != SUCCESS ) return ret_val;

        p_buffer = p_zlib_buffer;
        data_len = zlib_buffer_len;
        
		protocol = OP_KADEMLIAHEADER;

#ifdef _PACKET_DEBUG
        sd_memset(test_buffer,0,1024);
        str2hex( p_buffer, data_len, test_buffer, 1024);
        LOG_DEBUG( "kad_recv_handle_recv_packet, OP_KADEMLIAPACKEDPROT protocol:*hex_buffer[%u]=%s" ,(_u32)data_len, test_buffer);

#endif         
	}
	
	if ( OP_EMULEPROT == protocol )
	{
		//emule_p2p_udp_handler::get_instance().handle_emule_udp_package(opcode,package,remote_addr);
		sd_assert( FALSE );
        return SUCCESS;
	}

	if ( OP_VC_NAT_HEADER == protocol )
	{
		// 穿透的包的格式是 protocol(8) + pack_len(32) + opcode(8)
		//uint8  temp = 0;
		//package >> temp >> temp >> temp >> opcode;

		//verycd_connection_manager::get_instance().handle_package(opcode,package,remote_addr);
		//return enResponse_ok;
		//sd_assert( FALSE );
        return SUCCESS;
	}
	
	if ( OP_KADEMLIAHEADER != protocol )
	{
        LOG_ERROR( "kad_recv_handle_recv_packet, protocol unknown:%d", protocol );
		//sd_assert( FALSE );
		return KAD_PROTOCAL_ERR;
	}
	
	switch(opcode)
	{
    	case KADEMLIA_BOOTSTRAP_REQ:
    	case KADEMLIA2_BOOTSTRAP_REQ:
    	case KADEMLIA_HELLO_REQ:
    	case KADEMLIA2_HELLO_REQ:
    	case KADEMLIA_REQ:
    	case KADEMLIA2_REQ:
    	case KADEMLIA_SEARCH_REQ:
    	case KADEMLIA2_SEARCH_SOURCE_REQ:
    		ret_val = kad_on_query( ip, port, p_buffer, data_len, opcode );
            break;
    	case KADEMLIA_BOOTSTRAP_RES:
    	case KADEMLIA2_BOOTSTRAP_RES:
    	case KADEMLIA_HELLO_RES:
    	case KADEMLIA2_HELLO_RES:
    	case KADEMLIA_RES:
    	case KADEMLIA2_RES:
    	case KADEMLIA_SEARCH_RES:
    	case KADEMLIA2_SEARCH_RES:
    	case KADEMLIA_FINDBUDDY_RES:
    		ret_val =  kad_on_response( ip, port, p_buffer, data_len, opcode );
            break;
    	default:
            ret_val = KAD_PROTOCAL_ERR;
		break;
	}

    SAFE_DELETE( p_zlib_buffer );
    //CHECK_VALUE( ret_val );
	return SUCCESS;       
}

_int32 kad_on_query( _u32 ip, _u16 port, char *p_buffer, _u32 data_len, _u8 opcode )
{
    _int32 ret_val = SUCCESS;
	switch( opcode )
	{
    	case KADEMLIA_BOOTSTRAP_REQ:
    		ret_val = kad_on_query_boot( ip, port, p_buffer, data_len );
            break;    
   	    case KADEMLIA2_BOOTSTRAP_REQ:
    		ret_val = kad_on_query_boot2( ip, port, p_buffer, data_len );
            break;    
    	case KADEMLIA_HELLO_REQ:
    		ret_val = kad_on_query_hello( ip, port, p_buffer, data_len );
            break;    
    	case KADEMLIA2_HELLO_REQ:
    		ret_val = kad_on_query_hello2( ip, port, p_buffer, data_len );
            break;    
    	case KADEMLIA_REQ:
    		ret_val = kad_on_query_find_node( ip, port, p_buffer, data_len, FALSE );
            break;    
    	case KADEMLIA2_REQ:
    		ret_val = kad_on_query_find_node( ip, port, p_buffer, data_len, TRUE );
            break;    
    	case KADEMLIA_SEARCH_REQ:
    		ret_val = kad_on_query_find_source( ip, port, p_buffer, data_len, FALSE );
            break;    
    	case KADEMLIA2_SEARCH_SOURCE_REQ:
    		ret_val = kad_on_query_find_source( ip, port, p_buffer, data_len, TRUE );
            break;    
    	default:
		    break;    
	}
    if( ret_val != SUCCESS )
    {
        LOG_ERROR( "kad_on_query err_protocal:opcode:%d", opcode );
    }
	return SUCCESS;       
}

_int32 kad_on_query_boot( _u32 ip, _u16 port, char *p_buffer, _u32 data_len )
{
    _int32 ret_val = SUCCESS;
    KAD_NODE *p_kad_node = NULL;
    _u16 tcp_port = 0, upd_port = 0;
    _u8 version = 0;
    _u32 packet_ip = 0;
    _u8 node_id[KAD_ID_LEN];
    
	LOG_DEBUG( "kad_on_query_boot" );

    ret_val = kad_get_bytes( &p_buffer, (_int32 *)&data_len, (char *)node_id, KAD_ID_LEN );
    if( ret_val != SUCCESS ) return ret_val;
    
    ret_val = sd_get_int32_from_lt( &p_buffer, (_int32 *)&data_len, (_int32 *)&packet_ip );
    if( ret_val != SUCCESS ) return ret_val;

    ret_val = sd_get_int16_from_lt( &p_buffer, (_int32 *)&data_len, (_int16 *)&upd_port );
    if( ret_val != SUCCESS ) return ret_val;

    ret_val = sd_get_int16_from_lt( &p_buffer, (_int32 *)&data_len, (_int16 *)&tcp_port );
    if( ret_val != SUCCESS ) return ret_val;

    ret_val = sd_get_int8( &p_buffer, (_int32 *)&data_len, (_int8 *)&version );
    if( ret_val != SUCCESS ) return ret_val;
    
	ret_val = kad_node_create( node_id, KAD_ID_LEN, ip, port, tcp_port, version, &p_kad_node );
	CHECK_VALUE( ret_val );
    
    LOG_DEBUG( "kad_on_query_boot, rt_add_rout_node_from_packet:0x%x", p_kad_node );
	ret_val = rt_add_rout_node( rt_ptr(KAD_TYPE), (K_NODE *)p_kad_node );
	if( ret_val != SUCCESS )
	{
		kad_node_destory( (K_NODE *)p_kad_node );
	}	
	
	return SUCCESS;	
}

_int32 kad_on_query_boot2( _u32 ip, _u16 port, char *p_buffer, _u32 data_len )
{
    return SUCCESS;
}

_int32 kad_on_query_hello( _u32 ip, _u16 port, char *p_buffer, _u32 data_len )
{
	_u8 protocol = OP_KADEMLIAHEADER;
	_u8 opcode = KADEMLIA2_HELLO_RES;
    
	LOG_DEBUG( "kad_on_query_hello" );
  
	return kad_on_query_hello_common( ip, port, protocol, opcode, FALSE );    
}

_int32 kad_on_query_hello2( _u32 ip, _u16 port, char *p_buffer, _u32 data_len )
{
    _int32 ret_val = SUCCESS;
    KAD_NODE *p_kad_node = NULL;
    _u16 tcp_port = 0;
    _u8 version = 0;
    _u8 node_id[KAD_ID_LEN];
    _u8 protocol = OP_KADEMLIAHEADER;
    _u8 opcode = KADEMLIA2_HELLO_RES;
    
	LOG_DEBUG( "kad_on_query_hello2" );
    
    ret_val = kad_get_bytes( &p_buffer, (_int32 *)&data_len, (char *)node_id, KAD_ID_LEN );
    if( ret_val != SUCCESS ) return ret_val;

    ret_val = sd_get_int16_from_lt( &p_buffer, (_int32 *)&data_len, (_int16 *)&tcp_port );
    if( ret_val != SUCCESS ) return ret_val;

    ret_val = sd_get_int8( &p_buffer, (_int32 *)&data_len, (_int8 *)&version );
    if( ret_val != SUCCESS ) return ret_val;

	ret_val = kad_node_create( node_id, KAD_ID_LEN, ip, port, tcp_port, version, &p_kad_node );
	CHECK_VALUE( ret_val );
    
    LOG_DEBUG( "kad_on_query_hello2, rt_add_rout_node_from_packet:0x%x", p_kad_node );
	ret_val = rt_add_rout_node( rt_ptr(KAD_TYPE), (K_NODE *)p_kad_node );
	if( ret_val != SUCCESS )
	{
		kad_node_destory( (K_NODE *)p_kad_node );
	}	
    
	return kad_on_query_hello_common( ip, port, protocol, opcode, TRUE );    
        
}

_int32 kad_on_query_hello_common( _u32 ip, _u16 port, _u8 protocol, _u8 opcode, BOOL is_kad2 )
{
    _int32 ret_val = SUCCESS;

    char packet_buffer[DK_PACKET_MAX_LEN];
    char *p_send_buffer = packet_buffer;
    _u32 buffer_len = DK_PACKET_MAX_LEN;
    
	LOG_DEBUG( "kad_on_query_hello_common" );
    ret_val = sd_set_int8( &p_send_buffer, (_int32 *)&buffer_len, (_int8)protocol );
    CHECK_VALUE( ret_val ); 
    
    ret_val = sd_set_int8( &p_send_buffer, (_int32 *)&buffer_len, (_int8)opcode );
    CHECK_VALUE( ret_val );  

    ret_val = kad_fill_my_detail( &p_send_buffer, &buffer_len, is_kad2 );
    CHECK_VALUE( ret_val );  

    sd_assert( buffer_len < DK_PACKET_MAX_LEN );

    ret_val = kad_send_data( ip, port, packet_buffer, DK_PACKET_MAX_LEN - buffer_len );
    CHECK_VALUE( ret_val ); 

	return SUCCESS;        
}

_int32 kad_on_query_find_node( _u32 ip, _u16 port, char *p_buffer, _u32 data_len, BOOL is_kad2 )
{
    _int32 ret_val = SUCCESS;
	_u8 type = 0;
	_u8 target[KAD_ID_LEN];
	_u8 check[KAD_ID_LEN];
    K_NODE_ID *p_k_node_id = NULL;
    K_NODE_ID *p_k_target_node_id = NULL;
    _u8 *p_id_buffer = NULL;
    _u32 id_len = 0;
    LIST node_list;
    _u32 max_nod_num = 0;
    _u8 find_node_num = 0;
	_u8 protocol = 0;
	_u8 opcode = 0; 
    char packet_buffer[DK_PACKET_MAX_LEN];
    _u32 buffer_len = DK_PACKET_MAX_LEN;
    char *p_send_buffer = packet_buffer;
    LIST_ITERATOR cur_node = NULL;
    KAD_NODE *p_kad_node = NULL;
    _u32 node_ip = 0;
    
	LOG_DEBUG( "kad_on_query_find_node, is_kad2:%d", is_kad2 );
    
    ret_val = sd_get_int8( &p_buffer, (_int32 *)&data_len, (_int8 *)&type );
    if( ret_val != SUCCESS ) return ret_val;

    ret_val = kad_get_bytes( &p_buffer, (_int32 *)&data_len, (char *)target, KAD_ID_LEN );
    if( ret_val != SUCCESS ) return ret_val;

    ret_val = kad_get_bytes( &p_buffer, (_int32 *)&data_len, (char *)check, KAD_ID_LEN );
    if( ret_val != SUCCESS ) return ret_val;
    
	type = type & 0x1F;
	if ( type == 0 || type > kad_bucket_k() )
    {
        LOG_ERROR( "kad_on_query_find_node, KAD_PROTOCAL_ERR" );
        return KAD_PROTOCAL_ERR;
    }

    p_k_node_id = rt_get_local_id( rt_ptr(KAD_TYPE) );
    p_id_buffer = k_distance_get_bit_buffer( p_k_node_id );
    id_len = k_distance_buffer_len( p_k_node_id );
    if( id_len != KAD_ID_LEN )
    {
        sd_assert( FALSE );
        return DK_LOGIC_ERR;
    }

    if( !sd_data_cmp( p_id_buffer, check, id_len ) )
    {
		return KAD_PROTOCAL_ERR;
    }

    protocol = OP_KADEMLIAHEADER;
    if( !is_kad2 )
    {
        opcode = KADEMLIA2_RES;        
    }
    else
    {
        opcode = KADEMLIA2_RES;       
    }

    ret_val = sd_set_int8( &p_send_buffer, (_int32 *)&buffer_len, (_int8)protocol );
    CHECK_VALUE( ret_val ); 

    ret_val = sd_set_int8( &p_send_buffer, (_int32 *)&buffer_len, (_int8)opcode );
    CHECK_VALUE( ret_val ); 

    list_init( &node_list );
    ret_val = k_distance_create_with_char_buffer( target, KAD_ID_LEN, &p_k_target_node_id );
    CHECK_VALUE( ret_val ); 
    
    max_nod_num = type;
    ret_val = rt_get_nearest_nodes( rt_ptr(KAD_TYPE), p_k_target_node_id, &node_list, (_u32 *)&max_nod_num );
    k_distance_destory( &p_k_target_node_id );
    CHECK_VALUE( ret_val ); 

    ret_val = kad_set_bytes( &p_send_buffer, (_int32 *)&buffer_len, (char *)target, KAD_ID_LEN );
    CHECK_VALUE( ret_val ); 

    if( max_nod_num > type )
    {
        CHECK_VALUE( DK_LOGIC_ERR ); 
    }

    find_node_num = type - max_nod_num;
    ret_val = sd_set_int8( &p_send_buffer, (_int32 *)&buffer_len, (_int8)find_node_num );
    CHECK_VALUE( ret_val ); 

    sd_assert( find_node_num == list_size(&node_list) );

    cur_node = LIST_BEGIN( node_list );
    while( cur_node != LIST_END(node_list) )
    {
        p_kad_node = (KAD_NODE *)LIST_VALUE( cur_node );

        p_id_buffer = k_distance_get_bit_buffer( &p_kad_node->_k_node._node_id );
        ret_val = kad_set_bytes( &p_send_buffer, (_int32 *)&buffer_len, (char *)p_id_buffer, KAD_ID_LEN );
        CHECK_VALUE( ret_val ); 

        ret_val = kad_set_bytes( &p_send_buffer, (_int32 *)&buffer_len, (char *)p_id_buffer, KAD_ID_LEN );
        CHECK_VALUE( ret_val ); 

        node_ip = p_kad_node->_k_node._peer_addr._ip;
        node_ip = sd_ntohl( node_ip );
        
        ret_val = sd_set_int32_to_lt( &p_send_buffer, (_int32 *)&buffer_len, node_ip );
        CHECK_VALUE( ret_val ); 
        
        ret_val = sd_set_int16_to_lt( &p_send_buffer, (_int32 *)&buffer_len, p_kad_node->_k_node._peer_addr._port );
        CHECK_VALUE( ret_val );  

        ret_val = sd_set_int16_to_lt( &p_send_buffer, (_int32 *)&buffer_len, p_kad_node->_tcp_port );
        CHECK_VALUE( ret_val );  

        ret_val = sd_set_int16_to_lt( &p_send_buffer, (_int32 *)&buffer_len, p_kad_node->_version );
        CHECK_VALUE( ret_val );     
        
        cur_node = LIST_NEXT( cur_node );
    }
    
    list_clear( &node_list );
    
    sd_assert( buffer_len < DK_PACKET_MAX_LEN );

    ret_val = kad_send_data( ip, port, packet_buffer, DK_PACKET_MAX_LEN - buffer_len );
    CHECK_VALUE( ret_val ); 

    return SUCCESS;     
}

_int32 kad_on_query_find_source( _u32 ip, _u16 port, char *p_buffer, _u32 data_len, BOOL is_kad2 )
{
	LOG_DEBUG( "kad_on_query_find_source" );
    return SUCCESS;        
}

_int32 kad_on_response( _u32 ip, _u16 port, char *p_buffer, _u32 data_len, _u8 opcode )
{
    _int32 ret_val = SUCCESS;
	switch(opcode)
	{
    	case KADEMLIA_BOOTSTRAP_RES:
    		ret_val = kad_on_response_boot( ip, port, p_buffer, data_len );
    		break;
    	case KADEMLIA2_BOOTSTRAP_RES:
    		ret_val = kad_on_response_boot2( ip, port, p_buffer, data_len );
    		break;
    	case KADEMLIA_HELLO_RES:
    		ret_val = kad_on_response_hello( ip, port, p_buffer, data_len );
    		break;
    	case KADEMLIA2_HELLO_RES:
    		ret_val = kad_on_response_hello2( ip, port, p_buffer, data_len );
    		break;
    	case KADEMLIA_RES:
    		ret_val = kad_on_response_find_node( ip, port, p_buffer, data_len, FALSE );
    		break;
    	case KADEMLIA2_RES:
    		ret_val = kad_on_response_find_node( ip, port, p_buffer, data_len, TRUE );
    		break;
    	case KADEMLIA_SEARCH_RES:
    		ret_val = kad_on_response_find_source( ip, port, p_buffer, data_len, FALSE );
    		break;
    	case KADEMLIA2_SEARCH_RES:
    		ret_val = kad_on_response_find_source( ip, port, p_buffer, data_len, TRUE );
    		break;
    	case KADEMLIA_FINDBUDDY_RES:
    		//return on_response_find_buddy( ip, port, p_buffer, data_len );
    		break;
        case KADEMLIA_PUBLISH_RES:
    		ret_val = kad_on_response_publish( ip, port, p_buffer, data_len );
    		break;
    	default:
    		break;
	}
    if( ret_val != SUCCESS )
    {
        LOG_ERROR( "kad_on_response err_protocal:opcode:%d", opcode );
    }
    
    return SUCCESS;        
}

_int32 kad_on_response_boot( _u32 ip, _u16 port, char *p_buffer, _u32 data_len )
{
    _int32 ret_val = SUCCESS;
	_u16 contact_num = 0;
    _u32 contact_index = 0;
    KAD_NODE *p_kad_node = NULL;
    
#ifdef _LOGGER
    char addr[24] = {0};
    ret_val = sd_inet_ntoa(ip, addr, 24);
    sd_assert( ret_val == SUCCESS ); 
    
#endif

    ret_val = sd_get_int16_from_lt( &p_buffer, (_int32 *)&data_len, (_int16 *)&contact_num );
    if( ret_val != SUCCESS ) return ret_val;
    
#ifdef _LOGGER
    LOG_DEBUG( "kad_on_response_boot, ip:%s, port:%d, contact_num:%u", addr, port, contact_num );
#endif
    
    for( ; contact_index < contact_num; contact_index++ )
    {
        ret_val = kad_parser_contact_node( &p_buffer, &data_len, &p_kad_node );
        if( ret_val != SUCCESS ) return ret_val;
        
        ret_val = rt_ping_node( KAD_TYPE, p_kad_node->_k_node._peer_addr._ip,
            p_kad_node->_k_node._peer_addr._port, p_kad_node->_version, FALSE );
        LOG_DEBUG( "kad_on_response_boot, rt_add_rout_node_from_packet:0x%x", p_kad_node );
    	ret_val = rt_add_rout_node( rt_ptr(KAD_TYPE), (K_NODE *)p_kad_node );
    	if( ret_val != SUCCESS )
    	{
    		kad_node_destory( (K_NODE *)p_kad_node );
    	}	
    }
    //sd_assert( data_len == 0 );
    
    return SUCCESS;
}

_int32 kad_on_response_boot2( _u32 ip, _u16 port, char *p_buffer, _u32 data_len )
{
    _int32 ret_val = SUCCESS;
    KAD_NODE *p_kad_node = NULL;
    _u16 tcp_port = 0;
    _u8 version = 0;
    _u8 node_id[KAD_ID_LEN];
	_u16 contact_num = 0;
    _u32 contact_index = 0;
    
#ifdef _LOGGER
    char addr[24] = {0};
    ret_val = sd_inet_ntoa(ip, addr, 24);
    sd_assert( ret_val == SUCCESS ); 
#endif

    ret_val = kad_get_bytes( &p_buffer, (_int32 *)&data_len, (char *)node_id, KAD_ID_LEN );
    if( ret_val != SUCCESS ) return ret_val;

    ret_val = sd_get_int16_from_lt( &p_buffer, (_int32 *)&data_len, (_int16 *)&tcp_port );
    if( ret_val != SUCCESS ) return ret_val;

    ret_val = sd_get_int8( &p_buffer, (_int32 *)&data_len, (_int8 *)&version );
    if( ret_val != SUCCESS ) return ret_val;

    ret_val = sd_get_int16_from_lt( &p_buffer, (_int32 *)&data_len, (_int16 *)&contact_num );
    if( ret_val != SUCCESS ) return ret_val;

	ret_val = kad_node_create( node_id, KAD_ID_LEN, ip, port, tcp_port, version, &p_kad_node );
	CHECK_VALUE( ret_val );
    
    LOG_DEBUG( "kad_on_response_boot2, rt_add_rout_node_from_packet:0x%x", p_kad_node );
	ret_val = rt_add_rout_node( rt_ptr(KAD_TYPE), (K_NODE *)p_kad_node );
	if( ret_val != SUCCESS )
	{
		kad_node_destory( (K_NODE *)p_kad_node );
	}	    
    
#ifdef _LOGGER
    LOG_DEBUG( "kad_on_response_boot2, ip:%s, port:%d, contact_num:%u", addr, port, contact_num );
#endif    
    
    for( ; contact_index < contact_num; contact_index++ )
    {
        ret_val = kad_parser_contact_node( &p_buffer, &data_len, &p_kad_node );
        if( ret_val != SUCCESS ) return ret_val;
        
        ret_val = rt_ping_node( KAD_TYPE, p_kad_node->_k_node._peer_addr._ip,
            p_kad_node->_k_node._peer_addr._port, p_kad_node->_version, FALSE );

        LOG_DEBUG( "kad_on_response_boot2, rt_add_rout_node_from_packet:0x%x", p_kad_node );
    	ret_val = rt_add_rout_node( rt_ptr(KAD_TYPE), (K_NODE *)p_kad_node );
    	if( ret_val != SUCCESS )
    	{
    		kad_node_destory( (K_NODE *)p_kad_node );
    	}	
    }
    //sd_assert( data_len == 0 );
    return SUCCESS;
}

_int32 kad_parser_contact_node( char **pp_buffer, _u32 *p_data_len, KAD_NODE **pp_kad_node )
{
    _int32 ret_val = SUCCESS;
    _u16 tcp_port = 0;
    _u8 version = 0;
    _u8 node_id[KAD_ID_LEN];
    _u32 ip = 0;
    _u16 kad_port = 0;
    
	LOG_DEBUG( "kad_parser_contact_node" );
    
    ret_val = kad_get_bytes( pp_buffer, (_int32 *)p_data_len, (char *)node_id, KAD_ID_LEN );
    if( ret_val != SUCCESS ) return ret_val;
    
    ret_val = sd_get_int32_from_lt( pp_buffer, (_int32 *)p_data_len, (_int32 *)&ip );
    if( ret_val != SUCCESS ) return ret_val;
    ip = sd_htonl( ip );
    
    ret_val = sd_get_int16_from_lt( pp_buffer, (_int32 *)p_data_len, (_int16 *)&kad_port );
    if( ret_val != SUCCESS ) return ret_val;

    ret_val = sd_get_int16_from_lt( pp_buffer, (_int32 *)p_data_len, (_int16 *)&tcp_port );
    if( ret_val != SUCCESS ) return ret_val;

    ret_val = sd_get_int8( pp_buffer, (_int32 *)p_data_len, (_int8 *)&version );
    if( ret_val != SUCCESS ) return ret_val;

	ret_val = kad_node_create( node_id, KAD_ID_LEN, ip, kad_port, tcp_port, version, pp_kad_node );
	CHECK_VALUE( ret_val );
	
    return SUCCESS;
}

_int32 kad_on_response_hello( _u32 ip, _u16 port, char *p_buffer, _u32 data_len )
{
    _int32 ret_val = SUCCESS;
    KAD_NODE *p_kad_node = NULL;
    _u16 tcp_port = 0;
    _u8 version = 0;
    _u8 node_id[KAD_ID_LEN];
    _u16 udp_port = 0;
    _u32 packet_ip = 0;
    
#ifdef _LOGGER   
    char test_buffer[1024];
    char addr[24] = {0};
#endif    

	LOG_DEBUG( "kad_on_response_hello" );
    
    ret_val = kad_get_bytes( &p_buffer, (_int32 *)&data_len, (char *)node_id, KAD_ID_LEN );
    if( ret_val != SUCCESS ) return ret_val;
    
    ret_val = sd_get_int32_from_lt( &p_buffer, (_int32 *)&data_len, (_int32 *)&packet_ip );
    if( ret_val != SUCCESS ) return ret_val;
    
    ret_val = sd_get_int16_from_lt( &p_buffer, (_int32 *)&data_len, (_int16 *)&udp_port );
    if( ret_val != SUCCESS ) return ret_val;

    ret_val = sd_get_int16_from_lt( &p_buffer, (_int32 *)&data_len, (_int16 *)&tcp_port );
    if( ret_val != SUCCESS ) return ret_val;

    ret_val = sd_get_int8( &p_buffer, (_int32 *)&data_len, (_int8 *)&version );
    if( ret_val != SUCCESS ) return ret_val;

#ifdef _LOGGER   
    sd_inet_ntoa(ip, addr, 24);
    sd_memset(test_buffer,0,1024);
    str2hex( (char *)node_id, KAD_ID_LEN, test_buffer, 1024);
    LOG_DEBUG( "kad_on_response_hello, ip:%s, port:%d, node_id:%s, version:%d", 
        addr, port, test_buffer, version );
#endif   

	ret_val = kad_node_create( node_id, KAD_ID_LEN, ip, port, tcp_port, version, &p_kad_node );
	CHECK_VALUE( ret_val );

    LOG_DEBUG( "kad_on_response_hello, rt_add_rout_node_from_packet:0x%x", p_kad_node );
    sd_assert( data_len == 0 );
    
	ret_val = rt_add_rout_node( rt_ptr(KAD_TYPE), (K_NODE *)p_kad_node );
	if( ret_val != SUCCESS )
	{
		kad_node_destory( (K_NODE *)p_kad_node );
        return ret_val;
	}	    

    return SUCCESS;
}

_int32 kad_on_response_hello2( _u32 ip, _u16 port, char *p_buffer, _u32 data_len )
{
     _int32 ret_val = SUCCESS;
     KAD_NODE *p_kad_node = NULL;
     _u16 tcp_port = 0;
     _u8 version = 0;
     _u8 node_id[KAD_ID_LEN];
     _u8 tag_count = 0;
     
     LOG_DEBUG( "kad_on_response_hello2" );
     
     ret_val = kad_get_bytes( &p_buffer, (_int32 *)&data_len, (char *)node_id, KAD_ID_LEN );
     if( ret_val != SUCCESS ) return ret_val;

     ret_val = sd_get_int16_from_lt( &p_buffer, (_int32 *)&data_len, (_int16 *)&tcp_port );
     if( ret_val != SUCCESS ) return ret_val;
     
     ret_val = sd_get_int8( &p_buffer, (_int32 *)&data_len, (_int8 *)&version );
     if( ret_val != SUCCESS ) return ret_val;
     
     ret_val = sd_get_int8( &p_buffer, (_int32 *)&data_len, (_int8 *)&tag_count );
     if( ret_val != SUCCESS ) return ret_val;
     
     ret_val = kad_node_create( node_id, KAD_ID_LEN, ip, port, tcp_port, version, &p_kad_node );
     CHECK_VALUE( ret_val );
     
     LOG_DEBUG( "kad_on_response_hello2, rt_add_rout_node_from_packet:0x%x", p_kad_node );

     ret_val = rt_add_rout_node( rt_ptr(KAD_TYPE), (K_NODE *)p_kad_node );
     if( ret_val != SUCCESS )
     {
         kad_node_destory( (K_NODE *)p_kad_node );
         return ret_val;
     }       
 
    return SUCCESS;
}

_int32 kad_on_response_find_node( _u32 ip, _u16 port, char *p_buffer, _u32 data_len, BOOL is_kad2 )
{
    _int32 ret_val = SUCCESS;
	_u8 contact_num = 0;
    _u8 contact_index = 0;
    _u8 target[KAD_ID_LEN];
	DK_PROTOCAL_HANDLER *p_protocal_handler = NULL;
    KAD_NODE *p_kad_node = NULL;
    LIST node_list;
    LIST_ITERATOR cur_node = NULL;
    _u8 version = 0;
    _u32 key = 0;
#ifdef _LOGGER   
    char test_buffer[1024];
    char addr[24] = {0};
#endif

    ret_val = kad_get_bytes( &p_buffer, (_int32 *)&data_len, (char *)target, KAD_ID_LEN );
    if( ret_val != SUCCESS ) return ret_val;

    ret_val = sd_get_int8( &p_buffer, (_int32 *)&data_len, (_int8 *)&contact_num );
    if( ret_val != SUCCESS ) return ret_val;

    list_init( &node_list );
    
#ifdef _LOGGER   
    sd_inet_ntoa(ip, addr, 24);
    sd_memset(test_buffer,0,1024);
    str2hex( (char *)target, KAD_ID_LEN, test_buffer, 1024);
    LOG_DEBUG( "kad_on_response_find_node, ip:%s, port:%d, target:%s, contact_num:%d, is_kad2:%d", 
        addr, port, test_buffer, contact_num, is_kad2 );
#endif    

    for( ; contact_index < contact_num; contact_index++ )
    {
        ret_val = kad_parser_contact_node( &p_buffer, &data_len, &p_kad_node );
        if( ret_val != SUCCESS ) return ret_val;

        list_push( &node_list, p_kad_node );
    }
    //sd_assert( data_len == 0 );
    key = dk_calc_key( KAD_ID_LEN, KAD_ID_LEN, target, KAD_ID_LEN );
    
	ret_val = sh_get_resp_handler( sh_ptr(KAD_TYPE), key, &p_protocal_handler );
	if( ret_val != SUCCESS ) return ret_val;    

    if( is_kad2 )
    {
        version = KADEMLIA_VERSION2_47a;
    }
	if( p_protocal_handler != NULL && p_protocal_handler->_response_handler != NULL )
	{
		ret_val = p_protocal_handler->_response_handler( p_protocal_handler, ip, port, version, &node_list );
		sd_assert( ret_val == SUCCESS );
	}	

    cur_node = LIST_BEGIN( node_list );
    while( cur_node != LIST_END(node_list) )
    {
        p_kad_node = ( KAD_NODE * )LIST_VALUE( cur_node );
        
        ret_val = rt_ping_node( KAD_TYPE, p_kad_node->_k_node._peer_addr._ip,
            p_kad_node->_k_node._peer_addr._port, p_kad_node->_version, FALSE );

        CHECK_VALUE( ret_val ); 
        LOG_DEBUG( "kad_on_response_find_node, rt_add_rout_node_from_packet:0x%x", p_kad_node );
        ret_val = rt_add_rout_node( rt_ptr(KAD_TYPE), (K_NODE *)p_kad_node );
        if( ret_val != SUCCESS )
        {
            kad_node_destory( (K_NODE *)p_kad_node );
        } 
        cur_node = LIST_NEXT( cur_node );
    }
    list_clear( &node_list );
    return SUCCESS;
}

_int32 kad_on_response_find_source( _u32 ip, _u16 port, char *p_buffer, _u32 data_len, BOOL is_kad2 )
{
    _int32 ret_val = SUCCESS;
    _u8 target[KAD_ID_LEN];
	_u16 source_num = 0;
	_u16 source_index = 0;
    _u8 tag_count = 0;
    _u8 tag_index = 0;
    _u8 node_id[KAD_ID_LEN];
	DK_PROTOCAL_HANDLER *p_protocal_handler = NULL;
    EMULE_TAG_LIST tag_list;
    EMULE_TAG *p_tag = NULL;
    _u32 key = 0;
    _u8 sender_id[KAD_ID_LEN];
    
#ifdef _PACKET_DEBUG
      char test_buffer[1024];
#endif     

#ifdef _LOGGER  
    char addr[24] = {0};

    sd_inet_ntoa(ip, addr, 24);
    LOG_DEBUG( "kad_on_response_find_source, ip:%s, port:%d, is_kad2:%d", 
        addr, port, is_kad2 );
#endif    

    if( is_kad2 )
    {
        ret_val = kad_get_bytes( &p_buffer, (_int32 *)&data_len, (char *)sender_id, KAD_ID_LEN );
        CHECK_VALUE( ret_val );         
#ifdef _PACKET_DEBUG
        sd_memset(test_buffer,0,1024);
        str2hex( (char *)sender_id, KAD_ID_LEN, test_buffer, 1024);
        LOG_DEBUG( "kad_on_response_find_source:*sender_id[%u]=%s" ,
            KAD_ID_LEN, test_buffer );
#endif  
    }
    
    ret_val = kad_get_bytes( &p_buffer, (_int32 *)&data_len, (char *)target, KAD_ID_LEN );
    if( ret_val != SUCCESS ) return ret_val;
    
    ret_val = sd_get_int16_from_lt( &p_buffer, (_int32 *)&data_len, (_int16 *)&source_num );
    if( ret_val != SUCCESS ) return ret_val;
    
#ifdef _PACKET_DEBUG
        sd_memset(test_buffer,0,1024);
        str2hex( (char *)target, KAD_ID_LEN, test_buffer, 1024);
        LOG_DEBUG( "kad_on_response_find_source:*taget[%u]=%s, source_num:%d" ,
            KAD_ID_LEN, test_buffer, source_num );
#endif    
    
    key = dk_calc_key( DK_PACKET_MAX_LEN, DK_PACKET_MAX_LEN, target, KAD_ID_LEN );

	ret_val = sh_get_resp_handler( sh_ptr(KAD_TYPE), (_u32)key, &p_protocal_handler );
	if( ret_val != SUCCESS ) return ret_val;  
	if( p_protocal_handler == NULL || p_protocal_handler->_response_handler == NULL )
	{
        LOG_ERROR( "kad_on_response_find_source, DK_LOGIC_ERR" );
        return SUCCESS;
	}
     
    for( ; source_index < source_num; source_index++ )
    {
        tag_index = 0;
        emule_tag_list_init( &tag_list );
        
        ret_val = kad_get_bytes( &p_buffer, (_int32 *)&data_len, (char *)node_id, KAD_ID_LEN );
        if( ret_val != SUCCESS ) return ret_val;
        
        ret_val = sd_get_int8( &p_buffer, (_int32 *)&data_len, (_int8 *)&tag_count );
        if( ret_val != SUCCESS ) return ret_val;
        
#ifdef _PACKET_DEBUG
        sd_memset(test_buffer,0,1024);
        str2hex( (char *)node_id, KAD_ID_LEN, test_buffer, 1024);
        LOG_DEBUG( "kad_on_response_find_source:*node_id[%u]=%s, tag_count:%d" ,
            KAD_ID_LEN, test_buffer, tag_count );
#endif     

        for( ; tag_index < tag_count; tag_index++ )
        {
            ret_val = emule_tag_from_buffer( &p_tag, &p_buffer, (_int32 *)&data_len );
            if( ret_val != SUCCESS ) 
            {
                LOG_ERROR( "kad_on_response_find_source, emule_tag_ERR, source_index:%d", source_index );
                emule_tag_list_uninit( &tag_list, TRUE );
                return SUCCESS;
            }

            ret_val = emule_tag_list_add( &tag_list, p_tag );
            if( ret_val != SUCCESS )
            {
                emule_destroy_tag(p_tag);
            }
        }      
		ret_val = p_protocal_handler->_response_handler( p_protocal_handler, ip, port, (_u32)&node_id, &tag_list );
		sd_assert( ret_val == SUCCESS );

        emule_tag_list_uninit( &tag_list, TRUE );
    }
    
#ifdef _PACKET_DEBUG
    if( data_len != 0 )
    {
        sd_memset(test_buffer,0,1024);
        str2hex( (char *)p_buffer, data_len, test_buffer, 1024);
        LOG_DEBUG( "kad_on_response_find_source err data_len!!!:*node_id[%u]=%s" ,
            data_len, test_buffer );        
    }

#endif      
    return SUCCESS;
}

_int32 kad_on_response_publish( _u32 ip, _u16 port, char *p_buffer, _u32 data_len )
{
#ifdef _LOGGER
    char addr[24] = {0};
    sd_inet_ntoa(ip, addr, 24);

    LOG_DEBUG( "kad_on_response_publish: host_ip = %s, port = %u, data_len:%u", addr, port, data_len );
#endif   
    return SUCCESS;
}

_int32 kad_fill_my_detail( char **pp_buffer, _u32 *p_buffer_len, BOOL is_kad2 )
{
    _int32 ret_val = SUCCESS;
    K_NODE_ID *p_k_node_id = NULL;
    _u8 *p_id_buffer = NULL;
    _u32 id_buffer_len = 0;
    _u32 ip = 0;
    _u16 tcp_port = 22;//未完成
    _u16 udp_port = kad_udp_port();
    _u8 version = 0;
    _u8 tag_count = 2;
    EMULE_TAG *p_user_count_tag = NULL;
    EMULE_TAG *p_file_count_tag = NULL;
    char user_count[2] =  { FT_USER_COUNT, 0 };
    char file_count[2] =  { FT_FILE_COUNT, 0 };
    
    LOG_DEBUG( "kad_fill_my_detail, is_kad2:%d", is_kad2 );
    
    p_k_node_id = rt_get_local_id( rt_ptr(KAD_TYPE) );
    p_id_buffer = k_distance_get_bit_buffer( p_k_node_id );
    id_buffer_len = k_distance_buffer_len( p_k_node_id );
    
    ret_val = kad_set_bytes( pp_buffer, (_int32 *)p_buffer_len, (char *)p_id_buffer, id_buffer_len );
    CHECK_VALUE( ret_val );   
    
	if( is_kad2 )
	{
		version  = KADEMLIA_VERSION;
        
        ret_val = sd_set_int16_to_lt( pp_buffer, (_int32 *)p_buffer_len, tcp_port );
        CHECK_VALUE( ret_val );   
        
        ret_val = sd_set_int8( pp_buffer, (_int32 *)p_buffer_len, (_int8)version );
        CHECK_VALUE( ret_val );    

        ret_val = sd_set_int8( pp_buffer, (_int32 *)p_buffer_len, (_int8)tag_count );
        CHECK_VALUE( ret_val );  

        ret_val = emule_create_u32_tag( &p_user_count_tag, user_count, 0 );
        CHECK_VALUE( ret_val );
        
        ret_val = emule_create_u32_tag( &p_file_count_tag, file_count, 0 );
        CHECK_VALUE( ret_val ); 

        ret_val = emule_tag_to_buffer( p_user_count_tag, pp_buffer, (_int32 *)p_buffer_len );
        CHECK_VALUE( ret_val ); 
        
        ret_val = emule_tag_to_buffer( p_file_count_tag, pp_buffer, (_int32 *)p_buffer_len );
        CHECK_VALUE( ret_val );   
        
        emule_free_emule_tag_slip(p_user_count_tag);
        emule_free_emule_tag_slip(p_file_count_tag);
        
		//pack << local_id << tcp_port << version << tag_count << user_count << file_count;
	}
	else
	{
        version = 0;
        ip = sd_get_local_ip();
        ip = sd_htonl( ip );
        
        ret_val = sd_set_int32_to_lt( pp_buffer, (_int32 *)p_buffer_len, (_int32)ip );
        CHECK_VALUE( ret_val ); 
        
        ret_val = sd_set_int16_to_lt( pp_buffer, (_int32 *)p_buffer_len, (_int16)udp_port );
        CHECK_VALUE( ret_val );   
        
        ret_val = sd_set_int16_to_lt( pp_buffer, (_int32 *)p_buffer_len, (_int16)tcp_port );
        CHECK_VALUE( ret_val ); 

        ret_val = sd_set_int8( pp_buffer, (_int32 *)p_buffer_len, (_int8)version );
        CHECK_VALUE( ret_val );    
        
		//pack << local_id << ip << udp_port << tcp_port << version;
	}

    return SUCCESS;
}

_int32 kad_send_data( _u32 ip, _u16 port, const char *p_data, _u32 data_len )
{
    _int32 ret_val = SUCCESS;
    char *p_send_buffer = NULL;
    
    LOG_DEBUG( "kad_send_data" );
    
    ret_val = sd_malloc( data_len, (void**)&p_send_buffer );
    CHECK_VALUE( ret_val );  

    sd_memcpy( p_send_buffer, p_data, data_len );
        
    ret_val = sh_send_packet( sh_ptr(KAD_TYPE), ip, port, p_send_buffer, data_len, NULL, 0 );
    if( ret_val != SUCCESS )
    {
        SAFE_DELETE( p_send_buffer );
    }    
    return SUCCESS;
}

_int32 kad_get_bytes(char **buffer, _int32 *cur_buflen, char *dest_buf, _int32 dest_len)
{
    _int32 ret_val = SUCCESS;
    _int32 *p_tmp_int32 = (_int32 *)dest_buf;
    _u32 index = 0;

    sd_assert( dest_len % 4 == 0 );
    for( ; index < dest_len/4; index++ )
    {
        ret_val = sd_get_int32_from_bg( buffer, cur_buflen, p_tmp_int32 );
        if( ret_val != SUCCESS ) return ret_val;
        p_tmp_int32++;
    }
    return SUCCESS;
}

_int32 kad_set_bytes(char **buffer, _int32 *cur_buflen, char *dest_buf, _int32 dest_len)
{
    _int32 ret_val = SUCCESS;
    _int32 *p_tmp_int32 = (_int32 *)dest_buf;
    _u32 index = 0;

    sd_assert( dest_len % 4 == 0 );
    for( ; index < dest_len/4; index++ )
    {
        ret_val = sd_set_int32_to_bg( buffer, cur_buflen, *p_tmp_int32 );
        CHECK_VALUE( ret_val );       
        p_tmp_int32++;
    }
    return SUCCESS;
}


#endif
#endif

