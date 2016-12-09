
#if !defined(__INDEX_UTIL_20090201)
#define __INDEX_UTIL_20090201

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#include "utility/range.h"

_u16 rmff_get_uint16_be(const void *buf);

_u32 rmff_get_uint32_be(const void *buf);

_u32 rmff_get_uint32_le(const void *buf); 

void rmff_put_uint16_be(void *buf,  _u16 value) ;

void rmff_put_uint32_be(void *buf, _u32 value) ;

void rmff_put_uint32_le(void *buf,  _u32 value) ;

_int32 set_error(int error_number, const char *error_msg, int return_value) ;

struct _buffer_io;
typedef _int64 (*mb_file_read_t)(struct _buffer_io *file, void *buffer, _int64 bytes);
typedef struct _buffer_io {
    char* _buffer;
	_u64  _size;
    _u64  _pos;
    mb_file_read_t read;
} BUFFER_IO ;

_u32 buffer_read_uint8(BUFFER_IO *io, _u8* retval);
_u32 buffer_read_uint16_be(BUFFER_IO *io, _u16* retval);
_u32 buffer_read_uint32_be(BUFFER_IO *io, _u32* retval) ;
_u32 buffer_read_uint64_be(BUFFER_IO *io, _u64* retval);

_u32 buffer_read_uint16_le(BUFFER_IO *io, _u16* retval);
_u32 buffer_read_uint32_le(BUFFER_IO *io, _u32* retval) ;
_u32 buffer_read_uint64_le(BUFFER_IO *io, _u64* retval);

_int64 ip_std_buffer_read(BUFFER_IO *io, void *buffer,_int64 bytes) ;
_int32 skip_bytes(BUFFER_IO * io, _u64 size);

#ifdef __cplusplus
}
#endif

#endif /* !defined(__INDEX_UTIL_20090201)*/



