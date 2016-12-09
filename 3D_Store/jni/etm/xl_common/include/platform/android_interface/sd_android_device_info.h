/*******************************************************
 * 
 * Android 系统信息相关函数。
 *
 *******************************************************/

#ifndef __SD_ANDROID_DEVICE_INFO_20101008_h__
#define __SD_ANDROID_DEVICE_INFO_20101008_h__

#ifdef __cplusplus
extern "C" 
{
#endif

/*******************************************************************/

#include "utility/errcode.h"

/*******************************************************************/

const char * get_android_system_info(void);
_int32 get_android_screen_size(_int32 *x,_int32 *y);

// 获取设备IMEI号
const char * get_android_imei(void);

/*******************************************************************/

#ifdef __cplusplus
}
#endif

#endif

