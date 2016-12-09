#include "utility/define.h"
#if defined(ENABLE_HSC) && !defined(ENABLE_ETM_MEDIA_CENTER)
#include "hsc_info.h"
#include "task_manager/task_manager.h"
#include "utility/string.h"

#include "utility/logid.h"
#ifdef LOGID
#undef LOGID
#endif
#define LOGID LOGID_HIGH_SPEED_CHANNEL
#include "utility/logger.h"

static BOOL g_auto_enable_hsc;
static _u64 g_hsc_remainnig_flow;

_int32 hsc_init_info(HIGH_SPEED_CHANNEL_INFO *p_hsc_info)
{
	if(p_hsc_info == NULL)
	{
		return INVALID_MEMORY;
	}
	sd_memset(p_hsc_info, 0, sizeof(HIGH_SPEED_CHANNEL_INFO));
	p_hsc_info->_channel_type   = -1;
	p_hsc_info->_errcode        = 0;
	p_hsc_info->_use_hsc        = g_auto_enable_hsc;
	p_hsc_info->_used_hsc_before = FALSE;
	g_hsc_remainnig_flow        = 0;
	LOG_DEBUG("hsc_init_info, hsc_info_ptr:0x%X", p_hsc_info);
	return SUCCESS;
}

_int32 hsc_info_set_pdt_sub_id(_u32 task_id, _u32 product_id, _u32 sub_id)
{
	_int32 ret_val = SUCCESS;
	TASK *p_task = NULL;

	ret_val = tm_get_task_by_id(task_id, &p_task);
	CHECK_VALUE(ret_val);

	p_task->_hsc_info._product_id = product_id;
	p_task->_hsc_info._sub_id = sub_id;

	LOG_DEBUG("hsc_info_set_pdt_sub_id, task_ptr:0x%X, product_id=%u, subid=%u", p_task, product_id, sub_id);
	return ret_val;
}

BOOL *hsc_get_g_auto_enable_stat()
{
	return &g_auto_enable_hsc;
}

_u64 *hsc_get_g_remainning_flow()
{
	return &g_hsc_remainnig_flow;
}

void hsc_set_force_query_permission(HIGH_SPEED_CHANNEL_INFO *p_hsc_info)
{
	LOG_DEBUG("hsc_set_force_query_permission");
	p_hsc_info->_force_query_permission = TRUE;
}
#endif /* ENABLE_HSC */

