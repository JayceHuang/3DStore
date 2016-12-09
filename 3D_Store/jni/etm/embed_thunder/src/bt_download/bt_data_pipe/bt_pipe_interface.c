#include "utility/define.h"
#ifdef ENABLE_BT 

#include "../bt_task/bt_task_interface.h"
#include "connect_manager/pipe_interface.h"
#include "bt_pipe_interface.h"
#include "bt_device.h"
#include "bt_memory_slab.h"
#include "bt_data_pipe.h"
#include "bt_pipe_impl.h"
#include "../bt_data_manager/bt_data_manager.h"
#include "../bt_utility/bt_define.h"
#include "data_manager/data_manager_interface.h"
#include "../bt_data_manager/bt_data_manager_interface.h"
#ifdef UPLOAD_ENABLE
#include "upload_manager/upload_manager.h"
#endif
#include "connect_manager/resource.h"
#include "connect_manager/data_pipe.h"
#include "utility/utility.h"
#include "../bt_magnet/bt_magnet_data_manager.h"

#include "utility/logid.h"
#define LOGID	LOGID_BT_PIPE
#include "utility/logger.h"

_int32 init_bt_data_pipe()
{
       LOG_DEBUG("init_bt_data_pipe");
	return init_bt_memory_slab();
}

_int32 uninit_bt_data_pipe()
{
	return uninit_bt_memory_slab();
}

#ifdef ENABLE_BT_PROTOCOL

_int32 bt_resource_create(RESOURCE** resource, _u32 ip, _u16 port)
{
	_int32 ret;
	BT_RESOURCE* bt_resource = NULL;
	*resource = NULL;
	ret = bt_malloc_bt_resource(&bt_resource);
	CHECK_VALUE(ret);
	sd_memset(bt_resource, 0, sizeof(BT_RESOURCE));
	init_resource_info(&bt_resource->_res_pub, BT_RESOURCE_TYPE);
	bt_resource->_ip = ip;
	bt_resource->_port = port;
	*resource = (RESOURCE*)bt_resource;
	LOG_DEBUG("bt_resource_create, ip = %u, port = %u.", ip, port);
	return SUCCESS;
}

_int32 bt_resource_destroy(struct tagRESOURCE** resource)
{
	_int32 ret;
	BT_RESOURCE* bt_resource;
	sd_assert((*resource)->_resource_type == BT_RESOURCE_TYPE);
	if((*resource)->_resource_type != BT_RESOURCE_TYPE)
	{
		LOG_DEBUG("bt_resource_destroy failed, resource type is not BT_RESOURCE");
		return -1;
	}
	bt_resource = (BT_RESOURCE*)*resource;
	LOG_DEBUG("bt_resource_destroy, ip = %u, port = %u.", bt_resource->_ip, bt_resource->_port);
	uninit_resource_info(&bt_resource->_res_pub);
	ret = bt_free_bt_resource(bt_resource);
	CHECK_VALUE(ret);
	*resource = NULL;
	return SUCCESS;
}

_int32 bt_pipe_create(void* data_manager, RESOURCE* res, DATA_PIPE** pipe)
{
	BT_DATA_PIPE* bt_pipe = NULL;
	_int32 ret = SUCCESS;
	ret = bt_malloc_bt_data_pipe(&bt_pipe);
	if(ret != SUCCESS)
	{
		return ret;
	}
	sd_memset(bt_pipe, 0, sizeof(BT_DATA_PIPE));
	init_pipe_info(&bt_pipe->_data_pipe, BT_PIPE, res, data_manager);
	bitmap_init(&bt_pipe->_interested_bitmap);
	bitmap_init(&bt_pipe->_buddy_have_bitmap);
	list_init(&bt_pipe->_request_piece_list);
	list_init(&bt_pipe->_upload_piece_list);
	bt_pipe->_is_ever_unchoke = FALSE;
#ifdef UPLOAD_ENABLE
	bt_pipe->_data_pipe._dispatch_info._pipe_upload_state = UPLOAD_CHOKE_STATE;
#endif
	LOG_DEBUG("[bt_pipe = 0x%x]bt_pipe_create.", bt_pipe);
	*pipe = (DATA_PIPE*)bt_pipe;
	return ret;
}

_int32 bt_pipe_open(DATA_PIPE* pipe)
{
	_int32 ret = SUCCESS;
	BT_DATA_PIPE* bt_pipe = (BT_DATA_PIPE*)pipe;
	BT_RESOURCE* bt_res = (BT_RESOURCE*)bt_pipe->_data_pipe._p_resource;

#ifdef _DEBUG
	char buffer[24] = {0};
	sd_inet_ntoa(bt_res->_ip, buffer, 24);
	LOG_DEBUG("[bt_pipe = 0x%x]bt_pipe_open, connect to ip = %s, port = %u", bt_pipe, buffer, bt_res->_port);
#endif

	ret = bt_create_device(&bt_pipe->_device, bt_pipe);
	CHECK_VALUE(ret);
	bt_pipe_change_state(bt_pipe, PIPE_CONNECTING);
	ret = bt_device_connect(bt_pipe->_device, bt_res->_ip, bt_res->_port);
	if(ret != SUCCESS)
	{
		bt_close_device(bt_pipe->_device);
		bt_pipe->_device = NULL;
	}
	return ret;
}

_int32 bt_pipe_change_ranges(DATA_PIPE* pipe, const RANGE* range, BOOL cancel_flag)
{
	BT_PIECE_DATA* piece = NULL;
	BT_DATA_PIPE* bt_pipe = (BT_DATA_PIPE*)pipe;
	sd_assert(bt_pipe->_data_pipe._dispatch_info._pipe_state != PIPE_FAILURE);
	LOG_DEBUG("[bt_pipe = 0x%x]bt_pipe_change_ranges, uncomplete_range[%u, %u],down_range[%u, %u],cancel_flag = %d", bt_pipe, range->_index, 
			range->_num, pipe->_dispatch_info._down_range._index, pipe->_dispatch_info._down_range._num, cancel_flag);
	bt_pipe->_uncomplete_range._index = range->_index;
	bt_pipe->_uncomplete_range._num = range->_num;
	if(bt_pipe->_data_pipe._dispatch_info._pipe_state != PIPE_DOWNLOADING)
	{
		return SUCCESS;
	}
	if(cancel_flag == TRUE)
	{
		//取消掉所有的请求
		while(list_size(&bt_pipe->_request_piece_list) > 0)
		{
			list_pop(&bt_pipe->_request_piece_list, (void**)&piece);
			bt_pipe_send_cancel_cmd(bt_pipe, piece);
			bt_free_bt_piece_data(piece);
		}
		dp_clear_bt_download_range(&bt_pipe->_data_pipe);		//清空正在下载的down_range
	}
	return bt_pipe_request_data(bt_pipe);
}

_int32 bt_pipe_close(DATA_PIPE* pipe)
{
	BT_DATA_PIPE* bt_pipe = (BT_DATA_PIPE*)pipe;
	LOG_DEBUG("[bt_pipe = 0x%x]bt_pipe_close, pipe_state = %d.", bt_pipe, pipe->_dispatch_info._pipe_state);

	if(bt_pipe->_timeout_id != INVALID_MSG_ID)
	{
		cancel_timer(bt_pipe->_timeout_id);
		bt_pipe->_timeout_id = INVALID_MSG_ID;
	}
	if(bt_pipe->_get_data_buffer_timeout_id != INVALID_MSG_ID)
	{
		cancel_timer(bt_pipe->_get_data_buffer_timeout_id);
		bt_pipe->_get_data_buffer_timeout_id = INVALID_MSG_ID;
	}
	/* Clear all the read data operation */
	if(bt_pipe->_reading_piece != NULL)
	{
#ifdef UPLOAD_ENABLE
		ulm_cancel_read_data(&bt_pipe->_data_pipe);
#else
        if( !bt_pipe->_is_magnet_pipe )
        {
		bdm_bt_pipe_clear_read_data_buffer((BT_DATA_MANAGER*)bt_pipe->_data_pipe._p_data_manager, bt_pipe);
        }
#endif
		bt_free_bt_piece_data(bt_pipe->_reading_piece);
		bt_pipe->_reading_piece = NULL;
	}
#ifdef UPLOAD_ENABLE
	//这里已经直接把pipe从上传管理器移除了，所以不用改变状态
	ulm_remove_pipe(&bt_pipe->_data_pipe);
#endif
	dp_set_pipe_interface(pipe, NULL);
	if(bt_pipe->_device == NULL)
		bt_pipe_notify_close(bt_pipe);
	else
		bt_close_device(bt_pipe->_device);
	return SUCCESS;
}

_int32 bt_pipe_set_magnet_mode(DATA_PIPE* pipe)
{
	BT_DATA_PIPE* bt_pipe = (BT_DATA_PIPE*)pipe;
    bt_pipe->_is_magnet_pipe = TRUE;
    return SUCCESS;
}

_int32 bt_pipe_get_speed(struct tagDATA_PIPE* pipe)
{
	_int32 ret = SUCCESS;
	BT_DATA_PIPE* bt_pipe = (BT_DATA_PIPE*)pipe;
	if(pipe->_dispatch_info._pipe_state != PIPE_DOWNLOADING)
	{
		sd_assert(pipe->_dispatch_info._speed == 0);
		//LOG_DEBUG("[p2p_pipe = 0x%x]p2p_pipe_get_speed, but pipe is not downloading, pipe state is %d", pipe, pipe->_dispatch_info._pipe_state);
		return SUCCESS;
	}
	ret = calculate_speed(&bt_pipe->_device->_speed_calculator, &pipe->_dispatch_info._speed);
	LOG_DEBUG("[bt_pipe = 0x%x]bt_pipe_get_speed, speed = %u.", bt_pipe, pipe->_dispatch_info._speed);
	return ret;
}

BOOL bt_pipe_is_ever_unchoke( struct tagDATA_PIPE* pipe)
{
	BT_DATA_PIPE* bt_pipe = (BT_DATA_PIPE*)pipe;
	return bt_pipe->_is_ever_unchoke;
}

_int32 bt_pipe_notify_have_piece(struct tagDATA_PIPE* pipe, _u32 piece_index)
{
	BT_DATA_PIPE* bt_pipe = (BT_DATA_PIPE*)pipe;
	LOG_DEBUG("[bt_pipe = 0x%x]bt_pipe_notify_have_piece, piece_index = %u.", bt_pipe, piece_index);
	return bt_pipe_send_have_cmd(bt_pipe, piece_index);
}

/*-----------------------------------*/
_int32 bt_pipe_update_can_download_range(DATA_PIPE* pipe)
{
	BT_RANGE bt_range;
	
	_u32 i = 0;
	_u64 pos = 0;
	_u32 len = 0;
	BT_DATA_PIPE* bt_pipe = (BT_DATA_PIPE*)pipe;
	LOG_DEBUG("[bt_pipe = 0x%x]bt_pipe_update_can_download_range.", bt_pipe);

	dp_clear_can_download_ranges_list(pipe);
	
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
				LOG_DEBUG("[bt_pipe = 0x%x]bt_pipe_update_can_download_range, bt_range[%llu, %llu].", bt_pipe, bt_range._pos, bt_range._length);
				sd_assert(bt_range._length != 0);
				dp_add_bt_can_download_ranges(&bt_pipe->_data_pipe, &bt_range);
				bt_range._length = 0;		//初始状态
			}
		}
	}
	if(bt_range._length != 0)
	{	//最后一块有效
		LOG_DEBUG("[bt_pipe = 0x%x]bt_pipe_update_can_download_range, bt_range[%llu, %llu].", bt_pipe, bt_range._pos, bt_range._length);
		sd_assert(bt_range._length != 0);
		dp_add_bt_can_download_ranges(&bt_pipe->_data_pipe, &bt_range);
	}
	dp_adjust_uncomplete_2_can_download_ranges(&bt_pipe->_data_pipe);
	return bt_pipe_send_interested_cmd(bt_pipe, TRUE);		//不管对端有没有我需要的内容，我都希望它unchoke我
}

#ifdef UPLOAD_ENABLE
_int32 bt_pipe_stop_upload(struct tagBT_DATA_PIPE* bt_pipe)
{
	BT_PIECE_DATA* upload_data = NULL;
	LOG_DEBUG("[bt_pipe = 0x%x]bt_pipe_stop_upload.", bt_pipe);
#ifdef UPLOAD_ENABLE
	ulm_cancel_read_data(&bt_pipe->_data_pipe);
#else
    if( bt_pipe->_is_magnet_pipe )
    {
	bdm_bt_pipe_clear_read_data_buffer((BT_DATA_MANAGER*)bt_pipe->_data_pipe._p_data_manager, bt_pipe);
    }
#endif
	while(list_size(&bt_pipe->_upload_piece_list) > 0)
	{
		list_pop(&bt_pipe->_upload_piece_list, (void**)&upload_data);
		bt_free_bt_piece_data(upload_data);
	}
	return SUCCESS;	
}

_int32 bt_pipe_get_upload_speed(struct tagDATA_PIPE* pipe, _u32* speed)
{
	_int32 ret = SUCCESS;
	BT_DATA_PIPE* bt_pipe = (BT_DATA_PIPE*)pipe;
	ret = calculate_speed(&bt_pipe->_device->_upload_speed_calculator, speed);
	LOG_DEBUG("[bt_pipe = 0x%x]bt_pipe_get_upload_speed, speed = %u.", bt_pipe, *speed);
	return ret;
}

_int32 bt_pipe_choke_remote(struct tagDATA_PIPE* pipe, BOOL choke)
{
	_int32 ret = SUCCESS;
	BT_DATA_PIPE* bt_pipe = (BT_DATA_PIPE*)pipe;
	sd_assert(pipe->_data_pipe_type == BT_PIPE);
	if(choke == TRUE)
	{
		bt_pipe_change_upload_state(bt_pipe, UPLOAD_CHOKE_STATE);
		bt_pipe_stop_upload(bt_pipe);	
	}
	else
	{
		bt_pipe_change_upload_state(bt_pipe, UPLOAD_UNCHOKE_STATE);
	}
	ret = bt_pipe_send_choke_cmd(bt_pipe, choke);
	CHECK_VALUE(ret);
	return ret;	
}
#endif

#ifdef _CONNECT_DETAIL
_int32 bt_pipe_get_peer_pipe_info(DATA_PIPE* pipe, PEER_PIPE_INFO* info)
{
	BT_DATA_PIPE* bt_pipe = (BT_DATA_PIPE*)pipe;
	BT_RESOURCE* bt_res = (BT_RESOURCE*)pipe->_p_resource;
	sd_assert(pipe->_data_pipe_type == BT_PIPE);
	sd_memset(info, 0, sizeof(PEER_PIPE_INFO));
	info->_connect_type= 0;
	sd_inet_ntoa(bt_res->_ip, info->_external_ip, 24);
	info->_internal_tcp_port = bt_res->_port;
	sd_memcpy(info->_peerid, bt_pipe->_remote_peerid, 5);
	info->_speed = pipe->_dispatch_info._speed;
#ifdef UPLOAD_ENABLE
	bt_pipe_get_upload_speed(pipe, &info->_upload_speed);
#endif
	info->_score = pipe->_dispatch_info._score;
	info->_pipe_state = pipe->_dispatch_info._pipe_state;
	return SUCCESS;
}
#endif
#endif
#endif




