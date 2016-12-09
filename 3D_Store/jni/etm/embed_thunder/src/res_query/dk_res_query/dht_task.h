/*****************************************************************************
 *
 * Filename: 			dht_task.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2009/10/15
 *	
 * Purpose:				Define the basic structs of dht_task.
 *
 *****************************************************************************/


#if !defined(__dht_task_20091015)
#define __dht_task_20091015

#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"
#ifdef _DK_QUERY

#include "res_query/dk_res_query/dk_task.h"
#include "res_query/dk_res_query/find_node_handler.h"
#include "res_query/dk_res_query/get_peers_handler.h"
#include "res_query/dk_res_query/announce_peer_handler.h"

typedef struct tagDHT_TASK
{
	DK_TASK _dk_task;
	GET_PEERS_HANDLER _get_peers_handler;
}DHT_TASK;

_int32 dht_task_logic_create( DK_TASK **pp_dk_task );
_int32 dht_task_logic_init( DK_TASK *p_dk_task );
_int32 dht_task_logic_update( DK_TASK *p_dk_task );
_int32 dht_task_logic_uninit( DK_TASK *p_dk_task );
_int32 dht_task_logic_destory( DK_TASK *p_dk_task );
_int32 dht_add_node( DHT_TASK *p_dk_task, _u32 ip, _u16 port );
#endif

#ifdef __cplusplus
}
#endif

#endif // !defined(__dht_task_20091015)



