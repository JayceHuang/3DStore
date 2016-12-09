/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/08/17
-----------------------------------------------------------------------------------------------------------*/
#ifndef _P2P_UPLOAD_H_
#define _P2P_UPLOAD_H_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"
#ifdef UPLOAD_ENABLE
#include "asyn_frame/msg.h"
#include "connect_manager/data_pipe.h"

struct tagP2P_DATA_PIPE;
struct _tag_range_data_buffer;
enum tagPEER_UPLOAD_STATE;

_int32 p2p_process_request_data(struct tagP2P_DATA_PIPE* p2p_pipe);

_int32 p2p_upload_data(struct tagP2P_DATA_PIPE* p2p_pipe);

_int32 p2p_stop_upload(struct tagP2P_DATA_PIPE* p2p_pipe);

_int32 p2p_free_current_upload_request(struct tagP2P_DATA_PIPE* p2p_pipe);

_int32 p2p_malloc_range_data_buffer(struct tagP2P_DATA_PIPE* p2p_pipe);

_int32 p2p_free_range_data_buffer(struct tagP2P_DATA_PIPE* p2p_pipe);

_int32 p2p_read_data(struct tagP2P_DATA_PIPE* p2p_pipe);

_int32 p2p_read_data_callback(_int32 errcode, struct _tag_range_data_buffer* data_buffer, void* user_data);

_int32 p2p_malloc_range_data_buffer_timeout_handler(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid);

BOOL p2p_fill_upload_data(struct tagP2P_DATA_PIPE* p2p_pipe);

void p2p_change_upload_state(struct tagP2P_DATA_PIPE* p2p_pipe, enum tagPEER_UPLOAD_STATE state);

_int32 p2p_handle_upload_failed(struct tagP2P_DATA_PIPE* p2p_pipe, _int32 errcode);
#endif
#ifdef __cplusplus
}
#endif
#endif


