#if !defined(__PERSISTENT_TASK_IMPL_IMPL_H_20130301)
#define __PERSISTENT_TASK_IMPL_IMPL_H_20130301

#ifdef __cplusplus
extern "C" 
{
#endif

#include "PersistentImpl.h"

ClassMembers( CPersistentTaskImpl, CPersistentImpl )

	_u16 m_uiVersion;
	BOOL m_bValid;
	_u32 m_uiLen;


EndOfClassMembers;


#ifdef __cplusplus
}
#endif

#endif /* __PERSISTENT_TASK_IMPL_IMPL_H_20130301 */
