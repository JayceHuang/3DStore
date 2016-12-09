#include "platform/sd_device_info.h"
#include "utility/string.h"

#if defined(WINCE)
#include "platform/wm_interface/sd_wm_device_info.h"
#endif

static char g_imei[IMEI_LEN + 1];
static BOOL g_imei_ready = FALSE;

const char * get_system_info(void)
{
#if defined(WINCE)
    return get_wm_system_info();
#elif defined(MACOS)&&defined(MOBILE_PHONE)
    return get_ios_system_info();
#elif defined(_ANDROID_LINUX)
    return get_android_system_info();
#endif
	return NULL;
}

const char * get_imei(void)
{
	if (g_imei_ready)
	{
		return g_imei;
	}
	else
	{
		const char * p_imei = NULL;
#if defined(WINCE)
		p_imei = get_wm_imei();
#elif defined(_ANDROID_LINUX)
		p_imei = get_android_imei();
#elif defined(MACOS)&&defined(MOBILE_PHONE)
		p_imei = get_ios_imei();
#endif
		if (p_imei != NULL)
		{
			set_imei(p_imei, sd_strlen(p_imei));
		}
		return p_imei;
	}
}

void set_imei(const char * p_imei, _u32 len)
{
	sd_memset(g_imei, 0, IMEI_LEN + 1);
	sd_strncpy(g_imei, p_imei, len);
	g_imei_ready = TRUE;
}

