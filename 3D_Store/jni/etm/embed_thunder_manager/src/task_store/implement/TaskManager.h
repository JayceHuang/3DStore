#if !defined(__TASK_MANAGER_IMPL_H_20130301)
#define __TASK_MANAGER_IMPL_H_20130301

#ifdef __cplusplus
extern "C" 
{
#endif

#include "download_manager/download_task_interface.h"

ClassMembers( CTaskManager, Base )

    _u32 running_tasks_array[MAX_ALOW_TASKS];
    _u32 *_dling_task_order;
    _u32 task_num;
    _u32 max_task_id;

    BOOL loaded;
    BOOL tasks_loaded;

EndOfClassMembers;


#ifdef __cplusplus
}
#endif

#endif /* __TASK_MANAGER_IMPL_H_20130301 */
