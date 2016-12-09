#ifndef EM_MEMPOOL_H_00138F8F2E70_200806111942
#define EM_MEMPOOL_H_00138F8F2E70_200806111942

#include "em_common/em_define.h"

#include "utility/mempool.h"

#define  em_mpool_create_slab  mpool_create_slab
#define  em_mpool_get_slip  mpool_get_slip
#define  em_mpool_free_slip mpool_free_slip
#define  em_mpool_destory_slab   mpool_destory_slab  
#define   em_malloc   sd_malloc
#define   em_free  sd_free


/************************************************************************/
/*      Simple Fixed mpool                                              */
/************************************************************************/

#define EM_FIXED_MPOOL_INDEX_EOF	(-1)

#define EM_DECL_FIXED_MPOOL(type, size) \
		static type g_em_fixed_mpool[size];\
		static _int32 g_em_fixed_mpool_free_idx;\
		static _int32 __em_init_fixed_mpool(void){ \
			_int32 index = 0;\
			g_em_fixed_mpool_free_idx = 0;\
			for(index = 0; index < size - 1; index++)\
			{\
				*(_int32*)(g_em_fixed_mpool + index) = index + 1;\
			}\
			*(_int32*)(g_em_fixed_mpool + index) = EM_FIXED_MPOOL_INDEX_EOF;\
			return SUCCESS;\
		}\
		static _int32 __em_get_fixed_mpool_data(type **data)\
		{\
			if(g_em_fixed_mpool_free_idx == EM_FIXED_MPOOL_INDEX_EOF)\
				return OUT_OF_FIXED_MEMORY;\
			*data = g_em_fixed_mpool + g_em_fixed_mpool_free_idx;\
			g_em_fixed_mpool_free_idx = *(_int32*)(g_em_fixed_mpool + g_em_fixed_mpool_free_idx);\
			return SUCCESS;\
		}\
		static _int32 __em_free_fixed_mpool_data(type *data)\
		{\
			if(data > g_em_fixed_mpool + size - 1 || data < g_em_fixed_mpool)\
				return INVALID_MEMORY;\
			*(_int32*)data = g_em_fixed_mpool_free_idx;\
			g_em_fixed_mpool_free_idx = data - g_em_fixed_mpool;\
			return SUCCESS;\
		}


#define EM_INIT_FIXED_MPOOL				__em_init_fixed_mpool
#define EM_GET_FIEXED_MPOOL_DATA(ppdata)	__em_get_fixed_mpool_data(ppdata)
#define EM_FREE_FIXED_MPOOL_DATA(pdata)	__em_free_fixed_mpool_data(pdata)



/************************************************************************/
/*      Some macro                                                      */
/************************************************************************/
 
#define	EM_SAFE_DELETE(p)	{if(p) {em_free(p);(p) = NULL;}}

#endif
