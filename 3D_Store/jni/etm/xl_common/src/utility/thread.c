#include "utility/thread.h"
#include "utility/errcode.h"
#include "platform/sd_task.h"
#include "asyn_frame/notice.h"

#include "utility/logid.h"
#ifdef LOGID
#undef LOGID
#endif

#define LOGID LOGID_COMMON

#include "utility/logger.h"

_int32 finished_thread(THREAD_STATUS *status)
{
	*status = STOPPED;
	return SUCCESS;
}

_int32 wait_thread(THREAD_STATUS *status, _int32 notice_handle)
{
	_u32 wait_count = 0;
	while ((STOPPED != *status) && (wait_count < 500))
	{
		/* not care of whether if success */
		if ((0 != notice_handle) && (STOP == *status))
		{
		    notice_impl(notice_handle);
		}

		sd_sleep(20);
		wait_count++; // wait 2s at most
	}
	return SUCCESS;
}

_int32 thread_suspend_init(SUSPEND_DATA *data)
{
    LOG_DEBUG("thread_suspend_init suspend data:%x", data);
    
    _int32 ret = -1;    
#ifdef LINUX
    data->flag = 0;
    int nRet = pthread_mutex_init(&data->mutex, NULL); // Return Check
    if (0 == nRet)
    {
        nRet = pthread_cond_init(&data->cond, NULL); // Return Check
        if (0 == nRet)
        {
            nRet = pthread_mutex_lock(&data->mutex);// Return Check
            if (0 == nRet)
            {
                ret = SUCCESS;
            }
        }
    }
#endif

    return ret;
}


_int32 thread_suspend_uninit(SUSPEND_DATA *data)
{
    LOG_DEBUG("thread_suspend_uninit suspend data:%x", data);
    
#ifdef LINUX
    pthread_mutex_destroy(&data->mutex);
    pthread_cond_destroy(&data->cond);
#endif

    return SUCCESS;
}

BOOL thread_is_suspend(SUSPEND_DATA *data)
{
    LOG_DEBUG("thread_is_suspend suspend data:%x", data);
    BOOL bRet = FALSE;

#ifdef LINUX
    int nRet = pthread_mutex_trylock(&data->mutex);
    if (0 == nRet) // Thread was suspended
    {
        nRet = pthread_mutex_unlock(&data->mutex);
        bRet = TRUE;
    }
#endif
    
    return bRet;
}

_int32 thread_do_suspend(SUSPEND_DATA *data)
{
    LOG_DEBUG("thread_suspend suspend data:%x", data);

#ifdef LINUX
    if (thread_is_suspend(data))// Alredy Suspended
    {
        return SUCCESS;
    }
    data->flag = 1;    
#endif

    return -1;
}

_int32 thread_do_resume(SUSPEND_DATA *data)
{ 
    LOG_DEBUG("thread_resume suspend data:%x", data);
    _int32 nRet = -1;
#ifdef LINUX
    if (TRUE == thread_is_suspend(data))
    {
        if (0 == pthread_cond_signal(&data->cond))
        {
            nRet = SUCCESS;
        }
    } 
#endif
    return nRet;
}

void thread_check_suspend(SUSPEND_DATA *data)
{
//    LOG_DEBUG("thread_check_suspend suspend data:%x begin", data);

#ifdef LINUX
    if (data->flag == 1)
    {
        data->flag = 0;
        pthread_cond_wait(&data->cond, &data->mutex);
    } 
#endif

//    LOG_DEBUG("thread_check_suspend suspend data:%x end", data); 
}

