#if !defined(__DOWNLOAD_TASK_C_20080916)
#define __DOWNLOAD_TASK_C_20080916
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


#include "download_task.h"
#include "res_query/res_query_interface.h"

#include "p2sp_download/p2sp_task.h"
#include "high_speed_channel/hsc_interface.h"

#ifdef ENABLE_BT
#include "bt_download/bt_task/bt_task.h"
#include "bt_download/bt_task/bt_accelerate.h"
#include "task_manager/task_manager.h"
#include "bt_download/bt_data_manager/bt_file_manager.h"
#include "torrent_parser/torrent_parser.h"
#include "bt_download/bt_magnet/bt_magnet_task.h"


#include "utility/map.h"
#endif /* #ifdef ENABLE_BT */

#ifdef ENABLE_EMULE
#include "emule/emule_interface.h"
#include "emule/emule_data_manager/emule_data_manager.h"
#endif /* #ifdef ENABLE_BT */

#include "utility/time.h"
#include "utility/logid.h"
#ifdef LOGID
#undef LOGID
#endif

#define LOGID LOGID_DOWNLOAD_TASK

#include "utility/logger.h"
#include "utility/utility.h"
#include "p2p_data_pipe/p2p_pipe_interface.h"

#ifdef ENABLE_HSC
#include "high_speed_channel/hsc_info.h"

extern HSC_INFO_BRD g_task_hsc_info[];
extern RW_SHAREBRD *g_task_hsc_info_brd_ptr;

#endif /* ENABLE_HSC */



/*--------------------------------------------------------------------------*/
/*                Global declarations				        */
/*--------------------------------------------------------------------------*/




/*--------------------------------------------------------------------------*/
/*                Interfaces provid for external program				        */
/*--------------------------------------------------------------------------*/

_int32 init_download_task_module(void)
{
    LOG_DEBUG("init_download_task_module");
    return res_query_register_add_resource_handler(dt_add_server_resource_ex,
        dt_add_peer_resource, dt_add_relation_fileinfo);
}
_int32 uninit_download_task_module(void)
{
	LOG_DEBUG("uninit_download_task_module");
	return SUCCESS;
}

_int32 dt_add_server_resource(void* user_data, char *url, _u32 url_size, char* ref_url, _u32 ref_url_size, BOOL is_origin,_u8 resource_priority)
{
	return dt_add_server_resource_ex( user_data, url, url_size,  ref_url, ref_url_size,  is_origin, resource_priority, FALSE, 0);	
}
_int32 dt_add_server_resource_ex(void* user_data, char *url, _u32 url_size, char* ref_url, _u32 ref_url_size, BOOL is_origin,_u8 resource_priority, BOOL relation_res, _u32 relation_index)
{
	RES_QUERY_PARA * p_res_query_pata = (RES_QUERY_PARA *)user_data;
	TASK * p_task=NULL;
	_int32 ret_val=SUCCESS;
	RESOURCE *p_orgin_res=NULL;
	
	LOG_DEBUG( "AAAA MMMM dt_add_server_resource_ex origion:%d relation_res:%d, relation_index:%u", is_origin, relation_res, relation_index);

	sd_assert(user_data!=NULL);

	// 1. 启动下载任务查询时会指定该任务参数 2. 单独外部接口查询时是指定该参数为空(单独查询不需要添加资源)
	if(p_res_query_pata->_task_ptr==NULL) return SUCCESS;

	p_task=p_res_query_pata->_task_ptr;

	LOG_DEBUG( "Redirection dt_add_server_resource");
#ifdef KANKAN_PROJ
	/* 局域网任务不加外网资源 */
	if(((P2SP_TASK *)p_task)->_is_lan) return SUCCESS;

	LOG_DEBUG( "Redirection dt_add_server_resource 1");
#endif /* KANKAN_PROJ */
	LOG_DEBUG( "Redirection dt_add_server_resource 2");

	if(is_origin==TRUE)
	{

		if(p_task->_task_type==P2SP_TASK_TYPE)
		{
			ret_val = dm_set_origin_url_info( &(((P2SP_TASK *)p_task)->_data_manager),url, ref_url );
			CHECK_VALUE(ret_val);

			ret_val = cm_add_origin_server_resource( &(p_task->_connect_manager),p_res_query_pata->_file_index,  url,  url_size,ref_url,  ref_url_size, &p_orgin_res );
			CHECK_VALUE(ret_val);
			ret_val = dm_set_origin_resource( &(((P2SP_TASK *)p_task)->_data_manager),p_orgin_res );
			CHECK_VALUE(ret_val);
		}
		else
		{
			CHECK_VALUE(DT_ERR_INVALID_DOWNLOAD_TASK);
		}
		
	}
	else
	{
		cm_add_server_resource_ex(&(p_task->_connect_manager),p_res_query_pata->_file_index,  url,  url_size,ref_url,  ref_url_size, resource_priority, p_task, relation_res, relation_index);
	}
	return SUCCESS;
}

_int32 dt_add_peer_resource(void* user_data, char *peer_id
    , _u8 *gcid, _u64 file_size, _u32 peer_capability, _u32 host_ip, _u16 tcp_port
    , _u16 udp_port,  _u8 res_level, _u8 res_from, _u8 res_priority)
{
	RES_QUERY_PARA * p_res_query_pata = (RES_QUERY_PARA *)user_data;
	TASK * p_task=NULL;
	_u64 file_size_t=0;
	_u8 gcid_t[CID_SIZE];
	_int32 ret_val = -1;

	LOG_DEBUG( "AAAA MMMM dt_add_peer_resource" );

	sd_assert(user_data!=NULL);
    sd_memset(gcid_t, 0, CID_SIZE);
	
	p_task=p_res_query_pata->_task_ptr;
    if(p_task->_task_type == EMULE_TASK_TYPE)
    {
        sd_assert(p_res_query_pata->_file_index == 0);
    }

		if(gcid!=NULL)
		{
			ret_val = cm_add_active_peer_resource(&(p_task->_connect_manager)
			    , p_res_query_pata->_file_index
			    , peer_id
			    , gcid
			    , file_size
			    , peer_capability
			    , host_ip
			    , tcp_port
			    , udp_port
			    , res_level
			    , res_from 
			    , res_priority);
		}
		else
		{
			if(p_task->_task_type==P2SP_TASK_TYPE)
			{
				file_size_t = dm_get_file_size(&(((P2SP_TASK *)p_task)->_data_manager));
				if((((P2SP_TASK *)p_task)->_is_gcid_ok == TRUE)&&(dm_get_shub_gcid( &(((P2SP_TASK *)p_task)->_data_manager),  gcid_t)==TRUE))
				{
					ret_val = cm_add_active_peer_resource(&(p_task->_connect_manager)
					    , p_res_query_pata->_file_index,   peer_id,  gcid_t,file_size_t
					    ,  peer_capability,host_ip,tcp_port,udp_port
					    ,  res_level, res_from, res_priority);
				}
				
			}
#ifdef ENABLE_BT
			else if(p_task->_task_type==BT_TASK_TYPE)
			{
				file_size_t = bdm_get_sub_file_size( &(((BT_TASK *)p_task)->_data_manager),p_res_query_pata->_file_index);
				if(bdm_get_shub_gcid(  &(((BT_TASK *)p_task)->_data_manager),p_res_query_pata->_file_index, gcid_t)==TRUE)
				{
					ret_val = cm_add_active_peer_resource(&(p_task->_connect_manager)
					    ,p_res_query_pata->_file_index,   peer_id
					    ,  gcid_t,file_size_t,  peer_capability,host_ip,tcp_port,udp_port
					    ,  res_level, res_from, res_priority);
				}
			}
#endif /* #ifdef ENABLE_BT */

#ifdef ENABLE_EMULE
            else
            {
                file_size_t = emule_get_file_size( ((struct tagEMULE_TASK *)p_task)->_data_manager );
                emule_get_gcid(((struct tagEMULE_TASK *)p_task)->_data_manager, gcid_t);
                ret_val = cm_add_active_peer_resource(&(p_task->_connect_manager)
                    ,p_res_query_pata->_file_index,   peer_id,  gcid_t,file_size_t
                    ,  peer_capability,host_ip,tcp_port,udp_port
                    ,  res_level, res_from, res_priority);
            }
#endif /* #ifdef ENABLE_BT */
		}
        
#if defined(ENABLE_HSC) && !defined(ENABLE_ETM_MEDIA_CENTER)
	if(res_from==P2P_FROM_VIP_HUB&&ret_val==SUCCESS)
	{
		 hsc_handle_auto_enable(p_task, p_res_query_pata->_file_index, gcid, file_size);
	}
#endif

	return ret_val;
}
_int32 dt_add_relation_fileinfo(void* user_data,  _u8 gcid[CID_SIZE],  _u8 cid[CID_SIZE], _u64 file_size,  _u32 block_num,  RELATION_BLOCK_INFO** p_block_info_arr)
{
	LOG_DEBUG("dt_add_relation_fileinfo");

	RES_QUERY_PARA * p_res_query_pata = (RES_QUERY_PARA *)user_data;
	TASK * p_task=NULL;
	_int32 ret_val=SUCCESS;
	RESOURCE *p_orgin_res=NULL;
	_int32  current_index = -1;

	sd_assert(user_data!=NULL);

	p_task=p_res_query_pata->_task_ptr;

	sd_assert(p_task->_task_type == P2SP_TASK_TYPE);

	P2SP_TASK* p2sp_task = (P2SP_TASK*)p_task;

	if(p2sp_task->_relation_file_num >= MAX_RELATION_FILE_NUM)
	{
		LOG_DEBUG("dt_add_relation_fileinfo too much relation file... just return ...");
		return current_index;
	}

	_u32 i = 0; 
	for(i = 0; i < p2sp_task->_relation_file_num; i++)
	{
		P2SP_RELATION_FILEINFO* p_file = p2sp_task->_relation_fileinfo_arr[i];

		sd_assert(NULL != p_file);

		if(sd_memcmp(p_file->_gcid, gcid, CID_SIZE) == 0)
		{
			current_index = i ;
			LOG_DEBUG("dt_add_relation_fileinfo this cid already in list...current_index:%u", current_index);
			return current_index;
		}
	}

	
	LOG_DEBUG("dt_add_relation_fileinfo before add.. p2sp_task->_relation_file_num:%d", p2sp_task->_relation_file_num );
	

	ret_val = sd_malloc(sizeof(P2SP_RELATION_FILEINFO), (void**)&p2sp_task->_relation_fileinfo_arr[p2sp_task->_relation_file_num]);
	

	if(ret_val != SUCCESS)
	{
		LOG_DEBUG("dt_add_relation_fileinfo malloc mem failed... ");
		return current_index;
	}

	current_index = p2sp_task->_relation_file_num;	
	
	P2SP_RELATION_FILEINFO* p_relation_info = p2sp_task->_relation_fileinfo_arr[p2sp_task->_relation_file_num];

	sd_memcpy(p_relation_info->_cid, cid, CID_SIZE);
	sd_memcpy(p_relation_info->_gcid, gcid, CID_SIZE);
	p_relation_info->_file_size = file_size;
	p_relation_info->_block_info_num = block_num;
	p_relation_info->_p_block_info_arr = *p_block_info_arr;
	*p_block_info_arr = NULL;

	p2sp_task->_relation_file_num++;

#ifdef _DEBUG
{
	extern void dump_relation_file_info(char* pos,_u32 i, P2SP_RELATION_FILEINFO* p_relation_info);
	_u32 i = 0;
	for(i = 0 ; i < p2sp_task->_relation_file_num; i++)
	{
		dump_relation_file_info("dt_add_relation_fileinfo", i,p2sp_task->_relation_fileinfo_arr[i]);
	}
}
#endif

	return current_index;
}

_int32 dt_add_task_res(TASK* p_task, EDT_RES * p_resource)
{
    _int32 ret_val = SUCCESS;
    _u64 file_size_t = 0;
    _u8 gcid_t[CID_SIZE];
    BOOL need_add_lixian_server = TRUE;
	char lixian_url[MAX_URL_LEN] = {0};
	char cookie[MAX_COOKIE_LEN] = {0};
    LOG_DEBUG("dt_add_task_res");

    if(p_resource->_type ==EDT_SERVER)
    {
#ifdef ENABLE_BT
        if (p_task->_task_type == BT_TASK_TYPE)
        {
            BT_TASK *p_bt_task = (BT_TASK *)p_task;
			BT_FILE_INFO fileInfo = {0};
            BOOL isStartSucc;
			BT_FILE_INFO_POOL bt_info_pool = {0};
			ET_BT_FILE_INFO bt_file_info = {0};
            bt_get_file_info_pool(p_task, &bt_info_pool);
            if (!bt_info_pool._file_info_for_user)
            {
                LOG_ERROR("dt_add_task_res bt_get_file_info_pool _file_info_for_user null");
                CHECK_VALUE(FALSE);
            }
            ret_val = bt_get_file_info(&bt_info_pool, p_resource->edt_s_res._file_index, &bt_file_info);
            if (ret_val != SUCCESS)
            {
                LOG_ERROR("dt_add_task_res bt_get_file_info err:%d", ret_val);
                CHECK_VALUE(ret_val);
            }
			if ( (bt_file_info._file_status == BT_FILE_DOWNLOADING || bt_file_info._file_status == BT_FILE_IDLE) && bt_file_info._accelerate_state == BT_NO_ACCELERATE)
            {
                ret_val = bt_start_accelerate(p_bt_task, &fileInfo, p_resource->edt_s_res._file_index, &isStartSucc);
                if (ret_val != SUCCESS)
                {
					//CHECK_VALUE(ret_val);
					return ret_val;
                }
            }
            // 如果状态是BT_ACCELERATE_END,那么表示是出现了错误，子任务管理器可能取不到
			if (bt_file_info._accelerate_state == BT_ACCELERATE_END || bt_file_info._file_status == BT_FILE_FINISHED)
            {
                need_add_lixian_server = FALSE;
            }

        }
#endif
#ifdef ENABLE_LIXIAN
        if (need_add_lixian_server)
        {
            ret_val = cm_add_lixian_server_resource(&(p_task->_connect_manager),p_resource->edt_s_res._file_index,
                p_resource->edt_s_res._url,  p_resource->edt_s_res._url_len,p_resource->edt_s_res._ref_url,  p_resource->edt_s_res._ref_url_len,
                p_resource->edt_s_res._cookie,  p_resource->edt_s_res._cookie_len, p_resource->edt_s_res._resource_priority);
            if((ret_val!=SUCCESS)&&(ret_val!=CM_ADD_SERVER_PEER_EXIST))
            {
                CHECK_VALUE(ret_val);
            }
			if(p_task->_task_type==P2SP_TASK_TYPE)
			{
				sd_strncpy(lixian_url, p_resource->edt_s_res._url, p_resource->edt_s_res._url_len);
				LOG_DEBUG("dt_add_task_res, dm_set_origin_url_info ,url[%u] = %s", p_resource->edt_s_res._url_len, lixian_url);
				ret_val = dm_set_origin_url_info( &(((P2SP_TASK *)p_task)->_data_manager), NULL, lixian_url);
				CHECK_VALUE(ret_val);

				sd_strncpy(cookie, p_resource->edt_s_res._cookie, p_resource->edt_s_res._cookie_len);
				LOG_DEBUG("dt_add_task_res, dm_set_cookie_info ,cookie[%u] = %s", p_resource->edt_s_res._cookie_len, cookie);
				ret_val = dm_set_cookie_info( &(((P2SP_TASK *)p_task)->_data_manager), cookie);
				CHECK_VALUE(ret_val);
			}
        }
#endif
    }
    else
    {
        if(p_task->_task_type==P2SP_TASK_TYPE)
        {
            file_size_t = dm_get_file_size(&(((P2SP_TASK *)p_task)->_data_manager));
            if((((P2SP_TASK *)p_task)->_is_gcid_ok == TRUE)&&(dm_get_shub_gcid( &(((P2SP_TASK *)p_task)->_data_manager),  gcid_t)==TRUE))
            {
                ret_val = cm_add_cdn_peer_resource( &(p_task->_connect_manager),p_resource->edt_p_res._file_index, p_resource->edt_p_res._peer_id, gcid_t, file_size_t, p_resource->edt_p_res._peer_capability, p_resource->edt_p_res._host_ip, p_resource->edt_p_res._tcp_port , p_resource->edt_p_res._udp_port, p_resource->edt_p_res._res_level, P2P_FROM_CDN );
            }				
        }

#ifdef ENABLE_BT
        else if(p_task->_task_type==BT_TASK_TYPE)
        {
            file_size_t = bdm_get_sub_file_size( &(((BT_TASK *)p_task)->_data_manager),p_resource->edt_p_res._file_index);
            if(bdm_get_shub_gcid(  &(((BT_TASK *)p_task)->_data_manager),p_resource->edt_p_res._file_index, gcid_t)==TRUE)
            {
                ret_val = cm_add_cdn_peer_resource( &(p_task->_connect_manager),p_resource->edt_p_res._file_index, p_resource->edt_p_res._peer_id, gcid_t, file_size_t, p_resource->edt_p_res._peer_capability, p_resource->edt_p_res._host_ip, p_resource->edt_p_res._tcp_port , p_resource->edt_p_res._udp_port, p_resource->edt_p_res._res_level, P2P_FROM_LIXIAN );
            }
        }
#endif /* #ifdef ENABLE_BT */
		if((ret_val!=SUCCESS)&&(ret_val!=CM_ADD_SERVER_PEER_EXIST)&&(ret_val != CM_ADD_ACTIVE_PEER_EXIST))
        {
            CHECK_VALUE(ret_val);
        }
    }
    return SUCCESS;
}

_int32 dt_remove_task_tmp_file( TASK * p_task )
{

	LOG_DEBUG("dt_remove_task_tmp_file");
	
	p_task->need_remove_tmp_file = TRUE;
#ifdef ENABLE_BT
    if( p_task->_task_type==BT_TASK_TYPE )
    {
        bdm_set_del_tmp_flag( &(((BT_TASK *)p_task)->_data_manager));
    }
#endif /* ENABLE_BT */
	return SUCCESS;
}


/* Update the task information */
_int32 dt_update_task_info(TASK* p_task)
{
    LOG_DEBUG("dt_update_task_info");
    /* Get the downloading speed */
	p_task->speed = cm_get_current_task_speed(& (p_task->_connect_manager));
	LOG_DEBUG("dt_update_task_info p_task->speed = %d", p_task->speed);
	p_task->server_speed= cm_get_current_task_server_speed(& (p_task->_connect_manager));
	p_task->peer_speed= cm_get_current_task_peer_speed(& (p_task->_connect_manager));
	p_task->dowing_server_pipe_num= cm_get_working_server_pipe_num(& (p_task->_connect_manager));
	p_task->connecting_server_pipe_num= cm_get_connecting_server_pipe_num(& (p_task->_connect_manager));
	p_task->dowing_peer_pipe_num= cm_get_working_peer_pipe_num(& (p_task->_connect_manager));
	p_task->connecting_peer_pipe_num= cm_get_connecting_peer_pipe_num(& (p_task->_connect_manager));
#if defined( ENABLE_CDN)
#if defined(_DEBUG)&&defined(MOBILE_PHONE)
	p_task->dowing_server_pipe_num |= (cm_get_working_cdn_pipe_num( & (p_task->_connect_manager))<<8);
	p_task->connecting_server_pipe_num |= (cm_get_cdn_res_num(&(p_task->_connect_manager))<<8);
#if defined(KANKAN_PROJ)  //Just for testing...
	p_task->connecting_server_pipe_num = cm_get_cdn_res_num(&(p_task->_connect_manager));
#endif
#else
	p_task->dowing_server_pipe_num += cm_get_working_cdn_pipe_num( & (p_task->_connect_manager));
#endif
#ifdef ENABLE_HSC
	p_task->_hsc_info._speed = pt_get_vip_cdn_speed(p_task);
#endif /* ENABLE_HSC */
#endif /* ENABLE_CDN */

#ifdef ENABLE_LIXIAN
	p_task->_main_task_lixian_info._speed = pt_get_lixian_speed(p_task);
#endif

#ifdef _CONNECT_DETAIL

     cm_get_working_peer_type_info( & (p_task->_connect_manager),
        & p_task->downloading_tcp_peer_pipe_num,
        & p_task->downloading_udp_peer_pipe_num,
        & p_task->downloading_tcp_peer_pipe_speed,
        & p_task->downloading_udp_peer_pipe_speed  );

	p_task->idle_server_res_num = cm_get_idle_server_res_num(& (p_task->_connect_manager));
	p_task->using_server_res_num = cm_get_using_server_res_num(& (p_task->_connect_manager));
	p_task->candidate_server_res_num = cm_get_candidate_server_res_num(& (p_task->_connect_manager));
	p_task->retry_server_res_num = cm_get_retry_server_res_num(& (p_task->_connect_manager));
	p_task->discard_server_res_num = cm_get_discard_server_res_num(& (p_task->_connect_manager));
	p_task->destroy_server_res_num = cm_get_destroy_server_res_num(& (p_task->_connect_manager));

	p_task->idle_peer_res_num = cm_get_idle_peer_res_num(& (p_task->_connect_manager));
	p_task->using_peer_res_num = cm_get_using_peer_res_num(& (p_task->_connect_manager));

    	p_task->candidate_peer_res_num = cm_get_candidate_peer_res_num(& (p_task->_connect_manager));
	p_task->retry_peer_res_num = cm_get_retry_peer_res_num(& (p_task->_connect_manager));
	p_task->discard_peer_res_num = cm_get_discard_peer_res_num(& (p_task->_connect_manager));
	p_task->destroy_peer_res_num = cm_get_destroy_peer_res_num(& (p_task->_connect_manager));
	p_task->cm_level = cm_get_cm_level(& (p_task->_connect_manager));
	p_task->_peer_pipe_info_array = cm_get_peer_pipe_info_array(& (p_task->_connect_manager));
	p_task->_server_pipe_info_array = cm_get_server_pipe_info_array(& (p_task->_connect_manager));

#endif

	if(p_task->task_status==TASK_S_RUNNING)
		sd_time(&(p_task->_finished_time));

	/* Get other informations which reporting need */
	if((p_task->task_status == TASK_S_SUCCESS)||(p_task->task_status == TASK_S_FAILED))
	{
		p_task->_fault_block_size = 0;
		p_task->_total_server_res_count = 1;
		p_task->_valid_server_res_count = 1;
		p_task->is_nated = 0;
		p_task->dw_comefrom = 0;
		p_task->download_stat =  0; //B0:是否发生纠错;B1:是否强制成功;B2:是否边下边播
		p_task->_cdn_num = 0;
		p_task->_total_peer_res_count = 0;
		
		LOG_ERROR("last_task_info:task_id=%u,task_status=%d,failed_code=%u,progress=%u,_start_time=%u,_finished_time=%u,file_size=%llu,_file_create_status=%d",
			p_task->_task_id,p_task->task_status,p_task->failed_code,p_task->progress,p_task->_start_time,p_task->_finished_time,p_task->file_size,p_task->_file_create_status);
		
#if 0 //def LITTLE_FILE_TEST
		LOG_URGENT("****last_task_info:task_id=%u,task_status=%d,failed_code=%u,progress=%u,_start_time=%u,_finished_time=%u,file_size=%llu,_file_create_status=%d",
			p_task->_task_id,p_task->task_status,p_task->failed_code,p_task->progress,p_task->_start_time,p_task->_finished_time,p_task->file_size,p_task->_file_create_status);
#endif /* LITTLE_FILE_TEST */
	}
	
	return SUCCESS;

}

#ifdef ENABLE_LIXIAN
_int32 dt_get_lixian_info( TASK * p_task, _u32 file_index,void * p_lx_info )
{
	_int32 ret_val = SUCCESS;
	LIXIAN_INFO *p_info = (LIXIAN_INFO* )p_lx_info;
	sd_memset(p_info,0x00,sizeof(LIXIAN_INFO));
	if(p_task->_task_type!=BT_TASK_TYPE)
	{
		file_index = INVALID_FILE_INDEX;
	}


	// 表明获取整个BT任务的离线信息
	if(p_task->_task_type ==BT_TASK_TYPE  && file_index == INVALID_FILE_INDEX)
	{
		p_info->_dl_bytes = p_task->_main_task_lixian_info._dl_bytes;
		p_info->_speed =  p_task->_main_task_lixian_info._speed;
		p_info->_errcode = SUCCESS;
		p_info->_res_num = 0;
		p_info->_state = LS_RUNNING;
		return SUCCESS;
	}

	ret_val =  cm_get_lixian_info( &(p_task->_connect_manager) ,file_index, p_info);
	if(ret_val!=SUCCESS) return ret_val;
	
 	LOG_DEBUG("dt_get_lixian_info: state = %u, res_num = %u, speed = %u", p_info->_state, p_info->_res_num, p_info->_speed);
	if(p_info->_res_num!=0)
	{
		p_info->_state = LS_RUNNING;
		switch(p_task->_task_type)
		{
			case P2SP_TASK_TYPE:
				dm_get_dl_bytes( &(((P2SP_TASK *)p_task)->_data_manager),   NULL,NULL,NULL, &p_info->_dl_bytes, NULL  );
				break;
			case BT_TASK_TYPE:
			case BT_MAGNET_TASK_TYPE:
#ifdef ENABLE_BT
            {
                _u64 origion_dl_bytes = 0;
                
				bdm_get_sub_file_dl_bytes( &(((BT_TASK *)p_task)->_data_manager),  file_index, NULL,NULL,NULL, &p_info->_dl_bytes, &origion_dl_bytes);
            }
#else
				sd_assert(FALSE);
#endif
				break;
			case EMULE_TASK_TYPE:
#ifdef ENABLE_EMULE
				emule_get_dl_bytes( (((EMULE_TASK *)p_task)->_data_manager),  NULL,NULL,NULL, NULL,&p_info->_dl_bytes  );
#else
				sd_assert(FALSE);
#endif
				break;
		}
	}
	else
	{
		ret_val = TM_ERR_INVALID_TASK_ID;
	}
	return ret_val;
}
#endif /* ENABLE_LIXIAN */

/*--------------------------------------------------------------------------*/
/*                Interfaces provid for data_manager				        */
/*--------------------------------------------------------------------------*/
_int32 dt_dispatch_at_pipe( TASK * p_task , DATA_PIPE*  ptr_data_pipe)
{

	return ds_dispatch_at_pipe(&(p_task->_dispatcher),ptr_data_pipe);
}

_int32 dt_start_dispatcher_immediate( TASK * p_task )
{
	return ds_start_dispatcher_immediate(&(p_task->_dispatcher));
}
/*
#ifdef UPLOAD_ENABLE
_int32 dt_notify_have_block( TASK * p_task, _u32 file_index )
{
	return cm_notify_have_block( &p_task->_connect_manager, file_index );
}
#endif
*/

#ifdef _VOD_NO_DISK
BOOL dt_is_no_disk_task( TASK * p_task)
{
	P2SP_TASK * p_p2sp_task = NULL;
	
	if(p_task->_task_type==P2SP_TASK_TYPE)
	{
		p_p2sp_task = (P2SP_TASK *)p_task;
		return dm_vod_is_no_disk_mode(&p_p2sp_task->_data_manager);
	}
	
	return FALSE;
}
#endif

BOOL dt_is_vod_check_data_task( TASK * p_task)
{
	P2SP_TASK * p_p2sp_task = NULL;
	
	if(p_task->_task_type==P2SP_TASK_TYPE)
	{
		p_p2sp_task = (P2SP_TASK *)p_task;
		return dm_vod_is_check_data_mode(&p_p2sp_task->_data_manager);
	}
	
	return FALSE;
}


/*--------------------------------------------------------------------------*/
/*                Internal functions				        */
/*--------------------------------------------------------------------------*/

_int32 dt_failure_exit(TASK* p_task,_int32 err_code )
{
	LOG_DEBUG( " enter pt_failure_exit(_task_id = %u,task_status = %d,_error_code=%d )...",p_task->_task_id,p_task->task_status,err_code);

	
	p_task->task_status = TASK_S_FAILED;
	p_task->failed_code = err_code;

	return SUCCESS;

} /* End of dt_failure_exit */



#ifdef EMBED_REPORT
struct tagRES_QUERY_REPORT_PARA *dt_get_res_query_report_para( TASK * p_task )
{
	return &p_task->_res_query_report_para;
}
#endif


_int32 init_task(TASK* task, enum TASK_TYPE type)
{
	task->task_status = TASK_S_IDLE;
	task->_task_type = type;
    task->_extern_id = -1;
    task->_create_time = 0;
    task->_is_continue = FALSE;
	return SUCCESS;
}

_int32 init_dphub_query_context(DPHUB_QUERY_CONTEXT *context)
{
    LOG_DEBUG("init_dphub_query_context, context(0x%x)", context);
    list_init(&context->_to_query_node_list);
    list_init(&context->_exist_node_list);
    context->_is_query_root_finish = FALSE;
    context->_current_peer_rc_num = 0;
    context->_max_res = 0;
    context->_wait_dphub_root_timer_id = 0;
    return SUCCESS;
}

_int32 uninit_dphub_query_context(DPHUB_QUERY_CONTEXT *context)
{
    LOG_DEBUG("uninit_dphub_query_context, context(0x%x)", context);
    list_clear(&context->_to_query_node_list);
    list_clear(&context->_exist_node_list);
    context->_is_query_root_finish = FALSE;
    context->_current_peer_rc_num = 0;
    context->_max_res = 0;
    if (context->_wait_dphub_root_timer_id != 0)
    {
        LOG_DEBUG("cancel context->_wait_dphub_root_timer_id.");
        cancel_timer(context->_wait_dphub_root_timer_id);
        context->_wait_dphub_root_timer_id = 0;
    }
    return SUCCESS;
}

#ifdef ENABLE_EMULE
_int32 start_task(TASK* task, struct _tag_data_manager* data_manager, can_destory_resource_call_back func, void** pipe_function_table)
{
	_int32 ret = SUCCESS;

	sd_assert(task->_task_type == EMULE_TASK_TYPE);
	sd_time(&task->_start_time);

	ret = ds_start_dispatcher(&task->_dispatcher);
	if(ret != SUCCESS)
	{
		sd_assert(task->_task_type == EMULE_TASK_TYPE);
		cm_uninit_emule_connect_manager(&task->_connect_manager);
		return ret;
	}
	task->task_status = TASK_S_RUNNING;
	return SUCCESS;
}

_int32 stop_task(TASK* task)
{
	ds_stop_dispatcher(&task->_dispatcher);
	task->task_status = TASK_S_STOPPED;
	return SUCCESS;
}

_int32 delete_task(TASK* task)
{
	cm_uninit_emule_connect_manager(&task->_connect_manager);
	ds_unit_dispatcher(&task->_dispatcher);
	return SUCCESS;
}
#endif

_int32 dt_get_origin_resource_info( TASK * p_task, void * p_resource )
{
	_int32 ret_val = SUCCESS;
	ORIGIN_RESOURCE_INFO *p_info = (ORIGIN_RESOURCE_INFO* )p_resource;
	sd_memset(p_info,0x00,sizeof(ORIGIN_RESOURCE_INFO));

	p_info->_speed = cm_get_origin_resource_speed( &(p_task->_connect_manager));

		switch(p_task->_task_type)
		{
			case P2SP_TASK_TYPE:
				dm_get_origin_resource_dl_bytes( &(((P2SP_TASK *)p_task)->_data_manager), &p_info->_dl_bytes );
				break;
#ifdef ENABLE_BT
			case BT_TASK_TYPE:
			case BT_MAGNET_TASK_TYPE:
				break;
#endif
#ifdef ENABLE_EMULE
			case EMULE_TASK_TYPE:
				emule_get_origin_resource_dl_bytes(&(((EMULE_TASK *)p_task)->_data_manager), &p_info->_dl_bytes);
				break;
#endif
			default:
				break;
		}
	

	return ret_val;
}

_int32 dt_add_assist_task_res(TASK* p_task, EDT_RES * p_resource)
{
	_int32 ret_val = SUCCESS;
	_u64 file_size_t = 0;
	_u8 gcid_t[CID_SIZE];

	char ipaddr[100] = {0};
	sd_inet_ntoa(p_resource->edt_p_res._host_ip, ipaddr, sizeof(ipaddr));
	
	LOG_DEBUG("dt_add_assist_task_res,p_task->_task_type=%d,  file_index = %d, peer_id = %s, peer_capability = %d, host_ip = %u --> %s, tcp_port =%d, udp_port =%d, res_level = %d\n",
		p_task->_task_type,
		p_resource->edt_p_res._file_index, p_resource->edt_p_res._peer_id, p_resource->edt_p_res._peer_capability, p_resource->edt_p_res._host_ip, ipaddr ,
		p_resource->edt_p_res._tcp_port , p_resource->edt_p_res._udp_port, p_resource->edt_p_res._res_level);

	p_task->_use_assist_peer = TRUE;
	ret_val = sd_malloc(sizeof(EDT_RES), (void**)&p_task->_p_assist_res);
	if(ret_val != SUCCESS)
	{
		return ret_val;
	}
	sd_assert(p_task->_p_assist_res);
	sd_memcpy(p_task->_p_assist_res, p_resource, sizeof(EDT_RES));
	
	if(p_resource->_type == EDT_PEER)
	{
		if(p_task->_task_type==P2SP_TASK_TYPE)
		{
			//P2SP_TASK *p_p2sp_task = (P2SP_TASK*)p_task;
			//sd_assert(p_p2sp_task);
			//sd_assert( &p_p2sp_task->_data_manager);
			
			//p_p2sp_task->_data_manager._use_assist_peer = TRUE;
			//sd_memcpy(&(p_p2sp_task->_data_manager.assist_peer_res), p_resource, sizeof(EDT_RES));
			
			file_size_t = dm_get_file_size(&(((P2SP_TASK *)p_task)->_data_manager));
			if(file_size_t == 0)
			{
				ret_val = DT_ERR_INVALID_FILE_SIZE;
			}
			else
			{
				if((((P2SP_TASK *)p_task)->_is_gcid_ok == TRUE)&&(dm_get_shub_gcid( &(((P2SP_TASK *)p_task)->_data_manager),  gcid_t)==TRUE))
				{
					ret_val = cm_add_cdn_peer_resource( &(p_task->_connect_manager),p_resource->edt_p_res._file_index, p_resource->edt_p_res._peer_id, gcid_t, file_size_t, p_resource->edt_p_res._peer_capability, p_resource->edt_p_res._host_ip, p_resource->edt_p_res._tcp_port , p_resource->edt_p_res._udp_port, p_resource->edt_p_res._res_level, P2P_FROM_ASSIST_DOWNLOAD );
				}
				else
					ret_val = DT_ERR_INVALID_GCID;
			}
		}
		
#ifdef ENABLE_BT
		else if(p_task->_task_type==BT_TASK_TYPE)
		{
			BT_TASK *p_bt_task = (BT_TASK*)p_task;
			sd_assert(p_bt_task);
			sd_assert( &p_bt_task->_data_manager);
			
			//p_bt_task->_data_manager._use_assist_peer = TRUE;
			//sd_memcpy(&(p_bt_task->_data_manager.assist_peer_res), p_resource, sizeof(EDT_RES));
						
			MAP_ITERATOR cur_node = MAP_BEGIN( p_bt_task->_data_manager._bt_file_manager._bt_sub_file_map );
			while( cur_node != MAP_END( p_bt_task->_data_manager._bt_file_manager._bt_sub_file_map ) )
			{
				BT_SUB_FILE* p_sub_file = (BT_SUB_FILE *)MAP_VALUE( cur_node );
				
				BT_FILE_INFO *p_file_info = NULL;
				ret_val = map_find_node(&p_bt_task->_file_info_map, (void*) (p_sub_file->_file_index), (void**)&p_file_info);
				if(ret_val == SUCCESS 
					&& p_file_info != NULL
					&& p_file_info->_file_status == BT_FILE_DOWNLOADING
					&& p_file_info->_accelerate_state == BT_ACCELERATING)
				{
					file_size_t = bdm_get_sub_file_size( &p_bt_task->_data_manager, p_sub_file->_file_index);
					if(file_size_t > 0)
					{
						if(bdm_get_shub_gcid( &p_bt_task->_data_manager, p_sub_file->_file_index, gcid_t)==TRUE)
						{
							ret_val = cm_add_cdn_peer_resource( &(p_task->_connect_manager), p_sub_file->_file_index, p_resource->edt_p_res._peer_id, gcid_t, file_size_t, p_resource->edt_p_res._peer_capability, p_resource->edt_p_res._host_ip, p_resource->edt_p_res._tcp_port , p_resource->edt_p_res._udp_port, p_resource->edt_p_res._res_level, P2P_FROM_ASSIST_DOWNLOAD );
						}
					}
				}
				cur_node = MAP_NEXT( p_bt_task->_data_manager._bt_file_manager._bt_sub_file_map, cur_node);
			}
		}
		else if(p_task->_task_type == BT_MAGNET_TASK_TYPE)
		{
			//BT_MAGNET_TASK *p_magnet_task = (BT_MAGNET_TASK*)p_task;
			
			//p_magnet_task->_data_manager._use_assist_peer = TRUE;
			//sd_memcpy(&(p_magnet_task->_data_manager.assist_peer_res), p_resource, sizeof(EDT_RES));
		}
#endif /* #ifdef ENABLE_BT */

#ifdef ENABLE_EMULE
	    else if(p_task->_task_type == EMULE_TASK_TYPE)
	   	{
	   		EMULE_TASK* emule_task = (EMULE_TASK*)p_task;
			EMULE_DATA_MANAGER* data_mgr = emule_task->_data_manager;
			sd_assert(NULL != data_mgr);

			//emule_task->_data_manager._use_assist_peer = TRUE;
			//sd_memcpy(&(emule_task->_data_manager.assist_peer_res), p_resource, sizeof(EDT_RES));
			
	   		file_size_t = data_mgr->_file_info._file_size;
			if(file_size_t == 0)
			{
				ret_val = DT_ERR_INVALID_FILE_SIZE;
			}
			else
			{
				if(data_mgr->_is_cid_and_gcid_valid)
				{
					ret_val = cm_add_cdn_peer_resource(&p_task->_connect_manager, p_resource->edt_p_res._file_index, p_resource->edt_p_res._peer_id,data_mgr->_gcid, file_size_t, p_resource->edt_p_res._peer_capability, p_resource->edt_p_res._host_ip, p_resource->edt_p_res._tcp_port , p_resource->edt_p_res._udp_port, p_resource->edt_p_res._res_level, P2P_FROM_ASSIST_DOWNLOAD );
				}
				else
					ret_val = DT_ERR_INVALID_GCID;
			}
	   	}
#endif
		else
		{
			ret_val = DT_ERR_INVALID_TASK_TYPE;
		}
	}
	else
		ret_val = DT_ERR_INVALID_RESOURCE_TYPE;

	LOG_DEBUG("dt_add_assist_task_res : result = %d\n", ret_val);
	
	return ret_val;
}


_int32 dt_get_assist_task_res_info( TASK * p_task, _u32 file_index,void * p_lx_info )
{
	_int32 ret_val = SUCCESS;
	ASSIST_PEER_INFO *p_info = (ASSIST_PEER_INFO* )p_lx_info;
	sd_memset(p_info,0x00,sizeof(ASSIST_PEER_INFO));
	if(p_task->_task_type!=BT_TASK_TYPE)
	{
		file_index = INVALID_FILE_INDEX;
	}
	ret_val =  cm_get_assist_peer_info( &(p_task->_connect_manager) ,file_index,p_info);
	if(ret_val!=SUCCESS) return ret_val;

	if(p_info->_res_num != 0 )
	{
		switch(p_task->_task_type)
		{
			case P2SP_TASK_TYPE:
				dm_get_assist_peer_dl_bytes( &(((P2SP_TASK *)p_task)->_data_manager), &p_info->_dl_bytes  );
				break;
#ifdef ENABLE_BT
			case BT_TASK_TYPE:
			case BT_MAGNET_TASK_TYPE:
				p_info->_errcode = DT_ERR_INVALID_TASK_TYPE;
				break;
#endif

#ifdef ENABLE_EMULE
			case EMULE_TASK_TYPE:
				emule_get_assist_peer_dl_bytes(((EMULE_TASK *)p_task)->_data_manager, &p_info->_dl_bytes);
				break;
#endif
			default:
				break;
		}
	}
	
	LOG_DEBUG("dt_get_assist_task_res_info : result = %d, res_num = %d, speed = %d, dl_bytes = %llu", 
		ret_val, p_info->_res_num, p_info->_speed, p_info->_dl_bytes);
	
	return ret_val;
}

_int32 dt_get_task_downloading_res_info( TASK * p_task, void *p_res_info )
{
	_int32 ret_val = SUCCESS;
	TASK_DOWNLOADING_INFO *p_info = (TASK_DOWNLOADING_INFO* )p_res_info;
	sd_memset(p_info,0x00,sizeof(TASK_DOWNLOADING_INFO));
	P2SP_TASK *p2sp_task = NULL;
	_int32 p2p_speed = 0;
#ifdef ENABLE_EMULE
	EMULE_TASK *emule_task = NULL;
#endif
#ifdef ENABLE_BT
    BT_TASK *p_bt_task = NULL;
#endif
	int i = 0;
	LOG_DEBUG("dt_get_task_downloading_res_info : p_info = 0x%x", p_info);
	p_info->_orgin_res_speed = cm_get_origin_resource_speed( &(p_task->_connect_manager));

	

	if(p_task->_task_type == P2SP_TASK_TYPE 
#ifdef ENABLE_EMULE
	    || p_task->_task_type == EMULE_TASK_TYPE
#endif
	    )
	{
		p_info->_pc_assist_res_num = cm_get_current_connect_manager_assist_peer_res_num(&(p_task->_connect_manager));
		p_info->_pc_assist_res_speed = cm_get_current_connect_manager_assist_peer_speed(&(p_task->_connect_manager));
	}
#ifdef ENABLE_BT
	else if(p_task->_task_type == BT_TASK_TYPE)	///* || p_task->_task_type == BT_MAGNET_TASK_TYPE*/
	{
		p_bt_task = (BT_TASK*)p_task;
        if(p_bt_task)
		{
			MAP_ITERATOR cur_node;
			for(cur_node = MAP_BEGIN( p_bt_task->_file_task_info_map ); 
				cur_node != MAP_END(  p_bt_task->_file_task_info_map ); 
				cur_node = MAP_NEXT(  p_bt_task->_file_task_info_map, cur_node ))
			{
				_u32 file_index = (_u32)MAP_KEY(cur_node);
				CONNECT_MANAGER *p_sub_connect_manager = NULL;
				if(SUCCESS == cm_get_sub_connect_manager(&(p_task->_connect_manager), file_index, &p_sub_connect_manager))
				{
					if(p_sub_connect_manager)
					{	
						p_info->_pc_assist_res_num += cm_get_current_connect_manager_assist_peer_res_num(p_sub_connect_manager);
						p_info->_pc_assist_res_speed += cm_get_current_connect_manager_assist_peer_speed(p_sub_connect_manager);
					}
				}
			}
        }
	}
#endif //ENABLE_BT
	switch(p_task->_task_type)
	{
		case P2SP_TASK_TYPE:
			p2sp_task = (P2SP_TASK *)p_task;
			dm_get_origin_resource_dl_bytes( &p2sp_task->_data_manager, &p_info->_orgin_res_dl_bytes);
			dm_get_assist_peer_dl_bytes( &(p2sp_task->_data_manager), &p_info->_pc_assist_res_dl_bytes);
			p_info->_downloaded_data_size = dm_get_download_data_size( &(p2sp_task->_data_manager) );
			break;
#ifdef ENABLE_BT
		case BT_TASK_TYPE:
		case BT_MAGNET_TASK_TYPE:
             p_bt_task = (BT_TASK *)p_task;
             if (p_bt_task != NULL) {
                // 1、原始资源
                // 2、总资源数据
                bt_update_file_info(p_bt_task);
                p_info->_orgin_res_dl_bytes 
                    = p_bt_task->_data_manager._bt_file_manager._origin_dl_bytes;
                p_info->_downloaded_data_size 
                    = bdm_get_total_file_download_data_size(&(p_bt_task->_data_manager));
				bfm_get_assist_peer_dl_size(&(p_bt_task->_data_manager._bt_file_manager), &(p_info->_pc_assist_res_dl_bytes));
             }
        break;
#endif
			

#ifdef ENABLE_EMULE		
		case EMULE_TASK_TYPE:
			emule_task = (EMULE_TASK *)p_task;
			emule_get_origin_resource_dl_bytes(emule_task->_data_manager, &p_info->_orgin_res_dl_bytes);
			emule_get_assist_peer_dl_bytes(emule_task->_data_manager, &p_info->_pc_assist_res_dl_bytes);
			p_info->_downloaded_data_size = dm_get_download_data_size(&(emule_task->_data_manager->_file_info));
			break;
#endif
		default:
			break;
	}
	LOG_DEBUG("dt_get_task_downloading_res_info : orgin_res_speed = %d, \
        orgin_res_dl_bytes = %llu, downloaded_data_size= %llu, \
        _pc_assist_res_num = %d, _pc_assist_res_speed = %d, \
        _pc_assist_res_dl_bytes = %llu", 
		p_info->_orgin_res_speed, p_info->_orgin_res_dl_bytes,
		p_info->_downloaded_data_size, p_info->_pc_assist_res_num, 
		p_info->_pc_assist_res_speed, p_info->_pc_assist_res_dl_bytes);

    p_info->_all_download_speed     = cm_get_current_task_speed(&(p_task->_connect_manager));
#ifdef _CONNECT_DETAIL
	p_info->_idle_server_res_num     = cm_get_idle_server_res_num(&(p_task->_connect_manager));
	p_info->_idle_peer_res_num       = cm_get_idle_peer_res_num(&(p_task->_connect_manager));
	p_info->_using_server_res_num    = cm_get_using_server_res_num(&(p_task->_connect_manager));
	p_info->_using_peer_res_num      = cm_get_using_peer_res_num(&(p_task->_connect_manager));	
	p_info->_candidate_server_res_num= cm_get_candidate_server_res_num(&(p_task->_connect_manager));
	p_info->_candidate_server_res_num= cm_get_candidate_peer_res_num(&(p_task->_connect_manager));
	p_info->_retry_server_res_num    = cm_get_retry_server_res_num(&(p_task->_connect_manager));
	p_info->_retry_peer_res_num      = cm_get_retry_peer_res_num(&(p_task->_connect_manager));
	p_info->_discard_server_res_num  = cm_get_discard_server_res_num(&(p_task->_connect_manager));
	p_info->_discard_peer_res_num    = cm_get_discard_peer_res_num(&(p_task->_connect_manager));
	LOG_DEBUG("dt_get_task_downloading_res_info from task, \
		_using_server_res_num = %u, _using_peer_res_num= %u, \
        _idle_server_res_num = %u, _idle_peer_res_num = %u", p_task->using_server_res_num, 
        p_task->using_peer_res_num, p_task->idle_server_res_num, p_task->idle_peer_res_num);
#endif
	LOG_DEBUG("dt_get_task_downloading_res_info : _all_download_speed = %u, \
        _using_server_res_num = %u, _using_peer_res_num= %u, \
        _idle_server_res_num = %u, _idle_peer_res_num = %u", 
		p_info->_all_download_speed, p_info->_using_server_res_num,
		p_info->_using_peer_res_num, p_info->_idle_server_res_num, 
		p_info->_idle_peer_res_num);

#ifdef ENABLE_HSC

	cus_rws_begin_read_data(g_task_hsc_info_brd_ptr, NULL);
	
	for(i = 0; i < MAX_ET_ALOW_TASKS; i++)
	{
		if(p_task->_task_id==g_task_hsc_info[i]._task_id)
		{
			p_info->_state = g_task_hsc_info[i]._hsc_info._stat;
			p_info->_res_num = g_task_hsc_info[i]._hsc_info._res_num;
			p_info->_speed = g_task_hsc_info[i]._hsc_info._speed;
			p_info->_dl_bytes = g_task_hsc_info[i]._hsc_info._dl_bytes;
			p_info->_errcode = g_task_hsc_info[i]._hsc_info._errcode;

			//p_info->_all_download_speed += p_info->_speed;
                	LOG_DEBUG("dt_get_task_downloading_res_info : hsc data _state = %d, \
                        _res_num = %u, _speed= %u, \
                        _dl_types = %llu, _errcode = %d", 
                		p_info->_state, p_info->_res_num,
                		p_info->_speed, p_info->_dl_bytes, 
                		p_info->_errcode);
			
			break;
		}
	}
	
	cus_rws_end_read_data(g_task_hsc_info_brd_ptr);
#endif

	return ret_val;
}

_int32 dt_get_assist_task_info_impl(TASK* p_task, void * p_assist_task_info)
{
	P2SP_TASK *p2sp_task = NULL;

#ifdef ENABLE_EMULE
	EMULE_TASK *emule_task = NULL;
#endif

#ifdef ENABLE_BT
	BT_TASK *p_bt_task = NULL;
#endif
	_int32 ret_val = SUCCESS;
	ASSIST_TASK_INFO *p_info = (ASSIST_TASK_INFO* )p_assist_task_info;
	if(p_info == NULL)
	{
		sd_assert(FALSE);
		return -1;
	}
	sd_memset(p_info, 0x00, sizeof(ASSIST_TASK_INFO));
	p_info->_file_index = INVALID_FILE_INDEX;

	switch(p_task->_task_type)
	{
		case P2SP_TASK_TYPE:
			p2sp_task = (P2SP_TASK *)p_task;
			if(p2sp_task)
			{
				_u8 tmp[CID_SIZE+1]; 
				
				sd_memset(tmp, 0x00, sizeof(tmp));
				if(dm_get_shub_gcid(&(p2sp_task->_data_manager), tmp))
				{
					str2hex((const char*)tmp, CID_SIZE, p_info->_gcid, CID_SIZE*2);
				}
				else
				{
					ret_val = -1;
				}

				sd_memset(tmp, 0x00, sizeof(tmp));
				if(dm_get_cid(&(p2sp_task->_data_manager), tmp))
				{
					str2hex((const char*)tmp, CID_SIZE, p_info->_cid, CID_SIZE*2);
				}
				else
				{
					ret_val = -1;
				}

				p_info->_file_size = dm_get_file_size( &(p2sp_task->_data_manager) );
				if(p_info->_file_size == 0)
				{
					ret_val = -1;
				}
			}
			break;
#ifdef ENABLE_BT
		case BT_MAGNET_TASK_TYPE:
			ret_val = -1;
			break;
		case BT_TASK_TYPE:
             p_bt_task = (BT_TASK *)p_task;
             if(p_bt_task)
			 {
				BT_SUB_FILE* p_bt_sub_file = NULL;
				MAP *map = &(p_bt_task->_data_manager._bt_file_manager._bt_sub_file_map);
				if(map_size(map))
				{
					MAP_ITERATOR cur_node = MAP_BEGIN( *map );
					while( cur_node != MAP_END( *map ) )
					{
						BT_FILE_INFO *p_file_info = NULL;
						BT_SUB_FILE* p_sub_file = (BT_SUB_FILE *)MAP_VALUE( cur_node );
						int ret = map_find_node(&(p_bt_task->_file_info_map), (void*) (p_sub_file->_file_index), (void**)&p_file_info);
						if(ret == SUCCESS && p_file_info != NULL)
						{
							if( p_file_info->_file_status == BT_FILE_DOWNLOADING 
								&& p_file_info->_accelerate_state == BT_ACCELERATING
								&& p_sub_file->_file_index <= p_info->_file_index)
							{
								p_bt_sub_file = p_sub_file;
								p_info->_file_index = p_sub_file->_file_index;
							}
						}
						cur_node = MAP_NEXT( *map, cur_node);
					}
				}

				if(p_bt_sub_file)
				{
					p_info->_file_index = p_bt_sub_file->_file_index;
					
					_u8 tmp[CID_SIZE+1]; 
					sd_memset(tmp, 0x00, sizeof(tmp));
					if(bfm_get_shub_gcid(&(p_bt_task->_data_manager._bt_file_manager), p_info->_file_index, tmp))
					{
						str2hex((const char*)tmp, CID_SIZE, p_info->_gcid, CID_SIZE*2);
					}
					else
					{
						ret_val = -1;
					}

					sd_memset(tmp, 0x00, sizeof(tmp));
					if(bfm_get_cid(&(p_bt_task->_data_manager._bt_file_manager), p_info->_file_index, tmp))
					{
						str2hex((const char*)tmp, CID_SIZE, p_info->_cid, CID_SIZE*2);
					}
					else
					{
						ret_val = -1;
					}
					
					p_info->_file_size = p_bt_sub_file->_file_size;
					if(p_info->_file_size == 0)
					{
						ret_val = -1;
					}
					
					if(sd_is_cid_valid(p_bt_task->_torrent_parser_ptr->_info_hash))
					{
						str2hex((const char*)(p_bt_task->_torrent_parser_ptr->_info_hash), CID_SIZE, p_info->_info_id, CID_SIZE*2);
					}
					else
					{
						ret_val = -1;
					}
					
					p_info->_download_size = bfm_get_sub_file_download_data_size(&(p_bt_task->_data_manager._bt_file_manager), p_info->_file_index);
				}
				else
				{
					ret_val = -1;
				}
             }
        break;
#endif

#ifdef ENABLE_EMULE

		case EMULE_TASK_TYPE:
			emule_task = (EMULE_TASK *)p_task;
			if(emule_task)
			{
				_u8  tmp[CID_SIZE+1]; 
				sd_memset(tmp, 0x00, sizeof(tmp));
				if(emule_get_shub_gcid(emule_task->_data_manager, tmp))
				{
					str2hex((const char*)tmp, CID_SIZE, p_info->_gcid, CID_SIZE*2);
				}	
				else
				{
					ret_val = -1;
				}

				sd_memset(tmp, 0x00, sizeof(tmp));
				if(emule_get_shub_cid(emule_task->_data_manager, tmp))
				{
					str2hex((const char*)tmp, CID_SIZE, p_info->_cid, CID_SIZE*2);
				}
				else
				{
					ret_val = -1;
				}
				
				p_info->_file_size = emule_get_file_size(emule_task->_data_manager);
				if(p_info->_file_size == 0)
				{
					ret_val = -1;
				}
			}
			break;
#endif
		default:
			break;
	}
	
	LOG_DEBUG("dt_get_assist_task_info_impl : result = %d, gcid = %s, cid = %s, \
		file_size = %llu, info_id = %s, file_index = %d, \
		download_size = %llu", 
		ret_val, p_info->_gcid, p_info->_cid, p_info->_file_size, p_info->_info_id, 
		p_info->_file_index, p_info->_download_size);
	
	return ret_val;
}

_int32 dt_set_recv_data_from_assist_pc_only_impl(TASK* p_task, EDT_RES * p_resource, ASSIST_TASK_PROPERTY * p_task_prop)
{
	
	
	_int32 ret_val = 0;

	if(p_task == NULL || p_resource == NULL)
	{
		return INVALID_ARGUMENT;
	}

	if(p_task->_use_assist_pc_res_only == FALSE)
	{
		ret_val = cm_pause_pipes(&p_task->_connect_manager);
		LOG_DEBUG("dt_set_recv_data_from_assist_pc_only_impl: pause all pipes result = %d", ret_val);
		if(ret_val != SUCCESS)
		{
			p_task->_use_assist_pc_res_only = FALSE;
			
			sd_memset(p_task->_gcid, 0, CID_SIZE);
			sd_memset(p_task->_cid, 0, CID_SIZE);
			p_task->_file_size = 0;
			
			return ret_val;
		}
	}
	else if(p_task->_use_assist_pc_res_only == TRUE && 
		(   (p_resource->edt_p_res._host_ip != p_task->_p_assist_res->edt_p_res._host_ip) 
			|| (p_resource->edt_p_res._tcp_port != p_task->_p_assist_res->edt_p_res._tcp_port) 
			|| (p_resource->edt_p_res._udp_port != p_task->_p_assist_res->edt_p_res._udp_port) 
		))
	{
		ret_val = cm_destroy_all_pipes(&p_task->_connect_manager);
		LOG_DEBUG("dt_set_recv_data_from_assist_pc_only_impl: destroy assist pipes result = %d", ret_val);
		if(ret_val != SUCCESS)
		{
			p_task->_use_assist_pc_res_only = FALSE;
			
			sd_memset(p_task->_gcid, 0, CID_SIZE);
			sd_memset(p_task->_cid, 0, CID_SIZE);
			p_task->_file_size = 0;
			
			return ret_val;
		}
	}
	else
	{
		return ERR_RESOURCE_EXIST;
	}

	ret_val = dt_add_assist_task_res(p_task, p_resource);
	LOG_DEBUG("dt_set_recv_data_from_assist_pc_only_impl: add assist res result = %d", ret_val);
	if(ret_val != SUCCESS && ret_val != CM_ADD_ACTIVE_PEER_EXIST)
	{
		p_task->_use_assist_pc_res_only = FALSE;
		
		sd_memset(p_task->_gcid, 0, CID_SIZE);
		sd_memset(p_task->_cid, 0, CID_SIZE);
		p_task->_file_size = 0;

		p_task->_use_assist_peer = FALSE;
		if(p_task->_p_assist_res)
		{
			sd_free(p_task->_p_assist_res);
			p_task->_p_assist_res = NULL;
		}
		if(p_task->_p_assist_pipe)
		{
			p2p_pipe_close(p_task->_p_assist_pipe);
			p_task->_p_assist_pipe = NULL;
		}
		
		cm_resume_pipes(&p_task->_connect_manager);
		LOG_DEBUG("dt_set_recv_data_from_assist_pc_only_impl: resume all pipes result = %d", ret_val);

		return ret_val;
	}

	p_task->_use_assist_pc_res_only = TRUE;
	if(p_task->_task_type == P2SP_TASK_TYPE && p_task_prop)
	{
		_u8 gcid[CID_SIZE] = {0};
		_u8 cid[CID_SIZE] = {0};

		hex2str((char*)p_task_prop->_gcid, CID_SIZE*2, (char*)gcid, CID_SIZE);
		hex2str((char*)p_task_prop->_cid, CID_SIZE*2, (char*)cid, CID_SIZE);
	
		P2SP_TASK* p2sp_task = (P2SP_TASK*)p_task;

		if(sd_is_cid_valid(gcid))
		{
			sd_memcpy(p_task->_gcid, p_task_prop->_gcid, CID_SIZE);
			dm_set_gcid(&p2sp_task->_data_manager, gcid);
			p2sp_task->_is_gcid_ok = TRUE;
		}

		if(sd_is_cid_valid(cid))
		{
			sd_memcpy(p_task->_cid, p_task_prop->_cid, CID_SIZE);
			dm_set_cid(&p2sp_task->_data_manager, cid);
			p2sp_task->_is_cid_ok = TRUE;
		}

		if(p_task_prop->_file_size)
		{
			p_task->_file_size = p_task_prop->_file_size;
			dm_set_file_size(&p2sp_task->_data_manager, p_task_prop->_file_size, TRUE);
		}
	}

	LOG_DEBUG("dt_set_recv_data_from_assist_pc_only_impl: result = %d", ret_val);
	return ret_val;
}


#endif /* __DOWNLOAD_TASK_C_20080916 */
