/*****************************************************************************
 *
 * Filename: 			find_source_handler.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2009/10/15
 *	
 * Purpose:				Define the basic structs of find_source_handler.
 *
 *****************************************************************************/

#if !defined(__find_source_handler_20091015)
#define __find_source_handler_20091015

#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"
#ifdef _DK_QUERY
#ifdef ENABLE_EMULE


#include "res_query/dk_res_query/dk_protocal_handler.h"
#include "res_query/dk_res_query/find_node_handler.h"

struct tagKAD_TASK;
struct tagEMULE_TAG_LIST;

typedef struct tagFIND_SOURCE_NOTIFY
{
	DK_PROTOCAL_HANDLER _protocal_handler;
    void *_user_ptr;
    struct tagFIND_SOURCE_HANDLER *_find_source_ptr;
} FIND_SOURCE_NOTIFY;

typedef struct tagFIND_SOURCE_HANDLER
{
	FIND_NODE_HANDLER _find_node;
	//DK_PROTOCAL_HANDLER _protocal_handler;
    FIND_SOURCE_NOTIFY _res_notify;
	LIST _unqueryed_node_info_list;//NODE_INFO
    //void *_user_ptr;
    _u64 _file_size;
}FIND_SOURCE_HANDLER;

_int32 find_source_init( FIND_SOURCE_HANDLER *p_find_source_handler, _u8 *p_target, _u32 target_len, void *p_user );
_int32 find_source_update( FIND_SOURCE_HANDLER *p_find_source_handler );
_int32 find_source_uninit( FIND_SOURCE_HANDLER *p_find_source_handler );

_int32 find_node_wrap_handler( DK_PROTOCAL_HANDLER *p_handler, _u32 ip, _u32 port, _u32 version, void *p_data );
_int32 find_source_res_notify_handler( DK_PROTOCAL_HANDLER *p_handler, _u32 ip, _u32 port, _u32 id_para, void *p_data );
FIND_NODE_STATUS find_source_get_status( FIND_SOURCE_HANDLER *p_find_source_handler );
void find_source_set_file_size( FIND_SOURCE_HANDLER *p_find_source_handler, _u64 file_size );
_int32 find_source_find( FIND_SOURCE_HANDLER *p_find_source_handler );
_int32 find_source_add_node( FIND_SOURCE_HANDLER *p_find_source_handler, _u32 ip, _u16 port, _u8 version, _u8 *p_id, _u32 id_len  );
_int32 find_source_remove_node_info( FIND_SOURCE_HANDLER *p_find_source_handler, _u32 net_ip, _u16 host_port );
_int32 find_source_fill_announce_tag_list( FIND_SOURCE_HANDLER *p_find_source_handler, struct tagEMULE_TAG_LIST *p_tag_list );
#endif
#endif


#ifdef __cplusplus
}
#endif

#endif // !defined(__find_source_handler_20091015)









