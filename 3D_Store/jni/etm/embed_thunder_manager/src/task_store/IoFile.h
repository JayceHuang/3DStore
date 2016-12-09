#if !defined(__IO_FILE_IMPL_H_20130301)
#define __IO_FILE_IMPL_H_20130301

#ifdef __cplusplus
extern "C" 
{
#endif

#include "IFile.h"

DeclareClass( CIoFile, Base );

/* Virtual function definitions
 */

Virtuals( CIoFile, Base )

	Interface( IFile );

	BOOL (*Open)(Object, const char * filename, const char * mode);
	

EndOfVirtuals;


#ifdef __cplusplus
}
#endif

#endif /* __IO_FILE_IMPL_H_20130301 */

