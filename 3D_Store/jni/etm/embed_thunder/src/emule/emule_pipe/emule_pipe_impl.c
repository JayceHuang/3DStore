#include "utility/define.h"

#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL
#include "utility/mempool.h"
#include "utility/utility.h"
#include "utility/bytebuffer.h"
#include "utility/time.h"
#include "utility/logid.h"
#define LOGID LOGID_EMULE
#include "utility/logger.h"

#include "emule_pipe_impl.h"
#include "emule_pipe_cmd.h"
#include "emule_pipe_device.h"
#include "emule_pipe_handle_cmd.h"
#include "emule_pipe_upload.h"
#include "../emule_data_manager/emule_data_manager.h"
#include "../emule_utility/emule_define.h"
#include "../emule_utility/emule_opcodes.h"
#include "../emule_utility/emule_setting.h"
#include "../emule_interface.h"
#include "../emule_impl.h"
#include "../emule_utility/emule_utility.h"
#include "../emule_task_manager.h"
#include "connect_manager/connect_manager_interface.h"

#include "task_manager/task_manager.h"

#ifdef UPLOAD_ENABLE
#include "upload_manager/upload_manager.h"
#include "upload_manager/upload_control.h"
#endif

#define	EMULE_REQ_NUM			10

void emule_pipe_attach_emule_device(EMULE_DATA_PIPE* emule_pipe, EMULE_PIPE_DEVICE* pipe_device)
{
	LOG_DEBUG("[emule_pipe = 0x%x]emule_pipe(0x%x) attach emule_device(0x%x).", emule_pipe, emule_pipe, pipe_device);
	emule_pipe->_device = pipe_device;
	pipe_device->_pipe = emule_pipe;
}

void emule_pipe_detach_emule_device(EMULE_DATA_PIPE* emule_pipe, EMULE_PIPE_DEVICE* pipe_device)
{
	LOG_DEBUG("[emule_pipe = 0x%x]emule_pipe(0x%x) detach emule_device(0x%x).", emule_pipe, emule_pipe, pipe_device);
	emule_pipe->_device = NULL;
	pipe_device->_pipe = NULL;
}

_int32 emule_pipe_notify_connnect(EMULE_PIPE_DEVICE* emule_device)
{
	_int32 ret = SUCCESS;
	EMULE_DATA_PIPE* emule_pipe = emule_device->_pipe;
	if(emule_device->_passive == FALSE)
	{
		emule_pipe_change_state(emule_pipe, PIPE_CONNECTED);
		LOG_DEBUG("[emule_pipe = 0x%x]emule pipe connect success.", emule_pipe);
		ret = emule_pipe_send_hello_cmd(emule_pipe);
		CHECK_VALUE(ret);
	}
	else
	{
		sd_assert(emule_pipe == NULL);
	}
	return emule_pipe_device_recv_cmd(emule_pipe->_device, EMULE_HEADER_SIZE + 1);
}

_int32 emule_pipe_notify_handshake(EMULE_DATA_PIPE* emule_pipe)
{
	_int32 ret = SUCCESS;
	const EMULE_TAG* tag = NULL;
	_u8 support_sec_ident = 0;

    LOG_DEBUG("emule_pipe_notify_handshake, emule_pipe = 0x%x, is_passive = %d.",
        emule_pipe, emule_pipe->_device->_passive);    

    tag = emule_tag_list_get(&emule_pipe->_remote_peer._tag_list, CT_EMULE_MISCOPTIONS1);
    if(tag != NULL)
    {
        sd_assert(emule_tag_is_u32(tag) == TRUE);
        support_sec_ident = GET_MISC_OPTION(tag->_value._val_u32, SECUREIDENT);
    }
    if(support_sec_ident > 0)
    {
        //return emule_pipe_send_secident_state(emule_pipe);
    }

    LOG_DEBUG("[emule_pipe = 0x%x]emule_pipe_notify_handshake.", emule_pipe);

    if (emule_pipe->_device->_passive == FALSE)
    {
        // 如果是被动连过来的，不主动发包
        ret = emule_pipe_send_req_filename_cmd(emule_pipe);
        ret = emule_pipe_send_req_file_id_cmd(emule_pipe);
        if(emule_enable_source_exchange() == TRUE)
        {
            ret = emule_pipe_send_req_source_cmd(emule_pipe);
        }
    }

	return ret;
}

_int32 emule_pipe_notify_recv_part_data(EMULE_DATA_PIPE* emule_pipe, const RANGE* down_range)
{
	_int32 ret = SUCCESS;
	LOG_DEBUG("[emule_pipe = 0x%x]emule pipe commit range[%u, %u].", emule_pipe, down_range->_index, down_range->_num);
	ret = pi_put_data(&emule_pipe->_data_pipe, down_range, &emule_pipe->_device->_data_buffer, emule_pipe->_device->_data_buffer_offset, 
					emule_pipe->_device->_data_buffer_len, emule_pipe->_device->_pipe->_data_pipe._p_resource);
	CHECK_VALUE(ret);
	++emule_pipe->_data_pipe._dispatch_info._down_range._index;
	--emule_pipe->_data_pipe._dispatch_info._down_range._num;
	emule_pipe->_device->_data_pos = (_u64)get_data_unit_size() * emule_pipe->_data_pipe._dispatch_info._down_range._index;
	return emule_pipe_request_data(emule_pipe);
}

void emule_pipe_change_state(EMULE_DATA_PIPE* emule_pipe, PIPE_STATE state)
{
#ifdef _DEBUG
	char pipe_state_str[7][17] = {"PIPE_IDLE", "PIPE_CONNECTING", "PIPE_CONNECTED", "PIPE_CHOKED", "PIPE_DOWNLOADING", "PIPE_FAILURE", "PIPE_SUCCESS"};
	LOG_DEBUG("[emule_pipe = 0x%x]emule_pipe_change_state, from %s to %s", emule_pipe, pipe_state_str[emule_pipe->_data_pipe._dispatch_info._pipe_state], pipe_state_str[state]);
#endif
	if((state == PIPE_CONNECTING || state == PIPE_CHOKED || state == PIPE_DOWNLOADING) && emule_pipe->_data_pipe._dispatch_info._pipe_state != state)
		sd_time_ms(&emule_pipe->_data_pipe._dispatch_info._time_stamp);	
    //emule_pipe->_data_pipe._dispatch_info._pipe_state = state;
    dp_set_state(&emule_pipe->_data_pipe, state);
}

_int32 emule_pipe_request_data(EMULE_DATA_PIPE* emule_pipe)
{
	_u32 need_range_num = 0;
	RANGE uncomplete_range, down_range, new_range;
    
    LOG_DEBUG("[emule_pipe = 0x%x]emule_pipe_request_data.", emule_pipe);

	dp_get_download_range(&emule_pipe->_data_pipe, &down_range);
	if(down_range._num == EMULE_REQ_NUM )
		return SUCCESS;			//目前有数据正满负荷下载
	dp_get_uncomplete_ranges_head_range(&emule_pipe->_data_pipe, &uncomplete_range);
	if(uncomplete_range._num == 0)
	{
		LOG_DEBUG("[emule_pipe = 0x%x]emule_pipe_request_data, but uncomplete ranges is empty, no more range to request.just wait change ranges", emule_pipe);
		return pi_notify_dispatch_data_finish(&emule_pipe->_data_pipe);
	} 
	if(down_range._num == 0)
	{
		need_range_num = EMULE_REQ_NUM;
		down_range._index = uncomplete_range._index;
	}
	else if(down_range._index + down_range._num == uncomplete_range._index)
		need_range_num = EMULE_REQ_NUM - down_range._num;				//是连续的块
	need_range_num = MIN(need_range_num, uncomplete_range._num);
	if(need_range_num >= 4 || down_range._num == 0)
	{
		new_range._index = down_range._index + down_range._num;
		new_range._num = need_range_num;
		down_range._num += need_range_num;
		emule_pipe_send_req_part_cmd(emule_pipe, &new_range);
		dp_set_download_range(&emule_pipe->_data_pipe, &down_range);
		dp_delete_uncomplete_ranges(&emule_pipe->_data_pipe, &new_range);		//把uncomplete_ranges调整对
		emule_pipe->_device->_data_pos = (_u64)get_data_unit_size() * down_range._index;
	}
	return SUCCESS;
}

_int32 emule_pipe_handle_error(EMULE_DATA_PIPE* emule_pipe, _int32 errcode)
{
	LOG_DEBUG("[emule_pipe = 0x%x]emule_pipe_handle_error, errcode = %d.", emule_pipe, errcode);

#ifdef UPLOAD_ENABLE
     emule_pipe->_data_pipe._dispatch_info._pipe_upload_state = UPLOAD_FAILED_STATE;
#endif

	if(emule_pipe->_data_pipe._dispatch_info._pipe_state == PIPE_CHOKED && errcode == 2249)
	{
		emule_pipe_device_close(emule_pipe->_device);
		emule_pipe->_device = NULL;
#ifdef UPLOAD_ENABLE
        ulm_cancel_read_data(&emule_pipe->_data_pipe);
        ulm_remove_pipe(&emule_pipe->_data_pipe);
#endif        
        emule_upload_device_close(&emule_pipe->_upload_device);
		//启动定时器每隔一段时间查询一下队列位置
		sd_assert(emule_pipe->_timeout_id == INVALID_MSG_ID);
		start_timer(emule_pipe_handle_timeout, NOTICE_INFINITE, 30 * 1000, 0, emule_pipe, &emule_pipe->_timeout_id);
		return SUCCESS;
	}

	emule_upload_device_close(&emule_pipe->_upload_device);
	emule_pipe_change_state(emule_pipe, PIPE_FAILURE);
	set_resource_err_code(emule_pipe->_data_pipe._p_resource, errcode);
	return SUCCESS;
}

_int32 emule_pipe_part_bitmap_to_can_download_ranges(EMULE_DATA_PIPE* emule_pipe, char** buffer, _int32* buffer_len)
{
	_int32 ret = SUCCESS;
	_u16 count = 0;
	_u32 index = 0;
	_u64 part_pos = 0, part_len = 0, pos = 0, len = 0;
	RANGE range;
	BITMAP part_bitmap;
	EMULE_DATA_MANAGER* data_manager = (EMULE_DATA_MANAGER*)emule_pipe->_data_pipe._p_data_manager;
	LOG_DEBUG("[emule_pipe = 0x%x]emule_pipe_part_bitmap_to_can_download_ranges.", emule_pipe);
	sd_get_int16_from_lt(buffer, buffer_len, (_int16*)&count);
	bitmap_init(&part_bitmap);
	//如果count为0则表示是种子
	if(count == 0)
	{
		count = emule_get_part_count(data_manager);
		ret = bitmap_resize(&part_bitmap, count);
		CHECK_VALUE(ret);
		sd_memset(part_bitmap._bit, 0xFF, part_bitmap._mem_size); 
	}
	else
	{
		ret = bitmap_resize(&part_bitmap, count);
		CHECK_VALUE(ret);
		ret = sd_get_bytes(buffer, buffer_len, (char*)part_bitmap._bit, part_bitmap._mem_size);
		if(ret != SUCCESS)
			return ret;
		sd_assert(len == 0);
	}

	for(index = 0; index < part_bitmap._bit_count; ++index)
	{
		if(bitmap_emule_at(&part_bitmap, index) == TRUE)
		{
			part_pos = EMULE_PART_SIZE * index;
			part_len = EMULE_PART_SIZE;
			if(index == part_bitmap._bit_count - 1)
				part_len = emule_get_part_size(data_manager, index);
			if(len == 0)
			{
				pos = part_pos;
				len = part_len;
			}
			else
			{
				len += part_len;
			}
		}
		else if(len != 0)
		{
			range = pos_length_to_range2(pos, len, data_manager->_file_size);
			if(range._num > 0)
			{
				LOG_DEBUG("[emule_pipe = 0x%x]add can download range[%u, %u].",emule_pipe, range._index, range._num);
				range_list_add_range(&emule_pipe->_data_pipe._dispatch_info._can_download_ranges, &range, NULL, NULL);
				len = 0;		//初始状态
			}
		}
	}
	if(len != 0)
	{	//最后一块有效
		range = pos_length_to_range2(pos, len, data_manager->_file_size);
		if(range._num > 0)
		{
			LOG_DEBUG("[emule_pipe = 0x%x]add can download range[%u, %u].",emule_pipe, range._index, range._num);
			range_list_add_range(&emule_pipe->_data_pipe._dispatch_info._can_download_ranges, &range, NULL, NULL);
			len = 0;		//初始状态
		}
	}
	return bitmap_uninit(&part_bitmap);
}

_int32 emule_handle_recv_edonkey_cmd(EMULE_PIPE_DEVICE* emule_device, char* buffer, _int32 len)
{
	_int32 ret = SUCCESS;
	_u8 opcode = 0;
	EMULE_DATA_PIPE* emule_pipe = emule_device->_pipe;
	sd_get_int8(&buffer, &len, (_int8*)&opcode);

    LOG_DEBUG("[emule_pipe = 0x%x]emule_handle_recv_edonkey_cmd, opcode = %hhu.", emule_pipe, opcode);
	switch(opcode)
	{
		case OP_HELLO:
			if(emule_device->_pipe != NULL)
				break;			//主动连接却收到对方的hello请求，不处理该命令，对方版本可能有bug
			ret = emule_handle_hello_cmd(emule_device, buffer, len);
			if(ret != SUCCESS) return ret;
 			ret = emule_pipe_notify_handshake(emule_device->_pipe);
 			if(ret != SUCCESS) return ret;
            break;
		case OP_SENDINGPART:
			return emule_handle_sending_part_cmd(emule_pipe, buffer, len, FALSE);
		case OP_REQUESTPARTS:
			return emule_handle_request_parts_cmd(emule_pipe, buffer, len, FALSE);
		case OP_FILEREQANSNOFIL:
			return emule_handle_file_not_found_cmd(emule_pipe, buffer, len);
		case OP_HELLOANSWER:
			ret = emule_handle_hello_answer_cmd(emule_pipe, buffer, len);
			if(ret != SUCCESS) return ret;
			emule_pipe_notify_handshake(emule_pipe);
			//emule_pipe_send_file_state_cmd(emule_pipe);//verycd不会主动发OP_SETREQFILEID,所以不发这个就不会给verycd上传
			break;
		case OP_SETREQFILEID:
			ret = emule_handle_set_req_fileid_cmd(emule_pipe, buffer, len);
			emule_pipe_send_file_state_cmd(emule_pipe);
			break;
		case OP_FILESTATUS:
			return emule_handle_file_status_cmd(emule_pipe, buffer, len);
		case OP_HASHSETREQUEST:
			ret = emule_handle_part_hash_request_cmd(emule_pipe, buffer, len);
			if(ret == SUCCESS)
				emule_pipe_send_part_hash_answer_cmd(emule_pipe);
			break;
		case OP_HASHSETANSWER:
			return emule_handle_part_hash_answer_cmd(emule_pipe, buffer, len);
		case OP_STARTUPLOADREQ:
			ret = emule_handle_start_upload_req_cmd(emule_pipe, buffer, len);
			if(ret != SUCCESS) return ret;
#ifdef UPLOAD_ENABLE
            if(ulc_enable_upload())
            {
    			if(emule_pipe->_upload_device == NULL);
    			{
    				//有可能收到两个OP_STARTUPLOADREQ 命令
    				ret = emule_upload_device_create(&emule_pipe->_upload_device, emule_pipe);
    				if(ret != SUCCESS) return ret;
    			}
    			ret = emule_pipe_send_accept_upload_req_cmd(emule_pipe, TRUE);
    	    }
#endif
            break;
		case OP_ACCEPTUPLOADREQ:
			//有可能连续收到两个OP_ACCEPTUPLOADREQ命令
			if(emule_pipe->_data_pipe._dispatch_info._pipe_state == PIPE_DOWNLOADING)
				break;	
			return emule_handle_accept_upload_req_cmd(emule_pipe);
        case OP_CANCELTRANSFER:
			emule_handle_cancel_transfer_cmd(emule_pipe);
#ifdef UPLOAD_ENABLE
            ulm_cancel_read_data(&emule_pipe->_data_pipe);
            ulm_remove_pipe(&emule_pipe->_data_pipe);
#endif       
			ret = emule_upload_device_close(&emule_pipe->_upload_device);
			break;
		case OP_OUTOFPARTREQS:
			return emule_handle_out_of_part_req_cmd(emule_pipe);
		case OP_REQUESTFILENAME:
			return emule_handle_request_filename_cmd(emule_pipe, buffer, len);
		case OP_REQFILENAMEANSWER:
			return emule_handle_req_filename_answer_cmd(emule_pipe, buffer, len);
		case OP_QUEUERANK:
			//因为迅雷5会在发送accept_upload后紧接着发送QUEUE_RANK,所以目前先不处理这个
			//命令，以后再留意怎么改更合理
			if(emule_pipe->_data_pipe._dispatch_info._pipe_state == PIPE_DOWNLOADING)
				break;
			return emule_handle_queue_rank_cmd(emule_pipe, buffer, len);
		case OP_SENDINGPART_I64:	//本来这个是emule的协议，但发现网络上还真有一些客户端把它填为edonkey协议
			return emule_handle_sending_part_cmd(emule_pipe, buffer, len, TRUE);
		case OP_ASKSHAREDDIRS:
			LOG_ERROR("[emule_pipe = 0x%x]emule_handle_recv_edonkey_cmd, opcode = %d, not process now.", emule_pipe, opcode);
			break;
		default:
			LOG_ERROR("[emule_pipe = 0x%x]emule_handle_recv_edonkey_cmd, opcode = %d, not process now.", emule_pipe, opcode);
	}
	return ret;			
}

_int32 emule_handle_recv_emule_cmd(EMULE_DATA_PIPE* emule_pipe, char* buffer, _int32 len)
{
	_u8 opcode = 0;
	sd_get_int8(&buffer, &len, (_int8*)&opcode);
    LOG_DEBUG("[emule_pipe = 0x%x]emule_handle_recv_emule_cmd, opcode = %d.", emule_pipe, opcode);
	switch(opcode)
	{
		case OP_EMULEINFO:
			return emule_handle_emule_info_cmd(emule_pipe, buffer, len);
		case OP_REQFILENAMEANSWER:
			return emule_handle_req_filename_answer_cmd(emule_pipe, buffer, len);
		case OP_QUEUERANKING:
			return emule_handle_queue_ranking_cmd(emule_pipe, buffer, len);
		case OP_REQUESTSOURCES:
			return emule_handle_request_sources_cmd(emule_pipe, buffer, len);
		case OP_ANSWERSOURCES:
			return emule_handle_answer_sources_cmd(emule_pipe, buffer, len);
		case OP_SECIDENTSTATE:
			return emule_handle_secident_state_cmd(emule_pipe, buffer, len);
		case OP_SENDINGPART_I64:
			return emule_handle_sending_part_cmd(emule_pipe, buffer, len, TRUE);
		case OP_REQUESTPARTS_I64:
			return emule_handle_request_parts_cmd(emule_pipe, buffer, len, TRUE);
        case OP_AICHFILEHASHANS:
            return emule_handle_aich_hash_ans_cmd(emule_pipe, buffer, len);
        case OP_AICHFILEHASHREQ:
            return emule_handle_aich_hash_req_cmd(emule_pipe, buffer, len);
            
		case OP_SECALLBACK:
			LOG_ERROR("[emule_pipe = 0x%x]emule_handle_recv_emule_cmd, opcode = %d, not process now.", emule_pipe, opcode);
			break;
		case OP_TCPSENATREQ:
			LOG_ERROR("[emule_pipe = 0x%x]emule_handle_recv_emule_cmd, opcode = %d, not process now.", emule_pipe, opcode);
			break;
		default:
			LOG_ERROR("[emule_pipe = 0x%x]emule_handle_recv_emule_cmd, opcode = %d, not process now.", emule_pipe, opcode);
	}
	return SUCCESS;
}

_int32 emule_handle_recv_udp_cmd(char* buffer, _int32 len, _u32 ip, _u16 port)
{
	_u8 opcode = 0;
	_u32 offset = 0;
	EMULE_DATA_PIPE* emule_pipe = NULL;
	sd_get_int8(&buffer, &len, (_int8*)&opcode);
    LOG_DEBUG("emule_handle_recv_udp_cmd, opcode = %hhu.", opcode);
	switch(opcode)
	{
		case OP_REASKACK:
			emule_pipe = emule_download_queue_find(ip, port);
			if(emule_pipe != NULL)
			{
				//这里本来还有file_state字段的，这里没处理，直接跳过到最后的两个字节，表示rank
				sd_assert(len >= 2);
				offset = len - 2;
				buffer += offset;
				len = 2;
				emule_pipe->_rank = 0;
				sd_get_int16_from_lt(&buffer, &len, (_int16*)&emule_pipe->_rank);
				LOG_DEBUG("[emule_pipe = 0x%x]emule_pipe recv reask ack, and rank = %u.", emule_pipe, emule_pipe->_rank);
			}
			break;
		case OP_UDPTOUDPSENATREQ:
			LOG_ERROR("[emule_pipe = 0x%x]emule_pipe recv OP_UDPTOUDPSENATREQ cmd, but not process.", emule_pipe);
			break;
		default:
			LOG_ERROR("[emule_pipe = 0x%x]emule_pipe recv unknow udp cmd(op_code = %d), but not process.", emule_pipe, (_int32)opcode);
	}
	return SUCCESS;
}


_int32 emule_handle_file_not_found_cmd(EMULE_DATA_PIPE* emule_pipe, char* buffer, _int32 len)
{
	LOG_DEBUG("[emule_pipe = 0x%x]emule pipe recv FILE_NOT_FOUND cmd, close this pipe.", emule_pipe);
	return -1;
}

_int32 emule_handle_req_filename_answer_cmd(EMULE_DATA_PIPE* emule_pipe, char* buffer, _int32 len)
{
	_int32 ret = SUCCESS;
	_int16 file_name_len = 0;
	char file_name[512] = {0};
	_u8	file_id[FILE_ID_SIZE];
	EMULE_DATA_MANAGER* data_manager = (EMULE_DATA_MANAGER*)emule_pipe->_data_pipe._p_data_manager;
	LOG_DEBUG("[emule_pipe = 0x%x]emule pipe recv REQ_FILE_NAME_ANSWER cmd.", emule_pipe);
	sd_get_bytes(&buffer, &len, (char*)file_id, FILE_ID_SIZE);
	sd_get_int16_from_lt(&buffer, &len, &file_name_len);
	
    if(file_name_len>=512)
    {
        LOG_DEBUG("emule_handle_req_filename_answer_cmd: file_name_len %d >= 512 !",file_name_len);
        return EMULE_FILE_NAME_ERR;
    }
    
	ret = sd_get_bytes(&buffer, &len, file_name, file_name_len);
	if(ret != SUCCESS)
		return ret;
	return sd_memcmp(file_id, data_manager->_file_id, FILE_ID_SIZE);
}

_int32 emule_handle_file_status_cmd(EMULE_DATA_PIPE* emule_pipe, char* buffer, _int32 len)
{
	_int32 ret = SUCCESS;
	_u8 file_id[FILE_ID_SIZE];
	const RANGE_LIST* recvd_ranges = NULL;
	EMULE_DATA_MANAGER* data_manager = (EMULE_DATA_MANAGER*)emule_pipe->_data_pipe._p_data_manager;
    sd_assert(data_manager != NULL);
	LOG_DEBUG("[emule_pipe = 0x%x]emule pipe recv FILE_STATUS cmd.", emule_pipe);
	sd_get_bytes(&buffer, &len, (char*)file_id, FILE_ID_SIZE);
	if(sd_memcmp(file_id, data_manager->_file_id, FILE_ID_SIZE) != 0)
		return -1;

	emule_pipe_part_bitmap_to_can_download_ranges(emule_pipe, &buffer, &len);

	// 请求part hash,注意要检查can_download_ranges，如果为空的话说明对方没有part_hash，再请求part_hash
	//会被对方断开
	if(emule_is_part_hash_valid(data_manager) == FALSE 
       && range_list_size(&emule_pipe->_data_pipe._dispatch_info._can_download_ranges) > 0)
    {
		emule_pipe_send_part_hash_req_cmd(emule_pipe);
    }

	if(emule_is_aich_hash_valid(data_manager) == FALSE)
    {
		emule_pipe_send_arch_hash_req_cmd(emule_pipe);
    }

	dp_adjust_uncomplete_2_can_download_ranges(&emule_pipe->_data_pipe);

	// 有需要的数据就请求上传
	recvd_ranges = emule_get_recved_range_list(data_manager);
	if(range_list_is_contained(recvd_ranges, &emule_pipe->_data_pipe._dispatch_info._can_download_ranges) == FALSE
       && emule_pipe->_data_pipe._dispatch_info._pipe_state == PIPE_CONNECTED)
		ret = emule_pipe_send_start_upload_req_cmd(emule_pipe);
	return ret;
}

_int32 emule_handle_part_hash_request_cmd(EMULE_DATA_PIPE* emule_pipe, char* buffer, _int32 len)
{
	_int32 ret = SUCCESS;
	_u8	file_id[FILE_ID_SIZE];
	EMULE_DATA_MANAGER* data_manager = (EMULE_DATA_MANAGER*)emule_pipe->_data_pipe._p_data_manager;
	LOG_DEBUG("[emule_pipe = 0x%x]emule pipe recv PART_HASH_REQUEST cmd.", emule_pipe);
	ret = sd_get_bytes(&buffer, &len, (char*)file_id, FILE_ID_SIZE);
	sd_assert(ret == SUCCESS && len == 0);
	if(sd_memcmp(file_id, data_manager->_file_id, FILE_ID_SIZE) != 0)
		return -1;
	return ret;
}

_int32 emule_handle_part_hash_answer_cmd(EMULE_DATA_PIPE* emule_pipe, char* buffer, _int32 len)
{
	_u8	file_id[FILE_ID_SIZE];
	_u16 part_count = 0;
	EMULE_DATA_MANAGER* data_manager = (EMULE_DATA_MANAGER*)emule_pipe->_data_pipe._p_data_manager;
	LOG_DEBUG("[emule_pipe = 0x%x]emule pipe recv PART_HASH_ANSWER cmd.", emule_pipe);
	sd_get_bytes(&buffer, &len, (char*)file_id, FILE_ID_SIZE);
	if(sd_memcmp(file_id, data_manager->_file_id, FILE_ID_SIZE) != 0)
		return -1;
	sd_get_int16_from_lt(&buffer, &len, (_int16*)&part_count);
	sd_assert(part_count * 16 == len);
	return emule_set_part_hash(data_manager, (const _u8*)buffer, len);
}



_int32 emule_handle_out_of_part_req_cmd(EMULE_DATA_PIPE* emule_pipe)
{
	//OP_OUTOFPARTREQS will tell the downloading client to go back to OnQueue..
	//The main reason for this is that if we put the client back on queue and it goes
	//back to the upload before the socket times out... We get a situation where the
	//downloader thinks it already sent the requested blocks and the uploader thinks
	//the downloader didn't send any request blocks. Then the connection times out..
	//I did some tests with eDonkey also and it seems to work well with them also..
	LOG_DEBUG("[emule_pipe = 0x%x]emule pipe recv OUT_OF_PART_REQUEST cmd.", emule_pipe);
	emule_pipe_change_state(emule_pipe, PIPE_CHOKED);
	return SUCCESS;
}

_int32 emule_handle_request_filename_cmd(EMULE_DATA_PIPE* emule_pipe, char* buffer, _int32 len)
{
    _int32 ret = SUCCESS;
    _u8 gcid[CID_SIZE] = {0};
    CONNECT_MANAGER *connect_manager = NULL;
    _u32 file_idx = INVALID_FILE_INDEX;
    const RANGE_LIST* recvd_ranges = NULL;
	_u8 file_id[FILE_ID_SIZE] = {0};
	_u16 complete_source_count = 0;
	_u8 request_ver = emule_peer_get_extended_requests_option(&emule_pipe->_remote_peer);
	EMULE_DATA_MANAGER* data_manager = (EMULE_DATA_MANAGER*)emule_pipe->_data_pipe._p_data_manager;
	LOG_DEBUG("[emule_pipe = 0x%x]emule pipe recv OP_REQUESTFILENAME cmd, request_ver = %hhu.", 
        emule_pipe, request_ver);

	sd_get_bytes(&buffer, &len, (char*)file_id, FILE_ID_SIZE);
    emule_log_print_file_id(file_id, "in emule_handle_request_filename_cmd");

    if (data_manager == NULL)
    {
        // 对于被动连接的源，需要将其和相应的任务关联起来
        LOG_DEBUG("emule_data_manager is NULL.");

        emule_get_data_manager_by_file_id(file_id, &data_manager);
        if (data_manager == NULL)
        {
            LOG_ERROR("there is no data manager equal to file_id");
            emule_pipe_device_notify_error(emule_pipe->_device, EMULE_NO_DATA_MANAGER);
            return EMULE_NO_DATA_MANAGER;
        }
        LOG_DEBUG("get emule_data_manger = 0x%x.", data_manager);
        emule_pipe->_data_pipe._p_data_manager = data_manager;

        emule_get_gcid(data_manager, gcid);
        emule_log_print_gcid(gcid, "emule_handle_request_filename_cmd");

        connect_manager = tm_get_task_connect_manager(gcid, &file_idx, file_id);
        sd_assert(connect_manager != NULL);
        LOG_DEBUG("get emule connect manager = 0x%x.", connect_manager);
        dp_set_pipe_interface(&(emule_pipe->_data_pipe), &(connect_manager->_pipe_interface));

#ifdef UPLOAD_ENABLE
        if (ulm_is_file_exist(gcid, emule_get_file_size((void *)data_manager)) == TRUE)
        {
            LOG_DEBUG("local emule is pure upload, we don't want to support such pipe.");
            emule_pipe_device_notify_error(emule_pipe->_device, EMULE_LOCAL_PURE_UPLOAD);
            return EMULE_LOCAL_PURE_UPLOAD;
        }
#endif

        if (tm_is_task_exist(gcid, FALSE) == TRUE)
        {
            LOG_DEBUG("emule task is exist, this pipe is download and upload type.");
            
            if (cm_is_pause_pipes(connect_manager) == TRUE)
            {
                emule_pipe_device_notify_error(emule_pipe->_device, EMULE_TASK_HAS_BEEN_PAUSED);
                return EMULE_TASK_HAS_BEEN_PAUSED;
            }
        }
        else
        {
            LOG_ERROR("I have downloaded all the data, so I don't need you again, :)");
            return EMULE_TASK_HAS_BEEN_FINISHED;
        }
    }

	if(sd_memcmp(file_id, data_manager->_file_id, FILE_ID_SIZE) != 0)
    {
        LOG_ERROR("file_id = is not equal to data_manager->_file_id = !");
        return -1;
    }

	if(request_ver == 0)
		return SUCCESS;

	if(request_ver > 0)
	{
        if(emule_is_part_hash_valid(data_manager) == FALSE 
            && range_list_size(&emule_pipe->_data_pipe._dispatch_info._can_download_ranges) > 0)
        {
            emule_pipe_send_part_hash_req_cmd(emule_pipe);
        }

        if(emule_is_aich_hash_valid(data_manager) == FALSE)
        {
            emule_pipe_send_arch_hash_req_cmd(emule_pipe);
        }

		ret = emule_pipe_part_bitmap_to_can_download_ranges(emule_pipe, &buffer, &len);
		dp_adjust_uncomplete_2_can_download_ranges(&emule_pipe->_data_pipe);

        // 有需要的数据就请求上传
        recvd_ranges = emule_get_recved_range_list(data_manager);
        if(range_list_is_contained(recvd_ranges, &emule_pipe->_data_pipe._dispatch_info._can_download_ranges) == FALSE
           && emule_pipe->_data_pipe._dispatch_info._pipe_state == PIPE_CONNECTED)
            ret = emule_pipe_send_start_upload_req_cmd(emule_pipe);
	}

	if(request_ver > 1)
	{
		ret = sd_get_int16_from_lt(&buffer, &len, (_int16*)&complete_source_count);
	}

	sd_assert(ret == SUCCESS && len == 0);

	return emule_pipe_send_req_filename_answer_cmd(emule_pipe);
}

_int32 emule_handle_queue_rank_cmd(EMULE_DATA_PIPE* emule_pipe, char* buffer, _int32 len)
{
	_int32 ret = SUCCESS;
	ret = sd_get_int32_from_lt(&buffer, &len, (_int32*)&emule_pipe->_rank);
	LOG_DEBUG("[emule_pipe = 0x%x]emule pipe recv QUEUE_RANK cmd, rank = %u.", emule_pipe, emule_pipe->_rank);
	sd_assert(len == 0);
	emule_pipe_change_state(emule_pipe, PIPE_CHOKED);
	sd_time(&emule_pipe->_last_ack_time);
	emule_download_queue_add(emule_pipe);
	return ret;
}


_int32 emule_handle_emule_info_cmd(EMULE_DATA_PIPE* emule_pipe, char* buffer, _int32 len)
{
	return emule_pipe_send_emule_info_answer_cmd(emule_pipe);
}

_int32 emule_handle_queue_ranking_cmd(EMULE_DATA_PIPE* emule_pipe, char* buffer, _int32 len)
{
	_int32 ret = SUCCESS;
	emule_pipe->_rank = 0;		//记得值为0，否则获取16位结果有误
	ret = sd_get_int16_from_lt(&buffer, &len, (_int16*)&emule_pipe->_rank);
	//协议后面是全0的10个字节
	sd_assert(len == 10);			
	LOG_DEBUG("[emule_pipe = 0x%x]emule pipe recv QUEUE_RANKING cmd, rank = %u.", emule_pipe, (_u32)emule_pipe->_rank);
	emule_pipe_change_state(emule_pipe, PIPE_CHOKED);
	sd_time(&emule_pipe->_last_ack_time);
	emule_download_queue_add(emule_pipe);
	return ret;
}

_int32 emule_handle_request_sources_cmd(EMULE_DATA_PIPE* emule_pipe, char* buffer, _int32 len)
{
	//暂时没有给对方提供来源交换
	return SUCCESS;	
}

_int32 emule_pipe_handle_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid)
{
	_int32 ret = SUCCESS;
	_u32 cur_time = 0;
	EMULE_DATA_PIPE* emule_pipe = (EMULE_DATA_PIPE*)msg_info->_user_data;
	if(errcode == MSG_CANCELLED)
		return SUCCESS;
	LOG_DEBUG("[emule_pipe = 0x%x]emule_pipe_handle_timeout.", emule_pipe);
	sd_assert(emule_pipe->_device == NULL);
	sd_time(&cur_time);
	if(/*emule_pipe->_rank < 10 ||*/ cur_time - emule_pipe->_last_ack_time >= 5 * 60)
	{
		//如果队列在前十之内，那么开始连接对方
		emule_pipe_open(&emule_pipe->_data_pipe);
		ret = cancel_timer(emule_pipe->_timeout_id);
		sd_assert(ret == SUCCESS);
		emule_pipe->_timeout_id = INVALID_MSG_ID;
		return ret;
	}
	//继续询问队列中的位置
	return emule_pipe_send_reask_cmd(emule_pipe);
}

#endif /* ENABLE_EMULE */
#endif /* ENABLE_EMULE */

