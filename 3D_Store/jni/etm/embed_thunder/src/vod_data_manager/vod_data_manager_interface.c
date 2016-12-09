#include "utility/string.h"
#include "utility/utility.h"
#include "utility/url.h"
#include "utility/map.h"
#include "utility/errcode.h"
#include "platform/sd_socket.h"
#include "asyn_frame/msg.h"
#include "data_manager/data_manager_interface.h"
#include "data_manager/data_receive.h"
#include "utility/range.h"
#include "task_manager/task_manager.h"
#include "download_task/download_task.h"
#include "index_parser/index_parser_interface.h"
#include "utility/time.h"
#include "utility/md5.h"
#include "platform/sd_fs.h"
#include "p2sp_download/p2sp_task.h"
#include "bt_download/bt_task/bt_task.h"
#include "data_manager/file_manager_imp.h"

#ifdef EMBED_REPORT
#include "reporter/reporter_interface.h"
#endif

#include "utility/logid.h"
#define	 LOGID	LOGID_VOD_DATA_MANAGER
#include "utility/logger.h"

#include "vod_data_manager_interface.h"
#include "vdm_data_buffer.h"

static SLAB* g_vdm_info_slab = NULL;
static VOD_DATA_MANAGER_LIST g_vdm_list;
static _int32 g_vdm_buffer_time_len = VDM_DRAG_WINDOW_TIME_LEN;

static _u32 MIN_VOD_RANGE_DATA_BUFFER_NODE = DATA_RECEIVE_MIN_RANGE_DATA_BUFFER_NODE;

static SLAB* g_vod_range_data_buffer_node_slab = NULL;

static _u32  VOD_DATA_MANAGER_DOWN_WINDOW_TO_MOVE_NUM = 5;
static _u32  MIN_VOD_DATA_MANAGER_DOWN_WINDOW_NUM = 2;

static VOD_DL_POS g_vod_dl_pos[VDM_MAX_DL_POS_NUM];

_int32 vdm_write_data_buffer(VOD_DATA_MANAGER* ptr_vod_data_manager, _u64 start_pos, char *buffer, _u64 length, RANGE* range_to_write, VOD_RANGE_DATA_BUFFER_LIST* buffer_list);
_int32 vdm_buffer_list_find(VOD_RANGE_DATA_BUFFER_LIST *buffer_list, RANGE*  range, VOD_RANGE_DATA_BUFFER** ret_data_buffer);
_int32 vdm_uninit_vod_data_manager_list(void);
_int32 vdm_get_vod_data_manager_by_task_id(VOD_DATA_MANAGER_LIST* vod_data_manager_list, _u32 task_id, _u32 file_index, VOD_DATA_MANAGER** ret_ptr, BOOL only_run_vdm, BOOL force_start);
BOOL vdm_is_range_checked(VOD_DATA_MANAGER* ptr_vod_data_manager, RANGE* r);


BOOL g_enable_vod = TRUE;
_int32 vdm_enable_vod(BOOL b)
{
	LOG_DEBUG("vdm_enable_vod = %d", b);
	g_enable_vod = b;
	return SUCCESS;
}

BOOL vdm_is_vod_enabled(void)
{
	return g_enable_vod;
}

_int32 init_vod_range_data_buffer_slab(void)
{
	_int32 ret_val = SUCCESS;
	if (NULL == g_vod_range_data_buffer_node_slab)
	{
		ret_val = mpool_create_slab(sizeof(VOD_RANGE_DATA_BUFFER), MIN_VOD_RANGE_DATA_BUFFER_NODE, 0, &g_vod_range_data_buffer_node_slab);
	}
	return (ret_val);
}

_int32 destroy_vod_range_data_buffer_slab(void)
{
	_int32 ret_val = SUCCESS;
	if (NULL != g_vod_range_data_buffer_node_slab)
	{
		mpool_destory_slab(g_vod_range_data_buffer_node_slab);
		g_vod_range_data_buffer_node_slab = NULL;
	}
	return (ret_val);
}


/* platform of vod data manager module, must  be invoke in the initial state of the download program*/
_int32  init_vod_data_manager_module(void)
{
	_int32 ret_val = get_vod_data_manager_cfg_parameter();
	CHECK_VALUE(ret_val);

	ret_val = vdm_create_slabs_and_init_data_buffer();
	CHECK_VALUE(ret_val);

	ret_val = init_vod_range_data_buffer_slab();
	CHECK_VALUE(ret_val);

	ret_val = vdm_data_buffer_init();
	CHECK_VALUE(ret_val);

	list_init(&g_vdm_list._data_list);

	sd_memset(g_vod_dl_pos, 0x00, sizeof(VOD_DL_POS)*VDM_MAX_DL_POS_NUM);

	return ret_val;
}

_int32  uninit_vod_data_manager_module(void)
{
	_int32 ret_val = SUCCESS;

	ret_val = vdm_uninit_vod_data_manager_list();
	CHECK_VALUE(ret_val);

	ret_val = vdm_destroy_slabs_and_unit_data_buffer();
	CHECK_VALUE(ret_val);

	ret_val = destroy_vod_range_data_buffer_slab();
	CHECK_VALUE(ret_val);

	ret_val = vdm_data_buffer_uninit();
	CHECK_VALUE(ret_val);

	return ret_val;

}

/* platform of vod data manager module*/
_int32 get_vod_data_manager_cfg_parameter(void)
{
	_int32 ret_val = SUCCESS;

	ret_val = settings_get_int_item("vod.window_num_to_move", (_int32*)&VOD_DATA_MANAGER_DOWN_WINDOW_TO_MOVE_NUM);

	LOG_DEBUG("get_vod_data_manager_cfg_parameter,  vod.window_num_to_move: %u .", VOD_DATA_MANAGER_DOWN_WINDOW_TO_MOVE_NUM);

	ret_val = settings_get_int_item("vod.min_down_window", (_int32*)&MIN_VOD_DATA_MANAGER_DOWN_WINDOW_NUM);

	LOG_DEBUG("get_vod_data_manager_cfg_parameter,  vod.min_down_window: %u .", MIN_VOD_DATA_MANAGER_DOWN_WINDOW_NUM);

	return SUCCESS;
}

_int32 alloc_vod_range_data_buffer_node(VOD_RANGE_DATA_BUFFER** pp_node)
{
	return  mpool_get_slip(g_vod_range_data_buffer_node_slab, (void**)pp_node);
}

_int32 free_vod_range_data_buffer_node(VOD_RANGE_DATA_BUFFER* p_node)
{
	return  mpool_free_slip(g_vod_range_data_buffer_node_slab, (void*)p_node);
}




void vdm_buffer_list_init(VOD_RANGE_DATA_BUFFER_LIST *  buffer_list)
{
	list_init(&buffer_list->_data_list);
}

_u32 vdm_buffer_list_size(VOD_RANGE_DATA_BUFFER_LIST *buffer_list)
{
	return list_size(&buffer_list->_data_list);
}

_int32 vdm_buffer_list_add(VOD_RANGE_DATA_BUFFER_LIST *buffer_list, RANGE*  range, char *data, _u32 length, _u32 buffer_len, _int32 is_recved)
{
	VOD_RANGE_DATA_BUFFER* new_node = NULL;
	VOD_RANGE_DATA_BUFFER* cur_buffer = NULL;
	LIST_ITERATOR  cur_node = NULL;

	LOG_DEBUG("vdm_buffer_list_add, buffer_list:0x%x , range(%u,%u), buffer:0x%x, length:%u, buffer_len:%u .", buffer_list, range->_index, range->_num, data, length, buffer_len);

	//  sd_malloc(sizeof(RANGE_DATA_BUFFER) ,(void**)&new_node);

	alloc_vod_range_data_buffer_node(&new_node);
	if (new_node == NULL)
	{
		LOG_ERROR("vdm_buffer_list_add, buffer_list:0x%x , malloc RANGE_DATA_BUFFER failure .", buffer_list);

		return OUT_OF_MEMORY;
	}

	new_node->_range_data_buffer._range = *range;
	new_node->_range_data_buffer._data_length = length;
	new_node->_range_data_buffer._data_ptr = data;
	new_node->_range_data_buffer._buffer_length = buffer_len;
	new_node->_dead_time = 0;
	new_node->_is_recved = is_recved;

	cur_node = LIST_BEGIN(buffer_list->_data_list);

	while (cur_node != LIST_END(buffer_list->_data_list))
	{
		cur_buffer = (VOD_RANGE_DATA_BUFFER*)cur_node->_data;

		if (cur_buffer->_range_data_buffer._range._index <= range->_index)
		{
			cur_node = LIST_NEXT(cur_node);
		}
		else
		{
			break;
		}
	}

	if (cur_node != LIST_END(buffer_list->_data_list))
	{
		list_insert(&buffer_list->_data_list, new_node, cur_node);
	}
	else
	{
		list_push(&buffer_list->_data_list, new_node);
	}
	return SUCCESS;
}

/*
_int32 vdm_buffer_list_delete(VOD_RANGE_DATA_BUFFER_LIST *buffer_list, RANGE*  range, RANGE_LIST* ret_range_list)
{
VOD_RANGE_DATA_BUFFER* cur_buffer =  NULL;
LIST_ITERATOR  erase_node = NULL;

LIST_ITERATOR  cur_node = LIST_BEGIN(buffer_list->_data_list);

LOG_DEBUG("vdm_buffer_list_delete, buffer_list:0x%x , delete range(%u,%u) .", buffer_list, range->_index, range->_num);

while(cur_node != LIST_END(buffer_list->_data_list))
{
cur_buffer = (VOD_RANGE_DATA_BUFFER*)cur_node->_data;

if(cur_buffer->_range_data_buffer._range._index + cur_buffer->_range_data_buffer._range._num <= range->_index)
{
cur_node =  LIST_NEXT(cur_node);
}
else  if(cur_buffer->_range_data_buffer._range._index >= range->_index + range->_num)
{
break;
}
else
{
erase_node = cur_node;
cur_node =  LIST_NEXT(cur_node);
list_erase(&buffer_list->_data_list,erase_node);

range_list_add_range(ret_range_list, &cur_buffer->_range_data_buffer._range, NULL, NULL);

vdm_free_buffer_to_data_buffer(cur_buffer->_range_data_buffer._buffer_length, &(cur_buffer->_range_data_buffer._data_ptr));

//sd_free(cur_buffer);
free_vod_range_data_buffer_node(cur_buffer);
}
}

return SUCCESS;
}
*/

_int32 vdm_buffer_list_find(VOD_RANGE_DATA_BUFFER_LIST *buffer_list, RANGE*  range, VOD_RANGE_DATA_BUFFER** ret_data_buffer)
{
	VOD_RANGE_DATA_BUFFER* cur_buffer = NULL;

	LIST_ITERATOR  cur_node = LIST_BEGIN(buffer_list->_data_list);

	*ret_data_buffer = NULL;

	LOG_DEBUG("buffer_list_find, buffer_list:0x%x , to find range(%u,%u) .", buffer_list, range->_index, range->_num);

	while (cur_node != LIST_END(buffer_list->_data_list))
	{
		cur_buffer = (VOD_RANGE_DATA_BUFFER*)cur_node->_data;

		if (cur_buffer->_range_data_buffer._range._index + cur_buffer->_range_data_buffer._range._num <= range->_index)
		{
			cur_node = LIST_NEXT(cur_node);
		}
		else  if (cur_buffer->_range_data_buffer._range._index >= range->_index + range->_num)
		{
			break;
		}
		else
		{
			*ret_data_buffer = cur_buffer;
			return SUCCESS;
		}
	}

	return ERR_VOD_DATA_BUFFER_NOT_FOUND;
}

_int32  vdm_create_slabs_and_init_data_buffer()
{
	_int32 ret_val = SUCCESS;

	if (g_vdm_info_slab == NULL)
	{
		ret_val = mpool_create_slab(sizeof(VOD_DATA_MANAGER), MIN_VDM_INFO, 0, &g_vdm_info_slab);
		CHECK_VALUE(ret_val);
	}
	return ret_val;

}
_int32  vdm_destroy_slabs_and_unit_data_buffer()
{
	_int32 ret_val = SUCCESS;

	if (g_vdm_info_slab != NULL)
	{
		ret_val = mpool_destory_slab(g_vdm_info_slab);
		CHECK_VALUE(ret_val);
		g_vdm_info_slab = NULL;
	}
	return ret_val;

}

_int32 vdm_info_malloc_wrap(struct _tag_vod_data_manager **pp_node)
{
	_int32 ret_val = SUCCESS;
	ret_val = mpool_get_slip(g_vdm_info_slab, (void**)pp_node);
	CHECK_VALUE(ret_val);

	ret_val = sd_memset(*pp_node, 0, sizeof(VOD_DATA_MANAGER));
	return SUCCESS;
}

_int32 vdm_info_free_wrap(struct _tag_vod_data_manager *p_node)
{
	sd_assert(p_node != NULL);
	if (p_node == NULL) return SUCCESS;
	return mpool_free_slip(g_vdm_info_slab, (void*)p_node);
}



_int32 vdm_drop_buffer_list(VOD_DATA_MANAGER* p_vod_data_manager_interface, VOD_RANGE_DATA_BUFFER_LIST *buffer_list)
{
	VOD_RANGE_DATA_BUFFER* cur_buffer = NULL;
	LOG_DEBUG("vdm_drop_buffer_list, drop buffer_list:0x%x .", buffer_list);


	do
	{
		list_pop(&buffer_list->_data_list, (void**)&cur_buffer);
		if (cur_buffer == NULL)
		{
			break;
		}
		else
		{
			if (cur_buffer->_is_recved == 1)
			{
				vdm_free_data_buffer(p_vod_data_manager_interface, &(cur_buffer->_range_data_buffer._data_ptr), cur_buffer->_range_data_buffer._buffer_length);
			}
			free_vod_range_data_buffer_node(cur_buffer);
			cur_buffer = NULL;
		}


	} while (1);

	return SUCCESS;

}


_int32  vdm_uninit_data_manager(VOD_DATA_MANAGER* p_vod_data_manager_interface, BOOL keep_data)
{
	_int32 ret_val;
	ret_val = SUCCESS;
	if (p_vod_data_manager_interface->_vdm_timer)
	{
		ret_val = cancel_timer(p_vod_data_manager_interface->_vdm_timer);
		p_vod_data_manager_interface->_vdm_timer = 0;
	}


	LOG_DEBUG("vdm_uninit_data_manager .");

	if (TRUE == p_vod_data_manager_interface->_is_fetching)
	{
		if (p_vod_data_manager_interface->_user_data != NULL)
		{
			ret_val = (*p_vod_data_manager_interface->_callback_recv_handler)(ERR_VOD_DATA_OP_INTERUPT, 0, p_vod_data_manager_interface->_user_buffer, 0, p_vod_data_manager_interface->_user_data);
			LOG_DEBUG("vdm_vod_async_read_file .callback return = %d", ret_val);
			p_vod_data_manager_interface->_user_data = NULL;
			p_vod_data_manager_interface->_callback_recv_handler = NULL;
		}
	}
	p_vod_data_manager_interface->_is_fetching = FALSE;

	if (!keep_data)
	{
		p_vod_data_manager_interface->_current_download_window._index = 0;
		p_vod_data_manager_interface->_current_download_window._num = 0;

		vdm_drop_buffer_list(p_vod_data_manager_interface, &p_vod_data_manager_interface->_buffer_list);

		range_list_clear(&p_vod_data_manager_interface->_range_buffer_list);
		range_list_clear(&p_vod_data_manager_interface->_range_index_list);

	}
	else
	{
		RANGE_LIST_ITEROATOR cur;
		range_list_get_head_node(&p_vod_data_manager_interface->_range_waiting_list, &cur);
		while (cur != NULL)
		{
			vdm_drop_buffer_by_range(p_vod_data_manager_interface, &cur->_range);
			range_list_get_next_node(&p_vod_data_manager_interface->_range_waiting_list, cur, &cur);
		}
	}
	range_list_clear(&p_vod_data_manager_interface->_range_waiting_list);
	range_list_clear(&p_vod_data_manager_interface->_range_last_em_list);

	if (p_vod_data_manager_interface->_task_ptr != NULL)
	{
		vdm_uninit_dlpos(p_vod_data_manager_interface->_task_ptr->_task_id);
		p_vod_data_manager_interface->_task_ptr = NULL;
	}
	if (!keep_data) p_vod_data_manager_interface->_task_id = 0;
	p_vod_data_manager_interface->_file_index = 0;
#ifdef EMBED_REPORT
	p_vod_data_manager_interface->_vod_last_time_type = VOD_LAST_NONE;
	sd_time(&p_vod_data_manager_interface->_vod_last_time);
	p_vod_data_manager_interface->_vod_play_times = 0;
	p_vod_data_manager_interface->_vod_play_begin_time = 0;
	p_vod_data_manager_interface->_vod_first_buffer_time_len = 0;
	p_vod_data_manager_interface->_vod_failed_times = 0;
	p_vod_data_manager_interface->_vod_play_drag_num = 0;
	p_vod_data_manager_interface->_vod_play_total_drag_wait_time = 0;
	p_vod_data_manager_interface->_vod_play_max_drag_wait_time = 0;
	p_vod_data_manager_interface->_vod_play_min_drag_wait_time = 0;
	p_vod_data_manager_interface->_vod_play_interrupt_times = 0;
	p_vod_data_manager_interface->_vod_play_total_buffer_time_len = 0;
	p_vod_data_manager_interface->_vod_play_max_buffer_time_len = 0;
	p_vod_data_manager_interface->_vod_play_min_buffer_time_len = 0;
	p_vod_data_manager_interface->_cdn_stop_times = 0;
	p_vod_data_manager_interface->_is_ad_type = FALSE;
#endif

	CHECK_VALUE(ret_val);

	return ret_val;
}

_int32 vdm_get_free_vdm(VOD_DATA_MANAGER_LIST* p_vdm_list, VOD_DATA_MANAGER **pp_vdm)
{
	_int32 ret = SUCCESS;
	VOD_DATA_MANAGER* cur_manager = NULL;

	LIST_ITERATOR  cur_node = LIST_BEGIN(p_vdm_list->_data_list);

	LOG_DEBUG("vdm_get_free_vdm .");

	while (cur_node != LIST_END(p_vdm_list->_data_list))
	{
		if (NULL == cur_node)
			break;
		cur_manager = (VOD_DATA_MANAGER*)LIST_VALUE(cur_node);
		if (NULL == cur_manager->_task_ptr && cur_manager->_task_id == 0) //BT任务也只有一个播放session
		{
			*pp_vdm = cur_manager;
			ret = SUCCESS;
			return ret;
		}
		cur_node = LIST_NEXT(cur_node);
	}


	LOG_DEBUG("vdm_get_free_vdm malloc a vdm.");
	ret = vdm_info_malloc_wrap(&cur_manager);
	CHECK_VALUE(ret);

	list_push(&p_vdm_list->_data_list, cur_manager);

	*pp_vdm = cur_manager;
	return ret;

}


_int32 vdm_get_free_vod_data_manager(struct _tag_task* ptr_task, _u32 file_index, VOD_DATA_MANAGER **pp_vdm)
{
	VOD_DATA_MANAGER* ptr_vdm = NULL;
	_int32 ret;


	ret = vdm_get_free_vdm(&g_vdm_list, &ptr_vdm);
	CHECK_VALUE(ret);

	ret = vdm_init_data_manager(ptr_vdm, ptr_task, file_index);
	CHECK_VALUE(ret);

	*pp_vdm = ptr_vdm;

	return ret;
}

_int32 vdm_uninit_vod_data_manager_list()
{
	VOD_DATA_MANAGER_LIST* vod_data_manager_list = &g_vdm_list;
	VOD_DATA_MANAGER* cur_manager = NULL;

	LIST_ITERATOR  cur_node = LIST_BEGIN(vod_data_manager_list->_data_list);

	LOG_DEBUG("vdm_uninit_vod_data_manager_list .");

	while (cur_node != LIST_END(vod_data_manager_list->_data_list))
	{
		if (NULL == cur_node)
			break;
		cur_manager = (VOD_DATA_MANAGER*)LIST_VALUE(cur_node);
		LOG_DEBUG("vdm_uninit_vod_data_manager_list .free %08X", cur_manager);
		vdm_uninit_data_manager(cur_manager, FALSE);
		vdm_info_free_wrap(cur_manager);
		cur_node = LIST_NEXT(cur_node);
	}
	list_clear(&vod_data_manager_list->_data_list);

	LOG_DEBUG("vdm_uninit_vod_data_manager_list success.");
	return SUCCESS;
}


_int32 vdm_get_buffer_time_len(void)
{
	return g_vdm_buffer_time_len;
}

_int32 vdm_set_buffer_time_len(_int32 time_len)
{
	if (time_len <= VDM_BUFFERING_WINDOW_TIME_LEN && time_len >= VDM_MIN_WINDOW_TIME_LEN)
	{
		g_vdm_buffer_time_len = time_len;
	}
	return  SUCCESS;
}


//從DM傳過來的
_int32 vdm_sync_data_buffer_to_range_list(VOD_DATA_MANAGER* ptr_vod_data_manager, RANGE_DATA_BUFFER_LIST* buffer_list)
{
	RANGE_LIST* range_list;
	RANGE_DATA_BUFFER* cur_buffer = NULL;
	_int32 ret;
	char* data_ptr;
	_u32 data_len;

	LIST_ITERATOR  cur_node = LIST_BEGIN(buffer_list->_data_list);

	range_list = &ptr_vod_data_manager->_range_buffer_list;

	LOG_DEBUG("vdm_sync_data_buffer_to_range_list .");

	while (cur_node != LIST_END(buffer_list->_data_list))
	{
		cur_buffer = (RANGE_DATA_BUFFER*)LIST_VALUE(cur_node);

		LOG_DEBUG("vdm_sync_data_buffer_to_range_list sd_malloc %d.", cur_buffer->_buffer_length);
		data_len = cur_buffer->_buffer_length;
		ret = vdm_get_data_buffer(ptr_vod_data_manager, &data_ptr, &data_len);
		if (SUCCESS != ret)
		{
			LOG_DEBUG("vdm_sync_data_buffer_to_range_list vdm_get_data_buffer size=%d ret =%d", data_len, ret);
			return ret;
		}
		cur_buffer->_buffer_length = data_len;
		sd_memcpy(data_ptr, cur_buffer->_data_ptr, cur_buffer->_buffer_length);

		//這里要修改是設置緩沖區進去
		ret = vdm_buffer_list_add(&ptr_vod_data_manager->_buffer_list, &cur_buffer->_range, data_ptr, cur_buffer->_data_length, cur_buffer->_buffer_length, 1);

		ret = range_list_add_range(range_list, &cur_buffer->_range, NULL, NULL);

		cur_node = LIST_NEXT(cur_node);
	}

	LOG_DEBUG("vdm_sync_data_buffer_to_range_list success.");
	return SUCCESS;

}


_u32 vdm_notify_read_data_result(struct tagTMP_FILE*  file_struct, void* user_ptr, RANGE_DATA_BUFFER* data_buffer, _int32 read_result, _int32 read_len)
{
	RANGE_LIST range_list;
	RANGE      range_to_get;
	_int32     ret;
	_u64 filesize;
	_int32 file_index;
	//_u32 time;
	//_u32 time_ms;
	VOD_RANGE_DATA_BUFFER* cur_read_buffer = NULL;

	VOD_DATA_MANAGER*  ptr_vod_data_manager = (VOD_DATA_MANAGER*)user_ptr;
	VOD_DATA_MANAGER*  ptr_vod_data_manager_ret = NULL;
	/*get DOWNLOAD_TASK ptr here*/
	TASK* ptr_task = (TASK*)ptr_vod_data_manager->_task_ptr;
	ret = SUCCESS;

	LOG_DEBUG("vdm_notify_read_data_result .read_result=%d, user_ptr: 0x%x, task_ptr: 0x%x", read_result, user_ptr, ptr_task);

	if (read_result != SUCCESS)
	{
		ret = read_result;
		vdm_free_buffer_to_data_buffer(data_buffer->_buffer_length, &(data_buffer->_data_ptr));

		free_range_data_buffer_node(data_buffer);

		return ret;

	}

	if (NULL == user_ptr || ptr_task == NULL)
	{
		vdm_free_buffer_to_data_buffer(data_buffer->_buffer_length, &(data_buffer->_data_ptr));

		free_range_data_buffer_node(data_buffer);
		return SUCCESS;
	}

	file_index = ptr_vod_data_manager->_file_index;

	ret = vdm_get_vod_data_manager_by_task_id(&g_vdm_list, ptr_task->_task_id, file_index, &ptr_vod_data_manager_ret, TRUE, FALSE);
	if (SUCCESS != ret || NULL == ptr_vod_data_manager_ret)
	{
		vdm_free_buffer_to_data_buffer(data_buffer->_buffer_length, &(data_buffer->_data_ptr));

		free_range_data_buffer_node(data_buffer);
		return ret;
	}

	LOG_DEBUG("vdm_notify_read_data_result .read_result=%d, read_len=%d, range(%d,%d) ", read_result, read_len, data_buffer->_range._index, data_buffer->_range._num);


	if (P2SP_TASK_TYPE == ptr_task->_task_type)
	{
		filesize = dm_get_file_size(&((P2SP_TASK*)ptr_task)->_data_manager);
	}
#ifdef ENABLE_BT
	else if (BT_TASK_TYPE == ptr_task->_task_type)
	{
		filesize = bdm_get_sub_file_size(&((BT_TASK*)ptr_task)->_data_manager, ptr_vod_data_manager->_file_index);
	}
#endif
	else
	{
		LOG_DEBUG("vdm_notify_read_data_result .unknown task type ");
		vdm_free_buffer_to_data_buffer(data_buffer->_buffer_length, &(data_buffer->_data_ptr));

		free_range_data_buffer_node(data_buffer);
		return ret;
	}



	range_to_get = pos_length_to_range(ptr_vod_data_manager->_start_pos, ptr_vod_data_manager->_fetch_length, filesize);


	range_list_init(&range_list);

	if (read_result != SUCCESS)
	{
		ret = read_result;
		vdm_free_buffer_to_data_buffer(data_buffer->_buffer_length, &(data_buffer->_data_ptr));
		free_range_data_buffer_node(data_buffer);

	}
	else
	{
		/************************************************************************/
		/* SUCCESS                                                              */
		/************************************************************************/

		ret = vdm_buffer_list_find(&ptr_vod_data_manager->_buffer_list, &data_buffer->_range, &cur_read_buffer);

		if (ret == SUCCESS && cur_read_buffer->_range_data_buffer._data_ptr == data_buffer->_data_ptr)
		{
			cur_read_buffer->_is_recved = 1;
		}
		else
		{
			ret = vdm_free_buffer_to_data_buffer(data_buffer->_buffer_length, &(data_buffer->_data_ptr));
			free_range_data_buffer_node(data_buffer);

			return (ret);
		}


		range_list_add_range(&range_list, &data_buffer->_range, NULL, NULL);
		LOG_DEBUG("index = %u, num = %u ", data_buffer->_range._index, data_buffer->_range._num);
		range_list_delete_range_list(&ptr_vod_data_manager->_range_waiting_list, &range_list);

		range_list_add_range(&ptr_vod_data_manager->_range_buffer_list, &data_buffer->_range, NULL, NULL);

		free_range_data_buffer_node(data_buffer);
		data_buffer = NULL;


		if (TRUE != ptr_vod_data_manager->_is_fetching)
		{
			LOG_DEBUG("vdm_notify_read_data_result is not in fetching state...");
			range_list_clear(&range_list);
			return SUCCESS;
		}

		if (NULL == ptr_vod_data_manager->_user_buffer)
		{
			LOG_DEBUG("vdm_notify_read_data_result _user_buffer is NULL ...");
			range_list_clear(&range_list);
			return SUCCESS;
		}

#ifdef ENABLE_FLASH_PLAYER
		/* 只能在android 平台中用于flash 播放器点播 */
		vdm_process_index_parser(ptr_vod_data_manager, filesize, "FLV");
#endif /* ENABLE_FLASH_PLAYER */

		if (TRUE != ptr_vod_data_manager->_send_pause
			&& TRUE == range_list_is_include(&ptr_vod_data_manager->_range_buffer_list, &range_to_get)
			&& TRUE == vdm_is_range_checked(ptr_vod_data_manager, &range_to_get))
		{

			LOG_DEBUG("vdm_notify_read_data_result .range_list_is_include return TRUE ");
			//Buffer is in the bufferlist already, just return them
			LOG_DEBUG("vdm_period_dispatch.debug vdm_notify_read_data_result _callback_recv_handler %llu, %llu, range_to_get(%d,%d)", ptr_vod_data_manager->_start_pos, ptr_vod_data_manager->_fetch_length, range_to_get._index, range_to_get._num);
			ret = vdm_write_data_buffer(ptr_vod_data_manager, ptr_vod_data_manager->_start_pos, ptr_vod_data_manager->_user_buffer, ptr_vod_data_manager->_fetch_length, &range_to_get, &ptr_vod_data_manager->_buffer_list);
			LOG_DEBUG("vdm_notify_read_data_result .vdm_write_data_buffer return %d ", ret);
			CHECK_VALUE(ret);
			ret = (*(ptr_vod_data_manager->_callback_recv_handler))(SUCCESS, 0, ptr_vod_data_manager->_user_buffer, ptr_vod_data_manager->_fetch_length, ptr_vod_data_manager->_user_data);
			LOG_DEBUG("vdm_notify_read_data_result ._callback_recv_handler return %d ", ret);
			ptr_vod_data_manager->_is_fetching = FALSE;
			ptr_vod_data_manager->_send_pause = FALSE;
			ptr_vod_data_manager->_user_data = NULL;
			ptr_vod_data_manager->_block_time = 0;
			range_list_clear(&range_list);
			return ret;
		}
		LOG_DEBUG("vdm_notify_read_data_result .range_list_is_include return FALSE ");

	}
	range_list_clear(&range_list);
	return ret;
}

#define  READ_BLOCK_NUM 				1
#define  VOD_MAX_ASYNC_READ_NUM 		10

BOOL vdm_range_is_write(VOD_DATA_MANAGER* ptr_vod_data_manager, RANGE r)
{
	_int32 i;
	BOOL b = TRUE;
	TASK* ptr_task = (TASK*)ptr_vod_data_manager->_task_ptr;
	RANGE range;

	for (i = 0; i < r._num; i++)
	{
		range._index = i + r._index;
		range._num = 1;
		if (P2SP_TASK_TYPE == ptr_vod_data_manager->_task_ptr->_task_type)
		{
			if (dt_is_vod_check_data_task(ptr_task))
			{
				//if(dm_range_is_check(&((P2SP_TASK*)ptr_vod_data_manager->_task_ptr)->_data_manager, &range))
				{
					b = dm_range_is_write(&((P2SP_TASK*)ptr_vod_data_manager->_task_ptr)->_data_manager, &range);
				}
			}
			else
			{
				b = dm_range_is_write(&((P2SP_TASK*)ptr_vod_data_manager->_task_ptr)->_data_manager, &range);
			}
		}
#ifdef ENABLE_BT
		else if (BT_TASK_TYPE == ptr_vod_data_manager->_task_ptr->_task_type)
		{
			b = bdm_range_is_write(&((BT_TASK*)ptr_vod_data_manager->_task_ptr)->_data_manager, ptr_vod_data_manager->_file_index, &range);
		}
#endif
		else
		{
			LOG_DEBUG("vdm_alloc_buffer_for_waiting_list .unknown task type ");
			b = FALSE;
		}
		if (FALSE == b)
		{
			return b;
		}
	}
	return b;
}

_int32  vdm_alloc_buffer_for_waiting_range(VOD_DATA_MANAGER* ptr_vod_data_manager, RANGE* to_waiting_range, _u64 file_size)
{
	_int32 i;
	RANGE range;
	VOD_RANGE_DATA_BUFFER     range_data_buffer;
	_int32                ret = SUCCESS;
	_u32                data_len;
	_u32                buffer_len;
	VOD_RANGE_DATA_BUFFER*     ptr_range_data_buffer;
	BOOL b = FALSE;
	//TASK* ptr_task = (TASK*)ptr_vod_data_manager->_task_ptr;

	RANGE_DATA_BUFFER* _tmp_read_buffer = NULL;

	for (i = 0; i < ((to_waiting_range->_num - 1) / READ_BLOCK_NUM + 1); i++)
	{
		range._index = i * READ_BLOCK_NUM + to_waiting_range->_index;
		if (i == (to_waiting_range->_num - 1) / READ_BLOCK_NUM)
		{
			range._num = to_waiting_range->_num - (i * READ_BLOCK_NUM);
		}
		else
		{
			range._num = READ_BLOCK_NUM;
		}

		LOG_DEBUG("vdm_alloc_buffer_for_waiting_list . range(%d,%d)\n", range._index, range._num);
		b = FALSE;
		b = vdm_range_is_write(ptr_vod_data_manager, range);
		if (TRUE != b)
		{
			LOG_DEBUG("vdm_alloc_buffer_for_waiting_list . dm_range_is_write return FALSE range(%d,%d)", range._index, range._num);
			continue;
		}

		ret = vdm_buffer_list_find(&ptr_vod_data_manager->_buffer_list, &range, &ptr_range_data_buffer);
		if (SUCCESS != ret)
		{
			data_len = (_u32)range_to_length(&range, file_size);
			LOG_DEBUG("vdm_alloc_buffer_for_waiting_list sd_malloc size = %llu.", data_len);
			buffer_len = data_len;

			//if buffer_len > MAX_ALLOC_SIZE then get data_buffer multi-times
			ret = vdm_get_data_buffer(ptr_vod_data_manager, &range_data_buffer._range_data_buffer._data_ptr, &buffer_len);
			if (SUCCESS != ret)
			{
				LOG_DEBUG("vdm_get_data_buffer size=%d ret =%d", data_len, ret);
				return ret;
			}
			range_data_buffer._range_data_buffer._range = range;
			range_data_buffer._range_data_buffer._data_length = data_len;
			range_data_buffer._range_data_buffer._buffer_length = buffer_len;
			LOG_DEBUG("vdm_alloc_buffer_for_waiting_list ..._data_length= %lu, _buffer_length=%d.", data_len, buffer_len);

			ret = vdm_buffer_list_add(&ptr_vod_data_manager->_buffer_list, &range_data_buffer._range_data_buffer._range, range_data_buffer._range_data_buffer._data_ptr, range_data_buffer._range_data_buffer._data_length, range_data_buffer._range_data_buffer._buffer_length, 2);
			CHECK_VALUE(ret);


			ret = vdm_buffer_list_find(&ptr_vod_data_manager->_buffer_list, &range, &ptr_range_data_buffer);
			CHECK_VALUE(ret);
		}

		/*then range is recved*/
		LOG_DEBUG("vdm_alloc_buffer_for_waiting_list dm_asyn_read_data_buffer  range(%d,%d)", range._index, range._num);
		if (P2SP_TASK_TYPE == ptr_vod_data_manager->_task_ptr->_task_type)
		{
			ret = alloc_range_data_buffer_node(&_tmp_read_buffer);
			CHECK_VALUE(ret);
			_tmp_read_buffer->_buffer_length = ptr_range_data_buffer->_range_data_buffer._buffer_length;
			_tmp_read_buffer->_data_length = ptr_range_data_buffer->_range_data_buffer._data_length;
			_tmp_read_buffer->_data_ptr = ptr_range_data_buffer->_range_data_buffer._data_ptr;
			_tmp_read_buffer->_range._index = ptr_range_data_buffer->_range_data_buffer._range._index;
			_tmp_read_buffer->_range._num = ptr_range_data_buffer->_range_data_buffer._range._num;
			ret = dm_asyn_read_data_buffer(&((P2SP_TASK*)ptr_vod_data_manager->_task_ptr)->_data_manager, _tmp_read_buffer, (notify_read_result)vdm_notify_read_data_result, (void*)ptr_vod_data_manager);
		}
#ifdef ENABLE_BT
		else if (BT_TASK_TYPE == ptr_vod_data_manager->_task_ptr->_task_type)
		{
			ret = alloc_range_data_buffer_node(&_tmp_read_buffer);
			CHECK_VALUE(ret);
			_tmp_read_buffer->_buffer_length = ptr_range_data_buffer->_range_data_buffer._buffer_length;
			_tmp_read_buffer->_data_length = ptr_range_data_buffer->_range_data_buffer._data_length;
			_tmp_read_buffer->_data_ptr = ptr_range_data_buffer->_range_data_buffer._data_ptr;
			_tmp_read_buffer->_range._index = ptr_range_data_buffer->_range_data_buffer._range._index;
			_tmp_read_buffer->_range._num = ptr_range_data_buffer->_range_data_buffer._range._num;
			ret = bdm_asyn_read_data_buffer(&((BT_TASK*)ptr_vod_data_manager->_task_ptr)->_data_manager, _tmp_read_buffer, (notify_read_result)vdm_notify_read_data_result, (void*)ptr_vod_data_manager, ptr_vod_data_manager->_file_index);
		}
#endif
		else
		{
			LOG_DEBUG("vdm_alloc_buffer_for_waiting_list .unknown task type ");
			ret = -1;
		}
		LOG_DEBUG("vdm_alloc_buffer_for_waiting_list dm_asyn_read_data_buffer  range(%d,%d) ret =%d", range._index, range._num, ret);
		if (SUCCESS == ret)
		{
			ret = range_list_add_range(&ptr_vod_data_manager->_range_waiting_list, &range, NULL, NULL);
			CHECK_VALUE(ret);
			if (range_list_get_total_num(&ptr_vod_data_manager->_range_waiting_list) > VOD_MAX_ASYNC_READ_NUM)
			{
				break;
			}
		}
	}
	return ret;
}


_int32  vdm_alloc_buffer_for_waiting_list(VOD_DATA_MANAGER* ptr_vod_data_manager, RANGE_LIST* to_waiting_list, _u64 file_size)
{
	RANGE_LIST_ITEROATOR  cur_node;
	RANGE                 range;

	range_list_get_head_node(to_waiting_list, &cur_node);
	LOG_DEBUG("vdm_alloc_buffer_for_waiting_list .");
	while (cur_node != NULL)
	{
		range = cur_node->_range;
		if (range_list_get_total_num(&ptr_vod_data_manager->_range_waiting_list) > VOD_MAX_ASYNC_READ_NUM)
		{
			break;
		}
		/* */
		vdm_alloc_buffer_for_waiting_range(ptr_vod_data_manager, &range, file_size);

		cur_node = cur_node->_next_node;
	}

	return SUCCESS;
}

_int32 vdm_drop_buffer_not_in_emergency_window(VOD_DATA_MANAGER* ptr_vod_data_manager, RANGE_LIST* em_window_list)
{
	RANGE_LIST range_list;
	VOD_RANGE_DATA_BUFFER* cur_buffer = NULL;
	VOD_RANGE_DATA_BUFFER_LIST*  buffer_list;

	LIST_ITERATOR  cur_node = NULL;
	LIST_ITERATOR  erase_node = NULL;
	_int32 ret;

	buffer_list = &ptr_vod_data_manager->_buffer_list;
	cur_node = LIST_BEGIN(buffer_list->_data_list);
	range_list_init(&range_list);

	LOG_DEBUG("vdm_drop_buffer_not_in_emergency_window .");
	out_put_range_list(em_window_list);

	while (cur_node != LIST_END(buffer_list->_data_list))
	{
		range_list_clear(&range_list);
		range_list_init(&range_list);
		cur_buffer = (VOD_RANGE_DATA_BUFFER*)LIST_VALUE(cur_node);

		if (FALSE == range_list_is_relevant(em_window_list, &cur_buffer->_range_data_buffer._range)
			&& FALSE == range_list_is_relevant(&ptr_vod_data_manager->_range_index_list, &cur_buffer->_range_data_buffer._range)
			&& FALSE == range_list_is_relevant(&ptr_vod_data_manager->_range_waiting_list, &cur_buffer->_range_data_buffer._range))
		{
			range_list_add_range(&range_list, &cur_buffer->_range_data_buffer._range, NULL, NULL);
			range_list_delete_range_list(&ptr_vod_data_manager->_range_buffer_list, &range_list);

			LOG_DEBUG("vdm_drop_buffer_not_in_emergency_window vdm_free_data_buffer(0x%x, %u) range(%u, %u).",
				cur_buffer->_range_data_buffer._data_ptr, cur_buffer->_range_data_buffer._buffer_length,
				cur_buffer->_range_data_buffer._range._index, cur_buffer->_range_data_buffer._range._num);
			if (cur_buffer->_is_recved == 1)
			{
				ret = vdm_free_data_buffer(ptr_vod_data_manager, &cur_buffer->_range_data_buffer._data_ptr, cur_buffer->_range_data_buffer._buffer_length);
				CHECK_VALUE(ret);
			}

			erase_node = cur_node;
			cur_node = LIST_NEXT(cur_node);
			list_erase(&buffer_list->_data_list, erase_node);

			free_vod_range_data_buffer_node(cur_buffer);

			/*buffer_list_delete_without_free_buffer(buffer_list, &cur_buffer->_range, &range_list);*/
		}
		else
		{
			cur_node = LIST_NEXT(cur_node);
		}
	}

	range_list_clear(&range_list);
	LOG_DEBUG("vdm_drop_buffer_not_in_emergency_window.");
	return SUCCESS;
}


_int32 vdm_process_index_parser(VOD_DATA_MANAGER* ptr_vod_data_manager, _u64 file_size, char* file_name)
{
	RANGE  range;
	_int32   file_type;
	char*      buffer = NULL;
	static _u32 s_range_num = 1;

	_int32 ret = SUCCESS;

	LOG_DEBUG("vdm_process_index_parser .");

	if (range_list_get_total_num(&ptr_vod_data_manager->_range_index_list) > 0 && 0 != ptr_vod_data_manager->_bit_per_seconds)
	{
		/*we have already have index range*/
		LOG_DEBUG("vdm_process_index_parser . we have already have index range");
		return ERR_VOD_ALREADY_HAVE_INDEX;
	}

	if (file_name == NULL)
	{
		return ERR_VOD_DATA_FETCH_TIMEOUT;
	}

	/*the first block*/
	range._index = 0;
	range._num = 1;

	range._num = s_range_num;

#if 0//_DEBUG
	printf("\n @@@@@@vdm_process_index_parser:range_buffer_list._node_size=%u \n",ptr_vod_data_manager->_range_buffer_list._node_size);
	if(ptr_vod_data_manager->_range_buffer_list._node_size!=0)
	{
		printf("\n @@@@@@vdm_process_index_parser:head_node[%u,%u],range[%u,%u] \n",ptr_vod_data_manager->_range_buffer_list._head_node->_range._index,ptr_vod_data_manager->_range_buffer_list._head_node->_range._num,range._index,range._num);
	}
#endif
	if (TRUE == range_list_is_include(&ptr_vod_data_manager->_range_buffer_list, &range))
	{

		ret = sd_malloc(get_data_unit_size()*range._num, (void **)&buffer);
		if (SUCCESS != ret)
		{
			return ret;
		}

		ret = vdm_write_data_buffer(ptr_vod_data_manager, 0, buffer, get_data_unit_size()*range._num, &range, &ptr_vod_data_manager->_buffer_list);
		if (SUCCESS != ret)
		{
			sd_free(buffer);
			buffer = NULL;
			return ret;
		}
		/*parse index here*/
		LOG_DEBUG("vdm_process_index_parser , parse index. file_name:%s", file_name);
		if (sd_strstr(file_name, ".RM", 0) != NULL || sd_strstr(file_name, ".rm", 0) != NULL)
		{
			file_type = INDEX_PARSER_FILETYPE_RMVB;
		}
		else if (sd_strstr(file_name, ".WMV", 0) != NULL || sd_strstr(file_name, ".wmv", 0) != NULL)
		{
			file_type = INDEX_PARSER_FILETYPE_WMV;
		}
		else if (buffer[0] == 'F' && buffer[1] == 'L' && buffer[2] == 'V')//sd_strstr(file_name, ".FLV", 0 ) != NULL || sd_strstr(file_name, ".flv", 0 ) != NULL)
		{
			file_type = INDEX_PARSER_FILETYPE_FLV;
		}
		else
		{
			file_type = INDEX_PARSER_FILETYPE_UNKNOWN;
		}
		ret = ip_get_index_range_list_by_file_head(buffer, get_data_unit_size()*range._num, file_size, file_type, &ptr_vod_data_manager->_range_index_list, &ptr_vod_data_manager->_bit_per_seconds, &ptr_vod_data_manager->_flv_header_to_end_of_first_tag_len);
		if (ret == ERR_VOD_INDEX_FILE_HEAD_TOO_SMALL && s_range_num < 30)
		{
			s_range_num++;
		}
		else
		{
			s_range_num = 1;
		}
		LOG_DEBUG("vdm_process_index_parser , ip_get_index_range_list_by_file_head ret = %d.", ret);

		sd_free(buffer);
		buffer = NULL;
	}

	LOG_DEBUG("vdm_process_index_parser done...");
	return ret;
}

BOOL     vdm_pos_is_in_range_list(_u64 pos, _u64 len, _u64 filesize, RANGE_LIST* range_list)
{
	RANGE_LIST_ITEROATOR  cur_node;
	RANGE range;

	range_list_get_head_node(range_list, &cur_node);
	LOG_DEBUG("vdm_pos_is_in_range_list .pos=%lu, rangelist:", pos);
	out_put_range_list(range_list);
	if (pos + len > filesize)
	{
		len = filesize - pos;
	}
	while (cur_node != NULL)
	{
		range = cur_node->_range;
		if (pos + len <= (_u64)(get_data_pos_from_index(range._index) + range_to_length(&range, filesize))
			&& pos >= (_u64)get_data_pos_from_index(range._index))
		{
			LOG_DEBUG("vdm_pos_is_in_range_list .return TRUE");
			return TRUE;
		}

		cur_node = cur_node->_next_node;
	}
	LOG_DEBUG("vdm_pos_is_in_range_list .return FALSE");
	return FALSE;
}

_int32 vdm_get_continuing_end_pos(_u64 start_pos, _u64 file_size, RANGE_LIST* range_total_recved, _u64* p_end_pos)
{
	RANGE_LIST_ITEROATOR  cur_node;
	RANGE range;
	_u64 range_pos;
	_u64 range_len;
	*p_end_pos = start_pos;
	range_list_get_head_node(range_total_recved, &cur_node);
	while (cur_node != NULL)
	{
		range = cur_node->_range;
		range_pos = get_data_pos_from_index(range._index);
		range_len = range_to_length(&range, file_size);
		if (start_pos <= (range_pos + range_len) && start_pos >= range_pos)
		{
			*p_end_pos = range_pos + range_len;
			break;
		}

		cur_node = cur_node->_next_node;
	}

	LOG_DEBUG("vdm_get_continuing_end_pos . start_pos=%llu,   end_pos=%llu", start_pos, *p_end_pos);
	return SUCCESS;
}

_u32  vdm_cal_bytes_per_second(_u64 file_size, _u32 bit_per_seconds)
{
	_u32 bytes_per_second = 0;
	if (bit_per_seconds == 0)
	{
		if (file_size > VDM_MIN_HD_FILE_SIZE)
		{
			bytes_per_second = VDM_MAX_BYTES_RATE;
		}
		else
		{
			bytes_per_second = VDM_DEFAULT_BYTES_RATE;
		}
	}
	else
		//	if( bit_per_seconds > VDM_DEFAULT_BYTES_RATE*8)
	{
		bytes_per_second = (bit_per_seconds + 7) / 8;
	}
	//	else
	//	{
	//	         bytes_per_second = VDM_DEFAULT_BYTES_RATE;
	//	}

	return bytes_per_second;

}

_int32 	vdm_cdn_use_dispatch(VOD_DATA_MANAGER* ptr_vod_data_manager)
{
	static _u32 set_mode_count = 0;
	BOOL use_cdn;
	_u32  cdn_speed;
	_u64  play_pos;
	_u64  download_pos;
	_u64  filesize;
	_int32 ret;
	_u32 bytes_per_seconds;
	RANGE_LIST* range_total_recved;
	/*get DOWNLOAD_TASK ptr here*/
	TASK* ptr_task = (TASK*)ptr_vod_data_manager->_task_ptr;

	LOG_DEBUG("vdm_cdn_use_dispatch.... ");
	range_total_recved = NULL;
	ret = SUCCESS;
	if (P2SP_TASK_TYPE == ptr_task->_task_type)
	{
		filesize = dm_get_file_size(&((P2SP_TASK*)ptr_task)->_data_manager);
	}
#ifdef ENABLE_BT
	else if (BT_TASK_TYPE == ptr_task->_task_type)
	{
		filesize = bdm_get_sub_file_size(&((BT_TASK*)ptr_task)->_data_manager, ptr_vod_data_manager->_file_index);
	}
#endif
	else
	{
		LOG_DEBUG("vdm_cdn_use_dispatch .unknown task type ");
		return ret;
	}

	if (P2SP_TASK_TYPE == ptr_vod_data_manager->_task_ptr->_task_type)
	{
		range_total_recved = dm_get_recved_range_list(&((P2SP_TASK*)ptr_vod_data_manager->_task_ptr)->_data_manager);
	}
#ifdef ENABLE_BT
	else if (BT_TASK_TYPE == ptr_vod_data_manager->_task_ptr->_task_type)
	{
		/*
		range_total_recved = dm_get_recved_range_list(&((BT_TASK*)ptr_vod_data_manager->_task_ptr)->_data_manager,);
		*/
		LOG_DEBUG("vdm_cdn_use_dispatch . bt is not support cdn  ret=%d", ret);
		return ret;
	}
#endif
	use_cdn = TRUE;
	bytes_per_seconds = vdm_cal_bytes_per_second(filesize, ptr_vod_data_manager->_bit_per_seconds);
	/*if speed > bitrates*1.5 then don't use cdn*/
#ifdef ENABLE_CDN
	cdn_speed = pt_get_cdn_speed(ptr_task);
#else
	cdn_speed=0;
#endif
	LOG_ERROR("vdm_cdn_use_dispatch 1. speed=%u,filesize=%llu,real_bitrates=%u,bitrates=%u,  bitrates*1.5 =%u, cdn_speed=%u", ptr_task->speed, filesize, ptr_vod_data_manager->_bit_per_seconds, bytes_per_seconds, bytes_per_seconds * 3 / 2, cdn_speed);

	/*if play pos > download pos > 1 min data then don't use cdn*/
	play_pos = ptr_vod_data_manager->_start_pos;
	ret = vdm_get_continuing_end_pos(play_pos, filesize, range_total_recved, &download_pos);
	if (SUCCESS != ret)
	{
		LOG_DEBUG("vdm_cdn_use_dispatch . vdm_get_continuing_end_pos  ret=%d", ret);
		return ret;
	}


	LOG_DEBUG("vdm_cdn_use_dispatch 2. download_pos = %llu -play_pos = %llu )  window = (%lu) ", download_pos, play_pos, (bytes_per_seconds*VDM_EM_WINDOW_TIME_LEN));
	use_cdn = TRUE;
	if ((ptr_task->speed > cdn_speed) && ((ptr_task->speed - cdn_speed) > (bytes_per_seconds * 3 / 2)))
	{
		use_cdn = FALSE;
		LOG_DEBUG("vdm_cdn_use_dispatch . use_cdn=%lu speed%d > bitrates*1.5 cdn_speed=%d", use_cdn, ptr_task->speed, cdn_speed);
	}

	if ((download_pos > play_pos) && (((filesize > 0) && (ptr_task->_downloaded_data_size == filesize)) || (download_pos - play_pos) > (bytes_per_seconds*VDM_EM_WINDOW_TIME_LEN * 2)))
	{
		LOG_DEBUG("vdm_cdn_use_dispatch . use_cdn=%lu download_pos = %llu -play_pos = %llu )> (%lu) ", use_cdn, download_pos, play_pos, (bytes_per_seconds*VDM_EM_WINDOW_TIME_LEN * 2));
		use_cdn = FALSE;
	}

	if (ptr_vod_data_manager->_cdn_using != use_cdn)
	{
		if (use_cdn == FALSE)
		{
#ifdef EMBED_REPORT
			ptr_vod_data_manager->_cdn_stop_times++;
#endif
		}
		ptr_vod_data_manager->_cdn_using = use_cdn;

		/* 停掉或启用其他任务的下载模式 */
		tm_pause_download_except_task(ptr_task, use_cdn);

		set_mode_count = 0;
	}
	else if (set_mode_count++ > FORCE_SET_CDN_MODE_COUNT)
	{
		/* 之所以要强制设置CDN模式,是因为在这个任务被置为点播任务后启动的任务有可能会抢占带宽by zyq@20110119 */

		/* 停掉或启用其他任务的下载模式 */
		tm_pause_download_except_task(ptr_task, use_cdn);
		set_mode_count = 0;
	}

#ifdef ENABLE_CDN
	pt_set_cdn_mode(ptr_task, use_cdn);
#endif

	return SUCCESS;

}
_int32 	vdm_get_dm_buffer_to_buffer_list_from_range(VOD_DATA_MANAGER* ptr_vod_data_manager, RANGE range_to_read)
{
	RANGE range;
	RANGE_DATA_BUFFER_LIST ret_range_buffer;
	BOOL b;
	TASK* ptr_task = (TASK*)ptr_vod_data_manager->_task_ptr;
	_int32 ret;
	BOOL bCheckData;
	_int32 i;

	ret = SUCCESS;
	bCheckData = FALSE;
	bCheckData = dt_is_vod_check_data_task(ptr_task);
	//LOG_DEBUG("vdm_get_dm_buffer_to_buffer_list_from_range(%d,%d) .", range_to_read._index, range_to_read._num);
	for (i = 0; i < range_to_read._num; i++)
	{
		range._index = range_to_read._index + i;
		range._num = 1;
		list_init(&ret_range_buffer._data_list);

#ifdef _VOD_DEBUG
		LOG_DEBUG("vdm_get_dm_buffer_to_buffer_list_from_range vdm_sync_data_buffer_to_range_list range(%d, %d)...", range._index, range._num);
#endif
		if (P2SP_TASK_TYPE == ptr_vod_data_manager->_task_ptr->_task_type)
		{
			b = FALSE;
			if (bCheckData)
			{
				//if( dm_range_is_check(&((P2SP_TASK*)ptr_vod_data_manager->_task_ptr)->_data_manager, &range))
				{
					b = dm_range_is_recv(&((P2SP_TASK*)ptr_vod_data_manager->_task_ptr)->_data_manager, &range);
				}
			}
			else
			{
				b = dm_range_is_recv(&((P2SP_TASK*)ptr_vod_data_manager->_task_ptr)->_data_manager, &range);
			}
		}
#ifdef ENABLE_BT
		else if (BT_TASK_TYPE == ptr_vod_data_manager->_task_ptr->_task_type)
		{
			b = bdm_range_is_cache(&((BT_TASK*)ptr_vod_data_manager->_task_ptr)->_data_manager, ptr_vod_data_manager->_file_index, &range);
		}
#endif
		else
		{
			LOG_DEBUG("vdm_sync_buffer_to_buffer_list .unknown task type ");
			b = FALSE;
		}

		if (TRUE != b)
		{
#ifdef _VOD_DEBUG
			LOG_DEBUG("vdm_sync_buffer_to_buffer_list .dm_range_is_write return = false");
#endif
			continue;
		}


		if (P2SP_TASK_TYPE == ptr_task->_task_type)
		{
			ret = dm_get_range_data_buffer(&((P2SP_TASK*)ptr_task)->_data_manager, range, &ret_range_buffer);
		}
#ifdef ENABLE_BT
		else if (BT_TASK_TYPE == ptr_task->_task_type)
		{
			ret = bdm_get_range_data_buffer(&((BT_TASK*)ptr_task)->_data_manager, ptr_vod_data_manager->_file_index, range, &ret_range_buffer);
		}
#endif
		else
		{
			LOG_DEBUG("vdm_period_dispatch .unknown task type ");
			break;
		}

		if (SUCCESS != ret)
		{
			LOG_DEBUG("vdm_period_dispatch .dm_get_range_data_buffer return = %d", ret);
			list_clear(&ret_range_buffer._data_list);
			continue;
		}
		ret = vdm_sync_data_buffer_to_range_list(ptr_vod_data_manager, &ret_range_buffer);
		if (SUCCESS != ret)
		{
			LOG_DEBUG("vdm_period_dispatch .vdm_sync_data_buffer_to_range_list return = %d", ret);
			list_clear(&ret_range_buffer._data_list);
			continue;
		}
#ifdef ENABLE_FLASH_PLAYER
		/* 只能在android 平台中用于flash 播放器点播 */
		vdm_process_index_parser(ptr_vod_data_manager, ptr_task->file_size, "FLV");
#endif /* ENABLE_FLASH_PLAYER */
#ifdef _VOD_NO_DISK
		if(P2SP_TASK_TYPE == ptr_task->_task_type)
		{
			if(TRUE == dt_is_no_disk_task(ptr_task))
			{
				ret = dm_vod_no_disk_notify_free_data_buffer( &((P2SP_TASK*)ptr_task)->_data_manager, &ret_range_buffer);
			}
		}
#endif

		list_clear(&ret_range_buffer._data_list);
	}

	return SUCCESS;

}

_int32  vdm_dm_notify_flush_finished(_u32 task_id)
{
	_int32 ret = SUCCESS;

	VOD_DATA_MANAGER* vdm = NULL;
	ret = vdm_get_vod_data_manager_by_task_id(&g_vdm_list, task_id, 0, &vdm, TRUE, FALSE);
	if (SUCCESS != ret || NULL == vdm)
	{
		return ERR_VOD_DATA_MANAGER_NOT_FOUND;
	}

	TASK *task = vdm->_task_ptr;
	if (task == NULL)
		return -1;

	_u64 continue_end_pos = 0;
	_u64 file_size = dm_get_file_size(&(((P2SP_TASK*)task)->_data_manager));
	vdm_get_continuing_end_pos(vdm->_start_pos, file_size, &vdm->_range_buffer_list, &continue_end_pos);
	if (vdm->_last_notify_read_pos < continue_end_pos)
	{
		if (vdm->_can_read_data_notify != NULL)
		{
			vdm->_can_read_data_notify(vdm->_can_read_data_notify_args, continue_end_pos);
		}
		vdm->_last_notify_read_pos = continue_end_pos;
	}
	return ret;
}
_int32     vdm_dm_notify_buffer_recved(TASK* task_ptr, RANGE_DATA_BUFFER* range_data)
{
	_int32 ret = SUCCESS;

	RANGE_LIST* range_list;
	char* data_ptr;
	_u32 data_len;
	_int32 file_index = 0;
	VOD_DATA_MANAGER* ptr_vod_data_manager = NULL;

	LOG_DEBUG("vdm_sync_data_buffer_to_range_list  range(%u, %u)", range_data->_range._index, range_data->_range._num);
	ret = vdm_get_vod_data_manager_by_task_id(&g_vdm_list, task_ptr->_task_id, file_index, &ptr_vod_data_manager, TRUE, FALSE);
	if (SUCCESS != ret || NULL == ptr_vod_data_manager)
	{
		return ERR_VOD_DATA_MANAGER_NOT_FOUND;
	}

	range_list = &ptr_vod_data_manager->_range_buffer_list;

	if (range_list_is_include(range_list, &range_data->_range))
	{
		LOG_DEBUG("vdm_sync_data_buffer_to_range_list range(%u, %u) already in bufferlist.", range_data->_range._index, range_data->_range._num);
		//已经有了
		return SUCCESS;
	}


	LOG_DEBUG("vdm_sync_data_buffer_to_range_list sd_malloc %d.", range_data->_buffer_length);
	data_len = range_data->_buffer_length;
	ret = vdm_get_data_buffer(ptr_vod_data_manager, &data_ptr, &data_len);
	if (SUCCESS != ret)
	{
		LOG_DEBUG("vdm_sync_data_buffer_to_range_list vdm_get_data_buffer size=%d ret =%d", data_len, ret);
		return ret;
	}
	sd_memcpy(data_ptr, range_data->_data_ptr, range_data->_buffer_length);

	//這里要修改是設置緩沖區進去
	ret = vdm_buffer_list_add(&ptr_vod_data_manager->_buffer_list, &range_data->_range, data_ptr, range_data->_data_length, range_data->_buffer_length, 1);
	if (SUCCESS == ret)
	{
		range_list_add_range(range_list, &range_data->_range, NULL, NULL);
	}

	LOG_DEBUG("vdm_dm_notify_buffer_recved success.");
	return ret;

}

_int32 	vdm_get_dm_buffer_to_buffer_list(VOD_DATA_MANAGER* ptr_vod_data_manager, RANGE_LIST* p_list_to_sync)
{
	RANGE range;
	RANGE_LIST_ITEROATOR  cur_node;
	TASK* ptr_task = (TASK*)ptr_vod_data_manager->_task_ptr;
	BOOL bCheckData;

	bCheckData = dt_is_vod_check_data_task(ptr_task);
	range_list_get_head_node(p_list_to_sync, &cur_node);
	LOG_DEBUG("vdm_sync_buffer_to_buffer_list .");
	while (cur_node != NULL)
	{
		range = cur_node->_range;
		//LOG_DEBUG("vdm_sync_buffer_to_buffer_list vdm_sync_data_buffer_to_range_list range(%d, %d)...", range._index, range._num);
		vdm_get_dm_buffer_to_buffer_list_from_range(ptr_vod_data_manager, range);
		cur_node = cur_node->_next_node;
	}

	return SUCCESS;

}

RANGE  vdm_get_block_range_by_range(RANGE range, _u64 filesize, _u32 blocksize)
{
	_u64 pos;
	_u64 len;
	_u64 endpos;
	pos = get_data_pos_from_index(range._index);
	len = range_to_length(&range, filesize);
	endpos = pos + len;
	if ((pos % (_u64)blocksize) > 0)
	{
		pos = (pos / (_u64)blocksize) * (_u64)blocksize;
	}

	if ((endpos % (_u64)blocksize) > 0)
	{
		endpos = ((endpos + (_u64)blocksize - 1) / (_u64)blocksize) * (_u64)blocksize;
	}

	return pos_length_to_range(pos, endpos - pos, filesize);
}

#if 0
_int32 	vdm_period_dispatch(VOD_DATA_MANAGER* ptr_vod_data_manager)
{
	RANGE_LIST em_range_list;
	RANGE_LIST range_list_to_get;
	RANGE_LIST new_em_range_list;
	RANGE_LIST range_list_next_em_range;
	RANGE_LIST range_list_tmp;
	const RANGE_LIST* range_total_recved;
	RANGE_LIST range_list_null;
	_int32 ret;
	_u64  filesize;
	RANGE  range_to_get;
	RANGE  em_range_window;
	RANGE  drag_em_range_window;
	RANGE  new_em_range_windows;
	RANGE range;
	_u64  windowsize;
	_u64  dragwindowsize;
	_u64  newstartpos;
	_int32 bytes_per_seconds;
	char*  file_name;
	_u32   now_time;
	/*get DOWNLOAD_TASK ptr here*/
	TASK* ptr_task = (TASK*)ptr_vod_data_manager->_task_ptr;
	TASK* taskinfo = NULL;

	LOG_DEBUG("vdm_period_dispatch .taskid=%d...", ptr_task->_task_id);
	ret = tm_get_task_by_id( ptr_task->_task_id,&taskinfo);
	if(SUCCESS != ret || taskinfo == NULL)
	{
		LOG_DEBUG("vdm_period_dispatch .task is not found,taskid= %d ", ptr_task->_task_id);
		return ERR_VOD_DATA_TASK_STOPPED;
	}

	range_total_recved = NULL;
	ret = SUCCESS;

	//超时
	sd_time_ms(&now_time);
	if(ptr_vod_data_manager->_block_time > 0)
	{
		if( TIME_SUBZ(now_time, ptr_vod_data_manager->_last_fetch_time) > ptr_vod_data_manager->_block_time )
		{
			LOG_DEBUG("vdm_period_dispatch nowtime=%lu, - lasttime=%d bigger than blocktime %u ", now_time, ptr_vod_data_manager->_last_fetch_time, ptr_vod_data_manager->_block_time);
			ret = ERR_VOD_DATA_FETCH_TIMEOUT;
			return ret;
		}
	}

	//判斷任務狀態
	if(TASK_S_FAILED == ptr_task->task_status)
	{
		LOG_DEBUG("vdm_period_dispatch task_status is failed=%d ",  ptr_task->task_status);
		ret = ERR_VOD_DATA_TASK_STOPPED;
		return ret;
	}

	if(P2SP_TASK_TYPE == ptr_task->_task_type)
	{
		filesize = dm_get_file_size(&((P2SP_TASK*)ptr_task)->_data_manager);
	}
#ifdef ENABLE_BT
	else if(BT_TASK_TYPE == ptr_task->_task_type)
	{
		filesize = bdm_get_sub_file_size(&((BT_TASK*)ptr_task)->_data_manager, ptr_vod_data_manager->_file_index);
	}
#endif
	else
	{
		LOG_DEBUG("vdm_period_dispatch .unknown task type ");
		return ERR_VOD_DATA_UNKNOWN_TASK;
	}
	if(filesize == 0)
	{
		LOG_DEBUG("vdm_period_dispatch .unknown task filesize 2");
		return ERR_VOD_DATA_UNKNOWN_TASK;
	}

	if(P2SP_TASK_TYPE == ptr_task->_task_type)
	{
		if(FALSE == dm_get_filnal_file_name(&((P2SP_TASK*)ptr_task)->_data_manager, &file_name))
		{
			LOG_DEBUG("vdm_period_dispatch .dm_get_filnal_file_name failed");
			return ret;
		}
	}
#ifdef ENABLE_BT
	else if(BT_TASK_TYPE == ptr_task->_task_type)
	{
		if(SUCCESS != bdm_get_file_name(&((BT_TASK*)ptr_task)->_data_manager,ptr_vod_data_manager->_file_index ,  &file_name))
		{
			LOG_DEBUG("vdm_period_dispatch .bdm_get_file_name failed");
			return ret;
		}
	}
#endif
	else
	{
		LOG_DEBUG("vdm_period_dispatch .unknown task type ");
		return ERR_VOD_DATA_UNKNOWN_TASK;
	}


#ifdef ENABLE_BT
	if(BT_TASK_TYPE == ptr_task->_task_type)
	{
		if( BT_FILE_FAILURE == bdm_get_file_status(&((BT_TASK*)ptr_task)->_data_manager, ptr_vod_data_manager->_file_index))
		{
			bdm_set_vod_mode(&((BT_TASK*)ptr_task)->_data_manager, FALSE);
			LOG_DEBUG("vdm_period_dispatch .bdm_get_file_status == BT_FILE_FAILURE");
			return ERR_VOD_DATA_TASK_STOPPED;
		}
	}
#endif
	if(TRUE != ptr_vod_data_manager->_is_fetching)
	{
		LOG_DEBUG("vdm_period_dispatch is not in fetching state...");
		return ERR_VOD_DATA_NOT_IN_FETCH_STATE;
	}

	LOG_DEBUG("vdm_period_dispatch.debug taskid=%u, fileindex=%u, start_pos=%llu, fetch_length=%llu, filesize=%llu,blocktime=%u ",  ptr_task->_task_id,  ptr_vod_data_manager->_file_index, ptr_vod_data_manager->_start_pos, ptr_vod_data_manager->_fetch_length, filesize, ptr_vod_data_manager->_block_time);

	range_to_get = pos_length_to_range(ptr_vod_data_manager->_start_pos, ptr_vod_data_manager->_fetch_length, filesize);
	bytes_per_seconds = vdm_cal_bytes_per_second(filesize,ptr_vod_data_manager->_bit_per_seconds);
	windowsize = bytes_per_seconds*VDM_EM_WINDOW_TIME_LEN;
	windowsize = (windowsize<ptr_vod_data_manager->_fetch_length)?ptr_vod_data_manager->_fetch_length:windowsize;
	em_range_window = pos_length_to_range(ptr_vod_data_manager->_start_pos, windowsize, filesize);

	dragwindowsize = bytes_per_seconds*vdm_get_buffer_time_len();
	dragwindowsize = (dragwindowsize<ptr_vod_data_manager->_fetch_length)?ptr_vod_data_manager->_fetch_length:dragwindowsize;
	drag_em_range_window = pos_length_to_range(ptr_vod_data_manager->_start_pos, dragwindowsize, filesize);

	LOG_DEBUG("vdm_period_dispatch range_to_get(%d, %d), em_range_window(%d, %d), bytes_per_seconds=%d ", range_to_get._index, range_to_get._num, em_range_window._index, em_range_window._num, bytes_per_seconds);
	range_list_init(&em_range_list);
	range_list_init(&range_list_to_get);
	range_list_init(&new_em_range_list);
	range_list_init(&range_list_next_em_range);
	range_list_init(&range_list_null);
	range_list_add_range(&range_list_to_get , &range_to_get, NULL, NULL);
	range_list_add_range(&em_range_list, &em_range_window, NULL, NULL);
	/*add extra range to em_range_list again, by bitrates*/




	/* sub the range list first*/
	ret = range_list_delete_range_list(&range_list_to_get, &ptr_vod_data_manager->_range_buffer_list);
	if(SUCCESS != ret)
	{
		LOG_DEBUG("vdm_vod_read_file .range_list_delete_range_list return = %d", ret);
		range_list_clear(&em_range_list);
		range_list_clear(&range_list_to_get);
		range_list_clear(&new_em_range_list);
		range_list_clear(&range_list_next_em_range);
		range_list_clear(&range_list_null);
		return ret;
	}



	/*if the first block has received, parse the head to get index range*/
	ret =  vdm_process_index_parser(ptr_vod_data_manager, filesize, file_name);
	LOG_DEBUG("vdm_vod_read_file .vdm_process_index_parser return = %d", ret);
	range_list_init(&range_list_tmp);
	range_list_add_range_list(&range_list_tmp, &ptr_vod_data_manager->_range_index_list);
	range_list_delete_range_list(&range_list_tmp, &ptr_vod_data_manager->_range_buffer_list);
	if(range_list_get_total_num(&range_list_tmp)>0)
	{
		ret = vdm_get_dm_buffer_to_buffer_list(ptr_vod_data_manager, &range_list_tmp);
	}
	range_list_clear(&range_list_tmp);



	/*determinate the window of buffer, and free the buffer not in the window*/
	/*and if the buffer is in the index range window, so prior to keep it */
#ifdef _VOD_NO_DISK
	if(TRUE == dt_is_no_disk_task(ptr_task))
	{
		range_list_add_range_list(&em_range_list, &ptr_vod_data_manager->_range_index_list);

		range_list_init(&range_list_tmp);
		range_list_cp_range_list(&em_range_list, &range_list_tmp);
		range_list_add_range(&range_list_tmp, &ptr_vod_data_manager->_current_download_window, NULL, NULL);
		ret = vdm_drop_buffer_not_in_emergency_window(ptr_vod_data_manager, &range_list_tmp);
		if(SUCCESS != ret)
		{
			LOG_DEBUG("vdm_drop_buffer_not_in_emergency_window return = %d", ret);
			range_list_clear(&em_range_list);
			range_list_clear(&range_list_to_get);
			range_list_clear(&new_em_range_list);
			range_list_clear(&range_list_next_em_range);
			range_list_clear(&range_list_null);
			range_list_clear(&range_list_tmp);
			return ret;
		}
		//add current_download_window
		range_list_add_range(&range_list_tmp, &ptr_vod_data_manager->_current_download_window, NULL, NULL);
		if(P2SP_TASK_TYPE == ptr_task->_task_type)
		{
			ret = dm_drop_buffer_not_in_emerge_windows( &((P2SP_TASK*)ptr_task)->_data_manager, &range_list_tmp);
		}

#ifdef ENABLE_BT
		else if(BT_TASK_TYPE == ptr_task->_task_type)
		{
			//ret = bdm_get_range_data_buffer( &((BT_TASK*)ptr_task)->_data_manager, ptr_vod_data_manager->_file_index , range,  &ret_range_buffer);
		}
#endif
		range_list_clear(&range_list_tmp);
	}
	else
	{
		//have disk
		range_list_add_range_list(&em_range_list, &ptr_vod_data_manager->_range_index_list);
		ret = vdm_drop_buffer_not_in_emergency_window(ptr_vod_data_manager, &em_range_list);
		if(SUCCESS != ret)
		{
			LOG_DEBUG("vdm_drop_buffer_not_in_emergency_window return = %d", ret);
			range_list_clear(&em_range_list);
			range_list_clear(&range_list_to_get);
			range_list_clear(&new_em_range_list);
			range_list_clear(&range_list_next_em_range);
			range_list_clear(&range_list_null);
			return ret;
		}
	}


#else
	range_list_add_range_list(&em_range_list, &ptr_vod_data_manager->_range_index_list);
	ret = vdm_drop_buffer_not_in_emergency_window(ptr_vod_data_manager, &em_range_list);
	if(SUCCESS != ret)
	{
		LOG_DEBUG("vdm_drop_buffer_not_in_emergency_window return = %d", ret);
		range_list_clear(&em_range_list);
		range_list_clear(&range_list_to_get);
		range_list_clear(&new_em_range_list);
		range_list_clear(&range_list_next_em_range);
		range_list_clear(&range_list_null);
		return ret;
	}
#endif

	ret = vdm_get_dm_buffer_to_buffer_list(ptr_vod_data_manager, &range_list_to_get);


	ret = range_list_delete_range_list(&range_list_to_get, &ptr_vod_data_manager->_range_buffer_list);
	if(SUCCESS != ret)
	{
		LOG_DEBUG("vdm_vod_read_file .range_list_delete_range_list return = %d", ret);
		range_list_clear(&em_range_list);
		range_list_clear(&range_list_to_get);
		range_list_clear(&new_em_range_list);
		range_list_clear(&range_list_next_em_range);
		range_list_clear(&range_list_null);
		return ret;
	}

#ifdef _VOD_NO_DISK
	if(TRUE != dt_is_no_disk_task(ptr_task))
	{
		ret = range_list_delete_range_list(&range_list_to_get, &ptr_vod_data_manager->_range_waiting_list);
	}
#else
	ret = range_list_delete_range_list(&range_list_to_get, &ptr_vod_data_manager->_range_waiting_list);
#endif

#ifdef _VOD_NO_DISK
	if(TRUE != dt_is_no_disk_task(ptr_task))
	{
		ret = vdm_alloc_buffer_for_waiting_list(ptr_vod_data_manager, &range_list_to_get,  filesize);
		if(SUCCESS != ret)
		{
			LOG_DEBUG("vdm_alloc_buffer_for_waiting_list return = %d", ret);
			range_list_clear(&em_range_list);
			range_list_clear(&range_list_to_get);
			range_list_clear(&new_em_range_list);
			range_list_clear(&range_list_next_em_range);
			range_list_clear(&range_list_null);
			return ret;
		}

		ret = range_list_delete_range_list(&range_list_to_get, &ptr_vod_data_manager->_range_waiting_list);
	}

#else
	ret = vdm_alloc_buffer_for_waiting_list(ptr_vod_data_manager, &range_list_to_get,  filesize);
	if(SUCCESS != ret)
	{
		LOG_DEBUG("vdm_alloc_buffer_for_waiting_list return = %d", ret);
		range_list_clear(&em_range_list);
		range_list_clear(&range_list_to_get);
		range_list_clear(&new_em_range_list);
		range_list_clear(&range_list_next_em_range);
		range_list_clear(&range_list_null);
		return ret;
	}

	ret = range_list_delete_range_list(&range_list_to_get, &ptr_vod_data_manager->_range_waiting_list);
#endif


	if(P2SP_TASK_TYPE == ptr_vod_data_manager->_task_ptr->_task_type)
	{
		range_total_recved = dm_get_recved_range_list(&((P2SP_TASK*)ptr_vod_data_manager->_task_ptr)->_data_manager);
	}
#ifdef ENABLE_BT
	else if(BT_TASK_TYPE == ptr_vod_data_manager->_task_ptr->_task_type)
	{

		range_total_recved = bdm_get_resv_range_list(&((BT_TASK*)ptr_vod_data_manager->_task_ptr)->_data_manager, ptr_vod_data_manager->_file_index);
		if(NULL == range_total_recved)
		{
			range_total_recved = &range_list_null;
		}

	}
#endif

	if(NULL == range_total_recved)
	{
		LOG_DEBUG("vdm_vod_read_file .unknown task type ");
		return ERR_VOD_DATA_UNKNOWN_TASK;
	}

	LOG_DEBUG("vdm_vod_read_file . range_to_get (%d,%d)", range_to_get._index, range_to_get._num);
	LOG_DEBUG("vdm_vod_read_file . the last em_range_list is :");
	out_put_range_list(&ptr_vod_data_manager->_range_last_em_list);
	LOG_DEBUG("vdm_vod_read_file . the em_range_list is :");
	out_put_range_list(&em_range_list);
	LOG_DEBUG("vdm_vod_read_file . total_recved_range_list is :");
	out_put_range_list((RANGE_LIST*)range_total_recved);
	LOG_DEBUG("vdm_vod_read_file . >_range_buffer_list is :");
	out_put_range_list((RANGE_LIST*)&ptr_vod_data_manager->_range_buffer_list);
	LOG_DEBUG("vdm_period_dispatch.debug . current_download_window is (%d,%d)", ptr_vod_data_manager->_current_download_window._index, ptr_vod_data_manager->_current_download_window._num);

	range_list_init(&range_list_tmp);
	range_list_add_range(&range_list_tmp, &ptr_vod_data_manager->_current_download_window, NULL, NULL);
	range_list_add_range_list(&range_list_tmp, &ptr_vod_data_manager->_range_buffer_list);
	/*drag*/
	if(FALSE == range_list_is_include(&range_list_tmp, &range_to_get))
	{
		/*do drag*/
		LOG_DEBUG("vdm_period_dispatch.debug do drag, new current_download_window(%d,%d)",drag_em_range_window._index, drag_em_range_window._num);
		new_em_range_windows = drag_em_range_window;
		if( dt_is_vod_check_data_task(ptr_task))
		{
			//adjust range to block range
			new_em_range_windows = vdm_get_block_range_by_range(new_em_range_windows, filesize, compute_block_size(filesize));
			LOG_DEBUG("vdm_period_dispatch.debug do drag, adjust adjust!! new current_download_window(%d,%d)",drag_em_range_window._index, drag_em_range_window._num);
		}
	}
	else
	{
		//is include ,no drag
		new_em_range_windows = ptr_vod_data_manager->_current_download_window;
	}
	range_list_clear(&range_list_tmp);


	//get next em_window
	while(TRUE)
	{
		LOG_DEBUG("vdm_period_dispatch .new_em_range_windows(%d,%d)", new_em_range_windows._index, new_em_range_windows._num);
		if(new_em_range_windows._num == 0)
		{
			LOG_DEBUG("vdm_period_dispatch .there's no more range to set em list");
			break;
		}

		range = relevant_range(&new_em_range_windows, &em_range_window);
		range_list_init(&range_list_tmp);
		range_list_add_range(&range_list_tmp, &em_range_window, NULL, NULL);
		//sync buffer to buffer_list
		range_list_delete_range_list(&range_list_tmp, &ptr_vod_data_manager->_range_buffer_list);
		ret = vdm_get_dm_buffer_to_buffer_list(ptr_vod_data_manager, &range_list_tmp);
		range_list_clear(&range_list_tmp);

		range_list_add_range(&new_em_range_list, &new_em_range_windows, NULL, NULL);
		range_list_add_range_list(&new_em_range_list, &ptr_vod_data_manager->_range_index_list);
		LOG_DEBUG("vdm_period_dispatch . the new_em_range_list total num=%d ,  _range_last_em_list totalnum=%d:" , range_list_get_total_num(&new_em_range_list) , range_list_get_total_num(&ptr_vod_data_manager->_range_last_em_list) );
#ifdef _VOD_NO_DISK
		if(TRUE == dt_is_no_disk_task(ptr_task))
		{
			range_list_delete_range_list(&new_em_range_list, &ptr_vod_data_manager->_range_buffer_list);
		}
		else
		{
			range_list_delete_range_list(&new_em_range_list, (RANGE_LIST*)range_total_recved);
		}
#else
		range_list_delete_range_list(&new_em_range_list, (RANGE_LIST*)range_total_recved);
#endif

		if(range_list_get_total_num(&new_em_range_list) > 0)
		{
			LOG_DEBUG("vdm_period_dispatch new_em_range_list");
			out_put_range_list(&new_em_range_list);
			//set new download window
			if(   new_em_range_windows._index != ptr_vod_data_manager->_current_download_window._index || new_em_range_windows._num !=ptr_vod_data_manager->_current_download_window._num)
			{
				LOG_DEBUG("vdm_period_dispatch.debug dm_set_new_download_window(%d,%d), old_windows(%d,%d) ",new_em_range_windows._index,new_em_range_windows._num,ptr_vod_data_manager->_current_download_window._index, ptr_vod_data_manager->_current_download_window._num );
				ptr_vod_data_manager->_current_download_window = new_em_range_windows;
				range_list_cp_range_list(&new_em_range_list,  &ptr_vod_data_manager->_range_last_em_list  );
				if(P2SP_TASK_TYPE == ptr_task->_task_type)
				{
					ret = dm_set_emerge_rangelist( &((P2SP_TASK*)ptr_task)->_data_manager, &new_em_range_list);
				}
#ifdef ENABLE_BT
				else if(BT_TASK_TYPE == ptr_task->_task_type)
				{
					ret = bdm_set_emerge_rangelist(  &((BT_TASK*)ptr_task)->_data_manager, ptr_vod_data_manager->_file_index,  &new_em_range_list);
				}
#endif
				else
				{
					ret = ERR_VOD_DATA_UNKNOWN_TASK;
				}
				if(SUCCESS != ret)
				{
					LOG_DEBUG("vdm_vod_read_file .range_list_delete_range_list return = %d", ret);
					range_list_clear(&em_range_list);
					range_list_clear(&range_list_to_get);
					range_list_clear(&new_em_range_list);
					range_list_clear(&range_list_next_em_range);
					return ret;
				}
			}

			break;
		}
		else
		{
#ifdef _VOD_NO_DISK
			if(TRUE == dt_is_no_disk_task(ptr_task))
			{
				if (TRUE == ptr_vod_data_manager->_send_pause )
				{
					LOG_DEBUG("vdm_period_dispatch change pause to FALSE ");
					ptr_vod_data_manager->_send_pause = FALSE;
				}
			}
#endif
			LOG_DEBUG("vdm_period_dispatch range_list_get_total_num(&new_em_range_list) == 0 ");
			newstartpos = get_data_pos_from_index(new_em_range_windows._index)+range_to_length(&new_em_range_windows, filesize);
			newstartpos = MIN(newstartpos, filesize);
			new_em_range_windows= pos_length_to_range(newstartpos, windowsize, filesize);
			if( dt_is_vod_check_data_task(ptr_task))
			{
				//adjust range to block range
				new_em_range_windows = vdm_get_block_range_by_range(new_em_range_windows, filesize, compute_block_size(filesize));
				LOG_DEBUG("vdm_period_dispatch. adjust adjust!!  new current_download_window(%d,%d)",new_em_range_windows._index, new_em_range_windows._num);
			}
		}

	}


	/*determin whether the fetch  is timeout*/
	sd_time_ms(&now_time);
	LOG_DEBUG("vdm_period_dispatch nowtime=%lu, - lasttime=%u  , blocktime=%u", now_time, ptr_vod_data_manager->_last_fetch_time, ptr_vod_data_manager->_block_time);
	if( TIME_SUBZ(now_time,ptr_vod_data_manager->_last_fetch_time) > VDM_DEFAULT_FETCH_TIMEOUT*1000 )
	{
		LOG_DEBUG("vdm_period_dispatch nowtime=%lu, - lasttime=%d bigger than %u ", now_time, ptr_vod_data_manager->_last_fetch_time, VDM_DEFAULT_FETCH_TIMEOUT*1000);
#ifdef EMBED_REPORT
		ptr_vod_data_manager->_vod_interrupt_times++;
#endif

#ifdef _VOD_NO_DISK
		if(TRUE == dt_is_no_disk_task(ptr_task))
		{
			ptr_vod_data_manager->_send_pause = TRUE;
		}
#else
		ptr_vod_data_manager->_send_pause = TRUE;
#endif
	}

	if (TRUE == ptr_vod_data_manager->_send_pause )
	{
#ifdef _VOD_NO_DISK
		if(TRUE == dt_is_no_disk_task(ptr_task))
		{
			if(TRUE == range_list_is_include((RANGE_LIST*)&ptr_vod_data_manager->_range_buffer_list, &em_range_window))
			{
				LOG_DEBUG("vdm_period_dispatch range_list_is_include em_range_window(%d,%d) ",em_range_window._index, em_range_window._num);
				ptr_vod_data_manager->_send_pause = FALSE;
			}
		}
		else
		{
			if(TRUE == range_list_is_include((RANGE_LIST*)range_total_recved, &em_range_window))
			{
				LOG_DEBUG("vdm_period_dispatch range_list_is_include em_range_window(%d,%d) ",em_range_window._index, em_range_window._num);
				ptr_vod_data_manager->_send_pause = FALSE;
			}
		}
#else
		if(TRUE == range_list_is_include((RANGE_LIST*)range_total_recved, &em_range_window))
		{
			LOG_DEBUG("vdm_period_dispatch range_list_is_include em_range_window(%d,%d) ",em_range_window._index, em_range_window._num);
			ptr_vod_data_manager->_send_pause = FALSE;
		}
#endif

	}

	//超时
	sd_time_ms(&now_time);
	if(ptr_vod_data_manager->_block_time > 0)
	{
		if( TIME_SUBZ(now_time, ptr_vod_data_manager->_last_fetch_time) > ptr_vod_data_manager->_block_time )
		{
			LOG_DEBUG("vdm_period_dispatch nowtime=%lu, - lasttime=%d bigger than blocktime %u ", now_time, ptr_vod_data_manager->_last_fetch_time, ptr_vod_data_manager->_block_time);
			ret = ERR_VOD_DATA_FETCH_TIMEOUT;
			range_list_clear(&em_range_list);
			range_list_clear(&range_list_to_get);
			range_list_clear(&new_em_range_list);
			range_list_clear(&range_list_next_em_range);
			return ret;
		}
	}


	LOG_DEBUG("vdm_period_dispatch ptr_vod_data_manager->_send_pause=%d  ", ptr_vod_data_manager->_send_pause);
	vdm_cdn_use_dispatch(ptr_vod_data_manager);


	if( TRUE != ptr_vod_data_manager->_send_pause && TRUE == range_list_is_include(&ptr_vod_data_manager->_range_buffer_list, &range_to_get) )
	{
		/*Buffer is in the bufferlist already, just return them*/
		LOG_DEBUG("vdm_period_dispatch.debug _callback_recv_handler %llu, %llu, range_to_get(%d,%d)", ptr_vod_data_manager->_start_pos,ptr_vod_data_manager->_fetch_length, range_to_get._index,range_to_get._num);
		ret =  vdm_write_data_buffer(ptr_vod_data_manager, ptr_vod_data_manager->_start_pos,ptr_vod_data_manager->_user_buffer, ptr_vod_data_manager->_fetch_length, &range_to_get, &ptr_vod_data_manager->_buffer_list);
		CHECK_VALUE(ret);
		ret = (*(ptr_vod_data_manager->_callback_recv_handler))(SUCCESS, 0, ptr_vod_data_manager->_user_buffer, ptr_vod_data_manager->_fetch_length, ptr_vod_data_manager->_user_data);
		LOG_DEBUG("vdm_period_dispatch _callback_recv_handler ret %d ",ret);
#ifdef EMBED_REPORT
		if(range_to_get._index == 0)
		{
			sd_time(&ptr_vod_data_manager->_vod_first_buffer_time);
		}
#endif

		ptr_vod_data_manager->_is_fetching = FALSE;
		ptr_vod_data_manager->_send_pause = FALSE;
		ptr_vod_data_manager->_user_data = NULL;
		ptr_vod_data_manager->_block_time = 0;
		ptr_vod_data_manager->_bit_per_seconds = 0;
		range_list_clear(&em_range_list);
		range_list_clear(&range_list_to_get);
		range_list_clear(&new_em_range_list);
		range_list_clear(&range_list_next_em_range);
		ret = SUCCESS;
		return ret;
	}

	range_list_clear(&em_range_list);
	range_list_clear(&range_list_to_get);
	range_list_clear(&new_em_range_list);
	range_list_clear(&range_list_next_em_range);
	return ret;
}
#else
_int32  vdm_check_time_out(VOD_DATA_MANAGER* ptr_vod_data_manager)
{
	_u32   now_time;
	_int32 ret = SUCCESS;
	//超时
	sd_time_ms(&now_time);
	if (TRUE == ptr_vod_data_manager->_is_fetching && ptr_vod_data_manager->_block_time > 0)
	{
		if (TIME_SUBZ(now_time, ptr_vod_data_manager->_last_fetch_time) > ptr_vod_data_manager->_block_time)
		{
			LOG_DEBUG("vdm_period_dispatch nowtime=%u, - lasttime=%u bigger than blocktime %u", now_time, ptr_vod_data_manager->_last_fetch_time, ptr_vod_data_manager->_block_time);
			ret = ERR_VOD_DATA_FETCH_TIMEOUT;
			return ret;
		}
	}

	return ret;

}

_int32  vdm_check_task_status(VOD_DATA_MANAGER* ptr_vod_data_manager)
{
	_int32 ret = SUCCESS;
	TASK* ptr_task = (TASK*)ptr_vod_data_manager->_task_ptr;
	TASK* taskinfo = NULL;
	_u64  filesize;
	char*  file_name;


	ret = tm_get_task_by_id(ptr_task->_task_id, &taskinfo);
	if (SUCCESS != ret || taskinfo == NULL || NULL == ptr_task)
	{
		LOG_DEBUG("vdm_check_task_status .task is not found,taskid= %d ", ptr_task->_task_id);
		return ERR_VOD_DATA_TASK_STOPPED;
	}
	//判斷任務狀態
	if (TASK_S_FAILED == ptr_task->task_status)
	{
		LOG_DEBUG("vdm_check_task_status task_status is failed=%d ", ptr_task->task_status);
		ret = ERR_VOD_DATA_TASK_STOPPED;
		return ret;
	}

	if (P2SP_TASK_TYPE == ptr_task->_task_type)
	{
		filesize = dm_get_file_size(&((P2SP_TASK*)ptr_task)->_data_manager);
	}
#ifdef ENABLE_BT
	else if (BT_TASK_TYPE == ptr_task->_task_type)
	{
		filesize = bdm_get_sub_file_size(&((BT_TASK*)ptr_task)->_data_manager, ptr_vod_data_manager->_file_index);
	}
#endif
	else
	{
		LOG_DEBUG("vdm_period_dispatch .unknown task type ");
		return ERR_VOD_DATA_UNKNOWN_TASK;
	}
	if (filesize == 0)
	{
		LOG_DEBUG("vdm_period_dispatch .unknown task filesize 2");
		return ERR_VOD_DATA_UNKNOWN_TASK;
	}

	if (P2SP_TASK_TYPE == ptr_task->_task_type)
	{
		if (FALSE == dm_get_filnal_file_name(&((P2SP_TASK*)ptr_task)->_data_manager, &file_name))
		{
			LOG_DEBUG("vdm_period_dispatch .dm_get_filnal_file_name failed");
			return ret;
		}
	}
#ifdef ENABLE_BT
	else if (BT_TASK_TYPE == ptr_task->_task_type)
	{
		if (SUCCESS != bdm_get_file_name(&((BT_TASK*)ptr_task)->_data_manager, ptr_vod_data_manager->_file_index, &file_name))
		{
			LOG_DEBUG("vdm_period_dispatch .bdm_get_file_name failed");
			return ret;
		}
	}
#endif
	else
	{
		LOG_DEBUG("vdm_period_dispatch .unknown task type ");
		return ERR_VOD_DATA_UNKNOWN_TASK;
	}


#ifdef ENABLE_BT
	if (BT_TASK_TYPE == ptr_task->_task_type)
	{
		if (BT_FILE_FAILURE == bdm_get_file_status(&((BT_TASK*)ptr_task)->_data_manager, ptr_vod_data_manager->_file_index))
		{
			bdm_set_vod_mode(&((BT_TASK*)ptr_task)->_data_manager, FALSE);
			LOG_DEBUG("vdm_period_dispatch .bdm_get_file_status == BT_FILE_FAILURE");
			return ERR_VOD_DATA_TASK_STOPPED;
		}
	}
#endif

	return ret;
}

_int32  vdm_get_file_size(VOD_DATA_MANAGER* ptr_vod_data_manager, _u64* p_file_size)
{
	_int32 ret = SUCCESS;
	TASK* ptr_task = (TASK*)ptr_vod_data_manager->_task_ptr;
	TASK* taskinfo = NULL;


	ret = tm_get_task_by_id(ptr_task->_task_id, &taskinfo);
	if (SUCCESS != ret || taskinfo == NULL || NULL == ptr_task)
	{
		LOG_DEBUG("vdm_check_task_status .task is not found,taskid= %d ", ptr_task->_task_id);
		return ERR_VOD_DATA_TASK_STOPPED;
	}

	//判斷任務狀態
	if (TASK_S_FAILED == ptr_task->task_status)
	{
		LOG_DEBUG("vdm_check_task_status task_status is failed=%d ", ptr_task->task_status);
		ret = ERR_VOD_DATA_TASK_STOPPED;
		return ret;
	}

	if (P2SP_TASK_TYPE == ptr_task->_task_type)
	{
		*p_file_size = dm_get_file_size(&((P2SP_TASK*)ptr_task)->_data_manager);
	}
#ifdef ENABLE_BT
	else if (BT_TASK_TYPE == ptr_task->_task_type)
	{
		*p_file_size = bdm_get_sub_file_size(&((BT_TASK*)ptr_task)->_data_manager, ptr_vod_data_manager->_file_index);
	}
#endif
	else
	{
		LOG_DEBUG("vdm_period_dispatch .unknown task type ");
		return ERR_VOD_DATA_UNKNOWN_TASK;
	}
	if (*p_file_size == 0)
	{
		LOG_DEBUG("vdm_period_dispatch .unknown task filesize 2");
		return ERR_VOD_DATA_UNKNOWN_TASK;
	}

	return ret;
}

_int32  vdm_get_file_name(VOD_DATA_MANAGER* ptr_vod_data_manager, char** p_file_name)
{
	_int32 ret = SUCCESS;
	TASK* ptr_task = (TASK*)ptr_vod_data_manager->_task_ptr;
	TASK* taskinfo = NULL;


	ret = tm_get_task_by_id(ptr_task->_task_id, &taskinfo);
	if (SUCCESS != ret || taskinfo == NULL || NULL == ptr_task)
	{
		LOG_DEBUG("vdm_check_task_status .task is not found,taskid= %d ", ptr_task->_task_id);
		return ERR_VOD_DATA_TASK_STOPPED;
	}

	//判斷任務狀態
	if (TASK_S_FAILED == ptr_task->task_status)
	{
		LOG_DEBUG("vdm_check_task_status task_status is failed=%d ", ptr_task->task_status);
		ret = ERR_VOD_DATA_TASK_STOPPED;
		return ret;
	}

	if (P2SP_TASK_TYPE == ptr_task->_task_type)
	{
		if (FALSE == dm_get_filnal_file_name(&((P2SP_TASK*)ptr_task)->_data_manager, p_file_name))
		{
			LOG_DEBUG("vdm_period_dispatch .dm_get_filnal_file_name failed");
			return ret;
		}
	}
#ifdef ENABLE_BT
	else if (BT_TASK_TYPE == ptr_task->_task_type)
	{
		if (SUCCESS != bdm_get_file_name(&((BT_TASK*)ptr_task)->_data_manager, ptr_vod_data_manager->_file_index, p_file_name))
		{
			LOG_DEBUG("vdm_period_dispatch .bdm_get_file_name failed");
			return ret;
		}
	}
#endif
	else
	{
		LOG_DEBUG("vdm_period_dispatch .unknown task type ");
		return ERR_VOD_DATA_UNKNOWN_TASK;
	}
	return ret;

}

_int32  vdm_get_bytes_per_second(VOD_DATA_MANAGER* ptr_vod_data_manager, _u32*  bytes_per_second)
{
	*bytes_per_second = vdm_cal_bytes_per_second(0, ptr_vod_data_manager->_bit_per_seconds);
	return SUCCESS;
}

_int32  vdm_get_em_range_by_continue_pos(VOD_DATA_MANAGER* ptr_vod_data_manager, _u64  continuing_end_pos, _u64 filesize, _u32 bytes_per_seconds, _u32 speed, RANGE* p_ret_range)
{
	_u32   block_size = 0;
	_u32   block_no = 0;
	_u64  windowsize;
#ifdef _VOD_NO_DISK
	_int32  buffersize = 0;
#endif /* _VOD_NO_DISK */
	RANGE em_range_window;
	BOOL bCheckData = FALSE;

	if ((continuing_end_pos - ptr_vod_data_manager->_current_play_pos) < bytes_per_seconds*VDM_DRAG_WINDOW_TIME_LEN)
	{
		windowsize = bytes_per_seconds*VDM_EM_WINDOW_TIME_LEN;
	}
	else if ((continuing_end_pos - ptr_vod_data_manager->_current_play_pos) < bytes_per_seconds*vdm_get_buffer_time_len())
	{
		windowsize = bytes_per_seconds*VDM_EM_WINDOW_TIME_LEN;
	}
	else if ((continuing_end_pos - ptr_vod_data_manager->_current_play_pos) > bytes_per_seconds*VDM_EM_WINDOW_TIME_LEN)
	{
		if (continuing_end_pos >= (get_data_pos_from_index(ptr_vod_data_manager->_current_buffering_window._index) + range_to_length(&ptr_vod_data_manager->_current_buffering_window, filesize)))
		{
			windowsize = 0;
		}
		else
		{
			windowsize = MIN(bytes_per_seconds*VDM_EM_WINDOW_TIME_LEN, (get_data_pos_from_index(ptr_vod_data_manager->_current_buffering_window._index) + range_to_length(&ptr_vod_data_manager->_current_buffering_window, filesize)) - continuing_end_pos);
		}
	}
	else
	{
		//这里还需要做调整
		windowsize = bytes_per_seconds*VDM_EM_WINDOW_TIME_LEN;
	}
	// [TEST] vod点播性能测试，调大优先块窗口大小
	windowsize *= 4;

#ifdef _VOD_NO_DISK
	/* 无盘点播任务的紧急块窗口大小不大于vod buffer size */
	if (TRUE == dt_is_no_disk_task(ptr_vod_data_manager->_task_ptr))
	{
		vdm_get_vod_buffer_size(&buffersize);
		windowsize = MIN(windowsize, buffersize);
	}
#endif /* _VOD_NO_DISK */
	bCheckData = dt_is_vod_check_data_task((TASK*)(ptr_vod_data_manager->_task_ptr));
	if (TRUE == bCheckData)
	{
		//adjust for data check
		block_size = compute_block_size(filesize);
		block_no = continuing_end_pos / (block_size);
		windowsize = MAX(windowsize, block_size);
		em_range_window = pos_length_to_range((_u64)block_no*block_size, windowsize, filesize);
	}
	else
	{
		em_range_window = pos_length_to_range(continuing_end_pos, windowsize, filesize);
	}


	*p_ret_range = em_range_window;
	LOG_DEBUG("vdm_get_em_range_by_continue_pos em_range_window(%d, %d), windowsize=%llu", em_range_window._index, em_range_window._num, windowsize);
	return SUCCESS;

}

_int32  vdm_on_first_buffering(VOD_DATA_MANAGER* ptr_vod_data_manager)
{
	if (ptr_vod_data_manager->_send_pause)
	{
		return SUCCESS;
	}
	_u32 now_time = 0;
	sd_time(&now_time);
	LOG_DEBUG("vdm_on_first_buffering . set send_pause to TRUE, time=%d", now_time);
#ifndef IPAD_KANKAN
	//ptr_vod_data_manager->_send_pause = TRUE;
#endif
#ifdef EMBED_REPORT
	ptr_vod_data_manager->_vod_last_time_type = VOD_LAST_FIRST_BUFFERING;
	ptr_vod_data_manager->_vod_last_time = now_time;
	if (ptr_vod_data_manager->_vod_play_begin_time == 0)
	{
		ptr_vod_data_manager->_vod_play_begin_time = now_time;
	}
#endif
	return SUCCESS;
}

_int32  vdm_on_play_interrupt(VOD_DATA_MANAGER* ptr_vod_data_manager)
{
	_u32   now_time;


	sd_time_ms(&now_time);
	LOG_ERROR("vdm_on_play_interrupt nowtime=%lu, - lasttime=%d > %u ", now_time, ptr_vod_data_manager->_last_fetch_time_for_same_pos, VDM_DEFAULT_FETCH_TIMEOUT * 1000);
#ifdef EMBED_REPORT
	if (ptr_vod_data_manager->_vod_last_time_type == VOD_LAST_INTERRUPT)
	{
	}
	else if (ptr_vod_data_manager->_vod_last_time_type == VOD_LAST_FIRST_BUFFERING)
	{
	}
	else if (ptr_vod_data_manager->_vod_last_time_type == VOD_LAST_DRAG)
	{
	}
	else if (ptr_vod_data_manager->_vod_last_time_type == VOD_LAST_NONE)
	{
		ptr_vod_data_manager->_vod_play_interrupt_times++;
		ptr_vod_data_manager->_vod_last_time_type = VOD_LAST_INTERRUPT;
		sd_time(&ptr_vod_data_manager->_vod_last_time);
	}
#endif

	LOG_DEBUG("vdm_on_play_interrupt . set send_pause to TRUE");
#ifdef _VOD_NO_DISK
	ptr_vod_data_manager->_send_pause = TRUE;
#else
	ptr_vod_data_manager->_send_pause = TRUE;
#endif
	return SUCCESS;
}

_int32  vdm_on_play_drag(VOD_DATA_MANAGER* ptr_vod_data_manager)
{
	_u32 now_time;

	sd_time(&now_time);
	LOG_DEBUG("vdm_on_play_drag . set send_pause to TRUE, time=%d", now_time);
	ptr_vod_data_manager->_send_pause = TRUE;
#ifdef EMBED_REPORT
	ptr_vod_data_manager->_vod_play_drag_num++;
	ptr_vod_data_manager->_vod_last_time_type = VOD_LAST_DRAG;
	if (ptr_vod_data_manager->_vod_last_time_type == VOD_LAST_NONE)
	{
		sd_time(&ptr_vod_data_manager->_vod_last_time);
	}
	else if (ptr_vod_data_manager->_vod_last_time_type == VOD_LAST_INTERRUPT)
	{
		ptr_vod_data_manager->_vod_play_total_buffer_time_len += TIME_SUBZ(now_time, ptr_vod_data_manager->_vod_last_time);
		sd_time(&ptr_vod_data_manager->_vod_last_time);
	}

#endif
	return SUCCESS;
}

_int32  vdm_on_play_resume(VOD_DATA_MANAGER* ptr_vod_data_manager)
{
	_u32 now_time;
	_u32 time_len = 0;
	sd_time(&now_time);
#ifdef EMBED_REPORT
	time_len = TIME_SUBZ(now_time, ptr_vod_data_manager->_vod_last_time);
#endif
	LOG_DEBUG("vdm_on_play_resume . set send_pause to FALSE, time=%d", now_time);
	ptr_vod_data_manager->_send_pause = FALSE;
#ifdef EMBED_REPORT
	switch (ptr_vod_data_manager->_vod_last_time_type)
	{
	case VOD_LAST_FIRST_BUFFERING:
		ptr_vod_data_manager->_vod_first_buffer_time_len = time_len;
		break;
	case VOD_LAST_INTERRUPT:
		ptr_vod_data_manager->_vod_play_total_buffer_time_len += time_len;
		ptr_vod_data_manager->_vod_play_max_buffer_time_len = MAX(ptr_vod_data_manager->_vod_play_max_buffer_time_len, time_len);
		ptr_vod_data_manager->_vod_play_min_buffer_time_len = (ptr_vod_data_manager->_vod_play_min_buffer_time_len == 0) ? time_len : ptr_vod_data_manager->_vod_play_min_buffer_time_len;
		ptr_vod_data_manager->_vod_play_min_buffer_time_len = MIN(ptr_vod_data_manager->_vod_play_min_buffer_time_len, time_len);
		if (time_len < 60)
		{
			ptr_vod_data_manager->_play_interrupt_1++;
		}
		else if (time_len < 120 && time_len >= 60)
		{
			ptr_vod_data_manager->_play_interrupt_2++;
		}
		else if (time_len < 360 && time_len >= 120)
		{
			ptr_vod_data_manager->_play_interrupt_3++;
		}
		else if (time_len < 600 && time_len >= 360)
		{
			ptr_vod_data_manager->_play_interrupt_4++;
		}
		else if (time_len < 900 && time_len >= 600)
		{
			ptr_vod_data_manager->_play_interrupt_5++;
		}
		else if (time_len >= 800)
		{
			ptr_vod_data_manager->_play_interrupt_6++;
		}

		break;
	case VOD_LAST_DRAG:
		ptr_vod_data_manager->_vod_play_total_drag_wait_time += time_len;
		ptr_vod_data_manager->_vod_play_max_drag_wait_time = MAX(ptr_vod_data_manager->_vod_play_max_drag_wait_time, time_len);
		ptr_vod_data_manager->_vod_play_min_drag_wait_time = (ptr_vod_data_manager->_vod_play_min_drag_wait_time == 0) ? time_len : ptr_vod_data_manager->_vod_play_min_drag_wait_time;
		ptr_vod_data_manager->_vod_play_min_drag_wait_time = MIN(ptr_vod_data_manager->_vod_play_min_drag_wait_time, time_len);
		break;
	default:
		break;
	}
	ptr_vod_data_manager->_vod_last_time_type = VOD_LAST_NONE;
#endif
	sd_time_ms(&ptr_vod_data_manager->_last_begin_play_time);
	ptr_vod_data_manager->_last_read_pos = ptr_vod_data_manager->_current_play_pos;
	return SUCCESS;
}

_int32 vdm_get_buffering_windows_size(VOD_DATA_MANAGER* ptr_vod_data_manager, _u64 file_size)
{
	_int32 ret = 0, buffering_windows_size = 0;
	_u32 bytes_per_seconds = 0;
	_int32 index_range_num = 0;
	ret = vdm_get_vod_buffer_size(&buffering_windows_size);
	CHECK_VALUE(ret);
	buffering_windows_size -= get_data_unit_size() * 10; //预留10个range，防止vdm malloc buffer用尽
	bytes_per_seconds = vdm_cal_bytes_per_second(file_size, ptr_vod_data_manager->_bit_per_seconds);
	buffering_windows_size = MIN(bytes_per_seconds*VDM_BUFFERING_WINDOW_TIME_LEN, buffering_windows_size);
#ifdef KANKAN_PROJ
	/* 有盘点播也会因为5M 的vod内存空间所限制，需预留给索引空间(远程RMVB 任务可能接近2M) */
	index_range_num = range_list_get_total_num(&ptr_vod_data_manager->_range_index_list);
	buffering_windows_size -= index_range_num * get_data_unit_size();
#else
	buffering_windows_size -= get_data_unit_size(); //预留给索引空间
#endif
	LOG_DEBUG("%%%% %d-%d  %d, %d", bytes_per_seconds, VDM_BUFFERING_WINDOW_TIME_LEN, buffering_windows_size, VOD_MEM_BUFFER);
	return buffering_windows_size;
}

//2009-08-12 version
_int32 	vdm_period_dispatch(VOD_DATA_MANAGER* ptr_vod_data_manager, BOOL b_force_set_em)
{
	RANGE_LIST em_range_list;
	RANGE_LIST range_list_to_get;
	RANGE_LIST range_list_tmp;
	RANGE_LIST priority_range_list;
	RANGE         range_tmp;
	const RANGE_LIST* range_total_recved;
	RANGE_LIST range_list_null;
	_int32 ret;
	_u64  filesize;
	RANGE  range_to_get;
	RANGE  em_range_window;
	RANGE  new_em_range_windows;
	_u32 bytes_per_seconds;
	char*  file_name;
	_u32   now_time;
	_u64  continuing_end_pos;
	_int32   buffering_windows_size;
	BOOL priority_reset = FALSE;
	BOOL   to_get_index = FALSE;
	BOOL   to_move_down_win = FALSE;
	/*get DOWNLOAD_TASK ptr here*/
	TASK* ptr_task = (TASK*)ptr_vod_data_manager->_task_ptr;
	TASK* taskinfo = NULL;
	BOOL bCheckData = FALSE;
	_u32   block_size;
	_u32   block_no;
	_u64   current_play_pos;

	LOG_DEBUG("vdm_period_dispatch .taskid=%d...", ptr_task->_task_id);
	ret = tm_get_task_by_id(ptr_task->_task_id, &taskinfo);
	if (SUCCESS != ret || taskinfo == NULL)
	{
		LOG_DEBUG("vdm_period_dispatch .task is not found,taskid= %d ", ptr_task->_task_id);
		return ERR_VOD_DATA_TASK_STOPPED;
	}

	range_total_recved = NULL;
	ret = SUCCESS;

	/*暂时修改，对于纠错强制设置紧急窗口情况必须保证能设置上 ，所以在设置之前的所有返回情况都有可能产生问题。*/
	if (b_force_set_em == FALSE)
	{
		ret = vdm_check_time_out(ptr_vod_data_manager);
		if (SUCCESS != ret)
			return ret;

	}

	ret = vdm_check_task_status(ptr_vod_data_manager);
	if (SUCCESS != ret)
		return ret;

	ret = vdm_get_file_size(ptr_vod_data_manager, &filesize);
	CHECK_VALUE(ret);

	ret = vdm_get_file_name(ptr_vod_data_manager, &file_name);
	CHECK_VALUE(ret);

	//ret = vdm_get_vod_buffer_size(&buffering_windows_size);
	//CHECK_VALUE(ret);
	//buffering_windows_size -= get_data_unit_size(); //预留给索引空间
	buffering_windows_size = vdm_get_buffering_windows_size(ptr_vod_data_manager, filesize);

	LOG_DEBUG("vdm_period_dispatch.debug taskid=%u, fileindex=%u, start_pos=%llu, fetch_length=%llu, filesize=%llu,blocktime=%u ", ptr_task->_task_id, ptr_vod_data_manager->_file_index, ptr_vod_data_manager->_start_pos, ptr_vod_data_manager->_fetch_length, filesize, ptr_vod_data_manager->_block_time);

	range_to_get = pos_length_to_range(ptr_vod_data_manager->_start_pos, ptr_vod_data_manager->_fetch_length, filesize);


	ret = vdm_get_bytes_per_second(ptr_vod_data_manager, &bytes_per_seconds);
	CHECK_VALUE(ret);


	bCheckData = dt_is_vod_check_data_task((TASK*)(ptr_vod_data_manager->_task_ptr));
	if (TRUE == bCheckData)
	{
		//adjust for data check
		block_size = compute_block_size(filesize);
		block_no = (ptr_vod_data_manager->_current_play_pos) / (block_size);
		current_play_pos = (_u64)block_no*block_size;
	}
	else
	{
		current_play_pos = ptr_vod_data_manager->_current_play_pos;
	}

	ret = vdm_get_continuing_end_pos(current_play_pos, filesize, &ptr_vod_data_manager->_range_buffer_list, &continuing_end_pos);
	CHECK_VALUE(ret);

	if (ptr_vod_data_manager->_current_download_window._num > 0)
	{
		ptr_vod_data_manager->_current_buffering_window = pos_length_to_range(MIN(current_play_pos, get_data_pos_from_index(ptr_vod_data_manager->_current_download_window._index)), buffering_windows_size, filesize);
	}
	else
	{
		ptr_vod_data_manager->_current_buffering_window = pos_length_to_range(current_play_pos, buffering_windows_size, filesize);
	}

	ret = vdm_get_em_range_by_continue_pos(ptr_vod_data_manager, continuing_end_pos, filesize, bytes_per_seconds, taskinfo->speed, &em_range_window);
	CHECK_VALUE(ret);

	//for if em_range_window bigger than current_buffering_window
	//em_range_window = relevant_range(&ptr_vod_data_manager->_current_buffering_window, &em_range_window);

	LOG_DEBUG("vdm_period_dispatch range_to_get(%d, %d), em_range_window(%d, %d), buffering_window(%d, %d), bytes/sec=%d, 2 bytes/sec=%u ", range_to_get._index, range_to_get._num, em_range_window._index, em_range_window._num, ptr_vod_data_manager->_current_buffering_window._index, ptr_vod_data_manager->_current_buffering_window._num, bytes_per_seconds, ptr_vod_data_manager->_bit_per_seconds);
	range_list_init(&em_range_list);
	range_list_init(&range_list_to_get);
	range_list_init(&range_list_null);
	range_list_add_range(&range_list_to_get, &range_to_get, NULL, NULL);
	range_list_add_range(&em_range_list, &em_range_window, NULL, NULL);
	/*add extra range to em_range_list again, by bitrates*/


	/* sub the range list first*/
	ret = range_list_delete_range_list(&range_list_to_get, &ptr_vod_data_manager->_range_buffer_list);
	if (SUCCESS != ret)
	{
		LOG_DEBUG("vdm_vod_read_file .range_list_delete_range_list return = %d", ret);
		range_list_clear(&em_range_list);
		range_list_clear(&range_list_to_get);
		range_list_clear(&range_list_null);
		return ret;
	}



	/*if the first block has received, parse the head to get index range*/
	ret = vdm_process_index_parser(ptr_vod_data_manager, filesize, file_name);
	LOG_DEBUG("vdm_vod_read_file .vdm_process_index_parser return = %d", ret);
	range_list_init(&range_list_tmp);
	range_list_add_range_list(&range_list_tmp, &ptr_vod_data_manager->_range_index_list);
	range_list_delete_range_list(&range_list_tmp, &ptr_vod_data_manager->_range_buffer_list);
	if (range_list_get_total_num(&range_list_tmp) > 0)
	{
		ret = vdm_get_dm_buffer_to_buffer_list(ptr_vod_data_manager, &range_list_tmp);
	}
	range_list_clear(&range_list_tmp);



	/*determinate the window of buffer, and free the buffer not in the window*/
	/*and if the buffer is in the index range window, so prior to keep it */
#ifdef _VOD_NO_DISK
	if(TRUE == dt_is_no_disk_task(ptr_task))
	{
		range_list_init(&range_list_tmp);
		range_list_add_range(&range_list_tmp, &ptr_vod_data_manager->_current_buffering_window, NULL, NULL);
		range_list_add_range_list(&range_list_tmp, &ptr_vod_data_manager->_range_index_list);
		ret = vdm_drop_buffer_not_in_emergency_window(ptr_vod_data_manager, &range_list_tmp);
		if(SUCCESS != ret)
		{
			LOG_DEBUG("vdm_drop_buffer_not_in_emergency_window return = %d", ret);
			range_list_clear(&em_range_list);
			range_list_clear(&range_list_to_get);
			range_list_clear(&range_list_null);
			range_list_clear(&range_list_tmp);
			return ret;
		}
		if(P2SP_TASK_TYPE == ptr_task->_task_type)
		{
			ret = dm_drop_buffer_not_in_emerge_windows( &((P2SP_TASK*)ptr_task)->_data_manager, &range_list_tmp);
		}
#ifdef ENABLE_BT
		else if(BT_TASK_TYPE == ptr_task->_task_type)
		{
			//ret = bdm_get_range_data_buffer( &((BT_TASK*)ptr_task)->_data_manager, ptr_vod_data_manager->_file_index , range,  &ret_range_buffer);
		}
#endif
		range_list_clear(&range_list_tmp);
	}
	else
	{
		//have disk
		range_list_init(&range_list_tmp);
		range_list_add_range(&range_list_tmp, &ptr_vod_data_manager->_current_buffering_window, NULL, NULL);
		range_list_add_range_list(&range_list_tmp, &ptr_vod_data_manager->_range_index_list);
		ret = vdm_drop_buffer_not_in_emergency_window(ptr_vod_data_manager, &range_list_tmp);
		if(SUCCESS != ret)
		{
			LOG_DEBUG("vdm_drop_buffer_not_in_emergency_window return = %d", ret);
			range_list_clear(&em_range_list);
			range_list_clear(&range_list_to_get);
			range_list_clear(&range_list_null);
			range_list_clear(&range_list_tmp);
			return ret;
		}
		range_list_clear(&range_list_tmp);
	}


#else
	range_list_init(&range_list_tmp);
	range_list_add_range(&range_list_tmp, &ptr_vod_data_manager->_current_buffering_window, NULL, NULL);
	range_list_add_range_list(&range_list_tmp, &ptr_vod_data_manager->_range_index_list);
	ret = vdm_drop_buffer_not_in_emergency_window(ptr_vod_data_manager, &range_list_tmp);
	if (SUCCESS != ret)
	{
		LOG_DEBUG("vdm_drop_buffer_not_in_emergency_window return = %d", ret);
		range_list_clear(&em_range_list);
		range_list_clear(&range_list_to_get);
		range_list_clear(&range_list_tmp);
		range_list_clear(&range_list_null);
		return ret;
	}
	range_list_clear(&range_list_tmp);
#endif

	range_list_init(&range_list_tmp);
	range_list_add_range(&range_list_tmp, &ptr_vod_data_manager->_current_buffering_window, NULL, NULL);
	range_list_add_range_list(&range_list_tmp, &ptr_vod_data_manager->_range_index_list);
	range_list_delete_range_list(&range_list_tmp, &ptr_vod_data_manager->_range_buffer_list);
	range_list_delete_range_list(&range_list_tmp, &ptr_vod_data_manager->_range_waiting_list);
	ret = vdm_get_dm_buffer_to_buffer_list(ptr_vod_data_manager, &range_list_tmp);
	range_list_clear(&range_list_tmp);


	range_list_init(&range_list_tmp);
	range_list_add_range(&range_list_tmp, &ptr_vod_data_manager->_current_buffering_window, NULL, NULL);
	range_list_add_range_list(&range_list_tmp, &ptr_vod_data_manager->_range_index_list);
	range_list_delete_range_list(&range_list_tmp, &ptr_vod_data_manager->_range_buffer_list);
	range_list_delete_range_list(&range_list_tmp, &ptr_vod_data_manager->_range_waiting_list);

#ifdef _VOD_NO_DISK
	if(TRUE != dt_is_no_disk_task(ptr_task))
	{
		ret = vdm_alloc_buffer_for_waiting_list(ptr_vod_data_manager, &range_list_tmp,  filesize);
		if(SUCCESS != ret)
		{
			LOG_DEBUG("vdm_alloc_buffer_for_waiting_list return = %d", ret);
			range_list_clear(&range_list_tmp);
			range_list_clear(&em_range_list);
			range_list_clear(&range_list_to_get);
			range_list_clear(&range_list_null);
			return ret;
		}
	}

#else
	ret = vdm_alloc_buffer_for_waiting_list(ptr_vod_data_manager, &range_list_tmp, filesize);
	if (SUCCESS != ret)
	{
		LOG_DEBUG("vdm_alloc_buffer_for_waiting_list return = %d", ret);
		range_list_clear(&range_list_tmp);
		range_list_clear(&em_range_list);
		range_list_clear(&range_list_to_get);
		range_list_clear(&range_list_null);
		return ret;
	}
#endif
	range_list_clear(&range_list_tmp);


	if (P2SP_TASK_TYPE == ptr_vod_data_manager->_task_ptr->_task_type)
	{
		range_total_recved = dm_get_recved_range_list(&((P2SP_TASK*)ptr_vod_data_manager->_task_ptr)->_data_manager);
	}
#ifdef ENABLE_BT
	else if (BT_TASK_TYPE == ptr_vod_data_manager->_task_ptr->_task_type)
	{

		range_total_recved = bdm_get_resv_range_list(&((BT_TASK*)ptr_vod_data_manager->_task_ptr)->_data_manager, ptr_vod_data_manager->_file_index);
		if (NULL == range_total_recved)
		{
			range_total_recved = &range_list_null;
		}

	}
#endif

	if (NULL == range_total_recved)
	{
		LOG_DEBUG("vdm_vod_read_file .unknown task type ");
		return ERR_VOD_DATA_UNKNOWN_TASK;
	}

	LOG_DEBUG("vdm_vod_read_file . range_to_get (%d,%d)", range_to_get._index, range_to_get._num);
	LOG_DEBUG("vdm_vod_read_file . the last em_range_list is :");
	out_put_range_list(&ptr_vod_data_manager->_range_last_em_list);
	LOG_DEBUG("vdm_vod_read_file . the em_range_list is :");
	out_put_range_list(&em_range_list);
	LOG_DEBUG("vdm_vod_read_file . total_recved_range_list is :");
	out_put_range_list((RANGE_LIST*)range_total_recved);
	LOG_DEBUG("vdm_vod_read_file . >_range_buffer_list is :");
	out_put_range_list((RANGE_LIST*)&ptr_vod_data_manager->_range_buffer_list);
	LOG_ERROR("debug . current_download_window is (%d,%d), current_buffering_window is (%d,%d), cur_play_pos %llu, continuing_end_pos %llu", ptr_vod_data_manager->_current_download_window._index, ptr_vod_data_manager->_current_download_window._num, ptr_vod_data_manager->_current_buffering_window._index, ptr_vod_data_manager->_current_buffering_window._num, ptr_vod_data_manager->_current_play_pos, continuing_end_pos);
#ifdef VOD_DEBUG
	printf("debug . current_download_window is (%d,%d), current_buffering_window is (%d,%d), cur_play_pos %llu, continuing_end_pos %llu", ptr_vod_data_manager->_current_download_window._index, ptr_vod_data_manager->_current_download_window._num, ptr_vod_data_manager->_current_buffering_window._index, ptr_vod_data_manager->_current_buffering_window._num,ptr_vod_data_manager->_current_play_pos, continuing_end_pos);
#endif


	//new_em_range_windows = vdm_get_block_range_by_range(em_range_window, filesize, compute_block_size(filesize));
	new_em_range_windows = em_range_window;

	to_get_index = FALSE;
	if (range_list_get_total_num(&ptr_vod_data_manager->_range_index_list) > 0)
	{
		if ((FALSE == range_list_is_contained2(&ptr_vod_data_manager->_range_buffer_list, &ptr_vod_data_manager->_range_index_list))
			&& (FALSE == range_list_is_contained2(&ptr_vod_data_manager->_range_last_em_list, &ptr_vod_data_manager->_range_index_list)))
		{
			to_get_index = TRUE;
		}
	}


	range_tmp = relevant_range(&em_range_window, &ptr_vod_data_manager->_current_download_window);
	range_list_init(&range_list_tmp);
	range_list_add_range(&range_list_tmp, &range_tmp, NULL, NULL);
	range_list_delete_range_list(&range_list_tmp, &ptr_vod_data_manager->_range_buffer_list);
	LOG_ERROR("vdm_period_dispatch .new_em_range_windows(%d,%d),  em-cur_down=%d, em/2 =%d, down_left(%d) ", new_em_range_windows._index, new_em_range_windows._num, (em_range_window._index - (ptr_vod_data_manager->_current_download_window._index + ptr_vod_data_manager->_current_download_window._num / 2)), (em_range_window._num * 4 / 8), range_list_get_total_num(&range_list_tmp));
	//如果当前下载窗口只有一半在紧急窗口内,则移动下载窗口
	if (em_range_window._num > 0)
	{
		if ((em_range_window._index >= (ptr_vod_data_manager->_current_download_window._index + ptr_vod_data_manager->_current_download_window._num*VOD_DATA_MANAGER_DOWN_WINDOW_TO_MOVE_NUM / 10))
			|| (range_list_get_total_num(&range_list_tmp) < (ptr_vod_data_manager->_current_download_window._num*MIN_VOD_DATA_MANAGER_DOWN_WINDOW_NUM / 10)))
		{
			if (ptr_vod_data_manager->_current_download_window._num == 0)
			{
				to_move_down_win = TRUE;
			}
			else if (TRUE == range_list_is_contained2(&ptr_vod_data_manager->_range_buffer_list, &ptr_vod_data_manager->_range_index_list))
			{
				to_move_down_win = TRUE;
			}
		}
	}
	if ((TRUE == to_move_down_win)
		|| (em_range_window._index < ptr_vod_data_manager->_current_download_window._index)
		|| (TRUE == to_get_index)
		|| (TRUE == b_force_set_em)
		|| (FALSE == range_list_is_include(&ptr_vod_data_manager->_range_buffer_list, &range_to_get)))
	{
		range_list_clear(&range_list_tmp);
		//拖動 或者首緩沖
		//ptr_vod_data_manager->_send_pause = TRUE;
		//LOG_DEBUG("vdm_period_dispatch . set send_pause to TRUE");

		range_list_init(&range_list_tmp);
		range_list_add_range(&range_list_tmp, &new_em_range_windows, NULL, NULL);
		range_list_add_range_list(&range_list_tmp, &ptr_vod_data_manager->_range_index_list);
		LOG_ERROR("vdm_period_dispatch . the new_em_range_list total num=%d ,  _range_last_em_list totalnum=%d:", range_list_get_total_num(&range_list_tmp), range_list_get_total_num(&ptr_vod_data_manager->_range_last_em_list));
#ifdef _VOD_NO_DISK
		if(TRUE == dt_is_no_disk_task(ptr_task))
		{
			range_list_delete_range_list(&range_list_tmp, &ptr_vod_data_manager->_range_buffer_list);
		}
		else
		{
			range_list_delete_range_list(&range_list_tmp, &ptr_vod_data_manager->_range_buffer_list);
		}
#else
		range_list_delete_range_list(&range_list_tmp, &ptr_vod_data_manager->_range_buffer_list);
#endif

		LOG_DEBUG("vdm_period_dispatch new_em_range_list");
		out_put_range_list(&range_list_tmp);
		//set new download window
		range_list_init(&priority_range_list);
		dm_get_priority_range(&((P2SP_TASK*)ptr_task)->_data_manager, &priority_range_list);
		if (priority_range_list._node_size == 0)
		{
			priority_reset = TRUE;
		}

		if ((new_em_range_windows._index != ptr_vod_data_manager->_current_download_window._index || new_em_range_windows._num != ptr_vod_data_manager->_current_download_window._num) || TRUE == b_force_set_em || TRUE == priority_reset)
		{
			LOG_DEBUG("vdm_period_dispatch.debug dm_set_new_download_window(%d,%d), old_windows(%d,%d) ", new_em_range_windows._index, new_em_range_windows._num, ptr_vod_data_manager->_current_download_window._index, ptr_vod_data_manager->_current_download_window._num);
			ptr_vod_data_manager->_current_download_window = new_em_range_windows;
			//range_list_add_range(&ptr_vod_data_manager->_range_last_em_list, &ptr_vod_data_manager->_current_download_window, NULL, NULL);
			//range_list_cp_range_list(&range_list_tmp,  &ptr_vod_data_manager->_range_last_em_list  );
			if (P2SP_TASK_TYPE == ptr_task->_task_type)
			{
				ret = dm_set_emerge_rangelist(&((P2SP_TASK*)ptr_task)->_data_manager, &range_list_tmp);
			}
#ifdef ENABLE_BT
			else if (BT_TASK_TYPE == ptr_task->_task_type)
			{
				ret = bdm_set_emerge_rangelist(&((BT_TASK*)ptr_task)->_data_manager, ptr_vod_data_manager->_file_index, &range_list_tmp);
			}
#endif
			else
			{
				ret = ERR_VOD_DATA_UNKNOWN_TASK;
			}
			if (SUCCESS != ret)
			{
				LOG_DEBUG("vdm_vod_read_file .range_list_delete_range_list return = %d", ret);
				range_list_clear(&em_range_list);
				range_list_clear(&range_list_to_get);
				range_list_clear(&range_list_tmp);
				return ret;
			}
			priority_reset = FALSE;
		}
		range_list_clear(&range_list_tmp);
	}
	else
	{
		range_list_clear(&range_list_tmp);
	}



	/*determin whether the fetch  is timeout*/
	sd_time_ms(&now_time);
	LOG_DEBUG("vdm_period_dispatch nowtime=%lu, - lasttime=%u  , blocktime=%u", now_time, ptr_vod_data_manager->_last_fetch_time_for_same_pos, ptr_vod_data_manager->_block_time);
	if (TRUE == ptr_vod_data_manager->_is_fetching  && TIME_SUBZ(now_time, ptr_vod_data_manager->_last_fetch_time_for_same_pos) > VDM_DEFAULT_FETCH_TIMEOUT * 1000)
	{
		vdm_on_play_interrupt(ptr_vod_data_manager);
	}

	if (TRUE == ptr_vod_data_manager->_is_fetching)
	{
		LOG_DEBUG("vdm_period_dispatch _current_play_pos %lu- _last_read_pos:%lu = %lu  timelen %u ", ptr_vod_data_manager->_current_play_pos > ptr_vod_data_manager->_last_read_pos, TIME_SUBZ(now_time, ptr_vod_data_manager->_last_begin_play_time));
		if (ptr_vod_data_manager->_current_play_pos > ptr_vod_data_manager->_last_read_pos)
		{
			if ((ptr_vod_data_manager->_current_play_pos - ptr_vod_data_manager->_last_read_pos)
				< ((TIME_SUBZ(now_time, ptr_vod_data_manager->_last_begin_play_time) / 1000)* bytes_per_seconds))
			{
				//                                      vdm_on_play_interrupt(ptr_vod_data_manager);
			}
		}
	}

	if (TRUE == ptr_vod_data_manager->_is_fetching  && TRUE == ptr_vod_data_manager->_send_pause)
	{
#ifdef _VOD_NO_DISK
		LOG_DEBUG("vdm_period_dispatch continuing_end_pos %lu- start_pos:%lu = %lu  .vs. window size %u ",continuing_end_pos, ptr_vod_data_manager->_start_pos ,continuing_end_pos- ptr_vod_data_manager->_start_pos,  bytes_per_seconds*vdm_get_buffer_time_len());
		if((ptr_vod_data_manager->_start_pos == 0) && (range_list_get_total_num(&ptr_vod_data_manager->_range_index_list) > 0)
			&& (TRUE == range_list_is_contained2(&ptr_vod_data_manager->_range_buffer_list, &ptr_vod_data_manager->_range_index_list)))
		{
			vdm_on_play_resume(ptr_vod_data_manager);
		}
		else if( ((continuing_end_pos - ptr_vod_data_manager->_start_pos) >= (bytes_per_seconds*VDM_DRAG_WINDOW_TIME_LEN)) || continuing_end_pos == filesize)
		{
			vdm_on_play_resume(ptr_vod_data_manager);
		}
#else
		LOG_DEBUG("vdm_period_dispatch continuing_end_pos %lu- start_pos:%lu = %lu  .vs. window size %u ", continuing_end_pos, ptr_vod_data_manager->_start_pos, continuing_end_pos - ptr_vod_data_manager->_start_pos, bytes_per_seconds*vdm_get_buffer_time_len());
		if ((ptr_vod_data_manager->_start_pos == 0) && (range_list_get_total_num(&ptr_vod_data_manager->_range_index_list) > 0)
			&& (TRUE == range_list_is_contained2(&ptr_vod_data_manager->_range_buffer_list, &ptr_vod_data_manager->_range_index_list)))
		{
			vdm_on_play_resume(ptr_vod_data_manager);
		}
		else if (((continuing_end_pos - ptr_vod_data_manager->_start_pos) >= (bytes_per_seconds*VDM_DRAG_WINDOW_TIME_LEN)) || continuing_end_pos == filesize)
		{
			vdm_on_play_resume(ptr_vod_data_manager);
		}
#endif

	}

	//超时
	ret = vdm_check_time_out(ptr_vod_data_manager);
	if (SUCCESS != ret)
	{
		range_list_clear(&em_range_list);
		range_list_clear(&range_list_to_get);
		return ret;
	}


	LOG_DEBUG("vdm_period_dispatch ptr_vod_data_manager->_send_pause=%d  ", ptr_vod_data_manager->_send_pause);
	ret = vdm_cdn_use_dispatch(ptr_vod_data_manager);

	vdm_dm_notify_flush_finished(ptr_vod_data_manager->_task_id);

	if (TRUE == ptr_vod_data_manager->_is_fetching
		&& TRUE != ptr_vod_data_manager->_send_pause
		&& TRUE == range_list_is_include(&ptr_vod_data_manager->_range_buffer_list, &range_to_get)
		&& TRUE == vdm_is_range_checked(ptr_vod_data_manager, &range_to_get))
	{
		//if( (range_to_get._index == 0 && continuing_end_pos > bytes_per_seconds*vdm_get_buffer_time_len()) || range_to_get._index != 0)
		{
			/*Buffer is in the bufferlist already, just return them*/
			LOG_DEBUG("vdm_period_dispatch.debug _callback_recv_handler %llu, %llu, range_to_get(%d,%d)", ptr_vod_data_manager->_start_pos, ptr_vod_data_manager->_fetch_length, range_to_get._index, range_to_get._num);
			ret = vdm_write_data_buffer(ptr_vod_data_manager, ptr_vod_data_manager->_start_pos, ptr_vod_data_manager->_user_buffer, ptr_vod_data_manager->_fetch_length, &range_to_get, &ptr_vod_data_manager->_buffer_list);
			CHECK_VALUE(ret);
			ret = (*(ptr_vod_data_manager->_callback_recv_handler))(SUCCESS, 0, ptr_vod_data_manager->_user_buffer, ptr_vod_data_manager->_fetch_length, ptr_vod_data_manager->_user_data);
			LOG_DEBUG("vdm_period_dispatch _callback_recv_handler ret %d ", ret);

			ptr_vod_data_manager->_is_fetching = FALSE;
			ptr_vod_data_manager->_send_pause = FALSE;
			ptr_vod_data_manager->_user_data = NULL;
			ptr_vod_data_manager->_block_time = 0;
			range_list_clear(&em_range_list);
			range_list_clear(&range_list_to_get);
			ret = SUCCESS;
			return ret;
		}
	}

	range_list_clear(&em_range_list);
	range_list_clear(&range_list_to_get);
	return ret;
}

#endif

_int32 vdm_default_timer_handler(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid)
{
	_u32 time_id = msg_info->_device_id;
	_int32 ret;
	VOD_DATA_MANAGER* ptr_vod_data_manager = (VOD_DATA_MANAGER*)msg_info->_user_data;
	ret = SUCCESS;

	LOG_DEBUG("vdm_default_timer_handler");


	if (errcode == MSG_CANCELLED)
	{
		LOG_DEBUG("vdm_default_timer_handler, time id: %u, dispatcher:0x%x   return because cancelled", time_id, ptr_vod_data_manager);
		return SUCCESS;
	}

	if (errcode == MSG_TIMEOUT && time_id == VDM_TIMER)
	{
		LOG_DEBUG("vdm_default_timer_handler, time id: %u, dispatcher:0x%x   do dispatcher.", time_id, ptr_vod_data_manager);
		ret = vdm_period_dispatch(ptr_vod_data_manager, FALSE);
		if (SUCCESS != ret && NULL != ptr_vod_data_manager->_user_data)
		{
			LOG_DEBUG("vdm_default_timer_handler vdm_period_dispatch ret=%d ", ret);
			ret = (*(ptr_vod_data_manager->_callback_recv_handler))(ret, 0, ptr_vod_data_manager->_user_buffer, ptr_vod_data_manager->_fetch_length, ptr_vod_data_manager->_user_data);
			LOG_DEBUG("vdm_default_timer_handler _callback_recv_handler ret %d ", ret);

			ptr_vod_data_manager->_is_fetching = FALSE;
			//ptr_vod_data_manager->_send_pause = FALSE;
			ptr_vod_data_manager->_user_data = NULL;
			ptr_vod_data_manager->_block_time = 0;
		}
	}
	else
	{
		LOG_ERROR("vdm_default_timer_handler, error callback return value, errcode:%u, time id: %d, ptr_vod_data_manager:0x%x   do dispatcher.", errcode, time_id, ptr_vod_data_manager);
	}

	return SUCCESS;
}


/* platform of vod data manager struct*/
_int32  vdm_init_data_manager(VOD_DATA_MANAGER* p_vod_data_manager_interface, struct _tag_task* p_task, _u32 file_index)
{
	_int32 ret_val = -1;

	list_init(&p_vod_data_manager_interface->_buffer_list._data_list);
	p_vod_data_manager_interface->_task_ptr = p_task;
	p_vod_data_manager_interface->_task_id = p_task->_task_id;
	sd_memcpy(p_vod_data_manager_interface->_eigenvalue, p_task->_eigenvalue, CID_SIZE);
	p_vod_data_manager_interface->_file_index = file_index;

	range_list_init(&p_vod_data_manager_interface->_range_buffer_list);
	range_list_init(&p_vod_data_manager_interface->_range_index_list);
	range_list_init(&p_vod_data_manager_interface->_range_waiting_list);
	range_list_init(&p_vod_data_manager_interface->_range_last_em_list);
	p_vod_data_manager_interface->_current_download_window._index = 0;
	p_vod_data_manager_interface->_current_download_window._num = 0;
	p_vod_data_manager_interface->_current_buffering_window._index = 0;
	p_vod_data_manager_interface->_current_buffering_window._num = 0;
	p_vod_data_manager_interface->_current_play_pos = 0;
	p_vod_data_manager_interface->_cdn_using = FALSE;
	p_vod_data_manager_interface->_can_read_data_notify = NULL;
	p_vod_data_manager_interface->_last_notify_read_pos = 0;

	sd_time_ms(&p_vod_data_manager_interface->_last_begin_play_time);
	p_vod_data_manager_interface->_last_read_pos = 0;

#ifdef EMBED_REPORT
	p_vod_data_manager_interface->_vod_last_time_type = VOD_LAST_NONE;
	sd_time(&p_vod_data_manager_interface->_vod_last_time);
	p_vod_data_manager_interface->_vod_play_times = 0;
	p_vod_data_manager_interface->_vod_play_begin_time = 0;
	sd_time(&p_vod_data_manager_interface->_vod_play_begin_time);
	p_vod_data_manager_interface->_vod_first_buffer_time_len = 0;
	p_vod_data_manager_interface->_vod_failed_times = 0;
	p_vod_data_manager_interface->_vod_play_drag_num = 0;
	p_vod_data_manager_interface->_vod_play_total_drag_wait_time = 0;
	p_vod_data_manager_interface->_vod_play_max_drag_wait_time = 0;
	p_vod_data_manager_interface->_vod_play_min_drag_wait_time = 0;
	p_vod_data_manager_interface->_vod_play_interrupt_times = 0;
	p_vod_data_manager_interface->_vod_play_total_buffer_time_len = 0;
	p_vod_data_manager_interface->_vod_play_max_buffer_time_len = 0;
	p_vod_data_manager_interface->_vod_play_min_buffer_time_len = 0;
	p_vod_data_manager_interface->_cdn_stop_times = 0;
	p_vod_data_manager_interface->_is_ad_type = FALSE;
#endif
#ifdef EMBED_REPORT
	p_vod_data_manager_interface->_vod_play_times++;
#endif

	ret_val = start_timer((msg_handler)vdm_default_timer_handler, NOTICE_INFINITE, VDM_TIMER_INTERVAL, VDM_TIMER, p_vod_data_manager_interface, &p_vod_data_manager_interface->_vdm_timer);

	ret_val = SUCCESS;
	CHECK_VALUE(ret_val);
	return ret_val;
}

BOOL vdm_is_range_checked(VOD_DATA_MANAGER* ptr_vod_data_manager, RANGE* r)
{
	RANGE range_to_read;
	RANGE range;
	BOOL bCheckData;
	_int32 i;

	bCheckData = FALSE;
	range_to_read = *r;

	bCheckData = dt_is_vod_check_data_task(ptr_vod_data_manager->_task_ptr);
	if (FALSE == bCheckData)
	{
		return TRUE;
	}

	if (TRUE == range_list_is_relevant(&ptr_vod_data_manager->_range_index_list, r))
	{
		return TRUE;
	}

	for (i = 0; i < range_to_read._num; i++)
	{
		range._index = range_to_read._index + i;
		range._num = 1;
		if (P2SP_TASK_TYPE == ptr_vod_data_manager->_task_ptr->_task_type)
		{
			if (FALSE == dm_range_is_check(&((P2SP_TASK*)ptr_vod_data_manager->_task_ptr)->_data_manager, &range))
			{
				LOG_ERROR("vdm_is_range_checked  range(%d,%d) . (%u,%u ) return FALSE", r->_index, r->_num, range._index, range._num);
				return FALSE;
			}
		}
	}

	return TRUE;
}

_int32 vdm_write_data_buffer(VOD_DATA_MANAGER* ptr_vod_data_manager, _u64 start_pos, char *buffer, _u64 length, RANGE* range_to_write, VOD_RANGE_DATA_BUFFER_LIST* buffer_list)
{
	VOD_RANGE_DATA_BUFFER* cur_buffer = NULL;
	RANGE_LIST range_list_to_write;
	LIST_ITERATOR  cur_node = LIST_BEGIN(buffer_list->_data_list);
	_u64 len1, len2;

	range_list_init(&range_list_to_write);
	range_list_add_range(&range_list_to_write, range_to_write, NULL, NULL);


	LOG_DEBUG("vdm_write_data_buffer . range[%d, %d] start_pos=%llu, length=%llu", range_to_write->_index, range_to_write->_num, start_pos, length);

	/*这里有越*/

	while (cur_node != LIST_END(buffer_list->_data_list))
	{
		if (NULL == cur_node)
			break;
		cur_buffer = (VOD_RANGE_DATA_BUFFER*)LIST_VALUE(cur_node);

		if (TRUE == range_list_is_relevant(&range_list_to_write, &cur_buffer->_range_data_buffer._range)
			&& TRUE == range_list_is_include(&ptr_vod_data_manager->_range_buffer_list, &cur_buffer->_range_data_buffer._range))
		{
			if (((_u64)cur_buffer->_range_data_buffer._range._index*(_u64)get_data_unit_size()) <= start_pos)
			{
				/*the fist block*/
				len1 = length;
				len2 = (_u64)cur_buffer->_range_data_buffer._data_length - (start_pos - ((_u64)cur_buffer->_range_data_buffer._range._index*(_u64)get_data_unit_size()));
				LOG_DEBUG("vdm_write_data_buffer .1 destpos=%llu, srcpos=%llu, length=%llu, len1=%llu, len2=%llu", (_u64)0, (_u64)(start_pos - (_u64)cur_buffer->_range_data_buffer._range._index*(_u64)get_data_unit_size()), (_u64)(len1 < len2) ? len1 : len2, len1, len2);

				sd_memcpy(buffer, cur_buffer->_range_data_buffer._data_ptr + (start_pos - ((_u64)cur_buffer->_range_data_buffer._range._index*(_u64)get_data_unit_size())), (_int32)(len1 < len2) ? len1 : len2);
			}
			else if ((_u64)((_u64)cur_buffer->_range_data_buffer._range._index*(_u64)get_data_unit_size() + (_u64)cur_buffer->_range_data_buffer._data_length) >= start_pos + length)
			{
				/*the last block*/
				len1 = start_pos + length - (_u64)cur_buffer->_range_data_buffer._range._index*(_u64)get_data_unit_size();
				len2 = (_u64)cur_buffer->_range_data_buffer._data_length;
				LOG_DEBUG("vdm_write_data_buffer .2 destpos=%llu, srcpos=%llu, length=%llu, len1=%llu, len2=%llu", (_u64)(cur_buffer->_range_data_buffer._range._index*get_data_unit_size() - start_pos), (_u64)0, (_u64)(len1 < len2) ? len1 : len2, len1, len2);
				sd_memcpy(buffer + (((_u64)cur_buffer->_range_data_buffer._range._index*(_u64)get_data_unit_size()) - start_pos), cur_buffer->_range_data_buffer._data_ptr, (_int32)(len1 < len2) ? len1 : len2);
			}
			else if (((_u64)cur_buffer->_range_data_buffer._range._index*(_u64)get_data_unit_size()) >= start_pos && (_u64)((_u64)cur_buffer->_range_data_buffer._range._index*(_u64)get_data_unit_size() + (_u64)cur_buffer->_range_data_buffer._data_length) <= start_pos + length)
			{
				/*middle blocks*/
				len1 = length;
				len2 = (_u64)cur_buffer->_range_data_buffer._data_length;
				LOG_DEBUG("vdm_write_data_buffer .3 destpos=%llu, srcpos=%llu, length=%llu, len1=%llu, len2=%llu", (_u64)((_u64)cur_buffer->_range_data_buffer._range._index*(_u64)get_data_unit_size() - start_pos), (_u64)0, (_u64)cur_buffer->_range_data_buffer._data_length, len1, len2);
				sd_memcpy(buffer + (((_u64)cur_buffer->_range_data_buffer._range._index*(_u64)get_data_unit_size()) - start_pos), cur_buffer->_range_data_buffer._data_ptr, (_int32)(len1 < len2) ? len1 : len2);
			}

		}
		cur_node = LIST_NEXT(cur_node);
	}

	range_list_clear(&range_list_to_write);
	LOG_DEBUG("vdm_write_data_buffer success.");
	return SUCCESS;

}

_int32 vdm_get_vod_data_manager_by_task_id(VOD_DATA_MANAGER_LIST* vod_data_manager_list, _u32 task_id, _u32 file_index, VOD_DATA_MANAGER** ret_ptr, BOOL only_run_vdm, BOOL force_start)
{
	_int32 ret = ERR_VOD_DATA_MANAGER_NOT_FOUND;
	TASK *task = NULL;
	if (tm_get_task_by_id(task_id, &task) != SUCCESS)
	{
		task = NULL;
	}
	*ret_ptr = NULL;
	VOD_DATA_MANAGER* cur_manager = NULL;

	LIST_ITERATOR  cur_node = LIST_BEGIN(vod_data_manager_list->_data_list);
	while (cur_node != LIST_END(vod_data_manager_list->_data_list))
	{
		if (NULL == cur_node)
			break;
		cur_manager = (VOD_DATA_MANAGER*)LIST_VALUE(cur_node);

		if (cur_manager->_task_id == task_id && cur_manager->_task_ptr) //BT任务也只有一个播放session
		{
			*ret_ptr = cur_manager;
			return SUCCESS;
		}
		else if (!only_run_vdm && (cur_manager->_task_id == task_id ||
			(task && cur_manager->_task_id &&
			sd_memcmp(cur_manager->_eigenvalue, task->_eigenvalue, CID_SIZE) == 0)))
		{
			*ret_ptr = cur_manager;
			if (task && force_start)
			{
				cur_manager->_task_id = task_id;
				cur_manager->_task_ptr = task;

				if (P2SP_TASK_TYPE == task->_task_type)
				{
#ifdef ENABLE_VOD
					dm_set_vod_mode(&((P2SP_TASK *)task)->_data_manager, TRUE);
#ifdef ENABLE_CDN
					pt_set_cdn_mode(task, TRUE);
#endif
#endif
				}
				
				#ifdef EMBED_REPORT
				sd_time(&cur_manager->_vod_play_begin_time);
				#endif
				
				vdm_period_dispatch(cur_manager, TRUE);
				start_timer((msg_handler)vdm_default_timer_handler, NOTICE_INFINITE, VDM_TIMER_INTERVAL, VDM_TIMER, cur_manager, &cur_manager->_vdm_timer);
			}
			return SUCCESS;
		}
		cur_node = LIST_NEXT(cur_node);
	}

	LOG_DEBUG("vdm_get_vod_data_manager_by_task_id failed.");
	return ret;
}
_int32 vdm_get_the_other_vod_task(VOD_DATA_MANAGER_LIST* vod_data_manager_list, _u32 cur_task_id, _u32* p_task_id)
{

	_int32 ret = ERR_VOD_DATA_MANAGER_NOT_FOUND;
	VOD_DATA_MANAGER* cur_manager = NULL;

	LIST_ITERATOR  cur_node = LIST_BEGIN(vod_data_manager_list->_data_list);

	*p_task_id = 0;

	while (cur_node != LIST_END(vod_data_manager_list->_data_list))
	{
		if (NULL == cur_node)
		{
			break;
		}
		cur_manager = (VOD_DATA_MANAGER*)LIST_VALUE(cur_node);

		if (cur_manager->_task_id != 0 && cur_manager->_task_id != cur_task_id && cur_task_id != 0) //BT浠诲姟涔熷彧鏈変竴涓挱鏀緎ession
		{
			*p_task_id = cur_manager->_task_id;
			ret = SUCCESS;
			break;
		}
		cur_node = LIST_NEXT(cur_node);
	}

	LOG_DEBUG("vdm_get_the_other_vod_task success.");
	return ret;
}

_int32 vdm_vod_async_read_file(_int32 task_id, _u32 file_index, _u64 start_pos, _u64 len, char *buf, _int32 block_time, vdm_recv_handler callback_handler, void* user_data)
{
	/*
	1、Get Task ptr by task_id
	2、check whether the data is in the bufferlist
	3、if havn't got buffer, then call dm_   to get buffer
	4、after got buffer, then return the data to it
	*/
	TASK* taskinfo = NULL;
	_int32 ret = SUCCESS;

	//VOD_RANGE_DATA_BUFFER* data_buffer = NULL;
	VOD_DATA_MANAGER* ptr_vod_data_manager = NULL;
	TASK* ptr_task = NULL;

	RANGE  range_to_get = { 0 };
	_u64  filesize = 0;

#ifdef ENABLE_BT
#ifdef EMBED_REPORT
	_u32 end_time = 0;
	VOD_REPORT_PARA vod_para = { 0 };
#endif
#endif

	LOG_ERROR("[vod_dispatch_analysis] vdm_vod_async_read_file. taskid=%d, file_index=%d, [%llu, %llu, %llu), block_time= %u", task_id, file_index, start_pos, len, start_pos + len, block_time);
	/*
		if(FALSE == vdm_is_vod_enabled())
		{
		LOG_DEBUG("vdm_vod_async_read_file vdm_is_vod_enabled==FALSE");
		ret = ERR_VOD_IS_DISABLED;
		return ret;
		}
		*/
	ret = tm_get_task_by_id(task_id, &taskinfo);
	if (SUCCESS != ret)
	{
		LOG_DEBUG("vdm_vod_async_read_file .tm_get_task_by_id return = %d", ret);
		return ret;
	}
	/*get DOWNLOAD_TASK ptr here*/
	ptr_task = (TASK*)taskinfo;
	ret = vdm_get_vod_data_manager_by_task_id(&g_vdm_list, task_id, file_index, &ptr_vod_data_manager, FALSE, TRUE);
	if (SUCCESS != ret || NULL == ptr_vod_data_manager)
	{
		LOG_DEBUG("vdm_vod_async_read_file .vdm_get_vod_data_manager_by_task_ptr return = %d", ret);
		ret = vdm_get_free_vod_data_manager(ptr_task, file_index, &ptr_vod_data_manager);
		if (SUCCESS != ret)
		{
			LOG_DEBUG("vdm_vod_async_read_file .vdm_get_free_vod_data_manager return = %d", ret);
			return ret;
		}
		ret = vdm_get_vod_data_manager_by_task_id(&g_vdm_list, task_id, file_index, &ptr_vod_data_manager, TRUE, FALSE);
		if (SUCCESS != ret || NULL == ptr_vod_data_manager)
		{
			LOG_DEBUG("vdm_vod_async_read_file .vdm_get_vod_data_manager_by_task_ptr 2return = %d", ret);
			return ERR_VOD_DATA_MANAGER_NOT_FOUND;
		}

		vdm_vod_malloc_vod_buffer();

		if (P2SP_TASK_TYPE == ptr_task->_task_type)
		{
#ifdef ENABLE_VOD
			dm_set_vod_mode(&((P2SP_TASK*)ptr_task)->_data_manager, TRUE);
#ifdef ENABLE_CDN
			pt_set_cdn_mode( ptr_task,TRUE);
#endif
#endif
		}
#ifdef ENABLE_BT
		else if (BT_TASK_TYPE == ptr_task->_task_type)
		{
			bdm_set_vod_mode(&((BT_TASK*)ptr_task)->_data_manager, TRUE);
		}
#endif
		else
		{
			LOG_DEBUG("vdm_vod_async_read_file .unknown task type ");
			return ERR_VOD_DATA_UNKNOWN_TASK;
		}

		/* 同一时间只允许一个点播任务存在 */
		{
			_u32 other_task_id = 0;
			ret = vdm_get_the_other_vod_task(&g_vdm_list, task_id, &other_task_id);
			if (ret == SUCCESS)
			{
				sd_assert(FALSE);
				vdm_vod_stop_task(other_task_id, FALSE);
			}
		}

	}

#ifdef ENABLE_BT
	if (BT_TASK_TYPE == ptr_task->_task_type)
	{
		if (ptr_vod_data_manager->_file_index != file_index)
		{
			//BT任务切换文件
			//report
#ifdef EMBED_REPORT
			sd_time(&end_time);
			sd_memset(&vod_para, 0, sizeof(VOD_REPORT_PARA));
			vod_para._vod_play_time = 3;
			vod_para._vod_first_buffer_time = 4;
			vod_para._vod_interrupt_times = 5;
			vod_para._min_interrupt_time = 6;
			vod_para._max_interrupt_time = 7;
			vod_para._avg_interrupt_time = 8;

			emb_reporter_bt_stop_file(ptr_task, ptr_vod_data_manager->_file_index, &vod_para, TRUE);

			/*
						  emb_reporter_bt_stop_file(ptr_task
						  , ptr_vod_data_manager->_file_index
						  , ptr_vod_data_manager->_vod_times
						  , TIME_SUBZ(end_time,ptr_vod_data_manager->_vod_play_time)
						  , TIME_SUBZ(ptr_vod_data_manager->_vod_first_buffer_time,ptr_vod_data_manager->_vod_play_time)
						  , ptr_vod_data_manager->_vod_interrupt_times
						  , ptr_vod_data_manager->_vod_failed_times
						  );
						  */


#endif
			vdm_uninit_data_manager(ptr_vod_data_manager, FALSE);
			LOG_DEBUG("vdm_vod_async_read_file .BT task callback return = %d", ret);
			vdm_init_data_manager(ptr_vod_data_manager, ptr_task, file_index);
			//set vod mode
			bdm_set_vod_mode(&((BT_TASK*)ptr_task)->_data_manager, TRUE);
			ptr_vod_data_manager->_user_data = user_data;
			ptr_vod_data_manager->_callback_recv_handler = callback_handler;
		}
	}
#endif

	vdm_update_dlpos(task_id, start_pos);

	if (TRUE == ptr_vod_data_manager->_is_fetching)
	{
		if (ptr_vod_data_manager->_user_data == user_data)
		{
			LOG_DEBUG("vdm_vod_async_read_file .tm_get_task_by_id return = %d", ret);
			return ERR_VOD_DATA_OP_IN_PROGRESS;
		}
		else
		{
			ret = (*ptr_vod_data_manager->_callback_recv_handler)(ERR_VOD_DATA_OP_INTERUPT, 0, ptr_vod_data_manager->_user_buffer, 0, ptr_vod_data_manager->_user_data);
			LOG_DEBUG("vdm_vod_async_read_file .callback return = %d", ret);
			ptr_vod_data_manager->_user_data = user_data;
			ptr_vod_data_manager->_callback_recv_handler = callback_handler;
			ptr_vod_data_manager->_user_buffer = buf;
			ptr_vod_data_manager->_fetch_length = len;
		}
	}

	if (P2SP_TASK_TYPE == ptr_task->_task_type)
	{
		filesize = dm_get_file_size(&((P2SP_TASK*)ptr_task)->_data_manager);
	}
#ifdef ENABLE_BT
	else if (BT_TASK_TYPE == ptr_task->_task_type)
	{
		filesize = bdm_get_sub_file_size(&((BT_TASK*)ptr_task)->_data_manager, ptr_vod_data_manager->_file_index);
	}
#endif
	else
	{
		LOG_DEBUG("vdm_vod_async_read_file .unknown task type ");
		return ERR_VOD_DATA_UNKNOWN_TASK;
	}
	range_to_get = pos_length_to_range(start_pos, len, filesize);

	/*	if(len> (5*get_data_unit_size()))
		{
		LOG_DEBUG("vdm_vod_async_read_file .get too more data %d bytes ", range_to_get._num*get_data_unit_size() );
		return ERR_VOD_DATA_BUFFER_TOO_BIG;
		}*/

	if ((len == 0) || (filesize == 0))
	{
		LOG_DEBUG("vdm_vod_async_read_file .get 0 bytes ", range_to_get._num*get_data_unit_size());
		return ERR_VOD_DATA_SUCCESS;
	}


	if (ptr_vod_data_manager->_current_play_pos + ptr_vod_data_manager->_fetch_length == start_pos)
	{
		ptr_vod_data_manager->_current_play_pos = start_pos;
	}
	else
	{
		if (FALSE == vdm_pos_is_in_range_list(start_pos, len, filesize, &ptr_vod_data_manager->_range_index_list))
		{
			LOG_DEBUG("vdm_vod_async_read_file .vdm_pos_is_in_range_list set cur_play_pos:%llu to %llu ", ptr_vod_data_manager->_current_play_pos, start_pos);
			//判断首缓冲或者拖动
#ifdef _VOD_NO_DISK
			if( start_pos && (ptr_vod_data_manager->_current_play_pos+ptr_vod_data_manager->_fetch_length != start_pos  && ptr_vod_data_manager->_current_play_pos!=start_pos) )
			{
				LOG_ERROR("vdm_vod_async_read_file set send_pause = TRUE,  ptr_vod_data_manager->_start_pos:%llu, ptr_vod_data_manager->_fetch_length:%llu, start_pos:%llu,cur_play_pos:%llu",
					ptr_vod_data_manager->_start_pos, ptr_vod_data_manager->_fetch_length, start_pos, ptr_vod_data_manager->_current_play_pos);
				vdm_on_play_drag(ptr_vod_data_manager);
				ptr_vod_data_manager->_last_notify_read_pos = start_pos;
			}
			else if(start_pos == 0)
			{
				vdm_on_first_buffering(ptr_vod_data_manager);
			}
#endif /* _VOD_NO_DISK */
			ptr_vod_data_manager->_current_play_pos = start_pos;
		}
	}

	if (TRUE != ptr_vod_data_manager->_send_pause
		&&  TRUE == range_list_is_include(&ptr_vod_data_manager->_range_buffer_list, &range_to_get)
		&& TRUE == vdm_is_range_checked(ptr_vod_data_manager, &range_to_get))
	{
		ptr_vod_data_manager->_start_pos = start_pos;
		ptr_vod_data_manager->_fetch_length = len;
		ptr_vod_data_manager->_file_index = file_index;
		//Buffer is in the bufferlist already, just return them
		ret = vdm_write_data_buffer(ptr_vod_data_manager, start_pos, buf, len, &range_to_get, &ptr_vod_data_manager->_buffer_list);
		CHECK_VALUE(ret);
		ret = (*callback_handler)(SUCCESS, 0, buf, len, user_data);
		return ret;
	}

#ifdef VOD_DEBUG
	printf(" ready to  read task :%u  offset:%llu, len:%llu,  blocktime:%d , time:%u , pipe (%u,%u,%u,%u), speed %u (%u, %u) .\n",
		task_id , start_pos, len, block_time, time(0), ptr_task->dowing_server_pipe_num, ptr_task->connecting_server_pipe_num,
		ptr_task->dowing_peer_pipe_num, ptr_task->connecting_peer_pipe_num, ptr_task->speed, ptr_task->server_speed, ptr_task->peer_speed);
#endif

	/*record fetch time here , */
	sd_time_ms(&ptr_vod_data_manager->_last_fetch_time);
	if (start_pos != ptr_vod_data_manager->_start_pos)
	{
		ptr_vod_data_manager->_last_fetch_time_for_same_pos = ptr_vod_data_manager->_last_fetch_time;
	}
	if (start_pos == 0)
	{
		ptr_vod_data_manager->_last_fetch_time_for_same_pos = ptr_vod_data_manager->_last_fetch_time;
	}
	if (ptr_vod_data_manager->_last_fetch_time_for_same_pos == 0)
	{
		ptr_vod_data_manager->_last_fetch_time_for_same_pos = ptr_vod_data_manager->_last_fetch_time;
	}

	ptr_vod_data_manager->_is_fetching = TRUE;
	ptr_vod_data_manager->_start_pos = start_pos;
	ptr_vod_data_manager->_fetch_length = len;
	ptr_vod_data_manager->_file_index = file_index;
	ptr_vod_data_manager->_block_time = block_time;
	ptr_vod_data_manager->_user_data = user_data;
	ptr_vod_data_manager->_callback_recv_handler = callback_handler;
	ptr_vod_data_manager->_user_buffer = buf;

	ret = vdm_period_dispatch(ptr_vod_data_manager, FALSE);
	if (SUCCESS != ret)
	{
		LOG_DEBUG("vdm_vod_async_read_file .vdm_period_dispatch ret=%d ", ret);
		ptr_vod_data_manager->_is_fetching = FALSE;
		//防止vdm_period_dispatch调度再次释放信号量
		ptr_vod_data_manager->_user_data = NULL;
	}

	return ret;
}

/*api */
_int64 vdm_vod_read_file(_int32 task_id, _u64 start_pos, _u64 len, char *buf, _int32 block_time)
{
	/*
	1、Get Task ptr by task_id
	2、check whether the data is in the bufferlist
	3、if havn't got buffer, then call dm_   to get buffer
	4、after got buffer, then return the data to it
	*/
	TASK* taskinfo;
	_int32 ret;
	VOD_RANGE_DATA_BUFFER* data_buffer;


	LOG_DEBUG("vdm_vod_read_file .");
	data_buffer = NULL;

	if (FALSE == vdm_is_vod_enabled())
	{
		LOG_DEBUG("vdm_vod_read_file vdm_is_vod_enabled==FALSE");
		ret = ERR_VOD_IS_DISABLED;
		return ret;
	}

	ret = tm_get_task_by_id(task_id, &taskinfo);
	if (SUCCESS != ret)
	{
		LOG_DEBUG("vdm_vod_read_file .tm_get_task_by_id return = %d", ret);
		return ret;
	}
	return SUCCESS;
}
_int64 vdm_vod_bt_read_file(_int32 task_id, _int32 file_index, _u64 start_pos, _u64 len, char *buf, _int32 block_time)
{
	/**/
	return SUCCESS;

}
_int32 vdm_vod_set_index_data(_int32 task_id, _u64 start_pos, _u64 len)
{
	/**/
	return SUCCESS;
}

_int32 vdm_async_read_file_handler(_int32 errcode, _u32 pending_op_count, char* buffer, _u32 had_recv, void* user_data)
{
	VOD_DATA_MANAGER_READ* _p_param = (VOD_DATA_MANAGER_READ*)user_data;
	LOG_DEBUG("vdm_async_read_file_handler..., errcode=%d, had_recv =%d", errcode, had_recv);

#ifdef VOD_DEBUG
	printf(" ready to  return error: %d, recv len :%u .\n",   errcode, had_recv);
#endif

	if (errcode != SUCCESS || had_recv == 0)
	{
		_p_param->_result = errcode;
		LOG_DEBUG("signal_sevent_handle:_result=%d", _p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}

	_p_param->_result = SUCCESS;
	LOG_DEBUG("signal_sevent_handle:_result=%d", _p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}
#ifdef IPAD_KANKAN
static _int32 g_current_vod_task_id = 0;
static TASK* g_ptr_task = NULL;
static _int32 g_is_syn_read = -1; /* 1为同步读,0为异步读 */
static FILEINFO * g_file_info = NULL;
//static _u32 g_task_file_id = 0;
static VOD_REPORT g_vod_report;
_int32 vdm_set_current_vod_task_id(_int32 task_id)
{
	_int32 ret = SUCCESS;

	if(g_is_syn_read == -1)
	{
		sd_assert(g_current_vod_task_id == 0);
		sd_assert(g_ptr_task == NULL);
		sd_assert(g_is_syn_read == -1);
		sd_assert(g_file_info == NULL);
		//sd_assert(g_task_file_id == 0);

		g_current_vod_task_id = task_id;
		ret = tm_get_task_by_id(task_id,&g_ptr_task);
		sd_assert(SUCCESS == ret);
		if(SUCCESS != ret)
		{
			LOG_DEBUG("vdm_check_if_syn_read .tm_get_task_by_id return = %d", ret);
			return ret;
		}

		if(g_ptr_task->file_size!=0 && g_ptr_task->file_size == g_ptr_task->_downloaded_data_size)
		{
			g_is_syn_read = 1;
			g_file_info = &(((P2SP_TASK*)g_ptr_task)->_data_manager._file_info);
			//g_task_file_id = 0;
		}
		else
		{
			g_is_syn_read = 0;
			//g_current_vod_task_id = 0;
			g_ptr_task = NULL;
			g_file_info = NULL;
			//g_task_file_id = 0;
		}
	}
	return SUCCESS;
}
BOOL vdm_check_if_syn_read(_int32 task_id)
{
	_int32 ret;

	if(g_is_syn_read == -1)
	{
		return FALSE;
	}

	if(g_is_syn_read == 1)
	{
		if(g_current_vod_task_id != task_id)
		{
			//sd_assert(FALSE);
			return FALSE;
		}
	}

	return (g_is_syn_read == 1)?TRUE:FALSE;
}

_int32 vdm_syn_handle_block_data( TMP_FILE *p_file_struct, BLOCK_DATA_BUFFER *p_block_data_buffer, _u32 block_disk_index, _u32 file_id )
{
	_int32 ret_val;
	_u32 read_len;//????
	_u64 off_set = block_disk_index  *p_file_struct->_block_size + p_block_data_buffer->_block_offset;
	LOG_DEBUG( "vdm_syn_handle_block_data. block_disk_index: %u, file_id: %u", block_disk_index, file_id );
#if 1
	ret_val =  sd_pread( file_id, p_block_data_buffer->_data_ptr, p_block_data_buffer->_data_length,off_set, &read_len);
#else
	ret_val = sd_setfilepos( file_id, off_set );
	CHECK_VALUE( ret_val );

	ret_val = sd_read( file_id, p_block_data_buffer->_data_ptr, p_block_data_buffer->_data_length, &read_len );
#endif
	CHECK_VALUE( ret_val );

	sd_assert(p_block_data_buffer->_data_length==read_len);

	LOG_DEBUG( "vdm_syn_handle_block_data return. data_length: %u, read_len: %u", p_block_data_buffer->_data_length, read_len );
	return SUCCESS;
}
_int32 vdm_handle_syn_read_block_list( TMP_FILE *p_file_struct )
{
	_int32 ret_val = SUCCESS;
	BOOL ret_bool = FALSE;
	BLOCK_DATA_BUFFER *p_block_data_buffer = NULL;
	LIST_ITERATOR cur_node, next_node;

	LOG_DEBUG( "vdm_handle_syn_read_block_list." );

	if(p_file_struct->_device_id== 0)
	{
		CHECK_VALUE( FM_LOGIC_ERROR );
	}

	cur_node = LIST_BEGIN( p_file_struct->_syn_read_block_list );

	while( cur_node != LIST_END( p_file_struct->_syn_read_block_list ) )
	{
		_u32 block_disk_index;
		p_block_data_buffer = (BLOCK_DATA_BUFFER *)LIST_VALUE( cur_node );
		ret_bool = fm_get_cfg_disk_block_index( &p_file_struct->_block_index_array, p_block_data_buffer->_block_index, &block_disk_index );
		if( !ret_bool )
		{
			printf("\n\n total_index_num=%u,valid_index_num=%u,block_index=%u,syn_read_block_list size=%u\n\n",p_file_struct->_block_index_array._total_index_num,p_file_struct->_block_index_array._valid_index_num,p_block_data_buffer->_block_index,list_size( &p_file_struct->_syn_read_block_list ));
			printf("\n\n block_offset=%u,data_length=%u\n\n",p_block_data_buffer->_block_offset,p_block_data_buffer->_data_length);
			sd_assert(FALSE);
			return FM_LOGIC_ERROR;
		}
		ret_val = vdm_syn_handle_block_data( p_file_struct, p_block_data_buffer, block_disk_index, p_file_struct->_device_id);
		CHECK_VALUE( ret_val );
		ret_val = block_data_buffer_free_wrap( p_block_data_buffer );
		CHECK_VALUE( ret_val );
		next_node = LIST_NEXT( cur_node );
		list_erase( &p_file_struct->_syn_read_block_list, cur_node );
		cur_node = next_node;
	}

	return SUCCESS;
}

_int32 vdm_generate_block_list( TMP_FILE *p_file_struct, RANGE_DATA_BUFFER *p_range_buffer,  _u64 start_pos )
{
	_int32 ret_val = SUCCESS;
	_u32 range_num = 0;
	_u64 range_pos = 0;
	_u32 block_size, data_unit_size;
	_u32 end_block_index = 0, begin_block_index = 0, cur_block_index = 0;
	_u32 last_block_length = 0;

	data_unit_size = get_data_unit_size();
	range_num = p_range_buffer->_range._num;
	range_pos = p_range_buffer->_range._index;

	sd_assert( range_num != 0 );

	block_size = p_file_struct->_block_size;

	sd_assert( block_size % data_unit_size == 0 );
	sd_assert( block_size / data_unit_size != 0 );

	begin_block_index = range_pos / ( block_size / data_unit_size );
	if( ( range_pos + range_num ) % ( block_size / data_unit_size ) == 0 )
	{
		end_block_index = ( range_pos + range_num ) / ( block_size / data_unit_size ) - 1;
	}
	else
	{
		end_block_index = ( range_pos + range_num ) / ( block_size / data_unit_size );
	}

	LOG_DEBUG( "vdm_generate_block_list, begin_block_index:%u, end_block_index:%u############",
		begin_block_index, end_block_index );

	cur_block_index = begin_block_index;
	while( cur_block_index <= end_block_index )
	{
		BLOCK_DATA_BUFFER *p_block_data_buffer = NULL;
		ret_val = block_data_buffer_malloc_wrap( &p_block_data_buffer );
		CHECK_VALUE( ret_val );

		p_block_data_buffer->_block_index = cur_block_index;
		if( cur_block_index != end_block_index )//muti blocks generate normal blocks
		{
			if( cur_block_index == begin_block_index && (start_pos % block_size != 0) )
			{
				p_block_data_buffer->_block_offset = start_pos % block_size;
				p_block_data_buffer->_data_length = block_size - p_block_data_buffer->_block_offset;
			}
			else
			{
				p_block_data_buffer->_block_offset = 0;
				p_block_data_buffer->_data_length = block_size;
			}
			p_block_data_buffer->_is_range_buffer_end = FALSE;
		}
		else if(  begin_block_index != end_block_index )//muti blocks generate end block
		{
			p_block_data_buffer->_block_offset = 0;
			p_block_data_buffer->_data_length = (_u64) (start_pos + p_range_buffer->_data_length) % block_size;
			p_block_data_buffer->_is_range_buffer_end = TRUE;
		}
		else  //single blocks
		{
			p_block_data_buffer->_block_offset = start_pos % block_size;
			p_block_data_buffer->_data_length = p_range_buffer->_data_length;
			p_block_data_buffer->_is_range_buffer_end = TRUE;
		}
		LOG_DEBUG( "vdm_generate_block_list: _block_data_buffer_ptr:0x%x, block_index:%u, _block_offset:%u, _data_length:%u ", p_block_data_buffer,
			cur_block_index, p_block_data_buffer->_block_offset, p_block_data_buffer->_data_length);

		p_block_data_buffer->_data_ptr = p_range_buffer->_data_ptr + last_block_length;
		last_block_length = p_block_data_buffer->_data_length;

		p_block_data_buffer->_is_call_back = FALSE;
		p_block_data_buffer->_is_tmp_block = FALSE;
		p_block_data_buffer->_try_num = 0;
		p_block_data_buffer->_operation_type = FM_OP_BLOCK_SYN_READ;
		p_block_data_buffer->_buffer_para_ptr = NULL;
		p_block_data_buffer->_user_ptr = NULL;
		p_block_data_buffer->_call_back_ptr = NULL;

		p_block_data_buffer->_range_buffer = (void *)p_range_buffer;

		LOG_DEBUG( "fm_generate_block_list, block_buffer_ptr:0x%x, op_type = FM_OP_BLOCK_SYN_READ ", p_block_data_buffer );
		ret_val = list_push( &p_file_struct->_syn_read_block_list, (void*)p_block_data_buffer );
		CHECK_VALUE( ret_val );

		cur_block_index++;
	}

	return SUCCESS;
}
_int32  vdm_file_syn_read_buffer( struct tagTMP_FILE *p_file_struct, RANGE_DATA_BUFFER *p_data_buffer , _u64 start_pos)
{
	_int32 ret_val = SUCCESS;
	_u32 syn_read_list_size = 0;

	LOG_DEBUG( "vdm_file_syn_read_buffer. data_buffer ptr:0x%x. ", p_data_buffer );

	ret_val = vdm_generate_block_list( p_file_struct, p_data_buffer, start_pos);
	CHECK_VALUE( ret_val );

	ret_val = vdm_handle_syn_read_block_list( p_file_struct );
	CHECK_VALUE( ret_val );

	syn_read_list_size = list_size( &p_file_struct->_syn_read_block_list );
	sd_assert( syn_read_list_size == 0 );

	return SUCCESS;
}

_int32 vdm_vod_ync_read_file(_int32 task_id, _u32 file_index, _u64 start_pos, _u64 len, char *buf, _int32 block_time)
{
	_int32 ret_val = SUCCESS;
	RANGE f ;
	RANGE_DATA_BUFFER read_buf;
	FILEINFO * p_file_info = g_file_info;

	if(p_file_info->_tmp_file_struct == NULL)
	{
		/* 文件还没有打开 */
		return ERR_VOD_DATA_FETCH_TIMEOUT;
	}

	if(p_file_info->_tmp_file_struct->_device_id == 0||p_file_info->_tmp_file_struct->_device_id == INVALID_FILE_ID)
	{
		/* 文件还没有打开 */
		return ERR_VOD_DATA_FETCH_TIMEOUT;
	}

	f =  pos_length_to_range(start_pos, len,  p_file_info->_file_size);
	read_buf._range = f;
	read_buf._data_length = len;
	read_buf._buffer_length = len;
	if(start_pos+  len> p_file_info->_file_size)
	{
		read_buf._data_length = p_file_info->_file_size - start_pos;
	}

	read_buf._data_ptr = buf;

	ret_val =  vdm_file_syn_read_buffer( p_file_info->_tmp_file_struct ,  &read_buf,  start_pos);
	sd_assert(ret_val==SUCCESS);
	if(ret_val != SUCCESS)
	{
		LOG_ERROR("vdm_vod_ync_read_file, task_id=%u, start_pos=%llu,len=%llu,filesize:%llu , range(%u,%u), data_length:%u,  syn raed failure=%d.",task_id, start_pos,len,p_file_info->_file_size, f._index, f._num, read_buf._data_length, ret_val);
	}
	return ret_val;
}
_int32 vdm_vod_stop_ync_task(_int32 task_id)
{
	_int32 ret;
	TASK* taskinfo;
	TASK* ptr_task;
	_int32  file_index;
#ifdef EMBED_REPORT
	VOD_REPORT_PARA vod_para = {0};
#endif

	file_index = 0;

	LOG_DEBUG("vdm_vod_stop_task .taskid=%d, file_index=%d", task_id, file_index );
	ret = tm_get_task_by_id(task_id,&taskinfo);
	if(SUCCESS != ret)
	{
		LOG_DEBUG("vdm_vod_stop_task .tm_get_task_by_id return = %d", ret);
		return ret;
	}
	/*get DOWNLOAD_TASK ptr here*/
	ptr_task = (TASK*)taskinfo;
	sd_assert(ptr_task == g_ptr_task);
	sd_assert(P2SP_TASK_TYPE == ptr_task->_task_type);
	{
#ifdef EMBED_REPORT
		vod_para._vod_play_time = g_vod_report._vod_play_time_len;
		vod_para._vod_first_buffer_time = g_vod_report._vod_first_buffer_time_len;
		vod_para._vod_interrupt_times =  g_vod_report._vod_play_interrupt_times;
		vod_para._min_interrupt_time = 0;
		vod_para._max_interrupt_time =  0;
		vod_para._avg_interrupt_time = 0;

		vod_para._bit_rate = 0;
		vod_para._vod_total_buffer_time = g_vod_report._vod_play_total_buffer_time_len;
		vod_para._vod_total_drag_wait_time =  g_vod_report._vod_play_total_drag_wait_time;
		vod_para._vod_drag_num = g_vod_report._vod_play_drag_num;
		vod_para._play_interrupt_1 =  g_vod_report._play_interrupt_1;
		vod_para._play_interrupt_2 =  g_vod_report._play_interrupt_2;
		vod_para._play_interrupt_3 =  g_vod_report._play_interrupt_3;
		vod_para._play_interrupt_4 =  g_vod_report._play_interrupt_4;
		vod_para._play_interrupt_5 =  g_vod_report._play_interrupt_5;
		vod_para._play_interrupt_6 =  g_vod_report._play_interrupt_6;
		vod_para._cdn_stop_times = 0;
		emb_reporter_common_stop_task(ptr_task, &vod_para);
#endif

		dm_set_vod_mode(&((P2SP_TASK*)ptr_task)->_data_manager, FALSE);
		/* 启用其他任务的下载模式 */
		tm_pause_download_except_task(ptr_task,FALSE);

		g_current_vod_task_id = 0;
		g_ptr_task = NULL;
		g_is_syn_read = -1; /* 1为同步读,0为异步读 */
		g_file_info = NULL;

		//if(g_task_file_id != 0)
		//{
		//	sd_close_ex(g_task_file_id);
		//	g_task_file_id = 0;
		//}

	}

	if(ptr_task)
	{
		sd_time(&ptr_task->_stop_vod_time);
	}
	return SUCCESS;

}
#endif /* IPAD_KANKAN */


_int32 vdm_vod_read_file_handle(void * _param)
{
	VOD_DATA_MANAGER_READ* _p_param = (VOD_DATA_MANAGER_READ*)_param;
	_int32 ret;

#ifdef IPAD_KANKAN
	vdm_set_current_vod_task_id(_p_param->_task_id);
	if(vdm_check_if_syn_read( _p_param->_task_id))
	{
		ret = vdm_vod_ync_read_file(_p_param->_task_id, _p_param->_file_index, _p_param->_start_pos, _p_param->_length, _p_param->_buffer, _p_param->_blocktime);
		_p_param->_result = ret;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	else
#endif
	{
		if (_p_param->_blocktime > 50)
		{
			/* 不要等太久,以免界面卡死 */
			_p_param->_blocktime = 50;
		}
		ret = vdm_vod_async_read_file(_p_param->_task_id, _p_param->_file_index, _p_param->_start_pos, _p_param->_length, _p_param->_buffer, _p_param->_blocktime, vdm_async_read_file_handler, (void*)_p_param);
		if (SUCCESS != ret)
		{
			_p_param->_result = ret;
			LOG_DEBUG("signal_sevent_handle:_result=%d", _p_param->_result);
			return signal_sevent_handle(&(_p_param->_handle));
		}
	}
	/*wait for callback*/
	return SUCCESS;
}

_int32 vdm_vod_stop_task(_int32 task_id, BOOL keep_data)
{
	_int32 ret = -1;
	TASK* taskinfo;
	TASK* ptr_task = NULL;
	_int32  file_index;
	_u32 _current_task_id = 0;
#ifdef EMBED_REPORT
	_u32   end_time;
	VOD_REPORT_PARA vod_para = { 0 };

#endif
	VOD_DATA_MANAGER* ptr_vod_data_manager;

#ifdef ENABLE_BT
	VOD_DATA_MANAGER_LIST* ptr_vod_data_manager_list = &g_vdm_list;
	LIST_ITERATOR  cur_node = LIST_BEGIN(ptr_vod_data_manager_list->_data_list);
#endif

	file_index = 0;
	_current_task_id = vdm_get_current_vod_task_id();

	LOG_DEBUG("vdm_vod_stop_task .taskid=%d, file_index=%d, current_task_id:%u", task_id, file_index, _current_task_id);

#ifdef IPAD_KANKAN
	if(g_is_syn_read == -1)
	{
		return SUCCESS;
	}

	if(vdm_check_if_syn_read(task_id))
	{
		return vdm_vod_stop_ync_task(task_id);
	}

	g_current_vod_task_id = 0;
	g_is_syn_read = -1; /* 1为同步读,0为异步读 */
#endif

	ret = tm_get_task_by_id(task_id, &taskinfo);
	if (SUCCESS != ret)
	{
		LOG_DEBUG("vdm_vod_stop_task .tm_get_task_by_id return = %d", ret);
		return ret;
	}
	/*get DOWNLOAD_TASK ptr here*/
	ptr_task = (TASK*)taskinfo;
	if (ptr_task == NULL || P2SP_TASK_TYPE == ptr_task->_task_type)
	{
		ret = vdm_get_vod_data_manager_by_task_id(&g_vdm_list, task_id, file_index, &ptr_vod_data_manager, FALSE, FALSE);
		if (SUCCESS != ret || NULL == ptr_vod_data_manager)
		{
			LOG_DEBUG("vdm_vod_stop_task .vdm_get_vod_data_manager_by_task_ptr return = %d", ret);
			return ret;
		}
#ifdef EMBED_REPORT
		if (ptr_task && ptr_vod_data_manager->_vod_play_begin_time)
		{
			LOG_DEBUG("[vod_dispatch_analysis][vdm_vod_stop_task] task_type:%u, start_time:%u, stop_time:%u, duration:%u,\
					  				  speed:%u, file_size:%u, _downloaded_data_size:%u, _written_data_size:%u, _total_server_res_count:%u,\
									  				  _valid_server_res_count:%u, _total_peer_res_count:%u, dowing_peer_pipe_num:%u, connecting_peer_pipe_num:%u",
													  ptr_task->_task_type,
													  ptr_task->_start_time,
													  ptr_task->_stop_vod_time,
													  ptr_task->_stop_vod_time - ptr_task->_start_time,
													  ptr_task->speed,
													  ptr_task->file_size,
													  ptr_task->_downloaded_data_size,
													  ptr_task->_written_data_size,
													  ptr_task->_total_server_res_count,
													  ptr_task->_valid_server_res_count,
													  ptr_task->_total_peer_res_count,
													  ptr_task->dowing_peer_pipe_num,
													  ptr_task->connecting_peer_pipe_num
													  );

			sd_time(&end_time);
			//vod_para._vod_play_time = ptr_vod_data_manager->_vod_play_times;
			LOG_DEBUG("vod_play_time start_time: %d, end_time: %d", ptr_vod_data_manager->_vod_play_begin_time, end_time);
#ifdef IPAD_KANKAN
			// 上报广告类型的任务
			vod_para._is_ad_type = ptr_vod_data_manager->_is_ad_type;
#endif
			vod_para._vod_play_time = TIME_SUBZ(end_time, ptr_vod_data_manager->_vod_play_begin_time);
			vod_para._vod_first_buffer_time = ptr_vod_data_manager->_vod_first_buffer_time_len;
			vod_para._vod_interrupt_times = ptr_vod_data_manager->_vod_play_interrupt_times;
			vod_para._min_interrupt_time = ptr_vod_data_manager->_vod_play_min_buffer_time_len;
			vod_para._max_interrupt_time = ptr_vod_data_manager->_vod_play_max_buffer_time_len;
			if (ptr_vod_data_manager->_vod_play_interrupt_times > 0)
			{
				vod_para._avg_interrupt_time = ptr_vod_data_manager->_vod_play_total_buffer_time_len / ptr_vod_data_manager->_vod_play_interrupt_times;
			}
			else
			{
				vod_para._avg_interrupt_time = 0;
			}

			vod_para._bit_rate = ptr_vod_data_manager->_bit_per_seconds;
			vod_para._vod_total_buffer_time = ptr_vod_data_manager->_vod_play_total_buffer_time_len;
			vod_para._vod_total_drag_wait_time = ptr_vod_data_manager->_vod_play_total_drag_wait_time;
			vod_para._vod_drag_num = ptr_vod_data_manager->_vod_play_drag_num;
			vod_para._play_interrupt_1 = ptr_vod_data_manager->_play_interrupt_1;
			vod_para._play_interrupt_2 = ptr_vod_data_manager->_play_interrupt_2;
			vod_para._play_interrupt_3 = ptr_vod_data_manager->_play_interrupt_3;
			vod_para._play_interrupt_4 = ptr_vod_data_manager->_play_interrupt_4;
			vod_para._play_interrupt_5 = ptr_vod_data_manager->_play_interrupt_5;
			vod_para._play_interrupt_6 = ptr_vod_data_manager->_play_interrupt_6;
			vod_para._cdn_stop_times = ptr_vod_data_manager->_cdn_stop_times;
			LOG_DEBUG("vdm_reporter begin");
			LOG_DEBUG("vdm_reporter vod_para._vod_play_begin_time = %d", ptr_vod_data_manager->_vod_play_begin_time);
			LOG_DEBUG("vdm_reporter vod_para._vod_first_buffer_time_len = %d", ptr_vod_data_manager->_vod_first_buffer_time_len);
			LOG_DEBUG("vdm_reporter vod_para._vod_play_drag_num = %d", ptr_vod_data_manager->_vod_play_drag_num);
			LOG_DEBUG("vdm_reporter vod_para._vod_interrupt_times = %d", ptr_vod_data_manager->_vod_play_interrupt_times);
			LOG_DEBUG("vdm_reporter vod_para._vod_play_total_buffer_time_len = %d", ptr_vod_data_manager->_vod_play_total_buffer_time_len);
			LOG_DEBUG("vdm_reporter vod_para._vod_play_total_drag_wait_time = %d", ptr_vod_data_manager->_vod_play_total_drag_wait_time);
			LOG_DEBUG("vdm_reporter vod_para._bit_per_seconds = %d", ptr_vod_data_manager->_bit_per_seconds);
			LOG_DEBUG("vdm_reporter vod_para._vod_play_total_buffer_time_len = %d", ptr_vod_data_manager->_vod_play_total_buffer_time_len);
			LOG_DEBUG("vdm_reporter vod_para._vod_play_total_drag_wait_time = %d", ptr_vod_data_manager->_vod_play_total_drag_wait_time);
			LOG_DEBUG("vdm_reporter vod_para._vod_play_drag_num = %d", ptr_vod_data_manager->_vod_play_drag_num);
			LOG_DEBUG("vdm_reporter vod_para._cdn_stop_times = %d", ptr_vod_data_manager->_cdn_stop_times);
			LOG_DEBUG("vdm_reporter vod_para._play_interrupt_1 = %d", ptr_vod_data_manager->_play_interrupt_1);
			LOG_DEBUG("vdm_reporter vod_para._play_interrupt_2 = %d", ptr_vod_data_manager->_play_interrupt_2);
			LOG_DEBUG("vdm_reporter vod_para._play_interrupt_3 = %d", ptr_vod_data_manager->_play_interrupt_3);
			LOG_DEBUG("vdm_reporter vod_para._play_interrupt_4 = %d", ptr_vod_data_manager->_play_interrupt_4);
			LOG_DEBUG("vdm_reporter vod_para._play_interrupt_5 = %d", ptr_vod_data_manager->_play_interrupt_5);
			LOG_DEBUG("vdm_reporter vod_para._play_interrupt_6 = %d", ptr_vod_data_manager->_play_interrupt_6);
			LOG_DEBUG("vdm_reporter end");
			/*
					  emb_reporter_common_stop_task(ptr_task, ptr_vod_data_manager->_vod_times
					  , TIME_SUBZ(end_time,ptr_vod_data_manager->_vod_play_time)
					  , TIME_SUBZ(ptr_vod_data_manager->_vod_first_buffer_time,ptr_vod_data_manager->_vod_play_time)
					  , ptr_vod_data_manager->_vod_interrupt_times
					  , ptr_vod_data_manager->_vod_failed_times
					  );
					  */
			emb_reporter_common_stop_task(ptr_task, &vod_para);


		}
#endif
		vdm_uninit_data_manager(ptr_vod_data_manager, keep_data);
		if (ptr_task)
		{
			dm_set_vod_mode(&((P2SP_TASK*)ptr_task)->_data_manager, FALSE);
			/* 启用其他任务的下载模式 */
			tm_pause_download_except_task(ptr_task, FALSE);
		}
	}
#ifdef ENABLE_BT
	else if (BT_TASK_TYPE == ptr_task->_task_type)
	{

		LOG_DEBUG("vdm_vod_stop_task vod_data_manager_list .");
		while (cur_node != LIST_END(ptr_vod_data_manager_list->_data_list))
		{
			if (NULL == cur_node)
				break;
			ptr_vod_data_manager = (VOD_DATA_MANAGER*)LIST_VALUE(cur_node);
			LOG_DEBUG("vdm_uninit_vod_data_manager_list .get ptr %08X", ptr_vod_data_manager);
			if (ptr_task == ptr_vod_data_manager->_task_ptr)
			{
#ifdef EMBED_REPORT

				sd_time(&end_time);
				//vod_para._vod_play_time = ptr_vod_data_manager->_vod_play_times;
				vod_para._vod_play_time = TIME_SUBZ(end_time, ptr_vod_data_manager->_vod_play_begin_time);
				vod_para._vod_first_buffer_time = ptr_vod_data_manager->_vod_first_buffer_time_len;
				vod_para._vod_interrupt_times = ptr_vod_data_manager->_vod_play_interrupt_times;
				vod_para._min_interrupt_time = ptr_vod_data_manager->_vod_play_min_buffer_time_len;
				vod_para._max_interrupt_time = ptr_vod_data_manager->_vod_play_max_buffer_time_len;
				if (ptr_vod_data_manager->_vod_play_interrupt_times > 0)
				{
					vod_para._avg_interrupt_time = ptr_vod_data_manager->_vod_play_total_buffer_time_len / ptr_vod_data_manager->_vod_play_interrupt_times;
				}
				else
				{
					vod_para._avg_interrupt_time = 0;
				}

				vod_para._bit_rate = ptr_vod_data_manager->_bit_per_seconds;
				vod_para._vod_total_buffer_time = ptr_vod_data_manager->_vod_play_total_buffer_time_len;
				vod_para._vod_total_drag_wait_time = ptr_vod_data_manager->_vod_play_total_drag_wait_time;
				vod_para._vod_drag_num = ptr_vod_data_manager->_vod_play_drag_num;
				vod_para._play_interrupt_1 = ptr_vod_data_manager->_play_interrupt_1;
				vod_para._play_interrupt_2 = ptr_vod_data_manager->_play_interrupt_2;
				vod_para._play_interrupt_3 = ptr_vod_data_manager->_play_interrupt_3;
				vod_para._play_interrupt_4 = ptr_vod_data_manager->_play_interrupt_4;
				vod_para._play_interrupt_5 = ptr_vod_data_manager->_play_interrupt_5;
				vod_para._play_interrupt_6 = ptr_vod_data_manager->_play_interrupt_6;
				vod_para._cdn_stop_times = ptr_vod_data_manager->_cdn_stop_times;
				LOG_DEBUG("vdm_reporter begin");
				LOG_DEBUG("vdm_reporter vod_para._vod_play_begin_time = %d", ptr_vod_data_manager->_vod_play_begin_time);
				LOG_DEBUG("vdm_reporter vod_para._vod_first_buffer_time_len = %d", ptr_vod_data_manager->_vod_first_buffer_time_len);
				LOG_DEBUG("vdm_reporter vod_para._vod_play_drag_num = %d", ptr_vod_data_manager->_vod_play_drag_num);
				LOG_DEBUG("vdm_reporter vod_para._vod_interrupt_times = %d", ptr_vod_data_manager->_vod_play_interrupt_times);
				LOG_DEBUG("vdm_reporter vod_para._vod_play_total_buffer_time_len = %d", ptr_vod_data_manager->_vod_play_total_buffer_time_len);
				LOG_DEBUG("vdm_reporter vod_para._vod_play_total_drag_wait_time = %d", ptr_vod_data_manager->_vod_play_total_drag_wait_time);
				LOG_DEBUG("vdm_reporter vod_para._bit_per_seconds = %d", ptr_vod_data_manager->_bit_per_seconds);
				LOG_DEBUG("vdm_reporter vod_para._vod_play_total_buffer_time_len = %d", ptr_vod_data_manager->_vod_play_total_buffer_time_len);
				LOG_DEBUG("vdm_reporter vod_para._vod_play_total_drag_wait_time = %d", ptr_vod_data_manager->_vod_play_total_drag_wait_time);
				LOG_DEBUG("vdm_reporter vod_para._vod_play_drag_num = %d", ptr_vod_data_manager->_vod_play_drag_num);
				LOG_DEBUG("vdm_reporter vod_para._cdn_stop_times = %d", ptr_vod_data_manager->_cdn_stop_times);
				LOG_DEBUG("vdm_reporter vod_para._play_interrupt_1 = %d", ptr_vod_data_manager->_play_interrupt_1);
				LOG_DEBUG("vdm_reporter vod_para._play_interrupt_2 = %d", ptr_vod_data_manager->_play_interrupt_2);
				LOG_DEBUG("vdm_reporter vod_para._play_interrupt_3 = %d", ptr_vod_data_manager->_play_interrupt_3);
				LOG_DEBUG("vdm_reporter vod_para._play_interrupt_4 = %d", ptr_vod_data_manager->_play_interrupt_4);
				LOG_DEBUG("vdm_reporter vod_para._play_interrupt_5 = %d", ptr_vod_data_manager->_play_interrupt_5);
				LOG_DEBUG("vdm_reporter vod_para._play_interrupt_6 = %d", ptr_vod_data_manager->_play_interrupt_6);
				LOG_DEBUG("vdm_reporter end");

				emb_reporter_bt_stop_file(ptr_task, ptr_vod_data_manager->_file_index, &vod_para, TRUE);
				/*
				emb_reporter_bt_stop_file(ptr_task
				, ptr_vod_data_manager->_file_index
				, ptr_vod_data_manager->_vod_times
				, TIME_SUBZ(end_time,ptr_vod_data_manager->_vod_play_time)
				, TIME_SUBZ(ptr_vod_data_manager->_vod_first_buffer_time,ptr_vod_data_manager->_vod_play_time)
				, ptr_vod_data_manager->_vod_interrupt_times
				, ptr_vod_data_manager->_vod_failed_times
				);
				*/

#endif
				vdm_uninit_data_manager(ptr_vod_data_manager, FALSE);

				bdm_set_vod_mode(&((BT_TASK*)ptr_task)->_data_manager, FALSE);
			}
			cur_node = LIST_NEXT(cur_node);
		}
	}
#endif
	if (ptr_task)
	{
		sd_time(&ptr_task->_stop_vod_time);
	}
	return SUCCESS;

}

_int32 vdm_set_data_change_notify(void *args)
{
	VOD_DATA_MANAGER_READ *param = (VOD_DATA_MANAGER_READ *)args;

	VOD_DATA_MANAGER *vdm = NULL;
	TASK *task = NULL;
	_u32 ret = tm_get_task_by_id(param->_task_id, &task);
	if (ret != SUCCESS || task == NULL)
	{
		return ret;
	}

	ret = vdm_get_vod_data_manager_by_task_id(&g_vdm_list, param->_task_id, 0, &vdm, FALSE, TRUE);
	if (SUCCESS != ret || NULL == vdm)
	{
		ret = vdm_get_free_vod_data_manager(task, 0, &vdm);
		if (ret != SUCCESS || vdm == NULL)
		{
			return ret;
		}
		ret = vdm_get_vod_data_manager_by_task_id(&g_vdm_list, param->_task_id, 0, &vdm, TRUE, FALSE);
		if (SUCCESS != ret || NULL == vdm)
		{
			return ERR_VOD_DATA_MANAGER_NOT_FOUND;
		}

		vdm_vod_malloc_vod_buffer();

		if (P2SP_TASK_TYPE == task->_task_type)
		{
#ifdef ENABLE_VOD
			dm_set_vod_mode(&((P2SP_TASK *)task)->_data_manager, TRUE);
#ifdef ENABLE_CDN
			pt_set_cdn_mode(task, TRUE);
#endif
#endif
		}
#ifdef ENABLE_BT
		else if (BT_TASK_TYPE == task->_task_type)
		{
			bdm_set_vod_mode(&((BT_TASK *)task)->_data_manager, TRUE);
		}
#endif
		else
		{
			LOG_DEBUG("vdm_vod_async_read_file .unknown task type ");
			return ERR_VOD_DATA_UNKNOWN_TASK;
		}
		/* 同一时间只允许一个点播任务存在 */
		_u32 other_task_id = 0;
		ret = vdm_get_the_other_vod_task(&g_vdm_list, param->_task_id, &other_task_id);
		{
			sd_assert(FALSE);
			vdm_vod_stop_task(other_task_id, FALSE);
		}
	}
	vdm->_can_read_data_notify = ((NotifyCanReadDataChange)(param->_file_index));
	vdm->_can_read_data_notify_args = (void*)param->_blocktime;

	return signal_sevent_handle(&(param->_handle));
}

_int32 vdm_vod_set_buffer_time(_int32 buffer_time)
{

	LOG_DEBUG("vdm_vod_set_buffer_time");
	return vdm_set_buffer_time_len(buffer_time);
}

_int32 vdm_vod_get_buffer_percent(_int32 task_id, _int32* percent)
{
	_int32 ret;
	TASK* taskinfo;
	TASK* ptr_task;
	_int32  file_index;
	_u64    continuing_end_pos;
	_u64    filesize;
	_u32    bytes_per_seconds;

	VOD_DATA_MANAGER* ptr_vod_data_manager;
	file_index = 0;
	*percent = 0;
	filesize = 0;
	ptr_vod_data_manager = NULL;

	LOG_DEBUG("vdm_vod_get_buffer_percent .taskid=%d, file_index=%d", task_id, file_index);
	ret = tm_get_task_by_id(task_id, &taskinfo);
	if (SUCCESS != ret)
	{
		LOG_DEBUG("vdm_vod_get_buffer_percent .tm_get_task_by_id return = %d", ret);
		return ret;
	}
	/*get DOWNLOAD_TASK ptr here*/
	ptr_task = (TASK*)taskinfo;
	//判斷任務狀態
	if (TASK_S_FAILED == ptr_task->task_status)
	{
		LOG_DEBUG("vdm_vod_get_buffer_percent task_status is failed=%d ", ptr_task->task_status);
		ret = ERR_VOD_DATA_TASK_STOPPED;
		return ret;
	}


	if (P2SP_TASK_TYPE == ptr_task->_task_type)
	{
		ret = vdm_get_vod_data_manager_by_task_id(&g_vdm_list, task_id, file_index, &ptr_vod_data_manager, TRUE, FALSE);
		if (SUCCESS != ret || NULL == ptr_vod_data_manager)
		{
			LOG_DEBUG("vdm_vod_stop_task .vdm_get_vod_data_manager_by_task_ptr return = %d", ret);
			return ret;
		}

	}
#ifdef ENABLE_BT
	else if (BT_TASK_TYPE == ptr_task->_task_type)
	{
		return ERR_VOD_DATA_UNKNOWN_TASK;
	}
#endif
	else
	{
		return ERR_VOD_DATA_UNKNOWN_TASK;
	}

	if (P2SP_TASK_TYPE == ptr_task->_task_type)
	{
		filesize = dm_get_file_size(&((P2SP_TASK*)ptr_task)->_data_manager);
	}
#ifdef ENABLE_BT
	else if (BT_TASK_TYPE == ptr_task->_task_type)
	{
		filesize = bdm_get_sub_file_size(&((BT_TASK*)ptr_task)->_data_manager, ptr_vod_data_manager->_file_index);
	}
#endif
	else
	{
		LOG_DEBUG("vdm_vod_get_buffer_percent .unknown task type ");
		return ERR_VOD_DATA_UNKNOWN_TASK;
	}
	if (filesize == 0)
	{
		LOG_DEBUG("vdm_vod_get_buffer_percent .unknown task filesize 2");
		return ERR_VOD_DATA_UNKNOWN_TASK;
	}


	bytes_per_seconds = vdm_cal_bytes_per_second(filesize, ptr_vod_data_manager->_bit_per_seconds);

	vdm_get_continuing_end_pos(ptr_vod_data_manager->_current_play_pos, filesize, 
		&ptr_vod_data_manager->_range_buffer_list, &continuing_end_pos);
	*percent = (continuing_end_pos - ptr_vod_data_manager->_current_play_pos) * 100 / (vdm_get_buffer_time_len()*bytes_per_seconds);

	if ((continuing_end_pos == filesize) && (*percent < 100))
	{
		*percent = 100;
	}
	else if (*percent > 100)
	{
		if (P2SP_TASK_TYPE == ptr_vod_data_manager->_task_ptr->_task_type)
		{
			_u64 dm_recved_pos = 0;
			vdm_vod_get_download_position(task_id, &dm_recved_pos);
			if (dm_recved_pos > continuing_end_pos)
			{
				*percent = (dm_recved_pos - ptr_vod_data_manager->_current_play_pos) * 100 / (vdm_get_buffer_time_len()*bytes_per_seconds);
			}
		}
	}
	return SUCCESS;

}

_int32 vdm_vod_get_download_position(_int32 task_id, _u64* position)
{
	_int32 ret = SUCCESS;
	TASK* taskinfo;
	TASK* ptr_task;
	_int32  file_index = INVALID_FILE_INDEX;
	_u64    filesize;

	VOD_DATA_MANAGER* ptr_vod_data_manager = NULL;

	LOG_DEBUG("vdm_vod_get_download_position .taskid=%d", task_id);

	if (vdm_get_dlpos(task_id, position) == SUCCESS)
	{
		return SUCCESS;
	}

	ret = tm_get_task_by_id(task_id, &taskinfo);
	if (SUCCESS != ret)
	{
		LOG_DEBUG("vdm_vod_get_download_position .tm_get_task_by_id return = %d", ret);
		return ret;
	}

	/*get DOWNLOAD_TASK ptr here*/
	ptr_task = (TASK*)taskinfo;
	//判斷任務狀態
	if (TASK_S_FAILED == ptr_task->task_status)
	{
		LOG_DEBUG("vdm_vod_get_download_position task_status is failed=%d ", ptr_task->task_status);
		ret = ERR_VOD_DATA_TASK_STOPPED;
		return ret;
	}

	if (P2SP_TASK_TYPE == ptr_task->_task_type)
	{
		ret = vdm_get_vod_data_manager_by_task_id(&g_vdm_list, task_id, file_index, &ptr_vod_data_manager, TRUE, FALSE);
		if (SUCCESS != ret || NULL == ptr_vod_data_manager)
		{
			LOG_DEBUG("vdm_vod_stop_task .vdm_get_vod_data_manager_by_task_ptr return = %d", ret);
			return ret;
		}

	}
#ifdef ENABLE_BT
	else if (BT_TASK_TYPE == ptr_task->_task_type)
	{
		return ERR_VOD_DATA_UNKNOWN_TASK;
	}
#endif
	else
	{
		return ERR_VOD_DATA_UNKNOWN_TASK;
	}

	if (P2SP_TASK_TYPE == ptr_task->_task_type)
	{
		filesize = dm_get_file_size(&((P2SP_TASK*)ptr_task)->_data_manager);
	}
#ifdef ENABLE_BT
	else if (BT_TASK_TYPE == ptr_task->_task_type)
	{
		filesize = bdm_get_sub_file_size(&((BT_TASK*)ptr_task)->_data_manager, ptr_vod_data_manager->_file_index);
	}
#endif
	else
	{
		LOG_DEBUG("vdm_vod_get_download_position .unknown task type ");
		return ERR_VOD_DATA_UNKNOWN_TASK;
	}
	if (filesize == 0)
	{
		LOG_DEBUG("vdm_vod_get_download_position .unknown task filesize 2");
		return ERR_VOD_DATA_UNKNOWN_TASK;
	}

	if (P2SP_TASK_TYPE == ptr_vod_data_manager->_task_ptr->_task_type)
	{
		RANGE_LIST* range_total_recved = dm_get_recved_range_list(&((P2SP_TASK*)ptr_vod_data_manager->_task_ptr)->_data_manager);
		if (filesize != 0)
		{
			ret = vdm_init_dlpos(task_id, ptr_vod_data_manager->_current_play_pos, filesize, range_total_recved);
			CHECK_VALUE(ret);
		}
		/* 从任务的data_manager中获得已经下载到的数据偏移位置 */
		vdm_get_continuing_end_pos(ptr_vod_data_manager->_current_play_pos, filesize, range_total_recved, position);
	}
	else
	{
		return ERR_VOD_DATA_UNKNOWN_TASK;
	}
	return SUCCESS;
}

_int32 vdm_vod_get_bitrate(_int32 task_id, _u32 file_index, _u32* bitrate)
{
	_int32 ret;
	TASK* taskinfo;
	TASK* ptr_task;
	_int32  file_index2;

	VOD_DATA_MANAGER* ptr_vod_data_manager;
	file_index2 = 0;
	*bitrate = 0;
	ptr_vod_data_manager = NULL;

	LOG_DEBUG("vdm_vod_get_bitrate .taskid=%d, file_index=%d", task_id, file_index2);
	ret = tm_get_task_by_id(task_id, &taskinfo);
	if (SUCCESS != ret)
	{
		LOG_DEBUG("vdm_vod_get_bitrate .tm_get_task_by_id return = %d", ret);
		return ret;
	}
	/*get DOWNLOAD_TASK ptr here*/
	ptr_task = (TASK*)taskinfo;
	//判斷任務狀態
	if (TASK_S_FAILED == ptr_task->task_status)
	{
		LOG_DEBUG("vdm_vod_get_bitrate task_status is failed=%d ", ptr_task->task_status);
		ret = ERR_VOD_DATA_TASK_STOPPED;
		return ret;
	}


	if (P2SP_TASK_TYPE == ptr_task->_task_type)
	{
		ret = vdm_get_vod_data_manager_by_task_id(&g_vdm_list, task_id, file_index2, &ptr_vod_data_manager, TRUE, FALSE);
		if (SUCCESS != ret || NULL == ptr_vod_data_manager)
		{
			LOG_DEBUG("vdm_vod_stop_task .vdm_get_vod_data_manager_by_task_ptr return = %d", ret);
			return ret;
		}

	}
#ifdef ENABLE_BT
	else if (BT_TASK_TYPE == ptr_task->_task_type)
	{
		return ERR_VOD_DATA_UNKNOWN_TASK;
	}
#endif
	else
	{
		return ERR_VOD_DATA_UNKNOWN_TASK;
	}

	if (ptr_vod_data_manager->_bit_per_seconds == 0)
	{
		return ERR_VOD_DATA_BITRATE_NOT_FOUND;
	}

	*bitrate = ptr_vod_data_manager->_bit_per_seconds;

	return SUCCESS;

}
_int32 vdm_vod_report(_int32 task_id, VOD_REPORT* p_report)
{
	_int32 ret;
	TASK* taskinfo;
	TASK* ptr_task;
	_int32  file_index;
	VOD_DATA_MANAGER* ptr_vod_data_manager;


	file_index = 0;

	LOG_DEBUG("vdm_vod_report .taskid=%d, file_index=%d", task_id, file_index);
	ret = tm_get_task_by_id(task_id, &taskinfo);
	if (SUCCESS != ret)
	{
		LOG_DEBUG("vdm_vod_report .tm_get_task_by_id return = %d", ret);
		return ret;
	}
	/*get DOWNLOAD_TASK ptr here*/
	ptr_task = (TASK*)taskinfo;
	if (P2SP_TASK_TYPE == ptr_task->_task_type)
	{
		ret = vdm_get_vod_data_manager_by_task_id(&g_vdm_list, task_id, file_index, &ptr_vod_data_manager, TRUE, FALSE);
		if (SUCCESS != ret || NULL == ptr_vod_data_manager)
		{
			LOG_DEBUG("vdm_vod_report .vdm_get_vod_data_manager_by_task_ptr return = %d", ret);
			return ret;
		}

#ifdef EMBED_REPORT
#ifdef IPAD_KANKAN
		ptr_vod_data_manager->_is_ad_type = p_report->_is_ad_type;
#endif
		//ptr_vod_data_manager->_vod_play_time_len = p_report->_vod_play_time_len;
		//ptr_vod_data_manager->_vod_play_begin_time = p_report->_vod_play_begin_time;
		ptr_vod_data_manager->_vod_play_begin_time = p_report->_vod_play_begin_time;
		ptr_vod_data_manager->_vod_first_buffer_time_len = p_report->_vod_first_buffer_time_len;
		ptr_vod_data_manager->_vod_play_drag_num = p_report->_vod_play_drag_num;
		ptr_vod_data_manager->_vod_play_total_drag_wait_time = p_report->_vod_play_total_drag_wait_time;
		ptr_vod_data_manager->_vod_play_max_drag_wait_time = p_report->_vod_play_max_drag_wait_time;
		ptr_vod_data_manager->_vod_play_min_drag_wait_time = p_report->_vod_play_min_drag_wait_time;
		ptr_vod_data_manager->_vod_play_interrupt_times = p_report->_vod_play_interrupt_times;
		ptr_vod_data_manager->_vod_play_total_buffer_time_len = p_report->_vod_play_total_buffer_time_len;
		ptr_vod_data_manager->_vod_play_max_buffer_time_len = p_report->_vod_play_max_buffer_time_len;
		ptr_vod_data_manager->_vod_play_min_buffer_time_len = p_report->_vod_play_min_buffer_time_len;
		ptr_vod_data_manager->_play_interrupt_1 = p_report->_play_interrupt_1;
		ptr_vod_data_manager->_play_interrupt_2 = p_report->_play_interrupt_2;
		ptr_vod_data_manager->_play_interrupt_3 = p_report->_play_interrupt_3;
		ptr_vod_data_manager->_play_interrupt_4 = p_report->_play_interrupt_4;
		ptr_vod_data_manager->_play_interrupt_5 = p_report->_play_interrupt_5;
		ptr_vod_data_manager->_play_interrupt_6 = p_report->_play_interrupt_6;
#endif

	}
#ifdef ENABLE_BT
	else if (BT_TASK_TYPE == ptr_task->_task_type)
	{
		sd_assert(FALSE);
		return -1;
	}
#endif


	return SUCCESS;
}


BOOL vdm_is_vod_manager_exist(void)
{
	BOOL  bFound = FALSE;
	VOD_DATA_MANAGER* cur_manager = NULL;
	VOD_DATA_MANAGER_LIST* p_manager_list = &g_vdm_list;

	LIST_ITERATOR  cur_node = LIST_BEGIN(p_manager_list->_data_list);


	LOG_DEBUG("vdm_vod_free_vod_buffer .");

	while (cur_node != LIST_END(p_manager_list->_data_list))
	{
		if (NULL == cur_node)
			break;
		cur_manager = (VOD_DATA_MANAGER*)LIST_VALUE(cur_node);

		if (NULL != cur_manager->_task_ptr) //BT任务也只有一个播放session
		{
			bFound = TRUE;
			break;
		}
		cur_node = LIST_NEXT(cur_node);
	}

	LOG_DEBUG("vdm_vod_free_vod_buffer bFound=%d", bFound);
	return bFound;
}

_int32 vdm_vod_free_vod_buffer(void)
{
	//釋放內存，需判斷是否有點播任務在運行，
	//需要沒有任何點播實例才能釋放，并清除整個內存list
	//手工释放

	_int32 ret = SUCCESS;
	BOOL bFound = FALSE;

	bFound = vdm_is_vod_manager_exist();

	if (TRUE == bFound)
	{
		return ERR_VOD_DATA_MANAGER_NOT_EMPTY;
	}

	ret = vdm_reset_data_buffer_pool();
	if (SUCCESS != ret)
	{
		return ret;
	}

	ret = vdm_free_buffer_to_os();

	return ret;
}

_int32 vdm_vod_malloc_vod_buffer(void)
{
	_int32 ret = SUCCESS;
	LOG_DEBUG("vdm_vod_free_vod_buffer ");
	if (FALSE == vdm_is_buffer_alloced())
	{
		ret = vdm_reset_data_buffer_pool();
		if (SUCCESS != ret)
		{
			LOG_DEBUG("vdm_vod_free_vod_buffer  vdm_reset_data_buffer_pool ret =%u", ret);
			return ret;
		}

		ret = vdm_get_buffer_from_os();
	}
	else
	{
		return ERR_VOD_DATA_BUFFER_ALLOCATED;
	}
	return ret;
}

_int32 vdm_stop_vod_handle(void * _param)
{
	VOD_DATA_MANAGER_STOP* _p_param = (VOD_DATA_MANAGER_STOP*)_param;
	_int32 ret;
	TASK * p_task = NULL;

	ret = vdm_vod_stop_task(_p_param->_task_id, TRUE);
	if (ret == SUCCESS)
	{
		ret = tm_get_task_by_id((_u32)_p_param->_task_id, &p_task);
		if (ret == SUCCESS)
		{
#ifdef ENABLE_CDN
			/* 这个任务可以继续下载，不停止,只退出点播模式,所以如果有CDN资源的话,需要置为有效 */
			pt_set_cdn_mode(p_task, TRUE);
#endif
		}
	}
	_p_param->_result = ret;

	LOG_DEBUG("signal_sevent_handle:_result=%d", _p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

#ifdef ENABLE_VOD	
_int32 vdm_set_vod_buffer_time_handle(void * _param)
{
	VOD_DATA_MANAGER_SET_BUFFER_TIME* _p_param = (VOD_DATA_MANAGER_SET_BUFFER_TIME*)_param;
	_int32 ret = vdm_vod_set_buffer_time(_p_param->_buffer_time);
	SAFE_DELETE(_p_param);

	return ret;
}
#endif

_int32 vdm_get_vod_buffer_percent_handle(void * _param)
{
	VOD_DATA_MANAGER_GET_BUFFER_PERCENT* _p_param = (VOD_DATA_MANAGER_GET_BUFFER_PERCENT*)_param;
	_int32 ret = SUCCESS;

#ifdef IPAD_KANKAN
	if(vdm_check_if_syn_read( _p_param->_task_id))
	{
		_p_param->_percent = 100;
	}
	else
#endif
		ret = vdm_vod_get_buffer_percent(_p_param->_task_id, &(_p_param->_percent));
	_p_param->_result = ret;
	LOG_DEBUG("signal_sevent_handle:_result=%d", _p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 vdm_get_vod_download_position_handle(void * _param)
{
	VOD_DATA_MANAGER_GET_DOWNLOAD_POSITION* _p_param = (VOD_DATA_MANAGER_GET_DOWNLOAD_POSITION*)_param;
	_int32 ret = SUCCESS;

#ifdef IPAD_KANKAN
	if(vdm_check_if_syn_read( _p_param->_task_id))
	{
		_p_param->_position = g_ptr_task->file_size;
	}
	else
#endif
		ret = vdm_vod_get_download_position(_p_param->_task_id, &(_p_param->_position));
	_p_param->_result = ret;
	LOG_DEBUG("signal_sevent_handle:_result=%d", _p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

#ifdef ENABLE_VOD	
_int32 vdm_free_vod_buffer_handle(void * _param)
{
	_int32 ret = SUCCESS;
	TM_POST_PARA_0 *p_param = (TM_POST_PARA_0*)_param;
	ret = vdm_vod_free_vod_buffer();
	SAFE_DELETE(p_param);
	return ret;
}
#endif

_int32 vdm_get_vod_buffer_size_handle(void * _param)
{
	VOD_DATA_MANAGER_GET_SET_VOD_BUFFER_SIZE* _p_param = (VOD_DATA_MANAGER_GET_SET_VOD_BUFFER_SIZE*)_param;
	_int32 ret = SUCCESS;
	_int32 buffer_size = 0;

	ret = vdm_get_vod_buffer_size(&buffer_size);
	_p_param->_result = ret;
	_p_param->_buffer_size = buffer_size;
	LOG_DEBUG("signal_sevent_handle:_result=%d", _p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 vdm_set_vod_buffer_size_handle(void * _param)
{
	VOD_DATA_MANAGER_GET_SET_VOD_BUFFER_SIZE* _p_param = (VOD_DATA_MANAGER_GET_SET_VOD_BUFFER_SIZE*)_param;
	_int32 ret = SUCCESS;
	if (TRUE == vdm_is_vod_manager_exist())
	{
		ret = ERR_VOD_DATA_MANAGER_NOT_EMPTY;
	}
	else
	{
		ret = vdm_set_vod_buffer_size(_p_param->_buffer_size);
	}
	SAFE_DELETE(_p_param);

	return SUCCESS;
}

_int32 vdm_is_vod_buffer_allocated_handle(void * _param)
{
	VOD_DATA_MANAGER_IS_VOD_BUFFER_ALLOCATED* _p_param = (VOD_DATA_MANAGER_IS_VOD_BUFFER_ALLOCATED*)_param;
	BOOL b;

	b = vdm_is_buffer_alloced();
	_p_param->_result = SUCCESS;
	_p_param->_allocated = (_int32)b;
	LOG_DEBUG("signal_sevent_handle:_result=%d", _p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 vdm_get_vod_bitrate_handle(void * _param)
{
	VOD_DATA_MANAGER_GET_BITRATE * _p_param = (VOD_DATA_MANAGER_GET_BITRATE*)_param;
	_int32 ret = SUCCESS;

#ifdef IPAD_KANKAN
	if(vdm_check_if_syn_read( _p_param->_task_id))
	{
		_p_param->_bitrate = VDM_DEFAULT_BYTES_RATE*8;
	}
	else
#endif
		ret = vdm_vod_get_bitrate(_p_param->_task_id, _p_param->_file_index, &(_p_param->_bitrate));
	_p_param->_result = ret;
	LOG_DEBUG("signal_sevent_handle:_result=%d", _p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}
_int32 vdm_vod_report_handle(void * _param)
{
	TM_POST_PARA_2 * _p_param = (TM_POST_PARA_2*)_param;
	_int32 ret;
	_u32 task_id = (_u32)_p_param->_para1;
	VOD_REPORT* p_report = (VOD_REPORT*)_p_param->_para2;

#ifdef IPAD_KANKAN
	if(vdm_check_if_syn_read(task_id))
	{
		sd_memcpy(&g_vod_report, p_report,sizeof(VOD_REPORT));
		ret = SUCCESS;
	}
	else
#endif
		ret = vdm_vod_report(task_id, p_report);

	_p_param->_result = ret;
	LOG_DEBUG("signal_sevent_handle:_result=%d", _p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 vdm_vod_is_download_finished(_int32 task_id, BOOL* finished)
{
	TASK* taskinfo;
	_int32 ret = SUCCESS;
	VOD_DATA_MANAGER* ptr_vod_data_manager;
	TASK* ptr_task;
	_int32 file_index = 0;
	_u64   filesize;
	_u64   continuing_end_pos;

	*finished = FALSE;

	ret = tm_get_task_by_id(task_id, &taskinfo);
	if (SUCCESS != ret)
	{
		LOG_DEBUG("vdm_vod_is_download_finished .tm_get_task_by_id return = %d", ret);
		return ret;
	}
	/*get DOWNLOAD_TASK ptr here*/
	ptr_task = (TASK*)taskinfo;
	ret = vdm_get_vod_data_manager_by_task_id(&g_vdm_list, task_id, file_index, &ptr_vod_data_manager, TRUE, FALSE);
	if (SUCCESS != ret)
	{
		LOG_DEBUG("vdm_vod_is_download_finished .vdm_get_vod_data_manager_by_task_ptr return = %d", ret);
		return ret;
	}

	ret = vdm_get_file_size(ptr_vod_data_manager, &filesize);
	if (SUCCESS != ret)
	{
		LOG_DEBUG("vdm_vod_is_download_finished .vdm_get_file_size return = %d", ret);
		return ret;
	}

	ret = vdm_get_continuing_end_pos(ptr_vod_data_manager->_current_play_pos, filesize,
		&ptr_vod_data_manager->_range_buffer_list, &continuing_end_pos);
	if (SUCCESS != ret)
	{
		LOG_DEBUG("vdm_vod_is_download_finished .vdm_get_continuing_end_pos return = %d", ret);
		return ret;
	}

	if (filesize == continuing_end_pos)
	{
		*finished = TRUE;
	}
	LOG_DEBUG("vdm_vod_is_download_finished .return = %d finished=%u", ret, *finished);

	return ret;


}
_int32 vdm_vod_is_download_finished_handle(void * _param)
{
	VOD_DATA_MANAGER_IS_DOWNLOAD_FINISHED* _p_param = (VOD_DATA_MANAGER_IS_DOWNLOAD_FINISHED*)_param;
	_int32 ret = SUCCESS;

#ifdef IPAD_KANKAN
	if(vdm_check_if_syn_read( _p_param->_task_id))
	{
		_p_param->_finished = TRUE;
	}
	else
#endif
		ret = vdm_vod_is_download_finished(_p_param->_task_id, &_p_param->_finished);
	if (SUCCESS != ret)
	{
		_p_param->_result = ret;
		LOG_DEBUG("signal_sevent_handle:_result=%d", _p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}

	_p_param->_result = ret;
	LOG_DEBUG("signal_sevent_handle:_result=%d", _p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}



_int32 vdm_dm_get_data_buffer_by_range(TASK* task_ptr, RANGE* r, RANGE_DATA_BUFFER_LIST* ret_range_buffer)
{

	_int32 ret = SUCCESS;

	RANGE_LIST* range_list;
	_int32 file_index = 0;
	VOD_DATA_MANAGER* ptr_vod_data_manager = NULL;
	VOD_RANGE_DATA_BUFFER* vod_range_data_buffer;
	_int32 i;
	RANGE range;

	LOG_DEBUG("vdm_dm_get_data_buffer_by_range  range(%u, %u)", r->_index, r->_num);
	ret = vdm_get_vod_data_manager_by_task_id(&g_vdm_list, task_ptr->_task_id, file_index, &ptr_vod_data_manager, TRUE, FALSE);
	if (SUCCESS != ret || NULL == ptr_vod_data_manager)
	{
		return ERR_VOD_DATA_MANAGER_NOT_FOUND;
	}

	range_list = &ptr_vod_data_manager->_range_buffer_list;

	if (FALSE == range_list_is_relevant(range_list, r))
	{
		LOG_DEBUG("vdm_sync_data_buffer_to_range_list range(%u, %u) already in bufferlist.", r->_index, r->_num);
		//已经有了
		return ERR_VOD_DATA_BUFFER_NOT_FOUND;
	}


	for (i = 0; i < r->_num; i++)
	{
		range._index = r->_index + i;
		range._num = 1;
		ret = vdm_buffer_list_find(&ptr_vod_data_manager->_buffer_list, &range, &vod_range_data_buffer);
		if (SUCCESS == ret)
		{
			ret = buffer_list_add(ret_range_buffer, &range, vod_range_data_buffer->_range_data_buffer._data_ptr,
				vod_range_data_buffer->_range_data_buffer._data_length, vod_range_data_buffer->_range_data_buffer._buffer_length);
			LOG_DEBUG("vdm_dm_get_data_buffer_by_range  buffer_list_add range(%u, %u) return=%u", range._index, range._num, ret);
		}
	}


	LOG_DEBUG("vdm_dm_get_data_buffer_by_range success.");
	return ret;
}

//只接受r->_num == 1
_int32 vdm_drop_buffer_by_range(VOD_DATA_MANAGER* ptr_vod_data_manager, RANGE* r)
{
	RANGE_LIST range_list;
	VOD_RANGE_DATA_BUFFER* cur_buffer = NULL;
	VOD_RANGE_DATA_BUFFER_LIST*  buffer_list;

	LIST_ITERATOR  cur_node = NULL;
	LIST_ITERATOR  erase_node = NULL;
	_int32 ret;

	buffer_list = &ptr_vod_data_manager->_buffer_list;
	cur_node = LIST_BEGIN(buffer_list->_data_list);
	range_list_init(&range_list);

	LOG_DEBUG("vdm_drop_buffer_by_range . r(%u,%u)", r->_index, r->_num);

	while (cur_node != LIST_END(buffer_list->_data_list))
	{
		range_list_clear(&range_list);
		range_list_init(&range_list);
		range_list_add_range(&range_list, r, NULL, NULL);
		cur_buffer = (VOD_RANGE_DATA_BUFFER*)LIST_VALUE(cur_node);

		if (TRUE == range_list_is_include(&range_list, &cur_buffer->_range_data_buffer._range))
		{
			range_list_delete_range_list(&ptr_vod_data_manager->_range_buffer_list, &range_list);
			if (cur_buffer->_is_recved == 1)
			{
				ret = vdm_free_data_buffer(ptr_vod_data_manager, &cur_buffer->_range_data_buffer._data_ptr,
					cur_buffer->_range_data_buffer._buffer_length);
				LOG_DEBUG("vdm_drop_buffer_by_range done. free range(%u,%u) ret=%u", r->_index, r->_num, ret);
				CHECK_VALUE(ret);
			}

			erase_node = cur_node;
			cur_node = LIST_NEXT(cur_node);
			list_erase(&buffer_list->_data_list, erase_node);

			free_vod_range_data_buffer_node(cur_buffer);
		}
		else
		{
			cur_node = LIST_NEXT(cur_node);
		}
	}

	range_list_clear(&range_list);
	LOG_DEBUG("vdm_drop_buffer_by_range done.");
	return SUCCESS;
}

_int32 vdm_dm_notify_check_error_by_range(TASK* task_ptr, RANGE* r)
{
	_int32 ret = SUCCESS;

	_int32 i;
	RANGE range;
	_int32 file_index = 0;
	VOD_DATA_MANAGER* ptr_vod_data_manager = NULL;

	LOG_DEBUG("vdm_dm_notify_check_error_by_range  range(%u, %u)", r->_index, r->_num);
	ret = vdm_get_vod_data_manager_by_task_id(&g_vdm_list, task_ptr->_task_id, file_index, &ptr_vod_data_manager, TRUE, FALSE);
	if (SUCCESS != ret || NULL == ptr_vod_data_manager)
	{
		return ERR_VOD_DATA_MANAGER_NOT_FOUND;
	}

	for (i = 0; i < r->_num; i++)
	{
		range._index = r->_index + i;
		range._num = 1;
		ret = vdm_drop_buffer_by_range(ptr_vod_data_manager, &range);
		LOG_DEBUG("vdm_dm_notify_check_error_by_range  range(%u, %u) return=%u", range._index, range._num, ret);
	}
	range_list_delete_range(&ptr_vod_data_manager->_range_buffer_list, r, NULL, NULL);

	//设置紧急块
	vdm_period_dispatch(ptr_vod_data_manager, TRUE);

	LOG_DEBUG("vdm_dm_notify_check_error_by_range done.");
	return ret;
}

_int32 vdm_init_dlpos(_u32 task_id, _u64  start_pos, _u64  file_size, RANGE_LIST * range_total_recved)
{
	_int32 i = 0;
	for (i = 0; i < VDM_MAX_DL_POS_NUM; i++)
	{
		if (g_vod_dl_pos[i]._task_id == 0)
		{
			g_vod_dl_pos[i]._task_id = task_id;
			g_vod_dl_pos[i]._start_pos = start_pos;
			g_vod_dl_pos[i]._file_size = file_size;
			g_vod_dl_pos[i]._range_total_recved = range_total_recved;
			return SUCCESS;
		}
	}
	sd_assert(FALSE);
	return -1;
}

_int32 vdm_get_dlpos(_u32 task_id, _u64 *  dl_pos)
{
	_int32 i = 0;
	for (i = 0; i < VDM_MAX_DL_POS_NUM; i++)
	{
		if (g_vod_dl_pos[i]._task_id == task_id)
		{
			/* 从任务的data_manager中获得已经下载到的数据偏移位置 */
			return vdm_get_continuing_end_pos(g_vod_dl_pos[i]._start_pos, g_vod_dl_pos[i]._file_size,
				g_vod_dl_pos[i]._range_total_recved, dl_pos);
		}
	}

	return -1;
}
_int32 vdm_update_dlpos(_u32 task_id, _u64  start_pos)
{
	_int32 i = 0;
	for (i = 0; i < VDM_MAX_DL_POS_NUM; i++)
	{
		if (g_vod_dl_pos[i]._task_id == task_id)
		{
			g_vod_dl_pos[i]._start_pos = start_pos;
			return SUCCESS;
		}
	}

	return -1;
}

_int32 vdm_uninit_dlpos(_u32 task_id)
{
	_int32 i = 0;
	for (i = 0; i < VDM_MAX_DL_POS_NUM; i++)
	{
		if (g_vod_dl_pos[i]._task_id == task_id)
		{
			g_vod_dl_pos[i]._task_id = 0;
			g_vod_dl_pos[i]._start_pos = 0;
			g_vod_dl_pos[i]._file_size = 0;
			g_vod_dl_pos[i]._range_total_recved = NULL;
			return SUCCESS;
		}
	}
	//sd_assert(FALSE);
	return -1;
}

_int32 vdm_get_vod_task_num(void)
{
	_int32 i = 0, task_num = 0;
	for (i = 0; i < VDM_MAX_DL_POS_NUM; i++)
	{
		if (g_vod_dl_pos[i]._task_id != 0)
		{
			task_num++;
		}
	}

	return task_num;
}
/* 同一时间允许一个点播任务存在! */
_u32 vdm_get_current_vod_task_id(void)
{
	_int32 i = 0;
	for (i = 0; i < VDM_MAX_DL_POS_NUM; i++)
	{
		if (g_vod_dl_pos[i]._task_id != 0)
		{
			return g_vod_dl_pos[i]._task_id;
		}
	}

#ifdef IPAD_KANKAN
	if(g_current_vod_task_id != 0)
	{
		return g_current_vod_task_id;
	}
#endif
	return 0;
}


_int32     vdm_get_flv_header_to_first_tag(_u32 task_id, _u32 file_index, char ** pp_buffer, _u64 * data_len)
{
#ifdef ENABLE_FLASH_PLAYER
	_int32 ret_val = SUCCESS;
	TASK* task = NULL;
	VOD_DATA_MANAGER* ptr_vod_data_manager;
	_u64 pos = 0;

	LOG_DEBUG("vdm_get_flv_first_tag_end_pos .taskid=%d, file_index=%d", task_id, file_index );

	*data_len = 0;

	if(vdm_get_current_vod_task_id()!=task_id)
	{
		CHECK_VALUE(-1);
	}

	ret_val = tm_get_task_by_id(task_id,&task);
	CHECK_VALUE(ret_val);

	/*get DOWNLOAD_TASK ptr here*/
	if(P2SP_TASK_TYPE == task->_task_type)
	{
		ret_val = vdm_get_vod_data_manager_by_task_id(&g_vdm_list, task_id, file_index, &ptr_vod_data_manager, TRUE, FALSE);
		if(SUCCESS != ret_val || NULL == ptr_vod_data_manager)
		{
			LOG_DEBUG("vdm_get_flv_first_tag_end_pos .vdm_get_vod_data_manager_by_task_ptr return = %d", ret_val);
			CHECK_VALUE(-1);
		}

		pos = ptr_vod_data_manager->_flv_header_to_end_of_first_tag_len;
	}
	else
	{
		CHECK_VALUE(-1);
	}
	LOG_DEBUG("vdm_get_flv_first_tag_end_pos . start_pos=%llu,   end_pos=%llu", pos);
	/////////////////////////////////////////////////////////
	if(pos!=0)
	{
		RANGE  range;

		range = pos_length_to_range(0, pos, task->file_size);

		if( TRUE == range_list_is_include(&ptr_vod_data_manager->_range_buffer_list, &range))
		{

			ret_val = sd_malloc(get_data_unit_size()*range._num, (void **)pp_buffer);
			CHECK_VALUE(ret_val);

			ret_val =  vdm_write_data_buffer(ptr_vod_data_manager, 0, *pp_buffer, 
				get_data_unit_size()*range._num, &range, &ptr_vod_data_manager->_buffer_list);
			if(ret_val!=SUCCESS)
			{
				sd_free(*pp_buffer);
				*pp_buffer = NULL;
				CHECK_VALUE(ret_val);
			}

			*data_len = pos;
		}
		else
		{
			/* Service Unavailable */
			sd_assert(FALSE);
			CHECK_VALUE(503);
		}
	}
	else
	{
		/* Service Unavailable */
		CHECK_VALUE(503);
	}

#endif /* ENABLE_FLASH_PLAYER */
	return SUCCESS;
}


