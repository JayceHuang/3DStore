#ifndef EM_TASK_H_00138F8F2E70_200806121340
#define EM_TASK_H_00138F8F2E70_200806121340

#include "em_common/em_define.h"

#include "platform/sd_task.h"

#define em_create_task			sd_create_task
#define em_finish_task			sd_finish_task
#define em_ignore_signal			sd_ignore_signal
#define em_pthread_detach			sd_pthread_detach
#define em_init_task_lock			sd_init_task_lock
#define em_uninit_task_lock			sd_uninit_task_lock
#define em_task_lock			sd_task_lock
#define em_task_unlock			sd_task_unlock
#define em_get_self_taskid			sd_get_self_taskid
#define em_sleep			sd_sleep

#ifndef _MSG_POLL
#define  em_init_task_cond 		sd_init_task_cond
#define  em_uninit_task_cond  	sd_uninit_task_cond
#define  em_task_cond_signal 	sd_task_cond_signal
#define  em_task_cond_wait 	sd_task_cond_wait
#endif

#endif
