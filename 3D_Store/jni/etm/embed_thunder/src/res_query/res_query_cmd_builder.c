#include "utility/mempool.h"
#include "utility/bytebuffer.h"
#include "utility/md5.h"
#include "utility/aes.h"
#include "utility/utility.h"
#include "utility/logid.h"
#include "utility/version.h"
#define LOGID LOGID_RES_QUERY
#include "utility/logger.h"
#include "platform/sd_network.h"

#include "embed_thunder_version.h"

#include "res_query_cmd_builder.h"
#include "res_query_setting.h"
#include "res_query_interface_imp.h"
#include "res_query_security.h"

static _u32 g_shub_seq = 0;

/*该函数并不接管buffer指向的内存*/
_int32 xl_aes_encrypt(char* buffer,_u32* len)
{
	_int32 ret;
	char *pOutBuff;
	char *tmp_buffer = NULL;
	_int32 tmplen = *len;
	_int32 nOutLen;
	_int32 nBeginOffset;
	_u8 szKey[16];
	ctx_md5	md5;
	ctx_aes	aes;
	int nInOffset;
	int nOutOffset;
	unsigned char inBuff[ENCRYPT_BLOCK_SIZE],ouBuff[ENCRYPT_BLOCK_SIZE];
	if (buffer == NULL)
	{
		return -1;
	}
	ret = sd_malloc(*len + 16, (void**)&pOutBuff);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("aes_encrypt malloc failed.");
		CHECK_VALUE(ret);
	}
	nOutLen = 0;
	nBeginOffset = sizeof(_u32)*3;
	md5_initialize(&md5);
	md5_update(&md5, (const unsigned char*)buffer, sizeof(_u32) * 2);
	md5_finish(&md5, szKey);
	/*aes encrypt*/
	aes_init(&aes, 16, szKey);
	nInOffset = nBeginOffset; //不是从头开始加密
	nOutOffset = 0;
	sd_memset(inBuff,0,ENCRYPT_BLOCK_SIZE);
	sd_memset(ouBuff,0,ENCRYPT_BLOCK_SIZE);
	while(TRUE)
	{
		if (*len - nInOffset >= ENCRYPT_BLOCK_SIZE)
		{
			sd_memcpy(inBuff,buffer+nInOffset,ENCRYPT_BLOCK_SIZE);
			aes_cipher(&aes, inBuff, ouBuff);
			sd_memcpy(pOutBuff+nOutOffset,ouBuff,ENCRYPT_BLOCK_SIZE);
			nInOffset += ENCRYPT_BLOCK_SIZE;
			nOutOffset += ENCRYPT_BLOCK_SIZE;
		}
		else
		{
			int nDataLen = *len - nInOffset;
			int nFillData = ENCRYPT_BLOCK_SIZE - nDataLen;
			sd_memset(inBuff,nFillData,ENCRYPT_BLOCK_SIZE);
			sd_memset(ouBuff,0,ENCRYPT_BLOCK_SIZE);
			if (nDataLen > 0)
			{
				sd_memcpy(inBuff,buffer+nInOffset,nDataLen);
				aes_cipher(&aes, inBuff, ouBuff);
				sd_memcpy(pOutBuff+nOutOffset,ouBuff,ENCRYPT_BLOCK_SIZE);
				nInOffset += nDataLen;
				nOutOffset += ENCRYPT_BLOCK_SIZE;
			}
			else
			{
				aes_cipher(&aes, inBuff, ouBuff);
				sd_memcpy(pOutBuff+nOutOffset,ouBuff,ENCRYPT_BLOCK_SIZE);
				nOutOffset += ENCRYPT_BLOCK_SIZE;
			}
			break;
		}
	}
	nOutLen = nOutOffset;
	sd_memcpy(buffer + nBeginOffset,pOutBuff, nOutLen);
	tmp_buffer = buffer + sizeof(_u32) * 2;
    sd_set_int32_to_lt(&tmp_buffer, &tmplen, nOutLen);     
	sd_free(pOutBuff);
	pOutBuff = NULL;
	if(nOutLen + nBeginOffset > *len + 16)
		return -1;
	*len = nOutLen + nBeginOffset;
	return SUCCESS;
}

_int32 build_query_server_res_cmd(HUB_DEVICE* device,char** buffer, _u32* len, QUERY_SERVER_RES_CMD* cmd)
{
	_int32 ret;
	char* tmp_buf;
	_int32 tmp_len;
	char http_header[1024] = {0};
	_u32 http_header_len = 1024;
	_u32 encode_len = 0;
	cmd->_header._version = SHUB_PROTOCOL_VER;
	cmd->_header._seq = g_shub_seq++; 
	cmd->_header._cmd_len = 60 + cmd->_url_or_gcid_size + cmd->_cid_size + cmd->_origin_url_size + cmd->_peerid_size + cmd->_refer_url_size + cmd->_partner_id_size;
	*len = HUB_CMD_HEADER_LEN + cmd->_header._cmd_len;
	encode_len = (cmd->_header._cmd_len + HUB_ENCODE_PADDING_LEN) /HUB_ENCODE_PADDING_LEN * HUB_ENCODE_PADDING_LEN + HUB_CMD_HEADER_LEN;
	ret = res_query_build_http_header(http_header, &http_header_len, encode_len,device->_hub_type, device->_host, device->_port);
	CHECK_VALUE(ret);
	cmd->_cmd_type = QUERY_SERVER_RES;
	ret = sd_malloc(*len + HUB_ENCODE_PADDING_LEN + http_header_len, (void**)buffer);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("build_query_server_res_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
    sd_memset(*buffer, 0, *len + HUB_ENCODE_PADDING_LEN + http_header_len);
	sd_memcpy(*buffer, http_header, http_header_len);
	tmp_buf = *buffer + http_header_len;
	tmp_len = (_int32)*len;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._seq);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._cmd_len);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_client_version);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_compress);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_cmd_type);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_by_what);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_url_or_gcid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_url_or_gcid, (_int32)cmd->_url_or_gcid_size);
	if(cmd->_by_what & QUERY_BY_CID)
	{
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cid_size);
		sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_cid, (_int32)cmd->_cid_size);
	}
	else
	{
		_u32 len = 0;
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)len);
	}
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_origin_url_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_origin_url, cmd->_origin_url_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_max_server_res);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_bonus_res_num);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, (_int32)cmd->_peerid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char* )&cmd->_peer_report_ip, sizeof(_u32));
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_peer_capability);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_query_times_sn);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_cid_query_type);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_refer_url_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_refer_url, (_int32)cmd->_refer_url_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_partner_id_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_partner_id, cmd->_partner_id_size);
	ret = sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_product_flag);
    sd_assert(ret==SUCCESS);
	ret = xl_aes_encrypt(*buffer + http_header_len, len);
	if(ret != SUCCESS)
	{
		LOG_ERROR("build_query_server_res_cmd, but aes_encrypt failed, errcode = %d.", ret);
		sd_free(*buffer);
		*buffer = NULL;
		sd_assert(FALSE);
		return ret;
	}
	LOG_DEBUG("build_query_server_res_cmd, tmp_len=%u", tmp_len);
	sd_assert(tmp_len == 0);
	*len += http_header_len;
	return SUCCESS;
}

_int32 build_query_info_cmd(HUB_DEVICE* device,char** buffer, _u32* len, QUERY_RES_INFO_CMD* cmd)
{
	_int32 ret;
	char* tmp_buf;
	_int32 tmp_len;
	char http_header[1024] = {0};
	_u32 http_header_len = 1024;
	_u32 encode_len = 0;
	cmd->_header._version = NEW_SHUB_PROTOCOL_VER;
	cmd->_header._seq = g_shub_seq++; 

	if((cmd->_by_what & 0x01) != 0)
	{
		cmd->_header._cmd_len = 66 + cmd->_reserver_length + cmd->_cid_size + cmd->_cid_assist_url_size + cmd->_cid_origion_url_size + cmd->_cid_ref_url_size + cmd->_peerid_length ;
	}
	else
	{
		cmd->_header._cmd_len = 53 + cmd->_reserver_length +  cmd->_query_url_size + cmd->_origion_url_size + cmd->_ref_url_size + cmd->_peerid_length ;
	}
	
	*len = HUB_CMD_HEADER_LEN + cmd->_header._cmd_len;
	encode_len = (cmd->_header._cmd_len + HUB_ENCODE_PADDING_LEN) /HUB_ENCODE_PADDING_LEN * HUB_ENCODE_PADDING_LEN + HUB_CMD_HEADER_LEN;
	ret = res_query_build_http_header(http_header, &http_header_len, encode_len,device->_hub_type, device->_host, device->_port);
	CHECK_VALUE(ret);
	cmd->_cmd_type = QUERY_RES_INFO_CMD_ID;
	ret = sd_malloc(*len + HUB_ENCODE_PADDING_LEN + http_header_len, (void**)buffer);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("build_query_server_res_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
       sd_memset(*buffer, 0, *len + HUB_ENCODE_PADDING_LEN + http_header_len);
	sd_memcpy(*buffer, http_header, http_header_len);
	tmp_buf = *buffer + http_header_len;
	tmp_len = (_int32)*len;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._seq);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._cmd_len);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_client_version);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_compress);

	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_reserver_buffer,(_int32)cmd->_reserver_length);
	
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_cmd_type);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_by_what);


	if(cmd->_by_what != 0 )
	{
		_int32 struct_length = 4 + cmd->_cid_size + 8
			+ 4 + cmd->_cid_assist_url_size + 4
			+ 4 + cmd->_cid_origion_url_size + 4
			+ 4 + cmd->_cid_ref_url_size + 4
			+ 1;
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)struct_length);	
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cid_size);
		sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_cid, (_int32)cmd->_cid_size);

		sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_size);

		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cid_assist_url_size);
		sd_set_bytes(&tmp_buf, &tmp_len, cmd->_cid_assist_url, (_int32)cmd->_cid_assist_url_size);
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cid_assist_url_code_page);

		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cid_origion_url_size);
		sd_set_bytes(&tmp_buf, &tmp_len, cmd->_cid_origion_url, (_int32)cmd->_cid_origion_url_size);
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cid_origion_url_code_page);

		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cid_ref_url_size);
		sd_set_bytes(&tmp_buf, &tmp_len, cmd->_cid_ref_url, (_int32)cmd->_cid_ref_url_size);
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cid_ref_url_code_page);

		sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_cid_query_type);
	}
	else
	{
		_int32 struct_length = 4 + cmd->_query_url_size + 4
			+ 4 + cmd->_origion_url_size + 4
			+ 4 + cmd->_ref_url_size + 4;
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)struct_length);	
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_query_url_size);
		sd_set_bytes(&tmp_buf, &tmp_len, cmd->_query_url, (_int32)cmd->_query_url_size);
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_query_url_code_page);

		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_origion_url_size);
		sd_set_bytes(&tmp_buf, &tmp_len, cmd->_origion_url, (_int32)cmd->_origion_url_size);
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_origion_url_code_page);


		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_ref_url_size);
		sd_set_bytes(&tmp_buf, &tmp_len, cmd->_ref_url, (_int32)cmd->_ref_url_size);
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_ref_url_code_page);
	}


	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_length);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, (_int32)cmd->_peerid_length);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_local_ip);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_query_sn);
	ret = sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_bcid_range);
	

	dump_buffer( *buffer + http_header_len, *len );

       sd_assert(ret==SUCCESS);
	ret = xl_aes_encrypt(*buffer + http_header_len, len);
	if(ret != SUCCESS)
	{
		LOG_ERROR("build_query_server_res_cmd, but aes_encrypt failed, errcode = %d.", ret);
		sd_free(*buffer);
		*buffer = NULL;
		sd_assert(FALSE);
		return ret;
	}
	LOG_DEBUG("build_query_server_res_cmd, tmp_len=%u", tmp_len);
	sd_assert(tmp_len == 0);
	*len += http_header_len;
	return SUCCESS;
}

_int32 build_query_newserver_res_cmd(HUB_DEVICE* device,char** buffer, _u32* len, NEWQUERY_SERVER_RES_CMD* cmd)
{
	_int32 ret;
	char* tmp_buf;
	_int32 tmp_len;
	char http_header[1024] = {0};
	_u32 http_header_len = 1024;
	_u32 encode_len = 0;
	cmd->_header._version = NEW_SHUB_PROTOCOL_VER;
	cmd->_header._seq = g_shub_seq++; 
	cmd->_header._cmd_len = 74 + cmd->_reserver_length + cmd->_cid_size + cmd->_gcid_size + cmd->_assist_url_size 
			+ cmd->_origion_url_size + cmd->_ref_url_size + cmd->_peerid_length + cmd->_file_suffix_size;
	*len = HUB_CMD_HEADER_LEN + cmd->_header._cmd_len;
	encode_len = (cmd->_header._cmd_len + HUB_ENCODE_PADDING_LEN) /HUB_ENCODE_PADDING_LEN * HUB_ENCODE_PADDING_LEN + HUB_CMD_HEADER_LEN;
	ret = res_query_build_http_header(http_header, &http_header_len, encode_len,device->_hub_type, device->_host, device->_port);
	CHECK_VALUE(ret);
	cmd->_cmd_type = QUERY_NEW_RES_CMD_ID;
	ret = sd_malloc(*len + HUB_ENCODE_PADDING_LEN + http_header_len, (void**)buffer);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("build_query_newserver_res_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	  
    sd_memset(*buffer, 0, *len + HUB_ENCODE_PADDING_LEN + http_header_len);
	sd_memcpy(*buffer, http_header, http_header_len);
	tmp_buf = *buffer + http_header_len;
	tmp_len = (_int32)*len;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._seq);
      sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._cmd_len);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_client_version);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_compress);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_reserver_buffer,cmd->_reserver_length);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_cmd_type);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_cid, (_int32)cmd->_cid_size);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_gcid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_gcid, (_int32)cmd->_gcid_size);	

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_gcid_level);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_assist_url_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_assist_url, (_int32)cmd->_assist_url_size);		
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_assist_url_code_page);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_origion_url_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_origion_url, (_int32)cmd->_origion_url_size);		
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_origion_url_code_page);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_ref_url_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_ref_url, (_int32)cmd->_ref_url_size);		
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_ref_url_code_page);	
	
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_cid_query_type);

	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_max_server_res);

	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_bonus_res_num);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_length);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, (_int32)cmd->_peerid_length);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_local_ip);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_query_sn);


	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_suffix_size);
	ret = sd_set_bytes(&tmp_buf, &tmp_len, cmd->_file_suffix, (_int32)cmd->_file_suffix_size);

	dump_buffer(*buffer, cmd->_header._cmd_len);
      
	ret = xl_aes_encrypt(*buffer + http_header_len, len);
	if(ret != SUCCESS)
	{
		LOG_ERROR("build_query_newserver_res_cmd, but aes_encrypt failed, errcode = %d.", ret);
		sd_free(*buffer);
		*buffer = NULL;
		sd_assert(FALSE);
		return ret;
	}
	LOG_DEBUG("build_query_newserver_res_cmd, tmp_len=%u", tmp_len);
	sd_assert(tmp_len == 0);
	*len += http_header_len;
	return SUCCESS;
}
_int32 build_relation_query_server_res_cmd(HUB_DEVICE* device,char** buffer, _u32* len, QUERY_FILE_RELATION_RES_CMD* cmd)
{
	_int32 ret;
	char* tmp_buf;
	_int32 tmp_len;
	char http_header[1024] = {0};
	_u32 http_header_len = 1024;
	_u32 encode_len = 0;
	cmd->_header._version = 50;
	cmd->_header._seq = g_shub_seq++; 
	cmd->_header._cmd_len = 73 +cmd->_cid_size + cmd->_gcid_size + cmd->_assist_url_length + cmd->_original_url_length 
		+ cmd->_ref_url_length + cmd->_peerid_length + cmd->_file_suffix_length;

	*len = HUB_CMD_HEADER_LEN + cmd->_header._cmd_len;
	encode_len = (cmd->_header._cmd_len + HUB_ENCODE_PADDING_LEN) /HUB_ENCODE_PADDING_LEN * HUB_ENCODE_PADDING_LEN + HUB_CMD_HEADER_LEN;
	ret = res_query_build_http_header(http_header, &http_header_len, encode_len,device->_hub_type, device->_host, device->_port);
	CHECK_VALUE(ret);
	cmd->_cmd_type = QUERY_FILE_RELATION;
	ret = sd_malloc(*len + HUB_ENCODE_PADDING_LEN + http_header_len, (void**)buffer);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("build_relation_query_server_res_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
       sd_memset(*buffer, 0, *len + HUB_ENCODE_PADDING_LEN + http_header_len);
	sd_memcpy(*buffer, http_header, http_header_len);
	tmp_buf = *buffer + http_header_len;
	tmp_len = (_int32)*len;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._seq);
       sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._cmd_len);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_client_version);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_compress);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_cmd_type);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->_cid_size);
	sd_set_bytes(&tmp_buf, &tmp_len,(char*)cmd->_cid, cmd->_cid_size);

	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_size);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->_gcid_size);
	sd_set_bytes(&tmp_buf, &tmp_len,(char*)cmd->_gcid, cmd->_gcid_size);	
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->_gcid_level);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->_assist_url_length);
	sd_set_bytes(&tmp_buf, &tmp_len,cmd->_assist_url, cmd->_assist_url_length);	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->_assist_url_code_page);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->_original_url_length);
	sd_set_bytes(&tmp_buf, &tmp_len,cmd->_original_url, cmd->_original_url_length);	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->_original_url_code_page);


	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->_ref_url_length);
	sd_set_bytes(&tmp_buf, &tmp_len,cmd->_ref_url, cmd->_ref_url_length);	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->_ref_url_code_page);

	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_cid_query_type);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->_max_relation_resource_num);
	
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->_peerid_length);
	sd_set_bytes(&tmp_buf, &tmp_len,cmd->_peerid, cmd->_peerid_length);
	sd_set_bytes(&tmp_buf, &tmp_len, (char* )&cmd->_local_ip, sizeof(_u32));
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->_query_sn);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->_file_suffix_length);
	sd_set_bytes(&tmp_buf, &tmp_len,cmd->_file_suffilx, cmd->_file_suffix_length);	
	
	ret = xl_aes_encrypt(*buffer + http_header_len, len);
	if(ret != SUCCESS)
	{
		LOG_ERROR("build_relation_query_server_res_cmd, but aes_encrypt failed, errcode = %d.", ret);
		sd_free(*buffer);
		*buffer = NULL;
		sd_assert(FALSE);
		return ret;
	}

	LOG_DEBUG("build_relation_query_server_res_cmd, tmp_len=%u", tmp_len);
	sd_assert(tmp_len == 0);
	*len += http_header_len;
	return SUCCESS;
}
_int32 build_query_peer_res_cmd(HUB_DEVICE* device,char** buffer, _u32* len, QUERY_PEER_RES_CMD* cmd)
{
	static _u32 seq = 0;
	_int32 ret;
	char* tmp_buf;
	_int32  tmp_len;
	char	http_header[1024] = {0};
	_u32 http_header_len = 1024;
	_u32 encode_len = 0;
	cmd->_protocol_version = PHUB_PROTOCOL_VER;
	cmd->_seq = seq++;
	cmd->_cmd_len = 62 + cmd->_peerid_size + cmd->_cid_size + cmd->_gcid_size + cmd->_partner_id_size;
	cmd->_cmd_type = QUERY_PEER_RES;
	*len = HUB_CMD_HEADER_LEN + cmd->_cmd_len;
	encode_len = (cmd->_cmd_len + HUB_ENCODE_PADDING_LEN) /HUB_ENCODE_PADDING_LEN * HUB_ENCODE_PADDING_LEN + HUB_CMD_HEADER_LEN;
	LOG_DEBUG("build_query_peer_res_cmd,hub_type=%d",device->_hub_type);
	ret = res_query_build_http_header(http_header, &http_header_len, encode_len,device->_hub_type, device->_host, device->_port);
	CHECK_VALUE(ret);
	ret = sd_malloc(*len + HUB_ENCODE_PADDING_LEN + http_header_len, (void**)buffer);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("build_query_server_res_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	sd_memcpy(*buffer, http_header, http_header_len);
	tmp_buf = *buffer + http_header_len;
	tmp_len = (_int32)*len;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_protocol_version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cmd_len);   
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_cmd_type);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, (_int32)cmd->_peerid_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_cid, (_int32)cmd->_cid_size);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_gcid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_gcid, (_int32)cmd->_gcid_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peer_capability);
	sd_set_bytes(&tmp_buf, &tmp_len, (char* )&cmd->_internal_ip, sizeof(_u32));
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_nat_type);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_level_res_num);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_resource_capability);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->_server_res_num);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_query_times);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_p2p_capability);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)&cmd->_upnp_ip, sizeof(_u32));
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_upnp_port);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_rsc_type);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_partner_id_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_partner_id, cmd->_partner_id_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_product_flag);
#ifdef 	RES_QUERY_DEBUG
	{
		char test_buffer[1024];
		sd_memset(test_buffer,0,1024);
		str2hex( *buffer + http_header_len, tmp_buf-(*buffer)-http_header_len, test_buffer, 1024);
		LOG_DEBUG( "build_query_peer_res_cmd:buffer[%u]=%s" ,tmp_buf-(*buffer), *buffer);
		printf("\n build_query_peer_res_cmd:buffer[%u]=%s\n" ,tmp_buf-(*buffer), *buffer);
		LOG_DEBUG( "build_query_peer_res_cmd:buffer in hex[%u]=%s" ,tmp_buf-(*buffer)-http_header_len, test_buffer);
		printf("\n build_query_peer_res_cmd:buffer in hex[%u]=%s\n" ,tmp_buf-(*buffer)-http_header_len, test_buffer);
	}
#endif 
	ret = xl_aes_encrypt(*buffer + http_header_len, len);
	if(ret != SUCCESS)
	{
		LOG_ERROR("build_query_peer_res_cmd, but aes_encrypt failed. errcode = %d.", ret);
		sd_free(*buffer);
		*buffer = NULL;
		sd_assert(FALSE);
		return ret;
	}
	sd_assert(tmp_len == 0);
	*len += http_header_len;
	return SUCCESS;
}

_int32 build_query_tracker_res_cmd(HUB_DEVICE* device,char** buffer, _u32* len, QUERY_TRACKER_RES_CMD* cmd)
{
	static _u32 seq = 0;
	_int32 ret;
	char* tmp_buf;
	_int32  tmp_len;
	char http_header[1024] = {0};
	_u32 http_header_len = 1024;
	_u32 encode_len = 0;
	cmd->_protocol_version = TRACKER_PROTOCOL_VER;
	cmd->_seq = seq++;
	cmd->_cmd_len = 71 + cmd->_gcid_size + cmd->_peerid_size + cmd->_partner_id_size;
	cmd->_cmd_type = QUERY_TRACKER_RES;
	encode_len = (cmd->_cmd_len + HUB_ENCODE_PADDING_LEN) /HUB_ENCODE_PADDING_LEN * HUB_ENCODE_PADDING_LEN + HUB_CMD_HEADER_LEN;
	ret = res_query_build_http_header(http_header, &http_header_len, encode_len,device->_hub_type, device->_host, device->_port);
	CHECK_VALUE(ret);
	*len = HUB_CMD_HEADER_LEN + cmd->_cmd_len;
	ret = sd_malloc(*len + HUB_ENCODE_PADDING_LEN + http_header_len, (void**)buffer);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("build_query_tracker_res_cmd, malloc failed.");
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
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_by_what);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_gcid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_gcid, (_int32)cmd->_gcid_size);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, (_int32)cmd->_peerid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char* )&cmd->_local_ip, sizeof(_u32));
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_tcp_port);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_max_res);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_nat_type);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_download_ratio);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_download_map);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peer_capability);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_release_id);
	sd_set_bytes(&tmp_buf, &tmp_len, (char* )&cmd->_upnp_ip, sizeof(_u32));
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_upnp_port);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_p2p_capability);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_udp_port);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_rsc_type);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_partner_id_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_partner_id, (_int32)cmd->_partner_id_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_product_flag);

	ret = xl_aes_encrypt(*buffer + http_header_len, len);
	if(ret != SUCCESS)
	{
		LOG_ERROR("build_query_tracker_res_cmd, but aes_encrypt failed, errcode = %d.", ret);
		sd_free(*buffer);
		*buffer = NULL;
		sd_assert(FALSE);
		return ret;
	}
	sd_assert(tmp_len == 0);
	*len += http_header_len;
	return SUCCESS;
}

_int32 build_query_bt_info_cmd(HUB_DEVICE* device, char** buffer, _u32* len, QUERY_BT_INFO_CMD* cmd)
{
	static _u32 seq = 0;
	_int32 ret;
	char*  tmp_buf;
	_int32 tmp_len;
	char http_header[1024] = {0};
	_u32 http_header_len = 1024;
	_u32 encode_len = 0;
	cmd->_header._version = SHUB_PROTOCOL_VER;
	cmd->_header._seq = seq++;
	cmd->_header._cmd_len = 61;
	cmd->_cmd_type = QUERY_BT_INFO;
	encode_len = (cmd->_header._cmd_len + HUB_ENCODE_PADDING_LEN) /HUB_ENCODE_PADDING_LEN * HUB_ENCODE_PADDING_LEN + HUB_CMD_HEADER_LEN;
	ret = res_query_build_http_header(http_header, &http_header_len, encode_len,device->_hub_type, device->_host, device->_port);
	CHECK_VALUE(ret);
	*len = HUB_CMD_HEADER_LEN + cmd->_header._cmd_len;
	ret = sd_malloc(*len + HUB_ENCODE_PADDING_LEN + http_header_len, (void**)buffer);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("build_query_server_res_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	sd_memcpy(*buffer, http_header, http_header_len);
	tmp_buf = *buffer + http_header_len;
	tmp_len = (_int32)*len;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._cmd_len);   
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_client_version);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_compress);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_cmd_type);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, cmd->_peerid_len);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_info_id_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_info_id, cmd->_info_id_len);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_index);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_query_times);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_need_bcid);
	ret = xl_aes_encrypt(*buffer + http_header_len, len);
	if(ret != SUCCESS)
	{
		LOG_ERROR("build_query_bt_info_cmd, but aes_encrypt failed, errcode = %d.", ret);
		sd_free(*buffer);
		*buffer = NULL;
		sd_assert(FALSE);
		return ret;
	}
	sd_assert(tmp_len == 0);
	*len += http_header_len;
	return SUCCESS;
}

#ifdef ENABLE_CDN
_int32 build_query_cdn_manager_info_cmd(HUB_DEVICE* device,char** buffer, _u32* len, CDNMANAGERQUERY* cmd)
{
	static _u32 seq = 0;
	_int32 ret;
	char _Tbuffer[MAX_URL_LEN];
	char   str_cid[CID_SIZE*2+1];
#if defined( MOBILE_PHONE)
	char http_get_header[] = "GET /getCdnresource_common?ver=2000&gcid=";
#else
	char http_get_header[] = "GET /getCdnresource_common?ver=2000&gcid=%s HTTP/1.1\r\n"
		"Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/x-shockwave-flash, application/vnd.ms-excel, application/vnd.ms-powerpoint, application/msword, */*\r\n"
		"Accept-Language: zh-cn\r\n"
		"Accept-Encoding: gzip, deflate\r\n"
		"User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; .NET CLR 1.1.4322; .NET CLR 2.0.50727; InfoPath.2)\r\n"
		"Host: %s\r\n"
		"\r\n\r\n";
#endif

	LOG_DEBUG("build_query_cdn_manager_info_cmd,  ... ");
	cmd->_header._version = SHUB_PROTOCOL_VER;
	cmd->_header._seq = seq++;
	sd_memset(str_cid, 0, sizeof(str_cid));
       str2hex((char*)cmd->_gcid, (_int32)CID_SIZE, (char*)str_cid, CID_SIZE*2);
	
	sd_memset((void*)_Tbuffer, 0, sizeof(_Tbuffer));
#if defined(MOBILE_PHONE)
	if(NT_GPRS_WAP & sd_get_net_type())
	{
       	sd_snprintf(_Tbuffer,MAX_URL_LEN,"%s%s HTTP/1.1\r\nX-Online-Host: %s:%u\r\nConnection: Close\r\nAccept: */*\r\n\r\n",http_get_header, str_cid, get_res_query_setting()->_cdn_manager_addr, get_res_query_setting()->_cdn_manager_port);
	}
	else
	{
       	sd_snprintf(_Tbuffer,MAX_URL_LEN,"%s%s HTTP/1.1\r\nHost: %s:%u\r\nUser-Agent: Mozilla/4.0\r\nConnection: Keep-Alive\r\nAccept: */*\r\n\r\n",http_get_header, str_cid,  get_res_query_setting()->_cdn_manager_addr, get_res_query_setting()->_cdn_manager_port);
	}
#else
       sd_snprintf(_Tbuffer,MAX_URL_LEN,http_get_header, str_cid, get_res_query_setting()->_cdn_manager_addr);
#endif
	*len = sd_strlen(_Tbuffer);
	ret = sd_malloc((*len+1) , (void**)buffer);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("build_query_cdn_manager_info_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	sd_memset(*buffer, 0 , *len+1);
	sd_memcpy(*buffer, _Tbuffer, *len);
	cmd->_header._cmd_len = *len;
	LOG_DEBUG("build_query_cdn_manager_info_cmd cmd:[].");
	
	return SUCCESS;
}

#ifdef ENABLE_KANKAN_CDN
_int32 build_query_kankan_cdn_manager_info_cmd(HUB_DEVICE* device,char** buffer, _u32* len, CDNMANAGERQUERY* cmd)
{
    
	static _u32 seq = 0;
	_int32 ret;
	char _Tbuffer[MAX_URL_LEN];
	char   str_cid[CID_SIZE*2+1];
	
	char http_get_header[] = "GET /getCdnresource_flv?&gcid=";

	LOG_DEBUG("build_query_kankan_cdn_manager_info_cmd,  ... ");
	cmd->_header._version = SHUB_PROTOCOL_VER;
	cmd->_header._seq = seq++;

	sd_memset(str_cid, 0, sizeof(str_cid));
       str2hex((char*)cmd->_gcid, (_int32)CID_SIZE, (char*)str_cid, CID_SIZE*2);
	
	sd_memset((void*)_Tbuffer, 0, sizeof(_Tbuffer));

	if(NT_GPRS_WAP & sd_get_net_type())
	{
       	sd_snprintf(_Tbuffer,MAX_URL_LEN,"%s%s HTTP/1.1\r\nX-Online-Host: %s:%u\r\nConnection: Close\r\nAccept: */*\r\n\r\n",http_get_header, str_cid, get_res_query_setting()->_kankan_cdn_manager_addr, get_res_query_setting()->_kankan_cdn_manager_port);
	}
	else
	{
       	sd_snprintf(_Tbuffer,MAX_URL_LEN,"%s%s HTTP/1.1\r\nHost: %s:%u\r\nUser-Agent: Mozilla/4.0\r\nConnection: Keep-Alive\r\nAccept: */*\r\n\r\n",http_get_header, str_cid,  get_res_query_setting()->_kankan_cdn_manager_addr, get_res_query_setting()->_kankan_cdn_manager_port);
	}

	*len = sd_strlen(_Tbuffer);
	ret = sd_malloc((*len+1) , (void**)buffer);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("build_query_cdn_manager_info_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	sd_memset(*buffer, 0 , *len+1);
	sd_memcpy(*buffer, _Tbuffer, *len);
	cmd->_header._cmd_len = *len;
	LOG_DEBUG("build_query_cdn_manager_info_cmd cmd:[].");
	
	return SUCCESS;
}
#endif

#endif /* ENABLE_CDN */

_int32 build_enrollsp1_cmd(HUB_DEVICE* device,char** buffer, _u32* len, ENROLLSP1_CMD* cmd)
{
	_int32 ret = SUCCESS;
	char http_header[1024] = {0};
	_u32 http_header_len = 1024;
	char*  tmp_buf;
	_int32 tmp_len;
	_u32 encode_len = 0;
	cmd->_header._version = SHUB_PROTOCOL_VER;
	cmd->_header._seq = g_shub_seq++;
	cmd->_header._cmd_len = 80 + cmd->_internal_ip_len + cmd->_thunder_version_len + cmd->_partner_id_len;
	cmd->_cmd_type = ENROLLSP1;
	*len = HUB_CMD_HEADER_LEN + cmd->_header._cmd_len;
	encode_len = (cmd->_header._cmd_len + HUB_ENCODE_PADDING_LEN) /HUB_ENCODE_PADDING_LEN * HUB_ENCODE_PADDING_LEN + HUB_CMD_HEADER_LEN;
	ret = res_query_build_http_header(http_header, &http_header_len, encode_len,device->_hub_type, device->_host, device->_port);
	CHECK_VALUE(ret);
	ret = sd_malloc(*len + HUB_ENCODE_PADDING_LEN + http_header_len, (void**)buffer);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("build_enrollsp1_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
    LOG_DEBUG("build_enrollsp1_cmd.");
    
	sd_memcpy(*buffer, http_header, http_header_len);
	tmp_buf = *buffer + http_header_len;
	tmp_len = (_int32)*len;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._cmd_len);   
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_client_version);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_compress);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_cmd_type);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_local_peerid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_local_peerid, cmd->_local_peerid_len);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_internal_ip_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_internal_ip, cmd->_internal_ip_len);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_product_flag);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_thunder_version_len);
	sd_set_bytes(&tmp_buf, &tmp_len, &cmd->_thunder_version, cmd->_thunder_version_len);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_network_type);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_os_type);
	sd_assert(cmd->_os_details_len == 0);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_os_details_len);
	sd_assert(cmd->_user_details_len == 0);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_user_details_len);
	sd_assert(cmd->_plugin_size == 0);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_plugin_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_enable_login_control);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_filter_peerid_version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_user_agent_version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_partner_id_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_partner_id, cmd->_partner_id_len);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_head_suffix_map_version);

	ret = xl_aes_encrypt(*buffer + http_header_len, len);
	if(ret != SUCCESS)
	{
		LOG_ERROR("build_enrollsp1_cmd, but aes_encrypt failed, errcode = %d.", ret);
		sd_free(*buffer);
		*buffer = NULL;
		sd_assert(FALSE);
		return ret;
	}
	sd_assert(tmp_len == 0);
	*len += http_header_len;
	return SUCCESS;
}

_int32 build_query_config_cmd(HUB_DEVICE* device, char** buffer, _u32* len, QUERY_CONFIG_CMD* cmd)
{
	cmd->_header._version = PHUB_PROTOCOL_VER;
	cmd->_header._seq = g_shub_seq++;
	cmd->_header._cmd_len = 41 + cmd->_local_peerid_len + cmd->_os_details_len + 
	cmd->_ui_version_len + cmd->_thunder_version_len + cmd->_config_section_filter_len;
	cmd->_cmd_type = QUERY_CONFIG;
	*len = HUB_CMD_HEADER_LEN + cmd->_header._cmd_len;
	_u32 encode_len = (cmd->_header._cmd_len + HUB_ENCODE_PADDING_LEN) /HUB_ENCODE_PADDING_LEN * HUB_ENCODE_PADDING_LEN + HUB_CMD_HEADER_LEN;
	char http_header[1024] = {0};
	_u32 http_header_len = 1024;
	_int32 ret = res_query_build_http_header(http_header, &http_header_len, encode_len, device->_hub_type, device->_host, device->_port);
	CHECK_VALUE(ret);
	ret = sd_malloc(*len + HUB_ENCODE_PADDING_LEN + http_header_len, (void**)buffer);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("build_query_config_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	
    LOG_DEBUG("build_query_config_cmd. cmd_type = %d, peerid = %s, peer_capability = %d\
        , product_flag = %d, os_type = %d, os_detail = %s, ui_version = %s, thunder_version = %s\
        , network_type = %d", cmd->_cmd_type
        , cmd->_local_peerid
        , cmd->_peer_capability
        , cmd->_product_flag
        , cmd->_os_type
        , cmd->_os_details
        , cmd->_ui_version
        , cmd->_thunder_version
        , cmd->_network_type);
    
	sd_memcpy(*buffer, http_header, http_header_len);
	char *tmp_buf = *buffer + http_header_len;
	_int32 tmp_len = (_int32)*len;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._seq);
   	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._cmd_len);   
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_cmd_type);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_local_peerid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_local_peerid, cmd->_local_peerid_len);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_peer_capability);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_product_flag);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_os_type);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_os_details_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_os_details, cmd->_os_details_len);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_ui_version_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_ui_version, cmd->_ui_version_len);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_thunder_version_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_thunder_version, cmd->_thunder_version_len);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_network_type);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_enable_bt);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_enable_emule);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_storage_type);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_timestamp);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_config_section_filter_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_config_section_filter, cmd->_config_section_filter_len);

	ret = xl_aes_encrypt(*buffer + http_header_len, len);
	if(ret != SUCCESS)
	{
		LOG_ERROR("build_query_config_cmd, but aes_encrypt failed, errcode = %d.", ret);
		sd_free(*buffer);
		*buffer = NULL;
		sd_assert(FALSE);
		return ret;
	}
	sd_assert(tmp_len == 0);
	*len += http_header_len;
	return SUCCESS;
}


_int32 build_reservce_60ver(char** buffer, _u32* len )
{
	static char s_reserver_buffer[MAX_VERSION_LEN * 3 + 16] = {0};
	static _u32 s_reserver_data_length = 0;
	char  ui_version[MAX_VERSION_LEN] = {0};
	char  et_version[MAX_VERSION_LEN] = {0};
	char  partner_id[64] = {0};

	if(s_reserver_data_length > 0)
	{
		*buffer = s_reserver_buffer;
		*len      = s_reserver_data_length + 4;
		return SUCCESS;
	}

	char* tmp_buf = s_reserver_buffer;
	_int32 tmp_len = MAX_VERSION_LEN * 3 + 16;
#ifdef MOBILE_PHONE
	get_version(ui_version, MAX_VERSION_LEN);
#endif
	get_version(et_version, MAX_VERSION_LEN);
	get_partner_id(partner_id, 64);
	_u32 product_id = get_product_id();

	_int32 ui_length = sd_strlen(ui_version) ;

	_int32 et_vertion_length = sd_strlen(et_version);

	_int32 partner_id_length =  sd_strlen(partner_id);
	
	 s_reserver_data_length = 16 + ui_length + et_vertion_length +partner_id_length;
	

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, s_reserver_data_length);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, ui_length);
	sd_set_bytes(&tmp_buf, &tmp_len, ui_version, ui_length);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, product_id);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, partner_id_length);
	sd_set_bytes(&tmp_buf, &tmp_len, partner_id, partner_id_length);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, et_vertion_length);
	sd_set_bytes(&tmp_buf, &tmp_len, et_version, et_vertion_length);

	*buffer = s_reserver_buffer;
	*len      = s_reserver_data_length + 4;
	return SUCCESS;
}
_int32 res_query_build_http_header(char* buffer, _u32* len, _u32 data_len,HUB_TYPE hub_type, const char *device_addr, _u16 device_port)
{
	char * hub_addr=NULL;
	_u32 hub_port=0;
	RES_QUERY_SETTING* setting = get_res_query_setting();
	
	switch(hub_type)
	{
	case SHUB:
		{
			hub_addr = setting->_shub_addr;
			hub_port = setting->_shub_port;
			break;
		}
	case PHUB:
		{
			hub_addr = setting->_phub_addr;
			hub_port = setting->_phub_port;
			break;
		}
	case PARTNER_CDN:
		{
			hub_addr = setting->_partner_cdn_addr;
			hub_port = setting->_partner_cdn_port;
			break;
		}
	case TRACKER:
		{
			hub_addr = setting->_tracker_addr;
			hub_port = setting->_tracker_port;
			break;
		}
	case BT_HUB:
		{
			hub_addr = setting->_bt_hub_addr;
			hub_port = setting->_bt_hub_port;
			break;
		}
#ifdef ENABLE_CDN
	case CDN_MANAGER:
		{
			hub_addr = setting->_cdn_manager_addr;
			hub_port = setting->_cdn_manager_port;
		      break;
		}
#ifdef ENABLE_KANKAN_CDN
	case KANKAN_CDN_MANAGER:
		{
			hub_addr = setting->_kankan_cdn_manager_addr;
			hub_port = setting->_kankan_cdn_manager_port;
		      break;
		}
#endif
#ifdef ENABLE_HSC
	case VIP_HUB:
		{
			hub_addr = setting->_vip_hub_addr;
			hub_port = setting->_vip_hub_port;
			break;
		}
#endif /* ENABLE_HSC */
#endif		
#ifdef ENABLE_EMULE
    case EMULE_HUB:
        {
            hub_addr = setting->_emule_hub_addr;
            hub_port = setting->_emule_hub_port;
            break;
        }
#endif
    case CONFIG_HUB:
	{
            hub_addr = setting->_config_hub_addr;
            hub_port = setting->_config_hub_port;
            break;		
    	}
	case DPHUB_ROOT:
        {
            hub_addr = setting->_dphub_root_addr;
            hub_port = setting->_dphub_root_port;
            break;
        }
    case DPHUB_NODE:
        {
            hub_addr = (char*)device_addr;
            hub_port = device_port;
            break;
        }
	case EMULE_TRACKER:
	    {
	        hub_addr = setting->_emule_trakcer_addr;
	        hub_port = setting->_emule_tracker_port;
	        break;
	    }
	case NORMAL_CDN_MANAGER:
		{
			hub_addr = setting->_normal_cdn_manager_addr;
			hub_port = setting->_normal_cdn_manager_port;
			break;
		}
	default:
		{
			LOG_ERROR("build_http_header,hub_type=%d",hub_type);
			sd_assert(FALSE);
			return -1;
		}
	}
	
#if defined(MOBILE_PHONE)
	if(NT_GPRS_WAP & sd_get_net_type())
	{
	*len = sd_snprintf(buffer, *len, "POST / HTTP/1.1\r\nX-Online-Host: res.res.res.res:%u\r\nContent-Length: %u\r\nContent-Type: application/octet-stream\r\nConnection: Close\r\nAccept: */*\r\n\r\n",
				hub_port,data_len);
	}
	else
	{
		*len = sd_snprintf(buffer, *len, "POST / HTTP/1.1\r\nHost: res.res.res.res:%u\r\nContent-Length: %u\r\nContent-Type: application/octet-stream\r\nConnection: Close\r\nUser-Agent: Mozilla/4.0\r\nAccept: */*\r\n\r\n",
				hub_port,data_len);
	}
#else
	*len = sd_snprintf(buffer, *len, "POST / HTTP/1.1\r\nHost: res.res.res.res:%u\r\nContent-Length: %u\r\nContent-Type: application/octet-stream\r\nConnection: Close\r\nUser-Agent: Mozilla/4.0\r\nAccept: */*\r\n\r\n",
				hub_port, data_len);
#endif
	LOG_DEBUG("%s", buffer);
	sd_assert(*len < 1024);
	return SUCCESS;
}


///////////////////////////////////////////////////// Protocol with RSA ///////////////////////////////////////////////////////
#ifdef _RSA_RES_QUERY
_int32 build_rsa_encrypt_header(char **ppbuf, _int32 *p_buflen, _int32 pubkey_ver,
	_u8 *aes_key, _int32 data_len)
{
	_int32 ret = SUCCESS;
	char encrypted_aes[RES_QUERY_AES_CIPHER_LEN] = {0};
	_u32 encrypted_aes_len = 0;

	if (0 != res_query_rsa_pub_encrypt(16, aes_key, encrypted_aes, &encrypted_aes_len, pubkey_ver))
	{
		LOG_ERROR("build_rsa_encrypt_header, failed encrypt aes key by RSA");
		return -1;
	}
	LOG_DEBUG("build_rsa_encrypt_header, encrypted aes len=%d", encrypted_aes_len);
	if(encrypted_aes_len != RES_QUERY_AES_CIPHER_LEN)
	{
		LOG_ERROR("build_rsa_encrypt_header, aes cipher len: required len=%u, real len = %u",
			RES_QUERY_AES_CIPHER_LEN, encrypted_aes_len);
		sd_assert(FALSE);
		return -1;
	}

	ret = sd_set_int32_to_lt(ppbuf, p_buflen, (_int32)RSA_ENCRYPT_MAGIC_NUM);
	CHECK_VALUE(ret);
	ret = sd_set_int32_to_lt(ppbuf, p_buflen, pubkey_ver);
	CHECK_VALUE(ret);
	ret = sd_set_int32_to_lt(ppbuf, p_buflen, (_int32)RES_QUERY_AES_CIPHER_LEN);
	CHECK_VALUE(ret);
	ret = sd_set_bytes(ppbuf, p_buflen, encrypted_aes, (_int32)RES_QUERY_AES_CIPHER_LEN);
	CHECK_VALUE(ret);
	ret = sd_set_int32_to_lt(ppbuf, p_buflen, data_len);
	CHECK_VALUE(ret);

	return ret;
}

_int32 build_query_newserver_res_cmd_rsa(HUB_DEVICE* device,char** buffer, _u32* len, NEWQUERY_SERVER_RES_CMD* cmd, 
	_u8* aes_key, _int32 pubkey_version)
{
	_int32 ret;
	char* tmp_buf;
	_int32 tmp_len;
	char http_header[1024] = {0};
	_u32 http_header_len = 1024;
	_u32 encode_len = 0;
	cmd->_header._version = NEW_SHUB_PROTOCOL_VER;
	cmd->_header._seq = g_shub_seq++; 
	cmd->_header._cmd_len = 74 + cmd->_reserver_length + cmd->_cid_size + cmd->_gcid_size + cmd->_assist_url_size 
		+ cmd->_origion_url_size + cmd->_ref_url_size + cmd->_peerid_length + cmd->_file_suffix_size;
	*len = HUB_CMD_HEADER_LEN + cmd->_header._cmd_len;
	encode_len = (cmd->_header._cmd_len & 0xfffffff0) + HUB_ENCODE_PADDING_LEN + HUB_CMD_HEADER_LEN;
	encode_len = (encode_len & 0xfffffff0) + HUB_ENCODE_PADDING_LEN + RSA_ENCRYPT_HEADER_LEN;
	ret = res_query_build_http_header(http_header, &http_header_len, encode_len,device->_hub_type, device->_host, device->_port);
	CHECK_VALUE(ret);
	cmd->_cmd_type = QUERY_NEW_RES_CMD_ID;
	ret = sd_malloc(http_header_len + encode_len, (void**)buffer);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("malloc failed.");
		CHECK_VALUE(ret);
	}

	sd_memset(*buffer, 0, http_header_len + encode_len);
	sd_memcpy(*buffer, http_header, http_header_len);
	tmp_buf = *buffer + http_header_len + RSA_ENCRYPT_HEADER_LEN;
	tmp_len = (_int32)*len;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._cmd_len);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_client_version);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_compress);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_reserver_buffer,cmd->_reserver_length);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_cmd_type);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_cid, (_int32)cmd->_cid_size);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_gcid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_gcid, (_int32)cmd->_gcid_size);	

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_gcid_level);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_assist_url_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_assist_url, (_int32)cmd->_assist_url_size);		
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_assist_url_code_page);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_origion_url_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_origion_url, (_int32)cmd->_origion_url_size);		
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_origion_url_code_page);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_ref_url_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_ref_url, (_int32)cmd->_ref_url_size);		
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_ref_url_code_page);	

	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_cid_query_type);


	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_max_server_res);

	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_bonus_res_num);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_length);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, (_int32)cmd->_peerid_length);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_local_ip);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_query_sn);


	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_suffix_size);
	ret = sd_set_bytes(&tmp_buf, &tmp_len, cmd->_file_suffix, (_int32)cmd->_file_suffix_size);

	ret = xl_aes_encrypt(*buffer + http_header_len + RSA_ENCRYPT_HEADER_LEN, len);
	if(ret != SUCCESS)
	{
		LOG_ERROR("xl_aes_encrypt failed, errcode = %d.", ret);
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

_int32 build_query_peer_res_cmd_rsa(HUB_DEVICE* device,char** buffer, _u32* len, QUERY_PEER_RES_CMD* cmd,
	_u8* aes_key, _int32 pubkey_version)
{
	static _u32 seq = 0;
	_int32 ret;
	char* tmp_buf;
	_int32  tmp_len;
	char	http_header[1024] = {0};
	_u32 http_header_len = 1024;
	_u32 encode_len = 0;
	cmd->_protocol_version = PHUB_PROTOCOL_VER;
	cmd->_seq = seq++;
	cmd->_cmd_len = 62 + cmd->_peerid_size + cmd->_cid_size + cmd->_gcid_size + cmd->_partner_id_size;
	cmd->_cmd_type = QUERY_PEER_RES;
	*len = HUB_CMD_HEADER_LEN + cmd->_cmd_len;
	encode_len = (cmd->_cmd_len & 0xfffffff0) + HUB_ENCODE_PADDING_LEN + HUB_CMD_HEADER_LEN;
	encode_len = (encode_len & 0xfffffff0) + HUB_ENCODE_PADDING_LEN + RSA_ENCRYPT_HEADER_LEN;
	LOG_DEBUG("build_query_peer_res_cmd,hub_type=%d",device->_hub_type);
	ret = res_query_build_http_header(http_header, &http_header_len, encode_len,device->_hub_type, device->_host, device->_port);
	CHECK_VALUE(ret);
	ret = sd_malloc(http_header_len + encode_len, (void**)buffer);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("malloc failed.");
		CHECK_VALUE(ret);
	}

	sd_memset(*buffer, 0, http_header_len + encode_len);
	sd_memcpy(*buffer, http_header, http_header_len);
	tmp_buf = *buffer + http_header_len + RSA_ENCRYPT_HEADER_LEN;
	tmp_len = (_int32)*len;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_protocol_version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cmd_len);   
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_cmd_type);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, (_int32)cmd->_peerid_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_cid, (_int32)cmd->_cid_size);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_gcid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_gcid, (_int32)cmd->_gcid_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peer_capability);
	sd_set_bytes(&tmp_buf, &tmp_len, (char* )&cmd->_internal_ip, sizeof(_u32));
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_nat_type);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_level_res_num);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_resource_capability);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, cmd->_server_res_num);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_query_times);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_p2p_capability);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)&cmd->_upnp_ip, sizeof(_u32));
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_upnp_port);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_rsc_type);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_partner_id_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_partner_id, cmd->_partner_id_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_product_flag);
#ifdef _DEBUG
	{
		char *hexstr;
		ret = sd_malloc(*len, (void**)&hexstr);
		str2hex(*buffer + http_header_len + RSA_ENCRYPT_HEADER_LEN, *len, hexstr, *len);
		LOG_DEBUG("hub(%d) commit cmd(%hu): %s", device->_hub_type, cmd->_cmd_type, hexstr);
		sd_free(hexstr);
	}
#endif
	ret = xl_aes_encrypt(*buffer + http_header_len + RSA_ENCRYPT_HEADER_LEN, len);
	if(ret != SUCCESS)
	{
		LOG_ERROR("but aes_encrypt failed. errcode = %d.", ret);
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
	return SUCCESS;
}

_int32 build_query_info_cmd_rsa(HUB_DEVICE* device,char** buffer, _u32* len, QUERY_RES_INFO_CMD* cmd,
	_u8* aes_key, _int32 pubkey_version)
{
	_int32 ret;
	char* tmp_buf;
	_int32 tmp_len;
	char http_header[1024] = {0};
	_u32 http_header_len = 1024;
	_u32 encode_len = 0;
	cmd->_header._version = NEW_SHUB_PROTOCOL_VER;
	cmd->_header._seq = g_shub_seq++; 

	if((cmd->_by_what & 0x01) != 0)
	{
		cmd->_header._cmd_len = 66 + cmd->_reserver_length + cmd->_cid_size + cmd->_cid_assist_url_size + cmd->_cid_origion_url_size + cmd->_cid_ref_url_size + cmd->_peerid_length ;
	}
	else
	{
		cmd->_header._cmd_len = 53 + cmd->_reserver_length +  cmd->_query_url_size + cmd->_origion_url_size + cmd->_ref_url_size + cmd->_peerid_length ;
	}

	*len = HUB_CMD_HEADER_LEN + cmd->_header._cmd_len;
	encode_len = (cmd->_header._cmd_len & 0xfffffff0) + HUB_ENCODE_PADDING_LEN + HUB_CMD_HEADER_LEN;
	encode_len = (encode_len & 0xfffffff0) + HUB_ENCODE_PADDING_LEN + RSA_ENCRYPT_HEADER_LEN;
	ret = res_query_build_http_header(http_header, &http_header_len, encode_len,device->_hub_type, device->_host, device->_port);
	CHECK_VALUE(ret);
	cmd->_cmd_type = QUERY_RES_INFO_CMD_ID;
	ret = sd_malloc(http_header_len + encode_len, (void**)buffer);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("build_query_server_res_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	sd_memset(*buffer, 0, http_header_len + encode_len);
	sd_memcpy(*buffer, http_header, http_header_len);
	tmp_buf = *buffer + http_header_len + RSA_ENCRYPT_HEADER_LEN;
	tmp_len = (_int32)*len;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._cmd_len);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_client_version);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_compress);

	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_reserver_buffer,(_int32)cmd->_reserver_length);

	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_cmd_type);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_by_what);


	if(cmd->_by_what != 0 )
	{
		_int32 struct_length = 4 + cmd->_cid_size + 8
			+ 4 + cmd->_cid_assist_url_size + 4
			+ 4 + cmd->_cid_origion_url_size + 4
			+ 4 + cmd->_cid_ref_url_size + 4
			+ 1;
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)struct_length);	
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cid_size);
		sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_cid, (_int32)cmd->_cid_size);

		sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_size);

		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cid_assist_url_size);
		sd_set_bytes(&tmp_buf, &tmp_len, cmd->_cid_assist_url, (_int32)cmd->_cid_assist_url_size);
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cid_assist_url_code_page);

		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cid_origion_url_size);
		sd_set_bytes(&tmp_buf, &tmp_len, cmd->_cid_origion_url, (_int32)cmd->_cid_origion_url_size);
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cid_origion_url_code_page);

		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cid_ref_url_size);
		sd_set_bytes(&tmp_buf, &tmp_len, cmd->_cid_ref_url, (_int32)cmd->_cid_ref_url_size);
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cid_ref_url_code_page);

		sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_cid_query_type);
	}
	else
	{
		_int32 struct_length = 4 + cmd->_query_url_size + 4
			+ 4 + cmd->_origion_url_size + 4
			+ 4 + cmd->_ref_url_size + 4;
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)struct_length);	
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_query_url_size);
		sd_set_bytes(&tmp_buf, &tmp_len, cmd->_query_url, (_int32)cmd->_query_url_size);
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_query_url_code_page);

		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_origion_url_size);
		sd_set_bytes(&tmp_buf, &tmp_len, cmd->_origion_url, (_int32)cmd->_origion_url_size);
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_origion_url_code_page);


		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_ref_url_size);
		sd_set_bytes(&tmp_buf, &tmp_len, cmd->_ref_url, (_int32)cmd->_ref_url_size);
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_ref_url_code_page);
	}



	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_length);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, (_int32)cmd->_peerid_length);

	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_local_ip);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_query_sn);
	ret = sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_bcid_range);


	dump_buffer( *buffer + http_header_len, *len );

	sd_assert(ret==SUCCESS);
	ret = xl_aes_encrypt(*buffer + http_header_len + RSA_ENCRYPT_HEADER_LEN, len);
	if(ret != SUCCESS)
	{
		LOG_ERROR("build_query_server_res_cmd, but aes_encrypt failed, errcode = %d.", ret);
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
	return SUCCESS;
}

_int32 build_query_server_res_cmd_rsa(HUB_DEVICE* device,char** buffer, _u32* len, QUERY_SERVER_RES_CMD* cmd,
	_u8* aes_key, _int32 pubkey_version)
{
	_int32 ret;
	char* tmp_buf;
	_int32 tmp_len;
	char http_header[1024] = {0};
	_u32 http_header_len = 1024;
	_u32 encode_len = 0;
	cmd->_header._version = SHUB_PROTOCOL_VER;
	cmd->_header._seq = g_shub_seq++; 
	cmd->_header._cmd_len = 60 + cmd->_url_or_gcid_size + cmd->_cid_size + cmd->_origin_url_size + cmd->_peerid_size + cmd->_refer_url_size + cmd->_partner_id_size;
	*len = HUB_CMD_HEADER_LEN + cmd->_header._cmd_len;
	encode_len = (cmd->_header._cmd_len & 0xfffffff0) + HUB_ENCODE_PADDING_LEN + HUB_CMD_HEADER_LEN;
	encode_len = (encode_len & 0xfffffff0) + HUB_ENCODE_PADDING_LEN + RSA_ENCRYPT_HEADER_LEN;
	ret = res_query_build_http_header(http_header, &http_header_len, encode_len,device->_hub_type, device->_host, device->_port);
	CHECK_VALUE(ret);
	cmd->_cmd_type = QUERY_SERVER_RES;
	ret = sd_malloc(http_header_len + encode_len, (void**)buffer);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("build_query_server_res_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	sd_memset(*buffer, 0, http_header_len + encode_len);
	sd_memcpy(*buffer, http_header, http_header_len);
	tmp_buf = *buffer + http_header_len + RSA_ENCRYPT_HEADER_LEN;
	tmp_len = (_int32)*len;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._cmd_len);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_client_version);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_compress);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_cmd_type);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_by_what);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_url_or_gcid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_url_or_gcid, (_int32)cmd->_url_or_gcid_size);
	if(cmd->_by_what & QUERY_BY_CID)
	{
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_cid_size);
		sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_cid, (_int32)cmd->_cid_size);
	}
	else
	{
		_u32 len = 0;
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)len);
	}
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_origin_url_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_origin_url, cmd->_origin_url_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_max_server_res);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_bonus_res_num);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, (_int32)cmd->_peerid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char* )&cmd->_peer_report_ip, sizeof(_u32));
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_peer_capability);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_query_times_sn);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_cid_query_type);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_refer_url_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_refer_url, (_int32)cmd->_refer_url_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_partner_id_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_partner_id, cmd->_partner_id_size);
	ret = sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_product_flag);
	sd_assert(ret==SUCCESS);
	ret = xl_aes_encrypt(*buffer + http_header_len + RSA_ENCRYPT_HEADER_LEN, len);
	if(ret != SUCCESS)
	{
		LOG_ERROR("build_query_server_res_cmd, but aes_encrypt failed, errcode = %d.", ret);
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

_int32 build_enrollsp1_cmd_rsa(HUB_DEVICE* device,char** buffer, _u32* len, ENROLLSP1_CMD* cmd,
	_u8* aes_key, _int32 pubkey_version)
{
	_int32 ret = SUCCESS;
	char http_header[1024] = {0};
	_u32 http_header_len = 1024;
	char*  tmp_buf;
	_int32 tmp_len;
	_u32 encode_len = 0;
	cmd->_header._version = SHUB_PROTOCOL_VER;
	cmd->_header._seq = g_shub_seq++;
	cmd->_header._cmd_len = 80 + cmd->_internal_ip_len + cmd->_thunder_version_len + cmd->_partner_id_len;
	cmd->_cmd_type = ENROLLSP1;
	*len = HUB_CMD_HEADER_LEN + cmd->_header._cmd_len;
	encode_len = (cmd->_header._cmd_len & 0xfffffff0) + HUB_ENCODE_PADDING_LEN + HUB_CMD_HEADER_LEN;
	encode_len = (encode_len & 0xfffffff0) + HUB_ENCODE_PADDING_LEN + RSA_ENCRYPT_HEADER_LEN;
	ret = res_query_build_http_header(http_header, &http_header_len, encode_len,device->_hub_type, device->_host, device->_port);
	CHECK_VALUE(ret);
	ret = sd_malloc(http_header_len + encode_len, (void**)buffer);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("build_enrollsp1_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	LOG_DEBUG("build_enrollsp1_cmd.");

	sd_memcpy(*buffer, http_header, http_header_len);
	tmp_buf = *buffer + http_header_len + RSA_ENCRYPT_HEADER_LEN;
	tmp_len = (_int32)*len;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._cmd_len);   
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_client_version);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_compress);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_cmd_type);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_local_peerid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_local_peerid, cmd->_local_peerid_len);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_internal_ip_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_internal_ip, cmd->_internal_ip_len);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_product_flag);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_thunder_version_len);
	sd_set_bytes(&tmp_buf, &tmp_len, &cmd->_thunder_version, cmd->_thunder_version_len);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_network_type);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_os_type);
	sd_assert(cmd->_os_details_len == 0);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_os_details_len);
	sd_assert(cmd->_user_details_len == 0);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_user_details_len);
	sd_assert(cmd->_plugin_size == 0);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_plugin_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_enable_login_control);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_filter_peerid_version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_user_agent_version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_partner_id_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_partner_id, cmd->_partner_id_len);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_head_suffix_map_version);

	ret = xl_aes_encrypt(*buffer + http_header_len + RSA_ENCRYPT_HEADER_LEN, len);
	if(ret != SUCCESS)
	{
		LOG_ERROR("build_enrollsp1_cmd, but aes_encrypt failed, errcode = %d.", ret);
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

_int32 build_query_bt_info_cmd_rsa(HUB_DEVICE* device, char** buffer, _u32* len, QUERY_BT_INFO_CMD* cmd,
	_u8* aes_key, _int32 pubkey_version)
{
	static _u32 seq = 0;
	_int32 ret;
	char*  tmp_buf;
	_int32 tmp_len;
	char http_header[1024] = {0};
	_u32 http_header_len = 1024;
	_u32 encode_len = 0;
	cmd->_header._version = SHUB_PROTOCOL_VER;
	cmd->_header._seq = seq++;
	cmd->_header._cmd_len = 61;
	cmd->_cmd_type = QUERY_BT_INFO;
	*len = HUB_CMD_HEADER_LEN + cmd->_header._cmd_len;
	encode_len = (cmd->_header._cmd_len & 0xfffffff0) + HUB_ENCODE_PADDING_LEN + HUB_CMD_HEADER_LEN;
	encode_len = (encode_len & 0xfffffff0) + HUB_ENCODE_PADDING_LEN + RSA_ENCRYPT_HEADER_LEN;
	ret = res_query_build_http_header(http_header, &http_header_len, encode_len,device->_hub_type, device->_host, device->_port);
	CHECK_VALUE(ret);
	ret = sd_malloc(http_header_len + encode_len, (void**)buffer);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("build_query_server_res_cmd, malloc failed.");
		CHECK_VALUE(ret);
	}
	sd_memcpy(*buffer, http_header, http_header_len);
	tmp_buf = *buffer + http_header_len + RSA_ENCRYPT_HEADER_LEN;
	tmp_len = (_int32)*len;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._version);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_header._cmd_len);   
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_client_version);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_compress);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_cmd_type);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, cmd->_peerid_len);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_info_id_len);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_info_id, cmd->_info_id_len);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_file_index);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_query_times);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_need_bcid);
	ret = xl_aes_encrypt(*buffer + http_header_len + RSA_ENCRYPT_HEADER_LEN, len);
	if(ret != SUCCESS)
	{
		LOG_ERROR("build_query_bt_info_cmd, but aes_encrypt failed, errcode = %d.", ret);
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

_int32 build_query_tracker_res_cmd_rsa(HUB_DEVICE* device,char** buffer, _u32* len, QUERY_TRACKER_RES_CMD* cmd, 
	_u8* aes_key, _int32 pubkey_version)
{
	static _u32 seq = 0;
	_int32 ret;
	char* tmp_buf;
	_int32  tmp_len;
	char http_header[1024] = {0};
	_u32 http_header_len = 1024;
	_u32 encode_len = 0;
	cmd->_protocol_version = TRACKER_PROTOCOL_VER;
	cmd->_seq = seq++;
	cmd->_cmd_len = 71 + cmd->_gcid_size + cmd->_peerid_size + cmd->_partner_id_size;
	cmd->_cmd_type = QUERY_TRACKER_RES;
	*len = HUB_CMD_HEADER_LEN + cmd->_cmd_len;
	encode_len = (cmd->_cmd_len & 0xfffffff0) + HUB_ENCODE_PADDING_LEN + HUB_CMD_HEADER_LEN;
	encode_len = (encode_len & 0xfffffff0) + HUB_ENCODE_PADDING_LEN + RSA_ENCRYPT_HEADER_LEN;
	ret = res_query_build_http_header(http_header, &http_header_len, encode_len,device->_hub_type, device->_host, device->_port);
	CHECK_VALUE(ret);
	ret = sd_malloc(http_header_len + encode_len, (void**)buffer);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("malloc failed.");
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
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_by_what);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_gcid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)cmd->_gcid, (_int32)cmd->_gcid_size);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_file_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peerid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_peerid, (_int32)cmd->_peerid_size);
	sd_set_bytes(&tmp_buf, &tmp_len, (char* )&cmd->_local_ip, sizeof(_u32));
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_tcp_port);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_max_res);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_nat_type);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_download_ratio);
	sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)cmd->_download_map);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_peer_capability);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_release_id);
	sd_set_bytes(&tmp_buf, &tmp_len, (char* )&cmd->_upnp_ip, sizeof(_u32));
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd->_upnp_port);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_p2p_capability);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_udp_port);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd->_rsc_type);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_partner_id_size);
	sd_set_bytes(&tmp_buf, &tmp_len, cmd->_partner_id, (_int32)cmd->_partner_id_size);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd->_product_flag);

	ret = xl_aes_encrypt(*buffer + http_header_len + RSA_ENCRYPT_HEADER_LEN, len);
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
