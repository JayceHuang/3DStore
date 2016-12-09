#include "utility/sd_os.h"
#include "utility/errcode.h"
#include "utility/logid.h"
#include "utility/mempool.h"
#include "utility/queue.h"
#include "utility/list.h"
#include "platform/sd_network.h"
#include "utility/map.h"
#include "utility/string.h"
#include "utility/crosslink.h"

#ifdef _LOGGER
#include "utility/slog.h"
#endif

#define S_LOG_CONFIG_FILE LOG_CONFIG_FILE

static BOOL s_is_initialized = FALSE;
static _int32 g_os_critical_error = 0;

static _int32 g_is_device_lock_screen = 0;
static _int32 g_is_device_in_charge = 0;
static _int32 g_device_now_charge = 0;

#if (defined(MACOS)&&defined(MOBILE_PHONE))
_int32 et_os_init(const char * log_conf_file,const char* etm_system_path,_u32 path_len)
{
	_int32 ret_val = SUCCESS;
	
	if(s_is_initialized) return SUCCESS;
#ifdef MACOS
	/* MAC 系统中去掉文件权限掩码 */
	umask(002);
#endif
	// logger
	ret_val = sd_log_init(log_conf_file);
	if (ret_val != SUCCESS)
	{
		return ret_val;
	}
    
#if defined(_LOGGER)&& !defined(MACOS)||(defined(MACOS)&&defined(MOBILE_PHONE))
    char default_log_file[MAX_FULL_PATH_BUFFER_LEN];
    sd_memset(default_log_file,0x00,MAX_FULL_PATH_BUFFER_LEN);
    if((etm_system_path!=NULL)&&(path_len!=0))
    {
        sd_strncpy(default_log_file,etm_system_path,path_len);
        if(default_log_file[path_len-1]!='/') sd_strcat(default_log_file,"/",1);
    }
    sd_strcat(default_log_file,S_LOG_CONFIG_FILE,sd_strlen(S_LOG_CONFIG_FILE));
#if defined(_LOGGER)	
#if defined(MACOS)&& defined(MOBILE_PHONE)
    ret_val = slog_init(default_log_file, etm_system_path, path_len);
#else
    ret_val = slog_init(default_log_file);
#endif
#endif
    
	//ret_val = SLOG_INIT(S_LOG_CONFIG_FILE);
#endif
	
	ret_val = default_mpool_init();
	if (ret_val != SUCCESS)
	{
		return ret_val;
	}
	ret_val = queue_alloctor_init();
	CHECK_VALUE(ret_val);
    
	ret_val = list_alloctor_init();
	CHECK_VALUE(ret_val);
    
	ret_val = set_alloctor_init();
	CHECK_VALUE(ret_val);
    
	ret_val = map_alloctor_init();
	CHECK_VALUE(ret_val);
    
	ret_val = crosslink_alloctor_init();
	CHECK_VALUE(ret_val);
    
#ifdef MACOS
	ret_val = init_event_module();
	CHECK_VALUE(ret_val);
#endif
	s_is_initialized = TRUE;
    
#if defined(_ANDROID_LINUX)
	sd_uninit_network();
#endif /* _ANDROID_LINUX */
    
	set_urgent_file_path(log_conf_file);
    
	return SUCCESS;
}

#else


_int32 et_os_init(const char * log_conf_file)
{
	_int32 ret_val = SUCCESS;
	
	if(s_is_initialized) return SUCCESS;
#ifdef MACOS
	/* MAC 系统中去掉文件权限掩码 */
	umask(002);
#endif
	// logger
	ret_val = sd_log_init(log_conf_file);
	if (ret_val != SUCCESS)
	{
		return ret_val;
	}

#if defined(_LOGGER)&& !defined(MACOS)
	ret_val = slog_init(log_conf_file);
#endif
    
#if defined(_LOGGER) && defined(MACOS)&&defined(MOBILE_PHONE)
    ret_val = slog_init(log_conf_file, NULL, 0);
#endif
	
	ret_val = default_mpool_init();
	if (ret_val != SUCCESS)
	{
		return ret_val;
	}
	ret_val = queue_alloctor_init();
	CHECK_VALUE(ret_val);

	ret_val = list_alloctor_init();
	CHECK_VALUE(ret_val);

	ret_val = set_alloctor_init();
	CHECK_VALUE(ret_val);

	ret_val = map_alloctor_init();
	CHECK_VALUE(ret_val);

#ifdef MACOS
	ret_val = init_event_module();
	CHECK_VALUE(ret_val);
#endif
	s_is_initialized = TRUE;

#if defined(_ANDROID_LINUX)
	sd_uninit_network();
#endif /* _ANDROID_LINUX */

	set_urgent_file_path(log_conf_file);

	return SUCCESS;
}
#endif

BOOL et_os_is_initialized()
	{
	return s_is_initialized;
	}

_int32 et_os_uninit(void)
	{
	//_int32 ret = SUCCESS,ret_val= SUCCESS;
	if(!s_is_initialized) return SUCCESS;

#ifdef MACOS
	uninit_event_module();
#endif
	map_alloctor_uninit();
	set_alloctor_uninit();
	list_alloctor_uninit();
	queue_alloctor_uninit(); 
	default_mpool_uninit();


	/* network
	ret = sd_uninit_network();
	if (ret != SUCCESS)
		{
		return ret;
		}
*/
	// logger
#if defined(_LOGGER)&& !defined(MACOS)
	slog_finalize();
#endif
	sd_log_uninit();
	
	s_is_initialized = FALSE;
	return SUCCESS;
	}

_int32 et_os_set_critical_error(_int32 err_code)
{
	g_os_critical_error = err_code;
	return SUCCESS;
}
_int32 et_os_get_critical_error(void)
{
	return g_os_critical_error;
}

_int32 sd_set_now_charge(_int32 now_charge)
{
    _int32 ret = SUCCESS;
    g_device_now_charge = now_charge;
    return ret;
}

_int32 sd_set_charge_status(_int32 charge_status)
{

    _int32 ret = SUCCESS;
    g_is_device_in_charge = charge_status;
    return ret;
}

_int32 sd_set_lock_status(_int32 lock_status)
{

    _int32 ret = SUCCESS;
    g_is_device_lock_screen = lock_status;
    return ret;
}

_int32 sd_get_now_charge()
{
    return g_device_now_charge;
}

_int32 sd_get_charge_status()
{
    return g_is_device_in_charge;
}

_int32 sd_get_lock_status()
{
    return g_is_device_lock_screen;
}



