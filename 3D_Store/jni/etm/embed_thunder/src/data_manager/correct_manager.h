#if !defined(__CORRECT_MANAGER_20080726)
#define __CORRECT_MANAGER_20080726

#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"
#include "utility/range.h"
#include "utility/list.h"
#include "connect_manager/resource.h"

typedef struct _tag_error_block
{
       RANGE _r;
	_u32 _error_count;
	_u32 _valid_resources; /*0-所有server/peer资源 1-最快单资源 2-只有原始资源*/
	LIST  _error_resources;
}ERROR_BLOCK;


typedef struct _tag_error_block_list
{       
	LIST _error_block_list;
}ERROR_BLOCKLIST;

typedef struct  _tag_correct_manager
{
        ERROR_BLOCKLIST  _error_ranages;
	 RANGE_LIST  _prority_ranages;	

	RESOURCE* _origin_res;
	 
}CORRECT_MANAGER;


/* creat error block node slab, must  eb invoke in the initial state of the download program*/
_int32 get_correct_manager_cfg_parameter(void);
_int32 init_error_block_slab(void);
_int32 destroy_error_block_slab(void);
_int32 alloc_error_block_node(ERROR_BLOCK** pp_node);
_int32 alloc_error_block_node(ERROR_BLOCK** pp_node);

/*platform of correct manager*/

_int32 init_correct_manager(CORRECT_MANAGER*  correct_m);
_int32 clear_error_block_list(ERROR_BLOCKLIST *error_block_list);
_int32 unit_correct_manager(CORRECT_MANAGER*  correct_m);

void correct_manager_set_origin_res(CORRECT_MANAGER*  correct_m,  RESOURCE*  origin_res);
_int32 correct_manager_add_prority_range(CORRECT_MANAGER*  correct_m,  RANGE* p_r);
_int32 correct_manager_del_prority_range(CORRECT_MANAGER*  correct_m,  RANGE* p_r);
_int32 correct_manager_add_prority_range_list(CORRECT_MANAGER*  correct_m,  RANGE_LIST* p_list);
_int32 correct_manager_del_prority_range_list(CORRECT_MANAGER*  correct_m,  RANGE_LIST* p_list);
_int32 correct_manager_clear_prority_range_list(CORRECT_MANAGER*  correct_m);


_int32 correct_manager_add_error_block(CORRECT_MANAGER*  correct_m,  RANGE* p_r,LIST* res_list);
_int32 set_cannot_correct_error_block(ERROR_BLOCKLIST* p_err_block_list,  RANGE* p_r);
_int32 set_cannot_correct_error_block_list(ERROR_BLOCKLIST* p_err_block_list,  RANGE_LIST* p_r_list);
_int32 correct_manager_erase_error_block(CORRECT_MANAGER*  correct_m,  RANGE* p_r);

_int32 correct_manager_inc_res_error_times(RESOURCE* res);
_int32 correct_manager_add_res_list( LIST* res_list ,LIST* new_res_list);
_int32 correct_manager_clear_res_list( LIST* res_list );
BOOL range_is_relate_error_block(ERROR_BLOCKLIST*  error_block_list,  RANGE* p_r);
void get_error_block_range_list(ERROR_BLOCKLIST*  error_block_list,  RANGE_LIST* p_range_list);

_int32 correct_manager_erase_abandon_res(CORRECT_MANAGER*  correct_m,  RESOURCE* abandon_res);

BOOL connect_manager_origin_resource_is_useable( CORRECT_MANAGER*  correct_m);
BOOL resource_is_useable( RESOURCE* p_res);

BOOL correct_manager_is_origin_resource( CORRECT_MANAGER*  correct_m, RESOURCE*  res);

BOOL correct_manager_is_relate_error_block(CORRECT_MANAGER*  correct_m,  RANGE* p_r);


#ifdef __cplusplus
}
#endif

#endif /* !defined(__CORRECT_MANAGER_20080726)*/

