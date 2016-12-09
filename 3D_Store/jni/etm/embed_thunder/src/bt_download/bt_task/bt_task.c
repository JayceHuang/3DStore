
#include "utility/define.h"
#ifdef ENABLE_BT

#include "bt_task.h"
#include "bt_accelerate.h"
#include "bt_query.h"
#include "connect_manager/connect_manager_interface.h"
#include "data_manager/data_manager_interface.h"
#include "task_manager/task_manager.h"
#include "utility/settings.h"
#include "utility/sd_assert.h"
#include "utility/utility.h"
#include "utility/time.h"
#include "utility/mempool.h"
#include "platform/sd_fs.h"

#ifdef UPLOAD_ENABLE
#include "upload_manager/upload_manager.h"
#endif


#include "utility/logid.h"
#define LOGID LOGID_BT_DOWNLOAD
#include "utility/logger.h"

static SLAB* g_bt_task_slab = NULL;
static SLAB* g_bt_file_info_slab = NULL;
static SLAB* g_bt_file_task_info_slab = NULL;
static SLAB* g_bt_query_para_slab = NULL;



_int32 bt_check_task_para( BT_TASK *p_bt_task, struct tagTM_BT_TASK *p_task_para )
{
	_u32 file_total_num = 0;
	_u32 *file_index_array = 0;
	_u32 file_index = 0;
	_u32 file_num = 0;
	
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "bt_check_task_para." );

	
	file_total_num = tp_get_seed_file_num( p_bt_task->_torrent_parser_ptr );
	if( p_task_para->_file_num > file_total_num )
	{
		return BT_ERROR_FILE_INDEX;
	}
	if( 0 ==  p_task_para->_file_num ) return BT_ERROR_FILE_INDEX;
	ret_val = sd_malloc( file_total_num * sizeof(_u32), (void **)&file_index_array );
	CHECK_VALUE( ret_val );

	ret_val = sd_memset( file_index_array, 0, file_total_num * sizeof(_u32) );
	//CHECK_VALUE( ret_val );
      if(ret_val != SUCCESS)
      	{
      	     SAFE_DELETE( file_index_array );
	     return ret_val;		 
      	}

	for( file_num = 0; file_num < p_task_para->_file_num; file_num++ )
	{
		file_index =  p_task_para->_download_file_index_array[file_num];
		if( file_index >= file_total_num ) 
		{
			ret_val = BT_ERROR_FILE_INDEX;
			break;
		}
		file_index_array[file_index]++;
		if( file_index_array[file_index] > 1 )
		{
			ret_val = BT_ERROR_FILE_INDEX;
			break;
		}
	}

	SAFE_DELETE( file_index_array );
	return ret_val;
}

_int32 bt_update_task_info( BT_TASK *p_bt_task )
{


	TASK * p_task = (TASK *)p_bt_task;
	_int32 ret_val = SUCCESS;
	_int32 _data_status_code = 0;
	_u32 max_bt_accelerate_num= 3;   // 目前只能为1.  由于切换子任务的限制....


	LOG_DEBUG("bt_update_task_info:task_status=%d",p_task->task_status);
	
	if((p_task->task_status!=TASK_S_RUNNING)&&(p_task->task_status!=TASK_S_VOD))
	{
		return DT_ERR_TASK_NOT_RUNNING;
	}
	

	/* Get the downloading status */
	_data_status_code = bdm_get_status_code(& (p_bt_task->_data_manager));
	if(_data_status_code ==DATA_DOWNING) 
	{
		p_task->task_status = TASK_S_RUNNING;
		p_task->failed_code = 0;
	}else
	if(_data_status_code ==DATA_SUCCESS) 
	{
		p_task->task_status = TASK_S_SUCCESS;
		p_task->failed_code = 0;
	}else
	if(_data_status_code ==VOD_DATA_FINISHED) 
	{
		p_task->task_status = TASK_S_VOD;
		p_task->failed_code = 0;
	}else
	{
		p_task->task_status = TASK_S_FAILED;
		p_task->failed_code = _data_status_code;
	}

	/* Get the downloading progress */
	p_task->progress = bdm_get_total_file_percent(& (p_bt_task->_data_manager));
	//p_task->file_size = bdm_get_file_size(& (p_task->_data_manager));
	p_task->_downloaded_data_size= bdm_get_total_file_download_data_size(& (p_bt_task->_data_manager));
	p_task->_written_data_size= bdm_get_total_file_writed_data_size(& (p_bt_task->_data_manager));
	//if(p_task->_written_data_size!=0)
	if(p_task->_downloaded_data_size!=0)
	{
		p_task->_file_create_status=FILE_CREATED_SUCCESS;
	}
	
	p_task->ul_speed = 0;
	
	ret_val = dt_update_task_info( p_task  );
	if(ret_val!=SUCCESS) return ret_val;
	
	p_task->ul_speed= cm_get_upload_speed(& (p_task->_connect_manager));
    
#ifdef ENABLE_BT_PROTOCOL
	p_task->ul_speed += cm_get_sub_manager_upload_speed(& (p_task->_connect_manager));
#endif
	
#ifdef _CONNECT_DETAIL
	p_task->_bt_dl_speed= cm_get_task_bt_download_speed(& (p_task->_connect_manager));
	p_task->_bt_ul_speed= cm_get_upload_speed(& (p_task->_connect_manager));
#endif

	ret_val = bt_update_file_info( p_bt_task );
	if(ret_val!=SUCCESS) return ret_val;

	if(p_task->task_status == TASK_S_RUNNING)
	{
        LOG_DEBUG("p_bt_task->_file_task_info_map.size = %d, max_bt_acc_num = %d"
            , map_size(&(p_bt_task->_file_task_info_map)), max_bt_accelerate_num );
		if(map_size(&(p_bt_task->_file_task_info_map))<max_bt_accelerate_num)
		{
			bt_start_next_accelerate( p_bt_task);
		}
		else
		{
			LOG_DEBUG( "cant start more accelerate sub task , there are 3 sub task accelerating...");
			//bt_ajust_accelerate_list( p_bt_task);
			//bt_start_next_accelerate( p_bt_task);
		}
	}
	return SUCCESS;

}




_int32 bt_init_file_info( BT_TASK *p_bt_task, struct tagTM_BT_TASK *p_task_para )
{
	_int32 ret_val = SUCCESS;
	_u32 file_count = 0;
	_u32 file_index = 0;
	TORRENT_FILE_INFO *p_tr_file_info = NULL;
	BT_FILE_INFO * p_bt_file_info=NULL;
	PAIR info_map_node;
	_u64 min_file_size=DEFAULT_BT_ACCELERATE_MIN_SIZE;
	
	LOG_DEBUG( "bt_init_file_info." );

	ret_val = bt_check_task_para( p_bt_task, p_task_para );
	LOG_DEBUG( "bt_init_file_info:bt_check_task_para return:%d", ret_val);
	//CHECK_VALUE( ret_val );
	if( ret_val != SUCCESS ) return ret_val;
	
	settings_get_int_item( "bt.min_bt_accelerate_file_size",(_int32*)&min_file_size);
	
	for( file_count = 0; file_count < p_task_para->_file_num; file_count++ )
	{
		p_bt_file_info=NULL;
		ret_val=bt_file_info_malloc_wrap(&p_bt_file_info);		
		if( ret_val!=SUCCESS ) goto ErrorHanle;
		sd_memset(p_bt_file_info,0,sizeof(BT_FILE_INFO));
		
		file_index = p_task_para->_download_file_index_array[file_count];
		p_bt_file_info->_file_index = file_index;
		
		ret_val = tp_get_file_info( p_bt_task->_torrent_parser_ptr, file_index, &p_tr_file_info );
		if( ret_val!=SUCCESS )
		{
			bt_file_info_free_wrap(p_bt_file_info);
			goto ErrorHanle;
		}
		
		p_bt_file_info->_file_name_str = p_tr_file_info->_file_name;
		p_bt_file_info->_file_name_len = p_tr_file_info->_file_name_len;
		p_bt_file_info->_file_size = p_tr_file_info->_file_size;
		//p_bt_file_info->_file_status = BT_FILE_DOWNLOADING;
		p_bt_file_info->_need_update_to_user = TRUE;

		if(p_bt_file_info->_file_size<=(min_file_size*1024))
		{
			p_bt_file_info->_accelerate_state = BT_ACCELERATE_END;
		}
		
		info_map_node._key = (void *)file_index;
		info_map_node._value = (void *)p_bt_file_info;
		
		p_bt_task->_task.file_size += p_bt_file_info->_file_size;
		ret_val = map_insert_node( &p_bt_task->_file_info_map, &info_map_node );
		if( ret_val!=SUCCESS )
		{
			bt_file_info_free_wrap(p_bt_file_info);
			goto ErrorHanle;
		}
	}


	return SUCCESS;
	
ErrorHanle:
	bt_uninit_file_info( p_bt_task );
	return ret_val;
}
_int32 bt_uninit_file_info( BT_TASK *p_bt_task )
{
	
	BT_FILE_INFO * p_bt_file_info=NULL;
	MAP_ITERATOR cur_node , next_node;
	LOG_DEBUG( "bt_uninit_file_info." );

	cur_node = MAP_BEGIN( p_bt_task->_file_info_map );

	while( cur_node != MAP_END( p_bt_task->_file_info_map ) )
	{
		p_bt_file_info = (BT_FILE_INFO *)MAP_VALUE( cur_node );
		next_node = MAP_NEXT( p_bt_task->_file_info_map, cur_node );
		map_erase_iterator(&(p_bt_task->_file_info_map), cur_node );
		bt_file_info_free_wrap(p_bt_file_info);
		cur_node = next_node;
	}
	return SUCCESS;
}

_int32 bt_update_file_info( BT_TASK *p_bt_task )
{
	
	MAP_ITERATOR cur_node ;
	BT_FILE_INFO *p_file_info =NULL;
	
	LOG_DEBUG( "bt_update_file_info. p_bt_task=0x%x", p_bt_task );
	
	for(cur_node = MAP_BEGIN( p_bt_task->_file_info_map ); cur_node != MAP_END(  p_bt_task->_file_info_map ); cur_node = MAP_NEXT(  p_bt_task->_file_info_map, cur_node ))
	{
		p_file_info = (BT_FILE_INFO *)MAP_VALUE( cur_node );
		bt_update_file_info_sub( p_bt_task,p_file_info );
	}
	
	return SUCCESS;
}



/* Update the task information */
_int32 bt_update_file_info_sub(  BT_TASK *p_bt_task,BT_FILE_INFO* p_file_info )
{
	//enum BT_FILE_STATUS file_status;
	TASK* p_task =(TASK*) p_bt_task;
	_u32 file_index = p_file_info->_file_index;
	_int32 ret_val=SUCCESS;

	if((p_file_info->_file_status == BT_FILE_FINISHED)||(p_file_info->_file_status == BT_FILE_FAILURE))
    {
        LOG_DEBUG("bt_update_file_info_sub, file_index = %d, file_status = %d"
            , file_index, p_file_info->_file_status);
		return SUCCESS;
    }
	
	/* Get the downloading status */
	p_file_info->_file_status = bdm_get_file_status(& (p_bt_task->_data_manager),file_index);
	
	LOG_DEBUG("bt_update_file_info:file_index=%u,file_status=%d",file_index,p_file_info->_file_status);
	
	if(p_file_info->_file_status == BT_FILE_IDLE) 
	{
		//sd_assert(p_file_info->_accelerate_state!=BT_ACCELERATING);
			if(p_file_info->_accelerate_state==BT_ACCELERATING)
			{
				ret_val=bt_stop_accelerate( p_bt_task,  file_index );
				CHECK_VALUE(ret_val);
				p_file_info->_accelerate_state=BT_NO_ACCELERATE;
			}
	}
	else
	{
		/* Get the file error code */
		p_file_info->_sub_task_err_code = bdm_get_file_err_code(& (p_bt_task->_data_manager),file_index);
		
		/* Get the downloading progress */
		p_file_info->_file_percent = bdm_get_sub_file_percent(& (p_bt_task->_data_manager),file_index);
		//p_file_info->file_size = dm_get_file_size(& (p_bt_task->_data_manager),file_index);
		p_file_info->_downloaded_data_size= bdm_get_sub_file_download_data_size(& (p_bt_task->_data_manager),file_index);
		p_file_info->_written_data_size= bdm_get_sub_file_writed_data_size(& (p_bt_task->_data_manager),file_index);

		LOG_DEBUG( "bt_update_file_info:_task_id=%u,_file_index=%u,file_name=%s,_file_size=%llu,_downloaded_data_size=%llu,_written_data_size=%llu,_file_percent=%u,_file_status=%d,_query_result=%d,err_code=%d,control_flag=%d,has_record=%d,_accelerate_state=%d" ,
			p_bt_task->_task._task_id,file_index,p_file_info->_file_name_str  ,p_file_info->_file_size,p_file_info->_downloaded_data_size,p_file_info->_written_data_size,p_file_info->_file_percent,p_file_info->_file_status,p_file_info->_query_result,p_file_info->_sub_task_err_code,p_file_info->_control_flag,p_file_info->_has_record,p_file_info->_accelerate_state);

		if(p_file_info->_file_status == BT_FILE_DOWNLOADING
			&& p_file_info->_accelerate_state == BT_ACCELERATING )
		{
			if( bdm_is_file_info_opening(&(p_bt_task->_data_manager),file_index)
				&& p_file_info->_downloaded_data_size > 0 )
			{
				cm_set_not_idle_status(&(p_task->_connect_manager) ,file_index);
			}
			/*
			else if( cm_is_idle_status( & (p_task->_connect_manager),file_index )==TRUE )
			{
				LOG_DEBUG("BT_ACCELERATING and cm_is_idle_status,so bt_stop_accelerate!!");
				ret_val=bt_stop_accelerate( p_bt_task,  file_index );
				CHECK_VALUE(ret_val);
			}
			*/
		}

		else if((p_file_info->_file_status == BT_FILE_FINISHED)||(p_file_info->_file_status == BT_FILE_FAILURE))
		{
			if(p_file_info->_accelerate_state==BT_ACCELERATING)
			{
				ret_val=bt_stop_accelerate( p_bt_task,  file_index );
				CHECK_VALUE(ret_val);
			}
		}
		
		if(p_file_info->_need_update_to_user != TRUE)
		{
			p_file_info->_need_update_to_user = TRUE;
		}
	}	

	if(p_file_info->_need_update_to_user == TRUE)
	{
		ret_val=bt_file_info_for_user_write(p_bt_task,p_file_info);
		if((ret_val==SUCCESS)&&(p_file_info->_file_status != BT_FILE_DOWNLOADING))
		{
			p_file_info->_need_update_to_user = FALSE;
		}
	}
	
	return SUCCESS;

}

_int32 bt_init_tracker_url_list( BT_TASK *p_bt_task )
{
	_int32 ret_val = SUCCESS;
	//LIST *tp_url_list=NULL;
	//char *tp_tracker_url=NULL,*tracker_url=NULL;
	//_u32 url_len=0;
    	//LIST_ITERATOR  cur_node = NULL;
	
	LOG_DEBUG( "bt_init_tracker_url_list");

	ret_val=tp_get_tracker_url( p_bt_task->_torrent_parser_ptr,&(p_bt_task->_tracker_url_list));
	CHECK_VALUE(ret_val);
/*
	if((tp_url_list!=NULL)&&(0!=list_size(tp_url_list)))
	{
		    for( cur_node = LIST_BEGIN( *tp_url_list ); cur_node != LIST_END( *tp_url_list ); cur_node = LIST_NEXT( cur_node ) )
		    {
		        tp_tracker_url = (char*)LIST_VALUE( cur_node );
			url_len=sd_strlen(tp_tracker_url);
			
			ret_val = sd_malloc(url_len+1, (void**)&(tracker_url));
			CHECK_VALUE(ret_val);

			sd_memset(tracker_url,0,url_len+1);
			sd_strncpy(tracker_url,tp_tracker_url,url_len);
			
			ret_val =  list_push(&(p_bt_task->_tracker_url_list),( void *)tracker_url);
			if(ret_val!=SUCCESS) 
			{
				SAFE_DELETE(tracker_url);
				CHECK_VALUE(ret_val);
			}
		    }
	}
*/
	return SUCCESS;
}


_int32 bt_clear_tracker_url_list( BT_TASK *p_bt_task )
{
	//char *tracker_url=NULL;
	
	LOG_DEBUG( "bt_clear_tracker_url_list");
/*
	while(0!=list_size(&(p_bt_task->_tracker_url_list)))
	{
	 	list_pop(&(p_bt_task->_tracker_url_list),( void **)&tracker_url);
		SAFE_DELETE(tracker_url);
	}
*/
	p_bt_task->_tracker_url_list=NULL;
	return SUCCESS;
}
_int32 bt_delete_single_file( char* file_full_path)
{

	
	_int32 ret_val = SUCCESS;
	char *td_str = ".rf";
	char *cfg_str = ".cfg";
	_u32 full_path_len=0;

	LOG_DEBUG( "bt_delete_single_file");

		ret_val = sd_delete_file( file_full_path );
		LOG_DEBUG( "delete_file:%s, ret_val:%d", file_full_path, ret_val );
		//CHECK_VALUE( ret_val );	
		
		full_path_len=sd_strlen( file_full_path );
		sd_assert(full_path_len+sd_strlen(td_str)<MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN);
		
		sd_strncpy( file_full_path + full_path_len, td_str, MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN-full_path_len );
		ret_val = sd_delete_file( file_full_path );
		LOG_DEBUG( "delete_file:%s, ret_val:%d", file_full_path, ret_val );
		//if( ret_val != SUCCESS ) return ret_val;
		//CHECK_VALUE( ret_val );	

		full_path_len=sd_strlen( file_full_path );
		sd_assert(full_path_len+sd_strlen(cfg_str)<MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN);
		
		sd_strncpy( file_full_path + full_path_len, cfg_str, MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN-full_path_len);
		ret_val = sd_delete_file( file_full_path );
		LOG_DEBUG( "delete_file:%s, ret_val:%d", file_full_path, ret_val );
		//if( ret_val != SUCCESS ) return ret_val;

	return SUCCESS;
}

_int32 bt_delete_tmp_file( BT_TASK *p_bt_task )
{
	MAP_ITERATOR cur_node;
	_int32 ret_val = SUCCESS;
	char file_full_path[MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN];
	char *task_data_path=NULL;
	TORRENT_FILE_INFO *p_torrent_file_info = NULL;
	_u32 file_index=0,task_data_path_len=0;

	LOG_DEBUG( "bt_delete_tmp_file");
	
	ret_val = bdm_get_data_path(&(p_bt_task->_data_manager),&task_data_path,&task_data_path_len);
	CHECK_VALUE( ret_val );
	sd_assert(task_data_path!=NULL);
	sd_assert(task_data_path_len!=0);
	LOG_DEBUG( "bt_delete_tmp_file:task_data_path[%u]=%s",task_data_path_len,task_data_path);

	ret_val = sd_memset( file_full_path, 0, MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN );
	CHECK_VALUE( ret_val );

	sd_strncpy( file_full_path, task_data_path, MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN );

	LOG_DEBUG( "bt_delete_tmp_file,start delete...");
	
	cur_node = MAP_BEGIN( p_bt_task->_file_info_map );
	while( cur_node != MAP_END( p_bt_task->_file_info_map ) )
	{
		file_index = (_u32)MAP_KEY( cur_node );
		LOG_DEBUG( "Start delete the tmp file of index=%u", file_index);

		
		ret_val = tp_get_file_info( p_bt_task->_torrent_parser_ptr, file_index, &p_torrent_file_info );
		CHECK_VALUE( ret_val );
		
		ret_val = sd_memset( file_full_path+task_data_path_len, 0, MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN-task_data_path_len );
		CHECK_VALUE( ret_val );
		
		if(p_torrent_file_info->_file_path_len!=0)
		{
			sd_assert(task_data_path_len+p_torrent_file_info->_file_path_len<MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN);
			sd_strncpy( file_full_path + task_data_path_len, p_torrent_file_info->_file_path,MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN-task_data_path_len  );
		}
		sd_assert(task_data_path_len+p_torrent_file_info->_file_path_len+p_torrent_file_info->_file_name_len<MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN);
		sd_strncpy( file_full_path + task_data_path_len + p_torrent_file_info->_file_path_len,
			p_torrent_file_info->_file_name, MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN-task_data_path_len-p_torrent_file_info->_file_path_len );

		ret_val = bt_delete_single_file( file_full_path);
		if( ret_val != SUCCESS ) return ret_val;

		if(p_torrent_file_info->_file_path_len!=0)
		{
			file_full_path[task_data_path_len+p_torrent_file_info->_file_path_len]='\0';
			ret_val = sd_rmdir( file_full_path );
			LOG_DEBUG( "delete_file:%s, ret_val:%d", file_full_path, ret_val );
		}
		
		cur_node = MAP_NEXT( p_bt_task->_file_info_map, cur_node );
	}	

	return SUCCESS;
}

/***********************************************************************/
/***********************************************************************/
/***********************************************************************/

//bt_file_task_info malloc/free wrap
_int32 init_bt_file_info_slabs(void)
{
	_int32 ret_val = SUCCESS;
	if( g_bt_file_info_slab == NULL )
	{              
		ret_val = mpool_create_slab( sizeof( BT_FILE_INFO ), MIN_BT_FILE_INFO, 0, &g_bt_file_info_slab );
		CHECK_VALUE( ret_val );
	}
	return ret_val;
}

_int32 uninit_bt_file_info_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_bt_file_info_slab != NULL )
	{              
		ret_val = mpool_destory_slab( g_bt_file_info_slab );
		CHECK_VALUE( ret_val );
		g_bt_file_info_slab = NULL;
	}
	return ret_val;
}
_int32 bt_file_info_malloc_wrap(BT_FILE_INFO **pp_node)
{
	return mpool_get_slip( g_bt_file_info_slab, (void**)pp_node );
}

_int32 bt_file_info_free_wrap(BT_FILE_INFO *p_node)
{
	sd_assert( p_node != NULL );
	if( p_node == NULL ) return SUCCESS;
	return mpool_free_slip( g_bt_file_info_slab, (void*)p_node );
}

//bt_file_task_info malloc/free wrap
_int32 init_bt_file_task_info_slabs(void)
{
	_int32 ret_val = SUCCESS;
	if( g_bt_file_task_info_slab == NULL )
	{              
		ret_val = mpool_create_slab( sizeof( BT_FILE_TASK_INFO ), MIN_BT_FILE_TASK_INFO, 0, &g_bt_file_task_info_slab );
		CHECK_VALUE( ret_val );
	}
	return ret_val;
}

_int32 uninit_bt_file_task_info_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_bt_file_task_info_slab != NULL )
	{              
		ret_val = mpool_destory_slab( g_bt_file_task_info_slab );
		CHECK_VALUE( ret_val );
		g_bt_file_task_info_slab = NULL;
	}
	return ret_val;
}

_int32 bt_file_task_info_malloc_wrap(struct tagBT_FILE_TASK_INFO **pp_node)
{
	return mpool_get_slip( g_bt_file_task_info_slab, (void**)pp_node );
}

_int32 bt_file_task_info_free_wrap(struct tagBT_FILE_TASK_INFO *p_node)
{
	sd_assert( p_node != NULL );
	if( p_node == NULL ) return SUCCESS;
	return mpool_free_slip( g_bt_file_task_info_slab, (void*)p_node );
}




//bt_query_para malloc/free wrap
_int32 init_bt_query_para_slabs(void)
{
	_int32 ret_val = SUCCESS;
	if( g_bt_query_para_slab == NULL )
	{              
		ret_val = mpool_create_slab( sizeof( RES_QUERY_PARA ), MIN_BT_QUERY_PARA, 0, &g_bt_query_para_slab );
		CHECK_VALUE( ret_val );
	}
	return ret_val;
}

_int32 uninit_bt_query_para_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_bt_query_para_slab != NULL )
	{              
		ret_val = mpool_destory_slab( g_bt_query_para_slab );
		CHECK_VALUE( ret_val );
		g_bt_query_para_slab = NULL;
	}
	return ret_val;
}

_int32 bt_query_para_malloc_wrap(RES_QUERY_PARA **pp_node)
{
	return mpool_get_slip( g_bt_query_para_slab, (void**)pp_node );
}

_int32 bt_query_para_free_wrap(RES_QUERY_PARA *p_node)
{
	sd_assert( p_node != NULL );
	if( p_node == NULL ) return SUCCESS;
	return mpool_free_slip( g_bt_query_para_slab, (void*)p_node );
}



//bt_task malloc/free wrap
_int32 init_bt_task_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_bt_task_slab == NULL )
	{              
		ret_val = mpool_create_slab( sizeof( BT_TASK ), MIN_BT_TASK, 0, &g_bt_task_slab );
		CHECK_VALUE( ret_val );
	}
	return ret_val;
}

_int32 uninit_bt_task_slabs(void)
{
	_int32 ret_val = SUCCESS;

	if( g_bt_task_slab != NULL )
	{              
		ret_val = mpool_destory_slab( g_bt_task_slab );
		CHECK_VALUE( ret_val );
		g_bt_task_slab = NULL;
	}
	return ret_val;
}

_int32 bt_task_malloc_wrap(BT_TASK **pp_node)
{
	return mpool_get_slip( g_bt_task_slab, (void**)pp_node );
}

_int32 bt_task_free_wrap(BT_TASK *p_node)
{
	sd_assert( p_node != NULL );
	if( p_node == NULL ) return SUCCESS;
	sd_memset(p_node,0,sizeof(BT_TASK));
	return mpool_free_slip( g_bt_task_slab, (void*)p_node );
}


void bt_bubble_sort(_u32* pData,_u32 Count)
{ 
	_u32 iTemp,i,j;
	for( i=1;i<Count;i++)
	{
		for( j=Count-1;j>=i;j--)
		{
			if(pData[j]<pData[j-1])
			{
				iTemp = pData[j-1];
				pData[j-1] = pData[j];
				pData[j] = iTemp;
			}
		}
	}
} 

//file_info_for_user malloc/free wrap
_int32 bt_file_info_for_user_malloc_wrap(void **pp_file_info_for_user,_u32 number,_u32 *file_index_array)
{
    _int32 ret_val = SUCCESS;
  	_u32 index,*file_index_array_tmp=NULL;
	ET_BT_FILE_INFO * bt_file_info_for_user=NULL;
  sd_assert(*pp_file_info_for_user == NULL);
    sd_assert(number != 0);

   ret_val = sd_malloc(sizeof(_u32)*number, (void**)&file_index_array_tmp);
    CHECK_VALUE(ret_val);
    sd_memcpy(file_index_array_tmp, file_index_array, sizeof(_u32)*number);

    bt_bubble_sort(file_index_array_tmp,number);

   ret_val = sd_malloc(sizeof(ET_BT_FILE_INFO)*number, (void**)pp_file_info_for_user);
    LOG_DEBUG( "bt_file_info_for_user_malloc_wrap:number=%u, ret_val=%d,*pp_file_info_for_user=0x%X,size=%u ",number, ret_val,*pp_file_info_for_user,sizeof(ET_BT_FILE_INFO)*number);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(file_index_array_tmp);
		CHECK_VALUE(ret_val);
	}

    sd_memset(*pp_file_info_for_user, 0, sizeof(ET_BT_FILE_INFO)*number);

	bt_file_info_for_user=(ET_BT_FILE_INFO * )(*pp_file_info_for_user);
	
	for(index=0;index<number;index++)
	{
		bt_file_info_for_user->_file_index=file_index_array_tmp[index];
		bt_file_info_for_user++;
	}

	SAFE_DELETE(file_index_array_tmp);
	
    return SUCCESS;
}

_int32 bt_file_info_for_user_free_wrap(void *p_file_info_for_user)
{
    _int32 ret_val = SUCCESS;
	
    LOG_DEBUG( "bt_file_info_for_user_free_wrap");
	sd_assert( p_file_info_for_user != NULL );
	if( p_file_info_for_user == NULL ) return SUCCESS;
	
	    ret_val = sd_free(p_file_info_for_user);
		p_file_info_for_user = NULL;
	    CHECK_VALUE(ret_val);
	return SUCCESS;
}
ET_BT_FILE_INFO* bt_file_info_for_user_binary_search(const ET_BT_FILE_INFO  bt_file_info_for_user_array[], _u32 file_index, _u32 file_num)
{

	_int32 Low, Mid, High;

	Low = 0;

	High = file_num - 1;

	while(Low <= High)

	{

		Mid = (Low + High) / 2; 

		if(bt_file_info_for_user_array[Mid]._file_index< file_index)

			Low = Mid + 1;

		else if(bt_file_info_for_user_array[Mid]._file_index > file_index)

			High = Mid - 1;

		else

			return (ET_BT_FILE_INFO*)(&bt_file_info_for_user_array[Mid]);

	}

	return NULL;
}

_int32 bt_file_info_for_user_write( BT_TASK *p_bt_task,BT_FILE_INFO* p_file_info )
{
	//_u32 index;
	ET_BT_FILE_INFO * bt_file_info_for_user=NULL;
    LOG_DEBUG( "bt_file_info_for_user_write:p_bt_task=0x%X,task_id=%u,file_index=%u,_file_num=%u",p_bt_task,p_bt_task->_task._task_id,p_file_info->_file_index,p_bt_task->_file_num);
	
	if(SUCCESS==cus_rws_begin_write_data(p_bt_task->_brd_ptr, NULL))
	{
		bt_file_info_for_user=bt_file_info_for_user_binary_search((ET_BT_FILE_INFO * )p_bt_task->_file_info_for_user, p_file_info->_file_index, p_bt_task->_file_num);
		sd_assert(bt_file_info_for_user!=NULL);

		bt_file_info_for_user->_file_size=p_file_info->_file_size;	
		bt_file_info_for_user->_downloaded_data_size=p_file_info->_downloaded_data_size;	
		bt_file_info_for_user->_written_data_size=p_file_info->_written_data_size;	
		bt_file_info_for_user->_file_percent=p_file_info->_file_percent;	
		bt_file_info_for_user->_file_status=(_u32)p_file_info->_file_status;	
		bt_file_info_for_user->_query_result=(_u32)p_file_info->_query_result;	
		bt_file_info_for_user->_sub_task_err_code=p_file_info->_sub_task_err_code;	
		bt_file_info_for_user->_has_record=p_file_info->_has_record;	
		bt_file_info_for_user->_accelerate_state=(_u32)p_file_info->_accelerate_state;	

		cus_rws_end_write_data(p_bt_task->_brd_ptr);
	}
	else
	{
    		LOG_DEBUG( "bt_file_info_for_user_write:failed!");
		//sd_assert(FALSE);
		return -1;
	}

	return SUCCESS;
}

_int32 bt_file_info_for_user_read(struct  tag_BT_FILE_INFO_POOL* p_bt_file_info_pool,_u32 file_index, void *p_file_info )
{
	_u32 index;
	ET_BT_FILE_INFO * bt_file_info_for_user=NULL;
	ET_BT_FILE_INFO * bt_file_info_for_user_out=(ET_BT_FILE_INFO * )p_file_info;
	_int32 ret_val =cus_rws_begin_read_data(p_bt_file_info_pool->_brd_ptr, NULL);
	
	if(SUCCESS==ret_val)
	{
		bt_file_info_for_user=bt_file_info_for_user_binary_search((ET_BT_FILE_INFO * )p_bt_file_info_pool->_file_info_for_user, file_index, p_bt_file_info_pool->_file_num);
		if(bt_file_info_for_user==NULL)
		{
    			LOG_DEBUG( "bt_file_info_for_user_read fatal ERROR:task_id=%u,file_index=%u,_file_num=%u,_brd_ptr=0x%X,_file_info_for_user=0x%X",p_bt_file_info_pool->_task_id,file_index,p_bt_file_info_pool->_file_num,p_bt_file_info_pool->_brd_ptr,p_bt_file_info_pool->_file_info_for_user);
			bt_file_info_for_user_out = (ET_BT_FILE_INFO * )p_bt_file_info_pool->_file_info_for_user;
			for(index=0;index<p_bt_file_info_pool->_file_num;index++)
			{
    				LOG_DEBUG( "_file_info_for_user[%d]->_file_index=%u",index,bt_file_info_for_user_out->_file_index);
				bt_file_info_for_user_out++;
			}
    			LOG_DEBUG( "bt_file_info_for_user_read fatal ERROR end!");
			//sd_assert(bt_file_info_for_user!=NULL);
			return BT_ERROR_FILE_INDEX;
		}

		bt_file_info_for_user_out->_file_index=bt_file_info_for_user->_file_index;	
		bt_file_info_for_user_out->_file_size=bt_file_info_for_user->_file_size;	
		bt_file_info_for_user_out->_downloaded_data_size=bt_file_info_for_user->_downloaded_data_size;	
		bt_file_info_for_user_out->_written_data_size=bt_file_info_for_user->_written_data_size;	
		bt_file_info_for_user_out->_file_percent=bt_file_info_for_user->_file_percent;	
		bt_file_info_for_user_out->_file_status=bt_file_info_for_user->_file_status;	
		bt_file_info_for_user_out->_query_result=bt_file_info_for_user->_query_result;	
		bt_file_info_for_user_out->_sub_task_err_code=bt_file_info_for_user->_sub_task_err_code;	
		bt_file_info_for_user_out->_has_record=bt_file_info_for_user->_has_record;	
		bt_file_info_for_user_out->_accelerate_state=bt_file_info_for_user->_accelerate_state;	

		cus_rws_end_read_data(p_bt_file_info_pool->_brd_ptr);
	}
	else
	{
		return ret_val;
	}

	
	LOG_DEBUG( "bt_file_info_for_user_read:_task_id=%u,_file_index=%u,_file_size=%llu,_downloaded_data_size=%llu,_written_data_size=%llu,_file_percent=%u,_file_status=%d,_query_result=%u,err_code=%u,_has_record=%u,_accelerate_state=%d" ,
		p_bt_file_info_pool->_task_id,file_index,bt_file_info_for_user_out->_file_size,bt_file_info_for_user_out->_downloaded_data_size,bt_file_info_for_user_out->_written_data_size,bt_file_info_for_user_out->_file_percent,bt_file_info_for_user_out->_file_status,bt_file_info_for_user_out->_query_result,bt_file_info_for_user_out->_sub_task_err_code,bt_file_info_for_user_out->_has_record,bt_file_info_for_user_out->_accelerate_state);

	return SUCCESS;
}


#ifdef UPLOAD_ENABLE
_int32 bt_add_record_to_upload_manager(TASK* p_task ,_u32 file_index)
{
	_int32 ret_val = SUCCESS;
	BT_TASK * p_bt_task = (BT_TASK *)p_task;
	TORRENT_FILE_INFO *p_file_info=NULL;
	char file_full_path[MAX_FULL_PATH_BUFFER_LEN]={0};
	char *task_data_path=NULL;
	_u32 task_data_path_len=0;
	_u8 cid[CID_SIZE]={0};
	_u8 gcid[CID_SIZE]={0};
	
	LOG_DEBUG("bt_add_record_to_upload_manager...p_task=0x%X,task_id=%u,file_index=%u",p_task,p_task->_task_id,file_index);
	
	ret_val = tp_get_file_info( p_bt_task->_torrent_parser_ptr,  file_index, &p_file_info );
	CHECK_VALUE(ret_val);

	if(p_file_info->_file_size<MIN_UPLOAD_FILE_SIZE) return SUCCESS;
	
	if((FALSE==bdm_get_cid( &(p_bt_task->_data_manager), file_index,cid))
		||(FALSE==sd_is_cid_valid(cid)))
	{
		LOG_DEBUG("bt_add_record_to_upload_manager...DT_ERR_GETTING_CID!");
		return (DT_ERR_GETTING_CID);
	}

	if((FALSE==bdm_get_calc_gcid( &(p_bt_task->_data_manager),  file_index,gcid))
		||(FALSE==sd_is_cid_valid(gcid)))
	{
		if((FALSE==bdm_get_shub_gcid( &(p_bt_task->_data_manager), file_index, gcid))
			||(FALSE==sd_is_cid_valid(gcid)))
			{
				LOG_DEBUG("bt_add_record_to_upload_manager...DT_ERR_INVALID_GCID!");
				return (DT_ERR_INVALID_GCID);
			}
	}
	

	ret_val = bdm_get_data_path(&(p_bt_task->_data_manager),&task_data_path,&task_data_path_len);
	CHECK_VALUE( ret_val );
	sd_assert(task_data_path!=NULL);
	sd_assert(task_data_path_len!=0);

	if(task_data_path_len+p_file_info->_file_path_len+p_file_info->_file_name_len>=MAX_FILE_PATH_LEN)
	{
		return DT_ERR_INVALID_FILE_PATH;
	}
	
	sd_assert(task_data_path_len+p_file_info->_file_path_len+p_file_info->_file_name_len<MAX_FILE_PATH_LEN); // MAX_FULL_PATH_LEN

	sd_strncpy( file_full_path, task_data_path, MAX_FULL_PATH_BUFFER_LEN );
	sd_strncpy(file_full_path+task_data_path_len, p_file_info->_file_path, MAX_FULL_PATH_BUFFER_LEN-task_data_path_len);
	sd_strncpy(file_full_path+task_data_path_len+p_file_info->_file_path_len, p_file_info->_file_name, MAX_FULL_PATH_BUFFER_LEN-task_data_path_len-p_file_info->_file_path_len);

	return ulm_add_record(p_file_info->_file_size, cid, gcid, NORMAL_HUB, file_full_path);
	
}
#endif


#endif /* #ifdef ENABLE_BT  */





