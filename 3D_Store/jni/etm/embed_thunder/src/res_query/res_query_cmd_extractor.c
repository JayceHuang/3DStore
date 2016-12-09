#include "utility/bytebuffer.h"
#include "utility/mempool.h"
#include "utility/logid.h"
#define LOGID LOGID_RES_QUERY
#include "utility/logger.h"

#include "res_query_cmd_extractor.h"

_int32 extract_server_res_resp_cmd(char* buffer, _u32 len, SERVER_RES_RESP_CMD* cmd)
{
	_int32 ret = SUCCESS;
	_u32 i;
	char* tmp_buf = buffer;
	_int32  tmp_len = (_int32)len;
	sd_memset(cmd, 0, sizeof(SERVER_RES_RESP_CMD));
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._version);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._seq);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._cmd_len);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_client_version);
	sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_compress);
	sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_cmd_type);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_result);
	if (cmd->_result == 0)
	{	/*result == 0 means query failed*/
		_u32 requery_interval;
		_u32 errcode;
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&errcode);
		if (errcode == 1)
		{
			sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&requery_interval);
		}
		sd_assert(tmp_len == 0);
		return (tmp_len == 0 ? SUCCESS : -1);
	}
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&cmd->_file_size);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_cid_size);
	if (cmd->_cid_size != CID_SIZE && cmd->_cid_size != 0)
	{
		LOG_ERROR("[version = %u]extract_server_res_resp_cmd failed, cmd->_cid_size = %u.",
			cmd->_header._version, cmd->_cid_size);
		return ERR_RES_QUERY_INVALID_COMMAND;
	}
	sd_get_bytes(&tmp_buf, &tmp_len, (char*)cmd->_cid, cmd->_cid_size);
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&cmd->_origin_url_id);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_server_res_num);
	cmd->_server_res_buffer = tmp_buf;
	for (i = 0; i < cmd->_server_res_num; ++i)
	{
		_u32 res_len = 0;
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&res_len);
		cmd->_server_res_buffer_len += (res_len + 4);
		tmp_len -= res_len;
		tmp_buf += res_len;
	}
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_bonus_res_num);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_gcid_size);
	if (cmd->_gcid_size != CID_SIZE && cmd->_gcid_size != 0)
	{
		LOG_ERROR("[version = %u]extract_server_res_resp_cmd_failed, cmd->_gcid_size = %u.",
			cmd->_header._version, cmd->_gcid_size);
		return ERR_RES_QUERY_INVALID_COMMAND;
	}
	sd_get_bytes(&tmp_buf, &tmp_len, (char*)cmd->_gcid, cmd->_gcid_size);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_block_size);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_gcid_level);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_use_policy);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_bcid_size);
	if (cmd->_bcid_size > 0)
	{
		ret = sd_malloc(cmd->_bcid_size, (void**)&cmd->_bcid);
		if (ret != SUCCESS)
		{
			LOG_ERROR("extract_server_res_resp_cmd, malloc failed..");
			CHECK_VALUE(ret);
		}
		sd_get_bytes(&tmp_buf, &tmp_len, (char*)cmd->_bcid, cmd->_bcid_size);
	}
	sd_get_bytes(&tmp_buf, &tmp_len, (char*)&cmd->_tracker_ip, sizeof(_u32));
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_tracker_port);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_control_flag);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_svr_speed_threshold);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_put_file_size_threshold);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_origin_url_speed);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_origin_fetch_hint);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_origin_connect_time);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_max_connection);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_min_retry_interval);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_res_type);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_dspider_control_flag);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_orig_url_quality);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_file_suffix_size);
	if (cmd->_file_suffix_size >= 16)
	{
		LOG_ERROR("file_suffix_size = %u, is too big.", cmd->_file_suffix_size);
		sd_assert(FALSE);
		return ERR_RES_QUERY_INVALID_COMMAND;
	}
	sd_get_bytes(&tmp_buf, &tmp_len, cmd->_file_suffix, cmd->_file_suffix_size);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_user_agent);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_orig_res_level);
	ret = sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_orig_res_priority);
	if (ret != SUCCESS)
	{
		LOG_ERROR("[version = %u]extract_server_res_resp_cmd failed, ret = %d.", cmd->_header._version, ret);
		return ERR_RES_QUERY_INVALID_COMMAND;
	}
	if (tmp_len > 0)
	{
		LOG_ERROR("[version = %u]extract_server_res_resp_cmd, but last %d bytes is unknowned how to extract",
			cmd->_header._version, tmp_len);
	}
	return SUCCESS;
}

_int32 extract_relation_server_res_resp_cmd(char* buffer, _u32 len, QUERY_FILE_RELATION_RES_RESP* cmd)
{
	_int32 ret = SUCCESS;
	_u32 i;
	char* tmp_buf = buffer;
	_int32  tmp_len = (_int32)len;
	sd_memset(cmd, 0, sizeof(QUERY_FILE_RELATION_RES_RESP));
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._version);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._seq);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._cmd_len);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_client_version);
	sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_compress);
	sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_cmd_type);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_result);
	if (cmd->_result == 0)
	{	/*result == 0 means query failed*/
		_u32 requery_interval;
		_u32 errcode;
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&errcode);
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&requery_interval);


		LOG_DEBUG("extract_relation_server_res_resp_cmd cmd->_result == 0 , errcode=%u, requery_interval=%u",
			errcode, requery_interval);

		return (tmp_len == 0 ? SUCCESS : -1);
	}

	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_cid_size);
	if (cmd->_cid_size != CID_SIZE && cmd->_cid_size != 0)
	{
		LOG_ERROR("[version = %u]extract_relation_server_res_resp_cmd failed, cmd->_cid_size = %u.",
			cmd->_header._version, cmd->_cid_size);
		return ERR_RES_QUERY_INVALID_COMMAND;
	}
	sd_get_bytes(&tmp_buf, &tmp_len, (char*)cmd->_cid, cmd->_cid_size);
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&cmd->_file_size);

	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_gcid_size);
	if (cmd->_gcid_size != CID_SIZE && cmd->_gcid_size != 0)
	{
		LOG_ERROR("[version = %u]extract_relation_server_res_resp_cmd failed, cmd->_gcid_size = %u.",
			cmd->_header._version, cmd->_gcid_size);
		return ERR_RES_QUERY_INVALID_COMMAND;
	}
	sd_get_bytes(&tmp_buf, &tmp_len, (char*)cmd->_gcid, cmd->_gcid_size);

	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_relation_server_res_num);
	cmd->_relation_server_res_buffer = tmp_buf;
	for (i = 0; i < cmd->_relation_server_res_num; ++i)
	{
		_u32 res_len = 0;
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&res_len);
		cmd->_relation_server_res_buffer_len += (res_len + 4);
		tmp_len -= res_len;
		tmp_buf += res_len;
	}

	if (tmp_len > 0)
	{
		LOG_ERROR("[version = %u]extract_relation_server_res_resp_cmd, but last %d bytes is unknowned how to extract",
			cmd->_header._version, tmp_len);
	}
	return SUCCESS;
}
_int32 extract_peer_res_resp_cmd(char* buffer, _u32 len, PEER_RES_RESP_CMD* cmd)
{
	_int32 ret = SUCCESS;
	_u32 i, res_len;
	char* tmp_buf = buffer;
	_int32  tmp_len = (_int32)len;
	sd_memset(cmd, 0, sizeof(PEER_RES_RESP_CMD));
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._version);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._seq);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._cmd_len);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_cmd_type);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_result);
	if (cmd->_result != 0)
		return (tmp_len == 0 ? SUCCESS : ERR_RES_QUERY_INVALID_COMMAND);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_cid_size);
	if (cmd->_cid_size != CID_SIZE && cmd->_cid_size != 0)
	{
		LOG_ERROR("[version = %u]extract_peer_res_resp_cmd failed, cmd->_cid_size = %u",
			cmd->_header._version, cmd->_cid_size);
		return ERR_RES_QUERY_INVALID_COMMAND;
	}
	sd_get_bytes(&tmp_buf, &tmp_len, (char*)cmd->_cid, cmd->_cid_size);
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&cmd->_file_size);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_gcid_size);
	if (cmd->_gcid_size != CID_SIZE)
	{
		LOG_ERROR("[version = %u]extract_peer_res_resp_cmd failed, cmd->_gcid_size = %u",
			cmd->_header._version, cmd->_gcid_size);
		return ERR_RES_QUERY_INVALID_COMMAND;
	}
	sd_get_bytes(&tmp_buf, &tmp_len, (char*)cmd->_gcid, cmd->_gcid_size);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_bonus_res_num);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_peer_res_num);
	cmd->_peer_res_buffer = tmp_buf;
	for (i = 0; i < cmd->_peer_res_num; ++i)
	{
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&res_len);
		cmd->_peer_res_buffer_len += (res_len + 4);
		tmp_len -= res_len;
		tmp_buf += res_len;
	}
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_resource_count);
	ret = sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_query_interval);
	if (ret != SUCCESS)
	{
		LOG_ERROR("[version = %u]extract_peer_res_resp_cmd failed, ret = %d.", cmd->_header._version, ret);
		return ERR_RES_QUERY_INVALID_COMMAND;
	}
	if (tmp_len > 0)
	{
		LOG_ERROR("[version = %u]extract_peer_res_resp_cmd, but last %d bytes is unkowned how to extract.",
			cmd->_header._version, tmp_len);
	}
	return SUCCESS;
}

_int32 extract_tracker_res_resp_cmd(char* buffer, _u32 len, TRACKER_RES_RESP_CMD* cmd)
{
	_int32 ret = SUCCESS;
	char* tmp_buf = buffer;
	_int32  tmp_len = (_int32)len;
	sd_memset(cmd, 0, sizeof(TRACKER_RES_RESP_CMD));
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._version);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._seq);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._cmd_len);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_cmd_type);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_result);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_tracker_res_num);
	cmd->_tracker_res_buffer = tmp_buf;
	cmd->_tracker_res_buffer_len = 34 * cmd->_tracker_res_num;
	tmp_buf += cmd->_tracker_res_buffer_len;
	tmp_len -= cmd->_tracker_res_buffer_len;
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_reserve_size);
	sd_assert(cmd->_reserve_size == 0);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_query_stamp);
	ret = sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_query_interval);
	if (ret != SUCCESS)
	{
		LOG_ERROR("[version = %u]extract_tracker_res_resp_cmd failed, ret = %d.", cmd->_header._version, ret);
		return ERR_RES_QUERY_INVALID_COMMAND;
	}
	if (tmp_len > 0)
	{
		LOG_ERROR("[version = %u]extract_tracker_res_resp_cmd, but last %d bytes is unknowned how to extract.",
			cmd->_header._version, tmp_len);
	}
	return SUCCESS;
}

_int32 extract_query_bt_info_resp_cmd(char* buffer, _u32 len, QUERY_BT_INFO_RESP_CMD* cmd)
{
	_int32 ret = SUCCESS;
	char*	tmp_buf = buffer;
	_int32  tmp_len = (_int32)len;
	sd_memset(cmd, 0, sizeof(QUERY_BT_INFO_RESP_CMD));
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._version);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._seq);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._cmd_len);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_client_version);
	sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_compress);
	sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_cmd_type);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_result);
	if (cmd->_result == 0)
	{	/*result == 0 means query failed*/
		return SUCCESS;
	}
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_has_record);
	if (cmd->_has_record == 0)
	{	/*服务器上是否存在记录，1：存在；0：不存在，忽略后续字段*/
		return SUCCESS;
	}
	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&cmd->_file_size);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_cid_size);
	if (cmd->_cid_size != CID_SIZE)
		return ERR_RES_QUERY_INVALID_COMMAND;
	sd_get_bytes(&tmp_buf, &tmp_len, (char*)cmd->_cid, cmd->_cid_size);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_gcid_size);
	if (cmd->_gcid_size != CID_SIZE && cmd->_gcid_size != 0)
		return ERR_RES_QUERY_INVALID_COMMAND;
	sd_get_bytes(&tmp_buf, &tmp_len, (char*)cmd->_gcid, cmd->_gcid_size);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_gcid_part_size);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_gcid_level);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_control_flag);
	ret = sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_bcid_size);
	cmd->_bcid = (_u8*)tmp_buf;
	if (ret != SUCCESS || tmp_len < cmd->_bcid_size)
	{
		LOG_ERROR("[version = %u]extract_query_bt_info_resp_cmd failed, ret = %d, tmp_len = %d, cmd->_bcid_size = %u",
			cmd->_header._version, ret, tmp_len, cmd->_bcid_size);
		sd_assert(FALSE);
		return -1;
	}
	return SUCCESS;
}

_int32 extract_enrollsp1_resp_cmd(char* buffer, _u32 len, ENROLLSP1_RESP_CMD* cmd)
{
	_u32 i, size;
	_int32 ret = SUCCESS;
	char*  tmp_buf = buffer;
	_int32 tmp_len = (_int32)len;
	_u32 complete_len = 0;
	CONF_SETTING_NODE* conf_setting;
	SERVERS_NODE* servers;
	sd_memset(cmd, 0, sizeof(ENROLLSP1_RESP_CMD));
	list_init(&cmd->_client_conf_setting_list);
	list_init(&cmd->_servers_list);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._version);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._seq);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._cmd_len);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_client_version);
	sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_compress);
	sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_cmd_type);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_result);
	if (cmd->_result == 0)
	{
		return SUCCESS;		/*失败忽略其后字段*/
	}
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_latest_version_size);
	if (cmd->_latest_version_size >= 24)
	{
		sd_assert(FALSE);
		return -1;
	}
	sd_get_bytes(&tmp_buf, &tmp_len, cmd->_latest_version, cmd->_latest_version_size);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_login_hint);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_login_desc_size);
	if (cmd->_login_desc_size >= 256)
	{
		sd_assert(FALSE);
		return -1;
	}
	sd_get_bytes(&tmp_buf, &tmp_len, cmd->_login_desc, cmd->_login_desc_size);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_login_associate_size);
	if (cmd->_login_associate_size >= 256)
	{
		sd_assert(FALSE);
		return -1;
	}
	sd_get_bytes(&tmp_buf, &tmp_len, cmd->_login_associate, cmd->_login_associate_size);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_client_conf_setting_size);
	for (i = 0; i < cmd->_client_conf_setting_size; ++i)
	{
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&size);
		if (tmp_len < size)
		{
			sd_assert(FALSE);
			return -1;
		}
		complete_len = tmp_len;

		ret = sd_malloc(size, (void**)&conf_setting);
		CHECK_VALUE(ret);
		sd_memset(conf_setting, 0, size);
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&conf_setting->_section_size);
		if (conf_setting->_section_size >= 128)
		{
			sd_free(conf_setting);
			conf_setting = NULL;
			sd_assert(FALSE);
			return -1;
		}
		sd_get_bytes(&tmp_buf, &tmp_len, conf_setting->_section, conf_setting->_section_size);
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&conf_setting->_name_size);
		if (conf_setting->_name_size >= 128)
		{
			sd_free(conf_setting);
			conf_setting = NULL;
			sd_assert(FALSE);
			return -1;
		}
		sd_get_bytes(&tmp_buf, &tmp_len, conf_setting->_name, conf_setting->_name_size);
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&conf_setting->_value_size);
		if (conf_setting->_value_size >= 128)
		{
			sd_free(conf_setting);
			conf_setting = NULL;
			sd_assert(FALSE);
			return -1;
		}
		sd_get_bytes(&tmp_buf, &tmp_len, conf_setting->_value, conf_setting->_value_size);
		list_push(&cmd->_client_conf_setting_list, conf_setting);
		complete_len -= tmp_len;
		if (complete_len <= size)
		{
			tmp_len -= (size - complete_len);
			tmp_buf += (size - complete_len);
		}
		else
		{
			sd_free(conf_setting);
			conf_setting = NULL;
			sd_assert(FALSE);
			return -1;
		}
	}
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_servers_list_size);
	for (i = 0; i < cmd->_servers_list_size; ++i)
	{
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&size);

		if (tmp_len < size)
		{
			sd_assert(FALSE);
			return -1;
		}
		complete_len = tmp_len;

		ret = sd_malloc(size, (void**)&servers);
		CHECK_VALUE(ret);
		sd_memset(servers, 0, sizeof(SERVERS_NODE));
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)servers->_server_size);
		if (servers->_server_size >= MAX_URL_LEN)
		{
			sd_free(servers);
			servers = NULL;
			sd_assert(FALSE);
			return -1;
		}
		sd_get_bytes(&tmp_buf, &tmp_len, (char*)&servers->_server_ip, sizeof(_u32));
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&servers->_server_port);
		list_push(&cmd->_servers_list, servers);

		complete_len -= tmp_len;
		if (complete_len <= size)
		{
			tmp_len -= (size - complete_len);
			tmp_buf += (size - complete_len);
		}
		else
		{
			sd_free(conf_setting);
			conf_setting = NULL;
			sd_assert(FALSE);
			return -1;
		}
	}
	ret = sd_get_bytes(&tmp_buf, &tmp_len, (char*)&cmd->_external_ip, sizeof(_u32));
	if (ret != SUCCESS)
	{
		LOG_ERROR("[version = %u]extract_enrollsp1_resp_cmd failed, ret = %d.", cmd->_header._version, ret);
		return ERR_RES_QUERY_INVALID_COMMAND;
	}
	if (tmp_len > 0)
	{
		LOG_ERROR("[version = %u]extract_enrollsp1_resp_cmd, but last %d bytes is unknowned how to extract.",
			cmd->_header._version, tmp_len);
	}
	return SUCCESS;
}

/*调用该函数后，记得判断cmd里面的所以list结构，如果不为空，应该把里面的数据free掉*/
_int32 extract_query_config_resp_cmd(char* buffer, _u32 len, QUERY_CONFIG_RESP_CMD* cmd)
{
	_u32 i, size;
	_int32 ret = SUCCESS;
	char*  tmp_buf = buffer;
	_int32 tmp_len = (_int32)len;
	_u32 complete_len = 0;
	CONF_SETTING_NODE* conf_setting;
	sd_memset(cmd, 0, sizeof(QUERY_CONFIG_RESP_CMD));
	list_init(&cmd->_setting_list);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._version);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._seq);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._cmd_len);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_cmd_type);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_result);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_timestamp);
	if (cmd->_result != 0)
	{
		LOG_ERROR("[version = %u]extract_query_config_resp_cmd failed, result(%d) .",
			cmd->_header._version, cmd->_result);
		return cmd->_result;		/*失败忽略其后字段*/
	}

	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_setting_size);
	LOG_DEBUG("[version = %u]extract_query_config_resp_cmd success, timestamp:%u, param count:%d.",
		cmd->_header._version, cmd->_timestamp, cmd->_setting_size);
	for (i = 0; i < cmd->_setting_size; ++i)
	{
		ret = sd_malloc(sizeof(CONF_SETTING_NODE), (void**)&conf_setting);
		CHECK_VALUE(ret);
		sd_memset(conf_setting, 0, sizeof(CONF_SETTING_NODE));
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&conf_setting->_section_size);
		if (conf_setting->_section_size >= 128 || tmp_len < conf_setting->_section_size)
		{
			sd_free(conf_setting);
			conf_setting = NULL;
			sd_assert(FALSE);
			LOG_ERROR("[version = %u]extract_query_config_resp_cmd failed, section name is too long(%d) or remain data len is less(%d) .",
				cmd->_header._version, conf_setting->_section_size, tmp_len);
			continue;
		}
		sd_get_bytes(&tmp_buf, &tmp_len, conf_setting->_section, conf_setting->_section_size);
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&conf_setting->_name_size);
		if (conf_setting->_name_size >= 128 || tmp_len < conf_setting->_name_size)
		{
			sd_free(conf_setting);
			conf_setting = NULL;
			sd_assert(FALSE);
			LOG_ERROR("[version = %u]extract_query_config_resp_cmd failed, param name is too long(%d) or remain data len is less(%d) .",
				cmd->_header._version, conf_setting->_name_size, tmp_len);
			continue;
		}
		sd_get_bytes(&tmp_buf, &tmp_len, conf_setting->_name, conf_setting->_name_size);
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&conf_setting->_value_size);
		if (conf_setting->_value_size >= 128 || tmp_len < conf_setting->_value_size)
		{
			sd_free(conf_setting);
			conf_setting = NULL;
			sd_assert(FALSE);
			LOG_ERROR("[version = %u]extract_query_config_resp_cmd failed, param value is too long(%d) or remain data len is less(%d) .",
				cmd->_header._version, conf_setting->_value_size, tmp_len);
			continue;
		}
		sd_get_bytes(&tmp_buf, &tmp_len, conf_setting->_value, conf_setting->_value_size);
		list_push(&cmd->_setting_list, conf_setting);
		LOG_DEBUG("[version = %u]extract_query_config_resp_cmd , param section:%s, name:%s, value:%s.",
			cmd->_header._version, conf_setting->_section, conf_setting->_name, conf_setting->_value);
	}
	sd_assert(tmp_len == 0);

	return SUCCESS;
}



_int32 extract_server_res_info_resp_cmd(char* buffer, _u32 len, QUERY_RES_INFO_RESP* cmd)
{
	_int32 ret = SUCCESS;
	_u32 i;
	char* tmp_buf = buffer;
	_int32  tmp_len = (_int32)len;
	sd_memset(cmd, 0, sizeof(QUERY_RES_INFO_RESP));
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._version);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._seq);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._cmd_len);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_client_version);
	sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_compress);
	_u32 reverser_length = 0;
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&reverser_length);
	if (tmp_len < reverser_length)
	{
		CHECK_VALUE(-1);
		return -1;
	}

	tmp_len -= reverser_length;
	tmp_buf += reverser_length;

	sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_cmd_type);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_result);
	if (cmd->_result == 0)
	{	/*result == 0 means query failed*/
		_u32 requery_interval;
		_u32 errcode;
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&errcode);
		if (errcode == 1)
		{
			sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&requery_interval);
		}
		sd_assert(tmp_len == 0);
		return (tmp_len == 0 ? SUCCESS : -1);
	}

	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_cid_size);
	if (cmd->_cid_size != CID_SIZE && cmd->_cid_size != 0)
	{
		LOG_ERROR("extract_server_res_info_resp_cmd,  cmd->_cid_size != CID_SIZE %d", cmd->_cid_size);
		return -1;
	}

	if (cmd->_cid_size > 0)
	{
		sd_get_bytes(&tmp_buf, &tmp_len, (char*)cmd->_cid, (_int32)cmd->_cid_size);
	}

	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&cmd->_file_size);

	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_gcid_size);
	if (cmd->_gcid_size != CID_SIZE && cmd->_gcid_size != 0)
	{
		LOG_ERROR("extract_server_res_info_resp_cmd,  cmd->_gcid_size != CID_SIZE %d", cmd->_gcid_size);
		return -1;
	}

	if (cmd->_gcid_size > 0)
	{
		sd_get_bytes(&tmp_buf, &tmp_len, (char*)cmd->_gcid, (_int32)cmd->_gcid_size);
	}

	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_gcid_part_size);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_gcid_level);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_bcid_size);
	if (cmd->_bcid_size > 0)
	{
		ret = sd_malloc(cmd->_bcid_size, (void**)&cmd->_bcid);
		if (ret != SUCCESS)
		{
			LOG_ERROR("extract_server_res_info_resp_cmd, malloc failed..");
			CHECK_VALUE(ret);
			return ret;
		}
		sd_get_bytes(&tmp_buf, &tmp_len, (char*)cmd->_bcid, cmd->_bcid_size);
	}

	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_control_flag);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_svr_speed_threshold);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_pub_file_size_threshold);

	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_res_type);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_dspider_control_flag);

	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_file_suffix_size);
	if (cmd->_file_suffix_size > 16)
	{
		CHECK_VALUE(-1);
		return -1;
	}

	if (cmd->_file_suffix_size > 0)
	{
		sd_get_bytes(&tmp_buf, &tmp_len, (char*)cmd->_file_suffix, cmd->_file_suffix_size);
	}

	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_download_strategy);
	ret = sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_is_bcid_exist);

	if (tmp_len > 0)
	{
		LOG_ERROR("[version = %u]extract_server_res_info_resp_cmd, but last %d bytes is unknowned how to extract",
			cmd->_header._version, tmp_len);
	}

	return SUCCESS;
}


_int32 extract_newserver_res_resp_cmd(char* buffer, _u32 len, NEWQUERY_SERVER_RES_RESP* cmd)
{
	_int32 ret = SUCCESS;
	_u32 i;
	char* tmp_buf = buffer;
	_int32  tmp_len = (_int32)len;
	sd_memset(cmd, 0, sizeof(NEWQUERY_SERVER_RES_RESP));
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._version);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._seq);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._cmd_len);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_client_version);
	sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_compress);

	_u32 reverser_length = 0;
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&reverser_length);
	if (tmp_len < reverser_length)
	{
		CHECK_VALUE(-1);
		return -1;
	}

	tmp_len -= reverser_length;
	tmp_buf += reverser_length;

	sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_cmd_type);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_result);
	if (cmd->_result == 0)
	{	/*result == 0 means query failed*/
		_u32 requery_interval;
		_u32 errcode;
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&errcode);
		if (errcode == 1)
		{
			sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&requery_interval);
		}
		sd_assert(tmp_len == 0);
		return (tmp_len == 0 ? SUCCESS : -1);
	}

	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_cid_size);
	if (cmd->_cid_size != CID_SIZE && cmd->_cid_size != 0)
	{
		LOG_ERROR("[version = %u]extract_newserver_res_resp_cmd failed, cmd->_cid_size = %u.",
			cmd->_header._version, cmd->_cid_size);
		return ERR_RES_QUERY_INVALID_COMMAND;
	}

	sd_get_bytes(&tmp_buf, &tmp_len, (char*)cmd->_cid, cmd->_cid_size);

	sd_get_int64_from_lt(&tmp_buf, &tmp_len, (_int64*)&cmd->_file_size);

	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_gcid_size);
	if (cmd->_gcid_size != CID_SIZE && cmd->_gcid_size != 0)
	{
		LOG_ERROR("[version = %u]extract_newserver_res_resp_cmd failed, cmd->_cid_size = %u.",
			cmd->_header._version, cmd->_cid_size);
		return ERR_RES_QUERY_INVALID_COMMAND;
	}
	sd_get_bytes(&tmp_buf, &tmp_len, (char*)cmd->_gcid, cmd->_gcid_size);

	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_server_res_num);

	LOG_DEBUG("extract_newserver_res_resp_cmd _server_res_num:%d", cmd->_server_res_num);
	if (cmd->_server_res_num >= 10000)
	{
		LOG_DEBUG("extract_newserver_res_resp_cmd _server_res_num111:%d", cmd->_server_res_num);
		sd_assert(0);
		return -1;
	}

	cmd->_server_res_buffer = tmp_buf;
	for (i = 0; i < cmd->_server_res_num; ++i)
	{
		_u32 res_len = 0;
		sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&res_len);
		cmd->_server_res_buffer_len += (res_len + 4);
		tmp_len -= res_len;
		tmp_buf += res_len;
	}
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_bonus_res_num);

	if (tmp_len > 0)
	{
		LOG_ERROR("[version = %u]extract_newserver_res_resp_cmd, but last %d bytes is unknowned how to extract",
			cmd->_header._version, tmp_len);
	}

	return SUCCESS;
}

