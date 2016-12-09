#if !defined(__IFILE_H_20130301)
#define __IFILE_H_20130301

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#include "download_manager/download_task_interface.h"

#include <ooc/ooc.h>

DeclareInterface( IFile )

	BOOL (*Close)( Object ) ;
	size_t (*Read)( Object,  void *buffer, size_t size, size_t count) ;
	size_t (*Write)( Object,  const void *buffer, size_t size, size_t count);
	size_t (*PRead)( Object,  void *buffer, size_t size, size_t count, _int32 read_pos) ;
	size_t (*PWrite)( Object,  const void *buffer, size_t size, size_t count, _int32 write_pos);
	BOOL (*Seek)( Object,  _int32 offset, int origin) ;
	_int64 (*Tell)( Object) ;
	_int64 (*Size)( Object );
	BOOL (*Flush)( Object );
	BOOL (*Eof)( Object);
	BOOL (*Error)( Object);

EndOfInterface;


#ifdef __cplusplus
}
#endif

#endif /* __IFILE_H_20130301 */

