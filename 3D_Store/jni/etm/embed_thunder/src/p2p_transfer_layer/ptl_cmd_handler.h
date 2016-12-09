/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/07/14
-----------------------------------------------------------------------------------------------------------*/
#ifndef _PTL_CMD_HANDLER_H_
#define _PTL_CMD_HANDLER_H_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"

_int32 ptl_handle_recv_cmd(char** buffer, _u32 len, _u32 remote_ip, _u16 remote_port);

_int32 ptl_handle_someonecallyou_cmd(char* buffer, _u32 len);

_int32 ptl_handle_punch_hold_cmd(char* buffer, _u32 len, _u32 remote_ip, _u16 remote_port);

_int32 ptl_handle_syn_cmd(char* buffer, _u32 len, _u32 remote_ip, _u16 remote_port);

_int32 ptl_handle_sn2nn_logout_cmd(char* buffer, _u32 len);

_int32 ptl_handle_broker2_cmd(char* buffer, _u32 len);

_int32 ptl_handle_udp_broker_cmd(char* buffer, _u32 len);

_int32 ptl_handle_icallsomeone_resp(char* buffer, _u32 len, _u32 ip);

_int32 ptl_handle_ping_sn_resp_cmd(char* buffer, _u32 len);

_int32 ptl_handle_get_mysn_resp_cmd(char* buffer, _u32 len);

_int32 ptl_handle_get_peersn_resp(char* buffer, _u32 len);
#ifdef __cplusplus
}
#endif
#endif

