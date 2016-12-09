#include "udt_cmd_handler.h"
#include "udt_cmd_define.h"
#include "udt_impl.h"
#include "udt_cmd_extractor.h"
#include "udt_utility.h"
#include "udt_memory_slab.h"
#include "udt_device_manager.h"
#include "../ptl_callback_func.h"
#include "../ptl_memory_slab.h"
#include "../ptl_passive_connect.h"
#include "utility/utility.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_P2P_TRANSFER_LAYER
#include "utility/logger.h"

_int32 udt_handle_syn_cmd(struct tagSYN_CMD* cmd, _u32 remote_ip, _u16 remote_port)
{
	UDT_DEVICE* udt = NULL;
	CONN_ID id;
	id._virtual_source_port = cmd->_target_port;
	id._virtual_target_port = cmd->_source_port;
	id._peerid_hashcode = cmd->_peerid_hashcode;
	udt = udt_find_device(&id);
	if(udt != NULL)
		return udt_recv_syn_cmd(udt, cmd);
	if(cmd->_flags == 0)
	{	//收到一个被动连接
		ptl_accept_udt_passvie_connect(cmd, remote_ip, remote_port);
	}
	return SUCCESS;
}

_int32 udt_handle_advance_data_cmd(char** buffer, _u32 len)
{
	_int32 ret;
	CONN_ID id;
	UDT_DEVICE* udt = NULL;
	ADVANCED_DATA_CMD cmd;
	ret = udt_extract_advanced_data_cmd(*buffer, len, &cmd);
	if(ret != SUCCESS)
	{
		LOG_ERROR("udt_extract_advanced_data_cmd failed, errcode = %d.", ret);
		return ret;
	}
	id._virtual_source_port = cmd._target_port;
	id._virtual_target_port = cmd._source_port;
	id._peerid_hashcode = cmd._peerid_hashcode;
	udt = udt_find_device(&id);
	if(udt == NULL)
	{
		LOG_DEBUG("udt_handle_advance_data, can't find udt device, conn_id = %u", (_u32)id._virtual_source_port);
		return SUCCESS;
	}
	LOG_DEBUG("[udt = 0x%x, device = 0x%x]udt_handle_advance_data_cmd, seq = %u, ack = %u", udt, udt->_device, cmd._seq_num, cmd._ack_num);
	return udt_handle_data_package(udt, buffer, cmd._data, cmd._data_len, cmd._seq_num, cmd._ack_num, cmd._window_size, cmd._package_seq);
}

_int32 udt_handle_advance_ack_cmd(char* buffer, _u32 len)
{
	_int32 ret;
	CONN_ID id;
	UDT_DEVICE* udt = NULL;
	ADVANCED_ACK_CMD cmd;
	ret = udt_extract_advanced_ack_cmd(buffer, len, &cmd);
	if(ret != SUCCESS)
	{
		LOG_ERROR("udt_extract_advanced_ack_cmd failed, errcode = %d.", ret);
		return ret;
	}
	id._virtual_source_port = cmd._target_port;
	id._virtual_target_port = cmd._source_port;
	id._peerid_hashcode = cmd._peerid_hashcode;
	udt = udt_find_device(&id);
	if(udt == NULL)
	{
		LOG_DEBUG("udt_handle_advance_ack_cmd, can't find udt device, conn_id[%u, %u, %u]", 
					(_u32)id._virtual_source_port, (_u32)id._virtual_target_port, id._peerid_hashcode);
		return SUCCESS;
	}
	return udt_recv_advance_ack_cmd(udt, &cmd);
}

_int32	udt_handle_reset_cmd(char* buffer, _u32 len)
{
	_int32 ret = SUCCESS;
	CONN_ID id;
	UDT_DEVICE* udt = NULL;
	RESET_CMD cmd;
	ret = udt_extract_reset_cmd(buffer, len, &cmd);
	if(ret != SUCCESS)
		return ret;
	id._virtual_source_port = cmd._target_port;
	id._virtual_target_port = cmd._source_port;
	id._peerid_hashcode = cmd._peerid_hashcode;
	LOG_DEBUG("recv a RESET cmd, conn_id = %u", (_u32)id._virtual_source_port);
	udt = udt_find_device(&id);
	if(udt == NULL)
		return SUCCESS;		//没有对应的udt,不需要处理
	if(udt->_state == CLOSE_STATE)
		return SUCCESS;		//udt已经关闭了，不需要处理
	return udt_recv_reset(udt);
}

_int32	udt_handle_nat_keepalive_cmd(char* buffer, _u32 len)
{
	_int32 ret;
	CONN_ID id;
	UDT_DEVICE* udt = NULL;
	NAT_KEEPALIVE_CMD cmd;
	ret = udt_extract_keepalive_cmd(buffer, len, &cmd);
	if(ret != SUCCESS)
		return ret;
	id._virtual_source_port = cmd._target_port;
	id._virtual_target_port = cmd._source_port;
	id._peerid_hashcode = cmd._peerid_hashcode;
	udt = udt_find_device(&id);
	if(udt == NULL)
		return SUCCESS;
	if(udt->_state != ESTABLISHED_STATE)
		return SUCCESS;
	return udt_recv_keepalive(udt);
}


