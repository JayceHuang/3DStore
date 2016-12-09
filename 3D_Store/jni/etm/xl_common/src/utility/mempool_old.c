#include "utility/define.h"
#ifndef _USE_NEW_MEMPOOL
#include "utility/errcode.h"
#include "utility/string.h"
#include "utility/mempool.h"
#include "platform/sd_mem.h"

#ifdef _USE_GLOBAL_MEM_POOL
#include "platform/sd_task.h"
#endif

#if defined(LINUX) && defined(_MEM_DEBUG)
#include <signal.h>
#if defined(LINUX) 
#include <execinfo.h>
#endif
#endif
#define LOGID LOGID_MEMPOOL
#include "utility/logger.h"

/* implement of mempool */
static MEMPOOL *g_mpool = NULL;

#ifdef _USE_GLOBAL_MEM_POOL
TASK_LOCK  g_global_mem_lock ;
#endif

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
#if defined(LINUX)
#include <malloc.h>

#elif defined(WINCE)
#include "utility/pkfuncs.h"
#include "utility/mapfile.h"
#define MODULENAME  L"thunder"

#endif
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
#if defined(LINUX)
      char _stackframe[BACKTRACK_STACK_DEPTH][STACK_INFO_LEN];
#elif defined(WINCE)
	DWORD _stackframe[BACKTRACK_STACK_DEPTH];
#endif
      _int32 _stack_depth;
}MEM_CHECK_INFO;

#ifdef STACK_FRAME_DEPTH
#if defined(LINUX)
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
#elif defined(WINCE)
#define GET_STACK_FRAME(meminfo)  {\
	CallSnapshot lpFrames[BACKTRACK_STACK_DEPTH];\
	_u32 stack_depth = 0, stack_idx;\
	stack_depth = GetCallStackSnapshot(BACKTRACK_STACK_DEPTH, lpFrames, 0, 1);\
	for(stack_idx = 0; stack_idx < stack_depth; stack_idx++)\
		meminfo._stackframe[stack_idx] = lpFrames[stack_idx].dwReturnAddr;\
	(meminfo)._stack_depth = stack_depth;\
}
#else
#define GET_STACK_FRAME(meminfo)  (meminfo)._stack_depth = 0;
#endif
#else

#define GET_STACK_FRAME(meminfo)  (meminfo)._stack_depth = 0;

#endif

static SET g_memcheck_set;
static _u64 g_malloc_cur = 0;		//内存申请当前值
static _u64 g_malloc_max = 0;		//内存申请峰值
static _u32 g_malloc_singlemax = 0; //单次内存申请最大值
static _u32 g_malloc_count = 0;		//内存申请次数
static _u32 g_free_count = 0;		//内存释放次数
static _u32 g_exmalloc_cur = 0;		//额外内存申请当前值
static _u32 g_exmalloc_max = 0;		//额外内存申请当前值
static _u32 g_exmalloc_times = 0;	//额外内存申请次数
static _u32 g_exfree_times = 0;		//额外内存释放次数

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

	if(check_info->_status)		//内存申请统计
	{	
		g_malloc_cur += check_info->_memsize;
		g_malloc_count++;
		if(check_info->_memsize > g_malloc_singlemax)
			g_malloc_singlemax = check_info->_memsize;
		if(g_malloc_cur > g_malloc_max)
			g_malloc_max = g_malloc_cur;
		if(check_info->_other_data == MAX_U32)
		{
			g_exmalloc_cur += check_info->_memsize;
			g_exmalloc_times++;
			if(g_exmalloc_cur > g_exmalloc_max)
				g_exmalloc_max = g_exmalloc_cur;
		}
	}

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

		if(!check_info->_status)	//内存释放统计
		{
			g_malloc_cur -= pinfo->_memsize;
			g_free_count++;
			if(pinfo->_other_data == MAX_U32)
			{
				g_exmalloc_cur -= pinfo->_memsize;
				g_exfree_times++;
			}
		}

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
#ifdef WINCE
			LOG_ERROR("ref_file: %s, ref_line: %d, ref_time: %u(%u)",
				pinfo->_ref_file, pinfo->_ref_line, pinfo->_alloc_time, pinfo->_alloc_time);
#else
			LOG_ERROR("ref_file: %s, ref_line: %d, ref_time: %u(%s)",
				pinfo->_ref_file, pinfo->_ref_line, pinfo->_alloc_time, ctime((const time_t*)&pinfo->_alloc_time));
#endif			

                     LOG_ERROR("call stack:");
#if defined(LINUX)
                     for(stack_idx = 0; stack_idx <pinfo->_stack_depth; stack_idx++)
                     {
                          LOG_ERROR("%d:  %s", stack_idx, pinfo->_stackframe[stack_idx]);
                     }
#elif defined(WINCE)
			for(stack_idx = 0; stack_idx < pinfo->_stack_depth; stack_idx++)
			{              
				DWORD map_addr = pinfo->_stackframe[stack_idx] & 0xffffff; 
				//若要打印堆栈函数名，需要将编译生成的Thunder.map文件拷贝至机器根目录下
				LOG_ERROR( "call function %d: %s", stack_idx, MapfileLookupAddress( MODULENAME, map_addr ) );
			}
#endif
                     LOG_ERROR("-------------------------");

		}
	}
	LOG_ERROR("max single memory allocated : %d byte ", g_malloc_singlemax);
	LOG_ERROR("extent memory allocated cur: %d byte", g_exmalloc_cur);
	LOG_ERROR("extent memory allocated max: %d byte", g_exmalloc_max);
	LOG_ERROR("extent malloc times: %d", g_exmalloc_times);
	LOG_ERROR("extent free times: %d", g_exfree_times);
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
		LOG_DEBUG("too many slab be created, can not record");

	LOG_DEBUG("malloc a slab(0x%x) on mem(0x%x) of slip_size(%d) at %s:%d", *slab, slab, slip_size, ref_file, line);
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
	#if defined(WINCE)
	sd_time(&meminfo._alloc_time);
	#else
	meminfo._alloc_time = time(NULL);
	#endif
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
	meminfo._memsize = 0;
	meminfo._status = 0;
	sd_strncpy(meminfo._ref_file, ref_file, sizeof(meminfo._ref_file));
	meminfo._ref_line = (_u16)line;
	#if defined(WINCE)
	sd_time(&meminfo._alloc_time);
	#else
	meminfo._alloc_time = time(NULL);
	#endif
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
	#if defined(WINCE)
	sd_time(&meminfo._alloc_time);
	#else
	meminfo._alloc_time = time(NULL);
	#endif
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
	#if defined(WINCE)
	sd_time(&meminfo._alloc_time);
	#else
	meminfo._alloc_time = time(NULL);
	#endif
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

#if defined(LINUX)

static BOOL g_need_prn_info;
void signal_handle(_int32 sign)
{
	LOG_ERROR("received a signal, ready to prn mem-info");
	g_need_prn_info = TRUE;
}

#endif

void prn_info_when_need(void)
{
#if defined(LINUX)
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

#if defined(LINUX)
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
	(*mpool)->_extend_data_bottom = pdata;

#ifdef _MEM_DEBUG
	init_mpool_info_recorder(stardard_slip_count);
#endif

	return ret_val;
}

static _int32 sd_extend_malloc(MEMPOOL *mpool, _u32 index, void **mem)
{
	_u32 size = 0;
	size = get_size(index);

	/*check room*/
	if(mpool->_extend_data_bottom + size + SSLAB_ADDR_PREFIX_SIZE >= mpool->_custom_slab_cur_data)
	{
		return OUT_OF_MEMORY;
	}

	*(char**)mpool->_extend_data_bottom = mpool->_standard_slab_index + index * sizeof(char*);
	mpool->_extend_data_bottom += SSLAB_ADDR_PREFIX_SIZE;

	*mem = mpool->_extend_data_bottom;
	mpool->_extend_data_bottom += size;

	return SUCCESS;
}

static _int32 sd_extend_free(MEMPOOL *mpool, char *mem)
{
	char *free_index_head = NULL;
	_u32 free_index = 0;
	_u32 size = 0;

	free_index_head = *(char**)(mem - SSLAB_ADDR_PREFIX_SIZE);
	free_index = (free_index_head - mpool->_standard_slab_index) / sizeof(char*);

	size = get_size(free_index);

	/* simple check */
	if(mem + size == mpool->_extend_data_bottom)
	{/* can be collected */
		mpool->_extend_data_bottom -= (size + SSLAB_ADDR_PREFIX_SIZE);
		return SUCCESS;
	}

	*(char**)mem = *(char**)free_index_head;
	*(char**)free_index_head = mem;

	return SUCCESS;
}

#ifdef _MEM_DEBUG
static _int32 sd_malloc_from_index(MEMPOOL *mpool, _u32 index, void **mem, _int32 *res_index)
#else
static _int32 sd_malloc_from_index(MEMPOOL *mpool, _u32 index, void **mem)
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
			sd_assert(*free_slip <= mpool->_extend_data_bottom && *free_slip >= mpool->_standard_slab_low_base);
			*mem = *free_slip;
			*free_slip = *(char**)(*free_slip);
			/* 如果断言,极有可能内存被释放后又被使用了!
			请在define.h 中打开_USE_OS_MEM_ONLY宏再定位问题 */
			if(*free_slip != SLAB_INDEX_EOF)
				sd_assert(*free_slip <= mpool->_extend_data_bottom && *free_slip >= mpool->_standard_slab_low_base);
#ifdef _MEM_DEBUG
			*res_index = i;
#endif

			return SUCCESS;
		}
		free_slip ++;
	}

	if(sd_extend_malloc(mpool, index, mem) == SUCCESS)
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

#ifdef _MEM_DEBUG
	void *src_mem = *mem;
	MEM_CHECK_INFO meminfo;
	_int32 res_index = -1;

      GET_STACK_FRAME(meminfo);
#endif

	*mem = NULL;
	if(memsize == 0)
		return SUCCESS;

#ifdef _USE_OS_MEM_ONLY
       ret_val = sd_get_extent_mem_from_os(memsize, mem);
	     return ret_val;		
#endif
	
#ifdef _EXTENT_MEM_FROM_OS	
      if(memsize >= 4*1024)
     {
		 index = MAX_U32;	
		 ret_val = sd_get_extent_mem_from_os(memsize, mem);	
     }
	  else
	 {
#endif

	index = get_index(memsize);
	if(index >= (_u32)mpool->_stardard_slab_count)
	{
		LOG_DEBUG("too big memory required(%d)...", index);
		ret_val = OUT_OF_MEMORY;
	}
	else
#ifdef _MEM_DEBUG
	ret_val = sd_malloc_from_index(mpool, index, mem, &res_index);
#else
	ret_val = sd_malloc_from_index(mpool, index, mem);
#endif

#ifdef _EXTENT_MEM_FROM_OS
		if(ret_val != SUCCESS)
		{
		  index = MAX_U32;
		  ret_val = sd_get_extent_mem_from_os(memsize, mem);
		}
	  }
#endif
#ifdef _MEM_DEBUG
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
#if defined(WINCE)
		sd_time(&meminfo._alloc_time);
#else
		meminfo._alloc_time = time(NULL);
#endif
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

#ifdef _USE_OS_MEM_ONLY
       ret_val = sd_free_extent_mem_to_os( mem,0);
	     return ret_val;		
#endif

#ifdef _EXTENT_MEM_FROM_OS
      if(mem<mpool->_standard_slab_low_base || mem>=mpool->_mempool_bottom)
      {
           ret_val = sd_free_extent_mem_to_os(mem, 0);
#ifdef _MEM_DEBUG
		   meminfo._address = (_u32)data;
		   meminfo._memsize = 0;
		   meminfo._status = 0;
		   sd_strncpy(meminfo._ref_file, ref_file, sizeof(meminfo._ref_file));
		   meminfo._ref_line = (_u16)line;
#if defined(WINCE)
		   sd_time(&meminfo._alloc_time);
#else
		   meminfo._alloc_time = time(NULL);
#endif
		   meminfo._other_data = MAX_U32;
		   check_mem(&meminfo);
#endif
		   return ret_val;
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

#ifdef _MEM_DEBUG
		meminfo._address = (_u32)data;
		meminfo._memsize = 0;
		meminfo._status = 0;
		sd_strncpy(meminfo._ref_file, ref_file, sizeof(meminfo._ref_file));
		meminfo._ref_line = (_u16)line;
#if defined(WINCE)
		sd_time(&meminfo._alloc_time);
#else
		meminfo._alloc_time = time(NULL);
#endif
		meminfo._other_data = MAX_U32;

		check_mem(&meminfo);
#endif

		return ret_val;
	}
	else
		return INVALID_MEMORY;

	*(char**)mem = *(char**)free_index;
	*(char**)free_index = mem;

#ifdef _MEM_DEBUG

	/* cal mem-index */
	actual_index = (free_index - mpool->_standard_slab_index) / sizeof(char*);
	actual_msize = get_size(actual_index);

	meminfo._address = (_u32)data;
	meminfo._memsize = 0;
	meminfo._status = 0;
	sd_strncpy(meminfo._ref_file, ref_file, sizeof(meminfo._ref_file));
	meminfo._ref_line = (_u16)line;
#if defined(WINCE)
	sd_time(&meminfo._alloc_time);
#else
	meminfo._alloc_time = time(NULL);
#endif
	meminfo._other_data = MAX_U32;

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

#ifdef _MEM_DEBUG
	CSLAB_INFO *pinfo = NULL;
	SET_NODE *pnode = NULL;
//	MEM_CHECK_INFO meminfo;
#endif

#ifdef _USE_OS_MEM_ONLY
     
	ret_val = sd_get_extent_mem_from_os(sizeof(SLAB), (void**)slab);
	CHECK_VALUE(ret_val);

	sd_memset(*slab, 0, sizeof(SLAB));
	(*slab)->_slip_size = slip_size;
	     return 0;		
#endif

#ifdef _USE_GLOBAL_MEM_POOL

      ret_val = sd_malloc(sizeof(SLAB)+ slip_size * min_slip_count, (void**)slab);
	CHECK_VALUE(ret_val);

	sd_memset(*slab, 0, sizeof(SLAB));

	(*slab)->_free_index = (char*)(*slab) +  sizeof(SLAB);
	(*slab)->_slip_size = slip_size;
	(*slab)->_bottom  =  (*slab)->_free_index  + slip_size * min_slip_count;
	init_custom_slab_data((*slab)->_free_index, slip_size, min_slip_count);
/*
#ifdef _MEM_DEBUG
	meminfo._address = (_u32)slab;
	meminfo._memsize = slip_size * min_slip_count;
	meminfo._status = 1;
	sd_strncpy(meminfo._ref_file, ref_file, sizeof(meminfo._ref_file));
	meminfo._ref_line = (_u16)line;
#if defined(WIN32)
	sd_time(&meminfo._alloc_time);
#else
	meminfo._alloc_time = time(NULL);
#endif
	meminfo._other_data = min_slip_count;

	check_mem(&meminfo);
#endif	
*/
	return ret_val;
 #else	

	/* check room */
	if((char*)g_mpool->_custom_slab_cur_index <= g_mpool->_extend_data_index_bottom + sizeof(SLAB)
		|| g_mpool->_custom_slab_cur_data <= g_mpool->_extend_data_bottom + slip_size * min_slip_count)
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
#endif
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

#ifdef _USE_OS_MEM_ONLY
       return sd_get_extent_mem_from_os(slab->_slip_size, slip);
	     
#endif

#ifdef _USE_GLOBAL_MEM_POOL


     ret_val = sd_task_lock ( &g_global_mem_lock );
     CHECK_VALUE(ret_val);

        if(slab->_free_index != SLAB_INDEX_EOF)
	{
		*slip = slab->_free_index;
		slab->_free_index = *(char**)slab->_free_index;
	}
	else
	{
             #ifdef _EXTENT_MEM_FROM_OS	
              if(slab->_slip_size >= 128)
              {
                     ret_val = sd_get_extent_mem_from_os(slab->_slip_size, slip);
#ifdef _MEM_DEBUG
					 meminfo._address = (_u32)(*slip);
					 meminfo._memsize = slab->_slip_size;
					 meminfo._status = 1;
					 sd_strncpy(meminfo._ref_file, ref_file, sizeof(meminfo._ref_file));
					 meminfo._ref_line = (_u16)line;
#if defined(WINCE)
					 sd_time(&meminfo._alloc_time);
#else
					 meminfo._alloc_time = time(NULL);
#endif
					 meminfo._other_data = MAX_U32;
					 check_mem(&meminfo);
#endif
                     sd_task_unlock ( &g_global_mem_lock );

	                 return ret_val;		
              }
              #endif 
	
#ifdef _MEM_DEBUG
		*slip = slab;
		ret_val = sd_custom_mpool_malloc(g_mpool, slab->_slip_size, ref_file, line, slip);
#else
		ret_val = sd_custom_mpool_malloc(g_mpool, slab->_slip_size, slip);
#endif

	}
	
       sd_task_unlock ( &g_global_mem_lock );

       return ret_val;       
 #else	

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
		#if defined(WINCE)
		sd_time(&meminfo._alloc_time);
		#else
		meminfo._alloc_time = time(NULL);
		#endif
		meminfo._other_data = (_u32)slab;

		check_mem(&meminfo);
#endif

	}
	else
	{
             #ifdef _EXTENT_MEM_FROM_OS	
              if(slab->_slip_size >= 128)
              {
                       ret_val = sd_get_extent_mem_from_os(slab->_slip_size, slip);
	                 return ret_val;		
              }
              #endif 
	
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

#endif
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


	GET_STACK_FRAME(meminfo);
//	(meminfo)._stack_depth = 0;
#endif

#ifdef _USE_OS_MEM_ONLY
       return sd_free_extent_mem_to_os(slip, 0);
	     
#endif


#ifdef _USE_GLOBAL_MEM_POOL

     ret_val = sd_task_lock ( &g_global_mem_lock );
     CHECK_VALUE(ret_val);


      sd_assert(!((_u32)slip % ALIGN_SIZE));

      if((char*)slip >=  (char*)slab && (char*)slip < slab->_bottom)
	{
		*(char**)slip = slab->_free_index;
		slab->_free_index = slip;

	}
	else
	{/**/
#ifdef _MEM_DEBUG
	    ret_val = sd_custom_mpool_free(g_mpool,slip, ref_file, line);
#else
           ret_val =  sd_custom_mpool_free(g_mpool,slip);
#endif
	}

        sd_task_unlock ( &g_global_mem_lock );	
	
	return ret_val;


#else


#ifdef _EXTENT_MEM_FROM_OS
      if((char*)slip<g_mpool->_standard_slab_low_base || (char*)slip>=g_mpool->_mempool_bottom)
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
		meminfo._memsize = 0;
		meminfo._status = 0;
		sd_strncpy(meminfo._ref_file, ref_file, sizeof(meminfo._ref_file));
		meminfo._ref_line = (_u16)line;
		#if defined(WINCE)
		sd_time(&meminfo._alloc_time);
		#else
		meminfo._alloc_time = time(NULL);
		#endif
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

#endif
}

_int32 mpool_destory_slab(SLAB *slab)
{/* to be implemented */

_int32 ret_val = SUCCESS;

/*#ifdef _MEM_DEBUG
	MEM_CHECK_INFO meminfo;
#endif*/

#ifdef _USE_GLOBAL_MEM_POOL

      ret_val = sd_free((void*)slab);

#endif

	return ret_val;
}

_int32 mpool_init(_u32 page_count, _int32 stardard_slab_count, _u16 *stardard_slip_count)
{
       _int32 ret_val = SUCCESS;

 #ifdef _USE_GLOBAL_MEM_POOL
       ret_val = sd_init_task_lock(&g_global_mem_lock);	   
	CHECK_VALUE(ret_val);
#endif	  
	ret_val =  create_custom_mpool(page_count, stardard_slab_count, stardard_slip_count, &g_mpool);

	return ret_val;
}

_int32 mpool_uninit(_u32 page_count)
{
      sd_free_mem_to_os(g_mpool, page_count * MPAGE_SIZE);
      g_mpool = NULL; 
 #ifdef _USE_GLOBAL_MEM_POOL
      sd_uninit_task_lock(&g_global_mem_lock);	   
#endif	  	  
      return SUCCESS;
}

#ifdef _MEM_DEBUG

_int32 sd_malloc_impl(_u32 memsize, const char* ref_file, _int32 line, void **mem)
{
	_int32 ret_val = SUCCESS;

	*mem = NULL;

 #ifdef _USE_GLOBAL_MEM_POOL
     ret_val = sd_task_lock ( &g_global_mem_lock );
     CHECK_VALUE(ret_val);
#endif	

	ret_val = sd_custom_mpool_malloc(g_mpool, memsize, ref_file, line, mem);

	prn_info_when_need();

	LOG_DEBUG("malloc a new mem(0x%x) on mem(0x%x) of size(%d) at %s:%d", *mem, mem, memsize, ref_file, line);

#ifdef _USE_GLOBAL_MEM_POOL
       sd_task_unlock ( &g_global_mem_lock );
 #endif	
 
	return ret_val;
}

_int32 sd_free_impl(void *mem, const char* ref_file, _int32 line)
{
	_int32 ret_val = SUCCESS;
	
 #ifdef _USE_GLOBAL_MEM_POOL
     ret_val = sd_task_lock ( &g_global_mem_lock );
     CHECK_VALUE(ret_val);
#endif	
	
	ret_val = sd_custom_mpool_free(g_mpool, mem, ref_file, line);

	prn_info_when_need();

	LOG_DEBUG("free a mem(0x%x) at %s:%d", mem, ref_file, line);

#ifdef _USE_GLOBAL_MEM_POOL
       sd_task_unlock ( &g_global_mem_lock );
 #endif	
 
	return ret_val;
}

#else

_int32 sd_malloc(_u32 memsize, void **mem)
{
	_int32 ret_val = SUCCESS;

 #ifdef _USE_GLOBAL_MEM_POOL
     ret_val = sd_task_lock ( &g_global_mem_lock );
     CHECK_VALUE(ret_val);
#endif	
	
	ret_val = sd_custom_mpool_malloc(g_mpool, memsize, mem);

#ifdef _USE_GLOBAL_MEM_POOL
       sd_task_unlock ( &g_global_mem_lock );
 #endif	
 
	return ret_val;
}

_int32 sd_free(void *mem)
{
	_int32 ret_val = SUCCESS;
	
 #ifdef _USE_GLOBAL_MEM_POOL
     ret_val = sd_task_lock ( &g_global_mem_lock );
     CHECK_VALUE(ret_val);
#endif		
	ret_val = sd_custom_mpool_free(g_mpool, mem);

#ifdef _USE_GLOBAL_MEM_POOL
       sd_task_unlock ( &g_global_mem_lock );
 #endif	
 
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

#ifdef  MEMPOOL_1M	 
	    page_count = 256;
#endif

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

#ifdef  _EXTEND_ETM_3M
		  page_count += 768;
#endif

#ifdef _USE_OS_MEM_ONLY
#ifdef _USE_GLOBAL_MEM_POOL
       return sd_init_task_lock(&g_global_mem_lock);	   
#endif	  
      return 0;
#endif

/*
#ifdef ENABLE_VOD
	  page_count += VOD_MEM_BUFFER; //add 10 for vod cache buffer  6M realtek
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

#ifdef  MEMPOOL_1M	 
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
	slip_count[10] = 0; /* 4K */
	slip_count[11] = 0; /* 8K */
	slip_count[13] = 0; /* 16K */
	slip_count[41] = 0; /* 128K */
	slip_count[57] = 0; /* 192K */
	slip_count[73] = 0; /* 256K */   
#endif	

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
       slip_count[25] = 0; /* 64K */
	slip_count[41] = 0; /* 128K */
	slip_count[57] = 0; /* 192K */
	slip_count[73] = 0; /* 256K */       
#endif
	
#ifdef  MEMPOOL_10M	 
	slip_count[13] = 192; /* 16K */
       slip_count[25] = 0; /* 64K */
	slip_count[41] = 0; /* 128K */
	slip_count[57] = 0; /* 192K */
	slip_count[73] = 0; /* 256K */
#endif
	
#ifdef  _EXTEND_ETM_3M
	slip_count[13] += 48; /* 16K */
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

#ifdef  MEMPOOL_1M	 
	    page_count = 256;
#endif		

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
	
#ifdef  _EXTEND_ETM_3M
		  page_count += 768;
#endif
	
      LOG_DEBUG("default_mpool_uninit  begin !");

#ifdef _USE_OS_MEM_ONLY
#ifdef _USE_GLOBAL_MEM_POOL
      sd_uninit_task_lock(&g_global_mem_lock);	   
#endif	  	  
       return 0;
#endif

/*
#ifdef ENABLE_VOD
	  page_count += VOD_MEM_BUFFER; 
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

#endif

