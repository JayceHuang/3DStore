#include "platform/sd_network.h"
#if defined( LINUX) && !defined(_ANDROID_LINUX)

#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "platform/sd_socket.h"
#include "utility/utility.h"
#include <sys/ioctl.h>
#include <sys/types.h>


#include "utility/local_ip.h"
#ifdef MACOS
#include "platform/ios_interface/sd_ios_device_info.h"
#endif /* MACOS */

#ifdef LOGID
#undef LOGID
#endif
#define LOGID LOGID_INTERFACE
#include "utility/logger.h"

#define EXPORT_C  

static BOOL g_net_init = FALSE;

static _u32 g_net_iap_id = MAX_U32;
static _u32 g_net_type = 0;
static SD_NET_STATUS g_net_status = SNS_DISCNT;
static SD_NOTIFY_NET_CONNECT_RESULT g_notify_net_call_back_func=(SD_NOTIFY_NET_CONNECT_RESULT)NULL;
static _u32 g_proxy_ip = 0;
static _u16 g_proxy_port = 80;

static _u32 g_global_net_type = NT_WLAN;

EXPORT_C _int32 sd_init_network(_u32 iap_id, SD_NOTIFY_NET_CONNECT_RESULT call_back_func)
{
	LOG_ERROR("sd_init_network,iap_id=%u: g_net_status=%u",iap_id, g_net_status);
	g_net_iap_id = iap_id;
	if(g_net_status != SNS_DISCNT) return SUCCESS;
	g_net_status = SNS_CNTING;
	g_net_type = 0;
	g_notify_net_call_back_func = call_back_func;
	return SUCCESS;
}

EXPORT_C _int32 sd_uninit_network(void)
{
	LOG_ERROR("sd_uninit_network: g_net_status=%u", g_net_status);
	g_net_status = SNS_DISCNT;
	g_net_init = FALSE;
	g_net_type = 0;
	return SUCCESS;
}

EXPORT_C SD_NET_STATUS sd_get_network_status(void)
{
 	//_u32 local_ip = 0;
	//BOOL is_gprs = FALSE;
	
	sd_is_net_ok();
	if(SNS_CNTED==g_net_status)
	{
/*		
		local_ip = sd_get_local_ip();
		if((local_ip&0x000000FF)==0x0A)
		{
			is_gprs = TRUE;
		}
		if(is_gprs)
		{
			if(g_net_type == 0||g_net_type == NT_WLAN)
			{
				g_net_type = NT_CMNET;
			}
		}
		else
		{
			if(g_net_type != 0&&g_net_type != NT_WLAN)
			{
				g_net_type = NT_WLAN;
			}
		}
*/		
	}

 	LOG_ERROR("sd_get_network_status: g_net_status=%u,g_net_type=0x%X", g_net_status,g_net_type);
	return g_net_status;
}

EXPORT_C _int32 sd_get_network_iap(_u32 *iap_id)
{
	*iap_id = g_net_iap_id;
	LOG_ERROR("sd_get_network_iap: g_net_iap_id=%u", g_net_iap_id);
	return SUCCESS;
}

EXPORT_C const char* sd_get_network_iap_name(void)
{
	static char * g_iap_name = "unknown";
	return g_iap_name;
}

EXPORT_C void sd_check_net_connection_result(void)
{
	LOG_ERROR("sd_check_net_connection_result: g_net_status=%u", g_net_status);
	if(g_net_status == SNS_CNTING)
	{
		g_net_init = TRUE;
		sd_get_network_status();
		LOG_ERROR("sd_check_net_connection_result 2: g_net_status=%u", g_net_status);
		if(g_net_status == SNS_CNTED)
			((SD_NOTIFY_NET_CONNECT_RESULT)g_notify_net_call_back_func)(g_net_iap_id,SUCCESS,g_net_type);
		else
		{
			g_net_status = SNS_DISCNT;
			g_net_init = FALSE;
			g_net_type = 0;
			((SD_NOTIFY_NET_CONNECT_RESULT)g_notify_net_call_back_func)(g_net_iap_id,-1,0);
		}
	}
	return;
}

EXPORT_C BOOL sd_is_net_ok(void)
{
#ifdef MOBILE_PHONE
 	LOG_ERROR("sd_is_net_ok,g_net_status=%d,g_net_init=%d", g_net_status,g_net_init);
	if(!g_net_init) return FALSE;

 	if(g_net_init)
	{
	#if 0  //def MACOS
		if(check_network_connect()==1)
		{
			//g_net_iap_id = i;
			if(g_net_type == 0)
			{
	 			g_net_type = NT_WLAN;
				LOG_ERROR("sd_is_net_ok, the net is ok:g_net_type=0x%X",g_net_type);
			}
			g_net_status = SNS_CNTED;
			return TRUE;
		}
		else
		{
			g_net_type = 0;
			//((SD_NOTIFY_NET_CONNECT_RESULT)g_notify_net_call_back_func)(g_net_iap_id,NETWORK_NOT_READY,0);
			//set_critical_error(NETWORK_NOT_READY);
		}
	#else
			//g_net_iap_id = i;
			if(g_net_type == 0)
			{
	 			g_net_type = NT_WLAN;
				LOG_ERROR("sd_is_net_ok, the net is ok:g_net_type=0x%X",g_net_type);
			}
			g_net_status = SNS_CNTED;
			return TRUE;
	#endif /* MACOS */
	}
	else
	{
		g_net_type = 0;
	}

	if(g_net_status == SNS_CNTED)
	{
		g_net_status = SNS_DISCNT;
	}

 	LOG_ERROR("sd_is_net_ok,FALSE: g_net_status=%u", g_net_status);

 	return FALSE;
#else
	g_net_status = SNS_CNTED;
	g_net_type = NT_WLAN;
	return TRUE;
#endif
 }

EXPORT_C _u32 sd_get_net_type(void)
{
	return g_net_type;
}
EXPORT_C _int32 sd_set_net_type(_u32 net_type)
{
	g_net_type = net_type;
	return SUCCESS;
}



EXPORT_C _int32 sd_set_proxy(_u32 ip,_u16 port)
{
	g_proxy_ip = ip;
	g_proxy_port = port;

	return SUCCESS;
}
EXPORT_C _int32 sd_get_proxy(_u32 *p_ip,_u16 *p_port)
{
	if(g_proxy_ip==0)
	{
		if(sd_get_net_type() ==(NT_GPRS_WAP|CHINA_TELECOM))
		{
			/* 电信WAP */
			g_proxy_ip = 0xC800000A; //10.0.0.200
		}
		else
		{
			struct sockaddr_in os_addr;
			_int32 addr_len = sizeof(os_addr),ret_val = SUCCESS;
			_int32 flags = 0;
			unsigned long ul = 0; 

			_u32 sock = socket(SD_AF_INET, SD_SOCK_STREAM, 0);
			if(sock== (_u32)-1) return -1;


			/* set nonblock */
			flags = fcntl(sock, F_GETFL);

			ret_val = fcntl(sock, F_SETFL, flags | O_NONBLOCK);
			CHECK_VALUE(ret_val);

			flags = fcntl(sock, F_GETFL);
			
			ioctl(sock, FIONBIO, &ul); //设置为阻塞模式 

			flags = fcntl(sock, F_GETFL);
			
			os_addr.sin_family = AF_INET;
			os_addr.sin_port = htons(80);
			os_addr.sin_addr.s_addr=sd_inet_addr("10.0.0.172");
			ret_val = connect(sock, (struct sockaddr*)&os_addr, addr_len);
			close(sock);
			if(ret_val==0) 
			{
				g_proxy_ip = 0xAC00000A; //10.0.0.172
			}
			else
			{
				g_proxy_ip = 0xC800000A; //10.0.0.200
			}
		}
		g_proxy_port = 80;
	}
	
	if(p_ip)
		*p_ip = g_proxy_ip;
	
	if(p_port)
		*p_port = g_proxy_port;
	return SUCCESS;
}

EXPORT_C _u32 sd_get_global_net_type(void)
{
    return g_global_net_type;
}

EXPORT_C _int32 sd_set_global_net_type(_u32 net_type)
{
    g_global_net_type = net_type;
    return SUCCESS;
}

#endif

