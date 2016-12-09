/*----------------------------------------------------------------------------------------------------------
author:		zhangxiaobing
created:	2013/12/10
-----------------------------------------------------------------------------------------------------------*/
#ifndef _MHXY_ENCRYPTION_ALGORITHM_1_H_
#define	_MHXY_ENCRYPTION_ALGORITHM_1_H_
#ifdef __cplusplus
extern "C" 
{
#endif

#ifdef _SUPPORT_MHXY

#include "utility/define.h"

//BOOL encryption_algorithm_create_key_1(ENCRYPTION_ALGORITHM *p_algorithm, const _u8 *data, _u32 len, void *p_key, _u32 *key_len );
//BOOL encryption_algorithm_encrypt_1(ENCRYPTION_ALGORITHM *p_algorithm, _u8 *in_buffer, _u32 len );
//BOOL encryption_algorithm_decrypt_1(ENCRYPTION_ALGORITHM *p_algorithm, _u8 *in_buffer, _u32 len );

struct tagENCRYPTION_ALGORITHM;
BOOL encryption_algorithm_1_new(struct tagENCRYPTION_ALGORITHM ** pp_algorithm);

#endif //#ifdef _SUPPORT_MHXY

#ifdef __cplusplus
}
#endif 
#endif //_MHXY_ENCRYPTION_ALGORITHM_1_H_


