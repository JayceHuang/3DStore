#include "utility/define.h"
#ifdef _DK_QUERY

#include "dht_task.h"
#include "dk_socket_handler.h"
#include "get_peers_handler.h"
#include "dk_protocal_builder.h"
#include "utility/mempool.h"
#include "utility/utility.h"

#include "utility/logid.h"
#define LOGID LOGID_DK_QUERY
#include "utility/logger.h"

_int32 dht_task_logic_create( DK_TASK **pp_dk_task )
{
	return sd_malloc( sizeof( DHT_TASK ), (void **)pp_dk_task );
}

_int32 dht_task_logic_init( DK_TASK *p_dk_task )
{
    _int32 ret_val = SUCCESS;
	DHT_TASK *p_dht_task = (DHT_TASK *)p_dk_task;
	GET_PEERS_HANDLER *p_get_peer_handler = &p_dht_task->_get_peers_handler;
    _u8 *p_target = NULL;
    _u32 target_len = 0;
    
	ret_val = dk_get_target( &p_dht_task->_dk_task, &p_target, &target_len );
	CHECK_VALUE( ret_val );
    
 	LOG_DEBUG( "dht_task_logic_init, task_ptr:0x%x", p_dk_task );
	return get_peers_init( p_get_peer_handler, p_target, target_len, p_dk_task->_user_ptr );
}

_int32 dht_task_logic_update( DK_TASK *p_dk_task )
{
	DHT_TASK *p_dht_task = (DHT_TASK *)p_dk_task;
	GET_PEERS_HANDLER *p_get_peer_handler = &p_dht_task->_get_peers_handler;
	
 	LOG_DEBUG( "dht_task_logic_update, task_ptr:0x%x", p_dk_task );
	get_peers_update( p_get_peer_handler );	
	if( get_peers_get_status( p_get_peer_handler) == FIND_NODE_FINISHED )
	{
		dk_set_task_status( p_dk_task, DK_TASK_FINISHED);
	}
	return SUCCESS;
}

_int32 dht_task_logic_uninit( DK_TASK *p_dk_task )
{
	DHT_TASK *p_dht_task = (DHT_TASK *)p_dk_task;
	GET_PEERS_HANDLER *p_get_peer_handler = &p_dht_task->_get_peers_handler;
	
 	LOG_DEBUG( "dht_task_logic_uninit, task_ptr:0x%x", p_dk_task );

	return get_peers_uninit( p_get_peer_handler );	
}

_int32 dht_task_logic_destory( DK_TASK *p_dk_task )
{
	DHT_TASK *p_dht_task = (DHT_TASK *)p_dk_task;
    
 	LOG_DEBUG( "dht_task_logic_destory, task_ptr:0x%x", p_dk_task );
	SAFE_DELETE( p_dht_task );
	return SUCCESS;
}

_int32 dht_add_node( DHT_TASK *p_dk_task, _u32 ip, _u16 port )
{
    return get_peers_add_node( &p_dk_task->_get_peers_handler, ip, port );
}

#endif

