#include "utility/define.h"

#if defined(ENABLE_HSC) && !defined(ENABLE_ETM_MEDIA_CENTER)
#include "utility/peerid.h"
#include "utility/string.h"
#include "utility/range.h"
#include "utility/utility.h"
#include "utility/settings.h"
#include "utility/logid.h"
#define LOGID LOGID_HIGH_SPEED_CHANNEL
#include "utility/logger.h"
#include "utility/mempool.h"
#include "utility/md5.h"
#include "utility/aes.h"
#include "utility/bytebuffer.h"

#include "hsc_perm_query.h"
#include "hsc_info.h"
#include "asyn_frame/notice.h"
#include "connect_manager/pipe_function_table.h"
#include "data_manager/data_manager_interface.h"
#include "reporter/reporter_interface.h"
#include "res_query/res_query_interface.h"

#include "task_manager/task_manager.h"

#include "hsc_http_pipe_wrapper.h"


#ifdef ENABLE_BT
#include "bt_download/bt_task/bt_task.h"
#include "bt_download/bt_task/bt_task_interface.h"
#include "torrent_parser/torrent_parser.h"
#endif

#define SHUB_ENCODE_PADDING_LEN	16
#define HSC_HTTP_HEADER_LEN     1024

#define G_E_OK                  0
#define G_E_NOT_ENOUGH_SPACE    101
#define G_E_IN_PROGRESS         102

#define HSC_BATCH_COMMIT_TIMEOUT    30              //s
#define HSC_MAX_BT_SUB_TASK         10

static  HSC_USER_INFO g_hsc_user_info = {0};

#define LOG_ITEM(key, val, type) LOG_DEBUG("%s:##type", key, val);
//jjxh added...
//
static HSC_PQ_TASK_MANAGER g_hsc_pq_task_manager;


#define HSC_DATA_BUFFRE_LEN (16*1024)

static void hsc_on_deletet_task_notice(TASK *task_ptr);
_int32 hsc_pq_init_module()
{
	_int32 ret_val = SUCCESS;
	list_init(&g_hsc_pq_task_manager._hsc_pq_task_list);
	*hsc_get_g_auto_enable_stat() = FALSE;
    hsc_init_hsc_http_pipe_interface();
//    tm_register_deletet_task_notice(hsc_on_deletet_task_notice);
	return SUCCESS;
}

_int32 hsc_on_recv(HSC_DATA_PIPE_INTERFACE* p_HDPI)
{

	_int32 ret_val = SUCCESS;
	HSC_PQ_TASK_INFO *p_task_info = NULL;
    _int32      recv_buffer_len = p_HDPI->_recv_data_len;
    _int32      pi              = 0;
    TASK*       _p_task         = NULL;
    LIST        lst_acc_files;
    _u32        acc_file_count  = 0;
	LOG_DEBUG("hsc_on_recv, HDPI:0x%x, recv_data_len:%d", p_HDPI, recv_buffer_len);
	sd_assert(p_HDPI);

    LOG_DEBUG("hsc_on_recv finding task_info....");
    ret_val = hsc_find_pq_task_by_data_pipe(p_HDPI, &p_task_info);
    if(ret_val == SUCCESS && p_task_info)
    {
        LOG_DEBUG("hsc_on_recv finding task_info ok, status:%d....", p_HDPI->_status);
        if(p_HDPI->_status == HDPS_OK)
        {
            xl_aes_decrypt(p_HDPI->_p_recv_data, &recv_buffer_len);
            ret_val = hsc_on_response(p_task_info, p_HDPI->_p_recv_data, recv_buffer_len);
        }
        else if(p_HDPI->_status == HDPS_TIMEOUT || p_HDPI->_status == HDPS_FAIL)
        {
            for(pi = 0; pi < p_task_info->task_size; pi++)
            {
                _p_task = (p_task_info->_p_task + pi)->_p_task;
                if(_p_task->_hsc_info._stat == HSC_ENTERING)
                {
                    LOG_DEBUG("EEEE hsc_on_recv, index:%d", pi);
                    _p_task->_hsc_info._stat = HSC_FAILED;
                    _p_task->_hsc_info._errcode = HSC_BCR_TIMEOUT;
                }
                else if(_p_task->_hsc_info._stat == HSC_SUCCESS && _p_task->_task_type == BT_TASK_TYPE)
                {
                    list_init(&lst_acc_files);
                    hsc_get_bt_hsc_list(_p_task, &lst_acc_files);
                    acc_file_count = list_size(&lst_acc_files);
                    LOG_DEBUG("WWWW hsc_on_recv, index:%d, sub task silent commit timeout acc file count:%d....", pi, acc_file_count);
                    if(acc_file_count == 0)
                    {
                        _p_task->_hsc_info._stat    = HSC_BT_SUBTASK_FAILED;
                        _p_task->_hsc_info._errcode = HSC_BCR_TIMEOUT;
                    }
                    else
                    {}
                    list_clear(&lst_acc_files);
                }
            }
        }

        ret_val = hsc_remove_task_from_task_manager(p_task_info);
        LOG_DEBUG("on hsc_on_recv hsc_remove_task_from_task_manager:%d", ret_val);
        ret_val = hsc_destroy_HSC_PQ_TASK_INFO(p_task_info);
        LOG_DEBUG("on hsc_on_recv hsc_destroy_batch_commit_request:%d", ret_val);
        ret_val = sd_free(p_task_info);
        LOG_DEBUG("on hsc_on_recv free p_task_info:0x%x:%d",p_task_info, ret_val);
    }
    else //查询到结果之前删除任务，会走到这里
    {
		hsc_uninit_http_hdpi(p_HDPI);
        sd_free(p_HDPI);
        LOG_DEBUG("on hsc_on_recv can't find task_info, something wrong!!!!!!!");
    }
    return ret_val;
}

_int32 hsc_pq_query_permission(TASK *p_task, const _u8 *gcid, const _u8 *cid, _u64 file_size)
{
    return FALSE;
}

BOOL hsc_pq_is_hsc_available(TASK *p_task)
{
	if(p_task->_hsc_info._stat == HSC_SUCCESS)
	{
		return p_task->_hsc_info._can_use;
	}
	else
		return FALSE;
}

//jjxh added....
_int32 hsc_batch_commit(HSC_BATCH_COMMIT_PARAM* p_param, _int32 size)
{
    
    char* vip_host = DEFAULT_LOCAL_URL;	//"http://service.cdn.vip.xunlei.com";
    //char* vip_host = "http://61.188.190.47";
    _int32 ret_val = SUCCESS;
    HSC_DATA_PIPE_INTERFACE* p_HDPI = NULL;
    void *p_send_buffer = NULL;
    _u32 send_buffer_size = 0;
    _int32 i = 0;
    char* p_package_buffer = NULL;
    _int32 package_buffer_len = 0;
   // RANGE r;
    HSC_BATCH_COMMIT_REQUEST request  = {0};
	HSC_PQ_TASK_INFO *p_task_info = NULL;
	
#ifdef  _DEBUG
    char*   p_send_buffer_str   = NULL;
#endif  //_DEBUG

    LOG_DEBUG("hsc_batch_commit start....");
    ret_val = hsc_build_batch_commit_cmd_struct(p_param, size, &request);
    LOG_DEBUG("hsc_batch_commit build_batch_commit_cmd_struct, ret_val:%d", ret_val);
    if (ret_val != SUCCESS)
    {
        LOG_DEBUG("EEEEE hsc_build_batch_commit_cmd_struct err:%d", ret_val);
        goto HANDLE_ERROR;
    }
    ret_val = hsc_build_batch_commit_cmd(&request, &p_send_buffer, &send_buffer_size);
    LOG_DEBUG("hsc_batch_commit build_cmd, data:0x%x, len:%d", p_send_buffer, send_buffer_size);
    if (ret_val != SUCCESS)
    {
        LOG_DEBUG("EEEEE hsc_build_batch_commit_cmd err:%d", ret_val);
        goto HANDLE_ERROR;
    }

#ifdef  _DEBUG
    _u32 send_buffer_str_size = 1023;
    ret_val = sd_malloc(send_buffer_str_size, (void**)&p_send_buffer_str);
    if (ret_val == SUCCESS)
    {
        sd_memset(p_send_buffer_str, 0, send_buffer_str_size);
        str2hex(p_send_buffer, send_buffer_size, p_send_buffer_str, send_buffer_str_size);
        LOG_DEBUG("hsc_build_batch_commit_cmd send_buffer:%s", p_send_buffer_str);
        sd_free(p_send_buffer_str);
        p_send_buffer_str = NULL;
    }
#endif  //_DEBUG

    ret_val = xl_aes_encrypt(p_send_buffer, &send_buffer_size);
    if (ret_val != SUCCESS)
    {
        LOG_DEBUG("EEEE hsc_build_batch_commit_cmd aes_encrypt error:%d", ret_val);
        goto HANDLE_ERROR;
    }
    LOG_DEBUG("hsc_build_batch_commit_cmd aes_encrypt ok: len:%d", send_buffer_size);

    ret_val = hsc_build_http_package(p_send_buffer, send_buffer_size, &p_package_buffer, &package_buffer_len);
    if (ret_val != SUCCESS)
    {
        LOG_DEBUG("EEEEE hsc_build_http_package err:%d", ret_val);
        goto HANDLE_ERROR;
    }
    LOG_DEBUG("hsc_build_http_package ok len:%d....", package_buffer_len);

    sd_free(p_send_buffer);
    p_send_buffer = NULL;
    send_buffer_size = 0;

    ret_val = hsc_batch_commit_create_HDPI(&p_HDPI, vip_host, sd_strlen(vip_host), 80);
    if (ret_val != SUCCESS)
    {
        LOG_DEBUG("EEEE hsc_batch_commit_create_HDPI error:%d", ret_val);
        goto HANDLE_ERROR;
    }
    ret_val = hsc_send_data(p_HDPI, p_package_buffer, package_buffer_len);

    sd_free(p_package_buffer);
    p_package_buffer = NULL;
    package_buffer_len = 0;

	ret_val = sd_malloc(sizeof(HSC_PQ_TASK_INFO), (void**)&p_task_info);
    if(ret_val != SUCCESS)
    {
        LOG_DEBUG("EEEEE hsc_batch_commit malloc HSC_PQ_TASK_INFO error:%d", ret_val);
        goto HANDLE_ERROR;
    }
	ret_val = sd_memset(p_task_info, 0, sizeof(HSC_PQ_TASK_INFO));
    p_task_info->_p_data_pipe   = p_HDPI;         //p_data_pipe;
    p_task_info->_p_task        = p_param;
    p_task_info->task_size      = size;

    ret_val = list_push(&g_hsc_pq_task_manager._hsc_pq_task_list, (void*)p_task_info);
    if(ret_val != SUCCESS)
        goto HANDLE_ERROR;

    for(i = 0; i < size; i++)
    {
        if(p_param[i]._p_task->_task_type == BT_TASK_TYPE && (p_param[i]._p_task->_hsc_info._stat == HSC_SUCCESS))
        {
            //p_param[i]._p_task->_hsc_info._stat = HSC_SUCCESS;
        }
        else
        {
            p_param[i]._p_task->_hsc_info._stat = HSC_ENTERING;
        }
        p_param[i]._p_task->_hsc_info._force_query_permission = FALSE;
    }

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
	if (p_param && size > 0) {
		for (i = 0; i < size; i++) {
			list_clear(&(p_param[i]._lst_file_index));
		}
		sd_free(p_param);
		p_param = NULL;

    }
	hsc_remove_task_from_task_manager(p_task_info);
	hsc_destroy_HSC_PQ_TASK_INFO(p_task_info);
	if(p_task_info)
    {
        sd_free(p_task_info);
        p_task_info = NULL;
    }
    LOG_DEBUG("hsc_batch_commit error:%d....", ret_val);
    return ret_val;
}

_int32 hsc_batch_commit_create_HDPI(HSC_DATA_PIPE_INTERFACE** pp_HDPI, const char* host, const _u32 host_len, const _u32 port)
{
    _int32 ret_val = SUCCESS;
    sd_assert(pp_HDPI);
    ret_val = sd_malloc(sizeof(HSC_DATA_PIPE_INTERFACE), (void**)pp_HDPI);
    if(ret_val !=SUCCESS)
    {
        LOG_DEBUG("EEEE hsc_batch_commit_create_HDPI malloc pp_HDPI error:%d", ret_val);
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
    (*pp_HDPI)->_p_callbacks[HDPS_OK        ] = (void*)hsc_on_recv;
    (*pp_HDPI)->_p_callbacks[HDPS_TIMEOUT   ] = (void*)hsc_on_recv;
    (*pp_HDPI)->_p_callbacks[HDPS_FAIL      ] = (void*)hsc_on_recv;
    return ret_val;
}

_int32 hsc_remove_task_from_task_manager(HSC_PQ_TASK_INFO* info)
{
	_int32 			ret_val		= SUCCESS;
	LIST_ITERATOR iter = NULL, end_node = NULL;
	sd_assert(info);
	if(info == NULL)
	{
		LOG_DEBUG("on hsc_remove_task_from_task_manager error");
		return INVALID_ARGUMENT;
	}
	iter 			= LIST_BEGIN(g_hsc_pq_task_manager._hsc_pq_task_list);
	end_node		= LIST_END(g_hsc_pq_task_manager._hsc_pq_task_list);
	while(iter != end_node)
	{
		if(info == (HSC_PQ_TASK_INFO*)LIST_VALUE(iter))
		{
			LOG_DEBUG("on hsc_remove_task_from_task_manager find it");
			break;
		}
		iter = LIST_NEXT(iter);
	}
	if(iter == end_node)
	{
		LOG_DEBUG("on hsc_remove_task_from_task_manager not find, something must be wrong");
		return HSC_PQ_PUT_DATA_TASK_NOT_FOUND;
	}
	ret_val = list_erase(&g_hsc_pq_task_manager._hsc_pq_task_list, iter);
	CHECK_VALUE(ret_val);

	LOG_DEBUG("on hsc_remove_task_from_task_manager, SUCCESS!!!");
	return ret_val;
}

_int32 hsc_find_pq_task_by_data_pipe(HTTP_DATA_PIPE* pipe, HSC_PQ_TASK_INFO** pp_info)
{
	_int32				ret_val 	= FALSE;
	LIST_ITERATOR 		iter 		= NULL;
	HSC_PQ_TASK_INFO*   p_task_info = NULL;
    LOG_DEBUG("on hsc_find_pq_task_by_data_pipe start...");
	sd_assert(pp_info && pipe);
	if(pp_info == NULL || pipe == NULL)
	{
		LOG_DEBUG("on hsc_find_pq_task_by_data_pipe param error");
		return INVALID_ARGUMENT;
	}
	*pp_info = NULL;
	for(iter = LIST_BEGIN(g_hsc_pq_task_manager._hsc_pq_task_list);
		iter != LIST_END(g_hsc_pq_task_manager._hsc_pq_task_list);
		iter = LIST_NEXT(iter)
		)
	{
		p_task_info = (HSC_PQ_TASK_INFO*)LIST_VALUE(iter);
		if(p_task_info->_p_data_pipe == &(pipe->_data_pipe))
		{
			LOG_DEBUG("on hsc_find_pq_task_by_data_pipe finded!!");
			*pp_info = p_task_info;
			ret_val = SUCCESS;
			return ret_val;
		}
	}
	LOG_DEBUG("on hsc_find_pq_task_by_data_pipe not find...");
	return ret_val;

}

_int32 hsc_destroy_HSC_PQ_TASK_INFO(HSC_PQ_TASK_INFO* info)
{
	_int32 ret_val 			= SUCCESS;
    _int32 i                = 0;
	sd_assert(info);
    LOG_DEBUG("on hsc_destroy_HSC_PQ_TASK_INFO start, info:0x%x", info);
	if(info == NULL)
	{
		LOG_DEBUG("on hsc_destroy_HSC_PQ_TASK_INFO error");
		return INVALID_ARGUMENT;
	}
	if(info->_p_data_pipe)
	{
        hsc_uninit_http_hdpi(info->_p_data_pipe);
        sd_free(info->_p_data_pipe);
		info->_p_data_pipe = NULL;
	}
    if(info->_p_task && info->task_size > 0)
    {
        for(i = 0; i < info->task_size; i++)
        {
             list_clear(&info->_p_task[i]._lst_file_index);
        }
        sd_free(info->_p_task);
        info->_p_task = NULL;
        info->task_size = 0;
    }
    LOG_DEBUG("on hsc_destroy_HSC_PQ_TASK_INFO end");
	return SUCCESS;
}

_int32 hsc_build_shub_header(HSC_SHUB_HEADER* header, _u16 cmd_type)
{
    static _u32 _seq    = 0;        
    _int32 ret_val      = SUCCESS;
    if(header)
    {
        header->_protocol_version   = 3;
        header->_seq                = _seq;
        header->_cmd_length         = 0;
        header->_client_ver         = hsc_get_client_ver();
        header->_compress_flag      = 0;
        header->_cmd_type           = cmd_type; //0x0d;
        _seq++;
        return ret_val;
    }
    return INVALID_MEMORY;
}


_int32 hsc_build_batch_commit_cmd_struct(HSC_BATCH_COMMIT_PARAM* p_params, _int32 param_size, HSC_BATCH_COMMIT_REQUEST* request)
{
    _int32  ret_val                     = SUCCESS;
    _int32  i                           = 0;

    sd_assert(request && p_params && param_size);
    if(p_params == NULL || request == NULL)
    {
        LOG_DEBUG("EEEEE hsc_build_batch_commit_cmd_struct parame error");
        return FALSE;
    }
    sd_memset(request, 0, sizeof(HSC_BATCH_COMMIT_REQUEST));
    LOG_DEBUG("hsc_build_batch_commit_cmd_struct param:0x%x", p_params);
    ret_val = hsc_build_shub_header(&(request->_header), 0x0d);

    if(g_hsc_user_info._user_id)
    {
#ifdef ENABLE_MEMBER
        request->_user_id = g_hsc_user_info._user_id;
        sd_memcpy(request->_p_login_key, g_hsc_user_info._p_jump_key, g_hsc_user_info._jump_key_length);
        request->_login_key_length = g_hsc_user_info._jump_key_length;
        LOG_DEBUG("hsc_build_batch_commit_cmd_struct userid:%lld, login key:%s, len:%d", request->_user_id, request->_p_login_key, request->_login_key_length);
#endif
    }
    else
    {   //has not login or login failed....
        LOG_DEBUG("WWWWWW hsc_build_batch_commit_cmd_struct not login!!!!");
        request->_user_id = 0;
        sd_memset(request->_p_login_key, 0, URL_MAX_LEN);
        request->_login_key_length = 0;
    }

    sd_memset(request->_p_peer_id, 0, PEER_ID_SIZE);
    get_peerid(request->_p_peer_id, PEER_ID_SIZE);
    request->_peer_id_length = PEER_ID_SIZE;

    ret_val = sd_malloc(param_size * sizeof(HSC_SHUB_MAIN_TASKINFO), (void**)&request->_p_taskinfos);
    if(ret_val != SUCCESS)
    {
        LOG_DEBUG("EEEEE hsc_build_batch_commit_cmd_struct malloc shub_main_taskinfo err:%d", ret_val);
        hsc_destroy_batch_commit_request(request);
        return INVALID_MEMORY;
    }
    request->_main_taskinfo_count = param_size;
	request->_taskinfos_length 	= 0; //todo cacl length...
    for(i = 0; i < param_size; i++)
    {
        //request->_p_taskinfos[i]._p_url = NULL;
        request->_p_taskinfos[i]._main_taskinfo_length = request->_taskinfos_length;
        request->_p_taskinfos[i]._url_length = 0;
		request->_taskinfos_length += (request->_p_taskinfos[i]._url_length + 4);

        request->_p_taskinfos[i]._task_name_length = URL_MAX_LEN;
        hsc_get_task_name(p_params + i, request->_p_taskinfos[i]._p_task_name, &(request->_p_taskinfos[i]._task_name_length));
		request->_taskinfos_length += (request->_p_taskinfos[i]._task_name_length + 4);

        request->_p_taskinfos[i]._res_type = hsc_get_batch_commit_task_res_type(p_params[i]._p_task);
		request->_taskinfos_length += (1);

		request->_p_taskinfos[i]._res_id_length = 0;
        hsc_get_task_res_id(p_params[i]._p_task, request->_p_taskinfos[i]._p_res_id, &request->_p_taskinfos[i]._res_id_length);
		request->_taskinfos_length += (request->_p_taskinfos[i]._res_id_length + 4);

		request->_p_taskinfos[i]._main_server_task_id = 0;
		request->_taskinfos_length += (8);
		
		request->_p_taskinfos[i]._task_size = p_params[i]._file_size;
		request->_taskinfos_length += (8);
#ifdef      ENABLE_BT
		if(p_params[i]._p_task->_task_type == BT_TASK_TYPE)
		{
			ret_val = hsc_fill_bt_task_attribute(&request->_p_taskinfos[i]._p_sub_task_attribute, &request->_p_taskinfos[i]._sub_task_attribute_length, &request->_p_taskinfos[i]._sub_task_attribute_count,  p_params + i, &request->_p_taskinfos[i]._task_size);
		}
		else
#endif      //ENABLE_BT
		{
			ret_val = hsc_fill_p2sp_task_attribute(&request->_p_taskinfos[i]._p_sub_task_attribute, &request->_p_taskinfos[i]._sub_task_attribute_length, &request->_p_taskinfos[i]._sub_task_attribute_count, p_params + i);
		}
		if(ret_val != SUCCESS)
		{
			LOG_DEBUG("EEEEE hsc_build_batch_commit_cmd_struct fill p2sp task attr err:%d", ret_val);
			hsc_destroy_batch_commit_request(request);
			return INVALID_MEMORY;
		}

        LOG_DEBUG("hsc_build_batch_commit_cmd_struct sub_task_attribute count:%d, length:%d", request->_p_taskinfos[i]._sub_task_attribute_count, request->_p_taskinfos[i]._sub_task_attribute_length);
		request->_taskinfos_length += (request->_p_taskinfos[i]._sub_task_attribute_length + 4);
		request->_p_taskinfos[i]._need_query_manager = 1;
		request->_taskinfos_length += (1);
        request->_p_taskinfos[i]._main_taskinfo_length = request->_taskinfos_length - request->_p_taskinfos[i]._main_taskinfo_length;
        request->_taskinfos_length += (4);

    }

	request->_business_flag 		= hsc_get_business_flag(); //todo set flag...
    LOG_DEBUG("WWWWW hsc_build_batch_commit_cmd_struct end longin_key_length:%d", request->_login_key_length);
	return ret_val;
}


_int32 hsc_fill_p2sp_task_attribute(HSC_SHUB_SUB_TASK_ATTRIBUTE** pp_param, _int32* len, _int32* count, HSC_BATCH_COMMIT_PARAM* p_info)
{
	_int32 ret_val      = SUCCESS;
    char    cid_str[CID_SIZE*2 + 1]     = {0};
    char    gcid_str[CID_SIZE*2 + 1]    = {0};
	if(!(pp_param&&len&&p_info))
		return INVALID_MEMORY;
	sd_assert(p_info->_p_task->_task_type != BT_TASK_TYPE);

	ret_val = sd_malloc(sizeof(HSC_SHUB_SUB_TASK_ATTRIBUTE), (void**)pp_param);
	if(ret_val != SUCCESS)
	{
		LOG_DEBUG("EEEEE hsc_fill_p2sp_task_attribute malloc shub_sub_task_attr err:%d", ret_val);
		return INVALID_MEMORY;
	}
	sd_memset(*pp_param, 0, sizeof(HSC_SHUB_SUB_TASK_ATTRIBUTE));
	*len = 0;
    *count = 0;
	
    list_push(&p_info->_lst_file_index, (void*)0);

	str2hex( (char*)p_info->_p_gcid, CID_SIZE, gcid_str, CID_SIZE*2 + 1);
	str2hex( (char*)p_info->_p_cid, CID_SIZE, cid_str, CID_SIZE*2 + 1);

	sd_memcpy((*pp_param)->_p_gcid, gcid_str, CID_SIZE*2 + 1);
	(*pp_param)->_gcid_length = CID_SIZE*2;
	*len += ((*pp_param)->_gcid_length + 4);
    LOG_DEBUG("on hsc_fill_p2sp_task_attribute gcid:%s", (*pp_param)->_p_gcid);
	
	sd_memcpy((*pp_param)->_p_cid, cid_str, CID_SIZE*2 + 1);
	(*pp_param)->_cid_length 		= CID_SIZE*2;
	*len += ((*pp_param)->_cid_length + 4);
    LOG_DEBUG("on hsc_fill_p2sp_task_attribute cid:%s", (*pp_param)->_p_cid);

	(*pp_param)->_file_size 		= p_info->_file_size;
    LOG_DEBUG("on hsc_fill_p2sp_task_attribute file_size:%i64u", (*pp_param)->_file_size);
	*len += (8);
	(*pp_param)->_file_index 		= 0;
	*len += (4);
	(*pp_param)->_server_task_id 	= 0;
	*len += (8);
	//(*pp_param)->_p_file_name 		= NULL;
	(*pp_param)->_file_name_length 	= 0;
	*len += ((*pp_param)->_file_name_length + 4);
	(*pp_param)->_flag 				= 0;
	*len += (2);
    *count = 1;
    (*pp_param)->_sub_task_attr_length = *len;
    *len += (4);
	LOG_DEBUG("hsc_fill_p2sp_task_attribute len:%d, count:%d", *len, *count);
	return ret_val;
}

_int32 hsc_fill_emule_task_attribute(HSC_SHUB_SUB_TASK_ATTRIBUTE** pp_param, _int32* len, _int32* count, HSC_BATCH_COMMIT_PARAM* p_info)
{
	return SUCCESS;
}

_int32 hsc_fill_bt_task_attribute(HSC_SHUB_SUB_TASK_ATTRIBUTE** pp_param, _int32* len, _int32* count, HSC_BATCH_COMMIT_PARAM* p_info, _u64* download_file_size)
{
	//todu get sub task attribute from bt main task...
	_u32 	ret_val 					= SUCCESS;
	BT_TASK *p_bt_task 			        = NULL;
	_u32 	file_index_buffer_len 		= 0;
	_u32* 	file_index_buffer 			= NULL;
	_u32 	file_index 					= 0;
	_u8 	my_gcid[CID_SIZE 		] 	= {0};
	_u8 	my_cid[CID_SIZE 		] 	= {0};
    char    cid_str[CID_SIZE*2 + 1 	]   = {0};
    char    gcid_str[CID_SIZE*2 + 1 ]   = {0};
	BT_FILE_INFO_POOL bt_info_pool;
	ET_BT_FILE_INFO   bt_file_info;
    _int32  i                           = 0;
    _int32  org_file_index_count        = 0;
    _int32  real_sub_file_attr_index    = 0;
    *len                                = 0;
    *count                              = 0;
	
	LOG_DEBUG("hsc_fill_bt_task_attribute:_task_id=%u", p_info->_p_task->_task_id);
	
	sd_assert(p_info->_p_task->_task_type == BT_TASK_TYPE);
	p_bt_task = (BT_TASK*)p_info->_p_task;
	
    *download_file_size = 0;
	if(p_bt_task->_is_magnet_task && p_bt_task->_magnet_task_ptr)
	{
		LOG_DEBUG("EEEEE hsc_fill_bt_task_attribute task type is not bt");
		return FALSE;
	}

	ret_val = bt_get_download_file_index(p_info->_p_task, &file_index_buffer_len, file_index_buffer);
	if(ret_val == TM_ERR_BUFFER_NOT_ENOUGH)
	{
		ret_val = sd_malloc(file_index_buffer_len*sizeof(_u32), (void**)&file_index_buffer);
		if(ret_val != SUCCESS)
		{
			LOG_DEBUG("EEEEE hsc_fill_bt_task_attribute malloc downloading file index buffer err:%d", ret_val);
			return INVALID_MEMORY;
		}
	}
	ret_val = bt_get_download_file_index(p_info->_p_task, &file_index_buffer_len, file_index_buffer);
	if(ret_val != SUCCESS)
	{
		LOG_DEBUG("EEEEE hsc_fill_bt_task_attribute bt_get_download_file_index err:%d", ret_val);
		goto HANDLE_ERROR;
	}

	bt_get_file_info_pool(p_info->_p_task, &bt_info_pool);
	if(!bt_info_pool._file_info_for_user)
	{
		LOG_DEBUG("EEEEE hsc_fill_bt_task_attribute bt_get_file_info_pool err:%d", ret_val);
		goto HANDLE_ERROR;
	}

    ret_val = sd_malloc(sizeof(HSC_SHUB_SUB_TASK_ATTRIBUTE) * file_index_buffer_len, (void**)pp_param);
    if(ret_val != SUCCESS)
    {
		LOG_DEBUG("EEEEE hsc_fill_bt_task_attribute malloc shub_sub_task_attr err:%d", ret_val);
        goto HANDLE_ERROR;
    }
    sd_memset((*pp_param), 0, sizeof(HSC_SHUB_SUB_TASK_ATTRIBUTE) * file_index_buffer_len);

    org_file_index_count = list_size(&(p_info->_lst_file_index));
    LOG_DEBUG("hsc_fill_bt_task_attribute downloading file count:%d, org_file_index_count:%d, sub_attr_table:0x%x", file_index_buffer_len, org_file_index_count, *pp_param);
	for(i = 0; i < file_index_buffer_len; i++)
	{
		file_index = file_index_buffer[i];
		ret_val = bt_get_file_info(&bt_info_pool, file_index, &bt_file_info);
		if(ret_val != SUCCESS)
		{
			LOG_DEBUG("EEEEE hsc_fill_bt_task_attribute bt_get_file_info err:%d", ret_val);
			goto HANDLE_ERROR;
		}
        LOG_DEBUG("on hsc_fill_bt_task_attribute file_index:%d, file_status:%d", file_index, bt_file_info._file_status);
        *download_file_size += bt_file_info._file_size;
        if(bt_file_info._file_status != BT_FILE_DOWNLOADING) continue;
        if(cm_check_high_speed_channel_flag(&p_info->_p_task->_connect_manager, file_index) == TRUE) continue;
        if(org_file_index_count && (hsc_find_value_in_list(&p_info->_lst_file_index, (void*)file_index) == LIST_END(p_info->_lst_file_index))) continue;
        (*pp_param)[real_sub_file_attr_index]._gcid_length     = 0;
        (*pp_param)[real_sub_file_attr_index]._cid_length      = 0;
		if(TRUE == bdm_get_cid(&(p_bt_task->_data_manager), file_index, my_cid))
		{
			str2hex((char*)my_cid, CID_SIZE, cid_str, CID_SIZE*2);
            (*pp_param)[real_sub_file_attr_index]._cid_length      = CID_SIZE*2;
		}

		if(TRUE == bdm_get_shub_gcid(&(p_bt_task->_data_manager), file_index, my_gcid))
		{
			str2hex((char*)my_gcid, CID_SIZE, gcid_str, CID_SIZE*2);
            (*pp_param)[real_sub_file_attr_index]._gcid_length     = CID_SIZE*2;
		}
		else if(TRUE == bdm_get_calc_gcid(&(p_bt_task->_data_manager), file_index, my_gcid))
		{
			str2hex((char*)my_gcid, CID_SIZE, gcid_str, CID_SIZE*2);
            (*pp_param)[real_sub_file_attr_index]._gcid_length     = CID_SIZE*2;
		}
		
		//todo: data filling...

        if(!org_file_index_count)
        {
            LOG_DEBUG("on hsc_fill_bt_task_attribute commit sub file:%d", file_index);
            list_push(&p_info->_lst_file_index, (void*)file_index);
        }
        else
        {
            LOG_DEBUG("on hsc_fill_bt_task_attribute new commit sub file:%d", file_index);
        }
        (*pp_param)[real_sub_file_attr_index]._sub_task_attr_length = *len;

        sd_memcpy((*pp_param)[real_sub_file_attr_index]._p_gcid, gcid_str, CID_SIZE*2 + 1);
        *len += ((*pp_param)[real_sub_file_attr_index]._gcid_length + 4);
        LOG_DEBUG("on hsc_fill_bt_task_attribute sub task attr index:%d, gcid:%s, len:%d", i, gcid_str, (*pp_param)[real_sub_file_attr_index]._gcid_length);
        sd_memcpy((*pp_param)[real_sub_file_attr_index]._p_cid, cid_str,   CID_SIZE*2 + 1);
        *len += ((*pp_param)[real_sub_file_attr_index]._cid_length + 4);
        LOG_DEBUG("on hsc_fill_bt_task_attribute sub task attr index:%d, cid:%s, len:%d", i, cid_str, (*pp_param)[real_sub_file_attr_index]._cid_length);

        (*pp_param)[real_sub_file_attr_index]._file_size       = bt_file_info._file_size;
        *len += (8);
        (*pp_param)[real_sub_file_attr_index]._file_index      = file_index;
        *len += (4);
        (*pp_param)[real_sub_file_attr_index]._server_task_id  = 0;
        *len += (8);
        (*pp_param)[real_sub_file_attr_index]._file_name_length= 0;
        //(*pp_param)[i]._p_file_name     = NULL;
        *len += ((*pp_param)[real_sub_file_attr_index]._file_name_length + 4);
        (*pp_param)[real_sub_file_attr_index]._flag            = 0;
        *len += (2);
        (*pp_param)[real_sub_file_attr_index]._sub_task_attr_length = (*len) - (*pp_param)[real_sub_file_attr_index]._sub_task_attr_length;
        *len += (4);
        LOG_DEBUG("hsc_fill_bt_task_attribute index:%d, length:%d", i, (*pp_param)[real_sub_file_attr_index]._sub_task_attr_length);

        *count += 1;
        real_sub_file_attr_index++;
        //if(*count > HSC_MAX_BT_SUB_TASK)
        //{
        //    break;
        //}
	}
	if (file_index_buffer) {
		sd_free(file_index_buffer);
		file_index_buffer = NULL;
	}

	LOG_DEBUG("hsc_fill_bt_task_attribute len:%d, count:%d", *len, *count);
    return ret_val;

HANDLE_ERROR:
    if(file_index_buffer)
    {
        sd_free(file_index_buffer);
        file_index_buffer = NULL;
    }
    if(*pp_param)
    {
        sd_free(*pp_param);
        *pp_param = NULL;
    }
    return FALSE;
}

enum ECOMMITRESTYPE hsc_get_batch_commit_task_res_type(TASK* p_task)
{
    _u8 ret_val = ECRT_OTHERS;
    if(p_task)
    {
        switch(p_task->_task_type)
        {
            case P2SP_TASK_TYPE:        //todo 这里不完整，需要继续区分 http ftp...
                ret_val = ECRT_HTTP;
                break;
            case BT_TASK_TYPE:
                ret_val = ECRT_BT_RES;
                break;
            case EMULE_TASK_TYPE:
                ret_val = ECRT_EMULE;
                break;
        }
    }
    return ret_val;
}

LIST_ITERATOR hsc_find_value_in_list(LIST* list, const void* value)
{
    sd_assert(list);
    LIST_ITERATOR it = LIST_BEGIN(*list);
    LIST_ITERATOR end = LIST_END(*list);
    while(it != end)
    {
        if(value == LIST_VALUE(it))
        {
            LOG_DEBUG("on hsc_find_value_in_list value:%d finded!!!", value);
            return it;
        }
        it = LIST_NEXT(it);
    }
    LOG_DEBUG("on hsc_find_value_in_list value:%d not find!!!", value);
    return it;
}

_int32 hsc_destroy_batch_commit_request(HSC_BATCH_COMMIT_REQUEST* request)
{
    _int32      _main_task_info_count   = 0;
    _int32      i                       = 0;
    if(request)
    {
        _main_task_info_count = request->_main_taskinfo_count;
        for(i = 0; i < _main_task_info_count; i++)
        {
            if(request->_p_taskinfos[i]._p_sub_task_attribute)
            {
                LOG_DEBUG("hsc_destroy_batch_commit_request free sub task attr index:%d", i);
                sd_free(request->_p_taskinfos[i]._p_sub_task_attribute);
                request->_p_taskinfos[i]._p_sub_task_attribute = NULL;
            }
        }
        if(_main_task_info_count)
        {
            LOG_DEBUG("hsc_destroy_batch_commit_request free main task infos; count:%d", _main_task_info_count);
            sd_free(request->_p_taskinfos);
            request->_p_taskinfos = NULL;
            request->_main_taskinfo_count = 0;
        }
    }
    return SUCCESS;
}

_int32 hsc_parser_batch_commit_response(const char* p_data, const _int32 data_size, HSC_BATCH_COMMIT_RESPONSE* p_response)
{
    _int32 ret_val = SUCCESS;
    _int32 i = 0;
    _int32 j = 0;
    HSC_SHUB_MAIN_TASK_FLUX_INFO* p_main_flux_info = NULL;
    HSC_SHUB_SUB_TASK_FLUX_INFO*  p_sub_flux_info  = NULL;
    LOG_DEBUG("data:%s, size:%d", p_data, data_size);
    sd_assert(p_data && data_size && p_response);
    char* tem_buf = p_data;
    _int32 tem_len = data_size;
    sd_memset(p_response, 0, sizeof(HSC_BATCH_COMMIT_RESPONSE));
    sd_get_int32_from_lt(&tem_buf, &tem_len, (_int32*)&p_response->_header._protocol_version);
    sd_get_int32_from_lt(&tem_buf, &tem_len, (_int32*)&p_response->_header._seq);
    sd_get_int32_from_lt(&tem_buf, &tem_len, (_int32*)&p_response->_header._cmd_length);
    sd_get_int32_from_lt(&tem_buf, &tem_len, (_int32*)&p_response->_header._client_ver);
    sd_get_int16_from_lt(&tem_buf, &tem_len, (_int16*)&p_response->_header._compress_flag);
    sd_get_int16_from_lt(&tem_buf, &tem_len, (_int16*)&p_response->_header._cmd_type);

    sd_get_int32_from_lt(&tem_buf, &tem_len, (_int32*)&p_response->_result);
    LOG_DEBUG("result:%d", p_response->_result);
    sd_get_int32_from_lt(&tem_buf, &tem_len, (_int32*)&p_response->_message_length);
    LOG_DEBUG("message_length:%d", p_response->_message_length);
    if(p_response->_message_length > 0) //for optimization
    {
        ret_val = sd_malloc(p_response->_message_length + 1, (void**)&p_response->_p_message);
        if(ret_val != SUCCESS)
        {
            LOG_DEBUG("EEEEE malloc message buffer err:%d", ret_val);
            goto HANDLE_ERROR;
        }
        sd_memset(p_response->_p_message, 0, p_response->_message_length + 1);
        sd_get_bytes(&tem_buf, &tem_len, p_response->_p_message, p_response->_message_length);
        LOG_DEBUG("message:%s; len:%d", p_response->_p_message, p_response->_message_length);
    }
    
    sd_get_int64_from_lt(&tem_buf, &tem_len, (_int64*)&p_response->_capacity);
    LOG_DEBUG("_capacity:%lld", p_response->_capacity);
    sd_get_int64_from_lt(&tem_buf, &tem_len, (_int64*)&p_response->_remain);
    LOG_DEBUG("_remain:%lld", p_response->_remain);
    sd_get_int64_from_lt(&tem_buf, &tem_len, (_int64*)&p_response->_needed);
    LOG_DEBUG("_needed:%lld", p_response->_needed);

    sd_get_int32_from_lt(&tem_buf, &tem_len, (_int32*)&p_response->_flux_infos_length);
    ret_val = sd_malloc(p_response->_flux_infos_length*sizeof(HSC_SHUB_MAIN_TASK_FLUX_INFO), (void**)&p_response->_p_flux_infos);
    if(ret_val != SUCCESS)
    {
        LOG_DEBUG("EEEEE malloc main task flux info buffer err:%d", ret_val);
        goto HANDLE_ERROR;
    }
    if(p_response->_flux_infos_length == 0)
    {
        ret_val = -1;
        goto HANDLE_ERROR;
    }
    sd_memset(p_response->_p_flux_infos, 0, p_response->_flux_infos_length*sizeof(HSC_SHUB_MAIN_TASK_FLUX_INFO));
    for(i = 0; i < p_response->_flux_infos_length; i++)
    {
        p_main_flux_info    = p_response->_p_flux_infos + i;
        sd_get_int32_from_lt(&tem_buf, &tem_len, (_int32*)&p_main_flux_info->_main_task_flux_length);
        LOG_DEBUG("main task flux info index:%d, length:%d, remain:%d", i, p_main_flux_info->_main_task_flux_length, tem_len);

        sd_get_int64_from_lt(&tem_buf, &tem_len, (_int64*)&p_main_flux_info->_needed);
        LOG_DEBUG("_main_flux_info _needed:%lld", p_main_flux_info->_needed);
        sd_get_int32_from_lt(&tem_buf, &tem_len, (_int32*)&p_main_flux_info->_desc_length);
        LOG_DEBUG("_main_flux_info _desc_length:%d", p_main_flux_info->_desc_length);
        if(p_main_flux_info->_desc_length > 0)
        {
            ret_val = sd_malloc(p_main_flux_info->_desc_length + 1, (void**)&p_main_flux_info->_p_desc);
            if(ret_val != SUCCESS)
            {
                LOG_DEBUG("EEEEE malloc main_task_flux desc buffer err:%d", ret_val);
                goto HANDLE_ERROR;
            }
            sd_memset(p_main_flux_info->_p_desc, 0, p_main_flux_info->_desc_length + 1);
            sd_get_bytes(&tem_buf, &tem_len, p_main_flux_info->_p_desc, p_main_flux_info->_desc_length);
        }

        sd_get_int32_from_lt(&tem_buf, &tem_len, (_int32*)&p_main_flux_info->_subtasks_length);
        ret_val = sd_malloc(p_main_flux_info->_subtasks_length*sizeof(HSC_SHUB_SUB_TASK_FLUX_INFO), (void**)&p_main_flux_info->_p_subtasks);
        if(ret_val != SUCCESS)
        {
            LOG_DEBUG("EEEEE malloc sub task flux info buffer error:%d", ret_val);
            goto HANDLE_ERROR;
        }
        sd_memset(p_main_flux_info->_p_subtasks, 0, p_main_flux_info->_subtasks_length*sizeof(HSC_SHUB_SUB_TASK_FLUX_INFO));

        for(j = 0; j < p_main_flux_info->_subtasks_length; j++)
        {
            p_sub_flux_info = p_main_flux_info->_p_subtasks + j;
            sd_get_int32_from_lt(&tem_buf, &tem_len, (_int32*)&p_sub_flux_info->_sub_task_flux_length);
            LOG_DEBUG("sub task flux index:%d, length:%d, remain:%d", j, p_sub_flux_info->_sub_task_flux_length, tem_len);
            sd_get_int64_from_lt(&tem_buf, &tem_len, (_int64*)&p_sub_flux_info->_needed);
            LOG_DEBUG("sub_flux_info _needed:%lld", p_sub_flux_info->_needed);
            sd_get_int8(&tem_buf, &tem_len, (_int8*)&p_sub_flux_info->_vip_cdn_ok);
            LOG_DEBUG("sub_flux_info _vip_cdn_ok:%d", p_sub_flux_info->_vip_cdn_ok);
            sd_get_int16_from_lt(&tem_buf, &tem_len, (_int16*)&p_sub_flux_info->_code);
            LOG_DEBUG("sub_flux_info _code:%d", p_sub_flux_info->_code);
            sd_get_int32_from_lt(&tem_buf, &tem_len, (_int32*)&p_sub_flux_info->_desc_length);
            LOG_DEBUG("sub_flux_info _desc_length:%d", p_sub_flux_info->_desc_length);
            if(p_sub_flux_info->_desc_length > 0)
            {
                ret_val = sd_malloc(p_sub_flux_info->_desc_length + 1, (void**)&p_sub_flux_info->_p_desc);
                if(ret_val != SUCCESS)
                {
                    LOG_DEBUG("EEEEE malloc sub flux info desc buffer error:%d", ret_val);
                    goto HANDLE_ERROR;
                }
                sd_memset(p_sub_flux_info->_p_desc, 0, p_sub_flux_info->_desc_length + 1);
                sd_get_bytes(&tem_buf, &tem_len, p_sub_flux_info->_p_desc, p_sub_flux_info->_desc_length);
            }
        }
    }
    ret_val = sd_get_int16_from_lt(&tem_buf, &tem_len, (_int16*)&p_response->_free);
    LOG_DEBUG("_free:%d", p_response->_free);
    LOG_DEBUG("at last len:%d", tem_len);
    return ret_val;

HANDLE_ERROR:
    //todo release some resource....
    hsc_destroy_batch_commit_response(p_response);
    return ret_val;
}


_int32 hsc_destroy_batch_commit_response(HSC_BATCH_COMMIT_RESPONSE* p_response)
{
    _int32              i,j;
    HSC_SHUB_MAIN_TASK_FLUX_INFO* p_main_flux_info = NULL;
    HSC_SHUB_SUB_TASK_FLUX_INFO*  p_sub_flux_info  = NULL;

    LOG_DEBUG("on hsc_destroy_batch_commit_response");
    sd_assert(p_response);
    if(p_response == NULL)
        return INVALID_MEMORY;
    if(p_response->_p_message && p_response->_message_length)
    {
        sd_free(p_response->_p_message);
        p_response->_p_message = NULL;
        p_response->_message_length = 0;
    }
    for(i = 0; i < p_response->_flux_infos_length; i++)
    {
        p_main_flux_info = p_response->_p_flux_infos + i;
        if(p_main_flux_info->_desc_length && p_main_flux_info->_p_desc)
        {
            sd_free(p_main_flux_info->_p_desc);
            p_main_flux_info->_p_desc = NULL;
            p_main_flux_info->_desc_length = 0;
        }
        for(j = 0; j < p_main_flux_info->_subtasks_length; j++)
        {
            p_sub_flux_info = p_main_flux_info->_p_subtasks + j;
            if(p_sub_flux_info->_desc_length && p_sub_flux_info->_p_desc)
            {
                sd_free(p_sub_flux_info->_p_desc);
                p_sub_flux_info->_p_desc = NULL;
                p_sub_flux_info->_desc_length = 0;
            }
        }
        sd_free(p_main_flux_info->_p_subtasks);
        p_main_flux_info->_p_subtasks = NULL;
        p_main_flux_info->_subtasks_length = 0;
    }
    sd_free(p_response->_p_flux_infos);
    p_response->_p_flux_infos = NULL;
    p_response->_flux_infos_length = 0;
    return SUCCESS;
}

#ifdef ENABLE_MEMBER
_int32 hsc_set_user_info(const _u64 userid, const char* p_jumpkey, const _u32 jumpkey_len)
{
    _int32      ret_val     = SUCCESS;
    LOG_DEBUG("on hsc_set_user_info userid:%lld, jumpkey_len:%d, jumpkey:%s", userid, jumpkey_len, p_jumpkey);
    if(g_hsc_user_info._p_jump_key)
    {
        sd_free(g_hsc_user_info._p_jump_key);
        g_hsc_user_info._p_jump_key = NULL;
        g_hsc_user_info._jump_key_length = 0;
    }
    g_hsc_user_info._user_id = 0;
    if(userid != 0)
    {
        g_hsc_user_info._user_id = userid;
        ret_val = sd_malloc(jumpkey_len, (void**)&g_hsc_user_info._p_jump_key);
        if(ret_val != SUCCESS)
        {
            LOG_DEBUG("on hsc_set_user_info malloc jumpkey error:%d", ret_val);
            return FALSE;
        }
        sd_memcpy(g_hsc_user_info._p_jump_key, p_jumpkey, jumpkey_len);
        g_hsc_user_info._jump_key_length = jumpkey_len;
    }
    return SUCCESS;
}

_int32 hsc_get_user_info(HSC_USER_INFO** p_user_info)
{
    LOG_DEBUG("on hsc_get_user_info");
    sd_assert(p_user_info);
    *p_user_info = &g_hsc_user_info;
    return SUCCESS;
}

#endif


_int32 hsc_build_batch_commit_cmd(HSC_BATCH_COMMIT_REQUEST* p_request, void** pp_buffer, _int32* p_buffer_len)
{
    _int32      ret_val             = SUCCESS;
    char*       tem_buf             = NULL;
    _int32      tem_len             = 0;
    //char        http_header[1024]   = {0};
    //_u32        http_header_len     = 0;
    _int32      main_taskinfo_index = 0;
    _int32      sub_task_attr_count = 0;
    _int32      sub_task_attr_index = 0;
    HSC_SHUB_MAIN_TASKINFO* p_maintask = NULL;
    HSC_SHUB_SUB_TASK_ATTRIBUTE* p_subtask = NULL;
    LOG_DEBUG("hsc_build_batch_commit_cmd start....request:0x%x", p_request);
    sd_assert(p_request && pp_buffer && p_buffer_len);
    if(p_request == NULL || pp_buffer == NULL || p_buffer_len == NULL)
    {
        LOG_DEBUG("EEEEE hsc_build_batch_commit_cmd error param invalid");
        return FALSE;
    }
    ret_val = hsc_calc_batch_commit_cmd_length(p_request, &(p_request->_header._cmd_length));
    *p_buffer_len = p_request->_header._cmd_length + 12;
    tem_len = *p_buffer_len;
    LOG_DEBUG("hsc_build_batch_commit_cmd _cmd_length:%d, buffer_len:%d", p_request->_header._cmd_length, *p_buffer_len);
    // *p_buffer_len += (HSC_HTTP_HEADER_LEN + SHUB_ENCODE_PADDING_LEN);
    ret_val = sd_malloc((*p_buffer_len) + (HSC_HTTP_HEADER_LEN + SHUB_ENCODE_PADDING_LEN), pp_buffer);
    if(ret_val != SUCCESS)
    {
        LOG_DEBUG("EEEE hsc_build_batch_commit_cmd malloc cmd buffer error:%d", ret_val);
        return ret_val;
    }
    sd_memset(*pp_buffer, 0, *p_buffer_len);
    tem_buf = *pp_buffer;
    LOG_DEBUG("hsc_build_batch_commit_cmd buffer:0x%x,len:%d", *pp_buffer, *p_buffer_len);
    sd_set_int32_to_lt(&tem_buf, &tem_len, (_int32)p_request->_header._protocol_version);
    LOG_DEBUG("hsc_build_batch_commit_cmd protocol_version:%d", p_request->_header._protocol_version);
    sd_set_int32_to_lt(&tem_buf, &tem_len, (_int32)p_request->_header._seq);
    LOG_DEBUG("hsc_build_batch_commit_cmd seq:%d", p_request->_header._seq);
    sd_set_int32_to_lt(&tem_buf, &tem_len, (_int32)p_request->_header._cmd_length);
    LOG_DEBUG("hsc_build_batch_commit_cmd cmd_length:%d", p_request->_header._cmd_length);
    sd_set_int32_to_lt(&tem_buf, &tem_len, (_int32)p_request->_header._client_ver);
    LOG_DEBUG("hsc_build_batch_commit_cmd client_version:%d", p_request->_header._client_ver);
    sd_set_int16_to_lt(&tem_buf, &tem_len, (_int16)p_request->_header._compress_flag);
    LOG_DEBUG("hsc_build_batch_commit_cmd compress_flag:%d", p_request->_header._compress_flag);
    sd_set_int16_to_lt(&tem_buf, &tem_len, (_int16)p_request->_header._cmd_type);
    LOG_DEBUG("hsc_build_batch_commit_cmd cmd_type:%d", p_request->_header._cmd_type);
    sd_set_int64_to_lt(&tem_buf, &tem_len, (_int64)p_request->_user_id);
    LOG_DEBUG("hsc_build_batch_commit_cmd user_id:%i64u", p_request->_user_id);
    sd_set_int32_to_lt(&tem_buf, &tem_len, (_int32)p_request->_login_key_length);
    LOG_DEBUG("hsc_build_batch_commit_cmd login_key_length:%d", p_request->_login_key_length);
    sd_set_bytes(&tem_buf, &tem_len, p_request->_p_login_key, (_int32)p_request->_login_key_length);
    LOG_DEBUG("hsc_build_batch_commit_cmd login_key:%s", p_request->_p_login_key);
    sd_set_int32_to_lt(&tem_buf, &tem_len, (_int32)p_request->_peer_id_length);
    LOG_DEBUG("hsc_build_batch_commit_cmd peer_id_length:%d", p_request->_peer_id_length);
    sd_set_bytes(&tem_buf, &tem_len, p_request->_p_peer_id, (_int32)p_request->_peer_id_length);
    LOG_DEBUG("hsc_build_batch_commit_cmd peer_id:%d", p_request->_p_peer_id);
    sd_set_int32_to_lt(&tem_buf, &tem_len, (_int32)p_request->_main_taskinfo_count);
    LOG_DEBUG("hsc_build_batch_commit_cmd taskinfo_count:%d", p_request->_main_taskinfo_count);
    LOG_DEBUG("hsc_build_batch_commit_cmd comming remain_len:%d, main_taskinfo_count:%d, main_taskinfo_table:0x%x", tem_len, p_request->_main_taskinfo_count, p_request->_p_taskinfos);
    for(main_taskinfo_index = 0; main_taskinfo_index < p_request->_main_taskinfo_count; main_taskinfo_index++)
    {
        LOG_DEBUG("hsc_build_batch_commit_cmd main_taskinfo_index:%d, remain_len:%d",main_taskinfo_index, tem_len);
        p_maintask = p_request->_p_taskinfos + main_taskinfo_index;
        sd_set_int32_to_lt(&tem_buf, &tem_len, (_int32)p_maintask->_main_taskinfo_length);
        sd_set_int32_to_lt(&tem_buf, &tem_len, (_int32)p_maintask->_url_length);
        LOG_DEBUG("hsc_build_batch_commit_cmd url_length:%d", p_maintask->_url_length);
        sd_set_bytes(&tem_buf, &tem_len, p_maintask->_p_url, p_maintask->_url_length);
        sd_set_int32_to_lt(&tem_buf, &tem_len, (_int32)p_maintask->_task_name_length);
        sd_set_bytes(&tem_buf, &tem_len, p_maintask->_p_task_name, p_maintask->_task_name_length);

        sd_set_int8(&tem_buf, &tem_len, (_int8)p_maintask->_res_type);
        
        sd_set_int32_to_lt(&tem_buf, &tem_len, (_int32)p_maintask->_res_id_length);
        sd_set_bytes(&tem_buf, &tem_len, p_maintask->_p_res_id, p_maintask->_res_id_length);
        sd_set_int64_to_lt(&tem_buf, &tem_len, (_int64)p_maintask->_main_server_task_id);
        sd_set_int64_to_lt(&tem_buf, &tem_len, (_int64)p_maintask->_task_size);
        LOG_DEBUG("hsc_build_batch_commit_cmd task_size:%llu", p_maintask->_task_size);
        sub_task_attr_count = p_maintask->_sub_task_attribute_count;
        sd_set_int32_to_lt(&tem_buf, &tem_len, sub_task_attr_count);
        LOG_DEBUG("hsc_build_batch_commit_cmd comming sub_task_attr remain_len:%d, sub_task_attr_table:0x%x, sub_task_attr count:%d", tem_len, p_maintask->_p_sub_task_attribute, sub_task_attr_count);
        for(sub_task_attr_index = 0; sub_task_attr_index < sub_task_attr_count; sub_task_attr_index++)
        {
            LOG_DEBUG("hsc_build_batch_commit_cmd sub_task_attr_index:%d, remain_len:%d",sub_task_attr_index, tem_len);
            p_subtask = p_maintask->_p_sub_task_attribute + sub_task_attr_index;

            sd_set_int32_to_lt(&tem_buf, &tem_len, (_int32)p_subtask->_sub_task_attr_length);
            sd_set_int32_to_lt(&tem_buf, &tem_len, (_int32)p_subtask->_gcid_length);
            LOG_DEBUG("hsc_build_batch_commit_cmd setting gcid remain_len:%d", tem_len);
            sd_set_bytes(&tem_buf, &tem_len, p_subtask->_p_gcid, p_subtask->_gcid_length);
            LOG_DEBUG("hsc_build_batch_commit_cmd subtask gcid len:%d, remain_len:%d", p_subtask->_gcid_length, tem_len);

            sd_set_int32_to_lt(&tem_buf, &tem_len, (_int32)p_subtask->_cid_length);
            LOG_DEBUG("hsc_build_batch_commit_cmd setting cid remain_len:%d", tem_len);
            sd_set_bytes(&tem_buf, &tem_len, p_subtask->_p_cid, p_subtask->_cid_length);
            LOG_DEBUG("hsc_build_batch_commit_cmd subtask cid len:%d, remain_len:%d", p_subtask->_cid_length, tem_len);

            sd_set_int64_to_lt(&tem_buf, &tem_len, p_subtask->_file_size);
            sd_set_int32_to_lt(&tem_buf, &tem_len, p_subtask->_file_index);
            sd_set_int64_to_lt(&tem_buf, &tem_len, p_subtask->_server_task_id);

            sd_set_int32_to_lt(&tem_buf, &tem_len, (_int32)p_subtask->_file_name_length);
            sd_set_bytes(&tem_buf, &tem_len, p_subtask->_p_file_name, p_subtask->_file_name_length);

            sd_set_int16_to_lt(&tem_buf, &tem_len, p_subtask->_flag);
            LOG_DEBUG("hsc_build_batch_commit_cmd attr end remain_len:%d", tem_len);
        }
        sd_set_int8(&tem_buf, &tem_len, p_maintask->_need_query_manager);
    }
    ret_val = sd_set_int32_to_lt(&tem_buf, &tem_len, p_request->_business_flag);
    LOG_DEBUG("hsc_build_batch_commit_cmd at last len:%d", tem_len);
    sd_assert(tem_len == 0);
    return ret_val;
}

_int32 hsc_calc_batch_commit_cmd_length(const HSC_BATCH_COMMIT_REQUEST* p_request, _int32* p_cmd_length)
{
    _int32      ret_val             = SUCCESS;
    _int32      main_task_index     = 0;
    sd_assert(p_request && p_cmd_length);
    if(p_request == NULL || p_cmd_length == NULL)
    {
        LOG_DEBUG("EEEEE hsc_calc_batch_commit_cmd_length error request:0x%x,length:0x%x", p_request, p_cmd_length);
        return INVALID_MEMORY;
    }
    *p_cmd_length       =  8;            //header
    LOG_DEBUG("hsc_calc_batch_commit_cmd_length header cmd_length:%d", *p_cmd_length);
    *p_cmd_length       += 8;
    LOG_DEBUG("hsc_calc_batch_commit_cmd_length xxxx cmd_length:%d", *p_cmd_length);
    *p_cmd_length       += (4 + p_request->_login_key_length);
    LOG_DEBUG("hsc_calc_batch_commit_cmd_length login_ley cmd_length:%d, length:0x%x", *p_cmd_length, &(p_request->_login_key_length));
    *p_cmd_length       += (4 + p_request->_peer_id_length);
    LOG_DEBUG("hsc_calc_batch_commit_cmd_length peer_id cmd_length:%d", *p_cmd_length);
    *p_cmd_length       += (4 + p_request->_taskinfos_length);
    LOG_DEBUG("hsc_calc_batch_commit_cmd_length taskinfo cmd_length:%d", *p_cmd_length);
    *p_cmd_length       += (4);
    LOG_DEBUG("hsc_calc_batch_commit_cmd_length oooo cmd_length:%d", *p_cmd_length);
    return ret_val;
}

_int32 hsc_on_response(const HSC_PQ_TASK_INFO *p_task_info, const char* p_data, const _int32 data_size)
{
    _int32      ret_val             = SUCCESS;
    HSC_BATCH_COMMIT_RESPONSE* response = NULL;
    LOG_DEBUG("on hsc_on_response....");

	sd_assert(p_task_info && p_data && data_size);
	if(p_task_info == NULL || p_data == NULL || data_size == 0)
	{
		LOG_DEBUG("on hsc_on_response param error");
		return INVALID_ARGUMENT;
	}
    LOG_DEBUG("hsc_on_response data:0x%s, size:%d", p_data, data_size);

    ret_val = sd_malloc(sizeof(HSC_BATCH_COMMIT_RESPONSE), (void**)&response);
    if(ret_val != SUCCESS)
    {
        LOG_DEBUG("EEEEE hsc_on_response malloc response error:%d", ret_val);
        return ret_val;
    }
    ret_val = hsc_parser_batch_commit_response(p_data, data_size, response);

	//todo update task's info....

    if(ret_val != SUCCESS)
    {
        LOG_DEBUG("EEEEE hsc_on_response parser response error:%d", ret_val);
        hsc_destroy_batch_commit_response(response);
        sd_free(response);
        response = NULL;
        return ret_val;
    }

    hsc_process_commit_business(p_task_info, response);
    hsc_destroy_batch_commit_response(response);
    sd_free(response);
    response = NULL;
    return ret_val;
}

_int32 hsc_process_commit_business(const HSC_PQ_TASK_INFO *p_task_info, const HSC_BATCH_COMMIT_RESPONSE* response)
{
    _int32          ret_val         = SUCCESS;
    HSC_BATCH_COMMIT_PARAM*  param  = p_task_info->_p_task;
    CONNECT_MANAGER* p_sub_connect_manager = NULL;
    _int32          pi              = 0;
    TASK*           p_task          = NULL;
    _int32          task_count      = p_task_info->task_size;
    _u32            sub_task_index  = 0;
    LIST_ITERATOR   it_file_index   = NULL;
    LIST            lst_acc_files;
    _u32            acc_file_count  = 0;

    LOG_DEBUG("on hsc_process_commit_business enter...");
    sd_assert(p_task_info && response);
    if(p_task_info == NULL || response == NULL || param == NULL)
    {
        LOG_DEBUG("EEEEE on hsc_process_commit_business input param error!!!!");
        return ret_val;
    }
    LOG_DEBUG("on hsc_process_commit_business result:%d, task_count:%d", response->_result, task_count);
    if(response->_result == G_E_OK)
    {
        for(pi = 0; pi < task_count; pi++)
        {
            p_task = (param + pi)->_p_task;
			
            p_task->_hsc_info._errcode       = HSC_BCR_OK;
            p_task->_hsc_info._stat          = HSC_SUCCESS;
            p_task->_hsc_info._cur_cost_flow = response->_needed;
            p_task->_hsc_info._channel_type  = 2;
            p_task->_hsc_info._use_hsc       = TRUE;

            LOG_DEBUG("on hsc_process_commit_business for test commit cuccess, so enable_high_speed_channel index:%d, task:0x%x...", pi, p_task);
            if(p_task->_task_type == BT_TASK_TYPE)
            {
                sd_assert(response->_p_flux_infos[pi]._subtasks_length == list_size(&((param + pi)->_lst_file_index)));
                it_file_index = LIST_BEGIN((param + pi)->_lst_file_index);
                while(it_file_index != LIST_END((param + pi)->_lst_file_index))
                {
                    sub_task_index = (_u32)LIST_VALUE(it_file_index);
                    cm_enable_high_speed_channel(&(p_task->_connect_manager), sub_task_index, TRUE); //set enable flag
                    LOG_DEBUG("on hsc_process_commit_business sub_task_index:%u", sub_task_index);
                    ret_val = cm_get_sub_connect_manager(&(p_task->_connect_manager), sub_task_index, &p_sub_connect_manager);
                    if(ret_val == SUCCESS)
                    {
                        LOG_DEBUG("on hsc_process_commit_business updating cdn pipe:0x%x", p_sub_connect_manager);
                        cm_update_cdn_pipes(p_sub_connect_manager);
                    }
                    it_file_index = LIST_NEXT(it_file_index);
                }
            }
            else
            {
                cm_enable_high_speed_channel(&(p_task->_connect_manager), 0, TRUE);
                cm_update_cdn_pipes(&(p_task->_connect_manager)); // enable immediately
            }
            LOG_DEBUG("hsc_process_commit_business, task_ptr:0x%X, permission success, hsc entered", p_task);
        }
        tm_update_task_hsc_info();
    }
    else if(response->_result == G_E_NOT_ENOUGH_SPACE)
    {
        for(pi = 0; pi < task_count; pi++)
        {
            p_task = (param + pi)->_p_task;

            LOG_DEBUG("on hsc_process_commit_business flux not enough index:%d....", pi);
            if(p_task->_hsc_info._stat == HSC_ENTERING)
            {
                p_task->_hsc_info._errcode       = HSC_BCR_FLUX_NOT_ENOUGH;
                p_task->_hsc_info._stat          = HSC_FAILED;
                p_task->_hsc_info._cur_cost_flow = 0;
                p_task->_hsc_info._channel_type  = -1;
            }
            else if(p_task->_hsc_info._stat == HSC_SUCCESS && p_task->_task_type == BT_TASK_TYPE)
            {
                list_init(&lst_acc_files);
                hsc_get_bt_hsc_list(p_task, &lst_acc_files);
                acc_file_count = list_size(&lst_acc_files);
                LOG_DEBUG("on hsc_process_commit_business bt sub task silent commit flux not enough acc file count:%d....", acc_file_count);
                if(acc_file_count == 0)
                {
                    p_task->_hsc_info._errcode       = HSC_BCR_FLUX_NOT_ENOUGH;
                    p_task->_hsc_info._stat          = HSC_BT_SUBTASK_FAILED;
                    p_task->_hsc_info._cur_cost_flow = 0;
                    p_task->_hsc_info._channel_type  = 2;
                }
                else
                {

                }
                list_clear(&lst_acc_files);
            }
        }
    }
    else
    {
        for(pi = 0; pi < task_count; pi++)
        {
            p_task = (param + pi)->_p_task;

            LOG_DEBUG("on hsc_process_commit_business unknown error index:%d, stat:%d", pi, p_task->_hsc_info._stat);
            if(p_task->_hsc_info._stat == HSC_ENTERING)
            {
                p_task->_hsc_info._errcode       = HSC_BCR_SERVER_ERROR;
                p_task->_hsc_info._stat          = HSC_FAILED;
                p_task->_hsc_info._cur_cost_flow = 0;
                p_task->_hsc_info._channel_type  = -1;
            }
            else if(p_task->_hsc_info._stat == HSC_SUCCESS && p_task->_task_type == BT_TASK_TYPE)
            {
                list_init(&lst_acc_files);
                hsc_get_bt_hsc_list(p_task, &lst_acc_files);
                acc_file_count = list_size(&lst_acc_files);
                LOG_DEBUG("WWWWW on hsc_process_commit_business bt sub task silent commit error acc file count:%d....", acc_file_count);
                if(acc_file_count == 0)
                {
                    p_task->_hsc_info._errcode       = HSC_BCR_SERVER_ERROR;
                    p_task->_hsc_info._stat          = HSC_BT_SUBTASK_FAILED;
                    p_task->_hsc_info._cur_cost_flow = 0;
                    p_task->_hsc_info._channel_type  = 2;
                }
                else
                {}
                list_clear(&lst_acc_files);
            }
        }
    }
    LOG_DEBUG("on hsc_process_commit_business end...");
    return ret_val;
}

_int32 hsc_get_bt_hsc_list(TASK* p_task, LIST* p_list)
{
    _int32      ret_val         = SUCCESS;
    _u32        file_index      = 0;
    LIST        lst_acc_index;
    LIST_ITERATOR it,end;
    LOG_DEBUG("on hsc_get_bt_hsc_list p_task:0x%x, id:%u, list:0x%x", p_task, p_task->_task_id, p_list);
    if(p_task->_task_type != BT_TASK_TYPE)
        return FALSE;

    list_init(&lst_acc_index);
    ret_val = cm_get_bt_acc_file_idx_list(&p_task->_connect_manager, &lst_acc_index);
    if(ret_val != SUCCESS)
    {
        LOG_DEBUG("on hsc_get_bt_hsc_list cm_get_bt_acc_file_idx_list error:%d", ret_val);
        list_clear(&lst_acc_index);
        return ret_val;
    }
    
    it      = LIST_BEGIN(lst_acc_index);
    end     = LIST_END(lst_acc_index);
    file_index = list_size(&lst_acc_index);
    LOG_DEBUG("on hsc_get_bt_hsc_list ready to acc sub task count:%d", file_index);
    while(it != end)
    {
        file_index = (_u32)LIST_VALUE(it);
        if(TRUE == cm_check_high_speed_channel_flag(&p_task->_connect_manager, file_index))
        {
            LOG_DEBUG("on hsc_get_bt_hsc_list find acc file index:%d", file_index);
            list_push(p_list, (void*)file_index);
        }
        it = LIST_NEXT(it);
    }
    list_clear(&lst_acc_index);
    return ret_val;
}

_int32 hsc_build_http_package(const char* data, const _int32 data_len, char** buffer, _int32* buffer_len)
{
    _int32      ret_val         = SUCCESS;
    char*       host            = DEFAULT_LOCAL_IP;	//"service.cdn.vip.xunlei.com";
    char*       tem_buf         = NULL;
    char*       header          = "POST / HTTP/1.1\r\nHost: %s\r\nContent-Length: %d\r\nContent-Type: application/octet-stream\r\nUser-Agent: Mozilla/5.0\r\nConnection: Keep-Alive\r\nAccept: */*\r\n\r\n";
    if(buffer == NULL || buffer_len == NULL || data == NULL || data_len == 0)
    {
        LOG_DEBUG("EEEEE hsc_build_http_package error param err:");
        return FALSE;
    }
    ret_val     = sd_malloc(HSC_HTTP_HEADER_LEN + data_len, (void**)buffer);
    if(ret_val != SUCCESS)
    {
        LOG_DEBUG("on hsc_build_http_package malloc buffer err:%d", ret_val);
        return ret_val;
    }
    sd_memset(*buffer, 0, HSC_HTTP_HEADER_LEN + data_len);
    sd_snprintf(*buffer,HSC_HTTP_HEADER_LEN + data_len, header, host, data_len);
    *buffer_len = sd_strlen(*buffer);
    LOG_DEBUG("on hsc_build_http_package header len:%d, data:%s", *buffer_len, *buffer);
    tem_buf = (*buffer) + (*buffer_len);
    sd_memcpy(tem_buf, data, data_len);
    *buffer_len += data_len;
    LOG_DEBUG("on hsc_build_http_package package len:%d, data_len:%d", *buffer_len, data_len);
    return SUCCESS;
}

_int32 hsc_get_task_name(const HSC_BATCH_COMMIT_PARAM* p_param, char* buffer, _int32* len)
{
    _int32          ret_val             = SUCCESS;
    char*           p_name_buffer       = NULL;
    _int32          name_buffer_len     = 0;
    BT_TASK*        p_bt_task           = NULL;
    char*           tem_p               = NULL;
    TASK*           p_task              = p_param->_p_task;

    LOG_DEBUG("on hsc_get_task_name start  task:0x%x, len:%d", p_task, *len);
    sd_assert(p_task && buffer && len);
    if(p_task == NULL || buffer == NULL || len == NULL)
    {
        LOG_DEBUG("EEEEE on hsc_get_task_name param error!!!!");
        return INVALID_MEMORY;
    }
    sd_memset(buffer, 0, *len);
    if(p_task->_task_type == P2SP_TASK_TYPE)
    {
        if(p_param->_p_task_name && p_param->_task_name_length)
        {
            ret_val = sd_any_format_to_gbk(p_param->_p_task_name, p_param->_task_name_length, buffer, len);
            if(ret_val != SUCCESS)
            {
                *len = 0; 
            }
            LOG_DEBUG("on hsc_get_task_name using etm taskname:%d, %s", p_param->_task_name_length, p_param->_p_task_name);
        }
        else
        {
            ret_val = pt_get_task_file_name(p_task, NULL, &name_buffer_len);
            if(ret_val == DT_ERR_BUFFER_NOT_ENOUGH)
            {
                ret_val = sd_malloc(name_buffer_len, (void**)&p_name_buffer);
                if(ret_val != SUCCESS)
                {
                    LOG_DEBUG("EEEEE on hsc_get_task_name malloc name_buffer error:%d", ret_val);
                    goto HANDLE_ERROR;
                }
                ret_val = pt_get_task_file_name(p_task, p_name_buffer, &name_buffer_len);
                if(ret_val != SUCCESS)
                {
                    LOG_DEBUG("EEEEE on hsc_get_task_name pt_get_task_file_name error:%d", ret_val);
                    goto HANDLE_ERROR;
                }
                //*len = (*len) > name_buffer_len ? name_buffer_len : (*len);
                //sd_memcpy(buffer, p_name_buffer, *len);
                tem_p = sd_strstr(p_name_buffer, ".rf", 0);
                if(tem_p != NULL)
                {
                    name_buffer_len -= 4;
                }
                ret_val = sd_any_format_to_gbk(p_name_buffer, name_buffer_len, buffer, len);
                if(ret_val != SUCCESS)
                {
                    *len = 0; 
                }
                LOG_DEBUG("on hsc_get_task_name gbk len:%d, name:%s", *len, buffer);
                sd_free(p_name_buffer);
                p_name_buffer = NULL;
            }
        }
    }
#ifdef ENABLE_EMULE
    else if(p_task->_task_type == EMULE_TASK_TYPE)
    {
        if(p_param->_p_task_name && p_param->_task_name_length)
        {
            ret_val = sd_any_format_to_gbk(p_param->_p_task_name, p_param->_task_name_length, buffer, len);
            if(ret_val != SUCCESS)
            {
                *len = 0; 
            }
            LOG_DEBUG("on hsc_get_task_name using etm taskname:%d, %s", p_param->_task_name_length, p_param->_p_task_name);
        }
        else
        {
            ret_val = emule_get_task_file_name(p_task, NULL, &name_buffer_len);
            if(ret_val == DT_ERR_BUFFER_NOT_ENOUGH)
            {
                ret_val = sd_malloc(name_buffer_len, (void**)&p_name_buffer);
                if(ret_val != SUCCESS)
                {
                    LOG_DEBUG("EEEEE on hsc_get_task_name malloc name_buffer error:%d", ret_val);
                    goto HANDLE_ERROR;
                }
                ret_val = emule_get_task_file_name(p_task, p_name_buffer, &name_buffer_len);
                if(ret_val != SUCCESS)
                {
                    LOG_DEBUG("EEEEE on hsc_get_task_name emule_get_task_file_name error:%d", ret_val);
                    goto HANDLE_ERROR;
                }
                //*len = (*len) > name_buffer_len ? name_buffer_len : (*len);
                //sd_memcpy(buffer, p_name_buffer, *len);
                tem_p = sd_strstr(p_name_buffer, ".rf", 0);
                if(tem_p != NULL)
                {
                    name_buffer_len -= 4;
                }
                ret_val = sd_any_format_to_gbk(p_name_buffer, name_buffer_len, buffer, len);
                if(ret_val != SUCCESS)
                {
                    *len = 0; 
                }
                LOG_DEBUG("on hsc_get_task_name gbk len:%d, name:%s", *len, buffer);
                sd_free(p_name_buffer);
                p_name_buffer = NULL;
            }
        }
    }
#endif
#ifdef ENABLE_BT
    else if(p_task->_task_type == BT_TASK_TYPE)
    {
        p_bt_task = (BT_TASK*)p_task;
        if(p_bt_task->_torrent_parser_ptr)
        {
            name_buffer_len = p_bt_task->_torrent_parser_ptr->_title_name_len;
            //*len = (*len) > name_buffer_len ? name_buffer_len : (*len);
            //sd_memcpy(buffer, p_bt_task->_torrent_parser_ptr->_title_name, *len);
            ret_val = sd_any_format_to_gbk(p_bt_task->_torrent_parser_ptr->_title_name, name_buffer_len, buffer, len);
            if(ret_val != SUCCESS)
            {
                *len = 0; 
            }
            LOG_DEBUG("on hsc_get_task_name len:%d, buffer:%s", *len, buffer);
        }
        else
        {
            *len = 0;
            LOG_DEBUG("EEEEE on hsc_get_task_name bt task has no torrent_parser_ptr, so title is empty!!!!!");
        }
        p_name_buffer = NULL;
    }
#endif
    else
    {
        sd_assert(FALSE);
    }
    LOG_DEBUG("on hsc_get_task_name ok!!!! len:%d, name:%s", *len, buffer);
    return SUCCESS;

HANDLE_ERROR:
    if(p_name_buffer != NULL)
    {
        sd_free(p_name_buffer);
        p_name_buffer = NULL;
    }
    return ret_val;
}

_int32 hsc_get_task_res_id(const TASK* p_task, char* info_hash, _int32* info_hash_len)
{
    _int32          ret_val     = 0;
    BT_TASK*        p_bt_task   = NULL;
    LOG_DEBUG("on hsc_get_task_res_id p_task:0x%x:", p_task);
    sd_assert(p_task && info_hash && info_hash_len);
    if(p_task == NULL || info_hash == NULL || info_hash_len == NULL)
    {
        LOG_DEBUG("EEEEE on hsc_get_business_flag param error!!!!!");
        return FALSE;
    }
    *info_hash_len              = 0;
    sd_memset(info_hash, 0, URL_MAX_LEN);
    if(p_task->_task_type == BT_TASK_TYPE)
    {
        p_bt_task = (BT_TASK*)p_task;
        if(p_bt_task->_torrent_parser_ptr)
        {
            //ret_val = sd_memcpy(info_hash, p_bt_task->_torrent_parser_ptr->_info_hash, INFO_HASH_LEN);
            ret_val = str2hex( (char*)p_bt_task->_torrent_parser_ptr->_info_hash, INFO_HASH_LEN, info_hash, INFO_HASH_LEN*2);
            if(ret_val == SUCCESS)
                *info_hash_len = INFO_HASH_LEN * 2; 
        }
        else
        {
            LOG_DEBUG("EEEEE on hsc_get_task_res_id bt task has no torrent_parser_ptr, so info_hash is empty!!!!!");
        }
    }
    LOG_DEBUG("on hsc_get_task_res_id len:%d, info_hash:%s", *info_hash_len, info_hash);
    return ret_val;
}

_int32 hsc_remove_task_according_to_task_info(TASK *task_ptr)
{
	HSC_PQ_TASK_INFO *p_task_info = NULL;
	
    LOG_DEBUG("hsc_remove_task_for_task_info task_ptr=0x%x", task_ptr);
    if (NULL == task_ptr)
    {
        LOG_DEBUG("hsc_remove_task_for_task_info error INVALID_ARGUMENT");
        return INVALID_ARGUMENT;
    }
    LOG_DEBUG("hsc_remove_task_for_task_info list_size=%d", list_size(&(g_hsc_pq_task_manager._hsc_pq_task_list)));
    LIST_ITERATOR iter = LIST_BEGIN(g_hsc_pq_task_manager._hsc_pq_task_list);
    LIST_ITERATOR end_node = LIST_END(g_hsc_pq_task_manager._hsc_pq_task_list);
    while (iter != end_node)
    {
		p_task_info = (HSC_PQ_TASK_INFO*)LIST_VALUE(iter);
        if (task_ptr == p_task_info->_p_task->_p_task)
        {
            LOG_DEBUG("hsc_remove_task_for_task_info delete task p_task_info= %p", p_task_info);
            LIST_ITERATOR tmp_iter = iter;
            iter = LIST_NEXT(iter);
            list_erase(&g_hsc_pq_task_manager._hsc_pq_task_list, tmp_iter);
			hsc_destroy_HSC_PQ_TASK_INFO(p_task_info);
			sd_free(p_task_info);
			break;
        }
            iter = LIST_NEXT(iter);
    }
    return SUCCESS;
}



//jjxh added....
#endif /* ENABLE_HSC */

