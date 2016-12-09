#if !defined(__MEMORY_FILE_IMPL_H_20130301)
#define __MEMORY_FILE_IMPL_H_20130301

#ifdef __cplusplus
extern "C" 
{
#endif

#include "IFile.h"

DeclareClass( CMemoryFile, Base );

/* Virtual function definitions
 */

Virtuals( CMemoryFile, Base )

	Interface( IFile );

	BOOL (*Open)(Object);
	unsigned char* (*GetBuffer)(Object, BOOL bDetachBuffer);

EndOfVirtuals;


#ifdef __cplusplus
}
#endif

#endif /* __MEMORY_FILE_IMPL_H_20130301 */


