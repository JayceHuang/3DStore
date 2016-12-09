
#include "utility/define.h"
#ifdef ENABLE_BT

#include "bt_accelerate.h"
#include "bt_query.h"
#include "../bt_data_manager/bt_data_manager.h"
#include "res_query/res_query_interface.h"
#include "utility/settings.h"
#include "utility/sd_assert.h"
#include "utility/utility.h"
#include "utility/time.h"
#include "utility/define_const_num.h"
#include "connect_manager/connect_manager_interface.h"
#ifdef ENABLE_HSC
#include "high_speed_channel/hsc_info.h"
#endif /* ENABLE_HSC */
#include "utility/logid.h"
#ifdef LOGID
#undef LOGID
#endif

#define LOGID LOGID_BT_DOWNLOAD

#include "utility/logger.h"

_int32 bt_build_res_query_dphub_content(BT_TASK *p_bt_task, BT_FILE_TASK_INFO *p_bt_file_task_info, _u32 bt_file_idx);

_int32 bt_start_accelerate( BT_TASK *p_bt_task,BT_FILE_INFO *p_file_info, _u32 file_index, BOOL* is_start_succ )
{
	BT_FILE_TASK_INFO *p_file_task_info = NULL;
	//BT_FILE_INFO *p_file_info = NULL;
	TASK * p_task = (TASK *)p_bt_task;
	_int32 ret_val = SUCCESS;

	PAIR info_map_node;
	_u8 gcid[CID_SIZE];

	*is_start_succ = FALSE;


	LOG_DEBUG( "NNNN bt_start_accelerate.file_index=:%u", file_index );		
	ret_val = bdm_notify_start_speedup_sub_file(  &(p_bt_task->_data_manager),file_index);
	if( ret_val !=SUCCESS) 
	{
		LOG_DEBUG( "NNNN bt_start_accelerate FAILED!.file_index=:%u,ret_val=%d", file_index,ret_val );	
		if(ret_val!=BT_CREATE_FILE_LATER)
		{
			p_file_info->_accelerate_state=BT_ACCELERATE_END;
		}
		return ret_val;
	}

	ret_val = cm_create_sub_connect_manager( &(p_task->_connect_manager),  file_index );
	if(ret_val!=SUCCESS)
	{
		if (ret_val != MAP_DUPLICATE_KEY)
		{
			bdm_notify_stop_speedup_sub_file( &(p_bt_task->_data_manager),  file_index );
			p_file_info->_accelerate_state=BT_ACCELERATE_END;
		}
		//CHECK_VALUE( ret_val );
		return ret_val;

	}

	ret_val = bt_file_task_info_malloc_wrap( &p_file_task_info );
	if(ret_val!=SUCCESS)
	{
		cm_delete_sub_connect_manager( &(p_task->_connect_manager),  file_index );
		bdm_notify_stop_speedup_sub_file( &(p_bt_task->_data_manager),  file_index );
		p_file_info->_accelerate_state=BT_ACCELERATE_END;
		CHECK_VALUE( ret_val );
	}

	sd_memset(p_file_task_info,0,sizeof(BT_FILE_TASK_INFO));

	p_file_task_info->_file_status= BT_FILE_DOWNLOADING;
	p_file_task_info->_is_gcid_ok= bdm_get_shub_gcid( &(p_bt_task->_data_manager),file_index, gcid);
	p_file_task_info->_is_bcid_ok = bdm_bcid_is_valid( &(p_bt_task->_data_manager),file_index);
	p_file_task_info->_res_query_para._task_ptr=p_task;
	p_file_task_info->_res_query_para._file_index=file_index;
	init_dphub_query_context(&p_file_task_info->_dpub_query_context);

	info_map_node._key = (void *)file_index;
	info_map_node._value = (void *)p_file_task_info;

	ret_val = map_insert_node( &p_bt_task->_file_task_info_map, &info_map_node );
	if(ret_val!=SUCCESS)
	{
		cm_delete_sub_connect_manager( &(p_task->_connect_manager),  file_index );
		bdm_notify_stop_speedup_sub_file( &(p_bt_task->_data_manager),  file_index );
		bt_file_task_info_free_wrap(p_file_task_info);
		p_file_info->_accelerate_state=BT_ACCELERATE_END;
		CHECK_VALUE( ret_val );
	}

	if(bdm_get_cid(  &(p_bt_task->_data_manager),file_index, gcid)==TRUE)
	{
		ret_val =  bt_start_res_query_accelerate(p_bt_task , file_index,p_file_task_info);
		if(ret_val!=SUCCESS) goto ErrorHanle;
	}
	else
		if(p_file_info->_query_result!=RES_QUERY_REQING)
		{
			ret_val =  bt_start_query_hub_for_single_file(p_bt_task ,p_file_info, file_index);
			//ret_val=-1;

			sd_assert(ret_val==SUCCESS);
			if(ret_val!=SUCCESS) goto ErrorHanle;
		}
		ret_val = map_find_node(&(p_bt_task->_file_info_map) , (void*)file_index,(void**)&p_file_info);
		if(ret_val!=SUCCESS) goto ErrorHanle;

		p_file_info->_accelerate_state=BT_ACCELERATING;

		*is_start_succ = TRUE;

#if defined(ENABLE_HSC) && !defined(ENABLE_ETM_MEDIA_CENTER)
		hsc_set_force_query_permission( &p_bt_task->_task._hsc_info );
#endif /* ENABLE_HSC */
		LOG_DEBUG( "bt_start_accelerate:_task_id=%u,_file_index=%u,file_name=%s,_file_size=%llu,_downloaded_data_size=%llu,_written_data_size=%llu,_file_percent=%u,_file_status=%d,_accelerate_state=%d" ,
			p_bt_task->_task._task_id,file_index,p_file_info->_file_name_str  ,p_file_info->_file_size,p_file_info->_downloaded_data_size,p_file_info->_written_data_size,p_file_info->_file_percent,p_file_info->_file_status,p_file_info->_accelerate_state);

		//insert assist peer res
		if(p_task->_use_assist_peer)
		{
			_int64 file_size = bfm_get_sub_file_size( &(p_bt_task->_data_manager._bt_file_manager), file_index ); 
			_u8 gcid[CID_SIZE+1] = {0};
			if(bfm_get_shub_gcid( &(p_bt_task->_data_manager._bt_file_manager), file_index, gcid))
			{
				_int32 ret = cm_add_cdn_peer_resource( 
					&(p_task->_connect_manager), 
					file_index, 
					p_task->_p_assist_res->edt_p_res._peer_id, 
					gcid, 
					p_file_info->_file_size, 
					p_task->_p_assist_res->edt_p_res._peer_capability, 
					p_task->_p_assist_res->edt_p_res._host_ip, 
					p_task->_p_assist_res->edt_p_res._tcp_port , 
					p_task->_p_assist_res->edt_p_res._udp_port, 
					p_task->_p_assist_res->edt_p_res._res_level, 
					P2P_FROM_ASSIST_DOWNLOAD );
				LOG_DEBUG("insert assist peer res when single file start, ret = %d, ip = %d, file_index= %u", ret, p_task->_p_assist_res->edt_p_res._host_ip, file_index);
			}
		}

		LOG_DEBUG( "bt_start_accelerate:SUCCESS" );
		return SUCCESS;

ErrorHanle:

		bt_file_task_failure_exit( p_bt_task, file_index,ret_val );
		return SUCCESS;
}

_int32 bt_res_query_normal_cdn_callback(void* user_data, _int32 errcode, _u8 result, _u32 retry_interval)
{
	_int32 ret_val = SUCCESS;
	TASK * p_task = (TASK *)((RES_QUERY_PARA *)user_data)->_task_ptr;
	BT_TASK * p_bt_task = (BT_TASK *)p_task;
	BT_FILE_TASK_INFO * p_file_task=NULL;
	_u32 file_index=(_u32)((RES_QUERY_PARA *)user_data)->_file_index;

	LOG_DEBUG("bt_res_query_normal_cdn_callback:user_data=0x%X",user_data);

	sd_assert(user_data!=NULL);
	if(user_data==NULL) return INVALID_ARGUMENT;

	ret_val = map_find_node(&(p_bt_task->_file_task_info_map), (void*)file_index,(void**)&p_file_task);
	CHECK_VALUE(ret_val);
	sd_assert(p_file_task!=NULL);

	LOG_DEBUG( "bt_res_query_normal_cdn_callback:_task_id=%u,_file_index=%u,_file_status=%d,res_query_timer_id=%u,last_query_stamp=%u,query_shub_times_sn=%d,res_query_retry_count=%d,error_code=%d,shub_state=%d,phub_state=%d,tracker_state=%d,partner_cdn_state=%u,is_gcid_ok=%d,is_bcid_ok=%d,is_add_rc_ok=%d" ,
		p_bt_task->_task._task_id,file_index,p_file_task->_file_status	,p_file_task->_res_query_timer_id,p_file_task->_last_query_stamp,p_file_task->_query_shub_times_sn,p_file_task->_res_query_retry_count,p_file_task->_error_code,p_file_task->_res_query_shub_state,p_file_task->_res_query_phub_state,p_file_task->_res_query_tracker_state,p_file_task->_res_query_partner_cdn_state,p_file_task->_is_gcid_ok,p_file_task->_is_bcid_ok,p_file_task->_is_add_rc_ok);


	LOG_DEBUG( "bt_res_query_normal_cdn_callback:errcode=%d,result=%d,retry_interval=%u" ,
		errcode,result,retry_interval);

	if(BT_FILE_DOWNLOADING !=  bdm_get_file_status(& (p_bt_task->_data_manager),file_index))
	{
		p_file_task->_res_query_normal_cdn_state= RES_QUERY_END;
		return SUCCESS;
	}

	if((errcode !=SUCCESS)||(result!=SUCCESS))
	{
		LOG_DEBUG("MMMM Query normal cdn failed!");
		p_file_task->_res_query_normal_cdn_state = RES_QUERY_FAILED;
#ifdef EMBED_REPORT
		p_task->_res_query_report_para._normal_cdn_fail++;
#endif	
		
	}else
	{
		LOG_DEBUG("MMMM Query normal cdn success!");
		p_file_task->_res_query_normal_cdn_state = RES_QUERY_SUCCESS;

		cm_create_sub_manager_pipes(&(p_bt_task->_task._connect_manager), file_index );
	}

	/* Start timer */
	if(p_file_task->_res_query_timer_id == 0)
	{
		LOG_DEBUG("_res_query_normal_cdn_state:start_timer(bt_handle_query_accelerate_timeout)");
		ret_val =  start_timer(bt_handle_query_accelerate_timeout, NOTICE_INFINITE,REQ_RESOURCE_DEFAULT_TIMEOUT, 0,user_data,&(p_file_task->_res_query_timer_id));
		if(ret_val!=SUCCESS) 
		{
			bt_file_task_failure_exit( p_bt_task, file_index,ret_val );
		}
	}
	LOG_DEBUG("_res_query_normal_cdn_state:SUCCESS");
	return SUCCESS;
}

static _int32 bt_query_other_hub(BT_TASK *p_bt_task
						  , BT_FILE_TASK_INFO * p_file_task
						  , _u32 file_index
						  , _u8* cid
						  , _u64 file_size)
{
	_int32 ret_val = SUCCESS;
	_u8 gcid[CID_SIZE] = {0};
	TASK* p_task = (TASK*)p_bt_task;
	_int32 bonus_res_num = DEFAULT_BONUS_RES_NUM;	
	if(bdm_get_shub_gcid( &(p_bt_task->_data_manager),file_index, gcid)==TRUE)
	{
		if((p_file_task->_res_query_phub_state != RES_QUERY_REQING)&&(cm_is_need_more_peer_res( &(p_task->_connect_manager)
				,file_index  )))
		{
			p_file_task->_res_query_phub_state = RES_QUERY_REQING;
			LOG_DEBUG( "MMMM bt_start_res_query_accelerate:res_query_phub" );
			ret_val = res_query_phub(&(p_file_task->_res_query_para), bt_res_query_phub_callback
				,cid, gcid, file_size,  20, 4);
			if(ret_val!=SUCCESS) 
			{
				p_file_task->_res_query_phub_state = RES_QUERY_FAILED;
			}
			else
			{
#ifdef EMBED_REPORT
				sd_time_ms( &(p_task->_res_query_report_para._phub_query_time) );
#endif	
			}			
		}

		// 查询dphub
		if((p_file_task->_res_query_dphub_state != RES_QUERY_REQING) && 
			(cm_is_need_more_peer_res(&(p_task->_connect_manager), file_index)))
		{
			ret_val = bt_build_res_query_dphub_content(p_bt_task, p_file_task, file_index);

			if (ret_val == ERR_RES_QUERY_NO_NEED_QUERY_DPHUB)
				p_file_task->_res_query_dphub_state = RES_QUERY_END;
			else
				p_file_task->_res_query_dphub_state 
				= (ret_val != SUCCESS) ? RES_QUERY_FAILED : RES_QUERY_REQING;
		}

#ifdef ENABLE_HSC	
		if((p_file_task->_res_query_vip_hub_state != RES_QUERY_REQING)&&(cm_is_need_more_peer_res( &(p_task->_connect_manager),file_index  )))
		{			
			p_file_task->_res_query_vip_hub_state = RES_QUERY_REQING;
			LOG_DEBUG( "MMMM bt_start_res_query_accelerate:res_query_vip_hub" );
			ret_val = res_query_vip_hub(&(p_file_task->_res_query_para), bt_res_query_vip_hub_callback,cid, gcid, file_size,  bonus_res_num,4);
			if(ret_val!=SUCCESS) 
			{
				p_file_task->_res_query_vip_hub_state = RES_QUERY_FAILED;
			}
		}
#endif /* ENABLE_HSC */

		if((p_file_task->_res_query_tracker_state != RES_QUERY_REQING)&&(cm_is_need_more_peer_res( &(p_task->_connect_manager),file_index  )))
		{
			p_file_task->_res_query_tracker_state = RES_QUERY_REQING;
			LOG_DEBUG( "MMMM bt_start_res_query_accelerate:res_query_tracker" );
			ret_val = res_query_tracker(&(p_file_task->_res_query_para)
				,bt_res_query_tracker_callback,p_file_task->_last_query_stamp,gcid,  file_size,MAX_PEER,20,0,0);
			if(ret_val!=SUCCESS) 
			{
				p_file_task->_res_query_tracker_state = RES_QUERY_FAILED;
			}
			else
			{
#ifdef EMBED_REPORT
				sd_time_ms( &(p_task->_res_query_report_para._tracker_query_time) );
#endif	
			}				
		}


#if defined( MOBILE_PHONE)
		p_file_task->_res_query_partner_cdn_state = RES_QUERY_END;
#else

		if((p_file_task->_res_query_partner_cdn_state != RES_QUERY_REQING)&&(p_file_task->_res_query_partner_cdn_state != RES_QUERY_SUCCESS)&&
			(cm_is_need_more_peer_res( &(p_task->_connect_manager),file_index  )))
		{
			p_file_task->_res_query_partner_cdn_state = RES_QUERY_REQING;
			LOG_DEBUG( "MMMM bt_start_res_query_accelerate:res_query_partner_cdn" );
#if defined(MACOS) && defined(ENABLE_CDN)
			ret_val = res_query_cdn_manager(CDN_VERSION,gcid,file_size,bt_res_query_cdn_manager_callback,&(p_file_task->_res_query_para));
#else
			ret_val = res_query_partner_cdn(&(p_file_task->_res_query_para),bt_res_query_partner_cdn_callback
				, cid,gcid, file_size);
#endif /* MACOS */
			if(ret_val!=SUCCESS) 
			{
				p_file_task->_res_query_partner_cdn_state = RES_QUERY_FAILED;
			}
		}
#endif	/* MOBILE_PHONE */

		if( (p_file_task->_res_query_normal_cdn_state != RES_QUERY_SUCCESS)&&
			(p_file_task->_res_query_normal_cdn_state != RES_QUERY_END)&&
			(p_file_task->_res_query_normal_cdn_state != RES_QUERY_REQING)&&
			(p_file_task->_res_query_normal_cdn_cnt < 3 )&&
			cm_is_need_more_peer_res( &p_task->_connect_manager,file_index  )  )
		{
			LOG_DEBUG( "MMMM bt_start_res_query_accelerate:res_query_normal_cdn" );
			ret_val = res_query_normal_cdn_manager(&(p_file_task->_res_query_para), 
				bt_res_query_normal_cdn_callback, cid, gcid,file_size, DEFAULT_BONUS_RES_NUM, 4);
			if(ret_val!=SUCCESS) 
			{
				LOG_DEBUG( "MMMM bt_start_res_query_accelerate:res_query_normal_cdn start fail ret_val=%d",ret_val);
				p_file_task->_res_query_phub_state = RES_QUERY_FAILED;
			}
			else
			{
				p_file_task->_res_query_normal_cdn_state = RES_QUERY_REQING;
				p_file_task->_res_query_normal_cdn_cnt++;
			}
		}
	}
	return ret_val;
}

_int32 bt_start_next_accelerate( BT_TASK *p_bt_task)
{
	_int32 ret_val = SUCCESS;
	MAP_ITERATOR cur_node ;
	BT_FILE_INFO *p_file_info =NULL,*p_candidate=NULL;
	_u32 file_index = 0/*,max_bt_accelerate_num=DEFAULT_BT_ACCELERATE_NUM*/;
	_u64 min_file_size=DEFAULT_BT_ACCELERATE_MIN_SIZE;

	//settings_get_int_item( "bt.max_bt_accelerate_num",(_int32*)&max_bt_accelerate_num);
	settings_get_int_item( "bt.min_bt_accelerate_file_size",(_int32*)&min_file_size);

	LOG_DEBUG( "bt_start_next_accelerate:map_size(&(p_bt_task->_file_task_info_map))=%u,"
        ,map_size(&(p_bt_task->_file_info_map)) );


	for(cur_node = MAP_BEGIN( p_bt_task->_file_info_map ); cur_node != MAP_END(  p_bt_task->_file_info_map ); cur_node = MAP_NEXT(  p_bt_task->_file_info_map, cur_node ))
	{
		p_file_info = (BT_FILE_INFO *)MAP_VALUE( cur_node );
		file_index = (_u32 )MAP_KEY( cur_node );

		LOG_DEBUG( "bt_start_next_accelerate:_task_id=%u,_file_index=%u,file_name=%s,_file_size=%llu,_downloaded_data_size=%llu,_written_data_size=%llu,_file_percent=%u,_file_status=%d,_query_result=%d,_accelerate_state=%d" ,
		p_bt_task->_task._task_id,file_index,p_file_info->_file_name_str  
        ,p_file_info->_file_size,p_file_info->_downloaded_data_size
        ,p_file_info->_written_data_size,p_file_info->_file_percent
        ,p_file_info->_file_status,p_file_info->_query_result,p_file_info->_accelerate_state);

		if((p_file_info->_file_size>(min_file_size*1024))&&(p_file_info->_file_status==BT_FILE_DOWNLOADING) 
			&&(p_file_info->_accelerate_state != BT_ACCELERATING)
			&&  p_file_info->_sub_task_err_code !=  BT_FILE_DOWNLOAD_SUCCESS_WAIT_CHECK)
		{
			/*
			if((p_file_info->_query_result==RES_QUERY_IDLE)||(p_file_info->_query_result==RES_QUERY_REQING))
			{
			if(p_candidate==NULL)
			{
			p_candidate=p_file_info;
			}
			else
			{
			if(p_file_info->_file_size>p_candidate->_file_size)
			{
			p_candidate=p_file_info;
			}
			else
			{
			continue;
			}
			}
			}
			else
			*/
			if(p_file_info->_query_result==RES_QUERY_SUCCESS)
			{
				if(p_file_info->_has_record==TRUE)
				{
					if(p_candidate==NULL)
					{
						p_candidate=p_file_info;
					}
					else
					{
						if(p_file_info->_file_index<p_candidate->_file_index)
						{
							p_candidate=p_file_info;
						}
						else
						{
							continue;
						}
					}
				}
				else
				{
					continue;
				}
			}
		}
	}

	if(p_candidate!=NULL)
	{
		BOOL is_start_succ = FALSE;
		ret_val=bt_start_accelerate(p_bt_task,p_candidate,  p_candidate->_file_index, &is_start_succ);
		if( ret_val !=SUCCESS) return ret_val;
	}

	LOG_DEBUG( "bt_start_next_accelerate:SUCCESS" );
	return SUCCESS;
}



_int32 bt_update_accelerate_sub( BT_TASK *p_bt_task,BT_FILE_TASK_INFO *p_file_task_info,_u32 file_index )
{
	//TASK * p_task = (TASK *)p_bt_task;
	//_int32 ret_val = SUCCESS;


	LOG_DEBUG( "bt_update_accelerate_sub.file_index=:%d", file_index );		


	return SUCCESS;

}

_int32 bt_update_accelerate( BT_TASK *p_bt_task )
{
	BT_FILE_TASK_INFO *p_file_task_info = NULL;
	MAP_ITERATOR cur_node;
	_u32 file_index=0;


	LOG_DEBUG( "bt_update_accelerate:map_size(&(p_bt_task->_file_task_info_map))=%u",map_size(&(p_bt_task->_file_task_info_map)) );


	cur_node = MAP_BEGIN( p_bt_task->_file_task_info_map );

	while( cur_node != MAP_END( p_bt_task->_file_task_info_map ) )
	{
		p_file_task_info = (BT_FILE_TASK_INFO *)MAP_VALUE( cur_node );
		file_index=(_u32)MAP_KEY( cur_node );

		bt_update_accelerate_sub( p_bt_task,p_file_task_info, file_index );

		cur_node = MAP_NEXT( p_bt_task->_file_task_info_map, cur_node );
	}	

	LOG_DEBUG( "bt_update_accelerate:SUCCESS" );
	return SUCCESS;

}

_int32 bt_stop_accelerate( BT_TASK *p_bt_task, _u32 file_index )
{
	BT_FILE_TASK_INFO *p_file_task_info = NULL;
	_int32 ret_val = SUCCESS;


	LOG_DEBUG( "bt_stop_accelerate.file_index=:%d", file_index );		

	ret_val = map_find_node(&(p_bt_task->_file_task_info_map), (void*)file_index,(void**)&p_file_task_info);
	CHECK_VALUE( ret_val );
	sd_assert(p_file_task_info!=NULL);

	uninit_dphub_query_context(&p_file_task_info->_dpub_query_context);
	bt_stop_accelerate_sub(p_bt_task, p_file_task_info, file_index);    
	map_erase_node(&(p_bt_task->_file_task_info_map), (void*)file_index);

	LOG_DEBUG( "bt_stop_accelerate:SUCCESS" );
	return SUCCESS;

}

_int32 bt_clear_accelerate( BT_TASK *p_bt_task )
{
	BT_FILE_TASK_INFO *p_file_task_info = NULL;
	MAP_ITERATOR cur_node, next_node;
	_u32 file_index=0;


	LOG_DEBUG( "bt_clear_accelerate:map_size(&(p_bt_task->_file_task_info_map))=%u",map_size(&(p_bt_task->_file_task_info_map)) );


	cur_node = MAP_BEGIN( p_bt_task->_file_task_info_map );

	while( cur_node != MAP_END( p_bt_task->_file_task_info_map ) )
	{
		p_file_task_info = (BT_FILE_TASK_INFO *)MAP_VALUE( cur_node );
		file_index=(_u32)MAP_KEY( cur_node );
		next_node = MAP_NEXT( p_bt_task->_file_task_info_map, cur_node );

		uninit_dphub_query_context(&p_file_task_info->_dpub_query_context);
		bt_stop_accelerate_sub(p_bt_task ,p_file_task_info,file_index );
		map_erase_iterator(&(p_bt_task->_file_task_info_map), cur_node );

		cur_node = next_node;
	}	

	LOG_DEBUG( "bt_clear_accelerate:SUCCESS" );
	return SUCCESS;

}

_int32 bt_stop_accelerate_sub( BT_TASK *p_bt_task ,BT_FILE_TASK_INFO *p_file_task_info, _u32 file_index )
{
	TASK * p_task = (TASK *)p_bt_task;
	BT_FILE_INFO *p_file_info = NULL;
	_int32 ret_val = SUCCESS;


	LOG_DEBUG( "NNNN bt_stop_accelerate_sub. _task_id:0x%x, file_index:%d, file_status = %d, _res_query_timer_id:%d,_res_query_retry_count=%d,_error_code=%d,_res_query_shub_state=%d,_res_query_phub_state=%d,_res_query_tracker_state=%d,partner_cdn_state=%d,_is_gcid_ok=%d,_is_bcid_ok=%d, res_query_dphub_state=%d", 
		p_task->_task_id, file_index, p_file_task_info->_file_status, p_file_task_info->_res_query_timer_id , p_file_task_info->_res_query_retry_count,p_file_task_info->_error_code,p_file_task_info->_res_query_shub_state,p_file_task_info->_res_query_phub_state,p_file_task_info->_res_query_tracker_state,p_file_task_info->_res_query_partner_cdn_state,p_file_task_info->_is_gcid_ok,p_file_task_info->_is_bcid_ok, p_file_task_info->_res_query_dphub_state);
	//if(p_file_info->_query_result != RES_QUERY_REQING)
	//{
	if(p_file_task_info->_res_query_timer_id!=0)
	{
		cancel_timer(p_file_task_info->_res_query_timer_id);
		p_file_task_info->_res_query_timer_id=0;
	}


	bt_stop_res_query_accelerate(p_file_task_info);
	//}

	cm_delete_sub_connect_manager( &(p_task->_connect_manager),  file_index );

	bdm_notify_stop_speedup_sub_file( &(p_bt_task->_data_manager),  file_index );

	ret_val = map_find_node(&(p_bt_task->_file_info_map ), (void*)file_index,(void**)&p_file_info);
	if(ret_val==SUCCESS)
	{
		sd_assert(p_file_info!=NULL);
		p_file_info->_accelerate_state=BT_ACCELERATE_END;
		LOG_DEBUG( "bt_stop_accelerate_sub:_task_id=%u,_file_index=%u,file_name=%s,_file_size=%llu,_downloaded_data_size=%llu,_written_data_size=%llu,_file_percent=%u,_file_status=%d,_accelerate_state=%d" ,
			p_bt_task->_task._task_id,file_index,p_file_info->_file_name_str  ,p_file_info->_file_size,p_file_info->_downloaded_data_size,p_file_info->_written_data_size,p_file_info->_file_percent,p_file_info->_file_status,p_file_info->_accelerate_state);

	}
	bt_file_task_info_free_wrap(p_file_task_info);

	LOG_DEBUG( "bt_stop_accelerate_sub:SUCCESS" );
	return SUCCESS;

}

_int32 file_task_info_map_compare( void *E1, void *E2 )
{
	_u32 value1 = (_u32)E1;
	_u32 value2 = (_u32)E2;
	LOG_DEBUG( "file_info_map_compare_fun:value1=%u,value2=%u",value1,value2 );	
	return value1 > value2 ? 1 : ( value1 < value2 ? -1 : 0 );  
}

_int32 file_index_set_compare( void *E1, void *E2 )
{
	_u32 value1 = (_u32)E1;
	_u32 value2 = (_u32)E2;
	LOG_DEBUG( "file_index_set_compare:value1=%u,value2=%u",value1,value2 );	
	return value1 > value2 ? 1 : ( value1 < value2 ? -1 : 0 );  
}

_int32 bt_build_res_query_dphub_content(BT_TASK *p_bt_task, BT_FILE_TASK_INFO *p_bt_file_task_info, _u32 bt_file_idx)
{
	_int32 ret_val = 0;
	_int16 file_src_type = 1; // 1: 普通完整文件
	_u8 file_idx_desc_type = 0; // 0：无块描述
	_u32 file_idx_content_cnt = 0;
	char *file_idx_content = NULL;

	_u8 cid[CID_SIZE] = {'\0'};
	_u8 gcid[CID_SIZE] = {'\0'};

	LOG_DEBUG("MMMM bt_task:res_query_dphub");

	bdm_get_cid(&p_bt_task->_data_manager, bt_file_idx, cid);
	bdm_get_shub_gcid(&p_bt_task->_data_manager, bt_file_idx, gcid);
	ret_val = res_query_dphub(&(p_bt_file_task_info->_res_query_para), 
		bt_res_query_dphub_callback, 
		file_src_type, 
		file_idx_desc_type,
		file_idx_content_cnt,
		file_idx_content, 
		bdm_get_file_size(&(p_bt_task->_data_manager)), 
		bdm_get_file_block_size(&(p_bt_task->_data_manager), bt_file_idx),        
		(const char *)cid,
		(const char *)gcid,
		(_u16)BT_TASK_TYPE);  

	p_bt_file_task_info->_res_query_dphub_state = (ret_val != SUCCESS) ? RES_QUERY_FAILED : RES_QUERY_REQING;    

	if (ret_val == ERR_RES_QUERY_NO_ROOT)
	{
		p_bt_file_task_info->_res_query_dphub_state = RES_QUERY_IDLE;
		if (p_bt_file_task_info->_dpub_query_context._wait_dphub_root_timer_id == 0)
		{
			ret_val = start_timer(bt_handle_wait_dphub_root_timeout, 1, WAIT_FOR_DPHUB_ROOT_RETURN_TIMEOUT, 0, 
				(void *)&(p_bt_file_task_info->_res_query_para), 
				&(p_bt_file_task_info->_dpub_query_context._wait_dphub_root_timer_id));
			if(ret_val!=SUCCESS)
			{
				dt_failure_exit((TASK *)p_bt_task, ret_val);
			}
		}        
	}

	return ret_val;
}

_int32 bt_start_res_query_accelerate(BT_TASK * p_bt_task ,_u32 file_index,BT_FILE_TASK_INFO * p_file_task)
{
	_int32 ret_val = SUCCESS,max_query_shub_retry_count=MAX_QUERY_SHUB_RETRY_TIMES;
	TASK *p_task=(TASK*)p_bt_task;
	_int32 bonus_res_num = DEFAULT_BONUS_RES_NUM;
	_u8 cid[CID_SIZE],gcid[CID_SIZE];
	_u64 file_size = 0;

	LOG_DEBUG( "bt_start_res_query_accelerate:_task_id=%u,_file_index=%u,_file_status=%d,res_query_timer_id=%u,last_query_stamp=%u,query_shub_times_sn=%d,res_query_retry_count=%d,error_code=%d,shub_state=%d,phub_state=%d,tracker_state=%d,partner_cdn_state=%u,is_gcid_ok=%d,is_bcid_ok=%d,is_add_rc_ok=%d, res_query_dphub_state=%d" ,
		p_bt_task->_task._task_id,file_index,p_file_task->_file_status  ,p_file_task->_res_query_timer_id,p_file_task->_last_query_stamp,p_file_task->_query_shub_times_sn,p_file_task->_res_query_retry_count,p_file_task->_error_code,p_file_task->_res_query_shub_state,p_file_task->_res_query_phub_state,p_file_task->_res_query_tracker_state,p_file_task->_res_query_partner_cdn_state,p_file_task->_is_gcid_ok,p_file_task->_is_bcid_ok,p_file_task->_is_add_rc_ok, p_file_task->_res_query_dphub_state);

	if(bdm_get_cid( &(p_bt_task->_data_manager), file_index, cid)==TRUE)
	{
		file_size = bdm_get_sub_file_size(&(p_bt_task->_data_manager),file_index);
		settings_get_int_item("res_query.max_query_shub_retry_count",&max_query_shub_retry_count);
		if(p_file_task->_res_query_retry_count<max_query_shub_retry_count)
		{
			/* Query the shub for resources */
			if((p_file_task->_res_query_shub_state != RES_QUERY_REQING)&&(cm_is_need_more_server_res( &(p_task->_connect_manager),file_index )))
			{
				p_file_task->_res_query_retry_count++;
				p_file_task->_res_query_shub_state = RES_QUERY_REQING;

				if(p_file_task->_res_query_shub_step != QUERY_SHUB_SERVER_RES_STEP)
				{
					ret_val =  res_query_shub_by_cid_newcmd(&(p_file_task->_res_query_para)
						, bt_res_query_shub_callback,cid,file_size ,TRUE,(const char*)gcid
						,	TRUE, MAX_SERVER, bonus_res_num, p_file_task->_query_shub_times_sn++,3);
				}
				else
				{
					ret_val =  res_query_shub_by_resinfo_newcmd(&(p_file_task->_res_query_para), bt_query_only_res_shub_callback,cid,file_size ,TRUE,(const char*)gcid,	TRUE, MAX_SERVER,bonus_res_num, p_file_task->_query_shub_newres_times_sn++,3);
				}

				if(ret_val!=SUCCESS) 
				{
					p_file_task->_res_query_shub_state = RES_QUERY_FAILED;
				}
				else
				{
#ifdef EMBED_REPORT
					sd_time_ms( &(p_task->_res_query_report_para._shub_query_time) );
#endif	
				}
			}
		}
		else
		{
			LOG_DEBUG("MMMM Already retry res_query_shub %d times,do not retry again! END OF RES_QUERY_SHUB!",p_file_task->_res_query_retry_count);
		}
	}
	else
	{
		ret_val =DT_ERR_GETTING_CID;
		LOG_DEBUG( "bt_start_res_query_accelerate:DT_ERR_GETTING_CID" );
		return ret_val;
	}

	LOG_DEBUG( "bt_start_res_query_accelerate:SUCCESS" );
	return SUCCESS;

}

_int32 bt_stop_res_query_accelerate(BT_FILE_TASK_INFO *p_file_task_info)
{

	LOG_DEBUG("bt_stop_res_query_accelerate.");

	if(p_file_task_info->_res_query_shub_state==RES_QUERY_REQING)
	{
		res_query_cancel(&(p_file_task_info->_res_query_para), SHUB);
		p_file_task_info->_res_query_shub_state=RES_QUERY_END;
	}

	if(p_file_task_info->_res_query_phub_state==RES_QUERY_REQING)
	{
		res_query_cancel(&(p_file_task_info->_res_query_para), PHUB);
		p_file_task_info->_res_query_phub_state=RES_QUERY_END;
	}
#ifdef ENABLE_HSC
	if(p_file_task_info->_res_query_vip_hub_state==RES_QUERY_REQING)
	{
		res_query_cancel(&(p_file_task_info->_res_query_para), VIP_HUB);
		p_file_task_info->_res_query_vip_hub_state=RES_QUERY_END;
	}
#endif /* ENABLE_HSC */
	if(p_file_task_info->_res_query_tracker_state==RES_QUERY_REQING)
	{
		res_query_cancel(&(p_file_task_info->_res_query_para), TRACKER);
		p_file_task_info->_res_query_tracker_state=RES_QUERY_END;
	}

	if(p_file_task_info->_res_query_partner_cdn_state==RES_QUERY_REQING)
	{
#if defined(MACOS) && defined(ENABLE_CDN)
		res_query_cancel(&(p_file_task_info->_res_query_para), CDN_MANAGER);
#else
		res_query_cancel(&(p_file_task_info->_res_query_para), PARTNER_CDN);
#endif /* MACOS */
		p_file_task_info->_res_query_partner_cdn_state=RES_QUERY_END;
	}

	if(p_file_task_info->_res_query_normal_cdn_state==RES_QUERY_REQING)
	{
		res_query_cancel(&(p_file_task_info->_res_query_para), NORMAL_CDN_MANAGER);
		p_file_task_info->_res_query_normal_cdn_state = RES_QUERY_END;
	}

	//if (p_file_task_info->_res_query_dphub_state == RES_QUERY_REQING)
	{
		res_query_cancel(&p_file_task_info->_res_query_para, DPHUB_NODE);
	}

	LOG_DEBUG( "bt_stop_res_query_accelerate:SUCCESS ");
	return SUCCESS;
}

_int32 bt_query_only_res_shub_callback(void* user_data,_int32 errcode
									   , _u8 result, _u64 file_size, _u8* cid, _u8* gcid, _u32 gcid_level
									   , _u8* bcid, _u32 bcid_size, _u32 block_size,_u32 retry_interval,char * file_suffix,_int32 control_flag)
{
	_u8 _gcid_t[CID_SIZE],*bcid_from_dm=NULL;
	_int32 ret_val = SUCCESS;
	TASK * p_task = (TASK *)((RES_QUERY_PARA *)user_data)->_task_ptr;
	BT_TASK * p_bt_task = (BT_TASK *)p_task;
	BT_FILE_TASK_INFO * p_file_task=NULL;
	_u32 file_index=(_u32)((RES_QUERY_PARA *)user_data)->_file_index;
	_u32 num_of_bcid_from_dm=0;

#ifdef EMBED_REPORT
	_u32 cur_time = 0;
	_u32 shub_query_time = 0;
	RES_QUERY_REPORT_PARA *p_report_para = &p_task->_res_query_report_para;
#endif

	LOG_DEBUG("bt_query_only_res_shub_callback:user_data=0x%X",user_data);

	sd_assert(user_data!=NULL);
	if(user_data==NULL) return INVALID_ARGUMENT;

	ret_val = map_find_node(&(p_bt_task->_file_task_info_map),(void*) file_index,(void**)&p_file_task);
	CHECK_VALUE(ret_val);
	sd_assert(p_file_task!=NULL);

	sd_memset(_gcid_t,0,CID_SIZE);

	LOG_DEBUG( "bt_query_only_res_shub_callback:_task_id=%u,_file_index=%u,_file_status=%d,res_query_timer_id=%u,last_query_stamp=%u,query_shub_times_sn=%d,res_query_retry_count=%d,error_code=%d,shub_state=%d,phub_state=%d,tracker_state=%d,partner_cdn_state=%u,is_gcid_ok=%d,is_bcid_ok=%d,is_add_rc_ok=%d" ,
		p_bt_task->_task._task_id,file_index,p_file_task->_file_status  ,p_file_task->_res_query_timer_id,p_file_task->_last_query_stamp,p_file_task->_query_shub_times_sn,p_file_task->_res_query_retry_count,p_file_task->_error_code,p_file_task->_res_query_shub_state,p_file_task->_res_query_phub_state,p_file_task->_res_query_tracker_state,p_file_task->_res_query_partner_cdn_state,p_file_task->_is_gcid_ok,p_file_task->_is_bcid_ok,p_file_task->_is_add_rc_ok);

	LOG_DEBUG( "bt_query_only_res_shub_callback:errcode=%d,result=%d,file_size=%llu,gcid_level=%u,bcid_size=%u,block_size=%u,retry_interval=%u, file_suffix=%s,control_flag=%d" ,
		errcode,result,file_size,gcid_level,bcid_size,block_size,retry_interval, file_suffix,control_flag);

	if(p_file_task->_res_query_shub_state != RES_QUERY_REQING)
		CHECK_VALUE( -1);




	if(BT_FILE_DOWNLOADING !=  bdm_get_file_status(& (p_bt_task->_data_manager),file_index))
	{
		p_file_task->_res_query_shub_state = RES_QUERY_END;
		return SUCCESS;
	}



#ifdef EMBED_REPORT
	sd_time_ms( &cur_time );
	shub_query_time = TIME_SUBZ( cur_time, p_report_para->_shub_query_time );

	p_report_para->_hub_s_max = MAX(p_report_para->_hub_s_max, shub_query_time);
	if( p_report_para->_hub_s_min == 0 )
	{
		p_report_para->_hub_s_min = shub_query_time;
	}

	p_report_para->_hub_s_min = MIN(p_report_para->_hub_s_min, shub_query_time);
	p_report_para->_hub_s_avg = (p_report_para->_hub_s_avg*(p_report_para->_hub_s_succ+p_report_para->_hub_s_fail)+shub_query_time) / (p_report_para->_hub_s_succ+p_report_para->_hub_s_fail+1);

#endif	

	if(errcode == SUCCESS)
	{
		p_file_task->_res_query_shub_state = RES_QUERY_SUCCESS;
	}
	else
	{
		p_file_task->_res_query_shub_state = RES_QUERY_FAILED;
	}

	if(p_file_task->_res_query_timer_id == 0)
	{
		LOG_DEBUG("bt_res_query_shub_callback:start_timer(bt_handle_query_accelerate_timeout)");
		ret_val =  start_timer(bt_handle_query_accelerate_timeout, NOTICE_INFINITE,REQ_RESOURCE_DEFAULT_TIMEOUT, 0,user_data,&(p_file_task->_res_query_timer_id));
		if(ret_val!=SUCCESS) goto ErrorHanle;
	}

	return SUCCESS;
ErrorHanle:

	bt_file_task_failure_exit( p_bt_task, file_index,ret_val );
	return SUCCESS;
}
_int32 bt_res_query_shub_callback(void* user_data,_int32 errcode, _u8 result
								  , _u64 file_size, _u8* cid, _u8* gcid, _u32 gcid_level
								  , _u8* bcid, _u32 bcid_size, _u32 block_size
								  ,_u32 retry_interval,char * file_suffix,_int32 control_flag)
{
	_u8 _gcid_t[CID_SIZE],*bcid_from_dm=NULL;
	_int32 ret_val = SUCCESS;
	TASK * p_task = (TASK *)((RES_QUERY_PARA *)user_data)->_task_ptr;
	BT_TASK * p_bt_task = (BT_TASK *)p_task;
	BT_FILE_TASK_INFO * p_file_task=NULL;
	_u32 file_index=(_u32)((RES_QUERY_PARA *)user_data)->_file_index;
	_u32 num_of_bcid_from_dm=0;

#ifdef EMBED_REPORT
	_u32 cur_time = 0;
	_u32 shub_query_time = 0;
	RES_QUERY_REPORT_PARA *p_report_para = &p_task->_res_query_report_para;
#endif

	LOG_DEBUG("bt_res_query_shub_callback:user_data=0x%X",user_data);

	sd_assert(user_data!=NULL);
	if(user_data==NULL) return INVALID_ARGUMENT;

	ret_val = map_find_node(&(p_bt_task->_file_task_info_map),(void*) file_index,(void**)&p_file_task);
	CHECK_VALUE(ret_val);
	sd_assert(p_file_task!=NULL);

	sd_memset(_gcid_t,0,CID_SIZE);

	LOG_DEBUG( "bt_res_query_shub_callback:_task_id=%u,_file_index=%u,_file_status=%d,res_query_timer_id=%u,last_query_stamp=%u,query_shub_times_sn=%d,res_query_retry_count=%d,error_code=%d,shub_state=%d,phub_state=%d,tracker_state=%d,partner_cdn_state=%u,is_gcid_ok=%d,is_bcid_ok=%d,is_add_rc_ok=%d" ,
		p_bt_task->_task._task_id,file_index,p_file_task->_file_status  ,p_file_task->_res_query_timer_id,p_file_task->_last_query_stamp,p_file_task->_query_shub_times_sn,p_file_task->_res_query_retry_count,p_file_task->_error_code,p_file_task->_res_query_shub_state,p_file_task->_res_query_phub_state,p_file_task->_res_query_tracker_state,p_file_task->_res_query_partner_cdn_state,p_file_task->_is_gcid_ok,p_file_task->_is_bcid_ok,p_file_task->_is_add_rc_ok);

	LOG_DEBUG( "bt_res_query_shub_callback:errcode=%d,result=%d,file_size=%llu,gcid_level=%u,bcid_size=%u,block_size=%u,retry_interval=%u, file_suffix=%s,control_flag=%d" ,
		errcode,result,file_size,gcid_level,bcid_size,block_size,retry_interval, file_suffix,control_flag);

	if(p_file_task->_res_query_shub_state != RES_QUERY_REQING)
		CHECK_VALUE( -1);




	if(BT_FILE_DOWNLOADING !=  bdm_get_file_status(& (p_bt_task->_data_manager),file_index))
	{
		p_file_task->_res_query_shub_state = RES_QUERY_END;
		return SUCCESS;
	}

	p_file_task->_res_query_shub_step = QUERY_SHUB_RES_INFO_STEP;

#ifdef EMBED_REPORT
	sd_time_ms( &cur_time );
	shub_query_time = TIME_SUBZ( cur_time, p_report_para->_shub_query_time );

	p_report_para->_hub_s_max = MAX(p_report_para->_hub_s_max, shub_query_time);
	if( p_report_para->_hub_s_min == 0 )
	{
		p_report_para->_hub_s_min = shub_query_time;
	}

	p_report_para->_hub_s_min = MIN(p_report_para->_hub_s_min, shub_query_time);
	p_report_para->_hub_s_avg = (p_report_para->_hub_s_avg*(p_report_para->_hub_s_succ+p_report_para->_hub_s_fail)+shub_query_time) / (p_report_para->_hub_s_succ+p_report_para->_hub_s_fail+1);

#endif	

	if(errcode == SUCCESS)
	{
#ifdef EMBED_REPORT
		p_report_para->_hub_s_succ++;
#endif	
		p_file_task->_res_query_shub_state = RES_QUERY_SUCCESS;
		if((result !=SUCCESS)||(FALSE==sd_is_cid_valid(gcid))  || !sd_is_cid_valid(cid) )
		{
			/* res_query failed!*/ 
			LOG_DEBUG("MMMM Query shub failed!");
			p_file_task->_res_query_shub_state = RES_QUERY_FAILED;
            LOG_URGENT("bt_res_query_shub_callback, bdm_shub_no_result, filesize = %llu, bcid_size = %d, block_size = %d"
                , file_size, bcid_size, block_size);
			ret_val = bdm_shub_no_result( &(p_bt_task->_data_manager),file_index);
			if(ret_val!=SUCCESS) goto ErrorHanle;
		}
		else
		{

			LOG_DEBUG("MMMM Query shub success!");

			if(p_file_task->_is_gcid_ok==TRUE && bdm_get_shub_gcid( &(p_bt_task->_data_manager),file_index, _gcid_t)==TRUE)
			{
				if(FALSE==sd_is_cid_equal(_gcid_t,gcid))
				{
				//sd_assert(FALSE);
					ret_val = DT_ERR_INVALID_GCID;
					LOG_DEBUG("DT_ERR_INVALID_GCID!");
					bdm_set_gcid(&(p_bt_task->_data_manager),file_index, gcid);
					//goto ErrorHanle;
				}
			}
			else
			{
				if(sd_is_cid_valid(gcid))
				{
					ret_val = bdm_set_gcid( &(p_bt_task->_data_manager),file_index, gcid);
					CHECK_VALUE( ret_val );
				}
			}

			if(file_size!=0)
			{
				if(file_size != bdm_get_sub_file_size(&(p_bt_task->_data_manager), file_index))
				{
					LOG_URGENT("DT_ERR_INVALID_FILE_SIZE, file_index = %d, filesize from shub = %lld, bt_torrent filesize = %llu",
						file_index, file_size, bdm_get_sub_file_size(&(p_bt_task->_data_manager), file_index) );
					sd_assert(FALSE);
					ret_val = DT_ERR_INVALID_FILE_SIZE;
					goto ErrorHanle;
				}
			}

			p_file_task->_control_flag = control_flag;

			if((bcid!=NULL)&&(bcid_size!=0)&&sd_is_bcid_valid(file_size, bcid_size/CID_SIZE, block_size))
			{
				cm_create_sub_manager_pipes(&(p_bt_task->_task._connect_manager), file_index );
				if(bdm_bcid_is_valid( &(p_bt_task->_data_manager),file_index)==FALSE )
				{
					ret_val = bdm_set_hub_return_info(&(p_bt_task->_data_manager),file_index,(_int32)gcid_level, bcid,bcid_size/CID_SIZE, block_size,file_size);
					if(ret_val!=SUCCESS) goto ErrorHanle;
					p_file_task->_is_bcid_ok = TRUE;
				}
				else
				{
					if(TRUE==bdm_get_bcids( &(p_bt_task->_data_manager),file_index,&num_of_bcid_from_dm, &bcid_from_dm))
					{
						if(FALSE==sd_is_bcid_equal(bcid_from_dm,num_of_bcid_from_dm*CID_SIZE,bcid,bcid_size))
						{


#ifdef _LOGGER  
							char  bcid_str[CID_SIZE*2+1];
							_u32 i =0;
							if(num_of_bcid_from_dm*CID_SIZE == bcid_size)
							{								 	
								for(i=0; i<num_of_bcid_from_dm; i++)
								{
									str2hex((char*)(bcid + i*CID_SIZE), CID_SIZE,bcid_str, CID_SIZE*2);
									bcid_str[CID_SIZE*2] = '\0';   
									LOG_DEBUG("bt_res_query_shub_callback : bcidno %u:%s",i, bcid_str);
								}
							}
#endif 	  	 	   
							bdm_set_hub_return_info2(&(p_bt_task->_data_manager)
								, file_index
								, (_int32)gcid_level
								, bcid
								, bcid_size/CID_SIZE
								, block_size);
					
							LOG_ERROR("bt_res_query_shub_callback : bcid has change");				   
							/*sd_assert(FALSE);
							ret_val = DT_ERR_INVALID_BCID;
							LOG_DEBUG("DT_ERR_INVALID_BCID 1!");
							goto ErrorHanle;*/
						}
						else
						{
							if(p_file_task->_is_bcid_ok != TRUE)
								p_file_task->_is_bcid_ok = TRUE;
						}
					}
					else
					{
						sd_assert(FALSE);
						ret_val = DT_ERR_INVALID_BCID;
						LOG_DEBUG("DT_ERR_INVALID_BCID 2!");
						goto ErrorHanle;
					}

				}

				p_file_task->_res_query_shub_state = RES_QUERY_REQING;
				p_file_task->_res_query_shub_step =  QUERY_SHUB_SERVER_RES_STEP;
				p_file_task->_query_shub_times_sn = 0;
				p_file_task->_res_query_retry_count = 0;
				res_query_shub_by_resinfo_newcmd(&(p_file_task->_res_query_para), bt_query_only_res_shub_callback,cid,file_size ,TRUE,(const char*)gcid, 	TRUE, MAX_SERVER,DEFAULT_BONUS_RES_NUM, p_file_task->_query_shub_newres_times_sn++,3);
			}
			else
			{
				if(bdm_bcid_is_valid( &(p_bt_task->_data_manager),file_index)==FALSE)
				{
					sd_assert(FALSE);
					ret_val = DT_ERR_INVALID_BCID;
					LOG_DEBUG("DT_ERR_INVALID_BCID 3!");
					goto ErrorHanle;
				}

			}

		}
		bt_query_other_hub(p_bt_task, p_file_task, file_index, cid, file_size);
	}
	else
	{
#ifdef EMBED_REPORT
		p_report_para->_hub_s_fail++;
#endif	
		p_file_task->_res_query_shub_state = RES_QUERY_FAILED;
		LOG_DEBUG("MMMM Query shub failed!");
	}

	/* Start timer */
	if(p_file_task->_res_query_timer_id == 0)
	{
		LOG_DEBUG("bt_res_query_shub_callback:start_timer(bt_handle_query_accelerate_timeout)");
		ret_val =  start_timer(bt_handle_query_accelerate_timeout, NOTICE_INFINITE,REQ_RESOURCE_DEFAULT_TIMEOUT, 0,user_data,&(p_file_task->_res_query_timer_id));
		if(ret_val!=SUCCESS) goto ErrorHanle;
	}

	LOG_DEBUG("bt_res_query_shub_callback:SUCCESS");

	

	return SUCCESS;

ErrorHanle:

	bt_file_task_failure_exit( p_bt_task, file_index,ret_val );
	return SUCCESS;
}


_int32 bt_res_query_phub_callback(void* user_data,_int32 errcode, _u8 result, _u32 retry_interval)
{
	_int32 ret_val = SUCCESS;
	TASK * p_task = (TASK *)((RES_QUERY_PARA *)user_data)->_task_ptr;
	BT_TASK * p_bt_task = (BT_TASK *)p_task;
	BT_FILE_TASK_INFO * p_file_task=NULL;
	_u32 file_index=(_u32)((RES_QUERY_PARA *)user_data)->_file_index;

#ifdef EMBED_REPORT
	_u32 cur_time = 0;
	_u32 phub_query_time = 0;
	RES_QUERY_REPORT_PARA *p_report_para = &p_task->_res_query_report_para;
#endif

	LOG_DEBUG("bt_res_query_phub_callback:user_data=0x%X",user_data);

	sd_assert(user_data!=NULL);
	if(user_data==NULL) return INVALID_ARGUMENT;

	ret_val = map_find_node(&(p_bt_task->_file_task_info_map), (void*)file_index,(void**)&p_file_task);
	CHECK_VALUE(ret_val);
	sd_assert(p_file_task!=NULL);

	LOG_DEBUG( "bt_res_query_phub_callback:_task_id=%u,_file_index=%u,_file_status=%d,res_query_timer_id=%u,last_query_stamp=%u,query_shub_times_sn=%d,res_query_retry_count=%d,error_code=%d,shub_state=%d,phub_state=%d,tracker_state=%d,partner_cdn_state=%u,is_gcid_ok=%d,is_bcid_ok=%d,is_add_rc_ok=%d" ,
		p_bt_task->_task._task_id,file_index,p_file_task->_file_status  ,p_file_task->_res_query_timer_id,p_file_task->_last_query_stamp,p_file_task->_query_shub_times_sn,p_file_task->_res_query_retry_count,p_file_task->_error_code,p_file_task->_res_query_shub_state,p_file_task->_res_query_phub_state,p_file_task->_res_query_tracker_state,p_file_task->_res_query_partner_cdn_state,p_file_task->_is_gcid_ok,p_file_task->_is_bcid_ok,p_file_task->_is_add_rc_ok);


	LOG_DEBUG( "bt_res_query_phub_callback:errcode=%d,result=%d,retry_interval=%u" ,
		errcode,result,retry_interval);

	if(BT_FILE_DOWNLOADING !=  bdm_get_file_status(& (p_bt_task->_data_manager),file_index))
	{
		p_file_task->_res_query_phub_state = RES_QUERY_END;
		return SUCCESS;
	}


#ifdef EMBED_REPORT
	sd_time_ms( &cur_time );
	phub_query_time = TIME_SUBZ( cur_time, p_report_para->_phub_query_time );

	p_report_para->_hub_p_max = MAX(p_report_para->_hub_p_max, phub_query_time);
	if( p_report_para->_hub_p_min == 0 )
	{
		p_report_para->_hub_p_min = phub_query_time;
	}
	p_report_para->_hub_p_min = MIN(p_report_para->_hub_p_min, phub_query_time);
	p_report_para->_hub_p_avg = (p_report_para->_hub_p_avg*(p_report_para->_hub_p_succ+p_report_para->_hub_p_fail)+phub_query_time) / (p_report_para->_hub_p_succ+p_report_para->_hub_p_fail+1);

#endif	

	if((errcode !=SUCCESS)||(result!=SUCCESS))
	{
		LOG_DEBUG("MMMM Query phub failed!");
		p_file_task->_res_query_phub_state = RES_QUERY_FAILED;
#ifdef EMBED_REPORT
		p_report_para->_hub_p_fail++;
#endif			
	}else
	{
		LOG_DEBUG("MMMM Query phub success!");
		p_file_task->_res_query_phub_state = RES_QUERY_SUCCESS;

#ifdef EMBED_REPORT
		p_report_para->_hub_p_succ++;
#endif		
		cm_create_sub_manager_pipes(&(p_bt_task->_task._connect_manager), file_index );
	}

	/* Start timer */
	if(p_file_task->_res_query_timer_id == 0)
	{
		LOG_DEBUG("bt_res_query_phub_callback:start_timer(bt_handle_query_accelerate_timeout)");
		ret_val =  start_timer(bt_handle_query_accelerate_timeout, NOTICE_INFINITE,REQ_RESOURCE_DEFAULT_TIMEOUT, 0,user_data,&(p_file_task->_res_query_timer_id));
		if(ret_val!=SUCCESS) 
		{
			bt_file_task_failure_exit( p_bt_task, file_index,ret_val );
		}
	}
	LOG_DEBUG("bt_res_query_phub_callback:SUCCESS");
	return SUCCESS;

}

_int32 bt_res_query_dphub_callback(void* user_data,_int32 errcode, _u16 result, _u32 retry_interval, BOOL is_continued)
{
	_int32 ret_val = SUCCESS;
	TASK * p_task = (TASK *)((RES_QUERY_PARA *)user_data)->_task_ptr;
	BT_TASK * p_bt_task = (BT_TASK *)p_task;
	BT_FILE_TASK_INFO * p_file_task=NULL;
	_u32 file_index=(_u32)((RES_QUERY_PARA *)user_data)->_file_index;

	LOG_DEBUG("bt_notify_query_dphub_result:user_data=0x%x",user_data);

	sd_assert(user_data!=NULL);
	if(user_data==NULL) return INVALID_ARGUMENT;

	ret_val = map_find_node(&(p_bt_task->_file_task_info_map), (void*)file_index,(void**)&p_file_task);
	CHECK_VALUE(ret_val);
	sd_assert(p_file_task!=NULL);

	LOG_DEBUG( "bt_notify_query_dphub_result:errcode=%d, result=%u, retry_interval=%u" ,
		errcode, (_u32)result, retry_interval);

	if(BT_FILE_DOWNLOADING != bdm_get_file_status(&(p_bt_task->_data_manager),file_index))
	{
		p_file_task->_res_query_dphub_state = RES_QUERY_END;
		return SUCCESS;
	}

	if (errcode == SUCCESS)
	{
		if ((is_continued == TRUE) && (p_bt_task->_task.task_status == TASK_S_RUNNING))
		{
			ret_val = bt_build_res_query_dphub_content(p_bt_task, p_file_task, file_index);
			if (ret_val == ERR_RES_QUERY_NO_NEED_QUERY_DPHUB)
			{
				p_file_task->_res_query_dphub_state = RES_QUERY_END;
				return SUCCESS;
			}
		}
		else
		{
			p_file_task->_res_query_dphub_state = RES_QUERY_SUCCESS;
			cm_create_sub_manager_pipes(&(p_bt_task->_task._connect_manager), file_index);
		}
	}
	else
	{
		p_file_task->_res_query_dphub_state = RES_QUERY_FAILED;
	}

	/* Start timer */
	if(p_file_task->_res_query_timer_id == 0)
	{
		LOG_DEBUG("bt_res_query_dphub_callback:start_timer(bt_handle_query_accelerate_timeout)");
		ret_val =  start_timer(bt_handle_query_accelerate_timeout, 
			NOTICE_INFINITE, REQ_RESOURCE_DEFAULT_TIMEOUT, 0, user_data, &(p_file_task->_res_query_timer_id));
		if(ret_val != SUCCESS)
		{
			bt_file_task_failure_exit(p_bt_task, file_index, ret_val);
		}
	}

	LOG_DEBUG("bt_notify_query_dphub_result, query_dphub_state(%d).", 
		p_file_task->_res_query_dphub_state);

	return SUCCESS;
}

_int32 bt_res_query_tracker_callback(void* user_data,_int32 errcode, _u8 result, _u32 retry_interval,_u32 query_stamp)
{
	_int32 ret_val = SUCCESS;
	TASK * p_task = (TASK *)((RES_QUERY_PARA *)user_data)->_task_ptr;
	BT_TASK * p_bt_task = (BT_TASK *)p_task;
	BT_FILE_TASK_INFO * p_file_task=NULL;
	_u32 file_index=(_u32)((RES_QUERY_PARA *)user_data)->_file_index;
#ifdef EMBED_REPORT
	_u32 cur_time = 0;
	_u32 tracker_query_time = 0;
	RES_QUERY_REPORT_PARA *p_report_para = &p_task->_res_query_report_para;
#endif

	LOG_DEBUG("bt_res_query_tracker_callback:user_data=0x%X",user_data);

	sd_assert(user_data!=NULL);
	if(user_data==NULL) return INVALID_ARGUMENT;

	ret_val = map_find_node(&(p_bt_task->_file_task_info_map), (void*)file_index,(void**)&p_file_task);
	CHECK_VALUE(ret_val);
	sd_assert(p_file_task!=NULL);

	LOG_DEBUG( "bt_res_query_tracker_callback:_task_id=%u,_file_index=%u,_file_status=%d,res_query_timer_id=%u,last_query_stamp=%u,query_shub_times_sn=%d,res_query_retry_count=%d,error_code=%d,shub_state=%d,phub_state=%d,tracker_state=%d,partner_cdn_state=%u,is_gcid_ok=%d,is_bcid_ok=%d,is_add_rc_ok=%d" ,
		p_bt_task->_task._task_id,file_index,p_file_task->_file_status  ,p_file_task->_res_query_timer_id,p_file_task->_last_query_stamp,p_file_task->_query_shub_times_sn,p_file_task->_res_query_retry_count,p_file_task->_error_code,p_file_task->_res_query_shub_state,p_file_task->_res_query_phub_state,p_file_task->_res_query_tracker_state,p_file_task->_res_query_partner_cdn_state,p_file_task->_is_gcid_ok,p_file_task->_is_bcid_ok,p_file_task->_is_add_rc_ok);

	LOG_DEBUG( "bt_res_query_tracker_callback:errcode=%d,result=%d,retry_interval=%u,query_stamp=%u" ,
		errcode,result,retry_interval,query_stamp);

	if(BT_FILE_DOWNLOADING !=  bdm_get_file_status(& (p_bt_task->_data_manager),file_index))
	{
		p_file_task->_res_query_tracker_state = RES_QUERY_END;
		return SUCCESS;
	}

#ifdef EMBED_REPORT
	sd_time_ms( &cur_time );
	tracker_query_time = TIME_SUBZ( cur_time, p_report_para->_tracker_query_time );

	p_report_para->_hub_t_max = MAX(p_report_para->_hub_t_max, tracker_query_time);
	if( p_report_para->_hub_t_min == 0 )
	{
		p_report_para->_hub_t_min = tracker_query_time;
	}
	p_report_para->_hub_t_min = MIN(p_report_para->_hub_t_min, tracker_query_time);
	p_report_para->_hub_t_avg = (p_report_para->_hub_t_avg*(p_report_para->_hub_t_succ+p_report_para->_hub_t_fail)+tracker_query_time) / (p_report_para->_hub_t_succ+p_report_para->_hub_t_fail+1);

#endif	

	if((errcode !=SUCCESS)||(result!=SUCCESS))
	{
		LOG_DEBUG("MMMM Query tracker failed!");
		p_file_task->_res_query_tracker_state = RES_QUERY_FAILED;
#ifdef EMBED_REPORT
		p_report_para->_hub_t_fail++;
#endif			
	}
	else
	{
		LOG_DEBUG("MMMM Query tracker success!");
		p_file_task->_res_query_tracker_state = RES_QUERY_SUCCESS;
		p_file_task->_last_query_stamp = query_stamp;
#ifdef EMBED_REPORT
		p_report_para->_hub_t_succ++;
#endif			
		cm_create_sub_manager_pipes(&(p_bt_task->_task._connect_manager), file_index );
	}

	/* Start timer */
	if(p_file_task->_res_query_timer_id == 0)
	{
		LOG_DEBUG("bt_res_query_tracker_callback:start_timer(bt_handle_query_accelerate_timeout)");
		ret_val =  start_timer(bt_handle_query_accelerate_timeout, NOTICE_INFINITE,REQ_RESOURCE_DEFAULT_TIMEOUT, 0,user_data,&(p_file_task->_res_query_timer_id));
		if(ret_val!=SUCCESS) 
		{
			bt_file_task_failure_exit( p_bt_task, file_index,ret_val );
		}
	}
	LOG_DEBUG("bt_res_query_tracker_callback:SUCCESS");
	return SUCCESS;

}

#ifdef ENABLE_HSC
_int32 bt_res_query_vip_hub_callback(void* user_data,_int32 errcode, _u8 result, _u32 retry_interval)
{
	_int32 ret_val = SUCCESS;
	TASK * p_task = (TASK *)((RES_QUERY_PARA *)user_data)->_task_ptr;
	BT_TASK * p_bt_task = (BT_TASK *)p_task;
	BT_FILE_TASK_INFO * p_file_task=NULL;
	_u32 file_index=(_u32)((RES_QUERY_PARA *)user_data)->_file_index;

	LOG_DEBUG("bt_res_query_vip_hub_callback:user_data=0x%X",user_data);

	sd_assert(user_data!=NULL);
	if(user_data==NULL) return INVALID_ARGUMENT;

	ret_val = map_find_node(&(p_bt_task->_file_task_info_map), (void*)file_index,(void**)&p_file_task);
	CHECK_VALUE(ret_val);
	sd_assert(p_file_task!=NULL);

	LOG_DEBUG( "bt_res_query_vip_hub_callback:_task_id=%u,_file_index=%u,_file_status=%d,res_query_timer_id=%u,last_query_stamp=%u,query_shub_times_sn=%d,res_query_retry_count=%d,error_code=%d,shub_state=%d,phub_state=%d,tracker_state=%d,partner_cdn_state=%u,is_gcid_ok=%d,is_bcid_ok=%d,is_add_rc_ok=%d" ,
		p_bt_task->_task._task_id,file_index,p_file_task->_file_status  ,p_file_task->_res_query_timer_id,p_file_task->_last_query_stamp,p_file_task->_query_shub_times_sn,p_file_task->_res_query_retry_count,p_file_task->_error_code,p_file_task->_res_query_shub_state,p_file_task->_res_query_phub_state,p_file_task->_res_query_tracker_state,p_file_task->_res_query_partner_cdn_state,p_file_task->_is_gcid_ok,p_file_task->_is_bcid_ok,p_file_task->_is_add_rc_ok);


	LOG_DEBUG( "bt_res_query_vip_hub_callback:errcode=%d,result=%d,retry_interval=%u" ,
		errcode,result,retry_interval);

	if(BT_FILE_DOWNLOADING !=  bdm_get_file_status(& (p_bt_task->_data_manager),file_index))
	{
		p_file_task->_res_query_vip_hub_state = RES_QUERY_END;
		return SUCCESS;
	}

	if((errcode !=SUCCESS)||(result!=SUCCESS))
	{
		LOG_DEBUG("MMMM Query vip hub failed!");
		p_file_task->_res_query_vip_hub_state = RES_QUERY_FAILED;

	}
	else
	{
		LOG_DEBUG("MMMM Query vip hub success!");
		p_file_task->_res_query_vip_hub_state = RES_QUERY_SUCCESS;

		cm_create_sub_manager_pipes(&(p_bt_task->_task._connect_manager), file_index );
	}

	if(p_file_task->_res_query_timer_id == 0)
	{
		LOG_DEBUG("bt_res_query_vip_hub_callback:start_timer(bt_handle_query_accelerate_timeout)");
		ret_val =  start_timer(bt_handle_query_accelerate_timeout, NOTICE_INFINITE,REQ_RESOURCE_DEFAULT_TIMEOUT, 0,user_data,&(p_file_task->_res_query_timer_id));
		if(ret_val!=SUCCESS) 
		{
			bt_file_task_failure_exit( p_bt_task, file_index,ret_val );
		}
	}

	LOG_DEBUG("bt_res_query_vip_hub_callback:SUCCESS");
	return SUCCESS;

}
#endif /*ENABLE_HSC*/
#ifdef ENABLE_CDN
_int32 bt_res_query_cdn_manager_callback(_int32 errcode, void* user_data, _u8 result, BOOL is_end, char* host, _u16 port)
{
	_int32 ret_val = SUCCESS;
	_u64 file_size=0;
	_u8 gcid[CID_SIZE];
	_u32 host_ip=0;
	TASK * p_task = (TASK *)((RES_QUERY_PARA *)user_data)->_task_ptr;
	BT_TASK * p_bt_task = (BT_TASK *)p_task;
	BT_FILE_TASK_INFO * p_file_task=NULL;
	_u32 file_index=(_u32)((RES_QUERY_PARA *)user_data)->_file_index;

	LOG_DEBUG("pt_notify_res_query_cdn:user_data=0x%X",user_data);

	sd_assert(user_data!=NULL);
	if(user_data==NULL) return INVALID_ARGUMENT;

	ret_val = map_find_node(&(p_bt_task->_file_task_info_map), (void*)file_index,(void**)&p_file_task);
	CHECK_VALUE(ret_val);
	sd_assert(p_file_task!=NULL);

	LOG_DEBUG( "pt_notify_res_query_cdn:_task_id=%u,_file_index=%u,_file_status=%d,res_query_timer_id=%u,last_query_stamp=%u,query_shub_times_sn=%d,res_query_retry_count=%d,error_code=%d,shub_state=%d,phub_state=%d,tracker_state=%d,partner_cdn_state=%u,is_gcid_ok=%d,is_bcid_ok=%d,is_add_rc_ok=%d" ,
		p_bt_task->_task._task_id,file_index,p_file_task->_file_status  ,p_file_task->_res_query_timer_id,p_file_task->_last_query_stamp,p_file_task->_query_shub_times_sn,p_file_task->_res_query_retry_count,p_file_task->_error_code,p_file_task->_res_query_shub_state,p_file_task->_res_query_phub_state,p_file_task->_res_query_tracker_state,p_file_task->_res_query_partner_cdn_state,p_file_task->_is_gcid_ok,p_file_task->_is_bcid_ok,p_file_task->_is_add_rc_ok);


	LOG_DEBUG( "pt_notify_res_query_cdn:errcode=%d,result=%d" ,
		errcode,result);

	if(BT_FILE_DOWNLOADING !=  bdm_get_file_status(& (p_bt_task->_data_manager),file_index))
	{
		p_file_task->_res_query_partner_cdn_state= RES_QUERY_END;
		return SUCCESS;
	}


#ifdef _DEBUG
	LOG_ERROR("bt_res_query_cdn_manager_callback: errcode=%d,result=%d,is_end=%d,host=%s,port=%u",errcode,result,is_end,host,port);
	printf("\n bt_res_query_cdn_manager_callback: errcode=%d,result=%d,is_end=%d,host=%s,port=%u\n",errcode,result,is_end,host,port);
#endif

	if((errcode !=SUCCESS)||(result!=SUCCESS))
	{
		LOG_ERROR("MMMM Query cdn failed!");
		p_file_task->_res_query_partner_cdn_state = RES_QUERY_FAILED;
	}
	else
	{
		if(is_end==TRUE)
		{
			LOG_ERROR("MMMM Query cdn success!");
			p_file_task->_res_query_partner_cdn_state = RES_QUERY_SUCCESS;

			cm_create_sub_manager_pipes(&(p_bt_task->_task._connect_manager), file_index );
		}
		else
		{
			/* Add the resource to connect_manager */
			ret_val = sd_inet_aton(host, &host_ip);
			CHECK_VALUE(ret_val);

			file_size = bdm_get_sub_file_size(&(p_bt_task->_data_manager),file_index);
			if(bdm_get_shub_gcid( &(p_bt_task->_data_manager),file_index, gcid)==TRUE)
			{
				cm_add_cdn_peer_resource(&(p_task->_connect_manager),((RES_QUERY_PARA *)user_data)->_file_index, NULL,   gcid,file_size, 0, host_ip,port,0,0,P2P_FROM_CDN);
			}
			return SUCCESS;
		}
	}

	/* Start timer */
	if(p_file_task->_res_query_timer_id == 0)
	{
		LOG_DEBUG("_res_query_partner_cdn_state:start_timer(bt_handle_query_accelerate_timeout)");
		ret_val =  start_timer(bt_handle_query_accelerate_timeout, NOTICE_INFINITE,REQ_RESOURCE_DEFAULT_TIMEOUT, 0,user_data,&(p_file_task->_res_query_timer_id));
		if(ret_val!=SUCCESS) 
		{
			bt_file_task_failure_exit( p_bt_task, file_index,ret_val );
		}
	}
	LOG_DEBUG("_res_query_partner_cdn_state:SUCCESS");
	return SUCCESS;

}
#endif /* ENABLE_CDN */

_int32 bt_res_query_partner_cdn_callback(void* user_data,_int32 errcode, _u8 result, _u32 retry_interval)
{
	_int32 ret_val = SUCCESS;
	TASK * p_task = (TASK *)((RES_QUERY_PARA *)user_data)->_task_ptr;
	BT_TASK * p_bt_task = (BT_TASK *)p_task;
	BT_FILE_TASK_INFO * p_file_task=NULL;
	_u32 file_index=(_u32)((RES_QUERY_PARA *)user_data)->_file_index;

	LOG_DEBUG("bt_res_query_partner_cdn_callback:user_data=0x%X",user_data);

	sd_assert(user_data!=NULL);
	if(user_data==NULL) return INVALID_ARGUMENT;

	ret_val = map_find_node(&(p_bt_task->_file_task_info_map), (void*)file_index,(void**)&p_file_task);
	CHECK_VALUE(ret_val);
	sd_assert(p_file_task!=NULL);

	LOG_DEBUG( "bt_res_query_partner_cdn_callback:_task_id=%u,_file_index=%u,_file_status=%d,res_query_timer_id=%u,last_query_stamp=%u,query_shub_times_sn=%d,res_query_retry_count=%d,error_code=%d,shub_state=%d,phub_state=%d,tracker_state=%d,partner_cdn_state=%u,is_gcid_ok=%d,is_bcid_ok=%d,is_add_rc_ok=%d" ,
		p_bt_task->_task._task_id,file_index,p_file_task->_file_status  ,p_file_task->_res_query_timer_id,p_file_task->_last_query_stamp,p_file_task->_query_shub_times_sn,p_file_task->_res_query_retry_count,p_file_task->_error_code,p_file_task->_res_query_shub_state,p_file_task->_res_query_phub_state,p_file_task->_res_query_tracker_state,p_file_task->_res_query_partner_cdn_state,p_file_task->_is_gcid_ok,p_file_task->_is_bcid_ok,p_file_task->_is_add_rc_ok);


	LOG_DEBUG( "bt_res_query_partner_cdn_callback:errcode=%d,result=%d,retry_interval=%u" ,
		errcode,result,retry_interval);

	if(BT_FILE_DOWNLOADING !=  bdm_get_file_status(& (p_bt_task->_data_manager),file_index))
	{
		p_file_task->_res_query_partner_cdn_state= RES_QUERY_END;
		return SUCCESS;
	}

	if((errcode !=SUCCESS)||(result!=SUCCESS))
	{
		LOG_DEBUG("MMMM Query partner cdn failed!");
		p_file_task->_res_query_partner_cdn_state = RES_QUERY_FAILED;
	}else
	{
		LOG_DEBUG("MMMM Query partner cdn success!");
		p_file_task->_res_query_partner_cdn_state = RES_QUERY_SUCCESS;

		cm_create_sub_manager_pipes(&(p_bt_task->_task._connect_manager), file_index );
	}

	/* Start timer */
	if(p_file_task->_res_query_timer_id == 0)
	{
		LOG_DEBUG("_res_query_partner_cdn_state:start_timer(bt_handle_query_accelerate_timeout)");
		ret_val =  start_timer(bt_handle_query_accelerate_timeout, NOTICE_INFINITE,REQ_RESOURCE_DEFAULT_TIMEOUT, 0,user_data,&(p_file_task->_res_query_timer_id));
		if(ret_val!=SUCCESS) 
		{
			bt_file_task_failure_exit( p_bt_task, file_index,ret_val );
		}
	}
	LOG_DEBUG("_res_query_partner_cdn_state:SUCCESS");
	return SUCCESS;

}


/*--------------------------------------------------------------------------*/
/*                Callback function for timer				        */
/*--------------------------------------------------------------------------*/

_int32 bt_handle_query_accelerate_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid)
{
	_int32 ret_val = SUCCESS;
	TASK * p_task = NULL;
	BT_TASK * p_bt_task = NULL;
	BT_FILE_TASK_INFO * p_file_task=NULL;
	_u32 file_index=0;

	LOG_DEBUG("bt_handle_query_accelerate_timeout:errcode=%d,notice_count_left=%u,expired=%u,msgid=%u",errcode,notice_count_left,expired,msgid);

	if(errcode == MSG_TIMEOUT)
	{
		p_task = (TASK *)((RES_QUERY_PARA *)(msg_info->_user_data))->_task_ptr;
		p_bt_task = (BT_TASK *)p_task;
		file_index=(_u32)((RES_QUERY_PARA *)(msg_info->_user_data))->_file_index;

		sd_assert(p_bt_task!=NULL);
		if(p_bt_task==NULL) return INVALID_ARGUMENT;	

		ret_val = map_find_node(&(p_bt_task->_file_task_info_map),(void*) file_index,(void**)&p_file_task);
		if(ret_val!=SUCCESS) goto ErrorHanle;
		sd_assert(p_file_task!=NULL);

		LOG_DEBUG( "bt_handle_query_accelerate_timeout:_task_id=%u,_file_index=%u,_file_status=%d,res_query_timer_id=%u,last_query_stamp=%u,query_shub_times_sn=%d,res_query_retry_count=%d,error_code=%d,shub_state=%d,phub_state=%d,tracker_state=%d,partner_cdn_state=%u,is_gcid_ok=%d,is_bcid_ok=%d,is_add_rc_ok=%d" ,
			p_bt_task->_task._task_id,file_index,p_file_task->_file_status  ,p_file_task->_res_query_timer_id,p_file_task->_last_query_stamp,p_file_task->_query_shub_times_sn,p_file_task->_res_query_retry_count,p_file_task->_error_code,p_file_task->_res_query_shub_state,p_file_task->_res_query_phub_state,p_file_task->_res_query_tracker_state,p_file_task->_res_query_partner_cdn_state,p_file_task->_is_gcid_ok,p_file_task->_is_bcid_ok,p_file_task->_is_add_rc_ok);

		if(msgid == p_file_task->_res_query_timer_id)
		{

			if((bdm_get_file_status(& (p_bt_task->_data_manager),file_index)==BT_FILE_DOWNLOADING)&& cm_is_global_need_more_res()&&
				((cm_is_need_more_server_res( &(p_task->_connect_manager),file_index ))||(cm_is_need_more_peer_res( &(p_task->_connect_manager),file_index  ))))
			{
				ret_val= bt_start_res_query_accelerate(p_bt_task , file_index,p_file_task);
				if(ret_val!=SUCCESS) goto ErrorHanle;
			}
		}
	}

	LOG_DEBUG("bt_handle_query_accelerate_timeout:SUCCESS");
	return SUCCESS;

ErrorHanle:

	bt_file_task_failure_exit( p_bt_task, file_index,ret_val );
	return SUCCESS;


}

_int32 bt_handle_wait_dphub_root_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid)
{
	_int32 ret_val = SUCCESS;
	TASK * p_task = NULL;
	BT_TASK * p_bt_task = NULL;
	BT_FILE_TASK_INFO * p_file_task=NULL;
	_u32 file_index=0;

	LOG_DEBUG("bt_handle_wait_dphub_root_timeout:errcode=%d,notice_count_left=%u,expired=%u,msgid=%u",
		errcode,notice_count_left,expired,msgid);

	if(errcode == MSG_TIMEOUT)
	{
		p_task = (TASK *)((RES_QUERY_PARA *)(msg_info->_user_data))->_task_ptr;
		p_bt_task = (BT_TASK *)p_task;
		file_index=(_u32)((RES_QUERY_PARA *)(msg_info->_user_data))->_file_index;

		sd_assert(p_bt_task!=NULL);
		if(p_bt_task==NULL) return INVALID_ARGUMENT;	

		ret_val = map_find_node(&(p_bt_task->_file_task_info_map),(void*) file_index,(void**)&p_file_task);
		if(ret_val!=SUCCESS) goto ErrorHanle;
		sd_assert(p_file_task!=NULL);

		LOG_DEBUG( "bt_handle_wait_dphub_root_timeout:_task_id=%u,_file_index=%u,_file_status=%d,_wait_dphub_root_timer_id=%u,last_query_stamp=%u,query_shub_times_sn=%d,res_query_retry_count=%d,error_code=%d,shub_state=%d,phub_state=%d,tracker_state=%d,partner_cdn_state=%u,is_gcid_ok=%d,is_bcid_ok=%d,is_add_rc_ok=%d" ,
			p_bt_task->_task._task_id,file_index,p_file_task->_file_status, 
			p_file_task->_dpub_query_context._wait_dphub_root_timer_id, p_file_task->_last_query_stamp,
			p_file_task->_query_shub_times_sn,p_file_task->_res_query_retry_count,
			p_file_task->_error_code,p_file_task->_res_query_shub_state,p_file_task->_res_query_phub_state,
			p_file_task->_res_query_tracker_state,p_file_task->_res_query_partner_cdn_state,
			p_file_task->_is_gcid_ok,p_file_task->_is_bcid_ok,p_file_task->_is_add_rc_ok);

		if(msgid == p_file_task->_dpub_query_context._wait_dphub_root_timer_id)
		{
			if((bdm_get_file_status(& (p_bt_task->_data_manager),file_index)==BT_FILE_DOWNLOADING)&& cm_is_global_need_more_res()&&
				((cm_is_need_more_server_res( &(p_task->_connect_manager),file_index ))||(cm_is_need_more_peer_res( &(p_task->_connect_manager),file_index  ))))
			{
				ret_val= bt_build_res_query_dphub_content(p_bt_task, p_file_task, file_index);
			}
		}
	}

	LOG_DEBUG("bt_handle_wait_dphub_root_timeout:SUCCESS");
	return SUCCESS;

ErrorHanle:

	bt_file_task_failure_exit(p_bt_task, file_index,ret_val);
	return SUCCESS;
}

_int32 bt_file_task_failure_exit( BT_TASK *p_bt_task,_u32 file_index,_int32 err_code  )
{

	LOG_DEBUG( " enter bt_file_task_failure_exit(file_index = %d,_error_code=%d )...",file_index,err_code);

	bt_stop_accelerate( p_bt_task,  file_index );


	return SUCCESS;

} /* End of dt_failure_exit */


_int32 bt_ajust_accelerate_list( BT_TASK *p_bt_task)
{
	_int32 ret_val = SUCCESS;
	MAP_ITERATOR cur_node ;

	//settings_get_int_item( "bt.max_bt_accelerate_num",(_int32*)&max_bt_accelerate_num);

	LOG_DEBUG( "bt_ajust_accelerate_list");
	BT_FILE_INFO* p_idle_fileinfo = NULL;

	_u32 need_stop_file_index = 0xFFFFFFFF;
	_u32 file_index_idle = 0xFFFFFFFF;
	for(cur_node = MAP_BEGIN( p_bt_task->_file_info_map ); 
		cur_node != MAP_END(  p_bt_task->_file_info_map ); 
		cur_node = MAP_NEXT(  p_bt_task->_file_info_map, cur_node ))
	{
		BT_FILE_INFO *p_file_info = (BT_FILE_INFO *)MAP_VALUE(cur_node);
		if( p_file_info->_file_status == BT_FILE_DOWNLOADING
			&& p_file_info->_accelerate_state != BT_ACCELERATING
			&& p_file_info->_file_index < file_index_idle
			&& p_file_info->_query_result==RES_QUERY_SUCCESS
			&& p_file_info->_has_record
			&& p_file_info->_sub_task_err_code !=  BT_FILE_DOWNLOAD_SUCCESS_WAIT_CHECK
			)
		{
			file_index_idle = p_file_info->_file_index;
			p_idle_fileinfo = p_file_info;
		}
		else if(p_file_info->_accelerate_state  == BT_ACCELERATING
			&& p_file_info->_file_status == BT_FILE_DOWNLOADING
			&& p_file_info->_sub_task_err_code ==  BT_FILE_DOWNLOAD_SUCCESS_WAIT_CHECK)
		{
			need_stop_file_index =  p_file_info->_file_index;
			LOG_DEBUG( "bt_ajust_accelerate_list: fileidx = %d, need stop accerate for status is waiting check", need_stop_file_index);


		}
	}


	if(need_stop_file_index < 0xFFFFFFFF)
	{
		LOG_DEBUG( "bt_ajust_accelerate_list: need_stop_file_index < MAX_FILE_INDEX");
		bt_stop_accelerate(p_bt_task, need_stop_file_index);
		return 0;
	}

	LOG_DEBUG( "bt_ajust_accelerate_list: file_index_idle = %d", file_index_idle);

	_u32 file_index_ajust = 0;
	for(cur_node = MAP_BEGIN( p_bt_task->_file_task_info_map ); 
		cur_node != MAP_END(  p_bt_task->_file_task_info_map ); 
		cur_node = MAP_NEXT(  p_bt_task->_file_task_info_map, cur_node ))
	{
		_u32 file_index_acc = (_u32)MAP_KEY(cur_node);

		if(file_index_acc > file_index_ajust)
		{
			file_index_ajust = file_index_acc;
		}
	}

	if(file_index_ajust > file_index_idle)
	{
		sd_assert( NULL != p_idle_fileinfo);

		BOOL is_start_succ = FALSE;
		bt_start_accelerate(p_bt_task, p_idle_fileinfo, file_index_idle, &is_start_succ);
		LOG_DEBUG( "bt_ajust_accelerate_list: stop accelerate, file_index = %d, file_index_idle=%d, is_start_succ=%d", file_index_ajust
			, file_index_idle, is_start_succ);

		if(is_start_succ)
		{
			bt_stop_accelerate(p_bt_task, file_index_ajust);
		}
	}

	LOG_DEBUG( "bt_ajust_accelerate_list:SUCCESS" );
	return SUCCESS;
}


#endif /* ENABLE_BT */
