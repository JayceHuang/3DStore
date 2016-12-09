/*******************************************************************
 *  涓栫晫鏈棩灏辫鏉ヤ簡
 *      
 *      created by jjxh        
 *              2012-12-19
 * ****************************************************************/

#include "utility/define.h"
#ifdef ENABLE_HSC
#include "utility/logid.h"
#ifdef LOGID
#undef LOGID
#endif
#define LOGID LOGID_HIGH_SPEED_CHANNEL
#include "utility/logger.h"

#include "utility/string.h"
#include "utility/mempool.h"
#include "hsc_data_pipe_wrapper.h"
//#include "utility/mempool.h"

static HSC_DATA_PIPE_MANAGER    g_hsc_data_pipe_manager;
static BOOL                     g_hsc_hdpi_modle_inited = FALSE;

_int32 hsc_init_hdpi_modle()
{
    if(g_hsc_hdpi_modle_inited) return SUCCESS;
    LOG_DEBUG("hsc_init_hdpi_modle start...");
	list_init(&g_hsc_data_pipe_manager._lst_pipes);
    g_hsc_hdpi_modle_inited = TRUE;
    return SUCCESS;
}

_int32  hsc_init_hdpi(HSC_DATA_PIPE_INTERFACE* p_HDPI, const char* host, const _u32 host_len, const _u32 port)
{
    _int32      ret_val     = SUCCESS;
    LOG_DEBUG("hsc_init_hdpi start...");
    hsc_init_hdpi_modle();
    sd_assert(p_HDPI);
    sd_memset(p_HDPI, 0, sizeof(HSC_DATA_PIPE_INTERFACE));
    p_HDPI->_status         = HDPS_READY;

    ret_val = sd_malloc(HDPS_UPBOUND * sizeof(HSC_DATA_PIPE_INTERFACE_CALLBACK), (void**)&p_HDPI->_p_callbacks);
    if(ret_val != SUCCESS)
    {
        LOG_DEBUG("EEEE on hsc_init_http_hdpi malloc callbacks error:%d", ret_val);
        goto HANDLE_ERROR;
    }
    sd_memset(p_HDPI->_p_callbacks, 0, HDPS_UPBOUND * sizeof(HSC_DATA_PIPE_INTERFACE_CALLBACK));
    ret_val = sd_malloc(host_len + 1, (void**)&p_HDPI->_p_host);
    if(ret_val != SUCCESS)
    {
        LOG_DEBUG("EEEE on hsc_init_http_hdpi malloc host error:%d", ret_val);
        goto HANDLE_ERROR;
    }
    p_HDPI->_host_len = host_len;
    sd_memset(p_HDPI->_p_host, 0, host_len + 1);
    sd_memcpy(p_HDPI->_p_host, host, host_len);
    return ret_val;

HANDLE_ERROR:
    hsc_uninit_hdpi(p_HDPI);
    return ret_val;
}

_int32  hsc_uninit_hdpi(HSC_DATA_PIPE_INTERFACE* p_HDPI)
{

    _int32                              ret_val                 = SUCCESS;
    HSC_DATA_PIPE_INTERFACE_SEND_FUNC   cancel_func             = NULL;  
    LOG_DEBUG("hsc_uninit_hdpi:0x%x", p_HDPI);
    sd_assert(p_HDPI);

    hsc_cancel_sending(p_HDPI);

    if(p_HDPI->_p_host && p_HDPI->_host_len)
    {
        sd_free(p_HDPI->_p_host);
        p_HDPI->_p_host = NULL;
        p_HDPI->_host_len = 0;
    }
    if(p_HDPI->_p_callbacks)
    {
        sd_free(p_HDPI->_p_callbacks);
        p_HDPI->_p_callbacks = NULL;
    }
    if(p_HDPI->_p_send_data && p_HDPI->_send_data_len)
    {
        sd_free(p_HDPI->_p_send_data);
        p_HDPI->_p_send_data = NULL;
        p_HDPI->_send_data_len = 0;
    }
    if(p_HDPI->_p_recv_data && p_HDPI->_recv_data_len)
    {
        sd_free(p_HDPI->_p_recv_data);
        p_HDPI->_p_recv_data = NULL;
        p_HDPI->_recv_data_len = 0;
    }
    hsc_remove_pipe_from_manager(p_HDPI);
    return ret_val;
}

_int32  hsc_send_data(HSC_DATA_PIPE_INTERFACE* p_HDPI, const void* p_data, const _int32 data_len)
{
    _int32                              ret_val             = SUCCESS;
    LOG_DEBUG("on hsc_send_data HDPI:0x%x, p_data:0x%x, data_len:%d", p_HDPI, p_data, data_len);
    sd_assert(p_HDPI && p_data && data_len);

    ret_val = sd_malloc(data_len, (void**)&p_HDPI->_p_send_data);
    if(ret_val != SUCCESS)
    {
        LOG_DEBUG("EEEE on hsc_send_data malloc send_data error:%d", ret_val);
    }
    sd_memcpy(p_HDPI->_p_send_data, p_data, data_len);
    p_HDPI->_send_data_len = data_len;

    ret_val = hsc_do_send_data(p_HDPI);
    if(ret_val == SUCCESS)
        hsc_add_pipe_to_manager(p_HDPI);
    return ret_val;
}

_int32  hsc_set_speed_limit_ex(HSC_DATA_PIPE_INTERFACE* p_HDPI)
{
    _int32      ret_val     = SUCCESS;
    LOG_DEBUG("on hsc_set_speed_limit_ex retry_count:%d", p_HDPI->_retry_count);
    if(p_HDPI->_retry_count == 1 || p_HDPI->_retry_count == 0)
    {
    //NOTE jieouy:目前登录和高速通道协议都是用的下载库的data_pipe,限速都会被影响。另外socket_proxy_set_speed_limit没有返回值。review代码时注释掉。同时这个函数引入了不必要的物理依赖
        //ret_val = socket_proxy_set_speed_limit();
    }
    return ret_val;
}

_int32  hsc_do_send_data(HSC_DATA_PIPE_INTERFACE* p_HDPI)
{
    _int32  ret_val     = SUCCESS;
    HSC_DATA_PIPE_INTERFACE_SEND_FUNC   send_func           = NULL;  
    LOG_DEBUG("on hsc_do_send_data HDPI:0x%x", p_HDPI);
    sd_assert(p_HDPI);

    send_func = (HSC_DATA_PIPE_INTERFACE_SEND_FUNC)p_HDPI->_p_send_func;
    LOG_DEBUG("on hsc_do_send_data send_func:0x%x", send_func);
    if(send_func)
    {

        //hsc_set_speed_limit_ex(p_HDPI);       闄愰�熶細杩炴垜浠嚜宸变篃缁欓檺鍒舵帀锛屾墍浠ヤ笉闄愪簡銆傘�傘�傘�俹rz...
        ret_val = send_func(p_HDPI);
        if(ret_val == SUCCESS)
        {
            hsc_start_pipe_timeout_timer(p_HDPI);
            hsc_fire_http_data_pipe_event(p_HDPI);
        }
        return ret_val;
    }
    LOG_DEBUG("EEEE on hsc_do_send_data send func not impl!!!!");
    return FALSE;
}

_int32  hsc_cancel_sending(HSC_DATA_PIPE_INTERFACE* p_HDPI)
{
    _int32                              ret_val             = SUCCESS;
    HSC_DATA_PIPE_INTERFACE_SEND_FUNC   cancel_send_func    = NULL;  
    LOG_DEBUG("on hsc_cancel_sending HDPI:0x%x", p_HDPI);
    sd_assert(p_HDPI);

    if(p_HDPI->_status == HDPS_RESVING || p_HDPI->_status == HDPS_TIMEOUT \
    || p_HDPI->_status == HDPS_WAITTING_FOR_RETRY)
    {
        hsc_cancel_pipe_timer(p_HDPI);
        cancel_send_func = p_HDPI->_p_cancel_send_func;
        if(cancel_send_func)
        {
            ret_val = cancel_send_func(p_HDPI);
            if(ret_val == SUCCESS)
                p_HDPI->_status = HDPS_READY;
            return ret_val;
        }
        LOG_DEBUG("EEEE on hsc_cancel_sending cancel_sending func not impl!!!!");
    }
    return FALSE;
}

_int32 hsc_handle_hsc_data_pipe_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid)
{
    _int32      ret_val                         = SUCCESS;
    HSC_DATA_PIPE_INTERFACE*    p_HDPI          = NULL;
    LOG_DEBUG("on hsc_handle_hsc_data_pipe_timeout msgid:%d, errcode = %d", msgid, errcode);
    if(errcode != MSG_TIMEOUT)
        return SUCCESS;

    if(NULL == msg_info)
        return INVALID_MEMORY;

    p_HDPI = (HSC_DATA_PIPE_INTERFACE*)msg_info->_user_data;
    if(NULL == p_HDPI)
        return INVALID_MEMORY;

    hsc_cancel_sending(p_HDPI);
    p_HDPI->_status = HDPS_TIMEOUT;
    LOG_DEBUG("on hsc_handle_hsc_data_pipe_timeout retry_count:%d", p_HDPI->_retry_count);
    if(p_HDPI->_retry_count)
    {
        LOG_DEBUG("on hsc_handle_hsc_data_pipe_timeout retry_interval:%d", p_HDPI->_retry_interval);
        if(p_HDPI->_retry_interval)
        {
            LOG_DEBUG("on hsc_handle_hsc_data_pipe_timeout waitting retry");
            hsc_start_retry_interval_timer(p_HDPI);
            hsc_fire_http_data_pipe_event(p_HDPI);
        }
        else
        {
            ret_val = hsc_do_send_data(p_HDPI);
            p_HDPI->_retry_count--;
            LOG_DEBUG("on hsc_handle_hsc_data_pipe_timeout retry immediately:%d", ret_val);
        }
    }
    else
    {
        //notify to up layer
        hsc_fire_http_data_pipe_event(p_HDPI);
    }
    return SUCCESS;
}

_int32 hsc_fire_http_data_pipe_event(HSC_DATA_PIPE_INTERFACE* p_HDPI)
{
    _int32      ret_val     = SUCCESS;
    HSC_DATA_PIPE_INTERFACE_CALLBACK callback   = NULL;
    sd_assert(p_HDPI);
    callback = p_HDPI->_p_callbacks[p_HDPI->_status];
    LOG_DEBUG("on hsc_fire_http_data_pipe_event callback:0x%x, type:%d", callback, p_HDPI->_status);
    if(callback)
    {
        ret_val = callback(p_HDPI);
        LOG_DEBUG("on hsc_fire_http_data_pipe_event end ret:%d", ret_val);
    }
    return ret_val;
}

_int32 hsc_handle_hsc_data_pipe_retry_interval(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid)
{
    _int32      ret_val                 = SUCCESS;
    HSC_DATA_PIPE_INTERFACE*    p_HDPI  = NULL;
    if(errcode != MSG_TIMEOUT)
        return SUCCESS;

    if(NULL == msg_info)
        return INVALID_MEMORY;

    p_HDPI = (HSC_DATA_PIPE_INTERFACE*)msg_info->_user_data;
    LOG_DEBUG("on hsc_handle_hsc_data_pipe_retry_interval p_HDPI:0x%x....", p_HDPI);
    if(NULL == p_HDPI)
        return INVALID_MEMORY;

    hsc_cancel_pipe_timer(p_HDPI);
    ret_val = hsc_do_send_data(p_HDPI);
    p_HDPI->_retry_count--;
    LOG_DEBUG("on hsc_handle_hsc_data_pipe_retry_interval retry immediately:%d", ret_val);
    return SUCCESS;
}

_int32 hsc_start_pipe_timeout_timer(HSC_DATA_PIPE_INTERFACE* p_HDPI)
{
    _int32      ret_val     = SUCCESS;
    sd_assert(p_HDPI);
    ret_val = start_timer(hsc_handle_hsc_data_pipe_timeout, NOTICE_ONCE, p_HDPI->_time_out*1000, 0, (void*)p_HDPI, &p_HDPI->_timer_id);
    p_HDPI->_status = HDPS_RESVING;
    LOG_DEBUG("hsc_start_pipe_timeout_timer id:%d, ret:%d", p_HDPI->_timer_id, ret_val);
    return ret_val;
}

_int32 hsc_start_retry_interval_timer(HSC_DATA_PIPE_INTERFACE* p_HDPI)
{
    _int32      ret_val     = SUCCESS;
    LOG_DEBUG("hsc_start_retry_interval_timer HDPI:0x%x", p_HDPI);
    sd_assert(p_HDPI);
    ret_val = start_timer(hsc_handle_hsc_data_pipe_retry_interval, NOTICE_ONCE, p_HDPI->_retry_interval*1000, 0, (void*)p_HDPI, &p_HDPI->_timer_id);
    p_HDPI->_status = HDPS_WAITTING_FOR_RETRY;
    LOG_DEBUG("hsc_start_retry_interval_timer id:%d, ret:%d", p_HDPI->_timer_id, ret_val);
    return ret_val;
}

_int32 hsc_cancel_pipe_timer(HSC_DATA_PIPE_INTERFACE* p_HDPI)
{
    _int32      ret_val     = SUCCESS;
    LOG_DEBUG("hsc_cancel_pipe_timer HDPI:0x%x", p_HDPI);
    sd_assert(p_HDPI);
    LOG_DEBUG("hsc_cancel_pipe_timer id:%d", p_HDPI->_timer_id);
    if(p_HDPI->_timer_id)
    {
        ret_val = cancel_timer(p_HDPI->_timer_id);
        p_HDPI->_timer_id = 0;
    }
    return ret_val;
}

_int32 hsc_add_pipe_to_manager(HSC_DATA_PIPE_INTERFACE* p_HDPI)
{
    sd_assert(p_HDPI);
    LOG_DEBUG("on hsc_add_pipe_to_manager size:%d", list_size(&g_hsc_data_pipe_manager._lst_pipes));
    return list_push(&g_hsc_data_pipe_manager._lst_pipes, (void*)p_HDPI);
}

_int32 hsc_remove_pipe_from_manager(HSC_DATA_PIPE_INTERFACE* p_HDPI)
{
	_int32 			ret_val		= SUCCESS;
	LIST_ITERATOR iter = NULL, end_node = NULL;
	sd_assert(p_HDPI);
	iter 			= LIST_BEGIN(g_hsc_data_pipe_manager._lst_pipes);
	end_node		= LIST_END(g_hsc_data_pipe_manager._lst_pipes);
	while(iter != end_node)
	{
		if(p_HDPI == (HSC_DATA_PIPE_INTERFACE*)LIST_VALUE(iter))
		{
			LOG_DEBUG("on hsc_remove_pipe_from_manager find it");
			break;
		}
		iter = LIST_NEXT(iter);
	}
	if(iter == end_node)
	{
		LOG_DEBUG("on hsc_remove_pipe_from_manager not find, something must be wrong");
		return FALSE;
	}
	ret_val = list_erase(&g_hsc_data_pipe_manager._lst_pipes, iter);
	LOG_DEBUG("on hsc_remove_pipe_from_manager:%d", ret_val);
	return ret_val;
}

_int32 hsc_remove_pipe_from_manager_by_it(LIST_ITERATOR it)
{
	return list_erase(&g_hsc_data_pipe_manager._lst_pipes, it);
}

LIST_ITERATOR hsc_find_pipe_from_manager(HSC_DATA_PIPE_INTERFACE* p_HDPI)
{
    sd_assert(p_HDPI);
    LIST_ITERATOR it = LIST_BEGIN(g_hsc_data_pipe_manager._lst_pipes);
    LIST_ITERATOR end = LIST_END(g_hsc_data_pipe_manager._lst_pipes);
    while(it != end)
    {
        if(p_HDPI == LIST_VALUE(it))
        {
            LOG_DEBUG("on hsc_find_pipe_from_manager:%d finded!!!", p_HDPI);
            return it;
        }
        it = LIST_NEXT(it);
    }
    LOG_DEBUG("on hsc_find_pipe_from_manager:%d not find!!!", p_HDPI);
    return it;
}

LIST   hsc_get_hsc_pipe_manager()
{
    return g_hsc_data_pipe_manager._lst_pipes;
}

#endif /* ENABLE_HSC */
