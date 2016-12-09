#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL
#include "utility/bytebuffer.h"
#include "utility/utility.h"
#include "utility/logid.h"
#define LOGID LOGID_EMULE
#include "utility/logger.h"

#include "emule_pipe_handle_cmd.h"
#include "emule_pipe_device.h"
#include "emule_pipe_wait_accept.h"
#include "emule_pipe_impl.h"
#include "emule_pipe_cmd.h"
#include "emule_pipe_upload.h"
#include "../emule_data_manager/emule_data_manager.h"
#include "../emule_utility/emule_peer.h"
#include "../emule_interface.h"
#include "../emule_impl.h"
#include "res_query/res_query_interface.h"
#include "../emule_utility/emule_setting.h"

#ifdef UPLOAD_ENABLE
#include "upload_manager/upload_manager.h"
#endif

#include "task_manager/task_manager.h"

_int32 emule_handle_hello_cmd(EMULE_PIPE_DEVICE* pipe_device, char* buffer, _int32 len)
{
	_int32 ret = SUCCESS;
	EMULE_DATA_PIPE* emule_pipe = NULL;
	char* tmp_buf = NULL;
	_int32 tmp_len = 0;
	_u8 user_id_size = 0;
	_u8 user_id[USER_ID_SIZE];
	_u16 tcp_port = 0, server_port = 0;
	_u32 client_id = 0, server_ip = 0;
    RESOURCE *p_remote_emule_resource = NULL;
    DATA_PIPE *p_passive_emule_pipe = NULL;

	sd_get_int8(&buffer, &len, (_int8*)&user_id_size);
	sd_assert(user_id_size == USER_ID_SIZE);
	sd_get_bytes(&buffer, &len, (char*)user_id, USER_ID_SIZE);
	sd_get_int32_from_lt(&buffer, &len, (_int32*)&client_id);
	sd_get_int16_from_lt(&buffer, &len, (_int16*)&tcp_port);
	//先跳过tag_list的解析，获取后面的server_ip, server_port
	tmp_len = 6;
	tmp_buf = buffer + (len - tmp_len);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&server_ip);
	sd_get_int16_from_lt(&tmp_buf, &tmp_len, (_int16*)&server_port);
	emule_pipe = emule_find_wait_accept_pipe(user_id, client_id, server_ip, server_port);
	if((emule_pipe == NULL) && (pipe_device != NULL))
	{
        // 这里会有两种情况：
        // 1, 这个device确实已经被上层释放了
        // 2, 纯被动连接
        ret = emule_resource_create(&p_remote_emule_resource, client_id, tcp_port, server_ip, server_port);
        CHECK_VALUE(ret);

        ret = emule_pipe_create(&p_passive_emule_pipe, NULL, p_remote_emule_resource);
        CHECK_VALUE(ret);

        emule_pipe_change_state((EMULE_DATA_PIPE *)p_passive_emule_pipe, PIPE_CONNECTING);

        emule_pipe = (EMULE_DATA_PIPE *)p_passive_emule_pipe;

        LOG_DEBUG("create an emule pipe successfuly, emule_pipe = 0x%x.", emule_pipe);
	}
    else if ((emule_pipe != NULL) && (pipe_device != NULL))
    {
        LOG_DEBUG("find an accept pipe, so remove it from wait accept list, emule_pipe = 0x%x.", emule_pipe);
        ret = emule_remove_wait_accept_pipe(emule_pipe);
        CHECK_VALUE(ret);
        
        // 释放掉原来主动发起的那个device设备
        ret = emule_pipe_device_destroy(emule_pipe->_device);
        CHECK_VALUE(ret);

        // 其实这是本地主动发起的，应该不算是被动的资源
        pipe_device->_passive = FALSE;
    }
    else
    {
        LOG_ERROR("emule pipe device is NULL, return -1!");
        sd_assert(FALSE);
        return -1;
    }

	//解析emule_tag
	len -= 6;
	ret = emule_tag_list_from_buffer(&emule_pipe->_remote_peer._tag_list, &buffer, &len);
	sd_assert(ret == SUCCESS && len == 0);
	if(ret != SUCCESS)
		return ret;
	emule_pipe_attach_emule_device(emule_pipe, pipe_device);
	LOG_DEBUG("[emule_pipe = 0x%x]emule pipe recv HELLO cmd.", emule_pipe);
    emule_log_print_user_id(user_id);
	sd_assert(emule_pipe->_data_pipe._dispatch_info._pipe_state == PIPE_CONNECTING);
	emule_pipe_change_state(emule_pipe, PIPE_CONNECTED);
	return emule_pipe_send_hello_answer_cmd(emule_pipe);
}

_int32 emule_handle_hello_answer_cmd(EMULE_DATA_PIPE* emule_pipe, char* buffer, _int32 len)
{
	_int32 ret = SUCCESS;
	const EMULE_TAG* tag = NULL;
	_u16 kad_port = 0;
	_u8 kad_ver = 0;
	LOG_DEBUG("[emule_pipe = 0x%x]emule pipe recv HELLO_ANSWER cmd.", emule_pipe);
	sd_get_bytes(&buffer, &len, (char*)emule_pipe->_remote_peer._user_id, USER_ID_SIZE);
	sd_get_int32_from_lt(&buffer, &len, (_int32*)&emule_pipe->_remote_peer._client_id);
	sd_get_int16_from_lt(&buffer, &len, (_int16*)&emule_pipe->_remote_peer._tcp_port);
	ret = emule_tag_list_from_buffer(&emule_pipe->_remote_peer._tag_list, &buffer, &len);
	if(ret != SUCCESS)
		return ret;
	sd_get_int32_from_lt(&buffer, &len, (_int32*)&emule_pipe->_remote_peer._server_ip);
	ret = sd_get_int16_from_lt(&buffer, &len, (_int16*)&emule_pipe->_remote_peer._server_port);
	sd_assert(ret == SUCCESS);
	//len不一定为0，后面一些字段为版本信息
	tag = emule_tag_list_get(&emule_pipe->_remote_peer._tag_list, CT_EMULE_UDPPORTS);
	if(tag != NULL)
	{
		sd_assert(emule_tag_is_u32(tag) == TRUE);
		emule_pipe->_remote_peer._udp_port = (_u16)tag->_value._val_u32;
		kad_port = tag->_value._val_u32 >> 16;		
	}
	if(emule_enable_kad() && IS_HIGHID(emule_pipe->_remote_peer._client_id))
	{
		tag = emule_tag_list_get(&emule_pipe->_remote_peer._tag_list, CT_EMULE_MISCOPTIONS2);
		if(tag != NULL)
		{
			sd_assert(emule_tag_is_u32(tag) == TRUE);
			kad_ver = (_u8)GET_MISC_OPTION(tag->_value._val_u32, KADVER);
		}
#ifdef _DK_QUERY
		res_query_add_kad_node(emule_pipe->_remote_peer._client_id, kad_port, kad_ver);
#endif
    }
	return ret;
}


_int32 emule_handle_sending_part_cmd(EMULE_DATA_PIPE* emule_pipe, char* buffer, _int32 len, BOOL is_64_offset)
{
	_int32 ret = SUCCESS;
	_u8	file_id[FILE_ID_SIZE];
	_u32 start_pos = 0, end_pos = 0;
	_u64 start_pos64 = 0, end_pos64 = 0;
	EMULE_DATA_MANAGER* data_manager = (EMULE_DATA_MANAGER*)emule_pipe->_data_pipe._p_data_manager;
	sd_get_bytes(&buffer, &len, (char*)file_id, FILE_ID_SIZE);
	if(sd_memcmp(file_id, data_manager->_file_id, FILE_ID_SIZE) != 0)
		return -1;
	if(is_64_offset)
	{
		sd_get_int64_from_lt(&buffer, &len, (_int64*)&start_pos64);
		ret = sd_get_int64_from_lt(&buffer, &len, (_int64*)&end_pos64);
		sd_assert(start_pos64 < end_pos64 && len == 0 && ret == SUCCESS);
	}
	else
	{
		sd_get_int32_from_lt(&buffer, &len, (_int32*)&start_pos);
		ret = sd_get_int32_from_lt(&buffer, &len, (_int32*)&end_pos);
		sd_assert(start_pos <= end_pos && len == 0 && ret == SUCCESS);
		start_pos64 = start_pos;
		end_pos64 = end_pos;
	}
	//可能出现start_pos 和end_pos相同的情况
	if(start_pos64 == end_pos64)
	{
		//接收下一个命令包
		emule_pipe->_device->_recv_buffer_offset = 0;
		return emule_pipe_device_recv_cmd(emule_pipe->_device, EMULE_HEADER_SIZE + 1);
	}
	if(emule_pipe->_device->_data_pos + emule_pipe->_device->_data_buffer_offset == start_pos64)
		emule_pipe->_device->_valid_data = TRUE;
	else
		emule_pipe->_device->_valid_data = FALSE;
	ret = emule_pipe_device_recv_data(emule_pipe->_device, (_u32)(end_pos64 - start_pos64));
	LOG_DEBUG("[emule_pipe = 0x%x]emule pipe recv PART_DATA cmd, data[%llu, %llu].", emule_pipe, start_pos64, end_pos64);
	return ret;
}

_int32 emule_handle_aich_hash_ans_cmd(EMULE_DATA_PIPE* emule_pipe, char* buffer, _int32 len)
{
	_u8	file_id[FILE_ID_SIZE];
	_u8	aich_hash[AICH_HASH_SIZE];
#ifdef _LOGGER
    char test_buffer[1024];
#endif

    EMULE_DATA_MANAGER* data_manager = (EMULE_DATA_MANAGER*)emule_pipe->_data_pipe._p_data_manager;
	LOG_DEBUG("[emule_pipe = 0x%x]emule pipe recv OP_AICHFILEHASHANS cmd.", emule_pipe);

    sd_get_bytes(&buffer, &len, (char*)file_id, FILE_ID_SIZE);
	if(sd_memcmp(file_id, data_manager->_file_id, FILE_ID_SIZE) != 0 )
	{
       return SUCCESS;
	}
    
    sd_get_bytes(&buffer, &len, (char*)aich_hash, AICH_HASH_SIZE);
#ifdef _LOGGER
    sd_memset(test_buffer,0,1024);
    str2hex( (char*)aich_hash, AICH_HASH_SIZE, test_buffer, 1024);
    LOG_DEBUG("[emule_pipe = 0x%x]emule pipe recv OP_AICHFILEHASHANS cmd.aich_hash[%u]=%s", 
     emule_pipe, AICH_HASH_SIZE, test_buffer);
#endif

    emule_set_aich_hash(data_manager, aich_hash, AICH_HASH_SIZE);
    return SUCCESS;
}

_int32 emule_handle_aich_hash_req_cmd(EMULE_DATA_PIPE* emule_pipe, char* buffer, _int32 len)
{
    return emule_pipe_send_aich_answer_cmd(emule_pipe);
}

_int32 emule_handle_request_parts_cmd(EMULE_DATA_PIPE* emule_pipe, char* buffer, _int32 len, BOOL is_64_offset)
{
	_int32 ret = SUCCESS;
	_u8	file_id[FILE_ID_SIZE];
	_u32 start_pos1 = 0, start_pos2 = 0, start_pos3 = 0, end_pos1 = 0, end_pos2 = 0, end_pos3 = 0;
	_u64 start_pos64_1 = 0, start_pos64_2 = 0, start_pos64_3 = 0, end_pos64_1 = 0, end_pos64_2 = 0, end_pos64_3 = 0;
	EMULE_DATA_MANAGER* data_manager = (EMULE_DATA_MANAGER*)emule_pipe->_data_pipe._p_data_manager;
    // 不给对方上传，如果有请求的数据，直接返回之！
    return SUCCESS;
	if(emule_pipe->_upload_device == NULL)
		return SUCCESS;		//对方很狡猾，不请求上次，直接就来要数据，应该阻止
	sd_get_bytes(&buffer, &len, (char*)file_id, FILE_ID_SIZE);
	if(sd_memcmp(file_id, data_manager->_file_id, FILE_ID_SIZE) != 0)
		return -1;
	if(is_64_offset)
	{
		sd_get_int64_from_lt(&buffer, &len, (_int64*)&start_pos64_1);
		sd_get_int64_from_lt(&buffer, &len, (_int64*)&start_pos64_2);
		sd_get_int64_from_lt(&buffer, &len, (_int64*)&start_pos64_3);
		sd_get_int64_from_lt(&buffer, &len, (_int64*)&end_pos64_1);
		sd_get_int64_from_lt(&buffer, &len, (_int64*)&end_pos64_2);
		ret = sd_get_int64_from_lt(&buffer, &len, (_int64*)&end_pos64_3);
		sd_assert(ret == SUCCESS && len == 0);
	}
	else
	{
		sd_get_int32_from_lt(&buffer, &len, (_int32*)&start_pos1);
		sd_get_int32_from_lt(&buffer, &len, (_int32*)&start_pos2);
		sd_get_int32_from_lt(&buffer, &len, (_int32*)&start_pos3);
		sd_get_int32_from_lt(&buffer, &len, (_int32*)&end_pos1);
		sd_get_int32_from_lt(&buffer, &len, (_int32*)&end_pos2);
		ret = sd_get_int32_from_lt(&buffer, &len, (_int32*)&end_pos3);
		sd_assert(ret == SUCCESS && len == 0);
		start_pos64_1 = start_pos1;
		start_pos64_2 = start_pos2;
		start_pos64_3 = start_pos3;
		end_pos64_1 = end_pos1;
		end_pos64_2 = end_pos2;
		end_pos64_3 = end_pos3;
	}
	LOG_DEBUG("[emule_pipe = 0x%x]emule pipe recv REQUEST_PARTS cmd, req1[%llu, %llu], req2[%llu, %llu], req3[%llu, %llu].", emule_pipe,
				start_pos64_1, end_pos64_1, start_pos64_2, end_pos64_2, start_pos64_3, end_pos64_3);
	ret = emule_upload_recv_request(emule_pipe->_upload_device, start_pos64_1, end_pos64_1, start_pos64_2, end_pos64_2, start_pos64_3, end_pos64_3);
	return ret;
}

_int32 emule_handle_set_req_fileid_cmd(EMULE_DATA_PIPE* emule_pipe, char* buffer, _int32 len)
{
	_u8 file_id[FILE_ID_SIZE] = {0};
	EMULE_DATA_MANAGER* data_manager = (EMULE_DATA_MANAGER*)emule_pipe->_data_pipe._p_data_manager;
	LOG_DEBUG("[emule_pipe = 0x%x]emule pipe recv SET_REQ_FILE_ID cmd.", emule_pipe);
    sd_assert(data_manager != NULL);
	sd_get_bytes(&buffer, &len, (char*)file_id, FILE_ID_SIZE);
	if(sd_memcmp(file_id, data_manager->_file_id, FILE_ID_SIZE) != 0)
		return -1;
	return SUCCESS;
}

_int32 emule_handle_start_upload_req_cmd(EMULE_DATA_PIPE* emule_pipe, char* buffer, _int32 len)
{
	_u8 file_id[FILE_ID_SIZE] = {0};
	EMULE_DATA_MANAGER* data_manager = (EMULE_DATA_MANAGER*)emule_pipe->_data_pipe._p_data_manager;
    sd_assert(data_manager != NULL);
	LOG_DEBUG("[emule_pipe = 0x%x]emule pipe recv START_UPLOAD_REQ cmd.", emule_pipe);
	sd_get_bytes(&buffer, &len, (char*)file_id, FILE_ID_SIZE);
	if(sd_memcmp(file_id, data_manager->_file_id, FILE_ID_SIZE) != 0)
		return -1;
#ifdef UPLOAD_ENABLE
    ulm_add_pipe_by_file_hash( file_id, (DATA_PIPE *)emule_pipe );
#endif

    return SUCCESS;
}

_int32 emule_handle_accept_upload_req_cmd(EMULE_DATA_PIPE* emule_pipe)
{
	_int32 ret = SUCCESS;
    EMULE_DATA_MANAGER* data_manager = (EMULE_DATA_MANAGER*)emule_pipe->_data_pipe._p_data_manager;
    RESOURCE *emule_res = emule_pipe->_data_pipe._p_resource;
    CONNECT_MANAGER *connect_manager = NULL;
    _u8 gcid[CID_SIZE] = {0};
    _u32 file_idx = INVALID_FILE_INDEX;

	LOG_DEBUG("[emule_pipe = 0x%x]emule pipe recv ACCEPT_UPLOAD_REQUEST cmd, rank = %u.", 
        emule_pipe, emule_pipe->_rank);

	if(emule_pipe->_rank != MAX_U32)
	{
		ret = emule_download_queue_remove(emule_pipe);
	}
	emule_pipe_change_state(emule_pipe, PIPE_DOWNLOADING);

    // 只有是被动过来的才需要将其插入到调度器中
    if (emule_pipe->_device->_passive == TRUE)
    {
        LOG_DEBUG("data_manager is 0x%x, emule_res = 0x%x, insert passive peer into dispatcher.",
            data_manager, emule_res);
        if ((data_manager != NULL) && (emule_res != NULL))
        {
            emule_get_gcid(data_manager, gcid);
            connect_manager = tm_get_task_connect_manager(gcid, &file_idx, data_manager->_file_id);
            sd_assert(connect_manager != NULL);

            ret = cm_add_passive_peer(connect_manager, file_idx, emule_res, &(emule_pipe->_data_pipe));
        }
        else
        {
            LOG_ERROR("passive peer data_manager is NULL!");
            sd_assert(FALSE);
        }
    }

	return SUCCESS;
}

_int32 emule_handle_cancel_transfer_cmd(EMULE_DATA_PIPE* emule_pipe)
{
	LOG_DEBUG("[emule_pipe = 0x%x]emule pipe recv CANCEL_TRANSFER cmd.", emule_pipe);
	return SUCCESS;
}


//========================================================================================
//以下是emule扩展命令
//========================================================================================
_int32 emule_handle_answer_sources_cmd(EMULE_DATA_PIPE* emule_pipe, char* buffer, _int32 len)
{
	_int32 ret = SUCCESS;
	_u16 source_count = 0, i = 0, client_port = 0, server_port = 0;
	_u8	file_id[FILE_ID_SIZE], crypt_option = 0;
	_u8 user_id[USER_ID_SIZE];
	_u32 client_id = 0, server_ip = 0;
	_u8 source_exchange_ver = emule_peer_get_source_exchange_option(&emule_pipe->_remote_peer);
	EMULE_DATA_MANAGER* data_manager = (EMULE_DATA_MANAGER*)emule_pipe->_data_pipe._p_data_manager;
	sd_get_bytes(&buffer, &len, (char*)file_id, FILE_ID_SIZE);
	if(sd_memcmp(file_id, data_manager->_file_id, FILE_ID_SIZE) != 0)
		return -1;
	sd_get_int16_from_lt(&buffer, &len, (_int16*)&source_count);
	if(source_count * 12 == len)
	{
		if(source_exchange_ver < 1)
			return SUCCESS;
		source_exchange_ver = 1;
	}
	else if(source_count * 28 == len)
	{
		if(source_exchange_ver < 2 || source_exchange_ver > 3)
			return SUCCESS;
	}
	else if(source_count * 29 == len)
	{
		if(source_exchange_ver < 4)
			return SUCCESS;
    	}
	else
	{
		return SUCCESS;
	}
	LOG_DEBUG("[emule_pipe = 0x%x]emule pipe recv ANSWER_SOURCES cmd, source_count = %u.", emule_pipe, (_u32)source_count);
	for(i = 0; i < source_count; ++i)
	{
		sd_get_int32_from_lt(&buffer, &len, (_int32*)&client_id);
		sd_get_int16_from_lt(&buffer, &len, (_int16*)&client_port);
		sd_get_int32_from_lt(&buffer, &len, (_int32*)&server_ip);
		ret = sd_get_int16_from_lt(&buffer, &len, (_int16*)&server_port);
		if(source_exchange_ver > 1)
			ret = sd_get_bytes(&buffer, &len, (char*)user_id, USER_ID_SIZE);
		if(source_exchange_ver >= 4)
			ret = sd_get_int8(&buffer, &len, (_int8*)&crypt_option);	
		// 来源交换3.0以上版本发送的是客户端hybrid
		// hybrid计算的方法是，如果是低id,hybrid=lowid,如果是高id,hybrid=ntohl(hightid)
		if(IS_HIGHID(client_id) && source_exchange_ver >= 3)
			client_id = sd_htonl(client_id);	
		cm_add_emule_resource(&data_manager->_emule_task->_task._connect_manager, client_id, client_port, server_ip, server_port);
	}
	sd_assert(ret == SUCCESS && len == 0);
	return ret;
}

_int32 emule_handle_secure_key_and_signed_state()
{
    _int32 ret = SUCCESS;
    
//     // 获取本地的公钥
//     len = get_local_peer_public_key(&key);
// 
//     // 发送之
//     send_public_key(key);

    return ret;
}

_int32 emule_handle_secure_signature_needed_state()
{
    _int32 ret = SUCCESS;

//     // 设置对方发过来的挑战值
//     set_recvd_challenge(challenge);
// 
//     // 获取对应peer的公钥
//     get_client_peer_public_key(&key);
// 
//     if (ken_len == 0)
//     {
//         // 需要等待到对方的公钥获取到之后再发送这个回应
//         // 同时向对方发起获取key的命令
//         set_secure_ident_state(IS_SIGNATURENEEDED);
//         send_secure_ident(IS_KEYANDSIGNEEDED, challenge);
//         return;
//     }
// 
//     if (get_secure_ident_option() == 2)
//     {
//         if (IS_LOWID(local_client_id))
//         {
//             challenge_ip_kind = CRYPT_CIP_REMOTECLIENT;
//             challenge_ip = rmt_peer_ip;
//         }
//         else
//         {
//             challenge_ip_kind = CRYPT_CIP_LOCALCLIENT;
//             challenge_ip = local_client_id;
//         }
//     }
// 
//     create_signature();
// 
//     send_signature();
// 
//     set_secure_ident_state(IS_ALLREQUESTSSEND);

    return ret;
}

_int32 emule_handle_secident_state_cmd(EMULE_DATA_PIPE* emule_pipe, char* buffer, _int32 len)
{
	_int32 ret = SUCCESS;
	_u8 secure_state = IS_UNAVAILABLE;
	_u32 challenge = 0;
	sd_get_int8(&buffer, &len, (_int8*)&secure_state);
	ret = sd_get_int32_from_lt(&buffer, &len, (_int32*)&challenge);
	sd_assert(ret == SUCCESS && len == 0);

    LOG_DEBUG("emule_handle_secident_state_cmd, secure_state = %hhu, challenge = %u.", secure_state, challenge);

    switch (secure_state)
    {
    case IS_KEYANDSIGNEEDED:
        emule_handle_secure_key_and_signed_state();
        break;
    case IS_SIGNATURENEEDED:
        emule_handle_secure_signature_needed_state();
        break;
    default:
        break;
    }

	return ret;
}


#endif /* ENABLE_EMULE */
#endif /* ENABLE_EMULE */

