/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/7/6
-----------------------------------------------------------------------------------------------------------*/
#ifndef _UDT_DEVICE_MANAGER_H_
#define _UDT_DEVICE_MANAGER_H_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "p2p_transfer_layer/udt/udt_device.h"

_int32	conn_id_comparator(void *E1, void *E2);

_int32	udt_init_device_manager(void);

_int32	udt_uninit_device_manager(void);

_int32	udt_add_device(UDT_DEVICE* udt);

_int32	udt_remove_device(UDT_DEVICE* udt);

UDT_DEVICE*	udt_find_device(const CONN_ID* id);
#ifdef __cplusplus
}
#endif
#endif


