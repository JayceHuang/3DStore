#include "utility/queue.h"
#include "utility/time.h"
#include "utility/bytebuffer.h"
#include "utility/utility.h"
#include "utility/rw_sharebrd.h"
#include "utility/version.h"
#include "platform/sd_task.h"
#include "asyn_frame/asyn_frame.h"
#include "asyn_frame/device_handler.h"

#include "task_manager.h"
#include "res_query/res_query_interface.h"
#include "res_query/dk_res_query/dk_setting.h"
#include "socket_proxy_interface.h"
#include "p2p_data_pipe/p2p_pipe_interface.h"
#include "data_manager/data_manager_interface.h"
#include "http_data_pipe/http_data_pipe_interface.h"
#include "ftp_data_pipe/ftp_data_pipe_interface.h"
#include "download_task/download_task.h"
#include "embed_thunder_version.h"
#include "embed_thunder.h"
#include "socket_proxy_interface.h"
#ifdef ENABLE_BT
#include "bt_download/bt_task/bt_task_interface.h"
#include "bt_download/bt_task/bt_task.h"

#include "torrent_parser/torrent_parser_interface.h"
#include "bt_download/bt_magnet/bt_magnet_task.h"
#endif
#ifdef ENABLE_EMULE
#include "emule/emule_interface.h"
#include "emule/emule_utility/emule_utility.h"
#endif
#ifdef ENABLE_VOD
#include "vod_data_manager/vod_data_manager_interface.h"
#include "http_server/http_server_interface.h"
#endif
#ifdef AUTO_LAN
#include "autonomous_lan/al_interface.h"
#endif
#include "reporter/reporter_interface.h"
#include "p2p_transfer_layer/ptl_interface.h"
#include "connect_manager/connect_manager_interface.h"
#include "p2sp_download/p2sp_task.h"

#ifdef UPLOAD_ENABLE
#include "upload_manager/upload_manager.h"
#include "autonomous_lan/al_interface.h"
#endif

#ifdef ENABLE_DRM 
#include "drm/drm.h"
#endif
#ifdef ENABLE_WB_XML_ZIP
#include "platform/sd_gz.h"
#endif /* ENABLE_WB_XML_ZIP */	


#include "p2p_transfer_layer/ptl_cmd_sender.h"
#include "cmd_proxy/cmd_proxy_manager.h"

#include "platform/sd_network.h"
#include "http_data_pipe/http_mini_interface.h"

#include "utility/logid.h"
#ifdef LOGID
#undef LOGID
#endif

#define LOGID LOGID_TASK_MANAGER

#include "utility/logger.h"
#include "high_speed_channel/hsc_perm_query.h"
//#include "drm/drm.h"

#ifdef ENABLE_HSC
#include "high_speed_channel/hsc_perm_query.h"
#endif

#include "http_server/http_server_interface.h"

    
/*--------------------------------------------------------------------------*/
/*                Global declarations				        */
/*--------------------------------------------------------------------------*/
/* Pointer of the task_manager,only one task_manager when program is running */

typedef struct _RESOURCE_STAT_INFO
{
    int total_num;               //资源总数
    int valid_num;               //有效资源总数
    int connected_pipe_num;      //已连接资源总数
    int download_speed;          //下载数度(这个可以自己算出来)
    _int64 download_size;        //有效数据下载大小
    _int64 recv_size;            //接收数据大小
}RESOURCE_STAT_INFO;

typedef enum _MEDIA_TASK_STATUS {M_TASK_IDLE = 0, M_TASK_RUNNING, M_TASK_VOD,  M_TASK_SUCCESS, M_TASK_FAILED, M_TASK_STOPPED}MEDIA_TASK_STATUS;

typedef struct _STAT_INFO
{
    RESOURCE_STAT_INFO peer;
    RESOURCE_STAT_INFO server;   //server资源
    RESOURCE_STAT_INFO cdn;      //cdn资源

    _int64 total_recv_size;      //接收数据大小
    _int64 total_download_size;  //有效数据下载大小
    int download_time;           //下载用时
    MEDIA_TASK_STATUS status;    //当前状态
    _int64 file_size;
}MEDIA_TASK_STAT_INFO;

static TASK_MANAGER * g_pTask_manager=NULL;

static ET_TASK_INFO  g_Task_Info[MAX_ET_ALOW_TASKS];

static LIST g_delete_task_notice_list; // 保存删除任务之前回调通知

#ifdef ENABLE_HSC
HSC_INFO_BRD g_task_hsc_info[MAX_ET_ALOW_TASKS];
RW_SHAREBRD *g_task_hsc_info_brd_ptr = NULL;
#endif /* ENABLE_HSC */
#ifdef ENABLE_LIXIAN
static LIXIAN_INFO_BRD g_task_lixian_info[MAX_ET_ALOW_TASKS];
static RW_SHAREBRD *g_task_lixian_info_brd_ptr = NULL;
#endif /* ENABLE_LIXIAN */
#ifdef ENABLE_BT
static BT_FILE_INFO_POOL  g_bt_file_info_pool[MAX_ET_ALOW_TASKS];
static RW_SHAREBRD *g_bt_file_info_pool_brd_ptr = NULL;
#endif  /* ENABLE_BT */
static _u32 g_mini_htttp_num = 0;
static TASK_LOCK * p_tm_post_lock = NULL;

#ifdef MACOS
static SEVENT_HANDLE g_post_handle;
#endif

/* control enable and disable modules*/
typedef _int32 (*tm_handle_module_set_hook)(const _int32 set_value);
typedef struct _t_tm_handle_module_set{
	tm_handle_module_set_hook register_func;
	_int32 enable;
} TM_HANDLE_MODULE_SET;
static TM_HANDLE_MODULE_SET g_handle_module_set_hook[] = {
											{ptl_ping_sn_notask_hook, 0},
#if defined(ENABLE_EMULE) && defined(ENABLE_EMULE_PROTOCOL)
											{emule_nat_server_notask_hook, 0},
#else
											{0, 0},
#endif
											{dm_donot_fast_stop_hook, 0}
													};
static _int32 g_module_set_hook_num = sizeof(g_handle_module_set_hook)/sizeof(TM_HANDLE_MODULE_SET);

#define TM_RUN_DOWNTASK_HOOKS(download_task) \
	do {\
		int i;\
		for(i = 0; i < g_module_set_hook_num; i++) {\
			if(g_handle_module_set_hook[i].register_func && g_handle_module_set_hook[i].enable == 1) \
				g_handle_module_set_hook[i].register_func(download_task);\
		}\
	} while(0)


static _int32 g_tm_thread_pid = 0;

static void do_notice_delete_task(TASK *task_ptr);

static void tm_reset_global_task_manager_sevent_handle()
{
    LOG_DEBUG("g_pTask_manager->_p_sevent_handle=%d", g_pTask_manager->_p_sevent_handle);
    g_pTask_manager->_p_sevent_handle = NULL;
}

static _int32 tm_set_global_task_manager_sevent_handle(SEVENT_HANDLE *p_sevent_handle)
{
    LOG_URGENT("g_pTask_manager->_p_sevent_handle=%d, rval=%d", g_pTask_manager->_p_sevent_handle, p_sevent_handle);
    _int32 ret = SUCCESS;
    if (NULL != g_pTask_manager->_p_sevent_handle)
    {
        ret = TM_ERR_OPERATION_CLASH;
    }
    else
    {
        g_pTask_manager->_p_sevent_handle = p_sevent_handle;
    }
    LOG_URGENT("set global task ret=%d", ret);
    return ret;
}

static _int32 pt_get_task_download_stat_info( struct _tag_task *p_task,  ET_TASK_INFO *p_et_task_info)
{
    P2SP_TASK * p_p2sp_task = (P2SP_TASK *)p_task;
    DATA_MANAGER *p_data_manager = &p_p2sp_task->_data_manager;
    p_et_task_info->_orisrv_recv_size = p_data_manager->_origin_dl_bytes;	/*原始路径下载字节数*/
    p_et_task_info->_orisrv_download_size = p_data_manager->_origin_dl_bytes;

    p_et_task_info->_altsrv_recv_size = p_data_manager->_server_recv_bytes;	/*候选url下载字节数*/
    p_et_task_info->_altsrv_download_size = p_data_manager->_server_dl_bytes;
	if(NULL != p_p2sp_task->_data_manager._origin_res)
	{
	    if (p_p2sp_task->_data_manager._origin_res->_resource_type == HTTP_RESOURCE ||
	            p_p2sp_task->_data_manager._origin_res->_resource_type == FTP_RESOURCE )
	    {
	        p_et_task_info->_altsrv_recv_size -= p_data_manager->_origin_dl_bytes;
	        p_et_task_info->_altsrv_download_size -= p_data_manager->_origin_dl_bytes;
	    }
	}

    p_et_task_info->_p2p_recv_size = p_data_manager->_peer_recv_bytes;	/*p2p下载字节数*/
    p_et_task_info->_p2p_download_size = p_data_manager->_peer_dl_bytes;

#ifdef ENABLE_CDN
    p_et_task_info->_lixian_recv_size  = p_data_manager->_lixian_recv_bytes;	/*离线下载字节数*/
    p_et_task_info->_lixian_download_size = p_data_manager->_lixian_dl_bytes;
#else
    p_et_task_info->_lixian_recv_size  = 0;	/*离线下载字节数*/
    p_et_task_info->_lixian_download_size = 0;
#endif

    p_et_task_info->_total_recv_size = p_data_manager->_server_recv_bytes 
                            + p_data_manager->_peer_recv_bytes ;
#ifdef ENABLE_CDN
    p_et_task_info->_total_recv_size += p_data_manager->_cdn_recv_bytes
                            + p_data_manager->_lixian_recv_bytes;
#endif
    return SUCCESS;

}

static _int32 emule_get_task_download_stat_info( struct _tag_task *p_task,  ET_TASK_INFO *p_et_task_info)
{
#ifdef ENABLE_EMULE
    EMULE_TASK * p_emule_task = (EMULE_TASK *)p_task;
    EMULE_DATA_MANAGER *p_data_manager = p_emule_task->_data_manager;
    p_et_task_info->_orisrv_recv_size = p_data_manager->_origin_dl_bytes;	/*原始路径下载字节数*/
    p_et_task_info->_orisrv_download_size = p_data_manager->_origin_dl_bytes;

    p_et_task_info->_altsrv_recv_size = p_data_manager->_server_dl_bytes;	/*候选url下载字节数*/
    p_et_task_info->_altsrv_download_size = p_data_manager->_server_dl_bytes;

    p_et_task_info->_p2p_recv_size = p_data_manager->_peer_dl_bytes;	/*p2p下载字节数*/
    p_et_task_info->_p2p_download_size = p_data_manager->_peer_dl_bytes;

#ifdef ENABLE_CDN
    p_et_task_info->_lixian_recv_size  = p_data_manager->_lixian_dl_bytes;	/*离线下载字节数*/
    p_et_task_info->_lixian_download_size = p_data_manager->_lixian_dl_bytes;
#else
    p_et_task_info->_lixian_recv_size  = 0;	/*离线下载字节数*/
    p_et_task_info->_lixian_download_size = 0;
#endif

    p_et_task_info->_total_recv_size = p_data_manager->_server_dl_bytes 
                            + p_data_manager->_peer_dl_bytes ;
#ifdef ENABLE_CDN
    p_et_task_info->_total_recv_size += p_data_manager->_cdn_dl_bytes
                            + p_data_manager->_lixian_dl_bytes;
#endif
    return SUCCESS;
#endif
}


static _int32 bt_get_task_download_stat_info( struct _tag_task *p_task,  ET_TASK_INFO *p_et_task_info)
{
#ifdef ENABLE_BT
    BT_TASK * p_bt_task = (BT_TASK *)p_task;
    BT_FILE_MANAGER *p_file_manager = &p_bt_task->_data_manager._bt_file_manager;
    p_et_task_info->_orisrv_recv_size = p_file_manager->_origin_dl_bytes;	/*原始路径下载字节数*/
    p_et_task_info->_orisrv_download_size = p_file_manager->_origin_dl_bytes;

    p_et_task_info->_altsrv_recv_size = p_file_manager->_server_dl_bytes;	/*候选url下载字节数*/
    p_et_task_info->_altsrv_download_size = p_file_manager->_server_dl_bytes;

    p_et_task_info->_p2p_recv_size = p_file_manager->_peer_dl_bytes;	/*p2p下载字节数*/
    p_et_task_info->_p2p_download_size = p_file_manager->_peer_dl_bytes;

#ifdef ENABLE_CDN
    p_et_task_info->_lixian_recv_size  = p_file_manager->_lixian_dl_bytes;	/*离线下载字节数*/
    p_et_task_info->_lixian_download_size = p_file_manager->_lixian_dl_bytes;
#else
    p_et_task_info->_lixian_recv_size  = 0;	/*离线下载字节数*/
    p_et_task_info->_lixian_download_size = 0;
#endif

    p_et_task_info->_total_recv_size = p_file_manager->_server_dl_bytes 
                            + p_file_manager->_peer_dl_bytes ;
#ifdef ENABLE_CDN
    p_et_task_info->_total_recv_size += p_file_manager->_cdn_dl_bytes
                            ;//+ p_file_manager->_lixian_dl_bytes;
#endif
#endif
    return SUCCESS;

}

static _int32 tm_update_task_info()
{
    LOG_DEBUG("tm_update_task_info");
	_int32 _list_size =  list_size(&(g_pTask_manager->_task_list)); 
	if (0 == _list_size)
	{
	    return 0;
	}

	if (SUCCESS != begin_write_data(NULL))
	{
	    return 0;
	}

	pLIST_NODE _p_list_node = LIST_BEGIN(g_pTask_manager->_task_list);
	sd_memset(g_Task_Info, 0, sizeof(ET_TASK_INFO)*MAX_ET_ALOW_TASKS);
	_int32 i = 0;
	TASK* _p_task = NULL;
	while (_list_size)
	{
	    _p_task = (TASK *)LIST_VALUE(_p_list_node);
	    LOG_DEBUG("_list_size=%d, i=%d, task_id=%d, _task_type=%d, task_status=%d, file_create=%d, size=%llu/%llu, speed=%u. failed_code=%d",
	        _list_size, i, _p_task->_task_id, _p_task->_task_type, 
	        _p_task->task_status, _p_task->_file_create_status, 
	        _p_task->_downloaded_data_size, _p_task->file_size, _p_task->speed, _p_task->failed_code);
	    if ((_p_task->task_status == TASK_S_RUNNING) || (_p_task->task_status == TASK_S_VOD))
	    {
	        if (_p_task->_task_type == P2SP_TASK_TYPE)
	        {
	            pt_update_task_info( _p_task );
	        }
#ifdef ENABLE_BT
            else if(_p_task->_task_type==BT_TASK_TYPE)
            {
                bt_update_info(_p_task);
            }
#endif /* #ifdef ENABLE_BT */							
#ifdef ENABLE_EMULE
            else if(_p_task->_task_type == EMULE_TASK_TYPE)
            {
                emule_update_task_info(_p_task);
            }
#endif
        }

        if (i < MAX_ET_ALOW_TASKS)
        {
            g_Task_Info[i]._task_id = _p_task->_task_id;
            g_Task_Info[i]._connecting_peer_pipe_num = _p_task->connecting_peer_pipe_num;
            g_Task_Info[i]._connecting_server_pipe_num = _p_task->connecting_server_pipe_num;
            g_Task_Info[i]._dowing_peer_pipe_num = _p_task->dowing_peer_pipe_num;
            g_Task_Info[i]._dowing_server_pipe_num = _p_task->dowing_server_pipe_num;
#ifdef _CONNECT_DETAIL
            g_Task_Info[i].downloading_tcp_peer_pipe_num = _p_task->downloading_tcp_peer_pipe_num;
            g_Task_Info[i].downloading_udp_peer_pipe_num = _p_task->downloading_udp_peer_pipe_num;
            g_Task_Info[i].downloading_tcp_peer_pipe_speed = _p_task->downloading_tcp_peer_pipe_speed;
            g_Task_Info[i].downloading_udp_peer_pipe_speed = _p_task->downloading_udp_peer_pipe_speed;
            g_Task_Info[i].idle_server_res_num = _p_task->idle_server_res_num;
            g_Task_Info[i].using_server_res_num = _p_task->using_server_res_num;
            g_Task_Info[i].candidate_server_res_num = _p_task->candidate_server_res_num;
            g_Task_Info[i].retry_server_res_num = _p_task->retry_server_res_num;
            g_Task_Info[i].discard_server_res_num = _p_task->discard_server_res_num;
            g_Task_Info[i].destroy_server_res_num = _p_task->destroy_server_res_num;
            g_Task_Info[i].idle_peer_res_num = _p_task->idle_peer_res_num;
            g_Task_Info[i].using_peer_res_num = _p_task->using_peer_res_num;
            g_Task_Info[i].candidate_peer_res_num = _p_task->candidate_peer_res_num;
            g_Task_Info[i].retry_peer_res_num = _p_task->retry_peer_res_num;
            g_Task_Info[i].discard_peer_res_num = _p_task->discard_peer_res_num;
            g_Task_Info[i].destroy_peer_res_num = _p_task->destroy_peer_res_num;
            g_Task_Info[i].cm_level = _p_task->cm_level;
            if (NULL != _p_task->_peer_pipe_info_array)
            {
                sd_memcpy(&g_Task_Info[i]._peer_pipe_info_array, _p_task->_peer_pipe_info_array, sizeof(PEER_PIPE_INFO_ARRAY));
            }
            else
            {
                g_Task_Info[i]._peer_pipe_info_array._pipe_info_num = 0;
            }
            g_Task_Info[i]._valid_data_speed = _p_task->valid_data_speed;
            g_Task_Info[i]._bt_dl_speed= _p_task->_bt_dl_speed;
            g_Task_Info[i]._bt_ul_speed= _p_task->_bt_ul_speed;
#endif
            g_Task_Info[i]._failed_code= _p_task->failed_code;
            g_Task_Info[i]._file_size= _p_task->file_size;
            g_Task_Info[i]._finished_time= _p_task->_finished_time;
            g_Task_Info[i]._peer_speed= _p_task->peer_speed;
            g_Task_Info[i]._progress= _p_task->progress;
            g_Task_Info[i]._server_speed= _p_task->server_speed;
            g_Task_Info[i]._speed= _p_task->speed;
            g_Task_Info[i]._start_time= _p_task->_start_time;
            g_Task_Info[i]._task_status= _p_task->task_status;
            g_Task_Info[i]._file_create_status= _p_task->_file_create_status;
            g_Task_Info[i]._downloaded_data_size= _p_task->_downloaded_data_size;
            g_Task_Info[i]._written_data_size= _p_task->_written_data_size;
            g_Task_Info[i]._ul_speed= _p_task->ul_speed;

//-----------------------------------------------------------------------------------------------
#ifdef ENABLE_HSC
            g_Task_Info[i]._hsc_recv_size = _p_task->_hsc_info._dl_bytes;	/*高速通道下载字节数*/
            g_Task_Info[i]._hsc_download_size = _p_task->_hsc_info._dl_bytes;
#endif

            switch( _p_task->_task_type )
            {
                case P2SP_TASK_TYPE:
                {
                    pt_get_task_download_stat_info(_p_task, &g_Task_Info[i]);
                    break;
                }
                case BT_TASK_TYPE:
                case BT_MAGNET_TASK_TYPE:
                    bt_get_task_download_stat_info(_p_task, &g_Task_Info[i]);
                    break;
                case EMULE_TASK_TYPE:
#ifdef ENABLE_EMULE
                    emule_get_task_download_stat_info(_p_task, &g_Task_Info[i]);
#endif
                    break;
            }
	
            g_Task_Info[i]._download_time = _p_task->_finished_time - _p_task->_start_time;           //下载用时

		    /*
		        统计起速时长和零速时长
			*/
            _u32 now_time = 0;
            sd_time_ms(&now_time);
			if(0==_p_task->_last_download_bytes)
			{
				if(0!=_p_task->_downloaded_data_size)/*第一次统计到数据, 记录起速时长*/
				{
					sd_time(&_p_task->_first_byte_arrive_time);
					_p_task->_first_byte_arrive_time-=_p_task->_start_time;
					if(_p_task->_first_byte_arrive_time>1) /*减去统计误差,tm_update_task_info定时器驱动时一秒一次*/
					{
						_p_task->_first_byte_arrive_time-=1;
					}
					
				    _p_task->_last_download_bytes=_p_task->_downloaded_data_size;
					_p_task->_last_download_stat_time_ms=now_time;
				}
			}
			else
			{
				if(_p_task->_downloaded_data_size!=_p_task->_last_download_bytes)
				{	/*收到数据,更新统计时间*/
				    _p_task->_last_download_bytes=_p_task->_downloaded_data_size;
					_p_task->_last_download_stat_time_ms=now_time;
				}
				else
				{	/*超过一秒没收到数据,则增加零速时长,并更新统计时间*/
					if(now_time-_p_task->_last_download_stat_time_ms>1000)
					{
						_p_task->_zero_speed_time_ms+=now_time-_p_task->_last_download_stat_time_ms;
						_p_task->_last_download_stat_time_ms=now_time;
					}
				}
			}
			g_Task_Info[i]._first_byte_arrive_time=_p_task->_first_byte_arrive_time;
			g_Task_Info[i]._zero_speed_time=_p_task->_zero_speed_time_ms/1000;
			
            
            if ( (0!=_p_task->file_size) &&
				 (0==_p_task->_download_finish_time) && 
				 (_p_task->file_size==_p_task->_downloaded_data_size) &&  
				 (TASK_S_RUNNING==_p_task->task_status)  ) //数据全部下载完的时间，即_downloaded_data_size==file_size的时间点
            {
                _p_task->_download_finish_time = now_time;
            }
            if (0<_p_task->_download_finish_time && TASK_S_SUCCESS==_p_task->task_status)
            {
                g_Task_Info[i]._checksum_time = (now_time - _p_task->_download_finish_time)/1000;  //校验时长
                _p_task->_download_finish_time = 0;
            } 

            
        }
        
        i++;
        _p_list_node = LIST_NEXT(_p_list_node);
        _list_size--;
    }

    /* unlock data for read */
    end_write_data();

    return SUCCESS;
}

static void tm_update_task()
{
  	tm_update_task_info( );
#ifdef ENABLE_HSC
	tm_update_task_hsc_info( );
#endif
#ifdef ENABLE_LIXIAN
	tm_update_task_lixian_info( );
#endif
}

static _int32 tm_start_update_info_timer()
{
    _int32 ret = SUCCESS;
    if (NULL == g_pTask_manager)
    {
        ret = -1;
    }
    else if (g_pTask_manager->_global_statistic._update_info_timer_id == 0)
	{
	    ret = start_timer(tm_handle_update_info_timeout, 
	        NOTICE_INFINITE, 
	        UPDATE_INFO_WAIT_M_SEC, 
	        0,
	        (void*)g_pTask_manager, 
	        &(g_pTask_manager->_global_statistic._update_info_timer_id));
		if (SUCCESS != ret) 
		{
#ifdef _DEBUG
					CHECK_VALUE(1);
#endif
		}
		TM_RUN_DOWNTASK_HOOKS(1);
	}
	return ret;
}

static void tm_stop_update_info_timer()
{
    if ((NULL != g_pTask_manager) && (0 != g_pTask_manager->_global_statistic._update_info_timer_id))
	{
		cancel_timer(g_pTask_manager->_global_statistic._update_info_timer_id);
		g_pTask_manager->_global_statistic._update_info_timer_id = 0;
	}
}

static _int32 tm_start_target_task(TASK *p_task)
{
    	_int32 ret = SUCCESS;
	if (p_task->_task_type == P2SP_TASK_TYPE)
	{
		ret = pt_start_task(p_task);
		pt_start_query(p_task);  // 强制查hub
#ifdef ENABLE_ETM_MEDIA_CENTER
		 pt_set_origin_mode(p_task, 0); 
#else
        P2SP_TASK * p_p2sp_task = (P2SP_TASK *)p_task;
        if (p_p2sp_task->_is_vod_task)
        {
            pt_set_origin_mode(p_task, 0);                    
        }
#endif

#ifdef EMBED_REPORT
        if(!p_task->_is_continue)
        {
            report_task_create(p_task, INVALID_FILE_INDEX);
        }
#endif

	}
#ifdef ENABLE_BT
	else if (p_task->_task_type == BT_TASK_TYPE)
	{
		ret = bt_start_task(p_task);
		pt_set_origin_mode(p_task, FALSE);
	}
#endif
#ifdef ENABLE_EMULE
	else if (p_task->_task_type == EMULE_TASK_TYPE)
	{	   
		ret = emule_start_task(p_task);
		pt_set_origin_mode(p_task, FALSE);
		#ifdef EMBED_REPORT
		       if(!p_task->_is_continue)
		       {
		           report_task_create(p_task, INVALID_FILE_INDEX);
		       }
		#endif
	}
#endif
    else
	{
#ifdef _DEBUG
        CHECK_VALUE(1);
#endif
		ret = TM_ERR_UNKNOWN;
	}
	
	return ret;
}

static _int32 func_tm_stop_target_task(TASK *p_task)
{
    _int32 ret = SUCCESS;
    if (p_task->_task_type == P2SP_TASK_TYPE)
	{
		ret = pt_stop_task(p_task);
	}
#ifdef ENABLE_BT
	else if (p_task->_task_type == BT_TASK_TYPE)
	{
		ret = bt_stop_task(p_task);
	}
#endif
#ifdef ENABLE_EMULE
	else if (p_task->_task_type == EMULE_TASK_TYPE)
	{
		ret = emule_stop_task(p_task);
	}
#endif
    else
	{
		sd_assert(FALSE);
		ret = TM_ERR_UNKNOWN;
	}
    LOG_DEBUG("p_task=%d, stop_ret=%d", p_task, ret);
	return ret;
}

static _int32 tm_delete_target_task(TASK *p_task)
{
    _int32 ret = SUCCESS;
    do_notice_delete_task(p_task); // 删除任务之前通知其他依赖_p_task的地方释放资源。

	tm_delete_task_info(p_task->_task_id);
#if defined(ENABLE_HSC) && !defined(ENABLE_ETM_MEDIA_CENTER)
	hsc_remove_task_according_to_task_info(p_task);
	tm_delete_task_hsc_info(p_task->_task_id);
#endif
#ifdef ENABLE_LIXIAN
	tm_delete_task_lixian_info(p_task->_task_id);
#endif
    if (p_task->_task_type == P2SP_TASK_TYPE)
	{
		ret = pt_delete_task(p_task);
	}
#ifdef ENABLE_BT
	else if (p_task->_task_type == BT_TASK_TYPE)
	{
		ret = bt_delete_task(p_task);
		tm_delete_bt_file_info_to_pool(p_task->_task_id);
	}
#endif
#ifdef ENABLE_EMULE
	else if (p_task->_task_type == EMULE_TASK_TYPE)
	{
		ret = emule_delete_task(p_task);
	}
#endif
	else
	{
#ifdef _DEBUG
        CHECK_VALUE(1);
#endif
		ret = TM_ERR_UNKNOWN;
	}

 
	return ret;
}

/*--------------------------------------------------------------------------*/
/*                Internal functions				        */
/*--------------------------------------------------------------------------*/
_int32 tm_init(void * _param)
{
	_int32 ret_val = SUCCESS;
	char version_buffer[64] = {0};

#ifdef ENABLE_WB_XML_ZIP
	sd_gz_encode_buffer(NULL, 0, NULL, 0, NULL);
#endif /* ENABLE_WB_XML_ZIP */	

	get_version(version_buffer, sizeof(version_buffer));
#ifdef _DEBUG
	LOG_URGENT("tm_init:version=%s",version_buffer);
#endif

	if (g_pTask_manager != NULL)
	{
	    return TM_ERR_TASK_MANAGER_EXIST;
	}

	g_mini_htttp_num = 0;

    list_init(&g_delete_task_notice_list); //初始化删除任务通知列表

	ret_val = tm_init_post_msg();
	CHECK_VALUE(ret_val);
	
	/* 下载库全局基础模块的初始化 */
	ret_val = tm_basic_init();
	CHECK_VALUE(ret_val);

	/* 下载库功能模块的初始化 */
	ret_val=tm_sub_module_init();
	if(ret_val!=SUCCESS) goto ErrorHanle_sub;
	
	/* 下载库独立模块的初始化 */
	ret_val=tm_other_module_init();
	if(ret_val!=SUCCESS) goto ErrorHanle_other;
	
	/* task_manager 模块的初始化 */
	ret_val=init_task_manager(_param );
	if(ret_val!=SUCCESS) goto ErrorHanle_tm;
	
	LOG_DEBUG("tm_init SUCCESS ");

	tm_start_check_network();

	g_tm_thread_pid = sd_get_self_taskid();

	return SUCCESS;
	
ErrorHanle_tm:
	tm_other_module_uninit();
ErrorHanle_other:
	tm_sub_module_uninit();
ErrorHanle_sub:
	tm_basic_uninit();
	
	LOG_DEBUG("tm_init ERROR:%d ",ret_val);
	return ret_val;
}

_int32 tm_uninit(void* _param)
{
	_int32 ret_val = SUCCESS;

	LOG_DEBUG("tm_uninit...");

	g_tm_thread_pid = 0;
	
	tm_stop_check_network();
	
	ret_val=uninit_task_manager();
	CHECK_VALUE(ret_val);
	
	tm_other_module_uninit();

	tm_sub_module_uninit();

	tm_basic_uninit();

	tm_uninit_post_msg();

    list_clear(&g_delete_task_notice_list);
	
	LOG_URGENT("tm_uninit SUCCESS Bye-bye!");

	return SUCCESS;
}


/* 下载库全局基础模块的初始化 */
_int32 tm_basic_init( void  )
{
	_int32 ret_val = SUCCESS;
	LOG_DEBUG("tm_basic_init...");

	bytebuffer_init();

	 /* Get tht settings items from the config file */
	ret_val = settings_initialize();
	CHECK_VALUE(ret_val);
  	
	ret_val = init_range_module();
	if(ret_val!=SUCCESS) goto ErrorHanle_range;

       ret_val = init_dispatcher_module();
	if(ret_val!=SUCCESS) goto ErrorHanle_dispatcher;
	
	ret_val = init_connect_manager_module();
	if(ret_val!=SUCCESS) goto ErrorHanle_cm;
	
	ret_val = init_data_manager_module();
	if(ret_val!=SUCCESS) goto ErrorHanle_dm;

	ret_val = init_file_manager_module();
	if(ret_val!=SUCCESS) goto ErrorHanle_fm;

	ret_val = init_socket_proxy_module();
	if(ret_val!=SUCCESS) goto ErrorHanle_socket;
	
	LOG_DEBUG("tm_basic_init SUCCESS ");
	return SUCCESS;
	
ErrorHanle_socket:	
	uninit_file_manager_module();
ErrorHanle_fm:
	uninit_data_manager_module();
ErrorHanle_dm:
	uninit_connect_manager_module();
ErrorHanle_cm:
	uninit_dispatcher_module();
ErrorHanle_dispatcher:
	uninit_range_module();
ErrorHanle_range:
	settings_uninitialize( );
	
	LOG_DEBUG("tm_basic_init ERROR:%d ",ret_val);
	return ret_val;
}
_int32 tm_basic_uninit(void)
{
	LOG_DEBUG("tm_basic_uninit...");

	uninit_socket_proxy_module();

	 uninit_file_manager_module();
	
	 uninit_data_manager_module();
	
	uninit_connect_manager_module();
	
	uninit_dispatcher_module();

	uninit_range_module();

	settings_uninitialize( );
	
	LOG_DEBUG("tm_basic_uninit SUCCESS ");

	return SUCCESS;
}

/* 下载库功能模块的初始化 */
_int32 tm_sub_module_init( void  )
{
    _int32 ret_val = SUCCESS;
    _int32 index = 0;

	LOG_DEBUG("tm_sub_module_init...");

    //NODE:jieouy个人觉得这里初始化p2p模块，有待商榷。有可能p2p功能本身就被禁止使用。
	ret_val = init_p2p_module();
	 LOG_DEBUG("%d tm_sub_module_init init return %d\n", index++, ret_val);
	CHECK_VALUE(ret_val);
	
	ret_val = init_res_query_module();
	 LOG_DEBUG("%d tm_sub_module_init init return %d\n", index++, ret_val);
	if(ret_val!=SUCCESS) goto ErrorHanle_res_query;

 	ret_val = init_ptl_modular();
	 LOG_DEBUG("%d tm_sub_module_init init return %d\n", index++, ret_val);
	if(ret_val!=SUCCESS) goto ErrorHanle_ptl;
	
	ret_val =  init_http_pipe_module();
	 LOG_DEBUG("%d tm_sub_module_init init return %d\n", index++, ret_val);
	if(ret_val!=SUCCESS) goto ErrorHanle_http;

 	ret_val =  init_ftp_pipe_module( );
	 LOG_DEBUG("%d tm_sub_module_init init return %d\n", index++, ret_val);
	if(ret_val!=SUCCESS) goto ErrorHanle_ftp;

#ifdef EMBED_REPORT
 	ret_val =  init_reporter_module( );
	 LOG_DEBUG("%d tm_sub_module_init init return %d\n", index++, ret_val);
	if(ret_val!=SUCCESS) goto ErrorHanle_reporter;
#endif
	
	ret_val = init_p2sp_task_module();
	 LOG_DEBUG("%d tm_sub_module_init init return %d\n", index++, ret_val);
	if(ret_val!=SUCCESS) goto ErrorHanle_p2sp;
	
#ifdef ENABLE_BT
	ret_val = init_bt_download_module();
	 LOG_DEBUG("%d tm_sub_module_init init return %d\n", index++, ret_val);
	if(ret_val!=SUCCESS) goto ErrorHanle_bt;
#endif /* ENABLE_BT */

#ifdef ENABLE_EMULE
	ret_val = emule_init_modular();
	 LOG_DEBUG("%d tm_sub_module_init init return %d\n", index++, ret_val);
	if(ret_val!=SUCCESS) goto ErrorHanle_emule;
#endif

	ret_val = init_mini_http();
	 LOG_DEBUG("%d tm_sub_module_init init return %d\n", index++, ret_val);
	if(ret_val!=SUCCESS) goto ErrorHanle_mini_http;

	LOG_DEBUG("tm_sub_module_init SUCCESS ");
	return SUCCESS;
	
ErrorHanle_mini_http:
#ifdef ENABLE_EMULE
	emule_uninit_modular();
ErrorHanle_emule:
#endif /* ENABLE_EMULE */
#ifdef ENABLE_BT
	uninit_bt_download_module();
ErrorHanle_bt:
#endif /* ENABLE_BT */
	uninit_p2sp_task_module();
ErrorHanle_p2sp:
	#ifdef EMBED_REPORT
	uninit_reporter_module();
	#endif
ErrorHanle_reporter:
	uninit_ftp_pipe_module( );
	
ErrorHanle_ftp:
	uninit_http_pipe_module();
ErrorHanle_http:
	uninit_ptl_modular();
ErrorHanle_ptl:
	uninit_res_query_module();
ErrorHanle_res_query:
	uninit_p2p_module();
	
	LOG_DEBUG("tm_sub_module_init ERROR:%d ",ret_val);
	return ret_val;
}
_int32 tm_sub_module_uninit(void)
{
	LOG_DEBUG("tm_sub_module_uninit...");

	uninit_mini_http();
#ifdef ENABLE_BT
	uninit_bt_download_module();
#endif /* #ifdef ENABLE_BT */	
#ifdef ENABLE_EMULE
	emule_uninit_modular();
#endif
	uninit_p2sp_task_module();

#ifdef EMBED_REPORT
	uninit_reporter_module();
#endif

	uninit_ftp_pipe_module( );

	uninit_http_pipe_module();

	uninit_ptl_modular();

	uninit_res_query_module();

	uninit_p2p_module();
	
	LOG_DEBUG("tm_sub_module_uninit SUCCESS ");

	return SUCCESS;
}

/* 下载库独立模块的初始化 */
_int32 tm_other_module_init( void  )
{
	_int32 ret_val = SUCCESS;

	LOG_DEBUG("tm_other_module_init...");
	
#ifdef UPLOAD_ENABLE
 	ret_val =  init_upload_manager( );
	CHECK_VALUE(ret_val);
#endif

#ifdef AUTO_LAN
	ret_val =  init_al_modular();
	if(ret_val!=SUCCESS) goto ErrorHanle_al;
#endif

#ifdef ENABLE_VOD
	ret_val = init_vod_data_manager_module();
	if(ret_val!=SUCCESS) goto ErrorHanle_vod;
#endif

    ret_val = init_cmd_proxy_module();
	if(ret_val!=SUCCESS) goto ErrorHanle_cmd_proxy;

#ifdef ENABLE_DRM 
    ret_val = drm_init_module();
	if(ret_val!=SUCCESS) goto ErrorHanle_drm;
#endif

#if defined(ENABLE_HSC) && !defined(ENABLE_ETM_MEDIA_CENTER) 
		ret_val = hsc_pq_init_module();
#endif


	LOG_DEBUG("tm_other_module_initinit SUCCESS ");
	return SUCCESS;
    
#ifdef ENABLE_DRM 
ErrorHanle_drm:
    uninit_cmd_proxy_module();
#endif

ErrorHanle_cmd_proxy:

#ifdef ENABLE_VOD
    uninit_vod_data_manager_module();
ErrorHanle_vod:
#endif

#ifdef AUTO_LAN
	uninit_al_modular();
#endif

#ifdef AUTO_LAN
ErrorHanle_al:
#endif

#ifdef UPLOAD_ENABLE
 	uninit_upload_manager( );
#endif
	
	LOG_DEBUG("tm_other_module_init ERROR:%d ",ret_val);
	return ret_val;
}
_int32 tm_other_module_uninit(void)
{
	LOG_DEBUG("tm_other_module_uninit...");

#ifdef ENABLE_DRM 
    drm_uninit_module();
#endif

    uninit_cmd_proxy_module();

#ifdef ENABLE_VOD
	uninit_vod_data_manager_module();
#endif

#ifdef AUTO_LAN
	uninit_al_modular();
#endif

#ifdef UPLOAD_ENABLE
 	uninit_upload_manager( );
#endif
	
	LOG_DEBUG("tm_other_module_uninit SUCCESS ");

	return SUCCESS;
}

/* task_manager 模块的初始化 */
_int32 init_task_manager( void * _param )
{
	PROXY_INFO* proxy_for_hub = (PROXY_INFO*)_param;
	_int32 ret_val = SUCCESS;

	LOG_DEBUG("init_task_manager... ");

	ret_val = sd_malloc(sizeof(TASK_MANAGER) ,(void**)&g_pTask_manager);
	CHECK_VALUE(ret_val);
	
	sd_memset(g_pTask_manager,0,sizeof(TASK_MANAGER));

	g_pTask_manager->max_download_filesize = MAX_FILE_SIZE;
	g_pTask_manager->min_download_filesize =  MIN_FILE_SIZE;
	g_pTask_manager->max_tasks =  MAX_NUM_TASKS;
	g_pTask_manager->max_query_shub_retry_count =  MAX_QUERY_SHUB_RETRY_TIMES;
	g_pTask_manager->license_report_interval =  DEFAULT_LICENSE_REPORT_INTERVAL;
	g_pTask_manager->license_expire_time =  DEFAULT_LICENSE_EXPIRE_TIME;
	g_pTask_manager->license_callback_ptr =NULL;
	tm_reset_global_task_manager_sevent_handle();

	settings_get_int_item( "system.max_download_filesize",(_int32*)&(g_pTask_manager->max_download_filesize));
	settings_get_int_item( "system.min_download_filesize",(_int32*)&(g_pTask_manager->min_download_filesize));
	settings_get_int_item( "system.max_tasks",(_int32*)&(g_pTask_manager->max_tasks));
	
	if(g_pTask_manager->max_tasks>MAX_ET_ALOW_TASKS)
		g_pTask_manager->max_tasks = MAX_ET_ALOW_TASKS;
	
	settings_get_int_item( "res_query.max_query_shub_retry_count",(_int32*)&(g_pTask_manager->max_query_shub_retry_count));

	settings_get_int_item( "license.report_interval",(_int32*)&(g_pTask_manager->license_report_interval));
	settings_get_int_item( "license.expire_time",(_int32*)&(g_pTask_manager->license_expire_time));


	if(proxy_for_hub != NULL)
	{
		settings_set_int_item("proxy.type",proxy_for_hub->proxy_type);
		settings_set_str_item("proxy.host",proxy_for_hub->server);
		settings_set_int_item("proxy.port",proxy_for_hub->port);
		settings_set_str_item("proxy.user",proxy_for_hub->user);
		settings_set_str_item("proxy.password",proxy_for_hub->password);
	}

	ret_val = sd_time(&(g_pTask_manager->_global_statistic._sys_start_time));
	if(ret_val!=SUCCESS) goto ErrorHanle_start_time;
	
	g_pTask_manager->_global_statistic._cur_downloadtask_num = 0;
	g_pTask_manager->_global_statistic._cur_download_speed = 0;
	g_pTask_manager->_global_statistic._cur_pipe_num = 0;
	g_pTask_manager->_global_statistic._cur_mem_used = 0;
	g_pTask_manager->_global_statistic._last_update_time = g_pTask_manager->_global_statistic._sys_start_time;
	//_result = settings_set_int_item("system._last_update_time",g_pTask_manager->_global_statistic._last_update_time);
	//CHECK_VALUE(_result);

	list_init(&(g_pTask_manager->_task_list));

	init_default_rw_sharebrd(g_pTask_manager);

#ifdef ENABLE_HSC
	ret_val = tm_init_task_hsc_info();
	if (ret_val != SUCCESS) goto ErrorHanle_hsc_info;
#endif

#ifdef ENABLE_LIXIAN
	tm_init_task_lixian_info();
#endif

#ifdef ENABLE_BT
	ret_val =   tm_init_bt_file_info_pool();
	if(ret_val!=SUCCESS) goto ErrorHanle_bt_pool;
#endif

#if defined(MOBILE_PHONE)

#ifdef  ENABLE_LICENSE
	/* Start the license_report_timer */
	ret_val = start_timer(tm_handle_license_report_timeout, NOTICE_ONCE, FIRST_LICENSE_REPORT_INTERVAL *1000, 0,(void*)g_pTask_manager,&(g_pTask_manager->_license_report_timer_id));  
#endif // ENABLE_LICENCE
#endif /*  MOBILE_PHONE */

	if(ret_val!=SUCCESS) goto ErrorHanle_timer;
	LOG_DEBUG("init_task_manager SUCCESS ");
	return SUCCESS;
	
ErrorHanle_timer:

#ifdef ENABLE_BT
	tm_uninit_bt_file_info_pool();
ErrorHanle_bt_pool:
#endif	/* ENABLE_BT */

#ifdef ENABLE_HSC
	tm_uninit_task_hsc_info();
ErrorHanle_hsc_info:
#endif
#ifdef ENABLE_LIXIAN
	tm_uninit_task_lixian_info();
#endif

	uninit_default_rw_sharebrd();
	
ErrorHanle_start_time:
	SAFE_DELETE(g_pTask_manager);

	LOG_DEBUG("init_task_manager ERROR:%d ",ret_val);
	return ret_val;
}

_int32 uninit_task_manager(void)
{
	LOG_DEBUG("uninit_task_manager...");

	if(g_pTask_manager==NULL)
	{
		return -1;
	}
	
	if(g_pTask_manager->_global_statistic._cur_downloadtask_num != 0)
		return TM_ERR_TASK_IS_RUNNING;
	
	sd_assert(0==list_size(&(g_pTask_manager->_task_list)));
	
	tm_stop_update_info_timer();
	
#if !defined( MOBILE_PHONE)
	if(0 != g_pTask_manager->_license_report_timer_id)
	{
		cancel_timer(g_pTask_manager->_license_report_timer_id);
		g_pTask_manager->_license_report_timer_id=0;
	}
#endif /* MOBILE_PHONE */	
#ifdef ENABLE_LIXIAN
	tm_uninit_task_lixian_info();
#endif
#ifdef ENABLE_HSC
	tm_uninit_task_hsc_info();
#endif
#ifdef ENABLE_BT
	tm_uninit_bt_file_info_pool();
#endif	/* ENABLE_BT */

	uninit_default_rw_sharebrd();
	
	SAFE_DELETE(g_pTask_manager);
	
	LOG_DEBUG("uninit_task_manager SUCCESS ");

	return SUCCESS;
}

_int32 tm_force_close_resource(void)
{
    force_close_res_query_module_res();
	
#ifdef EMBED_REPORT
    force_close_report_module_res();
#endif

    force_close_ptl_modular_res();
#ifdef ENABLE_VOD
    force_close_http_server_module_res();
#endif
    return SUCCESS;
}

//返回值: 0  success
//value的单位:kbytes/second，为0 表示不限速
_int32 tm_set_speed_limit(void * _param)
{
	_int32 ret = SUCCESS;
	TM_SET_SPEED* _p_param = (TM_SET_SPEED*)_param;
	
	LOG_DEBUG("tm_set_speed_limit");

	if (NULL != g_pTask_manager)
	{
	    BOOL is_speed_limit = TRUE;
	    settings_get_bool_item("system.enable_limit_speed", &is_speed_limit);
	    if (is_speed_limit)
	    {
	        socket_proxy_set_speed_limit(_p_param->_download_limit_speed, _p_param->_upload_limit_speed);
#ifdef AUTO_LAN	
            al_set_speed_limit(_p_param->_download_limit_speed, _p_param->_upload_limit_speed);
#endif
            ret = settings_set_int_item("system.download_limit_speed",_p_param->_download_limit_speed);
            sd_assert(ret == SUCCESS);
            ret =settings_set_int_item("system.upload_limit_speed", _p_param->_upload_limit_speed);
            sd_assert(ret == SUCCESS);
            _p_param->_result = ret;
	    }
	}

    return signal_sevent_handle(&(_p_param->_handle));

}

//返回值: 0  success
//value的单位:kbytes/second，为0 表示不限速
_int32 tm_get_speed_limit( void * _param )
{
	TM_GET_SPEED* _p_param = (TM_GET_SPEED*)_param;
	
	LOG_DEBUG("tm_get_speed_limit");
	
	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	sl_get_speed_limit(_p_param->_download_limit_speed, _p_param->_upload_limit_speed);
	_p_param->_result = SUCCESS;
	
	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
	
}

//配置任务连接数
_int32 tm_get_task_connection_limit( void * _param )
{
	TM_GET_CONNECTION* _p_param = (TM_GET_CONNECTION*)_param;

	LOG_DEBUG("tm_get_task_connection_limit");
	
	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}

	*_p_param->task_connection = cm_get_task_max_pipe();
	_p_param->_result = SUCCESS;
	
	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}


_int32 tm_set_task_connection_limit( void * _param )
{
	TM_SET_CONNECTION* _p_param = (TM_SET_CONNECTION*)_param;

	LOG_DEBUG("tm_set_task_connection_limit");
	
	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	cm_set_task_max_pipe( _p_param->task_connection );

	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

#ifdef UPLOAD_ENABLE
_int32 tm_set_download_record_file_path( void * _param )
{
	TM_SET_RC_PATH* _p_param = (TM_SET_RC_PATH*)_param;

	LOG_DEBUG("tm_set_download_record_file_path");

	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	//_p_param->_result = ulm_set_record_list_file_path( _p_param->path,_p_param->path_len);
	// 禁止使用该接口
	_p_param->_result = SUCCESS;
	
#ifdef MACOS
	if(_p_param->_result == SUCCESS)
	{
		dht_set_cfg_path(_p_param->path);
		kad_set_cfg_path(_p_param->path);
	}
#endif /* MACOS */
	
	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

static _int32 func_tm_close_upload_pipes(void *event_handle)
{
    LOG_DEBUG("func_tm_close_upload_pipes");
    if (NULL == g_pTask_manager)
    {
        return -1;
    }
    return ulm_close_upload_pipes(event_handle);
}

_int32 tm_close_upload_pipes(void * _param)
{
	TM_CLOSE_PIPES* _p_param = (TM_CLOSE_PIPES*)_param;
	_p_param->_result = func_tm_close_upload_pipes(&(_p_param->_handle));
	
	if (SUCCESS == _p_param->_result)
	{
		return signal_sevent_handle(&(_p_param->_handle));
	}
	return SUCCESS;
}

_int32 tm_close_upload_pipes_notify(void *event_handle)
{
	SEVENT_HANDLE *p_sevent_handle = (SEVENT_HANDLE *)event_handle;

	LOG_DEBUG("tm_close_upload_pipes_notify,signal_sevent_handle");

	sd_assert(p_sevent_handle!=NULL);
	
	return signal_sevent_handle(p_sevent_handle);
}
#endif

_int32 tm_set_task_num( void * _param )
{
	TM_SET_TASK_NUM* _p_param = (TM_SET_TASK_NUM*)_param;

	LOG_DEBUG("tm_set_task_num");

	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}

	g_pTask_manager->max_tasks = _p_param->max_tasks;
	_p_param->_result = settings_set_int_item("system.max_tasks",_p_param->max_tasks);
	
	//g_pTask_manager->max_running_tasks = _p_param->max_running_tasks;
	//_p_param->_result = settings_set_int_item(STR_SYSTEM_MAX_RUNNING_TASKS, _p_param->max_running_tasks);
	
	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}



_int32 tm_get_task_num( void * _param )
{
	TM_GET_TASK_NUM* _p_param = (TM_GET_TASK_NUM*)_param;

	LOG_DEBUG("tm_get_task_num");
	
	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	*(_p_param->max_tasks)=g_pTask_manager->max_tasks;
	
	//*(_p_param->max_running_tasks)=g_pTask_manager->max_running_tasks;
	
	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}



_int32 tm_create_task_precheck(SEVENT_HANDLE *p_sevent_handle)
{
	if (NULL == g_pTask_manager)
	{
		return -1;
	}
	
    _int32 ret = SUCCESS;	
	LOG_DEBUG("_cur_downloadtask_num=%d, max_tasks=%d", g_pTask_manager->_global_statistic._cur_downloadtask_num, g_pTask_manager->max_tasks);
	if (g_pTask_manager->_global_statistic._cur_downloadtask_num >=g_pTask_manager->max_tasks)
	{
#if defined(_DEBUG ) && defined(MACOS) 
		printf("\n !!!tm_create_task_precheck:downloadtask_num(%u)>=max_tasks(%u)\n",g_pTask_manager->_global_statistic._cur_downloadtask_num,g_pTask_manager->max_tasks);
#endif
		return TM_ERR_TASK_FULL;
	}

	ret = tm_set_global_task_manager_sevent_handle(p_sevent_handle);
	return ret;
}

_int32 tm_create_task_tag(TASK *p_task)
{
    tm_reset_global_task_manager_sevent_handle();
	g_pTask_manager->_global_statistic._total_task_num++;
	LOG_DEBUG("_cur_downloadtask_num=%d", g_pTask_manager->_global_statistic._cur_downloadtask_num);
	g_pTask_manager->_global_statistic._cur_downloadtask_num++;

	if (0 == g_pTask_manager->_global_statistic._total_task_num)
	{
	    g_pTask_manager->_global_statistic._total_task_num++;
	}
	
	p_task->_task_id = g_pTask_manager->_global_statistic._total_task_num;
	
	return list_push(&(g_pTask_manager->_task_list), p_task);
}

_int32 tm_create_new_task_by_url(void *_param)
{
	TM_NEW_TASK_URL* _p_param = (TM_NEW_TASK_URL*)_param;
	_int32 _result = SUCCESS;
	TASK * _p_task = NULL;

	LOG_DEBUG("tm_create_new_task_by_url filename:%s", _p_param->file_name!=NULL?_p_param->file_name:"");

	_p_param->_result =tm_create_task_precheck( &(_p_param->_handle));
	if(_p_param->_result!=SUCCESS)
	{
		LOG_DEBUG("signal_sevent_handle:pre:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	_p_param->_result = pt_create_new_task_by_url( _p_param,&_p_task );

	if(_p_param->_result !=SUCCESS) 
	{
		if(_p_task != NULL)
		{
			/* Return but do not release the signal sevent until the tm_signal_sevent_handle is called */
			LOG_DEBUG("Return but do not release the signal sevent:_result=%d",_p_param->_result);
			return SUCCESS;
		}
		else
		{
			tm_reset_global_task_manager_sevent_handle();
			goto ErrorHanle;
		}
	}

	
	_result = tm_create_task_tag(_p_task);
	if(_result !=SUCCESS) goto ErrorHanle;

	* (_p_param->task_id) = _p_task->_task_id;

	
ErrorHanle:
	if(_result !=SUCCESS)
	{
		g_pTask_manager->_global_statistic._cur_downloadtask_num--;
		LOG_DEBUG("_cur_downloadtask_num=%d", g_pTask_manager->_global_statistic._cur_downloadtask_num);
		_p_param->_result = _result;
	}
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));

}



_int32 tm_create_continue_task_by_url( void * _param )
{
	TM_CON_TASK_URL* _p_param = (TM_CON_TASK_URL*)_param;
	_int32 _result = SUCCESS;
	TASK* _p_task = NULL;

  
	LOG_DEBUG("tm_create_continue_task_by_url");
	
	_p_param->_result =tm_create_task_precheck( &(_p_param->_handle));
	if(_p_param->_result!=SUCCESS)
	{
		LOG_DEBUG("signal_sevent_handle:pre:_result=%d", _p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	
	_p_param->_result = pt_create_continue_task_by_url(_p_param, &_p_task);

	if (_p_param->_result !=SUCCESS) 
	{
		if (_p_task != NULL)
		{
			/* Return but do not release the signal sevent until the tm_signal_sevent_handle is called */
			LOG_DEBUG("Return but do not release the signal sevent:_result=%d",_p_param->_result);
			return SUCCESS;
		}
		else
		{
			tm_reset_global_task_manager_sevent_handle();
			goto ErrorHanle;
		}
	}

	_result = tm_create_task_tag(_p_task);
	if(_result !=SUCCESS) goto ErrorHanle;

	* (_p_param->task_id) = _p_task->_task_id;

	
ErrorHanle:
	if(_result !=SUCCESS)
	{
		g_pTask_manager->_global_statistic._cur_downloadtask_num--;
		LOG_DEBUG("_cur_downloadtask_num=%d", g_pTask_manager->_global_statistic._cur_downloadtask_num);
		_p_param->_result = _result;
	}
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
}

	//新建任务-用tcid下
_int32 tm_create_new_task_by_tcid( void * _param )
{
	TM_NEW_TASK_CID* _p_param = (TM_NEW_TASK_CID*)_param;
	_int32 _result = SUCCESS;
	TASK *_p_task = NULL;

	LOG_DEBUG("tm_create_new_task_by_tcid");
	
	_p_param->_result =tm_create_task_precheck( &(_p_param->_handle));
	if(_p_param->_result!=SUCCESS)
	{
		LOG_DEBUG("signal_sevent_handle:pre:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	
	_p_param->_result = pt_create_new_task_by_tcid( _p_param,&_p_task );

	if(_p_param->_result !=SUCCESS) 
	{
		if(_p_task != NULL)
		{
			/* Return but do not release the signal sevent until the tm_signal_sevent_handle is called */
			LOG_DEBUG("Return but do not release the signal sevent:_result=%d",_p_param->_result);
			return SUCCESS;
		}
		else
		{
			tm_reset_global_task_manager_sevent_handle();
			goto ErrorHanle;
		}
	}

	_result = tm_create_task_tag(_p_task);
	if(_result !=SUCCESS) goto ErrorHanle;

	* (_p_param->task_id) = _p_task->_task_id;

	
ErrorHanle:
	if(_result !=SUCCESS)
	{
		g_pTask_manager->_global_statistic._cur_downloadtask_num--;
		LOG_DEBUG("_cur_downloadtask_num=%d", g_pTask_manager->_global_statistic._cur_downloadtask_num);
		_p_param->_result = _result;
	}
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));


}


	//断点续传-用tcid下
_int32 tm_create_continue_task_by_tcid( void * _param )
{
	TM_CON_TASK_CID* _p_param = (TM_CON_TASK_CID*)_param;
	_int32 _result = SUCCESS;
	TASK *_p_task = NULL;

	LOG_DEBUG("tm_create_continue_task_by_tcid");
	

	_p_param->_result =tm_create_task_precheck( &(_p_param->_handle));
	if(_p_param->_result!=SUCCESS)
	{
		LOG_DEBUG("signal_sevent_handle:pre:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	_p_param->_result = pt_create_continue_task_by_tcid( _p_param,&_p_task );

	if(_p_param->_result !=SUCCESS) 
	{
		if(_p_task != NULL)
		{
			/* Return but do not release the signal sevent until the tm_signal_sevent_handle is called */
			LOG_DEBUG("Return but do not release the signal sevent:_result=%d",_p_param->_result);
			return SUCCESS;
		}
		else
		{
			tm_reset_global_task_manager_sevent_handle();
			goto ErrorHanle;
		}
	}

	_result = tm_create_task_tag(_p_task);
	if(_result !=SUCCESS) goto ErrorHanle;


	* (_p_param->task_id) = _p_task->_task_id;

	
ErrorHanle:
	if(_result !=SUCCESS)
	{
		g_pTask_manager->_global_statistic._cur_downloadtask_num--;
		LOG_DEBUG("_cur_downloadtask_num=%d", g_pTask_manager->_global_statistic._cur_downloadtask_num);
		_p_param->_result = _result;
	}
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));


}

_int32 tm_create_task_by_tcid_file_size_gcid( void * _param )
{
	TM_NEW_TASK_GCID* _p_param = (TM_NEW_TASK_GCID*)_param;
	_int32 _result = SUCCESS;
	TASK *_p_task = NULL;

	LOG_DEBUG("tm_create_task_by_tcid_file_size_gcid");
	
	_p_param->_result =tm_create_task_precheck( &(_p_param->_handle));
	if(_p_param->_result!=SUCCESS)
	{
		LOG_DEBUG("signal_sevent_handle:pre:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	_p_param->_result = pt_create_task_by_tcid_file_size_gcid( _p_param,&_p_task );

	if(_p_param->_result !=SUCCESS) 
	{
		if(_p_task != NULL)
		{
			/* Return but do not release the signal sevent until the tm_signal_sevent_handle is called */
			LOG_DEBUG("Return but do not release the signal sevent:_result=%d",_p_param->_result);
			return SUCCESS;
		}
		else
		{
			tm_reset_global_task_manager_sevent_handle();
			goto ErrorHanle;
		}
	}

	_result = tm_create_task_tag(_p_task);
	if(_result !=SUCCESS) goto ErrorHanle;

	* (_p_param->task_id) = _p_task->_task_id;

	
ErrorHanle:
	if(_result !=SUCCESS)
	{
		g_pTask_manager->_global_statistic._cur_downloadtask_num--;
		LOG_DEBUG("_cur_downloadtask_num=%d", g_pTask_manager->_global_statistic._cur_downloadtask_num);
		_p_param->_result = _result;
	}
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));


}

	
#ifdef ENABLE_BT

_int32 tm_create_bt_task( void * _param )
{
	TM_BT_TASK* _p_param = (TM_BT_TASK*)_param;
	_int32 _result = SUCCESS;
	TASK * _p_task = NULL;
	BOOL is_wait_close = FALSE;

	LOG_DEBUG("tm_create_bt_task");
	
	_p_param->_result =tm_create_task_precheck( &(_p_param->_handle));
	if(_p_param->_result!=SUCCESS)
	{
		LOG_DEBUG("signal_sevent_handle:pre:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	_p_param->_result = bt_create_task( &_p_task );
	if(_p_param->_result !=SUCCESS) 
	{
		tm_reset_global_task_manager_sevent_handle();
		goto ErrorHanle;
	}

	_p_param->_result = bt_init_task( _p_param,_p_task, &is_wait_close );
	if(_p_param->_result !=SUCCESS) 
	{
		if(is_wait_close)
		{
			/* Return but do not release the signal sevent until the tm_signal_sevent_handle is called */
			LOG_DEBUG("Return but do not release the signal sevent:_result=%d",_p_param->_result);
			return SUCCESS;
		}
		else
		{
			tm_reset_global_task_manager_sevent_handle();
			goto ErrorHanle;
		}
	}


	_result = tm_create_task_tag(_p_task);
	if(_result !=SUCCESS) goto ErrorHanle;

	_result = tm_add_bt_file_info_to_pool( _p_task);
	if(_result !=SUCCESS) goto ErrorHanle;

	* (_p_param->_task_id) = _p_task->_task_id;

	
ErrorHanle:
	if(_result !=SUCCESS)
	{
		/*  FATAL ERROR !*/
		g_pTask_manager->_global_statistic._cur_downloadtask_num--;
		LOG_DEBUG("_cur_downloadtask_num=%d", g_pTask_manager->_global_statistic._cur_downloadtask_num);
		_p_param->_result = _result;
		sd_assert(FALSE);  
	}
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));

}

_int32 tm_create_bt_magnet_task( void * _param )
{
	TM_BT_MAGNET_TASK* _p_param = (TM_BT_MAGNET_TASK*)_param;
	_int32 _result = SUCCESS;
	TASK * _p_task = NULL;

	LOG_DEBUG("tm_create_bt_task");

	_p_param->_result =tm_create_task_precheck( &(_p_param->_handle));
	if(_p_param->_result!=SUCCESS)
	{
		LOG_DEBUG("signal_sevent_handle:pre:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}

	_p_param->_result = bt_create_task( &_p_task );
	if(_p_param->_result !=SUCCESS) 
	{
		tm_reset_global_task_manager_sevent_handle();
		goto ErrorHanle;
	}

	_p_param->_result = bt_init_magnet_task( _p_param, _p_task );
	if(_p_param->_result !=SUCCESS) 
	{
		tm_reset_global_task_manager_sevent_handle();
		goto ErrorHanle;

	}
	_result = tm_create_task_tag(_p_task);
	if(_result !=SUCCESS) goto ErrorHanle;

	* (_p_param->_task_id) = _p_task->_task_id;
#if defined(ENABLE_HSC) && !defined(ENABLE_ETM_MEDIA_CENTER)
		hsc_init_info(&_p_task->_hsc_info);
#endif

ErrorHanle:
	if(_result !=SUCCESS)
	{
		/*  FATAL ERROR !*/
		g_pTask_manager->_global_statistic._cur_downloadtask_num--;
		LOG_DEBUG("_cur_downloadtask_num=%d", g_pTask_manager->_global_statistic._cur_downloadtask_num);
		_p_param->_result = _result;
		sd_assert(FALSE);  
	}
	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

#endif /* #ifdef ENABLE_BT */

#ifdef ENABLE_EMULE
_int32 tm_create_emule_task(void* param)
{
	TM_EMULE_PARAM* p_param = (TM_EMULE_PARAM*)param;
	_int32 result = SUCCESS;
	TASK* p_task = NULL;
	LOG_DEBUG("tm_create_emule_task");
	p_param->_result =tm_create_task_precheck( &(p_param->_handle));
	if(p_param->_result != SUCCESS)
	{
		return signal_sevent_handle(&(p_param->_handle));
	}	
	p_param->_result = emule_create_task(p_param,&p_task );
	if(p_param->_result != SUCCESS) 
	{
		if(p_task != NULL)
		{
			/* Return but do not release the signal sevent until the tm_signal_sevent_handle is called */
			LOG_DEBUG("Return but do not release the signal sevent:_result=%d",p_param->_result);
			return SUCCESS;
		}
		else
		{
			tm_reset_global_task_manager_sevent_handle();
			goto ErrorHanle;
		}
	}
	result = tm_create_task_tag(p_task);
	if(result !=SUCCESS) goto ErrorHanle;
	*(p_param->_task_id) = p_task->_task_id;

	
ErrorHanle:
	if(result !=SUCCESS)
	{
		/*  FATAL ERROR !*/
		g_pTask_manager->_global_statistic._cur_downloadtask_num--;
		LOG_DEBUG("_cur_downloadtask_num=%d", g_pTask_manager->_global_statistic._cur_downloadtask_num);
		p_param->_result = result;
		sd_assert(FALSE);  
	}
		LOG_DEBUG("signal_sevent_handle:_result=%d",p_param->_result);
		return signal_sevent_handle(&(p_param->_handle));
}
#endif

#ifdef _VOD_NO_DISK   
_int32 tm_set_task_no_disk( void * _param )
{
	TM_START_TASK* _p_param = (TM_START_TASK*)_param;
	TASK * _p_task = NULL;
	P2SP_TASK * p_p2sp_task = NULL;

	LOG_DEBUG("tm_set_task_no_disk:%d",_p_param->task_id);

	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	_p_param->_result = SUCCESS;

	_p_param->_result = tm_get_task_by_id(_p_param->task_id,&_p_task);
	if(_p_param->_result!=SUCCESS) goto ErrorHanle;
	
	LOG_DEBUG("tm_set_task_no_disk,task_id=%d,_task_type=%d,task_status=%d",_p_param->task_id,_p_task->_task_type,_p_task->task_status);

	if(_p_task->task_status != TASK_S_IDLE)
	{
		_p_param->_result =TM_ERR_TASK_IS_NOT_READY;
		goto ErrorHanle;
	}

	if(_p_task->_task_type==P2SP_TASK_TYPE)
	{
		p_p2sp_task = (P2SP_TASK *)_p_task;
		
		if((p_p2sp_task->_task_created_type==TASK_CONTINUE_BY_URL)||
			((p_p2sp_task->_task_created_type==TASK_CONTINUE_BY_TCID)&&(p_p2sp_task->_is_vod_task==FALSE)))
		{
			_p_param->_result =TM_ERR_TASK_TYPE;
			goto ErrorHanle;
		}
		
		_p_param->_result = dm_vod_set_no_disk_mode( &p_p2sp_task->_data_manager);
	}
	else
	{
		_p_param->_result = TM_ERR_TASK_TYPE;
	}

	
ErrorHanle:

		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));

}
#endif


_int32 tm_set_task_check_data( void * _param )
{
	TM_START_TASK* _p_param = (TM_START_TASK*)_param;
	TASK * _p_task = NULL;
	P2SP_TASK * p_p2sp_task = NULL;

	LOG_DEBUG("tm_set_task_check_data:%d",_p_param->task_id);

	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	_p_param->_result = SUCCESS;

	_p_param->_result = tm_get_task_by_id(_p_param->task_id,&_p_task);
	if(_p_param->_result!=SUCCESS) goto ErrorHanle;
	
	LOG_DEBUG("tm_set_task_check_data,task_id=%d,_task_type=%d,task_status=%d",_p_param->task_id,_p_task->_task_type,_p_task->task_status);

	if(_p_task->task_status != TASK_S_IDLE)
	{
		_p_param->_result =TM_ERR_TASK_IS_NOT_READY;
		goto ErrorHanle;
	}

	if(_p_task->_task_type==P2SP_TASK_TYPE)
	{
		p_p2sp_task = (P2SP_TASK *)_p_task;
		
		if((p_p2sp_task->_task_created_type==TASK_CONTINUE_BY_URL)||
			((p_p2sp_task->_task_created_type==TASK_CONTINUE_BY_TCID)&&(p_p2sp_task->_is_vod_task==FALSE)))
		{
			_p_param->_result = TM_ERR_TASK_TYPE;
		      goto ErrorHanle;
		}
		
		_p_param->_result = dm_vod_set_check_data_mode( &p_p2sp_task->_data_manager);
	}
	else
	{
		_p_param->_result = TM_ERR_TASK_TYPE;
	}

	
ErrorHanle:

		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));

}

_int32 tm_set_task_write_mode( void * _param )
{
	TM_POST_PARA_2* _p_param = (TM_POST_PARA_2*)_param;
	_u32 task_id = (_u32)_p_param->_para1;
	_u32 mode = (_u32)_p_param->_para2;
	TASK * _p_task = NULL;
	P2SP_TASK * p_p2sp_task = NULL;

	LOG_DEBUG("tm_set_task_write_mode:%d",task_id);

	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	_p_param->_result = SUCCESS;

	_p_param->_result = tm_get_task_by_id(task_id,&_p_task);
	if(_p_param->_result!=SUCCESS) goto ErrorHanle;
	
	LOG_DEBUG("tm_set_task_write_mode,task_id=%d,_task_type=%d,task_status=%d,mode=%u",task_id,_p_task->_task_type,_p_task->task_status,mode);
	if(_p_task->_task_type==P2SP_TASK_TYPE)
	{
		p_p2sp_task = (P2SP_TASK *)_p_task;
		
		_p_param->_result = file_info_set_write_mode( &p_p2sp_task->_data_manager._file_info,(FILE_WRITE_MODE )mode);
	}
	else
	{
		_p_param->_result = TM_ERR_TASK_TYPE;
	}
	
	
ErrorHanle:

		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));

}

_int32 tm_get_task_write_mode( void * _param )
{
	TM_POST_PARA_2* _p_param = (TM_POST_PARA_2*)_param;
	_u32 task_id = (_u32)_p_param->_para1;
	_u32 * mode = (_u32*)_p_param->_para2;
	TASK * _p_task = NULL;
	P2SP_TASK * p_p2sp_task = NULL;

	LOG_DEBUG("tm_get_task_write_mode:%d",task_id);

	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	_p_param->_result = SUCCESS;

	_p_param->_result = tm_get_task_by_id(task_id,&_p_task);
	if(_p_param->_result!=SUCCESS) goto ErrorHanle;
	
	LOG_DEBUG("tm_get_task_write_mode,task_id=%d,_task_type=%d,task_status=%d",task_id,_p_task->_task_type,_p_task->task_status);

	if(_p_task->_task_type==P2SP_TASK_TYPE)
	{
		p_p2sp_task = (P2SP_TASK *)_p_task;
		
		_p_param->_result = file_info_get_write_mode( &p_p2sp_task->_data_manager._file_info,(FILE_WRITE_MODE *)mode);
	}
	else
	{
		_p_param->_result = TM_ERR_TASK_TYPE;
	}
	
ErrorHanle:

		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));

}

_int32 tm_start_task(void * _param)
{
	TM_START_TASK* _p_param = (TM_START_TASK*)_param;
	LOG_DEBUG("tm_start_task:%d", _p_param->task_id);

	if (NULL == g_pTask_manager)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d", _p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	TASK *_p_task = NULL;
	_p_param->_result = tm_get_task_by_id(_p_param->task_id, &_p_task);
	if (_p_param->_result != SUCCESS)
	{
	    goto ErrorHanle;
	}

	if (_p_task->task_status != TASK_S_IDLE)
	{
		_p_param->_result = TM_ERR_TASK_IS_NOT_READY;
		goto ErrorHanle;
	}

	_p_param->_result = tm_start_target_task(_p_task);
	if (_p_param->_result != SUCCESS)
	{
	    goto ErrorHanle;
	}
	
	tm_update_task();

	_p_param->_result = tm_start_update_info_timer();
	
#ifdef _DEBUG
    if (_p_param->_result != SUCCESS)
    {
        CHECK_VALUE(1);
    }
#endif
	
ErrorHanle:

	LOG_DEBUG("signal_sevent_handle:_result=%d", _p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

static _int32 func_tm_stop_task(_u32 taskid)
{
    if (NULL == g_pTask_manager)
    {
        return -1;
    }

    TASK *_p_task = NULL;
    _int32 ret_val = tm_get_task_by_id(taskid, &_p_task);
    LOG_DEBUG("taskid(%d), _p_task(%d)", taskid, _p_task);

    if (SUCCESS != ret_val)
    {
        return ret_val;
    }

    if ((TASK_S_IDLE == _p_task->task_status) || (TASK_S_STOPPED == _p_task->task_status))
    {
        return TM_ERR_TASK_NOT_RUNNING;
    }

    if ((TASK_S_RUNNING == _p_task->task_status) || (TASK_S_VOD == _p_task->task_status))
    {
        tm_update_task();
    }

#ifdef ENABLE_VOD
	vdm_vod_stop_task(taskid, TRUE);
#endif

    return func_tm_stop_target_task(_p_task);
}

_int32 tm_stop_task(void * _param)
{
	TM_STOP_TASK* _p_param = (TM_STOP_TASK*)_param;
	LOG_DEBUG("tm_stop_task:%d", _p_param->task_id);
	if (NULL == g_pTask_manager)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d", _p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	TASK *_p_task = NULL;
	_p_param->_result = tm_get_task_by_id(_p_param->task_id, &_p_task);
	if (_p_param->_result != SUCCESS)
	{
	    goto ErrorHanle;
	}

	if ((_p_task->task_status == TASK_S_IDLE) || (_p_task->task_status == TASK_S_STOPPED))
	{
		_p_param->_result = TM_ERR_TASK_NOT_RUNNING;
		goto ErrorHanle;
	}
	
	if ((_p_task->task_status==TASK_S_RUNNING) || (_p_task->task_status==TASK_S_VOD))
	{
	    tm_update_task();
	}    

#ifdef ENABLE_VOD
    vdm_vod_stop_task(_p_param->task_id, TRUE);
#endif

    _p_param->_result = func_tm_stop_target_task(_p_task);
	if (_p_param->_result == TM_ERR_WAIT_FOR_SIGNAL)
	{
	    return SUCCESS;
	}

ErrorHanle:

	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));

}

_int32 tm_delete_task(void * _param)
{
	TM_DEL_TASK* _p_param = (TM_DEL_TASK*)_param;
	if (NULL == g_pTask_manager)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d", _p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	LOG_URGENT("tm_delete_task:%d", _p_param->task_id);

	TASK * _p_task = NULL;
	_int32 _list_size = list_size(&(g_pTask_manager->_task_list));
	if (0 == _list_size)
	{
		_p_param->_result = TM_ERR_NO_TASK;
		goto ErrorHanle;
	}
	
	pLIST_NODE _p_list_node = LIST_BEGIN(g_pTask_manager->_task_list);
	while (_list_size)
	{
		_p_task = (TASK *)LIST_VALUE(_p_list_node);
		if (_p_param->task_id == _p_task->_task_id)
		{
			break;
		}
		
		_p_list_node = LIST_NEXT(_p_list_node);
		_list_size--;
	}
	
	if (_list_size == 0)
	{
		_p_param->_result = TM_ERR_INVALID_TASK_ID;
		goto ErrorHanle;
	}
	
	LOG_DEBUG("tm_delete_task,task_id=%d,_task_type=%d,task_status=%d", 
	    _p_param->task_id, _p_task->_task_type, _p_task->task_status);

	if ((_p_task->task_status != TASK_S_IDLE) && (_p_task->task_status != TASK_S_STOPPED))
	{
		_p_param->_result = TM_ERR_TASK_IS_RUNNING;
		goto ErrorHanle;
	}

    _p_param->_result= tm_set_global_task_manager_sevent_handle(&(_p_param->_handle));
	if (TM_ERR_OPERATION_CLASH== _p_param->_result)
	{
		goto ErrorHanle;
	}

	_u32 task_id = _p_task->_task_id;
	_p_param->_result = tm_delete_target_task(_p_task);

	list_erase(&(g_pTask_manager->_task_list), _p_list_node);
	
	g_pTask_manager->_global_statistic._cur_downloadtask_num --;
	if (g_pTask_manager->_global_statistic._cur_downloadtask_num == 0)
	{
	    tm_stop_update_info_timer();
		TM_RUN_DOWNTASK_HOOKS(0);
	}
	
#ifndef IPAD_KANKAN	
	if (_p_param->_result ==TM_ERR_WAIT_FOR_SIGNAL)
	{
		// Return but do not release the signal sevent until the tm_signal_sevent_handle is called
		LOG_URGENT("tm_delete_task,Return but do not release the signal sevent ");
		_p_param->_result = SUCCESS;
		return SUCCESS;

	}
#endif

	// else, return and release the signal sevent
	tm_reset_global_task_manager_sevent_handle();
	LOG_URGENT("tm_delete_task, Return and release the signal sevent ");
	
ErrorHanle:
    LOG_URGENT("signal_sevent_handle:_result=%d",_p_param->_result);
    return signal_sevent_handle(&(_p_param->_handle));

}

_int32 tm_register_deletet_task_notice(deletet_task_notice_proc callback)
{
    LOG_DEBUG("tm_register_deletet_task_notice callback=0x%x", callback);
    _int32 ret = -1;
    if (callback)
    {
        LIST_ITERATOR iter = NULL;
        BOOL is_need_add = TRUE;
        LOG_DEBUG("tm_register_deletet_task_notice begin list_size=%d", list_size(&g_delete_task_notice_list));
        for (iter = LIST_BEGIN(g_delete_task_notice_list); iter != LIST_END(g_delete_task_notice_list); iter = LIST_NEXT(iter))
        {
            if (((void*)callback) == LIST_VALUE(iter))
            {
                is_need_add = FALSE;
                break;
            }
        }
        if (is_need_add)
        {
            LOG_DEBUG("tm_register_deletet_task_notice add ok");
            list_push(&g_delete_task_notice_list, (void*) callback);
            ret = TRUE;
        }
    }
    else
    {
        sd_assert(0);
    }
    LOG_DEBUG("tm_register_deletet_task_notice ret=%d", ret);
    return ret;
}

_int32 tm_unregister_deletet_task_notice(deletet_task_notice_proc callback)
{
    LOG_DEBUG("tm_unregister_deletet_task_notice callback=0x%x", callback);
    _int32 ret = -1;
    if (callback)
    {
        LIST_ITERATOR iter = NULL;
        LIST_ITERATOR delete_iter = NULL;
        for (iter = LIST_BEGIN(g_delete_task_notice_list); iter != LIST_END(g_delete_task_notice_list); iter = LIST_NEXT(iter))
        {
            if (((void*)callback) == LIST_VALUE(iter))
            {
                delete_iter = iter;
                break;
            }
        }
        if (NULL != delete_iter)
        {
            list_erase(&g_delete_task_notice_list, delete_iter);
            ret = TRUE;
        }
    }
    else
    {
        sd_assert(0);
    }
    LOG_DEBUG("tm_unregister_deletet_task_notice ret=%d", ret);
    return ret;
}

static void do_notice_delete_task(TASK *task_ptr)
{
    LOG_DEBUG("do_notice_delete_task task_ptr=0x%x", task_ptr);
    if (task_ptr)
    {
        LOG_DEBUG("do_notice_delete_task begin list_size=%d", list_size(&g_delete_task_notice_list));

        LIST_ITERATOR iter = NULL;
        for (iter = LIST_BEGIN(g_delete_task_notice_list); iter != LIST_END(g_delete_task_notice_list); iter = LIST_NEXT(iter))
        {
            deletet_task_notice_proc callback =  (deletet_task_notice_proc)LIST_VALUE(iter);
            if (callback)
            {
                LOG_DEBUG("do_notice_delete_task exec callback = 0x%x", callback);
                callback(task_ptr);
            }
            else
            {
                sd_assert(0);
            }
        }
        LOG_DEBUG("do_notice_delete_task end");
    }
}


_int32 tm_get_task_by_id(_u32 task_id, TASK **_pp_task)
{	
    _int32 _list_size = list_size(&(g_pTask_manager->_task_list)); 
	if (0 == _list_size)
	{
        LOG_DEBUG("tm_get_task_by_id:%d, failed. return TM_ERR_NO_TASK", task_id);
		return TM_ERR_NO_TASK;
	}

	TASK *p_temp_task_node = NULL;
	pLIST_NODE _p_list_node = LIST_BEGIN(g_pTask_manager->_task_list);
	while (_list_size)
	{
	    p_temp_task_node = (TASK*)LIST_VALUE(_p_list_node);
		if (task_id == p_temp_task_node->_task_id)
		{
		    *_pp_task = p_temp_task_node;
			return SUCCESS;
		}
		_p_list_node = LIST_NEXT(_p_list_node);
		_list_size--;
	}
    LOG_DEBUG("tm_get_task_by_id failed, return TM_ERR_INVALID_TASK_ID.");
	return TM_ERR_INVALID_TASK_ID;
}

BOOL tm_is_task_exist_by_url( char* url, BOOL include_no_disk)
{
	LIST_ITERATOR p_list_node=NULL ;
	TASK * p_task = NULL;
	
	for(p_list_node = LIST_BEGIN(g_pTask_manager->_task_list); p_list_node != LIST_END(g_pTask_manager->_task_list); p_list_node = LIST_NEXT(p_list_node))
	{
		p_task = (TASK * )LIST_VALUE(p_list_node);
		if(p_task ->_task_type == P2SP_TASK_TYPE) 
		{
            if (pt_is_task_exist_by_url(p_task,url, include_no_disk) == TRUE)
			 {
				 return TRUE;
			 }
		}
	}	
	return FALSE;
}

BOOL tm_is_task_exist_by_cid( const _u8* cid, BOOL include_no_disk)
{
	
	LIST_ITERATOR p_list_node=NULL ;
	TASK * p_task = NULL;
	//LOG_DEBUG("tm_is_task_exist_by_cid");
#ifdef _LOGGER
	_int32 ret_val = SUCCESS;
	 char cid_str[CID_SIZE*2+1]; 

      ret_val = str2hex((char*)cid, CID_SIZE, cid_str, CID_SIZE*2);
	CHECK_VALUE( ret_val );
	cid_str[CID_SIZE*2] = '\0';
	LOG_DEBUG( "tm_is_task_exist_by_cid: cid = %s,", cid_str );
#endif
	
	for(p_list_node = LIST_BEGIN(g_pTask_manager->_task_list); p_list_node != LIST_END(g_pTask_manager->_task_list); p_list_node = LIST_NEXT(p_list_node))
	{
		p_task = (TASK * )LIST_VALUE(p_list_node);
		if(p_task ->_task_type == P2SP_TASK_TYPE) 
		{
            if (pt_is_task_exist_by_cid(p_task,cid, include_no_disk) == TRUE)
			{
		        return TRUE;
			}
		}
	}

	return FALSE;
	
}

#ifdef ENABLE_BT

BOOL tm_is_task_exist_by_info_hash( const _u8* info_hash)
{
	
	LIST_ITERATOR p_list_node=NULL ;
	TASK * p_task = NULL;
	//LOG_DEBUG("tm_is_task_exist_by_info_hash");
#ifdef _LOGGER
	_int32 ret_val = SUCCESS;
	 char cid_str[CID_SIZE*2+1]; 

      ret_val = str2hex((char*)info_hash, CID_SIZE, cid_str, CID_SIZE*2);
	CHECK_VALUE( ret_val );
	cid_str[CID_SIZE*2] = '\0';
	LOG_DEBUG( "tm_is_task_exist_by_info_hash: info_hash = %s,", cid_str );
#endif
	
	for(p_list_node = LIST_BEGIN(g_pTask_manager->_task_list); p_list_node != LIST_END(g_pTask_manager->_task_list); p_list_node = LIST_NEXT(p_list_node))
	{
		p_task = (TASK * )LIST_VALUE(p_list_node);
		if(p_task ->_task_type == BT_TASK_TYPE
				&& !( (BT_TASK*)p_task)->_is_magnet_task) 
		{
			 if(bt_is_task_exist_by_info_hash(p_task,info_hash) == TRUE )
			 {
				 return TRUE;
			 }
		}
	}

	return FALSE;
	
}
#endif /* #ifdef ENABLE_BT */

_int32 tm_init_post_msg(void)
{
    _int32 ret_val = init_post_msg(&p_tm_post_lock);
#ifdef MACOS
    if (ret_val == SUCCESS)
    {
        ret_val = init_simple_event(&g_post_handle);
        if (ret_val != SUCCESS)
        {
            sd_uninit_task_lock(p_tm_post_lock);
            SAFE_DELETE(p_tm_post_lock);
        }
    }
#endif
    return ret_val;
}

_int32 tm_uninit_post_msg(void)
{
#ifdef MACOS
	uninit_simple_event(&g_post_handle);
#endif
    return uninit_post_msg(&p_tm_post_lock);
}

_int32 tm_post_function(thread_msg_handle handler, void *args, SEVENT_HANDLE *_handle, _int32 * _result)
{
    if (sd_is_target_thread(g_tm_thread_pid))
    {
        handler(args);
        return (*_result);
    }
    
	int ret_val = 0;

#define ET_RELEASE_LOCK_CHECK_VALUE(value) {if(value!=SUCCESS) {sd_task_unlock(p_tm_post_lock); CHECK_VALUE(value);} }
	
	sd_task_lock(p_tm_post_lock);
	
#ifdef MACOS
	sd_assert(_handle->_value == 0);
	sd_memcpy(_handle, &g_post_handle, sizeof(SEVENT_HANDLE));
#else
	ret_val = init_simple_event(_handle);
	ET_RELEASE_LOCK_CHECK_VALUE(ret_val);
#endif
	
	ret_val = post_message_from_other_thread(handler, args);
	ET_RELEASE_LOCK_CHECK_VALUE(ret_val);

	ret_val = wait_sevent_handle(_handle);
	ET_RELEASE_LOCK_CHECK_VALUE(ret_val);
	
#ifdef MACOS
	sd_assert(_handle->_value == 0);
	sd_memset(_handle, 0x00, sizeof(SEVENT_HANDLE));
#else
	ret_val = uninit_simple_event(_handle);
	ET_RELEASE_LOCK_CHECK_VALUE(ret_val);
#endif
	
	sd_task_unlock(p_tm_post_lock);

#undef ET_RELEASE_LOCK_CHECK_VALUE
	
	return (*_result);
}

_int32 tm_try_post_function(thread_msg_handle handler, void *args,SEVENT_HANDLE *_handle,_int32 * _result)
{
    if (sd_is_target_thread(g_tm_thread_pid))
    {
        handler(args);
        return (*_result);
    }
    
	int ret_val = sd_task_trylock(p_tm_post_lock);
	if (ret_val != 0) 
	{
		/* 下载库正忙,请稍候再试 */
		LOG_URGENT("tm_try_post_function:ret_val=%d",ret_val);
		*_result = ret_val;
		return ret_val;
	}
	
#ifdef MACOS
	sd_assert(_handle->_value == 0);
	sd_memcpy(_handle, &g_post_handle, sizeof(SEVENT_HANDLE));
#else
	ret_val = init_simple_event(_handle);
	CHECK_VALUE(ret_val);
#endif
	
	ret_val = post_message_from_other_thread(handler, args);
	CHECK_VALUE(ret_val);

	ret_val = wait_sevent_handle(_handle);
	CHECK_VALUE(ret_val);
	
#ifdef MACOS
	sd_assert(_handle->_value == 0);
	sd_memset(_handle, 0x00, sizeof(SEVENT_HANDLE));
#else
	ret_val = uninit_simple_event(_handle);
	CHECK_VALUE(ret_val);
#endif
	
	sd_task_unlock(p_tm_post_lock);	
	return (*_result);
}

/* Release the signal sevent 
	Notice: this function is only called by dt_notify_file_closing_result_event from download_task when deleting a task */
void  tm_signal_sevent_handle(void)
{
	SEVENT_HANDLE * p_sevent_handle = NULL;
	
	LOG_URGENT("tm_signal_sevent_handle(p_sevent_handle=%X)",g_pTask_manager->_p_sevent_handle);
#ifndef IPAD_KANKAN	
	if (g_pTask_manager->_p_sevent_handle != NULL)
	{
		p_sevent_handle = g_pTask_manager->_p_sevent_handle;
		g_pTask_manager->_p_sevent_handle = NULL;
		LOG_DEBUG("signal_sevent_handle(*p_sevent_handle=%d)", *p_sevent_handle);
		signal_sevent_handle(p_sevent_handle);
	}
#endif
}
_int32 tm_post_function_unblock(thread_msg_handle handler, void *args, _int32 * _result)
{
	int ret_val = post_message_from_other_thread(handler, args);
	CHECK_VALUE(ret_val);	
	*_result = ret_val;	
	return ret_val;
}

#ifdef ENABLE_BT
/* Get the torrent seed file information  */
_int32 	tm_get_torrent_seed_info( void * _param)
{
	TM_GET_TORRENT_SEED_INFO* _p_param = (TM_GET_TORRENT_SEED_INFO*)_param;

	LOG_DEBUG("tm_get_torrent_seed_info");
	
	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	_p_param->_result = tp_get_seed_info( _p_param->seed_path,_p_param->encoding_switch_mode, (TORRENT_SEED_INFO**) (_p_param->pp_seed_info));
	
	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}


/* Release the memory which malloc by  tm_get_torrent_seed_info */
_int32 	tm_release_torrent_seed_info( void * _param)
{
	TM_RELEASE_TORRENT_SEED_INFO* _p_param = (TM_RELEASE_TORRENT_SEED_INFO*)_param;

	LOG_DEBUG("tm_release_torrent_seed_info");
	
	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	_p_param->_result = tp_release_seed_info( (TORRENT_SEED_INFO*)(_p_param->p_seed_info));
	
	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 	tm_set_seed_switch_type( void * _param )
{
	TM_SEED_SWITCH_TYPE* _p_param = (TM_SEED_SWITCH_TYPE*)_param;

	LOG_DEBUG("tm_set_seed_switch_type");
	
	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	_p_param->_result = tp_set_default_switch_mode( _p_param->_switch_type);
	if(SUCCESS ==_p_param->_result )
	{
		settings_set_int_item("bt.encoding_switch_mode", _p_param->_switch_type);
	}
	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 	tm_get_bt_download_file_index( void * _param )
{
	TM_GET_BT_FILE_INDEX* _p_param = (TM_GET_BT_FILE_INDEX*)_param;

	TASK* _p_task = NULL;
	
	LOG_DEBUG("tm_get_bt_download_file_index");
	
	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	_p_param->_result = tm_get_task_by_id(_p_param->_task_id,&_p_task);
	if(_p_param->_result == SUCCESS) 
	{
		if(_p_task->_task_type == BT_TASK_TYPE )
		{
		     _p_param->_result = bt_get_download_file_index( _p_task, _p_param->_buffer_len, _p_param->_file_index_buffer );
		}
		else
		{
		    _p_param->_result = TM_ERR_TASK_TYPE;
		}	
	}
	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

#endif /* #ifdef ENABLE_BT */

_int32 tm_get_task_file_name(void * _param)
{
	TM_GET_TASK_FILE_NAME* _p_param = (TM_GET_TASK_FILE_NAME*)_param;
	TASK* _p_task = NULL;
	LOG_DEBUG("tm_get_task_file_name");
	
	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	_p_param->_result = tm_get_task_by_id(_p_param->task_id,&_p_task);
	if(_p_param->_result==SUCCESS) 
	{
		if(_p_task->_task_type==P2SP_TASK_TYPE)
		{
			if(_p_task->task_status!=TASK_S_IDLE)
				_p_param->_result = pt_get_task_file_name(  _p_task ,_p_param->file_name_buffer,_p_param->buffer_size);
			else
			      _p_param->_result = TM_ERR_TASK_NOT_RUNNING;
		}
#ifdef ENABLE_EMULE
		else if(_p_task->_task_type == EMULE_TASK_TYPE)
		{
			_p_param->_result = emule_get_task_file_name(_p_task, _p_param->file_name_buffer, _p_param->buffer_size);
		}
#endif

#ifdef ENABLE_BT
	        else if(_p_task->_task_type == BT_MAGNET_TASK_TYPE || _p_task->_task_type == BT_TASK_TYPE)
        	{
            		_p_param->_result = bt_get_seed_title_name(_p_task, _p_param->file_name_buffer, _p_param->buffer_size);
        	}
#endif
		else
		{
			_p_param->_result = TM_ERR_TASK_TYPE;
		}

	}
	
	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}
_int32 tm_get_bt_task_sub_file_name(void * _param)
{
	TM_GET_TASK_FILE_NAME* _p_param = (TM_GET_TASK_FILE_NAME*)_param;
	TASK* _p_task = NULL;
	LOG_DEBUG("tm_get_bt_task_sub_file_name");
	
	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	_p_param->_result = tm_get_task_by_id(_p_param->task_id,&_p_task);
	if(_p_param->_result==SUCCESS) 
	{
#ifdef ENABLE_BT
		if(_p_task->_task_type==BT_TASK_TYPE ||_p_task->_task_type==BT_MAGNET_TASK_TYPE)
		{
			if(_p_task->task_status!=TASK_S_IDLE)
				_p_param->_result = bt_get_sub_file_name(_p_task,_p_param->file_index, _p_param->file_name_buffer,(_u32*)(_p_param->buffer_size));
			else
			      _p_param->_result = TM_ERR_TASK_NOT_RUNNING;
		}
		else
#endif /* ENABLE_BT */
		{
			_p_param->_result = TM_ERR_TASK_TYPE;
		}

	}
	
	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}
_int32 tm_get_task_tcid(void * _param)
{
	TM_GET_TASK_TCID* _p_param = (TM_GET_TASK_TCID*)_param;
	TASK* _p_task = NULL;
	LOG_DEBUG("tm_get_task_tcid");
	
	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	_p_param->_result = tm_get_task_by_id(_p_param->task_id,&_p_task);
	if(_p_param->_result==SUCCESS) 
	{
		if(_p_task->_task_type==P2SP_TASK_TYPE)
		{
			if(_p_task->task_status!=TASK_S_IDLE)
				_p_param->_result = pt_get_task_tcid(  _p_task ,_p_param->tcid);
			else
			      _p_param->_result = TM_ERR_TASK_NOT_RUNNING;
		}
		#ifdef ENABLE_EMULE
		else
		if(_p_task->_task_type==EMULE_TASK_TYPE)
		{
			if(_p_task->task_status!=TASK_S_IDLE)
			{
				_p_param->_result = emule_get_task_cid(  _p_task ,_p_param->tcid);
			}
			else
			      _p_param->_result = TM_ERR_TASK_NOT_RUNNING;
		}
		#endif /*  ENABLE_EMULE  */
		else
		{
			_p_param->_result = TM_ERR_TASK_TYPE;
		}

	}
	
	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}
_int32 tm_get_bt_task_sub_file_tcid(void * _param)
{
	TM_GET_TASK_TCID* _p_param = (TM_GET_TASK_TCID*)_param;
	TASK* _p_task = NULL;
	LOG_DEBUG("tm_get_bt_task_sub_file_tcid");
	
	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	_p_param->_result = tm_get_task_by_id(_p_param->task_id,&_p_task);
	if(_p_param->_result==SUCCESS) 
	{
#ifdef ENABLE_BT
		if(_p_task->_task_type==BT_TASK_TYPE ||_p_task->_task_type==BT_MAGNET_TASK_TYPE)
		{
			if(_p_task->task_status!=TASK_S_IDLE)
				_p_param->_result = bt_get_sub_file_tcid( _p_task,_p_param->file_index,_p_param->tcid);
			else
			      _p_param->_result = TM_ERR_TASK_NOT_RUNNING;
		}
		else
#endif /* ENABLE_BT */
		{
			_p_param->_result = TM_ERR_TASK_TYPE;
		}

	}
	
	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

//jjxh added....
_int32 tm_get_bt_task_sub_file_gcid(void * _param)
{
	TM_GET_TASK_TCID* _p_param = (TM_GET_TASK_TCID*)_param;
	TASK* _p_task = NULL;
	LOG_DEBUG("tm_get_bt_task_sub_file_gcid");
	
	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	_p_param->_result = tm_get_task_by_id(_p_param->task_id,&_p_task);
	if(_p_param->_result==SUCCESS) 
	{
#ifdef ENABLE_BT
		if(_p_task->_task_type==BT_TASK_TYPE ||_p_task->_task_type==BT_MAGNET_TASK_TYPE)
		{
			if(_p_task->task_status!=TASK_S_IDLE)
				_p_param->_result = bt_get_sub_file_gcid( _p_task,_p_param->file_index,_p_param->tcid);
			else
			      _p_param->_result = TM_ERR_TASK_NOT_RUNNING;
		}
		else
#endif /* ENABLE_BT */
		{
			_p_param->_result = TM_ERR_TASK_TYPE;
		}

	}
	
	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 tm_get_task_gcid(void * _param)
{
	TM_GET_TASK_GCID* _p_param = (TM_GET_TASK_GCID*)_param;
	TASK* _p_task = NULL;
	LOG_DEBUG("tm_get_task_gcid");

	if(g_pTask_manager == NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d", _p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}

	_p_param->_result = tm_get_task_by_id(_p_param->task_id, &_p_task);
	if(_p_param->_result == SUCCESS)
	{
		if(_p_task->_task_type == P2SP_TASK_TYPE)
		{
			if(_p_task->task_status != TASK_S_IDLE)
				_p_param->_result = pt_get_task_gcid( _p_task, _p_param->gcid);
			else
				_p_param->_result = TM_ERR_TASK_NOT_RUNNING;
		}
		#ifdef ENABLE_EMULE
		else
		if(_p_task->_task_type==EMULE_TASK_TYPE)
		{
			if(_p_task->task_status!=TASK_S_IDLE)
			{
				_p_param->_result = emule_get_task_gcid(  _p_task ,_p_param->gcid);
			}
			else
			      _p_param->_result = TM_ERR_TASK_NOT_RUNNING;
		}
		#endif /* ENABLE_EMULE */
		else
		{
			_p_param->_result = TM_ERR_TASK_TYPE;
		}
	}

	LOG_DEBUG("signal_sevent_handle:_result=%d", _p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 tm_add_task_resource(void * _param)
{
	TM_ADD_TASK_RES* _p_param = (TM_ADD_TASK_RES*)_param;
	TASK* _p_task = NULL;
	LOG_ERROR("tm_add_task_resource");
	
	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	_p_param->_result = tm_get_task_by_id(_p_param->task_id,&_p_task);
	if(_p_param->_result==SUCCESS) 
	{
		//if(_p_task->_task_type==P2SP_TASK_TYPE)
		//{
				_p_param->_result = dt_add_task_res(  _p_task ,(EDT_RES*)_p_param->_res);
		//}
		//else
		//{
		//	_p_param->_result = TM_ERR_TASK_TYPE;
		//}

	}
	
	LOG_ERROR("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 tm_get_task_info(void * _param)
{
	TM_GET_TASK_INFO* _p_param = (TM_GET_TASK_INFO*)_param;
	ET_TASK_INFO * info = (ET_TASK_INFO *)(_p_param->info);

	_int32 i=0;
	LOG_DEBUG("tm_get_task_info(task_id=%d)",_p_param->task_id);
	
	if(g_pTask_manager==NULL)
	{
		return -1;
	}
	
	begin_read_data_block(NULL);
	for(i=0;i<MAX_ET_ALOW_TASKS;i++)
	{
		if(_p_param->task_id==g_Task_Info[i]._task_id)
		{
			sd_memcpy(info,&(g_Task_Info[i]),sizeof(ET_TASK_INFO));
			sd_assert(_p_param->task_id==info->_task_id);
			break;
		}
	}
	end_read_data();
	if(i<MAX_ET_ALOW_TASKS)
	{
		LOG_DEBUG("tm_get_task_info,SUCCESS:task_id=%d,_task_status=%d,_failed_code=%d",_p_param->task_id,info->_task_status,info->_failed_code);
		sd_assert(_p_param->task_id==info->_task_id);
		return SUCCESS;
	}
	else
	{
		LOG_DEBUG("tm_get_task_info,TM_ERR_INVALID_TASK_ID");
#ifdef _DEBUG
{
	begin_read_data_block(NULL);
	for(i=0;i<MAX_ET_ALOW_TASKS;i++)
	{
		LOG_DEBUG("!!!! i=%d,task_id=%d !!!!",i, g_Task_Info[i]._task_id);
	}
	end_read_data();		
}
#endif
		return TM_ERR_INVALID_TASK_ID;
	}
}

_int32 tm_get_task_info_ex(void * _param)
{
    TM_GET_TASK_INFO* _p_param = (TM_GET_TASK_INFO*)_param;
    MEDIA_TASK_STAT_INFO * info = (MEDIA_TASK_STAT_INFO *)(_p_param->info);
    sd_memset(info, 0, sizeof(MEDIA_TASK_STAT_INFO));
    _u32 ret_val = SUCCESS;

    _int32 i=0;
    LOG_DEBUG("tm_get_task_info_ex(task_id=%d)",_p_param->task_id);

    if(g_pTask_manager==NULL)
    {
        return -1;
    }

    TASK * p_task = NULL;
    _p_param->_result = tm_get_task_by_id(_p_param->task_id, &p_task);
    if (_p_param->_result != SUCCESS)
        goto OVER;

    if (p_task->_task_type != P2SP_TASK_TYPE)
    {
        _p_param->_result = -1;
        goto OVER;
    }

    P2SP_TASK * p_p2sp_task = (P2SP_TASK *)p_task;
    _int32 _data_status_code = 0;

    if((p_task->task_status!=TASK_S_RUNNING)&&(p_task->task_status!=TASK_S_VOD))
    {
        _p_param->_result = DT_ERR_TASK_NOT_RUNNING;
        goto OVER;
    }

    /* Get the downloading status */
    _data_status_code = dm_get_status_code(& (p_p2sp_task->_data_manager));
    if(_data_status_code ==DATA_DOWNING) 
    {
        info->status = M_TASK_RUNNING;
    }
    else if(_data_status_code ==DATA_SUCCESS) 
    {
        info->status = M_TASK_SUCCESS;
    }
    else if(_data_status_code ==VOD_DATA_FINISHED) 
    {
        info->status = M_TASK_VOD;
    }
    else
    {
        info->status = M_TASK_FAILED;
    }

    _u32 cur_sec = 0;
    sd_time(&cur_sec);
    info->download_time = cur_sec - p_task->_start_time;
    info->file_size = p_task->file_size;
#ifdef _CONNECT_DETAIL
    //server
    info->server.total_num = cm_get_idle_server_res_num(& (p_task->_connect_manager))
        + cm_get_using_server_res_num(& (p_task->_connect_manager))
        + cm_get_candidate_server_res_num(& (p_task->_connect_manager))
        + cm_get_retry_server_res_num(& (p_task->_connect_manager))
        + cm_get_discard_server_res_num(& (p_task->_connect_manager))
        + cm_get_destroy_server_res_num(& (p_task->_connect_manager));

    info->server.valid_num = cm_get_idle_server_res_num(& (p_task->_connect_manager))
        + cm_get_using_server_res_num(& (p_task->_connect_manager));

    info->server.connected_pipe_num = cm_get_working_server_pipe_num(& (p_task->_connect_manager));
    info->server.download_speed = cm_get_current_task_server_speed(&(p_task->_connect_manager));
    info->server.recv_size = p_p2sp_task->_data_manager._server_recv_bytes;
    info->server.download_size = p_p2sp_task->_data_manager._server_dl_bytes;
    //peer
    info->peer.total_num = cm_get_idle_peer_res_num(& (p_task->_connect_manager))
        + cm_get_using_peer_res_num(& (p_task->_connect_manager))
        + cm_get_candidate_peer_res_num(& (p_task->_connect_manager))
        + cm_get_retry_peer_res_num(& (p_task->_connect_manager))
        + cm_get_discard_peer_res_num(& (p_task->_connect_manager))
        + cm_get_destroy_peer_res_num(& (p_task->_connect_manager));

    info->peer.valid_num = cm_get_idle_peer_res_num(& (p_task->_connect_manager))
        + cm_get_using_peer_res_num(& (p_task->_connect_manager));

    info->peer.connected_pipe_num = cm_get_working_peer_pipe_num(& (p_task->_connect_manager));
    info->peer.download_speed = cm_get_current_task_peer_speed(&(p_task->_connect_manager));
    info->peer.recv_size = p_p2sp_task->_data_manager._peer_recv_bytes;
    info->peer.download_size = p_p2sp_task->_data_manager._peer_dl_bytes;
#endif

#ifdef ENABLE_CDN
    info->cdn.total_num = cm_get_cdn_res_num(&(p_task->_connect_manager));
    info->cdn.valid_num = cm_get_cdn_res_num(&(p_task->_connect_manager));
    info->cdn.connected_pipe_num = cm_get_working_cdn_pipe_num(&(p_task->_connect_manager));
    info->cdn.recv_size = p_p2sp_task->_data_manager._cdn_recv_bytes;
    info->cdn.recv_size += p_p2sp_task->_data_manager._lixian_recv_bytes;
    //info->cdn.recv_size += p_p2sp_task->_data_manager._origin_dl_bytes;
    info->cdn.download_speed = cm_get_cdn_speed(&(p_task->_connect_manager), TRUE);
    info->cdn.download_size = p_p2sp_task->_data_manager._cdn_dl_bytes;
    info->cdn.download_size += p_p2sp_task->_data_manager._lixian_dl_bytes;
#endif /* ENABLE_CDN */

    info->total_download_size = info->server.download_size + info->peer.download_size + info->cdn.download_size;
    info->total_recv_size = info->server.recv_size + info->peer.recv_size + info->cdn.recv_size;
OVER:
    return signal_sevent_handle(&(_p_param->_handle));
}

#ifdef ENABLE_CDN
_int32 tm_pause_download_except_task(TASK * p_task,BOOL pause)
{
	//_int32 _list_size = 0,i=0;
       LIST_ITERATOR cur_node = NULL;
	TASK* p_task_tmp = NULL;
	BOOL is_pipe_paused = FALSE;
	_int32 ret_val = SUCCESS;

	LOG_DEBUG("tm_pause_download_except_task");
 //#if defined(IPAD_KANKAN)&&defined(_DEBUG)
//	 printf("\n tm_pause_download_except_task:task_id=%u,%d \n ",p_task->_task_id,pause);
// #endif

	if(begin_write_data(NULL) != SUCCESS) return SUCCESS;

	cur_node = LIST_BEGIN(g_pTask_manager->_task_list );
	while( cur_node != LIST_END( g_pTask_manager->_task_list ) )
	{
		p_task_tmp = (TASK *)LIST_VALUE( cur_node );
		if((p_task_tmp!=p_task)&&((p_task_tmp->task_status==TASK_S_RUNNING)||(p_task_tmp->task_status==TASK_S_VOD)))
		{
			if(p_task_tmp->_task_type==P2SP_TASK_TYPE)
			{
			    LOG_DEBUG("tm_pause_download_except_task, p2sp task");			
			}
#ifdef ENABLE_BT
			else if(p_task_tmp->_task_type==BT_TASK_TYPE)
			{
			}
#endif /* #ifdef ENABLE_BT */							
#ifdef ENABLE_EMULE
			else if(p_task_tmp->_task_type == EMULE_TASK_TYPE)
			{
			}
#endif
		}

		cur_node = LIST_NEXT( cur_node );
	}

	end_write_data();

	 return SUCCESS;
}

#ifdef ENABLE_HSC
_int32 tm_init_task_hsc_info(void)
{
	sd_memset(&g_task_hsc_info, 0, sizeof(HSC_INFO_BRD)*MAX_ET_ALOW_TASKS);
	_int32 ret_val = init_customed_rw_sharebrd(&g_task_hsc_info, &g_task_hsc_info_brd_ptr);
	CHECK_VALUE(ret_val);
	return ret_val;
}

_int32 tm_uninit_task_hsc_info(void)
{
	_int32 ret_val = SUCCESS;	
	sd_memset(&g_task_hsc_info, 0, sizeof(HSC_INFO_BRD)*MAX_ET_ALOW_TASKS);	
	if (g_task_hsc_info_brd_ptr!=NULL)
	{
		ret_val = uninit_customed_rw_sharebrd(g_task_hsc_info_brd_ptr);
		CHECK_VALUE(ret_val);
		g_task_hsc_info_brd_ptr=NULL;
	}	
	return ret_val;
}

//NOTE:jieouy涉及到对g_pTask_manager的处理，必须确保在et线程里面，所有调用该方法的，都需要调用tm_post_function。目前发现调用方有问题，后续优化
_int32 tm_update_task_hsc_info(void)
{
	LOG_DEBUG("tm_update_task_hsc_info");
    if (NULL == g_pTask_manager)
    {
        return -1;
    }

    _int32 _list_size = list_size(&(g_pTask_manager->_task_list)); 
	if (_list_size != 0)
	{
		pLIST_NODE _p_list_node = LIST_BEGIN(g_pTask_manager->_task_list);
		if (cus_rws_begin_write_data(g_task_hsc_info_brd_ptr, NULL) == SUCCESS)
		{
		    _int32 i = 0;
		    TASK *_p_task = NULL;
		    HIGH_SPEED_CHANNEL_INFO *_p_hsc_info = NULL;
			sd_memset(g_task_hsc_info, 0, sizeof(HSC_INFO_BRD)*MAX_ET_ALOW_TASKS);
			while (_list_size)
			{
				_p_task = (TASK * )LIST_VALUE(_p_list_node);
				_p_hsc_info = &_p_task->_hsc_info;
				LOG_DEBUG("_list_size=%d, i=%d, task_id=%d, _hsc_stat=%d", _list_size, i, _p_task->_task_id, _p_hsc_info->_stat);
				if (_p_hsc_info->_res_num!=0)
				{
				    if(i<MAX_ET_ALOW_TASKS)
				    {
				        g_task_hsc_info[i]._task_id = _p_task->_task_id;
				        sd_memcpy(&g_task_hsc_info[i]._hsc_info, _p_hsc_info, sizeof(HIGH_SPEED_CHANNEL_INFO));
				        i++;
				    }
				}
				_p_list_node = LIST_NEXT(_p_list_node);
				_list_size--;
			}
		
			/* unlock data for read */
			cus_rws_end_write_data(g_task_hsc_info_brd_ptr);
		}
	}
 	return SUCCESS;
}

_int32 tm_get_task_hsc_info(void *_param)
{
	TM_GET_TASK_HSC_INFO* _p_param = (TM_GET_TASK_HSC_INFO*)_param;
	HIGH_SPEED_CHANNEL_INFO_API *p_et_info = (HIGH_SPEED_CHANNEL_INFO_API *)(_p_param->_info);
#if !defined(ENABLE_ETM_MEDIA_CENTER)
	HIGH_SPEED_CHANNEL_INFO *p_info = NULL;
#endif

	LOG_DEBUG("tm_get_task_hsc_info(task_id=%d)", _p_param->_task_id);
	
	if (NULL == g_pTask_manager)
	{
	    _p_param->_result = -1;
		return signal_sevent_handle(&(_p_param->_handle));
	}
#if defined(ENABLE_ETM_MEDIA_CENTER)
	TASK * p_task = NULL;
    _p_param->_result = tm_get_task_by_id(_p_param->_task_id, &p_task);
    if (_p_param->_result != SUCCESS)
    {
        return signal_sevent_handle(&(_p_param->_handle));
	}

	cus_rws_begin_read_data(g_task_hsc_info_brd_ptr, NULL);

	p_et_info->_speed = p_task->_hsc_info._speed;
	p_et_info->_dl_bytes = p_task->_hsc_info._dl_bytes;
	cus_rws_end_read_data(g_task_hsc_info_brd_ptr);

#else
	cus_rws_begin_read_data(g_task_hsc_info_brd_ptr, NULL);
	_int32 i = 0;
	for (i = 0; i < MAX_ET_ALOW_TASKS; i++)
	{
		if (_p_param->_task_id == g_task_hsc_info[i]._task_id)
		{
			p_info = &g_task_hsc_info[i]._hsc_info;
			p_et_info->_can_use			= p_info->_can_use;
			p_et_info->_channel_type	= p_info->_channel_type;
			p_et_info->_cur_cost_flow	= p_info->_cur_cost_flow;
			p_et_info->_dl_bytes		= p_info->_dl_bytes;
			p_et_info->_errcode			= p_info->_errcode;
			p_et_info->_product_id		= p_info->_product_id;
			p_et_info->_remaining_flow	= *hsc_get_g_remainning_flow();
			p_et_info->_res_num			= p_info->_res_num;
			p_et_info->_speed			= p_info->_speed;
			p_et_info->_stat			= p_info->_stat;
			p_et_info->_sub_id			= p_info->_sub_id;
			p_et_info->_used_hsc_before	= p_info->_used_hsc_before;

			
			LOG_DEBUG("tm_get_task_hsc_info p_et_info->_cur_cost_flow=%llu",p_et_info->_cur_cost_flow);
			break;
		}
	}
	cus_rws_end_read_data(g_task_hsc_info_brd_ptr);
	
	if (i < MAX_ET_ALOW_TASKS)
	{
		LOG_DEBUG("tm_get_task_hsc_info,SUCCESS:task_id=%d,_hsc_stat=%d,_failed_code=%d", _p_param->_task_id, p_et_info->_stat, p_et_info->_errcode);
		_p_param->_result = SUCCESS;
	}
	else
	{
		LOG_DEBUG("tm_get_task_hsc_info,TM_ERR_INVALID_TASK_ID");
		_p_param->_result = TM_ERR_INVALID_TASK_ID;
	}
#endif

	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 tm_delete_task_hsc_info(_u32 task_id)
{
	_int32 i=0;
	
	LOG_DEBUG("tm_delete_task_hsc_info, delete id:%u .", task_id);	
		
	if(cus_rws_begin_write_data(g_task_hsc_info_brd_ptr, NULL) == SUCCESS)
	{
	   
             for(i=0;i<MAX_ET_ALOW_TASKS;i++)
       	{
       		if(task_id == g_task_hsc_info[i]._task_id)
       		{
       			sd_memset(&(g_task_hsc_info[i]),0,sizeof(HSC_INFO_BRD));
       			break;
       		}
       	}

             /* unlock data for read */
	     cus_rws_end_write_data(g_task_hsc_info_brd_ptr);
	}
			
			
        return SUCCESS;
}
#endif /* ENABLE_HSC */

#endif

_int32 tm_delete_task_info(_u32 task_id)
{
	_int32 i=0;
	
	LOG_DEBUG("tm_delete_task_info, delete id:%u .", task_id);	
		
	if(begin_write_data(NULL) == SUCCESS)
	{
	   
             for(i=0;i<MAX_ET_ALOW_TASKS;i++)
       	{
       		if(task_id == g_Task_Info[i]._task_id)
       		{
       			sd_memset(&(g_Task_Info[i]),0,sizeof(ET_TASK_INFO));
       			break;
       		}
       	}

             /* unlock data for read */
	     end_write_data();
	}
			
			
        return SUCCESS;
}


/* set_license */
_int32 	tm_set_license(void * _param)
{
	TM_SET_LICENSE* _p_param = (TM_SET_LICENSE*)_param;

	LOG_DEBUG("tm_set_license");
	
	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
#ifdef ENABLE_LICENSE
	_p_param->_result = settings_set_str_item("license.license", _p_param->license);;

	if(_p_param->_result == SUCCESS)
	{
		if(g_pTask_manager->_license_report_timer_id!=0)
		{
			cancel_timer(g_pTask_manager->_license_report_timer_id);
			g_pTask_manager->_license_report_timer_id = 0;
		}

		/* Report the license  */
		_p_param->_result =  reporter_license();
		if(_p_param->_result !=SUCCESS)
		{
			/* Start the license_report_timer and reporter the license 5 mins later */  // 
			LOG_DEBUG("Start the license_report_timer and reporter the license 0.5 mins later :ret_val=%d",_p_param->_result);
			start_timer(tm_handle_license_report_timeout, NOTICE_ONCE,30*1000, 0,(void*)g_pTask_manager,&(g_pTask_manager->_license_report_timer_id));  
		}
	}
#endif /* ENABLE_LICENSE */

	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}


/* set_license_callback */
_int32 	tm_set_license_callback( void * _param)
{
	TM_SET_LICENSE_CALLBACK* _p_param = (TM_SET_LICENSE_CALLBACK*)_param;

	LOG_DEBUG("tm_set_license_callback");
	
	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	g_pTask_manager->license_callback_ptr = _p_param->license_callback_ptr;
	//_p_param->_result = settings_set_int_item("license.callback_ptr",(_int32) (_p_param->license_callback_ptr));;
	
	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}


/* tm_notify_license_report_result */
typedef _int32 ( *NOTIFY_LICENSE_RESULT)(_u32 result, _u32 expire_time);
_int32 	tm_notify_license_report_result(_u32 result, _int32  report_interval,_int32  expire_time)
{
	_int32 ret_val,_max_retry_times;
	LOG_DEBUG("tm_notify_license_report_result:result=%d,report_interval=%d,expire_time=%d",result,report_interval,expire_time);
	ret_val= SUCCESS;
	_max_retry_times = MAX_LICENSE_REPORT_RETRY_TIMES;
	if(g_pTask_manager==NULL)
	{
		return -1;
	}

#if !defined( ENABLE_LICENSE)
	sd_assert(FALSE);
#else

	if(result==-1)
	{
		LOG_ERROR("Internal Error:errcode=%d,ret_val=%d", report_interval,  expire_time);
		result=TM_ERR_UNKNOWN;
		/* Maybe it is networks error */
		expire_time= report_interval;
		settings_get_int_item( "license.retry_count",&_max_retry_times);
		
		g_pTask_manager->license_report_interval = DEFAULT_LICENSE_REPORT_FAILED_INTERVAL;
		/* Check if retry failed */
		if(g_pTask_manager->_license_report_retry_count++ >= _max_retry_times)
		{
			/* Retry reporting failed,disable all the downloading channels */
		 	//cm_p2s_set_enable(FALSE);
		 	//cm_p2p_set_enable(FALSE);
#ifdef ENABLE_BT			
		 	//bt_set_enable(FALSE);
#endif /* #ifdef ENABLE_BT */
#ifdef ENABLE_EMULE
            //emule_set_enable(FALSE);      
#endif
    
			result=TM_ERR_LICENSE_REPORT_FAILED;
			LOG_ERROR("Internal Error:errcode=TM_ERR_LICENSE_REPORT_FAILED");
		}
		
		result = SUCCESS;
	}
	else
	{
		g_pTask_manager->license_report_interval = (_u32)report_interval;
		settings_set_int_item("license.report_interval", report_interval);
		
		g_pTask_manager->license_expire_time = (_u32)expire_time;
		settings_set_int_item("license.expire_time", expire_time);

		g_pTask_manager->_license_report_retry_count = 0;

		/* Ping to the server  */
		sd_assert(ret_val==SUCCESS);
	}
	/* call the license callback function to notify the result of the license report to the high level application */
	if(g_pTask_manager->license_callback_ptr !=NULL)
	{
		/* Be careful,very dangerous! */
		((NOTIFY_LICENSE_RESULT)(g_pTask_manager->license_callback_ptr))(result,(_u32)expire_time);
	}
	
	if(g_pTask_manager->_license_report_timer_id!=0)
	{
		cancel_timer(g_pTask_manager->_license_report_timer_id);
		g_pTask_manager->_license_report_timer_id = 0;
	}
	/* Start the license_report_timer */  // 
	ret_val = start_timer(tm_handle_license_report_timeout, NOTICE_ONCE,g_pTask_manager->license_report_interval*1000, 0,(void*)g_pTask_manager,&(g_pTask_manager->_license_report_timer_id));  
	if(ret_val!=SUCCESS)
	{
					/* FATAL ERROR! */
					LOG_ERROR("FATAL ERROR! should stop the program!ret_val= %d",ret_val);
					#ifdef _DEBUG
					CHECK_VALUE(ret_val);
					#endif
	}
#endif /* ENABLE_LICENSE */	

	return SUCCESS;
}


/* Remove the temp file(s) of task */
_int32 	tm_remove_task_tmp_file( void * _param)
{
	TM_REMOVE_TMP_FILE* _p_param = (TM_REMOVE_TMP_FILE*)_param;
	TASK * _p_task = NULL;

	LOG_DEBUG("tm_remove_task_tmp_file");
	
	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	_p_param->_result = tm_get_task_by_id(_p_param->task_id,&_p_task);
	if(_p_param->_result!=SUCCESS) goto ErrorHanle;

	if(_p_task->task_status != TASK_S_STOPPED)
	{
		_p_param->_result =TM_ERR_TASK_IS_RUNNING;
		goto ErrorHanle;
	}
	
	_p_param->_result = dt_remove_task_tmp_file( _p_task);

ErrorHanle:

		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
}

_int32 tm_get_all_task_id(void * _param)
{
    LOG_DEBUG("tm_get_all_task_id");
    TM_GET_ALL_TASK_ID* _p_param = (TM_GET_ALL_TASK_ID*)_param;
    if (g_pTask_manager == NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d", _p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}

	sd_memset(_p_param->_task_id_buffer, 0, (*(_p_param->_buffer_len))*sizeof(_u32));

	_u32 _list_size =  list_size(&(g_pTask_manager->_task_list)); 
	if (_list_size == 0)
	{
		LOG_DEBUG("TM_ERR_NO_TASK");
		_p_param->_result = TM_ERR_NO_TASK;
		goto ErrorHanle;
	}

	if (*(_p_param->_buffer_len) < _list_size)
	{
		*(_p_param->_buffer_len) = _list_size;
		_p_param->_result = TM_ERR_BUFFER_NOT_ENOUGH;
		goto ErrorHanle;
	}
	else if(*(_p_param->_buffer_len) > _list_size)
	{
		*(_p_param->_buffer_len) = _list_size;
	}

	pLIST_NODE p_list_node = LIST_BEGIN(g_pTask_manager->_task_list);
	_u32 index = 0;
	TASK* _p_task = NULL;
	for (; p_list_node != LIST_END(g_pTask_manager->_task_list); p_list_node = LIST_NEXT(p_list_node))
	{
		_p_task = (TASK * )LIST_VALUE(p_list_node);
		_p_param->_task_id_buffer[index++]=_p_task ->_task_id;
	}
	
ErrorHanle:
	
	LOG_DEBUG("signal_sevent_handle:_result=%d", _p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 tm_logout(void * _param)
{
	TM_LOGOUT* _p_param = (TM_LOGOUT*)_param;

	LOG_DEBUG("tm_logout");

	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
#if defined(MOBILE_PHONE)
	if(NT_WLAN==sd_get_global_net_type())
	{
		_p_param->_result = ptl_exit( );
	}
#else
	_p_param->_result = ptl_exit( );
#endif /* MOBILE_PHONE */
	
	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 tm_http_clear(void * _param)
{
	LOG_DEBUG("tm_http_clear");
	TM_LOGOUT* _p_param = (TM_LOGOUT*)_param;
	if (g_pTask_manager == NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	sd_http_clear();	

	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 tm_http_get( void * _param )
{
	TM_MINI_HTTP* _p_param = (TM_MINI_HTTP*)_param;
	HTTP_PARAM * p_http_param = (HTTP_PARAM*)_p_param->_param;
	_u32 * p_http_id = (_u32*)_p_param->_id;

	LOG_DEBUG("tm_http_get:htttp_num=%u,%s",g_mini_htttp_num,p_http_param->_url);

	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	if(p_http_param->_priority>=0)
	{
		/* -1:低优先级,不会暂停下载任务; 0:普通优先级,会暂停下载任务;1:高优先级,会暂停所有其他的网络连接  */
		cm_pause_global_pipes();
	}
	
	 _p_param->_result = sd_http_get(p_http_param,p_http_id);	
	if(_p_param->_result == SUCCESS)
	{
		if(p_http_param->_priority>=0)
		{
			g_mini_htttp_num++;
		}
	}
	LOG_DEBUG("signal_sevent_handle:_result=%d,http_id=%u",_p_param->_result,*p_http_id);
	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 tm_http_post( void * _param )
{
	TM_MINI_HTTP* _p_param = (TM_MINI_HTTP*)_param;
	HTTP_PARAM * p_http_param = (HTTP_PARAM*)_p_param->_param;
	_u32 * p_http_id = (_u32*)_p_param->_id;

	LOG_DEBUG("tm_http_post:htttp_num=%u,%s",g_mini_htttp_num,p_http_param->_url);

	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	if(p_http_param->_priority>=0)
	{
		/* -1:低优先级,不会暂停下载任务; 0:普通优先级,会暂停下载任务;1:高优先级,会暂停所有其他的网络连接  */
		cm_pause_global_pipes();
	}
	
	 _p_param->_result = sd_http_post(p_http_param,p_http_id);	
	if(_p_param->_result == SUCCESS)
	{
		if(p_http_param->_priority>=0)
		{
			g_mini_htttp_num++;
		}
	}
	LOG_DEBUG("signal_sevent_handle:_result=%d,http_id=%u",_p_param->_result,*p_http_id);
	return signal_sevent_handle(&(_p_param->_handle));
}


_int32 tm_http_cancel( void * _param )
{
	TM_MINI_HTTP_CLOSE* _p_param = (TM_MINI_HTTP_CLOSE*)_param;
	_u32 http_id = (_u32)_p_param->_id;

	LOG_DEBUG("tm_http_close:htttp_num=%u,http_id=%u",g_mini_htttp_num,http_id);

	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	 _p_param->_result = sd_http_cancel(http_id);	

	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 tm_http_close( void * _param )
{
	TM_MINI_HTTP_CLOSE* _p_param = (TM_MINI_HTTP_CLOSE*)_param;
	_u32 http_id = (_u32)_p_param->_id;

	LOG_DEBUG("tm_http_close:htttp_num=%u,http_id=%u",g_mini_htttp_num,http_id);

	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	 _p_param->_result = sd_http_close(http_id);	

	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

/* 取消 global_pipes */
_int32 tm_resume_global_pipes_after_close_mini_http( void  )
{
	 g_mini_htttp_num--;
	 sd_assert(g_mini_htttp_num>=0);
	if(g_mini_htttp_num==0)
	{
		cm_resume_global_pipes();
	}
	return SUCCESS;
}

/*--------------------------------------------------------------------------*/
/*                Interfaces provid for p2p module				        */
/*--------------------------------------------------------------------------*/

BOOL tm_is_task_exist(const _u8* gcid, BOOL include_no_disk)
{
	pLIST_NODE p_list_node=NULL ;
	TASK *_p_task = NULL;
	
#ifdef _LOGGER
	_int32 ret_val = SUCCESS;
	 char cid_str[CID_SIZE*2+1]; 

      ret_val = str2hex((char*)gcid, CID_SIZE, cid_str, CID_SIZE*2);
	CHECK_VALUE( ret_val );
	cid_str[CID_SIZE*2] = '\0';
	LOG_DEBUG( "tm_is_task_exist: gcid = %s,", cid_str );
#endif
	
	if(0!=list_size( &(g_pTask_manager->_task_list)))
	{
		for(p_list_node = LIST_BEGIN(g_pTask_manager->_task_list); p_list_node != LIST_END(g_pTask_manager->_task_list); p_list_node = LIST_NEXT(p_list_node))
		{
			_p_task = (TASK * )LIST_VALUE(p_list_node);
			if(_p_task->_task_type==P2SP_TASK_TYPE)
			{
                if (TRUE== pt_is_task_exist_by_gcid(_p_task, gcid, include_no_disk))
                {
					return TRUE;
                }
			}
#ifdef ENABLE_BT
			else if(_p_task->_task_type==BT_TASK_TYPE)
			{
					if(TRUE== bt_is_task_exist_by_gcid(_p_task, gcid))
						return TRUE;
			}
#endif /*  #ifdef ENABLE_BT */
#ifdef ENABLE_EMULE
			else if(_p_task->_task_type == EMULE_TASK_TYPE)
			{
                if(TRUE== emule_is_task_exist_by_gcid(_p_task, gcid))
                    return TRUE;
				//return emule_is_task_exist_by_gcid(_p_task, gcid);
			}
#endif /*  #ifdef ENABLE_BT */
		}
					
					
	}
	
	 	return FALSE;

}

CONNECT_MANAGER * tm_get_task_connect_manager(const _u8* gcid,_u32 * file_index, _u8 file_id[FILE_ID_SIZE])
{
	pLIST_NODE p_list_node=NULL ;
	TASK *_p_task = NULL;
	CONNECT_MANAGER * p_cm=NULL;
	
#ifdef _LOGGER
	_int32 ret_val = SUCCESS;
	char cid_str[CID_SIZE*2+1]; 

    ret_val = str2hex((char*)gcid, CID_SIZE, cid_str, CID_SIZE*2);	
	cid_str[CID_SIZE*2] = '\0';
	LOG_DEBUG( "tm_get_task_connect_manager: gcid = %s", cid_str );

#ifdef ENABLE_EMULE
    emule_log_print_file_id(file_id, "in tm_get_task_connect_manager");
#endif //ENABLE_EMULE
#endif
	
	if(0!=list_size( &(g_pTask_manager->_task_list)))
	{
		for(p_list_node = LIST_BEGIN(g_pTask_manager->_task_list); p_list_node != LIST_END(g_pTask_manager->_task_list); p_list_node = LIST_NEXT(p_list_node))
		{
			_p_task = (TASK * )LIST_VALUE(p_list_node);

            LOG_DEBUG("task type = %u", _p_task->_task_type);
			if(_p_task->_task_type==P2SP_TASK_TYPE)
			{
                if (TRUE== pt_is_task_exist_by_gcid(_p_task, gcid, TRUE))
				{
					p_cm=&(_p_task->_connect_manager);
					*file_index=INVALID_FILE_INDEX;
					return p_cm;
				}
			}
#ifdef ENABLE_BT
			else
			if(_p_task->_task_type==BT_TASK_TYPE)
			{
				p_cm=bt_get_task_connect_manager(_p_task, gcid,file_index);
				if(NULL!=p_cm )
					return p_cm;
			}
#endif
#ifdef ENABLE_EMULE
            else
            if(_p_task->_task_type==EMULE_TASK_TYPE)
            {
                if (emule_is_my_task(_p_task, file_id, gcid) == TRUE)
                {
                    p_cm= &_p_task->_connect_manager;
                    if(NULL == file_id)
                    {
                        *file_index = 0;
                    }
                    else
                    {
                        *file_index = INVALID_FILE_INDEX;
                    }
                    if(NULL!=p_cm )
                        return p_cm;
                }
            }
#endif
		}
	}

    LOG_DEBUG("get task connect manager error.");
    sd_assert(FALSE);
	
    return NULL;
}

 /*--------------------------------------------------------------------------*/
/*                Callback function for timer				        */
/*--------------------------------------------------------------------------*/
 _int32 tm_handle_update_info_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid)
 {
	pLIST_NODE p_list_node=NULL ;
	TASK* _p_task = NULL;
	P2SP_TASK * p_p2sp_task = NULL;
#ifdef ENABLE_BT
	BT_TASK * p_bt_task = NULL;
#endif
	LOG_DEBUG("tm_handle_update_info_timeout start");

	//if (!_pTask_manager) return TM_ERR_UNKNOWN;
	if(g_pTask_manager==NULL)
	{
		LOG_ERROR("tm_handle_update_info_timeout:g_pTask_manager==NULL");
		return -1;
	}

	if(get_critical_error()!=SUCCESS)
	{
		LOG_ERROR("tm_handle_update_info_timeout:%d",get_critical_error());
		return SUCCESS;
	}

	if(errcode == MSG_TIMEOUT)
	{
		if(/*msg_info->_device_id*/msgid == g_pTask_manager->_global_statistic._update_info_timer_id)
		{
		    tm_update_task();
		    cm_do_global_connect_dispatch();
				
				if(0!=list_size( &(g_pTask_manager->_task_list)))
				{
					for(p_list_node = LIST_BEGIN(g_pTask_manager->_task_list)
					    ; p_list_node != LIST_END(g_pTask_manager->_task_list)
					    ; p_list_node = LIST_NEXT(p_list_node))
					{
						_p_task = (TASK * )LIST_VALUE(p_list_node);
						if( (_p_task->task_status==TASK_S_RUNNING)
						    ||(_p_task->task_status==TASK_S_VOD) )
						{
							if(_p_task->_task_type==P2SP_TASK_TYPE)
							{
								p_p2sp_task = (P2SP_TASK *)_p_task;
								dm_handle_extra_things(&(p_p2sp_task->_data_manager));
							}
							else if(_p_task->_task_type == BT_TASK_TYPE)
							{
#ifdef ENABLE_BT
								p_bt_task = (BT_TASK *)_p_task;
							if( p_bt_task->_magnet_task_ptr == NULL )
							{
								bdm_on_time(&(p_bt_task->_data_manager));
							}
#endif
							}
						}
					}
								
								
				}
 			tm_check_network_status();
		}	
	}
	res_query_update_hub();	
	LOG_DEBUG("tm_handle_update_info_timeout end!");
	return SUCCESS;
 }


 _int32 tm_handle_license_report_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid)
 {
	_int32 ret_val = SUCCESS;
	LOG_DEBUG("tm_handle_license_report_timeout:%d",errcode);
	
	if(g_pTask_manager==NULL)
	{
		return -1;
	}
	
#if defined( MOBILE_PHONE)
	sd_assert(FALSE);
#endif /* symbian */	

	if(errcode == MSG_TIMEOUT)
	{
		if(/*msg_info->_device_id*/msgid == g_pTask_manager->_license_report_timer_id)
		{
			g_pTask_manager->_license_report_timer_id = 0;
		#ifdef EMBED_REPORT
			ret_val = reporter_license();
			if(ret_val!=SUCCESS)
			{
				tm_notify_license_report_result(-1,ret_val,0);
			}
		#endif
		}
	}
		
	return SUCCESS;
 }


#ifdef ENABLE_BT
_int32 tm_init_bt_file_info_pool(void)
{
	_int32 ret_val=SUCCESS;
	sd_memset(&g_bt_file_info_pool,0,sizeof(BT_FILE_INFO_POOL)*MAX_ET_ALOW_TASKS);
	ret_val = init_customed_rw_sharebrd(&g_bt_file_info_pool, &g_bt_file_info_pool_brd_ptr);
	CHECK_VALUE(ret_val);
	return SUCCESS;
}
_int32 tm_uninit_bt_file_info_pool(void)
{
	_int32 ret_val=SUCCESS;
	
	sd_memset(&g_bt_file_info_pool,0,sizeof(BT_FILE_INFO_POOL)*MAX_ET_ALOW_TASKS);
	
	if(g_bt_file_info_pool_brd_ptr!=NULL)
	{
		ret_val = uninit_customed_rw_sharebrd(g_bt_file_info_pool_brd_ptr);
		CHECK_VALUE(ret_val);
		g_bt_file_info_pool_brd_ptr=NULL;
	}
	
	return SUCCESS;
}

_int32 tm_add_bt_file_info_to_pool(TASK* p_task)
{
	_int32 index,ret_val=SUCCESS,wait_count=0;
	LOG_ERROR("tm_add_bt_file_info_to_pool[0x%X], task_id:%u", p_task, p_task->_task_id);
	
WRITE_AGAIN:
	ret_val = cus_rws_begin_write_data(g_bt_file_info_pool_brd_ptr, NULL);
	if(ret_val!=SUCCESS)
	{
		if(wait_count++>=3)
			CHECK_VALUE(ret_val);
		
		sd_sleep(1);	
		goto WRITE_AGAIN;
	}
	

	for(index=0;index<MAX_ET_ALOW_TASKS;index++)
	{
			if(g_bt_file_info_pool[index]._task_id==0)
			{
				
				bt_get_file_info_pool(p_task, &(g_bt_file_info_pool[index]) );
				break;
			}
	}
	
	cus_rws_end_write_data(g_bt_file_info_pool_brd_ptr);

	return SUCCESS;
}


_int32 tm_delete_bt_file_info_to_pool(_u32 task_id)
{
	_int32 index,ret_val=SUCCESS,wait_count=0;
WRITE_AGAIN:
	ret_val = cus_rws_begin_write_data(g_bt_file_info_pool_brd_ptr, NULL);
	if(ret_val!=SUCCESS)
	{
		if(wait_count++>=3)
			CHECK_VALUE(ret_val);
		
		sd_sleep(1);	
		goto WRITE_AGAIN;
	}

	for(index=0;index<MAX_ET_ALOW_TASKS;index++)
	{
			if(g_bt_file_info_pool[index]._task_id==task_id)
			{
				
				sd_memset(&(g_bt_file_info_pool[index]),0,sizeof(BT_FILE_INFO_POOL) );
				break;
			}
	}

	cus_rws_end_write_data(g_bt_file_info_pool_brd_ptr);

	return SUCCESS;
}

_int32 tm_get_bt_file_info(void * _param)
{
	TM_GET_BT_FILE_INFO* _p_param = (TM_GET_BT_FILE_INFO*)_param;
	_int32 ret_val = SUCCESS;

	_int32 index=0;
	ret_val=cus_rws_begin_read_data(g_bt_file_info_pool_brd_ptr, NULL);
	if(ret_val==SUCCESS)
	{
		for(index=0;index<MAX_ET_ALOW_TASKS;index++)
		{
			if(g_bt_file_info_pool[index]._task_id==_p_param->task_id)
			{
				
				ret_val=bt_get_file_info( &g_bt_file_info_pool[index], _p_param->file_index, _p_param->info );
				if( ret_val!=SUCCESS )
				{
					//CHECK_VALUE( ret_val );
					_p_param->_result = ret_val;
				}
				break;
			}
		}
		cus_rws_end_read_data(g_bt_file_info_pool_brd_ptr);
	}

	if(index>=MAX_ET_ALOW_TASKS)
	{
		ret_val = TM_ERR_INVALID_TASK_ID;
		_p_param->_result = ret_val;
	}
	return ret_val;
}

_int32 tm_get_bt_file_path_and_name(void * _param)
{
	TM_GET_BT_TASK_FILE_NAME* _p_param = (TM_GET_BT_TASK_FILE_NAME*)_param;
	TASK* _p_task = NULL;
	LOG_DEBUG("tm_get_bt_file_path_and_name");
	
	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	_p_param->_result = tm_get_task_by_id(_p_param->task_id,&_p_task);
	if(_p_param->_result==SUCCESS) 
	{
		if(_p_task->_task_type==BT_TASK_TYPE)
		{
				_p_param->_result = bt_get_task_file_path_and_name(  _p_task ,_p_param->file_index,_p_param->file_path_buffer,_p_param->file_path_buffer_size,_p_param->file_name_buffer,_p_param->file_name_buffer_size);
		}
		else
		{
			_p_param->_result = TM_ERR_TASK_TYPE;
		}

	}
	
	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 tm_get_bt_tmp_path(void * _param)
{
	TM_GET_BT_TMP_FILE_PATH* _p_param = (TM_GET_BT_TMP_FILE_PATH*)_param;
	TASK* _p_task = NULL;
	LOG_DEBUG("tm_get_bt_tmp_path");
	
	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	_p_param->_result = tm_get_task_by_id(_p_param->task_id,&_p_task);
	if(_p_param->_result==SUCCESS) 
	{
		if(_p_task->_task_type==BT_TASK_TYPE)
		{
			_p_param->_result = bt_get_tmp_file_path_name(  _p_task ,_p_param->tmp_file_path, _p_param->tmp_file_name);
		}
		else
		{
			_p_param->_result = TM_ERR_TASK_TYPE;
		}

	}
	
	LOG_DEBUG("tm_get_bt_tmp_path signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

//判断磁力连任务种子文件是否下载完成
_int32 tm_get_bt_magnet_torrent_seed_downloaded(void * _param)
{
	TM_GET_TORRENT_SEED_FILE_PATH* _p_param = (TM_GET_TORRENT_SEED_FILE_PATH*)_param;
	TASK* _p_task = NULL;
	LOG_DEBUG("tm_get_bt_magnet_torrent_seed_downloaded");
	
	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	_p_param->_result = tm_get_task_by_id(_p_param->task_id,&_p_task);
	if(_p_param->_result==SUCCESS) 
	{
		if(_p_task->_task_type==BT_TASK_TYPE)
		{
			_int32 ret_val = -1;
			BT_TASK *p_bt_task = (BT_TASK*)_p_task;
#if defined(ENABLE_ETM_MEDIA_CENTER)
			if( _p_task->task_status == TASK_S_RUNNING && 
				p_bt_task->_is_magnet_task && !p_bt_task->_magnet_task_ptr)
#else
			if( p_bt_task->_is_magnet_task && 
				(!p_bt_task->_magnet_task_ptr || (MAGNET_TASK_SUCCESS==p_bt_task->_magnet_task_ptr->_task_status) ) )
#endif
			{
				*_p_param->_seed_download= TRUE;
				ret_val = SUCCESS;
			}
			
			_p_param->_result = ret_val;
		}
		else
		{
			_p_param->_result = TM_ERR_TASK_TYPE;
		}

	}
	
	LOG_DEBUG("tm_get_bt_magnet_torrent_seed_downloaded signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));		

}


#endif  /*  #ifdef ENABLE_BT */

#ifdef ENABLE_DRM 

_int32 tm_set_certificate_path(void * _param)
{

	TM_SET_CERTIFICATE_PATH* _p_param = (TM_SET_CERTIFICATE_PATH*)_param;
	LOG_DEBUG("tm_set_certificate_path");
	
	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	_p_param->_result = drm_set_certificate_path( _p_param->_certificate_path_ptr );
	
	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));    
}


_int32 tm_open_drm_file(void * _param)
{

	TM_OPEN_DRM_FILE* _p_param = (TM_OPEN_DRM_FILE*)_param;
	LOG_DEBUG("tm_open_drm_file");
	
	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	_p_param->_result = drm_open_file( _p_param->_file_full_path_ptr, _p_param->_drm_id_ptr, _p_param->_origin_file_size_ptr );
	
	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));    
}

_int32 tm_is_certificate_ok(void * _param)
{

	TM_IS_CERTIFICATE_OK* _p_param = (TM_IS_CERTIFICATE_OK*)_param;
	LOG_DEBUG("tm_is_certificate_ok");
	
	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	_p_param->_result = drm_is_certificate_ok( _p_param->_drm_id, _p_param->_is_ok );
	
	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));    
}

_int32 tm_read_drm_file(void * _param)
{

	TM_READ_DRM_FILE* _p_param = (TM_READ_DRM_FILE*)_param;
	LOG_DEBUG("tm_read_drm_file");
	
	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	_p_param->_result = drm_read_file( _p_param->_drm_id, _p_param->_buffer_ptr,
        _p_param->_size, _p_param->_file_pos, _p_param->_read_size_ptr );

	
	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));    
}

_int32 tm_close_drm_file(void * _param)
{

	TM_CLOSE_DRM_FILE* _p_param = (TM_CLOSE_DRM_FILE*)_param;
	LOG_DEBUG("tm_close_drm_file");
	
	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	_p_param->_result = drm_close_file( _p_param->_drm_id );

	
	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));    
}

#endif  /*  #ifdef ENABLE_DRM */

_int32 tm_get_task_list(LIST * * pp_task_list)
{
	*pp_task_list = &g_pTask_manager->_task_list;
	return SUCCESS;
}

#if 0//ENABLE_CDN
_int32 tm_enable_high_speed_channel(void * _param)
{
	TM_HIGH_SPEED_CHANNEL_SWITCH *_p_param = (TM_HIGH_SPEED_CHANNEL_SWITCH*)_param;
	CONNECT_MANAGER *p_connect_manager = NULL;
	TASK *p_task = NULL;
	P2SP_TASK *p_p2sp_task = NULL;
	EMULE_TASK *p_emule_task = NULL;
	_u8 gcid[CID_SIZE];
	_u8 cid[CID_SIZE];
	_u64 file_size;
	LOG_DEBUG("tm_enable_high_speed_channel");
	_p_param->_result = tm_get_task_by_id(_p_param->_task_id, &p_task);

	if(_p_param->_result == SUCCESS)
	{

		if(p_task->_hsc_info._can_use == FALSE)
		{
			_p_param->_result = HSC_NOT_AVAILABLE;
			return signal_sevent_handle(&(_p_param->_handle));
		}
	
		switch(p_task->_task_type)
		{
			case P2SP_TASK_TYPE:
				p_p2sp_task = (P2SP_TASK*)p_task;
				_p_param->_result = pt_get_task_gcid(p_task, gcid);
				CHECK_VALUE(_p_param->_result);
				file_size = dm_get_file_size(&p_p2sp_task->_data_manager);
				_p_param->_result = pt_get_task_tcid(p_task, cid);
				break;
			case BT_TASK_TYPE:
				break;
			case EMULE_TASK_TYPE:
				p_emule_task = (EMULE_TASK*)p_task;
				_p_param->_result = emule_get_task_gcid(p_task, gcid);
				CHECK_VALUE(_p_param->_result);
				file_size = emule_get_file_size( (void*)p_emule_task->_data_manager);
				_p_param->_result = emule_get_task_cid(p_task, cid);
				break;
			default:
				sd_assert(FALSE);
		}
		if(_p_param->_result == SUCCESS)
		{
			if( (_p_param->_result = hsc_pq_query_permission(p_task, gcid, cid, file_size)) != SUCCESS)
			{
				return signal_sevent_handle(&(_p_param->_handle));
			}
		}
/*
		if(!hsc_pq_can_use_high_speed_channel(p_task))
		{
			_p_param->_result = p_task->_hsc_info._errcode;
			return signal_sevent_handle(&(_p_param->_handle));
		}
*/
		_p_param->_result = SUCCESS;
/*
		p_connect_manager = &(p_task->_connect_manager);
		cm_enable_high_speed_channel(p_connect_manager); // set enable flag true
		cm_update_cdn_pipes(p_connect_manager); // enable immediately
*/
	}
	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 tm_disable_high_speed_channel(void * _param)
{
	TM_HIGH_SPEED_CHANNEL_SWITCH *_p_param = (TM_HIGH_SPEED_CHANNEL_SWITCH*)_param;
	CONNECT_MANAGER *p_connect_manager = NULL;
	TASK *p_task = NULL;
	LOG_DEBUG("tm_enable_high_speed_channel");
	_p_param->_result = tm_get_task_by_id(_p_param->_task_id, &p_task);

	if(_p_param->_result == SUCCESS)
	{
		p_connect_manager = &(p_task->_connect_manager);
		cm_disable_high_speed_channel(p_connect_manager); // set enable flag false
		cm_update_cdn_pipes(p_connect_manager); // disable immediately
	}
	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 tm_get_hsc_info(void * _param)
{
	TM_HIGH_SPEED_CHANNEL_INFO *_p_param = (TM_HIGH_SPEED_CHANNEL_INFO*)_param;
	TASK *p_task = NULL;
	_p_param->_result = tm_get_task_by_id(_p_param->_task_id, &p_task);
	
	LOG_DEBUG("tm_get_hsc_info");
	if(_p_param->_result == SUCCESS)
	{
		sd_memcpy(_p_param->_p_hsc_info, &(p_task->_hsc_info), sizeof(HIGH_SPEED_CHANNEL_INFO));
	}
	
	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 tm_set_product_sub_id(void * _param)
{
	TM_HSC_SET_PRODUCT_SUB_ID_INFO *_p_param = (TM_HSC_SET_PRODUCT_SUB_ID_INFO*)_param;

	_p_param->_result =
	hsc_info_set_pdt_sub_id(_p_param->_task_id, _p_param->_product_id, _p_param->_sub_id);
	LOG_DEBUG("set product id %u, sub id %u", _p_param->_product_id, _p_param->_sub_id);

	return signal_sevent_handle(&(_p_param->_handle));
}
#endif /* ENABLE_CDN */
#ifdef ENABLE_LIXIAN
_int32 tm_init_task_lixian_info(void)
{
	_int32 ret_val=SUCCESS;
	sd_memset(&g_task_lixian_info,0,sizeof(LIXIAN_INFO_BRD)*MAX_ET_ALOW_TASKS);
	ret_val = init_customed_rw_sharebrd(&g_task_lixian_info, &g_task_lixian_info_brd_ptr);
	CHECK_VALUE(ret_val);
	return SUCCESS;
}

_int32 tm_uninit_task_lixian_info(void)
{
	_int32 ret_val=SUCCESS;
	
	sd_memset(&g_task_lixian_info,0,sizeof(LIXIAN_INFO_BRD)*MAX_ET_ALOW_TASKS);
	
	if(g_task_lixian_info_brd_ptr!=NULL)
	{
		ret_val = uninit_customed_rw_sharebrd(g_task_lixian_info_brd_ptr);
		CHECK_VALUE(ret_val);
		g_task_lixian_info_brd_ptr=NULL;
	}
	
	return SUCCESS;
}

_int32 tm_update_task_lixian_info(void)
{
	_int32 _list_size = 0,i=0,ret_val = SUCCESS;
	pLIST_NODE _p_list_node=NULL ;
	TASK* _p_task = NULL;
	LIXIAN_INFO lixian_info;

	LOG_DEBUG("tm_update_task_lixian_info");
	
	_list_size =  list_size(&(g_pTask_manager->_task_list)); 
	if(_list_size != 0)
	{
		_p_list_node = LIST_BEGIN(g_pTask_manager->_task_list);
		if(cus_rws_begin_write_data(g_task_lixian_info_brd_ptr, NULL) == SUCCESS)
		{
			sd_memset(g_task_lixian_info,0,sizeof(LIXIAN_INFO_BRD)*MAX_ET_ALOW_TASKS);
			while(_list_size)
			{
				_p_task = (TASK * )LIST_VALUE(_p_list_node);
				if(_p_task->_task_type == BT_TASK_TYPE) 
				{
					/* 暂不支持BT任务 */
					_p_list_node = LIST_NEXT(_p_list_node);
					_list_size--;
					continue;
				}
				ret_val = dt_get_lixian_info( _p_task,  INVALID_FILE_INDEX,&lixian_info );
				if(ret_val==SUCCESS)
				{
					if(i<MAX_ET_ALOW_TASKS)
					{
						g_task_lixian_info[i]._task_id = _p_task->_task_id;
						sd_memcpy(&g_task_lixian_info[i]._lixian_info, &lixian_info, sizeof(lixian_info));
						i++;
					}
				}
				_p_list_node = LIST_NEXT(_p_list_node);
				_list_size--;
			}
		
			/* unlock data for read */
			cus_rws_end_write_data(g_task_lixian_info_brd_ptr);
		}
	}
 	return SUCCESS;
}

_int32 tm_get_lixian_info(_u32 task_id,_u32 file_index,void * p_lx_info)
{
	_int32 i=0;
	LIXIAN_INFO * p_lixian_info = (LIXIAN_INFO *)p_lx_info;
	LOG_DEBUG("tm_get_task_lixian_info(task_id=%d), file_index = %d",task_id, file_index);
	
	if(g_pTask_manager==NULL)
	{
		return -1;
	}
	
	if(cus_rws_begin_read_data(g_task_lixian_info_brd_ptr, NULL)==SUCCESS)
	{
		for(i=0;i<MAX_ET_ALOW_TASKS;i++)
		{
			if(task_id==g_task_lixian_info[i]._task_id)
			{
				sd_memcpy(p_lixian_info, &g_task_lixian_info[i]._lixian_info,sizeof(LIXIAN_INFO));
				cus_rws_end_read_data(g_task_lixian_info_brd_ptr);
				return SUCCESS;
			}
		}
		cus_rws_end_read_data(g_task_lixian_info_brd_ptr);
	}

	return TM_ERR_INVALID_TASK_ID;
}


_int32 tm_delete_task_lixian_info(_u32 task_id)
{
	_int32 i=0;
	
	LOG_DEBUG("tm_delete_task_lixian_info, delete id:%u .", task_id);	
		
	if(cus_rws_begin_write_data(g_task_lixian_info_brd_ptr, NULL) == SUCCESS)
	{
	   
             for(i=0;i<MAX_ET_ALOW_TASKS;i++)
       	{
       		if(task_id == g_task_lixian_info[i]._task_id)
       		{
       			sd_memset(&(g_task_lixian_info[i]),0,sizeof(LIXIAN_INFO_BRD));
       			break;
       		}
       	}

             /* unlock data for read */
	     cus_rws_end_write_data(g_task_lixian_info_brd_ptr);
	}
			
			
        return SUCCESS;
}

_int32 tm_get_bt_lixian_info(void * _param)
{
	TM_POST_PARA_3 * p_param = (TM_POST_PARA_3*)_param;
	_u32 task_id = (_u32)p_param->_para1;
	_u32 file_index = (_u32)p_param->_para2;
	void * p_lx_info = (void *)p_param->_para3;
	TASK *p_task = NULL;
	LOG_DEBUG("tm_get_bt_lixian_info");
	p_param->_result = tm_get_task_by_id(task_id, &p_task);

	if(p_task->_task_type!=BT_TASK_TYPE)
	{
		p_param->_result = TM_ERR_INVALID_TASK_ID;
		return signal_sevent_handle(&(p_param->_handle));
	}

	if(p_param->_result == SUCCESS)
	{
		p_param->_result = dt_get_lixian_info( p_task,  file_index,p_lx_info );
	}
	return signal_sevent_handle(&(p_param->_handle));
}
#endif /* ENABLE_LIXIAN */

/* 设置一个可读写的路径，用于下载库存放临时数据*/
_int32 	tm_set_system_path( void * _param)
{
	TM_POST_PARA_1* _p_param = (TM_POST_PARA_1*)_param;
	const char * system_path =(const char *) _p_param->_para1;
	LOG_DEBUG("tm_set_license_callback");
	
	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	_p_param->_result = settings_set_str_item("system.system_path",(char*)system_path);

#ifdef LINUX
#ifdef ENABLE_EMULE
    kad_set_cfg_path(system_path);
#endif

#ifdef ENABLE_BT
    dht_set_cfg_path(system_path);
#endif
#endif
	
	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

/////////////////////////////////////////////////////////
#include "socket_proxy_interface.h"
#include "utility/local_ip.h"

#define MAX_TM_RETRY_COUNT (6)
#ifdef ENABLE_CHECK_NETWORK	
#define CHECK_NW_BUFFER_LEN (128)
#else
#define CHECK_NW_BUFFER_LEN (1)
#endif /* ENABLE_CHECK_NETWORK */
static SOCKET g_socket_id = INVALID_SOCKET ;
static SOCKET g_server_socket_id = INVALID_SOCKET ;
static _u16 g_server_port = 0;
static _u32 g_retry_count = 0;
static _u32 g_connection_refused_count = 0;
static _u32 g_connect_ok_count = 0;
static BOOL g_pipes_paused = FALSE;
static BOOL g_need_restart_server = FALSE;
static char g_rev_buffer[CHECK_NW_BUFFER_LEN] = {0};

_int32 tm_start_check_network(void)
{
	_int32 ret_val = SUCCESS;
#ifdef ENABLE_CHECK_NETWORK	

	g_socket_id =INVALID_SOCKET;
	g_retry_count = 0;
	g_connect_ok_count = 0;
	g_pipes_paused = FALSE;
	g_server_socket_id = INVALID_SOCKET ;
	g_server_port = 0;
	g_connection_refused_count = 0;
	g_need_restart_server = FALSE;
	ret_val =  tm_check_network_server_start();
#endif /* ENABLE_CHECK_NETWORK */
	
	return ret_val;
}
_int32 tm_stop_check_network(void)
{
	_int32 ret_val = SUCCESS;
#ifdef ENABLE_CHECK_NETWORK	
	_u32 op_count = 0;
	if(g_socket_id!=INVALID_SOCKET) 
	{
		ret_val = socket_proxy_peek_op_count(g_socket_id, DEVICE_SOCKET_TCP,&op_count);
		sd_assert(ret_val==SUCCESS);

		if(op_count!=0)
		{
			ret_val = socket_proxy_cancel(g_socket_id,DEVICE_SOCKET_TCP);
			sd_assert(ret_val==SUCCESS);
		}else
		{
			
			ret_val = socket_proxy_close(g_socket_id);
			sd_assert(ret_val==SUCCESS);
			g_socket_id = INVALID_SOCKET;
		}
	}
	
	ret_val = tm_check_network_server_stop();
	//sd_assert(ret_val==SUCCESS);
#endif /* ENABLE_CHECK_NETWORK */

	return ret_val;
}
_int32 tm_check_network_status(void)
{
	_int32 ret_val = SUCCESS;
#ifdef ENABLE_CHECK_NETWORK	
	SD_SOCKADDR addr = {0};

	if(g_need_restart_server)
	{
		/* 重启服务器 */
		ret_val = tm_check_network_server_stop();
		if(ret_val==SUCCESS)
		{
			ret_val = tm_check_network_server_start();
			CHECK_VALUE(ret_val);
			g_need_restart_server = FALSE;
		}
		return SUCCESS;
	}

	if(g_socket_id!=INVALID_SOCKET) return SUCCESS;
	ret_val = socket_proxy_create(&g_socket_id,SD_SOCK_STREAM);
	CHECK_VALUE(ret_val);

	addr._sin_family = SD_AF_INET;
	addr._sin_addr = sd_get_local_ip();
	addr._sin_port = sd_htons(g_server_port);
	
	LOG_DEBUG( "tm_check_network_status start connect:%u",g_socket_id );
	ret_val = socket_proxy_connect_with_timeout(g_socket_id,&addr,30,tm_handle_connect,NULL);
	if(ret_val!=SUCCESS)
	{
		socket_proxy_close(g_socket_id);
		g_socket_id = INVALID_SOCKET;
		CHECK_VALUE(ret_val);
	}
#endif /* ENABLE_CHECK_NETWORK */
	return ret_val;
}
_int32 tm_handle_connect( _int32 errcode, _u32 notice_count_left,void *user_data)
 {
	_int32 ret_val=SUCCESS;

	LOG_DEBUG( "tm_handle_connect:%d,%u",errcode,g_socket_id );

	ret_val = socket_proxy_close(g_socket_id);
	sd_assert(ret_val==SUCCESS);
	g_socket_id= INVALID_SOCKET;
	
	if(errcode==SUCCESS)
	{		
		/* Connecting success */
		g_retry_count = 0;
		g_connect_ok_count++;
		g_connection_refused_count = 0;
		if(g_pipes_paused&&g_connect_ok_count>=MAX_TM_RETRY_COUNT)
		{
			//cm_resume_all_global_pipes();
			tm_pause_download_except_task(NULL,FALSE);
			g_pipes_paused = FALSE;
			g_connect_ok_count = 0;
		}
		return SUCCESS;
	}		

	if((errcode== 61)||(errcode== 111))
	{
		/*
		UNIX: Error # 61: Connection refused
		Linux: Error # 111: Connection refused
		*/
		g_connection_refused_count++;
	}
	else
	{
		g_connection_refused_count = 0;
	}
	
	LOG_ERROR( "tm_handle_connect error:%d,retry=%u,ok=%u,paused=%d,connection_refused=%u",errcode ,g_retry_count,g_connect_ok_count,g_pipes_paused,g_connection_refused_count);
	if(errcode== MSG_CANCELLED)
	{
		LOG_DEBUG( "tm_handle_connect Cancelled!");
	}
	else
	{
		g_connect_ok_count = 0;
		g_retry_count++;
		if(g_retry_count<=MAX_TM_RETRY_COUNT)
		{
			tm_check_network_status();
		}
		else
		if(!g_pipes_paused)
		{
			LOG_URGENT( "tm_handle_connect error:%d,retry=%u,ok=%u,paused=%d,connection_refused=%u",errcode ,g_retry_count,g_connect_ok_count,g_pipes_paused,g_connection_refused_count);
			LOG_URGENT( "tm_handle_connect Connection time out,need pause all pipes!");
			//cm_pause_all_global_pipes();
			tm_pause_download_except_task(NULL,TRUE);
			g_pipes_paused = TRUE;
		}
		else
		if(g_connection_refused_count>MAX_TM_RETRY_COUNT)
		{
			LOG_URGENT( "tm_handle_connect error:%d,retry=%u,ok=%u,paused=%d,connection_refused=%u",errcode ,g_retry_count,g_connect_ok_count,g_pipes_paused,g_connection_refused_count);
			LOG_URGENT( "tm_handle_connect Connection refused,need restart server!");
			g_need_restart_server = TRUE;
			g_connection_refused_count = 0;
		}
	}

	return SUCCESS;
		
 } 
//////////////////////////////////////////////////////////////////////////////
_int32 tm_check_network_recv_callback(_int32 errcode, _u32 pending_op_count, char* buffer, _u32 had_recv, void* user_data)
{
	_int32 ret_val = SUCCESS,socket_err = SUCCESS;
	_u32 op_count= 0;
	SOCKET conn_sock = (SOCKET)user_data;
	
	LOG_DEBUG("tm_check_network_recv_callback..., sock=%d,errcode=%d,had_recv=%u", conn_sock,errcode,had_recv);
	if(errcode != SUCCESS)
	{
		socket_err = get_socket_error(conn_sock);
		ret_val = socket_proxy_peek_op_count(conn_sock, DEVICE_SOCKET_TCP,&op_count);
		sd_assert(ret_val==SUCCESS);
					
		LOG_DEBUG("tm_check_network_recv_callback:sock=%u,socket_err=%d,op_ret=%d,op_count=%u" ,conn_sock,socket_err,ret_val,op_count);
		if(op_count!=0)
		{
			ret_val = socket_proxy_cancel(conn_sock,DEVICE_SOCKET_TCP);
			sd_assert(ret_val==SUCCESS);
		}
		
		ret_val = socket_proxy_close(conn_sock);
		sd_assert(ret_val==SUCCESS);
		
		return SUCCESS;
	}
	
	//接收成功了
	sd_assert(had_recv > 0);
	LOG_DEBUG("tm_check_network_recv_callback:[%u]%s",had_recv,buffer);

	ret_val = socket_proxy_uncomplete_recv(conn_sock, g_rev_buffer, CHECK_NW_BUFFER_LEN, tm_check_network_recv_callback, (void*)conn_sock);
	sd_assert(ret_val==SUCCESS);
	
	return SUCCESS;
}

_int32 tm_check_network_accept_callback(_int32 errcode, _u32 pending_op_count, SOCKET conn_sock, void* user_data)
{
	_int32 ret_val = SUCCESS;

       LOG_DEBUG("tm_check_network_accept_callback errcode = %d, conn_sock=%d", errcode, conn_sock); 
	if(errcode == MSG_CANCELLED)
	{
		if(pending_op_count == 0)
		{
			ret_val =  socket_proxy_close(g_server_socket_id);
			sd_assert(ret_val == SUCCESS);
			g_server_socket_id = INVALID_SOCKET;
			return SUCCESS;
		}
		else 
			return SUCCESS;
	}
	
	if(errcode == SUCCESS)
	{	//accept success
		sd_memset(g_rev_buffer,0x00,CHECK_NW_BUFFER_LEN);

		ret_val = socket_proxy_uncomplete_recv(conn_sock, g_rev_buffer, CHECK_NW_BUFFER_LEN, tm_check_network_recv_callback, (void*)conn_sock);
		sd_assert(ret_val == SUCCESS);
		if(ret_val != SUCCESS)
		{
			socket_proxy_close(conn_sock);
		}
	}
	
       if(g_server_socket_id != INVALID_SOCKET)
       {
	      ret_val =  socket_proxy_accept(g_server_socket_id, tm_check_network_accept_callback, NULL);
		sd_assert(ret_val == SUCCESS);
       }
       return SUCCESS;
}
_int32 tm_check_network_server_stop(void)
{
	_u32 count = 0;
	_int32 ret_val = SUCCESS;
	LOG_DEBUG("tm_check_network_server_stop...");
	
	if(g_server_socket_id == INVALID_SOCKET)
		return SUCCESS;
	
	ret_val = socket_proxy_peek_op_count(g_server_socket_id, DEVICE_SOCKET_TCP, &count);
	if(SUCCESS != ret_val )
	{
	  	LOG_DEBUG("tm_check_network_server_stop. http_server is not running, ret_val = %d", ret_val);
	  	g_server_socket_id = INVALID_SOCKET;
		CHECK_VALUE(ret_val);
	}

	if(count > 0)
	{
		ret_val =  socket_proxy_cancel(g_server_socket_id, DEVICE_SOCKET_TCP);
		return 1;
	}
	else
	{
		ret_val  =  socket_proxy_close(g_server_socket_id);
	       g_server_socket_id = INVALID_SOCKET;
	}

	CHECK_VALUE(ret_val);

	return SUCCESS;
}

_int32 tm_check_network_server_start(void)
{
	SD_SOCKADDR	addr = {0};
	_int32 ret_val = SUCCESS;

	LOG_DEBUG("tm_check_network_server_start");

	sd_assert(g_server_socket_id==INVALID_SOCKET);
	
	ret_val = socket_proxy_create(&g_server_socket_id, SD_SOCK_STREAM);
	CHECK_VALUE(ret_val);
	
	if(g_server_port==0)
	{
		g_server_port = 36102;
	}

	
	addr._sin_family = SD_AF_INET;
	addr._sin_addr = ANY_ADDRESS;		/*any address*/
	addr._sin_port = sd_htons(g_server_port);
	
	ret_val = socket_proxy_bind(g_server_socket_id, &addr);
	if(ret_val != SUCCESS)
	{
	        LOG_DEBUG("tm_check_network_server_start...socket_proxy_bind ret=%d", ret_val);
		socket_proxy_close(g_server_socket_id);
		g_server_socket_id = INVALID_SOCKET;
		CHECK_VALUE(ret_val);
	}
	
	g_server_port = sd_ntohs(addr._sin_port);
	
	LOG_DEBUG("tm_check_network_server_start bind port %d...", g_server_port);
	
	ret_val = socket_proxy_listen(g_server_socket_id, 2);
	if(ret_val != SUCCESS)
	{
	        LOG_DEBUG("tm_check_network_server_start...socket_proxy_listen ret=%d", ret_val);
		socket_proxy_close(g_server_socket_id);
		g_server_socket_id = INVALID_SOCKET;
		CHECK_VALUE(ret_val);
	}
	
	ret_val = socket_proxy_accept(g_server_socket_id, tm_check_network_accept_callback, NULL);
	if(ret_val != SUCCESS)
	{
	        LOG_DEBUG("tm_check_network_server_start...socket_proxy_accept ret=%d", ret_val);
		socket_proxy_close(g_server_socket_id);
		g_server_socket_id = INVALID_SOCKET;
		CHECK_VALUE(ret_val);
	}

	return SUCCESS;
}

_int32 tm_res_query_notify_shub_handler(void* user_data,_int32 errcode, _u8 result, _u64 file_size, _u8* cid, _u8* gcid, _u32 gcid_level, _u8* bcid, _u32 bcid_size, _u32 block_size,_u32 retry_interval,char * file_suffix,_int32 control_flag)
{
	 _u8 final_result =errcode+result;
	TM_QUERY_SHUB_PAGE * p_res_query_pata = (TM_QUERY_SHUB_PAGE *)user_data;
	TM_QUERY_SHUB_RESULT query_result = {0};

	query_result._user_data = p_res_query_pata->_user_data;
	query_result._result = final_result;
	if(final_result==0)
	{
		query_result._file_size = file_size;
		sd_memcpy(query_result._cid, cid, CID_SIZE);
		sd_memcpy(query_result._gcid, gcid, CID_SIZE);
		sd_strncpy(query_result._file_suffix, file_suffix, 15);
#ifdef _DEBUG
{
		char cid_str[64] = {0};
		char gcid_str[64] = {0};
		str2hex((char*)query_result._cid, CID_SIZE, cid_str, 40);
		str2hex((char*)query_result._gcid, CID_SIZE, gcid_str, 40);
		LOG_DEBUG("tm_res_query_notify_shub_handler, fs=%llu,cid = %s, gcid = %s, file_suffix = %s", query_result._file_size,cid_str, gcid_str, query_result._file_suffix);
}
#endif
	}

	((tm_query_notify_shub_cb)(p_res_query_pata->_cb_ptr))(&query_result);

	SAFE_DELETE(p_res_query_pata);
	return SUCCESS;
}

_int32 tm_query_shub_by_url(void * _param)
{
	TM_POST_PARA_2* _p_param = (TM_POST_PARA_2*)_param;
	TM_QUERY_SHUB * p_query = (TM_QUERY_SHUB*)_p_param->_para1;
	_u32 * p_action_id = (_u32 *)_p_param->_para2;
	TM_QUERY_SHUB_PAGE * p_res_query_pata = NULL;
	char url_tmp[MAX_URL_LEN]={0};
	char *pos1=NULL,*pos2=NULL,*url = p_query->_url;
	_u32 url_length = sd_strlen(url);

	LOG_DEBUG("tm_query_shub_by_url,%s",p_query->_url);

	*p_action_id = 0;
	
	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}

	if((url==NULL)||(url_length<9))
	{
Q_BY_CID:
		/* 用cid和filesize方式查询 */
		if(!sd_is_cid_valid(p_query->_cid))
		{
			_p_param->_result = INVALID_ARGUMENT;
			goto ErrHandler;
		}

		_p_param->_result = sd_malloc(sizeof(TM_QUERY_SHUB_PAGE), (void**)&p_res_query_pata);
		if(_p_param->_result!=SUCCESS)
		{
			goto ErrHandler;
		}
		
		sd_memset(p_res_query_pata,0x00,sizeof(TM_QUERY_SHUB_PAGE));
		p_res_query_pata->_cb_ptr = p_query->_cb_ptr;
		p_res_query_pata->_user_data = p_query->_user_data;
		_p_param->_result = res_query_shub_by_cid((void*)p_res_query_pata, tm_res_query_notify_shub_handler, p_query->_cid, p_query->_file_size,FALSE, DEFAULT_LOCAL_URL, FALSE,1, 0, 0,0);
		if(_p_param->_result==SUCCESS)
		{
			*p_action_id = (_u32)p_res_query_pata;
		}
		else
		{
			SAFE_DELETE(p_res_query_pata);
		}
	}
	else
	{
		/* Check if the URL is thunder special URL */
		if(sd_strstr((char*)url, "thunder://", 0 )== url)
		{
			/* Decode to normal URL */
			if( sd_base64_decode(url+10, url_tmp, NULL)!=0)
			{
				_p_param->_result = INVALID_ARGUMENT;
				goto ErrHandler;
			}

			url_tmp[sd_strlen(url_tmp)-2]='\0';
			sd_strncpy(url_tmp,url_tmp+2,MAX_URL_LEN-1);
			url = url_tmp;
			url_length = sd_strlen(url);
		}
		
		if(sd_strncmp((char*)url, "magnet:?", sd_strlen("magnet:?") )== 0)
		{
			/* 暂不支持磁力链*/
			_p_param->_result = INVALID_ARGUMENT;
			goto ErrHandler;
		}
		else
		if(sd_strncmp((char*)url, "ed2k://", sd_strlen("ed2k://") )== 0)
		{
			if(sd_strncmp(url, "ed2k://%7", sd_strlen("ed2k://%7"))==0)
			{
				if(url_tmp!=url)
				{
					sd_strncpy(url_tmp, url, MAX_URL_LEN-1);
				}
				sd_replace_7c(url_tmp);
				url = url_tmp;
				url_length = sd_strlen(url);
			}
			/* 根据电驴URL 查询文件信息 */
			#ifdef ENABLE_EMULE
			_p_param->_result = sd_malloc(sizeof(TM_QUERY_SHUB_PAGE), (void**)&p_res_query_pata);
			if(_p_param->_result==SUCCESS)
			{
			   sd_memset(p_res_query_pata,0x00,sizeof(TM_QUERY_SHUB_PAGE));
			   p_res_query_pata->_cb_ptr = p_query->_cb_ptr;
			   p_res_query_pata->_user_data = p_query->_user_data;
			   _p_param->_result = emule_just_query_file_info(p_res_query_pata,url);
				if(_p_param->_result==SUCCESS)
				{
				   *p_action_id = (_u32)p_res_query_pata;
				}
				else
				{
				   SAFE_DELETE(p_res_query_pata);
				}
			}
			#else
			_p_param->_result = INVALID_ARGUMENT;
			#endif /* ENABLE_EMULE */
		}
		else
		{
			URL_OBJECT url_obj = {0};
			char file_name_tmp[MAX_FILE_NAME_BUFFER_LEN]={0};
			_u8 gcid_tmp[CID_SIZE]={0};
			
			if(url_tmp!=url)
			{
				sd_strncpy(url_tmp, url,MAX_URL_LEN-1);
				url = url_tmp;
			}

			/* 如果是看看URL，则改用cid方式查询*/
			if(sd_parse_kankan_vod_url(url,url_length ,gcid_tmp,p_query->_cid, &p_query->_file_size,file_name_tmp)==SUCCESS)
			{
				/* cid方式查询 */
				goto Q_BY_CID; 
			}

			_p_param->_result = sd_url_to_object(url,url_length,&url_obj);
			if(_p_param->_result!=SUCCESS) goto ErrHandler;
			
			_p_param->_result = sd_malloc(sizeof(TM_QUERY_SHUB_PAGE), (void**)&p_res_query_pata);
			if(_p_param->_result==SUCCESS)
			{
			   sd_memset(p_res_query_pata,0x00,sizeof(TM_QUERY_SHUB_PAGE));
			   p_res_query_pata->_cb_ptr = p_query->_cb_ptr;
			   p_res_query_pata->_user_data = p_query->_user_data;
				_p_param->_result = res_query_shub_by_url((void*)p_res_query_pata, tm_res_query_notify_shub_handler,url,p_query->_refer_url,FALSE, 1,0, 0,TRUE);
				if(_p_param->_result==SUCCESS)
				{
				   *p_action_id = (_u32)p_res_query_pata;
				}
				else
				{
				   SAFE_DELETE(p_res_query_pata);
				}
			}
		}
			
	}
	
ErrHandler:
	
	LOG_DEBUG("signal_sevent_handle:_result=%d,action_id=%X",_p_param->_result,*p_action_id);
	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 tm_query_shub_cancel(void * _param)
{
	TM_POST_PARA_1* _p_param = (TM_POST_PARA_1*)_param;
	void * user_data = (void* )_p_param->_para1;

	LOG_DEBUG("tm_query_shub_cancel,%X",user_data);

	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	 _p_param->_result = res_query_cancel( user_data, SHUB);

	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 tm_set_origin_mode( void * _param )
{
	TM_POST_PARA_2* _p_param = (TM_POST_PARA_2*)_param;
	_u32 task_id = (_u32)_p_param->_para1;
	BOOL origin_mode = (BOOL)_p_param->_para2;
	
	TASK * _p_task = NULL;

	LOG_DEBUG("tm_set_origin_mode:%d", origin_mode);

	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	_p_param->_result = tm_get_task_by_id(task_id, &_p_task);
	if(_p_param->_result != SUCCESS) goto ErrorHanle;
	
	
	if(_p_task->task_status != TASK_S_RUNNING)
	{
		_p_param->_result = TM_ERR_TASK_NOT_RUNNING;
		goto ErrorHanle;
	}

	_p_param->_result = pt_set_origin_mode(_p_task, origin_mode);

ErrorHanle:
	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));

}

_int32 tm_is_origin_mode( void * _param )
{
	TM_POST_PARA_2* _p_param = (TM_POST_PARA_2*)_param;
	_u32 task_id = (_u32)_p_param->_para1;
	BOOL* p_is_origin_mode = (BOOL*)_p_param->_para2;
	
	TASK * _p_task = NULL;

	LOG_DEBUG("tm_is_origin_mode:%d", task_id);

	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	_p_param->_result = tm_get_task_by_id(task_id, &_p_task);
	if(_p_param->_result != SUCCESS) goto ErrorHanle;
	

	_p_param->_result = pt_is_origin_mode(_p_task);
	if(_p_param->_result != -1)
		*p_is_origin_mode = _p_param->_result;

ErrorHanle:
	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));

}
_int32 tm_get_origin_resource_info( void * _param )
{
	TM_POST_PARA_2* _p_param = (TM_POST_PARA_2*)_param;
	_u32 task_id = (_u32)_p_param->_para1;
	void * p_resource = (void *)_p_param->_para2;
	
	TASK * _p_task = NULL;

	LOG_DEBUG("tm_get_origin_resource_info:%d", task_id);

	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	_p_param->_result = tm_get_task_by_id(task_id, &_p_task);
	if(_p_param->_result != SUCCESS) goto ErrorHanle;
	
	
	if(_p_task->task_status != TASK_S_RUNNING)
	{
		_p_param->_result = TM_ERR_TASK_NOT_RUNNING;
		goto ErrorHanle;
	}

	_p_param->_result = dt_get_origin_resource_info(_p_task, p_resource);

ErrorHanle:
	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));

}

_int32 tm_get_origin_download_data( void * _param )
{
	TM_POST_PARA_2* _p_param = (TM_POST_PARA_2*)_param;
	_u32 task_id = (_u32)_p_param->_para1;
	_u64 *download_data_size = (_u64 *)_p_param->_para2;
	
	TASK * _p_task = NULL;

	LOG_DEBUG("tm_get_origin_download_data:%d", task_id);

	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	_p_param->_result = tm_get_task_by_id(task_id, &_p_task);
	if(_p_param->_result != SUCCESS) goto ErrorHanle;
	
	
	if(_p_task->task_status != TASK_S_RUNNING)
	{
		_p_param->_result = TM_ERR_TASK_NOT_RUNNING;
		goto ErrorHanle;
	}

	if(P2SP_TASK_TYPE == _p_task->_task_type)
	{
		dm_get_origin_resource_dl_bytes( &(((P2SP_TASK *)_p_task)->_data_manager), download_data_size );
	}
ErrorHanle:
	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 tm_add_assist_peer_resource(void * _param)
{
	TM_ADD_TASK_RES* _p_param = (TM_ADD_TASK_RES*)_param;
	TASK* _p_task = NULL;
	LOG_ERROR("tm_add_task_resource");
	
	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	_p_param->_result = tm_get_task_by_id(_p_param->task_id,&_p_task);
	if(_p_param->_result==SUCCESS) 
	{
		_p_param->_result = dt_add_assist_task_res( _p_task ,(EDT_RES*)_p_param->_res);
	}
	
	LOG_ERROR("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 tm_get_assist_peer_info(void * _param)
{
	TM_POST_PARA_3 * p_param = (TM_POST_PARA_3*)_param;
	_u32 task_id = (_u32)p_param->_para1;
	_u32 file_index = (_u32)p_param->_para2;
	void * p_lx_info = (void *)p_param->_para3;
	TASK *p_task = NULL;
	LOG_DEBUG("tm_get_assist_peer_info");
	
	p_param->_result = tm_get_task_by_id(task_id, &p_task);

	if(p_param->_result == SUCCESS)
	{
		p_param->_result = dt_get_assist_task_res_info( p_task,  file_index,p_lx_info );
	}
	return signal_sevent_handle(&(p_param->_handle));
}

_int32 tm_set_extern_id( void * _param )
{
	TM_POST_PARA_2* _p_param = (TM_POST_PARA_2*)_param;
	_u32 inner_id = (_u32)_p_param->_para1;
	_u32 extern_id = (_u32)_p_param->_para2;
	
	TASK * _p_task = NULL;

	LOG_DEBUG("tm_set_extern_id:%d", inner_id);

	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("tm_set_extern_id:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	_p_param->_result = tm_get_task_by_id(inner_id, &_p_task);
	if(_p_param->_result != SUCCESS) goto ErrorHanle;

    LOG_DEBUG("tm_set_extern_id extern_id %d change to %d ",_p_task->_extern_id, extern_id);
	_p_task->_extern_id = extern_id;

ErrorHanle:
    
	LOG_DEBUG("tm_set_extern_id ErrorHanle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
    
}

_int32 tm_set_extern_info( void * _param )
{
	TM_POST_PARA_3* _p_param = (TM_POST_PARA_3*)_param;
	_u32 inner_id = (_u32)_p_param->_para1;
	_u32 extern_id = (_u32)_p_param->_para2;
	_u32 create_time = (_u32)_p_param->_para3;
	
	TASK * _p_task = NULL;

	LOG_DEBUG("tm_set_extern_info:%d extern_id=%u create_time=%u", inner_id,extern_id,create_time);

	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("tm_set_extern_info:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	_p_param->_result = tm_get_task_by_id(inner_id, &_p_task);
	if(_p_param->_result != SUCCESS) goto ErrorHanle;

    LOG_DEBUG("tm_set_extern_info extern_id %d change to %d ,create_time %d change to %d",
		_p_task->_extern_id,extern_id,_p_task->_create_time,create_time);
	_p_task->_extern_id = extern_id;
	_p_task->_create_time = create_time;

ErrorHanle:
    
	LOG_DEBUG("tm_set_extern_info ErrorHanle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
    
}

_int32 tm_get_task_downloading_info(void * _param)
{
	TM_POST_PARA_2 * p_param = (TM_POST_PARA_2*)_param;
	_u32 task_id = (_u32)p_param->_para1;
	void * p_res_info = (void *)p_param->_para2;
	
	TASK *p_task = NULL;

	p_param->_result = tm_get_task_by_id(task_id, &p_task);

	if(p_param->_result == SUCCESS && p_task != NULL)
	{
		p_param->_result = dt_get_task_downloading_res_info( p_task, p_res_info);
	}
		
	LOG_DEBUG("tm_get_task_downloading_info result = %d", p_param->_result);
	return signal_sevent_handle(&(p_param->_handle));
}

/* set_notify_event_callback */
_int32 	tm_set_notify_event_callback( void * _param)
{
	TM_POST_PARA_1* _p_param = (TM_POST_PARA_1*)_param;

	LOG_DEBUG("tm_set_notify_event_callback");
	
	_p_param->_result = dm_set_notify_event_callback(_p_param->_para1);

#ifdef ENABLE_EMULE
	_p_param->_result = emule_set_notify_event_callback(_p_param->_para1);
#endif
	
	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 tm_get_assist_task_info(void * _param)
{
	TM_POST_PARA_2 * p_param = (TM_POST_PARA_2*)_param;
	_u32 task_id = (_u32)p_param->_para1;
	void * p_assist_task_info = (void *)p_param->_para2;
	
	TASK *p_task = NULL;

	p_param->_result = tm_get_task_by_id(task_id, &p_task);

	if(p_param->_result == SUCCESS && p_task != NULL)
	{
		p_param->_result = dt_get_assist_task_info_impl( p_task, p_assist_task_info);
	}
		
	LOG_DEBUG("tm_get_assist_task_info result = %d", p_param->_result);
	return signal_sevent_handle(&(p_param->_handle));
}

_int32 tm_set_etm_scheduler_event(void *_param)
{
	TM_POST_PARA_1* _p_param = (TM_POST_PARA_1*)_param;

	LOG_DEBUG("tm_set_etm_scheduler_event");
	
	_p_param->_result = dm_set_notify_etm_scheduler(_p_param->_para1);

	
	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));    
}

_int32 tm_update_task_list(void *_param)
{
	TM_POST_PARA_0* _p_param = (TM_POST_PARA_0*)_param;
	LOG_DEBUG("tm_update_task_list");	
	tm_update_task_info();
	_p_param->_result = SUCCESS;
	LOG_DEBUG("tm_update_task_list signal_sevent_handle:_result=%d", _p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));    
}

_int32 tm_set_recv_data_from_assist_pc_only(void * _param)
{
	TM_POST_PARA_3 * p_param = (TM_POST_PARA_3*)_param;
	_u32 task_id = (_u32)p_param->_para1;
	void * p_resource = (void*)p_param->_para2;
	void * p_task_prop = (void*)p_param->_para3;
	
	TASK *p_task = NULL;

	p_param->_result = tm_get_task_by_id(task_id, &p_task);
	
	LOG_DEBUG("tm_set_recv_data_from_assist_pc_only: p_task= 0x%x, p_resource= 0x%x, p_task_prop= 0x%x", p_task, p_resource, p_task_prop);

	if(p_param->_result == SUCCESS && p_task != NULL)
	{
		p_param->_result = dt_set_recv_data_from_assist_pc_only_impl( p_task, p_resource, p_task_prop);
	}
		
	LOG_DEBUG("tm_set_recv_data_from_assist_pc_only result = %d", p_param->_result);
	return signal_sevent_handle(&(p_param->_handle));
}

BOOL tm_in_downloading()
{
    return  list_size(&g_pTask_manager->_task_list) == 0 ? FALSE : TRUE ;
}

_int32 tm_get_network_flow(void *_param)
{
    TM_POST_PARA_2 *p_param = (TM_POST_PARA_2*)_param;
    _u32 *p_download = (_u32*)p_param->_para1;
    _u32 *p_up = (_u32*)p_param->_para2;
    p_param->_result = get_network_flow(p_download, p_up);
    LOG_DEBUG("tm_get_network_flow, download(%d), up(%d)", *p_download, *p_up);
    return signal_sevent_handle(&(p_param->_handle));
}

#if defined(ENABLE_HSC) && !defined(ENABLE_ETM_MEDIA_CENTER)
_int32 tm_hsc_set_user_info(void * _param)
{
    TM_POST_PARA_3 * p_param = (TM_POST_PARA_3*)_param;
    _u64 *p_userid = (_u64*)p_param->_para1;
	_u64 userid = *p_userid;
	char * p_jumpkey = (char*)p_param->_para2;
	_u32 jumpkey_len = (_u32)p_param->_para3;

	p_param->_result = hsc_set_user_info(userid, p_jumpkey, jumpkey_len);
		
	LOG_DEBUG("tm_hsc_set_user_info result = %d", p_param->_result);
	return signal_sevent_handle(&(p_param->_handle));
}
#endif

#ifdef UPLOAD_ENABLE
_int32 tm_ulm_del_record_by_full_file_path(void *_param)
{
    TM_POST_PARA_2 *p_param = (TM_POST_PARA_2*)_param;
    _u64 *p_size = (_u64*)p_param->_para1;
    char * path = (char*)p_param->_para2;
    _u64 size = *p_size;
    p_param->_result = ulm_del_record_by_full_file_path(size, path);

    LOG_DEBUG("tm_ulm_del_record_by_full_file_path result = %d", p_param->_result);

    SAFE_DELETE(path);
    SAFE_DELETE(p_size);
    SAFE_DELETE(p_param);
    return SUCCESS;
}

_int32 tm_ulm_change_file_path(void *_param)
{
    TM_POST_PARA_3 *p_param = (TM_POST_PARA_3*)_param;
    _u64 *p_size = (_u64*)p_param->_para1;
    char * old_path = (char*)p_param->_para2;
    char *new_path = (char*)p_param->_para3;
    p_param->_result = ulm_change_file_path(*p_size, old_path, new_path);

    LOG_DEBUG("tm_ulm_change_file_path result = %d", p_param->_result);

    SAFE_DELETE(old_path);
    SAFE_DELETE(new_path);
    SAFE_DELETE(p_size);
    SAFE_DELETE(p_param);
    
    return SUCCESS;
}
#endif

_int32 tm_start_http_server(void *_param)
{
    HTTP_SERVER_START* _p_param = (HTTP_SERVER_START*)_param;
	_p_param->_result = http_server_start_handle(_p_param->_port);
	LOG_DEBUG("signal_sevent_handle:_result=%d", _p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 tm_stop_http_server(void *_param)
{
    HTTP_SERVER_STOP* _p_param = (HTTP_SERVER_STOP*)_param;
    _p_param->_result = http_server_stop_handle();
    LOG_DEBUG("signal_sevent_handle:_result=%d", _p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 tm_set_host_ip(void *_param)
{
    TM_POST_PARA_2* _p_param = (TM_POST_PARA_2*)_param;
	const char* host_name = _p_param->_para1;
	const char* ip_str = _p_param->_para2;
  	_p_param->_result = dns_add_domain_ip(host_name, ip_str);

	LOG_DEBUG("signal_sevent_handle:_result=%d", _p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 tm_clear_host_ip(void *_param)
{
    TM_POST_PARA_0* _p_param = (TM_POST_PARA_0*)_param;
    _p_param->_result = dns_clear_domain_map();
    LOG_DEBUG("signal_sevent_handle:_result=%d", _p_param->_result);
    return signal_sevent_handle(&(_p_param->_handle));
}

_int32 tm_set_original_url(void *param)
{
    TM_ORIGINAL_URL_INFO *p_param = (TM_ORIGINAL_URL_INFO*)param;
    TASK *p_task;
    P2SP_TASK *p_p2sptask;
    _int32 ret;

    p_param->_result = tm_get_task_by_id(p_param->_task_id, &p_task);

    if(SUCCESS == p_param->_result)
    {
        if(p_task->_task_type != P2SP_TASK_TYPE)
        {
            sd_assert(FALSE);
            p_param->_result = TM_ERR_TASK_TYPE;
            return signal_sevent_handle(&p_param->_handle);
        }
        p_p2sptask = (P2SP_TASK *)p_task;
        p_param->_result = dm_set_file_info(&p_p2sptask->_data_manager, NULL, NULL, p_param->_orignal_url, p_param->_ref_url);
    }

    return signal_sevent_handle(&p_param->_handle);
}

_int32 tm_add_cdn_peer_resource(void* _param)
{
	TM_ADD_CDN_PEER_RESOURCE* _p_param = (TM_ADD_CDN_PEER_RESOURCE*)_param;
	TASK* _p_task = NULL;
	LOG_DEBUG("tm_add_cdn_peer_resource");

	_p_param->_result = tm_get_task_by_id(_p_param->_task_id,&_p_task);
	if(SUCCESS != _p_param->_result)
	{
		return signal_sevent_handle(&(_p_param->_handle));
	}

	if(TASK_S_RUNNING != _p_task->task_status)
	{
		_p_param->_result = TM_ERR_TASK_NOT_RUNNING;
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	_p_param->_result = cm_add_cdn_peer_resource( &(_p_task->_connect_manager), 
			_p_param->_file_index, 
			_p_param->_peer_id, 
			_p_param->_gcid, 
			_p_param->_file_size, 
			_p_param->_peer_capability,  
			_p_param->_host_ip, 
			_p_param->_tcp_port , 
			_p_param->_udp_port, 
			_p_param->_res_level, 
			_p_param->_res_from );
	return signal_sevent_handle(&(_p_param->_handle));
}

_int32 tm_add_lixian_server_resource(void* _param)
{
	TM_ADD_LX_RESOURCE* _p_param = (TM_ADD_LX_RESOURCE*)_param;
	TASK* _p_task = NULL;
	LOG_DEBUG("tm_add_lixian_server_resource");

	_p_param->_result = tm_get_task_by_id(_p_param->_task_id,&_p_task);
	if(SUCCESS != _p_param->_result)
	{
		return signal_sevent_handle(&(_p_param->_handle));
	}

	if(TASK_S_RUNNING != _p_task->task_status)
	{
		_p_param->_result = TM_ERR_TASK_NOT_RUNNING;
		return signal_sevent_handle(&(_p_param->_handle));
	}
#ifdef ENABLE_LIXIAN	
	_p_param->_result = cm_add_lixian_server_resource(&(_p_task->_connect_manager), 
			_p_param->_file_index, 
			_p_param->_url, 
			_p_param->_url_size, 
			_p_param->_ref_url, 
			_p_param->_ref_url_size, 
			_p_param->_cookie, 
			_p_param->_cookie_size, 
			_p_param->_resource_priority);
#else
	_p_param->_result = NOT_IMPLEMENT;
#endif
	return signal_sevent_handle(&(_p_param->_handle));

}

_int32 tm_get_bt_acc_file_index(void *param)
{
	TM_BT_ACC_FILE_INDEX *p_param = (TM_BT_ACC_FILE_INDEX*)param;
	TASK *p_task;
    
	p_param->_result = tm_get_task_by_id(p_param->_task_id, &p_task);
	if(SUCCESS == p_param->_result)
	{
		if(p_task->_task_type != BT_TASK_TYPE)
		{
			sd_assert(FALSE);
            p_param->_result = TM_ERR_TASK_TYPE;
		}
        else
        {
		    p_param->_result = cm_get_bt_acc_file_idx_list(&p_task->_connect_manager, p_param->_file_index);
        }
	}

	return signal_sevent_handle(&p_param->_handle);
}

_int32  tm_get_bt_task_torrent_info( void * _param)
{
#ifdef ENABLE_BT
	TM_GET_BT_TASK_TORRENT_INFO* _p_param = (TM_GET_BT_TASK_TORRENT_INFO*)_param;
	TM_LX_TORRENT_SEED_INFO *p_torrent_info = (TM_LX_TORRENT_SEED_INFO*)_p_param->torrent_info;
	TASK *_task_info;
	BT_TASK *p_bt_task;
	MAP_ITERATOR map_cur = NULL, map_end = NULL;
	_int32 i = 0;

	LOG_DEBUG("tm_get_bt_task_torrent_info,task:%d", _p_param->task_id);

	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("tm_get_bt_task_torrent_info:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}

	_p_param->_result = tm_get_task_by_id(_p_param->task_id, &_task_info);
	if(_p_param->_result != SUCCESS)
	{
		return signal_sevent_handle(&(_p_param->_handle));
	}
	if(_task_info->task_status != TASK_S_RUNNING)
	{
		_p_param->_result = TM_ERR_TASK_IS_NOT_READY;
		return signal_sevent_handle(&(_p_param->_handle));
	}
	if(_task_info->_task_type==BT_TASK_TYPE)
	{
		p_bt_task = (BT_TASK *)_task_info;
		if( p_bt_task->_is_magnet_task && p_bt_task->_magnet_task_ptr)
		{
			_p_param->_result = TM_ERR_TASK_IS_NOT_READY;
		}
	}
	else
	{
		_p_param->_result = TM_ERR_TASK_TYPE;
	}
	
	if (SUCCESS == _p_param->_result)
	{
		sd_memcpy((void *)p_torrent_info->_title_name, (void *)p_bt_task->_torrent_parser_ptr->_title_name, p_bt_task->_torrent_parser_ptr->_title_name_len);
		p_torrent_info->_title_name_len= p_bt_task->_torrent_parser_ptr->_title_name_len; 
		p_torrent_info->_file_total_size= p_bt_task->_torrent_parser_ptr->_file_total_size;

		p_torrent_info->_file_num= p_bt_task->_torrent_parser_ptr->_file_num;

		p_torrent_info->_encoding= p_bt_task->_torrent_parser_ptr->_encoding;
		sd_memcpy(p_torrent_info->_info_hash, p_bt_task->_torrent_parser_ptr->_info_hash, sizeof(p_torrent_info->_info_hash));
		p_torrent_info->_piece_size = p_bt_task->_torrent_parser_ptr->_piece_length;
		p_torrent_info->_piece_hash_ptr = p_bt_task->_torrent_parser_ptr->_piece_hash_ptr;
		p_torrent_info->_piece_hash_len = p_bt_task->_torrent_parser_ptr->_piece_hash_len;
		p_torrent_info->_file_info_num = map_size(&p_bt_task->_file_info_map);
		_p_param->_result = sd_malloc(sizeof(_u32)*p_torrent_info->_file_info_num, (void**)&p_torrent_info->_file_info_buf);
		CHECK_VALUE(_p_param->_result);
		map_cur = MAP_BEGIN(p_bt_task->_file_info_map);
		map_end = MAP_END(p_bt_task->_file_info_map);
		while(map_cur != map_end)
		{
			p_torrent_info->_file_info_buf[i++] = (_u32)MAP_KEY(map_cur);
			map_cur = MAP_NEXT(p_bt_task->_file_info_map, map_cur);
		}
	}

	LOG_DEBUG("tm_get_bt_task_torrent_info:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
#else
	return -1;
#endif
}


_int32 tm_get_bt_file_index_info(void *_param)
{
#ifdef ENABLE_BT
	TM_BT_FILE_INDEX_INFO *p_param = (TM_BT_FILE_INDEX_INFO*)_param;
	_u32 file_index = p_param->_file_index;
	TASK *p_task;
	BT_TASK *p_bt_task;

    p_param->_result = tm_get_task_by_id(p_param->_task_id, &p_task);
    if(SUCCESS == p_param->_result)
    {
		if(p_task->_task_type != BT_TASK_TYPE)
        {
            p_param->_result = TM_ERR_TASK_TYPE;
			return signal_sevent_handle(&p_param->_handle);
        }
		p_bt_task = (BT_TASK *)p_task;

		if(bdm_get_cid(&(p_bt_task->_data_manager), file_index, (_u8 *)p_param->_cid) != TRUE)
		{
		       p_param->_result = TM_ERR_INVALID_TCID;
		}
		*(p_param->_file_size) = bdm_get_sub_file_size(&(p_bt_task->_data_manager),file_index);
		if( bdm_get_shub_gcid( &(p_bt_task->_data_manager),file_index, (_u8 *)p_param->_gcid)!=TRUE)
		{
		       p_param->_result = TM_ERR_INVALID_TCID;
		}
    }

    return signal_sevent_handle(&p_param->_handle);
#else
	return -1;
#endif 
}

_int32 tm_check_alive(void *_param)
{
    TM_POST_PARA_0 *p_param = (TM_POST_PARA_0 *)_param;
    p_param->_result = SUCCESS;
    return signal_sevent_handle(&p_param->_handle);
}

_int32 tm_set_task_dispatch_mode( void * _param )
{
	TM_POST_PARA_3* _p_param = (TM_POST_PARA_3*)_param;
	_u32 task_id = (_u32)_p_param->_para1;
	TASK_DISPATCH_MODE mode = (BOOL)_p_param->_para2;
	_u64 filesize = *((_u64 *)_p_param->_para3);
	
	TASK * _p_task = NULL;

	LOG_DEBUG("tm_set_task_dispatch_mode task_id:%u, mode:%d, filesize:%u", task_id, mode, filesize);

	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}
	
	_p_param->_result = tm_get_task_by_id(task_id, &_p_task);
	if(_p_param->_result != SUCCESS) goto ErrorHanle;
	
	
	if(_p_task->task_status != TASK_S_RUNNING)
	{
		_p_param->_result = TM_ERR_TASK_NOT_RUNNING;
		goto ErrorHanle;
	}

	_p_param->_result = pt_set_task_dispatch_mode(_p_task, mode, filesize);

ErrorHanle:
	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));

}


_int32 tm_get_download_bytes(void* param)
{
    TM_POST_PARA_3* _p_param = (TM_POST_PARA_3*)param;
    _u32 task_id = (_u32)_p_param->_para1;
    char* host = (char*)_p_param->_para2;
    _u64* download = (_u64*)_p_param->_para3;
    TASK * _p_task = NULL;

    LOG_DEBUG("tm_get_download_bytes task_id:%u, host: %s", task_id, host);
    
    if(g_pTask_manager==NULL)
    {
        _p_param->_result = -1;
        goto ErrorHanle;
    }

    _p_param->_result = tm_get_task_by_id(task_id, &_p_task);
    if(_p_param->_result != SUCCESS) 
    {
        goto ErrorHanle;
    }

    if(_p_task->_task_type != P2SP_TASK_TYPE)
    {
        _p_param->_result = DT_ERR_INVALID_TASK_TYPE;
        goto ErrorHanle;
    }

    _p_param->_result = pt_get_download_bytes(_p_task, host, download);

ErrorHanle:
    LOG_DEBUG("signal_sevent_handle:_result = %d",_p_param->_result);
    return signal_sevent_handle(&(_p_param->_handle));

}

_int32 tm_set_module_option(void* param)
{
    TM_POST_PARA_1* _p_param = (TM_POST_PARA_1*)param;
    _u32 p_options = (_u32)_p_param->_para1;
	_u32 i = 0;

    LOG_DEBUG("tm_set_module_option options:%u", p_options);
    
    if(g_pTask_manager==NULL)
    {
        _p_param->_result = -1;
        goto ErrorHanle;
    }

	while(i < g_module_set_hook_num) {
		if(g_handle_module_set_hook[i].register_func&& (p_options & (0x0001<<i)))
			g_handle_module_set_hook[i].enable = 1;
		i++;
	}

	if (g_pTask_manager->_global_statistic._cur_downloadtask_num == 0)
	{
		TM_RUN_DOWNTASK_HOOKS(0);
	}

    _p_param->_result = SUCCESS;

ErrorHanle:
    LOG_DEBUG("tm_set_module_option:_result = %d",_p_param->_result);
    return signal_sevent_handle(&(_p_param->_handle));

}

_int32 tm_get_module_option(void* param)
{
    TM_POST_PARA_1* _p_param = (TM_POST_PARA_1*)param;
    _u32 *p_options = (_u32*)_p_param->_para1;
	_u32 i = 0;

	while(i < g_module_set_hook_num) {
			*p_options += g_handle_module_set_hook[i].enable << i;
			i++;
	}
    _p_param->_result = SUCCESS;

ErrorHanle:
    LOG_DEBUG("tm_get_module_option:options = %d",*p_options);
    return signal_sevent_handle(&(_p_param->_handle));
}

_int32 tm_get_info_from_cfg_file(void * _param)
{
	TM_POST_PARA_9* _p_param = (TM_POST_PARA_9*)_param;
	char* cfg_file_path = (char *)_p_param->_para1;
	char* origin_url = (char *)_p_param->_para2;
	BOOL* cid_is_valid = (BOOL *)_p_param->_para3;
	_u8* cid = (_u8 *)_p_param->_para4;
	_u64* file_size = (_u64 *)_p_param->_para5;
	char* file_name = (char *)_p_param->_para6;
	char* lixian_url = (char *)_p_param->_para7;
	char* cookie = (char *)_p_param->_para8;
	char* user_data = (char *)_p_param->_para9;

	LOG_DEBUG("tm_get_info_from_cfg_file,%x,%x", cfg_file_path, origin_url);

	if(g_pTask_manager==NULL)
	{
		_p_param->_result = -1;
		LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
		return signal_sevent_handle(&(_p_param->_handle));
	}

	_p_param->_result = dm_get_info_from_cfg_file(cfg_file_path, origin_url, cid_is_valid, cid, file_size, file_name, lixian_url, cookie, user_data);
	
	LOG_DEBUG("signal_sevent_handle:_result=%d",_p_param->_result);
	return signal_sevent_handle(&(_p_param->_handle));
}

