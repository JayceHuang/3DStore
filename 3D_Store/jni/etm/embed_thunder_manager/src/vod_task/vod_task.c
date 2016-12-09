#if 1
#include "vod_task/vod_task.h"
#include "scheduler/scheduler.h"

#include "et_interface/et_interface.h"
#include "torrent_parser/torrent_parser_interface.h"
#include "download_manager/download_task_inner.h"

#include "em_asyn_frame/em_msg.h"
#include "em_common/em_utility.h"
#include "em_common/em_mempool.h"
#include "em_interface/em_fs.h"
#include "em_common/em_time.h"
#include "em_common/em_logid.h"
#ifdef EM_LOGID
#undef EM_LOGID
#endif

#define EM_LOGID EM_LOGID_VOD_MANAGER

#include "em_common/em_logger.h"


static VOD_MANAGER g_vod_manager;
static SLAB *gp_vod_task_slab = NULL;		//VOD_TASK

/*--------------------------------------------------------------------------*/
/*                Interfaces  				        */
/*--------------------------------------------------------------------------*/

_int32 init_vod_module(void)
{
	_int32 ret_val = SUCCESS;
	EM_LOG_DEBUG( "init_vod_module" );

	em_memset(&g_vod_manager, 0, sizeof(VOD_MANAGER));
	em_map_init(&g_vod_manager._all_tasks, vod_id_comp);
	g_vod_manager._total_task_num = MAX_DL_TASK_ID;
	em_settings_get_str_item("system.vod_cache_path", g_vod_manager._cache_path);
	em_settings_get_int_item("system.vod_cache_size", (_int32 *)&g_vod_manager._max_cache_size);
	
	sd_assert(gp_vod_task_slab==NULL);
	if(gp_vod_task_slab==NULL)
	{
		ret_val = em_mpool_create_slab(sizeof(VOD_TASK), MIN_VOD_TASK_MEMORY, 0, &gp_vod_task_slab);
		CHECK_VALUE(ret_val);
	}

	return ret_val;
}
_int32 uninit_vod_module(void)
{
	EM_LOG_DEBUG( "uninit_vod_module" );

	vod_clear_tasks();
	
	if(gp_vod_task_slab!=NULL)
	{
		em_mpool_destory_slab(gp_vod_task_slab);
		gp_vod_task_slab=NULL;
	}
	
	return SUCCESS;
}

_int32 vod_task_malloc(VOD_TASK **pp_slip)
{
	_int32 ret_val = em_mpool_get_slip( gp_vod_task_slab, (void**)pp_slip );
	
	if(ret_val==SUCCESS) 
		em_memset(*pp_slip,0,sizeof(VOD_TASK));
	
      EM_LOG_DEBUG("vod_task_malloc"); 
	return ret_val;
}

_int32 vod_task_free(VOD_TASK * p_slip)
{
	sd_assert( p_slip != NULL );
	if( p_slip == NULL ) return SUCCESS;

      //EM_LOG_DEBUG("vod_task_free,task_id=%u",p_slip->_task_id); 
	return em_mpool_free_slip( gp_vod_task_slab, (void*)p_slip );
}

_int32 vod_id_comp(void *E1, void *E2)
{
	//return ((_int32)E1) -((_int32)E2);
	_u32 id1 = (_u32)E1,id2 = (_u32)E2;
	_int32 ret_val = 0;
	
	if(id1>VOD_NODISK_TASK_MASK)
		id1-=VOD_NODISK_TASK_MASK;
	
	if(id2>VOD_NODISK_TASK_MASK)
		id2-=VOD_NODISK_TASK_MASK;
	ret_val =  ((_int32)id1) -((_int32)id2);
	return ret_val;
}

_u32 vod_get_task_num( void )
{
	return em_map_size(&g_vod_manager._all_tasks);
}

VOD_TASK * vod_get_task_from_map(_u32 task_id)
{
	_int32 ret_val = SUCCESS;
	VOD_TASK * p_task=NULL;

      EM_LOG_DEBUG("vod_get_task_from_map:task_id=%u",task_id); 
	ret_val=em_map_find_node(&g_vod_manager._all_tasks, (void *)task_id, (void **)&p_task);
	sd_assert(ret_val==SUCCESS);
	//sd_assert(p_task!=NULL);
	return p_task;
}
_int32 vod_add_task_to_map(VOD_TASK * p_task)
{
	_int32 ret_val = SUCCESS;
	PAIR info_map_node;
      EM_LOG_DEBUG("vod_add_task_to_map:task_id=%u",p_task->_task_id); 
	info_map_node._key =(void*) p_task->_task_id;
	info_map_node._value = (void*)p_task;
	ret_val = em_map_insert_node( &g_vod_manager._all_tasks, &info_map_node );
	CHECK_VALUE(ret_val);
	return SUCCESS;
}

_int32 vod_remove_task_from_map(VOD_TASK * p_task)
{
      EM_LOG_DEBUG("vod_remove_task_from_map:task_id=%u",p_task->_task_id); 
	return em_map_erase_node(&g_vod_manager._all_tasks, (void*)p_task->_task_id);
}
_int32 vod_clear_tasks(void)
{
      MAP_ITERATOR  cur_node = NULL; 
	 VOD_TASK * p_task = NULL;

      EM_LOG_DEBUG("vod_clear_tasks:%u",em_map_size(&g_vod_manager._all_tasks)); 
	  
      cur_node = EM_MAP_BEGIN(g_vod_manager._all_tasks);
      while(cur_node != EM_MAP_END(g_vod_manager._all_tasks))
      {
             p_task = (VOD_TASK *)EM_MAP_VALUE(cur_node);
		vod_report_impl( p_task->_task_id);
		eti_stop_task(p_task->_inner_id);
		eti_delete_task(p_task->_inner_id);
	      vod_task_free(p_task);
		em_map_erase_iterator(&g_vod_manager._all_tasks, cur_node);
	      cur_node = EM_MAP_BEGIN(g_vod_manager._all_tasks);
      }
	return SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

_int32 vod_create_task( void * p_param )
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	_int32 ret_val = SUCCESS;
	EM_CREATE_VOD_TASK * p_create_param =(EM_CREATE_VOD_TASK *)_p_param->_para1;
	_u32* p_task_id =(_u32 *)_p_param->_para2;
	_u32 inner_id = 0;
	VOD_TASK * p_task= NULL;
	EM_EIGENVALUE eigenvalue = {0};

	EM_LOG_DEBUG("vod_create_task:%d",p_create_param->_type);

	if(em_is_net_ok(TRUE)==FALSE)
	{
		ret_val = NETWORK_NOT_READY;
		goto ErrorHanle;
	}
	sd_assert(p_create_param->_is_no_disk==TRUE);

	if(p_create_param->_type==TT_BT)
	{
		ret_val = vod_create_bt_task( p_create_param ,&inner_id);
	}
	else
	if(p_create_param->_type==TT_FILE)
	{
		ret_val = INVALID_TASK_TYPE;
	}
	else
	{
		ret_val = vod_create_p2sp_task( p_create_param ,&inner_id);
	}

	if(ret_val!=SUCCESS) goto ErrorHanle;

	//if(p_create_param->_is_no_disk)
	{
		ret_val = eti_set_task_no_disk(inner_id);
		if(ret_val!=SUCCESS)
		{
			eti_delete_task(inner_id);
			goto ErrorHanle;
		}
	}
	
	if(p_create_param->_check_data)
	{
		ret_val = eti_vod_set_task_check_data(inner_id);
		if(ret_val!=SUCCESS)
		{
			eti_delete_task(inner_id);
			goto ErrorHanle;
		}
	}
	
	ret_val = eti_start_task(inner_id);
	if(ret_val!=SUCCESS)
	{
		eti_delete_task(inner_id);
		goto ErrorHanle;
	}
	
	///////////////////////////////////////
	ret_val = vod_task_malloc(&p_task);
	if(ret_val!=SUCCESS)
	{
		eti_stop_task(inner_id);
		eti_delete_task(inner_id);
		goto ErrorHanle;
	}
	g_vod_manager._total_task_num++;
	sd_assert(g_vod_manager._total_task_num!=-1);
	if(g_vod_manager._total_task_num==-1)
	{
		g_vod_manager._total_task_num = MAX_DL_TASK_ID+1;
	}
	p_task->_task_id = inner_id+VOD_NODISK_TASK_MASK;//g_vod_manager._total_task_num;
	p_task->_type = p_create_param->_type;
	p_task->_check_data = p_create_param->_check_data;
	p_task->_is_no_disk= p_create_param->_is_no_disk;
	p_task->_inner_id = inner_id;

	eigenvalue._type = p_task->_type;
	switch(eigenvalue._type)
	{
		case TT_URL:
		case TT_EMULE:
			eigenvalue._url = p_create_param->_url;
			eigenvalue._url_len = p_create_param->_url_len;
			break;
		case TT_TCID:
		case TT_LAN:
			sd_strncpy(eigenvalue._eigenvalue,p_create_param->_tcid, CID_SIZE*2);
			break;
		case TT_KANKAN:
			sd_strncpy(eigenvalue._eigenvalue,p_create_param->_gcid, CID_SIZE*2);
			break;
		case TT_BT:
		case TT_FILE:
		default:
			sd_assert(FALSE);
	}
	vod_generate_eigenvalue(&eigenvalue,p_task->_eigenvalue);

	ret_val = vod_add_task_to_map(p_task);
	if(ret_val!=SUCCESS)
	{
		eti_stop_task(inner_id);
		eti_delete_task(inner_id);
		vod_task_free(p_task);
		g_vod_manager._total_task_num--;
		goto ErrorHanle;
	}
	
	*p_task_id=p_task->_task_id;
	
	EM_LOG_DEBUG("vod_create_task %u success!em_signal_sevent_handle:_result=%d",*p_task_id,_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
	
ErrorHanle:
	_p_param->_result = ret_val;
	EM_LOG_DEBUG("vod_create_task failed! em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));

}


_int32 vod_stop_task( void * p_param )
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	_u32 task_id=(_u32)_p_param->_para1;
	_u32 inner_id = 0;
	VOD_TASK * p_task = NULL;

	EM_LOG_DEBUG("vod_stop_task:%u",task_id);

	if(task_id>VOD_NODISK_TASK_MASK)
	{
		if(em_is_net_ok(FALSE)==FALSE)
		{
			_p_param->_result = NETWORK_NOT_READY;
			goto ErrorHanle;
		}
		
		p_task = vod_get_task_from_map(task_id);
		if(p_task==NULL) 
		{
			_p_param->_result = INVALID_TASK_ID;
			goto ErrorHanle;
		}
		
		inner_id = p_task->_inner_id;
		
		vod_report_impl( task_id);
		
		eti_stop_task(inner_id);
		eti_delete_task(inner_id);
		vod_remove_task_from_map(p_task);
		vod_task_free(p_task);
	}
	else
	{
		/* do nothing */
		//sd_assert(FALSE);
		_p_param->_result = SUCCESS;
	}

	
ErrorHanle:

	EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 vod_read_file( void * p_param )
{
	POST_PARA_5* _p_param = (POST_PARA_5*)p_param;
	_u32 task_id=(_u32)_p_param->_para1;
	_u64 * start_pos = (_u64*)_p_param->_para2;
	_u64 * len=(_u64*)_p_param->_para3;
	char * buffer=(char *)_p_param->_para4;
	_u32 block_time=(_u32)_p_param->_para5;
	_u32 inner_id = 0;
	VOD_TASK * p_task = NULL;

	EM_LOG_DEBUG("vod_read_file:%u",task_id);

	if(em_is_net_ok(FALSE)==FALSE)
	{
		_p_param->_result = NETWORK_NOT_READY;
		goto ErrorHanle;
	}
		
	if(task_id>VOD_NODISK_TASK_MASK)
	{
		p_task = vod_get_task_from_map(task_id);
		if(p_task==NULL) 
		{
			_p_param->_result = INVALID_TASK_ID;
			goto ErrorHanle;
		}
		
		inner_id = p_task->_inner_id;
		_p_param->_result=eti_vod_read_file((_int32)inner_id, *start_pos,*len, buffer, (_int32)block_time );
	}
	else
	{
		inner_id =  dt_get_task_inner_id(  task_id );
		_p_param->_result=eti_vod_read_file((_int32)inner_id, *start_pos,*len, buffer, (_int32)block_time );
	}

	
ErrorHanle:

	EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 vod_read_bt_file( void * p_param )
{
	POST_PARA_6* _p_param = (POST_PARA_6*)p_param;
	_u32 task_id=(_u32)_p_param->_para1;
	_u64 * start_pos = (_u64*)_p_param->_para2;
	_u64 * len=(_u64*)_p_param->_para3;
	char * buffer=(char *)_p_param->_para4;
	_u32 block_time=(_u32)_p_param->_para5;
	_u32 file_index=(_u32)_p_param->_para6;
	_u32 inner_id = 0;
	VOD_TASK * p_task = NULL;

	EM_LOG_DEBUG("vod_read_file:%u",task_id);

	if(em_is_net_ok(FALSE)==FALSE)
	{
		_p_param->_result = NETWORK_NOT_READY;
		goto ErrorHanle;
	}
		
	if(task_id>VOD_NODISK_TASK_MASK)
	{
		p_task = vod_get_task_from_map(task_id);
		if(p_task==NULL) 
		{
			_p_param->_result = INVALID_TASK_ID;
			goto ErrorHanle;
		}
		
		inner_id = p_task->_inner_id;
		_p_param->_result=eti_vod_bt_read_file((_int32) inner_id,  file_index, *start_pos,  *len, buffer, (_int32) block_time );
	}
	else
	{
		inner_id =  dt_get_task_inner_id(  task_id );
		_p_param->_result=eti_vod_bt_read_file((_int32) inner_id,  file_index, *start_pos,  *len, buffer, (_int32) block_time );
	}

	
ErrorHanle:

	EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 vod_get_buffer_percent( void * p_param )
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	_u32 task_id=(_u32)_p_param->_para1;
	_u32 * percent=(_u32 *)_p_param->_para2;
	_u32 inner_id = 0;
	VOD_TASK * p_task = NULL;

	EM_LOG_DEBUG("vod_get_buffer_percent:%u",task_id);

	if(em_is_net_ok(FALSE)==FALSE)
	{
		_p_param->_result = NETWORK_NOT_READY;
		goto ErrorHanle;
	}
		
	if(task_id>VOD_NODISK_TASK_MASK)
	{
		p_task = vod_get_task_from_map(task_id);
		if(p_task==NULL) 
		{
			_p_param->_result = INVALID_TASK_ID;
			goto ErrorHanle;
		}
		
		inner_id = p_task->_inner_id;
		_p_param->_result=eti_vod_get_buffer_percent((_int32)inner_id, (_int32 *)percent );
	}
	else
	{
		inner_id =  dt_get_task_inner_id(  task_id );
		_p_param->_result=eti_vod_get_buffer_percent((_int32)inner_id, (_int32 *)percent );
	}

	
ErrorHanle:

	EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 vod_is_download_finished( void * p_param )
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	_u32 task_id=(_u32)_p_param->_para1;
	BOOL * finished=(BOOL *)_p_param->_para2;
	_u32 inner_id = 0;
	VOD_TASK * p_task = NULL;

	EM_LOG_DEBUG("vod_is_download_finished:%u",task_id);

	if(em_is_net_ok(FALSE)==FALSE)
	{
		_p_param->_result = NETWORK_NOT_READY;
		goto ErrorHanle;
	}
		
	if(task_id>VOD_NODISK_TASK_MASK)
	{
		p_task = vod_get_task_from_map(task_id);
		if(p_task==NULL) 
		{
			_p_param->_result = INVALID_TASK_ID;
			goto ErrorHanle;
		}
		
		inner_id = p_task->_inner_id;
		_p_param->_result=eti_vod_is_download_finished((_int32)inner_id, finished);
	}
	else
	{
		inner_id =  dt_get_task_inner_id(  task_id );
		_p_param->_result=eti_vod_is_download_finished((_int32)inner_id, finished );
	}

	
ErrorHanle:

	EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 vod_get_bitrate( void * p_param )
{
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	_u32 task_id=(_u32)_p_param->_para1;
	_u32 file_index=(_u32)_p_param->_para2;
	_u32 * bitrate=(_u32*)_p_param->_para3;
	_u32 inner_id = 0;
	VOD_TASK * p_task = NULL;

	EM_LOG_DEBUG("vod_get_bitrate:%u",task_id);

	if(em_is_net_ok(FALSE)==FALSE)
	{
		_p_param->_result = NETWORK_NOT_READY;
		goto ErrorHanle;
	}
		
	if(task_id>VOD_NODISK_TASK_MASK)
	{
		p_task = vod_get_task_from_map(task_id);
		if(p_task==NULL) 
		{
			_p_param->_result = INVALID_TASK_ID;
			goto ErrorHanle;
		}
		
		inner_id = p_task->_inner_id;
		_p_param->_result=et_vod_get_bitrate((_int32)inner_id, file_index,bitrate);
	}
	else
	{
		inner_id =  dt_get_task_inner_id(  task_id );
		_p_param->_result=et_vod_get_bitrate((_int32)inner_id, file_index,bitrate);
	}

	
ErrorHanle:

	EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 vod_get_download_position( void * p_param )
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	_u32 task_id=(_u32)_p_param->_para1;
	_u64 * position=(_u64 *)_p_param->_para2;
	_u32 inner_id = 0;
	VOD_TASK * p_task = NULL;

	EM_LOG_DEBUG("vod_get_download_position:%u",task_id);

	if(em_is_net_ok(FALSE)==FALSE)
	{
		_p_param->_result = NETWORK_NOT_READY;
		goto ErrorHanle;
	}
		
	if(task_id>VOD_NODISK_TASK_MASK)
	{
		p_task = vod_get_task_from_map(task_id);
		if(p_task==NULL) 
		{
			_p_param->_result = INVALID_TASK_ID;
			goto ErrorHanle;
		}
		
		inner_id = p_task->_inner_id;
		_p_param->_result=et_vod_get_download_position((_int32)inner_id, position);
	}
	else
	{
		inner_id =  dt_get_task_inner_id(  task_id );
		_p_param->_result=et_vod_get_download_position((_int32)inner_id, position );
	}

	
ErrorHanle:

	EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 vod_report( void * p_param )
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	_u32 task_id=(_u32)_p_param->_para1;
	ET_VOD_REPORT* p_report=(ET_VOD_REPORT *)_p_param->_para2;
	_u32 inner_id = 0;
	VOD_TASK * p_task = NULL;

	EM_LOG_DEBUG("vod_report:%u",task_id);

	if(em_is_net_ok(FALSE)==FALSE)
	{
		_p_param->_result = NETWORK_NOT_READY;
		goto ErrorHanle;
	}
		
	if(task_id>VOD_NODISK_TASK_MASK)
	{
		p_task = vod_get_task_from_map(task_id);
		if(p_task==NULL) 
		{
			_p_param->_result = INVALID_TASK_ID;
			goto ErrorHanle;
		}
		
		inner_id = p_task->_inner_id;
		_p_param->_result=et_vod_report((_int32)inner_id, p_report);
	}
	else
	{
		inner_id =  dt_get_task_inner_id(  task_id );
		_p_param->_result=et_vod_report((_int32)inner_id, p_report);
	}

	
ErrorHanle:

	EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 vod_final_server_host( void * p_param )
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	_u32 task_id=(_u32)_p_param->_para1;
	char* p_host=(char *)_p_param->_para2;
	_u32 inner_id = 0;
	VOD_TASK * p_task = NULL;

	EM_LOG_DEBUG("vod_final_server_host:%u",task_id);

	if(em_is_net_ok(FALSE)==FALSE)
	{
		_p_param->_result = NETWORK_NOT_READY;
		goto ErrorHanle;
	}
		
	if(task_id>VOD_NODISK_TASK_MASK)
	{
		p_task = vod_get_task_from_map(task_id);
		if(p_task==NULL) 
		{
			_p_param->_result = INVALID_TASK_ID;
			goto ErrorHanle;
		}
		
		inner_id = p_task->_inner_id;
		_p_param->_result= -1;//et_vod_final_server_host((_int32)inner_id, p_host);
	}
	else
	{
		inner_id =  dt_get_task_inner_id(  task_id );
		_p_param->_result= -1;//et_vod_final_server_host((_int32)inner_id, p_host);
	}

	
ErrorHanle:

	EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

/////界面通知下载库开始点播(只适用于云点播任务)
_int32 vod_ui_notify_start_play( void * p_param )
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	_u32 task_id=(_u32)_p_param->_para1;
	_u32 inner_id = 0;
	VOD_TASK * p_task = NULL;

	EM_LOG_DEBUG("vod_ui_notify_start_play:%u",task_id);

	if(em_is_net_ok(FALSE)==FALSE)
	{
		_p_param->_result = NETWORK_NOT_READY;
		goto ErrorHanle;
	}
		
	if(task_id>VOD_NODISK_TASK_MASK)
	{
		p_task = vod_get_task_from_map(task_id);
		if(p_task==NULL) 
		{
			_p_param->_result = INVALID_TASK_ID;
			goto ErrorHanle;
		}
		
		inner_id = p_task->_inner_id;
		_p_param->_result= -1;//et_vod_ui_notify_start_play((_int32)inner_id);
	}
	else
	{
		inner_id =  dt_get_task_inner_id(  task_id );
		_p_param->_result= -1;//et_vod_ui_notify_start_play((_int32)inner_id);
	}

	
ErrorHanle:

	EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 vod_get_task_info(void * p_param)
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	_u32 task_id = (_u32 )_p_param->_para1;
	EM_TASK_INFO * p_em_task_info = (EM_TASK_INFO* )_p_param->_para2;
	ETI_TASK info;
	_u32 inner_id = 0;
	VOD_TASK * p_task = NULL;

	EM_LOG_DEBUG("vod_get_task_info:%u",task_id);

		p_task = vod_get_task_from_map(task_id);
		if(p_task==NULL) 
		{
			_p_param->_result = INVALID_TASK_ID;
			goto ErrorHanle;
		}
		
		if(em_is_net_ok(FALSE)==FALSE)
		{
			_p_param->_result = NETWORK_NOT_READY;
			goto ErrorHanle;
		}
			
		inner_id = p_task->_inner_id;
		em_memset(&info,0,sizeof(ETI_TASK));
		_p_param->_result=eti_get_task_info(inner_id, &info);
		if(_p_param->_result==SUCCESS)
		{
			p_em_task_info->_task_id = task_id;
			switch(info._task_status)
			{
				case ET_TASK_IDLE :
					p_em_task_info->_state = TS_TASK_WAITING;
					break;
				case ET_TASK_RUNNING :
				case ET_TASK_VOD :
					p_em_task_info->_state = TS_TASK_RUNNING;
					break;
				case ET_TASK_SUCCESS :
					p_em_task_info->_state = TS_TASK_SUCCESS;
					break;
				case ET_TASK_FAILED :
					p_em_task_info->_state = TS_TASK_FAILED;
					break;
				case ET_TASK_STOPPED :
					p_em_task_info->_state = TS_TASK_PAUSED;
					break;
			}
			p_em_task_info->_type = p_task->_type;   

			p_em_task_info->_file_size=info._file_size;  
			p_em_task_info->_downloaded_data_size=info._downloaded_data_size; 		
	 
			p_em_task_info->_start_time=info._start_time;	
			p_em_task_info->_finished_time=info._finished_time;

			p_em_task_info->_failed_code=info._failed_code;
			p_em_task_info->_is_deleted=FALSE;	
			p_em_task_info->_check_data=p_task->_check_data;	
			p_em_task_info->_is_no_disk= p_task->_is_no_disk;
		}

ErrorHanle:	
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}


_int32 vod_get_task_running_status(_u32 task_id,RUNNING_STATUS *p_status)
{
	ETI_TASK info;
	_u32 inner_id = 0;
	_int32 ret_val = SUCCESS;
	//VOD_TASK * p_task = NULL;

	EM_LOG_DEBUG("vod_get_task_running_status:%u",task_id);

	sd_assert(task_id>VOD_NODISK_TASK_MASK);
		inner_id = task_id-VOD_NODISK_TASK_MASK;
		em_memset(&info,0,sizeof(ETI_TASK));
		ret_val=eti_get_task_info(inner_id, &info);
		if(ret_val != SUCCESS)
			return ret_val;
		
			switch(info._task_status)
			{
				case ET_TASK_IDLE :
					p_status->_state = TS_TASK_WAITING;
					break;
				case ET_TASK_RUNNING :
				case ET_TASK_VOD :
					p_status->_state = TS_TASK_RUNNING;
					break;
				case ET_TASK_SUCCESS :
					p_status->_state = TS_TASK_SUCCESS;
					break;
				case ET_TASK_FAILED :
					p_status->_state = TS_TASK_FAILED;
					break;
				case ET_TASK_STOPPED :
					p_status->_state = TS_TASK_PAUSED;
					break;
			}
			//p_status->_type = p_task->_type;
			p_status->_file_size= info._file_size; 		
			p_status->_downloaded_data_size = info._downloaded_data_size; 		
			p_status->_dl_speed = info._speed;
			p_status->_ul_speed= info._ul_speed;
			p_status->_downloading_pipe_num= info._dowing_server_pipe_num+info._dowing_peer_pipe_num;
			p_status->_connecting_pipe_num= info._connecting_server_pipe_num+info._connecting_peer_pipe_num;
	 

	return SUCCESS;
}

_int32 vod_get_bt_file_info( void * p_param )
{
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	_u32 task_id = (_u32 )_p_param->_para1;
	_u32 file_index = (_u32 )_p_param->_para2;
	EM_BT_FILE * p_bt_file = (EM_BT_FILE* )_p_param->_para3;
	ETI_BT_FILE info;
	_u32 inner_id = 0;
	VOD_TASK * p_task = NULL;

	EM_LOG_DEBUG("vod_get_bt_file_info:%u",task_id);

		p_task = vod_get_task_from_map(task_id);
		if(p_task==NULL) 
		{
			_p_param->_result = INVALID_TASK_ID;
			goto ErrorHanle;
		}
		
		if(em_is_net_ok(FALSE)==FALSE)
		{
			_p_param->_result = NETWORK_NOT_READY;
			goto ErrorHanle;
		}
		
		inner_id = p_task->_inner_id;
		em_memset(&info,0,sizeof(ETI_BT_FILE));
		_p_param->_result=iet_get_bt_file_info(inner_id,file_index, &info);
		if(_p_param->_result==SUCCESS)
		{
			p_bt_file->_file_index = info._file_index;
			p_bt_file->_need_download = TRUE;
			p_bt_file->_file_size = info._file_size;
			p_bt_file->_downloaded_data_size = info._downloaded_data_size;
			p_bt_file->_failed_code = info._sub_task_err_code;
			
			switch(info._file_status)
			{
				case 0 :
					p_bt_file->_status = EMFS_IDLE;
					break;
				case 1 :
					p_bt_file->_status = EMFS_DOWNLOADING;
					break;
				case 2 :
					p_bt_file->_status = EMFS_SUCCESS;
					break;
				case 3 :
					p_bt_file->_status = EMFS_FAILED;
					break;
			}
		}	

ErrorHanle:	
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 vod_get_all_bt_file_index(char  * seed_file_full_path, _u32 ** pp_need_dl_file_index_array,_u32 * p_need_dl_file_num)
{
#ifdef ENABLE_BT
	_u32 file_num = 0;
	TORRENT_SEED_INFO * p_seed_info = NULL;
	_u32 *file_index_array = 0;
	_u32 dl_file_num=0;
	const char *padding_str = "_____padding_file";
	TORRENT_FILE_INFO *file_info_array_ptr;
	_u32 encoding_mode = 2;
	
	_int32 ret_val = SUCCESS;
	EM_LOG_DEBUG( "vod_get_all_bt_file_index." );
//
	//char _title_name[256-ET_MAX_TD_CFG_SUFFIX_LEN];
	//u32 _title_name_len;
	//uint64 _file_total_size;
	//u32 _file_num;
	//u32 _encoding;//种子文件编码格式，GBK = 0, UTF_8 = 1, BIG5 = 2
	//unsigned char _info_hash[20];
///
	em_settings_get_int_item("system.encoding_mode", (_int32*)&encoding_mode);
	ret_val= tp_get_seed_info( seed_file_full_path,encoding_mode, &p_seed_info );
	if(ret_val!=SUCCESS) return ret_val;

	
	if(p_seed_info->_file_num==0)
	{
		tp_release_seed_info( p_seed_info );
		CHECK_VALUE( INVALID_FILE_NUM );
	}
	
	ret_val = em_malloc( p_seed_info->_file_num * sizeof(_u32), (void **)&file_index_array );
	CHECK_VALUE( ret_val );

	ret_val = em_memset( file_index_array, 0, p_seed_info->_file_num * sizeof(_u32) );
      if(ret_val != SUCCESS)
	{
      	 	EM_SAFE_DELETE( file_index_array );
		tp_release_seed_info( p_seed_info );
	 	CHECK_VALUE( ret_val );
	}

	file_info_array_ptr = *(p_seed_info->_file_info_array_ptr);
	for( file_num = 0; file_num < p_seed_info->_file_num; file_num++ )
	{
		if((dl_file_num >= MAX_FILE_NUM)||(file_info_array_ptr->_file_index>=MAX_FILE_INDEX))
		{
			break;
		}

		if(( file_info_array_ptr->_file_size > MIN_BT_FILE_SIZE )
			&& em_strncmp( file_info_array_ptr->_file_name, padding_str, em_strlen(padding_str) ) )
		{
			file_index_array[dl_file_num] = file_info_array_ptr->_file_index;
			dl_file_num++;
		}
		file_info_array_ptr++;
	}

	tp_release_seed_info( p_seed_info );

	ret_val = em_malloc( dl_file_num * sizeof(_u32), (void **)pp_need_dl_file_index_array );
      if(ret_val != SUCCESS)
      	{
      	     EM_SAFE_DELETE( file_index_array );
	     CHECK_VALUE( ret_val );
      	}

	em_memset( *pp_need_dl_file_index_array, 0, dl_file_num * sizeof(_u32) );

	for( file_num = 0; file_num < dl_file_num; file_num++ )
	{
		(*pp_need_dl_file_index_array)[file_num]=file_index_array[file_num];
	}

	*p_need_dl_file_num=dl_file_num;
	EM_SAFE_DELETE( file_index_array );
	return ret_val;	
#else
		return -1;
#endif
}

_int32 vod_create_bt_task( EM_CREATE_VOD_TASK * p_create_param ,_u32 * inner_id)
{
#ifdef ENABLE_BT
	_int32 ret_val = SUCCESS;
	char seed_path[MAX_FULL_PATH_BUFFER_LEN];
	EM_ENCODING_MODE  encoding_mode = 2;
	_u32 * need_dl_file_index_array=NULL;
	_u32 need_dl_file_num = 0;
	char * file_path = em_get_system_path();

	EM_LOG_DEBUG("vod_create_bt_task");

	if((p_create_param->_seed_file_full_path==NULL)||(em_strlen(p_create_param->_seed_file_full_path)==0)||(p_create_param->_seed_file_full_path_len==0)||(p_create_param->_seed_file_full_path_len>=MAX_FULL_PATH_LEN))
	{
		return INVALID_SEED_FILE;
	}
	
	em_memset(seed_path, 0, MAX_FULL_PATH_BUFFER_LEN);
	em_strncpy(seed_path, p_create_param->_seed_file_full_path, p_create_param->_seed_file_full_path_len);

	em_settings_get_int_item("system.encoding_mode", (_int32*)&encoding_mode);
	
	if(p_create_param->_download_file_index_array!=NULL)
	{
		ret_val = eti_create_bt_task(seed_path, em_strlen(seed_path),  
			file_path, em_strlen(file_path),p_create_param->_download_file_index_array, 
			p_create_param->_file_num,encoding_mode,inner_id);
	}
	else
	{
		ret_val=vod_get_all_bt_file_index( seed_path, &need_dl_file_index_array,&need_dl_file_num);
		if(ret_val!=SUCCESS) return ret_val;
		ret_val = eti_create_bt_task(seed_path, em_strlen(seed_path),  
			file_path, em_strlen(file_path),need_dl_file_index_array, 
			need_dl_file_num,encoding_mode,inner_id);
	      	     EM_SAFE_DELETE( need_dl_file_index_array );
	}
		

	return ret_val;
	
#else
		return -1;
#endif
}


_int32 vod_create_p2sp_task( EM_CREATE_VOD_TASK * p_create_param ,_u32 * inner_id)
{
	_int32 ret_val = SUCCESS;
	char url[MAX_URL_LEN];
	char buffer_tmp[MAX_URL_LEN];
	char * ref_url = NULL;
	char * file_path = em_get_system_path();
	_u8 tcid[CID_SIZE],gcid[CID_SIZE];
#ifdef ENABLE_LIXIAN
	char cookie[1024] = {0};
	_u32 cookie_len = 0;
#endif

	EM_LOG_DEBUG("vod_create_p2sp_task");


		switch(p_create_param->_type)
		{
			case TT_URL:
			case TT_LAN:
				if((p_create_param->_url==NULL)||(em_strlen(p_create_param->_url)==0)||(p_create_param->_url_len==0)||(p_create_param->_url_len>=MAX_URL_LEN))
				{
					return INVALID_URL;
				}
				
				em_memset(url, 0, MAX_URL_LEN);
				em_strncpy(url, p_create_param->_url, p_create_param->_url_len);
				
				if((p_create_param->_ref_url!=NULL)&&(em_strlen(p_create_param->_ref_url)!=0)&&(p_create_param->_ref_url_len!=0)&&(p_create_param->_ref_url_len<MAX_URL_LEN))
				{
					em_memset(buffer_tmp, 0, MAX_URL_LEN);
					em_strncpy(buffer_tmp, p_create_param->_ref_url, p_create_param->_ref_url_len);
					ref_url = buffer_tmp;
				}
				
#ifdef _ANDROID_LINUX
#ifdef ENABLE_LIXIAN
				if((p_create_param->_user_data!= NULL) && (p_create_param->_user_data_len > 0))
				{
					if(sd_memcmp(p_create_param->_user_data, "Cookie:", 7)==0)
					{
						cookie_len = p_create_param->_user_data_len;
						if(cookie_len > 1024-1)
							cookie_len = 1024-1;
						sd_strncpy(cookie, (char*)p_create_param->_user_data, cookie_len);
					}
					/* lixian 任务下载需要加cookie验证 */
					ret_val = eti_try_create_new_task_by_url(url, em_strlen(url),
															ref_url, em_strlen(ref_url),
															cookie, cookie_len,
															file_path, em_strlen(file_path),
															NULL, 0,
															inner_id);
				}
				else
				{
					ret_val = eti_try_create_new_task_by_url(url, em_strlen(url),
													ref_url, em_strlen(ref_url),
													NULL, 0,
													file_path, em_strlen(file_path),
													NULL, 0,
													inner_id);					
				}
#else
				/* Android平台下为避免monkey test 导致程序被系统杀掉,使用eti_try_create_new_task_by_url接口以防止程序卡死! */
				ret_val = eti_try_create_new_task_by_url(url, em_strlen(url),
													ref_url, em_strlen(ref_url),
													NULL, 0,
													file_path, em_strlen(file_path),
													NULL, 0,
													inner_id);
#endif
#else
#ifdef ENABLE_LIXIAN
				if((p_create_param->_user_data!= NULL) && (p_create_param->_user_data_len > 0))
				{
					if(sd_memcmp(p_create_param->_user_data, "Cookie:", 7)==0)
					{
						cookie_len = p_create_param->_user_data_len;
						if(cookie_len > 1024-1)
							cookie_len = 1024-1;
						sd_strncpy(cookie, (char*)p_create_param->_user_data, cookie_len);
					}
					/* lixian 任务下载需要加cookie验证 */
					ret_val = eti_create_new_task_by_url(url, em_strlen(url),
															ref_url, em_strlen(ref_url),
															cookie, cookie_len,
															file_path, em_strlen(file_path),
															NULL, 0,
															inner_id);
				}
				else
				{
					ret_val = eti_create_new_task_by_url(url, em_strlen(url),
														ref_url, em_strlen(ref_url),
														NULL, 0,
														file_path, em_strlen(file_path),
														NULL, 0,
														inner_id);					
				}
#else
				ret_val = eti_create_new_task_by_url(url, em_strlen(url),
													ref_url, em_strlen(ref_url),
													NULL, 0,
													file_path, em_strlen(file_path),
													NULL, 0,
													inner_id);
#endif
#endif
				break;
			case TT_EMULE:
				if((p_create_param->_url==NULL)||(em_strlen(p_create_param->_url)==0)||(p_create_param->_url_len==0)||(p_create_param->_url_len>=MAX_URL_LEN))
				{
					return INVALID_URL;
				}
				
				em_memset(url, 0, MAX_URL_LEN);
				em_strncpy(url, p_create_param->_url, p_create_param->_url_len);
				
				ret_val = eti_create_emule_task(url, em_strlen(url),
													file_path, em_strlen(file_path),
													NULL, 0,
													inner_id);
				break;
			case TT_KANKAN:
				em_memset(tcid,0,CID_SIZE);
				em_memset(gcid,0,CID_SIZE);
				if(em_string_to_cid(p_create_param->_tcid,tcid)!=0)
				{
					return INVALID_TCID;
				}
				
				if(em_string_to_cid(p_create_param->_gcid,gcid)!=0)
				{
					return INVALID_GCID;
				}

				em_memset(buffer_tmp, 0, CID_SIZE*2+1);
				em_strncpy(buffer_tmp, p_create_param->_gcid, CID_SIZE*2);

				ret_val = eti_create_task_by_tcid_file_size_gcid(tcid, p_create_param->_file_size, gcid,
												buffer_tmp, em_strlen(buffer_tmp), file_path, em_strlen(file_path), inner_id);
				break;
			case TT_TCID:
				em_memset(tcid,0,CID_SIZE);
				if(em_string_to_cid(p_create_param->_tcid,tcid)!=0)
				{
					return INVALID_TCID;
				}
				
				em_memset(buffer_tmp, 0, CID_SIZE*2+1);
				em_strncpy(buffer_tmp, p_create_param->_tcid, CID_SIZE*2);

				ret_val = eti_create_new_task_by_tcid(tcid, p_create_param->_file_size,buffer_tmp, em_strlen(buffer_tmp), 
												file_path, em_strlen(file_path),   inner_id);
				break;
			default:
				return INVALID_TASK_TYPE;
		}

	//CHECK_VALUE(ret_val);

	return ret_val;

}
_int32 vod_generate_eigenvalue(EM_EIGENVALUE * p_eigenvalue,_u8 * eigenvalue)
{
      EM_LOG_DEBUG("vod_generate_eigenvalue"); 
	switch(p_eigenvalue->_type)
	{
		case TT_URL:
		case TT_EMULE:
			em_sd_replace_str(p_eigenvalue->_url, "%7C", "|");
			p_eigenvalue->_url_len = em_strlen(p_eigenvalue->_url);
			return dt_get_url_eigenvalue(p_eigenvalue->_url,p_eigenvalue->_url_len,eigenvalue);
			break;
		case TT_TCID:
		case TT_LAN:
		case TT_KANKAN:
		case TT_BT:
			return dt_get_cid_eigenvalue(p_eigenvalue->_eigenvalue,eigenvalue);
			break;
		case TT_FILE:
		default:
			return INVALID_TASK_TYPE;
	}
	return SUCCESS;
}

_int32 vod_get_task_id_by_eigenvalue( EM_EIGENVALUE * p_eigenvalue ,_u32 * task_id)
{
	_int32 ret_val = SUCCESS;
	_u8  eigenvalue[CID_SIZE];
      MAP_ITERATOR  cur_node = NULL; 
	 VOD_TASK * p_task = NULL;

	if(vod_get_task_num(  )==0) 
	{
		return INVALID_TASK;
	}

	ret_val =  vod_generate_eigenvalue(p_eigenvalue,eigenvalue);
	CHECK_VALUE(ret_val);

      for(cur_node = EM_MAP_BEGIN(g_vod_manager._all_tasks);cur_node != EM_MAP_END(g_vod_manager._all_tasks);cur_node=EM_MAP_NEXT(g_vod_manager._all_tasks, cur_node))
      {
             p_task = (VOD_TASK *)EM_MAP_VALUE(cur_node);
		if(sd_is_cid_equal(p_task->_eigenvalue, eigenvalue))
		{
			*task_id = p_task->_task_id;
			return SUCCESS;
		}
      }
	  
	return INVALID_TASK;
}

_int32 vod_report_impl(_u32 task_id)
{
	_int32 ret_val = SUCCESS;
	_u32 inner_id = 0;
	VOD_TASK * p_task = NULL;
	char report[MAX_URL_LEN] = {0};
	const char * p_first_u = ETM_YUNBO_REPORT_PROTOCOL;
	//_int32 product_id = 0;
	EM_LOG_DEBUG("vod_report_impl: task_id = %u", task_id); 
	if(task_id>VOD_NODISK_TASK_MASK)
	{
		p_task = vod_get_task_from_map(task_id);
		if(p_task==NULL) 
		{
			EM_LOG_DEBUG("vod_report_impl: vod_get_task_from_map no task, task_id = %u", task_id); 
			return -1;
		}
		
		inner_id = p_task->_inner_id;
		 ret_val =  -1;//et_vod_get_report(inner_id, report);
	}
	else
	{
		inner_id =  dt_get_task_inner_id(  task_id );
		ret_val = -1;//et_vod_get_report(inner_id, report);
	}

	if(ret_val!=SUCCESS) return ret_val;

	//em_settings_get_int_item("system.ui_product", &product_id);
	//if(product_id!=19&&product_id!=39)
	//{
		/* 非云播安卓项目,需要定义该上报协议! */
		//sd_assert(FALSE);
		//return SUCCESS;
	//}
	
	ret_val = em_http_etm_inner_report(p_first_u, report);
	CHECK_VALUE(ret_val);
	
	return SUCCESS;
}

#if 0

/////2.8 设置有盘点播的默认临时文件缓存路径,必须已存在且可写,path_len 必须小于ETM_MAX_FILE_PATH_LEN
_int32 vod_set_cache_path( void * p_param )
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	char *cache_path= (char * )_p_param->_para1;

	EM_LOG_DEBUG("vod_set_cache_path");
	
	_p_param->_result=em_test_path_writable(cache_path);
	if(_p_param->_result==SUCCESS)
	{
		_p_param->_result=em_settings_set_str_item("system.vod_cache_path", cache_path);
		if(_p_param->_result==SUCCESS)
		{
			em_memset(g_vod_manager._cache_path,0, MAX_FILE_PATH_LEN);
			em_strncpy(g_vod_manager._cache_path, cache_path, MAX_FILE_PATH_LEN-1);
		}
	}
	
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}

char * vod_get_cache_path_impl( void  )
{
	if(em_strlen(g_vod_manager._cache_path)!=0)
	{
		return g_vod_manager._cache_path;
	}
	return NULL;
}
_int32 vod_get_cache_path( void * p_param )
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	char *cache_path= (char * )_p_param->_para1;
	char * p_path = vod_get_cache_path_impl();

	EM_LOG_DEBUG("vod_get_cache_path");

	if(p_path!=NULL)
	{
		em_strncpy(cache_path, p_path, MAX_FILE_PATH_LEN-1);
	}
	
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}


/////2.9 设置有盘点播的缓存区大小，单位KB，即接口etm_set_vod_cache_path 所设进来的目录的最大可写空间，建议3GB 以上
_int32 vod_set_cache_size( void * p_param )
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	_u32 cache_size= (_u32 )_p_param->_para1;

	EM_LOG_DEBUG("vod_set_cache_size");
	
	_p_param->_result=em_settings_set_int_item("system.vod_cache_size", (_int32)cache_size);
	if(_p_param->_result==SUCCESS)
	{
		g_vod_manager._max_cache_size=cache_size;
	}
	
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32  vod_get_cache_size( void * p_param )
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	_u32 * cache_size= (_u32 * )_p_param->_para1;

	EM_LOG_DEBUG("vod_get_cache_size");

	*cache_size = g_vod_manager._max_cache_size;
	
	//_p_param->_result=em_settings_get_int_item("system.vod_cache_size", (_int32 *)cache_size);
	
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}


/////2.10 设置点播的专用内存大小,单位KB，建议2MB 以上
_int32 vod_set_buffer_size( void * p_param )
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	_u32 buffer_size= (_u32 )_p_param->_para1;

	EM_LOG_DEBUG("vod_set_buffer_size");
	
	_p_param->_result=em_settings_set_int_item("system.vod_buffer_size", (_int32)buffer_size);
	if((_p_param->_result==SUCCESS)&&(em_is_et_running() == TRUE))
	{
		_p_param->_result=eti_vod_set_vod_buffer_size((_int32)buffer_size*1024);	
	}
	
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 vod_get_buffer_size( void * p_param )
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
	
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}


/////2.11 查询vod 专用内存是否已经分配
_int32 vod_is_buffer_allocated( void * p_param )
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	BOOL * is_allocated= (BOOL * )_p_param->_para1;
	_int32 allocated=0;

	EM_LOG_DEBUG("vod_is_buffer_allocated");

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
	
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}

/////2.12 手工释放vod 专用内存,ETM 本身在反初始化时会自动释放这块内存，但如果界面程序想尽早腾出这块内存的话，可调用此接口，注意调用之前要确认无点播任务在运行
_int32  vod_free_buffer( void * p_param )
{
	POST_PARA_0* _p_param = (POST_PARA_0*)p_param;

	EM_LOG_DEBUG("vod_free_buffer");
	
	if(em_is_et_running() == TRUE)
	{
		_p_param->_result=eti_vod_free_vod_buffer();
	 }
	
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}
#endif
#endif


