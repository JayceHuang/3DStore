#if !defined(__PERSISTENT_TASK_IMPL_H_20130301)
#define __PERSISTENT_TASK_IMPL_H_20130301

#ifdef __cplusplus
extern "C" 
{
#endif

#include "PersistentImpl.h"

DeclareClass( CPersistentTaskImpl, CPersistentImpl );

/* Virtual function definitions
 */

Virtuals( CPersistentTaskImpl, CPersistentImpl )

	Interface( IPersistentDocument );

	_u16 (*GetVersion)(Object);
	void (*SetVersion)(Object, _u16 ver);

	BOOL (*IsValid)(Object);
	void (*SetValid)(Object, BOOL bVal);

	_u32 (*GetLength)(Object);
	void (*SetLength)(Object, _u32 len);

	BOOL (*DetermineTaskType)(Object, Object fileObj, EM_TASK_TYPE *type);

	BOOL ( *Read )( Object, Object fileObj );

	BOOL (*ConvertVersionFrom1To2)( Object, Object fileObj );


EndOfVirtuals;

#ifdef __cplusplus
}
#endif

#endif /* __PERSISTENT_TASK_IMPL_H_20130301 */
