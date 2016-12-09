/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/12/14
-----------------------------------------------------------------------------------------------------------*/
#ifndef _EMULE_TRANSFER_STRATEGY_H_
#define _EMULE_TRANSFER_STRATEGY_H_

#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL

typedef enum tagEMULE_TRANSFER_METHOD
{
	TRANSFER_BY_SERVER = 0, TRANSFER_BY_SOURCE_EXCHANGE, TRANSFER_END
}EMULE_TRANSFER_METHOD;

typedef struct tagEMULE_TRANSFER_STRATEGY
{
	EMULE_TRANSFER_METHOD		_method;
	void*						_transfer;
}EMULE_TRANSFER_STRATEGY;

_int32 emule_transfer_create(EMULE_TRANSFER_STRATEGY** transfer_strategy, EMULE_TRANSFER_METHOD method, void* user_data);

_int32 emule_transfer_close(EMULE_TRANSFER_STRATEGY* transfer_strategy);

_int32 emule_transfer_start(EMULE_TRANSFER_STRATEGY* transfer_strategy);

_int32 emule_transfer_recv(EMULE_TRANSFER_STRATEGY* transfer_strategy, _u8 opcode, _u32 ip, _u16 port);

#endif
#endif /* ENABLE_EMULE */
#endif


