/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2008/11/4
-----------------------------------------------------------------------------------------------------------*/
#ifndef	_UDT_MEMORY_SLAB_H_
#define	_UDT_MEMORY_SLAB_H_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "p2p_transfer_layer/udt/udt_device.h"

_int32 udt_init_memory_slab(void);

_int32 udt_uninit_memory_slab(void);

_int32 udt_malloc_udt_device(UDT_DEVICE** pointer);

_int32 udt_free_udt_device(UDT_DEVICE* pointer);	

_int32 udt_malloc_udt_send_buffer(UDT_SEND_BUFFER** pointer);

_int32 udt_free_udt_send_buffer(UDT_SEND_BUFFER* pointer);

_int32 udt_malloc_udt_recv_buffer(UDT_RECV_BUFFER** pointer);

_int32 udt_free_udt_recv_buffer(UDT_RECV_BUFFER* pointer);
#ifdef __cplusplus
}
#endif
#endif

