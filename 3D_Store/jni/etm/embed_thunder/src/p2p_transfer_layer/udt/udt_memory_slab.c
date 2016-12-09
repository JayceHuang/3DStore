#include "udt_memory_slab.h"
#include "utility/mempool.h"

static	SLAB*	g_udt_device_slab = NULL;		/*type is UDT_DEVICE*/
static	SLAB*	g_udt_send_buffer_slab = NULL;	/*type is UDT_SEND_BUFFER*/
static	SLAB*	g_udt_recv_buffer_slab = NULL;	/*type is UDT_RECV_BUFFER*/

_int32 udt_init_memory_slab()
{
	_int32 ret;
	ret = mpool_create_slab(sizeof(UDT_DEVICE), UDT_DEVICE_SLAB_SIZE, 0, &g_udt_device_slab);
	CHECK_VALUE(ret);
	ret = mpool_create_slab(sizeof(UDT_SEND_BUFFER), UDT_SEND_BUFFER_SLAB_SIZE, 0, &g_udt_send_buffer_slab);
	CHECK_VALUE(ret);
	ret = mpool_create_slab(sizeof(UDT_RECV_BUFFER), UDT_RECV_BUFFER_SLAB_SIZE, 0, &g_udt_recv_buffer_slab);
	CHECK_VALUE(ret);
	return ret;
}

_int32 udt_uninit_memory_slab()
{
	_int32 ret;
	ret = mpool_destory_slab(g_udt_recv_buffer_slab);
	CHECK_VALUE(ret);
	g_udt_recv_buffer_slab = NULL;
	ret = mpool_destory_slab(g_udt_send_buffer_slab);
	CHECK_VALUE(ret);
	g_udt_send_buffer_slab = NULL;
	ret = mpool_destory_slab(g_udt_device_slab);
	CHECK_VALUE(ret);
	g_udt_device_slab = NULL;
	return ret;
}

_int32	udt_malloc_udt_device(UDT_DEVICE** pointer)
{
	return mpool_get_slip(g_udt_device_slab, (void**)pointer);
}

_int32	udt_free_udt_device(UDT_DEVICE* pointer)
{
	return mpool_free_slip(g_udt_device_slab, pointer);
}

_int32 udt_malloc_udt_send_buffer(UDT_SEND_BUFFER** pointer)
{
	return mpool_get_slip(g_udt_send_buffer_slab, (void**)pointer);
}

_int32 udt_free_udt_send_buffer(UDT_SEND_BUFFER* pointer)
{
	sd_memset(pointer,0x00,sizeof(UDT_SEND_BUFFER));
	return mpool_free_slip(g_udt_send_buffer_slab, pointer);
}

_int32	udt_malloc_udt_recv_buffer(UDT_RECV_BUFFER** pointer)
{
	return mpool_get_slip(g_udt_recv_buffer_slab, (void**)pointer);
}

_int32	udt_free_udt_recv_buffer(UDT_RECV_BUFFER* pointer)
{
	return mpool_free_slip(g_udt_recv_buffer_slab, pointer);
}

