/*----------------------------------------------------------------------------------------------------------
author:		zhangxiaobing
created:	2013/12/10
-----------------------------------------------------------------------------------------------------------*/
#ifndef _MHXY_UDP_H_
#define	_MHXY_UDP_H_
#ifdef __cplusplus
extern "C" 
{
#endif

#ifdef _SUPPORT_MHXY

#include "utility/define.h"

struct tagENCRYPTION_ALGORITHM;


// 第一套方法

/**
@purpose			: 加密数据 
@param	type		: 加密方法
@param	in_buffer	: 需加密的数据
@param	in_len		: 需加密的数据长度
@param	out_buffer	: 加密后的数据
@param	out_len 	: 加密后的数据长度
@return 			: 返回结果值
*/
void mhxy_udp_encrypt1(_int32 type, _u8 *in_buffer, _u32 in_len, _u8 *out_buffer, _u32 *p_out_len);

/**
@purpose			: 解密数据 
@param	in_buffer	: 需解密的数据
@param	in_len		: 需解密的数据长度
@param	out_buffer	: 解密后的数据
@param	out_len 	: 解密后的数据长度
@return 			: 返回加密方法
*/
_int32 mhxy_udp_decrypt1(_u8 *in_buffer, _u32 in_len, _u8 *out_buffer, _u32 *p_out_len);


// 第二套方法

/**
@purpose			: 加密数据 
@param	type		: 加密方法
@param	io_buffer	: 加密前后的数据
@param	io_len		: 加密前后的数据长度
@return 			: 返回结果值
*/
void mhxy_udp_encrypt2(_int32 type, _u8 *io_buffer, _u32 *p_io_len);

/**
@purpose			: 解密数据 
@param	io_buffer	: 解密前后的数据
@param	io_len		: 解密前后的数据长度
@return 			: 返回加密方法
*/
_int32 mhxy_udp_decrypt2(_u8 *io_buffer, _u32 *p_io_len);

#endif //#ifdef _SUPPORT_MHXY


#ifdef __cplusplus
}
#endif
#endif
