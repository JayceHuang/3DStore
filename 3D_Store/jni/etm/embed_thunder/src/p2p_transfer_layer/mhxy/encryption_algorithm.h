/*----------------------------------------------------------------------------------------------------------
author:		zhangxiaobing
created:	2013/12/10
-----------------------------------------------------------------------------------------------------------*/
#ifndef _MHXY_ENCRYPTION_ALGORITHM_H_
#define	_MHXY_ENCRYPTION_ALGORITHM_H_
#ifdef __cplusplus
extern "C" 
{
#endif

#ifdef _SUPPORT_MHXY

#include "utility/define.h"

struct tagENCRYPTION_ALGORITHM;

/**
@purpose    : 创建密钥
@param	data: 用于创建密钥的数据
@param	len : 用于创建密钥的数据的长度
@param  key	: 返回密钥
@param  key_len : 返回密钥长度
@return		: 是否创建成功
*/
typedef BOOL (*fun_encryption_algorithm_create_key)(struct tagENCRYPTION_ALGORITHM *p_algorithm, const _u8 *data, _u32 len, void *key, _u32 *p_key_len );

/**
@purpose			: 加密需要发送的数据 
@param	io_buffer	: 数据
@param  len			: 数据的长度
@return				: 返回结果值
*/
typedef BOOL (*fun_encryption_algorithm_encrypt)(struct tagENCRYPTION_ALGORITHM *p_algorithm, _u8 *in_buffer, _u32 len );

/**
@purpose			: 将收到的数据解密
@param	io_buffer	: 数据
@param  len			: 数据的长度
@return				: 返回结果值
*/
typedef BOOL (*fun_encryption_algorithm_decrypt)(struct tagENCRYPTION_ALGORITHM *p_algorithm, _u8 *in_buffer, _u32 len );

typedef struct tagENCRYPTION_ALGORITHM
{
    fun_encryption_algorithm_create_key create_key;
    fun_encryption_algorithm_encrypt encrypt;
    fun_encryption_algorithm_decrypt decrypt;

    _u8 _key[12];
    _u32 _key_len;
    _u32 _key_pos;

}ENCRYPTION_ALGORITHM;    

void encryption_algorithm_destroy(struct tagENCRYPTION_ALGORITHM ** pp_algorithm);

#endif //#ifdef _SUPPORT_MHXY


#ifdef __cplusplus
}
#endif 
#endif //_MHXY_ENCRYPTION_ALGORITHM_H_

