#include "utility/define.h"
#ifdef ENABLE_BT 


#include "bt_piece_checker.h"
#include "bt_data_manager_imp.h"
#include "utility/utility.h"
#include "utility/mempool.h"
#include "utility/logid.h"
#define	LOGID LOGID_BT_DOWNLOAD
#include "utility/logger.h"


#define	PIECE_CHECKER_TIMEOUT	3000

_int32 bt_init_piece_checker(BT_PIECE_CHECKER* piece_checker, BT_DATA_MANAGER* data_manager, _u8* piece_hash, char* path, char* name)
{
	_int32 ret = SUCCESS;
	LOG_DEBUG("[piece_checker = 0x%x]bt_init_piece_checker.", piece_checker);
	ret = bt_init_tmp_file(&piece_checker->_tmp_file, path, name);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("bt_init_piece_checker, but init temp file manager failed, errcode = %d.", ret);
		return ret;
	}
	piece_checker->_bt_data_manager = data_manager;
	piece_checker->_piece_hash = piece_hash;
	range_list_init(&piece_checker->_need_download_range_list);
	range_list_init(&piece_checker->_need_check_range_list);
	list_init(&piece_checker->_waiting_check_piece_list);
	return start_timer(bt_checker_handle_timeout, NOTICE_INFINITE, PIECE_CHECKER_TIMEOUT, 0, piece_checker, &piece_checker->_check_timeout_id);
}

_int32 bt_uninit_piece_checker(BT_PIECE_CHECKER* piece_checker)
{
	LOG_DEBUG("[piece_checker = 0x%x]bt_uninit_piece_checker.", piece_checker);
	range_list_clear(&piece_checker->_need_download_range_list);
	range_list_clear(&piece_checker->_need_check_range_list);
	list_clear(&piece_checker->_waiting_check_piece_list);
	bt_uninit_tmp_file(&piece_checker->_tmp_file);
	piece_checker->_piece_hash = NULL;
	if(piece_checker->_check_timeout_id != INVALID_MSG_ID)
	{
		cancel_timer(piece_checker->_check_timeout_id);
		piece_checker->_check_timeout_id = INVALID_MSG_ID;
	}
	if(piece_checker->_cur_hash_piece != NULL)
	{
		piece_checker->_cur_hash_piece->_piece_checker = NULL;
		piece_checker->_cur_hash_piece = NULL;
	}
	return SUCCESS;
}

_int32 bt_checker_get_need_download_range(BT_PIECE_CHECKER* piece_checker, RANGE_LIST* download_range_list)
{
	LOG_DEBUG("[piece_checker = 0x%x]bt_checker_get_need_download_range.", piece_checker);
	range_list_clear(download_range_list);
	range_list_add_range_list(download_range_list, &piece_checker->_need_download_range_list);
	out_put_range_list(download_range_list);
	return SUCCESS;
}

_int32 bt_checker_put_data(BT_PIECE_CHECKER* piece_ckecker, const RANGE* padding_range, char* buffer, _u32 data_len, _u32 buffer_len)
{
	_int32 ret = SUCCESS;
	LOG_DEBUG("[piece_checker = 0x%x]bt_checker_put_data, padding_range[%u, %u], buffer_len = %u, data_len = %u.", piece_ckecker, padding_range->_index, padding_range->_num, buffer_len, data_len);
	sd_assert(data_len <= buffer_len);
	//首先判断是否临时文件的range,是的话写入到临时文件中
	if(bt_is_tmp_file_range(&piece_ckecker->_tmp_file, padding_range) == TRUE)
	{	//这是临时文件中的range,需要写到临时文件中
		ret = bt_write_tmp_file(&piece_ckecker->_tmp_file, padding_range, buffer, buffer_len, data_len, bt_checker_write_tmp_file_callback, piece_ckecker);//这里应该还有一个回调接口，用于通知写入成功，再擦除need_download_range_list
		if(ret != SUCCESS)
		{
			LOG_DEBUG("[piece_checker = 0x%x]bt_write_tmp_file failed, padding_range[%u, %u], errcode = %d.", piece_ckecker, padding_range->_index, padding_range->_num, ret);
			return ret;
		}
		range_list_delete_range(&piece_ckecker->_need_download_range_list, padding_range, NULL, NULL);
	}
	else
	{
		return BT_NOT_IN_TMP_FILE;
	}
	return SUCCESS;
}

_int32 bt_checker_recv_range(BT_PIECE_CHECKER* piece_checker, const RANGE* padding_range)
{
	RANGE extra_padding_range;
	BOOL recv_flag = FALSE;			//全部收到标志
	_u32 first_piece = 0, last_piece = 0;
	LOG_DEBUG("[piece_checker = 0x%x]bt_checker_recv_range, padding_range[%u, %u].", piece_checker, padding_range->_index, padding_range->_num);
	//接着计算出这个range所在的piece是否可以需要校验
	brs_range_to_extra_piece(&piece_checker->_bt_data_manager->_bt_range_switch, padding_range, &first_piece, &last_piece);
	while(first_piece < last_piece + 1)
	{
		brs_piece_to_extra_padding_range(&piece_checker->_bt_data_manager->_bt_range_switch
		    , first_piece, &extra_padding_range);		//得到包含该piece的padding_range
		if(range_list_is_relevant(&piece_checker->_need_check_range_list, &extra_padding_range) == FALSE)
		{
			++first_piece;
			continue;		//这一块piece不需要校验
		}
		//查看下这一块piece的数据是否收到
		if(bt_is_tmp_file_range(&piece_checker->_tmp_file, &extra_padding_range) == TRUE)
		{	//这一块是在临时文件中
			recv_flag = bt_is_tmp_range_valid(&piece_checker->_tmp_file, &extra_padding_range);
		}
		else
		{	//这一块在普通文件中
			recv_flag = bdm_padding_range_is_resv(piece_checker->_bt_data_manager, &extra_padding_range);
		}
		if(recv_flag == TRUE)
		{	//需要的数据都已经收到，可以把这个piece放到待校验队列里面了
			bt_checker_commit_piece(piece_checker, first_piece);
		}
		++first_piece;
	}
	return SUCCESS;
}

_int32 bt_checker_erase_need_check_range(BT_PIECE_CHECKER* piece_checker, RANGE* padding_range)
{
	LOG_DEBUG("[piece_checker = 0x%x]bt_checker_erase_need_check_range, padding_range[%u, %u].", piece_checker, padding_range->_index, padding_range->_num);
	return range_list_delete_range(&piece_checker->_need_check_range_list, padding_range, NULL, NULL);
}

_int32 bt_checker_add_need_check_file(BT_PIECE_CHECKER* piece_checker, _u32 file_index)
{
	BT_RANGE bt_range;
	RANGE padding_range;
	BT_RANGE_SWITCH* range_switch = &piece_checker->_bt_data_manager->_bt_range_switch;
	FILE_RANGE_INFO* file_info = NULL;
	file_info = range_switch->_file_range_info_array + file_index;		//获得需要下载的文件信息
	bt_range._pos = file_info->_file_offset;
	bt_range._length = file_info->_file_size;
	brs_bt_range_to_extra_padding_range(range_switch, &bt_range, &padding_range);
	LOG_DEBUG("[piece_checker = 0x%x]bt_checker_add_need_check_range, file_index = %u, padding_range[%u, %u]", piece_checker, file_index, padding_range._index, padding_range._num);
	range_list_add_range(&piece_checker->_need_check_range_list, &padding_range, NULL, NULL);
	//添加临时文件
	bt_add_tmp_file(&piece_checker->_tmp_file, file_index, range_switch);
	bt_get_tmp_file_need_donwload_range_list(&piece_checker->_tmp_file, &piece_checker->_need_download_range_list);
	return SUCCESS;
}

_int32 bt_checker_del_tmp_file(BT_PIECE_CHECKER* piece_checker)
{
	return bt_tmp_file_set_del_flag(&piece_checker->_tmp_file, TRUE);
}

_int32 bt_checker_get_tmp_file_valid_range(BT_PIECE_CHECKER* piece_checker, RANGE_LIST* valid_padding_range_list)
{
	return bt_tmp_file_get_valid_range_list(&piece_checker->_tmp_file, valid_padding_range_list);
}

_int32 bt_checker_read_tmp_file(BT_PIECE_CHECKER* piece_checker, RANGE_DATA_BUFFER* data_buffer, notify_read_result callback, void* user_data)
{
	return bt_read_tmp_file(&piece_checker->_tmp_file, &data_buffer->_range, data_buffer, callback, user_data);
}

_int32 bt_checker_commit_piece(BT_PIECE_CHECKER* piece_checker, _u32 piece_index)
{
	LOG_DEBUG("[piece_checker = 0x%x]bt_checker_commit_piece, piece_index = %u.", piece_checker, piece_index);
	return list_push(&piece_checker->_waiting_check_piece_list, (void*)piece_index);
}

_int32 bt_checker_handle_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid)
{
	BT_PIECE_CHECKER* piece_checker = (BT_PIECE_CHECKER*)msg_info->_user_data;
	if(errcode == MSG_CANCELLED)
		return SUCCESS;
	LOG_DEBUG("[piece_checker = 0x%x]bt_checker_handle_timeout, piece_checker->_cur_hash_piece = 0x%x ", piece_checker, piece_checker->_cur_hash_piece);
	if(piece_checker->_cur_hash_piece != NULL)
	{
		if(piece_checker->_cur_hash_piece->_is_waiting_read == TRUE)
		{
			piece_checker->_cur_hash_piece->_is_waiting_read = FALSE;
			bt_checker_read_data(piece_checker);		//再一次试图去读
		}
		return SUCCESS;					//目前有hash正在计算
	}
	return bt_checker_start_piece_hash(piece_checker);
}

//当前正在校验的piece读取失败
_int32 bt_checker_handle_read_failed(BT_PIECE_CHECKER* piece_checker)
{
	READ_RANGE_INFO* range_info = NULL;
	BT_PIECE_HASH* piece_hash = piece_checker->_cur_hash_piece;
	sd_assert(piece_hash != NULL);
	if (piece_hash == NULL) {
		return SUCCESS;
	}
	LOG_DEBUG("[piece_checker = 0x%x]bt_checker_handle_read_failed, piece_index = %u.", piece_checker, piece_hash->_piece_index);
	while(list_size(&piece_hash->_read_range_info_list) > 0)
	{
		list_pop(&piece_hash->_read_range_info_list, (void**)&range_info);
		read_range_info_free_wrap(range_info);
	}
	return SUCCESS;
}

_int32 bt_checker_read_data_callback(struct tagTMP_FILE *p_file_struct, void *p_user, RANGE_DATA_BUFFER *data_buffer, _int32 read_result, _u32 read_len)
{
	_u32 len = 0;
	_u64 offset;
	BT_PIECE_HASH* piece_hash = (BT_PIECE_HASH*)p_user;
	BT_PIECE_CHECKER* piece_checker = piece_hash->_piece_checker;
	READ_RANGE_INFO* range_info = NULL;
	LOG_DEBUG("[piece_checker = 0x%x]bt_checker_read_data_callback, file_range[%u, %u], read_result = %d.", piece_checker, data_buffer->_range._index, data_buffer->_range._num, read_result);

	//add by peng : 任务结束的时候,收到回调消息,但是piece_checker已经被析构
	if( piece_checker == NULL )
	{
		SAFE_DELETE(piece_hash->_data_buffer._data_ptr);
		SAFE_DELETE(piece_hash);
		return SUCCESS;
	}
	
	if(read_result == SUCCESS)
	{
		range_info = (READ_RANGE_INFO*)LIST_VALUE(LIST_BEGIN(piece_checker->_cur_hash_piece->_read_range_info_list));
		sd_assert(range_info != NULL);
		offset = range_info->_padding_exact_range._pos - (_u64)get_data_unit_size() * range_info->_padding_range._index;
		len = MIN(data_buffer->_data_length - offset, range_info->_padding_exact_range._length);
		bt_checker_calc_hash(piece_checker->_cur_hash_piece, data_buffer->_data_ptr + offset, len);
		sd_free(data_buffer->_data_ptr);
		data_buffer->_data_ptr = NULL;
		range_info->_range_length -= data_buffer->_data_length;
		range_info->_padding_range._index += 1;
		range_info->_padding_range._num -= 1;
		range_info->_padding_exact_range._pos += len;
		range_info->_padding_exact_range._length -= len;
		if(range_info->_padding_exact_range._length == 0)
		{
			list_pop(&piece_hash->_read_range_info_list, (void**)&range_info);
			read_range_info_free_wrap(range_info);
		}
	}
	else
	{	
		bt_checker_handle_read_failed(piece_checker);
		return bt_checker_start_piece_hash(piece_checker);
	}
	//看看是否还有数据需要读
	if(list_size(&piece_checker->_cur_hash_piece->_read_range_info_list) == 0)
		return bt_checker_verify_piece(piece_checker);	//没有数据需要读了，对piece进行校验
	else
		return bt_checker_read_data(piece_checker);		//继续读数据
}

_int32 bt_checker_read_data(BT_PIECE_CHECKER* piece_checker)
{
	_u64 offset;
	_u32 len;
	_int32 ret = SUCCESS;
	RANGE padding_range;
	RANGE file_range;
	READ_RANGE_INFO* range_info = NULL;
	RANGE_DATA_BUFFER_LIST range_data_buffer_list;
	RANGE_DATA_BUFFER* data_buffer = NULL;
	sd_assert(list_size(&piece_checker->_cur_hash_piece->_read_range_info_list) > 0);
	if(piece_checker->_cur_hash_piece->_in_tmp_file == TRUE)
	{	//在临时文件中
		range_info = LIST_VALUE(LIST_BEGIN(piece_checker->_cur_hash_piece->_read_range_info_list));
		ret = sd_malloc(get_data_unit_size(), (void**)&piece_checker->_cur_hash_piece->_data_buffer._data_ptr);
		sd_assert(ret == SUCCESS);
		file_range._index = range_info->_padding_range._index;
		file_range._num = 1;
		brs_file_range_to_padding_range(&piece_checker->_bt_data_manager->_bt_range_switch, range_info->_file_index, &file_range, &padding_range);
		sd_assert(padding_range._num == 1);
		piece_checker->_cur_hash_piece->_data_buffer._range._index = padding_range._index;
		piece_checker->_cur_hash_piece->_data_buffer._range._num = padding_range._num;
		piece_checker->_cur_hash_piece->_data_buffer._buffer_length =get_data_unit_size();
		piece_checker->_cur_hash_piece->_data_buffer._data_length = get_data_unit_size();
		ret = bt_read_tmp_file(&piece_checker->_tmp_file, &padding_range, &piece_checker->_cur_hash_piece->_data_buffer, bt_checker_read_data_callback, piece_checker->_cur_hash_piece);
		if(ret != SUCCESS)
		{
			bt_checker_handle_read_failed(piece_checker);
			return bt_checker_start_piece_hash(piece_checker);
		}
		return ret;
	}
	//数据在正常文件中
	while(list_size(&piece_checker->_cur_hash_piece->_read_range_info_list) > 0)
	{
		range_info = LIST_VALUE(LIST_BEGIN(piece_checker->_cur_hash_piece->_read_range_info_list));
		padding_range._index = range_info->_padding_range._index;
		padding_range._num = 1;
		if(bdm_file_range_is_cached(piece_checker->_bt_data_manager, range_info->_file_index, &padding_range) == TRUE)
		{
			LOG_DEBUG("[piece_checker = 0x%x]bdm_file_range_is_cached, file_index = %u, range[%u, %u].", piece_checker, range_info->_file_index, padding_range._index, padding_range._num);
			buffer_list_init(&range_data_buffer_list);
			ret = bdm_get_cache_data_buffer(piece_checker->_bt_data_manager, range_info->_file_index, &padding_range, &range_data_buffer_list);
			sd_assert(ret == SUCCESS);
			sd_assert(buffer_list_size(&range_data_buffer_list) == 1);
			list_pop(&range_data_buffer_list._data_list, (void**)&data_buffer);
			offset = range_info->_padding_exact_range._pos - (_u64)get_data_unit_size() * range_info->_padding_range._index;
			len = MIN(data_buffer->_data_length - offset, range_info->_padding_exact_range._length);		
			bt_checker_calc_hash(piece_checker->_cur_hash_piece, data_buffer->_data_ptr + offset, len);
			range_info->_range_length -= data_buffer->_data_length;
			range_info->_padding_range._index += 1;
			range_info->_padding_range._num -= 1;
			range_info->_padding_exact_range._pos += len;
			range_info->_padding_exact_range._length -= len;
			if(range_info->_padding_exact_range._length == 0)
			{
				list_pop(&piece_checker->_cur_hash_piece->_read_range_info_list, (void**)&range_info);
				read_range_info_free_wrap(range_info);
			}
		}
		else if(bdm_range_is_write_in_disk(piece_checker->_bt_data_manager, range_info->_file_index, &padding_range) == TRUE)
		{	//这一块不在内存中
			sd_assert(piece_checker->_cur_hash_piece->_is_waiting_read == FALSE);
			ret = sd_malloc(get_data_unit_size(), (void**)&piece_checker->_cur_hash_piece->_data_buffer._data_ptr);
			sd_assert(ret == SUCCESS);
			piece_checker->_cur_hash_piece->_data_buffer._range._index = range_info->_padding_range._index;
			piece_checker->_cur_hash_piece->_data_buffer._range._num = 1;
			piece_checker->_cur_hash_piece->_data_buffer._buffer_length = get_data_unit_size();
			piece_checker->_cur_hash_piece->_data_buffer._data_length = MIN(get_data_unit_size(), range_info->_range_length);
			ret = bdm_read_data(piece_checker->_bt_data_manager, range_info->_file_index, &piece_checker->_cur_hash_piece->_data_buffer, bt_checker_read_data_callback, piece_checker->_cur_hash_piece);
			if(ret != SUCCESS)
			{
				bt_checker_handle_read_failed(piece_checker);
				return bt_checker_start_piece_hash(piece_checker);
			}
			return ret;
		}
		else
		{
			LOG_DEBUG("[piece_checker = 0x%x]bt_checker_read_data, but range[%u, %u] is writing, waiting to read.", piece_checker, padding_range._index, padding_range._num);
			piece_checker->_cur_hash_piece->_is_waiting_read = TRUE;
			return SUCCESS;		//此时不读
		}

	}
	//看看所有数据是否已经hash完毕
	if(list_size(&piece_checker->_cur_hash_piece->_read_range_info_list) == 0)
		bt_checker_verify_piece(piece_checker);
	return SUCCESS;
}

_int32 bt_checker_calc_hash(BT_PIECE_HASH* piece_hash, char* data, _u32 len)
{
	
//add test
/*
#ifdef _LOGGER  
	_u32 piece_index = piece_hash->_piece_index;
	char tmp_buffer[MAX_FILE_NAME_LEN];
	static _u32 num = 0;
	_u32 file_id;
	_u32 write_size;
	_u32 cur_time = 0;
	sd_time( &cur_time );
	if( piece_index == 144 )
	{
		sd_snprintf( tmp_buffer, MAX_FILE_NAME_LEN, "./144_piece_%d_len_%d_time:%u.test", num, len, cur_time );
		num++;
		sd_open_ex( tmp_buffer, 1, &file_id );
		sd_write( file_id, data, len, &write_size );
	}

#endif 
*/
//add test

	sha1_update(&piece_hash->_sha1, (const _u8*)data, len);
	return SUCCESS;
}

_int32 bt_checker_verify_piece(BT_PIECE_CHECKER* piece_checker)
{
	BOOL result = FALSE;
	RANGE padding_range;

#ifdef _LOGGER  
	_u8 piece_hash_str[CID_SIZE*2+1];
#endif 

	_u8 piece_hash[CID_SIZE];
	_u32 piece_index = piece_checker->_cur_hash_piece->_piece_index;

	//add by xiangqian
	_u32 range_first_piece = 0;
	_u32 range_last_piece = 0;
	LIST_ITERATOR cur_node = NULL;
	LIST_ITERATOR next_node = NULL;
	_u32 list_piece_index = 0;
	
	brs_piece_to_extra_padding_range(&piece_checker->_bt_data_manager->_bt_range_switch, piece_index, &padding_range);
	sha1_finish(&piece_checker->_cur_hash_piece->_sha1, piece_hash);
	
	result = sd_data_cmp(piece_hash, piece_checker->_piece_hash + CID_SIZE * piece_index, CID_SIZE);

//add by xiangqian
#ifdef _LOGGER  
	str2hex((char*)piece_hash, CID_SIZE, (char *)piece_hash_str, CID_SIZE*2);
	piece_hash_str[CID_SIZE*2] = '\0';	 

	LOG_DEBUG( "bt_checker_verify_piece, piece_index:%u, piece_hash_str:%s.", piece_index, piece_hash_str );

	str2hex((char*)piece_checker->_piece_hash + CID_SIZE * piece_index, CID_SIZE, (char *)piece_hash_str, CID_SIZE*2);
	piece_hash_str[CID_SIZE*2] = '\0';	 

	LOG_DEBUG( "bt_checker_verify_piece, piece_index:%u, correct piece_hash_str:%s.", piece_index, piece_hash_str );

	LOG_DEBUG( "bt_checker_verify_piece, piece_index:%u, result:%d", 
		piece_index, result );
#endif 		


	//除了通知bt_data_manager外，还要做修改need_download_range和need_check_range
	if(result == TRUE)
	{
	}
	else if(bt_is_tmp_file_range(&piece_checker->_tmp_file, &padding_range) == TRUE)
	{	//如果是临时文件里面的piece校验失败了，那么应该通知上层重新下载这块piece
		range_list_add_range(&piece_checker->_need_download_range_list, &padding_range, NULL, NULL);

/*		
		//add by xiangqian, 
		//发生piece校验成功,但是file_info还没下回来的情况,这时收到部分数据可能触发校验,若校验成功,向对方请求一块已经notify_have_piece的数据,导致对方关闭连接
		range_list_delete_range(&piece_checker->_tmp_file._tmp_padding_range_list, &padding_range, NULL, NULL);
*/
	}

	//add by xiangqian: 清除range相关的piece,否则相关piece校验时会一直读不到数据而卡死,甚至和调度发生死锁.
	if( !result )
	{
		brs_range_to_extra_piece(&piece_checker->_bt_data_manager->_bt_range_switch, &padding_range, &range_first_piece, &range_last_piece );
		cur_node = LIST_BEGIN( piece_checker->_waiting_check_piece_list );
		while( cur_node != LIST_END( piece_checker->_waiting_check_piece_list ) )
		{
			list_piece_index = (_u32) LIST_VALUE( cur_node );
			if( list_piece_index >= range_first_piece && list_piece_index <= range_last_piece )
			{
				next_node = LIST_NEXT( cur_node );
				list_erase( &piece_checker->_waiting_check_piece_list, cur_node );
				cur_node = next_node;
				continue;
			}
			cur_node = LIST_NEXT( cur_node );
		}
	}
	
	LOG_DEBUG("[piece_checker = 0x%x]bt_checker_verify_piece, check piece %u, result is %d.", piece_checker, piece_index, result);
	bdm_checker_notify_check_finished(piece_checker->_bt_data_manager, piece_index, &padding_range, result);
	//看下还有没有等待校验的piece
	if(list_size(&piece_checker->_waiting_check_piece_list) == 0)
	{	//没有需要校验的piece了
		LOG_DEBUG("[piece_checker = 0x%x]bt_checker_verify_piece, there is no waiting check piece, delete piece_checker->_cur_hash_piece.", piece_checker);
		sd_free(piece_checker->_cur_hash_piece);
		piece_checker->_cur_hash_piece = NULL;
		return SUCCESS;
	}
	//促发下一个校验
	return bt_checker_start_piece_hash(piece_checker);
}

_int32 bt_checker_start_piece_hash(BT_PIECE_CHECKER* piece_checker)
{
	_int32 ret = SUCCESS;
	_u32 piece_index = -1;
	RANGE padding_range;
	BT_RANGE bt_range;
	BT_RANGE_SWITCH* range_switch = &piece_checker->_bt_data_manager->_bt_range_switch;
	LOG_DEBUG("[piece_checker = 0x%x]bt_checker_start_piece_hash.", piece_checker);
	while(list_size(&piece_checker->_waiting_check_piece_list) > 0)
	{
		list_pop(&piece_checker->_waiting_check_piece_list, (void**)&piece_index);
		brs_piece_to_extra_padding_range(range_switch, piece_index, &padding_range);
		if(range_list_is_relevant(&piece_checker->_need_check_range_list, &padding_range) == FALSE)
		{	//这一块已经不需要校验了，直接丢弃
			LOG_DEBUG("[piece_checker = 0x%x]bt_checker_start_piece_hash, piece_index = %u, padding_range[%u,%u], but this range is no need to check.", piece_checker, piece_index, padding_range._index, padding_range._num);
			piece_index = -1;
			continue;
		}
		else
		{
			break;
		}
	}
	if(piece_index == -1)
	{
		LOG_DEBUG("[piece_checker = 0x%x]bt_checker_verify_piece, there is no waiting check piece, delete piece_checker->_cur_hash_piece.", piece_checker);
		sd_assert(list_size(&piece_checker->_waiting_check_piece_list) == 0);	
		SAFE_DELETE(piece_checker->_cur_hash_piece);
		piece_checker->_cur_hash_piece = NULL;
		return SUCCESS;					//没有需要校验的piece
	}
	LOG_DEBUG("[piece_checker = 0x%x]bt_checker_start_piece_hash, piece_index = %u.", piece_checker, piece_index);
	//开始读取数据进行校验
	if(piece_checker->_cur_hash_piece == NULL)
	{
		ret = sd_malloc(sizeof(BT_PIECE_HASH), (void**)&piece_checker->_cur_hash_piece);
		if(ret != SUCCESS)
		{
			list_push(&piece_checker->_waiting_check_piece_list, (void**)piece_index);
			return ret;
		}
	}
	sd_memset(piece_checker->_cur_hash_piece, 0, sizeof(BT_PIECE_HASH));
	piece_checker->_cur_hash_piece->_piece_index = piece_index;
	if(bt_is_tmp_file_range(&piece_checker->_tmp_file, &padding_range) == TRUE)
		piece_checker->_cur_hash_piece->_in_tmp_file = TRUE;
	else
		piece_checker->_cur_hash_piece->_in_tmp_file = FALSE;
	piece_checker->_cur_hash_piece->_is_waiting_read = FALSE;
	sha1_initialize(&piece_checker->_cur_hash_piece->_sha1);
	piece_checker->_cur_hash_piece->_piece_checker = piece_checker;
	list_init(&piece_checker->_cur_hash_piece->_read_range_info_list);
	bt_range._pos = (_u64)range_switch->_piece_size * piece_index;
	bt_range._length = range_switch->_piece_size;
	if(piece_index == range_switch->_piece_num - 1)		//如果是最后一个piece，大小可能不够一个piece_size
		bt_range._length = range_switch->_file_size - (_u64)range_switch->_piece_size * (range_switch->_piece_num - 1);
	brs_bt_range_to_read_range_info_list(range_switch, &bt_range, &piece_checker->_cur_hash_piece->_read_range_info_list);
	sd_assert(list_size(&piece_checker->_cur_hash_piece->_read_range_info_list) > 0);
	//开始读数据
	ret = bt_checker_read_data(piece_checker);
	sd_assert(ret == SUCCESS);
	return ret;
}

_int32 bt_checker_write_tmp_file_callback(_int32 errcode, void* user_data, RANGE* padding_range)
{	//写到临时文件成功后需要通知一下，看看这个padding_range所在的piece是否可以校验了
	BT_PIECE_CHECKER* piece_checker = (BT_PIECE_CHECKER*)user_data;
	if(errcode != SUCCESS)
		return errcode;
	LOG_DEBUG("[piece_checker = 0x%x]bt_checker_write_tmp_file_callback, padding_range[%u, %u].", piece_checker, padding_range->_index, padding_range->_num);
	bdm_notify_tmp_range_write_ok(piece_checker->_bt_data_manager, padding_range);
	return bt_checker_recv_range(piece_checker, padding_range);
}
#endif

