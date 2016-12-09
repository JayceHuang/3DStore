#if defined(_ANDROID_LINUX)

// ------------------------------------

#include "platform/android_interface/sd_android_utility.h"
#include "utility/errcode.h"
/*
static JavaParm g_java_parm;
static BOOL g_java_parm_is_init = FALSE;

_int32 sd_android_utility_init(JavaVM * vm, jobject thiz,jobject  de)
{
	g_java_parm._vm = vm;
	g_java_parm._thiz = thiz;
	g_java_parm._downloadengine = de;
	g_java_parm_is_init = TRUE;
	return SUCCESS;
}

_int32 sd_android_utility_uninit()
{
	g_java_parm_is_init = FALSE;
	g_java_parm._thiz = NULL;
	g_java_parm._downloadengine = NULL;
	return SUCCESS;
}

BOOL sd_android_utility_is_init()
{
	return g_java_parm_is_init;
}

JavaParm *sd_android_utility_get_java()
{
	return &g_java_parm;
}
*/
static JavaParm g_java_parm;
static AndroidDeviceInfo g_device_info;
static BOOL g_devide_info_is_init = FALSE;
static BOOL g_is_xiaomi = FALSE;
_int32 sd_android_utility_init(JavaVM * vm, jobject  de, AndroidDeviceInfo* deviceinfo)
{
	if(deviceinfo == NULL)
		return INVALID_ARGUMENT;
	sd_strncpy(g_device_info._system_info, deviceinfo->_system_info, MAX_OS_LEN - 1);
	sd_strncpy(g_device_info._system_imei, deviceinfo->_system_imei, MAX_OS_LEN - 1);
	g_device_info._screenwidth = deviceinfo->_screenwidth;
	g_device_info._screenheight = deviceinfo->_screenheight;

	g_java_parm._vm = vm;
	g_java_parm._downloadengine = de;

	if(sd_strstr(g_device_info._system_info,"Xiaomi",0)!=NULL)
	{
		g_is_xiaomi = TRUE;
	}
	
	g_devide_info_is_init = TRUE;
	return SUCCESS;
}

_int32 sd_android_utility_uninit()
{
	sd_memset(&g_device_info, 0x00, sizeof(g_device_info));
	g_devide_info_is_init = FALSE;

	g_java_parm._downloadengine = NULL;
	return SUCCESS;
}

BOOL sd_android_utility_is_init()
{
	return g_devide_info_is_init;
}

AndroidDeviceInfo *sd_android_utility_get_device_info()
{
	return &g_device_info;
}

JavaParm *sd_android_utility_get_java()
{
	return &g_java_parm;
}

BOOL sd_android_utility_is_xiaomi()
{
	return g_is_xiaomi;
}

// ------------------------------------

#endif

