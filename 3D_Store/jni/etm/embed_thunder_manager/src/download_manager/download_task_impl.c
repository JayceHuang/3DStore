/*--------------------------------------------------------------------------*/
/*                               IDENTIFICATION                             */
/*--------------------------------------------------------------------------*/
/*     Filename  : download_task.c                                         */
/*     Author     : ZengYuqing                                              */
/*     Project    : EmbedThunder                                        */
/*     Module     : download_task													*/
/*     Version    : 1.3  													*/
/*--------------------------------------------------------------------------*/
/*                  Shenzhen XunLei Networks			                    */
/*--------------------------------------------------------------------------*/
/*                  - C (copyright) - www.xunlei.com -		     		    */
/*                                                                          */
/*      This document is the proprietary of XunLei                          */
/*                                                                          */
/*      All rights reserved. Integral or partial reproduction               */
/*      prohibited without a written authorization from the                 */
/*      permanent of the author rights.                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
/*                         FUNCTIONAL DESCRIPTION                           */
/*--------------------------------------------------------------------------*/
/* This file contains the procedure of download_task                       */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*                              HISTORY                                     */
/*--------------------------------------------------------------------------*/
/*   Date     |    Author   | Modification                                  */
/*--------------------------------------------------------------------------*/
/* 2008.09.16 | ZengYuqing  | Creation                                      */
/*--------------------------------------------------------------------------*/
/* 2009.03.19 | ZengYuqing  | update to version 1.3                                      */
/*--------------------------------------------------------------------------*/

#include "torrent_parser/torrent_parser_interface.h"

#include "download_manager/download_task_data.h"
#include "download_manager/download_task_interface.h"
#include "download_manager/download_task_inner.h"
#include "download_manager/download_task_impl.h"
#include "download_manager/download_task_store.h"
#include "vod_task/vod_task.h"

#include "et_interface/et_interface.h"
#include "em_interface/em_fs.h"

#include "scheduler/scheduler.h"
#include "em_common/em_utility.h"
#include "em_common/em_mempool.h"
#include "em_common/em_time.h"

#include "em_common/em_logid.h"
#ifdef EM_LOGID
#undef EM_LOGID
#endif

#define EM_LOGID EM_LOGID_DOWNLOAD_TASK

#include "em_common/em_logger.h"

////////////////////////////////////

_int32 dt_start_task_impl(EM_TASK * p_task)
{
	EM_TASK_TYPE type;
	//TASK_INFO *p_task_info=NULL;

      EM_LOG_DEBUG("dt_start_task_impl:task_id=%u",p_task->_task_info->_task_id); 
	
	if((em_is_net_ok(TRUE)==FALSE)||(dt_is_running_task_full()==TRUE)||(em_is_license_ok()==FALSE))
	{
		dt_set_task_state(p_task,TS_TASK_WAITING);
		return NETWORK_INITIATING;
	}
	
	sd_assert(p_task->_inner_id==0);
	type=dt_get_task_type(p_task);
	sd_assert(dt_get_task_state(p_task)!=TS_TASK_SUCCESS);
	
	if(type==TT_BT)
	{
		return dt_start_bt_task(p_task);
	}
	else
	{
		return dt_start_p2sp_task(p_task);
	}
	
	return SUCCESS;
}
_int32 dt_start_bt_task(EM_TASK * p_task)
{
	_int32 ret_val = SUCCESS;
	TASK_INFO *p_task_info=NULL;
	EM_BT_TASK * p_bt_task = NULL;
	char * seed_file_full_path=NULL;
	char * file_path=NULL;
	_u32* download_file_index_array_32=NULL;
	_u16* download_file_index_array=NULL;
	enum ETI_ENCODING_SWITCH_MODE encoding_switch_mode=ET_ENCODING_DEFAULT_SWITCH;
	_u32* p_task_id=NULL;
	_u32 file_num = 0,i;
	char * user_data=NULL;
	_u32 user_data_len=0;

      EM_LOG_DEBUG("dt_start_bt_task:task_id=%u",p_task->_task_info->_task_id); 

	p_task_info = p_task->_task_info;
	file_num = p_task_info->_url_len_or_need_dl_num;
	if(p_task_info->_full_info)
	{
		p_bt_task = (EM_BT_TASK *)p_task->_task_info;
		file_path = p_bt_task->_file_path;
		seed_file_full_path = p_bt_task->_seed_file_path;
		download_file_index_array = p_bt_task->_dl_file_index_array;
	}
	else
	{
		file_path = dt_get_task_file_path_from_file(p_task);
		if(file_path==NULL)
		{
			CHECK_VALUE(INVALID_FILE_PATH);
		}
		
		seed_file_full_path = dt_get_task_seed_file_from_file(p_task);
		if(seed_file_full_path==NULL)
		{
			CHECK_VALUE(INVALID_SEED_FILE);
		}
		
		download_file_index_array = dt_get_task_bt_need_dl_file_index_array(p_task);
		if(download_file_index_array==NULL)
		{
			CHECK_VALUE(INVALID_FILE_INDEX_ARRAY);
		}
	
		if(p_task->_task_info->_user_data_len!=0)
		{
			ret_val = em_malloc(p_task->_task_info->_user_data_len, (void**)&user_data);
			CHECK_VALUE(ret_val);
			user_data_len = p_task->_task_info->_user_data_len;
			ret_val = dt_get_bt_task_user_data_from_file(p_task,(void*)user_data,&user_data_len);
			if(ret_val!=SUCCESS)
			{
				EM_SAFE_DELETE(user_data);
				user_data_len = 0;
				CHECK_VALUE(ret_val);
			}
		}
	}

	//ret_val=settings_get_int_item( "system.encoding_mode",(_int32*)&(encoding_switch_mode));
	//CHECK_VALUE(ret_val);

	ret_val=em_malloc(file_num*sizeof(_u32), (void**)&download_file_index_array_32);
	if(ret_val!=SUCCESS)
	{
		if(p_task_info->_full_info==FALSE)
		{
			dt_release_task_bt_need_dl_file_index_array(download_file_index_array);
		}
		CHECK_VALUE(ret_val);
	}
	
	for(i=0;i<file_num;i++)
	{
		download_file_index_array_32[i]=download_file_index_array[i];
	}
	
	
	p_task_id = &p_task->_inner_id;

	
	ret_val = eti_create_bt_task(seed_file_full_path, em_strlen(seed_file_full_path),  file_path, em_strlen(file_path),download_file_index_array_32, file_num,encoding_switch_mode,p_task_id);

	if(p_task_info->_full_info==FALSE)
	{
		dt_release_task_bt_need_dl_file_index_array(download_file_index_array);
	}
	EM_SAFE_DELETE(download_file_index_array_32);
	
	if(p_task_info->_full_info)
	{
		ret_val = dt_start_task_tag(p_task,ret_val,p_bt_task->_user_data, p_task->_task_info->_user_data_len,file_path);
	}
	else
	{
		ret_val = dt_start_task_tag(p_task,ret_val,(_u8*)user_data, user_data_len,file_path);
		EM_SAFE_DELETE(user_data);
		user_data_len = 0;
	}
	return ret_val;
}

_int32 dt_start_p2sp_task(EM_TASK * p_task)
{
	_int32 ret_val = SUCCESS;
	TASK_INFO *p_task_info=NULL;
	EM_P2SP_TASK * p_p2sp_task = NULL;
	char * file_name=NULL;
	char * file_path=NULL;
	char * url=NULL;
	char * ref_url=NULL;
	_u8 * tcid=NULL;
	_u8 * gcid=NULL;
	_u64 file_size = 0;
	_u32* p_task_id=NULL;
	char * user_data=NULL;
	char * cookie=NULL;
	_u32 cookie_len=0,user_data_len=0;
	//char * p0 = NULL;
	//char * p1 = NULL;
    
      EM_LOG_DEBUG("dt_start_p2sp_task:task_id=%u",p_task->_task_info->_task_id); 

	p_task_info = p_task->_task_info;
	if(p_task_info->_full_info)
	{
		EM_LOG_DEBUG("dt_start_p2sp_task: full_info"); 
		p_p2sp_task = (EM_P2SP_TASK *)p_task->_task_info;
		file_path = p_p2sp_task->_file_path;
		file_name = p_p2sp_task->_file_name;
		switch(p_task->_task_info->_type)
		{
			case TT_URL:
			case TT_LAN:
				url = p_p2sp_task->_url;
				ref_url = p_p2sp_task->_ref_url;
				if((p_p2sp_task->_user_data!=NULL)&&(p_task->_task_info->_user_data_len!=0))
				{
					ret_val = dt_get_task_common_user_data(p_p2sp_task->_user_data,p_task->_task_info->_user_data_len,(_u8 **)&cookie,&cookie_len);
					if(ret_val!=SUCCESS||em_strncmp(cookie,"Cookie:", 7)!=0)
					{
						cookie = NULL;
						cookie_len = 0;
					}
				}
				break;
			case TT_EMULE:
				url = p_p2sp_task->_url;
				break;
			case TT_KANKAN:
				gcid= p_task_info->_eigenvalue;
				tcid= p_p2sp_task->_tcid;
				break;
			case TT_TCID:
				tcid= p_task_info->_eigenvalue;
				break;
			default:
				CHECK_VALUE(INVALID_TASK_TYPE);
		}
	}
	else
	{
		file_path =dt_get_task_file_path_from_file(p_task);
		file_name = dt_get_task_file_name_from_file(p_task);
		if(p_task->_task_info->_user_data_len!=0)
		{
			ret_val = em_malloc(p_task->_task_info->_user_data_len, (void**)&user_data);
			CHECK_VALUE(ret_val);
			user_data_len = p_task->_task_info->_user_data_len;
			ret_val = dt_get_p2sp_task_user_data_from_file(p_task,(void*)user_data,&user_data_len);
			if(ret_val!=SUCCESS)
			{
				EM_SAFE_DELETE(user_data);
				user_data_len = 0;
				CHECK_VALUE(ret_val);
			}
		}
		
		switch(p_task->_task_info->_type)
		{
			case TT_URL:
			case TT_LAN:
				url =dt_get_task_url_from_file(p_task);
				ref_url = dt_get_task_ref_url_from_file(p_task);
				if(p_task->_task_info->_user_data_len!=0)
				{
					ret_val = dt_get_task_common_user_data((_u8*)user_data,user_data_len,(_u8 **)&cookie,&cookie_len);
					if(ret_val!=SUCCESS||em_strncmp(cookie,"Cookie:", 7)!=0)
					{
						cookie = NULL;
						cookie_len = 0;
					}
				}
				break;
			case TT_EMULE:
				url =dt_get_task_url_from_file(p_task);
				break;
			case TT_KANKAN:
				gcid= p_task_info->_eigenvalue;
				tcid= dt_get_task_tcid_from_file(p_task);
				break;
			case TT_TCID:
				tcid= p_task_info->_eigenvalue;
				break;
			default:
				EM_SAFE_DELETE(user_data);
				user_data_len = 0;
				CHECK_VALUE(INVALID_TASK_TYPE);
		}
	}

	file_size = p_task_info->_file_size;
	sd_assert(em_strlen(file_path)!=0);
	
	/* Check if enough free disk space before start downloading */
	// 创建本地文件任务不需要检查空间
	if( TT_FILE != p_task->_task_info->_type )
		ret_val = dt_check_task_free_disk(p_task,file_path);
	if(ret_val ==SUCCESS)
	{
		p_task_id = &p_task->_inner_id;

		switch(p_task->_task_info->_type)
		{
			case TT_URL:
			case TT_LAN:
				ret_val = eti_create_continue_task_by_url(url, em_strlen(url), ref_url, em_strlen(ref_url),cookie,cookie_len,file_path, em_strlen(file_path),file_name, em_strlen(file_name),p_task_id);
				if(((ret_val==DT_ERR_CFG_FILE_NOT_EXIST))||(ret_val==OPEN_OLD_FILE_FAIL))
				{
					#ifdef MOBILE_THUNDER_PROJ 
					if(p_task_info->_have_name==FALSE) file_name=NULL;
					#endif /* MOBILE_THUNDER_PROJ */
					ret_val = eti_create_new_task_by_url(url, em_strlen(url), ref_url, em_strlen(ref_url),cookie, cookie_len,file_path, em_strlen(file_path),file_name, em_strlen(file_name),p_task_id);
				}
	      			EM_LOG_ERROR("dt_start_p2sp_task:%d:task_id=%u,ret_val=%d,url=%s",p_task->_task_info->_type,p_task->_task_info->_task_id,ret_val,url); 
#ifdef ENABLE_ETM_MEDIA_CENTER
			if (SUCCESS == ret_val && p_task_info->_productid != 0)
			{
				ret_val = et_set_product_sub_id(*p_task_id, p_task_info->_productid, p_task_info->_subid);
				CHECK_VALUE(ret_val);
			}
#endif /* ENABLE_ETM_MEDIA_CENTER */
				break;
			case TT_EMULE:
				ret_val = eti_create_emule_task(url, em_strlen(url), file_path, em_strlen(file_path),file_name, em_strlen(file_name),p_task_id);
	      			EM_LOG_ERROR("dt_start_p2sp_task:TT_EMULE:task_id=%u,ret_val=%d,url=%s",p_task->_task_info->_task_id,ret_val,url); 
				break;
			case TT_KANKAN:
				ret_val = eti_create_task_by_tcid_file_size_gcid(tcid, file_size, gcid,file_name, em_strlen(file_name), file_path, em_strlen(file_path), p_task_id );
				#ifdef _LOGGER
				{
					char cid_buffer[CID_SIZE*2+1];
					em_memset(cid_buffer,0x00,CID_SIZE*2+1);
					em_str2hex((char*)gcid, CID_SIZE, cid_buffer, CID_SIZE*2+1);
		      			EM_LOG_ERROR("dt_start_p2sp_task:TT_KANKAN:task_id=%u,ret_val=%d,gcid=%s",p_task->_task_info->_task_id,ret_val,cid_buffer); 
				}
				#endif /* _LOGGER */
				break;
			case TT_TCID:
				ret_val = eti_create_continue_task_by_tcid(tcid, file_name, em_strlen(file_name), file_path, em_strlen(file_path), p_task_id );
				if(((ret_val==DT_ERR_CFG_FILE_NOT_EXIST))||(ret_val==OPEN_OLD_FILE_FAIL))
				{
					ret_val = eti_create_new_task_by_tcid(tcid,file_size, file_name, em_strlen(file_name), file_path, em_strlen(file_path), p_task_id );
				}
				#ifdef _LOGGER
				{
					char cid_buffer[CID_SIZE*2+1];
					em_memset(cid_buffer,0x00,CID_SIZE*2+1);
					em_str2hex((char*)tcid, CID_SIZE, cid_buffer, CID_SIZE*2+1);
		      			EM_LOG_ERROR("dt_start_p2sp_task:TT_TCID:task_id=%u,ret_val=%d,tcid=%s",p_task->_task_info->_task_id,ret_val,cid_buffer); 
				}
				#endif /* _LOGGER */
				break;
			default:
				EM_SAFE_DELETE(user_data);
				user_data_len = 0;
				CHECK_VALUE(INVALID_TASK_TYPE);
		}	
	}
	
	if(p_task_info->_full_info)
	{
		ret_val = dt_start_task_tag(p_task,ret_val,p_p2sp_task->_user_data, p_task->_task_info->_user_data_len,file_path);
	}
	else
	{
		ret_val = dt_start_task_tag(p_task,ret_val,(_u8*)user_data, user_data_len,file_path);
		EM_SAFE_DELETE(user_data);
		user_data_len = 0;
	}
	return ret_val;
}
_int32 dt_add_resource_to_task_impl(EM_TASK * p_task,EM_RES* res)
{
	if(!res)
	{
		return INVALID_ARGUMENT;
	}
	
	if(res->_type==ERT_PEER && dt_get_task_type(p_task)==TT_LAN) 
	{
	 	EM_LOG_ERROR("dt_add_resource_to_task_impl:res->_type=%u, ",res->_type); 
		/* 局域网任务不用p2p 资源 */
		return INVALID_TASK_TYPE;
	}
	
	eti_add_task_resource(p_task->_inner_id,(void*)res);
	
	return SUCCESS;
}

_int32 dt_add_resource_to_task(EM_TASK * p_task,_u8* user_data,_u32 user_data_len)
{
	_int32 index = 0;
	EM_RES * p_resource = NULL;
	
      EM_LOG_ERROR("dt_add_resource_to_task:task_id=%u",p_task->_task_info->_task_id); 
	  
	  if(user_data==NULL||user_data_len==0) return SUCCESS;
	  
	do
	{	
		p_resource = dt_get_resource_from_user_data(user_data,user_data_len, index);
		dt_add_resource_to_task_impl(p_task,p_resource);
		index++;
	}while(p_resource);
	
	return SUCCESS;
}
_int32 dt_start_task_tag(EM_TASK * p_task,_int32 ret_value,_u8* user_data,_u32 user_data_len,const char * path)
{
	_int32 ret_val = ret_value;
	TASK_INFO * p_task_info =p_task->_task_info;
	_u32 cur_time=0;

      EM_LOG_DEBUG("dt_start_task_tag:task_id=%u",p_task->_task_info->_task_id); 
	em_time(&cur_time);
	//if((p_task_info->_start_time==0)||(p_task->_task_info->_task_id > MAX_DL_TASK_ID))
	//{
		dt_set_task_start_time(p_task,cur_time);
	//}

	if(ret_val!=SUCCESS)
	{
		p_task->_inner_id = 0;
		dt_set_task_finish_time(p_task,cur_time);
		
		if(ret_val==4222)
		{
			dt_remove_task_from_order_list(p_task);
			dt_set_task_state(p_task,TS_TASK_SUCCESS);
		}
		else
		if(ret_val == INSUFFICIENT_DISK_SPACE)
		{
			dt_set_task_failed_code(p_task,112); //DATA_NO_SPACE_TO_CREAT_FILE
			dt_set_task_state(p_task,TS_TASK_FAILED);
		}
		else
		{
			dt_set_task_failed_code(p_task,ret_val);
			dt_set_task_state(p_task,TS_TASK_FAILED);
		}
	}
	else
	{	
		if(p_task_info->_check_data)//_check_data 用于_is_no_disk -----zyq @20101027 fro vod task
		{
			//ret_val = eti_vod_set_task_check_data(p_task->_inner_id);
			ret_val = eti_set_task_no_disk(p_task->_inner_id);
			if(ret_val!=SUCCESS)
			{
				eti_delete_task(p_task->_inner_id);
				dt_set_task_failed_code(p_task,ret_val);
				p_task->_inner_id = 0;
				em_time(&cur_time);
				dt_set_task_finish_time(p_task,cur_time);
				dt_set_task_state(p_task,TS_TASK_FAILED);
				goto ErrorHanle;
			}
		}
/*		
#if defined(_ANDROID_LINUX)
		BOOL support_big_file = FALSE;
		sd_is_support_create_big_file(path, &support_big_file);
		if((p_task->_task_info->_type != TT_BT)&&(!support_big_file))
		{
			//  Android平台下用自增长方式，乱序边下边写,最后再调成顺序by zyq @ 20130115 
			et_set_task_write_mode(p_task->_inner_id,3);
		}
#endif	
*/
		ret_val = eti_start_task(p_task->_inner_id);
		if(ret_val!=SUCCESS)
		{
			eti_delete_task(p_task->_inner_id);
			dt_set_task_failed_code(p_task,ret_val);
			p_task->_inner_id = 0;
			em_time(&cur_time);
			dt_set_task_finish_time(p_task,cur_time);
			dt_set_task_state(p_task,TS_TASK_FAILED);
		}
		else
		{	
			ret_val = dt_add_running_task(p_task);
			if(ret_val!=SUCCESS)
			{
				eti_stop_task(p_task->_inner_id);
				eti_delete_task(p_task->_inner_id);
				dt_set_task_failed_code(p_task,ret_val);
				p_task->_inner_id = 0;
				em_time(&cur_time);
				dt_set_task_finish_time(p_task,cur_time);
				dt_set_task_state(p_task,TS_TASK_FAILED);
			}
			else
			{
				dt_set_task_finish_time(p_task,0);
				dt_set_task_failed_code(p_task,0);
				dt_set_task_state(p_task,TS_TASK_RUNNING);
				dt_add_resource_to_task(p_task,user_data,user_data_len);
			}
			ret_val = et_set_origin_mode(p_task->_inner_id, FALSE);
			if(ret_val != SUCCESS)
			{
				EM_LOG_DEBUG("dt_start_task_tag: et_set_origin_mode-task_id=%u,ret_val=%d",p_task->_inner_id,ret_val); 
			}
		#ifdef ENABLE_HSC
			/* 启动高速通道 */
			/*LOG_DEBUG("dt_start_task_tag --p_task->_use_hsc:%d", p_task->_use_hsc);
			//if (p_task->_use_hsc)
			{
				//et_set_used_hsc_before(p_task->_inner_id, TRUE);
				TASK_STATE state;
				ET_HIGH_SPEED_CHANNEL_INFO hsc_info;
				char*   p_task_name         = NULL;
	    		_int32  task_name_length    = 0;
				_int32  hsc_ret = SUCCESS;
				state = dt_get_task_state(p_task);
				LOG_DEBUG("dt_start_task_tag --dt_get_task_state:%d", state);
				if (TS_TASK_RUNNING == state)
				{
					if (et_get_hsc_info(p_task->_inner_id, &hsc_info) == SUCCESS)
					{
						LOG_DEBUG("dt_start_task_tag --on dt_open_high_speed_channel _stat:%d",hsc_info._stat);
						if (ET_HSC_SUCCESS == hsc_info._stat )
						{
							hsc_ret = HIGH_SPEED_ALREADY_OPENED;
						}
						if (ET_HSC_ENTERING == hsc_info._stat)
						{
							hsc_ret = HIGH_SPEED_OPENING;
						}
					}
					if(SUCCESS == hsc_ret)
					{
					    p_task_name = dt_get_task_file_name(p_task);
					    if(p_task_name != NULL)
					        task_name_length = p_task->_task_info->_file_name_len;

					    LOG_DEBUG("dt_start_task_tag: et_high_speed_channel_switch: task_name_len:%d, name:%s", task_name_length, p_task_name);
						hsc_ret = et_high_speed_channel_switch(p_task->_inner_id, TRUE, TRUE, p_task_name, task_name_length);
					}
				}
			}
			*/
		#endif /* ENABLE_HSC */
		}
	}
	
ErrorHanle:
      EM_LOG_DEBUG("dt_start_task_tag :task_id=%u,ret_val=%d",p_task->_task_info->_task_id,ret_val); 
	return SUCCESS;

}
_int32 dt_stop_task_impl(EM_TASK * p_task)
{
	TASK_INFO *p_task_info=NULL;
	_u32 cur_time=0;

      EM_LOG_DEBUG("dt_stop_task_impl:task_id=%u",p_task->_task_info->_task_id); 

	p_task_info = p_task->_task_info;

	if(dt_get_task_state(p_task)==TS_TASK_RUNNING)
	{
		dt_remove_running_task(p_task);
		vod_report_impl( p_task->_task_info->_task_id);
		eti_stop_task(p_task->_inner_id);
		eti_delete_task(p_task->_inner_id);
		p_task->_inner_id = 0;
		em_time(&(cur_time));
		dt_set_task_finish_time(p_task,cur_time);
	}
	
	if((p_task->_task_info->_file_size > 0) && (p_task->_task_info->_downloaded_data_size>=p_task->_task_info->_file_size))
	{
		/* 已经下载完毕 */
		dt_set_task_state(p_task, TS_TASK_SUCCESS);
		dt_remove_task_from_order_list(p_task);
	}
	else
	{
	dt_set_task_state(p_task,TS_TASK_PAUSED);
	}
	/* save change to task_file */
	//ts_save_task_to_file(p_task);
	if(p_task->_waiting_stop)
	{
		p_task->_waiting_stop = FALSE;
	}

	dt_bt_running_file_safe_delete(p_task);
	
	return SUCCESS;
}

_int32 dt_delete_task_impl(EM_TASK * p_task)
{

      EM_LOG_DEBUG("dt_delete_task_impl:task_id=%u",p_task->_task_info->_task_id); 
	 if(dt_get_task_state(p_task)!=TS_TASK_SUCCESS)
		dt_remove_task_from_order_list(p_task);
	dt_set_task_state(p_task,TS_TASK_DELETED);
	
	return SUCCESS;
}
_int32 dt_recover_task_impl(EM_TASK * p_task)
{
	_int32 ret_val = SUCCESS;
      EM_LOG_DEBUG("dt_recover_task_impl:task_id=%u",p_task->_task_info->_task_id); 

	if(p_task->_task_info->_state!=TS_TASK_SUCCESS)
	{
		ret_val = dt_add_task_to_order_list(p_task);
		CHECK_VALUE(ret_val);
	}
 	p_task->_task_info->_is_deleted=FALSE;
	dt_set_task_change(p_task,CHANGE_DELETE);

 
	/* call the call_back_function to notify the UI */
	//dt_set_task_state_changed_callback
 	return SUCCESS;
}
_int32 dt_destroy_task_impl(EM_TASK * p_task,BOOL delete_file)
{
	//_int32 ret_val = SUCCESS;
	char *file_path=NULL,*file_name=NULL;
	EM_P2SP_TASK * p_p2sp_task = NULL;
	EM_BT_TASK *p_bt_task = NULL;
	_u32 encoding_mode = ET_ENCODING_UTF8_SWITCH;
	char *seed_path = NULL;
	_int32 ret_val = SUCCESS;
    TORRENT_SEED_INFO *p_seed_info = NULL;
    char fill_full_path[MAX_FILE_PATH_LEN] = {0};
    _u32 len = 0;
	char tmp_file_name[ MAX_FILE_NAME_LEN ] = {0};
	
	em_settings_get_int_item("system.encoding_mode", (_int32*)&encoding_mode);

      EM_LOG_DEBUG("dt_destroy_task_impl:task_id=%u",p_task->_task_info->_task_id); 
	/* delete file */
	if(delete_file)
	{
		if(p_task->_task_info->_type!=TT_BT)
		{
			if(p_task->_task_info->_full_info)
			{
				p_p2sp_task = (EM_P2SP_TASK*)p_task->_task_info;
				file_path = p_p2sp_task->_file_path;
				file_name = p_p2sp_task->_file_name;
			}
			else
			{
				file_path = dt_get_task_file_path_from_file(p_task);
				file_name = dt_get_task_file_name_from_file(p_task);
			}

	 		if((file_path!=NULL)&&(file_name!=NULL))
	 		{
	 			eti_remove_tmp_file(file_path, file_name);
	 		}
		}
		else
		{
#ifdef ENABLE_BT
			if(p_task->_task_info->_full_info)
			{
				p_bt_task = (EM_BT_TASK *)p_task->_task_info;
				seed_path = p_bt_task->_seed_file_path;
				file_path = p_bt_task->_file_path;
			}
			else
			{
				seed_path= dt_get_task_seed_file_from_file(p_task);
				file_path = dt_get_task_file_path_from_file(p_task);
			}

			ret_val= tp_get_seed_info( seed_path,encoding_mode, &p_seed_info );
			if( ret_val != SUCCESS )
			{
				p_seed_info = NULL;
			}

			if( p_seed_info != NULL )
			{			
			em_strncpy( fill_full_path, file_path, MAX_FILE_PATH_LEN );
			len = em_strlen(fill_full_path); 
			if( fill_full_path[len-1] != '/' && len < MAX_FILE_PATH_LEN )
			{
				fill_full_path[len] = '/';
				len += 1;
			}
			
			em_strncpy( tmp_file_name, fill_full_path, MAX_FILE_PATH_LEN );
			
			em_strcat( fill_full_path, p_seed_info->_title_name, MAX_FILE_PATH_LEN - len );
			ret_val = sd_delete_dir( fill_full_path );
			EM_LOG_DEBUG("dt_destroy_task_impl del dir:bt_task_id=%u, path:%s, ret=%d",
				p_task->_task_info->_task_id, fill_full_path, ret_val ); 

            if( p_seed_info->_file_info_array_ptr[0]->_file_path_len == 0 )
            {
				em_strcat( fill_full_path, ".rf", MAX_FILE_PATH_LEN - em_strlen(fill_full_path) );
				ret_val = sd_delete_dir( fill_full_path );
				EM_LOG_DEBUG("dt_destroy_task_impl del td:bt_task_id=%u, path:%s, ret=%d",
					p_task->_task_info->_task_id, fill_full_path, ret_val ); 
				
				em_strcat( fill_full_path, ".cfg", MAX_FILE_PATH_LEN - em_strlen(fill_full_path) );
				ret_val = sd_delete_dir( fill_full_path );				
				EM_LOG_DEBUG("dt_destroy_task_impl del cfg:bt_task_id=%u, path:%s, ret=%d",
					p_task->_task_info->_task_id, fill_full_path, ret_val );

				em_memset( fill_full_path, 0, MAX_FILE_PATH_LEN );
				str2hex((char*)p_seed_info->_info_hash, INFO_HASH_LEN, fill_full_path, INFO_HASH_LEN*2);
				
				em_strcat( tmp_file_name, fill_full_path, MAX_FILE_PATH_LEN - em_strlen(tmp_file_name) );
				em_strcat( tmp_file_name, ".tmp", MAX_FILE_PATH_LEN - em_strlen(tmp_file_name) );

				ret_val = sd_delete_dir( tmp_file_name );				
				EM_LOG_DEBUG("dt_destroy_task_impl del tmp:bt_task_id=%u, path:%s, ret=%d",
					p_task->_task_info->_task_id, tmp_file_name, ret_val );

				em_strcat( tmp_file_name, ".cfg", MAX_FILE_PATH_LEN - em_strlen(tmp_file_name) );

				ret_val = sd_delete_dir( tmp_file_name );				
				EM_LOG_DEBUG("dt_destroy_task_impl del tmp cfg:bt_task_id=%u, path:%s, ret=%d",
					p_task->_task_info->_task_id, tmp_file_name, ret_val );
            }

				tp_release_seed_info(p_seed_info);
			}
#ifdef ENABLE_ETM_MEDIA_CENTER			
			ret_val = sd_delete_dir( seed_path );
#endif
			EM_LOG_DEBUG("dt_destroy_task_impl del dir:bt_task_id=%u, seed_path:%s, ret=%d",
				p_task->_task_info->_task_id, seed_path, ret_val );

			/*  do nothing */
			//sd_assert(FALSE);
#endif
		}
	}
	
 	/* remove from dling_order_list */
	 if((dt_get_task_state(p_task)!=TS_TASK_SUCCESS)&&(dt_get_task_state(p_task)!=TS_TASK_DELETED))
	 {
		dt_remove_task_from_order_list(p_task);
	 }
	
	/* remove eigenvalue */
	dt_remove_task_eigenvalue(p_task->_task_info->_type,p_task->_task_info->_eigenvalue);
	
	if(p_task->_task_info->_file_name_eigenvalue!=0)
	{
		dt_remove_file_name_eigenvalue(p_task->_task_info->_file_name_eigenvalue);
	}
	/* remove from all_tasks_map */
	dt_remove_task_from_map(p_task);
	
	/* disable from task_file */
	dt_disable_task_in_file(p_task);

	/* if this is a running bt task */
	dt_bt_running_file_safe_delete(p_task);

	dt_uninit_task_info(p_task->_task_info);
	/* free task */
	dt_task_free(p_task);
	
	return SUCCESS;
}

_int32 dt_rename_task_impl(EM_TASK * p_task,char * new_name,_u32 new_name_len)
{
	_int32 ret_val = SUCCESS;
	char old_full_path[MAX_FULL_PATH_BUFFER_LEN];
	char new_full_path[MAX_FULL_PATH_BUFFER_LEN];
	char * old_name = dt_get_task_file_name(p_task);
	char * file_path = NULL;
	_u32 file_name_eigenvalue = 0;

      EM_LOG_DEBUG("dt_rename_task_impl:task_id=%u,old_name=%s,new_name=%s\n",p_task->_task_info->_task_id,old_name,new_name); 
	  sd_assert(old_name!=NULL);
  	
	if((new_name_len==p_task->_task_info->_file_name_len)&&(em_strncmp(old_name, new_name, new_name_len)==0))
	{
     		 EM_LOG_DEBUG("dt_rename_task_impl:task_id=%u,old_name=new_name=%s\n",p_task->_task_info->_task_id,old_name); 
		return SUCCESS;
	}
	
	em_memset(old_full_path,0,MAX_FULL_PATH_BUFFER_LEN);
	em_memset(new_full_path,0,MAX_FULL_PATH_BUFFER_LEN);

	file_path = dt_get_task_file_path(p_task);
	  sd_assert(file_path!=NULL);
	
	ret_val = dt_generate_file_name_eigenvalue(file_path,p_task->_task_info->_file_path_len,new_name,new_name_len,&file_name_eigenvalue);
	CHECK_VALUE(ret_val);
	
	if(dt_is_file_exist(file_name_eigenvalue)==TRUE)
	{
		ret_val=FILE_ALREADY_EXIST;
		return ret_val;
	}
	
	em_snprintf(old_full_path, MAX_FULL_PATH_BUFFER_LEN, "%s/%s", file_path,old_name);
	em_strncpy(new_full_path, file_path,MAX_FULL_PATH_BUFFER_LEN);
	em_strcat(new_full_path,"/",1);
	em_strcat(new_full_path,new_name,new_name_len);

	if( em_file_exist(new_full_path))
	{
		ret_val=FILE_ALREADY_EXIST;
		return ret_val;
	}

      EM_LOG_DEBUG("dt_rename_task_impl:task_id=%u,old_full_path=%s,new_full_path=%s\n",p_task->_task_info->_task_id,old_full_path,new_full_path); 

	ret_val =  em_rename_file(old_full_path, new_full_path);

	//sd_assert(FALSE);
	//ret_val = -1;
	if(ret_val==SUCCESS)
	{
		ret_val =  dt_set_task_name(p_task,new_name, new_name_len);
	}
	return ret_val;
}

_int32 dt_get_task_user_data_impl(EM_TASK * p_task,void * data_buffer,	_u32 buffer_len)
{
	_int32 ret_val = SUCCESS;
	_u32 in_buffer_len = buffer_len;
	EM_BT_TASK *p_bt_task=NULL;
	EM_P2SP_TASK *p_p2sp_task=NULL;
	
	  if(p_task->_task_info->_have_user_data)
	  {
	  	if(buffer_len<p_task->_task_info->_user_data_len) return NOT_ENOUGH_BUFFER;
		
		if(p_task->_task_info->_type== TT_BT)
		{
			if(p_task->_task_info->_full_info)
			{
				p_bt_task = (EM_BT_TASK *)p_task->_task_info;
				em_memcpy(data_buffer,p_bt_task->_user_data,p_task->_task_info->_user_data_len);
			}
			else
			{
				ret_val = dt_get_bt_task_user_data_from_file(p_task,data_buffer,&in_buffer_len);
				CHECK_VALUE(ret_val);
			}
		}
		else
		{
			if(p_task->_task_info->_full_info)
			{
				p_p2sp_task = (EM_P2SP_TASK *)p_task->_task_info;
				em_memcpy(data_buffer,p_p2sp_task->_user_data,p_task->_task_info->_user_data_len);
			}
			else
			{
				ret_val =  dt_get_p2sp_task_user_data_from_file(p_task,data_buffer,&in_buffer_len);
				CHECK_VALUE(ret_val);
			}
		}
	  }
	  else
	  {
	  	ret_val =  INVALID_USER_DATA;
	  }
	  return ret_val;
}	
_int32 dt_get_task_common_user_data(_u8 * user_data,	_u32 user_data_len,_u8 ** common_user_data,_u32 * common_user_data_len)
{
	//_int32 ret_val = SUCCESS;
	USER_DATA_STORE_HEAD user_data_head;
	USER_DATA_ITEM_HEAD user_data_item_head ;
	USER_DATA_STORE_HEAD * p_user_data_head = &user_data_head;
	USER_DATA_ITEM_HEAD * p_user_data_item_head= &user_data_item_head;

	if(user_data_len<=sizeof(USER_DATA_STORE_HEAD)+sizeof(USER_DATA_ITEM_HEAD))
	{
		return INVALID_USER_DATA;
	}
	
	sd_memcpy(&user_data_head,user_data,sizeof(USER_DATA_STORE_HEAD));
	if(p_user_data_head->_PACKING_1!=PACKING_1_VALUE||p_user_data_head->_PACKING_2!=PACKING_2_VALUE||p_user_data_head->_item_num==0)
	{
		return INVALID_USER_DATA;
	}

	sd_memcpy(&user_data_item_head,user_data+sizeof(USER_DATA_STORE_HEAD),sizeof(USER_DATA_ITEM_HEAD));
	if(p_user_data_item_head->_type !=UDIT_USER_DATA)
	{
		return INVALID_USER_DATA;
	}

	*common_user_data_len = p_user_data_item_head->_size;
	if(p_user_data_item_head->_size==0)
	{
		return INVALID_USER_DATA;
	}

	*common_user_data = user_data+sizeof(USER_DATA_STORE_HEAD)+sizeof(USER_DATA_ITEM_HEAD);
	
	return SUCCESS;
}	
EM_RES * dt_get_resource_from_user_data_impl(_u8 * user_data,_u32 user_data_len)
{
	//_int32 ret_val = SUCCESS;
	static EM_RES resource;
	static EM_RES * p_resource= &resource;

      EM_LOG_ERROR("dt_get_resource_from_user_data_impl"); 
	if(user_data_len<sizeof(EM_RES))
	{
		return NULL;
	}

	sd_memcpy(&resource,user_data,sizeof(EM_RES));
	
	if(p_resource->_type == ERT_SERVER)
	{
		if(p_resource->s_res._url_len==0 ) return NULL;
		
		p_resource->s_res._url = (char*)(user_data+sizeof(EM_RES));
		
		if(p_resource->s_res._ref_url_len!=0)
		{
			p_resource->s_res._ref_url = (char*)(user_data+sizeof(EM_RES)+p_resource->s_res._url_len);
		}
		else
		{
			p_resource->s_res._ref_url = NULL;
		}
		
		if(p_resource->s_res._cookie_len!=0)
		{
			p_resource->s_res._cookie = (char*)(user_data+sizeof(EM_RES)+p_resource->s_res._url_len+p_resource->s_res._ref_url_len);
		}
		else
		{
			p_resource->s_res._cookie = NULL;
		}
	}
	else
	if(p_resource->_type == ERT_PEER)
	{
		//return NULL;
	}

	return p_resource;
}	


EM_RES * dt_get_resource_from_user_data(_u8 * user_data,_u32 user_data_len,_int32 index)
{
	//_int32 ret_val = SUCCESS;
	//static EM_RES resource;
	USER_DATA_STORE_HEAD user_data_head;
	USER_DATA_ITEM_HEAD user_data_item_head ;
	USER_DATA_STORE_HEAD * p_user_data_head = &user_data_head;
	USER_DATA_ITEM_HEAD * p_user_data_item_head= &user_data_item_head;
	_u8 * pos = NULL;
	_int32 i = 0;
	_u32 item_size = 0;

      EM_LOG_ERROR("dt_get_resource_from_user_data:index=%u",index); 
	if(user_data_len<=sizeof(USER_DATA_STORE_HEAD)+sizeof(USER_DATA_ITEM_HEAD))
	{
		return NULL;
	}
	
	sd_memcpy(&user_data_head,user_data,sizeof(USER_DATA_STORE_HEAD));
	if(p_user_data_head->_PACKING_1!=PACKING_1_VALUE||p_user_data_head->_PACKING_2!=PACKING_2_VALUE||p_user_data_head->_item_num==0)
	{
		return NULL;
	}
	
	sd_memcpy(&user_data_item_head,user_data+sizeof(USER_DATA_STORE_HEAD),sizeof(USER_DATA_ITEM_HEAD));
	pos = user_data+sizeof(USER_DATA_STORE_HEAD)+sizeof(USER_DATA_ITEM_HEAD);
	while((pos-user_data<user_data_len)&&(i<=index))
	{
		if(i == index&&((p_user_data_item_head->_type == UDIT_SERVER_RES)||(p_user_data_item_head->_type == UDIT_PEER_RES)))
		{
			return dt_get_resource_from_user_data_impl(pos,p_user_data_item_head->_size);
		}
		else
		{
			if((p_user_data_item_head->_type == UDIT_SERVER_RES)||(p_user_data_item_head->_type == UDIT_PEER_RES))
			{
				i++;
			}
			item_size = p_user_data_item_head->_size;
			
			if(pos+item_size+sizeof(USER_DATA_ITEM_HEAD)>user_data+user_data_len)
			{
				break;
			}
			
			sd_memcpy(&user_data_item_head,pos+item_size,sizeof(USER_DATA_ITEM_HEAD));
			pos += item_size+sizeof(USER_DATA_ITEM_HEAD);
		}
	}

	return NULL;
}	

_int32 dt_get_task_extra_res_pos(USER_DATA_ITEM_TYPE item_type,void * extra_item,_u8 * user_data,	_u32 user_data_len,_u8 ** extra_item_pos)
{
	_int32 ret_val = SUCCESS;
	USER_DATA_STORE_HEAD user_data_head;
	USER_DATA_STORE_HEAD * p_user_data_head = &user_data_head;
	EM_RES * p_resource = (EM_RES *)extra_item;
	EM_RES res_tmp = {0};
	
	*extra_item_pos = NULL;

	if(user_data_len<=sizeof(USER_DATA_STORE_HEAD)+sizeof(USER_DATA_ITEM_HEAD))
	{
		return INVALID_USER_DATA;
	}
	
	sd_memcpy(&user_data_head,user_data,sizeof(USER_DATA_STORE_HEAD));
	if(p_user_data_head->_PACKING_1!=PACKING_1_VALUE||p_user_data_head->_PACKING_2!=PACKING_2_VALUE||p_user_data_head->_item_num==0)
	{
		return INVALID_USER_DATA;
	}

	ret_val =  dt_get_task_next_extra_item_pos( item_type,user_data+sizeof(USER_DATA_STORE_HEAD),user_data_len-sizeof(USER_DATA_STORE_HEAD),extra_item_pos);
	if(ret_val!=SUCCESS) return ret_val;
	while(*extra_item_pos)
	{
		em_memcpy(&res_tmp,*extra_item_pos,sizeof(res_tmp));
		if(res_tmp._type== p_resource->_type)
		{
			if(res_tmp._type == ERT_SERVER)
			{
				res_tmp.s_res._url = (char*)((*extra_item_pos)+sizeof(res_tmp));
				if((res_tmp.s_res._file_index==p_resource->s_res._file_index)&&
					(em_strncmp(res_tmp.s_res._url,p_resource->s_res._url,p_resource->s_res._url_len)==0))
				{
					return SUCCESS;
				}
			}
			else
			{
				if((res_tmp.p_res._file_index==p_resource->p_res._file_index)&&
					(em_strncmp(res_tmp.p_res._peer_id,p_resource->p_res._peer_id,PEER_ID_SIZE)==0))
				{
					return SUCCESS;
				}
			}
			
		}
		ret_val =  dt_get_task_next_extra_item_pos( item_type,(*extra_item_pos)+sizeof(DT_LIXIAN_ID),user_data_len-((*extra_item_pos)+sizeof(DT_LIXIAN_ID)-user_data),extra_item_pos);
		if(ret_val!=SUCCESS) return ret_val;
	}
	
	return INVALID_USER_DATA;
}	

_int32 dt_add_server_resource_impl(EM_TASK * p_task,EM_SERVER_RES * p_resource)
{
	_int32 ret_val = SUCCESS;
      USER_DATA_ITEM_HEAD item_head;
	 USER_DATA_STORE_HEAD user_data_head,user_data_head2;
	USER_DATA_STORE_HEAD * p_user_data_head = NULL;
	EM_RES resource;
	  _u8 *user_data = NULL,* extra_item_pos = NULL;
	  _u32 user_data_len = 0,off_set = 0;
	  char cookie[MAX_URL_LEN]="Cookie:";
	  BOOL correct_cookie = FALSE;

      EM_LOG_DEBUG("dt_add_server_resource_impl"); 

	if(p_resource->_url_len<7) return INVALID_URL;

	if(p_resource->_cookie_len!=0)
	{
		if(em_strncmp(p_resource->_cookie,"Cookie:", 7)!=0)
		{
			em_memcpy(cookie+7,p_resource->_cookie,p_resource->_cookie_len);
			cookie[p_resource->_cookie_len+7] = '\0';
			correct_cookie = TRUE;
		}
	
	}

	  item_head._type = (_u16)UDIT_SERVER_RES;
	  if(correct_cookie)
	  	item_head._size = sizeof(EM_RES)+ p_resource->_url_len + p_resource->_ref_url_len + p_resource->_cookie_len+7;
	  else
	 	item_head._size = sizeof(EM_RES)+ p_resource->_url_len + p_resource->_ref_url_len + p_resource->_cookie_len;
	  
	if(p_task->_task_info->_user_data_len!=0)
	{
	 	 user_data_len = p_task->_task_info->_user_data_len +sizeof(USER_DATA_STORE_HEAD)+ sizeof(USER_DATA_ITEM_HEAD) +  item_head._size;
	}
	else
	{
	  	user_data_len = sizeof(USER_DATA_STORE_HEAD) + sizeof(USER_DATA_ITEM_HEAD) +  item_head._size;
	}
	
	em_memset(&resource,0x00,sizeof(EM_RES ));
	resource._type = ERT_SERVER;
	em_memcpy(&resource.s_res,p_resource,sizeof(EM_SERVER_RES));
	if(TT_BT!=dt_get_task_type(p_task))
	{
		resource.s_res._file_index = INVALID_FILE_INDEX;
	}

	ret_val = em_malloc(user_data_len, (void**)&user_data);
	CHECK_VALUE(ret_val);
	em_memset(&user_data_head,0x00,sizeof(USER_DATA_STORE_HEAD ));
	user_data_head._PACKING_1 = PACKING_1_VALUE;
	user_data_head._PACKING_2 = PACKING_2_VALUE;
	user_data_head._ver = USER_DATA_VERSION;
	user_data_head._item_num= 0;
	
	if(p_task->_task_info->_user_data_len!=0)
	{
		ret_val = dt_get_task_user_data_impl(p_task,user_data,user_data_len);
		if(ret_val!= SUCCESS) goto ErrHandler;
		em_memcpy(&user_data_head2,user_data,sizeof(USER_DATA_STORE_HEAD ));
		p_user_data_head = &user_data_head2;
		if(p_task->_task_info->_user_data_len<sizeof(USER_DATA_STORE_HEAD)+sizeof(USER_DATA_ITEM_HEAD)
			||p_user_data_head->_PACKING_1!=PACKING_1_VALUE||p_user_data_head->_PACKING_2!=PACKING_2_VALUE
			||p_user_data_head->_item_num==0)
		{
			off_set = sizeof(USER_DATA_STORE_HEAD);
		}
		else
		{
			user_data_head._item_num=p_user_data_head->_item_num;
			off_set = p_task->_task_info->_user_data_len;
			dt_get_task_extra_res_pos( UDIT_SERVER_RES,&resource,user_data,	 p_task->_task_info->_user_data_len,&extra_item_pos);
			if(extra_item_pos!=NULL)
			{
				/* 相同资源已经存在 */
				EM_SAFE_DELETE(user_data);
				return SUCCESS;
			}
		}
	}
	else
	{
		off_set = sizeof(USER_DATA_STORE_HEAD);
	}

	user_data_head._item_num++;
	em_memcpy(user_data,&user_data_head,sizeof(USER_DATA_STORE_HEAD));
	em_memcpy(user_data+off_set,&item_head,sizeof(USER_DATA_ITEM_HEAD));
	off_set +=sizeof(USER_DATA_ITEM_HEAD);


  	if(correct_cookie)
  	{
  		resource.s_res._cookie_len = p_resource->_cookie_len+7;
  	}

	em_memcpy(user_data+off_set,&resource,sizeof(EM_RES));
	off_set +=sizeof(EM_RES);
	
	em_memcpy(user_data+off_set,p_resource->_url,p_resource->_url_len);
	off_set +=p_resource->_url_len;

	if(p_resource->_ref_url_len!=0)
	{
		em_memcpy(user_data+off_set,p_resource->_ref_url,p_resource->_ref_url_len);
		off_set +=p_resource->_ref_url_len;
	}
	
	if(p_resource->_cookie_len!=0)
	{
	  	if(correct_cookie)
	  	{
			em_memcpy(user_data+off_set,cookie,p_resource->_cookie_len+7);
			off_set +=p_resource->_cookie_len+7;
			resource.s_res._cookie = cookie;
			resource.s_res._cookie_len = p_resource->_cookie_len+7;
	  	}
		else
		{
			em_memcpy(user_data+off_set,p_resource->_cookie,p_resource->_cookie_len);
			off_set +=p_resource->_cookie_len;
		}
	}

	user_data_len = off_set;

	dt_save_task_user_data_to_file(p_task,user_data,user_data_len);
	
	if(dt_get_task_state(p_task)==TS_TASK_RUNNING)
	{
		dt_add_resource_to_task_impl(p_task,&resource);
	}
	
	if(!p_task->_task_info->_full_info)
	{
		EM_SAFE_DELETE(user_data);
	}
	return ret_val;

ErrHandler:
	EM_SAFE_DELETE(user_data);
	return ret_val;
}

_int32 dt_add_peer_resource_impl(EM_TASK * p_task, EM_PEER_RES * p_resource)
{
	_int32 ret_val = SUCCESS;
	EM_RES resource;
	em_memset(&resource, 0x00, sizeof(EM_RES));
	resource._type = ERT_PEER;
	em_memcpy(&resource.p_res, p_resource, sizeof(EM_PEER_RES));
	
	if(TT_BT != dt_get_task_type(p_task))
		resource.p_res._file_index = INVALID_FILE_INDEX;

	ret_val = dt_set_task_extra_item( p_task,UDIT_PEER_RES,&resource);
	CHECK_VALUE(ret_val);
	
	if(dt_get_task_state(p_task) == TS_TASK_RUNNING)
	{
		dt_add_resource_to_task_impl(p_task, &resource);
	}

	return ret_val;
}
_int32 dt_get_peer_resource_impl(EM_TASK * p_task, EM_PEER_RES * p_resource)
{
	_int32 ret_val = SUCCESS;
	EM_RES resource;

	if(TT_LAN != dt_get_task_type(p_task))
	{
		return INVALID_TASK_TYPE;
	}

	em_memset(&resource, 0x00, sizeof(EM_RES));
	resource._type = ERT_PEER;
	
	ret_val = dt_get_task_extra_item( p_task,UDIT_PEER_RES,&resource);
	if(ret_val!=SUCCESS) return ret_val;
	
	em_memcpy(p_resource,&resource.p_res, sizeof(EM_PEER_RES));
	
	return SUCCESS;
}
	
_int32  dt_save_task_user_data_to_file(EM_TASK * p_task,_u8 * user_data,_u32  user_data_len)
{
	if(TT_BT==dt_get_task_type(p_task))
	{
		EM_BT_TASK * p_bt_task = NULL;
		if(p_task->_task_info->_full_info)
		{
			p_bt_task = (EM_BT_TASK *)p_task->_task_info;
			if(p_task->_task_info->_have_user_data)
			{
				EM_SAFE_DELETE(p_bt_task->_user_data);
			}

			p_task->_task_info->_have_user_data = TRUE;
			p_task->_task_info->_user_data_len = user_data_len;
			p_bt_task->_user_data = user_data;
		}
		dt_save_bt_task_user_data_to_file(p_task,user_data,user_data_len);
	}
	else
	{
		EM_P2SP_TASK * p_p2sp_task = NULL;
		if(p_task->_task_info->_full_info)
		{
			p_p2sp_task = (EM_P2SP_TASK *)p_task->_task_info;
			if(p_task->_task_info->_have_user_data)
			{
				EM_SAFE_DELETE(p_p2sp_task->_user_data);
			}

			p_task->_task_info->_have_user_data = TRUE;
			p_task->_task_info->_user_data_len = user_data_len;
			p_p2sp_task->_user_data = user_data;
		}
		dt_save_p2sp_task_user_data_to_file(p_task,user_data,user_data_len);
	}
	return SUCCESS;
}
_u16 dt_get_sizeof_extra_item(USER_DATA_ITEM_TYPE item_type)
{
	switch(item_type)
		{
			case UDIT_SERVER_RES:
				return SIZE_OF_SERVER_RES;
				break;
			case UDIT_PEER_RES:
				return SIZE_OF_PEER_RES;
				break;
			case UDIT_LIXIAN_MODE:
				return SIZE_OF_LIXIAN_MODE;
				break;
			case UDIT_VOD_DOWNLOAD_MODE:
				return SIZE_OF_VOD_DOWNLOAD_MODE;
				break;
			case UDIT_HSC_MODE:
				return SIZE_OF_HSC_MODE;
				break;
			case UDIT_LIXIAN_ID:
				return SIZE_OF_LIXIAN_ID;
				break;
			case UDIT_AD_MODE:
				return SIZE_OF_AD_MODE;
			default:
				sd_assert(FALSE);
				return 0;
		}
	return 0;
}
_int32 dt_get_task_next_extra_item_pos(USER_DATA_ITEM_TYPE item_type,_u8 * user_data,	_u32 user_data_len,_u8 ** extra_item_pos)
{
	USER_DATA_ITEM_HEAD user_data_item_head ;
	USER_DATA_ITEM_HEAD * p_user_data_item_head= &user_data_item_head;
	_u8 * pos = NULL;
	_u32 item_size = 0;

	*extra_item_pos = NULL;

	if(user_data_len<= sizeof(USER_DATA_ITEM_HEAD))
	{
		return INVALID_USER_DATA;
	}
	
	sd_memcpy(&user_data_item_head,user_data,sizeof(USER_DATA_ITEM_HEAD));
	pos = user_data+sizeof(USER_DATA_ITEM_HEAD);
	while(pos-user_data<user_data_len)
	{
		if(p_user_data_item_head->_type == item_type)
		{
			*extra_item_pos = pos;
			return SUCCESS;
		}
		item_size = p_user_data_item_head->_size;
			
		if(pos+item_size+sizeof(USER_DATA_ITEM_HEAD)>user_data+user_data_len)
		{
			break;
		}

		sd_memcpy(&user_data_item_head,pos+item_size,sizeof(USER_DATA_ITEM_HEAD));
		pos += item_size+sizeof(USER_DATA_ITEM_HEAD);
	}
	
	return INVALID_USER_DATA;
}	

_int32 dt_get_task_extra_item_pos(USER_DATA_ITEM_TYPE item_type,_u8 * user_data,	_u32 user_data_len,_u8 ** extra_item_pos)
{
	//_int32 ret_val = SUCCESS;
	USER_DATA_STORE_HEAD user_data_head;
	USER_DATA_STORE_HEAD * p_user_data_head = &user_data_head;

	*extra_item_pos = NULL;

	if(user_data_len<=sizeof(USER_DATA_STORE_HEAD)+sizeof(USER_DATA_ITEM_HEAD))
	{
		return INVALID_USER_DATA;
	}
	
	sd_memcpy(&user_data_head,user_data,sizeof(USER_DATA_STORE_HEAD));
	if(p_user_data_head->_PACKING_1!=PACKING_1_VALUE||p_user_data_head->_PACKING_2!=PACKING_2_VALUE||p_user_data_head->_item_num==0)
	{
		return INVALID_USER_DATA;
	}

	return dt_get_task_next_extra_item_pos( item_type,user_data+sizeof(USER_DATA_STORE_HEAD),user_data_len-sizeof(USER_DATA_STORE_HEAD),extra_item_pos);
}
_int32 dt_set_task_extra_item(EM_TASK * p_task,USER_DATA_ITEM_TYPE item_type,void * extra_item)
{
	_int32 ret_val = SUCCESS;
      USER_DATA_ITEM_HEAD item_head;
	 USER_DATA_STORE_HEAD user_data_head,user_data_head2;
	USER_DATA_STORE_HEAD * p_user_data_head = NULL;
	  _u8 * user_data = NULL,* extra_item_pos = NULL;
	  _u32 user_data_len = 0,off_set = 0;

      EM_LOG_DEBUG("dt_set_task_extra_item"); 

	item_head._type = (_u16)item_type;
 	item_head._size = dt_get_sizeof_extra_item(item_type);
	  
	if(p_task->_task_info->_user_data_len!=0)
	{
	 	 user_data_len = p_task->_task_info->_user_data_len +sizeof(USER_DATA_STORE_HEAD)+ sizeof(USER_DATA_ITEM_HEAD) +  item_head._size;
	}
	else
	{
	  	user_data_len = sizeof(USER_DATA_STORE_HEAD) + sizeof(USER_DATA_ITEM_HEAD) +  item_head._size;
	}
	
	ret_val = em_malloc(user_data_len, (void**)&user_data);
	CHECK_VALUE(ret_val);
	em_memset(&user_data_head,0x00,sizeof(USER_DATA_STORE_HEAD ));
	user_data_head._PACKING_1 = PACKING_1_VALUE;
	user_data_head._PACKING_2 = PACKING_2_VALUE;
	user_data_head._ver = USER_DATA_VERSION;
	user_data_head._item_num= 0;
	
	if(p_task->_task_info->_user_data_len!=0)
	{
		ret_val = dt_get_task_user_data_impl(p_task,user_data,user_data_len);
		if(ret_val!= SUCCESS) goto ErrHandler;
		em_memcpy(&user_data_head2,user_data,sizeof(USER_DATA_STORE_HEAD ));
		p_user_data_head = &user_data_head2;
		if(p_task->_task_info->_user_data_len<sizeof(USER_DATA_STORE_HEAD)+sizeof(USER_DATA_ITEM_HEAD)
			||p_user_data_head->_PACKING_1!=PACKING_1_VALUE||p_user_data_head->_PACKING_2!=PACKING_2_VALUE
			||p_user_data_head->_item_num==0)
		{
			off_set = sizeof(USER_DATA_STORE_HEAD);
		}
		else
		{
			user_data_head._item_num=p_user_data_head->_item_num;
			off_set = p_task->_task_info->_user_data_len;
			#ifdef ENABLE_LIXIAN
			if(item_type == UDIT_LIXIAN_ID)
			{
				dt_get_task_lixian_id_pos( item_type,extra_item,user_data,	 p_task->_task_info->_user_data_len,&extra_item_pos);
			}
			else
			#endif /* ENABLE_LIXIAN */
			if(item_type == UDIT_PEER_RES||item_type == UDIT_SERVER_RES)
			{
				dt_get_task_extra_res_pos( item_type,extra_item,user_data,	 p_task->_task_info->_user_data_len,&extra_item_pos);
				if(extra_item_pos!=NULL && dt_get_task_type(p_task)!=TT_LAN)
				{
					/* 相同资源已经存在 */
					EM_SAFE_DELETE(user_data);
					return SUCCESS;
				}
			}
			else
			{
				dt_get_task_extra_item_pos(item_type,user_data,p_task->_task_info->_user_data_len,&extra_item_pos);
			}
		}
	}
	else
	{
		off_set = sizeof(USER_DATA_STORE_HEAD);
	}

	if(extra_item_pos==NULL)
	{
		if(extra_item== NULL)
		{
			EM_SAFE_DELETE(user_data);
			return SUCCESS;
		}
		user_data_head._item_num++;
		em_memcpy(user_data,&user_data_head,sizeof(USER_DATA_STORE_HEAD));
		em_memcpy(user_data+off_set,&item_head,sizeof(USER_DATA_ITEM_HEAD));
		off_set +=sizeof(USER_DATA_ITEM_HEAD);

		em_memcpy(user_data+off_set,extra_item,dt_get_sizeof_extra_item(item_type));
		off_set +=dt_get_sizeof_extra_item(item_type);
	}
	else
	{
		em_memcpy(extra_item_pos,extra_item,dt_get_sizeof_extra_item(item_type));
	}

	user_data_len = off_set;

	dt_save_task_user_data_to_file(p_task,user_data,user_data_len);
	
	if(!p_task->_task_info->_full_info)
	{
		EM_SAFE_DELETE(user_data);
	}
	return ret_val;

ErrHandler:
	EM_SAFE_DELETE(user_data);
	return ret_val;
}

_int32 dt_get_task_extra_item(EM_TASK * p_task,USER_DATA_ITEM_TYPE item_type,void * extra_item)
{
	_int32 ret_val = SUCCESS;
	  _u8 * user_data = NULL,* extra_item_pos = NULL;
	  _u32 user_data_len = 0;
	  EM_BT_TASK * p_bt_task = NULL;
	  EM_P2SP_TASK * p_p2sp_task = NULL;

      EM_LOG_DEBUG("dt_get_task_extra_item"); 

	  if(p_task->_task_info->_have_user_data)
	  {
		if(p_task->_task_info->_type== TT_BT)
		{
			if(p_task->_task_info->_full_info)
			{
				p_bt_task = (EM_BT_TASK *)p_task->_task_info;
				user_data = p_bt_task->_user_data;
			}
			else
			{
				ret_val = em_malloc(p_task->_task_info->_user_data_len, (void**)&user_data);
				CHECK_VALUE(ret_val);
				user_data_len = p_task->_task_info->_user_data_len;
				ret_val = dt_get_bt_task_user_data_from_file(p_task,user_data,&user_data_len);
				CHECK_VALUE(ret_val);
			}
		}
		else
		{
			if(p_task->_task_info->_full_info)
			{
				p_p2sp_task = (EM_P2SP_TASK *)p_task->_task_info;
				user_data = p_p2sp_task->_user_data;;
			}
			else
			{
				ret_val = em_malloc(p_task->_task_info->_user_data_len, (void**)&user_data);
				CHECK_VALUE(ret_val);
				user_data_len = p_task->_task_info->_user_data_len;
				ret_val =  dt_get_p2sp_task_user_data_from_file(p_task,user_data,&user_data_len);
				CHECK_VALUE(ret_val);
			}
		}
		
		#ifdef ENABLE_LIXIAN
		if(item_type == UDIT_LIXIAN_ID)
		{
			dt_get_task_lixian_id_pos( item_type,extra_item,user_data,	 p_task->_task_info->_user_data_len,&extra_item_pos);
		}
		else
		#endif /* ENABLE_LIXIAN */
		{
			dt_get_task_extra_item_pos(item_type,user_data,p_task->_task_info->_user_data_len,&extra_item_pos);
		}
		
		if(extra_item_pos)
		{
			em_memcpy(extra_item,extra_item_pos,dt_get_sizeof_extra_item(item_type));
		}

		if(!p_task->_task_info->_full_info)
		{
			EM_SAFE_DELETE(user_data);
		}
	  }
	  else
	  {
	  	return INVALID_USER_DATA;
	  }

	return SUCCESS;
}


#ifdef ENABLE_LIXIAN
_int32 dt_get_task_lixian_id_pos(USER_DATA_ITEM_TYPE item_type,void * extra_item,_u8 * user_data,	_u32 user_data_len,_u8 ** extra_item_pos)
{
	_int32 ret_val = SUCCESS;
	USER_DATA_STORE_HEAD user_data_head;
	USER_DATA_STORE_HEAD * p_user_data_head = &user_data_head;
	DT_LIXIAN_ID * p_lx_id = (DT_LIXIAN_ID *)extra_item;
	DT_LIXIAN_ID lixian_id_tmp = {0};
	
	*extra_item_pos = NULL;

	if(user_data_len<=sizeof(USER_DATA_STORE_HEAD)+sizeof(USER_DATA_ITEM_HEAD))
	{
		return INVALID_USER_DATA;
	}
	
	sd_memcpy(&user_data_head,user_data,sizeof(USER_DATA_STORE_HEAD));
	if(p_user_data_head->_PACKING_1!=PACKING_1_VALUE||p_user_data_head->_PACKING_2!=PACKING_2_VALUE||p_user_data_head->_item_num==0)
	{
		return INVALID_USER_DATA;
	}

	ret_val =  dt_get_task_next_extra_item_pos( item_type,user_data+sizeof(USER_DATA_STORE_HEAD),user_data_len-sizeof(USER_DATA_STORE_HEAD),extra_item_pos);
	if(ret_val!=SUCCESS) return ret_val;
	while(*extra_item_pos)
	{
		em_memcpy(&lixian_id_tmp,*extra_item_pos,sizeof(lixian_id_tmp));
		if(lixian_id_tmp._file_index == p_lx_id->_file_index)
		{
			return SUCCESS;
		}
		ret_val =  dt_get_task_next_extra_item_pos( item_type,(*extra_item_pos)+sizeof(DT_LIXIAN_ID),user_data_len-((*extra_item_pos)+sizeof(DT_LIXIAN_ID)-user_data),extra_item_pos);
		if(ret_val!=SUCCESS) return ret_val;
	}
	
	return INVALID_USER_DATA;
}	
_int32 dt_set_task_lixian_mode_impl(EM_TASK * p_task,BOOL is_lixian)
{
	_int32 ret_val = SUCCESS;

	ret_val = dt_set_task_extra_item( p_task,UDIT_LIXIAN_MODE,&is_lixian);
	return ret_val;
}

_int32 dt_is_lixian_task_impl(EM_TASK * p_task,BOOL * is_lixian)
{
	_int32 ret_val = SUCCESS;

	ret_val = dt_get_task_extra_item(p_task,UDIT_LIXIAN_MODE,is_lixian);

	return ret_val;
}

	
_int32 dt_set_lixian_task_id_impl(EM_TASK * p_task,_u32 file_index,_u64 lixian_id)
{
	_int32 ret_val = SUCCESS;
	DT_LIXIAN_ID lixian_id_tmp;
	
	lixian_id_tmp._file_index = file_index;
	lixian_id_tmp._lixian_id = lixian_id;

	ret_val = dt_set_task_extra_item( p_task,UDIT_LIXIAN_ID,&lixian_id_tmp);
	return ret_val;
}

_int32 dt_get_lixian_task_id_impl(EM_TASK * p_task,_u32 file_index,_u64 * p_lixian_id)
{
	_int32 ret_val = SUCCESS;
	DT_LIXIAN_ID lixian_id_tmp;
	
	lixian_id_tmp._file_index = file_index;
	lixian_id_tmp._lixian_id = 0;

	ret_val = dt_get_task_extra_item(p_task,UDIT_LIXIAN_ID,&lixian_id_tmp);
	if(ret_val!=SUCCESS) return ret_val;

	*p_lixian_id = lixian_id_tmp._lixian_id;
	
	return ret_val;
}
#endif /* ENABLE_LIXIAN */

///////////////////////////

_int32 dt_set_task_ad_mode_impl(EM_TASK * p_task,BOOL is_ad)
{
	_int32 ret_val = SUCCESS;

	ret_val = dt_set_task_extra_item( p_task, UDIT_AD_MODE,&is_ad);
	return ret_val;
}

_int32 dt_is_ad_task_impl(EM_TASK * p_task,BOOL * is_ad)
{
	_int32 ret_val = SUCCESS;

	ret_val = dt_get_task_extra_item(p_task, UDIT_AD_MODE, is_ad);

	return ret_val;
}

_int32 dt_vod_get_download_mode_impl(_u8 * user_data, _u32 user_data_len, _u8 ** download_mode)
{
	return dt_get_task_extra_item_pos(UDIT_VOD_DOWNLOAD_MODE,user_data,user_data_len,download_mode);
}	
_int32 dt_vod_set_download_mode_impl(EM_TASK * p_task)
{
	_int32 ret_val = SUCCESS;
	ret_val = dt_set_task_extra_item( p_task,UDIT_VOD_DOWNLOAD_MODE,&p_task->_download_mode);
	return ret_val;
}
_int32 dt_set_task_download_mode(_u32 task_id,BOOL is_download,_u32 file_retain_time)
{
	_int32 ret_val = SUCCESS;
	EM_TASK * p_task = NULL;
	_u32 timestamp = 0;

	EM_LOG_DEBUG("dt_set_task_download_mode:%u,%d", task_id, is_download);

	p_task = dt_get_task_from_map(task_id);
	
	if(p_task==NULL) 
	{
		ret_val = INVALID_TASK_ID;
		goto ErrorHanle;
	}

	/* 普通下载任务不能设置另存下载模式*/
	if(p_task->_task_info->_task_id <= MAX_DL_TASK_ID) 
	{
		ret_val = INVALID_TASK_ID;
		goto ErrorHanle;
	}

	/* 无盘vod 任务不能设置另存下载模式*/
	if(dt_is_vod_task_no_disk(p_task))
	{
		ret_val = INVALID_TASK;
		goto ErrorHanle;
	}

	/* 删除该文件在vod cache 中的占用空间计算*/
	if(is_download!=p_task->_download_mode._is_download)
	{
		if(is_download)
		{
			dt_decrease_vod_task_num(p_task);
			/* 移到顺序下载列表的末尾 */
			dt_pri_level_down_impl(task_id,MAX_U32);
		}
		else
		{
			dt_increase_vod_task_num(p_task);
		}
	}

	if(is_download && file_retain_time == 0)
	{
		file_retain_time = MAX_U32;
	}
		
	em_time(&timestamp);
	p_task->_download_mode._is_download = is_download;
	p_task->_download_mode._set_timestamp = timestamp;
	p_task->_download_mode._file_retain_time = file_retain_time;

	ret_val = dt_vod_set_download_mode_impl(p_task);
	
ErrorHanle:

	return ret_val;
}
_int32 dt_get_task_download_mode(_u32 task_id,BOOL* is_download,_u32* file_retain_time)
{
	_int32 ret_val = SUCCESS;
	EM_TASK * p_task = NULL;
	_u32 timestamp = 0;

	EM_LOG_DEBUG("dt_get_task_download_mode:%u", task_id);

	p_task = dt_get_task_from_map(task_id);
	
	if(p_task==NULL) 
	{
		ret_val = INVALID_TASK_ID;
		goto ErrorHanle;
	}
	
	if(p_task->_task_info->_task_id <= MAX_DL_TASK_ID) 
	{
		/* 普通下载任务 */
		*is_download = TRUE;
		*file_retain_time = MAX_U32;
		goto ErrorHanle;
	}

	/* 无盘vod 任务没有下载模式*/
	if(dt_is_vod_task_no_disk(p_task))
	{
		ret_val = INVALID_TASK;
		goto ErrorHanle;
	}
	
	em_time(&timestamp);
	*is_download = p_task->_download_mode._is_download;
	if( *is_download && p_task->_download_mode._file_retain_time != MAX_U32)
	{
		*file_retain_time = p_task->_download_mode._file_retain_time - (timestamp - p_task->_download_mode._set_timestamp);
	}
	else
	{
		*file_retain_time = 0;
	}
	
ErrorHanle:
	return ret_val;
}
_int32 dt_stop_vod_task_impl( EM_TASK * p_task )
{
	_int32 ret_val = SUCCESS;
	TASK_STATE state;

	state = dt_get_task_state(p_task);
	if(TS_TASK_SUCCESS == state)
	{
		if(p_task->_inner_id!=0)
		{
			em_close_ex(p_task->_inner_id);
			p_task->_inner_id = 0;
		}
	}

	switch(state)
	{
		case TS_TASK_WAITING:
		//case TS_TASK_RUNNING:
			dt_stop_task_impl(p_task);
		case TS_TASK_PAUSED:
		case TS_TASK_FAILED:
		case TS_TASK_SUCCESS:
		case TS_TASK_DELETED:
			/* 如果是无盘点播任务,直接销毁 */
			if(dt_is_vod_task_no_disk(p_task))
			{
				dt_destroy_vod_task(p_task);
			}
			break;
		case TS_TASK_RUNNING:
			p_task->_waiting_stop = TRUE;
			dt_have_task_waiting_stop();
			//dt_stop_task_impl(p_task);
			break;
	}
	
	return ret_val;
}




_int32 dt_set_vod_cache_path_impl( const char *cache_path  )
{
	_int32 ret_val = SUCCESS;
	ret_val = em_test_path_writable(cache_path);
	CHECK_VALUE(ret_val);
	ret_val = em_settings_set_str_item("system.vod_cache_path", (char*)cache_path);
	return ret_val;
}

char *dt_get_vod_cache_path_for_lan(void)
{
    _int32 ret_val = SUCCESS;
	static char vod_cache_path[MAX_FILE_PATH_LEN];
	em_memset(vod_cache_path,0x00,MAX_FILE_PATH_LEN);

	ret_val = em_settings_get_str_item("system.vod_cache_path", vod_cache_path);
    if(em_strlen(vod_cache_path) == 0)
	{
		return NULL;
	}
    return vod_cache_path;
}

char * dt_get_vod_cache_path_impl( void  )
{
	_int32 ret_val = SUCCESS;
	static char vod_cache_path[MAX_FILE_PATH_LEN];
	em_memset(vod_cache_path,0x00,MAX_FILE_PATH_LEN);
	#ifdef KANKAN_PROJ
	ret_val = em_settings_get_str_item("system.vod_cache_path", vod_cache_path);
	#else
	ret_val = em_settings_get_str_item("system.download_path", vod_cache_path);
	#endif /* KANKAN_PROJ */
	if(em_strlen(vod_cache_path) == 0)
	{
		return NULL;
	}
/*	if(em_strlen(vod_cache_path)==0)
	{
		ret_val=em_settings_get_str_item("system.download_path", vod_cache_path);
		if(em_strlen(vod_cache_path)==0)
		{
			char * p_sys_path = em_get_system_path();
			em_strncpy(vod_cache_path,em_get_system_path(),MAX_FILE_PATH_LEN);
			if(em_strlen(vod_cache_path)==0)
			{
				return NULL;
			}
		}
	}*/
	return vod_cache_path;
}


#if defined(ENABLE_NULL_PATH)
_int32 dt_et_get_task_info_impl(_u32 task_id, EM_TASK_INFO * p_em_task_info)
{
	//char *file_path=NULL,*file_name=NULL;
	//P2SP_TASK * p_p2sp_task = NULL;
	//BT_TASK * p_bt_task = NULL;
	_int32 ret_val = SUCCESS;
	ET_TASK _task_info;
	
	ret_val = et_get_task_info(task_id, &_task_info);
	if(SUCCESS == ret_val)
	{
		p_em_task_info->_task_id = _task_info._task_id;
		p_em_task_info->_state= _task_info._task_status; //p_task->_task_info->_state; //
		p_em_task_info->_type= TT_TCID; //dt_get_task_type(p_task);
		
		p_em_task_info->_is_deleted= FALSE;
		
		p_em_task_info->_file_size= _task_info._file_size;
		p_em_task_info->_downloaded_data_size= _task_info._downloaded_data_size;
		p_em_task_info->_start_time= _task_info._start_time;
		p_em_task_info->_finished_time= _task_info._finished_time;
		p_em_task_info->_failed_code= _task_info._failed_code;
		p_em_task_info->_bt_total_file_num= 0;

		p_em_task_info->_is_no_disk= TRUE;
		p_em_task_info->_file_name[0] = '\0';
		p_em_task_info->_file_path[0] = '\0';
       }
       return ret_val;
}
#endif

#ifdef ENABLE_HSC
_int32 dt_get_hsc_mode_impl(_u8 * user_data, _u32 user_data_len, _u8 ** hsc_mode)
{
	return dt_get_task_extra_item_pos(UDIT_HSC_MODE,user_data,user_data_len,hsc_mode);
}	

_int32 dt_set_hsc_mode_impl(EM_TASK * p_task)
{
	_int32 ret_val = SUCCESS;
	ret_val = dt_set_task_extra_item( p_task,UDIT_HSC_MODE,&p_task->_use_hsc);
	return ret_val;
}

#endif /* ENABLE_HSC */

