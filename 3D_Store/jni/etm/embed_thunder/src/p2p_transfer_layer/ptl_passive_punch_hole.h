/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2008/07/15
-----------------------------------------------------------------------------------------------------------*/
#ifndef _PTL_PASSIVE_PUNCH_HOLE_H_
#define _PTL_PASSIVE_PUNCH_HOLE_H_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "asyn_frame/msg.h"

struct tagSOMEONECALLYOU_CMD;
struct tagSYN_CMD;
struct tagPASSIVE_PUNCH_HOLE_DATA;

_int32 ptl_init_passive_punch_hole(void);

_int32 ptl_uninit_passive_punch_hole(void);

_int32 ptl_recv_someonecallyou_cmd(struct tagSOMEONECALLYOU_CMD* cmd);

_int32 ptl_handle_punch_hole_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid);

_int32 ptl_remove_passive_punch_hole_data(struct tagSYN_CMD* cmd);

_int32 ptl_erase_passive_punch_hole_data(struct tagPASSIVE_PUNCH_HOLE_DATA* data);
#ifdef __cplusplus
}
#endif
#endif

