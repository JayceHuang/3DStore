#if !defined(__PIPE_FUNCTION_TABLE_BUILDER_20090317)
#define __PIPE_FUNCTION_TABLE_BUILDER_20090317

#ifdef __cplusplus
extern "C" 
{
#endif

#include "connect_manager/pipe_function_table.h"
#include "connect_manager/data_pipe.h"

struct tagDATA_PIPE;

void build_globla_pipe_function_table(void);

void clear_globla_pipe_function_table(void);

/*****************************************************************************
 * common_pipe change range handle
 ****************************************************************************/
_int32 common_pipe_change_range_handle( struct tagDATA_PIPE *p_data_pipe, const RANGE *p_range, BOOL cancel_flag );

/*****************************************************************************
 * common_pipe get/set dispatcher range handle
 ****************************************************************************/

_int32 common_pipe_get_dispatcher_range( struct tagDATA_PIPE *p_data_pipe, const RANGE *p_dispatcher_range, void *p_pipe_range );

_int32 common_pipe_set_dispatcher_range( struct tagDATA_PIPE *p_data_pipe, const void *p_pipe_range, RANGE *p_dispatcher_range );

/*****************************************************************************
 * common_pipe data_manager platform handle
 ****************************************************************************/

_u64 common_pipe_get_file_size( struct tagDATA_PIPE *p_data_pipe );

_int32 common_pipe_set_file_size( struct tagDATA_PIPE *p_data_pipe,  _u64 filesize);

_int32 common_pipe_get_data_buffer_handler( struct tagDATA_PIPE *p_data_pipe,  
	char **pp_data_buffer, _u32 *p_data_len );

_int32 common_pipe_free_data_buffer_handler( struct tagDATA_PIPE *p_data_pipe,  
	char **pp_data_buffer, _u32 data_len);

_int32 common_pipe_put_data_handler( struct tagDATA_PIPE *p_data_pipe,   
	const RANGE *p_range, char **pp_data_buffer, _u32 data_len,_u32 buffer_len,
	RESOURCE *p_resource );

_int32 common_pipe_read_data_handler( struct tagDATA_PIPE *p_data_pipe, RANGE_DATA_BUFFER *p_data_buffer, 
	notify_read_result p_call_back, void *p_user );

_int32 common_pipe_notify_dispatch_data_finish_handler( struct tagDATA_PIPE *p_data_pipe );

_int32 common_pipe_get_file_type_handler( struct tagDATA_PIPE *p_data_pipe );

RANGE_LIST* common_pipe_get_checked_range_list_handler( struct tagDATA_PIPE *p_data_pipe );

#ifdef ENABLE_BT  


/*****************************************************************************
 * speedup_pipe change range handle
 ****************************************************************************/

_int32 speedup_pipe_change_range_handle( struct tagDATA_PIPE *p_data_pipe, const RANGE *p_range, BOOL cancel_flag );

/*****************************************************************************
 * speedup_pipe get/set dispatcher range handle
 ****************************************************************************/

_int32 speedup_pipe_get_dispatcher_range( struct tagDATA_PIPE *p_data_pipe, const RANGE *p_dispatcher_range, void *p_pipe_range );

_int32 speedup_pipe_set_dispatcher_range( struct tagDATA_PIPE *p_data_pipe, const void *p_pipe_range, RANGE *p_dispatcher_range );

/*****************************************************************************
 * speedup_pipe data_manager platform handle
 ****************************************************************************/

_u64 speedup_pipe_get_file_size( struct tagDATA_PIPE *p_data_pipe );

_int32 speedup_pipe_set_file_size( struct tagDATA_PIPE *p_data_pipe,  _u64 filesize);

_int32 speedup_pipe_get_data_buffer_handler( struct tagDATA_PIPE *p_data_pipe,  
	char **pp_data_buffer, _u32 *p_data_len );

_int32 speedup_pipe_free_data_buffer_handler( struct tagDATA_PIPE *p_data_pipe,  
	char **pp_data_buffer, _u32 data_len);

_int32 speedup_pipe_put_data_handler( struct tagDATA_PIPE *p_data_pipe,   
	const RANGE *p_range, char **pp_data_buffer, _u32 data_len,_u32 buffer_len,
	RESOURCE *p_resource );

_int32 speedup_pipe_read_data_handler( struct tagDATA_PIPE *p_data_pipe, RANGE_DATA_BUFFER *p_data_buffer, 
	notify_read_result p_call_back, void *p_user );

_int32 speedup_pipe_notify_dispatch_data_finish_handler( struct tagDATA_PIPE *p_data_pipe );

_int32 speedup_pipe_get_file_type_handler( struct tagDATA_PIPE *p_data_pipe );

RANGE_LIST* speedup_pipe_get_checked_range_list_handler( struct tagDATA_PIPE *p_data_pipe );

/*****************************************************************************
 * bt_pipe change range handle
 ****************************************************************************/

_int32 bt_pipe_change_range_handle( struct tagDATA_PIPE *p_data_pipe, const RANGE *p_range, BOOL cancel_flag );

/*****************************************************************************
 * bt_pipe get/set dispatcher range handle
 ****************************************************************************/

_int32 bt_pipe_get_dispatcher_range( struct tagDATA_PIPE *p_data_pipe, const RANGE *p_dispatcher_range, void *p_pipe_range );

_int32 bt_pipe_set_dispatcher_range( struct tagDATA_PIPE *p_data_pipe, const void *p_pipe_range, RANGE *p_dispatcher_range );


/*****************************************************************************
 * bt_pipe get/set data_manager platform handle
 ****************************************************************************/

_u64 bt_pipe_get_file_size( struct tagDATA_PIPE *p_data_pipe );

_int32 bt_pipe_set_file_size( struct tagDATA_PIPE *p_data_pipe,  _u64 filesize);

_int32 bt_pipe_get_data_buffer_handler( struct tagDATA_PIPE *p_data_pipe,  
	char **pp_data_buffer, _u32 *p_data_len );

_int32 bt_pipe_free_data_buffer_handler( struct tagDATA_PIPE *p_data_pipe,  
	char **pp_data_buffer, _u32 data_len);

_int32 bt_pipe_put_data_handler( struct tagDATA_PIPE *p_data_pipe,  
	const RANGE *p_range, char **pp_data_buffer, _u32 data_len,_u32 buffer_len,
	RESOURCE *p_resource ); 

_int32 bt_pipe_read_data_handler( struct tagDATA_PIPE *p_data_pipe, RANGE_DATA_BUFFER *p_data_buffer, 
	notify_read_result p_call_back, void *p_user );

_int32 bt_pipe_notify_dispatch_data_finish_handler( struct tagDATA_PIPE *p_data_pipe );

_int32 bt_pipe_notify_get_file_type_handler( struct tagDATA_PIPE *p_data_pipe );

RANGE_LIST* bt_pipe_get_checked_range_list_handler( struct tagDATA_PIPE *p_data_pipe );

#endif /* #ifdef ENABLE_BT  */


#ifdef __cplusplus
}
#endif

#endif // !defined(__PIPE_FUNCTION_TABLE_BUILDER_20090317)



