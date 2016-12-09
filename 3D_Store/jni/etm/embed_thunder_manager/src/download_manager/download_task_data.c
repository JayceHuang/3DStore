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


#include "download_manager/download_task_data.h"
#include "download_manager/download_task_inner.h"
#include "download_manager/download_task_impl.h"
#include "download_manager/download_task_store.h"

#include "utility/utility.h"

#include "asyn_frame/device_handler.h"
#include "scheduler/scheduler.h"
#include "vod_task/vod_task.h"
#include "et_interface/et_interface.h"

#include "em_common/em_mempool.h"
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
static DOWNLOAD_TASK_MANAGER g_dt_mgr;
static RUNNING_TASK g_running_task[MAX_ALOW_TASKS];
static BOOL g_running_task_lock=FALSE;
static BOOL g_have_dead_task=FALSE;
static SLAB *gp_task_info_slab = NULL;		//TASK_INFO
static SLAB *gp_task_slab = NULL;		//TASK
static SLAB *gp_bt_task_slab = NULL;		//BT_TASK
static SLAB *gp_p2sp_task_slab = NULL;		//P2SP_TASK
static SLAB *gp_bt_running_file_slab = NULL;		//BT_RUNNING_FILE

static BOOL g_have_waitting_task=TRUE;
static BOOL g_have_changed_task=TRUE;
static BOOL g_have_waiting_stop_task=FALSE;
static BOOL g_have_task_failed=FALSE;
static BOOL g_need_report_to_remote=FALSE;
#ifdef ENABLE_ETM_MEDIA_CENTER
static _u32 g_update_ticks =0;
#endif /* ENABLE_ETM_MEDIA_CENTER */
#ifdef ENABLE_NULL_PATH
static _u32 g_no_disk_vod_task_num=0;
#endif

#define MAX_PROGRESS_BACK 10*1024*1024L 

/*--------------------------------------------------------------------------*/
/*                Interfaces provid for external program				        */
/*--------------------------------------------------------------------------*/




/*--------------------------------------------------------------------------*/
/*                Internal functions				        */
/*--------------------------------------------------------------------------*/
_int32 dt_init_slabs(void)
{
	_int32 ret_val = SUCCESS;
	EM_LOG_DEBUG( "dt_init_slabs" );

	sd_assert(gp_task_info_slab==NULL);
	if(gp_task_info_slab==NULL)
	{
		ret_val = em_mpool_create_slab(sizeof(TASK_INFO), MIN_TASK_INFO_MEMORY, 0, &gp_task_info_slab);
		if(ret_val!=SUCCESS) goto ErrorHanle;
	}

	sd_assert(gp_task_slab==NULL);
	if(gp_task_slab==NULL)
	{
		ret_val = em_mpool_create_slab(sizeof(EM_TASK), MIN_DL_TASK_MEMORY, 0, &gp_task_slab);
		if(ret_val!=SUCCESS) goto ErrorHanle;
	}

	sd_assert(gp_bt_task_slab==NULL);
	if(gp_bt_task_slab==NULL)
	{
		ret_val = em_mpool_create_slab(sizeof(EM_BT_TASK), MIN_BT_TASK_MEMORY, 0, &gp_bt_task_slab);
		if(ret_val!=SUCCESS) goto ErrorHanle;
	}
	
	sd_assert(gp_p2sp_task_slab==NULL);
	if(gp_p2sp_task_slab==NULL)
	{
		ret_val = em_mpool_create_slab(sizeof(EM_P2SP_TASK), MIN_P2SP_TASK_MEMORY, 0, &gp_p2sp_task_slab);
		if(ret_val!=SUCCESS) goto ErrorHanle;
	}
	
	sd_assert(gp_bt_running_file_slab==NULL);
	if(gp_bt_running_file_slab==NULL)
	{
		ret_val = em_mpool_create_slab(sizeof(BT_RUNNING_FILE), MIN_BT_RUNNING_FILE_MEMORY, 0, &gp_bt_running_file_slab);
		if(ret_val!=SUCCESS) goto ErrorHanle;
	}
	
	return SUCCESS;
	
ErrorHanle:
	dt_uninit_slabs();
	return ret_val;
}
_int32 dt_uninit_slabs(void)
{
	EM_LOG_DEBUG( "dt_uninit_slabs" );

	if(gp_bt_running_file_slab!=NULL)
	{
		em_mpool_destory_slab(gp_bt_running_file_slab);
		gp_bt_running_file_slab=NULL;
	}
	
	if(gp_p2sp_task_slab!=NULL)
	{
		em_mpool_destory_slab(gp_p2sp_task_slab);
		gp_p2sp_task_slab=NULL;
	}
	
	if(gp_bt_task_slab!=NULL)
	{
		em_mpool_destory_slab(gp_bt_task_slab);
		gp_bt_task_slab=NULL;
	}
	
	if(gp_task_slab!=NULL)
	{
		em_mpool_destory_slab(gp_task_slab);
		gp_task_slab=NULL;
	}

	if(gp_task_info_slab!=NULL)
	{
		em_mpool_destory_slab(gp_task_info_slab);
		gp_task_info_slab=NULL;
	}

	
	return SUCCESS;
}

_int32 dt_init(void)
{
	_int32 ret_val = SUCCESS;
	_int32 init_int = 0;
	char* init_pchar = "";
	
	EM_LOG_DEBUG( "dt_init" );

	em_memset(&g_dt_mgr,0, sizeof(DOWNLOAD_TASK_MANAGER));

	em_list_init(&g_dt_mgr._full_task_infos);
	g_dt_mgr._max_cache_num = MAX_CACHE_NUM;
	em_settings_get_int_item( "download_task.max_cache_num",(_int32*)&(g_dt_mgr._max_cache_num));
	g_dt_mgr._max_running_task_num= MAX_RUNNING_TASK_NUM;
	em_settings_get_int_item( "system.max_running_tasks",(_int32*)&(g_dt_mgr._max_running_task_num));

	em_settings_set_int_item("system.download_piece_size", init_int);
	//em_settings_set_str_item("system.ui_version", init_pchar);
	//em_settings_set_int_item("system.ui_product", init_int);
	
	em_settings_set_str_item("system.vod_cache_path", init_pchar);
	em_settings_set_int_item("system.vod_cache_size", init_int);
	em_settings_set_int_item("system.vod_buffer_size", init_int);
	em_settings_set_int_item("system.vod_buffer_time", init_int);
	
	g_dt_mgr._max_vod_cache_size = ETM_DEFAULT_VOD_CACHE_SIZE;  //  1G

	em_list_init(&g_dt_mgr._dling_task_order);

	em_map_init(&g_dt_mgr._all_tasks, dt_id_comp);
	em_map_init(&g_dt_mgr._eigenvalue_url, dt_eigenvalue_comp);
	em_map_init(&g_dt_mgr._eigenvalue_tcid, dt_eigenvalue_comp);
	em_map_init(&g_dt_mgr._eigenvalue_gcid, dt_eigenvalue_comp);
	em_map_init(&g_dt_mgr._eigenvalue_bt, dt_eigenvalue_comp);
	em_map_init(&g_dt_mgr._eigenvalue_file, dt_eigenvalue_comp);
	em_map_init(&g_dt_mgr._file_name_eigenvalue, dt_file_name_eigenvalue_comp);

	em_list_init(&g_dt_mgr._file_path_cache);
	em_list_init(&g_dt_mgr._file_name_cache);
	
	em_memset(&g_running_task,0, MAX_ALOW_TASKS*sizeof(RUNNING_TASK));

	g_running_task_lock=FALSE;
	g_have_dead_task=FALSE;

	g_have_waitting_task=TRUE;
	g_have_changed_task=TRUE;

	
	ret_val = dt_create_task_file();
	if(ret_val!=SUCCESS) goto ErrorHanle;

	ret_val = dt_get_total_task_num();
	if(ret_val!=SUCCESS) goto ErrorHanle;

	return SUCCESS;
	
ErrorHanle:

	dt_uninit();
	CHECK_VALUE(ret_val);
	return ret_val;
}

_int32 dt_uninit(void)
{
	_int32 ret_val = SUCCESS;
	EM_LOG_DEBUG( "dt_uninit" );

	//dt_save_tasks();;
	dt_close_task_file(TRUE);

	sd_assert(em_list_size(&g_dt_mgr._file_name_cache)==0);
	sd_assert(em_list_size(&g_dt_mgr._file_path_cache)==0);
	
	sd_assert(em_list_size(&g_dt_mgr._full_task_infos)==0);
	sd_assert(em_list_size(&g_dt_mgr._dling_task_order)==0);

	sd_assert(em_map_size(&g_dt_mgr._all_tasks)==0);
	sd_assert(em_map_size(&g_dt_mgr._eigenvalue_url)==0);
	sd_assert(em_map_size(&g_dt_mgr._eigenvalue_tcid)==0);
	sd_assert(em_map_size(&g_dt_mgr._eigenvalue_gcid)==0);
	sd_assert(em_map_size(&g_dt_mgr._eigenvalue_bt)==0);
	sd_assert(em_map_size(&g_dt_mgr._eigenvalue_file)==0);
	sd_assert(em_map_size(&g_dt_mgr._file_name_eigenvalue)==0);


	return ret_val;
	
//ErrorHanle:

}


_int32 dt_load_tasks_from_file(void)
{
	_int32 ret_val = SUCCESS,task_count=0;
	EM_TASK *p_task=NULL,*p_task_tmp=NULL,*p_task_tmp2=NULL;
	TASK_INFO * p_task_info = NULL;

      EM_LOG_DEBUG("dt_load_tasks_from_file"); 
	dt_reset_vod_task_num();
	/* read out all the task from the file one by one ,DO NOT STOP! */
	for(p_task_tmp=dt_get_task_from_file();(p_task_tmp!=NULL)&&dt_is_task_locked();p_task_tmp=dt_get_task_from_file())
	{
      		EM_LOG_DEBUG("dt_load_tasks_from_file,get task_id=%u",p_task_tmp->_task_info->_task_id); 
		if(p_task==NULL)
		{
			ret_val = dt_task_malloc(&p_task);
			CHECK_VALUE(ret_val);
			sd_assert(p_task_info==NULL);
			ret_val = dt_task_info_malloc(&p_task_info);
			if(ret_val!=SUCCESS) 
			{
				dt_task_free(p_task);
				CHECK_VALUE(ret_val);
			}
		}
		
		em_memcpy(p_task, p_task_tmp, sizeof(EM_TASK));
		em_memcpy(p_task_info, p_task_tmp->_task_info, sizeof(TASK_INFO));
		p_task->_task_info = p_task_info;
		
		/* add to all_task_map */
		ret_val = dt_add_task_to_map(p_task);
		if(ret_val!=SUCCESS) 
		{
			if(ret_val==MAP_DUPLICATE_KEY)
			{
      				EM_LOG_ERROR("Need delete the old one!"); 
				p_task_tmp2 = dt_get_task_from_map(p_task_info->_task_id);
				sd_assert(p_task_tmp2!=NULL);
				if(p_task_tmp2==NULL) continue;
				dt_remove_task_eigenvalue(p_task_tmp2->_task_info->_type,p_task_tmp2->_task_info->_eigenvalue);
				if(p_task_tmp2->_task_info->_file_name_eigenvalue!=0)
				{
					dt_remove_file_name_eigenvalue(p_task_tmp2->_task_info->_file_name_eigenvalue);
				}
				/* remove from all_tasks_map */
				dt_remove_task_from_map(p_task_tmp2);
				/* disable from task_file */
				dt_disable_task_in_file(p_task_tmp2);
				
				dt_task_info_free(p_task_tmp2->_task_info);
				dt_task_free(p_task_tmp2);
				task_count--;
				
				ret_val = dt_add_task_to_map(p_task);
				sd_assert(ret_val==SUCCESS);
			}
			else
			{
				continue;
			}
		}
		
		/* add to eigenvalue map */
		ret_val = dt_add_task_eigenvalue(p_task_info->_type,p_task_info->_eigenvalue,p_task_info->_task_id);
		if(ret_val!=SUCCESS) 
		{
      			EM_LOG_ERROR("dt_add_task_eigenvalue,FAILED!"); 
			/* remove from all_tasks_map */
			dt_remove_task_from_map(p_task);
			continue;
		}

		if(p_task_info->_file_name_eigenvalue!=0)
		{
			dt_add_file_name_eigenvalue(p_task_info->_file_name_eigenvalue,p_task_info->_task_id);
		}
		p_task=NULL;
		p_task_info=NULL;
		task_count++;
	}

	if(p_task!=NULL)
	{
		sd_assert(p_task_info->_full_info==FALSE);
		dt_task_info_free(p_task_info);
		dt_task_free(p_task);
	}

	if(task_count==0)
	{
		/* no tasks !*/
		dt_reset_task_id();
		dt_reset_vod_task_num();
	}
	
      EM_LOG_DEBUG("dt_load_tasks_from_file:%u,end!",task_count); 
	return SUCCESS;
}
_int32 dt_notify_task_state_changed(EM_TASK * p_task)
{
	EM_TASK_INFO em_task_info;
	char *file_path=NULL,*file_name=NULL;

	EM_LOG_ERROR("dt_notify_task_state_changed:%u,state=%d",p_task->_task_info->_task_id,dt_get_task_state( p_task));

	em_memset(&em_task_info,0x00,sizeof(EM_TASK_INFO));

	if((dt_get_task_state( p_task)==TS_TASK_RUNNING )&& (p_task->_task_info->_task_id==dt_get_current_vod_task_id())&&(p_task->_task_info->_downloaded_data_size>=p_task->_task_info->_file_size))
	{
		/* 已经下载完成的任务进入点播状态,不通知界面 */
		return SUCCESS;
	}

	em_task_info._task_id = p_task->_task_info->_task_id;
	em_task_info._state= dt_get_task_state( p_task);
	em_task_info._type= p_task->_task_info->_type; //dt_get_task_type(p_task);
	
	if(p_task->_task_info->_is_deleted)
	{
		em_task_info._is_deleted= TRUE;
	}
	
	em_task_info._file_size= p_task->_task_info->_file_size;
	em_task_info._downloaded_data_size= p_task->_task_info->_downloaded_data_size;
	em_task_info._start_time= p_task->_task_info->_start_time;
	em_task_info._finished_time= p_task->_task_info->_finished_time;
	em_task_info._failed_code= p_task->_task_info->_failed_code;
	em_task_info._bt_total_file_num= p_task->_task_info->_bt_total_file_num;

	if(p_task->_task_info->_check_data)//_check_data 用于_is_no_disk -----zyq @20101027 fro vod task
	{
		//em_task_info._check_data= TRUE;
		em_task_info._is_no_disk= TRUE;
	}
	

	file_path = dt_get_task_file_path(p_task);
	file_name = dt_get_task_file_name(p_task);

	if(file_path!=NULL)
	{
		em_memcpy(em_task_info._file_path, file_path, p_task->_task_info->_file_path_len);
		if(em_task_info._file_path[p_task->_task_info->_file_path_len-1]!=DIR_SPLIT_CHAR)
		{	// 和谐路径
			sd_strcat(em_task_info._file_path, DIR_SPLIT_STRING, 1);
		}
	}
	else
	{
		sd_assert(FALSE);
		return INVALID_FILE_PATH;
	}
	
	if(file_name!=NULL)
	{
		em_memcpy(em_task_info._file_name, file_name, p_task->_task_info->_file_name_len);
	}
	else
	if(p_task->_task_info->_have_name)
	{
		sd_assert(FALSE);
		return INVALID_FILE_NAME;
	}
	em_notify_task_state_changed(p_task->_task_info->_task_id, &em_task_info);
    return SUCCESS;
}

_int32 dt_save_tasks(void)
{
    MAP_ITERATOR  cur_node = NULL; 
	EM_TASK * p_task = NULL;
	BOOL is_state_changed=FALSE;
	_int32 ret_val = SUCCESS;

	EM_LOG_DEBUG("dt_save_tasks"); 

	if(g_have_changed_task == FALSE)
		return SUCCESS;
	  
	ret_val = dt_check_task_file(em_map_size(&g_dt_mgr._all_tasks));
	if(ret_val != SUCCESS)	
	{
	#if 1
	  	/* 文件已被破坏,不能在运行过程中修复,因为dt_recover_file后任务的偏移地址有可能与内存中的不一致,程序必须重启! */
	  	em_set_critical_error(WRITE_TASK_FILE_ERR);
		CHECK_VALUE(WRITE_TASK_FILE_ERR);
	#else
	  	if(FALSE==dt_recover_file())
	  	{
	      		EM_LOG_ERROR("dt_save_tasks:ERROR:dt_recover_file FAILED!"); 
			CHECK_VALUE(INVALID_TASK);
	  	}
	#endif
	}
	  
	dt_backup_file();
	  
	cur_node = EM_MAP_BEGIN(g_dt_mgr._all_tasks);
	while(cur_node != EM_MAP_END(g_dt_mgr._all_tasks))
	{
		is_state_changed=FALSE;
		p_task = (EM_TASK *)EM_MAP_VALUE(cur_node);
		if(p_task->_change_flag!=0)
		{	
			if(p_task->_change_flag & CHANGE_STATE) 
				is_state_changed = TRUE;
				
			dt_save_task_to_file(p_task);

			if(is_state_changed)
			{
				EM_LOG_ERROR("dt_save_tasks:CHANGE_STATE:%u!",dt_get_task_state(p_task)); 
			}

			// 1.  需要通知界面或任务成功时 并且有状态改变 
			if( (dt_is_need_notify_state_changed()||dt_get_task_state(p_task)==TS_TASK_SUCCESS) && is_state_changed )
			{
				// 2. 任务类型不是TT_FILE时通知界面
				if( TT_FILE != dt_get_task_type(p_task) )
				{
					/* notify UI */
					dt_notify_task_state_changed(p_task);
				}
			}
				
		}
		cur_node = EM_MAP_NEXT(g_dt_mgr._all_tasks, cur_node);
	}
	  
	g_have_changed_task=FALSE;
	return SUCCESS;
}
_int32 dt_have_task_changed(void)
{
	if(!g_have_changed_task)
		g_have_changed_task=TRUE;
	
	return SUCCESS;
}

_int32 dt_have_task_waiting_stop(void)
{
	if(!g_have_waiting_stop_task)
		g_have_waiting_stop_task=TRUE;
	
	return SUCCESS;
}
_int32 dt_have_task_failed(void)
{
#ifdef ENABLE_ETM_MEDIA_CENTER
	g_have_task_failed = TRUE;
#else
	BOOL failed_auto_restart = FALSE;
	if(!g_have_task_failed)
	{
		em_settings_get_bool_item( "ui.failed_auto_restart",&failed_auto_restart);
		if(failed_auto_restart)
		{
			g_have_task_failed=TRUE;
		}
	}
#endif
	return SUCCESS;
}

_int32 dt_load_order_list(void)
{
	_int32 ret_val = SUCCESS;
	_u32 i=0,order_list_size=dt_get_order_list_size_from_file();
	_u32 * task_id_array=NULL;
	EM_TASK * p_task=NULL;
	MAP_ITERATOR cur_node;
	_u32 act_order_list_size=0;

      EM_LOG_DEBUG("dt_load_order_list:%u",order_list_size); 
	if(order_list_size!=0)
	{
		if(order_list_size>MAX_TASK_NUM) return INVALID_ORDER_LIST_SIZE;
		ret_val=em_malloc(order_list_size*sizeof(_u32), (void**)&task_id_array);
		CHECK_VALUE(ret_val);

		em_memset(task_id_array, 0, order_list_size*sizeof(_u32));
		ret_val = dt_get_order_list_from_file(task_id_array,order_list_size*sizeof(_u32));
		if(ret_val==SUCCESS)
		{
			for(i=0;i<order_list_size;i++)
			{
				p_task = dt_get_task_from_map(task_id_array[i]);
				if(p_task!=NULL)
				{
					if((TS_TASK_SUCCESS!=dt_get_task_state(p_task))&&(TS_TASK_DELETED!=dt_get_task_state(p_task)))
					{
						dt_add_task_to_order_list(p_task);
					}
					else
					{
						EM_LOG_ERROR("dt_load_order_list WARNNING:%u wrong task state:%d",task_id_array[i],dt_get_task_state(p_task)); 
					}
				}
				else
				{
					EM_LOG_ERROR("dt_load_order_list WARNNING:this task is not in map:%u",task_id_array[i]); 
					dt_clear_order_list();
					EM_SAFE_DELETE(task_id_array);
					return INVALID_TASK_ID;
				}
			}
		}
	}

	// check the task map, fix consistency
	cur_node = MAP_BEGIN( g_dt_mgr._all_tasks );

	while( cur_node != MAP_END( g_dt_mgr._all_tasks ) )
	{
		p_task = (EM_TASK *)MAP_VALUE( cur_node );
		if (p_task != NULL)
		{
			if ((TS_TASK_SUCCESS!=dt_get_task_state(p_task))&&(TS_TASK_DELETED!=dt_get_task_state(p_task)))
			{
				act_order_list_size ++;
			}
		}
		cur_node = MAP_NEXT( g_dt_mgr._all_tasks, cur_node );
	}

	if (act_order_list_size != em_list_size(&g_dt_mgr._dling_task_order))
	{
		EM_LOG_ERROR("dt_load_order_list WARNNING:act_order_list_size(%u) != _dling_task_order_size(%u)", act_order_list_size, em_list_size(&g_dt_mgr._dling_task_order)); 

		// fix consistency
		cur_node = MAP_BEGIN( g_dt_mgr._all_tasks );
		while( cur_node != MAP_END( g_dt_mgr._all_tasks ) )
		{
			p_task = (EM_TASK *)MAP_VALUE( cur_node );
			if (p_task == NULL)
			{
				EM_LOG_ERROR("dt_load_order_list WARNNING: meet a NULL node");
				cur_node = MAP_NEXT( g_dt_mgr._all_tasks, cur_node );
				continue;
			}

			for(i=0;i<order_list_size;i++)
			{
				if (task_id_array[i] == p_task->_task_info->_task_id)
				{
					break;
				}
			}

			if (i == order_list_size)
			{
				EM_LOG_DEBUG("dt_load_order_list DEBUG: task id %u not in order list, add it");
				dt_add_task_to_order_list(p_task);
			}
	
			cur_node = MAP_NEXT( g_dt_mgr._all_tasks, cur_node );
		}
	}

	if (order_list_size != 0)	
	{
		EM_SAFE_DELETE(task_id_array);
	}
	
	return SUCCESS;
}
_int32 dt_save_order_list(void)
{
	_int32 ret_val = SUCCESS;
	_u32 order_list_size =0 ,i=0;
	pLIST_NODE cur_node=NULL ;
	_u32 * task_id_array=NULL;
	
	if(g_dt_mgr._dling_order_changed)
	{
      		EM_LOG_DEBUG("dt_save_order_list"); 
		// Save to file ...
		order_list_size = em_list_size(&g_dt_mgr._dling_task_order);
		if(order_list_size==0) 
		{
			dt_save_order_list_to_file(order_list_size,NULL);
			g_dt_mgr._dling_order_changed = FALSE;
			return SUCCESS;
		}

		ret_val=em_malloc(order_list_size*sizeof(_u32), (void**)&task_id_array);
		CHECK_VALUE(ret_val);

		em_memset(task_id_array, 0, order_list_size*sizeof(_u32));
	
		cur_node = LIST_BEGIN(g_dt_mgr._dling_task_order);
		while(cur_node!=LIST_END(g_dt_mgr._dling_task_order))
		{
			task_id_array[i++]=((EM_TASK * )LIST_VALUE(cur_node))->_task_info->_task_id;
			cur_node = LIST_NEXT(cur_node);
		}
		sd_assert(order_list_size==i);
		dt_save_order_list_to_file(order_list_size,task_id_array);

		EM_SAFE_DELETE(task_id_array);
		g_dt_mgr._dling_order_changed = FALSE;
	}
	return SUCCESS;
	
}

_int32 dt_get_total_task_num(void)
{
      EM_LOG_DEBUG("dt_get_total_task_num"); 
	return dt_get_total_task_num_from_file(&g_dt_mgr._total_task_num);
}
_int32 dt_save_total_task_num(void)
{
      EM_LOG_DEBUG("dt_save_total_task_num :%u",g_dt_mgr._total_task_num); 
	return dt_save_total_task_num_to_file(g_dt_mgr._total_task_num);
}
_int32 dt_set_need_notify_state_changed(BOOL is_need)
{
      EM_LOG_DEBUG("dt_set_need_notify_state_changed :%d",is_need); 
	g_dt_mgr._need_notify_state_changed=is_need;
	return SUCCESS;
}

BOOL dt_is_need_notify_state_changed(void)
{
	return g_dt_mgr._need_notify_state_changed;
}

BOOL dt_is_running_tasks_loaded(void)
{
	return g_dt_mgr._running_task_loaded;
}

_int32 dt_load_running_tasks(void)
{
	//_int32 ret_val = SUCCESS;
	EM_TASK * p_task=NULL;
	_u32 i=0,*running_task_ids =NULL;
      EM_LOG_DEBUG("dt_load_running_tasks");

	  sd_assert(g_dt_mgr._running_task_loaded ==FALSE);

	  running_task_ids =dt_get_running_tasks_from_file();
	if(running_task_ids!=NULL)
	{
		for(i=0;i<MAX_ALOW_TASKS;i++)
		{
			if(running_task_ids[i]!=0)
			{
				p_task = dt_get_task_from_map(running_task_ids[i]);
				if(p_task!=NULL)
				{
					dt_start_task_impl(p_task);
				}
				else
				{
					EM_LOG_DEBUG("dt_load_running_tasks WARNNING:this task is not in map:%u",running_task_ids[i]); 
				}
			}
		}
	}
	g_dt_mgr._running_task_loaded = TRUE;
	return SUCCESS;
}
_int32 dt_save_running_tasks(BOOL clear)
{
	_int32 i=0;
	RUNNING_TASK running_task_tmp[MAX_ALOW_TASKS];
	_u32 running_task_ids[MAX_ALOW_TASKS];
	
	if(!et_check_running()) return SUCCESS;

	em_memset(running_task_ids,0,sizeof(running_task_ids));

	if(!g_dt_mgr._running_task_changed)
	{
		if(clear)
		{
			g_dt_mgr._running_task_loaded = FALSE;
		}
		return SUCCESS;
	}

	while(g_running_task_lock)
	{
		em_sleep(1);
		if(i++>=2)
		{
			CHECK_VALUE(RUNNING_TASK_LOCK_CRASH);
		}
	}
	
	g_running_task_lock=TRUE;

	em_memcpy(&running_task_tmp, &g_running_task, MAX_ALOW_TASKS*sizeof(RUNNING_TASK));

	g_running_task_lock=FALSE;
	
/////////////////////////////////////////////////////////////////////////////////////////////	
      EM_LOG_DEBUG("dt_save_running_tasks"); 

	for(i=0;i<MAX_ALOW_TASKS;i++)
	{
		if(running_task_tmp[i]._task_id<MAX_DL_TASK_ID)
			running_task_ids[i]=running_task_tmp[i]._task_id;
	}
	dt_save_running_tasks_to_file(running_task_ids);
	g_dt_mgr._running_task_changed=FALSE;
	if(clear)
	{
		g_dt_mgr._running_task_loaded = FALSE;
	}
	return SUCCESS;
}

_int32 dt_stop_tasks(void)
{
	_int32 i=0;
	RUNNING_TASK running_task_tmp[MAX_ALOW_TASKS];
	EM_TASK * p_task=NULL;

	while(g_running_task_lock)
	{
		em_sleep(1);
		if(i++>=2)
		{
			CHECK_VALUE(RUNNING_TASK_LOCK_CRASH);
		}
	}
	
	g_running_task_lock=TRUE;

	em_memcpy(&running_task_tmp, &g_running_task, MAX_ALOW_TASKS*sizeof(RUNNING_TASK));

	g_running_task_lock=FALSE;
	
      		EM_LOG_DEBUG("dt_stop_tasks"); 
/////////////////////////////////////////////////////////////////////////////////////////////	

	for(i=0;i<MAX_ALOW_TASKS;i++)
	{
		if(running_task_tmp[i]._task_id!=0)
		{
			p_task = dt_get_task_from_map(running_task_tmp[i]._task_id);
			sd_assert(p_task!=NULL);
			if(p_task!=NULL)
			{
				dt_stop_task_impl(p_task);
				/* 如果是无盘点播任务,直接销毁 */
				if(dt_is_vod_task(p_task)&&dt_is_vod_task_no_disk(p_task))
				{
					dt_destroy_vod_task(p_task);
				}
			}
			
		}
	}
	return SUCCESS;
	
}

_int32 dt_task_info_malloc(TASK_INFO **pp_slip)
{
	_int32 ret_val = em_mpool_get_slip( gp_task_info_slab, (void**)pp_slip );
	
	if(ret_val==SUCCESS) 
		em_memset(*pp_slip,0,sizeof(TASK_INFO));
	
      EM_LOG_DEBUG("dt_task_info_malloc"); 
	return ret_val;
}

_int32 dt_task_info_free(TASK_INFO * p_slip)
{
	sd_assert( p_slip != NULL );
	if( p_slip == NULL ) return SUCCESS;
      EM_LOG_DEBUG("dt_task_info_free"); 
	return em_mpool_free_slip( gp_task_info_slab, (void*)p_slip );
}

_int32 dt_task_malloc(EM_TASK **pp_slip)
{
	_int32 ret_val = em_mpool_get_slip( gp_task_slab, (void**)pp_slip );
	
	if(ret_val==SUCCESS) 
		em_memset(*pp_slip,0,sizeof(EM_TASK));
	
      EM_LOG_DEBUG("dt_task_malloc"); 
	return ret_val;
}

_int32 dt_task_free(EM_TASK * p_slip)
{
	sd_assert( p_slip != NULL );
	if( p_slip == NULL ) return SUCCESS;
      //EM_LOG_DEBUG("dt_task_free,task_id=%u",p_slip->_task_info->_task_id); 
	return em_mpool_free_slip( gp_task_slab, (void*)p_slip );
}


_int32 dt_bt_task_malloc(EM_BT_TASK **pp_slip)
{
	_int32 ret_val = em_mpool_get_slip( gp_bt_task_slab, (void**)pp_slip );
	
	if(ret_val==SUCCESS) 
		em_memset(*pp_slip,0,sizeof(EM_BT_TASK));

	/* add to task_info cache */
	ret_val = dt_add_task_info_to_cache((TASK_INFO *)*pp_slip);
	CHECK_VALUE(ret_val);

      EM_LOG_DEBUG("dt_bt_task_malloc"); 
	return ret_val;
}

_int32 dt_bt_task_free(EM_BT_TASK * p_slip)
{
	sd_assert( p_slip != NULL );
	if( p_slip == NULL ) return SUCCESS;
      EM_LOG_DEBUG("dt_bt_task_free"); 
	return em_mpool_free_slip( gp_bt_task_slab, (void*)p_slip );
}

_int32 dt_p2sp_task_malloc(EM_P2SP_TASK **pp_slip)
{
	_int32 ret_val = em_mpool_get_slip( gp_p2sp_task_slab, (void**)pp_slip );
	
	if(ret_val==SUCCESS) 
		em_memset(*pp_slip,0,sizeof(EM_P2SP_TASK));
	
	/* add to task_info cache */
	ret_val = dt_add_task_info_to_cache((TASK_INFO *)*pp_slip);
	CHECK_VALUE(ret_val);
      EM_LOG_DEBUG("dt_p2sp_task_malloc"); 
	return ret_val;
}

_int32 dt_p2sp_task_free(EM_P2SP_TASK * p_slip)
{
	sd_assert( p_slip != NULL );
	if( p_slip == NULL ) return SUCCESS;
      EM_LOG_DEBUG("dt_p2sp_task_free"); 
	return em_mpool_free_slip( gp_p2sp_task_slab, (void*)p_slip );
}
_int32 dt_bt_running_file_malloc(BT_RUNNING_FILE **pp_slip)
{
	_int32 ret_val = em_mpool_get_slip( gp_bt_running_file_slab, (void**)pp_slip );
	
	if(ret_val==SUCCESS) 
		em_memset(*pp_slip,0,sizeof(BT_RUNNING_FILE));
	
      EM_LOG_DEBUG("dt_bt_running_file_malloc"); 
	return ret_val;
}

_int32 dt_bt_running_file_free(BT_RUNNING_FILE * p_slip)
{
	sd_assert( p_slip != NULL );
	if( p_slip == NULL ) return SUCCESS;
      EM_LOG_DEBUG("dt_bt_running_file_free"); 
	return em_mpool_free_slip( gp_bt_running_file_slab, (void*)p_slip );
}
_int32 dt_bt_running_file_safe_delete(EM_TASK * p_task)
{
	if(p_task->_bt_running_files!=NULL)
	{
      		EM_LOG_DEBUG("dt_bt_running_file_safe_delete:task_id=%u",p_task->_task_info->_task_id); 
		EM_SAFE_DELETE(p_task->_bt_running_files->_need_dl_file_index_array);
		sd_assert(p_task->_bt_running_files->_need_dl_file_num>=p_task->_bt_running_files->_finished_file_num);
		sd_assert(p_task->_task_info!=NULL);
		if((p_task->_task_info->_state==TS_TASK_SUCCESS)||(p_task->_task_info->_state==TS_TASK_FAILED))
		{
			//sd_assert(p_task->_bt_running_files->_need_dl_file_num==p_task->_bt_running_files->_finished_file_num);
		}
		dt_bt_running_file_free(p_task->_bt_running_files);
		p_task->_bt_running_files = NULL;
	}
	return SUCCESS;
}

////////////////////////////////////////////////////////////////////
_int32 dt_reset_task_id(void)
{
      EM_LOG_DEBUG("dt_reset_task_id"); 

	g_dt_mgr._total_task_num=0;

	dt_save_total_task_num();
	
	return SUCCESS;
}

_u32 dt_get_max_task_id(void)
{
      MAP_ITERATOR  cur_node = NULL; 
	 _u32 max_task_id = 0,task_id;

      EM_LOG_DEBUG("dt_get_max_task_id"); 

      cur_node = EM_MAP_BEGIN(g_dt_mgr._all_tasks);
      while(cur_node != EM_MAP_END(g_dt_mgr._all_tasks))
      {
             task_id = (_u32)EM_MAP_KEY(cur_node);
		task_id&=0x3FFFFFFF;
		if(max_task_id<task_id)
		{
			max_task_id = task_id;
		}

	      cur_node = EM_MAP_NEXT(g_dt_mgr._all_tasks, cur_node);
      }
	  
	return max_task_id;
}

_u32 dt_create_task_id(BOOL is_remote)
{
	_u32 task_id = 0;
      EM_LOG_DEBUG("dt_create_task_id"); 

	  task_id = dt_get_max_task_id();
	  if((task_id!=MAX_LOCAL_DL_TASK_ID-1)&&g_dt_mgr._total_task_num<task_id)
	  {
	  	/* 确保_total_task_num 不小于最大任务id */
	  	g_dt_mgr._total_task_num = task_id;
	  }

	g_dt_mgr._total_task_num++;

	if(g_dt_mgr._total_task_num>=MAX_LOCAL_DL_TASK_ID)
	{
		/* Oh no .... */
		sd_assert(FALSE);
		g_dt_mgr._total_task_num=1;
	}

	dt_save_total_task_num();
	
	task_id = g_dt_mgr._total_task_num;
	if(is_remote)
	{
		/* 远程控制创建的任务id总大于MAX_LOCAL_DL_TASK_ID */
		task_id += MAX_LOCAL_DL_TASK_ID;
	}

	return task_id;
}

_int32 dt_decrease_task_id(void)
{
      EM_LOG_DEBUG("dt_decrease_task_id"); 

	if(g_dt_mgr._total_task_num==0)
	{
		sd_assert(FALSE);
		return INVALID_TASK_ID;
	}
	
	g_dt_mgr._total_task_num--;

	dt_save_total_task_num();
	
	return SUCCESS;
}
BOOL dt_is_local_task(EM_TASK * p_task)
{
	if(p_task->_task_info->_task_id>MAX_LOCAL_DL_TASK_ID)
		return FALSE;
	return TRUE;
}

EM_TASK * dt_get_task_from_map(_u32 task_id)
{
	_int32 ret_val = SUCCESS;
	EM_TASK * p_task=NULL;

      EM_LOG_DEBUG("dt_get_task_from_map:task_id=%u",task_id); 
	ret_val=em_map_find_node(&g_dt_mgr._all_tasks, (void *)task_id, (void **)&p_task);
	sd_assert(ret_val==SUCCESS);
	//sd_assert(p_task!=NULL);
	return p_task;
}
_int32 dt_add_task_to_map(EM_TASK * p_task)
{
	_int32 ret_val = SUCCESS;
	PAIR info_map_node;
      EM_LOG_DEBUG("dt_add_task_to_map:task_id=%u",p_task->_task_info->_task_id); 
	info_map_node._key =(void*) p_task->_task_info->_task_id;
	info_map_node._value = (void*)p_task;
	ret_val = em_map_insert_node( &g_dt_mgr._all_tasks, &info_map_node );
	//CHECK_VALUE(ret_val);
	if((ret_val==SUCCESS)&&(dt_is_vod_task( p_task)))
	{
		dt_increase_vod_task_num(p_task);
	}
	return ret_val;
}

_int32 dt_remove_task_from_map(EM_TASK * p_task)
{
	_int32 ret_val = SUCCESS;
      EM_LOG_DEBUG("dt_remove_task_from_map:task_id=%u",p_task->_task_info->_task_id); 
	ret_val = em_map_erase_node(&g_dt_mgr._all_tasks, (void*)p_task->_task_info->_task_id);
	if((ret_val==SUCCESS)&&(dt_is_vod_task( p_task)))
	{
		dt_decrease_vod_task_num(p_task);
	}
	return ret_val;
}
_int32 dt_clear_task_map(void)
{
      MAP_ITERATOR  cur_node = NULL; 
	 EM_TASK * p_task = NULL;

      EM_LOG_DEBUG("dt_clear_task_map:%u",em_map_size(&g_dt_mgr._all_tasks)); 
	  
      cur_node = EM_MAP_BEGIN(g_dt_mgr._all_tasks);
      while(cur_node != EM_MAP_END(g_dt_mgr._all_tasks))
      {
             p_task = (EM_TASK *)EM_MAP_VALUE(cur_node);
		dt_uninit_task_info(p_task->_task_info);
		//sd_assert(p_task->_change_flag==0);
		sd_assert(p_task->_bt_running_files==NULL);
	      dt_task_free(p_task);
		em_map_erase_iterator(&g_dt_mgr._all_tasks, cur_node);
	      cur_node = EM_MAP_BEGIN(g_dt_mgr._all_tasks);
      }
	  
	 dt_reset_vod_task_num();
	 
	return SUCCESS;
}

_u32 dt_get_task_num_in_map(void)
{
	return em_map_size(&g_dt_mgr._all_tasks);
}

BOOL dt_is_url_task_exist(_u8 * eigenvalue,_u32 * task_id)
{
	_int32 ret_val = SUCCESS;
	MAP_ITERATOR cur_node = NULL;

      EM_LOG_DEBUG("dt_is_url_task_exist:%u",em_map_size(&g_dt_mgr._eigenvalue_url)); 
	ret_val = em_map_find_iterator( &g_dt_mgr._eigenvalue_url, (void *)eigenvalue, &cur_node );
	sd_assert( ret_val == SUCCESS );

	if( cur_node == EM_MAP_END( g_dt_mgr._eigenvalue_url ) )
	{
		return FALSE;
	}

	if(task_id)
	{
		*task_id = (_u32)MAP_VALUE(cur_node);
	}

	return TRUE;
}
BOOL dt_is_tcid_task_exist(_u8 * eigenvalue,_u32 * task_id)
{
	_int32 ret_val = SUCCESS;
	MAP_ITERATOR cur_node = NULL;

      EM_LOG_DEBUG("dt_is_tcid_task_exist"); 
	ret_val = em_map_find_iterator( &g_dt_mgr._eigenvalue_tcid, (void *)eigenvalue, &cur_node );
	sd_assert( ret_val == SUCCESS );

	if( cur_node == EM_MAP_END( g_dt_mgr._eigenvalue_tcid ) )
	{
		return FALSE;
	}

	if(task_id)
	{
		*task_id = (_u32)MAP_VALUE(cur_node);
	}

	return TRUE;
}
BOOL dt_is_kankan_task_exist(_u8 * eigenvalue,_u32 * task_id)
{
	_int32 ret_val = SUCCESS;
	MAP_ITERATOR cur_node = NULL;

      EM_LOG_DEBUG("dt_is_kankan_task_exist"); 
	ret_val = em_map_find_iterator( &g_dt_mgr._eigenvalue_gcid, (void *)eigenvalue, &cur_node );
	sd_assert( ret_val == SUCCESS );

	if( cur_node == EM_MAP_END( g_dt_mgr._eigenvalue_gcid ) )
	{
		return FALSE;
	}

	if(task_id)
	{
		*task_id = (_u32)MAP_VALUE(cur_node);
	}

	return TRUE;
}
BOOL dt_is_bt_task_exist(_u8 * eigenvalue,_u32 * task_id)
{
	_int32 ret_val = SUCCESS;
	MAP_ITERATOR cur_node = NULL;

      EM_LOG_DEBUG("dt_is_bt_task_exist"); 
	ret_val = em_map_find_iterator( &g_dt_mgr._eigenvalue_bt, (void *)eigenvalue, &cur_node );
	sd_assert( ret_val == SUCCESS );

	if( cur_node == EM_MAP_END( g_dt_mgr._eigenvalue_bt ) )
	{
		return FALSE;
	}

	if(task_id)
	{
		*task_id = (_u32)MAP_VALUE(cur_node);
	}

	return TRUE;
}
BOOL dt_is_file_task_exist(_u8 * eigenvalue,_u32 * task_id)
{
	_int32 ret_val = SUCCESS;
	MAP_ITERATOR cur_node = NULL;

      EM_LOG_DEBUG("dt_is_file_task_exist"); 
	ret_val = em_map_find_iterator( &g_dt_mgr._eigenvalue_file, (void *)eigenvalue, &cur_node );
	sd_assert( ret_val == SUCCESS );

	if( cur_node == EM_MAP_END( g_dt_mgr._eigenvalue_file ) )
	{
		return FALSE;
	}

	if(task_id)
	{
		*task_id = (_u32)MAP_VALUE(cur_node);
	}

	return TRUE;
}

_int32 dt_add_bt_task_eigenvalue(_u8 * eigenvalue,_u32 task_id)
{
	_int32 ret_val = SUCCESS;
	PAIR info_map_node;
      EM_LOG_DEBUG("dt_add_bt_task_eigenvalue"); 
	info_map_node._key = (void*)eigenvalue;
	info_map_node._value = (void*)task_id;
	ret_val = em_map_insert_node( &g_dt_mgr._eigenvalue_bt, &info_map_node );
	CHECK_VALUE(ret_val);
	return SUCCESS;
}
_int32 dt_add_url_task_eigenvalue(_u8 * eigenvalue,_u32 task_id)
{
	_int32 ret_val = SUCCESS;
	PAIR info_map_node;
      EM_LOG_DEBUG("dt_add_url_task_eigenvalue"); 
	info_map_node._key = (void*)eigenvalue;
	info_map_node._value = (void*)task_id;
	ret_val = em_map_insert_node( &g_dt_mgr._eigenvalue_url, &info_map_node );
	CHECK_VALUE(ret_val);
	return SUCCESS;
}

_int32 dt_add_tcid_task_eigenvalue(_u8 * eigenvalue,_u32 task_id)
{
	_int32 ret_val = SUCCESS;
	PAIR info_map_node;
      EM_LOG_DEBUG("dt_add_tcid_task_eigenvalue"); 
	info_map_node._key = (void*)eigenvalue;
	info_map_node._value = (void*)task_id;
	ret_val = em_map_insert_node( &g_dt_mgr._eigenvalue_tcid, &info_map_node );
	CHECK_VALUE(ret_val);
	return SUCCESS;
}

_int32 dt_add_kankan_task_eigenvalue(_u8 * eigenvalue,_u32 task_id)
{
	_int32 ret_val = SUCCESS;
	PAIR info_map_node;
      EM_LOG_DEBUG("dt_add_kankan_task_eigenvalue"); 
	info_map_node._key = (void*)eigenvalue;
	info_map_node._value = (void*)task_id;
	ret_val = em_map_insert_node( &g_dt_mgr._eigenvalue_gcid, &info_map_node );
	CHECK_VALUE(ret_val);
	return SUCCESS;
}
_int32 dt_add_file_task_eigenvalue(_u8 * eigenvalue,_u32 task_id)
{
	_int32 ret_val = SUCCESS;
	PAIR info_map_node;
      EM_LOG_DEBUG("dt_add_file_task_eigenvalue"); 
	info_map_node._key = (void*)eigenvalue;
	info_map_node._value = (void*)task_id;
	ret_val = em_map_insert_node( &g_dt_mgr._eigenvalue_file, &info_map_node );
	CHECK_VALUE(ret_val);
	return SUCCESS;
}


_int32 dt_remove_bt_task_eigenvalue(_u8 * eigenvalue)
{
	_int32 ret_val = SUCCESS;
      EM_LOG_DEBUG("dt_remove_bt_task_eigenvalue"); 
	ret_val = em_map_erase_node(&g_dt_mgr._eigenvalue_bt, (void*)eigenvalue );
	CHECK_VALUE(ret_val);
	return SUCCESS;
}
_int32 dt_remove_url_task_eigenvalue(_u8 * eigenvalue)
{
	_int32 ret_val = SUCCESS;
      EM_LOG_DEBUG("dt_remove_url_task_eigenvalue"); 
	ret_val = em_map_erase_node(&g_dt_mgr._eigenvalue_url, (void*)eigenvalue );
	CHECK_VALUE(ret_val);
	return SUCCESS;
}
_int32 dt_remove_tcid_task_eigenvalue(_u8 * eigenvalue)
{
	_int32 ret_val = SUCCESS;
      EM_LOG_DEBUG("dt_remove_tcid_task_eigenvalue"); 
	ret_val = em_map_erase_node(&g_dt_mgr._eigenvalue_tcid, (void*)eigenvalue );
	CHECK_VALUE(ret_val);
	return SUCCESS;
}
_int32 dt_remove_kankan_task_eigenvalue(_u8 * eigenvalue)
{
	_int32 ret_val = SUCCESS;
      EM_LOG_DEBUG("dt_remove_kankan_task_eigenvalue"); 
	ret_val = em_map_erase_node(&g_dt_mgr._eigenvalue_gcid, (void*)eigenvalue );
	CHECK_VALUE(ret_val);
	return SUCCESS;
}
_int32 dt_remove_file_task_eigenvalue(_u8 * eigenvalue)
{
	_int32 ret_val = SUCCESS;
      EM_LOG_DEBUG("dt_remove_file_task_eigenvalue"); 
	ret_val = em_map_erase_node(&g_dt_mgr._eigenvalue_file, (void*)eigenvalue );
	CHECK_VALUE(ret_val);
	return SUCCESS;
}

BOOL dt_is_file_exist(_u32  eigenvalue)
{
	_int32 ret_val = SUCCESS;
	MAP_ITERATOR cur_node = NULL;

      EM_LOG_DEBUG("dt_is_file_exist"); 
	ret_val = em_map_find_iterator( &g_dt_mgr._file_name_eigenvalue, (void *)eigenvalue, &cur_node );
	sd_assert( ret_val == SUCCESS );

	if( cur_node == EM_MAP_END( g_dt_mgr._file_name_eigenvalue ) )
	{
		return FALSE;
	}

	return TRUE;
}
_int32 dt_add_file_name_eigenvalue(_u32  eigenvalue,_u32 task_id)
{
	_int32 ret_val = SUCCESS;
	PAIR info_map_node;
      EM_LOG_DEBUG("dt_add_file_name_eigenvalue, eigenvalue = %u, task_id = %u, map_size = %d", eigenvalue, task_id, em_map_size(&g_dt_mgr._file_name_eigenvalue)); 
	info_map_node._key = (void*)eigenvalue;
	info_map_node._value = (void*)task_id;
	ret_val = em_map_insert_node( &g_dt_mgr._file_name_eigenvalue, &info_map_node );
	CHECK_VALUE(ret_val);
	return SUCCESS;
}
_int32 dt_remove_file_name_eigenvalue(_u32  eigenvalue)
{
	_int32 ret_val = SUCCESS;
      EM_LOG_DEBUG("dt_remove_file_name_eigenvalue"); 
	ret_val = em_map_erase_node(&g_dt_mgr._file_name_eigenvalue, (void*)eigenvalue );
	//CHECK_VALUE(ret_val);
	return ret_val;
}


_int32 dt_clear_eigenvalue(void)
{
      	EM_LOG_DEBUG("dt_clear_eigenvalue"); 
	em_map_clear(&g_dt_mgr._eigenvalue_bt);
	em_map_clear(&g_dt_mgr._eigenvalue_url);
	em_map_clear(&g_dt_mgr._eigenvalue_tcid);
	em_map_clear(&g_dt_mgr._eigenvalue_gcid);
	em_map_clear(&g_dt_mgr._eigenvalue_file);
	em_map_clear(&g_dt_mgr._file_name_eigenvalue);
	return SUCCESS;
}



_int32 dt_add_task_to_order_list(EM_TASK * p_task)
{
	_int32 ret_val = SUCCESS;
      EM_LOG_DEBUG("dt_add_task_to_order_list:task_id=%u",p_task->_task_info->_task_id); 
	  
	if(em_list_size(&g_dt_mgr._dling_task_order)>= MAX_TASK_NUM)
	{
		ret_val = dt_remove_oldest_task_from_order_list();
		CHECK_VALUE(ret_val);
	}
	
	ret_val = em_list_push(&g_dt_mgr._dling_task_order,(void*)p_task);
	if(ret_val==SUCCESS)
		g_dt_mgr._dling_order_changed = TRUE;

	return ret_val;
}
_int32 dt_remove_oldest_task_from_order_list(void)
{
	EM_TASK * p_task = NULL;
	pLIST_NODE cur_node=NULL ;
	
      EM_LOG_DEBUG("dt_remove_oldest_task_from_order_list"); 
	  
	cur_node = LIST_BEGIN(g_dt_mgr._dling_task_order);
	while(cur_node!=LIST_END(g_dt_mgr._dling_task_order))
	{
		p_task=(EM_TASK * )LIST_VALUE(cur_node);
		if(p_task->_task_info->_state==TS_TASK_FAILED)
		{
			dt_set_task_state(p_task,TS_TASK_DELETED);
			em_list_erase(&g_dt_mgr._dling_task_order, cur_node);
			g_dt_mgr._dling_order_changed = TRUE;
			return SUCCESS;
		}

		cur_node = LIST_NEXT(cur_node);
	}
	
	cur_node = LIST_BEGIN(g_dt_mgr._dling_task_order);
	while(cur_node!=LIST_END(g_dt_mgr._dling_task_order))
	{
		p_task=(EM_TASK * )LIST_VALUE(cur_node);
		if(p_task->_task_info->_state==TS_TASK_PAUSED)
		{
			dt_set_task_state(p_task,TS_TASK_DELETED);
			em_list_erase(&g_dt_mgr._dling_task_order, cur_node);
			g_dt_mgr._dling_order_changed = TRUE;
			return SUCCESS;
		}

		cur_node = LIST_NEXT(cur_node);
	}

	cur_node = LIST_BEGIN(g_dt_mgr._dling_task_order);
	while(cur_node!=LIST_END(g_dt_mgr._dling_task_order))
	{
		p_task=(EM_TASK * )LIST_VALUE(cur_node);
		if(p_task->_task_info->_state==TS_TASK_WAITING)
		{
			dt_set_task_state(p_task,TS_TASK_DELETED);
			em_list_erase(&g_dt_mgr._dling_task_order, cur_node);
			g_dt_mgr._dling_order_changed = TRUE;
			return SUCCESS;
		}

		cur_node = LIST_NEXT(cur_node);
	}
	
	return TOO_MANY_TASKS;
}

_int32 dt_remove_task_from_order_list(EM_TASK * p_task)
{
	pLIST_NODE cur_node=NULL ;
	
      EM_LOG_DEBUG("dt_remove_task_from_order_list:task_id=%u",p_task->_task_info->_task_id); 
	cur_node = LIST_BEGIN(g_dt_mgr._dling_task_order);
	while(cur_node!=LIST_END(g_dt_mgr._dling_task_order))
	{
		if(p_task==(EM_TASK * )LIST_VALUE(cur_node))
		{
			em_list_erase(&g_dt_mgr._dling_task_order, cur_node);
			g_dt_mgr._dling_order_changed = TRUE;
			return SUCCESS;
		}

		cur_node = LIST_NEXT(cur_node);
	}
	
	if(cur_node== LIST_END(g_dt_mgr._dling_task_order))
	{
      		EM_LOG_ERROR("dt_remove_task_from_order_list:INVALID_TASK:task_id=%u",p_task->_task_info->_task_id); 
		return INVALID_TASK;
	}
	return SUCCESS;
}
_int32 dt_clear_order_list(void)
{
      		EM_LOG_DEBUG("dt_clear_order_list:%u",em_list_size(&g_dt_mgr._dling_task_order)); 
	em_list_clear(&g_dt_mgr._dling_task_order);

	return SUCCESS;	
	
}

_int32 dt_add_task_info_to_cache(TASK_INFO * p_task_info)
{
	_int32 ret_val=SUCCESS;
	pLIST_NODE cur_node=NULL ;
	TASK_INFO *p_task_info_t=NULL,*p_task_info_t2=NULL;
	EM_TASK * p_task = NULL;
	BOOL deleted=FALSE;

      EM_LOG_DEBUG("dt_add_task_info_to_cache:task_id=%u",p_task_info->_task_id); 
	if(em_list_size(&g_dt_mgr._full_task_infos)>=g_dt_mgr._max_cache_num)
	{
		//  delete the oldest one from cache,but the one is not a running task! 
		cur_node = LIST_BEGIN(g_dt_mgr._full_task_infos);
		while(cur_node!=LIST_END(g_dt_mgr._full_task_infos))
		{
			p_task_info_t = (TASK_INFO *)LIST_VALUE(cur_node);
			if((p_task_info_t->_is_deleted)||(p_task_info_t->_state!=TS_TASK_RUNNING))
			{
				p_task = dt_get_task_from_map(p_task_info_t->_task_id);
				sd_assert(p_task!=NULL);
      				EM_LOG_DEBUG("dt_erase_task_info_from_cache:task_id=%u",p_task_info_t->_task_id); 
				if(p_task->_change_flag!=0)
				{
					dt_save_task_to_file(p_task);
				}
				///////////////////////////////////
				ret_val = dt_task_info_malloc(&p_task_info_t2);
				CHECK_VALUE(ret_val);
		
				em_memcpy(p_task_info_t2, p_task_info_t, sizeof(TASK_INFO));
				p_task_info_t2->_full_info =FALSE;
		
				/* remove eigenvalue */
				ret_val = dt_remove_task_eigenvalue(p_task_info_t->_type,p_task_info_t->_eigenvalue);
				if(ret_val!=SUCCESS)
				{
					dt_task_info_free(p_task_info_t2);
					CHECK_VALUE(ret_val);
				}
				
				/* add to eigenvalue map */
				ret_val = dt_add_task_eigenvalue(p_task_info_t2->_type,p_task_info_t2->_eigenvalue,p_task_info_t2->_task_id);
				if(ret_val!=SUCCESS) 
				{
					dt_add_task_eigenvalue(p_task_info_t->_type,p_task_info_t->_eigenvalue,p_task_info_t->_task_id);
					dt_task_info_free(p_task_info_t2);
					CHECK_VALUE(ret_val);
				}
				
				p_task->_task_info = p_task_info_t2;
				dt_uninit_task_info(p_task_info_t);
				//em_list_erase(&g_dt_mgr._full_task_infos, cur_node);

				deleted=TRUE;
				break;
			}

			cur_node = LIST_NEXT(cur_node);
		}
		
		sd_assert(deleted==TRUE);
		if(deleted==FALSE)
		{
			return NOT_ENOUGH_CACHE;
		}
	}
	
	return em_list_push(&g_dt_mgr._full_task_infos,p_task_info);
}
_int32 dt_remove_task_info_from_cache(TASK_INFO * p_task_info)
{
	pLIST_NODE cur_node=NULL ;
	
      EM_LOG_DEBUG("dt_remove_task_info_from_cache:task_id=%u",p_task_info->_task_id); 
	cur_node = LIST_BEGIN(g_dt_mgr._full_task_infos);
	while(cur_node!=LIST_END(g_dt_mgr._full_task_infos))
	{
		if(p_task_info==(TASK_INFO *)LIST_VALUE(cur_node))
		{
			em_list_erase(&g_dt_mgr._full_task_infos, cur_node);
			//dt_uninit_task_info(p_task_info);
			return SUCCESS;
		}

		cur_node = LIST_NEXT(cur_node);
	}
	
	if(cur_node== LIST_END(g_dt_mgr._full_task_infos))
	{
		CHECK_VALUE(INVALID_TASK_INFO);
	}
	
	return SUCCESS;
}


_int32 dt_clear_cache(void)
{
	_u32 cache_list_size = em_list_size(&g_dt_mgr._full_task_infos);
	EM_LOG_DEBUG("dt_clear_cache:%u",cache_list_size); 
	sd_assert(cache_list_size==0);
	if(cache_list_size!=0)
	{
		em_list_clear(&g_dt_mgr._full_task_infos); 
	}
	return SUCCESS;	
}

_int32 dt_add_file_path_to_cache(EM_TASK * p_task,const char * path,_u32 path_len)
{
#if 1
	/* 效果不明显,不使用 */
	return NOT_IMPLEMENT;
#else
	_int32 ret_val=SUCCESS;
	EM_FILE_CACHE *p_file_cache_t=NULL;

      EM_LOG_DEBUG("dt_add_file_path_to_cache:task_id=%u,[%u]%s",p_task->_task_info->_task_id,path_len,path); 
	if(em_list_size(&g_dt_mgr._file_path_cache)>=g_dt_mgr._max_cache_num)
	{
		ret_val = em_list_pop(&g_dt_mgr._file_path_cache, (void **)&p_file_cache_t);
		CHECK_VALUE(ret_val);	
		if(p_file_cache_t==NULL)
		{
			CHECK_VALUE( NOT_ENOUGH_CACHE);
		}
	}

	if(p_file_cache_t==NULL)
	{
		ret_val=em_malloc(sizeof(EM_FILE_CACHE), (void**)&p_file_cache_t);
		CHECK_VALUE(ret_val);

		em_memset(p_file_cache_t, 0x00, sizeof(EM_FILE_CACHE));
	}
	
	p_file_cache_t->_task_id = p_task->_task_info->_task_id;
	if(p_file_cache_t->_str!=NULL)
	{
		if(em_strcmp(p_file_cache_t->_str,path)!=0)
		{
			EM_SAFE_DELETE(p_file_cache_t->_str);
		}
	}
	
	if(p_file_cache_t->_str==NULL)
	{
		ret_val=em_malloc(path_len+1, (void**)&p_file_cache_t->_str);
		if(ret_val!=SUCCESS)
		{
			EM_SAFE_DELETE(p_file_cache_t);
			CHECK_VALUE(ret_val);
		}
		em_memset(p_file_cache_t->_str, 0x00, path_len+1);
		em_memcpy(p_file_cache_t->_str, path, path_len);
	}
	
	return em_list_push(&g_dt_mgr._file_path_cache,p_file_cache_t);
	#endif
}
char * dt_get_file_path_from_cache(EM_TASK * p_task)
{
#if 1
	/* 效果不明显,不使用 */
	return NULL;
#else
	pLIST_NODE cur_node=NULL ;
	EM_FILE_CACHE *p_file_cache_t=NULL;
	
      EM_LOG_DEBUG("dt_get_file_path_from_cache:task_id=%u",p_task->_task_info->_task_id); 
	cur_node = LIST_BEGIN(g_dt_mgr._file_path_cache);
	while(cur_node!=LIST_END(g_dt_mgr._file_path_cache))
	{
		p_file_cache_t = (EM_FILE_CACHE *)LIST_VALUE(cur_node);
		if(p_file_cache_t->_task_id == p_task->_task_info->_task_id)
		{
			return p_file_cache_t->_str;
		}

		cur_node = LIST_NEXT(cur_node);
	}
	
	return NULL;
#endif
}


_int32 dt_clear_file_path_cache(void)
{
#if 1
	/* 效果不明显,不使用 */
	return NOT_IMPLEMENT;
#else
	pLIST_NODE cur_node=NULL ;
	EM_FILE_CACHE *p_file_cache_t=NULL;
	_u32 cache_list_size = em_list_size(&g_dt_mgr._file_path_cache);
	
      EM_LOG_DEBUG("dt_clear_file_path_cache:size=%u",cache_list_size); 
	  
	cur_node = LIST_BEGIN(g_dt_mgr._file_path_cache);
	while(cur_node!=LIST_END(g_dt_mgr._file_path_cache))
	{
		p_file_cache_t = (EM_FILE_CACHE *)LIST_VALUE(cur_node);
		EM_SAFE_DELETE(p_file_cache_t->_str);
		EM_SAFE_DELETE(p_file_cache_t);

		cur_node = LIST_NEXT(cur_node);
	}

	em_list_clear(&g_dt_mgr._file_path_cache); 

	return SUCCESS;	
#endif
}

/////////////////////////////////////

_int32 dt_add_file_name_to_cache(EM_TASK * p_task,const char * file_name,_u32 name_len)
{
#if 1
	/* 效果不明显,不使用 */
	return NOT_IMPLEMENT;
#else
	_int32 ret_val=SUCCESS;
	EM_FILE_CACHE *p_file_cache_t=NULL;

      EM_LOG_DEBUG("dt_add_file_name_to_cache:task_id=%u,[%u]%s",p_task->_task_info->_task_id,name_len,file_name); 
	if(em_list_size(&g_dt_mgr._file_name_cache)>=g_dt_mgr._max_cache_num)
	{
		ret_val = em_list_pop(&g_dt_mgr._file_name_cache, (void **)&p_file_cache_t);
		CHECK_VALUE(ret_val);	
		if(p_file_cache_t==NULL)
		{
			CHECK_VALUE( NOT_ENOUGH_CACHE);
		}
	}

	if(p_file_cache_t==NULL)
	{
		ret_val=em_malloc(sizeof(EM_FILE_CACHE), (void**)&p_file_cache_t);
		CHECK_VALUE(ret_val);

		em_memset(p_file_cache_t, 0x00, sizeof(EM_FILE_CACHE));
	}
	
	p_file_cache_t->_task_id = p_task->_task_info->_task_id;
	if(p_file_cache_t->_str!=NULL)
	{
		if(em_strcmp(p_file_cache_t->_str,file_name)!=0)
		{
			EM_SAFE_DELETE(p_file_cache_t->_str);
		}
	}
	
	if(p_file_cache_t->_str==NULL)
	{
		ret_val=em_malloc(name_len+1, (void**)&p_file_cache_t->_str);
		if(ret_val!=SUCCESS)
		{
			EM_SAFE_DELETE(p_file_cache_t);
			CHECK_VALUE(ret_val);
		}
		em_memset(p_file_cache_t->_str, 0x00, name_len+1);
		em_memcpy(p_file_cache_t->_str, file_name, name_len);
	}
	
	return em_list_push(&g_dt_mgr._file_name_cache,p_file_cache_t);
#endif
}
char * dt_get_file_name_from_cache(EM_TASK * p_task)
{
#if 1
	/* 效果不明显,不使用 */
	return NULL;
#else
	pLIST_NODE cur_node=NULL ;
	EM_FILE_CACHE *p_file_cache_t=NULL;
	
      EM_LOG_DEBUG("dt_get_file_name_from_cache:task_id=%u",p_task->_task_info->_task_id); 
	cur_node = LIST_BEGIN(g_dt_mgr._file_name_cache);
	while(cur_node!=LIST_END(g_dt_mgr._file_name_cache))
	{
		p_file_cache_t = (EM_FILE_CACHE *)LIST_VALUE(cur_node);
		if(p_file_cache_t->_task_id == p_task->_task_info->_task_id)
		{
			return p_file_cache_t->_str;
		}

		cur_node = LIST_NEXT(cur_node);
	}
	
	return NULL;
#endif
}

_int32 dt_remove_file_name_from_cache(EM_TASK * p_task)
{
#if 1
	/* 效果不明显,不使用 */
	return NOT_IMPLEMENT;
#else
	pLIST_NODE cur_node=NULL ;
	EM_FILE_CACHE *p_file_cache_t=NULL;
	
      EM_LOG_DEBUG("dt_remove_file_name_from_cache:task_id=%u",p_task->_task_info->_task_id); 
	cur_node = LIST_BEGIN(g_dt_mgr._file_name_cache);
	while(cur_node!=LIST_END(g_dt_mgr._file_name_cache))
	{
		p_file_cache_t = (EM_FILE_CACHE *)LIST_VALUE(cur_node);
		if(p_file_cache_t->_task_id == p_task->_task_info->_task_id)
		{
			EM_SAFE_DELETE(p_file_cache_t->_str);
			EM_SAFE_DELETE(p_file_cache_t);
			em_list_erase(&g_dt_mgr._file_name_cache, cur_node);
			return SUCCESS;
		}

		cur_node = LIST_NEXT(cur_node);
	}
	
	return SUCCESS;
#endif
}

_int32 dt_clear_file_name_cache(void)
{
#if 1
	/* 效果不明显,不使用 */
	return NOT_IMPLEMENT;
#else
	pLIST_NODE cur_node=NULL ;
	EM_FILE_CACHE *p_file_cache_t=NULL;
	_u32 cache_list_size = em_list_size(&g_dt_mgr._file_name_cache);
	
      EM_LOG_DEBUG("dt_clear_file_name_cache:size=%u",cache_list_size); 
	  
	cur_node = LIST_BEGIN(g_dt_mgr._file_name_cache);
	while(cur_node!=LIST_END(g_dt_mgr._file_name_cache))
	{
		p_file_cache_t = (EM_FILE_CACHE *)LIST_VALUE(cur_node);
		EM_SAFE_DELETE(p_file_cache_t->_str);
		EM_SAFE_DELETE(p_file_cache_t);

		cur_node = LIST_NEXT(cur_node);
	}

	em_list_clear(&g_dt_mgr._file_name_cache); 

	return SUCCESS;	
#endif
}

/* 检测磁盘空间时候不足*/
_int32 dt_check_free_disk_when_running_task( void )
{
	_int32 ret_val = SUCCESS;
	_u64 free_size = 0;
	_u32 cur_time = 0;
	static _u32 old_time=0,interval = 0;
	char download_path[MAX_FILE_PATH_LEN];
	
	EM_LOG_DEBUG( "dt_check_free_disk" );
		
	sd_time(&cur_time);
	if((old_time==0)||(TIME_SUBZ(cur_time,old_time)>=interval))
	{
		em_memset(download_path,0,MAX_FILE_PATH_LEN);
		ret_val = em_settings_get_str_item("system.download_path", download_path);
		CHECK_VALUE(ret_val);

		if(em_strlen(download_path)==0) return INVALID_FILE_PATH;
		
		old_time = cur_time;
		ret_val = sd_get_free_disk(download_path, &free_size);
		CHECK_VALUE(ret_val);

		/* 如果少于100MB,则返回空间不足 */
		if(free_size<MIN_FREE_DISK_SIZE) 
		{
    			EM_LOG_ERROR("dt_check_free_disk:free_disk=%u,INSUFFICIENT_DISK_SPACE!",free_size); 
    			//printf("\ndt_check_free_disk:free_disk=%u,INSUFFICIENT_DISK_SPACE!\n",free_size); 
			return INSUFFICIENT_DISK_SPACE;
		}

		/* 计算每次调度检测磁盘空间的间隔,如果大于1GB,就5秒检测一次,否则每秒检测一次 */
		interval =(free_size<(10*MIN_FREE_DISK_SIZE)?1:5);
		
    		EM_LOG_ERROR("dt_check_free_disk:free_disk=%u,interval=%u",free_size,interval); 
    		//printf("\ndt_check_free_disk:free_disk=%u,interval=%u\n",free_size,interval); 
	}
	return ret_val;	
}


_int32 dt_stop_over_running_task()
{
	pLIST_NODE cur_node=NULL ;
	EM_TASK * p_task = NULL;
	_u32 cur_time=0;
    _u32 need_stop_num = g_dt_mgr._running_task_num - g_dt_mgr._max_running_task_num;
    _u32 cur_num = 0;
	
	cur_node = LIST_RBEGIN(g_dt_mgr._dling_task_order);
	while((dt_is_running_task_full())&&(cur_node!=LIST_END(g_dt_mgr._dling_task_order)))
	{
		p_task = (EM_TASK * )LIST_VALUE(cur_node);
		if((dt_get_task_state(p_task) == TS_TASK_RUNNING)&&(!dt_is_vod_task(p_task)))
		{
            if( cur_num == need_stop_num)
                break;
			dt_remove_running_task(p_task);
			vod_report_impl( p_task->_task_info->_task_id);
			eti_stop_task(p_task->_inner_id);
			eti_delete_task(p_task->_inner_id);
			p_task->_inner_id = 0;
			em_time(&(cur_time));
			dt_set_task_finish_time(p_task,cur_time);
			dt_set_task_state(p_task,TS_TASK_WAITING);
			dt_bt_running_file_safe_delete(p_task);
			if(p_task->_waiting_stop)
			{
				p_task->_waiting_stop = FALSE;
			}
            cur_num++;
		}
		cur_node = LIST_PRE(cur_node);
	}

	return SUCCESS;
}


_int32 dt_set_max_running_task(_u32 max_tasks)
{
	g_dt_mgr._max_running_task_num = max_tasks;

	if(dt_is_running_task_full()&&!dt_is_task_locked())
	{
		dt_stop_over_running_task();
	}

	return SUCCESS;
}
BOOL dt_is_running_task_full(void)
{
	//em_settings_get_int_item( "system.max_running_tasks",(_int32*)&(g_dt_mgr._max_running_task_num));
      EM_LOG_DEBUG("dt_is_running_task_full:running_task_num=%u,running_task_num=%u",g_dt_mgr._running_task_num,g_dt_mgr._max_running_task_num); 
#ifdef ENABLE_NULL_PATH
	if(g_dt_mgr._running_task_num + g_no_disk_vod_task_num>=g_dt_mgr._max_running_task_num)
		return TRUE;
#else
	if(g_dt_mgr._running_task_num/*+dt_get_running_vod_task_num(  )*/>=g_dt_mgr._max_running_task_num)
		return TRUE;
#endif

	return FALSE;
}

_int32 dt_set_current_vod_task_id(_u32 task_id)
{
	if(g_dt_mgr._current_vod_task_id != task_id)
	{
		g_dt_mgr._current_vod_task_id = task_id;
	}
	return SUCCESS;
}

_u32 dt_get_current_vod_task_id(void)
{
	return g_dt_mgr._current_vod_task_id;
}

_int32 dt_set_reading_success_file(BOOL is_reading_success_file)
{
	g_dt_mgr._is_reading_success_file = is_reading_success_file;
	return SUCCESS;
}

BOOL dt_is_reading_success_file(void)
{
	return g_dt_mgr._is_reading_success_file;
}

_int32 dt_add_running_task(EM_TASK * p_task)
{
	_int32 i=0;
	
	while(g_running_task_lock)
	{
		em_sleep(1);
		if(i++>=2)
		{
			CHECK_VALUE(RUNNING_TASK_LOCK_CRASH);
		}
	}
	
	g_running_task_lock=TRUE;
	
	i=0;
	while((i<MAX_ALOW_TASKS)&&(g_running_task[i]._task_id!=0))
		i++;
	
	if(i==MAX_ALOW_TASKS)
	{
		g_running_task_lock=FALSE;
		CHECK_VALUE(TOO_MANY_RUNNING_TASKS);
	}
	g_running_task[i]._task_id = p_task->_task_info->_task_id;
	g_running_task[i]._inner_id= p_task->_inner_id;
	g_running_task[i]._dled_size_fresh_count = 0;
	g_running_task[i]._task = p_task;
	g_running_task[i]._status._state = TS_TASK_RUNNING;
	g_running_task[i]._status._type = p_task->_task_info->_type;
	g_running_task[i]._status._file_size= p_task->_task_info->_file_size;
	
	g_running_task_lock=FALSE;
	
	g_dt_mgr._running_task_num++;

	if(dt_is_vod_task(p_task))
	{
		//纯点播任务(非下载)开始时，增加运行的点播任务数
		dt_increase_running_vod_task_num();
	}
#ifdef KANKAN_PROJ
	if(dt_is_lan_task(p_task))
	{
		dt_increase_running_lan_task_num();
		//lan任务都是通过dt_start_next_task 调度开始，把waiting 的lan任务减去
		sd_assert(dt_get_waiting_lan_task_num());
		if(dt_get_waiting_lan_task_num()>0)
		{
			dt_decrease_waiting_lan_task_num();
		}
	}
#endif
     EM_LOG_DEBUG("dt_add_running_task:task_id=%u",p_task->_task_info->_task_id); 
	g_dt_mgr._running_task_changed=TRUE;
	return SUCCESS;
}
_int32 dt_remove_running_task(EM_TASK * p_task)
{
	_int32 i=0;

	sd_assert(g_dt_mgr._running_task_num!=0);
	if(g_dt_mgr._running_task_num==0)
		return SUCCESS;

	while(g_running_task_lock)
	{
		em_sleep(1);
		if(i++>=2)
		{
			CHECK_VALUE(RUNNING_TASK_LOCK_CRASH);
		}
	}
	
	g_running_task_lock=TRUE;
	
	i=0;
	while((i<MAX_ALOW_TASKS)&&(g_running_task[i]._task_id!=p_task->_task_info->_task_id))
		i++;

	if(i==MAX_ALOW_TASKS)
	{
		g_running_task_lock=FALSE;
		CHECK_VALUE(INVALID_TASK_ID);
	}

	em_memset(&g_running_task[i], 0,sizeof(RUNNING_TASK));
	
	g_running_task_lock=FALSE;
	
	g_dt_mgr._running_task_num--;

	if(dt_is_vod_task(p_task))
	{
		dt_decrease_running_vod_task_num();
	}
#ifdef KANKAN_PROJ
	if(dt_is_lan_task(p_task))
	{
		dt_decrease_running_lan_task_num();
	}
#endif
      EM_LOG_DEBUG("dt_remove_running_task:task_id=%u",p_task->_task_info->_task_id); 
	g_dt_mgr._running_task_changed=TRUE;
	return SUCCESS;
}
_int32 dt_get_running_et_task_id(_u32 etm_task_id,_u32 * et_task_id)
{
	_int32 i=0;

	//sd_assert(g_dt_mgr._running_task_num!=0);
	if(g_dt_mgr._running_task_num==0)
		return INVALID_TASK_ID;

	while(g_running_task_lock)
	{
		em_sleep(1);
		if(i++>=2)
		{
			CHECK_VALUE(RUNNING_TASK_LOCK_CRASH);
		}
	}
	
	g_running_task_lock=TRUE;
	
	i=0;
	while((i<MAX_ALOW_TASKS)&&(g_running_task[i]._task_id!=etm_task_id))
		i++;

	if(i==MAX_ALOW_TASKS)
	{
		g_running_task_lock=FALSE;
		CHECK_VALUE(INVALID_TASK_ID);
	}

	*et_task_id = g_running_task[i]._inner_id;
	
	g_running_task_lock=FALSE;
	
	return SUCCESS;
}

/* 由于Android 平台下在下载到4MB时在sd卡中创建大文件导致下载速度为0,影响用户体验,在此做一个假速度忽悠用户*/
void dt_4MB_fake_speed(RUNNING_STATUS * p_running_status)
{
/*
#if _ANDROID_LINUX
	if((p_running_status->_dl_speed==0)&&(is_big_file_enlarging())&&(em_is_net_ok(FALSE))&&(sd_get_net_type()!=UNKNOWN_NET))
	{
		_u32 times = 0;
		sd_time(&times);
		sd_srand(times);
		p_running_status->_dl_speed = sd_rand()%100000;   // 100KB以内
		p_running_status->_downloaded_data_size+=(p_running_status->_dl_speed/10);
	}
#endif
*/
}
_int32 dt_update_running_task(void)
{
	_int32 i=0;
	BOOL save_dled_size=FALSE;
	BOOL not_enough_free_disk = FALSE;
	ETI_TASK et_task_info;
	RUNNING_TASK running_task_tmp[MAX_ALOW_TASKS];
#ifdef ENABLE_HSC
	ET_HIGH_SPEED_CHANNEL_INFO hsc_info;
#endif /* ENABLE_HSC */

	if(g_dt_mgr._running_task_num==0)
		return SUCCESS;

	if(g_running_task_lock)
		return RUNNING_TASK_LOCK_CRASH;
	
	g_running_task_lock = TRUE;

	em_memcpy(&running_task_tmp, &g_running_task, MAX_ALOW_TASKS*sizeof(RUNNING_TASK));

	g_running_task_lock=FALSE;

/////////////////////////////////////////////////////////////////////////////////////////////	
	/* 检测磁盘空间是否不足*/
	if(SUCCESS != dt_check_free_disk_when_running_task( ))
	{
		/* 不够空间 */
		not_enough_free_disk = TRUE;
	}

	for(i=0;i<MAX_ALOW_TASKS;i++)
	{
		if(running_task_tmp[i]._inner_id != 0)
		{
			em_memset(&et_task_info,0,sizeof(ETI_TASK));
			if(eti_get_task_info(running_task_tmp[i]._inner_id, &et_task_info)==SUCCESS)
			{
				EM_LOG_DEBUG("dt_update_running_task: r_downloaded_data_size = %llu, et_downloaded_data_size =%llu, dled_size_fresh_count = %u", running_task_tmp[i]._status._downloaded_data_size, et_task_info._downloaded_data_size, running_task_tmp[i]._dled_size_fresh_count);
				running_task_tmp[i]._status._dl_speed= et_task_info._speed;
				dt_4MB_fake_speed(&running_task_tmp[i]._status);
				running_task_tmp[i]._status._ul_speed= et_task_info._ul_speed;
				running_task_tmp[i]._status._downloading_pipe_num= et_task_info._dowing_server_pipe_num+et_task_info._dowing_peer_pipe_num;
				running_task_tmp[i]._status._connecting_pipe_num= et_task_info._connecting_server_pipe_num+et_task_info._connecting_peer_pipe_num;
				save_dled_size=FALSE;
				if(running_task_tmp[i]._status._downloaded_data_size!= et_task_info._downloaded_data_size)
				{
				#ifdef ENABLE_PROGRESS_BACK_REPORT
					/* 回退超过10M就上报 */
					if(running_task_tmp[i]._status._downloaded_data_size > (et_task_info._downloaded_data_size + MAX_PROGRESS_BACK) )
					{
						EM_LOG_DEBUG("dt_update_running_task ERROR: progress_back = %llu", (running_task_tmp[i]._status._downloaded_data_size - et_task_info._downloaded_data_size)/(1024*1024) );
						if(running_task_tmp[i]._task->_task_info->_full_info)
						{
							EM_P2SP_TASK *p_p2sp_task = (EM_P2SP_TASK *)running_task_tmp[i]._task->_task_info;
							em_set_download_progress_back_info_to_report(p_p2sp_task->_url, et_task_info._file_size,running_task_tmp[i]._status._downloaded_data_size - et_task_info._downloaded_data_size, et_task_info._task_status, et_task_info._failed_code);
						}
						else
						{
							char *url = dt_get_task_url_from_file(running_task_tmp[i]._task);
							em_set_download_progress_back_info_to_report(url, et_task_info._file_size,running_task_tmp[i]._status._downloaded_data_size - et_task_info._downloaded_data_size, et_task_info._task_status, et_task_info._failed_code);
						}
						sd_create_task(em_download_progress_back_report_handler, 0, NULL, NULL);
					}
				#endif
					/* 不显示download size 回退的情况 */
					if(running_task_tmp[i]._status._downloaded_data_size > et_task_info._downloaded_data_size)
					{
						if((et_task_info._downloaded_data_size!=0)&&(running_task_tmp[i]._status._downloaded_data_size > et_task_info._downloaded_data_size+2*1024*1024))
						{
							/* ETM 和ET 之间的download size 相差大于2MB 时，才显示到界面*/
							running_task_tmp[i]._status._downloaded_data_size = et_task_info._downloaded_data_size;
						}
					}
					else
					{
						running_task_tmp[i]._status._downloaded_data_size= et_task_info._downloaded_data_size;
						EM_LOG_DEBUG("dt_update_running_task: r_downloaded_data_size = %llu", running_task_tmp[i]._status._downloaded_data_size);
					}
					if(++running_task_tmp[i]._dled_size_fresh_count>=EM_SAVE_DLED_SIZE_INTERVAL)
					{
						/* Save the downloaded data size to the file every EM_SAVE_DLED_SIZE_INTERVAL seconds */
						save_dled_size=TRUE;
						running_task_tmp[i]._dled_size_fresh_count=0;
					}
					dt_set_task_downloaded_size(running_task_tmp[i]._task,running_task_tmp[i]._status._downloaded_data_size,save_dled_size);
				}
				
				if((et_task_info._file_size!=0)&&(et_task_info._file_size!=running_task_tmp[i]._task->_task_info->_file_size))
				{
					//sd_assert(running_task_tmp[i]._task->_task_info->_file_size==0);
					running_task_tmp[i]._status._file_size= et_task_info._file_size;
					dt_set_task_file_size(running_task_tmp[i]._task,et_task_info._file_size);
					
					/* 是否有盘点播任务? */
					if(dt_is_vod_task(running_task_tmp[i]._task))
					{
						/* 注意:这个任务之前file_size应该为0,否则会导致used_vod_cache_size 计算错误 */
						dt_increase_used_vod_cache_size(running_task_tmp[i]._task);
					}
				}
						
				if((running_task_tmp[i]._task->_task_info->_correct_file_name==FALSE)&&(et_task_info._file_create_status==ET_FILE_CREATED_SUCCESS))
				{
					dt_get_task_file_name_from_et(running_task_tmp[i]._task);
				}
				
				if((running_task_tmp[i]._task->_task_info->_type==TT_URL)&&(running_task_tmp[i]._task->_task_info->_have_tcid==FALSE)&&(et_task_info._file_create_status==ET_FILE_CREATED_SUCCESS))
				{
					dt_get_task_tcid_from_et(running_task_tmp[i]._task);
				}
					
				switch(et_task_info._task_status)
				{
					case ET_TASK_RUNNING:
					case ET_TASK_VOD:
				#ifdef ENABLE_HSC
						if (et_get_hsc_info(running_task_tmp[i]._inner_id, &hsc_info) == SUCCESS)
						{
							EM_LOG_DEBUG("dt_update_running_task: et_get_hsc_info"); 
							if (ET_HSC_SUCCESS == hsc_info._stat)
							{
								running_task_tmp[i]._task->_use_hsc = TRUE;
								dt_set_hsc_mode_impl(running_task_tmp[i]._task);
							}

							if (running_task_tmp[i]._task->_hsc_info._can_use != hsc_info._can_use ||
								running_task_tmp[i]._task->_hsc_info._stat != hsc_info._stat)
							{
								g_need_report_to_remote = TRUE;
							}
							em_memcpy(&running_task_tmp[i]._task->_hsc_info, &hsc_info, sizeof(EM_HIGH_SPEED_CHANNEL_INFO));
							#ifdef ASSISTANT_PROJ
							running_task_tmp[i]._status._dl_speed+=hsc_info._speed;
							#endif /* ASSISTANT_PROJ */
						}
				#endif /* ENABLE_HSC */

						if((et_task_info._downloaded_data_size!=0) && (et_task_info._downloaded_data_size==et_task_info._file_size) && (running_task_tmp[i]._task_id!=dt_get_current_vod_task_id()))
						{
							if(g_have_dead_task == FALSE)
								g_have_dead_task = TRUE;
								
							running_task_tmp[i]._status._state = TS_TASK_SUCCESS;
							dt_set_task_state(running_task_tmp[i]._task, TS_TASK_SUCCESS);
						}
						else if(not_enough_free_disk&&(!running_task_tmp[i]._task->_task_info->_check_data))
						{
							if(running_task_tmp[i]._task->_task_info->_file_size != running_task_tmp[i]._task->_task_info->_downloaded_data_size)
							{
								//非下载完的任务
								if(running_task_tmp[i]._status._state!=TS_TASK_FAILED)
								{
									/* 强制使任务失败 */
									if(g_have_dead_task==FALSE)
										g_have_dead_task= TRUE;
								
									if((running_task_tmp[i]._task->_task_info->_have_name==FALSE)&&(running_task_tmp[i]._task->_task_info->_file_name_len!=0))
									{
										running_task_tmp[i]._task->_task_info->_have_name=TRUE;
										dt_set_task_change( running_task_tmp[i]._task,CHANGE_HAVE_NAME);
									}
								
									running_task_tmp[i]._status._state = TS_TASK_FAILED;
									dt_set_task_failed_code(running_task_tmp[i]._task,112); //DATA_NO_SPACE_TO_CREAT_FILE
									dt_set_task_state(running_task_tmp[i]._task, TS_TASK_FAILED);
								}
							}
						}

						break;
					case ET_TASK_SUCCESS:
						if(running_task_tmp[i]._status._state!=ET_TASK_SUCCESS)
						{
							if(g_have_dead_task==FALSE)
								g_have_dead_task= TRUE;
							
							running_task_tmp[i]._status._state = TS_TASK_SUCCESS;
							dt_set_task_state(running_task_tmp[i]._task, TS_TASK_SUCCESS);
						}
						break;
					case ET_TASK_FAILED:
						if(running_task_tmp[i]._status._state != TS_TASK_FAILED)
						{
							if(g_have_dead_task == FALSE)
								g_have_dead_task = TRUE;
							
							if((running_task_tmp[i]._task->_task_info->_have_name==FALSE)&&(running_task_tmp[i]._task->_task_info->_file_name_len!=0))
							{
								running_task_tmp[i]._task->_task_info->_have_name=TRUE;
								dt_set_task_change( running_task_tmp[i]._task,CHANGE_HAVE_NAME);
							}
							
							running_task_tmp[i]._status._state = TS_TASK_FAILED;
							sd_assert(et_task_info._failed_code!=SUCCESS);
							dt_set_task_failed_code(running_task_tmp[i]._task,et_task_info._failed_code);
							dt_set_task_state(running_task_tmp[i]._task, TS_TASK_FAILED);
						}
						break;
					default:
						sd_assert(FALSE);
				}
			}
		}
	}

	
/////////////////////////////////////////////////////////////////////////////////////////////	
	i = 0;
	while(g_running_task_lock)
	{
		em_sleep(1);
		if(i++>=10)
		{
			CHECK_VALUE(RUNNING_TASK_LOCK_CRASH);
		}
	}
	
	g_running_task_lock=TRUE;
	em_memcpy(&g_running_task,&running_task_tmp,  MAX_ALOW_TASKS*sizeof(RUNNING_TASK));

	g_running_task_lock=FALSE;
	
/////////////////////////////////////////////////////////////////////////////////////////////
	/* up date bt sub file info */
	for(i=0;i<MAX_ALOW_TASKS;i++)
	{
		if(running_task_tmp[i]._inner_id!=0)
		{
			if(running_task_tmp[i]._task->_task_info->_type==TT_BT)
			{
				dt_update_bt_sub_file_info(running_task_tmp[i]._task);
			}
		}
	}
/////////////////////////////////////////////////////////////////////////////////////////////	
	return SUCCESS;
}


_int32 dt_get_running_task(_u32 task_id,RUNNING_STATUS *p_status)
{
	_int32 i=0;

	if(g_dt_mgr._running_task_num==0)
		return INVALID_TASK_ID;
	
	while(g_running_task_lock)
	{
		em_sleep(1);
		if(i++>=2)
		{
			CHECK_VALUE(RUNNING_TASK_LOCK_CRASH);
		}
	}
	
	g_running_task_lock=TRUE;

	i=0;
	while((i<MAX_ALOW_TASKS)&&(g_running_task[i]._task_id!=task_id))
		i++;

	if(i==MAX_ALOW_TASKS)
	{
		g_running_task_lock=FALSE;
		return INVALID_TASK_ID;
	}

	em_memcpy(p_status,&g_running_task[i]._status, sizeof(RUNNING_STATUS));
	
	g_running_task_lock=FALSE;
	
      EM_LOG_DEBUG("dt_get_running_task:task_id=%u",task_id); 
	return SUCCESS;
}
_int32 dt_get_download_speed(_u32 * speed)
{
	_int32 i=0;

	if(g_dt_mgr._running_task_num==0)
	{
		*speed=0;
		return SUCCESS;
	}
	
	while(g_running_task_lock)
	{
		em_sleep(1);
		if(i++>=2)
		{
			CHECK_VALUE(RUNNING_TASK_LOCK_CRASH);
		}
	}
	
	g_running_task_lock=TRUE;
	
	*speed=0;
	i=0;
	while(i<MAX_ALOW_TASKS)
	{
		if(g_running_task[i]._task_id!=0)
		{
			*speed+=g_running_task[i]._status._dl_speed;
		}
		i++;
	}

	g_running_task_lock=FALSE;
	
      EM_LOG_DEBUG("dt_get_download_speed:%u",*speed); 
	return SUCCESS;
}

_int32 dt_get_upload_speed(_u32 * speed)
{
	_int32 i=0;

	if(g_dt_mgr._running_task_num==0)
	{
		*speed=0;
		return SUCCESS;
	}
	
	while(g_running_task_lock)
	{
		em_sleep(1);
		if(i++>=2)
		{
			CHECK_VALUE(RUNNING_TASK_LOCK_CRASH);
		}
	}
	
	g_running_task_lock=TRUE;
	
	*speed=0;
	i=0;
	while(i<MAX_ALOW_TASKS)
	{
		if(g_running_task[i]._task_id!=0)
		{
			*speed+=g_running_task[i]._status._ul_speed;
		}
		i++;
	}

	g_running_task_lock=FALSE;
	
      EM_LOG_DEBUG("dt_get_upload_speed:%u",*speed); 
	return SUCCESS;
}
#if defined( MACOS)||defined(_ANDROID_LINUX)
static BOOL g_start_delete = FALSE;
void dt_asyn_stop_task_handler(void *arglist)
{
	_int32 ret_val = SUCCESS;
	_u32 task_id = (_u32)arglist;
	g_start_delete = TRUE;
	//printf("\n ssss \n");
	em_pthread_detach();
	em_ignore_signal();

	eti_delete_task(task_id);

	g_start_delete = FALSE;
	//printf("\n eee \n");
	return ;
}		
_int32 dt_asyn_delete_task(_u32 task_id)
{
	_int32 thread_id = 0,ret_val = SUCCESS,count= 0;
#if 0 //defined(_DEBUG)
	_u32 old_time= 0,new_time = 0;
	em_time_ms(&old_time);
	printf("\n dt_asyn_delete_task start:%u,task_id =%u,g_start_stop=%d\n",old_time,task_id,g_start_delete);
#endif

	while(g_start_delete&&(count++<5000))
		em_sleep(1);
	g_start_delete = FALSE;
	
	ret_val = em_create_task(dt_asyn_stop_task_handler, 1024, (void*)task_id, &thread_id);
	CHECK_VALUE(ret_val);

	count = 0;
	while(!g_start_delete&&(count++<100))
		em_sleep(1);
	
#if 0 //defined(_DEBUG)
	em_time_ms(&new_time);
	printf("\n dt_asyn_delete_task end:%u,use time =%u,g_start_stop=%d,count=%d\n",new_time,new_time-old_time,g_start_delete,count);
#endif
	return SUCCESS;
}
#endif /* MACOS */

_int32 dt_clear_dead_task(void)
{
	_int32 i=0;
	EM_TASK *p_task=NULL;
	RUNNING_TASK running_task_tmp[MAX_ALOW_TASKS];
	_u32 cur_time = 0;
	TASK_STATE  task_state;   

	if((g_have_dead_task==FALSE)&&(g_have_waiting_stop_task==FALSE))
		return SUCCESS;
							
	if(g_running_task_lock)
	{
		return RUNNING_TASK_LOCK_CRASH;
	}
	
	g_running_task_lock=TRUE;

	em_memcpy(&running_task_tmp, &g_running_task, MAX_ALOW_TASKS*sizeof(RUNNING_TASK));

	g_running_task_lock=FALSE;
/////////////////////////////////////////////////////////
      EM_LOG_DEBUG("dt_clear_dead_task"); 

	for(i=0;i<MAX_ALOW_TASKS;i++)
	{
		if(running_task_tmp[i]._inner_id!=0)
		{
			if((running_task_tmp[i]._status._state==TS_TASK_SUCCESS)||(running_task_tmp[i]._status._state==TS_TASK_FAILED)||(running_task_tmp[i]._task->_waiting_stop))
			{

				p_task = running_task_tmp[i]._task;
				task_state = running_task_tmp[i]._status._state;

				dt_remove_running_task(p_task);
				vod_report_impl( p_task->_task_info->_task_id);
				eti_stop_task(p_task->_inner_id);
				#if defined(MACOS) //||defined(_ANDROID_LINUX)
				/* 另起线程删除任务 */
				dt_asyn_delete_task(p_task->_inner_id);
				#else
				eti_delete_task(p_task->_inner_id);
				#endif
				p_task->_inner_id = 0;
				em_time(&(cur_time));
				dt_set_task_finish_time(p_task,cur_time);
				
				if(p_task->_waiting_stop)
				{
					if(task_state==TS_TASK_RUNNING)
					{
						if((p_task->_task_info->_file_size > 0) && (p_task->_task_info->_downloaded_data_size>=p_task->_task_info->_file_size))
						{
							/* 已经下载完毕 */
							task_state = TS_TASK_SUCCESS;
							dt_set_task_state(p_task, TS_TASK_SUCCESS);
						}
						else
						if (p_task->_vod_stop) 
						{
							/* 从播放器终止返回的要恢复任务原先的状态 */
							if(p_task->_need_pause)
							{
								/* 原先是暂停状态 */
								dt_set_task_state(p_task,TS_TASK_PAUSED);
								p_task->_need_pause = FALSE;
							}
							else
							{
								/* 原先是等待状态 */
								if(dt_is_lan_task(p_task))
								{
									dt_increase_waiting_lan_task_num();
								}
								dt_set_task_state(p_task,TS_TASK_WAITING);
							}
							p_task->_vod_stop = FALSE;
						}
						else 
						{
							/* 从界面直接点的暂停任务 */
							dt_set_task_state(p_task,TS_TASK_PAUSED);
						}
					}
					p_task->_waiting_stop = FALSE;
				}
	
				/* remove from dling_order_list */
				if(task_state==TS_TASK_SUCCESS)
				{
					dt_remove_task_from_order_list(p_task);
				}

				if(task_state==TS_TASK_FAILED)
				{
					/* No resource  */
					if(dt_can_failed_task_restart(p_task))
					{
						dt_have_task_failed();
					}
				}

				/* save change to task_file */
				//ts_save_task_to_file(running_task_tmp[i]._task);

				dt_bt_running_file_safe_delete(p_task);
				
				/* 如果是无盘点播任务,直接销毁 */
				if(dt_is_vod_task_no_disk(p_task))
				{
					dt_destroy_vod_task(p_task);
				}
			}
		}
	}

	g_have_dead_task=FALSE;
	g_have_waiting_stop_task=FALSE;

	return SUCCESS;
}
/* 停掉最近启动的那个任务,并将它置为等待状态 */
_int32 dt_stop_the_latest_task( void )
{
	_int32 i=0;
	EM_TASK *p_task=NULL;
	RUNNING_TASK running_task_tmp[MAX_ALOW_TASKS];
	_u32 cur_time = 0,start_time = 0;
	TASK_STATE  task_state;   

	while(g_running_task_lock)
	{
		em_sleep(1);
		if(i++>=2)
		{
			CHECK_VALUE(RUNNING_TASK_LOCK_CRASH);
		}
	}

	g_running_task_lock=TRUE;

	em_memcpy(&running_task_tmp, &g_running_task, MAX_ALOW_TASKS*sizeof(RUNNING_TASK));

	g_running_task_lock=FALSE;
/////////////////////////////////////////////////////////
      EM_LOG_DEBUG("dt_stop_the_latest_task"); 

	for(i=0;i<MAX_ALOW_TASKS;i++)
	{
		if(running_task_tmp[i]._inner_id!=0)
		{
			if(start_time < running_task_tmp[i]._task->_task_info->_start_time)
			{
				start_time = running_task_tmp[i]._task->_task_info->_start_time;
				p_task = running_task_tmp[i]._task;
			}
		}
	}
	sd_assert(p_task!=NULL);
	if(p_task!=NULL)
	{
		task_state = p_task->_task_info->_state;

		dt_remove_running_task(p_task);
		//vod_report_impl( p_task->_task_info->_task_id);
		eti_stop_task(p_task->_inner_id);
		eti_delete_task(p_task->_inner_id);
		p_task->_inner_id = 0;
		em_time(&(cur_time));
		dt_set_task_finish_time(p_task,cur_time);
		
		/* remove from dling_order_list */
		if(task_state==TS_TASK_SUCCESS)
		{
			dt_remove_task_from_order_list(p_task);
		}

		if(task_state==TS_TASK_RUNNING)
		{
			dt_set_task_state(p_task,TS_TASK_WAITING);
		}
			
		if(p_task->_waiting_stop)
		{
			p_task->_waiting_stop = FALSE;
		}

		if(task_state==TS_TASK_FAILED)
		{
			/* No resource  */
			if(p_task->_task_info->_failed_code==130||p_task->_task_info->_failed_code>=0x08000000)
			{
				dt_have_task_failed();
			}
		}

		/* save change to task_file */
		//ts_save_task_to_file(running_task_tmp[i]._task);

		dt_bt_running_file_safe_delete(p_task);
		
		/* 如果是无盘点播任务,直接销毁 */
		if(dt_is_vod_task_no_disk(p_task))
		{
			dt_destroy_vod_task(p_task);
		}
	}
	return SUCCESS;
}
/* 停掉另一个下载任务,并将它置为等待状态 */
_int32 dt_stop_the_other_download_task( _u32 task_id )
{
	_int32 i=0;
	EM_TASK *p_task=NULL;
	RUNNING_TASK running_task_tmp[MAX_ALOW_TASKS];
	_u32 cur_time = 0;
	TASK_STATE  task_state;   

	while(g_running_task_lock)
	{
		em_sleep(1);
		if(i++>=2)
		{
			CHECK_VALUE(RUNNING_TASK_LOCK_CRASH);
		}
	}

	g_running_task_lock=TRUE;

	em_memcpy(&running_task_tmp, &g_running_task, MAX_ALOW_TASKS*sizeof(RUNNING_TASK));

	g_running_task_lock=FALSE;
/////////////////////////////////////////////////////////
      EM_LOG_DEBUG("dt_stop_the_latest_task"); 

	for(i=0;i<MAX_ALOW_TASKS;i++)
	{
		if((running_task_tmp[i]._inner_id!=0)&&(running_task_tmp[i]._task_id!=task_id))
		{
			/* 找到另一个正在下载的任务 */
			p_task = running_task_tmp[i]._task;
			break;
		}
	}

	if(p_task!=NULL)
	{
		task_state = p_task->_task_info->_state;

		dt_remove_running_task(p_task);
		vod_report_impl( p_task->_task_info->_task_id);
		eti_stop_task(p_task->_inner_id);
		eti_delete_task(p_task->_inner_id);
		p_task->_inner_id = 0;
		em_time(&(cur_time));
		dt_set_task_finish_time(p_task,cur_time);
		
		/* remove from dling_order_list */
		if(task_state==TS_TASK_SUCCESS)
		{
			dt_remove_task_from_order_list(p_task);
		}

		if(task_state==TS_TASK_RUNNING)
		{
			dt_set_task_state(p_task,TS_TASK_WAITING);
		}
			
		if(p_task->_waiting_stop)
		{
			p_task->_waiting_stop = FALSE;
		}

		if(task_state==TS_TASK_FAILED)
		{
			/* No resource  */
			if(p_task->_task_info->_failed_code==130||p_task->_task_info->_failed_code>=0x08000000)
			{
				dt_have_task_failed();
			}
		}

		/* save change to task_file */
		//ts_save_task_to_file(running_task_tmp[i]._task);

		dt_bt_running_file_safe_delete(p_task);
		
		/* 如果是无盘点播任务,直接销毁 */
		if(dt_is_vod_task_no_disk(p_task))
		{
			dt_destroy_vod_task(p_task);
		}
	}
	return SUCCESS;
}

_int32 dt_stop_all_waiting_tasks_impl(void)
{
	pLIST_NODE cur_node=NULL ;
	EM_TASK * p_task = NULL;

      EM_LOG_DEBUG("dt_stop_all_waiting_tasks"); 

	if(g_have_waitting_task==FALSE)
		return SUCCESS;

	cur_node = LIST_BEGIN(g_dt_mgr._dling_task_order);
	while(cur_node!=LIST_END(g_dt_mgr._dling_task_order))
	{
		p_task=(EM_TASK *  )LIST_VALUE(cur_node);
		cur_node = LIST_NEXT(cur_node);

		if(TS_TASK_WAITING==dt_get_task_state(p_task))
		{
			dt_stop_task_impl(p_task);
			/* 如果是无盘点播任务,直接销毁 */
			if(dt_is_vod_task(p_task)&&dt_is_vod_task_no_disk(p_task))
			{
				dt_destroy_vod_task(p_task);
			}
		}

	}

	g_have_waitting_task=FALSE;
	
	return SUCCESS;
	
}

_int32 dt_start_next_task(void)
{
	pLIST_NODE cur_node=NULL ;

	EM_TASK * p_task = NULL;
	_int32 ret_val = SUCCESS,node_conunt = 0;

	EM_LOG_DEBUG("dt_start_next_task:%d,%d",g_have_waitting_task,g_have_task_failed); 
	if((g_have_waitting_task==FALSE)&&(g_have_task_failed==FALSE))
		return SUCCESS;

	if(dt_is_running_task_full()==TRUE)
	{
		return SUCCESS;
	}

    EM_LOG_DEBUG("dt_start_next_task"); 

	if(g_have_waitting_task)
	{
		cur_node = LIST_BEGIN(g_dt_mgr._dling_task_order);
		while(cur_node!=LIST_END(g_dt_mgr._dling_task_order))
		{
			p_task=(EM_TASK *  )LIST_VALUE(cur_node);
			if(p_task==NULL)
			{	
				sd_assert(FALSE);
			}
			cur_node = LIST_NEXT(cur_node);

			if(p_task&&TS_TASK_WAITING==dt_get_task_state(p_task))
			{
				if(!dt_is_running_task_full())
				{
					ret_val = dt_start_task_impl(p_task);
					if(ret_val ==NETWORK_INITIATING)
						return SUCCESS;
				}
				else
				{
					g_have_waitting_task=TRUE;
					return SUCCESS;
				}
			}
			node_conunt++;
		}
	}
	
	g_have_waitting_task=FALSE;

	if(g_have_task_failed)
	{
		cur_node = LIST_BEGIN(g_dt_mgr._dling_task_order);
		while(cur_node!=LIST_END(g_dt_mgr._dling_task_order))
		{
			p_task=(EM_TASK *  )LIST_VALUE(cur_node);
			cur_node = LIST_NEXT(cur_node);

			if((TS_TASK_FAILED==dt_get_task_state(p_task)) && dt_can_failed_task_restart(p_task))
			{
				if(!dt_is_running_task_full())
				{
					ret_val = dt_start_task_impl(p_task);
					if(ret_val ==NETWORK_INITIATING)
						return SUCCESS;
				}
				else
				{
					g_have_task_failed=TRUE;
					return SUCCESS;
				}
			}

		}

	}
	
	g_have_task_failed = FALSE;

	return SUCCESS;

	
}


_int32 dt_have_task_waitting(void)
{
	if(!g_have_waitting_task)
		g_have_waitting_task=TRUE;
	
	return SUCCESS;
	
}

_u32 dt_get_running_task_num(void )
{
		return g_dt_mgr._running_task_num;
}

EM_TASK *  dt_get_pri_task(void)
{
	pLIST_NODE cur_node=NULL ;
	EM_TASK * p_task = NULL;
	//_int32 ret_val = SUCCESS;

    EM_LOG_DEBUG("dt_start_next_task"); 
	cur_node = LIST_BEGIN(g_dt_mgr._dling_task_order);
	while(cur_node!=LIST_END(g_dt_mgr._dling_task_order))
	{
		p_task=(EM_TASK *  )LIST_VALUE(cur_node);
		cur_node = LIST_NEXT(cur_node);

		if((!dt_is_vod_task(p_task))&&(TS_TASK_SUCCESS>dt_get_task_state(p_task)))
		{
			return p_task;
		}

	}

	return NULL;
}


////////////////////////////////////


_int32 dt_get_pri_id_list_impl(_u32 * id_array_buffer,_u32 * buffer_len)
{
	pLIST_NODE cur_node=NULL ;
	_u32 i=0;
	EM_TASK * p_task = NULL;
	
      EM_LOG_DEBUG("dt_get_pri_id_list_impl"); 
	if((em_list_size(&g_dt_mgr._dling_task_order)-dt_get_vod_task_num()>*buffer_len)||(id_array_buffer==NULL))	
	{
		*buffer_len = em_list_size(&g_dt_mgr._dling_task_order)-dt_get_vod_task_num();
		return NOT_ENOUGH_BUFFER;
	}

	cur_node = LIST_BEGIN(g_dt_mgr._dling_task_order);
	while(cur_node!=LIST_END(g_dt_mgr._dling_task_order))
	{
		p_task = (EM_TASK * )LIST_VALUE(cur_node);
		if(!dt_is_vod_task(p_task))
		{
			id_array_buffer[i++]=p_task->_task_info->_task_id;
		}
		cur_node = LIST_NEXT(cur_node);
	}

	*buffer_len = em_list_size(&g_dt_mgr._dling_task_order)-dt_get_vod_task_num();
	
	return SUCCESS;
}
_int32 dt_pri_level_up_impl(_u32 task_id,_u32 steps)
{
	pLIST_NODE cur_node=NULL ,before_node=NULL ;
	_int32 ret_val = SUCCESS;
	_u32 i=0;
	EM_TASK * p_task =NULL;
	
      EM_LOG_DEBUG("dt_pri_level_up_impl:task_id=%u",task_id); 

	  if(em_list_size(&g_dt_mgr._dling_task_order)==0)
	  {
	  	return INVALID_TASK_ID;
	  }
	  
	cur_node = LIST_BEGIN(g_dt_mgr._dling_task_order);
	p_task = (EM_TASK * )LIST_VALUE(cur_node);
	
	if((steps==0)||(task_id == p_task->_task_info->_task_id))
	{
		/* do nothing */
		return SUCCESS;
	}
	
	before_node=cur_node;
	while(cur_node!=LIST_END(g_dt_mgr._dling_task_order))
	{
		p_task = (EM_TASK * )LIST_VALUE(cur_node);
		if(task_id == p_task->_task_info->_task_id)
		{
			ret_val= em_list_insert(&g_dt_mgr._dling_task_order, (void *)p_task, before_node);
			sd_assert(ret_val==SUCCESS);
			if(ret_val==SUCCESS)
			{
				ret_val= em_list_erase(&g_dt_mgr._dling_task_order, cur_node);
				sd_assert(ret_val==SUCCESS);
				g_dt_mgr._dling_order_changed = TRUE;
				return ret_val;
			}
			else
			{
				return ret_val;
			}
		}
		else
		{
			if(i>=steps)
			{
				before_node = LIST_NEXT(before_node);
			}
		}
		
		/* 如果是vod任务,跳过*/
		if(!dt_is_vod_task(p_task))
		{
			i++;
		}
		
		cur_node = LIST_NEXT(cur_node);
	}
	
	return INVALID_TASK_ID;
}

_int32 dt_pri_level_down_impl(_u32 task_id,_u32 steps)
{
	pLIST_NODE cur_node=NULL ,before_node=NULL ;
	_int32 ret_val = SUCCESS;
	_u32 i=0;
	EM_TASK * p_task =NULL;
	
      EM_LOG_DEBUG("dt_pri_level_down_impl:task_id=%u",task_id); 

	  if(em_list_size(&g_dt_mgr._dling_task_order)==0)
	  {
	  	return INVALID_TASK_ID;
	  }
	  
	cur_node = LIST_RBEGIN(g_dt_mgr._dling_task_order);
	p_task = (EM_TASK * )LIST_VALUE(cur_node);
	
	if((steps==0)||(task_id == p_task->_task_info->_task_id))
	{
		/* do nothing */
		return SUCCESS;
	}
	
	before_node=LIST_NEXT(cur_node);
	while(cur_node!=LIST_END(g_dt_mgr._dling_task_order))
	{
		p_task = (EM_TASK * )LIST_VALUE(cur_node);
		if(task_id == p_task->_task_info->_task_id)
		{
			ret_val= em_list_insert(&g_dt_mgr._dling_task_order, (void *)p_task, before_node);
			sd_assert(ret_val==SUCCESS);
			if(ret_val==SUCCESS)
			{
				ret_val= em_list_erase(&g_dt_mgr._dling_task_order, cur_node);
				sd_assert(ret_val==SUCCESS);
				g_dt_mgr._dling_order_changed = TRUE;
				return ret_val;
			}
			else
			{
				return ret_val;
			}
		}
		else
		{
			if(i>=steps)
			{
				before_node = LIST_PRE(before_node);
			}
		}

		/* 如果是vod任务,跳过*/
		if(!dt_is_vod_task(p_task))
		{
			i++;
		}
		
		cur_node = LIST_PRE(cur_node);
	}
	
	return INVALID_TASK_ID;
}



_int32 dt_get_task_id_by_state_impl(TASK_STATE state,_u32 * id_array_buffer,_u32 *buffer_len,BOOL is_local)
{
	_int32 ret_val = SUCCESS;
      MAP_ITERATOR  cur_node = NULL; 
	 EM_TASK * p_task = NULL;
	 _u32 i=0;

      EM_LOG_DEBUG("dt_get_task_id_by_state_impl:%d",state); 

	if((state==TS_TASK_RUNNING)&&(0==dt_get_running_task_num()))
	{
		*buffer_len =0;
		return ret_val;
	}
	  
      cur_node = EM_MAP_BEGIN(g_dt_mgr._all_tasks);
      while(cur_node != EM_MAP_END(g_dt_mgr._all_tasks))
      {
             p_task = (EM_TASK *)EM_MAP_VALUE(cur_node);
	      if(state==dt_get_task_state(p_task))
          {
              if((i<*buffer_len)&&(id_array_buffer!=NULL))
              {
                  id_array_buffer[i]=p_task->_task_info->_task_id;
              }
             // else
             // {
             //     ret_val = NOT_ENOUGH_BUFFER;
             // }
              
              if(!dt_is_vod_task(p_task))
              {
                  /* 不是vod任务*/
                  if(is_local)
                  {
                      /* 只统计本地下载任务 */
                      if(dt_is_local_task(p_task))
                      {
                          i++;
                      }
                  }
                  else
                  {
                      i++;
                  }
              }
          }
	      cur_node = EM_MAP_NEXT(g_dt_mgr._all_tasks, cur_node);
      }
    if(*buffer_len < i)
    {
        ret_val = NOT_ENOUGH_BUFFER;
    }
	*buffer_len = i;
	return ret_val;
}
	
_int32 dt_get_all_task_ids_impl(_u32 * id_array_buffer,_u32 *buffer_len)
{
	_int32 ret_val = SUCCESS;
      MAP_ITERATOR  cur_node = NULL; 
	 EM_TASK * p_task = NULL;
	 _u32 i=0;

      EM_LOG_DEBUG("dt_get_all_task_ids_impl"); 
	  if((*buffer_len < em_map_size(&(g_dt_mgr._all_tasks))-dt_get_vod_task_num())||(id_array_buffer==NULL))
	  {
		*buffer_len = em_map_size(&(g_dt_mgr._all_tasks))-dt_get_vod_task_num();
		return NOT_ENOUGH_BUFFER;
	  }

	  cur_node = EM_MAP_BEGIN(g_dt_mgr._all_tasks);
      while(cur_node != EM_MAP_END(g_dt_mgr._all_tasks))
      {
             p_task = (EM_TASK *)EM_MAP_VALUE(cur_node);
			 
	/* 如果是无盘vod任务，直接跳过 */
		if(!dt_is_vod_task(p_task))
		{
			if(i < *buffer_len)
			{
				id_array_buffer[i]=p_task->_task_info->_task_id;
				i++;
			}
			else
			{
				ret_val = NOT_ENOUGH_BUFFER;
			}
		}
	      cur_node = EM_MAP_NEXT(g_dt_mgr._all_tasks, cur_node);
      }

	*buffer_len = em_map_size(&(g_dt_mgr._all_tasks))-dt_get_vod_task_num();
	return ret_val;
}

_int32 dt_get_all_task_total_file_size_impl(_u64 * p_total_file_size)
{
	_int32 ret_val = SUCCESS;
      MAP_ITERATOR  cur_node = NULL; 
	 EM_TASK * p_task = NULL;
	// _u32 i=0;
	*p_total_file_size = 0;

      EM_LOG_DEBUG("dt_get_all_task_total_file_size_impl"); 

	  cur_node = EM_MAP_BEGIN(g_dt_mgr._all_tasks);
      while(cur_node != EM_MAP_END(g_dt_mgr._all_tasks))
      {
             p_task = (EM_TASK *)EM_MAP_VALUE(cur_node);
			*p_total_file_size += p_task->_task_info->_file_size; 
	
			cur_node = EM_MAP_NEXT(g_dt_mgr._all_tasks, cur_node);
      }
	  
	return ret_val;
}

_int32 dt_get_all_task_need_space_impl(_u64 * p_need_space)
{
	_int32 ret_val = SUCCESS;
      MAP_ITERATOR  cur_node = NULL; 
	 EM_TASK * p_task = NULL;
	// _u32 i=0;
	*p_need_space = 0;

      EM_LOG_DEBUG("dt_get_all_task_need_space_impl"); 

	  cur_node = EM_MAP_BEGIN(g_dt_mgr._all_tasks);
      while(cur_node != EM_MAP_END(g_dt_mgr._all_tasks))
      {
             p_task = (EM_TASK *)EM_MAP_VALUE(cur_node);


		/* 普通任务开始下载后会把整个文件创建了 */
		if(p_task->_task_info->_downloaded_data_size > 0)
		{
			*p_need_space += 0;
		}
		else
		{
			*p_need_space += p_task->_task_info->_file_size;
		}
	
	      cur_node = EM_MAP_NEXT(g_dt_mgr._all_tasks, cur_node);
      }
	  
	return ret_val;
}

//////////////////////////////////

BOOL dt_is_lan_task(EM_TASK * p_task)
{
	if(p_task->_task_info->_type == TT_LAN)
		return TRUE;
	return FALSE;
}

_int32 dt_increase_running_lan_task_num(void)
{
	g_dt_mgr._lan_running_task_num++;
	return SUCCESS;
}
_int32 dt_decrease_running_lan_task_num(void)
{
	g_dt_mgr._lan_running_task_num--;
	return SUCCESS;
}

_u32 dt_get_running_lan_task_num( void )
{
	return g_dt_mgr._lan_running_task_num;
}

_int32 dt_increase_waiting_lan_task_num(void)
{
	g_dt_mgr._lan_waiting_task_num++;
	return SUCCESS;
}
_int32 dt_decrease_waiting_lan_task_num(void)
{
	g_dt_mgr._lan_waiting_task_num--;
	return SUCCESS;
}

_u32 dt_get_waiting_lan_task_num( void )
{
	return g_dt_mgr._lan_waiting_task_num;
}

//////////////////////////////////
_int32 dt_destroy_vod_task( EM_TASK * p_task )
{
	_int32 ret_val = SUCCESS;
	ret_val = dt_destroy_task_impl(p_task,TRUE);
	return ret_val;
}


BOOL dt_is_vod_task(EM_TASK * p_task)
{
	/* vod 任务被设置为下载模式后也属于普通下载任务 */
	if(p_task->_task_info->_task_id>MAX_DL_TASK_ID && !p_task->_download_mode._is_download)
		return TRUE;
	return FALSE;
}
BOOL dt_is_vod_task_no_disk(EM_TASK * p_task)
{
	return p_task->_task_info->_check_data;
}
_u32 dt_create_vod_task_id(void)
{
	_u32 task_id = dt_create_task_id(FALSE);
	/* 点播任务id总大于MAX_DL_TASK_ID */
	return task_id+MAX_DL_TASK_ID;
}
_int32 dt_decrease_vod_task_id(void)
{
	return dt_decrease_task_id();
}
_int32 dt_increase_vod_task_num(EM_TASK * p_task)
{
	g_dt_mgr._vod_task_num++;
	dt_increase_used_vod_cache_size(p_task);
	return SUCCESS;
}
_int32 dt_decrease_vod_task_num(EM_TASK * p_task)
{
	g_dt_mgr._vod_task_num--;
	dt_decrease_used_vod_cache_size(p_task);
	return SUCCESS;
}
_int32 dt_reset_vod_task_num(void)
{
	g_dt_mgr._vod_task_num = 0;
	dt_reset_used_vod_cache_size();
	return SUCCESS;
}
_u32 dt_get_vod_task_num( void )
{
	return g_dt_mgr._vod_task_num;
}

_int32 dt_increase_running_vod_task_num(void)
{
	g_dt_mgr._vod_running_task_num++;
	return SUCCESS;
}
_int32 dt_decrease_running_vod_task_num(void)
{
	g_dt_mgr._vod_running_task_num--;
	return SUCCESS;
}

_u32 dt_get_running_vod_task_num( void )
{
	return g_dt_mgr._vod_running_task_num;
}
_int32 dt_increase_used_vod_cache_size(EM_TASK * p_task)
{
	if(!dt_is_vod_task_no_disk(p_task))
	{
		g_dt_mgr._used_vod_cache_size+=(p_task->_task_info->_file_size/1024);
	}
	return SUCCESS;
}
_int32 dt_decrease_used_vod_cache_size(EM_TASK * p_task)
{
	if(!dt_is_vod_task_no_disk(p_task))
	{
		sd_assert(g_dt_mgr._used_vod_cache_size>=(p_task->_task_info->_file_size/1024));
		g_dt_mgr._used_vod_cache_size-=(p_task->_task_info->_file_size/1024);
	}
	return SUCCESS;
}
_int32 dt_reset_used_vod_cache_size(void)
{
	g_dt_mgr._used_vod_cache_size = 0;
	return SUCCESS;
}
_u32 dt_get_used_vod_cache_size( void )
{
	return g_dt_mgr._used_vod_cache_size;
}

_int32  dt_set_vod_cache_size_impl( _u32 cache_size )
{
	em_settings_set_int_item("system.vod_cache_size", cache_size);
	g_dt_mgr._max_vod_cache_size = cache_size;
	return SUCCESS;
}
_u32  dt_get_vod_cache_size_impl( void )
{
	_u32 cache_size = g_dt_mgr._max_vod_cache_size;

	if(cache_size == 0)
	{
		em_settings_get_int_item("system.vod_cache_size", (_int32 *)&cache_size);
	}
	return cache_size;
}

BOOL dt_check_enough_vod_cache_free_size( void )
{
	_u64 free_size = 0;
	_u32 unused_cache_size = 0;
	char* cache_path = dt_get_vod_cache_path_impl();

	if(cache_path==NULL) return TRUE;
	
	sd_get_free_disk(cache_path, &free_size);
	if(free_size<MIN_FREE_DISK_SIZE)  //100MB
	{
		/* 磁盘剩余空间不足10M */
		return FALSE;
	}
	
	if(dt_get_vod_cache_size_impl() > dt_get_used_vod_cache_size())
	{
		unused_cache_size = dt_get_vod_cache_size_impl() - dt_get_used_vod_cache_size();
	}

	if(unused_cache_size==0)
	{
		/* VOD CACHE 空间不足 */
		return FALSE;
	}
	
	return TRUE;
}
	
_int32 dt_clear_vod_cache_impl(BOOL clear_all)
{
	//_int32 ret_val = SUCCESS;
	MAP_ITERATOR  cur_node = NULL; 
	EM_TASK *p_task = NULL,*p_oldest_task = NULL;
	_u32 time_stamp = MAX_U32,vod_task_num = 0,running_vod_task_num = 0,old_task_num = 0;
	BOOL find_oldest = FALSE;
	_u32 current_time = 0;
	em_time(&current_time);

	if(dt_get_vod_task_num()==0)  return SUCCESS;
	
	/*	if((clear_all)||(!dt_check_enough_vod_cache_free_size()))*/
	{
		EM_LOG_DEBUG("dt_clear_vod_cache_impl:clear_all=%d,all_tasks_num=%u,max_vod_cache_size=%u,used_vod_cache_size=%u",clear_all,em_map_size(&g_dt_mgr._all_tasks),dt_get_vod_cache_size_impl(),dt_get_used_vod_cache_size()); 
	    cur_node = EM_MAP_BEGIN(g_dt_mgr._all_tasks);
	    while(cur_node != EM_MAP_END(g_dt_mgr._all_tasks))
	    {
			p_task = (EM_TASK *)EM_MAP_VALUE(cur_node);
		    if((p_task->_task_info->_task_id > MAX_DL_TASK_ID) && (dt_get_task_state(p_task)!=TS_TASK_RUNNING) && (dt_get_task_state(p_task)!=TS_TASK_WAITING))
		    {
				if(!p_task->_download_mode._is_download || current_time - p_task->_download_mode._set_timestamp > p_task->_download_mode._file_retain_time)
				{
					if(p_task->_inner_id==0)
		      		{
		      			// not running vod task
      					EM_LOG_DEBUG("dt_clear_vod_cache_impl 1:task_id=%u,type=%d,state=%d,start_time=%u,file_size=%llu",p_task->_task_info->_task_id,dt_get_task_type(p_task),dt_get_task_state(p_task),p_task->_task_info->_start_time,p_task->_task_info->_file_size); 
						sd_assert(dt_is_vod_task_no_disk(p_task)==FALSE);
						//sd_assert(p_task->_task_info->_start_time!=0);
						if(clear_all)
						{
							cur_node = EM_MAP_NEXT(g_dt_mgr._all_tasks, cur_node);
      						EM_LOG_DEBUG("dt_clear_vod_cache_impl 2,dt_destroy_vod_task:task_id=%u",p_task->_task_info->_task_id); 
							dt_destroy_vod_task( p_task );
							continue;
						}
						else
						{
				      		if(p_task->_task_info->_start_time<time_stamp)
			      			{
  								EM_LOG_DEBUG("dt_clear_vod_cache_impl 3:find a old task:task_id=%u",p_task->_task_info->_task_id); 
			      				time_stamp = p_task->_task_info->_start_time;
								p_oldest_task = p_task;
								find_oldest = TRUE;
			      			}
							old_task_num++;
						}
			  		}
					else
					{
      					EM_LOG_DEBUG("dt_clear_vod_cache_impl 4,find a running vod task:task_id=%u,type=%d,state=%d,start_time=%u,file_size=%llu",p_task->_task_info->_task_id,dt_get_task_type(p_task),dt_get_task_state(p_task),p_task->_task_info->_start_time,p_task->_task_info->_file_size); 
						sd_assert(dt_get_task_state(p_task)!=TS_TASK_WAITING);
						running_vod_task_num++;
					}
				}
		      		vod_task_num++;
			}
		    cur_node = EM_MAP_NEXT(g_dt_mgr._all_tasks, cur_node);
		}

		/* 找到了一个最旧的任务*/
		if((find_oldest)&&(old_task_num>=1))
		{
			/* 每次调用这个函数只删除一个旧任务*/
			EM_LOG_ERROR("dt_clear_vod_cache_impl 5,dt_destroy oldest vod_task:task_id=%u",p_oldest_task->_task_info->_task_id); 
			dt_destroy_vod_task( p_oldest_task );
			if(old_task_num>=2)
			{
				dt_clear_vod_cache_impl(clear_all);
			}
		}
		//sd_assert(vod_task_num==dt_get_vod_task_num());
	}

	return SUCCESS;
}

_int32 dt_load_task_vod_extra_mode(void)
{
	EM_TASK * p_task = NULL;
	_u8 * user_data = NULL;
	_u8 *download_mode = NULL;
	BOOL is_ad = FALSE;
#ifdef ENABLE_HSC
	_u8 *hsc_mode = NULL;
#endif /* ENABLE_HSC */
	MAP_ITERATOR  cur_node = NULL;
	_int32 ret_val = SUCCESS;

	cur_node = EM_MAP_BEGIN(g_dt_mgr._all_tasks);
	while(cur_node != EM_MAP_END(g_dt_mgr._all_tasks))
	{
		p_task = (EM_TASK *)EM_MAP_VALUE(cur_node);
		
		ret_val = dt_is_ad_task_impl(p_task, &is_ad);
		if(ret_val == SUCCESS)
		{
			p_task->_is_ad = is_ad;
		}
		else
		{
			p_task->_is_ad = FALSE;
		}
		if((p_task->_task_info->_user_data_len != 0)
			#ifndef ENABLE_HSC
			&&(p_task->_task_info->_task_id > MAX_DL_TASK_ID)
			#endif /* ENABLE_HSC */
			)
		{
			ret_val = em_malloc(p_task->_task_info->_user_data_len, (void**)&user_data);
			CHECK_VALUE(ret_val);
			ret_val = dt_get_task_user_data_impl(p_task, user_data, p_task->_task_info->_user_data_len);
			if(ret_val == SUCCESS)
			{
				/* 如果是vod任务，设置download mode*/
				if(p_task->_task_info->_task_id > MAX_DL_TASK_ID)
				{
					ret_val = dt_vod_get_download_mode_impl(user_data, p_task->_task_info->_user_data_len, &download_mode);
					if(ret_val == SUCCESS)
					{
						em_memcpy(&p_task->_download_mode, download_mode, sizeof(VOD_DOWNLOAD_MODE));
						if(p_task->_download_mode._is_download)
						{
							dt_decrease_vod_task_num(p_task);
						}
					}
				}

#ifdef ENABLE_HSC
				/* 获取高速通道模式 */
				ret_val = dt_get_hsc_mode_impl(user_data, p_task->_task_info->_user_data_len, &hsc_mode);
				if(ret_val == SUCCESS)
				{
					em_memcpy(&p_task->_use_hsc, hsc_mode, sizeof(BOOL));
				}
#endif /* ENABLE_HSC */
			}
			EM_SAFE_DELETE(user_data);
		}
		cur_node = EM_MAP_NEXT(g_dt_mgr._all_tasks, cur_node);
	}
	return SUCCESS;
}

/* 纠正下载任务的路径，ios覆盖安装后下载路径会变动，导致任务原先存储的路径不正确(重新启动后会设置新路径进来)，重新纠正
   另外，单独设置了缓存路径的话，需要另外重新纠正。
*/
_int32 dt_correct_all_tasks_path(void)
{
	EM_TASK * p_task = NULL;
	MAP_ITERATOR  cur_node = NULL;
	_int32 ret_val = SUCCESS;
	char * path = NULL;
	
#ifndef MACOS
	return SUCCESS;
#endif /* MACOS */

	if(0==em_map_size(&g_dt_mgr._all_tasks))
	{
		/*  将文件夹下的所有文件删除 */
		ret_val = sd_delete_dir(dt_get_vod_cache_path_impl( ));
		if(ret_val == SUCCESS)
		{
			/* 重新创建该文件夹 */
			ret_val = sd_mkdir(dt_get_vod_cache_path_impl( ));
			sd_assert(ret_val==SUCCESS);
		}
	}

	cur_node = EM_MAP_BEGIN(g_dt_mgr._all_tasks);
	while(cur_node != EM_MAP_END(g_dt_mgr._all_tasks))
	{
		p_task = (EM_TASK *)EM_MAP_VALUE(cur_node);

		path = dt_get_task_file_path_from_file(p_task);
		if(path)
		{
		ret_val = dt_correct_task_path(p_task, path,  p_task->_task_info->_file_path_len);
		}
		else
		{
			sd_assert(FALSE);
		}
		sd_assert(ret_val==SUCCESS);
		cur_node = EM_MAP_NEXT(g_dt_mgr._all_tasks, cur_node);
	}
	return SUCCESS;
}


BOOL dt_can_failed_task_restart(EM_TASK* p_task)
{
	TASK_INFO *task_info = NULL;
	EM_BT_TASK *bt_task = NULL;
	_int32 i = 0;
	_u16 *bt_need_dl = 0;
	BT_FILE *bt_file;

	task_info = p_task->_task_info;
	if (TS_TASK_FAILED != task_info->_state)
	{
		EM_LOG_ERROR("task %u not failed, but called dt_can_failed_task_restart", task_info->_task_id);
		return FALSE;
	}

	if (dt_is_vod_task(p_task))
	{
		EM_LOG_DEBUG("failed task %u is vod task, do not need restart", task_info->_task_id);
		return FALSE;
	}

	if (130 == task_info->_failed_code ||
		DATA_CANNOT_CORRECT_ERROR == task_info->_failed_code)
	{
		EM_LOG_DEBUG("failed task %u fail code is %u", task_info->_task_id, task_info->_failed_code);
		return TRUE;
	}

	if (EMEC_NO_RESOURCE == (task_info->_failed_code & EMEC_NO_RESOURCE))
	{
		EM_LOG_DEBUG("failed task %u fail code is %u", task_info->_task_id, task_info->_failed_code);
		return TRUE;
	}

	if (TT_BT == task_info->_type && BT_SUB_FILE_FAILED == task_info->_failed_code)
	{
		if (task_info->_full_info)
		{
			// 从内存获取子任务错误码
			bt_task = (EM_BT_TASK*)task_info;
			for (i=0;i<task_info->_url_len_or_need_dl_num;i++)
			{
				if (BT_COMMON_FILE_TOO_SLOW == bt_task->_file_array[i]._failed_code ||
					131 == bt_task->_file_array[i]._failed_code ||
					DATA_CANNOT_CORRECT_ERROR == bt_task->_file_array[i]._failed_code || 
					BT_DATA_MANAGER_CANNOT_CORRECT == bt_task->_file_array[i]._failed_code)
				{
					EM_LOG_DEBUG("failed bt task %u sub file %u fail code is %u", 
						task_info->_task_id, i, bt_task->_file_array[i]._failed_code);
					return TRUE;
				}
			}
		}
		else
		{
			// 从文件获取子任务错误码
			bt_need_dl = dt_get_task_bt_need_dl_file_index_array(p_task);
			sd_assert(bt_need_dl != NULL);

			for (i=0;i<p_task->_task_info->_url_len_or_need_dl_num;i++)
			{
				bt_file = dt_get_task_bt_sub_file_from_file(p_task, i);
				if (NULL == bt_file)
					continue;

				if (BT_COMMON_FILE_TOO_SLOW == bt_file->_failed_code||
					131 == bt_file->_failed_code ||
					DATA_CANNOT_CORRECT_ERROR == bt_file->_failed_code || 
					BT_DATA_MANAGER_CANNOT_CORRECT == bt_file->_failed_code)
				{
					EM_LOG_DEBUG("failed bt task %u sub file %u fail code is %u", 
						task_info->_task_id, i, bt_file->_failed_code);
					em_free(bt_need_dl);
					return TRUE;					
				}
			}

			em_free(bt_need_dl);
		}

		EM_LOG_DEBUG("failed bt task %u do not need restart", task_info->_task_id);
	}

	EM_LOG_DEBUG("failed task %u fail code is %u, do not restart", task_info->_task_id, task_info->_failed_code);
	return FALSE;
}

_int32 dt_report_to_remote(void)
{
	EM_LOG_DEBUG("dt_report_to_remote:g_need_report_to_remote=%d", g_need_report_to_remote);
	if (g_need_report_to_remote)
	{
#ifdef ENABLE_RC
		rc_report_task_process();
#endif /* ENABLE_RC */
		g_need_report_to_remote = FALSE;
	}

	return SUCCESS;
}

/* 在所有任务中查找是否有相同cid的任务存在 */
BOOL dt_is_same_cid_task_exist(char * tcid)
{
	_int32 ret_val = SUCCESS;
      MAP_ITERATOR  cur_node = NULL; 
	 EM_TASK * p_task = NULL;
	char cid_buffer[64] = {0};

      for(cur_node = EM_MAP_BEGIN(g_dt_mgr._all_tasks);cur_node != EM_MAP_END(g_dt_mgr._all_tasks);cur_node = EM_MAP_NEXT(g_dt_mgr._all_tasks, cur_node))
      {
             p_task = (EM_TASK *)EM_MAP_VALUE(cur_node);
		if(p_task->_task_info->_type == TT_TCID||p_task->_task_info->_type == TT_LAN || p_task->_task_info->_have_tcid)
		{
			sd_memset( cid_buffer,0x00,64);
			ret_val = dt_get_task_tcid_impl(p_task,cid_buffer);
			if((ret_val==SUCCESS)&&(sd_stricmp(cid_buffer, tcid)==0))
			{
				return TRUE;
			}
		}
      }

	return FALSE;

}

