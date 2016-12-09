#if !defined(__TASK_BT_H_20130301)
#define __TASK_BT_H_20130301

#ifdef __cplusplus
extern "C" 
{
#endif

#include "download_manager/download_task_interface.h"
#include "Task.h"

#include <ooc/ooc.h>

DeclareClass( CTaskBt, CTask );

/* Virtual function definitions
 */

Virtuals( CTaskBt, CTask )

	_u16 * (*GetNeedDlFile)(Object, _u16 *len);
	void (*SetNeedDlFile)(Object, const _u16 *data, _u16 len);
	
	char * (*GetSeedFile)(Object);
	void (*SetSeedFile)(Object, const char *seed_file, _u16 len);
	
	BT_FILE * (*GetSubFile)(Object, _u16 index);
	void (*SetSubFile)(Object, _u16 index, BT_FILE *sub_file);
    
EndOfVirtuals;

#ifdef __cplusplus
}
#endif

#endif /* __TASK_BT_H_20130301 */
