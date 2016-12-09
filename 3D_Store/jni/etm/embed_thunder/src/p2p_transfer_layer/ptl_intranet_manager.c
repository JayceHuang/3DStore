#include "ptl_intranet_manager.h"
#include "ptl_cmd_sender.h"
#include "ptl_cmd_define.h"
#include "utility/time.h"
#include "utility/logid.h"
#define  LOGID	LOGID_P2P_TRANSFER_LAYER
#include "utility/logger.h"

#define	NN_PING_SN_TIMEOUT			45000	
#define	MAX_SN_NO_PING_RESP_TIME	5

typedef struct tagMYSN_INFO
{
	BOOL	_is_valid;
	char		_peerid[PEER_ID_SIZE + 1];
	_u32	_ip;
	_u16	_port;
	_u32	_ping_sn_timeout_id;
	_u32	_ping_sn_failed_times;
}MYSN_INFO;

static MYSN_INFO	g_mysn_info;
static _int32 g_enable_ping_sn = 1;
extern _int32 ptl_ping_sn_notask_hook( const _int32 down_task);

_int32 ptl_start_intranet_manager(void)
{
	LOG_DEBUG("ptl_start_intranet_manager.");
	sd_memset(&g_mysn_info, 0, sizeof(g_mysn_info));
	g_mysn_info._is_valid = FALSE;
	g_mysn_info._ping_sn_timeout_id = INVALID_MSG_ID;
	return ptl_send_get_mysn_cmd(NULL);
}

_int32 ptl_stop_intranet_manager(void)
{
	LOG_DEBUG("ptl_stop_intranet_manager");
	if(g_mysn_info._is_valid == TRUE)
	{
		ptl_send_nn2sn_logout_cmd(g_mysn_info._ip, g_mysn_info._port);
		ptl_set_mysn_invalid();
	}
	return SUCCESS;
}

_int32 ptl_recv_get_mysn_resp_cmd(GET_MYSN_RESP_CMD* cmd)
{
	_int32 ret = SUCCESS;
	LOG_DEBUG("ptl_recv_get_mysn_resp, mysn = %s.", cmd->_mysn_peerid);
	if(cmd->_result != 1 || cmd->_mysn_array_num == 0)
	{
		LOG_DEBUG("ptl_recv_get_mysn_resp, but cmd->_result = %u, cmd->_mysn_array_num = %u, discard.", cmd->_result, cmd->_mysn_array_num);
		return SUCCESS;
	}
	if(g_mysn_info._is_valid == TRUE)
	{
		//目前mysn还是有效的，那么不用更新
		return SUCCESS;
	}
	g_mysn_info._is_valid = TRUE;
	sd_memcpy(g_mysn_info._peerid, cmd->_mysn_peerid, PEER_ID_SIZE);
	g_mysn_info._ip = cmd->_mysn_ip;
	g_mysn_info._port = cmd->_mysn_port;
	ret = ptl_send_ping_sn_cmd(g_mysn_info._ip, g_mysn_info._port);
	CHECK_VALUE(ret);
	++g_mysn_info._ping_sn_failed_times;
	if (g_enable_ping_sn == 1 && g_mysn_info._ping_sn_timeout_id == INVALID_MSG_ID)
		return start_timer(ptl_ping_sn_timeout, NOTICE_INFINITE, NN_PING_SN_TIMEOUT, 0, NULL, &g_mysn_info._ping_sn_timeout_id);
	return SUCCESS;
}

_int32 ptl_recv_ping_sn_resp_cmd(struct tagPING_SN_RESP_CMD* cmd)
{
	LOG_DEBUG("ptl_recv_ping_sn_resp_cmd, mysn = %s.", cmd->_sn_peerid);
	if(sd_strcmp(cmd->_sn_peerid, g_mysn_info._peerid) == 0)
	{	
		g_mysn_info._ping_sn_failed_times = 0;
	}
	else
	{
		LOG_DEBUG("ptl_recv_ping_sn_resp_cmd, but this resp not correct, mysn_info._sn_peerid = %s, cmd._sn_peerid = %s.",
					g_mysn_info._peerid, cmd->_sn_peerid);
	}
	return SUCCESS;
}

_int32 ptl_recv_sn2nn_logout_cmd(SN2NN_LOGOUT_CMD* cmd)
{
	LOG_DEBUG("ptl_recv_sn2nn_logout_cmd, sn_peerid = %s.", cmd->_sn_peerid);
	sd_assert(sd_strcmp(cmd->_sn_peerid, g_mysn_info._peerid) == 0);
	ptl_send_get_mysn_cmd(g_mysn_info._peerid);
	return ptl_set_mysn_invalid();
}

_int32 ptl_ping_sn_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid)
{
	_int32 ret = SUCCESS;
	if(errcode == MSG_CANCELLED)
		return SUCCESS;
	if( msgid != g_mysn_info._ping_sn_timeout_id) {
		cancel_timer(msgid);
	}
	sd_assert(g_mysn_info._is_valid == TRUE);
	++g_mysn_info._ping_sn_failed_times;
	if(g_mysn_info._ping_sn_failed_times >= MAX_SN_NO_PING_RESP_TIME)
	{
		LOG_DEBUG("ptl_ping_sn_timeout, ping sn max times but still no resp, my sn is invalid, set mysn invalid.");
		ptl_send_get_mysn_cmd(g_mysn_info._peerid);		//重新获得sn
		return ptl_set_mysn_invalid();
	}
	ret = ptl_send_ping_sn_cmd(g_mysn_info._ip, g_mysn_info._port);
	CHECK_VALUE(ret);
	return SUCCESS;
}

_int32 ptl_set_mysn_invalid()
{
	sd_assert(g_mysn_info._is_valid == TRUE);
	if(g_mysn_info._ping_sn_timeout_id != INVALID_MSG_ID)
		cancel_timer(g_mysn_info._ping_sn_timeout_id);
	return sd_memset(&g_mysn_info, 0, sizeof(g_mysn_info));
}

_int32 ptl_ping_sn_notask_hook( const _int32 down_task)
{
	_int32 ret = SUCCESS;
	if(down_task == 1) {
		g_enable_ping_sn = 1;
		if(g_mysn_info._is_valid == TRUE) {
			ret = ptl_send_ping_sn_cmd(g_mysn_info._ip, g_mysn_info._port);
			CHECK_VALUE(ret);
			if(g_mysn_info._ping_sn_timeout_id == INVALID_MSG_ID)
				start_timer(ptl_ping_sn_timeout, NOTICE_INFINITE, NN_PING_SN_TIMEOUT, 0, NULL, &g_mysn_info._ping_sn_timeout_id);
		} else {
			if(g_mysn_info._ping_sn_timeout_id == INVALID_MSG_ID)
				ptl_start_intranet_manager();
		}
	}
	if(down_task == 0) {
		if(g_mysn_info._ping_sn_timeout_id != INVALID_MSG_ID) {
			cancel_timer(g_mysn_info._ping_sn_timeout_id);
			g_mysn_info._ping_sn_timeout_id = INVALID_MSG_ID;
		}
		g_enable_ping_sn = 0;
	}
	return SUCCESS;
}	



