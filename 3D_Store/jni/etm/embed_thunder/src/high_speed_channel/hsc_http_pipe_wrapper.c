/*******************************************************************
 *  世界末日就要来了
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
#include "utility/range.h"
#include "utility/settings.h"
#include "utility/define_const_num.h"
#include "connect_manager/data_pipe.h"
#include "connect_manager/pipe_interface.h"
#include "http_data_pipe/http_data_pipe.h"
#include "hsc_http_pipe_wrapper.h"
#include "connect_manager/pipe_function_table.h"
#include "data_manager/data_manager_interface.h"


#define HSC_DATA_BUFFRE_LEN (16*1024)
static void *g_hsc_http_pipe_function_table[MAX_FUNC_NUM];
static PIPE_INTERFACE           g_hsc_http_pipe_interface;
static BOOL                     g_hsc_http_pipe_inited = FALSE;

_int32  hsc_init_hsc_http_pipe_interface()
{
	_int32      ret_val = SUCCESS;
    if(g_hsc_http_pipe_inited == TRUE) return ret_val;
    LOG_DEBUG("on hsc_init_hsc_http_pipe_interface start....");
	sd_memset(g_hsc_http_pipe_function_table, 0, MAX_FUNC_NUM);
	sd_memset(&g_hsc_http_pipe_interface, 0, sizeof(PIPE_INTERFACE));
	g_hsc_http_pipe_function_table[0] = (void*)hsc_http_pipe_change_range;
	g_hsc_http_pipe_function_table[1] = (void*)hsc_http_pipe_get_dispatcher_range;
	g_hsc_http_pipe_function_table[2] = (void*)hsc_http_pipe_set_dispatcher_range;
	g_hsc_http_pipe_function_table[3] = (void*)hsc_http_pipe_get_file_size;
	g_hsc_http_pipe_function_table[4] = (void*)hsc_http_pipe_set_file_size;
	g_hsc_http_pipe_function_table[5] = (void*)hsc_http_pipe_get_data_buffer;
	g_hsc_http_pipe_function_table[6] = (void*)hsc_http_pipe_free_data_buffer;
	g_hsc_http_pipe_function_table[7] = (void*)hsc_http_pipe_put_data_buffer;
	g_hsc_http_pipe_function_table[8] = (void*)hsc_http_pipe_read_data;
	g_hsc_http_pipe_function_table[9] = (void*)hsc_http_pipe_notify_dispatcher_data_finish;
	g_hsc_http_pipe_function_table[10] = (void*)hsc_http_pipe_get_file_type;
	g_hsc_http_pipe_function_table[11] = NULL;

	g_hsc_http_pipe_interface._file_index = 0;
	g_hsc_http_pipe_interface._range_switch_handler = NULL;
	g_hsc_http_pipe_interface._func_table_ptr = g_hsc_http_pipe_function_table;

    g_hsc_http_pipe_inited            = TRUE;
    LOG_DEBUG("on hsc_init_hsc_http_pipe_interface end....");
	return SUCCESS;
}

_int32  hsc_init_http_hdpi(HSC_DATA_PIPE_INTERFACE* p_HDPI, const char* host, const _u32 host_len, const _u32 port)
{
    _int32      ret_val             = SUCCESS;
    LOG_DEBUG("on hsc_init_http_hdpi HDPI:0x%x, host:%s, port:%d", p_HDPI, host, port);
    hsc_init_hsc_http_pipe_interface();
    sd_assert(p_HDPI && host && host_len && port);
    ret_val                         = hsc_init_hdpi(p_HDPI, host, host_len, port);
    p_HDPI->_type                   = HDPT_HTTP;
    p_HDPI->_p_send_func            = hsc_send_http_data;
    p_HDPI->_p_cancel_send_func     = hsc_cancel_http_sending;
    return ret_val;
}

_int32  hsc_uninit_http_hdpi(HSC_DATA_PIPE_INTERFACE* p_HDPI)
{
    LOG_DEBUG("on hsc_uninit_http_hdpi:0x%x", p_HDPI);
    sd_assert(p_HDPI);
    return hsc_uninit_hdpi(p_HDPI);
}

_int32 hsc_send_http_data(HSC_DATA_PIPE_INTERFACE* p_HDPI)
{
    _int32          ret_val             = SUCCESS;
    RESOURCE        *p_http_resource    = NULL;
    DATA_PIPE       *p_data_pipe        = NULL;
    RANGE           r;

    ret_val = http_resource_create(p_HDPI->_p_host, p_HDPI->_host_len, NULL, 0, TRUE, &p_http_resource);
    if(ret_val != SUCCESS)
        goto HANDLE_ERROR;

    ret_val = http_pipe_create(NULL, p_http_resource, &p_data_pipe);
    if(ret_val != SUCCESS)
        goto HANDLE_ERROR;

    LOG_DEBUG("hsc_send_http_data http_pipe_create %d", ret_val);

    p_data_pipe->_p_pipe_interface = &g_hsc_http_pipe_interface;
    p_data_pipe->_is_user_free = TRUE;

    ret_val = http_pipe_set_request(p_data_pipe, p_HDPI->_p_send_data, p_HDPI->_send_data_len);
    if(ret_val != SUCCESS)
    {
        LOG_DEBUG("EEEEE hsc_send_http_data http_pipe_set_request err:%d", ret_val);
        goto HANDLE_ERROR;
    }
    p_HDPI->_p_pipe = p_data_pipe;

    LOG_DEBUG("hsc_send_http_data http_data_pipe will open!!!!");
    ret_val = http_pipe_open(p_data_pipe);
    if(ret_val != SUCCESS)
    {
        goto HANDLE_ERROR;
    }

    LOG_DEBUG("hsc_send_http_data http_data_pipe opened:%d, pipe:0x%x", ret_val, p_data_pipe);
    r._index = 0;
    r._num = MAX_U32;
    ret_val = pi_pipe_change_range(p_data_pipe, &r, FALSE);
    if(ret_val != SUCCESS)
    {
        goto HANDLE_ERROR;
    }
    LOG_DEBUG("hsc_send_http_data change range ok....");
    return ret_val;

HANDLE_ERROR:
	if(p_data_pipe)
	{
		if(p_data_pipe->_p_resource)
		{       
			ret_val = http_resource_destroy(&(p_data_pipe->_p_resource));
		}
		http_pipe_close(p_data_pipe);
		p_data_pipe = NULL;
	}
    return ret_val;
}

_int32  hsc_cancel_http_sending(HSC_DATA_PIPE_INTERFACE* p_HDPI)
{
    _int32      ret_val     = FALSE;
    DATA_PIPE*  p_data_pipe = NULL;
    sd_assert(p_HDPI);

    p_data_pipe = (DATA_PIPE*)(p_HDPI->_p_pipe);
    if(p_data_pipe)
    {
        if(p_data_pipe->_p_resource)
        {
            ret_val = http_resource_destroy(&(p_data_pipe->_p_resource));
        }
        ret_val = http_pipe_close(p_data_pipe);
        p_HDPI->_p_pipe = NULL;
    }
    return ret_val;
}

_int32 hsc_find_pipe_from_manager_by_pipe(DATA_PIPE* p_pipe, HSC_DATA_PIPE_INTERFACE** pp_HDPI)
{
    LIST _list = hsc_get_hsc_pipe_manager();
    LOG_DEBUG("hsc_find_pipe_from_manager_by_pipe start...");
    LIST_ITERATOR it = LIST_BEGIN(_list);
    LIST_ITERATOR end = LIST_END(_list);
    sd_assert(pp_HDPI && p_pipe);
    HSC_DATA_PIPE_INTERFACE* p_HDPI = NULL;
    while (it != end)
    {
        p_HDPI = LIST_VALUE(it);
        if(p_pipe == p_HDPI->_p_pipe)
        {
            LOG_DEBUG("on hsc_find_pipe_from_manager_by_pipe:%d finded!!!", p_pipe);
            *pp_HDPI = p_HDPI;
            return SUCCESS;
        }
        it = LIST_NEXT(it);
    }
    LOG_DEBUG("on hsc_find_pipe_from_manager:%d not find!!!", p_pipe);
    return FALSE;
}

_int32 hsc_http_pipe_change_range(struct tagDATA_PIPE *p_data_pipe, const struct _tag_range *range, BOOL cancel_flag)
{
    LOG_DEBUG("hsc_http_pipe_change_range....");
	return http_pipe_change_ranges(p_data_pipe, range);
}

_int32 hsc_http_pipe_get_dispatcher_range(struct tagDATA_PIPE *p_data_pipe, const struct _tag_range *p_dispatcher_range, void *p_pipe_range)
{
    LOG_DEBUG("hsc_http_pipe_get_dispatcher_range....");
	RANGE *range = (RANGE*)p_pipe_range;
	range->_index = p_dispatcher_range->_index;
	range->_num = p_dispatcher_range->_num;
	return SUCCESS;
}

_int32 hsc_http_pipe_set_dispatcher_range(struct tagDATA_PIPE *p_data_pipe, const void *p_pipe_range, struct _tag_range *p_dispatcher_range)
{
    LOG_DEBUG("hsc_http_pipe_set_dispatcher_range....");
	RANGE *range = (RANGE*)p_pipe_range;
	p_dispatcher_range->_index = range->_index;
	p_dispatcher_range->_num = range->_num;
	return SUCCESS;
}

_u64 hsc_http_pipe_get_file_size(struct tagDATA_PIPE *p_data_pipe)
{
	return SUCCESS;
}

_int32 hsc_http_pipe_set_file_size(struct tagDATA_PIPE *p_data_pipe, _u64 filesize)
{
    LOG_DEBUG("hsc_http_pipe_set_file_size....");
	return SUCCESS;
}

_int32 hsc_http_pipe_get_data_buffer(struct tagDATA_PIPE *p_data_pipe, char **pp_data_buffer, _u32 *p_data_len)
{
	_int32 ret_val = SUCCESS;
	ret_val = sd_malloc(HSC_DATA_BUFFRE_LEN, (void**)pp_data_buffer);
	*p_data_len = HSC_DATA_BUFFRE_LEN;
	LOG_DEBUG("hsc_http_pipe_get_data_buffer, data_pipe:0x%x, buffer:0x%x", p_data_pipe, *pp_data_buffer);
	return ret_val;
}

_int32 hsc_http_pipe_free_data_buffer(struct tagDATA_PIPE *p_data_pipe, char **pp_data_buffer, _u32 data_len)
{
	LOG_DEBUG("hsc_http_pipe_free_data_buffer, data_pipe:0x%x, buffer:0x%x", p_data_pipe, *pp_data_buffer);
	sd_free( *pp_data_buffer );
	*pp_data_buffer = NULL;
	return SUCCESS;
}

_int32 hsc_http_pipe_put_data_buffer(struct tagDATA_PIPE *p_data_pipe, const struct _tag_range *p_range, char **pp_data_buffer, _u32 data_len, _u32 buffer_len, struct tagRESOURCE *p_resource)
{
    _int32  ret_val                     = SUCCESS;
    HSC_DATA_PIPE_INTERFACE*    p_HDPI  = NULL;
    LOG_DEBUG("hsc_http_pipe_put_data_buffer buffer_len:%d, data_len:%d....", buffer_len, data_len);
    ret_val = hsc_find_pipe_from_manager_by_pipe(p_data_pipe, &p_HDPI);
    if(ret_val == SUCCESS)
    {
        hsc_cancel_pipe_timer(p_HDPI);
        ret_val = sd_malloc(buffer_len, (void**)&p_HDPI->_p_recv_data);
        if(ret_val != SUCCESS)
        {
            LOG_DEBUG("EEEE on hsc_http_pipe_put_data_buffer malloc recv_data error:%d", ret_val);
            p_HDPI->_status = HDPS_FAIL;
            hsc_fire_http_data_pipe_event(p_HDPI);
            return SUCCESS;
        }
        sd_memset(p_HDPI->_p_recv_data, 0, buffer_len);
        sd_memcpy(p_HDPI->_p_recv_data, *pp_data_buffer, data_len);
        p_HDPI->_recv_data_len = data_len;

        ret_val = dm_free_data_buffer(NULL, pp_data_buffer, buffer_len);
        LOG_DEBUG("on hsc_http_pipe_put_data_buffer dm_free_data_buffer:%d", ret_val);

        p_HDPI->_status = HDPS_OK;
        hsc_fire_http_data_pipe_event(p_HDPI);
    }
    else
    {
        LOG_DEBUG("WWWW on hsc_http_pipe_put_data_buffer can not find HDPI, some thing must be wrong!!!!");
    }
    return SUCCESS;
}

_int32 hsc_http_pipe_read_data(struct tagDATA_PIPE *p_data_pipe, void *p_user, struct _tag_range_data_buffer *p_data_buffer, notify_read_result p_call_back)
{
    LOG_DEBUG("hsc_http_pipe_read_data....");
	return SUCCESS;
}

_int32 hsc_http_pipe_notify_dispatcher_data_finish(struct tagDATA_PIPE *p_data_pipe)
{
    LOG_DEBUG("hsc_http_pipe_notify_dispatcher_data_finish....");
	return SUCCESS;
}

_int32 hsc_http_pipe_get_file_type(struct tagDATA_PIPE *p_data_pipe)
{
    LOG_DEBUG("hsc_http_pipe_get_file_type....");
	return SUCCESS;
}


#endif /* ENABLE_HSC */
