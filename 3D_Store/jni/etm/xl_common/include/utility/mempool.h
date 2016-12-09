#ifndef SD_MEMPOOL_H_00138F8F2E70_200806111942
#define SD_MEMPOOL_H_00138F8F2E70_200806111942

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"

#define MPAGE_SIZE	(4096)
#define MIN_MPAGE_COUNT	(2)
#define ALIGN_SIZE	(4)

#define MIN_SLIP_SIZE		(4)
#define MIN_SLIP_SIZE_INDEX	(2)

/* log2(MPAGE_SIZE/MIN_SLIP_SIZE) */
#define LOW_INDEX_COUNT		(10)

#define SLAB_INDEX_EOF		(NULL)

/* size of starndard slab's address-prefix */
#define SSLAB_ADDR_PREFIX_SIZE	(4)
#define SSLAB_AGILN_SIZE (MPAGE_SIZE + SSLAB_ADDR_PREFIX_SIZE)




/* collect allocate info */
#ifdef VERSION_FOR_VALGRIND
#define DEFAULT_MALLOC	(1)
#else
#define DEFAULT_MALLOC	(0)
#endif

void prn_alloc_info(void);


/* initialize mempool with default parameter */
_int32 default_mpool_init(void);

_int32 default_mpool_uninit(void);

/* standard slip-size: 
 *    <  MPAGE_SIZE:  2^n * MIN_SLIP_SIZE
 *    >= MPAGE_SIZE:  n * MPAGE_SIZE
 *
 * stardard_slip_count[i]*slip-size:  either 0 or bigger than MPAGE_SIZE
 */
_int32 mpool_init(_u32 page_count, _int32 stardard_slab_count, _u16 *stardard_slip_count);


/* custom slab */
typedef struct {
	_u32 _slip_size;
	char *_free_index;
#ifdef _USE_GLOBAL_MEM_POOL
       char* _bottom;
#endif
	

#if DEFAULT_MALLOC
	_u32 _used_count;
	_u32 _max_used_count;
	_u32 _min_slip_count;
	char _ref_file[64];
	_int32 _ref_line;
#endif

} SLAB, *pSLAB;


/* standard slip-size: 
 *    <  MPAGE_SIZE:  2^n * MIN_SLIP_SIZE
 *    >= MPAGE_SIZE:  n * MPAGE_SIZE
 */
typedef struct {
	char *_standard_slab_low_base;
	char *_standard_slab_high_base;
	char *_extend_data_base;
	char *_extend_data_bottom; /* this value must < _custom_slab_cur_data*/

	char *_custom_slab_cur_data;   /* address that is bigger than this value has been used */

	char *_standard_slab_index;
	char *_extend_slab_index;

	char *_extend_data_index_bottom; /* this value must < _custom_slab_cur_index*/

	/* alloc inverse */
	SLAB *_custom_slab_cur_index;

	_int32 _stardard_slab_count;

	#ifdef _EXTENT_MEM_FROM_OS
        char* _mempool_bottom; 
	#endif
} MEMPOOL;


/***************************
 *    Slab
 ***************************/

#if (DEFAULT_MALLOC || defined(_MEM_DEBUG))
#define mpool_create_slab(slip_size, min_slip_count, invalid_offset, ppslab) mpool_create_slab_impl(slip_size, min_slip_count, invalid_offset, __FILE__, __LINE__, ppslab)
_int32 mpool_create_slab_impl(_u32 slip_size, _u32 min_slip_count, _u32 invalid_offset, const char* ref_file, _int32 line, SLAB **slab);

#define mpool_get_slip(slab, slip) mpool_get_slip_impl(slab, __FILE__, __LINE__, slip)
_int32 mpool_get_slip_impl(SLAB *slab, const char* ref_file, _int32 line, void **slip);

#define mpool_free_slip(slab, slip) mpool_free_slip_impl(slab, slip, __FILE__, __LINE__)
_int32 mpool_free_slip_impl(SLAB *slab, void *slip, const char* ref_file, _int32 line);

#else
/* sizeof(slip) - invalid_offset >= 4, invalid_offset be not used now */
_int32 mpool_create_slab(_u32 slip_size, _u32 min_slip_count, _u32 invalid_offset, SLAB **slab);
_int32 mpool_get_slip(SLAB *slab, void **slip);
_int32 mpool_free_slip(SLAB *slab, void *slip);
#endif

_int32 mpool_destory_slab(SLAB *slab);



/***************************
 *    Variable-size mempool
 ***************************/
#if (DEFAULT_MALLOC || defined(_MEM_DEBUG))
#define sd_malloc(memsize, mem) sd_malloc_impl(memsize, __FILE__, __LINE__, mem)
_int32 sd_malloc_impl(_u32 memsize, const char* ref_file, _int32 line, void **mem);

#define sd_free(mem) sd_free_impl(mem, __FILE__, __LINE__)
_int32 sd_free_impl(void *mem, const char* ref_file, _int32 line);

#else
_int32 sd_malloc(_u32 memsize, void **mem);
_int32 sd_free(void *mem);
#endif



/************************************************************************/
/*    Custom mempool                                                    */
/************************************************************************/

_int32 create_custom_mpool(_u32 page_count, _int32 stardard_slab_count, _u16 *stardard_slip_count, MEMPOOL **mpool);

#ifdef _MEM_DEBUG
_int32 sd_custom_mpool_malloc(MEMPOOL *mpool, _u32 memsize, const char* ref_file, _int32 line, void **mem);
_int32 sd_custom_mpool_free(MEMPOOL *mpool, void *mem, const char* ref_file, _int32 line);
#else
_int32 sd_custom_mpool_malloc(MEMPOOL *mpool, _u32 memsize, void **mem);
_int32 sd_custom_mpool_free(MEMPOOL *mpool, void *mem);
#endif



/************************************************************************/
/*      Simple Fixed mpool                                              */
/************************************************************************/

#define FIXED_MPOOL_INDEX_EOF	(-1)

#define DECL_FIXED_MPOOL(type, size) \
		static type g_fixed_mpool[size];\
		static _int32 g_fixed_mpool_free_idx;\
		static _int32 __init_fixed_mpool(void){ \
			_int32 index = 0;\
			g_fixed_mpool_free_idx = 0;\
			for(index = 0; index < size - 1; index++)\
			{\
				*(_int32*)(g_fixed_mpool + index) = index + 1;\
			}\
			*(_int32*)(g_fixed_mpool + index) = FIXED_MPOOL_INDEX_EOF;\
			return SUCCESS;\
		}\
		static _int32 __get_fixed_mpool_data(type **data)\
		{\
			if(g_fixed_mpool_free_idx == FIXED_MPOOL_INDEX_EOF)\
				return OUT_OF_FIXED_MEMORY;\
			*data = g_fixed_mpool + g_fixed_mpool_free_idx;\
			g_fixed_mpool_free_idx = *(_int32*)(g_fixed_mpool + g_fixed_mpool_free_idx);\
			return SUCCESS;\
		}\
		static _int32 __free_fixed_mpool_data(type *data)\
		{\
			if(data > g_fixed_mpool + size - 1 || data < g_fixed_mpool)\
				return INVALID_MEMORY;\
			*(_int32*)data = g_fixed_mpool_free_idx;\
			g_fixed_mpool_free_idx = data - g_fixed_mpool;\
			return SUCCESS;\
		}


#define INIT_FIXED_MPOOL				__init_fixed_mpool
#define GET_FIEXED_MPOOL_DATA(ppdata)	__get_fixed_mpool_data(ppdata)
#define FREE_FIXED_MPOOL_DATA(pdata)	__free_fixed_mpool_data(pdata)

#define MALLOC_ALIGN_8_BYTES(size)  ((size)%8)==0?(size):(((size/8)+1)*8)

/************************************************************************/
/*      Some macro                                                      */
/************************************************************************/
 
#define	SAFE_DELETE(p)	{if(p) {sd_free(p);(p) = NULL;}}

#ifdef __cplusplus
}
#endif
#endif
