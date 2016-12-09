#include "utility/define.h"
#include "platform/sd_fs.h"
#include "utility/mempool.h"
#include "utility/utility.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_EMULE

#ifdef ENABLE_EMULE
#include "utility/logger.h"
#include "utility/settings.h"

#include "emule_interface.h"
#include "emule_impl.h"
#include "emule_task_manager.h"
#include "emule_data_manager/emule_data_manager.h"
#include "emule_pipe/emule_pipe_function_table.h"
#include "emule_pipe/emule_pipe_wait_accept.h"
#include "emule_query/emule_query_emule_info.h"
#include "emule_query/emule_query_emule_tracker.h"
#include "emule_utility/emule_ed2k_link.h"
#include "emule_utility/emule_memory_slab.h"
#include "emule_utility/emule_udp_device.h"
#include "emule_utility/emule_setting.h"
#include "emule_utility/emule_credit.h"
#include "emule_utility/emule_utility.h"
#include "emule_server/emule_server_interface.h"
#include "emule_transfer_layer/emule_accept.h"
#include "emule_transfer_layer/emule_udt_device_pool.h"
#include "emule_transfer_layer/emule_nat_server.h"
#include "res_query/res_query_interface.h"
#include "emule_pipe/emule_pipe_device.h"
#include "task_manager/task_manager.h"
#ifdef UPLOAD_ENABLE
#include "upload_manager/upload_manager.h"
#endif
#include "reporter/reporter_interface.h"
static BOOL g_enable_emule_download = TRUE;
#ifdef ENABLE_HSC
#include "high_speed_channel/hsc_info.h"
#endif /* ENABLE_HSC */
_int32 emule_init_modular(void)
{
	_int32 ret = SUCCESS;
	LOG_DEBUG("emule_init_modular.");

	settings_get_bool_item( "emule.enable_bt_download", &g_enable_emule_download );

    ret = emule_init_memory_slab();
	CHECK_VALUE(ret);
	//create_udp_device和emule_create_acceptor一定要在emule_init_local_peer()之前，函数会填充tcp_port,udp_port字段
#ifdef ENABLE_EMULE_PROTOCOL

	ret = emule_create_udp_device();		//会填充local_peer的udp_port
	CHECK_VALUE(ret);

    ret = emule_create_acceptor();			//会填充local_peer的tcp_port
	CHECK_VALUE(ret);
    
	ret = emule_init_local_peer();
	CHECK_VALUE(ret);
#endif
    
	ret = emule_init_download_queue();
	CHECK_VALUE(ret);
    
#ifdef ENABLE_EMULE_PROTOCOL
	ret = emule_udp_recvfrom();
	CHECK_VALUE(ret);
	if(emule_enable_server() == TRUE)
	{
		ret = emule_server_start();
		CHECK_VALUE(ret);
	}
#endif
    
	ret = emule_init_task_manager();
	CHECK_VALUE(ret);
    
#ifdef ENABLE_EMULE_PROTOCOL
	emule_init_wait_accept_pipe();
	emule_udt_device_pool_init();
	ret = emule_nat_server_init();
	CHECK_VALUE(ret);
    
//	ret = emule_create_private_key();
//	CHECK_VALUE(ret);

    emule_pipe_device_module_init();
#endif

#ifdef _DK_QUERY
	if(emule_enable_kad())
		res_query_register_kad_res_handler(emule_kad_notify_find_sources);
#endif
    return ret;
}

_int32 emule_uninit_modular(void)
{
	_int32 ret = SUCCESS;
	LOG_DEBUG("emule_uninit_modular.");
    
#ifdef ENABLE_EMULE_PROTOCOL
	if(emule_enable_server() == TRUE)
	{
		ret = emule_server_stop();
		CHECK_VALUE(ret);
	}   

    emule_pipe_device_module_uninit();
	emule_uninit_download_queue();
    
	emule_udt_device_pool_uninit();
	emule_uninit_wait_accept_pipe();
	ret = emule_close_udp_device();
	CHECK_VALUE(ret);

	ret = emule_uninit_local_peer();
	CHECK_VALUE(ret);
#endif

	ret = emule_uninit_memory_slab();
	CHECK_VALUE(ret);
 
	return ret;
}

_int32 emule_create_task(TM_EMULE_PARAM* param, TASK** task)
{
	_int32 ret = SUCCESS;
	EMULE_TASK* emule_task = NULL;
	EMULE_TASK* emule_tmp_task = NULL;
	ED2K_LINK_INFO ed2k_info;
    LIST_ITERATOR cur_node = NULL;
    EMULE_PEER_SOURCE *source = NULL;
	DS_DATA_INTEREFACE dispatch_interface;

	LOG_DEBUG("emule_create_task, ed2k_link = %s, file_path = %s.", param->_ed2k_link, param->_file_path);
       *task = NULL;
  
    if( !g_enable_emule_download ) return EMULE_DOWNLOAD_DISABLE;

	if(sd_file_exist(param->_file_path) == FALSE)
	  	return DT_ERR_INVALID_FILE_PATH;
	ret = sd_malloc(sizeof(EMULE_TASK), (void**)&emule_task);
	CHECK_VALUE(ret);
	sd_memset(emule_task, 0, sizeof(EMULE_TASK));
	ret = init_task(&emule_task->_task, EMULE_TASK_TYPE);
	if(ret != SUCCESS)
	{
		sd_free(emule_task);
		emule_task = NULL;
		return ret;
	}
	list_init(&emule_task->_peer_source_list);
	ret = emule_extract_ed2k_link(param->_ed2k_link, &ed2k_info);
	if(ret != SUCCESS)
	{
		sd_free(emule_task);
		emule_task = NULL;
		return EMULE_ED2K_LINK_ERR;
	}
    if( sd_strlen(param->_file_name) != 0 )
    {
        sd_memcpy(ed2k_info._file_name, param->_file_name, MAX_FILE_NAME_LEN + 1);
    }
    
    emule_tmp_task = emule_find_task(ed2k_info._file_id);
    if(emule_tmp_task!=NULL)
    {
        cur_node = LIST_BEGIN( ed2k_info._source_list );
        while( cur_node != LIST_END(ed2k_info._source_list) )
        {
            source = (EMULE_PEER_SOURCE *)LIST_VALUE(cur_node);
            sd_free(source);
			source = NULL;
            cur_node = LIST_NEXT( cur_node );
        }
		SAFE_DELETE(ed2k_info._part_hash);
        list_clear(&ed2k_info._source_list);
		sd_free(emule_task);
		emule_task = NULL;
        return DT_ERR_ORIGIN_RESOURCE_EXIST;
    }
	list_swap(&emule_task->_peer_source_list, &ed2k_info._source_list);
	ret = emule_create_data_manager(&emule_task->_data_manager, &ed2k_info, param->_file_path, emule_task);
	if(ret != SUCCESS)
	{
		SAFE_DELETE(ed2k_info._part_hash);
		sd_free(emule_task);
		emule_task = NULL;
		return ret;
	}
	//如果存在part_hash，则通知part_hash已找到
	if(ed2k_info._part_hash != NULL)
	{
		emule_notify_get_part_hash(emule_task, ed2k_info._part_hash, ed2k_info._part_hash_size);
		sd_free(ed2k_info._part_hash);
		ed2k_info._part_hash = NULL;
	}
	emule_task->_task._connect_manager._main_task_ptr = &emule_task->_task;
	cm_init_emule_connect_manager(&emule_task->_task._connect_manager, emule_task->_data_manager, 
        emule_can_abandon_resource, emule_get_pipe_function_table());

	//如果存在http_source,则通知connect_manager加资源
	if(sd_strlen(ed2k_info._http_url) > 0)
	{
		ret = cm_add_server_resource(&emule_task->_task._connect_manager, 0, ed2k_info._http_url, sd_strlen(ed2k_info._http_url), NULL, 0,0);
		CHECK_VALUE(ret);
	}

    emule_set_dispatch_interface(emule_task->_data_manager, &dispatch_interface);

    ret = ds_init_dispatcher(&emule_task->_task._dispatcher, &dispatch_interface, &emule_task->_task._connect_manager);
	CHECK_VALUE(ret);
    
    emule_task->_query_param._file_index = 0;
    emule_task->_query_param._task_ptr = (TASK*)emule_task;    
    
    emule_task->_last_query_stamp = 0;
    emule_task->_last_query_emule_tracker_stamp = 0;
    emule_task->_query_shub_times_sn = 0;
    emule_task->_res_query_shub_retry_count = 0;
    
    emule_task->_res_query_emulehub_state = RES_QUERY_IDLE;
    emule_task->_res_query_shub_state = RES_QUERY_IDLE;
    emule_task->_res_query_phub_state = RES_QUERY_IDLE;
    emule_task->_res_query_tracker_state = RES_QUERY_IDLE;
    emule_task->_res_query_partner_cdn_state = RES_QUERY_IDLE; 
    emule_task->_res_query_dphub_state = RES_QUERY_IDLE;
	emule_task->_res_query_emule_tracker_state = RES_QUERY_IDLE;
#ifdef ENABLE_HSC
	emule_task->_res_query_vip_hub_state = RES_QUERY_IDLE;
#endif /* ENABLE_HSC */
	emule_task->_res_query_normal_cdn_state = RES_QUERY_IDLE;
	emule_task->_res_query_normal_cdn_cnt = 0;
    emule_task->_is_need_report = FALSE;

    init_dphub_query_context(&emule_task->_dpub_query_context);

    emule_task->_requery_emule_tracker_timer = INVALID_MSG_ID;

	*task = (TASK*)emule_task;

#if defined(ENABLE_HSC) && !defined(ENABLE_ETM_MEDIA_CENTER)
	hsc_init_info(&emule_task->_task._hsc_info);
#endif

	LOG_DEBUG("emule_create_task SUCCESS, task = 0x%x, ed2k_link = %s, file_path = %s.", emule_task, param->_ed2k_link, param->_file_path);
	return SUCCESS;
}

_int32 emule_start_task(TASK* task)
{
	_int32 ret = SUCCESS;
	EMULE_TASK* emule_task = (EMULE_TASK*)task;
	EMULE_DATA_MANAGER* data_manager = emule_task->_data_manager;
	EMULE_PEER_SOURCE* source = NULL;
	LOG_DEBUG("emule_start_task, task = 0x%x. _is_file_exist:%d", emule_task, data_manager->_is_file_exist);
	sd_assert(task->_task_type == EMULE_TASK_TYPE);
	emule_add_task(emule_task);
	
	emule_task->_res_query_normal_cdn_state = RES_QUERY_IDLE;
	emule_task->_res_query_normal_cdn_cnt = 0;
	ret = start_task(task, (struct _tag_data_manager*)emule_task->_data_manager, emule_can_abandon_resource, emule_get_pipe_function_table());
	if(ret != SUCCESS)
		return ret;
    if(data_manager->_is_file_exist)
    {
        return emule_notify_task_success(emule_task);
    }
	if(emule_enable_p2sp() && emule_task->_res_query_emulehub_state != RES_QUERY_REQING
     && emule_task->_data_manager->_is_cid_and_gcid_valid == FALSE )
		emule_query_emule_info(emule_task, data_manager->_file_id, data_manager->_file_size);
#ifdef _DK_QUERY
    if(emule_enable_kad())
	{
		res_query_kad(emule_task, data_manager->_file_id, data_manager->_file_size);
		//CHECK_VALUE(ret);
	}
#endif

#ifdef ENABLE_EMULE_PROTOCOL
	emule_server_query_source(data_manager->_file_id, data_manager->_file_size);
#endif

    emule_task_query_emule_tracker(emule_task);

	while(list_size(&emule_task->_peer_source_list) > 0)
	{
		list_pop(&emule_task->_peer_source_list, (void**)&source);
		ret = cm_add_emule_resource(&emule_task->_task._connect_manager, source->_ip, source->_port, 0, 0);
		//CHECK_VALUE(ret);
		sd_free(source);
		source = NULL;
	}
	return ret;
}

/*--------------------------------------------------------------------------*/
/*                Internal functions				        */
/*--------------------------------------------------------------------------*/
#ifdef UPLOAD_ENABLE
extern _int32 emule_get_file_name(EMULE_DATA_MANAGER* p_data_manager_interface, char ** file_name);
extern _int32 emule_get_file_path(EMULE_DATA_MANAGER* p_data_manager_interface,  char ** file_path);
extern BOOL emule_get_shub_cid(void* data_manager, _u8 cid[CID_SIZE]);

_int32 emule_add_record_to_upload_manager( TASK * p_task )
{
	EMULE_TASK* emule_task = (EMULE_TASK*)p_task;
	char file_name_buffer[MAX_FILE_PATH_LEN],*file_path=NULL,*file_name=NULL;
	_u8 cid[CID_SIZE]={0};
	_u8 gcid[CID_SIZE]={0};

	LOG_DEBUG("emule_add_record_to_upload_manager");

	if(p_task->file_size<MIN_UPLOAD_FILE_SIZE) return SUCCESS;
	//if(PIPE_FILE_HTML_TYPE==emule_get_file_type( &(emule_task->_data_manager )))
	//{
	//	return SUCCESS;
	//}

	EMULE_DATA_MANAGER* p_data_manager = emule_task->_data_manager;
	if((TRUE == emule_get_file_name( p_data_manager,  &file_name))
		&&(TRUE == emule_get_file_path(  p_data_manager, &file_path)))
	{
		if(sd_strlen(file_path)+sd_strlen(file_name)<MAX_FILE_PATH_LEN)
		{
			sd_memset(file_name_buffer,0,MAX_FILE_PATH_LEN);
			sd_snprintf(file_name_buffer, MAX_FILE_PATH_LEN,"%s%s", 
				file_path, file_name);
			LOG_DEBUG("[emule_add_record_to_upload_manager] file_path + file_name less MAX_LEN.");

			BOOL b_can_get_cid = FALSE;
			BOOL b_can_get_gcid = FALSE;
			b_can_get_cid = emule_get_shub_cid(p_data_manager, cid);
			if (FALSE == b_can_get_cid) {
                LOG_DEBUG("[b_can_get_cid FALSE]");
				b_can_get_cid = emule_get_cid(p_data_manager, cid);
			}

			b_can_get_gcid = emule_get_shub_gcid(p_data_manager, gcid);
			if (FALSE == b_can_get_gcid) {
                LOG_DEBUG("[b_can_get_gcid FALSE]");
				b_can_get_gcid = emule_get_calc_gcid(p_data_manager, gcid);
			}

            LOG_DEBUG("[b_can_get_cid = %d, b_can_get_gcid = %d.]", 
                b_can_get_cid, b_can_get_gcid);
			
			if((TRUE == b_can_get_cid) && (TRUE == b_can_get_gcid))
			{
				LOG_DEBUG("[emule_add_record_to_upload_manager] cid = %s gcid = %s.", 
					cid, gcid);
				if(SUCCESS ==ulm_add_record(p_task->file_size, cid, gcid, NORMAL_HUB, file_name_buffer))
				{
					emule_task->_is_add_rc_ok = TRUE;
				}
			}
		}
	}

	return SUCCESS;
}
#endif

_int32 emule_cancel_emule_tracker_timer(TASK *task)
{
    _int32 ret = SUCCESS;
    EMULE_TASK *emule_task = (EMULE_TASK *)task;
    if (emule_task && (emule_task->_requery_emule_tracker_timer != INVALID_MSG_ID))
    {
        ret = cancel_timer(emule_task->_requery_emule_tracker_timer);
        CHECK_VALUE(ret);    
        emule_task->_requery_emule_tracker_timer = INVALID_MSG_ID;
    }
    return ret;
}

_int32 emule_stop_task(TASK* task)
{
#ifdef EMBED_REPORT
    VOD_REPORT_PARA report_para;
#endif
    EMULE_TASK*emule_task= (EMULE_TASK*)task;
	LOG_DEBUG("[emule_task = 0x%x]emule_stop_task, dhub_state(%d).", task, 
        emule_task->_res_query_dphub_state);
	if(task->task_status == TASK_S_STOPPED)
		CHECK_VALUE(DT_ERR_TASK_NOT_RUNNING);
    
#ifdef _DK_QUERY
	res_query_cancel(task, EMULE_KAD);
#endif   
	
    if( emule_task->_res_query_emulehub_state == RES_QUERY_REQING )
    {
        res_query_cancel(emule_task, EMULE_HUB);
    }   
    if( emule_task->_res_query_shub_state == RES_QUERY_REQING )
    {
        res_query_cancel(&emule_task->_query_param, SHUB);
    }
    if( emule_task->_res_query_phub_state == RES_QUERY_REQING )
    {
        res_query_cancel(&emule_task->_query_param, PHUB);
    }
    // 由于dphub的查询不是一次性的，几乎会同时向多个节点发起查询，所以此时的查询状态
    // 就不能统一，为此可能造成有些节点未能取消的bug，所以在这里不论状态全部取消一次
    //if (emule_task->_res_query_dphub_state != RES_QUERY_IDLE || emule_task->_res_query_dphub_state != RES_QUERY_END)
    {
        res_query_cancel(&emule_task->_query_param, DPHUB_NODE);
    }
    if( emule_task->_res_query_tracker_state == RES_QUERY_REQING )
    {
        res_query_cancel(&emule_task->_query_param, TRACKER);
    }
    if( emule_task->_res_query_partner_cdn_state == RES_QUERY_REQING )
    {
#if defined(MACOS) && defined(ENABLE_CDN)
        res_query_cancel(&emule_task->_query_param, CDN_MANAGER);
#else
        res_query_cancel(&emule_task->_query_param, PARTNER_CDN);
#endif 
    }
#ifdef ENABLE_HSC
	if(emule_task->_res_query_vip_hub_state == RES_QUERY_REQING)
	{
		res_query_cancel(&emule_task->_query_param, VIP_HUB);
	}
#endif /* ENABLE_HSC */
	if(emule_task->_res_query_normal_cdn_state == RES_QUERY_REQING)
	{
		res_query_cancel(&emule_task->_query_param, NORMAL_CDN_MANAGER);
	}
    if (emule_task->_res_query_emule_tracker_state == RES_QUERY_REQING)
    {
        res_query_cancel(&emule_task->_query_param, EMULE_TRACKER);
    }

#ifdef UPLOAD_ENABLE
		emule_add_record_to_upload_manager( task );
#endif

#ifdef EMBED_REPORT
    if(task->task_status == TASK_S_RUNNING)
    {
        sd_memset(&report_para, 0, sizeof(VOD_REPORT_PARA));
        emb_reporter_emule_stop_task(task, &report_para, TASK_S_RUNNING );
    }
    reporter_task_emule_stat(task);
#endif /* EMBED_REPORT */
   

    emule_cancel_emule_tracker_timer(task);

    uninit_dphub_query_context(&emule_task->_dpub_query_context);

    emule_remove_task((EMULE_TASK*)task);
	stop_task(task);
	return SUCCESS;
}

_int32 emule_delete_task(TASK* task)
{
	_int32 ret = SUCCESS;
	EMULE_TASK* emule_task = (EMULE_TASK*)task;
	LOG_DEBUG("[emule_task = 0x%x]emule_delete_task.", task);
	if(emule_task->_data_manager != NULL)
	{
       ret = emule_close_data_manager(emule_task->_data_manager);
	}
    if( ret == TM_ERR_WAIT_FOR_SIGNAL ) return ret;

    delete_task(task);
    sd_free(task);
	task = NULL;
	return ret;
}
	

BOOL emule_can_abandon_resource(void* data_manager, struct tagRESOURCE* res)
{
	//这里以后再完善
	return TRUE;
}

BOOL emule_is_task_exist_by_gcid(TASK* task, const _u8* gcid)
{
	EMULE_TASK* emule_task = (EMULE_TASK*)task;
	if(emule_task->_data_manager == NULL)
		return FALSE;					//可能已经下载成功，data_manager已经被释放
	return sd_is_cid_equal(emule_task->_data_manager->_gcid, gcid);
}

_int32 emule_update_task_info(TASK* task)
{
	EMULE_TASK* emule_task = (EMULE_TASK*)task;
	EMULE_DATA_MANAGER* data_manager = emule_task->_data_manager;
	if(task->task_status != TASK_S_RUNNING && task->task_status != TASK_S_VOD)
	{
		return DT_ERR_TASK_NOT_RUNNING;
	}
    if( cm_is_idle_status( & (task->_connect_manager) ,INVALID_FILE_INDEX)==TRUE
        && cm_is_idle_status( & (task->_connect_manager) ,0)==TRUE )
    {
        /*
        task->task_status = TASK_S_FAILED;
        task->failed_code = 130;
        */
        emule_notify_task_failed(emule_task, 130);
    }
	task->progress = file_info_get_file_percent(&data_manager->_file_info);
	task->file_size = data_manager->_file_info._file_size;
	task->_downloaded_data_size = file_info_get_download_data_size(&data_manager->_file_info);
	task->_written_data_size = file_info_get_writed_data_size(&data_manager->_file_info);
	task->ul_speed = task->_connect_manager._cur_peer_upload_speed;
#ifdef _CONNECT_DETAIL
       task->valid_data_speed = file_info_get_valid_data_speed(&data_manager->_file_info);
#endif	

	dt_update_task_info(task);
	/* 确保任务正在运行的时候,_downloaded_data_size 不会大于 file_size */
	if(task->task_status == TASK_S_RUNNING||task->task_status == TASK_S_VOD)
	{
		if((task->file_size != 0) && (task->_downloaded_data_size >= task->file_size))
		{
			task->_downloaded_data_size = task->file_size -1;
			if(task->_written_data_size>=task->file_size)
			{
				if(task->task_status == TASK_S_VOD)
				{
					/* 该任务是一个点播任务,且已经下载完毕 */
					task->_downloaded_data_size = task->file_size;
				}
				else
				{
					if(data_manager->_file_success_flag)
					{
						task->_downloaded_data_size = task->file_size;
					}
				}
			}
		}
	}
	return SUCCESS;
}

void emule_get_peer_hash_value(_u32 ip, _u16 port, _u32* result)
{
	_u32 hash = 0, x = 0, i = 0;
	char* buffer = (char*)&ip;
	for(i = 1; i < 7; ++i)
	{
		hash = (hash << 4) + (*buffer);
		if((x = hash & 0xF0000000L) != 0)
		{
			hash ^= (x >> 24);
			hash &= ~x;
		}
		++buffer;
		if(i % 5 == 0)
			buffer = (char*)&port;
	}
	*result = hash & 0x7FFFFFFF;
}

_int32 emule_resource_create(RESOURCE** res, _u32 ip, _u16 port, _u32 server_ip, _u16 server_port)
{
	_int32 ret = SUCCESS;
	EMULE_RESOURCE* emule_res = NULL;
	ret = sd_malloc(sizeof(EMULE_RESOURCE), (void**)&emule_res);
	CHECK_VALUE(ret);
	sd_memset(emule_res, 0, sizeof(EMULE_RESOURCE));
	init_resource_info(&emule_res->_base_res, EMULE_RESOURCE_TYPE);
	emule_res->_client_id = ip;
	emule_res->_client_port = port;
	emule_res->_server_ip = server_ip;
	emule_res->_server_port = server_port;
	*res = (RESOURCE*)emule_res;
	LOG_DEBUG("emule_resource_create, ip = %u, port = %hu, server_ip = %u, server_port = %hu .res_ptr:0x%x", 
        ip, port, server_ip, server_port, emule_res);

	//这里估计是为了测试添加的
	/*
	{
		_u8 user_id[USER_ID_SIZE] 
            = {0x9d, 0x91, 0x75, 0x5f, 0x83, 0x0e, 0x76, 0x3a, 0xaf, 0x5c, 0x73, 0x67, 0x98, 0x35, 0x6f, 0x30};	
		sd_memcpy(emule_res->_user_id, user_id, USER_ID_SIZE);
	}
	*/
	return ret;
}

_int32 emule_resource_destroy(RESOURCE** resource)
{
	_int32 ret;
	EMULE_RESOURCE* emule_resource;
	sd_assert((*resource)->_resource_type == EMULE_RESOURCE_TYPE);
	if((*resource)->_resource_type != EMULE_RESOURCE_TYPE)
	{
		LOG_DEBUG("emule_resource_destroy failed, resource type is not EMULE_RESOURCE_TYPE");
		return -1;
	}
	emule_resource = (EMULE_RESOURCE*)*resource;
	LOG_DEBUG("emule_resource_destroy, ip = %u, port = %u.", emule_resource->_server_ip, emule_resource->_client_port);
	uninit_resource_info(&emule_resource->_base_res);
	ret = sd_free(emule_resource);
	emule_resource = NULL;
	CHECK_VALUE(ret);
	*resource = NULL;
	return SUCCESS;
}

#ifdef _CONNECT_DETAIL
_int32 emule_pipe_get_peer_pipe_info(DATA_PIPE* pipe, PEER_PIPE_INFO* info)
{
	EMULE_RESOURCE* emule_res = (EMULE_RESOURCE*)pipe->_p_resource;
	sd_memset(info, 0, sizeof(PEER_PIPE_INFO));
	info->_pipe_state = 2;
	info->_connect_type= 0;
	sd_inet_ntoa(emule_res->_client_id, info->_external_ip, 24);
	info->_internal_tcp_port = emule_res->_client_port;
	info->_speed = pipe->_dispatch_info._speed;
	info->_score = pipe->_dispatch_info._score;
	info->_pipe_state = pipe->_dispatch_info._pipe_state;
	return SUCCESS;
}
#endif

_int32 emule_get_task_file_name(TASK* p_task, char *file_name_buffer, _int32* buffer_size)
{
	EMULE_TASK* emule_task = (EMULE_TASK *)p_task;
	char * file_name = NULL;
	LOG_DEBUG("[emule_task = 0x%x]emule_get_task_file_name.", emule_task);
	sd_assert(emule_task->_data_manager != NULL);
	if(file_info_get_file_name(&emule_task->_data_manager->_file_info, &file_name))
	{
		if(sd_strlen(file_name)!=0)
		{
			if(sd_strlen(file_name)>=(*buffer_size))
			{
				*buffer_size = sd_strlen(file_name)+1;
				return DT_ERR_BUFFER_NOT_ENOUGH;
			}
			sd_memset(file_name_buffer,0,*buffer_size);
			sd_memcpy(file_name_buffer,file_name,sd_strlen(file_name));
            *buffer_size = sd_strlen(file_name);
            if(emule_task->_data_manager->_file_info._has_postfix == TRUE)
            {
                *buffer_size -= 3;
                file_name_buffer[*buffer_size] = '\0';
            }
            LOG_DEBUG("[emule_task = 0x%x]emule_get_task_file_name:%s, size:%d", 
                emule_task, file_name_buffer, *buffer_size);
			return SUCCESS;
		}
	}
	return DT_ERR_INVALID_FILE_NAME;
}

_int32 emule_get_file_path(EMULE_DATA_MANAGER* p_data_manager_interface,  char ** file_path)
{
   return file_info_get_file_path(&p_data_manager_interface->_file_info,  file_path);
}

_int32 emule_get_file_name(EMULE_DATA_MANAGER* p_data_manager_interface, char ** file_name)
{
   return file_info_get_file_name(&p_data_manager_interface->_file_info,  file_name);
}

void emule_set_enable( BOOL is_enable )
{
	g_enable_emule_download = is_enable;
	settings_set_int_item( "emule.enable_emule_download", g_enable_emule_download );
	LOG_DEBUG( "emule_set_enable:%d", is_enable );
}

_int32 emule_get_task_gcid(TASK *p_task, _u8 *gcid)
{
	EMULE_TASK *p_emule_task = (EMULE_TASK*)p_task;

	if(p_task == NULL) return INVALID_MEMORY;
	
	if(TRUE == emule_get_gcid(p_emule_task->_data_manager, gcid))
		return SUCCESS;
	return DT_ERR_INVALID_GCID;
}

_int32 emule_get_task_cid(TASK *p_task, _u8 *cid)
{
	EMULE_TASK *p_emule_task = (EMULE_TASK*)p_task;
	if(p_task == NULL) return INVALID_MEMORY;

	if(TRUE == emule_get_shub_cid(p_emule_task->_data_manager, cid))
		return SUCCESS;
	if(TRUE == emule_get_cid(p_emule_task->_data_manager, cid))
		return SUCCESS;
	return DT_ERR_GETTING_CID;
}

_int32 emule_set_notify_event_callback( void * call_ptr )
{
    if(NULL == call_ptr)
        return INVALID_HANDLER;
    LOG_DEBUG( "emule_set_notify_event_callback: 0x%x", call_ptr);
	
 	emule_query_set_notify_event_callback(call_ptr);

    return SUCCESS;
}

BOOL compare_emule_resource(EMULE_RESOURCE *res1, EMULE_RESOURCE *res2)
{
    sd_assert(res1 != NULL);
    sd_assert(res2 != NULL);

    LOG_DEBUG("res1 = 0x%x, client_id = %u, client_port = %hu, server_ip = %u, server_port = %u"
        " res2 = 0x%x, client_id = %u, client_port = %hu, server_ip = %u, server_port = %u", 
        res1, res1->_client_id, res1->_client_port, res1->_server_ip, res1->_server_port,
        res2, res2->_client_id, res2->_client_port, res2->_server_ip, res2->_server_port);

    return ((res1->_client_id == res2->_client_id)
        && (res1->_client_port == res2->_client_port)
        && (res1->_server_ip == res2->_server_ip)
        && (res1->_server_port == res2->_server_port)) ? TRUE : FALSE;
}

// 一个file_id只能由一个任务下载，所以file_id可以作为任务的唯一标识
// 如果file_id为NULL，尝试通过gcid来查找
BOOL emule_is_my_task(TASK *p_task, _u8 file_id[FILE_ID_SIZE], const _u8* gcid)
{
    EMULE_TASK *emule_task = (EMULE_TASK *)p_task;    

    sd_assert(emule_task);

    LOG_DEBUG("emule_is_my_task, task = 0x%x, file_id = 0x%x, gcid = 0x%x.", p_task, &file_id[0], gcid);

    if (file_id != NULL)
    {
        _u8 task_file_id[FILE_ID_SIZE] = {0};

        emule_log_print_file_id(file_id, "to compare task file_id");
        emule_log_print_file_id(task_file_id, "task file_id");

        if (emule_task)
        {
            emule_get_file_id((void *)(emule_task->_data_manager), task_file_id);
            return sd_strncmp((const char *)(&file_id[0]), 
                (const char *)(&task_file_id[0]), FILE_ID_SIZE) == 0 ? TRUE : FALSE;
        }
    }
    else if (gcid != NULL)
    {
        _u8 task_gcid[CID_SIZE] = {0};

        if (emule_task)
        {
            if (emule_get_task_gcid(&(emule_task->_task), task_gcid) == SUCCESS)
            {
                return sd_is_cid_equal(gcid, task_gcid);
            }
        }
    }

    return FALSE;
}

_int32 emule_just_query_file_info(void* user_data, const char * url)
{
	_int32 ret = SUCCESS;
	ED2K_LINK_INFO ed2k_info;
	TM_QUERY_SHUB_PAGE* p_res_query_pata = (TM_QUERY_SHUB_PAGE*)user_data;

	LOG_DEBUG("emule_just_query_file_info, ed2k_link = %s.",url);
       
    if( !g_enable_emule_download ) return EMULE_DOWNLOAD_DISABLE;

	ret = emule_extract_ed2k_link(url, &ed2k_info);
	if(ret != SUCCESS)
	{
		return EMULE_ED2K_LINK_ERR;
	}
    p_res_query_pata->_file_size = ed2k_info._file_size;
	if(sd_strlen(ed2k_info._file_name)!=0)
	{
		char * pos = sd_strrchr(ed2k_info._file_name,'.');
		if(pos!=NULL)
		{
			sd_strncpy(p_res_query_pata->_file_suffix,pos+1,15);
		}
	}
	ret = emule_just_query_emule_info((void*)p_res_query_pata, ed2k_info._file_id, ed2k_info._file_size);
	CHECK_VALUE(ret);
	
	return SUCCESS;
}
_int32 emule_just_query_file_info_resp(void* user_data,_int32 result,_u8 * cid,_u8* gcid,_u32 gcid_level,_u32 control_flag)
{
	_int32 ret = SUCCESS;
	TM_QUERY_SHUB_PAGE* p_res_query_pata = (TM_QUERY_SHUB_PAGE*)user_data;

	LOG_DEBUG("emule_just_query_file_info_resp, user_data = 0x%X, result = %d.", user_data, result);
	
	tm_res_query_notify_shub_handler(user_data,result,0,p_res_query_pata->_file_size,cid, gcid,gcid_level, NULL, 0,0,0,p_res_query_pata->_file_suffix,control_flag);
	return SUCCESS;
}


#endif
