
#include "utility/define.h"
#ifdef ENABLE_BT


#include "bt_query.h"
#include "bt_accelerate.h"
#include "res_query/res_query_interface.h"
#include "utility/sd_assert.h"
#include "utility/utility.h"
#include "utility/time.h"
#include "utility/settings.h"
#include "connect_manager/connect_manager_interface.h"
#include "../bt_magnet/bt_magnet_task.h"

#include "utility/logid.h"
#ifdef LOGID
#undef LOGID
#endif
#define LOGID LOGID_BT_DOWNLOAD
#include "utility/logger.h"

_int32 bt_start_query_hub( BT_TASK *p_bt_task )
{
	_int32 ret_val = SUCCESS;
	BT_FILE_INFO *p_info = NULL;
	RES_QUERY_PARA *p_query_para = NULL;
	_u32 file_index = 0;
	_u8 *info_hash = NULL;

	MAP_ITERATOR cur_node;
	
	LOG_DEBUG( "bt_start_query_hub");
	
	cur_node = MAP_BEGIN( p_bt_task->_file_info_map );
	while( cur_node != MAP_END( p_bt_task->_file_info_map ) )
	{
		p_info = (BT_FILE_INFO *)MAP_VALUE( cur_node );
		file_index = (_u32)MAP_KEY( cur_node );
		LOG_DEBUG( "bt_try_to_query_hub.file_index:%u,file_status:%d, query_status:%d",	file_index, p_info->_file_status, p_info->_query_result );	
		
		if((p_info->_file_status != BT_FILE_DOWNLOADING )||(p_info->_query_result!=RES_QUERY_IDLE))
		{
			cur_node = MAP_NEXT( p_bt_task->_file_info_map, cur_node );
			continue;
		}

		ret_val = tp_get_file_info_hash( p_bt_task->_torrent_parser_ptr, &info_hash );
		if(ret_val!=SUCCESS) goto ErrorHanle;

		p_query_para=NULL;
		ret_val = bt_query_para_malloc_wrap( &p_query_para );
		if(ret_val!=SUCCESS) goto ErrorHanle;
		
		p_query_para->_file_index = file_index;
		p_query_para->_task_ptr = (TASK *)p_bt_task;

		LOG_DEBUG( "MMMM bt_start_query_hub.file_index:%u.", file_index );		
		ret_val = res_query_bt_info( p_query_para, bt_query_info_callback, info_hash,file_index, TRUE,++p_bt_task->_info_query_times);
		if(ret_val != SUCCESS)
		{
		        bt_query_para_free_wrap( p_query_para );
		        goto ErrorHanle;
		}

              ret_val = list_push( &p_bt_task->_query_para_list, (void *)p_query_para );
              if(ret_val != SUCCESS)
              {
                      res_query_cancel( p_query_para ,BT_HUB);
		        bt_query_para_free_wrap( p_query_para );
		        goto ErrorHanle;
              }
        
		p_info->_query_result = RES_QUERY_REQING;
		
		cur_node = MAP_NEXT( p_bt_task->_file_info_map, cur_node );
	}
	
	LOG_DEBUG( "bt_start_query_hub:SUCCESS");
	return SUCCESS;
	
ErrorHanle:
	bt_stop_query_hub( p_bt_task );
	dt_failure_exit( (TASK*)p_bt_task,ret_val );
	return ret_val;
	
}
_int32 bt_start_query_hub_for_single_file( BT_TASK *p_bt_task,BT_FILE_INFO *p_file_info,_u32 file_index )
{
	_int32 ret_val = SUCCESS;
	RES_QUERY_PARA *p_query_para = NULL;
	_u8 *info_hash = NULL;

	
	LOG_DEBUG( "MMMM bt_start_query_hub_for_single_file:_task_id=%u,_file_index=%u,file_name=%s,_file_size=%llu,_downloaded_data_size=%llu,_written_data_size=%llu,_file_percent=%u,_file_status=%d,_accelerate_state=%d" ,
		p_bt_task->_task._task_id,file_index,p_file_info->_file_name_str  ,p_file_info->_file_size,p_file_info->_downloaded_data_size,p_file_info->_written_data_size,p_file_info->_file_percent,p_file_info->_file_status,p_file_info->_accelerate_state);


	sd_assert(p_file_info->_file_status != BT_FILE_FINISHED);
	//sd_assert(p_file_info->_query_result==RES_QUERY_IDLE);

		ret_val = tp_get_file_info_hash( p_bt_task->_torrent_parser_ptr, &info_hash );
		CHECK_VALUE(ret_val);

		ret_val = bt_query_para_malloc_wrap( &p_query_para );
		CHECK_VALUE(ret_val);
		
		p_query_para->_file_index = file_index;
		p_query_para->_task_ptr = (TASK *)p_bt_task;

		ret_val = res_query_bt_info( p_query_para, bt_query_info_callback, info_hash,file_index, TRUE,++p_bt_task->_info_query_times);
		if(ret_val != SUCCESS)
		{
		        bt_query_para_free_wrap( p_query_para );
			CHECK_VALUE(ret_val);
		}

              ret_val = list_push( &p_bt_task->_query_para_list, (void *)p_query_para );
              if(ret_val != SUCCESS)
              {
                      res_query_cancel( p_query_para ,BT_HUB);
		        bt_query_para_free_wrap( p_query_para );
		        CHECK_VALUE(ret_val);
              }
        
		p_file_info->_query_result = RES_QUERY_REQING;
	
	LOG_DEBUG( "bt_start_query_hub_for_single_file:SUCCESS");
	return SUCCESS;
		
}

_int32 bt_stop_query_hub( BT_TASK *p_bt_task )
{
	_int32 ret_val = SUCCESS;
	BT_FILE_INFO *p_info = NULL;
	RES_QUERY_PARA *p_query_para = NULL;
	_u32 file_index = 0;
	_u32 _list_size = list_size(&(p_bt_task->_query_para_list)); 
	
	LOG_DEBUG( "MMMM bt_stop_query_hub:_info_query_timer_id=%u,_list_size=%u",p_bt_task->_info_query_timer_id,_list_size );

	if(p_bt_task->_info_query_timer_id!=0)
	{
		cancel_timer(p_bt_task->_info_query_timer_id);
		p_bt_task->_info_query_timer_id = 0;
	}

	while(_list_size)
	{
 		if(list_pop(&(p_bt_task->_query_para_list), (void**)&p_query_para)==SUCCESS) 
 		{
 			file_index = p_query_para->_file_index;
			ret_val = map_find_node(&(p_bt_task->_file_info_map) , (void*)file_index,(void**)&p_info);
			if(ret_val==SUCCESS)
			{
				sd_assert(p_info!=NULL);
				LOG_DEBUG( "bt_stop_query_hub:_task_id=%u,_file_index=%u,file_name=%s,_file_size=%llu,_downloaded_data_size=%llu,_written_data_size=%llu,_file_percent=%u,_file_status=%d,_accelerate_state=%d" ,
					p_bt_task->_task._task_id,file_index,p_info->_file_name_str  ,p_info->_file_size,p_info->_downloaded_data_size,p_info->_written_data_size,p_info->_file_percent,p_info->_file_status,p_info->_accelerate_state);
	
				if(p_info->_query_result==RES_QUERY_REQING)
					res_query_cancel(p_query_para, BT_HUB) ;
			
				p_info->_query_result=RES_QUERY_END;
			}
			bt_query_para_free_wrap( p_query_para );
 		}
		_list_size--;
	}
	
	LOG_DEBUG( "bt_stop_query_hub:SUCCESS" );
	return SUCCESS;
	
	
}
_int32 bt_query_delete_query_para(BT_TASK *p_bt_task,RES_QUERY_PARA *p_query_para)
{
	_int32 ret_val = SUCCESS;
    	LIST_ITERATOR  cur_node = NULL,next_node;
	RES_QUERY_PARA *p_value=NULL;
	
	     cur_node = LIST_BEGIN( p_bt_task->_query_para_list );
	    while(cur_node != LIST_END( p_bt_task->_query_para_list ) )
	    {
	    	next_node = LIST_NEXT( cur_node );
	        p_value = LIST_VALUE( cur_node );
	        if( p_value == p_query_para )
	        {
	             ret_val=list_erase( &p_bt_task->_query_para_list, cur_node );
			sd_assert(ret_val==SUCCESS);
	            
	            LOG_DEBUG( "bt_query_delete_query_para.list erase query para:0x%x", p_query_para );
	            break;
	        }
		cur_node = next_node;
	    }
	    //sd_assert( cur_node != LIST_END( p_bt_task->_query_para_list ) );
 
   		ret_val = bt_query_para_free_wrap( p_query_para );
		sd_assert(ret_val==SUCCESS);;
		
		return ret_val;

}
_int32 bt_query_info_callback( void* user_data
                              , _int32 errcode
                              ,  _u8 result
                              , BOOL has_record
                              , _u64 file_size
                              , _u8* cid
                              , _u8* gcid
                              , _u32 gcid_level
                              , _u8* bcid
                              , _u32 bcid_size
                              , _u32 block_size
                              , _int32 control_flag )
{
	_int32 ret_val = SUCCESS;
	RES_QUERY_PARA *p_query_para = (RES_QUERY_PARA *)user_data;
	BT_TASK *p_bt_task =NULL;
	BT_FILE_INFO * p_file_info=NULL;
	_u32 file_index = 0;

	LOG_DEBUG( "MMMM bt_query_info_callback.user_data=0x%X",user_data );

	sd_assert(user_data!=NULL);
	if(user_data==NULL) return INVALID_ARGUMENT;
	
	p_bt_task =(BT_TASK *)( p_query_para->_task_ptr);
	file_index = p_query_para->_file_index;

	ret_val = map_find_node(&(p_bt_task->_file_info_map), (void*)file_index,(void**)&p_file_info);
	if(ret_val!=SUCCESS) goto ErrorHanle;
	sd_assert(p_file_info!=NULL);

	LOG_DEBUG( "MMMM bt_query_info_callback:_task_id=%u,_file_index=%u,file_name=%s,_file_size=%llu,_downloaded_data_size=%llu,_written_data_size=%llu,_file_percent=%u,_file_status=%d,_accelerate_state=%d" ,
		p_bt_task->_task._task_id,file_index,p_file_info->_file_name_str  ,p_file_info->_file_size,p_file_info->_downloaded_data_size,p_file_info->_written_data_size,p_file_info->_file_percent,p_file_info->_file_status,p_file_info->_accelerate_state);
	
	LOG_DEBUG( "MMMM bt_query_info_callback:errcode=%d,result=%d,has_record=%d,file_size=%llu,gcid_level=%d,bcid_size=%u,block_size=%u,control_flag=%d" ,
		errcode,result,has_record  ,file_size,gcid_level,bcid_size,block_size,control_flag);

	if(BT_FILE_DOWNLOADING !=  bdm_get_file_status(& (p_bt_task->_data_manager),file_index))
	{
   		ret_val = bt_query_delete_query_para(p_bt_task,p_query_para);
		if(ret_val!=SUCCESS) goto ErrorHanle;
		
		p_file_info->_query_result = RES_QUERY_END;
		return SUCCESS;
	}

	if(( errcode == SUCCESS )&&(result==1))
	{
   		ret_val = bt_query_delete_query_para(p_bt_task,p_query_para);
		if(ret_val!=SUCCESS) goto ErrorHanle;
		
		p_file_info->_query_result = RES_QUERY_SUCCESS;

		if((has_record== TRUE)&&(file_size!=0)&&(sd_is_cid_valid(cid)) )
		{
			p_file_info->_control_flag = control_flag;
			p_file_info->_has_record= has_record;
			ret_val = bt_query_info_success_callback( p_bt_task, file_index, p_file_info ,has_record, file_size,	cid, gcid, gcid_level, bcid, bcid_size, block_size );
			if(ret_val!=SUCCESS) goto ErrorHanle;
		}
		else
		{
			p_file_info->_query_result = RES_QUERY_END;
            LOG_URGENT("bt_query_info_callback, bdm_shub_no_result, filesize = %llu, bcid_size = %d, block_size = %d"
                , file_size, bcid_size, block_size);
			ret_val = bdm_shub_no_result(  &(p_bt_task->_data_manager),file_index );
			if(ret_val!=SUCCESS) goto ErrorHanle;
			
		}

		
	}
	else
	{
		p_file_info->_query_result = RES_QUERY_FAILED;
		p_file_info->_sub_task_err_code = 131;

		/* Start timer */
		if(p_bt_task->_info_query_timer_id==0)
		{
			LOG_DEBUG( "start_timer(bt_handle_query_info_timeout)" );
			ret_val = start_timer(bt_handle_query_info_timeout, NOTICE_ONCE,REQ_RESOURCE_DEFAULT_TIMEOUT, 0,(void*)p_bt_task,&(p_bt_task->_info_query_timer_id));  
			if(ret_val!=SUCCESS) goto ErrorHanle;
		}
	}
	LOG_DEBUG( "MMMM bt_query_info_callback:SUCCESS" );
	return SUCCESS;

ErrorHanle:

	dt_failure_exit(( TASK*)p_bt_task,ret_val );
	return SUCCESS;
	
}

_int32 bt_query_info_success_callback( BT_TASK *p_bt_task, _u32 file_index,BT_FILE_INFO * p_file_info,BOOL has_record, _u64 file_size,
	_u8 *cid,_u8* gcid, _u32 gcid_level,_u8 *bcid, _u32 bcid_size, _u32 block_size )
{
	_int32 ret_val = SUCCESS;
	//BT_FILE_TASK_INFO * p_file_task=NULL;
	//_u32 max_bt_accelerate_num=DEFAULT_BT_ACCELERATE_NUM;
	//_u64 min_file_size=DEFAULT_BT_ACCELERATE_MIN_SIZE;

#ifdef _LOGGER
       char gcid_str[CID_SIZE*2+1];
         char cid_str[CID_SIZE*2+1];

#endif
	
	LOG_DEBUG( "bt_query_info_success_callback: _task_id= %u, file_index = %u,has_record=%u", p_bt_task->_task._task_id, file_index, has_record);

#ifdef _LOGGER
     //  char gcid_str[CID_SIZE*2+1]; 
//	 char cid_str[CID_SIZE*2+1]; 

       ret_val = str2hex((char*)gcid, CID_SIZE, gcid_str, CID_SIZE*2);
	CHECK_VALUE( ret_val );
	gcid_str[CID_SIZE*2] = '\0';

      ret_val = str2hex((char*)cid, CID_SIZE, cid_str, CID_SIZE*2);
	CHECK_VALUE( ret_val );
	cid_str[CID_SIZE*2] = '\0';


	LOG_DEBUG( "bt_query_info_success_callback: cid = %s, gcid = %s", cid_str, gcid_str );
#endif
	ret_val = bdm_set_cid( &(p_bt_task->_data_manager),file_index, cid);
	if( ret_val != SUCCESS )
	{
		LOG_DEBUG( "bt_query_info_success_callback:can not set the cid !!!!!" );
		return SUCCESS;
	}

    if(sd_is_cid_valid(gcid))
    {
        ret_val = bdm_set_gcid( &(p_bt_task->_data_manager),file_index, gcid);
        CHECK_VALUE( ret_val );
    }

    if(file_size != bdm_get_sub_file_size(&(p_bt_task->_data_manager),file_index) )
    {
        LOG_DEBUG("bt_query_info_success_callback, filesize = %llu, bdm_get_sub_file_size(&(p_bt_task->_data_manager),file_index) = %llu"
            , file_size, bdm_get_sub_file_size(&(p_bt_task->_data_manager),file_index) );
        return SUCCESS;
    }

	if((bcid!=NULL)&&(bcid_size!=0) && sd_is_bcid_valid(file_size, bcid_size/CID_SIZE, block_size))
	{
		ret_val = bdm_set_hub_return_info(&(p_bt_task->_data_manager),file_index,(_int32)gcid_level, bcid,bcid_size/CID_SIZE, block_size,file_size);
		CHECK_VALUE( ret_val );
	}
/*
	settings_get_int_item( "bt.max_bt_accelerate_num",(_int32*)&max_bt_accelerate_num);
	settings_get_int_item( "bt.min_bt_accelerate_file_size",(_int32*)&min_file_size);
	
	if((has_record==TRUE)&&(file_status==BT_FILE_DOWNLOADING)&&(file_size>min_file_size)&&
		(map_size(&(p_bt_task->_file_task_info_map))<max_bt_accelerate_num))
	{
		ret_val=bt_start_accelerate(p_bt_task,  file_index );
		CHECK_VALUE( ret_val );
	}
*/	
/*
	if(p_file_info->_accelerate_state==BT_ACCELERATING)
	{
		ret_val = map_find_node(&(p_bt_task->_file_task_info_map),(void*) file_index,(void**)&p_file_task);
		CHECK_VALUE( ret_val );
		
		p_file_task->_is_gcid_ok= bdm_get_shub_gcid( &(p_bt_task->_data_manager),file_index, gcid);
		p_file_task->_is_bcid_ok = bdm_bcid_is_valid( &(p_bt_task->_data_manager),file_index);

		if(has_record==TRUE)
		{
			ret_val =  bt_start_res_query_accelerate(p_bt_task , file_index,p_file_task);
			CHECK_VALUE( ret_val );
		}
		else
		{
			ret_val = bt_stop_accelerate( p_bt_task,  file_index );
			CHECK_VALUE( ret_val );
			sd_assert(FALSE);
		}
	}
*/	
	LOG_DEBUG( "bt_query_info_success_callback:SUCCESS" );
	return SUCCESS;
}


//typedef _int32 (*bt_add_bt_peer_resource_handler)(void* user_data, _u32 host_ip, _u16 port);
_int32 bt_add_bt_peer_resource(void* user_data, _u32 host_ip, _u16 port)
{
#ifdef ENABLE_BT_PROTOCOL
	TASK *p_task = (TASK *)user_data;
	BT_TASK * p_bt_task = (BT_TASK * )user_data;
	_u8 *info_hash=NULL;
	_int32 ret_val = SUCCESS;
	enum TASK_TYPE task_type = (enum TASK_TYPE)(*(_u32*)user_data);
	
	LOG_DEBUG( "AAAA MMMM bt_add_bt_peer_resource:_task_id=%u,host_ip=%u,port=%u",p_task->_task_id,host_ip,port);
	if( task_type == BT_MAGNET_TASK_TYPE )
    {
        return bmt_add_bt_resource( user_data, host_ip, port );
    }
	ret_val = tp_get_file_info_hash(  p_bt_task->_torrent_parser_ptr, &info_hash );
	CHECK_VALUE(ret_val);
	
	cm_add_bt_resource( &(p_task->_connect_manager),   host_ip,  port ,info_hash);

	LOG_DEBUG( "bt_add_bt_peer_resource:SUCCESS" );
#endif
	return SUCCESS;
}
#ifdef ENABLE_BT_PROTOCOL

_int32 bt_start_res_query_bt_tracker(BT_TASK * p_bt_task )
{
	_int32 ret_val = SUCCESS;
	char *tracker_url=NULL;
	_u8 *info_hash=NULL;
    	LIST_ITERATOR  cur_node = NULL;
	BOOL all_failed=TRUE;

#ifdef _DEBUG
	BOOL is_enable = TRUE;
	settings_get_bool_item( "bt.common_tracker_enable", (BOOL *)&is_enable );
	if( !is_enable) return SUCCESS;
#endif
	//----------------------------add by zhangshaohan------------------------------------
	if(res_query_bt_tracker_exist(p_bt_task) == TRUE)
		return SUCCESS;				//如果该任务正在查询，那么需要等待完成后再重查
	//----------------------------------------------------------------------------------
	LOG_DEBUG( "MMMM bt_start_res_query_bt_tracker:_task_id=%u",p_bt_task->_task._task_id);

	if(0!=list_size(p_bt_task->_tracker_url_list))
	{
	       ret_val = tp_get_file_info_hash(  p_bt_task->_torrent_parser_ptr, &info_hash );
		CHECK_VALUE(ret_val);
		
		p_bt_task->_res_query_bt_tracker_state = RES_QUERY_REQING;

		    for( cur_node = LIST_BEGIN( *(p_bt_task->_tracker_url_list) ); cur_node != LIST_END( *(p_bt_task->_tracker_url_list) ); cur_node = LIST_NEXT( cur_node ) )
		    {
		        tracker_url = (char*)LIST_VALUE( cur_node );
			LOG_DEBUG( "bt_start_res_query_bt_tracker:_task_id=%u,tracker_url=%s",p_bt_task->_task._task_id,tracker_url);
			ret_val = res_query_bt_tracker( p_bt_task,bt_res_query_bt_tracker_callback,tracker_url, info_hash);
			LOG_DEBUG( "bt_start_res_query_bt_tracker:ret_val=%d",ret_val);
			if(ret_val==SUCCESS) 
			{
				all_failed=FALSE;
			}
		    }

		if(all_failed==TRUE) 
		{
			p_bt_task->_res_query_bt_tracker_state = RES_QUERY_FAILED;
			LOG_DEBUG( "bt_start_res_query_bt_tracker:all_failed!!!" );
			//CHECK_VALUE(ret_val);
		}
	}
	LOG_DEBUG( "bt_start_res_query_bt_tracker:SUCCESS" );
	return SUCCESS;

}
_int32 bt_stop_res_query_bt_tracker(BT_TASK * p_bt_task )
{
#ifdef _DEBUG
	BOOL is_enable = TRUE;
	settings_get_bool_item( "bt.common_tracker_enable", (BOOL *)&is_enable );
	if( !is_enable) return SUCCESS;
#endif

	LOG_DEBUG( "MMMM bt_stop_res_query_bt_tracker:_task_id=%u,tracker_state =%d,tracker_timer_id=%u",p_bt_task->_task._task_id,p_bt_task->_res_query_bt_tracker_state,p_bt_task->_query_bt_tracker_timer_id);

	//if(p_bt_task->_res_query_bt_tracker_state == RES_QUERY_REQING)
	//{
		res_query_cancel(p_bt_task,BT_TRACKER);
		p_bt_task->_res_query_bt_tracker_state = RES_QUERY_END;
	//}
	/* Stop timer */
	if(p_bt_task->_query_bt_tracker_timer_id!=0)
	{
		cancel_timer(p_bt_task->_query_bt_tracker_timer_id);
		p_bt_task->_query_bt_tracker_timer_id = 0;
	}

	LOG_DEBUG( "bt_stop_res_query_bt_tracker:SUCCESS" );
	return SUCCESS;

}

_int32 bt_res_query_bt_tracker_callback(void* user_data,_int32 errcode )
{
	_int32 ret_val = SUCCESS;
	BT_TASK *p_bt_task = (BT_TASK *)user_data;
    
	LOG_DEBUG( "MMMM bt_res_query_bt_tracker_callback:_task_id=%u,errcode=%d,tracker_timer_id=%u.",p_bt_task->_task._task_id,errcode,p_bt_task->_query_bt_tracker_timer_id );
	
	if( errcode == SUCCESS )
	{
		p_bt_task->_res_query_bt_tracker_state = RES_QUERY_SUCCESS;
		cm_create_pipes(&(p_bt_task->_task._connect_manager));
	}
	else
	{
		p_bt_task->_res_query_bt_tracker_state = RES_QUERY_FAILED;
		ret_val = bt_start_res_query_bt_tracker(p_bt_task );
	}

	/* Start timer */
	if(p_bt_task->_query_bt_tracker_timer_id==0)
	{
		LOG_DEBUG("start_timer(bt_handle_query_bt_tracker_timeout)");
		ret_val = start_timer(bt_handle_query_bt_tracker_timeout, NOTICE_ONCE,REQ_RESOURCE_DEFAULT_TIMEOUT, 0,(void*)p_bt_task,&(p_bt_task->_query_bt_tracker_timer_id));  
		if(ret_val!=SUCCESS) goto ErrorHanle;
	}
	LOG_DEBUG( "bt_res_query_bt_tracker_callback:SUCCESS" );
	return SUCCESS;
	
ErrorHanle:

	dt_failure_exit( (TASK*)p_bt_task,ret_val );
	return SUCCESS;
}

#endif	



_int32 bt_start_res_query_dht(BT_TASK * p_bt_task )
{
	_int32 ret_val = SUCCESS;
	_u8 *info_hash=NULL;
#ifdef _DK_QUERY
    LIST *p_seed_dht_nodes = NULL;
#endif		
	
#ifdef _DEBUG
	BOOL is_enable = TRUE;
	settings_get_bool_item( "bt.dht_enable", (BOOL *)&is_enable );
	if( !is_enable) return SUCCESS;
#endif	

	LOG_DEBUG( "MMMM bt_start_res_query_dht:_task_id=%u",p_bt_task->_task._task_id);

       ret_val = tp_get_file_info_hash(  p_bt_task->_torrent_parser_ptr, &info_hash );
	CHECK_VALUE(ret_val);
	
	p_bt_task->_res_query_bt_dht_state = RES_QUERY_REQING;
	ret_val =-1;// res_query_bt_dht( p_bt_task,bt_res_query_dht_callback, info_hash);
	
#ifdef _DK_QUERY
	ret_val = res_query_dht( p_bt_task, info_hash );

#endif
	
	if(ret_val!=SUCCESS) 
	{
		p_bt_task->_res_query_bt_dht_state = RES_QUERY_FAILED;
		LOG_DEBUG( "bt_start_res_query_dht:RES_QUERY_FAILED" );
		return ret_val;
	}

	LOG_DEBUG( "bt_start_res_query_dht:SUCCESS" );

	return SUCCESS;

}
_int32 bt_stop_res_query_dht(BT_TASK * p_bt_task )
{

#ifdef _DEBUG
	BOOL is_enable = TRUE;
	settings_get_bool_item( "bt.dht_enable", (BOOL *)&is_enable );
	if( !is_enable) return SUCCESS;
#endif	

	LOG_DEBUG( "MMMM bt_stop_res_query_dht:_task_id=%u,dht_state =%d,dht_timer_id=%u",p_bt_task->_task._task_id,p_bt_task->_res_query_bt_dht_state,p_bt_task->_query_dht_timer_id);

	if(p_bt_task->_res_query_bt_dht_state == RES_QUERY_REQING)
	{
#ifdef _DK_QUERY
		res_query_cancel(p_bt_task,BT_DHT);
#endif	
		p_bt_task->_res_query_bt_dht_state = RES_QUERY_END;
	}
	
	/* Stop timer */
	if(p_bt_task->_query_dht_timer_id!=0)
	{
		cancel_timer(p_bt_task->_query_dht_timer_id);
		p_bt_task->_query_dht_timer_id = 0;
	}

	LOG_DEBUG( "bt_stop_res_query_dht:SUCCESS" );
	return SUCCESS;

}

_int32 bt_res_query_dht_callback(void* user_data,_int32 errcode )
{
	_int32 ret_val = SUCCESS;
	BT_TASK *p_bt_task = (BT_TASK *)user_data;
    
	LOG_DEBUG( "MMMM bt_res_query_dht_callback:_task_id=%u,errcode=%d,dht_timer_id=%u.",p_bt_task->_task._task_id,errcode,p_bt_task->_query_dht_timer_id );
	
	if( errcode == SUCCESS )
	{
		p_bt_task->_res_query_bt_dht_state = RES_QUERY_SUCCESS;
		cm_create_pipes(&(p_bt_task->_task._connect_manager));
	}
	else
	{
		p_bt_task->_res_query_bt_dht_state = RES_QUERY_FAILED;
		//ret_val = bt_start_res_query_dht(p_bt_task );
	}

	/* Start timer */
	if(p_bt_task->_query_dht_timer_id==0)
	{
		LOG_DEBUG("start_timer(bt_handle_query_dht_timeout)");
		ret_val = start_timer(bt_handle_query_dht_timeout, NOTICE_ONCE,REQ_RESOURCE_DEFAULT_TIMEOUT, 0,(void*)p_bt_task,&(p_bt_task->_query_dht_timer_id));  
		if(ret_val!=SUCCESS) goto ErrorHanle;
	}
	
	LOG_DEBUG( "bt_res_query_dht_callback:SUCCESS" );
	return SUCCESS;
	
ErrorHanle:

	dt_failure_exit( (TASK*)p_bt_task,ret_val );
	return SUCCESS;


}






 /*--------------------------------------------------------------------------*/
/*                Callback function for timer				        */
/*--------------------------------------------------------------------------*/
_int32 bt_handle_query_info_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid)
 {
	_int32 ret_val = SUCCESS;
	TASK * p_task = NULL;
	 BT_TASK *p_bt_task = NULL;
	LIST_ITERATOR p_list_node=NULL ;
	RES_QUERY_PARA *p_res_query_para =NULL;
	BT_FILE_INFO * p_file_info=NULL;
	_u32 file_index=0;
	_u8 * info_hash=NULL;
	 
	LOG_DEBUG("bt_handle_query_info_timeout:errcode=%d,notice_count_left=%u,expired=%u,msgid=%u",errcode,notice_count_left,expired,msgid);

	if(errcode == MSG_TIMEOUT)
	{
		p_task = (TASK *)(msg_info->_user_data);
		p_bt_task = (BT_TASK *)p_task;
		sd_assert(p_bt_task!=NULL);
		if(p_bt_task==NULL) return INVALID_ARGUMENT;	
		
		LOG_DEBUG("bt_handle_query_info_timeout:_task_id=%u,info_query_timer_id=%u,task_status=%d,list_size( &p_bt_task->_query_para_list )=%u",p_task->_task_id,p_bt_task->_info_query_timer_id,p_task->task_status,list_size( &p_bt_task->_query_para_list ));
		if(/*msg_info->_device_id*/msgid == p_bt_task->_info_query_timer_id)
		{
		
			p_bt_task->_info_query_timer_id=0;
			
			if(p_task->task_status == TASK_S_RUNNING)
			{
				if(0!=list_size( &p_bt_task->_query_para_list ))
				{
					ret_val = tp_get_file_info_hash( p_bt_task->_torrent_parser_ptr, &info_hash );
					if(ret_val!=SUCCESS) goto ErrorHanle;
					
					for(p_list_node = LIST_BEGIN(p_bt_task->_query_para_list); p_list_node != LIST_END(p_bt_task->_query_para_list); p_list_node = LIST_NEXT(p_list_node))
					{
						p_res_query_para = (RES_QUERY_PARA * )LIST_VALUE(p_list_node);
						file_index = p_res_query_para->_file_index;

						ret_val = map_find_node(&(p_bt_task->_file_info_map),(void*) file_index,(void**)&p_file_info);
						if(ret_val!=SUCCESS) goto ErrorHanle;
						sd_assert(p_file_info!=NULL);
						
						if((p_file_info->_query_result== RES_QUERY_FAILED)&&(bdm_get_file_status(& (p_bt_task->_data_manager),file_index)==BT_FILE_DOWNLOADING)) 
						{

							LOG_DEBUG( "MMMM bt_retry_query_hub.file_index:%u.", file_index );		
							ret_val = res_query_bt_info( p_res_query_para, bt_query_info_callback, info_hash,file_index, TRUE,++p_bt_task->_info_query_times);
							if(ret_val!=SUCCESS) goto ErrorHanle;
					        
							p_file_info->_query_result = RES_QUERY_REQING;
						}
					}
					
					
				}
			}
		}
	}
		
	LOG_DEBUG( "bt_handle_query_info_timeout:SUCCESS" );
	return SUCCESS;

ErrorHanle:

	dt_failure_exit( p_task,ret_val );
	return SUCCESS;


 }

#ifdef ENABLE_BT_PROTOCOL

_int32 bt_handle_query_bt_tracker_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid)
 {
	_int32 ret_val = SUCCESS;
	TASK * p_task = (TASK *)(msg_info->_user_data);
	BT_TASK *p_bt_task = (BT_TASK *)(msg_info->_user_data);
	 
	LOG_DEBUG("bt_handle_query_bt_tracker_timeout:errcode=%d,notice_count_left=%u,expired=%u,msgid=%u",errcode,notice_count_left,expired,msgid);

	if(errcode == MSG_TIMEOUT)
	{
		sd_assert(p_bt_task!=NULL);
		if(p_bt_task==NULL) return INVALID_ARGUMENT;	
		
		LOG_DEBUG("bt_handle_query_bt_tracker_timeout:_task_id=%u,tracker_timer_id=%u,task_status=%d,tracker_state=%d,need_more_res=%d",p_task->_task_id,p_bt_task->_query_bt_tracker_timer_id,p_task->task_status,p_bt_task->_res_query_bt_tracker_state,cm_is_global_need_more_res());
		if(/*msg_info->_device_id*/msgid == p_bt_task->_query_bt_tracker_timer_id)
		{
		
			p_bt_task->_query_bt_tracker_timer_id=0;
			
			if((p_task->task_status == TASK_S_RUNNING)&&(p_bt_task->_res_query_bt_tracker_state != RES_QUERY_REQING))
			{
				if((cm_is_global_need_more_res())&&(cm_is_need_more_peer_res( &(p_task->_connect_manager), INVALID_FILE_INDEX )))
				{
					ret_val = bt_start_res_query_bt_tracker( p_bt_task );
					if(ret_val!=SUCCESS) goto ErrorHanle;
					
				}
				else
				{
					/* Start timer */
					LOG_DEBUG("start_timer(bt_handle_query_bt_tracker_timeout)");
					ret_val = start_timer(bt_handle_query_bt_tracker_timeout, NOTICE_ONCE,REQ_RESOURCE_DEFAULT_TIMEOUT, 0,(void*)p_bt_task,&(p_bt_task->_query_bt_tracker_timer_id));  
					if(ret_val!=SUCCESS) goto ErrorHanle;
				}
			}
		}
	}
		
	LOG_DEBUG( "bt_handle_query_bt_tracker_timeout:SUCCESS" );
	return SUCCESS;

ErrorHanle:

	dt_failure_exit( p_task,ret_val );
	return SUCCESS;


 }
#endif

_int32 bt_handle_query_dht_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid)
 {
	_int32 ret_val = SUCCESS;
	TASK * p_task = (TASK *)(msg_info->_user_data);
	BT_TASK *p_bt_task = (BT_TASK *)(msg_info->_user_data);
	 
	LOG_DEBUG("bt_handle_query_dht_timeout:errcode=%d,notice_count_left=%u,expired=%u,msgid=%u",errcode,notice_count_left,expired,msgid);

	if(errcode == MSG_TIMEOUT)
	{
		sd_assert(p_bt_task!=NULL);
		if(p_bt_task==NULL) return INVALID_ARGUMENT;	
		
		LOG_DEBUG("bt_handle_query_dht_timeout:_task_id=%u,dht_timer_id=%u,task_status=%d,dht_state=%d,need_more_res=%d",p_task->_task_id,p_bt_task->_query_dht_timer_id,p_task->task_status,p_bt_task->_res_query_bt_dht_state,cm_is_global_need_more_res());
		if(/*msg_info->_device_id*/msgid == p_bt_task->_query_dht_timer_id)
		{
		
			p_bt_task->_query_dht_timer_id=0;
			
			if((p_task->task_status == TASK_S_RUNNING)&&(p_bt_task->_res_query_bt_dht_state != RES_QUERY_REQING))
			{
				if((cm_is_global_need_more_res())&&(cm_is_need_more_peer_res( &(p_task->_connect_manager), INVALID_FILE_INDEX )))
				{
					ret_val = bt_start_res_query_dht( p_bt_task );
					if(ret_val!=SUCCESS) goto ErrorHanle;
					
				}
				else
				{
					/* Start timer */
					LOG_DEBUG("start_timer(bt_handle_query_dht_timeout)");
					ret_val = start_timer(bt_handle_query_dht_timeout, NOTICE_ONCE,REQ_RESOURCE_DEFAULT_TIMEOUT, 0,(void*)p_bt_task,&(p_bt_task->_query_dht_timer_id));  
					if(ret_val!=SUCCESS) goto ErrorHanle;
				}
			}
		}
	}
		
	LOG_DEBUG( "bt_handle_query_dht_timeout:SUCCESS" );
	return SUCCESS;

ErrorHanle:

	dt_failure_exit( p_task,ret_val );
	return SUCCESS;


 }




#endif /* #ifdef ENABLE_BT  */



