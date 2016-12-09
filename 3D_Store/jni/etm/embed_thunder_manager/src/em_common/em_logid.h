#ifndef EM_LOGID_H_00138F8F2E70_200806172148
#define EM_LOGID_H_00138F8F2E70_200806172148

#include "em_define.h"
#include "em_interface/em_task.h"

#include "utility/logid.h"

#ifdef _LOGGER
#define EM_LOGID_COUNT				LOGID_COUNT
#define em_get_logdesc				get_logdesc
#define em_current_loglv				current_loglv
#define em_log_file				log_file
#define em_check_logfile_size				check_logfile_size
#define em_log_lock				log_lock
#endif

#define EM_LOG_ERROR_LV	(1)
#define EM_LOG_DEBUG_LV	(2)



#endif
