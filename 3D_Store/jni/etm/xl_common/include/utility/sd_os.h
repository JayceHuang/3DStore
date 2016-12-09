#ifndef SD_OS_H_20100827
#define SD_OS_H_20100827
#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"
#include "utility/errcode.h"

#if (defined(MACOS)&&defined(MOBILE_PHONE))
_int32 et_os_init(const char * log_conf_file,const char* etm_system_path,_u32 path_len);
#else
_int32 et_os_init(const char * log_conf_file);
#endif
    
 BOOL et_os_is_initialized();
_int32 et_os_uninit(void);

_int32 et_os_set_critical_error(_int32 err_code);
_int32 et_os_get_critical_error(void);

_int32 sd_set_now_charge(_int32 now_charge);
_int32 sd_set_charge_status(_int32 charge_status);
_int32 sd_set_lock_status(_int32 lock_status);

_int32 sd_get_now_charge();
_int32 sd_get_charge_status();
_int32 sd_get_lock_status();

_int32 set_urgent_file_path(const char * path);

#define CHECK_OS_CRITICAL_ERROR  {CHECK_VALUE(et_os_get_critical_error());}

#ifdef __cplusplus
}
#endif
#endif /* SD_OS_H_20100827 */
