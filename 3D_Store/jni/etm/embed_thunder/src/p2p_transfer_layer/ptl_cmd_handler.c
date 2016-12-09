#include "ptl_cmd_handler.h"
#include "ptl_cmd_extractor.h"
#include "ptl_intranet_manager.h"
#include "ptl_cmd_define.h"
#include "ptl_active_punch_hole.h"
#include "ptl_passive_punch_hole.h"
#include "ptl_passive_tcp_broker.h"
#include "ptl_passive_udp_broker.h"
#include "ptl_get_peersn.h"
#include "udt/udt_cmd_define.h"
#include "udt/udt_cmd_handler.h"
#include "udt/udt_cmd_extractor.h"
#include "udt/udt_device.h"
#include "utility/bytebuffer.h"
#include "utility/local_ip.h"
#include "utility/logid.h"
#define  LOGID	LOGID_P2P_TRANSFER_LAYER
#include "utility/logger.h"

_int32 ptl_handle_recv_cmd(char** buffer, _u32 len, _u32 remote_ip, _u16 remote_port)
{
	_u32 version = 0;
	_u8  command_type = 0;
	//解析版本号
	char* tmp_buf = *buffer;
	_int32 tmp_len = (_int32)len;
	sd_assert(len >= 5);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&version);		//这里要注意大小端问题
	if(version < PTL_PROTOCOL_VERSION_50)
	{
		LOG_ERROR("ptl_handle_recv_udp_data, but version = %u is invalid.", version);
		return ERR_PTL_PROTOCOL_VERSION;			/*not support old protocol*/
	}
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&command_type);
	switch(command_type)
	{
	case SOMEONECALLYOU:
		{
			LOG_DEBUG("ptl_handle_recv_udp_data, recv a SOMEONECALLYOU command.");
			return ptl_handle_someonecallyou_cmd(*buffer, len);
		}
	case PUNCH_HOLE:
		{
			LOG_DEBUG("ptl_handle_recv_udp_data, recv a PUNCH_HOLE command, remote_ip = %u, remote_port = %u", remote_ip, remote_port);
			return ptl_handle_punch_hold_cmd(*buffer, len, remote_ip, remote_port);
		}
	case SYN:
		{
			LOG_DEBUG("ptl_handle_recv_udp_data, recv a P2P_SYN command, remote_ip = %u, remote_port = %u", remote_ip, remote_port);
			return ptl_handle_syn_cmd(*buffer, len, remote_ip, remote_port);
		}
	case SN2NN_LOGOUT:
		{
			LOG_DEBUG("ptl_handle_recv_udp_data, recv a SN2NN_LOGOUT command.");
			return ptl_handle_sn2nn_logout_cmd(*buffer, len);
		}
	case ADVANCED_ACK:
		{
			LOG_DEBUG("ptl_handle_recv_udp_data, recv a ADVANCED_ACK command. len = %u", len);
			return udt_handle_advance_ack_cmd(*buffer, len);
		}
	case BROKER1:
		{
			LOG_DEBUG("ptl_handle_recv_udp_data, recv a BRLDER1 command, but this command not support in embed thunder.");
			return SUCCESS;
		}
	case BROKER2:
		{
			LOG_DEBUG("ptl_handle_recv_udp_data, recv a BROKER2 command.");
			return ptl_handle_broker2_cmd(*buffer, len);
		}
	case UDP_BROKER:
		{
			LOG_DEBUG("ptl_handle_recv_udp_data, recv a UDP_BROKER command.");
			return ptl_handle_udp_broker_cmd(*buffer, len);
		}
	case ICALLSOMEONE_RESP:
		{
			LOG_DEBUG("ptl_handle_recv_udp_data, recv a ICALLSOMEONE_RESP command. remote_ip = %u, remote_port = %u", remote_ip, remote_port);
			return ptl_handle_icallsomeone_resp(*buffer, len, remote_ip);
		}
	case PING_SN_RESP:
		{
			LOG_DEBUG("ptl_handle_recv_udp_data, recv a PING_SN_RESP command.");
			return ptl_handle_ping_sn_resp_cmd(*buffer, len);
		}
	case GET_MYSN_RESP:
		{
			LOG_DEBUG("ptl_handle_recv_udp_data, recv a GET_MYSN_RESP command.");
			return ptl_handle_get_mysn_resp_cmd(*buffer, len);
		}
	case GET_PEERSN_RESP:
		{
			LOG_DEBUG("ptl_handle_recv_udp_data, recv a GET_PEERSN_RESP command.");
			return ptl_handle_get_peersn_resp(*buffer, len);
		}
	case RESET:
		{
			LOG_DEBUG("ptl_handle_recv_udp_data, recv a RESET command.");
			return udt_handle_reset_cmd(*buffer, len);
		}
	case NAT_KEEPALIVE:
		{
			LOG_DEBUG("ptl_handle_recv_udp_data, recv a NAT_KEEPALIVE command.");
			return udt_handle_nat_keepalive_cmd(*buffer, len);
		}
	case DATA_TRANSFER:
		{
			LOG_DEBUG("ptl_handle_recv_udp_data, recv a DATA_TRANSFER command, but this command not support in embed thunder.", len);
			return SUCCESS;
		}
	case ADVANCED_DATA:
		{
			LOG_DEBUG("ptl_handle_recv_udp_data, recv a ADVANCED_DATA command. len = %u", len);
			return udt_handle_advance_data_cmd(buffer, len);
		}
	default:
		{
			LOG_DEBUG("ptl_handle_recv_udp_data, but this command not support, command type is %d", command_type);
			return ERR_PTL_PROTOCOL_NOT_SUPPORT;
		}
	}
	return SUCCESS;
}

_int32 ptl_handle_someonecallyou_cmd(char* buffer, _u32 len)
{
	_int32 ret = SUCCESS;
	SOMEONECALLYOU_CMD cmd;
	ret = ptl_extract_someonecallyou_cmd(buffer, len, &cmd);
	if(ret != SUCCESS)
		return ret;
	return ptl_recv_someonecallyou_cmd(&cmd);
}

_int32 ptl_handle_punch_hold_cmd(char* buffer, _u32 len, _u32 remote_ip, _u16 remote_port)
{
	_int32 ret = SUCCESS;
	PUNCH_HOLE_CMD	cmd;
	ret = ptl_extract_punch_hole_cmd(buffer, len, &cmd);
	if(ret != SUCCESS)
		return ret;
	return ptl_recv_punch_hole_cmd(&cmd, remote_ip, remote_port);
}

_int32 ptl_handle_syn_cmd(char* buffer, _u32 len, _u32 remote_ip, _u16 remote_port)
{
	_int32 ret = SUCCESS;
	CONN_ID id;
	SYN_CMD cmd;
	ret = udt_extract_syn_cmd(buffer, len, &cmd);
	if(ret != SUCCESS)
		return ret;
	id._virtual_source_port = cmd._target_port;
	id._virtual_target_port = cmd._source_port;
	id._peerid_hashcode = cmd._peerid_hashcode;
	//提交给punch_hole模块，看是否需要处理
	if(sd_is_in_nat(sd_get_local_ip()) == TRUE)	
	{	//内网启动了punch_hole
		if(cmd._flags == 1)		
		{	//这是一个syn_ack命令
			ptl_remove_active_punch_hole_data(&cmd, remote_ip, remote_port);
		}
		else if(cmd._flags == 0)
		{	//这是一个syn命令
			ptl_remove_passive_punch_hole_data(&cmd);
		}
	}
	//提交给udt模块，看是否需要处理
	return udt_handle_syn_cmd(&cmd, remote_ip, remote_port);
}


_int32 ptl_handle_sn2nn_logout_cmd(char* buffer, _u32 len)
{
	_int32 ret = SUCCESS;
	SN2NN_LOGOUT_CMD cmd;
	ret = ptl_extract_sn2nn_logout_cmd(buffer, len, &cmd);
	if(ret != SUCCESS)
		return ret;
	return ptl_recv_sn2nn_logout_cmd(&cmd);
}

_int32 ptl_handle_broker2_cmd(char* buffer, _u32 len)
{
	_int32 ret = SUCCESS;
	BROKER2_CMD cmd;
	ret = ptl_extract_broker2_cmd(buffer, len, &cmd);
	if(ret != SUCCESS)
		return ret;
	return ptl_passive_tcp_broker(&cmd);
}

_int32 ptl_handle_udp_broker_cmd(char* buffer, _u32 len)
{
	_int32 ret = SUCCESS;
	UDP_BROKER_CMD cmd;
	ret = ptl_extract_udp_broker_cmd(buffer, len, &cmd);
	if(ret != SUCCESS)
		return ret;
	return ptl_passive_udp_broker(&cmd);
}

_int32 ptl_handle_icallsomeone_resp(char* buffer, _u32 len, _u32 ip)
{
	_int32 ret = SUCCESS; 
	ICALLSOMEONE_RESP_CMD cmd;
	ret = ptl_extract_icallsomeone_resp_cmd(buffer, len, &cmd);
	if(ret != SUCCESS)
		return ret;
	return ptl_recv_icallsomeone_resp_cmd(&cmd);
}

_int32 ptl_handle_ping_sn_resp_cmd(char* buffer, _u32 len)
{
	_int32 ret = SUCCESS;
	PING_SN_RESP_CMD cmd;
	ret = ptl_extract_ping_sn_resp_cmd(buffer, len, &cmd);
	if(ret != SUCCESS)
		return ret;
	return ptl_recv_ping_sn_resp_cmd(&cmd);
}

_int32 ptl_handle_get_mysn_resp_cmd(char* buffer, _u32 len)
{
	_int32 ret = SUCCESS;
	GET_MYSN_RESP_CMD cmd;
	ret = ptl_extract_get_mysn_resp_cmd(buffer, len, &cmd);
	if(ret != SUCCESS)
		return ret;
	return ptl_recv_get_mysn_resp_cmd(&cmd);
}

_int32 ptl_handle_get_peersn_resp(char* buffer, _u32 len)
{
	_int32 ret = SUCCESS;
	GET_PEERSN_RESP_CMD cmd;
	ret = ptl_extract_get_peersn_resp_cmd(buffer, len, &cmd);
	if(ret != SUCCESS)
		return ret;
	return ptl_recv_get_peersn_resp_cmd(&cmd);
}





