/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2008/10/13
-----------------------------------------------------------------------------------------------------------*/
#ifndef _PTL_CMD_EXTRACTOR_H_
#define _PTL_CMD_EXTRACTOR_H_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "p2p_transfer_layer/ptl_cmd_define.h"

_int32 ptl_extract_someonecallyou_cmd(char* buffer, _u32 len, SOMEONECALLYOU_CMD* cmd);

_int32 ptl_extract_sn2nn_logout_cmd(char* buffer, _u32 len, SN2NN_LOGOUT_CMD* cmd);

_int32 ptl_extract_get_mysn_resp_cmd(char* buffer, _u32 len, GET_MYSN_RESP_CMD* cmd);

_int32 ptl_extract_ping_sn_resp_cmd(char* buffer, _u32 len, PING_SN_RESP_CMD* cmd);

_int32 ptl_extract_get_peersn_resp_cmd(char* buffer, _u32 len, GET_PEERSN_RESP_CMD* cmd);

_int32 ptl_extract_broker2_cmd(char* buffer, _u32 len, BROKER2_CMD* cmd);

_int32 ptl_extract_udp_broker_cmd(char* buffer, _u32 len, UDP_BROKER_CMD* cmd);

_int32 ptl_extract_transfer_layer_control_cmd(char* buffer, _u32 len, TRANSFER_LAYER_CONTROL_CMD* cmd);

_int32 ptl_extract_transfer_layer_control_resp_cmd(char* buffer, _u32 len, TRANSFER_LAYER_CONTROL_RESP_CMD* cmd);

_int32 ptl_extract_icallsomeone_resp_cmd(char* buffer, _u32 len, ICALLSOMEONE_RESP_CMD* cmd);

_int32 ptl_extract_punch_hole_cmd(char* buffer, _u32 len, PUNCH_HOLE_CMD* cmd);

#ifdef __cplusplus
}
#endif
#endif

