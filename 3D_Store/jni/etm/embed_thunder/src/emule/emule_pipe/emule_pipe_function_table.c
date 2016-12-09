
#include "utility/define.h"
#ifdef ENABLE_EMULE
#include "emule_pipe_function_table.h"
#include "../emule_data_manager/emule_data_manager.h"
#include "../emule_interface.h"
#include "p2p_data_pipe/p2p_pipe_interface.h"
#include "download_task/download_task.h"
#include "data_manager/data_buffer.h"
#include "http_data_pipe/http_data_pipe_interface.h"
#include "ftp_data_pipe/ftp_data_pipe_interface.h"

#include "connect_manager/data_pipe.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_EMULE
#include "utility/logger.h"

static void* g_emule_pipe_function_table[] = {emule_pipe_change_range_interface, emule_pipe_get_dispatcher_range, emule_pipe_set_dispatcher_range,
										emule_pipe_get_file_size, emule_pipe_set_file_size, emule_pipe_get_data_buffer,emule_pipe_free_data_buffer,
										emule_pipe_put_data, emule_pipe_read_data, emule_pipe_notify_dispatch_data_finish, emule_pipe_get_file_type,
										emule_pipe_get_checked_range_list};

void** emule_get_pipe_function_table(void)
{
	return g_emule_pipe_function_table;
}

_int32 emule_pipe_change_range_interface(struct tagDATA_PIPE* pipe, const RANGE* range, BOOL cancel_flag)
{
	switch(pipe->_data_pipe_type)
	{
		case P2P_PIPE:
			return p2p_pipe_change_ranges(pipe, range, cancel_flag);
            
#ifdef ENABLE_EMULE_PROTOCOL
		case EMULE_PIPE:
			return emule_pipe_change_ranges(pipe, range, cancel_flag);
#endif           
		case HTTP_PIPE:
			return http_pipe_change_ranges(pipe, range);
        case FTP_PIPE:
            return ftp_pipe_change_ranges(pipe, range);

		default:
			sd_assert(FALSE);		
	}
	return SUCCESS;
}

_int32 emule_pipe_get_dispatcher_range(struct tagDATA_PIPE* pipe, const RANGE* dispatcher_range, void* pipe_range)
{
	RANGE* range = (RANGE*)pipe_range;
	range->_index = dispatcher_range->_index;
	range->_num = dispatcher_range->_num;
	return SUCCESS;
}

_int32 emule_pipe_set_dispatcher_range(struct tagDATA_PIPE* pipe, const void* pipe_range, RANGE* dispatcher_range)
{
	RANGE* range = (RANGE*)pipe_range;
	dispatcher_range->_index = range->_index;
	dispatcher_range->_num = range->_num;
	return SUCCESS;
}

_u64 emule_pipe_get_file_size(struct tagDATA_PIPE* pipe)
{
	EMULE_DATA_MANAGER* data_manager = (EMULE_DATA_MANAGER*)pipe->_p_data_manager;
	return data_manager->_file_size;
}	

_int32 emule_pipe_set_file_size(struct tagDATA_PIPE* pipe, _u64 filesize)
{
	sd_assert(FALSE);
	return SUCCESS;
}

_int32 emule_pipe_get_data_buffer(struct tagDATA_PIPE* pipe, char** buffer, _u32* buffer_len)
{
	//这里如果获取失败，那么要促发写文件，把内存写入到文件中，腾出空间来
	//暂没有实现，以后添加
	_int32 ret_val = SUCCESS;
    struct tagEMULE_DATA_MANAGER *p_data_manager = (struct tagEMULE_DATA_MANAGER *)pipe->_p_data_manager;

    ret_val= dm_get_buffer_from_data_buffer(buffer_len, buffer);
    LOG_DEBUG("[pipe = 0x%x]emule_pipe_get_data_buffer.ret:%d", pipe, ret_val);
	if( ret_val == DATA_BUFFER_IS_FULL || ret_val== OUT_OF_MEMORY )
	{
        file_info_flush_data_to_file(&p_data_manager->_file_info, FALSE);
	}
	return ret_val;
}

_int32 emule_pipe_free_data_buffer(struct tagDATA_PIPE* pipe, char** buffer, _u32 buffer_len)
{
	return dm_free_buffer_to_data_buffer(buffer_len, buffer);
}	

_int32 emule_pipe_put_data(struct tagDATA_PIPE* pipe, const RANGE* range, char** data_buffer, _u32 data_len,_u32 buffer_len, RESOURCE* res)
{
	return emule_write_data(pipe->_p_data_manager, range, data_buffer, data_len, buffer_len, res);
}

_int32 emule_pipe_read_data(struct tagDATA_PIPE* pipe, RANGE_DATA_BUFFER* buffer, notify_read_result callback, void* user_data)
{
	sd_assert(FALSE);
	return SUCCESS;
}

_int32 emule_pipe_notify_dispatch_data_finish(struct tagDATA_PIPE* pipe)
{
	EMULE_DATA_MANAGER* data_manager = (EMULE_DATA_MANAGER*)pipe->_p_data_manager;
	return dt_dispatch_at_pipe(&data_manager->_emule_task->_task, pipe);
}

_int32 emule_pipe_get_file_type(struct tagDATA_PIPE* pipe)
{
	//emule都是二进制文件
       return PIPE_FILE_BINARY_TYPE;
}	

RANGE_LIST* emule_pipe_get_checked_range_list(struct tagDATA_PIPE* pipe)
{
	EMULE_DATA_MANAGER* data_manager = (EMULE_DATA_MANAGER*)pipe->_p_data_manager;
	LOG_DEBUG("[emule_pipe = 0x%x]emule_pipe_get_checked_range_list.", pipe);
	return &data_manager->_file_info._done_ranges;
}

#endif /* ENABLE_EMULE */

