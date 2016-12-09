#include "utility/mempool.h"
#include "utility/sd_assert.h"

#include "p2p_memory_slab.h"

#include "p2p_data_pipe.h"
#include "p2p_socket_device.h"

static	SLAB*	g_p2p_resource_slab = NULL;		/*type is P2P_RESOURCE*/
static	SLAB*	g_p2p_data_pipe_slab = NULL;	/*type is P2P_DATA_PIPE*/
static	SLAB*	g_socket_device_slab = NULL;/*type is SOCKET_DEVICE*/
static	SLAB*	g_recv_cmd_buffer_slab = NULL;	/*type is char*/
static	SLAB*	g_range_slab = NULL;			/*type is RANGE*/
#ifdef UPLOAD_ENABLE
static	SLAB*	g_p2p_upload_data_slab = NULL;	/*type is P2P_UPLOAD_DATA*/
#endif

_int32 init_p2p_memory_slab()
{
	_int32 ret;
	ret = mpool_create_slab(sizeof(P2P_RESOURCE), P2P_RESOURCE_SLAB_SIZE, 0, &g_p2p_resource_slab);
	CHECK_VALUE(ret);
	ret = mpool_create_slab(sizeof(P2P_DATA_PIPE), P2P_DATA_PIPE_SLAB_SIZE, 0, &g_p2p_data_pipe_slab);
	CHECK_VALUE(ret);
	ret = mpool_create_slab(sizeof(SOCKET_DEVICE), SOCKET_DEVICE_SLAB_SIZE, 0, &g_socket_device_slab);
	CHECK_VALUE(ret);
	ret = mpool_create_slab(RECV_CMD_BUFFER_LEN, RECV_CMD_BUFFER_LEN_SLAB_SIZE, 0, &g_recv_cmd_buffer_slab);
	CHECK_VALUE(ret);
	ret = mpool_create_slab(sizeof(RANGE), RANGE_SLAB_SIZE, 0, &g_range_slab);
	CHECK_VALUE(ret);
#ifdef UPLOAD_ENABLE
	ret = mpool_create_slab(sizeof(P2P_UPLOAD_DATA), P2P_UPLOAD_DATA_SLAB_SIZE, 0, &g_p2p_upload_data_slab);
	CHECK_VALUE(ret);
#endif
	return ret;
}

_int32 uninit_p2p_memory_slab()
{
	_int32 ret;
#ifdef UPLOAD_ENABLE
	ret = mpool_destory_slab(g_p2p_upload_data_slab);
	CHECK_VALUE(ret);
#endif
	ret = mpool_destory_slab(g_range_slab);
	CHECK_VALUE(ret);
	g_range_slab = NULL;
	ret = mpool_destory_slab(g_recv_cmd_buffer_slab);
	CHECK_VALUE(ret);
	g_recv_cmd_buffer_slab = NULL;
	ret = mpool_destory_slab(g_socket_device_slab);
	CHECK_VALUE(ret);
	g_socket_device_slab = NULL;
	ret = mpool_destory_slab(g_p2p_data_pipe_slab);
	CHECK_VALUE(ret);
	g_p2p_data_pipe_slab = NULL;
	ret = mpool_destory_slab(g_p2p_resource_slab);
	CHECK_VALUE(ret);
	g_p2p_resource_slab = NULL;
	return ret;
}

_int32 p2p_malloc_p2p_data_pipe(P2P_DATA_PIPE** pointer)
{
	return mpool_get_slip(g_p2p_data_pipe_slab, (void**)pointer);	
}

_int32	p2p_free_p2p_data_pipe(P2P_DATA_PIPE* pointer)
{
	return mpool_free_slip(g_p2p_data_pipe_slab, pointer);
}

_int32	p2p_malloc_p2p_resource(P2P_RESOURCE** pointer)
{
	return mpool_get_slip(g_p2p_resource_slab, (void**)pointer);
}

_int32	p2p_free_p2p_resource(P2P_RESOURCE* pointer)
{
	return mpool_free_slip(g_p2p_resource_slab, pointer);
}

_int32	p2p_malloc_socket_device(SOCKET_DEVICE** pointer)
{
	return mpool_get_slip(g_socket_device_slab, (void**)pointer);
}

_int32	p2p_free_socket_device(SOCKET_DEVICE* pointer)
{
	return mpool_free_slip(g_socket_device_slab, pointer);
}

_int32	p2p_malloc_recv_cmd_buffer(char** pointer)
{
	_int32 ret;
	ret = mpool_get_slip(g_recv_cmd_buffer_slab, (void**)pointer);
	sd_assert(ret == SUCCESS && *pointer != NULL);
	return ret;
}

_int32	p2p_free_recv_cmd_buffer(char* pointer)
{
	return mpool_free_slip(g_recv_cmd_buffer_slab, pointer);
}

_int32	p2p_malloc_range(RANGE** pointer)
{
	return mpool_get_slip(g_range_slab, (void**)pointer);
}

_int32	p2p_free_range(RANGE* pointer)
{
	return mpool_free_slip(g_range_slab, pointer);
}

#ifdef UPLOAD_ENABLE
_int32 p2p_malloc_p2p_upload_data(P2P_UPLOAD_DATA** pointer)
{
	return mpool_get_slip(g_p2p_upload_data_slab, (void**)pointer);
}

_int32 p2p_free_p2p_upload_data(P2P_UPLOAD_DATA* pointer)
{
	return mpool_free_slip(g_p2p_upload_data_slab, pointer);
}
#endif

