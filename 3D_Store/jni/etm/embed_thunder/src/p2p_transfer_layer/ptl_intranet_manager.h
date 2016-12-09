#ifndef _PTL_INTRANET_MANAGER_H_
#define _PTL_INTRANET_MANAGER_H_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "asyn_frame/msg.h"
#include "utility/define.h"

struct tagGET_MYSN_RESP_CMD;
struct tagPING_SN_RESP_CMD;
struct tagSN2NN_LOGOUT_CMD;

_int32 ptl_start_intranet_manager(void);

_int32 ptl_stop_intranet_manager(void);

_int32 ptl_recv_get_mysn_resp_cmd(struct tagGET_MYSN_RESP_CMD* cmd);

_int32 ptl_recv_ping_sn_resp_cmd(struct tagPING_SN_RESP_CMD* cmd);

_int32 ptl_recv_sn2nn_logout_cmd(struct tagSN2NN_LOGOUT_CMD* cmd);

_int32 ptl_ping_sn_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid);

_int32 ptl_set_mysn_invalid(void);


#ifdef __cplusplus
}
#endif
#endif

