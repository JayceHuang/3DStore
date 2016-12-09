#if !defined(__TASK_H_20130301)
#define __TASK_H_20130301

#ifdef __cplusplus
extern "C" 
{
#endif

#include "download_manager/download_task_interface.h"

#include <ooc/ooc.h>

DeclareClass( CTask, Base );

/* Virtual function definitions
 */

Virtuals( CTask, Base )

	_u32 (*GetId)(Object);
	TASK_STATE (*GetState)(Object);
	EM_TASK *(*GetAttachedData)(Object);

	void (*Attach)( Object, EM_TASK * );
	EM_TASK *(*Detach)( Object );

	BOOL (*CreateTaskInfo)( Object, TASK_INFO ** );
	void (*FreeTaskInfo)( Object, TASK_INFO * );
	
	BOOL (*CorrectLoadedData)(Object);

	BOOL (*Run)(Object);

	char * (*GetUrl)(Object);
	void (*SetUrl)(Object, const char *url, _u32 len);

	char * (*GetRefUrl)(Object);
	void (*SetRefUrl)(Object, const char *url, _u32 len);

	char * (*GetName)(Object);
	void (*SetName)(Object, const char *name, _u32 len);

	char * (*GetPath)(Object);
	void (*SetPath)(Object, const char *path, _u32 len);
	
	char * (*GetTcid)(Object, _u32 *len);
	void (*SetTcid)(Object, const char *tcid, _u32 len);
	
	void * (*GetUserData)(Object, _u32 *len);
	void (*SetUserData)(Object, const void *data, _u32 len);


    char * (*GetTaskTag)(Object);
	void (*SetTaskTag)(Object, const char *taskTag, _u32 len);

EndOfVirtuals;

_int32 SetStrValue(char **dest, _u32 *dest_len, const char *src, _u32 src_len);

#ifdef __cplusplus
}
#endif

#endif /* __TASK_H_20130301 */
