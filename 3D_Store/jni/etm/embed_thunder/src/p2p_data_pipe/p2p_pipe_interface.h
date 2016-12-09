/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2008/07/02
-----------------------------------------------------------------------------------------------------------*/

#ifndef _P2P_DATA_PIPE_INTERFACE_
#define _P2P_DATA_PIPE_INTERFACE_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/range.h"

#include "connect_manager/resource.h"

struct tagRESOURCE;
struct tagP2P_DATA_PIPE;
struct tagDATA_PIPE;
struct _tag_data_manager;
enum   tagPIPE_STATE;
struct tagPEER_PIPE_INFO;
struct tagPTL_DEVICE;
struct tagP2P_RESOURCE;

_int32 init_p2p_module(void);

_int32 uninit_p2p_module(void);

_int32 p2p_resource_create(RESOURCE** resource, char *peer_id, _u8 *gcid
    , _u64 file_size, _u32 peer_capability, _u32 ip, _u16 tcp_port
    , _u16 udp_port, _u8 res_level, _u8 res_from, _u8 res_priority);

_u8 p2p_get_res_level(struct tagRESOURCE* res);

_u8 p2p_get_res_priority(RESOURCE* res);

BOOL compare_peer_resource(struct tagP2P_RESOURCE *res1, struct tagP2P_RESOURCE *res2);

BOOL p2p_is_ever_unchoke( struct tagDATA_PIPE* pipe);

_int32 p2p_resource_destroy(struct tagRESOURCE** resource);

_int32 p2p_pipe_create(struct _tag_data_manager* data_manager, struct tagRESOURCE* peer_resource, struct tagDATA_PIPE** pipe, struct tagPTL_DEVICE* device);

/*
function : open the pipe, try to connect peer
para	 : [in]pipe -- pipe pointer which have been create
return	 : 0 -- success;	nonzero -- error
*/
_int32 p2p_pipe_open(struct tagDATA_PIPE* pipe);

_int32 p2p_pipe_close(struct tagDATA_PIPE* pipe);

_int32 p2p_pipe_change_ranges(struct tagDATA_PIPE* pipe, const RANGE* range, BOOL cancel_flag);

_int32 p2p_pipe_get_speed(struct tagDATA_PIPE* pipe);

BOOL p2p_resource_copy_lack_info( RESOURCE* dest, RESOURCE* src );

BOOL get_peer_hash_value(char* peerid, _u32 peerid_len, _u32 ip, _u16 port, _u32* hash_value);

BOOL p2p_is_cdn_resource(struct tagRESOURCE* res);

_u32 p2p_get_capability( RESOURCE* res );

BOOL p2p_is_support_mhxy( struct tagDATA_PIPE* pipe );

_u8 p2p_get_from( RESOURCE* res );

#ifdef UPLOAD_ENABLE
_int32 p2p_pipe_choke_remote(struct tagDATA_PIPE* pipe, BOOL choke);

_int32 p2p_pipe_get_upload_speed(struct tagDATA_PIPE* pipe, _u32* upload_speed);

//底层通知某块数据已经校验成功，这个时候如果是边下边传应该发送interested_resp
_int32 p2p_pipe_notify_range_checked(struct tagDATA_PIPE* pipe);
#endif

#ifdef _CONNECT_DETAIL

BOOL p2p_pipe_is_transfer_by_tcp(struct tagDATA_PIPE* pipe);

_int32 p2p_pipe_get_peer_pipe_info(struct tagDATA_PIPE* pipe, struct tagPEER_PIPE_INFO* info);

#endif
#ifdef __cplusplus
}
#endif
#endif



