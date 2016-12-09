/*****************************************************************************
 *
 * Filename: 			find_node_handler.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2009/10/15
 *	
 * Purpose:				Define the basic structs of find_node_handler.
 *
 *****************************************************************************/

#if !defined(__find_node_handler_20091015)
#define __find_node_handler_20091015

#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"
#ifdef _DK_QUERY

#include "res_query/dk_res_query/dk_protocal_handler.h"
#include "res_query/dk_res_query/dk_protocal_builder.h"

typedef _int32 (*find_node_notify)( K_NODE *p_k_node );

#define DHT_COMPACT_ADDR_SIZE 6
#define DHT_COMPACT_NODE_SIZE ( DHT_ID_LEN+DHT_COMPACT_ADDR_SIZE )

typedef enum tagFIND_NODE_STATUS
{
	FIND_NODE_INIT,
	FIND_NODE_RUNNING,
	FIND_NODE_FINISHED
} FIND_NODE_STATUS;

typedef struct tagFIND_NODE_HANDLER
{
	DK_PROTOCAL_HANDLER _protocal_handler;
	find_node_cmd_builder _cmd_builder;
	K_NODE_ID _target;
	LIST _unqueryed_node_info_list;//NODE_INFO
	SET _near_node_key;
	K_DISTANCE *_nearst_dist_ptr;
	K_DISTANCE *_middle_dist_ptr;
    _u32 _near_node_count;
#ifdef _LOGGER
    _u32 _nearst_ip;
    _u8 *_near_id_ptr;
#endif  

	FIND_NODE_STATUS _status;
	node_info_creater _node_creater;
	node_info_destoryer _node_destoryer;
	_u32 _dk_type;
	_u32 _find_type;//此参数对于dht网络无用
	_u32 _resp_node_num;
	_u32 _idle_ticks;
    _u32 _get_nearest_node_max_num;
    _u32 _filter_low_limit;
    


}FIND_NODE_HANDLER;

_int32 fnh_create( FIND_NODE_HANDLER **pp_find_node_handler );
_int32 fnh_init( _u32 dk_type, FIND_NODE_HANDLER *p_find_node_handler, _u8 *p_target, _u32 target_len, _u8 find_type );
_int32 fnh_uninit( FIND_NODE_HANDLER *p_find_node_handler );
_int32 fnh_destory( FIND_NODE_HANDLER *p_find_node_handler );

void find_node_set_build_cmd_handler( FIND_NODE_HANDLER *p_find_node_handler, find_node_cmd_builder cmd_builder);

void find_node_set_response_handler( FIND_NODE_HANDLER *p_find_node_handler, dk_protocal_response_handler resp_handler);
_int32 dht_find_node_response_handler( DK_PROTOCAL_HANDLER *p_handler, _u32 ip, _u32 port, _u32 invalid_para, void *p_data );
_int32 kad_find_node_response_handler( DK_PROTOCAL_HANDLER *p_handler, _u32 ip, _u32 port, _u32 version, void *p_data );
_int32 fnh_handler_new_peer( FIND_NODE_HANDLER *p_find_node_handler,
	_u32 ip, _u16 port, _u8 version, _u8 *p_id, _u32 id_len );

void fnh_update( FIND_NODE_HANDLER *p_find_node_handler );
_int32 fnh_find_target_from_rt( FIND_NODE_HANDLER *p_find_node_handler );
_int32 fnh_add_node_with_id( FIND_NODE_HANDLER *p_find_node_handler, _u32 ip, _u16 port, _u8 version, _u8 *p_id, _u32 id_len );
_int32 fnh_add_node( FIND_NODE_HANDLER *p_find_node_handler, _u32 ip, _u16 port, _u8 version, _u8 *p_id, _u32 id_len );

void fnh_find( FIND_NODE_HANDLER *p_find_node_handler );

FIND_NODE_STATUS fnh_get_status( FIND_NODE_HANDLER *p_find_node_handler );
_int32 find_node_remove_node_info( FIND_NODE_HANDLER *p_find_node_handler, _u32 net_ip, _u16 host_port );
void find_node_clear_idle_ticks( FIND_NODE_HANDLER *p_find_node_handler );


//find_node_handler malloc/free wrap
_int32 init_find_node_handler_slabs(void);
_int32 uninit_find_node_handler_slabs(void);
_int32 find_node_handler_malloc_wrap(FIND_NODE_HANDLER **pp_node);
_int32 find_node_handler_free_wrap(FIND_NODE_HANDLER *p_node);

_int32 find_node_set_compare( void *E1, void *E2 );
#endif

#ifdef __cplusplus
}
#endif

#endif // !defined(__find_node_handler_20091015)







