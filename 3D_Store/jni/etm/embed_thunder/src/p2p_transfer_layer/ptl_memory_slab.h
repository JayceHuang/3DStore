/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2008/10/10
-----------------------------------------------------------------------------------------------------------*/
#ifndef _PTL_MEMORY_SLAB_H_
#define _PTL_MEMORY_SLAB_H_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"

struct tagGET_PEERSN_DATA;
struct tagPEERSN_CACHE_DATA;

_int32 ptl_init_memory_slab(void);

_int32 ptl_uninit_memory_slab(void);

_int32 ptl_malloc_get_peersn_data(struct tagGET_PEERSN_DATA** pointer);

_int32 ptl_free_get_peersn_data(struct tagGET_PEERSN_DATA* pointer);	

_int32 ptl_malloc_peersn_cache_data(struct tagPEERSN_CACHE_DATA** pointer);

_int32 ptl_free_peersn_cache_data(struct tagPEERSN_CACHE_DATA* pointer);

void ptl_set_recv_udp_package(void);

_int32 ptl_malloc_udp_buffer(char** pointer);

_int32 ptl_free_udp_buffer(char* pointer);
#ifdef __cplusplus
}
#endif
#endif

