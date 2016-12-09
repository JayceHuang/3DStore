/*****************************************************************************
 *
 * Filename: 			routing_table.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2009/10/15
 *	
 * Purpose:				Define the basic structs of routing_table.
 *
 *****************************************************************************/


#if !defined(__ROUTING_TABLE_20091015)
#define __ROUTING_TABLE_20091015

#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"
#ifdef _DK_QUERY

#include "res_query/dk_res_query/k_bucket.h"
#include "res_query/dk_res_query/ping_handler.h"

typedef enum tagROUTING_TABLE_STATUS
{
    RT_IDLE,
	RT_INIT,
	RT_RUNNING
}ROUTING_TABLE_STATUS;

typedef struct tagROUTING_TABLE
{		
	K_BUCKET *_root_ptr;
	K_BUCKET_PARA _bucket_para;
	K_NODE_ID *_local_node_id_ptr;
	PING_HANDLER _ping_handler;
	_u32 _time_ticks;
	_u32 _ping_root_nodes_times;
	LIST _find_node_list;
	_u32 _dk_type;
	ROUTING_TABLE_STATUS _status;
}ROUTING_TABLE;

/*public function */

_int32 dk_routing_table_create( _u32 dk_type );
_int32 dk_routing_table_destory( _u32 dk_type );

//调用此函数时,返回指针必然不会为NULL
ROUTING_TABLE *rt_ptr( _u32 dk_type );

void rt_update( ROUTING_TABLE *p_routing_table );
void rt_update_ping_handler( ROUTING_TABLE *p_routing_table );
void rt_handle_root_node( ROUTING_TABLE *p_routing_table );

_int32 rt_find_myself( ROUTING_TABLE *p_routing_table );
void rt_update_find_node_list( ROUTING_TABLE *p_routing_table );
_int32 rt_add_find_node_list( ROUTING_TABLE *p_routing_table );
BOOL rt_is_target_exist( ROUTING_TABLE *p_routing_table, K_NODE_ID *p_target );

_int32 rt_add_rout_node( ROUTING_TABLE *p_routing_table, K_NODE *p_node );


/*返回路由表中节点总数*/
//_u32 routing_table_size( ROUTING_TABLE *p_routing_table );

/*找到指定数据应该存放的桶*/
//_int32 rt_find_bucket( ROUTING_TABLE *p_routing_table, const K_NODE *p_k_node, K_BUCKET *p_k_bucket );	



/*取得最靠近指定节点的n个节点*/
_int32 rt_get_nearest_nodes( ROUTING_TABLE *p_routing_table, const K_NODE_ID *p_k_node_id, LIST *p_node_list, _u32 *p_node_num );	
	

/*根据节点ID查找节点*/
//_int32 rt_find_node( ROUTING_TABLE *p_routing_table, K_NODE_ID node_id, K_NODE *p_node );

//_int32 rt_get_distance( ROUTING_TABLE *p_routing_table, const K_NODE *p_node, K_DISTANCE *p_distance );

K_NODE_ID *rt_get_local_id( ROUTING_TABLE *p_routing_table );


//_int32 rt_response_handler( struct tagDK_PROTOCAL_HANDLER *p_handler, _u32 ip, _u16 port, BC_DICT *p_dict_data );
//_int32 rt_timeout_handler( struct tagDK_PROTOCAL_HANDLER *p_handler );

/*private function */

_int32 rt_load_from_cfg( ROUTING_TABLE *p_routing_table );
_int32 rt_save_to_cfg( ROUTING_TABLE *p_routing_table );
_int32 rt_ping_node( _u32 dk_type, _u32 ip, _u16 port, _u8 version, BOOL is_force_ping );

_int32 rt_distance_calc( _u32 dk_type, const K_NODE_ID *p_k_node_id, K_DISTANCE *p_k_distance );
BOOL rt_is_ready( _u32 dk_type );

#endif

#ifdef __cplusplus
}
#endif

#endif // !defined(__ROUTING_TABLE_20091015)





