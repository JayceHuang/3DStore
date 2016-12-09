#include "utility/version.h"
#include "utility/errcode.h"
#include "utility/string.h"
#include "utility/peerid.h"

#if defined(LINUX)
#include <sys/utsname.h>
#elif  defined( WINCE)
#include "platform/wm_interface/sd_wm_network_interface.h"
#include "platform/wm_interface/sd_wm_device_info.h"
#elif defined(_ANDROID_LINUX)
#include "platform/android_interface/sd_android_network_interface.h"
#endif

#include "platform/sd_device_info.h"

#include "utility/logid.h"
#ifdef LOGID
#undef LOGID
#endif

#define LOGID LOGID_COMMON

#include "utility/logger.h"

static char g_et_version[MAX_VERSION_LEN] = {0};
static _int32 g_partner_id = 0;

_int32 get_product_flag(void)
{

/* product  is  relation to  the last character , 0x10000000 is  V*/
#if defined(MOBILE_PHONE)
      return(0x10000000);
#else
	#ifdef MACOS
      		return (0x1000000);    // MAC 迅雷
	#else
      return (0x2000);     
#endif
#endif

}

_u32 get_product_id(void)
{
	return 0x52FFF;		//必须填该值，否则某些cdn可能连不上
}
_int32 set_partner_id(_int32 partner_id)
{
	g_partner_id = partner_id;
	return SUCCESS;
}
_int32 get_partner_id(char *buffer, _int32 bufsize)
{
#if defined(MOBILE_PHONE)
	sd_snprintf(buffer, bufsize-1, "%d",g_partner_id);
#else
	sd_strncpy(buffer, PARTNER_ID, bufsize);
#endif
	return SUCCESS;
}

/*
int uname(struct utsname *buf);


struct utsname {
    char sysname[];
    char nodename[];
    char release[];
    char version[];
    char machine[];
    #ifdef _GNU_SOURCE
    char domainname[];
    #endif
};
*/

_int32 sd_get_os(char *buffer, _int32 bufsize)
{
	_int32 ret_val = SUCCESS;
#if defined(LINUX)
	struct utsname os_info;
#endif

	if((buffer==NULL)||(bufsize<MAX_OS_LEN)) return -1;
	sd_memset(buffer, 0, MAX_OS_LEN);

#if defined(LINUX)
	#if defined(_ANDROID_LINUX)
	sd_memcpy(buffer, (const void*)get_android_system_info(), MAX_OS_LEN);
	#elif  defined(MACOS)&&defined(MOBILE_PHONE)
	sd_strncpy(buffer, get_ios_software_version(), sd_strlen(get_ios_software_version()));
	#else
	ret_val = uname(&os_info);
/*
{sysname = "Linux", '\0' <repeats 59 times>, nodename = "EmbedThunder", '\0' <repeats 52 times>, 
  release = "2.6.18-53.el5", '\0' <repeats 51 times>, version = "#1 SMP Wed Oct 10 16:34:02 EDT 2007", '\0' <repeats 29 times>, 
  machine = "i686", '\0' <repeats 60 times>, __domainname = "(none)", '\0' <repeats 58 times>}
*/
	if(ret_val == SUCCESS)
	{
		sd_snprintf(buffer,MAX_OS_LEN-1,"%s-%s",os_info.sysname,os_info.release);
	}
	#endif
#elif defined(WINCE)
 	sd_memcpy(buffer, get_system_info(), MAX_OS_LEN);   
#else
	sd_memcpy(buffer, "UNKNOWN", sizeof("UNKNOWN"));
#endif
	LOG_DEBUG( "sd_get_os:%s",buffer );
	return ret_val;
}
_int32 sd_get_device_name(char *buffer, _int32 bufsize)
{
	char *name = NULL;
#if defined(MACOS)&&defined(MOBILE_PHONE)
	name = get_ios_name();
#elif defined( _ANDROID_LINUX)
	name = (char*)get_android_system_info();
#elif defined( LINUX)
	{
		_int32 ret_val = SUCCESS;
		struct utsname os_info;
		ret_val = uname(&os_info);
		if(ret_val==SUCCESS)
		{
			sd_snprintf(buffer,bufsize-1,"%s",os_info.nodename);
			return SUCCESS;
		}
		else
		{
			name = "unknown_device";
		}
	}
#else
	name = "unknown_device";
#endif
	sd_strncpy(buffer, name, bufsize-1);
	return SUCCESS;
}
_int32 sd_get_hardware_version(char *buffer, _int32 bufsize)
{
	const char * hv = NULL;
#if defined(MACOS)&&defined(MOBILE_PHONE)
	hv = get_ios_hardware_version();
#elif defined( LINUX)
	{
		_int32 ret_val = SUCCESS;
		struct utsname os_info;
		ret_val = uname(&os_info);
		if(ret_val==SUCCESS)
		{
			sd_snprintf(buffer,bufsize-1,"%s",os_info.machine);
			return SUCCESS;
		}
		else
		{
			hv = "unknown_hardware";
		}
	}
#else
	hv = "unknown_hardware";
#endif
	sd_strncpy(buffer, hv, bufsize-1);
	return SUCCESS;
}

_int32 sd_get_screen_size(_int32 *x, _int32 *y)
{
	_int32 ret_val = SUCCESS;

#if  defined(WINCE)
 	ret_val = get_wm_screen_size(x,y);     
#elif defined( _ANDROID_LINUX)
 	ret_val = get_android_screen_size(x,y);   
#elif defined(MACOS)&&defined(MOBILE_PHONE)
 	ret_val = get_ios_screen_size(x,y);   
#else
	if(x) *x = 240;
	if(y) *y = 320;
#endif
    if(SUCCESS!=ret_val)
    {
		if(x) *x = 480;
		if(y) *y = 800;
		ret_val = SUCCESS;
    }
	LOG_DEBUG( "sd_get_screen_size:x=%d,y=%d",*x,*y );
	return ret_val;
}

#if defined(MACOS)&&defined(MOBILE_PHONE)
BOOL sd_is_ipad3(void)
{
	_int32 ret_val = SUCCESS;
	struct utsname os_info;
	//return FALSE;
	ret_val = uname(&os_info);
	if(ret_val == SUCCESS)
	{
		if(sd_stricmp(os_info.sysname, "Darwin")==0 && sd_strncmp(os_info.release, "10.3",4)==0)
		{
			return TRUE;
		}
	}
	return FALSE;
}
#endif

_u32 sd_get_version_num()
{
	char buff[32] = {0};
	_u32 result = 0;
	_int32 i = 0, si = 0, j = 0;
	_u32 sec[4] = {0};

	static _u32 version_num = 0;

	if(version_num != 0) return version_num;
	sd_get_version_new(buff, sizeof(buff));

	while(i < sizeof(buff) && buff[i])
	{
		if(buff[i] == '.')
		{
			si++;
			i++;
			continue;
		}
		sec[si] *= 10;
		sec[si] += buff[i++] - '0';
	}

	// main version
	result = sec[0];
	// company no. --  3-digit
	result *= 1000;
	result += sec[1];
	// debug/release flag -- 1-digit
	result *= 10;
	result += sec[2];
	// build num -- 3-digit
	result *= 1000;
	result += sec[3];
	
	version_num = result;
	return result;
}

_int32 sd_get_version_new(char *buff, _u32 buflen)
{
#ifdef ENABLE_ETM_MEDIA_CENTER
	return sd_strncpy(buff, VERSION(VERSION_NUM, _COMPANY_NUM, DEBUG_RELEASE_FLAG, BUILD_NUM), buflen);
#else
    sd_assert(0);
    return 0;
#endif
}

