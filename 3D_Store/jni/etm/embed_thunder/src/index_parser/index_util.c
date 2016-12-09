
#include "utility/string.h"
#include "utility/utility.h"
#include "utility/url.h"
#include "utility/map.h"
#include "utility/errcode.h"
#include "utility/url.h"
#include "index_parser.h"
#include "index_util.h"
#include "rmvb_parser.h"

_u16
rmff_get_uint16_be(const void *buf) {
	_u16 ret;
	_u8 *tmp;
	
	tmp = (_u8 *) buf;
	
	ret = tmp[0] & 0xff;
	ret = (ret << 8) + (tmp[1] & 0xff);
	
	return ret;
}

_u32
rmff_get_uint32_be(const void *buf) {
	_u32 ret;
	_u8 *tmp;
	
	tmp = (_u8 *) buf;
	
	ret = tmp[0] & 0xff;
	ret = (ret << 8) + (tmp[1] & 0xff);
	ret = (ret << 8) + (tmp[2] & 0xff);
	ret = (ret << 8) + (tmp[3] & 0xff);
	
	return ret;
}

_u64
rmff_get_uint64_be(const void *buf) {
	_u64 ret;
	_u8 *tmp;
	
	tmp = (_u8 *) buf;
	
	ret = tmp[0] & 0xff;
	ret = (ret << 8) + (tmp[1] & 0xff);
	ret = (ret << 8) + (tmp[2] & 0xff);
	ret = (ret << 8) + (tmp[3] & 0xff);
	ret = (ret << 8) + (tmp[4] & 0xff);
	ret = (ret << 8) + (tmp[5] & 0xff);
	ret = (ret << 8) + (tmp[6] & 0xff);
	ret = (ret << 8) + (tmp[7] & 0xff);
	
	return ret;
}

_u16
rmff_get_uint16_le(const void *buf) {
	_u16 ret;
	_u8 *tmp;
	
	tmp = (_u8 *) buf;
	
	ret = tmp[1] & 0xff;
	ret = (ret << 8) + (tmp[0] & 0xff);
	
	return ret;
}



_u32
rmff_get_uint32_le(const void *buf) {
	_u32 ret;
	_u8 *tmp;
	
	tmp = (_u8 *) buf;
	
	ret = tmp[3] & 0xff;
	ret = (ret << 8) + (tmp[2] & 0xff);
	ret = (ret << 8) + (tmp[1] & 0xff);
	ret = (ret << 8) + (tmp[0] & 0xff);
	
	return ret;
}

_u64
rmff_get_uint64_le(const void *buf) {
	_u64 ret;
	_u8 *tmp;
	
	tmp = (_u8 *) buf;
	
	ret = tmp[7] & 0xff;
	ret = (ret << 8) + (tmp[6] & 0xff);
	ret = (ret << 8) + (tmp[5] & 0xff);
	ret = (ret << 8) + (tmp[4] & 0xff);
	ret = (ret << 8) + (tmp[3] & 0xff);
	ret = (ret << 8) + (tmp[2] & 0xff);
	ret = (ret << 8) + (tmp[1] & 0xff);
	ret = (ret << 8) + (tmp[0] & 0xff);
	return ret;
}


void
rmff_put_uint16_be(void *buf,
                   _u16 value) {
	_u8 *tmp;
	
	tmp = (_u8 *) buf;
	
	tmp[1] = value & 0xff;
	tmp[0] = (value >>= 8) & 0xff;
}

void
rmff_put_uint32_be(void *buf,
                   _u32 value) {
	_u8 *tmp;
	
	tmp = (_u8 *) buf;
	
	tmp[3] = value & 0xff;
	tmp[2] = (value >>= 8) & 0xff;
	tmp[1] = (value >>= 8) & 0xff;
	tmp[0] = (value >>= 8) & 0xff;
}

void
rmff_put_uint32_le(void *buf,
                   _u32 value) {
	_u8 *tmp;
	
	tmp = (_u8 *) buf;
	
	tmp[0] = value & 0xff;
	tmp[1] = (value >>= 8) & 0xff;
	tmp[2] = (value >>= 8) & 0xff;
	tmp[3] = (value >>= 8) & 0xff;
}

_u32 buffer_read_uint8(BUFFER_IO *io, _u8* retval) {
	_u8 tmp;
	
	if(io->_pos + 1 >= io->_size){
		/*error , no more buffer left*/
		return INDEX_PARSER_ERR_IO;
	}else{
		tmp = io->_buffer[io->_pos];
		io->_pos++;
		*retval = tmp;
	}
	return INDEX_PARSER_ERR_OK;
}

_u32
buffer_read_uint16_be(BUFFER_IO *io, _u16* retval) {
	_u8 tmp[2];
	
	if(io->_pos + 2 >= io->_size){
		/*error , no more buffer left*/
		return RMFF_ERR_IO;
	}else{
		io->read(io, tmp, (_u64)2);
	}
	*retval = rmff_get_uint16_be(tmp);
	return RMFF_ERR_OK;
}

_u32
buffer_read_uint32_be(BUFFER_IO *io, _u32* retval) {
	_u8 tmp[4];
	
	if(io->_pos + 4 >= io->_size){
		/*error , no more buffer left*/
		return RMFF_ERR_IO;
	}else{
		io->read(io, tmp, (_u64)4);
	}
	*retval = rmff_get_uint32_be(tmp);
	return RMFF_ERR_OK;
}

_u32 buffer_read_uint64_be(BUFFER_IO *io, _u64* retval)
{
	_u8 tmp[8];
	
	if(io->_pos + 8 >= io->_size){
		/*error , no more buffer left*/
		return RMFF_ERR_IO;
	}else{
		io->read(io, tmp, (_u64)8);
	}
	*retval = rmff_get_uint64_be(tmp);
	return RMFF_ERR_OK;
}

_u32
buffer_read_uint16_le(BUFFER_IO *io, _u16* retval) {
	_u8 tmp[2];
	
	if(io->_pos + 2 >= io->_size){
		/*error , no more buffer left*/
		return RMFF_ERR_IO;
	}else{
		io->read(io, tmp, (_u64)2);
	}
	*retval = rmff_get_uint16_le(tmp);
	return RMFF_ERR_OK;
}

_u32
buffer_read_uint32_le(BUFFER_IO *io, _u32* retval) {
	_u8 tmp[4];
	
	if(io->_pos + 4 >= io->_size){
		/*error , no more buffer left*/
		return RMFF_ERR_IO;
	}else{
		io->read(io, tmp, (_u64)4);
	}
	*retval = rmff_get_uint32_le(tmp);
	return RMFF_ERR_OK;
}

_u32 buffer_read_uint64_le(BUFFER_IO *io, _u64* retval)
{
	_u8 tmp[8];
	
	if(io->_pos + 8 >= io->_size){
		/*error , no more buffer left*/
		return RMFF_ERR_IO;
	}else{
		io->read(io, tmp, (_u64)8);
	}
	*retval = rmff_get_uint64_le(tmp);
	return RMFF_ERR_OK;
}



_int64
ip_std_buffer_read(BUFFER_IO *io,
                 void *buffer,
                 _int64 bytes) {
	if ((io == NULL) || (buffer == NULL) || (bytes < 0))
		return -1;
	if(io->_pos + bytes >= io->_size )
	{
		return -1;
	}
	sd_memcpy(buffer, io->_buffer + io->_pos, bytes);
	io->_pos += bytes;
	return bytes;
}


_int32 skip_bytes(BUFFER_IO * io, _u64 size)
{
	if(io->_pos + size >= io->_size){
		/*error , no more buffer left*/
		return RMFF_ERR_IO;
	}else{
		io->_pos += size;
	}
	return RMFF_ERR_OK;

}
