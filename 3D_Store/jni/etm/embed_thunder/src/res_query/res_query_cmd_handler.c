#include "utility/mempool.h"
#include "utility/bytebuffer.h"
#include "utility/settings.h"
#include "utility/md5.h"
#include "utility/aes.h"
#include "utility/utility.h"
#include "utility/logid.h"
#define LOGID LOGID_RES_QUERY
#include "utility/logger.h"
#include "utility/peer_capability.h"
#include "platform/sd_network.h"

#include "res_query_cmd_handler.h"
#include "res_query_interface_imp.h"
#include "res_query_cmd_define.h"
#include "res_query_cmd_extractor.h"
#include "res_query_dphub.h"
#include "emule/emule_query/emule_query_emule_tracker.h"
#include "emule/emule_interface.h"
#include "emule/emule_utility/emule_utility.h"
#include "emule/emule_impl.h"


#include "high_speed_channel/hsc_perm_query.h"

static res_query_add_server_res_handler g_add_server_res = NULL;
static res_query_add_peer_res_handler g_add_peer_res = NULL;
static res_query_add_relation_fileinfo     g_add_relation_fileinfo  = NULL;

_int32 set_add_resource_func(res_query_add_server_res_handler add_server_res, res_query_add_peer_res_handler add_peer_res, res_query_add_relation_fileinfo add_relation_fileinfo)
{
	g_add_server_res = add_server_res;
	g_add_peer_res = add_peer_res;
	g_add_relation_fileinfo = add_relation_fileinfo;
	return SUCCESS;
}

_int32 xl_aes_decrypt(char* pDataBuff, _u32* nBuffLen)
{
	_int32 ret;
	int nBeginOffset;
	char *pOutBuff;
	int  nOutLen;
	unsigned char szKey[16];
	ctx_md5 md5;
	ctx_aes aes;
	int nInOffset;
	int nOutOffset;
	unsigned char inBuff[ENCRYPT_BLOCK_SIZE],ouBuff[ENCRYPT_BLOCK_SIZE];
	char * out_ptr;
	if (pDataBuff == NULL)
	{
		return FALSE;
	}
	nBeginOffset = sizeof(_u32)*3;
	if ((*nBuffLen-nBeginOffset)%ENCRYPT_BLOCK_SIZE != 0)
	{
		return FALSE;
	}
	ret = sd_malloc(*nBuffLen + 16, (void**)&pOutBuff);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("aes_decrypt, malloc failed.");
		CHECK_VALUE(ret);
	}
	nOutLen = 0;
	md5_initialize(&md5);
	md5_update(&md5, (unsigned char*)pDataBuff,sizeof(_u32)*2);
	md5_finish(&md5, szKey);
	aes_init(&aes,16,(unsigned char*)szKey);
	nInOffset = nBeginOffset;
	nOutOffset = 0;
	sd_memset(inBuff,0,ENCRYPT_BLOCK_SIZE);
	sd_memset(ouBuff,0,ENCRYPT_BLOCK_SIZE);
	while(*nBuffLen - nInOffset > 0)
	{
		sd_memcpy(inBuff,pDataBuff+nInOffset,ENCRYPT_BLOCK_SIZE);
		aes_invcipher(&aes, inBuff,ouBuff);
		sd_memcpy(pOutBuff+nOutOffset,ouBuff,ENCRYPT_BLOCK_SIZE);
		nInOffset += ENCRYPT_BLOCK_SIZE;
		nOutOffset += ENCRYPT_BLOCK_SIZE;
	}
	nOutLen = nOutOffset;
	sd_memcpy(pDataBuff + nBeginOffset,pOutBuff,nOutLen);
	out_ptr = pOutBuff + nOutLen - 1;
	if (*out_ptr <= 0 || *out_ptr > ENCRYPT_BLOCK_SIZE)
	{
		ret = -1;
	}
	else
	{
		if(nBeginOffset + nOutLen - *out_ptr < *nBuffLen)
		{
			*nBuffLen = nBeginOffset + nOutLen - *out_ptr;
			ret = SUCCESS;
		}
		else
		{
			ret = -1;
		}
	}
	sd_free(pOutBuff);
	pOutBuff = NULL;
	return ret;
}

_int32 handle_recv_resp_cmd(char* buffer, _u32 len, HUB_DEVICE* device)
{
	_u16 cmd_type = 0;
	
	_int32 tmp_len = len;
	char* cmd_ptr =NULL;
	_int32 ret;
	ret = xl_aes_decrypt(buffer, &len);
	CHECK_VALUE(ret);
    LOG_DEBUG("handle_recv_resp_cmd, device(%d).", device->_hub_type);
	if(device->_hub_type == EMULE_HUB)
	{
		return ((res_query_notify_recv_data)device->_last_cmd->_callback)(SUCCESS, buffer, len, device->_last_cmd->_user_data);
	}

	
	tmp_len = len;

	

	if(device->_hub_type == SHUB)
	{
		_u32 shub_ver = 0;
		cmd_ptr = buffer;
		sd_get_int32_from_lt((char**)&(cmd_ptr), &tmp_len,  (_int32*)&shub_ver);	

		if(shub_ver >= 60)
		{
			dump_buffer(buffer, len);
			tmp_len = len;
			cmd_ptr = buffer + HUB_CMD_HEADER_LEN + 6;
			tmp_len -= (HUB_CMD_HEADER_LEN + 6);
			_int32 reserver_length = 0;
			sd_get_int32_from_lt((char**)&(cmd_ptr), &tmp_len,  &reserver_length);
			tmp_len -= reserver_length;
			cmd_ptr += reserver_length;
			sd_get_int16_from_lt((char**)&(cmd_ptr), &tmp_len, (_int16*)&cmd_type);	
		}
		else
		{
			tmp_len = len;
			cmd_ptr = buffer + HUB_CMD_HEADER_LEN + 6;
			tmp_len -= (HUB_CMD_HEADER_LEN + 6);
       		sd_get_int16_from_lt((char**)&(cmd_ptr), &tmp_len, (_int16*)&cmd_type);		
       	}
	}
	else if( device->_hub_type == BT_HUB)
	{
		cmd_ptr = buffer + HUB_CMD_HEADER_LEN + 6;
		tmp_len -= (HUB_CMD_HEADER_LEN + 6);
       	 sd_get_int16_from_lt((char**)&(cmd_ptr), &tmp_len, (_int16*)&cmd_type);		
	}
    else if ((device->_hub_type == DPHUB_ROOT) || (device->_hub_type == DPHUB_NODE))
    {
        cmd_ptr = buffer + HUB_CMD_HEADER_LEN;
        tmp_len -= HUB_CMD_HEADER_LEN;
        sd_get_int16_from_lt((char**)&(cmd_ptr), &tmp_len, (_int16*)&cmd_type);		
    }
	else
	{	
		cmd_type = *(_u8*)(buffer + HUB_CMD_HEADER_LEN);
	}
	LOG_DEBUG("res query recv a response command, command type is %d", (_int32)cmd_type);
	switch(cmd_type)
	{
	case SERVER_RES_RESP:
		return handle_server_res_resp(buffer, len, device->_last_cmd);
	case QUERY_RES_INFO_RESP_ID:
		return handle_server_res_info(buffer, len, device->_last_cmd);
	case QUERY_NEW_RES_RESP_ID:
		return handle_newserver_res_resp(buffer, len, device->_last_cmd);
	case QUERY_FILE_RELATION_RESP:
		return handle_query_relation_server_res_resp(buffer, len, device->_last_cmd);
	case PEER_RES_RESP:
		{
			if( device->_hub_type == PARTNER_CDN )
			{
				return handle_peer_res_resp(buffer, len, device->_last_cmd, P2P_FROM_PARTNER_CDN);
			}
			else if(device->_hub_type == VIP_HUB)
			{
				return handle_peer_res_resp(buffer, len, device->_last_cmd, P2P_FROM_VIP_HUB);
			}
			else if (device->_hub_type == NORMAL_CDN_MANAGER)
			{
				return handle_peer_res_resp(buffer, len, device->_last_cmd, P2P_FROM_NORMAL_CDN);
			}
			else
			{
				return handle_peer_res_resp(buffer, len, device->_last_cmd, P2P_FROM_HUB);
			}
		}
	case TRACKER_RES_RESP:
		return handle_tracker_res_resp(buffer, len, device->_last_cmd);
#ifdef ENABLE_BT
	case QUERY_BT_INFO_RESP:
		return handle_bt_info_resp(buffer, len, device->_last_cmd);
#endif
	case ENROLLSP1_RESP:
		return handle_enrollsp1_resp(buffer, len, device->_last_cmd);
	case DPHUB_OWNER_QUERY_RESP_ID:
		return handle_query_owner_node_resp(buffer, len, device->_last_cmd);
	case DPHUB_RC_QUERY_RESP_ID:
	    return handle_rc_query_resp(g_add_peer_res, buffer, len, device->_last_cmd);
	case DPHUB_RC_NODE_QUERY_RESP_ID:
	    return handle_rc_node_query_resp(buffer, len, device->_last_cmd);
	case QUERY_CONFIG_RESP:
		return handle_query_config_resp(buffer, len, device->_last_cmd);
#ifdef ENABLE_EMULE
    case QUERY_EMULE_TRACKER_RESP:
        return handle_emule_tracker_res_resp(buffer, len, device->_last_cmd);		
#endif
	}
	LOG_DEBUG("handle_recv_resp_cmd, but cmd_type:%d is invalid", cmd_type);
	sd_assert(FALSE);
	return -1;
}

_int32 handle_server_res_resp(char* buffer, _u32 len, RES_QUERY_CMD* last_cmd)
{
	_int32 ret;
	_u32 i;
	_u32 res_len, comp_len;
	char* tmp_buffer = NULL;
	_int32  tmp_len = 0;
	
	SERVER_RES_RESP_CMD resp_cmd;
	SERVER_RES server_res;
#ifdef _DEBUG
      char cid[41], gcid[41];
#endif
	
	LOG_DEBUG("handle_server_res_resp...");
	if(last_cmd->_user_data == NULL)		
	{
		return SUCCESS;		/*command had been cancel*/
	}
	sd_memset(&resp_cmd, 0, sizeof(SERVER_RES_RESP_CMD));
	if(extract_server_res_resp_cmd(buffer, len, &resp_cmd) != SUCCESS)
	{
		((res_query_notify_shub_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0, NULL, NULL, 0, NULL, 0, 0, 0, NULL, 0);
	}
	else if(last_cmd->_cmd_seq == resp_cmd._header._seq)
	{
		tmp_buffer = resp_cmd._server_res_buffer;
		tmp_len = (_int32)resp_cmd._server_res_buffer_len;
		for(i = 0; i < resp_cmd._server_res_num; ++i)
		{
			sd_memset(&server_res, 0, sizeof(SERVER_RES));
			sd_get_int32_from_lt(&tmp_buffer, &tmp_len, (_int32*)&res_len);
			comp_len = tmp_len;
			sd_get_int32_from_lt(&tmp_buffer, &tmp_len, (_int32*)&server_res._url_size);
			if(server_res._url_size >= MAX_URL_LEN)
			{
				LOG_ERROR("extract_server_res_resp_cmd, url_size is too long...");
				sd_assert(FALSE);
				SAFE_DELETE(resp_cmd._bcid);
				return ((res_query_notify_shub_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0, NULL, NULL, 0, NULL, 0, 0, 0, NULL, 0);
			}
			sd_get_bytes(&tmp_buffer, &tmp_len, server_res._url, server_res._url_size);
			sd_get_int32_from_lt(&tmp_buffer, &tmp_len, (_int32*)&server_res._refer_url_size);
			if(server_res._refer_url_size >= MAX_URL_LEN)
			{
				LOG_ERROR("extract_server_res_resp_cmd, refer_url_size is too long...");
				sd_assert(FALSE);
				SAFE_DELETE(resp_cmd._bcid);
				return ((res_query_notify_shub_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0, NULL, NULL, 0, NULL, 0, 0, 0, NULL, 0);
			}
			sd_get_bytes(&tmp_buffer, &tmp_len, server_res._refer_url, server_res._refer_url_size);
			sd_get_int32_from_lt(&tmp_buffer, &tmp_len, (_int32*)&server_res._url_speed);
			sd_get_int8(&tmp_buffer, &tmp_len, (_int8*)&server_res._fetch_hint);
			sd_get_int32_from_lt(&tmp_buffer, &tmp_len, (_int32*)&server_res._connect_time);
			sd_get_int8(&tmp_buffer, &tmp_len, (_int8*)&server_res._gcid_type);
			sd_get_int8(&tmp_buffer, &tmp_len, (_int8*)&server_res._max_connection);
			sd_get_int8(&tmp_buffer, &tmp_len, (_int8*)&server_res._min_retry_interval);
			sd_get_int32_from_lt(&tmp_buffer, &tmp_len, (_int32*)&server_res._control_flag);
			sd_get_int8(&tmp_buffer, &tmp_len, (_int8*)&server_res._url_quality);
			sd_get_int32_from_lt(&tmp_buffer, &tmp_len, (_int32*)&server_res._user_agent);
			sd_get_int8(&tmp_buffer, &tmp_len, (_int8*)&server_res._res_level);
			ret = sd_get_int8(&tmp_buffer, &tmp_len, (_int8*)&server_res._res_priority);
			if(ret != SUCCESS)
			{
				LOG_ERROR("[version = %u]extract_server_res_resp_cmd failed, ret = %d.", resp_cmd._header._version, ret);
				sd_assert(FALSE);
				SAFE_DELETE(resp_cmd._bcid);
				return ((res_query_notify_shub_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0, NULL, NULL, 0, NULL, 0, 0, 0, NULL, 0);
			}
			comp_len = comp_len - tmp_len;
			/*
			sd_assert(comp_len == res_len);
			if(comp_len != res_len)
			{
				LOG_ERROR("extract server res failed, comp_len = %u, res_len = %u, not equal.", comp_len, res_len);
				sd_assert(FALSE);
				SAFE_DELETE(resp_cmd._bcid);
				return ((res_query_notify_shub_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0, NULL, NULL, 0, NULL, 0, 0, 0, NULL, 0);
			}
			*/
			if(comp_len > res_len)
			{
				LOG_ERROR("extract server res failed, comp_len = %u, res_len = %u, not equal.", comp_len, res_len);
				sd_assert(FALSE);
				SAFE_DELETE(resp_cmd._bcid);
				return ((res_query_notify_shub_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0, NULL, NULL, 0, NULL, 0, 0, 0, NULL, 0);
			}
			else
			{
				tmp_buffer += (res_len - comp_len);
				tmp_len -= (res_len - comp_len);
				LOG_ERROR("extract server res new version, comp_len = %u, res_len = %u.", comp_len, res_len);
			}
			LOG_ERROR("add server res, url = %s,_res_priority=%d", server_res._url,server_res._res_priority);
			if(sd_strlen(server_res._url) == server_res._url_size && (!last_cmd->_not_add_res))
			{	//过滤掉非法的url
				g_add_server_res(last_cmd->_user_data, server_res._url, server_res._url_size, server_res._refer_url, server_res._refer_url_size, FALSE,server_res._res_priority, FALSE, 0);
			}
			else
			{
				LOG_DEBUG("handle_server_res_resp, but url is invalid, url = %s.", server_res._url);
			}
		}

#ifdef _DEBUG
		str2hex((char*)resp_cmd._cid, CID_SIZE, cid, 40);
		cid[40] = '\0';
		str2hex((char*)resp_cmd._gcid, CID_SIZE, gcid, 40);
		gcid[40] = '\0';
		LOG_ERROR("notify_res_query_shub_success_event, cid = %s, gcid = %s, res_num = %u", cid, gcid, resp_cmd._server_res_num);
#ifdef IPAD_KANKAN
		printf("\nnotify_res_query_shub_success_event, cid = %s, gcid = %s, res_num = %u\n", cid, gcid, resp_cmd._server_res_num);
#endif
#endif

		((res_query_notify_shub_handler)last_cmd->_callback)(last_cmd->_user_data, SUCCESS, !resp_cmd._result, resp_cmd._file_size, resp_cmd._cid, resp_cmd._gcid, resp_cmd._gcid_level,
											resp_cmd._bcid, resp_cmd._bcid_size, resp_cmd._block_size, resp_cmd._min_retry_interval,resp_cmd._file_suffix,resp_cmd._control_flag);
	}
	else
	{
		sd_assert(FALSE);		/*last_cmd->_cmd_seq != resp_cmd._header._seq command not correspond*/
		((res_query_notify_shub_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0, NULL, NULL, 0, NULL, 0, 0, 0, NULL, 0);
	}
	SAFE_DELETE(resp_cmd._bcid);
	return SUCCESS;
}


_int32 handle_server_res_info(char* buffer, _u32 len, RES_QUERY_CMD* last_cmd)
{
	_int32 ret;
	QUERY_RES_INFO_RESP resp_cmd;
	
#ifdef _DEBUG
      char cid[41], gcid[41];
#endif
	
	LOG_DEBUG("handle_server_res_info...");
	if(last_cmd->_user_data == NULL)		
	{
		return SUCCESS;		/*command had been cancel*/
	}
	sd_memset(&resp_cmd, 0, sizeof(QUERY_RES_INFO_RESP));
	if(extract_server_res_info_resp_cmd(buffer, len, &resp_cmd) != SUCCESS)
	{
		((res_query_notify_shub_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0, NULL, NULL, 0, NULL, 0, 0, 0, NULL, 0);
	}
	else if(last_cmd->_cmd_seq == resp_cmd._header._seq)
	{

#ifdef _DEBUG
		str2hex((char*)resp_cmd._cid, CID_SIZE, cid, 40);
		cid[40] = '\0';
		str2hex((char*)resp_cmd._gcid, CID_SIZE, gcid, 40);
		gcid[40] = '\0';
		LOG_ERROR("handle_server_res_info, cid = %s, gcid = %s", cid, gcid);
#ifdef IPAD_KANKAN
		printf("\handle_server_res_info, cid = %s, gcid = %s\n", cid, gcid);
#endif
#endif

		((res_query_notify_shub_handler)last_cmd->_callback)(last_cmd->_user_data, SUCCESS, !resp_cmd._result, resp_cmd._file_size, resp_cmd._cid, resp_cmd._gcid, resp_cmd._gcid_level,
											resp_cmd._bcid, resp_cmd._bcid_size, resp_cmd._gcid_part_size,0,resp_cmd._file_suffix,resp_cmd._control_flag);
	}
	else
	{
		sd_assert(FALSE);		/*last_cmd->_cmd_seq != resp_cmd._header._seq command not correspond*/
		((res_query_notify_shub_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0, NULL, NULL, 0, NULL, 0, 0, 0, NULL, 0);
	}
	SAFE_DELETE(resp_cmd._bcid);
	return SUCCESS;
}



_int32 handle_newserver_res_resp(char* buffer, _u32 len, RES_QUERY_CMD* last_cmd)
{
	_int32 ret;
	_u32 i;
	_u32 res_len, comp_len;
	char* tmp_buffer = NULL;
	_int32  tmp_len = 0;
	
	NEWQUERY_SERVER_RES_RESP resp_cmd;
	NEWSERVER_RES server_res;
#ifdef _DEBUG
      char cid[41], gcid[41];
#endif
	
	LOG_DEBUG("handle_newserver_res_resp...");
	if(last_cmd->_user_data == NULL)		
	{
		return SUCCESS;		/*command had been cancel*/
	}
	sd_memset(&resp_cmd, 0, sizeof(NEWQUERY_SERVER_RES_RESP));
	if(extract_newserver_res_resp_cmd(buffer, len, &resp_cmd) != SUCCESS)
	{
		if(NULL != last_cmd->_callback)
		{
			((res_query_notify_shub_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0, NULL, NULL, 0, NULL, 0, 0, 0, NULL, 0);
		}	
	}
	else if(last_cmd->_cmd_seq == resp_cmd._header._seq)
	{
		tmp_buffer = resp_cmd._server_res_buffer;
		tmp_len = (_int32)resp_cmd._server_res_buffer_len;

		LOG_DEBUG("handle_newserver_res_resp...");	
		for(i = 0; i < resp_cmd._server_res_num; ++i)
		{
			sd_memset(&server_res, 0, sizeof(NEWSERVER_RES));
			sd_get_int32_from_lt(&tmp_buffer, &tmp_len, (_int32*)&res_len);
			comp_len = tmp_len;
			sd_get_int32_from_lt(&tmp_buffer, &tmp_len, (_int32*)&server_res._url_size);
			if(server_res._url_size >= MAX_URL_LEN)
			{
				LOG_ERROR("extract_newserver_res_resp_cmd, url_size is too long...");
				sd_assert(FALSE);

				if(NULL != last_cmd->_callback)
				{
					return ((res_query_notify_shub_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0, NULL, NULL, 0, NULL, 0, 0, 0, NULL, 0);
				}
				return SUCCESS;
			}
			sd_get_bytes(&tmp_buffer, &tmp_len, server_res._url, server_res._url_size);
			sd_get_int32_from_lt(&tmp_buffer, &tmp_len, (_int32*)&server_res._url_code_page);
			sd_get_int32_from_lt(&tmp_buffer, &tmp_len, (_int32*)&server_res._ref_url_size);
			if(server_res._ref_url_size >= MAX_URL_LEN)
			{
				LOG_ERROR("extract_newserver_res_resp_cmd, refer_url_size is too long...");
				sd_assert(FALSE);
				if(NULL != last_cmd->_callback)
				{
					return ((res_query_notify_shub_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0, NULL, NULL, 0, NULL, 0, 0, 0, NULL, 0);
				}
				return SUCCESS;
			}
			sd_get_bytes(&tmp_buffer, &tmp_len, server_res._ref_url, server_res._ref_url_size);
			sd_get_int32_from_lt(&tmp_buffer, &tmp_len, (_int32*)&server_res._ref_url_code_page);
			
			sd_get_int32_from_lt(&tmp_buffer, &tmp_len, (_int32*)&server_res._url_speed);
			sd_get_int8(&tmp_buffer, &tmp_len, (_int8*)&server_res._fetch_hint);
			sd_get_int32_from_lt(&tmp_buffer, &tmp_len, (_int32*)&server_res._connect_time);
			sd_get_int8(&tmp_buffer, &tmp_len, (_int8*)&server_res._gcid_type);
			sd_get_int8(&tmp_buffer, &tmp_len, (_int8*)&server_res._max_connection);
			sd_get_int8(&tmp_buffer, &tmp_len, (_int8*)&server_res._min_retry_interval);
			sd_get_int32_from_lt(&tmp_buffer, &tmp_len, (_int32*)&server_res._control_flag);
			sd_get_int8(&tmp_buffer, &tmp_len, (_int8*)&server_res._url_quality);
			sd_get_int32_from_lt(&tmp_buffer, &tmp_len, (_int32*)&server_res._user_agent);
			sd_get_int8(&tmp_buffer, &tmp_len, (_int8*)&server_res._resource_level);
			ret = sd_get_int8(&tmp_buffer, &tmp_len, (_int8*)&server_res._resource_priority);
			if(ret != SUCCESS)
			{
				LOG_ERROR("[version = %u]extract_newserver_res_resp_cmd failed, ret = %d.", resp_cmd._header._version, ret);
				sd_assert(FALSE);

				if(NULL != last_cmd->_callback)
				{
					return ((res_query_notify_shub_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0, NULL, NULL, 0, NULL, 0, 0, 0, NULL, 0);
				}
				return SUCCESS;
			}
			comp_len = comp_len - tmp_len;

			if(comp_len > res_len)
			{
				LOG_ERROR("extract_newserver_res_resp_cmd, comp_len = %u, res_len = %u, not equal.", comp_len, res_len);
				sd_assert(FALSE);

				if(NULL != last_cmd->_callback)
				{
					return ((res_query_notify_shub_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0, NULL, NULL, 0, NULL, 0, 0, 0, NULL, 0);
				}
				return SUCCESS;
			}
			else
			{
				tmp_buffer += (res_len - comp_len);
				tmp_len -= (res_len - comp_len);
				LOG_ERROR("extract_newserver_res_resp_cmd version, comp_len = %u, res_len = %u.", comp_len, res_len);
			}
			LOG_ERROR("add server res, url = %s,_res_priority=%d", server_res._url,server_res._resource_priority);
			if(sd_strlen(server_res._url) == server_res._url_size && (!last_cmd->_not_add_res))
			{	//过滤掉非法的url
				g_add_server_res(last_cmd->_user_data, server_res._url, server_res._url_size, server_res._ref_url, server_res._ref_url_size, FALSE,server_res._resource_priority, FALSE, 0);
			}
			else
			{
				LOG_DEBUG("handle_newserver_res_resp, but url is invalid, url = %s.", server_res._url);
			}
		}

#ifdef _DEBUG
		str2hex((char*)resp_cmd._cid, CID_SIZE, cid, 40);
		cid[40] = '\0';
		str2hex((char*)resp_cmd._gcid, CID_SIZE, gcid, 40);
		gcid[40] = '\0';
		LOG_ERROR("notify_res_query_shub_success_event, cid = %s, gcid = %s, res_num = %u", cid, gcid, resp_cmd._server_res_num);
#ifdef IPAD_KANKAN
		printf("\nnotify_res_query_shub_success_event, cid = %s, gcid = %s, res_num = %u\n", cid, gcid, resp_cmd._server_res_num);
#endif
#endif

		((res_query_notify_shub_handler)last_cmd->_callback)(last_cmd->_user_data, SUCCESS, !resp_cmd._result, resp_cmd._file_size, resp_cmd._cid, resp_cmd._gcid, 0,
											NULL, 0, 0, 0,0,0);
	}
	else
	{
		sd_assert(FALSE);		/*last_cmd->_cmd_seq != resp_cmd._header._seq command not correspond*/
		((res_query_notify_shub_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0, NULL, NULL, 0, NULL, 0, 0, 0, NULL, 0);
	}

	return SUCCESS;
}

_int32 sort_insert_block(RELATION_BLOCK_INFO* p_arr, _u32 block_num ,RELATION_BLOCK_INFO* block_info)
{
	_int32 ret = -1;
	_u32 i = 0, j = 0;
	for(i = 0; i < block_num; i++)
	{
		RELATION_BLOCK_INFO* p_arr_block_info = &p_arr[i];

		if(!p_arr_block_info->_block_info_valid)
		{
			p_arr_block_info->_block_info_valid = TRUE;
			p_arr_block_info->_origion_start_pos = block_info->_origion_start_pos;
			p_arr_block_info->_relation_start_pos = block_info->_relation_start_pos;
			p_arr_block_info->_length = block_info->_length;	
			ret = 0;
			break;
		}

		// 重叠块直接丢弃
		if(p_arr_block_info->_origion_start_pos <= block_info->_origion_start_pos )
		{
			if(p_arr_block_info->_origion_start_pos + p_arr_block_info->_length >  block_info->_origion_start_pos)
			{
				LOG_DEBUG("sort_insert_block blockidx error 11, nebor range handle just remove this block....");
				ret = -1;
				goto Error;
			}
		}

		// 重叠块直接丢弃
		if(p_arr_block_info->_origion_start_pos >= block_info->_origion_start_pos)
		{
			if(block_info->_origion_start_pos + block_info->_length > p_arr_block_info->_origion_start_pos )
			{
				LOG_DEBUG("sort_insert_block blockidx error22 , nebor range handle just remove this block....");
				ret = -1;
				goto Error;
			}
		}
		

		if(p_arr_block_info->_origion_start_pos > block_info->_origion_start_pos)
		{
			sd_assert( (block_num - 1) >  i);
			for(j = block_num - 1; j > i ;j--)
			{
				sd_memcpy(&p_arr[j], &p_arr[j-1], sizeof(RELATION_BLOCK_INFO));
			}
			
			p_arr_block_info->_origion_start_pos = block_info->_origion_start_pos;
			p_arr_block_info->_relation_start_pos = block_info->_relation_start_pos;
			p_arr_block_info->_length = block_info->_length;	
			ret = 0;
			break;
		}
	}
Error:
#ifdef _DEBUG
	for(i = 0; i < block_num; i++)
	{
		RELATION_BLOCK_INFO* p_arr_block_info = &p_arr[i];
		LOG_DEBUG("sort_insert_block blockidx:%u, (%d, %llu, %llu, %llu)", i, p_arr_block_info->_block_info_valid,
			p_arr_block_info->_origion_start_pos, p_arr_block_info->_relation_start_pos, p_arr_block_info->_length);
	}
#endif

	return ret;
}

_int32 handle_query_relation_server_res_resp(char* buffer, _u32 len, RES_QUERY_CMD* last_cmd)
{
	_int32 ret;
	_u32 i;
	_u32 res_len, comp_len;
	char* tmp_buffer = NULL;
	_int32  tmp_len = 0;
	
	QUERY_FILE_RELATION_RES_RESP resp_cmd;
	FILE_RELATION_SERVER_RES server_res;
	RELATION_RESOURCE_INFO   relation_res;
#ifdef _DEBUG
      char cid[41], gcid[41];
#endif
	
	LOG_DEBUG("handle_query_relation_server_res_resp...");
	if(last_cmd->_user_data == NULL)		
	{
		LOG_DEBUG("handle_query_relation_server_res_resp...last_cmd->_user_data == NULL");
		return SUCCESS;		/*command had been cancel*/
	}

#ifdef _DEBUG
	LOG_DEBUG("handle_query_relation_server_res_resp recv buffer is:");
	dump_buffer(buffer, len);
#endif
	
	sd_memset(&resp_cmd, 0, sizeof(QUERY_FILE_RELATION_RES_RESP));
	if(extract_relation_server_res_resp_cmd(buffer, len, &resp_cmd) != SUCCESS)
	{
		((res_query_notify_shub_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0, NULL, NULL, 0, NULL, 0, 0, 0, NULL, 0);
	}
	else if(last_cmd->_cmd_seq == resp_cmd._header._seq)
	{
		tmp_buffer = resp_cmd._relation_server_res_buffer;
		tmp_len = (_int32)resp_cmd._relation_server_res_buffer_len;
		LOG_DEBUG("handle_query_relation_server_res_resp resp_cmd._relation_server_res_num:%u", resp_cmd._relation_server_res_num);
		
		for(i = 0; i < resp_cmd._relation_server_res_num; ++i)
		{
			sd_memset(&server_res, 0, sizeof(FILE_RELATION_SERVER_RES));
			sd_get_int32_from_lt(&tmp_buffer, &tmp_len, (_int32*)&res_len);
			comp_len = tmp_len;
			sd_get_int32_from_lt(&tmp_buffer, &tmp_len, (_int32*)&server_res._gcid_size);
			if(server_res._gcid_size != CID_SIZE)
			{
				LOG_ERROR("handle_query_relation_server_res_resp extract_server_res_resp_cmd, _gcid_size is not correct...");
				//sd_assert(FALSE);
				return ((res_query_notify_shub_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0, NULL, NULL, 0, NULL, 0, 0, 0, NULL, 0);
			}
			sd_get_bytes(&tmp_buffer, &tmp_len, (char*)server_res._gcid, (_int32)server_res._gcid_size);
			
			sd_get_int32_from_lt(&tmp_buffer, &tmp_len, (_int32*)&server_res._cid_size);
			if(server_res._cid_size != CID_SIZE)
			{
				LOG_ERROR("handle_query_relation_server_res_resp extract_server_res_resp_cmd, cid_size is not correct..");
				//sd_assert(FALSE);
				return ((res_query_notify_shub_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0, NULL, NULL, 0, NULL, 0, 0, 0, NULL, 0);
			}
			sd_get_bytes(&tmp_buffer, &tmp_len, (char*)server_res._cid, (_int32)server_res._cid_size);



			#ifdef _DEBUG
			str2hex((char*)server_res._cid, CID_SIZE, cid, 40);
			cid[40] = '\0';
			str2hex((char*)server_res._gcid, CID_SIZE, gcid, 40);
			gcid[40] = '\0';
			LOG_ERROR("handle_query_relation_server_res_resp, cid = %s, gcid = %s", cid, gcid);
			#endif
			

			sd_get_int32_from_lt(&tmp_buffer, &tmp_len, (_int32*)&server_res._file_name_size);
			if(server_res._file_name_size >= MAX_FILE_NAME_LEN)
			{
				LOG_ERROR("handle_query_relation_server_res_resp extract_server_res_resp_cmd, _file_name_size is not correct..");
				return ((res_query_notify_shub_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0, NULL, NULL, 0, NULL, 0, 0, 0, NULL, 0);
			}
			sd_get_bytes(&tmp_buffer, &tmp_len, (char*)server_res._file_name, (_int32)server_res._file_name_size);

			sd_get_int32_from_lt(&tmp_buffer, &tmp_len, (_int32*)&server_res._file_suffix_length);
			if(server_res._file_suffix_length >= 16)
			{
				LOG_ERROR("handle_query_relation_server_res_resp extract_server_res_resp_cmd, _file_suffix_length is not correct..");
				return ((res_query_notify_shub_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0, NULL, NULL, 0, NULL, 0, 0, 0, NULL, 0);
			}
			sd_get_bytes(&tmp_buffer, &tmp_len, server_res._file_suffix, server_res._file_suffix_length);

			sd_get_int64_from_lt(&tmp_buffer, &tmp_len, (_int64*)&server_res._file_size);

			_u32 tmp_num = 0;
			sd_get_int32_from_lt(&tmp_buffer, &tmp_len, (_int32*)&tmp_num);

			sd_get_int64_from_lt(&tmp_buffer, &tmp_len, (_int64*)&server_res._block_total_size);

			_u32  blocknum = 0;
			sd_get_int32_from_lt(&tmp_buffer, &tmp_len, (_int32*)&blocknum);
			_u32 block_index = 0;

			if(blocknum == 0 || blocknum >= 10000)
			{
				LOG_ERROR("handle_query_relation_server_res_resp extract_server_res_resp_cmd, blocknum is not correct..");
				return ((res_query_notify_shub_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0, NULL, NULL, 0, NULL, 0, 0, 0, NULL, 0);
			}			

			ret = sd_malloc(sizeof(RELATION_BLOCK_INFO)*blocknum, (void**)&server_res._p_block_info);
			CHECK_VALUE(ret);

			sd_memset(server_res._p_block_info, 0, sizeof(RELATION_BLOCK_INFO)*blocknum);
			server_res._block_info_num= 0;
			for(block_index = 0; block_index < blocknum; block_index++)
			{
				_u32 block_struct_length = 0;
				
				RELATION_BLOCK_INFO block;;
				sd_memset(&block, 0, sizeof(RELATION_BLOCK_INFO));
				sd_get_int32_from_lt(&tmp_buffer, &tmp_len, (_int32*)&block_struct_length);

				_u32 block_comp_len = tmp_len;

				sd_get_int64_from_lt(&tmp_buffer, &tmp_len, (_int64*)&block._origion_start_pos);
				sd_get_int64_from_lt(&tmp_buffer, &tmp_len, (_int64*)&block._relation_start_pos);
				sd_get_int64_from_lt(&tmp_buffer, &tmp_len, (_int64*)&block._length);

				block_comp_len = block_comp_len - tmp_len;

				if(block_comp_len > block_struct_length)
				{
					LOG_ERROR("extract server res failed, block_comp_len = %u, block_struct_length=%u", block_comp_len, block_struct_length);
					//sd_assert(FALSE);
					SAFE_DELETE(server_res._p_block_info);
					return ((res_query_notify_shub_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0, NULL, NULL, 0, NULL, 0, 0, 0, NULL, 0);
				}

				tmp_buffer += (block_struct_length - block_comp_len);
				tmp_len -= (block_struct_length - block_comp_len);

				LOG_DEBUG("handle_query_relation_server_res_resp blockinfo(%llu,%llu, %llu)", block._origion_start_pos, block._relation_start_pos, block._length);

				if(block._length >= MIN_VALID_RELATION_BLOCK_LENGTH)
				{
					_int32 ret = sort_insert_block(server_res._p_block_info, server_res._block_info_num + 1, &block);
					if(ret == 0)
					{
						server_res._block_info_num++;
					}
				}
			
			}




			_u32 relation_res_num = 0;
			sd_get_int32_from_lt(&tmp_buffer, &tmp_len, (_int32*)&relation_res_num);

			_u32 relation_res_index = 0;
			if( relation_res_num  >= MAX_SERVER*3)
			{
				LOG_ERROR("extract server res failed, relation_res_num too big or zero:%u.", relation_res_num);
				SAFE_DELETE(server_res._p_block_info);
				return ((res_query_notify_shub_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0, NULL, NULL, 0, NULL, 0, 0, 0, NULL, 0);	
			}

			_u32 current_relation_index = -1;
			if(server_res._block_info_num > 0 && relation_res_num > 0)
			{
				current_relation_index = g_add_relation_fileinfo(last_cmd->_user_data, server_res._gcid, server_res._cid, server_res._file_size, server_res._block_info_num, &server_res._p_block_info);
			}

			
			SAFE_DELETE(server_res._p_block_info);

			for(relation_res_index = 0; relation_res_index < relation_res_num; relation_res_index++)
			{
				_u32 relation_res_structlen = 0;
				_u32 relation_res_comp = 0;
				sd_memset(&relation_res, 0, sizeof(relation_res));

				sd_get_int32_from_lt(&tmp_buffer, &tmp_len, (_int32*)&relation_res_structlen);
				relation_res_comp = tmp_len;
				
				sd_get_int32_from_lt(&tmp_buffer, &tmp_len, (_int32*)&relation_res._url_size);
				sd_get_bytes(&tmp_buffer, &tmp_len, relation_res._url, relation_res._url_size);
				sd_get_int32_from_lt(&tmp_buffer, &tmp_len, (_int32*)&relation_res._url_codepage);

				LOG_DEBUG("relation_res._url:%s", relation_res._url);

				sd_get_int32_from_lt(&tmp_buffer, &tmp_len, (_int32*)&relation_res._refurl_size);
				sd_get_bytes(&tmp_buffer, &tmp_len, relation_res._url, relation_res._refurl_size);
				sd_get_int32_from_lt(&tmp_buffer, &tmp_len, (_int32*)&relation_res._refurl_codepage);				

				sd_get_int8(&tmp_buffer, &tmp_len, (_int8*)&relation_res._res_level);
				sd_get_int8(&tmp_buffer, &tmp_len, (_int8*)&relation_res._res_priority);	

				relation_res_comp = relation_res_comp - tmp_len;

				if(relation_res_comp > relation_res_structlen)
				{
					LOG_ERROR("extract server res failed, relation_res_comp = %u, relation_res_structlen=%u", relation_res_comp, relation_res_structlen);
					//sd_assert(FALSE);
					return ((res_query_notify_shub_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0, NULL, NULL, 0, NULL, 0, 0, 0, NULL, 0);
				}

				tmp_buffer += (relation_res_structlen - relation_res_comp);
				tmp_len -= (relation_res_structlen - relation_res_comp);

				if(current_relation_index != -1)
				{
					g_add_server_res(last_cmd->_user_data, relation_res._url, relation_res._url_size, relation_res._refurl, relation_res._refurl_size, FALSE,relation_res._res_priority, TRUE, current_relation_index);
				}
			}

			comp_len = comp_len - tmp_len;
			if(comp_len > res_len)
			{
				LOG_ERROR("extract server res failed, comp_len = %u, res_len = %u, not equal.", comp_len, res_len);
				//sd_assert(FALSE);
				SAFE_DELETE(server_res._p_block_info);
				return ((res_query_notify_shub_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0, NULL, NULL, 0, NULL, 0, 0, 0, NULL, 0);
			}
			else
			{
				tmp_buffer += (res_len - comp_len);
				tmp_len -= (res_len - comp_len);
				LOG_ERROR("extract server res new version, comp_len = %u, res_len = %u.", comp_len, res_len);
			}		
		}



		((res_query_notify_shub_handler)last_cmd->_callback)(last_cmd->_user_data, SUCCESS,  0, 0, NULL, NULL, 0, NULL, 0, 0, 0, NULL, 0);
	}
	else
	{
		//sd_assert(FALSE);		/*last_cmd->_cmd_seq != resp_cmd._header._seq command not correspond*/
		((res_query_notify_shub_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0, NULL, NULL, 0, NULL, 0, 0, 0, NULL, 0);
	}

	return SUCCESS;
}

_int32 handle_peer_res_resp(char* buffer, _u32 len, RES_QUERY_CMD* last_cmd, _u8 res_from)
{
	PEER_RES_RESP_CMD	resp_cmd;
	PEER_RES			peer_res;
	_int32				ret, tmp_len, res_len;
	_u32				 filename_size, i;
	char*				tmp_buffer;
#ifdef _DEBUG
	char	target_peerid[PEER_ID_SIZE + 1] = {0};
	char cid[41], gcid[41];
#endif
	char * p_from = NULL;
	switch(res_from)
	{
		case P2P_FROM_HUB:
			p_from = "phub";
			break;
		case P2P_FROM_PARTNER_CDN:
			p_from = "partner_cdn";
			break;
		case P2P_FROM_VIP_HUB:
			p_from = "vip_hub";
			break;
		case P2P_FROM_NORMAL_CDN:
			p_from = "normal_cdn_manager";
			break;
		default:
			p_from = "other phub";
			break;
	}

	LOG_DEBUG("handle_peer_res_resp:%s", p_from);
	if(last_cmd->_user_data == NULL)
	{
		return SUCCESS;
	}
	
	sd_memset(&resp_cmd, 0, sizeof(PEER_RES_RESP_CMD));
	if(extract_peer_res_resp_cmd(buffer, len, &resp_cmd) != SUCCESS)
	{
		((res_query_notify_phub_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0);
	}
	else if(last_cmd->_cmd_seq == resp_cmd._header._seq)
	{
		tmp_buffer = resp_cmd._peer_res_buffer;
		tmp_len = (_int32)resp_cmd._peer_res_buffer_len;

		for(i = 0; i < resp_cmd._peer_res_num; ++i)
		{
			sd_memset(&peer_res, 0, sizeof(PEER_RES));
			sd_get_int32_from_lt(&tmp_buffer, &tmp_len, (_int32*)&res_len);
			if(tmp_len < res_len)
				break;
			tmp_len -= res_len;
			sd_get_int32_from_lt(&tmp_buffer, &res_len, (_int32*)&peer_res._peerid_size);
			if(peer_res._peerid_size != PEER_ID_SIZE)
			{
				LOG_ERROR("[version = %u]extract_peer_res_resp_cmd %s failed, res->_peerid_size = %u", 
					p_from,resp_cmd._header._version, peer_res._peerid_size);
				sd_assert(FALSE);
				return ((res_query_notify_phub_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0);
			}
			sd_get_bytes(&tmp_buffer, &res_len, peer_res._peerid, peer_res._peerid_size);
			peer_res._peerid[peer_res._peerid_size] = '\0';
			sd_get_int32_from_lt(&tmp_buffer, &res_len, (_int32*)&filename_size);
			tmp_buffer += filename_size;	/*escape file_name which is no use in new p2p protocol*/
			sd_get_bytes(&tmp_buffer, &res_len, (char*)&peer_res._internal_ip, sizeof(_u32));
			sd_get_int16_from_lt(&tmp_buffer, &res_len, (_int16*)&peer_res._tcp_port);
			if(res_len == 1)
			{
				//这是53版本的格式，之所以还存在该格式，原因是一些小的运营商破解
				//了我们的协议，会自己在协议里面插入自身的cache peer,这种peer还是用的
				//53版本的格式
				sd_get_int8(&tmp_buffer, &res_len, (_int8*)&peer_res._peer_capability);
			}
			else
			{
				sd_get_int16_from_lt(&tmp_buffer, &res_len, (_int16*)&peer_res._udp_port);
				sd_get_int8(&tmp_buffer, &res_len, (_int8*)&peer_res._resource_level);
				sd_get_int8(&tmp_buffer, &res_len, (_int8*)&peer_res._resource_priority);
				ret = sd_get_int32_from_lt(&tmp_buffer, &res_len, (_int32*)&peer_res._peer_capability);
				if(ret != SUCCESS)
				{
					LOG_ERROR("[version = %u]extract_peer_res_resp_cmd %s failed, ret = %d.",p_from, resp_cmd._header._version, ret);
					sd_assert(FALSE);
					return ((res_query_notify_phub_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0);
				}
				
				if(res_len != 0)
				{
					LOG_ERROR("[version = %u]extract_peer_res_resp_cmd %s warnning, get more %d bytes data!", resp_cmd._header._version, p_from,res_len);
					tmp_buffer+=res_len;
				}
			}
#ifdef	_DEBUG
			settings_get_str_item("p2p_setting.download_from_peerid", target_peerid);
			if(sd_strlen(target_peerid) != 0 && sd_strcmp(peer_res._peerid, target_peerid) != 0)		//这一句是为了调试才加上的
				continue;
#endif
			LOG_DEBUG("add peer res from %s, peerid = %s, peer_capability = 0x%X, ip = %u, tcp_port = %u, udp_port = %u, res_level = %u, res_priority = %u"
			        , p_from,peer_res._peerid, peer_res._peer_capability
			        , peer_res._internal_ip, peer_res._tcp_port
			        , peer_res._udp_port, peer_res._resource_level
			        , peer_res._resource_priority);

#if defined(MOBILE_PHONE)
			if((NT_GPRS_WAP & sd_get_net_type())&&(((peer_res._peer_capability&0x00008000)==0)||peer_res._peer_capability&0x00000001))
			{
				LOG_ERROR("Discard the peer from %s which do not support 'HTTP' packaging in CMWAP",p_from);
				continue;
			}
#endif /* MOBILE_PHONE  */			
			g_add_peer_res(last_cmd->_user_data, peer_res._peerid, resp_cmd._gcid, resp_cmd._file_size, peer_res._peer_capability,
						peer_res._internal_ip, peer_res._tcp_port, peer_res._udp_port, peer_res._resource_level, res_from, peer_res._resource_priority);
		}

#ifdef _DEBUG
		str2hex((char*)resp_cmd._cid, CID_SIZE, cid, 40);
		cid[40] = '\0';
		str2hex((char*)resp_cmd._gcid, CID_SIZE, gcid, 40);
		gcid[40] = '\0';
		LOG_ERROR("notify_res_query_%s_success, cid = %s, gcid = %s, res_num = %u",p_from, cid, gcid, resp_cmd._peer_res_num);
#ifdef IPAD_KANKAN
		printf("\nnotify_res_query_%s_success, cid = %s, gcid = %s, res_num = %u\n", p_from,cid, gcid, resp_cmd._peer_res_num);
#endif
#endif
		((res_query_notify_phub_handler)last_cmd->_callback)(last_cmd->_user_data, SUCCESS, resp_cmd._result, 60 * 10);
	}
	else
	{
		sd_assert(FALSE);		/*last_cmd->_cmd_seq != resp_cmd._header._seq command not correspond*/
		LOG_ERROR("notify_res_query_%s_failed!",p_from);
#ifdef IPAD_KANKAN
		printf("\n notify_res_query_%s_failed!\n",p_from);
#endif
		((res_query_notify_phub_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0);
	}
	return SUCCESS;
}

_int32 handle_tracker_res_resp(char* buffer, _u32 len, RES_QUERY_CMD* last_cmd)
{
	TRACKER_RES_RESP_CMD resp_cmd;
	TRACKER_RES		 tracker_res;
	char*			 tmp_buffer;
	_u32			 i;
	_int32			 ret, tmp_len;
	LOG_DEBUG("handle_tracker_res_resp...");
	if(last_cmd->_user_data == NULL)
	{
		return SUCCESS;
	}
	sd_memset(&resp_cmd, 0, sizeof(TRACKER_RES_RESP_CMD));
	if(extract_tracker_res_resp_cmd(buffer, len, &resp_cmd) != SUCCESS)
	{
		((res_query_notify_tracker_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0, 0);
	}
	else if(resp_cmd._header._seq == last_cmd->_cmd_seq)
	{
		tmp_buffer = resp_cmd._tracker_res_buffer;
		tmp_len = (_int32)resp_cmd._tracker_res_buffer_len;
		for(i = 0; i < resp_cmd._tracker_res_num; ++i)
		{
			sd_get_int32_from_lt(&tmp_buffer, &tmp_len, (_int32*)&tracker_res._peerid_size);
			if(tracker_res._peerid_size != PEER_ID_SIZE)
			{
				LOG_ERROR("[version = %u]extract_tracker_res_resp_cmd failed, tracker_res._peerid_size = %u", resp_cmd._header._version, tracker_res._peerid_size);
				sd_assert(FALSE);
				return ((res_query_notify_tracker_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0, 0);
			}
			sd_get_bytes(&tmp_buffer, &tmp_len, tracker_res._peerid, tracker_res._peerid_size);
			tracker_res._peerid[tracker_res._peerid_size] = '\0';
			sd_get_bytes(&tmp_buffer, &tmp_len, (char*)&tracker_res._internal_ip, sizeof(_u32));
			sd_get_int16_from_lt(&tmp_buffer, &tmp_len, (_int16*)&tracker_res._tcp_port);
			sd_get_int16_from_lt(&tmp_buffer, &tmp_len, (_int16*)&tracker_res._udp_port);
			sd_get_int8(&tmp_buffer, &tmp_len, (_int8*)&tracker_res._resource_level);
			sd_get_int8(&tmp_buffer, &tmp_len, (_int8*)&tracker_res._resource_priority);
			ret = sd_get_int32_from_lt(&tmp_buffer, &tmp_len, (_int32*)&tracker_res._peer_capability);
			if(ret != SUCCESS)
			{
				LOG_ERROR("[version = %u]extract_tracker_res_resp_cmd failed, ret = %d", resp_cmd._header._version, ret);
				sd_assert(FALSE);
				return ((res_query_notify_tracker_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0, 0);
			}
#ifdef _DEBUG
			{
				char	target_peerid[PEER_ID_SIZE + 1] = {0};
				settings_get_str_item("p2p_setting.download_from_peerid", target_peerid);
				if(sd_strlen(target_peerid) != 0 && sd_strcmp(tracker_res._peerid, target_peerid) != 0)		//这一句是为了调试才加上的
					continue;
			}
#endif
			LOG_ERROR("TRACKER:add peer res, peerid = %s, peer_capability = 0x%X, ip = %u, tcp_port = %u, udp_port = %u, res_level = %u", tracker_res._peerid, tracker_res._peer_capability,
					tracker_res._internal_ip, tracker_res._tcp_port, tracker_res._udp_port, tracker_res._resource_level);

#if defined(MOBILE_PHONE)
			if((NT_GPRS_WAP & sd_get_net_type())&&(((tracker_res._peer_capability&0x00008000)==0)||tracker_res._peer_capability&0x00000001))
			{
				LOG_ERROR("Discard the peer which do not support 'HTTP' packaging in CMWAP");
				continue;
			}
#endif /* MOBILE_PHONE  */			
			g_add_peer_res(last_cmd->_user_data, tracker_res._peerid, NULL, -1, tracker_res._peer_capability, 
							tracker_res._internal_ip, tracker_res._tcp_port, tracker_res._udp_port
							, tracker_res._resource_level, P2P_FROM_TRACKER, tracker_res._resource_priority);
		}
		LOG_ERROR("notify_res_query_tracker_success, res_num = %u", resp_cmd._tracker_res_num);	
#ifdef _DEBUG
#ifdef IPAD_KANKAN
		printf("\nnotify_res_query_tracker_success, res_num = %u\n", resp_cmd._tracker_res_num);	
#endif
#endif
		((res_query_notify_tracker_handler)last_cmd->_callback)(last_cmd->_user_data, SUCCESS, resp_cmd._result, 60 * 10, resp_cmd._query_stamp);
	}
	else
	{
		sd_assert(FALSE);		/*last_cmd->_cmd_seq != resp_cmd._header._seq command not correspond*/
		((res_query_notify_tracker_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0, 0);
	}
	return SUCCESS;
}

_int32 handle_enrollsp1_resp(char* buffer, _u32 len, RES_QUERY_CMD* last_cmd)
{
	void* data = NULL;
	ENROLLSP1_RESP_CMD	resp_cmd;
	LOG_DEBUG("handle_enrollsp1_resp...");
	extract_enrollsp1_resp_cmd(buffer, len, &resp_cmd);		//解析后的数据目前暂时没有使用
	/*notify response command*/
	while(list_size(&resp_cmd._client_conf_setting_list) > 0)
	{
		list_pop(&resp_cmd._client_conf_setting_list, &data);
		sd_free(data);
		data = NULL;
	}
	while(list_size(&resp_cmd._servers_list) > 0)
	{
		list_pop(&resp_cmd._servers_list, &data);
		sd_free(data);
		data = NULL;
	}
	return SUCCESS;
}

_int32 handle_query_config_resp(char* buffer, _u32 len, RES_QUERY_CMD* last_cmd)
{
	void* data = NULL;
	QUERY_CONFIG_RESP_CMD	resp_cmd;
	char config_item[64] = {0};
	LOG_DEBUG("handle_query_config_resp...");
	extract_query_config_resp_cmd(buffer, len, &resp_cmd);		//解析后的数据目前暂时没有使用

	settings_set_int_item("query_config.timestamp", resp_cmd._timestamp);
	/*notify response command*/
	while(list_size(&resp_cmd._setting_list) > 0)
	{
		list_pop(&resp_cmd._setting_list, &data);
		
		CONF_SETTING_NODE *pNode = ((CONF_SETTING_NODE *)data);
		sd_snprintf(config_item, 64, "%s.%s", pNode->_section, pNode->_name);
		settings_set_str_item(config_item, pNode->_value);
		
		sd_free(data);
		data = NULL;
	}

    // 刷新下配置文件
    settings_config_save();

	return SUCCESS;
}

#ifdef ENABLE_EMULE
_int32 handle_emule_tracker_res_resp(char *buffer, _int32 len, RES_QUERY_CMD *last_cmd)
{
    QUERY_EMULE_TRACKER_RESP_CMD resp_cmd;
    EMULE_TRACKER_PEER emule_tracker_res;
    char* tmp_buffer;
    _u32 i;
    _int32 ret, tmp_len;
    _u32 peer_capability = 0;
    EMULE_TASK *emule_task = 0;
    _u32 client_id = 0;

    LOG_DEBUG("handle_emule_tracker_res_resp.");

    if(last_cmd->_user_data == NULL)
    {
        return SUCCESS;
    }

    emule_task = (EMULE_TASK *)(((RES_QUERY_PARA *)(last_cmd->_user_data))->_task_ptr);
    sd_memset(&resp_cmd, 0, sizeof(QUERY_EMULE_TRACKER_RESP_CMD));
    if(emule_extract_query_emule_tracker_resp_cmd(buffer, len, &resp_cmd) != SUCCESS)
    {
        ((res_query_notify_tracker_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0, 0);
    }
    else if(resp_cmd._header._seq == last_cmd->_cmd_seq)
    {
        LOG_DEBUG("emule_extract_query_emule_tracker_resp_cmd SUCCESS, peer num = %d.", 
            resp_cmd._peer_res_num);

        tmp_buffer = resp_cmd._peer_res_buffer;
        tmp_len = (_int32)resp_cmd._peer_res_buffer_len;

        for(i = 0; i < resp_cmd._peer_res_num; ++i)
        {
            sd_get_int32_from_lt(&tmp_buffer, &tmp_len, (_int32*)&emule_tracker_res._user_id_len);
            if(emule_tracker_res._user_id_len != USER_ID_SIZE)
            {
                LOG_ERROR("[version = %u]emule_extract_query_emule_tracker_resp_cmd failed, emule_tracker_res._user_id_len = %u", 
                    resp_cmd._header._version, emule_tracker_res._user_id_len);
                sd_assert(FALSE);
                return ((res_query_notify_tracker_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0, 0);
            }
            sd_get_bytes(&tmp_buffer, &tmp_len, (char*)emule_tracker_res._user_id, (_int32)emule_tracker_res._user_id_len);
            emule_tracker_res._user_id[emule_tracker_res._user_id_len] = '\0';

            sd_get_int32_from_lt(&tmp_buffer, &tmp_len, (_int32*)&emule_tracker_res._peer_id_len);
            if(emule_tracker_res._peer_id_len != PEER_ID_SIZE)
            {
                LOG_ERROR("[version = %u]emule_extract_query_emule_tracker_resp_cmd failed, emule_tracker_res._peer_id_len = %u", 
                    resp_cmd._header._version, emule_tracker_res._peer_id_len);
                sd_assert(FALSE);
                return ((res_query_notify_tracker_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0, 0);
            }
            sd_get_bytes(&tmp_buffer, &tmp_len, (char*)emule_tracker_res._peer_id, (_int32)emule_tracker_res._peer_id_len);
            emule_tracker_res._peer_id[emule_tracker_res._peer_id_len] = '\0';

            sd_get_int32_from_lt(&tmp_buffer, &tmp_len, (_int32*)&emule_tracker_res._ip);
            sd_get_int16_from_lt(&tmp_buffer, &tmp_len, (_int16*)&emule_tracker_res._port);
            ret = sd_get_int8(&tmp_buffer, &tmp_len, (_int8*)&emule_tracker_res._capability);
            peer_capability 
                = emule_tracker_peer_capability_2_local_peer_capability(emule_tracker_res._capability);
            if(ret != SUCCESS)
            {
                LOG_ERROR("[version = %u]emule_extract_query_emule_tracker_resp_cmd failed, ret = %d", 
                    resp_cmd._header._version, ret);
                sd_assert(FALSE);
                return ((res_query_notify_tracker_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0, 0);
            }
#if defined(MOBILE_PHONE)
            if((NT_GPRS_WAP & sd_get_net_type()) 
                && ((peer_capability&0x00008000 == 0) || (peer_capability&0x00000001 == 0)))
            {
                LOG_ERROR("Discard the peer which do not support 'HTTP' packaging in CMWAP");
                continue;
            }
#endif /* MOBILE_PHONE  */

            // 由于emule_tracker并没有返回client_id，所以这里要设置一下
            if (is_same_nat(peer_capability) || is_nated(peer_capability))
            {
                client_id = sd_rand() % EMULE_DEFAULT_LOW_CLIENT_ID;
                if (client_id == emule_get_local_client_id())
                    client_id = ((sd_rand()%EMULE_DEFAULT_LOW_CLIENT_ID)+client_id)%EMULE_DEFAULT_LOW_CLIENT_ID;
                LOG_DEBUG("rmt peer from emule tracker is in nat or same nat with local, so change client_id = %u",
                    client_id);
            }
            else
            {
                client_id = emule_tracker_res._ip;
            }
            cm_add_emule_resource(&emule_task->_task._connect_manager, client_id, emule_tracker_res._port, 0, 0);
         }

         LOG_DEBUG("notify_res_query_emule_tracker_success, res_num = %u", resp_cmd._peer_res_num);

         ((res_query_notify_tracker_handler)last_cmd->_callback)(last_cmd->_user_data, SUCCESS, 
             resp_cmd._result, 60 * 10, resp_cmd._query_stamp);
    }
    else
    {
        sd_assert(FALSE);		/*last_cmd->_cmd_seq != resp_cmd._header._seq command not correspond*/
        ((res_query_notify_tracker_handler)last_cmd->_callback)(last_cmd->_user_data, -1, 0, 0, 0);
    }
    return SUCCESS;
}
#endif 

#ifdef ENABLE_BT

_int32 handle_bt_info_resp(char* buffer, _u32 len, RES_QUERY_CMD* last_cmd)
{
	QUERY_BT_INFO_RESP_CMD resp_cmd;
	res_query_bt_info_handler callback = (res_query_bt_info_handler)last_cmd->_callback;
	LOG_DEBUG("handle_bt_info_resp...");
	if(last_cmd->_user_data == NULL)
	{
		LOG_DEBUG("handle_bt_info_resp, but query_bt_info request is cancel, no notify.");
		return SUCCESS;
	}
	sd_memset(&resp_cmd, 0, sizeof(QUERY_BT_INFO_RESP_CMD));
	if(extract_query_bt_info_resp_cmd(buffer, len, &resp_cmd) != SUCCESS)
	{
		callback(last_cmd->_user_data, ERR_RES_QUERY_EXTRACT_CMD_FAIL, 0, 0, 0, NULL, NULL, 0, NULL, 0, 0, 0);
	}
	else if(resp_cmd._header._seq == last_cmd->_cmd_seq)
	{
		LOG_DEBUG("query bt info response.");
		callback(last_cmd->_user_data, SUCCESS, resp_cmd._result, resp_cmd._has_record, resp_cmd._file_size, resp_cmd._cid, resp_cmd._gcid, resp_cmd._gcid_level,
			resp_cmd._bcid, resp_cmd._bcid_size, resp_cmd._gcid_part_size, resp_cmd._control_flag);
	}
	else
	{
		sd_assert(FALSE);
		callback(last_cmd->_user_data, ERR_RES_QUERY_EXTRACT_CMD_FAIL, 0, 0, 0, NULL, NULL, 0, NULL, 0, 0, 0);
	}
	return SUCCESS;
}

#endif

_int32 handle_recv_resp_cmd_with_aes_key(char* buffer, _u32 len, HUB_DEVICE* device, _u8 *p_aeskey)
{
	_u16 cmd_type = 0;
	_int32 tmp_len = len;
	char* cmd_ptr =NULL;
	_int32 ret;

	ret = aes_decrypt_with_known_key(buffer, &len, p_aeskey);
	CHECK_VALUE(ret);

	ret = xl_aes_decrypt(buffer, &len);

	CHECK_VALUE(ret);

	LOG_DEBUG("device(%d).", device->_hub_type);

	if(device->_hub_type == EMULE_HUB)
	{
		return ((res_query_notify_recv_data)device->_last_cmd->_callback)(SUCCESS, buffer, len, device->_last_cmd->_user_data);
	}

	tmp_len = len;
	
	if(device->_hub_type == SHUB)
	{
		_u32 shub_ver = 0;
		cmd_ptr = buffer;
		sd_get_int32_from_lt((char**)&(cmd_ptr), &tmp_len,  (_int32*)&shub_ver);	

		if(shub_ver >= 60)
		{
			dump_buffer(buffer, len);
			tmp_len = len;
			cmd_ptr = buffer + HUB_CMD_HEADER_LEN + 6;
			tmp_len -= (HUB_CMD_HEADER_LEN + 6);
			_int32 reserver_length = 0;
			sd_get_int32_from_lt((char**)&(cmd_ptr), &tmp_len,  &reserver_length);
			tmp_len -= reserver_length;
			cmd_ptr += reserver_length;
			sd_get_int16_from_lt((char**)&(cmd_ptr), &tmp_len, (_int16*)&cmd_type);	
		}
		else
		{
			tmp_len = len;
			cmd_ptr = buffer + HUB_CMD_HEADER_LEN + 6;
			tmp_len -= (HUB_CMD_HEADER_LEN + 6);
			sd_get_int16_from_lt((char**)&(cmd_ptr), &tmp_len, (_int16*)&cmd_type);		
		}
	}
	else if( device->_hub_type == BT_HUB)
	{
		cmd_ptr = buffer + HUB_CMD_HEADER_LEN + 6;
		tmp_len -= (HUB_CMD_HEADER_LEN + 6);
		sd_get_int16_from_lt((char**)&(cmd_ptr), &tmp_len, (_int16*)&cmd_type);		
	}
	else if ((device->_hub_type == DPHUB_ROOT) || (device->_hub_type == DPHUB_NODE))
	{
		cmd_ptr = buffer + HUB_CMD_HEADER_LEN;
		tmp_len -= HUB_CMD_HEADER_LEN;
		sd_get_int16_from_lt((char**)&(cmd_ptr), &tmp_len, (_int16*)&cmd_type);		
	}
	else
	{	
		cmd_type = *(_u8*)(buffer + HUB_CMD_HEADER_LEN);
	}
	LOG_DEBUG("command type is %d", (_int32)cmd_type);
	switch(cmd_type)
	{
	case SERVER_RES_RESP:
		return handle_server_res_resp(buffer, len, device->_last_cmd);
	case QUERY_RES_INFO_RESP_ID:
		return handle_server_res_info(buffer, len, device->_last_cmd);
	case QUERY_NEW_RES_RESP_ID:
		return handle_newserver_res_resp(buffer, len, device->_last_cmd);
	case QUERY_FILE_RELATION_RESP:
		return handle_query_relation_server_res_resp(buffer, len, device->_last_cmd);
	case PEER_RES_RESP:
		{
			if( device->_hub_type == PARTNER_CDN )
			{
				return handle_peer_res_resp(buffer, len, device->_last_cmd, P2P_FROM_PARTNER_CDN);
			}
			else if(device->_hub_type == VIP_HUB)
			{
				return handle_peer_res_resp(buffer, len, device->_last_cmd, P2P_FROM_VIP_HUB);
			}
			else
			{
				return handle_peer_res_resp(buffer, len, device->_last_cmd, P2P_FROM_HUB);
			}
		}
	case TRACKER_RES_RESP:
		return handle_tracker_res_resp(buffer, len, device->_last_cmd);
#ifdef ENABLE_BT
	case QUERY_BT_INFO_RESP:
		return handle_bt_info_resp(buffer, len, device->_last_cmd);
#endif
	case ENROLLSP1_RESP:
		return handle_enrollsp1_resp(buffer, len, device->_last_cmd);
	case DPHUB_OWNER_QUERY_RESP_ID:
		return handle_query_owner_node_resp(buffer, len, device->_last_cmd);
	case DPHUB_RC_QUERY_RESP_ID:
		return handle_rc_query_resp(g_add_peer_res, buffer, len, device->_last_cmd);
	case DPHUB_RC_NODE_QUERY_RESP_ID:
		return handle_rc_node_query_resp(buffer, len, device->_last_cmd);
	case QUERY_CONFIG_RESP:
		return handle_query_config_resp(buffer, len, device->_last_cmd);
#ifdef ENABLE_EMULE
	case QUERY_EMULE_TRACKER_RESP:
		return handle_emule_tracker_res_resp(buffer, len, device->_last_cmd);		
#endif
	}
	LOG_ERROR("handle_recv_resp_cmd, but cmd_type is invalid...");
	sd_assert(FALSE);
	return -1;
}
