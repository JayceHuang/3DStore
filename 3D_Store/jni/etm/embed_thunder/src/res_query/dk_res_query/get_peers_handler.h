/*****************************************************************************
 *
 * Filename: 			get_peers_handler.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2009/10/15
 *	
 * Purpose:				Define the basic structs of get_peers_handler.
 *
 *****************************************************************************/


#if !defined(__get_peers_handler_20091015)
#define __get_peers_handler_20091015

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#ifdef _DK_QUERY
#include "res_query/dk_res_query/dk_protocal_handler.h"
#include "res_query/dk_res_query/find_node_handler.h"


typedef struct tagGET_PEERS_HANDLER
{
	FIND_NODE_HANDLER _find_node;
    void *_user_ptr;
}GET_PEERS_HANDLER;

_int32 get_peers_init( GET_PEERS_HANDLER *p_get_peers_handler, _u8 *p_target, _u32 target_len, void *p_user );
_int32 get_peers_update( GET_PEERS_HANDLER *p_get_peers_handler );
_int32 get_peers_uninit( GET_PEERS_HANDLER *p_get_peers_handler );

_int32 get_peers_response_handler( DK_PROTOCAL_HANDLER *p_handler, _u32 ip, _u32 port, _u32 invalid_para, void *p_data );
_int32 get_peers_announce( GET_PEERS_HANDLER *p_get_peers_handler, _u32 ip, _u16 port, char *p_token, _u32 token_len );
FIND_NODE_STATUS get_peers_get_status( GET_PEERS_HANDLER *p_get_peers_handler );
_int32 get_peers_add_node( GET_PEERS_HANDLER *p_get_peers_handler, _u32 ip, _u16 port );

#endif

#ifdef __cplusplus
}
#endif

#endif // !defined(__get_peers_handler_20091015)








