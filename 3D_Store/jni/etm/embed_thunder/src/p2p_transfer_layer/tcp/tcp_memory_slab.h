#ifndef _TCP_MEMORY_SLAB_H_
#define	_TCP_MEMORY_SLAB_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include "p2p_transfer_layer/tcp/tcp_device.h"

_int32 tcp_init_memory_slab(void);

_int32 tcp_uninit_memory_slab(void);

_int32 tcp_malloc_tcp_device(TCP_DEVICE** pointer);

_int32 tcp_free_tcp_device(TCP_DEVICE* pointer);

#ifdef __cplusplus
}
#endif

#endif

