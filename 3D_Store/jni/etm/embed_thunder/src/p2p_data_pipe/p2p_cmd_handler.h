/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2008/07/02
-----------------------------------------------------------------------------------------------------------*/

#ifndef	_P2P_CMD_HANDLER_H_
#define _P2P_CMD_HANDLER_H_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "asyn_frame/msg.h"

struct tagP2P_DATA_PIPE;

/*
function : receive a complete protocol package
para	 : [in]version -- the protocol version; [in] command_type -- the command type; 
		   [in]buffer -- point to the protocol body, exclude the protocol header which length is P2P_CMD_TCP_HEADER_LENGTH
		   [in]buffer_len -- the length of the protocol body
return	 : 0 -- success;	nonzero -- error
*/
_int32 handle_recv_cmd(struct tagP2P_DATA_PIPE* pipe, _u8 command_type, char* buffer, _u32 buffer_len);

#ifdef UPLOAD_ENABLE
_int32 handle_request_cmd(struct tagP2P_DATA_PIPE* p2p_pipe, char* buffer, _u32 len);
#endif
/*
function : process REQUEST_RESPONSE command,this is a special command must be process althought it's not a complete protocol package.
		   since this command's protocol body contains file data, just to avoid data copy, we must allocate a buffer from DATA_MANAGER to 
		   receive the file data, and then commit to DATA_MANAGER for processed.
para	 : [in]pipe -- point to P2P_DATA_PIPE struct; [in]version -- the protocol version; 
		   [in]buffer -- point to the protocol body, exclude the protocol header and file data.
		   [in]buffer_len -- the length of buffer
return	 : 0 -- success;	nonzero -- error
*/
_int32 handle_request_resp_cmd(struct tagP2P_DATA_PIPE* pipe, char* buffer, _u32 len);

_int32 handle_handshake_cmd(struct tagP2P_DATA_PIPE* pipe, char* buffer, _u32 len);

_int32 handle_handshake_resp_cmd(struct tagP2P_DATA_PIPE* pipe, char* buffer, _u32 len);

_int32 handle_interested_cmd(struct tagP2P_DATA_PIPE* pipe, char* buffer, _u32 len);

_int32 handle_interested_resp_cmd(struct tagP2P_DATA_PIPE* pipe, char* buffer, _u32 len);

_int32 handle_not_interested(struct tagP2P_DATA_PIPE* pipe, char* buffer, _u32 len);

_int32 handle_cancel_cmd(struct tagP2P_DATA_PIPE* p2p_pipe, char* buffer, _u32 len);

_int32 handle_cancel_resp_cmd(struct tagP2P_DATA_PIPE* pipe, char* buffer, _u32 len);

_int32 handle_keepalive_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid);

_int32 handle_choke_cmd(struct tagP2P_DATA_PIPE* p2p_pipe);

_int32 handle_unchoke_cmd(struct tagP2P_DATA_PIPE* p2p_pipe);

_int32 handle_fin_cmd(struct tagP2P_DATA_PIPE* p2p_pipe);
#ifdef __cplusplus
}
#endif
#endif
