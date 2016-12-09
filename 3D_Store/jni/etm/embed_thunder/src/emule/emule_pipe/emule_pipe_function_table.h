/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/09/21
-----------------------------------------------------------------------------------------------------------*/
#include "utility/define.h"
#ifdef ENABLE_EMULE
#include "data_manager/file_manager_interface.h"
#include "utility/range.h"

struct tagDATA_PIPE;

void** emule_get_pipe_function_table(void);

_int32 emule_pipe_change_range_interface(struct tagDATA_PIPE* pipe, const RANGE* range, BOOL cancel_flag);

_int32 emule_pipe_get_dispatcher_range(struct tagDATA_PIPE* pipe, const RANGE* dispatcher_range, void* pipe_range);

_int32 emule_pipe_set_dispatcher_range(struct tagDATA_PIPE* pipe, const void* pipe_range, RANGE* dispatcher_range);

_u64 emule_pipe_get_file_size(struct tagDATA_PIPE* pipe);

_int32 emule_pipe_set_file_size(struct tagDATA_PIPE* pipe, _u64 filesize);

_int32 emule_pipe_get_data_buffer(struct tagDATA_PIPE* pipe, char** buffer, _u32* buffer_len);

_int32 emule_pipe_free_data_buffer(struct tagDATA_PIPE* pipe, char** buffer, _u32 buffer_len);

_int32 emule_pipe_put_data(struct tagDATA_PIPE* pipe, const RANGE* range, char** data_buffer, _u32 data_len,_u32 buffer_len, RESOURCE* res);

_int32 emule_pipe_read_data(struct tagDATA_PIPE* pipe, RANGE_DATA_BUFFER* buffer, notify_read_result callback, void* user_data);

_int32 emule_pipe_notify_dispatch_data_finish(struct tagDATA_PIPE* pipe);

_int32 emule_pipe_get_file_type(struct tagDATA_PIPE* pipe);

RANGE_LIST* emule_pipe_get_checked_range_list(struct tagDATA_PIPE* pipe);


#endif /* ENABLE_EMULE */

