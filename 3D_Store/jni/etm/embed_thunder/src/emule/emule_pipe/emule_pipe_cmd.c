
#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL

#include "emule_pipe_cmd.h"
#include "emule_pipe_interface.h"
#include "emule_pipe_device.h"
#include "../emule_data_manager/emule_data_manager.h"
#include "../emule_server/emule_server_impl.h"
#include "../emule_utility/emule_tag_list.h"
#include "../emule_utility/emule_peer.h"
#include "../emule_utility/emule_utility.h"
#include "../emule_utility/emule_opcodes.h"
#include "../emule_utility/emule_define.h"
#include "../emule_utility/emule_setting.h"
#include "../emule_utility/emule_udp_device.h"
#include "../emule_impl.h"
#include "utility/mempool.h"
#include "utility/utility.h"
#include "utility/bytebuffer.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_EMULE
#include "utility/logger.h"

#define	IS_SIGNATURENEEDED	1
#define	IS_KEYANDSIGNEEDED	2

_int32 emule_pipe_send_hello_cmd(EMULE_DATA_PIPE* emule_pipe)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL, *tmp_buf = NULL;
	_int32 len = 0, tmp_len = 0;
	_int16 tcp_port = 0;
	_u32 server_ip = 0;
	_u16 server_port = 0;
	EMULE_PEER* local_peer = emule_get_local_peer();
	LOG_DEBUG("[emule_pipe = 0x%x]emule pipe send HELLO cmd.", emule_pipe);
	len = emule_tag_list_size(&local_peer->_tag_list) + 19 + USER_ID_SIZE;
	ret = sd_malloc(len, (void**)&buffer);
	if(ret != SUCCESS)
	{
		return ret;
	}
	// 如果是低client id,返回监听端口也没用,但迅雷的80端口会成为
	// 其他客户端封杀的特征值
	// 如果迅雷重启后再次握手，反吸血保护会检查本次的userid是否和上次一致，不一致将
	// 被ban
	if (IS_LOWID(local_peer->_client_id))
	{   
		tcp_port = 65535;
        LOG_DEBUG("IS_LOWID(local_peer->_client_id(%u)) == TRUE, set tcp_port = 65535.", 
            local_peer->_client_id);
	}
	tmp_buf = buffer;
	tmp_len = len;
	sd_set_int8(&tmp_buf, &tmp_len, OP_EDONKEYPROT);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, len - EMULE_HEADER_SIZE);
	sd_set_int8(&tmp_buf, &tmp_len, OP_HELLO);
	sd_set_int8(&tmp_buf, &tmp_len, USER_ID_SIZE);		//hash size
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)local_peer->_user_id, USER_ID_SIZE);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, local_peer->_client_id);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, tcp_port);
	emule_tag_list_to_buffer(&local_peer->_tag_list, &tmp_buf, &tmp_len);
	emule_get_local_server(&server_ip, &server_port);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)server_ip);
	ret = sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)server_port);
	sd_assert(ret == SUCCESS && tmp_len == 0);
	return emule_pipe_device_send(emule_pipe->_device, buffer, len);
}

_int32 emule_pipe_send_hello_answer_cmd(EMULE_DATA_PIPE* emule_pipe)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL, *tmp_buf = NULL;
	_int32 len = 0, tmp_len = 0;
	_u32 server_ip = 0;
	_u16 server_port = 0, tcp_port = 65535;
	EMULE_PEER* local_peer = emule_get_local_peer();
	LOG_DEBUG("[emule_pipe = 0x%x]emule pipe send HELLO_ANSWER cmd.", emule_pipe);
	len = emule_tag_list_size(&local_peer->_tag_list) + USER_ID_SIZE + 18;
	ret = sd_malloc(len, (void**)&buffer);
	if(ret != SUCCESS)
		return ret;
	tmp_buf = buffer;
	tmp_len = len;
	sd_set_int8(&tmp_buf, &tmp_len, OP_EDONKEYPROT);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, len - EMULE_HEADER_SIZE);
	sd_set_int8(&tmp_buf, &tmp_len, OP_HELLOANSWER);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)local_peer->_user_id, USER_ID_SIZE);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)local_peer->_client_id);
	// wjp(2008.10.13)
	// 如果是低client id,返回监听端口也没用,但迅雷的80端口会成为
	// 其他客户端封杀的特征值
	if (IS_LOWID(local_peer->_client_id))
	{
		tcp_port = 65535;
	}
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)tcp_port);
	emule_tag_list_to_buffer(&local_peer->_tag_list, &tmp_buf, &tmp_len);
	emule_get_local_server(&server_ip, &server_port);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)server_ip);
	ret = sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)server_port);
	sd_assert(ret == SUCCESS && tmp_len == 0);
	return emule_pipe_device_send(emule_pipe->_device, buffer, len);
}

_int32 emule_pipe_send_req_filename_cmd(EMULE_DATA_PIPE* emule_pipe)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL, *tmp_buf = NULL;
	_int32 len = 0, tmp_len = 0;
	_u8 request_option = 0;
	EMULE_DATA_MANAGER* data_manager = (EMULE_DATA_MANAGER*)emule_pipe->_data_pipe._p_data_manager;

#ifdef _LOGGER
        char test_buffer[1024];
#endif

	request_option = emule_peer_get_extended_requests_option(&emule_pipe->_remote_peer);
    LOG_DEBUG("[emule_pipe = 0x%x]emule pipe send REQUEST_FILENAME cmd.request_option:%d", emule_pipe,request_option);

    len = 6 + FILE_ID_SIZE;
	if(request_option > 0)
		len += (2 + data_manager->_part_bitmap._mem_size);
	if(request_option > 1)
		len += 2;
	ret = sd_malloc(len, (void**)&buffer);
	if(ret != SUCCESS)
		return ret;
	tmp_buf = buffer;
	tmp_len = len;
	sd_set_int8(&tmp_buf, &tmp_len, OP_EDONKEYPROT);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, len - EMULE_HEADER_SIZE);
	sd_set_int8(&tmp_buf, &tmp_len, OP_REQUESTFILENAME);
	ret = sd_set_bytes(&tmp_buf, &tmp_len, (char*)data_manager->_file_id, FILE_ID_SIZE);
	// 分块状态
	if(request_option > 0)
	{
		sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)data_manager->_part_bitmap._bit_count);
		ret = sd_set_bytes(&tmp_buf, &tmp_len, (char*)data_manager->_part_bitmap._bit, (_int32)data_manager->_part_bitmap._mem_size);

#ifdef _LOGGER
        sd_memset(test_buffer,0,1024);
        str2hex( (char*)data_manager->_part_bitmap._bit, data_manager->_part_bitmap._mem_size, test_buffer, 1024);
        LOG_DEBUG("[emule_pipe = 0x%x]emule pipe send REQUEST_FILENAME cmd.part_bitmap[%u]=%s", 
         emule_pipe, data_manager->_part_bitmap._bit_count, test_buffer);
#endif

    }
	// 源数目
	if(request_option > 1)
	{
		//目前假设源数目为0，以后再修改
		ret = sd_set_int16_to_lt(&tmp_buf, &tmp_len, 0);
	}
	sd_assert(ret == SUCCESS && tmp_len == 0);
	return emule_pipe_device_send(emule_pipe->_device, buffer, len);
}

_int32 emule_pipe_send_req_filename_answer_cmd(EMULE_DATA_PIPE* emule_pipe)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL, *tmp_buf = NULL;
	_int32 len = 0, tmp_len = 0;
	EMULE_DATA_MANAGER* data_manager = (EMULE_DATA_MANAGER*)emule_pipe->_data_pipe._p_data_manager;
	_int16 filename_len = sd_strlen(data_manager->_file_info._file_name);
	LOG_DEBUG("[emule_pipe = 0x%x]emule pipe send REQUEST_FILE_NAME_ANSWER cmd.", emule_pipe);
	len = 24 + filename_len;
	ret = sd_malloc(len, (void**)&buffer);
	CHECK_VALUE(ret);
	tmp_buf = buffer;
	tmp_len = len;
	sd_set_int8(&tmp_buf, &tmp_len, OP_EDONKEYPROT);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, len - EMULE_HEADER_SIZE);
	sd_set_int8(&tmp_buf, &tmp_len, OP_REQFILENAMEANSWER);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)data_manager->_file_id, FILE_ID_SIZE);
	//文件名
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, filename_len);
	ret = sd_set_bytes(&tmp_buf, &tmp_len, data_manager->_file_info._file_name, filename_len);
	sd_assert(ret == SUCCESS && tmp_len == 0);
	return emule_pipe_device_send(emule_pipe->_device, buffer, len);
}

_int32 emule_pipe_send_req_file_id_cmd(EMULE_DATA_PIPE* emule_pipe)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL, *tmp_buf = NULL;
	_int32 len = 0, tmp_len = 0;
	EMULE_DATA_MANAGER* data_manager = (EMULE_DATA_MANAGER*)emule_pipe->_data_pipe._p_data_manager;
	LOG_DEBUG("[emule_pipe = 0x%x]emule pipe send REQUEST_FILE_ID cmd.", emule_pipe);
	len = 6 + FILE_ID_SIZE;
	ret = sd_malloc(len, (void**)&buffer);
	if(ret != SUCCESS)
		return ret;
	tmp_buf = buffer;
	tmp_len = len;
	sd_set_int8(&tmp_buf, &tmp_len, OP_EDONKEYPROT);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, len - EMULE_HEADER_SIZE);
	sd_set_int8(&tmp_buf, &tmp_len, OP_SETREQFILEID);
	ret = sd_set_bytes(&tmp_buf, &tmp_len, (char*)data_manager->_file_id, FILE_ID_SIZE);
	sd_assert(ret == SUCCESS && tmp_len == 0);
	return emule_pipe_device_send(emule_pipe->_device, buffer, len);
}

_int32 emule_pipe_send_part_hash_req_cmd(EMULE_DATA_PIPE* emule_pipe)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL, *tmp_buf = NULL;
	_int32 len = 0, tmp_len = 0;
	EMULE_DATA_MANAGER* data_manager = (EMULE_DATA_MANAGER*)emule_pipe->_data_pipe._p_data_manager;
	LOG_DEBUG("[emule_pipe = 0x%x]emule pipe send HASHSET_REQUEST cmd.", emule_pipe);
	len = 6 + FILE_ID_SIZE;
	ret = sd_malloc(len, (void**)&buffer);
	if(ret != SUCCESS)
		return ret;
	tmp_buf = buffer;
	tmp_len = len;
	sd_set_int8(&tmp_buf, &tmp_len, OP_EDONKEYPROT);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, len - EMULE_HEADER_SIZE);
	sd_set_int8(&tmp_buf, &tmp_len, OP_HASHSETREQUEST);
	ret = sd_set_bytes(&tmp_buf, &tmp_len, (char*)data_manager->_file_id, FILE_ID_SIZE);
	sd_assert(ret == SUCCESS && tmp_len == 0);
	return emule_pipe_device_send(emule_pipe->_device, buffer, len);
}

_int32 emule_pipe_send_arch_hash_req_cmd(EMULE_DATA_PIPE* emule_pipe)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL, *tmp_buf = NULL;
	_int32 len = 0, tmp_len = 0;
	EMULE_DATA_MANAGER* data_manager = (EMULE_DATA_MANAGER*)emule_pipe->_data_pipe._p_data_manager;
	LOG_DEBUG("[emule_pipe = 0x%x]emule pipe send OP_AICHFILEHASHREQ cmd.", emule_pipe);
	len = 6 + FILE_ID_SIZE;
	ret = sd_malloc(len, (void**)&buffer);
	if(ret != SUCCESS)
		return ret;
	tmp_buf = buffer;
	tmp_len = len;
	sd_set_int8(&tmp_buf, &tmp_len, OP_EMULEPROT);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, len - EMULE_HEADER_SIZE);
	sd_set_int8(&tmp_buf, &tmp_len, OP_AICHFILEHASHREQ);
	ret = sd_set_bytes(&tmp_buf, &tmp_len, (char*)data_manager->_file_id, FILE_ID_SIZE);
	sd_assert(ret == SUCCESS && tmp_len == 0);
	return emule_pipe_device_send(emule_pipe->_device, buffer, len);
}

_int32 emule_pipe_send_part_hash_answer_cmd(struct tagEMULE_DATA_PIPE* emule_pipe)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL, *tmp_buf = NULL;
	_int32 len = 0, tmp_len = 0;
#ifdef _LOGGER
            char test_buffer[1024];
#endif

	EMULE_DATA_MANAGER* data_manager = (EMULE_DATA_MANAGER*)emule_pipe->_data_pipe._p_data_manager;
	LOG_DEBUG("[emule_pipe = 0x%x]emule pipe send HASHSET_ANSWER cmd. _part_hash:0x%x", 
     emule_pipe, data_manager->_part_checker._part_hash );
	if(NULL == data_manager->_part_checker._part_hash)
	{
       return SUCCESS;            //我自己也没有part_hash，不给对方回复
	}
	len = 8 + FILE_ID_SIZE + data_manager->_part_checker._part_hash_len;
	ret = sd_malloc(len, (void**)&buffer);
	if(ret != SUCCESS)
		return ret;
	tmp_buf = buffer;
	tmp_len = len;
	sd_set_int8(&tmp_buf, &tmp_len, OP_EDONKEYPROT);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, len - EMULE_HEADER_SIZE);
	sd_set_int8(&tmp_buf, &tmp_len, OP_HASHSETANSWER);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)data_manager->_file_id, FILE_ID_SIZE);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)data_manager->_part_checker._part_hash_len / PART_HASH_SIZE);
	ret = sd_set_bytes(&tmp_buf, &tmp_len, (char*)data_manager->_part_checker._part_hash, data_manager->_part_checker._part_hash_len);
	sd_assert(ret == SUCCESS && tmp_len == 0);

#ifdef _LOGGER
    sd_memset(test_buffer,0,1024);
    str2hex( (char*)data_manager->_part_checker._part_hash, data_manager->_part_checker._part_hash_len, test_buffer, 1024);
    LOG_DEBUG("[emule_pipe = 0x%x]emule pipe send HASHSET_ANSWER cmd.part_bitmap[%u]=%s", 
     emule_pipe, data_manager->_part_checker._part_hash_len, test_buffer);
#endif

	return emule_pipe_device_send(emule_pipe->_device, buffer, len);
}

_int32 emule_pipe_send_start_upload_req_cmd(EMULE_DATA_PIPE* emule_pipe)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL, *tmp_buf = NULL;
	_int32 len = 0, tmp_len = 0;
	EMULE_DATA_MANAGER* data_manager = (EMULE_DATA_MANAGER*)emule_pipe->_data_pipe._p_data_manager;
	LOG_DEBUG("[emule_pipe = 0x%x]emule pipe send START_UPLOAD_REQUEST cmd.", emule_pipe);
	len = 6 + FILE_ID_SIZE;
	ret = sd_malloc(len, (void**)&buffer);
	if(ret != SUCCESS)
		return ret;
	tmp_buf = buffer;
	tmp_len = len;
	sd_set_int8(&tmp_buf, &tmp_len, OP_EDONKEYPROT);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, len - EMULE_HEADER_SIZE);
	sd_set_int8(&tmp_buf, &tmp_len, OP_STARTUPLOADREQ);
	ret = sd_set_bytes(&tmp_buf, &tmp_len, (char*)data_manager->_file_id, FILE_ID_SIZE);
	sd_assert(ret == SUCCESS && tmp_len == 0);
	return emule_pipe_device_send(emule_pipe->_device, buffer, len);
}

// 每次可以请求3段数据,每段的长度不能大于180k*3
_int32 emule_pipe_send_req_part_cmd(EMULE_DATA_PIPE* emule_pipe, RANGE* req_range)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL, *tmp_buf = NULL;
	_int32 len = 0, tmp_len = 0;
	_u64 start_pos64 = 0, end_pos64 = 0;
	_u32 start_pos = 0, end_pos = 0;
	BOOL large_pos =FALSE;
	EMULE_DATA_MANAGER* data_manager = (EMULE_DATA_MANAGER*)emule_pipe->_data_pipe._p_data_manager;
	sd_assert(emule_get_range_size(data_manager, req_range) < 180 * 1024);
	start_pos64 = (_u64)get_data_unit_size() * req_range->_index;
	end_pos64 = start_pos64 + emule_get_range_size(data_manager, req_range);
	if(start_pos64 > 0xFFFFFFFF || end_pos64 >= 0xFFFFFFFF)
		large_pos = TRUE;
	if(large_pos)
		len = 6 + FILE_ID_SIZE + 48;
	else
		len = 6 + FILE_ID_SIZE + 24;
	ret = sd_malloc(len, (void**)&buffer);
	if(ret != SUCCESS)
		return ret;
	tmp_buf = buffer;
	tmp_len = len;
	if(large_pos)
	{
		sd_set_int8(&tmp_buf, &tmp_len, OP_EMULEPROT);
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, len - EMULE_HEADER_SIZE);
		sd_set_int8(&tmp_buf, &tmp_len, OP_REQUESTPARTS_I64);
	}
	else
	{
		sd_set_int8(&tmp_buf, &tmp_len, OP_EDONKEYPROT);
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, len - EMULE_HEADER_SIZE);
		sd_set_int8(&tmp_buf, &tmp_len, OP_REQUESTPARTS);
	}
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)data_manager->_file_id, FILE_ID_SIZE);
	if(large_pos)
	{
		sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)start_pos64);
		sd_set_int64_to_lt(&tmp_buf, &tmp_len, 0);
		sd_set_int64_to_lt(&tmp_buf, &tmp_len, 0);
		sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)end_pos64);
		sd_set_int64_to_lt(&tmp_buf, &tmp_len, 0);
		ret = sd_set_int64_to_lt(&tmp_buf, &tmp_len, 0);
	}
	else
	{
		start_pos = (_u32)start_pos64;
		end_pos = (_u32)end_pos64;
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)start_pos);
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, 0);
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, 0);
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)end_pos);
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, 0);
		ret = sd_set_int32_to_lt(&tmp_buf, &tmp_len, 0);
	}
	LOG_DEBUG("[emule_pipe = 0x%x]emule pipe send REQUEST_PARTS cmd, range[%u, %u], pos[%llu, %llu].", emule_pipe, req_range->_index, req_range->_num, start_pos64, end_pos64);
	sd_assert(ret == SUCCESS && tmp_len == 0);
	return emule_pipe_device_send(emule_pipe->_device, buffer, len);
}

_int32 emule_pipe_send_req_source_cmd(EMULE_DATA_PIPE* emule_pipe)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL, *tmp_buf = NULL;
	_int32 len = 0, tmp_len = 0;
	EMULE_DATA_MANAGER* data_manager = (EMULE_DATA_MANAGER*)emule_pipe->_data_pipe._p_data_manager;
	if(emule_enable_source_exchange() == FALSE)
		return SUCCESS;
	LOG_DEBUG("[emule_pipe = 0x%x]emule pipe send REQUEST_SOURCES cmd.", emule_pipe);
	len = 6 + FILE_ID_SIZE;
	ret = sd_malloc(len, (void**)&buffer);
	CHECK_VALUE(ret);
	tmp_buf = buffer;
	tmp_len = len;
	sd_set_int8(&tmp_buf, &tmp_len, OP_EMULEPROT);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, len - EMULE_HEADER_SIZE);
	sd_set_int8(&tmp_buf, &tmp_len, OP_REQUESTSOURCES);
	ret = sd_set_bytes(&tmp_buf, &tmp_len, (char*)data_manager->_file_id, FILE_ID_SIZE);
	sd_assert(ret == SUCCESS && tmp_len == 0);
	return emule_pipe_device_send(emule_pipe->_device, buffer, len);
}

_int32 emule_pipe_send_emule_info_answer_cmd(EMULE_DATA_PIPE* emule_pipe)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL, *tmp_buf = NULL;
	_int32 len = 0, tmp_len = 0;
	EMULE_TAG_LIST tag_list;
	EMULE_TAG* tag = NULL;
	EMULE_PEER* local_peer = emule_get_local_peer();
	LOG_DEBUG("[emule_pipe = 0x%x]emule pipe send EMULE_INFO_ANSWER cmd.", emule_pipe);
	emule_tag_list_init(&tag_list);
	emule_create_u32_tag(&tag, ET_COMPRESSION, 0);
	emule_tag_list_add(&tag_list, tag);
//	emule_create_u32_tag(&tag, ET_UDPVER, 4);
//	emule_tag_list_add(&tag_list, tag);
	emule_create_u32_tag(&tag, ET_UDPPORT, local_peer->_udp_port);
	emule_tag_list_add(&tag_list, tag);
	emule_create_u32_tag(&tag, ET_SOURCEEXCHANGE, 4);
	emule_tag_list_add(&tag_list, tag);
	emule_create_u32_tag(&tag, ET_COMMENTS, 1);
	emule_tag_list_add(&tag_list, tag);
	emule_create_u32_tag(&tag, ET_EXTENDEDREQUEST, 2);
	emule_tag_list_add(&tag_list, tag);
	emule_create_u32_tag(&tag, ET_FEATURES, 0);		//不支持身份验证
	emule_tag_list_add(&tag_list, tag);	
	len = 8 + emule_tag_list_size(&tag_list);
	ret = sd_malloc(len, (void**)&buffer);
	if(ret != SUCCESS)
	{
		emule_tag_list_uninit(&tag_list, TRUE);
		return ret;
	}	
	tmp_buf = buffer;
	tmp_len = len;
	sd_set_int8(&tmp_buf, &tmp_len, OP_EMULEPROT);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, len - EMULE_HEADER_SIZE);
	sd_set_int8(&tmp_buf, &tmp_len, OP_EMULEINFOANSWER);
	sd_set_int8(&tmp_buf, &tmp_len, 72);		//version
	sd_set_int8(&tmp_buf, &tmp_len, EMULE_PROTOCOL);	//protocol
	ret = emule_tag_list_to_buffer(&tag_list, &tmp_buf, &tmp_len);
	sd_assert(ret == SUCCESS && tmp_len == 0);
	emule_tag_list_uninit(&tag_list, TRUE);
	return emule_pipe_device_send(emule_pipe->_device, buffer, len);
}

_int32 emule_pipe_send_file_state_cmd(EMULE_DATA_PIPE* emule_pipe)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL, *tmp_buf = NULL;
	_int32 len = 0, tmp_len = 0;
	EMULE_DATA_MANAGER* data_manager = (EMULE_DATA_MANAGER*)emule_pipe->_data_pipe._p_data_manager;
    sd_assert(data_manager != NULL);

#ifdef _LOGGER
    char test_buffer[1024];
    sd_memset(test_buffer,0,1024);
    str2hex( (char*)data_manager->_part_bitmap._bit, data_manager->_part_bitmap._mem_size, test_buffer, 1024);
	LOG_DEBUG("[emule_pipe = 0x%x]emule pipe send FILE_STATE cmd.part_bitmap[%u]=%s", 
     emule_pipe, data_manager->_part_bitmap._bit_count, test_buffer);
#endif

	len = 8 + FILE_ID_SIZE + data_manager->_part_bitmap._mem_size;
	ret = sd_malloc(len, (void**)&buffer);
	if(ret != SUCCESS)
		return ret;
	tmp_buf = buffer;
	tmp_len = len;
	sd_set_int8(&tmp_buf, &tmp_len, OP_EDONKEYPROT);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, len - EMULE_HEADER_SIZE);
	sd_set_int8(&tmp_buf, &tmp_len, OP_FILESTATUS);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)data_manager->_file_id, FILE_ID_SIZE);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)data_manager->_part_bitmap._bit_count);
	ret = sd_set_bytes(&tmp_buf, &tmp_len, (char*)data_manager->_part_bitmap._bit, data_manager->_part_bitmap._mem_size);
	sd_assert(ret == SUCCESS && tmp_len == 0);
	return emule_pipe_device_send(emule_pipe->_device, buffer, len);
}

_int32 emule_pipe_send_accept_upload_req_cmd(EMULE_DATA_PIPE* emule_pipe, BOOL is_accept)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL, *tmp_buf = NULL;
	_int32 len = 0, tmp_len = 0;
	len = 6;
    
	LOG_DEBUG("[emule_pipe = 0x%x]emule_pipe_send_accept_upload_req_cmd.", emule_pipe);
	ret = sd_malloc(len, (void**)&buffer);
	if(ret != SUCCESS)
		return ret;
	tmp_buf = buffer;
	tmp_len = len;
	sd_set_int8(&tmp_buf, &tmp_len, OP_EDONKEYPROT);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, len - EMULE_HEADER_SIZE);
    if(is_accept)
    {
        ret = sd_set_int8(&tmp_buf, &tmp_len, OP_ACCEPTUPLOADREQ);
    }
    else
    {
        ret = sd_set_int8(&tmp_buf, &tmp_len, OP_OUTOFPARTREQS);
    }
	sd_assert(ret == SUCCESS && tmp_len == 0);
	return emule_pipe_device_send(emule_pipe->_device, buffer, len);
}

_int32 emule_pipe_send_reask_cmd(EMULE_DATA_PIPE* emule_pipe)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL, *tmp_buf = NULL;
	_int32 len = 0, tmp_len = 0;
	EMULE_DATA_MANAGER* data_manager = (EMULE_DATA_MANAGER*)emule_pipe->_data_pipe._p_data_manager;
	const EMULE_TAG* tag = NULL;
	_u32 udp_ver = 0;
	if(IS_LOWID(emule_pipe->_remote_peer._client_id) == TRUE)
		return SUCCESS; 
	tag = emule_tag_list_get(&emule_pipe->_remote_peer._tag_list, CT_EMULE_MISCOPTIONS1);
	if(tag == NULL)
		return SUCCESS;
	sd_assert(emule_tag_is_u32(tag) == TRUE);
	udp_ver = tag->_value._val_u32;
	udp_ver = GET_MISC_OPTION(udp_ver, UDPVER);
	len = 18;
	if(udp_ver > 3)
		len = 22 + data_manager->_part_bitmap._mem_size;
	else if(udp_ver > 2)
		len = 20;
	ret = sd_malloc(len, (void**)&buffer);
	if(ret != SUCCESS)
		return ret;
	tmp_buf = buffer;
	tmp_len = len;
	sd_set_int8(&tmp_buf, &tmp_len, OP_EMULEPROT);
	ret = sd_set_int8(&tmp_buf, &tmp_len, OP_REASKFILEPING);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)data_manager->_file_id, FILE_ID_SIZE);
	if(udp_ver > 3)
	{
		sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)data_manager->_part_bitmap._bit_count);
		ret = sd_set_bytes(&tmp_buf, &tmp_len, (char*)data_manager->_part_bitmap._bit, data_manager->_part_bitmap._mem_size);
	}
	if(udp_ver > 2)
		ret = sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)0);	//data.WriteUInt16(reqfile->m_nCompleteSourcesCount);
	sd_assert(ret == SUCCESS && tmp_len == 0);
	LOG_DEBUG("[emule_pipe = 0x%x]emule_pipe_send_reask_cmd.", emule_pipe);
	return emule_udp_sendto(buffer, len, emule_pipe->_remote_peer._client_id, emule_pipe->_remote_peer._udp_port, TRUE);
	//如果是low to low的用户，也要发reask，暂时没有low to low，所以没实现
}

_int32 emule_pipe_send_secident_state(EMULE_DATA_PIPE* emule_pipe)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL, *tmp_buf = NULL;
	_int32 len = 0, tmp_len = 0;
	len = 11;
	ret = sd_malloc(len, (void**)&buffer);
	if(ret != SUCCESS)
		return ret;
	tmp_buf = buffer;
	tmp_len = len;
	sd_set_int8(&tmp_buf, &tmp_len, OP_EMULEPROT);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, len - EMULE_HEADER_SIZE);
	sd_set_int8(&tmp_buf, &tmp_len, OP_SECIDENTSTATE);
	sd_set_int8(&tmp_buf, &tmp_len, IS_SIGNATURENEEDED);
	emule_pipe->_random = sd_rand() + 1;
	ret = sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)emule_pipe->_random);
	sd_assert(ret == SUCCESS && tmp_len == 0);
	LOG_DEBUG("[emule_pipe = 0x%x]emule_pipe_send_secident_state, state = %d.", emule_pipe, IS_SIGNATURENEEDED);
	return emule_pipe_device_send(emule_pipe->_device, buffer, len);
}

_int32 emule_pipe_send_aich_answer_cmd(struct tagEMULE_DATA_PIPE* emule_pipe)
{
    _int32 ret = SUCCESS;
    _u8 *aich_hash = NULL;
    _u32 aich_hash_len = 0;
    char* buffer = NULL, *tmp_buf = NULL;
    _int32 len = 0, tmp_len = 0;
    EMULE_DATA_MANAGER *data_manager = (EMULE_DATA_MANAGER *)(emule_pipe->_data_pipe._p_data_manager);

    LOG_DEBUG("[emule_pipe = 0x%x]emule pipe send AICH_HASH_ANSWER cmd, aich_len = %u.", emule_pipe, aich_hash_len);

    if (data_manager) {
        if (emule_is_aich_hash_valid(data_manager) == TRUE) {
            emule_get_aich_hash((void *)data_manager, &aich_hash, &aich_hash_len);
        }
        else {
            LOG_DEBUG("there is no aich, so return!");
            return SUCCESS;
        }
    }

    len = 6 + FILE_ID_SIZE + aich_hash_len;
    ret = sd_malloc(len, (void**)&buffer);
    if(ret != SUCCESS)
        return ret;
    tmp_buf = buffer;
    tmp_len = len;
    sd_set_int8(&tmp_buf, &tmp_len, OP_EMULEPROT);
    sd_set_int32_to_lt(&tmp_buf, &tmp_len, len - EMULE_HEADER_SIZE);
    sd_set_int8(&tmp_buf, &tmp_len, OP_AICHFILEHASHANS);
    sd_set_bytes(&tmp_buf, &tmp_len, (char*)data_manager->_file_id, FILE_ID_SIZE);
    ret = sd_set_bytes(&tmp_buf, &tmp_len, (char*)aich_hash, aich_hash_len);
    sd_assert(ret == SUCCESS && tmp_len == 0);
    return emule_pipe_device_send(emule_pipe->_device, buffer, len);
}

#endif /* ENABLE_EMULE */
#endif /* ENABLE_EMULE */

