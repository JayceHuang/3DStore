#include "asyn_frame/device.h"
#include "asyn_frame/device_handler.h"
#include "utility/mempool.h"
#include "utility/utility.h"
#include "utility/list.h"
#include "utility/speed_calculator.h"
#include "utility/time.h"
#include "utility/settings.h"
#include "utility/version.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_SOCKET_PROXY
#include "utility/logger.h"

#include "socket_proxy_speed_limit.h"
#include "socket_proxy_interface.h"
#include "socket_proxy_interface_imp.h"

/*每隔200毫秒算一个速度*/
#define		SPEED_SAMPLE_SIZE	(25)
#define		TIMER_INSTANCE		(200)

static	SLAB*	g_socket_recv_request_slab = NULL;
static	SLAB*	g_socket_send_request_slab = NULL;
static	LIST		g_recv_request_list;
static	LIST		g_send_request_list;
static	_u32	g_timer_id = INVALID_MSG_ID;
static	_u32	g_timer_id_ex = 0;

static	_u32	g_last_recv_bytes = 0;
static	_u32	g_last_send_bytes = 0;
static	_u32	g_download_speed = 0;
static	_u32	g_max_download_speed = 0;

static	_u32	g_upload_speed = 0;
static	_u32	g_download_limit_speed = -1;
static	_u32	g_upload_limit_speed = -1;
static	_u32	g_can_recv_bytes;
static	_u32	g_can_send_bytes;
static	_u32	g_p2s_recv_block_size;			/*p2s receive block size*/
static	SPEED_CALCULATOR g_download_speed_calculator;
static	SPEED_CALCULATOR g_upload_speed_calculator;

#define			MAX_FAILED_TIMES	10				//超过这个次数就把特别大的包丢出去
static	_u32	g_retry_recv_failed_times = 0;				//某个数据包发送失败次数

_u32 speed_limit_get_download_speed()
{
	calculate_speed(&g_download_speed_calculator, &g_download_speed);
	LOG_DEBUG("speed_limit_get_download_speed, download_speed = %u.", g_download_speed);
	g_max_download_speed = MAX( g_max_download_speed, g_download_speed );
	return g_download_speed;
}

_u32 speed_limit_get_max_download_speed()
{
	LOG_DEBUG("speed_limit_get_max_download_speed, max_download_speed = %u.", g_max_download_speed);
	return g_max_download_speed;
}

	
_u32 speed_limit_get_upload_speed()
{
	calculate_speed(&g_upload_speed_calculator, &g_upload_speed);
	LOG_DEBUG("speed_limit_get_upload_speed, upload_speed = %u.", g_upload_speed);
	return g_upload_speed;
}

void sl_set_speed_limit(_u32 download_limit_speed, _u32 upload_limit_speed)
{
	g_download_limit_speed = download_limit_speed;
#if defined(MACOS)&&defined(MOBILE_PHONE)
	if(sd_is_ipad3())
	{
		g_download_limit_speed = 300;
	}
#endif
	g_p2s_recv_block_size = 256 * 1024;
	if(g_download_limit_speed != -1)
	{
		g_download_limit_speed = g_download_limit_speed *1024;
		g_p2s_recv_block_size = (g_download_limit_speed / 2) & 0xFFFFE000;			/*8K整数倍*/
		if(g_p2s_recv_block_size == 0)
			g_p2s_recv_block_size = 8 * 1024;
	}
	g_upload_limit_speed = upload_limit_speed;
	if(g_upload_limit_speed != -1)
		g_upload_limit_speed = g_upload_limit_speed * 1024;
	g_can_recv_bytes = g_download_limit_speed;
	g_can_send_bytes = g_upload_limit_speed;
	LOG_DEBUG("sl_set_speed_limit, g_download_limit_speed = %u, g_upload_limit_speed = %u", g_download_limit_speed, g_upload_limit_speed);
}

_int32 sl_cancel_speed_limit(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid)
{
	LOG_DEBUG("sl_cancel_speed_limit errcode = %d", errcode);
	if(errcode == MSG_CANCELLED)
	{
		return SUCCESS; 
	}
	
	sl_set_speed_limit(-1, -1);
	g_timer_id_ex = 0;
	return SUCCESS;
}

//NOTE jieouy:目前登录和高速通道协议都是用的下载库的data_pipe,限速都会被影响。另外sl_set_speed_limit_ex没有返回值。review代码时注释掉。同时这个函数引入了不必要的物理依赖
/*
//登陆和加速的时候要向服务器请求发包，这个时候限制下载流量，目前速度限制为50
void sl_set_speed_limit_ex()
{
	int ret = -1; 
    return;         //目前登录和高速通道协议都是用的下载库的data_pipe,限速会连我们自己也给限制掉，所以不限了。。。。orz...
	_u32 download_limit_speed = 50;
	sl_set_speed_limit(download_limit_speed, -1);
	LOG_DEBUG("sl_set_speed_limit_ex g_timer_id_ex = %d", g_timer_id_ex);
	if(g_timer_id_ex != 0) 
	{
		ret = cancel_timer(g_timer_id_ex);
		LOG_DEBUG("sl_set_speed_limit_ex ret = %d", ret);
		g_timer_id_ex = 0;
	} 
	start_timer(sl_cancel_speed_limit, NOTICE_ONCE,10 * 1000, 0, 0, &g_timer_id_ex);
	LOG_DEBUG("g_timer_id_ex = %d", g_timer_id_ex);
}
*/

void sl_get_speed_limit(_u32* download_limit_speed, _u32* upload_limit_speed)
{
	if(download_limit_speed != NULL)
		*download_limit_speed = g_download_limit_speed / 1024;
	if(upload_limit_speed != NULL)
		*upload_limit_speed = g_upload_limit_speed / 1024;
}

_u32 sl_get_p2s_recv_block_size()
{
	LOG_DEBUG("sl_get_p2s_recv_block_size, block_size = %u, download_speed_limit = %u", g_p2s_recv_block_size, g_download_limit_speed);
	return g_p2s_recv_block_size;
}

_int32 init_socket_proxy_speed_limit()
{
	_int32 ret;
	_u32 download_limit_speed, upload_limit_speed;
	list_init(&g_recv_request_list);
	list_init(&g_send_request_list);
	ret = mpool_create_slab(sizeof(SOCKET_RECV_REQUEST), SOCKET_RECV_REQUEST_SLAB_SIZE, 0, &g_socket_recv_request_slab);
	CHECK_VALUE(ret);
	ret = mpool_create_slab(sizeof(SOCKET_SEND_REQUEST), SOCKET_SEND_REQUEST_SLAB_SIZE, 0, &g_socket_send_request_slab);
	CHECK_VALUE(ret);
	ret = start_timer(speed_limit_timeout_handler, NOTICE_INFINITE, TIMER_INSTANCE, 0, NULL, &g_timer_id);
	LOG_DEBUG("init_socket_proxy_speed_limit,  g_timer_id  = %u ", g_timer_id);  
	if(ret != SUCCESS)
	{
		mpool_destory_slab(g_socket_recv_request_slab);
		g_socket_recv_request_slab = NULL;
		mpool_destory_slab(g_socket_send_request_slab);
		g_socket_send_request_slab = NULL;
	}
	download_limit_speed = -1;
	upload_limit_speed = -1;
	settings_get_int_item("system.download_limit_speed", (_int32*)&download_limit_speed);
	settings_get_int_item("system.upload_limit_speed", (_int32*)&upload_limit_speed);
	sl_set_speed_limit(download_limit_speed, upload_limit_speed);
	init_speed_calculator(&g_download_speed_calculator, 20, 500);
	init_speed_calculator(&g_upload_speed_calculator, 20, 500);
	return ret;
}

_int32 uninit_socket_proxy_speed_limit()
{  
	SOCKET_RECV_REQUEST* recv_request = NULL;
	SOCKET_SEND_REQUEST* send_request = NULL;
	LOG_DEBUG("uninit_socket_proxy_speed_limit, g_recv_request_list size = %u ", list_size(&g_recv_request_list));  
	//如果还有未发出去的请求，通通清掉
	while(list_size(&g_recv_request_list) > 0)
	{
		list_pop(&g_recv_request_list, (void**)&recv_request);
		//这里不能再回调通知了，上层的数据结构可能全析构了
		mpool_free_slip(g_socket_recv_request_slab, recv_request);
	}
	while(list_size(&g_send_request_list) > 0)
	{
		list_pop(&g_send_request_list, (void**)&send_request);
		mpool_free_slip(g_socket_send_request_slab, send_request);
	}
	cancel_timer(g_timer_id);
	LOG_DEBUG("uninit_socket_proxy_speed_limit, cancel g_timer_id  = %u ", g_timer_id);  
	g_timer_id = INVALID_MSG_ID;
	uninit_speed_calculator(&g_download_speed_calculator);
	uninit_speed_calculator(&g_upload_speed_calculator);
	mpool_destory_slab(g_socket_recv_request_slab);
	g_socket_recv_request_slab = NULL;
	mpool_destory_slab(g_socket_send_request_slab);
	g_socket_send_request_slab = NULL;
	return SUCCESS;
}

_int32 speed_limit_peek_op_count(SOCKET sock, _u16 type, _u32* speed_limit_count)
{
	SOCKET_RECV_REQUEST* recv_request = NULL;
	SOCKET_SEND_REQUEST* send_request = NULL;
	LIST_ITERATOR iter;
	*speed_limit_count = 0;
	for(iter = LIST_BEGIN(g_recv_request_list); iter != LIST_END(g_recv_request_list); iter = LIST_NEXT(iter))
	{
		recv_request = (SOCKET_RECV_REQUEST*)LIST_VALUE(iter);
		if(recv_request->_sock == sock && recv_request->_type == type)
		{
			++(*speed_limit_count);
		}
	}
	for(iter = LIST_BEGIN(g_send_request_list); iter != LIST_END(g_send_request_list); iter = LIST_NEXT(iter))
	{	
		send_request = (SOCKET_SEND_REQUEST*)LIST_VALUE(iter);
		if(send_request->_sock == sock && send_request->_type == type)
		{
			++(*speed_limit_count);
		}
	}	
	return SUCCESS;
}

_int32 speed_limit_cancel_request(SOCKET sock, _u16 type)		/*cancel all pending recv or send request in speed limit*/
{
	SOCKET_RECV_REQUEST* recv_request = NULL;
	SOCKET_SEND_REQUEST* send_request = NULL;
	LIST_ITERATOR iter;
	//取消下载
	for(iter = LIST_BEGIN(g_recv_request_list); iter != LIST_END(g_recv_request_list); iter = LIST_NEXT(iter))
	{
		recv_request = (SOCKET_RECV_REQUEST*)LIST_VALUE(iter);
		if(recv_request->_sock == sock && recv_request->_type == type)
		{
			recv_request->_is_cancel = TRUE;
		}
	}
	//取消上传
	for(iter = LIST_BEGIN(g_send_request_list); iter != LIST_END(g_send_request_list); iter = LIST_NEXT(iter))
	{
		send_request = (SOCKET_SEND_REQUEST*)LIST_VALUE(iter);
		if(send_request->_sock == sock && send_request->_type == type)
		{
			send_request->_is_cancel = TRUE;
		}
	}
	return SUCCESS;
}

_int32 speed_limit_timeout_handler(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid)
{
	static _u32 count = 0;
	_u32	recv_bytes, send_bytes;
	LOG_DEBUG("speed_limit_timeout, msgid = %u, errcode = %d", msgid, errcode);
	if(errcode == MSG_CANCELLED)
		return SUCCESS;
	++count;
	if(count % 5 == 0)
	{
		recv_bytes = socket_tcp_device_recved_bytes() + socket_udp_device_recved_bytes();
		add_speed_record(&g_download_speed_calculator, recv_bytes - g_last_recv_bytes);
		calculate_speed(&g_download_speed_calculator, &g_download_speed);
		g_last_recv_bytes = recv_bytes;
		if(g_download_speed < g_download_limit_speed)
		{
			g_can_recv_bytes = g_download_limit_speed + g_download_limit_speed / 10;
		}
		else
		{
			g_can_recv_bytes = 0;
		}
		//更新upload信息
		send_bytes = socket_tcp_device_send_bytes() + socket_udp_device_send_bytes();
		add_speed_record(&g_upload_speed_calculator, send_bytes - g_last_send_bytes);
		calculate_speed(&g_upload_speed_calculator, &g_upload_speed);
		g_last_send_bytes = send_bytes;
		if(g_upload_speed < g_upload_limit_speed)
		{
			g_can_send_bytes = g_upload_limit_speed + g_upload_limit_speed / 10;
		}
		else
		{
			g_can_send_bytes = 0;
		}
		LOG_DEBUG("speed_limit_timeout_handler, g_cur_download_speed = %u, g_can_recv_bytes = %u, g_cur_upload_speed = %u, g_can_send_bytes = %u", 
					g_download_speed, g_can_recv_bytes, g_upload_speed, g_can_send_bytes);
	}
	speed_limit_update_request();
	speed_limit_update_send_request();
	return SUCCESS;
}

/*
_int32 speed_limit_add_send_request(SOCKET sock, _u16 type, const char* buffer, _u32 len, SD_SOCKADDR* addr, void* callback, void* user_data)
{
	_int32 ret = SUCCESS;
	SOCKET_SEND_REQUEST* request = NULL;
	LOG_DEBUG("speed_limit_add_send_request, len = %u.", len);
	ret = mpool_get_slip(g_socket_send_request_slab, (void**)&request);
	CHECK_VALUE(ret);
	request->_sock = sock;
	request->_type = type;
	request->_is_cancel = FALSE;
	request->_buffer = buffer;
	request->_len = len;
	if(addr != NULL)
		request->_addr = *addr;
	request->_callback = callback;
	request->_user_data = user_data;
	request->_is_data = FALSE;
	return list_push(&g_send_request_list, request);
}
*/

_int32 speed_limit_add_send_request_data_item(SOCKET sock
    , _u16 type, const char* buffer, _u32 len, SD_SOCKADDR* addr, void* callback, void* user_data)
{
	_int32 ret = SUCCESS;
	SOCKET_SEND_REQUEST* request = NULL;
	LOG_DEBUG("speed_limit_add_send_request_data_item, len = %u.", len);
	ret = mpool_get_slip(g_socket_send_request_slab, (void**)&request);
	CHECK_VALUE(ret);
	request->_sock = sock;
	request->_type = type;
	request->_is_cancel = FALSE;
	request->_buffer = buffer;
	request->_len = len;
	if(addr != NULL)
		request->_addr = *addr;
	request->_callback = callback;
	request->_user_data = user_data;
	request->_is_data   = TRUE;
	return list_push(&g_send_request_list, request);
}


_int32 speed_limit_add_recv_request(SOCKET sock, _u16 type, char* buffer, _u32 len, void* callback, void* user_data, BOOL oneshot)
{
	_int32 ret;
	SOCKET_RECV_REQUEST* request = NULL;
	LOG_DEBUG("speed_limit_add_recv_request, len = %u.", len);
	ret = mpool_get_slip(g_socket_recv_request_slab, (void**)&request);
	CHECK_VALUE(ret);
	request->_sock = sock;
	request->_type = type;
	request->_is_cancel = FALSE;
	request->_buffer = buffer;
	request->_len = len;
	request->_callback = callback;
	request->_user_data = user_data;
	request->_oneshot = oneshot;
	return list_push(&g_recv_request_list, request);
}

void speed_limit_update_request()
{
	LIST_ITERATOR iter;
	SOCKET_RECV_REQUEST* request = NULL;
	_u32 pending_op_count = 0;
	/*process a request*/
	while(TRUE)
	{
		iter = LIST_BEGIN(g_recv_request_list);
		request = (SOCKET_RECV_REQUEST*)LIST_VALUE(iter);
		if(request == NULL)
		{
			LOG_DEBUG("speed_limit_update_request, request == NULL, break.");
			break;
		}
		if(request->_is_cancel == FALSE)
		{
			if(request->_len < g_can_recv_bytes)
			{
				g_can_recv_bytes -= request->_len;
			}
			else if(g_retry_recv_failed_times < MAX_FAILED_TIMES)
			{
				LOG_DEBUG("speed_limit_update_request, request_len(%u) >= g_can_recv_bytes(%u), break.", request->_len, g_can_recv_bytes);
				++g_retry_recv_failed_times;
				break;
			}
			g_retry_recv_failed_times = 0;		//该包已经成功发送
			list_pop(&g_recv_request_list, (void**)&request);
			if(request->_type == DEVICE_SOCKET_TCP)
			{
				LOG_DEBUG("speed_limit_update_recv_request, g_cur_speed = %u, can_recv_bytes = %u, send a recv request, data_len = %u.", g_download_speed, g_can_recv_bytes, request->_len);
				socket_proxy_recv_impl(request);
			}
			else
			{
				LOG_DEBUG("speed_limit_update_recv_request, g_cur_speed = %u, can_recv_bytes = %u, send a recvfrom request, data_len = %u.", g_download_speed, g_can_recv_bytes, request->_len);
				sd_assert(request->_type == DEVICE_SOCKET_UDP);
				socket_proxy_recvfrom_impl(request);
			}
		}
		else
		{	//这个请求已经被cancel了
			list_pop(&g_recv_request_list, (void**)&request);		//一定要先弹出该请求再计算op_count
			socket_proxy_peek_op_count(request->_sock, request->_type, &pending_op_count);
			if(request->_type == DEVICE_SOCKET_TCP)
				((socket_proxy_recv_handler)request->_callback)(MSG_CANCELLED, pending_op_count, request->_buffer, request->_len, request->_user_data);
			else 
				((socket_proxy_recvfrom_handler)request->_callback)(MSG_CANCELLED, pending_op_count, request->_buffer, request->_len, NULL, request->_user_data);
		}
		mpool_free_slip(g_socket_recv_request_slab, request);
	}
}

void speed_limit_update_send_request()  // 只针对数据包进行限速
{
	_u32 pending_op_count = 0;
	LIST_ITERATOR iter;
	SOCKET_SEND_REQUEST* request = NULL;
	while(TRUE)
	{
		iter = LIST_BEGIN(g_send_request_list);
		request = (SOCKET_SEND_REQUEST*)LIST_VALUE(iter);
		if(request == NULL)
		{
			LOG_DEBUG("speed_limit_update_send_request, request == NULL, break.");
			break;
		}
		if(request->_is_cancel == FALSE)
		{
            if(request->_len < g_can_send_bytes)
			{
				g_can_send_bytes -= request->_len;
			}
			else if(request->_len >= g_upload_limit_speed && g_can_send_bytes > 0)
			{
			    LOG_DEBUG("request len = %d bigger than g_upload_limit_speed = %d, g_can_send_bytes = %d"
			        , request->_len, g_upload_limit_speed, g_can_send_bytes);
			    g_can_send_bytes = 0;
			}
			else
			{
			    LOG_DEBUG("speed_limit_update_send_request, request_len(%u) > g_can_send_bytes(%u), break."
			        , request->_len, g_can_send_bytes);
			    break;
			}

			if(request->_type == DEVICE_SOCKET_TCP)
			{
			    LOG_DEBUG("speed_limit_update_send_request, g_cur_upload_speed = %u \
			        , can_upload_bytes = %u, upload a tcp request, data_len = %u." \
			        , g_upload_speed, g_can_send_bytes, request->_len);
				socket_proxy_send_impl(request);
		    }
			else
			{
                LOG_DEBUG("speed_limit_update_send_request, g_cur_upload_speed = %u \
			        , can_upload_bytes = %u, upload a udp request, data_len = %u." \
			        , g_upload_speed, g_can_send_bytes, request->_len);
				socket_proxy_sendto_impl(request);
			}
			list_pop(&g_send_request_list, (void**)&request);
		}
		else
		{	//请求被cancel了
			list_pop(&g_send_request_list, (void**)&request);	//一定要先弹出该请求再计算op_count
			socket_proxy_peek_op_count(request->_sock,  request->_type, &pending_op_count);
			if(request->_type == DEVICE_SOCKET_TCP)
				((socket_proxy_send_handler)request->_callback)(MSG_CANCELLED,  pending_op_count, request->_buffer, request->_len, request->_user_data);
			else
				((socket_proxy_sendto_handler)request->_callback)(MSG_CANCELLED, pending_op_count,  request->_buffer, request->_len, request->_user_data);
		}
		mpool_free_slip(g_socket_send_request_slab, request);
	}
}


