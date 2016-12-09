#ifdef AUTO_LAN
#include "utility/define.h"
#include "utility/utility.h"
#include "utility/time.h"
#include "utility/logid.h"
#define LOGID LOGID_AUTO_LAN
#include "utility/logger.h"

#include "al_ping.h"
#include "al_impl.h"
#include "al_utility.h"

/************************inter function******************************/
_int32 al_ping_send_icmp_packet(SOCKET sock, _u32 dest_ip, _u16 seq)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL;
	_int32 len = 0;
	SD_SOCKADDR	addr;
	addr._sin_family = SD_AF_INET;
	addr._sin_addr = dest_ip;
	addr._sin_port = 0;
	ret = al_build_icmp_packet(&buffer, &len, seq);
	CHECK_VALUE(ret);
	ret = sd_sendto(sock, buffer, len, &addr, &len);
	if(ret != SUCCESS)
	{
		LOG_ERROR("al_ping_send_icmp_packet failed, errcode = %d.", ret);
	}
	return ret;
}

BOOL al_is_correct_reply(char* buffer, _int32 len, _u32 seq)
{
	SD_IP_HEADER* ip = NULL;
	SD_ICMP_HEADER* icmp = NULL;
	_u32 ip_header_len = 0;
	if(len < sizeof(SD_IP_HEADER) + sizeof(SD_ICMP_HEADER))
	{
		LOG_DEBUG("al_is_correct_reply,FALSE: len = %u too short!<%u.", len,sizeof(SD_IP_HEADER) + sizeof(SD_ICMP_HEADER) );
		return FALSE;
	}
	ip = (SD_IP_HEADER*)buffer;
	ip_header_len = ((ip->_ip_verlen & 0x0f) << 2);
	icmp = (SD_ICMP_HEADER*)(buffer + ip_header_len);
	if(icmp->_type == ICMP_ECHOREPLY && icmp->_sequence == seq)
		return TRUE;
	else
	{
		LOG_DEBUG("al_is_correct_reply,FALSE: icmp->_type = %d,icmp->_sequence=%u, seq=%u.", icmp->_type,icmp->_sequence,seq );
		return FALSE;
	}
}

/************************out function******************************/
_int32 al_start_ping(SOCKET sock, _u32 dest_ip)
{
	_int32 ret = SUCCESS;
	char buffer[512] = {0};
	_u32 buffer_len = 512;
	_int32 recv_len = 0, i;
	SD_SOCKADDR addr;
	_u16 seq = sd_rand();
	_u32 send_time = 0, now = 0;
	LOG_DEBUG("auto_lan start ping, dest_ip = %u.", dest_ip);
	
	if(seq>=65530) seq-=PING_NUM;
	
	for(i = 0; i < PING_NUM; ++i)
	{
		LOG_DEBUG("al_ping_send_icmp_packet, dest_ip = %u, seq = %u.", dest_ip, seq + 1);
		sd_time_ms(&send_time);
		al_ping_send_icmp_packet(sock, dest_ip, ++seq);
		while(TRUE)
		{
			ret = sd_recvfrom(sock, buffer, buffer_len, &addr, &recv_len);
			if(ret == SUCCESS)
			{
				sd_time_ms(&now);
				LOG_DEBUG("al_ping_recvfrom, dest_ip = %u, now = %u.", addr._sin_addr,now);
				if((dest_ip==addr._sin_addr)&&(al_is_correct_reply(buffer, recv_len, seq) == TRUE))
				{
					al_on_ping_result(TRUE,TIME_SUBZ(now, send_time) );
					break;
				}
				else
				{
					LOG_DEBUG("al_ping_recvfrom:uncorrect_reply!");
				}
			}
			
			sd_time_ms(&now);
			// 如果不是正确的报文,继续接收,如果超时则退出
			if(TIME_SUBZ(now, send_time) >= PING_TIMEOUT)
			{
				al_on_ping_result(TRUE, PING_TIMEOUT);	//超时
				break;
			}
			sd_sleep(20);
		}
	}
	return SUCCESS;
}






BOOL al_is_ping_valid(SOCKET sock, _u32 dest_ip)
{
	_int32 ret = SUCCESS;
	char buffer[512] = {0};
	_u32 buffer_len = 512;
	_int32 recv_len = 0;
	SD_SOCKADDR addr;
	_u16 seq = 0;
	_u32 send_time = 0, now = 0;
	
		LOG_DEBUG("al_ping_send_icmp_packet, dest_ip = %u, seq = %u.", dest_ip, seq );
		sd_time_ms(&send_time);
		ret=al_ping_send_icmp_packet(sock, dest_ip, seq);
		if(ret!=SUCCESS) return FALSE;
		
		while(TRUE)
		{
			ret = sd_recvfrom(sock, buffer, buffer_len, &addr, &recv_len);
			if(ret == SUCCESS)
			{
				sd_time_ms(&now);
				LOG_DEBUG("al_ping_recvfrom, dest_ip = %u, now = %u.", addr._sin_addr,now);
				if((dest_ip==addr._sin_addr)&&(al_is_correct_reply(buffer, recv_len, seq) == TRUE))
				{
					LOG_DEBUG("al_is_ping_valid:TRUE!");
					return TRUE;
				}
				else
				{
					LOG_DEBUG("al_ping_recvfrom:uncorrect_reply!");
				}
			}
			
			sd_time_ms(&now);
			// 如果不是正确的报文,继续接收,如果超时则退出
			if(TIME_SUBZ(now, send_time) >= PING_TIMEOUT)
			{
				LOG_DEBUG("al_is_ping_valid:FALSE!");
				return FALSE;
			}
			sd_sleep(20);
		}

	LOG_DEBUG("al_is_ping_valid:FALSE!");
	return FALSE;
}

#endif

