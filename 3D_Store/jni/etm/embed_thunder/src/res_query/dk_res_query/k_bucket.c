#include "utility/define.h"
#ifdef _DK_QUERY
#include "k_bucket.h"
#include "routing_table.h"

#include "utility/logid.h"
#define LOGID LOGID_DK_QUERY
#include "utility/logger.h"

#include "utility/define_const_num.h"
#include "utility/utility.h"

#include "utility/mempool.h"
#include "utility/time.h"


//public func
static SLAB* g_k_bucket_slab = NULL;

_int32 create_k_bucket_without_index( K_BUCKET *p_parent, K_BUCKET_PARA *p_bucket_para, K_BUCKET **pp_k_bucket )
{
	_int32 ret_val = SUCCESS;
	K_BUCKET *p_k_bucket = NULL;
	LOG_DEBUG( "create_k_bucket_without_index, p_parent:0x%x", p_parent );

	ret_val = k_bucket_malloc_wrap( pp_k_bucket );
	CHECK_VALUE( ret_val );
	
	p_k_bucket = *pp_k_bucket;
	p_k_bucket->_parent = p_parent;
	p_k_bucket->_sub[0] = NULL;
	p_k_bucket->_sub[1] = NULL;	
	p_k_bucket->_bucket_para_ptr = p_bucket_para;
	list_init( &p_k_bucket->_k_node_list );
	bitmap_init( &p_k_bucket->_bucket_index._bit_map );
	return ret_val;
}

_int32 destrory_k_bucket( K_BUCKET **pp_k_bucket )
{
	LIST_ITERATOR cur_node = NULL;
	K_NODE *p_k_node = NULL;
	K_BUCKET *p_k_bucket = NULL;
	_u32 dk_type = 0;
	k_node_destory_handler destory_handler = NULL;
	
	LOG_DEBUG( "destrory_k_bucket, p_k_bucket:0x%x", *pp_k_bucket );

	sd_assert( pp_k_bucket != NULL );
	if( pp_k_bucket == NULL ) return SUCCESS;
	
	p_k_bucket = *pp_k_bucket;
	if( p_k_bucket == NULL ) return SUCCESS;

	dk_type = bucket_para_get_dk_type( p_k_bucket->_bucket_para_ptr );
	destory_handler = k_node_get_destoryer(dk_type);

	cur_node = LIST_BEGIN( p_k_bucket->_k_node_list );
	while( cur_node != LIST_END( p_k_bucket->_k_node_list ) )
	{
		p_k_node = ( K_NODE *)LIST_VALUE( cur_node );
		destory_handler( p_k_node );
		cur_node = LIST_NEXT( cur_node );
	}
	list_clear( &p_k_bucket->_k_node_list );
	k_distance_uninit( &p_k_bucket->_bucket_index );
	p_k_bucket->_parent = NULL;
	p_k_bucket->_bucket_para_ptr = NULL;

	if( p_k_bucket->_sub[0] != NULL )
	{
		destrory_k_bucket( &p_k_bucket->_sub[0] );
	}
	if( p_k_bucket->_sub[1] != NULL )
	{
		destrory_k_bucket( &p_k_bucket->_sub[1] );
	}

	k_bucket_free_wrap( p_k_bucket );
	*pp_k_bucket = NULL;
	
	return SUCCESS;
}


_int32 kb_find_bucket_by_node( K_BUCKET *p_k_bucket, K_NODE *p_k_node, K_BUCKET **pp_k_target_bucket )
{
	_int32 ret_val = SUCCESS;
	K_DISTANCE k_dist;
	_u32 dk_type = bucket_para_get_dk_type( p_k_bucket->_bucket_para_ptr );
	LOG_DEBUG( "kb_find_bucket_by_node begin, p_k_bucket:0x%x", p_k_bucket );

    *pp_k_target_bucket = NULL;

	sd_assert( NULL != p_k_bucket->_bucket_para_ptr );
	if( NULL == p_k_bucket->_bucket_para_ptr ) return -1;

	ret_val = rt_distance_calc( dk_type, &p_k_node->_node_id, &k_dist );
	CHECK_VALUE( ret_val );

	ret_val = kb_find_bucket_by_key( p_k_bucket, &k_dist, pp_k_target_bucket );
	CHECK_VALUE( ret_val );

	k_distance_uninit( &k_dist );
    
	LOG_DEBUG( "kb_find_bucket_by_node end, p_k_bucket:0x%x, p_k_target_bucket:0x%x", 
     p_k_bucket, *pp_k_target_bucket );
	return SUCCESS;
}

_int32 kb_find_bucket_by_key( K_BUCKET *p_k_bucket, K_DISTANCE *p_k_distance, K_BUCKET **pp_k_target_bucket )
{
	_int32 ret_val = SUCCESS;
	_u32 level = 0;
	//LOG_DEBUG( "kb_find_bucket_by_key, p_k_bucket:0x%x", p_k_bucket );

	if( kb_is_leaf( p_k_bucket ) )
	{
        LOG_DEBUG( "kb_find_bucket_by_key success, p_k_bucket:0x%x, p_k_distance:0x%x", 
            p_k_bucket, p_k_distance );
#ifdef _LOGGER
/*
        LOG_DEBUG( "kb_find_bucket_by_key success, p_k_bucket:0x%x, print index:", p_k_bucket );
        k_distance_print( &p_k_bucket->_bucket_index );
        
        LOG_DEBUG( "kb_find_bucket_by_key success, p_k_distance:0x%x, print p_k_distance:", p_k_bucket );
        k_distance_print( p_k_distance );    
        */
#endif
        *pp_k_target_bucket = p_k_bucket;
		return SUCCESS;
	}
	
	ret_val = kb_get_level( p_k_bucket, &level );
	if( ret_val != SUCCESS ) return ret_val;

	sd_assert( level < bitmap_bit_count( &p_k_distance->_bit_map ) );
    
	if( bitmap_at( &p_k_distance->_bit_map, level ) )
	{
		return kb_find_bucket_by_key( p_k_bucket->_sub[1], p_k_distance, pp_k_target_bucket );
	}
	else
	{
		return kb_find_bucket_by_key( p_k_bucket->_sub[0], p_k_distance, pp_k_target_bucket );
	}	
	return SUCCESS;
}

_int32 kb_add_node( K_BUCKET *p_k_bucket, K_NODE *p_k_node )
{
	_int32 ret_val = SUCCESS;
	K_BUCKET *p_target_bucket = NULL;
    BOOL is_delete = FALSE;
	
#ifdef _LOGGER
    K_DISTANCE *p_distance = NULL;
    char addr[24] = {0};

    sd_inet_ntoa( p_k_node->_peer_addr._ip, addr, 24);
	LOG_DEBUG( "kb_add_node begin!!!!!!!!!!!, p_k_bucket:0x%x, p_k_node:0x%x, ip:%s, port:%u", 
        p_k_bucket, p_k_node, addr, p_k_node->_peer_addr._port );
#endif	

	ret_val = kb_find_bucket_by_node( p_k_bucket, p_k_node, &p_target_bucket );
	if( ret_val != SUCCESS ) return ret_val;
	
	ret_val = sd_time( &p_k_node->_last_active_time );
	if( ret_val != SUCCESS ) return ret_val;
    
	if( kb_is_full( p_target_bucket ) )
	{
      	kb_leaf_delete_node( p_target_bucket, p_k_node, &is_delete );

        if( is_delete )
        {
         	ret_val = list_push( &p_target_bucket->_k_node_list, (void *)p_k_node );
#ifdef _LOGGER
            LOG_DEBUG( "kb_add_node end!!!!!!!!!!!!!!!, p_target_bucket:0x%x, bucket_node_num:%u, p_k_node:0x%x, ip:%s, port:%u, _old_times:%d, is_delete:%d", 
                p_target_bucket, list_size(&p_target_bucket->_k_node_list), p_k_node, addr, p_k_node->_peer_addr._port, p_k_node->_old_times, is_delete );
            LOG_DEBUG( "kb_add_node print p_k_bucket info, p_target_bucket:0x%x, print index:", p_target_bucket );
            k_distance_print( &p_target_bucket->_bucket_index );
            ret_val = k_distance_create( &p_distance );
            CHECK_VALUE( ret_val ); 
            ret_val = rt_distance_calc( p_target_bucket->_bucket_para_ptr->_dk_type, &p_k_node->_node_id, p_distance );
            CHECK_VALUE( ret_val ); 
            
            LOG_DEBUG( "kb_add_node print p_k_node info, p_k_node:0x%x, print distance:",  p_k_node );
            k_distance_print( p_distance );
            
            k_distance_destory( &p_distance );
#endif	
            return ret_val;
               
        }
        
		if( kb_split( p_target_bucket ) )
		{
			ret_val = kb_add_node( p_target_bucket, p_k_node ); 
			return ret_val;
		}
        else
        {
            LOG_ERROR( "kb_split FAILED!!!, p_k_bucket:0x%x", p_k_bucket );
            return -1;
        }
	}
    
	kb_leaf_delete_node( p_target_bucket, p_k_node, &is_delete );

	ret_val = list_push( &p_target_bucket->_k_node_list, (void *)p_k_node );
	if( ret_val != SUCCESS ) return ret_val;
    

#ifdef _LOGGER
	LOG_DEBUG( "kb_add_node end!!!!!!!!!!!!!!!, p_k_bucket:0x%x, bucket_node_num:%u, p_k_node:0x%x, ip:%s, port:%u, _old_times:%d, is_delete:%d", 
        p_k_bucket, list_size(&p_target_bucket->_k_node_list), p_k_node, addr, p_k_node->_peer_addr._port, p_k_node->_old_times, is_delete );
#endif	
	
	return SUCCESS;
}

void kb_leaf_delete_node( K_BUCKET *p_k_bucket, K_NODE *p_k_node, BOOL *p_is_delete )
{
	LIST_ITERATOR cur_node = LIST_BEGIN( p_k_bucket->_k_node_list );
	K_NODE *p_list_node = NULL;
	_u32 level = 0;
	k_node_destory_handler destory_handler = NULL;
	_u32 dk_type = 0;
#ifdef _LOGGER
    char addr[24] = {0};
#endif	

	LOG_DEBUG( "kb_leaf_delete_node, p_k_bucket:0x%x, p_k_node:0x%x", 
        p_k_bucket, p_k_node );

    *p_is_delete = FALSE;

	sd_assert( kb_is_leaf( p_k_bucket ) );
	if( kb_is_empty( p_k_bucket ) ) return;
	
	level = kb_get_level( p_k_bucket, &level );
	dk_type = bucket_para_get_dk_type( p_k_bucket->_bucket_para_ptr );
	destory_handler = k_node_get_destoryer(dk_type);	
	
	while( cur_node != LIST_END( p_k_bucket->_k_node_list ) )
	{
		p_list_node = LIST_VALUE( cur_node );
		if( k_node_is_equal( p_k_node, p_list_node, level + 1 ) )
		{
        	LOG_DEBUG( "kb_leaf_delete_node, p_k_bucket:0x%x, new node:p_k_node:0x%x, old_node:p_list_node:0x%x", 
                p_k_bucket, p_k_node, p_list_node );   
            
#ifdef _LOGGER
            sd_inet_ntoa( p_k_node->_peer_addr._ip, addr, 24);
            LOG_DEBUG( "kb_leaf_delete_node, new node p_k_node:0x%x, ip:%s, port:%u", 
                p_k_node, addr, p_k_node->_peer_addr._port );

            sd_inet_ntoa( p_list_node->_peer_addr._ip, addr, 24);
            LOG_DEBUG( "kb_leaf_delete_node, old node p_list_node:0x%x, ip:%s, port:%u", 
                p_list_node, addr, p_list_node->_peer_addr._port );            
#endif	

            destory_handler( p_list_node );
            list_erase( &p_k_bucket->_k_node_list, cur_node );
            *p_is_delete = TRUE;
			return;
		}
		cur_node = LIST_NEXT( cur_node );
	}
}

_int32 kb_get_old_node_list( K_BUCKET *p_k_bucket, _u32 max_num, LIST *p_old_node_list )
{
	_int32 ret_val = SUCCESS;
	LIST_ITERATOR cur_node = NULL;
	K_NODE *p_k_node = NULL;
	_u32 dk_type = bucket_para_get_dk_type( p_k_bucket->_bucket_para_ptr );
	k_node_destory_handler node_destoryer = k_node_get_destoryer( dk_type );
	
	LOG_DEBUG( "kb_get_old_node_list, p_k_bucket:0x%x, _k_node_list_size:%u", 
        p_k_bucket, list_size( &p_k_bucket->_k_node_list) );
	
	if( max_num == 0 ) return -1;
	
	if( kb_is_leaf( p_k_bucket ) )
	{
		if( !kb_is_empty( p_k_bucket ) )
		{
			cur_node = LIST_BEGIN( p_k_bucket->_k_node_list );
			p_k_node = (K_NODE *)LIST_VALUE( cur_node );
			if( k_node_is_abandon( p_k_node ) )
			{
				node_destoryer( p_k_node );
				list_erase( &p_k_bucket->_k_node_list, cur_node );
				return kb_get_old_node_list( p_k_bucket, max_num, p_old_node_list );
			}
			if( k_node_is_old( p_k_node ) )
			{
				ret_val = list_push( p_old_node_list, p_k_node );
				if( ret_val != SUCCESS) return ret_val;
				
				sd_assert( max_num != 0 );
				max_num--;
			}		
		}
	}
	else
	{
		ret_val = kb_get_old_node_list( p_k_bucket->_sub[0], max_num, p_old_node_list );
		if( ret_val != SUCCESS) return ret_val;
		
		ret_val = kb_get_old_node_list( p_k_bucket->_sub[1], max_num, p_old_node_list );
		if( ret_val != SUCCESS) return ret_val;
	}
	return SUCCESS;
}

//private func

_int32 kb_get_bucket_list( K_BUCKET *p_k_bucket, _u32 *p_max_num, BOOL is_empty, LIST *p_empty_bucket_list )
{
	_int32 ret_val = SUCCESS;
	BOOL bucket_is_empty = TRUE;
	
	//LOG_DEBUG( "kb_get_bucket_list, p_k_bucket:0x%x", p_k_bucket );

	if( *p_max_num == 0 ) return SUCCESS;
	if( kb_is_leaf( p_k_bucket ) )
	{
		bucket_is_empty = kb_is_empty( p_k_bucket );
		if( bucket_is_empty == is_empty )
		{
			ret_val = list_push( p_empty_bucket_list, p_k_bucket );
            //LOG_DEBUG( "kb_get_bucket_list, push list node:p_k_bucket:0x%x", p_k_bucket );

            if( ret_val != SUCCESS) return ret_val;
			(*p_max_num)--;
		}
	}
	else
	{
		ret_val = kb_get_bucket_list( p_k_bucket->_sub[0], p_max_num, is_empty, p_empty_bucket_list );
		if( ret_val != SUCCESS) return ret_val;
		
		ret_val = kb_get_bucket_list( p_k_bucket->_sub[1], p_max_num, is_empty, p_empty_bucket_list );
		if( ret_val != SUCCESS) return ret_val;
	}
	return SUCCESS;
}

_int32 kb_get_bucket_find_node_id( K_BUCKET *p_k_bucket, K_NODE_ID *p_k_node_id )
{
	_int32 ret_val = SUCCESS;
	K_BUCKET *p_parent = p_k_bucket->_parent;
	K_BUCKET *p_other_child = NULL;
	K_BUCKET *p_other_leaf = NULL;
	LIST other_leaf_list;
	LIST_ITERATOR cur_node = NULL;
	K_NODE *p_k_node = NULL;
	_u32 level = 0;
	BOOL value = TRUE;
    _u32 bucket_num = 1;
    
#ifdef _LOGGER
    K_DISTANCE *p_distance = NULL;
#endif

	LOG_DEBUG( "kb_get_bucket_find_node_id begin, p_k_bucket:0x%x", p_k_bucket );
	
	if( p_parent == NULL ) return -1;

	if( p_parent->_sub[0] == p_k_bucket )
	{
		p_other_child = p_parent->_sub[1];
		value = FALSE;
	}
	else
	{
		sd_assert( p_parent->_sub[1] == p_k_bucket );
		p_other_child = p_parent->_sub[0];
		value = TRUE;
	}

	list_init( &other_leaf_list );
	ret_val = kb_get_bucket_list( p_other_child, &bucket_num, FALSE, &other_leaf_list );
	CHECK_VALUE( ret_val );

	if( list_size( &other_leaf_list ) == 0 ) return -1;

    cur_node = LIST_BEGIN( other_leaf_list );

	p_other_leaf = (K_BUCKET *)LIST_VALUE( cur_node );
	sd_assert( p_other_leaf != NULL );
	sd_assert( !kb_is_empty(p_other_leaf) );
    
	cur_node = LIST_RBEGIN( p_other_leaf->_k_node_list );
	p_k_node = (K_NODE*)LIST_VALUE( cur_node );

    list_clear(&other_leaf_list);
	
	ret_val = k_distance_copy_construct( &p_k_node->_node_id, p_k_node_id );
	CHECK_VALUE( ret_val );

	ret_val = kb_get_level( p_k_bucket, &level );
	CHECK_VALUE( ret_val );

	ret_val = k_distance_set_bit( p_k_node_id, level, value );
	CHECK_VALUE( ret_val );
    
#ifdef _LOGGER
    ret_val = k_distance_create( &p_distance );
    CHECK_VALUE( ret_val ); 
    
	ret_val = rt_distance_calc( p_k_bucket->_bucket_para_ptr->_dk_type, p_k_node_id, p_distance );
	CHECK_VALUE( ret_val );	
    
    LOG_DEBUG( "kb_get_bucket_find_node_id end, p_k_bucket:0x%x, index print:", p_k_bucket );
    k_distance_print( &p_k_bucket->_bucket_index );


    LOG_DEBUG( "kb_get_bucket_find_node_id end, p_k_node_id:0x%x, info print distance:", p_k_node_id );
    k_distance_print( p_distance );

    k_distance_destory( &p_distance );
#endif
	return SUCCESS;

}

_int32 kb_get_nearst_node( K_BUCKET *p_k_bucket, const K_NODE_ID *p_k_node_id, 
	LIST *p_node_list, _u32 *p_node_num )
{
	_int32 ret_val = SUCCESS;
	K_DISTANCE k_dist;
	_u32 dk_type = bucket_para_get_dk_type( p_k_bucket->_bucket_para_ptr );
#ifdef _LOGGER
/*
    char addr[24] = {0};
    LIST_ITERATOR cur_node = NULL;
    K_NODE *p_k_node = NULL;
    */
#endif	

	LOG_DEBUG( "kb_get_nearst_node begin, p_k_bucket:0x%x, max_node_num:%u", 
      p_k_bucket, *p_node_num );

	sd_assert( NULL != p_k_bucket->_bucket_para_ptr );
	if( NULL == p_k_bucket->_bucket_para_ptr ) return -1;

	ret_val = rt_distance_calc( dk_type, p_k_node_id, &k_dist );
	CHECK_VALUE( ret_val );

	ret_val =  kb_get_nearst_node_by_key( p_k_bucket, &k_dist, p_node_list, p_node_num );
	CHECK_VALUE( ret_val );

	k_distance_uninit( &k_dist );
    
	LOG_DEBUG( "kb_get_nearst_node end, p_k_bucket:0x%x, need node_num:%u", 
      p_k_bucket, *p_node_num );
    
#ifdef _LOGGER
/*
	LOG_DEBUG( "kb_get_nearst_node end, print info!!!, p_k_node_id:0x%x, print bits:", p_k_bucket );
    k_distance_print( p_k_node_id );

    cur_node = LIST_BEGIN( *p_node_list );
    while( cur_node != LIST_END( *p_node_list ) )
    {
        p_k_node = ( K_NODE *)LIST_VALUE( cur_node );
        
        sd_inet_ntoa( p_k_node->_peer_addr._ip, addr, 24);

        LOG_DEBUG( "kb_get_nearst_node end, print info!!!, p_k_bucket:0x%x, p_k_node:0x%x, ip:%s, port:%u", 
            p_k_bucket, p_k_node, addr, p_k_node->_peer_addr._port );        
        k_distance_print( &p_k_node->_node_id );

        cur_node = LIST_NEXT( cur_node );
    }
    */
#endif	
    
	return SUCCESS;
}


_int32 kb_get_nearst_node_by_key( K_BUCKET *p_k_bucket, const K_DISTANCE *p_key, 
	LIST *p_node_list, _u32 *p_node_num )
{
	_int32 ret_val = SUCCESS;
	LIST_ITERATOR cur_node = NULL;
	_u32 level = 0;
	BOOL index = 0;

	LOG_DEBUG( "kb_get_nearst_node_by_key, p_k_bucket:0x%x", p_k_bucket );

	if( kb_is_leaf( p_k_bucket ) )
	{
        LOG_DEBUG( "kb_get_nearst_node_by_key, leaf p_k_bucket:0x%x, node_list_size:%d", 
            p_k_bucket, list_size(&p_k_bucket->_k_node_list) );
		cur_node = LIST_BEGIN( p_k_bucket->_k_node_list );
		while( cur_node != LIST_END( p_k_bucket->_k_node_list ) 
			&& *p_node_num > 0 )
		{
			ret_val = list_push( p_node_list, LIST_VALUE( cur_node ) );
			CHECK_VALUE( ret_val );
			
			cur_node = LIST_NEXT( cur_node );
			(*p_node_num)--;
		}
		return SUCCESS;
	}
	else
	{
		ret_val = kb_get_level( p_k_bucket, &level );
		CHECK_VALUE( ret_val );

		index = k_distance_get_bool( p_key, level );
		ret_val = kb_get_nearst_node_by_key( p_k_bucket->_sub[index], p_key, p_node_list, p_node_num );
		CHECK_VALUE( ret_val );
		
		if( *p_node_num > 0 )
		{
			kb_get_nearst_node_by_key( p_k_bucket->_sub[!index], p_key, p_node_list, p_node_num );
			CHECK_VALUE( ret_val );
		}
	}
	return SUCCESS;	
}

_u32 kb_get_node_num( K_BUCKET *p_k_bucket )
{
	_u32 node_num = 0;
	
	if( kb_is_leaf( p_k_bucket ) )
	{
		node_num = list_size( &p_k_bucket->_k_node_list);
	}
	else
	{
		node_num = kb_get_node_num( p_k_bucket->_sub[0] );
		node_num += kb_get_node_num( p_k_bucket->_sub[1] );
	}
	
	LOG_DEBUG( "kb_get_node_num, p_k_bucket:0x%x, node_num:%u", p_k_bucket, node_num );
	return node_num;
}

_u32 bucket_para_get_dk_type( K_BUCKET_PARA *p_bucket_para )
{
	return p_bucket_para->_dk_type;
}

BOOL kb_split( K_BUCKET *p_k_bucket )
{
	_int32 ret_val = SUCCESS;
	_u32 level = 0;
	_u64 value = 0;
	K_BUCKET *p_k_bucket_0 = NULL;
	K_BUCKET *p_k_bucket_1 = NULL;
	LIST_ITERATOR cur_node = NULL;
	K_NODE *p_node = NULL;
	_u32 dk_type = bucket_para_get_dk_type( p_k_bucket->_bucket_para_ptr );
	k_node_destory_handler node_destoryer = k_node_get_destoryer(dk_type);
	
	LOG_DEBUG( "kb_split begin!!!!, p_k_bucket:0x%x", p_k_bucket );

	ret_val = kb_get_level( p_k_bucket, &level );
	if( ret_val != SUCCESS ) return FALSE;

	value = k_distance_get_value( &p_k_bucket->_bucket_index );
	if( ( level > p_k_bucket->_bucket_para_ptr->_min_level 
        && value > p_k_bucket->_bucket_para_ptr->_can_split_max_distance )
       || level >=  p_k_bucket->_bucket_para_ptr->_max_level )
	{
        LOG_DEBUG( "kb_split FAIL!!!, p_k_bucket:0x%x, value:%llu, level:%u",
            p_k_bucket, value, level );
		return FALSE;
	}

	ret_val = create_k_bucket_without_index( p_k_bucket, 
		p_k_bucket->_bucket_para_ptr, &p_k_bucket_0 );
	if( ret_val != SUCCESS ) return FALSE;	
	p_k_bucket->_sub[0] = p_k_bucket_0;
	
	ret_val = create_k_bucket_without_index( p_k_bucket, 
		p_k_bucket->_bucket_para_ptr, &p_k_bucket_1 );
	if( ret_val != SUCCESS )
	{
        sd_assert( FALSE );
		destrory_k_bucket( &p_k_bucket->_sub[0] );
		return FALSE;	
	}
	p_k_bucket->_sub[1] = p_k_bucket_1;

	ret_val = k_distance_child_copy_construct( &p_k_bucket->_bucket_index, 
		&p_k_bucket_0->_bucket_index, &p_k_bucket_1->_bucket_index );
	if( ret_val != SUCCESS )
	{
        sd_assert( FALSE );
		destrory_k_bucket( &p_k_bucket->_sub[0] );
		destrory_k_bucket( &p_k_bucket->_sub[1] );
		return FALSE;	
	}
	LOG_DEBUG( "kb_split create_sub_bucket end, p_k_bucket:0x%x, sub_bucket_0:0x%x, sub_bucket_1:0x%x", 
		p_k_bucket, p_k_bucket_0, p_k_bucket_1 );
    
	cur_node = LIST_BEGIN( p_k_bucket->_k_node_list );
	while( cur_node != LIST_END( p_k_bucket->_k_node_list ) )
	{
		p_node = (K_NODE *)LIST_VALUE( cur_node );
		ret_val = kb_add_node( p_k_bucket, p_node );
		if( ret_val != SUCCESS )
		{
			node_destoryer( p_node );
		}
		cur_node = LIST_NEXT( cur_node );
	}
	list_clear( &p_k_bucket->_k_node_list );
	
	LOG_DEBUG( "kb_split end!!!!, p_k_bucket:0x%x, sub_bucket_0:0x%x, sub_bucket_1:0x%x", 
		p_k_bucket, p_k_bucket_0, p_k_bucket_1 );
	return TRUE;
}

BOOL kb_is_leaf( K_BUCKET *p_k_bucket )
{
	BOOL ret_bool = FALSE;
	if( p_k_bucket->_sub[0] == NULL
		&& p_k_bucket->_sub[1] == NULL )
	{
		ret_bool = TRUE;
	}
	
	//LOG_DEBUG( "kb_is_leaf, p_k_bucket:0x%x, is_leaf:%d", p_k_bucket, ret_bool );

	return ret_bool;
}

BOOL kb_is_empty( K_BUCKET *p_k_bucket )
{
	BOOL ret_bool = FALSE;
	
	sd_assert( list_size( &p_k_bucket->_k_node_list) <= p_k_bucket->_bucket_para_ptr->_k ); 

	if( list_size( &p_k_bucket->_k_node_list) == 0 )
	{
		ret_bool = TRUE;
	}
	LOG_DEBUG( "kb_is_empty, p_k_bucket:0x%x, is_empty:%d", p_k_bucket, ret_bool );
	
	return ret_bool;
}

BOOL kb_is_full( K_BUCKET *p_k_bucket )
{
	BOOL ret_bool = FALSE;

	sd_assert( list_size( &p_k_bucket->_k_node_list) <= p_k_bucket->_bucket_para_ptr->_k ); 

	if( list_size( &p_k_bucket->_k_node_list) >= p_k_bucket->_bucket_para_ptr->_k )
	{
		ret_bool = TRUE;
	}
	LOG_DEBUG( "kb_is_full, p_k_bucket:0x%x, is_full:%d, node_list_size:%u", 
        p_k_bucket, ret_bool, list_size( &p_k_bucket->_k_node_list) );
	return ret_bool;
}

_int32 kb_get_level( K_BUCKET *p_k_bucket, _u32 *p_level )
{
	_u32 valid_num = bitmap_bit_count( &p_k_bucket->_bucket_index._bit_map );
	if( valid_num == 0 )
	{
		sd_assert( FALSE );
		return -1;
	}
	*p_level = valid_num-1;
	
	//LOG_DEBUG( "kb_get_level, p_k_bucket:0x%x, level:%d", p_k_bucket, *p_level );
	return SUCCESS;
}


//k_bucket malloc/free
_int32 init_k_bucket_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_k_bucket_slab == NULL )
	{              
		ret_val = mpool_create_slab( sizeof( K_BUCKET ), MIN_K_BUCKET, 0, &g_k_bucket_slab );
		CHECK_VALUE( ret_val );
	}

	return ret_val;
}

_int32 uninit_k_bucket_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_k_bucket_slab != NULL )
	{              
		ret_val = mpool_destory_slab( g_k_bucket_slab );
		CHECK_VALUE( ret_val );
		g_k_bucket_slab = NULL;
	}
	return ret_val;
}

_int32 k_bucket_malloc_wrap(K_BUCKET **pp_node)
{
	return mpool_get_slip( g_k_bucket_slab, (void**)pp_node );
}

_int32 k_bucket_free_wrap(K_BUCKET *p_node)
{
	sd_assert( p_node != NULL );
	if( p_node == NULL ) return SUCCESS;
	return mpool_free_slip( g_k_bucket_slab, (void*)p_node );
}

#ifdef _LOGGER
_int32 k_bucket_print_node_info( K_BUCKET *p_k_bucket )
{
    _int32 ret_val = SUCCESS;
	LIST_ITERATOR cur_node = NULL;
    K_NODE *p_k_node = NULL;
    char addr[24] = {0};
    K_DISTANCE *p_distance = NULL;
    
    if( kb_is_leaf(p_k_bucket) )
    {
        LOG_DEBUG( "k_bucket_print_node_info, leaf p_k_bucket print index:0x%x", p_k_bucket );
        k_distance_print( &p_k_bucket->_bucket_index );
        cur_node = LIST_BEGIN( p_k_bucket->_k_node_list );
        while( cur_node != LIST_END( p_k_bucket->_k_node_list) )
        {
            p_k_node = (K_NODE *)LIST_VALUE( cur_node );
            sd_inet_ntoa(p_k_node->_peer_addr._ip, addr, 24);

            ret_val = k_distance_create( &p_distance );
            CHECK_VALUE( ret_val ); 
            ret_val = rt_distance_calc( p_k_bucket->_bucket_para_ptr->_dk_type, &p_k_node->_node_id, p_distance );
            CHECK_VALUE( ret_val ); 
            
            LOG_DEBUG( "k_bucket_print_node_info,leaf p_k_bucket:0x%x, ip:%s, port:%d, print distance", p_k_bucket, addr, p_k_node->_peer_addr._port );
            
            k_distance_print( p_distance );

            cur_node = LIST_NEXT( cur_node );
        }
    }
    else
    {
        k_bucket_print_node_info(p_k_bucket->_sub[0]);
        LOG_DEBUG( "k_bucket_print_node_info, parrent p_k_bucket :0x%x, print index:", p_k_bucket );
        k_distance_print( &p_k_bucket->_bucket_index );       
        k_bucket_print_node_info(p_k_bucket->_sub[1]);
    }
    return SUCCESS;
}
#endif


#endif

