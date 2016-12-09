#if !defined(__TASK_MANAGER_H_20130301)
#define __TASK_MANAGER_H_20130301

#ifdef __cplusplus
extern "C" 
{
#endif

#include "download_manager/download_task_interface.h"

#include "Task.h"

#include <ooc/ooc.h>

DeclareClass( CTaskManager, Base );

/* Virtual function definitions
 */

Virtuals( CTaskManager, Base )

    _u32 (* GetMaxTaskId )( Object );
    _u32* (* GetOrderList )( Object, _u32 *len );
    _u32* (* GetRunningList )( Object );
    _u32 (* GetRunningCount )( Object );

    BOOL (* GetLoaded )( Object );
    BOOL (* GetTasksLoaded )( Object );

    void (* SetMaxTaskId )( Object, _u32 );
    void (* SetOrderList )( Object, _u32*, _u32 len );
    void (* SetRunningList )( Object, _u32*, _u32 len );

    void (* SetLoaded )( Object, BOOL );
    void (* SetTasksLoaded )( Object, BOOL );

    void (*RunTask)(Object, CTask);

    BOOL (*AddTask)( Object,  CTask );

EndOfVirtuals;

#ifdef __cplusplus
}
#endif

#endif /* __TASK_MANAGER_H_20130301 */
