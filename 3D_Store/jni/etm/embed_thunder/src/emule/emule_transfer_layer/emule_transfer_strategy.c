#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL
#include "emule_transfer_strategy.h"
#include "emule_traverse_by_svr.h"
#include "utility/mempool.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_EMULE
#include "utility/logger.h"

_int32 emule_transfer_create(EMULE_TRANSFER_STRATEGY** transfer_strategy, EMULE_TRANSFER_METHOD method, void* user_data)
{
	_int32 ret = SUCCESS;
	ret = sd_malloc(sizeof(EMULE_TRANSFER_STRATEGY), (void**)transfer_strategy);
	CHECK_VALUE(ret);
	(*transfer_strategy)->_method = method;
	switch(method)
	{
		case TRANSFER_BY_SERVER:
			ret = emule_traverse_by_svr_create((EMULE_TRANSFER_BY_SERVER**)&(*transfer_strategy)->_transfer, user_data);
			break;
		default:
			sd_assert(FALSE);
	}
	if(ret != SUCCESS)
	{
		sd_free(*transfer_strategy);
		transfer_strategy = NULL;
	}
	return ret;
}

_int32 emule_transfer_close(EMULE_TRANSFER_STRATEGY* transfer_strategy)
{
	_int32 ret = SUCCESS;
	switch(transfer_strategy->_method)
	{
		case TRANSFER_BY_SERVER:
			ret = emule_traverse_by_svr_close((EMULE_TRANSFER_BY_SERVER*)transfer_strategy->_transfer);
			break;
		default:
			sd_assert(FALSE);
	}
	return sd_free(transfer_strategy);
}

_int32 emule_transfer_start(EMULE_TRANSFER_STRATEGY* transfer_strategy)
{
	switch(transfer_strategy->_method)
	{
		case TRANSFER_BY_SERVER:
			return emule_traverse_by_svr_start((EMULE_TRANSFER_BY_SERVER*)transfer_strategy->_transfer);
		default:
			sd_assert(FALSE);
	}
	return SUCCESS;
}

_int32 emule_transfer_recv(EMULE_TRANSFER_STRATEGY* transfer_strategy, _u8 opcode, _u32 ip, _u16 port)
{
	switch(transfer_strategy->_method)
	{
		case TRANSFER_BY_SERVER:
			return emule_traverse_by_svr_recv((EMULE_TRANSFER_BY_SERVER*)transfer_strategy->_transfer, opcode, ip, port);
		default:
			sd_assert(FALSE);
	}
	return SUCCESS;
}

#endif /* ENABLE_EMULE */
#endif /* ENABLE_EMULE */

