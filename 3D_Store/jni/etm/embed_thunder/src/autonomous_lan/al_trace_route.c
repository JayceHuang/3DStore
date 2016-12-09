#ifdef AUTO_LAN

#include "utility/define.h"
#include "utility/utility.h"
#include "utility/time.h"
#include "utility/local_ip.h"
#include "utility/logid.h"
#define LOGID LOGID_AUTO_LAN
#include "utility/logger.h"

#include "platform/sd_socket.h"

#include "al_trace_route.h"
#include "al_utility.h"
#include "al_ping.h"

#define	TRACE_ROUTE_DEST_IP	"58.251.57.82" //"61.129.76.83" //"116.77.16.17"
#define TRACE_ROUTE_TIMEOUT	4000
#define	TRACE_RETRY_TIMES	5

/************************inter function******************************/
_int32 al_trace_send_icmp_packet(_u32 sock, _int32 ttl)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL;
	_int32 len = 0;
	SD_SOCKADDR	addr;
	static _u16 seq = 0;
	addr._sin_family = AF_INET;
	addr._sin_addr = sd_inet_addr(TRACE_ROUTE_DEST_IP);
	addr._sin_port = 0;
	ret = al_build_icmp_packet(&buffer, &len, ++seq);
	CHECK_VALUE(ret);
	ret = sd_set_socket_ttl(sock, ttl);
	CHECK_VALUE(ret);
	ret = sd_sendto(sock, buffer, len, &addr, &len);
	if(ret != SUCCESS)
	{
		LOG_ERROR("al_trace_send_icmp_packet, errcode = %d.", ret);
	}
	else
	{
		LOG_DEBUG("al_trace_send_icmp_packet, ttl = %d.", ttl);
	}
	return ret;
}

/************************out function******************************/
_int32 al_start_trace(SOCKET sock, _u32* external_ip)
{
	_int32 ret = SUCCESS;
	_int32 ttl = 1;
	char buffer[512];
	_u32 buffer_len = 512, fail_times = 0;
	_int32 recv_len = 0;
	SD_SOCKADDR addr;
	_u32 send_time = 0, now = 0;
	_u32 local_ip = sd_get_local_ip(); 
#ifdef _DEBUG
	char ip[20] = {0};
#endif
	*external_ip = 0;
	sd_time_ms(&send_time);
	al_trace_send_icmp_packet(sock, ++ttl);		//ttl从2开始发送，原因未知，迅雷5代码从2开始
	while(TRUE)
	{
		ret = sd_recvfrom(sock, buffer, buffer_len, &addr, &recv_len);
		if(ret == SUCCESS)
		{
			fail_times=0;
#ifdef _DEBUG
			sd_inet_ntoa(addr._sin_addr, ip, 20);
			LOG_DEBUG("trace router reply, ip = %s, ttl = %d", ip, ttl);
#endif
			if((addr._sin_addr==16777343) //"127.0.0.1"
			    ||(addr._sin_addr==local_ip))
			{
				//找到自己，说明没有连接外部网络
				LOG_DEBUG("find myself,the network in not connected to outside...");
				//sd_assert(FALSE);
				return -1;
				
			}

			if(al_is_lan_ip(addr._sin_addr) == FALSE)
			{//如果直接找到了最终地址，那么认为出口路由找不到
				if(addr._sin_addr == sd_inet_addr(TRACE_ROUTE_DEST_IP))
				{
					LOG_DEBUG("find final addr");
					//sd_assert(FALSE);
					return -1;
				}
				else
				if(al_is_ping_valid(sock, addr._sin_addr)==TRUE)
				{
#ifdef _DEBUG
					LOG_DEBUG("auto_lan trace find external_ip = %s, ttl = %d.", ip, ttl);
#endif
					*external_ip = addr._sin_addr;
					//成功之后要把ttl改为正常值
					ret = sd_set_socket_ttl(sock, 64);
					CHECK_VALUE(ret);
					return SUCCESS;
				}
			}
			
			if(ttl > 32)
			{
				return -1;		//只检测32跳
			}
			
			sd_time_ms(&send_time);
			al_trace_send_icmp_packet(sock, ++ttl);
		}
		else
		{
			sd_time_ms(&now);
			if(TIME_SUBZ(now, send_time) >= TRACE_ROUTE_TIMEOUT)
			{	//超时重试
				++fail_times;
				LOG_DEBUG("fail_times=%d",fail_times);
				if(fail_times > TRACE_RETRY_TIMES)
				{
					LOG_ERROR("auto_lan trace failed.");
					return -1;
				}
				sd_time_ms(&send_time);
				al_trace_send_icmp_packet(sock, ttl);
			}
		}
		
		sd_sleep(20);
	}
	return SUCCESS;
}

#endif

