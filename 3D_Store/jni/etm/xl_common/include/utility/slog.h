#ifndef _LIB_SAMPLE_LOG_H_
#define _LIB_SAMPLE_LOG_H_

#ifdef __cplusplus
extern "C"{
#endif

#ifdef _LOGGER

#if defined(MACOS)&&defined(MOBILE_PHONE)
int slog_init(const char* config,const char* etm_system_path,unsigned int path_len);
#else
int slog_init(const char* config);
#endif
void slog_finalize();

#ifdef __SLOG_FULL__
#define PRE_STR "[%s:%s(%d)] ... "
#define VAL_STR ,__FILE__,__FUNCTION__,__LINE__
#else
#define PRE_STR 
#define VAL_STR 
#endif

#define SLOG_TRACE(format, ...) slog_trace(__FILE_NAME__, __LINE__, __FUNCTION__, LOGID, PRE_STR format VAL_STR ,##__VA_ARGS__)
#define SLOG_DEBUG(format, ...) slog_debug(__FILE__, __LINE__, __FUNCTION__, LOGID, PRE_STR format VAL_STR ,##__VA_ARGS__)
#define SLOG_INFO(format, ...) slog_info(__FILE__, __LINE__, __FUNCTION__, LOGID, PRE_STR format VAL_STR ,##__VA_ARGS__)
#define SLOG_WARN(format, ...) slog_warn(__FILE__, __LINE__, __FUNCTION__, LOGID, PRE_STR format VAL_STR ,##__VA_ARGS__)
#define SLOG_ERROR(format, ...) slog_error(__FILE__, __LINE__, __FUNCTION__, LOGID, PRE_STR format VAL_STR ,##__VA_ARGS__)
#define SLOG_URGENT(format, ...) slog_urgent(__FILE__, __LINE__, __FUNCTION__, LOGID, PRE_STR format VAL_STR ,##__VA_ARGS__)

void slog_trace(const char* file, const int line, const char *func, int logid, const char *fmt, ...);
void slog_debug(const char* file, const int line, const char *func, int logid, const char *fmt, ...);
void slog_info(const char* file, const int line, const char *func, int logid, const char *fmt, ...);
void slog_warn(const char* file, const int line, const char *func, int logid, const char *fmt, ...);
void slog_error(const char* file, const int line, const char *func, int logid, const char *fmt, ...);
void slog_urgent(const char* file, const int line, const char *func, int logid, const char *fmt, ...);

#endif

#ifdef __cplusplus
}
#endif

#endif
