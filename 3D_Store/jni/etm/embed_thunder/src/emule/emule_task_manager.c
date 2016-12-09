#include "utility/define.h"
#ifdef ENABLE_EMULE
#include "emule_task_manager.h"
#include "emule_interface.h"
#include "emule_server/emule_server_interface.h"
#include "emule_impl.h"
#include "emule_data_manager/emule_data_manager.h"
#include "emule_query/emule_query_emule_info.h"
#include "emule_utility/emule_setting.h"
#include "emule_utility/emule_utility.h"
#include "res_query/res_query_interface.h"
#include "utility/list.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_EMULE
#include "utility/logger.h"

// 这里面的emule task只有在启动之后才会有，停止之后就没有了
static	LIST		g_emule_task_list;
static	_u32	g_timeout_id;

_int32 emule_init_task_manager(void)
{
	list_init(&g_emule_task_list);
	start_timer(emule_task_handle_timeout, NOTICE_INFINITE, 60 * 1000, 0, NULL, &g_timeout_id);
	return SUCCESS;
}
	
_int32 emule_add_task(struct tagEMULE_TASK* task)
{
	list_push(&g_emule_task_list, task);
	return SUCCESS;
}

_int32 emule_remove_task(struct tagEMULE_TASK* task)
{
	LIST_ITERATOR iter = NULL;
	for(iter = LIST_BEGIN(g_emule_task_list); iter != LIST_END(g_emule_task_list); iter = LIST_NEXT(iter))
	{
		if(task == iter->_data)
			return list_erase(&g_emule_task_list, iter);
	}
	return -1;
}

EMULE_TASK* emule_find_task(_u8 file_id[FILE_ID_SIZE])
{
	EMULE_TASK* task = NULL;
	LIST_ITERATOR iter = NULL;
	for(iter = LIST_BEGIN(g_emule_task_list); iter != LIST_END(g_emule_task_list); iter = LIST_NEXT(iter))
	{
		task = (EMULE_TASK*)LIST_VALUE(iter);
		if(sd_memcmp(file_id, task->_data_manager->_file_id, FILE_ID_SIZE) == 0)
			return task;
	}
	return NULL;
}

_int32 emule_get_data_manager_by_file_id(_u8 file_id[FILE_ID_SIZE], EMULE_DATA_MANAGER **emule_data_manager)
{
    _int32 ret = SUCCESS;
    EMULE_TASK* task = NULL;
    LIST_ITERATOR iter = NULL;
    *emule_data_manager = NULL;

    emule_log_print_file_id(file_id, "in emule_get_data_manager_by_file_id");

    for(iter = LIST_BEGIN(g_emule_task_list); iter != LIST_END(g_emule_task_list); iter = LIST_NEXT(iter))
    {
        task = (EMULE_TASK*)LIST_VALUE(iter);
        if(sd_memcmp(file_id, task->_data_manager->_file_id, FILE_ID_SIZE) == 0)
        {
            LOG_DEBUG("find emule task(0x%x) with file_id.", task);
            *emule_data_manager = task->_data_manager;
            ret = SUCCESS;
            break;
        }
    }

    return ret;
}

_int32 emule_get_task_list(const LIST** tasks_list)
{
	*tasks_list = &g_emule_task_list;
	return SUCCESS;
}

_int32 emule_task_handle_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid)
{
	EMULE_TASK* task = NULL;
	LIST_ITERATOR iter = NULL;
	for(iter = LIST_BEGIN(g_emule_task_list); iter != LIST_END(g_emule_task_list); iter = LIST_NEXT(iter))
	{
		task = (EMULE_TASK*)LIST_VALUE(iter);
		if(task->_task.task_status != TASK_S_RUNNING)
			continue;
        if( !cm_is_global_need_more_res() ) continue;

		LOG_DEBUG("MMMM[emule_task = 0x%x] need more resource, try to query.", task);
		if(emule_enable_p2sp())
		{
			if(task->_res_query_emulehub_state != RES_QUERY_REQING
               && task->_data_manager->_is_cid_and_gcid_valid == FALSE)
			{
				emule_query_emule_info(task, task->_data_manager->_file_id, task->_data_manager->_file_size);
			}
			else if (task->_data_manager->_is_cid_and_gcid_valid == TRUE)
			{
              	if(cm_is_need_more_server_res(&task->_task._connect_manager, INVALID_FILE_INDEX) )
                {
                    emule_task_shub_res_query(task);
                }
              	if(cm_is_need_more_peer_res(&task->_task._connect_manager, INVALID_FILE_INDEX) )
                {
                    emule_task_peer_res_query(task);
                }
			}
		}
        
#ifdef ENABLE_EMULE_PROTOCOL
		if(emule_enable_server())
		{
			emule_server_query_source(task->_data_manager->_file_id, task->_data_manager->_file_size);
		}
#endif /* ENABLE_EMULE */

	}
	return SUCCESS;
}
#endif /* ENABLE_EMULE */

