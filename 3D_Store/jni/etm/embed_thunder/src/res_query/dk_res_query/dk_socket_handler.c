#include "utility/define.h"
#ifdef _DK_QUERY

#include "dk_socket_handler.h"
#include "socket_proxy_interface.h"
#include "utility/utility.h"
#include "dht_recv_packet_handler.h"
#include "dk_setting.h"
#include "kad_packet_handler.h"

#include "utility/logid.h"
#define  LOGID	LOGID_DK_QUERY
#include "utility/logger.h"
#include "utility/mempool.h"

#include "utility/define_const_num.h"

#define REQUEST_ID_LEN 16
#define DK_MAX_RETRY_TIMES 3 
#define DK_MAX_RESP_NUM 5 


static SLAB* g_dk_request_node_slab = NULL;

static DK_SOCKET_HANDLER *g_dk_socket_handler[DK_TYPE_COUNT] = { NULL, NULL };

_int32 sh_create( _u32 dk_type )
{
	_int32 ret_val = SUCCESS;
	SD_SOCKADDR addr;
	DK_SOCKET_HANDLER *p_socket_handler = NULL;
	
	LOG_DEBUG( "sh_create, dk_type = ", dk_type );
	
	if( NULL != g_dk_socket_handler[ dk_type ]
		|| dk_type >= DK_TYPE_COUNT )
	{
		sd_assert( FALSE );
		return -1;
	}
	
	ret_val = sd_malloc( sizeof( DK_SOCKET_HANDLER ), (void **)&p_socket_handler );
	CHECK_VALUE( ret_val );

	ret_val = socket_proxy_create( &p_socket_handler->_udp_socket, SD_SOCK_DGRAM );
	CHECK_VALUE( ret_val );
	LOG_DEBUG("sh_create p_socket_handler->_udp_socket = %u", p_socket_handler->_udp_socket);

	if( dk_type == DHT_TYPE )
	{
		p_socket_handler->_udp_port = dht_udp_port();
		p_socket_handler->_recv_packet_handler = dht_recv_handle_recv_packet;
	}    
#ifdef ENABLE_EMULE
	else if( dk_type == KAD_TYPE )
	{
		p_socket_handler->_udp_port = kad_udp_port(); // 888
		p_socket_handler->_recv_packet_handler = kad_recv_handle_recv_packet;
	}
#endif

	addr._sin_family = SD_AF_INET;
	addr._sin_addr = ANY_ADDRESS;
	addr._sin_port = sd_htons( (_u16)p_socket_handler->_udp_port );
	ret_val = socket_proxy_bind( p_socket_handler->_udp_socket, &addr );
	if( ret_val != SUCCESS )
	{
		LOG_DEBUG("sh_create_create_udp_ bind port failed, port = %u", p_socket_handler->_udp_port);
		socket_proxy_close(p_socket_handler->_udp_socket);	
		SAFE_DELETE( p_socket_handler );
        return ret_val;
	}
	
	map_init( &p_socket_handler->_resp_handler_map, sh_request_id_comparator );
	list_init( &p_socket_handler->_request_list );
	p_socket_handler->_cur_resquest_node_ptr = NULL;
	p_socket_handler->_dk_type = dk_type;

    ret_val = sd_malloc( DK_MAX_PACKET_LEN, (void**)&p_socket_handler->_recv_buffer_ptr );
	CHECK_VALUE( ret_val );
    
	ret_val = socket_proxy_recvfrom( p_socket_handler->_udp_socket, 
		p_socket_handler->_recv_buffer_ptr, DK_MAX_PACKET_LEN, sh_udp_recvfrom_callback, p_socket_handler );
	sd_assert( ret_val == SUCCESS );

	g_dk_socket_handler[ dk_type ] = p_socket_handler;
	
	return SUCCESS;
}

_int32 sh_try_destory( _u32 dk_type )
{
	_u32 count = 0;
	DK_SOCKET_HANDLER *p_dk_socket_handler = sh_ptr( dk_type );
	
	LOG_DEBUG( "sh_try_destory, _udp_socket:%d", p_dk_socket_handler->_udp_socket );
	
	if( p_dk_socket_handler == NULL ) return SUCCESS;
    
	if(p_dk_socket_handler->_udp_socket == INVALID_SOCKET)
		return SUCCESS;
	socket_proxy_peek_op_count( p_dk_socket_handler->_udp_socket, DEVICE_SOCKET_UDP, &count );
    g_dk_socket_handler[ dk_type ] = NULL;
    
    if(count > 0)
	{
		return socket_proxy_cancel( p_dk_socket_handler->_udp_socket, DEVICE_SOCKET_UDP );
	}
	else
	{
        sh_destory( p_dk_socket_handler );  
	}	
	return SUCCESS;
}

_int32 sh_destory( DK_SOCKET_HANDLER *p_dk_socket_handler )
{
    LOG_DEBUG( "sh_destory" );
    socket_proxy_close( p_dk_socket_handler->_udp_socket );
    p_dk_socket_handler->_udp_socket = INVALID_SOCKET; 
    
    map_clear( &p_dk_socket_handler->_resp_handler_map );
    SAFE_DELETE( p_dk_socket_handler->_recv_buffer_ptr );
    sd_free( p_dk_socket_handler );
    p_dk_socket_handler = NULL;
	return SUCCESS;
}

_int32 sh_request_id_comparator(void *E1, void *E2)
{
	_u32 value1 = (_u32)E1;
	_u32 value2 = (_u32)E2;
	LOG_DEBUG( "sh_request_id_comparator:value1=%u,value2=%u",value1,value2 );	
	return value1 > value2 ? 1 : ( value1 < value2 ? -1 : 0 ); 
}

_int32 sh_send_packet_by_domain( DK_SOCKET_HANDLER *p_dk_socket_handler, 
	char *p_host, _u16 port, char *p_buffer, _u32 data_len )
{
	LOG_DEBUG( "sh_send_packet_by_domain, p_host:%s, port:%u", p_host, port );
	return socket_proxy_sendto_by_domain( p_dk_socket_handler->_udp_socket, p_buffer, data_len, 
		p_host, port, sh_udp_sendto_callback, NULL );	
}

_int32 sh_send_packet( DK_SOCKET_HANDLER *p_dk_socket_handler, _u32 ip, _u16 port, 
	char *p_buffer, _u32 data_len, DK_PROTOCAL_HANDLER *p_protocal_handler, _u32 packet_key )
{
	_int32 ret_val = SUCCESS;
	PAIR node;
	DK_REQUEST_NODE *p_dk_request_node = NULL;
    _u32 pack_num = list_size( &p_dk_socket_handler->_request_list );
    MAP_ITERATOR cur_node = NULL;
    
#ifdef _LOGGER
    char addr[24] = {0};
    sd_inet_ntoa(ip, addr, 24);
	LOG_DEBUG( "sh_send_packet, ip:%s, port:%u, pack_num:%u, key:%u", addr, port, pack_num, packet_key );
#endif

#ifdef _PACKET_DEBUG
    char test_buffer[1024];
    sd_memset(test_buffer,0,1024);
    str2hex( p_buffer, data_len, test_buffer, 1024);
    LOG_DEBUG( "sh_send_packet:*buffer[%u]=%s" ,(_u32)data_len, test_buffer);
/*
    sd_memset(test_buffer,0,1024);
    sd_memcpy(test_buffer,p_buffer,data_len);
    LOG_DEBUG( "sh_send_packet:*buffer[%u]=%s",data_len, test_buffer);
    */
#endif 

    if( pack_num > dk_socket_packet_max_num() )
    {
        LOG_DEBUG( "sh_send_packet,list full: pack_num:%u", pack_num );
        return -1; 
    }
    
	ret_val= dk_request_node_malloc_wrap( &p_dk_request_node );
	if( ret_val != SUCCESS )
	{
		goto ErrHandler2;
	}

	ret_val= dk_request_node_init( p_dk_request_node, ip, port, p_buffer, data_len, p_dk_socket_handler );
	if( ret_val != SUCCESS )
	{
		goto ErrHandler1;
	}

	if( p_protocal_handler != NULL )
	{
		node._key = (void *)packet_key;
		node._value = (void *)p_protocal_handler;
        LOG_DEBUG( "sh_send_packet, map_insert_node:_key:%u, _value :0x%x", packet_key, p_protocal_handler );

        map_find_iterator( &p_dk_socket_handler->_resp_handler_map, (void *)packet_key, &cur_node );
        if( cur_node != MAP_END( p_dk_socket_handler->_resp_handler_map) )
        {  
            //曾发现相近的file_id有相同的key,这种情况下会在更新桶时出现,也没必要查了
            //sd_assert( MAP_VALUE(cur_node) == p_protocal_handler );
            if(MAP_VALUE(cur_node) != p_protocal_handler)
            {
                ret_val = DK_SIMI_FILE_ID;
    			goto ErrHandler0;
            }
        }
        else
        {
       		ret_val = map_insert_node( &p_dk_socket_handler->_resp_handler_map, &node );
    		if( ret_val != SUCCESS )
    		{
    			goto ErrHandler0;
    		}		         
        }
	}
    
	list_push( &p_dk_socket_handler->_request_list, (void *)p_dk_request_node );

	sh_handle_request_list( p_dk_socket_handler );
	
	return SUCCESS;
ErrHandler0:
	dk_request_node_uninit( p_dk_request_node );
ErrHandler1:
	dk_request_node_free_wrap( p_dk_request_node );
ErrHandler2:
	sd_assert( ret_val != SUCCESS );
	return ret_val;
}


_int32 sh_handle_request_list( DK_SOCKET_HANDLER *p_dk_socket_handler )
{
	_int32 ret_val = SUCCESS;
	DK_REQUEST_NODE *p_dk_request_node = NULL;
	SD_SOCKADDR	addr;

	//LOG_DEBUG( "sh_handle_request_list" );
    
#ifdef _LOGGER
    char ip_addr[24] = {0};
#endif

	if( list_size( &p_dk_socket_handler->_request_list ) == 0
		|| p_dk_socket_handler->_cur_resquest_node_ptr != NULL )
		return SUCCESS;

	list_pop( &p_dk_socket_handler->_request_list, (void * *)& p_dk_request_node );

	addr._sin_family = SD_AF_INET;
	addr._sin_addr = p_dk_request_node->_ip;
	addr._sin_port = sd_htons(p_dk_request_node->_port);
	
	ret_val = socket_proxy_sendto( p_dk_socket_handler->_udp_socket, p_dk_request_node->_buffer, p_dk_request_node->_buffer_len, 
		&addr, sh_udp_sendto_callback, (void *)p_dk_request_node );
	if( ret_val != SUCCESS )
	{
		dk_request_node_uninit( p_dk_request_node );
		dk_request_node_free_wrap( p_dk_request_node );

		CHECK_VALUE( ret_val );
	}	
#ifdef _LOGGER
    sd_inet_ntoa(p_dk_request_node->_ip, ip_addr, 24);
    LOG_DEBUG( "sh_handle_request_list, socket_proxy_sendto, ip:%s, port:%u", ip_addr, p_dk_request_node->_port );
#endif

	return SUCCESS;
}

_int32 sh_udp_sendto_callback( _int32 errcode, _u32 pending_op_count, const char *buffer, _u32 had_send, void *user_data )
{
	_int32 ret_val = SUCCESS;
	DK_REQUEST_NODE *p_dk_request_node = ( DK_REQUEST_NODE * )user_data;
	DK_SOCKET_HANDLER *p_dk_socket_handler = NULL;
	SD_SOCKADDR addr;
	
	LOG_DEBUG( "sh_udp_sendto_callback, errcode:%d", errcode );

	if( p_dk_request_node == NULL )
    {
        sd_free((char*)buffer);
		buffer = NULL;
        return SUCCESS;
    }
    
	p_dk_socket_handler = p_dk_request_node->_dk_socket_handler_ptr;
	if( errcode == MSG_CANCELLED )
	{
		dk_request_node_uninit( p_dk_request_node );
		dk_request_node_free_wrap( p_dk_request_node );
        if( pending_op_count == 0 )
        {
            sh_destory( p_dk_socket_handler );
        }
		return SUCCESS;
	}
		
	if( errcode != SUCCESS && p_dk_request_node->_retry_times < DK_MAX_RETRY_TIMES )
	{
		addr._sin_family = SD_AF_INET;
		addr._sin_addr = p_dk_request_node->_ip;
		addr._sin_port = sd_htons(p_dk_request_node->_port);
		LOG_DEBUG("sh_udp_sendto, buffer = 0x%x, ip = %u, port = %u", buffer, addr._sin_addr, addr._sin_port );
		ret_val = socket_proxy_sendto( p_dk_socket_handler->_udp_socket, p_dk_request_node->_buffer, p_dk_request_node->_buffer_len, 
			&addr, sh_udp_sendto_callback, (void *)p_dk_request_node );
		
		p_dk_request_node->_retry_times++;

		if( ret_val != SUCCESS )
		{
			dk_request_node_uninit( p_dk_request_node );
			dk_request_node_free_wrap( p_dk_request_node );
		}
		return SUCCESS;
	}

	dk_request_node_uninit( p_dk_request_node );
	dk_request_node_free_wrap( p_dk_request_node );

	sh_handle_request_list( p_dk_socket_handler );
	return SUCCESS;	
}

_int32 sh_udp_recvfrom_callback( _int32 errcode, _u32 pending_op_count, char *buffer, _u32 had_recv, SD_SOCKADDR *from, void *user_data )
{
	_int32 ret_val = SUCCESS;
	DK_SOCKET_HANDLER *p_dk_socket_handler = ( DK_SOCKET_HANDLER * )user_data;
	
	LOG_DEBUG( "sh_udp_recvfrom_callback, errcode:%d", errcode );
	
	sd_assert( buffer == p_dk_socket_handler->_recv_buffer_ptr );
	if(errcode == MSG_CANCELLED)
	{
        if( pending_op_count == 0 )
        {
            sh_destory( p_dk_socket_handler );
        }
		return SUCCESS;
	}

    if( errcode == SUCCESS && had_recv > 0 )
    {
        ret_val = p_dk_socket_handler->_recv_packet_handler( from->_sin_addr, sd_ntohs(from->_sin_port), buffer, had_recv );
        if( ret_val != SUCCESS )
        {
            LOG_ERROR( "sh_udp_recvfrom_callback, handler_err!!!, ret:%d", ret_val );
        }
        
        ret_val = socket_proxy_recvfrom( p_dk_socket_handler->_udp_socket, p_dk_socket_handler->_recv_buffer_ptr, DK_MAX_PACKET_LEN, sh_udp_recvfrom_callback, p_dk_socket_handler );
        sd_assert( ret_val == SUCCESS );
    }

	return SUCCESS;	
}

DK_SOCKET_HANDLER *sh_ptr( _u32 dk_type )
{
	sd_assert( dk_type < DK_TYPE_COUNT );
	sd_assert( g_dk_socket_handler[ dk_type ] != NULL );

	return g_dk_socket_handler[ dk_type ];
}

_int32 sh_get_resp_handler( DK_SOCKET_HANDLER *p_dk_socket_handler, _u32 key, DK_PROTOCAL_HANDLER **pp_protocal_handler )
{
	_int32 ret_val = SUCCESS;
	MAP_ITERATOR cur_node = MAP_BEGIN( p_dk_socket_handler->_resp_handler_map );
	_u32 map_key = 0;
	
	*pp_protocal_handler = NULL;

	ret_val = map_find_node(&p_dk_socket_handler->_resp_handler_map, (void *)key, 
		(void **)pp_protocal_handler );

	/*while( cur_node != MAP_END( p_dk_socket_handler->_resp_handler_map ) )
	{
		map_key = (_u32)MAP_KEY( cur_node );
		if( map_key == key )
		{
			*pp_protocal_handler = (DK_PROTOCAL_HANDLER *)MAP_VALUE( cur_node );
			//map_erase_iterator( &p_dk_socket_handler->_resp_handler_map, cur_node );
			break;
		}
		cur_node = MAP_NEXT( p_dk_socket_handler->_resp_handler_map, cur_node );
	}*/
	
	LOG_DEBUG( "sh_get_resp_handler,key:%u, protocal_handler:0x%x", key, *pp_protocal_handler );
	return ret_val;
}

_int32 sh_clear_resp_handler( DK_SOCKET_HANDLER *p_dk_socket_handler, DK_PROTOCAL_HANDLER *p_protocal_handler )
{
	MAP_ITERATOR cur_node = MAP_BEGIN( p_dk_socket_handler->_resp_handler_map );
	MAP_ITERATOR next_node = NULL;
	DK_PROTOCAL_HANDLER *p_map_protocal_handler = NULL;
	
	LOG_DEBUG( "sh_clear_resp_handler, p_protocal_handler:0x%x", p_protocal_handler );

	while( cur_node != MAP_END( p_dk_socket_handler->_resp_handler_map ) )
	{
		p_map_protocal_handler = (DK_PROTOCAL_HANDLER *)MAP_VALUE( cur_node );
		next_node = MAP_NEXT( p_dk_socket_handler->_resp_handler_map, cur_node );
		if( p_map_protocal_handler == p_protocal_handler )
		{
			map_erase_iterator( &p_dk_socket_handler->_resp_handler_map, cur_node );
		}

		cur_node = next_node;
	}
	return SUCCESS;	
}

_u16 sh_get_udp_port( DK_SOCKET_HANDLER *p_dk_socket_handler )
{
	sd_assert( p_dk_socket_handler->_udp_socket != INVALID_SOCKET );
	LOG_DEBUG( "sh_get_udp_port, udp_port:%u", p_dk_socket_handler->_udp_port );

	return p_dk_socket_handler->_udp_port;
}


//dk_request_node malloc/free
_int32 init_dk_request_node_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_dk_request_node_slab == NULL )
	{              
		ret_val = mpool_create_slab( sizeof( DK_REQUEST_NODE ), MIN_DK_REQUEST_NODE, 0, &g_dk_request_node_slab );
		CHECK_VALUE( ret_val );
	}
	return ret_val;
}

_int32 uninit_dk_request_node_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_dk_request_node_slab != NULL )
	{              
		ret_val = mpool_destory_slab( g_dk_request_node_slab );
		CHECK_VALUE( ret_val );
		g_dk_request_node_slab = NULL;
	}
	return ret_val;
}

_int32 dk_request_node_malloc_wrap(DK_REQUEST_NODE **pp_node)
{
	_int32 ret_val = SUCCESS;
	ret_val = mpool_get_slip( g_dk_request_node_slab, (void**)pp_node );
	CHECK_VALUE( ret_val );

	ret_val = sd_memset( *pp_node, 0, sizeof( DK_REQUEST_NODE) );
	return SUCCESS;
}

_int32 dk_request_node_init( DK_REQUEST_NODE *p_node, _u32 ip, _u16 port, 
	char *p_buffer, _u32 buffer_len, DK_SOCKET_HANDLER *p_dk_socket_handler )
{
	p_node->_ip = ip;
	p_node->_port = port;	
	p_node->_retry_times = 0;
	p_node->_buffer = p_buffer;
	p_node->_buffer_len = buffer_len;
	p_node->_dk_socket_handler_ptr = p_dk_socket_handler;
	return SUCCESS;
}

_int32 dk_request_node_uninit( DK_REQUEST_NODE *p_node )
{
	p_node->_dk_socket_handler_ptr = NULL;
	SAFE_DELETE( p_node->_buffer );
	p_node->_retry_times = 0;
	return SUCCESS;
}

_int32 dk_request_node_free_wrap(DK_REQUEST_NODE *p_node)
{
	sd_assert( p_node != NULL );
	if( p_node == NULL ) return SUCCESS;
	return mpool_free_slip( g_dk_request_node_slab, (void*)p_node );
}

#endif

