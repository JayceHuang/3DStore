#if !defined(__P2SP_QUERY_C_20090319)
#define __P2SP_QUERY_C_20090319
/*--------------------------------------------------------------------------*/
/*                               IDENTIFICATION                             */
/*--------------------------------------------------------------------------*/
/*     Filename  : p2sp_query.c                                         */
/*     Author     : ZengYuqing                                              */
/*     Project    : EmbedThunder                                        */
/*     Module     : p2sp_download													*/
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
/* This file contains the procedure of p2sp resource query                       */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*                              HISTORY                                     */
/*--------------------------------------------------------------------------*/
/*   Date     |    Author   | Modification                                  */
/*--------------------------------------------------------------------------*/
/* 2009.03.19 | ZengYuqing  | Creation                                      */
/*--------------------------------------------------------------------------*/


#include "p2sp_task.h"
#include "p2sp_query.h"

#include "utility/settings.h"


#include "res_query/res_query_interface.h"
#include "utility/time.h"
#include "utility/utility.h"
#include "data_manager/data_manager_interface.h"
#include "reporter/reporter_interface.h"
#include "utility/base64.h"
#include "utility/url.h"
#include "utility/sd_iconv.h"
#include "platform/sd_fs.h"


#include "utility/logid.h"
#ifdef LOGID
#undef LOGID
#endif

#define LOGID LOGID_P2SP_TASK

#include "utility/logger.h"

_int32 pt_start_query(TASK * p_task)
{
	_int32 ret_val = SUCCESS;
	_int32 bonus_res_num = DEFAULT_BONUS_RES_NUM;
	 char * origin_url = NULL,*ref_origin_url=NULL;
	 _u8 cid[CID_SIZE],gcid[CID_SIZE];
	 _u64 file_size = 0;
	P2SP_TASK * p_p2sp_task = (P2SP_TASK *)p_task;

	if(p_p2sp_task->_res_query_shub_state == RES_QUERY_REQING) return SUCCESS;
 
	LOG_DEBUG("pt_start_query:_task_id=%u,_task_type=%d,_task_created_type=%d",p_task->_task_id,p_task->_task_type,p_p2sp_task->_task_created_type);

	BOOL bhub_ok = ((p_p2sp_task->_is_gcid_ok == TRUE)&&(dm_get_shub_gcid( &(p_p2sp_task->_data_manager),  gcid)==TRUE) && p_p2sp_task->_is_shub_ok);

	LOG_DEBUG( "MMMM pt_start_query:bhub_ok:%d, p_p2sp_task->_data_manager._has_bub_info:%d", bhub_ok, p_p2sp_task->_data_manager._has_bub_info);
	if(p_p2sp_task->_data_manager._has_bub_info && !bhub_ok)
	{
		LOG_ERROR( "MMMM pt_start_query:res_query_shub_by_cid 1 but has bub info is TURE, NOT query..");
		return SUCCESS;
	}
	
	
	/* 局域网任务不查资源 */
	if(p_p2sp_task->_is_lan)
	{
		p_p2sp_task->_res_query_shub_state = RES_QUERY_END;
		p_p2sp_task->_res_query_cdn_state = RES_QUERY_END;
		p_p2sp_task->_res_query_normal_cdn_state = RES_QUERY_END;
#ifdef ENABLE_KANKAN_CDN
		p_p2sp_task->_res_query_kankan_cdn_state = RES_QUERY_END;
#endif
		p_p2sp_task->_res_query_phub_state = RES_QUERY_END;
		p_p2sp_task->_res_query_tracker_state = RES_QUERY_END;
		p_p2sp_task->_res_query_partner_cdn_state = RES_QUERY_END;
        p_p2sp_task->_res_query_dphub_state = RES_QUERY_END;
		return SUCCESS;
	}

	settings_get_int_item("res_query._bonus_res_num",&bonus_res_num);
	p_p2sp_task->_res_query_para._task_ptr = p_task;
	p_p2sp_task->_res_query_para._file_index = INVALID_FILE_INDEX;

	 switch(p_p2sp_task->_task_created_type)
	 {
	 	case TASK_CREATED_BY_URL: 
				if(dm_get_cid( &(p_p2sp_task->_data_manager),  cid)==TRUE)
				{
					file_size = dm_get_file_size(&(p_p2sp_task->_data_manager));
					/* Query the shub for resources */
					p_p2sp_task->_res_query_shub_state = RES_QUERY_REQING;
					LOG_ERROR( "MMMM pt_start_query:res_query_shub_by_cid 1");
					if((p_p2sp_task->_is_gcid_ok == TRUE)&&(dm_get_shub_gcid( &(p_p2sp_task->_data_manager),  gcid)==TRUE) && p_p2sp_task->_is_shub_ok)
					{
#ifdef ENABLE_HSC
						pt_start_query_vip_hub( p_task, cid, gcid, file_size, bonus_res_num);
#endif /* ENABLE_HSC */
						pt_start_query_phub_tracker_cdn( p_task, cid, gcid, file_size, bonus_res_num);

						ret_val =  res_query_shub_by_resinfo_newcmd(&(p_p2sp_task->_res_query_para), pt_notify_only_res_query_shub ,cid,file_size ,TRUE,(const char*)gcid,	TRUE, MAX_SERVER,bonus_res_num, p_p2sp_task->_query_shub_newres_times_sn++,1);
						if(ret_val!=SUCCESS) goto ErrorHanle;

						break;						
					}
					
				}
				
				/* Get origin url and ref url from data manager*/
				 if(FALSE ==  dm_get_origin_url(&(p_p2sp_task->_data_manager), &origin_url))
				{
					ret_val =DT_ERR_GETTING_ORIGIN_URL;
					goto ErrorHanle;
				 }
				if(p_p2sp_task->_is_lan)
				{
					/* 尝试从局域网URL中解析出cid和file_size */
					ret_val = sd_get_cid_filesize_from_lan_url(origin_url,cid,&file_size);
					//sd_assert(ret_val==SUCCESS);
					if(ret_val==SUCCESS)
					{
						p_p2sp_task->_res_query_shub_state = RES_QUERY_REQING;
						p_p2sp_task->_res_query_shub_step=RQSS_ORIGIN;
						ret_val =  res_query_shub_by_cid_newcmd(&(p_p2sp_task->_res_query_para), pt_notify_res_query_shub,cid,file_size ,FALSE,DEFAULT_LOCAL_URL, 	TRUE, MAX_SERVER,bonus_res_num, p_p2sp_task->_query_shub_times_sn++,1);
						if(ret_val!=SUCCESS) goto ErrorHanle;
						break;
					}
				}
				dm_get_origin_ref_url(&(p_p2sp_task->_data_manager), &ref_origin_url);
				LOG_ERROR( "MMMM pt_start_query:res_query_shub_by_url 1 real origion url:%s, length:%u", p_p2sp_task->_origion_url, p_p2sp_task->_origion_url_length);
				
				if(p_p2sp_task->_origion_url_length > 0 && sd_strlen(p_p2sp_task->_origion_url) > 0)
				{
					origin_url = p_p2sp_task->_origion_url;
				}
				/* Query the shub for resources */
				p_p2sp_task->_res_query_shub_state = RES_QUERY_REQING;
				p_p2sp_task->_res_query_shub_step = RQSS_ORIGIN;
				LOG_ERROR( "MMMM pt_start_query:res_query_shub_by_url 1:origin_url=%s,ref_origin_url=%s",origin_url,ref_origin_url);
				ret_val =  res_query_shub_by_url_newcmd(&(p_p2sp_task->_res_query_para), pt_notify_res_query_shub,origin_url,ref_origin_url ,TRUE, MAX_SERVER,bonus_res_num, p_p2sp_task->_query_shub_times_sn++,FALSE);
				if(ret_val!=SUCCESS) goto ErrorHanle;
			break;
		case TASK_CONTINUE_BY_URL: 
			/* Get origin url and ref url from data manager*/
			 if(FALSE ==  dm_get_origin_url(&(p_p2sp_task->_data_manager), &origin_url))
			{
				ret_val =DT_ERR_GETTING_ORIGIN_URL;
				goto ErrorHanle;
			 }
			if(p_p2sp_task->_is_lan)
			{
				/* 尝试从局域网URL中解析出cid和file_size */
				ret_val = sd_get_cid_filesize_from_lan_url(origin_url,cid,&file_size);
				//sd_assert(ret_val==SUCCESS);
				if(ret_val==SUCCESS)
			{
					p_p2sp_task->_res_query_shub_state = RES_QUERY_REQING;
					p_p2sp_task->_res_query_shub_step=RQSS_ORIGIN;
					ret_val =  res_query_shub_by_cid_newcmd(&(p_p2sp_task->_res_query_para), pt_notify_res_query_shub,cid,file_size ,FALSE,DEFAULT_LOCAL_URL, 	TRUE, MAX_SERVER,bonus_res_num, p_p2sp_task->_query_shub_times_sn++,1);
					if(ret_val!=SUCCESS) goto ErrorHanle;
					break;
				}
			}
			if(dm_get_cid( &(p_p2sp_task->_data_manager),  cid)==TRUE)
			{
				file_size = dm_get_file_size(&(p_p2sp_task->_data_manager));
				/* Query the shub for resources */
				p_p2sp_task->_res_query_shub_state = RES_QUERY_REQING;
				LOG_ERROR( "MMMM pt_start_query:res_query_shub_by_cid 1");
				if((p_p2sp_task->_is_gcid_ok == TRUE)&&(dm_get_shub_gcid( &(p_p2sp_task->_data_manager),  gcid)==TRUE) && p_p2sp_task->_is_shub_ok)
				{
#ifdef ENABLE_HSC
					pt_start_query_vip_hub( p_task, cid, gcid, file_size, bonus_res_num);
#endif /* ENABLE_HSC */
					pt_start_query_phub_tracker_cdn( p_task, cid, gcid, file_size, bonus_res_num);

					ret_val =  res_query_shub_by_resinfo_newcmd(&(p_p2sp_task->_res_query_para), pt_notify_res_query_shub,cid,file_size ,TRUE,(const char*)gcid,	TRUE, MAX_SERVER,bonus_res_num, p_p2sp_task->_query_shub_newres_times_sn++,1);
					if(ret_val!=SUCCESS) goto ErrorHanle;
					break;
					
				}
			}

			dm_get_origin_ref_url(&(p_p2sp_task->_data_manager), &ref_origin_url);
			/* Query the shub for resources */
			p_p2sp_task->_res_query_shub_state = RES_QUERY_REQING;
			p_p2sp_task->_res_query_shub_step=RQSS_ORIGIN;
			LOG_ERROR( "MMMM pt_start_query:res_query_shub_by_url 2:origin_url=%s,ref_origin_url=%s",origin_url,ref_origin_url);
			ret_val =  res_query_shub_by_url_newcmd(&(p_p2sp_task->_res_query_para), pt_notify_res_query_shub,origin_url,ref_origin_url ,TRUE, MAX_SERVER,bonus_res_num, p_p2sp_task->_query_shub_times_sn++,FALSE);
			if(ret_val!=SUCCESS) goto ErrorHanle;
			break;
		case TASK_CREATED_BY_TCID: 
		case TASK_CONTINUE_BY_TCID: 
		case TASK_CREATED_BY_GCID: 
			if(dm_get_cid( &(p_p2sp_task->_data_manager),  cid)==TRUE)
			{
				file_size = dm_get_file_size(&(p_p2sp_task->_data_manager));
				/* Query the shub for resources */
				p_p2sp_task->_res_query_shub_state = RES_QUERY_REQING;
				LOG_ERROR( "MMMM pt_start_query:res_query_shub_by_cid 2");
				if((p_p2sp_task->_is_gcid_ok == TRUE)&&(dm_get_shub_gcid( &(p_p2sp_task->_data_manager),  gcid)==TRUE) && p_p2sp_task->_is_shub_ok)
				{
					//if(p_p2sp_task->_task_created_type!=TASK_CREATED_BY_TCID)   //Why ???
					{
#ifdef ENABLE_HSC
						pt_start_query_vip_hub( p_task, cid, gcid, file_size, bonus_res_num);
#endif /* ENABLE_HSC */
						pt_start_query_phub_tracker_cdn( p_task, cid, gcid, file_size, bonus_res_num);
					}
					
					ret_val =  res_query_shub_by_resinfo_newcmd(&(p_p2sp_task->_res_query_para), pt_notify_only_res_query_shub,cid,file_size ,	TRUE,(const char*)gcid, TRUE,MAX_SERVER,bonus_res_num, p_p2sp_task->_query_shub_newres_times_sn++,1);
					if(ret_val!=SUCCESS) goto ErrorHanle;
				}
				else
				{
					ret_val =  res_query_shub_by_cid_newcmd(&(p_p2sp_task->_res_query_para), pt_notify_res_query_shub,cid,file_size ,	FALSE,DEFAULT_LOCAL_URL, TRUE,MAX_SERVER,bonus_res_num, p_p2sp_task->_query_shub_times_sn++,1);
					if(ret_val!=SUCCESS) goto ErrorHanle;
				}
			}
			else
			{
					ret_val =DT_ERR_GETTING_CID;
					goto ErrorHanle;
			}
			break;
	 }
	 
#ifdef EMBED_REPORT
	sd_time_ms( &(p_task->_res_query_report_para._shub_query_time) );
#endif	 

	 return SUCCESS;
	 
ErrorHanle:
	
	if(ret_val !=SUCCESS)
	{
		p_p2sp_task->_res_query_shub_state = RES_QUERY_IDLE;
	}

		return ret_val;

}
_int32 pt_stop_query(TASK * p_task)
{
	P2SP_TASK * p_p2sp_task = (P2SP_TASK *)p_task;

	LOG_DEBUG("pt_stop_query");
	
		if(p_p2sp_task->_res_query_shub_state == RES_QUERY_REQING)
		{
			/* Cancel the res_query... */
			res_query_cancel(&(p_p2sp_task->_res_query_para), SHUB);

			p_p2sp_task->_res_query_shub_state = RES_QUERY_END;
		}
		
		if(p_p2sp_task->_res_query_phub_state == RES_QUERY_REQING)
		{
			/* Cancel the res_query... */
			res_query_cancel(&(p_p2sp_task->_res_query_para), PHUB);

			p_p2sp_task->_res_query_phub_state = RES_QUERY_END;
		}
		
		if(p_p2sp_task->_res_query_tracker_state == RES_QUERY_REQING)
		{
			/* Cancel the res_query... */
			res_query_cancel(&(p_p2sp_task->_res_query_para),TRACKER);

			p_p2sp_task->_res_query_tracker_state  = RES_QUERY_END;
		}
		
#ifdef ENABLE_CDN
		if(p_p2sp_task->_res_query_cdn_state == RES_QUERY_REQING)
		{
			/* Cancel the res_query... */
			res_query_cancel(&(p_p2sp_task->_res_query_para),CDN_MANAGER);

			p_p2sp_task->_res_query_cdn_state  = RES_QUERY_END;
		}
		if (p_p2sp_task->_res_query_normal_cdn_state == RES_QUERY_REQING)
		{
			res_query_cancel(&(p_p2sp_task->_res_query_para), NORMAL_CDN_MANAGER);

			p_p2sp_task->_res_query_normal_cdn_state = RES_QUERY_END;
		}
#ifdef ENABLE_KANKAN_CDN
		if(p_p2sp_task->_res_query_kankan_cdn_state== RES_QUERY_REQING)
		{
			/* Cancel the res_query... */
			res_query_cancel(&(p_p2sp_task->_res_query_para),KANKAN_CDN_MANAGER);

			p_p2sp_task->_res_query_kankan_cdn_state  = RES_QUERY_END;
		}		
#endif
#ifdef ENABLE_HSC
		if(p_p2sp_task->_res_query_vip_state == RES_QUERY_REQING)
		{
			/* Cancel the res_query... */
			res_query_cancel(&(p_p2sp_task->_res_query_para), VIP_HUB);

			p_p2sp_task->_res_query_vip_state  = RES_QUERY_END;
		}
#endif /* ENABLE_HSC */
#endif /* ENABLE_CDN */

		if(p_p2sp_task->_res_query_partner_cdn_state == RES_QUERY_REQING)
		{
			/* Cancel the res_query... */
			res_query_cancel(&(p_p2sp_task->_res_query_para),PARTNER_CDN);
			p_p2sp_task->_res_query_partner_cdn_state  = RES_QUERY_END;
		}

        //if (p_p2sp_task->_res_query_dphub_state == RES_QUERY_REQING)
        {
            res_query_cancel(&p_p2sp_task->_res_query_para, DPHUB_NODE);
            p_p2sp_task->_res_query_dphub_state = RES_QUERY_END;
        }
		
	LOG_DEBUG("pt_stop_task:_query_shub_timer_id=%d,_query_peer_timer_id=%d"
		,p_p2sp_task->_query_shub_timer_id,p_p2sp_task->_query_peer_timer_id);

	if(0 != p_p2sp_task->_query_shub_timer_id)
	{
		cancel_timer(p_p2sp_task->_query_shub_timer_id);
		p_p2sp_task->_query_shub_timer_id = 0;
	}

	if(0 != p_p2sp_task->_query_peer_timer_id)
	{
		cancel_timer(p_p2sp_task->_query_peer_timer_id);
		p_p2sp_task->_query_peer_timer_id = 0;
	}

	if (p_p2sp_task->_query_normal_cdn_timer_id != 0)
	{
		cancel_timer(p_p2sp_task->_query_normal_cdn_timer_id);
		p_p2sp_task->_query_normal_cdn_timer_id = 0;
	}
		
return SUCCESS;
}
#ifdef ENABLE_HSC
_int32 pt_start_query_vip_hub(TASK * p_task,_u8 * cid,_u8 * gcid,_u64 file_size,_int32 bonus_res_num)
{
	_int32 ret_val = SUCCESS;
	P2SP_TASK * p_p2sp_task = (P2SP_TASK *)p_task;


	LOG_DEBUG("pt_start_query_vip_hub");
	
	/* 局域网任务不查资源 */
	if(p_p2sp_task->_is_lan)
	{
		p_p2sp_task->_res_query_vip_state = RES_QUERY_END;
		return SUCCESS;
	}

	/* VIP HUB */
	if(p_p2sp_task->_res_query_vip_state != RES_QUERY_REQING)
	{
		p_p2sp_task->_res_query_vip_state = RES_QUERY_REQING;
		ret_val = res_query_vip_hub(&(p_p2sp_task->_res_query_para), pt_notify_res_query_vip_hub,cid, gcid, file_size,  bonus_res_num,4);
		if(ret_val !=SUCCESS)
		{
			p_p2sp_task->_res_query_vip_state = RES_QUERY_IDLE;
		}
	}

	return SUCCESS;
}
#endif /* ENABLE_HSC */

_int32 pt_build_res_query_dphub_content(P2SP_TASK *p_p2sp_task)
{
    _int32 ret_val = 0;
    _int16 file_src_type = 1; // 1: 普通完整文件
    _u8 file_idx_desc_type = 0; // 0：无块描述
    _u32 file_idx_content_cnt = 0;
    char *file_idx_content = NULL;
    _u8 cid[CID_SIZE] = {'\0'};
    _u8 gcid[CID_SIZE] = {'\0'};

    LOG_DEBUG("MMMM p2sp_query:res_query_dphub");

#if 0
    /*
    _u64 file_size = 0;
    _u32 block_size = 0;
    _u16 b16_idx = 0, e16_idx = 0, l16_idx = 0;
    _u32 b32_idx = 0, e32_idx = 0, l32_idx = 0;
    _u32 i = 0;
    RANGE_LIST_ITEROATOR range_it;
    _u16 *p_2b_idx = NULL;
    _u32 *p_4b_idx = NULL;
    _u32 file_idx_num = 0;
    RANGE_LIST uncom_rl;
    RANGE_LIST *p_recv_rl;
    */

    range_list_init(&uncom_rl);

    p_recv_rl = dm_get_recved_range_list(&p_p2sp_task->_data_manager);
    dm_get_uncomplete_range(&p_p2sp_task->_data_manager, &uncom_rl);    

    file_src_type = (range_list_size(p_recv_rl) != 0) ? 2 : 1;
    LOG_DEBUG("file_src_type = %d.", file_src_type);

    file_size = dm_get_file_size(&p_p2sp_task->_data_manager);
    block_size = dm_get_block_size(&p_p2sp_task->_data_manager);
    LOG_DEBUG("file_size = %llu, block_size = %u.", file_size, block_size);

    if ((file_src_type != 1) && (block_size != 0))
    {
        // 先预先申请存放足够file index的内存
        // 最差的情况就是每一个range，占用两个idx（起始和结束）
        file_idx_num = 2 * range_list_size(&uncom_rl);
        LOG_DEBUG("range_list_size(&uncom_rl) = %d, file_idx_num = %d.",
            range_list_size(&uncom_rl), file_idx_num);
        if (file_idx_num == 0)
        {
            // 注意：文件较小的话，可能已经下载完成了，所以不应该去查询dphub了
            LOG_DEBUG("There is no uncomplete range, so return.");
            range_list_clear(&uncom_rl);
            return ERR_RES_QUERY_NO_NEED_QUERY_DPHUB;
        }
        
        if (file_size / block_size < 65535)
        {
            LOG_DEBUG("idx-range mode in 16bit per range index.");

            // 16b模式
            ret_val = sd_malloc(2*file_idx_num, (void **)&file_idx_content);
            CHECK_VALUE(ret_val);
            sd_memset((void *)file_idx_content, 0, file_idx_num);
            
            p_2b_idx = (_u16 *)file_idx_content;
            for (i=0; i<range_list_size(&uncom_rl); ++i)
            {
                range_it = uncom_rl._head_node;
                b16_idx = (_u16)(((_u64)range_it->_range._index) * get_data_unit_size() / block_size + 1);
                e16_idx = (_u16)(range_to_length(&range_it->_range, file_size) / block_size + 1);
                if (b16_idx <= e16_idx)
                {
                    if ((file_idx_content == NULL) || (b16_idx > l16_idx + 1))
                    {
                        *p_2b_idx = b16_idx;
                        *(p_2b_idx + 1) = e16_idx;
                        p_2b_idx += 2;
                        file_idx_content_cnt += 2;
                    }
                    else
                    {
                        // 两个range相连
                        *p_2b_idx = e16_idx;
                    }
                    l16_idx = e16_idx;                        
                }
            }
            
            p_2b_idx = (_u16 *)file_idx_content;
            if ((file_idx_content_cnt == 2) && (*p_2b_idx == 1) && (*(p_2b_idx + 1) >= file_size/block_size))
            {
                // 超出文件范围，不再以部分文件的方式查询
                file_idx_desc_type = 0;
                file_idx_content_cnt = 0;
                if (file_idx_content != 0)
                {
                    sd_free(file_idx_content);
                    file_idx_content = 0;
                }
            }
            else
            {
                file_idx_desc_type = 2;
            }
        }
        else
        {
            LOG_DEBUG("idx-range mode in 32bit per range index.");

            // 32b模式
            ret_val = sd_malloc(4*file_idx_num, (void **)&file_idx_content);
            CHECK_VALUE(ret_val);
            sd_memset((void *)file_idx_content, 0, file_idx_num);

            p_4b_idx = (_u32 *)file_idx_content; 
            for (i=0; i<range_list_size(&uncom_rl); ++i)
            {
                range_it = uncom_rl._head_node;
                b32_idx = (_u32)(((_u64)range_it->_range._index) * get_data_unit_size() / block_size + 1);
                e32_idx = (_u32)(range_to_length(&range_it->_range, file_size) / block_size + 1);
                if (b32_idx <= e32_idx)
                {
                    if ((file_idx_content == NULL) || (b32_idx > l32_idx + 1))
                    {
                        *p_4b_idx = b32_idx;
                        *(p_4b_idx + 1) = e32_idx;
                        p_4b_idx += 2;
                        file_idx_content_cnt += 2;
                    }
                    else
                    {
                        // 两个range相连
                        *p_4b_idx = e32_idx;
                    }
                    l32_idx = e32_idx;              
                }
            }

            p_4b_idx = (_u32 *)file_idx_content;
            if ((file_idx_content_cnt == 2) && (*p_2b_idx == 1) && (*(p_2b_idx + 1) >= file_size/block_size))
            {
                // 超出文件范围，不再以部分文件的方式查询
                file_idx_desc_type = 0;
                file_idx_content_cnt = 0;
                if (file_idx_content != 0)
                {
                    sd_free(file_idx_content);
                    file_idx_content = 0;
                }
            }
            else
            {
                file_idx_desc_type = 4;
            }
        }
    }


    range_list_clear(&uncom_rl);

    LOG_DEBUG("file_idx_desc_type = %hhu, file_idx_content_cnt = %u, file_idx_content = 0x%x.", 
        file_idx_desc_type, file_idx_content_cnt, file_idx_content);

#endif

    dm_get_cid(&(p_p2sp_task->_data_manager), cid);    
    dm_get_shub_gcid(&(p_p2sp_task->_data_manager), gcid);
    ret_val = res_query_dphub(&(p_p2sp_task->_res_query_para), pt_notify_query_dphub_result, 
        file_src_type, 
        file_idx_desc_type,
        file_idx_content_cnt,
        file_idx_content, 
        dm_get_file_size(&(p_p2sp_task->_data_manager)), 
        dm_get_block_size(&(p_p2sp_task->_data_manager)),        
        (const char *)cid,
        (const char *)gcid,
        (_u16)P2SP_TASK_TYPE);

    p_p2sp_task->_res_query_dphub_state = (ret_val != SUCCESS) ? RES_QUERY_FAILED : RES_QUERY_REQING;

    if (ret_val == ERR_RES_QUERY_NO_ROOT)
    {
        p_p2sp_task->_res_query_dphub_state = RES_QUERY_IDLE;
        if (p_p2sp_task->_dpub_query_context._wait_dphub_root_timer_id == 0)
        {
            ret_val = start_timer(pt_handle_wait_dphub_root_timeout, 1, WAIT_FOR_DPHUB_ROOT_RETURN_TIMEOUT, 0, 
                (void *)p_p2sp_task, &(p_p2sp_task->_dpub_query_context._wait_dphub_root_timer_id));
            if(ret_val!=SUCCESS)
            {
                dt_failure_exit(&(p_p2sp_task->_task), ret_val);
            }
        }
    }  

    return ret_val;
}

_int32 pt_start_query_normal_cdn(TASK * p_task,_u8 * cid,_u8 * gcid,_u64 file_size,_int32 bonus_res_num)
{
	_int32 ret_val = SUCCESS;
	P2SP_TASK * p_p2sp_task = (P2SP_TASK *)p_task;

	LOG_DEBUG("pt_start_query_normal_cdn, p_p2sp_task->_res_query_normal_cdn_state = %u.", 
		p_p2sp_task->_res_query_normal_cdn_state);

	if((p_p2sp_task->_res_query_normal_cdn_state == RES_QUERY_IDLE) ||
	   (p_p2sp_task->_res_query_normal_cdn_state == RES_QUERY_FAILED))
	{
		// 只有在失败的时候或者第一次才会查询
		p_p2sp_task->_res_query_normal_cdn_state = RES_QUERY_REQING;
		ret_val = res_query_normal_cdn_manager(&(p_p2sp_task->_res_query_para), 
			pt_notify_res_query_normal_cdn, cid, gcid, file_size, bonus_res_num, 4);
		if(ret_val != SUCCESS)
		{
			p_p2sp_task->_res_query_normal_cdn_state = RES_QUERY_IDLE;
		}
		else
		{
#ifdef EMBED_REPORT
			// 暂时先不对这些细节统计
			//sd_time_ms( &(p_task->_res_query_report_para._phub_query_time) );
#endif								
		}
	}

	return ret_val;
}

_int32 pt_start_query_phub_tracker_cdn(TASK * p_task,_u8 * cid,_u8 * gcid,_u64 file_size,_int32 bonus_res_num)
{
	_int32 ret_val = SUCCESS;
	P2SP_TASK * p_p2sp_task = (P2SP_TASK *)p_task;    

	LOG_DEBUG("pt_start_query_phub_tracker_cdn, is_lan = %d", p_p2sp_task->_is_lan);     
	
	/* 局域网任务不查资源 */
	if(p_p2sp_task->_is_lan)
	{
		p_p2sp_task->_res_query_cdn_state = RES_QUERY_END;
		p_p2sp_task->_res_query_phub_state = RES_QUERY_END;
		p_p2sp_task->_res_query_tracker_state = RES_QUERY_END;
		p_p2sp_task->_res_query_partner_cdn_state = RES_QUERY_END;
        p_p2sp_task->_res_query_dphub_state = RES_QUERY_END;
		p_p2sp_task->_res_query_normal_cdn_state = RES_QUERY_END;
		return SUCCESS;
	}

#ifdef ENABLE_CDN
	if(	(p_p2sp_task->_is_vod_task==TRUE)&&
		(p_p2sp_task->_res_query_cdn_state != RES_QUERY_REQING)&&
		(p_p2sp_task->_res_query_cdn_state != RES_QUERY_SUCCESS)&&
		(cm_is_need_more_cdn_res( &(p_task->_connect_manager),INVALID_FILE_INDEX )))
	{
		p_p2sp_task->_res_query_cdn_state = RES_QUERY_REQING;
		ret_val = res_query_cdn_manager(CDN_VERSION,gcid,file_size,pt_notify_res_query_cdn,&(p_p2sp_task->_res_query_para));
		if(ret_val !=SUCCESS)
		{
			p_p2sp_task->_res_query_cdn_state = RES_QUERY_IDLE;
		}
	}

	pt_start_query_normal_cdn(p_task, cid, gcid, file_size, bonus_res_num);	

#ifdef ENABLE_KANKAN_CDN
	if(	(p_p2sp_task->_is_vod_task==TRUE)&&
		(p_p2sp_task->_res_query_kankan_cdn_state != RES_QUERY_REQING)&&
		(p_p2sp_task->_res_query_kankan_cdn_state != RES_QUERY_SUCCESS)&&
		(cm_is_need_more_cdn_res( &(p_task->_connect_manager),INVALID_FILE_INDEX )))
	{
		p_p2sp_task->_res_query_kankan_cdn_state = RES_QUERY_REQING;
		ret_val = res_query_kankan_cdn_manager(CDN_VERSION,gcid,file_size,pt_notify_res_query_kankan_cdn,&(p_p2sp_task->_res_query_para));
		if(ret_val !=SUCCESS)
		{
			p_p2sp_task->_res_query_kankan_cdn_state = RES_QUERY_IDLE;
		}
	}
#endif				
#endif /* ENABLE_CDN */

	if(p_p2sp_task->_res_query_phub_state != RES_QUERY_REQING)
	{
		p_p2sp_task->_res_query_phub_state = RES_QUERY_REQING;
		ret_val = res_query_phub(&(p_p2sp_task->_res_query_para), pt_notify_res_query_phub,cid, gcid, file_size,  bonus_res_num,4);
		if(ret_val != SUCCESS)
		{
			p_p2sp_task->_res_query_phub_state = RES_QUERY_IDLE;
		}
		else
		{
#ifdef EMBED_REPORT
			sd_time_ms( &(p_task->_res_query_report_para._phub_query_time) );
#endif								
		}

	}

    // 查询dphub
    if (p_p2sp_task->_res_query_dphub_state != RES_QUERY_REQING)
    {
        ret_val = pt_build_res_query_dphub_content(p_p2sp_task);

        if (ret_val == ERR_RES_QUERY_NO_NEED_QUERY_DPHUB)
            p_p2sp_task->_res_query_dphub_state = RES_QUERY_END;
        else
            p_p2sp_task->_res_query_dphub_state 
            = (ret_val == SUCCESS) ? RES_QUERY_REQING : RES_QUERY_FAILED;
    }
	
	if(p_p2sp_task->_res_query_tracker_state != RES_QUERY_REQING)
	{
		p_p2sp_task->_res_query_tracker_state = RES_QUERY_REQING;
		ret_val = res_query_tracker(&(p_p2sp_task->_res_query_para),pt_notify_res_query_tracker,p_p2sp_task->_last_query_stamp,gcid,  file_size,MAX_PEER,bonus_res_num,0,0);
		if(ret_val !=SUCCESS)
		{
			p_p2sp_task->_res_query_tracker_state = RES_QUERY_IDLE;
		}
		else
		{
#ifdef EMBED_REPORT
			sd_time_ms( &(p_task->_res_query_report_para._tracker_query_time) );
#endif								
		}
	}
	
#if defined( MOBILE_PHONE)
	p_p2sp_task->_res_query_partner_cdn_state = RES_QUERY_END;
#else
	if((p_p2sp_task->_res_query_partner_cdn_state != RES_QUERY_REQING)&&(p_p2sp_task->_res_query_partner_cdn_state != RES_QUERY_SUCCESS))
	{
		p_p2sp_task->_res_query_partner_cdn_state = RES_QUERY_REQING;
		ret_val = res_query_partner_cdn(&(p_p2sp_task->_res_query_para),pt_notify_res_query_partner_cdn,cid,gcid,file_size);
		if(ret_val !=SUCCESS)
		{
			p_p2sp_task->_res_query_partner_cdn_state = RES_QUERY_IDLE;
		}
	}
#endif						 
	return SUCCESS;
}

extern _int32 relation_res_query_shub(void * user_data, res_query_notify_shub_handler handler, const char * url, const char * refer_url, char cid [ CID_SIZE ], char gcid [ CID_SIZE ], _u64 file_size, _int32 max_res, _int32 query_times_sn);
_int32 pt_notify_only_res_query_shub(void* user_data,_int32 errcode, _u8 result, _u64 file_size, _u8* cid, _u8* gcid, _u32 gcid_level, _u8* bcid, _u32 bcid_size, _u32 block_size,_u32 retry_interval,char * file_suffix,_int32 control_flag)
{
	_int32 bonus_res_num = DEFAULT_BONUS_RES_NUM;
	 char * origin_url = NULL,*ref_origin_url=NULL;
	 char  origin_url_converted[MAX_URL_LEN];
	_u8 _gcid_t[CID_SIZE];
	_int32 ret_val = SUCCESS;
	TASK * p_task = (TASK *)((RES_QUERY_PARA *)user_data)->_task_ptr;
	P2SP_TASK * p_p2sp_task = (P2SP_TASK *)p_task;
	_u64 file_size_from_dm=0;

	LOG_DEBUG("pt_notify_only_res_query_shub result==%d", result);

#ifdef EMBED_REPORT
	_u32 cur_time = 0;
	_u32 shub_query_time = 0;
	RES_QUERY_REPORT_PARA *p_report_para = &p_task->_res_query_report_para;
#endif

	sd_assert(user_data!=NULL);
	if(user_data==NULL) return DT_ERR_INVALID_DOWNLOAD_TASK;
	sd_assert(p_p2sp_task!=NULL);
	
	LOG_ERROR("MMMM pt_notify_only_res_query_shub,task_id=%u,errcode=%d,result=%d,file_size=%llu,gcid_level=%d,bcid_size=%u,block_size=%u,retry_interval=%u,file_suffix=%s,control_flag=%d",
		p_task->_task_id,errcode,result,file_size,gcid_level,bcid_size,block_size,retry_interval,file_suffix,control_flag);
#ifdef _DEBUG
#ifdef IPAD_KANKAN
		printf("\pt_notify_only_res_query_shub,task_id=%u,errcode=%d,result=%d,file_size=%llu,gcid_level=%d,bcid_size=%u,block_size=%u,retry_interval=%u,file_suffix=%s,control_flag=%d\n",
		p_task->_task_id,errcode,result,file_size,gcid_level,bcid_size,block_size,retry_interval,file_suffix,control_flag);
#endif
#endif

	if(DATA_DOWNING !=  dm_get_status_code(& (p_p2sp_task->_data_manager)))
	{
		p_p2sp_task->_res_query_shub_state = RES_QUERY_END;
		return SUCCESS;
	}


	sd_memset(_gcid_t,0,CID_SIZE);

	if(p_p2sp_task->_res_query_shub_state != RES_QUERY_REQING)
		return -1;

	if(errcode != SUCCESS)
	{
		p_p2sp_task->_res_query_shub_state = RES_QUERY_FAILED;
		/* Start timer */
		if(p_p2sp_task->_query_shub_timer_id == 0 && errcode != SUCCESS)
		{
			ret_val =  start_timer(pt_handle_query_shub_timeout, NOTICE_INFINITE,REQ_RESOURCE_DEFAULT_TIMEOUT, 0,(void*)p_p2sp_task,&(p_p2sp_task->_query_shub_timer_id));
			if(ret_val!=SUCCESS)
			{
				dt_failure_exit( p_task,ret_val );
				return SUCCESS;
			}
		}		
	}


	p_p2sp_task->_res_query_shub_state =RES_QUERY_SUCCESS;


#ifdef ENABLE_HSC
	pt_start_query_vip_hub(p_task, cid, gcid, file_size, bonus_res_num);
#endif /* ENABLE_HSC */

	if((cm_is_global_need_more_res())&&(cm_is_need_more_peer_res( &(p_task->_connect_manager),INVALID_FILE_INDEX )))
	{
		pt_start_query_phub_tracker_cdn(p_task, cid, gcid, file_size, bonus_res_num);
	}

	
	if(cm_is_global_need_more_res()&&
			cm_is_use_multires( &(p_task->_connect_manager) )
			&& sd_is_cid_valid(gcid)
			&& sd_is_cid_valid(cid)
			 && !p_p2sp_task->_relation_query_finished
			)
	{
		char download_file_suffix[MAX_SUFFIX_LEN + 1] = {0};
		char* pfilesuffix = dm_get_file_suffix(&p_p2sp_task->_data_manager);
		sd_memcpy(download_file_suffix, pfilesuffix, MAX_SUFFIX_LEN);
		char config_suffix_value[MAX_CFG_VALUE_LEN] = {0};
		sd_strncpy(config_suffix_value, ".apk", sd_strlen(".apk"));

		settings_get_str_item("relation_config.support_file_suffix_list", config_suffix_value);
		LOG_DEBUG("pt_notify_only_res_query_shub config_suffix_value:%s, pfilesuffix:%s",
			config_suffix_value, download_file_suffix);

		if(sd_strlen(download_file_suffix) > 0 && sd_stristr(config_suffix_value, download_file_suffix, 0) != NULL)
		{
			dm_get_origin_url(&(p_p2sp_task->_data_manager), &origin_url);
			dm_get_origin_ref_url(&(p_p2sp_task->_data_manager), &ref_origin_url);  

			p_p2sp_task->_res_query_shub_state = RES_QUERY_REQING;
			ret_val  = relation_res_query_shub(user_data, pt_notify_relation_file_query_shub, origin_url, ref_origin_url, (char*)cid, (char*)gcid, file_size, MAX_SERVER*2, ++(p_p2sp_task->_query_relation_shub_times_sn));

			
			if(ret_val != SUCCESS)
			{
				LOG_DEBUG("pt_notify_res_query_shub successful ,  try query relation resource failed...");
				p_p2sp_task->_res_query_shub_state = RES_QUERY_FAILED;
			}			
		}
	

										
	}
	else
	{
		LOG_DEBUG("pt_notify_res_query_shub successful , but need res or other reson, dont query relation resource...p_p2sp_task->_relation_query_finished=%d", p_p2sp_task->_relation_query_finished);
	}	

	cm_create_pipes(&(p_p2sp_task->_task._connect_manager));
	return SUCCESS;	
}

_int32 pt_notify_res_query_shub(void* user_data,_int32 errcode, _u8 result, _u64 file_size, _u8* cid, _u8* gcid, _u32 gcid_level, _u8* bcid, _u32 bcid_size, _u32 block_size,_u32 retry_interval,char * file_suffix,_int32 control_flag)
{
	_int32 bonus_res_num = DEFAULT_BONUS_RES_NUM;
	 char * origin_url = NULL,*ref_origin_url=NULL;
	 char  origin_url_converted[MAX_URL_LEN];
	_u8 _gcid_t[CID_SIZE];
	_int32 ret_val = SUCCESS;
	TASK * p_task = (TASK *)((RES_QUERY_PARA *)user_data)->_task_ptr;
	P2SP_TASK * p_p2sp_task = (P2SP_TASK *)p_task;
	_u64 file_size_from_dm=0;
	
#ifdef EMBED_REPORT
	_u32 cur_time = 0;
	_u32 shub_query_time = 0;
	RES_QUERY_REPORT_PARA *p_report_para = &p_task->_res_query_report_para;
#endif

	sd_assert(user_data!=NULL);
	if(user_data==NULL) return DT_ERR_INVALID_DOWNLOAD_TASK;
	sd_assert(p_p2sp_task!=NULL);
	
	LOG_ERROR("MMMM pt_notify_res_query_shub,task_id=%u,errcode=%d,result=%d,file_size=%llu,gcid_level=%d,bcid_size=%u,block_size=%u,retry_interval=%u,file_suffix=%s,control_flag=%d",
		p_task->_task_id,errcode,result,file_size,gcid_level,bcid_size,block_size,retry_interval,file_suffix,control_flag);
#ifdef _DEBUG
#ifdef IPAD_KANKAN
		printf("\npt_notify_res_query_shub,task_id=%u,errcode=%d,result=%d,file_size=%llu,gcid_level=%d,bcid_size=%u,block_size=%u,retry_interval=%u,file_suffix=%s,control_flag=%d\n",
		p_task->_task_id,errcode,result,file_size,gcid_level,bcid_size,block_size,retry_interval,file_suffix,control_flag);
#endif
#endif

	if(DATA_DOWNING !=  dm_get_status_code(& (p_p2sp_task->_data_manager)))
	{
		p_p2sp_task->_res_query_shub_state = RES_QUERY_END;
		return SUCCESS;
	}


	sd_memset(_gcid_t,0,CID_SIZE);

	if(p_p2sp_task->_res_query_shub_state != RES_QUERY_REQING)
		return -1;
	
	settings_get_int_item("res_query._bonus_res_num",&bonus_res_num);

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
		p_p2sp_task->_res_query_shub_state = RES_QUERY_SUCCESS;

#ifdef EMBED_REPORT
		p_report_para->_hub_s_succ++;

#endif	
		if(result ==SUCCESS)
		{
			p_p2sp_task->_detail_err_code|=EC_QUERY_SHUB_OK;
		}

		BOOL  bcid_valid = ((bcid!=NULL)&&(bcid_size!=0)) ? TRUE : FALSE;
		if((result !=SUCCESS)||(FALSE==sd_is_cid_valid(gcid)) || !bcid_valid || file_size == 0 || !sd_is_cid_valid(cid))
		{			
				/* res_query failed!*/ 
				LOG_ERROR("MMMM Query shub failed!");
				p_p2sp_task->_res_query_shub_state = RES_QUERY_FAILED;
				ret_val = dm_shub_no_result( &(p_p2sp_task->_data_manager));
				if(ret_val!=SUCCESS) goto ErrorHanle;
               		goto ErrorHanle1;
		}
		else
		{
			file_size_from_dm=dm_get_file_size( &(p_p2sp_task->_data_manager));
			if((file_size_from_dm!=0)&&(file_size_from_dm!=file_size))
			{
				LOG_ERROR("MMMM Query shub failed because INVALID file size!");
				p_p2sp_task->_res_query_shub_state = RES_QUERY_FAILED;
				ret_val = dm_shub_no_result( &(p_p2sp_task->_data_manager));
				if(ret_val!=SUCCESS) goto ErrorHanle;
				return SUCCESS;
			}
			
			LOG_ERROR("MMMM Query shub success!");
			p_p2sp_task->_is_shub_ok=TRUE;

			if((file_size!=0)&&(p_p2sp_task->_is_shub_return_file_size!=TRUE))
			{
				p_p2sp_task->_is_shub_return_file_size=TRUE;
				if(p_task->file_size == 0)	
				{
					p_task->file_size = file_size;
				}
				/* Larger than 10MB */
				/*
				if(file_size>MAX_FILE_SIZE_USING_ORIGIN_MODE)
				{
					p_p2sp_task->_res_query_shub_state = RES_QUERY_REQING;
					pt_set_origin_mode(p_task, FALSE );
					p_p2sp_task->_res_query_shub_state = RES_QUERY_SUCCESS;
				}
				*/
			}
			
			p_p2sp_task->_shub_gcid_level = gcid_level;
			p_p2sp_task->_new_gcid_level = gcid_level;
				
			if((file_suffix!=NULL)&&(file_suffix[0]!='\0'))
			{
				ret_val = dm_set_file_suffix( &(p_p2sp_task->_data_manager), file_suffix);
				if(ret_val!=SUCCESS) goto ErrorHanle;
			}

			p_task->_control_flag = control_flag;

			p_p2sp_task->_detail_err_code|=EC_SHUB_HAVE_RECORD;

			if(dm_only_use_origin( &(p_p2sp_task->_data_manager))==FALSE)
			{
				if(p_p2sp_task->_is_cid_ok != TRUE)
				{
					p_p2sp_task->_is_cid_ok = TRUE;
					ret_val = dm_set_cid( &(p_p2sp_task->_data_manager), cid);
					if(ret_val!=SUCCESS) goto ErrorHanle;
				}

				/// shub 返回了文件大小，将该变量设置，避免小的p2sp任务
				if(file_size != 0)
				{
					LOG_ERROR("MMMM Query shub success! _has_bub_info is true...");
					p_p2sp_task->_data_manager._has_bub_info = TRUE;
				}

				/* Get GCID */
				if(p_p2sp_task->_is_gcid_ok == FALSE)	
				{
					p_p2sp_task->_is_gcid_ok = TRUE;
					ret_val = dm_set_gcid( &(p_p2sp_task->_data_manager), gcid);
					if(ret_val!=SUCCESS) goto ErrorHanle;
					//p_p2sp_task->_shub_gcid_level = gcid_level;
					//p_p2sp_task->_new_gcid_level = gcid_level;
				}

				if((bcid!=NULL)&&(bcid_size!=0)&&(dm_bcid_is_valid( &(p_p2sp_task->_data_manager))==FALSE))
				{
					ret_val = dm_set_hub_return_info(&(p_p2sp_task->_data_manager)
                            , (_int32)gcid_level
                            , bcid,bcid_size/CID_SIZE
                            , block_size
                            , file_size);
					if(ret_val!=SUCCESS) goto ErrorHanle;
				}

				p_p2sp_task->_res_query_shub_state = RES_QUERY_REQING;
		 		res_query_shub_by_resinfo_newcmd(&(p_p2sp_task->_res_query_para), pt_notify_only_res_query_shub,cid,file_size ,TRUE,(const char*)gcid, 	TRUE, MAX_SERVER,bonus_res_num, p_p2sp_task->_query_shub_newres_times_sn++,1);
			}
	
		}

			 
	}
	else
	{
#ifdef EMBED_REPORT
		p_report_para->_hub_s_fail++;
#endif	
		p_p2sp_task->_res_query_shub_state = RES_QUERY_FAILED;
	}
ErrorHanle1:
	/* Start timer */
	if(p_p2sp_task->_query_shub_timer_id == 0 && errcode != SUCCESS)
	{
		ret_val =  start_timer(pt_handle_query_shub_timeout, NOTICE_INFINITE,REQ_RESOURCE_DEFAULT_TIMEOUT, 0,(void*)p_p2sp_task,&(p_p2sp_task->_query_shub_timer_id));
		if(ret_val!=SUCCESS) goto ErrorHanle;
	}
	
	return SUCCESS;
	
ErrorHanle:

	dt_failure_exit( p_task,ret_val );
	return SUCCESS;
}

_int32 pt_notify_relation_file_query_shub(void* user_data,_int32 errcode, _u8 result, _u64 file_size, _u8* cid, _u8* gcid, _u32 gcid_level, _u8* bcid, _u32 bcid_size, _u32 block_size,_u32 retry_interval,char * file_suffix,_int32 control_flag)
{
	TASK * p_task = (TASK *)((RES_QUERY_PARA *)user_data)->_task_ptr;
	P2SP_TASK * p_p2sp_task = (P2SP_TASK *)p_task;

	p_p2sp_task->_res_query_shub_state = RES_QUERY_IDLE;

	LOG_DEBUG("pt_notify_relation_file_query_shub errcode=%d", errcode);

	if(errcode == SUCCESS)
	{
		p_p2sp_task->_relation_query_finished = TRUE;
	}
	return 0;
}

_int32 pt_notify_res_query_phub(void* user_data,_int32 errcode, _u8 result, _u32 retry_interval)
{
	_int32 ret_val = SUCCESS;
	P2SP_TASK * p_p2sp_task = (P2SP_TASK *)((RES_QUERY_PARA *)user_data)->_task_ptr;

#ifdef EMBED_REPORT
	_u32 cur_time = 0;
	_u32 phub_query_time = 0;
	RES_QUERY_REPORT_PARA *p_report_para = &p_p2sp_task->_task._res_query_report_para;
#endif

	LOG_DEBUG("pt_notify_res_query_phub");
	sd_assert(p_p2sp_task!=NULL);
	if(p_p2sp_task==NULL) return DT_ERR_INVALID_DOWNLOAD_TASK;
	
	if(DATA_DOWNING !=  dm_get_status_code(& (p_p2sp_task->_data_manager)))
	{
		p_p2sp_task->_res_query_phub_state = RES_QUERY_END;
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
		LOG_ERROR("MMMM Query phub failed!");
		p_p2sp_task->_res_query_phub_state = RES_QUERY_FAILED;
#ifdef EMBED_REPORT
		p_report_para->_hub_p_fail++;
#endif	
	}else
	{
		LOG_ERROR("MMMM Query phub success!");
		p_p2sp_task->_is_phub_ok=TRUE;
		p_p2sp_task->_detail_err_code|=EC_QUERY_PHUB_OK;
		p_p2sp_task->_res_query_phub_state = RES_QUERY_SUCCESS;
		 cm_create_pipes(&(p_p2sp_task->_task._connect_manager));
#ifdef EMBED_REPORT
		p_report_para->_hub_p_succ++;
#endif	
	}

	/* Start timer */
	if(p_p2sp_task->_query_peer_timer_id == 0)
	{
		ret_val= start_timer(pt_handle_query_peer_timeout,NOTICE_INFINITE,REQ_RESOURCE_DEFAULT_TIMEOUT, 0,(void*)p_p2sp_task,&(p_p2sp_task->_query_peer_timer_id));  
		if(ret_val!=SUCCESS) 
		{
			dt_failure_exit( ( TASK*)p_p2sp_task,ret_val );
		}
	}
	return SUCCESS;
}

_int32 pt_notify_res_query_normal_cdn(void* user_data, _int32 errcode, _u8 result, _u32 retry_interval)
{
	_int32 ret_val = SUCCESS;
	P2SP_TASK * p_p2sp_task = (P2SP_TASK *)((RES_QUERY_PARA *)user_data)->_task_ptr;
	_u32 *fail_timeout_times = &(((RES_QUERY_PARA *)user_data)->_task_ptr->_query_normal_cdn_cnt);
#ifdef EMBED_REPORT
	RES_QUERY_REPORT_PARA *p_report_para = &p_p2sp_task->_task._res_query_report_para;
#endif

	LOG_DEBUG("pt_notify_res_query_normal_cdn, user_data:0x%x, errcode:%d, retry_interval:%u, result:%u."
		, user_data, errcode, retry_interval, result);

	sd_assert(p_p2sp_task != NULL);
	if(p_p2sp_task == NULL)
	{
		LOG_ERROR("Invalid task ptr.");
		return DT_ERR_INVALID_DOWNLOAD_TASK;
	}

	if(DATA_DOWNING != dm_get_status_code(&(p_p2sp_task->_data_manager)))
	{
		p_p2sp_task->_res_query_normal_cdn_state = RES_QUERY_END;
		return SUCCESS;
	}

	if((errcode != SUCCESS) || (result != SUCCESS))
	{
		LOG_ERROR("MMMM Query normal cdn manager failed %u.!", *fail_timeout_times);
		p_p2sp_task->_res_query_normal_cdn_state = RES_QUERY_FAILED;

		if (*fail_timeout_times < 3)
		{
			/* Start timer */
			if(p_p2sp_task->_query_normal_cdn_timer_id == 0)
			{
				LOG_DEBUG("start timer on query normal cdn manager.");
				ret_val= start_timer(pt_handle_query_normal_cdn_manager_timeout,
					NOTICE_ONCE, REQ_RESOURCE_DEFAULT_TIMEOUT, 0, (void *)p_p2sp_task, 
					&(p_p2sp_task->_query_normal_cdn_timer_id));  
				if(ret_val != SUCCESS)
				{
					dt_failure_exit(( TASK*)p_p2sp_task, ret_val);
				}
			}
		}

		++(*fail_timeout_times);

#ifdef EMBED_REPORT
		p_report_para->_normal_cdn_fail++;
#endif
	}
	else
	{
		LOG_DEBUG("MMMM Query normal cdn manager success!");
		p_p2sp_task->_detail_err_code |= EC_QUERY_NORMAL_CDN_OK;
		p_p2sp_task->_res_query_normal_cdn_state = RES_QUERY_SUCCESS;
		cm_create_pipes(&(p_p2sp_task->_task._connect_manager));
#ifdef EMBED_REPORT
		/*p_report_para->_hub_p_succ++;*/
#endif
	}

	return SUCCESS;
}

_int32 pt_notify_res_query_tracker(void* user_data,_int32 errcode, _u8 result, _u32 retry_interval,_u32 query_stamp)
{
	_int32 ret_val = SUCCESS;
	P2SP_TASK * p_p2sp_task = (P2SP_TASK *)((RES_QUERY_PARA *)user_data)->_task_ptr;
#ifdef EMBED_REPORT
	_u32 cur_time = 0;
	_u32 tracker_query_time = 0;
	RES_QUERY_REPORT_PARA *p_report_para = &p_p2sp_task->_task._res_query_report_para;
#endif

	LOG_DEBUG("notify_res_query_tracker_success_event");
	sd_assert(p_p2sp_task!=NULL);
	if(p_p2sp_task==NULL) return DT_ERR_INVALID_DOWNLOAD_TASK;
	
	if(DATA_DOWNING !=  dm_get_status_code(& (p_p2sp_task->_data_manager)))
	{
		p_p2sp_task->_res_query_tracker_state = RES_QUERY_END;
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
		LOG_ERROR("MMMM Query tracker failed!");
		p_p2sp_task->_res_query_tracker_state = RES_QUERY_FAILED;
#ifdef EMBED_REPORT
		p_report_para->_hub_t_fail++;
#endif			
	}
	else
	{
		LOG_ERROR("MMMM Query tracker success!");
		p_p2sp_task->_is_tracker_ok=TRUE;
		p_p2sp_task->_detail_err_code|=EC_QUERY_TRACKER_OK;
		p_p2sp_task->_res_query_tracker_state = RES_QUERY_SUCCESS;
		p_p2sp_task->_last_query_stamp = query_stamp;

		 cm_create_pipes(&(p_p2sp_task->_task._connect_manager));
#ifdef EMBED_REPORT
		p_report_para->_hub_t_succ++;
#endif			 
	}

	/* Start timer */
	if(p_p2sp_task->_query_peer_timer_id == 0)
	{
			
		ret_val= start_timer(pt_handle_query_peer_timeout,NOTICE_INFINITE,REQ_RESOURCE_DEFAULT_TIMEOUT, 0,(void*)p_p2sp_task,&(p_p2sp_task->_query_peer_timer_id));  
		if(ret_val!=SUCCESS) 
		{
			dt_failure_exit( ( TASK*)p_p2sp_task,ret_val );
		}
	}
	return SUCCESS;
	
}

#ifdef ENABLE_CDN
_int32 pt_notify_res_query_cdn(_int32 errcode, void* user_data, _u8 result, BOOL is_end, char* host, _u16 port)
{
	_int32 ret_val = SUCCESS;
	_u64 file_size=0;
	_u8 gcid[CID_SIZE];
	_u32 host_ip=0;
	P2SP_TASK * p_p2sp_task = (P2SP_TASK *)((RES_QUERY_PARA *)user_data)->_task_ptr;
	sd_assert(p_p2sp_task!=NULL);
	LOG_DEBUG("pt_notify_res_query_cdn:_task_id=%u,errcode=%d,result=%d,is_end=%d",p_p2sp_task->_task._task_id,errcode,result,is_end);

	if(p_p2sp_task==NULL) return DT_ERR_INVALID_DOWNLOAD_TASK;
	
	if(DATA_DOWNING !=  dm_get_status_code(& (p_p2sp_task->_data_manager)))
	{
		p_p2sp_task->_res_query_cdn_state = RES_QUERY_END;
		return SUCCESS;
	}

	if((errcode !=SUCCESS)||(result!=SUCCESS))
	{
		LOG_ERROR("MMMM Query cdn failed!");
		p_p2sp_task->_res_query_cdn_state = RES_QUERY_FAILED;
	}
	else
	{
		if(is_end==TRUE)
		{
			LOG_ERROR("MMMM Query cdn success!");
			p_p2sp_task->_res_query_cdn_state = RES_QUERY_SUCCESS;

			 cm_create_pipes(&(p_p2sp_task->_task._connect_manager));
		}
		else
		{
			/* Add the resource to connect_manager */
			ret_val = sd_inet_aton(host, &host_ip);
			CHECK_VALUE(ret_val);

			file_size = dm_get_file_size(&(p_p2sp_task->_data_manager));
			if((p_p2sp_task->_is_gcid_ok == TRUE)&&(dm_get_shub_gcid( &(p_p2sp_task->_data_manager),  gcid)==TRUE))
			{
				cm_add_cdn_peer_resource(&(p_p2sp_task->_task._connect_manager),((RES_QUERY_PARA *)user_data)->_file_index, NULL,   gcid,file_size, 0, host_ip,port,0,0,P2P_FROM_CDN);
			}
			return SUCCESS;
		}
	}

	/* Start timer */
	if(p_p2sp_task->_query_peer_timer_id == 0)
	{
			
		ret_val= start_timer(pt_handle_query_peer_timeout,NOTICE_INFINITE,REQ_RESOURCE_DEFAULT_TIMEOUT, 0,(void*)p_p2sp_task,&(p_p2sp_task->_query_peer_timer_id));  
		if(ret_val!=SUCCESS) 
		{
			dt_failure_exit( ( TASK*)p_p2sp_task,ret_val );
		}
	}
	return SUCCESS;

}
#ifdef ENABLE_KANKAN_CDN
_int32 pt_notify_res_query_kankan_cdn(_int32 errcode, void* user_data, _u8 result, BOOL is_end, char* url, _u32 url_len)
{
	_int32 ret_val = SUCCESS;
	_u64 file_size=0;
	_u8 gcid[CID_SIZE];
	_u32 host_ip=0;
	P2SP_TASK * p_p2sp_task = (P2SP_TASK *)((RES_QUERY_PARA *)user_data)->_task_ptr;
	sd_assert(p_p2sp_task!=NULL);
	LOG_DEBUG("pt_notify_res_query_cdn:_task_id=%u,errcode=%d,result=%d,is_end=%d",p_p2sp_task->_task._task_id,errcode,result,is_end);

	if(p_p2sp_task==NULL) return DT_ERR_INVALID_DOWNLOAD_TASK;
	
	if(DATA_DOWNING !=  dm_get_status_code(& (p_p2sp_task->_data_manager)))
	{
		p_p2sp_task->_res_query_kankan_cdn_state = RES_QUERY_END;
		return SUCCESS;
	}

	if((errcode !=SUCCESS)||(result!=SUCCESS))
	{
		LOG_ERROR("MMMM Query cdn failed!");
		p_p2sp_task->_res_query_kankan_cdn_state = RES_QUERY_FAILED;
	}
	else
	{
		if(is_end==TRUE)
		{
			LOG_ERROR("MMMM Query cdn success!");
			p_p2sp_task->_res_query_kankan_cdn_state = RES_QUERY_SUCCESS;

			 cm_create_pipes(&(p_p2sp_task->_task._connect_manager));
		}
		else
		{
			/* Add the resource to connect_manager */
			file_size = dm_get_file_size(&(p_p2sp_task->_data_manager));
			
			cm_add_cdn_server_resource(&(p_p2sp_task->_task._connect_manager), ((RES_QUERY_PARA *)user_data)->_file_index, url, url_len, NULL, 0);
			return SUCCESS;
		}
	}

	/* Start timer */
	if(p_p2sp_task->_query_peer_timer_id == 0)
	{
			
		ret_val= start_timer(pt_handle_query_peer_timeout,NOTICE_INFINITE,REQ_RESOURCE_DEFAULT_TIMEOUT, 0,(void*)p_p2sp_task,&(p_p2sp_task->_query_peer_timer_id));  
		if(ret_val!=SUCCESS) 
		{
			dt_failure_exit( ( TASK*)p_p2sp_task,ret_val );
		}
	}
	return SUCCESS;

}
#endif
#ifdef ENABLE_HSC
_int32 pt_notify_res_query_vip_hub(void* user_data,_int32 errcode, _u8 result, _u32 retry_interval)
{
	_int32 ret_val = SUCCESS;
	P2SP_TASK * p_p2sp_task = (P2SP_TASK *)((RES_QUERY_PARA *)user_data)->_task_ptr;

	LOG_DEBUG("pt_notify_res_query_vip_hub");
	sd_assert(p_p2sp_task!=NULL);
	if(p_p2sp_task==NULL) return DT_ERR_INVALID_DOWNLOAD_TASK;
	

	if(DATA_DOWNING !=  dm_get_status_code(& (p_p2sp_task->_data_manager)))
	{
		p_p2sp_task->_res_query_vip_state = RES_QUERY_END;
		return SUCCESS;
	}
	
	if((errcode !=SUCCESS)||(result!=SUCCESS))
	{
		LOG_ERROR("MMMM Query vip phub failed!");
		p_p2sp_task->_res_query_vip_state = RES_QUERY_FAILED;
	}
	else
	{
		LOG_ERROR("MMMM Query vip hub success!");
		p_p2sp_task->_is_vip_ok=TRUE;
		p_p2sp_task->_detail_err_code|=EC_QUERY_PHUB_OK;
		p_p2sp_task->_res_query_vip_state = RES_QUERY_SUCCESS;
		ret_val = cm_create_pipes(&(p_p2sp_task->_task._connect_manager));
	}

	/* Start timer */
	if(p_p2sp_task->_query_peer_timer_id == 0)
	{
		ret_val= start_timer(pt_handle_query_peer_timeout,NOTICE_INFINITE,REQ_RESOURCE_DEFAULT_TIMEOUT, 0,(void*)p_p2sp_task,&(p_p2sp_task->_query_peer_timer_id));  
		if(ret_val!=SUCCESS) 
		{
			dt_failure_exit( ( TASK*)p_p2sp_task,ret_val );
		}
	}

	return ret_val;
}
#endif /* ENABLE_HSC */
#endif /* ENABLE_CDN */

_int32 pt_notify_res_query_partner_cdn( void* user_data,_int32 errcode, _u8 result, _u32 retry_interval)
{
	_int32 ret_val = SUCCESS;
	P2SP_TASK * p_p2sp_task = (P2SP_TASK *)((RES_QUERY_PARA *)user_data)->_task_ptr;
	
	LOG_DEBUG("pt_notify_res_query_partner_cdn");
	sd_assert(p_p2sp_task!=NULL);
	if(p_p2sp_task==NULL) return DT_ERR_INVALID_DOWNLOAD_TASK;
	
	if(DATA_DOWNING !=  dm_get_status_code(& (p_p2sp_task->_data_manager)))
	{
		p_p2sp_task->_res_query_partner_cdn_state = RES_QUERY_END;
		return SUCCESS;
	}


	if((errcode !=SUCCESS)||(result!=SUCCESS))
	{
		LOG_ERROR("MMMM Query partner cdn failed!");
		p_p2sp_task->_res_query_partner_cdn_state = RES_QUERY_FAILED;
	}else
	{
		LOG_ERROR("MMMM Query partner cdn  success!");
		//p_p2sp_task->_is_phub_ok=TRUE;
		p_p2sp_task->_res_query_partner_cdn_state = RES_QUERY_SUCCESS;

		 cm_create_pipes(&(p_p2sp_task->_task._connect_manager));
	}

	/* Start timer */
	if(p_p2sp_task->_query_peer_timer_id == 0)
	{
			
		ret_val= start_timer(pt_handle_query_peer_timeout,NOTICE_INFINITE,REQ_RESOURCE_DEFAULT_TIMEOUT, 0,(void*)p_p2sp_task,&(p_p2sp_task->_query_peer_timer_id));  
		if(ret_val!=SUCCESS) 
		{
			dt_failure_exit( ( TASK*)p_p2sp_task,ret_val );
		}
	}
	return SUCCESS;

}

_int32 pt_notify_query_dphub_result(void* user_data,_int32 errcode, _u16 result, _u32 retry_interval, BOOL is_continued)
{
    _int32 ret_val = SUCCESS;
    RES_QUERY_PARA *p_para = (RES_QUERY_PARA *)user_data;
    P2SP_TASK *p_p2sp_task = (P2SP_TASK *)p_para->_task_ptr;

    LOG_DEBUG("pt_notify_query_dphub_result, errcode = %d, result = %u, retry_interval = %u.", 
        errcode, (_u32)result, retry_interval);

    if(p_p2sp_task == NULL) return DT_ERR_INVALID_DOWNLOAD_TASK;
    if(DATA_DOWNING !=  dm_get_status_code(&p_p2sp_task->_data_manager))
    {
        p_p2sp_task->_res_query_dphub_state = RES_QUERY_END;
        return SUCCESS;
    }

    if (errcode == SUCCESS)
    {
        if ((is_continued == TRUE) && (p_p2sp_task->_task.task_status == TASK_S_RUNNING))
        {
            ret_val = pt_build_res_query_dphub_content(p_p2sp_task);
            if (ret_val == ERR_RES_QUERY_NO_NEED_QUERY_DPHUB)
            {
                p_p2sp_task->_res_query_dphub_state = RES_QUERY_END;
                return SUCCESS;
            }
        }
        else
        {
            p_p2sp_task->_res_query_dphub_state = RES_QUERY_SUCCESS;
            cm_create_pipes(&(p_p2sp_task->_task._connect_manager));
        }
    }
    else
    {
        p_p2sp_task->_res_query_dphub_state = RES_QUERY_FAILED;
    }
    
    /* Start timer */
    if(p_p2sp_task->_query_peer_timer_id == 0)
    {
        ret_val= start_timer(pt_handle_query_peer_timeout,
            NOTICE_INFINITE,REQ_RESOURCE_DEFAULT_TIMEOUT, 0,(void*)p_p2sp_task,&(p_p2sp_task->_query_peer_timer_id));  
        if(ret_val!=SUCCESS) 
        {
            dt_failure_exit( ( TASK*)p_p2sp_task,ret_val );
        }
    }

    return ret_val;
}

 /*--------------------------------------------------------------------------*/
/*                Callback function for timer				        */
/*--------------------------------------------------------------------------*/
_int32 pt_handle_query_shub_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid)
 {
	_int32 bonus_res_num = DEFAULT_BONUS_RES_NUM,max_query_shub_retry_count=MAX_QUERY_SHUB_RETRY_TIMES,ret_val = SUCCESS;
	TASK * p_task = (TASK *)(msg_info->_user_data);
	 P2SP_TASK * p_p2sp_task = (P2SP_TASK *)(msg_info->_user_data);

	 char * origin_url = NULL,*ref_origin_url=NULL;
	 _u8 cid[CID_SIZE],gcid[CID_SIZE];
	 _u64 file_size=0;
	 
	LOG_DEBUG("dt_handle_query_shub_timeout:errcode=%d,notice_count_left=%u,expired=%u,msgid=%u",errcode,notice_count_left,expired,msgid);


	if(errcode == MSG_TIMEOUT)
	{
		if (p_p2sp_task==NULL) CHECK_VALUE(DT_ERR_INVALID_DOWNLOAD_TASK);

		LOG_DEBUG("dt_handle_query_shub_timeout:_task_id=%u,_query_shub_timer_id=%u,task_status=%d,_res_query_shub_state=%d,need_more_res=%d",p_task->_task_id,p_p2sp_task->_query_shub_timer_id,p_task->task_status,p_p2sp_task->_res_query_shub_state,cm_is_global_need_more_res());
		if(/*msg_info->_device_id*/msgid == p_p2sp_task->_query_shub_timer_id)
		{
			settings_get_int_item("res_query.max_query_shub_retry_count",&max_query_shub_retry_count);
			
			pt_set_origin_mode(p_task, FALSE );
			
			if((p_p2sp_task->_res_query_shub_state != RES_QUERY_REQING)&&(p_p2sp_task->_query_shub_retry_count<max_query_shub_retry_count))
			{
				if((p_task->task_status==TASK_S_RUNNING)&&(cm_is_global_need_more_res())&&
					(cm_is_need_more_server_res( &(p_task->_connect_manager),INVALID_FILE_INDEX )))
				{
					settings_get_int_item("res_query._bonus_res_num",&bonus_res_num);
					if(dm_get_cid( &(p_p2sp_task->_data_manager),  cid)==TRUE)
					{
						
						LOG_ERROR("MMMM Retry res_query_shub_by_cid: _query_shub_retry_count=%d",p_p2sp_task->_query_shub_retry_count);
						file_size = dm_get_file_size( &(p_p2sp_task->_data_manager) );
						if((p_p2sp_task->_is_gcid_ok == TRUE)&&(dm_get_shub_gcid( &(p_p2sp_task->_data_manager),  gcid)==TRUE) && p_p2sp_task->_is_shub_ok)
						{
							ret_val =  res_query_shub_by_resinfo_newcmd(&(p_p2sp_task->_res_query_para), pt_notify_only_res_query_shub ,cid,file_size ,TRUE,(const char*)gcid,	TRUE, MAX_SERVER,bonus_res_num, p_p2sp_task->_query_shub_newres_times_sn++,1);
						}
						else
						{
							ret_val =  res_query_shub_by_cid_newcmd(&(p_p2sp_task->_res_query_para), pt_notify_res_query_shub,cid,file_size ,FALSE,DEFAULT_LOCAL_URL, 	TRUE, MAX_SERVER,bonus_res_num, p_p2sp_task->_query_shub_times_sn++,1);
						}
						if(ret_val!=SUCCESS) goto ErrorHanle;
					}
					else
					{
						
						LOG_ERROR("MMMM Retry res_query_shub_by_url: _query_shub_retry_count=%d",p_p2sp_task->_query_shub_retry_count);

						/* Get origin url and ref url from data manager*/
						 if(FALSE ==  dm_get_origin_url(&(p_p2sp_task->_data_manager), &origin_url))
						 {
							ret_val =DT_ERR_GETTING_ORIGIN_URL;
							goto ErrorHanle;
						 }
						 
						dm_get_origin_ref_url(&(p_p2sp_task->_data_manager), &ref_origin_url);   
						p_p2sp_task->_res_query_shub_step=RQSS_ORIGIN;
						LOG_ERROR("MMMM Retry res_query_shub_by_url:origin_url=%s,ref_origin_url=%s",origin_url,ref_origin_url);
                        p_p2sp_task->_query_shub_retry_count=0;//此处为了让没有查回cid的优先去下cid,无限超时下去
						LOG_ERROR( "MMMM pt_start_query:res_query_shub_by_url real origion url:%s, length:%u", p_p2sp_task->_origion_url, p_p2sp_task->_origion_url_length);
				
						if(p_p2sp_task->_origion_url_length > 0 && sd_strlen(p_p2sp_task->_origion_url) > 0)
						{
							origin_url = p_p2sp_task->_origion_url;
						}
						ret_val =  res_query_shub_by_url_newcmd(&(p_p2sp_task->_res_query_para), pt_notify_res_query_shub,origin_url,ref_origin_url ,TRUE, MAX_SERVER,bonus_res_num, p_p2sp_task->_query_shub_times_sn++,FALSE);
						if(ret_val!=SUCCESS) goto ErrorHanle;
					}
					
#ifdef EMBED_REPORT
					sd_time_ms( &(p_task->_res_query_report_para._shub_query_time) );
#endif	

					p_p2sp_task->_res_query_shub_state = RES_QUERY_REQING;
					p_p2sp_task->_query_shub_retry_count++;
				}
			}
			else
			if(p_p2sp_task->_query_shub_retry_count>=max_query_shub_retry_count)
			{
				LOG_ERROR("MMMM Already retry res_query_shub %d times,do not retry again! END OF RES_QUERY_SHUB!",p_p2sp_task->_query_shub_retry_count);
				if(0 != p_p2sp_task->_query_shub_timer_id)
				{
					cancel_timer(p_p2sp_task->_query_shub_timer_id);
					p_p2sp_task->_query_shub_timer_id = 0;
				}
			}
		}
	}
		
	return SUCCESS;

ErrorHanle:

	dt_failure_exit( p_task,ret_val );
	return SUCCESS;
 }

_int32 pt_handle_query_peer_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid)
{
	_int32 bonus_res_num = DEFAULT_BONUS_RES_NUM;
	TASK * p_task = (TASK *)(msg_info->_user_data);
	 P2SP_TASK * p_p2sp_task = (P2SP_TASK *)(msg_info->_user_data);

	 _u8 cid[CID_SIZE],gcid[CID_SIZE];
	 
	LOG_DEBUG("pt_handle_query_phub_timeout:errcode=%d,notice_count_left=%u,expired=%u,msgid=%u",errcode,notice_count_left,expired,msgid);

	if(errcode == MSG_TIMEOUT)
	{
		if (p_p2sp_task==NULL) CHECK_VALUE(DT_ERR_INVALID_DOWNLOAD_TASK);

		LOG_DEBUG("pt_handle_query_peer_timeout:_task_id=%u,_query_peer_timer_id=%u,task_status=%d,need_more_res=%d",p_task->_task_id,p_p2sp_task->_query_peer_timer_id,p_task->task_status,cm_is_global_need_more_res());

		if(msgid == p_p2sp_task->_query_peer_timer_id)
		{
				if((p_task->task_status==TASK_S_RUNNING)&&(cm_is_global_need_more_res())
					&&(cm_is_need_more_peer_res( &(p_task->_connect_manager),INVALID_FILE_INDEX )))
				{
					if((p_p2sp_task->_is_cid_ok == TRUE)&&(p_p2sp_task->_is_gcid_ok == TRUE)&&
						(dm_get_cid( &(p_p2sp_task->_data_manager),  cid)==TRUE)&&(dm_get_shub_gcid( &(p_p2sp_task->_data_manager),  gcid)==TRUE))
					{
						settings_get_int_item("res_query._bonus_res_num",&bonus_res_num);
#ifdef ENABLE_HSC
						pt_start_query_vip_hub(p_task, cid, gcid, dm_get_file_size( &(p_p2sp_task->_data_manager)), bonus_res_num);
#endif /* ENABLE_HSC */
						pt_start_query_phub_tracker_cdn(p_task, cid, gcid, dm_get_file_size( &(p_p2sp_task->_data_manager)), bonus_res_num);
					}
				}
		}
		
	}
	return SUCCESS;
 }

_int32 pt_handle_query_normal_cdn_manager_timeout(const MSG_INFO *msg_info, 
												  _int32 errcode, 
												  _u32 notice_count_left, 
												  _u32 expired,
												  _u32 msgid)
{
	_int32 bonus_res_num = DEFAULT_BONUS_RES_NUM;
	TASK * p_task = (TASK *)(msg_info->_user_data);
	P2SP_TASK * p_p2sp_task = (P2SP_TASK *)(msg_info->_user_data);
	_u8 cid[CID_SIZE] = {0};
	_u8 gcid[CID_SIZE] = {0};

	LOG_DEBUG("pt_handle_query_normal_cdn_manager_timeout:errcode=%d, notice_count_left=%u, expired=%u, msgid=%u",
		errcode,notice_count_left,expired,msgid);

	if(errcode == MSG_TIMEOUT)
	{
		if (p_p2sp_task==NULL) CHECK_VALUE(DT_ERR_INVALID_DOWNLOAD_TASK);

		LOG_DEBUG("pt_handle_query_normal_cdn_manager_timeout:_task_id=%u, _query_peer_timer_id=%u, task_status=%d, need_more_res=%d",
			p_task->_task_id, p_p2sp_task->_query_normal_cdn_timer_id, p_task->task_status, cm_is_global_need_more_res());

		if(msgid == p_p2sp_task->_query_normal_cdn_timer_id)
		{
			if(p_task->task_status == TASK_S_RUNNING)
			{
				if((p_p2sp_task->_is_cid_ok == TRUE) && 
				   (p_p2sp_task->_is_gcid_ok == TRUE) &&
				   (dm_get_cid(&(p_p2sp_task->_data_manager), cid) == TRUE) &&
				   (dm_get_shub_gcid( &(p_p2sp_task->_data_manager), gcid) == TRUE))
				{
					settings_get_int_item("res_query._bonus_res_num", &bonus_res_num);
					pt_start_query_normal_cdn(p_task, cid, gcid, 
						dm_get_file_size(&(p_p2sp_task->_data_manager)), bonus_res_num);
				}
			}
		}
	}
	return SUCCESS;
}

_int32 pt_handle_wait_dphub_root_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid)
{
    TASK * p_task = (TASK *)(msg_info->_user_data);
    P2SP_TASK * p_p2sp_task = (P2SP_TASK *)(msg_info->_user_data);
    _u8 cid[CID_SIZE] = {0};
    _u8 gcid[CID_SIZE] = {0};
    
    if(errcode == MSG_TIMEOUT)
    {
        if (p_p2sp_task==NULL) CHECK_VALUE(DT_ERR_INVALID_DOWNLOAD_TASK);

        LOG_DEBUG("pt_handle_requery_dphub_timeout:_task_id=%u,_wait_dphub_root_timer_id=%u,task_status=%d,need_more_res=%d",
            p_task->_task_id, p_p2sp_task->_dpub_query_context._wait_dphub_root_timer_id, p_task->task_status, cm_is_global_need_more_res());

        if(msgid == p_p2sp_task->_dpub_query_context._wait_dphub_root_timer_id)
        {
            if((p_task->task_status==TASK_S_RUNNING)&&(cm_is_global_need_more_res())
                &&(cm_is_need_more_peer_res( &(p_task->_connect_manager),INVALID_FILE_INDEX )))
            {
                if((p_p2sp_task->_is_cid_ok == TRUE)&&(p_p2sp_task->_is_gcid_ok == TRUE)&&
                    (dm_get_cid( &(p_p2sp_task->_data_manager), cid)==TRUE)&&(dm_get_shub_gcid( &(p_p2sp_task->_data_manager),  gcid)==TRUE))
                {
                    pt_build_res_query_dphub_content(p_p2sp_task);
                }
            }
        }

    }

    return SUCCESS;
}



/*--------------------------------------------------------------------------*/
/*                Internal functions				        */
/*--------------------------------------------------------------------------*/


_int32 pt_convert_url_format(enum TASK_RES_QUERY_SHUB_STEP * rqss ,const char* origin_url, char* str_buffer ,_int32 buffer_len )
{
	char _Tbuffer[MAX_URL_LEN];
	enum CODE_PAGE code_page;
	_u32 output_len = MAX_URL_LEN;
	_int32 ret_val = 0;

	LOG_DEBUG("pt_convert_url_format(_res_query_shub_step=%d)",*rqss);

	//sd_memset(_Tbuffer,0,MAX_URL_LEN);
	
		switch(*rqss)
		{
			case RQSS_ORIGIN:
				ret_val = url_object_to_noauth_string(origin_url,  str_buffer ,buffer_len);
				if(ret_val ==-1)
				{
					return DT_ERR_URL_NOAUTH_DECODE;
				}
				else
				if(ret_val >0)
				{
					*rqss = RQSS_NO_AUTH_DECODE;
					break;
				}
				
				LOG_DEBUG("noauth_decode_url = origin_url,so go to next step!");
				
			case RQSS_NO_AUTH_DECODE:
				url_object_to_noauth_string(origin_url,  _Tbuffer ,MAX_URL_LEN);
				code_page = sd_conjecture_code_page(_Tbuffer );	
				if(code_page==_CP_GBK)
				{
					LOG_DEBUG("  The url is in GBK format ");
					sd_gbk_2_utf8(_Tbuffer, sd_strlen(_Tbuffer),str_buffer, &output_len);
					if(output_len!=0)
					{
					       *rqss = RQSS_GBK2UTF8;
					}
					else
					{
						LOG_DEBUG(" Maybe the url is in BIG5 format ");
						output_len =MAX_URL_LEN;
						sd_big5_2_utf8(_Tbuffer, sd_strlen(_Tbuffer),str_buffer, &output_len);
						if(output_len!=0)
						{
						       *rqss  = RQSS_BIG52UTF8;
						}
						else
						{
							LOG_DEBUG(" Maybe the url is in UTF8 format ");
							output_len =MAX_URL_LEN;
							sd_utf8_2_gbk(_Tbuffer, sd_strlen(_Tbuffer),str_buffer, &output_len);
							if(output_len!=0)
							{
								*rqss  = RQSS_UTF82GBK ;
							}
							else
							{
								LOG_DEBUG(" sd_utf8_2_gbk Failed!do not retry any more!RQSS_END ");
								*rqss  = RQSS_END ;
								return -1; //HTTP_ERR_FULL_PATH_BIG5_2_UTF8;
							}
						}
					}
				}
				else
				if(code_page==_CP_BIG5)
				{
					LOG_DEBUG(" The url is in BIG5 format ");
					sd_big5_2_utf8(_Tbuffer, sd_strlen(_Tbuffer),str_buffer, &output_len);
					if(output_len!=0)
					{
						*rqss  = RQSS_BIG52UTF8;
					}
					else
					{
							LOG_DEBUG(" Maybe the url is in UTF8 format ");
							output_len =MAX_URL_LEN;
							sd_utf8_2_gbk(_Tbuffer, sd_strlen(_Tbuffer),str_buffer, &output_len);
							if(output_len!=0)
							{
								*rqss  = RQSS_UTF82GBK;
							}
							else
							{
								LOG_DEBUG(" sd_utf8_2_gbk Failed!do not retry any more!RQSS_END ");
								*rqss  = RQSS_END ;
								return -1;//HTTP_ERR_FULL_PATH_BIG5_2_UTF8;
							}
						
					}
				}
				else
				if(code_page==_CP_UTF8)
				{
					LOG_DEBUG(" The url is in UTF-8 format ");
					sd_utf8_2_gbk(_Tbuffer, sd_strlen(_Tbuffer),str_buffer, &output_len);
					if(output_len!=0)
					{
						*rqss  = RQSS_UTF82GBK;
					}
					else
					{
						LOG_DEBUG(" sd_utf8_2_gbk Failed!do not retry any more!RQSS_END ");
						*rqss  = RQSS_END ;
						return -1;//HTTP_ERR_FULL_PATH_BIG5_2_UTF8;
					}
				}
				else
				{
					LOG_DEBUG(" The url is in ASCII format ,do not retry any more!RQSS_END ");
					*rqss  = RQSS_END;
					return -1;//HTTP_ERR_FULL_PATH_UNKNOWN_CODE_PAGE;
				}

				break;
				
			case RQSS_GBK2UTF8:
				url_object_to_noauth_string(origin_url,  _Tbuffer ,MAX_URL_LEN);
				LOG_DEBUG(" Maybe the url is in BIG5 format ");
				sd_big5_2_utf8(_Tbuffer, sd_strlen(_Tbuffer),str_buffer, &output_len);
				if(output_len!=0)
				{
					*rqss  = RQSS_BIG52UTF8 ;
				}
				else
				{
					LOG_DEBUG(" Maybe the url is in UTF-8 format ");
					output_len =MAX_URL_LEN;
					sd_utf8_2_gbk(_Tbuffer, sd_strlen(_Tbuffer),str_buffer, &output_len);
					if(output_len!=0)
					{
						*rqss  = RQSS_UTF82GBK;
					}
					else
					{
						LOG_DEBUG(" sd_utf8_2_gbk Failed!do not retry any more!RQSS_END ");
						*rqss  = RQSS_END;
						return -1;//HTTP_ERR_FULL_PATH_BIG5_2_UTF8;
					}
						
				}

				break;
				
			case RQSS_BIG52UTF8:
				url_object_to_noauth_string(origin_url,  _Tbuffer ,MAX_URL_LEN);
				LOG_DEBUG(" Maybe the url is in UTF-8 format ");
				sd_utf8_2_gbk(_Tbuffer, sd_strlen(_Tbuffer),str_buffer, &output_len);
				if(output_len!=0)
				{
					*rqss  = RQSS_UTF82GBK;
				}
				else
				{
					LOG_DEBUG(" sd_utf8_2_gbk Failed!do not retry any more!RQSS_END ");
					*rqss  = RQSS_END;
					return -1;//HTTP_ERR_FULL_PATH_BIG5_2_UTF8;
				}

				break;
			
			case RQSS_UTF82GBK:
			default:
					LOG_DEBUG(" *rqss=%d,Do not retry any more!RQSS_END ",*rqss);
					*rqss  = RQSS_END;
					return -1;
				
				
		}

		return SUCCESS;
}

#endif /* __P2SP_QUERY_C_20090319 */
