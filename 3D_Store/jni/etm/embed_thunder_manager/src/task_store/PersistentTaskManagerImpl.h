#if !defined(__PERSISTENT_TASK_MANAGE_IMPL_H_20130301)
#define __PERSISTENT_TASK_MANAGE_IMPL_H_20130301

#ifdef __cplusplus
extern "C" 
{
#endif

#include "PersistentImpl.h"


DeclareClass( CPersistentTaskManagerImpl, CPersistentImpl );

/* Virtual function definitions
 */

Virtuals( CPersistentTaskManagerImpl, CPersistentImpl )

	Interface( IPersistentDocument );
	_u16 (*GetVersion)(Object);
	void (*SetVersion)(Object, _u16 ver);

	BOOL (*IsNeedLoadTasks)(Object);
	void (*SetNeedLoadTasks)(Object, BOOL val);
    
EndOfVirtuals;

#ifdef __cplusplus
}
#endif

#endif /* __PERSISTENT_TASK_MANAGE_IMPL_H_20130301 */
