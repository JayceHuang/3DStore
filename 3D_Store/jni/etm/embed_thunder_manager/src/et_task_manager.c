#include "et_task_manager.h"

#include "em_common/em_string.h"
#include "em_common/em_version.h"
#include "em_common/em_mempool.h"
#include "em_common/em_time.h"
#include "em_common/em_utility.h"
#include "em_interface/em_customed_interface.h"
#include "em_interface/em_task.h"
#include "em_interface/em_fs.h"
#include "em_asyn_frame/em_asyn_frame.h"

#include "download_manager/download_task_interface.h"
#include "download_manager/download_task_data.h"
#include "download_manager/mini_task_interface.h"

#include "tree_manager/tree_interface.h"
#include "scheduler/scheduler.h"
#include "vod_task/vod_task.h"

#include "utility/sd_os.h"
#include "utility/md5.h"
#include "utility/utility.h"
#include "utility/errcode.h"
#include "utility/string.h"
#include "utility/aes.h"
#include "platform/sd_gz.h"


#if defined(LINUX)
#include <string.h>
#endif

#include "em_common/em_logid.h"
#ifdef EM_LOGID
#undef EM_LOGID
#endif

#define EM_LOGID LOGID_ASYN_FRAME_TEST
#include "em_common/em_logger.h"

#define ETM_CHECK_CRITICAL_ERROR {if(em_get_critical_error()!=SUCCESS) return NULL;}

static BOOL g_already_init = FALSE;
static BOOL os_inited_by_etm = FALSE;

#if defined(ENABLE_NULL_PATH)
static BOOL g_etm_inited_by_null_path = FALSE;
static BOOL g_is_license_verified = FALSE;
static _u32  g_license_result=-1;
static _u32  g_license_expire_time=0;

BOOL etm_is_et_running(void)
{
       u32 ret_val;
       ret_val = et_get_max_tasks();
	if(ret_val == (u32)-1)
	{
	        return FALSE;
	}
	return TRUE;
}

#endif
/*--------------------------------------------------------------------------*/
/*                Interfaces provided by EmbedThunderTaskManager			        */
/*--------------------------------------------------------------------------*/
_int32 etm_clear(void);

ETDLL_API int32 etm_load_tasks(const char *etm_system_path,u32  path_len)
{
	int ret_val = SUCCESS;
	char system_path[MAX_FILE_PATH_LEN];
	char path[MAX_FILE_NAME_LEN] = {0};

	if(g_already_init)
		return ALREADY_ET_INIT;

	os_inited_by_etm = FALSE;

	sd_assert(sizeof(char)==1);
	sd_assert(sizeof(_int32)==4);
	sd_assert(sizeof(int32)==4);
	sd_assert(sizeof(Bool)==sizeof(_int32));
	sd_assert(sizeof(ETM_TASK_STATE)==sizeof(_int32));
	sd_assert(sizeof(ETM_TASK_TYPE)==sizeof(_int32));
	sd_assert(sizeof(ETM_TASK_INFO)==sizeof(EM_TASK_INFO));
	
	if(!em_is_et_version_ok()) return ET_WRONG_VERSION;

	if((etm_system_path==NULL)||(em_strlen(etm_system_path)==0)||(path_len==0)||(path_len>=MAX_FILE_PATH_LEN)) return INVALID_ARGUMENT;

	if(! et_os_is_initialized())
	{
#if defined(MACOS)&&defined(MOBILE_PHONE)
		char default_log_file[MAX_FULL_PATH_BUFFER_LEN];
		em_memset(default_log_file,0x00,MAX_FULL_PATH_BUFFER_LEN);
		if((etm_system_path!=NULL)&&(path_len!=0))
		{
			em_strncpy(default_log_file,etm_system_path,path_len);
			if(default_log_file[path_len-1]!='/') em_strcat(default_log_file,"/",1);
		}
		em_strcat(default_log_file,LOG_CONFIG_FILE,em_strlen(LOG_CONFIG_FILE));
		ret_val = et_os_init(default_log_file);
#else

		ret_val = et_os_init(LOG_CONFIG_FILE);
#endif
		CHECK_VALUE(ret_val);
		os_inited_by_etm = TRUE;
	}

	EM_LOG_DEBUG("etm_load_tasks, path[%u]:%s",path_len, etm_system_path );

	/* test cpu frequence */
	em_test_cpu_frq();


	em_memset(system_path,0,MAX_FILE_PATH_LEN);
	em_strncpy(system_path, etm_system_path, path_len);
	if(!em_file_exist(system_path))
	{
		if(os_inited_by_etm) et_os_uninit();
		CHECK_VALUE(INVALID_FILE_PATH);
	}

	em_set_critical_error(SUCCESS);


#if defined(MOBILE_PHONE)
	if((etm_system_path!=NULL)&&(path_len!=0))
	{
	#ifdef ENABLE_ETM_REPORT
		ret_val = reporter_init(etm_system_path, path_len);
		if(ret_val!=SUCCESS)
		{
			if(os_inited_by_etm) et_os_uninit();
			CHECK_VALUE(ret_val);
		}
	#endif
	}
#endif /* MOBILE_PHONE */

	ret_val = em_start_asyn_frame( em_init, (void*)system_path, em_uninit, NULL);
	if(ret_val==SUCCESS)
	{
		g_already_init = TRUE;
		em_sleep(10);
	}
	else
	if(os_inited_by_etm) 
	{
		et_os_uninit();
	}
	
	
	return ret_val;
}

ETDLL_API int32 etm_unload_tasks(void)
{
	_int32 ret_val = SUCCESS;

	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("uninit 1");
	
	etm_clear();
	
	//EM_CHECK_CRITICAL_ERROR;

       EM_LOG_DEBUG("uninit 2"); 
	
	EM_LOG_DEBUG("ok,em_stop_asyn_frame");

	ret_val = em_stop_asyn_frame();

        EM_LOG_DEBUG("uninit 3");

       //tm_force_close_resource();
	   
	EM_LOG_DEBUG("uninit 4");

#if defined(MOBILE_PHONE)
	#ifdef ENABLE_ETM_REPORT
	ret_val = reporter_uninit();
	#endif
#endif
#if defined(ENABLE_NULL_PATH)
      //
#else
	if(os_inited_by_etm)
	{
		et_os_uninit();
		os_inited_by_etm = FALSE;
	}
#endif
	
	g_already_init = FALSE;
	return SUCCESS;
}

#if defined(ENABLE_NULL_PATH)
/*  This is the call back function of license report result
	Please make sure keep it in this type:
	typedef int32 ( *ET_NOTIFY_LICENSE_RESULT)(u32 result, u32 expire_time);
*/
_int32 _callback_et_notify_license_result(_u32  result, _u32  expire_time)
{
	EM_LOG_DEBUG("em_notify_license_result:result=%u,expire_time=%u",result,expire_time);
	/* Add your code here ! */
	/* �ر�ע��:������ص���������,����Ӧ�þ�����࣬�����в����������������������κ����ؿ�Ľӿ�!��Ϊ�����ᵼ�����ؿ�����!����!!! */
	if((result==0)||(result>=21000))
	{
		g_is_license_verified = TRUE;
	}
	else
	{
		g_is_license_verified = FALSE;
	}

	g_license_result = result;
	g_license_expire_time = expire_time;

	return SUCCESS;
}
#endif

#ifdef ENABLE_ETM_MEDIA_CENTER
static int32 _startup_do_upgrade();
#endif
/////1 ��ʼ���뷴��ʼ��
/////1.1 ��ʼ��
ETDLL_API int32 etm_init(const char *etm_system_path,u32  path_len)
{
	int ret_val = SUCCESS;
#ifdef ENABLE_ETM_MEDIA_CENTER
	char system_path[MAX_FILE_PATH_LEN] = {0};
	_int32 link_ret = SUCCESS;
#endif

#if defined(_DEBUG) && defined(ENABLE_ETM_MEDIA_CENTER)
	signal(11, SIG_DFL);
	signal(6, SIG_DFL);
#endif

	if(g_already_init)
		return ALREADY_ET_INIT;

#ifdef ENABLE_ETM_MEDIA_CENTER
	link_ret = readlink(etm_system_path, system_path, MAX_FILE_NAME_LEN);
	if( link_ret > 0 )
	{
		etm_system_path = system_path;
		path_len = link_ret;
	}
#endif

	os_inited_by_etm = FALSE;

	sd_assert(sizeof(char)==1);
	sd_assert(sizeof(_int32)==4);
	sd_assert(sizeof(int32)==4);
	sd_assert(sizeof(Bool)==sizeof(_int32));
	sd_assert(sizeof(ETM_TASK_STATE)==sizeof(_int32));
	sd_assert(sizeof(ETM_TASK_TYPE)==sizeof(_int32));
	sd_assert(sizeof(ETM_TASK_INFO)==sizeof(EM_TASK_INFO));
	
	if(!em_is_et_version_ok()) return ET_WRONG_VERSION;

#if defined(ENABLE_NULL_PATH)
 	if((etm_system_path==NULL)||(em_strlen(etm_system_path)==0)||(path_len==0)||(path_len>=MAX_FILE_PATH_LEN))
      {
            ret_val = et_init(NULL);
	     if(SUCCESS != ret_val) return ret_val;
	     ret_val = et_set_license_callback(_callback_et_notify_license_result);
	     if(SUCCESS != ret_val) return ret_val;
            g_etm_inited_by_null_path = TRUE;
      }
      else
      {
	    ret_val = etm_load_tasks(etm_system_path, path_len);
      }

#else
 	if((etm_system_path==NULL)||(em_strlen(etm_system_path)==0)||(path_len==0)||(path_len>=MAX_FILE_PATH_LEN)) return INVALID_ARGUMENT;
	ret_val = etm_load_tasks(etm_system_path, path_len);
#endif

#if defined(ENABLE_ETM_MEDIA_CENTER) && ! defined(DVERSION_FOR_VALGRIND)
	_startup_do_upgrade();
#endif

	return ret_val;
}

#ifdef ENABLE_ETM_MEDIA_CENTER
static int32 _startup_do_upgrade()
{
	char *sl_path = MEDIA_CENTER_WEBUI_PATH"/cur";
	char cur_ver[MAX_FILE_NAME_LEN];
	char max_ver[MAX_FILE_NAME_LEN];
	int32 max_ver_num = 0;
	int32 ret_val = SUCCESS;
	int32 file_name_len = 0;
	u32 file_num = 0;
	int32 nr_sub_files = 0;
	FILE_ATTRIB *sub_files = NULL;
	int32 i = 0;
	int32 t;
	static BOOL force_upgrade = TRUE;

	if (! sd_file_exist(MEDIA_CENTER_WEBUI_PATH))
	{
		EM_LOG_DEBUG("path %s not found, do not upgrade", MEDIA_CENTER_WEBUI_PATH);
		return SUCCESS;
	}

	EM_LOG_DEBUG("_startup_do_upgrade(), force_upgrade=%d", force_upgrade);
	// ��ȡ��ǰ������ָ��İ汾��
	file_name_len = readlink(sl_path, cur_ver, MAX_FILE_NAME_LEN);

	// ��õ�ǰ���İ汾��
	ret_val = sd_get_sub_files(MEDIA_CENTER_WEBUI_PATH, NULL, 0, &file_num);
	EM_LOG_DEBUG("_startup_do_upgrade(), found %d files in %s", file_num, MEDIA_CENTER_WEBUI_PATH);
	
	nr_sub_files = file_num;
	ret_val = sd_malloc(sizeof(FILE_ATTRIB) * nr_sub_files, (void **)&sub_files);
	CHECK_VALUE(ret_val);

	ret_val = sd_get_sub_files(MEDIA_CENTER_WEBUI_PATH, sub_files, nr_sub_files, &file_num);
	CHECK_VALUE(ret_val);

	for (i=0;i<file_num;i++)
	{
		EM_LOG_DEBUG("_startup_do_upgrade(), found file %s, is_dir=%d", sub_files[i]._name, sub_files[i]._is_dir);
		if (FALSE == sub_files[i]._is_dir)
			continue;
		if ('.' == sub_files[i]._name[0])
			continue;
		t = sd_atoi(sub_files[i]._name);
		EM_LOG_DEBUG("_startup_do_upgrade(), found file %s, ver is %d", sub_files[i]._name, t);
		if (t > max_ver_num)
		{
			max_ver_num = t;
			sd_strncpy(max_ver, sub_files[i]._name, MAX_FILE_NAME_LEN);
		}
	}

	sd_free(sub_files);
	sub_files = NULL;

	// û�п��ð汾��ǿ������
	if (0 == max_ver_num)
	{
		if (! force_upgrade) return ret_val;

		EM_LOG_ERROR("can not find available version, force to upgrade!!!");
		force_upgrade = FALSE;
		ret_val = em_down_upgrade_files(TRUE);
		if (SUCCESS == ret_val)
		{
			return _startup_do_upgrade();
		}
		else
		{
			EM_LOG_ERROR("force upgrade: em_down_upgrade_files return %d", ret_val);
			return ret_val;
		}
	}

	// �ж��Ƿ���Ҫ����
	if (-1 == file_name_len ||
		sd_atoi(cur_ver) < max_ver_num)
	{
		EM_LOG_DEBUG("upgrade to version %s", max_ver);
		unlink(sl_path);
		symlink(max_ver, sl_path);
	}

	return ret_val;
}
#endif

_int32 etm_clear(void)
{
	POST_PARA_0 param;
	
	EM_LOG_DEBUG("etm_clear");

	//EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(POST_PARA_0));
	
	return em_post_function(em_clear, (void *)&param,&(param._handle),&(param._result));

}
/////1.2����ʼ��
ETDLL_API int32 etm_uninit(void)
{

#if defined(ENABLE_NULL_PATH)
	_int32 ret_val;
        ret_val = etm_unload_tasks();
        ret_val = em_force_stop_et();;
        if(os_inited_by_etm)
        {
		et_os_uninit();
		os_inited_by_etm = FALSE;
	 }
        g_etm_inited_by_null_path = FALSE;	   
#else	
       return etm_unload_tasks();
#endif	
	return SUCCESS;
}

/////1.3 ������ؽӿ�
ETDLL_API int32 etm_set_network_cnt_notify_callback( ETM_NOTIFY_NET_CONNECT_RESULT callback_function_ptr)
{
	static POST_PARA_1 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_set_network_cnt_notify_callback");

	EM_CHECK_CRITICAL_ERROR;

	if(callback_function_ptr==NULL) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)callback_function_ptr;
	
	return em_post_function_unlock(em_set_network_cnt_notify_callback, (void *)&param,&(param._handle),&(param._result));
}
extern _int32 em_sleep(_u32 ms);
ETDLL_API int32 etm_init_network(u32 iap_id)
{
	_int32 ret_val = SUCCESS;
	POST_PARA_1 param;
	
	EM_LOG_DEBUG("etm_init_network:%u",iap_id);

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1 =(void*) iap_id;
	
	ret_val = em_post_function(em_init_network, (void *)&param,&(param._handle),&(param._result));
	if(ret_val==SUCCESS)
	{
		_int32 count = 0;
		do
		{
			em_sleep(10);
		}while((ENS_CNTING== etm_get_network_status())&&(count++<10));
	}
	return ret_val;
}
ETDLL_API int32 etm_uninit_network(void)
{
	POST_PARA_0 param;
	
	EM_LOG_DEBUG("etm_uninit_network");

	//EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(POST_PARA_0));
	
	return em_post_function(em_uninit_network, (void *)&param,&(param._handle),&(param._result));

}
	
ETDLL_API ETM_NET_STATUS etm_get_network_status(void)
{
	POST_PARA_1 param;
	ETM_NET_STATUS status=ENS_DISCNT;
	_int32 ret_val=SUCCESS;
	
	if(!g_already_init)
		return ENS_DISCNT;
	
	EM_LOG_DEBUG("etm_get_network_status");

	if(em_get_critical_error()!=SUCCESS) return ENS_DISCNT;


	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)&status;
	
	ret_val= em_post_function(em_get_network_status, (void *)&param,&(param._handle),&(param._result));
	if(ret_val==SUCCESS)
	{
		return status;
	}
	
	return ENS_DISCNT;
}

ETDLL_API int32 etm_get_network_iap(u32 *iap_id)
{
	POST_PARA_1 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_get_network_iap");

	EM_CHECK_CRITICAL_ERROR;

	if(iap_id==NULL) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)iap_id;
	
	return em_post_function(em_get_network_iap, (void *)&param,&(param._handle),&(param._result));
}
ETDLL_API int32 etm_get_network_iap_from_ui(u32 *iap_id)
{
	POST_PARA_1 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_get_network_iap_from_ui");

	EM_CHECK_CRITICAL_ERROR;

	if(iap_id==NULL) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)iap_id;
	
	return em_post_function(em_get_network_iap_from_ui, (void *)&param,&(param._handle),&(param._result));
}

ETDLL_API const char* etm_get_network_iap_name(void)
{
	POST_PARA_1 param;
	static char iap_name[MAX_IAP_NAME_LEN];
	_int32 ret_val=SUCCESS;
	
	if(!g_already_init)
		return NULL;
	
	EM_LOG_DEBUG("etm_get_network_iap_name");

	ETM_CHECK_CRITICAL_ERROR;

	em_memset(iap_name,0,MAX_IAP_NAME_LEN);
	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)iap_name;
	
	ret_val= em_post_function( em_get_network_iap_name, (void *)&param,&(param._handle),&(param._result));
	if(ret_val==SUCCESS)
	{
		if(em_strlen(iap_name)>0)
			return iap_name;
	}
	
	return NULL;
}
ETDLL_API int32 etm_get_network_flow(u32 * download,u32 * upload)
{
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_get_network_flow");

	EM_CHECK_CRITICAL_ERROR;

	return  em_get_network_flow(download,upload);
	
}

ETDLL_API int32 etm_set_peerid(const char* peerid, u32 peerid_len)
{
	POST_PARA_2 param;
	_int32 ret_val=SUCCESS;

	if( NULL == peerid || peerid_len < PEER_ID_SIZE)
		return INVALID_ARGUMENT;
	em_memset(&param,0,sizeof(POST_PARA_2));
	param._para1= (void*)peerid;
	param._para2= (void*)peerid_len;
	
	EM_LOG_DEBUG("etm_set_peerid, peerid = %s, peerid_len = %u", peerid, peerid_len);
	
	ret_val= em_set_peerid((void*)&param);
	
	return ret_val;
}

ETDLL_API const char* etm_get_peerid(void)
{
	POST_PARA_1 param;
	static char peerid[PEER_ID_SIZE + 1];
	_int32 ret_val=SUCCESS;
	
	if(!g_already_init)
		return NULL;
	
	EM_LOG_DEBUG("etm_get_peerid");

	ETM_CHECK_CRITICAL_ERROR;

	em_memset(peerid,0,PEER_ID_SIZE + 1);
	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)peerid;
	
	ret_val= em_post_function( em_get_peerid, (void *)&param,&(param._handle),&(param._result));
	if(ret_val==SUCCESS)
	{
		if(em_strlen(peerid)>0)
			return peerid;
	}
	
	em_memset(peerid,0x30,PEER_ID_SIZE);
	peerid[PEER_ID_SIZE-1] = 'V';
	return peerid;
}

ETDLL_API int32 etm_user_set_network(Bool is_ok)
{
	em_user_set_netok(is_ok);
	return SUCCESS;
}

ETDLL_API int32 etm_set_net_type(u32 net_type)
{
	static POST_PARA_1 param;
	
	EM_LOG_DEBUG("etm_set_net_type:0x%X",net_type);

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1 =(void*) net_type;
	
	return em_post_function_unlock(em_set_net_type, (void *)&param,&(param._handle),&(param._result));
	
}

///////////////////////////////////////////////////////////////
/////2 ϵͳ����

/////2.1  �����û��Զ���ĵײ�ӿ�,����ӿڱ����ڵ���etm_init ֮ǰ����!
ETDLL_API int32 etm_set_customed_interface(int32 fun_idx, void *fun_ptr)
{
	if(g_already_init)
		return ALREADY_ET_INIT;

	return em_set_customed_interfaces(fun_idx, fun_ptr);
}

	
/////2.2 ��ȡ�汾
ETDLL_API const char* etm_get_version(void)
{
	static char version_buffer[64];

	em_get_version(version_buffer, sizeof(version_buffer));
	//sd_assert(em_strlen(version_buffer)>0);
	return version_buffer;
}

/////2.3 ע��license
ETDLL_API int32 	etm_set_license(const char *license, int32 license_size)
{
	POST_PARA_1 param;
	char license_buffer[LICENSE_LEN+1];
	
	if(!g_already_init)
	{
#if defined(ENABLE_NULL_PATH)
		if(!etm_is_et_running())
		{
		     return -1;
		}
#else
		     return -1;
#endif
	}
	
	EM_LOG_DEBUG("etm_set_license:license[%d]=%s",license_size,license);

	EM_CHECK_CRITICAL_ERROR;

	if((license==NULL)||(em_strlen(license)<LICENSE_LEN)||(license_size!=LICENSE_LEN)) return INVALID_ARGUMENT;

#if defined(ENABLE_NULL_PATH)
       if(!g_already_init)
       {
            return et_set_license(license, license_size);
       }
	else
	{
		em_memset(license_buffer,0,LICENSE_LEN+1);
		em_memcpy(license_buffer, license, LICENSE_LEN);
		
		em_memset(&param,0,sizeof(POST_PARA_1));
		param._para1= (void*)license_buffer;
		
		return em_post_function(em_set_license, (void *)&param,&(param._handle),&(param._result));
	}
#else
	em_memset(license_buffer,0,LICENSE_LEN+1);
	em_memcpy(license_buffer, license, LICENSE_LEN);
	
	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)license_buffer;
	
	return em_post_function(em_set_license, (void *)&param,&(param._handle),&(param._result));
#endif	
}

ETDLL_API const char* etm_get_license(void)
{
	POST_PARA_1 param;
	static char license[LICENSE_LEN+1];
	_int32 ret_val=SUCCESS;
	
	if(!g_already_init)
		return NULL;
	
	EM_LOG_DEBUG("etm_get_license");

	ETM_CHECK_CRITICAL_ERROR;

	em_memset(license,0,LICENSE_LEN+1);
	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)license;
	
	ret_val= em_post_function( em_get_license, (void *)&param,&(param._handle),&(param._result));
	if(ret_val==SUCCESS)
	{
		sd_assert(em_strlen(license)==LICENSE_LEN);
		return license;
	}
	
	return NULL;
}



/////2.4 ��ȡlicense��֤���
ETDLL_API int32 	etm_get_license_result(u32 * result)
{
	POST_PARA_1 param;
	
	if(!g_already_init)
#if defined(ENABLE_NULL_PATH)
		if(!etm_is_et_running())
		{
		     return -1;
		}
#else
		     return -1;
#endif
	
	EM_LOG_DEBUG("etm_get_license_result");

	EM_CHECK_CRITICAL_ERROR;

	if(result==NULL) return INVALID_ARGUMENT;

#if defined(ENABLE_NULL_PATH) 
  #if defined( ENABLE_LICENSE)
  	if(!g_already_init)
	{
		*result = g_license_result;
		if(g_is_license_verified == FALSE)
		{
			return LICENSE_VERIFYING;
		}
  	}
	else
	{
		em_memset(&param,0,sizeof(POST_PARA_1));
		param._para1= (void*)result;
		
		return em_post_function(em_get_license_result, (void *)&param,&(param._handle),&(param._result));
	}
   #else
	*result = 0;
    #endif 
	return SUCCESS;
#else
	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)result;
	
	return em_post_function(em_get_license_result, (void *)&param,&(param._handle),&(param._result));
#endif
}


/////2.5 ����ϵͳĬ���ַ������ʽ
ETDLL_API int32 etm_set_default_encoding_mode( ETM_ENCODING_MODE encoding_mode)
{
	static POST_PARA_1 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_set_default_encoding_mode:%d",encoding_mode);

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)encoding_mode;
	
	return em_post_function_unlock(em_set_default_encoding_mode, (void *)&param,&(param._handle),&(param._result));
}

ETDLL_API ETM_ENCODING_MODE etm_get_default_encoding_mode(void)
{
	POST_PARA_1 param;
	ETM_ENCODING_MODE encoding_mode = EEM_ENCODING_DEFAULT;
	_int32 ret_val=SUCCESS;
	
	if(!g_already_init)
		return EEM_ENCODING_DEFAULT;
	
	EM_LOG_DEBUG("etm_get_default_encoding_mode");

	if(em_get_critical_error()!=SUCCESS) return EEM_ENCODING_DEFAULT;

	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)&encoding_mode;
	
	ret_val= em_post_function( em_get_default_encoding_mode, (void *)&param,&(param._handle),&(param._result));
	if(ret_val==SUCCESS)
	{
		return encoding_mode;
	}
	
	return EEM_ENCODING_DEFAULT;
}



/////2.6 ����Ĭ�ϴ洢·��,�����Ѵ����ҿ�д,path_len ����С��256
ETDLL_API int32 etm_set_download_path(const char *path,u32  path_len)
{
	POST_PARA_1 param;
	char download_path[MAX_FILE_PATH_LEN];

	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_set_download_path:path[%d]=%s",path_len,path);

	EM_CHECK_CRITICAL_ERROR;

	if((path==NULL)||(em_strlen(path)==0)||(path_len==0)||(path_len>=MAX_FILE_PATH_LEN)) return INVALID_ARGUMENT;

	em_memset(download_path,0,MAX_FILE_PATH_LEN);
	em_strncpy(download_path, path, path_len);

	
	if(!em_file_exist(download_path))
	{
		CHECK_VALUE(INVALID_FILE_PATH);
	}
	
	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)download_path;
	
	return em_post_function(em_set_download_path, (void *)&param,&(param._handle),&(param._result));
}
ETDLL_API const char* etm_get_download_path(void)
{
	POST_PARA_1 param;
	static char download_path[MAX_FILE_PATH_LEN];
	_int32 ret_val=SUCCESS;
	
	if(!g_already_init)
		return NULL;
	
	EM_LOG_DEBUG("etm_get_download_path");

	ETM_CHECK_CRITICAL_ERROR;

	em_memset(download_path,0,MAX_FILE_PATH_LEN);
	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)download_path;
	
	ret_val= em_post_function( em_get_download_path, (void *)&param,&(param._handle),&(param._result));
	if(ret_val==SUCCESS)
	{
		//sd_assert(em_strlen(download_path)>0);
		return download_path;
	}
	
	return NULL;
}


/////2.7 ��������״̬ת���ص�֪ͨ����,�˺�����Ҫ�ڴ�������֮ǰ����
//typedef int32 ( *ETM_NOTIFY_TASK_STATE_CHANGED)(u32 task_id,ETM_TASK_INFO * p_task_info);
ETDLL_API int32 	etm_set_task_state_changed_callback( ETM_NOTIFY_TASK_STATE_CHANGED callback_function_ptr)
{
	POST_PARA_1 param;
	
	if(!g_already_init)
	{
#ifdef ENABLE_ETM_MEDIA_CENTER
		em_set_task_state_changed_callback_ptr((EM_NOTIFY_TASK_STATE_CHANGED)callback_function_ptr);
		return 0;
#else
		return -1;
#endif
	}
	
	EM_LOG_DEBUG("etm_set_task_state_changed_callback");

	EM_CHECK_CRITICAL_ERROR;

	if(callback_function_ptr==NULL) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)callback_function_ptr;
	
	return em_post_function( em_set_task_state_changed_callback, (void *)&param,&(param._handle),&(param._result));
}

//���ûص�������֪ͨ�����������ı䣬��Ҫ����
ETDLL_API int32 etm_set_file_name_changed_callback(ETM_NOTIFY_FILE_NAME_CHANGED callback_function_ptr)
{
	if(!g_already_init)
	{
		return -1;
	}

    POST_PARA_1 param;
        
	EM_LOG_DEBUG("etm_set_file_name_changed_callback");

	if(callback_function_ptr==NULL) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)callback_function_ptr;
	
	return em_post_function(dt_set_file_name_changed_callback, (void *)&param,&(param._handle),&(param._result));


}


/////2.8 �������̵㲥��Ĭ����ʱ�ļ�����·��,�����Ѵ����ҿ�д,path_len ����С��ETM_MAX_FILE_PATH_LEN
ETDLL_API int32 etm_set_vod_cache_path(const char *path,u32  path_len)
{
	POST_PARA_1 param;
	char cache_path[MAX_FILE_PATH_LEN];

	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_set_vod_cache_path:path[%d]=%s",path_len,path);

	EM_CHECK_CRITICAL_ERROR;

	if((path==NULL)||(em_strlen(path)==0)||(path_len==0)||(path_len>=MAX_FILE_PATH_LEN)) return INVALID_ARGUMENT;

	em_memset(cache_path,0,MAX_FILE_PATH_LEN);
	em_strncpy(cache_path, path, path_len);

	
	if(FALSE==em_file_exist(cache_path))
	{
		CHECK_VALUE(INVALID_FILE_PATH);
	}
	
	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)cache_path;
	
	//return em_post_function(vod_set_cache_path, (void *)&param,&(param._handle),&(param._result));
	return em_post_function(dt_set_vod_cache_path, (void *)&param,&(param._handle),&(param._result));
}
ETDLL_API const char* etm_get_vod_cache_path(void)
{
	POST_PARA_1 param;
	static char cache_path[MAX_FILE_PATH_LEN];
	_int32 ret_val=SUCCESS;
	
	if(!g_already_init)
		return NULL;
	
	EM_LOG_DEBUG("etm_get_vod_cache_path");

	ETM_CHECK_CRITICAL_ERROR;

	em_memset(cache_path,0,MAX_FILE_PATH_LEN);
	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)cache_path;
	
	//ret_val= em_post_function( vod_get_cache_path, (void *)&param,&(param._handle),&(param._result));
	ret_val= em_post_function( dt_get_vod_cache_path, (void *)&param,&(param._handle),&(param._result));
	if(ret_val==SUCCESS)
	{
		sd_assert(em_strlen(cache_path)>0);
		return cache_path;
	}
	
	return NULL;
}

/////2.9 �������̵㲥�Ļ�������С����λKB�����ӿ�etm_set_vod_cache_path ���������Ŀ¼������д�ռ䣬����3GB ����
ETDLL_API int32 etm_set_vod_cache_size(u32  cache_size)
{
	POST_PARA_1 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_set_vod_cache_size:%u",cache_size);

	EM_CHECK_CRITICAL_ERROR;

	if(cache_size==0)  return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)cache_size;
	
	//return em_post_function( vod_set_cache_size, (void *)&param,&(param._handle),&(param._result));
	return em_post_function( dt_set_vod_cache_size, (void *)&param,&(param._handle),&(param._result));
}
ETDLL_API u32  etm_get_vod_cache_size(void)
{
	POST_PARA_1 param;
	_u32 cache_size = 0;
	_int32 ret_val=SUCCESS;
	
	if(!g_already_init)
		return 0;
	
	EM_LOG_DEBUG("etm_get_vod_cache_size");

	if(em_get_critical_error()!=SUCCESS) return 0;

	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)&cache_size;
	
	//ret_val= em_post_function( vod_get_cache_size, (void *)&param,&(param._handle),&(param._result));
	ret_val= em_post_function( dt_get_vod_cache_size, (void *)&param,&(param._handle),&(param._result));
	if(ret_val==SUCCESS)
	{
		return cache_size;
	}
	
	return 0;
}
///// ǿ�����vod ����
ETDLL_API int32  etm_clear_vod_cache(void)
{
	POST_PARA_0 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_clear_vod_cache");

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(POST_PARA_0));
	
	return em_post_function(dt_clear_vod_cache, (void *)&param,&(param._handle),&(param._result));
}

/////���û���ʱ�䣬��λ��,Ĭ��Ϊ30�뻺��
ETDLL_API int32 etm_set_vod_buffer_time(u32 buffer_time)
{
	POST_PARA_1 param;

	if(!g_already_init)
	{
#if defined(ENABLE_NULL_PATH)
		if(!etm_is_et_running())
		{
		     return -1;
		}
#else
		     return -1;
#endif
	}

	EM_LOG_DEBUG("etm_set_vod_buffer_time:%u", buffer_time);

	EM_CHECK_CRITICAL_ERROR;

	if(buffer_time == 0)  return INVALID_ARGUMENT;

#if defined(ENABLE_NULL_PATH)
 	if(!g_already_init)
	{
               return et_vod_set_buffer_time(buffer_time);
 	}
	else
	{
		em_memset(&param,0,sizeof(POST_PARA_1));
		param._para1= (void*)buffer_time;

		return em_post_function(dt_set_vod_buffer_time, (void *)&param, &(param._handle), &(param._result));	
	}
#else
	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)buffer_time;

	return em_post_function(dt_set_vod_buffer_time, (void *)&param, &(param._handle), &(param._result));	
#endif
}


/////2.10 ���õ㲥��ר���ڴ��С,��λKB������2MB ����
ETDLL_API int32 etm_set_vod_buffer_size(u32  buffer_size)
{
	POST_PARA_1 param;
	
	if(!g_already_init)
	{
#if defined(ENABLE_NULL_PATH)
		if(!etm_is_et_running())
		{
		     return -1;
		}
#else
		     return -1;
#endif
	}
	
	EM_LOG_DEBUG("etm_set_vod_buffer_size:%u",buffer_size);

	EM_CHECK_CRITICAL_ERROR;

	if(buffer_size==0)  return INVALID_ARGUMENT;
	if(buffer_size<10240) buffer_size = 10240;  /* ���õ㲥�ڴ�����Ϊ10MB  by zyq @20130119 */
#if defined(ENABLE_NULL_PATH)
  	if(!g_already_init)
	{
                 return et_vod_set_vod_buffer_size(buffer_size*1024);
  	}
	else
	{
		em_memset(&param,0,sizeof(POST_PARA_1));
		param._para1= (void*)buffer_size;
		
		//return em_post_function( vod_set_buffer_size, (void *)&param,&(param._handle),&(param._result));
		return em_post_function( dt_set_vod_buffer_size, (void *)&param,&(param._handle),&(param._result));
	}
#else

	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)buffer_size;
	
	//return em_post_function( vod_set_buffer_size, (void *)&param,&(param._handle),&(param._result));
	return em_post_function( dt_set_vod_buffer_size, (void *)&param,&(param._handle),&(param._result));
#endif
}
ETDLL_API u32  etm_get_vod_buffer_size(void)
{
	POST_PARA_1 param;
	_u32 buffer_size = 0;
	_int32 ret_val=SUCCESS;
	
	if(!g_already_init)
	{
#if defined(ENABLE_NULL_PATH)
		if(!etm_is_et_running())
		{
		     return -1;
		}
#else
		     return -1;
#endif
	}
	
	EM_LOG_DEBUG("etm_get_vod_buffer_size");

	if(em_get_critical_error()!=SUCCESS) return 0;
#if defined(ENABLE_NULL_PATH)
        //how?
  	if(!g_already_init)
	{
	       ret_val = et_vod_get_vod_buffer_size(&buffer_size);
	       if(SUCCESS == ret_val)
	       {
	              return buffer_size;
	       }
		else
		{
		       return 0;
		}
  	}
	else
	{
		em_memset(&param,0,sizeof(POST_PARA_1));
		param._para1= (void*)&buffer_size;
		
		//ret_val= em_post_function( vod_get_buffer_size, (void *)&param,&(param._handle),&(param._result));
		ret_val= em_post_function( dt_get_vod_buffer_size, (void *)&param,&(param._handle),&(param._result));
		if(ret_val==SUCCESS)
		{
			return buffer_size;
		}
		
		return 0;
	}
#else
	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)&buffer_size;
	
	//ret_val= em_post_function( vod_get_buffer_size, (void *)&param,&(param._handle),&(param._result));
	ret_val= em_post_function( dt_get_vod_buffer_size, (void *)&param,&(param._handle),&(param._result));
	if(ret_val==SUCCESS)
	{
		return buffer_size;
	}
	
	return 0;
#endif	
}

/////2.11 ��ѯvod ר���ڴ��Ƿ��Ѿ�����
ETDLL_API BOOL etm_is_vod_buffer_allocated(void)
{
	POST_PARA_1 param;
	BOOL is_allocated = FALSE;
	_int32 ret_val=SUCCESS;
	
	if(!g_already_init)
	{
#if defined(ENABLE_NULL_PATH)
		if(!etm_is_et_running())
		{
		     return FALSE;
		}
#else
		     return FALSE;
#endif
	}
	
	EM_LOG_DEBUG("etm_is_vod_buffer_allocated");

	if(em_get_critical_error()!=SUCCESS) return FALSE;
#if defined(ENABLE_NULL_PATH)
   	if(!g_already_init)
	{
	     ret_val = et_vod_is_vod_buffer_allocated(&is_allocated);
	      if(SUCCESS == ret_val)
	      	{
	      	     return is_allocated;
	      	}
		else
		{
		     return FALSE;
		}
   	}
	else
	{
		em_memset(&param,0,sizeof(POST_PARA_1));
		param._para1= (void*)&is_allocated;
		
		//ret_val= em_post_function( vod_is_buffer_allocated, (void *)&param,&(param._handle),&(param._result));
		ret_val= em_post_function( dt_is_vod_buffer_allocated, (void *)&param,&(param._handle),&(param._result));
		if(ret_val==SUCCESS)
		{
			return is_allocated;
		}
		
		return FALSE;
	}
#else
	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)&is_allocated;
	
	//ret_val= em_post_function( vod_is_buffer_allocated, (void *)&param,&(param._handle),&(param._result));
	ret_val= em_post_function( dt_is_vod_buffer_allocated, (void *)&param,&(param._handle),&(param._result));
	if(ret_val==SUCCESS)
	{
		return is_allocated;
	}
	
	return FALSE;
#endif	
}

/*2.12 �ֹ��ͷ�vod ר���ڴ�,ETM �����ڷ���ʼ��ʱ���Զ��ͷ�����ڴ棬
�������������뾡���ڳ�����ڴ�Ļ����ɵ��ô˽ӿڣ�ע�����֮ǰҪȷ���޵㲥����������
*/
ETDLL_API int32  etm_free_vod_buffer(void)
{
	POST_PARA_0 param;
	
	if(!g_already_init)
	{
#if defined(ENABLE_NULL_PATH)
		if(!etm_is_et_running())
		{
		     return -1;
		}
#else
		     return -1;
#endif
	}
	
	EM_LOG_DEBUG("etm_free_vod_buffer");

	EM_CHECK_CRITICAL_ERROR;
#if defined(ENABLE_NULL_PATH)
   	if(!g_already_init)
	{
               return et_vod_free_vod_buffer();
   	}
	else
	{
		em_memset(&param,0,sizeof(POST_PARA_0));
		
		//return em_post_function(vod_free_buffer, (void *)&param,&(param._handle),&(param._result));
		return em_post_function(dt_free_vod_buffer, (void *)&param,&(param._handle),&(param._result));
	}
#else
	em_memset(&param,0,sizeof(POST_PARA_0));
	
	//return em_post_function(vod_free_buffer, (void *)&param,&(param._handle),&(param._result));
	return em_post_function(dt_free_vod_buffer, (void *)&param,&(param._handle),&(param._result));
#endif
}


/////2.8 ����2.9 ~2.14 ��Ĭ��ϵͳ����: max_tasks=5,download_limit_speed=-1,upload_limit_speed=-1,auto_limit_speed=FALSE,max_task_connection=128,task_auto_start=FALSE
ETDLL_API int32 	etm_load_default_settings(void)
{
	POST_PARA_0 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_load_default_settings");

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(POST_PARA_0));
	
	return em_post_function(em_load_default_settings, (void *)&param,&(param._handle),&(param._result));
}


/////2.9 �������������,��СΪ1,���Ϊ15
ETDLL_API int32 etm_set_max_tasks(u32 task_num)
{
	POST_PARA_1 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_set_max_tasks:%u",task_num);

	EM_CHECK_CRITICAL_ERROR;

	if((task_num==0)||(task_num>MAX_ALOW_TASKS)) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)task_num;
	
	return em_post_function( em_set_max_tasks, (void *)&param,&(param._handle),&(param._result));
}

ETDLL_API u32 etm_get_max_tasks(void)
{
	POST_PARA_1 param;
	_u32 task_num = 0;
	_int32 ret_val=SUCCESS;
	
	if(!g_already_init)
		return 0;
	
	EM_LOG_DEBUG("etm_get_max_tasks");

	if(em_get_critical_error()!=SUCCESS) return 0;

	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)&task_num;
	
	ret_val= em_post_function( em_get_max_tasks, (void *)&param,&(param._handle),&(param._result));
	if(ret_val==SUCCESS)
	{
		sd_assert(task_num>0);
		sd_assert(task_num<16);
		return task_num;
	}
	
	return 0;
}

/////2.10 ������������,��KB Ϊ��λ,����-1Ϊ������,����Ϊ0
ETDLL_API int32 etm_set_download_limit_speed(u32 max_speed)
{
	static POST_PARA_1 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_set_download_limit_speed:%u",max_speed);

	EM_CHECK_CRITICAL_ERROR;

	if(max_speed==0) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)max_speed;
	
	return em_post_function_unlock( em_set_download_limit_speed, (void *)&param,&(param._handle),&(param._result));
}
ETDLL_API u32 etm_get_download_limit_speed(void)
{
	POST_PARA_1 param;
	_u32 download_limit_speed = 0;
	_int32 ret_val=SUCCESS;
	
	if(!g_already_init)
		return 0;
	
	EM_LOG_DEBUG("etm_get_download_limit_speed");

	if(em_get_critical_error()!=SUCCESS) return 0;

	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)&download_limit_speed;
	
	ret_val= em_post_function( em_get_download_limit_speed, (void *)&param,&(param._handle),&(param._result));
	if(ret_val==SUCCESS)
	{
		sd_assert(download_limit_speed>0);
		return download_limit_speed;
	}
	
	return 0;
}

/////2.11 �����ϴ�����,��KB Ϊ��λ,����-1Ϊ������,����Ϊ0
ETDLL_API int32 etm_set_upload_limit_speed(u32 max_speed)
{
	static POST_PARA_1 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_set_upload_limit_speed:%u",max_speed);

	EM_CHECK_CRITICAL_ERROR;

	if(max_speed==0) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)max_speed;
	
	return em_post_function_unlock( em_set_upload_limit_speed, (void *)&param,&(param._handle),&(param._result));
}
ETDLL_API u32 etm_get_upload_limit_speed(void)
{
	POST_PARA_1 param;
	_u32 upload_limit_speed = 0;
	_int32 ret_val=SUCCESS;
	
	if(!g_already_init)
		return 0;
	
	EM_LOG_DEBUG("etm_get_upload_limit_speed");

	if(em_get_critical_error()!=SUCCESS) return 0;

	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)&upload_limit_speed;
	
	ret_val= em_post_function( em_get_upload_limit_speed, (void *)&param,&(param._handle),&(param._result));
	if(ret_val==SUCCESS)
	{
		sd_assert(upload_limit_speed>0);
		return upload_limit_speed;
	}
	
	return 0;
}

/////2.12 �����������٣�������root Ȩ������Ѹ�׵�ʱ�������Ч
ETDLL_API int32 etm_set_auto_limit_speed(BOOL auto_limit)
{
	POST_PARA_1 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_set_auto_limit_speed:%d",auto_limit);

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)auto_limit;
	
	return em_post_function( em_set_auto_limit_speed, (void *)&param,&(param._handle),&(param._result));
}
ETDLL_API BOOL etm_get_auto_limit_speed(void)
{
	POST_PARA_1 param;
	BOOL auto_limit_speed = FALSE;
	_int32 ret_val=SUCCESS;
	
	if(!g_already_init)
		return FALSE;
	
	EM_LOG_DEBUG("etm_get_auto_limit_speed");

	if(em_get_critical_error()!=SUCCESS) return FALSE;

	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)&auto_limit_speed;
	
	ret_val= em_post_function( em_get_auto_limit_speed, (void *)&param,&(param._handle),&(param._result));
	if(ret_val==SUCCESS)
	{
		return auto_limit_speed;
	}
	
	return FALSE;
}

/////2.13 �������������,10M �汾ȡֵ��ΧΪ[1~200]
ETDLL_API int32 etm_set_max_task_connection(u32 connection_num)
{
	POST_PARA_1 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_set_max_task_connection:%u",connection_num);

	EM_CHECK_CRITICAL_ERROR;

	if((connection_num==0)||(connection_num>2000)) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)connection_num;
	
	return em_post_function( em_set_max_task_connection, (void *)&param,&(param._handle),&(param._result));
}
ETDLL_API u32 etm_get_max_task_connection(void)
{
	POST_PARA_1 param;
	_u32 max_task_connection = 0;
	_int32 ret_val=SUCCESS;
	
	if(!g_already_init)
		return 0;
	
	EM_LOG_DEBUG("etm_get_max_task_connection");

	if(em_get_critical_error()!=SUCCESS) return 0;

	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)&max_task_connection;
	
	ret_val= em_post_function( em_get_max_task_connection, (void *)&param,&(param._handle),&(param._result));
	if(ret_val==SUCCESS)
	{
		sd_assert(max_task_connection>0);
		return max_task_connection;
	}
	
	return 0;
}

/////2.14 ����ETM �������Զ���ʼ��������
ETDLL_API int32 etm_set_task_auto_start(BOOL auto_start)
{
	POST_PARA_1 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_set_task_auto_start:%d",auto_start);

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)auto_start;
	
	return em_post_function( em_set_task_auto_start, (void *)&param,&(param._handle),&(param._result));
}
ETDLL_API BOOL etm_get_task_auto_start(void)
{
	POST_PARA_1 param;
	BOOL task_auto_start = FALSE;
	_int32 ret_val=SUCCESS;
	
	if(!g_already_init)
		return FALSE;
	
	EM_LOG_DEBUG("etm_get_task_auto_start");

	if(em_get_critical_error()!=SUCCESS) return FALSE;

	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)&task_auto_start;
	
	ret_val= em_post_function( em_get_task_auto_start, (void *)&param,&(param._handle),&(param._result));
	if(ret_val==SUCCESS)
	{
		return task_auto_start;
	}
	
	return FALSE;
}
/////2.20 ��ȡ��ǰȫ���������ٶ�,��Byte Ϊ��λ
ETDLL_API u32 etm_get_current_download_speed(void)
	
	{
		POST_PARA_1 param;
		_u32 current_download_speed = 0;
		_int32 ret_val=SUCCESS;
#if defined(MOBILE_PHONE)
		_u32 cur_flow = 0;
		static _u32 old_flow = 0;
#endif			
		
		if(!g_already_init)
			return 0;
		
		EM_LOG_DEBUG("etm_get_current_download_speed");
	
		if(em_get_critical_error()!=SUCCESS) return 0;

#if defined(MOBILE_PHONE)
		if(sd_get_net_type()>0&&sd_get_net_type()<NT_WLAN)
		{
			ret_val= em_get_network_flow(&cur_flow,NULL);
			if(ret_val==SUCCESS)
			{
				if(cur_flow>old_flow)
				{
					current_download_speed = cur_flow - old_flow;
				}
				else
				{
					current_download_speed = 0;
				}
				old_flow = cur_flow;
				return current_download_speed;
			}
		}
		else
#endif			
		{
			em_memset(&param,0,sizeof(POST_PARA_1));
			param._para1= (void*)&current_download_speed;
			
			ret_val= em_post_function( dt_get_current_download_speed, (void *)&param,&(param._handle),&(param._result));
			if(ret_val==SUCCESS)
			{
				return current_download_speed;
			}
		}
		return 0;
	}


/////2.21 ��ȡ��ǰȫ���ϴ����ٶ�,��Byte Ϊ��λ
ETDLL_API u32 etm_get_current_upload_speed(void)
	
	{
		POST_PARA_1 param;
		_u32 current_upload_speed = 0;
		_int32 ret_val=SUCCESS;
		
		if(!g_already_init)
			return 0;
		
		EM_LOG_DEBUG("etm_get_current_upload_speed");
	
		if(em_get_critical_error()!=SUCCESS) return 0;
	
		em_memset(&param,0,sizeof(POST_PARA_1));
		param._para1= (void*)&current_upload_speed;
		
		ret_val= em_post_function( dt_get_current_upload_speed, (void *)&param,&(param._handle),&(param._result));
		if(ret_val==SUCCESS)
		{
			return current_upload_speed;
		}
		
		return 0;
	}

/////2.22 �������طֶδ�С,[100~1000]����λKB
ETDLL_API int32 etm_set_download_piece_size(u32 piece_size)
{
	POST_PARA_1 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_set_download_piece_size:%u",piece_size);

	EM_CHECK_CRITICAL_ERROR;

	if((piece_size<100)||(piece_size>1000)) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)piece_size;
	
	return em_post_function( em_set_download_piece_size, (void *)&param,&(param._handle),&(param._result));
}
ETDLL_API u32 etm_get_download_piece_size(void)
{
	POST_PARA_1 param;
	_u32 piece_size = 0;
	_int32 ret_val=SUCCESS;
	
	if(!g_already_init)
		return 300;
	
	EM_LOG_DEBUG("etm_get_download_piece_size");

	if(em_get_critical_error()!=SUCCESS) return 300;

	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)&piece_size;
	
	ret_val= em_post_function( em_get_download_piece_size, (void *)&param,&(param._handle),&(param._result));
	if(ret_val==SUCCESS)
	{
		sd_assert((piece_size>=100)&&(piece_size<=1000));
		return piece_size;
	}
	
	return 300;
}
/////2.22 ���ý���汾��
ETDLL_API int32 etm_set_ui_version(const char *ui_version, int32 product,int32 partner_id)
{
	POST_PARA_3 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_set_ui_version:%s,product=%d,partner_id=%d",ui_version,product,partner_id);

	EM_CHECK_CRITICAL_ERROR;

	if((ui_version==NULL)||(em_strlen(ui_version)==0)) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_3));
	param._para1= (void*)ui_version;
	param._para2= (void*)product;
	param._para3= (void*)partner_id;
	
	return em_post_function( em_set_ui_version, (void *)&param,&(param._handle),&(param._result));
}

ETDLL_API int32 etm_get_business_type_by_product_id(int32 product_id)
{
	if( 0 == product_id ) 
		return INVALID_ARGUMENT;
	return em_get_business_type(product_id); 
}

ETDLL_API int32 etm_reporter_mobile_user_action_to_file(u32 action_type, u32 action_value, void *data, u32 data_len)
{
	POST_PARA_4 param;

	if(!g_already_init)
		return -1;

	EM_LOG_DEBUG("etm_reporter_mobile_user_action_to_file:%u", action_type);

	EM_CHECK_CRITICAL_ERROR;

	if((data == NULL) && (data_len != 0)) return INVALID_ARGUMENT;

	em_memset(&param, 0, sizeof(POST_PARA_4));
	param._para1 = (void*)action_type;
	param._para2 = (void*)action_value;
	param._para3 = data;
	param._para4 = (void*)data_len;

	return em_post_function(em_reporter_mobile_user_action_to_file, (void *)&param, &(param._handle), &(param._result));
}

/* http ��ʽ�ϱ����ϱ�data����Ϊ��ֵ�Ը�ʽ:
	u=aaaa&u1=bbbb&u2=cccc&u3=dddd&u4=eeeee
����: u=mh_chanel&u1={�ֻ��˰汾��}&u2={peerid}&u3={�ֻ���IMEI}&u4={ʱ��unixʱ��}&u5={Ƶ������}
*/
ETDLL_API int32 etm_http_report(char *data, u32 data_len)
{
	POST_PARA_2 param;

	if(!g_already_init)
		return -1;

	EM_LOG_DEBUG("etm_http_report");

	EM_CHECK_CRITICAL_ERROR;
	
	sd_assert(data!=NULL);
	sd_assert(data_len>36);
	if((data == NULL) || (data_len <36)) return INVALID_ARGUMENT;

	em_memset(&param, 0, sizeof(POST_PARA_2));
	param._para1 = (void*)data;
	param._para2 = (void*)data_len;

	return em_post_function(em_http_report, (void *)&param, &(param._handle), &(param._result));
}

/* http ��ʽ�ϱ����ϱ�data����Ϊ��ֵ�Ը�ʽ:
	u=aaaa&u1=bbbb&u2=cccc&u3=dddd&u4=eeeee
����: http://host:port/u=mh_chanel&u1={�ֻ��˰汾��}&u2={peerid}&u3={�ֻ���IMEI}&u4={ʱ��unixʱ��}&u5={Ƶ������}
ui ƴ���Լ����ϱ�url
*/
ETDLL_API int32 etm_http_report_by_url(char *data, u32 data_len)
{
	POST_PARA_2 param;

	if(!g_already_init)
		return -1;

	EM_LOG_DEBUG("etm_http_report");

	EM_CHECK_CRITICAL_ERROR;
	
	if((data == NULL) || (data_len <16)) return INVALID_ARGUMENT;

	em_memset(&param, 0, sizeof(POST_PARA_2));
	param._para1 = (void*)data;
	param._para2 = (void*)data_len;

	return em_post_function(em_http_report_by_url, (void *)&param, &(param._handle), &(param._result));
}


ETDLL_API int32 etm_enable_user_action_report(BOOL is_enable)
{
	POST_PARA_1 param;

	if(!g_already_init)
		return -1;

	EM_LOG_DEBUG("etm_reporter_mobile_enable_user_action_report, is enable:%d", is_enable);

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)is_enable;

	return em_post_function(em_reporter_mobile_enable_user_action_report, (void *)&param, &(param._handle), &(param._result));
}

/////2.23 �������������ʾ����������:0 -�ر�,1-��,3-��,4-�����龰ģʽ
ETDLL_API int32 etm_set_prompt_tone_mode(u32 mode)
{
	POST_PARA_1 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_set_prompt_tone_mode:%u",mode);

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)mode;
	
	return em_post_function( em_set_prompt_tone_mode, (void *)&param,&(param._handle),&(param._result));
}
ETDLL_API u32 etm_get_prompt_tone_mode(void)
{
	POST_PARA_1 param;
	_u32 mode = 0;
	_int32 ret_val=SUCCESS;
	
	if(!g_already_init)
		return 0;
	
	EM_LOG_DEBUG("etm_get_prompt_tone_mode");

	if(em_get_critical_error()!=SUCCESS) return 0;

	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)&mode;
	
	ret_val= em_post_function( em_get_prompt_tone_mode, (void *)&param,&(param._handle),&(param._result));
	if(ret_val==SUCCESS)
	{
		return mode;
	}
	
	return 0;
}

/////2.24 ����p2p ����ģʽ:0 -��,1-��WIFI�����´�,2-�ر�
ETDLL_API int32 etm_set_p2p_mode(u32 mode)
{
	POST_PARA_1 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_set_p2p_mode:%u",mode);

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)mode;
	
	return em_post_function( em_set_p2p_mode, (void *)&param,&(param._handle),&(param._result));
}
ETDLL_API u32 etm_get_p2p_mode(void)
{
	POST_PARA_1 param;
	_u32 mode = 0;
	_int32 ret_val=SUCCESS;
	
	if(!g_already_init)
		return 0;
	
	EM_LOG_DEBUG("etm_get_p2p_mode");

	if(em_get_critical_error()!=SUCCESS) return 0;

	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)&mode;
	
	ret_val= em_post_function( em_get_p2p_mode, (void *)&param,&(param._handle),&(param._result));
	if(ret_val==SUCCESS)
	{
		return mode;
	}
	
	return 0;
}
/////2.25 �������������CDN ʹ�ò���:
// enable - ʹ�ò�����Ч,enable_cdn_speed - ���������ٶ�С�ڸ�ֵʱ����CDN,disable_cdn_speed -- �����ٶȼ�ȥCDN�ٶȴ��ڸ�ֵʱ�ر�CDN
ETDLL_API int32 etm_set_cdn_mode(BOOL enable,_u32 enable_cdn_speed,_u32 disable_cdn_speed)
{
	POST_PARA_3 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_set_cdn_mode:%d,enable_cdn_speed=%u,disable_cdn_speed=%u",enable,enable_cdn_speed,disable_cdn_speed);

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(POST_PARA_3));
	param._para1= (void*)enable;
	param._para2= (void*)enable_cdn_speed;
	param._para3= (void*)disable_cdn_speed;
	
	return em_post_function( em_set_cdn_mode, (void *)&param,&(param._handle),&(param._result));
}

/* *item_value Ϊһ��in/out put ���������Ҳ�����item_name ��Ӧ��������*item_value ΪĬ��ֵ�������� ,   item_name ��item_value�Ϊ63�ֽ�*/
ETDLL_API int32  etm_settings_set_str_item(char * item_name,char *item_value)
{
	
	if(!g_already_init)
		return -1;
	
	if((item_name==NULL)||(item_value==NULL)) return INVALID_ARGUMENT;
	
	EM_LOG_DEBUG("etm_settings_set_str_item:%s=%s",item_name,item_value);

	EM_CHECK_CRITICAL_ERROR;

	return em_settings_set_str_item(item_name, item_value);
}
ETDLL_API int32  etm_settings_get_str_item(char * item_name,char *item_value)
{
	
	if(!g_already_init)
		return -1;
	
	if((item_name==NULL)||(item_value==NULL)) return INVALID_ARGUMENT;
	
	EM_LOG_DEBUG("etm_settings_get_str_item:%s",item_name);

	EM_CHECK_CRITICAL_ERROR;

	return em_settings_get_str_item(item_name, item_value);

}

ETDLL_API int32  etm_settings_set_int_item(char * item_name,int32 item_value)
{
	if(!g_already_init)
		return -1;
	
	if(item_name==NULL) return INVALID_ARGUMENT;
	
	EM_LOG_DEBUG("etm_settings_set_int_item:%s=%d",item_name,item_value);

	EM_CHECK_CRITICAL_ERROR;

	return em_settings_set_int_item(item_name, item_value);
}
ETDLL_API int32  etm_settings_get_int_item(char * item_name,int32 *item_value)
{
	if(!g_already_init)
		return -1;
	
	if((item_name==NULL)||(item_value==NULL)) return INVALID_ARGUMENT;
	
	EM_LOG_DEBUG("etm_settings_get_int_item:%s",item_name);

	EM_CHECK_CRITICAL_ERROR;

	return em_settings_get_int_item(item_name, item_value);
}

ETDLL_API int32  etm_settings_set_bool_item(char * item_name,Bool item_value)
{
	
	if(!g_already_init)
		return -1;
	
	if(item_name==NULL) return INVALID_ARGUMENT;
	
	EM_LOG_DEBUG("etm_settings_set_bool_item:%s=%d",item_name,item_value);

	EM_CHECK_CRITICAL_ERROR;

	return em_settings_set_bool_item(item_name, item_value);

}
ETDLL_API int32  etm_settings_get_bool_item(char * item_name,Bool *item_value)
{
	if(!g_already_init)
		return -1;
	
	if((item_name==NULL)||(item_value==NULL)) return INVALID_ARGUMENT;
	
	EM_LOG_DEBUG("etm_settings_get_bool_item:%s",item_name);

	EM_CHECK_CRITICAL_ERROR;

	return em_settings_get_bool_item(item_name, item_value);
}

/*��� ǿ���滻���������Ӧ��ip��ַ,���ؿⲻ�ٶԸ�������dns, ���ʹ�ö�Ӧ��ip��ַ(���ж��)*/
ETDLL_API int32  etm_set_host_ip(const char * host_name,const char *ip)
{
	POST_PARA_2 param;
	if(!g_already_init)
		return -1;
	
	if((host_name==NULL) ||(ip==NULL)||(em_strlen(host_name)==0)||(em_strlen(ip)==0)) return INVALID_ARGUMENT;
	
	EM_LOG_DEBUG("etm_set_host_ip:%s=%s",host_name,ip);

	EM_CHECK_CRITICAL_ERROR;
	em_memset(&param, 0, sizeof(POST_PARA_2));
	param._para1= (void*)host_name;
	param._para2= (void*)ip;
	
	return em_post_function( em_set_host_ip, (void *)&param,&(param._handle),&(param._result));
}

/* ���������ӵ�����ip */
ETDLL_API int32  etm_clean_host(void)
{
	POST_PARA_0 param;
	if(!g_already_init)
		return -1;
		
	EM_LOG_DEBUG("etm_clean_host");

	EM_CHECK_CRITICAL_ERROR;
	em_memset(&param, 0, sizeof(POST_PARA_0));
	
	return em_post_function( em_clear_host_ip, (void *)&param,&(param._handle),&(param._result));
}
///////////////////////////////////////////////////////////////
/////3�������

/////1 ������������
ETDLL_API int32 etm_create_task(ETM_CREATE_TASK * p_param,u32* p_task_id )
{
	POST_PARA_3 param;
    BOOL thread_unlock = TRUE;
	if(!g_already_init)
		return -1;
	
	if((p_param==NULL)||(p_task_id==NULL)) return INVALID_ARGUMENT;
	
	EM_LOG_DEBUG("etm_create_task:%d",p_param->_type);
#ifdef LITTLE_FILE_TEST2
	       EM_LOG_URGENT("****etm_create_task:%d,%s",p_param->_type,p_param->_url);
#endif /* LITTLE_FILE_TEST2 */

	EM_CHECK_CRITICAL_ERROR;


	em_memset(&param,0,sizeof(POST_PARA_3));
	param._para1= (void*)p_param;
	param._para2= (void*)p_task_id;
	param._para3= (void*)thread_unlock;
	
	return em_post_function( dt_create_task, (void *)&param,&(param._handle),&(param._result));

}


/* ����: ͨ��td.cfg�ļ�·����������
*/
ETDLL_API int32 etm_create_task_by_cfg_file(char* cfg_file_path, u32* p_task_id)
{
	POST_PARA_2 param;
 
	if(!g_already_init)
		return -1;
	
	if((cfg_file_path==NULL)||(p_task_id==NULL)) return INVALID_ARGUMENT;
	
	EM_LOG_DEBUG("etm_create_task_by_cfg_file:cfg_path = %s", cfg_file_path);

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(POST_PARA_2));
	param._para1= (void*)cfg_file_path;
	param._para2= (void*)p_task_id;
	
	return em_post_function( dt_create_task_by_cfg_file, (void *)&param,&(param._handle),&(param._result));

}

/* ����: �滻�Ѵ���δ������������ԭʼurl��
	1. ��ɾ����ԭ������(��ɾ���ļ�)��2. �޸Ķ�Ӧcfg�ļ��洢��ԭʼurl�� 3. ���µ�url��������
����:
	new_origin_url: ���滻���µ�ԭʼurl��
	p_task_id: �������ָ�룬������ֵ��ֵ��Ӧ������Ҫ�滻url��ԭʼ���������id�� ���´���֮���ֵ���յ��µ�����id��
*/
ETDLL_API int32 etm_use_new_url_create_task(char *new_origin_url, u32* p_task_id)
{
	POST_PARA_5 param;
 	int32 ret_val = SUCCESS;
	char userdata[MAX_USER_DATA_LEN]= {0};
	u32 userdata_len = MAX_USER_DATA_LEN;
	if(!g_already_init)
		return -1;
	
	if( (NULL == new_origin_url) || (NULL == p_task_id) ) return INVALID_ARGUMENT;
	
	EM_LOG_DEBUG("etm_use_new_url_create_task:new_origin_url = %s, p_task_id = %u", new_origin_url, *p_task_id);

	EM_CHECK_CRITICAL_ERROR;

	if(0 == *p_task_id)
		return INVALID_ARGUMENT;
	// ��ȡ��Ӧ������Ϣ
	ETM_TASK_INFO task_info = {0};
	ret_val = etm_get_task_info(*p_task_id, &task_info);
	if(SUCCESS != ret_val)
		return ret_val;
	// ��ȡ�����Ӧ��userdata
	ret_val = etm_get_task_user_data(*p_task_id, NULL, &userdata_len);
	if( (SUCCESS != ret_val) && (102411 != ret_val) )
		return ret_val;
	ret_val = etm_get_task_user_data(*p_task_id, userdata, &userdata_len);
	if(SUCCESS != ret_val)
		return ret_val;
	// ɾ����Ӧ����
	ret_val = etm_destroy_task(*p_task_id, FALSE);
	if(SUCCESS != ret_val)
		return ret_val;

	em_memset(&param,0,sizeof(POST_PARA_5));
	param._para1= (void*)new_origin_url;
	param._para2= (void*)&task_info;
	param._para3= (void*)p_task_id;
	param._para4= (void*)userdata;
	param._para5= (void*)userdata_len;
	
	return em_post_function( dt_use_new_url_create_task, (void *)&param,&(param._handle),&(param._result));

}
/////1.1 �����̳�����
ETDLL_API int32 etm_create_shop_task(ETM_CREATE_SHOP_TASK * p_param,u32* p_task_id)
{
#ifdef ENABLE_ETM_MEDIA_CENTER
	POST_PARA_3 param;
    BOOL thread_unlock = TRUE;
	
	if(!g_already_init)
		return -1;
	
	if((p_param==NULL)||(p_task_id==NULL)) return INVALID_ARGUMENT;
	sd_assert(sd_strlen(p_param->_url)>9);
	EM_LOG_DEBUG("etm_create_shop_task:%s",p_param->_url);

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(POST_PARA_3));
	param._para1= (void*)p_param;
	param._para2= (void*)p_task_id;
	param._para3= (void*)thread_unlock;
	
	return em_post_function( dt_create_shop_task, (void *)&param,&(param._handle),&(param._result));
#else
	return NOT_IMPLEMENT;
#endif
}


/////2 ��ʼ��������ͣ������
ETDLL_API int32 etm_resume_task(u32 task_id)
{
	POST_PARA_3 param;
    BOOL thread_unlock = TRUE;
	BOOL exact_errcode = FALSE;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_resume_task:%u",task_id);

	EM_CHECK_CRITICAL_ERROR;

	if(task_id==0) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_2));
	param._para1= (void*)task_id;
	param._para2= (void*)exact_errcode;
	param._para3= (void*)thread_unlock;
	
	return em_post_function( dt_start_task, (void *)&param,&(param._handle),&(param._result));

}


ETDLL_API int32 etm_vod_resume_task(u32 task_id)
{
	POST_PARA_2 param;
    BOOL thread_unlock = TRUE;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_vod_resume_task:%u",task_id);

	EM_CHECK_CRITICAL_ERROR;

	if(task_id==0) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_2));
	param._para1= (void*)task_id;
	param._para2= (void*)thread_unlock;
	
	return em_post_function( dt_vod_start_task, (void *)&param,&(param._handle),&(param._result));

}


/////3 ֹͣ����ͣ������
ETDLL_API int32 etm_pause_task (u32 task_id)
{
	POST_PARA_2 param;
    BOOL thread_unlock = TRUE;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_pause_task:%u",task_id);

	EM_CHECK_CRITICAL_ERROR;

	if(task_id==0) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_2));
	param._para1= (void*)task_id;
	param._para2= (void*)thread_unlock;
	
	return em_post_function( dt_stop_task, (void *)&param,&(param._handle),&(param._result));

}


/////4 ������������վ
ETDLL_API int32 etm_delete_task(u32 task_id)
{
	POST_PARA_2 param;
    BOOL thread_unlock = TRUE;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_delete_task:%u",task_id);

	EM_CHECK_CRITICAL_ERROR;

	if(task_id==0) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_2));
	param._para1= (void*)task_id;
	param._para2= (void*)thread_unlock;
	
	return em_post_function( dt_delete_task, (void *)&param,&(param._handle),&(param._result));

}


/////5 ��ԭ��ɾ��������
ETDLL_API int32 etm_recover_task(u32 task_id)
{
	POST_PARA_1 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_recover_task:%u",task_id);

	EM_CHECK_CRITICAL_ERROR;

	if(task_id==0) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)task_id;
	
	return em_post_function( dt_recover_task, (void *)&param,&(param._handle),&(param._result));

}

/////6 ����ɾ������
ETDLL_API int32 etm_destroy_task(u32 task_id,BOOL delete_file)
{
	POST_PARA_3 param;
    BOOL thread_unlock = TRUE;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_destroy_task:%u,%d",task_id,delete_file);

	EM_CHECK_CRITICAL_ERROR;

	if(task_id==0) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_3));
	param._para1= (void*)task_id;
	param._para2= (void*)delete_file;
	param._para3= (void*)thread_unlock;
	
	return em_post_function( dt_destroy_task, (void *)&param,&(param._handle),&(param._result));

}

/////7 ��ȡ�����������ȼ�task_id �б�
ETDLL_API int32 etm_get_task_pri_id_list (u32 * id_array_buffer,u32 *buffer_len)
{
	POST_PARA_2 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_get_task_pri_id_list");

	EM_CHECK_CRITICAL_ERROR;

	//if((id_array_buffer==NULL)||(buffer_len==NULL)||(*buffer_len==0)) return INVALID_ARGUMENT;
	if(buffer_len==NULL) return INVALID_ARGUMENT;

	if(id_array_buffer!=NULL)
	{
		em_memset(id_array_buffer,0,(*buffer_len)*sizeof(_u32));
	}
	em_memset(&param,0,sizeof(POST_PARA_2));
	param._para1= (void*)id_array_buffer;
	param._para2= (void*)buffer_len;
	
	return em_post_function( dt_get_pri_id_list, (void *)&param,&(param._handle),&(param._result));

}
/////8 �������������ȼ�����
ETDLL_API int32 etm_task_pri_level_up(u32 task_id,u32 steps)
{
	POST_PARA_2 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_task_pri_level_up:%u,%u",task_id,steps);

	EM_CHECK_CRITICAL_ERROR;

	if((task_id==0)||(steps==0)) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_2));
	param._para1= (void*)task_id;
	param._para2= (void*)steps;
	
	return em_post_function( dt_pri_level_up, (void *)&param,&(param._handle),&(param._result));

}
	
/////9 �������������ȼ�����
ETDLL_API int32 etm_task_pri_level_down (u32 task_id,u32 steps)
{
	POST_PARA_2 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_task_pri_level_down:%u,%u",task_id,steps);

	EM_CHECK_CRITICAL_ERROR;

	if((task_id==0)||(steps==0)) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_2));
	param._para1= (void*)task_id;
	param._para2= (void*)steps;
	
	return em_post_function( dt_pri_level_down, (void *)&param,&(param._handle),&(param._result));

}

/////10 �����������ļ���ֻ��p2sp������Ч����������bt����
ETDLL_API int32 etm_rename_task(u32 task_id,const char * new_name,u32 new_name_len)
{
	POST_PARA_3 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_rename_task:%u,new_name[%u]=%s",task_id,new_name_len,new_name);

	EM_CHECK_CRITICAL_ERROR;

	if((task_id==0)||(new_name==NULL)||(em_strlen(new_name)==0)||(new_name_len==0)) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_3));
	param._para1= (void*)task_id;
	param._para2= (void*)new_name;
	param._para3= (void*)new_name_len;
	
	return em_post_function( dt_rename_task, (void *)&param,&(param._handle),&(param._result));

}


/////11 ��ȡ�������Ϣ
ETDLL_API int32 etm_get_task_info(u32 task_id,ETM_TASK_INFO *p_task_info)
{
	POST_PARA_3 param;
    BOOL thread_unlock = TRUE;
#if defined(ENABLE_NULL_PATH)
       int32  ret_val = SUCCESS;
#endif
	
	if(!g_already_init)
#if defined(ENABLE_NULL_PATH)
		if(!etm_is_et_running())
		{
		     return -1;
		}
#else
		     return -1;
#endif
	
	EM_LOG_DEBUG("etm_get_task_info:%u",task_id);

	EM_CHECK_CRITICAL_ERROR;
	
	
	if((task_id==0)||(p_task_info==NULL)) return INVALID_ARGUMENT;

#if defined(ENABLE_NULL_PATH)
      if(VOD_NODISK_TASK_MASK == (task_id&VOD_NODISK_TASK_MASK))
      {
	    em_memset(p_task_info,0,sizeof(ETM_TASK_INFO));
	    em_memset(&param,0,sizeof(POST_PARA_3));
	    param._para1= (void*)(task_id-VOD_NODISK_TASK_MASK);
	    param._para2= (void*)p_task_info;
           ret_val = dt_et_get_task_info(&param);
          return ret_val;
      }
      else
      {
	  	if(g_already_init)
		{
			em_memset(p_task_info,0,sizeof(ETM_TASK_INFO));
			em_memset(&param,0,sizeof(POST_PARA_3));
			param._para1= (void*)task_id;
			param._para2= (void*)p_task_info;
			param._para3= (void*)thread_unlock;
			
			//if(task_id>MAX_DL_TASK_ID)
			//{
			//	return em_post_function( vod_get_task_info, (void *)&param,&(param._handle),&(param._result));
			//}
			//else
			//{
				return em_post_function( dt_get_task_info, (void *)&param,&(param._handle),&(param._result));
			//}
	  	}
		else
		{
		      return -1;
		}
      }
#else
	em_memset(p_task_info,0,sizeof(ETM_TASK_INFO));
	em_memset(&param,0,sizeof(POST_PARA_3));
	param._para1= (void*)task_id;
	param._para2= (void*)p_task_info;
	param._para3= (void*)thread_unlock;
	
	if(task_id>VOD_NODISK_TASK_MASK)
	{
		return em_post_function( vod_get_task_info, (void *)&param,&(param._handle),&(param._result));
	}
	else
	{
		return em_post_function( dt_get_task_info, (void *)&param,&(param._handle),&(param._result));
	}
#endif	

}

ETDLL_API int32 etm_get_task_download_info(u32 task_id,ETM_TASK_INFO *p_task_info)
{
	POST_PARA_3 param;
    BOOL thread_unlock = TRUE;
#if defined(ENABLE_NULL_PATH)
       int32  ret_val = SUCCESS;
#endif
	
	if(!g_already_init)
#if defined(ENABLE_NULL_PATH)
		if(!etm_is_et_running())
		{
		     return -1;
		}
#else
		     return -1;
#endif
	
	EM_LOG_DEBUG("etm_get_task_download_info:%u",task_id);

	EM_CHECK_CRITICAL_ERROR;
	
	
	if((task_id==0)||(p_task_info==NULL)) return INVALID_ARGUMENT;

#if defined(ENABLE_NULL_PATH)
      if(VOD_NODISK_TASK_MASK == (task_id&VOD_NODISK_TASK_MASK))
      {
	    em_memset(p_task_info,0,sizeof(ETM_TASK_INFO));
	    em_memset(&param,0,sizeof(POST_PARA_3));
	    param._para1= (void*)(task_id-VOD_NODISK_TASK_MASK);
	    param._para2= (void*)p_task_info;
           ret_val = dt_et_get_task_info(&param);
          return ret_val;
      }
      else
      {
	  	if(g_already_init)
		{
			em_memset(p_task_info,0,sizeof(ETM_TASK_INFO));
			em_memset(&param,0,sizeof(POST_PARA_3));
			param._para1= (void*)task_id;
			param._para2= (void*)p_task_info;
			param._para3= (void*)thread_unlock;
			
			//if(task_id>MAX_DL_TASK_ID)
			//{
			//	return em_post_function( vod_get_task_info, (void *)&param,&(param._handle),&(param._result));
			//}
			//else
			//{
				return em_post_function( dt_get_task_download_info, (void *)&param,&(param._handle),&(param._result));
			//}
	  	}
		else
		{
		      return -1;
		}
      }
#else
	em_memset(p_task_info,0,sizeof(ETM_TASK_INFO));
	em_memset(&param,0,sizeof(POST_PARA_3));
	param._para1= (void*)task_id;
	param._para2= (void*)p_task_info;
	param._para3= (void*)thread_unlock;
	
	if(task_id>VOD_NODISK_TASK_MASK)
	{
		return em_post_function( vod_get_task_info, (void *)&param,&(param._handle),&(param._result));
	}
	else
	{
		return em_post_function( dt_get_task_download_info, (void *)&param,&(param._handle),&(param._result));
	}
#endif

}

/////12 ��ȡ�������е� ���������״��
ETDLL_API int32 etm_get_task_running_status(u32 task_id,ETM_RUNNING_STATUS *p_status)
{
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_get_task_running_status:%u",task_id);

	sd_assert(sizeof(RUNNING_STATUS)==sizeof(ETM_RUNNING_STATUS));

	EM_CHECK_CRITICAL_ERROR;

	if((task_id==0)||(p_status==NULL)) return INVALID_ARGUMENT;

	em_memset(p_status,0,sizeof(ETM_RUNNING_STATUS));

	if(task_id>VOD_NODISK_TASK_MASK)
	{
		return  vod_get_task_running_status(task_id,(RUNNING_STATUS*)p_status);
	}
	else
	{
		return  dt_get_task_running_status(task_id,(RUNNING_STATUS*)p_status);
	}
	
}



/////13 ��ȡ����ΪETT_URL,ETT_EMULE,ETT_LAN �� �����URL
ETDLL_API const char* etm_get_task_url(u32 task_id)
{
	POST_PARA_3 param;
	static char url[MAX_URL_LEN];
	_int32 ret_val=SUCCESS;
    BOOL thread_unlock = TRUE;

	if(!g_already_init)
		return NULL;
	
	EM_LOG_DEBUG("etm_get_task_url:%u",task_id);

	ETM_CHECK_CRITICAL_ERROR;

	if(task_id==0) return NULL;

	em_memset(url,0,MAX_URL_LEN);
	em_memset(&param,0,sizeof(POST_PARA_3));
	param._para1= (void*)task_id;
	param._para2= (void*)url;
	param._para3= (void*)thread_unlock;

	ret_val= em_post_function( dt_get_task_url, (void *)&param,&(param._handle),&(param._result));
	if(ret_val==SUCCESS)
	{
		sd_assert(em_strlen(url)>0);
		EM_LOG_DEBUG("etm_get_task_url:%u,%s",task_id,url);
		return url;
	}
	
	EM_LOG_DEBUG("etm_get_task_url:%u,no url!",task_id);
	return NULL;
}

/////�滻����ΪETT_LAN�� �����URL
ETDLL_API int32 etm_set_task_url(u32 task_id,const char * url)
{
	POST_PARA_2 param;
	_int32 url_len = 0;
	
	if(!g_already_init)
	return -1;
	
	EM_LOG_DEBUG("etm_set_task_url:%u,%s",task_id,url);
	sd_assert(sd_strlen(url)>9);

	EM_CHECK_CRITICAL_ERROR;

	if(task_id==0||url==NULL) return INVALID_ARGUMENT;

	url_len = em_strlen(url);
	if(url_len==0||url_len>=MAX_URL_LEN) return INVALID_URL;

	em_memset(&param,0,sizeof(param));
	param._para1= (void*)task_id;
	param._para2= (void*)url;
	
	return em_post_function( dt_set_task_url, (void *)&param,&(param._handle),&(param._result));

}


/////14 ��ȡ����ΪETT_URL �� ���������ҳ��URL
ETDLL_API const char* etm_get_task_ref_url(u32 task_id)
{
	POST_PARA_3 param;
	static char url[MAX_URL_LEN];
	_int32 ret_val=SUCCESS;
    BOOL thread_unlock = TRUE;

	if(!g_already_init)
		return NULL;
	
	EM_LOG_DEBUG("etm_get_task_ref_url:%u",task_id);

	ETM_CHECK_CRITICAL_ERROR;

	if(task_id==0) return NULL;

	em_memset(url,0,MAX_URL_LEN);
	em_memset(&param,0,sizeof(POST_PARA_3));
	param._para1= (void*)task_id;
	param._para2= (void*)url;
	param._para3= (void*)thread_unlock;

	ret_val= em_post_function( dt_get_task_ref_url, (void *)&param,&(param._handle),&(param._result));
	if(ret_val==SUCCESS)
	{
		sd_assert(em_strlen(url)>0);
		return url;
	}
	
	return NULL;
}

ETDLL_API const char* etm_get_task_tcid(u32 task_id)
{
	POST_PARA_2 param;
	static char cid[CID_SIZE*2+1];
	_int32 ret_val=SUCCESS;
	
	if(!g_already_init)
		return NULL;
	
	EM_LOG_DEBUG("etm_get_task_tcid:%u",task_id);

	ETM_CHECK_CRITICAL_ERROR;

	if(task_id==0) return NULL;

	em_memset(cid,0,CID_SIZE*2+1);
	em_memset(&param,0,sizeof(POST_PARA_2));
	param._para1= (void*)task_id;
	param._para2= (void*)cid;
	
	ret_val= em_post_function( dt_get_task_tcid, (void *)&param,&(param._handle),&(param._result));
	if(ret_val==SUCCESS)
	{
		sd_assert(em_strlen(cid)>0);
		return cid;
	}
	
	return NULL;
}

ETDLL_API const char* etm_get_bt_task_sub_file_tcid(u32 task_id, u32 file_index)
{
	POST_PARA_3 param;
	static char cid[CID_SIZE*2+1];
	_int32 ret_val=SUCCESS;
	
	if(!g_already_init)
		return NULL;
	
	EM_LOG_DEBUG("etm_get_bt_task_sub_file_tcid:%u,%u",task_id,file_index);

	ETM_CHECK_CRITICAL_ERROR;

	if(task_id==0) return NULL;

	em_memset(cid,0,CID_SIZE*2+1);
	em_memset(&param,0,sizeof(POST_PARA_3));
	param._para1= (void*)task_id;
	param._para2= (void*)file_index;
	param._para3= (void*)cid;
	
	ret_val= em_post_function( dt_get_bt_task_sub_file_tcid, (void *)&param,&(param._handle),&(param._result));
	if(ret_val==SUCCESS)
	{
		sd_assert(em_strlen(cid)>0);
		return cid;
	}
	
	return NULL;
}

ETDLL_API const char* etm_get_file_tcid(const char * file_path)
{
	static char cid_str[CID_SIZE*2+1] = {0};
	_u8 cid_u8[CID_SIZE]={0};
	_int32 ret_val=SUCCESS;
	
	
	EM_LOG_DEBUG("etm_get_file_tcid:%s",file_path);

	if(file_path==NULL||sd_strlen(file_path)<2) return NULL;

	em_memset(cid_str,0,CID_SIZE*2+1);
	
	ret_val= sd_calc_file_cid( file_path,cid_u8);
	if(ret_val==SUCCESS)
	{
		ret_val = em_str2hex((char*)cid_u8, CID_SIZE, cid_str, CID_SIZE*2+1);
		if((ret_val==SUCCESS)&&(em_strlen(cid_str)>0))
		{
			EM_LOG_DEBUG("etm_get_file_tcid:%s,cid=%s",file_path,cid_str);
			return cid_str;
		}
	}
	
	return NULL;
}


/////16 ��ȡ����ΪETT_KANKAN �� �����gcid
ETDLL_API const char* etm_get_task_gcid(u32 task_id)
{
	POST_PARA_2 param;
	static char cid[CID_SIZE*2+1];
	_int32 ret_val=SUCCESS;
	
	if(!g_already_init)
		return NULL;
	
	EM_LOG_DEBUG("etm_get_task_gcid:%u",task_id);

	ETM_CHECK_CRITICAL_ERROR;

	if(task_id==0) return NULL;

	em_memset(cid,0,CID_SIZE*2+1);
	em_memset(&param,0,sizeof(POST_PARA_2));
	param._para1= (void*)task_id;
	param._para2= (void*)cid;
	
	ret_val= em_post_function( dt_get_task_gcid, (void *)&param,&(param._handle),&(param._result));
	if(ret_val==SUCCESS)
	{
		sd_assert(em_strlen(cid)>0);
		return cid;
	}
	
	return NULL;
}


/////17 ��ȡ����ΪETT_BT �� ����������ļ�·��
ETDLL_API const char* etm_get_bt_task_seed_file(u32 task_id)
{
	POST_PARA_3 param;
	static char seed_file[MAX_FULL_PATH_BUFFER_LEN];
	_int32 ret_val=SUCCESS;
    BOOL thread_unlock = TRUE;
	
	if(!g_already_init)
		return NULL;
	
	EM_LOG_DEBUG("etm_get_bt_task_seed_file:%u",task_id);

	ETM_CHECK_CRITICAL_ERROR;

	if(task_id==0) return NULL;

	em_memset(seed_file,0,MAX_FULL_PATH_BUFFER_LEN);
	em_memset(&param,0,sizeof(POST_PARA_3));
	param._para1= (void*)task_id;
	param._para2= (void*)seed_file;
	param._para3= (void*)thread_unlock;
	
	ret_val= em_post_function( dt_get_bt_task_seed_file, (void *)&param,&(param._handle),&(param._result));
	if(ret_val==SUCCESS)
	{
		sd_assert(em_strlen(seed_file)>0);
		return seed_file;
	}
	
	return NULL;
}

/////18 ��ȡBT������ĳ���ļ�����Ϣ
ETDLL_API int32 etm_get_bt_file_info(u32 task_id, u32 file_index, ETM_BT_FILE *file_info)
{
	POST_PARA_4 param;
    BOOL thread_unlock = TRUE;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_get_bt_file_info:%u",task_id);

	EM_CHECK_CRITICAL_ERROR;

	if((task_id==0)||(file_info==NULL)||(file_index>=MAX_FILE_INDEX)) return INVALID_ARGUMENT;

	em_memset(file_info,0,sizeof(ETM_BT_FILE));
	em_memset(&param,0,sizeof(POST_PARA_4));
	param._para1= (void*)task_id;
	param._para2= (void*)file_index;
	param._para3= (void*)file_info;
	param._para4= (void*)thread_unlock;
	
	//if(task_id>MAX_DL_TASK_ID)
	//{
	//	return em_post_function( vod_get_bt_file_info, (void *)&param,&(param._handle),&(param._result));
	//}
	//else
	//{
		return em_post_function( dt_get_bt_file_info, (void *)&param,&(param._handle),&(param._result));
	//}
}

ETDLL_API const char* etm_get_bt_task_sub_file_name(u32 task_id, u32 file_index)
{
	POST_PARA_3 param;
	static char file_name[MAX_FILE_NAME_BUFFER_LEN];
	_int32 ret_val=SUCCESS;
	
	if(!g_already_init)
		return NULL;
	
	EM_LOG_DEBUG("etm_get_bt_task_sub_file_name:%u,%u",task_id,file_index);

	ETM_CHECK_CRITICAL_ERROR;

	if(task_id==0) return NULL;

	em_memset(file_name,0,MAX_FILE_NAME_BUFFER_LEN);
	em_memset(&param,0,sizeof(POST_PARA_3));
	param._para1= (void*)task_id;
	param._para2= (void*)file_index;
	param._para3= (void*)file_name;
	
	ret_val= em_post_function( dt_get_bt_task_sub_file_name, (void *)&param,&(param._handle),&(param._result));
	if(ret_val==SUCCESS)
	{
		sd_assert(em_strlen(file_name)>0);
		return file_name;
	}
	
	return NULL;
}


/////19 �޸�BT�������費��Ҫ���ص��ļ�,���֮ǰ��Ҫ�µ��ļ���Ȼ��Ҫ���صĻ���ҲҪ����new_file_index_array��
ETDLL_API int32 etm_set_bt_need_download_file_index(u32 task_id, u32* new_file_index_array,	u32 new_file_num)
{
	POST_PARA_3 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_set_bt_need_download_file_index:%u",task_id);

	EM_CHECK_CRITICAL_ERROR;

	if((task_id==0)||(new_file_index_array==NULL)||(new_file_num==0)||(new_file_num>=MAX_FILE_NUM)) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_3));
	param._para1= (void*)task_id;
	param._para2= (void*)new_file_index_array;
	param._para3= (void*)new_file_num;
	
	return em_post_function( dt_set_bt_need_download_file_index, (void *)&param,&(param._handle),&(param._result));
}

ETDLL_API int32 etm_get_bt_need_download_file_index(u32 task_id, u32* id_array_buffer,	u32 * buffer_len) /* *buffer_len�ĵ�λ��sizeof(u32) */
{
	POST_PARA_3 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_get_bt_need_download_file_index:%u",task_id);

	EM_CHECK_CRITICAL_ERROR;

	if((task_id==0)||(buffer_len==NULL)) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_3));
	param._para1= (void*)task_id;
	param._para2= (void*)id_array_buffer;
	param._para3= (void*)buffer_len;
	
	return em_post_function( dt_get_bt_need_download_file_index, (void *)&param,&(param._handle),&(param._result));
}

/////20 ��ȡ������û���������
ETDLL_API _int32 etm_get_task_user_data(u32 task_id,void * data_buffer,u32 * buffer_len)
{
	POST_PARA_3 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_get_task_user_data:%u",task_id);

	EM_CHECK_CRITICAL_ERROR;

	//if((task_id==0)||(data_buffer==NULL)||(buffer_len==NULL)||(*buffer_len==0)) return INVALID_ARGUMENT;
	if((task_id==0)||(buffer_len==NULL)) return INVALID_ARGUMENT;

	if( data_buffer!=NULL)
	{
		em_memset(data_buffer,0,*buffer_len);
	}
	em_memset(&param,0,sizeof(POST_PARA_3));
	param._para1= (void*)task_id;
	param._para2= (void*)data_buffer;
	param._para3= (void*)buffer_len;
	
	return em_post_function( dt_get_task_user_data, (void *)&param,&(param._handle),&(param._result));
}

/////21 ��������״̬��ȡ����id �б�
ETDLL_API int32 	etm_get_task_id_by_state(ETM_TASK_STATE state,u32 * id_array_buffer,u32 *buffer_len)
{
	POST_PARA_4 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_get_task_id_by_state:%d",state);

	EM_CHECK_CRITICAL_ERROR;

	//if((id_array_buffer==NULL)||(buffer_len==NULL)||(*buffer_len==0)) return INVALID_ARGUMENT;
	if(buffer_len==NULL) return INVALID_ARGUMENT;

	if(id_array_buffer!=NULL)
	{
		em_memset(id_array_buffer,0,(*buffer_len)*sizeof(_u32));
	}
	em_memset(&param,0,sizeof(POST_PARA_4));
	param._para1= (void*)state;
	param._para2= (void*)id_array_buffer;
	param._para3= (void*)buffer_len;
	param._para4= (void*)FALSE;
	
	return em_post_function( dt_get_task_id_by_state, (void *)&param,&(param._handle),&(param._result));
}
/////3.22 ��������״̬��ȡ���ش���������id �б�(������Զ�̿��ƴ���������),*buffer_len�ĵ�λ��sizeof(u32)
ETDLL_API int32 	etm_get_local_task_id_by_state(ETM_TASK_STATE state,u32 * id_array_buffer,u32 *buffer_len)
{
	POST_PARA_4 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_get_task_id_by_state:%d",state);

	EM_CHECK_CRITICAL_ERROR;

	//if((id_array_buffer==NULL)||(buffer_len==NULL)||(*buffer_len==0)) return INVALID_ARGUMENT;
	if(buffer_len==NULL) return INVALID_ARGUMENT;

	if(id_array_buffer!=NULL)
	{
		em_memset(id_array_buffer,0,(*buffer_len)*sizeof(_u32));
	}
	em_memset(&param,0,sizeof(POST_PARA_4));
	param._para1= (void*)state;
	param._para2= (void*)id_array_buffer;
	param._para3= (void*)buffer_len;
	param._para4= (void*)TRUE;
	
	return em_post_function( dt_get_task_id_by_state, (void *)&param,&(param._handle),&(param._result));
}

/////3.23 ��ȡ���е�����id �б�,*buffer_len�ĵ�λ��sizeof(u32),���������̺����̵㲥����(�������̵㲥����תΪ����ģʽ�������)
ETDLL_API int32 	etm_get_all_task_ids(u32 * id_array_buffer,u32 *buffer_len)
{
	POST_PARA_2 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_get_all_task_ids");

	EM_CHECK_CRITICAL_ERROR;

	//if((id_array_buffer==NULL)||(buffer_len==NULL)||(*buffer_len==0)) return INVALID_ARGUMENT;
	if(buffer_len==NULL) return INVALID_ARGUMENT;

	if(id_array_buffer!=NULL)
	{
		em_memset(id_array_buffer,0,(*buffer_len)*sizeof(_u32));
	}
	em_memset(&param,0,sizeof(POST_PARA_2));
	param._para1= (void*)id_array_buffer;
	param._para2= (void*)buffer_len;
	
	return em_post_function( dt_get_all_task_ids, (void *)&param,&(param._handle),&(param._result));
}

ETDLL_API uint64 etm_get_all_task_total_file_size(void)
{
	POST_PARA_1 param;
	int32 ret_val = SUCCESS;
	uint64 total_file_size = 0;
	
	if(!g_already_init)
		return 0;
	
	EM_LOG_DEBUG("etm_get_all_task_total_file_size");

	if(em_get_critical_error()!=SUCCESS) return 0;
	
	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)&total_file_size;
	
	ret_val = em_post_function( dt_get_all_task_total_file_size, (void *)&param,&(param._handle),&(param._result));
	if(ret_val == SUCCESS)
	{
		return total_file_size;
	}
	else
	{
		return 0;
	}
}

ETDLL_API uint64 etm_get_all_task_need_space(void)
{
	POST_PARA_1 param;
	int32 ret_val = SUCCESS;
	uint64 need_space = 0;
	
	if(!g_already_init)
		return 0;
	
	EM_LOG_DEBUG("etm_get_all_task_need_space");

	if(em_get_critical_error()!=SUCCESS) return 0;
	
	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)&need_space;
	
	ret_val = em_post_function( dt_get_all_task_need_space, (void *)&param,&(param._handle),&(param._result));
	if(ret_val == SUCCESS)
	{
		return need_space;
	}
	else
	{
		return 0;
	}
}

///// ��ȡ���񱾵ص㲥URL,ֻ�������˱���http �㲥������ʱ���ܵ���
ETDLL_API const char* etm_get_task_local_vod_url(u32 task_id)
{
#ifdef ENABLE_HTTP_VOD
	_int32 ret_val=SUCCESS;
	_u32 task_inner_id = 0;
	static char url[64],buffer[16];
	uint64 position = 0;
	ETM_TASK_INFO task_info = {0};
	
	if(!g_already_init)
		return NULL;
	
	EM_LOG_DEBUG("etm_get_task_local_vod_url:%X",task_id);

	ETM_CHECK_CRITICAL_ERROR;

	if(task_id==0) return NULL;


	ret_val = etm_get_task_info( task_id,&task_info);
	if(ret_val!=SUCCESS||task_info._file_size==0)
	{
		return NULL;
	}

	if(VOD_NODISK_TASK_MASK<task_id)
	{
		/* no disk task */
		task_inner_id = task_id - VOD_NODISK_TASK_MASK;
	}
	else
	{
		ret_val = etm_read_vod_file( task_id, 0, 16, buffer, 1);
		ret_val = dt_get_running_et_task_id(task_id, &task_inner_id);
	}
	
	etm_vod_get_download_position( task_id, &position );
	
#if 0 //def ENABLE_FLASH_PLAYER
	if(ret_val==SUCCESS)
	{
		_u32 bitrate = 0;
		ret_val = etm_vod_get_bitrate( task_id, INVALID_FILE_INDEX, &bitrate );
		if(bitrate==0) ret_val = -1;
	}
#endif /* ENABLE_FLASH_PLAYER */

	if(ret_val==SUCCESS)
	{
		_int32 port= DEFAULT_LOCAL_HTTP_SERVER_PORT;
		sd_memset(url, 0, sizeof(url));
		em_settings_get_int_item("system.http_server_port", (_int32*)&port);
		sd_snprintf(url, sizeof(url)-1, "http://127.0.0.1:%u/%u.flv",port, task_inner_id);
		EM_LOG_DEBUG("etm_get_task_local_vod_url end:task_id=%X,url=%s",task_id,url);
		return url;
	}
	EM_LOG_DEBUG("etm_get_task_local_vod_url ERROR end:%d,task_id=%X",ret_val,task_id);
#endif /* ENABLE_HTTP_VOD */	
	return NULL;
}

/////3.23 ����server ��Դ,file_indexֻ��BT�������õ�,��ͨ����file_index�ɺ���
ETDLL_API int32 etm_add_server_resource( u32 task_id, ETM_SERVER_RES * p_resource )
{
	POST_PARA_2 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_add_server_resource:%u",task_id);

	EM_CHECK_CRITICAL_ERROR;

	if(p_resource==NULL) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_2));
	param._para1= (void*)task_id;
	param._para2= (void*)p_resource;
	
	return em_post_function( em_add_server_resource, (void *)&param,&(param._handle),&(param._result));
}


/////3.24 ����peer ��Դ,file_indexֻ��BT�������õ�,��ͨ����file_index�ɺ���
ETDLL_API int32 etm_add_peer_resource( u32 task_id, ETM_PEER_RES  * p_resource )
{
	POST_PARA_2 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_add_peer_resource:%u",task_id);

	EM_CHECK_CRITICAL_ERROR;

	if(p_resource==NULL) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_2));
	param._para1= (void*)task_id;
	param._para2= (void*)p_resource;
	
	return em_post_function( em_add_peer_resource, (void *)&param,&(param._handle),&(param._result));
}

/////��ȡ����ΪETT_LAN�� �����peer�����Ϣ
ETDLL_API int32 etm_get_peer_resource( u32 task_id, ETM_PEER_RES  * p_resource )
{
	POST_PARA_2 param;
	
	if(!g_already_init)
	return -1;
	
	EM_LOG_DEBUG("etm_get_peer_resource:%u",task_id);

	EM_CHECK_CRITICAL_ERROR;

	if(p_resource==NULL) return INVALID_ARGUMENT;

	em_memset(&p_resource,0,sizeof(p_resource));
	em_memset(&param,0,sizeof(POST_PARA_2));
	param._para1= (void*)task_id;
	param._para2= (void*)p_resource;
	
	return em_post_function( em_get_peer_resource, (void *)&param,&(param._handle),&(param._result));
}

ETDLL_API int32 etm_get_lixian_info(u32 task_id, u32 file_index, ETM_LIXIAN_INFO * p_lx_info)
{
#ifdef ENABLE_LIXIAN_ETM
	em_memset(p_lx_info,0x00,sizeof(ETM_LIXIAN_INFO));
	return em_get_lixian_info(  task_id, file_index, (void *) p_lx_info );
#else
	return NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM */
}

/* ���û��ȡ��������id */
ETDLL_API _int32 etm_set_lixian_task_id(u32 task_id, u32 file_index,uint64 lixian_task_id)
{
#ifdef ENABLE_LIXIAN_ETM
	POST_PARA_3 param;
	uint64 lixian_id = lixian_task_id;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_set_lixian_task_id:%u",task_id);

	EM_CHECK_CRITICAL_ERROR;

	if(task_id==0||lixian_task_id==0) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(param));
	param._para1= (void*)task_id;
	param._para2= (void*)file_index;
	param._para3= (void*)&lixian_id;
	
	return em_post_function( dt_set_lixian_task_id, (void *)&param,&(param._handle),&(param._result));
#else
	return NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM */
}

ETDLL_API int32 etm_get_lixian_task_id(u32 task_id, u32 file_index,uint64 * p_lixian_task_id)
{
#ifdef ENABLE_LIXIAN_ETM
	POST_PARA_3 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_get_lixian_task_id:%u",task_id);

	EM_CHECK_CRITICAL_ERROR;

	if(task_id==0||p_lixian_task_id==NULL) return INVALID_ARGUMENT;

	*p_lixian_task_id = 0;
	em_memset(&param,0,sizeof(param));
	param._para1= (void*)task_id;
	param._para2= (void*)file_index;
	param._para3= (void*)p_lixian_task_id;
	
	return em_post_function( dt_get_lixian_task_id, (void *)&param,&(param._handle),&(param._result));
#else
	return NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM */
}

/////3.25 ����URL ,cid,gicd ��BT���ӵ�info_hash ���Ҷ�Ӧ������id
ETDLL_API int32 	etm_get_task_id_by_eigenvalue(ETM_EIGENVALUE * p_eigenvalue,u32 * task_id)
{
	POST_PARA_2 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_get_task_id_by_eigenvalue:%d",p_eigenvalue->_type);

	EM_CHECK_CRITICAL_ERROR;

	if(p_eigenvalue==NULL||task_id==NULL) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_2));
	param._para1= (void*)p_eigenvalue;
	param._para2= (void*)task_id;
	
	return em_post_function( dt_get_task_id_by_eigenvalue, (void *)&param,&(param._handle),&(param._result));
}

ETDLL_API int32 etm_set_task_lixian_mode(u32 task_id,Bool is_lixian)
{
#ifdef ENABLE_LIXIAN_ETM
	POST_PARA_2 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_set_task_lixian_mode:%u,%d",task_id,is_lixian);

	EM_CHECK_CRITICAL_ERROR;

	if(task_id==0) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_2));
	param._para1= (void*)task_id;
	param._para2= (void*)is_lixian;
	
	return em_post_function( dt_set_task_lixian_mode, (void *)&param,&(param._handle),&(param._result));

#else
	return NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM */
}
ETDLL_API Bool etm_is_lixian_task(u32 task_id)
{
#ifdef ENABLE_LIXIAN_ETM
	POST_PARA_2 param;
	BOOL is_lixian = FALSE;
	_int32 ret_val=SUCCESS;
	
	if(!g_already_init)
		return FALSE;
	
	EM_LOG_DEBUG("etm_is_lixian_task:%u",task_id);

	//ETM_CHECK_CRITICAL_ERROR;

	if(task_id==0) return FALSE;

	em_memset(&param,0,sizeof(POST_PARA_2));
	param._para1= (void*)task_id;
	param._para2= (void*)&is_lixian;
	
	ret_val= em_post_function( dt_is_lixian_task, (void *)&param,&(param._handle),&(param._result));
	//sd_assert(ret_val==SUCCESS);
	
	return is_lixian;
#else
	return NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM */
}

////����������ýӿ�
ETDLL_API int32 etm_set_task_ad_mode(u32 task_id,Bool is_ad)
{
	POST_PARA_2 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_set_task_ad_mode:%u,%d",task_id,is_ad);

	EM_CHECK_CRITICAL_ERROR;

	if(task_id==0) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_2));
	param._para1= (void*)task_id;
	param._para2= (void*)is_ad;
	
	return em_post_function( dt_set_task_ad_mode, (void *)&param,&(param._handle),&(param._result));
}

ETDLL_API Bool etm_is_ad_task(u32 task_id)
{
	POST_PARA_2 param;
	BOOL is_ad = FALSE;
	_int32 ret_val=SUCCESS;
	
	if(!g_already_init)
		return FALSE;
	
	EM_LOG_DEBUG("etm_is_ad_task:%u",task_id);

	//ETM_CHECK_CRITICAL_ERROR;

	if(task_id==0) return FALSE;

	em_memset(&param,0,sizeof(POST_PARA_2));
	param._para1= (void*)task_id;
	param._para2= (void*)&is_ad;
	
	ret_val= em_post_function( dt_is_ad_task, (void *)&param,&(param._handle),&(param._result));
	//sd_assert(ret_val==SUCCESS);
	
	return is_ad;
}

/////ǿ�ƿ�ʼĳ������
ETDLL_API int32 etm_force_run_task(u32 task_id)
{
	POST_PARA_2 param;
    BOOL thread_unlock = TRUE;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_resume_task:%u",task_id);

	EM_CHECK_CRITICAL_ERROR;

	if(task_id==0) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_2));
	param._para1= (void*)task_id;
	param._para2= (void*)thread_unlock;
	
	return em_post_function( dt_run_task, (void *)&param,&(param._handle),&(param._result));
}

/*****************************************************************************************/
/******   ����ͨ����ؽӿ�            **************************/
ETDLL_API int32 etm_get_hsc_info(u32 task_id, u32 file_index, ETM_HSC_INFO * p_hsc_info)
{
#ifdef ENABLE_HSC
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_get_hsc_info:%u",task_id);

	EM_CHECK_CRITICAL_ERROR;

	if(task_id==0||p_hsc_info==NULL) return INVALID_ARGUMENT;

	em_memset(p_hsc_info,0,sizeof(ETM_HSC_INFO));
	
	return dt_get_hsc_info(task_id,file_index,(EM_HSC_INFO * )p_hsc_info);
#else
	return NOT_IMPLEMENT;
#endif /* ENABLE_HSC */
}
ETDLL_API int32 etm_is_high_speed_channel_usable( u32 task_id , u32 file_index,BOOL * p_usable)
{
#ifdef ENABLE_HSC
	*p_usable = FALSE;
	return dt_is_hsc_usable(  task_id ,  file_index,p_usable);
#else
	return NOT_IMPLEMENT;
#endif /* ENABLE_HSC */
}

/////3.28 ��������ͨ��
ETDLL_API int32 etm_open_high_speed_channel( u32 task_id, u32 file_index)
{
#ifdef ENABLE_HSC
	POST_PARA_2 param;
    BOOL thread_unlock = TRUE;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_open_high_speed_channel:%u",task_id);

	EM_CHECK_CRITICAL_ERROR;

	if(task_id==0) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_2));
	param._para1= (void*)task_id;
	param._para2= (void*)thread_unlock;
	
	return em_post_function( dt_open_high_speed_channel, (void *)&param,&(param._handle),&(param._result));
#else
	return NOT_IMPLEMENT;
#endif /* ENABLE_HSC */
}

///////////////////////////////////////////////////////////////
/// 4. ����洢����ؽӿ�

// �������һ����,����ɹ�*p_tree_id���������ڵ�id
ETDLL_API int32 	etm_open_tree(const char * file_path,_int32 flag ,u32 * p_tree_id)
{
	POST_PARA_3 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_open_tree:%s",file_path);

	EM_CHECK_CRITICAL_ERROR;

	if((em_strlen(file_path)==0)||(p_tree_id==NULL)) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_3));
	param._para1= (void*)file_path;
	param._para2= (void*)flag;
	param._para3= (void*)p_tree_id;
	
	return em_post_function(trm_open_tree, (void *)&param,&(param._handle),&(param._result));
}

// �ر�һ����
ETDLL_API int32 	etm_close_tree(u32 tree_id)
{
	POST_PARA_1 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_close_tree:%u",tree_id);

	EM_CHECK_CRITICAL_ERROR;

	if(tree_id<MAX_NODE_ID) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)tree_id;
	
	return em_post_function(trm_close_tree, (void *)&param,&(param._handle),&(param._result));
}

// ɾ��һ����
ETDLL_API int32 	etm_destroy_tree(const char * file_path)
{
	POST_PARA_1 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_destroy_tree:%s",file_path);

	EM_CHECK_CRITICAL_ERROR;

	if(file_path==NULL||em_strlen(file_path)==0) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)file_path;
	
	return em_post_function(trm_destroy_tree, (void *)&param,&(param._handle),&(param._result));
}

// �ж����Ƿ����
ETDLL_API Bool 	etm_tree_exist(const char * file_path)
{
	_int32 ret_val = SUCCESS;
	POST_PARA_2 param;
	Bool ret_b = FALSE;
	
	if(!g_already_init)
		return FALSE;
	
	EM_LOG_DEBUG("etm_destroy_tree:%s",file_path);

	if(em_get_critical_error()!=SUCCESS) return FALSE;

	if(file_path==NULL||em_strlen(file_path)==0) return FALSE;

	em_memset(&param,0,sizeof(POST_PARA_2));
	param._para1= (void*)file_path;
	param._para2= (void*)&ret_b;
	
	ret_val =  em_post_function(trm_tree_exist, (void *)&param,&(param._handle),&(param._result));
	if(ret_val == SUCCESS)
	{
		return ret_b;
	}
	return FALSE;
}

// ����һ����
ETDLL_API int32 	etm_copy_tree(const char * file_path,const char * new_file_path)
{
	return -1;
}

// 5.����һ���ڵ�,data_len(��λByte)��С������,�����鲻����256
ETDLL_API int32 	etm_create_node(u32 tree_id,u32 parent_id,const char * name, u32 name_len,void * data, _u32 data_len,u32 * p_node_id)
{
	POST_PARA_7 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_create_node:paren_id=%u",parent_id);

	EM_CHECK_CRITICAL_ERROR;

	if((tree_id<MAX_NODE_ID)||(parent_id==TRM_INVALID_NODE_ID)||(p_node_id==NULL)) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_7));
	param._para1= (void*)parent_id;
	param._para2= (void*)data;
	param._para3= (void*)data_len;
	param._para4= (void*)p_node_id;
	param._para5= (void*)tree_id;
	param._para6= (void*)name;
	param._para7= (void*)name_len;
	
	return em_post_function(trm_create_node, (void *)&param,&(param._handle),&(param._result));
}


// 6.ɾ��һ���ڵ㡣ע��:������node_id����֦��������������Ҷ�ڵ�ͬʱɾ��
ETDLL_API int32 	etm_delete_node(u32 tree_id,u32 node_id)
{
	POST_PARA_2 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_delete_node:%u",node_id);

	EM_CHECK_CRITICAL_ERROR;

	if((tree_id<MAX_NODE_ID)||(node_id<=TRM_ROOT_NODE_ID)||(node_id==tree_id)) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_2));
	param._para1= (void*)node_id;
	param._para2= (void*)tree_id;
	
	return em_post_function(trm_delete_node, (void *)&param,&(param._handle),&(param._result));
}

// 7.���ýڵ�����(��������),name_len����С��256 bytes
ETDLL_API int32 	etm_set_node_name(u32 tree_id,u32 node_id,const char * name, u32 name_len)
{
	POST_PARA_4 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_set_node_name:node_id=%u",node_id);

	EM_CHECK_CRITICAL_ERROR;

	if((tree_id<MAX_NODE_ID)||(node_id<=TRM_ROOT_NODE_ID)||(name_len>=MAX_FILE_NAME_BUFFER_LEN)||(node_id==tree_id)) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_4));
	param._para1= (void*)node_id;
	param._para2= (void*)name;
	param._para3= (void*)name_len;
	param._para4= (void*)tree_id;
	
	return em_post_function(trm_set_name, (void *)&param,&(param._handle),&(param._result));
}
// 8.��ȡ�ڵ�����
ETDLL_API const char * etm_get_node_name(u32 tree_id,u32 node_id)
{
	POST_PARA_3 param;
	static char name_buffer[MAX_FILE_NAME_BUFFER_LEN];
	_int32 ret_val = SUCCESS;
	
	if(!g_already_init)
		return NULL;
	
	EM_LOG_DEBUG("etm_get_node_name:node_id=%u",node_id);

	ETM_CHECK_CRITICAL_ERROR;

	if((tree_id<MAX_NODE_ID)||(node_id<=TRM_ROOT_NODE_ID)) return NULL;

	em_memset(name_buffer,0,MAX_FILE_NAME_BUFFER_LEN);
	em_memset(&param,0,sizeof(POST_PARA_3));
	param._para1= (void*)node_id;
	param._para2= (void*)name_buffer;
	param._para3= (void*)tree_id;
	
	ret_val = em_post_function(trm_get_name, (void *)&param,&(param._handle),&(param._result));
	if(ret_val == SUCCESS)
	{
		if(em_strlen(name_buffer)>0)
			return name_buffer;
	}
	return NULL;
}

// 9.���û��޸Ľڵ�����,new_data_len(��λByte)��С������,�����鲻����256
ETDLL_API int32 	etm_set_node_data(u32 tree_id,u32 node_id,void * new_data, _u32 new_data_len)
{
	POST_PARA_4 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_set_node_data:node_id=%u",node_id);

	EM_CHECK_CRITICAL_ERROR;

	if((tree_id<MAX_NODE_ID)||(node_id<=TRM_ROOT_NODE_ID)) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_4));
	param._para1= (void*)node_id;
	param._para2= (void*)new_data;
	param._para3= (void*)new_data_len;
	param._para4= (void*)tree_id;
	
	return em_post_function(trm_set_data, (void *)&param,&(param._handle),&(param._result));
}

// 10.��ȡ�ڵ�����,*buffer_len�ĵ�λ��Byte
ETDLL_API int32	etm_get_node_data(u32 tree_id,u32 node_id,void * data_buffer,_u32 * buffer_len)
{
	POST_PARA_4 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_get_node_data:node_id=%u",node_id);

	EM_CHECK_CRITICAL_ERROR;

	if((tree_id<MAX_NODE_ID)||(node_id<=TRM_ROOT_NODE_ID)|| (buffer_len==NULL) ) return INVALID_ARGUMENT;

	if(data_buffer!=NULL)
	{
		em_memset(data_buffer,0,*buffer_len);
	}
	em_memset(&param,0,sizeof(POST_PARA_4));
	param._para1= (void*)node_id;
	param._para2= (void*)data_buffer;
	param._para3= (void*)buffer_len;
	param._para4= (void*)tree_id;
	
	return em_post_function(trm_get_data, (void *)&param,&(param._handle),&(param._result));
}


// 3.�ƶ��ڵ�
///������ͬ����
ETDLL_API int32 	etm_set_node_parent(u32 tree_id,u32 node_id,u32 parent_id)
{
	POST_PARA_3 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_set_node_parent:node_id=%u,paren_id=%u",node_id,parent_id);

	EM_CHECK_CRITICAL_ERROR;

	if((tree_id<MAX_NODE_ID)||(node_id<=TRM_ROOT_NODE_ID)||(parent_id==TRM_INVALID_NODE_ID)||(node_id==parent_id)||(node_id==tree_id)) return INVALID_ARGUMENT;
	
	em_memset(&param,0,sizeof(POST_PARA_3));
	param._para1= (void*)node_id;
	param._para2= (void*)parent_id;
	param._para3= (void*)tree_id;
	
	return em_post_function(trm_set_parent, (void *)&param,&(param._handle),&(param._result));
}

// 4 ��ȡ���ڵ�id
ETDLL_API u32 	etm_get_node_parent(u32 tree_id,u32 node_id)
{
	POST_PARA_3 param;
	_u32 parent_id=ETM_INVALID_NODE_ID;
	_int32 ret_val=SUCCESS;
	
	if(!g_already_init)
		return ETM_INVALID_NODE_ID;
	
	EM_LOG_DEBUG("etm_get_node_parent:node_id=%u",node_id);

	if(em_get_critical_error()!=SUCCESS) return ETM_INVALID_NODE_ID;

	if((tree_id<MAX_NODE_ID)||(node_id<=TRM_ROOT_NODE_ID)||(node_id==tree_id)) return ETM_INVALID_NODE_ID;

	em_memset(&param,0,sizeof(POST_PARA_3));
	param._para1= (void*)node_id;
	param._para2= (void*)&parent_id;
	param._para3= (void*)tree_id;
	
	ret_val= em_post_function(trm_get_parent, (void *)&param,&(param._handle),&(param._result));
	if(ret_val==SUCCESS)
	{
		return parent_id;
	}

	return ETM_INVALID_NODE_ID;
}

///5 ͬһ�������ƶ�
ETDLL_API int32 	etm_node_move_up(u32 tree_id,u32 node_id,_u32 steps)
{
	POST_PARA_3 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_node_move_up:node_id=%u,steps=%u",node_id,steps);

	EM_CHECK_CRITICAL_ERROR;

	if((tree_id<MAX_NODE_ID)||(node_id<=TRM_ROOT_NODE_ID)||(steps==0)||(node_id==tree_id)) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_3));
	param._para1= (void*)node_id;
	param._para2= (void*)steps;
	param._para3= (void*)tree_id;
	
	return em_post_function(trm_move_up, (void *)&param,&(param._handle),&(param._result));
}

ETDLL_API int32 	etm_node_move_down(u32 tree_id,u32 node_id,_u32 steps)
{
	POST_PARA_3 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_node_move_down:node_id=%u,steps=%u",node_id,steps);

	EM_CHECK_CRITICAL_ERROR;

	if((tree_id<MAX_NODE_ID)||(node_id<=TRM_ROOT_NODE_ID)||(steps==0)||(node_id==tree_id)) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_3));
	param._para1= (void*)node_id;
	param._para2= (void*)steps;
	param._para3= (void*)tree_id;
	
	return em_post_function(trm_move_down, (void *)&param,&(param._handle),&(param._result));
}


//6 ��ȡ�����ӽڵ�id
ETDLL_API int32	etm_get_node_children(u32 tree_id,u32 node_id,_u32 * id_buffer,u32 * buffer_len)
{
	POST_PARA_4 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_get_node_children:node_id=%u",node_id);

	EM_CHECK_CRITICAL_ERROR;

	if((tree_id<MAX_NODE_ID)||(node_id==TRM_INVALID_NODE_ID)|| (buffer_len==NULL) ) return INVALID_ARGUMENT;
	if(id_buffer!=NULL)
	{
		em_memset(id_buffer,0,(*buffer_len)*sizeof(_u32));
	}
	em_memset(&param,0,sizeof(POST_PARA_4));
	param._para1= (void*)node_id;
	param._para2= (void*)id_buffer;
	param._para3= (void*)buffer_len;
	param._para4= (void*)tree_id;
	
	return em_post_function(trm_get_children, (void *)&param,&(param._handle),&(param._result));
}



// 15.��ȡ��һ���ӽڵ�id
ETDLL_API u32	etm_get_first_child(u32 tree_id,u32 parent_id)
{
	_int32 ret_val = SUCCESS;
	POST_PARA_3 param;
	_u32 node_id = ETM_INVALID_NODE_ID;
	
	if(!g_already_init)
		return ETM_INVALID_NODE_ID;
	
	EM_LOG_DEBUG("etm_get_first_child:tree_id=%u,parent_id=%u",tree_id,parent_id);

	if(em_get_critical_error()!=SUCCESS) return ETM_INVALID_NODE_ID;


	if((tree_id<MAX_NODE_ID)|| (parent_id==TRM_INVALID_NODE_ID) ) return ETM_INVALID_NODE_ID;
	em_memset(&param,0,sizeof(POST_PARA_3));
	param._para1= (void*)tree_id;
	param._para2= (void*)parent_id;
	param._para3= (void*)&node_id;
	
	ret_val= em_post_function(trm_get_first_child, (void *)&param,&(param._handle),&(param._result));
	if(ret_val==SUCCESS)
	{
		return node_id;
	}

	return ETM_INVALID_NODE_ID;
}

// 16.��ȡ�ֵܽڵ�id
ETDLL_API u32	etm_get_next_brother(u32 tree_id,u32 node_id)
{
	_int32 ret_val = SUCCESS;
	POST_PARA_3 param;
	_u32 bro_id = ETM_INVALID_NODE_ID;
	
	if(!g_already_init)
		return ETM_INVALID_NODE_ID;
	
	EM_LOG_DEBUG("etm_get_next_brother:tree_id=%u,node_id=%u",tree_id,node_id);

	if(em_get_critical_error()!=SUCCESS) return ETM_INVALID_NODE_ID;

	if((tree_id<MAX_NODE_ID)|| (node_id==TRM_INVALID_NODE_ID) ) return ETM_INVALID_NODE_ID;
	em_memset(&param,0,sizeof(POST_PARA_3));
	param._para1= (void*)tree_id;
	param._para2= (void*)node_id;
	param._para3= (void*)&bro_id;
	
	ret_val= em_post_function(trm_get_next_brother, (void *)&param,&(param._handle),&(param._result));
	if(ret_val==SUCCESS)
	{
		return bro_id;
	}

	return ETM_INVALID_NODE_ID;
}

ETDLL_API u32	etm_get_prev_brother(u32 tree_id,u32 node_id)
{
	_int32 ret_val = SUCCESS;
	POST_PARA_3 param;
	_u32 bro_id = ETM_INVALID_NODE_ID;
	
	if(!g_already_init)
		return ETM_INVALID_NODE_ID;
	
	EM_LOG_DEBUG("etm_get_prev_brother:tree_id=%u,node_id=%u",tree_id,node_id);

	if(em_get_critical_error()!=SUCCESS) return ETM_INVALID_NODE_ID;

	if((tree_id<MAX_NODE_ID)|| (node_id==TRM_INVALID_NODE_ID) ) return ETM_INVALID_NODE_ID;
	em_memset(&param,0,sizeof(POST_PARA_3));
	param._para1= (void*)tree_id;
	param._para2= (void*)node_id;
	param._para3= (void*)&bro_id;
	
	ret_val= em_post_function(trm_get_prev_brother, (void *)&param,&(param._handle),&(param._result));
	if(ret_val==SUCCESS)
	{
		return bro_id;
	}

	return ETM_INVALID_NODE_ID;
}


// 17.�������ֲ��ҽڵ�id���������ݲ�֧�ֲַ�ʽ��д������aaa.bbb.ccc��
// ���ص�һ��ƥ��Ľڵ�id���Ҳ�������ETM_INVALID_NODE_ID
ETDLL_API u32	etm_find_first_node_by_name(u32 tree_id,u32 parent_id,const char * name, u32 name_len)
{
	return etm_find_first_node(tree_id,parent_id,name,  name_len,NULL, 0);
}


// ������һ��ƥ��Ľڵ�id���Ҳ�������ETM_INVALID_NODE_ID
ETDLL_API u32	etm_find_next_node_by_name(u32 tree_id,u32 parent_id,u32 node_id,const char * name, u32 name_len)
{
	return etm_find_next_node(tree_id,parent_id,node_id,name,  name_len,NULL, 0);
}
// ������һ��ƥ��Ľڵ�id���Ҳ�������ETM_INVALID_NODE_ID
ETDLL_API u32	etm_find_prev_node_by_name(u32 tree_id,u32 parent_id,u32 node_id,const char * name, u32 name_len)
{
	return etm_find_prev_node(tree_id,parent_id,node_id,name,  name_len,NULL, 0);
}


// 18.���ݽڵ����ݲ��ҽڵ�,data_len��λByte.
/*
// ֻ���ص�һ��ƥ��Ľڵ�id���Ҳ�������ETM_INVALID_NODE_ID
ETDLL_API u32	etm_find_node_id_by_data(u32 parent_id,void * data, u32 data_len)
{
	POST_PARA_4 param;
	_u32 node_id = TRM_INVALID_NODE_ID; 
	_int32 ret_val = SUCCESS;
	
	if(!g_already_init)
		return node_id;
	
	EM_LOG_DEBUG("etm_find_node_id_by_data");

	if(em_get_critical_error()!=SUCCESS) return node_id;

	if(parent_id==TRM_INVALID_NODE_ID) return node_id;

	em_memset(&param,0,sizeof(POST_PARA_4));
	param._para1= (void*)parent_id;
	param._para2= (void*)data;
	param._para3= (void*)data_len;
	param._para4= (void*)&node_id;
	
	ret_val =  em_post_function(trm_find_node_id_by_data, (void *)&param,&(param._handle),&(param._result));
	if(ret_val!=SUCCESS)
	{
		return TRM_INVALID_NODE_ID;
	}
	return node_id;
}
*/
// ���ص�һ��ƥ��Ľڵ�id���Ҳ�������ETM_INVALID_NODE_ID
ETDLL_API u32	etm_find_first_node_by_data(u32 tree_id,u32 parent_id,void * data, u32 data_len)
{
	return etm_find_first_node(tree_id,parent_id,NULL, 0,data,  data_len);
}

// ������һ��ƥ��Ľڵ�id���Ҳ�������ETM_INVALID_NODE_ID
ETDLL_API u32	etm_find_next_node_by_data(u32 tree_id,u32 parent_id,u32 node_id,void * data, u32 data_len)
{
	return etm_find_next_node(tree_id,parent_id,node_id,NULL, 0,data,  data_len);
}
// ������һ��ƥ��Ľڵ�id���Ҳ�������ETM_INVALID_NODE_ID
ETDLL_API u32	etm_find_prev_node_by_data(u32 tree_id,u32 parent_id,u32 node_id,void * data, u32 data_len)
{
	return etm_find_prev_node(tree_id,parent_id,node_id,NULL, 0,data,  data_len);
}

// 19.���ݽڵ����ֺ����ݲ��Һ��ʵĽڵ�,data_len��λByte.
// ���ص�һ��ƥ��Ľڵ�id���Ҳ�������ETM_INVALID_NODE_ID
ETDLL_API u32	etm_find_first_node(u32 tree_id,u32 parent_id,const char * name, u32 name_len,void * data, u32 data_len)
{
	POST_PARA_7 param;
	_u32 node_id = TRM_INVALID_NODE_ID; 
	_int32 ret_val = SUCCESS;
	
	if(!g_already_init)
		return node_id;
	
	EM_LOG_DEBUG("etm_find_first_node");

	if(em_get_critical_error()!=SUCCESS) return node_id;

	if((tree_id<MAX_NODE_ID)|| (parent_id==TRM_INVALID_NODE_ID) ) return ETM_INVALID_NODE_ID;
	if(((name==NULL)||(name_len==0) )&&((data==NULL)||(data_len==0) )) return ETM_INVALID_NODE_ID;

	em_memset(&param,0,sizeof(POST_PARA_7));
	param._para1= (void*)tree_id;
	param._para2= (void*)parent_id;
	param._para3= (void*)name;
	param._para4= (void*)name_len;
	param._para5= (void*)data;
	param._para6= (void*)data_len;
	param._para7= (void*)&node_id;
	
	ret_val =  em_post_function(trm_find_first_node, (void *)&param,&(param._handle),&(param._result));
	if(ret_val!=SUCCESS)
	{
		return TRM_INVALID_NODE_ID;
	}
	return node_id;
}

// ������һ��ƥ��Ľڵ�id���Ҳ�������ETM_INVALID_NODE_ID
ETDLL_API u32	etm_find_next_node(u32 tree_id,u32 parent_id,u32 node_id,const char * name, u32 name_len,void * data, u32 data_len)
{
	POST_PARA_8 param;
	_u32 nxt_node_id = TRM_INVALID_NODE_ID; 
	_int32 ret_val = SUCCESS;
	
	if(!g_already_init)
		return TRM_INVALID_NODE_ID;
	
	EM_LOG_DEBUG("etm_find_next_node");

	if(em_get_critical_error()!=SUCCESS) return TRM_INVALID_NODE_ID;

	if((tree_id<MAX_NODE_ID)|| (parent_id==TRM_INVALID_NODE_ID) || (node_id==TRM_INVALID_NODE_ID) ||(parent_id==node_id)|| (tree_id==node_id)) return ETM_INVALID_NODE_ID;
	if(((name==NULL)||(name_len==0) )&&((data==NULL)||(data_len==0) )) return ETM_INVALID_NODE_ID;

	em_memset(&param,0,sizeof(POST_PARA_8));
	param._para1= (void*)tree_id;
	param._para2= (void*)parent_id;
	param._para3= (void*)name;
	param._para4= (void*)name_len;
	param._para5= (void*)data;
	param._para6= (void*)data_len;
	param._para7= (void*)node_id;
	param._para8= (void*)&nxt_node_id;
	
	ret_val =  em_post_function(trm_find_next_node, (void *)&param,&(param._handle),&(param._result));
	if(ret_val!=SUCCESS)
	{
		return TRM_INVALID_NODE_ID;
	}
	return nxt_node_id;
}
// ������һ��ƥ��Ľڵ�id���Ҳ�������ETM_INVALID_NODE_ID
ETDLL_API u32	etm_find_prev_node(u32 tree_id,u32 parent_id,u32 node_id,const char * name, u32 name_len,void * data, u32 data_len)
{
	POST_PARA_8 param;
	_u32 prev_node_id = TRM_INVALID_NODE_ID; 
	_int32 ret_val = SUCCESS;
	
	if(!g_already_init)
		return TRM_INVALID_NODE_ID;
	
	EM_LOG_DEBUG("etm_find_prev_node");

	if(em_get_critical_error()!=SUCCESS) return TRM_INVALID_NODE_ID;

	if((tree_id<MAX_NODE_ID)|| (parent_id==TRM_INVALID_NODE_ID) || (node_id==TRM_INVALID_NODE_ID) ||(parent_id==node_id)|| (tree_id==node_id)) return ETM_INVALID_NODE_ID;
	if(((name==NULL)||(name_len==0) )&&((data==NULL)||(data_len==0) )) return ETM_INVALID_NODE_ID;

	em_memset(&param,0,sizeof(POST_PARA_8));
	param._para1= (void*)tree_id;
	param._para2= (void*)parent_id;
	param._para3= (void*)name;
	param._para4= (void*)name_len;
	param._para5= (void*)data;
	param._para6= (void*)data_len;
	param._para7= (void*)node_id;
	param._para8= (void*)&prev_node_id;
	
	ret_val =  em_post_function(trm_find_prev_node, (void *)&param,&(param._handle),&(param._result));
	if(ret_val!=SUCCESS)
	{
		return TRM_INVALID_NODE_ID;
	}
	return prev_node_id;
}
///////////////////////////////////////////////////////////////
/////5 BT�������
/////1 �������ļ�������������Ϣ
ETDLL_API int32 etm_get_torrent_seed_info (const  char *seed_path, ETM_ENCODING_MODE encoding_mode,ETM_TORRENT_SEED_INFO **pp_seed_info )
{
	POST_PARA_3 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_get_torrent_seed_info:%s",seed_path);

	EM_CHECK_CRITICAL_ERROR;

	if((seed_path==NULL)||(em_strlen(seed_path)==0)||(em_strlen(seed_path)>=MAX_FULL_PATH_LEN)||(pp_seed_info==NULL)||(!em_file_exist(seed_path))) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_3));
	param._para1= (void*)seed_path;
	param._para2= (void*)encoding_mode;
	param._para3= (void*)pp_seed_info;
	
	return em_post_function(em_get_torrent_seed_info, (void *)&param,&(param._handle),&(param._result));
}
/////2 �ͷ�������Ϣ
ETDLL_API int32 etm_release_torrent_seed_info( ETM_TORRENT_SEED_INFO *p_seed_info )
{
	POST_PARA_1 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_release_torrent_seed_info");

	EM_CHECK_CRITICAL_ERROR;

	if(p_seed_info==NULL) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)p_seed_info;
	
	return em_post_function(em_release_torrent_seed_info, (void *)&param,&(param._handle),&(param._result));
}
/////4.3 ��E-Mule�����н������ļ���Ϣ

ETDLL_API int32 etm_extract_ed2k_url(char* ed2k, ETM_ED2K_LINK_INFO* info)
{
	POST_PARA_2 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_extract_ed2k_url");

	EM_CHECK_CRITICAL_ERROR;

	if((ed2k==NULL)||(info==NULL)) return INVALID_ARGUMENT;
	sd_assert(sd_strlen(ed2k)>9);

	em_memset(info,0,sizeof(ETM_ED2K_LINK_INFO));
	em_memset(&param,0,sizeof(POST_PARA_2));
	param._para1= (void*)ed2k;
	param._para2= (void*)info;
	
	return em_post_function(em_extract_ed2k_url, (void *)&param,&(param._handle),&(param._result));
}

/////4.4 �Ӵ��������н������ļ���Ϣ,Ŀǰֻ�ܽ���xtΪbt info hash �Ĵ�����
extern _int32 em_parse_magnet_url(const char * url ,_u8 info_hash[INFO_HASH_LEN], char * name_buffer,_u64 * file_size);
ETDLL_API int32 etm_parse_magnet_url(char* magnet, ETM_MAGNET_INFO* info)
{
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_parse_magnet_url");
	sd_assert(sd_strlen(magnet)>9);

	EM_CHECK_CRITICAL_ERROR;

	if((info==NULL)||(sd_strlen(magnet)==0)) return INVALID_ARGUMENT;

	return em_parse_magnet_url(magnet,info->_file_hash,info->_file_name,&info->_file_size);
}

/* ��bt�����info_hash ���ɴ�����*/
extern _int32 em_generate_magnet_url(_u8 info_hash[INFO_HASH_LEN], const char * display_name,_u64 total_size,char * url_buffer ,_u32 buffer_len);
ETDLL_API int32 etm_generate_magnet_url(const char * info_hash, const char * display_name,uint64 total_size,char * url_buffer ,u32 buffer_len)
{
	_int32 ret_val = SUCCESS;
	_u8 info_hash_u8[INFO_HASH_LEN]={0};
	
	ret_val = sd_string_to_cid((char * )info_hash,info_hash_u8);
	CHECK_VALUE(ret_val);
	ret_val = em_generate_magnet_url( info_hash_u8, display_name, total_size, url_buffer , buffer_len);
	CHECK_VALUE(ret_val);
	return SUCCESS;
}

ETDLL_API int32 etm_aes128_encrypt(const char* aes_key,const u8 * src, u32 src_len, u8 * des, u32 * des_len)
{
	_int32 ret_val = SUCCESS;
	
	ret_val = sd_aes_encrypt(aes_key, src, src_len, des, des_len);
	CHECK_VALUE(ret_val);
	return ret_val;
}

ETDLL_API int32 etm_aes128_decrypt(const char* aes_key,const u8 * src, u32 src_len, u8 * des, u32 * des_len)
{
	_int32 ret_val = SUCCESS;
	
	ret_val = sd_aes_decrypt(aes_key, src, src_len, des, des_len);
	CHECK_VALUE(ret_val);
	return ret_val;
}

ETDLL_API int32 etm_gzip_encode(u8 * src, u32 src_len, u8 * des, u32 des_len, u32 *encode_len)
{
	_int32 ret_val = SUCCESS;
	
	ret_val = sd_gz_encode_buffer(src, src_len, des, des_len, encode_len);
	CHECK_VALUE(ret_val);
	return ret_val;
}

ETDLL_API int32 etm_gzip_decode(u8 * src, u32 src_len, u8 **des, u32 *des_len, u32 *decode_len)
{
	_int32 ret_val = SUCCESS;
	
	ret_val = sd_gz_decode_buffer(src, src_len, des, des_len, decode_len);
	CHECK_VALUE(ret_val);
	return ret_val;
}
///////////////////////////////////////////////////////////////
/////6 VOD�㲥���
/////1 �����㲥����,task_id��ETM�ۼӷ���
ETDLL_API int32 etm_create_vod_task(ETM_CREATE_VOD_TASK * p_param,u32* p_task_id )
{
	POST_PARA_2 param;
	_int32 ret_val = SUCCESS;


#if defined(ENABLE_NULL_PATH)
       char gcid[CID_SIZE];
       char cid[CID_SIZE];
       //em_string_to_cid
#endif
	if(!g_already_init)
	{
#if defined(ENABLE_NULL_PATH)
		if(!etm_is_et_running())
		{
		     return -1;
		}
#else
		     return -1;
#endif
	}
	
	if((p_param==NULL)||(p_task_id==NULL)) return INVALID_ARGUMENT;
	
	EM_LOG_DEBUG("etm_create_vod_task:%d",p_param->_type);

	EM_CHECK_CRITICAL_ERROR;

#if defined(ENABLE_NULL_PATH)
      em_memset(gcid, 0, sizeof(gcid));
      em_memset(cid, 0, sizeof(cid));
      em_string_to_cid(p_param->_tcid, (_u8*)cid);
      em_string_to_cid(p_param->_gcid, (_u8*)gcid);
	  

      if(TRUE == p_param->_is_no_disk)
      {
                ret_val = et_create_task_by_tcid_file_size_gcid(cid, p_param->_file_size, gcid, p_param->_gcid, em_strlen(p_param->_gcid), "voddefaultpath/", em_strlen("voddefaultpath/"), p_task_id);
                if(SUCCESS == ret_val)
                {
                      ret_val = et_set_task_no_disk(*p_task_id);
                      ret_val = et_start_task(*p_task_id);
				  if (SUCCESS != ret_val)
				  {
				  	et_delete_task(*p_task_id);
					return ret_val;
                }

				  dt_inc_no_disk_vod_task_num();
		  *p_task_id = *p_task_id+VOD_NODISK_TASK_MASK;
      }
      }
      else if(!g_already_init)
     {
            return -1;
     }
     else
      {
		em_memset(&param,0,sizeof(POST_PARA_2));
		param._para1= (void*)p_param;
		param._para2= (void*)p_task_id;
		
		//return em_post_function( vod_create_task, (void *)&param,&(param._handle),&(param._result));
		ret_val =  em_post_function( dt_create_vod_task, (void *)&param,&(param._handle),&(param._result));
      }

#else

	// �����ĵ㲥������Ҫ��http��ͷ��Referer ,Ŀǰд�������������������Ҫ�ɸĵ���by hexiang 2013-12-5
	p_param->_ref_url = "http://vod.runmit.com";
	p_param->_ref_url_len = sd_strlen("http://vod.runmit.com");

	em_memset(&param,0,sizeof(POST_PARA_2));
	param._para1= (void*)p_param;
	param._para2= (void*)p_task_id;

	//return em_post_function( vod_create_task, (void *)&param,&(param._handle),&(param._result));
	if(TRUE == p_param->_is_no_disk)
	{
		ret_val =  em_post_function( vod_create_task, (void *)&param,&(param._handle),&(param._result));
	}
	else
	{
		ret_val =  em_post_function( dt_create_vod_task, (void *)&param,&(param._handle),&(param._result));
	}

#endif
	return ret_val;
}


/////2 ֹͣ�㲥,��������������etm_create_vod_task�ӿڴ����ģ���ô������񽫱�ɾ��,���������etm_create_task�ӿڴ��������޲���
ETDLL_API int32 etm_stop_vod_task (u32 task_id)
{
	POST_PARA_2 param;
	
    BOOL thread_unlock = TRUE;

	if(!g_already_init)
	{
#if defined(ENABLE_NULL_PATH)
		if(!etm_is_et_running())
		{
		     return -1;
		}
#else
		     return -1;
#endif
	}
	
	EM_LOG_DEBUG("etm_stop_vod_task:%u",task_id);

	EM_CHECK_CRITICAL_ERROR;

	if(task_id==0) return INVALID_ARGUMENT;
	
#if defined(ENABLE_NULL_PATH)
      if(VOD_NODISK_TASK_MASK == (task_id&VOD_NODISK_TASK_MASK))
     {
             et_stop_task(task_id-VOD_NODISK_TASK_MASK);
	      et_delete_task(task_id-VOD_NODISK_TASK_MASK);
		  dt_dec_no_disk_vod_task_num();
		  return SUCCESS;
     }
     else if(!g_already_init)
     {
            return -1;
     }
     else
     {
	  	em_memset(&param,0,sizeof(POST_PARA_2));
		param._para1= (void*)task_id;
		param._para2= (void*)thread_unlock;

	    
		//if(task_id>MAX_DL_TASK_ID)
		//{
		//	return em_post_function( vod_stop_task, (void *)&param,&(param._handle),&(param._result));
		//}
		//else
		//{
			return em_post_function( dt_stop_vod_task, (void *)&param,&(param._handle),&(param._result));
		//}
   }
#else
	em_memset(&param,0,sizeof(POST_PARA_2));
	param._para1= (void*)task_id;
	param._para2= (void*)thread_unlock;

    
	if(task_id>VOD_NODISK_TASK_MASK)
	{
		return em_post_function( vod_stop_task, (void *)&param,&(param._handle),&(param._result));
	}
	else
	{
		return em_post_function( dt_stop_vod_task, (void *)&param,&(param._handle),&(param._result));
	}
#endif	
}


/////3 ��ȡ�ļ����ݣ���etm_create_task�ӿڴ���������Ҳ�ɵ�������ӿ�ʵ�ֱ����ر߲���
ETDLL_API int32 etm_read_vod_file(u32 task_id, uint64 start_pos, uint64 len, char *buf, u32 block_time )
{
	POST_PARA_5 param;
	_int32 ret_val = SUCCESS;
#if defined(ENABLE_NULL_PATH)
	u32 task_inner_id = 0;
#endif

	
	if(!g_already_init)
	{
#if defined(ENABLE_NULL_PATH)
		if(!etm_is_et_running())
		{
		     return -1;
		}
#else
		     return -1;
#endif
	}
	//block_time = 100;
	//if(task_id<=MAX_DL_TASK_ID) return INVALID_TASK_ID;
	
	if((task_id==0)||(len==0)||(buf==NULL)) return INVALID_ARGUMENT;
	
	EM_LOG_DEBUG("etm_read_vod_file:%u,%llu start",task_id,start_pos);

	EM_CHECK_CRITICAL_ERROR;


#if defined(ENABLE_NULL_PATH)
      if(VOD_NODISK_TASK_MASK == (task_id&VOD_NODISK_TASK_MASK))
      {
            ret_val = et_vod_read_file( (task_id-VOD_NODISK_TASK_MASK), start_pos, len, buf, block_time);
      }
      else if(!g_already_init)
      {
            return -1;
      }
      else
      {	       
	       if(dt_get_running_et_task_id(task_id, &task_inner_id) == SUCCESS)
		{
			return iet_vod_read_file((_int32)task_inner_id, start_pos, len, buf, (_int32)block_time );
		}
		else
		{
			em_memset(&param,0,sizeof(POST_PARA_5));
			param._para1= (void*)task_id;
			param._para2= (void*)&start_pos;
			param._para3= (void*)&len;
			param._para4= (void*)buf;
			param._para5= (void*)block_time;
			
			ret_val =  em_post_function( dt_vod_read_file, (void *)&param,&(param._handle),&(param._result));
      }

	
#else
	{
		em_memset(&param,0,sizeof(POST_PARA_5));
		param._para1= (void*)task_id;
		param._para2= (void*)&start_pos;
		param._para3= (void*)&len;
		param._para4= (void*)buf;
		param._para5= (void*)block_time;
		
	if(task_id>VOD_NODISK_TASK_MASK)
	{
		ret_val =  em_post_function( vod_read_file, (void *)&param,&(param._handle),&(param._result));
	}
	else
	{
		ret_val =  em_post_function( dt_vod_read_file, (void *)&param,&(param._handle),&(param._result));
	}
#endif

	}
	EM_LOG_DEBUG("etm_read_vod_file:%u,%llu  end with %d",task_id,start_pos,ret_val);
	return ret_val;

}

ETDLL_API int32 etm_read_bt_vod_file(u32 task_id, u32 file_index, uint64 start_pos, uint64 len, char *buf, u32 block_time )
{
	_int32 ret_val = SUCCESS;
	POST_PARA_6 param;
	
	if(!g_already_init)
		return -1;
	
	//if(task_id<=MAX_DL_TASK_ID) return INVALID_TASK_ID;
	
	if((task_id==0)||(file_index==-1)||(len==0)||(buf==NULL)) return INVALID_ARGUMENT;
	
	EM_LOG_DEBUG("etm_read_bt_vod_file:%u",task_id);

	EM_CHECK_CRITICAL_ERROR;


	em_memset(&param,0,sizeof(POST_PARA_6));
	param._para1= (void*)task_id;
	param._para2= (void*)&start_pos;
	param._para3= (void*)&len;
	param._para4= (void*)buf;
	param._para5= (void*)block_time;
	param._para6= (void*)file_index;
	
	//return em_post_function( vod_read_bt_file, (void *)&param,&(param._handle),&(param._result));
	if(task_id>VOD_NODISK_TASK_MASK)
	{
		ret_val =  em_post_function( vod_read_bt_file, (void *)&param,&(param._handle),&(param._result));
	}
	else
	{
		ret_val = em_post_function( dt_vod_read_bt_file, (void *)&param,&(param._handle),&(param._result));
	}
	return ret_val;
}

/////6 ��ȡ����ٷֱ�
ETDLL_API int32 etm_vod_get_buffer_percent(u32 task_id,  u32 * percent )
{
	POST_PARA_2 param;
	
	if(!g_already_init)
	{
#if defined(ENABLE_NULL_PATH)
		if(!etm_is_et_running())
		{
		     return -1;
		}
#else
		     return -1;
#endif
	}
	
	//if(task_id<=MAX_DL_TASK_ID) return INVALID_TASK_ID;
	
	if((task_id==0)||(percent==NULL)) return INVALID_ARGUMENT;
	
	EM_LOG_DEBUG("etm_vod_get_buffer_percent:%u",task_id);

	EM_CHECK_CRITICAL_ERROR;

#if defined(ENABLE_NULL_PATH)
      if(VOD_NODISK_TASK_MASK == (task_id&VOD_NODISK_TASK_MASK))
      {
           return et_vod_get_buffer_percent(task_id-VOD_NODISK_TASK_MASK, percent);
      }
      else if(!g_already_init)
      {
            return -1;
      }
      else
      {
		em_memset(&param,0,sizeof(POST_PARA_2));
		param._para1= (void*)task_id;
		param._para2= (void*)percent;
		
		//return em_post_function( vod_get_buffer_percent, (void *)&param,&(param._handle),&(param._result));
		return em_post_function( dt_vod_get_buffer_percent, (void *)&param,&(param._handle),&(param._result));
      }
#else
/*
       if(dt_get_running_et_task_id(task_id, &task_inner_id) == SUCCESS)
	{
		return et_vod_get_buffer_percent((_int32)task_inner_id, (_int32 *)percent );
	}
	else
*/
	{
		em_memset(&param,0,sizeof(POST_PARA_2));
		param._para1= (void*)task_id;
		param._para2= (void*)percent;

	if(task_id>VOD_NODISK_TASK_MASK)
	{
		return em_post_function( vod_get_buffer_percent, (void *)&param,&(param._handle),&(param._result));
	}
	else
	{
		return em_post_function( dt_vod_get_buffer_percent, (void *)&param,&(param._handle),&(param._result));
	}
	}
#endif
}

/////5.5 ��ѯ�㲥�����ʣ�������Ƿ��Ѿ�������ɣ�һ�������ж��Ƿ�Ӧ�ÿ�ʼ������һ����Ӱ��ͬһ����Ӱ����һƬ�Ρ���etm_create_task�ӿڴ����������ڱ����ر߲���ʱҲ�ɵ�������ӿ�
ETDLL_API int32 etm_vod_is_download_finished(u32 task_id, BOOL* finished )
{
	POST_PARA_2 param;
	
	if(!g_already_init)
	{
#if defined(ENABLE_NULL_PATH)
		if(!etm_is_et_running())
		{
		     return -1;
		}
#else
		     return -1;
#endif
	}
	
	//if(task_id<=MAX_DL_TASK_ID) return INVALID_TASK_ID;
	
	if((task_id==0)||(finished==NULL)) return INVALID_ARGUMENT;
	
	EM_LOG_DEBUG("etm_vod_is_download_finished:%u",task_id);

	EM_CHECK_CRITICAL_ERROR;

#if defined(ENABLE_NULL_PATH)
      if(VOD_NODISK_TASK_MASK == (task_id&VOD_NODISK_TASK_MASK))
      {
            return et_vod_is_download_finished(task_id-VOD_NODISK_TASK_MASK, finished);
      	}
      else if(!g_already_init)
      {
            return -1;
      }
      else
      {
		em_memset(&param,0,sizeof(POST_PARA_2));
		param._para1= (void*)task_id;
		param._para2= (void*)finished;
		
		//return em_post_function( vod_is_download_finished, (void *)&param,&(param._handle),&(param._result));
		return em_post_function( dt_vod_is_download_finished, (void *)&param,&(param._handle),&(param._result));
      }
#else
	em_memset(&param,0,sizeof(POST_PARA_2));
	param._para1= (void*)task_id;
	param._para2= (void*)finished;
	
	if(task_id>VOD_NODISK_TASK_MASK)
	{
		return em_post_function( vod_is_download_finished, (void *)&param,&(param._handle),&(param._result));
	}
	else
	{
		return em_post_function( dt_vod_is_download_finished, (void *)&param,&(param._handle),&(param._result));
	}
#endif
}

/////5.6 ��ȡ�㲥�����ļ�������
ETDLL_API int32 etm_vod_get_bitrate(u32 task_id, u32 file_index, _u32* bitrate )
{
	POST_PARA_3 param;
	
	if(!g_already_init)
	{
#if defined(ENABLE_NULL_PATH)
		if(!etm_is_et_running())
		{
		     return -1;
		}
#else
		     return -1;
#endif
	}
	
	//if(task_id<=MAX_DL_TASK_ID) return INVALID_TASK_ID;
	
	if((task_id==0)||(bitrate==NULL)) return INVALID_ARGUMENT;
	
	EM_LOG_DEBUG("etm_vod_get_bitrate:%u",task_id);

	EM_CHECK_CRITICAL_ERROR;

#if defined(ENABLE_NULL_PATH)
      if(VOD_NODISK_TASK_MASK == (task_id&VOD_NODISK_TASK_MASK))
      {
            return et_vod_get_bitrate(task_id-VOD_NODISK_TASK_MASK, file_index, bitrate);
      	}
      else if(!g_already_init)
      {
            return -1;
      }
      else
      	{
		em_memset(&param,0,sizeof(POST_PARA_3));
		param._para1 = (void*)task_id;
		param._para2 = (void*)file_index;
		param._para3 = (void*)bitrate;
		
		return em_post_function( dt_vod_get_bitrate, (void *)&param,&(param._handle),&(param._result));
      	}
#else
	em_memset(&param,0,sizeof(POST_PARA_3));
	param._para1 = (void*)task_id;
	param._para2 = (void*)file_index;
	param._para3 = (void*)bitrate;
	
	if(task_id>VOD_NODISK_TASK_MASK)
	{
		return em_post_function( vod_get_bitrate, (void *)&param,&(param._handle),&(param._result));
	}
	else
	{
		return em_post_function( dt_vod_get_bitrate, (void *)&param,&(param._handle),&(param._result));
	}
#endif
}

/////5.7 ��ȡ��ǰ���ص�������λ��
ETDLL_API int32 etm_vod_get_download_position(u32 task_id, uint64* position )
{
	POST_PARA_2 param;
	
	if(!g_already_init)
	{
#if defined(ENABLE_NULL_PATH)
		if(!etm_is_et_running())
		{
		     return -1;
		}
#else
		     return -1;
#endif
	}
	
	//if(task_id<=MAX_DL_TASK_ID) return INVALID_TASK_ID;
	
	if((task_id==0)||(position==NULL)) return INVALID_ARGUMENT;
	
	EM_LOG_DEBUG("etm_vod_get_download_position:%u",task_id);

	EM_CHECK_CRITICAL_ERROR;

#if defined(ENABLE_NULL_PATH)
      if(VOD_NODISK_TASK_MASK == (task_id&VOD_NODISK_TASK_MASK))
      {
           return et_vod_get_download_position(task_id-VOD_NODISK_TASK_MASK, position);
      }
      else if(!g_already_init)
      {
            return -1;
      }
      else
      {
		em_memset(&param,0,sizeof(POST_PARA_2));
		param._para1= (void*)task_id;
		param._para2= (void*)position;
		
		//return em_post_function( vod_get_buffer_percent, (void *)&param,&(param._handle),&(param._result));
		return em_post_function( dt_vod_get_download_position, (void *)&param,&(param._handle),&(param._result));
      }
#else
/*
       if(dt_get_running_et_task_id(task_id, &task_inner_id) == SUCCESS)
	{
		return et_vod_get_download_position((_int32)task_inner_id, position );
	}
	else
*/
	{
		em_memset(&param,0,sizeof(POST_PARA_2));
		param._para1= (void*)task_id;
		param._para2= (void*)position;
		
	if(task_id>VOD_NODISK_TASK_MASK)
	{
		return em_post_function( vod_get_download_position, (void *)&param,&(param._handle),&(param._result));
	}
	else
	{
		return em_post_function( dt_vod_get_download_position, (void *)&param,&(param._handle),&(param._result));
	}
	}
#endif
}

/////5.8 ���㲥����תΪ��������,file_retain_timeΪ�����ļ��ڴ��̵ı���ʱ��(����0Ϊ���ñ���),��λ��
ETDLL_API int32 etm_vod_set_download_mode(u32 task_id, BOOL download,u32 file_retain_time )
{
	POST_PARA_3 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_vod_set_download_mode:%u,%d",task_id, download);

	EM_CHECK_CRITICAL_ERROR;

	if(task_id == 0||task_id>VOD_NODISK_TASK_MASK) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_3));
	param._para1 = (void*)task_id;
	param._para2 = (void*)download;
	param._para3 = (void*)file_retain_time;
	
	return em_post_function( dt_vod_set_download_mode, (void *)&param,&(param._handle),&(param._result));
}

/////5.9 ��ѯ�㲥�����Ƿ�תΪ��������
ETDLL_API int32 etm_vod_get_download_mode(u32 task_id, Bool * download,u32 * file_retain_time )
{
	POST_PARA_3 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_vod_get_download_mode:%u,%d",task_id, download);

	EM_CHECK_CRITICAL_ERROR;

	if(task_id == 0||task_id>VOD_NODISK_TASK_MASK) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_3));
	param._para1 = (void*)task_id;
	param._para2 = (void*)download;
	param._para3 = (void*)file_retain_time;
	
	return em_post_function( dt_vod_get_download_mode, (void *)&param,&(param._handle),&(param._result));
}

/////5.10 �㲥ͳ���ϱ�
ETDLL_API int32 etm_vod_report(u32 task_id, ETM_VOD_REPORT * p_report )
{
	POST_PARA_2 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_vod_report:%u",task_id);

	EM_CHECK_CRITICAL_ERROR;

	if(task_id == 0) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_2));
	param._para1 = (void*)task_id;
	param._para2 = (void*)p_report;
	
	if(task_id>VOD_NODISK_TASK_MASK)
	{
		return em_post_function( vod_report, (void *)&param,&(param._handle),&(param._result));
	}
	else
	{
		return em_post_function( dt_vod_report, (void *)&param,&(param._handle),&(param._result));
	}
}

/////5.11 ��ѯ�㲥�������ջ�ȡ���ݵķ���������(ֻ�������Ƶ㲥����)
ETDLL_API int32 etm_vod_get_final_server_host(u32 task_id, char  server_host[256] )
{
	POST_PARA_2 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_vod_get_final_server_host:%u",task_id);

	EM_CHECK_CRITICAL_ERROR;

	if(task_id == 0) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_2));
	param._para1 = (void*)task_id;
	param._para2 = (void*)server_host;
	
	if(task_id>VOD_NODISK_TASK_MASK)
	{
		return em_post_function( vod_final_server_host, (void *)&param,&(param._handle),&(param._result));
	}
	else
	{
		return em_post_function( dt_vod_final_server_host, (void *)&param,&(param._handle),&(param._result));
	}
}

/////5.12 ����֪ͨ���ؿ⿪ʼ�㲥(ֻ�������Ƶ㲥����)
ETDLL_API int32 etm_vod_ui_notify_start_play(u32 task_id )
{
	POST_PARA_1 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_vod_ui_notify_start_play:%u",task_id);

	EM_CHECK_CRITICAL_ERROR;

	if(task_id == 0) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1 = (void*)task_id;
	
	if(task_id>VOD_NODISK_TASK_MASK)
	{
		return em_post_function( vod_ui_notify_start_play, (void *)&param,&(param._handle),&(param._result));
	}
	else
	{
		return em_post_function( dt_vod_ui_notify_start_play, (void *)&param,&(param._handle),&(param._result));
	}
}

///////////////////////////////////////////////////////////////
/////7 ����
/////1 ��URL�л���ļ�����������ܴ�URL��ֱ�ӵõ��������ѯѸ�׷��������
extern _int32  sd_parse_kankan_vod_url(char* url, _int32 length ,_u8 *p_gcid,_u8 * p_cid, _u64 * p_file_size,char * file_name);
ETDLL_API int32 etm_get_file_name_and_size_from_url(const char* url,u32 url_len,char *name_buffer,u32 *name_buffer_len,uint64 *file_size, int32 block_time )
{
	char *file_name=NULL;
	_u32 name_len = 0;
	char file_name_tmp[MAX_FILE_NAME_BUFFER_LEN]={0};
	_u8 gcid_tmp[CID_SIZE];
	_u8 cid_tmp[CID_SIZE];

	sd_assert(sd_strlen(url)>9);
	if(name_buffer_len!=NULL)
	{
		file_name =em_get_file_name_from_url((char*)url, url_len);
		if(file_name==NULL) return INVALID_FILE_NAME;
		
		name_len = em_strlen(file_name);
		if(name_len==0) return INVALID_FILE_NAME;

		if((*name_buffer_len<=name_len)||(name_buffer==NULL))
		{
			*name_buffer_len=name_len;
			return NOT_ENOUGH_BUFFER;
		}
		
		em_memset(name_buffer,0,*name_buffer_len);
		em_memcpy(name_buffer,file_name,name_len);
		*name_buffer_len=name_len;
	}

	if(file_size!=NULL)
	{
		/* ���Դӿ���url�л���ļ���С */
		if(sd_parse_kankan_vod_url((char*)url,url_len ,gcid_tmp,cid_tmp, file_size,file_name_tmp)!=SUCCESS)
		{
			*file_size = 0;
			return EM_INVALID_FILE_SIZE;
		}

	}
	return SUCCESS;
}

/*--------------------------------------------------------------------------*/
/*            �򵥲�ѯ��Դ�ӿ�
----------------------------------------------------------------------------*/
ETDLL_API int32 etm_query_shub_by_url(ETM_QUERY_SHUB * p_param,u32 * p_action_id)
{
	POST_PARA_2 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_query_shub_by_url:%s",p_param->_url);
	if(!p_param ||!(p_param->_cb_ptr)||!p_action_id)
		return INVALID_ARGUMENT;
	

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(param));
	param._para1= (void*)p_param;
	param._para2= (void*)p_action_id;
	
	return em_post_function( em_query_shub_by_url, (void *)&param,&(param._handle),&(param._result));
}

ETDLL_API int32 etm_cancel_query_shub(u32 action_id)
{
	POST_PARA_1 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_cancel_query_shub:0x%X",action_id);

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(param));
	param._para1= (void*)action_id;
	
	return em_post_function( em_cancel_query_shub, (void *)&param,&(param._handle),&(param._result));
}

/* ��GCID ,CID,filesize,filename ƴ�ճ�һ����http://pubnet.sandai.net:8080/0/ ��ͷ�Ŀ���ר��URL */
ETDLL_API int32 etm_generate_kankan_url(const char* gcid,const char* cid,uint64 file_size,const char* file_name, char *url_buffer,u32 *url_buffer_len )
{
	_int32 ret_val = SUCCESS;
	_u8 gcid_u[CID_SIZE] = {0};
	_u8 cid_u[CID_SIZE] = {0};
	_u8 fs_buffer[32] = {0};
	ctx_md5	md5;
	_u8 url_mid[16]= {0};
	char url_mid_buffer[64]={0};
	
	/*
	pubnet.sandai.net
	�㲥url��ʽ����/�ָ��ֶ�
	�汾��0
	http://host:port/version/gcid/cid/file_size/head_size/index_size/bitrate/packet_size/packet_count/tail_pos/url_mid/filename
	*/

	if((gcid==NULL || sd_strlen(gcid)<CID_SIZE*2)||
		(cid==NULL || sd_strlen(cid)<CID_SIZE*2)||
		(file_size==0)||
		(file_name==NULL || sd_strlen(file_name)==0)||
		(url_buffer==NULL || url_buffer_len==NULL||*url_buffer_len<MAX_URL_LEN))
	{
		INVALID_ARGUMENT;
	}
	
	ret_val = sd_string_to_cid((char *)gcid,gcid_u);
	CHECK_VALUE(ret_val);

	if(!sd_is_cid_valid(gcid_u))
	{
		CHECK_VALUE(INVALID_GCID);
	}

	ret_val = sd_string_to_cid((char *)cid,cid_u);
	CHECK_VALUE(ret_val);
	if(!sd_is_cid_valid(cid_u))
	{
		CHECK_VALUE(INVALID_TCID);
	}
	
	
	sd_u64toa(file_size, (char *)fs_buffer, 32, 16);
	sd_strtolower((char *)fs_buffer);
	
	md5_initialize(&md5);
	md5_update(&md5, (const unsigned char*)gcid_u, CID_SIZE);
	md5_update(&md5, (const unsigned char*)cid_u, CID_SIZE);
	md5_update(&md5, (const unsigned char*)&(file_size), sizeof(_u64));
	md5_finish(&md5, url_mid);
	str2hex((char*)url_mid, 16, url_mid_buffer, CID_SIZE*2);
	sd_strtolower(url_mid_buffer);
	
	
	*url_buffer_len = sd_snprintf(url_buffer, MAX_URL_LEN-1,"http://pubnet.sandai.net:8080/0/%s/%s/%s/200000/0/4afb9/0/0/%s/%s/%s",gcid,cid,fs_buffer,fs_buffer,url_mid_buffer,file_name);

	EM_LOG_DEBUG("etm_generate_kankan_url[%u]:%s",*url_buffer_len,url_buffer);

	return SUCCESS;
}

/////2 ����΢����������
ETDLL_API int32 etm_get_mini_file_from_url(ETM_MINI_TASK * p_mini_param )
{
	POST_PARA_1 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_get_mini_file_from_url:%s",p_mini_param->_url);
	sd_assert(sd_strlen(p_mini_param->_url)>9);

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)p_mini_param;
	
	return em_post_function( em_get_mini_file_from_url, (void *)&param,&(param._handle),&(param._result));
}

/////3 ����΢���ϴ�����
ETDLL_API int32 etm_post_mini_file_to_url(ETM_MINI_TASK * p_mini_param )
{
	POST_PARA_1 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_post_mini_file_to_url:%s",p_mini_param->_url);
	sd_assert(sd_strlen(p_mini_param->_url)>9);

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)p_mini_param;
	
	return em_post_function( em_post_mini_file_to_url, (void *)&param,&(param._handle),&(param._result));
}
/////7.4 ��ȡhttpͷ���ֶ�ֵ
ETDLL_API char * etm_get_http_header_value(void * p_header,ETM_HTTP_HEADER_FIELD header_field )
{
	static char sbuffer[32];
	static char * ret_char = NULL;
	MINI_HTTP_HEADER * p_http_header = (MINI_HTTP_HEADER *)p_header;
	switch(header_field)
	{
		case EHHV_STATUS_CODE:
			em_memset(&sbuffer,0x00,32);
                	em_u32toa(p_http_header->_status_code, sbuffer, 31, 10);
			ret_char = sbuffer;	
		break;
		case EHHV_LAST_MODIFY_TIME:
			ret_char = p_http_header->_last_modified;
		break;
		case EHHV_COOKIE:
			ret_char = p_http_header->_cookie;
		break;
		case EHHV_CONTENT_ENCODING:
			ret_char = p_http_header->_content_encoding;
		break;
		case EHHV_CONTENT_LENGTH:
			em_memset(&sbuffer,0x00,32);
                	em_u64toa(p_http_header->_content_length, sbuffer, 31, 10);
			ret_char = sbuffer;	
		break;
		default:
			ret_char = NULL;
	}

	if(em_strlen(ret_char)!=0)
		return ret_char;
	else
		return NULL;
}
ETDLL_API int32 etm_cancel_mini_task(u32 mini_id )
{
	POST_PARA_1 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_cancel_mini_task:%u",mini_id);

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)mini_id;
	
	return em_post_function( em_cancel_mini_task, (void *)&param,&(param._handle),&(param._result));
}
ETDLL_API int32 etm_http_get(ETM_HTTP_GET * p_http_get ,u32 * http_id)
{
	POST_PARA_2 param;
	
	if(!g_already_init)
		return -1;
	
	//if(task_id<=MAX_DL_TASK_ID) return INVALID_TASK_ID;
	
	if((p_http_get==NULL)||(http_id==NULL)) return INVALID_ARGUMENT;
	sd_assert(sd_strlen(p_http_get->_url)>9);
	if((p_http_get->_url_len<9)||(p_http_get->_callback_fun==NULL)) 
	{
		CHECK_VALUE(INVALID_ARGUMENT);
	}
	
	EM_LOG_DEBUG("etm_http_get:%s",p_http_get->_url);

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(POST_PARA_2));
	param._para1= (void*)p_http_get;
	param._para2= (void*)http_id;
	
	return em_post_function( em_http_get, (void *)&param,&(param._handle),&(param._result));

}
ETDLL_API int32 etm_http_get_file(ETM_HTTP_GET_FILE * p_http_get_file ,u32 * http_id)
{
	POST_PARA_2 param;
	_int32 ret_val = SUCCESS;
	if(!g_already_init)
		return -1;
	
	//if(task_id<=MAX_DL_TASK_ID) return INVALID_TASK_ID;
	
	if((p_http_get_file==NULL)||(http_id==NULL)) return INVALID_ARGUMENT;
	sd_assert(sd_strlen(p_http_get_file->_url)>9);
	if((p_http_get_file->_url_len<9)||(p_http_get_file->_file_path_len==0)||(p_http_get_file->_file_name_len==0)||(p_http_get_file->_callback_fun==NULL)) 
	{
		CHECK_VALUE(INVALID_ARGUMENT);
	}
	
	EM_LOG_DEBUG("etm_http_get_file:%s",p_http_get_file->_url);

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(POST_PARA_2));
	param._para1= (void*)p_http_get_file;
	param._para2= (void*)http_id;
	ret_val =  em_post_function( em_http_get_file, (void *)&param,&(param._handle),&(param._result));
	return ret_val;
}
ETDLL_API int32 etm_http_post(ETM_HTTP_POST * p_http_post ,u32 * http_id)
{
	POST_PARA_2 param;
	
	if(!g_already_init)
		return -1;
	
	//if(task_id<=MAX_DL_TASK_ID) return INVALID_TASK_ID;
	
	if((p_http_post==NULL)||(http_id==NULL)) return INVALID_ARGUMENT;
	sd_assert(sd_strlen(p_http_post->_url)>9);
	if((p_http_post->_url_len<9)||(p_http_post->_callback_fun==NULL)) 
	{
		CHECK_VALUE(INVALID_ARGUMENT);
	}
	
	EM_LOG_DEBUG("etm_http_post:%s",p_http_post->_url);

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(POST_PARA_2));
	param._para1= (void*)p_http_post;
	param._para2= (void*)http_id;
	
	return em_post_function( em_http_post, (void *)&param,&(param._handle),&(param._result));

}

ETDLL_API int32 etm_http_post_file(ETM_HTTP_POST_FILE * p_http_post_file ,u32 * http_id)
{
	POST_PARA_2 param;
	_int32 ret_val = SUCCESS;
	if(!g_already_init)
		return -1;
	
	//if(task_id<=MAX_DL_TASK_ID) return INVALID_TASK_ID;
	
	if((p_http_post_file==NULL)||(http_id==NULL)) return INVALID_ARGUMENT;
	sd_assert(sd_strlen(p_http_post_file->_url)>9);
	if((p_http_post_file->_url_len<9)||(p_http_post_file->_file_path_len==0)||(p_http_post_file->_file_name_len==0)||(p_http_post_file->_callback_fun==NULL)) 
	{
		CHECK_VALUE(INVALID_ARGUMENT);
	}
	
	EM_LOG_DEBUG("etm_http_post_file:%s",p_http_post_file->_url);

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(POST_PARA_2));
	param._para1= (void*)p_http_post_file;
	param._para2= (void*)http_id;
	ret_val =  em_post_function( em_http_post_file, (void *)&param,&(param._handle),&(param._result));
	return ret_val;
}

ETDLL_API int32 etm_http_cancel(u32  http_id)
{
	POST_PARA_1 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_http_cancel:%u",http_id);

	EM_CHECK_CRITICAL_ERROR;


	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)http_id;
	
	return em_post_function( em_http_cancel, (void *)&param,&(param._handle),&(param._result));

}

const static  char* g_et_err_code_common[] =
{
"OUT_OF_MEMORY",		//0
"INVALID_MEMORY",
"INVALID_SSLAB_SIZE	",
"TOO_FEW_MPAGE",

"OUT_OF_FIXED_MEMORY",

"QUEUE_NO_ROOM", //5

"LOCK_SHAREBRD_FAILED",

"MAP_UNINIT",
"MAP_DUPLICATE_KEY",
"MAP_KEY_NOT_FOUND",

"INVALID_ITERATOR", //10

"BUFFER_OVERFLOW",

"BITMAP_BITSIZE_OVERFLOW",

"INVALID_ARGUMENT",

"NOT_FOUND_MAC_ADDR",
"INVALID_SOCKET_DESCRIPTOR",  // 15

"ERROR_LOG_CONF_FILE",

"ERROR_INVALID_INADDR",
"ERROR_INVALID_PEER_ID"  // 18
};
const static  char* g_et_err_code_asyn_frame[] =
{
"INVALID_OPERATION_TYPE", // 0
"INVALID_MSG_HANDLER",

"NOT_IMPLEMENT",

"REACTOR_LOGIC_ERROR",
"REACTOR_LOGIC_ERROR_1",  // 4
"REACTOR_LOGIC_ERROR_2",
"REACTOR_LOGIC_ERROR_3	",
"REACTOR_LOGIC_ERROR_4	",
"REACTOR_LOGIC_ERROR_5	",


"SOCKET_EPOLLERR",  // 9
"SOCKET_CLOSED",

"END_OF_FILE",

"BAD_POLL_EVENT	",
"NO_ROOM_OF_POLL",
"BAD_POLL_ARUMENT",  // 14
"NOT_FOUND_POLL_EVENT",

"INVALID_TIMER_INDEX",

"TOO_MANY_EVENT",
"INVALID_EVENT_HANDLE",

"DNS_NO_SERVER ",  // 19
"DNS_INVALID_ADDR",
"DNS_NETWORK_EXCEPTION",
"DNS_INVALID_REQUEST "  // 22
};
const static  char* g_et_err_code_os_interface[] =
{
"INTERFACE_ERRCODE_BASE", // 0

"BAD_DIR_PATH",
"FILE_CANNOT_TRUNCATE",
"INSUFFICIENT_DISK_SPACE",

"INVALID_CUSTOMED_INTERFACE_IDX",
"INVALID_CUSTOMED_INTERFACE_PTR", // 5

"ALREADY_ET_INIT"
};
const static  char* g_et_err_code_task_mgr[] =
{
"TM_ERR_UNKNOWN",
"TM_ERR_TASK_MANAGER_EXIST",
"TM_ERR_FILE_NOT_EXIST ",
"TM_ERR_CFG_FILE_NOT_EXIST",
"TM_ERR_INVALID_URL",
"TM_ERR_INVALID_FILE_PATH",
"TM_ERR_INVALID_FILE_NAME",
"TM_ERR_TASK_FULL",
"TM_ERR_RUNNING_TASK_FULL",
"TM_ERR_INVALID_TCID",
"TM_ERR_INVALID_FILE_SIZE",
"TM_ERR_INVALID_TASK_ID",
"TM_ERR_INVALID_DOWNLOAD_TASK",
"TM_ERR_TASK_IS_RUNNING",
"TM_ERR_TASK_NOT_RUNNING",
"TM_ERR_IVALID_GCID",
"TM_ERR_INVALID_PARAMETER",
"TM_ERR_NO_TASK",
"TM_ERR_RES_QUERY_REQING",
"TM_ERR_BUFFER_NOT_ENOUGH ",
"TM_ERR_TASK_TYPE  ",
"TM_ERR_TASK_IS_NOT_READY",
"TM_ERR_LICENSE_REPORT_FAILED",
"TM_ERR_OPERATION_CLASH ",
"TM_ERR_WAIT_FOR_SIGNAL "
};
const static  char* g_et_err_code_download_task[] =
{
"DT_ERR_UNKNOWN",
"DT_ERR_TASK_MANAGER_EXIST",
"DT_ERR_FILE_NOT_EXIST",
"DT_ERR_CFG_FILE_NOT_EXIST",
"DT_ERR_INVALID_URL",
"DT_ERR_INVALID_FILE_PATH",
"DT_ERR_INVALID_FILE_NAME",
"DT_ERR_TASK_FULL",
"DT_ERR_RUNNING_TASK_FULL",
"DT_ERR_INVALID_TCID",
"DT_ERR_INVALID_FILE_SIZE",
"DT_ERR_INVALID_TASK_ID",
"DT_ERR_INVALID_DOWNLOAD_TASK",
"DT_ERR_TASK_IS_RUNNING",
"DT_ERR_TASK_NOT_RUNNING",
"DT_ERR_INVALID_GCID",
"DT_ERR_INVALID_PARAMETER",
"DT_ERR_NO_TASK",
"DT_ERR_RES_QUERY_REQING",
"DT_ERR_BUFFER_NOT_ENOUGH",
"DT_ERR_ORIGIN_RESOURCE_EXIST",
"DT_ERR_INVALID_BCID",
"DT_ERR_GETTING_ORIGIN_URL",
"DT_ERR_GETTING_CID",
"DT_ERR_TASK_IS_NOT_READY",
"DT_ERR_URL_NOAUTH_DECODE",
"DT_ERR_FILE_EXIST"
};
 const char * etm_get_et_error_code_description(int32 error_code)
{
	int32 et_err_code=1024;
	if(error_code==0) return "SUCCESS";

	if(error_code<=ERROR_INVALID_PEER_ID)
	{
		if(error_code<=1028) et_err_code=error_code-OUT_OF_MEMORY;
		else	if(error_code==OUT_OF_FIXED_MEMORY) et_err_code=4;
 		else	if(error_code==QUEUE_NO_ROOM) et_err_code=5;
		else	if(error_code==LOCK_SHAREBRD_FAILED) et_err_code=6;
		else	if(error_code==MAP_UNINIT) et_err_code=7;
		else if(error_code==MAP_DUPLICATE_KEY) et_err_code=8;
		else	if(error_code==MAP_KEY_NOT_FOUND) et_err_code=9;
		else	if(error_code==INVALID_ITERATOR) et_err_code=10;
		else	if(error_code==BUFFER_OVERFLOW) et_err_code=11;
		else if(error_code==BITMAP_BITSIZE_OVERFLOW) et_err_code=12;
		else if(error_code==INVALID_ARGUMENT) et_err_code=13;
		else if(error_code==NOT_FOUND_MAC_ADDR) et_err_code=14;
		else if(error_code==INVALID_SOCKET_DESCRIPTOR) et_err_code=15;
		else if(error_code==ERROR_LOG_CONF_FILE) et_err_code=16;
		else if(error_code==ERROR_INVALID_INADDR) et_err_code=17;
		else if(error_code==ERROR_INVALID_PEER_ID) et_err_code=18;
		else return "UNKNOWN";

		return g_et_err_code_common[et_err_code];
	}
	else
	if((error_code>=INVALID_OPERATION_TYPE)&&(error_code<=DNS_INVALID_REQUEST))
	{
		if(error_code==INVALID_OPERATION_TYPE) et_err_code=0;
		else	if(error_code==INVALID_MSG_HANDLER) et_err_code=1;
 		else	if(error_code==NOT_IMPLEMENT) et_err_code=2;
		else	if(error_code==REACTOR_LOGIC_ERROR) et_err_code=3;
		else	if(error_code==REACTOR_LOGIC_ERROR_1) et_err_code=4;
		else if(error_code==REACTOR_LOGIC_ERROR_2) et_err_code=5;
		else if(error_code==REACTOR_LOGIC_ERROR_3) et_err_code=6;
		else	if(error_code==REACTOR_LOGIC_ERROR_4) et_err_code=7;
		else	if(error_code==REACTOR_LOGIC_ERROR_5) et_err_code=8;
		else	if(error_code==SOCKET_EPOLLERR) et_err_code=9;
		else if(error_code==SOCKET_CLOSED) et_err_code=10;
		else if(error_code==END_OF_FILE) et_err_code=11;
		else if(error_code==BAD_POLL_EVENT) et_err_code=12;
		else if(error_code==NO_ROOM_OF_POLL) et_err_code=13;
		else if(error_code==BAD_POLL_ARUMENT) et_err_code=14;
		else if(error_code==NOT_FOUND_POLL_EVENT) et_err_code=15;
		else if(error_code==INVALID_TIMER_INDEX) et_err_code=16;
		else if(error_code==TOO_MANY_EVENT) et_err_code=17;
		else if(error_code==INVALID_EVENT_HANDLE) et_err_code=18;
		else if(error_code==DNS_NO_SERVER) et_err_code=19;
		else if(error_code==DNS_INVALID_ADDR) et_err_code=20;
		else if(error_code==DNS_NETWORK_EXCEPTION) et_err_code=21;
		else if(error_code==DNS_INVALID_REQUEST) et_err_code=22;
		else return "UNKNOWN";

		return g_et_err_code_asyn_frame[et_err_code];
	}
	else
	if((error_code>=INTERFACE_ERRCODE_BASE)&&(error_code<=ALREADY_ET_INIT))
	{
		if(error_code==INTERFACE_ERRCODE_BASE) et_err_code=0;
		else	if(error_code==BAD_DIR_PATH) et_err_code=1;
 		else	if(error_code==FILE_CANNOT_TRUNCATE) et_err_code=2;
		else	if(error_code==INSUFFICIENT_DISK_SPACE) et_err_code=3;
		else	if(error_code==INVALID_CUSTOMED_INTERFACE_IDX) et_err_code=4;
		else if(error_code==INVALID_CUSTOMED_INTERFACE_PTR) et_err_code=5;
		else if(error_code==ALREADY_ET_INIT) et_err_code=6;
		else return "UNKNOWN";

		return g_et_err_code_os_interface[et_err_code];
	}
	else
	if((error_code>=TM_ERR_UNKNOWN)&&(error_code<=TM_ERR_WAIT_FOR_SIGNAL))
	{
		return g_et_err_code_task_mgr[error_code-TM_ERR_UNKNOWN];
	}
	else
	if((error_code>=DT_ERR_UNKNOWN)&&(error_code<=DT_ERR_FILE_EXIST))
	{
		return g_et_err_code_download_task[error_code-DT_ERR_UNKNOWN];
	}


	if(error_code==15364)
		return "EM_INVALID_FILE_INDEX";
	
	return "UNKNOWN";
}

const static  char* g_dt_err_code_dec[] =
{
        "ETM_OUT_OF_MEMORY",
        "ETM_BUSY",
        "INVALID_SYSTEM_PATH",
        "INVALID_LICENSE_LEN",
        "LICENSE_VERIFYING",
        "TOO_MANY_TASKS",
        "TOO_MANY_RUNNING_TASKS",
        "TASK_ALREADY_EXIST",
        "RUNNING_TASK_LOCK_CRASH",
        "NOT_ENOUGH_BUFFER",
        "NOT_ENOUGH_CACHE",
        "INVALID_WRITE_SIZE",
        "INVALID_READ_SIZE",
        "ET_RESTART_CRASH",
        "FILE_ALREADY_EXIST",
        "MSG_ALREADY_EXIST",
        "ERR_TASK_PLAYING",
        "WRITE_TASK_FILE_ERR",
        "UNKNOWN",
        "UNKNOWN",
        "UNKNOWN",
        "UNKNOWN",
        "UNKNOWN",
        "UNKNOWN",
        "UNKNOWN",
        "UNKNOWN",
        "UNKNOWN",
        "UNKNOWN",
        "UNKNOWN",
        "UNKNOWN",
        "UNKNOWN",
        "INVALID_TASK",
        "INVALID_TASK_ID",
        "INVALID_TASK_TYPE",
        "INVALID_TASK_STATE",
        "INVALID_TASK_INFO",
        "INVALID_FILE_PATH",
        "INVALID_URL",
        "INVALID_SEED_FILE",
        "SD_INVALID_FILE_SIZE",
        "INVALID_TCID",
        "INVALID_GCID",
        "INVALID_FILE_NAME",
        "INVALID_FILE_NUM",
        "INVALID_FILE_INDEX_ARRAY",
        "INVALID_USER_DATA",
        "INVALID_EIGENVALUE",
        "INVALID_ORDER_LIST_SIZE"
};
const static  char* g_tree_err_code_dec[] =
{
        "INVALID_NODE_ID",
        "INVALID_PARENT_ID",
        "INVALID_CHILD",
        "CHILD_NOT_EMPTY",
        "NAME_TOO_LONG",
        "INVALID_TREE_ID",
        "INVALID_NODE_NAME",
        "INVALID_NODE_DATA",
        "INVALID_NAME_HASH",
        "INVALID_DATA_HASH",
        "NODE_NOT_FOUND",
        "WRITE_TREE_FILE_ERR"
};
const static  char* g_wb_err_code_dec[] =
{
	"WBE_WRONG_STATE",
	"WBE_ACTION_NOT_FOUND",
	"WBE_PATH_TOO_LONG",
	"WBE_PATH_IS_DIR",
	"WBE_PATH_NOT_DIR",
	"WBE_WRONG_RESPONE",
	"WBE_STATUS_NOT_FOUND",
	"WBE_KEYWORD_NOT_FOUND",
	"WBE_PATH_ALREADY_EXIST",
	"WBE_WRONG_RESPONE_PATH",
	"WBE_WRONG_FILE_NUM",
	"WBE_NO_THUMBNAIL", 
	"WBE_GZED_FAILED",
	"WBE_DEGZED_FAILED",
	"WBE_UNCOMPRESS_FAILED",
	"WBE_XML_ENTITY_REPLACE_FAILED",
	"WBE_SAME_ACTION_EXIST",	
	"WBE_WRONG_REQUEST",
	"WBE_NOT_SUPPORT_LIST_OFFSET",
	"WBE_OTHER_ACTION_EXIST",
	"WBE_ENCRYPT_FAILED",
	"WBE_DECRYPT_FAILED"
};
const static  char* g_lx_err_code_dec[] =
{
	"LXE_ACTION_NOT_FOUND",
	"LXE_WRONG_REQUEST",
	"LXE_WRONG_RESPONE",
	"LXE_STATUS_NOT_FOUND",
	"LXE_KEYWORD_NOT_FOUND",
	"LXE_WRONG_GCID_LIST_NUM",
	"LXE_WRONG_GCID",
	"LXE_DOWNLOAD_PIC"
};
const static  char* g_nb_err_code_dec[] =
{
	"NBE_ACTION_NOT_FOUND",
	"NBE_WRONG_REQUEST",
	"NBE_WRONG_RESPONE",
	"NBE_STATUS_NOT_FOUND",
	"NBE_KEYWORD_NOT_FOUND",
	"NBE_NEIGHBOR_NOT_FOUND",
	"NBE_RESOURCE_NOT_FOUND",
	"NBE_MSG_NOT_FOUND",
	"NBE_NOT_ENOUGH_BUFFER",
	"NBE_PICTURE_NOT_FOUND",
	"NBE_LOCATION_NOT_FOUND",
	"NBE_ZONE_NOT_FOUND",
	"NBE_SAME_ZONE",
	"NBE_QUERYING_NEIGHBOR",
	"NBE_CHANGING_ZONE",
	"NBE_INVALID_ZONE_NAME",
	"NBE_RESOURCE_NOT_CHANGED",
	"NBE_SEARCH_RS_CANCEL",
	"NBE_RESOURCE_INVALID",
	"NBE_RECVER_INVALID"	
};

/////4 ��ѯ������ļ�Ҫ����
ETDLL_API const char * etm_get_error_code_description(int32 error_code)
{
	if(error_code==0)
	{
		return "SUCCESS";
	}
	else
	if(error_code<=1024)
	{
#if defined(LINUX)
		return strerror(error_code);
#else
		return "UNKNOWN";
#endif
	}
	else
	if((error_code<ETM_ERRCODE_BASE))
	{
		return etm_get_et_error_code_description( error_code);
	}
	else	
	if(error_code==ET_WRONG_VERSION)
	{
		return "Wrong version of libembed_thunder.so,1.3.3 or later is required!";
	}
	else	
	if((error_code>=ETM_OUT_OF_MEMORY)&&(error_code<=INVALID_ORDER_LIST_SIZE))
	{
		return g_dt_err_code_dec[error_code-ETM_OUT_OF_MEMORY];
	}
	else
	if((error_code>=INVALID_NODE_ID)&&(error_code<=WRITE_TREE_FILE_ERR))
	{
		return g_tree_err_code_dec[error_code-INVALID_NODE_ID];
	}
	else
	if((error_code>=WBE_WRONG_STATE)&&(error_code<=WBE_DECRYPT_FAILED))
	{
		return g_wb_err_code_dec[error_code-WBE_WRONG_STATE];
	}	
	else
	if((error_code>=LXE_ACTION_NOT_FOUND)&&(error_code<=LXE_DOWNLOAD_PIC))
	{
		return g_lx_err_code_dec[error_code-LXE_ACTION_NOT_FOUND];
	}
	else
	if((error_code>=NBE_ACTION_NOT_FOUND)&&(error_code<=NBE_RECVER_INVALID))
	{
		return g_nb_err_code_dec[error_code-NBE_ACTION_NOT_FOUND];
	}	
	else
	if(error_code==15364)
		return "EM_INVALID_FILE_INDEX";
	
	return "UNKNOWN";
}

ETDLL_API int32 etm_set_certificate_path(const char *certificate_path)
{
#ifdef ENABLE_DRM
	POST_PARA_1 param;
	char path_buffer[MAX_FILE_PATH_LEN];
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_set_certificate_path:certificate_path=%s",certificate_path);

	EM_CHECK_CRITICAL_ERROR;

	if((certificate_path==NULL)||(em_strlen(certificate_path)>= MAX_FILE_PATH_LEN)) return INVALID_ARGUMENT;

	em_memset(path_buffer,0,MAX_FILE_PATH_LEN);
	em_memcpy(path_buffer, certificate_path, MAX_FILE_PATH_LEN);
	
	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)path_buffer;
	
	return em_post_function(em_set_certificate_path, (void *)&param,&(param._handle),&(param._result));
#else
       return NOT_IMPLEMENT;
#endif
}


ETDLL_API int32 etm_open_drm_file(const char *p_file_full_path, u32 *p_drm_id, uint64 *p_origin_file_size)
{
#ifdef ENABLE_DRM
	POST_PARA_3 param;
	char path_buffer[MAX_FULL_PATH_LEN];
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_open_drm_file:file_full_path=%s",p_file_full_path);

	EM_CHECK_CRITICAL_ERROR;

	if((p_file_full_path==NULL)||(em_strlen(p_file_full_path)>= MAX_FULL_PATH_LEN)) return INVALID_ARGUMENT;

	em_memset(path_buffer,0,MAX_FULL_PATH_LEN);
	em_memcpy(path_buffer, p_file_full_path, em_strlen(p_file_full_path));
	
	em_memset(&param,0,sizeof(POST_PARA_3));
	param._para1= (void*)path_buffer;
	param._para2= (void*)p_drm_id;
	param._para3= (void*)p_origin_file_size;
	
	return em_post_function(em_open_drm_file, (void *)&param,&(param._handle),&(param._result));
#else
       return NOT_IMPLEMENT;
#endif
}

ETDLL_API int32 etm_is_certificate_ok(u32 drm_id, Bool *p_is_ok)
{
#ifdef ENABLE_DRM
	POST_PARA_2 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_is_certificate_ok:drm_id=%d",drm_id);

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(POST_PARA_2));
	param._para1= (void*)drm_id;
	param._para2= (void*)p_is_ok;
    
	return em_post_function(em_is_certificate_ok, (void *)&param,&(param._handle),&(param._result));
#else
       return NOT_IMPLEMENT;
#endif
}

/*
ETDLL_API int32 etm_read_drm_file(u32 drm_id, char *p_buffer, u32 size, 
    uint64 file_pos, u32 *p_read_size )
{
	POST_PARA_5 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_read_drm_file:drm_id=%d",drm_id);

	EM_CHECK_CRITICAL_ERROR;

	if(p_buffer==NULL||size==0) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_5));
	param._para1= (void*)drm_id;
	param._para2= (void*)p_buffer;
	param._para3= (void*)size;
	param._para4= (void*)&file_pos;
	param._para5= (void*)p_read_size;
    
	return em_post_function(em_read_drm_file, (void *)&param,&(param._handle),&(param._result));
}
*/
#ifdef ENABLE_DRM
extern int32 drm_read_file( u32 drm_id, char *buf, u32 size, uint64 start_pos, u32 *p_read_size );
#endif

ETDLL_API int32 etm_read_drm_file(u32 drm_id, char *p_buffer, u32 size, 
    uint64 file_pos, u32 *p_read_size )
{
#ifdef ENABLE_DRM
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_read_drm_file:drm_id=%d",drm_id);

	EM_CHECK_CRITICAL_ERROR;

	if(p_buffer==NULL||size==0) return INVALID_ARGUMENT;

	return drm_read_file(drm_id,  p_buffer, size, file_pos, p_read_size);
#else
       return NOT_IMPLEMENT;
#endif
}


ETDLL_API int32 etm_close_drm_file(u32 drm_id)
{
#ifdef ENABLE_DRM
	POST_PARA_1 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_set_certificate_path:drm_id=%d",drm_id);

	EM_CHECK_CRITICAL_ERROR;

	
	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)drm_id;
	
	return em_post_function(em_close_drm_file, (void *)&param,&(param._handle),&(param._result));
#else
	return NOT_IMPLEMENT;
#endif
}

ETDLL_API int32 etm_set_openssl_rsa_interface(int32 fun_count, void *fun_ptr_table)
{
#ifdef ENABLE_DRM
	if(g_already_init)
		return ALREADY_ET_INIT;

	return em_set_openssl_rsa_interface(fun_count, fun_ptr_table);
#else
	return NOT_IMPLEMENT;
#endif
}

ETDLL_API int32 etm_mc_json_call( const char *p_input_json, char *p_output_json_buffer, u32 *p_buffer_len )
{
#ifdef ENABLE_ETM_MEDIA_CENTER
	POST_PARA_3 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_mc_json_call:input_json=%s",p_input_json );

	EM_CHECK_CRITICAL_ERROR;

	
	em_memset(&param,0,sizeof(POST_PARA_3));
	param._para1= (void*)p_input_json;
	param._para2= (void*)p_output_json_buffer;
	param._para3= (void*)p_buffer_len;
	
	return em_post_function( mc_json_call, (void *)&param,&(param._handle),&(param._result));
#else
	return NOT_IMPLEMENT;
#endif

}

//////////////////////////////////////////////////////////////
/////12 ָ����������������������ؽӿ�
//��ʼ����
ETDLL_API int32 etm_start_search_server(ETM_SEARCH_SERVER * p_search)
{
	POST_PARA_1 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_start_search_server" );

	EM_CHECK_CRITICAL_ERROR;

	
	em_memset(&param,0,sizeof(param));
	param._para1= (void*)p_search;

	return em_post_function( em_start_search_server, (void *)&param,&(param._handle),&(param._result));
}

//ֹͣ����,����ýӿ���ETM�ص�_search_finished_callback_fun֮ǰ���ã���ETM�����ٻص�_search_finished_callback_fun
ETDLL_API int32 etm_stop_search_server(void)
{
	POST_PARA_0 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_stop_search_server" );

	EM_CHECK_CRITICAL_ERROR;

	
	em_memset(&param,0,sizeof(param));

	return em_post_function( em_stop_search_server, (void *)&param,&(param._handle),&(param._result));
}

ETDLL_API int32 etm_restart_search_server(void)
{
	POST_PARA_0 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_restart_search_server" );

	EM_CHECK_CRITICAL_ERROR;

	
	em_memset(&param,0,sizeof(param));

	return em_post_function( em_restart_search_server, (void *)&param,&(param._handle),&(param._result));
}

//////////////////////////////////////////////////////////////
/////14 ���߿ռ���ؽӿ�
//	ע��:����char *�ַ�����ΪUTF8��ʽ!
#ifdef ENABLE_LIXIAN_ETM
#include "lixian/lixian_interface.h"
#endif /* ENABLE_LIXIAN_ETM */

/* �����ؿ�����Ѹ���û��ʺ���Ϣ,new_user_name���������ִ� */
ETDLL_API int32 etm_lixian_set_user_info(uint64 user_id,const char * new_user_name,const char * old_user_name,int32 vip_level,const char * session_id,char* jumpkey, u32 jumpkey_len)
 {
 	_int32 ret_val = SUCCESS;
#ifdef ENABLE_LIXIAN_ETM
	POST_PARA_7 param;
	
	if(!g_already_init)
		return -1;
	
    	EM_LOG_DEBUG( "etm_lixian_set_user_info:user_id = %llu,user_name=%s,vip_level=%d" ,user_id,new_user_name,vip_level);
		
	if(user_id==0) return INVALID_ARGUMENT;

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(param));
	param._para1 = (void*)&user_id;
	param._para2 = (void*)new_user_name;
	param._para3 = (void*)old_user_name;
	param._para4 = (void*)vip_level;
	param._para5 = (void*)session_id;
	param._para6 = (void*)jumpkey;
	param._para7 = (void*)jumpkey_len;

	ret_val =  em_post_function( lixian_set_user_info, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM  */
	return ret_val;
 }

/* �û�����etm_member_relogin���µ�¼֮���轫���µ�sessionid�������õ�����ģ��  */
ETDLL_API int32 etm_lixian_set_sessionid(const char * session_id)
 {
 	_int32 ret_val = SUCCESS;
#ifdef ENABLE_LIXIAN_ETM
	POST_PARA_1 param;
	
	if(!g_already_init)
		return -1;
	
    EM_LOG_DEBUG( "etm_lixian_set_sessionid: sessionid=%s" ,session_id);
		
	if(NULL == session_id) return INVALID_ARGUMENT;

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(param));
	param._para1 = (void*)session_id;

	ret_val =  em_post_function( lixian_set_sessionid, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM  */
	return ret_val;
 }


/* ����:
	1. �����ؿ�����Ѹ���û��ʺ���Ϣ,new_user_name���������ִ���
	2. �����ؿ�����jump key��
	3. ��¼���߿ռ䡣
   ����: ELX_LOGIN_INFO
	�û���¼֮���ȡ������Ϣ���ݹ����������ߵ�¼��
*/
ETDLL_API int32 etm_lixian_login(ELX_LOGIN_INFO* p_param, u32 * p_action_id)
 {
 	_int32 ret_val = SUCCESS;
#ifdef ENABLE_LIXIAN_ETM
	POST_PARA_2 param;
	
	if(!g_already_init)
		return -1;
	
    EM_LOG_DEBUG( "etm_lixian_login: callback_fun = 0x%x", p_param);
		
	if(p_param == NULL || p_action_id == NULL) return INVALID_ARGUMENT;

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(param));
	param._para1 = (void*)p_param;
	param._para2 = (void*)p_action_id;

	ret_val =  em_post_function( lixian_login, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM  */
	return ret_val;
 }

ETDLL_API int32 etm_lixian_has_list_cache(uint64 userid, BOOL *is_cache)
 {
 	_int32 ret_val = SUCCESS;
#ifdef ENABLE_LIXIAN_ETM
	POST_PARA_2 param;
	
	if(!g_already_init)
		return -1;
	 	EM_LOG_DEBUG( "etm_lixian_has_list_cache: userid = %llu", userid);		
	if(userid == 0 || is_cache == NULL) return INVALID_ARGUMENT;

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(param));
	param._para1 = (void*)&userid;
	param._para2 = (void*)is_cache;

	ret_val =  em_post_function( lixian_has_list_cache, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM  */
	return ret_val;
 }

/* ��ȡ���߿ռ������б�
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
ETDLL_API int32 etm_lixian_get_task_list(ELX_GET_TASK_LIST * p_param,u32 * p_action_id)
 {
 	_int32 ret_val = SUCCESS;
#ifdef ENABLE_LIXIAN_ETM
	POST_PARA_2 param;
	
	if(!g_already_init)
		return -1;
	
    	EM_LOG_DEBUG( "etm_lixian_get_task_list");
		
	if(p_param==NULL||p_action_id==NULL) return INVALID_ARGUMENT;

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(param));
	param._para1 = (void*)p_param;
	param._para2 = (void*)p_action_id;

	ret_val =  em_post_function( lixian_get_task_list, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM  */
	return ret_val;
 }

/* ��ȡ���߿ռ���ڻ�����ɾ��������б�
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
ETDLL_API int32 etm_lixian_get_overdue_or_deleted_task_list(ELX_GET_OD_OR_DEL_TASK_LIST * p_param,u32 * p_action_id)
 {
 	_int32 ret_val = SUCCESS;
#ifdef ENABLE_LIXIAN_ETM
	POST_PARA_2 param;
	
	if(!g_already_init)
		return -1;
	
    EM_LOG_DEBUG( "etm_lixian_get_overdue_or_deleted_task_list");
		
	if(p_param==NULL||p_action_id==NULL) return INVALID_ARGUMENT;

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(param));
	param._para1 = (void*)p_param;
	param._para2 = (void*)p_action_id;

	ret_val =  em_post_function( lixian_get_overdue_or_deleted_task_list, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM  */
	return ret_val;
 }

/* ����: ��������Ĳ�ͬ״̬��ȡ��������id �б�
   ����:
     state: ��Χ 0-127, �������¼���״ֵ̬������ȡ��Ҫ��ȡ�ļ���״ֵ̬
     state = 0��ʾ��ȡ����״̬id�б�
	    	ELXS_WAITTING =1, 
			ELXS_RUNNING  =2, 
			ELXS_PAUSED   =4,
			ELXS_SUCCESS  =8, 
			ELXS_FAILED   =16, 
			ELXS_OVERDUE  =32,
			ELXS_DELETED  =64
     id_array_buffer: ��֪��ĳ״̬��Ӧ��id��Ŀʱ����һ�ε��ô���NULL�����ݷ��ص�buffer_len������ռ�ڶ��δ����ȡid�б�
     buffer_len: ��Ҫ��ȡ��id�б���Ŀ����һ��id_array_bufferΪNULLʱ����ֵ��0���ɡ�
*/
ETDLL_API int32 etm_lixian_get_task_ids_by_state(int32 state,uint64 * id_array_buffer,u32 *buffer_len)
 {
 	_int32 ret_val = SUCCESS;
#ifdef ENABLE_LIXIAN_ETM
	POST_PARA_3 param;
	
	if(!g_already_init)
		return -1;
	
    	EM_LOG_DEBUG( "etm_lixian_get_task_ids_by_state");
		
	if(buffer_len==NULL || state < 0 || state > ELXS_DELETED * 2) return INVALID_ARGUMENT;

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(param));
	param._para1 = (void*)state;
	param._para2 = (void*)id_array_buffer;
	param._para3 = (void*)buffer_len;

	ret_val =  em_post_function( lixian_get_task_ids_by_state, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM  */
	return ret_val;
 }

/* ��ȡ����������Ϣ */
ETDLL_API int32 etm_lixian_get_task_info(uint64 task_id,ELX_TASK_INFO * p_info)
 {
 	_int32 ret_val = SUCCESS;
#ifdef ENABLE_LIXIAN_ETM
	POST_PARA_2 param;
	
	if(!g_already_init)
		return -1;
	
    	EM_LOG_DEBUG( "etm_lixian_get_task_info");
		
	if(task_id==0||p_info==NULL) return INVALID_ARGUMENT;

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(param));
	param._para1 = (void*)&task_id;
	param._para2 = (void*)p_info;

	ret_val =  em_post_function( lixian_get_task_info, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM  */
	return ret_val;
 }

/* ��ȡ���߿ռ�BT ��������ļ��б�
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
ETDLL_API int32 etm_lixian_get_bt_task_file_list(ELX_GET_BT_FILE_LIST * p_param,u32 * p_action_id)
 {
 	_int32 ret_val = SUCCESS;
#ifdef ENABLE_LIXIAN_ETM
	POST_PARA_2 param;
	
	if(!g_already_init)
		return -1;
	
    	EM_LOG_DEBUG( "etm_lixian_get_bt_task_file_list");
		
	if(p_param==NULL||p_action_id==NULL) return INVALID_ARGUMENT;

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(param));
	param._para1 = (void*)p_param;
	param._para2 = (void*)p_action_id;

	ret_val =  em_post_function( lixian_get_bt_task_file_list, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM  */
	return ret_val;
 }
/* ����: ��������Ĳ�ͬ״̬��ȡ��������id �б�
   ����:
     task_id: BT�������id
     state: ��Χ 0-127, �������¼���״ֵ̬������ȡ��Ҫ��ȡ�ļ���״ֵ̬
     		state = 0��ʾ��ȡ����״̬id�б�
	    	ELXS_WAITTING =1, 
			ELXS_RUNNING  =2, 
			ELXS_PAUSED   =4,
			ELXS_SUCCESS  =8, 
			ELXS_FAILED   =16, 
			ELXS_OVERDUE  =32,
			ELXS_DELETED  =64
     id_array_buffer: ��֪��ĳ״̬��Ӧ��id��Ŀʱ����һ�ε��ô���NULL�����ݷ��ص�buffer_len������ռ�ڶ��δ����ȡid�б�
     buffer_len: ��Ҫ��ȡ��id�б���Ŀ����һ��id_array_bufferΪNULLʱ����ֵ��0���ɡ�
*/
ETDLL_API int32 etm_lixian_get_bt_sub_file_ids_by_state(uint64 task_id,int32 state,uint64 * id_array_buffer,u32 *buffer_len)
 {
 	_int32 ret_val = SUCCESS;
#ifdef ENABLE_LIXIAN_ETM
	POST_PARA_4 param;
	
	if(!g_already_init)
		return -1;
	
    	EM_LOG_DEBUG( "etm_lixian_get_bt_sub_file_ids_by_state:%llu,%d",task_id, state);
		
	if(task_id==0||buffer_len==NULL) return INVALID_ARGUMENT;

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(param));
	param._para1 = (void*)&task_id;
	param._para2 = (void*)state;
	param._para3 = (void*)id_array_buffer;
	param._para4 = (void*)buffer_len;

	ret_val =  em_post_function( lixian_get_bt_sub_file_ids, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM  */
	return ret_val;
 }

/////��ȡ����BT������ĳ�����ļ�����Ϣ
ETDLL_API int32 etm_lixian_get_bt_sub_file_info(uint64 task_id, uint64 file_id, ELX_FILE_INFO *p_file_info)
{
 	_int32 ret_val = SUCCESS;
#ifdef ENABLE_LIXIAN_ETM
	POST_PARA_3 param;
	
	if(!g_already_init)
		return -1;
	
    	EM_LOG_DEBUG( "etm_lixian_get_bt_sub_file_info");
		
	if(task_id==0||p_file_info==NULL) return INVALID_ARGUMENT;

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(param));
	param._para1 = (void*)&task_id;
	param._para2 = (void*)&file_id;
	param._para3 = (void*)p_file_info;

	ret_val =  em_post_function(lixian_get_bt_sub_file_info, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM  */
	return ret_val;
 }

/* ��ѯ������Ϣ����֧�ֵ������߶�������������Ϣ��ѯ task_id Ϊ��Ҫ��ѯ������id���飬task_numΪ��Ҫ��ѯ��������Ŀ
   callback_funΪ���첽�����Ļص�����, p_action_id ���ظ��첽������id��������;ȡ���ò���*/
ETDLL_API int32 etm_lixian_query_task_info(uint64 *task_id, u32 task_num, void* user_data, ELX_GET_TASK_LIST_CALLBACK callback_fun, u32 * p_action_id)
{
 	_int32 ret_val = SUCCESS;
#ifdef ENABLE_LIXIAN_ETM
	POST_PARA_5 param;
	
	if(!g_already_init)
		return -1;
	
    EM_LOG_DEBUG( "etm_lixian_query_task_info");
		
	if(task_id == 0 || callback_fun == NULL || p_action_id == NULL) return INVALID_ARGUMENT;

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(param));
	param._para1 = (void*)task_id;
	param._para2 = (void*)task_num;
	param._para3 = (void*)user_data;
	param._para4 = (void*)callback_fun;
	param._para5 = (void*)p_action_id;

	ret_val =  em_post_function( lixian_query_task_info, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM  */
	return ret_val;
}

ETDLL_API int32 etm_lixian_query_bt_task_info(uint64 *task_id, u32 task_num, void* user_data, ELX_QUERY_TASK_INFO_CALLBACK callback_fun, u32 * p_action_id)
{
 	_int32 ret_val = SUCCESS;
#ifdef ENABLE_LIXIAN_ETM
	POST_PARA_5 param;
	
	if(!g_already_init)
		return -1;
	
    EM_LOG_DEBUG( "etm_lixian_query_bt_task_info");
		
	if(task_id == NULL || callback_fun == NULL || p_action_id == NULL) return INVALID_ARGUMENT;

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(param));
	param._para1 = (void*)task_id;
	param._para2 = (void*)task_num;
	param._para3 = (void*)user_data;
	param._para4 = (void*)callback_fun;
	param._para5 = (void*)p_action_id;

	ret_val =  em_post_function( lixian_query_bt_task_info, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM  */
	return ret_val;
}


/* ��ȡ��Ƶ��ͼ,��ͼΪjpg��ʽ
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
ETDLL_API int32 etm_lixian_get_screenshot(ELX_GET_SCREENSHOT * p_param,u32 * p_action_id)
 {
 	_int32 ret_val = SUCCESS;
#ifdef ENABLE_LIXIAN_ETM
	POST_PARA_2 param;
	
	if(!g_already_init)
		return -1;
	
    	EM_LOG_DEBUG( "etm_lixian_get_screenshot");
		
	if(p_param==NULL||p_action_id==NULL||p_param->_file_num == 0||p_param->_gcid_array == NULL||p_param->_store_path == NULL) return INVALID_ARGUMENT;

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(param));
	param._para1 = (void*)p_param;
	param._para2 = (void*)p_action_id;

	ret_val =  em_post_function( lixian_get_screenshot, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM  */
	return ret_val;
 }

/* ��ȡ��Ƶ�ļ����Ƶ㲥URL
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
ETDLL_API int32 etm_lixian_get_vod_url(ELX_GET_VOD * p_param,u32 * p_action_id)
 {
 	_int32 ret_val = SUCCESS;
#ifdef ENABLE_LIXIAN_ETM
	POST_PARA_2 param;
	
	if(!g_already_init)
		return -1;
	
    	EM_LOG_DEBUG( "etm_lixian_get_vod_url");
		
	if(p_param==NULL||p_action_id==NULL) return INVALID_ARGUMENT;

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(param));
	param._para1 = (void*)p_param;
	param._para2 = (void*)p_action_id;

	ret_val =  em_post_function( lixian_get_vod_url, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM  */
	return ret_val;
 }

/* �����߿ռ��ύ��������,ʹ�øýӿ�ǰ�����ù�jump key*/
ETDLL_API int32 etm_lixian_create_task(ELX_CREATE_TASK_INFO * p_param,u32 * p_action_id)
{
 	_int32 ret_val = SUCCESS;
#ifdef ENABLE_LIXIAN_ETM
	POST_PARA_2 param;
	
	if(!g_already_init)
		return -1;
	
    	EM_LOG_DEBUG( "etm_lixian_create_task");
		
	if(p_param==NULL||p_action_id==NULL) return INVALID_ARGUMENT;

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(param));
	param._para1 = (void*)p_param;
	param._para2 = (void*)p_action_id;

	ret_val =  em_post_function( lixian_create_task, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM  */
	return ret_val;
}

/* ��������: �ѹ�������ɾ�����������ؽӿ�(�첽�ӿ�)
   ��������: �ɵ���ִ��ɾ���ѹ�������Ĳ�����Ҳ�ɶ��ѹ��������Զ�ɾ�����������ز�����
   ��������:
     task_id Ϊ��Ҫ�������ص�����id;
     user_data �û��������ص������д���;
     callback Ϊ�ѹ�����������´�����Ļص�����(����ɾ���������ʱ���ûص������ǰ������������ɾ�����������������迼��)
     download_flag �������ر�ʶ��FALSE��ʾֻ����ִ��ɾ�����������������TRUE��ʾ�������ز�����
     p_action_id ���ظ��첽������id��������;ȡ���ò���
*/
ETDLL_API int32 etm_lixian_create_task_again(uint64 task_id, void* user_data, ELX_CREATE_TASK_CALLBACK callback, u32 * p_action_id)
{
	_int32 ret_val = SUCCESS;
#ifdef ENABLE_LIXIAN_ETM
	POST_PARA_4 param;
	
	if(!g_already_init)
		return -1;
	
    	EM_LOG_DEBUG( "etm_lixian_create_task_again");
		
	if( 0 == task_id || callback == NULL|| p_action_id == NULL) return INVALID_ARGUMENT;

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(param));
	param._para1 = (void*)&task_id;
	param._para2 = (void*)user_data;
	param._para3 = (void*)callback;
	param._para4 = (void*)p_action_id;

	ret_val =  em_post_function( lixian_create_task_again, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM  */
	return ret_val;
}

/* �����߿ռ��ύbt ��������,ʹ�øýӿ�ǰ�����ù�jump key*/
ETDLL_API int32 etm_lixian_create_bt_task(ELX_CREATE_BT_TASK_INFO * p_param,u32 * p_action_id)
{
 	_int32 ret_val = SUCCESS;
#ifdef ENABLE_LIXIAN_ETM
	POST_PARA_2 param;
	
	if(!g_already_init)
		return -1;
	
    	EM_LOG_DEBUG( "etm_lixian_create_bt_task");
		
	if(p_param==NULL||p_action_id==NULL) return INVALID_ARGUMENT;

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(param));
	param._para1 = (void*)p_param;
	param._para2 = (void*)p_action_id;

	ret_val =  em_post_function( lixian_create_bt_task, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM  */
	return ret_val;
}


/* �����߿ռ�ɾ����������,ʹ�øýӿ�ǰ�����ù�jump key*/
ETDLL_API int32 etm_lixian_delete_task(uint64 task_id, void* user_data, ELX_DELETE_TASK_CALLBACK callback_fun, u32 * p_action_id)
{
 	_int32 ret_val = SUCCESS;
#ifdef ENABLE_LIXIAN_ETM
	POST_PARA_4 param;
	
	if(!g_already_init)
		return -1;
	
    	EM_LOG_DEBUG( "etm_lixian_delete_task:%llu",task_id);
		
	if(task_id==0||callback_fun == NULL || p_action_id==NULL) return INVALID_ARGUMENT;

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(param));
	param._para1 = (void*)&task_id;
	param._para2 = (void*)user_data;
	param._para3 = (void*)callback_fun;
	param._para4 = (void*)p_action_id;
	
	ret_val =  em_post_function( lixian_delete_task, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM  */
	return ret_val;
}

/* ɾ����������*/
ETDLL_API int32 etm_lixian_delete_overdue_task(uint64 task_id, void* user_data, ELX_DELETE_TASK_CALLBACK callback_fun, u32 * p_action_id)
{
 	_int32 ret_val = SUCCESS;
#ifdef ENABLE_LIXIAN_ETM
	POST_PARA_4 param;
	
	if(!g_already_init)
		return -1;
	
    	EM_LOG_DEBUG( "etm_lixian_delete_task:%llu",task_id);
		
	if(task_id==0||callback_fun == NULL || p_action_id==NULL) return INVALID_ARGUMENT;

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(param));
	param._para1 = (void*)&task_id;
	param._para2 = (void*)user_data;
	param._para3 = (void*)callback_fun;
	param._para4 = (void*)p_action_id;
	
	ret_val =  em_post_function( lixian_delete_overdue_task, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM  */
	return ret_val;
}

/* �����߿ռ�����ɾ����������,ʹ�øýӿ�ǰ�����ù�jump key*/
ETDLL_API int32 etm_lixian_delete_tasks(u32 task_num,uint64 * p_task_ids,BOOL is_overdue, void* user_data, ELX_DELETE_TASKS_CALLBACK callback_fun, u32 * p_action_id)
{
 	_int32 ret_val = SUCCESS;
#ifdef ENABLE_LIXIAN_ETM
	POST_PARA_6 param;
	
	if(!g_already_init)
		return -1;
	
    	EM_LOG_DEBUG( "etm_lixian_delete_tasks:%u,is_overdue=%d",task_num,is_overdue);
		
	if(task_num==0||p_task_ids==NULL||callback_fun == NULL || p_action_id==NULL) return INVALID_ARGUMENT;

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(param));
	param._para1 = (void*)task_num;
	param._para2 = (void*)p_task_ids;
	param._para3 = (void*)is_overdue;
	param._para4 = (void*)user_data;
	param._para5 = (void*)callback_fun;
	param._para6 = (void*)p_action_id;
	
	ret_val =  em_post_function( lixian_delete_tasks, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM  */
	return ret_val;
}

/* �����߿ռ�������������,ʹ�øýӿ�ǰ�����ù�jump key*/
ETDLL_API int32 etm_lixian_delay_task(uint64 task_id, void* user_data, ELX_DELAY_TASK_CALLBACK callback_fun, u32 * p_action_id)
{
 	_int32 ret_val = SUCCESS;
#ifdef ENABLE_LIXIAN_ETM
	POST_PARA_4 param;
	
	if(!g_already_init)
		return -1;
	
    	EM_LOG_DEBUG( "etm_lixian_delay_task");
		
	if(task_id==0||callback_fun == NULL || p_action_id==NULL) return INVALID_ARGUMENT;

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(param));
	param._para1 = (void*)&task_id;
	param._para2 = (void*)user_data;
	param._para3 = (void*)callback_fun;
	param._para4 = (void*)p_action_id;
	
	ret_val =  em_post_function( lixian_delay_task, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM  */
	return ret_val;
}

/* */
ETDLL_API int32 etm_lixian_miniquery_task(uint64 task_id, void* user_data, ELX_MINIQUERY_TASK_CALLBACK callback_fun, u32 * p_action_id)
{
 	_int32 ret_val = SUCCESS;
#ifdef ENABLE_LIXIAN_ETM
	POST_PARA_4 param;
	
	if(!g_already_init)
		return -1;
	
    	EM_LOG_DEBUG( "etm_lixian_miniquery_task");
		
	if(task_id==0||callback_fun == NULL || p_action_id==NULL) return INVALID_ARGUMENT;

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(param));
	param._para1 = (void*)&task_id;
	param._para2 = (void*)user_data;
	param._para3 = (void*)callback_fun;
	param._para4 = (void*)p_action_id;
	
	ret_val =  em_post_function( lixian_miniquery_task, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM  */
	return ret_val;
}


///// ����URL ,cid,gicd ��BT���ӵ�info_hash ���Ҷ�Ӧ����������id
ETDLL_API int32 	etm_lixian_get_task_id_by_eigenvalue(ETM_EIGENVALUE * p_eigenvalue,uint64 * task_id)
{
 	_int32 ret_val = SUCCESS;
#ifdef ENABLE_LIXIAN_ETM
	POST_PARA_2 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_lixian_get_task_id_by_eigenvalue:%d",p_eigenvalue->_type);

	EM_CHECK_CRITICAL_ERROR;

	if(p_eigenvalue==NULL||task_id==NULL) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_2));
	param._para1= (void*)p_eigenvalue;
	param._para2= (void*)task_id;
	
	ret_val = em_post_function( lixian_get_task_id_by_eigenvalue, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val = NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM  */
	return ret_val;	
}

 
///// ����gicd ���Ҷ�Ӧ����������id
ETDLL_API int32 etm_lixian_get_task_id_by_gcid(char * p_gcid, uint64 * task_id, uint64 * file_id)
{
 	_int32 ret_val = SUCCESS;
#ifdef ENABLE_LIXIAN_ETM
	POST_PARA_3 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_lixian_get_task_id_by_gcid:%s",p_gcid);

	EM_CHECK_CRITICAL_ERROR;

	if(p_gcid==NULL || task_id==NULL || file_id==NULL) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(POST_PARA_3));
	param._para1= (void*)p_gcid;
	param._para2= (void*)task_id;
	param._para3= (void*)file_id;
	
	ret_val = em_post_function( lixian_get_task_id_by_gcid, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val = NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM  */
	return ret_val;	
}
   
/* �����ؿ�����jump key*/
ETDLL_API int32 etm_lixian_set_jumpkey(char* jumpkey, u32 jumpkey_len)
{
 	_int32 ret_val = SUCCESS;
#ifdef ENABLE_LIXIAN_ETM
	POST_PARA_2 param;
	
	if(!g_already_init)
		return -1;
	
    	EM_LOG_DEBUG( "etm_lixian_set_jumpkey");
		
	EM_CHECK_CRITICAL_ERROR;

	if(jumpkey == NULL || jumpkey_len == 0) return INVALID_ARGUMENT;

	em_memset(&param,0,sizeof(param));
	param._para1 = (void*)jumpkey;
	param._para2 = (void*)jumpkey_len;

	ret_val =  em_post_function( lixian_set_jumpkey, (void*)&param,&(param._handle),&(param._result));
#else
	ret_val = NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM  */
	return ret_val;	
}

/* ��ȡ�û����߿ռ���Ϣ,,��λByte*/
ETDLL_API int32 etm_lixian_get_space(uint64 * p_max_space,uint64 * p_available_space)
{
 	_int32 ret_val = SUCCESS;
#ifdef ENABLE_LIXIAN_ETM
	POST_PARA_2 param;
	
	if(!g_already_init)
		return -1;
	
    	EM_LOG_DEBUG( "etm_lixian_get_space");
		
	EM_CHECK_CRITICAL_ERROR;

	if(p_max_space == NULL || p_available_space == NULL) return INVALID_ARGUMENT;
	
	*p_max_space = 0;
	*p_available_space = 0;
	
	em_memset(&param,0,sizeof(param));
	param._para1 = (void*)p_max_space;
	param._para2 = (void*)p_available_space;

	ret_val =  em_post_function( lixian_get_space, (void*)&param,&(param._handle),&(param._result));
#else
	ret_val = NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM  */
	return ret_val;	
}

/* ��ȡ�㲥ʱ��cookie */
ETDLL_API const char * etm_lixian_get_cookie(void)
 {
#ifdef ENABLE_LIXIAN_ETM
 	_int32 ret_val = SUCCESS;
	POST_PARA_1 param;
	static char cookie_buffer[MAX_URL_LEN] = {0};
	
	if(!g_already_init)
		return NULL;
	
    	EM_LOG_DEBUG( "etm_lixian_get_cookie");
		
	ETM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(param));
	param._para1 = (void*)cookie_buffer;

	ret_val =  em_post_function( lixian_get_cookie, (void *)&param,&(param._handle),&(param._result));
	if(ret_val == SUCCESS && sd_strlen(cookie_buffer)!=0)
	{	
		return cookie_buffer;
	}
#endif /* ENABLE_LIXIAN_ETM  */
	return NULL;
 }

/* ȡ��ĳ���첽���� */
ETDLL_API int32 etm_lixian_cancel(u32 action_id)
 {
 	_int32 ret_val = SUCCESS;
#ifdef ENABLE_LIXIAN_ETM
	POST_PARA_1 param;
	
	if(!g_already_init)
		return -1;
	
    	EM_LOG_DEBUG( "etm_lixian_cancel");
		
	if(action_id==0) return INVALID_ARGUMENT;

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(param));
	param._para1 = (void*)action_id;

	ret_val =  em_post_function( lixian_cancel, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM  */
	return ret_val;
 }
/* �˳����߿ռ� */
ETDLL_API int32 etm_lixian_logout(void)
 {
 	_int32 ret_val = SUCCESS;
#ifdef ENABLE_LIXIAN_ETM
	POST_PARA_0 param;
	
	if(!g_already_init)
		return -1;
	
    	EM_LOG_DEBUG( "etm_lixian_logout");
		
	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(param));

	ret_val =  em_post_function( lixian_logout, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM  */
	return ret_val;
 }

ETDLL_API int32 etm_get_free_strategy(uint64 user_id, const char * session_id, void* user_data, ETM_FREE_STRATEGY_CALLBACK callback_fun, u32 * p_action_id)
{
 	_int32 ret_val = SUCCESS;
#ifdef ENABLE_LIXIAN_ETM
	POST_PARA_5 param;
	
	if(!g_already_init)
		return -1;
	
    EM_LOG_DEBUG( "etm_get_free_strategy");
		
	if(NULL == session_id || NULL == callback_fun || NULL == p_action_id) return INVALID_ARGUMENT;

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(param));
	param._para1 = (void*)&user_id;
	param._para2 = (void*)session_id;
	param._para3 = (void*)user_data;
	param._para4 = (void*)callback_fun;
	param._para5 = (void*)p_action_id;
	
	ret_val =  em_post_function( lixian_get_free_strategy, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM  */
	return ret_val;
}

ETDLL_API int32 etm_get_free_experience_member(uint64 user_id, void* user_data, ETM_FREE_EXPERIENCE_MEMBER_CALLBACK callback_fun, u32 * p_action_id)
{
 	_int32 ret_val = SUCCESS;
#ifdef ENABLE_LIXIAN_ETM
	POST_PARA_4 param;
	
	if(!g_already_init)
		return -1;
	
    EM_LOG_DEBUG( "etm_get_free_experience_member");
		
	if(0 == user_id || NULL == callback_fun || NULL == p_action_id) return INVALID_ARGUMENT;

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(param));
	param._para1 = (void*)&user_id;
	param._para2 = (void*)user_data;
	param._para3 = (void*)callback_fun;
	param._para4 = (void*)p_action_id;
	
	ret_val =  em_post_function( lixian_get_free_experience_member, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM  */
	return ret_val;
}

ETDLL_API int32 etm_get_experience_member_remain_time(uint64 user_id, void* user_data, ETM_FREE_EXPERIENCE_MEMBER_CALLBACK callback_fun, u32 * p_action_id)
{
 	_int32 ret_val = SUCCESS;
#ifdef ENABLE_LIXIAN_ETM
	POST_PARA_4 param;
	
	if(!g_already_init)
		return -1;
	
    EM_LOG_DEBUG( "etm_get_experience_member_remain_time");
		
	if(0 == user_id || NULL == callback_fun || NULL == p_action_id) return INVALID_ARGUMENT;

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(param));
	param._para1 = (void*)&user_id;
	param._para2 = (void*)user_data;
	param._para3 = (void*)callback_fun;
	param._para4 = (void*)p_action_id;
	
	ret_val =  em_post_function( lixian_get_experience_member_remain_time, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM  */
	return ret_val;
}

ETDLL_API int32 etm_get_high_speed_flux(ETM_GET_HIGH_SPEED_FLUX *flux_param, u32 * p_action_id)
{
 	_int32 ret_val = SUCCESS;
#ifdef ENABLE_LIXIAN_ETM
	POST_PARA_2 param;
	
	if(!g_already_init)
		return -1;
	
    EM_LOG_DEBUG( "etm_get_high_speed_flux");
		
	if( NULL == flux_param ||  NULL == p_action_id) return INVALID_ARGUMENT;

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(param));
	param._para1 = (void*)flux_param;
	param._para2 = (void*)p_action_id;
	
	ret_val =  em_post_function( lixian_get_high_speed_flux, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM  */
	return ret_val;
}

/* ����: �Ӹ��ٿ۳���������ʹ���߷��صļ���url�ܹ�������û����� */
ETDLL_API int32 etm_lixian_take_off_flux_from_high_speed(uint64 task_id, uint64 file_id, void* user_data, ELX_TAKE_OFF_FLUX_CALLBACK callback_fun, u32 * p_action_id)
 {
 	 _int32 ret_val = SUCCESS;
#ifdef ENABLE_LIXIAN_ETM
	POST_PARA_5 param;
	
	if(!g_already_init)
		return -1;
	
    	EM_LOG_DEBUG( "etm_lixian_take_off_flux_from_high_speed");
		
	if(task_id==0|| callback_fun == NULL || p_action_id==NULL) return INVALID_ARGUMENT;

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(param));
	param._para1 = (void*)&task_id;
	param._para2 = (void*)&file_id;
	param._para3 = (void*)user_data;
	param._para4 = (void*)callback_fun;
	param._para5 = (void*)p_action_id;
	
	ret_val =  em_post_function( lixian_take_off_flux_from_high_speed, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM  */
	return ret_val;
 }


/* ����ҳ�л�ÿɹ����ص�url  */
ETDLL_API int32 etm_lixian_get_downloadable_url_from_webpage(const char* url,void* user_data,ELX_DOWNLOADABLE_URL_CALLBACK callback_fun , u32 * p_action_id)
 {
 	_int32 ret_val = SUCCESS;
#ifdef ENABLE_LIXIAN_ETM
	POST_PARA_4 param;
	
	if(!g_already_init)
		return -1;
	
    	EM_LOG_DEBUG( "etm_lixian_get_downloadable_url_from_webpage:%s",url);
		
	if(url==NULL||sd_strlen(url)<9||p_action_id==NULL) return INVALID_ARGUMENT;

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(param));
	param._para1 = (void*)url;
	param._para2 = user_data;
	param._para3 = (void*)callback_fun;
	param._para4 = (void*)p_action_id;

	ret_val =  em_post_function( lixian_get_downloadable_url_from_webpage, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM  */
	return ret_val;
 }

/* ���Ѿ���ȡ������ҳԴ�ļ�����̽�ɹ����ص�url ,ע��,�����ͬ���ӿڣ�����Ҫ���罻��  */
ETDLL_API int32 etm_lixian_get_downloadable_url_from_webpage_file(const char* url,const char * source_file_path,u32 * p_url_num)
 {
 	_int32 ret_val = SUCCESS;
#ifdef ENABLE_LIXIAN_ETM
	POST_PARA_3 param;
	
	if(!g_already_init)
		return -1;
	
    	EM_LOG_DEBUG( "etm_lixian_get_downloadable_url_from_webpage_file:%s",url);
		
	if(url==NULL||sd_strlen(url)<9||source_file_path==NULL||p_url_num==NULL) return INVALID_ARGUMENT;

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(param));
	param._para1 = (void*)url;
	param._para2 = (void*)source_file_path;
	param._para3 = (void*)p_url_num;

	ret_val =  em_post_function( lixian_get_downloadable_url_from_webpage_file, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM  */
	return ret_val;
 }

///// ��ȡ��ҳ�пɹ�����URL��id �б�,*buffer_len�ĵ�λ��sizeof(u32)
ETDLL_API int32 etm_lixian_get_downloadable_url_ids(const char* url,u32 * id_array_buffer,u32 *buffer_len)
 {
 	_int32 ret_val = SUCCESS;
#ifdef ENABLE_LIXIAN_ETM
	POST_PARA_3 param;
	
	if(!g_already_init)
		return -1;
	
    	EM_LOG_DEBUG( "etm_lixian_get_downloadable_url_ids");
		
	if(sd_strlen(url)<9 ||buffer_len==NULL) return INVALID_ARGUMENT;

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(param));
	param._para1 = (void*)url;
	param._para2 = (void*)id_array_buffer;
	param._para3 = (void*)buffer_len;

	ret_val =  em_post_function( lixian_get_downloadable_url_ids, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM  */
	return ret_val;
 }

/* ��ȡ�ɹ�����URL ��Ϣ */
ETDLL_API int32 etm_lixian_get_downloadable_url_info(u32 url_id,ELX_DOWNLOADABLE_URL * p_info)
 {
 	_int32 ret_val = SUCCESS;
#ifdef ENABLE_LIXIAN_ETM
	POST_PARA_2 param;
	
	if(!g_already_init)
		return -1;
	
    	EM_LOG_DEBUG( "etm_lixian_get_downloadable_url_info");
		
	if(url_id==0||p_info==NULL) return INVALID_ARGUMENT;

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(param));
	param._para1 = (void*)url_id;
	param._para2 = (void*)p_info;

	ret_val =  em_post_function( lixian_get_downloadable_url_info, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_LIXIAN_ETM  */
	return ret_val;
 }

/* �жϸ�url �Ƿ�ɹ����� */
extern BOOL em_is_url_downloadable( const char * url);
ETDLL_API BOOL etm_lixian_is_url_downloadable(const char* url)
 {
     if (sd_strlen(url)<=9) {
         return FALSE;
     }
	return em_is_url_downloadable(url);
 }

/* ��ȡurl������ */
extern EM_URL_TYPE em_get_url_type( const char * url);
ETDLL_API  ELX_URL_TYPE etm_lixian_get_url_type(const char* url)
 {
     if (sd_strlen(url)<=9) {
         return ELUT_ERROR;//��������url����Ϊ�ǲ������ص�url���򷵻�-1
     }
     EM_URL_TYPE url_type = em_get_url_type(url);
     if(url_type > UT_BT)
     {
     		url_type = ELUT_ERROR;//��������url����Ϊ�ǲ������ص�url���򷵻�-1
     }

     return url_type;
 }

//////////////////////////////////////////////////////////////
/////��¼ģ����ؽӿ�
//	ע��:����char *�ַ�����ΪUTF8��ʽ!

#ifdef ENABLE_MEMBER
#include "member/member_interface.h"
#endif /* ENABLE_NEIGHBOR */

//���õ�¼�ص��¼�����
ETDLL_API int32 etm_member_set_callback_func(ETM_MEMBER_EVENT_NOTIFY notify_func)
{
	_int32 ret_val = SUCCESS;
#ifdef ENABLE_MEMBER
	POST_PARA_1 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_member_set_callback_func" );
	
	if(notify_func == NULL) return INVALID_ARGUMENT;

	EM_CHECK_CRITICAL_ERROR;
	
	em_memset(&param,0,sizeof(param));
	param._para1= (void*)notify_func;

	ret_val = em_post_function( member_set_callback, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_MEMBER  */
	return ret_val;
}	

// �û���¼����������utf8��ʽ�����벻���ܡ�
ETDLL_API int32 etm_member_login(const char* username, const char* password)
{
	_int32 ret_val = SUCCESS;
#ifdef ENABLE_MEMBER
	POST_PARA_3 param;
	BOOL is_md5 = FALSE;
	
	if(!g_already_init)
		return -1;
	
    	EM_LOG_DEBUG( "etm_member_login" );

	if((username == NULL) || (password == NULL)) return INVALID_ARGUMENT;

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(param));
	param._para1 = (void*)username;
	param._para2 = (void*)password;
	param._para3 = (void*)is_md5;

	// �������ؿ��ϱ�
	em_set_username_password(username, password);

	ret_val = em_post_function( member_login,  (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_MEMBER  */
	return ret_val;
}

// sessionid��¼
ETDLL_API int32 etm_member_sessionid_login(const char* sessionid, uint64 userid)
{
	_int32 ret_val = SUCCESS;
#ifdef ENABLE_MEMBER
	POST_PARA_2 param;
	
	if(!g_already_init)
		return -1;
	
    EM_LOG_DEBUG( "etm_member_sessionid_login" );

	if( NULL == sessionid || 0 == userid ) return INVALID_ARGUMENT;

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(param));
	param._para1 = (void*)sessionid;
	param._para2 = (void*)&userid;

	ret_val = em_post_function( member_sessionid_login,  (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_MEMBER  */
	return ret_val;
}


// ���ܷ�ʽ��¼������ʹ�ñ��ر����MD5���ܺ�����룬��������utf8��ʽ
ETDLL_API int32 etm_member_encode_login(const char* username, const char* md5_pw)
{
	_int32 ret_val = SUCCESS;
#ifdef ENABLE_MEMBER
	POST_PARA_3 param;
	BOOL is_md5 = TRUE;
	
	if(!g_already_init)
		return -1;
	
    	EM_LOG_DEBUG( "etm_member_encode_login" );

	if((username == NULL) || (md5_pw == NULL)) return INVALID_ARGUMENT;

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(param));
	param._para1 = (void*)username;
	param._para2 = (void*)md5_pw;
	param._para3 = (void*)is_md5;

	ret_val = em_post_function( member_login,  (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_MEMBER  */
	return ret_val;
}

// 
ETDLL_API int32 etm_member_relogin(void)
{
	_int32 ret_val = SUCCESS;
#ifdef ENABLE_MEMBER
	POST_PARA_0 param;
	
	if(!g_already_init)
		return -1;
	
    EM_LOG_DEBUG( "etm_member_relogin" );

	em_memset(&param,0,sizeof(param));

	ret_val = em_post_function( member_relogin,  (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_MEMBER  */
	return ret_val;
}

// �û�ע����
ETDLL_API int32 etm_member_logout(void)
{
	_int32 ret_val = SUCCESS;
#ifdef ENABLE_MEMBER
	POST_PARA_0 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_member_logout" );

	EM_CHECK_CRITICAL_ERROR;
	
	em_memset(&param,0,sizeof(param));

	ret_val =  em_post_function( member_logout, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_MEMBER  */
	return ret_val;
}

// �ϱ������ļ���
/*ETDLL_API int32 etm_member_report_download_file(uint64 filesize, const char cid[40], char* url, u32 url_len)
{
	return NOT_IMPLEMENT;
}*/

// �û�����ping����ȷ���û�sessionid����Ч���ڡ�
ETDLL_API int32 etm_member_keepalive(void)
{
	_int32 ret_val = SUCCESS;
#ifdef ENABLE_MEMBER
	POST_PARA_0 param;
	
	if(!g_already_init)
		return -1;
	
    EM_LOG_DEBUG( "etm_member_keepalive" );

	EM_CHECK_CRITICAL_ERROR;

	em_memset(&param,0,sizeof(param));

	ret_val = em_post_function( member_keepalive_server, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_MEMBER  */
	return ret_val;
}

//���û�е�¼�ɹ����øú����᷵��ʧ��
ETDLL_API int32 etm_member_get_info(ETM_MEMBER_INFO* info)
{
	_int32 ret_val = SUCCESS;
#ifdef ENABLE_MEMBER
	POST_PARA_1 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_member_get_info" );

	if(info == NULL) return INVALID_ARGUMENT;

	EM_CHECK_CRITICAL_ERROR;
	
	em_memset(&param,0,sizeof(param));
	param._para1= (void*)info;

	ret_val = em_post_function( member_get_info, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_MEMBER  */
	return ret_val;	
}

//
ETDLL_API int32 etm_member_refresh_user_info(ETM_MEMBER_REFRESH_NOTIFY notify_func)
{
	_int32 ret_val = SUCCESS;
#ifdef ENABLE_MEMBER
	POST_PARA_1 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_member_refresh_user_info" );

	if(notify_func == NULL) return INVALID_ARGUMENT;

	EM_CHECK_CRITICAL_ERROR;
	
	em_memset(&param,0,sizeof(param));
	param._para1= (void*)notify_func;

	ret_val = em_post_function( member_refresh_user_info, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val =  NOT_IMPLEMENT;
#endif /* ENABLE_MEMBER  */
	return ret_val;	
}

//��ȡ�û�ͷ��ͼƬȫ·���ļ���������ͼƬ����NULL
ETDLL_API char * etm_member_get_picture(void)
{
#ifdef ENABLE_MEMBER
	POST_PARA_1 param;
	static char picture_path[MAX_FILE_PATH_LEN*2];
	_int32 ret_val=SUCCESS;
	
	if(!g_already_init)
		return NULL;
	
	EM_LOG_DEBUG("etm_member_get_picture");

	ETM_CHECK_CRITICAL_ERROR;

	em_memset(picture_path,0,MAX_FILE_PATH_LEN*2);
	em_memset(&param,0,sizeof(POST_PARA_1));
	param._para1= (void*)picture_path;
	
	ret_val= em_post_function( member_get_picture_filepath, (void *)&param,&(param._handle),&(param._result));
	if(ret_val==SUCCESS)
	{
		sd_assert(em_strlen(picture_path)>0);
		return picture_path;
	}
	return NULL;
#else
	return NULL;
#endif /* ENABLE_MEMBER  */
}

//��ȡ��¼״̬
ETDLL_API ETM_MEMBER_STATE etm_member_get_state(void)
{
#ifdef ENABLE_MEMBER
	POST_PARA_1 param;
	ETM_MEMBER_STATE member_state;
	_int32 ret_val=SUCCESS;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_member_get_state" );

	EM_CHECK_CRITICAL_ERROR;
	
	em_memset(&param,0,sizeof(param));
	param._para1= (void*)&member_state;

	ret_val = em_post_function( member_get_state, (void *)&param,&(param._handle),&(param._result));
	if(ret_val == SUCCESS)
	{
		return member_state;
	}
	else
	{
		return -1;
	}
#else
	return -1;
#endif /* ENABLE_MEMBER  */
}

//��ȡ��������͡�
//const char* etm_get_error_code_description(int32 errcode);

//�û�ע�ᣬ�鿴id�Ƿ����
ETDLL_API int32 etm_member_register_check_id(ETM_REGISTER_INFO* register_info)
{
	_int32 ret_val = SUCCESS;
#ifdef ENABLE_MEMBER
	POST_PARA_1 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_member_register_check_id" );

	if(register_info == NULL) return INVALID_ARGUMENT;

	EM_CHECK_CRITICAL_ERROR;
	
	em_memset(&param,0,sizeof(param));
	param._para1= (void*)register_info;

	ret_val =  em_post_function( member_register_check_id, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val = NOT_IMPLEMENT;
#endif /* ENABLE_MEMBER  */
	return ret_val;
}

//�û�ע�ᣬ��������utf8��ʽ��
ETDLL_API int32 etm_member_register_user(ETM_REGISTER_INFO* register_info)
{
	_int32 ret_val = SUCCESS;
#ifdef ENABLE_MEMBER
	POST_PARA_1 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_member_register_user" );

	if(register_info == NULL) return INVALID_ARGUMENT;

	EM_CHECK_CRITICAL_ERROR;
	
	em_memset(&param,0,sizeof(param));
	param._para1= (void*)register_info;

	ret_val = em_post_function( member_register_user, (void *)&param,&(param._handle),&(param._result));	
#else
	ret_val = NOT_IMPLEMENT;
#endif /* ENABLE_MEMBER  */
	return ret_val;
}

ETDLL_API int32 etm_member_register_cancel(void)
{
	_int32 ret_val = SUCCESS;
#ifdef ENABLE_MEMBER
	POST_PARA_0 param;
	
	if(!g_already_init)
		return -1;
	
	EM_LOG_DEBUG("etm_member_register_cancel" );

	EM_CHECK_CRITICAL_ERROR;
	
	em_memset(&param,0,sizeof(param));

	ret_val = em_post_function( member_register_cancel, (void *)&param,&(param._handle),&(param._result));
#else
	ret_val = NOT_IMPLEMENT;
#endif /* ENABLE_MEMBER  */
	return ret_val;
}


