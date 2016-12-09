#ifdef LINUX

#ifndef SD_TASK_LINUX_H_20130730
#define SD_TASK_LINUX_H_20130730

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <pthread.h>

typedef pthread_mutex_t TASK_LOCK;
typedef pthread_cond_t  TASK_COND;

_int32 sd_get_task_ids(_int32* p_task_count, _int32 task_array_size, char* task_array);

#ifdef __cplusplus
}
#endif

#endif

#endif
