#include "udt_impl.h"
#include "udt_interface.h"
#include "udt_utility.h"
#include "udt_memory_slab.h"
#include "udt_cmd_define.h"
#include "udt_cmd_sender.h"
#include "../ptl_memory_slab.h"
#include "../ptl_callback_func.h"
#include "../ptl_udp_device.h"
#include "../ptl_cmd_define.h"
#include "socket_proxy_interface.h"
#include "asyn_frame/device.h"
#include "utility/mempool.h"
#include "utility/bytebuffer.h"
#include "utility/utility.h"
#include "utility/time.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_P2P_TRANSFER_LAYER
#include "utility/logger.h"

#define		SEND_BUFFER_NUM		8				//大于该值不通知上层继续发送数据
#define		TIMEOUT_INSTANCE		30
#define		DELAY_ACK_INTERVAL	200
#define		KEEPALIVE_TIME			(1000*15)
#define		RECV_PACKAGE_TIMEOUT	(1000*60*3)		//收包超时 3 minutes

//序号比较宏
#define		SEQ_LT(a,b)				((int)((a)-(b)) <  0)
#define		SEQ_LEQ(a,b)			((int)((a)-(b)) <= 0)
#define		SEQ_GT(a,b)				((int)((a)-(b)) >  0)
#define		SEQ_GEQ(a,b)			((int)((a)-(b)) >= 0)

static	BOOL	g_is_udp_buffer_low	= FALSE;
static	BITMAP	g_bitmap;

_int32 udt_init_global_bitmap()
{
	bitmap_init(&g_bitmap);
	return bitmap_resize(&g_bitmap, 8 * 100);
}

_int32 udt_uninit_global_bitmap()
{
	return bitmap_uninit(&g_bitmap);
}

void udt_set_udp_buffer_low(BOOL value)
{
	g_is_udp_buffer_low = value;
}

_int32 udt_notify_connect_result(UDT_DEVICE* udt, _int32 errcode)
{
	_int32 ret = SUCCESS;
	sd_assert(udt->_cca == NULL);
	sd_assert(udt->_rtt == NULL);
	LOG_DEBUG("[udt = 0x%x, device = 0x%x]udt_notify_connect_result, errcode = %d.", udt, udt->_device, errcode);
	if(errcode != SUCCESS)
	{
		udt_change_state(udt, CLOSE_STATE);
		return ptl_connect_callback(errcode, udt->_device);
	}
	//udt连接建立成功，在此处进行udt初始化
	ret = sd_malloc(sizeof(SLOW_START_CCA), (void**)&udt->_cca);
	if(ret != SUCCESS)
	{
		LOG_ERROR("[udt = 0x%x, device = 0x%x]udt_connected, but sd_malloc failed, errcode = %d.", udt, udt->_device, ret);
		ptl_connect_callback(ret, udt->_device);
		return ret;
	}
	udt_init_slow_start_cca(udt->_cca, udt_get_mtu_size() - UDT_NEW_HEADER_SIZE);
	ret = sd_malloc(sizeof(NORMAL_RTT), (void**)&udt->_rtt);
	if(ret != SUCCESS)
	{
		LOG_ERROR("[udt = 0x%x, device = 0x%x]udt_connected, but sd_malloc failed, errcode = %d.", udt, udt->_device, ret);
		sd_free(udt->_cca);
		udt->_cca = NULL;
		ptl_connect_callback(ret, udt->_device);
		return ret;
	}
	rtt_init(udt->_rtt);
	++udt->_next_send_seq;		//注意在连接成功后，_next_send_seq必须加1,否则将不能传输
	udt->_unack_send_seq = udt->_next_send_seq;
	udt->_start_read_seq = udt->_next_recv_seq;
	udt->_next_send_pkt_seq = 1;	
	udt->_next_recv_pkt_seq = 1;
	udt->_max_recved_remote_pkt_seq = 1;
	udt->_delay_ack_time_flag = FALSE;
	udt->_delay_ack_time = 0;
	sd_time_ms(&udt->_last_recv_package_time);
	udt->_last_send_package_time = 0;
	list_init(&udt->_waiting_send_queue);
	list_init(&udt->_had_send_queue);
	set_init(&udt->_udt_recv_buffer_set, udt_recv_buffer_comparator);
	udt_update_real_send_window(udt);
	sd_assert(udt->_timeout_id == INVALID_MSG_ID);
	start_timer(udt_handle_timeout, NOTICE_INFINITE, TIMEOUT_INSTANCE, (_u32)udt, NULL, &udt->_timeout_id);
	LOG_DEBUG("[udt = 0x%x, device = 0x%x]udt_connected.", udt, udt->_device);
	udt_change_state(udt, ESTABLISHED_STATE);
	return ptl_connect_callback(SUCCESS, udt->_device);
}

_int32 udt_active_connect(UDT_DEVICE* udt, _u32 remote_ip, _u16 remote_port)
{
	_int32 ret = SUCCESS;
	sd_assert(udt->_state == INIT_STATE);
	udt_change_state(udt, SYN_STATE);
	udt->_syn_retry_times = 0;
	ret = start_timer(udt_handle_syn_timeout, NOTICE_INFINITE, SYN_TIMEOUT, 0, udt, &udt->_timeout_id);
	CHECK_VALUE(ret);
	udt->_remote_ip = remote_ip;
	udt->_remote_port = remote_port;
	LOG_DEBUG("[udt = 0x%x, device = 0x%x]udt_active_connect, send_syn_cmd, seq = %u, ack = %u, wnd = %u.", udt, udt->_device, udt->_next_send_seq, udt->_next_recv_seq, udt->_recv_window);
	return udt_send_syn_cmd(FALSE, udt->_next_send_seq, udt->_next_recv_seq, udt->_recv_window, 
							udt->_id._virtual_source_port, udt->_id._virtual_target_port, remote_ip, remote_port);
}

//这是一个被动连接
_int32 udt_passive_connect(UDT_DEVICE* udt, SYN_CMD* cmd, _u32 remote_ip, _u32 remote_port)
{	
	_int32 ret = SUCCESS;
	sd_assert(udt->_state == INIT_STATE);
	udt->_remote_ip = remote_ip;
	udt->_remote_port = remote_port;
	udt->_send_window = cmd->_window_size;
	udt->_next_recv_seq = cmd->_seq_num + 1;
	//这里创建一个syn_ack命令，并发送
	udt_change_state(udt, SYN_ACK_STATE);
	udt->_syn_retry_times = 0;
	ret = start_timer(udt_handle_syn_timeout, NOTICE_INFINITE, SYN_TIMEOUT, 0, udt, &udt->_timeout_id);
	CHECK_VALUE(ret);
	ret = udt_send_syn_cmd(TRUE, udt->_next_send_seq, udt->_next_recv_seq, udt->_recv_window, 
						udt->_id._virtual_source_port, udt->_id._virtual_target_port, remote_ip, remote_port);
	CHECK_VALUE(ret);
	return ret;
}

void udt_change_state(UDT_DEVICE* udt, UDT_STATE state)
{
    LOG_DEBUG("[udt = 0x%x, device = 0x%x]from %d to %d.", udt, udt->_device, udt->_state, state);
    udt->_state = state;
}

_int32 udt_sendto(UDT_DEVICE* udt, UDT_SEND_BUFFER* send_buffer)
{
	_int32 ret;
	SD_SOCKADDR	addr;
	LOG_DEBUG("udt send package, package_seq = %u", send_buffer->_package_seq);
	addr._sin_family = SD_AF_INET;
	addr._sin_addr = udt->_remote_ip;
	addr._sin_port = sd_htons(udt->_remote_port);
	if(send_buffer->_is_data)
	{
    	ret = socket_proxy_sendto_in_speed_limit(ptl_get_global_udp_socket()
    	    , send_buffer->_buffer
    	    , send_buffer->_buffer_len
    	    , &addr
    	    , udt_sendto_callback
    	    , send_buffer);
    }
    else
    {
        ret = socket_proxy_sendto(ptl_get_global_udp_socket()
            , send_buffer->_buffer
            , send_buffer->_buffer_len
            , &addr
            , udt_sendto_callback
            , send_buffer);
    }
	if(ret != SUCCESS)
	{
		LOG_ERROR("[udt = 0x%x, device = 0x%x]udt_sendto failed, socket_proxy_sendto return errcode %d.", udt, udt->_device, ret);
		return ret;
	}
	++send_buffer->_reference_count;
	udt_update_last_send_package_time(udt);
	return SUCCESS;
}

_int32 udt_sendto_callback(_int32 errcode, _u32 pending_op_count, const char* buffer, _u32 had_send, void* user_data)
{
	UDT_SEND_BUFFER* send_buffer = (UDT_SEND_BUFFER*)user_data;
	LOG_DEBUG("udt had send package, package_seq = %u", send_buffer->_package_seq);
	--send_buffer->_reference_count;	//回调成功,引用计数减1
	if(send_buffer->_reference_count == 0)
	{	//为0表示可以回收内存
		sd_free(send_buffer->_buffer);
		send_buffer->_buffer = NULL;
		udt_free_udt_send_buffer(send_buffer);
	}
	return SUCCESS;
}

void udt_update_waiting_send_queue(UDT_DEVICE* udt)
{
	_int32 ret;
	LIST_ITERATOR iter;
	UDT_SEND_BUFFER* send_buffer = NULL;
	while(list_size(&udt->_waiting_send_queue) > 0)
	{
		iter = LIST_BEGIN(udt->_waiting_send_queue);
		send_buffer = (UDT_SEND_BUFFER*)LIST_VALUE(iter);
		if(udt_get_remain_send_window(udt) < send_buffer->_data_len)
		{
			LOG_ERROR("[udt = 0x%x, device = 0x%x]udt send data, but remain send window not enought, remain = %u, data_len = %u", udt, 
						udt->_device, udt_get_remain_send_window(udt), send_buffer->_data_len);
			break;		//发送窗口不足，停止发送
		}
		list_pop(&udt->_waiting_send_queue, (void**)&send_buffer);
		--send_buffer->_reference_count;	//引用减1,因为pop出来了
		ret = udt_fill_package_header(udt, send_buffer->_buffer, send_buffer->_buffer_len, send_buffer->_data_len);
		sd_assert(ret == SUCCESS);
		send_buffer->_seq = udt->_next_send_seq;
		send_buffer->_package_seq = udt->_next_send_pkt_seq;
		sd_time_ms(&send_buffer->_last_send_time);
		LOG_DEBUG("[udt = 0x%x, device = 0x%x]udt_update_waiting_send_queue, udt send package, seq = %u, data_len = %u, package_seq = %u", udt, udt->_device, send_buffer->_seq, send_buffer->_data_len, send_buffer->_package_seq);
		ret = udt_sendto(udt, send_buffer);
		sd_assert(ret == SUCCESS);
		udt->_next_send_seq += send_buffer->_data_len;		//更新_next_send_seq
		++udt->_next_send_pkt_seq;							//更新_next_send_pkt_seq
		list_push(&udt->_had_send_queue, send_buffer);
		++send_buffer->_reference_count;
	}
}

void udt_update_had_send_queue(UDT_DEVICE* udt)		//超时重传
{
	LIST_ITERATOR iter;
	UDT_SEND_BUFFER* send_buffer = NULL;
	BOOL retransmit = FALSE;
	_u32 cur_time, timeout;
	sd_time_ms(&cur_time);
	timeout = rtt_get_retransmit_timeout(udt->_rtt);
//	LOG_DEBUG("rtt_get_retransmit_timeout, timeout = %u", timeout);
	// 检查所有已发送未确认的包是否需要重传
	for(iter = LIST_BEGIN(udt->_had_send_queue); iter != LIST_END(udt->_had_send_queue); iter = LIST_NEXT(iter))
	{
		send_buffer = LIST_VALUE(iter);
		// 判断当前时刻与发送时刻之差是否大于超时时间是
		if(TIME_GT(cur_time, send_buffer->_last_send_time + timeout))
		{
			LOG_DEBUG("[udt = 0x%x, device = 0x%x]udt_update_had_send_queue, cur_time - send_buffer->_last_send_time = %u, timeout = %u.",udt, udt->_device, cur_time - send_buffer->_last_send_time, timeout);
			// 如果在发送窗口之内，则重传。
			if(udt_is_seq_in_send_window(udt, send_buffer->_seq))
			{
				send_buffer->_remote_req_times = 0;
				retransmit = TRUE;
				udt_resend_package(udt, send_buffer);
			}
			else
			{
				// 不在发送窗口之内，不能重传。后面的包也不用检查了，肯定也不在发送窗口之内。
				LOG_DEBUG("udt_update_had_send_queue, not in send_window, resend failed.");
				break;
			}
		}
	}
	//如果有重传，需要通知相关地方。
	if(retransmit)
	{
		udt_handle_package_lost(udt->_cca, TRUE, TRUE);
		rtt_handle_retransmit(udt->_rtt, TRUE);
	}
}

void udt_update_recv_buffer_set(UDT_DEVICE* udt)
{
	_u32 copy_len;
	UDT_RECV_BUFFER* recv_buffer = NULL;
	SET_ITERATOR cur_iter, tmp_iter;
	LOG_DEBUG("[udt = 0x%x, device = 0x%x]udt_update_recv_buffer_set.", udt, udt->_device);
	cur_iter = SET_BEGIN(udt->_udt_recv_buffer_set);
	while(cur_iter != SET_END(udt->_udt_recv_buffer_set))
	{
		recv_buffer = (UDT_RECV_BUFFER*)cur_iter->_data;
		if(SEQ_GEQ(udt->_start_read_seq,  recv_buffer->_seq) && SEQ_LT(udt->_start_read_seq, recv_buffer->_seq + recv_buffer->_data_len) &&
			udt->_expect_recv_len > udt->_had_recv_len)
		{	//有数据可读且未读完整
			copy_len = MIN(udt->_expect_recv_len - udt->_had_recv_len, recv_buffer->_data_len - (udt->_start_read_seq - recv_buffer->_seq));
			sd_memcpy(udt->_recv_buffer + udt->_had_recv_len, recv_buffer->_data + (udt->_start_read_seq - recv_buffer->_seq), copy_len);
			udt->_had_recv_len += copy_len;
			udt->_start_read_seq += copy_len;
			udt->_recv_window += copy_len;
		}
		if(SEQ_GEQ(udt->_start_read_seq, recv_buffer->_seq + recv_buffer->_data_len))
		{	//recv_buffer数据已经读取完毕，可以删除了
			tmp_iter = cur_iter;
			cur_iter = SET_NEXT(udt->_udt_recv_buffer_set, cur_iter);
			set_erase_iterator(&udt->_udt_recv_buffer_set, tmp_iter);
			ptl_free_udp_buffer(recv_buffer->_buffer);
			udt_free_udt_recv_buffer(recv_buffer);
		}
		else
		{
			cur_iter = SET_NEXT(udt->_udt_recv_buffer_set, cur_iter);
		}
	}
	if(udt->_expect_recv_len == udt->_had_recv_len)
	{
		_int32 ret;
		MSG_INFO msg = {};
		msg._device_id = (_u32)udt;
		msg._user_data = (void*)udt->_had_recv_len;
		msg._device_type = DEVICE_NONE;
		msg._operation_type = OP_NONE;
		ret = post_message(&msg, udt_device_notify_recv_data, NOTICE_ONCE, 0, &udt->_notify_recv_msgid);
		sd_assert(ret == SUCCESS);
		udt->_recv_buffer = NULL;
		udt->_expect_recv_len = 0;
		udt->_had_recv_len = 0;
	}
}

_int32 udt_handle_data_package(UDT_DEVICE* udt, char** buffer, char* data, _u32 data_len, _u32 seq, _u32 ack, _u32 wnd, _u32 pkt_seq)
{
	_int32 ret;
	UDT_RECV_BUFFER* recv_buffer = NULL;
	LOG_DEBUG("[udt = 0x%x, device = 0x%x]udt_handle_data_package, data_len = %u, seq = %u, ack = %u, wnd = %u, pkt_seq = %u.", udt, udt->_device, data_len, seq, ack, wnd, pkt_seq);
	if(udt->_state != ESTABLISHED_STATE)
	{
		LOG_ERROR("[udt = 0x%x, device = 0x%x]udt_handle_data_package, but udt state = %d, state is error, discard package.", udt, udt->_device, udt->_state);
		return -1;
	}
	sd_time_ms(&udt->_last_recv_package_time);
	if(SEQ_GT(seq, udt->_last_recv_seq))
		udt->_last_recv_seq = seq;
	if(g_is_udp_buffer_low == TRUE && seq != udt->_next_recv_seq)
	{	//已经没有足够的udp_buffer了，此时收到的包不是紧接着的最需要的包就丢弃，腾出udp_buffer供底层接收数据
		LOG_ERROR("[udt = 0x%x, device = 0x%x]udt recv data package, but udp buffer is low, just diacard package.", udt, udt->_device);
		return udt_send_ack_answer(udt);	//发送ack，指示对方发送我最需要的包
	}
	//首先处理数据包的seq信息
	//查看收到的seq是否落在有效的接收窗口范围内
	if(!udt_is_seq_in_recv_window(udt, seq, data_len))
	{	//数据不在接收窗口
		LOG_DEBUG("[udt = 0x%x, device = 0x%x]udt_send_ack_answer, because seq is not in_recv_window.", udt, udt->_device);
		udt_send_ack_answer(udt);
	}
	else
	{	//这是一个数据包,不是纯ack应答包，把收到的数据添加到recv_buffer_set缓存起来
		ret = udt_malloc_udt_recv_buffer(&recv_buffer);
		CHECK_VALUE(ret);
		recv_buffer->_seq = seq;
		recv_buffer->_buffer = *buffer;
		recv_buffer->_data = data;
		recv_buffer->_data_len = data_len;
		recv_buffer->_package_seq = pkt_seq;
		sd_assert(SEQ_GEQ(seq, udt->_start_read_seq));
		ret = set_insert_node(&udt->_udt_recv_buffer_set, recv_buffer);
		if(ret == SUCCESS)
		{
			*buffer = NULL;		//缓冲区已被接管
			if(SEQ_GT(pkt_seq, udt->_max_recved_remote_pkt_seq))
				udt->_max_recved_remote_pkt_seq = pkt_seq;
		}
		else
		{
			udt_free_udt_recv_buffer(recv_buffer);		//重复的数据包，丢弃
		}
		//查看是否需要发送ack应答
		if(udt->_next_recv_seq == seq)
		{	//没有跳包，可以隔包发送ack应答包，计算出下次发送ack应答包的时间
			//注意这里面的算法 ： 如果收到的都是连续的数据包，则每隔一个包发送一个ack，delay_ack_time的作用是控制定时器，如果后面没收到数据包
			//					  则用定时器促发去发送ack包
			udt_update_next_recv_seq(udt);		//更新udt->_next_recv_seq
			if(udt->_delay_ack_time_flag == FALSE)
			{	//更新延时应答时间
				sd_time_ms(&udt->_delay_ack_time);
				udt->_delay_ack_time += (MIN(DELAY_ACK_INTERVAL, rtt_get(udt->_rtt) / 3));
				udt->_delay_ack_time_flag = TRUE;
			}
			else
			{
				udt_send_ack_answer(udt);
			}
			if(udt->_notify_recv_msgid == INVALID_MSG_ID && udt->_recv_buffer != NULL)		//上层没有在读数据时才需要更新一下，促发上层读数据
			{
				sd_assert(udt->_expect_recv_len > 0 && udt->_expect_recv_len > udt->_had_recv_len);
				udt_update_recv_buffer_set(udt);		//此处可以优化，直接把数据读入，不用放入udt->_recv_buffer_set中
			}
		}
		else
		{	//已经跳包了，立即发生ack应答
			LOG_DEBUG("[udt = 0x%x, device = 0x%x]udt_send_ack_answer, because tiao bao.", udt, udt->_device);
			udt_send_ack_answer(udt);
		}
	}
	//接着处理数据包的ack信息
	return udt_handle_ack_answer(udt, seq, ack, wnd, 0, 0, NULL, 0);	
}

_int32 udt_handle_ack_answer(UDT_DEVICE* udt, _u32 seq, _u32 ack, _u32 recv_wnd, _u32 last_recv_seq, _u32 pkt_base, char* bit, _u32 bit_count)
{
	_int32 ret = SUCCESS;
	LIST_ITERATOR iter, tmp_iter;
	_u32 i;
	UDT_SEND_BUFFER* send_buffer = NULL;
	if(udt->_state != ESTABLISHED_STATE)
	{
		LOG_ERROR("[udt = 0x%x, device = 0x%x]udt_handle_ack_answer, but udt state = %d, state is error, discard package.", udt, udt->_device, udt->_state);
		return -1;
	}
	LOG_DEBUG("[udt = 0x%x, device = 0x%x]udt_handle_ack_answer, seq = %u, ack = %u, recv_wnd = %u, last_recv_seq = %u, pkt_base = %u, bitmap = 0x%x, bit_count = %u", udt, udt->_device, seq, ack, recv_wnd, last_recv_seq, pkt_base, bit, bit_count);
	if(SEQ_GT(seq, udt->_last_recv_seq))
		udt->_last_recv_seq = seq;
	udt_update_last_recv_package_time(udt);			// 更新最新收包时间
	if(!udt_is_ack_in_send_window(udt, ack, recv_wnd))
	{
		LOG_ERROR("ack not in send window.");
		return -1;		//ack无效
	}
	// 如果对方接收窗口不为0，则取消坚持定时器，坚持定时器超时计数清零。
	//if (cmd._recv_window != 0)
	//{
	//	udt_timer::cancel_timer(this, TIMER_TYPE_PERSIST);
	//	_persist_retry_times = 0;
	//}
	//回收已经发送且得到确认的数据包
	while(list_size(&udt->_had_send_queue) > 0)
	{
		iter = LIST_BEGIN(udt->_had_send_queue);
		send_buffer = (UDT_SEND_BUFFER*)LIST_VALUE(iter);
		if(SEQ_GEQ(ack, send_buffer->_seq + send_buffer->_data_len))
		{	//该包已被确认,回收该包占用的空间
			LOG_DEBUG("[udt = 0x%x, device = 0x%x]remove had_send_queue item by ack, seq = %u, data_len = %u, retry_time = %u, remote_req_times = %u, package_seq = %u",
				udt, udt->_device, send_buffer->_seq, send_buffer->_data_len, send_buffer->_retry_time, send_buffer->_remote_req_times, send_buffer->_package_seq);
			list_pop(&udt->_had_send_queue, (void**)&send_buffer);
			--send_buffer->_reference_count;
			if(send_buffer->_retry_time == 0)		//没有出现重传，可以用来计算rtt
			{
				udt_update_rtt(udt, send_buffer->_seq, last_recv_seq, send_buffer->_last_send_time);
				rtt_handle_retransmit(udt->_rtt, FALSE);
				udt_handle_package_lost(udt->_cca, FALSE, TRUE);
			}
			if(send_buffer->_reference_count == 0)
			{	//没有任何引用，可以回收内存
				sd_free(send_buffer->_buffer);
				send_buffer->_buffer = NULL;
				udt_free_udt_send_buffer(send_buffer);
				 LOG_DEBUG("[udt = 0x%x, device = 0x%x]  send 1", udt, udt->_device);
				//udt_notify_ptl_send_callback(udt);
			}
			udt_notify_ptl_send_callback(udt);
		}
		else
		{
			break;
		}
	}
	//接着处理丢包信息，并对丢包进行重传
	if(bit != NULL && bit_count > 0)		//这样判读编译器是否能翻译正确？
	{
		udt_print_bitmap_pkt_info(udt, pkt_base, &g_bitmap);
		ret = bitmap_from_bits(&g_bitmap, bit, (bit_count + 7) / 8, bit_count);
		if(ret != SUCCESS)
		{
			LOG_ERROR("[udt = 0x%x, device = 0x%x]udt_handle_ack_answer, but bitmap_from_bits failed, errcode = %d.", udt, udt->_device, ret);
			return ret;
		}
		iter = LIST_BEGIN(udt->_had_send_queue);
		for(i = 0; i < bit_count; ++i)
		{
			if(iter == LIST_END(udt->_had_send_queue))
			{
				break;		//说明bitmap表示的包已经都收到，且已经被remove了，直接退出
			}
			if(bitmap_at(&g_bitmap, i) == TRUE)
			{	//该包被对方收到
				send_buffer = (UDT_SEND_BUFFER*)LIST_VALUE(iter);
				if(send_buffer->_package_seq != pkt_base + i)
				{	//如果对不上号，说明应该是被删除了
					continue;
				}
				if(send_buffer->_retry_time == 0)
				{	//没有出现重传可以用来计算rtt
					udt_update_rtt(udt, send_buffer->_seq, last_recv_seq, send_buffer->_last_send_time);
					rtt_handle_retransmit(udt->_rtt, FALSE);
					udt_handle_package_lost(udt->_cca, FALSE, TRUE);
				}
				//回收此包
				LOG_DEBUG("[udt = 0x%x, device = 0x%x]remove had_send_queue item by bitmap, seq = %u, data_len = %u, retry_time = %u, remote_req_times = %u, package_seq = %u",
					udt, udt->_device, send_buffer->_seq, send_buffer->_data_len, send_buffer->_retry_time, send_buffer->_remote_req_times, send_buffer->_package_seq);		
				tmp_iter = iter;
				iter = LIST_NEXT(iter);
				list_erase(&udt->_had_send_queue, tmp_iter);
				--send_buffer->_reference_count;		//从list中删除，引用计数减1
				if(send_buffer->_reference_count == 0)
				{
					sd_free(send_buffer->_buffer);
					send_buffer->_buffer = NULL;
					udt_free_udt_send_buffer(send_buffer);
				}
				udt_notify_ptl_send_callback(udt);
			}
			else
			{
				//这里有可能对不上号，对方可能通知这一块收到了，但又接着通知这一块没收到
				//send_buffer = (UDT_SEND_BUFFER*)LIST_VALUE(iter);
				//sd_assert(send_buffer->_package_seq == pkt_base + i);
				iter = LIST_NEXT(iter);
			}
		}
	}
	udt->_unack_send_seq = ack;		// 更新确认序列号
	udt->_send_window = recv_wnd;	// 发送窗口
	udt_update_real_send_window(udt);	// 更新实际发送窗口
	//看看是否有packet需要快速重传
	if(list_size(&udt->_had_send_queue) > 0)
	{
		iter = LIST_BEGIN(udt->_had_send_queue);
		send_buffer = (UDT_SEND_BUFFER*)LIST_VALUE(iter);
		//这里有可能对不上号，对方可能通知这一块收到了，但又接着通知这一块没收到
		//sd_assert(udt->_unack_send_seq == send_buffer->_seq);
		++send_buffer->_remote_req_times;
		if(send_buffer->_remote_req_times == 3) 
		{
			LOG_DEBUG("[udt = 0x%x, device = 0x%x]send_buffer->_remote_req_times == 3, for quit resend.", udt, udt->_device);
			udt_resend_package(udt, send_buffer);
			udt_handle_package_lost(udt->_cca, TRUE, FALSE);
			//send_buffer->_remote_req_times = 0;		//这里不清0，在重传的时候再重新设置该参数	
		}
	}
	//是否需要启动坚持定时器
	if (recv_wnd == 0) 
	{	//对方通告了一个大小为0的窗口，启动坚持定时器，暂不实现
		//sd_assert(FALSE);
		//udt_timer::set_timer(p2p_traversal_config::P2P_PERSIST_INTERVAL, this, TIMER_TYPE_PERSIST);
	}
	else 
	{
		//		udt_timer::cancel_timer(this, TIMER_TYPE_PERSIST);
		//		_persist_retry_times = 0;
	}
	return SUCCESS;
}

_int32 udt_resend_package(UDT_DEVICE* udt, UDT_SEND_BUFFER* send_buffer)		//重新发送指定的数据包
{
	_int32 ret;
	char* buf = send_buffer->_buffer + 17;		//指向ack字段
	_int32 buf_size = 8;
#ifdef _DEBUG
	_u32 seq, ack, data_len, pkt_seq, wnd;
#endif
	//重新发送的数据包不必考虑在发送窗口之内，也不必考虑是否发送窗口不足
	++send_buffer->_retry_time;
	sd_time_ms(&send_buffer->_last_send_time);
	//更新ack和recv_window
	sd_set_int32_to_lt(&buf, &buf_size, (_int32)udt->_next_recv_seq);
	sd_set_int32_to_lt(&buf, &buf_size, (_int32)udt->_recv_window);
#ifdef _DEBUG
	sd_memcpy(&seq, send_buffer->_buffer + 13, sizeof(_u32));
	sd_memcpy(&ack, send_buffer->_buffer + 17, sizeof(_u32));
	sd_memcpy(&wnd, send_buffer->_buffer + 21, sizeof(_u32));
	sd_memcpy(&data_len, send_buffer->_buffer + 25, sizeof(_u32));
	sd_memcpy(&pkt_seq, send_buffer->_buffer + 29, sizeof(_u32)); 
	LOG_DEBUG("[udt = 0x%x, device = 0x%x]udt_resend_package, seq = %u, ack = %u,  wnd = %u, data_len = %u, pkt_seq = %u.", udt, udt->_device, seq, ack, wnd, data_len, pkt_seq);
#endif
	ret = udt_sendto(udt, send_buffer);
	CHECK_VALUE(ret);
	udt->_delay_ack_time_flag = FALSE;
	udt->_delay_ack_time = 0;
	return ret;
}

_int32 udt_send_ack_answer(UDT_DEVICE* udt)
{
	_int32 ret, buffer_len, tmp_len;
	char* buffer = NULL, *tmp_buf = NULL;
	BITMAP bitmap;		//这个地方有待优化
	bitmap_init(&bitmap);
	LOG_DEBUG("[udt = 0x%x, device = 0x%x]udt_send_ack_answer, seq = %u, ack = %u, recv_wnd = %u, udt_recv_buffer_set_size = %u", udt, udt->_device, udt->_next_send_seq, udt->_next_recv_seq, udt->_recv_window, set_size(&udt->_udt_recv_buffer_set));
	udt_get_lost_packet_bitmap(udt, &bitmap);
	buffer_len = 37 + (bitmap._bit_count + 7) / 8;
	ret = sd_malloc(buffer_len , (void**)&buffer);
	if(ret!= SUCCESS) return ret;
	tmp_buf = buffer;
	tmp_len = (_int32)buffer_len;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, PTL_PROTOCOL_VERSION); 
	sd_set_int8(&tmp_buf, &tmp_len, ADVANCED_ACK);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)udt->_id._virtual_source_port);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)udt->_id._virtual_target_port);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)udt_local_peerid_hashcode());		//这里的peerid_hashcode得获得填自己的
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)udt->_recv_window);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)udt->_next_send_seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)udt->_next_recv_seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)udt->_last_recv_seq);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)udt->_next_recv_pkt_seq);
	ret = sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)bitmap._bit_count);
	CHECK_VALUE(ret);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)bitmap._bit, (bitmap._bit_count + 7) / 8);
	sd_assert(tmp_len == 0);
	ret = ptl_udp_sendto(buffer, buffer_len, udt->_remote_ip, udt->_remote_port);
	CHECK_VALUE(ret);
	udt_update_last_send_package_time(udt);
	udt->_delay_ack_time_flag = FALSE;
	udt->_delay_ack_time = 0;
	bitmap_uninit(&bitmap);
	return ret;
}

_int32 udt_handle_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid)
{
	_u32 cur_time;
	UDT_DEVICE* udt = (UDT_DEVICE*)msg_info->_device_id;
	if(errcode == MSG_CANCELLED)
		return SUCCESS;

	LOG_DEBUG("[udt = 0x%x, device = 0x%x]udt_handle_timeout, elapsed = %u.", udt, udt->_device, elapsed);
	udt_update_waiting_send_queue(udt);
	udt_update_had_send_queue(udt);
	sd_time_ms(&cur_time);
	if(TIME_GE(cur_time, udt->_last_send_package_time + KEEPALIVE_TIME))
	{
		udt_send_keepalive(udt); // 发送保活包      
	}
	//查看是否需要发送延时的ack应答包
	if(udt->_delay_ack_time_flag == TRUE && TIME_GE(cur_time, udt->_delay_ack_time))
	{
		LOG_DEBUG("[udt = 0x%x, device = 0x%x]udt_send_ack_answer,because of timeout, this is delay ack.", udt, udt->_device);
		udt_send_ack_answer(udt);
	}
	//长时间没有从对方收到包，就断开连接
	if(TIME_GE(cur_time, udt->_last_recv_package_time + RECV_PACKAGE_TIMEOUT))
	{
		ptl_recv_cmd_callback(ERR_UDT_RECV_TIMEOUT, udt->_device, 0);
	}
	return SUCCESS;
}

_int32 udt_send_keepalive(UDT_DEVICE* udt)
{
	_int32 ret;
	char* buffer;
	_u32 len;
	char* tmp_buf;
	_int32 tmp_len;
	NAT_KEEPALIVE_CMD cmd;
	cmd._version = PTL_PROTOCOL_VERSION;
	cmd._cmd_type = NAT_KEEPALIVE;
	cmd._source_port = udt->_id._virtual_source_port;
	cmd._target_port = udt->_id._virtual_target_port;
	cmd._peerid_hashcode = udt_local_peerid_hashcode();
	len = 13;
	ret = sd_malloc(len, (void**)&buffer);
	CHECK_VALUE(ret);
	tmp_buf = buffer;
	tmp_len = (_int32)len;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd._version);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd._cmd_type);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd._source_port);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd._target_port);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd._peerid_hashcode);
	sd_assert(tmp_len == 0);
	ret = ptl_udp_sendto(buffer, len, udt->_remote_ip, udt->_remote_port);
	CHECK_VALUE(ret);
	udt_update_last_send_package_time(udt);
	return ret;
}

_int32 udt_recv_advance_ack_cmd(UDT_DEVICE * udt, ADVANCED_ACK_CMD * cmd)
{
	LOG_DEBUG("[udt = 0x%x, device = 0x%x]udt_recv_advance_ack_cmd, seq = %u, ack = %u, bitmap_base =  %u, bitmap_count = %u", 
				udt, udt->_device, cmd->_seq_num, cmd->_ack_num, cmd->_bitmap_base, cmd->_bitmap_count);
	if(udt->_state == SYN_ACK_STATE)
	{
		if (cmd->_ack_num == udt->_next_send_seq + 1)
		{
			LOG_DEBUG("[udt = 0x%x, device = 0x%x]udt passive connect success, conn_id[%u, %u, %u].", udt, udt->_device,
						(_u32)udt->_id._virtual_source_port, (_u32)udt->_id._virtual_target_port, udt->_id._peerid_hashcode);
			cancel_timer(udt->_timeout_id);
			udt->_timeout_id = INVALID_MSG_ID;
			udt_notify_connect_result(udt, SUCCESS);
		}
		else
		{
			return SUCCESS;
		}
	}	
	return udt_handle_ack_answer(udt, cmd->_seq_num, cmd->_ack_num, cmd->_window_size, cmd->_ack_seq, cmd->_bitmap_base, cmd->_bit, cmd->_bitmap_count);
}
_int32 udt_recv_keepalive(UDT_DEVICE* udt)
{	//仅需要更新最后一次收包的时间
	return sd_time_ms(&udt->_last_recv_package_time);
}

_int32 udt_send_reset(UDT_DEVICE* udt)
{
	_int32 ret;
	char* buffer;
	_u32 len;
	char* tmp_buf;
	_int32 tmp_len;
	RESET_CMD cmd;
	cmd._version = PTL_PROTOCOL_VERSION;
	cmd._cmd_type = RESET;
	cmd._source_port = udt->_id._virtual_source_port;
	cmd._target_port = udt->_id._virtual_target_port;
	cmd._peerid_hashcode = udt_local_peerid_hashcode();
	len = 13;
	ret = sd_malloc(len, (void**)&buffer);
	CHECK_VALUE(ret);
	tmp_buf = buffer;
	tmp_len = (_int32)len;
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd._version);
	sd_set_int8(&tmp_buf, &tmp_len, (_int8)cmd._cmd_type);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd._source_port);
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)cmd._target_port);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)cmd._peerid_hashcode);
	sd_assert(tmp_len == 0);
	ret = ptl_udp_sendto(buffer, len, udt->_remote_ip, udt->_remote_port);
	CHECK_VALUE(ret);
	udt_update_last_send_package_time(udt);
	return ret;
}

_int32 udt_recv_reset(UDT_DEVICE* udt)
{	//收到该命令后应该通知上层网络错误，让上层关闭连接
	LOG_DEBUG("[udt = 0x%x, device = 0x%x]udt_recv_reset, current state = %u.", udt, udt->_device, udt->_state);
	udt->_delay_ack_time_flag = FALSE;
	udt->_delay_ack_time = 0;	//已经被reset了，不需要延迟发送ack包
	if(udt->_state == ESTABLISHED_STATE)
	{
		udt->_state = CLOSE_STATE;
		ptl_udt_recv_callback(ERR_UDT_RESET, udt->_recv_type, udt->_device, udt->_had_recv_len);
	}
	else
	{	/*其他任何状态收到reset都通知上层连接失败*/
		udt->_state = CLOSE_STATE;
		ptl_connect_callback(ERR_UDT_RESET, udt->_device);
	}
	return SUCCESS;
}

_int32 udt_recv_syn_cmd(UDT_DEVICE* udt, struct tagSYN_CMD* cmd)
{
	LOG_DEBUG("[udt = 0x%x, device = 0x%x]udt_recv_syn_cmd, flag = %u, seq = %u, ack = %u, wnd = %u", udt, udt->_device, cmd->_flags, cmd->_seq_num, cmd->_ack_num, cmd->_window_size);
	if(udt->_state == INIT_STATE)
		return SUCCESS;
	if(udt->_state == SYN_STATE)
	{	//我是主动方，发送SYN后收到对方的ACK，向对方发送ACK后 主动连接到此建立成功
		if(cmd->_flags == 1)		//这是一个syn_ack命令
		{
			sd_assert(udt->_timeout_id != INVALID_MSG_ID);
			cancel_timer(udt->_timeout_id);		// 取消SYN定时器
			udt->_timeout_id = INVALID_MSG_ID;
			udt->_send_window = cmd->_window_size;	// 初始化发送窗口
			udt->_next_recv_seq = cmd->_seq_num + 1;	// 下一个要接收的序号
			udt->_last_recv_seq = cmd->_seq_num; 		// 最新接收序号
			// 通知连接成功
			LOG_DEBUG("[udt = 0x%x, device = 0x%x]notify active connect success.", udt, udt->_device);
			udt_notify_connect_result(udt, SUCCESS);
			udt_send_ack_answer(udt);
		}
	}	
	else if (udt->_state == SYN_ACK_STATE)
	{	//我是被动方，发送syn_ack后收到对方的syn,继续发送syn_ack	
		if (cmd->_flags == 0) 
		{
			udt_send_syn_cmd(TRUE, udt->_next_send_seq, udt->_next_recv_seq, udt->_recv_window, 
					udt->_id._virtual_source_port, udt->_id._virtual_target_port, udt->_remote_ip, udt->_remote_port);
		}
	}
	else
	{
		LOG_ERROR("[udt = 0x%x, device = 0x%x]udt_recv_syn_cmd, but udt state is wrong, state = %d.", udt, udt->_device, udt->_state);
	}
	/*
	else if (s == ESTABLISHED)
	{
		if (cmd._flags == 1 || cmd._flags == 2)
		{
			LOG4CPLUS_THIS_INFO(_logger, "remote ask my window!");
			send_pure_ack();
		}
	}
	else
	{
		send_reset();
	}
	*/
	udt_update_last_recv_package_time(udt);
	return SUCCESS;
}

_int32 udt_handle_syn_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid)
{
	BOOL is_syn_ack = FALSE;
	UDT_DEVICE* udt = msg_info->_user_data;
	if(errcode == MSG_CANCELLED)
		return SUCCESS;
	++udt->_syn_retry_times;
	if(udt->_syn_retry_times > SYN_MAX_RETRY)
	{
		cancel_timer(udt->_timeout_id);
		udt->_timeout_id = INVALID_MSG_ID;
		return udt_notify_connect_result(udt, -1);
	}
	if(udt->_state == SYN_ACK_STATE)
		is_syn_ack = TRUE;
	return  udt_send_syn_cmd(is_syn_ack, udt->_next_send_seq, udt->_next_recv_seq, udt->_recv_window, 
							udt->_id._virtual_source_port, udt->_id._virtual_target_port, udt->_remote_ip, udt->_remote_port);
}

/*--------------------------------------------------------------------------------------------------------------------------------------
以下部分是辅助函数的实现
----------------------------------------------------------------------------------------------------------------------------------------*/
BOOL udt_is_seq_in_recv_window(UDT_DEVICE* udt, _u32 seq, _u32 data_len)
{
	// 数据起始序列号与窗口左侧比较
	if(SEQ_LT(seq, udt->_next_recv_seq))
	{
		return FALSE; // 在窗口左侧
	}
	// 数据结束序列号与窗口右侧比较
	if(SEQ_GT(seq + data_len, udt->_next_recv_seq + udt->_recv_window))
	{
		return FALSE; // 在窗口右侧
	}
	return TRUE;
}

BOOL udt_is_seq_in_send_window(UDT_DEVICE* udt, _u32 seq)
{
	if(SEQ_LEQ(udt->_unack_send_seq, seq) && SEQ_LT(seq, udt->_unack_send_seq + udt->_real_send_window))
		return TRUE;
	else
		return FALSE;
}

BOOL udt_is_ack_in_send_window(UDT_DEVICE* udt, _u32 ack, _u32 recv_wnd)
{
	if(SEQ_LT(ack + recv_wnd, udt->_send_window + udt->_unack_send_seq))
	{
		return FALSE;//滑动窗口右边向左滑动，认为无效
	}
	//对方希望接收的下一个字节序号是否有效？
	if(SEQ_LT(ack, udt->_unack_send_seq))
	{
		return FALSE;//曾经确认过
	}
	if(SEQ_GT(ack, udt->_next_send_seq))
	{
		LOG_DEBUG("[udt = 0x%x, device = 0x%x]ack = %u greater than  udt->_next_send_seq = %u.", udt, udt->_device, ack, udt->_next_send_seq);
		return FALSE;//不正常,某字节都还没发出去过，就被确认了
	}
	return TRUE;
}

_u32 udt_get_remain_send_window(UDT_DEVICE* udt)
{
	_u32 used_send_window = udt->_next_send_seq - udt->_unack_send_seq;
	if (used_send_window >= udt->_real_send_window)
		return 0;
	else
		return udt->_real_send_window - used_send_window;		//发送窗口减掉已经发送但还没应答的数据大小
}

_int32 udt_fill_package_header(UDT_DEVICE* udt, char* cmd_buffer, _int32 cmd_buffer_len, _int32 data_len)
{
	_int32 ret, len;
	len = UDT_NEW_HEADER_SIZE;
	sd_set_int32_to_lt(&cmd_buffer, &len, PTL_PROTOCOL_VERSION);
	sd_set_int8(&cmd_buffer, &len, ADVANCED_DATA);
	sd_set_int16_to_lt(&cmd_buffer, &len, (_int16)udt->_id._virtual_source_port);
	sd_set_int16_to_lt(&cmd_buffer, &len, (_int16)udt->_id._virtual_target_port);
	sd_set_int32_to_lt(&cmd_buffer, &len, (_int32)udt_local_peerid_hashcode());		//这里的peerid_hashcode得获得填自己的
	sd_set_int32_to_lt(&cmd_buffer, &len, (_int32)udt->_next_send_seq);
	sd_set_int32_to_lt(&cmd_buffer, &len, (_int32)udt->_next_recv_seq);
	sd_set_int32_to_lt(&cmd_buffer, &len, (_int32)udt->_recv_window);
	sd_set_int32_to_lt(&cmd_buffer, &len, data_len);
	ret = sd_set_int32_to_lt(&cmd_buffer, &len, (_int32)udt->_next_send_pkt_seq);
	sd_assert(len == 0);
	return ret;
}

void udt_update_real_send_window(UDT_DEVICE* udt)	// 更新实际发送窗口
{
	// 实际发送窗口为发送窗口（对方的接收窗口）与拥塞窗口的较小者
	
	_u32 ccw = udt_get_cur_congestion_window(udt->_cca);
	LOG_DEBUG("[udt = 0x%x, device = 0x%x] udt_get_cur_congestion_window = %d, udt_send_window =%d"
	        , ccw, udt->_send_window);
	udt->_real_send_window = MIN(udt->_send_window, ccw);
}

void udt_update_next_recv_seq(UDT_DEVICE* udt)
{
	UDT_RECV_BUFFER* recv_buffer = NULL;
	SET_ITERATOR iter;
//	LOG_DEBUG("[udt = 0x%x, device = 0x%x]before udt_update_next_recv_seq, next_recv_seq = %u.", udt, udt->_device, udt->_next_recv_seq);
	for(iter = SET_BEGIN(udt->_udt_recv_buffer_set); iter != SET_END(udt->_udt_recv_buffer_set); iter = SET_NEXT(udt->_udt_recv_buffer_set, iter))
	{
		recv_buffer = (UDT_RECV_BUFFER*)iter->_data;
		if(udt->_next_recv_seq == recv_buffer->_seq)
		{
			udt->_next_recv_seq += recv_buffer->_data_len;
			udt->_recv_window -= recv_buffer->_data_len;
			udt->_next_recv_pkt_seq = recv_buffer->_package_seq + 1;
		}
	}
//	LOG_DEBUG("[udt = 0x%x, device = 0x%x]after udt_update_next_recv_seq, next_recv_seq = %u.", udt, udt->_device, udt->_next_recv_seq);
	sd_assert(iter != SET_BEGIN(udt->_udt_recv_buffer_set));		//udt->_next_recv_seq一定发生了变化
}

void udt_get_lost_packet_bitmap(UDT_DEVICE* udt, BITMAP* bitmap)
{
	_int32 ret = SUCCESS;
	UDT_RECV_BUFFER* recv_buffer = NULL;
	SET_ITERATOR iter;
	_u32 first_not_recved_pkt_seq = udt->_next_recv_pkt_seq;
	if(set_size(&udt->_udt_recv_buffer_set) == 0)
	{
		return;
	}
	LOG_DEBUG("[udt = 0x%x, device = 0x%x]udt_get_lost_packet_bitmap, first_not_recved_pkt_seq = %u, last_recved_pkt_seq = %u.", udt, udt->_device, first_not_recved_pkt_seq, udt->_max_recved_remote_pkt_seq);
	if(SEQ_GEQ(first_not_recved_pkt_seq, udt->_max_recved_remote_pkt_seq))
	{
		return;
	}
	//外面有一个迅雷五的版本，会把pkt_seq填错，前面多添了0x100,例如本来pkt_seq=0xA094,但是该版本会
	//错填充为pkt_seq=0x100A094,导致udt->_max_recved_remote_pkt_seq特别大，这样分配内存就会失败.
	ret = bitmap_resize(bitmap, udt->_max_recved_remote_pkt_seq - first_not_recved_pkt_seq + 1);
	if(ret != SUCCESS)
	{
		LOG_ERROR("[udt = 0x%x, device = 0x%x]udt_get_lost_packet_bitmap, bitmap_resize failed, errcode = %d,bit_count = %u.", udt, udt->_device, ret, udt->_max_recved_remote_pkt_seq - first_not_recved_pkt_seq + 1);
		return;
	}
	for (iter = SET_BEGIN(udt->_udt_recv_buffer_set); iter != SET_END(udt->_udt_recv_buffer_set); iter = SET_NEXT(udt->_udt_recv_buffer_set, iter))
	{
		recv_buffer = (UDT_RECV_BUFFER*)iter->_data;
		bitmap_set(bitmap, recv_buffer->_package_seq - first_not_recved_pkt_seq, TRUE);
	}
	udt_print_bitmap_pkt_info(udt, first_not_recved_pkt_seq, bitmap);
}

void udt_update_last_recv_package_time(UDT_DEVICE* udt)
{
	sd_time_ms(&udt->_last_recv_package_time);
}

void udt_update_last_send_package_time(UDT_DEVICE* udt)
{
	sd_time_ms(&udt->_last_send_package_time);
}

void udt_update_rtt(UDT_DEVICE* udt, _u32 seq, _u32 last_recv_seq, _u32 last_send_time)
{
	_u32 now, raw_rtt;
	sd_time_ms(&now);
	raw_rtt = now - last_send_time;
//	LOG_ERROR("[udt = 0x%x, device = 0x%x]udt_update_rtt, old_rtt = %u", udt, udt->_device, udt->_rtt->_round_trip_time_value);
	if(seq == last_recv_seq)	//last_recv_seq表示对端收到的最后本地的最后一个seq包序号
	{
		rtt_update(udt->_rtt, raw_rtt);
	}
//	LOG_ERROR("[udt = 0x%x, device = 0x%x]udt_update_rtt, new_rtt = %u", udt, udt->_device, udt->_rtt->_round_trip_time_value);
}

_int32 udt_notify_ptl_send_callback(UDT_DEVICE* udt)
{
	_int32 ret = SUCCESS;
	_u32 send_buffer_count = 0;
	MSG_INFO msg = {};
	if(udt->_send_len_record == 0 || udt->_notify_send_msgid != INVALID_MSG_ID)
		return SUCCESS;		//没有必要通知上层数据已发送成功，之前已经通知过了
	send_buffer_count = list_size(&udt->_waiting_send_queue) + list_size(&udt->_had_send_queue);
	if(send_buffer_count >= SEND_BUFFER_NUM)
	{
        LOG_DEBUG("[udt = 0x%x, device = 0x%x]  send calback = %u.", udt, udt->_device, send_buffer_count);
		return SUCCESS;		
	}
	//可以通知上层继续发送数据了
	msg._device_id = 0;
	msg._user_data = udt;
	msg._device_type = DEVICE_NONE;
	msg._operation_type = OP_NONE;
	ret = post_message(&msg, udt_device_notify_send_data, NOTICE_ONCE, 0, &udt->_notify_send_msgid);
	CHECK_VALUE(ret);
	return ret;
}

void udt_print_bitmap_pkt_info(UDT_DEVICE* udt, _u32 pkt_base, BITMAP* bitmap)
{
	_u32 i, count;
	count = bitmap_bit_count(bitmap);
	for(i = 0; i < count; ++i)
	{
		if(bitmap_at(bitmap, i) == FALSE)
		{
			LOG_DEBUG("[udt = 0x%x, device = 0x%x]bitmap_pkt_info, not recv package = %u.", udt, udt->_device, pkt_base + i);
		}
	}
}

