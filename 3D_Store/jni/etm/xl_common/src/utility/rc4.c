#include "utility/rc4.h"
#include "utility/errcode.h"

void rc4_swap_byte (_u8* a, _u8* b)
{
	_u8 by_swap;
	by_swap = *a;
	*a = *b;
	*b = by_swap;
}

void rc4_crypt(const _u8* in, _u8* out, _u32 len, RC4_KEY* key)
{
	_u8 byX = key->_byX;
	_u8 byY = key->_byY;
	_u8* paby_state = &key->_aby_state[0];
	_u8 byXorIndex;
	_u32 i;

	sd_assert(key != NULL);    
	for( i  = 0; i < len; ++i)
	{
		byX = (byX + 1) % 256;
		byY = (paby_state[byX] + byY) % 256;
		rc4_swap_byte(&paby_state[byX], &paby_state[byY]);
		byXorIndex = (paby_state[byX] + paby_state[byY]) % 256;
		if (in != NULL)
			out[i] = in[i] ^ paby_state[byXorIndex];
	}
	key->_byX = byX;
	key->_byY = byY;
}

_int32 rc4_init_key(RC4_KEY* key, const _u8* data, _u32 data_len)
{
	_u8 index1, index2;
	_u8* paby_state;
	_int32 i  = 0;
	paby_state = &key->_aby_state[0];
	for( i = 0; i < 256; ++i)
		paby_state[i] = (_u8)i;
	key->_byX = 0;
	key->_byY = 0;
	index1 = 0;
	index2 = 0;
	for( i = 0; i < 256; ++i)
	{
		index2 = (data[index1] + paby_state[i] + index2) % 256;
		rc4_swap_byte(&paby_state[i], &paby_state[index2]);
		index1 = (_u8)((index1 + 1) % data_len);
	}
	rc4_crypt(NULL, NULL, 1024, key);
	return SUCCESS;
}

_int32 rc4_decrypt(const char* in, _u32 in_len, const char** out, _u32* out_len, const void* key)
{
	rc4_crypt((const _u8*)in,(_u8*)in, in_len, (RC4_KEY*)key);
	*out = in;
	*out_len = in_len;
	return SUCCESS;
}

_int32 rc4_encrypt(const char* in, _u32 in_len, const char** out, _u32* out_len, const void* key)
{
	rc4_crypt((const _u8*)in, (_u8*)in, in_len, (RC4_KEY*)key);
	*out = in;
	*out_len = in_len;
	return SUCCESS;
}



