/*----------------------------------------------------------------------------------------------------------
author:		zhangxiaobing
created:	2013/12/10
-----------------------------------------------------------------------------------------------------------*/
#ifndef _MHXY_TCP_H_
#define	_MHXY_TCP_H_
#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"

#ifdef _SUPPORT_MHXY

struct tagENCRYPTION_ALGORITHM;

typedef struct tagMHXY_TCP
{
	_int32 _mhxy_type; // 加密方法

	struct tagENCRYPTION_ALGORITHM* _ea_decrypt;
	struct tagENCRYPTION_ALGORITHM* _ea_encrypt;

	BOOL _is_first_decrypt;
	BOOL _is_first_encrypt;

}MHXY_TCP;


_int32 mhxy_tcp_new(MHXY_TCP **pp_mhxy_tcp);
_int32 mhxy_tcp_init(MHXY_TCP *p_mhxy_tcp);
_int32 mhxy_tcp_destroy(MHXY_TCP **pp_mhxy_tcp);

_int32 mhxy_tcp_set_type(MHXY_TCP *p_mhxy_tcp, _int32 type);

BOOL mhxy_tcp_is_encrypt(MHXY_TCP *p_mhxy_tcp);

// 有两套方法，建议第一次加密/解密用第一套方法，之后使用第二套方法。

// 第一套方法

/**
@purpose			: 加密数据 
@param	in_buffer	: 需加密的数据
@param	in_len		: 需加密的数据长度
@param	out_buffer	: 加密后的数据
@param	out_len		: 加密后的数据长度
@return				: 返回结果值
*/
void mhxy_tcp_encrypt1(MHXY_TCP *p_mhxy_tcp, _u8 *in_buffer, _u32 in_len, _u8 *out_buffer, _u32 *p_out_len);

/**
@purpose			: 解密数据 
@param	in_buffer	: 需解密的数据
@param	in_len		: 需解密的数据长度
@param	out_buffer	: 解密后的数据
@param	out_len		: 解密后的数据长度
@return				: 返回加密方法
*/
_int32 mhxy_tcp_decrypt1(MHXY_TCP *p_mhxy_tcp, _u8 *in_buffer, _u32 in_len, _u8 *out_buffer, _u32 *p_out_len);


// 第二套方法

/**
@purpose			: 加密数据 
@param	io_buffer	: 加密前后的数据
@param	io_len		: 加密前后的数据长度
@return				: 返回结果值
*/
void mhxy_tcp_encrypt2(MHXY_TCP *p_mhxy_tcp, _u8 *io_buffer, _u32 *p_io_len);

/**
@purpose			: 解密数据 
@param	io_buffer	: 解密前后的数据
@param	io_len		: 解密前后的数据长度
@return				: 返回加密方法
*/
_int32 mhxy_tcp_decrypt2(MHXY_TCP *p_mhxy_tcp, _u8 *io_buffer, _u32 *p_io_len);

#endif//_SUPPORT_MHXY

#ifdef __cplusplus
}
#endif
#endif
