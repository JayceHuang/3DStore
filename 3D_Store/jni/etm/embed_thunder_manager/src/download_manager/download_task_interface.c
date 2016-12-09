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


#include "download_manager/download_task_interface.h"
#include "download_manager/download_task_data.h"
#include "download_manager/download_task_inner.h"
#include "download_manager/download_task_impl.h"
#include "download_manager/download_task_store.h"

#include "torrent_parser/torrent_parser_interface.h"
#include "scheduler/scheduler.h"
#include "vod_task/vod_task.h"

#include "em_interface/em_task.h"
#include "em_asyn_frame/em_msg.h"
#include "em_asyn_frame/em_notice.h"
#include "em_common/em_utility.h"
#include "em_common/em_time.h"
#include "em_common/em_logid.h"
#ifdef EM_LOGID
#undef EM_LOGID
#endif
#define EM_LOGID EM_LOGID_DOWNLOAD_TASK

#include "em_common/em_logger.h"

/*--------------------------------------------------------------------------*/
/*                Global declarations				        */
/*--------------------------------------------------------------------------*/

static BOOL g_task_locked=FALSE;
static _int32			g_task_thread_id = 0;
static  CREATE_TASK * gp_urgent_param=NULL;
static _u32 urgent_task_id=0;
static _u32 g_dt_do_next_msg_id=0;
static BOOL g_need_correct_path = FALSE;

/*--------------------------------------------------------------------------*/
/*                Interfaces provid for external program				        */
/*--------------------------------------------------------------------------*/

_int32 init_download_manager_module(void)
{
	_int32 ret_val = SUCCESS;
	EM_LOG_DEBUG( "init_download_manager_module" );

	ret_val = dt_init_slabs();
	CHECK_VALUE(ret_val);
	
	g_dt_do_next_msg_id=0;	
	g_need_correct_path = FALSE;
	ret_val = dt_init();
	if(ret_val!=SUCCESS) goto ErrorHanle;

	g_task_locked=FALSE;

	return SUCCESS;
	
ErrorHanle:
	dt_uninit_slabs();
	CHECK_VALUE(ret_val);
	return ret_val;
}
_int32 uninit_download_manager_module(void)
{
	EM_LOG_DEBUG( "uninit_download_manager_module" );

	if(g_dt_do_next_msg_id!=0)
	{
		em_cancel_message_by_msgid(g_dt_do_next_msg_id);
		g_dt_do_next_msg_id=0;
	}

	if(g_task_locked)
	{
		g_task_locked=FALSE;
		em_sleep(5);
	}

	if(g_task_thread_id!=0)
	{
		em_finish_task(g_task_thread_id);
		g_task_thread_id=0;
	}

	if(gp_urgent_param)
	{
		dt_delete_urgent_task_param();
	}
	
	dt_uninit();
	
	dt_uninit_slabs();
	
	return SUCCESS;
}

/* 预占1MB 的空间用于确保程序能顺利启动 */
void dt_create_empty_disk_file(void * parm)
{
	_int32 ret_val = SUCCESS;
	_u32 file_id=0;
	_u64 free_size = 0;
	char file_buffer[MAX_FULL_PATH_BUFFER_LEN];
	char * system_path = (char*)parm;
	_u64 filesize = 0;
	em_pthread_detach();
	em_ignore_signal();
		
	EM_LOG_DEBUG( "dt_create_empty_disk_file" );

	if(system_path == NULL || em_strlen(system_path) == 0)
	{
		return;
	}
	
	ret_val = sd_get_free_disk(system_path, &free_size);
	if(ret_val!=SUCCESS) return;

	if(free_size<ETM_EMPTY_DISK_FILE_SIZE/1024)
	{
		EM_LOG_ERROR( "dt_create_empty_disk_file:No space left on device!" );
		return;
	}
	
	em_memset(file_buffer,0,MAX_FULL_PATH_BUFFER_LEN);

	em_snprintf(file_buffer, MAX_FULL_PATH_BUFFER_LEN, "%s/%s", system_path,ETM_EMPTY_DISK_FILE);
	if(em_file_exist(file_buffer)) return;
	
	ret_val=em_open_ex(file_buffer, O_FS_CREATE, &file_id);
	if (ret_val != SUCCESS) return;
	em_enlarge_file( file_id, ETM_EMPTY_DISK_FILE_SIZE, &filesize);
	em_close_ex(file_id);
    EM_LOG_DEBUG("dt_create_empty_disk_file SUCCESS!"); 
	return ;
}


#if defined( __SYMBIAN32__)
_int32 dt_load_tasks_loop(void * parm)
{
	_int32 ret_val = SUCCESS;
	if(g_task_locked)  return ret_val;

	EM_LOG_DEBUG( "dt_load_tasks_loop" );
	g_task_locked=TRUE;

READ_AGAIN:
		
	ret_val = dt_load_tasks_from_file();
	//sd_assert(ret_val==SUCCESS);
	if(ret_val==SUCCESS)
	{
		ret_val = dt_load_order_list();
		//sd_assert(ret_val==SUCCESS);
		if(ret_val!=SUCCESS)
		{
      			EM_LOG_ERROR("dt_load_tasks_loop:ERROR:ret_val=%d need recover the task file",ret_val); 
			dt_clear_eigenvalue();
			dt_clear_task_map();
			if(dt_recover_file())
			{
				goto READ_AGAIN;
			}
			else
			{
      				EM_LOG_ERROR("dt_load_tasks_loop:recover the task file FAILED!need dt_create_task_file "); 
				dt_create_task_file();
				dt_save_total_task_num();
			}
		}
	}
	else
	{
      		EM_LOG_ERROR("dt_load_tasks_loop:ERROR:ret_val=%d",ret_val); 
		//sd_assert(FALSE);
		dt_clear_eigenvalue();
		dt_clear_task_map();
		if(dt_recover_file())
		{
			goto READ_AGAIN;
		}
		else
		{
  			EM_LOG_ERROR("dt_load_tasks_loop 2:recover the task file FAILED!need dt_create_task_file "); 
			dt_create_task_file();
			dt_save_total_task_num();
		}
	}
	g_task_locked=FALSE;
	dt_close_task_file(TRUE);

	return SUCCESS;
}
#else
void dt_load_tasks_loop(void * parm)
{
	_int32 ret_val = SUCCESS;
	_int32 empty_disk_file_thread_id = 0;	
	
	if(g_task_locked)  return;

	em_ignore_signal();
		
	EM_LOG_DEBUG( "dt_load_tasks_loop" );
	g_task_locked=TRUE;
	
READ_AGAIN:
		
	ret_val = dt_load_tasks_from_file();
	sd_assert(ret_val==SUCCESS);
	if(!g_task_locked) return;
	if(ret_val==SUCCESS)
	{
		ret_val = dt_load_order_list();
		sd_assert(ret_val==SUCCESS);
		if(ret_val!=SUCCESS)
		{
      			EM_LOG_ERROR("dt_load_tasks_loop:ERROR:ret_val=%d need recover the task file",ret_val); 
			dt_clear_eigenvalue();
			dt_clear_task_map();
			if(dt_recover_file())
			{
				goto READ_AGAIN;
			}
			else
			{
      				EM_LOG_ERROR("dt_load_tasks_loop:recover the task file FAILED!need dt_create_task_file "); 
				dt_create_task_file();
				dt_save_total_task_num();
			}
		}
		dt_load_task_vod_extra_mode();
	}
	else
	{
      		EM_LOG_ERROR("dt_load_tasks_loop:ERROR:ret_val=%d",ret_val); 
		sd_assert(FALSE);
		dt_clear_eigenvalue();
		dt_clear_task_map();
		if(dt_recover_file())
		{
			goto READ_AGAIN;
		}
		else
		{
  			EM_LOG_ERROR("dt_load_tasks_loop 2:recover the task file FAILED!need dt_create_task_file "); 
			dt_create_task_file();
			dt_save_total_task_num();
		}
	}
	
	g_task_locked=FALSE;
	dt_close_task_file(TRUE);
	
	em_create_task(dt_create_empty_disk_file, 4096, (void*)em_get_system_path(), &empty_disk_file_thread_id);
	return ;
}

#endif /* symbian */

_int32 dt_load_tasks(void)
{
	_int32 ret_val = SUCCESS,count = 0;
	sd_assert(g_task_thread_id==0);
	dt_close_task_file(TRUE);
	ret_val = em_create_task(dt_load_tasks_loop, 4096, NULL, &g_task_thread_id);
	CHECK_VALUE(ret_val);
	
	while(!g_task_locked&&(count++<100))
		em_sleep(1);
	
	return ret_val;
}
_int32 dt_start_tasks(void)
{
	if(g_task_locked) return ETM_BUSY;
	return dt_load_running_tasks();
}
_int32 dt_restart_tasks(void)
{
	_int32 ret_val = SUCCESS;
	
	if(g_task_locked) return ETM_BUSY;
	
	ret_val = dt_load_running_tasks();
	CHECK_VALUE(ret_val);
	
	/* save tasks if changed */
	ret_val=dt_save_tasks();
	return ret_val;
}

_int32 dt_clear(void)
{
	_int32 ret_val = SUCCESS;
	EM_LOG_DEBUG( "dt_clear" );

	if(g_task_locked)
	{
		g_task_locked=FALSE;
		em_sleep(5);
	}

	if(g_task_thread_id!=0)
	{
		em_finish_task(g_task_thread_id);
		g_task_thread_id=0;
	}

	if(gp_urgent_param)
	{
		dt_delete_urgent_task_param();
	}
	
	dt_save_total_task_num();
	
	/* save the running tasks */
	dt_save_running_tasks(TRUE);
	
	/* save the dling_task_order_list if changed */
	dt_save_order_list();
	
	/* stop all tasks  */
	dt_stop_tasks();
	
	/* save tasks if changed */
	dt_save_tasks();
	
	/* close the task_store file if long time no action */
	dt_close_task_file(TRUE);

	dt_clear_order_list();
	
	dt_clear_eigenvalue();
	
	dt_clear_task_map();
	
	/* clear all list and map  */
	dt_clear_cache();
	
	dt_clear_file_path_cache();
	
	dt_clear_file_name_cache();
	
	return ret_val;
	
}

_int32 dt_clear_running_tasks_before_restart_et(void)
{
	_int32 ret_val = SUCCESS;
	
	if(g_task_locked)
	{
		g_task_locked=FALSE;
		em_sleep(5);
	}

	if(g_task_thread_id!=0)
	{
		em_finish_task(g_task_thread_id);
		g_task_thread_id=0;
	}

	if(gp_urgent_param)
	{
		dt_delete_urgent_task_param();
	}
	/* save the running tasks */
	dt_save_running_tasks(TRUE);
	
	/* save the dling_task_order_list if changed */
	dt_save_order_list();
	
	/* stop all tasks  */
	dt_stop_tasks();
	
	dt_set_need_notify_state_changed(FALSE);
	
	/* save tasks if changed */
	dt_save_tasks();


	return ret_val;
}
_int32 dt_stop_all_waiting_tasks(void)
{
	return dt_stop_all_waiting_tasks_impl();
}
_int32 dt_clear_up_task_file(void)
{
	_int32 ret_val = SUCCESS;
	
	if(dt_have_running_task()) return SUCCESS;

	if(!dt_is_task_file_need_clear_up()) return SUCCESS;

	if(g_task_locked) return SUCCESS;
	
	EM_LOG_DEBUG( "dt_clear_up_task_file" );

	dt_clear();
	
	g_task_locked = TRUE;
	
	ret_val = dt_clear_task_file_impl(NULL);
	sd_assert(ret_val==SUCCESS);
	#if 0
	if(ret_val == SUCCESS)
	{
		  ret_val = dt_check_task_file(dt_get_task_num_in_map());
		  if(ret_val!=SUCCESS)	
		  {
		  	if(!dt_recover_file())
		  	{
				dt_create_task_file();
				dt_save_total_task_num();
		  	}
		  }
		  else
		  {
		  	dt_backup_file();
		  }
	}
	else
	{
	  	if(!dt_recover_file())
	  	{
			dt_create_task_file();
			dt_save_total_task_num();
	  	}
	}
	#endif
	g_task_locked = FALSE;
	
	dt_load_tasks_loop(NULL);
	
	EM_LOG_DEBUG( "dt_clear_up_task_file SUCCESS!" );
	return SUCCESS;
}

void dt_scheduler(void)
{
//	_int32 ret_val = SUCCESS;
	static BOOL call_flag = FALSE;
	EM_LOG_DEBUG( "dt_scheduler start,g_task_locked=%d,call_flag=%d,g_task_thread_id=%d",g_task_locked ,call_flag,g_task_thread_id);

	if(g_task_locked) return;

	sd_assert(call_flag == FALSE);
	if(call_flag) return;
	call_flag = TRUE;
	
	if(g_task_thread_id!=0)
	{
		em_finish_task(g_task_thread_id);
		g_task_thread_id=0;
	}

	if(g_need_correct_path)
	{
		dt_correct_all_tasks_path();
		g_need_correct_path = FALSE;
	}

	if(gp_urgent_param) 
	{
		dt_create_urgent_task_impl();
	}

	/* update task info */
	dt_update_running_task();
	
	/* clear SUCCESS or FAILED tasks */
	dt_clear_dead_task();

	if(em_is_license_ok()==TRUE&&em_is_net_ok(FALSE))
	{
		if((em_is_task_auto_start()==TRUE)&&( dt_is_running_tasks_loaded()==FALSE))
		{
			dt_start_tasks();
			
			dt_set_need_notify_state_changed(TRUE);
			/* start next tasks in the dling_task_order_list */
			dt_start_next_task();
			
#ifdef ENABLE_ETM_MEDIA_CENTER			
			/* 所有任务状态处理完通知，使用特殊task id为0*/
			// em_notify_task_state_changed(0, NULL);
#endif
		}
		else
		{
			dt_set_need_notify_state_changed(TRUE);
			/* start next tasks in the dling_task_order_list */
			dt_start_next_task();
		}
	}

	dt_save_running_tasks(FALSE);
	
	/* save the dling_task_order_list if changed */
	dt_save_order_list();
	
	/* save tasks if changed */
	dt_save_tasks();

	/* check if vod cache is full */
//	dt_clear_vod_cache_impl(FALSE);
	
	/* close the task_store file if long time no action */
	dt_close_task_file(FALSE);

	dt_clear_up_task_file();

	/* report to remote */
	dt_report_to_remote();

	call_flag = FALSE;
	
	EM_LOG_DEBUG( "dt_scheduler end!");
	return;
	
}
_int32 dt_delete_urgent_task_param( void )
{
	EM_SAFE_DELETE(gp_urgent_param->_file_path);
	EM_SAFE_DELETE(gp_urgent_param->_file_name);
	EM_SAFE_DELETE(gp_urgent_param->_url);
	EM_SAFE_DELETE(gp_urgent_param->_ref_url);
	EM_SAFE_DELETE(gp_urgent_param->_seed_file_full_path);
	EM_SAFE_DELETE(gp_urgent_param->_download_file_index_array);
	EM_SAFE_DELETE(gp_urgent_param->_tcid);
	EM_SAFE_DELETE(gp_urgent_param->_gcid);
	EM_SAFE_DELETE(gp_urgent_param->_user_data);
#ifdef ENABLE_ETM_MEDIA_CENTER
	EM_SAFE_DELETE(gp_urgent_param->_group_name);
#endif
	EM_SAFE_DELETE(gp_urgent_param);
	urgent_task_id = 0;
	return SUCCESS;
}

_int32 dt_create_urgent_task_impl( void )
{
	gp_urgent_param->_check_data = FALSE;//_check_data 用于_is_no_disk -----zyq @20101027 fro vod task
	dt_create_task_impl(gp_urgent_param,&urgent_task_id,FALSE,FALSE,FALSE);
	
	dt_delete_urgent_task_param(  );
	return SUCCESS;
}
_int32 dt_create_urgent_task( CREATE_TASK * p_create_param,_u32* p_task_id ,BOOL is_remote)
{
	_int32 ret_val = SUCCESS;
	_u8  eigenvalue[CID_SIZE];
	char default_path[MAX_FULL_PATH_BUFFER_LEN];
	TORRENT_SEED_INFO *p_seed_info;
	_u16 *p_need_dl_file_index_array=NULL;
	_u16 need_dl_file_num=0;
	_u32 encoding_mode = 2;

	EM_LOG_DEBUG("dt_create_urgent_task:%d",p_create_param->_type);

	if(gp_urgent_param) return ETM_BUSY;

	if(p_create_param->_type==TT_BT)
	{
#ifdef ENABLE_BT
		if((p_create_param->_seed_file_full_path==NULL)||(em_strlen(p_create_param->_seed_file_full_path)==0)||(p_create_param->_seed_file_full_path_len==0)||(p_create_param->_seed_file_full_path_len>=MAX_FULL_PATH_LEN))
		{
			return INVALID_SEED_FILE;
		}
		
		em_memset(default_path, 0, MAX_FULL_PATH_BUFFER_LEN);
		em_strncpy(default_path, p_create_param->_seed_file_full_path, p_create_param->_seed_file_full_path_len);
		em_settings_get_int_item("system.encoding_mode", (_int32*)&encoding_mode);
		ret_val= tp_get_seed_info( default_path,encoding_mode, &p_seed_info );
		if(ret_val!=SUCCESS) return ret_val;
		if(p_seed_info->_file_num==0||p_seed_info->_file_total_size==0)
		{
			tp_release_seed_info( p_seed_info );
			return INVALID_SEED_FILE;
		}
		
		if((p_create_param->_download_file_index_array!=NULL)&&(p_create_param->_file_num!=0))
		{
			ret_val = dt_check_and_sort_bt_file_index(p_create_param->_download_file_index_array,p_create_param->_file_num,p_seed_info->_file_num,&p_need_dl_file_index_array,&need_dl_file_num);
			if(ret_val!=SUCCESS) 
			{
				tp_release_seed_info( p_seed_info );
				return ret_val;
			}
			EM_SAFE_DELETE(p_need_dl_file_index_array);
		}
		tp_release_seed_info( p_seed_info );
#endif
	}
	else
	{
		ret_val = dt_generate_eigenvalue(p_create_param,eigenvalue);
		if(ret_val!=SUCCESS) return ret_val;

		if((p_create_param->_type==TT_URL)||(p_create_param->_type==TT_EMULE)||(p_create_param->_type==TT_LAN))
		{
				/* url */
				if((p_create_param->_url!=NULL)&&(em_strlen(p_create_param->_url)!=0)&&(p_create_param->_url_len!=0)&&(p_create_param->_url_len<MAX_URL_LEN))
				{
					ret_val = SUCCESS;
				}
				else
				{
					/* no url */
					return INVALID_URL;
				}
		}
		else
		if(p_create_param->_type==TT_KANKAN)
		{
				/* gcid */
				if(em_string_to_cid(p_create_param->_tcid,eigenvalue)!=0)
				{
					return INVALID_TCID;
				}
				
				if(p_create_param->_file_size==0)
				{
					return EM_INVALID_FILE_SIZE;
				}
		}
	}
	
		/*  file path */
		if((p_create_param->_file_path!=NULL)&&(em_strlen(p_create_param->_file_path)!=0)&&(p_create_param->_file_path_len!=0)&&(p_create_param->_file_path_len<MAX_FILE_PATH_LEN))
		{
			em_memset(default_path, 0, MAX_FULL_PATH_BUFFER_LEN);
			em_strncpy(default_path, p_create_param->_file_path, p_create_param->_file_path_len);
			if(FALSE==em_file_exist(default_path))
			{
				return INVALID_FILE_PATH;
			}
		}
		else
		if(p_create_param->_file_path!=NULL)
		{
			return INVALID_FILE_PATH;
		}
	
		if((p_create_param->_file_name!=NULL)&&(em_strlen(p_create_param->_file_name)!=0)&&(p_create_param->_file_name_len!=0)&&(p_create_param->_file_name_len<MAX_FILE_NAME_LEN))
		{
			ret_val = SUCCESS;
		}
		else
		if((p_create_param->_file_name==NULL)&&(p_create_param->_type==TT_URL||p_create_param->_type==TT_BT||p_create_param->_type==TT_EMULE||p_create_param->_type==TT_LAN))
		{
			ret_val = SUCCESS;
		}
		else
		{
			return INVALID_FILE_NAME;
		}


	ret_val = em_malloc(sizeof(CREATE_TASK), (void**)&gp_urgent_param);
	CHECK_VALUE(ret_val);
	em_memcpy(gp_urgent_param, p_create_param, sizeof(CREATE_TASK));

	if(p_create_param->_file_path!=NULL)
	{
		ret_val = em_malloc(p_create_param->_file_path_len+1, (void**)&gp_urgent_param->_file_path);
		if(ret_val!=SUCCESS)
		{
			goto ErrorHanle;
		}
		em_strncpy(gp_urgent_param->_file_path, p_create_param->_file_path, p_create_param->_file_path_len);
		gp_urgent_param->_file_path[p_create_param->_file_path_len]='\0';
	}
	
	if(p_create_param->_file_name!=NULL)
	{
		ret_val = em_malloc(p_create_param->_file_name_len+1, (void**)&gp_urgent_param->_file_name);
		if(ret_val!=SUCCESS)
		{
			goto ErrorHanle;
		}
		em_strncpy(gp_urgent_param->_file_name, p_create_param->_file_name, p_create_param->_file_name_len);
		gp_urgent_param->_file_name[p_create_param->_file_name_len]='\0';
	}
	
	if(p_create_param->_url!=NULL)
	{
		ret_val = em_malloc(p_create_param->_url_len+1, (void**)&gp_urgent_param->_url);
		if(ret_val!=SUCCESS)
		{
			goto ErrorHanle;
		}
		em_strncpy(gp_urgent_param->_url, p_create_param->_url, p_create_param->_url_len);
		gp_urgent_param->_url[p_create_param->_url_len]='\0';
	}
	
	if(p_create_param->_ref_url!=NULL)
	{
		ret_val = em_malloc(p_create_param->_ref_url_len+1, (void**)&gp_urgent_param->_ref_url);
		if(ret_val!=SUCCESS)
		{
			goto ErrorHanle;
		}
		em_strncpy(gp_urgent_param->_ref_url, p_create_param->_ref_url, p_create_param->_ref_url_len);
		gp_urgent_param->_ref_url[p_create_param->_ref_url_len]='\0';
	}
	
	if(p_create_param->_seed_file_full_path!=NULL)
	{
#ifdef ENABLE_BT
		ret_val = em_malloc(p_create_param->_seed_file_full_path_len+1, (void**)&gp_urgent_param->_seed_file_full_path);
		if(ret_val!=SUCCESS)
		{
			goto ErrorHanle;
		}
		em_strncpy(gp_urgent_param->_seed_file_full_path, p_create_param->_seed_file_full_path, p_create_param->_seed_file_full_path_len);
		gp_urgent_param->_seed_file_full_path[p_create_param->_seed_file_full_path_len]='\0';
#endif
	}
	
	if(p_create_param->_download_file_index_array!=NULL)
	{
		ret_val = em_malloc(p_create_param->_file_num*sizeof(_u32), (void**)&gp_urgent_param->_download_file_index_array);
		if(ret_val!=SUCCESS)
		{
			goto ErrorHanle;
		}
		em_memcpy(gp_urgent_param->_download_file_index_array, p_create_param->_download_file_index_array, p_create_param->_file_num*sizeof(_u32));
	}
	
	if(p_create_param->_tcid!=NULL)
	{
		ret_val = em_malloc(41, (void**)&gp_urgent_param->_tcid);
		if(ret_val!=SUCCESS)
		{
			goto ErrorHanle;
		}
		em_memcpy(gp_urgent_param->_tcid, p_create_param->_tcid, 40);
		p_create_param->_tcid[40]='\0';
	}
	
	if(p_create_param->_gcid!=NULL)
	{
		ret_val = em_malloc(41, (void**)&gp_urgent_param->_gcid);
		if(ret_val!=SUCCESS)
		{
			goto ErrorHanle;
		}
		em_memcpy(gp_urgent_param->_gcid, p_create_param->_gcid, 40);
		p_create_param->_gcid[40]='\0';
	}
	
	if(p_create_param->_user_data!=NULL)
	{
		ret_val = em_malloc(p_create_param->_user_data_len, (void**)&gp_urgent_param->_user_data);
		if(ret_val!=SUCCESS)
		{
			goto ErrorHanle;
		}
		em_memcpy(gp_urgent_param->_user_data, p_create_param->_user_data, p_create_param->_user_data_len);
	}
	
#ifdef ENABLE_ETM_MEDIA_CENTER
	if (p_create_param->_group_name != NULL)
	{
		ret_val = em_malloc(p_create_param->_group_name_len, (void**)&gp_urgent_param->_group_name);
		if(ret_val!=SUCCESS)
		{
			goto ErrorHanle;
		}
		sd_memcpy(gp_urgent_param->_group_name, p_create_param->_group_name, p_create_param->_group_name_len);
	}
#endif		
	
	urgent_task_id=dt_create_task_id(is_remote);
	*p_task_id = urgent_task_id;
	return SUCCESS;

ErrorHanle:	
	
	dt_delete_urgent_task_param(  );
	return ret_val;	
}
_int32 dt_create_task_impl( CREATE_TASK * p_create_param,_u32* p_task_id ,BOOL is_vod_task,BOOL is_remote,BOOL create_by_cfg)
{
	_int32 ret_val = SUCCESS;
	_int32 ret_val2 = SUCCESS;
	TASK_INFO * p_task_info = NULL;
	EM_TASK * p_task = NULL;
	_u32 cur_time=0,task_id=0;
	BOOL is_download = FALSE;
	_u32 file_retain_time = 0;

	EM_LOG_DEBUG("dt_create_task_impl:%d,is_vod_task=%d",p_create_param->_type,is_vod_task);

	ret_val = dt_init_task_info(p_create_param,&p_task_info,&task_id,create_by_cfg);
	if(ret_val!=SUCCESS)
	{
		if(ret_val==TASK_ALREADY_EXIST)
		{
			// 如果是因为有盘点播导致任务已存在，则删除有盘点播缓存内容后再创建任务
			if(task_id > MAX_DL_TASK_ID && !is_vod_task )
			{
				ret_val = dt_clear_vod_cache_impl(TRUE);
				if(SUCCESS == ret_val)
				{
					ret_val = dt_create_task_impl(p_create_param, p_task_id, FALSE, FALSE, FALSE);
					return ret_val;
				}
			}
			/*
			*p_task_id = task_id;
			// 正常本地下载任务，将已有的有盘点播任务转换为下载模式。
			if(task_id>MAX_DL_TASK_ID&&!is_vod_task)
			{
				// 下载任务 
				ret_val2 = dt_get_task_download_mode(task_id,&is_download,&file_retain_time);
				sd_assert(ret_val2 == SUCCESS);
				if(ret_val2 == SUCCESS)
				{
					if(is_download==FALSE)
					{
						ret_val2 = dt_set_task_download_mode( task_id,TRUE,0);
						sd_assert(ret_val2 == SUCCESS);
						p_task = dt_get_task_from_map(task_id);
						if (TS_TASK_PAUSED == dt_get_task_state(p_task))
						{
							dt_set_task_state(p_task,TS_TASK_WAITING);
						}
						#ifdef KANKAN_PROJ
						if ((TS_TASK_RUNNING == dt_get_task_state(p_task)) && (dt_get_running_vod_task_num() > 0)) 
						{
							//把正在点播的任务置为下载模式，减少正在下载的点播任务数
							dt_decrease_running_vod_task_num();
						}
						#endif

						// 返回成功
						ret_val = SUCCESS;
					}
				}
			}*/
		}
		goto ErrorHanle_end;
	}

	ret_val = dt_init_task(p_task_info, &p_task);
	if(ret_val!=SUCCESS) goto ErrorHanle;

	if(*p_task_id == 0)
	{
		if(is_vod_task)
		{
			p_task->_task_info->_task_id= dt_create_vod_task_id();
		}
		else
		{
			p_task->_task_info->_task_id= dt_create_task_id(is_remote);
		}
	}
	else
	{
		p_task->_task_info->_task_id= *p_task_id;
	}
	
	ret_val = dt_add_task_to_file(p_task);
	if(ret_val!=SUCCESS) goto ErrorHanle_save;

	/* add to all_task_map */
	ret_val = dt_add_task_to_map(p_task);
	if(ret_val!=SUCCESS) goto ErrorHanle_all_map;
	
	/* add to eigenvalue map */
	ret_val = dt_add_task_eigenvalue(p_task_info->_type,p_task_info->_eigenvalue,p_task_info->_task_id);
	if(ret_val!=SUCCESS) goto ErrorHanle_eigenvalue;
	
	if(p_task_info->_file_name_eigenvalue!=0)
	{
		dt_add_file_name_eigenvalue(p_task_info->_file_name_eigenvalue,p_task_info->_task_id);
	}
	
	if(p_create_param->_type==TT_FILE)
	{
		em_time(&cur_time);
		dt_set_task_start_time(p_task,cur_time);
		dt_set_task_finish_time(p_task,cur_time);
		dt_set_task_state(p_task,TS_TASK_SUCCESS);
	}
	else
	{
		/* add to order list */
		ret_val =dt_add_task_to_order_list(p_task); 
		if(ret_val!=SUCCESS) goto ErrorHanle_order_list;
		if(p_create_param->_manual_start)
			dt_set_task_state(p_task,TS_TASK_PAUSED);
		else
			dt_set_task_state(p_task,TS_TASK_WAITING);
		if(dt_is_lan_task(p_task))
		{
			dt_increase_waiting_lan_task_num();
		}
	}
	*p_task_id = p_task->_task_info->_task_id;

	return SUCCESS;

//ErrorHanle_cache:
	/* remove from dling_order_list */
	//dt_remove_task_from_order_list(p_task);
	
ErrorHanle_order_list:
	/* remove eigenvalue */
	dt_remove_task_eigenvalue(p_task_info->_type,p_task_info->_eigenvalue);
	if(p_task_info->_file_name_eigenvalue!=0)
	{
		dt_remove_file_name_eigenvalue(p_task_info->_file_name_eigenvalue);
	}
	
ErrorHanle_eigenvalue:
	/* remove from all_tasks_map */
	dt_remove_task_from_map(p_task);
	
ErrorHanle_all_map:
	/* disable from task_file */
	dt_disable_task_in_file(p_task);

ErrorHanle_save:

	if(*p_task_id == 0)
	{
		if(is_vod_task)
		{
			dt_decrease_vod_task_id();
		}
		else
		{
			dt_decrease_task_id();
		}
	}
	/* uninit task */
	dt_uninit_task(p_task);
	
ErrorHanle:
	
	dt_uninit_task_info(p_task_info);
	
ErrorHanle_end:
	return ret_val;
	
}

_int32 dt_create_task( void * p_param )
{
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	EM_CREATE_TASK * p_create_param =(EM_CREATE_TASK *)_p_param->_para1;
	_u32* p_task_id =(_u32 *)_p_param->_para2;
    BOOL thread_unlock = (BOOL)_p_param->_para3;
	CREATE_TASK create_task_param;

	em_memset(&create_task_param,0x00,sizeof(CREATE_TASK));
	create_task_param._type = p_create_param->_type;
	create_task_param._file_path = p_create_param->_file_path;
	create_task_param._file_path_len = p_create_param->_file_path_len;
	create_task_param._file_name = p_create_param->_file_name;
	create_task_param._file_name_len = p_create_param->_file_name_len;
	create_task_param._url = p_create_param->_url;
	create_task_param._url_len = p_create_param->_url_len;
	create_task_param._ref_url = p_create_param->_ref_url;
	create_task_param._ref_url_len = p_create_param->_ref_url_len;
	create_task_param._seed_file_full_path = p_create_param->_seed_file_full_path;
	create_task_param._seed_file_full_path_len = p_create_param->_seed_file_full_path_len;
	create_task_param._download_file_index_array = p_create_param->_download_file_index_array;
	create_task_param._file_num = p_create_param->_file_num;
	create_task_param._tcid = p_create_param->_tcid;
	create_task_param._file_size = p_create_param->_file_size;
	create_task_param._gcid = p_create_param->_gcid;
	create_task_param._check_data = p_create_param->_check_data;
	create_task_param._manual_start = p_create_param->_manual_start;
	create_task_param._user_data = p_create_param->_user_data;
	create_task_param._user_data_len = p_create_param->_user_data_len;

	EM_LOG_DEBUG("dt_create_task: task_type = %d",p_create_param->_type);

#ifdef ENABLE_ETM_MEDIA_CENTER
	if(create_task_param._file_path != NULL && !em_file_exist(create_task_param._file_path))
	{
		em_mkdir(create_task_param._file_path);
	}
#endif

	*p_task_id=0;

	if(g_task_locked) 
	{
		_p_param->_result = dt_create_urgent_task( &create_task_param,p_task_id,!thread_unlock );
	}
	else
	{
		create_task_param._check_data = FALSE;//_check_data 用于_is_no_disk -----zyq @20101027 fro vod task
		_p_param->_result = dt_create_task_impl( &create_task_param,p_task_id ,FALSE,!thread_unlock, FALSE);
	}

	EM_LOG_DEBUG("dt_create_task %u end!em_signal_sevent_handle:_result=%d",*p_task_id,_p_param->_result);
	if(_p_param->_result==SUCCESS)
	{
		if(!create_task_param._manual_start)
		{
			/* start net work */
			em_is_net_ok(TRUE);
		}
		
		dt_force_scheduler();
	}
    if(!thread_unlock)
    {
        return _p_param->_result;
    }
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 dt_create_task_by_cfg_file(void * p_param)
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	char * cfg_file_path =(char *)_p_param->_para1;
	_u32* p_task_id =(_u32 *)_p_param->_para2;
	
	BOOL cid_is_valid = FALSE;
	_u8 cid[CID_SIZE*2 +1] = {0};
	char url[MAX_URL_LEN] = {0};
	char lixian_url[MAX_URL_LEN] = {0};
	char cookie[MAX_COOKIE_LEN] = {0};
	//char user_data[MAX_USER_DATA_LEN] = {0};
	char tmp_name[MAX_FILE_NAME_LEN] = {0};
	char file_name[MAX_FILE_NAME_LEN] = {0};
	_u64 file_size = 0;
	
	EM_LOG_DEBUG("dt_create_task_by_cfg_file: cfg_file_path = %s", cfg_file_path);
	CREATE_TASK create_task_param;
	em_memset(&create_task_param,0x00,sizeof(CREATE_TASK));

	*p_task_id = 0;
	if(sd_file_exist(cfg_file_path))
	{
		_p_param->_result = eti_get_info_from_cfg_file(cfg_file_path, url, &cid_is_valid, cid, &file_size,tmp_name, lixian_url, cookie, NULL);
		if(_p_param->_result != SUCCESS)
		{
			return em_signal_sevent_handle(&(_p_param->_handle));
		}
		EM_LOG_DEBUG("dt_create_task_by_cfg_file: url = %s, cid_is_valid = %d, cid = %s, file_size = %llu, file_name = %s", url, cid_is_valid, cid, file_size, tmp_name);
	}
	else
	{
		_p_param->_result = DT_ERR_CFG_FILE_NOT_EXIST;
		return em_signal_sevent_handle(&(_p_param->_handle));
	}
	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		return em_signal_sevent_handle(&(_p_param->_handle));
	}
	else
	{		
		// cid创建的任务，url默认是DEFAULT_LOCAL_URL。
		if(0 != sd_strncmp(url, DEFAULT_LOCAL_URL, sd_strlen(DEFAULT_LOCAL_URL)) )
		{
			if( 0 == sd_strncmp(url, "ed2k://", sd_strlen("ed2k://")))
				create_task_param._type = TT_EMULE;
			else
				create_task_param._type = TT_URL;
		}
		else
		{
			if(cid_is_valid)
			{	
				create_task_param._type = TT_TCID;
				create_task_param._tcid = cid;
			}
			else
			{
				_p_param->_result = DT_ERR_INVALID_TCID;
				return em_signal_sevent_handle(&(_p_param->_handle));
			}
		}
		// 去除获取到的文件名末尾的".rf"后缀
		if( 0 != sd_strstr(tmp_name, ".rf", 0)  )
		{
			sd_strncpy(file_name, tmp_name, sd_strlen(tmp_name) - sd_strlen(".rf"));
			create_task_param._file_name = file_name;
			create_task_param._file_name_len = sd_strlen(file_name);
		}
		else
		{
			create_task_param._file_name = tmp_name;
			create_task_param._file_name_len = sd_strlen(tmp_name);
		}
		create_task_param._file_size = file_size;
		create_task_param._url = url;
		create_task_param._url_len = sd_strlen(url);
		create_task_param._check_data = FALSE;
		//create_task_param._user_data = user_data;
		//create_task_param._user_data_len = sd_strlen(user_data);
		//EM_LOG_DEBUG("dt_create_task_by_cfg_file: userdata_len = %d, userdata = %s", sd_strlen(user_data), user_data);
		_p_param->_result = dt_create_task_impl( &create_task_param,p_task_id ,FALSE, FALSE, TRUE);
	}

	if(_p_param->_result==SUCCESS)
	{
		if(!create_task_param._manual_start)
		{
			/* start net work */
			em_is_net_ok(TRUE);
		}
		dt_force_scheduler();
	}

	if( sd_strlen(lixian_url) > 9 &&  (NULL != sd_strstr(lixian_url, "gdl.lixian.vip.xunlei.com",0)) )
	{
		EM_SERVER_RES server_res = {0};
		_int32 ret = 0;
		EM_TASK *p_task = dt_get_task_from_map(*p_task_id);
		if(NULL != p_task) 
		{
			EM_LOG_DEBUG("dt_create_task_by_cfg_file: lixian_url = %s, cookie = %s", lixian_url, cookie);
			server_res._resource_priority = 106;
			server_res._url = lixian_url;
			server_res._url_len = sd_strlen(lixian_url);
			server_res._cookie = cookie;
			server_res._cookie_len = sd_strlen(cookie);
			
			ret = dt_add_server_resource_impl(p_task, &server_res);
			if(0 != ret)
				EM_LOG_DEBUG("dt_create_task_by_cfg_file, dt_add_server_resource_impl:_result=%d", ret);
		}
	}

	EM_LOG_DEBUG("dt_create_task_by_cfg_file %u end! em_signal_sevent_handle:_result=%d",*p_task_id,_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 dt_use_new_url_create_task(void * p_param)
{
	POST_PARA_5* _p_param = (POST_PARA_5*)p_param;
	char * new_origin_url =(char *)_p_param->_para1;
	EM_TASK_INFO* p_task_info =(EM_TASK_INFO *)_p_param->_para2;
	_u32* p_task_id =(_u32 *)_p_param->_para3;
	char* userdata =(char *)_p_param->_para4;
	_u32 userdata_len =(_u32)_p_param->_para5;
	
	CREATE_TASK create_task_param;
	char cfg_file_path[MAX_FILE_PATH_LEN] = {0};
	
	em_memset(&create_task_param,0x00,sizeof(CREATE_TASK));
	sd_strncpy(cfg_file_path, p_task_info->_file_path, MAX_FILE_PATH_LEN - 1);
	// 找到对应cfg文件读取信息
	if(  '/' != cfg_file_path[sd_strlen(cfg_file_path) -1 ] )
		em_strcat(cfg_file_path, "/", 1);
	em_strcat(cfg_file_path, p_task_info->_file_name, sd_strlen(p_task_info->_file_name));
	em_strcat(cfg_file_path, ".rf.cfg", sd_strlen(".rf.cfg"));
	
	EM_LOG_DEBUG("dt_use_new_url_create_task: cfg_file_path = %s, new_origin_url = %s", cfg_file_path, new_origin_url);
	
	if(sd_strlen(cfg_file_path) > MAX_FILE_PATH_LEN)
	{	
		EM_LOG_DEBUG("dt_use_new_url_create_task: ERROR %d: INVALID_FILE_PATH", INVALID_FILE_PATH);
		return INVALID_FILE_PATH;
	}
	*p_task_id = 0;
	/*if(sd_file_exist(cfg_file_path))
	{
		//_p_param->_result = eti_set_origin_url_to_cfg_file(cfg_file_path, new_origin_url, sd_strlen(new_origin_url));
		if(_p_param->_result != SUCCESS)
		{
			EM_LOG_DEBUG("dt_use_new_url_create_task: eti_set_origin_url_to_cfg_file --ret_val = %d", _p_param->_result);
			return em_signal_sevent_handle(&(_p_param->_handle));
		}
		
	}
	else
	{
		_p_param->_result = DT_ERR_CFG_FILE_NOT_EXIST;
		EM_LOG_DEBUG("dt_use_new_url_create_task: ERROR %d: DT_ERR_CFG_FILE_NOT_EXIST", DT_ERR_CFG_FILE_NOT_EXIST);
		return em_signal_sevent_handle(&(_p_param->_handle));
	}
	*/
	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		EM_LOG_DEBUG("dt_use_new_url_create_task: ERROR %d: ETM_BUSY", ETM_BUSY);
		return em_signal_sevent_handle(&(_p_param->_handle));
	}
	else
	{	
		create_task_param._type = TT_URL;
		create_task_param._file_name = p_task_info->_file_name;
		create_task_param._file_name_len = sd_strlen(p_task_info->_file_name);
		create_task_param._file_size = p_task_info->_file_size;
		create_task_param._url = new_origin_url;
		create_task_param._url_len = sd_strlen(new_origin_url);
		create_task_param._check_data = FALSE;
		create_task_param._user_data = userdata;
		create_task_param._user_data_len = userdata_len;
		_p_param->_result = dt_create_task_impl( &create_task_param, p_task_id ,FALSE, FALSE, TRUE);
	}

	EM_LOG_DEBUG("dt_use_new_url_create_task %u end!em_signal_sevent_handle:_result=%d",*p_task_id,_p_param->_result);
	if(_p_param->_result == SUCCESS)
	{
		if(!create_task_param._manual_start)
		{
			/* start net work */
			em_is_net_ok(TRUE);
		}
		dt_force_scheduler();
	}

	return em_signal_sevent_handle(&(_p_param->_handle));
}



#ifdef ENABLE_ETM_MEDIA_CENTER
_int32 dt_create_shop_task( void * p_param )
{
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	EM_CREATE_SHOP_TASK * p_create_shop_param =(EM_CREATE_SHOP_TASK *)_p_param->_para1;
	_u32* p_task_id =(_u32 *)_p_param->_para2;
    BOOL thread_unlock = (BOOL)_p_param->_para3;
	CREATE_TASK create_task_param;

	em_memset(&create_task_param,0x00,sizeof(CREATE_TASK));
	create_task_param._type = TT_URL;
	create_task_param._file_path = p_create_shop_param->_file_path;
	create_task_param._file_path_len = p_create_shop_param->_file_path_len;
	create_task_param._file_name = p_create_shop_param->_file_name;
	create_task_param._file_name_len = p_create_shop_param->_file_name_len;
	create_task_param._url = p_create_shop_param->_url;
	create_task_param._url_len = p_create_shop_param->_url_len;
	create_task_param._ref_url = p_create_shop_param->_ref_url;
	create_task_param._ref_url_len = p_create_shop_param->_ref_url_len;
	//create_task_param._seed_file_full_path = p_create_shop_param->_seed_file_full_path;
	//create_task_param._seed_file_full_path_len = p_create_shop_param->_seed_file_full_path_len;
	//create_task_param._download_file_index_array = p_create_shop_param->_download_file_index_array;
	//create_task_param._file_num = p_create_shop_param->_file_num;
	create_task_param._tcid = p_create_shop_param->_tcid;
	create_task_param._file_size = p_create_shop_param->_file_size;
	create_task_param._gcid = p_create_shop_param->_gcid;
	create_task_param._check_data = FALSE;
	create_task_param._manual_start = FALSE;
	//create_task_param._user_data = p_create_shop_param->_user_data;
	//create_task_param._user_data_len = p_create_shop_param->_user_data_len;
	create_task_param._group_id = p_create_shop_param->_group_id;
	create_task_param._group_name = p_create_shop_param->_group_name;
	create_task_param._group_name_len= p_create_shop_param->_group_name_len;
	create_task_param._group_type = p_create_shop_param->_group_type;
	create_task_param._product_id = p_create_shop_param->_product_id;
	create_task_param._sub_id = p_create_shop_param->_sub_id;

	EM_LOG_DEBUG("dt_create_task:%s",p_create_shop_param->_url);

	*p_task_id=0;

	if(create_task_param._file_path != NULL && !em_file_exist(create_task_param._file_path))
	{
		em_mkdir(create_task_param._file_path);
	}

	if(g_task_locked) 
	{
		_p_param->_result = dt_create_urgent_task( &create_task_param,p_task_id,!thread_unlock );
	}
	else
	{
		create_task_param._check_data = FALSE;//_check_data 用于_is_no_disk -----zyq @20101027 fro vod task
		_p_param->_result = dt_create_task_impl( &create_task_param,p_task_id ,FALSE,!thread_unlock,FALSE);
	}

	EM_LOG_DEBUG("dt_create_task %u end!em_signal_sevent_handle:_result=%d",*p_task_id,_p_param->_result);
	if(_p_param->_result==SUCCESS)
	{
		if(!create_task_param._manual_start)
		{
			/* start net work */
			em_is_net_ok(TRUE);
		}
		
		dt_force_scheduler();
	}
    if(!thread_unlock)
    {
        return _p_param->_result;
    }
	return em_signal_sevent_handle(&(_p_param->_handle));
}
#endif


_int32 dt_start_task( void * p_param )
{
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	_u32 task_id=(_u32)_p_param->_para1;
	EM_TASK * p_task = NULL;
	BOOL exact_errcode = (BOOL)_p_param->_para2;
    BOOL thread_unlock = (BOOL)_p_param->_para3;

	EM_LOG_DEBUG("@@@@@@@@@@@@ dt_start_task:%u",task_id);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	if(g_need_correct_path)
	{
		dt_correct_all_tasks_path();
		g_need_correct_path = FALSE;
	}

	p_task = dt_get_task_from_map(task_id);
	
	if(p_task==NULL) 
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}
	
	if((TS_TASK_PAUSED!=dt_get_task_state(p_task))&&(TS_TASK_FAILED!=dt_get_task_state(p_task)))
	{
		_p_param->_result = INVALID_TASK_STATE;
		goto ErrorHanle;
	}

	if((p_task->_task_info->_file_size != 0) && (p_task->_task_info->_downloaded_data_size>=p_task->_task_info->_file_size))
	{
		/* 已经下载完毕,强制置为成功状态 */
		dt_set_task_state(p_task, TS_TASK_SUCCESS);
		dt_remove_task_from_order_list(p_task);
		/* Do nothing */
		goto ErrorHanle;
	}
	
	_p_param->_result = dt_check_task_free_disk(p_task,dt_get_task_file_path(p_task));
	if(_p_param->_result == SUCCESS)
	{
#ifdef KANKAN_PROJ
	dt_set_task_state(p_task, TS_TASK_WAITING);
	if(dt_is_lan_task(p_task))
	{
		dt_increase_waiting_lan_task_num();
	}
#else
	_p_param->_result=dt_start_task_impl(p_task);
#endif
	}	
	
ErrorHanle:

	EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
	if(_p_param->_result==SUCCESS)
	{
		dt_force_scheduler();
	}
	
	if(_p_param->_result==NETWORK_INITIATING && !exact_errcode)
	{
		_p_param->_result = SUCCESS;
	}
	
	if(_p_param->_result==SUCCESS)
	{
#ifdef ENABLE_RC
        rc_remove_static_task_id( task_id );
#endif /* ENABLE_RC */
	}    
    
    if(!thread_unlock)
    {
        return _p_param->_result;
    }

	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 dt_vod_start_task( void * p_param )
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	_u32 task_id=(_u32)_p_param->_para1;
	EM_TASK * p_task = NULL;
    BOOL thread_unlock = (BOOL)_p_param->_para2;

	EM_LOG_DEBUG("@@@@@@@@@@@@ dt_start_task:%u",task_id);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	p_task = dt_get_task_from_map(task_id);
	
	if(p_task==NULL) 
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}
	
	if((TS_TASK_WAITING!=dt_get_task_state(p_task))&&(TS_TASK_PAUSED!=dt_get_task_state(p_task))&&(TS_TASK_FAILED!=dt_get_task_state(p_task)))
	{
		_p_param->_result = INVALID_TASK_STATE;
		goto ErrorHanle;
	}

	if((p_task->_task_info->_file_size > 0) && (p_task->_task_info->_downloaded_data_size>=p_task->_task_info->_file_size))
	{
		/* 已经下载完毕,强制置为成功状态 */
		dt_set_task_state(p_task, TS_TASK_SUCCESS);
		dt_remove_task_from_order_list(p_task);
		/* Do nothing */
		goto ErrorHanle;
	}

	_p_param->_result = dt_check_task_free_disk(p_task,dt_get_task_file_path(p_task));
	if(_p_param->_result == SUCCESS)
	{
#if defined(KANKAN_PROJ) || defined(LIXIAN_PROJ) || defined(LIXIAN_IPHONE_PROJ)
	if(!dt_is_vod_task(p_task) && TS_TASK_PAUSED == dt_get_task_state(p_task))
	{
		p_task->_need_pause = TRUE;
	}
	dt_set_task_state(p_task, TS_TASK_WAITING);
	if(dt_is_lan_task(p_task))
	{
		dt_increase_waiting_lan_task_num();
	}
	if (p_task->_is_ad) 
	{
		_p_param->_result = dt_force_start_task(p_task);
	}
#else
	_p_param->_result=dt_start_task_impl(p_task);
#endif
	}	
	
ErrorHanle:

	EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
	if(_p_param->_result==SUCCESS)
	{
		dt_force_scheduler();
	}
	
	if(_p_param->_result==NETWORK_INITIATING)
	{
		_p_param->_result = SUCCESS;
	}
	
	if(_p_param->_result==SUCCESS)
	{
#ifdef ENABLE_RC
        rc_remove_static_task_id( task_id );
#endif /* ENABLE_RC */
	}    
    
    if(!thread_unlock)
    {
        return _p_param->_result;
    }

	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 dt_run_task( void * p_param )
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	_u32 task_id=(_u32)_p_param->_para1;
	EM_TASK * p_task = NULL;
    BOOL thread_unlock = (BOOL)_p_param->_para2;

	EM_LOG_DEBUG("@@@@@@@@@@@@ dt_start_task:%u",task_id);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	p_task = dt_get_task_from_map(task_id);
	
	if(p_task==NULL) 
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}
	
	if((TS_TASK_PAUSED!=dt_get_task_state(p_task))&&(TS_TASK_FAILED!=dt_get_task_state(p_task))&&(TS_TASK_WAITING!=dt_get_task_state(p_task)))
	{
		_p_param->_result = INVALID_TASK_STATE;
		goto ErrorHanle;
	}

	if((p_task->_task_info->_file_size > 0) && (p_task->_task_info->_downloaded_data_size>=p_task->_task_info->_file_size))
	{
		/* 已经下载完毕,强制置为成功状态 */
		dt_set_task_state(p_task, TS_TASK_SUCCESS);
		dt_remove_task_from_order_list(p_task);
		/* Do nothing */
		goto ErrorHanle;
	}

	_p_param->_result = dt_check_task_free_disk(p_task,dt_get_task_file_path(p_task));
	if(_p_param->_result == SUCCESS)
	{
		_p_param->_result=dt_start_task_impl(p_task);
	}	
	
ErrorHanle:

	EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
	if(_p_param->_result==SUCCESS)
	{
		dt_force_scheduler();
	}
	
	if(_p_param->_result==NETWORK_INITIATING)
	{
		_p_param->_result = SUCCESS;
	}
	
	if(_p_param->_result==SUCCESS)
	{
#ifdef ENABLE_RC
        rc_remove_static_task_id( task_id );
#endif /* ENABLE_RC */
	}    
    
    if(!thread_unlock)
    {
        return _p_param->_result;
    }

	return em_signal_sevent_handle(&(_p_param->_handle));	
}

_int32 dt_stop_task( void * p_param )
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	_u32 task_id=(_u32)_p_param->_para1;
	TASK_STATE state;
	EM_TASK * p_task = NULL;
    BOOL thread_unlock = (BOOL)_p_param->_para2;

	EM_LOG_DEBUG("@@@@@@@@@@@@ dt_stop_task:%u",task_id);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	p_task = dt_get_task_from_map(task_id);
	
	if(p_task==NULL) 
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}

//	点播任务也可被强制暂停，切换成3g网络时会调用	
//	if(task_id == dt_get_current_vod_task_id())
//	{
//		/* 该任务为当前点播任务,不停止 */
//		_p_param->_result = ERR_TASK_PLAYING;
//		goto ErrorHanle;
//	}
	
	/* 是否点播任务? */
	if(dt_is_vod_task(p_task))
	{
		dt_stop_vod_task_impl(p_task);
	}
	else
	{
		state = dt_get_task_state(p_task);
		if(TS_TASK_SUCCESS == state)
		{
			if(p_task->_inner_id!=0)
			{
				em_close_ex(p_task->_inner_id);
				p_task->_inner_id = 0;
			}
		}
	
		if((TS_TASK_WAITING!=state)&&(TS_TASK_RUNNING!=state))
		{
			_p_param->_result = INVALID_TASK_STATE;
			goto ErrorHanle;
		}

		if(TS_TASK_WAITING==state)
		{
			_p_param->_result=dt_stop_task_impl(p_task);
		}
		else
		{
			p_task->_waiting_stop = TRUE;
			dt_have_task_waiting_stop();
		}
	}
	
ErrorHanle:

	EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
	if(_p_param->_result==SUCCESS)
	{
		dt_force_scheduler();
#ifdef ENABLE_RC
        rc_remove_static_task_id( task_id );
#endif /* ENABLE_RC */
	}

    if(!thread_unlock)
    {
        return _p_param->_result;
	}
	return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 dt_delete_task( void * p_param )
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	_u32 task_id=(_u32)_p_param->_para1;
	EM_TASK * p_task = NULL;
	TASK_STATE state;
    BOOL thread_unlock = (BOOL)_p_param->_para2;
	EM_LOG_DEBUG("dt_delete_task:%u",task_id);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	p_task = dt_get_task_from_map(task_id);
	
	if(p_task==NULL) 
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}

	state = dt_get_task_state(p_task);
	
	if(TS_TASK_RUNNING==state)
	{
		dt_stop_task_impl(p_task);
	}

	if(TS_TASK_DELETED!=state)
	{
		if(TS_TASK_SUCCESS == state)
		{
			if(p_task->_inner_id!=0)
			{
				em_close_ex(p_task->_inner_id);
				p_task->_inner_id = 0;
			}
		}
		
		if(dt_is_vod_task(p_task))
		{
			/* vod 任务直接销毁*/
			_p_param->_result=dt_destroy_vod_task(p_task);
		}
		else
		{
			_p_param->_result=dt_delete_task_impl(p_task);
		}
	}
	
ErrorHanle:

	EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
	if(_p_param->_result==SUCCESS)
	{
		dt_force_scheduler();
#ifdef ENABLE_RC
        rc_remove_static_task_id( task_id );
#endif /* ENABLE_RC */
	}
    if(!thread_unlock)
    {
        return _p_param->_result;
	}
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 dt_recover_task( void * p_param )
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	_u32 task_id=(_u32)_p_param->_para1;
	EM_TASK * p_task = NULL;

	EM_LOG_DEBUG("dt_recover_task:%u",task_id);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	p_task = dt_get_task_from_map(task_id);
	
	if(p_task==NULL) 
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}

	if(TS_TASK_DELETED==dt_get_task_state( p_task))
	{
		_p_param->_result=dt_recover_task_impl(p_task);
	}
	else
	{
		/* do nothing */
		_p_param->_result=SUCCESS;
	}
 	
ErrorHanle:
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
	if(_p_param->_result==SUCCESS)
	{
		dt_force_scheduler();
	}
		return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 dt_destroy_task( void * p_param )
{
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	_u32 task_id=(_u32)_p_param->_para1;
	BOOL delete_file=(_u32)_p_param->_para2;
    BOOL thread_unlock = (BOOL)_p_param->_para3;
	EM_TASK * p_task = NULL;
    EM_TASK_TYPE task_type;

	EM_LOG_DEBUG("dt_destroy_task:%u,%d",task_id,delete_file);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	p_task = dt_get_task_from_map(task_id);
	
	if(p_task==NULL) 
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}

	if(TS_TASK_RUNNING==dt_get_task_state( p_task))
	{
		dt_stop_task_impl(p_task);
	}

	task_type = dt_get_task_type( p_task);

	if(TS_TASK_SUCCESS == dt_get_task_state( p_task))
	{
		if(p_task->_inner_id!=0)
		{
			em_close_ex(p_task->_inner_id);
			p_task->_inner_id = 0;
		}
	}
		
	if(dt_is_vod_task(p_task))
	{
		/* vod 任务直接销毁*/
		_p_param->_result=dt_destroy_vod_task(p_task);
	}
	else
	{
		_p_param->_result=dt_destroy_task_impl(p_task,delete_file);
	}
	
ErrorHanle:

	EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
	if(_p_param->_result==SUCCESS)
	{
		dt_force_scheduler();
#ifdef ENABLE_RC
        rc_notify_destory_task( task_id, task_type );
#endif /* ENABLE_RC */
	}
    
    if(!thread_unlock)
    {
        return _p_param->_result;
    }	
    
	return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 dt_get_pri_id_list( void * p_param )
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	_u32 * id_array_buffer = (_u32 *)_p_param->_para1;
	_u32 *buffer_len = (_u32 *)_p_param->_para2;

	EM_LOG_DEBUG("dt_get_pri_id_list:buffer_len=%u",*buffer_len);
	
	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	_p_param->_result=dt_get_pri_id_list_impl(id_array_buffer,buffer_len);
	
ErrorHanle:

		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 dt_pri_level_up( void * p_param )
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	_u32 task_id = (_u32 )_p_param->_para1;
	_u32 steps = (_u32 )_p_param->_para2;
	
	EM_LOG_DEBUG("dt_pri_level_up:%u,%d",task_id,steps);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	_p_param->_result=dt_pri_level_up_impl(task_id,steps);
	
ErrorHanle:

		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 dt_pri_level_down( void * p_param )
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	_u32 task_id = (_u32 )_p_param->_para1;
	_u32 steps = (_u32 )_p_param->_para2;
	
	EM_LOG_DEBUG("dt_pri_level_down:%u,%d",task_id,steps);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	_p_param->_result=dt_pri_level_down_impl(task_id,steps);
	
ErrorHanle:

		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 dt_rename_task( void * p_param )
{
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	_u32 task_id = (_u32 )_p_param->_para1;
	char * new_name = (char* )_p_param->_para2;
	_u32 new_name_len = (_u32 )_p_param->_para3;
	EM_TASK * p_task = NULL;

	EM_LOG_DEBUG("dt_rename_task:%u,%s",task_id,new_name);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	p_task = dt_get_task_from_map(task_id);
	
	if(p_task==NULL) 
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}
	
	/* 如果是vod任务,不能改文件名*/
	if(p_task->_task_info->_task_id > MAX_DL_TASK_ID)
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}
	
	if(dt_get_task_type(p_task) == TT_BT)
	{
		_p_param->_result = INVALID_TASK_TYPE;
		goto ErrorHanle;
	}
	
	if(TS_TASK_SUCCESS==dt_get_task_state( p_task))
	{
		if(p_task->_inner_id!=0)
		{
			em_close_ex(p_task->_inner_id);
			p_task->_inner_id = 0;
		}
		_p_param->_result=dt_rename_task_impl(p_task,new_name,new_name_len);
	}
	else
	{
		_p_param->_result=INVALID_TASK_STATE;
	}
	
ErrorHanle:
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}

#if defined(ENABLE_NULL_PATH)
_int32 dt_et_get_task_info(void * p_param)
{
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	_u32 task_id = (_u32 )_p_param->_para1;
	EM_TASK_INFO * p_em_task_info = (EM_TASK_INFO* )_p_param->_para2;
	//TASK_INFO * p_task_info=NULL;
	//EM_TASK * p_task = NULL;
	//char *file_path=NULL,*file_name=NULL;
	//P2SP_TASK * p_p2sp_task = NULL;
	//BT_TASK * p_bt_task = NULL;
	_int32 ret_val = SUCCESS;
	
	ret_val = dt_et_get_task_info_impl(task_id, p_em_task_info);
       return ret_val;
}
#endif

_int32 dt_get_task_info(void * p_param)
{
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	_u32 task_id = (_u32 )_p_param->_para1;
	EM_TASK_INFO * p_em_task_info = (EM_TASK_INFO* )_p_param->_para2;
	//TASK_INFO * p_task_info=NULL;
	EM_TASK * p_task = NULL;
	char *file_path=NULL,*file_name=NULL;
	//P2SP_TASK * p_p2sp_task = NULL;
	//BT_TASK * p_bt_task = NULL;
    BOOL thread_unlock = (BOOL)_p_param->_para3;

	EM_LOG_DEBUG("dt_get_task_info:%u",task_id);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	p_task = dt_get_task_from_map(task_id);
	
	if(p_task==NULL) 
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}
	
		p_em_task_info->_task_id = p_task->_task_info->_task_id;
		p_em_task_info->_state= dt_get_task_state( p_task); //p_task->_task_info->_state; //
		if((p_task->_task_info->_file_size > 0) && (p_task->_task_info->_downloaded_data_size>=p_task->_task_info->_file_size))
		{
			/* 已经下载完毕,强制置为成功状态 */
			p_em_task_info->_state = TS_TASK_SUCCESS;
		}

		p_em_task_info->_type= p_task->_task_info->_type; //dt_get_task_type(p_task);
		
		if(p_task->_task_info->_is_deleted)
		{
			p_em_task_info->_is_deleted= TRUE;
		}
		
		p_em_task_info->_file_size= p_task->_task_info->_file_size;
		p_em_task_info->_downloaded_data_size= p_task->_task_info->_downloaded_data_size;
		p_em_task_info->_start_time= p_task->_task_info->_start_time;
		p_em_task_info->_finished_time= p_task->_task_info->_finished_time;
		p_em_task_info->_failed_code= p_task->_task_info->_failed_code;
		p_em_task_info->_bt_total_file_num= p_task->_task_info->_bt_total_file_num;

		if(p_task->_task_info->_check_data)  //_check_data 用于_is_no_disk -----zyq @20101027 fro vod task
		{
			//p_em_task_info->_check_data= TRUE;
			p_em_task_info->_is_no_disk= TRUE;
		}
		

	/* 如果是vod任务,没有文件路径和名字*/
	if(p_task->_task_info->_task_id < MAX_DL_TASK_ID)
	{
		file_path = dt_get_task_file_path(p_task);
		file_name = dt_get_task_file_name(p_task);

 		if(file_path!=NULL)
 		{
			em_memcpy(p_em_task_info->_file_path, file_path, p_task->_task_info->_file_path_len);
			#ifndef LINUX
			if(p_em_task_info->_file_path[p_task->_task_info->_file_path_len-1]!='\\')
			{	// 和谐路径
				sd_strcat(p_em_task_info->_file_path, "\\", 1);
//				p_em_task_info->_file_path[p_task->_task_info->_file_path_len]='/';
//				p_em_task_info->_file_path[p_task->_task_info->_file_path_len+1]='\0';
			}
			#endif
 		}
		else
		{
			sd_assert(FALSE);
			_p_param->_result = INVALID_FILE_PATH;
			goto ErrorHanle;
		}
		
 		if(file_name!=NULL)
 		{
			em_memcpy(p_em_task_info->_file_name, file_name, p_task->_task_info->_file_name_len);
 		}
		else
 		if(p_task->_task_info->_have_name)
		{
			sd_assert(FALSE);
			_p_param->_result = INVALID_FILE_NAME;
			goto ErrorHanle;
		}
	}
		_p_param->_result=SUCCESS;

	EM_LOG_DEBUG("dt_get_task_info: task_id = %u, failed_code = %d, file_size = %llu, is_no_disk = %d, filename = %s, file_path = %s, state = %d, type = %d",
		task_id, p_em_task_info->_failed_code, p_em_task_info->_file_size, p_em_task_info->_is_no_disk,p_em_task_info->_file_name, p_em_task_info->_file_path, p_em_task_info->_state, p_em_task_info->_type);
ErrorHanle:
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
        if(!thread_unlock)
        {
            return _p_param->_result;
        }   

		return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 dt_get_task_download_info(void * p_param)
{
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	_u32 task_id = (_u32 )_p_param->_para1;
	EM_TASK_INFO * p_em_task_info = (EM_TASK_INFO* )_p_param->_para2;
	//TASK_INFO * p_task_info=NULL;
	EM_TASK * p_task = NULL;
	char *file_path=NULL,*file_name=NULL;
	//P2SP_TASK * p_p2sp_task = NULL;
	//BT_TASK * p_bt_task = NULL;
    BOOL thread_unlock = (BOOL)_p_param->_para3;

	EM_LOG_DEBUG("dt_get_task_info:%u",task_id);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	p_task = dt_get_task_from_map(task_id);
	
	if(p_task==NULL) 
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}
	
		p_em_task_info->_task_id = p_task->_task_info->_task_id;
		if(p_task->_need_pause == TRUE)       //ipad kankan 下载任务的下载状态
			p_em_task_info->_state = TS_TASK_PAUSED;
		else
			p_em_task_info->_state= dt_get_task_state( p_task); //p_task->_task_info->_state; //

		if((p_task->_task_info->_file_size > 0) && (p_task->_task_info->_downloaded_data_size>=p_task->_task_info->_file_size))
		{
			/* 已经下载完毕,强制置为成功状态 */
			p_em_task_info->_state = TS_TASK_SUCCESS;
		}

		p_em_task_info->_type= p_task->_task_info->_type; //dt_get_task_type(p_task);
		
		if(p_task->_task_info->_is_deleted)
		{
			p_em_task_info->_is_deleted= TRUE;
		}
		
		p_em_task_info->_file_size= p_task->_task_info->_file_size;
		p_em_task_info->_downloaded_data_size= p_task->_task_info->_downloaded_data_size;
		p_em_task_info->_start_time= p_task->_task_info->_start_time;
		p_em_task_info->_finished_time= p_task->_task_info->_finished_time;
		p_em_task_info->_failed_code= p_task->_task_info->_failed_code;
		p_em_task_info->_bt_total_file_num= p_task->_task_info->_bt_total_file_num;

		if(p_task->_task_info->_check_data)  //_check_data 用于_is_no_disk -----zyq @20101027 fro vod task
		{
			//p_em_task_info->_check_data= TRUE;
			p_em_task_info->_is_no_disk= TRUE;
		}
		

	/* 如果是vod任务,没有文件路径和名字*/
	if(p_task->_task_info->_task_id < MAX_DL_TASK_ID)
	{
		file_path = dt_get_task_file_path(p_task);
		file_name = dt_get_task_file_name(p_task);

 		if(file_path!=NULL)
 		{
			em_memcpy(p_em_task_info->_file_path, file_path, p_task->_task_info->_file_path_len);
			#ifndef LINUX
			if(p_em_task_info->_file_path[p_task->_task_info->_file_path_len-1]!='\\')
			{	// 和谐路径
				sd_strcat(p_em_task_info->_file_path, "\\", 1);
//				p_em_task_info->_file_path[p_task->_task_info->_file_path_len]='/';
//				p_em_task_info->_file_path[p_task->_task_info->_file_path_len+1]='\0';
			}
			#endif
 		}
		else
		{
			sd_assert(FALSE);
			_p_param->_result = INVALID_FILE_PATH;
			goto ErrorHanle;
		}
		
 		if(file_name!=NULL)
 		{
			em_memcpy(p_em_task_info->_file_name, file_name, p_task->_task_info->_file_name_len);
 		}
		else
 		if(p_task->_task_info->_have_name)
		{
			sd_assert(FALSE);
			_p_param->_result = INVALID_FILE_NAME;
			goto ErrorHanle;
		}
	}
		_p_param->_result=SUCCESS;
	
ErrorHanle:
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
        if(!thread_unlock)
        {
            return _p_param->_result;
        }   

		return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 dt_get_task_running_status(_u32 task_id,RUNNING_STATUS *p_status)
{
	if(g_task_locked)  return ETM_BUSY;
	return dt_get_running_task(task_id,p_status);
}

_int32 dt_set_task_url( void * p_param )
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	_u32 task_id=(_u32)_p_param->_para1;
	const char * url=(const char *)_p_param->_para2;
	EM_TASK * p_task = NULL;
	EM_P2SP_TASK *p_p2sp_task=NULL;
    EM_TASK_TYPE task_type;
	char * old_url = NULL;

	EM_LOG_DEBUG("dt_set_task_url:%u,%s",task_id,url);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	p_task = dt_get_task_from_map(task_id);
	
	if(p_task==NULL) 
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}

	task_type = dt_get_task_type( p_task);
	if(TT_LAN!=task_type)
	{
		_p_param->_result = INVALID_TASK_TYPE;
		goto ErrorHanle;
	}

	if(p_task->_task_info->_full_info)
	{
		p_p2sp_task = (EM_P2SP_TASK *)p_task->_task_info;
		old_url = p_p2sp_task->_url;
	}
	else
	{
		old_url=dt_get_task_url_from_file(p_task);
	}

	sd_assert(old_url!=NULL);

	/* 比较新旧URL */
	if(sd_strcmp(old_url, url)!=0)
	{
	if(TS_TASK_RUNNING==dt_get_task_state( p_task))
	{
		dt_stop_task_impl(p_task);
	}
		
	_p_param->_result=dt_set_p2sp_task_url(p_task,(char*)url,em_strlen(url)); 
	}

	
ErrorHanle:

	EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 dt_get_task_url( void * p_param )
{
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	_u32 task_id = (_u32 )_p_param->_para1;
	char * url_buffer = (char* )_p_param->_para2;
	EM_TASK * p_task = NULL;
	EM_P2SP_TASK *p_p2sp_task=NULL;
	char * url=NULL;
    BOOL thread_unlock = (BOOL)_p_param->_para3;

	EM_LOG_DEBUG("dt_get_task_url:%u",task_id);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	p_task = dt_get_task_from_map(task_id);
	
	if(p_task==NULL) 
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}

	EM_LOG_DEBUG("dt_get_task_url:%u,type=%d,state=%d,inner_id=%u",task_id,p_task->_task_info->_type,p_task->_task_info->_state,p_task->_inner_id);
	if((p_task->_task_info->_type != TT_URL)&&(p_task->_task_info->_type != TT_EMULE)&&(p_task->_task_info->_type != TT_LAN))
	{
		_p_param->_result = INVALID_TASK_TYPE;
		goto ErrorHanle;
	}
	
	if(p_task->_task_info->_full_info)
	{
		p_p2sp_task = (EM_P2SP_TASK *)p_task->_task_info;
		url = p_p2sp_task->_url;
	}
	else
	{
		url=dt_get_task_url_from_file(p_task);
	}

	if(url== NULL)
	{
		_p_param->_result = INVALID_URL;
		goto ErrorHanle;
	}
	else
	{
		em_strncpy(url_buffer, url, MAX_URL_LEN);
		_p_param->_result = SUCCESS;
	}
	
ErrorHanle:
    if(!thread_unlock)
    {
        return _p_param->_result;
    }	

    EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 dt_get_task_ref_url( void * p_param )
{
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	_u32 task_id = (_u32 )_p_param->_para1;
	char * url_buffer = (char* )_p_param->_para2;
	EM_TASK * p_task = NULL;
	EM_P2SP_TASK *p_p2sp_task=NULL;
	char * url=NULL;
    BOOL thread_unlock = (BOOL)_p_param->_para3;

	EM_LOG_DEBUG("dt_get_task_ref_url:%u",task_id);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	p_task = dt_get_task_from_map(task_id);
	
	if(p_task==NULL) 
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}

	if(p_task->_task_info->_type != TT_URL)
	{
		_p_param->_result = INVALID_TASK_TYPE;
		goto ErrorHanle;
	}
	
	if(p_task->_task_info->_have_ref_url)
	{
		if(p_task->_task_info->_full_info)
		{
			p_p2sp_task = (EM_P2SP_TASK *)p_task->_task_info;
			url = p_p2sp_task->_ref_url;
		}
		else
		{
			url= dt_get_task_ref_url_from_file(p_task);
		}
	}
	
	if(url== NULL)
	{
		_p_param->_result = INVALID_URL;
		goto ErrorHanle;
	}
	else
	{
		em_strncpy(url_buffer, url, MAX_URL_LEN);
		_p_param->_result = SUCCESS;
	}
	
	
ErrorHanle:
	EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
    if(!thread_unlock)
    {
        return _p_param->_result;
    }   

    return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 dt_get_task_tcid_impl(EM_TASK * p_task, char * cid_buffer)
{
	_int32 ret_val = SUCCESS;
	EM_P2SP_TASK *p_p2sp_task=NULL;
	_u8 * cid=NULL;


	if(p_task->_task_info->_type == TT_TCID||p_task->_task_info->_type == TT_LAN)
	{
		ret_val = em_str2hex((char*)p_task->_task_info->_eigenvalue, CID_SIZE, cid_buffer, CID_SIZE*2+1);
	}
	else
	if((p_task->_task_info->_type == TT_KANKAN)||(p_task->_task_info->_type == TT_URL))
	{
		if(p_task->_task_info->_have_tcid)
		{
			if(p_task->_task_info->_full_info)
			{
				p_p2sp_task = (EM_P2SP_TASK *)p_task->_task_info;
				cid = p_p2sp_task->_tcid;
			}
			else
			{
				cid= dt_get_task_tcid_from_file(p_task);
			}
		}
		
		if(cid== NULL)
		{
			ret_val = INVALID_TCID;
		}
		else
		{
			ret_val = em_str2hex((char*)cid, CID_SIZE, cid_buffer, CID_SIZE*2+1);
		}
	}
	else
	{
		ret_val = INVALID_TASK_TYPE;
	}
	
	if((ret_val!=SUCCESS)&&(p_task->_inner_id!=0)&&(p_task->_task_info->_type != TT_BT))
	{
		_u8  tcid_tmp[CID_SIZE]={0};
		ret_val = et_get_task_tcid(p_task->_inner_id, tcid_tmp);
		if(ret_val == SUCCESS)
		{
			ret_val = em_str2hex((char*)tcid_tmp, CID_SIZE, cid_buffer, CID_SIZE*2+1);
		}
	}
	
	return ret_val;
}
_int32 dt_get_task_tcid( void * p_param )
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	_u32 task_id = (_u32 )_p_param->_para1;
	char * cid_buffer = (char* )_p_param->_para2;
	EM_TASK * p_task = NULL;
	//EM_P2SP_TASK *p_p2sp_task=NULL;
	//_u8 * cid=NULL;

	EM_LOG_DEBUG("dt_get_task_tcid:%u",task_id);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	p_task = dt_get_task_from_map(task_id);
	
	if(p_task==NULL) 
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}

	_p_param->_result = dt_get_task_tcid_impl(p_task, cid_buffer);

ErrorHanle:
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 dt_get_bt_task_sub_file_tcid( void * p_param )
{
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	_u32 task_id = (_u32 )_p_param->_para1;
	_u32 file_index = (_u32 )_p_param->_para2;
	char * cid_buffer = (char* )_p_param->_para3;
	EM_TASK * p_task = NULL;
	_u8 cid[CID_SIZE]= {0};

	EM_LOG_DEBUG("dt_get_bt_task_sub_file_tcid:%u,%u",task_id,file_index);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	p_task = dt_get_task_from_map(task_id);
	
	if(p_task==NULL) 
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}

	if(p_task->_task_info->_type == TT_BT)
	{
		if(p_task->_task_info->_state == TS_TASK_RUNNING)
		{
			if(p_task->_bt_running_files!=NULL)
			{
				_p_param->_result = et_get_bt_task_sub_file_tcid(p_task->_inner_id,  file_index,cid);
				if(_p_param->_result==SUCCESS)
				{
					_p_param->_result = em_str2hex((char*)cid, CID_SIZE, cid_buffer, CID_SIZE*2+1);
				}
			}
			else
			{
				_p_param->_result = INVALID_TASK_STATE;
			}
		}
		else
		{
			_p_param->_result = INVALID_TASK_STATE;
		}
	}
	else
	{
		_p_param->_result = INVALID_TASK_TYPE;
	}

ErrorHanle:
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 dt_get_task_gcid( void * p_param )
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	_u32 task_id = (_u32 )_p_param->_para1;
	char * cid_buffer = (char* )_p_param->_para2;
	EM_TASK * p_task = NULL;
	u8   gcid[CID_SIZE] = {0};

	EM_LOG_DEBUG("dt_get_task_gcid:%u",task_id);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	p_task = dt_get_task_from_map(task_id);
	
	if(p_task==NULL) 
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}

	if(p_task->_task_info->_type == TT_KANKAN)
	{
		_p_param->_result = em_str2hex((char*)p_task->_task_info->_eigenvalue, CID_SIZE, cid_buffer, CID_SIZE*2+1);
	}
	else
	if(p_task->_task_info->_type != TT_BT)
	{
		if(TS_TASK_RUNNING==dt_get_task_state(p_task))
		{
			_p_param->_result = et_get_task_gcid(p_task->_inner_id, gcid);
			if(_p_param->_result==SUCCESS)
			{
		            str2hex((char*)gcid, CID_SIZE, cid_buffer, CID_SIZE*2);
			     cid_buffer[CID_SIZE*2] = '\0';   
			}
		}
		else
		{
			_p_param->_result = INVALID_TASK_STATE;
		}
	}
	else
	{
		_p_param->_result = INVALID_TASK_TYPE;
	}
	
ErrorHanle:
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 dt_get_bt_task_seed_file( void * p_param )
{
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	_u32 task_id = (_u32 )_p_param->_para1;
	char * path_buffer = (char* )_p_param->_para2;
    BOOL thread_unlock = (BOOL)_p_param->_para3;
	EM_TASK * p_task = NULL;
	EM_BT_TASK *p_bt_task=NULL;
	char * seed_path=NULL;

	EM_LOG_DEBUG("dt_get_bt_task_seed_file:%u",task_id);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	p_task = dt_get_task_from_map(task_id);
	
	if(p_task==NULL) 
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}

	if(p_task->_task_info->_type!= TT_BT)
	{
		_p_param->_result = INVALID_TASK_TYPE;
		goto ErrorHanle;
	}
	
	if(p_task->_task_info->_full_info)
	{
		p_bt_task = (EM_BT_TASK *)p_task->_task_info;
		seed_path = p_bt_task->_seed_file_path;
	}
	else
	{
		seed_path= dt_get_task_seed_file_from_file(p_task);
	}

	if(seed_path== NULL)
	{
		_p_param->_result = INVALID_SEED_FILE;
		goto ErrorHanle;
	}
	else
	{
		em_strncpy(path_buffer, seed_path, MAX_FULL_PATH_BUFFER_LEN);
		_p_param->_result = SUCCESS;
	}
	
	
ErrorHanle:
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
        if(!thread_unlock)
        {
            return _p_param->_result;
        }
        
		return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 dt_get_bt_file_info( void * p_param )
{
	POST_PARA_4* _p_param = (POST_PARA_4*)p_param;
	_u32 task_id = (_u32 )_p_param->_para1;
	_u32 file_index_32 = (_u32 )_p_param->_para2;
	EM_BT_FILE * p_bt_file = (EM_BT_FILE* )_p_param->_para3;
    BOOL thread_unlock = (BOOL)_p_param->_para4;
	BT_FILE * p_bt_file_t=NULL;
	EM_TASK * p_task = NULL;
	EM_BT_TASK *p_bt_task=NULL;
	_u16 array_index=0,i=0,file_index=0;
	_int32 ret_val=SUCCESS;
	

	EM_LOG_DEBUG("dt_get_bt_file_info:%u,file_index=%u",task_id,file_index_32);


	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	p_task = dt_get_task_from_map(task_id);
	
	if(p_task==NULL) 
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}

	if(p_task->_task_info->_type!= TT_BT)
	{
		_p_param->_result = INVALID_TASK_TYPE;
		goto ErrorHanle;
	}
	
	file_index = (_u16)file_index_32;

	if(p_task->_bt_running_files!=NULL)
	{
		for(i=0;i<MAX_BT_RUNNING_SUB_FILE;i++)	
		{
			if(p_task->_bt_running_files->_sub_files[i]._file_index==file_index)
			{
				p_bt_file_t = &p_task->_bt_running_files->_sub_files[i];
				break;
			}
		}
	}
	
	if(p_bt_file_t==NULL)
	{
		ret_val = dt_get_bt_sub_file_array_index(p_task,file_index,&array_index);
		if(ret_val!= SUCCESS) 
		{
			_p_param->_result = ret_val;
			goto ErrorHanle;
		}
		
		if(p_task->_task_info->_full_info)
		{
			p_bt_task = (EM_BT_TASK *)p_task->_task_info;
			p_bt_file_t = &p_bt_task->_file_array[array_index];
		}
		else
		{
			p_bt_file_t = dt_get_task_bt_sub_file_from_file(p_task,array_index);
		}
	}
	
	if((p_bt_file_t==NULL)||(p_bt_file_t->_file_index != file_index))
	{
		_p_param->_result = EM_INVALID_FILE_INDEX;
		goto ErrorHanle;
	}
	else
	{
		//sd_memcpy(p_bt_file, p_bt_file_t, sizeof(BT_FILE));
		p_bt_file->_file_index = p_bt_file_t->_file_index;
		p_bt_file->_need_download= TRUE;
		p_bt_file->_status= p_bt_file_t->_status;
		p_bt_file->_file_size= p_bt_file_t->_file_size;
		p_bt_file->_downloaded_data_size= p_bt_file_t->_downloaded_data_size;
		p_bt_file->_failed_code= p_bt_file_t->_failed_code;
		if((p_task->_task_info->_state!= TS_TASK_RUNNING)&&(p_bt_file->_status==FS_DOWNLOADING))
		{
			p_bt_file->_status=FS_IDLE;
		}
		_p_param->_result = SUCCESS;
	}
	
	
ErrorHanle:
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
        if(!thread_unlock)
        {
            return _p_param->_result;
        }
		return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 dt_get_bt_task_sub_file_name( void * p_param )
{
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	_u32 task_id = (_u32 )_p_param->_para1;
	_u32 file_index = (_u32 )_p_param->_para2;
	char * file_name = (char* )_p_param->_para3;
	EM_TASK * p_task = NULL;
	_int32 buffer_size = MAX_FILE_NAME_BUFFER_LEN;

	EM_LOG_DEBUG("dt_get_bt_task_sub_file_name:%u,%u",task_id,file_index);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	p_task = dt_get_task_from_map(task_id);
	
	if(p_task==NULL) 
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}

	if(p_task->_task_info->_type == TT_BT)
	{
		if(p_task->_task_info->_state == TS_TASK_RUNNING)
		{
			if(p_task->_bt_running_files!=NULL)
			{
				_p_param->_result = et_get_bt_task_sub_file_name(p_task->_inner_id,  file_index,file_name,&buffer_size);
			}
			else
			{
				_p_param->_result = INVALID_TASK_STATE;
			}
		}
		else
		{
			_p_param->_result = INVALID_TASK_STATE;
		}
	}
	else
	{
		_p_param->_result = INVALID_TASK_TYPE;
	}

ErrorHanle:
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 dt_set_bt_need_download_file_index( void * p_param )
{
	_int32 ret_val=SUCCESS;
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	_u32 task_id = (_u32 )_p_param->_para1;
	_u32 * new_index_array = (_u32* )_p_param->_para2;
	_u32  new_file_num = (_u32 )_p_param->_para3;
	EM_TASK * p_task = NULL;
	EM_BT_TASK *p_bt_task=NULL;
	TASK_STATE state;
	_u16 *dl_file_index_array=NULL;
	_u16 *dl_file_index_array_current=NULL;
	_u16 need_dl_num=0; 
	_u16 i=0;
	BOOL need_release = FALSE;

	EM_LOG_DEBUG("dt_set_bt_need_download_file_index:%u",task_id);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	p_task = dt_get_task_from_map(task_id);
	
	if(p_task==NULL) 
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}

	if(p_task->_task_info->_type!= TT_BT)
	{
		_p_param->_result = INVALID_TASK_TYPE;
		goto ErrorHanle;
	}
	
	state = dt_get_task_state(p_task);
	
	if(state==TS_TASK_DELETED)
	{
		_p_param->_result = INVALID_TASK_STATE;
		goto ErrorHanle;
	}
	else if(state==TS_TASK_SUCCESS)
	{
		if(p_task->_inner_id!=0)
		{
			em_close_ex(p_task->_inner_id);
			p_task->_inner_id = 0;
		}
	}

	ret_val = dt_check_and_sort_bt_file_index(new_index_array,new_file_num,p_task->_task_info->_bt_total_file_num,&dl_file_index_array,&need_dl_num);
	if(ret_val!=SUCCESS)
	{
		_p_param->_result = ret_val;
		goto ErrorHanle;
	}

	if(need_dl_num==0)
	{
		EM_SAFE_DELETE(dl_file_index_array);
		_p_param->_result = INVALID_FILE_NAME;
		goto ErrorHanle;
	}
	
	if(need_dl_num==p_task->_task_info->_url_len_or_need_dl_num)
	{
		if(p_task->_task_info->_full_info)
		{
			p_bt_task = (EM_BT_TASK *)p_task->_task_info;
			dl_file_index_array_current = p_bt_task->_dl_file_index_array;
		}
		else
		if(p_task->_bt_running_files!=NULL)
		{
			dl_file_index_array_current=p_task->_bt_running_files->_need_dl_file_index_array;
		}
		else
		{
			dl_file_index_array_current = dt_get_task_bt_need_dl_file_index_array(p_task);
			if(dl_file_index_array_current!=NULL)
				need_release = TRUE;
		}

		if(dl_file_index_array_current!=NULL)
		{
			for(i=0;i<need_dl_num;i++)
			{
				if(dl_file_index_array_current[i]!=dl_file_index_array[i])
					break;
			}

			if(need_release)
			{
				dt_release_task_bt_need_dl_file_index_array(dl_file_index_array_current);
			}
			
			if(i==need_dl_num)
			{
				EM_SAFE_DELETE(dl_file_index_array);
				_p_param->_result = 0;
				goto ErrorHanle;
			}
		}

	}

	if(state== TS_TASK_RUNNING)
	{
		dt_stop_task_impl(p_task);
		//dt_save_task_to_file(p_task);
	}

	if(p_task->_task_info->_full_info)
	{
		p_bt_task = (EM_BT_TASK *)p_task->_task_info;
		// 之前保存的子文件下载个数小于需要重新设置的个数时，需要重新分配空间
		if(p_task->_task_info->_url_len_or_need_dl_num<need_dl_num)
		{
			EM_SAFE_DELETE(p_bt_task->_file_array);
			ret_val = em_malloc(need_dl_num*(sizeof(BT_FILE)), (void**)&p_bt_task->_file_array);
			if(ret_val!=SUCCESS)
			{
				EM_SAFE_DELETE(dl_file_index_array);
				_p_param->_result = ret_val;
				goto ErrorHanle;
			}
			em_memset(p_bt_task->_file_array,0,need_dl_num*(sizeof(BT_FILE)));
		}
		else
		{
			em_memset(p_bt_task->_file_array,0,p_task->_task_info->_url_len_or_need_dl_num*(sizeof(BT_FILE)));
		}
		// 重新赋值
		EM_SAFE_DELETE(p_bt_task->_dl_file_index_array);
		p_bt_task->_dl_file_index_array = dl_file_index_array;
		p_task->_task_info->_url_len_or_need_dl_num = need_dl_num;
		
		for(i=0;i<need_dl_num;i++)
		{
			p_bt_task->_file_array[i]._file_index = p_bt_task->_dl_file_index_array[i];
			//p_bt_task->_file_array[i]._need_download = TRUE;
			//p_bt_task->_file_array[i]._file_size = 1024;
		}

		_p_param->_result = dt_save_bt_task_need_dl_file_change_to_file( p_task,dl_file_index_array,need_dl_num);
	}
	else
	{
		_p_param->_result = dt_save_bt_task_need_dl_file_change_to_file( p_task,dl_file_index_array,need_dl_num);
		EM_SAFE_DELETE(dl_file_index_array);
	}

	if(_p_param->_result==SUCCESS)
	{
		/* restart the task */
		_p_param->_result=dt_set_task_state(p_task,TS_TASK_WAITING);
	}

ErrorHanle:
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}


_int32 dt_get_bt_need_download_file_index( void * p_param )
{
	//_int32 ret_val=SUCCESS;
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	_u32 task_id = (_u32 )_p_param->_para1;
	_u32 * got_index_array = (_u32* )_p_param->_para2;
	_u32 * got_file_num = (_u32 *)_p_param->_para3;
	
	EM_TASK * p_task = NULL;
	EM_BT_TASK *p_bt_task = NULL;
	TASK_STATE state;
	_u16 *dl_file_index_array_current = NULL;
	_u16 i = 0;

	EM_LOG_DEBUG("dt_get_bt_need_download_file_index:%u",task_id);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	p_task = dt_get_task_from_map(task_id);
	
	if(NULL == p_task) 
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}

	if(p_task->_task_info->_type != TT_BT)
	{
		_p_param->_result = INVALID_TASK_TYPE;
		goto ErrorHanle;
	}
	
	state = dt_get_task_state(p_task);
	
	if(state == TS_TASK_DELETED)
	{
		_p_param->_result = INVALID_TASK_STATE;
		goto ErrorHanle;
	}

	if(p_task->_task_info->_full_info)
	{
		p_bt_task = (EM_BT_TASK *)p_task->_task_info;
		*got_file_num = (_u32)p_task->_task_info->_url_len_or_need_dl_num;
		if( NULL != got_index_array )
		{
			for(i = 0; i < *got_file_num; i++)
			{
				got_index_array[i] = p_bt_task->_dl_file_index_array[i];
			}
		}
	}
	else if(p_task->_bt_running_files != NULL)
	{
		*got_file_num = (_u32)p_task->_bt_running_files->_need_dl_file_num;
		if( NULL != got_index_array )
		{
			for(i = 0; i < *got_file_num; i++)
			{
				got_index_array[i] = p_task->_bt_running_files->_need_dl_file_index_array[i];
			}
		}
	}
	else
	{
		dl_file_index_array_current = dt_get_task_bt_need_dl_file_index_array(p_task);
		if(dl_file_index_array_current != NULL)
		{
			*got_file_num = (_u32)p_task->_task_info->_url_len_or_need_dl_num;
			if( NULL != got_index_array )
			{
				for(i = 0; i < *got_file_num; i++)
				{
					got_index_array[i] = dl_file_index_array_current[i];
				}
			}
			SAFE_DELETE(dl_file_index_array_current);
		}
	}
	
ErrorHanle:
	EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}


_int32 dt_get_task_user_data( void * p_param )
{
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	_u32 task_id = (_u32 )_p_param->_para1;
	_u8 * data_buffer = (_u8* )_p_param->_para2;
	_u32* buffer_len = (_u32* )_p_param->_para3;
	EM_TASK * p_task = NULL;
	//EM_BT_TASK *p_bt_task=NULL;
	//EM_P2SP_TASK *p_p2sp_task=NULL;
	_u8 * common_data = NULL;
	_u32 common_data_len = 0;
	_u8 * data_buffer_tmp = NULL;
	//_u32 tmp_buffer_len = 0;

	EM_LOG_DEBUG("dt_get_task_user_data:%u",task_id);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	p_task = dt_get_task_from_map(task_id);
	
	if(p_task==NULL) 
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}

	if(p_task->_task_info->_have_user_data==FALSE) 
	{
		_p_param->_result = INVALID_USER_DATA;
		goto ErrorHanle;
	}

	_p_param->_result = em_malloc(p_task->_task_info->_user_data_len+1,(void**)&data_buffer_tmp);
	if(_p_param->_result != SUCCESS) goto ErrorHanle;
	
	sd_memset(data_buffer_tmp,0x00,p_task->_task_info->_user_data_len+1);
		
	_p_param->_result = dt_get_task_user_data_impl(p_task,data_buffer_tmp,	p_task->_task_info->_user_data_len);
	if(_p_param->_result == SUCCESS)
	{
		_p_param->_result = dt_get_task_common_user_data(data_buffer_tmp,p_task->_task_info->_user_data_len,&common_data,&common_data_len);
		if(_p_param->_result == SUCCESS)
		{
			if((*buffer_len<common_data_len)||(data_buffer==NULL)) 
			{
				EM_LOG_DEBUG("dt_get_task_user_data:  NOT_ENOUGH_BUFFER--userdata_len = %u",common_data_len);
				_p_param->_result = NOT_ENOUGH_BUFFER;
				*buffer_len=common_data_len;
				EM_SAFE_DELETE(data_buffer_tmp);
				goto ErrorHanle;
			}

			em_memcpy(data_buffer, common_data,common_data_len);
			//data_buffer[common_data_len] = '\0';
			_p_param->_result = SUCCESS;
		}
	}
	
	EM_SAFE_DELETE(data_buffer_tmp);
	
ErrorHanle:
		EM_LOG_DEBUG("dt_get_task_user_data, em_signal_sevent_handle:_result=%d, userdata_len = %d, userdata = %s",_p_param->_result, common_data_len, data_buffer);
		return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 dt_get_task_id_by_state( void * p_param )
{
	POST_PARA_4* _p_param = (POST_PARA_4*)p_param;
	TASK_STATE state = (TASK_STATE )_p_param->_para1;
	_u32 * id_array_buffer = (_u32 * )_p_param->_para2;
	_u32 *buffer_len = (_u32 * )_p_param->_para3;
	BOOL is_local = (BOOL)_p_param->_para4;
	_u32 wait_count = 0;

	while(g_task_locked) 
	{
		if(wait_count++>10000)
		{
			_p_param->_result = ETM_BUSY;
			goto ErrorHanle;
		}
		em_sleep(1);
	}


	_p_param->_result = dt_get_task_id_by_state_impl(state,id_array_buffer,buffer_len,is_local);
	
ErrorHanle:
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 dt_get_all_task_ids( void * p_param )
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	_u32 * id_array_buffer = (_u32 * )_p_param->_para1;
	_u32 *buffer_len = (_u32 * )_p_param->_para2;

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}


	_p_param->_result = dt_get_all_task_ids_impl(id_array_buffer,buffer_len);
	
ErrorHanle:
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 dt_get_all_task_total_file_size( void * p_param )
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	_u64 * p_total_file_size = (_u64 * )_p_param->_para1;

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	_p_param->_result = dt_get_all_task_total_file_size_impl(p_total_file_size);
	
ErrorHanle:
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 dt_get_all_task_need_space( void * p_param )
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	_u64 * p_need_space = (_u64 * )_p_param->_para1;

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	_p_param->_result = dt_get_all_task_need_space_impl(p_need_space);
	
ErrorHanle:
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 em_add_server_resource( void * p_param )
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	_u32 task_id = (_u32  )_p_param->_para1;
	EM_SERVER_RES * p_resource = (EM_SERVER_RES * )_p_param->_para2;
	EM_TASK * p_task = NULL;

	EM_LOG_DEBUG("em_add_server_resource:%u",task_id);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	p_task = dt_get_task_from_map(task_id);
	
	if(p_task==NULL) 
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}

	_p_param->_result = dt_add_server_resource_impl(p_task,p_resource);
	
ErrorHanle:
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 em_add_peer_resource( void * p_param )
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	_u32 task_id = (_u32  )_p_param->_para1;
	EM_PEER_RES * p_resource = (EM_PEER_RES * )_p_param->_para2;
	EM_TASK * p_task = NULL;
	
	EM_LOG_DEBUG("em_add_peer_resource:%u",task_id);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	p_task = dt_get_task_from_map(task_id);

	if(p_task == NULL)
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}
	
	_p_param->_result = dt_add_peer_resource_impl(p_task, p_resource);
	
ErrorHanle:
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 em_get_peer_resource( void * p_param )
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	_u32 task_id = (_u32  )_p_param->_para1;
	EM_PEER_RES * p_resource = (EM_PEER_RES * )_p_param->_para2;
	EM_TASK * p_task = NULL;
	
	EM_LOG_DEBUG("em_get_peer_resource:%u",task_id);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	p_task = dt_get_task_from_map(task_id);

	if(p_task == NULL)
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}
	
	_p_param->_result = dt_get_peer_resource_impl(p_task, p_resource);
	
ErrorHanle:
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}
#ifdef ENABLE_LIXIAN
_int32 em_get_lixian_info( _u32 task_id,_u32 file_index, void * p_lx_info )
{
	_int32 ret_val = SUCCESS;
	_u32 task_inner_id = 0;
	
	EM_LOG_DEBUG("em_get_lixian_info:%u",task_id);

	ret_val = dt_get_running_et_task_id(task_id, &task_inner_id);
	if(ret_val!=SUCCESS) return ret_val;
	
	return et_get_lixian_info(task_inner_id, file_index,(ET_LIXIAN_INFO *) p_lx_info);
	
}
_int32 dt_set_lixian_task_id( void * p_param )
{
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	_u32 task_id = (_u32  )_p_param->_para1;
	_u32 file_index = (_u32  )_p_param->_para2;
	_u64 * p_lx_id = (_u64 * )_p_param->_para3;
	_u64 lixian_id = *p_lx_id;
	EM_TASK * p_task = NULL;
	//TASK_STATE state;
	
	EM_LOG_DEBUG("dt_set_lixian_task_id:%u,file_index=%u,lixian_id=%llu",task_id,file_index,lixian_id);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	p_task = dt_get_task_from_map(task_id);

	if(p_task == NULL)
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}
/*	
	state = dt_get_task_state(p_task);
	if (TS_TASK_RUNNING != state)
	{
		_p_param->_result = NOT_RUNNINT_TASK;
		goto ErrorHanle;
	}
*/	
	_p_param->_result = dt_set_lixian_task_id_impl(p_task, file_index, lixian_id);
	
ErrorHanle:
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 dt_get_lixian_task_id( void * p_param )
{
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	_u32 task_id = (_u32  )_p_param->_para1;
	_u32 file_index = (_u32  )_p_param->_para2;
	_u64 * p_lx_id = (_u64 * )_p_param->_para3;
	EM_TASK * p_task = NULL;
	//TASK_STATE state;
	
	EM_LOG_DEBUG("dt_get_lixian_task_id:%u,file_index=%u",task_id,file_index);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	p_task = dt_get_task_from_map(task_id);

	if(p_task == NULL)
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}
/*	
	state = dt_get_task_state(p_task);
	if (TS_TASK_RUNNING != state)
	{
		_p_param->_result = NOT_RUNNINT_TASK;
		goto ErrorHanle;
	}
*/	
	_p_param->_result = dt_get_lixian_task_id_impl(p_task, file_index, p_lx_id);
	
ErrorHanle:
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}

#endif /* ENABLE_LIXIAN */
_int32 dt_get_task_id_by_eigenvalue_impl( EM_EIGENVALUE * p_eigenvalue ,_u32 * task_id)
{
	_int32 ret_val = SUCCESS;
	_u8  eigenvalue[CID_SIZE];

	if(g_task_locked) 
	{
		ret_val =  ETM_BUSY;
		goto ErrorHanle;
	}

	switch(p_eigenvalue->_type)
	{
		case TT_URL:
		case TT_EMULE:
			ret_val =  dt_get_url_eigenvalue(p_eigenvalue->_url,p_eigenvalue->_url_len,eigenvalue);
			if(ret_val!=SUCCESS) goto ErrorHanle;
			break;
		case TT_TCID:
		case TT_KANKAN:
		case TT_BT:
		case TT_LAN:
			ret_val =  dt_get_cid_eigenvalue(p_eigenvalue->_eigenvalue,eigenvalue);
			if(ret_val!=SUCCESS) goto ErrorHanle;
			break;
		case TT_FILE:
		default:
			ret_val = INVALID_TASK_TYPE;
			goto ErrorHanle;
	}

	if(dt_is_task_exist(p_eigenvalue->_type,eigenvalue,task_id)==FALSE)
	{
		ret_val = INVALID_TASK;
		goto ErrorHanle;
	}
	
	ret_val = SUCCESS;
	
ErrorHanle:

		return ret_val;
}

_int32 dt_get_task_id_by_eigenvalue( void * p_param )
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	EM_EIGENVALUE * p_eigenvalue = (EM_EIGENVALUE * )_p_param->_para1;
	_u32 * task_id = (_u32 * )_p_param->_para2;

	_p_param->_result = dt_get_task_id_by_eigenvalue_impl( p_eigenvalue ,task_id);
	if(_p_param->_result == INVALID_TASK)
	{
		_p_param->_result = vod_get_task_id_by_eigenvalue( p_eigenvalue ,task_id);

	}

		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}
#ifdef ENABLE_LIXIAN
_int32 dt_set_task_lixian_mode( void * p_param )
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	_u32 task_id=(_u32)_p_param->_para1;
	BOOL is_lixian=(_u32)_p_param->_para2;
	EM_TASK * p_task = NULL;

	EM_LOG_DEBUG("dt_set_task_lixian_mode:%u,%d",task_id,is_lixian);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	p_task = dt_get_task_from_map(task_id);
	
	if(p_task==NULL) 
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}

	/* vod 任务不能设置为离线任务 */
	if(dt_is_vod_task(p_task)) 
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}

	_p_param->_result=dt_set_task_lixian_mode_impl(p_task,is_lixian);
	
ErrorHanle:

	EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 dt_is_lixian_task( void * p_param )
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	_u32 task_id = (_u32 )_p_param->_para1;
	BOOL * is_lixian = (BOOL* )_p_param->_para2;
	EM_TASK * p_task = NULL;

	EM_LOG_DEBUG("dt_is_lixian_task:%u",task_id);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	p_task = dt_get_task_from_map(task_id);
	
	if(p_task==NULL) 
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}

	_p_param->_result=dt_is_lixian_task_impl(p_task,is_lixian);
	
ErrorHanle:
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}

#endif /* ENABLE_LIXIAN */

_u32 dt_get_task_inner_id( _u32 task_id )
{
	EM_TASK * p_task = NULL;
	
	if(g_task_locked) 
	{
		return 0;
	}

	p_task = dt_get_task_from_map(task_id);
	
	if(p_task!=NULL) 
	{
		return p_task->_inner_id;
	}
	
	return 0;
}

_int32 dt_set_task_ad_mode( void * p_param )
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	_u32 task_id=(_u32)_p_param->_para1;
	BOOL is_ad=(_u32)_p_param->_para2;
	EM_TASK * p_task = NULL;

	EM_LOG_DEBUG("dt_set_task_ad_mode:%u,%d",task_id,is_ad);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	p_task = dt_get_task_from_map(task_id);
	
	if(p_task==NULL) 
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}

	/* vod 任务不能设置为广告任务 */
	if(dt_is_vod_task(p_task)) 
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}

	p_task->_is_ad = is_ad;
		
	_p_param->_result=dt_set_task_ad_mode_impl(p_task, is_ad);
	
ErrorHanle:

	EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 dt_is_ad_task( void * p_param )
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	_u32 task_id = (_u32 )_p_param->_para1;
	BOOL * is_ad = (BOOL* )_p_param->_para2;
	EM_TASK * p_task = NULL;

	EM_LOG_DEBUG("dt_is_lixian_task:%u",task_id);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	p_task = dt_get_task_from_map(task_id);
	
	if(p_task==NULL) 
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}

	*is_ad = p_task->_is_ad;
	
	_p_param->_result = SUCCESS;
	
ErrorHanle:
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}


#ifdef ENABLE_HSC
_int32 dt_get_task_hsc_info( _u32 task_id, EM_HIGH_SPEED_CHANNEL_INFO *p_hsc_info)
{
	EM_TASK * p_task = NULL;
	_int32 ret_val;
	
	if(g_task_locked) 
	{
		return ETM_BUSY;
	}

	p_task = dt_get_task_from_map(task_id);
	if (0 == p_task)
	{
		return INVALID_TASK_ID;
	}

	ret_val = em_memcpy(p_hsc_info, &p_task->_hsc_info, sizeof(EM_HIGH_SPEED_CHANNEL_INFO));
	CHECK_VALUE(ret_val);

	return ret_val;
}
#endif /* ENABLE_HSC */
#ifdef ENABLE_ETM_MEDIA_CENTER
_int32 dt_get_task_shop_info( _u32 task_id, EM_TASK_SHOP_INFO *p_shop_info )
{
	EM_TASK * p_task = NULL;
	TASK_INFO *task_info;
	
	if(g_task_locked) 
	{
		return ETM_BUSY;
	}

	p_task = dt_get_task_from_map(task_id);
	
	if(p_task!=NULL) 
	{
		task_info = p_task->_task_info;
		p_shop_info->_groupid = task_info->_groupid;
		em_memcpy(p_shop_info->_group_name, task_info->_group_name, EM_MAX_GROUP_NAME_LEN);
		p_shop_info->_group_type = task_info->_group_type;
		p_shop_info->_productid = task_info->_productid;
		p_shop_info->_subid = task_info->_subid;
		p_shop_info->_created_file_size = task_info->_created_file_size;
	}
	else
	{
		em_memset(p_shop_info, 0, sizeof(EM_TASK_SHOP_INFO));
		EM_LOG_ERROR("dt_get_task_shop_info, can not find task");
	}
	
	return SUCCESS;
}
#endif

_int32 dt_set_max_running_tasks(_u32 max_tasks)
{
	return dt_set_max_running_task( max_tasks);
}
BOOL dt_is_task_locked(void)
{
	return g_task_locked;
}
_int32 dt_get_current_download_speed( void * p_param )
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	_u32 * speed = (_u32* )_p_param->_para1;

	EM_LOG_DEBUG("dt_get_current_download_speed");

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	_p_param->_result =dt_get_download_speed(speed);// + vod task download speed ---------- To be done....
ErrorHanle:
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 dt_get_current_upload_speed( void * p_param )
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	_u32 * speed = (_u32* )_p_param->_para1;

	EM_LOG_DEBUG("dt_get_current_upload_speed");

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	_p_param->_result =dt_get_upload_speed(speed);// + vod task upload speed ---------- To be done....
ErrorHanle:
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}

BOOL dt_have_running_task(void )
{
	if(0!= dt_get_running_task_num( ))
		return TRUE;

	return FALSE;
}

_int32 dt_force_scheduler(void)
{
	_int32 ret_val = SUCCESS;

	ret_val=dt_post_next();
	
	return ret_val;
}


_int32 dt_post_next(void)
{
	_int32 ret_val = SUCCESS;
	MSG_INFO msg_info;

	if(g_dt_do_next_msg_id!=0) return -1;
	
	msg_info._device_id = 0;
	msg_info._device_type = DEVICE_COMMON;
	msg_info._msg_priority = MSG_PRIORITY_NORMAL;
	msg_info._operation_parameter = NULL;
	msg_info._operation_type = OP_COMMON;
	msg_info._pending_op_count = 0;
	msg_info._user_data = NULL;

	ret_val = em_post_message(&msg_info, dt_do_next, NOTICE_ONCE, 0, &g_dt_do_next_msg_id);

	return ret_val;
}
_int32 dt_do_next(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid)
{
	EM_LOG_DEBUG("dt_do_next");

	if(/*msg_info->_device_id*/msgid == g_dt_do_next_msg_id)
	{
		g_dt_do_next_msg_id=0;
		dt_scheduler();
	}	
	
	EM_LOG_DEBUG("dt_do_next end!");
	return SUCCESS;
}

/////////////////////////////////
_int32 dt_create_vod_task( void * p_param )
{
	//_int32 ret_val = SUCCESS;
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	EM_CREATE_VOD_TASK * p_create_param =(EM_CREATE_VOD_TASK *)_p_param->_para1;
	_u32* p_task_id =(_u32 *)_p_param->_para2;
	EM_TASK * p_task = NULL;
	TASK_STATE task_state;

	EM_LOG_DEBUG("dt_create_vod_task:%d",p_create_param->_type);

	*p_task_id=0;

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
	}
	/*  Just for testing */
	else if(p_create_param->_is_no_disk && (dt_get_running_vod_task_num()!=0))
	{
		_p_param->_result = TOO_MANY_TASKS;
	}
	/***************/
	else
	{
		CREATE_TASK create_task_param;

		if(p_create_param->_type ==TT_URL)
		{
			//decode base64
			// is kankan url ?
			EM_LOG_DEBUG("vod_create_task_impl:url[%u]=%s",p_create_param->_url_len,p_create_param->_url);
		}

		em_memset(&create_task_param,0x00,sizeof(CREATE_TASK));
		create_task_param._type = p_create_param->_type;
		create_task_param._url = p_create_param->_url;
		create_task_param._url_len = p_create_param->_url_len;
		create_task_param._ref_url = p_create_param->_ref_url;
		create_task_param._ref_url_len = p_create_param->_ref_url_len;
		create_task_param._seed_file_full_path = p_create_param->_seed_file_full_path;
		create_task_param._seed_file_full_path_len = p_create_param->_seed_file_full_path_len;
		create_task_param._download_file_index_array = p_create_param->_download_file_index_array;
		create_task_param._file_num = p_create_param->_file_num;
		create_task_param._tcid = p_create_param->_tcid;
		create_task_param._file_size = p_create_param->_file_size;
		create_task_param._gcid = p_create_param->_gcid;
		create_task_param._user_data = p_create_param->_user_data;
		create_task_param._user_data_len = p_create_param->_user_data_len;

		create_task_param._file_path = dt_get_vod_cache_path_for_lan();//dt_get_vod_cache_path_impl( );
		create_task_param._file_path_len = em_strlen(create_task_param._file_path);
		create_task_param._manual_start = FALSE;

		create_task_param._check_data = p_create_param->_is_no_disk; //p_create_param._check_data;//_check_data 用于_is_no_disk -----zyq @20101027 fro vod task

		create_task_param._file_name = p_create_param->_file_name;
		create_task_param._file_name_len = p_create_param->_file_name_len;
		/* check if vod cache is full , only save the last vod task cache*/
		dt_clear_vod_cache_impl(TRUE);

		if (!create_task_param._file_path_len)	//若无设置点播文件路径，无盘点播设置一个虚拟路径，有盘点播返回错误
		{
			if (create_task_param._check_data)
			{
				create_task_param._file_path = "vod_defaultpath/";
				create_task_param._file_path_len = em_strlen(create_task_param._file_path);
				_p_param->_result = dt_create_task_impl( &create_task_param,p_task_id ,TRUE,FALSE,FALSE);
			}
			else
				_p_param->_result = INVALID_FILE_PATH; 
		}
		else
			_p_param->_result = dt_create_task_impl( &create_task_param,p_task_id ,TRUE,FALSE, FALSE);
		// 有盘点播任务已经存在处理
		if(_p_param->_result ==TASK_ALREADY_EXIST) 
		{
			p_task = dt_get_task_from_map(*p_task_id);
			_p_param->_result = dt_check_task_free_disk(p_task,dt_get_task_file_path(p_task));
			if(_p_param->_result == INSUFFICIENT_DISK_SPACE&&create_task_param._check_data)
			{
				/* 已存在的任务用无盘方式再次创建 ,暂不支持!*/
				sd_assert(FALSE);
			}
		}
		
		if(_p_param->_result==SUCCESS)
		{
			if(p_task==NULL)
			{
				p_task = dt_get_task_from_map(*p_task_id);
			}
			sd_assert(p_task!=NULL);
			task_state = dt_get_task_state(p_task);
			if(task_state==TS_TASK_DELETED)
			{
				_p_param->_result=dt_recover_task_impl(p_task);
				task_state = dt_get_task_state(p_task);
			}
#ifndef KANKAN_PROJ
			if(_p_param->_result==SUCCESS)
			{
				switch(task_state)
				{
					case TS_TASK_PAUSED:
					case TS_TASK_FAILED:
					case TS_TASK_WAITING:
						_p_param->_result = dt_force_start_task(p_task);
						if(_p_param->_result == ERR_VOD_DATA_FETCH_TIMEOUT)
						{
							_p_param->_result = SUCCESS;
						}
						break;
					case TS_TASK_SUCCESS:
						sd_assert(p_task->_inner_id == 0);
						if(p_task->_inner_id == 0)
						{
							char file_full_path[MAX_FULL_PATH_BUFFER_LEN];
							char * file_path = dt_get_task_file_path( p_task);
							char * file_name = dt_get_task_file_name( p_task);
							sd_assert(file_path!=NULL);
							sd_assert(file_name!=NULL);
							if((file_path==NULL)||(file_name==NULL))
							{
								_p_param->_result = INVALID_FILE_PATH;
								break;
							}
							em_memset(file_full_path,0x00,MAX_FULL_PATH_BUFFER_LEN);
							em_strcat(file_full_path,file_path,p_task->_task_info->_file_path_len);
							if(file_full_path[p_task->_task_info->_file_path_len-1]!='/') em_strcat(file_full_path,"/",1);
							em_strcat(file_full_path,file_name,p_task->_task_info->_file_name_len);
							_p_param->_result = em_open_ex(file_full_path,O_FS_RDONLY,&p_task->_inner_id);

							if(_p_param->_result != SUCCESS)
							{
								/* 文件可能被用户手工删除了,需要重新下载! */
								p_task->_task_info->_state = TS_TASK_WAITING;
								dt_add_task_to_order_list(p_task);
								_p_param->_result = dt_force_start_task(p_task);
								if(_p_param->_result == ERR_VOD_DATA_FETCH_TIMEOUT)
								{
									_p_param->_result = SUCCESS;
								}
							}
						}
						break;
					case TS_TASK_RUNNING:
					case TS_TASK_DELETED:
						break;
				}
			}
#endif
		}
	}

	EM_LOG_DEBUG("dt_create_vod_task %u end!em_signal_sevent_handle:_result=%d",*p_task_id,_p_param->_result);
	if(_p_param->_result==SUCCESS)
	{
		/* start net work */
		em_is_net_ok(TRUE);
		
		dt_force_scheduler();
	}
	else
	{
		*p_task_id = 0;
	}
	return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 dt_stop_vod_task( void * p_param )
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	_u32 task_id=(_u32)_p_param->_para1;
	TASK_STATE state;
	EM_TASK * p_task = NULL;
    BOOL thread_unlock = (BOOL)_p_param->_para2;

	EM_LOG_DEBUG("@@@@@@@@@@@@ dt_stop_vod_task:%u",task_id);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	p_task = dt_get_task_from_map(task_id);
	
	if(p_task==NULL) 
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}
	
	if(task_id == dt_get_current_vod_task_id())
	{
		if(dt_is_reading_success_file())
		{
			if(p_task->_inner_id!=0)
			{
				em_close_ex(p_task->_inner_id);
				p_task->_inner_id = 0;
			}
		}
		
		dt_set_current_vod_task_id(0);
		dt_set_reading_success_file(FALSE);
		//dt_decrease_running_vod_task_num();
		if ((p_task->_task_info->_downloaded_data_size == p_task->_task_info->_file_size)
			&& !dt_is_vod_task(p_task) && (dt_get_running_vod_task_num() > 0) && !(p_task->_is_ad))
		{
			dt_decrease_running_vod_task_num();
		}
			
	}
	
	/* 是否点播任务? */
	if(dt_is_vod_task(p_task))
	{
		dt_stop_vod_task_impl(p_task);
	}
	else
	{

		if(((_int32)p_task->_inner_id != 0) && (!p_task->_is_ad) && (dt_get_running_task_num() < 2 && !(p_task->_need_pause)))
		{
			/* 这个任务可以继续下载，不停止,只退出点播模式 */
			_p_param->_result = et_stop_vod((_int32)p_task->_inner_id,  INVALID_FILE_INDEX);
			//sd_assert(_p_param->_result == SUCCESS);
			//_p_param->_result = SUCCESS;
			goto ErrorHanle;
		}
		
		if(((_int32)p_task->_inner_id != 0) && dt_is_lan_task(p_task) && (dt_get_running_lan_task_num() < 2) && !(p_task->_need_pause))
		{
			/* 这个局域网任务可以继续下载，不停止,只退出点播模式 */
			_p_param->_result = et_stop_vod((_int32)p_task->_inner_id,  INVALID_FILE_INDEX);
			sd_assert(_p_param->_result == SUCCESS);
			//_p_param->_result = SUCCESS;
			goto ErrorHanle;			
		}

		state = dt_get_task_state(p_task);
		if(TS_TASK_SUCCESS == state)
		{
			if(p_task->_inner_id!=0)
			{
				em_close_ex(p_task->_inner_id);
				p_task->_inner_id = 0;
			}
		}
	
		if((TS_TASK_WAITING!=state)&&(TS_TASK_RUNNING!=state))
		{
			_p_param->_result = INVALID_TASK_STATE;
			goto ErrorHanle;
		}

		if(TS_TASK_WAITING==state)
		{	
			if (!dt_is_vod_task(p_task) && !p_task->_need_pause) {
				_p_param->_result = SUCCESS;
			}
			else {
				_p_param->_result = dt_stop_task_impl(p_task);;
			}
		}
		else
		{
			p_task->_waiting_stop = TRUE;
			dt_have_task_waiting_stop();
			p_task->_vod_stop = TRUE;
		}
	}
	
ErrorHanle:

	if(_p_param->_result==SUCCESS)
	{
		dt_force_scheduler();
#ifdef ENABLE_RC
        rc_remove_static_task_id( task_id );
#endif /* ENABLE_RC */
	}

    if(!thread_unlock)
    {
        return _p_param->_result;
	}
	EM_LOG_DEBUG("dt_stop_vod_task, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 dt_vod_read_file( void * p_param )
{
	POST_PARA_5* _p_param = (POST_PARA_5*)p_param;
	_u32 task_id=(_u32)_p_param->_para1;
	_u64 * start_pos = (_u64*)_p_param->_para2;
	_u64 * len=(_u64*)_p_param->_para3;
	char * buffer=(char *)_p_param->_para4;
	_u32 block_time=(_u32)_p_param->_para5;
	EM_TASK * p_task = NULL;
	_u32 readsize = 0;
	TASK_STATE state;
	_int32 ret_val = SUCCESS;
	BOOL is_reading_success_file = FALSE;

	EM_LOG_DEBUG("vod_read_file:%u",task_id);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	p_task = dt_get_task_from_map(task_id);
	
	if(p_task==NULL) 
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}

	if(p_task->_waiting_stop) 
	{
		_p_param->_result = INVALID_TASK_STATE;
		sd_assert(FALSE);
		goto ErrorHanle;
	}
	
	if(*start_pos > p_task->_task_info->_file_size)
	{
		_p_param->_result = INVALID_ARGUMENT;
		goto ErrorHanle;
	}
	
	
	state = dt_get_task_state(p_task);
	switch(state)
	{
		case TS_TASK_RUNNING:
			ret_val = eti_vod_read_file((_int32)p_task->_inner_id, *start_pos,*len, buffer, (_int32)block_time );
			break;
		case TS_TASK_WAITING:
		case TS_TASK_FAILED:
			/* 启动之前检查剩余空间是否足够 */
			ret_val = dt_check_task_free_disk(p_task,dt_get_task_file_path(p_task));
			/* 启动任务 */
			if(ret_val == SUCCESS)
			{
				ret_val = dt_force_start_task(p_task);
			}
			else
			{
				//清空当前点播的task id
				if(task_id == dt_get_current_vod_task_id())
				{
					dt_set_current_vod_task_id(0);
					dt_set_reading_success_file(FALSE);
					//dt_decrease_running_vod_task_num();
				}	
			}
			break;
		case TS_TASK_SUCCESS:
			#ifdef KANKAN_PROJ
			if ((!p_task->_is_ad) && (!dt_is_lan_task(p_task))) 
			{
				if((dt_get_current_vod_task_id() == task_id)&&(dt_is_reading_success_file()))
				{
					goto READ_SUCCESS_FILE;
				}
				p_task->_task_info->_state = TS_TASK_WAITING;
				dt_add_task_to_order_list(p_task);
				ret_val = dt_force_start_task(p_task);	
				if(p_task->_task_info->_state == TS_TASK_SUCCESS&&(p_task->_inner_id == 0))
				{
					is_reading_success_file = TRUE;
					goto READ_SUCCESS_FILE;
				}
			}
			else 
			{
			READ_SUCCESS_FILE:
				//
				if(p_task->_inner_id == 0)
				{
					char file_full_path[MAX_FULL_PATH_BUFFER_LEN];
					char * file_path = dt_get_task_file_path( p_task);
					char * file_name = dt_get_task_file_name( p_task);
					sd_assert(file_path!=NULL);
					sd_assert(file_name!=NULL);
					if((file_path==NULL)||(file_name==NULL))
					{
						ret_val = INVALID_FILE_PATH;
						break;
					}
					em_memset(file_full_path,0x00,MAX_FULL_PATH_BUFFER_LEN);
					em_strcat(file_full_path,file_path,p_task->_task_info->_file_path_len);
					if(file_full_path[p_task->_task_info->_file_path_len-1]!='/') em_strcat(file_full_path,"/",1);
					em_strcat(file_full_path,file_name,p_task->_task_info->_file_name_len);
					ret_val = em_open_ex(file_full_path,O_FS_RDONLY,&p_task->_inner_id);
					
					if(ret_val != SUCCESS)
					{
						/* 文件可能被用户手工删除了,需要重新下载! */
						p_task->_task_info->_state = TS_TASK_WAITING;
						dt_add_task_to_order_list(p_task);
						ret_val = dt_force_start_task(p_task);
						break;
					}
					
					if(is_reading_success_file)
					{
						dt_set_reading_success_file(TRUE);
					}
				}
				ret_val=em_pread((_int32)p_task->_inner_id,buffer,(_int32)*len, *start_pos,  &readsize );				
			}
			#else
			if(p_task->_inner_id == 0)
			{
				char file_full_path[MAX_FULL_PATH_BUFFER_LEN];
				char * file_path = dt_get_task_file_path( p_task);
				char * file_name = dt_get_task_file_name( p_task);
				sd_assert(file_path!=NULL);
				sd_assert(file_name!=NULL);
				if((file_path==NULL)||(file_name==NULL))
				{
					ret_val = INVALID_FILE_PATH;
					break;
				}
				em_memset(file_full_path,0x00,MAX_FULL_PATH_BUFFER_LEN);
				em_strcat(file_full_path,file_path,p_task->_task_info->_file_path_len);
				if(file_full_path[p_task->_task_info->_file_path_len-1]!='/') em_strcat(file_full_path,"/",1);
				em_strcat(file_full_path,file_name,p_task->_task_info->_file_name_len);
				ret_val = em_open_ex(file_full_path,O_FS_RDONLY,&p_task->_inner_id);

				if(ret_val != SUCCESS)
				{
					/* 文件可能被用户手工删除了,需要重新下载! */
					p_task->_task_info->_state = TS_TASK_WAITING;
					dt_add_task_to_order_list(p_task);
					ret_val = dt_force_start_task(p_task);
					break;
				}
			}
			ret_val=em_pread((_int32)p_task->_inner_id,buffer,(_int32)*len, *start_pos,  &readsize );
			#endif
			break;
		case TS_TASK_PAUSED:
		case TS_TASK_DELETED:
			ret_val =INVALID_TASK_STATE;
			break;
	}
	
	_p_param->_result = ret_val;
	if(_p_param->_result==SUCCESS||_p_param->_result==ERR_VOD_DATA_FETCH_TIMEOUT)
	{
		//设置当前点播的task id
		if(dt_get_current_vod_task_id() != task_id)
		{
			//sd_assert(dt_get_current_vod_task_id() == 0);
			dt_set_current_vod_task_id(task_id);
			//dt_increase_running_vod_task_num();
		}
		if (((p_task->_task_info->_downloaded_data_size == p_task->_task_info->_file_size) && (p_task->_task_info->_file_size > 0))
			&& !dt_is_vod_task(p_task) && (dt_get_running_vod_task_num() == 0) && !(p_task->_is_ad)) 
		{
			//下载任务点播时下完了也增加正在运行的点播任务数，目的是让调度可以开始一个等待的任务
			if(!dt_is_lan_task(p_task))
			{
				dt_increase_running_vod_task_num();
			}
		}
	}

	
ErrorHanle:

	EM_LOG_DEBUG("dt_vod_read_file, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));

}

_int32 dt_force_start_task(EM_TASK * p_task)
{
	_int32 ret_val = SUCCESS;
	
	//if(!dt_is_vod_task(p_task))
	//{
		/* 下载优先级置顶 */
	//	dt_pri_level_up_impl( p_task->_task_info->_task_id, MAX_U32);
	//}
	if(dt_is_running_task_full())
	{
		/* 停掉最近启动的任务 */
		dt_stop_the_latest_task();
	}
	ret_val=dt_start_task_impl(p_task);
	sd_assert(ret_val == SUCCESS);
	if(ret_val == NETWORK_INITIATING)
	{
		ret_val = SUCCESS;
	}
	sd_assert(ret_val == SUCCESS);
	if(ret_val != SUCCESS)
	{
		dt_stop_vod_task_impl(p_task);
	}
	else
	{
		/* 强制返回超时  */
		ret_val = ERR_VOD_DATA_FETCH_TIMEOUT;
	}
	return ret_val;
}

_int32 dt_vod_read_bt_file( void * p_param )
{
	POST_PARA_6* _p_param = (POST_PARA_6*)p_param;
	_u32 task_id=(_u32)_p_param->_para1;
	_u64 * start_pos = (_u64*)_p_param->_para2;
	_u64 * len=(_u64*)_p_param->_para3;
	char * buffer=(char *)_p_param->_para4;
	_u32 block_time=(_u32)_p_param->_para5;
	_u32 file_index=(_u32)_p_param->_para6;
	EM_TASK * p_task = NULL;

	EM_LOG_DEBUG("vod_read_file:%u",task_id);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	p_task = dt_get_task_from_map(task_id);
	
	if(p_task==NULL) 
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}
		

	if(TS_TASK_RUNNING==dt_get_task_state(p_task))
	{
		_p_param->_result=eti_vod_bt_read_file((_int32) p_task->_inner_id,  file_index, *start_pos,  *len, buffer, (_int32) block_time );
	}
	else
	/*
	if(TS_TASK_SUCCESS==dt_get_task_state(p_task))
	{
		if(p_task->_inner_id == 0)
		{
			char file_full_path[MAX_FULL_PATH_BUFFER_LEN];
			char * file_path = dt_get_task_file_path( p_task);
			char * file_name = dt_get_task_file_name( p_task);
			sd_assert(file_path!=NULL);
			sd_assert(file_name!=NULL);
			if((file_path==NULL)||(file_name==NULL))
			{
				_p_param->_result = -1;
				goto ErrorHanle;
			}
			em_memset(file_full_path,0x00,MAX_FULL_PATH_BUFFER_LEN);
			em_strcat(file_full_path,file_path,p_task->_task_info->_file_path_len);
			if(file_full_path[p_task->_task_info->_file_path_len-1]!='/') em_strcat(file_full_path,"/",1);
			em_strcat(file_full_path,file_name,p_task->_task_info->_file_name_len);
			_p_param->_result = em_open_ex(file_full_path,O_FS_RDONLY,&p_task->_inner_id);
			if(_p_param->_result!=SUCCESS) goto ErrorHanle;
		}
		_p_param->_result=em_pread((_int32)p_task->_inner_id, *start_pos,*len, buffer, (_int32)block_time );
	}
	else
	*/
	{
		_p_param->_result = INVALID_TASK_STATE;
		goto ErrorHanle;
	}

	
ErrorHanle:

	EM_LOG_DEBUG("dt_vod_read_bt_file, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 dt_vod_get_buffer_percent( void * p_param )
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	_u32 task_id=(_u32)_p_param->_para1;
	_u32 * percent=(_u32 *)_p_param->_para2;
	_u32 inner_id = 0;
	EM_TASK * p_task = NULL;

	EM_LOG_DEBUG("vod_get_buffer_percent:%u",task_id);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	p_task = dt_get_task_from_map(task_id);
	
	if(p_task==NULL) 
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}
		
	if(TS_TASK_SUCCESS==dt_get_task_state(p_task))
	{
		*percent = 100;
		goto ErrorHanle;
	}
	else
	if(TS_TASK_RUNNING!=dt_get_task_state(p_task))
	{
		_p_param->_result = INVALID_TASK_STATE;
		goto ErrorHanle;
	}

	inner_id =  p_task->_inner_id;
	_p_param->_result=eti_vod_get_buffer_percent((_int32)inner_id, (_int32 *)percent );

	
ErrorHanle:

	EM_LOG_DEBUG("dt_vod_get_buffer_percent, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 dt_vod_get_download_position( void * p_param )
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	_u32 task_id=(_u32)_p_param->_para1;
	_u64 * position=(_u64 *)_p_param->_para2;
	_u32 inner_id = 0;
	EM_TASK * p_task = NULL;

	EM_LOG_DEBUG("vod_get_download_position:%u",task_id);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	p_task = dt_get_task_from_map(task_id);
	
	if(p_task==NULL) 
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}
		
	if(TS_TASK_SUCCESS==dt_get_task_state(p_task))
	{
		*position = p_task->_task_info->_file_size;
		goto ErrorHanle;
	}
	else
	if(TS_TASK_RUNNING!=dt_get_task_state(p_task))
	{
		_p_param->_result = INVALID_TASK_STATE;
		goto ErrorHanle;
	}

	inner_id =  p_task->_inner_id;
	_p_param->_result=eti_vod_get_download_position((_int32)inner_id, (_u64 *)position );

	
ErrorHanle:

	EM_LOG_DEBUG("dt_vod_get_download_position, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 dt_vod_set_download_mode(void * p_param)
{
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	_u32 task_id = (_u32)_p_param->_para1;
	BOOL is_download = (BOOL)_p_param->_para2;
	_u32 file_retain_time = (_u32)_p_param->_para3;

	EM_LOG_DEBUG("dt_vod_set_download_mode:%u,%d", task_id, is_download);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	_p_param->_result = dt_set_task_download_mode( task_id, is_download, file_retain_time);
	
ErrorHanle:

	EM_LOG_DEBUG("dt_vod_set_download_mode, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 dt_vod_get_download_mode(void * p_param)
{
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	_u32 task_id = (_u32)_p_param->_para1;
	BOOL* is_download = (BOOL*)_p_param->_para2;
	_u32* file_retain_time = (_u32*)_p_param->_para3;

	EM_LOG_DEBUG("dt_vod_get_download_mode:%u", task_id);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	_p_param->_result = dt_get_task_download_mode(task_id,is_download,file_retain_time);
	
ErrorHanle:

	EM_LOG_DEBUG("dt_vod_get_download_mode, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 dt_vod_is_download_finished( void * p_param )
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	_u32 task_id=(_u32)_p_param->_para1;
	BOOL * finished=(BOOL *)_p_param->_para2;
	_u32 inner_id = 0;
	EM_TASK * p_task = NULL;

	EM_LOG_DEBUG("vod_is_download_finished:%u",task_id);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	p_task = dt_get_task_from_map(task_id);
	
	if(p_task==NULL) 
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}
		
	if(TS_TASK_SUCCESS==dt_get_task_state(p_task))
	{
		*finished = TRUE;
		goto ErrorHanle;
	}
	else
	if(TS_TASK_RUNNING!=dt_get_task_state(p_task))
	{
		_p_param->_result = INVALID_TASK_STATE;
		goto ErrorHanle;
	}

	inner_id =  p_task->_inner_id;
	_p_param->_result=eti_vod_is_download_finished((_int32)inner_id, finished );

	
ErrorHanle:

	EM_LOG_DEBUG("dt_vod_is_download_finished, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 dt_vod_get_bitrate( void * p_param )
{
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	_u32 task_id=(_u32)_p_param->_para1;
	_u32 file_index=(_u32)_p_param->_para2;
	_u32 * bitrate=(_u32 *)_p_param->_para3;
	_u32 inner_id = 0;
	EM_TASK * p_task = NULL;

	EM_LOG_DEBUG("vod_get_bitrate:%u",task_id);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	p_task = dt_get_task_from_map(task_id);
	
	if(p_task==NULL) 
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}
		
	if(TS_TASK_SUCCESS==dt_get_task_state(p_task))
	{
		*bitrate = 64*1024*8;
		goto ErrorHanle;
	}
	
	if(TS_TASK_RUNNING!=dt_get_task_state(p_task))
	{
		_p_param->_result = INVALID_TASK_STATE;
		goto ErrorHanle;
	}

	inner_id =  p_task->_inner_id;
	_p_param->_result=eti_vod_get_bitrate((_int32)inner_id, file_index, bitrate );

	
ErrorHanle:

	EM_LOG_DEBUG("dt_vod_get_bitrate, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 dt_vod_report(void * p_param)
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	_u32 task_id = (_u32)_p_param->_para1;
	ET_VOD_REPORT* p_report = (ET_VOD_REPORT*)_p_param->_para2;
	EM_TASK * p_task = NULL;

	EM_LOG_DEBUG("dt_vod_report:%u", task_id);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	p_task = dt_get_task_from_map(task_id);
	
	if(p_task==NULL) 
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}

#ifdef IPAD_KANKAN
	//判断是否广告类型任务，加入上报参数
	p_report->_is_ad_type = p_task->_is_ad;
#else
	p_report->_is_ad_type = FALSE;
#endif

	if(TS_TASK_RUNNING==dt_get_task_state(p_task))
	{
		_p_param->_result = et_vod_report( p_task->_inner_id, p_report);;
	}
	
ErrorHanle:

	EM_LOG_DEBUG("dt_vod_report, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 dt_vod_final_server_host(void * p_param)
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	_u32 task_id = (_u32)_p_param->_para1;
	char* p_host = (char*)_p_param->_para2;
	EM_TASK * p_task = NULL;

	EM_LOG_DEBUG("dt_vod_final_server_host:%u", task_id);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	p_task = dt_get_task_from_map(task_id);
	
	if(p_task==NULL) 
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}


	if(TS_TASK_RUNNING==dt_get_task_state(p_task))
	{
		_p_param->_result = -1;//et_vod_final_server_host( p_task->_inner_id, p_host);
	}
	else
	{
		_p_param->_result = INVALID_TASK_STATE;
	}
ErrorHanle:

	EM_LOG_DEBUG("dt_vod_report, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

/////界面通知下载库开始点播(只适用于云点播任务)
_int32 dt_vod_ui_notify_start_play(void * p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	_u32 task_id = (_u32)_p_param->_para1;
	EM_TASK * p_task = NULL;

	EM_LOG_DEBUG("dt_vod_ui_notify_start_play:%u", task_id);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	p_task = dt_get_task_from_map(task_id);
	
	if(p_task==NULL) 
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}


	if(TS_TASK_RUNNING==dt_get_task_state(p_task))
	{
		_p_param->_result = -1;//et_vod_ui_notify_start_play( p_task->_inner_id);
	}
	else
	{
		_p_param->_result = INVALID_TASK_STATE;
	}
ErrorHanle:

	EM_LOG_DEBUG("dt_vod_report, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}


/////2.8 设置有盘点播的默认临时文件缓存路径,必须已存在且可写,path_len 必须小于ETM_MAX_FILE_PATH_LEN
_int32 dt_set_vod_cache_path( void * p_param )
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	char *cache_path= (char * )_p_param->_para1;

	EM_LOG_DEBUG("dt_set_vod_cache_path:%s",cache_path);
	
	_p_param->_result=dt_set_vod_cache_path_impl( cache_path  );
	// 修正下载库下载任务的路径
	if(!g_task_locked) 
	{
		dt_correct_all_tasks_path();
	}
	else
	{
		g_need_correct_path = TRUE;
	}
	
		EM_LOG_DEBUG("dt_set_vod_cache_path, em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 dt_get_vod_cache_path( void * p_param )
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	char *cache_path= (char * )_p_param->_para1;

	EM_LOG_DEBUG("vod_get_cache_path");

	_p_param->_result = em_settings_get_str_item("system.vod_cache_path", cache_path);
	if(em_strlen(cache_path)==0)
	{
		_p_param->_result = INVALID_FILE_PATH;
	}
	
		EM_LOG_DEBUG("dt_get_vod_cache_path, em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}


/////2.9 设置有盘点播的缓存区大小，单位KB，即接口etm_set_vod_cache_path 所设进来的目录的最大可写空间，建议3GB 以上
_int32 dt_set_vod_cache_size( void * p_param )
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	_u32 cache_size= (_u32 )_p_param->_para1;

	EM_LOG_DEBUG("dt_set_vod_cache_size:%u",cache_size);
	
	_p_param->_result = dt_set_vod_cache_size_impl(cache_size);
	
		EM_LOG_DEBUG("dt_set_vod_cache_size, em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32  dt_get_vod_cache_size( void * p_param )
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	_u32 * cache_size= (_u32 * )_p_param->_para1;

	EM_LOG_DEBUG("vod_get_cache_size");

	*cache_size = dt_get_vod_cache_size_impl(  );
	
	//_p_param->_result=em_settings_get_int_item("system.vod_cache_size", (_int32 *)cache_size);
	
		EM_LOG_DEBUG("dt_get_vod_cache_size, em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32  dt_clear_vod_cache( void * p_param )
{
	POST_PARA_0* _p_param = (POST_PARA_0*)p_param;

	EM_LOG_DEBUG("dt_clear_vod_cache");
	_p_param->_result = dt_clear_vod_cache_impl(TRUE);
		EM_LOG_DEBUG("dt_clear_vod_cache, em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}

/////设置缓冲时间，单位秒,默认为30秒缓冲
_int32 dt_set_vod_buffer_time(void * p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	_u32 buffer_time = (_u32 )_p_param->_para1;

	EM_LOG_DEBUG("dt_set_vod_buffer_time:%u", buffer_time);

	_p_param->_result = em_settings_set_int_item("system.vod_buffer_time", (_int32)buffer_time);
	if(_p_param->_result == SUCCESS)
	{
		if(em_is_et_running() == TRUE)
		{
			_p_param->_result = eti_vod_set_buffer_time((_int32)buffer_time);	
		}
		//else: set by em_set_et_config()
	}

	EM_LOG_DEBUG("dt_set_vod_buffer_time, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

/////2.10 设置点播的专用内存大小,单位KB，建议2MB 以上
_int32 dt_set_vod_buffer_size( void * p_param )
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	_u32 buffer_size= (_u32 )_p_param->_para1;

	EM_LOG_DEBUG("dt_set_vod_buffer_size:%u",buffer_size);
	
	_p_param->_result=em_settings_set_int_item("system.vod_buffer_size", (_int32)buffer_size);
	if(_p_param->_result==SUCCESS)
	{
		if(em_is_et_running() == TRUE)
		{
			_p_param->_result=eti_vod_set_vod_buffer_size((_int32)buffer_size*1024);	
		}
		//else: set by em_set_et_config()
	}
	
		EM_LOG_DEBUG("dt_set_vod_buffer_size, em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 dt_get_vod_buffer_size( void * p_param )
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	_u32 * buffer_size= (_u32 * )_p_param->_para1;
	//_int32 buffer_size_int=0;

	EM_LOG_DEBUG("vod_get_buffer_size");

	*buffer_size = 0;
	// _p_param->_result= eti_vod_get_vod_buffer_size(&buffer_size_int);
	// if(_p_param->_result==SUCCESS)
	// {
	// 	*buffer_size=buffer_size_int/(1024);
	// }
	
	_p_param->_result=em_settings_get_int_item("system.vod_buffer_size", (_int32 *)buffer_size);
	
		EM_LOG_DEBUG("dt_set_vod_buffer_size, em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}


/////2.11 查询vod 专用内存是否已经分配
_int32 dt_is_vod_buffer_allocated( void * p_param )
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	BOOL * is_allocated= (BOOL * )_p_param->_para1;
	_int32 allocated=0;

	EM_LOG_DEBUG("dt_is_vod_buffer_allocated");

	*is_allocated = FALSE;
	if(em_is_et_running() == TRUE)
	{
		 _p_param->_result= eti_vod_is_vod_buffer_allocated(&allocated);
	}
	 if(_p_param->_result==SUCCESS)
	 {
	 	if(allocated!=0)
			*is_allocated = TRUE;
	 }
	
		EM_LOG_DEBUG("dt_is_vod_buffer_allocated, em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}

/////2.12 手工释放vod 专用内存,ETM 本身在反初始化时会自动释放这块内存，但如果界面程序想尽早腾出这块内存的话，可调用此接口，注意调用之前要确认无点播任务在运行
_int32  dt_free_vod_buffer( void * p_param )
{
	POST_PARA_0* _p_param = (POST_PARA_0*)p_param;

	EM_LOG_DEBUG("dt_free_vod_buffer");
	
	if(em_is_et_running() == TRUE)
	{
		_p_param->_result=eti_vod_free_vod_buffer();
	 }
	
		EM_LOG_DEBUG("dt_free_vod_buffer, em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}

/*****************************************************************************************/
/******   高速通道相关接口            **************************/
#ifdef ENABLE_HSC
_int32 dt_get_hsc_info( _u32 task_id, u32 file_index,EM_HSC_INFO * p_hsc_info )
{
	_int32 ret_val = SUCCESS;
	_u32 task_inner_id = 0;
	ET_HIGH_SPEED_CHANNEL_INFO et_hsc_info={0};

	ret_val = dt_get_running_et_task_id(task_id, &task_inner_id);
       if(ret_val == SUCCESS)
	{
		ret_val = et_get_hsc_info(task_inner_id, &et_hsc_info);
		if(ret_val == SUCCESS)
		{
			switch(et_hsc_info._stat) 
			{
				case ET_HSC_IDLE:
				case ET_HSC_STOP:
					p_hsc_info->_state = HS_IDLE; 
					break;
				case ET_HSC_ENTERING:
				case ET_HSC_SUCCESS:
					p_hsc_info->_state = HS_RUNNING; 
					break;
				case ET_HSC_FAILED:
					p_hsc_info->_state = HS_FAILED;
					break;
			}
			
			p_hsc_info->_res_num= et_hsc_info._res_num;
			p_hsc_info->_dl_bytes = et_hsc_info._dl_bytes;
			p_hsc_info->_speed = et_hsc_info._speed;
			p_hsc_info->_errcode = et_hsc_info._errcode;
		}
	}
	return ret_val;
}

_int32 dt_is_hsc_usable( _u32 task_id, _u32 file_index ,BOOL * p_usable)
{
	_int32 ret_val = SUCCESS;
	_u32 task_inner_id = 0;
	ET_HIGH_SPEED_CHANNEL_INFO et_hsc_info={0};

	ret_val = dt_get_running_et_task_id(task_id, &task_inner_id);
       if(ret_val == SUCCESS)
	{
		ret_val = et_get_hsc_info(task_inner_id, &et_hsc_info);
		if(ret_val == SUCCESS)
		{
			*p_usable = et_hsc_info._can_use;
		}
	}
	return ret_val;
		
}

_int32 dt_open_high_speed_channel( void * p_param )
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	_u32 task_id=(_u32)_p_param->_para1;
	TASK_STATE state;
	EM_TASK * p_task = NULL;
    BOOL thread_unlock = (BOOL)_p_param->_para2;
	ET_HIGH_SPEED_CHANNEL_INFO hsc_info;

	EM_LOG_DEBUG("@@@@@@@@@@@@ dt_open_high_speed_channel:%u",task_id);

	if(g_task_locked) 
	{
		_p_param->_result = ETM_BUSY;
		goto ErrorHanle;
	}

	p_task = dt_get_task_from_map(task_id);
	
	if(p_task==NULL) 
	{
		_p_param->_result = INVALID_TASK_ID;
		goto ErrorHanle;
	}
	
	state = dt_get_task_state(p_task);
	if (TS_TASK_RUNNING != state)
	{
		_p_param->_result = NOT_RUNNINT_TASK;
		goto ErrorHanle;
	}

	if (et_get_hsc_info(p_task->_inner_id, &hsc_info) == SUCCESS)
	{
		if (ET_HSC_SUCCESS == hsc_info._stat)
		{
			_p_param->_result = HIGH_SPEED_ALREADY_OPENED;
			goto ErrorHanle;
		}

		if (ET_HSC_ENTERING == hsc_info._stat)
		{
			_p_param->_result = HIGH_SPEED_OPENING;
			goto ErrorHanle;
		}
	}

	_p_param->_result = -1;//et_high_speed_channel_switch(p_task->_inner_id, TRUE, p_task->_task_infon);
	
ErrorHanle:

	EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);

    if(!thread_unlock)
    {
        return _p_param->_result;
	}
	return em_signal_sevent_handle(&(_p_param->_handle));
}

#endif /* ENABLE_HSC */

static DT_NOTIFY_FILE_NAME_CHANGED g_notify_filename_changed_callback_ptr = NULL;

_int32 dt_set_file_name_changed_callback( void * p_param )
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	void *call_ptr= (void * )_p_param->_para1;

	EM_LOG_DEBUG("dt_set_file_name_changed_callback:%x",call_ptr);
	
    g_notify_filename_changed_callback_ptr = (DT_NOTIFY_FILE_NAME_CHANGED)call_ptr;

    return em_signal_sevent_handle(&(_p_param->_handle));
}


_int32 dt_notify_file_name_changed( _u32 task_id, const char* file_name,_u32 length )
{

	EM_LOG_DEBUG("dt_notify_file_name_changed:0x%X,file_name[%u]=%s",task_id,length,file_name);

	if(g_notify_filename_changed_callback_ptr!=NULL)
	{
    		g_notify_filename_changed_callback_ptr(  task_id, file_name, length );
	}
	return SUCCESS;
}


