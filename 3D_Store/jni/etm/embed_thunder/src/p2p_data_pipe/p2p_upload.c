#include "utility/define.h"
#ifdef UPLOAD_ENABLE

#include "p2p_upload.h"
#include "p2p_data_pipe.h"
#include "p2p_memory_slab.h"
#include "p2p_pipe_impl.h"
#include "p2p_cmd_define.h"
#include "p2p_cmd_builder.h"
#include "p2p_socket_device.h"
#include "p2p_send_cmd.h"
#include "upload_manager/upload_manager.h"
#include "data_manager/data_manager_interface.h"
#include "connect_manager/pipe_interface.h"
#include "utility/utility.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_P2P_PIPE
#include "utility/logger.h"

//request_resp命令在数据后面带了25个字节的命令字段
#define		REQUEST_RESP_TAIL_LEN	25	


_int32 p2p_stop_upload(P2P_DATA_PIPE* p2p_pipe)
{
	_int32 ret = SUCCESS;
	P2P_UPLOAD_DATA* upload_data = NULL;
	LOG_ERROR("[p2p_pipe = 0x%x]p2p_stop_upload, cur_upload = 0x%x, range_data_buffer:0x%x", 
		p2p_pipe, p2p_pipe->_cur_upload, p2p_pipe->_range_data_buffer);
	if(p2p_pipe->_cur_upload == NULL)
	{
		//sd_assert( p2p_pipe->_range_data_buffer != NULL );
		//sd_assert( p2p_pipe->_get_upload_buffer_timeout_id == INVALID_MSG_ID );
		return SUCCESS;					//没有上传
	}
	if(p2p_pipe->_range_data_buffer != NULL)
	{	//说明有数据正在读,取消掉
		sd_assert(p2p_pipe->_cur_upload != NULL);
		ret = ulm_cancel_read_data(&p2p_pipe->_data_pipe);
		if(ret != SUCCESS)
		{
			LOG_ERROR("[p2p_pipe = 0x%x]p2p_stop_upload, cur_upload = 0x%x, ret:%d", p2p_pipe, p2p_pipe->_cur_upload, ret);
			p2p_free_range_data_buffer(p2p_pipe);
		}
		else
		{
			p2p_pipe->_range_data_buffer = NULL;		//在cancel回调会删除该块内存
		}
	}
	p2p_free_current_upload_request(p2p_pipe);
	while(list_size(&p2p_pipe->_upload_data_queue) > 0)
	{
		list_pop(&p2p_pipe->_upload_data_queue, (void**)&upload_data);
		sd_assert(upload_data->_buffer == NULL);
		p2p_free_p2p_upload_data(upload_data);
	}
	if(p2p_pipe->_get_upload_buffer_timeout_id != INVALID_MSG_ID)
	{
		cancel_timer(p2p_pipe->_get_upload_buffer_timeout_id);
		p2p_pipe->_get_upload_buffer_timeout_id = INVALID_MSG_ID;
	}
	return ret;
}

_int32 p2p_process_request_data(P2P_DATA_PIPE* p2p_pipe)
{
	_int32 ret = SUCCESS;
	REQUEST_RESP_CMD cmd;
	sd_assert(p2p_pipe->_cur_upload == NULL);
	list_pop(&p2p_pipe->_upload_data_queue, (void**)&p2p_pipe->_cur_upload);
	if(p2p_pipe->_cur_upload == NULL)
	{
		return p2p_free_range_data_buffer(p2p_pipe);	/*没有任何请求需要处理,清空range_data_buffer*/
	}
	LOG_DEBUG("[p2p_pipe = 0x%x]p2p process next request[%llu, %u].", p2p_pipe, p2p_pipe->_cur_upload->_data_pos, p2p_pipe->_cur_upload->_data_len);
	if(p2p_pipe->_data_pipe._dispatch_info._pipe_upload_state != UPLOAD_UNCHOKE_STATE)
	{
		LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]p2p_process_request_data, but upload_state = %d is wrong.", p2p_pipe, p2p_pipe->_device, p2p_pipe->_data_pipe._dispatch_info._pipe_upload_state);
		return SUCCESS;
	}
	sd_assert(p2p_pipe->_data_pipe._dispatch_info._pipe_state == PIPE_CHOKED || p2p_pipe->_data_pipe._dispatch_info._pipe_state == PIPE_DOWNLOADING);
	sd_memset(&cmd, 0, sizeof(REQUEST_RESP_CMD));
	cmd._data_pos = p2p_pipe->_cur_upload->_data_pos;
	cmd._data_len = p2p_pipe->_cur_upload->_data_len;
	ret = build_request_resp_cmd(&p2p_pipe->_cur_upload->_buffer
	    , &p2p_pipe->_cur_upload->_buffer_len
	    , &p2p_pipe->_cur_upload->_buffer_offset
	    , p2p_pipe->_device
	    , &cmd);
	if(ret != SUCCESS)
		return p2p_handle_upload_failed(p2p_pipe, ret);
	if(p2p_fill_upload_data(p2p_pipe) == TRUE)
	{
		LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]p2p_pipe upload data, data_pos = %llu, data_len = %u.", p2p_pipe, p2p_pipe->_device, p2p_pipe->_cur_upload->_data_pos, p2p_pipe->_cur_upload->_data_len);
		ret = p2p_socket_device_send(p2p_pipe, REQUEST_RESP, p2p_pipe->_cur_upload->_buffer, p2p_pipe->_cur_upload->_buffer_len);
		if(ret != SUCCESS)
		{
			p2p_pipe->_cur_upload->_buffer = NULL;
			return p2p_pipe_handle_error(p2p_pipe, ret);
		}
		ulm_stat_add_up_bytes(p2p_pipe->_cur_upload->_data_len);
		p2p_free_p2p_upload_data(p2p_pipe->_cur_upload);
		p2p_pipe->_cur_upload = NULL;
		return p2p_process_request_data(p2p_pipe);
	}
	return p2p_read_data(p2p_pipe);
}

_int32 p2p_read_data(P2P_DATA_PIPE* p2p_pipe)
{
	_int32 ret = SUCCESS;
	_u64 file_size = 0;
	_u32 data_offset = 0;
	if(p2p_pipe->_range_data_buffer == NULL)
	{
		//如果获取range_data_buffer失败的话，那么等待定时器超时重新分配
		if(p2p_malloc_range_data_buffer(p2p_pipe) != SUCCESS)
			return SUCCESS;		
	}
	//这个offset算出了目前已经读出了多少填充了多少数据，第一次读的话offset
	//是0，如果连续读的话offset不为0，连续读表示这个request的数据跨越了两个range,
	//要读两次才能把需要的数据凑齐
	data_offset = p2p_pipe->_cur_upload->_data_len - (p2p_pipe->_cur_upload->_buffer_len - REQUEST_RESP_TAIL_LEN - p2p_pipe->_cur_upload->_buffer_offset);
#ifdef _DEBUG
	if(data_offset > 0)
		LOG_DEBUG("[p2p_pipe = 0x%x]p2p_read_data again, offset = %u, since this request[%llu, %u] in diffrent ranges.", p2p_pipe, 
					data_offset, p2p_pipe->_cur_upload->_data_pos, p2p_pipe->_cur_upload->_data_len);
#endif
	sd_assert(p2p_pipe->_range_data_buffer->_data_ptr != NULL);
	p2p_pipe->_range_data_buffer->_range._index = (p2p_pipe->_cur_upload->_data_pos + data_offset) / get_data_unit_size();
	p2p_pipe->_range_data_buffer->_range._num = 1;
	ulm_get_file_size(&p2p_pipe->_data_pipe, &file_size);
	p2p_pipe->_range_data_buffer->_data_length = get_data_unit_size();
	if(p2p_pipe->_range_data_buffer->_range._index == file_size / get_data_unit_size())
		p2p_pipe->_range_data_buffer->_data_length = file_size % get_data_unit_size();		//最后一块要进行规整
	sd_assert(p2p_pipe->_cur_upload->_data_len <= get_data_unit_size());
	LOG_DEBUG("[p2p_pipe = 0x%x]ulm_read_data, range[%u, %u], cur_request[%llu, %u].", p2p_pipe, p2p_pipe->_range_data_buffer->_range._index,
				p2p_pipe->_range_data_buffer->_range._num, p2p_pipe->_cur_upload->_data_pos, p2p_pipe->_cur_upload->_data_len);
	ret = ulm_read_data(p2p_pipe->_data_pipe._p_data_manager
	    , p2p_pipe->_range_data_buffer
	    , p2p_read_data_callback
	    , &p2p_pipe->_data_pipe);
	if(ret != SUCCESS)
	{	//如果本次读失败了，那么此时的上传请求将被丢弃
		LOG_ERROR("[p2p_pipe = 0x%x, device = 0x%x]ulm_read_data failed, errcode = %d.", p2p_pipe, p2p_pipe->_device, ret);
		return p2p_handle_upload_failed(p2p_pipe, ret);
	}
	return ret;
}

BOOL p2p_fill_upload_data(P2P_DATA_PIPE* p2p_pipe)
{
	_u64 pos = 0;
	_u32 offset = 0, len = 0, data_offset = 0;
	sd_assert(p2p_pipe->_cur_upload != NULL);
	if(p2p_pipe->_range_data_buffer == NULL)
		return FALSE;
	if(p2p_pipe->_range_data_buffer->_data_ptr == NULL)
		return FALSE;
	pos = (_u64)get_data_unit_size() * p2p_pipe->_range_data_buffer->_range._index;
	//要先判断下要读的数据是否在range_data_buffer中
	if(p2p_pipe->_cur_upload->_data_pos >= pos + p2p_pipe->_range_data_buffer->_data_length ||
	    p2p_pipe->_cur_upload->_data_pos + p2p_pipe->_cur_upload->_data_len <= pos)
	{
		//要读的数据与range_data_buffer没有交集，说明不在这个range_data_buffer中
		return FALSE;
	}
	//这个data_offset算出了目前已经读出了多少填充了多少数据，第一次读的话offset
	//是0，如果连续读的话offset不为0，连续读表示这个request的数据跨越了两个range,
	//要读两次才能把需要的数据凑齐
	data_offset = p2p_pipe->_cur_upload->_data_len - (p2p_pipe->_cur_upload->_buffer_len - REQUEST_RESP_TAIL_LEN - p2p_pipe->_cur_upload->_buffer_offset);
	//以下的offset计算出要读的数据偏移
	offset = p2p_pipe->_cur_upload->_data_pos + data_offset - pos;
	len = MIN(p2p_pipe->_cur_upload->_data_len - data_offset, p2p_pipe->_range_data_buffer->_data_length - offset);
	//填充命令中的数据
	sd_memcpy(p2p_pipe->_cur_upload->_buffer + p2p_pipe->_cur_upload->_buffer_offset, p2p_pipe->_range_data_buffer->_data_ptr + offset, len);
	LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]p2p_fill_upload_data, copy len = %u.", p2p_pipe, p2p_pipe->_device, len);
	p2p_pipe->_cur_upload->_buffer_offset += len;
	//REQUEST_RESP_TAIL_LEN表示request_resp后面的25个字节的参数，此判断表示数据已经全部准备完毕	
	if(p2p_pipe->_cur_upload->_buffer_offset + REQUEST_RESP_TAIL_LEN == p2p_pipe->_cur_upload->_buffer_len)	
		return TRUE;
	else
	{
		sd_assert(p2p_pipe->_cur_upload->_buffer_offset + REQUEST_RESP_TAIL_LEN < p2p_pipe->_cur_upload->_buffer_len);
		return FALSE;
	}
}

_int32 p2p_read_data_callback(_int32 errcode,RANGE_DATA_BUFFER* data_buffer, void* user_data)
{
	_int32 ret = SUCCESS;
	P2P_DATA_PIPE* p2p_pipe = (P2P_DATA_PIPE*)user_data;
	LOG_DEBUG("[p2p_pipe = 0x%x]p2p_read_data_callback, errcode = %d. range[%u, %u]."
	    , p2p_pipe
	    , errcode, data_buffer->_range._index
	    , data_buffer->_range._num);
	if(errcode == UFM_ERR_READ_CANCELED)
	{	//此时p2p_pipe可能已经被析构了，不能再使用
		LOG_DEBUG("[p2p_pipe = 0x%x]p2p_read_data_callback failed, errcode = %d, free data_buffer = 0x%x."
        		, p2p_pipe
        		, errcode
        		, data_buffer->_data_ptr);
		dm_free_data_buffer(NULL, &data_buffer->_data_ptr, data_buffer->_buffer_length);
		return sd_free(data_buffer);
	}
	LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]p2p_read_data_callback.", p2p_pipe,  p2p_pipe->_device);
	sd_assert(data_buffer == p2p_pipe->_range_data_buffer);
	if(errcode != SUCCESS)
	{	//如果本次读失败了，那么此时的上传请求将被丢弃
		LOG_ERROR("[p2p_pipe = 0x%x]p2p pipe read data failed, errcode = %d, current upload request[%llu, %u].", p2p_pipe, errcode, p2p_pipe->_cur_upload->_data_pos, p2p_pipe->_cur_upload->_data_len);
		return p2p_handle_upload_failed(p2p_pipe, errcode);
	}
	if(p2p_fill_upload_data(p2p_pipe) == FALSE)
	{
		LOG_DEBUG("[p2p_pipe = 0x%x]p2p pipe upload read data again, cur_upload request[%llu, %u].", p2p_pipe, p2p_pipe->_cur_upload->_data_pos, p2p_pipe->_cur_upload->_data_len);
		return p2p_read_data(p2p_pipe);
	}
	LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]p2p_pipe upload data, data_pos = %llu, data_len = %u.", p2p_pipe, p2p_pipe->_device, p2p_pipe->_cur_upload->_data_pos, p2p_pipe->_cur_upload->_data_len);
	ret = p2p_socket_device_send(p2p_pipe, REQUEST_RESP, p2p_pipe->_cur_upload->_buffer, p2p_pipe->_cur_upload->_buffer_len);
	if(ret != SUCCESS)
	{
		p2p_pipe->_cur_upload->_buffer = NULL;
		return p2p_pipe_handle_error(p2p_pipe, ret);
	}
	ulm_stat_add_up_bytes(p2p_pipe->_cur_upload->_data_len);
	p2p_free_p2p_upload_data(p2p_pipe->_cur_upload);
	p2p_pipe->_cur_upload = NULL;
	return p2p_process_request_data(p2p_pipe);		//处理下一个请求
}

_int32 p2p_malloc_range_data_buffer_timeout_handler(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid)
{
	P2P_DATA_PIPE* p2p_pipe = (P2P_DATA_PIPE*)msg_info->_user_data;
	if(errcode == MSG_CANCELLED)
	{	
		LOG_DEBUG("get_upload_buffer_timeout_handler error, since timeout message is cancel.");
		return SUCCESS;
	}
	p2p_pipe->_get_upload_buffer_timeout_id = INVALID_MSG_ID;
	if(errcode != MSG_TIMEOUT)
	{
		LOG_ERROR("[p2p_pipe = 0x%x, device = 0x%x]get_upload_buffer_timeout_handler failed, error code is %d.", p2p_pipe, p2p_pipe->_device, errcode);
		return p2p_handle_upload_failed(p2p_pipe, errcode);
	}
	sd_assert(p2p_pipe->_data_pipe._dispatch_info._pipe_state != PIPE_FAILURE);
	if(p2p_malloc_range_data_buffer(p2p_pipe) == SUCCESS)
		p2p_read_data(p2p_pipe);
	return SUCCESS;
}

_int32 p2p_free_current_upload_request(P2P_DATA_PIPE* p2p_pipe)
{	
	sd_assert(p2p_pipe->_cur_upload  != NULL);
	LOG_DEBUG("[p2p_pipe = 0x%x]p2p cancel current upload request[%llu, %u]", p2p_pipe,
				p2p_pipe->_cur_upload->_data_pos, p2p_pipe->_cur_upload->_data_len);
	if( p2p_pipe->_cur_upload->_buffer != NULL )
	{
	ptl_free_cmd_buffer(p2p_pipe->_cur_upload->_buffer);
	}
	p2p_free_p2p_upload_data(p2p_pipe->_cur_upload);
	p2p_pipe->_cur_upload = NULL;
	return SUCCESS;
}

_int32 p2p_malloc_range_data_buffer(P2P_DATA_PIPE* p2p_pipe)
{
	_int32 ret = SUCCESS;
	char* data_buffer = NULL;
	_u32 data_buffer_len = get_data_unit_size();
	sd_assert(p2p_pipe->_range_data_buffer == NULL);
	//这里获得内存时之所以不用pi_get_data_buffer，是因为后面释放时，pipe可能不存在了，不能使用pi_free_data_buffer
	ret = pi_get_data_buffer(&p2p_pipe->_data_pipe, &data_buffer,  &data_buffer_len);
	if(ret != SUCCESS)
	{
		if(ret == DATA_BUFFER_IS_FULL || ret == OUT_OF_MEMORY)
		{
			LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]p2p get upload data buffer failed, start timer to retry.", p2p_pipe, p2p_pipe->_device);
			start_timer(p2p_malloc_range_data_buffer_timeout_handler, NOTICE_ONCE, 1000, 0, p2p_pipe, &p2p_pipe->_get_upload_buffer_timeout_id);
		}
		else
		{
			LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]p2p get upload data buffer failed, pipe set to FAILURE.", p2p_pipe, p2p_pipe->_device);
			p2p_handle_upload_failed(p2p_pipe, ret);
		}
		return ret;
	}
	ret = sd_malloc(sizeof(RANGE_DATA_BUFFER), (void**)&p2p_pipe->_range_data_buffer);
	if(ret != SUCCESS)
	{
		pi_free_data_buffer(&p2p_pipe->_data_pipe, &data_buffer, data_buffer_len);
		return p2p_handle_upload_failed(p2p_pipe, ret);
	}
	LOG_DEBUG("[p2p_pipe = 0x%x]p2p_malloc_range_data_buffer, _range_data_buffer:0x%x, data_buffer:0x%x", 
		p2p_pipe, p2p_pipe->_range_data_buffer, p2p_pipe->_range_data_buffer->_data_ptr );
	
	sd_memset(p2p_pipe->_range_data_buffer, 0, sizeof(RANGE_DATA_BUFFER));
	p2p_pipe->_range_data_buffer->_buffer_length = data_buffer_len;
	p2p_pipe->_range_data_buffer->_data_ptr = data_buffer;
	return ret;
}

_int32 p2p_free_range_data_buffer(P2P_DATA_PIPE* p2p_pipe)
{
	LOG_DEBUG("[p2p_pipe = 0x%x]p2p_free_range_data_buffer, _range_data_buffer:0x%x, data_buffer:0x%x", 
		p2p_pipe, p2p_pipe->_range_data_buffer, p2p_pipe->_range_data_buffer->_data_ptr );
	if(p2p_pipe->_range_data_buffer != NULL)
	{
		pi_free_data_buffer(&p2p_pipe->_data_pipe, &p2p_pipe->_range_data_buffer->_data_ptr, p2p_pipe->_range_data_buffer->_buffer_length);
		sd_free(p2p_pipe->_range_data_buffer);
		p2p_pipe->_range_data_buffer = NULL;
	}
	return SUCCESS;
}

void p2p_change_upload_state(P2P_DATA_PIPE* p2p_pipe, PEER_UPLOAD_STATE state)
{
#ifdef _DEBUG
	char pipe_state_str[4][21] = {"UPLOAD_CHOKE_STATE", "UPLOAD_UNCHOKE_STATE", "UPLOAD_FIN_STATE", "UPLOAD_FAILED_STATE"};
	LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]p2p_change_upload_state, from %s to %s", p2p_pipe, p2p_pipe->_device, pipe_state_str[p2p_pipe->_data_pipe._dispatch_info._pipe_upload_state], pipe_state_str[state]);
#endif
	p2p_pipe->_data_pipe._dispatch_info._pipe_upload_state = state;
}

_int32 p2p_handle_upload_failed(P2P_DATA_PIPE* p2p_pipe, _int32 errcode)
{
	LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]p2p_handle_upload_failed, errcode = %d.", p2p_pipe, p2p_pipe->_device, errcode);
	if(p2p_pipe->_data_pipe._dispatch_info._pipe_upload_state == UPLOAD_UNCHOKE_STATE)
	{
		LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]p2p_handle_upload_failed and send choke_cmd to choke remote peer.", p2p_pipe, p2p_pipe->_device, errcode);
		p2p_send_choke_cmd(p2p_pipe, TRUE);
	}
	p2p_change_upload_state(p2p_pipe, UPLOAD_FAILED_STATE);
	p2p_stop_upload(p2p_pipe);
	return SUCCESS;
}

#endif
