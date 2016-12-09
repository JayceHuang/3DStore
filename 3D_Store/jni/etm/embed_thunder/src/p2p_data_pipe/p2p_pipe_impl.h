/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2009/06/02
-----------------------------------------------------------------------------------------------------------*/
#ifndef _P2P_PIPE_IMPL_H_
#define	_P2P_PIPE_IMPL_H_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "p2p_data_pipe/p2p_data_pipe.h"
#include "asyn_frame/msg.h"

struct _tag_range_data_buffer;

_int32 p2p_handle_passive_connect(P2P_DATA_PIPE* p2p_pipe, BOOL upload_and_download);

_int32 p2p_pipe_request_data(struct tagP2P_DATA_PIPE* p2p_pipe);

void p2p_pipe_change_state(struct tagP2P_DATA_PIPE* p2p_pipe, enum tagPIPE_STATE state);

_int32 p2p_pipe_handle_error(struct tagP2P_DATA_PIPE* p2p_pipe, _int32 error_code);

//关闭p2p_pipe是一个异步过程，调用该函数才真正把p2p_pipe关闭
_int32 p2p_pipe_notify_close(struct tagP2P_DATA_PIPE* p2p_pipe);
#ifdef __cplusplus
}
#endif
#endif
