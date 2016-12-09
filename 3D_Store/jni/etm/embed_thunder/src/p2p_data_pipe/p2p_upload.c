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

//request_resp���������ݺ������25���ֽڵ������ֶ�
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
		return SUCCESS;					//û���ϴ�
	}
	if(p2p_pipe->_range_data_buffer != NULL)
	{	//˵�����������ڶ�,ȡ����
		sd_assert(p2p_pipe->_cur_upload != NULL);
		ret = ulm_cancel_read_data(&p2p_pipe->_data_pipe);
		if(ret != SUCCESS)
		{
			LOG_ERROR("[p2p_pipe = 0x%x]p2p_stop_upload, cur_upload = 0x%x, ret:%d", p2p_pipe, p2p_pipe->_cur_upload, ret);
			p2p_free_range_data_buffer(p2p_pipe);
		}
		else
		{
			p2p_pipe->_range_data_buffer = NULL;		//��cancel�ص���ɾ���ÿ��ڴ�
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
		return p2p_free_range_data_buffer(p2p_pipe);	/*û���κ�������Ҫ����,���range_data_buffer*/
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
		//�����ȡrange_data_bufferʧ�ܵĻ�����ô�ȴ���ʱ����ʱ���·���
		if(p2p_malloc_range_data_buffer(p2p_pipe) != SUCCESS)
			return SUCCESS;		
	}
	//���offset�����Ŀǰ�Ѿ������˶�������˶������ݣ���һ�ζ��Ļ�offset
	//��0������������Ļ�offset��Ϊ0����������ʾ���request�����ݿ�Խ������range,
	//Ҫ�����β��ܰ���Ҫ�����ݴ���
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
		p2p_pipe->_range_data_buffer->_data_length = file_size % get_data_unit_size();		//���һ��Ҫ���й���
	sd_assert(p2p_pipe->_cur_upload->_data_len <= get_data_unit_size());
	LOG_DEBUG("[p2p_pipe = 0x%x]ulm_read_data, range[%u, %u], cur_request[%llu, %u].", p2p_pipe, p2p_pipe->_range_data_buffer->_range._index,
				p2p_pipe->_range_data_buffer->_range._num, p2p_pipe->_cur_upload->_data_pos, p2p_pipe->_cur_upload->_data_len);
	ret = ulm_read_data(p2p_pipe->_data_pipe._p_data_manager
	    , p2p_pipe->_range_data_buffer
	    , p2p_read_data_callback
	    , &p2p_pipe->_data_pipe);
	if(ret != SUCCESS)
	{	//������ζ�ʧ���ˣ���ô��ʱ���ϴ����󽫱�����
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
	//Ҫ���ж���Ҫ���������Ƿ���range_data_buffer��
	if(p2p_pipe->_cur_upload->_data_pos >= pos + p2p_pipe->_range_data_buffer->_data_length ||
	    p2p_pipe->_cur_upload->_data_pos + p2p_pipe->_cur_upload->_data_len <= pos)
	{
		//Ҫ����������range_data_bufferû�н�����˵���������range_data_buffer��
		return FALSE;
	}
	//���data_offset�����Ŀǰ�Ѿ������˶�������˶������ݣ���һ�ζ��Ļ�offset
	//��0������������Ļ�offset��Ϊ0����������ʾ���request�����ݿ�Խ������range,
	//Ҫ�����β��ܰ���Ҫ�����ݴ���
	data_offset = p2p_pipe->_cur_upload->_data_len - (p2p_pipe->_cur_upload->_buffer_len - REQUEST_RESP_TAIL_LEN - p2p_pipe->_cur_upload->_buffer_offset);
	//���µ�offset�����Ҫ��������ƫ��
	offset = p2p_pipe->_cur_upload->_data_pos + data_offset - pos;
	len = MIN(p2p_pipe->_cur_upload->_data_len - data_offset, p2p_pipe->_range_data_buffer->_data_length - offset);
	//��������е�����
	sd_memcpy(p2p_pipe->_cur_upload->_buffer + p2p_pipe->_cur_upload->_buffer_offset, p2p_pipe->_range_data_buffer->_data_ptr + offset, len);
	LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]p2p_fill_upload_data, copy len = %u.", p2p_pipe, p2p_pipe->_device, len);
	p2p_pipe->_cur_upload->_buffer_offset += len;
	//REQUEST_RESP_TAIL_LEN��ʾrequest_resp�����25���ֽڵĲ��������жϱ�ʾ�����Ѿ�ȫ��׼�����	
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
	{	//��ʱp2p_pipe�����Ѿ��������ˣ�������ʹ��
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
	{	//������ζ�ʧ���ˣ���ô��ʱ���ϴ����󽫱�����
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
	return p2p_process_request_data(p2p_pipe);		//������һ������
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
	//�������ڴ�ʱ֮���Բ���pi_get_data_buffer������Ϊ�����ͷ�ʱ��pipe���ܲ������ˣ�����ʹ��pi_free_data_buffer
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
