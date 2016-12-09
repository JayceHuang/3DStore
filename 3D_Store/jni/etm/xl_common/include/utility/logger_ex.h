#ifndef SD_LOGGER_EX_H_00138F8F2E70_201112131910
#define SD_LOGGER_EX_H_00138F8F2E70_201112131910
#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#include "utility/errcode.h"
#include "utility/arg.h"
#include "utility/logid.h"

#ifdef _LOGGER

_int32 logger(_int32 logid, const char *fmt, sd_va_list ap);

inline _int32 static logger_wrap_ex(_int32 logid, const char *fmt, ...)
{
	_int32 ret_val = SUCCESS;

	sd_va_list ap;
	sd_va_start(ap, fmt);
	ret_val = logger(logid, fmt, ap);
	sd_va_end(ap);

	return ret_val;
}


#define LOG_DEBUG_EX  logger_wrap_ex
#define LOG_ERROR_EX  logger_wrap_ex
#define LOG_URGENT_EX	logger_wrap_ex


#else

#define LOG_DEBUG_EX(...)
#define LOG_ERROR_EX(...)
#define LOG_URGENT_EX(...)
#ifdef _ANDROID_LINUX
#define ANDROID_LOG_EX(...)
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif
