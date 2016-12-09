#include "p2p_cmd_builder.h"
#include "p2p_cmd_define.h"
#include "p2p_transfer_layer/ptl_interface.h"
#include "p2p_transfer_layer/udt/udt_cmd_define.h"
#include "utility/mempool.h"
#include "utility/utility.h"
#include "utility/bytebuffer.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_P2P_PIPE
#include "utility/logger.h"

_int32 build_handshake_cmd(char** cmd_buffer, _u32* len, PTL_DEVICE* device, HANDSHAKE_CMD* cmd)
{
	_int32 ret;
	_u32 offset;
	char* tmp_buf;
	_int32  tmp_len;
	cmd->_header._version = P2P_PROTOCOL_VERSION;
	cmd->_header._command_len = 117 + cmd->_internal_addr_len + MIN(255, cmd->_home_location_len);
	cmd->_header._command_type = HANDSHAKE;
	*len = cmd->_header._command_len + 8;
	ret = ptl_malloc_cmd_buffer(cmd_buffer, len, &offset, device);
	CHECK_VALUE(ret);
	tmp_buf = *cmd_buffer + offset;
	tmp_len = *len - offset;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._command_len);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_header._command_type);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_handshake_connect_id);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_by_what);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_gcid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_gcid, CID_SIZE);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_size);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_file_status);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_len);
	sd_assert(cmd->_peerid_len == PEER_ID_SIZE);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, cmd->_peerid_len);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_internal_addr_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_internal_addr, cmd->_internal_addr_len);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_tcp_listen_port);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_product_ver);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_downbytes_in_history);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_upbytes_in_history);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_not_in_nat);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_upload_speed_limit);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_same_nat_tcp_speed_max);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_diff_nat_tcp_speed_max);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_udp_speed_max);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_p2p_capability);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_phub_res_count);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_load_level);
	ret = sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_home_location_len);
	if(cmd->_home_location_len > 0)
		ret = sd_set_bytes(&tmp_buf, &tmp_len, cmd->_home_location, MIN(255, cmd->_home_location_len));
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_res_type);
	sd_assert(ret == SUCCESS && tmp_len == 0);
	if(ret != SUCCESS)
	{
		LOG_ERROR("build_handshake_cmd failed, errcode = %d", ret);
		ptl_free_cmd_buffer(*cmd_buffer);
	}
	return ret;
}

_int32 build_handshake_resp_cmd(char** cmd_buffer, _u32* len, PTL_DEVICE* device, HANDSHAKE_RESP_CMD* cmd)
{
	_int32 ret;
	_u32 offset;
	char* tmp_buf;
	_int32  tmp_len;
	cmd->_header._version = P2P_PROTOCOL_VERSION;
	cmd->_header._command_len = 72 + cmd->_home_location_len;
	cmd->_header._command_type = HANDSHAKE_RESP;
	*len = cmd->_header._command_len + 8;
	ret = ptl_malloc_cmd_buffer(cmd_buffer, len, &offset, device);
	CHECK_VALUE(ret);
	tmp_buf = *cmd_buffer + offset;
	tmp_len = *len - offset;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._command_len);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_header._command_type);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_result);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_len);
	sd_assert(cmd->_peerid_len == PEER_ID_SIZE);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, cmd->_peerid_len);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_product_ver);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_downbytes_in_history);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_upbytes_in_history);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_not_in_nat);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_upload_speed_limit);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_same_nat_tcp_speed_max);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_diff_nat_tcp_speed_max);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_udp_speed_max);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_p2p_capablility);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_phub_res_count);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_load_level);
	ret = sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_home_location_len);
	if(cmd->_home_location_len > 0)
		ret = sd_set_bytes(&tmp_buf, &tmp_len, cmd->_home_location, MIN(255, cmd->_home_location_len));
	sd_assert(ret == SUCCESS && tmp_len == 0);
	if(ret != SUCCESS)
	{
		LOG_ERROR("build_handshake_resp_cmd failed, errcode = %d", ret);
		ptl_free_cmd_buffer(*cmd_buffer);
	}
	return ret;
}

_int32 build_choke_cmd(char** cmd_buffer, _u32* len, PTL_DEVICE* device, BOOL choke)
{
	_int32 ret = SUCCESS;
	_u32 offset = 0;
	char* tmp_buf = NULL;
	_int32 tmp_len = 0;
	*len = 9;
	ret = ptl_malloc_cmd_buffer(cmd_buffer, len, &offset, device);
	CHECK_VALUE(ret);
	tmp_buf = *cmd_buffer + offset;
	tmp_len = *len - offset;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, P2P_PROTOCOL_VERSION);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, 1);
	if(choke == TRUE)
		ret = sd_set_int8(&tmp_buf, &tmp_len, CHOKE);
	else
		ret = sd_set_int8(&tmp_buf, &tmp_len, UNCHOKE);
	sd_assert(ret == SUCCESS && tmp_len == 0);
	if(ret != SUCCESS)
	{
		LOG_ERROR("build_choke_cmd failed, errcode = %d, choke = %d", ret, choke);
		ptl_free_cmd_buffer(*cmd_buffer);
	}
	return ret;
}

_int32 build_interested_cmd(char** cmd_buffer, _u32* len, PTL_DEVICE* device, INTERESTED_CMD* cmd)
{
	_int32 ret;
	_u32 offset;
	char* tmp_buf;
	_int32  tmp_len;
	cmd->_header._version = P2P_PROTOCOL_VERSION;
	cmd->_header._command_len = 10;
	cmd->_header._command_type = INTERESTED;
	*len = cmd->_header._command_len + 8;
	ret = ptl_malloc_cmd_buffer(cmd_buffer, len, &offset, device);
	CHECK_VALUE(ret);
	tmp_buf = *cmd_buffer + offset;
	tmp_len = *len - offset;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._command_len);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_header._command_type);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_by_what);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_min_block_size);
	sd_assert(tmp_len == 4);
	ret = sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_max_block_count);
	sd_assert(ret == SUCCESS);
	if(ret != SUCCESS)
	{
		LOG_ERROR("build_interested_cmd failed, errcode = %d", ret);
		ptl_free_cmd_buffer(*cmd_buffer);
	}
	return ret;
}

_int32 build_cancel_cmd(char** cmd_buffer, _u32* len, PTL_DEVICE* device, CANCEL_CMD* cmd)
{
	_int32 ret;
	_u32 offset;
	char* tmp_buf;
	_int32  tmp_len;
	cmd->_header._version = P2P_PROTOCOL_VERSION;
	cmd->_header._command_len = 1;
	cmd->_header._command_type = CANCEL;
	*len = cmd->_header._command_len + 8;
	ret = ptl_malloc_cmd_buffer(cmd_buffer, len, &offset, device);
	CHECK_VALUE(ret);
	tmp_buf = *cmd_buffer + offset;
	tmp_len = *len - offset;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._command_len);
	sd_assert(tmp_len == 1);
	ret = sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_header._command_type);
	sd_assert(ret == SUCCESS);
	if(ret != SUCCESS)
	{
		LOG_ERROR("build_cancel_cmd failed, errcode = %d", ret);
		ptl_free_cmd_buffer(*cmd_buffer);
	}
	return ret;
}

_int32 build_cancel_resp_cmd(char** cmd_buffer, _u32* len, PTL_DEVICE* device, CANCEL_RESP_CMD* cmd)
{
	_int32 ret;
	_u32 offset;
	char* tmp_buf;
	_int32 tmp_len;
	cmd->_header._version = P2P_PROTOCOL_VERSION;
	cmd->_header._command_len = 1;
	cmd->_header._command_type = CANCEL_RESP;
	*len = cmd->_header._command_len + 8;
	ret = ptl_malloc_cmd_buffer(cmd_buffer, len, &offset, device);
	CHECK_VALUE(ret);
	tmp_buf = *cmd_buffer + offset;
	tmp_len = *len - offset;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._command_len);
	sd_assert(tmp_len == 1);
	ret = sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_header._command_type);
	sd_assert(ret == SUCCESS);
	if(ret != SUCCESS)
	{
		LOG_ERROR("build_cancel_cmd failed, errcode = %d", ret);
		ptl_free_cmd_buffer(*cmd_buffer);
	}
	return ret;
}

_int32 build_request_cmd(char** cmd_buffer, _u32* len, PTL_DEVICE* device, REQUEST_CMD* cmd)
{
	_int32 ret;
	_u32 offset;
	char* tmp_buf;
	_int32  tmp_len;
	cmd->_header._version = P2P_PROTOCOL_VERSION;
	cmd->_header._command_len = 48;
	cmd->_header._command_type = REQUEST;
	*len = cmd->_header._command_len + 8;
	ret = ptl_malloc_cmd_buffer(cmd_buffer, len, &offset, device);
	CHECK_VALUE(ret);
	tmp_buf = *cmd_buffer + offset;
	tmp_len = *len - offset;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._command_len);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_header._command_type);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_by_what);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_pos);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_len);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_max_package_size);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_priority);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_upload_speed);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_unchoke_num);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_pipe_num);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_task_num);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_local_requested);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_remote_requested);
	ret = sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_file_ratio);
	sd_assert(ret == SUCCESS && tmp_len == 0);
	if(ret != SUCCESS)
	{
		LOG_ERROR("build_request_cmd failed, errcode = %d", ret);
		ptl_free_cmd_buffer(*cmd_buffer);
	}
	return ret;
}

_int32 build_request_resp_cmd(char** cmd_buffer, _u32* len, _u32* data_offset, PTL_DEVICE* device, REQUEST_RESP_CMD* cmd)
{
	_int32 ret;
	_u32 offset;
	char* tmp_buf;
	_int32 tmp_len;
	cmd->_header._version = P2P_PROTOCOL_VERSION;
	cmd->_header._command_len = 39 + cmd->_data_len;
	cmd->_header._command_type = REQUEST_RESP;
	*len = cmd->_header._command_len + 8;
	ret = ptl_malloc_cmd_buffer(cmd_buffer, len, &offset, device);
	CHECK_VALUE(ret);
	*data_offset = offset + 22;
	tmp_buf = *cmd_buffer + offset;
	tmp_len = *len - offset;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._command_len);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_header._command_type);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_result);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_data_pos);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_data_len);
	tmp_buf += cmd->_data_len;
	tmp_len -= cmd->_data_len;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_upload_speed);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_unchoke_num);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_pipe_num);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_task_num);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_local_request);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_remote_request);
	ret = sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_file_ratio);
	sd_assert(ret == SUCCESS && tmp_len == 0);
	if(ret != SUCCESS)
	{
		LOG_ERROR("build_request_resp_cmd failed, errcode = %d", ret);
		ptl_free_cmd_buffer(*cmd_buffer);
	}
	return ret;
}


_int32 build_keepalive_cmd(char** cmd_buffer, _u32* len, PTL_DEVICE* device, KEEPALIVE_CMD* cmd)
{
	_int32 ret;
	_u32 offset;
	char* tmp_buf;
	_int32  tmp_len;
	cmd->_header._version = P2P_PROTOCOL_VERSION;
	cmd->_header._command_len = 1;
	cmd->_header._command_type = KEEPALIVE;
	*len = cmd->_header._command_len + 8;
	ret = ptl_malloc_cmd_buffer(cmd_buffer, len, &offset, device);
	CHECK_VALUE(ret);
	tmp_buf = *cmd_buffer + offset;
	tmp_len = *len - offset;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._command_len);
	sd_assert(tmp_len == 1);
	ret = sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_header._command_type);
	sd_assert(ret == SUCCESS);
	if(ret != SUCCESS)
	{
		LOG_ERROR("build_keepalive_cmd failed, errcode = %d", ret);
		ptl_free_cmd_buffer(*cmd_buffer);
	}
	return ret;
}

_int32 build_fin_resp_cmd(char** cmd_buffer, _u32* len, PTL_DEVICE* device, FIN_RESP_CMD* cmd)
{
	_int32 ret;
	_u32 offset;
	char* tmp_buf;
	_int32 tmp_len;
	cmd->_version = P2P_PROTOCOL_VERSION;
	cmd->_cmd_len = 1;
	cmd->_cmd_type = FIN_RESP;
	*len = cmd->_cmd_len + 8;
	ret = ptl_malloc_cmd_buffer(cmd_buffer, len, &offset, device);
	CHECK_VALUE(ret);
	tmp_buf = *cmd_buffer + offset;
	tmp_len = *len - offset;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cmd_len);
	sd_assert(tmp_len == 1);
	ret = sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_cmd_type);
	sd_assert(ret == SUCCESS);
	if(ret != SUCCESS)
	{
		LOG_ERROR("build_fin_resp_cmd failed, errcode = %d", ret);
		ptl_free_cmd_buffer(*cmd_buffer);
	}
	return ret;
}

_int32 build_interested_resp_cmd(char** cmd_buffer, _u32* len, PTL_DEVICE* device, INTERESTED_RESP_CMD* cmd)
{
	_int32 ret;
	_u32 offset;
	char* tmp_buf;
	_int32  tmp_len;
	cmd->_header._version = P2P_PROTOCOL_VERSION;
	cmd->_header._command_len = 6;
	cmd->_header._command_type = INTERESTED_RESP;
	*len = cmd->_header._command_len + 8;
	ret = ptl_malloc_cmd_buffer(cmd_buffer, len, &offset, device);
	CHECK_VALUE(ret);
	tmp_buf = *cmd_buffer + offset;
	tmp_len = *len - offset;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._command_len);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_header._command_type);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_download_ratio);
	sd_assert(cmd->_block_num == 0);		/*this version not implement upload*/
	sd_assert(tmp_len == 4);
	ret = sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_block_num);
	if(ret != SUCCESS)
	{
		LOG_ERROR("build_interested_resp_cmd failed, errcode = %d", ret);
		ptl_free_cmd_buffer(*cmd_buffer);
	}
	return ret;
}

_int32 is_p2p_cmd_valid(_u8 cmd_type)
{
	LOG_DEBUG("is_p2p_cmd_valid, cmd_type:%u.", cmd_type);

	return (cmd_type < HANDSHAKE || cmd_type > FIN_RESP) ? -1 : 0;
}
