#include "utility/define.h"

#ifdef ENABLE_EMULE
#include "utility/bytebuffer.h"
#include "utility/peerid.h"
#include "utility/mempool.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_EMULE
#include "utility/logger.h"

#include "res_query/res_query_cmd_builder.h"
#include "emule_query_emule_info.h"
#include "../emule_interface.h"
#include "../emule_impl.h"
#include "res_query/res_query_interface.h"

static _u32 g_hub_seq = 0;

static EMULE_NOTIFY_EVENT g_emule_notify_event_callback = NULL;

_int32 emule_query_emule_info(struct tagEMULE_TASK* emule_task, _u8 file_hash[FILE_ID_SIZE], _u64 file_size)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL;
	_u32 len = 0;
	QUERY_EMULE_INFO_CMD cmd;
	cmd._cmd_type = QUERY_EMULE_INFO;
	cmd._local_peerid_len = PEER_ID_SIZE;
	get_peerid(cmd._local_peerid, PEER_ID_SIZE + 1);
	cmd._file_hash_len = FILE_ID_SIZE;
	sd_memcpy(cmd._file_id, file_hash, FILE_ID_SIZE);
	cmd._file_size = file_size;
	cmd._query_times = 1;
	cmd._partner_id_len = 0;
	cmd._product_flag = 0;
	ret = emule_build_query_emule_info_cmd(&cmd, &buffer, &len);
	CHECK_VALUE(ret);
	ret = res_query_commit_request(EMULE_HUB, g_hub_seq, &buffer, len, emule_notify_query_emule_info_result, emule_task);
	if(ret != SUCCESS)
	{
		LOG_ERROR("MMMM[emule_task = 0x%x]emule_query_emule_info, res_query_commit_request failed, errcode = %d.", emule_task, ret);
		sd_assert(FALSE);
        emule_task->_res_query_emulehub_state = RES_QUERY_FAILED;
	}
    else
    {
        emule_task->_res_query_emulehub_state = RES_QUERY_REQING;
        LOG_DEBUG("MMMM[emule_task = 0x%x]emule_query_emule_info SUCCESS.", emule_task);
    }
	return ret;
}

_int32 emule_build_query_emule_info_cmd(QUERY_EMULE_INFO_CMD* cmd, char** buffer, _u32* len)
{
	_int32 ret = SUCCESS;
	char	http_header[1024] = {0};
	_u32 http_header_len = 1024;
	char*  tmp_buf = NULL;
	_int32 tmp_len = 0;
	_u32 encode_len = 0;
    
	EMULE_HUB_HEADER header;
	header._version = EMULE_HUB_VERSION;
	header._hub_seq = g_hub_seq;
	header._cmd_len = 0;
	header._client_version = 0;
	header._compress = 0;
	*len = 62 + cmd->_partner_id_len + 6;
    
	encode_len = (*len + HUB_ENCODE_PADDING_LEN) /HUB_ENCODE_PADDING_LEN * HUB_ENCODE_PADDING_LEN + HUB_CMD_HEADER_LEN;
	ret = res_query_build_http_header(http_header, &http_header_len, encode_len,EMULE_HUB, NULL, 0);
	CHECK_VALUE(ret);
	*len += HUB_CMD_HEADER_LEN;
	ret = sd_malloc(*len + HUB_ENCODE_PADDING_LEN + http_header_len, (void**)buffer);
	CHECK_VALUE(ret);
	sd_memcpy(*buffer, http_header, http_header_len);
	tmp_buf = *buffer + http_header_len;
	tmp_len = (_int32)*len;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)header._hub_seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)header._cmd_len);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)header._client_version);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)header._compress);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_cmd_type);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_local_peerid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_local_peerid, cmd->_local_peerid_len);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_hash_len);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_file_id, (_int32)cmd->_file_hash_len);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_size);	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_query_times);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_partner_id_len);
	if(cmd->_partner_id_len > 0)
		sd_set_bytes(&tmp_buf, &tmp_len, cmd->_partner_id, (_int32)cmd->_partner_id_len);
	ret = sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_product_flag);
	if(ret != SUCCESS || tmp_len != 0)
	{
		LOG_ERROR("emule_build_query_emule_info_cmd failed, errcode = %d.", ret);
		sd_free(*buffer);
		*buffer = NULL;
		return ret;
	}
	ret = res_query_aes_encrypt(*buffer+http_header_len, len);
	if(ret != SUCCESS)
	{
		LOG_ERROR("emule_build_query_emule_info_cmd, but res_query_aes_encrypt failed, errcode = %d.", ret);
		sd_free(*buffer);
		*buffer = NULL;
	}
    *len += http_header_len;
	return ret;
}

_int32 emule_extract_query_emule_info_resp_cmd(char* buffer, _u32 len, QUERY_EMULE_INFO_RESP_CMD* cmd)
{
	_int32 ret = SUCCESS;
	EMULE_HUB_HEADER header;
	char* tmp_buf = buffer;
	_int32  tmp_len = (_int32)len;
	sd_memset(cmd, 0, sizeof(QUERY_EMULE_INFO_RESP_CMD));
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&header._version);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&header._hub_seq);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&header._cmd_len);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&header._client_version);
	sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&header._compress);
	sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_cmd_type);
	sd_assert(cmd->_cmd_type == QUERY_EMULE_INFO_RESP);
	ret = sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_result);
	if(cmd->_result == 0 || ret != SUCCESS)
		return ret;
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_has_record);
	if(cmd->_has_record == 0)
		return SUCCESS;			//ºöÂÔºóÃæ×Ö¶Î
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_aich_hash_len);
	if(cmd->_aich_hash_len != AICH_HASH_LEN)
		return -1;
	sd_get_bytes(&tmp_buf, &tmp_len, (char*)cmd->_aich_hash, cmd->_aich_hash_len);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_part_hash_len);
	cmd->_part_hash = (_u8*)tmp_buf;
	tmp_buf += cmd->_part_hash_len;
	tmp_len -= cmd->_part_hash_len;
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_cid_len);
	if(cmd->_cid_len != CID_SIZE)
		return -1;
	sd_get_bytes(&tmp_buf, &tmp_len, (char*)cmd->_cid, cmd->_cid_len);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_gcid_len);
	if(cmd->_gcid_len != CID_SIZE)
		return -1;
	sd_get_bytes(&tmp_buf, &tmp_len, (char*)cmd->_gcid, cmd->_gcid_len);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_gcid_part_size);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_gcid_level);
	ret = sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_control_flag);
	if(ret != SUCCESS)
	{
		LOG_ERROR("[version = %u]emule_extract_query_emule_info_resp_cmd failed, ret = %d.", header._version, ret);
		return -1;
	}
	if(tmp_len > 0)
	{
		LOG_ERROR("[version = %u]emule_extract_query_emule_info_resp_cmd, but last %d bytes is unkowned how to extract.", header._version, tmp_len);
	}
	return ret;
}

_int32 emule_notify_query_emule_info_result(_int32 errcode, char* buffer, _u32 len, void* user_data)
{
	_int32 ret = SUCCESS;
	QUERY_EMULE_INFO_RESP_CMD cmd;
	EMULE_TASK* task = (EMULE_TASK*)user_data;
    
    LOG_DEBUG("MMMM[emule_task = 0x%x]emule_notify_query_emule_info_result:%d.", task, errcode);

	if(task == NULL)
	{
		return SUCCESS;
	}
	if(errcode != SUCCESS)
	{
        task->_res_query_emulehub_state = RES_QUERY_FAILED;
		return ret;
	}    
	ret = emule_extract_query_emule_info_resp_cmd(buffer, len, &cmd);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("emule_notify_query_emule_info_result emule_extract_query_emule_info_resp_cmd ret,%d", ret);
        task->_res_query_emulehub_state = RES_QUERY_FAILED;
		return ret;
	}
	if(cmd._result != 0 && cmd._has_record != 0)
	{
        task->_res_query_emulehub_state = RES_QUERY_SUCCESS;
		emule_notify_get_cid_info(task, cmd._cid, cmd._gcid);
		if(NULL != g_emule_notify_event_callback)
		{
			g_emule_notify_event_callback(task->_task._extern_id, CID_IS_OK);
			LOG_DEBUG("emule_notify_query_emule_info_result: callback...taskid = %d.", task->_task._extern_id);
		}
		emule_notify_get_part_hash(task, cmd._part_hash, cmd._part_hash_len);
		emule_notify_get_aich_hash(task, cmd._aich_hash, cmd._aich_hash_len);
	}
	else
	{
        task->_res_query_emulehub_state = RES_QUERY_FAILED;
        emule_notify_query_failed(task);
		LOG_DEBUG("MMMM[emule_task = 0x%x]emule_notify_query_emule_info_result, but emule hub no record.", task);
        task->_is_need_report = TRUE;
    }	
	return ret;
}

_int32 emule_query_set_notify_event_callback( void * call_ptr )
{
    if(NULL == call_ptr)
        return INVALID_HANDLER;
    
    g_emule_notify_event_callback = (EMULE_NOTIFY_EVENT)call_ptr ;

	LOG_DEBUG("emule_query_set_notify_event_callback:0x%x",call_ptr);

    return SUCCESS;

}


_int32 emule_just_query_emule_info(void* userdata, _u8 file_hash[FILE_ID_SIZE], _u64 file_size)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL;
	_u32 len = 0;
	QUERY_EMULE_INFO_CMD cmd;
	cmd._cmd_type = QUERY_EMULE_INFO;
	cmd._local_peerid_len = PEER_ID_SIZE;
	get_peerid(cmd._local_peerid, PEER_ID_SIZE + 1);
	cmd._file_hash_len = FILE_ID_SIZE;
	sd_memcpy(cmd._file_id, file_hash, FILE_ID_SIZE);
	cmd._file_size = file_size;
	cmd._query_times = 1;
	cmd._partner_id_len = 0;
	cmd._product_flag = 0;
	ret = emule_build_query_emule_info_cmd(&cmd, &buffer, &len);
	CHECK_VALUE(ret);
	ret = res_query_commit_request(EMULE_HUB, g_hub_seq, &buffer, len, emule_notify_just_query_emule_info_result, userdata);
	CHECK_VALUE(ret);
	return SUCCESS;
}


_int32 emule_notify_just_query_emule_info_result(_int32 errcode, char* buffer, _u32 len, void* user_data)
{
	_int32 ret = SUCCESS;
	QUERY_EMULE_INFO_RESP_CMD cmd = {0};
    
    LOG_DEBUG("MMMM[user_data = 0x%x]emule_notify_just_query_emule_info_result:%d.", user_data, errcode);

	if(user_data == NULL)
	{
		return SUCCESS;
	}
	if(errcode != SUCCESS)
	{
		emule_just_query_file_info_resp(user_data,errcode ,NULL,NULL,0,0);
		return SUCCESS;
	}    
	ret = emule_extract_query_emule_info_resp_cmd(buffer, len, &cmd);
	if(ret==SUCCESS && cmd._result != 0 && cmd._has_record != 0)
	{
		emule_just_query_file_info_resp(user_data,0 ,cmd._cid, cmd._gcid,cmd._gcid_level,cmd._control_flag);
	}
	else
	{
		emule_just_query_file_info_resp(user_data,1 ,NULL,NULL,0,0);
    }	
	return SUCCESS;
}


#endif /* ENABLE_EMULE */

