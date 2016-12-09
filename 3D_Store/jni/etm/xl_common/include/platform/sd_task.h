#ifndef SD_TASK_H_00138F8F2E70_200806121340
#define SD_TASK_H_00138F8F2E70_200806121340

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#include "platform/sd_task_linux.h"
#include "platform/sd_task_wince.h"

#define DEFAULE_STACK_SIZE	(64 * 1024)

#ifdef _DEBUG
#define CHECK_THREAD_VALUE(code) CHECK_VALUE(code)
#else
#define CHECK_THREAD_VALUE(code) {if(code != SUCCESS) EXIT(code);}
#endif

typedef void (*start_address) (void *);

_int32 sd_create_task(start_address handler, _u32 stack_size, void *arglist, _int32 *task_id);

_int32 sd_finish_task(_int32 task_id);
_int32 sd_ignore_signal(void);
_int32 sd_pthread_detach(void);

_int32 sd_init_task_lock(TASK_LOCK *lock);
_int32 sd_uninit_task_lock(TASK_LOCK *lock);

_int32 sd_task_lock(TASK_LOCK *lock);
_int32 sd_task_trylock(TASK_LOCK *lock);
_int32 sd_task_unlock(TASK_LOCK *lock);

_u32 sd_get_self_taskid(void);
BOOL sd_is_target_thread(_int32 target_pid);

_int32 sd_sleep(_u32 ms);

_int32 sd_init_task_cond(TASK_COND *cond);
_int32 sd_uninit_task_cond(TASK_COND *cond);

_int32 sd_task_cond_signal(TASK_COND *cond);
_int32 sd_task_cond_wait(TASK_COND *cond, TASK_LOCK *lock);

#ifdef __cplusplus
}
#endif

#endif
