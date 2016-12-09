
#include "utility/define.h"

#if defined(LINUX)	
#include <sys/mman.h>
#include <errno.h>
#ifndef MACOS
#include <malloc.h>
#endif
#endif

#include "utility/errcode.h"
#include "platform/sd_customed_interface.h"
#include "platform/sd_mem.h"

_int32 sd_get_mem_from_os(_u32 memsize, void **mem)
{
	_int32 ret_val = SUCCESS;

    	if(is_available_ci(CI_MEM_IDX_GET_MEM))
		return ((et_mem_get_mem)ci_ptr(CI_MEM_IDX_GET_MEM))(memsize, mem);

#if defined(LINUX)
#ifdef _DEBUG
	*mem = malloc(memsize);
	if(*mem==NULL)
	{
		ret_val = errno;
	}
#else

	*mem = mmap(0, memsize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, 0, 0);
	if((*mem) == MAP_FAILED)
		ret_val = errno;
#endif
#elif defined(WINCE)
	*mem = malloc(memsize);
#endif

	if(*mem==NULL)
	{
		ret_val = OUT_OF_MEMORY;
	}

	return ret_val;
}

_int32 sd_free_mem_to_os(void* mem, _u32 memsize)
{
	_int32 ret_val = SUCCESS;

    	if(is_available_ci(CI_MEM_IDX_FREE_MEM))
		return ((et_mem_free_mem)ci_ptr(CI_MEM_IDX_FREE_MEM))(mem, memsize);

#if defined(LINUX)
#ifdef _DEBUG
	free(mem);
#else

	ret_val = munmap(mem, memsize);
	if(ret_val == -1)
		ret_val = errno;
#endif
#elif defined(WINCE)
	 free(mem);
#endif

	return ret_val;
}


#ifdef _EXTENT_MEM_FROM_OS
_int32 sd_get_extent_mem_from_os(_u32 memsize, void **mem)
{
	_int32 ret_val = SUCCESS;

    	if(is_available_ci(CI_MEM_IDX_GET_MEM))
		return ((et_mem_get_mem)ci_ptr(CI_MEM_IDX_GET_MEM))(memsize, mem);

#if defined(LINUX)||defined(WINCE)
	*mem = malloc(memsize);
#endif

	if(*mem==NULL)
	{
		ret_val = OUT_OF_MEMORY;
	}

        return ret_val;	 	

}

_int32 sd_free_extent_mem_to_os(void* mem, _u32 memsize)
{
	_int32 ret_val = SUCCESS;

    	if(is_available_ci(CI_MEM_IDX_FREE_MEM))
		return ((et_mem_free_mem)ci_ptr(CI_MEM_IDX_FREE_MEM))(mem, memsize);

#if defined(LINUX)||defined(WINCE)

	free(mem);
	return ret_val;		
#endif	
}

#endif

