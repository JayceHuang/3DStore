#include "utility/define.h"
#include "utility/define_const_num.h"
#include "utility/mempool.h"
#include "utility/errcode.h"

#include "tcp_memory_slab.h"

static SLAB* g_tcp_device_slab = NULL;

_int32 tcp_init_memory_slab()
{
	_int32 ret = SUCCESS;
	ret = mpool_create_slab(sizeof(TCP_DEVICE), TCP_DEVICE_SLAB_SIZE, 0, &g_tcp_device_slab);
	CHECK_VALUE(ret);
	return ret;
}

_int32 tcp_uninit_memory_slab()
{
	_int32 ret;
	ret = mpool_destory_slab(g_tcp_device_slab);
	CHECK_VALUE(ret);
	g_tcp_device_slab = NULL;
	return ret;
}

_int32 tcp_malloc_tcp_device(TCP_DEVICE** pointer)
{
	return mpool_get_slip(g_tcp_device_slab, (void**)pointer);
}

_int32 tcp_free_tcp_device(TCP_DEVICE* pointer)
{
	return mpool_free_slip(g_tcp_device_slab, pointer);
}

