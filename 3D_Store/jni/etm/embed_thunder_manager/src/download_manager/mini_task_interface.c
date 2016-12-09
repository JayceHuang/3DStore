/*--------------------------------------------------------------------------*/
/*                               IDENTIFICATION                             */
/*--------------------------------------------------------------------------*/
/*     Filename  : mini_task_interface.c                                         */
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
/* 2010.04.20 | ZengYuqing  | Creation                                      */
/*--------------------------------------------------------------------------*/


#include "download_manager/mini_task_interface.h"
#include "download_manager/download_task_interface.h"

#include "platform/sd_network.h"

#include "scheduler/scheduler.h"
#include "vod_task/vod_task.h"
#include "et_interface/et_interface.h"
#include "settings/em_settings.h"
#include "em_asyn_frame/em_msg.h"

#include "em_asyn_frame/em_msg.h"
#include "em_asyn_frame/em_notice.h"
#include "em_common/em_utility.h"
#include "em_common/em_mempool.h"
#include "em_interface/em_fs.h"
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

static MAP g_mini_task_map;
static SLAB *gp_mini_task_slab = NULL;		//MINI_TASK
static BOOL g_speed_limited = FALSE;
static _u32	g_timeout_id = INVALID_MSG_ID;

#define	MAX_POST_BUFFER_LEN		(16*1024)	// 16KB
/*--------------------------------------------------------------------------*/
/*                Interfaces provid for external program				        */
/*--------------------------------------------------------------------------*/

_int32 init_mini_task_module(void)
{
	_int32 ret_val = SUCCESS;
	EM_LOG_DEBUG( "init_mini_task_module" );

	em_map_init(&g_mini_task_map, mini_id_comp);
	
	sd_assert(gp_mini_task_slab==NULL);
	if(gp_mini_task_slab==NULL)
	{
		ret_val = em_mpool_create_slab(sizeof(MINI_TASK), MIN_MINI_TASK_MEMORY, 0, &gp_mini_task_slab);
		CHECK_VALUE(ret_val);
	}
	g_speed_limited = FALSE;
	g_timeout_id = INVALID_MSG_ID;
	return SUCCESS;
	
}
_int32 uninit_mini_task_module(void)
{
	EM_LOG_DEBUG( "uninit_mini_task_module" );

	if(g_timeout_id != INVALID_MSG_ID)
	{
		em_cancel_timer(g_timeout_id);
		g_timeout_id = INVALID_MSG_ID;
	}
	
	mini_clear();
	
	sd_assert(em_map_size(&g_mini_task_map)==0);
	
	if(gp_mini_task_slab!=NULL)
	{
		em_mpool_destory_slab(gp_mini_task_slab);
		gp_mini_task_slab=NULL;
	}

	
	return SUCCESS;
}
_int32 mini_id_comp(void *E1, void *E2)
{
	return ((_int32)E1) -((_int32)E2);
}

_int32 mini_clear(void)
{
      MAP_ITERATOR  cur_node = NULL; 
	 MINI_TASK * p_task = NULL;

      EM_LOG_ERROR("mini_clear:%u",em_map_size(&g_mini_task_map)); 
	  
      cur_node = EM_MAP_BEGIN(g_mini_task_map);
      while(cur_node != EM_MAP_END(g_mini_task_map))
      {
             p_task = (MINI_TASK *)EM_MAP_VALUE(cur_node);
	      mini_delete_task(p_task);
		//em_map_erase_iterator(&g_mini_task_map, cur_node);
	      cur_node = EM_MAP_BEGIN(g_mini_task_map);
      }

	return SUCCESS;
}

_int32 mini_handle_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid)
{
	if(errcode == MSG_CANCELLED)
		return SUCCESS;
	
	mini_scheduler();

	if(em_map_size(&g_mini_task_map)==0)
	{
		if(g_timeout_id != INVALID_MSG_ID)
		{
			em_cancel_timer(g_timeout_id);
			g_timeout_id = INVALID_MSG_ID;
		}
	}
	return SUCCESS;
}

void mini_scheduler(void)
{
      MAP_ITERATOR  cur_node = NULL,nxt_node = NULL; 
	 MINI_TASK * p_task = NULL;
	 
      EM_LOG_DEBUG("mini_scheduler:%u",em_map_size(&g_mini_task_map)); 
	  
      cur_node = EM_MAP_BEGIN(g_mini_task_map);
      while(cur_node != EM_MAP_END(g_mini_task_map))
      {
        	p_task = (MINI_TASK *) EM_MAP_VALUE(cur_node);
		if(p_task->_state == MTS_FINISHED)
		{
			nxt_node = EM_MAP_NEXT(g_mini_task_map, cur_node);
			mini_delete_task(p_task);
			cur_node = nxt_node;
		}
		else
		{
			cur_node = EM_MAP_NEXT(g_mini_task_map, cur_node);
		}
      }
	return ;
}
MINI_TASK * mini_get_task_from_map(_u32 task_id)
{
	_int32 ret_val = SUCCESS;
	MINI_TASK * p_task=NULL;

      EM_LOG_DEBUG("mini_get_task_from_map:task_id=%u",task_id); 
	ret_val=em_map_find_node(&g_mini_task_map, (void *)task_id, (void **)&p_task);
	sd_assert(ret_val==SUCCESS);
	//sd_assert(p_task!=NULL);
	return p_task;
}
_int32 mini_add_task_to_map(MINI_TASK * p_task)
{
	_int32 ret_val = SUCCESS;
	PAIR info_map_node;
      EM_LOG_DEBUG("mini_add_task_to_map:task_id=%u",p_task->_task_id); 
	info_map_node._key =(void*) p_task->_task_id;
	info_map_node._value = (void*)p_task;
	ret_val = em_map_insert_node( &g_mini_task_map, &info_map_node );
	CHECK_VALUE(ret_val);
	if(g_timeout_id == INVALID_MSG_ID)
	{
		em_start_timer(mini_handle_timeout, NOTICE_INFINITE, MINI_TIMEOUT_INSTANCE, 0, NULL, &g_timeout_id);
	}
	return SUCCESS;
}

_int32 mini_remove_task_from_map(MINI_TASK * p_task)
{
      EM_LOG_DEBUG("mini_remove_task_from_map:task_id=%u",p_task->_task_id); 
	return em_map_erase_node(&g_mini_task_map, (void*)p_task->_task_id);
}

void mini_map_test(void)
{
      MAP_ITERATOR  cur_node = NULL; 
	 MINI_TASK * p_task = NULL;
	 _u32 task_id = 0;
	 
      EM_LOG_ERROR("mini_map_test:%u",em_map_size(&g_mini_task_map)); 
	  
      cur_node = EM_MAP_BEGIN(g_mini_task_map);
      while(cur_node != EM_MAP_END(g_mini_task_map))
      {
        	p_task = (MINI_TASK *) EM_MAP_VALUE(cur_node);
		task_id = (_u32)EM_MAP_KEY(cur_node);
      		EM_LOG_ERROR("mini_map_test:task_id=%u == %u",task_id,p_task->_task_id); 
		p_task = NULL;
		task_id = 0;
		cur_node = EM_MAP_NEXT(g_mini_task_map, cur_node);
      }
	return ;
}

_int32 mini_http_resp_callback(ET_HTTP_CALL_BACK * p_http_call_back_param)
//_int32  mini_http_resp_callback(_u32 http_id,ET_HTTP_RESP_TYPE type, void * resp)
{
	_int32 ret_val = SUCCESS,get_count = 0;
	MINI_TASK *p_task=NULL;

      EM_LOG_ERROR("mini_http_resp_callback:task_id=%u,type=%d",p_http_call_back_param->_http_id,p_http_call_back_param->_type); 

	/* 发现某种情况下在该任务被加入MAP之前就已经回调了,因此在这里做一个循环延时一下.zyq @20110506 */
	/*  这里有隐患,不同线程操作同一个MAP会有竞争,要加锁解决 */
	do
	{
		p_task = mini_get_task_from_map(p_http_call_back_param->_http_id);
		if(p_task!=NULL) 
		{
			break;
		}
		mini_map_test();
	}while(get_count++<5);
	
	if(p_task==NULL) 
	{
		mini_map_test();
		//sd_assert(FALSE);
		return -1;
	}
	
	//sd_assert(p_task->_state==MTS_RUNNING);
	if(p_task->_state!=MTS_RUNNING)
	{
		/* 该mini http 任务已经被关闭 */
		if(p_http_call_back_param->_type == EHT_PUT_RECVED_DATA)
		{
			if(p_task->_task_info._is_file)
			{
				/* 释放内存 */
				em_free(p_http_call_back_param->_recved_data);
			}
		}
		return -1;
	}

	switch(p_http_call_back_param->_type)
	{
		case 	EHT_NOTIFY_RESPN:
		{
			em_memcpy(&p_task->_header,p_http_call_back_param->_header,sizeof(MINI_HTTP_HEADER));
      			EM_LOG_ERROR("EHT_NOTIFY_RESPN,status_code=%d",p_task->_header._status_code); 
		}
		break;
		case EHT_GET_RECV_BUFFER:
		{
			sd_assert(p_task->_task_info._is_file==TRUE);
			//*p_http_call_back_param->_recv_buffer_len = 16384;  //16k
			ret_val = em_malloc(*p_http_call_back_param->_recv_buffer_len,p_http_call_back_param->_recv_buffer);
      		EM_LOG_ERROR("EHT_GET_RECV_BUFFER,ret_val=%d,buffer_len=%u",ret_val,*p_http_call_back_param->_recv_buffer_len ); 
		}
		break;
		case 	EHT_PUT_RECVED_DATA:
		{
			//printf("EHT_PUT_RECVED_DATA\n");
      			EM_LOG_ERROR("EHT_PUT_RECVED_DATA,is_file=%d,recved_data_len=%u,file_id=%u,data_recved=%llu",p_task->_task_info._is_file,p_http_call_back_param->_recved_data_len ,p_task->_file_id,p_task->_data_recved); 
			if(p_task->_task_info._is_file)
			{
				_u64 filepos = p_task->_data_recved;
				_u32 writesize = 0;
				if(p_http_call_back_param->_recved_data_len!=0)
				{
					if(p_task->_file_id==0)
					{
						char full_path[MAX_FULL_PATH_BUFFER_LEN];
						em_memset(full_path,0x00,MAX_FULL_PATH_BUFFER_LEN);
						em_strncpy(full_path,p_task->_task_info._file_path,p_task->_task_info._file_path_len);
						if(full_path[p_task->_task_info._file_path_len-1]!='/')
							full_path[p_task->_task_info._file_path_len]='/';
						em_strcat(full_path,p_task->_task_info._file_name,p_task->_task_info._file_name_len);
						if(em_file_exist(full_path))
						{
							/* 把旧文件删除掉 */
							ret_val = em_delete_file(full_path);
							sd_assert(ret_val ==SUCCESS );
						}
						ret_val = em_open_ex(full_path, O_FS_CREATE, &p_task->_file_id);
						CHECK_VALUE(ret_val);
					}

					ret_val = em_pwrite(p_task->_file_id, (char *)p_http_call_back_param->_recved_data,(_int32) p_http_call_back_param->_recved_data_len, filepos, &writesize);
					em_free(p_http_call_back_param->_recved_data);
					CHECK_VALUE(ret_val);
      					EM_LOG_ERROR("EHT_PUT_RECVED_DATA,em_pwrite=%d,writesize=%u",ret_val,writesize); 
					sd_assert(writesize==p_http_call_back_param->_recved_data_len);

					p_task->_data_recved+=writesize;
				}
				else
				{
      					EM_LOG_ERROR("EHT_PUT_RECVED_DATA,em_free"); 
					em_free(p_http_call_back_param->_recved_data);
				}
			}
			else
			{
      				EM_LOG_ERROR("EHT_PUT_RECVED_DATA:recv_buffer=0x%X,recved_data=0x%X",p_task->_task_info._recv_buffer,p_http_call_back_param->_recved_data); 
				sd_assert(p_task->_task_info._recv_buffer==(void*)p_http_call_back_param->_recved_data);
				p_task->_data_recved+=p_http_call_back_param->_recved_data_len;
			}
		}
		break;
		case 	EHT_NOTIFY_FINISHED:
		{
      			EM_LOG_ERROR("EHT_NOTIFY_FINISHED,result=%d",p_http_call_back_param->_result); 
			p_task->_failed_code = p_http_call_back_param->_result;
			p_task->_state = MTS_FINISHED;
			if(p_task->_file_id!=0)
			{
				em_close_ex(p_task->_file_id);
				p_task->_file_id = 0;
			}
		}
		break;
		case EHT_NOTIFY_SENT_DATA:
      		EM_LOG_ERROR("EHT_NOTIFY_SENT_DATA:send_data=0x%X,sent_data2=0x%X",p_task->_task_info._send_data,p_http_call_back_param->_sent_data); 
			//sd_assert(p_task->_task_info._send_data==p_http_call_back_param->_sent_data);
			if(p_task->_task_info._send_data_len == p_task->_data_recved)
			{	
				//eti_http_close(p_task->_task_id);
				EM_LOG_ERROR("send end"); 
			}
			SAFE_DELETE(p_http_call_back_param->_send_data);
		break;
		case EHT_GET_SEND_DATA:
		{ 	
			if(p_task->_task_info._is_file)
			{
				_u64 filepos = p_task->_data_recved;
				_u32 readsize = 0;
				char data_buf[MAX_POST_BUFFER_LEN] = {0};
				if( p_task->_task_info._send_data_len != filepos)
				{
					if(p_task->_file_id == 0)
					{
						char full_path[MAX_FULL_PATH_BUFFER_LEN];
						em_memset(full_path,0x00,MAX_FULL_PATH_BUFFER_LEN);
						em_strncpy(full_path,p_task->_task_info._file_path,p_task->_task_info._file_path_len);
						if(full_path[p_task->_task_info._file_path_len-1]!='/')
							full_path[p_task->_task_info._file_path_len]='/';
						em_strcat(full_path,p_task->_task_info._file_name,p_task->_task_info._file_name_len);
						if(em_file_exist(full_path))
						{
							ret_val = em_open_ex(full_path, O_FS_RDONLY, &p_task->_file_id);
							CHECK_VALUE(ret_val);
						}
						else
							return FILE_NOT_EXIST;
					}
					
					ret_val = em_pread(p_task->_file_id, (char *)data_buf,MAX_POST_BUFFER_LEN, filepos, &readsize);
					CHECK_VALUE(ret_val);
					*p_http_call_back_param->_send_data_len = readsize;
					ret_val = em_malloc(*p_http_call_back_param->_send_data_len, (void**)p_http_call_back_param->_send_data);
					CHECK_VALUE(ret_val);
					sd_memcpy(*p_http_call_back_param->_send_data, data_buf, readsize);

					EM_LOG_ERROR("EHT_GET_SEND_DATA,em_pread=%d,writesize=%u",ret_val,readsize); 
					//sd_assert(readsize == p_task->_task_info._send_data_len);
					p_task->_data_recved += readsize;
					
				}
				else
				{
      				EM_LOG_ERROR("EHT_GET_SEND_DATA"); 
				}
			}
			
		}
		break;
		default:
			EM_LOG_ERROR("mini_http_resp_callback:Error!"); 
			sd_assert(FALSE);
		break;
	}
	return SUCCESS;
}

_int32 mini_delete_task(MINI_TASK * p_task )
{
	EM_NOTIFY_MINI_TASK_FINISHED callback_fun=(EM_NOTIFY_MINI_TASK_FINISHED)p_task->_task_info._callback_fun;
	void * user_data =p_task->_task_info._user_data;
	void * recv_buffer=p_task->_task_info._recv_buffer;
	_u8 * send_data=p_task->_task_info._send_data;
	_u64 data_recved=p_task->_data_recved;
	BOOL is_file=p_task->_is_file;
	_u32  task_id =  p_task->_task_id;
	_int32 result =-1;
	static MINI_HTTP_HEADER mini_http_header;

		if(MTS_FINISHED==p_task->_state)
		{
			result = p_task->_failed_code;
		}

	EM_LOG_ERROR("mini_delete_task:%d,result=%d,data_recved=%llu",p_task->_task_id,result,data_recved);
	
		if(p_task->_file_id!=0)
		{
			em_close_ex(p_task->_file_id);
			p_task->_file_id = 0;
		}

		em_memcpy(&mini_http_header,&p_task->_header,sizeof(MINI_HTTP_HEADER));
		
		eti_http_close(p_task->_task_id);
		 mini_remove_task_from_map(p_task);
		mini_task_free(p_task);
		if(is_file && callback_fun!=NULL)
		{
			ET_HTTP_CALL_BACK cb_param;
			em_memset(&cb_param,0x00,sizeof(ET_HTTP_CALL_BACK));
			cb_param._http_id = task_id;
			cb_param._user_data = user_data;

			if(mini_http_header._status_code != 0)
			{
				cb_param._type = EHT_NOTIFY_RESPN;
				if(result==SUCCESS && mini_http_header._status_code==200 && mini_http_header._content_length == 0)
				{
					mini_http_header._content_length = data_recved;
				}
				cb_param._header= &mini_http_header;
				EM_LOG_ERROR("ET_HTTP_CALL_BACK_FUNCTION:EHT_NOTIFY_RESPN");
				((ET_HTTP_CALL_BACK_FUNCTION)callback_fun)(&cb_param);
			}
			
			cb_param._type = EHT_NOTIFY_FINISHED;
			cb_param._header= NULL;
			cb_param._result = result;
			EM_LOG_ERROR("ET_HTTP_CALL_BACK_FUNCTION:EHT_NOTIFY_FINISHED:%d",result);
			((ET_HTTP_CALL_BACK_FUNCTION)callback_fun)(&cb_param);
		}
		else
		if(callback_fun)
		{
			EM_LOG_ERROR("ETM_NOTIFY_MINI_TASK_FINISHED:result=%d,data_recved=%llu",result,data_recved);
			callback_fun(user_data,result,recv_buffer,(_u32)data_recved,&mini_http_header,send_data);
			EM_LOG_ERROR("ETM_NOTIFY_MINI_TASK_FINISHED end!");
		}
	
	return SUCCESS;
}


_int32 em_get_mini_file_from_url_impl( EM_MINI_TASK * p_task_param )
{
	_int32 ret_val = SUCCESS;
	ET_HTTP_PARAM et_http_param;
	MINI_TASK *p_task=NULL;
	_u32 http_id = 0;

	EM_LOG_ERROR("em_get_mini_file_from_url_impl:url[%u]=%s,file[%u+%u]=%s+%s",
		p_task_param->_url_len,p_task_param->_url,p_task_param->_file_path_len,p_task_param->_file_name_len,p_task_param->_file_path,p_task_param->_file_name);


	em_memset(&et_http_param,0x00,sizeof(ET_HTTP_PARAM));

	et_http_param._url = p_task_param->_url;
	et_http_param._url_len= p_task_param->_url_len;
	et_http_param._content_len= p_task_param->_send_data_len;
	et_http_param._send_gzip= p_task_param->_gzip;
	et_http_param._accept_gzip= p_task_param->_gzip;
	et_http_param._send_data= p_task_param->_send_data;
	et_http_param._send_data_len= p_task_param->_send_data_len;
	et_http_param._recv_buffer= p_task_param->_recv_buffer;
	et_http_param._recv_buffer_size= p_task_param->_recv_buffer_size;
	et_http_param._callback_fun=(void*) mini_http_resp_callback;
	et_http_param._user_data= p_task_param->_user_data;
	et_http_param._timeout= p_task_param->_timeout;
	et_http_param._priority = 0;
	
	if(FALSE==em_is_et_running())
	{
		ret_val = em_start_et(  );
		CHECK_VALUE(ret_val);
	}

	ret_val = eti_http_get(&et_http_param,&p_task_param->_mini_id);
	CHECK_VALUE(ret_val);
		 
	http_id = p_task_param->_mini_id;
	
	ret_val = mini_task_malloc(&p_task);
	if(ret_val!=SUCCESS)
	{
		eti_http_close(http_id);
		CHECK_VALUE(ret_val);
	}
	
	p_task->_task_id = http_id;
	em_memcpy(&p_task->_task_info,p_task_param,sizeof(EM_MINI_TASK));

	ret_val = mini_add_task_to_map(p_task);
	if(ret_val!=SUCCESS)
	{
		eti_http_close(http_id);
		mini_task_free(p_task);
		CHECK_VALUE(ret_val);
	}
	
	EM_LOG_ERROR("em_get_mini_file_from_url_impl end:url[%u]=%s,ret_val=%d,task_id=%u",p_task_param->_url_len,p_task_param->_url,ret_val,p_task->_task_id);

	return ret_val;
}


_int32 em_get_mini_file_from_url(void* p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	EM_MINI_TASK * p_mini_param= (EM_MINI_TASK* )_p_param->_para1;

	EM_LOG_DEBUG("em_get_mini_file_from_url");

	if(em_is_net_ok(TRUE)==TRUE)
	{
		_p_param->_result = em_get_mini_file_from_url_impl( p_mini_param);
	}
	else
	{
		_p_param->_result = -1;
	}
	
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 em_post_mini_file_to_url_impl( EM_MINI_TASK * p_task_param )
{
	_int32 ret_val = SUCCESS;
	ET_HTTP_PARAM et_http_param;
	MINI_TASK *p_task=NULL;
	_u32 http_id = 0;

	EM_LOG_ERROR("em_post_mini_file_to_url_impl:url[%u]=%s,send_data_len=%u",
		p_task_param->_url_len,p_task_param->_url,p_task_param->_send_data_len);


	em_memset(&et_http_param,0x00,sizeof(ET_HTTP_PARAM));

	et_http_param._url = p_task_param->_url;
	et_http_param._url_len= p_task_param->_url_len;
	et_http_param._content_len= p_task_param->_send_data_len;
	et_http_param._send_gzip= p_task_param->_gzip;
	et_http_param._accept_gzip= p_task_param->_gzip;
	et_http_param._send_data= p_task_param->_send_data;
	et_http_param._send_data_len= p_task_param->_send_data_len;
	et_http_param._recv_buffer= p_task_param->_recv_buffer;
	et_http_param._recv_buffer_size= p_task_param->_recv_buffer_size;
	et_http_param._callback_fun=(void*) mini_http_resp_callback;//p_task_param->_callback_fun;
	et_http_param._user_data= p_task_param->_user_data;
	et_http_param._timeout= p_task_param->_timeout;
	et_http_param._priority = 0;
	
	if(FALSE==em_is_et_running())
	{
		ret_val = em_start_et(  );
		CHECK_VALUE(ret_val);
	}

	ret_val = eti_http_post(&et_http_param,&p_task_param->_mini_id);
	CHECK_VALUE(ret_val);
		 
	http_id = p_task_param->_mini_id;
	
	ret_val = mini_task_malloc(&p_task);
	if(ret_val!=SUCCESS)
	{
		eti_http_close(http_id);
		CHECK_VALUE(ret_val);
	}
	
	p_task->_task_id = http_id;
	em_memcpy(&p_task->_task_info,p_task_param,sizeof(EM_MINI_TASK));

	ret_val = mini_add_task_to_map(p_task);
	if(ret_val!=SUCCESS)
	{
		eti_http_close(http_id);
		mini_task_free(p_task);
		CHECK_VALUE(ret_val);
	}

	EM_LOG_ERROR("em_post_mini_file_to_url_impl end:url[%u]=%s,ret_val=%d,task_id=%u",p_task_param->_url_len,p_task_param->_url,ret_val,p_task->_task_id);

	return ret_val;
}


_int32 em_post_mini_file_to_url(void* p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	EM_MINI_TASK * p_mini_param= (EM_MINI_TASK* )_p_param->_para1;

	EM_LOG_DEBUG("em_post_mini_file_to_url");
	
	if(em_is_net_ok(TRUE)==TRUE)
	{
		_p_param->_result = em_post_mini_file_to_url_impl( p_mini_param);
	}
	else
	{
		_p_param->_result = -1;
	}
	

		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 em_cancel_mini_task(void* p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	_u32   mini_id = (_u32 )_p_param->_para1;
	MINI_TASK * p_task = NULL;

	EM_LOG_ERROR("em_cancel_mini_task:%u",mini_id);
	
	p_task = mini_get_task_from_map(mini_id);
	if(p_task!=NULL) 
	{
		p_task->_failed_code = MSG_CANCELLED;
		p_task->_state = MTS_FINISHED;
		mini_delete_task(p_task);
	}
	else
	{
		eti_http_close(mini_id);
	}
	

		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 em_http_get_impl(EM_HTTP_GET * p_http_get ,_u32 * http_id,_int32 priority)
{
	_int32 ret_val = SUCCESS;
	ET_HTTP_PARAM et_http_param;

	EM_LOG_ERROR("em_http_get_impl :url[%u]=%s",p_http_get->_url_len,p_http_get->_url);

	em_memset(&et_http_param,0x00,sizeof(ET_HTTP_PARAM));

	et_http_param._url = p_http_get->_url;
	et_http_param._url_len= p_http_get->_url_len;
	et_http_param._ref_url = p_http_get->_ref_url;
	et_http_param._ref_url_len= p_http_get->_ref_url_len;
	et_http_param._cookie= p_http_get->_cookie;
	et_http_param._cookie_len= p_http_get->_cookie_len;
	et_http_param._range_from= p_http_get->_range_from;
	et_http_param._range_to = p_http_get->_range_to;
	et_http_param._accept_gzip= p_http_get->_accept_gzip;
	et_http_param._recv_buffer= p_http_get->_recv_buffer;
	et_http_param._recv_buffer_size= p_http_get->_recv_buffer_size;
	et_http_param._callback_fun= p_http_get->_callback_fun;
	et_http_param._user_data= p_http_get->_user_data;
	et_http_param._timeout= p_http_get->_timeout;
	et_http_param._priority = priority;
	
	if(FALSE==em_is_et_running())
	{
		ret_val = em_start_et(  );
		CHECK_VALUE(ret_val);
	}

	ret_val = eti_http_get(&et_http_param,http_id);
	CHECK_VALUE(ret_val);
		 
	EM_LOG_ERROR("em_http_get_impl end:url[%u]=%s,ret_val=%d,task_id=%u",p_http_get->_url_len,p_http_get->_url,ret_val,*http_id);

	return SUCCESS;
}

_int32 em_http_get(void* p_param)
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	EM_HTTP_GET  * p_http_get = (EM_HTTP_GET * )_p_param->_para1;
	_u32  * http_id = (_u32 * )_p_param->_para2;

	EM_LOG_DEBUG("em_http_get");

	if(em_is_net_ok(TRUE)==TRUE)
	{
		_p_param->_result = em_http_get_impl(p_http_get ,http_id,0);
	}
	else
	{
		_p_param->_result = -1;
	}

		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 em_http_get_file_impl(EM_HTTP_GET_FILE * p_http_get_file ,_u32 * http_id)
{
	_int32 ret_val = SUCCESS;
	ET_HTTP_PARAM et_http_param;
	EM_MINI_TASK mini_task_param;
	MINI_TASK *p_task=NULL;

	EM_LOG_ERROR("em_http_get_file_impl:url[%u]=%s,file[%u+%u]=%s+%s",
		p_http_get_file->_url_len,p_http_get_file->_url,p_http_get_file->_file_path_len,p_http_get_file->_file_name_len,p_http_get_file->_file_path,p_http_get_file->_file_name);

	em_memset(&et_http_param,0x00,sizeof(ET_HTTP_PARAM));
	em_memset(&mini_task_param,0x00,sizeof(EM_MINI_TASK));

	et_http_param._url = p_http_get_file->_url;
	et_http_param._url_len= p_http_get_file->_url_len;
	et_http_param._ref_url = p_http_get_file->_ref_url;
	et_http_param._ref_url_len= p_http_get_file->_ref_url_len;
	et_http_param._cookie= p_http_get_file->_cookie;
	et_http_param._cookie_len= p_http_get_file->_cookie_len;
	et_http_param._range_from= p_http_get_file->_range_from;
	et_http_param._range_to = p_http_get_file->_range_to;
	et_http_param._accept_gzip= p_http_get_file->_accept_gzip;
	et_http_param._recv_buffer= NULL;
	et_http_param._recv_buffer_size= 0;
	et_http_param._callback_fun= (void*) mini_http_resp_callback;
	et_http_param._user_data= p_http_get_file->_user_data;
	et_http_param._timeout= p_http_get_file->_timeout;
	et_http_param._priority = 0;
	
	if(FALSE==em_is_et_running())
	{
		ret_val = em_start_et(  );
		CHECK_VALUE(ret_val);
	}

	ret_val = mini_task_malloc(&p_task);
	CHECK_VALUE(ret_val);
	
	mini_task_param._url = p_http_get_file->_url;
	mini_task_param._url_len= p_http_get_file->_url_len;
	mini_task_param._is_file= TRUE;
	em_memcpy(mini_task_param._file_path,p_http_get_file->_file_path,p_http_get_file->_file_path_len);
	mini_task_param._file_path_len= p_http_get_file->_file_path_len;
	em_memcpy(mini_task_param._file_name,p_http_get_file->_file_name,p_http_get_file->_file_name_len);
	mini_task_param._file_name_len= p_http_get_file->_file_name_len;
	mini_task_param._send_data= NULL;
	mini_task_param._send_data_len= 0;
	mini_task_param._gzip= p_http_get_file->_accept_gzip;
	mini_task_param._recv_buffer= NULL;
	mini_task_param._recv_buffer_size= 0;
	mini_task_param._callback_fun= (void*) p_http_get_file->_callback_fun;
	mini_task_param._user_data= p_http_get_file->_user_data;
	mini_task_param._timeout= p_http_get_file->_timeout;
	em_memcpy(&p_task->_task_info,&mini_task_param,sizeof(EM_MINI_TASK));
	p_task->_is_file = TRUE;

	ret_val = eti_http_get(&et_http_param,http_id);
	if(ret_val!=SUCCESS)
	{
		mini_task_free(p_task);
		CHECK_VALUE(ret_val);
	}
	p_task->_task_id = *http_id;
		 
	ret_val = mini_add_task_to_map(p_task);
	if(ret_val!=SUCCESS)
	{
		eti_http_close(*http_id);
		mini_task_free(p_task);
		CHECK_VALUE(ret_val);
	}

	EM_LOG_ERROR("em_http_get_file_impl end:url[%u]=%s,ret_val=%d,task_id=%u",p_http_get_file->_url_len,p_http_get_file->_url,ret_val,*http_id);

	return ret_val;
}
_int32 em_http_get_file(void* p_param)
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	EM_HTTP_GET_FILE  * p_http_get_file = (EM_HTTP_GET_FILE * )_p_param->_para1;
	_u32  * http_id = (_u32 * )_p_param->_para2;

	EM_LOG_DEBUG("em_http_get_file");

	if(em_is_net_ok(TRUE)==TRUE)
	{
		_p_param->_result = em_http_get_file_impl(p_http_get_file ,http_id);
	}
	else
	{
		_p_param->_result = -1;
	}

		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 em_http_post_impl(EM_HTTP_POST * p_http_post ,_u32 * http_id)
{
	_int32 ret_val = SUCCESS;
	ET_HTTP_PARAM et_http_param;

	EM_LOG_ERROR("em_http_post_impl :url[%u]=%s",p_http_post->_url_len,p_http_post->_url);


	em_memset(&et_http_param,0x00,sizeof(ET_HTTP_PARAM));

	et_http_param._url = p_http_post->_url;
	et_http_param._url_len= p_http_post->_url_len;
	et_http_param._ref_url = p_http_post->_ref_url;
	et_http_param._ref_url_len= p_http_post->_ref_url_len;
	et_http_param._cookie= p_http_post->_cookie;
	et_http_param._cookie_len= p_http_post->_cookie_len;
	et_http_param._content_len= p_http_post->_content_len;
	et_http_param._send_gzip= p_http_post->_send_gzip;
	et_http_param._accept_gzip= p_http_post->_accept_gzip;
	et_http_param._send_data= p_http_post->_send_data;
	et_http_param._send_data_len= p_http_post->_send_data_len;
	et_http_param._recv_buffer= p_http_post->_recv_buffer;
	et_http_param._recv_buffer_size= p_http_post->_recv_buffer_size;
	et_http_param._callback_fun= p_http_post->_callback_fun;
	et_http_param._user_data= p_http_post->_user_data;
	et_http_param._timeout= p_http_post->_timeout;
	et_http_param._priority = 0;
	
	if(FALSE==em_is_et_running())
	{
		ret_val = em_start_et(  );
		CHECK_VALUE(ret_val);
	}

	ret_val = eti_http_post(&et_http_param,http_id);
	CHECK_VALUE(ret_val);
		 
	EM_LOG_ERROR("em_http_post_impl end:url[%u]=%s,ret_val=%d,task_id=%u",p_http_post->_url_len,p_http_post->_url,ret_val,*http_id);

	return SUCCESS;
}

_int32 em_http_post(void* p_param)
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	EM_HTTP_POST  * p_http_post = (EM_HTTP_POST * )_p_param->_para1;
	_u32  * http_id = (_u32 * )_p_param->_para2;

	EM_LOG_DEBUG("em_http_get");

	if(em_is_net_ok(TRUE)==TRUE)
	{
		_p_param->_result = em_http_post_impl(p_http_post ,http_id);
	}
	else
	{
		_p_param->_result = -1;
	}

		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 em_http_post_file_impl(EM_HTTP_POST_FILE * p_http_post_file ,_u32 * http_id)
{
	_int32 ret_val = SUCCESS;
	ET_HTTP_PARAM et_http_param;
	EM_MINI_TASK mini_task_param;
	MINI_TASK *p_task=NULL;
	_u64 file_size = 0;
	_u32 modify_time = 0;
	char full_path[MAX_FULL_PATH_BUFFER_LEN];

	EM_LOG_ERROR("em_http_post_file_impl:url[%u]=%s,file[%u+%u]=%s+%s",
		p_http_post_file->_url_len,p_http_post_file->_url,p_http_post_file->_file_path_len,p_http_post_file->_file_name_len,p_http_post_file->_file_path,p_http_post_file->_file_name);

	em_memset(&et_http_param,0x00,sizeof(ET_HTTP_PARAM));
	em_memset(&mini_task_param,0x00,sizeof(EM_MINI_TASK));


	em_memset(full_path,0x00,MAX_FULL_PATH_BUFFER_LEN);
	em_strncpy(full_path,p_http_post_file->_file_path,p_http_post_file->_file_path_len);
	if(full_path[p_http_post_file->_file_path_len - 1] != '/' )
		full_path[p_http_post_file->_file_path_len]='/';
	em_strcat(full_path,p_http_post_file->_file_name,p_http_post_file->_file_name_len);
	if(em_file_exist(full_path))
	{
		ret_val = em_get_file_size_and_modified_time(full_path, &file_size, &modify_time);
		CHECK_VALUE(ret_val);
	}
	else
		return FILE_NOT_EXIST;

	et_http_param._url = p_http_post_file->_url;
	et_http_param._url_len= p_http_post_file->_url_len;
	et_http_param._ref_url = p_http_post_file->_ref_url;
	et_http_param._ref_url_len= p_http_post_file->_ref_url_len;
	et_http_param._cookie= p_http_post_file->_cookie;
	et_http_param._cookie_len= p_http_post_file->_cookie_len;
	et_http_param._range_from= p_http_post_file->_range_from;
	et_http_param._range_to = p_http_post_file->_range_to;
	et_http_param._accept_gzip= p_http_post_file->_accept_gzip;
	et_http_param._recv_buffer= NULL;
	et_http_param._recv_buffer_size= 0;
	et_http_param._content_len = file_size;
	et_http_param._send_data = NULL;
	et_http_param._send_data_len = 0;
	et_http_param._callback_fun= (void*) mini_http_resp_callback;
	et_http_param._user_data= p_http_post_file->_user_data;
	et_http_param._timeout= p_http_post_file->_timeout;
	et_http_param._priority = 0;
	
	if(FALSE==em_is_et_running())
	{
		ret_val = em_start_et(  );
		CHECK_VALUE(ret_val);
	}

	ret_val = mini_task_malloc(&p_task);
	CHECK_VALUE(ret_val);
	
	mini_task_param._url = p_http_post_file->_url;
	mini_task_param._url_len= p_http_post_file->_url_len;
	mini_task_param._is_file= TRUE;
	em_memcpy(mini_task_param._file_path,p_http_post_file->_file_path,p_http_post_file->_file_path_len);
	mini_task_param._file_path_len= p_http_post_file->_file_path_len;
	em_memcpy(mini_task_param._file_name,p_http_post_file->_file_name,p_http_post_file->_file_name_len);
	mini_task_param._file_name_len= p_http_post_file->_file_name_len;
	mini_task_param._send_data= NULL;
	mini_task_param._send_data_len= file_size;
	mini_task_param._gzip= p_http_post_file->_accept_gzip;
	mini_task_param._recv_buffer= NULL;
	mini_task_param._recv_buffer_size= 0;
	mini_task_param._callback_fun= (void*) p_http_post_file->_callback_fun;
	mini_task_param._user_data= p_http_post_file->_user_data;
	mini_task_param._timeout= p_http_post_file->_timeout;
	em_memcpy(&p_task->_task_info,&mini_task_param,sizeof(EM_MINI_TASK));
	p_task->_is_file = TRUE;

	ret_val = eti_http_post(&et_http_param,http_id);
	if(ret_val!=SUCCESS)
	{
		mini_task_free(p_task);
		CHECK_VALUE(ret_val);
	}
	p_task->_task_id = *http_id;
		 
	ret_val = mini_add_task_to_map(p_task);
	if(ret_val!=SUCCESS)
	{
		eti_http_close(*http_id);
		mini_task_free(p_task);
		CHECK_VALUE(ret_val);
	}

	EM_LOG_ERROR("em_http_get_file_impl end:url[%u]=%s,ret_val=%d,task_id=%u",p_http_post_file->_url_len,p_http_post_file->_url,ret_val,*http_id);

	return ret_val;
}


_int32 em_http_post_file(void* p_param)
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	EM_HTTP_POST_FILE  * p_http_post_file = (EM_HTTP_POST_FILE * )_p_param->_para1;
	_u32  * http_id = (_u32 * )_p_param->_para2;

	EM_LOG_DEBUG("em_http_post_file");

	if(em_is_net_ok(TRUE)==TRUE)
	{
		_p_param->_result = em_http_post_file_impl(p_http_post_file ,http_id);
	}
	else
	{
		_p_param->_result = -1;
	}

	EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}


_int32 em_http_cancel(void* p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	_u32   http_id = (_u32  )_p_param->_para1;
	MINI_TASK * p_task = NULL;

	EM_LOG_ERROR("em_http_cancel:%u",http_id);

	p_task = mini_get_task_from_map(http_id);
	if(p_task!=NULL) 
	{
		p_task->_failed_code = MSG_CANCELLED;
		p_task->_state = MTS_FINISHED;
		mini_delete_task(p_task);
	}
	else
	{
		eti_http_close(http_id);
	}

		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 mini_task_malloc(MINI_TASK **pp_slip)
{
	_int32 ret_val = em_mpool_get_slip( gp_mini_task_slab, (void**)pp_slip );
	
	if(ret_val==SUCCESS) 
		em_memset(*pp_slip,0,sizeof(MINI_TASK));
	
      EM_LOG_DEBUG("mini_task_malloc"); 
	return ret_val;
}

_int32 mini_task_free(MINI_TASK * p_slip)
{
	sd_assert( p_slip != NULL );
	if( p_slip == NULL ) return SUCCESS;
      EM_LOG_DEBUG("mini_task_free"); 
	return em_mpool_free_slip( gp_mini_task_slab, (void*)p_slip );
}



_int32 mini_limit_speed(void)
{
	_int32 ret_val = SUCCESS;
#if defined(MOBILE_PHONE)
	_u32 download_limit_speed=MAX_U32,upload_limit_speed = MAX_U32,current_dl_speed=0;
	if(!g_speed_limited&&em_is_et_running()&&/*(sd_get_net_type()<NT_3G)&&*/(dt_have_running_task()/*||(0!=dt_get_running_vod_task_num( ))*/))
	{
		ret_val = eti_get_limit_speed(&download_limit_speed, &upload_limit_speed);
		CHECK_VALUE(ret_val);
		current_dl_speed = eti_get_current_download_speed();
      		EM_LOG_ERROR("mini_limit_speed:%u,%u,current_dl_speed=%u",download_limit_speed,upload_limit_speed,current_dl_speed); 
		if(current_dl_speed>8*1024)
		{
			download_limit_speed = current_dl_speed*8/(10*1024); //10;//
			eti_set_limit_speed(download_limit_speed, upload_limit_speed);
			g_speed_limited = TRUE;
		}
	}
#endif /* MOBILE_PHONE */
	return ret_val;
}

_int32 mini_unlimit_speed(void)
{
	//_int32 ret_val = SUCCESS;
	_u32 download_limit_speed=MAX_U32,upload_limit_speed = MAX_U32;
	
	if(g_speed_limited&&em_is_et_running())
	{
		em_settings_get_int_item("system.download_limit_speed", (_int32 *)&download_limit_speed);
		em_settings_get_int_item("system.upload_limit_speed", (_int32 *)&upload_limit_speed);
		eti_set_limit_speed(download_limit_speed, upload_limit_speed);
		g_speed_limited = FALSE;
	}
	return SUCCESS;
}


_int32 em_query_shub_by_url(void* p_param)
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	ET_QUERY_SHUB * p_query_param = (ET_QUERY_SHUB* )_p_param->_para1;
	_u32 * p_action_id = (_u32* )_p_param->_para2;

	EM_LOG_DEBUG("em_query_shub_by_url");
	
	if(em_is_net_ok(TRUE)==TRUE)
	{
		_p_param->_result = iet_query_shub_by_url(p_query_param,p_action_id);
	}
	else
	{
		_p_param->_result = -1;
	}
	

		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 em_cancel_query_shub(void* p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	_u32 action_id= (_u32)_p_param->_para1;

	EM_LOG_DEBUG("em_cancel_query_shub");
	
	_p_param->_result = et_cancel_query_shub( action_id);
	

	EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}


