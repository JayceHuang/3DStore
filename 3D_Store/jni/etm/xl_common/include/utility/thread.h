#ifndef SD_THREAD_H_00138F8F2E70_200808231422
#define SD_THREAD_H_00138F8F2E70_200808231422

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"

/* Very Simple Thread Crontrol */

typedef struct 
{
    _int32 _thread_idx;
    void *_user_param;
}THREADS_PARAM;

typedef enum 
{
    INIT = 0,
    RUNNING, 
    STOP, 
    STOPPING, 
    STOPPED,
} THREAD_STATUS;

#ifdef LINUX
#include<pthread.h>
#include <stddef.h>

typedef struct 
{
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    _int32 flag;                           //是否通知线程进入休眠状态
    
}SUSPEND_DATA;

#endif


#define THREAD_IDX(arglist) (((THREADS_PARAM*)(arglist))->_thread_idx)
#define USER_PARAM(arglist) (((THREADS_PARAM*)(arglist))->_user_param)

#define RUN_THREAD(status)	((status) = RUNNING)
#define IS_THREAD_RUN(status) ((status) == RUNNING)

#define STOP_THREAD(status)	((status) = STOP)
#define BEGIN_STOP(status) ((status) = STOPPING)

_int32 finished_thread(THREAD_STATUS *status);

/* maybe use notice in future */
_int32 wait_thread(THREAD_STATUS *status, _int32 notice_handle);

_int32 thread_suspend_init(SUSPEND_DATA *data);
_int32 thread_suspend_uninit(SUSPEND_DATA *data);
BOOL thread_is_suspend(SUSPEND_DATA *data);
_int32 thread_do_suspend(SUSPEND_DATA *data); // 如果命名成为thread_suspend则与系统自带函数冲突
_int32 thread_do_resume(SUSPEND_DATA *data); // 如果命名成为thread_resume则与系统自带函数冲突
void thread_check_suspend(SUSPEND_DATA *data);

#define BEGIN_THREAD_LOOP(status)\
       while(IS_THREAD_RUN(status)) {

#define END_THREAD_LOOP(status) } BEGIN_STOP(status);

#ifdef __cplusplus
}
#endif

#endif
