#include "utility/define.h"
#include "utility/string.h"
#include "utility/version.h"
#include "utility/sd_os.h"
#include "utility/mempool.h"
#include "utility/time.h"
#include "utility/url.h"
#include "utility/rw_sharebrd.h"
#include "utility/peerid.h"
#include "asyn_frame/asyn_frame.h"
#include "platform/sd_network.h"
#include "platform/sd_customed_interface.h"
#include "platform/sd_task.h"
#include "platform/sd_fs.h"
#include "platform/sd_mem.h"
#include "platform/sd_task.h"

#include "utility/logid.h"
#define LOGID LOGID_TASK_MANAGER
#include "utility/logger.h"

#include "embed_thunder.h"
#include "task_manager/task_manager.h"
#include "socket_proxy_interface.h"

#include "download_task/download_task.h"
#include "p2sp_download/p2sp_task.h"
#ifdef ENABLE_BT
#include "bt_download/bt_task/bt_task_interface.h"
#endif
#ifdef ENABLE_EMULE
#include "emule/emule_interface.h"
#endif
#include "http_server/http_server_interface.h"
#ifdef ENABLE_VOD
#include "vod_data_manager/vod_data_manager_interface.h"
#endif
#ifdef _CONNECT_DETAIL
#ifdef UPLOAD_ENABLE
#include "upload_manager/upload_manager.h"
#endif
#endif

#include "cmd_proxy/cmd_proxy_manager.h"

#ifdef ENABLE_DRM 
#include "drm/openssl_interface.h"
#include "drm/drm.h"
#endif

#include "high_speed_channel/hsc_interface.h"

#include "reporter/reporter_mobile.h"

#include "embed_thunder_version.h"


static BOOL g_already_init = FALSE;
static BOOL g_os_init_by_et = FALSE;
static char   g_log_conf_file_path[255] = {0};

/*--------------------------------------------------------------------------*/
/*                Interfaces provid for external program				        */
/*--------------------------------------------------------------------------*/


//返回值: 0  success, NOT_SUPPORT_PROXY_TYPE 不支持的代理类型
//如果连接hub不需要代理，proxy_for_hub设置成NULL或proxy_type为0
ETDLL_API _int32 et_init(ET_PROXY_INFO* proxy_for_hub)
{     
	if (g_already_init)
	{
#ifdef ENABLE_NULL_PATH
        return ALREADY_ET_INIT;
#else
        CHECK_VALUE( ALREADY_ET_INIT);
#endif
    }

	sd_assert(sizeof(ET_TASK)==sizeof(ET_TASK_INFO));
	
#ifdef LINUX
    /* test cpu frequence */
    test_cpu_frq();
#endif

	g_os_init_by_et = FALSE;
	_int32 ret_val = SUCCESS;
	if (! et_os_is_initialized())
	{
	    if (g_log_conf_file_path[0] == '\0')
	    {
	        sd_strncpy(g_log_conf_file_path, LOG_CONFIG_FILE, sizeof(g_log_conf_file_path)-1);
	    }
#if (defined(MACOS)&&defined(MOBILE_PHONE))
        ret_val = et_os_init(g_log_conf_file_path, NULL, 0);
#else
        ret_val = et_os_init(g_log_conf_file_path);
#endif

        CHECK_VALUE(ret_val);
        g_os_init_by_et = TRUE;
	}

	set_critical_error(SUCCESS);

	ret_val = start_asyn_frame(tm_init, (void *) proxy_for_hub, tm_uninit, NULL);
	sd_assert(ret_val==SUCCESS);
	if ((ret_val == SUCCESS) || (ret_val == ALREADY_ET_INIT))
	{
		g_already_init = TRUE;
	}
	else if (g_os_init_by_et)
	{
		et_os_uninit();
	}


	return ret_val;
}

#ifdef UPLOAD_ENABLE
/* Close all the pipes in upload_manager */
int32 et_close_upload_pipes(void)
{
	LOG_DEBUG("et_close_upload_pipes");
	TM_CLOSE_PIPES _param;
	sd_memset(&_param, 0, sizeof(TM_CLOSE_PIPES));
	return tm_post_function(tm_close_upload_pipes, (void *)&_param, &(_param._handle), &(_param._result));
}
#endif

/* Send logout to ping server */
int32 et_logout(void)
{
    LOG_DEBUG("et_logout");
    TM_LOGOUT _param;
    sd_memset(&_param, 0, sizeof(TM_LOGOUT));
    return tm_post_function(tm_logout, (void *)&_param, &(_param._handle), &(_param._result));
}

int32 et_http_clear(void)
{
    LOG_DEBUG("et_http_clear");
    TM_LOGOUT _param;
    sd_memset(&_param, 0, sizeof(TM_LOGOUT));
    return tm_post_function(tm_http_clear, (void *)&_param, &(_param._handle), &(_param._result));
}

void et_clear_tasks(void)
{
    _u32 id_buffer_len = MAX_ET_ALOW_TASKS;
    _u32 task_id_buffer[MAX_ET_ALOW_TASKS];
    _int32 ret_val = et_get_all_task_id(&id_buffer_len, task_id_buffer);
	sd_assert(ret_val != TM_ERR_BUFFER_NOT_ENOUGH);	
	if (ret_val==SUCCESS)
	{
		while (id_buffer_len--)
		{
			if (task_id_buffer[id_buffer_len] != 0)
			{
				et_stop_task( task_id_buffer[id_buffer_len]);
				et_delete_task( task_id_buffer[id_buffer_len]);
			}
		}
	}
	
#ifdef UPLOAD_ENABLE
	et_close_upload_pipes();	
#endif
	et_logout();
	et_http_clear();
	
#if defined(ENABLE_HTTP_VOD)&&(!defined(ENABLE_ASSISTANT))
	 et_stop_http_server();
#endif /* ENABLE_HTTP_VOD */
}

/* Check the status of et */
ETDLL_API _int32 et_check_critical_error(void)
{
	_int32 ret_val = SUCCESS;
	
	if(g_already_init)
	{
		if(!sd_is_net_ok()) return NETWORK_NOT_READY;
		
		ret_val=get_critical_error();
		//CHECK_CRITICAL_ERROR;
	}
	return ret_val;
}

/* Check if et is running */
ETDLL_API BOOL et_check_running(void)
{
    return (g_already_init && (is_asyn_frame_running()));
}

/* Uninitalize the task_manager */
ETDLL_API _int32 et_uninit(void)
{	
	if ((!g_already_init)&&(!is_asyn_frame_running()))
		return -1;
	
	LOG_DEBUG("uninit 1");
	et_clear_tasks();
	LOG_DEBUG("uninit 2");
	_int32 ret_val = stop_asyn_frame();
	LOG_DEBUG("uninit 3");
	tm_force_close_resource();
	LOG_DEBUG("uninit 4");

	if(g_os_init_by_et)
	{
		et_os_uninit();
		g_os_init_by_et = FALSE;
	}

	set_critical_error(SUCCESS);
	
	g_already_init = FALSE;
	return ret_val;
}

/* set_license */
int32 	et_set_license(char *license, int32 license_size)
{
	TM_SET_LICENSE _param;
#ifdef VOD_DEBUG
        printf("et_set_license    1 \n"); 
#endif

	if(!g_already_init)
		return -1;
	
#ifdef VOD_DEBUG
        printf("et_set_license    1 \n"); 
#endif
	
	LOG_DEBUG("set_license,license=%s",license);

	CHECK_CRITICAL_ERROR;

#ifdef VOD_DEBUG
        printf("et_set_license    3 \n"); 
#endif

	if((license==NULL)||(license_size<1)||(sd_strlen(license)!=license_size)) return TM_ERR_INVALID_PARAMETER;
	sd_memset(&_param,0,sizeof(TM_SET_LICENSE));
	_param.license_size=license_size;
	_param.license=license;
	
	return tm_post_function(tm_set_license, (void *)&_param,&(_param._handle),&(_param._result));
}

/* set_license_callback */
int32 	et_set_license_callback( ET_NOTIFY_LICENSE_RESULT license_callback_ptr)
{
	TM_SET_LICENSE_CALLBACK _param;

	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("set_license_callback");

	CHECK_CRITICAL_ERROR;

	if(license_callback_ptr==NULL) return TM_ERR_INVALID_PARAMETER;
	sd_memset(&_param,0,sizeof(TM_SET_LICENSE_CALLBACK));
	_param.license_callback_ptr=(void*)license_callback_ptr;
	
	return tm_post_function(tm_set_license_callback, (void *)&_param,&(_param._handle),&(_param._result));
}

ETDLL_API _int32 et_set_file_name_changed_callback( void *callback_ptr)
{
	LOG_DEBUG("et_set_file_name_changed_callback %x",callback_ptr);

	CHECK_CRITICAL_ERROR;
    
	if(callback_ptr==NULL) return TM_ERR_INVALID_PARAMETER;
	
	return dm_set_file_name_changed_callback(callback_ptr);
}

ETDLL_API _int32 et_set_notify_event_callback( void *callback_ptr)
{
	TM_POST_PARA_1 _param;

	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_set_notify_event_callback");

	CHECK_CRITICAL_ERROR;

	if(callback_ptr==NULL) return TM_ERR_INVALID_PARAMETER;
	
	sd_memset(&_param,0,sizeof(TM_POST_PARA_1));
	
	_param._para1 =(void*)callback_ptr;
	
	return tm_post_function(tm_set_notify_event_callback, (void *)&_param,&(_param._handle),&(_param._result));
}

ETDLL_API _int32 et_set_notify_etm_scheduler( void *callback_ptr)
{
	TM_POST_PARA_1 _param;

	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_set_notify_etm_scheduler");

	CHECK_CRITICAL_ERROR;

	if(callback_ptr==NULL) return TM_ERR_INVALID_PARAMETER;
	
	sd_memset(&_param,0,sizeof(TM_POST_PARA_1));
	
	_param._para1 =(void*)callback_ptr;
	
	return tm_post_function(tm_set_etm_scheduler_event, (void *)&_param,&(_param._handle),&(_param._result));
}

ETDLL_API _int32 et_update_task_manager()
{
	TM_POST_PARA_0 _param;

	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_update_task_manager");

	CHECK_CRITICAL_ERROR;
	
	sd_memset(&_param,0,sizeof(TM_POST_PARA_0));

	
	return tm_post_function(tm_update_task_list, (void *)&_param,&(_param._handle),&(_param._result));
}



ETDLL_API _int32 et_create_new_task_by_url(char* url, u32 url_length, 
								 char* ref_url, u32 ref_url_length,
								 char* description, u32 description_len,
								 char* file_path, u32 file_path_len,
								 char* file_name, u32 file_name_length,
								 u32* task_id)
{
	TM_NEW_TASK_URL _param;
	_int32 url_len = url_length;
	_int32 ref_url_len = ref_url_length;
	char decode_url[MAX_URL_LEN] = {0};
	char ref_decode_url[MAX_URL_LEN] = {0};
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("create_new_task_by_url[%s]",url);

	CHECK_CRITICAL_ERROR;

	if((url==NULL)||(url_length>=MAX_URL_LEN)||(url_length==0)||(sd_strlen(url)==0)) return DT_ERR_INVALID_URL;
	
	if(ref_url_length>=MAX_URL_LEN) return TM_ERR_INVALID_PARAMETER;
	
	//if(description_len>=MAX_URL_LEN*2) return TM_ERR_INVALID_PARAMETER;
	
	if((file_path==NULL)||(file_path_len>=MAX_FILE_PATH_LEN)||(file_path_len==0)||(sd_strlen(file_path)==0)) return DT_ERR_INVALID_FILE_PATH;
	
	if((file_name_length>=MAX_FILE_NAME_LEN)||(sd_strlen(file_name)!=file_name_length)) return DT_ERR_INVALID_FILE_NAME;
	
	if(task_id==NULL) return TM_ERR_INVALID_PARAMETER;

	//url_len = url_object_decode( url, decode_url, MAX_URL_LEN );
	//if(url_len == -1) return DT_ERR_INVALID_URL;
	url_len = sd_strlen(url);

	if( ref_url != NULL && ref_url_length != 0 )
	{
		ref_url_len = url_object_decode( ref_url, ref_decode_url, MAX_URL_LEN );
	    if(ref_url_len== -1) return DT_ERR_INVALID_URL;	
		ref_url_len = sd_strlen(ref_decode_url);
	}
	
	sd_memset(&_param,0,sizeof(TM_NEW_TASK_URL));
	_param.url= url;
	_param.url_length= url_len;
	_param.ref_url= ref_decode_url;
	_param.ref_url_length= ref_url_len;
	_param.description= description;
	_param.description_len= description_len;
	_param.file_path= file_path;
	_param.file_path_len= file_path_len;
	_param.file_name= file_name;
	_param.file_name_length= file_name_length;
	_param.task_id= task_id;
	_param.origion_url = url;
	_param.origion_url_length = url_length;
	
	return tm_post_function(tm_create_new_task_by_url, (void *)&_param,&(_param._handle),&(_param._result));

}
ETDLL_API _int32 et_try_create_new_task_by_url(char* url, u32 url_length, 
								 char* ref_url, u32 ref_url_length,
								 char* description, u32 description_len,
								 char* file_path, u32 file_path_len,
								 char* file_name, u32 file_name_length,
								 u32* task_id)
{
	_int32 ret_val = SUCCESS;
#ifdef _ANDROID_LINUX
	TM_NEW_TASK_URL _param;
	_int32 url_len = url_length;
	_int32 ref_url_len = ref_url_length;
	char decode_url[MAX_URL_LEN] = {0};
	char ref_decode_url[MAX_URL_LEN] = {0};
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("create_new_task_by_url[%s]",url);

	CHECK_CRITICAL_ERROR;

	if((url==NULL)||(url_length>=MAX_URL_LEN)||(url_length==0)||(sd_strlen(url)==0)) return DT_ERR_INVALID_URL;
	
	if(ref_url_length>=MAX_URL_LEN) return TM_ERR_INVALID_PARAMETER;
	
	//if(description_len>=MAX_URL_LEN*2) return TM_ERR_INVALID_PARAMETER;
	
	if((file_path==NULL)||(file_path_len>=MAX_FILE_PATH_LEN)||(file_path_len==0)||(sd_strlen(file_path)==0)) return DT_ERR_INVALID_FILE_PATH;
	
	if((file_name_length>=MAX_FILE_NAME_LEN)||(sd_strlen(file_name)!=file_name_length)) return DT_ERR_INVALID_FILE_NAME;
	
	if(task_id==NULL) return TM_ERR_INVALID_PARAMETER;

	url_len = url_object_decode( url, decode_url, MAX_URL_LEN );
	if(url_len == -1) return DT_ERR_INVALID_URL;
	url_len = sd_strlen(decode_url);

	if( ref_url != NULL && ref_url_length != 0 )
	{
		ref_url_len = url_object_decode( ref_url, ref_decode_url, MAX_URL_LEN );
	    if(ref_url_len== -1) return DT_ERR_INVALID_URL;	
		ref_url_len = sd_strlen(ref_decode_url);
	}
	
	sd_memset(&_param,0,sizeof(TM_NEW_TASK_URL));
	_param.url= decode_url;
	_param.url_length= url_len;
	_param.ref_url= ref_decode_url;
	_param.ref_url_length= ref_url_len;
	_param.description= description;
	_param.description_len= description_len;
	_param.file_path= file_path;
	_param.file_path_len= file_path_len;
	_param.file_name= file_name;
	_param.file_name_length= file_name_length;
	_param.task_id= task_id;
	
	ret_val =  tm_try_post_function(tm_create_new_task_by_url, (void *)&_param,&(_param._handle),&(_param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif
	return ret_val;
}

ETDLL_API _int32 et_create_continue_task_by_url(char* url, u32 url_length, 
									  char* ref_url, u32 ref_url_length,
									  char* description, u32 description_len,
									  char* file_path, u32 file_path_len,
									  char* file_name, u32 file_name_length,
									  u32* task_id)
{
	TM_CON_TASK_URL _param;
	_int32 url_len = url_length;
	_int32 ref_url_len = ref_url_length;
	char decode_url[MAX_URL_LEN] = {0};
	char ref_decode_url[MAX_URL_LEN] = {0};
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("create_continue_task_by_url[%s]",url);
	
	CHECK_CRITICAL_ERROR;

	if((url==NULL)||(url_length>=MAX_URL_LEN)||(url_length==0)||(sd_strlen(url)==0)) return DT_ERR_INVALID_URL;
	
	if(ref_url_length>=MAX_URL_LEN) return TM_ERR_INVALID_PARAMETER;
	
	//if(description_len>=MAX_URL_LEN*2) return TM_ERR_INVALID_PARAMETER;
	
	if((file_path==NULL)||(file_path_len>=MAX_FILE_PATH_LEN)||(file_path_len==0)||(sd_strlen(file_path)==0)) return DT_ERR_INVALID_FILE_PATH;
	
	if((file_name==NULL)||(file_name_length>=MAX_FILE_NAME_LEN)||(file_name_length==0)||(sd_strlen(file_name)==0)) return DT_ERR_INVALID_FILE_NAME;
	
	if(task_id==NULL) return TM_ERR_INVALID_PARAMETER;

	url_len = url_object_decode( url, decode_url, MAX_URL_LEN );
	if(url_len == -1) return DT_ERR_INVALID_URL;
	url_len = sd_strlen(decode_url);

	if( ref_url != NULL && ref_url_length != 0 )
	{
		ref_url_len = url_object_decode( ref_url, ref_decode_url, MAX_URL_LEN );
	    if(ref_url_len== -1) return DT_ERR_INVALID_URL;	
		ref_url_len = sd_strlen(ref_decode_url);
	}
	
	sd_memset(&_param,0,sizeof(TM_CON_TASK_URL));
	_param.url= decode_url;
	_param.url_length= url_len;
	_param.ref_url= ref_decode_url;
	_param.ref_url_length= ref_url_len;
	_param.description= description;
	_param.description_len= description_len;
	_param.file_path= file_path;
	_param.file_path_len= file_path_len;
	_param.file_name= file_name;
	_param.file_name_length= file_name_length;
	_param.task_id= task_id;
	_param.origion_url = url;
	_param.origion_url_length = url_length;
	
	return tm_post_function(tm_create_continue_task_by_url, (void *)&_param,&(_param._handle),&(_param._result));
}

	//新建任务-用tcid下
ETDLL_API _int32 et_create_new_task_by_tcid(u8 *tcid, uint64 file_size, char *file_name, u32 file_name_length, char*file_path, u32 file_path_len, u32* task_id )
{
	TM_NEW_TASK_CID _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("create_new_task_by_tcid,file_name=%s",file_name);

	CHECK_CRITICAL_ERROR;

	if(tcid==NULL) return DT_ERR_INVALID_TCID;
	
	if((file_path==NULL)||(file_path_len>=MAX_FILE_PATH_LEN)||(file_path_len==0)||(sd_strlen(file_path)==0)) return DT_ERR_INVALID_FILE_PATH;
	
	if((file_name==NULL)||(file_name_length>=MAX_FILE_NAME_LEN)||(file_name_length==0)||(sd_strlen(file_name)==0)) return DT_ERR_INVALID_FILE_NAME;
	
	if(task_id==NULL) return TM_ERR_INVALID_PARAMETER;
	
	sd_memset(&_param,0,sizeof(TM_NEW_TASK_CID));
	_param.tcid= tcid;
	_param.file_size= file_size;
	_param.file_path= file_path;
	_param.file_path_len= file_path_len;
	_param.file_name= file_name;
	_param.file_name_length= file_name_length;
	_param.task_id= task_id;
	
	return tm_post_function(tm_create_new_task_by_tcid, (void *)&_param,&(_param._handle),&(_param._result));
}

	//断点续传-用tcid下
ETDLL_API _int32 et_create_continue_task_by_tcid(u8 *tcid, char*file_name, u32 file_name_length, char *file_path, u32 file_path_len, u32* task_id)
{
	TM_CON_TASK_CID _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("create_continue_task_by_tcid,file_name=%s",file_name);

	CHECK_CRITICAL_ERROR;

	if(tcid==NULL) return DT_ERR_INVALID_TCID;
	
	if((file_path==NULL)||(file_path_len>=MAX_FILE_PATH_LEN)||(file_path_len==0)||(sd_strlen(file_path)==0)) return DT_ERR_INVALID_FILE_PATH;
	
	if((file_name==NULL)||(file_name_length>=MAX_FILE_NAME_LEN)||(file_name_length==0)||(sd_strlen(file_name)==0)) return DT_ERR_INVALID_FILE_NAME;
	
	if(task_id==NULL) return TM_ERR_INVALID_PARAMETER;
	
	sd_memset(&_param,0,sizeof(TM_CON_TASK_CID));
	_param.tcid= tcid;
	_param.file_path= file_path;
	_param.file_path_len= file_path_len;
	_param.file_name= file_name;
	_param.file_name_length= file_name_length;
	_param.task_id= task_id;
	
	return tm_post_function(tm_create_continue_task_by_tcid, (void *)&_param,&(_param._handle),&(_param._result));
}

ETDLL_API _int32 et_create_task_by_tcid_file_size_gcid(u8 *tcid, uint64 file_size, u8 *gcid,char *file_name, u32 file_name_length, char *file_path, u32 file_path_len, u32* task_id )
{
	TM_NEW_TASK_GCID _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_create_task_by_tcid_file_size_gcid,file_name=%s",file_name);

	CHECK_CRITICAL_ERROR;

	if((tcid==NULL)||(gcid==NULL)	||(file_size==0) ||(file_name==NULL)||(sd_strlen(file_name)==0)||(file_name_length==0)||(file_name_length>=MAX_FILE_NAME_LEN) ||(file_path==NULL)||(sd_strlen(file_path)==0)||(file_path_len==0)||(file_path_len>=MAX_FILE_PATH_LEN)||(task_id==NULL))
	{
		return TM_ERR_INVALID_PARAMETER;
	}
	
	sd_memset(&_param,0,sizeof(TM_NEW_TASK_GCID));
	_param.tcid= tcid;
	_param.file_size= file_size;
	_param.gcid= gcid;
	_param.file_path= file_path;
	_param.file_path_len= file_path_len;
	_param.file_name= file_name;
	_param.file_name_length= file_name_length;
	_param.task_id= task_id;
	
	return tm_post_function(tm_create_task_by_tcid_file_size_gcid, (void *)&_param,&(_param._handle),&(_param._result));
}

	
ETDLL_API _int32 et_create_new_bt_task(char* seed_file_full_path, u32 seed_file_full_path_len, 
								char* file_path, u32 file_path_len,
								u32* download_file_index_array, u32 file_num,
								u32* task_id)
{
#ifdef ENABLE_BT

	TM_BT_TASK _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("create_new_bt_task,seed_file_full_path=%s",seed_file_full_path);

	CHECK_CRITICAL_ERROR;
	
	if((seed_file_full_path==NULL)||(sd_strlen(seed_file_full_path)==0)||(seed_file_full_path_len>=MAX_FILE_PATH_LEN+MAX_FILE_NAME_LEN)
		||(sd_strlen(seed_file_full_path)!=seed_file_full_path_len)
		||(file_path_len>=MAX_FILE_PATH_LEN)||(sd_strlen(file_path)==0)||(sd_strlen(file_path)!=file_path_len)
		||(download_file_index_array==NULL)||(file_num==0)||(task_id==NULL)
	  )
	{
		return TM_ERR_INVALID_PARAMETER;
	}

	sd_memset(&_param,0,sizeof(TM_BT_TASK));

	sd_strncpy(_param._seed_full_path, seed_file_full_path, MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN );
	_param._seed_full_path_len= seed_file_full_path_len;
	sd_strncpy(_param._data_path, file_path, MAX_FILE_PATH_LEN );

	_param._data_path_len= file_path_len;
	
	/* Make sure the file path is end with a '/'  */
	if(_param._data_path[_param._data_path_len-1]!='/')
	{
		if(_param._data_path_len+1==MAX_FILE_PATH_LEN)
		{
			return DT_ERR_INVALID_FILE_PATH;
		}
		else
		{
			_param._data_path[_param._data_path_len]='/';
			_param._data_path_len++;
			_param._data_path[_param._data_path_len]='\0';
		}
	}
	
	_param._download_file_index_array= download_file_index_array;
	_param._file_num= file_num;
	_param._encoding_switch_mode = ET_ENCODING_DEFAULT_SWITCH;
	_param._task_id= task_id;
	
	return tm_post_function(tm_create_bt_task, (void *)&_param,&(_param._handle),&(_param._result));
#else
	return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_BT */
}


int32 et_create_bt_task(char* seed_file_full_path, u32 seed_file_full_path_len, 
								char* file_path, u32 file_path_len,
								u32* download_file_index_array, u32 file_num,
								enum ET_ENCODING_SWITCH_MODE encoding_switch_mode, u32* task_id)
{
#ifdef ENABLE_BT 
	TM_BT_TASK _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_create_bt_task,seed_file_full_path=%s, file_path:%s, encoding_switch_mode:%d",
		seed_file_full_path, file_path, encoding_switch_mode);

	CHECK_CRITICAL_ERROR;
	if((seed_file_full_path==NULL)||(sd_strlen(seed_file_full_path)==0)||(seed_file_full_path_len>=MAX_FILE_PATH_LEN+MAX_FILE_NAME_LEN)
		||(sd_strlen(seed_file_full_path)!=seed_file_full_path_len)
		||(file_path_len>=MAX_FILE_PATH_LEN)||(sd_strlen(file_path)==0)||(sd_strlen(file_path)!=file_path_len)
		||(download_file_index_array==NULL)||(file_num==0)||(task_id==NULL)
	  )
	{
		return TM_ERR_INVALID_PARAMETER;
	}


	sd_memset(&_param,0,sizeof(TM_BT_TASK));

	sd_strncpy(_param._seed_full_path, seed_file_full_path, MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN );
	_param._seed_full_path_len= seed_file_full_path_len;
	sd_strncpy(_param._data_path, file_path, MAX_FILE_PATH_LEN );

	_param._data_path_len= file_path_len;
	
	/* Make sure the file path is end with a '/'  */
	if(_param._data_path[_param._data_path_len-1]!='/')
	{
		if(_param._data_path_len+1==MAX_FILE_PATH_LEN)
		{
			return DT_ERR_INVALID_FILE_PATH;
		}
		else
		{
			_param._data_path[_param._data_path_len]='/';
			_param._data_path_len++;
			_param._data_path[_param._data_path_len]='\0';
		}
	}
	
	_param._download_file_index_array= download_file_index_array;
	_param._file_num= file_num;
	_param._encoding_switch_mode = encoding_switch_mode;
	_param._task_id= task_id;
	
	return tm_post_function(tm_create_bt_task, (void *)&_param,&(_param._handle),&(_param._result));
#else
	return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_BT */
}

int32 et_create_bt_magnet_task(char* url, u32 url_len, 
							char* file_path, u32 file_path_len,char * file_name,u32 file_name_len,BOOL bManual,
							enum ET_ENCODING_SWITCH_MODE encoding_switch_mode, u32* task_id)
{
#ifdef ENABLE_BT 
	TM_BT_MAGNET_TASK _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_create_bt_magnet_task,url=%s, encoding_switch_mode:%d",
		url, encoding_switch_mode);

	CHECK_CRITICAL_ERROR;
	if((url==NULL)||(sd_strlen(url)==0)||(url_len>=MAX_BT_MAGNET_LEN)
		||(file_path_len>=MAX_FILE_PATH_LEN)||(sd_strlen(file_path)==0)||(sd_strlen(file_path)!=file_path_len)
		|| sd_test_path_writable(file_path)
	  )
	{
		return TM_ERR_INVALID_PARAMETER;
	}


	sd_memset(&_param,0,sizeof(TM_BT_MAGNET_TASK));

	sd_strncpy(_param._url, url, MAX_BT_MAGNET_LEN );
	_param._url_len= url_len;
    
	sd_strncpy(_param._data_path, file_path, MAX_FILE_PATH_LEN );
	_param._data_path_len= file_path_len;

	sd_strncpy(_param._data_name, file_name, MAX_FILE_PATH_LEN );
	_param._data_name_len= file_name_len;

	_param._bManual= bManual;

	_param._encoding_switch_mode= encoding_switch_mode;
	_param._task_id= task_id;
	
	return tm_post_function(tm_create_bt_magnet_task, (void *)&_param,&(_param._handle),&(_param._result));
#else
	return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_BT */
}


ETDLL_API _int32 et_create_emule_task(const char* ed2k_link, u32 ed2k_link_len, char* path, u32 path_len, char* file_name, u32 file_name_length, u32* task_id)
{
#ifdef ENABLE_EMULE
	TM_EMULE_PARAM param;
	if(!g_already_init)
		return -1;
	CHECK_CRITICAL_ERROR;
	if(ed2k_link == NULL || ed2k_link_len > MAX_ED2K_LEN|| path == NULL ||path_len > MAX_FILE_PATH_LEN || task_id == NULL)
		return TM_ERR_INVALID_PARAMETER;
	if((file_name_length>=MAX_FILE_NAME_LEN)||(sd_strlen(file_name)!=file_name_length)) return DT_ERR_INVALID_FILE_NAME;

	LOG_DEBUG("et_create_emule_task[%s], path:%s, name:%s",ed2k_link, path, file_name);

	sd_memset(&param,0,sizeof(TM_EMULE_PARAM));
	sd_strncpy(param._ed2k_link, ed2k_link, ed2k_link_len);
	sd_strncpy(param._file_path, path, path_len);
    if( param._file_path[path_len-1] != '/' )
    {
        param._file_path[path_len] = '/';
    }
    
	sd_strncpy(param._file_name, file_name, file_name_length);
	param._task_id= task_id;
	return tm_post_function(tm_create_emule_task, (void *)&param,&(param._handle),&(param._result));
#else
	return NOT_IMPLEMENT;
#endif
}




ETDLL_API _int32 et_set_task_no_disk(u32 task_id)
{
#ifdef _VOD_NO_DISK   
	TM_START_TASK _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_set_task_no_disk,task_id=%u",task_id);
	
	CHECK_CRITICAL_ERROR;

	if(task_id==0) return TM_ERR_INVALID_TASK_ID;

	sd_memset(&_param,0,sizeof(TM_START_TASK));
	_param.task_id= task_id;
	
	return tm_post_function(tm_set_task_no_disk, (void *)&_param,&(_param._handle),&(_param._result));
#else
	return NOT_IMPLEMENT;
#endif /* #ifdef _VOD_NO_DISK */
}

ETDLL_API _int32 et_vod_set_task_check_data(u32 task_id)
{
#ifdef _VOD_NO_DISK   
	TM_START_TASK _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_vod_set_task_check_data,task_id=%u",task_id);
	
	CHECK_CRITICAL_ERROR;

	if(task_id==0) return TM_ERR_INVALID_TASK_ID;

	sd_memset(&_param,0,sizeof(TM_START_TASK));
	_param.task_id= task_id;
	
	return tm_post_function(tm_set_task_check_data, (void *)&_param,&(_param._handle),&(_param._result));
#else
	return NOT_IMPLEMENT;
#endif /* #ifdef _VOD_NO_DISK */
}
ETDLL_API _int32 et_set_task_write_mode(u32 task_id,u32 mode)
{
	TM_POST_PARA_2 _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_set_task_write_mode,task_id=%u",task_id);
	
	CHECK_CRITICAL_ERROR;

	if(task_id==0) return TM_ERR_INVALID_TASK_ID;

	sd_memset(&_param,0,sizeof(TM_POST_PARA_2));
	_param._para1= (void*)task_id;
	_param._para2= (void*)mode;
	
	return tm_post_function(tm_set_task_write_mode, (void *)&_param,&(_param._handle),&(_param._result));
}
ETDLL_API _int32 et_get_task_write_mode(u32 task_id,u32 * mode)
{
	TM_POST_PARA_2 _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_get_task_write_mode,task_id=%u",task_id);
	
	CHECK_CRITICAL_ERROR;

	if(task_id==0) return TM_ERR_INVALID_TASK_ID;

	sd_memset(&_param,0,sizeof(TM_POST_PARA_2));
	_param._para1= (void*)task_id;
	_param._para2= (void*)mode;
	
	return tm_post_function(tm_get_task_write_mode, (void *)&_param,&(_param._handle),&(_param._result));
}

ETDLL_API _int32 et_start_task(u32 task_id)
{
	TM_START_TASK _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("start_task,task_id=%u",task_id);
	
	CHECK_CRITICAL_ERROR;

	if(task_id==0) return TM_ERR_INVALID_TASK_ID;

	sd_memset(&_param,0,sizeof(TM_START_TASK));
	_param.task_id= task_id;
	
	return tm_post_function(tm_start_task, (void *)&_param,&(_param._handle),&(_param._result));
}

ETDLL_API _int32 et_stop_task(u32 task_id)
{
    LOG_DEBUG("stop_task, task_id=%u", task_id);
    if (!et_check_running())
    {
        LOG_URGENT("et_stop_task:critical error");
#ifdef MACOS
        printf("\n\n ***** et_stop_task:critical error !!! **** \n\n");
#endif
        sd_assert(FALSE);
    }
    
	if (0 == task_id)
	{
	    return TM_ERR_INVALID_TASK_ID;
	}

	TM_STOP_TASK _param;
	sd_memset(&_param, 0, sizeof(TM_STOP_TASK));
	_param.task_id= task_id;
	return tm_post_function(tm_stop_task, (void *)&_param, &(_param._handle), &(_param._result));
}


ETDLL_API _int32 et_delete_task(u32 task_id)
{
    LOG_URGENT("delete_task,task_id=%u", task_id);
    if (0 == task_id)
    {
        return TM_ERR_INVALID_TASK_ID;
    }

    TM_DEL_TASK _param;
	sd_memset(&_param, 0, sizeof(TM_DEL_TASK));
	_param.task_id= task_id;
	return tm_post_function(tm_delete_task, (void *)&_param, &(_param._handle), &(_param._result));
}

ETDLL_API _int32 et_get_task_info(u32 task_id, ET_TASK *info)
{

	TM_GET_TASK_INFO _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("get_task_info,task_id=%u",task_id);

	if(get_critical_error()!=SUCCESS)
		return get_critical_error();

	if(task_id==0) return TM_ERR_INVALID_TASK_ID;
	if(!info) return TM_ERR_INVALID_PARAMETER;

	sd_memset(&_param,0,sizeof(TM_GET_TASK_INFO));
	_param.task_id= task_id;
	_param.info= info;
	
	return tm_get_task_info((void *)&_param);

}

ETDLL_API _int32 et_get_task_info_ex(u32 task_id, void *info)
{

    TM_GET_TASK_INFO _param;

    if(!g_already_init)
        return -1;

    LOG_DEBUG("get_task_info,task_id=%u",task_id);

    if(get_critical_error()!=SUCCESS)
        return get_critical_error();

    if(task_id==0) return TM_ERR_INVALID_TASK_ID;
    if(!info) return TM_ERR_INVALID_PARAMETER;

    sd_memset(&_param,0,sizeof(TM_GET_TASK_INFO));
    _param.task_id= task_id;
    _param.info= info;

    return tm_post_function(tm_get_task_info_ex, (void *)&_param,&(_param._handle),&(_param._result));
}

ETDLL_API int32 et_get_task_file_name(u32 task_id, char *file_name_buffer, int32* buffer_size)
{
	TM_GET_TASK_FILE_NAME _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("get_task_file_name,task_id=%u",task_id);

	CHECK_CRITICAL_ERROR;
 
	if(task_id==0) return TM_ERR_INVALID_TASK_ID;

	if((file_name_buffer==NULL)||(buffer_size==NULL)||(*buffer_size==0)) return TM_ERR_INVALID_PARAMETER;

	sd_memset(&_param,0,sizeof(TM_GET_TASK_FILE_NAME));
	_param.task_id= task_id;
	_param.file_name_buffer= file_name_buffer;
	_param.buffer_size= buffer_size;
	

	return tm_post_function(tm_get_task_file_name, (void *)&_param,&(_param._handle),&(_param._result));
}
ETDLL_API int32 et_get_bt_task_sub_file_name(u32 task_id, u32 file_index,char *file_name_buffer, int32* buffer_size)
{
	TM_GET_TASK_FILE_NAME _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_get_bt_task_sub_file_name,task_id=%u",task_id);

	CHECK_CRITICAL_ERROR;
 
	if(task_id==0) return TM_ERR_INVALID_TASK_ID;

	if((file_name_buffer==NULL)||(buffer_size==NULL)||(*buffer_size==0)) return TM_ERR_INVALID_PARAMETER;

	sd_memset(&_param,0,sizeof(TM_GET_TASK_FILE_NAME));
	_param.task_id= task_id;
	_param.file_name_buffer= file_name_buffer;
	_param.buffer_size= buffer_size;
	_param.file_index = file_index;

	return tm_post_function(tm_get_bt_task_sub_file_name, (void *)&_param,&(_param._handle),&(_param._result));
}
ETDLL_API int32 et_get_task_tcid(u32 task_id, u8 * tcid)
{
	TM_GET_TASK_TCID _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_get_task_tcid,task_id=%u",task_id);

	CHECK_CRITICAL_ERROR;
 
	if(task_id==0) return TM_ERR_INVALID_TASK_ID;

	if(tcid==NULL) return TM_ERR_INVALID_PARAMETER;

	sd_memset(&_param,0,sizeof(TM_GET_TASK_TCID));
	_param.task_id= task_id;
	_param.tcid= tcid;
	

	return tm_post_function(tm_get_task_tcid, (void *)&_param,&(_param._handle),&(_param._result));
}

ETDLL_API int32 et_get_bt_task_sub_file_tcid(u32 task_id, u32 file_index,u8 * tcid)
{
	TM_GET_TASK_TCID _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_get_bt_task_sub_file_tcid,task_id=%u,file_index=%u",task_id,file_index);

	CHECK_CRITICAL_ERROR;
 
	if(task_id==0) return TM_ERR_INVALID_TASK_ID;

	if(tcid==NULL) return TM_ERR_INVALID_PARAMETER;

	sd_memset(&_param,0,sizeof(TM_GET_TASK_TCID));
	_param.task_id= task_id;
	_param.tcid= tcid;
	_param.file_index = file_index;

	return tm_post_function(tm_get_bt_task_sub_file_tcid, (void *)&_param,&(_param._handle),&(_param._result));
}

//jjxh added...
ETDLL_API int32 et_get_bt_task_sub_file_gcid(u32 task_id, u32 file_index,u8 * tcid)
{
	TM_GET_TASK_TCID _param;

	if(!g_already_init)
		return -1;

	LOG_DEBUG("et_get_bt_task_sub_file_gcid,task_id=%u,file_index=%u",task_id,file_index);

	CHECK_CRITICAL_ERROR;

	if(task_id==0) return TM_ERR_INVALID_TASK_ID;

	if(tcid==NULL) return TM_ERR_INVALID_PARAMETER;

	sd_memset(&_param,0,sizeof(TM_GET_TASK_TCID));
	_param.task_id= task_id;
	_param.tcid= tcid;
	_param.file_index = file_index;

	return tm_post_function(tm_get_bt_task_sub_file_gcid, (void *)&_param,&(_param._handle),&(_param._result));
}



ETDLL_API int32 et_get_task_gcid(u32 task_id, u8 * gcid)
{
	TM_GET_TASK_GCID _param;

	if(!g_already_init)
		return -1;

	LOG_DEBUG("et_get_task_gcid, task_id=%u", task_id);

	CHECK_CRITICAL_ERROR;

	if(task_id == 0) return TM_ERR_INVALID_TASK_ID;

	if(gcid == NULL) return TM_ERR_INVALID_PARAMETER;

	sd_memset(&_param, 0, sizeof(TM_GET_TASK_GCID));
	_param.task_id = task_id;
	_param.gcid = gcid;

	return tm_post_function(tm_get_task_gcid, (void *)&_param, &(_param._handle), &(_param._result));
}

ETDLL_API int32 et_add_task_resource(u32 task_id, void * p_resource)
{

    TM_ADD_TASK_RES _param;

    if(!g_already_init)
        return -1;

    LOG_ERROR("et_add_task_resource,task_id=%u",task_id);

    CHECK_CRITICAL_ERROR;

    if(task_id==0) return TM_ERR_INVALID_TASK_ID;

    if(p_resource==NULL) return TM_ERR_INVALID_PARAMETER;

    sd_memset(&_param,0,sizeof(TM_ADD_TASK_RES));
    _param.task_id= task_id;
    _param._res= p_resource;


    return tm_post_function(tm_add_task_resource, (void *)&_param,&(_param._handle),&(_param._result));

}

int32 et_get_bt_file_info(u32 task_id, u32 file_index, ET_BT_FILE *info)
{
 #ifdef ENABLE_BT
	TM_GET_BT_FILE_INFO _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("get_bt_file_info,task_id=%u",task_id);

	CHECK_CRITICAL_ERROR;

	if(task_id==0) return TM_ERR_INVALID_TASK_ID;

	if((info==NULL)||(file_index==-1)) return TM_ERR_INVALID_PARAMETER;

	sd_memset(&_param,0,sizeof(TM_GET_BT_FILE_INFO));
	_param.task_id= task_id;
	_param.info= info;
	_param.file_index = file_index;
	
	return tm_get_bt_file_info((void *)&_param);
#else
	return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_BT */
}

//判断磁力连任务种子文件是否下载完成，
_int32 et_get_bt_magnet_torrent_seed_downloaded(_u32 task_id, BOOL *_seed_download)
{
 #ifdef ENABLE_BT
	TM_GET_TORRENT_SEED_FILE_PATH _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_get_bt_magnet_torrent_seed_downloaded, task_id=%u",task_id);

	CHECK_CRITICAL_ERROR;

	if(task_id==0) return TM_ERR_INVALID_TASK_ID;

	if(_seed_download==NULL) return TM_ERR_INVALID_PARAMETER;

	sd_memset(&_param, 0, sizeof(TM_GET_TORRENT_SEED_FILE_PATH));
	_param.task_id = task_id;
	_param._seed_download = _seed_download;

	return tm_post_function(tm_get_bt_magnet_torrent_seed_downloaded,(void *)&_param,&(_param._handle),&(_param._result)); 
#else   
	return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_BT */
}

ETDLL_API const char* et_get_version(void)
{
	static char version_buffer[MAX_VERSION_LEN] = {0};

	if (0 == sd_strlen(version_buffer))
	{
	    sd_strncpy(version_buffer, ET_INNER_VERSION, MAX_VERSION_LEN-1);
	}

	return version_buffer;
}

ETDLL_API int32 et_set_download_record_file_path(const char *path,u32  path_len)
{
#ifdef UPLOAD_ENABLE
	TM_SET_RC_PATH _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_set_download_record_file_path,path=%s",path);
	
	CHECK_CRITICAL_ERROR;

	if((path==NULL)||(path_len==0)||(path_len>=MAX_FILE_PATH_LEN)||(sd_strlen(path)!=path_len)||(!sd_file_exist(path)))
		 return TM_ERR_INVALID_PARAMETER;
	
	sd_memset(&_param,0,sizeof(TM_SET_RC_PATH));
	_param.path= path;
	_param.path_len= path_len;
	
	return tm_post_function(tm_set_download_record_file_path, (void *)&_param,&(_param._handle),&(_param._result));
#else
	return NOT_IMPLEMENT;
#endif /* #ifdef UPLOAD_ENABLE */
}

/************************************************* 
  Function:    et_set_decision_filename_policy   
  Description: 设置文件命名策略，用于解决MIUI添加后缀名的问题
  Input:       ET_FILENAME_POLICY namepolicy 取值范围:  
                                             ET_FILENAME_POLICY_DEFAULT_SMART:智能命名
                                             ET_FILENAME_POLICY_SIMPLE_USER: 用户命名
  Output:      无
  Return:      成功返回: SUCCESS

  History:     created: chenyangyuan 2013-10-12    
               modified:   
*************************************************/
ETDLL_API int32 et_set_decision_filename_policy(ET_FILENAME_POLICY namepolicy)
{
    LOG_DEBUG("et_set_decision_filename_policy, namepolicy = %d",namepolicy);
    
    return dm_set_decision_filename_policy(namepolicy);
}

/************************************************* 
  Function:    et_set_device_id   
  Description: 通过device_id来设置peerid;初始化之前都要调用一次
  Input:       const char *device_id 设备标识，不同设备具有唯一性
               int32 id_length 取值:PEER_ID_SIZE - 4 (16-4=12)
  Output:      无
  Return:      成功返回: SUCCESS
               失败返回: ALREADY_ET_INIT
                         INVALID_ARGUMENT
  History:     created: chenyangyuan 2013-9-30    
               modified:   
*************************************************/

ETDLL_API int32 et_set_device_id(const char *device_id, int32 id_length)
{
    int i;
    
    if (NULL == device_id || (id_length != PEER_ID_SIZE - 4))
    {
        return INVALID_ARGUMENT;
    }

    /* if et already init */
    if(g_already_init)
	{
		return ALREADY_ET_INIT;
	}
	
    /* device_id必须是十六进制字符串 */
    for (i = 0; i < id_length; i++)
    {
        if (('0' <= *(device_id+i) && *(device_id+i) <= '9')
            || ('a' <= *(device_id+i) && *(device_id+i) <= 'f')
            || ('A' <= *(device_id+i) && *(device_id+i) <= 'F'))
        {
            continue;
        }
        else
        {
            return INVALID_ARGUMENT;
        }
    }
	
	return set_peerid(device_id, id_length);
}

ETDLL_API int32 et_set_max_tasks(u32 task_num)
{
	TM_SET_TASK_NUM _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("set_task_num,max_tasks=%u",task_num);
	
	CHECK_CRITICAL_ERROR;

	if((task_num<1)||(task_num>16)) return TM_ERR_INVALID_PARAMETER;
	sd_memset(&_param,0,sizeof(TM_SET_TASK_NUM));
	_param.max_tasks= task_num;
	
	return tm_post_function(tm_set_task_num, (void *)&_param,&(_param._handle),&(_param._result));
}

ETDLL_API u32 et_get_max_tasks(void)
{
	int32 ret_val =SUCCESS;
	_u32 task_num;
	TM_GET_TASK_NUM _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("get_task_num");
	
	CHECK_CRITICAL_ERROR;

	sd_memset(&_param,0,sizeof(TM_GET_TASK_NUM));
	_param.max_tasks= &task_num;
	
	ret_val=tm_post_function(tm_get_task_num, (void *)&_param,&(_param._handle),&(_param._result));
	
	 if(ret_val ==SUCCESS)
	 	return task_num;

       return 0;
}

//返回值: 0  success
//value的单位:kbytes/second，为-1 表示不限速
ETDLL_API int32 et_set_limit_speed(u32 download_limit_speed, u32 upload_limit_speed)
{
    _int32 ret = SUCCESS;
	if (!g_already_init)
	{
	    return -1;
	}
	
	LOG_DEBUG("set_speed_limit:dl_speed=%u,ul_speed=%u", download_limit_speed, upload_limit_speed);
    if ((download_limit_speed < 1) || (upload_limit_speed < 1))
    {
        return TM_ERR_INVALID_PARAMETER;
    }	
	
	CHECK_CRITICAL_ERROR;
	TM_SET_SPEED _param ;
    sd_memset(&_param,0,sizeof(TM_SET_SPEED));
    _param._download_limit_speed = download_limit_speed;
    _param._upload_limit_speed = upload_limit_speed;
    return tm_post_function(tm_set_speed_limit, (void *)&_param,&(_param._handle),&(_param._result));
}

//返回值: 0  success
//value的单位:kbytes/second，为-1 表示不限速
int32 et_get_limit_speed(u32* download_limit_speed, u32* upload_limit_speed)
{
	TM_GET_SPEED _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("get_speed_limit");
	
	CHECK_CRITICAL_ERROR;

	if((download_limit_speed==NULL)||(upload_limit_speed==NULL)) return TM_ERR_INVALID_PARAMETER;
	
	sd_memset(&_param,0,sizeof(TM_GET_SPEED));
	_param._download_limit_speed = download_limit_speed;
	_param._upload_limit_speed = upload_limit_speed;
	return tm_post_function(tm_get_speed_limit, (void *)&_param,&(_param._handle),&(_param._result));
}

//配置任务连接数
ETDLL_API int32 et_set_max_task_connection(u32 connection_num)
{
	TM_SET_CONNECTION _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("set_task_connection_limit =%u",connection_num);
	
	CHECK_CRITICAL_ERROR;

	if((connection_num<1)||(connection_num>2000)) return TM_ERR_INVALID_PARAMETER;
	sd_memset(&_param,0,sizeof(TM_SET_CONNECTION));
	_param.task_connection= connection_num;
	
	return tm_post_function(tm_set_task_connection_limit, (void *)&_param,&(_param._handle),&(_param._result));
}

ETDLL_API u32 et_get_max_task_connection(void)
{
	int32 ret_val =SUCCESS;
	_u32 task_connection;
	
	TM_GET_CONNECTION _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("get_task_connection_limit");
	
	CHECK_CRITICAL_ERROR;

	sd_memset(&_param,0,sizeof(TM_GET_CONNECTION));
	_param.task_connection= &task_connection;
	
	ret_val=tm_post_function(tm_get_task_connection_limit, (void *)&_param,&(_param._handle),&(_param._result));

	 if(ret_val ==SUCCESS)
	 	return task_connection;

       return 0;
}

ETDLL_API u32 et_get_current_download_speed(void)
{
	if(!g_already_init)
		return -1;
	
	CHECK_CRITICAL_ERROR;
    //NODE:jieouy 这里涉及到多线程对一个未加锁变量的操作，需要review
	return socket_proxy_speed_limit_get_download_speed();
}

ETDLL_API u32 et_get_current_upload_speed(void)
{
	if(!g_already_init)
		return -1;
	
	CHECK_CRITICAL_ERROR;
    //NODE:jieouy 这里涉及到多线程对一个未加锁变量的操作，需要review
	return socket_proxy_speed_limit_get_upload_speed();
}

/* Get the torrent seed file information from seed_path and store the information in the struct: pp_seed_info */
ETDLL_API int32 et_get_torrent_seed_info( char *seed_path, ET_TORRENT_SEED_INFO **pp_seed_info )
{
#ifdef ENABLE_BT
	TM_GET_TORRENT_SEED_INFO _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("get_torrent_seed_info,seed_path=%s",seed_path);

	CHECK_CRITICAL_ERROR;

	if((seed_path==NULL)||(sd_strlen(seed_path)>=MAX_FILE_PATH_LEN+MAX_FILE_NAME_LEN)||(pp_seed_info==NULL)||(!sd_file_exist(seed_path))) return TM_ERR_INVALID_PARAMETER;
	
	sd_memset(&_param,0,sizeof(TM_GET_TORRENT_SEED_INFO));
	_param.seed_path = seed_path;
	
	_param.encoding_switch_mode = ET_ENCODING_DEFAULT_SWITCH;
	_param.pp_seed_info=(void**)pp_seed_info;
	
	return tm_post_function(tm_get_torrent_seed_info, (void *)&_param,&(_param._handle),&(_param._result));
#else
	return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_BT */
}


/* Get the torrent seed file information from seed_path and store the information in the struct: pp_seed_info */
ETDLL_API int32 et_get_torrent_seed_info_with_encoding_mode( char *seed_path, enum ET_ENCODING_SWITCH_MODE encoding_switch_mode, ET_TORRENT_SEED_INFO **pp_seed_info )
{
#ifdef ENABLE_BT
	TM_GET_TORRENT_SEED_INFO _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("get_torrent_seed_info,seed_path=%s",seed_path);

	CHECK_CRITICAL_ERROR;

	if((seed_path==NULL)||(sd_strlen(seed_path)==0)||(sd_strlen(seed_path)>=MAX_FULL_PATH_LEN)||(pp_seed_info==NULL)||(!sd_file_exist(seed_path))) return TM_ERR_INVALID_PARAMETER;
	
	sd_memset(&_param,0,sizeof(TM_GET_TORRENT_SEED_INFO));
	_param.seed_path = seed_path;
	
	_param.encoding_switch_mode = encoding_switch_mode;
	_param.pp_seed_info=(void**)pp_seed_info;
	
	return tm_post_function(tm_get_torrent_seed_info, (void *)&_param,&(_param._handle),&(_param._result));
#else
	return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_BT */
}


/* Release the memory in the struct:p_seed_info which malloc by  et_get_torrent_seed_info */
ETDLL_API int32 et_release_torrent_seed_info( ET_TORRENT_SEED_INFO *p_seed_info )
{
#ifdef ENABLE_BT
	TM_RELEASE_TORRENT_SEED_INFO _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("release_torrent_seed_info");
	
	CHECK_CRITICAL_ERROR;

	if(p_seed_info==NULL) return TM_ERR_INVALID_PARAMETER;
	
	sd_memset(&_param,0,sizeof(TM_RELEASE_TORRENT_SEED_INFO));
	_param.p_seed_info=(void*)p_seed_info;
	
	return tm_post_function(tm_release_torrent_seed_info, (void *)&_param,&(_param._handle),&(_param._result));
#else
	return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_BT */
}

ETDLL_API int32 et_set_seed_switch_type( enum ET_ENCODING_SWITCH_MODE switch_type )
{
#ifdef ENABLE_BT
	TM_SEED_SWITCH_TYPE _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("set_seed_switch_type");
	
	CHECK_CRITICAL_ERROR;

	sd_memset(&_param,0,sizeof(TM_SEED_SWITCH_TYPE));
	_param._switch_type= switch_type;

	return tm_post_function(tm_set_seed_switch_type, (void *)&_param,&(_param._handle),&(_param._result));
#else
	return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_BT */
}

ETDLL_API int32 et_get_bt_download_file_index( u32 task_id, u32 *buffer_len, u32 *file_index_buffer )
{
#ifdef ENABLE_BT
	TM_GET_BT_FILE_INDEX _param;

	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_get_bt_download_file_index");
	
	CHECK_CRITICAL_ERROR;

	if(task_id==0) return TM_ERR_INVALID_TASK_ID;
	
	if((buffer_len==NULL)||(*buffer_len==0)||(file_index_buffer==NULL)) return TM_ERR_INVALID_PARAMETER;

	sd_memset(&_param,0,sizeof(TM_GET_BT_FILE_INDEX));
	
	_param._task_id = task_id;
	_param._buffer_len = buffer_len;
	_param._file_index_buffer = file_index_buffer;

	return tm_post_function(tm_get_bt_download_file_index, (void *)&_param,&(_param._handle),&(_param._result));
#else
	return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_BT */
}
ETDLL_API int32 et_get_bt_file_path_and_name(u32 task_id, u32 file_index,char *file_path_buffer, int32 *file_path_buffer_size, char *file_name_buffer, int32* file_name_buffer_size)
{
#ifdef ENABLE_BT
	TM_GET_BT_TASK_FILE_NAME _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_get_bt_file_path_and_name,task_id=%u",task_id);

	CHECK_CRITICAL_ERROR;

	if(task_id==0) return TM_ERR_INVALID_TASK_ID;
	
	if(file_index==-1) return TM_ERR_INVALID_PARAMETER;
	
	if((file_path_buffer==NULL)||(file_path_buffer_size==NULL)||(*file_path_buffer_size==0)) return TM_ERR_INVALID_PARAMETER;

	if((file_name_buffer==NULL)||(file_name_buffer_size==NULL)||(*file_name_buffer_size==0)) return TM_ERR_INVALID_PARAMETER;

	sd_memset(&_param,0,sizeof(TM_GET_BT_TASK_FILE_NAME));
	_param.task_id= task_id;
	_param.file_index= file_index;
	_param.file_path_buffer= file_path_buffer;
	_param.file_path_buffer_size= file_path_buffer_size;
	_param.file_name_buffer= file_name_buffer;
	_param.file_name_buffer_size= file_name_buffer_size;
	

	return tm_post_function(tm_get_bt_file_path_and_name, (void *)&_param,&(_param._handle),&(_param._result));
#else
	return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_BT */
}



ETDLL_API int32 et_get_bt_tmp_file_path(u32 task_id, char * tmp_file_path, char * tmp_file_name)
{
#ifdef ENABLE_BT
	TM_GET_BT_TMP_FILE_PATH _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_get_bt_tmp_file_path,task_id=%u",task_id);

	CHECK_CRITICAL_ERROR;

	if(task_id==0) return TM_ERR_INVALID_TASK_ID;

	sd_memset(&_param,0,sizeof(TM_GET_BT_TMP_FILE_PATH));
	_param.task_id= task_id;
	_param.tmp_file_path = tmp_file_path;
	_param.tmp_file_name = tmp_file_name;
	

	return tm_post_function(tm_get_bt_tmp_path, (void *)&_param,&(_param._handle),&(_param._result));
#else
	return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_BT */
}

ETDLL_API int32 et_remove_task_tmp_file(u32 task_id)
{
	TM_REMOVE_TMP_FILE _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("remove_task_tmp_file,task_id=%u",task_id);
	
	CHECK_CRITICAL_ERROR;

	if(task_id==0) return TM_ERR_INVALID_TASK_ID;

	sd_memset(&_param,0,sizeof(TM_REMOVE_TMP_FILE));
	_param.task_id= task_id;
	
	return tm_post_function(tm_remove_task_tmp_file, (void *)&_param,&(_param._handle),&(_param._result));
}

ETDLL_API int32 et_set_customed_interface(int32 fun_idx, void *fun_ptr)
{
	if(g_already_init)
		return ALREADY_ET_INIT;

	return set_customed_interface(fun_idx, fun_ptr);
}

#define	I_MEM_IDX_COUNT	3

#define I_MEM_IDX_MALLOC  (0)
#define I_MEM_IDX_FREE		(1)

ETDLL_API int32 et_set_customed_interface_mem(int32 fun_idx, void *fun_ptr)
{
	if(g_already_init)
		return ALREADY_ET_INIT;
	//for test, following 5 lines
 	if(fun_idx < 0 || fun_idx >= I_MEM_IDX_COUNT)
		return INVALID_CUSTOMED_INTERFACE_IDX;
	if(fun_ptr == NULL)
		return INVALID_CUSTOMED_INTERFACE_PTR;
	
	if(fun_idx == I_MEM_IDX_MALLOC){
		fun_idx = CI_MEM_IDX_GET_MEM;
	}else if(fun_idx == I_MEM_IDX_FREE){
		fun_idx = CI_MEM_IDX_FREE_MEM;
	}else{
		return INVALID_CUSTOMED_INTERFACE_IDX;
	}

      return set_customed_interface(fun_idx, fun_ptr);
}

ETDLL_API int32 et_set_host_ip(const char * host_name, const char *ip)
{
	LOG_DEBUG("host:%s, ip:%s", host_name, ip);
    if (!g_already_init)
    {
        return -1;
    }
    CHECK_CRITICAL_ERROR;

    TM_POST_PARA_2 _param;
	sd_memset(&_param, 0, sizeof(_param));
	_param._para1 = (void*)host_name;
	_param._para2 = (void*)ip;
	return tm_post_function(tm_set_host_ip, (void *)&_param, &(_param._handle), &(_param._result));
}

ETDLL_API int32 et_clear_host_ip(void)
{
    if (!g_already_init)
    {
        return -1;
    }
	
	LOG_DEBUG("et_clear_host_ip");
	
	CHECK_CRITICAL_ERROR;
	TM_POST_PARA_0 _param;
	sd_memset(&_param,0,sizeof(_param));	
	return tm_post_function(tm_clear_host_ip, (void *)&_param, &(_param._handle), &(_param._result));
}

void asyn_delete_file_handler(void *arglist)
{
	_int32 ret_val = SUCCESS;
	char *full_path = (char *)arglist;
	sd_pthread_detach();
	sd_ignore_signal();
	LOG_ERROR("asyn_delete_file_handler start %s",full_path);
	ret_val = sd_delete_file(full_path);
	if(ret_val!=SUCCESS)
	{
		
		sd_strcat(full_path, ".rf", 3);
        LOG_ERROR("asyn_delete_file_handler 1:ret_val=%d,%s",ret_val,full_path);

		ret_val = sd_delete_file(full_path);
		
		sd_strcat(full_path, ".cfg", 4);
		LOG_ERROR("asyn_delete_file_handler 2:ret_val=%d,%s",ret_val,full_path);
		
		sd_delete_file(full_path);

		full_path[sd_strlen(full_path) - 4] = '\0';
		sd_strcat(full_path, ".crs", 4);
		sd_delete_file(full_path);		
	}
	
	SAFE_DELETE(full_path);
	LOG_ERROR("asyn_delete_file_handler end");
	
	return ;
}		
_int32 asyn_delete_file(char* file_path)
{
	_int32 thread_id = 0,ret_val = SUCCESS,path_len = sd_strlen(file_path);
	char * p_file_path = NULL;
		
	
	ret_val = sd_malloc(path_len+MAX_TD_CFG_SUFFIX_LEN, (void**)&p_file_path);
	CHECK_VALUE(ret_val);
	sd_memset(p_file_path,0x00,path_len+MAX_TD_CFG_SUFFIX_LEN);
	sd_strncpy(p_file_path, file_path, path_len);
	ret_val = sd_create_task(asyn_delete_file_handler, 1024, (void*)p_file_path, &thread_id);
	CHECK_VALUE(ret_val);
	return SUCCESS;
}

ETDLL_API int32 et_remove_tmp_file(char* file_path, char* file_name)
{
	int32 ret_val=SUCCESS,file_path_len = 0,file_name_len = 0;
	char full_path[MAX_FILE_PATH_LEN+MAX_FILE_NAME_LEN+MAX_TD_CFG_SUFFIX_LEN];

	//if(!g_already_init)
	//	return -1;
	
	//LOG_DEBUG("et_remove_tmp_file:%s%s",file_path,file_name);

	//CHECK_CRITICAL_ERROR;

	file_path_len = sd_strlen(file_path);
	file_name_len = sd_strlen(file_name);
	if((file_path==NULL)||(file_path_len>=MAX_FILE_PATH_LEN)||(file_name==NULL)||(file_name_len>=MAX_FILE_NAME_LEN)||
		(!sd_file_exist(file_path))||(!sd_is_file_name_valid(file_name)))
		return TM_ERR_INVALID_PARAMETER;
	
	sd_memset(full_path,0,MAX_FILE_PATH_LEN+MAX_FILE_NAME_LEN+MAX_TD_CFG_SUFFIX_LEN);

	sd_strncpy(full_path, file_path, MAX_FILE_PATH_LEN+MAX_FILE_NAME_LEN+MAX_TD_CFG_SUFFIX_LEN);
	if(full_path[file_path_len-1]!='/')
		full_path[file_path_len]='/';

	sd_strcat(full_path, file_name, file_name_len);
#if (defined(MACOS) && !defined(OSX))||defined(_ANDROID_LINUX)
	ret_val = asyn_delete_file(full_path);
#else
	ret_val = sd_delete_file(full_path);
	if(ret_val==SUCCESS)
	{
		/* Notify upload manager */
		//ulm_del_record(_u64 size, const _u8 *tcid,const _u8 *gcid);
	}
	else
	{
		LOG_ERROR("et_remove_tmp_file 1:ret_val=%d,%s",ret_val,full_path);
		sd_strcat(full_path, ".rf", 3);

		ret_val = sd_delete_file(full_path);
		LOG_ERROR("et_remove_tmp_file 2:ret_val=%d,%s",ret_val,full_path);
		
		sd_strcat(full_path, ".cfg", 4);
		sd_delete_file(full_path);

		full_path[sd_strlen(full_path) - 4] = '\0';
		sd_strcat(full_path, ".crs", 4);
		sd_delete_file(full_path);
		
	}
	
#endif /* MACOS */
	return ret_val;	
}

ETDLL_API int32 et_get_all_task_id(u32 *buffer_len, u32 *task_id_buffer)
{
    LOG_DEBUG("et_get_all_task_id");
	if ((buffer_len == NULL) || (*buffer_len == 0) || (task_id_buffer == NULL))
	{
	    return TM_ERR_INVALID_PARAMETER;
	}
	
    TM_GET_ALL_TASK_ID _param;
    sd_memset(&_param, 0, sizeof(TM_GET_ALL_TASK_ID));
    _param._buffer_len = buffer_len;
    _param._task_id_buffer = task_id_buffer;
    return tm_post_function(tm_get_all_task_id, (void *)&_param, &(_param._handle), &(_param._result));
}

ETDLL_API int32 et_vod_set_can_read_callback(u32 task_id, void *callback, void *args)
{
#ifdef ENABLE_VOD
    VOD_DATA_MANAGER_READ  _param;

    int32  ret_val = 0;

    if(!g_already_init)
        return -1;

    CHECK_CRITICAL_ERROR;

    if(task_id==0) return TM_ERR_INVALID_TASK_ID;

    sd_memset(&_param,0,sizeof(VOD_DATA_MANAGER_READ));
    _param._task_id = task_id;
    _param._file_index = (int)callback;
    _param._blocktime = (int)args;

    ret_val = tm_post_function(vdm_set_data_change_notify, (void *)&_param,&(_param._handle),&(_param._result));

    return ret_val;
#else
    return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_VOD */
    return -1;
}


ETDLL_API int32 et_vod_read_file(int32 task_id, uint64 start_pos, uint64 len, char *buf, int32 block_time )
{
#ifdef ENABLE_VOD
	VOD_DATA_MANAGER_READ  _param;

        int32  ret_val = 0;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_vod_read_file");

	CHECK_CRITICAL_ERROR;

	if(task_id==0) return TM_ERR_INVALID_TASK_ID;
	
	if((len==0)||(buf==NULL)) return TM_ERR_INVALID_PARAMETER;
	
	sd_memset(&_param,0,sizeof(VOD_DATA_MANAGER_READ));
	_param._task_id = task_id;
	_param._file_index = 0;
	_param._start_pos = start_pos;
	_param._length = len;
	_param._buffer = buf;
	_param._blocktime = block_time;

#ifdef VOD_DEBUG
        printf("begin read task :%u  offset:%llu, len:%llu,  blocktime:%d , time:%u .\n",   task_id , start_pos, len, block_time, time(0));  
#endif
	
       	
	ret_val = tm_post_function(vdm_vod_read_file_handle, (void *)&_param,&(_param._handle),&(_param._result));

#ifdef VOD_DEBUG
        printf("end read task :%u  offset:%llu, len:%llu,  blocktime:%d , time:%u , ret_val:%d .\n",   task_id , start_pos, len, block_time, time(0), ret_val);  
#endif

       return ret_val;
#else
	return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_VOD */
}


ETDLL_API int32 et_vod_bt_read_file(int32 task_id, u32 file_index, uint64 start_pos, uint64 len, char *buf, int32 block_time )
{
#ifdef ENABLE_VOD
	VOD_DATA_MANAGER_READ  _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_vod_bt_read_file");

	CHECK_CRITICAL_ERROR;

	if(task_id==0) return TM_ERR_INVALID_TASK_ID;
	
	if((file_index==-1)||(len==0)||(buf==NULL)) return TM_ERR_INVALID_PARAMETER;
	
	sd_memset(&_param,0,sizeof(VOD_DATA_MANAGER_READ));
	_param._task_id = task_id;
	_param._file_index = file_index;
	_param._start_pos = start_pos;
	_param._length = len;
	_param._buffer = buf;
	_param._blocktime = block_time;
       	
	return tm_post_function(vdm_vod_read_file_handle, (void *)&_param,&(_param._handle),&(_param._result));
#else
	return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_VOD */
}
ETDLL_API int32 et_stop_vod(int32 task_id,  int32 file_index )
{
#ifdef ENABLE_VOD
	VOD_DATA_MANAGER_STOP _param;
	int32 ret;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("vdm_stop_vod_handle");

	CHECK_CRITICAL_ERROR;

	if(task_id==0) return TM_ERR_INVALID_TASK_ID;
	
	sd_memset(&_param,0,sizeof(VOD_DATA_MANAGER_STOP));
	_param._task_id = task_id;
	_param._file_index= file_index;
	
	ret = tm_post_function(vdm_stop_vod_handle, (void *)&_param,&(_param._handle),&(_param._result));
	return ret;
	
#else
	return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_VOD */
}


ETDLL_API int32 et_vod_set_buffer_time(int32 buffer_time )
{
#ifdef ENABLE_VOD	
	if (!g_already_init)
	{
	    return -1;
	}
	
	LOG_DEBUG("et_vod_set_buffer_time");

	CHECK_CRITICAL_ERROR;
	VOD_DATA_MANAGER_SET_BUFFER_TIME *p_param = NULL;
	sd_malloc(sizeof(VOD_DATA_MANAGER_SET_BUFFER_TIME), (void**)&p_param);
	sd_memset(p_param, 0, sizeof(VOD_DATA_MANAGER_SET_BUFFER_TIME));
	p_param->_buffer_time = buffer_time;
	
	return tm_post_function_unblock(vdm_set_vod_buffer_time_handle, (void *)p_param, &(p_param->_result));
#else
	return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_VOD */
}


ETDLL_API int32 et_vod_get_buffer_percent(int32 task_id,  int32* percent )
{
#ifdef ENABLE_VOD
	VOD_DATA_MANAGER_GET_BUFFER_PERCENT _param;
	int32 ret;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_vod_get_buffer_percent");

	CHECK_CRITICAL_ERROR;

	if(task_id==0) return TM_ERR_INVALID_TASK_ID;
	
	if(percent==NULL) return TM_ERR_INVALID_PARAMETER;
	
	sd_memset(&_param,0,sizeof(VOD_DATA_MANAGER_GET_BUFFER_PERCENT));
	_param._task_id = task_id;
	
	ret = tm_post_function(vdm_get_vod_buffer_percent_handle, (void *)&_param,&(_param._handle),&(_param._result));
	*percent = _param._percent;
	return ret;
	
#else
	return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_VOD */
}

ETDLL_API int32 et_vod_get_download_position(int32 task_id,  uint64* position )
{
#ifdef ENABLE_VOD
	VOD_DATA_MANAGER_GET_DOWNLOAD_POSITION _param;
	int32 ret;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_vod_get_download_position");

	CHECK_CRITICAL_ERROR;

	if(task_id==0) return TM_ERR_INVALID_TASK_ID;
	
	if(position==NULL) return TM_ERR_INVALID_PARAMETER;
	
	sd_memset(&_param,0,sizeof(VOD_DATA_MANAGER_GET_DOWNLOAD_POSITION));
	_param._task_id = task_id;
	
	ret = tm_post_function(vdm_get_vod_download_position_handle, (void *)&_param,&(_param._handle),&(_param._result));
	*position = _param._position;
	return ret;
	
#else
	return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_VOD */
}

ETDLL_API int32 et_vod_free_vod_buffer(void)
{
#ifdef ENABLE_VOD	
	if (!g_already_init)
	{
	    return -1;
	}
	
	LOG_DEBUG("et_vod_get_buffer_percent");

	CHECK_CRITICAL_ERROR;
	TM_POST_PARA_0 *p_param = NULL;
	sd_malloc(sizeof(TM_POST_PARA_0), (void**)&p_param);	
	sd_memset(p_param, 0, sizeof(TM_POST_PARA_0));
	
	return tm_post_function_unblock(vdm_free_vod_buffer_handle, (void *)p_param, &(p_param->_result));
#else
	return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_VOD */
}

ETDLL_API int32 et_vod_get_vod_buffer_size(_int32* buffer_size)
{
#ifdef ENABLE_VOD
       VOD_DATA_MANAGER_GET_SET_VOD_BUFFER_SIZE _param;
	int32 ret;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_vod_get_vod_buffer_size");

	CHECK_CRITICAL_ERROR;

	
	sd_memset(&_param,0,sizeof(VOD_DATA_MANAGER_GET_SET_VOD_BUFFER_SIZE));
	
	ret = tm_post_function(vdm_get_vod_buffer_size_handle, (void *)&_param,&(_param._handle),&(_param._result));
	*buffer_size= _param._buffer_size;
	return ret;
#else
	return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_VOD */
}


ETDLL_API int32 et_vod_set_vod_buffer_size(_int32 buffer_size)
{
#ifdef ENABLE_VOD	
	if (!g_already_init)
	{
	    return -1;
	}
	
	LOG_DEBUG("et_vod_set_vod_buffer_size");

	CHECK_CRITICAL_ERROR;

	VOD_DATA_MANAGER_GET_SET_VOD_BUFFER_SIZE *p_param = NULL;
	sd_malloc(sizeof(VOD_DATA_MANAGER_GET_SET_VOD_BUFFER_SIZE), (void**)&p_param);
	sd_memset(p_param, 0, sizeof(VOD_DATA_MANAGER_GET_SET_VOD_BUFFER_SIZE));
	p_param->_buffer_size = buffer_size;
	
	return tm_post_function_unblock(vdm_set_vod_buffer_size_handle, (void *)p_param, &(p_param->_result));
#else
	return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_VOD */
}

ETDLL_API int32 et_vod_is_vod_buffer_allocated(_int32* allocated)
{
#ifdef ENABLE_VOD
       VOD_DATA_MANAGER_IS_VOD_BUFFER_ALLOCATED _param;
	int32 ret;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_vod_is_vod_buffer_allocated");

	CHECK_CRITICAL_ERROR;

	
	sd_memset(&_param,0,sizeof(VOD_DATA_MANAGER_IS_VOD_BUFFER_ALLOCATED));
	
	ret = tm_post_function(vdm_is_vod_buffer_allocated_handle, (void *)&_param,&(_param._handle),&(_param._result));
	*allocated = _param._allocated;
	return ret;
#else
	return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_VOD */
}

ETDLL_API int32 et_vod_get_bitrate(int32 task_id, u32 file_index, u32* bitrate)
{
#ifdef ENABLE_VOD
	VOD_DATA_MANAGER_GET_BITRATE _param;

	int32 ret;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_vod_get_bitrate");

	CHECK_CRITICAL_ERROR;

	if(task_id==0) return TM_ERR_INVALID_TASK_ID;
	
	if(bitrate==NULL) return TM_ERR_INVALID_PARAMETER;
	
	sd_memset(&_param,0,sizeof(VOD_DATA_MANAGER_GET_BITRATE));
	_param._task_id = task_id;
	_param._file_index = file_index;
	
	ret = tm_post_function(vdm_get_vod_bitrate_handle, (void *)&_param,&(_param._handle),&(_param._result));
	*bitrate = _param._bitrate;
	return ret;
	
#else
	return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_VOD */

}

ETDLL_API int32 et_vod_is_download_finished(int32 task_id, BOOL* finished )
{
#ifdef ENABLE_VOD
	VOD_DATA_MANAGER_IS_DOWNLOAD_FINISHED  _param;

        int32  ret_val = 0;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_vod_is_download_finished");

	CHECK_CRITICAL_ERROR;

	if(task_id==0) return TM_ERR_INVALID_TASK_ID;
	
	
	sd_memset(&_param,0,sizeof(VOD_DATA_MANAGER_IS_DOWNLOAD_FINISHED));
	_param._task_id = task_id;
	_param._finished = FALSE;
      	
	ret_val = tm_post_function(vdm_vod_is_download_finished_handle, (void *)&_param,&(_param._handle),&(_param._result));
	*finished = _param._finished;
       return ret_val;
#else
	return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_VOD */
}
ETDLL_API int32 et_vod_report(_u32 task_id, ET_VOD_REPORT* p_report)
{
#ifdef ENABLE_VOD
	TM_POST_PARA_2 _param;

	int32 ret;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_vod_report");

	CHECK_CRITICAL_ERROR;

	if(task_id==0) return TM_ERR_INVALID_TASK_ID;
	
	if(p_report==NULL) return TM_ERR_INVALID_PARAMETER;
	
	sd_memset(&_param,0,sizeof(TM_POST_PARA_2));
	_param._para1= (void*) task_id;
	_param._para2= p_report;
	
	ret = tm_post_function(vdm_vod_report_handle, (void *)&_param,&(_param._handle),&(_param._result));
	return ret;
	
#else
	return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_VOD */

}

#if 0
ETDLL_API int32 et_reporter_set_version(char * ui_version,  int32 product,int32 partner_id)
{
#if defined(MOBILE_PHONE)
	TM_POST_PARA_3 _param;

	int32 ret;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_reporter_set_version");

	CHECK_CRITICAL_ERROR;
	
	if(ui_version==NULL) return TM_ERR_INVALID_PARAMETER;
	
	sd_memset(&_param,0,sizeof(TM_POST_PARA_3));
	_param._para1= (void*)ui_version;
	_param._para2= (void*)product;
	_param._para3= (void*)partner_id;
	
	ret =  tm_post_function(reporter_set_version_handle, (void *)&_param,&(_param._handle),&(_param._result));
	return ret;
#else
	return NOT_IMPLEMENT;
#endif /* #ifdef MOBILE_PHONE */
}

ETDLL_API int32 et_reporter_new_install(BOOL is_install)
{
#if defined(MOBILE_PHONE)
	TM_POST_PARA_1 _param;

	int32 ret;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_reporter_set_login_type");

	CHECK_CRITICAL_ERROR;
	
	sd_memset(&_param,0,sizeof(TM_POST_PARA_1));
	_param._para1= (void*)is_install;
	
	ret =  tm_post_function(reporter_new_install_handle, (void *)&_param,&(_param._handle),&(_param._result));
	return ret;
#else
	return NOT_IMPLEMENT;
#endif /* #ifdef MOBILE_PHONE */
}

ETDLL_API int32 et_reporter_mobile_user_action_to_file(u32 action_type, u32 action_value, void* data, u32 data_len)
{
#if defined(MOBILE_PHONE)
	TM_POST_PARA_4 _param;

	int32 ret;

	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_reporter_mobile_user_action_to_file");

	CHECK_CRITICAL_ERROR;

	sd_memset(&_param,0,sizeof(TM_POST_PARA_4));
	_param._para1 = (void*)action_type;
	_param._para2 = (void*)action_value;
	_param._para3 = data;
	_param._para4 = (void*)data_len;

	ret = tm_post_function(reporter_mobile_user_action_to_file_handle, (void *)&_param,&(_param._handle),&(_param._result));
	return ret;
#else
	return NOT_IMPLEMENT;
#endif /* #ifdef MOBILE_PHONE */
}

ETDLL_API int32 et_reporter_enable_user_action_report(BOOL is_enable)
{
#if defined(MOBILE_PHONE)
	TM_POST_PARA_1 _param;

	int32 ret;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_reporter_enable_user_action_report");

	CHECK_CRITICAL_ERROR;
	
	sd_memset(&_param,0,sizeof(TM_POST_PARA_1));
	_param._para1= (void*)is_enable;
	
	ret =  tm_post_function(reporter_mobile_enable_user_action_report_handle, (void *)&_param,&(_param._handle),&(_param._result));
	return ret;
#else
	return NOT_IMPLEMENT;
#endif /* #ifdef MOBILE_PHONE */
}
#else
ETDLL_API int32 et_reporter_set_version(char * ui_version,  int32 product,int32 partner_id)
{

}

ETDLL_API int32 et_reporter_new_install(BOOL is_install)
{

}

ETDLL_API int32 et_reporter_mobile_user_action_to_file(u32 action_type, u32 action_value, void* data, u32 data_len)
{

}

ETDLL_API int32 et_reporter_enable_user_action_report(BOOL is_enable)
{

}

#endif

ETDLL_API int32 et_set_log_conf_path(const char* path)
{
        int32  ret_val = 0;
	
	if(g_already_init)
		return -1;
	
	LOG_DEBUG("et_set_log_conf_path");

      sd_memset(g_log_conf_file_path, 0, sizeof(g_log_conf_file_path));
	sd_strncpy(g_log_conf_file_path, path, sizeof(g_log_conf_file_path)-1);

       return ret_val;
}
	

#if defined(LINUX)
ETDLL_API int32 et_get_task_ids(int32* p_task_count, int32 task_array_size, char* task_array)
{
     return sd_get_task_ids(p_task_count, task_array_size,  task_array);
}
#else
ETDLL_API int32 et_get_task_ids(int32* p_task_count, int32 task_array_size, char* task_array)
{
     return NOT_IMPLEMENT;
}
#endif

#ifdef _CONNECT_DETAIL
/* 迅雷公司内部调试用  */
ETDLL_API int32 et_get_upload_pipe_info(ET_PEER_PIPE_INFO_ARRAY * p_upload_pipe_info_array)
{
#ifdef UPLOAD_ENABLE
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_get_upload_pipe_info");

	CHECK_CRITICAL_ERROR;

	if(!p_upload_pipe_info_array) return TM_ERR_INVALID_PARAMETER;

	return ulm_get_pipe_info_array((PEER_PIPE_INFO_ARRAY *)p_upload_pipe_info_array);

#else
	return NOT_IMPLEMENT;
#endif /* #ifdef UPLOAD_ENABLE */
}
#endif /*  _CONNECT_DETAIL  */



ETDLL_API int32 et_create_cmd_proxy( u32 ip, u16 port, u32 attribute, u32 *p_cmd_proxy_id )
{
	PM_CREATE_CMD_PROXY param;
	if(!g_already_init)
		return -1;
	CHECK_CRITICAL_ERROR;
    
	sd_memset(&param,0,sizeof(PM_CREATE_CMD_PROXY));
    param._ip = ip;
    param._port = port;
    param._attribute = attribute;
    param._cmd_proxy_id_ptr = p_cmd_proxy_id;

	return tm_post_function(pm_create_cmd_proxy, (void *)&param,&(param._handle),&(param._result));
}

ETDLL_API int32 et_create_cmd_proxy_by_domain( const char* host, u16 port, u32 attribute, u32 *p_cmd_proxy_id )
{
	PM_CREATE_CMD_PROXY_BY_DOMAIN param;
	if(!g_already_init)
		return -1;
	CHECK_CRITICAL_ERROR;
    
	sd_memset(&param,0,sizeof(PM_CREATE_CMD_PROXY_BY_DOMAIN));
    param._host = host;
    param._port = port;
    param._attribute = attribute;
    param._cmd_proxy_id_ptr = p_cmd_proxy_id;

	return tm_post_function(pm_create_cmd_proxy_by_domain, (void *)&param,&(param._handle),&(param._result));
}

ETDLL_API int32 et_cmd_proxy_get_info( u32 cmd_proxy_id, int32 *p_errcode, u32 *p_recv_cmd_id, u32 *p_data_len )
{
	PM_CMD_PROXY_GET_INFO param;
	if(!g_already_init)
		return -1;
	CHECK_CRITICAL_ERROR;
    
	sd_memset(&param,0,sizeof(PM_CMD_PROXY_GET_INFO));
    param._cmd_proxy_id = cmd_proxy_id;
    param._errcode = p_errcode;
    param._recv_cmd_id = p_recv_cmd_id;
    param._data_len = p_data_len;

	return tm_post_function(pm_cmd_proxy_get_info, (void *)&param,&(param._handle),&(param._result));
}

ETDLL_API int32 et_cmd_proxy_send( u32 cmd_proxy_id, const char* buffer, u32 _len )
{
    PM_CMD_PROXY_SEND param;
    if(!g_already_init)
        return -1;
    CHECK_CRITICAL_ERROR;
    
    sd_memset(&param,0,sizeof(PM_CMD_PROXY_SEND));
    param._cmd_proxy_id = cmd_proxy_id;
    param._buffer = buffer;
    param._len = _len;

    return tm_post_function(pm_cmd_proxy_send, (void *)&param,&(param._handle),&(param._result));
}

ETDLL_API int32 et_cmd_proxy_get_recv_info( u32 cmd_proxy_id, u32 recv_cmd_id, char *data_buffer, u32 data_buffer_len )
{
    PM_CMD_PROXY_GET_RECV_INFO param;
    if(!g_already_init)
        return -1;
    CHECK_CRITICAL_ERROR;
    
    sd_memset(&param,0,sizeof(PM_CMD_PROXY_GET_RECV_INFO));
    param._cmd_proxy_id = cmd_proxy_id;
    param._recv_cmd_id = recv_cmd_id;
    param._data_buffer = data_buffer;
    param._data_buffer_len = data_buffer_len;
    
    return tm_post_function(pm_cmd_proxy_get_recv_info, (void *)&param,&(param._handle),&(param._result));
}

ETDLL_API int32 et_cmd_proxy_close( u32 cmd_proxy_id )
{
    PM_CMD_PROXY_CLOSE param;
    if(!g_already_init)
        return -1;
    CHECK_CRITICAL_ERROR;
    
    sd_memset(&param,0,sizeof(PM_CMD_PROXY_CLOSE));
    param._cmd_proxy_id = cmd_proxy_id;

    return tm_post_function(pm_cmd_proxy_close, (void *)&param,&(param._handle),&(param._result));
}


ETDLL_API int32 et_get_peerid( char *buffer, u32 buffer_size )
{
    PM_GET_PEER_ID param;
    if(!g_already_init)
        return -1;
    CHECK_CRITICAL_ERROR;
    
    sd_memset(&param,0,sizeof(PM_GET_PEER_ID));
    param._buffer = buffer;
    param._buffer_len = buffer_size;

    return tm_post_function(pm_get_peer_id, (void *)&param,&(param._handle),&(param._result));

}

ETDLL_API int32 et_get_free_disk( const char *path, uint64 *free_size )
{
    return sd_get_free_disk(path, free_size);
}
ETDLL_API int32 et_extract_ed2k_url(char* ed2k, ET_ED2K_LINK_INFO* info)
{	
#ifdef ENABLE_EMULE
	int32 ret = 0,len=0;
	ED2K_LINK_INFO ed2k_info;
	sd_memset(info, 0, sizeof(ET_ED2K_LINK_INFO));
	ret = emule_extract_ed2k_link(ed2k, &ed2k_info);
	if(ret != 0)
		return ret;
	len = sd_strlen(ed2k_info._file_name);
	sd_memcpy(info->_file_name, ed2k_info._file_name, len<256?len:255);
	info->_file_size = ed2k_info._file_size;
	sd_memcpy(info->_file_id, ed2k_info._file_id, 16);
	sd_memcpy(info->_root_hash, ed2k_info._root_hash, 20);
	sd_memcpy(info->_http_url, ed2k_info._http_url, 512);
	return ret;
#else
	return NOT_IMPLEMENT;
#endif

}

ETDLL_API int32 et_http_get(ET_HTTP_PARAM * param, _u32 *http_id)
{
    if (!g_already_init)
    {
        return -1;
    }
	
	LOG_DEBUG("et_http_get");

	CHECK_CRITICAL_ERROR;
	if ((param->_url == NULL) || (param->_url_len == 0) || (param->_callback_fun == NULL) || (http_id == NULL)) 
	{
	    return TM_ERR_INVALID_PARAMETER;
	}
	
	TM_MINI_HTTP _param;
	sd_memset(&_param, 0, sizeof(TM_MINI_HTTP));
	_param._param= (void*)param;
	_param._id = http_id;
	param->_priority = -1; // download first
	
	return tm_post_function(tm_http_get, (void *)&_param, &(_param._handle), &(_param._result));
}

ETDLL_API int32 et_http_post(ET_HTTP_PARAM * param,_u32 * http_id)
{	
	TM_MINI_HTTP  _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_http_post");

	CHECK_CRITICAL_ERROR;

	if((param->_url==NULL)||(param->_url_len==0)||(param->_callback_fun==NULL)||(http_id==NULL)) return TM_ERR_INVALID_PARAMETER;
	
	sd_memset(&_param,0,sizeof(TM_MINI_HTTP));
	_param._param= (void*)param;
	_param._id = http_id;
	param->_priority = -1; //download first
	
	return tm_post_function(tm_http_post, (void *)&_param,&(_param._handle),&(_param._result));
}
ETDLL_API int32 et_http_close(_u32 http_id)
{	
	TM_MINI_HTTP_CLOSE  _param;
	LOG_DEBUG("et_http_close-begin");
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_http_close-2");

	CHECK_CRITICAL_ERROR;

	if(http_id==0) return TM_ERR_INVALID_PARAMETER;
	
	sd_memset(&_param,0,sizeof(TM_MINI_HTTP_CLOSE));
	_param._id = http_id;
       	
	return tm_post_function(tm_http_close, (void *)&_param,&(_param._handle),&(_param._result));
}

ETDLL_API int32 et_http_cancel(_u32 http_id)
{	
	TM_MINI_HTTP_CLOSE  _param;
	LOG_DEBUG("et_http_cancel-begin");
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_http_cancel-2");

	CHECK_CRITICAL_ERROR;

	if(http_id==0) return TM_ERR_INVALID_PARAMETER;
	
	sd_memset(&_param,0,sizeof(TM_MINI_HTTP_CLOSE));
	_param._id = http_id;

		
	return tm_post_function(tm_http_cancel, (void *)&_param,&(_param._handle),&(_param._result));
}


///// 获取http头的字段值
#include "http_data_pipe/http_mini.h"
ETDLL_API char * et_get_http_header_value(void * p_header,ET_HTTP_HEADER_FIELD header_field )
{
	static char sbuffer[32];
	static char * ret_char = NULL;
	HTTP_MINI_HEADER * p_http_header = (HTTP_MINI_HEADER *)p_header;
	switch(header_field)
	{
		case EHV_STATUS_CODE:
			sd_memset(&sbuffer,0x00,32);
                	sd_u32toa(p_http_header->_status_code, sbuffer, 31, 10);
			ret_char = sbuffer;	
		break;
		case EHV_LAST_MODIFY_TIME:
			ret_char = p_http_header->_last_modified;
		break;
		case EHV_COOKIE:
			ret_char = p_http_header->_cookie;
		break;
		case EHV_CONTENT_ENCODING:
			ret_char = p_http_header->_content_encoding;
		break;
		case EHV_CONTENT_LENGTH:
			sd_memset(&sbuffer,0x00,32);
                	sd_u64toa(p_http_header->_content_length, sbuffer, 31, 10);
			ret_char = sbuffer;	
		break;
		default:
			ret_char = NULL;
	}

	if(sd_strlen(ret_char)!=0)
		return ret_char;
	else
		return NULL;
}


ETDLL_API int32 et_is_drm_enable( BOOL *p_is_enable )
{
#ifdef ENABLE_DRM 
    *p_is_enable = TRUE;
#else
    *p_is_enable = FALSE;
#endif /* #ifdef ENABLE_DRM */
    return SUCCESS;
}

ETDLL_API int32 et_set_certificate_path( const char *certificate_path )
{
#ifdef ENABLE_DRM 
    TM_SET_CERTIFICATE_PATH param;
    if(!g_already_init)
        return -1;
    CHECK_CRITICAL_ERROR;
    
    sd_memset(&param,0,sizeof(TM_SET_CERTIFICATE_PATH));
    param._certificate_path_ptr = certificate_path;

    return tm_post_function(tm_set_certificate_path, (void *)&param,&(param._handle),&(param._result));
#else
    return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_DRM */

}

ETDLL_API int32 et_open_drm_file( const char *p_file_full_path, 
    u32 *p_drm_id, uint64 *p_origin_file_size )
{
#ifdef ENABLE_DRM 
    TM_OPEN_DRM_FILE param;
    if(!g_already_init)
        return -1;
    CHECK_CRITICAL_ERROR;
    
    sd_memset(&param,0,sizeof(TM_OPEN_DRM_FILE));
    param._file_full_path_ptr = p_file_full_path;
    param._drm_id_ptr = p_drm_id;
    param._origin_file_size_ptr = p_origin_file_size;

    return tm_post_function(tm_open_drm_file, (void *)&param,&(param._handle),&(param._result));
#else
    return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_DRM */

}

ETDLL_API int32 et_is_certificate_ok( u32 drm_id, BOOL *p_is_ok )
{
#ifdef ENABLE_DRM 

    TM_IS_CERTIFICATE_OK param;
    if(!g_already_init)
        return -1;
    CHECK_CRITICAL_ERROR;
    
    sd_memset(&param,0,sizeof(TM_IS_CERTIFICATE_OK));
    param._drm_id = drm_id;
    param._is_ok = p_is_ok;

    return tm_post_function(tm_is_certificate_ok, (void *)&param,&(param._handle),&(param._result));
#else
    return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_DRM */
}


ETDLL_API int32 et_read_drm_file( u32 drm_id, char *p_buffer, u32 size, 
    uint64 file_pos, u32 *p_read_size )
{
#ifdef ENABLE_DRM 
    TM_READ_DRM_FILE param;
    if(!g_already_init)
        return -1;
    CHECK_CRITICAL_ERROR;
    
    sd_memset(&param,0,sizeof(TM_READ_DRM_FILE));
    param._drm_id = drm_id;
    param._buffer_ptr = p_buffer;
    param._size = size;
    param._file_pos = file_pos;
    param._read_size_ptr = p_read_size;

    return drm_read_file( drm_id, p_buffer, size, file_pos, p_read_size );
   // return tm_post_function(tm_read_drm_file, (void *)&param,&(param._handle),&(param._result));
#else
    return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_DRM */
}

ETDLL_API int32 et_close_drm_file( u32 drm_id )
{
#ifdef ENABLE_DRM 
    TM_CLOSE_DRM_FILE param;
    if(!g_already_init)
        return -1;
    CHECK_CRITICAL_ERROR;
    
    sd_memset(&param,0,sizeof(TM_CLOSE_DRM_FILE));
    param._drm_id = drm_id;

    return tm_post_function(tm_close_drm_file, (void *)&param,&(param._handle),&(param._result));
#else
    return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_DRM */

}

ETDLL_API int32 et_set_openssl_rsa_interface(int32 fun_count, void *fun_ptr_table)
{
#ifdef ENABLE_DRM 
		if(g_already_init)
			return ALREADY_ET_INIT;
		
		return set_openssli_rsa_interface(fun_count, fun_ptr_table);
#else
		return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_DRM */
}


ETDLL_API int32 et_high_speed_channel_switch(u32 task_id, BOOL sw, BOOL has_pay, const char* task_name, const u32 name_length)
{
#if defined(ENABLE_HSC) && !defined(ENABLE_ETM_MEDIA_CENTER)
	HSC_SWITCH _param;
	switch(sw)
	{
		case TRUE:
			sd_memset(&_param, 0, sizeof(_param));
			_param._task_id = task_id;
			_param._hsc_has_pay = has_pay;
            _param._hsc_task_name = (char*)task_name;
            _param._hsc_task_name_length = name_length;
			return tm_post_function(hsc_enable, (void*)&_param, &(_param._handle), &(_param._result));
		case FALSE:
			sd_memset(&_param, 0, sizeof(_param));
			_param._task_id = task_id;
			return tm_post_function(hsc_disable, (void*)&_param, &(_param._handle), &(_param._result));
	}
#endif
	return NOT_IMPLEMENT;
}

ETDLL_API int32 et_hsc_query_flux_info(void* callback)
{
#if defined(ENABLE_HSC) && !defined(ENABLE_ETM_MEDIA_CENTER)
	HSC_QUERY_FLUX_INFO _param;
    sd_memset(&_param, 0, sizeof(_param));
	_param._p_callback = callback;
	return tm_post_function(hsc_query_flux_info, (void*)&_param, &(_param._handle), &(_param._result));
#endif
	return NOT_IMPLEMENT;
}

ETDLL_API int32 et_get_hsc_info(u32 task_id, ET_HIGH_SPEED_CHANNEL_INFO *_p_hsc_info)
{
#ifdef ENABLE_HSC
	if (!g_already_init)
	{
	    return -1;
	}
	
	if (_p_hsc_info == NULL)
	{
	    return INVALID_MEMORY;
	}
	if (0 == task_id)
	{
	    return TM_ERR_INVALID_TASK_ID;
	}	
	
	LOG_DEBUG("et_get_hsc_info,task_id=%u", task_id);

	CHECK_CRITICAL_ERROR;

	TM_GET_TASK_HSC_INFO _param;
	sd_memset(&_param, 0, sizeof(_param));
	_param._task_id = task_id;
	_param._info = _p_hsc_info;

	return tm_post_function(tm_get_task_hsc_info, (void*)&_param, &(_param._handle), &(_param._result));
#endif
	return NOT_IMPLEMENT;
}

ETDLL_API int32 et_set_product_sub_id(u32 task_id, uint64 product_id, uint64 sub_id)
{
#if defined(ENABLE_HSC) && !defined(ENABLE_ETM_MEDIA_CENTER)
	HSC_PRODUCT_SUB_ID_INFO _param;
	sd_memset(&_param, 0, sizeof(_param));
	_param._task_id = task_id;
	_param._product_id = product_id;
	_param._sub_id = sub_id;

	return tm_post_function(hsc_set_product_sub_id, (void*)&_param, &(_param._handle), &(_param._result));
#endif
		return NOT_IMPLEMENT;
}

ETDLL_API int32 et_set_hsc_business_flag(const u32 flag, const u32 client_ver)
{
#if defined(ENABLE_HSC) && !defined(ENABLE_ETM_MEDIA_CENTER)
    HSC_BUSINESS_FLAG _param;
    _param._flag = flag;
	_param._client_ver = client_ver;
    return tm_post_function(hsc_set_business_flag, (void*)&_param, &(_param._handle), &(_param._result));
#endif
    return NOT_IMPLEMENT;
}

ETDLL_API int32 et_auto_hsc_sw(BOOL sw)
{
#if defined(ENABLE_HSC) && !defined(ENABLE_ETM_MEDIA_CENTER)
	HSC_AUTO_SWITCH _param;
	sd_memset(&_param, 0, sizeof(_param));
	_param._enable = sw;
	return tm_post_function(hsc_auto_sw, (void*)&_param, &(_param._handle), &(_param._result));
#endif
	return NOT_IMPLEMENT;
}

ETDLL_API int32 et_set_used_hsc_before(u32 task_id, BOOL used)
{
#if defined(ENABLE_HSC) && !defined(ENABLE_ETM_MEDIA_CENTER)
	HSC_USED_BEFORE_FLAG _param;
	sd_memset(&_param, 0, sizeof(_param));
	_param._task_id = task_id;
	_param._used_before = used;
	return tm_post_function(hsc_set_used_hsc_before, (void*)&_param, &(_param._handle), &(_param._result));
#endif
	return NOT_IMPLEMENT;
}

ETDLL_API int32 et_get_hsc_is_auto(BOOL *p_is_auto)
{
#if defined(ENABLE_HSC) && !defined(ENABLE_ETM_MEDIA_CENTER)
	HSC_AUTO_INFO _param;
	if(p_is_auto == NULL)
		return INVALID_MEMORY;
	sd_memset(&_param, 0, sizeof(_param));
	_param._p_is_auto = p_is_auto;
	return tm_post_function(hsc_is_auto, (void*)&_param, &(_param._handle), &(_param._result));
#endif
	return NOT_IMPLEMENT;
}

ETDLL_API int32 et_get_lixian_info(u32 task_id,u32 file_index, ET_LIXIAN_INFO * p_lx_info)
{
#ifdef ENABLE_LIXIAN
	_int32 ret_val = SUCCESS;
	TM_POST_PARA_3 param;

	if(task_id==0||p_lx_info==NULL) return INVALID_ARGUMENT;

	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_get_lixian_info,task_id=%u",task_id);

	CHECK_CRITICAL_ERROR;

	ret_val =  tm_get_lixian_info(task_id,file_index,(void*)p_lx_info);
	if(ret_val != SUCCESS)
	{
		sd_memset(&param, 0, sizeof(param));
		param._para1= (void*)task_id;
		param._para2 = (void*)file_index;
		param._para3 = (void*)p_lx_info;
	
		ret_val =tm_post_function(tm_get_bt_lixian_info, (void *)&param,&(param._handle),&(param._result));
	}
	return ret_val;
#else
	return NOT_IMPLEMENT;
#endif
}

//开始搜索
ETDLL_API int32 et_start_search_server(ET_SEARCH_SERVER * p_search)
{
	TM_POST_PARA_1 param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_start_search_server");
	
	CHECK_CRITICAL_ERROR;

	sd_memset(&param,0,sizeof(param));
	param._para1= (void*)p_search;
	
	return tm_post_function(http_server_start_search_server, (void *)&param,&(param._handle),&(param._result));
}

//停止搜索,如果该接口在ET回调_search_finished_callback_fun之前调用，则ET不会再回调_search_finished_callback_fun
ETDLL_API int32 et_stop_search_server(void)
{
	TM_POST_PARA_0 param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_stop_search_server");
	
	CHECK_CRITICAL_ERROR;

	sd_memset(&param,0,sizeof(param));
	
	return tm_post_function(http_server_stop_search_server, (void *)&param,&(param._handle),&(param._result));
}

ETDLL_API int32 et_restart_search_server(void)
{
	TM_POST_PARA_0 param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_restart_search_server");
	
	CHECK_CRITICAL_ERROR;

	sd_memset(&param,0,sizeof(param));
	
	return tm_post_function(http_server_restart_search_server, (void *)&param,&(param._handle),&(param._result));
}


//////////////////////////////////////////////////////////////
/////17 手机助手相关的http server

/* 启动和关闭http server */
ETDLL_API int32 et_start_http_server(u16 * port)
{
#if defined(ENABLE_VOD) || defined(ENABLE_ASSISTANT) || defined(ENABLE_HTTP_SERVER)
	
	if (!g_already_init)
	{
	    return -1;
	}
	
	LOG_DEBUG("start_http_server:port=%u",*port);
	CHECK_CRITICAL_ERROR;
	HTTP_SERVER_START _param;
	sd_memset(&_param, 0, sizeof(HTTP_SERVER_START));
	_param._port = port;
	return tm_post_function(tm_start_http_server, (void *)&_param, &(_param._handle), &(_param._result));
#else
	return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_VOD || ENABLE_ASSISTANT || ENABLE_HTTP_SERVER*/
}

ETDLL_API int32 et_stop_http_server(void)
{
#if defined( ENABLE_VOD)||defined(ENABLE_ASSISTANT) || defined(ENABLE_HTTP_SERVER)	
	if (!g_already_init)
	{
	    return -1;
	}
	LOG_DEBUG("stop http server");
	CHECK_CRITICAL_ERROR;
	HTTP_SERVER_STOP _param;
	sd_memset(&_param,0,sizeof(HTTP_SERVER_STOP));
	return tm_post_function(tm_stop_http_server, (void *)&_param,&(_param._handle),&(_param._result));
#else
	return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_VOD || ENABLE_ASSISTANT || ENABLE_HTTP_SERVER*/
}

ETDLL_API int32 et_http_server_add_account(char *username, char *password)
{
#ifdef ENABLE_HTTP_SERVER
	TM_POST_PARA_2 _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_http_server_add_account");

	CHECK_CRITICAL_ERROR;

	sd_memset(&_param,0,sizeof(_param));
	_param._para1 = (void*)username;
	_param._para2 = (void*)password;
	
	return tm_post_function(http_server_add_account_handle, (void *)&_param,&(_param._handle),&(_param._result));
#else
	return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_HTTP_SERVER*/
}

ETDLL_API int32 et_http_server_add_path(char *real_path, char *virtual_path, BOOL need_auth)
{
#ifdef ENABLE_HTTP_SERVER
	TM_POST_PARA_3 _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("stop http server");

	CHECK_CRITICAL_ERROR;

	sd_memset(&_param,0,sizeof(_param));
	_param._para1 = (void*)real_path;
	_param._para2 = (void*)virtual_path;
	_param._para3 = (void*)need_auth;

	return tm_post_function(http_server_add_path_handle, (void *)&_param,&(_param._handle),&(_param._result));
#else
	return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_HTTP_SERVER*/
}

ETDLL_API int32 et_http_server_get_path_list(const char * virtual_path,char ** path_array_buffer,u32 * path_num)
{
#ifdef ENABLE_HTTP_SERVER
	TM_POST_PARA_3 _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("stop http server");

	CHECK_CRITICAL_ERROR;

	sd_memset(&_param,0,sizeof(_param));
	_param._para1 = (void*)virtual_path;
	_param._para2 = (void*)path_array_buffer;
	_param._para3 = (void*)path_num;
	
	return tm_post_function(http_server_get_path_list_handle, (void *)&_param,&(_param._handle),&(_param._result));
#else
	return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_HTTP_SERVER*/
}

ETDLL_API int32 et_http_server_set_callback_func(void *callback)//FB_HTTP_SERVER_CALLBACK callback)
{
#ifdef ENABLE_HTTP_SERVER
	TM_POST_PARA_1 _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_http_server_set_callback_func");

	CHECK_CRITICAL_ERROR;

	sd_memset(&_param,0,sizeof(_param));
	_param._para1 = (void*)callback;
	
	return tm_post_function(http_server_set_callback_func_handle, (void *)&_param,&(_param._handle),&(_param._result));
#else
	return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_HTTP_SERVER*/
}

ETDLL_API int32 et_http_server_send_resp(void *p_param)//FB_HS_AC * p_param)
{
#ifdef ENABLE_HTTP_SERVER
	TM_POST_PARA_1 _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_http_server_set_callback_func");

	CHECK_CRITICAL_ERROR;

	sd_memset(&_param,0,sizeof(_param));
	_param._para1 = (void*)p_param;
	
	return tm_post_function(http_server_send_resp_handle, (void *)&_param,&(_param._handle),&(_param._result));
#else
	return NOT_IMPLEMENT;
#endif /*#ifdef ENABLE_HTTP_SERVER*/
}

ETDLL_API int32 et_http_server_cancel(u32 action_id)
{
#ifdef ENABLE_HTTP_SERVER
	TM_POST_PARA_1 _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("stop http server");

	CHECK_CRITICAL_ERROR;

	sd_memset(&_param,0,sizeof(_param));
	_param._para1 = (void*)action_id;
	
	return tm_post_function(http_server_cancel_handle, (void *)&_param,&(_param._handle),&(_param._result));
#else
	return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_HTTP_SERVER*/
}

/* 网下载库设置回调函数 */
ETDLL_API int32 et_assistant_set_callback_func(ET_ASSISTANT_INCOM_REQ_CALLBACK req_callback,ET_ASSISTANT_SEND_RESP_CALLBACK resp_callback)
{
#ifdef ENABLE_ASSISTANT
	TM_POST_PARA_2 _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_assistant_set_callback_func");
	
	CHECK_CRITICAL_ERROR;

	sd_memset(&_param,0,sizeof(_param));
	_param._para1 = (void*)req_callback;
	_param._para2 = (void*)resp_callback;
	
	return tm_post_function(assistant_set_callback_func, (void *)&_param,&(_param._handle),&(_param._result));
#else
	return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_ASSISTANT */
}

/* 发送响应 */
ETDLL_API int32 et_assistant_send_resp_file(u32 ma_id,const char * resp_file,void * user_data)
{
#ifdef ENABLE_ASSISTANT
	TM_POST_PARA_3 _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_assistant_send_resp_file:ma_id=%u",ma_id);
	
	CHECK_CRITICAL_ERROR;

	sd_memset(&_param,0,sizeof(_param));
	_param._para1 = (void*)ma_id;
	_param._para2 = (void*)resp_file;
	_param._para3 = (void*)user_data;
	
	return tm_post_function(assistant_send_resp_file, (void *)&_param,&(_param._handle),&(_param._result));
#else
	return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_ASSISTANT */
}

/* 取消异步操作 */
ETDLL_API int32 et_assistant_cancel(u32 ma_id)
{
#ifdef ENABLE_ASSISTANT
	TM_POST_PARA_1 _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_assistant_cancel:ma_id=%u",ma_id);
	
	CHECK_CRITICAL_ERROR;

	sd_memset(&_param,0,sizeof(_param));
	_param._para1 = (void*)ma_id;
	
	return tm_post_function(assistant_cancel, (void *)&_param,&(_param._handle),&(_param._result));
#else
	return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_ASSISTANT */
}

/* 设置一个可读写的路径，用于下载库存放临时数据*/
ETDLL_API int32 et_set_system_path(const char * system_path)
{
	TM_POST_PARA_1 _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_set_system_path:%s",system_path);
	
	CHECK_CRITICAL_ERROR;

	sd_memset(&_param,0,sizeof(_param));
	_param._para1 = (void*)system_path;
	
	return tm_post_function(tm_set_system_path, (void *)&_param,&(_param._handle),&(_param._result));
}

ETDLL_API int32 et_query_shub_by_url(ET_QUERY_SHUB * p_param,u32 * p_action_id)
{
	TM_POST_PARA_2 _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_query_shub_by_url:%s",p_param->_url);
	
	CHECK_CRITICAL_ERROR;

	sd_memset(&_param,0,sizeof(_param));
	_param._para1 = (void*)p_param;
	_param._para2 = (void*)p_action_id;	
	
	return tm_post_function(tm_query_shub_by_url, (void *)&_param,&(_param._handle),&(_param._result));
}

ETDLL_API int32 et_cancel_query_shub(u32 action_id)
{
	TM_POST_PARA_1 _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_cancel_query_shub:%X",action_id);
	
	CHECK_CRITICAL_ERROR;

	sd_memset(&_param,0,sizeof(_param));
	_param._para1 = (void*)action_id;
	
	return tm_post_function(tm_query_shub_cancel, (void *)&_param,&(_param._handle),&(_param._result));
}

ETDLL_API int32 et_set_origin_mode(u32 task_id, BOOL origin_mode)
{
	TM_POST_PARA_2 _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_set_origin_mode: origin_mode = %d",origin_mode);
	
	CHECK_CRITICAL_ERROR;

	sd_memset(&_param,0,sizeof(_param));
	_param._para1 = (void*)task_id;
	_param._para2 = (void*)origin_mode;
	
	return tm_post_function(tm_set_origin_mode, (void *)&_param,&(_param._handle),&(_param._result));
}

ETDLL_API int32 et_is_origin_mode(u32 task_id, BOOL* p_is_origin_mode)
{
	_int32 ret_val = SUCCESS;
	TM_POST_PARA_2 _param;

	if(!g_already_init)
		return FALSE;
		
	LOG_DEBUG("et_is_origin_mode:%d", task_id);

	CHECK_CRITICAL_ERROR;

	sd_memset(&_param,0,sizeof(_param));
	_param._para1= (void*)task_id;
	_param._para2 = (void*) p_is_origin_mode;

	ret_val = tm_post_function(tm_is_origin_mode, (void *)&_param,&(_param._handle),&(_param._result));

	return ret_val;
}

ETDLL_API int32 et_get_origin_resource_info(u32 task_id, void* p_resource)
{
	TM_POST_PARA_2 _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_get_origin_resource_info:%d",task_id);
	
	CHECK_CRITICAL_ERROR;

	sd_memset(&_param,0,sizeof(_param));
	_param._para1 = (void*)task_id;
	_param._para2 = (void*)p_resource;
	
	return tm_post_function(tm_get_origin_resource_info, (void *)&_param,&(_param._handle),&(_param._result));
}

ETDLL_API int32 et_get_origin_download_data(u32 task_id, uint64 *download_data_size)
{
	TM_POST_PARA_2 _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_get_origin_resource_info:%d",task_id);
	
	CHECK_CRITICAL_ERROR;

	sd_memset(&_param,0,sizeof(_param));
	_param._para1 = (void*)task_id;
	_param._para2 = (void*)download_data_size;
	
	return tm_post_function(tm_get_origin_download_data, (void *)&_param,&(_param._handle),&(_param._result));
}


ETDLL_API int32 et_add_assist_peer_resource(u32 task_id, void * p_resource)
{
	TM_ADD_TASK_RES _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_ERROR("et_add_task_resource,task_id=%u",task_id);

	CHECK_CRITICAL_ERROR;
 
	if(task_id==0) return TM_ERR_INVALID_TASK_ID;

	if(p_resource==NULL) return TM_ERR_INVALID_PARAMETER;

	sd_memset(&_param,0,sizeof(TM_ADD_TASK_RES));
	_param.task_id= task_id;
	_param._res= p_resource;
	
	return tm_post_function(tm_add_assist_peer_resource, (void *)&_param,&(_param._handle),&(_param._result));

}


ETDLL_API int32 et_get_assist_peer_info(u32 task_id,u32 file_index, void * p_lx_info)
{
	_int32 ret_val = SUCCESS;
	TM_POST_PARA_3 param;

	if(task_id==0||p_lx_info==NULL) return INVALID_ARGUMENT;

	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_get_assist_peer_info,task_id=%u",task_id);

	CHECK_CRITICAL_ERROR;

	sd_memset(&param, 0, sizeof(param));
	param._para1= (void*)task_id;
	param._para2 = (void*)file_index;
	param._para3 = (void*)p_lx_info;
	
	ret_val = tm_post_function( tm_get_assist_peer_info, (void *)&param,&(param._handle),&(param._result));
	
	return ret_val;
}

ETDLL_API int32 et_set_extern_id(u32 et_inner_id,u32 extern_id)
{
    if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_set_extern_id:%d",et_inner_id);
	
	CHECK_CRITICAL_ERROR;

	TM_POST_PARA_2 _param;
    sd_memset(&_param,0,sizeof(TM_POST_PARA_2));
	_param._para1 = (void*)et_inner_id;
	_param._para2 = (void*)extern_id;

    return tm_post_function(tm_set_extern_id, (void *)&_param,&(_param._handle),&(_param._result));

}

ETDLL_API int32 et_set_extern_info(u32 et_inner_id,u32 extern_id,u32 create_time)
{
    if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_set_extern_info:%d",et_inner_id);
	
	CHECK_CRITICAL_ERROR;

	TM_POST_PARA_3 _param;
    sd_memset(&_param,0,sizeof(TM_POST_PARA_3));
	_param._para1 = (void*)et_inner_id;
	_param._para2 = (void*)extern_id;
	_param._para3 = (void*)create_time;
	
    return tm_post_function(tm_set_extern_info, (void *)&_param,&(_param._handle),&(_param._result));

}

ETDLL_API int32 et_get_task_downloading_info(u32 task_id, void *p_res_info)
{
    if (!g_already_init)
    {
        return -1;
    }
	
	LOG_DEBUG("et_get_task_downloading_info: task_id = %d", task_id);
	
	CHECK_CRITICAL_ERROR;

	TM_POST_PARA_2 _param;
    sd_memset(&_param,0,sizeof(TM_POST_PARA_2));
	_param._para1 = (void*)task_id;
	_param._para2 = (void*)p_res_info;

    return tm_post_function(tm_get_task_downloading_info, (void *)&_param, &(_param._handle),&(_param._result));

}

ETDLL_API int32 et_get_assist_task_info(u32 task_id, void * p_assist_task_info)
{
	TM_POST_PARA_2 _param;
	
    if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_get_assist_task_info: task_id = %d",task_id);
	
	CHECK_CRITICAL_ERROR;

    sd_memset(&_param,0,sizeof(TM_POST_PARA_2));
	_param._para1 = (void*)task_id;
	_param._para2 = (void*)p_assist_task_info;

    return tm_post_function(tm_get_assist_task_info, (void *)&_param, &(_param._handle),&(_param._result));
}

ETDLL_API int32 et_set_recv_data_from_assist_pc_only(u32 task_id, void * p_resource, void* p_task_prop)
{
	TM_POST_PARA_3 _param;
	
    if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_set_recv_data_from_assist_pc_only: task_id = %d",task_id);
	
	CHECK_CRITICAL_ERROR;

    sd_memset(&_param,0,sizeof(TM_POST_PARA_2));
	_param._para1 = (void*)task_id;
	_param._para2 = (void*)p_resource;
	_param._para3 = (void*)p_task_prop;

    return tm_post_function(tm_set_recv_data_from_assist_pc_only, (void *)&_param, &(_param._handle),&(_param._result));
}

#if defined(ENABLE_HSC) && !defined(ENABLE_ETM_MEDIA_CENTER)
ETDLL_API int32 et_hsc_set_user_info(const uint64 userid, const char* p_jumpkey, const u32 jumpkey_len)
{
    TM_POST_PARA_3 _param;
    if (!g_already_init)
    {
        return -1;
    }

    CHECK_CRITICAL_ERROR;
    _u64 temp_userid = userid;
    sd_memset(&_param, 0, sizeof(TM_POST_PARA_3));
    _param._para1 = (void*)(&temp_userid);
    _param._para2 = (void*)p_jumpkey;
    _param._para3 = (void*)jumpkey_len;

    return tm_post_function(tm_hsc_set_user_info, (void *)&_param, &(_param._handle),&(_param._result));
}

#endif

#ifdef UPLOAD_ENABLE
ETDLL_API int32 et_ulm_del_record_by_full_file_path(uint64 size,  const char *path)
{
    if (!g_already_init)
    {
        return -1;
    }

    CHECK_CRITICAL_ERROR;

    _u64 *p_temp_size = NULL;
    sd_malloc(sizeof(_u64), (void**)&p_temp_size);
    *p_temp_size = size;
    
    char *buffer = NULL;
    sd_malloc(MAX_FILE_PATH_LEN, (void**)&buffer);
    sd_memcmp(buffer, path, MAX_FILE_PATH_LEN);

    TM_POST_PARA_2 *p_param = NULL;
    sd_malloc(sizeof(TM_POST_PARA_2), (void**)(&p_param));
    sd_memset(p_param, 0, sizeof(TM_POST_PARA_2));
    p_param->_para1 = (void*)(p_temp_size);
    p_param->_para2 = (void*)buffer;

    return tm_post_function_unblock(tm_ulm_del_record_by_full_file_path, (void *)(p_param), &(p_param->_result));
    
}

ETDLL_API int32 et_ulm_change_file_path(uint64 size, const char *old_path, const char *new_path)
{
    if (!g_already_init)
    {
        return -1;
    }

    CHECK_CRITICAL_ERROR;

    _u64 *p_temp_size = NULL;
    sd_malloc(sizeof(_u64), (void**)&p_temp_size);
    *p_temp_size = size;

    char *old_path_buffer = NULL;
    sd_malloc(MAX_FULL_PATH_BUFFER_LEN, (void**)&old_path_buffer);
    sd_memcmp(old_path_buffer, old_path, MAX_FULL_PATH_BUFFER_LEN);

    char *new_path_buffer = NULL;
    sd_malloc(MAX_FULL_PATH_BUFFER_LEN, (void**)&new_path_buffer);
    sd_memcmp(new_path_buffer, new_path, MAX_FULL_PATH_BUFFER_LEN);

    TM_POST_PARA_3 *p_param = NULL;
    sd_malloc(sizeof(TM_POST_PARA_3), (void**)&p_param);
    sd_memset(p_param, 0, sizeof(TM_POST_PARA_3));
    p_param->_para1 = (void*)p_temp_size;
    p_param->_para2 = (void*)old_path_buffer;
    p_param->_para3 = (void*)new_path_buffer;

    return tm_post_function_unblock(tm_ulm_change_file_path, (void *)p_param, &(p_param->_result));
}

#endif

ETDLL_API int32 et_get_network_flow(u32 *download, u32 *upload)
{
    if (!g_already_init)
    {
        return -1;
    }
    CHECK_CRITICAL_ERROR;

    TM_POST_PARA_2 p_param;
    sd_memset(&p_param, 0, sizeof(TM_POST_PARA_2));
	p_param._para1 = (void*)download;
	p_param._para2 = (void*)upload;
	return tm_post_function(tm_get_network_flow, (void *)&p_param, &(p_param._handle),&(p_param._result));    
}

ETDLL_API int32 et_set_original_url(u32 taskid, char *url, char *refurl)
{
    TM_ORIGINAL_URL_INFO param;
    sd_memset(&param, 0, sizeof(param));
    param._task_id = taskid;
    param._orignal_url = url;
    param._ref_url = refurl;
    return tm_post_function(tm_set_original_url, &param, &(param._handle), &(param._result));
}

ETDLL_API int32 et_add_cdn_peer_resource( u32 task_id, u32 file_index, char *peer_id, u8 *gcid, uint64 file_size, u8 peer_capability,  u32 host_ip, u16 tcp_port , u16 udp_port, u8 res_level, u8 res_from )
{
       TM_ADD_CDN_PEER_RESOURCE _param;

       if(!g_already_init)
               return -1;

       LOG_DEBUG("dt_add_cdn_peer_resource, task_id = %d, file_index = %d, sn_peerid = %s"
           , task_id, file_index, peer_id);

       CHECK_CRITICAL_ERROR;

       sd_memset(&_param,0,sizeof(TM_ADD_CDN_PEER_RESOURCE));
       _param._task_id = task_id;
       _param._file_index = file_index;
       _param._peer_id = peer_id;
       _param._gcid = gcid;
       _param._file_size = file_size;
       _param._peer_capability = peer_capability;
       _param._host_ip = host_ip;
       _param._tcp_port = tcp_port;
       _param._udp_port = udp_port;
       _param._res_level = res_level;
       _param._res_from = res_from;

       return tm_post_function(tm_add_cdn_peer_resource, (void *)&_param,&(_param._handle),&(_param._result));

}

ETDLL_API int32 et_add_lixian_server_resource( u32 task_id, u32 file_index, char *url, u32 url_size, char* ref_url, u32 ref_url_size, char* cookie, u32 cookie_size,u32 resource_priority )
{
       TM_ADD_LX_RESOURCE _param;

       if(!g_already_init)
               return -1;

       LOG_DEBUG("dt_add_lixian_server_resource");

       CHECK_CRITICAL_ERROR;

       sd_memset(&_param,0,sizeof(TM_ADD_LX_RESOURCE));
       _param._task_id= task_id;
       _param._file_index = file_index;
       _param._url = url;
       _param._url_size = url_size;
       _param._ref_url = ref_url;
       _param._ref_url_size = ref_url_size;
       _param._cookie = cookie;
       _param._cookie_size = cookie_size;
       _param._resource_priority = resource_priority;

       return tm_post_function(tm_add_lixian_server_resource, (void *)&_param,&(_param._handle),&(_param._result));

}

ETDLL_API int32 et_get_bt_acc_file_index(u32 task_id, void *sublist)
{
#ifdef ENABLE_BT
    TM_BT_ACC_FILE_INDEX _param;
       LIST *_sublist = (LIST*)sublist;

    if(!g_already_init)
        return -1;

    LOG_DEBUG("et_get_bt_acc_file_index, task_id:%d", task_id);

    CHECK_CRITICAL_ERROR;

    if(task_id==0) return TM_ERR_INVALID_TASK_ID;

    sd_memset(&_param,0,sizeof(TM_BT_ACC_FILE_INDEX));

    _param._task_id = task_id;
    _param._file_index = _sublist;

    return tm_post_function(tm_get_bt_acc_file_index, (void *)&_param,&(_param._handle),&(_param._result));
#else
    return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_BT */
}

ETDLL_API int32 et_get_bt_task_torrent_info(u32 task_id, void *p_torrent_info)
{
#ifdef ENABLE_BT
    TM_GET_BT_TASK_TORRENT_INFO _param;

    if(!g_already_init)
        return -1;

    LOG_DEBUG("et_get_bt_task_torrent_info,task_id=%d",task_id);

    CHECK_CRITICAL_ERROR;

    sd_memset(&_param,0,sizeof(TM_GET_BT_TASK_TORRENT_INFO));
    _param.task_id = task_id;
    _param.torrent_info =(void*)p_torrent_info;

    return tm_post_function(tm_get_bt_task_torrent_info, (void *)&_param,&(_param._handle),&(_param._result));
#else
    return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_BT */
}

ETDLL_API int32 et_get_bt_file_index_info(u32 task_id, u32 file_index, char* cid, char* gcid, uint64 *file_size)
{
#ifdef ENABLE_BT
    TM_BT_FILE_INDEX_INFO _param;

    if(!g_already_init)
        return -1;
    LOG_DEBUG("et_get_bt_file_index_info, task_id = %u, file_index = %u", task_id, file_index);

    CHECK_CRITICAL_ERROR;

    if(task_id==0) return TM_ERR_INVALID_TASK_ID;

    sd_memset(&_param,0,sizeof(TM_BT_FILE_INDEX_INFO));

    _param._task_id = task_id;
    _param._file_index = file_index;
       _param._cid = cid;
       _param._gcid = gcid;
       _param._file_size = file_size;

    return tm_post_function(tm_get_bt_file_index_info, (void *)&_param,&(_param._handle),&(_param._result));
#else
    return NOT_IMPLEMENT;
#endif /* #ifdef ENABLE_BT */
}

ETDLL_API int32 et_check_alive()
{
    TM_POST_PARA_0 _param;
    sd_memset(&_param, 0, sizeof(TM_POST_PARA_0));
    return tm_post_function(tm_check_alive, (void *)&_param, &(_param._handle), &(_param._result));
    
}

ETDLL_API int32 et_set_task_dispatch_mode(u32 task_id, ET_TASK_DISPATCH_MODE mode, uint64 filesize_to_little_file)
{
    TM_POST_PARA_3 param;
    sd_memset(&param, 0, sizeof(TM_POST_PARA_3));
    param._para1 = (void *)task_id;
    param._para2 = (void *)mode;
    param._para3 = (void *)&filesize_to_little_file;
    
    return tm_post_function( tm_set_task_dispatch_mode, (void *)&param, &(param._handle), &(param._result));    
}


ETDLL_API int32 et_get_download_bytes(u32 task_id, char* host, uint64* download)
{
    TM_POST_PARA_3 param;
    sd_memset(&param, 0, sizeof(TM_POST_PARA_3));
	if(!host) {
		LOG_URGENT("error host!!");
		return INVALID_ARGUMENT;
	}
    param._para1 = (void *)task_id;
    param._para2 = (void *)host;
    param._para3 = (void *)download;

    return tm_post_function( tm_get_download_bytes, (void *)&param, &(param._handle), &(param._result));    
}

ETDLL_API int32 et_set_module_option(const ET_MODULE_OPTIONS p_options)
{
	TM_POST_PARA_1 param;
	sd_memset(&param, 0, sizeof(TM_POST_PARA_1));
    param._para1 = (void *)(p_options.options);

    return tm_post_function( tm_set_module_option, (void *)&param, &(param._handle), &(param._result));    
}

ETDLL_API int32 et_get_module_option(ET_MODULE_OPTIONS* p_options)
{
	TM_POST_PARA_1 param;
	sd_memset(&param, 0, sizeof(TM_POST_PARA_1));
	p_options->options = 0;
    param._para1 = (void *)(&p_options->options);

    return tm_post_function( tm_get_module_option, (void *)&param, &(param._handle), &(param._result));    
}

ETDLL_API int32 et_get_info_from_cfg_file(char* cfg_path, char* url, BOOL* cid_is_valid, u8* cid, uint64* file_size, char* file_name, char *lixian_url, char *cookie, char *user_data)
{	
	TM_POST_PARA_9 _param;
	
	if(!g_already_init)
		return -1;
	
	LOG_DEBUG("et_get_info_from_cfg_file[%s]", cfg_path);
	
	if( (cfg_path==NULL) || (url==NULL) || cid_is_valid==NULL || cid==NULL)
		return TM_ERR_INVALID_PARAMETER;

	CHECK_CRITICAL_ERROR;
	
	sd_memset(&_param, 0, sizeof(_param));
	_param._para1 = cfg_path;
	_param._para2 = url;
	_param._para3 = cid_is_valid;
	_param._para4 = cid;
	_param._para5 = file_size;
	_param._para6 = file_name;
	_param._para7 = lixian_url;
	_param._para8 = cookie;
	_param._para9 = user_data;
	
	return tm_post_function(tm_get_info_from_cfg_file, (void *)&_param,&(_param._handle),&(_param._result));
}


