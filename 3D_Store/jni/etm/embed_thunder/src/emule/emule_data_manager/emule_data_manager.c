#include "utility/define.h"
#ifdef ENABLE_EMULE
#include "upload_manager/upload_manager.h"
#include "emule_data_manager.h"
#include "../emule_interface.h"
#include "../emule_impl.h"
#include "../emule_utility/emule_ed2k_link.h"
#include "dispatcher/dispatcher_interface_info.h"
#include "platform/sd_fs.h"
#include "utility/mempool.h"
#include "utility/sd_assert.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_EMULE
#include "utility/logger.h"
#include "p2p_data_pipe/p2p_pipe_interface.h"
#include "http_data_pipe/http_resource.h"

#include "utility/utility.h"
#include "reporter/reporter_interface.h"

_int32 emule_create_data_manager(EMULE_DATA_MANAGER** manager, ED2K_LINK_INFO* ed2k_info, char* file_path, struct tagEMULE_TASK* emule_task)
{
	_int32 ret = SUCCESS;
	_u16 part_count = 0;
	RANGE_LIST_ITEROATOR node = NULL;
	_u32 index = 0, num = 0;
	FILE_INFO_CALLBACK emule_file_info_callback;
	char full_td_name[MAX_FILE_PATH_LEN] = { 0 }, full_cfg_name[MAX_FILE_PATH_LEN] = { 0 };
	char success_file_name[MAX_FILE_PATH_LEN] = { 0 };
	_u64 disk_file_size = 0;
	EMULE_DATA_MANAGER* data_manager = NULL;


	LOG_ERROR("emule_create_data_manager.file_name:%s, file_size:%llu", ed2k_info->_file_name, ed2k_info->_file_size);
	ret = sd_malloc(sizeof(EMULE_DATA_MANAGER), (void**)&data_manager);
	CHECK_VALUE(ret);
	sd_memset(data_manager, 0, sizeof(EMULE_DATA_MANAGER));
	data_manager->_is_cid_and_gcid_valid = FALSE;
	data_manager->_is_task_deleting = FALSE;
	range_list_init(&data_manager->_priority_range_list);
	range_list_init(&data_manager->_recv_range_list);
	range_list_init(&data_manager->_uncomplete_range_list);
	range_list_init(&data_manager->_3part_cid_list);
	data_manager->_file_size = ed2k_info->_file_size;
	sd_memcpy(data_manager->_file_id, ed2k_info->_file_id, FILE_ID_SIZE);

	compute_3part_range_list(data_manager->_file_size, &data_manager->_3part_cid_list);

	data_manager->_next_start_download_pos = 0;
	data_manager->_server_dl_bytes = 0;
	data_manager->_peer_dl_bytes = 0;
	data_manager->_emule_dl_bytes = 0;
	data_manager->_cdn_dl_bytes = 0;
	data_manager->_normal_cdn_dl_bytes = 0;
	data_manager->_lixian_dl_bytes = 0;
	data_manager->_origin_dl_bytes = 0;
	data_manager->_assist_peer_dl_bytes = 0;

	init_file_info(&data_manager->_file_info, data_manager);
	file_info_set_bcid_enable(&data_manager->_file_info, FALSE);
	file_info_set_user_name(&data_manager->_file_info, ed2k_info->_file_name, file_path);
	sd_memcpy(data_manager->_file_info._path, file_path, strlen(file_path));
	file_info_set_filesize(&data_manager->_file_info, ed2k_info->_file_size);
	sd_memcpy(success_file_name, file_path, sd_strlen(file_path));
	sd_strcat(success_file_name, ed2k_info->_file_name, sd_strlen(ed2k_info->_file_name));
	sd_strcat(ed2k_info->_file_name, ".rf", 3);
	file_info_set_final_file_name(&data_manager->_file_info, ed2k_info->_file_name);
	file_info_set_td_flag(&data_manager->_file_info);
	sd_memcpy(full_td_name, file_path, sd_strlen(file_path));
	sd_strcat(full_td_name, ed2k_info->_file_name, sd_strlen(ed2k_info->_file_name));
	sd_memcpy(full_cfg_name, file_path, sd_strlen(file_path));
	sd_strcat(full_cfg_name, ed2k_info->_file_name, sd_strlen(ed2k_info->_file_name));

	sd_strncpy(data_manager->_cross_file_path, full_cfg_name, MAX_FILE_PATH_LEN);
	sd_strcat(data_manager->_cross_file_path, ".crs", sd_strlen(".crs"));

	sd_strcat(full_cfg_name, ".cfg", sd_strlen(".cfg"));

	data_manager->_is_file_exist = FALSE;
	if (sd_file_exist(success_file_name))
	{
		ret = sd_get_file_size_and_modified_time(success_file_name, &disk_file_size, NULL);
		sd_assert(ret == SUCCESS);

		LOG_ERROR("emule_create_data_manager, file_exist:%s, file_size:%llu", success_file_name, disk_file_size);
		if (disk_file_size == ed2k_info->_file_size)
		{
			data_manager->_is_file_exist = TRUE;
		}
	}
	if (sd_file_exist(full_td_name) == TRUE && sd_file_exist(full_cfg_name) == TRUE)
	{
		file_info_load_from_cfg_file(&data_manager->_file_info, full_cfg_name);
		emule_date_manager_adjust_uncomplete_range(data_manager);
		emule_task->_task._is_continue = TRUE;
	}
	part_count = (_u16)(ed2k_info->_file_size / EMULE_PART_SIZE + 1);
	LOG_DEBUG("emule_create_data_manager part_count = %hu.", part_count);
	bitmap_init(&data_manager->_part_bitmap);
	ret = bitmap_resize(&data_manager->_part_bitmap, part_count);
	if (ret != SUCCESS)
		return ret;

	//把range_list转到part_bitmap
	range_list_get_head_node(&data_manager->_file_info._done_ranges, &node);
	while (node != NULL)
	{
		emule_range_to_include_part_index(data_manager, &node->_range, &index, &num);
		while (num > 0)
		{
			bitmap_emule_set(&data_manager->_part_bitmap, index, TRUE);
			++index;
			--num;
		}
		range_list_get_next_node(&data_manager->_file_info._done_ranges, node, &node);
	}

	emule_file_info_callback._p_notify_create_event = emule_notify_file_create_result;
	emule_file_info_callback._p_notify_check_event = emule_notify_file_block_check_result;
	emule_file_info_callback._p_notify_close_event = emule_notify_file_close_result;
	emule_file_info_callback._p_notify_file_result_event = emule_notify_file_result;
	file_info_register_callbacks(&data_manager->_file_info, &emule_file_info_callback);
	data_manager->_file_success_flag = FALSE;
	emule_create_part_checker(&data_manager->_part_checker, data_manager);
	//小于9.28M的文件没有part_hash, 使用文件hash进行校验
	if (data_manager->_file_size < EMULE_PART_SIZE)
		emule_set_part_hash(data_manager, data_manager->_file_id, FILE_ID_SIZE);
	data_manager->_emule_task = emule_task;
	init_range_manager(&data_manager->_range_manager);
	init_correct_manager(&data_manager->_correct_manager);
	*manager = data_manager;
	emule_checker_init_check_range(&data_manager->_part_checker);
	return SUCCESS;
}

// 最大设置为1G
_int32 emule_date_manager_adjust_uncomplete_range(EMULE_DATA_MANAGER* data_manager)
{
	RANGE file_range;
	_u64 range_len = 0;

	LOG_DEBUG("emule_date_manager_adjust_uncomplete_range, data_manager = 0x%x, _next_start_download_pos:%llu.",
		data_manager, data_manager->_next_start_download_pos);
	if (data_manager->_next_start_download_pos >= data_manager->_file_size) return SUCCESS;
	if (data_manager->_next_start_download_pos + MAX_CUR_DOWNLOADING_SIZE > data_manager->_file_size)
	{
		range_len = data_manager->_file_size - data_manager->_next_start_download_pos;
	}
	else
	{
		range_len = MAX_CUR_DOWNLOADING_SIZE;
	}

	file_range = pos_length_to_range(data_manager->_next_start_download_pos, range_len, data_manager->_file_size);

	range_list_add_range(&data_manager->_uncomplete_range_list, &file_range, NULL, NULL);
	range_list_delete_range_list(&data_manager->_uncomplete_range_list, &data_manager->_file_info._writed_range_list);
	data_manager->_next_start_download_pos += range_len;

	return SUCCESS;
}

_int32 emule_close_data_manager(EMULE_DATA_MANAGER* data_manager)
{
	EMULE_TASK* emule_task = data_manager->_emule_task;

	LOG_DEBUG("emule_close_data_manager, data_manager = 0x%x.", data_manager);
	if (data_manager->_file_info._tmp_file_struct != NULL)
	{
		file_info_close_tmp_file(&data_manager->_file_info);
		data_manager->_is_task_deleting = TRUE;
		return TM_ERR_WAIT_FOR_SIGNAL;
	}
	if (emule_task->_task.need_remove_tmp_file == TRUE)
	{
		LOG_DEBUG("[emule_task = 0x%x]emule need_remove_tmp_file.", emule_task);
		file_info_delete_cfg_file(&data_manager->_file_info);
		sd_delete_file(data_manager->_cross_file_path);
		file_info_delete_tmp_file(&data_manager->_file_info);
	}
	emule_close_part_checker(&data_manager->_part_checker);
	unit_range_manager(&data_manager->_range_manager);
	unit_correct_manager(&data_manager->_correct_manager);
	return emule_destroy_data_manager(data_manager);
}

_int32 emule_destroy_data_manager(EMULE_DATA_MANAGER* data_manager)
{
	bitmap_uninit(&data_manager->_part_bitmap);
	unit_file_info(&data_manager->_file_info);
	range_list_clear(&data_manager->_priority_range_list);
	range_list_clear(&data_manager->_recv_range_list);
	range_list_clear(&data_manager->_uncomplete_range_list);
	data_manager->_emule_task->_data_manager = NULL;
	sd_memset(data_manager, 0, sizeof(EMULE_DATA_MANAGER));
	return sd_free(data_manager);
}

_int32 emule_set_part_hash(EMULE_DATA_MANAGER* data_manager, const _u8* part_hash, _u32 part_hash_len)
{
	_int32 ret = SUCCESS;

	LOG_DEBUG("emule_set_part_hash, data_manager = 0x%x, part_hash_len = %u.", data_manager, part_hash_len);
	if (part_hash_len == 0) return ret;
	if (data_manager->_part_checker._part_hash_len > 0)
	{
		sd_assert(data_manager->_part_checker._part_hash != NULL);
		return ret;
	}
	ret = sd_malloc(part_hash_len, (void**)&data_manager->_part_checker._part_hash);
	if (ret != SUCCESS)
	{
		LOG_ERROR("emule_set_part_hash failed, data_manager = 0x%x, part_hash_len = %u, errcode = %d.",
			data_manager, part_hash_len, ret);
		sd_assert(FALSE);
		return ret;
	}
	sd_memcpy(data_manager->_part_checker._part_hash, part_hash, part_hash_len);
	data_manager->_part_checker._part_hash_len = part_hash_len;
	return ret;
}

_int32 emule_set_aich_hash(EMULE_DATA_MANAGER* data_manager, const _u8* aich_hash, _u32 aich_hash_len)
{
	_int32 ret = SUCCESS;

	LOG_DEBUG("emule_set_aich_hash, data_manager = 0x%x, aich_hash_len = %u.", data_manager, aich_hash_len);
	if (aich_hash_len == 0) return ret;
	if (data_manager->_part_checker._aich_hash_len > 0)
	{
		sd_assert(data_manager->_part_checker._aich_hash != NULL);
		return ret;
	}
	ret = sd_malloc(aich_hash_len, (void**)&data_manager->_part_checker._aich_hash);
	if (ret != SUCCESS)
	{
		LOG_ERROR("emule_set_aich_hash failed, data_manager = 0x%x, aich_hash_len = %u, errcode = %d.", 
			data_manager, aich_hash_len, ret);
		sd_assert(FALSE);
		return ret;
	}
	sd_memcpy(data_manager->_part_checker._aich_hash, aich_hash, aich_hash_len);
	data_manager->_part_checker._aich_hash_len = aich_hash_len;
	return ret;
}


BOOL emule_is_part_hash_valid(EMULE_DATA_MANAGER* data_manager)
{
	if (data_manager->_part_checker._part_hash_len == 0)
		return FALSE;
	else
		return TRUE;
}

BOOL emule_is_aich_hash_valid(EMULE_DATA_MANAGER* data_manager)
{
	if (data_manager->_part_checker._aich_hash_len == 0)
		return FALSE;
	else
		return TRUE;
}

const RANGE_LIST* emule_get_recved_range_list(EMULE_DATA_MANAGER* data_manager)
{
	return &data_manager->_file_info._data_receiver._total_receive_range;
}

_int32 emule_read_data(EMULE_DATA_MANAGER* data_manager, RANGE_DATA_BUFFER* data_buffer, notify_read_result callback, void* user_data)
{
	return file_info_read_data(&data_manager->_file_info, data_buffer, callback, user_data);
}


void  emule_merge_cross_range(EMULE_DATA_MANAGER * data_manager, const RANGE * range, char * * data_buffer, _u32 data_len, _u32 buffer_len)
{

	char* pbuffer = *data_buffer;
	RANGE data_r = *range;
	_u32  data_length = data_len;
	FILEINFO* p_file_info = &data_manager->_file_info;

	static char s_buffer[RANGE_DATA_UNIT_SIZE + CROSS_DATA_MAGIC_LENGTH];

	LOG_DEBUG("emule_merge_cross_range , the range is (%u,%u), data_len is %u, buffer_length:%u, buffer:0x%x .",
		data_r._index, data_r._num, data_length, buffer_len, *data_buffer);

	if (p_file_info->_file_size != 0)
	{
		if (((_u64)(data_r._index))*(_u64)get_data_unit_size() + data_length > p_file_info->_file_size)
		{
			data_length = p_file_info->_file_size - ((_u64)(data_r._index))*(_u64)get_data_unit_size();
			data_r._num = (data_length + (get_data_unit_size() - 1)) / get_data_unit_size();
		}
	}

	if (data_r._num*get_data_unit_size() != data_length)
	{

		LOG_DEBUG("emule_merge_cross_range data_length:%u is not full range.", data_length);

		if (p_file_info->_file_size != 0)
		{
			if (((_u64)(data_r._index))*get_data_unit_size() + data_length  < p_file_info->_file_size)
			{
				data_r._num = data_length / get_data_unit_size();
				data_length = data_r._num*get_data_unit_size();
			}
			LOG_DEBUG("emule_merge_cross_range data_length:, after process, the range is (%u,%u), data_len is %u.",
				data_r._index, data_r._num, data_length);

			if (data_length == 0 || data_r._index == MAX_U32 || data_r._index*get_data_unit_size() + data_length > p_file_info->_file_size)
			{
				LOG_ERROR("emule_merge_cross_range data_length:, after process, the range is (%u,%u), data_len is %u, because data_len=0, so drop buffer.",
					data_r._index, data_r._num, data_length);
				return;
			}
		}
		else
		{
			LOG_DEBUG("emule_merge_cross_range data_length:%u is not full range, filesize is invalid.", data_length);
		}
	}


	if (file_info_range_is_recv(p_file_info, &data_r) == TRUE)
	{
		LOG_ERROR("emule_merge_cross_range data_length:, the range is (%u,%u), data_len is %u, buffer_length:%u, buffer:0x%x  is recved, so drop it.",
			data_r._index, data_r._num, data_length, buffer_len, *data_buffer);
		return;
	}



	_u32 cur_range_index = 0, end_range_index = 0;
	cur_range_index = data_r._index;
	end_range_index = data_r._index + data_r._num - 1;

	sd_assert(cur_range_index <= end_range_index);

	for (; cur_range_index <= end_range_index; cur_range_index++)
	{
		RANGE r;
		r._index = cur_range_index;
		r._num = 1;
		_u64 start_pos = ((_u64)get_data_unit_size())* cur_range_index;
		_u64 end_pos = start_pos + (_u64)emule_get_range_size(data_manager, &r);

		_u32 begin_part_index = start_pos / EMULE_PART_SIZE;
		_u32 end_part_index = (end_pos - 1) / EMULE_PART_SIZE;


		BOOL is_cross = (begin_part_index == end_part_index ? FALSE : TRUE);

		if (!is_cross)
		{
			continue;
		}


		sd_assert((end_part_index - begin_part_index) == 1);

		BOOL is_before_merge = (bitmap_emule_at(&data_manager->_part_bitmap, begin_part_index) ? TRUE : FALSE);
		BOOL is_end_merge = (bitmap_emule_at(&data_manager->_part_bitmap, end_part_index) ? TRUE : FALSE);

		if (!is_before_merge && !is_end_merge)
		{
			LOG_DEBUG("emule_merge_cross_range befor and end not check succ now , no need merge");
			continue;
		}

		if (is_before_merge && is_end_merge)
		{
			LOG_ERROR("emule_merge_cross_range befor and end check succ now , no need merge");
			continue;
		}

		_u64 merge_size = 0;
		_u64 cross_offset = begin_part_index* (get_data_unit_size() + CROSS_DATA_MAGIC_LENGTH);
		_u64 merge_buffer_offset = 0;
		_u64 need_read_size = end_pos - start_pos + CROSS_DATA_MAGIC_LENGTH;
		_u64 range_offset = 0;

		if (is_before_merge)
		{
			merge_size = ((_u64)end_part_index)*EMULE_PART_SIZE - start_pos;
			merge_buffer_offset = ((_u64)(cur_range_index - data_r._index)) * ((_u64)get_data_unit_size());
			range_offset = 0;
		}
		else
		{
			merge_size = end_pos - ((_u64)end_part_index)*EMULE_PART_SIZE;
			range_offset = (((_u64)end_part_index)*((_u64)EMULE_PART_SIZE)) % ((_u64)get_data_unit_size());
			merge_buffer_offset = (cur_range_index - data_r._index) * get_data_unit_size() + range_offset;
		}

		LOG_DEBUG("emule_merge_cross_range isbefore:%d, isend:%d, range(%u,%u), part_idx(%u,%u), merge_size:%llu, merge_buffer_offset:%llu, cross_offset:%llu need_read_size=%llu, filesize:%llu, range_offset:%llu",
			is_before_merge, is_end_merge, cur_range_index, 1, begin_part_index, end_part_index, merge_size, merge_buffer_offset,
			cross_offset
			, need_read_size, data_manager->_file_info._file_size, range_offset);

		_u32 file_id = 0;
		_int32 ret = sd_open_ex(data_manager->_cross_file_path, O_FS_RDWR | O_FS_CREATE, &file_id);

		if (ret != SUCCESS)
		{
			LOG_ERROR("emule_merge_cross_range open file %s, failed... ret:%d", data_manager->_cross_file_path, ret);
			continue;
		}

		ret = sd_setfilepos(file_id, cross_offset);
		if (ret != SUCCESS)
		{
			LOG_ERROR("emule_merge_cross_range  setfilepos failed offset:%llu... ret:%d", cross_offset, ret);
			sd_close_ex(file_id);
			continue;
		}

		_u32 readsize = 0;
		sd_memset(s_buffer, 0, sizeof(s_buffer));
		ret = sd_read(file_id, s_buffer, need_read_size, &readsize);

		if (ret != SUCCESS || readsize != need_read_size)
		{
			LOG_ERROR("emule_save_cross_data write failure.....ret=%d, readsize=%u", ret, readsize);
			sd_close_ex(file_id);
			continue;
		}

		_int32 magic = 0;
		sd_memcpy(&magic, s_buffer, CROSS_DATA_MAGIC_LENGTH);
		if (magic != CROSS_DATA_VALIED_MAGIC)
		{
			LOG_ERROR("emule_save_cross_data  magic is not valid...magic=%x", magic);
			sd_close_ex(file_id);
			continue;
		}

		sd_memcpy(pbuffer + merge_buffer_offset, s_buffer + range_offset + CROSS_DATA_MAGIC_LENGTH, merge_size);
		sd_close_ex(file_id);
	}


}

_int32 emule_write_data(EMULE_DATA_MANAGER* data_manager, const RANGE* range, char** data_buffer, _u32 data_len, _u32 buffer_len, RESOURCE* res)
{
	_int32 ret = SUCCESS;
	//有空能任务出现错误了，状态为TASK_S_FAILED,正在等待上层来关闭，但此时它的pipe又收到
	//数据，调用该函数写数据，对于这种情况应该过滤掉，否则会导致内存非法读写崩溃
	if (data_manager->_emule_task->_task.task_status != TASK_S_RUNNING)
	{
		dm_free_buffer_to_data_buffer(data_len, data_buffer);
		return SUCCESS;
	}
	LOG_DEBUG("[emule_task = 0x%x]emule_write_data, range[%u, %u].", data_manager->_emule_task, range->_index, range->_num);
	/*
		if(file_info_range_is_recv(&data_manager->_file_info, range) == TRUE)
		{
		LOG_DEBUG("emule_write_data, but range[%u, %u] is recved, discard this range.", range->_index, range->_num);
		return dm_free_buffer_to_data_buffer(data_len, data_buffer);
		}
		*/


	emule_merge_cross_range(data_manager, range, data_buffer, data_len, buffer_len);

	ret = file_info_put_data(&data_manager->_file_info, range, data_buffer, data_len, buffer_len);
	if (ret == SUCCESS)
	{
		*data_buffer = NULL;
		range_list_delete_range(&data_manager->_uncomplete_range_list, range, NULL, NULL);
		range_list_add_range(&data_manager->_recv_range_list, range, NULL, NULL);
		put_range_record(&data_manager->_range_manager, res, range);
	}
	else
	{
#ifdef EMBED_REPORT
		file_info_add_overloap_date(&data_manager->_file_info, data_len);
#endif
		dm_free_buffer_to_data_buffer(data_len, data_buffer);
		//sd_assert(FALSE);
		return SUCCESS;
	}
	emule_checker_recv_range(&data_manager->_part_checker, range);

#ifdef EMBED_REPORT
	file_info_handle_valid_data_report(&data_manager->_file_info, res, data_len);
#endif
	LOG_DEBUG("emule_write_data , the resource type = %d , is_cdn_resource = %d", res->_resource_type, p2p_is_cdn_resource(res));
	if (res->_resource_type == HTTP_RESOURCE
		|| res->_resource_type == FTP_RESOURCE)
	{
		if ((res->_resource_type == HTTP_RESOURCE) && http_resource_is_lixian((HTTP_SERVER_RESOURCE *)res))
		{
			data_manager->_lixian_dl_bytes += data_len;
		}
		else
		{
			data_manager->_server_dl_bytes += data_len;//以后要将离线下载的数据从_server_dl_bytes中区分开来
		}

		if ((res->_resource_type == HTTP_RESOURCE) && http_resource_is_origin((HTTP_SERVER_RESOURCE *)res))
		{
			BOOL bRet = file_info_range_is_write(&data_manager->_file_info, (RANGE*)range);
			LOG_DEBUG("emule_write_data, file_info_range_is_write: %d", bRet);
			// 新收到的原始数据已经在写入文件的RANGE列表中，则该数据不需要记录在原始资源字段
			if (FALSE == bRet)
			{
				data_manager->_origin_dl_bytes += data_len;
			}
		}

	}
	else if (res->_resource_type == PEER_RESOURCE)
	{
#ifdef ENABLE_HSC
		if (p2p_get_from(res) == P2P_FROM_VIP_HUB)
		{
			data_manager->_emule_task->_task._hsc_info._dl_bytes += data_len;
			LOG_DEBUG("emule_write_data, resouce from vip hub, downloaded bytes: %llu",
				data_manager->_emule_task->_task._hsc_info._dl_bytes);
		}
#endif /* ENABLE_HSC */
		LOG_DEBUG("emule_write_data , p2p_get_from =%d", p2p_get_from(res));
		//if( p2p_get_from(res) == P2P_FROM_CDN
		// || p2p_get_from(res) == P2P_FROM_PARTNER_CDN ||
		if (p2p_get_from(res) == P2P_FROM_VIP_HUB)
		{
			data_manager->_cdn_dl_bytes += data_len;
		}
		else if (p2p_get_from(res) == P2P_FROM_LIXIAN)
		{
			data_manager->_lixian_dl_bytes += data_len;
			data_manager->_peer_dl_bytes += data_len;//以后要将离线下载的数据从_peer_dl_bytes中区分开来
			data_manager->_emule_task->_task._main_task_lixian_info._dl_bytes += data_len;
		}
		else if (p2p_get_from(res) == P2P_FROM_NORMAL_CDN)
		{
			data_manager->_normal_cdn_dl_bytes += data_len;
		}
		else
		{
			data_manager->_peer_dl_bytes += data_len;
		}
	}
	else if (res->_resource_type == EMULE_RESOURCE_TYPE)
	{
		data_manager->_emule_dl_bytes += data_len;
		data_manager->_origin_dl_bytes += data_len;
	}
	else
	{
		sd_assert(FALSE);
	}
	LOG_DEBUG("emule_write_data ,_server_dl_bytes = %llu ,_origin_dl_bytes=%llu, _assist_peer_dl_bytes=%llu , _peer_dl_bytes=%llu",
		data_manager->_server_dl_bytes, data_manager->_origin_dl_bytes, data_manager->_assist_peer_dl_bytes, data_manager->_peer_dl_bytes);
	return ret;
}

_u32 emule_get_part_count(EMULE_DATA_MANAGER* data_manager)
{
	return (data_manager->_file_info._file_size + EMULE_PART_SIZE - 1) / EMULE_PART_SIZE;
}

_u32 emule_get_part_size(EMULE_DATA_MANAGER* data_manager, _u32 part_index)
{
	_u32 last_index = (data_manager->_file_info._file_size - 1) / EMULE_PART_SIZE;
	if (part_index < last_index)
		return EMULE_PART_SIZE;
	else if (part_index == last_index)
	{
		sd_assert(data_manager->_file_info._file_size >(_u64)EMULE_PART_SIZE * last_index);
		return (_u32)(data_manager->_file_info._file_size - (_u64)EMULE_PART_SIZE * last_index);
	}
	else
	{
		sd_assert(FALSE);
		return -1;
	}
}

_u64 emule_get_range_size(EMULE_DATA_MANAGER* data_manager, const RANGE* range)
{
	_u32 last_index = (data_manager->_file_info._file_size - 1) / get_data_unit_size();
	if (range->_index + range->_num - 1 < last_index)
		return get_data_unit_size() * (_u64)range->_num;
	else if (range->_index + range->_num - 1 == last_index)
	{	//包含最好一块
		return (data_manager->_file_info._file_size - get_data_unit_size() * (_u64)range->_index);
	}
	else
	{
		sd_assert(FALSE);
	}
	return 0;
}

_int32 emule_notify_check_part_hash_result(EMULE_DATA_MANAGER* data_manager, _u32 index, _int32 result)
{
	_int32 ret = 0;
	_u32 part_index = index;
	_u32 part_num = 1;
	LIST res_list;
	RESOURCE* res = NULL;
	RANGE part_range;
	if (result == SUCCESS)
	{
		bitmap_emule_set(&data_manager->_part_bitmap, index, TRUE);
		if (index > 0 && bitmap_emule_at(&data_manager->_part_bitmap, index - 1))
		{
			--part_index;
			++part_num;
		}
		if (index + 1 < data_manager->_part_bitmap._bit_count && bitmap_emule_at(&data_manager->_part_bitmap, index + 1))
		{
			++part_num;
		}
		emule_part_to_include_range(data_manager, part_index, part_num, &part_range);
		LOG_DEBUG("[emule_task = 0x%x]emule_notify_check_part_hash_result, part_index = %u, part_range[%u, %u], check_result = %d.",
			data_manager->_emule_task, index, part_range._index, part_range._num, result);
		file_info_add_check_done_range(&data_manager->_file_info, &part_range);
		range_manager_erase_range(&data_manager->_range_manager, &part_range, NULL);
		//这里要留意哦，一定要先转part_range,然后再从correct_manager中减掉，要不然会无法减掉
		emule_part_index_to_range(data_manager, index, &part_range);
		correct_manager_erase_error_block(&data_manager->_correct_manager, &part_range);
		file_info_has_block_need_check(&data_manager->_file_info);
		return SUCCESS;
	}
	//校验出错了
	emule_part_index_to_range(data_manager, index, &part_range);
	//把整块数据给抹掉了，必须去掉part_checker里面相邻的两个part
	file_info_delete_range(&data_manager->_file_info, &part_range);
	//	data_receiver_erase_range(&data_manager->_file_info._data_receiver, &part_range);
	//	range_list_delete_range(&data_manager->_file_info._writed_range_list, &part_range, NULL, NULL);
	range_list_add_range(&data_manager->_uncomplete_range_list, &part_range, NULL, NULL);
	LOG_DEBUG("[emule_task = 0x%x]emule check range[%u, %u] failed.", data_manager->_emule_task, part_range._index, part_range._num);
	list_init(&res_list);
	get_res_from_range(&data_manager->_range_manager, &part_range, &res_list);
	/*
		  0   no error
		  -1  peer/server  correct
		  -2  one resource  correct
		  -3  origin correct
		  -4  can not correct
		  */
	if (list_size(&res_list) == 1)
	{
		res = (RESOURCE*)LIST_VALUE(LIST_BEGIN(res_list));
#ifdef EMBED_REPORT
		file_info_handle_err_data_report(&data_manager->_file_info, res, part_range._num * get_data_unit_size());
#endif
		if (range_is_all_from_res(&data_manager->_range_manager, res, &part_range) == FALSE)
			correct_manager_clear_res_list(&res_list);
	}
	ret = correct_manager_add_error_block(&data_manager->_correct_manager, &part_range, &res_list);
	LOG_DEBUG("[emule_task = 0x%x]emule check block failed, correct manager return %d.", data_manager->_emule_task, ret);
	if (ret == -4)
	{
		if (file_info_add_no_need_check_range(&data_manager->_file_info, &part_range) == FALSE)
		{
			//can not correct error 		
			emule_notify_task_failed(data_manager->_emule_task, DATA_CANNOT_CORRECT_ERROR);
		}
		else
		{
			range_manager_erase_range(&data_manager->_range_manager, &part_range, NULL);
		}
	}
	else if (ret == -3)
	{
		sd_assert(FALSE);
	}
	else
	{
		range_manager_erase_range(&data_manager->_range_manager, &part_range, NULL);
	}
	return correct_manager_clear_res_list(&res_list);
}

_int32 emule_set_dispatch_interface(EMULE_DATA_MANAGER* data_manager, struct _tag_ds_data_intereface* dispatch_interface)
{
	dispatch_interface->_get_file_size = emule_get_file_size;
	dispatch_interface->_is_only_using_origin_res = emule_is_only_using_origin_res;
	dispatch_interface->_is_origin_resource = emule_is_origin_resource;
	dispatch_interface->_get_priority_range = emule_get_priority_range;
	dispatch_interface->_get_uncomplete_range = emule_get_uncomplete_range;
	dispatch_interface->_get_error_range_block_list = emule_get_error_range_block_list;
	dispatch_interface->_get_ds_is_vod_mode = emule_is_vod_mod;
	dispatch_interface->_own_ptr = data_manager;
	return SUCCESS;
}

_u64 emule_get_file_size(void* data_manager)
{
	return ((EMULE_DATA_MANAGER*)data_manager)->_file_size;
}

BOOL emule_get_gcid(void* data_manager, _u8 gcid[CID_SIZE])
{
	EMULE_DATA_MANAGER* emule_data_manager = (EMULE_DATA_MANAGER*)data_manager;
	if (emule_data_manager->_is_cid_and_gcid_valid == TRUE)
	{
		sd_memcpy(gcid, emule_data_manager->_gcid, CID_SIZE);
		return TRUE;
	}
	return FALSE;
}

BOOL emule_is_only_using_origin_res(void* data_manager)
{
	return FALSE;
}

BOOL emule_is_origin_resource(void* data_manager, struct tagRESOURCE* res)
{
	return FALSE;
}

_int32 emule_get_priority_range(void* data_manager, struct _tag_range_list* priority_range_list)
{
	EMULE_DATA_MANAGER* emule_data_manager = (EMULE_DATA_MANAGER*)data_manager;
	if (range_list_size(&emule_data_manager->_priority_range_list) == 0)
		return SUCCESS;
	range_list_cp_range_list(&emule_data_manager->_priority_range_list, priority_range_list);
	return range_list_delete_range_list(priority_range_list, &emule_data_manager->_recv_range_list);
}

_int32 emule_get_uncomplete_range(void* data_manager, struct _tag_range_list* uncomplete_range_list)
{
	EMULE_DATA_MANAGER* emule_data_manager = (EMULE_DATA_MANAGER*)data_manager;
	if (range_list_size(&emule_data_manager->_uncomplete_range_list) == 0)
	{
		emule_date_manager_adjust_uncomplete_range(emule_data_manager);
	}
	range_list_cp_range_list(&emule_data_manager->_uncomplete_range_list, uncomplete_range_list);
	return SUCCESS;
}

_int32 emule_get_error_range_block_list(void* data_manager, struct _tag_error_block_list** error_block_list)
{
	EMULE_DATA_MANAGER* emule_data_manager = (EMULE_DATA_MANAGER*)data_manager;
	*error_block_list = NULL;
	if (list_size(&emule_data_manager->_correct_manager._error_ranages._error_block_list) == 0)
		return SUCCESS;
	*error_block_list = &emule_data_manager->_correct_manager._error_ranages;
	return SUCCESS;
}

BOOL emule_is_vod_mod(void* data_manager)
{
	return FALSE;
}

void emule_notify_file_create_result(void* user_data, FILEINFO* file_info, _int32 creat_result)
{
	EMULE_DATA_MANAGER* data_manager = (EMULE_DATA_MANAGER*)user_data;
	EMULE_TASK* emule_task = data_manager->_emule_task;

	LOG_DEBUG("emule_notify_file_create_result, user_data = 0x%x, create_result = %d.", user_data, creat_result);
	if (emule_task == NULL)
	{
		sd_assert(FALSE);
		return;
	}

	if (data_manager->_is_task_deleting) return;
	if (creat_result == SUCCESS)
	{
		emule_task->_task._file_create_status = FILE_CREATED_SUCCESS;
	}
	else
	{
		emule_task->_task._file_create_status = FILE_CREATED_FAILED;
		emule_notify_task_failed(emule_task, creat_result);
	}

}

void emule_notify_file_block_check_result(void* user_data, FILEINFO* file_info, RANGE* range, BOOL result)
{
	LOG_DEBUG("emule_notify_file_block_check_result, user_data = 0x%x, range[%u, %u], block check result is %d.",
		user_data, range->_index, range->_num, result);
}

void emule_notify_file_result(void* user_data, FILEINFO* file_info, _u32 result)
{
	EMULE_DATA_MANAGER* data_manager = (EMULE_DATA_MANAGER*)user_data;
	EMULE_TASK* emule_task = data_manager->_emule_task;
#ifdef EMBED_REPORT
	VOD_REPORT_PARA report_para;
#endif

	LOG_DEBUG("[emule_task = 0x%x]emule_notify_file_result, result = %d.", emule_task, result);
	sd_assert(file_info == &data_manager->_file_info);

	if (data_manager->_is_task_deleting) return;

	if (result != SUCCESS)
	{	//下载文件发生了错误
		emule_notify_task_failed(emule_task, result);
		return;
	}
	//文件下载成功
	/*
	if(file_info_cid_is_valid(&data_manager->_file_info) == TRUE)
	{
	if(file_info_check_cid(&data_manager->_file_info, &errcode, NULL) == FALSE)
	{
	emule_notify_task_failed(emule_task, errcode);
	return;
	}
	}
	if(file_info_gcid_is_valid(&data_manager->_file_info) == TRUE)
	{
	if(file_info_check_gcid(&data_manager->_file_info) == FALSE)
	{
	emule_notify_task_failed(emule_task, -1);
	return;
	}
	}
	*/
#ifdef EMBED_REPORT
	sd_memset(&report_para, 0, sizeof(VOD_REPORT_PARA));
	emb_reporter_emule_stop_task((TASK*)emule_task, &report_para, TASK_S_SUCCESS);
	reporter_emule_download_stat((TASK*)emule_task);
	if (emule_task->_is_need_report)
	{
		reporter_emule_insert_res((TASK*)emule_task);
	}
#endif    


	data_manager->_file_success_flag = TRUE;		//通知关文件成功后改名并删临时文件


	file_info_close_tmp_file(&data_manager->_file_info);
	return;
}

void emule_notify_file_close_result(void* user_data, FILEINFO* file_info, _int32 result)
{
	EMULE_DATA_MANAGER* data_manager = (EMULE_DATA_MANAGER*)user_data;
	EMULE_TASK* emule_task = data_manager->_emule_task;
	LOG_DEBUG("[emule_task = 0x%x]emule_notify_file_close_result, result = %d, file_success_flag = %d.",
		emule_task, result, data_manager->_file_success_flag);
	sd_assert(file_info == &data_manager->_file_info);
	//sd_assert(result == SUCCESS);
	if (data_manager->_file_success_flag == TRUE)
	{
		LOG_DEBUG("[emule_task = 0x%x]emule rename td file and del cfg file.", emule_task);
		file_info_change_td_name(&data_manager->_file_info);
		file_info_delete_cfg_file(&data_manager->_file_info);
		sd_delete_file(data_manager->_cross_file_path);
		file_info_decide_finish_filename(&data_manager->_file_info);
		emule_notify_task_success(emule_task);
	}
	data_manager->_file_info._tmp_file_struct = NULL;
	if (data_manager->_is_task_deleting)
	{
		emule_notify_task_delete(emule_task);
	}
}

void emule_get_dl_bytes(void* data_manager, _u64 *p_server_dl_bytes, _u64 *p_peer_dl_bytes, _u64 *p_emule_dl_bytes, _u64 *p_cdn_dl_bytes, _u64 *p_lixian_dl_bytes)
{
	EMULE_DATA_MANAGER* emule_data_manager = (EMULE_DATA_MANAGER*)data_manager;
	if (p_server_dl_bytes)
		*p_server_dl_bytes = emule_data_manager->_server_dl_bytes;

	if (p_peer_dl_bytes)
		*p_peer_dl_bytes = emule_data_manager->_peer_dl_bytes;

	if (p_emule_dl_bytes)
		*p_emule_dl_bytes = emule_data_manager->_emule_dl_bytes;

	if (p_cdn_dl_bytes)
		*p_cdn_dl_bytes = emule_data_manager->_cdn_dl_bytes;

	if (p_lixian_dl_bytes)
		*p_lixian_dl_bytes = emule_data_manager->_lixian_dl_bytes;
}

void emule_get_normal_cdn_down_bytes(void *data_manager, _u64 * p_normal_cdn_dl_bytes)
{
	EMULE_DATA_MANAGER* emule_data_manager = (EMULE_DATA_MANAGER*)data_manager;
	if (p_normal_cdn_dl_bytes)
		*p_normal_cdn_dl_bytes = emule_data_manager->_normal_cdn_dl_bytes;
}

void emule_get_origin_resource_dl_bytes(void* data_manager, _u64 *p_origin_resource_dl_bytes)
{
	EMULE_DATA_MANAGER* emule_data_manager = (EMULE_DATA_MANAGER*)data_manager;
	if (p_origin_resource_dl_bytes)
		*p_origin_resource_dl_bytes = emule_data_manager->_origin_dl_bytes;
}


void emule_get_assist_peer_dl_bytes(void* data_manager, _u64 *p_assist_peer_dl_bytes)
{
	EMULE_DATA_MANAGER* emule_data_manager = (EMULE_DATA_MANAGER*)data_manager;
	if (p_assist_peer_dl_bytes)
		*p_assist_peer_dl_bytes = emule_data_manager->_assist_peer_dl_bytes;
}

void emule_get_file_id(void* data_manager, _u8 file_id[FILE_ID_SIZE])
{
	EMULE_DATA_MANAGER* emule_data_manager = (EMULE_DATA_MANAGER*)data_manager;
	sd_memcpy(file_id, emule_data_manager->_file_id, FILE_ID_SIZE);
}

void emule_get_part_hash(void* data_manager, _u8** pp_part_hash, _u32 *p_hash_len)
{
	EMULE_DATA_MANAGER* emule_data_manager = (EMULE_DATA_MANAGER*)data_manager;

	*pp_part_hash = emule_data_manager->_part_checker._part_hash;
	*p_hash_len = emule_data_manager->_part_checker._part_hash_len;

#ifdef _DEBUG
	{
		char test_buffer[1024];
		sd_memset(test_buffer, 0, 1024);
		str2hex((char*)(*pp_part_hash), *p_hash_len, test_buffer, 1024);
		LOG_DEBUG("emule_get_part_hash:*buffer[%u]=%s", *p_hash_len, test_buffer);
	}
#endif /* _DEBUG */

}

void emule_get_aich_hash(void* data_manager, _u8** pp_aich_hash, _u32 *p_aich_hash_len)
{
	EMULE_DATA_MANAGER* emule_data_manager = (EMULE_DATA_MANAGER*)data_manager;
	*pp_aich_hash = emule_data_manager->_part_checker._aich_hash;
	*p_aich_hash_len = emule_data_manager->_part_checker._aich_hash_len;
#ifdef _DEBUG
	{
		char test_buffer[1024];
		sd_memset(test_buffer, 0, 1024);
		str2hex((char*)(*pp_aich_hash), *p_aich_hash_len, test_buffer, 1024);
		LOG_DEBUG("emule_get_aich_hash:*buffer[%u]=%s", *p_aich_hash_len, test_buffer);
	}
#endif /* _DEBUG */

}

BOOL emule_get_shub_cid(void* data_manager, _u8 cid[CID_SIZE])
{
	EMULE_DATA_MANAGER *p_emule_dm = (EMULE_DATA_MANAGER*)data_manager;
	if (NULL == data_manager || cid == NULL)
		return INVALID_MEMORY;
	if (p_emule_dm->_is_cid_and_gcid_valid)
	{
		sd_memcpy(cid, p_emule_dm->_cid, CID_SIZE);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL emule_get_cid(void* data_manager, _u8 cid[CID_SIZE])
{
	BOOL ret_val = TRUE;
	EMULE_DATA_MANAGER* emule_data_manager = (EMULE_DATA_MANAGER*)data_manager;
#ifdef _LOGGER  
	char  cid_str[CID_SIZE * 2 + 1];
#endif 	  

	LOG_DEBUG("calling emule_get_cid.");

	if (file_info_filesize_is_valid(&emule_data_manager->_file_info) == FALSE)
	{
		LOG_ERROR("emule_get_cid, no filesize  so cannot get cid.");
		return FALSE;

	}

	if ((FALSE == range_list_is_contained(
		&emule_data_manager->_file_info._writed_range_list,
		&emule_data_manager->_3part_cid_list))
		|| (FALSE == range_list_is_contained(
		&emule_data_manager->_file_info._done_ranges,
		&emule_data_manager->_3part_cid_list)))
	{
		return FALSE;
	}

	// 先判断以前是否计算过cid，如果没有计算则重新计算，否则返回之前计算的结果
	// _cid_is_valid有两个地方可能设置为真值
	// 1， 在计算完成后，file_info_set_cid
	// 2， 在加载临时文件后，file_info_load_from_cfg_file
	FILEINFO * p_file_info = &emule_data_manager->_file_info;
	if (p_file_info != NULL) {
		if (p_file_info->_cid_is_valid == TRUE) {
			LOG_DEBUG("[emule_get_cid] _cid_is_valid is TRUE.");
			sd_memcpy(cid, p_file_info->_cid, CID_SIZE);
			return TRUE;
		}
	}
	else 
	{
		LOG_ERROR("[emule_get_cid] the p_file_info is NULL.");
		return FALSE;
	}

	// 真正计算之
	ret_val = get_file_3_part_cid(&emule_data_manager->_file_info, cid, NULL);
	if (ret_val == TRUE)
	{
#ifdef _LOGGER  
		str2hex((char*)cid, CID_SIZE, cid_str, CID_SIZE * 2);
		cid_str[CID_SIZE * 2] = '\0';

		LOG_DEBUG("emule_get_cid, calc cid success, cid:%s.", cid_str);
#endif 			   
		file_info_set_cid(&emule_data_manager->_file_info, cid);

	}
	else
	{
		LOG_ERROR("emule_get_cid, calc cid failure.");
	}

	return ret_val;
}

BOOL emule_get_shub_gcid(void* data_manager, _u8 gcid[CID_SIZE])
{
	EMULE_DATA_MANAGER *p_emule_dm = (EMULE_DATA_MANAGER*)data_manager;
	if (NULL == data_manager || gcid == NULL)
		return INVALID_MEMORY;
	if (p_emule_dm->_is_cid_and_gcid_valid)
	{
		sd_memcpy(gcid, p_emule_dm->_gcid, CID_SIZE);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL emule_get_calc_gcid(void* data_manager, _u8 gcid[CID_SIZE])
{
	BOOL ret_bool = TRUE;
	EMULE_DATA_MANAGER* emule_data_manager = (EMULE_DATA_MANAGER*)data_manager;
#ifdef _LOGGER  
	char  cid_str[CID_SIZE * 2 + 1];
#endif 	

	ret_bool = get_file_gcid(&emule_data_manager->_file_info, gcid);
#ifdef _LOGGER  
	if (ret_bool)
	{
		str2hex((char*)gcid, CID_SIZE, cid_str, CID_SIZE * 2);
		cid_str[CID_SIZE * 2] = '\0';

		LOG_DEBUG("emule_get_calc_gcid:%s.", cid_str);
	}

#endif 	
	return ret_bool;
}

_int32 emule_get_block_size(void* data_manager)
{
	EMULE_DATA_MANAGER* emule_data_manager = (EMULE_DATA_MANAGER*)data_manager;

	LOG_DEBUG("emule_get_block_size. get block size: %u.", emule_data_manager->_file_info._block_size);

	return  file_info_get_blocksize(&emule_data_manager->_file_info);
}

BOOL emule_get_bcids(void* data_manager, _u32* bcid_num, _u8** bcid_buffer)
{
	BOOL   ret_val = FALSE;
	_u32  bcid_block_num = 0;
	_u8*  p_bcid_buffer = NULL;
	EMULE_DATA_MANAGER* emule_data_manager = (EMULE_DATA_MANAGER*)data_manager;

	if (file_info_is_all_checked(&emule_data_manager->_file_info) == FALSE)
	{
		LOG_DEBUG("emule_get_bcids, because can not finish download, so no bcid .");
		ret_val = FALSE;

	}
	else
	{
		bcid_block_num = file_info_get_bcid_num(&emule_data_manager->_file_info);
		p_bcid_buffer = file_info_get_bcid_buffer(&emule_data_manager->_file_info);
		ret_val = TRUE;
		LOG_DEBUG("emule_get_bcids, because bcid is invalid and has finished download, so  blocksize : %u , bcid buffer:0x%x .",
			bcid_block_num, p_bcid_buffer);
	}

	*bcid_num = bcid_block_num;
	*bcid_buffer = p_bcid_buffer;

	return ret_val;
}


#ifdef EMBED_REPORT
struct tagFILE_INFO_REPORT_PARA *emule_get_report_para(void* data_manager)
{
	EMULE_DATA_MANAGER* emule_data_manager = (EMULE_DATA_MANAGER*)data_manager;
	return file_info_get_report_para(&emule_data_manager->_file_info);
}
#endif

#endif /* ENABLE_EMULE */

