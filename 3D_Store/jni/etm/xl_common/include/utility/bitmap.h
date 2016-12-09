/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2008/11/28
-----------------------------------------------------------------------------------------------------------*/
#ifndef _BITMAP_H_
#define _BITMAP_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"

//考虑到嵌入式对性能的要求，该bitmap的实现使用固定的8字节做为最大存储容量,目前发现最大的bit_count为161
#define		MAX_BITMAP_SIZE		100

typedef struct tagBITMAP
{
	_u8*	_bit;
	_u32	_bit_count; //位数
	_u32	_mem_size;	//内存大小
}BITMAP;

_int32 bitmap_init(BITMAP* bitmap);

_int32 bitmap_uninit(BITMAP* bitmap);

_int32 bitmap_resize(BITMAP* bitmap, _u32 bit_count);	//重设大小

_int32 bitmap_init_with_bit_count( BITMAP* bitmap, _u32 bit_count );	

_int32 bitmap_set(BITMAP* bitmap, _u32 pos, BOOL val);	// 给指定位赋值

_u32 bitmap_bit_count(const BITMAP* bitmap);	//得到位数

BOOL bitmap_at(const BITMAP* bitmap, _u32 pos);	//指定位是否为1

BOOL bitmap_all_none(BITMAP* bitmap);	//是否所有位均为0

_int32 bitmap_from_bits(BITMAP* bitmap, char* data, _u32 data_len, _u32 bit_count);

_int32 bitmap_adjust(BITMAP* left, BITMAP* right);

void bitmap_print(const BITMAP* bitmap);

BOOL bitmap_range_is_all_set(BITMAP* bitmap, _u32 start_pos, _u32 end_pos);

_int32 bitmap_xor( const BITMAP* bitmap1, const BITMAP* bitmap2, BITMAP* ret );

BOOL bitmap_is_equal( const BITMAP* bitmap1, const BITMAP* bitmap2, _u32 pos );

//为提高效率,bitmap_dest必须已经初始化并且_bit_count和_mem_size不小于对应bitmap_src的值.
_int32 bitmap_copy( const BITMAP* bitmap_src, BITMAP* bitmap_dest );

_u8 *bitmap_get_bits( BITMAP* bitmap );

_int32 bitmap_compare( BITMAP *p_bitmap_1, BITMAP *p_bitmap_2, _int32 *p_result );

#ifdef ENABLE_EMULE
//emule的bitmap访问是一个字节从右向左的顺序，与一般的不一样，所以加多这两个函数
_int32 bitmap_emule_at(BITMAP* bitmap, _u32 pos);

_int32 bitmap_emule_set(BITMAP* bitmap, _u32 pos, BOOL val);
#endif

#ifdef __cplusplus
}
#endif

#endif
