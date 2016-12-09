#ifdef LINUX

#include <unistd.h>
#include <signal.h>
typedef void* (*start_routine)(void*);

#include "platform/sd_task.h"
#include "utility/utility.h"
#include "utility/errcode.h"
#include "utility/string.h"
#define LOGID LOGID_INTERFACE
#include "utility/logger.h"

static _int32 g_sTask_Ids[16] = {
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0};

static void* g_sTask_Stacks[16] = {
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0};

static _int32 g_sTaskCount = 0;

_int32 sd_get_task_ids(_int32* p_task_count, _int32 task_array_size, char* task_array)
{
    *p_task_count = g_sTaskCount;
    sd_memcpy((void*)task_array, (void*)g_sTask_Ids, MIN(task_array_size, g_sTaskCount*sizeof(_int32)));
	return SUCCESS;
}

_int32 sd_create_task(start_address handler, _u32 stack_size, void *arglist, _int32 *task_id)
{
    pthread_attr_t attr;
	pthread_attr_init(&attr);
	/* set stack size */
	if (0 == stack_size)
	{
	    stack_size = DEFAULE_STACK_SIZE;
	}
	pthread_attr_setstacksize(&attr, stack_size);

    _int32 id = 0;
    _int32 *p_id = &id;
	if (NULL != task_id)
	{
	    p_id = task_id;
	}

	/* begin thread */
	_int32 ret_val =ret_val = pthread_create((pthread_t*)p_id, &attr, (start_routine)handler, arglist);
	CHECK_VALUE(ret_val);
	if (SUCCESS == ret_val)
	{
#if (!defined(MACOS) && !defined(_ANDROID_LINUX))
        g_sTask_Ids[g_sTaskCount] = *p_id;
        g_sTask_Stacks[g_sTaskCount] = NULL;
        g_sTaskCount++;
        LOG_DEBUG("\n add g_sTaskCount = %d", g_sTaskCount);
#endif
    }
	pthread_attr_destroy(&attr);

	return ret_val;
}

_int32 sd_finish_task(_int32 task_id)
{
    if (sd_get_self_taskid() == task_id)
    {
    //NODE:jieouy 这块的内容是有问题的，需要再次review.
#if (!defined(MACOS) && !defined(_ANDROID_LINUX))
	    g_sTaskCount--;
	    LOG_DEBUG("\n del g_sTaskCount = %d", g_sTaskCount);
#endif
        CHECK_VALUE(-1);
    }
    
    LOG_DEBUG("\n sd_finish_task thread id= %d", task_id);

	return pthread_join(task_id, NULL);
}

_int32 sd_ignore_signal(void)
{
    signal(SIGPIPE, SIG_IGN);
    return SUCCESS;
}
_int32 sd_pthread_detach(void)
{
	return pthread_detach(sd_get_self_taskid());
}

_int32 sd_init_task_lock(TASK_LOCK *lock)
{
    return pthread_mutex_init(lock, NULL);
}

_int32 sd_uninit_task_lock(TASK_LOCK *lock)
{
    return pthread_mutex_destroy(lock);
}

_int32 sd_task_lock(TASK_LOCK *lock)
{
    return pthread_mutex_lock(lock);
}

_int32 sd_task_trylock(TASK_LOCK *lock)
{
    return pthread_mutex_trylock(lock);
}

_int32 sd_task_unlock(TASK_LOCK *lock)
{
    return pthread_mutex_unlock(lock);
}

_u32 sd_get_self_taskid(void)
{
	return pthread_self();
}

BOOL sd_is_target_thread(_int32 target_pid)
{
    return ((0 != target_pid) && (sd_get_self_taskid()== target_pid));
}

_int32 sd_sleep(_u32 ms)
{
    usleep(ms * 1000);
    return SUCCESS;
}

_int32 sd_init_task_cond(TASK_COND *cond)
{
    return pthread_cond_init(cond, NULL);
}

_int32 sd_uninit_task_cond(TASK_COND *cond)
{
    return pthread_cond_destroy(cond);
}

_int32 sd_task_cond_signal(TASK_COND *cond)
{
    return pthread_cond_signal(cond);
}

_int32 sd_task_cond_wait(TASK_COND *cond, TASK_LOCK *lock)
{
    return pthread_cond_wait(cond, lock);
}

#endif

