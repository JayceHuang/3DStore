#ifdef ENABLE_ETM_MEDIA_CENTER
#include "utility/define.h"
#if defined(LINUX)	
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#if defined(MACOS)
#include <net/if_dl.h>
#endif
#elif defined(WINCE)
	#include <windows.h>
	#include <IPHlpApi.h>
#endif

#include "platform/sd_device_info.h"

#include "utility/peerid.h"
#include "utility/string.h"
#include "utility/errcode.h"
#include "utility/utility.h"
#include "platform/sd_socket.h"
#include "utility/settings.h"
#include "utility/logid.h"
#define	LOGID	LOGID_COMMON
#include "utility/logger.h"
#include "platform/sd_fs.h"

static char g_my_peerid[PEER_ID_SIZE + 1] = {0};
static BOOL	g_is_peerid_valid = FALSE;

static _int32 _get_mac_addr_from_file(const char *file, char *buffer, _int32 *bufsize) 
{
	_int32 ret_val = SUCCESS;
	_u32 eth0_addr_fi;
	char addr_str[24] = {0};
	int i, j;
	_u8 tmp;
	_u8 eth0_addr_buff[10] = {0};
	_u32 read_size = 0;
	ret_val = sd_open_ex(file, O_FS_RDONLY, &eth0_addr_fi);
    if (ret_val != SUCCESS)
    {
        LOG_DEBUG("sd_open_ex, ret_val = %d", ret_val);
        return ret_val;
    }
    
	//CHECK_VALUE(ret_val);
	ret_val = sd_read(eth0_addr_fi, addr_str, 24, &read_size);
    if (ret_val != SUCCESS)
    {
        LOG_DEBUG("sd_read, ret_val = %d", ret_val);
        return ret_val;
    }
	//CHECK_VALUE(ret_val);

	if(read_size >= 17)
	{
		for(i = 0, j = 0; i < 24 && addr_str[i];)
		{
			if(addr_str[i] == ':')
			{
				i++;
				continue;
			}
			tmp = 0;
			tmp |= sd_hex_2_int(addr_str[i]);
			tmp <<= 4;
			tmp |= sd_hex_2_int(addr_str[i+1]);
			eth0_addr_buff[j++] = tmp;
			i++;
			i++;
		}
	}
	else 
	{
		ret_val = -1;
	}

	if (ret_val == SUCCESS) 
	{
		sd_memcpy(buffer, eth0_addr_buff, 6);
		*bufsize = 6;
	}

	ret_val = sd_close_ex(eth0_addr_fi);
	CHECK_VALUE(ret_val);

	return ret_val;
}

#ifdef _HUAWEI_ANDROID_HAISI
#define ETH0_ADDR_FILE_PATH "/sys/class/net/wlan0/address"
#else
#define ETH0_ADDR_FILE_PATH "/sys/class/net/eth0/address"
#endif
 _int32 get_physical_address(char *buffer, _int32 *bufsize)
{
	_int32 ret_val = SUCCESS;


#ifdef LINUX
	ret_val = _get_mac_addr_from_file(ETH0_ADDR_FILE_PATH, buffer, bufsize);
    if (ret_val == SUCCESS)
    {
        return ret_val;
    }

#ifndef MACOS
	struct ifreq ifr, *pifr;
	struct ifconf ifc;
	_u32 tmp_fd = 0;
	_int32 idx = 0;
	char if_buf[1024];
	char virtual_net[6] = {0x00, 0x50, 0x56, 0xC0, 0x00, 0x01};			//虚拟机生成的网卡
	sd_assert(*bufsize >= 6);

	ret_val = sd_create_socket(SD_AF_INET, SD_SOCK_DGRAM, 0, &tmp_fd);
	CHECK_VALUE(ret_val);

	ifc.ifc_len = sizeof(if_buf);
	ifc.ifc_buf = if_buf;
	ioctl(tmp_fd, SIOCGIFCONF, &ifc);

	ret_val = NOT_FOUND_MAC_ADDR;
	pifr = ifc.ifc_req;
	for(idx = ifc.ifc_len / sizeof(struct ifreq); --idx >= 0; pifr++)
	{
		sd_strncpy(ifr.ifr_name, pifr->ifr_name, IFNAMSIZ);

		if(ioctl(tmp_fd, SIOCGIFFLAGS, &ifr) == 0)
		{
			if(!(ifr.ifr_flags & IFF_LOOPBACK)) 
			{
				if(ioctl(tmp_fd, SIOCGIFHWADDR, &ifr) == 0) 
				{
					sd_memset(buffer, 0, *bufsize);
					if(sd_data_cmp((_u8*)ifr.ifr_hwaddr.sa_data, (_u8*)buffer, 6) == TRUE)		//过滤地址为全0的网卡
						continue;
					if(sd_data_cmp((_u8*)ifr.ifr_hwaddr.sa_data, (_u8*)virtual_net, 6) == TRUE)	//过滤虚拟机生成的网卡
						continue;				
					ret_val = SUCCESS;
					break;
				}
			}

		}
	}

	sd_close_socket(tmp_fd);

	if (ret_val == SUCCESS) 
	{
		sd_memcpy(buffer, ifr.ifr_hwaddr.sa_data, 6);
		*bufsize = 6;
	}
 #else
	struct ifreq ifr, *pifr;
	struct ifconf ifc;
	_u32 tmp_fd = 0;
	_int32 idx = 0;
	char if_buf[1024];
	char virtual_net[6] = {0x00, 0x50, 0x56, 0xC0, 0x00, 0x01};			//虚拟机生成的网卡
	char* cp, *cplim;
	char temp[80];
	
	sd_assert(*bufsize >= 6);

	ret_val = sd_create_socket(SD_AF_INET, SD_SOCK_DGRAM, 0, &tmp_fd);
	CHECK_VALUE(ret_val);

	ifc.ifc_len = sizeof(if_buf);
	ifc.ifc_buf = if_buf;
	ioctl(tmp_fd, SIOCGIFCONF, &ifc);

	ret_val = NOT_FOUND_MAC_ADDR;
	pifr = ifc.ifc_req;
	cp = if_buf;
	cplim = if_buf + ifc.ifc_len;
	
	for(cp = if_buf;cp < cplim; )
	{
		pifr = (struct ifreq *)cp;
		if (pifr->ifr_addr.sa_family == AF_LINK)
		{
			struct sockaddr_dl *sdl = (struct sockaddr_dl *)&pifr->ifr_addr;
			int a,b,c,d,e,f;
			
			sd_memset(temp, 0, sizeof(temp));
			sd_strncpy(temp, (char *)ether_ntoa(LLADDR(sdl)), sizeof(temp));
			sscanf(temp, "%x:%x:%x:%x:%x:%x", &a, &b, &c, &d, &e, &f);
			temp[0]=a;
			temp[1]=b;
			temp[2]=c;
			temp[3]=d;
			temp[4]=e;
			temp[5]=f;
			//sprintf(temp, "%02X%02X%02X%02X%02X%02X",a,b,c,d,e,f);
			if(!(pifr->ifr_flags & IFF_LOOPBACK)) 
			{
				sd_memset(buffer, 0, *bufsize);
#if defined(MACOS)&&defined(MOBILE_PHONE)
				if(sd_data_cmp((_u8*)pifr->ifr_name, "en1", 3) == TRUE)	//ipad下过滤掉网卡eth1地址
				{
					cp += sizeof(pifr->ifr_name) + MAX(sizeof(pifr->ifr_addr), pifr->ifr_addr.sa_len);
					continue;
				}
#endif
				if(sd_data_cmp((_u8*)temp, (_u8*)buffer, 3) == TRUE)		//π??àμ?÷∑???′0μ??ˉ??
				{
					cp += sizeof(pifr->ifr_name) + MAX(sizeof(pifr->ifr_addr), pifr->ifr_addr.sa_len);
					continue;
				}
				if(sd_data_cmp((_u8*)temp, (_u8*)virtual_net, 6) == TRUE)	//π??à–è??a˙…˙≥…μ??ˉ??
				{
					cp += sizeof(pifr->ifr_name) + MAX(sizeof(pifr->ifr_addr), pifr->ifr_addr.sa_len);
					continue;
				}
				ret_val = SUCCESS;
				break;
			}
			
		}
		cp += sizeof(pifr->ifr_name) + MAX(sizeof(pifr->ifr_addr), pifr->ifr_addr.sa_len);

	}

	sd_close_socket(tmp_fd);

	if (ret_val == SUCCESS) 
	{
		sd_memcpy(buffer, (_u8*)temp, 6);
		*bufsize = 6;
	}

 #endif
#elif  defined(WINCE)
	IP_ADAPTER_INFO Data[12];
	ULONG len = sizeof(IP_ADAPTER_INFO) * 12;
	PIP_ADAPTER_INFO pDatas = Data;
	char*   pDesc;
	_int32 i;

	if((GetAdaptersInfo(pDatas, &len)) == ERROR_SUCCESS)
	{
		if( pDatas )
		{
			for(i = 1; i < 12; i++)
			{
				pDesc = pDatas->Description;
				if ( sd_strncmp((const char*)pDesc, "PPP", sd_strlen("PPP")) == 0
					|| sd_strncmp((const char*)pDesc, "VMare", sd_strlen("VMare")) == 0
					|| sd_strncmp((const char*)pDesc, "Virtual", sd_strlen("Virtual")) == 0
					|| sd_strncmp((const char*)pDesc, "SLIP", sd_strlen("SLIP")) == 0
					|| sd_strncmp((const char*)pDesc, "P.P.P", sd_strlen("P.P.P")) == 0
					)
				{
					pDatas = pDatas->Next;
					if(pDatas == NULL)
					{
					    ret_val = -1;
						break;
					}
				}
				else
				{
					sd_memcpy(buffer, pDatas->Address, 6);
					*bufsize = 6;
					ret_val = SUCCESS;
					break;
				}
			}
		}
	}
#endif

	return ret_val;
}

_int32 get_peerid(char *buffer, _int32 bufsize)
{
	_int32 ret_val = SUCCESS;
	char physical_addr[10];
	_int32 physical_addr_len = 10;
	const char * p_imei = NULL;

#ifdef CDN_PEERID_SUFFIX
	char peerid_suffix[5];
#endif
	LOG_ERROR("get_peerid, g_is_peerid_valid=%d,g_my_peerid=%s", g_is_peerid_valid,g_my_peerid);

	//如果g_my_peerid有效，则直接赋值，不用再计算peerid
	if(g_is_peerid_valid == TRUE)
	{
		sd_assert(sd_strncmp(g_my_peerid, "0000000000", 10)!=0);
		return sd_memcpy(buffer, g_my_peerid, MIN(bufsize, PEER_ID_SIZE + 1));
	}
    
	/* 其他平台都用MAC地址作为PEERID */
	ret_val = get_physical_address(physical_addr, &physical_addr_len);
	if(ret_val == SUCCESS)
	{
		/* convert to peerid */
		ret_val = str2hex(physical_addr, physical_addr_len, g_my_peerid, PEER_ID_SIZE);
		CHECK_VALUE(ret_val);
		sd_assert(sd_strncmp(g_my_peerid, "0000000000", 10)!=0);
		g_is_peerid_valid = TRUE;
	}
	else
	{
		/* 获取MAC 地址失败，尝试获取IMEI号 */
		p_imei = get_imei();
#if defined(_ANDROID_LINUX)		
		if (p_imei != NULL)
		{
			/* 用IMEI(15bytes)号或MEID(14bytes)作为peerid */
			sd_strncpy(g_my_peerid, "XXXXXXXXXXXX004V", PEER_ID_SIZE);
			sd_strncpy(g_my_peerid, p_imei, MIN(sd_strlen(p_imei), PEER_ID_SIZE));
			g_my_peerid[PEER_ID_SIZE - 1] = 'V';
			g_is_peerid_valid = TRUE;
			sd_memcpy(buffer, g_my_peerid, MIN(bufsize, PEER_ID_SIZE + 1));
			return SUCCESS;	
		}
		else
#endif /* _ANDROID_LINUX */
		{
			/* fill in default peerid */
			sd_strncpy(g_my_peerid, "XXXXXXXXXXXX", sizeof(g_my_peerid));
			physical_addr_len = 6;
		}
	}
#ifdef CDN_PEERID_SUFFIX
	sd_strncpy(peerid_suffix, "000Z", 5);
	peerid_suffix[4]='\0';
	settings_get_str_item("system.peerid_suffix", peerid_suffix);
	peerid_suffix[4]='\0';
	sd_strncpy(g_my_peerid + 2 * physical_addr_len, peerid_suffix, sizeof(g_my_peerid) - 2 * physical_addr_len);
#else
	sd_strncpy(g_my_peerid + 2 * physical_addr_len, PEERID_SUFFIX, sizeof(g_my_peerid) - 2 * physical_addr_len);
#endif
    settings_get_str_item("system.peerid", g_my_peerid);
	sd_memcpy(buffer, g_my_peerid, MIN(bufsize, PEER_ID_SIZE + 1));
	if(g_is_peerid_valid == TRUE)
		return SUCCESS;
	else
		return ERROR_INVALID_PEER_ID;
}



_int32 set_peerid(const char* buff, _int32 length)
{
	sd_memcpy(g_my_peerid, buff, MIN(length, PEER_ID_SIZE));
	g_my_peerid[PEER_ID_SIZE-1] = 'V';
	g_is_peerid_valid = TRUE;
	return SUCCESS;
}

#else

#include "utility/define.h"
#if defined(LINUX)	
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#if defined(MACOS)
#include <net/if_dl.h>
#include <time.h>
#include <stdlib.h>
#endif

#elif defined(WINCE)
	#include <windows.h>
	#include <IPHlpApi.h>
#endif

#include "platform/sd_device_info.h"

#include "utility/peerid.h"
#include "utility/string.h"
#include "utility/errcode.h"
#include "utility/utility.h"
#include "platform/sd_socket.h"
#include "utility/settings.h"
#include "utility/logid.h"
#define	LOGID	LOGID_COMMON
#include "utility/logger.h"
#include "platform/sd_fs.h"

#if defined(MACOS) && defined(MOBILE_PHONE)

#define PEER_ID_RAND_STRING_LEN 11
static _int32 get_peer_id_rand_string(char *buffer, _int32 bufsize);
static _int32 save_peer_id_rand_string_to_file(char *buffer, _int32 bufsize);
static _int32 load_peer_id_rand_string_from_file(char *buffer, _int32 bufsize);
static _int32 get_save_file_name(char *filename, _int32 size);
#endif

static char g_my_peerid[PEER_ID_SIZE + 1] = {0};
static BOOL	g_is_peerid_valid = FALSE;

 _int32 get_physical_address(char *buffer, _int32 *bufsize)
{
	_int32 ret_val = SUCCESS;

#ifdef MEDIA_CENTER_ANDROID
	_u32 eth0_addr_fi;
	char addr_str[24] = {0};
	int i, j;
	_u8 tmp;
	_u8 eth0_addr_buff[10] = {0};
	_u32 read_size;
	ret_val = sd_open_ex("/sys/class/net/eth0/address", O_FS_RDONLY, &eth0_addr_fi);
	CHECK_VALUE(ret_val);
	ret_val = sd_read(eth0_addr_fi, addr_str, 24, &read_size);
	CHECK_VALUE(ret_val);
	
	if(read_size >= 17)
	{
		for(i = 0, j = 0; i < 24 && addr_str[i];)
		{
			if(addr_str[i] == ':')
			{
				i++;
				continue;
			}
			tmp = 0;
			tmp |= sd_hex_2_int(addr_str[i]);
			tmp <<= 4;
			tmp |= sd_hex_2_int(addr_str[i+1]);
			eth0_addr_buff[j++] = tmp;
			i++;
			i++;
		}
	}

	ret_val = sd_close_ex(eth0_addr_fi);
	CHECK_VALUE(ret_val);

	if (ret_val == SUCCESS) 
	{
		sd_memcpy(buffer, eth0_addr_buff, 6);
		*bufsize = 6;
	}
	
#elif LINUX && !defined MEDIA_CENTER_ANDROID

#ifndef MACOS
	struct ifreq ifr, *pifr;
	struct ifconf ifc;
	_u32 tmp_fd = 0;
	_int32 idx = 0;
	char if_buf[1024];
	char virtual_net[6] = {0x00, 0x50, 0x56, 0xC0, 0x00, 0x01};			//虚拟机生成的网卡
	sd_assert(*bufsize >= 6);

	ret_val = sd_create_socket(SD_AF_INET, SD_SOCK_DGRAM, 0, &tmp_fd);
	CHECK_VALUE(ret_val);

	ifc.ifc_len = sizeof(if_buf);
	ifc.ifc_buf = if_buf;
	ioctl(tmp_fd, SIOCGIFCONF, &ifc);

	ret_val = NOT_FOUND_MAC_ADDR;
	pifr = ifc.ifc_req;
	for(idx = ifc.ifc_len / sizeof(struct ifreq); --idx >= 0; pifr++)
	{
		sd_strncpy(ifr.ifr_name, pifr->ifr_name, IFNAMSIZ);

		if(ioctl(tmp_fd, SIOCGIFFLAGS, &ifr) == 0)
		{
			if(!(ifr.ifr_flags & IFF_LOOPBACK)) 
			{
				if(ioctl(tmp_fd, SIOCGIFHWADDR, &ifr) == 0) 
				{
					sd_memset(buffer, 0, *bufsize);
					if(sd_data_cmp((_u8*)ifr.ifr_hwaddr.sa_data, (_u8*)buffer, 6) == TRUE)		//过滤地址为全0的网卡
						continue;
					if(sd_data_cmp((_u8*)ifr.ifr_hwaddr.sa_data, (_u8*)virtual_net, 6) == TRUE)	//过滤虚拟机生成的网卡
						continue;				
					ret_val = SUCCESS;
					break;
				}
			}

		}
	}

	sd_close_socket(tmp_fd);

	if (ret_val == SUCCESS) 
	{
		sd_memcpy(buffer, ifr.ifr_hwaddr.sa_data, 6);
		*bufsize = 6;
	}
 #else
	struct ifreq ifr, *pifr;
	struct ifconf ifc;
	_u32 tmp_fd = 0;
	_int32 idx = 0;
	char if_buf[1024];
	char virtual_net[6] = {0x00, 0x50, 0x56, 0xC0, 0x00, 0x01};			//虚拟机生成的网卡
	char* cp, *cplim;
	char temp[80];
	
	sd_assert(*bufsize >= 6);

	ret_val = sd_create_socket(SD_AF_INET, SD_SOCK_DGRAM, 0, &tmp_fd);
	CHECK_VALUE(ret_val);

	ifc.ifc_len = sizeof(if_buf);
	ifc.ifc_buf = if_buf;
	ioctl(tmp_fd, SIOCGIFCONF, &ifc);

	ret_val = NOT_FOUND_MAC_ADDR;
	pifr = ifc.ifc_req;
	cp = if_buf;
	cplim = if_buf + ifc.ifc_len;
	
	for(cp = if_buf;cp < cplim; )
	{
		pifr = (struct ifreq *)cp;
		if (pifr->ifr_addr.sa_family == AF_LINK)
		{
			struct sockaddr_dl *sdl = (struct sockaddr_dl *)&pifr->ifr_addr;
			int a,b,c,d,e,f;
			
			sd_memset(temp, 0, sizeof(temp));
			sd_strncpy(temp, (char *)ether_ntoa(LLADDR(sdl)), sizeof(temp));
			sscanf(temp, "%x:%x:%x:%x:%x:%x", &a, &b, &c, &d, &e, &f);
			temp[0]=a;
			temp[1]=b;
			temp[2]=c;
			temp[3]=d;
			temp[4]=e;
			temp[5]=f;
			//sprintf(temp, "%02X%02X%02X%02X%02X%02X",a,b,c,d,e,f);
			if(!(pifr->ifr_flags & IFF_LOOPBACK)) 
			{
				sd_memset(buffer, 0, *bufsize);
#if defined(MACOS)&&defined(MOBILE_PHONE)
				if(sd_data_cmp((_u8*)pifr->ifr_name, "en1", 3) == TRUE)	//ipad下过滤掉网卡eth1地址
				{
					cp += sizeof(pifr->ifr_name) + MAX(sizeof(pifr->ifr_addr), pifr->ifr_addr.sa_len);
					continue;
				}
#endif
				if(sd_data_cmp((_u8*)temp, (_u8*)buffer, 3) == TRUE)		//π??àμ?÷∑???′0μ??ˉ??
				{
					cp += sizeof(pifr->ifr_name) + MAX(sizeof(pifr->ifr_addr), pifr->ifr_addr.sa_len);
					continue;
				}
				if(sd_data_cmp((_u8*)temp, (_u8*)virtual_net, 6) == TRUE)	//π??à–è??a˙…˙≥…μ??ˉ??
				{
					cp += sizeof(pifr->ifr_name) + MAX(sizeof(pifr->ifr_addr), pifr->ifr_addr.sa_len);
					continue;
				}
				ret_val = SUCCESS;
				break;
			}
			
		}
		cp += sizeof(pifr->ifr_name) + MAX(sizeof(pifr->ifr_addr), pifr->ifr_addr.sa_len);

	}

	sd_close_socket(tmp_fd);

	if (ret_val == SUCCESS) 
	{
		sd_memcpy(buffer, (_u8*)temp, 6);
		*bufsize = 6;
	}

 #endif
#elif  defined(WINCE)
	IP_ADAPTER_INFO Data[12];
	ULONG len = sizeof(IP_ADAPTER_INFO) * 12;
	PIP_ADAPTER_INFO pDatas = Data;
	char*   pDesc;
	_int32 i;

	if((GetAdaptersInfo(pDatas, &len)) == ERROR_SUCCESS)
	{
		if( pDatas )
		{
			for(i = 1; i < 12; i++)
			{
				pDesc = pDatas->Description;
				if ( sd_strncmp((const char*)pDesc, "PPP", sd_strlen("PPP")) == 0
					|| sd_strncmp((const char*)pDesc, "VMare", sd_strlen("VMare")) == 0
					|| sd_strncmp((const char*)pDesc, "Virtual", sd_strlen("Virtual")) == 0
					|| sd_strncmp((const char*)pDesc, "SLIP", sd_strlen("SLIP")) == 0
					|| sd_strncmp((const char*)pDesc, "P.P.P", sd_strlen("P.P.P")) == 0
					)
				{
					pDatas = pDatas->Next;
					if(pDatas == NULL)
					{
					    ret_val = -1;
						break;
					}
				}
				else
				{
					sd_memcpy(buffer, pDatas->Address, 6);
					*bufsize = 6;
					ret_val = SUCCESS;
					break;
				}
			}
		}
	}
#endif

	return ret_val;
}

#if 0 //defined(MOBILE_PHONE)&&(!defined(IPAD_KANKAN))

_int32 get_peerid(char *buffer, _int32 bufsize)
{
	const char * p_imei = NULL;
	char physical_addr[10];
	_int32 physical_addr_len = 10;
	_int32 ret_val = SUCCESS;


	// 如果g_my_peerid无效，重新计算peerid
	if(!g_is_peerid_valid)
	{
		sd_memset(g_my_peerid, 0, PEER_ID_SIZE + 1);
		if (bufsize < PEER_ID_SIZE)
		{
			return -1;
		}
		sd_memset(buffer, 0, bufsize);
		p_imei = get_imei();
		if (p_imei != NULL)
		{
			sd_strncpy(g_my_peerid, p_imei, IMEI_SIZE);
			g_my_peerid[PEER_ID_SIZE - 1] = 'V';
			g_is_peerid_valid = TRUE;
		}
		else
		{
			ret_val = get_physical_address(physical_addr, &physical_addr_len);
			if(ret_val == SUCCESS)
			{
				/* convert to peerid */
				ret_val = str2hex(physical_addr, physical_addr_len, g_my_peerid, PEER_ID_SIZE);
				CHECK_VALUE(ret_val);
				sd_assert(sd_strncmp(g_my_peerid, "0000000000", 10)!=0);
				g_is_peerid_valid = TRUE;
			}
			else
			{
				/* fill in default peerid */
				sd_strncpy(g_my_peerid, "XXXXXXXXXXXX", sizeof(g_my_peerid));
				physical_addr_len = 6;
			}
#ifdef CDN_PEERID_SUFFIX
			sd_strncpy(peerid_suffix, "000Z", 5);
			peerid_suffix[4]='\0';
			settings_get_str_item("system.peerid_suffix", peerid_suffix);
			peerid_suffix[4]='\0';
			sd_strncpy(g_my_peerid + 2 * physical_addr_len, peerid_suffix, sizeof(g_my_peerid) - 2 * physical_addr_len);
#else
			sd_strncpy(g_my_peerid + 2 * physical_addr_len, PEERID_SUFFIX, sizeof(g_my_peerid) - 2 * physical_addr_len);
#endif
			sd_memcpy(buffer, g_my_peerid, MIN(bufsize, PEER_ID_SIZE + 1));
			if(g_is_peerid_valid == TRUE)
				return SUCCESS;
			else
				return ERROR_INVALID_PEER_ID;
		}
	}

	LOG_ERROR("get_peerid, g_is_peerid_valid=%d,g_my_peerid=%s", g_is_peerid_valid,g_my_peerid);
	if (g_is_peerid_valid)
	{
		return sd_memcpy(buffer, g_my_peerid, MIN(bufsize, PEER_ID_SIZE + 1));
	}
	else
	{
		// 可能获取 imei 无效，直接返回全0
		return sd_memset(buffer, 0, bufsize);
	}
}

#else

_int32 get_peerid(char *buffer, _int32 bufsize)
{
	_int32 ret_val = SUCCESS;
	char physical_addr[10];
	_int32 physical_addr_len = 10;
	const char * p_imei = NULL;

#ifdef CDN_PEERID_SUFFIX
	char peerid_suffix[5];
#endif
	LOG_ERROR("get_peerid, g_is_peerid_valid=%d,g_my_peerid=%s", g_is_peerid_valid,g_my_peerid);

	//如果g_my_peerid有效，则直接赋值，不用再计算peerid
	if(g_is_peerid_valid == TRUE)
	{
		sd_assert(sd_strncmp(g_my_peerid, "0000000000", 10)!=0);
		return sd_memcpy(buffer, g_my_peerid, MIN(bufsize, PEER_ID_SIZE + 1));
	}
    
	/* 其他平台都用MAC地址作为PEERID */
	ret_val = get_physical_address(physical_addr, &physical_addr_len);
#if defined(MACOS) && defined(MOBILE_PHONE)    
    if (get_ios_system_version() >= 7.0)
    {
        physical_addr_len = 10;
        ret_val = get_peer_id_rand_string(physical_addr, physical_addr_len);
    }
#endif
	if(ret_val == SUCCESS)
	{
		/* convert to peerid */
		ret_val = str2hex(physical_addr, physical_addr_len, g_my_peerid, PEER_ID_SIZE);
		CHECK_VALUE(ret_val);
		sd_assert(sd_strncmp(g_my_peerid, "0000000000", 10)!=0);
		g_is_peerid_valid = TRUE;
	}
	else
	{
		/* 获取MAC 地址失败，尝试获取IMEI号 */
		p_imei = get_imei();
#if defined(_ANDROID_LINUX)		
		if (p_imei != NULL)
		{
			/* 用IMEI(15bytes)号或MEID(14bytes)作为peerid */
			sd_strncpy(g_my_peerid, "XXXXXXXXXXXX004V", PEER_ID_SIZE);
			sd_strncpy(g_my_peerid, p_imei, MIN(sd_strlen(p_imei), PEER_ID_SIZE));
			g_my_peerid[PEER_ID_SIZE - 1] = 'V';
			g_is_peerid_valid = TRUE;
			sd_memcpy(buffer, g_my_peerid, MIN(bufsize, PEER_ID_SIZE + 1));
			return SUCCESS;	
		}
		else
#endif /* _ANDROID_LINUX */
		{
			/* fill in default peerid */
			sd_strncpy(g_my_peerid, "XXXXXXXXXXXX", sizeof(g_my_peerid));
			physical_addr_len = 6;
		}
	}
#ifdef CDN_PEERID_SUFFIX
	sd_strncpy(peerid_suffix, "000Z", 5);
	peerid_suffix[4]='\0';
	settings_get_str_item("system.peerid_suffix", peerid_suffix);
	peerid_suffix[4]='\0';
	sd_strncpy(g_my_peerid + 2 * physical_addr_len, peerid_suffix, sizeof(g_my_peerid) - 2 * physical_addr_len);
#else
	sd_strncpy(g_my_peerid + 2 * physical_addr_len, PEERID_SUFFIX, sizeof(g_my_peerid) - 2 * physical_addr_len);
#endif
	sd_memcpy(buffer, g_my_peerid, MIN(bufsize, PEER_ID_SIZE + 1));
	if(g_is_peerid_valid == TRUE)
		return SUCCESS;
	else
		return ERROR_INVALID_PEER_ID;
}

#endif

BOOL is_set_peerid(void)
{
    return g_is_peerid_valid;
}

void uninit_peerid(void)
{
    g_is_peerid_valid = FALSE;
    sd_memset(g_my_peerid, 0, sizeof(g_my_peerid));

    return;
}

_int32 set_peerid(const char* buff, _int32 length)
{
    
#ifdef CDN_PEERID_SUFFIX
	char peerid_suffix[5];
#endif

    if (NULL == buff || length <= 0)
    {
        CHECK_VALUE(INVALID_ARGUMENT);
    }

    sd_memset(g_my_peerid, 0, sizeof(g_my_peerid));
	sd_memcpy(g_my_peerid, buff, MIN(length, PEER_ID_SIZE));
	
#ifdef CDN_PEERID_SUFFIX
	sd_strncpy(peerid_suffix, "000Z", 5);
	peerid_suffix[4]='\0';
	settings_get_str_item("system.peerid_suffix", peerid_suffix);
	peerid_suffix[4]='\0';
	sd_strncpy(g_my_peerid + length, peerid_suffix, sizeof(g_my_peerid) - length);
#else
	sd_strncpy(g_my_peerid + length, PEERID_SUFFIX, sizeof(g_my_peerid) - length);
#endif

	g_my_peerid[PEER_ID_SIZE-1] = 'V';
	g_is_peerid_valid = TRUE;
	
	return SUCCESS;
}

#if defined(MACOS) && defined(MOBILE_PHONE)

static _int32 get_peer_id_rand_string(char *buffer, _int32 bufsize)
{
    _int32 ret = -1;
    if ((NULL == buffer) || (0 == bufsize))
    {
        return ret;
    }
    static char rand_string[PEER_ID_RAND_STRING_LEN] = {0};
    if (0 == sd_strlen(rand_string))
    {
        ret = load_peer_id_rand_string_from_file(rand_string, PEER_ID_RAND_STRING_LEN - 1);
        if (0 == sd_strlen(rand_string))
        {
            srand((unsigned)time(NULL));
            for (int i = 0; i < (PEER_ID_RAND_STRING_LEN - 1); ++i)
            {
                rand_string[i] = (rand() % 10) + '0';
            }
            ret = save_peer_id_rand_string_to_file(rand_string, PEER_ID_RAND_STRING_LEN - 1);
        }
    }
    sd_strncpy(buffer, rand_string, bufsize);
    return SUCCESS;
}

static _int32 save_peer_id_rand_string_to_file(char *buffer, _int32 bufsize)
{
    if ((NULL == buffer) || (NULL == bufsize))
    {
        return -1;
    }
    char filename[SD_MAX_PATH] = {0};
    _int32 ret = get_save_file_name(filename, SD_MAX_PATH);
    if (SUCCESS != ret)
    {
        return ret;
    }
    _u32 file_id = 0;
    ret = sd_open_ex(filename, O_FS_RDWR|O_FS_CREATE, &file_id);
    if (SUCCESS != ret)
    {
        return ret;
    }
    _u32 write_size = 0;
    ret = sd_write(file_id, buffer, bufsize, &write_size);
    sd_close_ex(file_id);
    return ret;
}

static _int32 load_peer_id_rand_string_from_file(char *buffer, _int32 bufsize)
{
    if ((NULL == buffer) || (NULL == bufsize))
    {
        return -1;
    }
    char filename[SD_MAX_PATH] = {0};
    _int32 ret = get_save_file_name(filename, SD_MAX_PATH);
    if (SUCCESS != ret)
    {
        return ret;
    }
    _u32 file_id = 0;
    ret = sd_open_ex(filename, O_FS_RDONLY, &file_id);
    if (SUCCESS != ret)
    {
        return ret;
    }
    _u32 read_size = 0;
    ret = sd_read(file_id, buffer, bufsize, &read_size);
    sd_close_ex(file_id);
    return ret;
}

static _int32 get_save_file_name(char *filename, _int32 size)
{
    if (NULL == filename)
    {
        return -1;
    }
    sd_snprintf(filename, size, "%s%s", get_app_home_path(), "/Library/peerid.txt");
    
    return SUCCESS;
}

#endif
#endif
