#include "utility/define.h"
#ifdef ENABLE_BT 
#include "bt_tmp_file.h"
#include "bt_range_switch.h"
#include "../bt_data_pipe/bt_memory_slab.h"
#include "data_manager/file_manager.h"
#include "data_manager/data_buffer.h"
#include "platform/sd_fs.h"
#include "utility/mempool.h"
#include "utility/utility.h"
#include "utility/logid.h"
#define  LOGID	LOGID_BT_DOWNLOAD
#include "utility/logger.h"

_int32 range_change_node_comparator(void *E1, void *E2)
{
	RANGE_CHANGE_NODE* left = (RANGE_CHANGE_NODE*)E1;
	RANGE_CHANGE_NODE* right = (RANGE_CHANGE_NODE*)E2;
	if(left->_padding_range._index > right->_padding_range._index)
		return 1;
	else if(left->_padding_range._index < right->_padding_range._index)
		return -1;
	else
	{
#ifdef _DEBUG
		if(left->_padding_range._num != right->_padding_range._num)
		{
			LOG_DEBUG("left->_padding_range._num = %u, right->_padding_range._num = %u", left->_padding_range._num, right->_padding_range._num);
			sd_assert(FALSE);
		}
#endif
		return 0;
	}
}

_int32 bt_init_tmp_file(BT_TMP_FILE* tmp_file, char* tmp_file_path, char* tmp_file_name)
{
	MAP_ITERATOR cur_iter, next_iter;
	RANGE_CHANGE_NODE* data = NULL;
	LOG_DEBUG("[tmp_file = 0x%x]bt_init_tmp_file.", tmp_file);
	sd_memset(tmp_file, 0 , sizeof(BT_TMP_FILE));
	range_list_init(&tmp_file->_tmp_padding_range_list);
	range_list_init(&tmp_file->_valid_padding_range_list);	
	set_init(&tmp_file->_range_change_set, range_change_node_comparator);
	tmp_file->_base_file = NULL;
	list_init(&tmp_file->_read_request_list);
	list_init(&tmp_file->_write_request_list);
	buffer_list_init(&tmp_file->_write_data_buffer_list);
	tmp_file->_cur_read_req = NULL;
	tmp_file->_cur_write_req = NULL;
	tmp_file->_state = IDLE_STATE;
	tmp_file->_total_range_num = 0;
	sd_memcpy(tmp_file->_file_path, tmp_file_path, MIN(sd_strlen(tmp_file_path), MAX_FILE_PATH_LEN - 1));
	sd_memcpy(tmp_file->_file_name, tmp_file_name, MIN(sd_strlen(tmp_file_name), MAX_FILE_NAME_LEN - 1));
	tmp_file->_del_flag = FALSE;
	if(bt_tmp_file_load_from_cfg_file(tmp_file) != SUCCESS)
	{
		LOG_DEBUG("bt_tmp_file_load_from_cfg_file failed.");
		range_list_clear(&tmp_file->_tmp_padding_range_list);
		range_list_clear(&tmp_file->_valid_padding_range_list);	
		tmp_file->_total_range_num = 0;
		cur_iter = SET_BEGIN(tmp_file->_range_change_set);
		while(cur_iter != SET_END(tmp_file->_range_change_set))
		{
			next_iter = SET_NEXT(tmp_file->_range_change_set, cur_iter);
			data = (RANGE_CHANGE_NODE*)(cur_iter->_data);
			bt_free_range_change_node(data);
			set_erase_iterator(&tmp_file->_range_change_set, cur_iter);
			cur_iter = next_iter;
		}
	}
	return SUCCESS;
}

_int32 bt_uninit_tmp_file(BT_TMP_FILE* tmp_file)
{
	char file_name[MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN] = {0};
	MAP_ITERATOR cur_iter, next_iter;
	RANGE_CHANGE_NODE* data;
	BT_TMP_FILE_READ_REQ* read_req = NULL;
	BT_TMP_FILE_WRITE_REQ* write_req = NULL;
	LOG_DEBUG("[tmp_file = 0x%x]bt_uninit_tmp_file.", tmp_file);
	if(tmp_file->_del_flag == FALSE)
	{
		bt_tmp_file_save_to_cfg_file(tmp_file);
	}
	else
	{
		sd_snprintf(file_name, MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN, "%s%s.cfg", tmp_file->_file_path, tmp_file->_file_name);
		sd_delete_file(file_name);
	}
	range_list_clear(&tmp_file->_tmp_padding_range_list);
	range_list_clear(&tmp_file->_valid_padding_range_list);
	cur_iter = SET_BEGIN(tmp_file->_range_change_set);
	while(cur_iter != SET_END(tmp_file->_range_change_set))
	{
		next_iter = SET_NEXT(tmp_file->_range_change_set, cur_iter);
		data = (RANGE_CHANGE_NODE*)(cur_iter->_data);
		bt_free_range_change_node(data);
		set_erase_iterator(&tmp_file->_range_change_set, cur_iter);
		cur_iter = next_iter;
	}
	while(list_size(&tmp_file->_read_request_list) > 0)
	{
		list_pop(&tmp_file->_read_request_list, (void**)&read_req);
		sd_free(read_req->_data_buffer->_data_ptr);
		read_req->_data_buffer->_data_ptr = NULL;
		sd_free(read_req);
		read_req = NULL;
	}
	while(list_size(&tmp_file->_write_request_list) > 0)
	{
		list_pop(&tmp_file->_write_request_list, (void**)&write_req);
		dm_free_buffer_to_data_buffer(write_req->_buffer_len, &write_req->_buffer);
		write_req->_buffer = NULL;
		sd_free(write_req);
		write_req = NULL;
	}
	if(tmp_file->_base_file != NULL)
	{
		fm_close(tmp_file->_base_file, notify_bt_tmp_file_close, (void*)tmp_file->_del_flag);
		tmp_file->_base_file = NULL;
		tmp_file->_state = UNINIT_STATE;
	}
	return SUCCESS;
}

_int32 bt_tmp_file_set_del_flag(BT_TMP_FILE* tmp_file, BOOL flag)
{
	tmp_file->_del_flag = flag;
	return SUCCESS;
}


_int32 bt_add_tmp_file(BT_TMP_FILE* tmp_file, _u32 file_index, struct tagBT_RANGE_SWITCH* range_switch)
{
	_u32 piece_index = 0;
	_u64 start_pos = 0, end_pos = 0;
	FILE_RANGE_INFO* file_info = NULL;
	BT_RANGE bt_range;
	RANGE padding_range;
	file_info = range_switch->_file_range_info_array + file_index;		//获得需要下载的文件信息
	if(file_index != 0)		//第一个文件不需要计算头部位置
	{
		//计算文件起始位置所在的piece，并把该piece转为padding_range保存起来
		start_pos = file_info->_file_offset;		//文件的起始位置
		piece_index = start_pos / range_switch->_piece_size;
		bt_range._pos = (_u64)range_switch->_piece_size * piece_index;
		bt_range._length = range_switch->_piece_size;
		if(piece_index == range_switch->_piece_num - 1)
		{
			bt_range._length = range_switch->_file_size - (_u64)range_switch->_piece_size * piece_index;
		}
		brs_bt_range_to_extra_padding_range(range_switch, &bt_range, &padding_range);
		range_list_add_range(&tmp_file->_tmp_padding_range_list, &padding_range, NULL, NULL);
	}
	//计算文件结束位置所在的piece,并把该piece转为padding_range保存起来
	if(file_index != range_switch->_file_num - 1)	//最后一个文件不需要计算尾部位置
	{
		end_pos = file_info->_file_offset + file_info->_file_size;
		piece_index = end_pos / range_switch->_piece_size;
		bt_range._pos = (_u64)range_switch->_piece_size * piece_index;
		bt_range._length = range_switch->_piece_size;
		if(piece_index == range_switch->_piece_num - 1)
		{
			bt_range._length = range_switch->_file_size - (_u64)range_switch->_piece_size * piece_index;
		}
		brs_bt_range_to_extra_padding_range(range_switch, &bt_range, &padding_range);
		range_list_add_range(&tmp_file->_tmp_padding_range_list, &padding_range, NULL, NULL);
	}

#ifdef _DEBUG
	LOG_DEBUG("bt_add_tmp_file, file_index = %u, start_pos = %llu, end_pos = %llu, tmp_padding_range_list is : ", file_index, start_pos, end_pos);
	out_put_range_list(&tmp_file->_tmp_padding_range_list);
#endif

	return SUCCESS;
}

_int32 bt_get_tmp_file_need_donwload_range_list(BT_TMP_FILE* tmp_file, RANGE_LIST* tmp_file_need_download_range_list)
{
	LIST_ITERATOR iter;
	BT_TMP_FILE_WRITE_REQ* req = NULL;
	range_list_clear(tmp_file_need_download_range_list);
	range_list_add_range_list(tmp_file_need_download_range_list, &tmp_file->_tmp_padding_range_list);
	//减去已经写到磁盘中的有效的padding_range
	range_list_delete_range_list(tmp_file_need_download_range_list, &tmp_file->_valid_padding_range_list);
	//减掉正在写的padding_range
	for(iter = LIST_BEGIN(tmp_file->_write_request_list); iter != LIST_END(tmp_file->_write_request_list); iter = LIST_NEXT(iter))
	{
		req = (BT_TMP_FILE_WRITE_REQ*)LIST_VALUE(iter);
		range_list_delete_range(tmp_file_need_download_range_list, &req->_pending_range, NULL, NULL);
	}
	if(tmp_file->_cur_write_req != NULL)
		range_list_delete_range(tmp_file_need_download_range_list, &tmp_file->_cur_write_req->_pending_range, NULL, NULL);
	return SUCCESS;
}

_int32 bt_tmp_file_get_valid_range_list(BT_TMP_FILE* tmp_file, RANGE_LIST* valid_range_list)
{
	range_list_clear(valid_range_list);
	range_list_add_range_list(valid_range_list, &tmp_file->_valid_padding_range_list);
	return SUCCESS;
}

BOOL bt_is_tmp_file_range(BT_TMP_FILE* tmp_file, const RANGE* padding_range)
{
	return range_list_is_include(&tmp_file->_tmp_padding_range_list, padding_range);
}

//临时文件中某个padding_range是否已经有效，即写在磁盘中
BOOL bt_is_tmp_range_valid(BT_TMP_FILE* tmp_file, const RANGE* padding_range)
{
	return range_list_is_include(&tmp_file->_valid_padding_range_list, padding_range);
}

_int32 bt_read_tmp_file(BT_TMP_FILE* tmp_file, const RANGE* padding_range, RANGE_DATA_BUFFER* data_buffer, notify_read_result callback, void* user_data)
{
	_int32 ret = SUCCESS;
	BT_TMP_FILE_READ_REQ* req = NULL;
	LOG_DEBUG("bt_read_tmp_file, padding_range[%u, %u].", padding_range->_index, padding_range->_num);
	sd_assert(padding_range->_num == 1);
	if(range_list_is_include(&tmp_file->_valid_padding_range_list, padding_range) == FALSE)
	{
		LOG_DEBUG("bt_read_tmp_file, but padding_range[%u, %u] is not in valid_padding_range_list ", padding_range->_index, padding_range->_num);
		return -1;		//临时文件中并没有要读的这一块
	}
	if(data_buffer->_buffer_length < get_data_unit_size())		
	{	//至少也得一个range的大小
		sd_assert(FALSE);
		return -1;
	}
	//创建一个请求命令
	ret = sd_malloc(sizeof(BT_TMP_FILE_READ_REQ), (void**)&req);
	CHECK_VALUE(ret);
	req->_padding_range._index = padding_range->_index;
	req->_padding_range._num = padding_range->_num;
	req->_data_buffer = data_buffer;
	req->_callback = callback;
	req->_user_data = user_data;
	list_push(&tmp_file->_read_request_list, req);
	return bt_update_tmp_file(tmp_file);		//触发一下状态机
}

_int32 bt_write_tmp_file(BT_TMP_FILE* tmp_file,const RANGE* padding_range, char* buffer, _u32 buffer_len, _u32 data_len, bt_tmp_file_write_callback callback, void* user_data)
{
	_int32 ret = SUCCESS;
	BT_TMP_FILE_WRITE_REQ* req = NULL;
	LOG_DEBUG("bt_write_tmp_file, padding_range[%u, %u].", padding_range->_index, padding_range->_num);
	sd_assert(padding_range->_num == 1);
	if(range_list_is_include(&tmp_file->_tmp_padding_range_list, padding_range) == FALSE)
	{
		sd_assert(FALSE);
		return -1;		//这并不是一个临时文件的pending_range
	}

#ifdef _DEBUG
	if(range_list_is_include(&tmp_file->_valid_padding_range_list, padding_range) == TRUE)
	{
		LOG_DEBUG("bt_write_tmp_file, but padding_range[%u, %u] had recv, write this range again.", padding_range->_index, padding_range->_num);	//要写的这一块已经在临时文件中
	}
#endif

	//创建一个请求命令
	ret = sd_malloc(sizeof(BT_TMP_FILE_WRITE_REQ), (void**)&req);
	CHECK_VALUE(ret);
	req->_pending_range._index = padding_range->_index;
	req->_pending_range._num = padding_range->_num;
	ret = dm_get_buffer_from_data_buffer(&data_len, &req->_buffer);
	if(ret != SUCCESS)
	{
		sd_free(req);
		req = NULL;
		return ret;
	}
	sd_memcpy(req->_buffer, buffer, data_len);
	req->_buffer_len = data_len;
	req->_callback = callback;
	req->_user_data = user_data;
	list_push(&tmp_file->_write_request_list, req);
	return bt_update_tmp_file(tmp_file);		//触发一下状态机
}

_int32 bt_send_read_request(BT_TMP_FILE* tmp_file)
{
	MAP_ITERATOR iter;
	RANGE_CHANGE_NODE* data = NULL;
	sd_assert(tmp_file->_state == READ_WRITE_STATE);
	sd_assert(tmp_file->_cur_read_req == NULL);
	sd_assert(list_size(&tmp_file->_read_request_list) > 0);
	list_pop(&tmp_file->_read_request_list, (void**)&tmp_file->_cur_read_req);
	set_find_iterator(&tmp_file->_range_change_set, &tmp_file->_cur_read_req->_padding_range, &iter);
	sd_assert(iter != SET_END(tmp_file->_range_change_set));	//一定找得到
	data = (RANGE_CHANGE_NODE*)iter->_data;
	tmp_file->_read_data_buffer._range._index = data->_file_range_index;
	tmp_file->_read_data_buffer._range._num = 1;
	tmp_file->_read_data_buffer._data_ptr = tmp_file->_cur_read_req->_data_buffer->_data_ptr;
	tmp_file->_read_data_buffer._buffer_length = get_data_unit_size();
	tmp_file->_read_data_buffer._data_length = get_data_unit_size();
	return fm_file_asyn_read_buffer(tmp_file->_base_file, &tmp_file->_read_data_buffer, notify_bt_tmp_file_read, tmp_file);
}

_int32 bt_send_write_request(BT_TMP_FILE* tmp_file)
{
	RANGE_CHANGE_NODE* data = NULL;
	RANGE file_range;
	sd_assert(tmp_file->_state == READ_WRITE_STATE);
	sd_assert(tmp_file->_cur_write_req == NULL);
	sd_assert(list_size(&tmp_file->_write_request_list) > 0);
	list_pop(&tmp_file->_write_request_list, (void**)&tmp_file->_cur_write_req);
	set_find_node(&tmp_file->_range_change_set, &tmp_file->_cur_write_req->_pending_range, (void**)&data);
	if(data == NULL)
		file_range._index = tmp_file->_total_range_num;
	else
		file_range._index = data->_file_range_index;
	file_range._num = 1;
	//这里写文件的时候一定要把data_len规整为64K的整数倍，因为上层可能看到的数据长度不是64K整数倍，但下层的临时文件大小是64K整数倍，不规整的话会导致底层断言失败
	buffer_list_add(&tmp_file->_write_data_buffer_list, &file_range, tmp_file->_cur_write_req->_buffer, tmp_file->_cur_write_req->_buffer_len, tmp_file->_cur_write_req->_buffer_len);
	fm_init_file_info(tmp_file->_base_file, get_data_unit_size() * (_u64)tmp_file->_total_range_num, 0);
	return fm_file_write_buffer_list(tmp_file->_base_file, &tmp_file->_write_data_buffer_list, notify_bt_tmp_file_write, tmp_file);
}

_int32 notify_bt_tmp_file_read(struct tagTMP_FILE *p_file_struct, void* user_data, RANGE_DATA_BUFFER *data_buffer, _int32 read_result, _u32 read_len)
{
	BT_TMP_FILE* tmp_file = (BT_TMP_FILE*)user_data;
	if(read_result == FM_READ_CLOSE_EVENT
     || tmp_file->_state == UNINIT_STATE )
	{
		LOG_DEBUG("notify_bt_tmp_file_read, write_result = FM_WRITE_CLOSE_EVENT, tmp_file->_cur_read_req = 0x%x.", tmp_file->_cur_read_req);
		if(tmp_file->_cur_read_req != NULL)
		{
			sd_free(tmp_file->_cur_read_req->_data_buffer->_data_ptr);
			tmp_file->_cur_read_req->_data_buffer->_data_ptr = NULL;
			sd_free(tmp_file->_cur_read_req);
			tmp_file->_cur_read_req = NULL;
		}
		return SUCCESS;
	}
	LOG_DEBUG("notify_bt_tmp_file_read, pending_range[%u, %u] read file callback.", tmp_file->_cur_read_req->_padding_range._index, tmp_file->_cur_read_req->_padding_range._num);
	tmp_file->_cur_read_req->_callback(p_file_struct, tmp_file->_cur_read_req->_user_data, tmp_file->_cur_read_req->_data_buffer, read_result, read_len);
	sd_free(tmp_file->_cur_read_req);
	tmp_file->_cur_read_req = NULL;
	return bt_update_tmp_file(tmp_file);
}

_int32 notify_bt_tmp_file_write(struct tagTMP_FILE* base_file, void* user_data, RANGE_DATA_BUFFER_LIST* data_buffer_list, _int32 write_result)
{
	_int32 ret = SUCCESS;
	RANGE_CHANGE_NODE* data = NULL;
	BT_TMP_FILE* tmp_file = (BT_TMP_FILE*)user_data;
	drop_buffer_list_without_buffer(data_buffer_list);
	if(write_result == FM_WRITE_CLOSE_EVENT
       || tmp_file->_state == UNINIT_STATE )
	{
		//这个时候文件已经处于关闭状态，当前的请求被取消了，释放内存
		LOG_DEBUG("notify_bt_tmp_file_write, write_result = FM_WRITE_CLOSE_EVENT, tmp_file->_cur_write_req = 0x%x.", tmp_file->_cur_write_req);
		if(tmp_file->_cur_write_req != NULL)
		{
			//dm_free_buffer_to_data_buffer(tmp_file->_cur_write_req->_buffer_len, &tmp_file->_cur_write_req->_buffer);
			sd_free(tmp_file->_cur_write_req);
			tmp_file->_cur_write_req = NULL;
		}
		return SUCCESS;
	}
	LOG_DEBUG("notify_bt_tmp_file_write, pending_range[%u, %u] write to file success", tmp_file->_cur_write_req->_pending_range._index, tmp_file->_cur_write_req->_pending_range._num);
	range_list_add_range(&tmp_file->_valid_padding_range_list, &tmp_file->_cur_write_req->_pending_range, NULL, NULL);
	set_find_node(&tmp_file->_range_change_set, &tmp_file->_cur_write_req->_pending_range, (void**)&data);
	if(data == NULL)
	{	//只有写成功了才添加range_change_node
		ret = bt_malloc_range_change_node(&data);
		sd_assert(ret == SUCCESS);
		data->_padding_range._index = tmp_file->_cur_write_req->_pending_range._index;
		data->_padding_range._num = 1;
		data->_file_range_index = tmp_file->_total_range_num;
		ret = set_insert_node(&tmp_file->_range_change_set, data);
		sd_assert(ret == SUCCESS);
		++tmp_file->_total_range_num;//文件多的时候,这个值会很大,导致大小为20,12的结构过多导致1025
	}
	tmp_file->_cur_write_req->_callback(write_result, tmp_file->_cur_write_req->_user_data, &tmp_file->_cur_write_req->_pending_range);
	sd_free(tmp_file->_cur_write_req);
	tmp_file->_cur_write_req = NULL;
	//bt_tmp_file_save_to_cfg_file(tmp_file);				//写成功一块即重写配置文件
	return bt_update_tmp_file(tmp_file);
}

_int32 notify_bt_tmp_file_create(struct tagTMP_FILE* base_file, void* user_data, _int32 create_result)
{
	BT_TMP_FILE* tmp_file = (BT_TMP_FILE*)user_data;
	if(create_result == FM_CREAT_CLOSE_EVENT
     || tmp_file->_state == UNINIT_STATE )
		return SUCCESS;
	tmp_file->_state = READ_WRITE_STATE;
	return bt_update_tmp_file(tmp_file);
}

_int32 notify_bt_tmp_file_close(struct tagTMP_FILE* base_file, void* user_data, _int32 close_result)
{
	char file_name[MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN] = {0};
	BOOL del_flag = (BOOL)user_data;
	if(del_flag == TRUE)
	{
		sd_snprintf(file_name, MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN, "%s%s", base_file->_file_path, base_file->_file_name);
		sd_delete_file(file_name);
	}
	return SUCCESS;		
}

_int32 bt_tmp_file_save_to_cfg_file(BT_TMP_FILE* tmp_file)		//序列化
{
	_int32 ret = SUCCESS;
	_u32 file_id = INVALID_FILE_ID;
	char file_name[MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN] = {0};
	_u32 version = 2, write_size = 0, num = 0;
	RANGE_LIST_ITEROATOR node = NULL;
	SET_ITERATOR iter;
	RANGE_CHANGE_NODE* data = NULL;
	char  buffer[1024];
	_u32 buffer_pos=0,buffer_len=1024;
	_u32 data_unit_size = get_data_unit_size();

	LOG_DEBUG("bt_tmp_file_save_to_cfg_file");
	
	sd_snprintf(file_name, MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN, "%s%s.cfg", tmp_file->_file_path, tmp_file->_file_name);
	ret = sd_open_ex(file_name, O_FS_CREATE, &file_id);
	if( ret != SUCCESS )
		return ret;

	sd_setfilepos(file_id, 0);
	/*version: 4 bytes*/
	ret = sd_write_save_to_buffer(file_id, buffer, buffer_len,&buffer_pos, (char*)&version, sizeof(_u32));
	if(ret != SUCCESS) goto ErrHandler;
	
	/*data_unit_size: 4Bytes*/
	ret = sd_write_save_to_buffer(file_id, buffer, buffer_len,&buffer_pos, (char*)&data_unit_size, sizeof(data_unit_size));
	if(ret != SUCCESS) goto ErrHandler;
		
	/*_tmp_padding_range_list: 头四个字节表示range的节点数，其后依次为节点range*/
	num = range_list_size(&tmp_file->_tmp_padding_range_list);
	ret = sd_write_save_to_buffer(file_id, buffer, buffer_len,&buffer_pos, (char*)&num, sizeof(_u32));
	if(ret != SUCCESS) goto ErrHandler;
		
	range_list_get_head_node(&tmp_file->_tmp_padding_range_list, &node);	  
	while(node != NULL)
	{
		ret = sd_write_save_to_buffer(file_id, buffer, buffer_len,&buffer_pos, (char*)&node->_range._index, sizeof(node->_range._index));
		if(ret != SUCCESS) goto ErrHandler;
		
		ret = sd_write_save_to_buffer(file_id, buffer, buffer_len,&buffer_pos, (char*)&node->_range._num, sizeof(node->_range._num));
		if(ret != SUCCESS) goto ErrHandler;
		LOG_DEBUG("bt_tmp_file_save_to_cfg_file, _tmp_padding_range[%u, %u].", node->_range._index, node->_range._num); 
		
		range_list_get_next_node(&tmp_file->_tmp_padding_range_list, node, &node); 
	}
	/*_valid_padding_range_list: 头四个字节表示range的节点数，其后依次为节点range*/	
	num = range_list_size(&tmp_file->_valid_padding_range_list);
	ret = sd_write_save_to_buffer(file_id, buffer, buffer_len,&buffer_pos, (char*)&num, sizeof(_u32));
	if(ret != SUCCESS) goto ErrHandler;
		
	range_list_get_head_node(&tmp_file->_valid_padding_range_list, &node);
	while(node != NULL)
	{
		
		LOG_DEBUG("bt_tmp_file_save_to_cfg_file, add range[%u, %u].", node->_range._index, node->_range._num); 
		ret = sd_write_save_to_buffer(file_id, buffer, buffer_len,&buffer_pos, (char*)&node->_range._index, sizeof(node->_range._index));
		if(ret != SUCCESS) goto ErrHandler;
		
		ret = sd_write_save_to_buffer(file_id, buffer, buffer_len,&buffer_pos, (char*)&node->_range._num, sizeof(node->_range._num));
		if(ret != SUCCESS) goto ErrHandler;
		
		range_list_get_next_node(&tmp_file->_valid_padding_range_list, node, &node); 
	}
	/*_total_range_num;	4 bytes*/
	ret = sd_write_save_to_buffer(file_id, buffer, buffer_len,&buffer_pos, (char*)&tmp_file->_total_range_num, sizeof(_u32));
	if(ret != SUCCESS) goto ErrHandler;
		
	/*_range_change_set; 头四个字节表示set的节点数*/
	num = set_size(&tmp_file->_range_change_set);
	sd_assert(num == tmp_file->_total_range_num);
	ret = sd_write_save_to_buffer(file_id, buffer, buffer_len,&buffer_pos, (char*)&num, sizeof(_u32));
	if(ret != SUCCESS) goto ErrHandler;
		
	for(iter = SET_BEGIN(tmp_file->_range_change_set); iter != SET_END(tmp_file->_range_change_set); iter = SET_NEXT(tmp_file->_range_change_set, iter))
	{
		data = (RANGE_CHANGE_NODE*)iter->_data;
		
		LOG_DEBUG("bt_tmp_file_save_to_cfg_file, add range_change_node, padding_range[%u, %u], file_range_index = %u.", data->_padding_range._index, data->_padding_range._num, data->_file_range_index);
		ret = sd_write_save_to_buffer(file_id, buffer, buffer_len,&buffer_pos, (char*)&data->_padding_range._index, sizeof(data->_padding_range._index));
		if(ret != SUCCESS) goto ErrHandler;
		
		ret = sd_write_save_to_buffer(file_id, buffer, buffer_len,&buffer_pos, (char*)&data->_padding_range._num, sizeof(data->_padding_range._num));
		if(ret != SUCCESS) goto ErrHandler;
		
		ret = sd_write_save_to_buffer(file_id, buffer, buffer_len,&buffer_pos, (char*)&data->_file_range_index, sizeof(data->_file_range_index));
		if(ret != SUCCESS) goto ErrHandler;
	}

	if(buffer_pos!=0)
	{
		ret =sd_write(file_id,buffer, buffer_pos, &write_size);
        LOG_DEBUG("bt_tmp_file_save_to_cfg_file, buffer_pos = %u, write_size = %u", buffer_pos, write_size);
		sd_assert(buffer_pos==write_size);
	}

ErrHandler:
	
	sd_close_ex(file_id);

	LOG_DEBUG("bt_tmp_file_save_to_cfg_file end:ret=%d,last write_size=%u",ret,buffer_pos);
	
	return ret;
}


_int32 bt_tmp_file_load_from_cfg_file(BT_TMP_FILE* tmp_file)	//反序列化
{
	_int32 ret = SUCCESS;
	_u32 file_id = INVALID_FILE_ID;
	char file_name[MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN] = {0};
	_u32 version = 0, read_size = 0, num = 0, i = 0;
	RANGE_LIST record_range_list;
	RANGE range;
	RANGE_CHANGE_NODE* data = NULL;
	_u32 cfg_data_unit_size = 0;
	sd_snprintf(file_name, MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN, "%s%s", tmp_file->_file_path, tmp_file->_file_name);
	if(sd_file_exist(file_name) == FALSE)
		return -1;	//临时文件不存在
	sd_snprintf(file_name, MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN, "%s%s.cfg", tmp_file->_file_path, tmp_file->_file_name);
	ret = sd_open_ex(file_name, O_FS_CREATE, &file_id);
	if(ret != SUCCESS)
		return ret;
	/*version: 4 bytes*/
	ret = sd_read(file_id, (char*)&version, sizeof(version), &read_size);
	if( ret != SUCCESS || read_size == 0 )
	{
		sd_close_ex(file_id);
		return -1;
	}	
	
	if( 2 == version)
	{    
		sd_read(file_id, (char*)&cfg_data_unit_size, sizeof(cfg_data_unit_size), &read_size);
		LOG_DEBUG("bt_tmp_file_load_from_cfg_file, read cfg_data_unit_size = %u  .", cfg_data_unit_size);	
		if( cfg_data_unit_size != get_data_unit_size())
		{
			sd_close_ex(file_id);
			return -1;
		}
	}
	else
	{
		//版本号不对
		LOG_DEBUG("bt_tmp_file_load_from_cfg_file, unsupport version." );	
		sd_close_ex(file_id);
		return -1;		
	}
	
	/*_tmp_padding_range_list: 头四个字节表示range的节点数，其后依次为节点range*/
	ret = sd_read(file_id, (char*)&num, sizeof(num), &read_size);
	if( ret != SUCCESS || read_size == 0 )
	{
		sd_close_ex(file_id);
		return -1;
	}	
	
	for(i = 0; i < num; ++i)
	{
		sd_read(file_id,(char*)&range._index, sizeof(range._index), &read_size);	
		ret = sd_read(file_id,(char*)&range._num, sizeof(range._num), &read_size);	   
		if( ret != SUCCESS || read_size == 0 )
		{
			sd_close_ex(file_id);
			return -1;
		}

		range_list_add_range(&tmp_file->_tmp_padding_range_list, &range, NULL, NULL); 
	}
	/*_valid_padding_range_list: 头四个字节表示range的节点数，其后依次为节点range*/	
	ret = sd_read(file_id, (char*)&num, sizeof(num), &read_size);
	if( ret != SUCCESS || read_size == 0 )
	{
		sd_close_ex(file_id);
		return -1;
	}	
	for(i = 0; i < num; ++i)
	{
		sd_read(file_id,(char*)&range._index, sizeof(range._index), &read_size);	
		ret = sd_read(file_id,(char*)&range._num, sizeof(range._num), &read_size);	   
		if( ret != SUCCESS || read_size == 0 )
		{
			sd_close_ex(file_id);
			return -1;
		}
		LOG_DEBUG("bt_tmp_file_load_from_cfg_file, add range[%u, %u].", range._index, range._num); 
		range_list_add_range(&tmp_file->_valid_padding_range_list, &range, NULL, NULL); 
	}
	/*_total_range_num;	4 bytes*/
	ret = sd_read(file_id, (char*)&num, sizeof(num), &read_size);
	if( ret != SUCCESS || read_size == 0 )
	{
		sd_close_ex(file_id);
		return -1;
	}
	tmp_file->_total_range_num = num;
	/*_range_change_set; 头四个字节表示set的节点数*/
	ret = sd_read(file_id, (char*)&num, sizeof(num), &read_size);
	if( ret != SUCCESS || read_size == 0 )
	{
		sd_close_ex(file_id);
		return -1;
	}	
	
	if(num != tmp_file->_total_range_num)
	{
		sd_close_ex(file_id);
		return -1;
	}
	range_list_init(&record_range_list);
	for(i = 0; i < num; ++i)
	{
		ret = bt_malloc_range_change_node(&data);
		CHECK_VALUE(ret);
		sd_read(file_id, (char*)&data->_padding_range._index, sizeof(data->_padding_range._index), &read_size);
		sd_read(file_id, (char*)&data->_padding_range._num, sizeof(data->_padding_range._num), &read_size);
		ret = sd_read(file_id, (char*)&data->_file_range_index, sizeof(data->_file_range_index), &read_size);
		if( ret != SUCCESS || read_size == 0 )
		{
			sd_close_ex(file_id);
			return -1;
		}

		LOG_DEBUG("bt_tmp_file_load_from_cfg_file, add range_change_node, padding_range[%u, %u], file_range_index = %u.", data->_padding_range._index, data->_padding_range._num, data->_file_range_index);

		if( !range_list_is_include( &tmp_file->_valid_padding_range_list, &data->_padding_range ) )
		{
			bt_free_range_change_node(data);
			data = NULL;
			sd_close_ex(file_id);
			range_list_clear( &record_range_list );
			return -1;
		}
		
		range_list_add_range(&record_range_list,  &data->_padding_range, NULL, NULL);
		set_insert_node(&tmp_file->_range_change_set, data);
	}
	//做一次规整，防止两个信息不一致
	//range_list_adjust(&tmp_file->_valid_padding_range_list, &record_range_list);
	if( !range_list_is_contained(&record_range_list, &tmp_file->_valid_padding_range_list) )
	{
		sd_close_ex(file_id);
		range_list_clear( &record_range_list );
		return -1;		
	}
	range_list_clear( &record_range_list );
	return sd_close_ex(file_id);
}


_int32 bt_update_tmp_file(BT_TMP_FILE* tmp_file)
{
	_int32 ret = SUCCESS;
	switch(tmp_file->_state)
	{
	case IDLE_STATE:
		{
			sd_assert(tmp_file->_cur_read_req == NULL && tmp_file->_cur_write_req == NULL);
			if(list_size(&tmp_file->_read_request_list) > 0 || list_size(&tmp_file->_write_request_list) > 0)
			{	//有请求，打开文件
				sd_assert(tmp_file->_base_file == NULL);
				ret = fm_create_file_struct(tmp_file->_file_name, tmp_file->_file_path, 0, tmp_file, notify_bt_tmp_file_create, &tmp_file->_base_file,FWM_RANGE);
				sd_assert(ret == SUCCESS);
				tmp_file->_state = OPENING_STATE;
			}
			break;
		}
	case OPENING_STATE:
		{
			break;
		}
	case READ_WRITE_STATE:
		{
			if(tmp_file->_cur_read_req == NULL && list_size(&tmp_file->_read_request_list) > 0)
			{
				bt_send_read_request(tmp_file);	//发起一个读请求
			}
			if(tmp_file->_cur_write_req == NULL && list_size(&tmp_file->_write_request_list) > 0)
			{
				bt_send_write_request(tmp_file);	//发起一个写请求
			}
			break;
		}
	case CLOSING_STATE:
		{
			break;
		}
	default:
		sd_assert(FALSE);
	}
	return SUCCESS;
}
#endif

