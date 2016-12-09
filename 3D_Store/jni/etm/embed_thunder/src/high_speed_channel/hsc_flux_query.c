#include "utility/define.h"

#ifdef ENABLE_HSC
#include "utility/logid.h"
#define LOGID LOGID_HIGH_SPEED_CHANNEL
#include "utility/logger.h"
#include "utility/peerid.h"
#include "utility/string.h"
#include "utility/utility.h"
#include "utility/settings.h"
#include "utility/mempool.h"

#include "hsc_flux_query.h"
#include "hsc_perm_query.h"
#include "res_query/res_query_interface.h"
#include "hsc_http_pipe_wrapper.h"

static      HSC_QUERY_HSC_FLUX_INFO_CALLBACK    g_call_back       = NULL;
static      HSC_FLUX_INFO                       g_flux_info       = {0} ;
static      HSC_DATA_PIPE_INTERFACE*            g_p_HDPI          = NULL;

_int32      hsc_flux_query_create_HDPI(HSC_DATA_PIPE_INTERFACE** pp_HDPI, const char* host, const _u32 host_len, const _u32 port);
_int32      hsc_on_flux_query_recv(HSC_DATA_PIPE_INTERFACE* p_HDPI);

_int32  hsc_query_hsc_flux_info(HSC_QUERY_HSC_FLUX_INFO_CALLBACK callback)
{
    _int32      ret_val = SUCCESS;
    LOG_DEBUG("hsc_query_hsc_flux_info callback:0x%x, status:%d", callback, g_flux_info._result);
    g_call_back         = callback;

    hsc_cancel_query_flux_info();
    return hsc_query_flux_info_impl();
}

_int32  hsc_query_flux_info_impl()
{
    char*   vip_host            = DEFAULT_LOCAL_URL; // "http://service.cdn.vip.xunlei.com";
    //char*   vip_host          = "http://10.10.199.26:8080";
    _int32  ret_val             = SUCCESS;
    void    *p_send_buffer      = NULL;
    _u32    send_buffer_size    = 0;

    char*   p_package_buffer    = NULL;
    _int32  package_buffer_len  = 0;
    HSC_BATCH_COMMIT_REQUEST    request  = {0};
#ifdef  _DEBUG
    char*   p_send_buffer_str   = NULL;
#endif  //_DEBUG


    LOG_DEBUG("hsc_query_flux_info_impl start....");
    ret_val = hsc_build_flux_query_cmd_struct(&request);
    LOG_DEBUG("hsc_query_flux_info_impl build_flux_query_cmd_struct, ret_val:%d", ret_val);
    if(ret_val != SUCCESS)
    {
        LOG_DEBUG("EEEEE hsc_build_batch_commit_cmd_struct err:%d", ret_val);
        goto HANDLE_ERROR;
    }
    ret_val = hsc_build_batch_commit_cmd(&request, &p_send_buffer, &send_buffer_size);
    LOG_DEBUG("hsc_batch_commit build_cmd, data:0x%x, len:%d", p_send_buffer, send_buffer_size);
    if(ret_val != SUCCESS)
    {
        LOG_DEBUG("EEEEE hsc_build_batch_commit_cmd err:%d", ret_val);
        goto HANDLE_ERROR;
    }

#ifdef  _DEBUG
    ret_val = sd_malloc(1023, (void**)&p_send_buffer_str);
    if(ret_val == SUCCESS)
    {
        sd_memset(p_send_buffer_str, 0, 1023);
        str2hex(p_send_buffer, send_buffer_size, p_send_buffer_str, 1023);
        LOG_DEBUG("hsc_build_batch_commit_cmd send_buffer:%s", p_send_buffer_str);
        sd_free(p_send_buffer_str);
        p_send_buffer_str = NULL;
    }
#endif  //_DEBUG

    ret_val = xl_aes_encrypt(p_send_buffer, &send_buffer_size);
    if(ret_val != SUCCESS)
    {
        LOG_DEBUG("EEEE hsc_build_batch_commit_cmd aes_encrypt error:%d", ret_val);
        goto HANDLE_ERROR;
    }
    LOG_DEBUG("hsc_build_batch_commit_cmd aes_encrypt ok: len:%d", send_buffer_size);

    ret_val = hsc_build_http_package(p_send_buffer, send_buffer_size, &p_package_buffer, &package_buffer_len);
    if(ret_val != SUCCESS)
    {
        LOG_DEBUG("EEEEE hsc_build_http_package err:%d", ret_val);
        goto HANDLE_ERROR;
    }
    LOG_DEBUG("hsc_build_http_package ok len:%d....", package_buffer_len);

    sd_free(p_send_buffer);
    p_send_buffer = NULL;
    send_buffer_size = 0;

    ret_val = hsc_flux_query_create_HDPI(&g_p_HDPI, vip_host, sd_strlen(vip_host), 80);
    if(ret_val != SUCCESS)
    {
        LOG_DEBUG("EEEE hsc_flux_query_create_HDPI error:%d", ret_val);
        goto HANDLE_ERROR;
    }
    ret_val = hsc_send_data(g_p_HDPI, p_package_buffer, package_buffer_len);
    if(ret_val == SUCCESS)
    {
        g_flux_info._result = HFIR_QUERYING;
        hsc_fire_query_flux_event();
    }

    sd_free(p_package_buffer);
    p_package_buffer = NULL;
    package_buffer_len = 0;

	LOG_DEBUG("hsc_batch_commit request sent");
    return ret_val;


HANDLE_ERROR:
    hsc_destroy_batch_commit_request(&request);
    if(p_send_buffer)
    {
        sd_free(p_send_buffer);
        p_send_buffer = NULL;
    }
    if(p_package_buffer)
    {
        sd_free(p_package_buffer);
        p_package_buffer = NULL;
    }
    hsc_cancel_query_flux_info();
    LOG_DEBUG("hsc_batch_commit error:%d....", ret_val);
    return ret_val;
}


_int32  hsc_cancel_query_flux_info()
{
    _int32      ret_val = SUCCESS;
    LOG_DEBUG("on hsc_cancel_query_flux_info query_status:%d, g_HDPI:0x%x", g_flux_info._result, g_p_HDPI);
    if(g_p_HDPI)
    {
         ret_val = hsc_cancel_sending(g_p_HDPI);
         ret_val = hsc_uninit_http_hdpi(g_p_HDPI);
         sd_free(g_p_HDPI);
         g_p_HDPI = NULL;
    }
    sd_memset(&g_flux_info, 0, sizeof(HSC_FLUX_INFO));
    return ret_val;
}


_int32 hsc_flux_query_create_HDPI(HSC_DATA_PIPE_INTERFACE** pp_HDPI, const char* host, const _u32 host_len, const _u32 port)
{
    _int32      ret_val     = SUCCESS;
    sd_assert(pp_HDPI);
    ret_val = sd_malloc(sizeof(HSC_DATA_PIPE_INTERFACE), (void**)pp_HDPI);
    if(ret_val !=SUCCESS)
    {
        LOG_DEBUG("EEEE hsc_flux_query_create_HDPI malloc pp_HDPI error:%d", ret_val);
        return ret_val;
    }
    ret_val = hsc_init_http_hdpi(*pp_HDPI, host, host_len, port);
    if(ret_val != SUCCESS)
    {
        LOG_DEBUG("EEEE hsc_batch_commit_init_HDPI init_http_hdpi error:%d", ret_val);
        return ret_val;
    }
    (*pp_HDPI)->_retry_count    = 2;
    (*pp_HDPI)->_time_out       = 30;
    (*pp_HDPI)->_retry_interval       = 0;
    (*pp_HDPI)->_p_callbacks[HDPS_OK        ] = (void*)hsc_on_flux_query_recv;
    (*pp_HDPI)->_p_callbacks[HDPS_TIMEOUT   ] = (void*)hsc_on_flux_query_recv;
    (*pp_HDPI)->_p_callbacks[HDPS_FAIL      ] = (void*)hsc_on_flux_query_recv;
    return ret_val;
}

_int32 hsc_on_flux_query_recv(HSC_DATA_PIPE_INTERFACE* p_HDPI)
{

	_int32      ret_val         = SUCCESS;
    _int32      recv_buffer_len = 0;
	LOG_DEBUG("hsc_on_flux_query_recv, HDPI:0x%x", p_HDPI);
	sd_assert(p_HDPI);
    recv_buffer_len = p_HDPI->_recv_data_len;

    LOG_DEBUG("hsc_on_flux_query_recv query_status;%d, HDPI_status:%d, recv_buffer_len:%d....", g_flux_info._result, p_HDPI->_status, recv_buffer_len);
    if(g_flux_info._result == HFIR_QUERYING)
    {
        if(p_HDPI->_status == HDPS_OK)
        {
            xl_aes_decrypt(p_HDPI->_p_recv_data, &recv_buffer_len);
            ret_val = hsc_handle_recv_data(p_HDPI->_p_recv_data, recv_buffer_len);
        }
        else if(p_HDPI->_status == HDPS_TIMEOUT || p_HDPI->_status == HDPS_FAIL)
        {
            g_flux_info._result = HFIR_TIMEOUT;
            hsc_fire_query_flux_event();
        }
    }
    else
    {
        sd_assert(0);
    }
    ret_val = hsc_cancel_query_flux_info();
    LOG_DEBUG("hsc_on_flux_query_recv end:%d", ret_val);
    return ret_val;
}


_int32 hsc_build_flux_query_cmd_struct(HSC_BATCH_COMMIT_REQUEST* request)
{
    _int32  ret_val                     = SUCCESS;
    _int32  i                           = 0;
    HSC_USER_INFO*  p_user_info         = NULL;

    sd_assert(request);
    if(request == NULL)
    {
        LOG_DEBUG("EEEEE hsc_build_flux_query_cmd_struct parame error");
        return FALSE;
    }
    sd_memset(request, 0, sizeof(HSC_BATCH_COMMIT_REQUEST));
    LOG_DEBUG("hsc_build_flux_query_cmd_struct request:0x%x", request);
    ret_val = hsc_build_shub_header(&(request->_header), 0x0c);

#ifdef ENABLE_MEMBER
    hsc_get_user_info(&p_user_info);
    if(p_user_info->_user_id)
    {
        request->_user_id = p_user_info->_user_id;
        sd_memcpy(request->_p_login_key, p_user_info->_p_jump_key, p_user_info->_jump_key_length);
        request->_login_key_length = p_user_info->_jump_key_length;
        LOG_DEBUG("hsc_build_flux_query_cmd_struct userid:%lld, login key:%s, len:%d", request->_user_id, request->_p_login_key, request->_login_key_length);
    }
    else
#endif
    {   //has not login or login failed....
        LOG_DEBUG("WWWWWW hsc_build_flux_query_cmd_struct not login!!!!");
        request->_user_id = 0;
        sd_memset(request->_p_login_key, 0, URL_MAX_LEN);
        request->_login_key_length = 0;
    }

    g_flux_info._userid = request->_user_id;

    sd_memset(request->_p_peer_id, 0, PEER_ID_SIZE);
    get_peerid(request->_p_peer_id, PEER_ID_SIZE);
    request->_peer_id_length = PEER_ID_SIZE;

    request->_main_taskinfo_count = 0;
	request->_taskinfos_length 	= 0; //todo cacl length...

	request->_business_flag 		= hsc_get_business_flag(); //todo set flag...
    LOG_DEBUG("WWWWW hsc_build_flux_query_cmd_struct end longin_key_length:%d", request->_login_key_length);
	return ret_val;
}

_int32  hsc_handle_recv_data(const char* p_data, const _int32 data_size)
{
    _int32      ret_val             = SUCCESS;
    HSC_BATCH_COMMIT_RESPONSE* response = NULL;
    HSC_USER_INFO*  p_user_info         = NULL;
    LOG_DEBUG("on hsc_handle_recv_data....");

	sd_assert(p_data && data_size);
	if(p_data == NULL || data_size == 0)
	{
		LOG_DEBUG("on hsc_handle_recv_data param error");
		return INVALID_ARGUMENT;
	}
    LOG_DEBUG("hsc_handle_recv_data data:0x%s, size:%d", p_data, data_size);

    ret_val = sd_malloc(sizeof(HSC_BATCH_COMMIT_RESPONSE), (void**)&response);
    if(ret_val != SUCCESS)
    {
        LOG_DEBUG("EEEEE hsc_handle_recv_data malloc response error:%d", ret_val);
        goto HANDLE_ERROR;
    }
    ret_val = hsc_parser_batch_commit_response(p_data, data_size, response);

    if(ret_val != SUCCESS)
    {
        LOG_DEBUG("EEEEE hsc_handle_recv_data parser response error:%d", ret_val);
        goto HANDLE_ERROR;
    }

    if(response->_result == 0)
    {
        hsc_get_user_info(&p_user_info);
        LOG_DEBUG("on hsc_handle_recv_data current userid:%llu, querying userid:%llu", \
                                        p_user_info->_user_id, g_flux_info._userid);
        if(g_flux_info._userid == p_user_info->_user_id)
        {
            g_flux_info._result         = HFIR_OK;
            g_flux_info._capacity       = response->_capacity;
            g_flux_info._remain         = response->_remain;
            hsc_fire_query_flux_event();

            hsc_destroy_batch_commit_response(response);
            sd_free(response);
            response = NULL;
            return ret_val;
        }
    }
HANDLE_ERROR:
    if(response)
    {
        hsc_destroy_batch_commit_response(response);
        sd_free(response);
        response = NULL;
    }
    g_flux_info._result         = HFIR_FAILED;
    g_flux_info._capacity       = 0;
    g_flux_info._remain         = 0;
    hsc_fire_query_flux_event();
    return ret_val;
}

_int32 hsc_fire_query_flux_event()
{
    _int32      ret_val = SUCCESS;
    LOG_DEBUG("on hsc_fire_query_flux_event callback:0x%x, status:%d", g_call_back, g_flux_info._result);
    if(g_call_back)
    {
        ret_val = g_call_back(&g_flux_info);
    }
    return ret_val;
}
#endif  //ENABLE_HSC
