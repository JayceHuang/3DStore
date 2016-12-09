#ifndef __SD_ANDROID_LOG_H__
#define __SD_ANDROID_LOG_H__

#ifdef _LOGGER_ANDROID
#include <android/log.h>
#define ANDROID_LOG __android_log_print
#else
//开发期间打开该开关
//#include <android/log.h>
//#define ANDROID_LOG __android_log_print
#define ANDROID_LOG_DEBUG 0
#define ANDROID_LOG_INFO 0
#define ANDROID_LOG_WARN 0
#define ANDROID_LOG_ERROR 0

#define ANDROID_LOG(...)
#endif

#endif

