

/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/09/16
-----------------------------------------------------------------------------------------------------------*/
#ifndef _EMULE_QUERY_DEVICE_H_
#define _EMULE_QUERY_DEVICE_H_

#include "utility/define.h"
#ifdef ENABLE_EMULE

struct tagEMULE_TASK;

typedef struct tagEMULE_QUERY_DEVICE
{
	struct tagEMULE_TASK* _emule_task;
}EMULE_QUERY_DEVICE;

_int32 emule_start_query_device(EMULE_QUERY_DEVICE* device);
#endif /* ENABLE_EMULE */

#endif



