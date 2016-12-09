#ifndef SD_LOGGER_H_00138F8F2E70_200806171745
#define SD_LOGGER_H_00138F8F2E70_200806171745

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#include "utility/errcode.h"
#include "utility/arg.h"
#include "utility/logid.h"

_int32 logger_printf(const char *fmt, ...);
_int32 running_logger(const char *fmt, ...);
void close_running_log();

#ifdef _RELEASE_LOG
#define LOG_RUNNING running_logger
#else
#define LOG_RUNNING(...)
#endif



#if (defined(_LOGGER)&& !defined(MACOS))||(defined(_LOGGER)&&defined(MACOS)&&defined(MOBILE_PHONE) )//zion 2013-01-11
#include "utility/slog.h"
#endif

_int32 set_urgent_file_path(const char * path);
_int32  write_urgent_to_file(const char *fmt, ...);
_int32 print_strace_to_file(void);

#define URGENT_TO_FILE	write_urgent_to_file
#define PRINT_STRACE_TO_FILE	{print_strace_to_file();}

#ifdef _LOGGER

#ifndef LOGID
#error Compile error, must define LOGID
#endif

_int32 logger(_int32 logid, const char *fmt, sd_va_list ap);

_int32 static logger_wrap(const char *fmt, ...)
{
	_int32 ret_val = SUCCESS;

	sd_va_list ap;
	sd_va_start(ap, fmt);
	ret_val = logger(LOGID, fmt, ap);
	sd_va_end(ap);

	return ret_val;
}


#if defined(_LOGGER)&& !defined(MACOS)
#define LOG_DEBUG	if(current_loglv(LOGID) >=  LOG_DEBUG_LV) SLOG_DEBUG
#define LOG_ERROR	if(current_loglv(LOGID) >=  LOG_ERROR_LV) SLOG_ERROR
#define LOG_URGENT	SLOG_ERROR
#elif defined(_LOGGER)&&defined(MACOS)&&defined(MOBILE_PHONE)   //ios   //zion-201301-11
#define LOG_DEBUG	if(current_loglv(LOGID) >=  LOG_DEBUG_LV) SLOG_DEBUG
#define LOG_ERROR	if(current_loglv(LOGID) >=  LOG_ERROR_LV) SLOG_ERROR
#define LOG_URGENT	SLOG_DEBUG

#else
#define LOG_DEBUG	if(current_loglv(LOGID) >=  LOG_DEBUG_LV) logger_wrap
#define LOG_ERROR	if(current_loglv(LOGID) >=  LOG_ERROR_LV) logger_wrap
#define LOG_URGENT	logger_wrap

#endif

#else

#define LOG_DEBUG(...)
#define LOG_ERROR(...)
#ifdef ENABLE_ETM_MEDIA_CENTER
#define LOG_URGENT(...)
#else
#define LOG_URGENT URGENT_TO_FILE
#endif
#ifdef _ANDROID_LINUX
#define ANDROID_LOG(...)
#endif

#endif


#ifdef __cplusplus
}
#endif

#endif
