#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL
#include "emule_udt_send_queue.h"
#include "emule_udt_cmd.h"
#include "emule_socket_device.h"
#include "../emule_utility/emule_memory_slab.h"
#include "asyn_frame/device.h"
#include "utility/time.h"
#include "utility/utility.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_EMULE
#include "utility/logger.h"

#define		DEF_MSS						512
#define		MAX_BUF_SIZE					8192
#define		DUPACKS						3
#define		MAX_CHECK_TIME				1721
#define		MIN_RTO						25

_int32 emule_udt_send_queue_create(EMULE_UDT_SEND_QUEUE** send_queue, struct tagEMULE_UDT_DEVICE* udt_device)
{
	_int32 ret = SUCCESS;
	ret = emule_get_emule_udt_send_queue_slip(send_queue);
	CHECK_VALUE(ret);
	sd_memset(*send_queue, 0, sizeof(EMULE_UDT_SEND_QUEUE));
	list_init(&(*send_queue)->_waiting_send_queue);
	list_init(&(*send_queue)->_had_send_queue);
	(*send_queue)->_udt_device = udt_device;
	(*send_queue)->_send_wnd = MAX_BUF_SIZE;
	(*send_queue)->_congestion_wnd = DEF_MSS;
	(*send_queue)->_slow_start_threshold = 65535;
	(*send_queue)->_time_to_check = 1721;
	(*send_queue)->_backoff = 0;
	(*send_queue)->_srtt = 0;
	(*send_queue)->_mdev = 3;
	return SUCCESS;
}

_int32 emule_udt_send_queue_close(EMULE_UDT_SEND_QUEUE* send_queue)
{
	EMULE_UDT_SEND_BUFFER* send_buffer = NULL;
	if(send_queue->_send_msg_id != INVALID_MSG_ID)
	{
		cancel_message_by_msgid(send_queue->_send_msg_id);
		send_queue->_send_msg_id = INVALID_MSG_ID;
	}
	if(send_queue->_cur_req != NULL)
	{
		emule_socket_device_send_callback(send_queue->_udt_device->_socket_device, send_queue->_cur_req->_buffer, send_queue->_cur_req->_len, MSG_CANCELLED);
		emule_free_emule_udt_send_req_slip(send_queue->_cur_req);
	}
	while(list_size(&send_queue->_waiting_send_queue) > 0)
	{	
		list_pop(&send_queue->_waiting_send_queue, (void**)&send_queue->_cur_req);
		emule_socket_device_send_callback(send_queue->_udt_device->_socket_device, send_queue->_cur_req->_buffer, send_queue->_cur_req->_len, MSG_CANCELLED);
		emule_free_emule_udt_send_req_slip(send_queue->_cur_req);
	}
	send_queue->_cur_req = NULL;
	while(list_size(&send_queue->_had_send_queue) > 0)
	{
		list_pop(&send_queue->_had_send_queue, (void**)&send_buffer);
		emule_free_emule_udt_send_buffer_slip(send_buffer);
	}
	return emule_free_emule_udt_send_queue_slip(send_queue);
}

_int32 emule_udt_send_queue_add(EMULE_UDT_SEND_QUEUE* send_queue, char* data
    , _u32 data_len, BOOL is_data)
{
	_int32 ret = SUCCESS;
	EMULE_UDT_SEND_REQ* req = NULL;
	ret = emule_get_emule_udt_send_req_slip(&req);
	CHECK_VALUE(ret);
	req->_buffer = data;
	req->_len = data_len;
	req->_is_data = is_data;
	list_init(&req->_udt_send_buffer_list);
	list_push(&send_queue->_waiting_send_queue, req);
	return emule_udt_send_queue_update(send_queue);
}

_int32 emule_udt_send_queue_remove(EMULE_UDT_SEND_QUEUE* send_queue, _u32 ack, _u32 wnd)
{
	EMULE_UDT_SEND_BUFFER* send_buffer = NULL;
	LIST_ITERATOR iter = NULL;
	_u32 first_unack_seq = 0;
	_u32 rtt = 0, cur_time = 0, abserr = 0;
	sd_assert(ack != 0);
	send_queue->_send_wnd = wnd;		//更新发送窗口
	for(iter = LIST_BEGIN(send_queue->_had_send_queue); iter != LIST_END(send_queue->_had_send_queue); iter = LIST_NEXT(iter))
	{
		send_buffer = (EMULE_UDT_SEND_BUFFER*)LIST_VALUE(iter);
		if(send_buffer->_seq_num == ack)
			break;
	}
	if(iter != LIST_END(send_queue->_had_send_queue))
	{
		//找到了
		send_queue->_pending_wnd -= send_buffer->_data_len;
		//增加拥塞窗口
		if(send_queue->_congestion_wnd < send_queue->_slow_start_threshold)
			send_queue->_congestion_wnd += MAX_SEGMENT_SIZE;
		else
			send_queue->_congestion_wnd += (MAX_SEGMENT_SIZE * MAX_SEGMENT_SIZE / send_queue->_congestion_wnd);
		if(send_buffer->_send_count < 2)
		{
			first_unack_seq = ((EMULE_UDT_SEND_BUFFER*)LIST_VALUE(LIST_BEGIN(send_queue->_had_send_queue)))->_seq_num;
			//这个包没有出现重传，可以用来计算rto和快速重传
			//按tcp方式计算rto
			sd_time_ms(&cur_time);
			rtt = cur_time - send_buffer->_send_time;
			abserr = (rtt > send_queue->_srtt) ? rtt - send_queue->_srtt : send_queue->_srtt - rtt;
			send_queue->_srtt = (7 * send_queue->_srtt + rtt) >> 3;
			send_queue->_mdev = (3 * send_queue->_mdev + abserr) >> 2;
			send_queue->_time_to_check = send_queue->_srtt + 4 * send_queue->_mdev;
			send_queue->_time_to_check = MAX(MIN_RTO, send_queue->_time_to_check);
			// 快速重传及快速恢复
			if(first_unack_seq == ack)
			{
				if(send_queue->_unack_count > 0)
					send_queue->_congestion_wnd = send_queue->_slow_start_threshold;	// 快速恢复
				send_queue->_unack_count = 0;
			}
			else
			{
				++send_queue->_unack_count;
				if(send_queue->_unack_count == DUPACKS)
				{ 
					//快速重传第一个没有被确认的包
					LOG_DEBUG("[emule_udt_device = 0x%x]quick resend package, seq = %u.", send_queue->_udt_device, first_unack_seq);
					emule_udt_send_data(send_queue->_udt_device, (EMULE_UDT_SEND_BUFFER*)LIST_VALUE(LIST_BEGIN(send_queue->_had_send_queue)));
					send_queue->_slow_start_threshold = send_queue->_congestion_wnd /2;
					send_queue->_slow_start_threshold = MAX(send_queue->_slow_start_threshold, MAX_SEGMENT_SIZE);
					//跟tcp快速重传算法不同，由于每收到一个不是第一个unack的包，cwnd也会增加，所以这里cwnd不用加DUPACKS*mss。
				}
			}
		}
		list_erase(&send_queue->_had_send_queue, iter);
		emule_free_emule_udt_send_buffer_slip(send_buffer);
	}
	return emule_udt_send_queue_update(send_queue);
}

_int32 emule_udt_send_queue_timeout(EMULE_UDT_SEND_QUEUE* send_queue)
{
	_u32 rto = 0, back_off = 0, cur_time = 0;
	BOOL first_timeout = TRUE;
	EMULE_UDT_SEND_BUFFER* send_buffer = NULL;
	LIST_ITERATOR iter = NULL;
	back_off = send_queue->_backoff;
	if(back_off > 31) back_off = 31;
	rto = (1L << back_off) * send_queue->_time_to_check;
	sd_time_ms(&cur_time);
	for(iter = LIST_BEGIN(send_queue->_had_send_queue); iter != LIST_END(send_queue->_had_send_queue); iter = LIST_NEXT(iter))
	{
		send_buffer = (EMULE_UDT_SEND_BUFFER*)LIST_VALUE(iter);
		if(cur_time - send_buffer->_send_time < rto)
			continue;
		if(first_timeout)
		{
			first_timeout = FALSE;
			send_queue->_backoff++;
			send_queue->_slow_start_threshold = send_queue->_congestion_wnd / 2;
			send_queue->_slow_start_threshold = MAX(send_queue->_slow_start_threshold, MAX_SEGMENT_SIZE);
			send_queue->_congestion_wnd = send_queue->_slow_start_threshold;//m_UserModeTCPConfig->MaxSegmentSize; //经实验此处设为m_ssthresh比设为mss效果好。
		}
		emule_udt_send_data(send_queue->_udt_device, send_buffer);
	}
	return SUCCESS;
}

_int32 emule_udt_send_queue_update(EMULE_UDT_SEND_QUEUE* send_queue)
{
	_int32 ret = SUCCESS;
	MSG_INFO msg = {};
	LIST_ITERATOR iter = NULL;
	_u32 usable = 0;
	EMULE_UDT_SEND_BUFFER* send_buffer = NULL;
	if(send_queue->_cur_req == NULL)
	{
		if(list_size(&send_queue->_waiting_send_queue) == 0)
			return SUCCESS;		//没有未发送的请求
		list_pop(&send_queue->_waiting_send_queue, (void**)&send_queue->_cur_req);
		//进行拆包，把大包拆成emule 的udp片段
		ret = emule_split_package_to_segment(send_queue->_cur_req, &send_queue->_seq_num);
		sd_assert(ret == SUCCESS);
	}
	//计算可以发送的数据大小
	usable = MIN(send_queue->_send_wnd, send_queue->_congestion_wnd);
	if(usable > send_queue->_pending_wnd)
		usable -= send_queue->_pending_wnd;
	else
		return SUCCESS;		//没有窗口可以发送数据
	//处理当前的请求
	while(list_size(&send_queue->_cur_req->_udt_send_buffer_list) > 0)
	{
		iter = LIST_BEGIN(send_queue->_cur_req->_udt_send_buffer_list);
		send_buffer = (EMULE_UDT_SEND_BUFFER*)LIST_VALUE(iter);
		if(send_buffer->_data_len > usable && send_queue->_pending_wnd > 0)
			break;		//发送窗口不足
		//可以发送数据
		list_pop(&send_queue->_cur_req->_udt_send_buffer_list, (void**)&send_buffer);
		emule_udt_send_data(send_queue->_udt_device, send_buffer);
		ret = list_push(&send_queue->_had_send_queue, send_buffer);
		CHECK_VALUE(ret);
		//发送窗口不需要更新
		//send_queue->_send_wnd -= send_buffer->_data_len;
		send_queue->_pending_wnd += send_buffer->_data_len;
		usable -= send_buffer->_data_len;
	}
	if(list_size(&send_queue->_cur_req->_udt_send_buffer_list) == 0 && send_queue->_send_msg_id == INVALID_MSG_ID)
	{
		//通知上层这个数据发送请求已经成功
		msg._device_id = 0;
		msg._user_data = (void*)send_queue;
		msg._device_type = DEVICE_NONE;
		msg._operation_type = OP_NONE;
		ret = post_message(&msg, emule_udt_send_queue_msg, NOTICE_ONCE, 0, &send_queue->_send_msg_id);
		CHECK_VALUE(ret);
	}
	return ret;
}

_int32 emule_udt_send_queue_msg(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid)
{
	EMULE_UDT_SEND_QUEUE* send_queue = (EMULE_UDT_SEND_QUEUE*)msg_info->_user_data;
	if(errcode == MSG_CANCELLED)
		return SUCCESS;
	send_queue->_send_msg_id = INVALID_MSG_ID;
	sd_assert(list_size(&send_queue->_cur_req->_udt_send_buffer_list) == 0);
	emule_socket_device_send_callback(send_queue->_udt_device->_socket_device, send_queue->_cur_req->_buffer, send_queue->_cur_req->_len, SUCCESS);
	emule_free_emule_udt_send_req_slip(send_queue->_cur_req);
	send_queue->_cur_req = NULL;
	return emule_udt_send_queue_update(send_queue);
}

//这里可以进一步优化，即把所有的数据尽量填满一个segment后一起发送，目前没有，以后可以优化
_int32 emule_split_package_to_segment(EMULE_UDT_SEND_REQ* req, _u32* seq)
{
	_int32 ret = SUCCESS;
	EMULE_UDT_SEND_BUFFER*	send_buffer = NULL;	
	_u32 copy_len = 0, offset = 0;
	while(offset < req->_len)
	{
		ret = emule_get_emule_udt_send_buffer_slip(&send_buffer);
		CHECK_VALUE(ret);
		sd_memset(send_buffer, 0, sizeof(EMULE_UDT_SEND_BUFFER));
		copy_len = MIN(req->_len - offset, MAX_SEGMENT_SIZE - DATA_SEGMENT_HEADER_SIZE);
		sd_memcpy(send_buffer->_buffer + DATA_SEGMENT_HEADER_SIZE, req->_buffer + offset, copy_len);
		send_buffer->_data_len = copy_len;
		offset += copy_len;
		send_buffer->_seq_num = ++(*seq);
		send_buffer->_is_data = req->_is_data;
		ret = list_push(&req->_udt_send_buffer_list, send_buffer);
		CHECK_VALUE(ret);
	}
	LOG_DEBUG("emule split package(size = %u) to segment, send_buffer_list size = %u.", req->_len, list_size(&req->_udt_send_buffer_list));
	return ret;
}

#endif /* ENABLE_EMULE */
#endif /* ENABLE_EMULE */

