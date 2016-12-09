/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/11/27
-----------------------------------------------------------------------------------------------------------*/
#ifndef _EMULE_RC4_H_
#define _EMULE_RC4_H_

#include "utility/define.h"

typedef struct tagRC4_KEY
{
	_u8 _aby_state[256];
	_u8 _byX;
	_u8 _byY;
}RC4_KEY;

_int32 rc4_init_key(RC4_KEY* key, const _u8* data, _u32 data_len);

void rc4_crypt(const _u8* in, _u8* out, _u32 len, RC4_KEY* key);

_int32 rc4_decrypt(const char* in, _u32 in_len, const char** out, _u32* out_len, const void* key);

_int32 rc4_encrypt(const char* in, _u32 in_len, const char** out, _u32* out_len, const void* key);

#endif

