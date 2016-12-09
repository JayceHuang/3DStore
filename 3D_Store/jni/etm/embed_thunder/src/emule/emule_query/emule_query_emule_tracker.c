#include "utility/define.h"

#ifdef ENABLE_EMULE
#include "utility/logid.h"
#define	 LOGID	LOGID_EMULE
#include "utility/logger.h"
#include "utility/bytebuffer.h"
#include "utility/utility.h"
#include "utility/peer_capability.h"
#include "utility/aes.h"
#include "emule_query_emule_tracker.h"
#include "../emule_impl.h"
#include "../emule_interface.h"
#include "res_query/res_query_interface.h"
#include "res_query/res_query_cmd_builder.h"
#include "download_task/download_task.h"
#include "res_query/res_query_security.h"

#define CURRENT_EMULE_VERSION 1

_int32 emule_build_query_emule_tracker_cmd(QUERY_EMULE_TRACKER_CMD *cmd, char **buffer, _u32 *len)
{
    static _u32 seq = 0;
    _int32 ret = SUCCESS;
    char *tmp_buf = NULL;
    _int32 tmp_len = 0;
    char http_header[1024] = {0};
    _u32 http_header_len = 1024;
    _u32 encode_len = 0;

    LOG_DEBUG("emule_build_query_emule_tracker_cmd.");

    cmd->_protocol_version = CURRENT_EMULE_VERSION;         // 1
    cmd->_seq = seq++;
    cmd->_cmd_len = 38 + cmd->_file_hash_len + cmd->_user_id_len + cmd->_local_peer_id_len;
    cmd->_cmd_type = QUERY_EMULE_TRACKER;
    *len = HUB_CMD_HEADER_LEN + cmd->_cmd_len;
    encode_len 
        = (cmd->_cmd_len + HUB_ENCODE_PADDING_LEN) /HUB_ENCODE_PADDING_LEN * HUB_ENCODE_PADDING_LEN + HUB_CMD_HEADER_LEN;
    ret = res_query_build_http_header(http_header, &http_header_len, encode_len, EMULE_TRACKER, NULL, 0);
    CHECK_VALUE(ret);
    ret = sd_malloc(*len + HUB_ENCODE_PADDING_LEN + http_header_len, (void **)buffer);
    if (ret != SUCCESS)
    {
        LOG_ERROR("emule_build_query_emule_tracker_cmd, malloc error.");
        CHECK_VALUE(ret);
    }
    sd_memcpy(*buffer, http_header, http_header_len);
    tmp_buf = *buffer + http_header_len;
    tmp_len = (_int32)*len;
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_protocol_version);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_seq);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cmd_len);   
    sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_cmd_type);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_last_query_stamp);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_hash_len);
    sd_set_bytes(&tmp_buf, &tmp_len, (char *)cmd->_file_hash, (_int32)cmd->_file_hash_len);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_user_id_len);
    sd_set_bytes(&tmp_buf, &tmp_len, (char *)cmd->_user_id, (_int32)cmd->_user_id_len);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_local_peer_id_len);
    sd_set_bytes(&tmp_buf, &tmp_len, (char *)cmd->_local_peer_id, cmd->_local_peer_id_len);
    sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_capability);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->_ip);
    sd_set_int16_to_lt(&tmp_buf, &tmp_len, cmd->_port);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->_nat_type);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->_product_id);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->_upnp_ip);
    sd_set_int16_to_lt(&tmp_buf, &tmp_len, cmd->_upnp_port);

#ifdef _LOGGER 
    {
        char *log_file_hash = 0; 
        char *log_user_id = 0;
        char *log_peer_id = 0;
        _int32 ret = 0;
        ret = sd_malloc(2*cmd->_file_hash_len+1, (void **)&log_file_hash);
        CHECK_VALUE(ret);
        sd_memset((void *)log_file_hash, 0, 2*cmd->_file_hash_len+1);
        str2hex((const char*)cmd->_file_hash, cmd->_file_hash_len, log_file_hash, 2*cmd->_file_hash_len);

        ret = sd_malloc(2*cmd->_user_id_len+1, (void **)&log_user_id);
        CHECK_VALUE(ret);
        sd_memset((void *)log_user_id, 0, 2*cmd->_user_id_len+1);
        str2hex((const char*)cmd->_user_id, cmd->_user_id_len, log_user_id, 2*cmd->_user_id_len);

        ret = sd_malloc(cmd->_local_peer_id_len+1, (void **)&log_peer_id);
        CHECK_VALUE(ret);
        sd_memset((void *)log_peer_id, 0, cmd->_local_peer_id_len+1);
        sd_memcpy((void *)log_peer_id, (void *)cmd->_local_peer_id, cmd->_local_peer_id_len);

        LOG_DEBUG("QUERY_EMULE_TRACKER_CMD description :\n _protocol_version = %u, _seq = %u, _cmd_len = %u,"
            "cmd_type = %hhu, _last_query_stamp = %u, _file_hash = %s, _user_id = %s, _local_peer_id = %s, "
            "_capability = %hhu, _ip = %u, _port = %hu, _nat_typ = %u, _product_id = %u, _upnp_ip = %u,"
            " _upnp_port = %hu.", 
            cmd->_protocol_version, cmd->_seq, cmd->_cmd_len, cmd->_cmd_type, cmd->_last_query_stamp, 
            log_file_hash, log_user_id, log_peer_id, cmd->_capability, cmd->_ip, 
            cmd->_port, cmd->_nat_type, cmd->_product_id, cmd->_upnp_ip, cmd->_upnp_port);

        sd_free(log_file_hash);
        log_file_hash = 0;
        sd_free(log_user_id);
        log_peer_id = 0;
        sd_free(log_peer_id);
        log_peer_id = 0;
    }
#endif     

    ret = xl_aes_encrypt(*buffer + http_header_len, len);
    if(ret != SUCCESS)
    {
        LOG_ERROR("emule_build_query_emule_tracker_cmd, but aes_encrypt failed, errcode = %d.", ret);
        sd_free(*buffer);
        *buffer = NULL;
        sd_assert(FALSE);
        return ret;
    }
    sd_assert(tmp_len == 0);
    *len += http_header_len;
    return SUCCESS;
}

_int32 emule_extract_query_emule_tracker_resp_cmd(char* buffer, _u32 len, QUERY_EMULE_TRACKER_RESP_CMD* cmd)
{
    _int32 ret = SUCCESS;
    char* tmp_buf = buffer;
    _int32  tmp_len = (_int32)len;

    LOG_DEBUG("emule_extract_query_emule_tracker_resp_cmd.");

    sd_memset(cmd, 0, sizeof(QUERY_EMULE_TRACKER_RESP_CMD));
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._version);
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._seq);
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_header._cmd_len);
    sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_cmd_type);
    sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&cmd->_result);
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_query_stamp);
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_query_interval);
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_file_hash_len);
    if (cmd->_file_hash_len != FILE_ID_SIZE && cmd->_file_hash_len != 0)
    {
        LOG_ERROR("[version = %u]emule_extract_query_emule_tracker_resp_cmd failed, cmd->_file_hash_len = %u", 
            cmd->_header._version, cmd->_file_hash_len);
        return ERR_RES_QUERY_INVALID_COMMAND;
    }
    sd_get_bytes(&tmp_buf, &tmp_len, (char *)cmd->_file_hash, cmd->_file_hash_len);
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_super_node_ip);
    sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&cmd->_super_node_port);
    sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd->_peer_res_num);
    cmd->_peer_res_buffer = tmp_buf;
    cmd->_peer_res_buffer_len = 47 * cmd->_peer_res_num;
    tmp_buf += cmd->_peer_res_buffer_len;
    tmp_len -= cmd->_peer_res_buffer_len;
    if(tmp_len > 0)
    {
        LOG_ERROR("[version = %u]emule_extract_query_emule_tracker_resp_cmd, but last %d bytes is unknowned how to extract.", 
            cmd->_header._version, tmp_len);
    }

#ifdef _LOGGER
    {
        char *log_file_hash = 0;
        _int32 log_ret = 0;
        log_ret = sd_malloc(cmd->_file_hash_len+1, (void **)&log_file_hash);
        CHECK_VALUE(log_ret);
        sd_memset((void *)log_file_hash, 0, cmd->_file_hash_len+1);
        str2hex((const char*)cmd->_file_hash, cmd->_file_hash_len, log_file_hash, cmd->_file_hash_len);

        LOG_DEBUG("QUERY_EMULE_TRACKER_RESP_CMD description: \n"
            "_cmd_type = %hhu, _result = %hhu, _query_stamp = %u, _query_interval = %u, _file_hash = %s "
            "_sn_ip = %u, _sn_port = %hu, _peer_res_num = %u.", cmd->_cmd_type, cmd->_result, cmd->_query_stamp, 
            cmd->_query_interval, log_file_hash, cmd->_super_node_ip, cmd->_super_node_port, cmd->_peer_res_num);

        sd_free(log_file_hash);
        log_file_hash = 0;
    }
#endif   

    return SUCCESS;
}

_int32 emule_notify_query_emule_tracker_callback(void* user_data,_int32 errcode, _u8 result, 
                                                 _u32 retry_interval,_u32 query_stamp)
{
    _int32 return_res = SUCCESS;
    RES_QUERY_PARA *p_para = (RES_QUERY_PARA *)user_data;
    struct tagEMULE_TASK* task = (struct tagEMULE_TASK*)p_para->_task_ptr;

    sd_assert(task != NULL);
    LOG_DEBUG("emule_notify_query_emule_tracker_callback, errcode = %d, result = %u.", errcode, (_u32)result);
	
    if(errcode == SUCCESS)
    {
        task->_res_query_emule_tracker_state = RES_QUERY_SUCCESS;
        task->_last_query_emule_tracker_stamp = query_stamp;
    }
    else
    {
        task->_res_query_emule_tracker_state = RES_QUERY_FAILED;
    }

    if (task && (task->_requery_emule_tracker_timer == 0))
    {
        return_res = start_timer(emule_handle_query_emule_tracker_timeout, NOTICE_INFINITE, 
            REQ_RESOURCE_DEFAULT_TIMEOUT, 0, (void *)task, &task->_requery_emule_tracker_timer);
        CHECK_VALUE(return_res);
    }

    LOG_DEBUG("emule_notify_query_emule_tracker_callback, query_state(%d)", 
        task->_res_query_emule_tracker_state);
    return return_res;
}

_int32 emule_handle_query_emule_tracker_timeout(const MSG_INFO *msg_info, _int32 errcode, 
                                                _u32 notice_count_left, _u32 expired,_u32 msgid)
{
    _int32 return_res = SUCCESS;
    TASK *task = (TASK *)(msg_info->_user_data);
    EMULE_TASK *emule_task = (EMULE_TASK *)task;

    sd_assert(emule_task != NULL);
    LOG_DEBUG("emule_handle_query_emule_tracker_timeout:errcode(%d), notice_count_left(%u), expired(%u), msgid(%u)"
        ,errcode, notice_count_left, expired,msgid);

    if(errcode == MSG_TIMEOUT)
    {
        if (!emule_task) CHECK_VALUE(DT_ERR_INVALID_DOWNLOAD_TASK);

        LOG_DEBUG("emule_handle_query_emule_tracker_timeout:_task_id=%u, _query_peer_timer_id=%u, "
            "task_status=%d, need_more_res=%d",
            task->_task_id, emule_task->_requery_emule_tracker_timer, task->task_status,
            cm_is_global_need_more_res());

        if(msgid == emule_task->_requery_emule_tracker_timer)
        {
            if((task->task_status == TASK_S_RUNNING) && cm_is_global_need_more_res() &&
               (cm_is_need_more_peer_res(&(task->_connect_manager), INVALID_FILE_INDEX)))
            {
                LOG_DEBUG("emule_handle_query_emule_tracker_timeout, requery_emule_tracker.");
                emule_task_query_emule_tracker(emule_task);            
            }
        }
    }

    return SUCCESS;
}

_u32 build_emule_tracker_peer_capability()
{
    _u32 capability = 0;
    if (is_nated(get_peer_capability()) == TRUE)
    {
        capability |= 0x01;
    }
    // 加密不支持
    return capability;
}

_u32 emule_tracker_peer_capability_2_local_peer_capability(_u32 et_peer_capability)
{
    _u32 peer_capability = 0;
    if ((et_peer_capability & 0x01) != 0)
        peer_capability |= 0x01;
    if ((et_peer_capability & 0x02) != 0)
        peer_capability |= 0x04;
    LOG_DEBUG("emule_tracker_peer_capability_2_local_peer_capability, et_peer_capability = %hhu "
        "peer_capability = %hhu", et_peer_capability, peer_capability);
    return peer_capability;
}

/////////////////////////////// RSA ////////////////////////////

#ifdef _RSA_RES_QUERY
_int32 emule_build_query_emule_tracker_cmd_rsa(QUERY_EMULE_TRACKER_CMD *cmd, char **buffer, _u32 *len, 
	_u8* aes_key, _int32 pubkey_version)
{
	static _u32 seq = 0;
	_int32 ret = SUCCESS;
	char *tmp_buf = NULL;
	_int32 tmp_len = 0;
	char http_header[1024] = {0};
	_u32 http_header_len = 1024;
	_u32 encode_len = 0;

	LOG_DEBUG("emule_build_query_emule_tracker_cmd_rsa...");

	cmd->_protocol_version = CURRENT_EMULE_VERSION;         // 1
	cmd->_seq = seq++;
	cmd->_cmd_len = 38 + cmd->_file_hash_len + cmd->_user_id_len + cmd->_local_peer_id_len;
	cmd->_cmd_type = QUERY_EMULE_TRACKER;
	*len = HUB_CMD_HEADER_LEN + cmd->_cmd_len;
	encode_len = (cmd->_cmd_len & 0xfffffff0) + HUB_ENCODE_PADDING_LEN + HUB_CMD_HEADER_LEN;
	encode_len = (encode_len & 0xfffffff0) + HUB_ENCODE_PADDING_LEN + RSA_ENCRYPT_HEADER_LEN;
	ret = res_query_build_http_header(http_header, &http_header_len, encode_len, EMULE_TRACKER, NULL, 0);
	CHECK_VALUE(ret);
	ret = sd_malloc(http_header_len + encode_len, (void **)buffer);
	if (ret != SUCCESS)
	{
		LOG_ERROR("malloc error.");
		CHECK_VALUE(ret);
	}
	sd_memcpy(*buffer, http_header, http_header_len);
	tmp_buf = *buffer + http_header_len + RSA_ENCRYPT_HEADER_LEN;
	tmp_len = (_int32)*len;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_protocol_version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cmd_len);   
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_cmd_type);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_last_query_stamp);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_hash_len);
	sd_set_bytes(&tmp_buf, &tmp_len, (char *)cmd->_file_hash, (_int32)cmd->_file_hash_len);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_user_id_len);
	sd_set_bytes(&tmp_buf, &tmp_len, (char *)cmd->_user_id, (_int32)cmd->_user_id_len);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_local_peer_id_len);
	sd_set_bytes(&tmp_buf, &tmp_len, (char *)cmd->_local_peer_id, cmd->_local_peer_id_len);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_capability);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->_ip);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, cmd->_port);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->_nat_type);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->_product_id);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->_upnp_ip);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, cmd->_upnp_port);

#ifdef _DEBUG 
	{
		char *log_file_hash = 0; 
		char *log_user_id = 0;
		char *log_peer_id = 0;
		_int32 ret = 0;
		ret = sd_malloc(2*cmd->_file_hash_len+1, (void **)&log_file_hash);
		CHECK_VALUE(ret);
		sd_memset((void *)log_file_hash, 0, 2*cmd->_file_hash_len+1);
		str2hex((const char*)cmd->_file_hash, cmd->_file_hash_len, log_file_hash, 2*cmd->_file_hash_len);

		ret = sd_malloc(2*cmd->_user_id_len+1, (void **)&log_user_id);
		CHECK_VALUE(ret);
		sd_memset((void *)log_user_id, 0, 2*cmd->_user_id_len+1);
		str2hex((const char*)cmd->_user_id, cmd->_user_id_len, log_user_id, 2*cmd->_user_id_len);

		ret = sd_malloc(cmd->_local_peer_id_len+1, (void **)&log_peer_id);
		CHECK_VALUE(ret);
		sd_memset((void *)log_peer_id, 0, cmd->_local_peer_id_len+1);
		sd_memcpy((void *)log_peer_id, (void *)cmd->_local_peer_id, cmd->_local_peer_id_len);

		LOG_DEBUG("QUERY_EMULE_TRACKER_CMD description :\n _protocol_version = %u, _seq = %u, _cmd_len = %u,"
			"cmd_type = %hhu, _last_query_stamp = %u, _file_hash = %s, _user_id = %s, _local_peer_id = %s, "
			"_capability = %hhu, _ip = %u, _port = %hu, _nat_typ = %u, _product_id = %u, _upnp_ip = %u,"
			" _upnp_port = %hu.", 
			cmd->_protocol_version, cmd->_seq, cmd->_cmd_len, cmd->_cmd_type, cmd->_last_query_stamp, 
			log_file_hash, log_user_id, log_peer_id, cmd->_capability, cmd->_ip, 
			cmd->_port, cmd->_nat_type, cmd->_product_id, cmd->_upnp_ip, cmd->_upnp_port);

		sd_free(log_file_hash);
		log_file_hash = 0;
		sd_free(log_user_id);
		log_peer_id = 0;
		sd_free(log_peer_id);
		log_peer_id = 0;
	}
#endif     

	ret = xl_aes_encrypt(*buffer + http_header_len, len);
	if(ret != SUCCESS)
	{
		LOG_ERROR("aes_encrypt failed, errcode = %d.", ret);
		sd_free(*buffer);
		*buffer = NULL;
		sd_assert(FALSE);
		return ret;
	}
	ret = aes_encrypt_with_known_key(*buffer + http_header_len + RSA_ENCRYPT_HEADER_LEN, len, aes_key);
	if(ret != SUCCESS)
	{
		LOG_ERROR("aes_encrypt failed. errcode = %d.", ret);
		sd_free(*buffer);
		sd_assert(FALSE);
		return ret;
	}
	sd_assert(tmp_len == 0);
	tmp_buf = *buffer + http_header_len;
	tmp_len = RSA_ENCRYPT_HEADER_LEN;
	ret = build_rsa_encrypt_header(&tmp_buf, &tmp_len, pubkey_version, aes_key, *len);
	if(ret != SUCCESS)
	{
		sd_free(*buffer);
		return ret;
	}
	sd_assert(tmp_len == 0);
	*len += (http_header_len + RSA_ENCRYPT_HEADER_LEN);
	LOG_DEBUG("tmp_len=%u", tmp_len);
	return SUCCESS;
}
#endif

#endif
