#include "http_mini.h"
#include "data_manager/data_manager_interface.h"
#include "connect_manager/pipe_interface.h"
#include "utility/url.h"
#include "utility/map.h"
#include "utility/utility.h"
#include "utility/settings.h"
#include "utility/mempool.h"

#include "utility/logid.h"
#define LOGID LOGID_HTTP_PIPE
#include "utility/logger.h"

#include "task_manager/task_manager.h"

//////////////////////////////
static _u32	g_mini_http_num = 0;
static MAP g_mini_https;
static SLAB *gp_mini_http_slab = NULL;
static void* g_mini_http_function_table[MAX_FUNC_NUM];
static PIPE_INTERFACE g_pipe_interface;
static _u32	g_timeout_id = INVALID_MSG_ID;
//////////////////////////////

_int32 init_mini_http(void)
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "init_mini_http" );
	
	if(gp_mini_http_slab==NULL)
	{
		ret_val = mpool_create_slab(sizeof(MINI_HTTP), MIN_MINI_HTTP_MEMORY, 0, &gp_mini_http_slab);
		CHECK_VALUE(ret_val);
	}
	
	map_init(&g_mini_https,mini_http_id_comp);
	g_mini_http_num = 0;
	g_mini_http_function_table[0] = (void*)mini_http_change_range;
	g_mini_http_function_table[1] = (void*)mini_http_get_dispatcher_range;
	g_mini_http_function_table[2] = (void*)mini_http_set_dispatcher_range;
	g_mini_http_function_table[3] = (void*)mini_http_get_file_size;
	g_mini_http_function_table[4] = (void*)mini_http_set_file_size;
	g_mini_http_function_table[5] = (void*)mini_http_get_data_buffer;
	g_mini_http_function_table[6] = (void*)mini_http_free_data_buffer;
	g_mini_http_function_table[7] =(void*) mini_http_put_data_buffer;
	g_mini_http_function_table[8] = (void*)mini_http_read_data;
	g_mini_http_function_table[9] = (void*)mini_http_notify_dispatch_data_finish;
	g_mini_http_function_table[10] = (void*)mini_http_get_file_type;
	g_mini_http_function_table[11] = NULL;
	g_pipe_interface._file_index = 0;
	g_pipe_interface._range_switch_handler = NULL;
	g_pipe_interface._func_table_ptr = g_mini_http_function_table;

	g_timeout_id = INVALID_MSG_ID;
	LOG_DEBUG( "init_mini_http success!" );
	return SUCCESS;
}

_int32 uninit_mini_http(void)
{
	LOG_DEBUG( "uninit_mini_http" );
	if(gp_mini_http_slab==NULL)
	{
		return SUCCESS;
	}
	sd_assert(map_size(&g_mini_https) == 0);

	if(g_timeout_id != INVALID_MSG_ID)
	{
		cancel_timer(g_timeout_id);
		g_timeout_id = INVALID_MSG_ID;
	}
	
	mini_http_clear_map();

	if(gp_mini_http_slab!=NULL)
	{
		mpool_destory_slab(gp_mini_http_slab);
		gp_mini_http_slab = NULL;
	}

	LOG_DEBUG( "uninit_mini_http success!" );
	return SUCCESS;
}

_int32 sd_http_clear(void)
{
	LOG_ERROR( "mini_http:sd_http_clear" );
	if(gp_mini_http_slab==NULL)
	{
		return SUCCESS;
	}
	mini_http_clear_map();
	return SUCCESS;
}

 _int32 sd_http_get(HTTP_PARAM * param,_u32 * http_id)
{
	_int32 ret_val = SUCCESS;
	MINI_HTTP * p_mini_http = NULL;
	char * p_request=NULL;
	//_u32 request_len=0;
	RANGE rg;
	LOG_DEBUG( "mini_http:sd_http_get[%u]:%s",param->_url_len,param->_url);
	
	if(gp_mini_http_slab==NULL)
	{
		return -1;
	}
	
	ret_val =mini_http_malloc(&p_mini_http);
	CHECK_VALUE(ret_val);

	sd_memcpy(&p_mini_http->_http_param, param, sizeof(HTTP_PARAM));

	ret_val = http_resource_create((char*)param->_url, param->_url_len, param->_ref_url, param->_ref_url_len, TRUE, &p_mini_http->_res);
	if(ret_val != SUCCESS) goto ErrorHanle;

	ret_val = http_pipe_create(NULL, p_mini_http->_res, (DATA_PIPE**)&p_mini_http->_http_pipe);
	if(ret_val != SUCCESS) goto ErrorHanle;
	
	dp_set_pipe_interface((DATA_PIPE*)p_mini_http->_http_pipe, &g_pipe_interface);

	p_mini_http->_http_pipe->_max_try_count = 10;
	
	ret_val =http_resource_set_cookies((HTTP_SERVER_RESOURCE * )p_mini_http->_res,param->_cookie, param->_cookie_len);
	if(ret_val != SUCCESS) goto ErrorHanle;
	
	http_resource_set_send_gzip((HTTP_SERVER_RESOURCE * )p_mini_http->_res,param->_send_gzip);
	http_resource_set_gzip((HTTP_SERVER_RESOURCE * )p_mini_http->_res,param->_accept_gzip);
	
	if(param->_recv_buffer_size!=0)
	{
		p_mini_http->_recv_data_need_notified= TRUE;
	}
	
	p_mini_http->_id = mini_http_create_id();
	p_mini_http->_http_pipe->_data_pipe._id = p_mini_http->_id;
	sd_time_ms(&p_mini_http->_start_time);
	if(p_mini_http->_http_param._timeout == 0)
	{
		p_mini_http->_http_param._timeout = DEFAULT_MINI_HTTP_TASK_TIMEOUT;
	}

	ret_val = http_pipe_open((DATA_PIPE*)p_mini_http->_http_pipe);
	if(ret_val != SUCCESS) goto ErrorHanle;

	rg._index = 0;
	rg._num = MAX_U32;
	ret_val = pi_pipe_change_range((DATA_PIPE*)p_mini_http->_http_pipe, &rg, FALSE);
	if(ret_val != SUCCESS) goto ErrorHanle;

	ret_val = mini_http_add_to_map(p_mini_http);
	if(ret_val != SUCCESS) goto ErrorHanle;

	*http_id = p_mini_http->_id ;

	LOG_DEBUG( "mini_http:sd_http_get success!");
	return SUCCESS;

ErrorHanle:
	LOG_DEBUG( "mini_http:sd_http_get FAILED:%d!",ret_val);
	if(p_mini_http->_id != 0)
	{
		mini_http_decrease_id();
	}

	SAFE_DELETE(p_request);

	if(p_mini_http->_http_pipe)
	{
		http_pipe_close((DATA_PIPE*)p_mini_http->_http_pipe);
	}
	
	if(p_mini_http->_res)
	{
		http_resource_destroy(&p_mini_http->_res);
	}
	
	if(p_mini_http)
	{
		mini_http_free(p_mini_http);
	}
	return ret_val;
}

 _int32 sd_http_post(HTTP_PARAM * param,_u32 * http_id)
{
	_int32 ret_val = SUCCESS;
	MINI_HTTP * p_mini_http = NULL;
	char * p_request=NULL;
	//_u32 request_len=0;
	RANGE rg;
	LOG_DEBUG( "mini_http:sd_http_post[%u]:%s,content_len=%llu,send_data_len=%u",param->_url_len,param->_url,param->_content_len,param->_send_data_len);
	
	if(gp_mini_http_slab==NULL)
	{
		return -1;
	}
	
	ret_val =mini_http_malloc(&p_mini_http);
	CHECK_VALUE(ret_val);

	sd_memcpy(&p_mini_http->_http_param, param, sizeof(HTTP_PARAM));

	ret_val = http_resource_create((char*)param->_url, param->_url_len, param->_ref_url, param->_ref_url_len, TRUE, &p_mini_http->_res);
	if(ret_val != SUCCESS) goto ErrorHanle;

	//ret_val =http_resource_set_extra_header((HTTP_SERVER_RESOURCE * )p_mini_http->_res,p_mini_http->_http_param._send_header,p_mini_http->_http_param._send_header_len);
	//if(ret_val != SUCCESS) goto ErrorHanle;

	ret_val =http_resource_set_cookies((HTTP_SERVER_RESOURCE * )p_mini_http->_res,param->_cookie, param->_cookie_len);
	if(ret_val != SUCCESS) goto ErrorHanle;
	
	ret_val =http_resource_set_post_content_len((HTTP_SERVER_RESOURCE * )p_mini_http->_res,param->_content_len);
	if(ret_val != SUCCESS) goto ErrorHanle;

	if(param->_content_len!=0)
	{
		p_mini_http->_send_data_need_notified = TRUE;
	}
	
	if(param->_recv_buffer_size!=0)
	{
		p_mini_http->_recv_data_need_notified= TRUE;
	}
	
	http_resource_set_send_gzip((HTTP_SERVER_RESOURCE * )p_mini_http->_res,param->_send_gzip);
	http_resource_set_gzip((HTTP_SERVER_RESOURCE * )p_mini_http->_res,param->_accept_gzip);
	
	//ret_val =http_resource_set_post_data((HTTP_SERVER_RESOURCE * )p_mini_http->_res,p_mini_http->_http_param._data,p_mini_http->_http_param._data_len);
	//if(ret_val != SUCCESS) goto ErrorHanle;
	
	ret_val = http_pipe_create(NULL, p_mini_http->_res, (DATA_PIPE**)&p_mini_http->_http_pipe);
	if(ret_val != SUCCESS) goto ErrorHanle;
	
	dp_set_pipe_interface((DATA_PIPE*)p_mini_http->_http_pipe, &g_pipe_interface);

	p_mini_http->_http_pipe->_max_try_count = 10;

	
	p_mini_http->_id = mini_http_create_id();
	p_mini_http->_http_pipe->_data_pipe._id = p_mini_http->_id;
	sd_time_ms(&p_mini_http->_start_time);
	if(p_mini_http->_http_param._timeout == 0)
	{
		p_mini_http->_http_param._timeout = DEFAULT_MINI_HTTP_TASK_TIMEOUT;
	}

	ret_val = http_pipe_open((DATA_PIPE*)p_mini_http->_http_pipe);
	if(ret_val != SUCCESS) goto ErrorHanle;

	rg._index = 0;
	rg._num = MAX_U32;
	ret_val = pi_pipe_change_range((DATA_PIPE*)p_mini_http->_http_pipe, &rg, FALSE);
	if(ret_val != SUCCESS) goto ErrorHanle;

	ret_val = mini_http_add_to_map(p_mini_http);
	if(ret_val != SUCCESS) goto ErrorHanle;

	*http_id = p_mini_http->_id ;

	LOG_DEBUG("id=%d",*http_id);

	LOG_DEBUG( "mini_http:sd_http_post success!");
	return SUCCESS;

ErrorHanle:
	LOG_DEBUG( "mini_http:sd_http_post FAILED:%d!",ret_val);
	if(p_mini_http->_id != 0)
	{
		mini_http_decrease_id();
	}

	SAFE_DELETE(p_request);

	if(p_mini_http->_http_pipe)
	{
		http_pipe_close((DATA_PIPE*)p_mini_http->_http_pipe);
	}
	
	if(p_mini_http->_res)
	{
		http_resource_destroy(&p_mini_http->_res);
	}
	
	if(p_mini_http)
	{
		mini_http_free(p_mini_http);
	}
	return ret_val;
}
 
  _int32 sd_http_cancel(_u32 http_id)
 {
	 MINI_HTTP * p_mini_http =NULL;
	 
	 LOG_DEBUG( "mini_http:sd_http_cancel:%u!",http_id);
	 if(gp_mini_http_slab==NULL)
	 {
		 return -1;
	 }
	 
	 p_mini_http =mini_http_get_from_map(http_id);
	 if(p_mini_http == NULL) return -1;
 
	 LOG_DEBUG("sd_http_cancel--get from map success");
	 
	 mini_http_cancel(p_mini_http);
 
	 mini_http_remove_from_map(p_mini_http);
	 
	 mini_http_free(p_mini_http);
	 
	 return SUCCESS;
 }

 _int32 sd_http_close(_u32 http_id)
{
	MINI_HTTP * p_mini_http =NULL;
	
	LOG_DEBUG( "mini_http:sd_http_close:%u!",http_id);
	if(gp_mini_http_slab==NULL)
	{
		return -1;
	}
	
	p_mini_http =mini_http_get_from_map(http_id);
	if(p_mini_http == NULL) return -1;

	LOG_DEBUG("sd_http_close--get from map success");
	
	mini_http_close(p_mini_http,MINI_HTTP_CLOSE_NORMAL);

	mini_http_remove_from_map(p_mini_http);
	
	mini_http_free(p_mini_http);
	
	return SUCCESS;
}

_int32 mini_http_id_comp(void *E1, void *E2)
{
	return ((_int32)E1) -((_int32)E2);
}

_int32 mini_http_change_range(struct tagDATA_PIPE *p_data_pipe, const struct _tag_range *range, BOOL cancel_flag )
{
	LOG_DEBUG( "mini_http_change_range:%u[%u,%u]!",p_data_pipe->_id,range->_index,range->_num);
	return http_pipe_change_ranges(p_data_pipe, range);
}

_int32 mini_http_get_dispatcher_range(struct tagDATA_PIPE *p_data_pipe, const struct _tag_range *p_dispatcher_range, void *p_pipe_range )
{
	RANGE* range = (RANGE*)p_pipe_range;
	range->_index = p_dispatcher_range->_index;
	range->_num = p_dispatcher_range->_num;
	LOG_DEBUG( "mini_http_get_dispatcher_range:%u[%u,%u]!",p_data_pipe->_id,range->_index,range->_num);
	return SUCCESS;
}

_int32 mini_http_set_dispatcher_range(struct tagDATA_PIPE *p_data_pipe, const void *p_pipe_range, struct _tag_range *p_dispatcher_range )
{
	RANGE* range = (RANGE*)p_pipe_range;
	p_dispatcher_range->_index = range->_index;
	p_dispatcher_range->_num = range->_num;
	LOG_DEBUG( "mini_http_set_dispatcher_range:%u[%u,%u]!",p_data_pipe->_id,range->_index,range->_num);
	return SUCCESS;
}

_u64 mini_http_get_file_size( struct tagDATA_PIPE *p_data_pipe)
{
	MINI_HTTP * p_mini_http = NULL;
	
	if(gp_mini_http_slab==NULL)
	{
		return -1;
	}
	
	p_mini_http =mini_http_get_from_map(p_data_pipe->_id);
	
	if(p_mini_http == NULL) return 0;

	LOG_DEBUG( "mini_http_get_file_size:%u,%llu!",p_data_pipe->_id,p_mini_http->_filesize);
	return p_mini_http->_filesize ;
}

_int32 mini_http_set_file_size( struct tagDATA_PIPE *p_data_pipe, _u64 filesize)
{
	MINI_HTTP * p_mini_http =NULL;
	
	LOG_DEBUG( "mini_http_set_file_size:%u,%llu!",p_data_pipe->_id,filesize);
	if(gp_mini_http_slab==NULL)
	{
		return -1;
	}

	p_mini_http =mini_http_get_from_map(p_data_pipe->_id);
	
	if(p_mini_http == NULL) return -1;

	p_mini_http->_filesize = filesize;
	
	return SUCCESS;
}
/* 获取缓存 */
_int32 mini_http_get_data_buffer( struct tagDATA_PIPE *p_data_pipe, char **pp_data_buffer, _u32 *p_data_len)
{
	//LOG_DEBUG( "mini_http_get_data_buffer:%u!",p_data_pipe->_id);
	//return dm_get_data_buffer(NULL, pp_data_buffer, p_data_len);

	_int32 ret_val = SUCCESS;
	MINI_HTTP * p_mini_http =NULL;
	HTTP_CALL_BACK_FUNCTION callback_fun ;
	HTTP_CALL_BACK http_call_back_param;
	_u32 expected_data_len = *p_data_len;
	
	LOG_DEBUG( "mini_http_get_data_buffer:%u!",p_data_pipe->_id);
	if(gp_mini_http_slab==NULL)
	{
		return -1;
	}
	p_mini_http =mini_http_get_from_map(p_data_pipe->_id);
	if(p_mini_http == NULL) return -1;

	callback_fun = (HTTP_CALL_BACK_FUNCTION)p_mini_http->_http_param._callback_fun;

	if((p_mini_http->_dledsize==0)&&(p_mini_http->_http_param._recv_buffer_size!=0))
	{
		/* 第一块缓存可以从创建会话中的参数中传进来 */
		*pp_data_buffer =p_mini_http->_http_param._recv_buffer;
		*p_data_len = p_mini_http->_http_param._recv_buffer_size;
		p_mini_http->_current_recv_data = (_u8*)*pp_data_buffer;
		p_mini_http->_recv_data_need_notified = TRUE;
		sd_assert(*p_data_len>=expected_data_len);
		return SUCCESS;
	}

	/* 通过回调函数从界面获取缓存 */
	sd_memset(&http_call_back_param,0x00,sizeof(HTTP_CALL_BACK));
	http_call_back_param._http_id = p_data_pipe->_id;
	http_call_back_param._user_data= p_mini_http->_http_param._user_data;
	http_call_back_param._type = EHC_GET_RECV_BUFFER;
	http_call_back_param._recv_buffer = (void**)pp_data_buffer;
	http_call_back_param._recv_buffer_len= p_data_len;
	
	ret_val = callback_fun(&http_call_back_param);
	if(ret_val==SUCCESS)
	{
	p_mini_http->_current_recv_data = (_u8*)*pp_data_buffer;
	p_mini_http->_recv_data_need_notified = TRUE;
	sd_assert(*p_data_len>=expected_data_len);
	}
	else
	{
		p_mini_http->_current_recv_data = NULL;
		p_mini_http->_recv_data_need_notified = FALSE;
		*pp_data_buffer = NULL;
		*p_data_len = 0;
	}
	return ret_val;

}

/* 释放内存*/
_int32 mini_http_free_data_buffer( struct tagDATA_PIPE *p_data_pipe, char **pp_data_buffer, _u32 data_len)
{
	_int32 ret_val = SUCCESS;
	MINI_HTTP * p_mini_http =NULL;
	HTTP_CALL_BACK_FUNCTION callback_fun ;
	HTTP_CALL_BACK http_call_back_param;
	
	LOG_DEBUG( "mini_http_free_data_buffer:%u!",p_data_pipe->_id);
	if(gp_mini_http_slab==NULL)
	{
		return -1;
	}
	p_mini_http =mini_http_get_from_map(p_data_pipe->_id);
	if(p_mini_http == NULL) return -1;

	callback_fun = (HTTP_CALL_BACK_FUNCTION)p_mini_http->_http_param._callback_fun;

	if(!p_mini_http->_recv_data_need_notified)  return SUCCESS;

	sd_assert(p_mini_http->_current_recv_data!=NULL);

	//p_mini_http->_dledsize+=data_len;

	sd_memset(&http_call_back_param,0x00,sizeof(HTTP_CALL_BACK));
	http_call_back_param._http_id = p_data_pipe->_id;
	http_call_back_param._user_data= p_mini_http->_http_param._user_data;
	http_call_back_param._type = EHC_PUT_RECVED_DATA;
	http_call_back_param._recved_data = (_u8*)*pp_data_buffer;
	http_call_back_param._recved_data_len= 0;//data_len;
	
	callback_fun(&http_call_back_param);
	
	*pp_data_buffer = NULL;
	p_mini_http->_current_recv_data = NULL;
	p_mini_http->_recv_data_need_notified = FALSE;
	return ret_val;
}

/* 当前数据块下载完毕,向界面回调通知*/
_int32 mini_http_put_data_buffer( struct tagDATA_PIPE *p_data_pipe, const struct _tag_range *p_range, char **pp_data_buffer, _u32 data_len,_u32 buffer_len, struct tagRESOURCE *p_resource )
{
	_int32 ret_val = SUCCESS;
	MINI_HTTP * p_mini_http =NULL;
	HTTP_CALL_BACK_FUNCTION callback_fun ;
	HTTP_CALL_BACK http_call_back_param;
	
	LOG_DEBUG( "mini_http_put_data_buffer:%u,[%u,%u],data_len=%u,buffer_len=%u!",p_data_pipe->_id,p_range->_index,p_range->_num,data_len,buffer_len);
	if(gp_mini_http_slab==NULL)
	{
		return -1;
	}
	p_mini_http =mini_http_get_from_map(p_data_pipe->_id);
	if(p_mini_http == NULL) return -1;

	callback_fun = (HTTP_CALL_BACK_FUNCTION)p_mini_http->_http_param._callback_fun;

	if(!p_mini_http->_recv_data_need_notified)  return SUCCESS;

	sd_assert(p_mini_http->_current_recv_data!=NULL);


	p_mini_http->_dledsize+=data_len;

	sd_memset(&http_call_back_param,0x00,sizeof(HTTP_CALL_BACK));
	http_call_back_param._http_id = p_data_pipe->_id;
	http_call_back_param._user_data= p_mini_http->_http_param._user_data;
	http_call_back_param._type = EHC_PUT_RECVED_DATA;
	http_call_back_param._recved_data = (_u8*)*pp_data_buffer;
	http_call_back_param._recved_data_len= data_len;
	
	callback_fun(&http_call_back_param);

	*pp_data_buffer = NULL;
	p_mini_http->_current_recv_data = NULL;
	p_mini_http->_recv_data_need_notified = FALSE;	
	
	/* 是否可以判断出所有数据下载完毕? */
	if(((p_mini_http->_filesize!=0)&&(p_mini_http->_dledsize==p_mini_http->_filesize)))
		/*||(buffer_len==RANGE_DATA_UNIT_SIZE && data_len<buffer_len))  // 去掉这种对chunked类型的山寨判断方式-- By zyq@20120804 for ipadkankan 3.12  */
	{
		LOG_DEBUG( "mini_http_put_data_buffer,MINI_HTTP_SUCCESS:%u,_filesize=%llu,dledsize=%llu,data_len=%u,buffer_len=%u!",p_data_pipe->_id,p_mini_http->_filesize,p_mini_http->_dledsize,data_len,buffer_len);
		p_mini_http->_state =MINI_HTTP_SUCCESS;
	}

	if((p_mini_http->_state ==MINI_HTTP_SUCCESS)&&(!p_mini_http->_notified))
	{
		LOG_DEBUG( "mini_http_put_data_buffer,HRT_FINISHED,resp_callback:%u!",p_data_pipe->_id);
		/* 所有数据下载完毕,向界面回调通知*/
		p_mini_http->_notified = TRUE;
		http_call_back_param._type = EHC_NOTIFY_FINISHED;
		http_call_back_param._recved_data = NULL;
		http_call_back_param._recved_data_len= 0;
		http_call_back_param._result = 0;
		callback_fun(&http_call_back_param);
	}
	
	return ret_val;

}

/* 该接口只有具有上传功能的P2P pipe 才需要用到，http不用*/
_int32  mini_http_read_data( struct tagDATA_PIPE *p_data_pipe, void *p_user, struct _tag_range_data_buffer* p_data_buffer, notify_read_result p_call_back )
{
	LOG_DEBUG( "mini_http_read_data:%u!",p_data_pipe->_id);
	return SUCCESS;
}

/* 所有数据下载完毕,向界面回调通知*/
_int32 mini_http_notify_dispatch_data_finish( struct tagDATA_PIPE *p_data_pipe)
{
	_int32 ret_val = SUCCESS;
	MINI_HTTP * p_mini_http =NULL;
	HTTP_CALL_BACK_FUNCTION callback_fun ;
	HTTP_CALL_BACK http_call_back_param;
	
	if(gp_mini_http_slab==NULL)
	{
		return -1;
	}
	p_mini_http =mini_http_get_from_map(p_data_pipe->_id);
	if(p_mini_http == NULL) return -1;
	LOG_DEBUG( "mini_http_notify_dispatch_data_finish:%u,notified=%d!",p_data_pipe->_id,p_mini_http->_notified);

	callback_fun = (HTTP_CALL_BACK_FUNCTION)p_mini_http->_http_param._callback_fun;

	if(p_mini_http->_notified) return SUCCESS; //已经通知过了，不再通知

	p_mini_http->_state =MINI_HTTP_SUCCESS;
	p_mini_http->_notified = TRUE;
	sd_memset(&http_call_back_param,0x00,sizeof(HTTP_CALL_BACK));
	http_call_back_param._http_id = p_data_pipe->_id;
	http_call_back_param._user_data= p_mini_http->_http_param._user_data;
	http_call_back_param._type = EHC_NOTIFY_FINISHED;
	http_call_back_param._result = 0;
	
	callback_fun(&http_call_back_param);
	
	return ret_val;

}

_int32 mini_http_get_file_type( struct tagDATA_PIPE* p_data_pipe)
{
	LOG_DEBUG( "mini_http_get_file_type:%u!",p_data_pipe->_id);
	/* 未知类型 */
	return 0;
}
/* 从http响应头中提取界面关心的几个信息:status_code,content_length,last_modified,cookie,content_encoding */
HTTP_MINI_HEADER *mini_http_get_header(HTTP_RESPN_HEADER * p_header)
{
	static HTTP_MINI_HEADER mini_http_header;
	HTTP_RESPN_HEADER * p_http_respn_header =p_header;
	
      LOG_DEBUG("mini_get_http_header"); 
	sd_memset(&mini_http_header,0x00,sizeof(HTTP_MINI_HEADER));
	mini_http_header._status_code = p_http_respn_header->_status_code;
	if(p_http_respn_header->_has_content_length)
	{
		mini_http_header._content_length= p_http_respn_header->_content_length;
	}
	else
	if(p_http_respn_header->_has_entity_length)
	{
		mini_http_header._content_length= p_http_respn_header->_entity_length;
	}

	if(p_http_respn_header->_last_modified_len!=0)
	{
		sd_strncpy(mini_http_header._last_modified,p_http_respn_header->_last_modified,(63<p_http_respn_header->_last_modified_len)?63:p_http_respn_header->_last_modified_len);
	}
	if(p_http_respn_header->_cookie_len!=0)
	{
		sd_strncpy(mini_http_header._cookie,p_http_respn_header->_cookie,(1023<p_http_respn_header->_cookie_len)?1023:p_http_respn_header->_cookie_len);
	}
	
	if(p_http_respn_header->_content_encoding_len!=0)
	{
		sd_strncpy(mini_http_header._content_encoding,p_http_respn_header->_content_encoding,(31<p_http_respn_header->_content_encoding_len)?31:p_http_respn_header->_content_encoding_len);
	}
	return &mini_http_header;
}

_int32 mini_http_set_header( struct tagDATA_PIPE *p_data_pipe, HTTP_RESPN_HEADER * p_header,_int32 status_code)
{
	_int32 ret_val = SUCCESS;
	MINI_HTTP * p_mini_http = NULL;
	HTTP_CALL_BACK_FUNCTION callback_fun ;
	HTTP_CALL_BACK http_call_back_param;
	HTTP_MINI_HEADER * p_mini_header=NULL;
	
	LOG_DEBUG( "mini_http_set_header:%u,status_code=%u!",p_data_pipe->_id,status_code);
	if(gp_mini_http_slab==NULL)
	{
		return -1;
	}
	p_mini_http =mini_http_get_from_map(p_data_pipe->_id);
	if(p_mini_http == NULL) return -1;

	callback_fun = (HTTP_CALL_BACK_FUNCTION)p_mini_http->_http_param._callback_fun;

	p_mini_header = mini_http_get_header( p_header);
	if(status_code!=200&&status_code!=206)
	{
		/* 出错 */
		p_mini_header->_status_code = status_code;
	}
	
	/* 向界面回调通知服务器响应头信息 */
	sd_memset(&http_call_back_param,0x00,sizeof(HTTP_CALL_BACK));
	http_call_back_param._http_id = p_data_pipe->_id;
	http_call_back_param._user_data= p_mini_http->_http_param._user_data;
	http_call_back_param._type = EHC_NOTIFY_RESPN;
	http_call_back_param._header = (void*)p_mini_header ;
	
	ret_val = callback_fun(&http_call_back_param);
	return ret_val;
}

_int32 mini_http_handle_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid)
{
	if(errcode == MSG_CANCELLED)
		return SUCCESS;
	
	mini_http_scheduler();

	/* 已无任务，停止调度 */
	if(map_size(&g_mini_https)==0)
	{
		if(g_timeout_id != INVALID_MSG_ID)
		{
			cancel_timer(g_timeout_id);
			g_timeout_id = INVALID_MSG_ID;
		}
	}
	return SUCCESS;
}
/*
_int32 mini_http_build_get_request(char** buffer, _u32* len, MINI_HTTP * p_mini_http)
{
	_int32 ret_val = SUCCESS;

	URL_OBJECT *p_url_o=NULL;
	LOG_DEBUG( "mini_http_build_get_request");

	p_url_o = http_pipe_get_url_object(p_mini_http->_http_pipe);

	ret_val = http_create_request(buffer,len,p_url_o->_full_path,http_resource_get_ref_url((HTTP_SERVER_RESOURCE *)p_mini_http->_res),
		p_url_o->_host,p_url_o->_port,p_url_o->_user,p_url_o->_password,http_resource_get_cookies((HTTP_SERVER_RESOURCE *)p_mini_http->_res),-1,0,TRUE);
	CHECK_VALUE(ret_val);

	return SUCCESS;
}
_int32 mini_http_build_post_request(char** buffer, _u32* len, MINI_HTTP * p_mini_http)
{
	_int32 ret_val = SUCCESS;
	//URL_OBJECT *p_url_o=NULL;
	char url[MAX_URL_LEN] ;
	char request_header[MAX_URL_LEN];
	_int32 header_len = 0,total_len = 0;
	LOG_DEBUG( "mini_http_build_post_request");

	//p_url_o = http_pipe_get_url_object(p_mini_http->_http_pipe);
	sd_memset(url,0x00,MAX_URL_LEN);
	sd_memset(request_header,0x00,MAX_URL_LEN);
	sd_strncpy(url, p_mini_http->_http_param._url, p_mini_http->_http_param._url_len);

	header_len = sd_snprintf(request_header, MAX_URL_LEN, "POST %s HTTP/1.1\r\nContent-Length: %u\r\nContent-Type: application/octet-stream\r\nConnection: Close\r\n\r\n",
				url, p_mini_http->_http_param._data_len);

	total_len = header_len+p_mini_http->_http_param._data_len;

	ret_val = sd_malloc(total_len, (void**)buffer);
	CHECK_VALUE(ret_val);

	*len = total_len;
	sd_memcpy(*buffer,request_header,header_len);
	sd_memcpy(*buffer+header_len,p_mini_http->_http_param._data,p_mini_http->_http_param._data_len);

	return SUCCESS;
}
*/
/* 准备发送数据*/
_int32 mini_http_get_post_data(struct tagDATA_PIPE* p_data_pipe,_u8 ** data,_u32 *data_len)
{
	_int32 ret_val = SUCCESS;
	MINI_HTTP * p_mini_http = NULL;
	HTTP_CALL_BACK_FUNCTION callback_fun ;
	HTTP_CALL_BACK http_call_back_param;
	
	LOG_DEBUG( "mini_http_get_post_data:%u!",p_data_pipe->_id);
	if(gp_mini_http_slab==NULL)
	{
		return -1;
	}
	p_mini_http =mini_http_get_from_map(p_data_pipe->_id);
	if(p_mini_http == NULL) return -1;

	callback_fun = (HTTP_CALL_BACK_FUNCTION)p_mini_http->_http_param._callback_fun;

	if(p_mini_http->_http_param._content_len == 0)
	{
		/* 没有数据要发送*/
		*data =NULL;
		*data_len = 0;
		return SUCCESS;
	}

	if((p_mini_http->_sentsize==0)&&(p_mini_http->_http_param._send_data_len!=0))
	{
		/* 第一块需要发送的数据可以在创建任务的时候传进来*/
		*data =p_mini_http->_http_param._send_data;
		*data_len = p_mini_http->_http_param._send_data_len;
		p_mini_http->_current_send_data = *data;
		p_mini_http->_send_data_need_notified = TRUE;
		return SUCCESS;
	}

	/* 向界面回调通知获取需要发送的数据 */
	sd_memset(&http_call_back_param,0x00,sizeof(HTTP_CALL_BACK));
	http_call_back_param._http_id = p_data_pipe->_id;
	http_call_back_param._user_data= p_mini_http->_http_param._user_data;
	http_call_back_param._type = EHC_GET_SEND_DATA;
	http_call_back_param._send_data = data;
	http_call_back_param._send_data_len= data_len;
	
	ret_val = callback_fun(&http_call_back_param);
	/* 记录数据地址,用于发送完毕后传回界面释放这块内存 */
	if(ret_val==SUCCESS)
	{
	p_mini_http->_current_send_data = *data;
	p_mini_http->_send_data_need_notified = TRUE;
	}
	else
	{
		*data = NULL;
		*data_len = 0;
		p_mini_http->_current_send_data = NULL;
		p_mini_http->_send_data_need_notified = FALSE;
	}
	return ret_val;
}
/* 已经发送完当前数据块，向界面回调通知，并把数据地址和已发大小传给界面 */
_int32 mini_http_notify_sent_data(struct tagDATA_PIPE* p_data_pipe,_u32 sent_data_len,BOOL * send_end)
{
	_int32 ret_val = SUCCESS;
	MINI_HTTP * p_mini_http = NULL;
	HTTP_CALL_BACK_FUNCTION callback_fun ;
	HTTP_CALL_BACK http_call_back_param;
	
	LOG_DEBUG( "mini_http_notify_sent_data:%u!",p_data_pipe->_id);
	if(gp_mini_http_slab==NULL)
	{
		return -1;
	}
	p_mini_http =mini_http_get_from_map(p_data_pipe->_id);
	//sd_assert(p_mini_http != NULL);
	if(p_mini_http == NULL) return -1;

	callback_fun = (HTTP_CALL_BACK_FUNCTION)p_mini_http->_http_param._callback_fun;

	if(!p_mini_http->_send_data_need_notified) return SUCCESS;
	
	sd_assert(p_mini_http->_current_send_data!=NULL);
	
	if(p_mini_http->_sentsize==0)
	{
		/* 为防止由于网络反应过快在mini_add_task_to_map 之前就返回而导致找不到对应的MINI_TASK,所以第一次发完数据故意在此延时1ms  */
		sd_sleep(1);

		if(p_mini_http->_http_param._send_data_len!=0)
		{
			if(sent_data_len>p_mini_http->_http_param._send_data_len)
			{
				/* 第一块数据发送成功,去掉请求头部分的字节数 */
				sent_data_len = p_mini_http->_http_param._send_data_len;
			}
		}
	}

	p_mini_http->_sentsize+=sent_data_len;
	if(p_mini_http->_sentsize<p_mini_http->_http_param._content_len)
	{
		*send_end = FALSE;
	}
	else
	{
		*send_end = TRUE;
	}

	sd_memset(&http_call_back_param,0x00,sizeof(HTTP_CALL_BACK));
	http_call_back_param._http_id = p_data_pipe->_id;
	http_call_back_param._user_data= p_mini_http->_http_param._user_data;
	http_call_back_param._type = EHC_NOTIFY_SENT_DATA;
	http_call_back_param._sent_data = p_mini_http->_current_send_data;
	http_call_back_param._sent_data_len= sent_data_len;
	
	ret_val = callback_fun(&http_call_back_param);
	p_mini_http->_current_send_data = NULL;
	p_mini_http->_send_data_need_notified = FALSE;
	return ret_val;
}

_int32 mini_http_malloc(MINI_HTTP **pp_slip)
{
	_int32 ret_val = mpool_get_slip( gp_mini_http_slab, (void**)pp_slip );
	
	if(ret_val==SUCCESS) 
		sd_memset(*pp_slip,0,sizeof(MINI_HTTP));
	
      LOG_DEBUG("mini_http_malloc"); 
	return ret_val;
}

_int32 mini_http_free(MINI_HTTP * p_slip)
{
	sd_assert( p_slip != NULL );
	if( p_slip == NULL ) return SUCCESS;
	return mpool_free_slip( gp_mini_http_slab, (void*)p_slip );
}
//////////////////////////////////
_int32 mini_http_reset_id(void)
{
      LOG_DEBUG("mini_http_reset_id"); 

	g_mini_http_num=0;

	return SUCCESS;
}


_u32 mini_http_create_id(void)
{
      LOG_DEBUG("mini_http_create_id:%u",g_mini_http_num+1); 

	g_mini_http_num++;

	if(g_mini_http_num>=MAX_U32)
		g_mini_http_num=1;
	
	return g_mini_http_num;
}

_int32 mini_http_decrease_id(void)
{
      LOG_DEBUG("mini_http_decrease_id:%u",g_mini_http_num); 

	g_mini_http_num--;

	return SUCCESS;
}

MINI_HTTP * mini_http_get_from_map(_u32 id)
{
	_int32 ret_val = SUCCESS;
	MINI_HTTP * p_mini_http=NULL;

      LOG_DEBUG("mini_http_get_from_map:id=%u",id); 
	ret_val=map_find_node(&g_mini_https, (void *)id, (void **)&p_mini_http);
	sd_assert(ret_val==SUCCESS);
	//sd_assert(p_task!=NULL);
	return p_mini_http;
}
_int32 mini_http_add_to_map(MINI_HTTP * p_mini_http)
{
	_int32 ret_val = SUCCESS;
	PAIR info_map_node;
      LOG_DEBUG("mini_http_add_to_map:id=%u",p_mini_http->_id); 
	info_map_node._key =(void*)p_mini_http->_id;
	info_map_node._value = (void*)p_mini_http;
	ret_val = map_insert_node( &g_mini_https, &info_map_node );
	//CHECK_VALUE(ret_val);
	if(g_timeout_id == INVALID_MSG_ID)
	{
		start_timer(mini_http_handle_timeout, NOTICE_INFINITE, MINI_HTTP_TIMEOUT_INSTANCE, 0, NULL, &g_timeout_id);
	}
	return ret_val;
}

_int32 mini_http_remove_from_map(MINI_HTTP * p_mini_http)
{
	_int32 ret_val = SUCCESS;
      LOG_DEBUG("mini_http_remove_from_map:id=%u",p_mini_http->_id); 
	ret_val = map_erase_node(&g_mini_https, (void*)p_mini_http->_id);
	if(map_size(&g_mini_https)==0)
	{
		if(g_timeout_id != INVALID_MSG_ID)
		{
			cancel_timer(g_timeout_id);
			g_timeout_id = INVALID_MSG_ID;
		}
	}
	return ret_val;
}
_int32 mini_http_clear_map(void)
{
      MAP_ITERATOR  cur_node = NULL; 
	MINI_HTTP * p_mini_http = NULL;

      LOG_ERROR("mini_http_clear_map:%u",map_size(&g_mini_https)); 
	  
      cur_node = MAP_BEGIN(g_mini_https);
      while(cur_node != MAP_END(g_mini_https))
      {
             p_mini_http = (MINI_HTTP *)MAP_VALUE(cur_node);
		//dt_uninit_task_info(p_task->_task_info);
		//sd_assert(p_task->_change_flag==0);
	      mini_http_close(p_mini_http,MINI_HTTP_CLOSE_FORCE);
	      mini_http_free(p_mini_http);
		map_erase_iterator(&g_mini_https, cur_node);
	      cur_node = MAP_BEGIN(g_mini_https);
      }
	return SUCCESS;
}

_int32 mini_http_cancel(MINI_HTTP * p_mini_http)
{
	_int32 err_code = SUCCESS,pipe_err_code = SUCCESS;
	//_int32 ret_val = SUCCESS;
	//enum HTTP_PIPE_STATE http_state;
	HTTP_CALL_BACK_FUNCTION callback_fun;
	HTTP_CALL_BACK http_call_back_param;
	
	if(p_mini_http == NULL) return -1;
	
      LOG_ERROR("mini_http_cancel:id=%u,_notified=%d,http_pipe=0x%X,res=0x%X,send_data_need_notified=%d,current_send_data=0x%X,recv_data_need_notified=%d,current_send_data=0x%X",p_mini_http->_id,p_mini_http->_notified,p_mini_http->_http_pipe,p_mini_http->_res,
	  	p_mini_http->_send_data_need_notified,p_mini_http->_current_send_data,p_mini_http->_recv_data_need_notified,p_mini_http->_current_send_data); 
	LOG_ERROR("url--%s",p_mini_http->_http_param._url);
	/* Get the call back function ! */
	callback_fun = (HTTP_CALL_BACK_FUNCTION)p_mini_http->_http_param._callback_fun;

	/* Get the error code ! */
	if((p_mini_http->_res!=NULL)&&(p_mini_http->_res->_err_code!=SUCCESS))
	{
		pipe_err_code = p_mini_http->_res->_err_code;
	}
	else
	if((p_mini_http->_http_pipe!=NULL)&&(p_mini_http->_http_pipe->_error_code!=SUCCESS))
	{
		pipe_err_code = p_mini_http->_http_pipe->_error_code;
	}

	/* Close the http_pipe if need ! */
	if(p_mini_http->_http_pipe!=NULL)
	{
		http_pipe_close((DATA_PIPE*)p_mini_http->_http_pipe);
		p_mini_http->_http_pipe = NULL;
	}
	
	/* Destroy the http_resource if need ! */
	if(p_mini_http->_res)
	{
		http_resource_destroy(&p_mini_http->_res);
		p_mini_http->_res = NULL;
	}
	
	/* Notify to release send_data buffer if need ! */
	if(p_mini_http->_send_data_need_notified)
	{
		LOG_DEBUG("func %s,--1",__FUNCTION__);
		sd_memset(&http_call_back_param,0x00,sizeof(HTTP_CALL_BACK));
		http_call_back_param._http_id = p_mini_http->_id;
		http_call_back_param._user_data= p_mini_http->_http_param._user_data;
		http_call_back_param._type = EHC_NOTIFY_SENT_DATA;
		if(p_mini_http->_current_send_data!=NULL)
		{
			http_call_back_param._sent_data= p_mini_http->_current_send_data;
		}
		else
		{
			http_call_back_param._sent_data= p_mini_http->_http_param._send_data;
		}
		http_call_back_param._sent_data_len = 0;
		
		callback_fun(&http_call_back_param);
		p_mini_http->_current_send_data = NULL;
		p_mini_http->_send_data_need_notified = FALSE;
	}

	/* Notify to release recv_buffer if need ! */
	if(p_mini_http->_recv_data_need_notified)
	{
		LOG_DEBUG("func %s,--2",__FUNCTION__);
		sd_memset(&http_call_back_param,0x00,sizeof(HTTP_CALL_BACK));
		http_call_back_param._http_id = p_mini_http->_id;
		http_call_back_param._user_data= p_mini_http->_http_param._user_data;
		http_call_back_param._type = EHC_PUT_RECVED_DATA;
		if(p_mini_http->_current_recv_data!=NULL)
		{
			http_call_back_param._recved_data= p_mini_http->_current_recv_data;
		}
		else
		{
			http_call_back_param._recved_data= p_mini_http->_http_param._recv_buffer;
		}
		http_call_back_param._recved_data_len = 0;
		
		callback_fun(&http_call_back_param);
		p_mini_http->_current_recv_data = NULL;
		p_mini_http->_recv_data_need_notified = FALSE;
	}

	LOG_DEBUG("func %s,--3,notify-%d",__FUNCTION__,p_mini_http->_notified);
	/* Notify EHC_NOTIFY_FINISHED if need ! */
	//if(p_mini_http->_notified != TRUE)
	{
		LOG_DEBUG("func %s,--4",__FUNCTION__);
		/* 会话已结束，向界面回调通知，并把执行结果传给界面 */
		err_code = (p_mini_http->_state ==MINI_HTTP_SUCCESS)?SUCCESS:-1;
		if(err_code!=SUCCESS)
		{
			if(pipe_err_code!=SUCCESS)
			{
				err_code = pipe_err_code;
			}
		}
		LOG_DEBUG( "mini_http_cancel,HRT_FINISHED,resp_callback:%u,result=%d!",p_mini_http->_id,err_code);
		p_mini_http->_notified = TRUE;

		sd_memset(&http_call_back_param,0x00,sizeof(HTTP_CALL_BACK));
		http_call_back_param._http_id = p_mini_http->_id;
		http_call_back_param._user_data= NULL;//p_mini_http->_http_param._user_data;
		http_call_back_param._type = EHC_NOTIFY_FINISHED;
		http_call_back_param._result =HTTP_CANCEL	;
		
		callback_fun(&http_call_back_param);
	
	}

	/* 取消 global_pipes */
	if(p_mini_http->_http_param._priority>=0)
	{
		/* -1:低优先级,不会暂停下载任务; 0:普通优先级,会暂停下载任务;1:高优先级,会暂停所有其他的网络连接  */
		tm_resume_global_pipes_after_close_mini_http( );
	}
	return SUCCESS;

}


_int32 mini_http_close(MINI_HTTP * p_mini_http,_u32 close_code)
{
	_int32 err_code = SUCCESS,pipe_err_code = SUCCESS;
	//_int32 ret_val = SUCCESS;
	//enum HTTP_PIPE_STATE http_state;
	HTTP_CALL_BACK_FUNCTION callback_fun;
	HTTP_CALL_BACK http_call_back_param;
	
	if(p_mini_http == NULL) return -1;
	
      LOG_ERROR("mini_http_close:id=%u,_notified=%d,http_pipe=0x%X,res=0x%X,send_data_need_notified=%d,current_send_data=0x%X,recv_data_need_notified=%d,current_send_data=0x%X",p_mini_http->_id,p_mini_http->_notified,p_mini_http->_http_pipe,p_mini_http->_res,
	  	p_mini_http->_send_data_need_notified,p_mini_http->_current_send_data,p_mini_http->_recv_data_need_notified,p_mini_http->_current_send_data); 
	LOG_ERROR("url--%s",p_mini_http->_http_param._url);
	/* Get the call back function ! */
	callback_fun = (HTTP_CALL_BACK_FUNCTION)p_mini_http->_http_param._callback_fun;

	/* Get the error code ! */
	if((p_mini_http->_res!=NULL)&&(p_mini_http->_res->_err_code!=SUCCESS))
	{
		pipe_err_code = p_mini_http->_res->_err_code;
	}
	else
	if((p_mini_http->_http_pipe!=NULL)&&(p_mini_http->_http_pipe->_error_code!=SUCCESS))
	{
		pipe_err_code = p_mini_http->_http_pipe->_error_code;
	}

	/* Close the http_pipe if need ! */
	if(p_mini_http->_http_pipe!=NULL)
	{
		http_pipe_close((DATA_PIPE*)p_mini_http->_http_pipe);
		p_mini_http->_http_pipe = NULL;
	}
	
	/* Destroy the http_resource if need ! */
	if(p_mini_http->_res)
	{
		http_resource_destroy(&p_mini_http->_res);
		p_mini_http->_res = NULL;
	}
	
	/* Notify to release send_data buffer if need ! */
	if(p_mini_http->_send_data_need_notified)
	{
		sd_memset(&http_call_back_param,0x00,sizeof(HTTP_CALL_BACK));
		http_call_back_param._http_id = p_mini_http->_id;
		http_call_back_param._user_data= p_mini_http->_http_param._user_data;
		http_call_back_param._type = EHC_NOTIFY_SENT_DATA;
		if(p_mini_http->_current_send_data!=NULL)
		{
			http_call_back_param._sent_data= p_mini_http->_current_send_data;
		}
		else
		{
			http_call_back_param._sent_data= p_mini_http->_http_param._send_data;
		}
		http_call_back_param._sent_data_len = 0;
		
		callback_fun(&http_call_back_param);
		p_mini_http->_current_send_data = NULL;
		p_mini_http->_send_data_need_notified = FALSE;
	}

	/* Notify to release recv_buffer if need ! */
	if(p_mini_http->_recv_data_need_notified)
	{
		sd_memset(&http_call_back_param,0x00,sizeof(HTTP_CALL_BACK));
		http_call_back_param._http_id = p_mini_http->_id;
		http_call_back_param._user_data= p_mini_http->_http_param._user_data;
		http_call_back_param._type = EHC_PUT_RECVED_DATA;
		if(p_mini_http->_current_recv_data!=NULL)
		{
			http_call_back_param._recved_data= p_mini_http->_current_recv_data;
		}
		else
		{
			http_call_back_param._recved_data= p_mini_http->_http_param._recv_buffer;
		}
		http_call_back_param._recved_data_len = 0;
		
		callback_fun(&http_call_back_param);
		p_mini_http->_current_recv_data = NULL;
		p_mini_http->_recv_data_need_notified = FALSE;
	}
	
	/* Notify EHC_NOTIFY_FINISHED if need ! */
	if(p_mini_http->_notified != TRUE)
	{
		/* 会话已结束，向界面回调通知，并把执行结果传给界面 */
		err_code = (p_mini_http->_state ==MINI_HTTP_SUCCESS)?SUCCESS:-1;
		if (MINI_HTTP_CLOSE_TIMEOUT==close_code)
		{
			err_code=MINI_HTTP_CLOSE_TIMEOUT;
		}
		if(err_code!=SUCCESS)
		{
			if(pipe_err_code!=SUCCESS)
			{
				err_code = pipe_err_code;
			}
		}
		LOG_DEBUG( "mini_http_close,HRT_FINISHED,resp_callback:%u,result=%d!",p_mini_http->_id,err_code);
		p_mini_http->_notified = TRUE;

		sd_memset(&http_call_back_param,0x00,sizeof(HTTP_CALL_BACK));
		http_call_back_param._http_id = p_mini_http->_id;
		http_call_back_param._user_data= p_mini_http->_http_param._user_data;
		http_call_back_param._type = EHC_NOTIFY_FINISHED;
		http_call_back_param._result = err_code;
		
		callback_fun(&http_call_back_param);
	
	}

	/* 取消 global_pipes */
	if(p_mini_http->_http_param._priority>=0)
	{
		/* -1:低优先级,不会暂停下载任务; 0:普通优先级,会暂停下载任务;1:高优先级,会暂停所有其他的网络连接  */
		tm_resume_global_pipes_after_close_mini_http( );
	}
	return SUCCESS;
}


_int32 mini_http_scheduler(void)
{
      MAP_ITERATOR  cur_node = NULL,nxt_node = NULL; 
	MINI_HTTP * p_mini_http = NULL;
	 _u32 cur_time = 0;

      LOG_DEBUG("mini_http_scheduler:%u",map_size(&g_mini_https)); 
	  
	sd_time_ms(&cur_time);
      cur_node = MAP_BEGIN(g_mini_https);
      while(cur_node != MAP_END(g_mini_https))
      {
             p_mini_http = (MINI_HTTP *)MAP_VALUE(cur_node);
		if(((cur_time<p_mini_http->_start_time)||TIME_SUBZ(cur_time, p_mini_http->_start_time)>=p_mini_http->_http_param._timeout*1000)
			&&(p_mini_http->_http_pipe->_http_state<HTTP_PIPE_STATE_CONNECTED)  /* 应建业的要求,mini http的timeout只用于连接的时候,一旦开始下载数据,则pipe不超时! zyq @20110913 */
			)
		{
			/* pipe 超时,关闭并彻底销毁 */
			if(!p_mini_http->_wait_delete)
			{
	      			LOG_ERROR("mini_http_close:%u:timeout = %u,pipe_state=%d,http_state=%d!",p_mini_http->_id,p_mini_http->_http_param._timeout,p_mini_http->_http_pipe->_data_pipe._dispatch_info._pipe_state,p_mini_http->_http_pipe->_http_state); 
				if(((p_mini_http->_http_pipe->_http_state == HTTP_PIPE_STATE_REQUESTING)&&(p_mini_http->_current_send_data!=NULL))
					||((p_mini_http->_http_pipe->_http_state == HTTP_PIPE_STATE_READING)&&(p_mini_http->_current_recv_data!=NULL)))
				{
      					LOG_DEBUG("http_pipe_close to release momey!"); 
					http_pipe_close((DATA_PIPE*)p_mini_http->_http_pipe);
					p_mini_http->_http_pipe = NULL;
					p_mini_http->_wait_delete = TRUE;
		      			cur_node = MAP_NEXT(g_mini_https, cur_node) ;
				}
				else
				{
      					LOG_DEBUG("mini_http_close do not need to release momey!"); 
					sd_assert(p_mini_http->_current_send_data==NULL);
					//sd_assert(p_mini_http->_current_recv_data==NULL);
					p_mini_http->_state =MINI_HTTP_FAILED;
			      		mini_http_close(p_mini_http,MINI_HTTP_CLOSE_TIMEOUT);
				       mini_http_free(p_mini_http);
					nxt_node = MAP_NEXT(g_mini_https, cur_node) ;
					map_erase_iterator(&g_mini_https, cur_node);
					cur_node = nxt_node;
				}
			}
			else
			{
	      			LOG_DEBUG("mini http wait_delete:%u:timeout = %u,current_send_data=0x%X,current_recv_data=0x%X!",p_mini_http->_id,p_mini_http->_http_param._timeout,p_mini_http->_current_send_data,p_mini_http->_current_recv_data); 
				if((p_mini_http->_current_send_data!=NULL)||(p_mini_http->_current_recv_data!=NULL))
				{
      					LOG_DEBUG("continue wait to release momey!"); 
		      			cur_node = MAP_NEXT(g_mini_https, cur_node) ;
				}
				else
				{
      					LOG_DEBUG("mini_http_close because momey already released!"); 
					p_mini_http->_state =MINI_HTTP_FAILED;
			      		mini_http_close(p_mini_http,MINI_HTTP_CLOSE_TIMEOUT);
				       mini_http_free(p_mini_http);
					nxt_node = MAP_NEXT(g_mini_https, cur_node) ;
					map_erase_iterator(&g_mini_https, cur_node);
					cur_node = nxt_node;
				}
			}
			
		}
		else
		if((p_mini_http->_state ==MINI_HTTP_RUNNING)&&(p_mini_http->_http_pipe)&&(p_mini_http->_http_pipe->_data_pipe._dispatch_info._pipe_state==PIPE_FAILURE))
		{
			p_mini_http->_state =MINI_HTTP_FAILED;
			/* pipe 内部出错,关闭并彻底销毁 */
      			LOG_DEBUG("mini_http_close:pipe_state=%d=%d!",p_mini_http->_http_pipe->_data_pipe._dispatch_info._pipe_state,PIPE_FAILURE); 
			//sd_assert(p_mini_http->_current_send_data==NULL);
			//sd_assert(p_mini_http->_current_recv_data==NULL);
	      		mini_http_close(p_mini_http,MINI_HTTP_CLOSE_ERROR);
		      mini_http_free(p_mini_http);
			nxt_node = MAP_NEXT(g_mini_https, cur_node) ;
			map_erase_iterator(&g_mini_https, cur_node);
			cur_node = nxt_node;
		}
		else
		if(p_mini_http->_state !=MINI_HTTP_RUNNING)
		{
			/* pipe已完成,关闭并彻底销毁 */
			LOG_DEBUG("mini_http_close:state=%d!",p_mini_http->_state);
			sd_assert(p_mini_http->_current_send_data==NULL);
			sd_assert(p_mini_http->_current_recv_data==NULL);
	      		mini_http_close(p_mini_http,MINI_HTTP_CLOSE_NORMAL);
		      mini_http_free(p_mini_http);
			nxt_node = MAP_NEXT(g_mini_https, cur_node) ;
			map_erase_iterator(&g_mini_https, cur_node);
			cur_node = nxt_node;
		}
		else
		{
	      		cur_node = MAP_NEXT(g_mini_https, cur_node) ;
		}
      }
	return SUCCESS;
}

// end of file
