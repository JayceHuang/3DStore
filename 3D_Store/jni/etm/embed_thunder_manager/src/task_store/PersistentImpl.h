#if !defined(__PERSISTENT_IMPL_H_20130301)
#define __PERSISTENT_IMPL_H_20130301

#ifdef __cplusplus
extern "C" 
{
#endif

#include "IPersistent.h"

DeclareClass( CPersistentImpl, Base );

/* Virtual function definitions
 */

Virtuals( CPersistentImpl, Base )

	Interface( IPersistent );

EndOfVirtuals;

_int32 Util_Read(Object fileObj, void *buf, _u32 read_len, _int32 read_pos);
_int32 Util_AllocAndRead(Object fileObj, void **buf, _u32 buf_need_len, _u32 read_len, _int32 read_pos);
_int32 Util_Write(Object fileObj, void *buf, _u32 write_len, _int32 write_pos);

#ifdef __cplusplus
}
#endif

#endif /* __PERSISTENT_IMPL_H_20130301 */
