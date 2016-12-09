#include "utility/define.h"

#if defined(ENABLE_HSC) && !defined(ENABLE_ETM_MEDIA_CENTER)
#include "utility/errcode.h"
#include "utility/list.h"
#include "utility/map.h"
#include "utility/logid.h"
#define LOGID LOGID_HIGH_SPEED_CHANNEL
#include "utility/logger.h"

#include "hsc_interface.h"
#include "hsc_info.h"
#include "hsc_perm_query.h"
#include "hsc_flux_query.h"
#include "connect_manager/connect_manager_interface.h"
#include "connect_manager/connect_manager_imp.h"
#include "task_manager/task_manager.h"
#include "p2sp_download/p2sp_task.h"
#include "data_manager/data_manager_interface.h"
#include "emule/emule_interface.h"
#include "bt_download/bt_task/bt_task.h"

#define MAKE_CLIENT_VER(a, b, c, d) ( ((a) << 24) + ((b) << 16) + ((c) << 8) + d )

static _u32 g_hsc_business_flag = 16;
static _u32 g_hsc_client_ver = MAKE_CLIENT_VER(1, 0, 2, 0);          //default 1.0.2.xxxx

static _int32 hsc_start_commit(TASK* p_task, char* task_name, const _u32 name_length, const _u32 file_index)
{
    if (p_task == NULL)
    {
        LOG_DEBUG("bad param");
        return FALSE;
    }
    HSC_BATCH_COMMIT_PARAM *param = NULL;
    _int32 ret_val = sd_malloc(sizeof(HSC_BATCH_COMMIT_PARAM), (void**)&param);
    if (ret_val != SUCCESS)
    {
        LOG_DEBUG("malloc HSC_BATCH_COMMIT_PARAM error:%d", ret_val);
        return ret_val;
    }
    sd_memset(param, 0, sizeof(HSC_BATCH_COMMIT_PARAM));
    param->_p_task = p_task;
    list_init(&param->_lst_file_index);
    
    switch(p_task->_task_type)
    {
        case P2SP_TASK_TYPE:
        {
            P2SP_TASK *p_p2sp_task = (P2SP_TASK*)p_task;
            ret_val = pt_get_task_gcid(p_task, param->_p_gcid);
            if (ret_val != SUCCESS)
            {
                LOG_DEBUG("get p2sp task:0x%x gcid err:%d", p_task, ret_val);
                return ret_val;
            }
            param->_file_size = dm_get_file_size(&p_p2sp_task->_data_manager);
            ret_val = pt_get_task_tcid(p_task, param->_p_cid);
            if (ret_val != SUCCESS)
            {
                LOG_DEBUG("get p2sp task:0x%x cid err:%d", p_task, ret_val);
				SAFE_DELETE(param);//add by tlm
				return ret_val;
            }
            param->_p_task_name = task_name;
            param->_task_name_length = name_length;
            break;
        
        }
#if defined(ENABLE_EMULE) && defined(ENABLE_EMULE_PROTOCOL)
        case EMULE_TASK_TYPE:
        {
            EMULE_TASK *p_emule_task = (EMULE_TASK*)p_task;
            ret_val = emule_get_task_gcid(p_task, param->_p_gcid);
            if (ret_val != SUCCESS)
            {
                LOG_DEBUG("get emule task:0x%x gcid err:%d", p_task, ret_val);
                SAFE_DELETE(param);//add by tlm
				return ret_val;
            }
            param->_file_size = emule_get_file_size( (void*)p_emule_task->_data_manager);
            ret_val = emule_get_task_cid(p_task, param->_p_cid);
            if (ret_val != SUCCESS)
            {
                LOG_DEBUG("get emule task:0x%x cid err:%d", p_task, ret_val);
                SAFE_DELETE(param);//add by tlm
				return ret_val;
            }
            param->_p_task_name = task_name;
            param->_task_name_length = name_length;
            break;        
        }
#endif /* emule */

#if defined(ENABLE_BT) && defined(ENABLE_BT_PROTOCOL)
        case BT_TASK_TYPE:
        {
            if(file_index != MAX_U32)
            {
                LOG_DEBUG("sliently commiting bt task:%d, file_index:%d", p_task->_task_id, file_index);
                list_push(&param->_lst_file_index, (void*)file_index);
            }
            break;
        }
#endif /* BT */
        default:
            sd_assert(FALSE);
    }

    return hsc_batch_commit(param, 1);
}

_int32 hsc_enable(void * _param)
{
    _int32      ret_val     = SUCCESS;
	HSC_SWITCH  *_p_param   = (HSC_SWITCH*)_param;
	TASK        *p_task     = NULL;
	
	_p_param->_result = tm_get_task_by_id(_p_param->_task_id, &p_task);

	if(_p_param->_result == SUCCESS)
	{
        LOG_DEBUG("on hsc_enable taskid:%u, res_num:%d, can_use:%d", _p_param->_task_id, p_task->_hsc_info._res_num, p_task->_hsc_info._can_use);
		if(p_task->_hsc_info._can_use != TRUE || p_task->_hsc_info._res_num == 0)
		{
			_p_param->_result = HSC_NOT_AVAILABLE;
			LOG_DEBUG("EEEEEE hsc_enable, taskid:%u, enable error, resource not found", _p_param->_task_id);
			return signal_sevent_handle(&(_p_param->_handle));
		}

        LOG_DEBUG("on hsc_enable task:0x%x has_pay:%d _stat:%d...", p_task, _p_param->_hsc_has_pay, p_task->_hsc_info._stat);
        if(_p_param->_hsc_has_pay == TRUE)
        {
            if(p_task->_hsc_info._stat == HSC_IDLE)
            {
                p_task->_hsc_info._stat = HSC_SUCCESS;
                _p_param->_result = SUCCESS;
                p_task->_hsc_info._errcode = 0;
                p_task->_hsc_info._channel_type = 2;
                p_task->_hsc_info._use_hsc = TRUE;
                tm_update_task_hsc_info();
                if(p_task->_task_type == BT_TASK_TYPE)
                {
                    hsc_start_commit(p_task, _p_param->_hsc_task_name, _p_param->_hsc_task_name_length, MAX_U32);
                }
                else
                {
                    cm_enable_high_speed_channel(&p_task->_connect_manager, 0, TRUE);
                    cm_update_cdn_pipes(&p_task->_connect_manager);
                }

                //LOG_DEBUG("on hsc_enable, the task:%u has pay, so set hsc connect flags and update pipes", _p_param->_task_id);
                return signal_sevent_handle(&(_p_param->_handle));
            }
            else if(p_task->_hsc_info._stat == HSC_BT_SUBTASK_FAILED && p_task->_task_type == BT_TASK_TYPE)
            {
                hsc_start_commit(p_task, _p_param->_hsc_task_name, _p_param->_hsc_task_name_length, MAX_U32);
            }
        }
        if(p_task->_hsc_info._stat == HSC_ENTERING)
        {
			_p_param->_result = HSC_IS_ENTERING;
			LOG_DEBUG("EEEEEE hsc_enable, taskid:%u, enable error, hsc is entering, duplicate enable action", _p_param->_task_id);
			return signal_sevent_handle(&(_p_param->_handle));
        }
        else if(p_task->_hsc_info._stat == HSC_SUCCESS)
        {
			_p_param->_result = HSC_IN_USE;
			LOG_DEBUG("EEEEEE hsc_enable, taskid:%u, enable error, hsc is in use, duplicate enable action", _p_param->_task_id);
			return signal_sevent_handle(&(_p_param->_handle));
        }
        
        ret_val = hsc_start_commit(p_task, _p_param->_hsc_task_name, _p_param->_hsc_task_name_length, MAX_U32);
        if(ret_val != SUCCESS)
        {
            LOG_DEBUG("on hsc_enable hsc_start_commit error:%d", ret_val);
        }
        else
        {
            LOG_DEBUG("on hsc_enable hsc_start_commit ok");
        }
        //jjxh added...
	}
	return signal_sevent_handle(&(_p_param->_handle));
}


_int32 hsc_disable(void * _param)
{
	HSC_SWITCH *_p_param = (HSC_SWITCH*)_param;
	CONNECT_MANAGER *p_connect_manager = NULL;
	TASK *p_task = NULL;
	_p_param->_result = tm_get_task_by_id(_p_param->_task_id, &p_task);

    LOG_DEBUG("on hsc_disable start taskid:%d", _p_param->_task_id);
	if(_p_param->_result == SUCCESS)
	{
		p_task->_hsc_info._use_hsc = FALSE;
		p_connect_manager = &(p_task->_connect_manager);
        LOG_DEBUG("on hsc_disable _stat:%d", p_task->_hsc_info._stat);
		if((p_task->_hsc_info._stat != HSC_SUCCESS) && (p_task->_hsc_info._stat != HSC_STOP))
		{
			p_task->_hsc_info._stat = HSC_IDLE;
            cm_disable_high_speed_channel(p_connect_manager); // set enable flag false
			_p_param->_result = HSC_NOT_IN_USE;
		}
		else
		{
			p_task->_hsc_info._stat = HSC_IDLE;
			cm_disable_high_speed_channel(p_connect_manager); // set enable flag false
			cm_update_cdn_pipes(p_connect_manager); // disable immediately
		}
	}
	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 hsc_set_product_sub_id(void * _param)
{
	HSC_PRODUCT_SUB_ID_INFO *_p_param = (HSC_PRODUCT_SUB_ID_INFO*)_param;

	_p_param->_result = hsc_info_set_pdt_sub_id(_p_param->_task_id, _p_param->_product_id, _p_param->_sub_id);

	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 hsc_auto_sw(void *_param)
{
	HSC_AUTO_SWITCH *_p_param = (HSC_AUTO_SWITCH*)_param;
	LIST *p_task_list = NULL;
	LIST_ITERATOR cur_node = NULL, end_node = NULL;
	TASK *p_task = NULL;

	*hsc_get_g_auto_enable_stat() = _p_param->_enable;
	LOG_DEBUG("hsc_auto_sw, %d", _p_param->_enable);
	tm_get_task_list(&p_task_list);
	if(p_task_list)
	{
		cur_node = LIST_BEGIN(*p_task_list);
		end_node = LIST_END(*p_task_list);

		while(cur_node != end_node)
		{
			p_task = (TASK*)LIST_VALUE(cur_node);
			if(p_task)
			{
				p_task->_hsc_info._use_hsc = _p_param->_enable;
			}
			cur_node = LIST_NEXT(cur_node);
		}
	}
	_p_param->_result = SUCCESS;

	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 hsc_set_used_hsc_before(void * _param)
{
	HSC_USED_BEFORE_FLAG *_p_param = (HSC_USED_BEFORE_FLAG*)_param;
	TASK *p_task = NULL;
	_p_param->_result = tm_get_task_by_id(_p_param->_task_id, &p_task);
	if(_p_param->_result == SUCCESS)
	{
		p_task->_hsc_info._used_hsc_before = _p_param->_used_before;
		LOG_DEBUG("hsc_set_used_hsc_before, task_ptr:0x%x, _used_before:%d, ", p_task, _p_param->_used_before);
	}
	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 hsc_is_auto(void * _param)
{
	HSC_AUTO_INFO *_p_param = (HSC_AUTO_INFO*)_param;
	if(NULL == _p_param->_p_is_auto)
	{
		_p_param->_result = INVALID_MEMORY;
		return signal_sevent_handle(&(_p_param->_handle));
	}
	*(_p_param->_p_is_auto) = *hsc_get_g_auto_enable_stat();
	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 hsc_handle_auto_enable(TASK *p_task, _u32 file_index, _u8 *gcid, _u64 file_size )
{
    _int32      ret_val = 0;
    p_task->_hsc_info._res_num++;
    LOG_DEBUG("hsc_handle_auto_enable, added vip resource, res_num:%u, hsc_stat:%d", p_task->_hsc_info._res_num, p_task->_hsc_info._stat);

    if(p_task->_hsc_info._res_num > 0)
        p_task->_hsc_info._can_use = TRUE;

    tm_update_task_hsc_info();

    LOG_DEBUG("on hsc_handle_auto_enable task_type:%d, _stat:%d, file_index:%d",\
            p_task->_task_type, p_task->_hsc_info._stat, file_index);
    if(p_task->_task_type == BT_TASK_TYPE)
    {
        ret_val = cm_check_high_speed_channel_flag(&p_task->_connect_manager, file_index);
        LOG_DEBUG("on hsc_handle_auto_enable sub task flag:%d", ret_val);
        if((p_task->_hsc_info._stat == HSC_SUCCESS || p_task->_hsc_info._stat == HSC_BT_SUBTASK_FAILED) && ret_val == FALSE)
        {
            cm_enable_high_speed_channel(&p_task->_connect_manager, file_index, 2);
            hsc_start_commit(p_task, NULL, 0, file_index);
        }
    }
    return SUCCESS;
}

_int32 hsc_set_business_flag(void* param)
{
    HSC_BUSINESS_FLAG* _param = (HSC_BUSINESS_FLAG*)param;
    LOG_DEBUG("on hsc_set_business_flag:%d, client_ver:%d", _param->_flag, _param->_client_ver);
    g_hsc_business_flag = _param->_flag;
    g_hsc_client_ver    = _param->_client_ver;
    _param->_result = SUCCESS;
	return signal_sevent_handle(&(_param->_handle));
}

_int32 hsc_query_flux_info(void* _param)
{
    HSC_QUERY_FLUX_INFO* _p_param = (HSC_QUERY_FLUX_INFO*)_param;
    LOG_DEBUG("on hsc_query_flux_info param:0x%x", _p_param);
    
    _p_param->_result = hsc_query_hsc_flux_info(_p_param->_p_callback);
    return signal_sevent_handle(&(_p_param->_handle));
}

_int32 hsc_get_business_flag()
{
    LOG_DEBUG("on hsc_get_business_flag:%d", g_hsc_business_flag);
    return g_hsc_business_flag;
}

_int32 hsc_get_client_ver()
{
    LOG_DEBUG("on hsc_get_client_ver:%d", g_hsc_client_ver);
    return g_hsc_client_ver;
}

#else
/* 仅仅是为了在MACOS下能顺利编译 */
#if defined(MACOS)&&(!defined(MOBILE_PHONE))
void hsc_test(void)
{
	return ;
}
#endif /* defined(MACOS)&&(!defined(MOBILE_PHONE)) */

#endif /* ENABLE_HSC */


