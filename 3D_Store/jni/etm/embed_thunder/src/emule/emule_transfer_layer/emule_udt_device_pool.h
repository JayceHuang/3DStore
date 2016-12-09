/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/12/3
-----------------------------------------------------------------------------------------------------------*/
#ifndef _EMULE_UDT_DEVICE_POOL_H_
#define _EMULE_UDT_DEVICE_POOL_H_
#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL

#include "emule/emule_transfer_layer/emule_udt_device.h"

_int32 emule_udt_device_pool_init(void);

_int32 emule_udt_device_pool_uninit(void);

_int32 emule_udt_device_add(EMULE_UDT_DEVICE* udt_device);

_int32 emule_udt_device_remove(EMULE_UDT_DEVICE* udt_device);

EMULE_UDT_DEVICE* emule_udt_device_find_by_conn_id(_u8 conn_id[CONN_ID_SIZE]);

EMULE_UDT_DEVICE* emule_udt_device_find(_u32 ip, _u16 port, _u32 conn_num);

#endif
#endif /* ENABLE_EMULE */
#endif


