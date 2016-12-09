/*****************************************************************************
 *
 * Filename: 			ping_handler.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2009/10/15
 *	
 * Purpose:				Define the basic structs of ping_handler.
 *
 *****************************************************************************/


#if !defined(__ping_handler_20091015)
#define __ping_handler_20091015

#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"
#ifdef _DK_QUERY

#include "res_query/dk_res_query/k_node.h"
#include "res_query/dk_res_query/dk_protocal_builder.h"


typedef struct tagPING_HANDLER
{
	//DK_PROTOCAL_HANDLER _protocal_handler;
	LIST _wait_to_ping_list;
	node_info_creater _node_creater;
	node_info_destoryer _node_destoryer;
	ping_cmd_builder _cmd_builder;
	_u32 _dk_type;
}PING_HANDLER;

_int32 ping_handler_init( PING_HANDLER *p_ping_handler, _u32 dk_type );
_int32 ping_handler_uninit( PING_HANDLER *p_ping_handler );

_int32 ph_ping_node( PING_HANDLER *p_ping_handler, _u32 ip, _u16 port, _u8 version );

_int32 ph_ping_dht_boot_host( PING_HANDLER *p_ping_handler );
_int32 ph_update( PING_HANDLER *p_ping_handler );
#endif


#ifdef __cplusplus
}
#endif

#endif // !defined(__ping_handler_20091015)









