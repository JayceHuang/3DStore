#include "utility/define.h"
#ifdef ENABLE_BT 

#include "bt_memory_slab.h"
#include "bt_device.h"
#include "bt_data_pipe.h"
#include "../bt_data_manager/bt_tmp_file.h"
#include "utility/mempool.h"
#include "utility/define_const_num.h"
#include "utility/sd_assert.h"

static	SLAB*	g_range_change_node_slab = NULL;/*type is RANGE_CHANGE_NODE*/
#ifdef ENABLE_BT_PROTOCOL

static	SLAB*	g_bt_resource_slab = NULL;		/*type is BT_RESOURCE*/
static	SLAB*	g_bt_data_pipe_slab = NULL;		/*type is BT_DATA_PIPE*/
static	SLAB*	g_bt_device_slab = NULL;		/*type is BT_DEVICE*/
static	SLAB*	g_bt_piece_slab = NULL;			/*type is BT_PIECE*/
#endif


_int32 init_bt_memory_slab()
{
	_int32 ret;
#ifdef ENABLE_BT_PROTOCOL
	ret = mpool_create_slab(sizeof(BT_RESOURCE), BT_RESOURCE_SLAB_SIZE, 0, &g_bt_resource_slab);
	CHECK_VALUE(ret);
	ret = mpool_create_slab(sizeof(BT_DATA_PIPE), BT_DATA_PIPE_SLAB_SIZE, 0, &g_bt_data_pipe_slab);
	CHECK_VALUE(ret);
	ret = mpool_create_slab(sizeof(BT_DEVICE), BT_DEVICE_SLAB_SIZE, 0, &g_bt_device_slab);
	CHECK_VALUE(ret);
	ret = mpool_create_slab(sizeof(BT_PIECE_DATA), BT_PIECE_SLAB_SIZE, 0, &g_bt_piece_slab);
	CHECK_VALUE(ret);
#endif
    
	ret = mpool_create_slab(sizeof(RANGE_CHANGE_NODE), RANGE_CHANGE_NODE_SLAB_SIZE, 0, &g_range_change_node_slab);
	return ret;
}

_int32 uninit_bt_memory_slab()
{
	_int32 ret;
	ret = mpool_destory_slab(g_range_change_node_slab);
	CHECK_VALUE(ret);
	g_range_change_node_slab = NULL;
    
#ifdef ENABLE_BT_PROTOCOL
	ret = mpool_destory_slab(g_bt_piece_slab);
	CHECK_VALUE(ret);
	g_bt_piece_slab = NULL;
	ret = mpool_destory_slab(g_bt_device_slab);
	CHECK_VALUE(ret);
	g_bt_device_slab = NULL;
	ret = mpool_destory_slab(g_bt_data_pipe_slab);
	CHECK_VALUE(ret);
	g_bt_data_pipe_slab = NULL;
	ret = mpool_destory_slab(g_bt_resource_slab);
	g_bt_resource_slab = NULL;
#endif
	return ret;
}

#ifdef ENABLE_BT_PROTOCOL

_int32 bt_malloc_bt_resource(struct tagBT_RESOURCE** pointer)
{
	return mpool_get_slip(g_bt_resource_slab, (void**)pointer);
}

_int32 bt_free_bt_resource(struct tagBT_RESOURCE* pointer)
{
	return mpool_free_slip(g_bt_resource_slab, pointer);
}

_int32 bt_malloc_bt_data_pipe(struct tagBT_DATA_PIPE** pointer)
{
	return mpool_get_slip(g_bt_data_pipe_slab, (void**)pointer);
}

_int32 bt_free_bt_data_pipe(struct tagBT_DATA_PIPE* pointer)
{
	return mpool_free_slip(g_bt_data_pipe_slab, pointer);
}

_int32 bt_malloc_bt_device(BT_DEVICE** pointer)
{
	return mpool_get_slip(g_bt_device_slab, (void**)pointer);
}

_int32 bt_free_bt_device(BT_DEVICE* pointer)
{
	return mpool_free_slip(g_bt_device_slab, pointer);
}

_int32 bt_malloc_bt_piece_data(BT_PIECE_DATA** pointer)
{
	return mpool_get_slip(g_bt_piece_slab, (void**)pointer);
}

_int32 bt_free_bt_piece_data(BT_PIECE_DATA* pointer)
{
	return mpool_free_slip(g_bt_piece_slab, pointer);
}
#endif

_int32 bt_malloc_range_change_node(struct tagRANGE_CHANGE_NODE** pointer)
{
	return mpool_get_slip(g_range_change_node_slab, (void**)pointer);
}

_int32 bt_free_range_change_node(struct tagRANGE_CHANGE_NODE* pointer)
{
	return mpool_free_slip(g_range_change_node_slab, pointer);
}

#endif

