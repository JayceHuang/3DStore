/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2008/08/27
-----------------------------------------------------------------------------------------------------------*/
#ifndef _P2P_SEND_CMD_H_
#define _P2P_SEND_CMD_H_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "p2p_data_pipe/p2p_data_pipe.h"

_int32 p2p_send_hanshake_cmd(P2P_DATA_PIPE* p2p_pipe);

_int32 p2p_send_interested_cmd(P2P_DATA_PIPE* p2p_pipe);

_int32 p2p_send_handshake_resp_cmd(P2P_DATA_PIPE* p2p_pipe, _u8 result);

_int32 p2p_send_request_cmd(P2P_DATA_PIPE* p2p_pipe, _u64 file_pos, _u64 file_len);

_int32 p2p_send_keepalive_cmd(P2P_DATA_PIPE* p2p_pipe);

_int32 p2p_send_cancel_cmd(P2P_DATA_PIPE* p2p_pipe);

_int32 p2p_send_cancel_resp_cmd(P2P_DATA_PIPE* p2p_pipe);

_int32 p2p_send_fin_resp_cmd(P2P_DATA_PIPE* p2p_pipe);

#ifdef UPLOAD_ENABLE
_int32 p2p_send_choke_cmd(P2P_DATA_PIPE* p2p_pipe, BOOL choke);

_int32 p2p_fill_interested_resp_block(char** tmp_buf, _int32* tmp_len, _u64 pos, _u64 length);

_int32 p2p_send_interested_resp_cmd(P2P_DATA_PIPE* p2p_pipe, _u32 min_block_size, _u32 max_block_num);
#endif
#ifdef __cplusplus
}
#endif
#endif
