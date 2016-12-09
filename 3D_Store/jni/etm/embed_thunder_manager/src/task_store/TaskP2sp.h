#if !defined(__TASK_P2SP_H_20130301)
#define __TASK_P2SP_H_20130301

#ifdef __cplusplus
extern "C" 
{
#endif

#include "download_manager/download_task_interface.h"
#include "Task.h"

#include <ooc/ooc.h>

DeclareClass( CTaskP2sp, CTask );

/* Virtual function definitions
 */

Virtuals( CTaskP2sp, CTask )
	
EndOfVirtuals;

#ifdef __cplusplus
}
#endif

#endif /* __TASK_P2SP_H_20130301 */
