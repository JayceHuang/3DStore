

/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2009/02/21
-----------------------------------------------------------------------------------------------------------*/
#ifndef _BT_PIPE_INTERFACE_
#define _BT_PIPE_INTERFACE_

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#ifdef ENABLE_BT 

struct tagRESOURCE;
struct tagBT_RESOURCE;
struct tagDATA_PIPE;
struct _tag_range;
struct tagPEER_PIPE_INFO;
struct tagBT_DATA_PIPE;

_int32 init_bt_data_pipe(void);

_int32 uninit_bt_data_pipe(void);

#ifdef ENABLE_BT_PROTOCOL

_int32 bt_resource_create(struct tagRESOURCE** resource, _u32 ip, _u16 port);

_int32 bt_resource_destroy(struct tagRESOURCE** resource);

_int32 bt_pipe_create(void* data_manager, struct tagRESOURCE* res, struct tagDATA_PIPE** pipe);

_int32 bt_pipe_open(struct tagDATA_PIPE* pipe);

_int32 bt_pipe_close(struct tagDATA_PIPE* pipe);

_int32 bt_pipe_set_magnet_mode(DATA_PIPE* pipe);

//这个range是加了pending
_int32 bt_pipe_change_ranges(struct tagDATA_PIPE* pipe, const struct _tag_range* range, BOOL cancel_flag);

_int32 bt_pipe_get_speed(struct tagDATA_PIPE* pipe);

BOOL bt_pipe_is_ever_unchoke( struct tagDATA_PIPE* pipe);

_int32 bt_pipe_notify_have_piece(struct tagDATA_PIPE* pipe, _u32 piece_index);

/*-----------------------------------*/
_int32 bt_pipe_update_can_download_range(struct tagDATA_PIPE*  pipe);

#ifdef UPLOAD_ENABLE
_int32 bt_pipe_stop_upload(struct tagBT_DATA_PIPE* bt_pipe);

_int32 bt_pipe_get_upload_speed(struct tagDATA_PIPE* pipe, _u32* speed);

_int32 bt_pipe_choke_remote(struct tagDATA_PIPE* pipe, BOOL choke);
#endif

#ifdef _CONNECT_DETAIL
_int32 bt_pipe_get_peer_pipe_info(struct tagDATA_PIPE* pipe, struct tagPEER_PIPE_INFO* info);
#endif
#endif /*  ENABLE_BT_PROTOCOL  */

#endif /*  ENABLE_BT */

#ifdef __cplusplus
}
#endif
#endif



