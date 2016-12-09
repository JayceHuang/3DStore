#include "utility/define.h"
#ifdef _DK_QUERY

#include "ping_handler.h"
#include "dk_socket_handler.h"
#include "dk_setting.h"

#include "utility/logid.h"
#define LOGID LOGID_DK_QUERY
#include "utility/logger.h"

#include "utility/utility.h"

#include "utility/define.h"
#include "utility/mempool.h"


_int32 ping_handler_init( PING_HANDLER *p_ping_handler, _u32 dk_type )
{
	list_init( &p_ping_handler->_wait_to_ping_list );
	LOG_DEBUG( "ping_handler_init, p_ping_handler:0x%x, type:%u", p_ping_handler, dk_type );

	if( dk_type == DHT_TYPE )
	{
		p_ping_handler->_cmd_builder = dht_build_ping_cmd;	
		p_ping_handler->_node_creater = dht_node_info_creater;
		p_ping_handler->_node_destoryer = dht_node_info_destoryer;
	}
#ifdef ENABLE_EMULE
	else if( dk_type == KAD_TYPE )
	{
		p_ping_handler->_cmd_builder = kad_build_ping_cmd;	
		p_ping_handler->_node_creater = kad_node_info_creater;
		p_ping_handler->_node_destoryer = kad_node_info_destoryer;
	}
#endif
    else
	{
		sd_assert( FALSE );
		return -1;
	}
	p_ping_handler->_dk_type = dk_type;

	return SUCCESS;
}

_int32 ping_handler_uninit( PING_HANDLER *p_ping_handler )
{
	NODE_INFO *p_node_info = NULL;
	LIST_ITERATOR cur_node = LIST_BEGIN( p_ping_handler->_wait_to_ping_list );

    LOG_DEBUG( "ping_handler_uninit, p_ping_handler:0x%x, type:%u", p_ping_handler, p_ping_handler->_dk_type );

	if( p_ping_handler->_node_destoryer == NULL )
	{
		sd_assert( FALSE );
		return -1;
	}
	
	while( cur_node != LIST_END( p_ping_handler->_wait_to_ping_list ) )
	{
		p_node_info = LIST_VALUE( cur_node );
		p_ping_handler->_node_destoryer( p_node_info );
		cur_node = LIST_NEXT( cur_node );
	}
	list_clear( &p_ping_handler->_wait_to_ping_list );
	p_ping_handler->_cmd_builder = NULL;
	p_ping_handler->_node_creater = NULL;
	p_ping_handler->_node_destoryer = NULL;

	return SUCCESS;
}

_int32 ph_ping_node( PING_HANDLER *p_ping_handler, _u32 ip, _u16 port, _u8 version )
{
	NODE_INFO *p_node_info = NULL;
	_int32 ret_val = SUCCESS;
	
    LOG_DEBUG( "ph_ping_node, p_ping_handler:0x%x, type:%u", p_ping_handler, p_ping_handler->_dk_type );
	if( p_ping_handler->_node_creater == NULL )
	{
		sd_assert( FALSE );
		return -1;
	}
    if( list_size(&p_ping_handler->_wait_to_ping_list) > dk_max_wait_ping_num() ) return SUCCESS;
	ret_val = p_ping_handler->_node_creater( ip, port, version, NULL, 0, &p_node_info );
	CHECK_VALUE( ret_val );

	ret_val = list_push( &p_ping_handler->_wait_to_ping_list, p_node_info );
	if( ret_val != SUCCESS )
	{
		sd_assert( FALSE );
		p_ping_handler->_node_destoryer( p_node_info );
	}

	return SUCCESS;
}

_int32 ph_ping_dht_boot_host( PING_HANDLER *p_ping_handler )
{	
 	_int32 ret_val = SUCCESS;
	char *p_buffer = NULL;
	_u32 buffer_len = 0;
	char host[MAX_URL_LEN];
	_u64 port = 0;
	char *p_host_str = dht_boot_host();
	_u32 str_index = 0;
	_u32 parser_len = 0;
	_u32 parser_pos = 0;
	DK_SOCKET_HANDLER *p_dk_socket_handler = NULL;
	
    LOG_DEBUG( "ph_ping_dht_boot_host, p_ping_handler:0x%x, type:%u", p_ping_handler, p_ping_handler->_dk_type );
	
	p_dk_socket_handler = sh_ptr( p_ping_handler->_dk_type );
    
	while( p_host_str[str_index] != '\0' && str_index < MAX_URL_LEN )
	{
		parser_len++;
		if( p_host_str[str_index] == ':' )
		{
			sd_memcpy( host, p_host_str + parser_pos, parser_len - 1 );
			host[parser_len-1] = '\0';
			parser_pos += parser_len;
			parser_len = 0;
		}
		else if( p_host_str[str_index] == ';' )
		{
			ret_val = sd_str_to_u64( p_host_str + parser_pos, parser_len - 1, &port );
			CHECK_VALUE( ret_val );
			
			ret_val = p_ping_handler->_cmd_builder( NULL, &p_buffer, &buffer_len );
			if( ret_val != SUCCESS ) return SUCCESS;	
			
			LOG_DEBUG( "ph_ping_dht_boot_host, host:%s, port:%u", host, (_u16)port );
			ret_val = sh_send_packet_by_domain( p_dk_socket_handler, host, (_u16)port, p_buffer, buffer_len );
			if( ret_val != SUCCESS )
			{
				SAFE_DELETE( p_buffer );
				return ret_val;
			}

			parser_pos += parser_len;
			parser_len = 0;
		}
		str_index++;
	}

	return SUCCESS;
}

_int32 ph_update( PING_HANDLER *p_ping_handler )
{
 	_int32 ret_val = SUCCESS;
	NODE_INFO *p_node_info = NULL;
	LIST_ITERATOR cur_node = LIST_BEGIN( p_ping_handler->_wait_to_ping_list );
	LIST_ITERATOR next_node = NULL;
	char *p_buffer = NULL;
	_u32 buffer_len = 0;
	DK_SOCKET_HANDLER *p_dk_socket_handler = NULL;
    
#ifdef _DEBUG
    char addr[24] = {0};
#endif	

    LOG_DEBUG( "ph_update, p_ping_handler:0x%x, type:%u", p_ping_handler, p_ping_handler->_dk_type );

	if( p_ping_handler->_cmd_builder == NULL )
	{
		sd_assert( FALSE );
		return -1;
	}

	p_dk_socket_handler = sh_ptr( p_ping_handler->_dk_type );
	while( cur_node != LIST_END( p_ping_handler->_wait_to_ping_list ) )
	{
		p_node_info = LIST_VALUE( cur_node );
		
		ret_val = p_ping_handler->_cmd_builder( p_node_info, &p_buffer, &buffer_len );
		if( ret_val != SUCCESS ) return SUCCESS;

#ifdef _DEBUG
        sd_inet_ntoa(p_node_info->_peer_addr._ip, addr, 24);
        LOG_DEBUG( "ph_ping_node!!!.p_node_info:0x%x, ip:%s, port:%d", 
            p_node_info, addr, p_node_info->_peer_addr._port );
#endif	

		ret_val = sh_send_packet( p_dk_socket_handler, p_node_info->_peer_addr._ip,
			p_node_info->_peer_addr._port, p_buffer, buffer_len, NULL, 0 );
		if( ret_val != SUCCESS ) 
		{
			SAFE_DELETE( p_buffer );
			return SUCCESS;
		}

		p_ping_handler->_node_destoryer( p_node_info );

		next_node = LIST_NEXT( cur_node );
		list_erase( &p_ping_handler->_wait_to_ping_list, cur_node );
		cur_node = next_node;
	}

	return SUCCESS;
}

#endif

