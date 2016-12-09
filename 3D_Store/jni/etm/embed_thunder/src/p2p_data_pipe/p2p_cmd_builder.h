/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2008/07/26
-----------------------------------------------------------------------------------------------------------*/
#ifndef _P2P_CMD_BUILDER_H_
#define	_P2P_CMD_BUILDER_H_
#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"

struct tagPTL_DEVICE;
struct tagGET_PEERSN_CMD;
struct tagHANDSHAKE_CMD;
struct tagHANDSHAKE_RESP_CMD;
struct tagINTERESTED_CMD;
struct tagINTERESTED_RESP_CMD;
struct tagCANCEL_CMD;
struct tagCANCEL_RESP_CMD;
struct tagREQUEST_CMD;
struct tagREQUEST_RESP_CMD;
struct tagKEEPALIVE_CMD;
struct tagFIN_RESP_CMD;

_int32 build_handshake_cmd(char** cmd_buffer, _u32* len, struct tagPTL_DEVICE* device, struct tagHANDSHAKE_CMD* cmd);

_int32 build_handshake_resp_cmd(char** cmd_buffer, _u32* len, struct tagPTL_DEVICE* device, struct tagHANDSHAKE_RESP_CMD* cmd);

_int32 build_choke_cmd(char** cmd_buffer, _u32* len, struct tagPTL_DEVICE* device, BOOL choke);

_int32 build_interested_cmd(char** cmd_buffer, _u32* len, struct tagPTL_DEVICE* device, struct tagINTERESTED_CMD* cmd);

_int32 build_interested_resp_cmd(char** cmd_buffer, _u32* len, struct tagPTL_DEVICE* device, struct tagINTERESTED_RESP_CMD* cmd);

_int32 build_cancel_cmd(char** cmd_buffer, _u32* len, struct tagPTL_DEVICE* device, struct tagCANCEL_CMD* cmd);

_int32 build_cancel_resp_cmd(char** cmd_buffer, _u32* len, struct tagPTL_DEVICE* device, struct tagCANCEL_RESP_CMD* cmd);

_int32 build_request_cmd(char** cmd_buffer, _u32* len, struct tagPTL_DEVICE* device, struct tagREQUEST_CMD* cmd);

_int32 build_request_resp_cmd(char** cmd_buffer, _u32* len, _u32* data_offset,  struct tagPTL_DEVICE* device, struct tagREQUEST_RESP_CMD* cmd);

_int32 build_keepalive_cmd(char** cmd_buffer, _u32* len, struct tagPTL_DEVICE* device, struct tagKEEPALIVE_CMD* cmd);

_int32 build_fin_resp_cmd(char** cmd_buffer, _u32* len, struct tagPTL_DEVICE* device, struct tagFIN_RESP_CMD* cmd);

#ifdef __cplusplus
}
#endif
#endif
