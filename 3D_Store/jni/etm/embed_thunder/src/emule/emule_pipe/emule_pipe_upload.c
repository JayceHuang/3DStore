
#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL
#include "emule_pipe_upload.h"
#include "emule_pipe_device.h"
#include "../emule_utility/emule_memory_slab.h"
#include "../emule_data_manager/emule_data_manager.h"
#include "../emule_utility/emule_opcodes.h"
#include "connect_manager/pipe_interface.h"
#include "data_manager/data_receive.h"
#include "utility/utility.h"
#include "utility/bytebuffer.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_EMULE
#include "utility/logger.h"

#define MAX_UPLOAD_DATA_LEN (16*1024)
#define MAX_REQ_DATA_LEN (180*1024)


_int32 emule_upload_device_create(EMULE_UPLOAD_DEVICE** upload_device, EMULE_DATA_PIPE* emule_pipe)
{
	_int32 ret = SUCCESS;
	LOG_DEBUG("[emule_pipe = 0x%x]emule_upload_device_create.", emule_pipe);
	ret = sd_malloc(sizeof(EMULE_UPLOAD_DEVICE), (void**)upload_device);
	CHECK_VALUE(ret);
	sd_memset(*upload_device, 0, sizeof(EMULE_UPLOAD_DEVICE));
	list_init(&(*upload_device)->_upload_req_list);
	(*upload_device)->_close_flag = FALSE;
	(*upload_device)->_emule_pipe = emule_pipe;
	(*upload_device)->_is_own_cur_upload_buffer = FALSE;
	(*upload_device)->_reading_flag = FALSE;
	return ret;
}

_int32 emule_upload_device_close(EMULE_UPLOAD_DEVICE** upload_device)
{
	EMULE_UPLOAD_REQ* req = NULL;
	EMULE_UPLOAD_DEVICE* device = *upload_device;
	if(device == NULL)
		return SUCCESS;
	LOG_DEBUG("[emule_pipe = 0x%x]emule_upload_device_close._is_own_cur_upload_buffer:%d, _reading_flag:%d", 
     device->_emule_pipe, device->_is_own_cur_upload_buffer, device->_reading_flag);
	*upload_device = NULL;	//这里一定要置NULL，防止两次关闭
	while(list_size(&device->_upload_req_list) > 0)
	{
		list_pop(&device->_upload_req_list, (void**)&req);
		emule_free_emule_upload_req_slip(req);
	}
    emule_pipe_remove_user(device);

    if(device->_is_own_cur_upload_buffer == TRUE)
    {
        sd_assert( device->_cur_upload != NULL );
		sd_free(device->_cur_upload->_buffer);
		device->_cur_upload->_buffer = NULL;
    }
    
    if( device->_cur_upload != NULL )
    {
        emule_free_emule_upload_req_slip(device->_cur_upload);
        device->_cur_upload = NULL;
    }

	//如果正在上传直接设置close标志后返回，在回调中删除该upload_device
    device->_close_flag = TRUE;
    if(device->_reading_flag)
	{
        device->_emule_pipe = NULL;
		return SUCCESS;
	}

	if(device->_upload_data_buffer != NULL)
	{
		dm_free_buffer_to_data_buffer(device->_upload_data_buffer->_buffer_length, &device->_upload_data_buffer->_data_ptr);
		sd_free(device->_upload_data_buffer);
		device->_upload_data_buffer = NULL;
	}

	return sd_free(device);
}

_int32 emule_upload_recv_request(EMULE_UPLOAD_DEVICE* upload_device, _u64 start_pos1, _u64 end_pos1, _u64 start_pos2, _u64 end_pos2, _u64 start_pos3, _u64 end_pos3)
{
	_int32 ret = SUCCESS;
    
	LOG_DEBUG("[emule_pipe = 0x%x]emule_upload_recv_request.", upload_device->_emule_pipe);

    ret = emule_pipe_add_upload_req(upload_device, start_pos1, end_pos1);
    CHECK_VALUE(ret);
    
    ret = emule_pipe_add_upload_req(upload_device, start_pos2, end_pos2);
    CHECK_VALUE(ret);

    ret = emule_pipe_add_upload_req(upload_device, start_pos3, end_pos3);
    CHECK_VALUE(ret);    

/*
	if(end_pos1 > start_pos1)
	{
		ret = emule_get_emule_upload_req_slip(&req);
		CHECK_VALUE(ret);
		req->_data_pos = start_pos1;
		req->_data_len = (_u32)(end_pos1 - start_pos1);
        if(emule_pipe_is_upload_req_exist(upload_device, req))
        {
            emule_free_emule_upload_req_slip(req);
        }
        else
        {
            list_push(&upload_device->_upload_req_list, req);
        }
	}
	if(end_pos2 > start_pos2)
	{
		ret = emule_get_emule_upload_req_slip(&req);
		CHECK_VALUE(ret);
		req->_data_pos = start_pos2;
		req->_data_len = (_u32)(end_pos2 - start_pos2);
        if(emule_pipe_is_upload_req_exist(upload_device, req))
        {
            emule_free_emule_upload_req_slip(req);
        }
        else
        {
            list_push(&upload_device->_upload_req_list, req);
        }
	}
	if(end_pos3 > start_pos3)
	{
		ret = emule_get_emule_upload_req_slip(&req);
		CHECK_VALUE(ret);
		req->_data_pos = start_pos3;
		req->_data_len = (_u32)(end_pos3 - start_pos3);
        if(emule_pipe_is_upload_req_exist(upload_device, req))
        {
            emule_free_emule_upload_req_slip(req);
        }
        else
        {
            list_push(&upload_device->_upload_req_list, req);
        }
	}
	*/
	if(upload_device->_cur_upload == NULL)
		ret = emule_upload_process_request(upload_device);
	return ret;
}

_int32 emule_pipe_add_upload_req(EMULE_UPLOAD_DEVICE* upload_device, _u64 start_pos, _u64 end_pos)
{
    _int32 ret = SUCCESS;
    _u32 data_len = 0;
	EMULE_UPLOAD_REQ* req = NULL;
    
	if(end_pos <= start_pos) return SUCCESS;

    data_len = (_u32)(end_pos - start_pos);
	if(data_len > MAX_REQ_DATA_LEN) return SUCCESS;

    while(data_len>0)
    {
        ret = emule_get_emule_upload_req_slip(&req);
        CHECK_VALUE(ret);
        
        req->_data_pos = start_pos;
        if(data_len > MAX_UPLOAD_DATA_LEN)
        {
            req->_data_len = (_u32)(MAX_UPLOAD_DATA_LEN);
            start_pos += MAX_UPLOAD_DATA_LEN;
        }
        else
        {
            req->_data_len = data_len;
        }
        
        data_len -= req->_data_len;
        if(emule_pipe_is_upload_req_exist(upload_device, req))
        {
            emule_free_emule_upload_req_slip(req);
        }
        else
        {
            list_push(&upload_device->_upload_req_list, req);
        }
    }
    return SUCCESS;
}


BOOL emule_pipe_is_upload_req_exist(EMULE_UPLOAD_DEVICE* upload_device, EMULE_UPLOAD_REQ* req)
{
	EMULE_UPLOAD_REQ* list_req = NULL;
    LIST_ITERATOR cur_node = LIST_BEGIN(upload_device->_upload_req_list);
    while(cur_node!=LIST_END(upload_device->_upload_req_list))
    {
        list_req = (EMULE_UPLOAD_REQ*)LIST_VALUE(cur_node);
        if(req->_data_pos == list_req->_data_pos
            && req->_data_len == list_req->_data_len)
        {
            return TRUE;
        }
        cur_node = LIST_NEXT( cur_node );
    }
    return FALSE;
}

_int32 emule_upload_process_call_back(void* user)
{
    EMULE_UPLOAD_DEVICE* upload_device = (EMULE_UPLOAD_DEVICE*)user;
	LOG_DEBUG("[emule_pipe = 0x%x]emule_upload_process_call_back.", upload_device->_emule_pipe);
    if(upload_device->_cur_upload != NULL)
	{
        emule_pipe_device_try_send_data(upload_device);
	}
    else
    {
        emule_upload_process_request(upload_device);
    }
    return SUCCESS;
}

_int32 emule_upload_process_request(EMULE_UPLOAD_DEVICE* upload_device)
{
	_int32 ret = SUCCESS;
	char* tmp_buf = NULL;
	_int32 tmp_len = 0;
	BOOL is_64_offset = FALSE;
	EMULE_DATA_MANAGER* data_manager = (EMULE_DATA_MANAGER*)upload_device->_emule_pipe->_data_pipe._p_data_manager;
	sd_assert(upload_device->_cur_upload == NULL);
    //if(emule_pipe_device_can_send()== FALSE) return SUCCESS;
	list_pop(&upload_device->_upload_req_list, (void**)&upload_device->_cur_upload);
	if(upload_device->_cur_upload == NULL)
		return SUCCESS;			//没有请求要处理了
	LOG_DEBUG("[emule_pipe = 0x%x]emule_upload_process_request[%llu, %u].", upload_device->_emule_pipe, upload_device->_cur_upload->_data_pos, upload_device->_cur_upload->_data_len);
	//先填充协议头
	if(upload_device->_cur_upload->_data_pos + upload_device->_cur_upload->_data_len > 0xFFFFFFFF)
		is_64_offset = TRUE;
	if(is_64_offset)
		upload_device->_cur_upload->_buffer_len = upload_device->_cur_upload->_data_len + 38;
	else
		upload_device->_cur_upload->_buffer_len = upload_device->_cur_upload->_data_len + 30;
	ret = sd_malloc(upload_device->_cur_upload->_buffer_len, (void**)&upload_device->_cur_upload->_buffer);
	CHECK_VALUE(ret);
    upload_device->_is_own_cur_upload_buffer = TRUE;
	tmp_buf = upload_device->_cur_upload->_buffer;
	tmp_len = upload_device->_cur_upload->_buffer_len;
	if(is_64_offset)
	{
		sd_set_int8(&tmp_buf, &tmp_len, OP_EMULEPROT);
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, upload_device->_cur_upload->_buffer_len - EMULE_HEADER_SIZE);
		sd_set_int8(&tmp_buf, &tmp_len, OP_SENDINGPART_I64);
		sd_set_bytes(&tmp_buf, &tmp_len, (char*)data_manager->_file_id, FILE_ID_SIZE);
		sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64)upload_device->_cur_upload->_data_pos);
		ret = sd_set_int64_to_lt(&tmp_buf, &tmp_len, (_int64) (upload_device->_cur_upload->_data_pos + upload_device->_cur_upload->_data_len));
		upload_device->_cur_upload->_buffer_offset = 38;
	}
	else
	{
		sd_set_int8(&tmp_buf, &tmp_len, OP_EDONKEYHEADER);
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, upload_device->_cur_upload->_buffer_len - EMULE_HEADER_SIZE);
		sd_set_int8(&tmp_buf, &tmp_len, OP_SENDINGPART);
		sd_set_bytes(&tmp_buf, &tmp_len, (char*)data_manager->_file_id, FILE_ID_SIZE);
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)upload_device->_cur_upload->_data_pos);
		ret = sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)(upload_device->_cur_upload->_data_pos + upload_device->_cur_upload->_data_len));
		upload_device->_cur_upload->_buffer_offset = 30;
	}
	sd_assert(ret == SUCCESS);
    /*
	if(emule_upload_fill_data(upload_device) == TRUE)
	{
		LOG_DEBUG("[emule_pipe = 0x%x]emule_pipe send upload part_data[%llu, %u].", upload_device->_emule_pipe, upload_device->_cur_upload->_data_pos, upload_device->_cur_upload->_data_len);
		ret = emule_pipe_device_send_data(upload_device->_emule_pipe->_device, upload_device->_cur_upload->_buffer, upload_device->_cur_upload->_buffer_len);
		CHECK_VALUE(ret);
		emule_free_emule_upload_req_slip(upload_device->_cur_upload);
		upload_device->_cur_upload = NULL;
		return emule_upload_process_request(upload_device);
	}
	*/
	emule_upload_read_data(upload_device);
    return SUCCESS;
}

_int32 emule_upload_read_data(EMULE_UPLOAD_DEVICE* upload_device)
{
	_int32 ret = SUCCESS;
	_u32 data_offset = 0;
	EMULE_DATA_MANAGER* data_manager = (EMULE_DATA_MANAGER*)upload_device->_emule_pipe->_data_pipe._p_data_manager;
	if(upload_device->_upload_data_buffer == NULL)
	{
		ret = sd_malloc(sizeof(RANGE_DATA_BUFFER), (void**)&upload_device->_upload_data_buffer);
		sd_assert(ret == SUCCESS);
		upload_device->_upload_data_buffer->_data_length = 0;
		upload_device->_upload_data_buffer->_buffer_length = get_data_unit_size();
		ret = dm_get_buffer_from_data_buffer(&upload_device->_upload_data_buffer->_buffer_length, &upload_device->_upload_data_buffer->_data_ptr);
        if( ret != SUCCESS )
        {
            if(ret == DATA_BUFFER_IS_FULL || ret== OUT_OF_MEMORY)
                file_info_flush_data_to_file(&data_manager->_file_info, TRUE);
            sd_free(upload_device->_upload_data_buffer);
            upload_device->_upload_data_buffer = NULL;
            return ret;
        }
	}
	//这个offset算出了目前已经读出了多少填充了多少数据，第一次读的话offset
	//是0，如果连续读的话offset不为0，连续读表示这个request的数据跨越了两个range,
	//要读两次才能把需要的数据凑齐
	data_offset = upload_device->_cur_upload->_data_len - (upload_device->_cur_upload->_buffer_len - upload_device->_cur_upload->_buffer_offset);
	upload_device->_upload_data_buffer->_range._index = (upload_device->_cur_upload->_data_pos + data_offset) / get_data_unit_size();
	upload_device->_upload_data_buffer->_range._num = 1;
	upload_device->_upload_data_buffer->_data_length = get_data_unit_size();
	if(upload_device->_upload_data_buffer->_range._index == data_manager->_file_size / get_data_unit_size())
		upload_device->_upload_data_buffer->_data_length = data_manager->_file_size % get_data_unit_size();		//最后一块要进行规整
	ret = emule_read_data(data_manager, upload_device->_upload_data_buffer, emule_upload_read_data_callback, upload_device);
    LOG_DEBUG("[emule_pipe = 0x%x]emule_read_data:[%u, %u].", upload_device->_emule_pipe, 
        upload_device->_upload_data_buffer->_range._index, 
        upload_device->_upload_data_buffer->_range._num);

	if(ret == SUCCESS)
	{
		//在内存中，直接读到了
		if(emule_upload_fill_data(upload_device) == FALSE)
			return emule_upload_read_data(upload_device);
		LOG_DEBUG("[emule_pipe = 0x%x]emule_read_data SUCCESS, send upload part_data[%llu, %u].", upload_device->_emule_pipe, upload_device->_cur_upload->_data_pos, upload_device->_cur_upload->_data_len);

        return emule_pipe_device_try_send_data(upload_device);

/*
        emule_pipe_device_send_data(upload_device->_emule_pipe->_device, upload_device->_cur_upload->_buffer, upload_device->_cur_upload->_buffer_len, (void *)upload_device);
        emule_free_emule_upload_req_slip(upload_device->_cur_upload);
		upload_device->_cur_upload = NULL;
		return emule_upload_process_request(upload_device);
		*/
	}	
    if(ret == ERROR_WAIT_NOTIFY )
    {
        upload_device->_reading_flag = TRUE;
        return SUCCESS;
    }
	return ret;
}

_int32 emule_pipe_device_try_send_data(EMULE_UPLOAD_DEVICE* upload_device)
{
    _int32 ret = SUCCESS;
	LOG_DEBUG("[emule_pipe = 0x%x]emule_pipe_device_try_send_data.", upload_device->_emule_pipe);

    sd_assert(upload_device->_cur_upload != NULL);
    ret = emule_pipe_device_send_data(upload_device->_emule_pipe->_device, upload_device->_cur_upload->_buffer, 
        upload_device->_cur_upload->_buffer_len, (void*)upload_device);
    
    if( ret != SUCCESS )
    {
       if( ret == EMULE_SEND_DATA_WAIT )
        {
            ret = SUCCESS;
        }
       LOG_DEBUG("[emule_pipe = 0x%x]emule_pipe_device_try_send_data.ret:%d", upload_device->_emule_pipe, ret);
       return ret;
    }
    
    upload_device->_is_own_cur_upload_buffer = FALSE;
    emule_free_emule_upload_req_slip(upload_device->_cur_upload);
    upload_device->_cur_upload = NULL;
    return emule_upload_process_request(upload_device);
}

_int32 emule_upload_read_data_callback(struct tagTMP_FILE* file_struct, void *p_user, RANGE_DATA_BUFFER* data_buffer, _int32 read_result, _u32 read_len)
{
	EMULE_UPLOAD_DEVICE* upload_device = (EMULE_UPLOAD_DEVICE*)p_user;
	if(read_result == FM_READ_CLOSE_EVENT)
	{
		LOG_DEBUG("emule_upload_read_data_callback, but read result is FM_READ_CLOSE_EVENT.");
		return SUCCESS;
	}
    if(upload_device->_close_flag == TRUE)
	{
		LOG_DEBUG("[upload_device = 0x%x]emule_pipe_read_upload_data_callback, but close_flag is TRUE, destroy upload_device.", upload_device);
		sd_assert(list_size(&upload_device->_upload_req_list) == 0);
		//sd_free(upload_device->_cur_upload->_buffer);
		//emule_free_emule_upload_req_slip(upload_device->_cur_upload);
		dm_free_buffer_to_data_buffer(upload_device->_upload_data_buffer->_buffer_length, &upload_device->_upload_data_buffer->_data_ptr);
		sd_free(upload_device->_upload_data_buffer);
		upload_device->_upload_data_buffer = NULL;
		sd_free(upload_device);
		upload_device = NULL;
		return SUCCESS;
	}
    upload_device->_reading_flag = FALSE;
	sd_assert(read_result == SUCCESS);
	if(emule_upload_fill_data(upload_device) == FALSE)
	{
		emule_upload_read_data(upload_device);
        return SUCCESS;
	}
	LOG_DEBUG("[emule_pipe = 0x%x]emule_upload_read_data_callback send upload part_data[%llu, %u].", upload_device->_emule_pipe, upload_device->_cur_upload->_data_pos, upload_device->_cur_upload->_data_len);
/*	emule_pipe_device_send_data(upload_device->_emule_pipe->_device, upload_device->_cur_upload->_buffer, upload_device->_cur_upload->_buffer_len, (void*)upload_device);
	emule_free_emule_upload_req_slip(upload_device->_cur_upload);
	upload_device->_cur_upload = NULL;
	return emule_upload_process_request(upload_device);
    */
    return emule_pipe_device_try_send_data(upload_device);
}

BOOL emule_upload_fill_data(EMULE_UPLOAD_DEVICE* upload_device)
{
	_u64 pos = 0;
	_u32 offset = 0, len = 0, data_offset = 0;
	sd_assert(upload_device->_cur_upload != NULL);
	if(upload_device->_upload_data_buffer == NULL)
		return FALSE;
	if(upload_device->_upload_data_buffer->_data_ptr == NULL)
		return FALSE;
	pos = (_u64)get_data_unit_size() * upload_device->_upload_data_buffer->_range._index;
	//要先判断下要读的数据是否在range_data_buffer中
	if(upload_device->_cur_upload->_data_pos >= pos + upload_device->_upload_data_buffer->_data_length ||
	    upload_device->_cur_upload->_data_pos + upload_device->_cur_upload->_data_len <= pos)
	{
		//要读的数据与range_data_buffer没有交集，说明不在这个range_data_buffer中
		return FALSE;
	}
	//这个data_offset算出了目前已经读出了多少填充了多少数据，第一次读的话offset
	//是0，如果连续读的话offset不为0，连续读表示这个request的数据跨越了两个range,
	//要读两次才能把需要的数据凑齐
	data_offset = upload_device->_cur_upload->_data_len - (upload_device->_cur_upload->_buffer_len - upload_device->_cur_upload->_buffer_offset);
	//以下的offset计算出要读的数据偏移
	offset = upload_device->_cur_upload->_data_pos + data_offset - pos;
	len = MIN(upload_device->_cur_upload->_data_len - data_offset, upload_device->_upload_data_buffer->_data_length - offset);
	//填充命令中的数据
	sd_memcpy(upload_device->_cur_upload->_buffer + upload_device->_cur_upload->_buffer_offset, upload_device->_upload_data_buffer->_data_ptr + offset, len);
	upload_device->_cur_upload->_buffer_offset += len;
	if(upload_device->_cur_upload->_buffer_offset == upload_device->_cur_upload->_buffer_len)	
		return TRUE;
	else
		return FALSE;
}




#endif /* ENABLE_EMULE */
#endif /* ENABLE_EMULE */

