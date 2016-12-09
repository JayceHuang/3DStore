/*****************************************************************************
 *
 * Filename: 			k_bucket.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2009/10/15
 *	
 * Purpose:				Define the basic structs of k_bucket.
 *
 *****************************************************************************/


#if !defined(__K_BUCKET_20091015)
#define __K_BUCKET_20091015

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#ifdef _DK_QUERY
#include "res_query/dk_res_query/k_node.h"
#include "utility/list.h"

typedef struct tagK_BUCKET_PARA
{
	_u32 _max_level;		// 最大允许裂变层数
	_u32 _min_level;		// 最小允许裂变层数
	_u32 _k;			// K桶裂变临界值
	_u32 _can_split_max_distance; //裂变时桶离主树最大距离
	_u32 _dk_type;
//	k_distance_split_handler _split_handler;
}K_BUCKET_PARA;

/*
typedef struct tagK_INDEX
{
	K_DISTANCE _distance;
	_u32 _valid_num; //valid_num-1 为桶所在层 _valid_num范围:( KAD:[1,128] dht:[1,160] )
}K_INDEX;
*/

struct tagK_BUCKET;

typedef struct tagK_BUCKET
{
	struct tagK_BUCKET *_parent;
	struct tagK_BUCKET *_sub[2];

	LIST _k_node_list;          //K_NODE 类型节点

	K_DISTANCE _bucket_index;	
	K_BUCKET_PARA *_bucket_para_ptr;
}K_BUCKET;

//public func
_int32 create_k_bucket_without_index( K_BUCKET *p_parent, K_BUCKET_PARA *p_bucket_para, K_BUCKET **pp_k_bucket );
_int32 destrory_k_bucket( K_BUCKET **pp_k_bucket );

_int32 kb_find_bucket_by_node( K_BUCKET *p_k_bucket, K_NODE *p_k_node, K_BUCKET **pp_k_target_bucket );
_int32 kb_find_bucket_by_key( K_BUCKET *p_k_bucket, K_DISTANCE *p_k_distance, K_BUCKET **pp_k_target_bucket );

_int32 kb_add_node( K_BUCKET *p_k_bucket, K_NODE *p_k_node );
_int32 kb_get_old_node_list( K_BUCKET *p_k_bucket, _u32 max_num, LIST *p_old_node_list );
_int32 kb_get_bucket_list( K_BUCKET *p_k_bucket, _u32 *p_max_num, BOOL is_empty, LIST *p_empty_bucket_list );
_int32 kb_get_bucket_find_node_id( K_BUCKET *p_k_bucket, K_NODE_ID *p_k_node );
_int32 kb_get_nearst_node( K_BUCKET *p_k_bucket, const K_NODE_ID *p_k_node_id, LIST *p_node_list, _u32 *p_node_num );
_int32 kb_get_nearst_node_by_key( K_BUCKET *p_k_bucket, const K_DISTANCE *p_key, 
	LIST *p_node_list, _u32 *p_node_num );
_u32 kb_get_node_num( K_BUCKET *p_k_bucket );
_u32 bucket_para_get_dk_type( K_BUCKET_PARA *p_bucket_para );

//private func

void kb_leaf_delete_node( K_BUCKET *p_k_bucket, K_NODE *p_k_node, BOOL *p_is_delete );
BOOL kb_split( K_BUCKET *p_k_bucket );
BOOL kb_is_leaf( K_BUCKET *p_k_bucket );
BOOL kb_is_empty( K_BUCKET *p_k_bucket );
BOOL kb_is_full( K_BUCKET *p_k_bucket );
_int32 kb_get_level( K_BUCKET *p_k_bucket, _u32 *p_level );


//k_bucket malloc/free
_int32 init_k_bucket_slabs(void);
_int32 uninit_k_bucket_slabs(void);
_int32 k_bucket_malloc_wrap(K_BUCKET **pp_node);
_int32 k_bucket_free_wrap(K_BUCKET *p_node);

#ifdef _LOGGER
_int32 k_bucket_print_node_info( K_BUCKET *p_k_bucket );
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif // !defined(__K_BUCKET_20091015)




