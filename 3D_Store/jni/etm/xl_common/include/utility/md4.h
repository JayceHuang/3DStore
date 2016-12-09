/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/09/29
-----------------------------------------------------------------------------------------------------------*/
#ifndef _MD4_H_
#define _MD4_H_

#include "utility/define.h"

typedef struct tagCTX_MD4
{
	_u32	total[2];     /*!< number of bytes processed  */
	_u32	state[4];     /*!< intermediate digest state  */
    	_u8		buffer[64];   /*!< data block being processed */
	_u8		ipad[64];     /*!< HMAC: inner padding        */
	_u8		opad[64];     /*!< HMAC: outer padding        */
}CTX_MD4;

void md4_initialize(CTX_MD4* ctx);

void md4_update(CTX_MD4* ctx, const _u8* input, _u32 len);

void md4_finish(CTX_MD4* ctx, _u8 output[16]);

#endif
