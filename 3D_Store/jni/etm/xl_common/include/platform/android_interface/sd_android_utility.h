#ifndef _SD_ANDROID_UTILITY_H
#define _SD_ANDROID_UTILITY_H

#if defined(_ANDROID_LINUX)

#ifdef __cplusplus
extern "C" {
#endif

// ------------------------------------

#include "utility/define.h"
#include <jni.h>
/*
typedef struct tag_JavaParm
{
	JavaVM *_vm;
	jobject _thiz;
	jobject _downloadengine;
} JavaParm;

_int32 sd_android_utility_init(JavaVM * vm, jobject thiz,jobject  de);

_int32 sd_android_utility_uninit();

BOOL sd_android_utility_is_init();

JavaParm *sd_android_utility_get_java();
*/

typedef struct tag_JavaParm
{
	JavaVM *_vm;
	jobject _downloadengine;
} JavaParm;

typedef struct tag_android_device_info_param
{
	char _system_info[MAX_OS_LEN];
	char _system_imei[IMEI_LEN];
	_int32 _screenwidth;
	_int32 _screenheight;
} AndroidDeviceInfo;

_int32 sd_android_utility_init(JavaVM * vm, jobject  de, AndroidDeviceInfo* deviceinfo);

_int32 sd_android_utility_uninit();

BOOL sd_android_utility_is_init();

AndroidDeviceInfo *sd_android_utility_get_device_info();

JavaParm *sd_android_utility_get_java();

// ------------------------------------

#ifdef __cplusplus
}
#endif

#endif
#endif

