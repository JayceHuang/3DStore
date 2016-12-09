#include "utility/bitmap.h"
#include "utility/string.h"
#include "utility/errcode.h"
#include "utility/sd_assert.h"
#include "utility/mempool.h"
#include "utility/logid.h"
#define	LOGID	LOGID_COMMON
#include "utility/logger.h"

_int32 bitmap_init(BITMAP* bitmap)
{
	bitmap->_bit = NULL;
	bitmap->_bit_count = 0;
	bitmap->_mem_size = 0;
	return SUCCESS;
}

_int32 bitmap_uninit(BITMAP* bitmap)
{
	SAFE_DELETE(bitmap->_bit);
	bitmap->_bit_count = 0;
	bitmap->_mem_size = 0;
	return SUCCESS;
}

// 重设大小,只会扩大，不会缩小
_int32 bitmap_resize(BITMAP* bitmap, _u32 bit_count)
{
	_int32 ret = SUCCESS;
	sd_assert(bit_count > 0);
	bitmap->_bit_count = bit_count;
	if(bitmap->_mem_size < (bit_count + 7) / 8)
	{
		bitmap->_mem_size = (bit_count + 7) / 8;
		SAFE_DELETE(bitmap->_bit);
		ret = sd_malloc(bitmap->_mem_size, (void**)&bitmap->_bit);
		if(ret != SUCCESS)
		{
			//sd_assert(FALSE);
			return ret;
		}
	}
	return sd_memset(bitmap->_bit, 0, bitmap->_mem_size);
}

_int32 bitmap_init_with_bit_count(BITMAP* bitmap, _u32 bit_count)
{
	_int32 ret = SUCCESS;
	bitmap_init( bitmap );
	if( bit_count == 0 ) return SUCCESS;
	ret = bitmap_resize( bitmap, bit_count );
	if(ret != SUCCESS)
	{
		return ret;
	}
	return SUCCESS;	
}

// 给指定位赋值
_int32 bitmap_set(BITMAP* bitmap, _u32 pos, BOOL val)
{
	_u8 ch = '\x80';
	_u32 byte_index, bit_index;
	if(bitmap->_bit == NULL || pos >= bitmap->_bit_count)
	{
		return -1;
	}
	byte_index = pos / 8;
	bit_index  = pos % 8;
	ch >>= bit_index;
	if (val)
	{
		bitmap->_bit[byte_index] |= ch;
	}
	else
	{	
		bitmap->_bit[byte_index] &= ~ch;
	}
	return SUCCESS;
}

_u32 bitmap_bit_count(const BITMAP* bitmap)
{
	return bitmap->_bit_count;
}

BOOL bitmap_at(const BITMAP* bitmap, _u32 pos)
{
	sd_assert(pos < bitmap->_bit_count);
	sd_assert(bitmap->_bit != NULL);
    
    if (!bitmap->_bit) {
        return FALSE;
    }
	if(pos >= bitmap->_bit_count)
		return FALSE;
	if( (bitmap->_bit[pos / 8] & ((_u8)('\x80') >> (pos % 8))) == 0)
		return FALSE;
	else
		return TRUE;
}

BOOL bitmap_all_none(BITMAP* bitmap)
{
	_int32 i = 0;
	if(bitmap->_bit == NULL)
		return TRUE;
	for(i = 0; i < bitmap->_mem_size; ++i) 
	{
		if(bitmap->_bit[i] != 0)
			return FALSE;
	}
	return TRUE;
}

_int32 bitmap_from_bits(BITMAP* bitmap, char* data, _u32 data_len, _u32 bit_count)
{
	_int32 ret = SUCCESS;
	if((bit_count + 7) / 8 != data_len)
	{
		LOG_ERROR("bitmap_from_bits error. data_len = %u, bit_count = %u.", data_len, bit_count);
		//sd_assert(FALSE);
		return -1;
	}
	if(bitmap->_mem_size < data_len)
	{
		SAFE_DELETE(bitmap->_bit);
		ret = sd_malloc(data_len, (void**)&bitmap->_bit);
		if(ret != SUCCESS)
		{
			LOG_ERROR("bitmap_from_bits, but malloc failed.");
			CHECK_VALUE(ret);
		}
		bitmap->_mem_size = data_len;
	}
	sd_memcpy(bitmap->_bit, data, (_int32)data_len);
	bitmap->_bit_count = bit_count;
	return SUCCESS;
}

_int32 bitmap_adjust(BITMAP* left, BITMAP* right)
{
	_u32 i = 0;
	if(left->_bit == NULL || right->_bit == NULL || left->_bit_count != right->_bit_count || left->_mem_size != right->_mem_size)
	{
		LOG_ERROR("bitmap_adjust failed.");
		sd_assert(FALSE);
		return -1;
	}
	for(i = 0; i < left->_mem_size; ++i) 
	{
		left->_bit[i] = left->_bit[i] & right->_bit[i];
	}
	return SUCCESS;
}

void bitmap_print(const BITMAP* bitmap)
{
#ifdef _LOGGER
	
	_u32 map_num  = bitmap->_bit_count;
	_u32 print_size = 50;
	_int32 ret_val = SUCCESS;
	char *p_map_value  = NULL;
	_u32 map_index = 0;
	_u32 index = 0;
	
	LOG_DEBUG("bitmap_print:bit_map:0x%x.", bitmap );

	ret_val = sd_malloc( print_size + 1,  (void **)&p_map_value );
	if( ret_val != SUCCESS ) return;

	for( map_index = 0; map_index < map_num; map_index++ )
	{
		index = map_index % print_size;
		if( bitmap_at( bitmap, map_index ) )
		{
			p_map_value[index] = '1';
		}
		else
		{
			p_map_value[index] = '0';
		}
		if( index == print_size - 1 )
		{
			p_map_value[ print_size ] = '\0';
			LOG_DEBUG("bitmap_print:line:%d, value:%s.", 
				map_index / print_size, p_map_value );
		}
	}
	if( map_num % print_size != 0 )
	{
		p_map_value[ map_num % print_size ] = '\0';
		LOG_DEBUG("bitmap_print:line:%d, value:%s.", 
			map_num / print_size, p_map_value );
	}
	
	sd_free( p_map_value );
	p_map_value = NULL;
#endif
	
}



static _u8 BITMAP_HEAD_MASK[8]={0xFF,0x7F,0x3F,0x1F,0x0F,0x07,0x03,0x01};
static _u8 BITMAP_END_MASK[8]={0x80,0xC0,0xE0,0xF0,0xF8,0xFC,0xFE,0xFF};

BOOL bitmap_range_is_all_set(BITMAP* bitmap, _u32 start_pos, _u32 end_pos)
{
         _u32 start_index = 0;
	 _u32 end_index = 0;	  
        _u32 start_off = 0;
	 _u32 end_off = 0;	  


        sd_assert(start_pos < bitmap->_bit_count);
	 sd_assert(end_pos < bitmap->_bit_count);
	 if(start_pos >= bitmap->_bit_count || end_pos >= bitmap->_bit_count)
	 {
	       return FALSE;
	 }

	start_index =  start_pos/8;
       start_off = start_pos%8; 

	end_index =  end_pos/8;
       end_off = end_pos%8;

	if(start_index == end_index) 
	{
	      if((bitmap->_bit[start_index]&BITMAP_HEAD_MASK[start_off]&BITMAP_END_MASK[end_off])
		  	== (BITMAP_HEAD_MASK[start_off]&BITMAP_END_MASK[end_off]))
		{
		     return TRUE;
		}
		else
		{
		       return FALSE;
		}
	}

	if((bitmap->_bit[start_index]&BITMAP_HEAD_MASK[start_off]) != BITMAP_HEAD_MASK[start_off])
	{
	      return FALSE;
	}

	start_index++;
	
       if((bitmap->_bit[end_index]&BITMAP_END_MASK[end_off]) != BITMAP_END_MASK[end_off])
	{
	      return FALSE;
	}

	end_index--;

	for(;start_index<=end_index; start_index++)
	{
	       if(bitmap->_bit[start_index] != 0xFF)
	       {
	             return FALSE;
	       }
	}

	return TRUE;
	   
}
_int32 bitmap_xor( const BITMAP* bitmap1, const BITMAP* bitmap2, BITMAP* ret )
{
	_int32 ret_val = SUCCESS;
	_u32 index = 0;
	
    sd_assert(bitmap1->_bit_count == bitmap2->_bit_count);
    sd_assert(bitmap1->_mem_size == bitmap2->_mem_size);

	if( bitmap1->_bit_count != bitmap2->_bit_count
		|| bitmap1->_mem_size != bitmap2->_mem_size
		|| bitmap1->_bit == NULL || bitmap2->_bit == NULL )
	{
		//sd_assert( FALSE );
		return INVALID_ARGUMENT;
	}
	ret_val = bitmap_init_with_bit_count( ret, bitmap1->_bit_count );
	if( ret_val != SUCCESS ) return ret_val;
    if (!ret->_bit) {
        return -1;
    }
    
	for( ; index < bitmap1->_mem_size; index++ )
	{
		ret->_bit[index] = bitmap1->_bit[index] ^ bitmap2->_bit[index];
	}
	return SUCCESS;
}

BOOL bitmap_is_equal( const BITMAP* bitmap1, const BITMAP* bitmap2, _u32 pos )
{
	_int32 ret_bool = TRUE;
	_u32 index = (pos+7) / 8;
	_u8 value = 0;
	if( bitmap1->_bit_count != bitmap2->_bit_count
		|| bitmap1->_mem_size != bitmap2->_mem_size
		|| bitmap1->_bit == NULL || bitmap2->_bit == NULL )
	{
		sd_assert( FALSE );
		return FALSE;
	}
	for( ; index < bitmap1->_mem_size; index++ )
	{
		value = bitmap1->_bit[index] ^ bitmap2->_bit[index];
		if( value != 0 )
		{
			ret_bool = FALSE;
			break;
		}
	}
	return ret_bool;
}

_int32 bitmap_copy( const BITMAP* bitmap_src, BITMAP* bitmap_dest )
{
	if( bitmap_src->_bit_count > bitmap_dest->_bit_count
		|| bitmap_src->_mem_size > bitmap_dest->_mem_size
		|| bitmap_src->_bit == NULL || bitmap_dest->_bit == NULL )
	{
		sd_assert( FALSE );
		return INVALID_ARGUMENT;
	}

	sd_memcpy( bitmap_dest->_bit, bitmap_src->_bit, bitmap_src->_mem_size );
	return SUCCESS;
}

_u8 *bitmap_get_bits( BITMAP* bitmap )
{
	return bitmap->_bit;
}

_int32 bitmap_compare( BITMAP *p_bitmap_1, BITMAP *p_bitmap_2, _int32 *p_result )
{
	//_u32 int_count = 0;
	//_u32 int_index = 0;
	_u32 buffer_index = 0;
	//_u32 value_32_1 = 0, value_32_2 = 0;
	_u8 value_8_1 = 0, value_8_2 = 0;
	if( p_bitmap_1->_bit_count > p_bitmap_2->_bit_count
		|| p_bitmap_1->_mem_size > p_bitmap_2->_mem_size
		|| p_bitmap_1->_bit == NULL || p_bitmap_2->_bit == NULL
		|| p_bitmap_1->_mem_size == 0 )
	{
		sd_assert( FALSE );
		return INVALID_ARGUMENT;
	}
    /*
	int_count = p_bitmap_1->_bit_count / 32;
	while( int_index < int_count )
	{
		value_32_1 = (_u32)p_bitmap_1->_bit[buffer_index];
		value_32_2 = (_u32)p_bitmap_2->_bit[buffer_index];

		if( value_32_1 > value_32_2 )
		{
			*p_result = 1;
            return SUCCESS;
		}
		else if( value_32_1 < value_32_2 )
		{
            *p_result = -1;
            return SUCCESS;
		}

		int_index++;
		buffer_index += 4;		
	}
	*/
	while( buffer_index < p_bitmap_1->_mem_size )
	{
		value_8_1 = (_u8)p_bitmap_1->_bit[buffer_index];
		value_8_2 = (_u8)p_bitmap_2->_bit[buffer_index];

		if( value_8_1 > value_8_2 )
		{
			*p_result = 1;
            return SUCCESS;
		}
		else if( value_8_1 < value_8_2 )
		{
            *p_result = -1;
            return SUCCESS;
		}
		buffer_index++;
	}
    *p_result = 0;
    return SUCCESS;
}

#ifdef ENABLE_EMULE
_int32 bitmap_emule_at(BITMAP* bitmap, _u32 pos)
{
	sd_assert(pos < bitmap->_bit_count);
	sd_assert(bitmap->_bit != NULL);
    
    if (!bitmap->_bit) {
        return FALSE;
    }
	if(pos >= bitmap->_bit_count)
		return FALSE;
	if( (bitmap->_bit[pos / 8] & ((_u8)1 << (pos % 8))) == 0)
		return FALSE;
	else
		return TRUE;
}

_int32 bitmap_emule_set(BITMAP* bitmap, _u32 pos, BOOL val)
{
	_u8 ch = 1;
	_u32 byte_index, bit_index;
	if(bitmap->_bit == NULL || pos >= bitmap->_bit_count)
	{
		return -1;
	}
	byte_index = pos / 8;
	bit_index  = pos % 8;
	ch <<= bit_index;
	if (val)
	{
		bitmap->_bit[byte_index] |= ch;
	}
	else
	{	
		bitmap->_bit[byte_index] &= ~ch;
	}
	return SUCCESS;
}
#endif


