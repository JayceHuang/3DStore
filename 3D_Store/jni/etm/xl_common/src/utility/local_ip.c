#include "utility/define.h"
#if defined(LINUX)	
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#elif  defined( WINCE)
#include "platform/wm_interface/sd_wm_network_interface.h"
#elif defined(_ANDROID_LINUX)
#include "platform/android_interface/sd_android_network_interface.h"
#endif

#include "utility/local_ip.h"
#include "utility/string.h"
#include "utility/errcode.h"
#include "utility/utility.h"
#include "platform/sd_socket.h"
#include "platform/sd_network.h"
#include "utility/logid.h"
#define	LOGID	LOGID_COMMON
#include "utility/logger.h"


/*network byte order*/
static _u32 g_local_ip = 0;	

_u32 sd_get_local_ip(void)
{
#if defined(LINUX)

#ifndef MACOS

	struct ifreq ifr, *pifr;
	struct ifconf ifc;
	_u32 tmp_fd = 0;
	_int32 idx = 0, ret_val = 0;
	char if_buf[1024];
	struct sockaddr_in *paddr;


	if(g_local_ip == 0)
	{
		/* traverse network platform and get local ip */

		ret_val = sd_create_socket(SD_AF_INET, SD_SOCK_DGRAM, 0, &tmp_fd);
		if(ret_val != SUCCESS)
			return 0;

		ifc.ifc_len = sizeof(if_buf);
		ifc.ifc_buf = if_buf;
		ioctl(tmp_fd, SIOCGIFCONF, &ifc);

		pifr = ifc.ifc_req;
		for(idx = ifc.ifc_len / sizeof(struct ifreq); --idx >= 0; pifr++)
		{
			sd_strncpy(ifr.ifr_name, pifr->ifr_name, IFNAMSIZ);

			if(ioctl(tmp_fd, SIOCGIFFLAGS, &ifr) == 0)
			{
				if(!(ifr.ifr_flags & IFF_LOOPBACK)) 
				{
					if(ioctl(tmp_fd, SIOCGIFADDR, &ifr) == 0) 
					{
						paddr = (struct sockaddr_in*)(&ifr.ifr_addr);
						g_local_ip = paddr->sin_addr.s_addr;
						if(!sd_is_in_nat(g_local_ip))
							break;
					}
				}
			}
		}

		sd_close_socket(tmp_fd);
	}
#else

	struct ifreq ifr, *pifr;
	struct ifconf ifc;
	_u32 tmp_fd = 0;
	_int32 idx = 0, ret_val = 0;
	char if_buf[1024];
	struct sockaddr_in *paddr;
	char* cp, *cplim;

	if(g_local_ip == 0)
	{
		/* traverse network platform and get local ip */

		ret_val = sd_create_socket(SD_AF_INET, SD_SOCK_DGRAM, 0, &tmp_fd);
		if(ret_val != SUCCESS)
			return 0;

		ifc.ifc_len = sizeof(if_buf);
		ifc.ifc_buf = if_buf;
		ioctl(tmp_fd, SIOCGIFCONF, &ifc);

		pifr = ifc.ifc_req;
		cp = if_buf;
		cplim = if_buf + ifc.ifc_len;
		
		for(cp = if_buf; cp < cplim; )
		{
			pifr = (struct ifreq*)cp;
			
			sd_strncpy(ifr.ifr_name, pifr->ifr_name, IFNAMSIZ);

			if(ioctl(tmp_fd, SIOCGIFFLAGS, &ifr) == 0)
			{
				if(!(ifr.ifr_flags & IFF_LOOPBACK)) 
				{
					if(ioctl(tmp_fd, SIOCGIFADDR, &ifr) == 0) 
					{
						paddr = (struct sockaddr_in*)(&ifr.ifr_addr);
						g_local_ip = paddr->sin_addr.s_addr;
#if defined(MACOS)&&defined(MOBILE_PHONE)
						if(NT_WLAN == sd_get_net_type() && sd_data_cmp((_u8*)cp, "en0", 3))
						{
							//用wifi网络,捕捉该ip
							break;
						}
						else if(NT_3G == sd_get_net_type() && sd_data_cmp((_u8*)cp, "pdp_ip0", 7))
						{
							//用3g网络,捕捉该ip
							break;
						}
#else
						if(!sd_is_in_nat(g_local_ip))
							break;
#endif
					}
				}
			}
			cp += sizeof(pifr->ifr_name) + MAX(sizeof(pifr->ifr_addr), pifr->ifr_addr.sa_len);
		}

		sd_close_socket(tmp_fd);
	}

#endif
	
#elif defined(WINCE)
	if(g_local_ip==0)
	{
		g_local_ip = get_local_ip();
	}
#endif	

#ifdef _DEBUG
	{
		char addr[24] = {0};
		sd_inet_ntoa(g_local_ip, addr, 24);
		LOG_DEBUG("sd_get_local_ip, local_ip = %s", addr);
	}
#endif
	return g_local_ip;
}

void sd_set_local_ip(_u32 ip)
{
#ifdef _DEBUG
	char addr[24] = {0};
	sd_inet_ntoa(ip, addr, 24);
	LOG_ERROR("sd_set_local_ip, local_ip 0x%X= %s", ip,addr);
#endif
	g_local_ip = ip;
}

BOOL sd_is_in_nat(_u32 ip)
{
	_u8* pointer = (_u8*)&ip;
	_u8 c1 = pointer[0];
	_u8 c2 = pointer[1];
	if((c1 == 10) || ((c1 == 172) && (c2 > 15) && (c2 < 32)) || ((c1 == 192) && (c2 == 168)))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

_u32 sd_get_local_netmask(void)
{

#if defined(LINUX)

#ifdef MACOS

	struct ifreq ifr, *pifr;
	struct ifconf ifc;
	_u32 tmp_fd = 0;
	_int32 idx = 0, ret_val = 0;
	char if_buf[1024];
	struct sockaddr_in *pnetmask;
	char* cp, *cplim;

	_u32 netmask = 0;
	char maskbuf[24] = {0};

	if(0x80000 != sd_get_net_type())
	{
		//用wifi网络才需要子网掩码	
		return 0;
	}

	if(netmask == 0)
	{
		/* traverse network platform and get local ip */

		ret_val = sd_create_socket(SD_AF_INET, SD_SOCK_DGRAM, 0, &tmp_fd);
		if(ret_val != SUCCESS)
			return 0;

		ifc.ifc_len = sizeof(if_buf);
		ifc.ifc_buf = if_buf;
		ioctl(tmp_fd, SIOCGIFCONF, &ifc);

		pifr = ifc.ifc_req;
		cp = if_buf;
		cplim = if_buf + ifc.ifc_len;
		
		for(cp = if_buf; cp < cplim; )
		{
			pifr = (struct ifreq*)cp;
			
			sd_memset(ifr.ifr_name, 0x00, IFNAMSIZ);
			sd_strncpy(ifr.ifr_name, pifr->ifr_name, IFNAMSIZ);

			if(ioctl(tmp_fd, SIOCGIFFLAGS, &ifr) == 0)
			{
				if(!(ifr.ifr_flags & IFF_LOOPBACK)) 
				{
					if(ioctl(tmp_fd, SIOCGIFNETMASK, &ifr) == 0) 
					{
						pnetmask = (struct sockaddr_in*)(&ifr.ifr_addr);
						netmask = pnetmask->sin_addr.s_addr;
						sd_inet_ntoa(netmask, maskbuf, 24);
#ifdef MOBILE_PHONE
						if(sd_data_cmp((_u8*)ifr.ifr_name, "en0", 3))
						{
							break;
						}
#else
						if(sd_strlen(maskbuf) > 0)
						{
							break;
						}
#endif
					}					
				}
			}
			cp += sizeof(pifr->ifr_name) + MAX(sizeof(pifr->ifr_addr), pifr->ifr_addr.sa_len);
		}

		sd_close_socket(tmp_fd);
	}
	
	return netmask;

#else
	struct ifreq ifr, *pifr;
	struct ifconf ifc;
	_u32 tmp_fd = 0;
	_int32 idx = 0, ret_val = 0;
	char if_buf[1024];
	struct sockaddr_in *pnetmask;
	_u32 netmask = 0;
	char maskbuf[24] = {0};
	
	if(netmask == 0)
	{
		/* traverse network platform and get local ip */

		ret_val = sd_create_socket(SD_AF_INET, SD_SOCK_DGRAM, 0, &tmp_fd);
		if(ret_val != SUCCESS)
			return 0;

		ifc.ifc_len = sizeof(if_buf);
		ifc.ifc_buf = if_buf;
		ioctl(tmp_fd, SIOCGIFCONF, &ifc);

		pifr = ifc.ifc_req;
		for(idx = ifc.ifc_len / sizeof(struct ifreq); --idx >= 0; pifr++)
		{
			sd_strncpy(ifr.ifr_name, pifr->ifr_name, IFNAMSIZ);

			if(ioctl(tmp_fd, SIOCGIFFLAGS, &ifr) == 0)
			{
				if(!(ifr.ifr_flags & IFF_LOOPBACK)) 
				{
					if(ioctl(tmp_fd, SIOCGIFNETMASK, &ifr) == 0) 
					{
						pnetmask = (struct sockaddr_in*)(&ifr.ifr_addr);
						netmask = pnetmask->sin_addr.s_addr;
						sd_inet_ntoa(netmask, maskbuf, 24);
						if(sd_strlen(maskbuf) > 0)
						{
							break;
						}
					}
				}
			}
		}

		sd_close_socket(tmp_fd);
	}
	return netmask;
#endif
#else
	sd_assert(FALSE);
#endif
	return 0;

}

