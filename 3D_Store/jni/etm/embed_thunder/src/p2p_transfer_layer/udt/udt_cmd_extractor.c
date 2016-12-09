#include "udt_cmd_extractor.h"
#include "../ptl_cmd_define.h"
#include "utility/string.h"
#include "utility/bytebuffer.h"
#include "utility/errcode.h"
#include "utility/logid.h"
#define	LOGID	LOGID_P2P_TRANSFER_LAYER
#include "utility/logger.h"

//注意: ptl层所有命令的解析都不能依靠版本号,外面所有的迅雷版本号全为50

_int32	udt_extract_data_transfer_cmd(char* buffer, _u32 len, DATA_TRANSFER_CMD* cmd)
{
	_int32 ret = SUCCESS;
	char* tmp_buf;
	_int32 tmp_len;
	sd_memset(cmd, 0, sizeof(DATA_TRANSFER_CMD));
	tmp_buf = buffer;
	tmp_len = len;
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_version);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_cmd_type);
	sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_source_port);
	sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_target_port);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_peerid_hashcode);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_seq_num);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_ack_num);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_window_size);
	ret = sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_data_len);
	if(ret != SUCCESS || tmp_len != cmd->_data_len)
	{
		LOG_ERROR("[remote peer version = %u]udt_extract_data_transfer_cmd failed, ret = %d", cmd->_version, ret);
		return -1;
	}
	cmd->_data = tmp_buf;
	return SUCCESS;
}

_int32	udt_extract_reset_cmd(char* buffer, _u32 len, RESET_CMD* cmd)
{
	_int32	ret = SUCCESS;
	char*	tmp_buf;
	_int32	tmp_len;
	sd_memset(cmd, 0, sizeof(RESET_CMD));
	tmp_buf = buffer;
	tmp_len = len;
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_version);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_cmd_type);
	sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_source_port);
	sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_target_port);
	ret = sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_peerid_hashcode);
	if(ret != SUCCESS)
	{
		LOG_ERROR("[remote peer version = %u]udt_extract_reset_cmd failed, ret = %d", cmd->_version, ret);
		return -1;
	}
	if(tmp_len > 0)
	{
		LOG_ERROR("[remote peer version = %u]udt_extract_reset_cmd, but last %u bytes is unknown how to extract", cmd->_version, tmp_len);
	}
	return SUCCESS;
}


_int32 udt_extract_syn_cmd(char* buffer, _u32 len, SYN_CMD* cmd)
{
	_int32	ret = SUCCESS;
	char*	tmp_buf;
	_int32	tmp_len;
	sd_memset(cmd, 0, sizeof(SYN_CMD));
	tmp_buf = buffer;
	tmp_len = len;
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_version);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_cmd_type);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_flags);
	sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_source_port);
	sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_target_port);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_peerid_hashcode);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_seq_num);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_ack_num);
	ret = sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_window_size);
	if(tmp_len > 0)
		ret = sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_udt_version);
	if(ret != SUCCESS)
	{
		LOG_ERROR("[remote peer version = %u]udt_extract_syn_cmd failed, ret = %d", cmd->_version, ret);
		return -1;
	}
	return SUCCESS;
}

_int32 udt_extract_keepalive_cmd(char* buffer, _u32 len, NAT_KEEPALIVE_CMD* cmd)
{
	_int32 ret = SUCCESS;
	char* tmp_buf;
	_int32 tmp_len;
	sd_memset(cmd, 0, sizeof(NAT_KEEPALIVE_CMD));
	tmp_buf = buffer; 
	tmp_len = len;
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_version);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_cmd_type);
	sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_source_port);
	sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_target_port);
	ret = sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_peerid_hashcode);
	if(ret != SUCCESS)
	{
		LOG_ERROR("[remote peer version = %u]udt_extract_keepalive_cmd failed, ret = %d", cmd->_version, ret);
		return -1;
	}
	if(tmp_len > 0)
	{
		LOG_ERROR("[remote peer version = %u]udt_extract_keepalive_cmd, but last %u bytes is unknown how to extract", cmd->_version, tmp_len);
	}
	return SUCCESS;
}

_int32 udt_extract_advanced_ack_cmd(char* buffer, _u32 len, ADVANCED_ACK_CMD* cmd)
{
	_int32 ret = SUCCESS;
	char* tmp_buf;
	_int32 tmp_len;
	sd_memset(cmd, 0, sizeof(ADVANCED_ACK_CMD));
	tmp_buf = buffer;
	tmp_len = len;
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_version);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_cmd_type);
	sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_source_port);
	sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_target_port);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_peerid_hashcode);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_window_size);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_seq_num);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_ack_num);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_ack_seq);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_bitmap_base);
	ret = sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_bitmap_count);
	if(ret != SUCCESS)
	{
		LOG_ERROR("[remote peer version = %u]udt_extract_advanced_ack_cmd failed, ret = %d", cmd->_version, ret);
		return -1;
	}
	if(tmp_len * 8 < cmd->_bitmap_count || tmp_len * 8 > cmd->_bitmap_count + 7)
	{
		LOG_ERROR("[remote peer version = %u]udt_extract_advanced_ack_cmd failed, bitmap is invalid.", cmd->_version);
		return -1;
	}
	cmd->_bit = tmp_buf;
	return SUCCESS;
}

_int32 udt_extract_advanced_data_cmd(char* buffer, _u32 len, ADVANCED_DATA_CMD* cmd)
{
	_int32 ret = SUCCESS;
	char*	tmp_buf;
	_int32	tmp_len;
	sd_memset(cmd, 0, sizeof(ADVANCED_DATA_CMD));
	tmp_buf = buffer;
	tmp_len = len;
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_version);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_cmd_type);
	sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_source_port);
	sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_target_port);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_peerid_hashcode);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_seq_num);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_ack_num);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_window_size);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_data_len);
	ret = sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_package_seq);
	cmd->_data = tmp_buf;
	if(tmp_len != cmd->_data_len || ret != SUCCESS)
	{
		LOG_ERROR("[remote peer version = %u]udt_extract_advanced_data_cmd failed, tmp_len(%d) != cmd->_data_len(%u), ret = %d.", cmd->_version,tmp_len, cmd->_data_len, ret);
		return -1;
	}
	return SUCCESS;
}

