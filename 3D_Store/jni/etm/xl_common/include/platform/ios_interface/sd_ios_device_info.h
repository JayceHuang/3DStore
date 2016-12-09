/*
 ============================================================================
 Name		: sd_ios_device_info.h
 Author	  : zyq
 Version	 : 1.0
 Copyright   : Your copyright notice
 Description : 
 ============================================================================
 */

#ifndef SD_IOS_DEVICE_INFO_H_20101026
#define SD_IOS_DEVICE_INFO_H_20101026

#include "utility/define.h"
#if defined(MACOS) && defined(MOBILE_PHONE)

// ios handset type. such as iPhone 2, iPod Touch 3, iPhone3GS, iPhone4GS and so on.
typedef enum{
    IOS_HANDSET_UNKNOWN=0,
    IPHONE_SIMULATOR,
    IPAD_2G,
	IPAD_3G,
    IPHONE_1G,
    IPOD_TOUCH_1G,
    IPOD_TOUCH_2G,
    IPHONE_3G,
    IPOD_TOUCH_3G,
    IPHONE_3GS,
    IPHONE_4G,
    IPOD_TOUCH_4G,
    IPHONE_4GS
}IPHONE_HARDWARE_VERSION;

#ifdef __cplusplus
extern "C" 
{
#endif

/*******************************************************************/

#include "utility/errcode.h"

/*******************************************************************/

const char * get_ios_system_info(void);
float get_ios_system_version(void);
_int32 get_ios_screen_size(_int32 *x,_int32 *y);
IPHONE_HARDWARE_VERSION get_ios_hardware_type(void);
    
// 0: active; 1: inactive; 2: background
_int32 get_current_application_status(void);
    
// 获取设备IMEI号
const char * get_ios_imei(void);
_int32 check_network_connect(void);

const char * get_ios_software_version(void);
const char * get_ios_hardware_version(void);
const char * get_ios_name(void);
const char * get_app_home_path(void);
/*******************************************************************/

#ifdef __cplusplus
}
#endif

#endif
#endif
