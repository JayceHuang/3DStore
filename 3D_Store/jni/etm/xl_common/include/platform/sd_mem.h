#ifndef SD_MEM_H_00138F8F2E70_200806111929
#define SD_MEM_H_00138F8F2E70_200806111929
#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"

_int32 set_customed_interface_mem(_int32 fun_idx, void *fun_ptr);
_int32 sd_get_mem_from_os(_u32 memsize, void **mem);
_int32 sd_free_mem_to_os(void* mem, _u32 memsize);

#ifdef _EXTENT_MEM_FROM_OS
_int32 sd_get_extent_mem_from_os(_u32 memsize, void **mem);
_int32 sd_free_extent_mem_to_os(void* mem, _u32 memsize);
#endif
#ifdef __cplusplus
}
#endif
#endif
