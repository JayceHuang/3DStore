/*
 ============================================================================
 Name		 : sd_gz.h
 Author	  	 : xueren
 Copyright   : copyright xunlei 2010
 Description : Static lib header file
 ============================================================================
 */

#ifndef __SD_GZ_H_20100908__
#define __SD_GZ_H_20100908__

//  Include Files

//#include <e32base.h>
#include "utility/define.h"

//  Function Prototypes
#ifdef __cplusplus
extern "C" 
{
#endif
#ifdef ENABLE_ZIP
/**
 * 使用gz解压文件
 * @para:	filename-输入文件名, newname-输出文件名
 * @return: 0-SUCCESS else-FAIL
 */
_u32 sd_gz_uncompress_file(char * filename, char * newname);

/**
 * 使用gz对buffer解码，注意，如果传入buffer_out不够长度，sd_gzip_decode_buffer会内部重新分配长度为buffer_out_len的内存
 * @para: 	buffer_in-输入buffer, buffer_in_len-输入buffer的长度
 * 		  	buffer_out-输出buffer, buffer_out_len-输出buffer的总长度
 * 			encode_len-解压后有效数据的长度
 * @return: 0-SUCCESS else-FAIL
 */
_int32 sd_gz_decode_buffer(_u8* buffer_in, _u32 buffer_in_len, _u8** buffer_out, _u32* buffer_out_len,  _u32* decode_len);

/**
 * 使用gz对buffer编码
 * @para: 	buffer_in-输入buffer, buffer_in_len-输入buffer的长度
 * 		  	buffer_out-输出buffer, buffer_out_len-输出buffer的长度
 * 			encode_len-压缩后的长度
 * @return: 0-SUCCESS else-FAIL
 */
_int32 sd_gz_encode_buffer(_u8* buffer_in, _u32 buffer_in_len, _u8* buffer_out, _u32 buffer_out_len, _u32* encode_len);


//////////////////////////////////
//对src中的数据进行gzip压缩，压缩后数据保存在des中
//src:待压缩数据
//src_len:待压缩数据长度
//des:压缩后的数据
//des_len:输入输出参数，传进去的的是des的内存最大长度,传出来的是压缩后的数据长度
_int32 sd_zip_data( const _u8 * src, _u32 src_len,_u8 * des, _u32 * des_len);

//////////////////////////////////
//对src中的数据进行gzip解压，解压后数据保存在des中
//src:待解压数据
//src_len:待解压数据长度
//des:解压后的数据
//des_len:输入输出参数，传进去的的是des的内存最大长度,传出来的是解压后的数据长度
_int32 sd_unzip_data( const _u8 * src, _u32 src_len,_u8 * des, _u32 * des_len);

//////////////////////////////////
//对整个文件进行gzip解压，并用解压后的文件替换解压前的文件
//file_name:待解压文件全名
_int32 sd_unzip_file(const char * file_name);


#endif /*  ENABLE_ZIP */
#ifdef __cplusplus
}
#endif

#endif  // __SD_GZ_H_20100908__

