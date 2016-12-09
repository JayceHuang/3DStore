/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2008/10/6
-----------------------------------------------------------------------------------------------------------*/
#ifndef _PTL_CMD_BUILDER_H_
#define	_PTL_CMD_BUILDER_H_
#ifdef __cplusplus
extern "C" 
{
#endif

#include "p2p_transfer_layer/ptl_cmd_define.h"

_int32 ptl_build_ping_cmd(char** buffer, _u32* len, PING_CMD* cmd);

_int32 ptl_build_logout_cmd(char** buffer, _u32* len, LOGOUT_CMD* cmd);

_int32 ptl_build_get_peersn_cmd(char** buffer, _u32* len, GET_PEERSN_CMD* cmd);

_int32 ptl_build_get_mysn_cmd(char** buffer, _u32* len, GET_MYSN_CMD* cmd);

_int32 ptl_build_ping_sn_cmd(char** buffer, _u32* len, PING_SN_CMD* cmd);

_int32 ptl_build_nn2sn_logout_cmd(char** buffer, _u32* len, NN2SN_LOGOUT_CMD* cmd);

_int32 ptl_build_punch_hole_cmd(char** buffer, _u32* len, PUNCH_HOLE_CMD* cmd);

_int32 ptl_build_transfer_layer_control_cmd(char** buffer, _u32* len, TRANSFER_LAYER_CONTROL_CMD* cmd);

_int32 ptl_build_transfer_layer_control_resp_cmd(char** buffer, _u32* len, TRANSFER_LAYER_CONTROL_RESP_CMD* cmd);

_int32 ptl_build_broker2_req_cmd(char** buffer, _u32* len, BROKER2_REQ_CMD* cmd);

_int32 ptl_build_udp_broker_req_cmd(char** buffer, _u32* len, UDP_BROKER_REQ_CMD* cmd);

_int32 ptl_build_icallsomeone_cmd(char** buffer, _u32* len, ICALLSOMEONE_CMD* cmd);

#ifdef __cplusplus
}
#endif
#endif

