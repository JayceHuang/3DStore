#include "utility/settings.h"
#include "utility/peerid.h"
#include "utility/local_ip.h"
#include "utility/version.h"
#include "utility/utility.h"
#include "utility/mempool.h"
#include "utility/logid.h"
#define  LOGID	LOGID_P2P_TRANSFER_LAYER
#include "utility/logger.h"

#include "ptl_cmd_sender.h"

#include "ptl_cmd_define.h"
#include "ptl_cmd_builder.h"
#include "ptl_udp_device.h"
#include "ptl_utility.h"
#include "ptl_interface.h"
#include "udt/udt_utility.h"

#define		PING_VERSION		59

static	char		g_net_server_addr[MAX_URL_LEN];
static	char		g_ping_server_addr[MAX_URL_LEN];
static	_u32	g_net_server_port = 0;
static	_u32	g_ping_server_port = 0;

_int32 ptl_init_cmd_sender(void)
{
	sd_memcpy(g_net_server_addr, DEFAULT_NET_SERVER_HOST_NAME, sizeof(DEFAULT_NET_SERVER_HOST_NAME));
	settings_get_str_item("p2p_setting.net_server_addr", g_net_server_addr);

	g_net_server_port = 8000;
	settings_get_int_item("p2p_setting.net_server_port", (_int32*)&g_net_server_port);
	
	sd_memcpy(g_ping_server_addr, DEFAULT_PING_SERVER_HOST_NAME, sizeof(DEFAULT_PING_SERVER_HOST_NAME));
	settings_get_str_item("ptl_setting.ping_server_addr", g_ping_server_addr);
	
	g_ping_server_port = DEFAULT_PING_SERVERB_PORT;
	settings_get_int_item("ptl_setting.ping_server_port", (_int32*)&g_ping_server_port);
	return SUCCESS;
}

//NOTE:jieouy无线下载库因为不考虑上传，所以目前是不会ping phub和dphub
_int32 ptl_send_ping_cmd(void)
{
	char* buffer = NULL;
	_u32 len = 0;
	_int32 ret = SUCCESS;
	PING_CMD cmd;
	sd_memset(&cmd, 0, sizeof(PING_CMD));
	cmd._version = PING_VERSION;
	cmd._cmd_type = PING;
	cmd._local_peerid_len = PEER_ID_SIZE;
	get_peerid(cmd._local_peerid, PEER_ID_SIZE + 1);
	cmd._internal_ip = sd_get_local_ip();
	cmd._natwork_mask = sd_inet_addr("255.255.255.0");
	cmd._listen_port = (_u16)ptl_get_local_tcp_port();
	cmd._product_flag = get_product_flag();
	cmd._product_version = get_product_id();
	cmd._max_download_speed = 10240;
	ret = ptl_build_ping_cmd(&buffer, &len, &cmd);
	sd_assert(ret == SUCCESS);
	LOG_DEBUG("ptl_send_ping_cmd, ip = %s, port = %u.", g_ping_server_addr, g_ping_server_port );
	return ptl_udp_sendto_by_domain(buffer, len, g_ping_server_addr, g_ping_server_port);
}

_int32 ptl_send_logout_cmd(void)
{
	char* buffer = NULL;
	_u32 len = 0;
	_int32 ret = SUCCESS;
	LOGOUT_CMD cmd;
	cmd._version = PTL_PROTOCOL_VERSION;
	cmd._cmd_type = LOGOUT;
	cmd._ip = sd_get_local_ip();
	cmd._local_peerid_len = PEER_ID_SIZE;
	get_peerid(cmd._local_peerid, PEER_ID_SIZE + 1);
	ret = ptl_build_logout_cmd(&buffer, &len, &cmd);
	sd_assert(ret == SUCCESS);
	LOG_DEBUG("ptl_send_logout_cmd.");
	return ptl_udp_sendto_by_domain(buffer, len, g_ping_server_addr, g_ping_server_port);
}

_int32 ptl_send_get_peersn_cmd(const char* target_peerid)
{
	char* buffer = NULL;
	_u32 len = 0;
	_int32 ret = SUCCESS;
	GET_PEERSN_CMD cmd;
	cmd._target_peerid_len = PEER_ID_SIZE;
	sd_memcpy(cmd._target_peerid, target_peerid, PEER_ID_SIZE + 1);
	ret = ptl_build_get_peersn_cmd(&buffer, &len, &cmd);
	CHECK_VALUE(ret);
	LOG_DEBUG("ptl_send_get_peersn_cmd, target_peerid = %s", target_peerid);
	return ptl_udp_sendto_by_domain(buffer, len, g_net_server_addr, g_net_server_port);
}

_int32 ptl_send_get_mysn_cmd(const char* invalid_mysn)
{
    GET_MYSN_CMD cmd;
    cmd._local_peerid_len = PEER_ID_SIZE;
    get_peerid(cmd._local_peerid, PEER_ID_SIZE + 1);
    if (invalid_mysn == NULL)
    {
		cmd._invalid_sn_num = 0;
	}
	else
	{
		cmd._invalid_sn_num = 1;
		cmd._invalid_sn_peerid_len = PEER_ID_SIZE;
		sd_memcpy(cmd._invalid_sn_peerid, invalid_mysn, PEER_ID_SIZE + 1);
	}
	char* buffer = NULL;
	_u32 len = 0;
	_int32 ret = ptl_build_get_mysn_cmd(&buffer, &len, &cmd);
	CHECK_VALUE(ret);
	LOG_DEBUG("ptl_send_get_my_sn, invalid_mysn = %s.", invalid_mysn);
	return ptl_udp_sendto_by_domain(buffer, len, g_net_server_addr, g_net_server_port);
}

_int32 ptl_send_nn2sn_logout_cmd(_u32 ip, _u16 port)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL;
	_u32 len = 0;
	NN2SN_LOGOUT_CMD cmd;
	cmd._local_peerid_len = PEER_ID_SIZE;
	get_peerid(cmd._local_peerid, PEER_ID_SIZE + 1);
	ret = ptl_build_nn2sn_logout_cmd(&buffer, &len, &cmd);
	CHECK_VALUE(ret);
	LOG_DEBUG("ptl_send_nn2sn_logout_cmd");
	return ptl_udp_sendto(buffer, len, ip, port);
}

_int32 ptl_send_ping_sn_cmd(_u32 ip, _u16 port)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL;
	_u32 len = 0;
	PING_SN_CMD cmd;
	cmd._local_peerid_len = PEER_ID_SIZE;
	get_peerid(cmd._local_peerid, PEER_ID_SIZE + 1);
	cmd._nat_type = 0;
	cmd._latest_port = 0;
	cmd._time_elapsed = 0;
	cmd._delta_port = 0;
	ret = ptl_build_ping_sn_cmd(&buffer, &len, &cmd);
	CHECK_VALUE(ret);
	
	LOG_DEBUG("ptl_send_ping_sn_cmd");
	return ptl_udp_sendto(buffer, len, ip, port);
}

_int32 ptl_send_punch_hole_cmd(_u16 source_port, _u16 target_port, _u32 ip, _u16 port, _u16 latest_ex_port, _u16 guess_port)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL;
	_u32 len = 0;
	PUNCH_HOLE_CMD cmd;
	LOG_DEBUG("ptl_send_punch_hole_cmd, source_port = %u, target_port = %u, ip = %u, port = %u, last_port = %u, guess_port = %u.", source_port, target_port, ip, port, latest_ex_port, guess_port);
	cmd._peerid_len = PEER_ID_SIZE;
	get_peerid(cmd._peerid, PEER_ID_SIZE + 1);
	cmd._source_port = source_port;
	cmd._target_port = target_port;
	ret = ptl_build_punch_hole_cmd(&buffer, &len, &cmd);
	CHECK_VALUE(ret);
	ret = ptl_udp_sendto(buffer, len, ip, port);
	CHECK_VALUE(ret);
	if(latest_ex_port != port)
	{
		ret = ptl_build_punch_hole_cmd(&buffer, &len, &cmd);
		CHECK_VALUE(ret);
		ret = ptl_udp_sendto(buffer, len, ip, latest_ex_port);
	}
	if(guess_port != port && guess_port != latest_ex_port)
	{
		ret = ptl_build_punch_hole_cmd(&buffer, &len, &cmd);
		CHECK_VALUE(ret);
		ret = ptl_udp_sendto(buffer, len, ip, guess_port);
	}
	return ret;
}

_int32 ptl_send_broker2_req_cmd(_u32 seq_num, const char* remote_peerid, _u32 sn_ip, _u16 sn_udp_port)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL;
	_u32 len = 0;
	BROKER2_REQ_CMD cmd;
	LOG_DEBUG("ptl_send_broker2_req_cmd, seq_num = %u, remote_peerid = %s.", seq_num, remote_peerid);
	sd_memset(&cmd, 0, sizeof(BROKER2_REQ_CMD));
	cmd._seq_num = seq_num;
	cmd._requestor_ip = sd_get_local_ip();
	cmd._requestor_port = ptl_get_local_tcp_port();
	cmd._remote_peerid_len = PEER_ID_SIZE;
	sd_memcpy(cmd._remote_peerid, remote_peerid, PEER_ID_SIZE);
	ret = ptl_build_broker2_req_cmd(&buffer, &len, &cmd);
	CHECK_VALUE(ret);
	ret = ptl_udp_sendto(buffer, len, sn_ip, sn_udp_port);
	CHECK_VALUE(ret);
	return ret;
}

_int32 ptl_send_udp_broker_req_cmd(_u32 seq_num, const char* remote_peerid, _u32 sn_ip, _u16 sn_udp_port)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL;
	_u32 len = 0;
	UDP_BROKER_REQ_CMD cmd;
	LOG_DEBUG("ptl_send_udp_broker_req_cmd, seq_num = %u, remote_peerid = %s.", seq_num, remote_peerid);
	sd_memset(&cmd, 0, sizeof(UDP_BROKER_REQ_CMD));
	cmd._seq_num = seq_num;
	cmd._requestor_ip = sd_get_local_ip();
	cmd._requestor_udp_port = ptl_get_local_udp_port();
	cmd._remote_peerid_len = PEER_ID_SIZE;
	sd_memcpy(cmd._remote_peerid, remote_peerid, PEER_ID_SIZE);
	cmd._requestor_peerid_len = PEER_ID_SIZE;
	get_peerid(cmd._requestor_peerid, PEER_ID_SIZE + 1);
	ret = ptl_build_udp_broker_req_cmd(&buffer, &len, &cmd);
	CHECK_VALUE(ret);
	ret = ptl_udp_sendto(buffer, len, sn_ip, sn_udp_port);
	CHECK_VALUE(ret);
	return ret;
}

_int32 ptl_send_icallsomeone_cmd(const char* remote_peerid, _u16 virtual_source_port, _u32 sn_ip, _u16 sn_udp_port)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL;
	_u32  len = 0;
	ICALLSOMEONE_CMD cmd;
	LOG_DEBUG("ptl_send_icallsomeone_cmd, remote_peerid = %s, virtual_source_port = %u, sn_ip = %u, sn_port = %u.", remote_peerid, virtual_source_port, sn_ip, sn_udp_port);
	sd_memset(&cmd, 0, sizeof(ICALLSOMEONE_CMD));
	cmd._version = PTL_PROTOCOL_VERSION;
	cmd._cmd_type = ICALLSOMEONE;
	cmd._local_peerid_size = PEER_ID_SIZE;
	get_peerid(cmd._local_peerid, cmd._local_peerid_size + 1);
	cmd._remote_peerid_size = PEER_ID_SIZE;
	sd_memcpy(cmd._remote_peerid, remote_peerid, cmd._remote_peerid_size);
	cmd._virtual_port = virtual_source_port;
	cmd._nat_type = 0;		/*网络类型未知*/
	/*以下字段为端口猜测用，填0，不使用*/
	cmd._latest_extenal_port = 0;
	cmd._time_elapsed = 0;
	cmd._delta_port = 0;
	ret = ptl_build_icallsomeone_cmd(&buffer, &len, &cmd);
	if (ret != SUCCESS)
	{
		LOG_ERROR("udt_build_icallsomeone_cmd failed, ret = %d.", ret);
		return ret;
	}
	ret = ptl_udp_sendto(buffer, len, sn_ip, sn_udp_port);
	CHECK_VALUE(ret);
	return ret;
}

_int32 ptl_send_transfer_layer_control_cmd(struct tagPTL_DEVICE * device, _u32 seq)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL;
	_u32 len = 0;
	TRANSFER_LAYER_CONTROL_CMD cmd;
	LOG_DEBUG("[device = 0x%x]ptl_send_transfer_layer_control_cmd. seq = %u.", device, seq);
	sd_memset(&cmd, 0, sizeof(TRANSFER_LAYER_CONTROL_CMD));
	cmd._broker_seq = seq;
	ret = ptl_build_transfer_layer_control_cmd(&buffer, &len, &cmd);
	if (ret != SUCCESS)
	{
		LOG_ERROR("ptl_send_transfer_layer_control_cmd failed, ret = %d.", ret);
		return ret;
	}
	return ptl_send(device, buffer, len);
}

_int32 ptl_send_transfer_layer_control_resp_cmd(struct tagPTL_DEVICE* device, _u32 result)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL;
	_u32 len = 0;
	TRANSFER_LAYER_CONTROL_RESP_CMD cmd;
	LOG_DEBUG("[device = 0x%x]ptl_send_transfer_layer_control_resp_cmd, result = %u.", device, result);
	sd_memset(&cmd, 0, sizeof(TRANSFER_LAYER_CONTROL_RESP_CMD));
	cmd._result = result;
	ret = ptl_build_transfer_layer_control_resp_cmd(&buffer, &len, &cmd);
	if (ret != SUCCESS)
	{
		LOG_ERROR("ptl_build_transfer_layer_control_resp_cmd failed, ret = %d.", ret);
		return ret;
	}
	return ptl_send(device, buffer, len);
}
