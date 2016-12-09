#ifndef LOGGER_H_
#define LOGGER_H_

#if defined(_DEBUG) && defined(_ANDROID_LINUX)

#include <android/log.h>
//#define TAG "XLMediaServer"
#ifdef VERBOSE_ENABLED
#define VERBOSE(...)  __android_log_print(ANDROID_LOG_VERBOSE, __VA_ARGS__)
#else
#define VERBOSE(...)
#endif
#define DEBUG(...)    __android_log_print(ANDROID_LOG_DEBUG, __VA_ARGS__)
#define INFO(...)     __android_log_print(ANDROID_LOG_INFO, __VA_ARGS__)
#define WARN(...)     __android_log_print(ANDROID_LOG_WARN, __VA_ARGS__)
#define ERROR(...)    __android_log_print(ANDROID_LOG_ERROR, __VA_ARGS__)
#define ASSERT(x) \
  if (!(x)) \
  { \
    __android_log_assert("assert", "assert", "Assertion failed: (%s), function %s, file %s, line %d.", #x, __FUNCTION__, __FILE__, __LINE__);  \
  }

#else

#define VERBOSE(...)
#define DEBUG(...)
#define INFO(...)
#define WARN(...)
#define ERROR(...)
#define ASSERT(x)

#endif

#endif /* LOGGER_H_ */
