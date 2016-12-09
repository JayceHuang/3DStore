#if !defined(__IPERSISTENT_H_20130301)
#define __IPERSISTENT_H_20130301

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#include "download_manager/download_task_interface.h"

#include <ooc/ooc.h>
#include "IFile.h"

DeclareInterface( IPersistent )

    BOOL    (* Serialize      )( Object, BOOL bRead, Object fileObj );
    void    (* SetDataObj  )( Object, Object p_task);
    Object (* GetDataObj  )( Object );

EndOfInterface;

DeclareInterface( IPersistentDocument )

	BOOL ( *Save )( Object, const char *cszFileName);
	BOOL ( *Open )( Object, const char *cszFileName);

EndOfInterface;


#ifdef __cplusplus
}
#endif

#endif /* __IPERSISTENT_H_20130301 */
