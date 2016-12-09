#if !defined(__DATA_MANAGER_20080709)
#define __DATA_MANAGER_20080709

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#include "utility/range.h"
#include "utility/list.h"
#include "utility/map.h"
#include "connect_manager/resource.h"

/*
      pair:   resource*  and RANGE_LIST*
*/

 _int32  range_map_compare(void *E1, void *E2);

typedef struct _tag_range_manage
{
      MAP   res_range_map;        
	  
}RANGE_MANAGER;

_u32 init_range_manager(RANGE_MANAGER*  range_manage);

_u32 unit_range_manager(RANGE_MANAGER*  range_manage);

_u32 put_range_record(RANGE_MANAGER*  range_manage, RESOURCE *resource_ptr, const RANGE *r);

_u32 get_res_from_range(RANGE_MANAGER*  range_manage,  const RANGE *r, LIST* res_list);

BOOL  range_is_all_from_res(RANGE_MANAGER*  range_manage,  RESOURCE *res, RANGE* r);

_u32 get_range_from_res(RANGE_MANAGER*  range_manage,  RESOURCE *res, RANGE_LIST*  r_list);

_u32 range_manager_erase_resource(RANGE_MANAGER*  range_manage, RESOURCE *resource_ptr);
_u32 range_manager_erase_range(RANGE_MANAGER*  range_manage, const RANGE* r, RESOURCE* origin_res);

BOOL range_manager_has_resource(RANGE_MANAGER*  range_manage, RESOURCE *resource_ptr);
_int32 out_put_res_list(LIST* res_list);

void out_put_res_recv_records(RANGE_MANAGER*  range_manage);

void range_manager_get_download_bytes(RANGE_MANAGER* range_manage, char* host, _u64* download, _u64 filesize);

#ifdef __cplusplus
}
#endif

#endif /* !defined(__DATA_MANAGER_20080709)*/



