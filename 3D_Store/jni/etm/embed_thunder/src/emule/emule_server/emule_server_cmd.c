#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL
#include "emule_server_cmd.h"
#include "emule_server_device.h"
#include "../emule_utility/emule_opcodes.h"
#include "../emule_utility/emule_tag_list.h"
#include "../emule_utility/emule_peer.h"
#include "../emule_utility/emule_udp_device.h"
#include "../emule_utility/emule_setting.h"
#include "../emule_impl.h"
#include "p2p_transfer_layer/ptl_utility.h"
#include "utility/mempool.h"
#include "utility/bytebuffer.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_EMULE
#include "utility/logger.h"

_int32 emule_send_udp_query_source_cmd(EMULE_SERVER* server, _u8 file_id[FILE_ID_SIZE], _u64 file_size)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL, *tmp_buf = NULL;
	_int32 buffer_len = 2 + FILE_ID_SIZE, tmp_len = 0;
	LOG_DEBUG("emule_send_udp_query_source_cmd.");
	if(server->_udp_flags & SRV_UDPFLG_EXT_GETSOURCES2)
	{
		buffer_len += sizeof(_u32);		//四个字节的file_size
		if(file_size >= OLD_MAX_EMULE_FILE_SIZE)
			buffer_len += sizeof(_u64);	//八个字节的file_size
	}
	ret = sd_malloc(buffer_len, (void**)&buffer);
	if(ret != SUCCESS)
		return ret;
	tmp_buf = buffer;
	tmp_len = buffer_len;
	sd_set_int8(&tmp_buf, &tmp_len, OP_EDONKEYPROT);
	if(server->_udp_flags & SRV_UDPFLG_EXT_GETSOURCES2)
	{
		sd_set_int8(&tmp_buf, &tmp_len, OP_GLOBGETSOURCES2);
		sd_set_bytes(&tmp_buf, &tmp_len, (char*)file_id, FILE_ID_SIZE);
		if(file_size < (_u64)OLD_MAX_EMULE_FILE_SIZE)
		{
			ret = sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)file_size);
		}
		else		//大文件
		{
			sd_set_int32_to_lt(&tmp_buf, &tmp_len, 0);
			ret = sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)file_size);
		}
	}
	else
	{
		sd_set_int8(&tmp_buf, &tmp_len, OP_GLOBGETSOURCES);
		ret = sd_set_bytes(&tmp_buf, &tmp_len, (char*)file_id, FILE_ID_SIZE);
	}
	sd_assert(ret == SUCCESS && tmp_len == 0);
	return emule_udp_sendto(buffer, buffer_len, server->_ip, server->_port + 4, TRUE);
}

_int32 emule_send_login_req_cmd(EMULE_SERVER_DEVICE* device)
{	
	_int32 ret = SUCCESS;
	char* buffer = NULL, *tmp_buf = NULL;
	_int32 buffer_len = 0, tmp_len = 0;
	_u32 crypt_flags = 0;
	EMULE_TAG_LIST tag_list;
	const EMULE_TAG* tag = NULL;
	EMULE_TAG* flag_tag = NULL, *emule_ver_tag = NULL;
	EMULE_PEER* local_peer = emule_get_local_peer();
	emule_tag_list_init(&tag_list);
	tag = emule_tag_list_get(&local_peer->_tag_list, CT_NAME);
	sd_assert(tag != NULL);
	emule_tag_list_add(&tag_list, (EMULE_TAG*)tag);
	tag = emule_tag_list_get(&local_peer->_tag_list, CT_VERSION);
	emule_tag_list_add(&tag_list, (EMULE_TAG*)tag);
	if(emule_enable_obfuscation())
	{
		crypt_flags |= SRVCAP_SUPPORTCRYPT;
		crypt_flags |= SRVCAP_REQUESTCRYPT;
		crypt_flags |= SRVCAP_REQUIRECRYPT;
	}
	//注意应该支持SRVCAP_ZLIB，否则有些服务器会拒绝连接
	emule_create_u32_tag(&flag_tag, CT_SERVER_FLAGS, SRVCAP_ZLIB | SRVCAP_NEWTAGS | SRVCAP_LARGEFILES | SRVCAP_UNICODE | crypt_flags);
	emule_tag_list_add(&tag_list, flag_tag);
	emule_create_u32_tag(&emule_ver_tag, CT_EMULE_VERSION, (0<<17) |(48<<10) |(0<<7));
	emule_tag_list_add(&tag_list, emule_ver_tag);
	buffer_len = 12 + USER_ID_SIZE + emule_tag_list_size(&tag_list);
	ret = sd_malloc(buffer_len, (void**)&buffer);
	if(ret != SUCCESS)
	{
		emule_destroy_tag(flag_tag);
		emule_destroy_tag(emule_ver_tag);
		return ret;
	}
	tmp_buf = buffer;
	tmp_len = buffer_len;
	sd_set_int8(&tmp_buf, &tmp_len, OP_EDONKEYPROT);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, buffer_len - EMULE_HEADER_SIZE);
	sd_set_int8(&tmp_buf, &tmp_len, OP_LOGINREQUEST);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)local_peer->_user_id, USER_ID_SIZE);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)local_peer->_client_id);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)local_peer->_tcp_port);
	ret = emule_tag_list_to_buffer(&tag_list, &tmp_buf, &tmp_len);
	sd_assert(ret == SUCCESS && tmp_len == 0);
	emule_destroy_tag(flag_tag);
	emule_destroy_tag(emule_ver_tag);
	emule_tag_list_uninit(&tag_list, FALSE);
	LOG_DEBUG("[emule_server] send OP_LOGINREQUEST cmd.");
	return emule_server_device_send(device, buffer, buffer_len);
}

_int32 emule_send_tcp_query_source_cmd(EMULE_SERVER_DEVICE* device, _u8 file_id[FILE_ID_SIZE], _u64 file_size)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL, *tmp_buf = NULL;
	_int32 buffer_len = 0, tmp_len = 0;

    LOG_DEBUG("emule_send_tcp_query_source_cmd.");

	buffer_len = 10 + FILE_ID_SIZE;
	if(file_size >= OLD_MAX_EMULE_FILE_SIZE)
		buffer_len += 8;
	ret = sd_malloc(buffer_len, (void**)&buffer);
	if(ret != SUCCESS)
		return ret;
	tmp_buf = buffer;
	tmp_len = buffer_len;
	sd_set_int8(&tmp_buf, &tmp_len, OP_EDONKEYPROT);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, buffer_len - EMULE_HEADER_SIZE);
	if(emule_enable_obfuscation() == TRUE)
		sd_set_int8(&tmp_buf, &tmp_len, OP_GETSOURCES_OBFU);
	else
		sd_set_int8(&tmp_buf, &tmp_len, OP_GETSOURCES);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)file_id, FILE_ID_SIZE);
	if(file_size < OLD_MAX_EMULE_FILE_SIZE)
	{
		ret = sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)file_size);
	}
	else
	{
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, 0);
		ret = sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)file_size);
	}
	sd_assert(ret == SUCCESS && tmp_len == 0);
	return emule_server_device_send(device, buffer, buffer_len);
}

_int32 emule_send_request_callback_cmd(EMULE_SERVER_DEVICE* device, _u32 client_id)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL, *tmp_buf = NULL;
	_int32 buffer_len = 0, tmp_len = 0;
	buffer_len = 10;
	ret = sd_malloc(buffer_len, (void**)&buffer);
	if(ret != SUCCESS)
		return ret;
	tmp_buf = buffer;
	tmp_len = buffer_len;
	sd_set_int8(&tmp_buf, &tmp_len, OP_EDONKEYPROT);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, buffer_len - EMULE_HEADER_SIZE);
	sd_set_int8(&tmp_buf, &tmp_len, OP_CALLBACKREQUEST);
	ret = sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)client_id);
	sd_assert(ret == SUCCESS && tmp_len == 0);
	return emule_server_device_send(device, buffer, buffer_len);
}


#endif /* ENABLE_EMULE */

#endif /* ENABLE_EMULE */

