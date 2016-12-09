#include "utility/define.h"
#ifdef ENABLE_EMULE
#include "emule_part_checker.h"
#include "emule_data_manager.h"
#include "../emule_utility/emule_utility.h"
#include "utility/mempool.h"
#include "utility/md4.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_EMULE
#include "utility/logger.h"
#include "platform/sd_fs.h"

#define MAX_ONCE_READ_CHECK_RANGE_NUM (128)


_int32 emule_create_part_checker(EMULE_PART_CHECKER* part_checker, struct tagEMULE_DATA_MANAGER* data_manager)
{
	_int32 ret = SUCCESS;
	part_checker->_data_manager = data_manager;
	list_init(&part_checker->_part_index_list);
	part_checker->_cur_hash_index = INVALID_PART_INDEX;
	ret = sd_malloc(get_data_unit_size() * MAX_ONCE_READ_CHECK_RANGE_NUM, (void**)&part_checker->_data_buffer._data_ptr);
	CHECK_VALUE(ret);
    
	part_checker->_data_buffer._buffer_length = get_data_unit_size() * MAX_ONCE_READ_CHECK_RANGE_NUM;
	part_checker->_part_hash_len = 0;
	part_checker->_part_hash = NULL;
    
	part_checker->_aich_hash_len = 0;
	part_checker->_aich_hash = NULL;


	// EMULE跨文件两块数据保存，避免临快校验失败又覆盖写
	int i = 0;
	for(i = 0; i < 2; i++)
	{
		_int32 magic = CROSS_DATA_VALIED_MAGIC;
		
		part_checker->_cross_data_buffer[i]._buffer_length = get_data_unit_size() + CROSS_DATA_MAGIC_LENGTH;
		ret = sd_malloc(get_data_unit_size() + CROSS_DATA_MAGIC_LENGTH, (void**)&part_checker->_cross_data_buffer[i]._data_ptr);
		CHECK_VALUE(ret);		

		sd_memcpy(part_checker->_cross_data_buffer[i]._data_ptr, &magic, CROSS_DATA_MAGIC_LENGTH);
	}

    
	part_checker->_delay_read_msgid = INVALID_MSG_ID;
    
	LOG_DEBUG("emule_create_part_checker, checker = 0x%x.", part_checker);
	return ret;
}

_int32 emule_close_part_checker(EMULE_PART_CHECKER* part_checker)
{
	LOG_DEBUG("emule_close_part_checker, checker = 0x%x.", part_checker);
	if(part_checker->_delay_read_msgid != INVALID_MSG_ID)
		cancel_message_by_msgid(part_checker->_delay_read_msgid);
	list_clear(&part_checker->_part_index_list);
	sd_free(part_checker->_data_buffer._data_ptr);

	int i = 0;
	for(i = 0; i < 2; i++)
	{
		sd_free(part_checker->_cross_data_buffer[i]._data_ptr);
	}
	
	part_checker->_data_buffer._data_ptr = NULL;
	SAFE_DELETE(part_checker->_part_hash);
	SAFE_DELETE(part_checker->_aich_hash);
	return SUCCESS;
}

_int32 emule_checker_recv_range(EMULE_PART_CHECKER* checker, const RANGE* range)
{
	_u32 start_index, end_index;
	RANGE part_range;
	emule_range_to_exclude_part_index(	checker->_data_manager, range, &start_index, &end_index);
	while(start_index <= end_index)
	{
		emule_part_index_to_range(checker->_data_manager, start_index, &part_range);
		if(file_info_range_is_recv(&checker->_data_manager->_file_info, &part_range) == TRUE
          && file_info_range_is_check(&checker->_data_manager->_file_info, &part_range) == FALSE)
		{	//都收到了，那么可以进行校验
			list_push(&checker->_part_index_list, (void**)start_index);
			LOG_DEBUG("[emule_task = 0x%x]emule part_index = %u is recv and add to need check list.", checker->_data_manager->_emule_task, start_index);
			emule_check_next_part(checker);
		}
		++start_index;
	}
	return SUCCESS;
}

_int32 emule_checker_init_check_range(EMULE_PART_CHECKER* checker)
{
    RANGE_LIST_ITEROATOR cur_iter;
	_u32 start_index, end_index;
	RANGE part_range;
    RANGE_LIST* resv_range_list = file_info_get_recved_range_list(&checker->_data_manager->_file_info);
    LOG_DEBUG("[emule_task = 0x%x]emule_checker_init_check_range.", checker->_data_manager->_emule_task);

    if(resv_range_list == NULL) return SUCCESS;
    range_list_get_head_node( resv_range_list, &cur_iter );
	while( cur_iter != NULL )
	{	
       LOG_DEBUG("[emule_task = 0x%x]emule_checker_init_check_range, range[%u, %u] is recv and add to need check list.", 
        checker->_data_manager->_emule_task, cur_iter->_range._index, cur_iter->_range._num);
        emule_range_to_exclude_part_index( checker->_data_manager, &cur_iter->_range, &start_index, &end_index);
        while(start_index <= end_index)
        {
            emule_part_index_to_range(checker->_data_manager, start_index, &part_range);
            if(file_info_range_is_check(&checker->_data_manager->_file_info, &part_range) == FALSE)
            {   
                list_push(&checker->_part_index_list, (void**)start_index);
                LOG_DEBUG("[emule_task = 0x%x]emule_checker_init_check_range, part_index = %u is recv and add to need check list.", checker->_data_manager->_emule_task, start_index);
            }
            else
            {
                LOG_DEBUG("[emule_task = 0x%x]emule_checker_init_check_range, part_index = %u is recv and add check ok.", checker->_data_manager->_emule_task, start_index);
            }
            ++start_index;
        }

		range_list_get_next_node( resv_range_list, cur_iter, &cur_iter );
	}
    emule_check_next_part(checker);
    return SUCCESS;
}

//把range转化为包含整个range的part
_int32 emule_range_to_exclude_part_index(EMULE_DATA_MANAGER* data_manager, const RANGE* range, _u32* start_index, _u32* end_index)
{
	_u64 start_pos = (_u64)range->_index * get_data_unit_size();
	_u64 end_pos = start_pos + emule_get_range_size(data_manager, range);
	*start_index = start_pos / EMULE_PART_SIZE;
	*end_index = end_pos / EMULE_PART_SIZE;
	return SUCCESS;
}

//把range转化为被range包含的part
_int32 emule_range_to_include_part_index(EMULE_DATA_MANAGER* data_manager, const RANGE* range, _u32* index, _u32* num)
{
	_u32 end_index = 0;
	_u64 start_pos = (_u64)range->_index * get_data_unit_size();
	_u64 end_pos = start_pos + emule_get_range_size(data_manager, range);
	*index = (start_pos + EMULE_PART_SIZE - 1) / EMULE_PART_SIZE;
	end_index = end_pos / EMULE_PART_SIZE;
	if(end_pos == emule_get_file_size(data_manager))
		++end_index;
	if(end_index >= *index)
		*num = end_index - *index;
	else
		*num = 0;
	LOG_DEBUG("[emule_task = 0x%x]emule_range_to_include_part_index, range[%u, %u] to part[%u, %u].", data_manager->_emule_task, range->_index, range->_index + range->_num, *index, *index + *num);
	return SUCCESS;
}

//把part_index转为包含该完整part范围的range
_int32 emule_part_index_to_range(EMULE_DATA_MANAGER* data_manager, _u32 part_index, RANGE* part_range)
{
	_u64 pos = EMULE_PART_SIZE * (_u64)part_index;
	_u32 len = emule_get_part_size(data_manager, part_index);
	part_range->_index = pos / get_data_unit_size();
	part_range->_num = (pos + len + get_data_unit_size() - 1) / get_data_unit_size() - part_range->_index;
	return SUCCESS;
}

//把part转化为该part包含的range
_int32 emule_part_to_include_range(EMULE_DATA_MANAGER* data_manager, _u32 part_index, _u32 part_num, RANGE* part_range)
{
	_u64 pos = EMULE_PART_SIZE * (_u64)part_index;
	_u64 len = 0;
	_u32 index = part_index, num = part_num;
	while(num > 0)
	{
		len += emule_get_part_size(data_manager, index);
		++index;
		--num;
	}
	*part_range = pos_length_to_range2(pos, len, data_manager->_file_size);
	LOG_DEBUG("[emule_task = 0x%x]emule_part_to_include_range, part[%u, %u] to range[%u, %u].", data_manager->_emule_task, part_index, part_index + part_num, part_range->_index, part_range->_index + part_range->_num);
	return SUCCESS;
}

_int32 emule_check_next_part(EMULE_PART_CHECKER* checker)
{
	_u32 part_index = INVALID_PART_INDEX;
	if(checker->_cur_hash_index != INVALID_PART_INDEX)
		return SUCCESS;
	if(list_size(&checker->_part_index_list) == 0)
		return SUCCESS;
	list_pop(&checker->_part_index_list, (void**)&part_index);
	LOG_DEBUG("emule_check_part_hash, part_index = %u.", part_index);
	if(bitmap_emule_at(&checker->_data_manager->_part_bitmap, part_index))
	{
		LOG_DEBUG("emule_check_part_hash, part_index = %u, but this part had checked, just process next.", part_index);
		return emule_check_next_part(checker);
	}
	//开始校验当前的part块
	md4_initialize(&checker->_hash_result);
	checker->_cur_hash_index = part_index;

	int i = 0;
	for(i  = 0;i < 2; i++)
	{
		checker->_cross_data_buffer[i]._range._index = 0;
		checker->_cross_data_buffer[i]._range._num = 0;
	}
	
	emule_part_index_to_range(checker->_data_manager, part_index, &checker->_part_range);	
	return emule_check_part_hash(checker);
}

_int32 emule_check_part_hash(EMULE_PART_CHECKER* checker)
{
	_int32 ret = SUCCESS;
	MSG_INFO msg = {};
	while(TRUE)
	{
		checker->_data_buffer._range._index = checker->_part_range._index;
		checker->_data_buffer._range._num = emule_once_check_range_num(checker);
		checker->_data_buffer._data_length = emule_get_range_size(checker->_data_manager, &checker->_data_buffer._range);
		LOG_DEBUG("emule_check_part_hash, read_data, range[%u, %u], data_len = %u.", checker->_data_buffer._range._index,
					checker->_data_buffer._range._num, checker->_data_buffer._data_length);
		ret = emule_read_data(checker->_data_manager, &checker->_data_buffer, emule_notify_part_hash_read, checker);
		if(ret == ERROR_WAIT_NOTIFY || ret == FM_READ_CLOSE_EVENT)
			return SUCCESS;
		if(ret == ERROR_DATA_IS_WRITTING)
		{	//数据正在写，过一段时间再来读
			msg._device_id = 0;
			msg._user_data = checker;
			msg._device_type = DEVICE_NONE;
			msg._operation_type = OP_NONE;
			return post_message(&msg, emule_notify_delay_read, NOTICE_ONCE, 1000, &checker->_delay_read_msgid);
		}
		if(ret ==  -1)
		{
			//上面要读的range被纠错抹掉了，所以投递一个延迟读消息，等待这一块重新下回来
			LOG_DEBUG("emule checker read range[%u, %u] failed, this range is not recv, just wait 5 sec to read again.", checker->_data_buffer._range._index,
						checker->_data_buffer._range._num);
			//sd_assert(file_info_range_is_recv(&checker->_data_manager->_file_info, &checker->_data_buffer._range) == FALSE);
			msg._device_id = 0;
			msg._user_data = checker;
			msg._device_type = DEVICE_NONE;
			msg._operation_type = OP_NONE;
			return post_message(&msg, emule_notify_delay_read, NOTICE_ONCE, 5000, &checker->_delay_read_msgid);
			
		}
		//已经读取到
		sd_assert(ret == SUCCESS);
		emule_calc_part_hash(checker, &checker->_data_buffer);

		emule_cache_cross_buffer(checker);

		checker->_part_range._index += emule_once_check_range_num(checker);
		checker->_part_range._num  -= emule_once_check_range_num(checker);
		//++checker->_part_range._index;
		//--checker->_part_range._num;
		if(checker->_part_range._num == 0)
		{
			emule_verify_part_hash(checker);
			return emule_check_next_part(checker);
		}
	}
}

_int32 emule_notify_part_hash_read (struct tagTMP_FILE* file, void* user_data, RANGE_DATA_BUFFER* data_buffer, _int32 result, _u32 read_len)
{
	EMULE_PART_CHECKER* checker = (EMULE_PART_CHECKER*)user_data;
	if(result == FM_READ_CLOSE_EVENT)
	{
		LOG_DEBUG("emule_notify_part_hash_read, but read result is FM_READ_CLOSE_EVENT.");
		return FM_READ_CLOSE_EVENT;	
	}
	LOG_DEBUG("emule_notify_part_hash_read, range[%u, %u].", data_buffer->_range._index, data_buffer->_range._num);
	emule_calc_part_hash(checker, data_buffer);
	sd_assert(checker->_part_range._index == data_buffer->_range._index);

	emule_cache_cross_buffer(checker);


	checker->_part_range._index += emule_once_check_range_num( checker);
	checker->_part_range._num -= emule_once_check_range_num(checker);
	//++checker->_part_range._index;
	//--checker->_part_range._num;
	if(checker->_part_range._num == 0)
	{
		emule_verify_part_hash(checker);
		return emule_check_next_part(checker);
	}
	else
	{
		return emule_check_part_hash(checker);
	}
}

_int32 emule_calc_part_hash(EMULE_PART_CHECKER* checker, RANGE_DATA_BUFFER* data_buffer)
{
	_u32 offset = 0;
	_u64 part_start_pos, part_end_pos, range_start_pos, range_end_pos;
	part_start_pos = (_u64)checker->_cur_hash_index * EMULE_PART_SIZE;
	part_end_pos = (_u64)checker->_cur_hash_index * EMULE_PART_SIZE + emule_get_part_size(checker->_data_manager, checker->_cur_hash_index);
	range_start_pos = (_u64)data_buffer->_range._index * get_data_unit_size();
	range_end_pos = (_u64)data_buffer->_range._index * get_data_unit_size() + data_buffer->_data_length;
	if(part_start_pos > range_start_pos)
	{
		offset = part_start_pos - range_start_pos;
		sd_assert(offset < data_buffer->_data_length);
		md4_update(&checker->_hash_result, (const _u8*)(data_buffer->_data_ptr + offset), data_buffer->_data_length - offset);
	}
	else if(part_end_pos < range_end_pos)
	{
		offset = part_end_pos - range_start_pos;
		sd_assert(offset < data_buffer->_data_length);
		md4_update(&checker->_hash_result, (const _u8*)data_buffer->_data_ptr, offset);
	}
	else
	{
		md4_update(&checker->_hash_result, (const _u8*)data_buffer->_data_ptr, data_buffer->_data_length);
	}
	return SUCCESS;
}

_int32 emule_verify_part_hash(EMULE_PART_CHECKER* checker)
{
	_u8	md4_result[16];
	_int32 check_result = -1;
	md4_finish(&checker->_hash_result, md4_result);
	if(checker->_part_hash != NULL)
		check_result = sd_memcmp(md4_result, checker->_part_hash + 16 * checker->_cur_hash_index, 16);

	
	if(0 == check_result)
	{
		emule_save_cross_data(checker);
	}
	
		
	emule_notify_check_part_hash_result(checker->_data_manager, checker->_cur_hash_index, check_result);

	
	
	checker->_cur_hash_index = INVALID_PART_INDEX;
	return SUCCESS;
}

_int32 emule_notify_delay_read(const MSG_INFO* msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid)
{
	EMULE_PART_CHECKER* checker = (EMULE_PART_CHECKER*)msg_info->_user_data;
	if(errcode == MSG_CANCELLED)
		return SUCCESS;
	LOG_DEBUG("emule_notify_delay_read.checker:0x%x", checker);
	checker->_delay_read_msgid = INVALID_MSG_ID;
	return emule_check_part_hash(checker);
}

void emule_cache_cross_buffer(EMULE_PART_CHECKER *checker)
{
	_u32 range_index =  checker->_data_buffer._range._index;

	
	_u64 start_pos = range_index;
	start_pos *= (_u64)get_data_unit_size();
	_u64 end_pos = start_pos + (_u64)emule_get_range_size(checker->_data_manager, &checker->_data_buffer._range);

	_u32 begin_part_index = start_pos / EMULE_PART_SIZE;
	_u32 end_part_index = (end_pos - 1) / EMULE_PART_SIZE;


	BOOL ret = (begin_part_index == end_part_index ? FALSE:TRUE);

	LOG_DEBUG("emule_cache_cross_buffer enter range(%u,%u) partindex(%u,%u) is cross range ret:%d", checker->_data_buffer._range._index,  checker->_data_buffer._range._num, begin_part_index, end_part_index, ret);


	if(!ret)
	{
		return;
	}


	int buffer_idx =  begin_part_index < checker->_cur_hash_index ? 0 : 1;
	
	LOG_DEBUG("emule_cache_cross_buffer   range(%u,%u) checker->_cur_hash_index:%u, buffer_idx:%d", checker->_data_buffer._range._index, checker->_data_buffer._range._num,  checker->_cur_hash_index, buffer_idx);		

	_u32  nebor_part_index = 0;
	if(buffer_idx == 0)
	{
		// 第0个part不可能有前面的跨块
		sd_assert(checker->_cur_hash_index != 0);
		nebor_part_index = checker->_cur_hash_index - 1;		
	}
	else
	{
		nebor_part_index = checker->_cur_hash_index + 1;
	}

	if(bitmap_emule_at(&checker->_data_manager->_part_bitmap, nebor_part_index))
	{
		LOG_DEBUG("emule_cache_cross_buffer nebor part has check succ , no need save..  range(%u,%u) checker->_cur_hash_index:%u, buffer_idx:%d", range_index, 1,  checker->_cur_hash_index, buffer_idx);		
		return;
	}


	_u32  cross_range_index = 0;
	if(buffer_idx == 0)
	{
		cross_range_index =checker->_data_buffer._range._index;
	}
	else
	{
		cross_range_index = checker->_data_buffer._range._index + checker->_data_buffer._range._num - 1;
	}



	

	checker->_cross_data_buffer[buffer_idx]._range._index = cross_range_index;
	checker->_cross_data_buffer[buffer_idx]._range._num = 1;
	_u32 cross_data_length = emule_get_range_size(checker->_data_manager, &checker->_cross_data_buffer[buffer_idx]._range);;
	_u32 cross_range_buffer_offset = (cross_range_index - range_index)*get_data_unit_size();
	
	checker->_cross_data_buffer[buffer_idx]._data_length = cross_data_length;

	sd_memcpy(checker->_cross_data_buffer[buffer_idx]._data_ptr + CROSS_DATA_MAGIC_LENGTH, checker->_data_buffer._data_ptr + cross_range_buffer_offset, cross_data_length);


	LOG_DEBUG("emule_cache_cross_buffer   range(%u,%u) checker->_cur_hash_index:%u, buffer_idx:%d, cross_range_index=%u, cross_data_length=%u,cross_range_buffer_offset=%u", checker->_data_buffer._range._index, checker->_data_buffer._range._num,  checker->_cur_hash_index, buffer_idx,
	 cross_range_index, cross_data_length, cross_range_buffer_offset);		
						
}


void emule_save_cross_data(EMULE_PART_CHECKER * checker)
{
	int i = 0;
	_u32 file_id = 0;
	_u32 write_bytes = 0;
	_u32 need_write_bytes = 0;

       _int32 ret = sd_open_ex(checker->_data_manager->_cross_file_path, O_FS_RDWR|O_FS_CREATE, &file_id);

	if(ret != SUCCESS)
	{
		LOG_ERROR("emule_save_cross_data open file %s, failed... ret:%d", checker->_data_manager->_cross_file_path, ret);
		return;
	}


	for(i = 0; i < 2; i ++)
	{
		RANGE_DATA_BUFFER* data_buffer = &checker->_cross_data_buffer[i];

		if(data_buffer->_range._index == 0)
		{
			// 文件开始位置不可能为跨块
			sd_assert(data_buffer->_range._num == 0);
			continue;
		}

		sd_assert(data_buffer->_range._num == 1);

		need_write_bytes = (emule_get_range_size(checker->_data_manager, &data_buffer->_range)+ CROSS_DATA_MAGIC_LENGTH);

		// 偏移计算，先得到文件偏移，再计算part_index，然后再乘range大小
		_u64 offset = data_buffer->_range._index;
		offset *= get_data_unit_size();
		offset /= EMULE_PART_SIZE;
		offset *= (get_data_unit_size() + CROSS_DATA_MAGIC_LENGTH);

		ret = sd_setfilepos(file_id, offset);
		if(ret != SUCCESS)
		{
			LOG_ERROR("emule_save_cross_data sd_setfilepos....ret=%d, offset=%llu", ret, offset);
			continue;
		}
		
		ret = sd_write(file_id, data_buffer->_data_ptr, need_write_bytes, &write_bytes);
		if(ret == SUCCESS)
		{
			sd_assert(write_bytes == need_write_bytes);
		}
		else
		{
			LOG_ERROR("emule_save_cross_data write failure.....ret=%d, write_bytes=%u", ret, write_bytes);
		}

		LOG_DEBUG("emule_save_cross_data range(%u, %u),  offset:%llu, need_write_bytes=%u, filename:%s", data_buffer->_range._index, data_buffer->_range._num, offset, need_write_bytes, checker->_data_manager->_cross_file_path);

	}

	sd_close_ex(file_id);

}


_u32  emule_once_check_range_num(EMULE_PART_CHECKER* checker)
{

	if(checker->_part_range._num > MAX_ONCE_READ_CHECK_RANGE_NUM)
	{
		return MAX_ONCE_READ_CHECK_RANGE_NUM;
	}

	return checker->_part_range._num;

}

#endif /* ENABLE_EMULE */

