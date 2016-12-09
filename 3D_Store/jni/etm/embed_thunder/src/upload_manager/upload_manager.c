#if !defined(__UPLOAD_MANAGER_C_20081111)
#define __UPLOAD_MANAGER_C_20081111

#include "upload_manager.h"
#include "rclist_manager.h"
#include "upload_control.h"
#ifdef UPLOAD_ENABLE


#include "utility/settings.h"
#include "utility/mempool.h"
#include "utility/string.h"
#include "utility/utility.h"
#include "platform/sd_fs.h"
#include "reporter/reporter_interface.h"
#include "utility/peer_capability.h"
#include "task_manager/task_manager.h"
#ifdef _CONNECT_DETAIL
#include "utility/rw_sharebrd.h"
#endif

#include "utility/logid.h"
#ifdef LOGID
#undef LOGID
#endif

#define LOGID LOGID_UPLOAD_MANAGER

#include "utility/logger.h"

typedef struct
{
	_u32	_begin_time;/*统计开始时间*/
	_u32    _end_time;/*统计结束时间*/
	_u32    _up_duration;/*设备运行时长*/

	_u32	_up_use_duration;/*上传用时*/
	_u64    _up_data_bytes;/*上传数据总字节数*/
	_u32    _up_pipe_num;  /*新建的连接总数*/
	_u32    _up_passive_pipe_num;/*新建的被动连接总数*/
}UPLOAD_MANAGER_STAT;

typedef struct 
{
	_u32 	scheduler_timer_id;
	_u32 	scheduler_time;
	_int32     	error_code;
	BOOL      is_rclist_reported;
	_u32 	isrc_online_timer_id;
	BOOL      is_rc_online;

	_u32 	slow_timer_id;
	_u32    slow_timer_interval;
	int     last_stat_state;//0-ok,1- reporting
	_u32    rep_seqno;	// 统计上报的seqno
	UPLOAD_MANAGER_STAT last_stat;
	UPLOAD_MANAGER_STAT cur_stat;
	int     pipe_num_using;	/*当前在使用连接数*/
	_u32    up_using_begin_time;/*尚未统计的上传时长的开始时间*/
}UPLOAD_MANAGER;
    
/*--------------------------------------------------------------------------*/
/*                Global declarations				        */
/*--------------------------------------------------------------------------*/
/* Pointer of the upload_manager,only one upload_manager when program is running */
static UPLOAD_MANAGER * gp_upload_manager;
#ifdef _CONNECT_DETAIL
static PEER_PIPE_INFO_ARRAY  g_pipe_info_array;
static RW_SHAREBRD *g_pipe_info_array_brd_ptr=NULL;
#endif  /* _CONNECT_DETAIL */

static void _ulm_load_stat();
static _int32 _ulm_handle_upload_manager_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid);


/*--------------------------------------------------------------------------*/
/*                Interfaces provid for task_manager				        */
/*--------------------------------------------------------------------------*/
_int32 init_upload_manager(void)
{
	_int32 ret_val = SUCCESS, load_res = 0;
	UPLOAD_MANAGER_STAT * p_stat;
	_u32 now;
	
	LOG_DEBUG("init_upload_manager");
	
	if(gp_upload_manager != NULL)
		return ULM_ERR_ULM_MGR_EXIST;

	ret_val = sd_malloc(sizeof(UPLOAD_MANAGER) ,(void**)&gp_upload_manager);
	if(ret_val!=SUCCESS) return ret_val;

	sd_memset(gp_upload_manager,0,sizeof(UPLOAD_MANAGER));

	gp_upload_manager->scheduler_time = ULM_SCHEDULER_TIME_OUT;
	settings_get_int_item( "upload_manager.scheduler_time",(_int32*)&(gp_upload_manager->scheduler_time));
	
	ret_val = init_rclist_manager();
	if(ret_val!=SUCCESS) goto ErrorHanle;

	ret_val = init_upload_file_manager();
	if(ret_val!=SUCCESS) goto ErrorHanle;

	ret_val = init_upload_pipe_manager();
	if(ret_val!=SUCCESS) goto ErrorHanle;

#ifdef _CONNECT_DETAIL
	sd_memset(&g_pipe_info_array, 0, sizeof(PEER_PIPE_INFO_ARRAY));
	ret_val = init_customed_rw_sharebrd(&g_pipe_info_array, &g_pipe_info_array_brd_ptr);
	if( ret_val!=SUCCESS ) goto ErrorHanle;
#endif

	/* Start the isrc-onlice_report_timer */
	ret_val =   start_timer(ulm_handle_isrc_online_timeout, NOTICE_ONCE, 1000, 0,(void*)gp_upload_manager,&(gp_upload_manager->isrc_online_timer_id));  
	if(ret_val!=SUCCESS) goto ErrorHanle;

	//启动定时器
	gp_upload_manager->slow_timer_interval=300;
	ret_val =    start_timer(_ulm_handle_upload_manager_timeout, NOTICE_INFINITE, gp_upload_manager->slow_timer_interval*1000, 0,(void*)gp_upload_manager,&(gp_upload_manager->slow_timer_id));  
	if(ret_val!=SUCCESS) goto ErrorHanle;

	/*加载统计信息到last_stat*/
	_ulm_load_stat();
	p_stat=&gp_upload_manager->last_stat;
	if( (p_stat->_end_time < p_stat->_begin_time)||
		(p_stat->_up_duration > p_stat->_end_time- p_stat->_begin_time)||
		(p_stat->_up_use_duration > p_stat->_up_duration) ||
		(p_stat->_up_passive_pipe_num > p_stat->_up_pipe_num) )
	{	//统计信息有误
		load_res = -1;
	}
	else
	{
		sd_time(&now);	//时间回拨
		if(p_stat->_begin_time > now)
		{	//时间回拨太大
			load_res=-2;
		}
		else
		{
			if(p_stat->_end_time > now)
			{
				p_stat->_end_time = now;
				load_res =1;
			}
		}
	}
	
	if(load_res < 0)
	{
		LOG_DEBUG("init_upload_manager: load stat fail , load_res=%d ,reset stat ",load_res);
		memset(p_stat,0,sizeof(*p_stat));
	}
	else
	{
		LOG_DEBUG("init_upload_manager: load stat ok , load_res=%d ",load_res);
	}
	//设置当前统计值
	gp_upload_manager->cur_stat._begin_time = now;
		
    ret_val = init_ulc();

	LOG_DEBUG("init_upload_manager:SUCCESS");
	return SUCCESS;
	
ErrorHanle:

    uninit_ulc();
	
#ifdef _CONNECT_DETAIL
	if(g_pipe_info_array_brd_ptr!=NULL)
	{
		uninit_customed_rw_sharebrd(g_pipe_info_array_brd_ptr);
		g_pipe_info_array_brd_ptr=NULL;
	}
#endif

	uninit_upload_pipe_manager();
	
	uninit_upload_file_manager();
	
	uninit_rclist_manager();
	
	SAFE_DELETE(gp_upload_manager);
	
	LOG_DEBUG("init_upload_manager:FAILED! ret_val=%d",ret_val);
	return ret_val;
}

_int32 uninit_upload_manager(void)
{

	LOG_DEBUG("uninit_upload_manager");
	
	if(gp_upload_manager == NULL)
		return ULM_ERR_ULM_MGR_NOT_EXIST;

	if(gp_upload_manager->slow_timer_id!=0)
	{
		_ulm_handle_upload_manager_timeout(NULL,MSG_CANCELLED,0,0,0);
		cancel_timer(gp_upload_manager->slow_timer_id);
		gp_upload_manager->slow_timer_id=0;
	}

	if(gp_upload_manager->isrc_online_timer_id!=0)
	{
		cancel_timer(gp_upload_manager->isrc_online_timer_id);
		gp_upload_manager->isrc_online_timer_id=0;
	}
	
	if(gp_upload_manager->scheduler_timer_id != 0)
	{
		cancel_timer(gp_upload_manager->scheduler_timer_id);
		gp_upload_manager->scheduler_timer_id = 0;
	}
	
#ifdef _CONNECT_DETAIL
	if(g_pipe_info_array_brd_ptr!=NULL)
	{
		uninit_customed_rw_sharebrd(g_pipe_info_array_brd_ptr);
		g_pipe_info_array_brd_ptr=NULL;
	}
#endif
    

	uninit_upload_pipe_manager();
	
	uninit_upload_file_manager();
	
	 uninit_rclist_manager();

	SAFE_DELETE(gp_upload_manager);

    uninit_ulc();
    
	LOG_DEBUG("uninit_upload_manager:SUCCESS");
	return SUCCESS;
}

_int32 ulm_close_upload_pipes(void *event_handle)
{
	upm_close_upload_pipes();
	return  ufm_clear(event_handle);
}

_int32 ulm_close_upload_pipes_notify(void *event_handle )
{
	return  tm_close_upload_pipes_notify(event_handle);
}

_int32 ulm_set_record_list_file_path( const char *path,_u32  path_len)
{
	LOG_DEBUG("ulm_set_record_list_file_path:path=%s",path);
	if(gp_upload_manager==NULL)	return ULM_ERR_ULM_MGR_NOT_EXIST;
	
	return rclist_set_rc_list_path(  path,path_len);
}

_u32 ulm_get_cur_upload_speed( void)
{
	return upm_get_cur_upload_speed( );
}

_u32 ulm_get_max_upload_speed( void)
{
	return upm_get_max_upload_speed( );
}


/*--------------------------------------------------------------------------*/
/*                Interfaces provid for download_task				        */
/*--------------------------------------------------------------------------*/
// 添加资源
_int32 ulm_add_record(_u64 size, const _u8 *tcid, const _u8 *gcid, _u8 chub, const char *path)
{
	_int32 ret_val = SUCCESS;
	
	LOG_DEBUG("ulm_add_record:path=%s",path);
	
	if(gp_upload_manager==NULL)	return ULM_ERR_ULM_MGR_NOT_EXIST;
	//if(gp_upload_manager->error_code!=SUCCESS) return gp_upload_manager->error_code;
	
	ret_val = rclist_add_rc( size, tcid, gcid,  chub, path);
	if((ret_val == SUCCESS)
        && (gp_upload_manager->error_code==SUCCESS)
        && (gp_upload_manager->is_rc_online))
	{
			#ifdef EMBED_REPORT
			/* Report  to phub */
			if(gp_upload_manager->is_rclist_reported == FALSE)
			{
				ret_val = reporter_rc_list(rclist_get_rc_list(),(_u32)get_peer_capability());
				if(ret_val== SUCCESS)
				{
					gp_upload_manager->is_rclist_reported = TRUE;
				}
			}
			else
			{
				return reporter_insert_rc(size, tcid,gcid,(_u32)get_peer_capability());
			}
			#endif
	}

	return ret_val;
}
// 删除资源
_int32 ulm_del_record(_u64 size, const _u8 *tcid,const _u8 *gcid)
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG("ulm_del_record");
	if(gp_upload_manager==NULL)	return ULM_ERR_ULM_MGR_NOT_EXIST;
	//if(gp_upload_manager->error_code!=SUCCESS) return gp_upload_manager->error_code;
	
	ret_val = rclist_del_rc( gcid);
	if((ret_val == SUCCESS)&&(gp_upload_manager->error_code==SUCCESS)&&(gp_upload_manager->is_rc_online))
	{
		#ifdef EMBED_REPORT
		/* report to the phub */
		if(gp_upload_manager->is_rclist_reported != FALSE)
			return reporter_delete_rc(size,tcid, gcid);
		#endif
	}

	return ret_val;
}

_int32 ulm_change_file_path(_u64 size, const char *old_path, const char *new_path)
{
	if(gp_upload_manager==NULL)	
		return ULM_ERR_ULM_MGR_NOT_EXIST;
	return rclist_change_file_path(size,old_path,new_path);
}

//比较文件大小和文件路径名，如果相同就可以删除
_int32 ulm_del_record_by_full_file_path(_u64 size,  const char *path)
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG("ulm_del_record_by_full_file_path");
	if(gp_upload_manager==NULL)	return ULM_ERR_ULM_MGR_NOT_EXIST;
	//if(gp_upload_manager->error_code!=SUCCESS) return gp_upload_manager->error_code;

	ID_RC_INFO *p_rc_info = rclist_query_by_size_and_path(size, path);
	if (p_rc_info)
	{
		ret_val = rclist_del_rc( p_rc_info->_gcid );
		if((ret_val == SUCCESS)&&(gp_upload_manager->error_code==SUCCESS)&&(gp_upload_manager->is_rc_online))
		{
			#ifdef EMBED_REPORT
			/* report to the phub */
			if(gp_upload_manager->is_rclist_reported != FALSE)
				return reporter_delete_rc(size, p_rc_info->_tcid, p_rc_info->_gcid);
			#endif
		}
	}

	return ret_val;
}

/*--------------------------------------------------------------------------*/
/*                Interfaces provid for data_manager				        */
/*--------------------------------------------------------------------------*/
//
_int32 ulm_notify_have_block( const _u8 *gcid)
{
	LOG_DEBUG("ulm_notify_have_block");
	if(gp_upload_manager==NULL)	return ULM_ERR_ULM_MGR_NOT_EXIST;

	return upm_notify_have_block( gcid);
}

/*--------------------------------------------------------------------------*/
/*                Interfaces provid for p2p				        */
/*--------------------------------------------------------------------------*/
BOOL ulm_is_td_file( char * file_path)
{
	char * point_pos=NULL;
	
	point_pos=sd_strrchr(file_path, '.');
	if((point_pos!=NULL)&&(sd_strlen(point_pos)==3))
	{
		if(0==sd_strncmp(point_pos, ".rf",3))
			return TRUE;
	}
	return FALSE;
}

// 查询资源
BOOL  ulm_is_record_exist_by_gcid( const _u8 *gcid )
{
	ID_RC_INFO info,*p_rc_info = NULL;
	_int32 ret_val=SUCCESS;
	char new_path[MAX_FULL_PATH_BUFFER_LEN];

	LOG_DEBUG("ulm_is_record_exist_by_gcid");
	
	if(gcid==NULL) return INVALID_ARGUMENT;
	if(gp_upload_manager==NULL)	return ULM_ERR_ULM_MGR_NOT_EXIST;
	if(gp_upload_manager->error_code!=SUCCESS) return gp_upload_manager->error_code;
	
	p_rc_info = rclist_query( gcid );


	if ( p_rc_info!=NULL )
	{
		LOG_DEBUG( "Record found from the list:path= %s" ,p_rc_info->_path);
		/* Check if the file is not modified */
		if(ulm_check_is_file_modified(p_rc_info->_size,p_rc_info->_path)==TRUE)
		{
			/* Check if the file end with ".rf" */
			if(ulm_is_td_file(p_rc_info->_path)==TRUE)
			{
				sd_memset(new_path,0,MAX_FULL_PATH_BUFFER_LEN);
				sd_strncpy(new_path, p_rc_info->_path,MAX_FULL_PATH_BUFFER_LEN);
				new_path[sd_strlen(p_rc_info->_path)-3]='\0';
				if(ulm_check_is_file_modified(p_rc_info->_size,new_path)==FALSE)
				{
					ret_val=rclist_change_rc_path( gcid,new_path,sd_strlen(new_path));
					if(ret_val==SUCCESS)
					{
						/*文件名被修改了，但是内容没有变动过,不需要删除*/
						return TRUE;
					}
				}
			}
			
			/*列表文件内容有变动过,不可用*/
			sd_memset(&info,0,sizeof(ID_RC_INFO));
			sd_memcpy(&info, p_rc_info, sizeof(ID_RC_INFO));
			ulm_del_record(info._size, info._tcid,info._gcid);
			
			LOG_DEBUG( "Record found but file is not usable!" );
			
			return FALSE;
		}
		return TRUE; 
	}
	else
	{
		return FALSE;
	}
}

BOOL ulm_is_file_exist(const _u8* gcid, _u64 file_size)
{
	return ulm_is_record_exist_by_gcid( gcid );
}

BOOL ulm_is_pipe_full(void)
{
	return upm_is_pipe_full();
}


_int32 ulm_add_pipe_by_gcid( const _u8 *gcid  ,DATA_PIPE * p_pipe)
{
	_int32 ret_val = SUCCESS;
	ID_RC_INFO *p_rc_info = NULL;
	ULM_PIPE_TYPE type;

	LOG_DEBUG("ulm_add_pipe_by_gcid");

    if( 0 == ulc_enable_upload())
    {
        LOG_DEBUG("ulm_add_pipe_by_gcid, ulc_enable_upload == 0");
        return ULM_ERR_NOT_ALLOW_UPLOAD;
    }    

	if((gcid==NULL)||(p_pipe==NULL)) return INVALID_ARGUMENT;
		
	if(gp_upload_manager==NULL)	return ULM_ERR_ULM_MGR_NOT_EXIST;
	if(gp_upload_manager->error_code!=SUCCESS) return gp_upload_manager->error_code;
	
	if(p_pipe->_data_pipe_type!=P2P_PIPE) return ULM_ERR_INVALID_PIPE_TYPE;
	
	//if(TRUE== upm_is_pipe_full())
	//	return ULM_ERR_TOO_MANY_PIPES;

	type = upm_get_pipe_type(p_pipe);
	
	if(type==ULM_PU_P2P_PIPE)
	{

		p_rc_info = rclist_query( gcid );

		if ( p_rc_info==NULL )
		{
			return ULM_ERR_RECORD_NOT_FOUND;
		}

		LOG_DEBUG( "Record found from the list:path= %s" ,p_rc_info->_path);
	}
		
	ret_val=upm_add_pipe(gcid  ,p_pipe,type);
	if(ret_val!=SUCCESS) return ret_val;

	if(gp_upload_manager->scheduler_timer_id == 0)
	{
		ret_val = start_timer(ulm_handle_scheduler_timeout,NOTICE_INFINITE,gp_upload_manager->scheduler_time,0,NULL,&(gp_upload_manager->scheduler_timer_id));
		if(ret_val!=SUCCESS) 
		{
			ulm_failure_exit(ret_val);
		}
	}

	if( 0== gp_upload_manager->pipe_num_using)
	{
		sd_time(&gp_upload_manager->up_using_begin_time);
	}
	gp_upload_manager->pipe_num_using++;
	
	gp_upload_manager->cur_stat._up_pipe_num++;
	if(type==ULM_PU_P2P_PIPE)
	{
		gp_upload_manager->cur_stat._up_passive_pipe_num++;		
	}
	return SUCCESS;
}



_int32 ulm_remove_pipe( DATA_PIPE * p_pipe)
{
	_u32 now;
	gp_upload_manager->pipe_num_using--;
	if(0==gp_upload_manager->pipe_num_using)
	{
		sd_time(&now);
		if( now > gp_upload_manager->cur_stat._up_use_duration) 
		{
			gp_upload_manager->cur_stat._up_use_duration+=now-gp_upload_manager->up_using_begin_time;
		}
		gp_upload_manager->up_using_begin_time = 0;
	}

	return upm_remove_pipe( p_pipe);
}




_int32 ulm_read_data(void * data_manager
    , RANGE_DATA_BUFFER * p_data_buffer
    , read_data_callback_func callback
    , DATA_PIPE * p_pipe)
{
	_int32 ret_val = SUCCESS;
	ID_RC_INFO  *p_rc_info=NULL;
	_u8 * gcid=NULL;
	ULM_PIPE_TYPE pipe_type;

	LOG_DEBUG( "ulm_read_data:p_pipe= 0x%X,_range[%u,%u],_data_length=%u,_buffer_length=%u" ,p_pipe,p_data_buffer->_range._index,p_data_buffer->_range._num,p_data_buffer->_data_length,p_data_buffer->_buffer_length);

	sd_assert(p_data_buffer->_data_length!=0);
	sd_assert(p_data_buffer->_data_length<=p_data_buffer->_buffer_length);
	if(TRUE!=upm_is_pipe_unchoked(p_pipe))
	{
		LOG_DEBUG("ulm_read_data:p_pipe= 0x%X is not a unchoked pipe,error!",p_pipe);
		return -1;
	}

	pipe_type=upm_get_pipe_type(p_pipe);

	if(pipe_type==ULM_PU_P2P_PIPE)
	{
		// 普通p2p 纯上传pipe
		gcid = upm_get_gcid(p_pipe);
		if ( gcid==NULL )
		{
			return -1;
		}
		
		p_rc_info = rclist_query( gcid );

		if ( p_rc_info==NULL )
		{
			return ULM_ERR_RECORD_NOT_FOUND;
		}
		
		ret_val = ufm_read_file(p_rc_info,p_data_buffer, (void*) callback, p_pipe);
		return ret_val;
	}
	else
	if(pipe_type==ULM_D_AND_U_P2P_PIPE)
	{
		// 普通p2p 边下边传pipe
		return ufm_read_file_from_common_data_manager(data_manager,p_data_buffer, (void*) callback,  p_pipe);
	}
#ifdef ENABLE_BT
	else
	if(pipe_type==ULM_D_AND_U_BT_P2P_PIPE)
	{
		
		// bt加速的p2p 边下边传pipe

		/* 1. Get file index from BT task */

		/* 2. Read data from BT data manager */	
		return ufm_speedup_pipe_read_file_from_bt_data_manager(data_manager,p_data_buffer, (void*) callback,  p_pipe);
	}

#endif
#ifdef ENABLE_EMULE
	else if(pipe_type == ULM_D_AND_U_EMULE_P2P_PIPE)
	{
		return ufm_speedup_pipe_read_file_from_emule_data_manager(data_manager, p_data_buffer, (void*)callback, p_pipe);
	}
#endif
	else
		
	{
		sd_assert(FALSE);
		/*
#ifdef ENABLE_BT
			ULM_D_AND_U_BT_PIPE,		  // bt 边下边传pipe
			ULM_PU_BT_PIPE ,  			  // bt 纯上传pipe
			ULM_D_AND_U_BT_P2P_PIPE,   // bt加速的p2p 边下边传pipe
#endif

		*/
	}
	return SUCCESS;
}

#ifdef ENABLE_BT
_int32 ulm_add_pipe_by_info_hash( const _u8 *info_hash  ,DATA_PIPE * p_pipe)
{
	_int32 ret_val = SUCCESS;
	ULM_PIPE_TYPE type;
	
	LOG_DEBUG("ulm_add_pipe_by_info_hash");

    if( 0 == ulc_enable_upload() )
    {
        LOG_DEBUG("ulm_add_pipe_by_info_hash, ulc_enable_upload == 0");
        return ULM_ERR_NOT_ALLOW_UPLOAD;
    }

	if((info_hash==NULL)||(p_pipe==NULL)) return INVALID_ARGUMENT;
		
	if(gp_upload_manager==NULL)	return ULM_ERR_ULM_MGR_NOT_EXIST;
	if(gp_upload_manager->error_code!=SUCCESS) return gp_upload_manager->error_code;
	
	if(p_pipe->_data_pipe_type!=BT_PIPE) return ULM_ERR_INVALID_PIPE_TYPE;
	
	//if(TRUE== upm_is_pipe_full())
	//	return ULM_ERR_TOO_MANY_PIPES;

	type = upm_get_pipe_type(p_pipe);
	
	if(type!=ULM_D_AND_U_BT_PIPE)
	{

		LOG_DEBUG( "ULM_PU_BT_PIPE is not supported yet!");
		sd_assert(FALSE);
	}
		
	ret_val=upm_add_pipe(info_hash  ,p_pipe,type);
	if(ret_val!=SUCCESS) return ret_val;

	if(gp_upload_manager->scheduler_timer_id == 0)
	{
		ret_val = start_timer(ulm_handle_scheduler_timeout,NOTICE_INFINITE,gp_upload_manager->scheduler_time,0,NULL,&(gp_upload_manager->scheduler_timer_id));
		if(ret_val!=SUCCESS) 
		{
			ulm_failure_exit(ret_val);
		}
	}
	return SUCCESS;

}

_int32 ulm_bt_pipe_read_data(void * data_manager,const BT_RANGE *p_bt_range, 
	char *p_data_buffer, _u32 buffer_len, bt_pipe_read_result p_call_back, DATA_PIPE * p_pipe )
{
	_int32 ret_val = SUCCESS;
	ULM_PIPE_TYPE pipe_type;

	LOG_DEBUG( "ulm_read_data_bt:p_pipe= 0x%X,p_bt_range[%llu,%llu]" ,p_pipe,p_bt_range->_pos,p_bt_range->_length);

	sd_assert(p_bt_range->_length!=0);
	sd_assert(p_bt_range->_length<=buffer_len);
	sd_assert(TRUE==upm_is_pipe_unchoked(p_pipe));

	pipe_type=upm_get_pipe_type(p_pipe);

	if(pipe_type!=ULM_D_AND_U_BT_PIPE)
	{

		LOG_DEBUG( "ULM_PU_BT_PIPE is not supported yet!");
		sd_assert(FALSE);
	}

	ret_val= ufm_bt_pipe_read_file_from_bt_data_manager(data_manager,p_bt_range, p_data_buffer, buffer_len,(void*)p_call_back, p_pipe);
	return ret_val;
}
#endif

#ifdef ENABLE_EMULE
_int32 ulm_add_pipe_by_file_hash( const _u8 *file_hash, DATA_PIPE * p_pipe)
{
	_int32 ret_val = SUCCESS;
	ULM_PIPE_TYPE type;
    _u8 file_hash_tmp[CID_SIZE];
	
	LOG_DEBUG("ulm_add_pipe_by_file_hash, pipe:0x%x", p_pipe);

    if( 0 == ulc_enable_upload())
    {
        LOG_DEBUG("ulm_add_pipe_by_file_hash, ulc_enable_upload == 0");
        return ULM_ERR_NOT_ALLOW_UPLOAD;
    }

	if((file_hash==NULL)||(p_pipe==NULL)) return INVALID_ARGUMENT;
		
	if(gp_upload_manager==NULL)	return ULM_ERR_ULM_MGR_NOT_EXIST;
	if(gp_upload_manager->error_code!=SUCCESS) return gp_upload_manager->error_code;
	
	if(p_pipe->_data_pipe_type!=EMULE_PIPE) return ULM_ERR_INVALID_PIPE_TYPE;
	
	//if(TRUE== upm_is_pipe_full())
	//	return ULM_ERR_TOO_MANY_PIPES;

	type = upm_get_pipe_type(p_pipe);
	
	if(type!=ULM_D_AND_U_EMULE_PIPE)
	{
		LOG_DEBUG( "ULM_PU_EMULE_PIPE is not supported yet!");
		sd_assert(FALSE);
	}

    sd_memset(file_hash_tmp, 0, CID_SIZE);
    sd_memcpy(file_hash_tmp, file_hash, FILE_ID_SIZE);
	ret_val=upm_add_pipe(file_hash_tmp  ,p_pipe,type);
	if(ret_val!=SUCCESS) return ret_val;

	if(gp_upload_manager->scheduler_timer_id == 0)
	{
		ret_val = start_timer(ulm_handle_scheduler_timeout,NOTICE_INFINITE,gp_upload_manager->scheduler_time,0,NULL,&(gp_upload_manager->scheduler_timer_id));
		if(ret_val!=SUCCESS) 
		{
			ulm_failure_exit(ret_val);
		}
	}
	return SUCCESS;

}

_int32 ulm_emule_pipe_read_data(void * data_manager,RANGE_DATA_BUFFER * p_data_buffer, read_data_callback_func callback, DATA_PIPE * p_pipe)
{
    ULM_PIPE_TYPE pipe_type;

	LOG_DEBUG( "ulm_emule_pipe_read_data:p_pipe= 0x%X,_range[%u,%u],_data_length=%u,_buffer_length=%u" ,p_pipe,p_data_buffer->_range._index,p_data_buffer->_range._num,p_data_buffer->_data_length,p_data_buffer->_buffer_length);

    sd_assert(p_data_buffer->_data_length!=0);
    sd_assert(p_data_buffer->_data_length<=p_data_buffer->_buffer_length);
    sd_assert(TRUE==upm_is_pipe_unchoked(p_pipe));

    pipe_type=upm_get_pipe_type(p_pipe);

    if(pipe_type!=ULM_D_AND_U_EMULE_PIPE)
    {
        LOG_DEBUG( "ULM_PU_EMULE_PIPE is not supported yet!");
        sd_assert(FALSE);
    }
    return ufm_read_file_from_emule_data_manager(data_manager, p_data_buffer, (void*)callback, p_pipe);
}

#endif


_int32 ulm_cancel_read_data( DATA_PIPE * p_pipe)
{
	_int32 ret_val = SUCCESS;
	_u8 * gcid=NULL;
	ULM_PIPE_TYPE pipe_type;

	LOG_DEBUG( "ulm_cancel_read_data:p_pipe= 0x%X" ,p_pipe);

	pipe_type=upm_get_pipe_type(p_pipe);

	if(pipe_type==ULM_PU_P2P_PIPE)
	{
		// 普通p2p 纯上传pipe
		gcid = upm_get_gcid(p_pipe);
		if ( gcid==NULL )
		{
			return ULM_ERR_RECORD_NOT_FOUND;
		}
		
		ret_val = ufm_cancel_read_file(gcid, p_pipe);
		//CHECK_VALUE(ret_val);
	}
	else
	{
		ret_val =  ufm_cancel_read_from_data_manager((void*)p_pipe);
	}
	return ret_val;
}


_int32 ulm_get_file_size(DATA_PIPE * p_pipe,_u64 * file_size)
{
	ID_RC_INFO *p_rc_info = NULL;
	_u8 * gcid = NULL;
	ULM_PIPE_TYPE pipe_type = upm_get_pipe_type(p_pipe);

	LOG_DEBUG("ulm_get_file_size:p_pipe= 0x%X" ,p_pipe);

	if (pipe_type == ULM_PU_P2P_PIPE)
	{
		// 普通p2p 纯上传pipe
		gcid = upm_get_gcid(p_pipe);
		if (gcid == NULL)
		{
			CHECK_VALUE(-1);
		}
		
		p_rc_info = rclist_query(gcid);
		if (p_rc_info == NULL)
		{
			CHECK_VALUE( ULM_ERR_RECORD_NOT_FOUND);
		}
		
		*file_size = p_rc_info->_size;
	}
	else
	{
		*file_size = pi_get_file_size(p_pipe);
	}
	
	sd_assert(*file_size != 0);
	LOG_DEBUG("ulm_get_file_size:p_pipe= 0x%X,file_size=%llu", p_pipe, *file_size);
		
	return SUCCESS;
}


/*--------------------------------------------------------------------------*/
/*                Interfaces provid for data_manager				        */
/*--------------------------------------------------------------------------*/
//
BOOL ulm_handle_invalid_record( const _u8 *gcid,_int32 err_code)
{
	_int32 ret_val = SUCCESS;
	ID_RC_INFO info,*p_rc_info = NULL;
	char new_path[MAX_FULL_PATH_BUFFER_LEN];

	LOG_DEBUG("ulm_notify_invalid_record:err_code=%d",err_code);

	if(gp_upload_manager==NULL)	return ULM_ERR_ULM_MGR_NOT_EXIST;
	
	p_rc_info = rclist_query( gcid);
	sd_assert(p_rc_info!=NULL);
		
	if ( p_rc_info!=NULL )
	{
		LOG_DEBUG( "Record found from the list:path= %s" ,p_rc_info->_path);
		/* Check if the file is not modified */
		if(ulm_check_is_file_modified(p_rc_info->_size,p_rc_info->_path)==FALSE)
		{
			/*列表文件内容没有变动过,不需要删除*/
			LOG_DEBUG( "Record found and file is not modified,do not need to delete from rc_list!" );
			return FALSE;
		}

		/* Check if the file end with ".rf" */
		if(ulm_is_td_file(p_rc_info->_path)==TRUE)
		{
			sd_memset(new_path,0,MAX_FULL_PATH_BUFFER_LEN);
			sd_strncpy(new_path, p_rc_info->_path,MAX_FULL_PATH_BUFFER_LEN);
			new_path[sd_strlen(p_rc_info->_path)-3]='\0';
			if(ulm_check_is_file_modified(p_rc_info->_size,new_path)==FALSE)
			{
				ret_val=rclist_change_rc_path( gcid,new_path,sd_strlen(new_path));
				if(ret_val==SUCCESS)
				{
					/*文件名被修改了，但是内容没有变动过,不需要删除*/
					return FALSE;
				}
			}
		}
			
		/*列表文件内容有变动过,不可用*/
		LOG_DEBUG("Record found and file is modified,delete from rc_list!" );
		sd_memset(&info,0,sizeof(ID_RC_INFO));
		sd_memcpy(&info, p_rc_info, sizeof(ID_RC_INFO));
		ulm_del_record(info._size, info._tcid,info._gcid);
		return TRUE;
	}
	
	return FALSE;
}


/*--------------------------------------------------------------------------*/
/*                Interfaces provid for reporter				        */
/*--------------------------------------------------------------------------*/
_int32  ulm_isrc_online_resp( BOOL result, _int32 should_report_rcList )
{
	_int32 ret_val = SUCCESS;
	LIST * p_rc_list = NULL;
	
	LOG_DEBUG("ulm_isrc_online_resp:result=%d,should_report_rcList=%d",result,should_report_rcList);
	if(gp_upload_manager==NULL)	return ULM_ERR_ULM_MGR_NOT_EXIST;
	
	if(result == TRUE)
	{
		gp_upload_manager->is_rc_online=TRUE;
		if(should_report_rcList>0)
		{
			/* Report the rc list to phub */
			p_rc_list = rclist_get_rc_list();
			if(0!=list_size(p_rc_list))
			{
				#ifdef EMBED_REPORT
				ret_val = reporter_rc_list(p_rc_list,(_u32)get_peer_capability());
				if(ret_val== SUCCESS)
				{
					gp_upload_manager->is_rclist_reported = TRUE;
				}
				#endif
			}
			else
			{
				gp_upload_manager->is_rclist_reported = FALSE;
			}
			
		}
		else
		{
			//ret_val = ULM_ERR_NOT_NEED_REPORT_RCLIST; 
			gp_upload_manager->is_rclist_reported = TRUE;
		}
		
	}
	else
	{
		if(gp_upload_manager->isrc_online_timer_id!=0)
		{
			cancel_timer(gp_upload_manager->isrc_online_timer_id);
			gp_upload_manager->isrc_online_timer_id=0;
		}
		ret_val =   start_timer(ulm_handle_isrc_online_timeout, NOTICE_ONCE, 600 *1000, 0,(void*)gp_upload_manager,&(gp_upload_manager->isrc_online_timer_id));  
	}
	
	if(ret_val!= SUCCESS)
		ulm_failure_exit(ret_val);
	
	return SUCCESS;
}


_int32  ulm_report_rclist_resp( BOOL result )
{
	LOG_DEBUG("ulm_report_rclist_resp:result=%d",result);
	//if(gp_upload_manager==NULL)	return ULM_ERR_ULM_MGR_NOT_EXIST;
	
	if(result != TRUE)
	{
		gp_upload_manager->is_rclist_reported = FALSE;
		//	ulm_failure_exit(ULM_ERR_RCLIST_REPORT_FAILED);
	}
	
	return SUCCESS;
}


_int32  ulm_insert_rc_resp( BOOL result)
{
	LOG_DEBUG("ulm_insert_rc_resp:result=%d",result);
	return SUCCESS;
}


_int32  ulm_delete_rc_resp(BOOL result)
{
	LOG_DEBUG("ulm_delete_rc_resp:result=%d",result);
	return SUCCESS;
}

#ifdef _CONNECT_DETAIL
void ulm_update_pipe_info(void)
{
	_int32 ret_val=SUCCESS;
	 PEER_PIPE_INFO_ARRAY * p_pipe_info_array=NULL;

	LOG_DEBUG("ulm_update_pipe_info");
	upm_update_pipe_info();
	
	 p_pipe_info_array= upm_get_pipe_info();

	ret_val = cus_rws_begin_write_data(g_pipe_info_array_brd_ptr, NULL);
	if(ret_val!=SUCCESS) 
	{
		LOG_DEBUG("ulm_update_pipe_info:Write Error!");
		return;
	}

	sd_memcpy(&g_pipe_info_array, p_pipe_info_array, sizeof(PEER_PIPE_INFO_ARRAY));
	
	cus_rws_end_write_data(g_pipe_info_array_brd_ptr);
	
	return ;
}
_int32 ulm_get_pipe_info_array(PEER_PIPE_INFO_ARRAY * p_pipe_info_array)
{
	LOG_DEBUG("ulm_get_pipe_info_array");
	if(SUCCESS==cus_rws_begin_read_data_block(g_pipe_info_array_brd_ptr, NULL))
	{
		sd_memcpy(p_pipe_info_array,&g_pipe_info_array,  sizeof(PEER_PIPE_INFO_ARRAY));

		cus_rws_end_read_data(g_pipe_info_array_brd_ptr);
		
		return SUCCESS;
	}
	LOG_DEBUG("ulm_get_pipe_info_array:Error!");
	return -1;
}
#endif

/*--------------------------------------------------------------------------*/
/*                Callback function for timer				        */
/*--------------------------------------------------------------------------*/
_int32 ulm_handle_scheduler_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid)
{

	LOG_DEBUG("ulm_handle_scheduler_timeout");

	if(errcode == MSG_TIMEOUT)
	{
		if(gp_upload_manager==NULL)	return ULM_ERR_ULM_MGR_NOT_EXIST;
		
		if(/*msg_info->_device_id*/msgid == gp_upload_manager->scheduler_timer_id)
		{
		  	ulm_scheduler();
		}
	}
	return SUCCESS;
}
 _int32 ulm_handle_isrc_online_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid)
 {
	_int32 ret_val = SUCCESS;
	LOG_DEBUG("ulm_handle_isrc_online_timeout:%d",errcode);
	
	if(errcode == MSG_TIMEOUT)
	{
		if(/*msg_info->_device_id*/msgid == gp_upload_manager->isrc_online_timer_id)
		{
			gp_upload_manager->isrc_online_timer_id = 0;
			#ifdef EMBED_REPORT
			/* Report to the phub  */
			ret_val = reporter_isrc_online();
			if(ret_val!=SUCCESS)
			{
				ulm_isrc_online_resp( FALSE, ret_val );
			}
			#endif
		}
	}
		
	return SUCCESS;
 }

/*--------------------------------------------------------------------------*/
/*                Internal functions				        */
/*--------------------------------------------------------------------------*/


_int32 ulm_scheduler(void)
{

	LOG_DEBUG("ulm_scheduler");

	ufm_scheduler();

	upm_scheduler();

#ifdef _CONNECT_DETAIL
	ulm_update_pipe_info();
#endif

	return SUCCESS;
}

/* Check if the file is not modified */
BOOL ulm_check_is_file_modified(_u64 filesize,const char * filepath)
{
	_int32 ret_val = SUCCESS;
	_u64 file_size = 0;

		
		ret_val = sd_get_file_size_and_modified_time(filepath,&file_size,NULL);

		if((ret_val!=0)||((ret_val==0)&&(file_size!=filesize))||
			(!sd_is_file_readable(filepath)))
		{
			/*列表文件内容有变动过,不可用*/
			LOG_DEBUG("ulm_check_if_file_modified:check_file :ret_val=%d,file_size=%llu,rc_info._size=%llu,rc_info._path=%s",ret_val,file_size,filesize,filepath);
			return TRUE;
		}
		return FALSE; 
}
_int32 ulm_failure_exit(_int32 err_code)
{
	LOG_DEBUG("ulm_failure_exit:err_code = %d",err_code);

	if(err_code==ULM_ERR_ISRC_ONLINE_FAILED)
	    return SUCCESS;
	
	gp_upload_manager->error_code = err_code;
	
	/* Stop the scheduler timer */
	if(gp_upload_manager->scheduler_timer_id != 0)
	{
		cancel_timer(gp_upload_manager->scheduler_timer_id);
		gp_upload_manager->scheduler_timer_id = 0;
	}
	
	/* Close and clean up all the p2p pipes */
	//ulm_clean_pipes();
	CHECK_VALUE(err_code);
	
	return SUCCESS;
}

void ulm_stat_add_up_bytes(_u32 bytes)
{
	gp_upload_manager->cur_stat._up_data_bytes+=bytes;
}

void _ulm_combine_stat(UPLOAD_MANAGER_STAT * p_dst,UPLOAD_MANAGER_STAT * p_last,UPLOAD_MANAGER_STAT * p_cur)
{
	if(0==p_last->_begin_time)
	{
		memcpy(p_dst,p_cur,sizeof(UPLOAD_MANAGER_STAT));
		return ;	
	}
	p_dst->_begin_time = p_last->_begin_time;
	p_dst->_end_time = p_cur->_end_time;	
	if(p_dst->_end_time < p_dst->_begin_time)
	{
		LOG_DEBUG("_ulm_combine_stat warn, time is incorrect ,last=(%u - %u) cur=(%u - %u)",
			p_last->_begin_time,p_last->_end_time,p_cur->_begin_time,p_cur->_end_time);	
	}
	p_dst->_up_duration = p_last->_up_duration + p_cur->_up_duration;
	p_dst->_up_use_duration = p_last->_up_use_duration + p_cur->_up_use_duration;
	p_dst->_up_data_bytes = p_last->_up_data_bytes + p_cur->_up_data_bytes;
	p_dst->_up_pipe_num = p_last->_up_pipe_num + p_cur->_up_pipe_num;
	p_dst->_up_passive_pipe_num = p_last->_up_passive_pipe_num + p_cur->_up_passive_pipe_num;
}

void _ulm_load_stat()
{
	UPLOAD_MANAGER_STAT * p_stat;
	char buf[MAX_CFG_VALUE_LEN];
	p_stat = &gp_upload_manager->last_stat;
	settings_get_int_item("upload_stat.begin_time",(_int32*)&p_stat->_begin_time);
	settings_get_int_item("upload_stat.end_time",(_int32*)&p_stat->_end_time);
	settings_get_int_item("upload_stat.up_duration",(_int32*)&p_stat->_up_duration);
	settings_get_int_item("upload_stat.up_use_duration",(_int32*)&p_stat->_up_use_duration);
	settings_get_int_item("upload_stat.up_pipe_num",(_int32*)&p_stat->_up_pipe_num);
	settings_get_int_item("upload_stat.up_passive_pipe_num",(_int32*)&p_stat->_up_passive_pipe_num);
	buf[0]=0;
	settings_get_str_item("upload_stat.up_data_bytes",buf);
	p_stat->_up_data_bytes = atoll(buf);
}

void _ulm_flush_stat()
{
	UPLOAD_MANAGER_STAT * p_stat;
	char buf[MAX_CFG_VALUE_LEN];
	p_stat=&gp_upload_manager->last_stat;
	settings_set_int_item("upload_stat.begin_time",p_stat->_begin_time);
	settings_set_int_item("upload_stat.end_time",p_stat->_end_time);
	settings_set_int_item("upload_stat.up_duration",p_stat->_up_duration);
	settings_set_int_item("upload_stat.up_use_duration",p_stat->_up_use_duration);
	sd_snprintf(buf,sizeof(buf)-1,"%llu",p_stat->_up_data_bytes);
	buf[sizeof(buf)-1]=0;
	settings_set_str_item("upload_stat.up_data_bytes",buf);
	settings_set_int_item("upload_stat.up_pipe_num",p_stat->_up_pipe_num);
	settings_set_int_item("upload_stat.up_passive_pipe_num",p_stat->_up_passive_pipe_num);
	settings_config_save();
}

_int32 _ulm_handle_upload_manager_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid)
{
	_u32 now,  seqno;
	UPLOAD_MANAGER_STAT total_stat,* p_cur_stat,*p_last_stat;

	if (0 == gp_upload_manager->slow_timer_id )
	{
		return SUCCESS;
	}

	sd_time(&now);
	p_cur_stat = &gp_upload_manager->cur_stat;
	p_last_stat = &gp_upload_manager->last_stat;

#if 0/*for test*/
	p_cur_stat->_up_data_bytes+=100;
#endif
	//更新当前统计
	p_cur_stat->_end_time= now;

 	if( now <  p_cur_stat->_begin_time) 
	{
		p_cur_stat->_up_duration += gp_upload_manager->slow_timer_interval;
	}
	else
	{
		p_cur_stat->_up_duration += now-p_cur_stat->_begin_time;
	}

	if(0 != gp_upload_manager->up_using_begin_time)
	{	
		if(gp_upload_manager->up_using_begin_time < p_cur_stat->_begin_time)
		{
			gp_upload_manager->up_using_begin_time = p_cur_stat->_begin_time;
		}
	
		if( now < gp_upload_manager->up_using_begin_time)
		{	
			p_cur_stat->_up_use_duration += gp_upload_manager->slow_timer_interval;
		}
		else
		{
			p_cur_stat->_up_use_duration += now-gp_upload_manager->up_using_begin_time ;
		}
		gp_upload_manager->up_using_begin_time = now;
	}

	//	合并统计数据
	_ulm_combine_stat(&total_stat,p_last_stat,p_cur_stat);
	memcpy(p_last_stat,&total_stat,sizeof(UPLOAD_MANAGER_STAT));
	memset(p_cur_stat,0,sizeof(UPLOAD_MANAGER_STAT));
	p_cur_stat->_begin_time= now;
	
	//备份统计数据
	_ulm_flush_stat();
	
	if(MSG_CANCELLED == errcode)
	{	
		gp_upload_manager->slow_timer_id = 0;
		//结束流程时候,无需上报
		return SUCCESS;	
	}
	
	/*上报信息*/
	#ifdef EMBED_REPORT
	if( (p_last_stat->_begin_time> now)||
		( now - p_last_stat->_begin_time > 86400) )
	{
		if(SUCCESS==report_upload_stat(p_last_stat->_begin_time,p_last_stat->_end_time,p_last_stat->_up_duration,p_last_stat->_up_use_duration,
			p_last_stat->_up_data_bytes,p_last_stat->_up_pipe_num,p_last_stat->_up_passive_pipe_num ,&seqno) )
		{
			gp_upload_manager->rep_seqno = seqno;
			gp_upload_manager->last_stat_state = 1;
		}
	}
	#endif
	
	return SUCCESS;
}

void ulm_stat_handle_report_response(_u32 seqno , _int32 result)
{
	if (0 == gp_upload_manager->slow_timer_id )
	{
		LOG_ERROR("ulm_stat_handle_report_response  warn  , slow_timer_id is canceled");
		return ;
	}
	if( (seqno != gp_upload_manager->rep_seqno )||
		( 1 != result  ) )
	{
		LOG_ERROR("ulm_stat_handle_report_response  warn, seqno=%d result=%d expected_seqno=%d",seqno,result,gp_upload_manager->rep_seqno);
		return ;
	}
	
	gp_upload_manager->last_stat_state = 0;
	gp_upload_manager->rep_seqno = 0;

	//清理已报统计
	memset(&gp_upload_manager->last_stat,0,sizeof(UPLOAD_MANAGER_STAT));
	_ulm_flush_stat();
} 

#endif /* UPLOAD_ENABLE */

#endif /* __UPLOAD_MANAGER_C_20081111 */

