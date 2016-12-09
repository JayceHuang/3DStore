/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2008/10/6
-----------------------------------------------------------------------------------------------------------*/
#ifndef _PTL_CMD_SENDER_H_
#define _PTL_CMD_SENDER_H_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"
#include "utility/list.h"

struct tagPTL_DEVICE;

_int32 ptl_init_cmd_sender(void);

_int32 ptl_send_ping_cmd(void);

_int32 ptl_send_logout_cmd(void);

_int32 ptl_send_get_peersn_cmd(const char* target_peerid);

_int32 ptl_send_get_mysn_cmd(const char* invalid_mysn);

_int32 ptl_send_nn2sn_logout_cmd(_u32 ip, _u16 port);

_int32 ptl_send_ping_sn_cmd(_u32 ip, _u16 port);

_int32 ptl_send_punch_hole_cmd(_u16 source_port, _u16 target_port, _u32 ip, _u16 port, _u16 latest_ex_port, _u16 guess_port);

_int32 ptl_send_broker2_req_cmd(_u32 seq_num, const char* remote_peerid, _u32 sn_ip, _u16 sn_udp_port);

_int32 ptl_send_udp_broker_req_cmd(_u32 seq_num, const char* remote_peerid, _u32 sn_ip, _u16 sn_udp_port);

_int32 ptl_send_icallsomeone_cmd(const char* remote_peerid, _u16 virtual_source_port, _u32 sn_ip, _u16 sn_udp_port);

_int32 ptl_send_transfer_layer_control_resp_cmd(struct tagPTL_DEVICE* device, _u32 result);
#ifdef __cplusplus
}
#endif
#endif

