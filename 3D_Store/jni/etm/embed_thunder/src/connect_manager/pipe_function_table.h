#if !defined(__PIPE_FUNCTION_TABLE_20080920)
#define __PIPE_FUNCTION_TABLE_20080920

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/range.h"
#include "data_manager/file_manager_interface.h"

typedef enum tagPIPE_FUNCTION_TABLE_INDEX
{
    CHANGE_PIPE_RANGE_FUNC = 0,
    GET_DISPATCHER_RANGE_FUNC,
    SET_DISPATCHER_RANGE_FUNC,
    GET_FILE_SIZE_FUNC,
    SET_FILE_SIZE_FUNC,
    GET_DATA_BUFFER_FUNC,
    FREE_DATA_BUFFER_FUNC,
    PUT_DATA_FUNC,
    READ_DATA_FUNC,
    NOTIFY_DISPATCH_DATA_FINISHED_FUNC,
    GET_FILE_TYPE_FUNC,
    GET_CHECKED_RANGE_LIST_FUNC,

    MAX_FUNC_NUM
}PIPE_FUNCTION_TABLE_INDEX;

struct tagDATA_PIPE;
struct tagRESOURCE;
struct _tag_range_data_buffer;


/*****************************************************************************
 * handler function:
 * dispatcher need to call the pri_change_range_handler to change range.
 *****************************************************************************/

typedef _int32 (*change_range_handler)(struct tagDATA_PIPE *p_data_pipe, const RANGE *range, BOOL cancel_flag);

/*****************************************************************************
 * handler function:
 * pipe module need to call the function to get the download_range used by dispatcher.
 *****************************************************************************/

typedef _int32 (*get_dispatcher_range_handler)(struct tagDATA_PIPE *p_data_pipe, 
const RANGE *p_dispatcher_range, void *p_pipe_range);

typedef _int32 (*set_dispatcher_range_handler)(struct tagDATA_PIPE *p_data_pipe, 
	const void *p_pipe_range, RANGE *p_dispatcher_range);

/*****************************************************************************
 * handler function:
 * pipe module need these functions to call the platform of data_manager
 *****************************************************************************/

typedef _u64 (*get_file_size_handler)(struct tagDATA_PIPE *p_data_pipe);

typedef _int32 (*set_file_size_handler)(struct tagDATA_PIPE *p_data_pipe, _u64 filesize);

typedef _int32 (*get_data_buffer_handler)(struct tagDATA_PIPE *p_data_pipe,
	char **pp_data_buffer, _u32 *p_data_len );

typedef _int32 (*free_data_buffer_handler)(struct tagDATA_PIPE *p_data_pipe,
	char **pp_data_buffer, _u32 data_len);

typedef _int32 (*put_data_handler)(struct tagDATA_PIPE *p_data_pipe, 
	const RANGE *p_range, char **pp_data_buffer, _u32 data_len, _u32 buffer_len,
	struct tagRESOURCE *p_resource );

typedef _int32 (*read_data_handler)(struct tagDATA_PIPE *p_data_pipe, struct _tag_range_data_buffer *p_data_buffer, 
	notify_read_result p_call_back, void *p_user );

typedef _int32 (*notify_dispatch_data_finish_handler)(struct tagDATA_PIPE *p_data_pipe);

typedef _int32 (*get_file_type_handler)(struct tagDATA_PIPE *p_data_pipe);

typedef RANGE_LIST* (*get_checked_range_list_handler)(struct tagDATA_PIPE *p_data_pipe);

/*****************************************************************************
 * get function table function:
 *****************************************************************************/

void **pft_get_common_pipe_function_table(void);

#ifdef ENABLE_BT 
void **pft_get_speedup_pipe_function_table(void);

void **pft_get_bt_pipe_function_table(void);
#endif

/*****************************************************************************
 * register function:
 *****************************************************************************/
void pft_register_change_range_handler(void **p_func_table, change_range_handler p_change_range_handler);

void pft_register_get_dispatcher_range_handler(void **p_func_table, get_dispatcher_range_handler p_change_range_handler);

void pft_register_set_dispatcher_range_handler(void **p_func_table, set_dispatcher_range_handler p_change_range_handler);

void pft_register_get_file_size_handler(void **p_func_table, get_file_size_handler p_get_file_size_handler);

void pft_register_set_file_size_handler(void **p_func_table, set_file_size_handler p_set_file_size_handler);

void pft_register_get_data_buffer_handler(void **p_func_table, get_data_buffer_handler p_get_data_buffer_handler);

void pft_register_free_data_buffer_handler(void **p_func_table, free_data_buffer_handler p_free_data_buffer_handler);

void pft_register_put_data_handler(void **p_func_table, put_data_handler p_put_data_handler);

void pft_register_read_data_handler(void **p_func_table, read_data_handler p_put_data_handler);

void pft_register_notify_dispatch_data_finish_handler(void **p_func_table, notify_dispatch_data_finish_handler p_notify_dispatch_data_finish_handler);

void pft_register_get_file_type_handler(void **p_func_table, get_file_type_handler p_get_file_type_handler);

void pft_register_get_checked_range_list_handler(void **p_func_table, get_checked_range_list_handler p_get_checked_range_list_handler);

void pft_clear_func_table(void **p_func_table);

#ifdef __cplusplus
}
#endif

#endif // !defined(__PIPE_FUNCTION_TABLE_20080920)

