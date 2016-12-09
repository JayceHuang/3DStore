/*
 ============================================================================
 Name		: sd_symbian_os.h
 Author	  : liujian
 Version	 : 1.0
 Copyright   : Your copyright notice
 Description : 
 ============================================================================
 */

#ifndef SD_WM_OS_H_20100130_1648
#define SD_WM_OS_H_20100130_1648

#include "utility/define.h"
#if defined(WINCE)

#ifdef __cplusplus
extern "C" 
{
#endif

/*******************************************************************/

#include "utility/errcode.h"

/*******************************************************************/

const char * get_wm_system_info(void);
_int32 get_wm_screen_size(_int32 *x,_int32 *y);

// 获取设备IMEI号
const char * get_wm_imei(void);

/*******************************************************************/

#ifdef __cplusplus
}
#endif

#endif
#endif
