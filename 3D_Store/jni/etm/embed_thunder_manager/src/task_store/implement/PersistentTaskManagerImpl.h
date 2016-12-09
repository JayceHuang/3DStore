#if !defined(__PERSISTENT_TASK_MANAGE_IMPL_IMPL_H_20130301)
#define __PERSISTENT_TASK_MANAGE_IMPL_IMPL_H_20130301

#ifdef __cplusplus
extern "C" 
{
#endif

#include "PersistentImpl.h"

ClassMembers( CPersistentTaskManagerImpl, CPersistentImpl )

    _u16 m_uiVersion;
    BOOL m_bLoadTasks;

EndOfClassMembers;


#ifdef __cplusplus
}
#endif

#endif /* __PERSISTENT_TASK_MANAGE_IMPL_IMPL_H_20130301 */
