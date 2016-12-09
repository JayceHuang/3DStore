#ifdef WINCE

#include "utility/define.h"

#include "platform/sd_task.h"
#include "utility/utility.h"
#include "utility/errcode.h"
#include "utility/string.h"
#define LOGID LOGID_INTERFACE
#include "utility/logger.h"

_int32 sd_create_task(start_address handler, _u32 stack_size, void *arglist, _int32 *task_id)
{
    _int32 ret_val = SUCCESS;
    _int32 id = 0;
    _int32 *p_id = &id;
    if (NULL != task_id)
    {
        p_id = task_id;
    }
	*p_id = (_int32)CreateThread(NULL, stack_size, (LPTHREAD_START_ROUTINE)handler, arglist, 0, NULL);
	if (-1 == *p_id)
	{
	    ret_val = GetLastError();
	}
	return ret_val;
}

_int32 sd_finish_task(_int32 task_id)
{
    //TODO:
    return SUCCESS;
}

_int32 sd_ignore_signal(void)
{
    //TODO:
	return SUCCESS;
}
_int32 sd_pthread_detach(void)
{
    //TODO:
	return SUCCESS;
}

_int32 sd_init_task_lock(TASK_LOCK *lock)
{
	InitializeCriticalSection((CRITICAL_SECTION*)lock);
	return SUCCESS;
}

_int32 sd_uninit_task_lock(TASK_LOCK *lock)
{
	DeleteCriticalSection((CRITICAL_SECTION*)lock);
	return SUCCESS;
}

_int32 sd_task_lock(TASK_LOCK *lock)
{
	EnterCriticalSection((CRITICAL_SECTION*)lock);
	return SUCCESS;
}

_int32 sd_task_trylock(TASK_LOCK *lock)
{
    return SUCCESS;;
}

_int32 sd_task_unlock(TASK_LOCK *lock)
{
    LeaveCriticalSection((CRITICAL_SECTION*)lock);
    return SUCCESS;
}

_u32 sd_get_self_taskid(void)
{
	return GetCurrentThreadId();
}

BOOL sd_is_target_thread(_int32 target_pid)
{
    return ((0 != target_pid) && (sd_get_self_taskid()== target_pid));
}

_int32 sd_sleep(_u32 ms)
{
	Sleep(ms);
	return SUCCESS;
}

_int32 sd_init_task_cond(TASK_COND *cond)
{
    _int32 ret_val = SUCCESS;
    HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (NULL == hEvent)
	{
	    ret_val = GetLastError();
	}
	cond->LockSemaphore = (_int32)hEvent;
	return ret_val;
}

_int32 sd_uninit_task_cond(TASK_COND *cond)
{
    CloseHandle((HANDLE)(cond->LockSemaphore));
    return SUCCESS;
}

_int32 sd_task_cond_signal(TASK_COND *cond)
{
    _int32 ret_val = SUCCESS;
    BOOL ret_b = SetEvent((HANDLE)(cond->LockSemaphore));
    if (FALSE == ret_b)
    {
        ret_val = GetLastError();
    }
	return ret_val;
}

_int32 sd_task_cond_wait(TASK_COND *cond, TASK_LOCK *lock)
{
    return WaitForSingleObject((HANDLE)(cond->LockSemaphore), INFINITE);
}

#endif

