#include "utility/sd_iconv.h"
#include "utility/mempool.h"
#include "utility/peerid.h"
#include "utility/local_ip.h"
#include "utility/version.h"
#include "utility/url.h"
#include "utility/utility.h"
#include "utility/settings.h"
#include "platform/sd_fs.h"
#include "platform/sd_network.h"
#include "utility/logid.h"
#define LOGID LOGID_REPORTER
#include "utility/logger.h"
#include "utility/peer_capability.h"

#include "asyn_frame/device.h"

#include "reporter_interface.h"

#include "reporter_cmd_define.h"
#include "reporter_setting.h"
#include "reporter_cmd_builder.h"
#include "reporter_impl.h"
#include "p2sp_download/p2sp_task.h"
#include "data_manager/data_manager_interface.h"
#include "embed_thunder_version.h"

#ifdef ENABLE_BT
#include "bt_download/bt_task/bt_task.h"
#include "bt_download/bt_data_manager/bt_data_manager_interface.h"
#endif /* ENABLE_BT */

#ifdef ENABLE_EMULE
#include "emule/emule_interface.h"
#endif /* ENABLE_EMULE */


#include "socket_proxy_interface.h"

#include "connect_manager/connect_manager_interface.h"


static REPORTER_DEVICE	g_license_device;
static REPORTER_DEVICE	g_report_shub;
static REPORTER_DEVICE	g_stat_hub;
#ifdef ENABLE_BT
static REPORTER_DEVICE	g_bt_hub;
#endif

#ifdef ENABLE_EMULE
static REPORTER_DEVICE	g_emule_hub;
#endif

#ifdef UPLOAD_ENABLE
#include "upload_manager/upload_manager.h"
static REPORTER_DEVICE	g_phub;
#endif

#ifdef EMBED_REPORT
static REPORTER_DEVICE	g_embed_hub;
REPORTER_DEVICE	* get_embed_hub()
{
	return &g_embed_hub;
}
#endif

#define NEW_TASK_CREATED_BY_KANKAN_PRIVATE 4
#define CONTINUED_TASK_CREATED_BY_KANKAN_PRIVATE 5

#define REPORT_STOP_TASK_MIN_TIME 3   //10 -> 3  by zhengzhe 

/*--------------------------------------------------------------------------*/
/*                Interfaces provid for task_manager				        */
/*--------------------------------------------------------------------------*/

_int32 init_reporter_module(void)
{
	_int32 ret=SUCCESS;
	LOG_DEBUG("init_reporter_module...");
	ret = init_reporter_setting();
	CHECK_VALUE(ret);
	
	ret = init_reporter_device(&g_license_device,REPORTER_LICENSE);
	CHECK_VALUE(ret);
	
	ret = init_reporter_device(&g_report_shub,REPORTER_SHUB);
	CHECK_VALUE(ret);
	
	ret = init_reporter_device(&g_stat_hub,REPORTER_STAT_HUB);
	CHECK_VALUE(ret);
	
#ifdef ENABLE_BT
	ret = init_reporter_device(&g_bt_hub,REPORTER_BT_HUB);
	CHECK_VALUE(ret);
#endif
	
#ifdef ENABLE_EMULE
	ret = init_reporter_device(&g_emule_hub,REPORTER_EMULE_HUB);
	CHECK_VALUE(ret);
#endif


#ifdef UPLOAD_ENABLE
	ret = init_reporter_device(&g_phub,REPORTER_PHUB);
	CHECK_VALUE(ret);
#endif	

#ifdef EMBED_REPORT
	ret = init_reporter_device(&g_embed_hub,REPORTER_EMBED);
	CHECK_VALUE(ret);
	reporter_init_timeout_event(FALSE);
#endif	

	return ret;
}

_int32 uninit_reporter_module(void)
{
	_int32 ret=SUCCESS;
	LOG_DEBUG("uninit_reporter_module...");
#ifdef EMBED_REPORT
	ret = uninit_reporter_device(&g_embed_hub);
	CHECK_VALUE(ret);
	reporter_uninit_timeout_event();
#endif	

#ifdef UPLOAD_ENABLE
	ret = uninit_reporter_device(&g_phub);
	CHECK_VALUE(ret);
#endif	
	
#ifdef ENABLE_BT
	ret = uninit_reporter_device(&g_bt_hub);
	CHECK_VALUE(ret);
#endif

#ifdef ENABLE_EMULE
    ret = uninit_reporter_device(&g_emule_hub);
    CHECK_VALUE(ret);
#endif

	ret = uninit_reporter_device(&g_stat_hub);
	CHECK_VALUE(ret);

	ret = uninit_reporter_device(&g_report_shub);
	CHECK_VALUE(ret);

	return uninit_reporter_device(&g_license_device);
}

_int32 force_close_report_module_res(void)
{
	LOG_DEBUG("force_close_report_module_res...");
#ifdef EMBED_REPORT
	force_close_reporter_device_res(&g_embed_hub);
#endif	

#ifdef UPLOAD_ENABLE
	force_close_reporter_device_res(&g_phub);
#endif	
	
#ifdef ENABLE_BT
	 force_close_reporter_device_res(&g_bt_hub);	
#endif

#ifdef ENABLE_EMULE
     force_close_reporter_device_res(&g_emule_hub);    
#endif

	force_close_reporter_device_res(&g_stat_hub);
	
	force_close_reporter_device_res(&g_report_shub);
	
	force_close_reporter_device_res(&g_license_device);	

	return SUCCESS;
}

/* license report */
_int32 reporter_license(void)
{	
	char* buffer = NULL;
	_u32  len = 0,_ip=0;
	/*fill request package*/
	REPORT_LICENSE_CMD cmd;
	_int32 ret_val = SUCCESS;
	LOG_DEBUG("reporter_license...");
	sd_memset(&cmd, 0, sizeof(REPORT_LICENSE_CMD));

	cmd._peerid_size = PEER_ID_SIZE;
	ret_val=get_peerid(cmd._peerid, PEER_ID_SIZE + 1);
	if(ret_val!=SUCCESS)
	{
		LOG_DEBUG("reporter_license...Error when getting peerid!");
		return ret_val;
	}

	cmd._items_count = 5;

	sd_memset(cmd.partner_id_name,0,16);
	cmd.partner_id_name_len = sd_strlen("partner_id");
	sd_strncpy(cmd.partner_id_name, "partner_id", 16);

	ret_val = get_partner_id(cmd.partner_id, MAX_PARTNER_ID_LEN-1); 
	CHECK_VALUE(ret_val);
	cmd.partner_id_len = sd_strlen(cmd.partner_id);

	sd_memset(cmd.product_flag_name,0,16);
	cmd.product_flag_name_len = sd_strlen("product_flag");
	sd_strncpy(cmd.product_flag_name, "product_flag", 16);

	sd_snprintf(cmd.product_flag, 15,"%d", get_product_flag());
	cmd.product_flag_len = sd_strlen(cmd.product_flag);

	sd_memset(cmd.license_name,0,8);
	cmd.license_name_len = sd_strlen("license");
	sd_strncpy(cmd.license_name, "license", 8);

	ret_val = reporter_get_license(cmd.license, MAX_LICENSE_LEN); 
	CHECK_VALUE(ret_val);
	cmd.license_len = sd_strlen(cmd.license);

	sd_memset(cmd.ip_name,0,4);
	cmd.ip_name_len = sd_strlen("ip");
	sd_strncpy(cmd.ip_name, "ip", 4);

	_ip = sd_get_local_ip(); 
	sd_snprintf(cmd.ip, MAX_HOST_NAME_LEN-1,"%d.%d.%d.%d", (_ip)&0xFF, (_ip>>8)&0xFF, (_ip>>16)&0xFF, (_ip>>24)&0xFF);
	cmd.ip_len = sd_strlen(cmd.ip);

	sd_memset(cmd.os_name,0,4);
	cmd.os_name_len = sd_strlen("os");
	sd_strncpy(cmd.os_name, "os", 4);

	ret_val = sd_get_os(cmd.os, MAX_OS_LEN); 
	CHECK_VALUE(ret_val);
	cmd.os_len = sd_strlen(cmd.os);

	LOG_DEBUG("reporter_license:_peerid=%s,partner_id=%s,product_flag=%s,license=%s,ip=%s,os=%s,",cmd._peerid,cmd.partner_id,cmd.product_flag,cmd.license,cmd.ip,cmd.os);

	/*build command*/
	ret_val = reporter_build_report_license_cmd(&buffer, &len, &cmd);
	CHECK_VALUE(ret_val);
	return reporter_commit_cmd(&g_license_device, REPORT_LICENSE_CMD_TYPE, buffer, len, NULL, cmd._header._seq);			//commit a request
}



/*--------------------------------------------------------------------------*/
/*                Interfaces provid for download_task				        */
/*--------------------------------------------------------------------------*/
_int32 reporter_dw_stat(TASK* _p_task)
{
	char *buffer = NULL,*origin_url=NULL,*file_suffix=NULL,*file_name=NULL;
	_u32  len = 0;
	/*fill request package*/
	REPORT_DW_STAT_CMD cmd;
	_int32 ret_val = SUCCESS,is_only_origin = 0;
	P2SP_TASK * p_p2sp_task = (P2SP_TASK *)_p_task;
	LOG_DEBUG("report_dw_stat..._p_task=0x%X,task_id=%u",_p_task,_p_task->_task_id);
	sd_memset(&cmd, 0, sizeof(REPORT_DW_STAT_CMD));

	/*  Peerid */
	cmd._peerid_size = PEER_ID_SIZE;
	ret_val=get_peerid(cmd._peerid, PEER_ID_SIZE + 1);
	if(ret_val!=SUCCESS)
	{
		LOG_DEBUG("Error when getting peerid!");
		return ret_val;
	}

	LOG_DEBUG("get_peerid=%s",cmd._peerid);
	/* ContentID */
	/* Get the local cid from data_manager */			
	if((TRUE==dm_get_cid( &(p_p2sp_task->_data_manager), cmd._cid))
		&&(TRUE==sd_is_cid_valid(cmd._cid)))
	{
		cmd._cid_size = CID_SIZE;
	}
	else
	{
		LOG_DEBUG("report_dw_stat..._p_task=0x%X,task_id=%u:Error:REPORTER_ERR_GET_CID!",
			_p_task,_p_task->_task_id);
        return (REPORTER_ERR_GET_CID);
	}

	/* url_flag */
	if((p_p2sp_task->_task_created_type==TASK_CREATED_BY_URL)||
	   (p_p2sp_task->_task_created_type==TASK_CONTINUE_BY_URL))
		cmd.url_flag = URL_FLAG_COMMON|URL_FLAG_BY_URL;
	else
		cmd.url_flag = URL_FLAG_COMMON|URL_FLAG_BY_CID;

	if(!(cmd.url_flag&URL_FLAG_BY_CID))
	{
		/* originalUrl */
		origin_url = NULL;
		if(p_p2sp_task->_origion_url_length > 0)origin_url = p_p2sp_task->_origion_url;
		
		if(NULL != origin_url ||  TRUE ==  dm_get_origin_url(&(p_p2sp_task->_data_manager), &origin_url))
		{
			/* remove user name and password and decode */
			url_object_to_noauth_string(origin_url, cmd._origin_url,MAX_URL_LEN);
			cmd._origin_url_size = sd_strlen(cmd._origin_url);
		}
		else
		{
			LOG_DEBUG("report_dw_stat..._p_task=0x%X,task_id=%u:Error:REPORTER_ERR_GET_URL!",
				_p_task,_p_task->_task_id);
			return (REPORTER_ERR_GET_URL);
		}

		/* refererUrl */
		origin_url=NULL;
		if(TRUE ==  dm_get_origin_ref_url(&(p_p2sp_task->_data_manager), &origin_url))
		{
			sd_strncpy(cmd._refer_url, origin_url,MAX_URL_LEN-1);
			cmd._refer_url_size = sd_strlen(cmd._refer_url);
		}
	}

	cmd._file_size = _p_task->file_size;
	
	dm_get_dl_bytes(&(p_p2sp_task->_data_manager), &cmd.size_by_server, &cmd.size_by_peer, &cmd.size_by_other ,NULL, NULL);

	cmd.fault_block_size = _p_task->_fault_block_size;

	if(_p_task->_finished_time>_p_task->_start_time)
		cmd.download_time =(_int32)( _p_task->_finished_time-_p_task->_start_time);
	else
		cmd.download_time=1;

	file_suffix = dm_get_file_suffix(&(p_p2sp_task->_data_manager));
	if((file_suffix!=NULL)&&(file_suffix[0]!='\0'))
	{
		sd_strncpy(cmd.file_suffix, file_suffix,MAX_SUFFIX_LEN-1);
		cmd.file_suffix_len = sd_strlen(cmd.file_suffix);
	}

	cmd.total_server_res = (_u8)(_p_task->_total_server_res_count);
	cmd.valid_server_res = (_u8)(_p_task->_valid_server_res_count);

	cmd.is_nated = (_u8)(_p_task->is_nated);

	cmd.dw_comefrom = _p_task->dw_comefrom;
	cmd.product_flag = get_product_flag();

	ret_val = get_version(cmd.thunder_version, MAX_VERSION_LEN-1); 
	CHECK_VALUE(ret_val);
	cmd.thunder_version_len = sd_strlen(cmd.thunder_version);

	ret_val = get_partner_id(cmd.partner_id, MAX_PARTNER_ID_LEN-1); 
	CHECK_VALUE(ret_val);
	cmd.partner_id_len = sd_strlen(cmd.partner_id);

	cmd.download_ip =p_p2sp_task->_origin_url_ip;
	cmd.lan_ip = (_int32) (sd_get_local_ip());

	cmd.download_stat =_p_task->download_stat;

	/* Gcid */
	/* Get the local cid from data_manager */			
	if((TRUE==dm_get_calc_gcid( &(p_p2sp_task->_data_manager), cmd._gcid))
		&& (TRUE==sd_is_cid_valid(cmd._gcid)))
	{
		cmd._gcid_size = CID_SIZE;
	}
	else
	if((TRUE==dm_get_shub_gcid( &(p_p2sp_task->_data_manager), cmd._gcid))
		&&(TRUE==sd_is_cid_valid(cmd._gcid)))
	{
		cmd._gcid_size = CID_SIZE;
	}
	else
	{
		LOG_DEBUG("report_dw_stat..._p_task=0x%X,task_id=%u:Error:REPORTER_ERR_GET_GCID!",
			_p_task,_p_task->_task_id);
        return (REPORTER_ERR_GET_GCID);
	}

	cmd.CDN_number =(_u8) (_p_task->_cdn_num);

	if(p_p2sp_task->_is_shub_return_file_size==TRUE)
	{
		cmd.download_ur_info |= DUL_FILE_SIZE_OK;
	}

	if(p_p2sp_task->_is_shub_ok==TRUE)
	{
		cmd.download_ur_info|= DUL_QUERY_SHUB_OK;
	}

	settings_get_int_item("connect_manager.is_only_using_origin_server", &is_only_origin);

	if(is_only_origin!=0)
	{
		cmd.download_ur_info|= DUL_ONLY_ORIGIN;
	}
	
	if(p_p2sp_task->_is_phub_ok==TRUE)
	{
		cmd.download_ur_info|= DUL_QUERY_PHUB_OK;
	}

	if(p_p2sp_task->_is_tracker_ok==TRUE)
	{
		cmd.download_ur_info|= DUL_QUERY_TRACKER_OK;
	}

	if(TRUE==dm_get_filnal_file_name( &(p_p2sp_task->_data_manager),&file_name))
	{
		sd_strncpy(cmd._file_name, file_name,MAX_FILE_NAME_LEN-1);
		cmd._file_name_size = sd_strlen(cmd._file_name);
	}
	else
	{
		LOG_DEBUG("report_dw_stat..._p_task=0x%X,task_id=%u:Error:REPORTER_ERR_GET_FILE_NAME!",
			_p_task,_p_task->_task_id);
		return (REPORTER_ERR_GET_FILE_NAME);
	}

#ifdef REP_DW_VER_54
	cmd._task_info_size = 0;
	cmd._task_info[0] = '\0';
#endif /* REP_DW_VER_54 */

	/*build command*/
	ret_val = reporter_build_report_dw_stat_cmd(&buffer, &len, &cmd);
	CHECK_VALUE(ret_val);
	return reporter_commit_cmd(&g_stat_hub, REPORT_DW_STAT_CMD_TYPE, buffer, len, _p_task, cmd._header._seq);			//commit a request
}


_int32 reporter_dw_fail(TASK* _p_task)
{
	char* buffer = NULL;
	_u32  len = 0;
	/*fill request package*/
	REPORT_DW_FAIL_CMD cmd;
	_int32 ret_val = SUCCESS;
	P2SP_TASK * p_p2sp_task = (P2SP_TASK *)_p_task;
	char *url=NULL;
	LOG_DEBUG("reporter_dw_fail..._p_task=0x%X,task_id=%u",_p_task,_p_task->_task_id);
	sd_memset(&cmd, 0, sizeof(REPORT_DW_FAIL_CMD));

	cmd._peerid_size = PEER_ID_SIZE;
	ret_val=get_peerid(cmd._peerid, PEER_ID_SIZE + 1);
	if(ret_val!=SUCCESS)
	{
		LOG_DEBUG("Error when getting peerid!");
		return ret_val;
	}

	LOG_DEBUG("get_peerid=%s",cmd._peerid);
	/* url */
	if((p_p2sp_task->_task_created_type==TASK_CREATED_BY_URL)||(p_p2sp_task->_task_created_type==TASK_CONTINUE_BY_URL))
	{
		url = NULL;
		if(p_p2sp_task->_origion_url_length > 0)url = p_p2sp_task->_origion_url;
		
		if(NULL != url ||  TRUE ==  dm_get_origin_url(&(p_p2sp_task->_data_manager), &url))
		{
			/* remove user name and password and decode */
			url_object_to_noauth_string(url, cmd._url,MAX_URL_LEN);
			cmd._url_size = sd_strlen(cmd._url);
		}
		else
		{
			LOG_DEBUG("reporter_dw_fail..._p_task=0x%X,task_id=%u:Error:REPORTER_ERR_GET_URL!",_p_task,_p_task->_task_id);
            return (REPORTER_ERR_GET_URL);
		}
		
		/* _page_url */
		url=NULL;
		 if(TRUE ==  dm_get_origin_ref_url(&(p_p2sp_task->_data_manager), &url))
		{
			sd_strncpy(cmd._page_url, url,MAX_URL_LEN-1);
			cmd._page_url_size = sd_strlen(cmd._page_url);
		}
	
	}

	cmd.fail_code = _p_task->failed_code;

	/* ContentID */
	/* Get the local cid from data_manager */			
	if((TRUE==dm_get_cid( &(p_p2sp_task->_data_manager), cmd._cid))
		&&(TRUE==sd_is_cid_valid(cmd._cid)))
	{
		cmd._cid_size = CID_SIZE;
	}
	else
	{
		LOG_DEBUG("reporter_dw_fail..._p_task=0x%X,task_id=%u:Error:REPORTER_ERR_GET_CID!",_p_task,_p_task->_task_id);
        return REPORTER_ERR_GET_CID;
	}

	cmd._file_size = _p_task->file_size;

	cmd.dw_comefrom = _p_task->dw_comefrom;
	cmd.product_flag = get_product_flag();

	ret_val = get_version(cmd.thunder_version, MAX_VERSION_LEN-1); 
	CHECK_VALUE(ret_val);
	cmd.thunder_version_len = sd_strlen(cmd.thunder_version);

	ret_val = get_partner_id(cmd.partner_id, MAX_PARTNER_ID_LEN-1); 
	CHECK_VALUE(ret_val);
	cmd.partner_id_len = sd_strlen(cmd.partner_id);

	/* remove user name and password and decode */
	//url_object_to_noauth_string(_p_task->redirect_url, cmd._redirect_url,MAX_URL_LEN);
	//cmd._redirect_url_size = sd_strlen(cmd._redirect_url);
	//cmd._redirect_url_size = _p_task->redirect_url_length;
	//sd_memcpy(cmd._redirect_url, _p_task->redirect_url,MAX_URL_LEN);

	cmd.download_ip =p_p2sp_task->_origin_url_ip;
	cmd.lan_ip = (_int32) (sd_get_local_ip());


/*
	_int32	limit_upload;
*/
	cmd.download_stat =_p_task->download_stat;
	cmd.server_rc_num = (_int32)(_p_task->_total_server_res_count);
	cmd.peer_rc_num = (_int32)(_p_task->_total_peer_res_count);
	cmd.CDN_number =(_u8) (_p_task->_cdn_num);


/*
	_u32	_statistic_page_url_size;
	char		_statistic_page_url[MAX_URL_LEN];

	_int32	recommend_download_stat;
*/

	cmd.download_ratio =(_int32) (_p_task->progress);

	/*build command*/
	ret_val = reporter_build_report_dw_fail_cmd(&buffer, &len, &cmd);
	CHECK_VALUE(ret_val);
	return reporter_commit_cmd(&g_stat_hub, REPORT_DW_FAIL_CMD_TYPE, buffer, len, _p_task, cmd._header._seq);			//commit a request
}

/* INSERTSRES */
_int32 reporter_insertsres(TASK* _p_task)
{	
	char tmp_url[MAX_URL_LEN];
	char* buffer = NULL,*url=NULL,*file_suffix=NULL;
	_u32  len = 0;
	P2SP_TASK * p_p2sp_task = (P2SP_TASK *)_p_task;
	/*fill request package*/
	REPORT_INSERTSRES_CMD cmd;
	_int32 ret_val = SUCCESS;
	LOG_DEBUG("reporter_insertsres..._p_task=0x%X,task_id=%u, insert course:%d",_p_task,_p_task->_task_id, p_p2sp_task->_insert_course);
	sd_memset(&cmd, 0, sizeof(REPORT_INSERTSRES_CMD));

	if(_p_task->file_size<MIN_REPORT_FILE_SIZE) return SUCCESS;
	//if(PIPE_FILE_HTML_TYPE==dm_get_file_type( &(p_p2sp_task->_data_manager )))
	//{
	//	return SUCCESS;
	//}
	
	cmd._peerid_size = PEER_ID_SIZE;
	ret_val=get_peerid(cmd._peerid, PEER_ID_SIZE + 1);
	if(ret_val!=SUCCESS)
	{
		LOG_DEBUG("Error when getting peerid!");
		return ret_val;
	}

	if((p_p2sp_task->_task_created_type==TASK_CREATED_BY_URL)||(p_p2sp_task->_task_created_type==TASK_CONTINUE_BY_URL))
	{
		url = NULL;
		if(p_p2sp_task->_origion_url_length > 0)
		{
			url = p_p2sp_task->_origion_url;
		}
		 if(NULL != url || TRUE ==  dm_get_origin_url(&(p_p2sp_task->_data_manager), &url))
		{
			if(TRUE==sd_is_url_has_user_name(url))
			{
				cmd.have_password = 1;
			}

			/* remove user name and password and decode */
			url_object_to_noauth_string(url, tmp_url,MAX_URL_LEN);
			cmd._origin_url_size = sd_strlen(tmp_url);
			

			if(cmd._origin_url_size > 0)
			{
				_u32 utf8_output_len = MAX_URL_LEN - 1;
				sd_any_format_to_utf8(tmp_url, cmd._origin_url_size,  cmd._origin_url, &utf8_output_len);
				cmd._origin_url_size  = utf8_output_len;
			}
		}
		else
		{
			LOG_DEBUG("reporter_insertsres..._p_task=0x%X,task_id=%u:Error:REPORTER_ERR_GET_URL!",_p_task,_p_task->_task_id);
			return (REPORTER_ERR_GET_URL);
		}
		
		/* _refer_url */
		url=NULL;
		 if(TRUE ==  dm_get_origin_ref_url(&(p_p2sp_task->_data_manager), &url))
		{
			sd_strncpy(tmp_url, url,MAX_URL_LEN-1);
			cmd._refer_url_size = sd_strlen(tmp_url);
			if(cmd._refer_url_size > 0)
			{
				_u32 utf8_output_len = MAX_URL_LEN - 1;
				sd_any_format_to_utf8(tmp_url, cmd._refer_url_size,  cmd._refer_url, &utf8_output_len);
				cmd._refer_url_size  = utf8_output_len;
			}
		}
	
		/* remove user name and password and decode */
		//url_object_to_noauth_string(_p_task->redirect_url, cmd._redirect_url,MAX_URL_LEN);
		//cmd._redirect_url_size = sd_strlen(cmd._redirect_url);
	}

	cmd._file_size = _p_task->file_size;
	
	/* ContentID */
	/* Get the local cid from data_manager */			
	if((TRUE==dm_get_cid( &(p_p2sp_task->_data_manager), cmd._cid))
		&&(TRUE==sd_is_cid_valid(cmd._cid)))
	{
		cmd._cid_size = CID_SIZE;
	}
	else
	{
		LOG_DEBUG("reporter_insertsres..._p_task=0x%X,task_id=%u:Error:REPORTER_ERR_GET_CID!",_p_task,_p_task->_task_id);
		return (REPORTER_ERR_GET_CID);
	}

	/* Gcid */
	/* Get the local cid from data_manager */			
	if((TRUE==dm_get_calc_gcid( &(p_p2sp_task->_data_manager), cmd._gcid))
		&&(TRUE==sd_is_cid_valid(cmd._gcid)))
	{
		cmd._gcid_size = CID_SIZE;
	}else
	if((TRUE==dm_get_shub_gcid( &(p_p2sp_task->_data_manager), cmd._gcid))
		&&(TRUE==sd_is_cid_valid(cmd._gcid)))
	{
		cmd._gcid_size = CID_SIZE;
	}
	else
	{
		LOG_DEBUG("reporter_insertsres..._p_task=0x%X,task_id=%u:Error:REPORTER_ERR_GET_GCID!",_p_task,_p_task->_task_id);
		return (REPORTER_ERR_GET_GCID);
	}

	cmd._gcid_level = p_p2sp_task->_new_gcid_level;

	cmd._gcid_part_size = dm_get_block_size( &(p_p2sp_task->_data_manager));
	if(TRUE==dm_get_bcids( &(p_p2sp_task->_data_manager),&(cmd._bcid_size), &(cmd._bcid)))
	{
		cmd._bcid_size*= CID_SIZE;
	}
	else
	{
		LOG_DEBUG("reporter_insertsres..._p_task=0x%X,task_id=%u:Error:REPORTER_ERR_GET_BCID!",_p_task,_p_task->_task_id);
		return (REPORTER_ERR_GET_BCID);
	}
	
	file_suffix = dm_get_file_suffix(&(p_p2sp_task->_data_manager));
	if((file_suffix!=NULL)&&(file_suffix[0]!='\0'))
	{
		sd_strncpy(cmd.file_suffix, file_suffix,MAX_SUFFIX_LEN-1);
		cmd.file_suffix_len = sd_strlen(cmd.file_suffix);
	}

	cmd.download_status = p_p2sp_task->_download_status;

	cmd.download_ip =(_int32) (sd_get_local_ip());
	cmd.download_stat =_p_task->download_stat;
	cmd.insert_course = p_p2sp_task->_insert_course;

	cmd._origin_url_code_page = -1;
	cmd._refer_url_code_page = -1;
	cmd._redirect_url_code_page = -1;

#ifdef REP_DW_VER_54
	ret_val = get_partner_id(cmd.partner_id, MAX_PARTNER_ID_LEN-1); 
	CHECK_VALUE(ret_val);
	cmd.partner_id_len = sd_strlen(cmd.partner_id);
	cmd.product_flag = get_product_flag();
	cmd._refer_url_type = 0;
#endif /* REP_DW_VER_54 */

	LOG_DEBUG("reporter_insertsres cmd:peerid:%s _origin_url:%s, _refer_url:%s", cmd._peerid, cmd._origin_url, cmd._refer_url);
	LOG_DEBUG("reporter_insertsres cmd:_redirect_url:%s _file_size:%llu", cmd._redirect_url, cmd._file_size);

#ifdef _DEBUG
{
	char t_cid[41] = {0}, t_gcid[41] = {0};
	str2hex((const char*)cmd._cid, CID_SIZE, t_cid, 40);
	str2hex((const char*)cmd._gcid, CID_SIZE, t_gcid, 40);
	//char* t_bcid = NULL;
	//if(cmd._bcid_size > 0)
	//{
		//sd_malloc(cmd._bcid_size*2 + 1, &t_bcid);
		//t_bcid[cmd._bcid_size*2] = 0;
		//str2hex(cmd._bcid, cmd._bcid_size, t_bcid, cmd._bcid_size * 2);
	//}
	LOG_DEBUG("reporter_insertsres gcid:%s, cid:%s", t_gcid, t_cid);
	//LOG_DEBUG("reporter_insertsres bcidsize:%u, bcid:%s", cmd._bcid_size, t_bcid);
}
#endif
	

	/*build command*/
	ret_val = reporter_build_report_insertsres_cmd(&buffer, &len, &cmd);
	CHECK_VALUE(ret_val);
	return reporter_commit_cmd(&g_report_shub, REPORT_INSERTSRES_CMD_TYPE, buffer, len, _p_task, cmd._header._seq);			//commit a request
}

/* 
----------------BT task reprot commands ------------------------ */
#ifdef ENABLE_BT
//INSERTBTRES
_int32 reporter_bt_insert_res(TASK* _p_task,_u32 file_index)
{
	char *buffer = NULL;
	_u32  len = 0;
	/*fill request package*/
	REPORT_INSERTBTRES_CMD cmd;
	_int32 ret_val = SUCCESS;
	BT_TASK * p_bt_task = (BT_TASK *)_p_task;
	_u8 *info_hash=NULL;
	TORRENT_FILE_INFO *p_file_info=NULL;
	
	LOG_DEBUG("reporter_bt_insert_res..._p_task=0x%X,task_id=%u,file_index=%u",_p_task,_p_task->_task_id,file_index);
	sd_memset(&cmd, 0, sizeof(REPORT_INSERTBTRES_CMD));

	/*  Peerid */
	cmd._peerid_size = PEER_ID_SIZE;
	ret_val=get_peerid(cmd._peerid, PEER_ID_SIZE + 1);
	if(ret_val!=SUCCESS)
	{
		LOG_DEBUG("Error when getting peerid!");
		return ret_val;
	}

	ret_val = tp_get_file_info_hash(  p_bt_task->_torrent_parser_ptr, &info_hash );
	CHECK_VALUE(ret_val);
	sd_memcpy(cmd._info_id, info_hash,CID_SIZE);
	cmd._info_id_size = CID_SIZE;

	cmd._file_index= file_index;

	cmd._cource = 1; //上报原因（b0：服务器无信息，b1：未返回gcid，b2：未返回bcid，b3：cid计算不同，b4：gcid计算不同，b5：filesize不同，b6：其它原因。

	ret_val = tp_get_file_info( p_bt_task->_torrent_parser_ptr,  file_index, &p_file_info );
	CHECK_VALUE(ret_val);

	if(p_file_info->_file_size<MIN_REPORT_FILE_SIZE) return SUCCESS;

	cmd._file_size = p_file_info->_file_size;

	if((TRUE==bdm_get_cid( &(p_bt_task->_data_manager), file_index,cmd._cid))
		&&(TRUE==sd_is_cid_valid(cmd._cid)))
	{
			cmd._cid_size = CID_SIZE;
	}
	else
	{
		LOG_DEBUG("reporter_bt_insert_res..._p_task=0x%X,task_id=%u:Error:REPORTER_ERR_GET_CID!",_p_task,_p_task->_task_id);
		return (REPORTER_ERR_GET_CID);
	}
		
	if((TRUE==bdm_get_calc_gcid( &(p_bt_task->_data_manager),  file_index,cmd._gcid))
		&&(TRUE==sd_is_cid_valid(cmd._gcid)))
	{
		cmd._gcid_size = CID_SIZE;
	}else
	if((TRUE==bdm_get_shub_gcid( &(p_bt_task->_data_manager), file_index, cmd._gcid))
		&&(TRUE==sd_is_cid_valid(cmd._gcid)))
	{
		cmd._gcid_size = CID_SIZE;
	}
	else
	{
		LOG_DEBUG("reporter_bt_insert_res..._p_task=0x%X,task_id=%u:Error:REPORTER_ERR_GET_GCID!",_p_task,_p_task->_task_id);
		return (REPORTER_ERR_GET_GCID);
	}

	cmd._gcid_level= 1;  //全文CID级别：1：Peer级别 10：多Server级别 90：原始URL级别

	cmd._gcid_part_size= bdm_get_file_block_size( &(p_bt_task->_data_manager),  file_index );

	if(TRUE==bdm_get_bcids(&(p_bt_task->_data_manager),  file_index, &cmd._bcid_size, &cmd._bcid))
	{
		cmd._bcid_size*=CID_SIZE;
	}
	else
	{
		LOG_DEBUG("reporter_bt_insert_res..._p_task=0x%X,task_id=%u:Error:REPORTER_ERR_GET_BCID!",_p_task,_p_task->_task_id);
		return (REPORTER_ERR_GET_BCID);
	}

	cmd._file_total_size=  tp_get_file_total_size( p_bt_task->_torrent_parser_ptr );
	cmd._start_offset= p_file_info->_file_offset;
	cmd._block_size = tp_get_piece_size( p_bt_task->_torrent_parser_ptr );

#ifdef REP_DW_VER_54
	ret_val = get_partner_id(cmd.partner_id, MAX_PARTNER_ID_LEN-1); 
	CHECK_VALUE(ret_val);
	cmd.partner_id_len = sd_strlen(cmd.partner_id);
	cmd.product_flag = get_product_flag();
#endif /* REP_DW_VER_54 */

	/*build command*/
	ret_val = reporter_build_report_bt_insert_res_cmd(&buffer, &len, &cmd);
	CHECK_VALUE(ret_val);
	return reporter_commit_cmd(&g_bt_hub, REPORT_BT_INSERT_RES_CMD_TYPE, buffer, len, _p_task, cmd._header._seq);			//commit a request
}

//REPORTBTDOWNLOADSTAT	
_int32 reporter_bt_download_stat(TASK* _p_task,_u32 file_index)
{
	char *buffer = NULL;
	_u32  len = 0;
	/*fill request package*/
	REPORT_BT_DL_STAT_CMD cmd;
	_int32 ret_val = SUCCESS;
	BT_TASK * p_bt_task = (BT_TASK *)_p_task;
	_u8 *info_hash=NULL;
	char * p_str=NULL;
	TORRENT_FILE_INFO *p_file_info=NULL;
	
	LOG_DEBUG("reporter_bt_download_stat..._p_task=0x%X,task_id=%u,file_index=%u",_p_task,_p_task->_task_id,file_index);
	sd_memset(&cmd, 0, sizeof(REPORT_BT_DL_STAT_CMD));

	/*  Peerid */
	cmd._peerid_size = PEER_ID_SIZE;
	ret_val=get_peerid(cmd._peerid, PEER_ID_SIZE + 1);
	if(ret_val!=SUCCESS)
	{
		LOG_DEBUG("Error when getting peerid!");
		return ret_val;
	}

	ret_val = tp_get_file_info_hash(  p_bt_task->_torrent_parser_ptr, &info_hash );
	CHECK_VALUE(ret_val);
	sd_memcpy(cmd._info_id, info_hash,CID_SIZE);
	cmd._info_id_size = CID_SIZE;

	cmd._file_index= file_index;
	
	ret_val = tp_get_seed_title_name(  p_bt_task->_torrent_parser_ptr, &p_str );
	CHECK_VALUE(ret_val);
	sd_strncpy(cmd._title, p_str,MAX_FILE_NAME_BUFFER_LEN-1);
	cmd._title_size = sd_strlen(cmd._title);

	ret_val = tp_get_file_info( p_bt_task->_torrent_parser_ptr,  file_index, &p_file_info );
	CHECK_VALUE(ret_val);

	cmd._file_size = p_file_info->_file_size;

	if((TRUE==bdm_get_cid( &(p_bt_task->_data_manager), file_index,cmd._cid))
		&&(TRUE==sd_is_cid_valid(cmd._cid)))
	{
			cmd._cid_size = CID_SIZE;
	}
	else
	{
		LOG_DEBUG("reporter_bt_download_stat..._p_task=0x%X,task_id=%u:Error:REPORTER_ERR_GET_CID!",_p_task,_p_task->_task_id);
		return (REPORTER_ERR_GET_CID);
	}

	if((TRUE==bdm_get_calc_gcid( &(p_bt_task->_data_manager),  file_index,cmd._gcid))
		&&(TRUE==sd_is_cid_valid(cmd._gcid)))
	{
		cmd._gcid_size = CID_SIZE;
	}else
	if((TRUE==bdm_get_shub_gcid( &(p_bt_task->_data_manager), file_index, cmd._gcid))
		&&(TRUE==sd_is_cid_valid(cmd._gcid)))
	{
		cmd._gcid_size = CID_SIZE;
	}
	else
	{
		LOG_DEBUG("reporter_bt_download_stat..._p_task=0x%X,task_id=%u:Error:REPORTER_ERR_GET_GCID!",_p_task,_p_task->_task_id);
		return (REPORTER_ERR_GET_GCID);
	}

	cmd._file_name_size = p_file_info->_file_name_len;
	sd_strncpy(cmd._file_name, p_file_info->_file_name, MAX_FILE_NAME_BUFFER_LEN-1);

	cmd._bt_dl_size = cmd._file_size;
	cmd._server_dl_size = 0;
	cmd._peer_dl_size= cmd._file_size-cmd._bt_dl_size-cmd._server_dl_size;
	
	cmd._duration=bdm_get_file_download_time( &(p_bt_task->_data_manager), file_index );
	if(cmd._duration==0)
		cmd._duration=1;

	cmd._avg_speed=cmd._file_size/cmd._duration;

	cmd.product_flag = get_product_flag();

	ret_val = get_version(cmd.thunder_version, MAX_VERSION_LEN-1); 
	CHECK_VALUE(ret_val);
	cmd.thunder_version_len = sd_strlen(cmd.thunder_version);

	ret_val = get_partner_id(cmd.partner_id, MAX_PARTNER_ID_LEN-1); 
	CHECK_VALUE(ret_val);
	cmd.partner_id_len = sd_strlen(cmd.partner_id);

	cmd._file_total_size=  tp_get_file_total_size( p_bt_task->_torrent_parser_ptr );
	cmd._start_offset= p_file_info->_file_offset;
	cmd._block_size = tp_get_piece_size( p_bt_task->_torrent_parser_ptr );

	cmd._data_from_oversi= 0;
	cmd._bytes_by_emule= 0;
	
#ifdef REP_DW_VER_60
	cmd._download_bytes_from_dcache= 0;
#endif /* REP_DW_VER_54 */


	/*build command*/
	ret_val = reporter_build_report_bt_download_stat_cmd(&buffer, &len, &cmd);
	CHECK_VALUE(ret_val);
	return reporter_commit_cmd(&g_stat_hub, REPORT_BT_DL_STAT_CMD_TYPE, buffer, len, _p_task, cmd._header._seq);			//commit a request
}

//REPORTBTTASKDOWNLOADSTAT	 
_int32 reporter_bt_task_download_stat(TASK* _p_task)
{
	char *buffer = NULL;
	_u32  len = 0;
	/*fill request package*/
	REPORT_BT_TASK_DL_STAT_CMD cmd;
	_int32 ret_val = SUCCESS;
	BT_TASK * p_bt_task = (BT_TASK *)_p_task;
	_u8 *info_hash=NULL;
	char * p_str=NULL;
	
	LOG_DEBUG("reporter_bt_task_download_stat..._p_task=0x%X,task_id=%u",_p_task,_p_task->_task_id);
	sd_memset(&cmd, 0, sizeof(REPORT_BT_TASK_DL_STAT_CMD));

	/*  Peerid */
	cmd._peerid_size = PEER_ID_SIZE;
	ret_val=get_peerid(cmd._peerid, PEER_ID_SIZE + 1);
	if(ret_val!=SUCCESS)
	{
		LOG_DEBUG("Error when getting peerid!");
		return ret_val;
	}

	ret_val = tp_get_file_info_hash(  p_bt_task->_torrent_parser_ptr, &info_hash );
	CHECK_VALUE(ret_val);
	sd_memcpy(cmd._info_id, info_hash,CID_SIZE);
	cmd._info_id_size = CID_SIZE;

	ret_val = tp_get_seed_title_name(  p_bt_task->_torrent_parser_ptr, &p_str );
	CHECK_VALUE(ret_val);
	sd_strncpy(cmd._title, p_str,MAX_FILE_NAME_BUFFER_LEN-1);
	cmd._title_size = sd_strlen(cmd._title);

	cmd._file_number = tp_get_seed_file_num(  p_bt_task->_torrent_parser_ptr );
	
	cmd._file_size = tp_get_file_total_size( p_bt_task->_torrent_parser_ptr );

	cmd._block_size = tp_get_piece_size( p_bt_task->_torrent_parser_ptr );

	if(_p_task->_finished_time>_p_task->_start_time)
		cmd._duration =(_int32)( _p_task->_finished_time-_p_task->_start_time);
	else
		cmd._duration=1;

	cmd._avg_speed=cmd._file_size/cmd._duration;

/*
	_u32	_seed_url_size;
	char 	_seed_url[MAX_URL_LEN];
	_u32	_page_url_size;
	char 	_page_url[MAX_URL_LEN];
	_u32	_statistic_page_url_size;
	char 	_statistic_page_url[MAX_URL_LEN];
*/
	cmd.product_flag = get_product_flag();

	ret_val = get_version(cmd.thunder_version, MAX_VERSION_LEN-1); 
	CHECK_VALUE(ret_val);
	cmd.thunder_version_len = sd_strlen(cmd.thunder_version);

	ret_val = get_partner_id(cmd.partner_id, MAX_PARTNER_ID_LEN-1); 
	CHECK_VALUE(ret_val);
	cmd.partner_id_len = sd_strlen(cmd.partner_id);

	cmd._download_flag = 0;
	
#ifdef REP_DW_VER_54
	cmd.dw_comefrom = _p_task->dw_comefrom;
#endif /* REP_DW_VER_54 */


	/*build command*/
	ret_val = reporter_build_report_bt_task_download_stat_cmd(&buffer, &len, &cmd);
	CHECK_VALUE(ret_val);
	return reporter_commit_cmd(&g_stat_hub, REPORT_BT_TASK_DL_STAT_CMD_TYPE, buffer, len, _p_task, cmd._header._seq);			//commit a request
}

_int32 reporter_bt_task_normal_cdn_stat(TASK *task, int stat_type)
{
	_int32 ret_val = SUCCESS;
	char *buffer = NULL;
	_u32  len = 0;
	unsigned char * p_info_hash;
	BT_REPORT_NORMAL_CDN_STAT cmd;
	CM_REPORT_PARA *report_para = NULL;
	RES_QUERY_REPORT_PARA *query_para = NULL;
	
	BT_TASK *p_bt_task = (BT_TASK *)task;
	LOG_DEBUG("task = 0x%x, task_id = %u", task, task->_task_id);

	sd_memset(&cmd, 0, sizeof(BT_REPORT_NORMAL_CDN_STAT));

	cmd._stat_type = stat_type;
	tp_get_file_info_hash(  p_bt_task->_torrent_parser_ptr, &p_info_hash );
	cmd._info_id_size = INFO_HASH_LEN;
	sd_memcpy(cmd._info_id, p_info_hash,INFO_HASH_LEN);
	cmd._file_size = task->_file_size;
	// userid 暂时不需要
	cmd._user_id_size = 0;

	cmd._peerid_size = PEER_ID_SIZE;
	ret_val = get_peerid((char *)&(cmd._peerid[0]), PEER_ID_SIZE + 1);
	if(ret_val != SUCCESS)
	{
		LOG_DEBUG("Error when getting peerid!");
		return ret_val;
	}

	LOG_DEBUG("get_peerid = %s", cmd._peerid);

	report_para = cm_get_report_para(&(task->_connect_manager), INVALID_FILE_INDEX);
	if (!report_para) 
	{
		LOG_ERROR("can't get report parameter, so drop to report.");
		return REPORTER_ERR_BUILD_EMB_REPORT;
	}

	LOG_DEBUG("_res_server_total = %u, _res_peer_total = %u, _res_normal_cdn_total = %u.", 
		report_para->_res_server_total, report_para->_res_peer_total, report_para->_res_normal_cdn_total);

	cmd._normal_cdn_cnt = report_para->_res_normal_cdn_total;

	if ((report_para->_res_peer_total + report_para->_res_server_total) > report_para->_res_normal_cdn_total)
	{
		cmd._other_res_added = 1;
	}
	else
	{
		cmd._other_res_added = 0;
	}
	
	_u64 ser_bytes = 0, peer_bytes = 0, cdn_bytes = 0,lixian_bytes = 0, origion_bytes = 0;
	bdm_get_dl_bytes(&(p_bt_task->_data_manager), &ser_bytes, &peer_bytes, &cdn_bytes, &origion_bytes);
	cmd._total_down_bytes = ser_bytes + peer_bytes + cdn_bytes + origion_bytes;
	cmd._normal_cdn_down_bytes = bdm_get_normal_cdn_down_bytes(&(p_bt_task->_data_manager));
	cmd._normal_cdn_trans_time = report_para->_res_normal_cdn_sum_use_time;

	cmd._normal_cdn_conn_time = cm_get_normal_cdn_connect_time(&(task->_connect_manager));

	cmd._time_span_from_0_to_nonzero = task->_first_byte_arrive_time;

	cmd._time_sum_of_all_zero_period = task->_zero_speed_time_ms;

	if(task->_finished_time > task->_start_time)
		cmd._time_of_download = (_int32)(task->_finished_time - task->_start_time);
	else
		cmd._time_of_download = 1;

	query_para = dt_get_res_query_report_para(task);
	if (query_para)
	{
		cmd._query_normal_cdn_fail_times = query_para->_normal_cdn_fail;
	}

	cmd._task_magic_id = 0;

	_u32 mid_time = 0;
	sd_time(&mid_time);
	_u64 mid_rand = sd_rand();

	cmd._magic_id = (mid_rand << 32) | mid_time;

	cmd._random_id = mid_rand;

	/*build command*/
	ret_val = reporter_build_report_bt_normal_cdn_stat_cmd(&buffer, &len, &cmd);
	CHECK_VALUE(ret_val);

	return reporter_commit_cmd(&g_stat_hub, EMB_REPORT_BT_NORMAL_CDN_STAT_TYPE, buffer, len, task, 
		cmd._header._seq);
}

_int32 reporter_bt_sub_normal_cdn_stat(TASK *task, _u32 file_index, int stat_type)
{
	_int32 ret_val = SUCCESS;
	char *buffer = NULL;
	_u32  len = 0;
	BT_REPORT_NORMAL_CDN_STAT cmd;
	CM_REPORT_PARA *report_para = NULL;
	RES_QUERY_REPORT_PARA *query_para = NULL;
	unsigned char * p_info_hash;
	
	BT_TASK *p_bt_task = (BT_TASK *)task;
	BT_FILE_INFO *p_file_info=NULL;
	CONNECT_MANAGER * p_connect_manager=NULL;

	LOG_DEBUG("task = 0x%x, task_id = %u file_index=%d", task, task->_task_id,file_index);

	ret_val = map_find_node(&(p_bt_task->_file_info_map) , (void*)file_index,(void**)&p_file_info);
	if(ret_val!=SUCCESS) return BT_ERROR_FILE_INDEX;
#if 0
	ret_val = map_find_node(&(p_bt_task->_file_task_info_map), (void*)file_index,(void**)&p_file_task_info);
	if(ret_val!=SUCCESS) goto BT_ERROR_FILE_INDEX;
#endif
	cm_get_sub_connect_manager(&(task->_connect_manager), file_index,&p_connect_manager);
	if(ret_val!=SUCCESS) return BT_ERROR_FILE_INDEX;

	sd_memset(&cmd, 0, sizeof(BT_REPORT_NORMAL_CDN_STAT));

	cmd._stat_type = stat_type;
	cmd._info_id_size = INFO_HASH_LEN;
	tp_get_file_info_hash(  p_bt_task->_torrent_parser_ptr, &p_info_hash );
	sd_memcpy(cmd._info_id, p_info_hash,INFO_HASH_LEN);
	cmd._file_size = p_file_info->_file_size;
	// userid 暂时不需要
	cmd._user_id_size = 0;

	cmd._peerid_size = PEER_ID_SIZE;
	ret_val = get_peerid((char *)&(cmd._peerid[0]), PEER_ID_SIZE + 1);
	if(ret_val != SUCCESS)
	{
		LOG_DEBUG("Error when getting peerid!");
		return ret_val;
	}

	LOG_DEBUG("get_peerid = %s", cmd._peerid);


	report_para = cm_get_report_para(p_connect_manager, INVALID_FILE_INDEX);
	if (!report_para) 
	{
		LOG_ERROR("can't get report parameter, so drop to report.");
		return REPORTER_ERR_BUILD_EMB_REPORT;
	}

	LOG_DEBUG("_res_server_total = %u, _res_peer_total = %u, _res_normal_cdn_total = %u.", 
		report_para->_res_server_total, report_para->_res_peer_total, report_para->_res_normal_cdn_total);

	cmd._normal_cdn_cnt = report_para->_res_normal_cdn_total;

	if ((report_para->_res_peer_total + report_para->_res_server_total) > report_para->_res_normal_cdn_total)
	{
		cmd._other_res_added = 1;
	}
	else
	{
		cmd._other_res_added = 0;
	}
	
	_u64 ser_bytes = 0, peer_bytes = 0, cdn_bytes = 0,lixian_bytes = 0, origion_bytes = 0 ;
	bdm_get_sub_file_dl_bytes(&(p_bt_task->_data_manager),file_index, &ser_bytes, &peer_bytes, &cdn_bytes, &lixian_bytes, &origion_bytes);
	cmd._total_down_bytes = ser_bytes + peer_bytes + cdn_bytes + lixian_bytes+origion_bytes;
#if 0
	cmd._normal_cdn_down_bytes = dm_get_normal_cdn_down_bytes(&(p_p2sp_task->_data_manager));
#endif
	cmd._normal_cdn_trans_time = report_para->_res_normal_cdn_sum_use_time;

	cmd._normal_cdn_conn_time = cm_get_normal_cdn_connect_time(p_connect_manager);
#if 0
	cmd._time_span_from_0_to_nonzero = task->_first_byte_arrive_time;

	cmd._time_sum_of_all_zero_period = task->_zero_speed_time_ms;
#endif	
	cmd._time_of_download=bdm_get_file_download_time( &p_bt_task->_data_manager, file_index );
#if 0
	cmd._query_normal_cdn_fail_times = p_file_task_info->_res_query_normal_cdn_failcnt;
#endif
	cmd._task_magic_id = 0;

	cmd._magic_id = 0;

	cmd._random_id = sd_rand();

	/*build command*/
	ret_val = reporter_build_report_bt_normal_cdn_stat_cmd(&buffer, &len, &cmd);
	CHECK_VALUE(ret_val);

	return reporter_commit_cmd(&g_stat_hub, EMB_REPORT_BT_NORMAL_CDN_STAT_TYPE, buffer, len, task, 
		cmd._header._seq);
}
#endif /* ENABLE_BT */

#ifdef ENABLE_EMULE
_int32 reporter_emule_insert_res(TASK* _p_task)
{	
	char* buffer = NULL;
	_u32  len = 0;
	EMULE_TASK * p_emule_task = (EMULE_TASK *)_p_task;
	/*fill request package*/
	REPORT_INSERT_EMULE_RES_CMD cmd;
	_int32 ret_val = SUCCESS;
	LOG_DEBUG("reporter_emule_insert_res..._p_task=0x%X,task_id=%u",_p_task,_p_task->_task_id);
	sd_memset(&cmd, 0, sizeof(REPORT_INSERT_EMULE_RES_CMD));
	
	cmd._peerid_size = PEER_ID_SIZE;
	ret_val=get_peerid(cmd._peerid, PEER_ID_SIZE);
	if(ret_val!=SUCCESS)
	{
		LOG_DEBUG("Error when getting peerid!");
		return ret_val;
	}
  
	cmd._fileid_len = FILE_ID_SIZE;
    emule_get_file_id(p_emule_task->_data_manager, (_u8*)cmd._fileid );


	// 文件大小 
	cmd._file_size = _p_task->file_size;
    
    cmd._cource = 1;
    
	cmd._file_name_len = MAX_FILE_NAME_BUFFER_LEN;
    ret_val = emule_get_task_file_name( _p_task, cmd._file_name, (_int32*)&cmd._file_name_len );
    if( ret_val != SUCCESS )
    {
        cmd._file_name_len = 0;
    }

	/* aich hash */
    emule_get_aich_hash(p_emule_task->_data_manager, &cmd._aich_hash_ptr, &cmd._aich_hash_len );
    
	/* part hash */
    emule_get_part_hash(p_emule_task->_data_manager, &cmd._part_hash_ptr, &cmd._part_hash_len );

	/* cid */
    if((TRUE==emule_get_cid( p_emule_task->_data_manager, cmd._cid))
        &&(TRUE==sd_is_cid_valid(cmd._cid)))
    {
		cmd._cid_size = CID_SIZE;
    }
	else
	{
		LOG_DEBUG("reporter_emule_insert_res..._p_task=0x%X,task_id=%u:Error:REPORTER_ERR_GET_CID!",
            _p_task, _p_task->_task_id);
		return (REPORTER_ERR_GET_CID);
	}

	/* Gcid */
	if( ( TRUE==emule_get_gcid( p_emule_task->_data_manager, cmd._gcid)
        || TRUE==emule_get_calc_gcid( p_emule_task->_data_manager, cmd._gcid))
		&&(TRUE==sd_is_cid_valid(cmd._gcid)))
	{
		cmd._gcid_size = CID_SIZE;
	}
	else
	{
		LOG_DEBUG("reporter_emule_insert_res..._p_task=0x%X,task_id=%u:Error:REPORTER_ERR_GET_GCID!",_p_task,_p_task->_task_id);
		//return (REPORTER_ERR_GET_GCID);
	}

	cmd._gcid_level = 81; //emule 此值填81

	cmd._gcid_part_size = emule_get_block_size( p_emule_task->_data_manager);

    if(TRUE==emule_get_bcids( p_emule_task->_data_manager,&(cmd._bcid_size), &(cmd._bcid)))
	{
		cmd._bcid_size*= CID_SIZE;
	}
	else
	{
		LOG_DEBUG("reporter_emule_insert_res..._p_task=0x%X,task_id=%u:Error:REPORTER_ERR_GET_BCID!",_p_task,_p_task->_task_id);
		return (REPORTER_ERR_GET_BCID);
	}

	cmd._md5_size = 0;
	cmd._md5 = NULL;

#ifdef REP_DW_VER_54
	ret_val = get_partner_id(cmd._partner_id, MAX_PARTNER_ID_LEN); 
	CHECK_VALUE(ret_val);
	cmd._partner_id_len = sd_strlen(cmd._partner_id);
	cmd._product_flag = get_product_flag();
#endif /* REP_DW_VER_54 */

	/*build command*/
	ret_val = reporter_build_emule_report_insertsres_cmd(&buffer, &len, &cmd);
	CHECK_VALUE(ret_val);
	return reporter_commit_cmd(&g_emule_hub, REPORT_EMULE_INSERTSRES_CMD_TYPE, buffer, len, _p_task, cmd._header._seq);			//commit a request
}

_int32 reporter_emule_download_stat(TASK* _p_task)
{	
	char* buffer = NULL;
	_u32  len = 0;
	EMULE_TASK * p_emule_task = (EMULE_TASK *)_p_task;
	/*fill request package*/
	REPORT_EMULE_DL_STAT_CMD cmd;
	_int32 ret_val = SUCCESS;
	_u64 server_dl_bytes = 0;
	_u64 peer_dl_bytes = 0;
	_u64 emule_dl_bytes = 0;
	_u64 cdn_dl_bytes = 0;
	_u64 dl_bytes = 0;

	LOG_DEBUG("reporter_emule_download_stat..._p_task=0x%X,task_id=%u",_p_task,_p_task->_task_id);
	sd_memset(&cmd, 0, sizeof(REPORT_EMULE_DL_STAT_CMD));
	
	cmd._peerid_size = PEER_ID_SIZE;
	ret_val=get_peerid(cmd._peerid, PEER_ID_SIZE);
	if(ret_val!=SUCCESS)
	{
		LOG_DEBUG("Error when getting peerid!");
		return ret_val;
	}
  
	cmd._fileid_len = FILE_ID_SIZE;
    emule_get_file_id(p_emule_task->_data_manager, (_u8*)cmd._fileid );

	// 文件大小 
	cmd._file_size = _p_task->file_size;
        
	cmd._file_name_len = MAX_FILE_NAME_BUFFER_LEN;
    ret_val = emule_get_task_file_name( _p_task, cmd._file_name, (_int32*)&cmd._file_name_len );
    if( ret_val != SUCCESS )
    {
        cmd._file_name_len = 0;
    }
	
	/* part hash */
    emule_get_aich_hash(p_emule_task->_data_manager, &cmd._aich_hash_ptr, &cmd._aich_hash_len );
    
	/* part hash */
    emule_get_part_hash(p_emule_task->_data_manager, &cmd._part_hash_ptr, &cmd._part_hash_len );

	/* cid */
    if((TRUE==emule_get_cid( p_emule_task->_data_manager, cmd._cid))
        &&(TRUE==sd_is_cid_valid(cmd._cid)))
    {
		cmd._cid_size = CID_SIZE;
    }
	else
	{
		LOG_DEBUG("reporter_emule_insert_res..._p_task=0x%X,task_id=%u:Error:REPORTER_ERR_GET_CID!",_p_task,_p_task->_task_id);
		return (REPORTER_ERR_GET_CID);
	}

	/* Gcid */
	if( ( TRUE==emule_get_gcid( p_emule_task->_data_manager, cmd._gcid)
        || TRUE==emule_get_calc_gcid( p_emule_task->_data_manager, cmd._gcid))
		&&(TRUE==sd_is_cid_valid(cmd._gcid)))
	{
		cmd._gcid_size = CID_SIZE;
	}
	else
	{
		LOG_DEBUG("reporter_emule_insert_res..._p_task=0x%X,task_id=%u:Error:REPORTER_ERR_GET_GCID!",_p_task,_p_task->_task_id);
		//return (REPORTER_ERR_GET_GCID);
	}

	emule_get_dl_bytes(p_emule_task->_data_manager, &server_dl_bytes, &peer_dl_bytes, &emule_dl_bytes, &cdn_dl_bytes ,NULL);
	dl_bytes = server_dl_bytes + peer_dl_bytes + emule_dl_bytes + cdn_dl_bytes;
    cmd._emule_dl_bytes = emule_dl_bytes;
    cmd._server_dl_bytes = server_dl_bytes;
    cmd._peer_dl_bytes = peer_dl_bytes;


	/* 下载花费时间 ,平均下载速度*/
	if(_p_task->_finished_time>_p_task->_start_time)
	{
		cmd._duration =(_int32)( _p_task->_finished_time-_p_task->_start_time);
		cmd._avg_speed = (_u64)dl_bytes / cmd._duration;
	}
	else
	{
		cmd._duration=0;
		cmd._avg_speed = 0;
	}
    
	cmd._product_flag = get_product_flag();
	/* 版本号 */
	ret_val = get_version(cmd._thunder_version, MAX_VERSION_LEN-1); 
	CHECK_VALUE(ret_val);
	cmd._thunder_version_len = sd_strlen(cmd._thunder_version);

	ret_val = get_partner_id(cmd._partner_id, MAX_PARTNER_ID_LEN); 
	CHECK_VALUE(ret_val);
	cmd._partner_id_len = sd_strlen(cmd._partner_id);


    cmd._page_url_size = 0;
    cmd._statistic_page_url_size = 0;

    cmd._download_flag = 0;
    
    cmd._bt_dl_bytes = 0;
    
#ifdef REP_DW_VER_54
    cmd._dw_comefrom = 0;
#endif /* REP_DW_VER_54 */
    cmd._cdn_dl_bytes = 0;

    if(p_emule_task->_is_need_report)
    {
       cmd._is_speedup = 0;
    }
    else
    {
       cmd._is_speedup = 1;
    }
    cmd._band_width = 0;

	/*build command*/
	ret_val = reporter_build_report_emule_dl_cmd(&buffer, &len, &cmd);
	CHECK_VALUE(ret_val);
	return reporter_commit_cmd(&g_stat_hub, REPORT_EMULE_DL_STAT_CMD_TYPE, buffer, len, _p_task, cmd._header._seq);			//commit a request
}

_int32 reporter_emule_normal_cdn_stat(TASK *task, int stat_type)
{
	_int32 ret_val = SUCCESS;
	char *buffer = NULL;
	_u32  len = 0;
	EMULE_REPORT_NORMAL_CDN_STAT cmd;
	CM_REPORT_PARA *report_para = NULL;
	RES_QUERY_REPORT_PARA *query_para = NULL;
	
	EMULE_TASK *p_emule_task = (EMULE_TASK *)task;
	_u64 ser_bytes = 0, peer_bytes = 0, emule_dl_bytes = 0, cdn_bytes = 0, lixian_bytes = 0;
	
	LOG_DEBUG("task = 0x%x, task_id = %u", task, task->_task_id);

	sd_memset(&cmd, 0, sizeof(EMULE_REPORT_NORMAL_CDN_STAT));

	cmd._stat_type = stat_type;
	cmd._file_id_size = FILE_ID_SIZE;
    emule_get_file_id(p_emule_task->_data_manager, (_u8*)cmd._file_id );
	cmd._file_size = task->_file_size;

	if (emule_get_cid(&(p_emule_task->_data_manager), cmd._cid) != 0)
	{
		LOG_DEBUG("task = 0x%x, task_id = %u: Error:REPORTER_ERR_GET_CID!", task, task->_task_id);
		return REPORTER_ERR_GET_CID;
	}
	cmd._cid_size = CID_SIZE;

	if (emule_get_gcid(&(p_emule_task->_data_manager), cmd._gcid) != 0)
	{
		LOG_DEBUG("task = 0x%x, task_id = %u: Error:REPORTER_ERR_GET_GCID!", task, task->_task_id);
		return REPORTER_ERR_GET_GCID;
	}
	cmd._gcid_size = CID_SIZE;

	// userid 暂时不需要
	cmd._user_id_size = 0;

	cmd._peerid_size = PEER_ID_SIZE;
	ret_val = get_peerid((char *)&(cmd._peerid[0]), PEER_ID_SIZE );
	if(ret_val != SUCCESS)
	{
		LOG_DEBUG("Error when getting peerid!");
		return ret_val;
	}

	LOG_DEBUG("get_peerid = %s", cmd._peerid);

	report_para = cm_get_report_para(&(task->_connect_manager), INVALID_FILE_INDEX);
	if (!report_para) 
	{
		LOG_ERROR("can't get report parameter, so drop to report.");
		return REPORTER_ERR_BUILD_EMB_REPORT;
	}

	LOG_DEBUG("_res_server_total = %u, _res_peer_total = %u, _res_normal_cdn_total = %u.", 
		report_para->_res_server_total, report_para->_res_peer_total, report_para->_res_normal_cdn_total);

	cmd._normal_cdn_cnt = report_para->_res_normal_cdn_total;

	if ((report_para->_res_peer_total + report_para->_res_server_total) > report_para->_res_normal_cdn_total)
	{
		cmd._other_res_added = 1;
	}
	else
	{
		cmd._other_res_added = 0;
	}
	
	emule_get_dl_bytes(p_emule_task->_data_manager, &ser_bytes, &peer_bytes, &emule_dl_bytes, &cdn_bytes ,&lixian_bytes);
	cmd._total_down_bytes = ser_bytes + peer_bytes + cdn_bytes + lixian_bytes;
	emule_get_normal_cdn_down_bytes(&p_emule_task->_data_manager, &cmd._normal_cdn_down_bytes );
	cmd._normal_cdn_trans_time = report_para->_res_normal_cdn_sum_use_time;

	cmd._normal_cdn_conn_time = cm_get_normal_cdn_connect_time(&(task->_connect_manager));

	cmd._time_span_from_0_to_nonzero = task->_first_byte_arrive_time;

	cmd._time_sum_of_all_zero_period = task->_zero_speed_time_ms;

	if(task->_finished_time > task->_start_time)
		cmd._time_of_download = (_int32)(task->_finished_time - task->_start_time);
	else
		cmd._time_of_download = 1;

	query_para = dt_get_res_query_report_para(task);
	if (query_para)
	{
		cmd._query_normal_cdn_fail_times = query_para->_normal_cdn_fail;
	}

	_u32 mid_time = 0;
	sd_time(&mid_time);
	_u64 mid_rand = sd_rand();

	cmd._magic_id = (mid_rand << 32) | mid_time;

	cmd._random_id = mid_rand;

	/*build command*/
	ret_val = reporter_build_report_emule_normal_cdn_stat_cmd(&buffer, &len, &cmd);
	CHECK_VALUE(ret_val);

	return reporter_commit_cmd(&g_stat_hub, EMB_REPORT_EMULE_NORMAL_CDN_STAT_TYPE, buffer, len, task, 
		cmd._header._seq);
}
#endif

_int32 reporter_get_cid(DATA_MANAGER *data_manager, _u8 cid[CID_SIZE])
{
	_int32 ret = 0;
	if(dm_get_cid(data_manager, cid))
	{
		if (sd_is_cid_valid(cid) == FALSE) ret = -1;
	}
	else
	{
		ret = -1;
	}

	return ret;
}

_int32 reporter_get_gcid(DATA_MANAGER *data_manager, _u8 gcid[CID_SIZE])
{
	if (dm_get_calc_gcid(data_manager, gcid) == FALSE)
	{
		if (dm_get_shub_gcid(data_manager, gcid) == FALSE)
		{
			return -1;
		}
	}

	if (sd_is_cid_valid(gcid) == FALSE) return -2;

	return 0;	
}

// stat_type: 0, 完成上报；1, 暂停上报。
_int32 reporter_p2sp_normal_cdn_stat(TASK *task, int stat_type)
{
	_int32 ret_val = SUCCESS;
	char *buffer = NULL;
	_u32  len = 0;
	P2SP_REPORT_NORMAL_CDN_STAT cmd;
	CM_REPORT_PARA *report_para = NULL;
	RES_QUERY_REPORT_PARA *query_para = NULL;
	
	P2SP_TASK *p_p2sp_task = (P2SP_TASK *)task;
	LOG_DEBUG("task = 0x%x, task_id = %u", task, task->_task_id);

	sd_memset(&cmd, 0, sizeof(P2SP_REPORT_NORMAL_CDN_STAT));

	cmd._stat_type = stat_type;
	cmd._file_size = task->_file_size;

	if (reporter_get_cid(&(p_p2sp_task->_data_manager), cmd._cid) != 0)
	{
		LOG_DEBUG("task = 0x%x, task_id = %u: Error:REPORTER_ERR_GET_CID!", task, task->_task_id);
		return REPORTER_ERR_GET_CID;
	}
	cmd._cid_size = CID_SIZE;

	if (reporter_get_gcid(&(p_p2sp_task->_data_manager), cmd._gcid) != 0)
	{
		LOG_DEBUG("task = 0x%x, task_id = %u: Error:REPORTER_ERR_GET_GCID!", task, task->_task_id);
		return REPORTER_ERR_GET_GCID;
	}
	cmd._gcid_size = CID_SIZE;

	// userid 暂时不需要
	cmd._user_id_size = 0;

	cmd._peerid_size = PEER_ID_SIZE;
	ret_val = get_peerid((char *)&(cmd._peerid[0]), PEER_ID_SIZE );
	if(ret_val != SUCCESS)
	{
		LOG_DEBUG("Error when getting peerid!");
		return ret_val;
	}

	LOG_DEBUG("get_peerid = %s", cmd._peerid);

	report_para = cm_get_report_para(&(task->_connect_manager), INVALID_FILE_INDEX);
	if (!report_para) 
	{
		LOG_ERROR("can't get report parameter, so drop to report.");
		return REPORTER_ERR_BUILD_EMB_REPORT;
	}

	LOG_DEBUG("_res_server_total = %u, _res_peer_total = %u, _res_normal_cdn_total = %u.", 
		report_para->_res_server_total, report_para->_res_peer_total, report_para->_res_normal_cdn_total);

	cmd._normal_cdn_cnt = report_para->_res_normal_cdn_total;

	if ((report_para->_res_peer_total + report_para->_res_server_total) > report_para->_res_normal_cdn_total)
	{
		cmd._other_res_added = 1;
	}
	else
	{
		cmd._other_res_added = 0;
	}
	
	_u64 ser_bytes = 0, peer_bytes = 0, cdn_bytes = 0, lixian_bytes = 0;
	dm_get_dl_bytes(&(p_p2sp_task->_data_manager), &ser_bytes, &peer_bytes, &cdn_bytes, &lixian_bytes, NULL);
	cmd._total_down_bytes = ser_bytes + peer_bytes + cdn_bytes + lixian_bytes;

	cmd._normal_cdn_down_bytes = dm_get_normal_cdn_down_bytes(&(p_p2sp_task->_data_manager));

	cmd._normal_cdn_trans_time = report_para->_res_normal_cdn_sum_use_time;

	cmd._normal_cdn_conn_time = cm_get_normal_cdn_connect_time(&(task->_connect_manager));

	cmd._time_span_from_0_to_nonzero = task->_first_byte_arrive_time;

	cmd._time_sum_of_all_zero_period = task->_zero_speed_time_ms;

	if(task->_finished_time > task->_start_time)
		cmd._time_of_download = (_int32)(task->_finished_time - task->_start_time);
	else
		cmd._time_of_download = 1;

	query_para = dt_get_res_query_report_para(task);
	if (query_para)
	{
		cmd._query_normal_cdn_fail_times = query_para->_normal_cdn_fail;
	}

	cmd._task_magic_id = 0;

	if ((p_p2sp_task->_task_created_type == TASK_CREATED_BY_URL) ||
		(p_p2sp_task->_task_created_type==TASK_CONTINUE_BY_URL))
	{
		char *origin_url = NULL;
		if(p_p2sp_task->_origion_url_length > 0) origin_url = p_p2sp_task->_origion_url;
		
		if((NULL != origin_url) || TRUE == dm_get_origin_url(&(p_p2sp_task->_data_manager), &origin_url))
		{
			cmd._url_size = sd_strlen(origin_url);
			sd_strncpy((char *)&(cmd._url[0]), origin_url, cmd._url_size);
		}
		else
		{
			LOG_DEBUG("task = 0x%x, task_id = %u: Error:REPORTER_ERR_GET_URL!", task, task->_task_id);
			// 非致命错误，不需要返回
			cmd._url_size = 0;
		}
	}

	_u32 mid_time = 0;
	sd_time(&mid_time);
	_u64 mid_rand = sd_rand();

	cmd._magic_id = (mid_rand << 32) | mid_time;

	cmd._random_id = mid_rand;

	/*build command*/
	ret_val = reporter_build_report_p2sp_normal_cdn_stat_cmd(&buffer, &len, &cmd);
	CHECK_VALUE(ret_val);

	return reporter_commit_cmd(&g_stat_hub, EMB_REPORT_P2SP_NORMAL_CDN_STAT_TYPE, buffer, len, task, 
		cmd._header._seq);
}

#ifdef UPLOAD_ENABLE
/*--------------------------------------------------------------------------*/
/*                Interfaces provid for upload_manager				        */
/*--------------------------------------------------------------------------*/
/* isrc_online */
_int32 reporter_isrc_online(void)
{	
	char* buffer = NULL;
	_u32  len = 0;
	/*fill request package*/
	REPORT_RC_ONLINE_CMD cmd;
	_int32 ret_val = SUCCESS;
	LOG_DEBUG("reporter_isrc_online...");
	sd_memset(&cmd, 0, sizeof(REPORT_RC_ONLINE_CMD));

	cmd._peerid_size = PEER_ID_SIZE;
	ret_val=get_peerid(cmd._peerid, PEER_ID_SIZE + 1);
	if(ret_val!=SUCCESS)
	{
		LOG_DEBUG("Error when getting peerid!");
		return ret_val;
	}


	LOG_DEBUG("reporter_isrc_online:_peerid=%s",cmd._peerid);

	/*build command*/
	ret_val = reporter_build_isrc_online_cmd(&buffer, &len, &cmd);
	CHECK_VALUE(ret_val);
	return reporter_commit_cmd(&g_phub, REPORT_ISRC_ONLINE_CMD_TYPE, buffer, len, NULL, cmd._header._seq);			//commit a request
}
/* reportRClist */
_int32 reporter_rc_list(const LIST * rc_list,const _u32 p2p_capability)
{	
	char* buffer = NULL;
	_u32  len = 0;
	/*fill request package*/
	REPORT_RC_LIST_CMD cmd;
	LIST_NODE * _start_node=NULL;
	_int32 ret_val = SUCCESS,start_node_index=0;
	LOG_DEBUG("reporter_rc_list...");
	sd_memset(&cmd, 0, sizeof(REPORT_RC_LIST_CMD));

	cmd._peerid_size = PEER_ID_SIZE;
	ret_val=get_peerid(cmd._peerid, PEER_ID_SIZE + 1);
	if(ret_val!=SUCCESS)
	{
		LOG_DEBUG("Error when getting peerid!");
		return ret_val;
	}

	cmd._rc_list= (LIST *)rc_list;
	_start_node = LIST_BEGIN(*rc_list);
	cmd._p2p_capability= p2p_capability;

#ifdef REPORTER_DEBUG
	{
		LIST_NODE * p_list_node = NULL;
		_int32 l_index = 0;
		for(p_list_node =  LIST_BEGIN(*(cmd._rc_list));p_list_node !=  LIST_END(*(cmd._rc_list));p_list_node =  LIST_NEXT(p_list_node))
		{
			LOG_DEBUG("p_list_node[%d]=0x%X",l_index++,p_list_node);
		}

		LOG_DEBUG("END NODE:p_list_node[%d]=0x%X",l_index++,p_list_node);
	}
#endif /* _DEBUG */

	
	do
	{
		buffer = NULL;
		len = 0;		
		/*build command*/
		ret_val = reporter_build_rc_list_cmd(&buffer, &len, &cmd,&_start_node,&start_node_index);
		CHECK_VALUE(ret_val);
		ret_val = reporter_commit_cmd(&g_phub, REPORT_RC_LIST_CMD_TYPE, buffer, len, NULL, cmd._header._seq);			//commit a request
		CHECK_VALUE(ret_val);
	}while((_start_node!=LIST_END(*rc_list))||(start_node_index!=-1));
	
	return SUCCESS;
}

/* insertRC */
_int32 reporter_insert_rc(const _u64 file_size,const _u8* tcid,const _u8* gcid,const _u32 p2p_capability)
{	
	char* buffer = NULL;
	_u32  len = 0;
	/*fill request package*/
	REPORT_INSERT_RC_CMD cmd;
	_int32 ret_val = SUCCESS;
	LOG_DEBUG("reporter_insert_rc...");
	sd_memset(&cmd, 0, sizeof(REPORT_INSERT_RC_CMD));

	cmd._peerid_size = PEER_ID_SIZE;
	ret_val=get_peerid(cmd._peerid, PEER_ID_SIZE + 1);
	if(ret_val!=SUCCESS)
	{
		LOG_DEBUG("Error when getting peerid!");
		return ret_val;
	}

	cmd._cid_size= CID_SIZE;
	sd_memcpy(cmd._cid,tcid,CID_SIZE);
	
	cmd._file_size= file_size;
	
	cmd._gcid_size= CID_SIZE;
	sd_memcpy(cmd._gcid,gcid,CID_SIZE);
	
	cmd._p2p_capability= p2p_capability;


	//LOG_DEBUG("reporter_rc_list:_peerid=%s,partner_id=%s,product_flag=%s,license=%s,ip=%s,os=%s,",cmd._peerid,cmd.partner_id,cmd.product_flag,cmd.license,cmd.ip,cmd.os);

	/*build command*/
	ret_val = reporter_build_insert_rc_cmd(&buffer, &len, &cmd);
	CHECK_VALUE(ret_val);
	return reporter_commit_cmd(&g_phub, REPORT_INSERT_RC_CMD_TYPE, buffer, len, NULL, cmd._header._seq);			//commit a request
}

/* deleteRC */
_int32 reporter_delete_rc(const _u64 file_size,const _u8* tcid,const _u8* gcid)
{	
	char* buffer = NULL;
	_u32  len = 0;
	/*fill request package*/
	REPORT_DELETE_RC_CMD cmd;
	_int32 ret_val = SUCCESS;
	LOG_DEBUG("reporter_delete_rc...");
	sd_memset(&cmd, 0, sizeof(REPORT_DELETE_RC_CMD));

	cmd._peerid_size = PEER_ID_SIZE;
	ret_val=get_peerid(cmd._peerid, PEER_ID_SIZE + 1);
	if(ret_val!=SUCCESS)
	{
		LOG_DEBUG("Error when getting peerid!");
		return ret_val;
	}

	cmd._cid_size= CID_SIZE;
	sd_memcpy(cmd._cid,tcid,CID_SIZE);
	
	cmd._file_size= file_size;
	
	cmd._gcid_size= CID_SIZE;
	sd_memcpy(cmd._gcid,gcid,CID_SIZE);
	

	//LOG_DEBUG("reporter_rc_list:_peerid=%s,partner_id=%s,product_flag=%s,license=%s,ip=%s,os=%s,",cmd._peerid,cmd.partner_id,cmd.product_flag,cmd.license,cmd.ip,cmd.os);

	/*build command*/
	ret_val = reporter_build_delete_rc_cmd(&buffer, &len, &cmd);
	CHECK_VALUE(ret_val);
	return reporter_commit_cmd(&g_phub, REPORT_DELETE_RC_CMD_TYPE, buffer, len, NULL, cmd._header._seq);			//commit a request
}
#endif

#ifdef EMBED_REPORT
//COMMONTASKDLSTAT	 
_int32 emb_reporter_common_task_download_stat(TASK* _p_task)
{
	char *buffer = NULL;
	_u32  len = 0;
	/*fill request package*/
	EMB_REPORT_COMMON_TASK_DL_STAT_CMD cmd;
	_int32 ret_val = SUCCESS;
	P2SP_TASK * p_p2sp_task = (P2SP_TASK *)_p_task;
	char *origin_url=NULL, *file_name = NULL;
	
	_u64 server_dl_bytes = 0;
	_u64 peer_dl_bytes = 0;
	_u64 cdn_dl_bytes = 0;
	_u64 dl_bytes = 0;


	LOG_DEBUG("emb_reporter_common_task_download_stat..._p_task=0x%X,task_id=%u ",_p_task,_p_task->_task_id );

	if( _p_task->_task_type != P2SP_TASK_TYPE )
	{
		CHECK_VALUE( REPORTER_ERR_TASK_TYPE );
	}
	
	sd_memset(&cmd, 0, sizeof(EMB_REPORT_COMMON_TASK_DL_STAT_CMD));

	/* 版本号 */
	ret_val = get_version(cmd._thunder_version, MAX_VERSION_LEN-1); 
	CHECK_VALUE(ret_val);
	cmd._thunder_version_len = sd_strlen(cmd._thunder_version);

	/* 合作伙伴ID */
	ret_val = get_partner_id(cmd._partner_id, MAX_PARTNER_ID_LEN-1); 
	CHECK_VALUE(ret_val);
	cmd._partner_id_len = sd_strlen(cmd._partner_id);

	/* Peer ID */
	cmd._peerid_len = PEER_ID_SIZE;
	ret_val=get_peerid(cmd._peerid, PEER_ID_SIZE);
	if(ret_val!=SUCCESS)
	{
		LOG_DEBUG("Error when getting peerid!");
		return ret_val;
	}

	/* 任务创建类型:*/
	cmd._task_create_type = p_p2sp_task->_task_created_type;
	if( p_p2sp_task->_is_vod_task )
	{
		if( p_p2sp_task->_task_created_type == TASK_CREATED_BY_TCID )
		{
			cmd._task_create_type = NEW_TASK_CREATED_BY_KANKAN_PRIVATE;
		}
		else if( p_p2sp_task->_task_created_type == TASK_CONTINUE_BY_TCID )
		{
			cmd._task_create_type = CONTINUED_TASK_CREATED_BY_KANKAN_PRIVATE;
		}
	}

    if( p_p2sp_task->_task_created_type == TASK_CREATED_BY_URL
		|| p_p2sp_task->_task_created_type == TASK_CONTINUE_BY_URL )
    {
		/* 客户端下载的URL:*/
		origin_url = NULL;
		if(p_p2sp_task->_origion_url_length > 0)origin_url = p_p2sp_task->_origion_url;
		
		if(NULL != origin_url ||  TRUE ==  dm_get_origin_url(&(p_p2sp_task->_data_manager), &origin_url))
		{
			/* remove user name and password and decode */
			url_object_to_noauth_string(origin_url, cmd._url,MAX_URL_LEN);
			cmd._url_len = sd_strlen(cmd._url);
		}

		/* refererUrl */
		origin_url=NULL;
		if(TRUE ==  dm_get_origin_ref_url(&(p_p2sp_task->_data_manager), &origin_url))
		{
			sd_strncpy(cmd._ref_url, origin_url,MAX_URL_LEN);
			cmd._ref_url_len = sd_strlen(cmd._ref_url);
		}
    }

	
	/* 三段CID */
	if((TRUE==dm_get_cid( &(p_p2sp_task->_data_manager), cmd._cid))
		&&(TRUE==sd_is_cid_valid(cmd._cid)))
	{
		cmd._cid_len = CID_SIZE;
	}
	else
	{
		LOG_DEBUG("emb_reporter_common_task_download_stat _p_task=0x%X,task_id=%u:Error:REPORTER_ERR_GET_CID!",_p_task,_p_task->_task_id);
        return (REPORTER_ERR_GET_CID);
	}

	/* 全文CID */
	if((TRUE==dm_get_calc_gcid( &(p_p2sp_task->_data_manager), cmd._gcid))
		&&(TRUE==sd_is_cid_valid(cmd._gcid)))
	{
		cmd._gcid_len = CID_SIZE;
	}else if((TRUE==dm_get_shub_gcid( &(p_p2sp_task->_data_manager), cmd._gcid))
		&&(TRUE==sd_is_cid_valid(cmd._gcid)))
	{
		cmd._gcid_len = CID_SIZE;
	}
	else
	{
		LOG_DEBUG("emb_reporter_common_task_download_stat _p_task=0x%X,task_id=%u:Error:REPORTER_ERR_GET_GCID!",_p_task,_p_task->_task_id);
        return (REPORTER_ERR_GET_GCID);
	}

	/* 文件大小 */
	cmd._file_size = _p_task->file_size;

	/* 块大小 */
	cmd._block_size = dm_get_block_size( &(p_p2sp_task->_data_manager));

	/* 文件名称 */
	if(TRUE==dm_get_filnal_file_name( &(p_p2sp_task->_data_manager),&file_name))
	{
		sd_strncpy(cmd._file_name, file_name,MAX_FILE_NAME_LEN);
		cmd._file_name_len = sd_strlen(cmd._file_name);
	}
	else
	{
		LOG_DEBUG("emb_reporter_common_task_download_stat..._p_task=0x%X,task_id=%u:Error:REPORTER_ERR_GET_FILE_NAME!",_p_task,_p_task->_task_id);
		return (REPORTER_ERR_GET_FILE_NAME);
	}
	
	dm_get_dl_bytes(&(p_p2sp_task->_data_manager), &server_dl_bytes, &peer_dl_bytes, &cdn_dl_bytes,NULL, NULL );
	dl_bytes = server_dl_bytes + peer_dl_bytes + cdn_dl_bytes;
	
	/* 下载花费时间 ,平均下载速度*/
	if(_p_task->_finished_time>_p_task->_start_time)
	{
		cmd._duration =(_int32)( _p_task->_finished_time-_p_task->_start_time);
		cmd._avg_speed = (_u64)dl_bytes / cmd._duration;
	}
	else
	{
		cmd._duration=0;
		cmd._avg_speed = 0;
	}
	/* 下载状态 */
	cmd._download_stat = 0; //未完成

	/*build command*/
	ret_val = emb_reporter_build_common_task_download_stat_cmd(&buffer, &len, &cmd);
	CHECK_VALUE(ret_val);
	return reporter_commit_cmd(&g_embed_hub, EMB_REPORT_COMMON_TASK_DL_STAT_CMD_TYPE, buffer, len, _p_task, cmd._header._seq);			//commit a request
}

#ifdef ENABLE_BT

//BTTASKDLSTAT	 
_int32 emb_reporter_bt_task_download_stat(TASK* _p_task)
{
	char *buffer = NULL;
	_u32  len = 0;
	/*fill request package*/
	EMB_REPORT_BT_TASK_DL_STAT_CMD cmd;
	_int32 ret_val = SUCCESS;
	BT_TASK * p_bt_task = (BT_TASK *)_p_task;
	_u8 *info_hash=NULL;
	char * p_str=NULL;
	_u64 server_dl_bytes = 0;
	_u64 peer_dl_bytes = 0;
	_u64 cdn_dl_bytes = 0;
	_u64 origion_dl_bytes = 0;
	_u64 dl_bytes = 0;
	
	LOG_DEBUG("emb_reporter_bt_task_download_stat..._p_task=0x%X,task_id=%u",_p_task,_p_task->_task_id);
	sd_memset(&cmd, 0, sizeof(EMB_REPORT_BT_TASK_DL_STAT_CMD));

	if( _p_task->_task_type != BT_TASK_TYPE )
	{
		CHECK_VALUE( REPORTER_ERR_TASK_TYPE );
	}


	/* 版本号 */
	ret_val = get_version(cmd._thunder_version, MAX_VERSION_LEN-1); 
	CHECK_VALUE(ret_val);
	cmd._thunder_version_len = sd_strlen(cmd._thunder_version);

	/* 合作伙伴ID */
	ret_val = get_partner_id(cmd._partner_id, MAX_PARTNER_ID_LEN-1); 
	CHECK_VALUE(ret_val);
	cmd._partner_id_len = sd_strlen(cmd._partner_id);

	/* Peer ID */
	cmd._peerid_len = PEER_ID_SIZE;
	ret_val=get_peerid(cmd._peerid, PEER_ID_SIZE);
	if(ret_val!=SUCCESS)
	{
		LOG_DEBUG("Error when getting peerid!");
		return ret_val;
	}

	/* info_hash */
	ret_val = tp_get_file_info_hash(  p_bt_task->_torrent_parser_ptr, &info_hash );
	CHECK_VALUE(ret_val);
	sd_memcpy(cmd._info_hash, info_hash,CID_SIZE);
	cmd._info_hash_len = CID_SIZE;

	/* title_name */
	ret_val = tp_get_seed_title_name(  p_bt_task->_torrent_parser_ptr, &p_str );
	CHECK_VALUE(ret_val);
	sd_strncpy(cmd._file_title, p_str,MAX_FILE_NAME_BUFFER_LEN);
	cmd._file_title_len = sd_strlen(cmd._file_title);


	cmd._file_num = tp_get_seed_file_num(  p_bt_task->_torrent_parser_ptr );
	
	cmd._file_total_size = tp_get_file_total_size( p_bt_task->_torrent_parser_ptr );

	cmd._piece_size = tp_get_piece_size( p_bt_task->_torrent_parser_ptr );

	bdm_get_dl_bytes(&(p_bt_task->_data_manager), &server_dl_bytes, &peer_dl_bytes, &cdn_dl_bytes, &origion_dl_bytes );
	dl_bytes = server_dl_bytes + peer_dl_bytes + cdn_dl_bytes+ origion_dl_bytes;
	

	LOG_DEBUG("emb_reporter_bt_task_download_stat...server_dl_bytes=%llu, peer_dl_bytes=%llu,cdn_dl_bytes=%llu,origion_dl_bytes=%llu, dl_bytes=%llu",
		server_dl_bytes, peer_dl_bytes, cdn_dl_bytes, origion_dl_bytes, dl_bytes);
		
	if(_p_task->_finished_time>_p_task->_start_time)
	{
		cmd._duration =(_int32)( _p_task->_finished_time-_p_task->_start_time);
		cmd._avg_speed= (_u64)dl_bytes / cmd._duration;
	}
	else
	{
		cmd._duration=0;
		cmd._avg_speed=0;
	}
		

	/*build command*/
	ret_val = emb_reporter_build_bt_task_download_stat_cmd(&buffer, &len, &cmd);
	CHECK_VALUE(ret_val);
	return reporter_commit_cmd(&g_embed_hub, EMB_REPORT_BT_TASK_DL_STAT_CMD_TYPE, buffer, len, _p_task, cmd._header._seq);			//commit a request
}



//BTFILEDLSTAT	 
_int32 emb_reporter_bt_file_download_stat(TASK* _p_task, _u32 file_index)
{
	char *buffer = NULL;
	_u32  len = 0;
	/*fill request package*/
	EMB_REPORT_BT_FILE_DL_STAT_CMD cmd;
	_int32 ret_val = SUCCESS;
	BT_TASK * p_bt_task = (BT_TASK *)_p_task;
	_u8 *info_hash=NULL;
	char * p_str=NULL;
	TORRENT_FILE_INFO *p_file_info=NULL;
	_u64 server_dl_bytes = 0;
	_u64 peer_dl_bytes = 0;
	_u64 cdn_dl_bytes = 0;
	_u64 origion_dl_bytes = 0;
	_u64 dl_bytes = 0;
	
	LOG_DEBUG("emb_reporter_bt_file_download_stat..._p_task=0x%X,task_id=%u,file_index=%u",_p_task,_p_task->_task_id,file_index);
	sd_memset(&cmd, 0, sizeof(EMB_REPORT_BT_FILE_DL_STAT_CMD));
	
	if( _p_task->_task_type != BT_TASK_TYPE )
	{
		CHECK_VALUE( REPORTER_ERR_TASK_TYPE );
	}

	/* 版本号 */
	ret_val = get_version(cmd._thunder_version, MAX_VERSION_LEN-1); 
	CHECK_VALUE(ret_val);
	cmd._thunder_version_len = sd_strlen(cmd._thunder_version);

	/* 合作伙伴ID */
	ret_val = get_partner_id(cmd._partner_id, MAX_PARTNER_ID_LEN-1); 
	CHECK_VALUE(ret_val);
	cmd._partner_id_len = sd_strlen(cmd._partner_id);

	/* Peer ID */
	cmd._peerid_len = PEER_ID_SIZE;
	ret_val=get_peerid(cmd._peerid, PEER_ID_SIZE);
	if(ret_val!=SUCCESS)
	{
		LOG_DEBUG("Error when getting peerid!");
		return ret_val;
	}

	/* info_hash */
	ret_val = tp_get_file_info_hash(  p_bt_task->_torrent_parser_ptr, &info_hash );
	CHECK_VALUE(ret_val);
	sd_memcpy(cmd._info_hash, info_hash,CID_SIZE);
	cmd._info_hash_len = CID_SIZE;

	/* title_name */
	ret_val = tp_get_seed_title_name(  p_bt_task->_torrent_parser_ptr, &p_str );
	CHECK_VALUE(ret_val);
	sd_strncpy(cmd._file_title, p_str,MAX_FILE_NAME_BUFFER_LEN);
	cmd._file_title_len = sd_strlen(cmd._file_title);


	cmd._file_num = tp_get_seed_file_num(  p_bt_task->_torrent_parser_ptr );
	
	cmd._file_total_size = tp_get_file_total_size( p_bt_task->_torrent_parser_ptr );

	cmd._piece_size = tp_get_piece_size( p_bt_task->_torrent_parser_ptr );

	cmd._file_index= file_index;
	
	ret_val = tp_get_file_info( p_bt_task->_torrent_parser_ptr,  file_index, &p_file_info );
	CHECK_VALUE(ret_val);
	
	cmd._file_offset= p_file_info->_file_offset;

	if((TRUE==bdm_get_cid( &(p_bt_task->_data_manager), file_index,cmd._cid))
		&&(TRUE==sd_is_cid_valid(cmd._cid)))
	{
			cmd._cid_len = CID_SIZE;
	}
	else
	{
		LOG_DEBUG("emb_reporter_bt_file_download_stat..._p_task=0x%X,task_id=%u:Error:REPORTER_ERR_GET_CID!",_p_task,_p_task->_task_id);
		return (REPORTER_ERR_GET_CID);
	}

	if((TRUE==bdm_get_calc_gcid( &(p_bt_task->_data_manager),  file_index,cmd._gcid))
		&&(TRUE==sd_is_cid_valid(cmd._gcid)))
	{
		cmd._gcid_len = CID_SIZE;
	}else
	if((TRUE==bdm_get_shub_gcid( &(p_bt_task->_data_manager), file_index, cmd._gcid))
		&&(TRUE==sd_is_cid_valid(cmd._gcid)))
	{
		cmd._gcid_len = CID_SIZE;
	}
	else
	{
		LOG_DEBUG("emb_reporter_bt_file_download_stat..._p_task=0x%X,task_id=%u:Error:REPORTER_ERR_GET_GCID!",_p_task,_p_task->_task_id);
		return (REPORTER_ERR_GET_GCID);
	}



	cmd._file_size = p_file_info->_file_size;


	cmd._file_name_len = p_file_info->_file_name_len;
	sd_strncpy(cmd._file_name, p_file_info->_file_name, MAX_FILE_NAME_BUFFER_LEN);

	bdm_get_sub_file_dl_bytes(&(p_bt_task->_data_manager), file_index, &server_dl_bytes, &peer_dl_bytes, &cdn_dl_bytes ,NULL, &origion_dl_bytes);
	dl_bytes = server_dl_bytes + peer_dl_bytes + cdn_dl_bytes + origion_dl_bytes;

	LOG_DEBUG("emb_reporter_bt_file_download_stat...server_dl_bytes=%llu, peer_dl_bytes=%llu,cdn_dl_bytes=%llu,origion_dl_bytes=%llu, dl_bytes=%llu",
		server_dl_bytes, peer_dl_bytes, cdn_dl_bytes, origion_dl_bytes, dl_bytes);

	cmd._duration=bdm_get_file_download_time( &(p_bt_task->_data_manager), file_index );
	if(cmd._duration==0)
	{
		cmd._avg_speed = 0;
	}
	else
	{
		cmd._avg_speed=(_u64)dl_bytes/cmd._duration;
	}

	cmd._download_stat = 0;

	/*build command*/
	ret_val = emb_reporter_build_bt_file_download_stat_cmd(&buffer, &len, &cmd);
	CHECK_VALUE(ret_val);
	return reporter_commit_cmd(&g_embed_hub, EMB_REPORT_BT_FILE_DL_STAT_CMD_TYPE, buffer, len, _p_task, cmd._header._seq);			//commit a request
	
}
#endif

//COMMONTASKDLFAIL	 
_int32 emb_reporter_common_task_download_fail(TASK* _p_task)
{
	
	char *buffer = NULL;
	_u32  len = 0;
	
	_u32  ip = 0;
	/*fill request package*/
	EMB_REPORT_COMMON_TASK_DL_FAIL_CMD cmd;
	_int32 ret_val = SUCCESS;
	P2SP_TASK * p_p2sp_task = (P2SP_TASK *)_p_task;
	char *origin_url=NULL, *file_name = NULL;
	_u64 server_dl_bytes = 0;
	_u64 peer_dl_bytes = 0;
	_u64 cdn_dl_bytes = 0;
	_u64 dl_bytes = 0;


	LOG_DEBUG("emb_reporter_common_task_download_fail..._p_task=0x%X,task_id=%u ",_p_task,_p_task->_task_id );

	if( _p_task->_task_type != P2SP_TASK_TYPE )
	{
		CHECK_VALUE( REPORTER_ERR_TASK_TYPE );
	}
	
	sd_memset(&cmd, 0, sizeof(EMB_REPORT_COMMON_TASK_DL_FAIL_CMD));

	/* 版本号 */
	ret_val = get_version(cmd._thunder_version, MAX_VERSION_LEN-1); 
	CHECK_VALUE(ret_val);
	cmd._thunder_version_len = sd_strlen(cmd._thunder_version);

	/* 合作伙伴ID */
	ret_val = get_partner_id(cmd._partner_id, MAX_PARTNER_ID_LEN-1); 
	CHECK_VALUE(ret_val);
	cmd._partner_id_len = sd_strlen(cmd._partner_id);

	/* Peer ID */
	cmd._peerid_len = PEER_ID_SIZE;
	ret_val=get_peerid(cmd._peerid, PEER_ID_SIZE);
	if(ret_val!=SUCCESS)
	{
		LOG_DEBUG("Error when getting peerid!");
		return ret_val;
	}

	/* 任务创建类型:*/
	cmd._task_create_type = p_p2sp_task->_task_created_type;
	if( p_p2sp_task->_is_vod_task )
	{
		if( p_p2sp_task->_task_created_type == TASK_CREATED_BY_TCID )
		{
			cmd._task_create_type = NEW_TASK_CREATED_BY_KANKAN_PRIVATE;
		}
		else if( p_p2sp_task->_task_created_type == TASK_CONTINUE_BY_TCID )
		{
			cmd._task_create_type = CONTINUED_TASK_CREATED_BY_KANKAN_PRIVATE;
		}
	}
	
    if( p_p2sp_task->_task_created_type == TASK_CREATED_BY_URL
		|| p_p2sp_task->_task_created_type == TASK_CONTINUE_BY_URL )
    {
		/* 客户端下载的URL:*/
		origin_url = NULL;
		if(p_p2sp_task->_origion_url_length > 0)origin_url = p_p2sp_task->_origion_url;
		
		if(NULL != origin_url ||  TRUE ==  dm_get_origin_url(&(p_p2sp_task->_data_manager), &origin_url))
		{
			/* remove user name and password and decode */
			url_object_to_noauth_string(origin_url, cmd._url,MAX_URL_LEN);
			cmd._url_len = sd_strlen(cmd._url);
		}

		/* refererUrl */
		origin_url=NULL;
		if(TRUE ==  dm_get_origin_ref_url(&(p_p2sp_task->_data_manager), &origin_url))
		{
			sd_strncpy(cmd._ref_url, origin_url,MAX_URL_LEN);
			cmd._ref_url_len = sd_strlen(cmd._ref_url);
		}
    }
	
	/* 三段CID */
	if((TRUE==dm_get_cid( &(p_p2sp_task->_data_manager), cmd._cid))
		&&(TRUE==sd_is_cid_valid(cmd._cid)))
	{
		cmd._cid_len = CID_SIZE;
	}

	/* 全文CID */
	/*
	if((TRUE==dm_get_calc_gcid( &(p_p2sp_task->_data_manager), cmd._gcid))
		&&(TRUE==sd_is_cid_valid(cmd._gcid)))
	{
		cmd._gcid_len = CID_SIZE;
	}else*/
	if((TRUE==dm_get_shub_gcid( &(p_p2sp_task->_data_manager), cmd._gcid))
		&&(TRUE==sd_is_cid_valid(cmd._gcid)))
	{
		cmd._gcid_len = CID_SIZE;
	}


	/* 文件大小 */
	cmd._file_size = _p_task->file_size;

	/* 块大小 */
	cmd._block_size = dm_get_block_size( &(p_p2sp_task->_data_manager));

	/* 文件名称 */
	if(TRUE==dm_get_filnal_file_name( &(p_p2sp_task->_data_manager),&file_name))
	{
		sd_strncpy(cmd._file_name, file_name,MAX_FILE_NAME_LEN);
		cmd._file_name_len = sd_strlen(cmd._file_name);
	}

	cmd._fail_code = _p_task->failed_code;
	cmd._percent = _p_task->progress;

	
	ip = sd_get_local_ip(); 
	sd_snprintf(cmd._local_ip, MAX_HOST_NAME_LEN,"%d.%d.%d.%d", (ip)&0xFF, (ip>>8)&0xFF, (ip>>16)&0xFF, (ip>>24)&0xFF);
	cmd._ip_len = sd_strlen(cmd._local_ip);
	
	dm_get_dl_bytes(&(p_p2sp_task->_data_manager), &server_dl_bytes, &peer_dl_bytes, &cdn_dl_bytes,NULL, NULL );
	dl_bytes = server_dl_bytes + peer_dl_bytes + cdn_dl_bytes;

	/* 下载花费时间 ,平均下载速度*/
	if(_p_task->_finished_time>_p_task->_start_time)
	{
		cmd._duration =(_int32)( _p_task->_finished_time-_p_task->_start_time);
		cmd._avg_speed = (_u64)dl_bytes / cmd._duration;
	}
	else
	{
		cmd._duration=0;
		cmd._avg_speed = 0;
	}
	/* 下载状态 */
	cmd._download_stat = 0; //未完成

	/*build command*/
	ret_val = emb_reporter_build_common_task_download_fail_cmd(&buffer, &len, &cmd);
	CHECK_VALUE(ret_val);
	return reporter_commit_cmd(&g_embed_hub, EMB_REPORT_COMMON_TASK_DL_FAIL_CMD_TYPE, buffer, len, _p_task, cmd._header._seq);			//commit a request
}

#ifdef ENABLE_BT

//BTFILEDLFAIL	 
_int32 emb_reporter_bt_file_download_fail(TASK* _p_task, _u32 file_index)
{
	char *buffer = NULL;
	_u32  len = 0;
	/*fill request package*/
	EMB_REPORT_BT_FILE_DL_FAIL_CMD cmd;
	_int32 ret_val = SUCCESS;
	BT_TASK * p_bt_task = (BT_TASK *)_p_task;
	_u8 *info_hash=NULL;
	char * p_str=NULL;
	TORRENT_FILE_INFO *p_file_info=NULL;
	_u32  ip = 0;
	_u64 server_dl_bytes = 0;
	_u64 peer_dl_bytes = 0;
	_u64 cdn_dl_bytes = 0;
	_u64 origion_dl_bytes = 0;
	_u64 dl_bytes = 0;
	
	LOG_DEBUG("emb_reporter_bt_file_download_fail..._p_task=0x%X,task_id=%u,file_index=%u",_p_task,_p_task->_task_id,file_index);
	sd_memset(&cmd, 0, sizeof(EMB_REPORT_BT_FILE_DL_FAIL_CMD));
	
	if( _p_task->_task_type != BT_TASK_TYPE )
	{
		CHECK_VALUE( REPORTER_ERR_TASK_TYPE );
	}

	/* 版本号 */
	ret_val = get_version(cmd._thunder_version, MAX_VERSION_LEN-1); 
	CHECK_VALUE(ret_val);
	cmd._thunder_version_len = sd_strlen(cmd._thunder_version);

	/* 合作伙伴ID */
	ret_val = get_partner_id(cmd._partner_id, MAX_PARTNER_ID_LEN-1); 
	CHECK_VALUE(ret_val);
	cmd._partner_id_len = sd_strlen(cmd._partner_id);

	/* Peer ID */
	cmd._peerid_len = PEER_ID_SIZE;
	ret_val=get_peerid(cmd._peerid, PEER_ID_SIZE);
	if(ret_val!=SUCCESS)
	{
		LOG_DEBUG("Error when getting peerid!");
		return ret_val;
	}

	/* info_hash */
	ret_val = tp_get_file_info_hash(  p_bt_task->_torrent_parser_ptr, &info_hash );
	CHECK_VALUE(ret_val);
	sd_memcpy(cmd._info_hash, info_hash,CID_SIZE);
	cmd._info_hash_len = CID_SIZE;

	/* title_name */
	ret_val = tp_get_seed_title_name(  p_bt_task->_torrent_parser_ptr, &p_str );
	CHECK_VALUE(ret_val);
	sd_strncpy(cmd._file_title, p_str,MAX_FILE_NAME_BUFFER_LEN);
	cmd._file_title_len = sd_strlen(cmd._file_title);


	cmd._file_num = tp_get_seed_file_num(  p_bt_task->_torrent_parser_ptr );
	
	cmd._file_total_size = tp_get_file_total_size( p_bt_task->_torrent_parser_ptr );

	cmd._piece_size = tp_get_piece_size( p_bt_task->_torrent_parser_ptr );

	cmd._file_index= file_index;
	
	ret_val = tp_get_file_info( p_bt_task->_torrent_parser_ptr,  file_index, &p_file_info );
	CHECK_VALUE(ret_val);
	
	cmd._file_offset= p_file_info->_file_offset;

	if((TRUE==bdm_get_cid( &(p_bt_task->_data_manager), file_index,cmd._cid))
		&&(TRUE==sd_is_cid_valid(cmd._cid)))
	{
			cmd._cid_len = CID_SIZE;
	}
	
	/*
	if((TRUE==bdm_get_calc_gcid( &(p_bt_task->_data_manager),  file_index,cmd._gcid))
		&&(TRUE==sd_is_cid_valid(cmd._gcid)))
	{
		cmd._gcid_len = CID_SIZE;
	}else*/
	if((TRUE==bdm_get_shub_gcid( &(p_bt_task->_data_manager), file_index, cmd._gcid))
		&&(TRUE==sd_is_cid_valid(cmd._gcid)))
	{
		cmd._gcid_len = CID_SIZE;
	}


	cmd._file_size = p_file_info->_file_size;

	cmd._file_name_len = p_file_info->_file_name_len;
	sd_strncpy(cmd._file_name, p_file_info->_file_name, MAX_FILE_NAME_BUFFER_LEN);


	cmd._fail_code = bdm_get_file_err_code(&(p_bt_task->_data_manager), file_index);
	cmd._percent = bdm_get_sub_file_percent(&(p_bt_task->_data_manager), file_index);
	
	ip = sd_get_local_ip(); 
	sd_snprintf(cmd._local_ip, MAX_HOST_NAME_LEN,"%d.%d.%d.%d", (ip)&0xFF, (ip>>8)&0xFF, (ip>>16)&0xFF, (ip>>24)&0xFF);
	cmd._ip_len = sd_strlen(cmd._local_ip);


	bdm_get_sub_file_dl_bytes(&(p_bt_task->_data_manager), file_index, &server_dl_bytes, &peer_dl_bytes, &cdn_dl_bytes ,NULL, &origion_dl_bytes);
	dl_bytes = server_dl_bytes + peer_dl_bytes + cdn_dl_bytes+ origion_dl_bytes;
	
	LOG_DEBUG("emb_reporter_bt_file_download_fail...server_dl_bytes=%llu, peer_dl_bytes=%llu,cdn_dl_bytes=%llu,origion_dl_bytes=%llu, dl_bytes=%llu",
		server_dl_bytes, peer_dl_bytes, cdn_dl_bytes, origion_dl_bytes, dl_bytes);
		
	cmd._duration=bdm_get_file_download_time( &(p_bt_task->_data_manager), file_index );
	if(cmd._duration==0)
	{
		cmd._avg_speed=0;
	}
	else
	{
		cmd._avg_speed= (_u64)dl_bytes /cmd._duration;
	}

	cmd._download_stat = 0;

	/*build command*/
	ret_val = emb_reporter_build_bt_file_download_fail_cmd(&buffer, &len, &cmd);
	CHECK_VALUE(ret_val);
	return reporter_commit_cmd(&g_embed_hub, EMB_REPORT_BT_FILE_DL_FAIL_CMD_TYPE, buffer, len, _p_task, cmd._header._seq);			//commit a request
	
}
#endif

//ONLINEPEERSTAT	 
_int32 emb_reporter_dns_abnormal( _u32 err_code, LIST *p_dns_ip_list, 
	char *p_hub_domain, LIST *p_parse_ip_list )
{
	char *buffer = NULL;
	_u32  len = 0;
	/*fill request package*/
	EMB_DNS_ABNORMAL_CMD cmd;
	_int32 ret_val = SUCCESS;
	
	LOG_DEBUG("emb_reporter_dns_abnormal..err_code:%d, p_hub_domain=%s", err_code, p_hub_domain);
	sd_memset(&cmd, 0, sizeof(EMB_DNS_ABNORMAL_CMD));
	

	/* 版本号 */
	ret_val = get_version(cmd._thunder_version, MAX_VERSION_LEN-1); 
	CHECK_VALUE(ret_val);
	cmd._thunder_version_len = sd_strlen(cmd._thunder_version);

	/* 合作伙伴ID */
	ret_val = get_partner_id(cmd._partner_id, MAX_PARTNER_ID_LEN-1); 
	CHECK_VALUE(ret_val);
	cmd._partner_id_len = sd_strlen(cmd._partner_id);

	/* Peer ID */
	cmd._peerid_len = PEER_ID_SIZE;
	ret_val=get_peerid(cmd._peerid, PEER_ID_SIZE);
	if(ret_val!=SUCCESS)
	{
		LOG_DEBUG("Error when getting peerid!");
		return ret_val;
	}

	cmd._err_code = err_code;

	cmd._dns_ip_list_ptr = p_dns_ip_list;
	cmd._hub_domain_str = p_hub_domain;

	cmd._hub_domain_len = sd_strlen(p_hub_domain);
	
	cmd._parse_ip_list_ptr = p_parse_ip_list;
	
	/*build command*/
	ret_val = emb_reporter_build_dns_abnormal_cmd(&buffer, &len, &cmd);
	CHECK_VALUE(ret_val);
	return reporter_commit_cmd(&g_embed_hub, EMB_REPORT_DNS_ABNORMAL_CMD_TYPE, buffer, len, NULL, cmd._header._seq);			//commit a request
}

//ONLINEPEERSTAT	 
_int32 emb_reporter_online_peer_state( void )
{
	char *buffer = NULL;
	_u32  len = 0;
	/*fill request package*/
	EMB_REPORT_ONLINE_PEER_STATE_CMD cmd;
	_int32 ret_val = SUCCESS;
	_u32  _ip=0;

	LOG_DEBUG("emb_reporter_online_peer_state.");
	sd_memset(&cmd, 0, sizeof(EMB_REPORT_ONLINE_PEER_STATE_CMD));
	

	/* 版本号 */
	ret_val = get_version(cmd._thunder_version, MAX_VERSION_LEN-1); 
	CHECK_VALUE(ret_val);
	cmd._thunder_version_len = sd_strlen(cmd._thunder_version);

	/* 合作伙伴ID */
	ret_val = get_partner_id(cmd._partner_id, MAX_PARTNER_ID_LEN-1); 
	CHECK_VALUE(ret_val);
	cmd._partner_id_len = sd_strlen(cmd._partner_id);

	/* Peer ID */
	cmd._peerid_len = PEER_ID_SIZE;
	ret_val=get_peerid(cmd._peerid, PEER_ID_SIZE);
	if(ret_val!=SUCCESS)
	{
		LOG_DEBUG("Error when getting peerid!");
		return ret_val;
	}
	
	cmd._peerid_len = PEER_ID_SIZE;

	_ip = sd_get_local_ip(); 
	sd_snprintf(cmd._ip_name, MAX_HOST_NAME_LEN,"%d.%d.%d.%d", (_ip)&0xFF, (_ip>>8)&0xFF, (_ip>>16)&0xFF, (_ip>>24)&0xFF);
	cmd._ip_name_len = sd_strlen(cmd._ip_name);

	
	cmd._memory_pool_size = 0;
	
#ifdef  MEMPOOL_3M	 
	cmd._memory_pool_size = 3*1024*1024;
#endif

#ifdef  MEMPOOL_5M	 
	cmd._memory_pool_size = 5*1024*1024;
#endif

#ifdef  MEMPOOL_8M	 
	cmd._memory_pool_size = 8*1024*1024;
#endif

#ifdef  MEMPOOL_10M	 
	cmd._memory_pool_size = 10*1024*1024;
#endif
	LOG_DEBUG("emb_reporter_online_peer_state.cmd._memory_pool_size:%u", cmd._memory_pool_size );

	ret_val = sd_get_os(cmd._os_name, MAX_OS_LEN); 
	CHECK_VALUE(ret_val);
	cmd._os_name_len = sd_strlen(cmd._os_name);

	cmd._download_speed = socket_proxy_speed_limit_get_download_speed();
	cmd._download_max_speed = socket_proxy_speed_limit_get_max_download_speed();
	
	cmd._upload_speed = 0;
	cmd._upload_max_speed = 0;

#ifdef UPLOAD_ENABLE
	cmd._upload_speed = ulm_get_cur_upload_speed();
	cmd._upload_max_speed = ulm_get_max_upload_speed();
#endif

	cmd._res_num = cm_get_global_res_num();
	cmd._pipe_num = cm_get_global_pipe_num();

	/*build command*/
	ret_val = emb_reporter_build_online_peer_state_cmd(&buffer, &len, &cmd);
	CHECK_VALUE(ret_val);
	return reporter_commit_cmd(&g_embed_hub, EMB_REPORT_ONLINE_PEER_STATE_CMD_TYPE, buffer, len, NULL, cmd._header._seq);			//commit a request
}


//VODSTAT	 
_int32 emb_reporter_common_stop_task( TASK* _p_task, VOD_REPORT_PARA *p_report_para )
{
	char *buffer = NULL;
	_u32  len = 0;
	/*fill request package*/
	EMB_REPORT_COMMON_STOP_TASK_CMD cmd;
	_int32 ret_val = SUCCESS;
	P2SP_TASK * p_p2sp_task = (P2SP_TASK *)_p_task;
	char *origin_url=NULL, *file_name = NULL;
	_u64 server_dl_bytes = 0;
	_u64 peer_dl_bytes = 0;
	_u64 cdn_dl_bytes = 0;
	_u64 dl_bytes = 0;
	_u64 appacc_dl_bytes  = 0;
	struct tagFILE_INFO_REPORT_PARA *p_file_info_report_para = NULL;
	struct tagCM_REPORT_PARA *p_cm_report_para = NULL;
	struct tagRES_QUERY_REPORT_PARA *p_res_query_report_para = NULL;
	
	if( _p_task->_task_type != P2SP_TASK_TYPE )
	{
		CHECK_VALUE( REPORTER_ERR_TASK_TYPE );
	}

	LOG_DEBUG("emb_reporter_common_stop_task...vod_play_time:%u, vod_first_buffer_time:%u, vod_interrupt_times:%u, min_interrupt_time:%u, max_interrupt_time:%u, avg_interrupt_time:%u", 
		p_report_para->_vod_play_time, p_report_para->_vod_first_buffer_time, p_report_para->_vod_interrupt_times,
		p_report_para->_min_interrupt_time, p_report_para->_max_interrupt_time, p_report_para->_avg_interrupt_time );
	sd_memset(&cmd, 0, sizeof(EMB_REPORT_COMMON_STOP_TASK_CMD));
	
	dm_get_dl_bytes(&(p_p2sp_task->_data_manager), &server_dl_bytes, &peer_dl_bytes, &cdn_dl_bytes ,NULL, &appacc_dl_bytes);
	dl_bytes = server_dl_bytes + peer_dl_bytes + cdn_dl_bytes;

	LOG_DEBUG("emb_reporter_common_stop_task. dl_bytes=%llu, server:%llu, peer:%llu, cdn:%llu, accapp:%llu"
		,dl_bytes, server_dl_bytes, peer_dl_bytes, cdn_dl_bytes, appacc_dl_bytes);
	
	/* 下载花费时间 ,平均下载速度*/
	if(_p_task->_finished_time>_p_task->_start_time)
	{
		cmd._duration =(_int32)( _p_task->_finished_time-_p_task->_start_time);
		cmd._avg_speed = (_u64)dl_bytes / cmd._duration;
	}
	else
	{
		cmd._duration=0;
		cmd._avg_speed = 0;
	}
	if( cmd._duration < REPORT_STOP_TASK_MIN_TIME ) return SUCCESS;

	/* 版本号 */
	ret_val = get_version(cmd._thunder_version, MAX_VERSION_LEN-1); 
	CHECK_VALUE(ret_val);
	cmd._thunder_version_len = sd_strlen(cmd._thunder_version);

	/* 合作伙伴ID */
	ret_val = get_partner_id(cmd._partner_id, MAX_PARTNER_ID_LEN-1); 
	CHECK_VALUE(ret_val);
	cmd._partner_id_len = sd_strlen(cmd._partner_id);

	/* Peer ID */
	cmd._peerid_len = PEER_ID_SIZE;
	ret_val=get_peerid(cmd._peerid, PEER_ID_SIZE);
	if(ret_val!=SUCCESS)
	{
		LOG_DEBUG("Error when getting peerid!");
		return ret_val;
	}

	/* 任务创建类型:*/
	cmd._task_create_type = p_p2sp_task->_task_created_type;
	if( p_p2sp_task->_is_vod_task )
	{
		if( p_p2sp_task->_task_created_type == TASK_CREATED_BY_TCID )
		{
			cmd._task_create_type = NEW_TASK_CREATED_BY_KANKAN_PRIVATE;
		}
		else if( p_p2sp_task->_task_created_type == TASK_CONTINUE_BY_TCID )
		{
			cmd._task_create_type = CONTINUED_TASK_CREATED_BY_KANKAN_PRIVATE;
		}
	}

    if( p_p2sp_task->_task_created_type == TASK_CREATED_BY_URL
		|| p_p2sp_task->_task_created_type == TASK_CONTINUE_BY_URL )
    {

		/* 客户端下载的URL:*/
		origin_url = NULL;
		if(p_p2sp_task->_origion_url_length > 0)origin_url = p_p2sp_task->_origion_url;
		
		if(NULL != origin_url ||  TRUE ==  dm_get_origin_url(&(p_p2sp_task->_data_manager), &origin_url))
		{
			/* remove user name and password and decode */
			url_object_to_noauth_string(origin_url, cmd._url,MAX_URL_LEN);
			cmd._url_len = sd_strlen(cmd._url);
		}

		/* refererUrl */
		origin_url=NULL;
		if(TRUE ==  dm_get_origin_ref_url(&(p_p2sp_task->_data_manager), &origin_url))
		{
			sd_strncpy(cmd._ref_url, origin_url,MAX_URL_LEN);
			cmd._ref_url_len = sd_strlen(cmd._ref_url);
		}
    }
#ifdef IPAD_KANKAN
	// 广告任务在上报任务创建类型的第四位标识
	if(p_report_para->_is_ad_type)
	{
		cmd._task_create_type |= 8;	
	}
#endif
	/* 三段CID */
	if((TRUE==dm_get_cid( &(p_p2sp_task->_data_manager), cmd._cid))
		&&(TRUE==sd_is_cid_valid(cmd._cid)))
	{
		cmd._cid_len = CID_SIZE;
	}


	/* 全文CID */
	if((TRUE==dm_get_calc_gcid( &(p_p2sp_task->_data_manager), cmd._gcid))
		&&(TRUE==sd_is_cid_valid(cmd._gcid)))
	{
		cmd._gcid_len = CID_SIZE;
	}else if((TRUE==dm_get_shub_gcid( &(p_p2sp_task->_data_manager), cmd._gcid))
		&&(TRUE==sd_is_cid_valid(cmd._gcid)))
	{
		cmd._gcid_len = CID_SIZE;
	}


	/* 文件大小 */
	cmd._file_size = _p_task->file_size;

	/* 块大小 */
	cmd._block_size = dm_get_block_size( &(p_p2sp_task->_data_manager));

	/* 文件名称 */
	if(TRUE==dm_get_filnal_file_name( &(p_p2sp_task->_data_manager),&file_name))
	{
		sd_strncpy(cmd._file_name, file_name,MAX_FILE_NAME_LEN);
		cmd._file_name_len = sd_strlen(cmd._file_name);
	}
	
	cmd._task_type = 0;
	if( p_report_para->_vod_play_time > 0 )
	{
		cmd._task_type = cmd._task_type | 0x01;
	}
	
#ifdef _VOD_NO_DISK
	if( dt_is_no_disk_task(_p_task) )
	{
		cmd._task_type = cmd._task_type | 0x02;
	}
#endif

	cmd._vod_play_time = p_report_para->_vod_play_time;
	cmd._vod_first_buffer_time = p_report_para->_vod_first_buffer_time;
	cmd._vod_interrupt_times = p_report_para->_vod_interrupt_times;
	cmd._min_interrupt_time = p_report_para->_min_interrupt_time;
	cmd._max_interrupt_time = p_report_para->_max_interrupt_time;
	cmd._avg_interrupt_time = p_report_para->_avg_interrupt_time;
	
	cmd._server_dl_bytes = server_dl_bytes;
	cmd._peer_dl_bytes = peer_dl_bytes;
	cmd._cdn_dl_bytes = cdn_dl_bytes;
	cmd._appacc_dl_bytes = appacc_dl_bytes;

	if( _p_task->task_status == TASK_S_SUCCESS 
#ifdef KANKAN_PROJ
		|| ((_p_task->_downloaded_data_size == _p_task->file_size) && (_p_task->file_size > 0))
#endif
		|| _p_task->task_status == TASK_S_VOD )
	{
		cmd._task_status = 1;
	}
	else if( _p_task->task_status == TASK_S_RUNNING )
	{
		cmd._task_status = 0;
	}
	else if( _p_task->task_status == TASK_S_FAILED )
	{
		cmd._task_status = 2;
	}
	
	cmd._failed_code = _p_task->failed_code;
	cmd._dl_percent = _p_task->progress;

//////////////////add new
//////////////////vod_report

	cmd._bit_rate = p_report_para->_bit_rate;
	cmd._vod_total_buffer_time = p_report_para->_vod_total_buffer_time;
	cmd._vod_total_drag_wait_time = p_report_para->_vod_total_drag_wait_time;
	cmd._vod_drag_num = p_report_para->_vod_drag_num;
	
	cmd._play_interrupt_1 = p_report_para->_play_interrupt_1;
	cmd._play_interrupt_2 = p_report_para->_play_interrupt_2;
	cmd._play_interrupt_3 = p_report_para->_play_interrupt_3;
	cmd._play_interrupt_4 = p_report_para->_play_interrupt_4;
	cmd._play_interrupt_5 = p_report_para->_play_interrupt_5;
	cmd._play_interrupt_6 = p_report_para->_play_interrupt_6;
	
	cmd._cdn_stop_times = p_report_para->_cdn_stop_times;
	
	LOG_DEBUG("emb_reporter_common_stop_task, vod_report:cmd._bit_rate=%u, cmd._vod_total_buffer_time=%u, cmd._vod_total_drag_wait_time=%u, cmd._vod_drag_num=%u, \
		cmd._play_interrupt_1=%u, cmd._play_interrupt_2=%u, cmd._play_interrupt_3=%u, cmd._play_interrupt_4=%u, cmd._play_interrupt_5=%u, cmd._play_interrupt_6=%u,\
		cmd._cdn_stop_times=%u",
		cmd._bit_rate, cmd._vod_total_buffer_time, cmd._vod_total_drag_wait_time, cmd._vod_drag_num, 
		cmd._play_interrupt_1, cmd._play_interrupt_2, cmd._play_interrupt_3, cmd._play_interrupt_4, 
		cmd._play_interrupt_5, cmd._play_interrupt_6, cmd._cdn_stop_times);


//////file_info_report
	p_file_info_report_para = dm_get_report_para(&p_p2sp_task->_data_manager);
	if( p_file_info_report_para == NULL )
	{
		sd_assert( FALSE );	
		return SUCCESS;
	}

	cmd._down_exist = p_file_info_report_para->_down_exist;
	cmd._overlap_bytes = p_file_info_report_para->_overlap_bytes;
	
	cmd._down_n2i_no = p_file_info_report_para->_down_n2i_no;
	cmd._down_n2i_yes = p_file_info_report_para->_down_n2i_yes;
	cmd._down_n2n_no = p_file_info_report_para->_down_n2n_no;
	cmd._down_n2n_yes = p_file_info_report_para->_down_n2n_yes;
	cmd._down_n2s_no = p_file_info_report_para->_down_n2s_no;
	cmd._down_n2s_yes = p_file_info_report_para->_down_n2s_yes;
	cmd._down_i2i_no = p_file_info_report_para->_down_i2i_no;
	cmd._down_i2i_yes = p_file_info_report_para->_down_i2i_yes;
	cmd._down_i2n_no = p_file_info_report_para->_down_i2n_no;
	cmd._down_i2n_yes = p_file_info_report_para->_down_i2n_yes;
	
	cmd._down_by_partner_cdn = p_file_info_report_para->_down_by_partner_cdn;
	cmd._down_err_by_cdn = p_file_info_report_para->_down_err_by_cdn;
	cmd._down_err_by_partner_cdn = p_file_info_report_para->_down_err_by_partner_cdn;
	cmd._down_err_by_peer = p_file_info_report_para->_down_err_by_peer;
	cmd._down_err_by_svr = p_file_info_report_para->_down_err_by_svr;
	cmd._down_err_by_appacc = p_file_info_report_para->_down_err_by_appacc;

	LOG_DEBUG("emb_reporter_common_stop_task, file_info_report:cmd._down_exist:%llu, cmd._overlap_bytes:%llu,\
		cmd._down_n2i_no:%llu, cmd._down_n2i_yes:%llu, cmd._down_n2n_no:%llu, cmd._down_n2n_yes:%llu, cmd._down_n2s_no:%llu,cmd._down_n2s_yes:%llu, cmd._down_i2i_no:%llu, cmd._down_i2i_yes:%llu, cmd._down_i2n_no:%llu, cmd._down_i2n_yes:%llu,\
		cmd._down_by_partner_cdn:%llu, cmd._down_err_by_cdn:%llu, cmd._down_err_by_partner_cdn:%llu, cmd._down_err_by_peer:%llu, cmd._down_err_by_svr:%llu, ",\
		cmd._down_exist, cmd._overlap_bytes, cmd._down_n2i_no, cmd._down_n2i_yes, cmd._down_n2n_no, cmd._down_n2n_yes,
		cmd._down_n2s_no, cmd._down_n2s_yes, cmd._down_i2i_no, cmd._down_i2i_yes, cmd._down_i2n_no, cmd._down_i2n_yes,
		cmd._down_by_partner_cdn, cmd._down_err_by_cdn, cmd._down_err_by_partner_cdn, cmd._down_err_by_peer, cmd._down_err_by_svr);
	
//////cm_report
	p_cm_report_para = cm_get_report_para( &_p_task->_connect_manager, INVALID_FILE_INDEX );
	if( p_cm_report_para == NULL )
	{
		sd_assert( FALSE );	
		return SUCCESS;
	}	
	
	cmd._res_server_total = p_cm_report_para->_res_server_total;
	cmd._res_server_valid = p_cm_report_para->_res_server_valid;
	cmd._res_cdn_total = p_cm_report_para->_res_cdn_total;
	cmd._res_cdn_valid = p_cm_report_para->_res_cdn_valid;
	cmd._res_partner_cdn_total = p_cm_report_para->_res_partner_cdn_total;
	cmd._res_partner_cdn_valid = p_cm_report_para->_res_partner_cdn_valid;	
	
	cmd._cdn_res_max_num = p_cm_report_para->_cdn_res_max_num;
	cmd._cdn_connect_failed_times = p_cm_report_para->_cdn_connect_failed_times;
	
	cmd._res_n2i_total = p_cm_report_para->_res_n2i_total;
	cmd._res_n2i_valid = p_cm_report_para->_res_n2i_valid;
	cmd._res_n2n_total = p_cm_report_para->_res_n2n_total;
	cmd._res_n2n_valid = p_cm_report_para->_res_n2n_valid;
	cmd._res_n2s_total = p_cm_report_para->_res_n2s_total;
	cmd._res_n2s_valid = p_cm_report_para->_res_n2s_valid;
	cmd._res_i2i_total = p_cm_report_para->_res_i2i_total;
	cmd._res_i2i_valid = p_cm_report_para->_res_i2i_valid;
	cmd._res_i2n_total = p_cm_report_para->_res_i2n_total;
	cmd._res_i2n_valid = p_cm_report_para->_res_i2n_valid;

	cmd._hub_res_total = p_cm_report_para->_hub_res_total;
	cmd._hub_res_valid = p_cm_report_para->_hub_res_valid;
	cmd._active_tracker_res_total = p_cm_report_para->_active_tracker_res_total;
	cmd._active_tracker_res_valid = p_cm_report_para->_active_tracker_res_valid;

	LOG_DEBUG("emb_reporter_common_stop_task, cm_report :cmd._res_server_total=%u, cmd._res_server_valid=%u, cmd._res_cdn_total=%u, cmd._res_cdn_valid=%u,cmd._res_partner_cdn_total=%u, cmd._res_partner_cdn_valid=%u,\
		cmd._cdn_res_max_num=%u, cmd._cdn_connect_failed_times=%u, \
		cmd._res_n2i_total=%u, cmd._res_n2i_valid=%u, cmd._res_n2n_total=%u, cmd._res_n2n_valid=%u, cmd._res_n2s_total=%u, cmd._res_n2s_valid=%u, cmd._res_i2i_total=%u, cmd._res_i2i_valid=%u, cmd._res_i2n_total=%u, cmd._res_i2n_valid=%u,\
		cmd._hub_res_total=%u, cmd._hub_res_valid=%u, cmd._active_tracker_res_total=%u, cmd._active_tracker_res_valid=%u, ",\
		cmd._res_server_total, cmd._res_server_valid, cmd._res_cdn_total, cmd._res_cdn_valid, cmd._res_partner_cdn_total, cmd._res_partner_cdn_valid,
		cmd._cdn_res_max_num, cmd._cdn_connect_failed_times, cmd._res_n2i_total, cmd._res_n2i_valid, 
		cmd._res_n2n_total, cmd._res_n2n_valid, cmd._res_n2s_total, cmd._res_n2s_valid, cmd._res_i2i_total, 
		cmd._res_i2i_valid, cmd._res_i2n_total, cmd._res_i2n_valid, cmd._hub_res_total, cmd._hub_res_valid,
		cmd._active_tracker_res_total, cmd._active_tracker_res_valid);
	

//////res_query_report
	p_res_query_report_para = dt_get_res_query_report_para(_p_task);
	if( p_res_query_report_para == NULL )
	{
		sd_assert( FALSE );	
		return SUCCESS;
	}	

	cmd._hub_s_max = p_res_query_report_para->_hub_s_max;
	cmd._hub_s_min = p_res_query_report_para->_hub_s_min;
	cmd._hub_s_avg = p_res_query_report_para->_hub_s_avg;
	cmd._hub_s_succ = p_res_query_report_para->_hub_s_succ;
	cmd._hub_s_fail = p_res_query_report_para->_hub_s_fail;

	cmd._hub_p_max = p_res_query_report_para->_hub_p_max;
	cmd._hub_p_min = p_res_query_report_para->_hub_p_min;
	cmd._hub_p_avg = p_res_query_report_para->_hub_p_avg;
	cmd._hub_p_succ = p_res_query_report_para->_hub_p_succ;
	cmd._hub_p_fail = p_res_query_report_para->_hub_p_fail;

	cmd._hub_t_max = p_res_query_report_para->_hub_t_max;
	cmd._hub_t_min = p_res_query_report_para->_hub_t_min;
	cmd._hub_t_avg = p_res_query_report_para->_hub_t_avg;
	cmd._hub_t_succ = p_res_query_report_para->_hub_t_succ;
	cmd._hub_t_fail = p_res_query_report_para->_hub_t_fail;
	
#if defined(MOBILE_PHONE)
	cmd._network_info = sd_get_net_type();
#endif


	LOG_DEBUG("emb_reporter_common_stop_task cmd._server_dl_bytes:%llu,cmd._peer_dl_bytes:%llu,  cmd._cdn_dl_bytes :%llu",cmd._server_dl_bytes ,
		cmd._peer_dl_bytes , cmd._cdn_dl_bytes );
	
	LOG_DEBUG("emb_reporter_common_stop_task, res_query_report :\
		cmd._hub_s_max=%u, cmd._hub_s_min=%u,cmd._hub_s_avg=%u, cmd._hub_s_succ=%u, cmd._hub_s_fail=%u, \
		cmd._hub_p_max=%u, cmd._hub_p_min=%u,cmd._hub_p_avg=%u, cmd._hub_p_succ=%u, cmd._hub_p_fail=%u, \
		cmd._hub_t_max=%u, cmd._hub_t_min=%u, cmd._hub_t_avg=%u, cmd._hub_t_succ=%u, cmd._hub_t_fail=%u",\
		cmd._hub_s_max, cmd._hub_s_min, cmd._hub_s_avg, cmd._hub_s_succ, cmd._hub_s_fail, 
		cmd._hub_p_max, cmd._hub_p_min, cmd._hub_p_avg, cmd._hub_p_succ, cmd._hub_p_fail,
		cmd._hub_t_max, cmd._hub_t_min, cmd._hub_t_avg, cmd._hub_t_succ, cmd._hub_t_fail);
	

	/*build command*/
	ret_val = emb_reporter_build_common_stop_task_cmd(&buffer, &len, &cmd);
	CHECK_VALUE(ret_val);
	return reporter_commit_cmd(&g_embed_hub, EMB_REPORT_COMMON_STOP_TASK_CMD_TYPE, buffer, len, NULL, cmd._header._seq);			//commit a request
}

#ifdef ENABLE_BT

//STOPSTAT	 
_int32 emb_reporter_bt_stop_task( TASK* _p_task )
{
	char *buffer = NULL;
	_u32  len = 0;
	/*fill request package*/
	EMB_REPORT_BT_STOP_TASK_CMD cmd;
	_int32 ret_val = SUCCESS;
	BT_TASK * p_bt_task = (BT_TASK *)_p_task;
	_u8 *info_hash=NULL;
	char * p_str=NULL;
	_u64 server_dl_bytes = 0;
	_u64 peer_dl_bytes = 0;
	_u64 cdn_dl_bytes = 0;
	_u64 origion_dl_bytes = 0;
	_u64 dl_bytes = 0;

	LOG_DEBUG("emb_reporter_bt_stop_task" );
	sd_memset(&cmd, 0, sizeof(EMB_REPORT_BT_STOP_TASK_CMD));
	
	bdm_get_dl_bytes(&(p_bt_task->_data_manager), &server_dl_bytes, &peer_dl_bytes, &cdn_dl_bytes, &origion_dl_bytes );
	dl_bytes = server_dl_bytes + peer_dl_bytes + cdn_dl_bytes+ origion_dl_bytes;

	LOG_DEBUG("emb_reporter_bt_stop_task...server_dl_bytes=%llu, peer_dl_bytes=%llu,cdn_dl_bytes=%llu,origion_dl_bytes=%llu, dl_bytes=%llu",
		server_dl_bytes, peer_dl_bytes, cdn_dl_bytes, origion_dl_bytes, dl_bytes);
	
	if(_p_task->_finished_time>_p_task->_start_time)
	{
		cmd._duration =(_int32)( _p_task->_finished_time-_p_task->_start_time);
		cmd._avg_speed= (_u64)dl_bytes/ cmd._duration;
	}
	else
	{
		cmd._duration=0;
		cmd._avg_speed=0;
	}
	if( cmd._duration < REPORT_STOP_TASK_MIN_TIME ) return SUCCESS;	
	
	/* 版本号 */
	ret_val = get_version(cmd._thunder_version, MAX_VERSION_LEN-1); 
	CHECK_VALUE(ret_val);
	cmd._thunder_version_len = sd_strlen(cmd._thunder_version);

	/* 合作伙伴ID */
	ret_val = get_partner_id(cmd._partner_id, MAX_PARTNER_ID_LEN-1); 
	CHECK_VALUE(ret_val);
	cmd._partner_id_len = sd_strlen(cmd._partner_id);

	/* Peer ID */
	cmd._peerid_len = PEER_ID_SIZE;
	ret_val=get_peerid(cmd._peerid, PEER_ID_SIZE);
	if(ret_val!=SUCCESS)
	{
		LOG_DEBUG("Error when getting peerid!");
		return ret_val;
	}


	/* info_hash */
	ret_val = tp_get_file_info_hash(  p_bt_task->_torrent_parser_ptr, &info_hash );
	CHECK_VALUE(ret_val);
	sd_memcpy(cmd._info_hash, info_hash,CID_SIZE);
	cmd._info_hash_len = CID_SIZE;

	/* title_name */
	ret_val = tp_get_seed_title_name(  p_bt_task->_torrent_parser_ptr, &p_str );
	CHECK_VALUE(ret_val);
	sd_strncpy(cmd._file_title, p_str,MAX_FILE_NAME_BUFFER_LEN);
	cmd._file_title_len = sd_strlen(cmd._file_title);


	cmd._file_num = tp_get_seed_file_num(  p_bt_task->_torrent_parser_ptr );
	
	cmd._file_total_size = tp_get_file_total_size( p_bt_task->_torrent_parser_ptr );

	cmd._piece_size = tp_get_piece_size( p_bt_task->_torrent_parser_ptr );
	

	cmd._task_type = 0;
	if( bdm_is_vod_mode(&p_bt_task->_data_manager) )
	{
		cmd._task_type = cmd._task_type | 0x01;
	}	

	cmd._server_dl_bytes = server_dl_bytes;
	cmd._peer_dl_bytes = peer_dl_bytes;
	cmd._cdn_dl_bytes = cdn_dl_bytes;

	LOG_DEBUG("emb_reporter_bt_stop_task cmd._server_dl_bytes:%llu,cmd._peer_dl_bytes:%llu,  cmd._cdn_dl_bytes :%llu",cmd._server_dl_bytes ,
		cmd._peer_dl_bytes , cmd._cdn_dl_bytes );

	if( _p_task->task_status == TASK_S_RUNNING )
	{
		cmd._task_status = 0;
	}
	else if( _p_task->task_status == TASK_S_SUCCESS 
		|| _p_task->task_status == TASK_S_VOD )
	{
		cmd._task_status = 1;
	}
	else if( _p_task->task_status == TASK_S_FAILED )
	{
		cmd._task_status = 2;
	}
	
	cmd._failed_code = _p_task->failed_code;
	cmd._dl_percent = _p_task->progress;
		
#if defined(MOBILE_PHONE)
	cmd._network_info = sd_get_net_type();
#endif
		
	/*build command*/
	ret_val = emb_reporter_build_bt_stop_task_cmd(&buffer, &len, &cmd);
	CHECK_VALUE(ret_val);
	return reporter_commit_cmd(&g_embed_hub, EMB_REPORT_BT_STOP_TASK_CMD_TYPE, buffer, len, NULL, cmd._header._seq);			//commit a request
}

_int32 emb_reporter_bt_stop_file( TASK* _p_task, _u32 file_index, VOD_REPORT_PARA *p_report_para, BOOL is_vod_file )
{
	char *buffer = NULL;
	_u32  len = 0;
	/*fill request package*/
	EMB_REPORT_BT_STOP_FILE_CMD cmd;
	_int32 ret_val = SUCCESS;
	BT_TASK * p_bt_task = (BT_TASK *)_p_task;
	_u8 *info_hash=NULL;
	char * p_str=NULL;
	TORRENT_FILE_INFO *p_file_info=NULL;
	_u32 percent = 0;
	enum BT_FILE_STATUS file_status;
	_u64 server_dl_bytes = 0;
	_u64 peer_dl_bytes = 0;
	_u64 cdn_dl_bytes = 0;
	_u64 origion_dl_bytes = 0;
	_u64 dl_bytes = 0;
	struct tagFILE_INFO_REPORT_PARA *p_file_info_report_para = NULL;
	struct tagCM_REPORT_PARA *p_cm_report_para = NULL;
	struct tagRES_QUERY_REPORT_PARA *p_res_query_report_para = NULL;
	

	LOG_DEBUG("emb_reporter_bt_stop_file, file_index:%u", file_index );
	sd_memset(&cmd, 0, sizeof(EMB_REPORT_BT_STOP_FILE_CMD));
	
	bdm_get_sub_file_dl_bytes(&(p_bt_task->_data_manager), file_index, &server_dl_bytes, &peer_dl_bytes, &cdn_dl_bytes,NULL , &origion_dl_bytes);
	dl_bytes = server_dl_bytes + peer_dl_bytes + cdn_dl_bytes+ origion_dl_bytes;

	LOG_DEBUG("emb_reporter_bt_stop_file...server_dl_bytes=%llu, peer_dl_bytes=%llu,cdn_dl_bytes=%llu,origion_dl_bytes=%llu, dl_bytes=%llu",
		server_dl_bytes, peer_dl_bytes, cdn_dl_bytes, origion_dl_bytes, dl_bytes);
	
	cmd._duration=bdm_get_file_download_time( &(p_bt_task->_data_manager), file_index );
	if(cmd._duration==0)
	{
		cmd._avg_speed=0;
	}
	else
	{
		cmd._avg_speed= (_u64)dl_bytes /cmd._duration;
	}
	if( cmd._duration < REPORT_STOP_TASK_MIN_TIME ) return SUCCESS;	

	/* 版本号 */
	ret_val = get_version(cmd._thunder_version, MAX_VERSION_LEN-1); 
	CHECK_VALUE(ret_val);
	cmd._thunder_version_len = sd_strlen(cmd._thunder_version);

	/* 合作伙伴ID */
	ret_val = get_partner_id(cmd._partner_id, MAX_PARTNER_ID_LEN-1); 
	CHECK_VALUE(ret_val);
	cmd._partner_id_len = sd_strlen(cmd._partner_id);

	/* Peer ID */
	cmd._peerid_len = PEER_ID_SIZE;
	ret_val=get_peerid(cmd._peerid, PEER_ID_SIZE);
	if(ret_val!=SUCCESS)
	{
		LOG_DEBUG("Error when getting peerid!");
		return ret_val;
	}


	/* info_hash */
	ret_val = tp_get_file_info_hash(  p_bt_task->_torrent_parser_ptr, &info_hash );
	CHECK_VALUE(ret_val);
	sd_memcpy(cmd._info_hash, info_hash,CID_SIZE);
	cmd._info_hash_len = CID_SIZE;

	/* title_name */
	ret_val = tp_get_seed_title_name(  p_bt_task->_torrent_parser_ptr, &p_str );
	CHECK_VALUE(ret_val);
	sd_strncpy(cmd._file_title, p_str,MAX_FILE_NAME_BUFFER_LEN);
	cmd._file_title_len = sd_strlen(cmd._file_title);


	cmd._file_num = tp_get_seed_file_num(  p_bt_task->_torrent_parser_ptr );
	
	cmd._file_total_size = tp_get_file_total_size( p_bt_task->_torrent_parser_ptr );

	cmd._piece_size = tp_get_piece_size( p_bt_task->_torrent_parser_ptr );

	cmd._file_index= file_index;
	
	ret_val = tp_get_file_info( p_bt_task->_torrent_parser_ptr,  file_index, &p_file_info );
	CHECK_VALUE(ret_val);
	
	cmd._file_offset= p_file_info->_file_offset;

	if((TRUE==bdm_get_cid( &(p_bt_task->_data_manager), file_index,cmd._cid))
		&&(TRUE==sd_is_cid_valid(cmd._cid)))
	{
			cmd._cid_len = CID_SIZE;
	}

	if((TRUE==bdm_get_calc_gcid( &(p_bt_task->_data_manager),  file_index,cmd._gcid))
		&&(TRUE==sd_is_cid_valid(cmd._gcid)))
	{
		cmd._gcid_len = CID_SIZE;
	}else
	if((TRUE==bdm_get_shub_gcid( &(p_bt_task->_data_manager), file_index, cmd._gcid))
		&&(TRUE==sd_is_cid_valid(cmd._gcid)))
	{
		cmd._gcid_len = CID_SIZE;
	}

	cmd._file_size = p_file_info->_file_size;

	cmd._file_name_len = p_file_info->_file_name_len;
	sd_strncpy(cmd._file_name, p_file_info->_file_name, MAX_FILE_NAME_BUFFER_LEN);

	percent = bdm_get_sub_file_percent(&(p_bt_task->_data_manager), file_index);

	cmd._task_type = 0;
	
	if( is_vod_file )
	{
		cmd._task_type = cmd._task_type | 0x01;
	}	
	
	cmd._vod_play_time = p_report_para->_vod_play_time;
	cmd._vod_first_buffer_time = p_report_para->_vod_first_buffer_time;
	cmd._vod_interrupt_times = p_report_para->_vod_interrupt_times;
	cmd._min_interrupt_time = p_report_para->_min_interrupt_time;
	cmd._max_interrupt_time = p_report_para->_max_interrupt_time;
	cmd._avg_interrupt_time = p_report_para->_avg_interrupt_time;

	cmd._server_dl_bytes = server_dl_bytes;
	cmd._peer_dl_bytes = peer_dl_bytes;
	cmd._cdn_dl_bytes = cdn_dl_bytes;

	file_status = bdm_get_file_status(&(p_bt_task->_data_manager), file_index);
	if( file_status == BT_FILE_DOWNLOADING 
		&& !bdm_is_waited_success_close_file(&(p_bt_task->_data_manager), file_index))
	{
		cmd._task_status = 0;
	}
	else if( file_status == BT_FILE_FINISHED
		|| file_status == BT_FILE_DOWNLOADING )
	{
		cmd._task_status = 1;
	}
	else if( file_status == BT_FILE_FAILURE )
	{
		cmd._task_status = 2;
	}
	
	cmd._failed_code = bdm_get_file_err_code(&(p_bt_task->_data_manager), file_index);
	cmd._dl_percent = bdm_get_sub_file_percent(&(p_bt_task->_data_manager), file_index);


	//////////////////add new
	//////////////////vod_report

	cmd._bit_rate = p_report_para->_bit_rate;
	cmd._vod_total_buffer_time = p_report_para->_vod_total_buffer_time;
	cmd._vod_total_drag_wait_time = p_report_para->_vod_total_drag_wait_time;
	cmd._vod_drag_num = p_report_para->_vod_drag_num;
	
	cmd._play_interrupt_1 = p_report_para->_play_interrupt_1;
	cmd._play_interrupt_2 = p_report_para->_play_interrupt_2;
	cmd._play_interrupt_3 = p_report_para->_play_interrupt_3;
	cmd._play_interrupt_4 = p_report_para->_play_interrupt_4;
	cmd._play_interrupt_5 = p_report_para->_play_interrupt_5;
	cmd._play_interrupt_6 = p_report_para->_play_interrupt_6;
	
	cmd._cdn_stop_times = p_report_para->_cdn_stop_times;
	
	LOG_DEBUG("emb_reporter_bt_stop_file, vod_report:cmd._bit_rate=%u, cmd._vod_total_buffer_time=%u, cmd._vod_total_drag_wait_time=%u, cmd._vod_drag_num=%u, \
		cmd._play_interrupt_1=%u, cmd._play_interrupt_2=%u, cmd._play_interrupt_3=%u, cmd._play_interrupt_4=%u, cmd._play_interrupt_5=%u, cmd._play_interrupt_6=%u,\
		cmd._cdn_stop_times=%u",
		cmd._bit_rate, cmd._vod_total_buffer_time, cmd._vod_total_drag_wait_time, cmd._vod_drag_num, 
		cmd._play_interrupt_1, cmd._play_interrupt_2, cmd._play_interrupt_3, cmd._play_interrupt_4, 
		cmd._play_interrupt_5, cmd._play_interrupt_6, cmd._cdn_stop_times);


	//////file_info_report
	p_file_info_report_para = bdm_get_report_para(&p_bt_task->_data_manager, file_index );
	if( p_file_info_report_para == NULL )
	{
		sd_assert( FALSE ); 
		return SUCCESS;
	}	
	
	cmd._down_exist = p_file_info_report_para->_down_exist;
	cmd._overlap_bytes = p_file_info_report_para->_overlap_bytes;

	cmd._down_n2i_no = p_file_info_report_para->_down_n2i_no;
	cmd._down_n2i_yes = p_file_info_report_para->_down_n2i_yes;
	cmd._down_n2n_no = p_file_info_report_para->_down_n2n_no;
	cmd._down_n2n_yes = p_file_info_report_para->_down_n2n_yes;
	cmd._down_n2s_no = p_file_info_report_para->_down_n2s_no;
	cmd._down_n2s_yes = p_file_info_report_para->_down_n2s_yes;
	cmd._down_i2i_no = p_file_info_report_para->_down_i2i_no;
	cmd._down_i2i_yes = p_file_info_report_para->_down_i2i_yes;
	cmd._down_i2n_no = p_file_info_report_para->_down_i2n_no;
	cmd._down_i2n_yes = p_file_info_report_para->_down_i2n_yes;

	cmd._down_by_partner_cdn = p_file_info_report_para->_down_by_partner_cdn;
	cmd._down_err_by_cdn = p_file_info_report_para->_down_err_by_cdn;
	cmd._down_err_by_partner_cdn = p_file_info_report_para->_down_err_by_partner_cdn;
	cmd._down_err_by_peer = p_file_info_report_para->_down_err_by_peer;
	cmd._down_err_by_svr = p_file_info_report_para->_down_err_by_svr;

	LOG_DEBUG("emb_reporter_bt_stop_file, cmd._cdn_dl_bytes:%llu, file_info_report:cmd._down_exist:%llu, cmd._overlap_bytes:%llu,\
		cmd._down_n2i_no:%llu, cmd._down_n2i_yes:%llu, cmd._down_n2n_no:%llu, cmd._down_n2n_yes:%llu, cmd._down_n2s_no:%llu,cmd._down_n2s_yes:%llu, cmd._down_i2i_no:%llu, cmd._down_i2i_yes:%llu, cmd._down_i2n_no:%llu, cmd._down_i2n_yes:%llu,\
		cmd._down_by_partner_cdn:%llu, cmd._down_err_by_cdn:%llu, cmd._down_err_by_partner_cdn:%llu, cmd._down_err_by_peer:%llu, cmd._down_err_by_svr:%llu, ",\
		cmd._cdn_dl_bytes,
		cmd._down_exist, cmd._overlap_bytes, cmd._down_n2i_no, cmd._down_n2i_yes, cmd._down_n2n_no, cmd._down_n2n_yes,
		cmd._down_n2s_no, cmd._down_n2s_yes, cmd._down_i2i_no, cmd._down_i2i_yes, cmd._down_i2n_no, cmd._down_i2n_yes,
		cmd._down_by_partner_cdn, cmd._down_err_by_cdn, cmd._down_err_by_partner_cdn, cmd._down_err_by_peer, cmd._down_err_by_svr);
	
	//////cm_report
	p_cm_report_para = cm_get_report_para( &_p_task->_connect_manager, file_index );
	if( p_cm_report_para != NULL )
	{
		cmd._res_server_total = p_cm_report_para->_res_server_total;
		cmd._res_server_valid = p_cm_report_para->_res_server_valid;
		cmd._res_cdn_total = p_cm_report_para->_res_cdn_total;
		cmd._res_cdn_valid = p_cm_report_para->_res_cdn_valid;
		cmd._res_partner_cdn_total = p_cm_report_para->_res_partner_cdn_total;
		cmd._res_partner_cdn_valid = p_cm_report_para->_res_partner_cdn_valid;


		cmd._cdn_res_max_num = p_cm_report_para->_cdn_res_max_num;
		cmd._cdn_connect_failed_times = p_cm_report_para->_cdn_connect_failed_times;

		cmd._res_n2i_total = p_cm_report_para->_res_n2i_total;
		cmd._res_n2i_valid = p_cm_report_para->_res_n2i_valid;
		cmd._res_n2n_total = p_cm_report_para->_res_n2n_total;
		cmd._res_n2n_valid = p_cm_report_para->_res_n2n_valid;
		cmd._res_n2s_total = p_cm_report_para->_res_n2s_total;
		cmd._res_n2s_valid = p_cm_report_para->_res_n2s_valid;
		cmd._res_i2i_total = p_cm_report_para->_res_i2i_total;
		cmd._res_i2i_valid = p_cm_report_para->_res_i2i_valid;
		cmd._res_i2n_total = p_cm_report_para->_res_i2n_total;
		cmd._res_i2n_valid = p_cm_report_para->_res_i2n_valid;

		cmd._hub_res_total = p_cm_report_para->_hub_res_total;
		cmd._hub_res_valid = p_cm_report_para->_hub_res_valid;
		cmd._active_tracker_res_total = p_cm_report_para->_active_tracker_res_total;
		cmd._active_tracker_res_valid = p_cm_report_para->_active_tracker_res_valid;		

		LOG_DEBUG("emb_reporter_bt_stop_file, cm_report :cmd._res_server_total=%u, cmd._res_server_valid=%u, cmd._res_cdn_total=%u, cmd._res_cdn_valid=%u,cmd._res_partner_cdn_total=%u, cmd._res_partner_cdn_valid=%u,\
			cmd._cdn_res_max_num=%u, cmd._cdn_connect_failed_times=%u, \
			cmd._res_n2i_total=%u, cmd._res_n2i_valid=%u, cmd._res_n2n_total=%u, cmd._res_n2n_valid=%u, cmd._res_n2s_total=%u, cmd._res_n2s_valid=%u, cmd._res_i2i_total=%u, cmd._res_i2i_valid=%u, cmd._res_i2n_total=%u, cmd._res_i2n_valid=%u,\
			cmd._hub_res_total=%u, cmd._hub_res_valid=%u, cmd._active_tracker_res_total=%u, cmd._active_tracker_res_valid=%u, ",\
			cmd._res_server_total, cmd._res_server_valid, cmd._res_cdn_total, cmd._res_cdn_valid, cmd._res_partner_cdn_total, cmd._res_partner_cdn_valid,
			cmd._cdn_res_max_num, cmd._cdn_connect_failed_times, cmd._res_n2i_total, cmd._res_n2i_valid, 
			cmd._res_n2n_total, cmd._res_n2n_valid, cmd._res_n2s_total, cmd._res_n2s_valid, cmd._res_i2i_total, 
			cmd._res_i2i_valid, cmd._res_i2n_total, cmd._res_i2n_valid, cmd._hub_res_total, cmd._hub_res_valid,
			cmd._active_tracker_res_total, cmd._active_tracker_res_valid);
		

	}



	//////res_query_report
	p_res_query_report_para = dt_get_res_query_report_para(_p_task);
	if( p_res_query_report_para == NULL )
	{
		sd_assert( FALSE ); 
		return SUCCESS;
	}	
	
	cmd._hub_s_max = p_res_query_report_para->_hub_s_max;
	cmd._hub_s_min = p_res_query_report_para->_hub_s_min;
	cmd._hub_s_avg = p_res_query_report_para->_hub_s_avg;
	cmd._hub_s_succ = p_res_query_report_para->_hub_s_succ;
	cmd._hub_s_fail = p_res_query_report_para->_hub_s_fail;

	cmd._hub_p_max = p_res_query_report_para->_hub_p_max;
	cmd._hub_p_min = p_res_query_report_para->_hub_p_min;
	cmd._hub_p_avg = p_res_query_report_para->_hub_p_avg;
	cmd._hub_p_succ = p_res_query_report_para->_hub_p_succ;
	cmd._hub_p_fail = p_res_query_report_para->_hub_p_fail;

	cmd._hub_t_max = p_res_query_report_para->_hub_t_max;
	cmd._hub_t_min = p_res_query_report_para->_hub_t_min;
	cmd._hub_t_avg = p_res_query_report_para->_hub_t_avg;
	cmd._hub_t_succ = p_res_query_report_para->_hub_t_succ;
	cmd._hub_t_fail = p_res_query_report_para->_hub_t_fail;

#if defined(MOBILE_PHONE)
	cmd._network_info = sd_get_net_type();
#endif


	LOG_DEBUG("emb_reporter_bt_stop_file cmd._server_dl_bytes:%llu,cmd._peer_dl_bytes:%llu,  cmd._cdn_dl_bytes :%llu",cmd._server_dl_bytes ,
		cmd._peer_dl_bytes , cmd._cdn_dl_bytes );
	
	LOG_DEBUG("emb_reporter_bt_stop_file, res_query_report :\
		cmd._hub_s_max=%u, cmd._hub_s_min=%u,cmd._hub_s_avg=%u, cmd._hub_s_succ=%u, cmd._hub_s_fail=%u, \
		cmd._hub_p_max=%u, cmd._hub_p_min=%u,cmd._hub_p_avg=%u, cmd._hub_p_succ=%u, cmd._hub_p_fail=%u, \
		cmd._hub_t_max=%u, cmd._hub_t_min=%u, cmd._hub_t_avg=%u, cmd._hub_t_succ=%u, cmd._hub_t_fail=%u",\
		cmd._hub_s_max, cmd._hub_s_min, cmd._hub_s_avg, cmd._hub_s_succ, cmd._hub_s_fail, 
		cmd._hub_p_max, cmd._hub_p_min, cmd._hub_p_avg, cmd._hub_p_succ, cmd._hub_p_fail,
		cmd._hub_t_max, cmd._hub_t_min, cmd._hub_t_avg, cmd._hub_t_succ, cmd._hub_t_fail);

	/*build command*/
	ret_val = emb_reporter_build_bt_stop_file_cmd(&buffer, &len, &cmd);
	CHECK_VALUE(ret_val);
	return reporter_commit_cmd(&g_embed_hub, EMB_REPORT_BT_STOP_FILE_CMD_TYPE, buffer, len, NULL, cmd._header._seq);			//commit a request
}

#endif

#ifdef ENABLE_EMULE
_int32 emb_reporter_emule_stop_task( TASK* _p_task, VOD_REPORT_PARA *p_report_para, enum TASK_STATUS status )
{
	char *buffer = NULL;
	_u32  len = 0;
	/*fill request package*/
	EMB_REPORT_EMULE_STOP_TASK_CMD cmd;
	_int32 ret_val = SUCCESS;
	EMULE_TASK * p_emule_task = (EMULE_TASK *)_p_task;
	_u64 server_dl_bytes = 0;
	_u64 peer_dl_bytes = 0;
	_u64 emule_dl_bytes = 0;
	_u64 cdn_dl_bytes = 0;
	_u64 dl_bytes = 0;
	struct tagFILE_INFO_REPORT_PARA *p_file_info_report_para = NULL;
	struct tagCM_REPORT_PARA *p_cm_report_para = NULL;
	struct tagRES_QUERY_REPORT_PARA *p_res_query_report_para = NULL;
	
	if( _p_task->_task_type != EMULE_TASK_TYPE )
	{
		CHECK_VALUE( REPORTER_ERR_TASK_TYPE );
	}

	LOG_DEBUG("emb_reporter_emule_stop_task...vod_play_time:%u, vod_first_buffer_time:%u, vod_interrupt_times:%u, min_interrupt_time:%u, max_interrupt_time:%u, avg_interrupt_time:%u", 
		p_report_para->_vod_play_time, p_report_para->_vod_first_buffer_time, p_report_para->_vod_interrupt_times,
		p_report_para->_min_interrupt_time, p_report_para->_max_interrupt_time, p_report_para->_avg_interrupt_time );
	sd_memset(&cmd, 0, sizeof(EMB_REPORT_EMULE_STOP_TASK_CMD));
	
	emule_get_dl_bytes(p_emule_task->_data_manager, &server_dl_bytes, &peer_dl_bytes, &emule_dl_bytes, &cdn_dl_bytes ,NULL);
	dl_bytes = server_dl_bytes + peer_dl_bytes + emule_dl_bytes + cdn_dl_bytes;
	
	/* 下载花费时间 ,平均下载速度*/
	if(_p_task->_finished_time>_p_task->_start_time)
	{
		cmd._duration =(_u32)( _p_task->_finished_time-_p_task->_start_time);
		cmd._avg_speed = (_u64)dl_bytes / cmd._duration;
	}
	else
	{
		cmd._duration=0;
		cmd._avg_speed = 0;
	}
	//if( cmd._duration < REPORT_STOP_TASK_MIN_TIME ) return SUCCESS;

	/* 版本号 */
	ret_val = get_version(cmd._thunder_version, MAX_VERSION_LEN-1); 
	CHECK_VALUE(ret_val);
	cmd._thunder_version_len = sd_strlen(cmd._thunder_version);

	/* 合作伙伴ID */
	ret_val = get_partner_id(cmd._partner_id, MAX_PARTNER_ID_LEN-1); 
	CHECK_VALUE(ret_val);
	cmd._partner_id_len = sd_strlen(cmd._partner_id);

	/* Peer ID */
	cmd._peerid_len = PEER_ID_SIZE;
	ret_val=get_peerid(cmd._peerid, PEER_ID_SIZE);
	if(ret_val!=SUCCESS)
	{
		LOG_DEBUG("Error when getting peerid!");
		return ret_val;
	}

	/* file ID */
    
	cmd._fileid_len = FILE_ID_SIZE;
    emule_get_file_id(p_emule_task->_data_manager, (_u8*)cmd._fileid );

	/* aich hash */
    emule_get_aich_hash(p_emule_task->_data_manager, &cmd._aich_hash_ptr, &cmd._aich_hash_len );
    
	/* part hash */
    emule_get_part_hash(p_emule_task->_data_manager, &cmd._part_hash_ptr, &cmd._part_hash_len );

	/* cid */
    if((TRUE==emule_get_cid( p_emule_task->_data_manager, cmd._cid))
        &&(TRUE==sd_is_cid_valid(cmd._cid)))
    {
		cmd._cid_size = CID_SIZE;
    }
	else
	{
		LOG_DEBUG("emb_reporter_emule_stop_task..._p_task=0x%X,task_id=%u:Error:REPORTER_ERR_GET_CID!",_p_task,_p_task->_task_id);
		//return (REPORTER_ERR_GET_CID);
	}

	/* Gcid */
	if( ( TRUE==emule_get_gcid( p_emule_task->_data_manager, cmd._gcid)
        || TRUE==emule_get_calc_gcid( p_emule_task->_data_manager, cmd._gcid))
		&&(TRUE==sd_is_cid_valid(cmd._gcid)))
	{
		cmd._gcid_size = CID_SIZE;
	}
	else
	{
		LOG_DEBUG("emb_reporter_emule_stop_task..._p_task=0x%X,task_id=%u:Error:REPORTER_ERR_GET_GCID!",_p_task,_p_task->_task_id);
		//return (REPORTER_ERR_GET_GCID);
	}

	// 文件大小 
	cmd._file_size = _p_task->file_size;
    
	cmd._file_name_len = MAX_FILE_NAME_BUFFER_LEN;
    ret_val = emule_get_task_file_name( _p_task, cmd._file_name, (_int32*)&cmd._file_name_len );
    if( ret_val != SUCCESS )
    {
        cmd._file_name_len = 0;
    }
	
	cmd._task_type = 0;

	cmd._vod_play_time = p_report_para->_vod_play_time;
	cmd._vod_first_buffer_time = p_report_para->_vod_first_buffer_time;
	cmd._vod_interrupt_times = p_report_para->_vod_interrupt_times;
	cmd._min_interrupt_time = p_report_para->_min_interrupt_time;
	cmd._max_interrupt_time = p_report_para->_max_interrupt_time;
	cmd._avg_interrupt_time = p_report_para->_avg_interrupt_time;
	
	cmd._server_dl_bytes = server_dl_bytes;
	cmd._peer_dl_bytes = peer_dl_bytes;
	cmd._emule_dl_bytes = emule_dl_bytes;
	cmd._cdn_dl_bytes = cdn_dl_bytes;

	LOG_DEBUG("emb_reporter_emule_stop_task, _server_dl_bytes=%llu, peer_dl_bytes=%llu, emule_dl_bytes=%llu, cdn_dl_bytes=%llu",
		cmd._server_dl_bytes, cmd._peer_dl_bytes, cmd._emule_dl_bytes, cmd._cdn_dl_bytes );

	if( status == TASK_S_RUNNING )
	{
		cmd._task_status = 0;
	}
	else if( status == TASK_S_SUCCESS 
		|| status == TASK_S_VOD )
	{
		cmd._task_status = 1;
	}
	else if( status == TASK_S_FAILED )
	{
		cmd._task_status = 2;
	}
	
	cmd._failed_code = _p_task->failed_code;
	cmd._dl_percent = _p_task->progress;

//////////////////add new
//////////////////vod_report

	cmd._bit_rate = p_report_para->_bit_rate;
	cmd._vod_total_buffer_time = p_report_para->_vod_total_buffer_time;
	cmd._vod_total_drag_wait_time = p_report_para->_vod_total_drag_wait_time;
	cmd._vod_drag_num = p_report_para->_vod_drag_num;
	
	cmd._play_interrupt_1 = p_report_para->_play_interrupt_1;
	cmd._play_interrupt_2 = p_report_para->_play_interrupt_2;
	cmd._play_interrupt_3 = p_report_para->_play_interrupt_3;
	cmd._play_interrupt_4 = p_report_para->_play_interrupt_4;
	cmd._play_interrupt_5 = p_report_para->_play_interrupt_5;
	cmd._play_interrupt_6 = p_report_para->_play_interrupt_6;
	
	cmd._cdn_stop_times = p_report_para->_cdn_stop_times;
	
	LOG_DEBUG("emb_reporter_emule_stop_task, vod_report:cmd._bit_rate=%u, cmd._vod_total_buffer_time=%u, cmd._vod_total_drag_wait_time=%u, cmd._vod_drag_num=%u, \
		cmd._play_interrupt_1=%u, cmd._play_interrupt_2=%u, cmd._play_interrupt_3=%u, cmd._play_interrupt_4=%u, cmd._play_interrupt_5=%u, cmd._play_interrupt_6=%u,\
		cmd._cdn_stop_times=%u",
		cmd._bit_rate, cmd._vod_total_buffer_time, cmd._vod_total_drag_wait_time, cmd._vod_drag_num, 
		cmd._play_interrupt_1, cmd._play_interrupt_2, cmd._play_interrupt_3, cmd._play_interrupt_4, 
		cmd._play_interrupt_5, cmd._play_interrupt_6, cmd._cdn_stop_times);


//////file_info_report
	p_file_info_report_para = emule_get_report_para(p_emule_task->_data_manager);
	if( p_file_info_report_para == NULL )
	{
		sd_assert( FALSE );	
		return SUCCESS;
	}

	cmd._down_exist = p_file_info_report_para->_down_exist;
	cmd._overlap_bytes = p_file_info_report_para->_overlap_bytes;
	
	cmd._down_n2i_no = p_file_info_report_para->_down_n2i_no;
	cmd._down_n2i_yes = p_file_info_report_para->_down_n2i_yes;
	cmd._down_n2n_no = p_file_info_report_para->_down_n2n_no;
	cmd._down_n2n_yes = p_file_info_report_para->_down_n2n_yes;
	cmd._down_n2s_no = p_file_info_report_para->_down_n2s_no;
	cmd._down_n2s_yes = p_file_info_report_para->_down_n2s_yes;
	cmd._down_i2i_no = p_file_info_report_para->_down_i2i_no;
	cmd._down_i2i_yes = p_file_info_report_para->_down_i2i_yes;
	cmd._down_i2n_no = p_file_info_report_para->_down_i2n_no;
	cmd._down_i2n_yes = p_file_info_report_para->_down_i2n_yes;
	
	cmd._down_by_partner_cdn = p_file_info_report_para->_down_by_partner_cdn;
	cmd._down_err_by_cdn = p_file_info_report_para->_down_err_by_cdn;
	cmd._down_err_by_partner_cdn = p_file_info_report_para->_down_err_by_partner_cdn;
	cmd._down_err_by_peer = p_file_info_report_para->_down_err_by_peer;
	cmd._down_err_by_svr = p_file_info_report_para->_down_err_by_svr;

	LOG_DEBUG("emb_reporter_emule_stop_task, file_info_report:cmd._down_exist:%llu, cmd._overlap_bytes:%llu,\
		cmd._down_n2i_no:%llu, cmd._down_n2i_yes:%llu, cmd._down_n2n_no:%llu, cmd._down_n2n_yes:%llu, cmd._down_n2s_no:%llu,cmd._down_n2s_yes:%llu, cmd._down_i2i_no:%llu, cmd._down_i2i_yes:%llu, cmd._down_i2n_no:%llu, cmd._down_i2n_yes:%llu,\
		cmd._down_by_partner_cdn:%llu, cmd._down_err_by_cdn:%llu, cmd._down_err_by_partner_cdn:%llu, cmd._down_err_by_peer:%llu, cmd._down_err_by_svr:%llu, ",\
		cmd._down_exist, cmd._overlap_bytes, cmd._down_n2i_no, cmd._down_n2i_yes, cmd._down_n2n_no, cmd._down_n2n_yes,
		cmd._down_n2s_no, cmd._down_n2s_yes, cmd._down_i2i_no, cmd._down_i2i_yes, cmd._down_i2n_no, cmd._down_i2n_yes,
		cmd._down_by_partner_cdn, cmd._down_err_by_cdn, cmd._down_err_by_partner_cdn, cmd._down_err_by_peer, cmd._down_err_by_svr);
	
//////cm_report
	p_cm_report_para = cm_get_report_para( &_p_task->_connect_manager, 0 );
	if( p_cm_report_para == NULL )
	{
		sd_assert( FALSE );	
		return SUCCESS;
	}	
	
	cmd._res_server_total = p_cm_report_para->_res_server_total;
	cmd._res_server_valid = p_cm_report_para->_res_server_valid;
	cmd._res_cdn_total = p_cm_report_para->_res_cdn_total;
	cmd._res_cdn_valid = p_cm_report_para->_res_cdn_valid;
	cmd._res_partner_cdn_total = p_cm_report_para->_res_partner_cdn_total;
	cmd._res_partner_cdn_valid = p_cm_report_para->_res_partner_cdn_valid;	
	
	cmd._cdn_res_max_num = p_cm_report_para->_cdn_res_max_num;
	cmd._cdn_connect_failed_times = p_cm_report_para->_cdn_connect_failed_times;
	
	cmd._res_n2i_total = p_cm_report_para->_res_n2i_total;
	cmd._res_n2i_valid = p_cm_report_para->_res_n2i_valid;
	cmd._res_n2n_total = p_cm_report_para->_res_n2n_total;
	cmd._res_n2n_valid = p_cm_report_para->_res_n2n_valid;
	cmd._res_n2s_total = p_cm_report_para->_res_n2s_total;
	cmd._res_n2s_valid = p_cm_report_para->_res_n2s_valid;
	cmd._res_i2i_total = p_cm_report_para->_res_i2i_total;
	cmd._res_i2i_valid = p_cm_report_para->_res_i2i_valid;
	cmd._res_i2n_total = p_cm_report_para->_res_i2n_total;
	cmd._res_i2n_valid = p_cm_report_para->_res_i2n_valid;

	cmd._hub_res_total = p_cm_report_para->_hub_res_total;
	cmd._hub_res_valid = p_cm_report_para->_hub_res_valid;
	cmd._active_tracker_res_total = p_cm_report_para->_active_tracker_res_total;
	cmd._active_tracker_res_valid = p_cm_report_para->_active_tracker_res_valid;

	LOG_DEBUG("emb_reporter_emule_stop_task, cm_report :cmd._res_server_total=%u, cmd._res_server_valid=%u, cmd._res_cdn_total=%u, cmd._res_cdn_valid=%u,cmd._res_partner_cdn_total=%u, cmd._res_partner_cdn_valid=%u,\
		cmd._cdn_res_max_num=%u, cmd._cdn_connect_failed_times=%u, \
		cmd._res_n2i_total=%u, cmd._res_n2i_valid=%u, cmd._res_n2n_total=%u, cmd._res_n2n_valid=%u, cmd._res_n2s_total=%u, cmd._res_n2s_valid=%u, cmd._res_i2i_total=%u, cmd._res_i2i_valid=%u, cmd._res_i2n_total=%u, cmd._res_i2n_valid=%u,\
		cmd._hub_res_total=%u, cmd._hub_res_valid=%u, cmd._active_tracker_res_total=%u, cmd._active_tracker_res_valid=%u, ",\
		cmd._res_server_total, cmd._res_server_valid, cmd._res_cdn_total, cmd._res_cdn_valid, cmd._res_partner_cdn_total, cmd._res_partner_cdn_valid,
		cmd._cdn_res_max_num, cmd._cdn_connect_failed_times, cmd._res_n2i_total, cmd._res_n2i_valid, 
		cmd._res_n2n_total, cmd._res_n2n_valid, cmd._res_n2s_total, cmd._res_n2s_valid, cmd._res_i2i_total, 
		cmd._res_i2i_valid, cmd._res_i2n_total, cmd._res_i2n_valid, cmd._hub_res_total, cmd._hub_res_valid,
		cmd._active_tracker_res_total, cmd._active_tracker_res_valid);
	

//////res_query_report
	p_res_query_report_para = dt_get_res_query_report_para(_p_task);
	if( p_res_query_report_para == NULL )
	{
		sd_assert( FALSE );	
		return SUCCESS;
	}	

	cmd._hub_s_max = p_res_query_report_para->_hub_s_max;
	cmd._hub_s_min = p_res_query_report_para->_hub_s_min;
	cmd._hub_s_avg = p_res_query_report_para->_hub_s_avg;
	cmd._hub_s_succ = p_res_query_report_para->_hub_s_succ;
	cmd._hub_s_fail = p_res_query_report_para->_hub_s_fail;

	cmd._hub_p_max = p_res_query_report_para->_hub_p_max;
	cmd._hub_p_min = p_res_query_report_para->_hub_p_min;
	cmd._hub_p_avg = p_res_query_report_para->_hub_p_avg;
	cmd._hub_p_succ = p_res_query_report_para->_hub_p_succ;
	cmd._hub_p_fail = p_res_query_report_para->_hub_p_fail;

	cmd._hub_t_max = p_res_query_report_para->_hub_t_max;
	cmd._hub_t_min = p_res_query_report_para->_hub_t_min;
	cmd._hub_t_avg = p_res_query_report_para->_hub_t_avg;
	cmd._hub_t_succ = p_res_query_report_para->_hub_t_succ;
	cmd._hub_t_fail = p_res_query_report_para->_hub_t_fail;

#if defined(MOBILE_PHONE)
	cmd._network_info = sd_get_net_type();
#endif
	
	LOG_DEBUG("emb_reporter_emule_stop_task, res_query_report :\
		cmd._hub_s_max=%u, cmd._hub_s_min=%u,cmd._hub_s_avg=%u, cmd._hub_s_succ=%u, cmd._hub_s_fail=%u, \
		cmd._hub_p_max=%u, cmd._hub_p_min=%u,cmd._hub_p_avg=%u, cmd._hub_p_succ=%u, cmd._hub_p_fail=%u, \
		cmd._hub_t_max=%u, cmd._hub_t_min=%u, cmd._hub_t_avg=%u, cmd._hub_t_succ=%u, cmd._hub_t_fail=%u",\
		cmd._hub_s_max, cmd._hub_s_min, cmd._hub_s_avg, cmd._hub_s_succ, cmd._hub_s_fail, 
		cmd._hub_p_max, cmd._hub_p_min, cmd._hub_p_avg, cmd._hub_p_succ, cmd._hub_p_fail,
		cmd._hub_t_max, cmd._hub_t_min, cmd._hub_t_avg, cmd._hub_t_succ, cmd._hub_t_fail);


	LOG_DEBUG("emb_reporter_emule_stop_task cmd._server_dl_bytes:%llu,cmd._peer_dl_bytes:%llu,  cmd._cdn_dl_bytes :%llu",cmd._server_dl_bytes ,
		cmd._peer_dl_bytes , cmd._cdn_dl_bytes );

	//build command
	ret_val = emb_reporter_build_emule_stop_task_cmd(&buffer, &len, &cmd);
	CHECK_VALUE(ret_val);


	
	return reporter_commit_cmd(&g_embed_hub, EMB_REPORT_EMULE_STOP_TASK_CMD_TYPE, buffer, len, NULL, cmd._header._seq);			//commit a request
}

#endif



#endif


/*--------------------------------------------------------------------------*/
/*                Internal functions				        */
/*--------------------------------------------------------------------------*/
_int32 init_reporter_device(REPORTER_DEVICE* device,enum REPORTER_DEVICE_TYPE		_type)
{
	_int32 ret;
	sd_memset(device, 0, sizeof(REPORTER_DEVICE));
	device->_type = _type;
	device->_socket = INVALID_SOCKET;
	ret = sd_malloc(REPORTER_MAX_REC_BUFFER_LEN, (void**)&device->_recv_buffer);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("init_reporter_device, malloc failed.");
		CHECK_VALUE( ret);
	}
	device->_recv_buffer_len = REPORTER_MAX_REC_BUFFER_LEN;
	list_init(&device->_cmd_list);
	device->_timeout_id = INVALID_MSG_ID;
	return SUCCESS;
}

_int32 uninit_reporter_device(REPORTER_DEVICE* device)
{
	REPORTER_CMD* cmd;
	if(device->_timeout_id != INVALID_MSG_ID)
	{
		cancel_timer(device->_timeout_id);
		device->_timeout_id = INVALID_MSG_ID;
	}
	
	if(device->_socket != INVALID_SOCKET)
	{
		_u32 count = 0;
		socket_proxy_peek_op_count(device->_socket, DEVICE_SOCKET_TCP, &count);
		if(count > 0)
		{
			socket_proxy_cancel(device->_socket, DEVICE_SOCKET_TCP);
		}
		else
		{
			socket_proxy_close(device->_socket);
			device->_socket = INVALID_SOCKET;

		}	
		if(device->_last_cmd != NULL)
		{
			sd_free((char*)device->_last_cmd->_cmd_ptr);
			sd_free(device->_last_cmd);
			device->_last_cmd = NULL;
		}
		
	}

	sd_assert(device->_last_cmd == NULL);
	SAFE_DELETE(device->_recv_buffer);
	device->_recv_buffer = NULL;
	device->_recv_buffer_len = 0;
	
	/*free command list*/
	while(list_pop(&device->_cmd_list, (void**)&cmd) == SUCCESS)
	{
		if(cmd == NULL)
			break;
		sd_free((char*)cmd->_cmd_ptr);
		sd_free(cmd);
		cmd = NULL;
	}
	sd_assert(list_size(&device->_cmd_list) == 0);
	return SUCCESS;
}

_int32 reporter_get_license(char *buffer, _int32 bufsize)
{
/*
FOR 192.168.13.66:8888

raolinghe(饶凌河-IceRao) 16:49:50
License编号                               |序号   |有效期                |状态
0810100001099a951a5fcd4ad593a129815438ef39|0000000|2008-10-09┄2009-11-28|00
08101000019bbc734db05346e68b9d10d2ac499cbd|0000001|2008-10-09┄2009-11-28|00
0810100001f09e49a344b84489b1095ce8750f39f5|0000002|2008-10-09┄2009-11-28|00
08101000016b9163bcda8b4692b43d90facbc012f9|0000003|2008-10-09┄2009-11-28|00


FOR license.xunlei.com:8888

raolinghe(饶凌河-IceRao) 10:53:35
License编号                               |序号   |有效期                |状态
08083000021b29b27e57604f68bece019489747910|0000000|2008-08-13┄2009-08-06|00
08083000029d1e40353e81499599dc1f86f3c58fa7|0000001|2008-08-13┄2009-08-06|00
0808300002c75656b7e3a648a08c0d36d8d24d3445|0000002|2008-08-13┄2009-08-06|00

09092800030000000000003mxy9o03f3e58yv48i1j
批次 0909280003 + 序列号0000000 + 厂商编号000003 + 随机数 mxy9o03f3e58yv48i1j


*/
	//_int32 ret_val = SUCCESS;
	if((buffer==NULL)||(bufsize<MAX_LICENSE_LEN)) return -1;
	sd_memset(buffer, 0, MAX_LICENSE_LEN);
	sd_memcpy(buffer,DEFAULT_LICENSE, sizeof(DEFAULT_LICENSE));
	//sd_memcpy(buffer,"0810100001099a951a5fcd4ad593a129815438ef39", sizeof("0810100001099a951a5fcd4ad593a129815438ef39"));
	return settings_get_str_item("license.license", buffer);
}



/*
_int32 reporter_remove_cmd(REPORTER_DEVICE* device, DOWNLOAD_TASK* _p_task)
{	
	LIST_ITERATOR cur_iter = NULL;
	LIST_ITERATOR next_iter = NULL;
	REPORTER_CMD* cmd = NULL;
	if(device->_last_cmd != NULL && device->_last_cmd->_task == (void*)_p_task)
		device->_last_cmd->_task = NULL;
	cur_iter = LIST_BEGIN(device->_cmd_list);
	while(cur_iter != LIST_END(device->_cmd_list))
	{
		next_iter = LIST_NEXT(cur_iter);
		cmd = (REPORTER_CMD*)cur_iter->_data;
		if(cmd->_task == (void*)_p_task)
		{
			sd_free((char*)cmd->_cmd_ptr);
			sd_free(cmd);
			list_erase(&device->_cmd_list, cur_iter);
		}
		cur_iter = next_iter;
	}
	return SUCCESS;
}
*/


_int32 force_close_reporter_device_res(REPORTER_DEVICE* device)
{
         if(device->_socket != INVALID_SOCKET)
         {
                //socket_proxy_close(device->_socket);
		  device->_socket = INVALID_SOCKET;
         }

	  return SUCCESS;
}



static _int32 _reporter_get_task_common_stat(TASK *task, _u64 down_size, struct tagREPORT_TASK_COMMON_STAT * p_common_stat)
{
    _int32 ret_val, down_time;
    p_common_stat->_task_id=task->_extern_id;
	p_common_stat->_create_time=task->_create_time;
	
    p_common_stat->_start_time=task->_start_time;
	p_common_stat->_finish_time=task->_finished_time;

    if( task->task_status == TASK_S_SUCCESS 
        || ((task->_downloaded_data_size == task->file_size) && (task->file_size > 0)) )
    {
        p_common_stat->_task_status = 1;
    }
    else if( task->task_status == TASK_S_FAILED )
    {
        p_common_stat->_task_status = 2;
    }
    else
    {
        p_common_stat->_task_status = 0;
    }

	p_common_stat->_fail_code=task->failed_code;

	/* Peer ID */
	p_common_stat->_peerid_len = PEER_ID_SIZE;
	ret_val=get_peerid(p_common_stat->_peerid, PEER_ID_SIZE);
	if(SUCCESS != ret_val)
	{
		LOG_ERROR("Error when get_peerid!");
		return ret_val;
	}	

	/* 版本号 */
	ret_val = get_version(p_common_stat->_thunder_version, MAX_VERSION_LEN-1); 
	CHECK_VALUE(ret_val);
	p_common_stat->_thunder_version_len = sd_strlen(p_common_stat->_thunder_version);

	/* 合作伙伴ID */
	ret_val = get_partner_id(p_common_stat->_partner_id, MAX_PARTNER_ID_LEN-1); 
	CHECK_VALUE(ret_val);
	p_common_stat->_partner_id_len = sd_strlen(p_common_stat->_partner_id);

	p_common_stat->_product_flag =	get_product_flag();

	down_time=p_common_stat->_finish_time-p_common_stat->_start_time;
	if(down_time <= 0 )
	{
		down_time = 1;
	}
    p_common_stat->_avg_speed= down_size /down_time;

	p_common_stat->_first_zero_speed_duration=(_int32)task->_first_byte_arrive_time;
	p_common_stat->_other_zero_speed_duraiton=(_int32)(task->_zero_speed_time_ms/1000);
	if(0 == task->_download_finish_time)	//未下载完
	{
		p_common_stat->_percent100_duration = 0;
	}
	else
	{
		p_common_stat->_percent100_duration=task->_finished_time - task->_download_finish_time;  
		if(p_common_stat->_percent100_duration < 0)
		{
			p_common_stat->_percent100_duration = 0;
		}
	}
    p_common_stat->_percent = task->progress;

	return SUCCESS;
}

_int32 reporter_task_p2sp_stat(TASK *task)
{
	_int32 ret_val = SUCCESS;
	char *buffer = NULL , * p_url, *p_file_name;
	_u32  len = 0;

	EMB_REPORT_TASK_P2SP_STAT cmd;
	P2SP_TASK *p_p2sp_task = (P2SP_TASK *)task;
	
	LOG_DEBUG("task = 0x%x, type=%d task_id = %u", task,task->_task_type, task->_task_id);

	sd_memset(&cmd, 0, sizeof(EMB_REPORT_TASK_P2SP_STAT));

    _u64 ser_bytes = 0, peer_bytes = 0, cdn_bytes = 0, lixian_bytes = 0, acc_bytes = 0, origin_bytes = 0, zero_cdn_bytes = 0, dl_bytes = 0;
    dm_get_dl_bytes( &(p_p2sp_task->_data_manager), &ser_bytes, &peer_bytes, &cdn_bytes, &lixian_bytes, &acc_bytes );
    zero_cdn_bytes = dm_get_normal_cdn_down_bytes(&p_p2sp_task->_data_manager);
    dl_bytes = ser_bytes + peer_bytes + cdn_bytes + lixian_bytes + zero_cdn_bytes;

	ret_val = _reporter_get_task_common_stat(task, dl_bytes, &cmd._common);
	if(SUCCESS != ret_val)
	{
		return ret_val;
	}

	cmd._task_create_type=p_p2sp_task->_task_created_type;

	if((TASK_CREATED_BY_URL == p_p2sp_task->_task_created_type)||
	   (TASK_CONTINUE_BY_URL == p_p2sp_task->_task_created_type))
	{
		/* originalUrl */
		p_url = NULL;
		if(p_p2sp_task->_origion_url_length > 0)
		{
			p_url = p_p2sp_task->_origion_url;
		}
		
		if(NULL != p_url ||  TRUE ==  dm_get_origin_url(&(p_p2sp_task->_data_manager), &p_url))
		{
			/* remove user name and password and decode */
			url_object_to_noauth_string(p_url, cmd._url,sizeof(cmd._url));
			cmd._url_len = sd_strlen(cmd._url);
		}
		else
		{
			LOG_DEBUG("..._p_task=0x%X,task_id=%u: get url fail!",task,task->_task_id);
		}

		/* refererUrl */
		p_url=NULL;
		if(TRUE ==  dm_get_origin_ref_url(&(p_p2sp_task->_data_manager), &p_url))
		{
			sd_strncpy(cmd._ref_url, p_url,sizeof(cmd._ref_url)-1);
			cmd._ref_url_len = sd_strlen(cmd._ref_url);
		}
	}

	if((TRUE==dm_get_cid( &(p_p2sp_task->_data_manager), cmd._cid))
		&&(TRUE==sd_is_cid_valid(cmd._cid)))
	{
		cmd._cid_size = CID_SIZE;
	}
	else
	{
	   cmd._cid_size=0;
	}

	if((TRUE==dm_get_calc_gcid( &(p_p2sp_task->_data_manager), cmd._gcid))
		&&(TRUE==sd_is_cid_valid(cmd._gcid)))
	{
		cmd._gcid_size = CID_SIZE;
	}else if((TRUE==dm_get_shub_gcid( &(p_p2sp_task->_data_manager), cmd._gcid))
		&&(TRUE==sd_is_cid_valid(cmd._gcid)))
	{
		cmd._gcid_size = CID_SIZE;
	}
	else
	{
		cmd._gcid_size = 0;
	}

	cmd._file_size = dm_get_file_size(&(p_p2sp_task->_data_manager));

	if(TRUE==dm_get_filnal_file_name( &(p_p2sp_task->_data_manager),&p_file_name))
	{
		sd_strncpy(cmd._file_name, p_file_name,sizeof(cmd._file_name)-1);
		cmd._file_name_len = sd_strlen(cmd._file_name);
	}
	else
	{
		LOG_DEBUG("..._p_task=0x%X,task_id=%u: get filename fail!",task,task->_task_id);
	}

    dm_get_origin_resource_dl_bytes(&(p_p2sp_task->_data_manager),&origin_bytes);
    cmd._origin_bytes = origin_bytes;
	cmd._other_server_bytes = ser_bytes-origin_bytes;
	cmd._peer_dl_bytes = peer_bytes;
	cmd._speedup_cdn_bytes = cdn_bytes;
	cmd._lixian_cdn_bytes = lixian_bytes;
	cmd._zero_cdn_dl_bytes = zero_cdn_bytes;

	cm_get_normal_cdn_stat_para(&(task->_connect_manager), FALSE,
		(_u32 *)&(cmd._zero_cdn_use_time), 
		(_u32 *)&(cmd._zero_cdn_count), 
		(_u32 *)&(cmd._zero_cdn_valid_count),
		(_u32 *)&(cmd._first_use_zero_cdn_duration));

	/*build command*/
	_u32 seqno=0;
	ret_val = reporter_build_report_task_p2sp_stat_cmd(&buffer, &len, &cmd,&seqno);
	CHECK_VALUE(ret_val);

	return reporter_commit_cmd(&g_stat_hub, EMB_REPORT_P2SP_DOWNLOAD_STAT_CMD_TYPE, buffer, len, NULL, seqno);
}

#ifdef ENABLE_EMULE
_int32 reporter_task_emule_stat(TASK *task)
{
	_int32 ret_val = SUCCESS;
	char *buffer = NULL ;
	_u32  len = 0;

	EMB_REPORT_TASK_EMULE_STAT cmd;
	EMULE_TASK *p_emule_task = (EMULE_TASK *)task;

	CONNECT_MANAGER * p_sub_conn_manager = NULL;
	
	LOG_DEBUG("task = 0x%x, type=%d task_id = %u", task,task->_task_type, task->_task_id);

	sd_memset(&cmd, 0, sizeof(EMB_REPORT_TASK_EMULE_STAT));

    _u64 ser_bytes = 0, peer_bytes = 0, cdn_bytes = 0,emule_dl_bytes=0, lixian_bytes = 0, zero_cdn_dl_bytes = 0, dl_bytes  = 0;
    emule_get_dl_bytes(p_emule_task->_data_manager, &ser_bytes, &peer_bytes, &emule_dl_bytes, &cdn_bytes , &lixian_bytes);
    emule_get_normal_cdn_down_bytes(p_emule_task->_data_manager, &zero_cdn_dl_bytes );
    dl_bytes = ser_bytes+peer_bytes+cdn_bytes+emule_dl_bytes+lixian_bytes+zero_cdn_dl_bytes;
	ret_val = _reporter_get_task_common_stat(task, dl_bytes, &cmd._common);
	if(SUCCESS != ret_val)
	{
		return ret_val;
	}

	cmd._fileid_len = FILE_ID_SIZE;
    emule_get_file_id(p_emule_task->_data_manager, (_u8*)cmd._fileid );
	if( (TRUE == emule_get_shub_cid(p_emule_task->_data_manager, cmd._cid)) ||
		(TRUE == emule_get_cid(p_emule_task->_data_manager, cmd._cid))    )
	{
		cmd._cid_size = CID_SIZE;
	}
	if (TRUE == emule_get_gcid(p_emule_task->_data_manager, cmd._gcid))
	{
		cmd._gcid_size = CID_SIZE;
	}
	cmd._file_size = emule_get_file_size(p_emule_task->_data_manager);	//task->_file_size;

    cmd._emule_dl_bytes = emule_dl_bytes;
	cmd._server_dl_bytes = ser_bytes;
	cmd._peer_dl_bytes = peer_bytes;
	cmd._speedup_cdn_bytes = cdn_bytes;
	cmd._lixian_cdn_bytes = lixian_bytes;
	cmd._zero_cdn_dl_bytes = zero_cdn_dl_bytes;
	
	ret_val = cm_get_sub_connect_manager(&(task->_connect_manager), 0, &p_sub_conn_manager);
	CHECK_VALUE(ret_val);

	cm_get_normal_cdn_stat_para(p_sub_conn_manager, FALSE,
		(_u32 *)&(cmd._zero_cdn_use_time), 
		(_u32 *)&(cmd._zero_cdn_count), 
		(_u32 *)&(cmd._zero_cdn_valid_count),
		(_u32 *)&(cmd._first_use_zero_cdn_duration));

	/*build command*/
	_u32 seqno=0;
	ret_val = reporter_build_report_task_emule_stat_cmd(&buffer, &len, &cmd,&seqno);
	CHECK_VALUE(ret_val);

	return reporter_commit_cmd(&g_stat_hub, EMB_REPORT_EMULE_DOWNLOAD_STAT_CMD_TYPE, buffer, len, NULL, seqno);
}
#endif

_int32 reporter_task_bt_stat(TASK *task)
{
	_int32 ret_val = SUCCESS;
	char *buffer = NULL ;
	unsigned char* p_info_hash=NULL;
	_u32  len = 0;

	EMB_REPORT_TASK_BT_STAT cmd;
	BT_TASK *p_bt_task = (BT_TASK *)task;
	
	LOG_DEBUG("task = 0x%x, type=%d task_id = %u", task,task->_task_type, task->_task_id);

	sd_memset(&cmd, 0, sizeof(EMB_REPORT_TASK_BT_STAT));

    _u64 ser_bytes = 0, peer_bytes = 0, cdn_bytes = 0,origion_bytes=0, lixian_bytes = 0,zero_cdn_bytes = 0 ,dl_bytes  = 0;
    bdm_get_dl_bytes(&p_bt_task->_data_manager, &ser_bytes, &peer_bytes,  &cdn_bytes ,&origion_bytes);
	zero_cdn_bytes=bdm_get_normal_cdn_down_bytes(&p_bt_task->_data_manager);
    dl_bytes = ser_bytes+peer_bytes+cdn_bytes+origion_bytes+zero_cdn_bytes;
    ret_val = _reporter_get_task_common_stat(task,dl_bytes,&cmd._common);
	if(SUCCESS != ret_val)
	{
		return ret_val;
	}

	tp_get_file_info_hash(p_bt_task->_torrent_parser_ptr, &p_info_hash );
	cmd._info_id_size = INFO_HASH_LEN;
	sd_memcpy(cmd._info_id, p_info_hash,INFO_HASH_LEN);

	cmd._file_number= tp_get_seed_file_num(p_bt_task->_torrent_parser_ptr );
	
	cmd._file_total_size = tp_get_file_total_size(p_bt_task->_torrent_parser_ptr );

	cmd._piece_size = tp_get_piece_size(p_bt_task->_torrent_parser_ptr );

	cmd._server_dl_bytes = ser_bytes;
	cmd._bt_dl_bytes = origion_bytes;
	cmd._peer_dl_bytes = peer_bytes;
	cmd._speedup_cdn_bytes = cdn_bytes;
	cmd._lixian_cdn_bytes = lixian_bytes;
	cmd._zero_cdn_dl_bytes=zero_cdn_bytes;

	struct tagCM_REPORT_PARA *report_para;
	report_para = cm_get_report_para(&(task->_connect_manager), INVALID_FILE_INDEX);
	sd_assert(NULL != report_para);
	cm_get_normal_cdn_stat_para(&(task->_connect_manager), TRUE,
		(_u32 *)&(cmd._zero_cdn_use_time), 
		(_u32 *)&(cmd._zero_cdn_count), 
		(_u32 *)&(cmd._zero_cdn_valid_count),
		(_u32 *)&(cmd._first_use_zero_cdn_duration));

	/*build command*/
	_u32 seqno=0;
	ret_val = reporter_build_report_task_bt_stat_cmd(&buffer, &len, &cmd,&seqno);
	CHECK_VALUE(ret_val);

	return reporter_commit_cmd(&g_stat_hub, EMB_REPORT_BT_DOWNLOAD_STAT_CMD_TYPE, buffer, len, NULL, seqno);
}

_int32 reporter_task_bt_file_stat(TASK* task, _u32 file_index)
{
    _int32 ret_val = SUCCESS ,duration;
    char *buffer = NULL;
    _u32  len = 0;
    EMB_REPORT_TASK_BT_FILE_STAT cmd;
    struct tagREPORT_TASK_COMMON_STAT * p_common_stat=&cmd._common;

    BT_TASK * p_bt_task = (BT_TASK *)task;
    _u8 *info_hash=NULL;
    TORRENT_FILE_INFO *p_file_info=NULL;
    enum BT_FILE_STATUS file_status = BT_FILE_IDLE;

    _u64 server_dl_bytes=0,bt_dl_bytes=0,cdn_dl_bytes=0,lixian_cdn_dl_bytes=0,peer_dl_bytes=0,zero_cdn_dl_bytes=0,dl_bytes=0;

    _u32 first_zero_speed_duration =0, other_zero_speed_duration=0 ,percent100_time=0;

	CONNECT_MANAGER *p_sub_conn_manager = NULL;

	LOG_DEBUG("task = 0x%x, type=%d task_id = %u file_index=%d ", task,task->_task_type, task->_task_id,file_index);	
	
    sd_memset(&cmd, 0, sizeof(EMB_REPORT_TASK_BT_FILE_STAT));

	p_common_stat->_task_id = task->_extern_id;
	p_common_stat->_create_time = task->_create_time;

    p_common_stat->_start_time = bdm_get_sub_task_start_time(&(p_bt_task->_data_manager), file_index);
    sd_time((_u32*)&p_common_stat->_finish_time);

    file_status = bdm_get_file_status(&(p_bt_task->_data_manager), file_index);
    if( file_status == BT_FILE_DOWNLOADING 
        && !bdm_is_waited_success_close_file(&(p_bt_task->_data_manager), file_index))
    {
        p_common_stat->_task_status = 0;   // 下载中暂停
    }
    else if( file_status == BT_FILE_FINISHED
        || file_status == BT_FILE_DOWNLOADING )
    {
        p_common_stat->_task_status = 1;    // 下载成功
    }
    else if( file_status == BT_FILE_FAILURE )
    {
        p_common_stat->_task_status = 2;
    }

    p_common_stat->_fail_code= bdm_get_file_err_code(&(p_bt_task->_data_manager), file_index);

	/* Peer ID */
	p_common_stat->_peerid_len = PEER_ID_SIZE;
	ret_val=get_peerid(p_common_stat->_peerid, PEER_ID_SIZE);
	CHECK_VALUE(ret_val);

	/* 版本号 */
	ret_val = get_version(p_common_stat->_thunder_version, MAX_VERSION_LEN-1); 
	CHECK_VALUE(ret_val);
	p_common_stat->_thunder_version_len = sd_strlen(p_common_stat->_thunder_version);

	/* 合作伙伴ID */
	ret_val = get_partner_id(p_common_stat->_partner_id, MAX_PARTNER_ID_LEN-1); 
	CHECK_VALUE(ret_val);
	p_common_stat->_partner_id_len = sd_strlen(p_common_stat->_partner_id);

	p_common_stat->_product_flag = get_product_flag();

    /* dl bytes */
    bdm_get_sub_file_dl_bytes(&(p_bt_task->_data_manager), file_index, 
               &server_dl_bytes, &peer_dl_bytes, &cdn_dl_bytes, &lixian_cdn_dl_bytes  , &bt_dl_bytes);

    zero_cdn_dl_bytes = bdm_get_sub_file_normal_cdn_bytes(&(p_bt_task->_data_manager), file_index);

    dl_bytes = server_dl_bytes + peer_dl_bytes + bt_dl_bytes + cdn_dl_bytes + lixian_cdn_dl_bytes + zero_cdn_dl_bytes;

    LOG_DEBUG("reporter_task_bt_file_stat...server_dl_bytes=%llu, peer_dl_bytes=%llu,cdn_dl_bytes=%llu,lixian_dl_bytes = %llu, bt_dl_bytes=%llu, zero_cdn_dl_bytes = %llu, sum_dl_bytes=%llu",
        server_dl_bytes, peer_dl_bytes, cdn_dl_bytes, lixian_cdn_dl_bytes, bt_dl_bytes, zero_cdn_dl_bytes, dl_bytes);

    /* avg speed */
    duration = p_common_stat->_finish_time - p_common_stat->_start_time;

    if(duration <= 0)
    {
        p_common_stat->_avg_speed = 0;
    }
    else
    {
        p_common_stat->_avg_speed = dl_bytes / duration;
    }

    // 
    bdm_get_sub_file_stat_data(&p_bt_task->_data_manager
        , file_index
        , &first_zero_speed_duration
        
        , &other_zero_speed_duration
        , &percent100_time);

    p_common_stat->_first_zero_speed_duration = first_zero_speed_duration;
    p_common_stat->_other_zero_speed_duraiton = other_zero_speed_duration;

	if(0 != percent100_time )
	{
	    p_common_stat->_percent100_duration =p_common_stat->_finish_time - percent100_time;
	    if (p_common_stat->_percent100_duration < 0)
	    {
	        p_common_stat->_percent100_duration = 0;
	    }
	}

    p_common_stat->_percent = bdm_get_sub_file_percent(&(p_bt_task->_data_manager), file_index);
 
    /* info_hash */
    ret_val = tp_get_file_info_hash(  p_bt_task->_torrent_parser_ptr, &info_hash );
    CHECK_VALUE(ret_val);
    sd_memcpy(cmd._info_id, info_hash,INFO_HASH_LEN);
    cmd._info_id_size = INFO_HASH_LEN;

    cmd._file_index= file_index;

    ret_val = tp_get_file_info( p_bt_task->_torrent_parser_ptr,  file_index, &p_file_info );
    CHECK_VALUE(ret_val);


    if((TRUE==bdm_get_cid( &(p_bt_task->_data_manager), file_index,cmd._cid))
        &&(TRUE==sd_is_cid_valid(cmd._cid)))
    {
        cmd._cid_size = CID_SIZE;
    }

    if((TRUE==bdm_get_calc_gcid( &(p_bt_task->_data_manager),  file_index,cmd._gcid))
        &&(TRUE==sd_is_cid_valid(cmd._gcid)))
    {
        cmd._gcid_size = CID_SIZE;
    }else if((TRUE==bdm_get_shub_gcid( &(p_bt_task->_data_manager), file_index, cmd._gcid))
            &&(TRUE==sd_is_cid_valid(cmd._gcid)))
    {
        cmd._gcid_size = CID_SIZE;
    }

    cmd._file_size = p_file_info->_file_size;

    cmd._server_dl_bytes = server_dl_bytes;
    cmd._bt_dl_bytes= bt_dl_bytes;
    cmd._speedup_cdn_bytes = cdn_dl_bytes;
    cmd._lixian_cdn_bytes = lixian_cdn_dl_bytes;
    cmd._peer_dl_bytes = peer_dl_bytes;
    cmd._zero_cdn_dl_bytes = zero_cdn_dl_bytes;
	
	ret_val = cm_get_sub_connect_manager(&(task->_connect_manager), file_index, &p_sub_conn_manager);
	//CHECK_VALUE(ret_val);

	if (p_sub_conn_manager) {
		LOG_DEBUG("prepare to get sub connect manager:0x%x.", p_sub_conn_manager);
		cm_get_normal_cdn_stat_para(p_sub_conn_manager, FALSE,
			(_u32 *)&(cmd._zero_cdn_use_time), 
			(_u32 *)&(cmd._zero_cdn_count), 
			(_u32 *)&(cmd._zero_cdn_valid_count),
			(_u32 *)&(cmd._first_use_zero_cdn_duration));
	}

    /*build command*/
	_u32 seqno=0;	
    ret_val = reporter_build_report_task_bt_file_stat_cmd(&buffer, &len, &cmd,&seqno);
    CHECK_VALUE(ret_val);
    return reporter_commit_cmd(&g_stat_hub, EMB_REPORT_BT_SUBTASK_DOWNLOAD_STAT_CMD_TYPE, buffer, len, NULL, seqno);			//commit a request
}

_int32 report_task_create(TASK* task, _u32 file_index)
{
	_int32 ret_val = SUCCESS;
    char *buffer = NULL;
    _u32  len = 0, flag=0;
    EMB_REPORT_TASK_CREATE_STAT cmd;
    
	LOG_DEBUG("report_task_create, task:%u", task);

    sd_memset(&cmd, 0, sizeof(EMB_REPORT_TASK_CREATE_STAT));

    cmd._task_id = task->_extern_id;  
    cmd._create_time = task->_create_time;
    cmd._file_index = file_index;

	// TODO 这个字段感觉意义不大，暂时不填写。
	cmd._task_status = 3;

	cmd._peerid_len = PEER_ID_SIZE;
	ret_val = get_peerid(cmd._peerid, PEER_ID_SIZE);
	if(SUCCESS != ret_val)
	{
		LOG_ERROR("Error when get_peerid!");
		return ret_val;
	}	

	ret_val = get_version(cmd._thunder_version, MAX_VERSION_LEN-1); 
	CHECK_VALUE(ret_val);
	cmd._thunder_version_len = sd_strlen(cmd._thunder_version);

	ret_val = get_partner_id(cmd._partner_id, MAX_PARTNER_ID_LEN-1); 
	CHECK_VALUE(ret_val);
	cmd._partner_id_len = sd_strlen(cmd._partner_id);

	cmd._product_flag = get_product_flag();

    cmd._task_type = task->_task_type;

	switch(cmd._task_type)
	{
#ifdef ENABLE_EMULE
		case EMULE_TASK_TYPE:
			{
				EMULE_TASK *p_emule_task = (EMULE_TASK *)task;
				cmd._create_info_len= FILE_ID_SIZE;
				emule_get_file_id(p_emule_task->_data_manager, (_u8*)cmd._create_info );
				break;
			}
#endif
		case BT_TASK_TYPE:
			{
				BT_TASK *p_bt_task = (BT_TASK *)task;
				unsigned char * p_info_hash=NULL;
				tp_get_file_info_hash(p_bt_task->_torrent_parser_ptr, &p_info_hash );
				cmd._create_info_len = INFO_HASH_LEN;
				sd_memcpy(cmd._create_info, p_info_hash,INFO_HASH_LEN);
				break;
			}
		case P2SP_TASK_TYPE:
			{
				P2SP_TASK *p_p2sp_task = (P2SP_TASK *)task;
				char * p_url;
				switch(p_p2sp_task->_task_created_type)
				{
					case TASK_CREATED_BY_URL:
					case TASK_CONTINUE_BY_URL:
						/* originalUrl */
						p_url = NULL;
						if(p_p2sp_task->_origion_url_length > 0)
						{
							p_url = p_p2sp_task->_origion_url;
						}
						
						if(NULL != p_url ||  TRUE ==  dm_get_origin_url(&(p_p2sp_task->_data_manager), &p_url))
						{
							/* remove user name and password and decode */
							url_object_to_noauth_string(p_url, (char*)cmd._create_info,sizeof(cmd._create_info));
							cmd._create_info_len = sd_strlen((char*)cmd._create_info);
						}
						else
						{
							LOG_DEBUG("..._p_task=0x%X,task_id=%u: get url fail!",task,task->_task_id);
						}
						flag=1;
						break;
#if 0						
					case TASK_CREATED_BY_TCID:
					case TASK_CONTINUE_BY_TCID:	
						if((TRUE==dm_get_cid( &(p_p2sp_task->_data_manager), cmd._create_info))
							&&(TRUE==sd_is_cid_valid(cmd._create_info)))
						{
							cmd._create_info_len = CID_SIZE;
						}
						else
						{
						   cmd._create_info_len=0; 
						}
						break;
					case TASK_CREATED_BY_GCID:
						if((TRUE==dm_get_calc_gcid( &(p_p2sp_task->_data_manager), cmd._create_info))
							&&(TRUE==sd_is_cid_valid(cmd._create_info)))
						{
							cmd._create_info_len = CID_SIZE;
						}else if((TRUE==dm_get_shub_gcid( &(p_p2sp_task->_data_manager), cmd._create_info))
							&&(TRUE==sd_is_cid_valid(cmd._create_info)))
						{
							cmd._create_info_len = CID_SIZE;
						}
						else
						{
							cmd._create_info_len = 0;
						}
						break;
#endif						
					default:
						break;
				}//end  p_p2sp_task->_task_created_type
				break;
			}
		default:
				break;
	}
		

    /*build command*/
	_u32 seqno=0;
    ret_val = reporter_build_report_task_create_stat_cmd(&buffer, &len, &cmd,&seqno,flag);
    CHECK_VALUE(ret_val);
    return reporter_commit_cmd(&g_stat_hub, EMB_REPORT_TASK_CREATE_STAT_CMD_TYPE, buffer, len, NULL, seqno);
}

#ifdef UPLOAD_ENABLE
_int32 report_upload_stat(_u32	_begin_time, _u32	_end_time , _u32 up_duration, _u32 up_use_duration, _u64 up_data_bytes,
	                         _u32 up_pipe_num ,_u32	 up_passive_pipe_num, _u32  * p_seqno)
{
	_int32 ret_val = SUCCESS;
    char *buffer = NULL;
    _u32  len = 0 ;

	EMB_REPORT_UPLOAD_STAT cmd,* p_common_stat=&cmd;

	/* Peer ID */
	p_common_stat->_peerid_len = PEER_ID_SIZE;
	ret_val=get_peerid(p_common_stat->_peerid, PEER_ID_SIZE);
	if(SUCCESS != ret_val)
	{
		LOG_ERROR("Error when get_peerid!");
		return ret_val;
	}	

	/* 版本号 */
	ret_val = get_version(p_common_stat->_thunder_version, MAX_VERSION_LEN-1); 
	CHECK_VALUE(ret_val);
	p_common_stat->_thunder_version_len = sd_strlen(p_common_stat->_thunder_version);

	/* 合作伙伴ID */
	ret_val = get_partner_id(p_common_stat->_partner_id, MAX_PARTNER_ID_LEN-1); 
	CHECK_VALUE(ret_val);
	p_common_stat->_partner_id_len = sd_strlen(p_common_stat->_partner_id);

	p_common_stat->_product_flag =	get_product_flag();

	cmd._begin_time = _begin_time;
	cmd._end_time = _end_time;
	cmd._peer_capacity = get_peer_capability();
	cmd._up_duration = up_duration;
	cmd._up_use_duration= up_use_duration;
	cmd._up_data_bytes = up_data_bytes;
	cmd._up_pipe_num = up_pipe_num;
	cmd._up_passive_pipe_num = up_passive_pipe_num;

	/*build command*/
	_u32 seqno=0;
	ret_val = reporter_build_report_upload_stat_cmd(&buffer, &len, &cmd,&seqno);
	CHECK_VALUE(ret_val);
	*p_seqno=seqno;
	return reporter_commit_cmd(&g_stat_hub, EMB_REPORT_UPLOAD_STAT_CMD_TYPE, buffer, len, 0x11223344, seqno);
}
#endif
