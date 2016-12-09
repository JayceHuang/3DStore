#include "utility/mempool.h"
#include "utility/bytebuffer.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_P2P_TRANSFER_LAYER
#include "utility/logger.h"

#include "ptl_cmd_builder.h"

_int32 ptl_build_ping_cmd(char** buffer, _u32* len, PING_CMD* cmd)
{
	_int32	ret, i;
	char*	tmp_buf;
	_int32	tmp_len;
	*len = 90;
	ret = sd_malloc(*len, (void**)buffer);
	if (ret != SUCCESS)
	{
		return ret;
	}
	tmp_buf = *buffer;
	tmp_len = (_int32)*len;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_version);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_cmd_type);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_local_peerid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_local_peerid, cmd->_local_peerid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, (char* )&cmd->_internal_ip, sizeof(_u32));
	sd_set_bytes(&tmp_buf, &tmp_len, (char* )&cmd->_natwork_mask, sizeof(_u32));
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_listen_port);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_product_flag);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_product_version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_sn_list_size);
	for (i = 0; i < cmd->_sn_list_size; ++i)
	{
		sd_assert(FALSE);
	}
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_network_type);
	sd_set_bytes(&tmp_buf, &tmp_len, (char* )&cmd->_upnp_ip, sizeof(_u32));
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_upnp_port);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_online_time);	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_download_bytes);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_upload_bytes);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_upload_resource_number);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_cur_uploading_number);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_cur_download_task_number);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_cur_uploading_connect_num);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_cur_download_speed);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_cur_upload_speed);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_max_download_speed);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_max_upload_speed);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_auto_upload_speed_limit);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_user_upload_speed_limit);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_download_speed_limit);
	ret = sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_peer_status_flag);
	sd_assert(ret == SUCCESS && tmp_len == 0);
	if (ret != SUCCESS)
	{
		sd_free(*buffer);
		*buffer = NULL;
	}
	return ret;
}

_int32 ptl_build_logout_cmd(char** buffer, _u32* len, LOGOUT_CMD* cmd)
{
	_int32	ret;
	char*	tmp_buf;
	_int32	tmp_len;
	*len = 29;
	ret = sd_malloc(*len, (void**)buffer);
	if (ret != SUCCESS)
	{
		return ret;
	}
	tmp_buf = *buffer;
	tmp_len = (_int32)*len;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_version);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_cmd_type);
	sd_set_bytes(&tmp_buf, &tmp_len, (char* )&cmd->_ip, sizeof(_u32));
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_local_peerid_len);
	ret = sd_set_bytes(&tmp_buf, &tmp_len, cmd->_local_peerid, cmd->_local_peerid_len);
	if (ret != SUCCESS)
	{
		sd_free(*buffer);
		*buffer = NULL;
	}
	return ret;
}

_int32 ptl_build_get_peersn_cmd(char** buffer, _u32* len, GET_PEERSN_CMD* cmd)
{
	_int32 ret;
	char* tmp_buf;
	_int32  tmp_len;
	cmd->_version = PTL_PROTOCOL_VERSION;
	cmd->_cmd_type = GET_PEERSN;
	*len = GET_PEERSN_CMD_LEN;
	ret = sd_malloc(*len, (void**)buffer);
	if (ret != SUCCESS)
	{
		LOG_DEBUG("ptl_build_get_peersn_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	tmp_buf = *buffer;
	tmp_len = (_int32)*len;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_version);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_cmd_type);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_target_peerid_len);
	sd_assert(tmp_len == PEER_ID_SIZE);
	ret = sd_set_bytes(&tmp_buf, &tmp_len, cmd->_target_peerid, cmd->_target_peerid_len);
	sd_assert(ret == SUCCESS);
	if (ret != SUCCESS)
	{
		LOG_ERROR("ptl_build_get_peersn_cmd failed, errcode = %d", ret);
		sd_free(*buffer);
		*buffer = NULL;
	}
	return ret;
}

_int32 ptl_build_get_mysn_cmd(char** buffer, _u32* len, GET_MYSN_CMD* cmd)
{
	_int32 ret = SUCCESS;
	char* tmp_buf = NULL;
	_int32 tmp_len = 0;
	cmd->_version = PTL_PROTOCOL_VERSION;
	cmd->_cmd_type = GET_MYSN;
	if (cmd->_invalid_sn_num == 0)
	{
	    *len = 29;
	}
	else
	{
	    *len = 49;
	}
	ret = sd_malloc(*len, (void**)buffer);
	if (ret != SUCCESS)
	{
		LOG_DEBUG("ptl_build_get_my_sn_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	tmp_buf = *buffer;
	tmp_len = (_int32)*len;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_version);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_cmd_type);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_local_peerid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_local_peerid, cmd->_local_peerid_len);
	ret = sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_invalid_sn_num);
	if (cmd->_invalid_sn_num > 0)
	{
		sd_assert(cmd->_invalid_sn_num == 1);
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_invalid_sn_peerid_len);
		ret = sd_set_bytes(&tmp_buf, &tmp_len, cmd->_invalid_sn_peerid, cmd->_invalid_sn_peerid_len);
	}
	sd_assert(ret == SUCCESS);
	if (ret != SUCCESS)
	{
		LOG_ERROR("ptl_build_get_my_sn_cmd failed, errcode = %d.", ret);
		sd_free(*buffer);
		*buffer = NULL;
	}
	return ret;
}

_int32 ptl_build_ping_sn_cmd(char** buffer, _u32* len, PING_SN_CMD* cmd)
{
	_int32 ret = SUCCESS;
	char* tmp_buf = NULL;
	_int32 tmp_len = 0;
	cmd->_version = PTL_PROTOCOL_VERSION;
	cmd->_cmd_type = PING_SN;
	*len = 39;
	ret = sd_malloc(*len, (void**)buffer);
	if (ret != SUCCESS)
	{
		LOG_DEBUG("ptl_build_ping_sn_cmd, malloc failed.");
		return ret;
	}
	tmp_buf = *buffer;
	tmp_len = (_int32)*len;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_version);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_cmd_type);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_local_peerid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_local_peerid, (_int32)cmd->_local_peerid_len);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_nat_type);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_latest_port);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_time_elapsed);
	ret = sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_delta_port);
	if (ret != SUCCESS)
	{
		LOG_ERROR("ptl_build_ping_sn_cmd failed, errcode = %d.", ret);
		sd_free(*buffer);
		*buffer = NULL;
	}
	return ret;
}

_int32 ptl_build_nn2sn_logout_cmd(char** buffer, _u32* len, NN2SN_LOGOUT_CMD* cmd)
{
	_int32 ret = SUCCESS;
	char* tmp_buf = NULL;
	_int32 tmp_len = 0;
	cmd->_version = PTL_PROTOCOL_VERSION;
	cmd->_cmd_type = NN2SN_LOGOUT;
	*len = 25;
	ret = sd_malloc(*len, (void**)buffer);
	if (ret != SUCCESS)
	{
		LOG_DEBUG("ptl_build_ping_sn_cmd, malloc failed.");
		return ret;
	}
	tmp_buf = *buffer;
	tmp_len = (_int32)*len;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_version);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_cmd_type);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_local_peerid_len);
	ret = sd_set_bytes(&tmp_buf, &tmp_len, cmd->_local_peerid, (_int32)cmd->_local_peerid_len);
	if (ret != SUCCESS)
	{
		LOG_ERROR("ptl_build_ping_sn_cmd failed, errcode = %d.", ret);
		sd_free(*buffer);
		*buffer = NULL;
	}
	return ret;
}

_int32 ptl_build_punch_hole_cmd(char** buffer, _u32* len, PUNCH_HOLE_CMD* cmd)
{
	_int32 ret = SUCCESS;
	char* tmp_buf = NULL;
	_int32 tmp_len = 0;
	cmd->_version = PTL_PROTOCOL_VERSION;
	cmd->_cmd_type = PUNCH_HOLE;
	*len = 29;
	ret = sd_malloc(*len, (void**)buffer);
	if (ret != SUCCESS)
	{
		LOG_DEBUG("ptl_build_punch_hole_cmd, malloc failed.");
		return ret;
	}
	tmp_buf = *buffer;
	tmp_len = (_int32)*len;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_version);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_cmd_type);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_len);
	sd_assert(cmd->_peerid_len == PEER_ID_SIZE);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, cmd->_peerid_len);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_source_port);
	ret = sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_target_port);
	if (ret != SUCCESS)
	{
		LOG_ERROR("ptl_build_punch_hole_cmd failed, errcode = %d.", ret);
		sd_free(*buffer);
		*buffer = NULL;
	}
	return ret;
}

_u64 ptl_header_hash(const char * buffer, _u32 len)
{
    _u64 nr  = 1;
    _u64 nr2 = 4; 
    while (len--) 
    { 
        nr^= (((nr & 63)+nr2)*((_u64) (_u8) *buffer++))+(nr << 8); 
        nr2 += 3; 
    }  
    return nr;
}

_int32 ptl_build_transfer_layer_control_cmd(char** buffer, _u32* len, TRANSFER_LAYER_CONTROL_CMD* cmd)
{
	_int32 ret = SUCCESS;
	char* tmp_buf = NULL;
	_int32 tmp_len = 0;
	cmd->_version = PTL_PROTOCOL_VERSION;
	cmd->_cmd_type = TRANSFER_LAYER_CONTROL;
	cmd->_cmd_len = 13;
	*len = 21;
	ret = sd_malloc(*len, (void**)buffer);
	if (ret != SUCCESS)
	{
		LOG_DEBUG("ptl_build_transfer_layer_control_cmd, malloc failed.");
		return ret;
	}
	tmp_buf = *buffer;
	tmp_len = (_int32)*len;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cmd_len);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_cmd_type);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_broker_seq);
	cmd->_header_hash = ptl_header_hash(*buffer, 13);
	ret = sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_header_hash);
	if(ret != SUCCESS)
	{
		LOG_ERROR("ptl_build_transfer_layer_control_cmd failed, errcode = %d.", ret);
	}
	return ret;
}

_int32 ptl_build_transfer_layer_control_resp_cmd(char** buffer, _u32* len, TRANSFER_LAYER_CONTROL_RESP_CMD* cmd)
{
	_int32 ret = SUCCESS;
	char* tmp_buf = NULL;
	_int32 tmp_len = 0;
	cmd->_version = PTL_PROTOCOL_VERSION;
	cmd->_cmd_len = 5;
	cmd->_cmd_type = TRANSFER_LAYER_CONTROL_RESP;
	*len = 13;
	ret = sd_malloc(*len, (void**)buffer);
	if (ret != SUCCESS)
	{
		LOG_DEBUG("ptl_build_transfer_layer_control_resp_cmd, malloc failed.");
		return ret;
	}
	tmp_buf = *buffer;
	tmp_len = (_int32)*len;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cmd_len);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_cmd_type);
	ret = sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_result);
	if (ret != SUCCESS)
	{
		LOG_ERROR("ptl_build_transfer_layer_control_resp_cmd failed, errcode = %d.", ret);
		sd_free(*buffer);
		*buffer = NULL;
	}
	return ret;
}
	
_int32 ptl_build_broker2_req_cmd(char** buffer, _u32* len, BROKER2_REQ_CMD* cmd)
{
	_int32 ret = SUCCESS;
	char* tmp_buf = NULL;
	_int32 tmp_len = 0;
	cmd->_version = PTL_PROTOCOL_VERSION;
	cmd->_cmd_type = BROKER2_REQ;
	*len = 35;
	ret = sd_malloc(*len, (void**)buffer);
	if (ret != SUCCESS)
	{
		LOG_DEBUG("ptl_build_broker2_req_cmd, malloc failed.");
		return ret;
	}
	tmp_buf = *buffer;
	tmp_len = (_int32)*len;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_version);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_cmd_type);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_seq_num);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_requestor_ip);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_requestor_port);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_remote_peerid_len);
	sd_assert(cmd->_remote_peerid_len == PEER_ID_SIZE);
	ret = sd_set_bytes(&tmp_buf, &tmp_len, cmd->_remote_peerid, cmd->_remote_peerid_len);
	if (ret != SUCCESS)
	{
		LOG_ERROR("ptl_build_broker2_req_cmd failed, errcode = %d.", ret);
		sd_free(*buffer);
		*buffer = NULL;
	}
	return ret;
}

_int32 ptl_build_udp_broker_req_cmd(char** buffer, _u32* len, UDP_BROKER_REQ_CMD* cmd)
{
	_int32 ret = SUCCESS;
	char* tmp_buf = NULL;
	_int32 tmp_len = 0;
	cmd->_version = PTL_PROTOCOL_VERSION;
	cmd->_cmd_type = UDP_BROKER_REQ;
	*len = 55;
	ret = sd_malloc(*len, (void**)buffer);
	if (ret != SUCCESS)
	{
		LOG_DEBUG("ptl_build_udp_broker_req_cmd, malloc failed.");
		return ret;
	}
	tmp_buf = *buffer;
	tmp_len = (_int32)*len;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_version);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_cmd_type);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_seq_num);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_requestor_ip);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_requestor_udp_port);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_remote_peerid_len);
	sd_assert(cmd->_remote_peerid_len == PEER_ID_SIZE);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_remote_peerid, cmd->_remote_peerid_len);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_requestor_peerid_len);
	ret = sd_set_bytes(&tmp_buf, &tmp_len, cmd->_requestor_peerid, cmd->_requestor_peerid_len);
	if (ret != SUCCESS)
	{
		LOG_ERROR("ptl_build_udp_broker_req_cmd failed, errcode = %d.", ret);
		sd_free(*buffer);
		*buffer = NULL;
	}
	return ret;
}

_int32 ptl_build_icallsomeone_cmd(char** buffer, _u32* len, ICALLSOMEONE_CMD* cmd)
{
	_int32 ret;
	char*  tmp_buf;
	_int32 tmp_len;
	*len = 61;
	ret = sd_malloc(*len, (void**)buffer);
	if (ret != SUCCESS)
	{
		LOG_DEBUG("ptl_build_icallsomeone_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	tmp_buf = *buffer;
	tmp_len = (_int32)*len;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_version);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_cmd_type);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_local_peerid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_local_peerid, cmd->_local_peerid_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_remote_peerid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_remote_peerid, cmd->_remote_peerid_size);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_virtual_port);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_nat_type);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_latest_extenal_port);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_time_elapsed);
	ret = sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->_delta_port);
	sd_assert(ret == SUCCESS);
	sd_assert(tmp_len == 0);
	if (ret != SUCCESS)
	{
		LOG_ERROR("ptl_build_icallsomeone_cmd failed, errcode = %d", ret);
		sd_free(*buffer);
		*buffer = NULL;
	}
	return ret;
}
