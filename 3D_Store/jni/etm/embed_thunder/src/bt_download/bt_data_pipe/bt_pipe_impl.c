#include "utility/define.h"
#ifdef ENABLE_BT 
#ifdef ENABLE_BT_PROTOCOL

#include "../bt_task/bt_task_interface.h"
#include "connect_manager/pipe_interface.h"
#include "bt_pipe_impl.h"
#include "bt_build_cmd.h"
#include "bt_extract_cmd.h"
#include "bt_device.h"
#include "bt_cmd_define.h"
#include "bt_memory_slab.h"
#include "../bt_data_manager/bt_data_manager_interface.h"
#include "../bt_data_manager/bt_data_manager.h"
#include "../bt_utility/bt_utility.h"
#ifdef UPLOAD_ENABLE
#include "upload_manager/upload_manager.h"
#endif
#include "connect_manager/pipe_interface.h"
#include "utility/sd_assert.h"
#include "utility/utility.h"
#include "utility/time.h"
#include "utility/mempool.h"
#include "utility/logid.h"
#define	LOGID	LOGID_BT_PIPE
#include "utility/logger.h"
#include "../bt_magnet/bt_magnet_data_manager.h"

#define		BT_GET_DATA_TIMEOUT			1000
#define		BT_MAX_SENDING_PIECE_NUM	32

_int32 bt_pipe_notify_connnect_success(BT_DATA_PIPE* bt_pipe)
{
	_int32 ret = SUCCESS;
#ifndef UPLOAD_ENABLE
	bt_pipe->_choke_remote = TRUE;
#endif
	bt_pipe_change_state(bt_pipe, PIPE_CONNECTED);
    if( !bt_pipe->_is_magnet_pipe )
    {
	bt_pipe->_total_piece = bdm_get_total_piece_num((BT_DATA_MANAGER*)bt_pipe->_data_pipe._p_data_manager);
	bt_pipe->_total_size = bdm_get_file_size((BT_DATA_MANAGER*)bt_pipe->_data_pipe._p_data_manager);
	bt_pipe->_piece_size = bdm_get_piece_size((BT_DATA_MANAGER*)bt_pipe->_data_pipe._p_data_manager);
	ret = bitmap_resize(&bt_pipe->_buddy_have_bitmap, bt_pipe->_total_piece);
	if(ret != SUCCESS)
		return bt_pipe_handle_error(ret, bt_pipe);
    }

	LOG_ERROR("[bt_pipe = 0x%x]bt pipe connect success, device:0x%X, total_size = %llu, piece_size = %u.", 
		bt_pipe, bt_pipe->_device, bt_pipe->_total_size, bt_pipe->_piece_size);
	ret = bt_pipe_send_handshake_cmd(bt_pipe);
	CHECK_VALUE(ret);
	sd_assert(bt_pipe->_device->_recv_buffer == NULL);
	ret = sd_malloc(1024, (void**)&bt_pipe->_device->_recv_buffer);
	if(ret != SUCCESS)
		return bt_pipe_handle_error(ret, bt_pipe);
	bt_pipe->_device->_recv_buffer_len = 1024;
	bt_pipe->_device->_recv_buffer_offset = 0;
	ret = bt_device_recv(bt_pipe->_device, BT_HANDSHAKE_PRO_LEN);		//连接上后首先接收对方发送来的handshake命令
	if(ret != SUCCESS)
		return bt_pipe_handle_error(ret, bt_pipe);
	return ret;
}

_int32 bt_pipe_notify_send_success(BT_DATA_PIPE* bt_pipe)
{
	return sd_time_ms(&bt_pipe->_last_send_package_time);
}

_int32 bt_pipe_notify_recv_success(BT_DATA_PIPE* bt_pipe)
{
	return sd_time_ms(&bt_pipe->_last_recv_package_time);
}

_int32 bt_pipe_notify_recv_data(BT_DATA_PIPE* bt_pipe)
{
	RANGE down_range;
	BT_RANGE bt_range;
	sd_time_ms(&bt_pipe->_data_pipe._dispatch_info._last_recv_data_time);
	dp_get_bt_download_range(&bt_pipe->_data_pipe, &down_range);
	//如果down_range的_num为空,说明是被cancel掉了,不用提交
	if(list_size(&bt_pipe->_request_piece_list) == 0 && down_range._num == 1)		
	{
		//把数据提交给data_manager
		LOG_DEBUG("[bt_pipe = 0x%x]bt pipe commit data to data_manager, range[%u, %u].", bt_pipe, down_range._index, down_range._num);
		dp_switch_range_2_bt_range(&bt_pipe->_data_pipe, &down_range, &bt_range);
		pi_put_data(&bt_pipe->_data_pipe, &down_range, &bt_pipe->_data_buffer, bt_range._length, bt_pipe->_data_buffer_len, bt_pipe->_data_pipe._p_resource);
		dp_clear_bt_download_range(&bt_pipe->_data_pipe);
		bt_pipe_request_data(bt_pipe);			//继续请求新的数据
	}
	return SUCCESS;
}

_int32 bt_pipe_notify_close(BT_DATA_PIPE* bt_pipe)
{
	BT_PIECE_DATA* piece = NULL;
	bt_free_bt_device(bt_pipe->_device);
	bt_pipe->_device = NULL;
	bitmap_uninit(&bt_pipe->_buddy_have_bitmap);
	bitmap_uninit(&bt_pipe->_interested_bitmap);
	while(list_size(&bt_pipe->_request_piece_list) > 0)
	{
		list_pop(&bt_pipe->_request_piece_list, (void**)&piece);
		bt_free_bt_piece_data(piece);
	}
	if(bt_pipe->_data_buffer != NULL)
	{
		pi_free_data_buffer(&bt_pipe->_data_pipe, &bt_pipe->_data_buffer, bt_pipe->_data_buffer_len);
		bt_pipe->_data_buffer = NULL;
	}
	LOG_DEBUG("[bt_pipe = 0x%x]bt_pipe destroy success.", bt_pipe);
	uninit_pipe_info(&bt_pipe->_data_pipe);
	return bt_free_bt_data_pipe(bt_pipe);
}

_int32 bt_pipe_handle_error(_int32 errcode, BT_DATA_PIPE* bt_pipe)
{
	LOG_DEBUG("[bt_pipe = 0x%x]bt_pipe_handle_error, errcode = %d.", bt_pipe, errcode);
#ifdef UPLOAD_ENABLE
	bt_pipe_change_upload_state(bt_pipe, UPLOAD_FAILED_STATE);
#endif
	bt_pipe_change_state(bt_pipe, PIPE_FAILURE);
	set_resource_err_code(bt_pipe->_data_pipe._p_resource, errcode);
	return SUCCESS;
}

_int32 bt_pipe_handle_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid)
{
	_u32 now;
	BT_DATA_PIPE* bt_pipe = (BT_DATA_PIPE*)msg_info->_user_data;
	if(errcode == MSG_CANCELLED)
		return SUCCESS;
#ifndef UPLOAD_ENABLE
	if(bt_pipe->_choke_remote == TRUE)
	{
		LOG_DEBUG("[bt_pipe = 0x%x]bt pipe unchoke remote.", bt_pipe);
		bt_pipe_send_choke_cmd(bt_pipe, FALSE);
		bt_pipe->_choke_remote = FALSE;
	}
#endif
	sd_time_ms(&now);
	if(TIME_SUBZ(now, bt_pipe->_last_recv_package_time) >120000)	
	{	//超过2分钟没有收到过任何数据
		LOG_ERROR("[bt_pipe = 0x%x]bt_pipe_handle_timeout, but no data received in recent 120 seconds, close connection.", bt_pipe);
		return bt_pipe_handle_error(-1, bt_pipe);
	}
	if(TIME_SUBZ(now , bt_pipe->_last_send_package_time)>= 50000)
	{
		bt_pipe_send_keepalive_cmd(bt_pipe);
	}
	return SUCCESS;
}

_int32 bt_pipe_get_data_buffer_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid)
{
	_int32 ret = SUCCESS;
	_u32 offset = 0;
	BT_DATA_PIPE* bt_pipe = NULL;
	RANGE down_range;
	BT_RANGE bt_range;
	BT_PIECE_DATA* piece = (BT_PIECE_DATA*)msg_info->_device_id;
	if(errcode == MSG_CANCELLED)
	{
		bt_free_bt_piece_data(piece);
		return SUCCESS;
	}
	bt_pipe = (BT_DATA_PIPE*)msg_info->_user_data;
	bt_pipe->_get_data_buffer_timeout_id = INVALID_MSG_ID;
	bt_pipe->_data_buffer_len = get_data_unit_size();		//每次都是请求一个range
	ret = pi_get_data_buffer(&bt_pipe->_data_pipe, &bt_pipe->_data_buffer, &bt_pipe->_data_buffer_len);
	if(ret != SUCCESS)
	{
		if(ret == DATA_BUFFER_IS_FULL || ret == OUT_OF_MEMORY)
		{
			LOG_DEBUG("[bt_pipe = 0x%x]pi_get_data_buffer failed, start timer to retry.", bt_pipe);
			pipe_set_err_get_buffer(&bt_pipe->_data_pipe, TRUE);
			return start_timer(bt_pipe_get_data_buffer_timeout, NOTICE_ONCE, BT_GET_DATA_TIMEOUT, (_u32)piece, bt_pipe, &bt_pipe->_get_data_buffer_timeout_id);
		}
		else
		{
			LOG_ERROR("[bt_pipe = 0x%x]pi_get_data_buffer failed, pipe set to FAILURE.", bt_pipe);
			return bt_pipe_handle_error(ret, bt_pipe);
		}
	}
	pipe_set_err_get_buffer(&bt_pipe->_data_pipe, FALSE);
	dp_get_bt_download_range(&bt_pipe->_data_pipe, &down_range);
	dp_switch_range_2_bt_range(&bt_pipe->_data_pipe, &down_range, &bt_range);
	offset = (_u64)piece->_index * bt_pipe->_piece_size + piece->_offset - bt_range._pos;
	ret = bt_device_recv_data(bt_pipe->_device, bt_pipe->_data_buffer + offset, piece->_len);
	CHECK_VALUE(ret);
	return bt_free_bt_piece_data(piece);
}

_int32 bt_pipe_request_data(BT_DATA_PIPE* bt_pipe)
{
	_int32 ret = SUCCESS;
	LIST_ITERATOR iter;
	BT_PIECE_DATA* piece = NULL;
	RANGE down_range;
	BT_RANGE bt_range;
	if(bt_pipe->_uncomplete_range._num == 0)
	{
		return pi_notify_dispatch_data_finish(&bt_pipe->_data_pipe);		//通知调度器重新调度
	}
	dp_get_bt_download_range(&bt_pipe->_data_pipe, &down_range);
	if(down_range._num != 0)
		return SUCCESS;				//说明现在还有一个pending_range在下载
	down_range._index = bt_pipe->_uncomplete_range._index;
	down_range._num = 1;
	++bt_pipe->_uncomplete_range._index;
	--bt_pipe->_uncomplete_range._num;
	dp_set_bt_download_range(&bt_pipe->_data_pipe, &down_range);
	dp_switch_range_2_bt_range(&bt_pipe->_data_pipe, &down_range, &bt_range);
	sd_assert(bt_range._length > 0);
	LOG_DEBUG("[bt_pipe = 0x%x]bt_pipe_request_data, pending_range[%u, %u], bt_range[%llu, %u].", bt_pipe, down_range._index, down_range._num, bt_range._pos, bt_range._length);
	ret = bt_range_to_piece_data(bt_pipe, &bt_range, &bt_pipe->_request_piece_list);
	if(ret != SUCCESS)
		return ret;
	for(iter = LIST_BEGIN(bt_pipe->_request_piece_list); iter != LIST_END(bt_pipe->_request_piece_list); iter = LIST_NEXT(iter))
	{
		piece = (BT_PIECE_DATA*)LIST_VALUE(iter);
		ret = bt_pipe_send_request_cmd(bt_pipe, piece);
		if(ret != SUCCESS)
			return ret;
	}
	return SUCCESS;
}

void bt_pipe_change_state(BT_DATA_PIPE* bt_pipe, PIPE_STATE state)
{
#ifdef _DEBUG
	char pipe_state_str[7][17] = {"PIPE_IDLE", "PIPE_CONNECTING", "PIPE_CONNECTED", "PIPE_CHOKED", "PIPE_DOWNLOADING", "PIPE_FAILURE", "PIPE_SUCCESS"};
	LOG_DEBUG("[bt_pipe = 0x%x]bt_pipe_change_state, from %s to %s", bt_pipe, pipe_state_str[bt_pipe->_data_pipe._dispatch_info._pipe_state], pipe_state_str[state]);
#endif
	if((state == PIPE_CONNECTING || state == PIPE_CHOKED || state == PIPE_DOWNLOADING) && bt_pipe->_data_pipe._dispatch_info._pipe_state != state)
		sd_time_ms(&bt_pipe->_data_pipe._dispatch_info._time_stamp);	
	if(state == PIPE_DOWNLOADING)
		sd_time_ms(&bt_pipe->_data_pipe._dispatch_info._last_recv_data_time);
    //bt_pipe->_data_pipe._dispatch_info._pipe_state = state;
    dp_set_state(&bt_pipe->_data_pipe, state);
}

/*发送数据包*/
_int32 bt_pipe_send_handshake_cmd(BT_DATA_PIPE* bt_pipe)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL;
	_u32 len = 0;
	_u8* info_hash = NULL;
    
    if( bt_pipe->_is_magnet_pipe )
    {
        bt_magnet_data_manager_get_infohash((BT_MAGNET_DATA_MANAGER*)bt_pipe->_data_pipe._p_data_manager, &info_hash);
    }
    else
    {
	bdm_get_info_hash((BT_DATA_MANAGER*)bt_pipe->_data_pipe._p_data_manager, &info_hash);
    }
    
    ret = bt_build_handshake_cmd(&buffer, &len, info_hash, bt_pipe->_is_magnet_pipe);
	if(ret != SUCCESS)
		return ret;
	LOG_DEBUG("[bt_pipe = 0x%x]bt pipe send handshake cmd.", bt_pipe);
	return bt_device_send(bt_pipe->_device, buffer, len);
}

_int32 bt_pipe_send_ex_handshake_cmd(BT_DATA_PIPE* bt_pipe)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL;
	_u32 len = 0;
    ret = bt_build_ex_handshake_cmd(&buffer, &len);
	if(ret != SUCCESS)
		return ret;
	LOG_DEBUG("[bt_pipe = 0x%x]bt_pipe_send_ex_handshake_cmd.", bt_pipe);
	return bt_device_send(bt_pipe->_device, buffer, len);
}

_int32 bt_pipe_send_metadata_request_cmd(BT_DATA_PIPE* bt_pipe, _u32 piece_index)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL;
	_u32 len = 0;
    _u32 data_type = 0;
    ret = bt_magnet_logic_get_metadata_type(bt_pipe->_data_pipe._p_data_manager, 
        bt_pipe, &data_type );
    CHECK_VALUE( ret );

    ret = bt_build_metadata_request_cmd(&buffer, &len, data_type, piece_index);
	if(ret != SUCCESS)
		return ret;
	LOG_DEBUG("[bt_pipe = 0x%x]bt_pipe_send_metadata_request_cmd.", bt_pipe);
	return bt_device_send(bt_pipe->_device, buffer, len);

}

_int32 bt_pipe_send_choke_cmd(BT_DATA_PIPE* bt_pipe, BOOL is_choke)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL;
	_u32 len = 0;
	ret = bt_build_choke_cmd(&buffer, &len, is_choke);
	if(ret != SUCCESS)
		return ret;
	LOG_DEBUG("[bt_pipe = 0x%x]bt_pipe_send_choke_cmd.", bt_pipe);
	return bt_device_send(bt_pipe->_device, buffer, len);
}

_int32 bt_pipe_send_bitfield_cmd(BT_DATA_PIPE* bt_pipe, const BITMAP* bitmap_i_have)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL;
	_u32 len = 0;
	ret = bt_build_bitfield_cmd(&buffer, &len, bitmap_i_have);
	if(ret != SUCCESS)
		return ret;
	LOG_DEBUG("[bt_pipe = 0x%x]bt pipe send bitfield cmd.", bt_pipe);
	return bt_device_send(bt_pipe->_device, buffer, len);
}

_int32 bt_pipe_send_interested_cmd(BT_DATA_PIPE* bt_pipe, BOOL is_interested)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL;
	_u32 len = 0;
	ret = bt_build_interested_cmd(&buffer, &len, is_interested);
	if(ret != SUCCESS)
		return ret;
	LOG_DEBUG("[bt_pipe = 0x%x]bt pipe send interested cmd.", bt_pipe);
	return bt_device_send(bt_pipe->_device, buffer, len);
}

_int32 bt_pipe_send_have_cmd(BT_DATA_PIPE* bt_pipe, _u32 piece_index)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL;
	_u32 len = 0;
	ret = bt_build_have_cmd(&buffer, &len, piece_index);
	if(ret != SUCCESS)
		return ret;
	LOG_DEBUG("[bt_pipe = 0x%x]bt_pipe_send_have_cmd.", bt_pipe);
	return bt_device_send(bt_pipe->_device, buffer, len);
}

_int32 bt_pipe_send_request_cmd(BT_DATA_PIPE* bt_pipe, BT_PIECE_DATA* piece)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL;
	_u32 len = 0;
	ret = bt_build_request_cmd(&buffer, &len, piece);
	if(ret != SUCCESS)
	{
		sd_assert(FALSE);
		return ret;
	}
	LOG_DEBUG("[bt_pipe = 0x%x]bt pipe send request cmd, request piece[%u, %u, %u]", bt_pipe, piece->_index, piece->_offset, piece->_len);
	sd_time_ms(&bt_pipe->_data_pipe._dispatch_info._last_recv_data_time);
	return bt_device_send(bt_pipe->_device, buffer, len);
}

_int32 bt_pipe_send_cancel_cmd(BT_DATA_PIPE* bt_pipe, BT_PIECE_DATA* piece)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL;
	_u32 len = 0;
	ret = bt_build_cancel_cmd(&buffer, &len, piece);
	if(ret != SUCCESS)
		return ret;
	LOG_DEBUG("[bt_pipe = 0x%x]bt_pipe_send_cancel_cmd.", bt_pipe);
	return bt_device_send(bt_pipe->_device, buffer, len);
}

_int32 bt_pipe_send_piece_cmd(BT_DATA_PIPE* bt_pipe, BT_PIECE_DATA* piece, char* data)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL;
	_u32 len = 0;
	ret = bt_build_piece_cmd(&buffer, &len, piece, data);
	if(ret != SUCCESS)
		return ret;
	LOG_DEBUG("[bt_pipe = 0x%x]bt_pipe_send_piece_cmd.", bt_pipe);

	return bt_device_send_in_speed_limit(bt_pipe->_device, buffer, len);

}

_int32 bt_pipe_send_a0_cmd(BT_DATA_PIPE* bt_pipe)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL;
	_u32 len = 0;
	ret = bt_build_a0_cmd(&buffer, &len);
	if(ret != SUCCESS)
		return ret;
	LOG_DEBUG("[bt_pipe = 0x%x]bt pipe send ex_a0 cmd.", bt_pipe);
	return bt_device_send(bt_pipe->_device, buffer, len);
}

_int32 bt_pipe_send_a1_cmd(BT_DATA_PIPE* bt_pipe)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL;
	_u32 len = 0;
	ret = bt_build_a1_cmd(&buffer, &len);
	if(ret != SUCCESS)
		return ret;
	LOG_DEBUG("[bt_pipe = 0x%x]bt pipe send a1 cmd.", bt_pipe);
	return bt_device_send(bt_pipe->_device, buffer, len);
}

_int32 bt_pipe_send_a2_cmd(BT_DATA_PIPE* bt_pipe)
{
	//收到a1_cmd,发送a2_cmd, 发一个假的peer过去敷衍
	_int32 ret = SUCCESS;
	char* buffer = NULL;
	_u32 len = 0;
	char bt_peerid[BT_PEERID_LEN] = {0};
	ret = bt_get_local_peerid(bt_peerid, BT_PEERID_LEN);
	CHECK_VALUE(ret);
	ret = bt_build_a2_cmd(&buffer, &len, bt_peerid, 0, (_u16)(sd_rand() % 65535));
	if(ret != SUCCESS)
		return ret;
	LOG_DEBUG("[bt_pipe = 0x%x]bt pipe send a2 cmd.", bt_pipe);
	return bt_device_send(bt_pipe->_device, buffer, len);
}

_int32 bt_pipe_send_keepalive_cmd(BT_DATA_PIPE* bt_pipe)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL;
	_u32 buffer_len = 4;
	ret = sd_malloc(buffer_len, (void**)&buffer);
	if(ret != SUCCESS)
		return ret;
	LOG_DEBUG("[bt_pipe = 0x%x]bt_pipe_send_keepalive_cmd.", bt_pipe);
	sd_memset(buffer, 0, buffer_len);
	return bt_device_send(bt_pipe->_device, buffer, buffer_len);
}

/*处理收到的数据包*/
_int32 bt_pipe_recv_cmd_package(BT_DATA_PIPE* bt_pipe, _u8 cmd_type)
{
	LOG_DEBUG("[bt_pipe = 0x%x]bt_pipe_recv_cmd_package,cmd_type=%d",bt_pipe,cmd_type);
	bt_pipe_notify_recv_success(bt_pipe);		//更新最后收到数据包的时间

    if(bt_pipe->_is_magnet_pipe)
    {
        if( cmd_type != BT_HANDSHAKE 
            && cmd_type != BT_MAGNET 
            && cmd_type != BT_KEEPALIVE )
            return SUCCESS;
    }
    
	switch(cmd_type)	{
	case BT_CHOKE:
		return bt_pipe_handle_choke_cmd(bt_pipe);
	case BT_UNCHOKE:
		return bt_pipe_handle_unchoke_cmd(bt_pipe);
	case BT_INTERESTED:
		return bt_pipe_handle_interested_cmd(bt_pipe);
	case BT_NOT_INTERESTED:
		return bt_pipe_handle_not_interested_cmd(bt_pipe);
	case BT_HAVE:
		return bt_pipe_handle_have_cmd(bt_pipe);
	case BT_BITFIELD:
		return bt_pipe_handle_bitfield_cmd(bt_pipe);
	case BT_REQUEST:
		return bt_pipe_handle_request_cmd(bt_pipe);
	case BT_PIECE:
		return bt_pipe_handle_piece_cmd(bt_pipe);
	case BT_CANCEL:
		return bt_pipe_handle_cancel_cmd(bt_pipe);
	case BT_PORT:
		return bt_pipe_handle_port_cmd(bt_pipe);
	case BT_HANDSHAKE:
		return bt_pipe_handle_handshake_cmd(bt_pipe);
	case BT_A0:
		return bt_pipe_handle_a0_cmd(bt_pipe);
	case BT_A1:
		return bt_pipe_handle_a1_cmd(bt_pipe);
	case BT_A2:
		return bt_pipe_handle_a2_cmd(bt_pipe);
	case BT_A3:
		return bt_pipe_handle_a3_cmd(bt_pipe);
	case BT_AC:
		return bt_pipe_handle_ac_cmd(bt_pipe);
	case BT_KEEPALIVE:
		return bt_pipe_handle_keepalive_cmd(bt_pipe);
	case BT_MAGNET:
		return bt_pipe_handle_magnet_cmd(bt_pipe);        
//		sd_assert(FALSE);
	}
	return SUCCESS;
}

_int32 bt_pipe_handle_choke_cmd(BT_DATA_PIPE* bt_pipe)
{
	BT_PIECE_DATA* piece = NULL;
	LOG_DEBUG("[bt_pipe = 0x%x]bt_pipe_handle_choke_cmd.", bt_pipe);
	bt_pipe_change_state(bt_pipe, PIPE_CHOKED);
	while(list_size(&bt_pipe->_request_piece_list) > 0)
	{
		list_pop(&bt_pipe->_request_piece_list, (void**)&piece);
		bt_free_bt_piece_data(piece);
	}
	dp_clear_bt_download_range(&bt_pipe->_data_pipe);
	return SUCCESS;
}

_int32 bt_pipe_handle_unchoke_cmd(BT_DATA_PIPE* bt_pipe)
{
	LOG_DEBUG("[bt_pipe = 0x%x]bt_pipe_handle_unchoke_cmd.", bt_pipe);
	bt_pipe->_is_ever_unchoke = TRUE;
	bt_pipe_change_state(bt_pipe, PIPE_DOWNLOADING);
	return bt_pipe_request_data(bt_pipe);
}

_int32 bt_pipe_handle_interested_cmd(BT_DATA_PIPE* bt_pipe)
{
	_int32 ret = SUCCESS;
#ifdef UPLOAD_ENABLE
	_u8* info_hash = NULL;
#endif
	LOG_DEBUG("[bt_pipe = 0x%x]bt_pipe_handle_interested_cmd.", bt_pipe);
#ifdef UPLOAD_ENABLE

    if( !bt_pipe->_is_magnet_pipe )
    {
	    bdm_get_info_hash((BT_DATA_MANAGER*)bt_pipe->_data_pipe._p_data_manager, &info_hash);
    }
    else
    {
        sd_assert( FALSE );
    }

	ulm_add_pipe_by_info_hash(info_hash, &bt_pipe->_data_pipe);
#else
	if(bt_pipe->_choke_remote == TRUE)
	{
		bt_pipe->_choke_remote = FALSE;
		ret = bt_pipe_send_choke_cmd(bt_pipe, FALSE);
	}
#endif
	return ret;
}

_int32 bt_pipe_handle_not_interested_cmd(BT_DATA_PIPE* bt_pipe)
{
	LOG_DEBUG("[bt_pipe = 0x%x]bt_pipe_handle_not_interested_cmd.", bt_pipe);
#ifdef UPLOAD_ENABLE
	bt_pipe_change_upload_state(bt_pipe, UPLOAD_FIN_STATE);
#endif
	return SUCCESS;
}

_int32 bt_pipe_handle_have_cmd(BT_DATA_PIPE* bt_pipe)
{
	_int32 ret = SUCCESS;
	BT_RANGE bt_range;
	_u32 index = 0, offset = 1,start_index, end_index, i, len;
	ret = bt_extract_have_cmd(bt_pipe->_device->_recv_buffer + BT_PRO_HEADER_LEN, bt_pipe->_device->_recv_buffer_offset - BT_PRO_HEADER_LEN, &index);
	if(ret != SUCCESS)
		return ret;
	if(index >= bt_pipe->_total_piece)
	{
		return bt_pipe_handle_error(-1, bt_pipe);	//协议错误
	}
	bitmap_set(&bt_pipe->_buddy_have_bitmap, index, TRUE);
	if(bt_pipe->_piece_size < get_data_unit_size())
		offset = get_data_unit_size() / bt_pipe->_piece_size;		//计算出需要向前向后查看的偏移数
	start_index = index;
	end_index = index;
	//找出起始的piece
	for(i = 0; i < offset; ++i)
	{
		if(start_index != 0 && bitmap_at(&bt_pipe->_buddy_have_bitmap, start_index - 1) == TRUE)
		{
			--start_index;
		}
		else
		{
			break;
		}
	}
	//找出结尾的piece
	for(i = 0; i < offset; ++i)
	{
		if(end_index != bt_pipe->_total_piece - 1 && bitmap_at(&bt_pipe->_buddy_have_bitmap, end_index + 1) == TRUE)
		{
			++end_index;
		}
		else
		{
			break;
		}
	}
	//接着把对方的bitmap转为BT_RANGE,再把BT_RANGE转为加了pengding的can_downlaod_range
	bt_range._pos = (_u64)bt_pipe->_piece_size * start_index;
	bt_range._length = 0;
	for(i = start_index; i < end_index + 1; ++i)
	{		
			len = bt_pipe->_piece_size;
			if(i == bt_pipe->_total_piece - 1)
				len = bt_pipe->_total_size - (_u64)bt_pipe->_piece_size * (bt_pipe->_total_piece - 1);
			bt_range._length += len;
	}
	LOG_DEBUG("bt_pipe_handle_have_cmd, piece_index = %u, bt_range[%llu, %llu].", index, bt_range._pos, bt_range._length);
	return dp_add_bt_can_download_ranges(&bt_pipe->_data_pipe, &bt_range);
	//这个data_manager还没有实现需要的功能，暂时没实现

	//if(dm_is_piece_need_download(index) && !dm_is_piece_ok(index))
	//{
	//	//我对对方刚下完的index感兴趣
	//	bitmap_set(bt_pipe->_interested_bitmap, index, TRUE);
	//	bt_send_interest_cmd(bt_pipe->_device, TRUE);
	//	if(bt_pipe->_choke_by_remote == FALSE)
	//	{
	//		//			m_range_queue_can_download += bt_piece_range_utility::piece_2_range(n_piece_index, m_p_bt_data_manager_wrapper->get_piece_size(), m_p_bt_data_manager_wrapper->get_piece_size(n_piece_index));
	//		bt_pipe_request_piece(bt_pipe);
	//	}

	//}
}

_int32 bt_pipe_handle_bitfield_cmd(BT_DATA_PIPE* bt_pipe)
{
	BT_RANGE bt_range;
	_int32 ret = SUCCESS;
	char* data = NULL;
	_u32 data_len = 0, i = 0;
	_u64 pos = 0;
	_u32 len = 0;
	ret = bt_extract_bitfield_cmd(bt_pipe->_device->_recv_buffer + BT_PRO_HEADER_LEN, bt_pipe->_device->_recv_buffer_offset - BT_PRO_HEADER_LEN, &data, &data_len);
	if(ret != SUCCESS)
		return ret;
	LOG_DEBUG("[bt_pipe = 0x%x]bt_pipe_handle_bitfield_cmd.", bt_pipe);
	ret = bitmap_from_bits(&bt_pipe->_buddy_have_bitmap, data, data_len, bt_pipe->_total_piece);
	if(ret != SUCCESS)
	{
		LOG_ERROR("[bt_pipe = 0x%x]bt_pipe_handle_bitfield_cmd, bitmap_from_bits failed, errcode = %d.", bt_pipe, ret);
		return ret;
	}
	//接着把对方的bitmap转为BT_RANGE,再把BT_RANGE转为加了pengding的can_downlaod_range
	bt_range._pos = 0;
	bt_range._length = 0;
	for(i = 0; i < bt_pipe->_total_piece; ++i)
	{
		if(bitmap_at(&bt_pipe->_buddy_have_bitmap, i) == TRUE)
		{
			pos = (_u64)bt_pipe->_piece_size * i;
			len = bt_pipe->_piece_size;
			if(i == bt_pipe->_total_piece - 1)
				len = bt_pipe->_total_size - (_u64)bt_pipe->_piece_size * (bt_pipe->_total_piece - 1);
			if(bt_range._length == 0)
			{
				bt_range._pos = pos;
				bt_range._length = len;
			}
			else
			{
				bt_range._length += len;
			}
		}
		else
		{
			if(bt_range._length != 0)
			{
				LOG_DEBUG("[bt_pipe = 0x%x]dp_add_bt_can_download_ranges, bt_range[%llu, %llu].", bt_pipe, bt_range._pos, bt_range._length);
				sd_assert(bt_range._length != 0);
				dp_add_bt_can_download_ranges(&bt_pipe->_data_pipe, &bt_range);
				bt_range._length = 0;		//初始状态
			}
		}
	}
	if(bt_range._length != 0)
	{	//最后一块有效
		LOG_DEBUG("[bt_pipe = 0x%x]dp_add_bt_can_download_ranges, bt_range[%llu, %llu].", bt_pipe, bt_range._pos, bt_range._length);
		sd_assert(bt_range._length != 0);
		dp_add_bt_can_download_ranges(&bt_pipe->_data_pipe, &bt_range);
	}
	dp_adjust_uncomplete_2_can_download_ranges(&bt_pipe->_data_pipe);
	return bt_pipe_send_interested_cmd(bt_pipe, TRUE);		//不管对端有没有我需要的内容，我都希望它unchoke我
}

_int32 bt_pipe_handle_request_cmd(BT_DATA_PIPE* bt_pipe)
{	_int32 ret = SUCCESS;
	BT_PIECE_DATA* piece = NULL;
#ifdef UPLOAD_ENABLE
	if(bt_pipe->_data_pipe._dispatch_info._pipe_upload_state != UPLOAD_UNCHOKE_STATE)
		return SUCCESS;				//并不处于unchoke，不处理request命令
#endif
	ret = bt_malloc_bt_piece_data(&piece);
	LOG_DEBUG("[bt_pipe = 0x%x]bt_pipe_handle_request_cmd,ret=%d",bt_pipe,ret);
	if(ret != SUCCESS)
		return ret;
	ret = bt_extract_request_cmd(bt_pipe->_device->_recv_buffer + BT_PRO_HEADER_LEN, bt_pipe->_device->_recv_buffer_offset - BT_PRO_HEADER_LEN, piece);
	LOG_DEBUG("[bt_pipe = 0x%x]bt_extract_request_cmd:ret=%d,_index=%u,_len=%u,_offset=%u",bt_pipe,ret,piece->_index,piece->_len,piece->_offset);
	if(ret != SUCCESS)
	{
		bt_free_bt_piece_data(piece);
		return ret;
	}
	if(bt_pipe_is_piece_valid(bt_pipe, piece) == FALSE)
	{	//无效的块请求
		LOG_DEBUG("[bt_pipe = 0x%x]bt_pipe_handle_request_cmd:the piece is unvalid,list_size=%u",bt_pipe,list_size(&bt_pipe->_upload_piece_list));
		bt_free_bt_piece_data(piece);
		return -1;
	}
    if( !bt_pipe->_is_magnet_pipe )
    {
	if(bdm_is_piece_ok((BT_DATA_MANAGER*)bt_pipe->_data_pipe._p_data_manager, piece->_index) == FALSE)
	{
		bt_free_bt_piece_data(piece);
		return ret;
	}
    }
    else
    {
        sd_assert( FALSE );
    }
	LOG_DEBUG("[bt_pipe = 0x%x]bt_pipe_handle_request_cmd:ok,the piece is valid,list_size=%u",bt_pipe,list_size(&bt_pipe->_upload_piece_list));
	//我当前是否有这块数据
	//if(!dm_is_piece_ok(bt_pipe->_data_pipe._p_data_manager, piece->_index))
	//{
	//	LOG_ERROR("[bt_pipe = 0x%x]bt_pipe_handle_request, but I have not this piece, piece_index = %u.", bt_pipe, piece->_index);
	//	return;
	//}
	//当前正在发送的片数 > BT_MAX_SENDING_PIECE_NUM
	if(list_size(&bt_pipe->_upload_piece_list) > BT_MAX_SENDING_PIECE_NUM) 
	{
		LOG_DEBUG("[bt_pipe = 0x%x]too many upload piece, ignore new request[%u, %u, %u]", bt_pipe, piece->_index, piece->_offset, piece->_len);
		return SUCCESS;
	}	
	list_push(&bt_pipe->_upload_piece_list, piece);
	return bt_pipe_upload_piece_data(bt_pipe);
}

_int32 bt_pipe_handle_piece_cmd(BT_DATA_PIPE* bt_pipe)
{
	_int32 ret = SUCCESS;
	LIST_ITERATOR iter;
	BT_PIECE_DATA piece;
	BT_PIECE_DATA* req_piece = NULL;
	char* buffer = NULL;
	_u32 offset = 0;
	RANGE down_range;
	BT_RANGE bt_range;
	_u32 package_len = sd_ntohl(*(_u32*)bt_pipe->_device->_recv_buffer);
	ret = bt_extract_piece_cmd(bt_pipe->_device->_recv_buffer + BT_PRO_HEADER_LEN, bt_pipe->_device->_recv_buffer_offset - BT_PRO_HEADER_LEN, &piece);
	if(ret != SUCCESS)
		return ret;
	for(iter = LIST_BEGIN(bt_pipe->_request_piece_list); iter != LIST_END(bt_pipe->_request_piece_list); iter = LIST_NEXT(iter))
	{
		req_piece = (BT_PIECE_DATA*)LIST_VALUE(iter);
		if(req_piece->_index == piece._index && req_piece->_offset == piece._offset)
			break;
	}
	if(iter == LIST_END(bt_pipe->_request_piece_list))
	{	//这是一个无效的回复，把无效的数据收回来
		ret = sd_malloc(package_len - 9, (void**)&buffer);
		if(ret != SUCCESS)
			return ret;
		LOG_DEBUG("[bt_pipe = 0x%x]bt_device_recv_discard_data, discard piece[%u, %u, %u].", bt_pipe, piece._index, piece._offset, package_len - 9);
		return bt_device_recv_discard_data(bt_pipe->_device, buffer, package_len - 9);
	}
	list_erase(&bt_pipe->_request_piece_list, iter);
	LOG_DEBUG("[bt_pipe = 0x%x]bt_pipe_handle_piece_cmd, piece[%u, %u, %u].", bt_pipe, req_piece->_index, req_piece->_offset, req_piece->_len);
	if(bt_pipe->_data_buffer == NULL)
	{
		bt_pipe->_data_buffer_len = get_data_unit_size();		//每次都是请求一个range
		ret = pi_get_data_buffer(&bt_pipe->_data_pipe, &bt_pipe->_data_buffer, &bt_pipe->_data_buffer_len);
		if(ret != SUCCESS)
		{
			if(ret == DATA_BUFFER_IS_FULL || ret == OUT_OF_MEMORY)
			{
				LOG_DEBUG("[bt_pipe = 0x%x]pi_get_data_buffer failed, start timer to get data buffer later.", bt_pipe);
				start_timer(bt_pipe_get_data_buffer_timeout, NOTICE_ONCE, BT_GET_DATA_TIMEOUT, (_u32)req_piece, bt_pipe, &bt_pipe->_get_data_buffer_timeout_id);
				return BT_PIPE_GET_DATA_BUFFER_FAILED;
			}
			else
			{
				LOG_ERROR("[bt_pipe = 0x%x]pi_get_data_buffer failed, errcode = %d", bt_pipe, ret);
				return bt_free_bt_piece_data(req_piece);
			}
		}
	}
	dp_get_bt_download_range(&bt_pipe->_data_pipe, &down_range);
	dp_switch_range_2_bt_range(&bt_pipe->_data_pipe, &down_range, &bt_range);
	offset = (_u64)req_piece->_index * bt_pipe->_piece_size + req_piece->_offset - bt_range._pos;
	ret = bt_device_recv_data(bt_pipe->_device, bt_pipe->_data_buffer + offset, req_piece->_len);
	bt_free_bt_piece_data(req_piece);
	if(ret == SUCCESS)
		return BT_PIPE_RECV_PIECE_DATA;
	else
		return ret;
}

_int32 bt_pipe_handle_cancel_cmd(BT_DATA_PIPE* bt_pipe)
{
	LIST_ITERATOR iter;
	BT_PIECE_DATA* upload_piece = NULL;
	_int32 ret = SUCCESS;
	BT_PIECE_DATA piece;
	ret = bt_extract_cancel_cmd(bt_pipe->_device->_recv_buffer + BT_PRO_HEADER_LEN, bt_pipe->_device->_recv_buffer_offset - BT_PRO_HEADER_LEN, &piece);
	if(ret != SUCCESS)
		return ret;
	for(iter = LIST_BEGIN(bt_pipe->_upload_piece_list); iter != LIST_END(bt_pipe->_upload_piece_list); iter = LIST_NEXT(iter))
	{
		upload_piece = (BT_PIECE_DATA*)LIST_VALUE(iter);
		if(upload_piece->_index == piece._index && upload_piece->_offset == piece._offset && upload_piece->_len == piece._len)
			break;
	}
	if(iter == LIST_END(bt_pipe->_upload_piece_list))
		return SUCCESS;
	list_erase(&bt_pipe->_upload_piece_list, iter);
	LOG_DEBUG("[bt_pipe = 0x%x]bt_pipe_handle_cancel_cmd, cancel upload piece[%u, %u, %u].", bt_pipe, upload_piece->_index, upload_piece->_offset, upload_piece->_len);
	return bt_free_bt_piece_data(upload_piece);
}

_int32 bt_pipe_handle_port_cmd(BT_DATA_PIPE* bt_pipe)
{
	_int32 ret = SUCCESS;
	_u16 port = 0;
	ret = bt_extract_port_cmd(bt_pipe->_device->_recv_buffer + BT_PRO_HEADER_LEN, bt_pipe->_device->_recv_buffer_offset - BT_PRO_HEADER_LEN, &port);
	if(ret != SUCCESS)
		return ret;
	return SUCCESS;
}

_int32 bt_pipe_handle_handshake_cmd(BT_DATA_PIPE* bt_pipe)
{
	_int32 ret = SUCCESS;
	_u8 remote_info_hash[BT_INFO_HASH_LEN] = {0};
	_u8* my_info_hash = NULL;
	char local_peerid[BT_PEERID_LEN + 1] = {0};
	char resevered[8] = {0};
	BITMAP* bitmap_i_have = NULL;
	LOG_DEBUG("[bt_pipe = 0x%x]bt_pipe_handle_handshake_cmd.", bt_pipe);
	ret = bt_extract_handshake_cmd(bt_pipe->_device->_recv_buffer
	    , bt_pipe->_device->_recv_buffer_offset, 
        remote_info_hash, bt_pipe->_remote_peerid, resevered );
    LOG_DEBUG("remote_info_hash = %s, remote_peerid = %s"
            , remote_info_hash, bt_pipe->_remote_peerid);
	if(ret != SUCCESS)
		return ret;
    if( bt_pipe->_is_magnet_pipe )
    {
        bt_magnet_data_manager_get_infohash((BT_MAGNET_DATA_MANAGER*)bt_pipe->_data_pipe._p_data_manager, &my_info_hash);
    }
    else
    {
	bdm_get_info_hash((BT_DATA_MANAGER*)bt_pipe->_data_pipe._p_data_manager, &my_info_hash);
    }
	//收到对方的握手包
	if(sd_strncmp((const char*)remote_info_hash, (const char*)my_info_hash, BT_INFO_HASH_LEN) != 0)
	{
		LOG_ERROR("[bt_pipe = 0x%x]bt_recv_handshake_cmd, but info hash not identical.", bt_pipe);
		return -1;
	}
	bt_get_local_peerid(local_peerid, BT_PEERID_LEN);
	if(sd_strcmp(local_peerid, bt_pipe->_remote_peerid) == 0)
	{	//不允许自己连自己
		LOG_ERROR("[bt_pipe = 0x%x]bt_recv_handshake_cmd, but peerid is the same, not allow.", bt_pipe);
		return -1;
	}
    if(bt_pipe->_is_magnet_pipe )
    {
        if(resevered[5]!='\x10')
        {
            //不支持扩展协议
    		return -1;
        }    
        if( !bt_is_UT_peer(bt_pipe->_remote_peerid) )
        {
            //磁力连接目前只连ut
    		return -1;
        }    
        bt_pipe_send_ex_handshake_cmd(bt_pipe);
        return SUCCESS;
    }

	//握手成功
	start_timer(bt_pipe_handle_timeout, NOTICE_INFINITE, 10000, 0, bt_pipe, &bt_pipe->_timeout_id);	//启动一个10秒定时器
	//发送bitfield
	bdm_get_complete_bitmap((BT_DATA_MANAGER*)bt_pipe->_data_pipe._p_data_manager, &bitmap_i_have);
	bt_pipe_send_bitfield_cmd(bt_pipe, bitmap_i_have);
	if(bt_is_bitcomet_peer(bt_pipe->_remote_peerid))
	{	//对方是bitcomet
		bt_pipe_send_a0_cmd(bt_pipe);
		bt_pipe_send_a1_cmd(bt_pipe);
	}
	return SUCCESS;
}

_int32 bt_pipe_handle_a0_cmd(BT_DATA_PIPE* bt_pipe)
{
	LOG_DEBUG("[bt_pipe = 0x%x]bt_pipe_handle_a0_cmd.", bt_pipe);
	return SUCCESS;
}

_int32 bt_pipe_handle_a1_cmd(BT_DATA_PIPE* bt_pipe)
{
	LOG_DEBUG("[bt_pipe = 0x%x]bt_pipe_handle_a1_cmd.", bt_pipe);
	//if (!m_p_bt_download_param->is_peer_exchange_enable())//没有启动peer交换
	//	return;
	return bt_pipe_send_a2_cmd(bt_pipe);
}

_int32 bt_pipe_handle_a2_cmd(BT_DATA_PIPE* bt_pipe)
{
	LOG_DEBUG("[bt_pipe = 0x%x]bt_pipe_handle_a2_cmd.", bt_pipe);
	//暂时没有实现
	return SUCCESS;
}

_int32 bt_pipe_handle_a3_cmd(BT_DATA_PIPE* bt_pipe)
{
	LOG_DEBUG("[bt_pipe = 0x%x]bt_pipe_handle_a3_cmd.", bt_pipe);
	//暂时没实现
	return SUCCESS;
}

_int32 bt_pipe_handle_ac_cmd(BT_DATA_PIPE* bt_pipe)
{
	LOG_DEBUG("[bt_pipe = 0x%x]bt_pipe_handle_ac_cmd, but I don't known how to do with this.",bt_pipe);
	return SUCCESS;
}

_int32 bt_pipe_handle_keepalive_cmd(BT_DATA_PIPE* bt_pipe)
{
	LOG_DEBUG("[bt_pipe = 0x%x]bt_pipe_handle_keepalive_cmd.", bt_pipe);
	return SUCCESS;
}

_int32 bt_pipe_handle_magnet_cmd(BT_DATA_PIPE* bt_pipe)
{
    _u32 ret_val = SUCCESS;
	_u32 package_len = sd_ntohl(*(_u32*)(bt_pipe->_device->_recv_buffer));

    if( bt_pipe->_device->_recv_buffer[5] == 0 )
    {
		sd_assert( package_len > 6 );
		ret_val = bt_pipe_handle_ex_handshake_cmd( bt_pipe, bt_pipe->_device->_recv_buffer + 6, package_len - 2 );
    }
    else if( bt_pipe->_device->_recv_buffer[5] == 0x03 )
    {
		if( package_len > 6 )
			ret_val = bt_pipe_handle_metadata_cmd( bt_pipe, bt_pipe->_device->_recv_buffer + 6, package_len - 2 );
    }
	else 
	{
		LOG_DEBUG("bt_pipe_handle_magnet_cmd, unknown message %d",
			bt_pipe->_device->_recv_buffer[5]);
		return ret_val;
	}
    return ret_val;
}

_int32 bt_pipe_handle_ex_handshake_cmd(BT_DATA_PIPE* bt_pipe, char *p_data_buffer, _u32 data_len )
{
    _u32 ret_val = SUCCESS;
    _u32 metadata_type = 0, data_size = 0;
    _u32 piece_count = 0, piece_index = 0;

    ret_val = bt_extract_ex_handshake_cmd( p_data_buffer, data_len, &metadata_type, &data_size );
    if( ret_val ) return ret_val;

	if(data_size == 0) return BT_INVALID_CMD;
	
	piece_count = (data_size-1) / BT_METADATA_PIECE_SIZE + 1;

	// don't handle, if size > 10M
	if (piece_count * BT_METADATA_PIECE_SIZE > 1024*1024*10) return BT_INVALID_CMD;

    ret_val = bt_magnet_data_manager_set_metadata_size(bt_pipe->_data_pipe._p_data_manager,
        bt_pipe, data_size, piece_count );

    ret_val = bt_magnet_logic_set_metadata_type(bt_pipe->_data_pipe._p_data_manager,
        bt_pipe, metadata_type );    
    CHECK_VALUE( ret_val );

    for( ; piece_index < piece_count; piece_index++ )
    {
        bt_pipe_send_metadata_request_cmd(bt_pipe, piece_index);
    }
    
    return SUCCESS;
}

_int32 bt_pipe_handle_metadata_cmd(BT_DATA_PIPE* bt_pipe, char *p_data_buffer, _u32 data_len )
{
    _u32 ret_val = SUCCESS;
    char *p_metadata = NULL;
    _u32 metadata_len = 0;
    _u32 piece_index = 0;

    ret_val = bt_extract_metadata( p_data_buffer, data_len, &piece_index, &p_metadata, &metadata_len );
    if( ret_val ) return ret_val;

    ret_val = bt_magnet_data_manager_write_data(bt_pipe->_data_pipe._p_data_manager,
        bt_pipe, piece_index, p_metadata, metadata_len );    
    CHECK_VALUE( ret_val );

    return SUCCESS;
}


/*辅助函数*/
_int32 bt_range_to_piece_data(BT_DATA_PIPE* bt_pipe, const BT_RANGE* down_range, LIST* piece_list)
{
	_int32 ret = SUCCESS;
	BT_PIECE_DATA* data = NULL;
	BT_RANGE range;
	range._pos = down_range->_pos;
	range._length = down_range->_length;
	while(range._length > 0)
	{
		ret = bt_malloc_bt_piece_data(&data);
		if(ret != SUCCESS)
			return ret;
		data->_index = (_u32)(range._pos / bt_pipe->_piece_size);
		data->_offset = (_u32)(range._pos - (_u64)bt_pipe->_piece_size * data->_index);
		data->_len = MIN(16384, range._length);
		data->_len = MIN(data->_len, bt_pipe->_piece_size - data->_offset);		//还有注意每个请求的范围不能超过这个piece
		list_push(piece_list, data);
		range._pos += data->_len;
		range._length -= data->_len;
	}
	return SUCCESS;
}

BOOL bt_pipe_is_piece_valid(BT_DATA_PIPE* bt_pipe, BT_PIECE_DATA* piece)
{
	_u64 begin_pos;
	if(piece->_len == 0)
		return FALSE;
	begin_pos = (_u64)piece->_index * bt_pipe->_piece_size + piece->_offset;
	LOG_DEBUG("[bt_pipe = 0x%x]bt_pipe_is_piece_valid:begin_pos=%llu,_index=%u,_len=%u,_offset=%u,_total_size=%llu,_total_piece=%u,_piece_size=%u",
		bt_pipe,begin_pos,piece->_index,piece->_len,piece->_offset,bt_pipe->_total_size,bt_pipe->_total_piece,bt_pipe->_piece_size);
	if(begin_pos >= bt_pipe->_total_size)
		return FALSE;
	if(begin_pos + piece->_len > bt_pipe->_total_size)
		return FALSE;
	if(piece->_index != bt_pipe->_total_piece - 1 && piece->_offset + piece->_len > bt_pipe->_piece_size)
		return FALSE;
	if(begin_pos / bt_pipe->_piece_size != (begin_pos + piece->_len - 1) / bt_pipe->_piece_size)		//必须在一个piece内
		return FALSE;
	else
		return TRUE;
}

_int32 bt_pipe_upload_piece_data(BT_DATA_PIPE* bt_pipe)
{
	char* data_buffer = NULL;
	BT_PIECE_DATA* piece = NULL;
	BT_RANGE *bt_range=NULL;
	_int32 ret = SUCCESS;
	LOG_DEBUG( "[bt_pipe = 0x%x]bt_pipe_upload_piece_data:_reading_piece = 0x%x,list_size(&bt_pipe->_upload_piece_list)=%u",bt_pipe,bt_pipe->_reading_piece ,list_size(&bt_pipe->_upload_piece_list));
	if(bt_pipe->_reading_piece != NULL)
		return SUCCESS;			//目前已有piece在读取数据上传
	if(list_size(&bt_pipe->_upload_piece_list) == 0)
		return SUCCESS;		//没有数据需要上传
	list_pop(&bt_pipe->_upload_piece_list, (void**)&piece);
	bt_pipe->_reading_piece= piece;
	ret = sd_malloc(sizeof(BT_RANGE), (void**)&bt_range);
	if( ret!=SUCCESS ) goto ErrorHanle;
	
	bt_range->_pos = (_u64)bt_pipe->_piece_size * piece->_index + piece->_offset;
	bt_range->_length = piece->_len;
	ret = sd_malloc(bt_range->_length, (void**)&data_buffer);
	LOG_DEBUG( " [0x%x]bt_pipe_upload_piece_data 2:_reading_piece = 0x%x,list_size(&bt_pipe->_upload_piece_list)=%u,_piece_size=%u,_index=%u,_offset=%u,bt_range._pos=%llu,bt_range._length=%llu,ret=%d",
		bt_pipe,bt_pipe->_reading_piece ,list_size(&bt_pipe->_upload_piece_list),bt_pipe->_piece_size,piece->_index,piece->_offset,bt_range->_pos,bt_range->_length,ret);
	if( ret!=SUCCESS ) goto ErrorHanle;
    if( !bt_pipe->_is_magnet_pipe )
    {
#ifdef UPLOAD_ENABLE
	ret = ulm_bt_pipe_read_data(bt_pipe->_data_pipe._p_data_manager, bt_range, data_buffer, bt_range->_length, bt_pipe_read_data_callback, &bt_pipe->_data_pipe);
#else
	ret = bdm_bt_pipe_read_data_buffer((BT_DATA_MANAGER*)bt_pipe->_data_pipe._p_data_manager, bt_range, data_buffer,  bt_range->_length, bt_pipe_read_data_callback,  bt_pipe);
#endif
    }
    else
    {
        sd_assert( FALSE );
    }

	LOG_DEBUG("[bt_pipe = 0x%x]bdm_bt_pipe_read_data_buffer:ret=%d", bt_pipe,ret);
	if(ret!=SUCCESS)
	{
		if(ret==BT_PIPE_READER_FULL)
		{
			ret = list_push(&bt_pipe->_upload_piece_list, piece);
			if(ret != SUCCESS)
			{
				bt_free_bt_piece_data(bt_pipe->_reading_piece);
			}
			bt_pipe->_reading_piece = NULL;
			SAFE_DELETE(data_buffer);
			SAFE_DELETE(bt_range);
			return SUCCESS;
		}
	}
	
ErrorHanle:
	
	if(ret != SUCCESS)
	{
		bt_free_bt_piece_data(bt_pipe->_reading_piece);
		bt_pipe->_reading_piece = NULL;
		SAFE_DELETE(data_buffer);
		SAFE_DELETE(bt_range);
		return bt_pipe_upload_piece_data(bt_pipe);		//上传下一个piece
	}
	return SUCCESS;
}

_int32 bt_pipe_read_data_callback(void* user_data, BT_RANGE* bt_range, char* data_buffer, _int32 read_result, _u32 read_len)
{
	BT_DATA_PIPE* bt_pipe = (BT_DATA_PIPE*)user_data;
#ifdef UPLOAD_ENABLE
	if(read_result == UFM_ERR_READ_CANCELED)
	{
		sd_free(data_buffer);
		data_buffer = NULL;
		sd_free(bt_range);
		bt_range = NULL;
		return SUCCESS;
	}
#endif
	LOG_DEBUG( " [bt_pipe = 0x%x]bt_pipe_read_data_callback:_reading_piece = 0x%x,list_size(&bt_pipe->_upload_piece_list)=%u,read_result=%d,read_len=%u,_reading_piece->_len=%u,bt_range->_pos=%llu,bt_range->len=%llu",
		bt_pipe,bt_pipe->_reading_piece ,list_size(&bt_pipe->_upload_piece_list),read_result,read_len,bt_pipe->_reading_piece->_len,bt_range->_pos,bt_range->_length);
	if(read_result == SUCCESS)
	{
		bt_pipe_send_piece_cmd(bt_pipe, bt_pipe->_reading_piece, data_buffer);
	}
	bt_free_bt_piece_data(bt_pipe->_reading_piece);
	bt_pipe->_reading_piece = NULL;
	sd_free(data_buffer);
	data_buffer = NULL;
	sd_free(bt_range);
	bt_range = NULL;
	return bt_pipe_upload_piece_data(bt_pipe);		//上传下一个piece
}

#ifdef UPLOAD_ENABLE
void bt_pipe_change_upload_state(BT_DATA_PIPE* bt_pipe, enum tagPEER_UPLOAD_STATE state)
{
#ifdef _DEBUG
	char pipe_state_str[4][21] = {"UPLOAD_CHOKE_STATE", "UPLOAD_UNCHOKE_STATE", "UPLOAD_FIN_STATE", "UPLOAD_FAILED_STATE"};
	LOG_DEBUG("[bt_pipe = 0x%x]bt_pipe_change_upload_state, from %s to %s", bt_pipe, pipe_state_str[bt_pipe->_data_pipe._dispatch_info._pipe_upload_state], pipe_state_str[state]);
#endif
	bt_pipe->_data_pipe._dispatch_info._pipe_upload_state = state;
}
#endif

#endif
#endif





