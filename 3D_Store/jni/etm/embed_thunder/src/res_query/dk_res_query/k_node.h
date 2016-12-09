/*****************************************************************************
 *
 * Filename: 			k_distance.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2009/10/15
 *	
 * Purpose:				Define the basic structs of k_distance and k_node.
 *
 *****************************************************************************/


#if !defined(__K_NODE_20091015)
#define __K_NODE_20091015

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#ifdef _DK_QUERY
#include "utility/bitmap.h"

#include "torrent_parser/torrent_parser_struct_define.h"

_int32 k_node_module_init( void );
_int32 k_node_module_uninit( void );

_int32 k_distance_create( K_DISTANCE **pp_k_distance );
_int32 k_distance_create_with_char_buffer( _u8 *p_buffer, _u32 buffer_len, K_DISTANCE **pp_k_distance );

_int32 k_distance_init( K_DISTANCE *p_k_distance, _u32 bit_count );
_int32 k_distance_init_with_char_buffer( K_DISTANCE *p_k_distance, _u8 *p_buffer, _u32 buffer_len );
_int32 k_distance_child_copy_construct( const K_DISTANCE *p_src_dist, K_DISTANCE *p_dist_0, K_DISTANCE *p_dist_1 );
_int32 k_distance_copy_construct( const K_DISTANCE *p_src_dist, K_DISTANCE *p_dist_dest );
_int32 k_distance_set_bit( K_DISTANCE *p_dist, _u32 bit_index, BOOL value );

_int32 k_distance_xor( const K_DISTANCE *p_k_dist_1, const K_DISTANCE *p_k_dist_2, K_DISTANCE *p_dist_ret );

_int32 k_distance_uninit( K_DISTANCE *p_k_distance );
_int32 k_distance_destory( K_DISTANCE **pp_k_distance );
_u64 k_distance_get_value( K_DISTANCE *p_k_distance );
BOOL k_distance_get_bool( const K_DISTANCE *p_k_distance, _u32 pos );
_u8 *k_distance_get_bit_buffer( K_DISTANCE *p_k_distance );
_u32 k_distance_buffer_len( K_DISTANCE *p_k_distance );
_int32 k_distance_compare( K_DISTANCE *p_k_distance_1, K_DISTANCE *p_k_distance_2, _int32 *p_result );
void k_distance_print( const K_DISTANCE *p_k_distance );

_int32 k_node_create( _u8 *p_node_id_buffer,_u32 id_len, _u32 ip, _u16 port, K_NODE **pp_k_node );
_int32 k_node_destory( K_NODE *p_k_node );

const K_NODE_ID* k_node_get_node_id( const K_NODE *p_k_node );
BOOL k_node_is_equal( const K_NODE *p_k_node_1, const K_NODE *p_k_node_2, _u32 pos );
BOOL k_node_is_old( K_NODE *p_k_node );
BOOL k_node_is_abandon( K_NODE *p_k_node );

_int32 k_node_generate_id( _u8 *buffer, _int32 size );

//k_node malloc/free
_int32 init_k_node_slabs(void);
_int32 uninit_k_node_slabs(void);
_int32 k_node_malloc_wrap(K_NODE **pp_node);
_int32 k_node_free_wrap(K_NODE *p_node);



_int32 kad_node_create( _u8 *p_node_id_buffer,_u32 id_len, _u32 ip, _u16 port, _u16 tcp_port, _u8 version, KAD_NODE **pp_kad_node  );
_int32 kad_node_destory( K_NODE *p_k_node );

k_node_destory_handler k_node_get_destoryer( _u32 dk_type );

//kad_node malloc/free
_int32 init_kad_node_slabs(void);
_int32 uninit_kad_node_slabs(void);
_int32 kad_node_malloc_wrap(KAD_NODE **pp_node);
_int32 kad_node_free_wrap(KAD_NODE *p_node);


//private func

_int32 dht_node_info_creater( _u32 ip, _u16 port, _u8 version, _u8 *p_id, _u32 id_len, NODE_INFO **pp_node_info );
_int32 kad_node_info_creater( _u32 ip, _u16 port, _u8 version, _u8 *p_id, _u32 id_len, NODE_INFO **pp_node_info );

_int32 dht_node_info_destoryer( NODE_INFO *p_node_info );
_int32 kad_node_info_destoryer( NODE_INFO *p_node_info );


//node_info malloc/free wrap
_int32 init_node_info_slabs(void);
_int32 uninit_node_info_slabs(void);
_int32 node_info_malloc_wrap(NODE_INFO **pp_node);
_int32 node_info_free_wrap(NODE_INFO *p_node);


//kad_node_info malloc/free wrap
_int32 init_kad_node_info_slabs(void);
_int32 uninit_kad_node_info_slabs(void);
_int32 kad_node_info_malloc_wrap(KAD_NODE_INFO **pp_node);
_int32 kad_node_info_free_wrap(KAD_NODE_INFO *p_node);

_u32 dk_calc_key( _u32 ip, _u16 port, _u8 *p_id, _u32 id_len );

#endif


#ifdef __cplusplus
}
#endif

#endif // !defined(__K_NODE_20091015)





