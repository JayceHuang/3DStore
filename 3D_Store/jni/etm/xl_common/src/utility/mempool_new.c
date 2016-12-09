#include "utility/define.h"
#ifdef _USE_NEW_MEMPOOL

#include "utility/errcode.h"
#include "utility/string.h"
#include "utility/mempool.h"
#include "platform/sd_mem.h"

#if defined(LINUX) && defined(_MEM_DEBUG)
#include <signal.h>
#include <execinfo.h>
#endif
#define LOGID LOGID_MEMPOOL
#include "utility/logger.h"

/*-----------------------------------------------------------------------------
 *  extend memory allocation
 *-----------------------------------------------------------------------------*/

struct mem_extend_chunk {
	struct mem_extend_chunk *prev;
	struct mem_extend_chunk *next;
}; /* ----------  end of struct mem_chunk  ---------- */

typedef struct mem_extend_chunk chunk_t;


/*-----------------------------------------------------------------------------
 *  extend memory pool struction define
 *-----------------------------------------------------------------------------*/
struct mem_extend_pool {
	_u32 total;
	_u8* bitmap;
	_u32* header;
	chunk_t* free_list;
	char* address;
}; /* ----------  end of struct mem_pool  ---------- */

typedef struct mem_extend_pool pool_t;


#define		CHUNK_SIZE	1024			/*  */

_u8 mark[8]={1,2,4,8,16,32,64,128};

#define setbit(x,y) (x)|=(mark[y]) 	//set the bit value to 1 in X with offset Y
#define clrbit(x,y) (x)&=(~mark[y]) 	//clear the bit value to 0 in X with offset Y
/*-----------------------------------------------------------------------------
 * Misc function to manipulating map and chunk !
 *-----------------------------------------------------------------------------*/
static __inline__ _int32 idxchunk( _int32 addr) 
{
	return addr >> 10;
}

static __inline__ _int32 chunkidx(_int32 idx) 
{
	return idx << 10;
}

static __inline__ _int32 idxmap( pool_t* mp, char* chunk) 
{
	if (chunk > mp->address)
		return (chunk - mp->address);
	else
		return 0;
}

static __inline__ char* mapidx(pool_t* mp, _int32 idx) 
{
	return mp->address + (idx << 10);
}

 /* BOOL setbit(_u8 x, _u32 y) 
 { 

        LOG_DEBUG( "setbit :x: %u,  y :%u" , (_u32)x ,(_u32)y);
 	x |= mark[y]; 
 	return SUCCESS; 
 } 

  BOOL clrbit(_u8 x, _u32 y) 
{ 
     LOG_DEBUG( "clrbit :x: %u,  y :%u" , (_u32)x ,(_u32)y);
 	x &= (~ mark[y]); 
 	return SUCCESS; 
 } */

void show_bitmap(pool_t* mp);
static __inline__ BOOL chkbit(pool_t *mp, _u32 idx, _u32 offset) 
{
	_u8 target = mp->bitmap[idx]; 
	BOOL flag = (mp->bitmap[idx] > (target ^ mark[offset]))?TRUE:FALSE;
	return flag;
}

/*-----------------------------------------------------------------------------
 *  platform define of memory pool manager
 *-----------------------------------------------------------------------------*/
pool_t* mpool_extend_create( pool_t* extend_base, _u32 size);
_int32 mpool_extend_destory(pool_t *mp_p);
_int32 mpool_extend_init(pool_t *mp_p);

_int32 set_bitmap ( pool_t *mp, _u32 index, _u32 total, BOOL flag);
_int32 set_all_bitmap ( pool_t *mp, _u32 index, _u32 total);
BOOL check_bitmap ( pool_t *mp, _u32 index );

_int32 set_chunk_size(pool_t *mp, _u32 index, _u32 value);
_int32 get_chunk_size(pool_t *mp, _u32 index);
char* get_prev_free_chunk ( pool_t *mp, chunk_t* start );

char* get_chunk(pool_t *mp, _int32 chunk_size);
_int32 free_chunk(pool_t *mp,  char* address);


/* implement of mempool */
MEMPOOL *g_mpool = NULL;
static _int32 inuse = 0;

static _u32 get_size(_u32 index)
{
	if(index <= LOW_INDEX_COUNT)
		return MIN_SLIP_SIZE * (1 << index);
	else
		return MPAGE_SIZE * (index + 1 - LOW_INDEX_COUNT);
}

static _u32 get_index(_u32 memsize)
{
	_u32 index = 0;

	if(memsize > MPAGE_SIZE)
	{
		/* (memsize - 1) / MPAGE_SIZE + LOW_INDEX_COUNT */
		index = ((memsize - 1) >> 12) + LOW_INDEX_COUNT;
	}
	else
	{
		/* (memsize - 1) / MIN_SLIP_SIZE */
		memsize = (memsize - 1) >> 2;
		while(memsize)
		{
			index ++;
			memsize >>= 1;
		}
	}

	return index;
}

#if (DEFAULT_MALLOC || defined(_MEM_DEBUG))

#include <time.h>
#include <malloc.h>
#include "utility/map.h"

#define BACKTRACK_STACK_DEPTH  (10)
#define STACK_INFO_LEN  (128)

/* mem check */
typedef struct {
	_u32 _address;
	_u32 _memsize;
	_u16 _status; /* 0: free   1: malloc */
	_u16 _ref_line;
	char _ref_file[64];
	_u32 _alloc_time; /* seconds from epoch*/
	_u32 _other_data;
	char _stackframe[BACKTRACK_STACK_DEPTH][STACK_INFO_LEN];
	_int32 _stack_depth;
}MEM_CHECK_INFO;

#ifdef STACK_FRAME_DEPTH

#define GET_STACK_FRAME(meminfo)  {\
	void * stack_frame[BACKTRACK_STACK_DEPTH];\
	_int32 stack_depth = 0, stack_idx;\
	char **stack_info = NULL;\
	stack_depth = backtrace(stack_frame, BACKTRACK_STACK_DEPTH);\
	stack_info = backtrace_symbols(stack_frame, stack_depth);\
	for(stack_idx = 0; stack_idx < stack_depth; stack_idx++)\
	{\
		sd_strncpy((meminfo)._stackframe[stack_idx], stack_info[stack_idx], STACK_INFO_LEN);\
		(meminfo)._stackframe[stack_idx][STACK_INFO_LEN - 1] = 0;\
	}\
	(meminfo)._stack_depth = stack_depth;\
	sd_free_mem_to_os(stack_info, 0);\
}
#else

#define GET_STACK_FRAME(meminfo)  (meminfo)._stack_depth = 0;

#endif

static SET g_memcheck_set;

_int32 memcheck_comparator(void *E1, void *E2)
{
	return ((MEM_CHECK_INFO*)E1)->_address - ((MEM_CHECK_INFO*)E2)->_address;
}

static void init_memcheck(void)
{
	set_init(&g_memcheck_set, memcheck_comparator);
}

static _u32 check_mem(MEM_CHECK_INFO *check_info)
{
	SET_ITERATOR it_find;
	SET_NODE *pnode = NULL;
	MEM_CHECK_INFO *pinfo = NULL;
	_u32 ret_val = 0;

	set_find_iterator(&g_memcheck_set, check_info, &it_find);
	if(it_find == SET_END(g_memcheck_set))
	{
		sd_assert(check_info->_status == 1);

		sd_get_mem_from_os(sizeof(MEM_CHECK_INFO), (void**)&pinfo);
		sd_memcpy(pinfo, check_info, sizeof(MEM_CHECK_INFO));

		sd_get_mem_from_os(sizeof(SET_NODE), (void**)&pnode);
		pnode->_data = pinfo;
		set_insert_setnode(&g_memcheck_set, pnode);
	}
	else
	{
		pinfo = SET_DATA(it_find);
		sd_assert(pinfo->_status != check_info->_status);
		ret_val = pinfo->_memsize;
		sd_memcpy(pinfo, check_info, sizeof(MEM_CHECK_INFO));
	}

	return ret_val;
}

static void prn_left_node(void)
{
	SET_ITERATOR it;
	MEM_CHECK_INFO *pinfo = NULL;
	char * free_index;
	_int32 stack_idx = 0;

	LOG_ERROR("------------------------ Memory Allocated -----------------------");
	for(it = SET_BEGIN(g_memcheck_set); it != SET_END(g_memcheck_set); it = SET_NEXT(g_memcheck_set, it))
	{
		pinfo = SET_DATA(it);


		if(pinfo->_status == 1)
		{

#if !(DEFAULT_MALLOC)
			/* print type-info */

			if(pinfo->_address < (_u32)g_mpool->_standard_slab_low_base)
			{
				/* invalid memory */
				LOG_ERROR("mem-type: INVALID MEMORY !!!");
			}
			else if(pinfo->_address < (_u32)g_mpool->_standard_slab_high_base)
			{
				free_index = *(char**)(g_mpool->_standard_slab_low_base + ((char*)pinfo->_address - g_mpool->_standard_slab_low_base) / SSLAB_AGILN_SIZE * SSLAB_AGILN_SIZE);
				LOG_ERROR("mem-type: low-sslab(idx:%d)", (free_index - g_mpool->_standard_slab_index) / sizeof(char*));
			}
			else if(pinfo->_address < (_u32)g_mpool->_extend_data_base)
			{
				free_index = *(char**)(pinfo->_address - SSLAB_ADDR_PREFIX_SIZE);
				LOG_ERROR("mem-type: high-sslab(idx:%d)", (free_index - g_mpool->_standard_slab_index) / sizeof(char*));
			}
			else if(pinfo->_address < (_u32)g_mpool->_custom_slab_cur_data)
			{/* extend buffer */
				LOG_ERROR("mem-type: extend-mem");
			}
			else
			{
				/* assume memory-block in legal page, so do not judge the mem < all-page-bottom */
				LOG_ERROR("mem-type: cslab-mem");
			}

#endif

			LOG_ERROR("addr: 0x%X, size: %d, status: %s, other_data: 0x%X", 
					pinfo->_address, pinfo->_memsize, (pinfo->_status == 1 ? "used":"free"), pinfo->_other_data);
			LOG_ERROR("ref_file: %s, ref_line: %d, ref_time: %u(%s)",
					pinfo->_ref_file, pinfo->_ref_line, pinfo->_alloc_time, ctime((const time_t*)&pinfo->_alloc_time));

			LOG_ERROR("call stack:");
			for(stack_idx = 0; stack_idx <pinfo->_stack_depth; stack_idx++)
			{
				LOG_ERROR("%d:  %s", stack_idx, pinfo->_stackframe[stack_idx]);
			}

			LOG_ERROR("-------------------------");

		}
	}
}

#endif

#if DEFAULT_MALLOC


typedef struct {
	_int32 _total;
	_int32 _current;
	_int32 _count;
	_int32 _max;
	_u32 _total_waste;
} ALLOC_INFO;

#define MAX_SLAB_COUNT	(128)

static SLAB *g_slab_list[MAX_SLAB_COUNT];
static _int32 g_current_slab_idx = 0;

static ALLOC_INFO *g_alloc_info = NULL;
static _int32 g_alloc_info_size = 0;

void prn_alloc_info(void)
{
	_int32 index = 0;

	LOG_ERROR("begin statics:===============");
	LOG_ERROR("malloc statics: \n");
	for(index = 0; index < g_alloc_info_size; index++)
	{
		if(g_alloc_info[index]._max > 0)
		{
			LOG_ERROR("[index: %d size: %d] count: %d, avg: %d, current: %d, max: %d, total waste bytes: %d", 
					index, get_size(index), g_alloc_info[index]._count, 
					g_alloc_info[index]._total / g_alloc_info[index]._count, g_alloc_info[index]._current,
					g_alloc_info[index]._max, g_alloc_info[index]._total_waste);
		}
	}

	LOG_ERROR("SLAB statics: \n");
	for(index = 0; index < MAX_SLAB_COUNT && g_slab_list[index]; index++)
	{
		LOG_ERROR("-------------------------- SLAB %d --------------------------", index);
		LOG_ERROR("SLAB: 0x%X, ref_file:%s, ref_line: %d", g_slab_list[index], g_slab_list[index]->_ref_file, g_slab_list[index]->_ref_line);
		LOG_ERROR("slip_size: %d, reserved_slip: %d, cur_slip_count: %d, max_slip_count: %d\n",
				g_slab_list[index]->_slip_size, g_slab_list[index]->_min_slip_count,
				g_slab_list[index]->_used_count, g_slab_list[index]->_max_used_count);
	}

	LOG_ERROR("\n");
	prn_left_node();
}

_int32 mpool_init(_u32 page_count, _int32 stardard_slab_count, _u16 *stardard_slip_count)
{
	_int32 ret_val = SUCCESS;
	ret_val = sd_get_mem_from_os(sizeof(ALLOC_INFO) * stardard_slab_count, (void**)&g_alloc_info);
	CHECK_VALUE(ret_val);

	sd_memset(g_alloc_info, 0, sizeof(ALLOC_INFO) * stardard_slab_count);
	g_alloc_info_size = stardard_slab_count;

	sd_memset(g_slab_list, 0, MAX_SLAB_COUNT * sizeof(SLAB*));

	init_memcheck();
	return SUCCESS;
}

_int32 mpool_uninit(_u32 page_count)
{
	return SUCCESS;
}

_int32 mpool_create_slab_impl(_u32 slip_size, _u32 min_slip_count, _u32 invalid_offset, const char* ref_file, _int32 line, SLAB **slab)
{
	_int32 ret_val = SUCCESS;
	_int32 index=0,temp=0,total=0;

	ret_val = sd_get_mem_from_os(sizeof(SLAB), (void**)slab);
	CHECK_VALUE(ret_val);

	sd_memset(*slab, 0, sizeof(SLAB));
	(*slab)->_slip_size = slip_size;

	(*slab)->_used_count = 0;
	(*slab)->_max_used_count = 0;
	(*slab)->_min_slip_count = min_slip_count;

	sd_strncpy((*slab)->_ref_file, ref_file, sizeof((*slab)->_ref_file));
	(*slab)->_ref_line = line;

	if(g_current_slab_idx < MAX_SLAB_COUNT)
	{
		g_slab_list[g_current_slab_idx++] = *slab;
	}
	else
	{
		LOG_DEBUG("too many slab be created, can not record");

		LOG_DEBUG("malloc a slab(0x%x) on mem(0x%x) of slip_size(%d) at %s:%d", *slab, slab, slip_size, ref_file, line);
		// locked memory pool area in bitmap which has been allocate with slab 
	}

	return ret_val;
}

_int32 mpool_get_slip_impl(SLAB *slab, const char* ref_file, _int32 line, void **slip)
{
	_int32 ret_val = SUCCESS;
	MEM_CHECK_INFO meminfo;

	GET_STACK_FRAME(meminfo);

	ret_val = sd_get_mem_from_os(slab->_slip_size, slip);
	CHECK_VALUE(ret_val);

	slab->_used_count ++;
	if(slab->_used_count > slab->_max_used_count)
		slab->_max_used_count = slab->_used_count;

	LOG_DEBUG("get slip(0x%x) on addr(0x%x) from slab(0x%x %s:%d) at %s:%d", *slip, slip, slab, slab->_ref_file, slab->_ref_line, ref_file, line);

	meminfo._address = (_u32)*slip;
	meminfo._memsize = slab->_slip_size;
	meminfo._status = 1;
	sd_strncpy(meminfo._ref_file, ref_file, sizeof(meminfo._ref_file));
	meminfo._ref_line = (_u16)line;
	meminfo._alloc_time = time(NULL);
	meminfo._other_data = (_u32)slab;
	check_mem(&meminfo);

	return ret_val;
}

_int32 mpool_free_slip_impl(SLAB *slab, void *slip, const char* ref_file, _int32 line)
{
	_int32 ret_val = SUCCESS;
	MEM_CHECK_INFO meminfo;

	//GET_STACK_FRAME(meminfo);
	(meminfo)._stack_depth = 0;

	sd_assert(slip > 0);

	ret_val = sd_free_mem_to_os(slip, slab->_slip_size);
	CHECK_VALUE(ret_val);

	slab->_used_count --;

	LOG_DEBUG("free slip(0x%x) from slab(0x%x %s:%d) at %s:%d", slip, slab, slab->_ref_file, slab->_ref_line, ref_file, line);

	meminfo._address = (_u32)slip;
	meminfo._memsize = slab->_slip_size;
	meminfo._status = 0;
	sd_strncpy(meminfo._ref_file, ref_file, sizeof(meminfo._ref_file));
	meminfo._ref_line = (_u16)line;
	meminfo._alloc_time = time(NULL);
	meminfo._other_data = (_u32)slab;
	check_mem(&meminfo);

	return ret_val;
}

_int32 mpool_destory_slab(SLAB *slab)
{
	_int32 ret_val = SUCCESS;
	/*
	   ret_val = sd_free_mem_to_os(slab, sizeof(SLAB));
	   CHECK_VALUE(ret_val);
	   */
	return ret_val;
}


_int32 sd_malloc_impl(_u32 memsize, const char* ref_file, _int32 line, void **mem)
{
	_int32 ret_val = SUCCESS;
	_u32 index = get_index(memsize);
	MEM_CHECK_INFO meminfo;

	GET_STACK_FRAME(meminfo);

	ret_val = sd_get_mem_from_os(get_size(index), mem);
	CHECK_VALUE(ret_val);

	/* record alloc info */

	if(index >= (_u32)g_alloc_info_size)
	{
		LOG_ERROR("alloc large memory(size:%d, idx:%d) bigger than standard slab, can not record this info!", memsize, index);
	}
	else
	{
		g_alloc_info[index]._current ++;
		g_alloc_info[index]._total += g_alloc_info[index]._current;
		if(g_alloc_info[index]._current > g_alloc_info[index]._max)
			g_alloc_info[index]._max = g_alloc_info[index]._current;
		g_alloc_info[index]._count ++;
		g_alloc_info[index]._total_waste += (get_size(index) - memsize);
	}

	meminfo._address = (_u32)*mem;
	meminfo._memsize = memsize;
	meminfo._status = 1;
	sd_strncpy(meminfo._ref_file, ref_file, sizeof(meminfo._ref_file));
	meminfo._ref_line = (_u16)line;
	meminfo._alloc_time = time(NULL);
	meminfo._other_data = (_u32)index;
	check_mem(&meminfo);

	LOG_DEBUG("malloc a new mem(0x%x) on mem(0x%x) of size(%d) at %s:%d", *mem, mem, memsize, ref_file, line);
	return ret_val;
}

_int32 sd_free_impl(void *mem, const char* ref_file, _int32 line)
{
	_int32 ret_val = SUCCESS;
	_u32 index = 0, size = 0;
	MEM_CHECK_INFO meminfo;
	//GET_STACK_FRAME(meminfo);
	(meminfo)._stack_depth = 0;
	sd_assert(mem > 0);

	/* alloc info */
#ifdef WINCE
	size = _msize(mem);
	index = get_index(size);
#endif

#ifdef LINUX
	/*
	   malloc_usable_size(mem) = ((size - 5) / 8) * 8 + 12;
	   */
	/*
	   size = malloc_usable_size(mem) - 4;
	   index = get_index(size);
	   */
#endif


	meminfo._address = (_u32)mem;
	meminfo._memsize = 0;
	meminfo._status = 0;
	sd_strncpy(meminfo._ref_file, ref_file, sizeof(meminfo._ref_file));
	meminfo._ref_line = (_u16)line;
	meminfo._alloc_time = time(NULL);
	meminfo._other_data = (_u32)index;

	size = check_mem(&meminfo);
	index = get_index(size);

	ret_val = sd_free_mem_to_os(mem, 0);
	CHECK_VALUE(ret_val);

	if(index >= (_u32)g_alloc_info_size)
	{
		LOG_ERROR("alloc large memory(size:%d, idx:%d) bigger than standard slab, can not record this info!", size, index);
	}
	else
	{
		g_alloc_info[index]._current --;
	}

	LOG_DEBUG("free a mem(0x%x) of size(%d) at %s:%d", mem, size, ref_file, line);

	return ret_val;
}

/* end of collect info */
#else


#ifdef _MEM_DEBUG

typedef struct {
	_int32 _init_count;
	_int32 _cur_count;
	_int32 _other_count;
	_u32 _wasted_bytes;
}SSLAB_INFO;

typedef struct {
	SLAB *_slab_addr;
	_int32 _init_slip_count;
	_int32 _cur_slip_count;
	char _ref_file[64];
	_int32 _ref_line;
}CSLAB_INFO;

static SSLAB_INFO *g_sslab_info;
static SET g_cslab_info_set;

#ifdef LINUX

static BOOL g_need_prn_info;
void signal_handle(_int32 sign)
{
	LOG_ERROR("received a signal, ready to prn mem-info");
	g_need_prn_info = TRUE;
}

#endif

void prn_info_when_need(void)
{
#ifdef LINUX
	if(g_need_prn_info)
		prn_alloc_info();

	g_need_prn_info = FALSE;
#endif
}

_int32 cslab_info_comparator(void *E1, void *E2)
{
	return ((CSLAB_INFO*)E1)->_slab_addr - ((CSLAB_INFO*)E2)->_slab_addr;
}

static void init_mpool_info_recorder(_u16 *stardard_slip_count)
{
	_int32 idx = 0;
	sd_get_mem_from_os(g_mpool->_stardard_slab_count * sizeof(SSLAB_INFO), (void**)&g_sslab_info);

	for(idx = 0; idx < g_mpool->_stardard_slab_count; idx++)
	{
		g_sslab_info[idx]._init_count = (_int32)stardard_slip_count[idx];
		g_sslab_info[idx]._cur_count = 0;
		g_sslab_info[idx]._other_count = 0;
		g_sslab_info[idx]._wasted_bytes = 0;
	}

	init_memcheck();

	set_init(&g_cslab_info_set, cslab_info_comparator);

#ifdef LINUX
	signal(SIGUSR1, signal_handle);
#endif
}


#endif


const _u32 align_mask = ~(ALIGN_SIZE - 1);

#define ALIGN(pmem)	((pmem) = (char*)(((_u32)(pmem) + ALIGN_SIZE - 1) & align_mask))

void prn_alloc_info(void)
{
#ifdef _MEM_DEBUG
	_int32 idx = 0;
	SET_ITERATOR it;
	CSLAB_INFO *pinfo = NULL;

	LOG_ERROR("------------- SSLAB INFO ------------");
	for(idx = 0; idx < g_mpool->_stardard_slab_count; idx++)
	{
		if(g_sslab_info[idx]._init_count != 0 || g_sslab_info[idx]._cur_count != 0)
		{
			LOG_ERROR("idx: %d, size: %d, init-count: %d, used-count: %d, used-for-other: %d, wasted-bytes: %u"
					, idx, get_size(idx), g_sslab_info[idx]._init_count, g_sslab_info[idx]._cur_count, g_sslab_info[idx]._other_count, g_sslab_info[idx]._wasted_bytes);
		}
	}		
	LOG_DEBUG("extend buffer:   0x%x - 0x%x = %d", g_mpool->_custom_slab_cur_data, g_mpool->_extend_data_bottom, g_mpool->_custom_slab_cur_data - g_mpool->_extend_data_bottom);

	LOG_ERROR("-------- CSLAB INFO ------------");
	for(it = SET_BEGIN(g_cslab_info_set); it != SET_END(g_cslab_info_set); it = SET_NEXT(g_cslab_info_set, it))
	{
		pinfo = SET_DATA(it);
		LOG_ERROR("SLAB: 0x%X, ref_file:%s, ref_line: %d", pinfo->_slab_addr, pinfo->_ref_file, pinfo->_ref_line);
		LOG_ERROR("slip_size: %d, reserved_slip: %d, cur_slip_count: %d\n", ((SLAB*)pinfo->_slab_addr)->_slip_size, pinfo->_init_slip_count, pinfo->_cur_slip_count);
	}


	prn_left_node();
#endif	
}

static void init_custom_slab_data(char *buffer, _int32 slip_size, _int32 slip_count)
{
	_int32 i = 0;

	sd_assert(!((_u32)buffer % ALIGN_SIZE));

	for(i = 0; i < slip_count - 1; i++)
	{
		*(char**)buffer = buffer + slip_size;
		buffer += slip_size;
	}
	*(char**)(buffer) = SLAB_INDEX_EOF;
}

/* Return: the address of list-head */
static char* init_standard_slab_data(char **buffer, _int32 slip_size, _u16 slip_count, char *slab_index)
{
	_int32 i = 0;
	_u32 block_size = slip_size;
	char *ret_val = NULL, *pre_buf = NULL;

	*(char**)(*buffer) = slab_index;
	*buffer += SSLAB_ADDR_PREFIX_SIZE;
	ret_val = pre_buf = *buffer;

	for(i = 0; i < slip_count - 1; i++)
	{
		*buffer += slip_size;
		if(block_size >= MPAGE_SIZE)
		{
			*(char**)(*buffer) = slab_index;
			*buffer += SSLAB_ADDR_PREFIX_SIZE;
			block_size = slip_size;
		}
		else
			block_size += slip_size;

		*(char**)pre_buf = *buffer;
		pre_buf = *buffer;
	}

	/* the last one */
	*(char**)(*buffer) = SLAB_INDEX_EOF;
	*buffer += slip_size;

	return ret_val;
}

_int32 create_custom_mpool(_u32 page_count, _int32 stardard_slab_count, _u16 *stardard_slip_count, MEMPOOL **mpool)
{
	_int32 ret_val = SUCCESS;
	char *pidx = NULL, *pdata = NULL;
	_u32 slip_size = MIN_SLIP_SIZE;
	_u32 size = 0;
	_u16 i = 0;

	if(page_count < MIN_MPAGE_COUNT)
	{
		return TOO_FEW_MPAGE;
	}

	/* alloc mem from os */
	ret_val = sd_get_mem_from_os(page_count * MPAGE_SIZE, (void**)&pidx);
	CHECK_VALUE(ret_val);

	/* pmem must be mpage-aligned */
	sd_assert(!((_u32)pidx % ALIGN_SIZE));

	/* ctl & index put in the first page */
	pdata = pidx + MPAGE_SIZE;

	/* mpool */
	*mpool = (MEMPOOL*)pidx;
	pidx += sizeof(MEMPOOL);
	/* aligned */
	ALIGN(pidx);

	/* set some variables */
	(*mpool)->_standard_slab_index = pidx;
	(*mpool)->_standard_slab_low_base = pdata;

	(*mpool)->_custom_slab_cur_data = pdata + (page_count - 1) * MPAGE_SIZE;
	(*mpool)->_custom_slab_cur_index = (SLAB*)pdata;

	(*mpool)->_standard_slab_high_base = NULL;
	(*mpool)->_stardard_slab_count = stardard_slab_count;

#ifdef _EXTENT_MEM_FROM_OS
	(*mpool)->_mempool_bottom = pdata + (page_count - 1) * MPAGE_SIZE;
#endif


	/* standard slab */
	for(i = 0; i < stardard_slab_count; i++)
	{
		size = slip_size * stardard_slip_count[i];

		if(size > 0)
		{
			/* check: all non-zero size of standard-slab must be bigger than MPAGE_SIZE*/
			if(size < MPAGE_SIZE)
			{
				LOG_ERROR("the size of stardard slab must be bigger than MPAGE_SIZE!\n");
				return INVALID_SSLAB_SIZE;
			}

			/* check room*/
			if((slip_size < MPAGE_SIZE && pdata + size + size / MPAGE_SIZE * SSLAB_ADDR_PREFIX_SIZE > (*mpool)->_custom_slab_cur_data)
					|| (slip_size >= MPAGE_SIZE && pdata + size + SSLAB_ADDR_PREFIX_SIZE * stardard_slip_count[i] > (*mpool)->_custom_slab_cur_data))
			{
				return OUT_OF_MEMORY;
			}
			if(pidx >= (char*)(*mpool)->_custom_slab_cur_index)
			{
				return OUT_OF_MEMORY;
			}

			if(slip_size >= MPAGE_SIZE && !(*mpool)->_standard_slab_high_base)
			{
				(*mpool)->_standard_slab_high_base = pdata;
			}

			*(char**)pidx = init_standard_slab_data(&pdata, slip_size, stardard_slip_count[i], pidx);
		}
		else
			*(char**)pidx = SLAB_INDEX_EOF;

		pidx += sizeof(char*);

		if(slip_size >= MPAGE_SIZE)
			slip_size += MPAGE_SIZE;
		else
			slip_size <<= 1;
	}
	if(!(*mpool)->_standard_slab_high_base)
		(*mpool)->_standard_slab_high_base = pdata;

	/* init extend */
	(*mpool)->_extend_data_base = pdata;
	(*mpool)->_extend_slab_index = pidx;
	(*mpool)->_extend_data_index_bottom = pidx;
	(*mpool)->_extend_data_bottom = (char*)(*mpool) + (page_count * MPAGE_SIZE);
	mpool_extend_create((pool_t*)(*mpool)->_extend_data_base,(*mpool)->_extend_data_bottom - (*mpool)->_extend_data_base); 

	LOG_DEBUG("EXTEND_MEM: start address:0x%x-end0x%x, total:%d btyes, offset:%d\n",(*mpool)->_extend_data_base,(*mpool)->_extend_data_bottom ,(*mpool)->_extend_data_bottom - (*mpool)->_extend_data_base,(page_count*MPAGE_SIZE));

#ifdef _MEM_DEBUG
	init_mpool_info_recorder(stardard_slip_count);
#endif

	return ret_val;
}

static _int32 sd_extend_malloc(MEMPOOL *mpool, _u32 memsize, void **mem)
{
	_u32 total=0,flag=0;

	total = memsize>>10;
	flag = memsize % CHUNK_SIZE;

	if (0 == total && 0 == flag)
		return OUT_OF_MEMORY;
	else
	{
		if(0 != flag)
			total += 1;
	}

//	LOG_DEBUG("bitmap value at begin of sd_extend_malloc");
//	show_bitmap((pool_t *)mpool->_extend_data_base);

	*mem = get_chunk ((pool_t *)mpool->_extend_data_base, total );
	if(NULL == *mem)
		return OUT_OF_MEMORY;

//	LOG_DEBUG("bitmap value at end of sd_extend_malloc");
//	show_bitmap((pool_t *)mpool->_extend_data_base);

	return SUCCESS;

	//    /*check room*/
	//    if(mpool->_extend_data_bottom + size + SSLAB_ADDR_PREFIX_SIZE >= mpool->_custom_slab_cur_data)
	//    {
	//	return OUT_OF_MEMORY;
	//    }
	//
	//    *(char**)mpool->_extend_data_bottom = mpool->_standard_slab_index + index * sizeof(char*);
	//    mpool->_extend_data_bottom += SSLAB_ADDR_PREFIX_SIZE;
	//
	//    *mem = mpool->_extend_data_bottom;
	//    mpool->_extend_data_bottom += size;
	//
}

static _int32 sd_extend_free(MEMPOOL *mpool, char *mem)
{
//	LOG_DEBUG("bitmap value at begin of sd_free_malloc");
//	show_bitmap((pool_t *)mpool->_extend_data_base);

	return free_chunk ( (pool_t*)mpool->_extend_data_base, mem );

	//    char *free_index_head = NULL;
	//    _u32 free_index = 0;
	//    _u32 size = 0;

	//    free_index_head = *(char**)(mem - SSLAB_ADDR_PREFIX_SIZE);
	//    free_index = (free_index_head - mpool->_standard_slab_index) / sizeof(char*);
	//
	//    size = get_size(free_index);
	//
	//    /* simple check */
	//    if(mem + size == mpool->_extend_data_bottom)
	//    {/* can be collected */
	//	mpool->_extend_data_bottom -= (size + SSLAB_ADDR_PREFIX_SIZE);
	//	return SUCCESS;
	//    }
	//
	//    *(char**)mem = *(char**)free_index_head;
	//    *(char**)free_index_head = mem;
	//
//	return SUCCESS;
}

#ifdef _MEM_DEBUG
static _int32 sd_malloc_from_index(MEMPOOL *mpool, _u32 index, _u32 memsize, void **mem, _int32 *res_index)
#else
static _int32 sd_malloc_from_index(MEMPOOL *mpool, _u32 index, _u32 memsize, void **mem)
#endif
{
	char **free_slip = NULL;
	_u32 i = 0, end = 0;

	i = index;
	if(index > LOW_INDEX_COUNT)
		end = index;
	else
		end = LOW_INDEX_COUNT;

	free_slip = (char**)(mpool->_standard_slab_index + index * sizeof(char*));

	for(i = index; i <= end; i++)
	{
		if(*free_slip != SLAB_INDEX_EOF)
		{
			*mem = *free_slip;
			*free_slip = *(char**)(*free_slip);

#ifdef _MEM_DEBUG
			if(*free_slip != SLAB_INDEX_EOF)
				sd_assert(*free_slip <= mpool->_extend_data_bottom && *free_slip >= mpool->_standard_slab_low_base);

			*res_index = i;
#endif

			return SUCCESS;
		}
		free_slip ++;
	}

	if(sd_extend_malloc(mpool, memsize, mem) == SUCCESS)
	{
#ifdef _MEM_DEBUG

#endif
		return SUCCESS;
	}

	/* replace recursive with loop, for case of too deep recursion */
	for(i = end + 1; i < (_u32)mpool->_stardard_slab_count; i++)
	{
		if(*free_slip != SLAB_INDEX_EOF)
		{
			*mem = *free_slip;
			*free_slip = *(char**)(*free_slip);

#ifdef _MEM_DEBUG
			if(*free_slip != SLAB_INDEX_EOF)
				sd_assert(*free_slip <= mpool->_extend_data_bottom && *free_slip >= mpool->_standard_slab_low_base);

			*res_index = i;
#endif

			return SUCCESS;
		}
		free_slip ++;
	}

	return OUT_OF_MEMORY;
}

#ifdef _MEM_DEBUG
_int32 sd_custom_mpool_malloc(MEMPOOL *mpool, _u32 memsize, const char* ref_file, _int32 line, void **mem)
#else
_int32 sd_custom_mpool_malloc(MEMPOOL *mpool, _u32 memsize, void **mem)
#endif
{
	_u32 index = 0;
	_int32 ret_val = SUCCESS;
	/*     _int32 total=0,flag=0; */

#ifdef _MEM_DEBUG
	void *src_mem = *mem;
	MEM_CHECK_INFO meminfo;
	_int32 res_index = -1;

	GET_STACK_FRAME(meminfo);
#endif

	*mem = NULL;
	if(memsize == 0)
		return SUCCESS;

	index = get_index(memsize);
	if(index >= (_u32)mpool->_stardard_slab_count)
	{
		LOG_DEBUG("too big memory required(%d)...", index);
		return OUT_OF_MEMORY;
	}

#ifdef _MEM_DEBUG

	ret_val = sd_malloc_from_index(mpool, index, memsize, mem, &res_index);

	if(ret_val == OUT_OF_MEMORY)
	{
		/* print mem-info */
		LOG_ERROR("out of memory when alloc memsize(%d), ready to prn mem-info", memsize);
		prn_alloc_info();
	}
	else
	{
		meminfo._address = (_u32)*mem;
		meminfo._memsize = memsize;
		meminfo._status = 1;
		sd_strncpy(meminfo._ref_file, ref_file, sizeof(meminfo._ref_file));
		meminfo._ref_line = (_u16)line;
		meminfo._alloc_time = time(NULL);
		if(src_mem == NULL)
			meminfo._other_data = (_u32)index;
		else
			meminfo._other_data = (_u32)src_mem;

		check_mem(&meminfo);

		if(res_index >= 0)
		{
			g_sslab_info[res_index]._cur_count ++;
			if(index != res_index)
			{
				g_sslab_info[res_index]._other_count ++;
			}
			g_sslab_info[res_index]._wasted_bytes += (get_size(res_index) - memsize);
		}
	}
#else
	ret_val = sd_malloc_from_index(mpool, index, memsize,mem);
#endif

#ifdef _EXTENT_MEM_FROM_OS
	if(ret_val != SUCCESS)
		ret_val = sd_get_extent_mem_from_os(memsize, mem);

#endif

	return ret_val;
}

#ifdef _MEM_DEBUG
_int32 sd_custom_mpool_free(MEMPOOL *mpool, void *data, const char* ref_file, _int32 line)
#else
_int32 sd_custom_mpool_free(MEMPOOL *mpool, void *data)
#endif
{
	_int32 ret_val = SUCCESS;
	char *free_index = NULL;
	char *mem = data;

#ifdef _MEM_DEBUG
	MEM_CHECK_INFO meminfo;
	_u32 src_msize = 0, src_index, actual_msize, actual_index;

	//GET_STACK_FRAME(meminfo);
	(meminfo)._stack_depth = 0;
#endif

#ifdef _EXTENT_MEM_FROM_OS
	if(mem<mpool->_standard_slab_low_base || mem>=mpool->_mempool_bottom)
	{
		return sd_free_extent_mem_to_os(mem, 0);
	}
#endif

	/* find its slab */
	if(mem < mpool->_standard_slab_high_base)
	{
		if(mem < mpool->_standard_slab_low_base)
			return INVALID_MEMORY;

		/* can be improved with bit-operator in future */
		free_index = *(char**)(mpool->_standard_slab_low_base + (mem - mpool->_standard_slab_low_base) / SSLAB_AGILN_SIZE * SSLAB_AGILN_SIZE);
	}
	else if(mem < mpool->_extend_data_base)
	{
		free_index = *(char**)(mem - SSLAB_ADDR_PREFIX_SIZE);
	}
	else if(mem < mpool->_custom_slab_cur_data)
	{/* extend buffer */
		ret_val = sd_extend_free(mpool, mem);

		//#ifdef _MEM_DEBUG
		//	meminfo._address = (_u32)data;
		//	meminfo._memsize = 0;
		//	meminfo._status = 0;
		//	sd_strncpy(meminfo._ref_file, ref_file, sizeof(meminfo._ref_file));
		//	meminfo._ref_line = (_u16)line;
		//	meminfo._alloc_time = time(NULL);
		//	meminfo._other_data = -1;
		//
		//	check_mem(&meminfo);
		//#endif

		return ret_val;
	}
	else
		return INVALID_MEMORY;

	if(mem < mpool->_extend_data_base || mem >= mpool->_custom_slab_cur_data)
	{
		*(char**)mem = *(char**)free_index;
		*(char**)free_index = mem;
	}

#ifdef _MEM_DEBUG

	/* cal mem-index */
	actual_index = (free_index - mpool->_standard_slab_index) / sizeof(char*);
	actual_msize = get_size(actual_index);

	meminfo._address = (_u32)data;
	meminfo._memsize = 0;
	meminfo._status = 0;
	sd_strncpy(meminfo._ref_file, ref_file, sizeof(meminfo._ref_file));
	meminfo._ref_line = (_u16)line;
	meminfo._alloc_time = time(NULL);
	meminfo._other_data = -1;

	src_msize = check_mem(&meminfo);
	src_index = get_index(src_msize);

	g_sslab_info[actual_index]._cur_count --;
	if(src_index != actual_index)
	{
		g_sslab_info[actual_index]._other_count --;
		g_sslab_info[actual_index]._wasted_bytes -= (actual_msize - src_msize);
	}

#endif

	return ret_val;
}


/************************************************************************/
/*        DEFAULT MEMPOOL                                               */
/************************************************************************/

#ifdef _MEM_DEBUG
_int32 mpool_create_slab_impl(_u32 slip_size, _u32 min_slip_count, _u32 invalid_offset, const char* ref_file, _int32 line, SLAB **slab)
#else
_int32 mpool_create_slab(_u32 slip_size, _u32 min_slip_count, _u32 invalid_offset, SLAB **slab)
#endif
{
	_int32 ret_val = SUCCESS;
	_u32 index=0,total=0,temp=0;
	_int32 j=0;
	chunk_t *P = NULL;
	pool_t* pool = NULL;

#ifdef _MEM_DEBUG
	CSLAB_INFO *pinfo = NULL;
	SET_NODE *pnode = NULL;
#endif

	/* check room */
	if(g_mpool->_custom_slab_cur_data <= g_mpool->_extend_data_base + slip_size * min_slip_count)
	{
#ifdef _MEM_DEBUG
		prn_alloc_info();
#endif
		return OUT_OF_MEMORY;
	}

	g_mpool->_custom_slab_cur_index -= 1;
	g_mpool->_custom_slab_cur_data -= slip_size * min_slip_count;
	*slab = g_mpool->_custom_slab_cur_index;

	(*slab)->_free_index = g_mpool->_custom_slab_cur_data;
	(*slab)->_slip_size = slip_size;

	pool = (pool_t*)g_mpool->_extend_data_base;
	index = idxchunk(idxmap(pool,(char*)(*slab)->_free_index));	
	total = pool->total - index;

	/* set flag to be used in chunks bitmap. */
	set_all_bitmap(pool,index,total);

//	LOG_DEBUG("current slab more then %d chunks",total);
//	LOG_DEBUG("index of new slab is:%d, address: 0x%x, pool start address is :0x%x",index,(*slab)->_free_index,pool->address);
//	LOG_DEBUG("bitmap value after create slab");
//	show_bitmap(pool);

	for(j = pool->total-1; (j>=0) && (pool->free_list[j].prev == &(pool->free_list[j]));j--);

	temp = pool->total - total - 1 ;
	if(j > temp)
	{	
		P=pool->free_list[j].prev;

		// clear othe old list
		pool->free_list[j].prev = pool->free_list[j].next = &(pool->free_list[j]);

		//mount free space to the new list and remove it from this old one.
		P->prev = pool->free_list[temp].prev;
		P->next = &(pool->free_list[temp]);
		pool->free_list[temp].prev = P;
		P->prev->next = P;

//		P->prev = &(pool->free_list[temp]);
//		P->next = &(pool->free_list[temp]);
//		pool->free_list[temp].prev = pool->free_list[temp].next = P;

		//reset total of  chunks.
		pool->total -= total;

		// reset  order.
		set_chunk_size(pool,0,index);
	}
	else
	{
		// reset  order.
		set_chunk_size(pool,pool->total-j,j-total);

		//reset total of  chunks.
		pool->total -= total;
	}
	
#ifdef  _MEM_DEUBG
		_u32 i=0;
		for ( i=0;i<pool->total ; i++) 
		{
			if(pool->free_list[i].prev != &(pool->free_list[i]))
			{	
				LOG_DEBUG ( "header size:%d, bitmap value:%d\n",pool->header[i],pool->bitmap[i>>3] );
				LOG_DEBUG ( "prev address:0x%x, next address:0x%x of free_list[%d] address: 0x%x,\n",
							pool->free_list[i].prev,pool->free_list[i].next,i,&(pool->free_list[i]) );
			}
		}
#endif     

	/* init current slab after allocation. */
	init_custom_slab_data(g_mpool->_custom_slab_cur_data, slip_size, min_slip_count);

#ifdef _MEM_DEBUG
	/* record cslab-info */

	sd_get_mem_from_os(sizeof(CSLAB_INFO), (void**)&pinfo);

	pinfo->_init_slip_count = min_slip_count;
	pinfo->_cur_slip_count = 0;
	pinfo->_slab_addr = *slab;
	pinfo->_ref_line = line;
	sd_strncpy(pinfo->_ref_file, ref_file, sizeof(pinfo->_ref_file));

	sd_get_mem_from_os(sizeof(SET_NODE), (void**)&pnode);
	pnode->_data = pinfo;
	set_insert_setnode(&g_cslab_info_set, pnode);

	LOG_DEBUG("malloc a slab(0x%x) on mem(0x%x) of slip_size(%d) at %s:%d", *slab, slab, slip_size, ref_file, line);

#endif


	return ret_val;
}

#ifdef _MEM_DEBUG
_int32 mpool_get_slip_impl(SLAB *slab, const char* ref_file, _int32 line, void **slip)
#else
_int32 mpool_get_slip(SLAB *slab, void **slip)
#endif
{
	_int32 ret_val = SUCCESS;

#ifdef _MEM_DEBUG
	MEM_CHECK_INFO meminfo;
	CSLAB_INFO find_node, *result_node = NULL;
	GET_STACK_FRAME(meminfo);
#endif

	if(slab->_free_index != SLAB_INDEX_EOF)
	{
		*slip = slab->_free_index;
		slab->_free_index = *(char**)slab->_free_index;

#ifdef _MEM_DEBUG

		find_node._slab_addr = slab;
		set_find_node(&g_cslab_info_set, &find_node, (void**)&result_node);
		sd_assert(result_node != NULL);
		result_node->_cur_slip_count ++;

		meminfo._address = (_u32)(*slip);
		meminfo._memsize = slab->_slip_size;
		meminfo._status = 1;
		sd_strncpy(meminfo._ref_file, ref_file, sizeof(meminfo._ref_file));
		meminfo._ref_line = (_u16)line;
		meminfo._alloc_time = time(NULL);
		meminfo._other_data = (_u32)slab;

		check_mem(&meminfo);
#endif

	}
	else
	{
#ifdef _MEM_DEBUG
		*slip = slab;
		ret_val = sd_custom_mpool_malloc(g_mpool, slab->_slip_size, ref_file, line, slip);
#else
		ret_val = sd_malloc(slab->_slip_size, slip);
#endif
	}

#ifdef _MEM_DEBUG
	prn_info_when_need();

	LOG_DEBUG("get slip(0x%x) on addr(0x%x) from slab(0x%x) at %s:%d", *slip, slip, slab, ref_file, line);
#endif

	return ret_val;
}

#ifdef _MEM_DEBUG
_int32 mpool_free_slip_impl(SLAB *slab, void *slip, const char* ref_file, _int32 line)
#else
_int32 mpool_free_slip(SLAB *slab, void *slip)
#endif
{
	_int32 ret_val = SUCCESS;

#ifdef _MEM_DEBUG
	MEM_CHECK_INFO meminfo;
	CSLAB_INFO find_node, *result_node = NULL;


	//GET_STACK_FRAME(meminfo);
	(meminfo)._stack_depth = 0;
#endif

#ifdef _EXTENT_MEM_FROM_OS
	if(slip<g_mpool->_standard_slab_low_base || slip>=g_mpool->_mempool_bottom)
	{
		return sd_free_extent_mem_to_os(slip, 0);
	}
#endif

	sd_assert(!((_u32)slip % ALIGN_SIZE));

	if((char*)slip >= g_mpool->_custom_slab_cur_data)
	{
		*(char**)slip = slab->_free_index;
		slab->_free_index = slip;

#ifdef _MEM_DEBUG
		find_node._slab_addr = slab;
		set_find_node(&g_cslab_info_set, &find_node, (void**)&result_node);
		sd_assert(result_node != NULL);
		result_node->_cur_slip_count --;

		meminfo._address = (_u32)(slip);
		meminfo._memsize = slab->_slip_size;
		meminfo._status = 0;
		sd_strncpy(meminfo._ref_file, ref_file, sizeof(meminfo._ref_file));
		meminfo._ref_line = (_u16)line;
		meminfo._alloc_time = time(NULL);
		meminfo._other_data = (_u32)slab;

		check_mem(&meminfo);
#endif

	}
	else
	{/**/
#ifdef _MEM_DEBUG
		ret_val = sd_free_impl(slip, ref_file, line);
#else
		return sd_free(slip);
#endif
	}

#ifdef _MEM_DEBUG
	prn_info_when_need();

	LOG_DEBUG("free slip(0x%x) from slab(0x%x) at %s:%d", slip, slab, ref_file, line);
#endif

	return ret_val;
}

_int32 mpool_destory_slab(SLAB *slab)
{/* to be implemented */
	return SUCCESS;
}

_int32 mpool_init(_u32 page_count, _int32 stardard_slab_count, _u16 *stardard_slip_count)
{
	return create_custom_mpool(page_count, stardard_slab_count, stardard_slip_count, &g_mpool);
}

_int32 mpool_uninit(_u32 page_count)
{
	sd_free_mem_to_os(g_mpool, page_count * MPAGE_SIZE);
	g_mpool = NULL; 
	return SUCCESS;
}

#ifdef _MEM_DEBUG

_int32 sd_malloc_impl(_u32 memsize, const char* ref_file, _int32 line, void **mem)
{
	_int32 ret_val = SUCCESS;

	*mem = NULL;
	ret_val = sd_custom_mpool_malloc(g_mpool, memsize, ref_file, line, mem);

	prn_info_when_need();

	LOG_DEBUG("malloc a new mem(0x%x) on mem(0x%x) of size(%d) at %s:%d", *mem, mem, memsize, ref_file, line);

	return ret_val;
}

_int32 sd_free_impl(void *mem, const char* ref_file, _int32 line)
{
	_int32 ret_val = SUCCESS;
	ret_val = sd_custom_mpool_free(g_mpool, mem, ref_file, line);

	prn_info_when_need();

	LOG_DEBUG("free a mem(0x%x) at %s:%d", mem, ref_file, line);

	return ret_val;
}

#else

_int32 sd_malloc(_u32 memsize, void **mem)
{
	_int32 ret_val = SUCCESS;
	ret_val = sd_custom_mpool_malloc(g_mpool, memsize, mem);
	return ret_val;
}

_int32 sd_free(void *mem)
{
	_int32 ret_val = SUCCESS;
	ret_val = sd_custom_mpool_free(g_mpool, mem);
	return ret_val;
}

#endif

#endif

_int32 default_mpool_init(void)
{
	/* 10M bytes */
	_int32 page_count = 2560;

	/* max pre-alloc size 2M */
	_u16 slip_count[522];

#ifdef  MEMPOOL_3M	 
	page_count = 768;
#endif

#ifdef  MEMPOOL_5M	 
	page_count = 1280;
#endif

#ifdef  MEMPOOL_8M
	page_count = 2048;

#endif

#ifdef  MEMPOOL_10M
	page_count = 2560;
#endif

	/*
#ifdef ENABLE_VOD
page_count += 2560; //add 10 for vod cache buffer
#endif
*/

	/*#ifdef _VOD_NO_DISK   
	  page_count += 2048 ; //add 8M for vod cache buffer without disk
#endif*/

	/*
#ifdef _VOD_NO_DISK_EXTEND   
page_count += VOD_EXTEND_BUFFER ; //add 8M for vod cache buffer without disk
#endif
*/
	sd_memset(slip_count, 0, sizeof(slip_count));

	slip_count[0] = MPAGE_SIZE / 4;
	slip_count[1] = MPAGE_SIZE / 8;
	slip_count[2] = 4 * MPAGE_SIZE / 16;
	slip_count[3] = 16 * MPAGE_SIZE / 32;
	slip_count[4] = MPAGE_SIZE / 64;
	slip_count[5] = 4 * MPAGE_SIZE / 128;
	slip_count[6] = 8 * MPAGE_SIZE / 256;
	slip_count[7] = 16 * MPAGE_SIZE / 512;
	slip_count[8] = 32; /* 1K */
	slip_count[9] = 32; /* 2K */
	slip_count[10] = 32; /* 4K */
	slip_count[11] = 8; /* 8K */
	slip_count[13] = 16; /* 16K */
	//slip_count[25] = 16; /* 64K */
	slip_count[41] = 4; /* 128K */
	slip_count[57] = 4; /* 192K */
	slip_count[73] = 16; /* 256K */

#ifdef  MEMPOOL_3M	 
	slip_count[13] = 48; /* 16K */
	slip_count[41] = 0; /* 128K */
	slip_count[57] = 0; /* 192K */
	slip_count[73] = 0; /* 256K */   
#endif


#ifdef  MEMPOOL_5M	 
	slip_count[13] = 96; /* 16K */
	slip_count[41] = 0; /* 128K */
	slip_count[57] = 0; /* 192K */
	slip_count[73] = 0; /* 256K */   
#endif

#ifdef  MEMPOOL_8M	 
	slip_count[13] = 180; /* 16K */
	slip_count[41] = 0; /* 128K */
	slip_count[57] = 0; /* 192K */
	slip_count[73] = 0; /* 256K */       
#endif

#ifdef  MEMPOOL_10M	 
	slip_count[13] = 196; /* 16K */
	slip_count[41] = 0; /* 128K */
	slip_count[57] = 0; /* 192K */
	slip_count[73] = 0; /* 256K */
#endif

	slip_count[137] = 0; /* 512K */
	slip_count[265] = 0; /* 1M */
	slip_count[521] = 0; /* 2M */

	/*#ifdef ENABLE_VOD
	  slip_count[25] +=  32 ; //add 2M for vod 
#endif

#ifdef _VOD_NO_DISK   
slip_count[25] +=  128 ; //add 4M for vod cache buffer without disk
#endif*/

	/*#ifdef _VOD_NO_DISK_EXTEND   
	  slip_count[25] +=  128 ; //add 4M for vod cache buffer without disk
#endif*/


	return mpool_init(page_count, 522, slip_count);
}

_int32 default_mpool_uninit(void)
{

	/* 10M bytes */
	_int32 page_count = 2560;

#ifdef  MEMPOOL_3M	 
	page_count = 768;
#endif

#ifdef  MEMPOOL_5M	 
	page_count = 1280;
#endif

#ifdef  MEMPOOL_8M
	page_count = 2048;

#endif

#ifdef  MEMPOOL_10M
	page_count = 2560;
#endif


	LOG_DEBUG("default_mpool_uninit  begin !");

	/*
#ifdef ENABLE_VOD
page_count += 2560;
#endif
*/
	/*#ifdef _VOD_NO_DISK   
	  page_count += 2048 ; //add 8M for vod cache buffer without disk
#endif*/

	/*
#ifdef _VOD_NO_DISK_EXTEND   
page_count += VOD_EXTEND_BUFFER ; //add 8M for vod cache buffer without disk
#endif
*/
	prn_alloc_info();

	mpool_uninit(page_count);

	LOG_DEBUG("default_mpool_uninit  end !");

	return SUCCESS;

}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  int set_bitmap
 *  Description:  set bit value of chunks in bitmap
 * =====================================================================================
 */
_int32 set_bitmap ( pool_t *mp,_u32 index, _u32 total, BOOL flag)
{
	_u32 idx=0,offset=0,i=0;
//	_u32 end_idx=0,end_offset=0;

	if(mp == NULL )
		return INVALID_MEMORY;

	if(index < 0 || index > mp->total - total)
	{
		LOG_DEBUG( "index out of bitmap range\n" );
		return INVALID_MEMORY;
	}

//	idx = index / 8;
//	end_idx = (index+total)/8 ;
//	offset = index % 8 ;
//	end_offset = (index+total)%8 ;


	if ( flag )
	{
//		LOG_DEBUG ("total:%d",total);
//		LOG_DEBUG ("before setbit %dth of bitmap[%d],value:%d ",index,idx,mp->bitmap[idx]); 
//		setbit(mp->bitmap[idx],offset);

		LOG_DEBUG ("before set bitmap with total");
		for (i=index; i<index+total; i++)
		{
			idx = i / 8;
			offset = i % 8 ;		
			setbit(mp->bitmap[idx],offset);
//			LOG_DEBUG ("%d:set the %dth of bitmap[%d] to 1,value:%d",i,offset,idx,mp->bitmap[idx]); 

		}
//		LOG_DEBUG ("before end %dth of bitmap[%d] ,value:%d\n",end_offset,end_idx,mp->bitmap[end_idx]); 
//		setbit(mp->bitmap[end_idx],end_offset);
//		LOG_DEBUG ("set the end %dth of bitmap[%d] to 1,value:%d\n\n",end_offset,end_idx,mp->bitmap[end_idx]); 
	}
	else
	{
//		LOG_DEBUG ("total:%d",total);
//		LOG_DEBUG ("before clear %dth of bitmap[%d], value:%d\n",offset,idx,mp->bitmap[idx]); 
//		clrbit(mp->bitmap[idx],offset);

		LOG_DEBUG ("before clear bitmap with total");
		for (i=index; i<index+total; i++)
		{
			idx = i / 8;
			offset = i % 8 ;		
			clrbit(mp->bitmap[idx],offset);
//			LOG_DEBUG ("%d:clear the %dth of bitmap[%d] to 0,value:%d\n",i,offset,idx,mp->bitmap[idx]); 
		}

//		LOG_DEBUG ("current %dth of bitmap[%d],value:%d \n",end_offset,end_idx,mp->bitmap[end_idx]); 
//		clrbit(mp->bitmap[end_idx],end_offset);
//		LOG_DEBUG ("clear the %dth of bitmap[%d] to 0,value:%d\n",end_offset,end_idx,mp->bitmap[end_idx]); 
	}

	return SUCCESS;

} /* -----  end of function int set_bit  ----- */


_int32 set_all_bitmap(pool_t *mp,_u32 index, _u32 total)
{
	_u32 idx=0,offset=0,i=0;
	if(mp == NULL )
		return INVALID_MEMORY;

	if(index < 0 || index > mp->total - total)
	{
		LOG_DEBUG( "index out of bitmap range\n" );
		return INVALID_MEMORY;
	}

	for (i=index; i<index+total; i++)
	{
		idx = i / 8;
		offset = i % 8 ;		
		setbit(mp->bitmap[idx],offset);
	}

	return SUCCESS;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  int check_bit
 *  Description:  check bit value of current chunk status in bitmap 
 * =====================================================================================
 */
BOOL check_bitmap ( pool_t *mp, _u32 index )
{
	if(mp == NULL )
		return INVALID_MEMORY;

	if(index < 0 || index > mp->total-1 )
	{
		LOG_DEBUG( "index out of bitmap range\n" );
		return INVALID_MEMORY;
	}

	_u32 idx = index >> 3;
	_u32 offset = index % 8 ;

	return chkbit(mp,idx,offset);

} /* -----  end of function int check_bitmap  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  int set_chunk_size
 *  Description:  modify current chunk size (the unit of size is K bityes)
 * =====================================================================================
 */
_int32 set_chunk_size ( pool_t *mp, _u32 index, _u32 value )
{
	if(mp == NULL )
		return INVALID_MEMORY;

	if(index < 0 || index > mp->total-value)
	{
		LOG_DEBUG( "index out of header range\n" );
		return INVALID_MEMORY;
	}
	
	if(value == 0)
		return INVALID_MEMORY;

#ifdef	_MEM_DEBUG	
	_u32 i=0;

	for ( i=index;i<index+value;i++)
	{
		mp->header[i] = 0;
	}
#endif
	mp->header[index] = value;
	mp->header[index+value-1] = value;


	return SUCCESS;
} /* -----  end of function int set_chunk_size  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  int get_chunk_size
 *  Description:  get current chunk size from memory pool header information
 * =====================================================================================
 */
_int32 get_chunk_size ( pool_t *mp, _u32 index )
{
	if(mp == NULL )
		return INVALID_MEMORY;

	if(index < 0 || index > mp->total)
	{
		LOG_DEBUG( "index out of bitmap range\n" );
		return INVALID_MEMORY;
	}

	return mp->header[index];
} /* -----  end of function int get_chunk_size  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  get_prev_free_chunk
 *  Description:  get the previous not allocted chunk address
 * =====================================================================================
 */
char* get_prev_free_chunk ( pool_t *mp, chunk_t *start )
{
	char* P = (char*) start;
	_u32 previous=0,idx=0;

//	LOG_DEBUG("bitmap value at begin of get_prev_free_chunk");
//	show_bitmap(mp);

	if ( P > mp->address )
	{
		P -= CHUNK_SIZE;
		idx = idxchunk(idxmap(mp,P));
		previous = get_chunk_size(mp,idx);

		if(previous == 0)
			return (char*)start;

		if ( 1 == previous )
		{
			if ( !check_bitmap(mp,idx))
			{	
				return P;
			}
			else
				return (char*)start;
		}
		else
		{
			if(P < mp->address)
				return (char*)start;
			else
			{
				if(P == mp->address && !check_bitmap(mp,idx) )
					return P;

				if ( !check_bitmap(mp,idx) )
				{		
					return ((char*)start - (previous << 10) );
				}
				else
					return (char*)start;
			}
		}
	}
	
//	LOG_DEBUG("bitmap value at end of get_prev_free_chunk");
//	show_bitmap(mp);

	return (char*)start;
} /* -----  end of function get_prev_free_chunk  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  pool_t * mpool_extend_create
 *  Description:  create extend memory pool
 * =====================================================================================
 */
pool_t * mpool_extend_create ( pool_t* mpool_extend_base, _u32 size )
{
	_u32 bit_num=0;

	pool_t *pool = NULL;
	pool = (pool_t*)mpool_extend_base;

	if(pool != NULL)
	{
		pool->total = (size-8)*8 / (97 + CHUNK_SIZE*8);

		if ( pool->total % 8 == 0 )
			bit_num = pool->total/8;
		else
			bit_num = pool->total/8 +1 ;

		pool->bitmap = (_u8 *)((_u8*)pool + sizeof(pool_t));
		pool->header = (_u32*)(pool->bitmap + bit_num);
		pool->free_list =(chunk_t*)(pool->header + pool->total); 
		pool->address = (char*)(pool->free_list + pool->total);

		while((_u32)(pool->address) & 0x03)
		{
			(char*)pool->address++;
		}

//		LOG_DEBUG(" total :%d, address 0x%x\n",pool->total, (_u32)&pool->total);
//		LOG_DEBUG(" bitmap start address 0x%x\n",pool->bitmap);
//		LOG_DEBUG(" header start address 0x%x\n",pool->header);
//		LOG_DEBUG(" free_list start address 0x%x\n",pool->free_list);
//		LOG_DEBUG(" start point and address 0x%x,0x%x\n",pool->address,&pool->address);

		mpool_extend_init(pool);

#ifdef  _MEM_DEUBG
		_u32 i=0;
		for ( i=0;i<pool->total ; i++) 
		{
			if(pool->free_list[i].prev != &(pool->free_list[i]))
			{	
				LOG_DEBUG ( "header size:%d, bitmap value:%d\n",pool->header[i],pool->bitmap[i>>3] );
				LOG_DEBUG ( "prev address:0x%x, next address:0x%x of free_list[%d] address: 0x%x,\n",
							pool->free_list[i].prev,pool->free_list[i].next,i,&(pool->free_list[i]) );
			}
		}
#endif     
	}

	return pool;
} /* -----  end of function pool_t * mpool_extend_create  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  int mpool_destory
 *  Description:  destory extend memory pool and free the physical memory space.
 * =====================================================================================
 */
int mpool_extend_destory ( pool_t *pool )
{
	return TRUE; 
} /* -----  end of function pool_t * mpool_destory  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  int mpool_extend_init
 *  Description:  initialisation memory pool
 * =====================================================================================
 */
_int32 mpool_extend_init ( pool_t *pool )
{
	_u32 i=0,j=0,offset=0,total=0;
	chunk_t *P = NULL;

	if(pool == NULL)
		return -1;

	P=(chunk_t*)pool->address;
	if (!P)
	{
		return -1;
	}

	total = pool->total;

//	LOG_DEBUG("TOTAL: %d,bitmap value before mpool_extend_init",pool->total);
//	show_bitmap(pool);

	for(i = 0; i < total; i++) 
	{
		
		// set all of bits to 0
		j = i/8;
		offset = i % 8 ;
		pool->bitmap[j] = 0;

		if(i != 0 && i != pool->total-1)
		{	
			pool->header[i]=0;
		}
		else
		{
			pool->header[i]=pool->total;
		}

		pool->free_list[i].prev = &(pool->free_list[i]);
		pool->free_list[i].next = &(pool->free_list[i]);		
		if(i == total-1)
		{
			P->prev = pool->free_list[i].prev;
			P->next = &(pool->free_list[i]);
			pool->free_list[i].prev = P;
			P->prev->next = P;
		}
//		LOG_DEBUG("free_list[%d]:0x%x,prev:0x%x\n,next:0x%x\n",i,pool->free_list[i],pool->free_list[i].prev,pool->free_list[i].next);
	}

//	LOG_DEBUG("TOTAL: %d,bitmap value after mpool_extend_init",pool->total);
#ifdef _LOGGER
	show_bitmap(pool);
#endif

	return SUCCESS;
} /* -----  end of function int mpool_extend_init  ----- */



/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  char* get_chunk
 *  Description:  get free chunk unit from memory pool
 * =====================================================================================
 */
char* get_chunk ( pool_t *mp, _int32 size )
{
	chunk_t *L=NULL, *P=NULL;
	_u32 order=0,index=0;
	_int32 j=0;

	if(inuse == 1)
		LOG_DEBUG("ASYNCHRO \n");
	inuse = 1;

	if(mp == NULL)
		return NULL;

	if(size < 1)
	{	
		LOG_DEBUG( "  can't allocate less than 1K bytes memory chunk!\n" );
		return NULL;
	}

	if(size > mp->total)
	{	
		LOG_DEBUG( "please get it from heap not here !\n" );
		return NULL;
	}

//	LOG_DEBUG("BBBBBBBBBBBBBBBBBBBBGET:bitmap value before start get_chunk");
//	show_bitmap(mp);

	//
	for(j = size-1; (j < mp->total) && (mp->free_list[j].prev == &(mp->free_list[j]));j++);

	if(j > mp->total-1) 
	{	
		LOG_DEBUG( "out of this memory pool range\n" );
		return NULL;
	}

//	LOG_DEBUG( " FreeList[%d], PREV: 0x%x, NEXT: 0x%x\n",j,mp->free_list[j].prev,mp->free_list[j].next);

	// remove from free list
	L = mp->free_list[j].prev;
	//	mp->free_list[j].prev = L->next;
	//	L->prev->next = &(mp->free_list[j]);
	L->prev->next = L->next;
	L->next->prev = L->prev;
	
	index = idxchunk(idxmap(mp,(char*)L));
	set_bitmap(mp, index, size, 1);
	set_chunk_size(mp,index,size);

	LOG_DEBUG( " allocated chunk size:%d \n",mp->header[index] );
	LOG_DEBUG( " allocated %dth chunk and the address is 0x%x \n",index,L);

	// Split required 
	if(j != size-1) 
	{
		// Split
		order = j - size;
		P = (chunk_t*)mapidx(mp,(index+size));
		P->prev = mp->free_list[order].prev;
		P->next = &(mp->free_list[order]);
		P->prev->next = P;
		mp->free_list[order].prev = P;

//		P->prev = &(mp->free_list[order]);
//		P->next = &(mp->free_list[order]);
//		mp->free_list[order].prev = mp->free_list[order].next = P;

		set_chunk_size(mp,idxchunk(idxmap(mp,(char*)P)), order+1);
	}

	inuse = 0;

//	LOG_DEBUG("FFFFFFFFFFFFFFFFFFFFGET:bitmap value after start get_chunk");
//	show_bitmap(mp);
//	_u32 i=0;
//	for(i=0;i<mp->total;i++)
//		LOG_DEBUG("free_list[%d]:0x%x,prev:0x%x\n,next:0x%x\n",i,mp->free_list[i],mp->free_list[i].prev,mp->free_list[i].next);
//

	return (char*)L;

} /* -----  end of function get_chunk  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  int free_chunk *  Description:  free chunk and put on free list of memory pool
 * =====================================================================================
 */
_int32 free_chunk ( pool_t *mp, char* address )
{
	chunk_t *L=NULL, *P=NULL,*T=NULL;
	_u32 order=0,index=0,idx=0,cur_idx=0;
	BOOL flag=0,combian_flag = 0;

	if(inuse == 1)
		LOG_DEBUG("ASYNCHRO \n");
	inuse = 1;

	if(mp == NULL || address == NULL || address < mp->address)
		return INVALID_MEMORY;
	
	L = (chunk_t *)address;
	L->prev = L;
	L->next = L;
	T = L;
	P = L;
	
//	LOG_DEBUG( "Free Address: current address is 0x%x \n", address);
//	LOG_DEBUG("bitmap value before start free_chunk");
//	show_bitmap(mp);
//
	index = idxchunk(idxmap(mp,address));
	order = get_chunk_size(mp,index);

	do {
		// look for the other unit and combine it
		if(!flag)
		{
			cur_idx = idxchunk(idxmap(mp,(char*)T));
			if(cur_idx + get_chunk_size(mp,cur_idx)< mp->total-1 && order < mp->total) 
			{
				idx = cur_idx + get_chunk_size(mp,cur_idx);
				if ( !check_bitmap(mp,idx) && get_chunk_size(mp,idx) != 0 )
				{
					P = (chunk_t*)mapidx(mp,idx);
					order += get_chunk_size(mp,idx);

//					LOG_DEBUG("bitmap value in combin the next chunk with free_chunk");
//					show_bitmap(mp);

					T = P;
				}
				else
				{
					flag ^= TRUE;
					combian_flag = 1;
				}
			}
			else
			{
				flag ^= TRUE;
				combian_flag = 1;
			}
		}
		else
		{
			combian_flag = 0;
			P = (chunk_t*)get_prev_free_chunk(mp,L);
			cur_idx = idxchunk(idxmap(mp,(char*)P));

			if (P == L)
			{
				flag ^= TRUE;
				combian_flag = 1;	
//				LOG_DEBUG("bitmap value in finish combin with free_chunk BEFOR SETBIT");
//				show_bitmap(mp);

				set_bitmap(mp,cur_idx,1,1);

//				LOG_DEBUG("bitmap value in finish combin with free_chunk AFTER SETBIT");
//				show_bitmap(mp);
			}
			else
			{					
				order += get_chunk_size(mp,cur_idx);

//				LOG_DEBUG("bitmap value in combin the prev chunk with free_chunk");
//				show_bitmap(mp);

				if(P < L)
					L = P;
			}
		}

		// Combine them to larger unit
		if (0==combian_flag)
		{
		P->next->prev = P->prev;
		P->prev->next = P->next;
		}
		// Not any chunks are available 
		if((order == mp->total) ||(check_bitmap(mp,idxchunk(idxmap(mp,(char*)P))) && flag == FALSE))  
			break;

	} while(1);

	// Put on list
	L->prev = mp->free_list[order-1].prev;
	L->next = &(mp->free_list[order-1]);
	mp->free_list[order-1].prev->next = L;
	mp->free_list[order-1].prev = L;		

	set_bitmap(mp,idxchunk(idxmap(mp,(char*)L)),order,0);
	set_chunk_size(mp,idxchunk(idxmap(mp,(char*)L)),order);

	inuse = 0;

//	LOG_DEBUG( "order:%d, Combine Free Address: combine address is 0x%x ,prev address is:0x%x and next address is:0x%x\n", order,L, L->prev,L->next);
//	LOG_DEBUG("bitmap value at end of free_chunk");
//	show_bitmap(mp);

	return SUCCESS;
} /* -----  end of function _u32 free_chunk  ----- */

void show_bitmap(pool_t* mp)
{
	_u32 i=0,j=0,idx=0;
	idx = mp->total >> 3;

	for (i=0;i<idx;i+=10)
	{
		LOG_DEBUG("%d\t 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,",i,(_u32)mp->bitmap[i],(_u32)mp->bitmap[i+1],(_u32)mp->bitmap[i+2],(_u32)mp->bitmap[i+3],(_u32)mp->bitmap[i+4],(_u32)mp->bitmap[i+5],(_u32)mp->bitmap[i+6],(_u32)mp->bitmap[i+7],(_u32)mp->bitmap[i+8],(_u32)mp->bitmap[i+9]);
	}

	for (j=0;j<mp->total;j+=10)
	{
		LOG_DEBUG("%d\t %d,%d,%d,%d,%d,%d,%d,%d,%d,%d",j,mp->header[j],mp->header[j+1],mp->header[j+2],mp->header[j+3],mp->header[j+4],mp->header[j+5],mp->header[j+6],mp->header[j+7],mp->header[j+8],mp->header[j+9]);
	}
}

#endif
