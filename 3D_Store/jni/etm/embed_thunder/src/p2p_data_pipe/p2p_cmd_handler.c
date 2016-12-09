#include "p2p_cmd_handler.h"
#include "p2p_data_pipe.h"
#include "p2p_cmd_define.h"
#include "p2p_utility.h"
#include "p2p_cmd_builder.h"
#include "p2p_cmd_extractor.h"
#include "p2p_send_cmd.h"
#include "p2p_pipe_interface.h"
#include "p2p_memory_slab.h"
#include "p2p_socket_device.h"
#include "p2p_pipe_impl.h"
#ifdef UPLOAD_ENABLE
#include "p2p_upload.h"
#include "upload_manager/upload_manager.h"
#include "upload_manager/upload_control.h"
#endif
#include "task_manager/task_manager.h"
#include "data_manager/data_manager_interface.h"

#include "utility/peer_capability.h"
#include "utility/time.h"
#include "utility/peerid.h"
#include "utility/utility.h"
#include "utility/local_ip.h"
#include "utility/logid.h"
#include "utility/bytebuffer.h"
#define	 LOGID	LOGID_P2P_PIPE
#include "utility/logger.h"

#define	P2P_KEEPALIVE_TIMEOUT			120000
#define	P2P_REQUEST_RESP_MAX_DATA_LEN	65536

BOOL is_remote_in_nat(const char* remote_host)
{
	_u32 ip = sd_inet_addr(remote_host);
	unsigned char* pointer = (unsigned char*)&ip;
	unsigned char c1 = pointer[0];
	unsigned char c2 = pointer[1];
	if((c1 == 10) || ((c1 == 172) && (c2 > 15) && (c2 < 32)) || ((c1 == 192) && (c2 == 168)))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/*if return nonzero means handle protocol command failed, pipe must be close*/
_int32 handle_recv_cmd(P2P_DATA_PIPE* pipe, _u8 command_type, char* buffer, _u32 buffer_len)
{
#ifdef _DEBUG
	char cmd_str[17][16] = {"HANDSHAKE", "HANDSHAKE_RESP", "INTERESTED", "INTERESTED_RESP", "NOT_INTERESTED", "KEEPALIVE", "REQUEST", "REQUEST_RESP", "CANCEL",
							"CANCEL_RESP", "UNKNOWN", "UNKNOWN", "UNKNOWN_COMMAND", "CHOKE", "UNCHOKE", "FIN", "FIN_RESP"};
       cmd_str[0][15]=0;
	LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]handle recv %s command, buffer = 0x%x, buffer_len = %u, version = %u", pipe, pipe->_device, cmd_str[command_type - 100], buffer, buffer_len, pipe->_remote_protocol_ver);
#endif
	switch(command_type)
	{
	case HANDSHAKE:
		{
			return handle_handshake_cmd(pipe, buffer, buffer_len);
		}
	case HANDSHAKE_RESP:
		{
			return handle_handshake_resp_cmd(pipe, buffer, buffer_len);
		}
	case INTERESTED:
		{
			return handle_interested_cmd(pipe, buffer, buffer_len);
		}
	case INTERESTED_RESP:
		{
			return handle_interested_resp_cmd(pipe, buffer, buffer_len);
		}
	case NOT_INTERESTED:
		{
			return handle_not_interested(pipe, buffer, buffer_len);
		}
	case KEEPALIVE:
		{
			return SUCCESS;
		}
#ifdef UPLOAD_ENABLE
	case REQUEST:
		{
		    if(ulc_enable_upload())
    			return handle_request_cmd(pipe, buffer, buffer_len);
    	    else
    	        return SUCCESS;
		}
#endif
	case REQUEST_RESP:
		{	/*if receive a complete request response command, it means no data response*/
			/*this may be happen, example in p2p version 51*/
			return handle_request_resp_cmd(pipe, buffer, buffer_len);
		}
	case CANCEL:
		{
			return handle_cancel_cmd(pipe, buffer, buffer_len);
		}
	case CANCEL_RESP:
		{
			return handle_cancel_resp_cmd(pipe, buffer, buffer_len);
		}
	case UNKNOWN_COMMAND:
		{	/*remote peer unknown your command*/
			LOG_DEBUG("[p2p_pipe = 0x%x]recv a UNKNOWN_COMMAND response, remote peer is unknown my last command, remote peer version = %u", pipe, pipe->_remote_protocol_ver);
			return ERR_P2P_REMOTE_UNKNOWN_MY_CMD;
		}	
	case CHOKE:
		{
			return handle_choke_cmd(pipe);
		}
	case UNCHOKE:
		{
			return handle_unchoke_cmd(pipe);
		}
	case FIN:
		{
			return handle_fin_cmd(pipe);
		}
	default:
		{
			LOG_ERROR("[p2p_pipe = 0x%x]recv a p2p command which not process. command_type = %u, version = %u.", pipe, command_type, pipe->_remote_protocol_ver);
		}
	}
	return SUCCESS;
}

#ifdef UPLOAD_ENABLE
_int32 handle_request_cmd(P2P_DATA_PIPE* p2p_pipe, char* buffer, _u32 buffer_len)
{
	_int32 ret = SUCCESS;
	_u64 len  = 0;
	P2P_UPLOAD_DATA* upload_data = NULL;
	REQUEST_CMD request_cmd;
	ret = extract_request_cmd(buffer, buffer_len, &request_cmd);
	if(ret != SUCCESS)
	{
		return p2p_handle_upload_failed(p2p_pipe, ret);
	}
	LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]handle_request_cmd, request_data[%llu, %llu].", p2p_pipe, p2p_pipe->_device, request_cmd._file_pos, request_cmd._file_len);
	if(p2p_pipe->_data_pipe._dispatch_info._pipe_upload_state != UPLOAD_UNCHOKE_STATE)
	{
		LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]handle_request_cmd,but upload state(%d) is not UPLOAD_UNCHOKE_STATE, discard request.",
					p2p_pipe, p2p_pipe->_device, p2p_pipe->_data_pipe._dispatch_info._pipe_upload_state);
		return SUCCESS;
	}
	len = request_cmd._file_len;
    if(request_cmd._file_len > 65536)
	{
		return p2p_handle_upload_failed(p2p_pipe, -1);
	}
	//sd_assert(request_cmd._file_len <= 65536);
	while(request_cmd._file_len > 0 && request_cmd._file_len <= 65536)		//请求的数据长度不能大于64K
	{
		ret = p2p_malloc_p2p_upload_data(&upload_data);
		if(ret != SUCCESS)
		{
			LOG_ERROR("[p2p_pipe = 0x%x]p2p handle request[%llu, %llu], but malloc p2p upload data failed.", p2p_pipe,
						request_cmd._file_pos, request_cmd._file_len);
			return ret;
		}
		sd_memset(upload_data, 0, sizeof(P2P_UPLOAD_DATA));
		upload_data->_data_pos = request_cmd._file_pos;
		if(request_cmd._file_len > request_cmd._max_package_size)
			upload_data->_data_len = request_cmd._max_package_size;
		else
			upload_data->_data_len = (_u32)request_cmd._file_len;
		list_push(&p2p_pipe->_upload_data_queue, upload_data);
		request_cmd._file_pos += upload_data->_data_len;
		request_cmd._file_len -= upload_data->_data_len;
	}
	if(p2p_pipe->_cur_upload == NULL)
		return p2p_process_request_data(p2p_pipe);		//开始上传
	else
		return SUCCESS;
}
#endif

_int32 handle_request_resp_cmd(P2P_DATA_PIPE* p2p_pipe, char* buffer, _u32 len)
{
	_u32 data_len;
	REQUEST_RESP_CMD request_resp_cmd;
    _int32 ret = extract_request_resp_cmd(buffer, len, &request_resp_cmd);
	if(ret != SUCCESS)
		return ret;
	if(request_resp_cmd._result != P2P_RESULT_SUCC)
	{
		/*receive data failed, just set pipe failed, not to retry*/
		LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]p2p_pipe receive a request response, but request_resp_cmd._result = %d, not success.", p2p_pipe, p2p_pipe->_device, request_resp_cmd._result);
		return ERR_P2P_REQUEST_RESP_FAIL;
	}
	/*had received command package exclude file data*/
	data_len = 0;
	if(request_resp_cmd._header._version < P2P_PROTOCOL_VERSION_54)
	{
		data_len = request_resp_cmd._header._command_len - 2;
	}
	else
	{
		data_len = request_resp_cmd._data_len; 
	}
	if(data_len > P2P_REQUEST_RESP_MAX_DATA_LEN || data_len == 0)		/*greater 64K, must be wrong*/
	{
		LOG_ERROR("[p2p_pipe = 0x%x]handle_request_resp_cmd, but data_len is greate than 64K, must be wrong.", p2p_pipe);
		return -1;
	}
	LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]handle_request_resp_cmd, buffer = 0x%x, buffer_len = %u, data_pos = %llu, data_len = %u", p2p_pipe, p2p_pipe->_device, buffer, len, request_resp_cmd._data_pos, request_resp_cmd._data_len);
	if(p2p_pipe->_cancel_flag == TRUE)
	{
		p2p_pipe->_socket_device->_is_valid_data = FALSE;
	}
	else if(request_resp_cmd._header._version >= P2P_PROTOCOL_VERSION_54)
	{
		RANGE down_range;
		_u64 pos;
		dp_get_download_range(&p2p_pipe->_data_pipe, &down_range);
		pos = (_u64)down_range._index * get_data_unit_size();
		if(request_resp_cmd._data_pos == pos + p2p_pipe->_socket_device->_data_buffer_offset)
		{		
			p2p_pipe->_socket_device->_is_valid_data = TRUE;
		}
		else
		{
			LOG_ERROR("[p2p_pipe = 0x%x, device = 0x%x]handle_request_resp_cmd,but data isn't valid. request_resp_cmd._data_pos = %llu, cur_pos = %llu",
						p2p_pipe, p2p_pipe->_device, request_resp_cmd._data_pos, pos + p2p_pipe->_socket_device->_data_buffer_offset);
			p2p_pipe->_socket_device->_is_valid_data = FALSE;
		}
	}
	return p2p_socket_device_recv_data(p2p_pipe, data_len);
}

_int32 handle_handshake_cmd(P2P_DATA_PIPE* pipe, char* buffer, _u32 len)
{
	sd_assert(FALSE);			//按照目前的逻辑，不可能跑到这个地方
	return 	p2p_send_handshake_resp_cmd(pipe, P2P_RESULT_ADD_TASK_ERROR);
}

_int32 handle_handshake_resp_cmd(P2P_DATA_PIPE* pipe, char* buffer, _u32 len)
{
	_int32 http_encap_state = 0; /*  0-未知,1-迅雷p2p协议不需要http封装,2-迅雷p2p协议需要http封装  */
	_int32 http_encap_test_count = 0; /*  测试MAX_HTTP_ENCAP_P2P_TEST_COUNT 次  */
	P2P_RESOURCE* peer_resource = (P2P_RESOURCE*)pipe->_data_pipe._p_resource;
	HANDSHAKE_RESP_CMD handshake_resp_cmd;
	_int32 ret = extract_handshake_resp_cmd(buffer, len, &handshake_resp_cmd);
	
	/* 迅雷p2p协议可以用了*/
	settings_get_int_item( "p2p.http_encap_state", &http_encap_state );
	if(http_encap_state == 0)
	{
		#if defined(MACOS)&&defined(_DEBUG)
		settings_get_int_item( "p2p.http_encap_state", &http_encap_test_count );
		printf("\n !!!!!!!!handle_handshake_resp_cmd is ok,do not need http encap!:http_encap_test_count=%d!!!!!!!!!\n",http_encap_test_count);
		http_encap_test_count = 0;
		#endif
		/* 迅雷p2p协议没有被屏蔽,不需要用http封装 */
		http_encap_state = 1;
		settings_set_int_item( "p2p.http_encap_state", http_encap_state );
		settings_set_int_item( "p2p.http_encap_test_count", http_encap_test_count );
	}
	
	if(ret != SUCCESS)
		return ret;
	if(handshake_resp_cmd._result == P2P_RESULT_SUCC)
	{	/*handshake success*/
		pipe->_remote_not_in_nat = handshake_resp_cmd._not_in_nat;
		sd_assert(pipe->_socket_device->_keepalive_timeout_id == INVALID_MSG_ID);
		start_timer(handle_keepalive_timeout, NOTICE_INFINITE, P2P_KEEPALIVE_TIMEOUT, 0, pipe, &pipe->_socket_device->_keepalive_timeout_id);
        if (tm_is_task_exist(peer_resource->_gcid, TRUE) == TRUE)
		{	/*is downloading this file*/
			return p2p_send_interested_cmd(pipe);
		}
		else
		{	/*这个时候任务已经完成了，但可能pipe还没关闭，直接把pipe设为failed,等待上层关闭*/
			LOG_DEBUG("[p2p_pipe = 0x%x]handle_handshake_resp_cmd, but task is not exist, just wait for close this pipe, set pipe to failed.", pipe);
			return p2p_pipe_handle_error(pipe, -1);
		}
	}
	/*handshake response failed, maybe peer have too many uploads, can retry but this version just close this pipe*/
	LOG_DEBUG("[p2p_pipe = 0x%x]handshake_resp_cmd result failed, p2p result is %d", pipe, handshake_resp_cmd._result);
	if(handshake_resp_cmd._result == P2P_RESULT_UPLOAD_OVER_MAX)
		return ERR_P2P_UPLOAD_OVER_MAX;
	else
		return ERR_P2P_HANDSHAKE_RESP_FAIL;
}

_int32 handle_interested_cmd(P2P_DATA_PIPE* p2p_pipe, char* buffer, _u32 len)
{
	/*this version not implement upload, so all interested response are set to no data*/
	_int32 ret = SUCCESS;
#ifdef UPLOAD_ENABLE
    if( ulc_enable_upload() )
    {
    	P2P_RESOURCE* res = (P2P_RESOURCE*)p2p_pipe->_data_pipe._p_resource;
    	INTERESTED_CMD interested_cmd;
    	ret = extract_interested_cmd(buffer, len, &interested_cmd);
    	if(ret != SUCCESS)
    		return ret;	
    	if(res != NULL && interested_cmd._header._version >= P2P_PROTOCOL_VERSION_58)
    	{	
    		//这个是边下边传,只有大于P2P_PROTOCOL_VERSION_58版本的迅雷才
    		//支持choke/unchoke，对于之前的版本不提供上传
    		ret = ulm_add_pipe_by_gcid(res->_gcid, &p2p_pipe->_data_pipe);
    		if(ret != SUCCESS)
    		{
    		    return ret;
    		}
    	}
    	return p2p_send_interested_resp_cmd(p2p_pipe, interested_cmd._min_block_size, interested_cmd._max_block_count);
    }
    else
    {
    	char* cmd_buffer = NULL;
    	_u32  cmd_len = 0;
    	INTERESTED_RESP_CMD interested_resp_cmd;
    	//不给对方提供上传，发送空的intrested_resp，欺骗对方没有数据提供上传
    	interested_resp_cmd._download_ratio = 0;
    	interested_resp_cmd._block_num = 0;
    	ret = build_interested_resp_cmd(&cmd_buffer, &cmd_len, p2p_pipe->_device, &interested_resp_cmd);
    	CHECK_VALUE(ret);
    	return p2p_socket_device_send(p2p_pipe, INTERESTED_RESP, cmd_buffer, cmd_len);       
    }
#else
	char* cmd_buffer = NULL;
	_u32  cmd_len = 0;
	INTERESTED_RESP_CMD interested_resp_cmd;
	//不给对方提供上传，发送空的intrested_resp，欺骗对方没有数据提供上传
	interested_resp_cmd._download_ratio = 0;
	interested_resp_cmd._block_num = 0;
	ret = build_interested_resp_cmd(&cmd_buffer, &cmd_len, p2p_pipe->_device, &interested_resp_cmd);
	CHECK_VALUE(ret);
	return p2p_socket_device_send(p2p_pipe, INTERESTED_RESP, cmd_buffer, cmd_len);
    
#endif
}

_int32 handle_interested_resp_cmd(P2P_DATA_PIPE* pipe, char* buffer, _u32 len)
{
	_u32 i;
	_u64 file_size = 0;
	BLOCK block;
	RANGE  range, down_range;
	INTERESTED_RESP_CMD	interested_resp_cmd;
	_int32 ret = extract_interested_resp_cmd(buffer, len, &interested_resp_cmd);
	if(ret != SUCCESS)
		return ret;
	/*clear can_download_ranges info*/
	dp_clear_can_download_ranges_list(&pipe->_data_pipe);
	LOG_DEBUG("extract_interested_resp_cmd, block_num is %u, version is %u", interested_resp_cmd._block_num, interested_resp_cmd._header._version);
	/*change interested response BLOCK to _can_download_range*/ 
	for(i = 0; i < interested_resp_cmd._block_num; ++i)
	{
		_u8 len_hi = 0, len_low = 0, byte_value = 0;
		_u32 loop = 0;
		ret = sd_get_int8(&interested_resp_cmd._block_data, &interested_resp_cmd._block_data_len, (_int8*)&block._store_type);
		CHECK_VALUE(ret);
		len_low = block._store_type % 0x10;
		len_hi = block._store_type / 0x10;
		block._file_pos = 0;
		while(len_low > 0)
		{
			ret = sd_get_int8(&interested_resp_cmd._block_data, &interested_resp_cmd._block_data_len, (_int8*)&byte_value);
			CHECK_VALUE(ret);
			block._file_pos += (((_u64)byte_value) << loop);
			len_low--;
			loop += 8;
		}
		loop = 0;
		block._file_len = 0;
		while(len_hi > 0)
		{
			ret = sd_get_int8(&interested_resp_cmd._block_data, &interested_resp_cmd._block_data_len, (_int8*)&byte_value);
			CHECK_VALUE(ret);
			block._file_len += (((_u64)byte_value) << loop);
			len_hi--;
			loop+=8;
		}
		file_size = pi_get_file_size(&pipe->_data_pipe);
		if(file_size==0)
		{
			file_size = block._file_pos+block._file_len;
		}
		range = pos_length_to_range2(block._file_pos, block._file_len, file_size);
		LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]handle_interested_resp_cmd, translate pos_length[%llu, %llu] to range[%u, %u]", pipe, pipe->_device, block._file_pos, block._file_len, range._index, range._num);
		dp_add_can_download_ranges(&pipe->_data_pipe, &range);
	}
//	sd_assert(interested_resp_cmd._block_data_len == 0);
	dp_adjust_uncomplete_2_can_download_ranges(&pipe->_data_pipe);
	if(pipe->_remote_protocol_ver < P2P_PROTOCOL_VERSION_58)	//对端是老版本，不用等待unchoke消息，直接发送数据请求
	{
		return handle_unchoke_cmd(pipe);
	}
	dp_get_download_range(&pipe->_data_pipe, &down_range);
	if(pipe->_data_pipe._dispatch_info._pipe_state == PIPE_DOWNLOADING && down_range._num == 0)
			return p2p_pipe_request_data(pipe);
	return SUCCESS;
}

_int32 handle_cancel_cmd(P2P_DATA_PIPE* p2p_pipe, char* buffer, _u32 len)
{	
#ifdef UPLOAD_ENABLE
	p2p_stop_upload(p2p_pipe);
#endif
	return p2p_send_cancel_resp_cmd(p2p_pipe);
}

_int32 handle_cancel_resp_cmd(P2P_DATA_PIPE* p2p_pipe, char* buffer, _u32 len)
{
	CANCEL_RESP_CMD cancel_resp_cmd;
	_int32 ret = extract_cancel_resp_cmd(buffer, len, &cancel_resp_cmd);
	if(ret != SUCCESS)
		return ret;
	sd_assert(p2p_pipe->_cancel_flag == TRUE);
//	sd_assert(pipe->_data_pipe._dispatch_info._down_range._num == 0);
//	sd_assert(pipe->_data_pipe._dispatch_info._down_range._index == 0);
	p2p_socket_device_free_data_buffer(p2p_pipe);
	p2p_pipe->_cancel_flag = FALSE;		/*set this flat to notify cancel response, can accepted new request_response_command data*/
	p2p_pipe->_socket_device->_is_valid_data = TRUE;	//之后收到的数据是有效的
	return p2p_pipe_request_data(p2p_pipe);
}

_int32 handle_keepalive_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid)
{	
	P2P_DATA_PIPE* p2p_pipe = (P2P_DATA_PIPE*)msg_info->_user_data;
	if(errcode == MSG_CANCELLED)
		return SUCCESS;
	//sd_assert(errcode == MSG_TIMEOUT);
	LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]handle_keepalive_timeout, send keepalive command", p2p_pipe, p2p_pipe->_device);
	return p2p_send_keepalive_cmd(p2p_pipe);
}

_int32 handle_not_interested(P2P_DATA_PIPE* pipe, char* buffer, _u32 len)
{
	LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]handle_not_interested, buffer = 0x%x, len = %u", pipe, pipe->_device, buffer, len);
	return SUCCESS;
}

_int32 handle_choke_cmd(P2P_DATA_PIPE* p2p_pipe)
{
	RANGE* range = NULL;
	LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]pipe is choke.", p2p_pipe, p2p_pipe->_device);
	sd_assert(p2p_pipe->_remote_protocol_ver > P2P_PROTOCOL_VERSION_57);
	/*发现一种新情况，一个60版本的peer，发送了interested_resp后，没有发送unchoke，但是却发送了choke*/
	sd_assert(p2p_pipe->_data_pipe._dispatch_info._pipe_state == PIPE_DOWNLOADING || p2p_pipe->_data_pipe._dispatch_info._pipe_state == PIPE_CHOKED);
	p2p_pipe_change_state(p2p_pipe, PIPE_CHOKED);
	//接着清空所有的数据请求
	dp_clear_download_range(&p2p_pipe->_data_pipe);
	while(list_size(&p2p_pipe->_request_data_queue) > 0)
	{
		list_pop(&p2p_pipe->_request_data_queue, (void**)&range);
		p2p_free_range(range);
	}
	return p2p_socket_device_free_data_buffer(p2p_pipe);
}

_int32 handle_unchoke_cmd(P2P_DATA_PIPE* p2p_pipe)
{
	LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]pipe is unchoke.", p2p_pipe, p2p_pipe->_device);
	p2p_pipe->_is_ever_unchoke = TRUE;
	p2p_pipe_change_state(p2p_pipe, PIPE_DOWNLOADING);
	if(dp_get_can_download_ranges_list_size(&p2p_pipe->_data_pipe) == 0)
		return SUCCESS;		//此时还没有收到interested response或者已经收到了interested response，但对方没有可以下载的range
	return p2p_pipe_request_data(p2p_pipe);			
}

_int32 handle_fin_cmd(P2P_DATA_PIPE* p2p_pipe)
{
#ifdef UPLOAD_ENABLE
	p2p_change_upload_state(p2p_pipe, UPLOAD_FIN_STATE);
	p2p_stop_upload(p2p_pipe);
#endif
	return p2p_send_fin_resp_cmd(p2p_pipe);
}

