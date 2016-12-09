/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2008/07/23
-----------------------------------------------------------------------------------------------------------*/
#ifndef _P2P_CMD_EXTRACTOR_H_
#define _P2P_CMD_EXTRACTOR_H_
#ifdef __cplusplus
extern "C" 
{
#endif

#include "p2p_data_pipe/p2p_cmd_define.h"

_int32 extract_handshake_cmd(char* buffer, _u32 len, HANDSHAKE_CMD* cmd);

_int32 extract_handshake_resp_cmd(char* buffer, _u32 len, HANDSHAKE_RESP_CMD* cmd);

_int32 extract_interested_cmd(char* buffer, _u32 len, INTERESTED_CMD* cmd);

_int32 extract_interested_resp_cmd(char* buffer, _u32 len, INTERESTED_RESP_CMD* cmd);

_int32 extract_request_cmd(char* buffer, _u32 len, REQUEST_CMD* cmd);

_int32 extract_request_resp_cmd(char* buffer, _u32 len, REQUEST_RESP_CMD* cmd);

_int32 extract_cancel_resp_cmd(char* buffer, _u32 len, CANCEL_RESP_CMD* cmd);
#ifdef __cplusplus
}
#endif
#endif

