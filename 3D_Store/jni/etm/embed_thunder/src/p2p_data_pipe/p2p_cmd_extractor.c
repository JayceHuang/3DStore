#include "p2p_cmd_extractor.h"
#include "p2p_memory_slab.h"
#include "utility/bytebuffer.h"
#include "utility/mempool.h"
#include "utility/utility.h"
#include "utility/string.h"
#include "utility/logid.h"
#define  LOGID	 LOGID_P2P_PIPE
#include "utility/logger.h"

_int32 extract_handshake_cmd(char* buffer, _u32 len, HANDSHAKE_CMD* cmd)
{
	_int32 ret = SUCCESS;
	char* tmp_buf;
	_int32  tmp_len;
	sd_memset(cmd, 0, sizeof(HANDSHAKE_CMD));
	tmp_buf = buffer;
	tmp_len = (_int32)len;
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._version);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._command_len);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_header._command_type);
	sd_assert(cmd->_header._command_len + 8 == len);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_handshake_connect_id);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_by_what);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_gcid_len);
	if(cmd->_gcid_len != CID_SIZE)
	{
		LOG_ERROR("[remote peer version = %u]extract_handshake_cmd failed, cmd->_gcid_len = %u", cmd->_header._version, cmd->_gcid_len);
		return ERR_P2P_INVALID_COMMAND;
	}
	sd_get_bytes(&tmp_buf, &tmp_len, (char*)cmd->_gcid, (_int32)cmd->_gcid_len);
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&cmd->_file_size);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_file_status);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_peerid_len);
	if(cmd->_peerid_len != PEER_ID_SIZE)
	{
		LOG_ERROR("[remote peer version = %u]extract_handshake_cmd failed, cmd->_peerid_len = %u", cmd->_header._version, cmd->_peerid_len);
		return ERR_P2P_INVALID_COMMAND;
	}
	sd_get_bytes(&tmp_buf, &tmp_len, cmd->_peerid, (_int32)cmd->_peerid_len);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_internal_addr_len);
	if(cmd->_internal_addr_len >= 24)
	{
		LOG_ERROR("[remote peer version = %u]extract_handshake_cmd failed, cmd->_internal_addr_len = %u", cmd->_header._version, cmd->_internal_addr_len);
		return ERR_P2P_INVALID_COMMAND;
	}
	sd_get_bytes(&tmp_buf, &tmp_len, cmd->_internal_addr, (_int32)cmd->_internal_addr_len);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_tcp_listen_port);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_product_ver);
	if(cmd->_header._version > P2P_PROTOCOL_VERSION_51)
	{
		sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&cmd->_downbytes_in_history);
		sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&cmd->_upbytes_in_history);
		ret = sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_not_in_nat);
	}
	if(cmd->_header._version > P2P_PROTOCOL_VERSION_54)
	{
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_upload_speed_limit);
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_same_nat_tcp_speed_max);
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_diff_nat_tcp_speed_max);
		ret = sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_udp_speed_max);
	}
	if(cmd->_header._version > P2P_PROTOCOL_VERSION_57)
	{
		ret = sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_p2p_capability);
	}
	if(cmd->_header._version > P2P_PROTOCOL_VERSION_58)
	{
		ret = sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_phub_res_count);
	}
	if(cmd->_header._version > P2P_PROTOCOL_VERSION_60)
	{
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_load_level);
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_home_location_len);
		ret = sd_get_bytes(&tmp_buf, &tmp_len, cmd->_home_location, MIN(255, cmd->_home_location_len));
	}
	if(ret != SUCCESS)
	{
		LOG_ERROR("[remote peer version = %u]extract_handshake_cmd failed, ret = %d", cmd->_header._version, ret);
		return ERR_P2P_INVALID_COMMAND;
	}
	if(tmp_len > 0)
	{
		LOG_ERROR("[remote peer version = %u]extract_handshake_cmd, but last %u bytes is unknown how to extract.", cmd->_header._version, tmp_len);
	}
	return SUCCESS;
}

_int32 extract_handshake_resp_cmd(char* buffer, _u32 len, HANDSHAKE_RESP_CMD* cmd)
{
	_int32 ret = SUCCESS;
	char* tmp_buf;
	_int32  tmp_len;
	sd_memset(cmd, 0, sizeof(HANDSHAKE_RESP_CMD));
	tmp_buf = buffer;
	tmp_len = (_int32)len;
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._version);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._command_len);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_header._command_type);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_result);
	if(cmd->_result != P2P_RESULT_SUCC)
		return SUCCESS;					/*no need to extract left bytes*/
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_peerid_len);
	if(cmd->_peerid_len != PEER_ID_SIZE)
	{
		LOG_ERROR("[remote peer version = %u]extract_handshake_resp_cmd failed, cmd->_peerid_len = %u", cmd->_header._version, cmd->_peerid_len);
		return ERR_P2P_INVALID_COMMAND;
	}
	sd_get_bytes(&tmp_buf, &tmp_len, cmd->_peerid, (_int32)cmd->_peerid_len);
	ret = sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_product_ver);
	if(cmd->_header._version > P2P_PROTOCOL_VERSION_51)
	{
		sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&cmd->_downbytes_in_history);
		sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&cmd->_upbytes_in_history);
		ret = sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_not_in_nat);
	}
	if(cmd->_header._version > P2P_PROTOCOL_VERSION_54)
	{
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_upload_speed_limit);
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_same_nat_tcp_speed_max);
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_diff_nat_tcp_speed_max);
		ret = sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_udp_speed_max);
	}
	if(cmd->_header._version > P2P_PROTOCOL_VERSION_57)
	{
		ret = sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_p2p_capablility);
	}
	if(cmd->_header._version > P2P_PROTOCOL_VERSION_58)
	{
		ret = sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_phub_res_count);
	}
	if(cmd->_header._version >= P2P_PROTOCOL_VERSION_60)
	{
		sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_load_level);
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_home_location_len);
		ret = sd_get_bytes(&tmp_buf, &tmp_len, cmd->_home_location, MIN(255, cmd->_home_location_len));
	}
	if(ret != SUCCESS)
	{
		LOG_ERROR("[remote peer version = %u]extract_handshake_resp_cmd failed, ret = %d", cmd->_header._version, ret);
		return ERR_P2P_INVALID_COMMAND;
	}
	if(tmp_len > 0)
	{
		LOG_ERROR("[remote peer version = %u]extract_handshake_resp_cmd, but last %u bytes is unknown how to extract.", cmd->_header._version, tmp_len);
	}
	return SUCCESS;
}


_int32 extract_interested_cmd(char* buffer, _u32 len, INTERESTED_CMD* cmd)
{
	_int32 ret = SUCCESS;
	char* tmp_buf;
	_int32 tmp_len;
	sd_memset(cmd, 0, sizeof(INTERESTED_CMD));
	tmp_buf = buffer;
	tmp_len = (_int32)len;
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._version);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._command_len);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_header._command_type);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_by_what);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_min_block_size);
	ret = sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_max_block_count);
	if(ret != SUCCESS)
	{
		LOG_ERROR("[remote peer version = %u]extract_interested_cmd failed, ret = %d", cmd->_header._version, ret);
		return ERR_P2P_INVALID_COMMAND;
	}
	if(tmp_len > 0)
	{
		LOG_ERROR("[remote peer version = %u]extract_interested_cmd, but last %u bytes is unknown how to extract.", cmd->_header._version, tmp_len);
	}
	return SUCCESS;
}

_int32 extract_interested_resp_cmd(char* buffer, _u32 len, INTERESTED_RESP_CMD* cmd)
{
	_int32 ret = SUCCESS;
	char* tmp_buf;
	_int32  tmp_len;
	sd_memset(cmd, 0, sizeof(INTERESTED_RESP_CMD));
	tmp_buf = buffer;
	tmp_len = (_int32)len;
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._version);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._command_len);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_header._command_type);
	sd_assert(cmd->_header._command_len + 8 == len);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_download_ratio);
	ret = sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_block_num);
	if(ret != SUCCESS)
	{
		LOG_ERROR("[remote peer version = %u]extract_interested_resp_cmd failed, ret = %d", cmd->_header._version, ret);
		return ERR_P2P_INVALID_COMMAND;
	}
	if(cmd->_block_num == 0)
	{
		sd_assert(tmp_len == 0);	
		return SUCCESS;
	}
	cmd->_block_data_len = tmp_len;
	cmd->_block_data = tmp_buf;
	return SUCCESS;
}


_int32 extract_request_cmd(char* buffer, _u32 len, REQUEST_CMD* cmd)
{
	_int32 ret = SUCCESS;
	char* tmp_buf = NULL;
	_int32 tmp_len = 0;
	sd_memset(cmd, 0, sizeof(REQUEST_CMD));
	tmp_buf = buffer;
	tmp_len = (_int32)len;
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._version);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._command_len);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_header._command_type);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_by_what);
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&cmd->_file_pos);
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&cmd->_file_len);
	ret = sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_max_package_size);
	if(cmd->_header._version >= P2P_PROTOCOL_VERSION_57)
	{
		ret = sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_priority);
	}
	if(cmd->_header._version >= P2P_PROTOCOL_VERSION_58)
	{
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_upload_speed);
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_unchoke_num);
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_pipe_num);
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_task_num);
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_local_requested);
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_remote_requested);
		ret = sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_file_ratio);
	}
	if(ret != SUCCESS)
	{
		LOG_ERROR("[remote peer version = %u]extract_request_cmd failed, ret = %d", cmd->_header._version, ret);
		return ERR_P2P_INVALID_COMMAND;
	}
	if(tmp_len > 0)
	{
		LOG_ERROR("[remote peer version = %u]extract_request_cmd, but last %u bytes is unknown how to extract.", cmd->_header._version, tmp_len);
	}
	return SUCCESS;
}


_int32 extract_request_resp_cmd(char* buffer, _u32 len, REQUEST_RESP_CMD* cmd)
{
	_int32 ret = SUCCESS;
	char* tmp_buf;
	_int32  tmp_len;
	sd_memset(cmd, 0, sizeof(REQUEST_RESP_CMD));
	tmp_buf = buffer;
	tmp_len = (_int32)len;
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._version);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._command_len);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_header._command_type);
	ret = sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_result);
	if(cmd->_header._version >= P2P_PROTOCOL_VERSION_54)
	{
		sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&cmd->_data_pos);
		ret = sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_data_len);
	}
	if(ret != SUCCESS)
	{
		LOG_ERROR("[remote peer version = %u]extract_request_resp_cmd failed, ret = %d", cmd->_header._version, ret);
		return ERR_P2P_INVALID_COMMAND;
	}
	cmd->_data = tmp_buf;
	return SUCCESS;
}

_int32 extract_cancel_resp_cmd(char* buffer, _u32 len, CANCEL_RESP_CMD* cmd)
{
	_int32 ret = SUCCESS;
	char* tmp_buf = buffer;
	_int32  tmp_len = (_int32)len;
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._version);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._command_len);
	ret = sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_header._command_type);
	if(ret != SUCCESS)
	{
		LOG_ERROR("[remote peer version = %u]extract_cancel_resp_cmd failed, ret = %d", cmd->_header._version, ret);
		return ERR_P2P_INVALID_COMMAND;
	}
	if(tmp_len > 0)
	{
		LOG_ERROR("[remote peer version = %u]extract_cancel_resp_cmd, but last %u bytes is unknown how to extract.", cmd->_header._version, tmp_len);
	}
	return SUCCESS;
}
