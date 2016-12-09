/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/11/4
-----------------------------------------------------------------------------------------------------------*/
#ifndef _EMULE_TASK_MANAGER_H_
#define _EMULE_TASK_MANAGER_H_
#include "utility/define.h"
#ifdef ENABLE_EMULE

#include "emule/emule_utility/emule_define.h"
#include "asyn_frame/msg.h"
#include "utility/list.h"
#include "emule/emule_data_manager/emule_data_manager.h"

struct tagEMULE_TASK;

_int32 emule_init_task_manager(void);

_int32 emule_add_task(struct tagEMULE_TASK* task);

_int32 emule_remove_task(struct tagEMULE_TASK* task);

struct tagEMULE_TASK* emule_find_task(_u8 file_id[FILE_ID_SIZE]);

_int32 emule_get_task_list(const LIST** tasks_list);

_int32 emule_task_handle_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid);

_int32 emule_get_data_manager_by_file_id(_u8 file_id[FILE_ID_SIZE], EMULE_DATA_MANAGER **emule_data_manager);

#endif /* ENABLE_EMULE */

#endif


