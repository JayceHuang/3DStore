#include "utility/mempool.h"
#include "utility/settings.h"
#include "utility/define_const_num.h"

#include "ptl_memory_slab.h"

#include "ptl_get_peersn.h"
#include "ptl_udp_device.h"
#include "udt/udt_impl.h"

#if defined(_EXTENT_MEM_FROM_OS) && defined(MEMPOOL_1M) 
static	_u32	g_udp_buffer_left = UDP_BUFFER_SLAB_SIZE*6;
static	_u32	g_min_udp_buffer_num = 10;
#else
static	_u32	g_udp_buffer_left = UDP_BUFFER_SLAB_SIZE;
static	_u32	g_min_udp_buffer_num = 10;
#endif

static BOOL g_recv_udp_package = FALSE;
static SLAB* g_get_peersn_data_slab = NULL;	/*type is GET_PEERSN_DATA*/
static SLAB* g_peersn_cache_data_slab = NULL;/*type is PEERSN_CACHE_DATA*/
static SLAB* g_udp_buffer_slab = NULL;		/*type is char*/

_int32 ptl_init_memory_slab()
{
	_int32 ret = SUCCESS;
	settings_get_int_item("ptl_setting.max_udp_buffer_num", (_int32*)&g_udp_buffer_left);
	settings_get_int_item("ptl_setting.min_udp_buffer_num", (_int32*)&g_min_udp_buffer_num);
	ret = mpool_create_slab(sizeof(GET_PEERSN_DATA), GET_PEERSN_DATA_SLAB_SIZE, 0, &g_get_peersn_data_slab);
	CHECK_VALUE(ret);
	ret = mpool_create_slab(sizeof(PEERSN_CACHE_DATA), PEERSN_CACHE_DATA_SLAB_SIZE, 0, &g_peersn_cache_data_slab);
	CHECK_VALUE(ret);
	ret = mpool_create_slab(UDP_BUFFER_SIZE, UDP_BUFFER_SLAB_SIZE, 0, &g_udp_buffer_slab);
	CHECK_VALUE(ret);
	return ret;
}

_int32 ptl_uninit_memory_slab()
{
	_int32 ret;
	ret = mpool_destory_slab(g_udp_buffer_slab);
	CHECK_VALUE(ret);
	g_udp_buffer_slab = NULL;
	ret = mpool_destory_slab(g_peersn_cache_data_slab);
	CHECK_VALUE(ret);
	g_peersn_cache_data_slab = NULL;
	ret = mpool_destory_slab(g_get_peersn_data_slab);
	CHECK_VALUE(ret);
	g_get_peersn_data_slab = NULL;
	return ret;
}

_int32	ptl_malloc_get_peersn_data(GET_PEERSN_DATA** pointer)
{
	return mpool_get_slip(g_get_peersn_data_slab, (void**)pointer);
}

_int32	ptl_free_get_peersn_data(GET_PEERSN_DATA* pointer)
{
	return mpool_free_slip(g_get_peersn_data_slab, pointer);
}

_int32	ptl_malloc_peersn_cache_data(PEERSN_CACHE_DATA** pointer)
{
	return mpool_get_slip(g_peersn_cache_data_slab, (void**)pointer);
}

_int32	ptl_free_peersn_cache_data(PEERSN_CACHE_DATA* pointer)
{
	return mpool_free_slip(g_peersn_cache_data_slab, pointer);
}

void ptl_set_recv_udp_package()
{
	g_recv_udp_package = TRUE;
}

_int32 ptl_malloc_udp_buffer(char** pointer)
{
	_int32 ret = -1;
	if (g_udp_buffer_left == 0)
	{
	    return ret;
	}
	
	ret = mpool_get_slip(g_udp_buffer_slab, (void**)pointer);
	CHECK_VALUE(ret);
	--g_udp_buffer_left;
	if (g_udp_buffer_left < g_min_udp_buffer_num)
	{
	    udt_set_udp_buffer_low(TRUE);
	}
	
	return ret;
}

_int32 ptl_free_udp_buffer(char* pointer)
{
	mpool_free_slip(g_udp_buffer_slab, pointer);
	++g_udp_buffer_left;
	if (g_udp_buffer_left >= g_min_udp_buffer_num)
	{
	    udt_set_udp_buffer_low(FALSE);
	}
	
	if (g_recv_udp_package == TRUE)
	{
		ptl_udp_recvfrom();
		g_recv_udp_package = FALSE;
	}
	return SUCCESS;
}

