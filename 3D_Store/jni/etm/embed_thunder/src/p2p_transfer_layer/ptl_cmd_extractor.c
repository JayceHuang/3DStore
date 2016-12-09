#include "ptl_cmd_extractor.h"
#include "ptl_memory_slab.h"
#include "utility/bytebuffer.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_P2P_TRANSFER_LAYER
#include "utility/logger.h"

_int32 ptl_extract_someonecallyou_cmd(char* buffer, _u32 len, SOMEONECALLYOU_CMD* cmd)
{
	_int32 ret = SUCCESS;
	char* tmp_buf = buffer;
	_int32 tmp_len = (_int32)len;
	sd_memset(cmd, 0, sizeof(SOMEONECALLYOU_CMD));
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_version);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_cmd_type);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_source_peerid_len);
	if(cmd->_source_peerid_len != PEER_ID_SIZE)
		return -1;
	sd_get_bytes(&tmp_buf, &tmp_len, cmd->_source_peerid, cmd->_source_peerid_len);
	sd_memcpy(&cmd->_source_ip, tmp_buf, sizeof(_u32));
	tmp_buf += sizeof(_u32);
	tmp_len -= sizeof(_u32);
	sd_memcpy(&cmd->_source_port, tmp_buf, sizeof(_u16));
	tmp_buf += sizeof(_u16);
	tmp_len -= sizeof(_u16);
	sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_virtual_port);
	ret = sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_nat_type);
	sd_memcpy(&cmd->_latest_remote_port, tmp_buf, sizeof(_u16));
	tmp_buf += sizeof(_u16);
	tmp_len -= sizeof(_u16);
	sd_memcpy(&cmd->_guess_remote_port, tmp_buf, sizeof(_u16));
	tmp_buf += sizeof(_u16);
	tmp_len -= sizeof(_u16);
	if(tmp_len > 0)
		ret = sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_udt_version);
	if(tmp_len > 0)
		ret = sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_mhxy_version);
	if(ret != SUCCESS)
	{
		LOG_ERROR("[remote peer version = %u]ptl_extract_someonecallyou_cmd failed, ret = %d", cmd->_version, ret);
		return -1;
	}
	if(tmp_len > 0)
	{
		LOG_ERROR("[remote peer version = %u]ptl_extract_someonecallyou_cmd, but last %u bytes is unknown how to extract", cmd->_version, tmp_len);
	}
	return ret;
}

_int32 ptl_extract_sn2nn_logout_cmd(char* buffer, _u32 len, SN2NN_LOGOUT_CMD* cmd)
{
	_int32 ret = SUCCESS;
	char* tmp_buf = buffer;
	_int32 tmp_len = (_int32)len;
	sd_memset(cmd, 0, sizeof(SN2NN_LOGOUT_CMD));
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_version);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_cmd_type);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_sn_peerid_len);
	if(cmd->_sn_peerid_len != PEER_ID_SIZE)
		return -1;
	ret = sd_get_bytes(&tmp_buf, &tmp_len, cmd->_sn_peerid, cmd->_sn_peerid_len);
	if(ret != SUCCESS)
	{
		LOG_ERROR("[remote peer version = %u]ptl_extract_sn2nn_logout_cmd failed, ret = %d", cmd->_version, ret);
		return -1;
	}
	if(tmp_len > 0)
	{
		LOG_ERROR("[remote peer version = %u]ptl_extract_sn2nn_logout_cmd, but last %u bytes is unknown how to extract", cmd->_version, tmp_len);
	}
	return SUCCESS;
}

_int32 ptl_extract_get_mysn_resp_cmd(char* buffer, _u32 len, GET_MYSN_RESP_CMD* cmd)
{
	_int32 ret = SUCCESS;
	char* tmp_buf = buffer;
	_int32 tmp_len = (_int32)len;
	sd_memset(cmd, 0, sizeof(GET_MYSN_RESP_CMD));
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_version);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_cmd_type);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_result);
	sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_max_sn);
	ret = sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_mysn_array_num);
	if(cmd->_mysn_array_num > 0)
	{
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_mysn_peerid_len);
		if(cmd->_mysn_peerid_len != PEER_ID_SIZE)
			return -1;
		sd_assert(cmd->_mysn_array_num == 1);
		sd_get_bytes(&tmp_buf, &tmp_len, cmd->_mysn_peerid, cmd->_mysn_peerid_len);
		sd_memcpy(&cmd->_mysn_ip, tmp_buf, sizeof(_u32));
		tmp_buf += sizeof(_u32);
		tmp_len -= sizeof(_u32);
		sd_memcpy(&cmd->_mysn_port, tmp_buf, sizeof(_u16));
		tmp_buf += sizeof(_u16);
		tmp_len -= sizeof(_u16);
	}
	if(ret != SUCCESS)
	{
		LOG_ERROR("[remote peer version = %u]ptl_extract_get_mysn_resp_cmd failed, ret = %d", cmd->_version, ret);
		return -1;
	}
	if(tmp_len > 0)
	{
		LOG_ERROR("[remote peer version = %u]ptl_extract_get_mysn_resp_cmd, but last %u bytes is unknown how to extract", cmd->_version, tmp_len);
	}
	return SUCCESS;
}

_int32 ptl_extract_ping_sn_resp_cmd(char* buffer, _u32 len, PING_SN_RESP_CMD* cmd)
{
	_int32 ret = SUCCESS;
	char* tmp_buf = buffer;
	_int32 tmp_len = (_int32)len;
	sd_memset(cmd, 0, sizeof(PING_SN_RESP_CMD));
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_version);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_cmd_type);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_sn_peerid_len);
	if(cmd->_sn_peerid_len != PEER_ID_SIZE)
		return -1;
	ret = sd_get_bytes(&tmp_buf, &tmp_len, cmd->_sn_peerid, cmd->_sn_peerid_len);
	if(ret != SUCCESS)
	{
		LOG_ERROR("[remote peer version = %u]ptl_extract_ping_sn_resp_cmd failed, ret = %d", cmd->_version, ret);
		return -1;
	}
	if(tmp_len > 0)
	{
		LOG_ERROR("[remote peer version = %u]ptl_extract_ping_sn_resp_cmd, but last %u bytes is unknown how to extract", cmd->_version, tmp_len);
	}
	return SUCCESS;	
}

_int32 ptl_extract_get_peersn_resp_cmd(char* buffer, _u32 len, GET_PEERSN_RESP_CMD* cmd)
{
	_int32 ret = SUCCESS;
	char* tmp_buf = buffer;
	_int32  tmp_len = (_int32)len;
	sd_memset(cmd, 0, sizeof(GET_PEERSN_RESP_CMD));
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_version);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_cmd_type);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_result);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_sn_num);

    LOG_DEBUG("cmd->_sn_num = %u.", cmd->_sn_num);
    if (cmd->_sn_num != 0)
    {
        sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_sn_peerid_len);
        if(cmd->_sn_peerid_len != PEER_ID_SIZE)
        {
            LOG_ERROR("[remote peer version = %u]extract_get_peersn_resp_cmd failed, sn_peerid_len = %u", cmd->_version, cmd->_sn_peerid_len);
            return -1;
        }
        sd_get_bytes(&tmp_buf, &tmp_len, cmd->_sn_peerid, (_int32)cmd->_sn_peerid_len);
        sd_memcpy(&cmd->_sn_ip, tmp_buf, sizeof(_u32));
        tmp_buf += sizeof(_u32);
        tmp_len -= sizeof(_u32);
        sd_memcpy(&cmd->_sn_port, tmp_buf, sizeof(_u16));
        tmp_buf += sizeof(_u16);
        tmp_len -= sizeof(_u16);
        sd_assert(tmp_len >= 0);
    }
    else
    {
        if(cmd->_version > PTL_PROTOCOL_VERSION_50)
        {
            sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_peerid_len);
            if(cmd->_peerid_len != PEER_ID_SIZE)
            {
                LOG_ERROR("[remote peer version = %u]extract_get_peersn_resp_cmd failed, cmd->_peerid_len = %u", 
                    cmd->_version, cmd->_peerid_len);
                return -1;
            }
            ret = sd_get_bytes(&tmp_buf, &tmp_len, cmd->_peerid, cmd->_peerid_len);
            ret = sd_get_int32_from_lt(&tmp_buf, &tmp_len, &cmd->_nat_type);
        }
    }

	if(ret != SUCCESS)
	{
		LOG_ERROR("[remote peer version = %u]extract_get_peersn_resp_cmd failed, ret = %d", cmd->_version, ret);
		return ERR_P2P_INVALID_COMMAND;
	}
	if(tmp_len > 0)
	{
		LOG_ERROR("[remote peer version = %u]extract_get_peersn_resp_cmd, but last %u bytes is unknown how to extract", cmd->_version, tmp_len);
	}
	return SUCCESS;
}

_int32 ptl_extract_broker2_cmd(char* buffer, _u32 len, BROKER2_CMD* cmd)
{
	_int32 ret = SUCCESS;
	char* tmp_buf = buffer;
	_int32 tmp_len = (_int32)len;
	sd_memset(cmd, 0, sizeof(BROKER2_CMD));
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_version);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_cmd_type);
	ret = sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_seq_num);
	sd_memcpy(&cmd->_ip, tmp_buf, sizeof(_u32));
	tmp_buf += sizeof(_u32);
	tmp_len -= sizeof(_u32);
	sd_memcpy(&cmd->_tcp_port, tmp_buf, sizeof(_u16));
	tmp_buf += sizeof(_u16);
	tmp_len -= sizeof(_u16);
	if(ret != SUCCESS)
	{
		LOG_ERROR("[remote peer version = %u]ptl_extract_broker2_cmd failed, ret = %d", cmd->_version, ret);
		return -1;
	}
	if(tmp_len > 0)
	{
		LOG_ERROR("[remote peer version = %u]ptl_extract_broker2_cmd, but last %u bytes is unknown how to extract", cmd->_version, tmp_len);
	}
	return SUCCESS;
}

_int32 ptl_extract_udp_broker_cmd(char* buffer, _u32 len, UDP_BROKER_CMD* cmd)
{
	_int32 ret = SUCCESS;
	char* tmp_buf = buffer;
	_int32 tmp_len = (_int32)len;
	sd_memset(cmd, 0, sizeof(UDP_BROKER_CMD));
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_version);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_cmd_type);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_seq_num);
	sd_memcpy(&cmd->_ip, tmp_buf, sizeof(_u32));
	tmp_buf += sizeof(_u32);
	tmp_len -= sizeof(_u32);
	sd_memcpy(&cmd->_udp_port, tmp_buf, sizeof(_u16));
	tmp_buf += sizeof(_u16);
	tmp_len -= sizeof(_u16);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_peerid_len);
	if(cmd->_peerid_len != PEER_ID_SIZE)
		return -1;
	ret = sd_get_bytes(&tmp_buf, &tmp_len, cmd->_peerid, cmd->_peerid_len);
	if(ret != SUCCESS)
	{
		LOG_ERROR("[remote peer version = %u]ptl_extract_udp_broker_cmd failed, ret = %d", cmd->_version, ret);
		return -1;
	}
	if(tmp_len > 0)
	{
		LOG_ERROR("[remote peer version = %u]ptl_extract_udp_broker_cmd, but last %u bytes is unknown how to extract", cmd->_version, tmp_len);
	}
	return SUCCESS;
}

_int32 ptl_extract_transfer_layer_control_cmd(char* buffer, _u32 len, TRANSFER_LAYER_CONTROL_CMD* cmd)
{
	_int32 ret = SUCCESS;
	char* tmp_buf = buffer;
	_int32 tmp_len = (_int32)len;
	sd_memset(cmd, 0, sizeof(TRANSFER_LAYER_CONTROL_CMD));
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_version);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_cmd_len);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_cmd_type);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_broker_seq);
	ret = sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&cmd->_header_hash);
	if(ret != SUCCESS)
	{
		LOG_ERROR("[remote peer version = %u]ptl_extract_transfer_layer_control_cmd failed, ret = %d", cmd->_version, ret);
		return -1;
	}
	if(tmp_len > 0)
	{
		LOG_ERROR("[remote peer version = %u]ptl_extract_transfer_layer_control_cmd, but last %u bytes is unknown how to extract", cmd->_version, tmp_len);
	}
	return SUCCESS;
}

_int32 ptl_extract_transfer_layer_control_resp_cmd(char* buffer, _u32 len, TRANSFER_LAYER_CONTROL_RESP_CMD* cmd)
{
	_int32 ret = SUCCESS;
	char* tmp_buf = buffer;
	_int32 tmp_len = (_int32)len;
	sd_memset(cmd, 0, sizeof(TRANSFER_LAYER_CONTROL_RESP_CMD));
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_version);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_cmd_len);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_cmd_type);
	ret = sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_result);
	if(ret != SUCCESS)
	{
		LOG_ERROR("[remote peer version = %u]ptl_extract_transfer_layer_control_resp_cmd failed, ret = %d", cmd->_version, ret);
		return -1;
	}
	if(tmp_len > 0)
	{
		LOG_ERROR("[remote peer version = %u]ptl_extract_transfer_layer_control_resp_cmd, but last %u bytes is unknown how to extract", cmd->_version, tmp_len);
	}
	return SUCCESS;
}
	
_int32 ptl_extract_icallsomeone_resp_cmd(char* buffer, _u32 len, ICALLSOMEONE_RESP_CMD* cmd)
{
	_int32 ret = SUCCESS;
	char*  tmp_buf;
	_int32 tmp_len;
	sd_memset(cmd, 0, sizeof(ICALLSOMEONE_RESP_CMD));
	tmp_buf = buffer;
	tmp_len = (_int32)len;
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_version);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_cmd_type);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_sn_peerid_len);
	if(cmd->_sn_peerid_len != PEER_ID_SIZE)
	{
		LOG_ERROR("udt_extract_icallsomeone_resp_cmd failed, cmd->_sn_peerid_len != PEER_ID_SIZE.");
		return -1;
	}
	sd_get_bytes(&tmp_buf, &tmp_len, cmd->_sn_peerid, cmd->_sn_peerid_len);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_remote_peerid_len);
	if(cmd->_remote_peerid_len != PEER_ID_SIZE)
		return -1;
	sd_get_bytes(&tmp_buf, &tmp_len, cmd->_remote_peerid, cmd->_remote_peerid_len);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_is_on_line);
	if(cmd->_is_on_line == 0)
	{
		LOG_ERROR("ptl_extract_icallsomeone_resp_cmd failed, remote peer is not online.....%s" ,cmd->_sn_peerid);
		//sd_assert(tmp_len == 0 && ret == SUCCESS);
		return SUCCESS;
	}
	sd_get_bytes(&tmp_buf, &tmp_len, (char*)&cmd->_remote_ip, sizeof(_u32));
	sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_remote_port);
	sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_virtual_port);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_nat_type);
	ret = sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_latest_ex_port);
	if(cmd->_version > PTL_PROTOCOL_VERSION_58)
		ret = sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_guess_ex_port);
	if(ret != SUCCESS)
	{
		LOG_ERROR("[remote peer version = %u]ptl_extract_icallsomeone_resp_cmd failed, ret = %d", cmd->_version, ret);
		return -1;
	}
	if(tmp_len > 0)
	{
		LOG_ERROR("[remote peer version = %u]ptl_extract_icallsomeone_resp_cmd, but last %u bytes is unknown how to extract", cmd->_version, tmp_len);
	}
	return SUCCESS;
}

_int32 ptl_extract_punch_hole_cmd(char* buffer, _u32 len, PUNCH_HOLE_CMD* cmd)
{
	_int32	ret = SUCCESS;
	char*	tmp_buf;
	_int32	tmp_len;
	sd_memset(cmd, 0, sizeof(PUNCH_HOLE_CMD));
	tmp_buf = buffer;
	tmp_len = len;
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_version);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_cmd_type);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_peerid_len);
	if(cmd->_peerid_len != PEER_ID_SIZE)
		return -1;
	sd_get_bytes(&tmp_buf, &tmp_len, cmd->_peerid, cmd->_peerid_len);
	sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_source_port);
	ret = sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_target_port);
	if(ret != SUCCESS)
	{
		LOG_ERROR("[remote peer version = %u]udt_extract_punch_hole_cmd failed, ret = %d", cmd->_version, ret);
		return -1;
	}
	if(tmp_len > 0)
	{
		LOG_ERROR("[remote peer version = %u]udt_extract_punch_hole_cmd, but last %u bytes is unknown how to extract", cmd->_version, tmp_len);
	}
	return SUCCESS;
}





