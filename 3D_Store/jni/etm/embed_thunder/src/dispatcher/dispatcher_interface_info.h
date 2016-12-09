
#if !defined(__DISPATCHER_INTERFACE_INFO_20090323)
#define __DISPATCHER_INTERFACE_INFO_20090323

#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"
#include "utility/range.h"
#include "connect_manager/resource.h"
#include "data_manager/correct_manager.h"


typedef _u64  (*ds_get_file_size)( void* p_owner_data_manager);
typedef BOOL (*ds_is_only_using_origin_res)(void* p_owner_data_manager);
typedef BOOL (*ds_is_origin_resource)( void* p_owner_data_manager, RESOURCE*  res);
typedef  _int32 (*ds_get_priority_range)(void* p_owner_data_manager, RANGE_LIST*  priority_range_list);
typedef _int32 (*ds_get_uncomplete_range)(void* p_owner_data_manager, RANGE_LIST*  un_complete_range_list);
typedef _int32 (*ds_get_error_range_block_list)(void* p_owner_data_manager, ERROR_BLOCKLIST**  pp_error_block_list);		
typedef BOOL (*ds_is_vod_mod)(void* p_owner_data_manager);	




typedef struct _tag_ds_data_intereface
{
      ds_get_file_size  _get_file_size;
      ds_is_only_using_origin_res _is_only_using_origin_res;
      ds_is_origin_resource _is_origin_resource;
      ds_get_priority_range _get_priority_range;
      ds_get_uncomplete_range _get_uncomplete_range;
      ds_get_error_range_block_list  _get_error_range_block_list;	
      ds_is_vod_mod _get_ds_is_vod_mode;  
	   
      void* _own_ptr; 
}DS_DATA_INTEREFACE;

#define  DS_GET_FILE_SIZE(_DATA_INTEREFACE)  ((_DATA_INTEREFACE)._get_file_size)((_DATA_INTEREFACE)._own_ptr)
#define  DS_IS_ONLY_USING_ORIGIN_RES(_DATA_INTEREFACE)  ((_DATA_INTEREFACE)._is_only_using_origin_res)((_DATA_INTEREFACE)._own_ptr)
#define  DS_IS_ORIGIN_RESOURCE(_DATA_INTEREFACE, RES)  ((_DATA_INTEREFACE)._is_origin_resource)((_DATA_INTEREFACE)._own_ptr, RES)
#define  DS_GET_PRIORITY_RANGE(_DATA_INTEREFACE, PRIORITY_RANGE_LIST)  (0) //((_DATA_INTEREFACE)._get_priority_range)((_DATA_INTEREFACE)._own_ptr, PRIORITY_RANGE_LIST)
#define  DS_GET_PRIORITY_RANGE_VOD(_DATA_INTEREFACE, PRIORITY_RANGE_LIST)  ((_DATA_INTEREFACE)._get_priority_range)((_DATA_INTEREFACE)._own_ptr, PRIORITY_RANGE_LIST)
#define  DS_GET_UNCOMPLETE_RANGE(_DATA_INTEREFACE, UNCOMPLETE_RANGE_LIST)  ((_DATA_INTEREFACE)._get_uncomplete_range)((_DATA_INTEREFACE)._own_ptr, UNCOMPLETE_RANGE_LIST)
#define  DS_GET_ERROR_RANGE_BLOCK_LIST(_DATA_INTEREFACE, ERROR_BLOCK_LIST)  ((_DATA_INTEREFACE)._get_error_range_block_list)((_DATA_INTEREFACE)._own_ptr, ERROR_BLOCK_LIST)
#define  DS_IS_VOD_MODE(_DATA_INTEREFACE)  ((_DATA_INTEREFACE)._get_ds_is_vod_mode)((_DATA_INTEREFACE)._own_ptr)

#ifdef __cplusplus
}
#endif

#endif /* !defined(__DISPATCHER_INTERFACE_INFO_20090323)*/


