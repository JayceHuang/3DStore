#if !defined(__announce_peer_handler_20091015)
#define __announce_peer_handler_20091015

#ifdef __cplusplus
extern "C" 
{
#endif

#include "res_query/dk_res_query/dk_protocal_handler.h"
#include "bt_download/torrent_parser/bencode.h"


typedef struct tagANNOUNCE_PEER_HANDLER
{
	DK_PROTOCAL_HANDLER _protocal_handler;
	struct tagDK_TASK *_task_ptr;
}ANNOUNCE_PEER_HANDLER;

_int32 announce_peer_init( ANNOUNCE_PEER_HANDLER *p_announce_peers_handler );
_int32 announce_peer_response_handler( struct tagDK_PROTOCAL_HANDLER *p_handler, _u32 ip, _u16 port, BC_DICT *p_dict_data );
_int32 announce_peer_update( ANNOUNCE_PEER_HANDLER *p_announce_peers_handler );
_int32 announce_peer_send_query( ANNOUNCE_PEER_HANDLER *p_announce_peers_handler, _u32 ip, _u16 port );


#ifdef __cplusplus
}
#endif

#endif // !defined(__announce_peer_handler_20091015)








