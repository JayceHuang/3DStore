/*
 ============================================================================
 Name		: sd_ios_time_wrapper.h
 Author		: dragon,(Hotel), xiaohaotian@xunlei.com
 Version	 : 1.0
 Copyright   : Thunder Networking Inc
 ============================================================================
 */

#ifndef SD_IOS_TIME_WRAPPER_H_20100916_1819
#define SD_IOS_TIME_WRAPPER_H_20100916_1819

#if defined(MACOS) && defined(MOBILE_PHONE)

#include "utility/define.h"
#include "utility/errcode.h"

#ifdef __cplusplus
extern "C" 
{
#endif

_int32 sd_ios_time_time(_u32 *times);
_int32 sd_ios_time_time_ms(_u32 *time_ms);
_int32 sd_ios_time_get_current_time_info(_int64 *time_stamp, _u32 *year, _u32 *mon, _u32 *day, _u32 *hour, _u32 *min, _u32 *sec, _u32 *msec,_u32 *day_year, _u32 *day_week, _u32 *week_year);
_int32 sd_ios_time_get_time_info(_u32 times, _int64 *time_stamp,_u32 *year, _u32 *mon, _u32 *day, _u32 *hour, _u32 *min, _u32 *sec, _u32 *msec, _u32 *day_year, _u32 *day_week, _u32 *week_year);


#ifdef __cplusplus
}
#endif

#endif
#endif
