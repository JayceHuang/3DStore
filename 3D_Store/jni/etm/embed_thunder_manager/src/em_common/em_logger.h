#ifndef EM_LOGGER_H_00138F8F2E70_200806171745
#define EM_LOGGER_H_00138F8F2E70_200806171745

#ifdef __cplusplus
extern "C" 
{
#endif

#include "em_define.h"
#include "em_errcode.h"
#include "em_arg.h"
#include "em_logid.h"

#define EM_URGENT_TO_FILE	write_urgent_to_file

#ifdef _LOGGER

#ifndef EM_LOGID
#error Compile error, must define EM_LOGID
#endif

#define LOGID EM_LOGID
#include "utility/logger.h"
#define em_logger logger
#define em_logger_wrap logger_wrap
#define EM_LOG_DEBUG LOG_DEBUG
#define EM_LOG_ERROR  LOG_ERROR
#define EM_LOG_URGENT LOG_URGENT

#else

#define EM_LOG_DEBUG(...)
#define EM_LOG_ERROR(...)
#define EM_LOG_URGENT(...)
			//EM_URGENT_TO_FILE

#endif


#ifdef __cplusplus
}
#endif

#endif
