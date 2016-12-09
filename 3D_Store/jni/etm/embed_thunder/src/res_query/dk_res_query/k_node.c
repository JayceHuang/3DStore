#include "utility/define.h"
#ifdef _DK_QUERY

#include "k_node.h"
#include "dk_setting.h"

#include "utility/logid.h"
#define LOGID LOGID_DK_QUERY
#include "utility/logger.h"
#include "utility/mempool.h"
#include "utility/time.h"
#include "utility/peerid.h"
#include "utility/utility.h"
#include "utility/string.h"
#include "utility/crc.h"

#include "utility/mempool.h"

#include "utility/utility.h"
#include "utility/local_ip.h"

#include "utility/define_const_num.h"

static SLAB* g_k_node_slab = NULL;
static SLAB* g_kad_node_slab = NULL;
static SLAB* g_node_info_slab = NULL;
static SLAB* g_kad_node_info_slab = NULL;


#define DK_PEERID_SUFFIX           ("EMB")

static k_node_destory_handler g_k_node_destory_table[DK_TYPE_COUNT] = { NULL, NULL };

_int32 k_node_module_init( void )
{
	_int32 ret_val = SUCCESS;
	
	g_k_node_destory_table[DHT_TYPE] = k_node_destory;
	g_k_node_destory_table[KAD_TYPE] = kad_node_destory;
		
	ret_val = init_k_node_slabs();
	if( ret_val != SUCCESS ) goto ErrHandler;
	
	ret_val = init_kad_node_slabs();
	if( ret_val != SUCCESS ) goto ErrHandler;
	
	ret_val = init_node_info_slabs();
	if( ret_val != SUCCESS ) goto ErrHandler;
	
	ret_val = init_kad_node_info_slabs();
	if( ret_val != SUCCESS ) goto ErrHandler;
	return SUCCESS;	
ErrHandler:
	k_node_module_uninit();
	return ret_val;
}

_int32 k_node_module_uninit( void )
{
	g_k_node_destory_table[DHT_TYPE] = NULL;
	g_k_node_destory_table[KAD_TYPE] = NULL;
		
	uninit_k_node_slabs();
	uninit_kad_node_slabs();
	uninit_node_info_slabs();
	uninit_kad_node_info_slabs();
	
	return SUCCESS;	
}

_int32 k_distance_create( K_DISTANCE **pp_k_distance )
{
	_int32 ret_val = SUCCESS;
	K_DISTANCE *p_k_distance = NULL;
	
	//LOG_DEBUG( "k_distance_create" );

	*pp_k_distance = NULL;
	ret_val = sd_malloc( sizeof(K_DISTANCE), (void **)&p_k_distance );
	CHECK_VALUE( ret_val );	

	ret_val = bitmap_init( &p_k_distance->_bit_map );
	if( ret_val != SUCCESS ) 
	{
		SAFE_DELETE( p_k_distance );
		return ret_val;
	}

    *pp_k_distance = p_k_distance;
	return SUCCESS;	
}

_int32 k_distance_create_with_char_buffer( _u8 *p_buffer, _u32 buffer_len, K_DISTANCE **pp_k_distance )
{
	_int32 ret_val = SUCCESS;
	K_DISTANCE *p_k_distance = NULL;
	
	LOG_DEBUG( "k_distance_create_with_char_buffer" );

	*pp_k_distance = NULL;

	ret_val = sd_malloc( sizeof(K_DISTANCE), (void **)&p_k_distance );
	CHECK_VALUE( ret_val );

	ret_val = k_distance_init_with_char_buffer( p_k_distance, p_buffer, buffer_len );
	if( ret_val != SUCCESS ) 
	{
		SAFE_DELETE( p_k_distance );
		return ret_val;
	}

	*pp_k_distance = p_k_distance;
	return SUCCESS;	
}

_int32 k_distance_init( K_DISTANCE *p_k_distance, _u32 bit_count )
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "k_distance_init, p_k_distance:0x%x, bit_count:%u", p_k_distance, bit_count );

	ret_val = bitmap_init_with_bit_count( &p_k_distance->_bit_map, bit_count );
	CHECK_VALUE( ret_val );

	return SUCCESS;	
}

_int32 k_distance_init_with_char_buffer( K_DISTANCE *p_k_distance, _u8 *p_buffer, _u32 buffer_len )
{
	_int32 ret_val = SUCCESS;
	
	LOG_DEBUG( "k_distance_init_with_char_buffer, p_k_distance:0x%x, buffer_len:%u", p_k_distance, buffer_len );
	
	ret_val = bitmap_init_with_bit_count( &p_k_distance->_bit_map, buffer_len * 8 );
	if( ret_val != SUCCESS ) 
	{
		SAFE_DELETE( p_k_distance );
		return ret_val;
	}

    sd_memcpy( p_k_distance->_bit_map._bit, p_buffer, buffer_len);

	return SUCCESS;	
}

_int32 k_distance_child_copy_construct( const K_DISTANCE *p_src_dist, K_DISTANCE *p_dist_0, K_DISTANCE *p_dist_1 )
{
	_int32 ret_val = SUCCESS;
	_u32 bit_count = bitmap_bit_count( &p_src_dist->_bit_map );

	//若有大小,则调用此函数会导致内存泄漏
	sd_assert( bitmap_bit_count( &p_dist_0->_bit_map ) == 0 );
	sd_assert( bitmap_bit_count( &p_dist_1->_bit_map ) == 0 );
	
	LOG_DEBUG( "k_distance_child_copy_construct begin, p_src_dist:0x%x, p_dist_0:0x%x, p_dist_1:0x%x",
        p_src_dist, p_dist_0, p_dist_1 );

	ret_val = bitmap_init_with_bit_count( &p_dist_0->_bit_map, bit_count + 1 );
	if( ret_val != SUCCESS )
	{
		goto ErrHandle;
	}
	
	ret_val = bitmap_init_with_bit_count( &p_dist_1->_bit_map, bit_count + 1 );
	if( ret_val != SUCCESS )
	{
		goto ErrHandle1;
	}

	ret_val = bitmap_copy( &p_src_dist->_bit_map, &p_dist_0->_bit_map );
	if( ret_val != SUCCESS )
	{
		goto ErrHandle2;
	}

	ret_val = bitmap_copy( &p_src_dist->_bit_map, &p_dist_1->_bit_map );
	if( ret_val != SUCCESS )
	{
		goto ErrHandle2;
	}	
	
	ret_val = bitmap_set( &p_dist_0->_bit_map, bit_count, 0 );
	if( ret_val != SUCCESS )
	{
        sd_assert( FALSE );
		goto ErrHandle2;
	}		
	
	ret_val = bitmap_set( &p_dist_1->_bit_map, bit_count, 1 );
	if( ret_val != SUCCESS )
	{
        sd_assert( FALSE );
		goto ErrHandle2;
	}	
	LOG_DEBUG( "k_distance_child_copy_construct SUCCESS, print value:" );
    
	LOG_DEBUG( "p_src_dist->_bit_map:" );
    bitmap_print( &p_src_dist->_bit_map );
	LOG_DEBUG( "p_dist_0->_bit_map:" );
    bitmap_print( &p_dist_0->_bit_map );
	LOG_DEBUG( "p_dist_1->_bit_map:" );
    bitmap_print( &p_dist_1->_bit_map );
    
	return SUCCESS;	
	
ErrHandle2:
	bitmap_uninit( &p_dist_1->_bit_map );
ErrHandle1:
	bitmap_uninit( &p_dist_0->_bit_map );
ErrHandle:
    sd_assert( FALSE );
	return ret_val;
		
}

_int32 k_distance_copy_construct( const K_DISTANCE *p_src_dist, K_DISTANCE *p_dist_dest )
{
	_int32 ret_val = SUCCESS;
	_u32 bit_count = bitmap_bit_count( &p_src_dist->_bit_map );
	sd_assert( bitmap_bit_count( &p_dist_dest->_bit_map ) == 0 );
	LOG_DEBUG( "k_distance_copy_construct" );

	ret_val = bitmap_init_with_bit_count( &p_dist_dest->_bit_map, bit_count );
	CHECK_VALUE( ret_val );
	
	return bitmap_copy( &p_src_dist->_bit_map, &p_dist_dest->_bit_map );
}

_int32 k_distance_set_bit( K_DISTANCE *p_dist, _u32 bit_index, BOOL value )
{
	LOG_DEBUG( "k_distance_set_bit" );
	return bitmap_set( &p_dist->_bit_map, bit_index, value );
}

_int32 k_distance_xor( const K_DISTANCE *p_k_dist_1, const K_DISTANCE *p_k_dist_2, K_DISTANCE *p_dist_ret )
{
    _int32 ret_val = SUCCESS;
	ret_val = bitmap_xor( &p_k_dist_1->_bit_map, &p_k_dist_2->_bit_map, &p_dist_ret->_bit_map );

#ifdef _LOGGER
/*
	LOG_DEBUG( "k_distance_xor, p_k_dist_1" );
    bitmap_print( &p_k_dist_1->_bit_map );
	LOG_DEBUG( "k_distance_xor, p_k_dist_2" );
    bitmap_print( &p_k_dist_2->_bit_map );
	LOG_DEBUG( "k_distance_xor, p_dist_ret" );
    bitmap_print( &p_dist_ret->_bit_map );
    */
#endif
    return SUCCESS;
}

_int32 k_distance_uninit( K_DISTANCE *p_k_distance )
{
	LOG_DEBUG( "k_distance_uninit, p_k_distance:0x%x", p_k_distance );
	if( p_k_distance == NULL )
	{
		sd_assert( FALSE );
		return SUCCESS;
	}
	 bitmap_uninit( &p_k_distance->_bit_map );
	return SUCCESS;	

}

_int32 k_distance_destory( K_DISTANCE **pp_k_distance )
{
	LOG_DEBUG( "k_distance_destory, p_k_distance:0x%x", *pp_k_distance  );
	k_distance_uninit( *pp_k_distance );
	SAFE_DELETE( *pp_k_distance );
	*pp_k_distance = NULL;
	return SUCCESS;	
}

_u64 k_distance_get_value( K_DISTANCE *p_k_distance )
{
	_u64 value = 0;
	_u32 pos = 0;
	_u32 valid_num = bitmap_bit_count( &p_k_distance->_bit_map );
	
	for( ; pos < valid_num; pos++ )
	{
		if( bitmap_at( &p_k_distance->_bit_map, pos ) )
		{
			break;
		}
	}

	for( ; pos < valid_num; pos++ )
	{
		value = value<<1;
		if( bitmap_at( &p_k_distance->_bit_map, pos ) )
		{
			value += 1;
		}		
	}
    
	LOG_DEBUG( "k_distance_get_value, p_k_distance:0x%x, value:%llu, print distance:", p_k_distance, value );
	bitmap_print( &p_k_distance->_bit_map );

	return value;
}

BOOL k_distance_get_bool( const K_DISTANCE *p_k_distance, _u32 pos )
{
	return bitmap_at( &p_k_distance->_bit_map, pos );
}

_u8 *k_distance_get_bit_buffer( K_DISTANCE *p_k_distance )
{
	return bitmap_get_bits( &p_k_distance->_bit_map );
}

_u32 k_distance_buffer_len( K_DISTANCE *p_k_distance )
{
	return p_k_distance->_bit_map._mem_size;
}

_int32 k_distance_compare( K_DISTANCE *p_k_distance_1, K_DISTANCE *p_k_distance_2, _int32 *p_result )
{
    _int32 ret_val = SUCCESS;
    
	ret_val = bitmap_compare( &p_k_distance_1->_bit_map, &p_k_distance_2->_bit_map, p_result );
#ifdef _DEBUG
/*
	LOG_DEBUG( "k_distance_compare, p_k_distance_1:0x%x, print:", p_k_distance_1 );
    k_distance_print(p_k_distance_1);

    LOG_DEBUG( "k_distance_compare, p_k_distance_2:0x%x, print:", p_k_distance_2 );
    k_distance_print(p_k_distance_2);

    LOG_DEBUG( "k_distance_compare, p_k_distance_1:0x%x, p_k_distance_2:0x%x, result:%d", 
        p_k_distance_1, p_k_distance_2, *p_result );
*/
#endif

    return ret_val;
}

void k_distance_print( const K_DISTANCE *p_k_distance )
{
    return bitmap_print( &p_k_distance->_bit_map );
}

_int32 k_node_create( _u8 *p_node_id_buffer,_u32 id_len, _u32 ip, _u16 port, K_NODE **pp_k_node )
{
	_int32 ret_val = SUCCESS;
	K_NODE *p_k_node = NULL;

#ifdef _DEBUG
    char addr[24] = {0};
#endif    

	*pp_k_node = NULL;

	ret_val = k_node_malloc_wrap( &p_k_node );
	CHECK_VALUE( ret_val );

	ret_val = k_distance_init_with_char_buffer( &p_k_node->_node_id, p_node_id_buffer, id_len );
	CHECK_VALUE( ret_val );

	p_k_node->_peer_addr._ip = ip;
	p_k_node->_peer_addr._port = port;
	
	sd_time( &p_k_node->_last_active_time );
	p_k_node->_old_times = 0;

	*pp_k_node = p_k_node;
    
#ifdef _DEBUG
    sd_inet_ntoa(p_k_node->_peer_addr._ip, addr, 24);
    LOG_DEBUG( "k_node_create.p_k_node:0x%x, ip:%s, port:%d", 
        p_k_node, addr, p_k_node->_peer_addr._port );
#endif    

	return SUCCESS;
}

_int32 k_node_destory( K_NODE *p_k_node )
{
#ifdef _DEBUG
    char addr[24] = {0};
    sd_inet_ntoa(p_k_node->_peer_addr._ip, addr, 24);
    LOG_DEBUG( "k_node_destory.p_k_node:0x%x, ip:%s, port:%d", 
        p_k_node, addr, p_k_node->_peer_addr._port );
#endif   

	k_distance_uninit( &p_k_node->_node_id );
	k_node_free_wrap( p_k_node );
	return SUCCESS;	
}

const K_NODE_ID* k_node_get_node_id( const K_NODE *p_k_node )
{
	//LOG_DEBUG( "k_node_get_node_id" );
	if( p_k_node == NULL )
	{
		sd_assert( FALSE );
		return NULL;	
	}
	return &p_k_node->_node_id;
}


BOOL k_node_is_equal( const K_NODE *p_k_node_1, const K_NODE *p_k_node_2, _u32 pos )
{
    BOOL ret_bool = FALSE;
#ifdef _DEBUG
/*
    char addr[24] = {0};
    sd_inet_ntoa(p_k_node_1->_peer_addr._ip, addr, 24);
    LOG_DEBUG( "k_node_is_equal.p_k_node_1:0x%x, ip:%s, port:%d", 
        p_k_node_1, addr, p_k_node_1->_peer_addr._port );
    
    sd_inet_ntoa(p_k_node_2->_peer_addr._ip, addr, 24);
    LOG_DEBUG( "k_node_is_equal.p_k_node_2:0x%x, ip:%s, port:%d", 
        p_k_node_2, addr, p_k_node_2->_peer_addr._port );
        */
#endif  

	const K_NODE_ID *p_k_node_id_1 = k_node_get_node_id( p_k_node_1 );
	const K_NODE_ID *p_k_node_id_2 = k_node_get_node_id( p_k_node_2 );

/*
	LOG_DEBUG( "k_node_is_equal,p_k_node_id_1:0x%x", p_k_node_id_1 );
    bitmap_print( &p_k_node_id_1->_bit_map );
	LOG_DEBUG( "k_node_is_equal,p_k_node_id_2::0x%x", p_k_node_id_2 );
    bitmap_print( &p_k_node_id_1->_bit_map );
*/
	ret_bool = bitmap_is_equal( &p_k_node_id_1->_bit_map, &p_k_node_id_2->_bit_map, pos );
    /*
	LOG_DEBUG( "k_node_is_equal,p_k_node_1:0x%x, p_k_node_2:0x%x, ret:%d", 
        p_k_node_1, p_k_node_2, ret_bool );
    */
        
    return ret_bool;
    
}

BOOL k_node_is_old( K_NODE *p_k_node )
{
	_int32 ret_val = SUCCESS;
	_u32 cur_time = 0;
	BOOL ret_bool = FALSE;
    _u32 last_active_time = p_k_node->_last_active_time;
    
#ifdef _LOGGER
    char addr[24] = {0};
#endif	

	ret_val = sd_time( &cur_time );
	if( ret_val != SUCCESS )
	{
		sd_assert( FALSE );
		return FALSE;
	}
	
	if( cur_time < last_active_time )
	{
		//sd_assert( FALSE );
		return FALSE;
	}
	
	if( cur_time - last_active_time < dk_node_old_time() )
	{
		ret_bool = FALSE;
	}	
	else 
	{
		ret_bool = TRUE;
		p_k_node->_last_active_time = cur_time;
        p_k_node->_old_times++;
	}
#ifdef _LOGGER
    sd_inet_ntoa( p_k_node->_peer_addr._ip, addr, 24);
    LOG_DEBUG( "k_node_is_old, p_k_node:0x%x, ip:%s, port:%u, is_old:%d, cur_time:%u, last_active_time:%u, standed_old_time:%u, old_times:%u", 
        p_k_node, addr, p_k_node->_peer_addr._port, ret_bool, cur_time, last_active_time, dk_node_old_time(), p_k_node->_old_times );
#endif	

	return ret_bool;
}

BOOL k_node_is_abandon( K_NODE *p_k_node )
{
	BOOL ret_bool = FALSE;
#ifdef _LOGGER
    char addr[24] = {0};
#endif	

	if( p_k_node->_old_times > dk_node_max_old_times() )
		ret_bool = TRUE;
    
#ifdef _LOGGER
    sd_inet_ntoa( p_k_node->_peer_addr._ip, addr, 24);
    LOG_DEBUG( "k_node_is_abandon, p_k_node:0x%x, ip:%s, port:%u, is_abandon:%d, _old_times:%u", 
        p_k_node, addr, p_k_node->_peer_addr._port, ret_bool, p_k_node->_old_times );
#endif		

	return ret_bool;
}

_int32 k_node_generate_id( _u8 *buffer, _int32 size )
{
	_int32 ret_val = SUCCESS;
	char physical_addr[10];
	_int32 physical_addr_len = 10;
	_u32 suffix_len = sd_strlen(DK_PEERID_SUFFIX);
    _u16 crc = 0;
    _u32 local_ip = sd_get_local_ip();
	
	LOG_DEBUG( "k_node_generate_id" );
	if( size < physical_addr_len
		|| suffix_len >= size )
	{
		sd_assert( FALSE );
		return -1;
	}
	
	sd_memset( buffer, 'X', size );

	ret_val = get_physical_address(physical_addr, &physical_addr_len);
	if(ret_val == SUCCESS)
	{
		/* convert to peerid */
		ret_val = str2hex(physical_addr, physical_addr_len, ( char *)(buffer+suffix_len), size-suffix_len);
		CHECK_VALUE(ret_val);
	}

	sd_memcpy( buffer, DK_PEERID_SUFFIX, sd_strlen(DK_PEERID_SUFFIX) );

    crc = sd_get_crc16( buffer, size - 2 );
    
    if( sd_is_in_nat( local_ip ) )
    {
        crc = ~crc;
    }
    
	sd_memcpy( buffer + size - 2, (void *)&crc, 2 );
    
	return SUCCESS;
}


//k_node malloc/free
_int32 init_k_node_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_k_node_slab == NULL )
	{              
		ret_val = mpool_create_slab( sizeof( K_NODE ), MIN_K_NODE, 0, &g_k_node_slab );
		CHECK_VALUE( ret_val );
	}

	return ret_val;
}

_int32 uninit_k_node_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_k_node_slab != NULL )
	{              
		ret_val = mpool_destory_slab( g_k_node_slab );
		CHECK_VALUE( ret_val );
		g_k_node_slab = NULL;
	}
	return ret_val;
}

_int32 k_node_malloc_wrap(K_NODE **pp_node)
{
	return mpool_get_slip( g_k_node_slab, (void**)pp_node );
}

_int32 k_node_free_wrap(K_NODE *p_node)
{
	sd_assert( p_node != NULL );
	if( p_node == NULL ) return SUCCESS;
	return mpool_free_slip( g_k_node_slab, (void*)p_node );
}
	
_int32 kad_node_create( _u8 *p_node_id_buffer,_u32 id_len, _u32 ip, _u16 port, _u16 tcp_port, _u8 version, KAD_NODE **pp_kad_node )
{
    _int32 ret_val = SUCCESS;
    KAD_NODE *p_kad_node = NULL;
    K_NODE *p_k_node = NULL;

#ifdef _DEBUG
    char addr[24] = {0};
#endif    

    *pp_kad_node = NULL;

    ret_val = kad_node_malloc_wrap( &p_kad_node );
    CHECK_VALUE( ret_val );

    p_k_node = &p_kad_node->_k_node;

    ret_val = k_distance_init_with_char_buffer( &p_k_node->_node_id, p_node_id_buffer, id_len );
    CHECK_VALUE( ret_val );

    p_k_node->_peer_addr._ip = ip;
    p_k_node->_peer_addr._port = port;
    
    sd_time( &p_k_node->_last_active_time );
    p_k_node->_old_times = 0;
    
    p_kad_node->_tcp_port = tcp_port;
    p_kad_node->_version = version;

    *pp_kad_node = p_kad_node;
    
#ifdef _DEBUG
    sd_inet_ntoa(ip, addr, 24);
    LOG_DEBUG( "kad_node_create.p_kad_node:0x%x, ip:%s, port:%d, tcp_port:%d, version:%d", 
        p_kad_node, addr, port, tcp_port, version );
#endif    

    return SUCCESS;    

}

_int32 kad_node_destory( K_NODE *p_k_node )
{
	KAD_NODE *p_kad_node = (KAD_NODE *)p_k_node;
	LOG_DEBUG( "kad_node_destory" );
	k_distance_uninit( &p_k_node->_node_id );
	kad_node_free_wrap( p_kad_node );
	return SUCCESS;
}

k_node_destory_handler k_node_get_destoryer( _u32 dk_type )
{
	//LOG_DEBUG( "k_node_get_destoryer" );
	return g_k_node_destory_table[dk_type];
}

//kad_node malloc/free
_int32 init_kad_node_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_kad_node_slab == NULL )
	{			   
		ret_val = mpool_create_slab( sizeof( KAD_NODE ), MIN_KAD_NODE, 0, &g_kad_node_slab );
		CHECK_VALUE( ret_val );
	}

	return ret_val;
}

_int32 uninit_kad_node_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_kad_node_slab != NULL )
	{			   
		ret_val = mpool_destory_slab( g_kad_node_slab );
		CHECK_VALUE( ret_val );
		g_kad_node_slab = NULL;
	}
	return ret_val;
}

_int32 kad_node_malloc_wrap(KAD_NODE **pp_node)
{
	return mpool_get_slip( g_kad_node_slab, (void**)pp_node );
}

_int32 kad_node_free_wrap(KAD_NODE *p_node)
{
	sd_assert( p_node != NULL );
	if( p_node == NULL ) return SUCCESS;
	return mpool_free_slip( g_kad_node_slab, (void*)p_node );
}
	
	
_int32 dht_node_info_creater( _u32 ip, _u16 port, _u8 version, _u8 *p_id, _u32 id_len, NODE_INFO **pp_node_info )
{
	_int32 ret_val = SUCCESS;
	NODE_INFO *p_node_info = NULL;
	
	LOG_DEBUG( "dht_node_info_creater" );

	ret_val = node_info_malloc_wrap( &p_node_info );
	CHECK_VALUE( ret_val );

	p_node_info->_peer_addr._ip = ip;
	p_node_info->_peer_addr._port = port;
	p_node_info->_try_times = 0;

	*pp_node_info = p_node_info;
	return SUCCESS;
}

_int32 kad_node_info_creater( _u32 ip, _u16 port, _u8 version, _u8 *p_id, _u32 id_len, NODE_INFO **pp_node_info )
{
	_int32 ret_val = SUCCESS;
	KAD_NODE_INFO *p_kad_node_info = NULL;
	NODE_INFO *p_node_info = NULL;
	
    if( id_len != 0 && id_len != KAD_ID_LEN )
    {
        sd_assert( FALSE );
        return KAD_NODE_ID_ERR;
    }
	ret_val = kad_node_info_malloc_wrap( &p_kad_node_info );
	CHECK_VALUE( ret_val );

	p_kad_node_info->_version = version;
	p_kad_node_info->_node_id_ptr = NULL;

    if( p_id != NULL )
    {
        sd_assert( id_len == KAD_ID_LEN );
        ret_val = sd_malloc( KAD_ID_LEN, (void**)&p_kad_node_info->_node_id_ptr );
        if( ret_val != SUCCESS )
        {
            kad_node_info_free_wrap( p_kad_node_info );
            CHECK_VALUE( ret_val );
        }
        sd_memcpy( p_kad_node_info->_node_id_ptr, p_id, KAD_ID_LEN );
    }
    
	p_node_info = &p_kad_node_info->_node_info;
	p_node_info->_peer_addr._ip = ip;
	p_node_info->_peer_addr._port = port;
	p_node_info->_try_times = 0;
    
	LOG_DEBUG( "kad_node_info_creater, p_kad_node_info:0x%x", p_kad_node_info );
    
	*pp_node_info = ( NODE_INFO * )p_kad_node_info;
	return SUCCESS;
}

_int32 dht_node_info_destoryer( NODE_INFO *p_node_info )
{
	LOG_DEBUG( "dht_node_info_destoryer" );
	return node_info_free_wrap( p_node_info );
}

_int32 kad_node_info_destoryer( NODE_INFO *p_node_info )
{
	KAD_NODE_INFO *p_kad_node_info = ( KAD_NODE_INFO *)p_node_info;
	LOG_DEBUG( "kad_node_info_destoryer, p_kad_node_info:0x%x", p_kad_node_info );
    SAFE_DELETE( p_kad_node_info->_node_id_ptr )

	return kad_node_info_free_wrap( p_kad_node_info );
}


//node_info malloc/free wrap
_int32 init_node_info_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_node_info_slab == NULL )
	{			   
		ret_val = mpool_create_slab( sizeof( NODE_INFO ), MIN_NODE_INFO, 0, &g_node_info_slab );
		CHECK_VALUE( ret_val );
	}
	return ret_val;
}

_int32 uninit_node_info_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_node_info_slab != NULL )
	{			   
		ret_val = mpool_destory_slab( g_node_info_slab );
		CHECK_VALUE( ret_val );
		g_node_info_slab = NULL;
	}
	return ret_val;
}

_int32 node_info_malloc_wrap(NODE_INFO **pp_node)
{
	_int32 ret_val = mpool_get_slip( g_node_info_slab, (void**)pp_node );
	CHECK_VALUE( ret_val );
	LOG_DEBUG( "node_info_malloc_wrap:0x%x ", *pp_node );			
	return SUCCESS;
}

_int32 node_info_free_wrap(NODE_INFO *p_node)
{
	_int32 ret_val = SUCCESS;
	sd_assert( p_node != NULL );
	if( p_node == NULL ) return SUCCESS;
	LOG_DEBUG( "node_info_free_wrap:0x%x ", p_node );		
	ret_val = mpool_free_slip( g_node_info_slab, (void*)p_node );
	CHECK_VALUE( ret_val ); 
	
	return SUCCESS;
}

//kad_node_info malloc/free wrap
_int32 init_kad_node_info_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_kad_node_info_slab == NULL )
	{			   
		ret_val = mpool_create_slab( sizeof( KAD_NODE_INFO ), MIN_KAD_NODE_INFO, 0, &g_kad_node_info_slab );
		CHECK_VALUE( ret_val );
	}
	return ret_val;
}

_int32 uninit_kad_node_info_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_kad_node_info_slab != NULL )
	{			   
		ret_val = mpool_destory_slab( g_kad_node_info_slab );
		CHECK_VALUE( ret_val );
		g_kad_node_info_slab = NULL;
	}
	return ret_val;
}

_int32 kad_node_info_malloc_wrap(KAD_NODE_INFO **pp_node)
{
	_int32 ret_val = mpool_get_slip( g_kad_node_info_slab, (void**)pp_node );
	CHECK_VALUE( ret_val );
	//LOG_DEBUG( "kad_node_info_malloc_wrap:0x%x ", *pp_node );			
	return SUCCESS;
}

_int32 kad_node_info_free_wrap(KAD_NODE_INFO *p_node)
{
	_int32 ret_val = SUCCESS;
	sd_assert( p_node != NULL );
	if( p_node == NULL ) return SUCCESS;
	//LOG_DEBUG( "kad_node_info_free_wrap:0x%x ", p_node );		
	ret_val = mpool_free_slip( g_kad_node_info_slab, (void*)p_node );
	CHECK_VALUE( ret_val ); 
	
	return SUCCESS;
}

_u32 dk_calc_key( _u32 ip, _u16 port, _u8 *p_id, _u32 id_len )
{
	char* buffer;
	_u32 hash = 0, x = 0, i = 0;
    if( p_id == NULL)
    {
        sd_assert( FALSE );
        return 0;
    }
	while(id_len != 0)
	{
		hash = (hash << 4) + (*p_id);
		if((x = hash & 0xF0000000L) != 0)
		{
			hash ^= (x >> 24);
			hash &= ~x;
		} 
		++p_id;
		--id_len;
	} 
	buffer = (char*)&ip;
	for(i = 1; i < 7; ++i)
	{
		hash = (hash << 4) + (*buffer);
		if((x = hash & 0xF0000000L) != 0)
		{
			hash ^= (x >> 24);
			hash &= ~x;
		}
		++buffer;
		if(i % 5 == 0)
			buffer = (char*)&port;
	}
	hash = hash & 0x7FFFFFFF;
    
	return hash;
}


#endif

