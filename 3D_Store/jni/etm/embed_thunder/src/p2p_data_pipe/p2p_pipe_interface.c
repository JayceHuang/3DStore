#include "utility/peer_capability.h"
#include "utility/utility.h"
#include "utility/peerid.h"
#include "utility/time.h"
#include "utility/logid.h"
#define LOGID LOGID_P2P_PIPE
#include "utility/logger.h"	

#include "p2p_pipe_interface.h"

#include "connect_manager/data_pipe.h"
#include "p2p_pipe_impl.h"
#include "p2p_data_pipe.h"
#include "p2p_cmd_define.h"
#include "p2p_cmd_builder.h"
#include "p2p_utility.h"
#include "p2p_memory_slab.h"
#include "p2p_send_cmd.h"
#include "p2p_socket_device.h"
#include "p2p_passive_connect.h"
#ifdef UPLOAD_ENABLE
#include "p2p_upload.h"
#include "upload_manager/upload_manager.h"
#endif
#include "p2p_transfer_layer/ptl_interface.h"
#include "p2p_transfer_layer/ptl_callback_func.h"
#include "p2p_transfer_layer/ptl_passive_connect.h"
#include "connect_manager/resource.h"
#include "data_manager/data_manager_interface.h"
#include "asyn_frame/msg.h"
#include "high_speed_channel/hsc_info.h"


static void* g_p2p_callback_table[6] = {(void*)p2p_socket_device_connect_callback, (void*)p2p_socket_device_send_callback, 
									(void*)p2p_socket_device_recv_cmd_callback, (void*)p2p_socket_device_recv_data_callback,
									(void*)p2p_socket_device_recv_diacard_data_callback, (void*)p2p_notify_close_socket_device};

_int32 init_p2p_module()
{
	_int32 ret = SUCCESS;
	LOG_DEBUG("init p2p module...");
	
	ret = init_p2p_memory_slab();
	if (ret != SUCCESS)
	{
	    return ret;
	}
	
	ret = init_p2p_sending_queue_mpool();
	if (ret != SUCCESS)
	{
		uninit_p2p_memory_slab();
		return ret;
	}
	
	ptl_regiest_p2p_accept_func(p2p_handle_passive_accept);
	return SUCCESS;
}

_int32 uninit_p2p_module()
{
	_int32 ret;
	LOG_DEBUG("uninit p2p module...");
	ret = uninit_p2p_sending_queue_mpool();
	CHECK_VALUE(ret);	
	ret = uninit_p2p_memory_slab();
	return ret;
}

_int32 p2p_pipe_create(DATA_MANAGER* data_manager, RESOURCE* resource, DATA_PIPE** pipe, PTL_DEVICE* device)
{
	/*when accept open, both data_manager and peer_resource are NULL*/
	_int32 ret = SUCCESS;
	P2P_DATA_PIPE* p2p_pipe = NULL;
	ret = p2p_malloc_p2p_data_pipe(&p2p_pipe);
	if(ret != SUCCESS)
		return ret;
	sd_memset(p2p_pipe, 0, sizeof(P2P_DATA_PIPE));
	if(device == NULL)
	{
		ret = ptl_create_device(&p2p_pipe->_device, p2p_pipe, g_p2p_callback_table);
		if(ret != SUCCESS)
		{
			p2p_free_p2p_data_pipe(p2p_pipe);
			return ret;
		}
	}
	else
	{
		p2p_pipe->_device = device;
		p2p_pipe->_device->_user_data = p2p_pipe;
		p2p_pipe->_device->_table = g_p2p_callback_table;		//记得把函数表设对
	}
	ret = p2p_create_socket_device(p2p_pipe);
	if(ret !=  SUCCESS)
	{
		ptl_destroy_device(p2p_pipe->_device);
		p2p_free_p2p_data_pipe(p2p_pipe);
		return ret;
	}
	init_pipe_info(&p2p_pipe->_data_pipe, P2P_PIPE, resource, (void *)data_manager);
	p2p_pipe->_handshake_connect_id = 0x80000000L + sd_rand() % 0x80000000L;		/*must greater 0x80000000*/
	p2p_pipe->_cancel_flag = FALSE;
	p2p_pipe->_remote_not_in_nat = FALSE;
	p2p_pipe->_remote_peer_capability = 0;
	p2p_pipe->_remote_protocol_ver = 0;
	p2p_pipe->_is_ever_unchoke = FALSE;
	p2p_pipe->_last_cmd_is_handshake = FALSE;
	list_init(&p2p_pipe->_request_data_queue);
#ifdef UPLOAD_ENABLE
	list_init(&p2p_pipe->_upload_data_queue);
	init_speed_calculator(&p2p_pipe->_upload_speed, 20, 500);
#endif
	LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]p2p pipe create.", p2p_pipe, p2p_pipe->_device);
	*pipe = (DATA_PIPE*)p2p_pipe;
	return ret;
}

/*open the pipe, try to connect peer*/
_int32 p2p_pipe_open(DATA_PIPE* pipe)
{
	P2P_DATA_PIPE* p2p_pipe = (P2P_DATA_PIPE*)pipe;
	P2P_RESOURCE* peer_resource = (P2P_RESOURCE*)p2p_pipe->_data_pipe._p_resource;
	
	p2p_pipe_change_state(p2p_pipe, PIPE_CONNECTING);

	LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]p2p pipe open, connect to remote peer = %s, remote_ip = %u, remote_port = %u, connect_type = %u", p2p_pipe, 
				p2p_pipe->_device, peer_resource->_peer_id, peer_resource->_ip, peer_resource->_tcp_port, ptl_get_connect_type(peer_resource->_peer_capability));
	
	return p2p_socket_device_connect(p2p_pipe, peer_resource);
}

_int32 p2p_pipe_close(DATA_PIPE* pipe)
{
	_int32 ret;
	P2P_DATA_PIPE* p2p_pipe = (P2P_DATA_PIPE*)pipe;

	LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]p2p_pipe_close, pipe_state = %d", 
		p2p_pipe, p2p_pipe->_device, p2p_pipe->_data_pipe._dispatch_info._pipe_state);
	pipe->_p_data_manager = NULL;
	pipe->_p_resource = NULL;
	if(p2p_pipe->_data_pipe._dispatch_info._pipe_state != PIPE_FAILURE)
		p2p_pipe_change_state(p2p_pipe, PIPE_FAILURE);	/*remember to set PIPE_FAILURE, since other close process assume this pipe state is PIPE_FAILURE*/
	sd_assert(p2p_pipe->_device != NULL);
#ifdef UPLOAD_ENABLE
	//这里已经直接把pipe从上传管理器移除了，所以不用改变状态
	uninit_speed_calculator(&p2p_pipe->_upload_speed);
	p2p_stop_upload(p2p_pipe);			//停止上传
	ulm_remove_pipe(&p2p_pipe->_data_pipe);
#endif
	dp_set_pipe_interface(pipe, NULL);
	ret = p2p_close_socket_device(p2p_pipe);
	if(ret == SUCCESS)
	{
		return p2p_pipe_notify_close(p2p_pipe);
	}
	else
	{
		sd_assert(ret == ERR_P2P_WAITING_CLOSE);		//表示异步关闭，等待底层回调p2p_pipe_notify_close
	}
	return SUCCESS;
}

_int32 p2p_pipe_change_ranges(DATA_PIPE* pipe, const RANGE* range, BOOL cancel_flag)
{
	_int32 ret = SUCCESS;
	RANGE*	req_range = NULL;
	P2P_DATA_PIPE* p2p_pipe = (P2P_DATA_PIPE*)pipe;
	sd_assert(p2p_pipe->_data_pipe._dispatch_info._pipe_state != PIPE_FAILURE);
	if(p2p_pipe->_data_pipe._dispatch_info._pipe_state != PIPE_DOWNLOADING)
	{
		LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]p2p_pipe_change_ranges, but state is %d, not DOWNLOADING state, just return.", p2p_pipe, p2p_pipe->_device,p2p_pipe->_data_pipe._dispatch_info._pipe_state);
		return SUCCESS;
	}
	LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]p2p_pipe_change_ranges, uncomplete_range[%u, %u],down_range[%u, %u],cancel_flag = %d", p2p_pipe, p2p_pipe->_device, range->_index, range->_num, p2p_pipe->_data_pipe._dispatch_info._down_range._index, p2p_pipe->_data_pipe._dispatch_info._down_range._num, cancel_flag);
	
      /* 优化代码，减少不必要的取消操作 ! zyq @20110110 */
	if(p2p_pipe->_data_pipe._dispatch_info._down_range._num!=0&&
	p2p_pipe->_data_pipe._dispatch_info._down_range._index == range->_index&&
	p2p_pipe->_data_pipe._dispatch_info._down_range._num<=range->_num+2)
	{
		LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]p2p_pipe_change_ranges, downloading same range, just return.", p2p_pipe, p2p_pipe->_device);
		return SUCCESS;
	}

	dp_clear_uncomplete_ranges_list(&p2p_pipe->_data_pipe);
	dp_add_uncomplete_ranges(&p2p_pipe->_data_pipe, range);

	if(p2p_pipe->_cancel_flag == TRUE)
	{
		LOG_DEBUG("[p2p_pipe = 0x%x, devcie = 0x%x]p2p_pipe_change_ranges, but cancel_flag is TRUE, just return.");
		return SUCCESS;
	}
	/*pipe_downloading, 当pipe握手成功得到能下载的数据范围后，状态变为PIPE_DOWNLOADING*/
	if(cancel_flag == TRUE)
	{	//更改range后，不包含已经发送请求的range,cancel掉所有请求的range
		LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]p2p_pipe send cancel cmd.", p2p_pipe, p2p_pipe->_device);
		ret = p2p_send_cancel_cmd(p2p_pipe);
		if(ret != SUCCESS)
		{
			LOG_ERROR("[p2p_pipe = 0x%x, device = 0x%x]p2p_pipe send cancel cmd failed, errcode = %d.", p2p_pipe, p2p_pipe->_device, ret);
			return ret;
		}
		while(list_size(&p2p_pipe->_request_data_queue) > 0)
		{
			list_pop(&p2p_pipe->_request_data_queue, (void**)&req_range);
			p2p_free_range(req_range);
		}
		p2p_pipe->_cancel_flag = TRUE;
		dp_clear_download_range(&p2p_pipe->_data_pipe);
#ifdef VVVV_VOD_DEBUG		
		gloabal_discard_data += p2p_pipe->_socket_device->_data_buffer_offset;
#endif		
		p2p_pipe->_socket_device->_data_buffer_offset = 0;		//把接收到的数据全清掉，因为发送cancel后，表示这些数据不需要
		p2p_pipe->_socket_device->_is_valid_data = FALSE;		//之后收到的任何数据都是无效的
		return SUCCESS;
	}
	ret = p2p_pipe_request_data(p2p_pipe);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]p2p_pipe_change_ranges failed, errcode = %d", p2p_pipe, p2p_pipe->_device, ret);
		//sd_assert(FALSE);
		p2p_pipe_handle_error(p2p_pipe, ret);
	}
	return ret;
}

BOOL compare_peer_resource(struct tagP2P_RESOURCE *res1, struct tagP2P_RESOURCE *res2)
{
    sd_assert(res1 != NULL);
    sd_assert(res2 != NULL);

    return (sd_is_cid_equal(res1->_gcid, res2->_gcid) 
            && (res1->_file_size == res2->_file_size) 
            && (sd_stricmp(res1->_peer_id, res2->_peer_id) == 0)) ? TRUE : FALSE;
}

BOOL p2p_resource_copy_lack_info( RESOURCE* dest, RESOURCE* src )
{
	P2P_RESOURCE* peer_dest, *peer_src;
	//sd_assert(dest->_resource_type == PEER_RESOURCE && src->_resource_type == PEER_RESOURCE);
	if(dest->_resource_type != PEER_RESOURCE || src->_resource_type != PEER_RESOURCE)
		return FALSE;
	peer_dest = (P2P_RESOURCE*)dest;
	peer_src = (P2P_RESOURCE*)src;
	peer_dest->_peer_capability = peer_src->_peer_capability;
	peer_dest->_res_level = peer_src->_res_level;
	peer_dest->_ip = peer_src->_ip;
	peer_dest->_tcp_port = peer_src->_tcp_port;
	peer_dest->_udp_port = peer_src->_udp_port;

	return TRUE;
}

_int32 p2p_resource_create(RESOURCE** resource, char *peer_id
    , _u8 *gcid, _u64 file_size, _u32 peer_capability, _u32 ip, _u16 tcp_port, _u16 udp_port
    , _u8 res_level, _u8 res_from, _u8 res_priority)
{
	_int32 ret;
	P2P_RESOURCE* peer_resource = NULL;
	*resource = NULL;
	ret = p2p_malloc_p2p_resource(&peer_resource);
	CHECK_VALUE(ret);
	sd_memset(peer_resource, 0, sizeof(P2P_RESOURCE));
	init_resource_info(&peer_resource->_res_pub, PEER_RESOURCE);
	//sd_assert(sd_strlen(peer_id) == PEER_ID_SIZE);
	sd_memcpy(peer_resource->_peer_id, peer_id, PEER_ID_SIZE + 1);
	sd_memcpy(peer_resource->_gcid, gcid, CID_SIZE);
	peer_resource->_file_size = file_size;
	peer_resource->_peer_capability = peer_capability;
	peer_resource->_ip = ip;
	peer_resource->_tcp_port = tcp_port;
	peer_resource->_udp_port = udp_port;
	peer_resource->_res_level = res_level;
	peer_resource->_from = res_from;
	peer_resource->_res_priority = res_priority;
	set_resource_level( (RESOURCE* )peer_resource, res_level);
	*resource = (RESOURCE*)peer_resource;
	return SUCCESS;
}

_u8 p2p_get_res_level(RESOURCE* res)
{
	P2P_RESOURCE* p2p_res = (P2P_RESOURCE*)res;
	return p2p_res->_res_level;
}

_u8 p2p_get_res_priority(RESOURCE* res)
{
	P2P_RESOURCE* p2p_res = (P2P_RESOURCE*)res;
	return p2p_res->_res_priority;
}


BOOL p2p_is_ever_unchoke( struct tagDATA_PIPE* pipe)
{
	P2P_DATA_PIPE* p2p_pipe = (P2P_DATA_PIPE*)pipe;
	return p2p_pipe->_is_ever_unchoke;
}

_int32 p2p_resource_destroy(RESOURCE** resource)
{
	_int32 ret;
	P2P_RESOURCE* peer_resource;
	sd_assert((*resource)->_resource_type == PEER_RESOURCE);
	if((*resource)->_resource_type != PEER_RESOURCE)
	{
		LOG_DEBUG("p2p_resource_destroy failed, resource type is not PEER_RESOURCE");
		return -1;
	}
	peer_resource = (P2P_RESOURCE*)*resource;
	uninit_resource_info(&peer_resource->_res_pub);
	ret = p2p_free_p2p_resource(peer_resource);
	CHECK_VALUE(ret);
	*resource = NULL;
	return SUCCESS;
}

_int32 p2p_pipe_get_speed(DATA_PIPE* pipe)
{
	_int32 ret = SUCCESS;
	P2P_DATA_PIPE* p2p_pipe = (P2P_DATA_PIPE*)pipe;

	sd_assert(pipe->_data_pipe_type == P2P_PIPE && pipe->_dispatch_info._pipe_state == PIPE_DOWNLOADING);
	ret = calculate_speed(&p2p_pipe->_socket_device->_speed_calculator, &pipe->_dispatch_info._speed);

	LOG_DEBUG("[p2p_pipe = 0x%x]p2p_pipe_get_speed, speed = %u.", p2p_pipe, pipe->_dispatch_info._speed);
	return ret;
}

BOOL get_peer_hash_value(char* peerid, _u32 peerid_len, _u32 ip, _u16 port, _u32* hash_value)
{
	char* buffer;
	_u32 hash = 0, x = 0, i = 0;
	if(peerid_len != PEER_ID_SIZE || peerid == NULL)
		return -1;
	while(peerid_len != 0)
	{
		hash = (hash << 4) + (*peerid);
		if((x = hash & 0xF0000000L) != 0)
		{
			hash ^= (x >> 24);
			hash &= ~x;
		} 
		++peerid;
		--peerid_len;
	} 
	buffer = (char*)&ip;
	for(i = 1; i < 7; ++i)
	{
		hash = (hash << 4) + (*buffer);
		if((x = hash & 0xF0000000L) != 0)
		{
			hash ^= (x >> 24);
			hash &= ~x;
		}
		++buffer;
		if(i % 5 == 0)
			buffer = (char*)&port;
	}
	*hash_value = hash & 0x7FFFFFFF;
	return SUCCESS;
}

#ifdef UPLOAD_ENABLE
_int32 p2p_pipe_choke_remote(struct tagDATA_PIPE* pipe, BOOL choke)
{
	P2P_DATA_PIPE* p2p_pipe = (P2P_DATA_PIPE*)pipe;
	sd_assert(pipe->_data_pipe_type == P2P_PIPE);
	if(choke == TRUE)
	{
		p2p_change_upload_state(p2p_pipe, UPLOAD_CHOKE_STATE);
		p2p_stop_upload(p2p_pipe);
	}
	else
	{
		p2p_change_upload_state(p2p_pipe, UPLOAD_UNCHOKE_STATE);
	}
	return p2p_send_choke_cmd(p2p_pipe, choke);
}

_int32 p2p_pipe_get_upload_speed(struct tagDATA_PIPE* pipe, _u32* upload_speed)
{
	P2P_DATA_PIPE* p2p_pipe = (P2P_DATA_PIPE*)pipe;
	calculate_speed(&p2p_pipe->_upload_speed, upload_speed);
	LOG_DEBUG("[p2p_pipe = 0x%x]p2p_pipe_get_upload_speed, speed = %u.", p2p_pipe, *upload_speed);
	return SUCCESS;
}

_int32 p2p_pipe_notify_range_checked(struct tagDATA_PIPE* pipe)
{
	return p2p_send_interested_resp_cmd((P2P_DATA_PIPE*)pipe, 0, 128);
}
#endif

#ifdef _CONNECT_DETAIL
BOOL p2p_pipe_is_transfer_by_tcp(struct tagDATA_PIPE* pipe)
{
	P2P_DATA_PIPE* p2p_pipe = (P2P_DATA_PIPE*)pipe;
	if(p2p_pipe->_device->_type == TCP_TYPE)
		return TRUE;
	else
		return FALSE;
}

_int32 p2p_pipe_get_peer_pipe_info(DATA_PIPE* pipe, PEER_PIPE_INFO* info)
{
	P2P_DATA_PIPE* p2p_pipe = (P2P_DATA_PIPE*)pipe;
	P2P_RESOURCE* res = (P2P_RESOURCE*)p2p_pipe->_data_pipe._p_resource;
	sd_assert(pipe->_data_pipe_type == P2P_PIPE);
	sd_memset(info, 0, sizeof(PEER_PIPE_INFO));
	info->_connect_type = p2p_pipe->_device->_connect_type;
	if(p2p_pipe->_device->_type == UDT_TYPE)
	{
		UDT_DEVICE* udt = (UDT_DEVICE*)p2p_pipe->_device->_device;
		sd_inet_ntoa(udt->_remote_ip, info->_external_ip, 24);
		info->_external_port = udt->_remote_port;
	}
	if(res!=NULL)
	{
		sd_inet_ntoa(res->_ip, info->_internal_ip, 24);
		info->_internal_tcp_port = res->_tcp_port;
		info->_internal_udp_port = res->_udp_port;
		sd_memcpy(info->_peerid, res->_peer_id, PEER_ID_SIZE);
	}
	info->_speed = pipe->_dispatch_info._speed;
#ifdef UPLOAD_ENABLE
	p2p_pipe_get_upload_speed(pipe, &info->_upload_speed);
#endif
	info->_score = pipe->_dispatch_info._score;
	info->_pipe_state = pipe->_dispatch_info._pipe_state;
	return SUCCESS;
}

#endif

BOOL p2p_is_cdn_resource(RESOURCE* res)
{
	P2P_RESOURCE* p2p_res = NULL;
	if(res->_resource_type != PEER_RESOURCE)
		return FALSE;
	p2p_res = (P2P_RESOURCE*)res;
	return is_cdn(p2p_res->_peer_capability);
}

_u32 p2p_get_capability( RESOURCE* res )
{
	P2P_RESOURCE* p2p_res = NULL;
	if(res->_resource_type != PEER_RESOURCE)
	{
		sd_assert( FALSE );
		return 0;
	}
	
	p2p_res = (P2P_RESOURCE*)res;
	return p2p_res->_peer_capability;
	
}

BOOL p2p_is_support_mhxy( struct tagDATA_PIPE* pipe )
{
    _u32 peer_capability = p2p_get_capability( pipe->_p_resource );
    return is_support_mhxy_version1( peer_capability );
}

_u8 p2p_get_from( RESOURCE* res )
{
	P2P_RESOURCE* p2p_res = NULL;
	if(res->_resource_type != PEER_RESOURCE)
	{
		sd_assert( FALSE );
		return P2P_FROM_UNKNOWN;
	}
	
	p2p_res = (P2P_RESOURCE*)res;
	return p2p_res->_from;
	
}

