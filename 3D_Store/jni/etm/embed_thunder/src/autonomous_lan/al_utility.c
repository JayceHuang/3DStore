#ifdef AUTO_LAN

#include "platform/sd_socket.h"
#include "utility/mempool.h"
#include "utility/utility.h"
#include "utility/string.h"
#include "utility/errcode.h"

#include "al_utility.h"


#define	ICMP_PACKET_SIZE	34
static char g_icmp_packet[ICMP_PACKET_SIZE] = {0};

/************************inter function******************************/
_u16 al_check_sum(_u16* data, _int32 len)  
{
	_u32 sum = 0;
	while(len > 1) 
	{ 
		sum += *data++;  
		len -= 2;  
	}  
	if (len) 
	{ 
		sum += *(_u16*)data;  
	}  
	sum = (sum>>16)+(sum&0xffff);   
	sum += (sum>>16);  
	return (_u16)(~sum);
}

/************************out function******************************/
_int32 al_build_icmp_packet(char** buffer, _int32* len, _u16 seq)
{	
	SD_ICMP_HEADER* icmp = (SD_ICMP_HEADER*)g_icmp_packet;
	*buffer = g_icmp_packet;
	*len = ICMP_PACKET_SIZE;
	icmp->_type = ICMP_ECHO;
	icmp->_code = 0;
	icmp->_checksum = 0;
	icmp->_id = sd_htons(512);	//ping的icmp_id必须为512，原因未知
	icmp->_sequence = seq; 
	sd_memcpy(g_icmp_packet + sizeof(SD_ICMP_HEADER), "abcdefghijklmnopqrstuvwxyz", 26);
	icmp->_checksum = al_check_sum((_u16*)g_icmp_packet, ICMP_PACKET_SIZE); 
	return SUCCESS;
}

BOOL al_is_lan_ip(_u32 ip)
{
	_u32 host_ip = sd_ntohl(ip);
	if(ip == sd_inet_addr("0.0.0.0") || ip == sd_inet_addr("127.0.0.1") ||
	   ip == sd_inet_addr("224.0.0.1") || ip == sd_inet_addr("255.255.255.255") ||
	   (host_ip >= sd_ntohl(sd_inet_addr("10.0.0.0")) && host_ip <= sd_ntohl(sd_inet_addr("10.255.255.255"))) ||
	   (host_ip >= sd_ntohl(sd_inet_addr("169.254.0.0")) && host_ip <= sd_ntohl(sd_inet_addr("169.254.255.255"))) ||
	   (host_ip >= sd_ntohl(sd_inet_addr("172.16.0.0")) && host_ip <= sd_ntohl(sd_inet_addr("172.31.255.255"))) ||
	   (host_ip >= sd_ntohl(sd_inet_addr("192.168.0.0")) && host_ip <= sd_ntohl(sd_inet_addr("192.168.255.255"))))
	{
	
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

#endif

