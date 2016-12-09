#include "p2p_pipe_impl.h"
#include "p2p_cmd_define.h"
#include "p2p_cmd_builder.h"
#include "p2p_socket_device.h"
#include "p2p_memory_slab.h"
#include "p2p_utility.h"
#include "p2p_send_cmd.h"
#ifdef UPLOAD_ENABLE
#include "p2p_upload.h"
#include "upload_manager/upload_manager.h"
#endif
#include "p2p_transfer_layer/ptl_interface.h"
#include "data_manager/data_receive.h"
#include "data_manager/data_manager_interface.h"
#include "connect_manager/pipe_interface.h"
#include "utility/mempool.h"
#include "utility/settings.h"
#include "utility/time.h"
#include "utility/utility.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_P2P_PIPE
#include "utility/logger.h"

/*同时发送数据请求命令(request_cmd)的数量*/
#define		REQUEST_CMD_NUM		12			

//接收到外来连接
_int32 p2p_handle_passive_connect(P2P_DATA_PIPE* p2p_pipe, BOOL upload_and_download)
{
	_int32 ret = SUCCESS;
	LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]p2p passive connect success, upload_and_download flag is %d.", p2p_pipe, p2p_pipe->_device, upload_and_download);
	p2p_pipe_change_state(p2p_pipe, PIPE_CHOKED);
	sd_time_ms(&p2p_pipe->_data_pipe._dispatch_info._time_stamp);	/*set dispatch connect time*/
	/*create sending queue*/
	if(create_p2p_sending_queue(&p2p_pipe->_socket_device->_p2p_sending_queue) != SUCCESS)
	{	/*create sending queue failed, change status to notify close this pipe*/
		return p2p_pipe_handle_error(p2p_pipe, -1);
	}
	/*create a recv buffer*/
	ret = p2p_malloc_recv_cmd_buffer(&p2p_pipe->_socket_device->_cmd_buffer);
	if(ret != SUCCESS)
	{
		LOG_ERROR("device->_cmd_buffer malloc failed.");
		return p2p_pipe_handle_error(p2p_pipe, ret);
	}
	p2p_pipe->_socket_device->_cmd_buffer_len = RECV_CMD_BUFFER_LEN;
	sd_assert(p2p_pipe->_socket_device->_cmd_buffer_offset == 0);
	/*try to receive protocol header*/
	ret = p2p_socket_device_recv_cmd(p2p_pipe, P2P_CMD_HEADER_LENGTH);
	if(ret != SUCCESS)
	{
		return p2p_pipe_handle_error(p2p_pipe, ret);
	}
	//send handshake_resp_cmd command	
	ret = p2p_send_handshake_resp_cmd(p2p_pipe, P2P_RESULT_SUCC);
	if(ret != SUCCESS)
	{
		return p2p_pipe_handle_error(p2p_pipe, ret);
	}
	/*send interest request*/
	//只有是边下边传的时候才发intrested,纯上传不需要
	if(upload_and_download == TRUE)
	{
		ret = p2p_send_interested_cmd(p2p_pipe);
		if(ret != SUCCESS)
		{
			return p2p_pipe_handle_error(p2p_pipe, ret);
		}
	}
	return ret;
}

/*if this function failed, must change pipe state to PIPE_FALURE*/
_int32 p2p_pipe_request_data(P2P_DATA_PIPE* p2p_pipe)
{	
	RANGE *req_range = NULL;
	RANGE down_range;
	P2P_RESOURCE* peer_resource = (P2P_RESOURCE*)p2p_pipe->_data_pipe._p_resource;
	_int32 ret = SUCCESS;
	RANGE uncomplete_range;
	/*if I have send cancel command and not recv cancel response, I must wait for cancel response before I send new data request*/
	if( p2p_pipe->_cancel_flag == TRUE || p2p_pipe->_data_pipe._dispatch_info._pipe_state == PIPE_CHOKED)
		return SUCCESS;
	/*get a range to request*/
	sd_assert(is_p2p_pipe_connected(p2p_pipe) == TRUE);
	dp_get_uncomplete_ranges_head_range(&p2p_pipe->_data_pipe, &uncomplete_range);
//	LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]dp_get_uncomplete_ranges_head_range, uncomplete_range[%u, %u].", p2p_pipe, p2p_pipe->_device, uncomplete_range._index, uncomplete_range._num);
	if(uncomplete_range._num == 0)
	{
		//		sd_assert(p2p_pipe->_data_pipe._dispatch_info._uncomplete_ranges._node_size == 0);
		LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]p2p_pipe_request_data, but uncomplete ranges is empty, no more range to request.just wait change ranges", p2p_pipe, p2p_pipe->_device);
		pi_notify_dispatch_data_finish(&p2p_pipe->_data_pipe);
		return SUCCESS;			/*no range to request, just wait change ranges*/
	} 
	dp_get_download_range(&p2p_pipe->_data_pipe, &down_range);
	if(list_size(&p2p_pipe->_request_data_queue) > 0 && uncomplete_range._index != down_range._index + down_range._num)
	{	//如果还有请求未完成，且新的请求不是一个连续的range的话，不再继续发送请求
		LOG_DEBUG("[p2p_pipe = 0x%x]p2p_pipe_request_data, but next request range not continue, just wait...", p2p_pipe);
		return SUCCESS;
	}
	//是连续的range或者是新的一段range请求，可以继续发送下一个请求
	if(down_range._num == 0)
	{	//新的一段range请求，要设置正确的down_range.index
		sd_assert(list_size(&p2p_pipe->_request_data_queue) == 0);
		down_range._index = uncomplete_range._index;
	}
	while(list_size(&p2p_pipe->_request_data_queue) < REQUEST_CMD_NUM)
	{
		if(uncomplete_range._num == 0)
			break;		//所有的数据已经发送了请求，直接退出
		sd_assert(down_range._index + down_range._num == uncomplete_range._index);
		ret = p2p_malloc_range(&req_range);
		CHECK_VALUE(ret);
		req_range->_index = uncomplete_range._index;
		req_range->_num = 1;
		LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]p2p_pipe_request_data, down_range[%u, %u], pos_len[%llu, %llu]", p2p_pipe, p2p_pipe->_device, req_range->_index, 
			req_range->_num, get_data_unit_size() * (_u64)req_range->_index, range_to_length(req_range, peer_resource->_file_size));
		ret = p2p_send_request_cmd(p2p_pipe, get_data_unit_size() * (_u64)req_range->_index,  range_to_length(req_range, peer_resource->_file_size));
		if(ret!=SUCCESS)
		{
			p2p_free_range(req_range);
			return ret;
		}
		list_push(&p2p_pipe->_request_data_queue, req_range);
		++down_range._num;
		dp_set_download_range(&p2p_pipe->_data_pipe, &down_range);
		++uncomplete_range._index;
		--uncomplete_range._num;
	}
//	LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]dp_delete_uncomplete_ranges, down_range[%u, %u].", p2p_pipe, p2p_pipe->_device, down_range._index, down_range._num);
	dp_delete_uncomplete_ranges(&p2p_pipe->_data_pipe, &down_range);		//把uncomplete_ranges调整对
//	LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]after dp_delete_uncomplete_ranges, uncomplete_range[%u, %u].", p2p_pipe, p2p_pipe->_device, down_range._index, down_range._num);
	return SUCCESS;
}

_int32 p2p_pipe_handle_error(P2P_DATA_PIPE* p2p_pipe, _int32 errcode)
{
	_int32 http_encap_state = 0; /*  0-未知,1-迅雷p2p协议不需要http封装,2-迅雷p2p协议需要http封装  */
	_int32 http_encap_test_count = 0; /*  测试MAX_HTTP_ENCAP_P2P_TEST_COUNT 次  */
#ifdef _DEBUG
	P2P_RESOURCE* p2p_res;
#endif
	LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]p2p_pipe_handle_error, errcode = %d.", p2p_pipe, p2p_pipe->_device, errcode);
	if(p2p_pipe->_data_pipe._dispatch_info._pipe_state==PIPE_CHOKED && p2p_pipe->_last_cmd_is_handshake && errcode == SOCKET_CLOSED)
	{
		/* 迅雷p2p协议被屏蔽,需要用http封装之后再试 */
		settings_get_int_item( "p2p.http_encap_state",&http_encap_state );
		settings_get_int_item( "p2p.http_encap_test_count",&http_encap_test_count );
		if(http_encap_state == 0 && (http_encap_test_count++>=MAX_HTTP_ENCAP_P2P_TEST_COUNT))
		{
			http_encap_state = 2;
			settings_set_int_item( "p2p.http_encap_state", http_encap_state );
			#if defined(MACOS)&&defined(_DEBUG)
			printf("\n !!!!!!!!p2p_pipe_handle_error NEED http encap: http_encap_state=%d,http_encap_test_count=%d!!!!!!!!!!!\n",http_encap_state,http_encap_test_count);
			#endif
			http_encap_test_count = 0;
		}
		settings_set_int_item( "p2p.http_encap_test_count", http_encap_test_count );
	}
	/*set pipe status PIPE_FAILURE to notify dispatcher module to close this pipe*/
	p2p_pipe_change_state(p2p_pipe, PIPE_FAILURE);
	if(p2p_pipe->_data_pipe._p_resource != NULL)	//纯上传的p2p_pipe没有resource		
		set_resource_err_code(p2p_pipe->_data_pipe._p_resource, errcode);

#ifdef _DEBUG
	p2p_res = (P2P_RESOURCE*)p2p_pipe->_data_pipe._p_resource;
	if(p2p_res != NULL)
	{
		char addr[24] = {0};
		sd_inet_ntoa(p2p_res->_ip, addr, 24);
		LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]p2p_pipe change state to PIPE_FAILURE, remote addr = %s, remote port = %u, errcode = %d, peerid = %s", p2p_pipe, p2p_pipe->_device, addr, p2p_res->_tcp_port, errcode, p2p_res->_peer_id);
	}
#endif

	/*cancel keep alive timeout if necessary*/
	if(p2p_pipe->_socket_device->_keepalive_timeout_id != INVALID_MSG_ID)
	{
		cancel_timer(p2p_pipe->_socket_device->_keepalive_timeout_id);
		p2p_pipe->_socket_device->_keepalive_timeout_id = INVALID_MSG_ID;
	}
#ifdef UPLOAD_ENABLE
	/*stop upload*/
	p2p_change_upload_state(p2p_pipe, UPLOAD_FAILED_STATE);
	return p2p_stop_upload(p2p_pipe);
#else
	return SUCCESS;
#endif
}

void p2p_pipe_change_state(P2P_DATA_PIPE* p2p_pipe, PIPE_STATE state)
{
#if defined(_DEBUG)
	char pipe_state_str[7][17] = {"PIPE_IDLE", "PIPE_CONNECTING", "PIPE_CONNECTED", "PIPE_CHOKED", "PIPE_DOWNLOADING", "PIPE_FAILURE", "PIPE_SUCCESS"};
       pipe_state_str[0][16]=0;
	LOG_DEBUG("[p2p_pipe = 0x%x, device = 0x%x]p2p_pipe_change_state, from %s to %s", p2p_pipe, p2p_pipe->_device, pipe_state_str[p2p_pipe->_data_pipe._dispatch_info._pipe_state], pipe_state_str[state]);
#endif
	if( ( state == PIPE_CONNECTING || state == PIPE_CHOKED || state == PIPE_DOWNLOADING )
		&& p2p_pipe->_data_pipe._dispatch_info._pipe_state != state )
	sd_time_ms(&p2p_pipe->_data_pipe._dispatch_info._time_stamp);
	if(state == PIPE_DOWNLOADING)
		sd_time_ms(&p2p_pipe->_data_pipe._dispatch_info._last_recv_data_time);
    //p2p_pipe->_data_pipe._dispatch_info._pipe_state = state;
    dp_set_state(&p2p_pipe->_data_pipe, state);
}

_int32 p2p_pipe_notify_close(struct tagP2P_DATA_PIPE* p2p_pipe)
{
	RANGE* range = NULL;
	LOG_DEBUG("[p2p_pipe = 0x%x]p2p_pipe_notify_close, p2p_pipe is destroy.",p2p_pipe);
	while(list_size(&p2p_pipe->_request_data_queue) > 0)
	{
		list_pop(&p2p_pipe->_request_data_queue, (void**)&range);
		p2p_free_range(range);
	}
	ptl_destroy_device(p2p_pipe->_device);
	uninit_pipe_info(&p2p_pipe->_data_pipe);
	return p2p_free_p2p_data_pipe(p2p_pipe);
}

	



