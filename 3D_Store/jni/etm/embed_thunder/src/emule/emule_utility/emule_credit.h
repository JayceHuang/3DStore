/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/12/25
-----------------------------------------------------------------------------------------------------------*/
#ifndef _EMULE_CREDIT_H_
#define _EMULE_CREDIT_H_
#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL


#include "rsa/rsaeuro.h"

typedef struct tagEMULE_CREDIT_INFO
{
	R_RSA_PUBLIC_KEY	_public_key;
	R_RSA_PRIVATE_KEY	_private_key;
}EMULE_CREDIT_INFO;

_int32 emule_create_private_key(void);
#endif

#endif /* ENABLE_EMULE */

#endif

