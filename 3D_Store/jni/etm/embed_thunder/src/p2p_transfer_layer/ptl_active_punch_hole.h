/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/07/21
-----------------------------------------------------------------------------------------------------------*/
#ifndef _PTL_ACTIVE_PUNCH_HOLE_H_
#define _PTL_ACTIVE_PUNCH_HOLE_H_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "asyn_frame/msg.h"
#include "utility/define.h"

struct tagCONN_ID;
struct tagICALLSOMEONE_RESP_CMD;
struct tagPUNCH_HOLE_CMD;
struct tagSYN_CMD;
struct tagACTIVE_PUNCH_HOLE_DATA;

_int32 ptl_init_active_punch_hole(void);

_int32 ptl_uninit_active_punch_hole(void);

_int32 ptl_active_punch_hole(struct tagCONN_ID* id, const char* peerid);

_int32 ptl_active_punch_hole_get_peersn_callback(_int32 errcode, _u32 sn_ip, _u16 sn_port, void* user_data);

_int32 ptl_recv_icallsomeone_resp_cmd(struct tagICALLSOMEONE_RESP_CMD* cmd);

_int32 ptl_recv_punch_hole_cmd(struct tagPUNCH_HOLE_CMD* cmd, _u32 remote_ip, _u16 remote_port);

_int32 ptl_remove_active_punch_hole_data(struct tagSYN_CMD* cmd, _u32 remote_ip, _u16 remote_port);

_int32 ptl_erase_active_punch_hole_data(struct tagACTIVE_PUNCH_HOLE_DATA * data);

_int32 ptl_handle_icallsomeone_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid);

_int32 ptl_handle_syn_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid);
#ifdef __cplusplus
}
#endif
#endif

