/*--------------------------------------------------------------------------*/
/*                               IDENTIFICATION                             */
/*--------------------------------------------------------------------------*/
/*     Filename  : lixian_impl.c                                         */
/*     Author     : ZengYuqing                                              */
/*     Project    : lixian                                        */
/*     Module     : lixian													*/
/*     Version    : 1.0  													*/
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
/* This file contains the procedure of lixian                       */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*                              HISTORY                                     */
/*--------------------------------------------------------------------------*/
/*   Date     |    Author   | Modification                                  */
/*--------------------------------------------------------------------------*/
/* 2011.10.29 | ZengYuqing  | Creation                                      */
/*--------------------------------------------------------------------------*/

#include "utility/define.h"

#ifdef ENABLE_LIXIAN_ETM
#include "lixian/lixian_impl.h"
#include "lixian/lixian_interface.h"
#include "lixian/lixian_protocol.h"

#include "utility/base64.h"
#include "utility/peerid.h"
#include "utility/version.h"
#include "utility/time.h"
#include "utility/local_ip.h"
#include "utility/sd_iconv.h"

#include "scheduler/scheduler.h"
#include "tree_manager/tree_impl.h"
#include "download_manager/download_task_interface.h"
#include "download_manager/mini_task_interface.h"

#include "em_common/em_define.h"
#include "et_interface/et_interface.h"
#include "em_interface/em_fs.h"
#include "em_common/em_utility.h"
#include "em_common/em_time.h"
#include "em_common/em_url.h"
#include "em_common/em_logid.h"
#ifdef EM_LOGID
#undef EM_LOGID
#endif

#define EM_LOGID LOGID_LIXIAN

#include "em_common/em_logger.h"

/*--------------------------------------------------------------------------*/
/*                Global declarations				        */
/*--------------------------------------------------------------------------*/
static LX_MANAGER g_lx_mgr ;
static MAP * gp_sniffed_urls_map = NULL; /* ��̽������ҳ������ɹ����ص�URL */
static MAP * gp_downloadable_urls = NULL; /* �ɹ����ص�URL */
/*--------------------------------------------------------------------------*/
/*                Interfaces provid for other module				        */
/*--------------------------------------------------------------------------*/

_int32 lx_init_mgr(void)
{
	_int32 ret_val = SUCCESS;

	sd_memset(&g_lx_mgr,0x00,sizeof(LX_MANAGER));
	list_init(&g_lx_mgr._action_list);
	map_init(&g_lx_mgr._task_map,lx_task_id_comp);
	map_init(&g_lx_mgr._task_list_action_map,lx_action_offset_comp);
	list_init(&g_lx_mgr._task_list);
	
	return ret_val;
}
_int32 lx_uninit_mgr(void)
{
	_int32 ret_val = SUCCESS;
	g_lx_mgr._cmd_protocal_seq = 0;
	g_lx_mgr._jump_key_len = 0;
	sd_memset(&g_lx_mgr._jump_key,0x00,sizeof(LX_JUMPKEY_MAX_SIZE));
	
	lx_clear_action_list();
	lx_clear_task_map();
	lx_clear_task_list_not_free();
	lx_clear_downloadable_url_map();
	lx_clear_url_map();
	#ifdef LX_GET_TASK_LIST_NEW
	lx_clear_task_list_action_map_new();
	#else
	lx_clear_task_list_action_map();
	#endif
	return ret_val;
}

/* �˳����߿ռ� */
_int32 lx_logout(void)
{
	_int32 ret_val = SUCCESS;
	EM_LOG_DEBUG( "lx_logout" );

	sd_memset(&g_lx_mgr._base,0x00,sizeof(LX_BASE));

	sd_memset(&g_lx_mgr._cookie,0x00,sizeof(MAX_URL_LEN));
	sd_memset(&g_lx_mgr._download_cookie,0x00,sizeof(MAX_URL_LEN));
	
	g_lx_mgr._jump_key_len = 0;
	sd_memset(&g_lx_mgr._jump_key,0x00,sizeof(LX_JUMPKEY_MAX_SIZE));
	
	g_lx_mgr._cmd_protocal_seq = 0;
	
	lx_clear_action_list_except_sniff();

	g_lx_mgr._max_task_num = 0;
	g_lx_mgr._current_task_num = 0;
	g_lx_mgr._total_space = 0;
	g_lx_mgr._available_space = 0;

	lx_clear_task_map();
	lx_clear_task_list_not_free();
	lx_clear_task_list_action_map();
	return ret_val;
}


_int32 lx_set_base(_u64 user_id,const char * user_name,const char * old_user_name,_int32 vip_level,const char * session_id)
{
	_int32 ret_val = SUCCESS;

	EM_LOG_DEBUG("lx_set_base: user_id = %llu, vip_level = %d, session_id = %s", user_id, vip_level, session_id); 
	g_lx_mgr._base._net = 0;
	g_lx_mgr._base._from = 2;  // 2 �ƶ��豸
	g_lx_mgr._base._userid = user_id;
	g_lx_mgr._base._vip_level = vip_level;
	if (user_name) {
		sd_strncpy(g_lx_mgr._base._user_name, user_name, MAX_USER_NAME_LEN-1);
	}
	if (old_user_name) {
		sd_strncpy(g_lx_mgr._base._user_name_old, old_user_name, MAX_USER_NAME_LEN-1);
	}
	
	sd_strncpy(g_lx_mgr._base._session_id, session_id, MAX_SESSION_ID_LEN-1);

	g_lx_mgr._base._local_ip = sd_get_local_ip();

	sd_snprintf(g_lx_mgr._cookie,MAX_URL_LEN-1, "Cookie: userid=%llu",user_id);
	
	return ret_val;
}

_int32 lx_set_sessionid(const char * session_id)
{
	_int32 ret_val = SUCCESS;

	EM_LOG_DEBUG("lx_set_sessionid: session_id = %s", session_id); 

	sd_memset(g_lx_mgr._base._session_id, 0x00, MAX_SESSION_ID_LEN);

	sd_strncpy(g_lx_mgr._base._session_id, session_id, MAX_SESSION_ID_LEN - 1);

	return ret_val;
}

LX_MANAGER* lx_get_manager(void)
{
	return &g_lx_mgr;
}

LX_BASE * lx_get_base(void)
{
	return &g_lx_mgr._base;
}
BOOL lx_is_logined(void)
{
	if(g_lx_mgr._base._userid !=0)
		return TRUE;
	return FALSE;
}
_int32 lx_set_download_cookie(const char * cookie)
{
	_int32 ret_val = SUCCESS;

	if(g_lx_mgr._download_cookie[0]=='\0')
	{
		sd_strncpy(g_lx_mgr._download_cookie, cookie, MAX_URL_LEN-1);
	}
	
	return ret_val;
}

_int32 lx_set_section_type(_int32 type)
{
	g_lx_mgr._base._section_type = type;
	return SUCCESS;
}

_int32 lx_get_section_type(_int32 type)
{
	return g_lx_mgr._base._section_type;
}

_int32 lx_get_aes_key( char aes_key[MAX_SESSION_ID_LEN])
{
	sd_snprintf(aes_key, MAX_SESSION_ID_LEN-1, "Cookie: key=%s",g_lx_mgr._base._session_id);
	return SUCCESS;
}

_int32 lx_get_session_id( char sessionid[MAX_SESSION_ID_LEN])
{
	sd_snprintf(sessionid, MAX_SESSION_ID_LEN-1, "Cookie: Sessionid=%s",g_lx_mgr._base._session_id);
	return SUCCESS;
}

const char * lx_get_server_url()
{
	static char server_buffer[128] = {0};

	if(sd_strlen(server_buffer)==0)
	{
		sd_snprintf(server_buffer, 127,"http://%s:%u/",DEFAULT_LIXIAN_HOST_NAME,DEFAULT_LIXIAN_PORT);
		//return "http://121.10.137.164:8889/wireless_interface"; // "http://221.238.25.224:8889/";
	}
	return server_buffer;
}

const char * lx_get_free_strategy_server_url()
{
	static char free_strategy_server[128] = {0};

	if(sd_strlen(free_strategy_server)==0)
	{
		sd_snprintf(free_strategy_server, 127, "http://wireless.yun.vip.runmit.com:80/xlShareFile?encrypt=1");
	}
	return free_strategy_server;
}

const char * lx_get_high_speed_flux_server_url()
{
	static char high_speed_flux_server[128] = {0};

	if(sd_strlen(high_speed_flux_server)==0)
	{
		sd_snprintf(high_speed_flux_server, 127, "http://wireless.yun.vip.runmit.com:80/xlCloudShare?encrypt=1");
	}
	return high_speed_flux_server;
}

const char * lx_get_task_off_flux_server_url()
{
	static char task_off_flux[128] = {0};

	if(sd_strlen(task_off_flux)==0)
	{
		sd_snprintf(task_off_flux, 127,"http://%s:%u/",DEFAULT_HIGH_SPEED_SERVER_HOST_NAME, DEFAULT_HIGH_SPEED_SERVER_PORT);
	}
	return task_off_flux;
}

_int32 lx_get_cookie_impl(char* cookie_buffer)
{
	_int32 ret_val = SUCCESS;
	
	if(sd_strlen(g_lx_mgr._cookie)!=0)
	{
		sd_memcpy(cookie_buffer, g_lx_mgr._cookie, MAX_URL_LEN);

		#if 0
		/* ���������ص�cookieҲ��ӵ�cookie�� */
		if(sd_strlen(g_lx_mgr._download_cookie)!=0)
		{
			sd_strcat(cookie_buffer, "; ", 2);
			sd_strcat(cookie_buffer, g_lx_mgr._download_cookie, sd_strlen(g_lx_mgr._download_cookie));
		}
		#endif
	}

	return ret_val;
}

_u32 lx_get_cmd_protocal_seq(void)
{
	return g_lx_mgr._cmd_protocal_seq++;
}


_int32 lx_set_jumpkey(char* jumpkey, _u32 jumpkey_len)
{
	if(jumpkey_len > LX_JUMPKEY_MAX_SIZE)
	{
		return -1;
	}
    g_lx_mgr._jump_key_len = jumpkey_len;
	sd_memcpy(g_lx_mgr._jump_key, jumpkey, jumpkey_len);
    
    /* get user info */
    //lx_get_user_info_req();
    
	return SUCCESS;
}

_int32 lx_get_jumpkey(char* jumpkey_buffer, _u32* len)
{
	if(g_lx_mgr._jump_key_len > 0)
	{
		*len = g_lx_mgr._jump_key_len;
		sd_memcpy(jumpkey_buffer, g_lx_mgr._jump_key, g_lx_mgr._jump_key_len);
		return SUCCESS;
	}
	else
	{
		return -1;	
	}
}

_int32 lx_set_user_lixian_info(_u32 max_task_num,_u32 current_task_num,_u64 total_space,_u64 available_space)
{
	EM_LOG_DEBUG( "lx_set_user_lixian_info:max_task_num=%u, current_task_num=%u, total_space=%llu, available_space=%llu",max_task_num, current_task_num, total_space, available_space);
	g_lx_mgr._current_task_num = current_task_num;
	g_lx_mgr._max_task_num = max_task_num;
	g_lx_mgr._total_space = total_space;
	g_lx_mgr._available_space = available_space;
	return SUCCESS;
}

/* ��ȡ�û����߿ռ���Ϣ*/
_int32 lx_get_space(_u64 * p_max_space,_u64 * p_available_space)
{
	if( 0 == g_lx_mgr._total_space )
		return LXE_SPACE_SIZE_NULL;
	*p_max_space = g_lx_mgr._total_space;
	*p_available_space = g_lx_mgr._available_space;
	return SUCCESS;
}
_int32 lx_get_task_num(_u32 * p_max_task_num,_u32 * p_current_task_num)
{
	*p_max_task_num = g_lx_mgr._max_task_num;
	*p_current_task_num = g_lx_mgr._current_task_num;
	return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////////


_int32 lx_add_action_to_list(LX_PT * p_action)
{
	_int32 ret_val = SUCCESS;
	ret_val = list_push(&g_lx_mgr._action_list,( void * )p_action);
	CHECK_VALUE(ret_val);

	lx_start_dispatch();

	return SUCCESS;
}
_int32 lx_remove_action_from_list(LX_PT * p_action)
{
	pLIST_NODE cur_node = NULL;
	LX_PT * p_action_in_list = NULL;
	
      EM_LOG_DEBUG("lx_remove_action_from_list"); 
	if(list_size(&g_lx_mgr._action_list)==0)	
	{
		CHECK_VALUE( LXE_ACTION_NOT_FOUND);
	}

	cur_node = LIST_BEGIN(g_lx_mgr._action_list);
	while(cur_node!=LIST_END(g_lx_mgr._action_list))
	{
		
		p_action_in_list = (LX_PT * )LIST_VALUE(cur_node);
		if(p_action == p_action_in_list)
		{
			list_erase(&g_lx_mgr._action_list, cur_node);
			return SUCCESS;
		}

		cur_node = LIST_NEXT(cur_node);
	}

	return LXE_ACTION_NOT_FOUND;
}
LX_PT *  lx_get_action_from_list(_u32 action_id)
{
	pLIST_NODE cur_node = NULL;
	LX_PT * p_action = NULL;
	
      EM_LOG_DEBUG("lx_get_action_from_list"); 
	if(list_size(&g_lx_mgr._action_list)==0)	
	{
		return NULL;
	}

	cur_node = LIST_BEGIN(g_lx_mgr._action_list);
	while(cur_node!=LIST_END(g_lx_mgr._action_list))
	{
		
		p_action = (LX_PT * )LIST_VALUE(cur_node);
		if(action_id == p_action->_action_id)
		{
			return p_action;
		}

		cur_node = LIST_NEXT(cur_node);
	}

	return NULL;
}

_int32 lx_check_action_in_list(LX_PT * p_action)
{
	pLIST_NODE cur_node = NULL;
	LX_PT * p_action_in_list = NULL;
	
      EM_LOG_DEBUG("lx_handle_action_list"); 
	if(list_size(&g_lx_mgr._action_list)==0)	
	{
		CHECK_VALUE( LXE_ACTION_NOT_FOUND);
	}

	cur_node = LIST_BEGIN(g_lx_mgr._action_list);
	while(cur_node!=LIST_END(g_lx_mgr._action_list))
	{
		
		p_action_in_list = (LX_PT * )LIST_VALUE(cur_node);
		if(p_action == p_action_in_list)
		{
			return SUCCESS;
		}

		cur_node = LIST_NEXT(cur_node);
	}

	return LXE_ACTION_NOT_FOUND;
}

/////////////////////////////////////////////////////////////////////////////////////

_int32 lx_add_task_to_list(LIST *list,LX_TASK_INFO_EX * p_task)
{ 
	return list_push(list,(void *)p_task);
}

_int32 lx_add_new_create_task_to_list(LIST *list, LX_TASK_INFO_EX * p_task)
{ 	
	_int32 ret_val = SUCCESS;

	LIST_ITERATOR list_begin = NULL;

	LX_TASK_INFO_EX *p_task_in_map = NULL;
	
	EM_LOG_DEBUG("lx_add_new_create_task_to_list list_size(path_list)= %d", list_size(list));

	list_begin = LIST_BEGIN(*list);

	ret_val = list_insert(list,(void *)p_task, list_begin);

	return ret_val;
}
_int32 lx_remove_task_from_list(LIST *list,void *task_id)
{
	pLIST_NODE cur_node = NULL;
	LX_PT * p_action_in_list = NULL;
	
	if(list_size(&g_lx_mgr._task_list)==0)	
	{
		CHECK_VALUE( LXE_TASK_ID_NOT_FOUND);
	}

	cur_node = LIST_BEGIN(g_lx_mgr._task_list);
	while(cur_node!=NULL && cur_node!=LIST_END(g_lx_mgr._task_list))
	{
		LX_TASK_INFO_EX * p_task = (LX_TASK_INFO_EX *)LIST_VALUE(cur_node);
		if(p_task && *((_u64 *)task_id) == p_task->_task_id)   //compare the task id
		{
			list_erase(&g_lx_mgr._task_list, cur_node);
			return SUCCESS;
		}

		cur_node = LIST_NEXT(cur_node);
	}

	return LXE_TASK_ID_NOT_FOUND;
}


_int32 lx_clear_task_list_not_free(void)
{
	_int32 ret_val = SUCCESS;
	pLIST_NODE cur_node = NULL ,nxt_node = NULL ;
	_u64 * task_id = NULL;
	
	EM_LOG_DEBUG("lx_clear_task_list_not_free"); 
	if(list_size(&g_lx_mgr._task_list) == 0)	
	{
		return SUCCESS;
	}

	//do not free the memory (node->data),clear map will do it
	list_clear(&g_lx_mgr._task_list);

	return ret_val;
}


/////////////////////////////////////////////////////////////////////////////////////
_int32 lx_mini_http_post_callback(ET_HTTP_CALL_BACK * p_http_call_back_param)
{
	_int32 ret_val = SUCCESS;
	LX_PT * p_action = (LX_PT *)p_http_call_back_param->_user_data;

      EM_LOG_ERROR("lx_mini_http_post_callback:task_id=%u,type=%d",p_http_call_back_param->_http_id,p_http_call_back_param->_type); 

	/* ����ĳ��������ڸò���������list֮ǰ���Ѿ��ص���,�����������һ��ѭ����ʱһ��.zyq @20110727 */
	/*  ����������,��ͬ�̲߳���ͬһ��list���о���,Ҫ������� */
	ret_val = lx_check_action_in_list(p_action);
	sd_assert(ret_val==SUCCESS);
	if(ret_val!=SUCCESS) 
	{
		sd_assert(FALSE);
		return -1;
	}
	
	switch(p_http_call_back_param->_type)
	{
		case 	EHT_NOTIFY_RESPN:
		{
			_u32 http_status_code = *((_u32*) p_http_call_back_param->_header);
			p_action->_resp_status= (_int32) http_status_code;
		}
		break;
		case EHT_GET_SEND_DATA:
			sd_assert(FALSE);
		break;

		case EHT_NOTIFY_SENT_DATA:
			//printf("\n lx_mini_http_post_callback:EHT_NOTIFY_SENT_DATA:data_len=%u\n",p_http_call_back_param->_sent_data_len);
		break;

		case EHT_GET_RECV_BUFFER:
		{
			*p_http_call_back_param->_recv_buffer = p_action->_resp_buffer;
			*p_http_call_back_param->_recv_buffer_len = MAX_POST_RESP_BUFFER_LEN;
		}
		break;
		case 	EHT_PUT_RECVED_DATA:
		{
			_u64 filepos = p_action->_resp_data_len;
			_u32 writesize = 0;
			if(p_http_call_back_param->_recved_data_len!=0)
			{
				if(p_action->_file_id ==0) //�ļ��Ƿ��Ѵ�
				{
					if(em_file_exist(p_action->_file_path))
					{
						/* �Ѿ��ļ�ɾ���� */
						ret_val = em_delete_file(p_action->_file_path);
						sd_assert(ret_val ==SUCCESS );
					}
					ret_val = em_open_ex(p_action->_file_path, O_FS_CREATE, &p_action->_file_id);
					CHECK_VALUE(ret_val);
				}

				ret_val = em_pwrite(p_action->_file_id, (char *)p_http_call_back_param->_recved_data,(_int32) p_http_call_back_param->_recved_data_len, filepos, &writesize);
				CHECK_VALUE(ret_val);
				
  				EM_LOG_ERROR("EHT_PUT_RECVED_DATA,em_pwrite=%d,writesize=%u",ret_val,writesize); 
				
				sd_assert(writesize==p_http_call_back_param->_recved_data_len);

				p_action->_resp_data_len+=writesize;
			}
		}
		break;
		case 	EHT_NOTIFY_FINISHED:
		{
      			EM_LOG_ERROR("EHT_NOTIFY_FINISHED,result=%d",p_http_call_back_param->_result); 
			p_action->_error_code = p_http_call_back_param->_result;

#if 0 //def _DEBUG	  
	  	printf("\n ---------------------------\n"); 
		printf("\n EHT_NOTIFY_FINISHED:%d,resp_data_len=%u\n",p_http_call_back_param->_result,p_action->_resp_data_len); 
	  	printf("\n ---------------------------\n"); 
#endif /* _DEBUG */

			if(p_action->_file_id !=0)
			{
				/* �ر��ļ� */
				ret_val = em_close_ex(p_action->_file_id);
				sd_assert(ret_val==SUCCESS);
				p_action->_file_id = 0;
			}
			
			if(p_http_call_back_param->_result == SUCCESS)
			{
				p_action->_state = LPS_SUCCESS;
			}
			else
			{
				p_action->_state = LPS_FAILED;
			}
		}
		break;
	}
	return SUCCESS;

}

// ֻΪ��Ѳ���ʹ�õ�http�ص�����
_int32 lx_mini_http_post_not_lixian_callback(ET_HTTP_CALL_BACK * p_http_call_back_param)
{
	_int32 ret_val = SUCCESS;
	LX_PT * p_action = (LX_PT *)p_http_call_back_param->_user_data;

    EM_LOG_ERROR("lx_mini_http_post_not_lixian_callback:http_id=%u,type=%d",p_http_call_back_param->_http_id,p_http_call_back_param->_type); 

	switch(p_http_call_back_param->_type)
	{
		case 	EHT_NOTIFY_RESPN:
		{
			_u32 http_status_code = *((_u32*) p_http_call_back_param->_header);
			p_action->_resp_status= (_int32) http_status_code;
		}
			break;
		case EHT_GET_SEND_DATA:
			sd_assert(FALSE);
			break;
		case EHT_NOTIFY_SENT_DATA:
			break;
		case EHT_GET_RECV_BUFFER:
			{	
				*p_http_call_back_param->_recv_buffer = p_action->_resp_buffer;
				*p_http_call_back_param->_recv_buffer_len = MAX_POST_RESP_BUFFER_LEN;
			}
			break;
		case EHT_PUT_RECVED_DATA:
		{
			_u64 filepos = p_action->_resp_data_len;
			_u32 writesize = 0;
			if(p_http_call_back_param->_recved_data_len!=0)
			{
				if(p_action->_file_id ==0) //�ļ��Ƿ��Ѵ�
				{
					if(em_file_exist(p_action->_file_path))
					{
						/* �Ѿ��ļ�ɾ���� */
						ret_val = em_delete_file(p_action->_file_path);
						sd_assert(ret_val ==SUCCESS );
					}
					ret_val = em_open_ex(p_action->_file_path, O_FS_CREATE, &p_action->_file_id);
					CHECK_VALUE(ret_val);
				}

				ret_val = em_pwrite(p_action->_file_id, (char *)p_http_call_back_param->_recved_data,(_int32) p_http_call_back_param->_recved_data_len, filepos, &writesize);
				CHECK_VALUE(ret_val);
				
  				EM_LOG_ERROR("EHT_PUT_RECVED_DATA,em_pwrite=%d,writesize=%u",ret_val,writesize); 
				
				sd_assert(writesize==p_http_call_back_param->_recved_data_len);

				p_action->_resp_data_len+=writesize;
			}
		}
			break;
		case EHT_NOTIFY_FINISHED:
		{
      		EM_LOG_ERROR("EHT_NOTIFY_FINISHED,result=%d",p_http_call_back_param->_result); 
			p_action->_error_code = p_http_call_back_param->_result;

			if(p_action->_file_id !=0)
			{
				/* �ر��ļ� */
				ret_val = em_close_ex(p_action->_file_id);
				sd_assert(ret_val==SUCCESS);
				p_action->_file_id = 0;
			}
			if(p_http_call_back_param->_result == SUCCESS)
			{
				p_action->_state = LPS_SUCCESS;
			}
			else
			{
				p_action->_state = LPS_FAILED;
			}
			if( LPT_FREE_STRATEGY == p_action->_type )
				lx_get_free_strategy_resp(p_action);
			else if( LPT_GET_HIGH_SPEED_FLUX == p_action->_type )
				lx_get_high_speed_flux_resp(p_action);
			else if( LPT_FREE_EXPERIENCE_MEMBER == p_action->_type || LPT_GET_EXPERIENCE_MEMBER_REMAIN_TIME == p_action->_type )
				lx_get_free_experience_member_resp(p_action);
		}
			break;
	}
	return SUCCESS;
}


_int32 lx_post_req(LX_PT * p_action,_u32 * p_http_id,_int32 priority)
{
 	_int32 ret_val = SUCCESS;
	ET_HTTP_PARAM http_param = {0};
	static char server_url[128] = {0};
	char sessionid[MAX_SESSION_ID_LEN] = {0};

	if(p_action->_type == LPT_CRE_TASK || p_action->_type == LPT_DEL_TASK || p_action->_type == LPT_DEL_TASKS|| p_action->_type == LPT_CRE_BT_TASK 
		|| p_action->_type == LPT_DELAY_TASK || p_action->_type == LPT_GET_USER_INFO 
		|| p_action->_type == LPT_GET_OD_OR_DEL_LS || p_action->_type == LPT_MINIQUERY_TASK
		|| p_action->_type == LPT_QUERY_TASK_INFO || p_action->_type == LPT_QUERY_BT_TASK_INFO
		|| p_action->_type == LPT_BATCH_QUERY_TASK_INFO || p_action->_type == LPT_BINARY_TASK_LS
		|| p_action->_type == LPT_TASK_LS_NEW)
	{
		if(sd_strlen(server_url)==0)
		{
			sd_snprintf(server_url, 127,"http://%s:%u/",DEFAULT_LIXIAN_SERVER_HOST_NAME, DEFAULT_LIXIAN_SERVER_PORT);
		}
		http_param._url = server_url;
	}
	else if(p_action->_type == LPT_FREE_STRATEGY )
	{
		http_param._url = (char*)lx_get_free_strategy_server_url();
	}
	else if(p_action->_type == LPT_GET_HIGH_SPEED_FLUX || p_action->_type == LPT_FREE_EXPERIENCE_MEMBER|| p_action->_type == LPT_GET_EXPERIENCE_MEMBER_REMAIN_TIME)
	{
		http_param._url = (char*)lx_get_high_speed_flux_server_url();
	}
	else if(p_action->_type == LPT_TAKE_OFF_FLUX)
	{
		http_param._url = (char*)lx_get_task_off_flux_server_url();
	}
	else
	{
		http_param._url = (char*)lx_get_server_url();	
	}
	http_param._url_len= sd_strlen(http_param._url);

	if(p_action->_is_aes)
	{
		http_param._cookie = p_action->_aes_key;
		http_param._cookie_len= sd_strlen(http_param._cookie);
		// ��ȡ�㲥url�������Ӷ���֤�߼���cookies�������sessionid
		if( p_action->_type == LPT_VOD_URL )
		{
			lx_get_session_id(sessionid);
			sd_strcat(http_param._cookie, sessionid, MAX_SESSION_ID_LEN);
			http_param._cookie_len= sd_strlen(http_param._cookie);
		}
	}
	else
	{
		// ��ȡ�㲥url�������Ӷ���֤�߼���cookies�������sessionid
		if( p_action->_type == LPT_VOD_URL )
		{
			lx_get_session_id(sessionid);
			http_param._cookie = sessionid;
			http_param._cookie_len= sd_strlen(http_param._cookie);
		}
	}
	
	http_param._content_len = p_action->_req_data_len;

	http_param._send_gzip = p_action->_is_compress;
	http_param._accept_gzip = p_action->_is_compress;
	
	http_param._send_data = (_u8*)p_action->_req_buffer;
	http_param._send_data_len = p_action->_req_data_len;

	http_param._recv_buffer =(void*) p_action->_resp_buffer;
	p_action->_resp_buffer_len = MAX_POST_RESP_BUFFER_LEN;
	http_param._recv_buffer_size = p_action->_resp_buffer_len;
	if(p_action->_type == LPT_FREE_STRATEGY || p_action->_type == LPT_GET_HIGH_SPEED_FLUX
		|| p_action->_type == LPT_FREE_EXPERIENCE_MEMBER || p_action->_type == LPT_GET_EXPERIENCE_MEMBER_REMAIN_TIME )
		http_param._callback_fun = (void *)lx_mini_http_post_not_lixian_callback;
	else
		http_param._callback_fun = (void *)lx_mini_http_post_callback;
	http_param._user_data = (void*)p_action;
	http_param._timeout = LX_DEFAULT_HTTP_POST_TIMEOUT;

	http_param._priority = priority;

	ret_val = eti_http_post(&http_param,p_http_id);
	CHECK_VALUE(ret_val);
	return SUCCESS;
}


_int32 lx_mini_http_get_callback(ET_HTTP_CALL_BACK * p_http_call_back_param)
{
	_int32 ret_val = SUCCESS;
	LX_PT * p_action = (LX_PT *)p_http_call_back_param->_user_data;

      EM_LOG_ERROR("lx_mini_http_get_callback:task_id=%u,type=%d",p_http_call_back_param->_http_id,p_http_call_back_param->_type); 

	/* ����ĳ��������ڸò���������list֮ǰ���Ѿ��ص���,�����������һ��ѭ����ʱһ��.zyq @20110727 */
	/*  ����������,��ͬ�̲߳���ͬһ��list���о���,Ҫ������� */
	ret_val = lx_check_action_in_list(p_action);
	sd_assert(ret_val==SUCCESS);
	if(ret_val!=SUCCESS) 
	{
		sd_assert(FALSE);
		return -1;
	}

	/* ������Ӧ */
	switch(p_http_call_back_param->_type)
	{
		case 	EHT_NOTIFY_RESPN:
		{
			_u32 http_status_code = *((_u32*) p_http_call_back_param->_header);
			char * conten_len =  et_get_http_header_value(p_http_call_back_param->_header,EHV_CONTENT_LENGTH);
			p_action->_resp_status= (_int32) http_status_code;

			if(conten_len!=NULL)
			{
				ret_val = sd_str_to_u64( conten_len,sd_strlen(conten_len), &p_action->_resp_content_len );
				sd_assert(ret_val==SUCCESS);
			}
		}
		break;
		case EHT_GET_SEND_DATA:
			sd_assert(FALSE);
		break;
		case EHT_NOTIFY_SENT_DATA:
			//printf("\n lx_mini_http_get_callback:EHT_NOTIFY_SENT_DATA:data_len=%u\n",p_http_call_back_param->_sent_data_len);
			sd_assert(FALSE);
		break;

		case EHT_GET_RECV_BUFFER:
			*p_http_call_back_param->_recv_buffer = p_action->_resp_buffer;
			*p_http_call_back_param->_recv_buffer_len = MAX_POST_RESP_BUFFER_LEN;
		break;
		case 	EHT_PUT_RECVED_DATA:
		{
			_u64 filepos = p_action->_resp_data_len;
			_u32 writesize = 0;
			if(p_http_call_back_param->_recved_data_len!=0)
			{
				if(p_action->_file_id ==0)
				{
					if(em_file_exist(p_action->_file_path))
					{
						/* �Ѿ��ļ�ɾ���� */
						ret_val = em_delete_file(p_action->_file_path);
						sd_assert(ret_val ==SUCCESS );
					}
					ret_val = em_open_ex(p_action->_file_path, O_FS_CREATE, &p_action->_file_id);
					CHECK_VALUE(ret_val);
				}

				ret_val = em_pwrite(p_action->_file_id, (char *)p_http_call_back_param->_recved_data,(_int32) p_http_call_back_param->_recved_data_len, filepos, &writesize);
				CHECK_VALUE(ret_val);
				
  				EM_LOG_ERROR("EHT_PUT_RECVED_DATA,em_pwrite=%d,writesize=%u",ret_val,writesize); 
				
				sd_assert(writesize==p_http_call_back_param->_recved_data_len);

				p_action->_resp_data_len+=writesize;
			}
		}
		break;
		case 	EHT_NOTIFY_FINISHED:
		{

			p_action->_error_code = p_http_call_back_param->_result;

		      EM_LOG_ERROR("lx_mini_http_get_callback EHT_NOTIFY_FINISHED:%d,get file data len:%u,should receive bytes:%llu",p_http_call_back_param->_result,p_action->_resp_data_len,p_action->_resp_content_len); 
#if 0 //def _DEBUG	  
			printf("\n ---------------------------\n"); 
			printf("\n lx_mini_http_get_callback EHT_NOTIFY_FINISHED:%d,get file data len:%u\n",p_http_call_back_param->_result,p_action->_resp_data_len); 
			printf("\n ---------------------------\n"); 
#endif /* _DEBUG */

			if(p_action->_file_id !=0)
			{
				/* �ر��ļ� */
				ret_val = em_close_ex(p_action->_file_id);
				sd_assert(ret_val==SUCCESS);
				p_action->_file_id = 0;
			}
			
			if(p_action->_error_code == SUCCESS && p_action->_resp_data_len<p_action->_resp_content_len)
			{
				p_action->_error_code = FILE_SIZE_NOT_BELIEVE;
			}
				
			if(p_action->_error_code == SUCCESS )
			{
				p_action->_state = LPS_SUCCESS;
			}
			else
			{
				p_action->_state = LPS_FAILED;
			}
		}
		break;
	}
	return SUCCESS;

}

_int32 lx_get_file_req(LX_PT * p_action)
{
 	_int32 ret_val = SUCCESS;
	_u32 http_timeout = LX_DEFAULT_HTTP_GET_TIMEOUT;
	ET_HTTP_PARAM http_param = {0};

	if(p_action->_type == LPT_DL_PIC)
	{
		http_param._url = (char*)(((LX_DL_FILE*)p_action)->_url);
	}else
	if(p_action->_type == LPT_GET_DL_URL)
	{
		http_param._url = (char*)(((LX_PT_GET_DL_URL*)p_action)->_url);
		http_timeout = 2*LX_DEFAULT_HTTP_GET_TIMEOUT;
	}else
	if(p_action->_type == LPT_GET_REGEX)
	{
		http_param._url = (char*)(((LX_PT_GET_REGEX*)p_action)->_url);
	}else
	if(p_action->_type == LPT_GET_DETECT_STRING)
	{
		http_param._url = (char*)(((LX_PT_GET_DETECT_STRING*)p_action)->_url);
	}	
	else
	{
		CHECK_VALUE(-1);
	}
	
	
	http_param._url_len= sd_strlen(http_param._url);

	//http_param._ref_url = (char*)p_dl->_ref_url;
	//http_param._ref_url_len= sd_strlen(http_param._ref_url);

	//http_param._cookie= (char*)p_dl->_cookie;
	//http_param._cookie_len= sd_strlen(http_param._cookie);


	//http_param._send_gzip = p_action->_is_compress;
	//http_param._accept_gzip = p_action->_is_compress;
	
	//http_param._send_data = (_u8*)p_action->_req_buffer;
	//http_param._send_data_len = p_action->_req_data_len;

	http_param._recv_buffer =(void*) p_action->_resp_buffer;
	p_action->_resp_buffer_len = MAX_POST_RESP_BUFFER_LEN;
	http_param._recv_buffer_size = p_action->_resp_buffer_len;

	http_param._callback_fun = (void *)lx_mini_http_get_callback;
	http_param._user_data = (void*)p_action;
	http_param._timeout = http_timeout;
	http_param._priority = -1;

      EM_LOG_ERROR("lx_get_file_req[%u]:\n%s",http_param._url_len,http_param._url); 
	  
#if 0 //def _DEBUG	  
	  	printf("\n ---------------------------\n"); 
	printf("\n lx_get_file_req[%u]:\n%s\n",http_param._url_len,http_param._url); 
	  	printf("\n ---------------------------\n"); 
#endif /* _DEBUG */
	p_action->_state = LPS_RUNNING;
	ret_val = eti_http_get(&http_param,&p_action->_action_id);
	sd_assert(ret_val==SUCCESS);
	if(ret_val!=SUCCESS)
	{
		p_action->_error_code = ret_val;
		p_action->_state = LPS_FAILED;
	}
	return ret_val;
}




/*--------------------------------------------------------------------------*/
/*                Internal functions				        */
/*--------------------------------------------------------------------------*/
_int32 lx_start_dispatch(void)
{
	_int32 ret_val = SUCCESS;
	
	if(g_lx_mgr._timer_id!=0) return SUCCESS;

	ret_val = em_start_timer(lx_dispatch_timeout, NOTICE_INFINITE, LX_DISPATCH_TIMEOUT, 0, NULL, &g_lx_mgr._timer_id);
	return ret_val;
}
_int32 lx_stop_dispatch(void)
{
	_int32 ret_val = SUCCESS;
	
	if(g_lx_mgr._timer_id!=0) 
	{
		em_cancel_timer(g_lx_mgr._timer_id);

		g_lx_mgr._timer_id = 0;
	}

	return ret_val;
}

_int32 lx_dispatch_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid)
{	
	if(errcode == MSG_CANCELLED)
	{
		g_lx_mgr._timer_id = 0;
		return SUCCESS;
	}

	lx_handle_action_list();

	return SUCCESS;
}


_int32 lx_handle_action_list(void)
{
	_int32 ret_val = SUCCESS;
	pLIST_NODE cur_node = NULL ,nxt_node = NULL ;
	LX_PT * p_action = NULL;
	
     // EM_LOG_DEBUG("lx_handle_action_list"); 
	if(list_size(&g_lx_mgr._action_list)==0)	
	{
		return SUCCESS;
	}

	cur_node = LIST_BEGIN(g_lx_mgr._action_list);
	while(cur_node!=LIST_END(g_lx_mgr._action_list))
	{
		nxt_node = LIST_NEXT(cur_node);
		
		p_action = (LX_PT * )LIST_VALUE(cur_node);
		/*
		if(p_action->_type == LPT_SCREEN_SHOT)
		{
			//lx_update_download_action(p_action);
		}
		else
		*/
		if(p_action->_state == LPS_SUCCESS||p_action->_state == LPS_FAILED)
		{
			list_erase(&g_lx_mgr._action_list, cur_node);
			lx_action_finished(p_action);
		}

		cur_node = nxt_node;
	}

	if(list_size(&g_lx_mgr._action_list)==0)	
	{
		lx_stop_dispatch();
	}
	return ret_val;
}

_int32 lx_action_finished(LX_PT *  p_action)
{
	eti_http_close(p_action->_action_id);	
	//p_action->_action_id = 0;
	
	switch(p_action->_type)
	{
		case LPT_TASK_LS:
			lx_get_task_list_resp(p_action);
			break;
		case LPT_BT_LS:
			lx_get_bt_task_file_list_resp(p_action);
			break;
		case LPT_SCREEN_SHOT:
			lx_get_screenshot_resp(p_action);
			break;
		case LPT_VOD_URL:
			lx_get_vod_url_resp(p_action);
			break;
		case LPT_DL_PIC:
			lx_dowanload_pic_resp(p_action);
			break;
		case LPT_CRE_TASK:
			lx_create_task_resp(p_action);
			break;
		case LPT_DEL_TASK:
			lx_delete_task_resp(p_action);
			break;
		case LPT_DEL_TASKS:
			lx_delete_tasks_resp(p_action);
			break;
		case LPT_CRE_BT_TASK:
			lx_create_bt_task_resp(p_action);
			break;
		case LPT_DELAY_TASK:
			lx_delay_task_resp(p_action);
			break;
		case LPT_MINIQUERY_TASK:
			lx_miniquery_task_resp(p_action);
			break;
		case LPT_GET_DL_URL:
			lx_get_downloadable_url_from_webpage_resp(p_action);
			break;
		case LPT_GET_REGEX:
			lx_get_detect_regex_resp(p_action);
			break;
		case LPT_GET_DETECT_STRING:
			lx_get_detect_string_resp(p_action);
			break;
        case LPT_GET_USER_INFO:
			lx_get_user_info_resp(p_action);
        	break;
		case LPT_GET_OD_OR_DEL_LS:
			lx_get_overdue_or_deleted_task_list_resp(p_action);
			break;
		case LPT_QUERY_BT_TASK_INFO:
			lx_query_bt_task_info_resp(p_action);
			break;
		case LPT_BATCH_QUERY_TASK_INFO:
			lx_batch_query_task_info_resp(p_action);
			break;
		case LPT_BINARY_TASK_LS:
			lx_get_task_list_binary_resp(p_action);
			break;
		case LPT_TASK_LS_NEW:
			lx_get_task_list_new_resp(p_action);
			break;
		case LPT_TAKE_OFF_FLUX:
			lx_take_off_flux_from_high_speed_resp(p_action);
			break;
		default:
			sd_assert(FALSE);
			break;
	}
	return SUCCESS;
}

//////////////////////////////////////////////////////////
_int32 lx_cancel_action(LX_PT* p_action)
{
 	_int32 ret_val = SUCCESS;

	EM_LOG_DEBUG( "lx_cancel_action" );

	ret_val = eti_http_close(p_action->_action_id);
	
	lx_remove_action_from_list(p_action);
	
	if(p_action->_file_id!=0)
	{
		em_close_ex(p_action->_file_id);
		p_action->_file_id = 0;
	}

	/* ɾ�����Ʒ */
	sd_delete_file(p_action->_file_path);
	
	SAFE_DELETE(p_action);
	
	return SUCCESS;
}

_int32 lx_clear_action_list(void)
{
	_int32 ret_val = SUCCESS;
	pLIST_NODE cur_node = NULL ,nxt_node = NULL ;
	LX_PT * p_action = NULL;
	
     // EM_LOG_DEBUG("lx_handle_action_list"); 
	if(list_size(&g_lx_mgr._action_list)==0)	
	{
		return SUCCESS;
	}

	cur_node = LIST_BEGIN(g_lx_mgr._action_list);
	while(cur_node!=LIST_END(g_lx_mgr._action_list))
	{
		nxt_node = LIST_NEXT(cur_node);
		
		p_action = (LX_PT * )LIST_VALUE(cur_node);

		if(p_action->_type == LPT_TASK_LS)
		{
			ret_val = lx_cancel_get_task_list(p_action);
		}
		else
		if(p_action->_type == LPT_SCREEN_SHOT)
		{
			ret_val = lx_cancel_screenshot(p_action);
		}
		else
		if(p_action->_type == LPT_DL_PIC)
		{
			ret_val = lx_cancel_download_pic(p_action,TRUE);
		}
		else
		if(p_action->_type == LPT_GET_DL_URL)
		{
			ret_val = lx_cancel_get_downloadable_url_from_webpage(p_action);
		}
		else
		{
			ret_val = lx_cancel_action(p_action);
		}

		cur_node = nxt_node;
	}

	list_clear(&g_lx_mgr._action_list);

	lx_stop_dispatch();

	return SUCCESS;
}
_int32 lx_clear_action_list_except_sniff(void)
{
	_int32 ret_val = SUCCESS;
	pLIST_NODE cur_node = NULL ,nxt_node = NULL ;
	LX_PT * p_action = NULL;
	
     // EM_LOG_DEBUG("lx_handle_action_list"); 
	if(list_size(&g_lx_mgr._action_list)==0)	
	{
		return SUCCESS;
	}

	cur_node = LIST_BEGIN(g_lx_mgr._action_list);
	while(cur_node!=LIST_END(g_lx_mgr._action_list))
	{
		nxt_node = LIST_NEXT(cur_node);
		
		p_action = (LX_PT * )LIST_VALUE(cur_node);

		if(p_action->_type == LPT_TASK_LS)
		{
			ret_val = lx_cancel_get_task_list(p_action);
		}
		else
		if(p_action->_type == LPT_SCREEN_SHOT)
		{
			ret_val = lx_cancel_screenshot(p_action);
		}
		else
		if(p_action->_type == LPT_DL_PIC)
		{
			ret_val = lx_cancel_download_pic(p_action,TRUE);
		}
		else
		if(p_action->_type == LPT_GET_DL_URL)
		{
			/* ��ȡ����̽���� */
			//ret_val = lx_cancel_get_downloadable_url_from_webpage(p_action);
		}
		else
		{
			ret_val = lx_cancel_action(p_action);
		}

		cur_node = nxt_node;
	}

	if(list_size(&g_lx_mgr._action_list)==0)	
	{
		lx_stop_dispatch();
	}

	return SUCCESS;
}

/* ����list xml�ļ��Ĵ���ļ��� */
_int32  lx_make_xml_file_store_dir( void)
{
	//_int32 ret_val = SUCCESS;
	char * system_path = em_get_system_path();
	char xml_file_store_path[MAX_FULL_PATH_BUFFER_LEN] = {0};
	
	sd_snprintf(xml_file_store_path, MAX_FULL_PATH_BUFFER_LEN-1, "%s/%s",system_path,LX_LIST_XML_FILE_PATH);
	if(sd_dir_exist(xml_file_store_path))
	{
		sd_recursive_rmdir(xml_file_store_path);
	}
	
	sd_mkdir(xml_file_store_path);

	return SUCCESS;
}
_int32  lx_get_xml_file_store_path( char * xml_file_path_buffer)
{
	_int32 ret_val = SUCCESS;
	static _u32 count = 0;
	char * system_path = em_get_system_path();
	_u32 cur_time_stamp = 0;

	sd_time_ms(&cur_time_stamp);
	sd_snprintf(xml_file_path_buffer, MAX_FULL_PATH_BUFFER_LEN-1, "%s/%s/%u_%u.xml",system_path,LX_LIST_XML_FILE_PATH,count++,cur_time_stamp);
	
	return ret_val;
}

/////////////////////////////////////////////////////////////////////////////////////////////
_int32 lx_file_id_comp(void *E1, void *E2)
{
	_u32 id1 = (_u32)E1,id2 = (_u32)E2;
	
	if(id1==id2)
		return 0;
	else
	if(id1>id2)
		return 1;
	else
		return -1;
}
_int32 lx_add_file_to_map(MAP * p_map,LX_FILE_INFO * p_file)
{
	_int32 ret_val = SUCCESS;
	PAIR info_map_node;
	
	info_map_node._key = (void*)&p_file->_file_id;
	info_map_node._value = (void*)p_file;

	ret_val = em_map_insert_node( p_map, &info_map_node );

      EM_LOG_DEBUG("lx_add_file_to_map:ret_val=%d,file_id=%llu,state=%d",ret_val,p_file->_file_id,p_file->_state); 
	return ret_val;
}
LX_FILE_INFO * lx_get_file_from_map(MAP * p_map,_u64 file_id)
{
	_int32 ret_val = SUCCESS;
	LX_FILE_INFO * p_ile_in_map=NULL;

	ret_val=em_map_find_node(p_map, (void *)&file_id, (void **)&p_ile_in_map);
	sd_assert(ret_val==SUCCESS);
	//sd_assert(p_task!=NULL);
	return p_ile_in_map;
}
_int32 lx_get_file_array_from_map(MAP * p_map,LX_FILE_TYPE file_type,_int32 file_status,_u32 offset,_u32 file_num,LX_FILE_INFO ** pp_file_array)
{
	_int32 ret_val = SUCCESS;
      MAP_ITERATOR  cur_node = NULL; 
	LX_FILE_INFO *p_file_array = NULL,*p_info = NULL,*p_info_in_map = NULL;

	//sd_assert(file_type==LXF_UNKNOWN);
	//sd_assert(file_status==0);
	sd_assert(offset==0);
	sd_assert(file_num!=0);
	
	ret_val = sd_malloc(file_num*sizeof(LX_FILE_INFO), (void**)&p_file_array);
	CHECK_VALUE(ret_val);
	sd_memset(p_file_array,0x00,file_num*sizeof(LX_FILE_INFO));
	p_info = p_file_array;

#if 0
	LIST_NODE *cur_list_node = NULL;
	for(cur_list_node = LIST_BEGIN(g_lx_mgr._task_list);cur_list_node!=NULL && cur_list_node != LIST_END(g_lx_mgr._task_list);cur_list_node = LIST_NEXT(cur_list_node))
	{
		p_info_in_map = (LX_TASK_INFO *)LIST_VALUE(cur_list_node);
#else
	for(cur_node = EM_MAP_BEGIN(*p_map);cur_node != EM_MAP_END(*p_map);cur_node = EM_MAP_NEXT(*p_map,cur_node))
	{
	 	p_info_in_map = (LX_FILE_INFO *)EM_MAP_VALUE(cur_node);
#endif
		sd_memcpy(p_info, p_info_in_map, sizeof(LX_FILE_INFO));
		p_info++;
		file_num--;
		if(file_num==0) break;
      }

	*pp_file_array = p_file_array;
	return SUCCESS;  
}

_int32 lx_remove_file_from_map(MAP * p_map,_u64 file_id)
{
	_int32 ret_val = SUCCESS;
	return ret_val;
}
_int32 lx_clear_file_map(MAP * p_map)
{
      MAP_ITERATOR  cur_node = NULL; 
	 LX_FILE_INFO * p_file_in_map = NULL;

	EM_LOG_DEBUG("lx_clear_file_map:%u",em_map_size(p_map)); 
	  
      cur_node = EM_MAP_BEGIN(*p_map);
      while(cur_node != EM_MAP_END(*p_map))
      {
             p_file_in_map = (LX_FILE_INFO *)EM_MAP_VALUE(cur_node);

		SAFE_DELETE(p_file_in_map);

		em_map_erase_iterator(p_map, cur_node);
	      cur_node = EM_MAP_BEGIN(*p_map);
      }
	  
	return SUCCESS;
}

///// ��ȡbt���������ļ�id �б�,*buffer_len�ĵ�λ��sizeof(_u64)
_int32 lx_get_bt_sub_file_ids(_u64 task_id,_int32 state,_u64 * id_array_buffer,_u32 *buffer_len)
{
 	//_int32 ret_val = SUCCESS;
      MAP_ITERATOR  cur_node = NULL; 
	LX_FILE_INFO *p_info_in_map = NULL;
	_u32 task_num = 0;
	_u64 * p_id = id_array_buffer;
	LX_TASK_INFO_EX * p_task_in_map = (LX_TASK_INFO_EX *) lx_get_task_from_map( task_id);

	sd_assert(buffer_len!=NULL);
	if(id_array_buffer!=NULL)
	{
		sd_assert(*buffer_len>0);
	}

	if(p_task_in_map==NULL)
	{
		*buffer_len = 0;
		CHECK_VALUE(INVALID_TASK_ID);
	}
	if(p_task_in_map->_type!=LXT_BT_ALL)
	{
		*buffer_len = 0;
		CHECK_VALUE(INVALID_TASK_TYPE);
	}
	
    for(cur_node = EM_MAP_BEGIN(p_task_in_map->_bt_sub_files);cur_node != EM_MAP_END(p_task_in_map->_bt_sub_files);cur_node = EM_MAP_NEXT(p_task_in_map->_bt_sub_files,cur_node))
    {
		if((id_array_buffer!=NULL)&&(*buffer_len<=task_num))
		{
			break;
		}
        p_info_in_map = (LX_FILE_INFO *)EM_MAP_VALUE(cur_node);
		/*if((state==0)||											
		((state==1)&&(p_info_in_map->_state<LXS_SUCCESS))||		
		((state==2)&&(p_info_in_map->_state==LXS_SUCCESS)))	
		*/
		if( (state==0) ||	( (state & p_info_in_map->_state) > 0 ) )
		{
			task_num++;
			if(id_array_buffer!=NULL)
			{
				*p_id = p_info_in_map->_file_id;
				EM_LOG_DEBUG("lx_get_bt_sub_file_ids:get a file:%llu",*p_id); 
				p_id++;
			}
		}
    }
	if(*buffer_len!=task_num)
	{
	    *buffer_len = task_num;
	}
	return SUCCESS;  
}

/////��ȡ����BT������ĳ�����ļ�����Ϣ
_int32 lx_get_bt_sub_file_info(_u64 task_id, _u64 file_id, LX_FILE_INFO *p_file_info)
{
	LX_FILE_INFO * p_info_in_map = NULL;
	LX_TASK_INFO_EX* p_task_in_map = (LX_TASK_INFO_EX *) lx_get_task_from_map( task_id);
	if(p_task_in_map==NULL)
	{
		CHECK_VALUE(INVALID_TASK_ID);
	}
	
	if(p_task_in_map->_type!=LXT_BT_ALL)
	{
		CHECK_VALUE(INVALID_TASK_TYPE);
	}
	
	p_info_in_map = lx_get_file_from_map(&p_task_in_map->_bt_sub_files, file_id);	
	if(p_info_in_map==NULL)
	{
		CHECK_VALUE(EM_INVALID_FILE_INDEX);
	}
	
	sd_memcpy(p_file_info, p_info_in_map, sizeof(LX_FILE_INFO));
	if(sd_strlen(p_file_info->_name)==0)
	{
		sd_strncpy(p_file_info->_name,"Unknown file name",MAX_FILE_NAME_BUFFER_LEN);
	}
	return SUCCESS;
}


_int32 lx_malloc_ex_task(LX_TASK_INFO_EX ** pp_task)
{
	_int32 ret_val = SUCCESS;
	ret_val = sd_malloc(sizeof(LX_TASK_INFO_EX), (void**)pp_task);
	CHECK_VALUE(ret_val);
	sd_memset(*pp_task,0x00,sizeof(LX_TASK_INFO_EX));
	map_init(&((*pp_task)->_bt_sub_files),lx_task_id_comp);
	return SUCCESS;
}
_int32 lx_release_ex_task(LX_TASK_INFO_EX * p_task)
{
	if( 0 != em_map_size(&p_task->_bt_sub_files))
		lx_clear_file_map(&p_task->_bt_sub_files);
	
	SAFE_DELETE(p_task);
	return SUCCESS;
}

_int32 lx_task_id_comp(void *E1, void *E2)
{
	_u64 id1 = *((_u64*)E1),id2 = *((_u64*)E2);
	
	if(id1==id2)
		return 0;
	else
	if(id1>id2)
		return 1;
	else
		return -1;
}
_int32 lx_add_task_to_map(LX_TASK_INFO_EX * p_task)
{
	_int32 ret_val = SUCCESS;
	PAIR info_map_node;
	
	info_map_node._key = (void*)(&(p_task->_task_id));
	info_map_node._value = (void*)p_task;

	ret_val = em_map_insert_node( &g_lx_mgr._task_map, &info_map_node );
	if(SUCCESS == ret_val)
	{
		ret_val = lx_add_task_to_list(&g_lx_mgr._task_list,p_task);
		sd_assert(ret_val==SUCCESS);
		sd_assert(list_size(&g_lx_mgr._task_list) == map_size(&g_lx_mgr._task_map));
	}
	
     EM_LOG_DEBUG("lx_add_task_to_map:ret_val=%d,task_id=%llu,type=%d,state=%d",ret_val,p_task->_task_id,p_task->_type,p_task->_state); 

	return ret_val;
}

_int32 lx_add_new_create_task_to_map(LX_TASK_INFO_EX * p_task)
{
	_int32 ret_val = SUCCESS;
	PAIR info_map_node;
	
	info_map_node._key = (void*)(&(p_task->_task_id));
	info_map_node._value = (void*)p_task;

	ret_val = em_map_insert_node( &g_lx_mgr._task_map, &info_map_node );
	if(SUCCESS == ret_val)
	{
		ret_val = lx_add_new_create_task_to_list(&g_lx_mgr._task_list,p_task);
		sd_assert(ret_val==SUCCESS);
		sd_assert(list_size(&g_lx_mgr._task_list) == map_size(&g_lx_mgr._task_map));
	}
	
     EM_LOG_DEBUG("lx_add_new_create_task_to_map:ret_val=%d,task_id=%llu,type=%d,state=%d",ret_val,p_task->_task_id,p_task->_type,p_task->_state); 

	return ret_val;
}

LX_TASK_INFO_EX * lx_get_task_from_map(_u64 task_id)
{
	_int32 ret_val = SUCCESS;
	LX_TASK_INFO_EX * p_task=NULL;

	ret_val=em_map_find_node(&g_lx_mgr._task_map, (void *)(&task_id), (void **)&p_task);
	sd_assert(ret_val==SUCCESS);
	//sd_assert(p_task!=NULL);
	return p_task;
}

_int32 lx_get_task_array_from_map(LX_FILE_TYPE file_type,_int32 file_status,_u32 offset,_u32 task_num,LX_TASK_INFO ** pp_task_array)
{
	_int32 ret_val = SUCCESS;
      MAP_ITERATOR  cur_node = NULL; 
	LX_TASK_INFO *p_task_array = NULL,*p_info = NULL,*p_info_in_map = NULL;

	//sd_assert(file_type==LXF_UNKNOWN);
	//sd_assert(file_status==0);
	//sd_assert(offset==0);
	sd_assert(task_num!=0);
	
	if(task_num>em_map_size(&g_lx_mgr._task_map))
	{
		return LXE_WRONG_RESPONE;
	}
		
	ret_val = sd_malloc(task_num*sizeof(LX_TASK_INFO), (void**)&p_task_array);
	CHECK_VALUE(ret_val);
	sd_memset(p_task_array,0x00,task_num*sizeof(LX_TASK_INFO));
	p_info = p_task_array;

#ifdef OUT_OF_DATE
	LIST_NODE *cur_list_node = NULL;
	for(cur_list_node = LIST_BEGIN(g_lx_mgr._task_list);cur_list_node!=NULL && cur_list_node != LIST_END(g_lx_mgr._task_list);cur_list_node = LIST_NEXT(cur_list_node))
	{
		p_info_in_map = (LX_TASK_INFO *)LIST_VALUE(cur_list_node);
#else
	for(cur_node = EM_MAP_BEGIN(g_lx_mgr._task_map);cur_node != EM_MAP_END(g_lx_mgr._task_map);cur_node = EM_MAP_NEXT(g_lx_mgr._task_map,cur_node))
	{
		p_info_in_map = (LX_TASK_INFO *)EM_MAP_VALUE(cur_node);
#endif
		sd_memcpy(p_info, p_info_in_map, sizeof(LX_TASK_INFO));
		p_info++;
		task_num--;
		if(task_num==0) break;
      }
	  
	*pp_task_array = p_task_array;
	return SUCCESS;  
}
_int32 lx_remove_task_from_map(_u64 task_id)
{
	_int32 ret_val = SUCCESS;
	MAP_ITERATOR map_node = NULL,sub_map_node = NULL;
	LX_TASK_INFO_EX *p_task_in_map = NULL;
	LX_FILE_INFO * p_file_in_map = NULL;
	
	ret_val=em_map_find_iterator(&g_lx_mgr._task_map, &task_id,&map_node);
	if(map_node!=NULL && map_node != EM_MAP_END(g_lx_mgr._task_map))
	{
        		p_task_in_map = (LX_TASK_INFO_EX *)EM_MAP_VALUE(map_node);
		ret_val = em_map_erase_iterator(&g_lx_mgr._task_map, map_node);

		//remove the task from the list,need the task info
		if(SUCCESS == ret_val)			
		{
			ret_val = lx_remove_task_from_list(&g_lx_mgr._task_list,&task_id);
			sd_assert(ret_val==SUCCESS);
			sd_assert(list_size(&g_lx_mgr._task_list) == map_size(&g_lx_mgr._task_map));
		}

		//now the task can be release 
		lx_release_ex_task(p_task_in_map);		
	}
	else
	{
		/*  ���Ǵ�����,�п�����bt���ļ�*/
	      for(map_node = EM_MAP_BEGIN(g_lx_mgr._task_map);map_node != EM_MAP_END(g_lx_mgr._task_map);map_node = EM_MAP_NEXT(g_lx_mgr._task_map,map_node))
	      {
	             p_task_in_map = (LX_TASK_INFO_EX *)EM_MAP_VALUE(map_node);

			if(p_task_in_map->_type==LXT_BT_ALL)
			{
				ret_val=em_map_find_iterator(&p_task_in_map->_bt_sub_files, &task_id,&sub_map_node);
				if(sub_map_node!=NULL&&sub_map_node != EM_MAP_END(p_task_in_map->_bt_sub_files))
				{
			             p_file_in_map = (LX_FILE_INFO *)EM_MAP_VALUE(sub_map_node);

					SAFE_DELETE(p_file_in_map);

					em_map_erase_iterator(&p_task_in_map->_bt_sub_files, sub_map_node);

					break;
				}
			}
	      }
	}
	return SUCCESS;
}
_int32 lx_clear_task_map(void)
{
	MAP_ITERATOR  cur_node = NULL; 
	LX_TASK_INFO_EX * p_task_in_map = NULL;

	EM_LOG_DEBUG("lx_clear_task_map: map_size = %u",em_map_size(&g_lx_mgr._task_map)); 

	cur_node = EM_MAP_BEGIN(g_lx_mgr._task_map);
	while(cur_node != EM_MAP_END(g_lx_mgr._task_map))
	{
		p_task_in_map = (LX_TASK_INFO_EX *)EM_MAP_VALUE(cur_node);

		lx_release_ex_task(p_task_in_map);

		em_map_erase_iterator(&g_lx_mgr._task_map, cur_node);
		cur_node = EM_MAP_BEGIN(g_lx_mgr._task_map);
	}
	lx_stop_update_task_dispatch();

	lx_clear_task_list_not_free();

	return SUCCESS;
}
_u32 lx_num_of_task_in_map(void)
{
	return em_map_size(&g_lx_mgr._task_map);
}

_u32 lx_num_of_task_in_list(void)
{
	return em_list_size(&g_lx_mgr._task_list);
}
_int32 lx_get_all_task_id(_u64 * id_array_buffer, _u64 last_task_id)
{
	LX_TASK_INFO_EX * p_task_in_map = NULL;
	_u64 *all_task_id = id_array_buffer;
	BOOL find_flag = FALSE;
#if 0
	MAP_ITERATOR  map_node = NULL; 
	for(map_node = EM_MAP_BEGIN(g_lx_mgr._task_map);map_node != EM_MAP_END(g_lx_mgr._task_map);map_node = EM_MAP_NEXT(g_lx_mgr._task_map,map_node))
	{
	 	p_task_in_map = (LX_TASK_INFO_EX *)EM_MAP_VALUE(map_node);
		if(p_task_in_map != NULL)
		{
			*all_task_id = p_task_in_map->_task_id;
			all_task_id++;
		}
	}
#else
	LIST *id_list = &g_lx_mgr._task_list;
	pLIST_NODE cur_node = NULL;
	cur_node = LIST_BEGIN(*id_list);
	if(last_task_id == 0)
		find_flag = TRUE;
	while(cur_node != LIST_END(*id_list))
	{
		p_task_in_map = (LX_TASK_INFO_EX *)(LIST_VALUE(cur_node));
		sd_assert(p_task_in_map != NULL);
		if(p_task_in_map != NULL)
		{
			if(find_flag)
			{
				*all_task_id = p_task_in_map->_task_id;
				all_task_id++;
			}
			if(p_task_in_map->_task_id == last_task_id)
				find_flag = TRUE;
		}
		cur_node = LIST_NEXT(cur_node);
	}
#endif
	return SUCCESS;
}

// ��ȡ�б����õ���map����
_int32 lx_action_offset_comp(void *E1, void *E2)
{
	_u32 id1 = (_u32)E1,id2 = (_u32)E2;
	
	if(id1==id2)
		return 0;
	else
	if(id1>id2)
		return 1;
	else
		return -1;
}

_u32 lx_num_of_task_list_action_in_map_new(void)
{
	return em_map_size(&g_lx_mgr._task_list_action_map);
}

_int32 lx_add_task_list_action_to_map_new(LX_PT_TASK_LS_NEW* p_action)
{
	_int32 ret_val = SUCCESS;
	PAIR info_map_node;
	
	info_map_node._key = (void*)(p_action->_req._task_list._offset);
	info_map_node._value = (void*)p_action;

	ret_val = em_map_insert_node( &g_lx_mgr._task_list_action_map, &info_map_node );

    EM_LOG_DEBUG("lx_add_task_list_action_to_map_new:ret_val=%d,_offset=%d",ret_val,p_action->_req._task_list._offset); 

	return ret_val;
}

LX_PT_TASK_LS_NEW * lx_get_task_list_action_from_map_new(_u32 offset)
{
	_int32 ret_val = SUCCESS;
	LX_PT_TASK_LS_NEW * p_action = NULL;
	
	ret_val = em_map_find_node(&g_lx_mgr._task_list_action_map, (void *)offset, (void **)&p_action);
	if(ret_val != SUCCESS)
		return NULL;
	return p_action;
}

_int32 lx_remove_task_list_action_from_map_new(_u32 offset)
{
	_int32 ret_val = SUCCESS;
	MAP_ITERATOR map_node = NULL;
	LX_PT_TASK_LS_NEW *p_action = NULL;
	MAP * p_map = &g_lx_mgr._task_list_action_map;
	
	ret_val = em_map_find_iterator(p_map, (void *)offset,&map_node);
	if(ret_val != SUCCESS)
		return ret_val;
	if(map_node != NULL)
	{
        p_action = (LX_PT_TASK_LS_NEW *)EM_MAP_VALUE(map_node);
		EM_LOG_DEBUG("lx_remove_task_list_action_from_map_new:offset=%d",p_action->_req._task_list._offset); 
		ret_val = em_map_erase_iterator(p_map, map_node);
		if(p_action->_action._file_id != 0)
		{
			sd_close_ex(p_action->_action._file_id);
			p_action->_action._file_id = 0;
		}
		sd_delete_file(p_action->_action._file_path);
		SAFE_DELETE(p_action);	
	}
	return ret_val;
}

_int32 lx_clear_task_list_action_map_new(void)
{
	MAP_ITERATOR  cur_node = NULL; 
	LX_PT_TASK_LS_NEW * p_action = NULL;
	MAP * p_map = &g_lx_mgr._task_list_action_map;
	
	EM_LOG_DEBUG("lx_clear_task_list_action_map_new: map_size = %u",em_map_size(p_map)); 

	cur_node = EM_MAP_BEGIN(*p_map);
	while(cur_node != EM_MAP_END(*p_map))
	{
		p_action = (LX_PT_TASK_LS_NEW *)EM_MAP_VALUE(cur_node);

		if(p_action->_action._file_id != 0)
		{
			sd_close_ex(p_action->_action._file_id);
			p_action->_action._file_id = 0;
		}
		sd_delete_file(p_action->_action._file_path);
		SAFE_DELETE(p_action);	

		em_map_erase_iterator(p_map, cur_node);
		
		cur_node = EM_MAP_BEGIN(*p_map);
	}
	return SUCCESS;
}


_u32 lx_num_of_task_list_action_in_map(void)
{
	return em_map_size(&g_lx_mgr._task_list_action_map);
}

_int32 lx_add_task_list_action_to_map(LX_PT_TASK_LS * p_action)
{
	_int32 ret_val = SUCCESS;
	PAIR info_map_node;
	
	info_map_node._key = (void*)(p_action->_req._offset);
	info_map_node._value = (void*)p_action;

	ret_val = em_map_insert_node( &g_lx_mgr._task_list_action_map, &info_map_node );

    EM_LOG_DEBUG("lx_add_task_list_action_to_map:ret_val=%d,_offset=%d",ret_val,p_action->_req._offset); 

	return ret_val;
}

LX_PT_TASK_LS * lx_get_task_list_action_from_map(_u32 offset)
{
	_int32 ret_val = SUCCESS;
	LX_PT_TASK_LS * p_action = NULL;
	
	ret_val = em_map_find_node(&g_lx_mgr._task_list_action_map, (void *)offset, (void **)&p_action);
	if(ret_val != SUCCESS)
		return NULL;
	return p_action;
}

_int32 lx_remove_task_list_action_from_map(_u32 offset)
{
	_int32 ret_val = SUCCESS;
	MAP_ITERATOR map_node = NULL;
	LX_PT_TASK_LS *p_action = NULL;
	MAP * p_map = &g_lx_mgr._task_list_action_map;
	
	ret_val = em_map_find_iterator(p_map, (void *)offset,&map_node);
	if(ret_val != SUCCESS)
		return ret_val;
	if(map_node != NULL)
	{
        p_action = (LX_PT_TASK_LS *)EM_MAP_VALUE(map_node);
		ret_val = em_map_erase_iterator(p_map, map_node);

		if(p_action->_action._file_id != 0)
		{
			sd_close_ex(p_action->_action._file_id);
			p_action->_action._file_id = 0;
		}
		sd_delete_file(p_action->_action._file_path);
		SAFE_DELETE(p_action);	
	}
	return ret_val;
}

_int32 lx_clear_task_list_action_map(void)
{
	MAP_ITERATOR  cur_node = NULL; 
	LX_PT_TASK_LS * p_action = NULL;
	MAP * p_map = &g_lx_mgr._task_list_action_map;
	
	EM_LOG_DEBUG("lx_clear_task_list_action_map: map_size = %u",em_map_size(p_map)); 

	cur_node = EM_MAP_BEGIN(*p_map);
	while(cur_node != EM_MAP_END(*p_map))
	{
		p_action = (LX_PT_TASK_LS *)EM_MAP_VALUE(cur_node);

		if(p_action->_action._file_id != 0)
		{
			sd_close_ex(p_action->_action._file_id);
			p_action->_action._file_id = 0;
		}
		sd_delete_file(p_action->_action._file_path);
		SAFE_DELETE(p_action);	

		em_map_erase_iterator(p_map, cur_node);
		
		cur_node = EM_MAP_BEGIN(*p_map);
	}
	return SUCCESS;
}


_int32 lx_get_task_ids_by_state( _int32 state,_u64 * id_array_buffer,_u32 *buffer_len)
{
 	_int32 ret_val = SUCCESS;
#ifdef OUT_OF_DATE
	LIST *id_list = &g_lx_mgr._task_list;
	pLIST_NODE cur_node = NULL;
	_u64 *cur_task_id = NULL;
	_u32 cur_num = 0;
	LX_TASK_INFO_EX *p_task_in_map = NULL;
	
	EM_LOG_DEBUG("lx_get_task_ids_by_state"); 

	if( list_size(id_list) == 0 )
	{
		*buffer_len = 0;
		return SUCCESS;
	}

	EM_LOG_DEBUG("lx_get_task_ids_by_state list_size(path_list)= %d", list_size(id_list));
	cur_task_id = id_array_buffer;
	cur_node = LIST_BEGIN(*id_list);
	while(cur_node != LIST_END(*id_list))
	{
		if((id_array_buffer!=NULL)&&(*buffer_len<=cur_num))
		{
			break;
		}
		
		p_task_in_map = (LX_TASK_INFO_EX *)(LIST_VALUE(cur_node));
		if( NULL == p_task_in_map )
		{
			cur_node = LIST_NEXT(cur_node);
			continue;
		}
	
		if( (state==0) ||	( (state & p_task_in_map->_state) > 0 ) ) 	  
		{
			cur_num++;
			if(id_array_buffer!=NULL)
			{
				*cur_task_id = p_task_in_map->_task_id;
				cur_task_id++;
			}
		}
		cur_node = LIST_NEXT(cur_node);
	}
	if(*buffer_len != cur_num)
		*buffer_len = cur_num;
#else
	MAP_ITERATOR  cur_node = NULL; 
	LX_TASK_INFO *p_info_in_map = NULL;
	_u32 task_num = 0;
	_u64 * p_task_id = id_array_buffer;

	EM_LOG_DEBUG("lx_get_task_ids_by_state:%d",state); 
	sd_assert(buffer_len!=NULL);
	sd_assert(state==0||state==1||state==2||state==3);
	if(id_array_buffer!=NULL)
	{
		sd_assert(*buffer_len>0);
	}
     for(cur_node = EM_MAP_BEGIN(g_lx_mgr._task_map);cur_node != EM_MAP_END(g_lx_mgr._task_map);cur_node = EM_MAP_NEXT(g_lx_mgr._task_map,cur_node))
      {
		if((id_array_buffer!=NULL)&&(*buffer_len<=task_num))
		{
			break;
		}
		
             p_info_in_map = (LX_TASK_INFO *)EM_MAP_VALUE(cur_node);
		if( (state==0)||											//  ȫ��
		((state==1)&&(p_info_in_map->_state < LXS_SUCCESS)) ||		//  ����������
		((state==2)&&(p_info_in_map->_state == LXS_SUCCESS))	 ||	    // ���
		((state==3)&&(p_info_in_map->_state == LXS_OVERDUE)) )
		{
			task_num++;
			if(id_array_buffer!=NULL)
			{
				*p_task_id = p_info_in_map->_task_id;
				EM_LOG_DEBUG("lx_get_task_ids_by_state:get a task:%llu",*p_task_id); 
				p_task_id++;
			}
		}

      }

	 if(*buffer_len!=task_num)
	 {
	 	*buffer_len = task_num;
	 }
	 
#endif
	return ret_val;  
}

/* ��ȡ����������Ϣ */
_int32 lx_get_task_info(_u64 task_id,LX_TASK_INFO * p_info)
{
    LX_TASK_INFO_EX * p_task_in_map = (LX_TASK_INFO_EX *) lx_get_task_from_map( task_id);
	EM_LOG_DEBUG("lx_get_task_info:%llu,p_task_in_map=0x%X",task_id,p_task_in_map); 
	if(p_task_in_map==NULL)
	{
		MAP_ITERATOR map_node = NULL;
		LX_FILE_INFO * p_file_in_map = NULL;
		/*  ���Ǵ�����,�п�����bt���ļ�*/
	    for(map_node = EM_MAP_BEGIN(g_lx_mgr._task_map);map_node != EM_MAP_END(g_lx_mgr._task_map);map_node = EM_MAP_NEXT(g_lx_mgr._task_map,map_node))
	    {
	    	p_task_in_map = (LX_TASK_INFO_EX *)EM_MAP_VALUE(map_node);

			if(p_task_in_map->_type==LXT_BT_ALL)
			{
				p_file_in_map = lx_get_file_from_map(&p_task_in_map->_bt_sub_files, task_id);
				if(p_file_in_map!=NULL)
				{
					/* ���task info */
					p_info->_task_id = p_file_in_map->_file_id;
					p_info->_state = p_file_in_map->_state;
					p_info->_size = p_file_in_map->_size;
					p_info->_progress = p_file_in_map->_progress;
					p_info->_vod = p_file_in_map->_vod;

					p_info->_type = LXT_BT_FILE;

					sd_memcpy(p_info->_name, p_file_in_map->_name, MAX_FILE_NAME_BUFFER_LEN-1);
					sd_memcpy(p_info->_file_suffix, p_file_in_map->_file_suffix, 15);
					sd_memcpy(p_info->_cid, p_file_in_map->_cid, CID_SIZE);
					sd_memcpy(p_info->_gcid, p_file_in_map->_gcid, CID_SIZE);
					sd_memcpy(p_info->_url, p_file_in_map->_url, MAX_URL_LEN-1);
					sd_memcpy(p_info->_cookie, p_file_in_map->_cookie, MAX_URL_LEN-1);
					
					if(sd_strlen(p_info->_name)==0)
					{
						sd_strncpy(p_info->_name,"Unknown file name",MAX_FILE_NAME_BUFFER_LEN);
					}
					
					p_info->_sub_file_num= 0;
					if(p_info->_state==LXS_SUCCESS)
					{
						p_info->_finished_file_num = 1;
					}
					else
					{
						p_info->_finished_file_num = 0;
					}
					p_info->_left_live_time = p_task_in_map->_left_live_time;
					
                    return SUCCESS;
				}
			}
		}
		return INVALID_TASK_ID;
	}
	sd_memcpy(p_info, p_task_in_map, sizeof(LX_TASK_INFO));
	
#ifdef REMOTE_CONTROL_PROJ
	/* Զ�̿�����Ŀ��ֱ�ӷ���ԭʼURL .  by  zyq @ 20121108 */
	if(p_task_in_map->_origin_url_len!=0)
	{
		sd_memcpy(p_info->_url, p_task_in_map->_origin_url, MAX_URL_LEN-1);
		p_info->_cookie[0] = '\0';
	}
#endif /* REMOTE_CONTROL_PROJ */
	
	if(p_info->_left_live_time==0)
	{
		p_info->_left_live_time = 1;
	}
	
	if(sd_strlen(p_info->_name)==0)
	{
		sd_strncpy(p_info->_name,"Unknown file name",MAX_FILE_NAME_BUFFER_LEN);
	}
					
	return SUCCESS;
}


_int32 lx_start_update_task_dispatch(void)
{
	_int32 ret_val = SUCCESS;
	
	if(g_lx_mgr._update_task_timer_id!=0) return SUCCESS;

	ret_val = em_start_timer(lx_update_task_dispatch_timeout, NOTICE_INFINITE, LX_UPDATE_TASK_DISPATCH_TIMEOUT, 0, NULL, &g_lx_mgr._update_task_timer_id);
	return ret_val;
}
_int32 lx_stop_update_task_dispatch(void)
{
	_int32 ret_val = SUCCESS;
	
	if(g_lx_mgr._update_task_timer_id!=0) 
	{
		em_cancel_timer(g_lx_mgr._update_task_timer_id);

		g_lx_mgr._update_task_timer_id = 0;
	}

	return ret_val;
}

_int32 lx_update_task_dispatch_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid)
{	
	_int32 ret_val = SUCCESS;
	static _u32 timeout_count = 0;
	if(errcode == MSG_CANCELLED)
	{
		g_lx_mgr._update_task_timer_id = 0;
		timeout_count = 0;
		return SUCCESS;
	}

	if(timeout_count++<9)
	{
		_u32 action_id = 0;
		LX_GET_TASK_LIST create_param;
		#ifndef KANKAN_PROJ
		create_param._file_type = LXF_UNKNOWN;
		#else
		create_param._file_type = LXF_VEDIO;
		#endif
		create_param._file_status = 1;
		create_param._offset = 0;
		create_param._max_task_num = LX_MAX_TASK_NUM_EACH_REQ;
		create_param._user_data = NULL;
		create_param._callback = NULL;
		/* ÿ20��ֻ�����������ص����� */
		//ret_val = lx_get_task_list(&create_param,&action_id,-1);
		//sd_assert(ret_val==SUCCESS);
	}
	else
	{
		timeout_count = 0;
		/* ÿ3���Ӹ������������б����ػ��� */
		//ret_val = lx_get_task_list(NULL,NULL,-1);
		//sd_assert(ret_val==SUCCESS);
	}
	return SUCCESS;
}

_int32 lx_get_bt_task_lists(_int32 file_status)
{
	_int32 ret_val = SUCCESS;
      MAP_ITERATOR  cur_node = NULL; 
	LX_TASK_INFO_EX *p_info_in_map = NULL;

	//if(file_status==2) return SUCCESS;

	//sd_assert(file_status==0||file_status==1);
	
      for(cur_node = EM_MAP_BEGIN(g_lx_mgr._task_map);cur_node != EM_MAP_END(g_lx_mgr._task_map);cur_node = EM_MAP_NEXT(g_lx_mgr._task_map,cur_node))
      {
             p_info_in_map = (LX_TASK_INFO_EX *)EM_MAP_VALUE(cur_node);

		if(p_info_in_map->_type == LXT_BT_ALL && p_info_in_map->_state ==LXS_RUNNING && map_size(&p_info_in_map->_bt_sub_files)!=0)
		{
			_u32 action_id = 0;
			LX_GET_BT_FILE_LIST create_param;

			//if(file_status==1&& p_info_in_map->_state !=LXS_RUNNING)
			//{
			//	continue;
			//}
			create_param._task_id = p_info_in_map->_task_id;
			#ifndef KANKAN_PROJ
			create_param._file_type = LXF_UNKNOWN;
			#else
			create_param._file_type = LXF_VEDIO;
			#endif
			create_param._file_status = file_status;
			create_param._offset = 0;
			create_param._max_file_num = LX_MAX_TASK_NUM_EACH_REQ;
			create_param._user_data = NULL;
			create_param._callback = NULL;

			ret_val = lx_get_bt_task_file_list(&create_param,&action_id,-1);
			sd_assert(ret_val==SUCCESS);
		}
      }
	return SUCCESS;  
		
}




/***************************************************************************************************************************/
/***************************************************************************************************************************/
/***************************************************************************************************************************/

_int32 lx_url_key_comp(void *E1, void *E2)
{
	_u32 id1 = (_u32)E1,id2 = (_u32)E2;
	
	if(id1==id2)
		return 0;
	else
	if(id1>id2)
		return 1;
	else
		return -1;
}
_int32 lx_add_url_to_map(const char * url,_u32 url_num,EM_DOWNLOADABLE_URL * p_dl_url)
{
	_int32 ret_val = SUCCESS;
	_u32 hash_value = 0;
	LX_DL_URL * p_url_in_map = NULL;
	PAIR info_map_node;
	
	ret_val = em_get_url_hash_value((char*)url,sd_strlen(url), &hash_value);
	CHECK_VALUE(ret_val);

	ret_val = sd_malloc(sizeof(LX_DL_URL), (void**)&p_url_in_map);
	CHECK_VALUE(ret_val);

	p_url_in_map->_url_num = url_num;
	p_url_in_map->_dl_url_array = p_dl_url;
	
	info_map_node._key = (void*)hash_value;
	info_map_node._value = (void*)p_url_in_map;

	if(gp_sniffed_urls_map==NULL)
	{
		ret_val = sd_malloc(sizeof(MAP), (void**)&gp_sniffed_urls_map);
		if(ret_val!=SUCCESS)
		{
			SAFE_DELETE(p_url_in_map);
			CHECK_VALUE(ret_val);
		}
		map_init(gp_sniffed_urls_map,lx_url_key_comp);
	}
	
	ret_val = em_map_insert_node( gp_sniffed_urls_map, &info_map_node );
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_url_in_map);
	}
      EM_LOG_DEBUG("lx_add_url_to_map:%s,num=%u,map_size=%u,ret_val=%d",url,url_num,em_map_size(gp_sniffed_urls_map),ret_val); 
	return ret_val;
}
LX_DL_URL * lx_get_url_from_map(const char * url)
{
	_int32 ret_val = SUCCESS;
	_u32 hash_value = 0;
	LX_DL_URL * p_url_in_map=NULL;

	if(gp_sniffed_urls_map==NULL) return NULL;

	ret_val = em_get_url_hash_value((char*)url,sd_strlen(url), &hash_value);
	if(ret_val!=SUCCESS)
	{
		return NULL;
	}
	
	ret_val=em_map_find_node(gp_sniffed_urls_map, (void *)hash_value, (void **)&p_url_in_map);
	sd_assert(ret_val==SUCCESS);
	//sd_assert(p_task!=NULL);
	return p_url_in_map;
}
_int32 lx_clear_url_map(void)
{
      MAP_ITERATOR  cur_node = NULL; 
	 LX_DL_URL * p_url_in_map = NULL;

	if(gp_sniffed_urls_map==NULL) return SUCCESS;
		
	EM_LOG_DEBUG("lx_clear_url_map:%u",em_map_size(gp_sniffed_urls_map)); 
	  
      cur_node = EM_MAP_BEGIN(*gp_sniffed_urls_map);
      while(cur_node != EM_MAP_END(*gp_sniffed_urls_map))
      {
             p_url_in_map = (LX_DL_URL *)EM_MAP_VALUE(cur_node);

		SAFE_DELETE(p_url_in_map->_dl_url_array);
		SAFE_DELETE(p_url_in_map);

		em_map_erase_iterator(gp_sniffed_urls_map, cur_node);
	      cur_node = EM_MAP_BEGIN(*gp_sniffed_urls_map);
      }
	  
	SAFE_DELETE(gp_sniffed_urls_map);
	return SUCCESS;
}

_int32 lx_add_downloadable_url_to_map(EM_DOWNLOADABLE_URL * p_dl_url)
{
	_int32 ret_val = SUCCESS;
	PAIR info_map_node;
	char utf8url[MAX_URL_LEN] = {0};
	_u32 utf8url_len = MAX_URL_LEN;
	_u32 hash_value = 0;
	
	info_map_node._key = (void*)p_dl_url->_url_id;
	info_map_node._value = (void*)p_dl_url;

	/* nameǿ��ת��Ϊutf-8��ʽ.*/
	ret_val = sd_any_format_to_utf8(p_dl_url->_name, sd_strlen(p_dl_url->_name), utf8url, &utf8url_len);
	if( SUCCESS == ret_val)
	{
		sd_memset(p_dl_url->_name, 0x00, sizeof(p_dl_url->_name));
		sd_strncpy(p_dl_url->_name, utf8url, utf8url_len);
	}

	// ���
	sd_memset(utf8url, 0x00, sizeof(utf8url));
	utf8url_len = MAX_URL_LEN;
	/* urlǿ��ת��Ϊutf-8��ʽ��Ϊ��ͳһ������ģ���url(utf-8��ʽ)ƥ�䡣
	eg: ������������ɹ�֮��ȥ����̽MAP��ƥ��urlʱ������ȷ��Ӧ�������Ƿ������ߵ�״̬
	*/
	ret_val = sd_any_format_to_utf8(p_dl_url->_url, sd_strlen(p_dl_url->_url), utf8url, &utf8url_len);
	if( SUCCESS == ret_val)
	{
		sd_memset(p_dl_url->_url, 0x00, sizeof(p_dl_url->_url));
		sd_strncpy(p_dl_url->_url, utf8url, utf8url_len);
	}
	/* ǿ�ƽ�url���봦���Է�ֹͨ������̽url����������������pc�˲��ܲ��� */
	// ���
	sd_memset(utf8url, 0x00, sizeof(utf8url));

	// ��̽���ı��봦��
	url_object_encode(p_dl_url->_url, utf8url, MAX_URL_LEN);
	// ��¿������Ҫ��������%7c�滻
	if(p_dl_url->_type == UT_EMULE)
		em_replace_7c(utf8url);
	utf8url_len = sd_strlen(utf8url);
	sd_get_url_hash_value(utf8url, utf8url_len, &hash_value);

	// ��������url��url_id
	sd_memset(p_dl_url->_url, 0x00, sizeof(p_dl_url->_url));
	sd_strncpy(p_dl_url->_url, utf8url, MAX_URL_LEN - 1 );
	p_dl_url->_url_id = hash_value;
	
	if(gp_downloadable_urls==NULL)
	{
		ret_val = sd_malloc(sizeof(MAP), (void**)&gp_downloadable_urls);
		if(ret_val!=SUCCESS)
		{
			CHECK_VALUE(ret_val);
		}
		map_init(gp_downloadable_urls,lx_url_key_comp);
	}
	
	ret_val = em_map_insert_node( gp_downloadable_urls, &info_map_node );

      EM_LOG_DEBUG("lx_add_downloadable_url_to_map:%s,url_id=%u,num=%u,ret_val=%d",p_dl_url->_url,p_dl_url->_url_id,em_map_size(gp_downloadable_urls),ret_val); 
	return ret_val;
}
EM_DOWNLOADABLE_URL * lx_get_downloadable_url_from_map(_u32 url_id)
{
	_int32 ret_val = SUCCESS;
	EM_DOWNLOADABLE_URL * p_url_in_map=NULL;

	if(gp_downloadable_urls==NULL) return NULL;

	ret_val=em_map_find_node(gp_downloadable_urls, (void *)url_id, (void **)&p_url_in_map);
	sd_assert(ret_val==SUCCESS);
	//sd_assert(p_task!=NULL);
	return p_url_in_map;
}
_int32 lx_clear_downloadable_url_map(void)
{

	if(gp_downloadable_urls==NULL) return SUCCESS;
		
	EM_LOG_DEBUG("lx_clear_downloadable_url_map:%u",em_map_size(gp_downloadable_urls)); 
	  
	em_map_clear(gp_downloadable_urls);
	  
	SAFE_DELETE(gp_downloadable_urls);
	return SUCCESS;
}

_int32 lx_set_downloadable_url_status_to_lixian(char *url)
{
	_int32 ret_val = SUCCESS;
	_u32 url_len = 0,url_id = 0;
	 EM_DOWNLOADABLE_URL* p_dl_url = NULL;
	char url_buffer[MAX_URL_LEN] = {0};
	char *p_url = NULL;

	if(sd_strncmp(url, "thunder://",sd_strlen("thunder://"))==0)
	{
		/* ������һ���ַ���/,��Ҫȥ�����ַ� */
		if ('/' == url[sd_strlen(url)-1])
		{
			url[sd_strlen(url)-1] = '\0';
		}
		
		/* Decode to normal URL */
		if( sd_base64_decode(url+10,(_u8 *)url_buffer,NULL)!=0)
		{
			CHECK_VALUE(INVALID_URL);
		}

		url_buffer[sd_strlen(url_buffer)-2]='\0';
		p_url = url_buffer+2;
	}
	else
	{
		p_url = url;
	}
	
	url_len = sd_strlen(p_url);
	
	if(url_len==0) 
	{
		CHECK_VALUE(INVALID_ARGUMENT);
	}
		
	EM_LOG_DEBUG("lx_set_downloadable_url_status_to_lixian:url=%s",p_url); 
	
    // ͨ��url��ϣ�õ�url_id
	ret_val = em_get_url_hash_value( p_url,url_len, &url_id );
	CHECK_VALUE(ret_val);
    
    // �ӿ�����urlӳ����еõ���Ӧurl��Ϣ
	p_dl_url = lx_get_downloadable_url_from_map( url_id);
	if(p_dl_url==NULL)
	{
		return INVALID_URL;
	}

	if(sd_strcmp(p_dl_url->_url, p_url)==0)
	{
		p_dl_url->_status = EUS_IN_LIXIAN;
	}
	else
	{
		sd_assert(FALSE);
	}
	  
	return SUCCESS;

}
	/* �ڱ������ؿ��в����Ƿ��Ѵ�����ͬ���� */
_int32 lx_get_downloadable_url_status_in_local(EM_DOWNLOADABLE_URL * p_dl_url)
{
	_int32 ret_val = SUCCESS;
	_u32 task_id = 0;
	EM_EIGENVALUE eigenvalue;

	/* �ڱ������ؿ��в����Ƿ��Ѵ�����ͬ���� */
	switch(p_dl_url->_type)
	{
		case UT_HTTP:
		case UT_FTP:
		case UT_THUNDER:
		case UT_EMULE:
			eigenvalue._type = TT_URL;
			eigenvalue._url = p_dl_url->_url;
			eigenvalue._url_len = sd_strlen(eigenvalue._url);
		break;
		case UT_MAGNET:
			eigenvalue._type = TT_BT;
			sd_strncpy(eigenvalue._eigenvalue,p_dl_url->_name, 40);
		break;
		default:
			CHECK_VALUE(INVALID_URL);
	}
	
	
	ret_val = dt_get_task_id_by_eigenvalue_impl(&eigenvalue ,&task_id);
	if(ret_val== SUCCESS)
	{
		p_dl_url->_status = EUS_IN_LOCAL;
		return SUCCESS;
	}
	
	return -1;
}

_int32 lx_url_compare(const char*sniff_url, const char* lxspace_url)
{
	_int32 ret_val = -1;
	char tmp[MAX_URL_LEN] = {0};
	char url_1[MAX_URL_LEN] = {0};
	_u32 url_1_len = MAX_URL_LEN;
	_u32 url_1_hash= 0;
	char url_2[MAX_URL_LEN] = {0};
	_u32 url_2_len = MAX_URL_LEN;
	_u32 url_2_hash= 0;
	if(NULL == sniff_url || NULL == lxspace_url)
		return ret_val;
	sd_any_format_to_utf8(sniff_url, sd_strlen(sniff_url), tmp, &url_1_len);
	sd_any_format_to_utf8(lxspace_url, sd_strlen(lxspace_url), url_2, &url_2_len);

	if(url_1_len == url_2_len)
	{
		ret_val = em_get_url_hash_value( url_1, url_1_len, &url_1_hash);
		CHECK_VALUE(ret_val);
		ret_val = em_get_url_hash_value( url_2, url_2_len, &url_2_hash);
		CHECK_VALUE(ret_val);

		if(url_1_hash == url_2_hash)
		{
			if( 0 == sd_strcmp(url_1, url_2) )
				return SUCCESS;
		}	
	}
	return ret_val;
}

/* �����߿ռ��в����Ƿ��Ѵ�����ͬ���� */
_int32 lx_get_downloadable_url_status_in_lixian(EM_DOWNLOADABLE_URL * p_dl_url)
{
	_int32 ret_val = SUCCESS;
	_u32 url_len = 0,url_hash = 0;
      MAP_ITERATOR  cur_node = NULL; 
	LX_TASK_INFO_EX *p_info_in_map = NULL;
	//  �洢��¿��bt�����infohashֵ
	char str_buffer[MAX_URL_LEN] = {0};
	char* p_url = p_dl_url->_url;
	char encode_url[MAX_URL_LEN] = {0};
	_u32 encode_url_len = MAX_URL_LEN;
	_u32 encode_url_hash = 0;
    // ����url�����Ͳ�ͬ����Դ����ֱ��url����Ԥ����
	if(p_dl_url->_type==UT_BT)
	{
		return INVALID_URL;// BT���͵�urlӦ�ò�����״̬����ǰ������û��BT�ļ��е�����
	}
	else if(p_dl_url->_type==UT_MAGNET)
	{
		_u8 info_hash[INFO_HASH_LEN] = {0};
		ret_val = em_parse_magnet_url(p_url ,info_hash, str_buffer,NULL);
		CHECK_VALUE(ret_val);
		
		str2hex((const char*)info_hash, INFO_HASH_LEN, str_buffer, 40);	
		str_buffer[40] = '\0';
	}
	else if(p_dl_url->_type==UT_THUNDER)
	{
		/* ������һ���ַ���/,��Ҫȥ�����ַ� */
		if ('/' == p_url[sd_strlen(p_url)-1])
		{
			p_url[sd_strlen(p_url)-1] = '\0';
		}
		
		/* Decode to normal URL */
		if( sd_base64_decode(p_url+10,(_u8 *)str_buffer,NULL)!=0)
		{
			CHECK_VALUE(INVALID_URL);
		}

		str_buffer[sd_strlen(str_buffer)-2]='\0';
		p_url = str_buffer+2;
	}
	
    // �õ�url�Ĺ�ϣֵ
	url_len = sd_strlen(p_url);
	ret_val = em_get_url_hash_value( p_url,url_len, &url_hash);
	CHECK_VALUE(ret_val);
     
     
    // ��g_lx_mgr�в��ҵ�ǰurl
    for(cur_node = EM_MAP_BEGIN(g_lx_mgr._task_map);cur_node != EM_MAP_END(g_lx_mgr._task_map);cur_node = EM_MAP_NEXT(g_lx_mgr._task_map,cur_node))
    {
		p_info_in_map = (LX_TASK_INFO_EX *)EM_MAP_VALUE(cur_node);
		// �ų�MAP���ѹ��ں���ɾ��������
		if( LXS_OVERDUE == p_info_in_map->_state || LXS_DELETED == p_info_in_map->_state)
			continue;
		// ��Դ������������ͣ������߿ռ��ӦΪbt���������
		if(p_info_in_map->_type==LXT_BT_ALL&&p_dl_url->_type==UT_MAGNET)
		{
            // ���ʱBT���ͻ��ߴ��������ͣ���ͨ���Ƚ�url������str_buffer���ж��Ƿ��������б���
			if(sd_strstr(p_info_in_map->_origin_url,str_buffer,0) != NULL)
			{
				p_dl_url->_status = EUS_IN_LIXIAN;
				return SUCCESS;
			}
		}
		// �ų�bt����
		else if(p_info_in_map->_type!=LXT_BT && p_info_in_map->_type!=LXT_BT_ALL && p_info_in_map->_type!=LXT_BT_FILE )
		{
            // ��̽����url���Ǿ���url_encode�󱣴���map�еġ� �����б��ȡ����url���Ǿ���decode�˵ġ� ������Ҫ������url encodeһ�º���бȽ�
			sd_memset(encode_url, 0x00, MAX_URL_LEN);
			ret_val = url_object_encode(p_info_in_map->_origin_url, encode_url, MAX_URL_LEN);
			if(SUCCESS == ret_val)
			{
				sd_get_url_hash_value(encode_url, sd_strlen(encode_url), &encode_url_hash);
				if( (sd_strlen(encode_url) == url_len) && (encode_url_hash == url_hash) )
				{
					if( 0 == sd_strcmp(p_url, encode_url) )
					{
						p_dl_url->_status = EUS_IN_LIXIAN;
						return SUCCESS;
					}
				}
			}
			else
			{
				if(p_info_in_map->_origin_url_len==url_len && p_info_in_map->_origin_url_hash==url_hash)
				{
					if(sd_strcmp(p_url, p_info_in_map->_origin_url)==0)
					{
						p_dl_url->_status = EUS_IN_LIXIAN;
						return SUCCESS;
					}
				}
			}
		}
	}
	  
	return -1;  
	
}

_int32 lx_get_downloadable_url_status(EM_DOWNLOADABLE_URL * p_dl_url)
{
	_int32 ret_val = SUCCESS;

	/* �ڱ������ؿ��в����Ƿ��Ѵ�����ͬ���� */
	//ret_val = lx_get_downloadable_url_status_in_local(p_dl_url);
	//if(ret_val== SUCCESS) return SUCCESS;
		
	/* �����߿ռ��в����Ƿ��Ѵ�����ͬ���� */
	ret_val = lx_get_downloadable_url_status_in_lixian(p_dl_url);
	if(ret_val== SUCCESS) return SUCCESS;

	/* ������ */
	p_dl_url->_status = EUS_UNTREATED;
	
	return ret_val;
}
///// ��ȡ��ҳ�пɹ�����URL��id �б�,*buffer_len�ĵ�λ��sizeof(u32)
_int32 lx_get_downloadable_url_ids( const char* url,_u32 * id_array_buffer,_u32 *buffer_len)
{
 	_int32 ret_val = SUCCESS;
      //MAP_ITERATOR  cur_node = NULL; 
	EM_DOWNLOADABLE_URL *p_dl_url = NULL;
	_u32 url_num = 0;
	_u32 * p_id = id_array_buffer;
	LX_DL_URL * p_dl_url_in_map = lx_get_url_from_map(url);	

	sd_assert(buffer_len!=NULL);
	
	if(p_dl_url_in_map==NULL)
	{
		*buffer_len= 0;
		CHECK_VALUE(INVALID_URL);
	}
	
	if(id_array_buffer!=NULL)
	{
		sd_assert(*buffer_len>0);
		if(*buffer_len>p_dl_url_in_map->_url_num)
		{
			*buffer_len = p_dl_url_in_map->_url_num;
		}
	}
	else
	{
		*buffer_len = p_dl_url_in_map->_url_num;
		return SUCCESS;
	}
	
	p_dl_url = p_dl_url_in_map->_dl_url_array;
	
     for(url_num=0;url_num<*buffer_len;url_num++)
      {
		*p_id = p_dl_url->_url_id;
		ret_val =lx_add_downloadable_url_to_map(p_dl_url);
		//sd_assert(ret_val==SUCCESS);
		p_id++;
		p_dl_url++;
      }

	return SUCCESS;  
}

/* ��ȡ�ɹ�����URL ��Ϣ */
_int32 lx_get_downloadable_url_info(_u32 url_id,EM_DOWNLOADABLE_URL * p_info)
{
    // ����url_id�ӱ��ؿ�����ӳ����л�ȡdownloadable_url����Ϣ
	EM_DOWNLOADABLE_URL * p_info_in_map = lx_get_downloadable_url_from_map( url_id);
	if(p_info_in_map==NULL)
	{
		CHECK_VALUE(INVALID_URL);
	}

    // ����Ϣ���������ؽṹ��
	sd_memcpy(p_info,p_info_in_map, sizeof(EM_DOWNLOADABLE_URL));
	
    // ��ѯurl�Ĵ���״̬,�˴���Ϊ��ȷ���������������߿ռ��״̬(���ܸ�����ɾ������̽�Ľ���ͺ����߿ռ�ʵ��״̬��ƥ��)
	lx_get_downloadable_url_status(p_info);
	
	return SUCCESS;
}

///// ����URL ,cid,gicd ��BT���ӵ�info_hash ���Ҷ�Ӧ����������id
_int32 lx_get_task_id_by_eigenvalue( EM_EIGENVALUE * p_eigenvalue ,_u64 * task_id)
{
 	_int32 ret_val = SUCCESS;
      MAP_ITERATOR  cur_node = NULL; 
	LX_TASK_INFO_EX *p_info_in_map = NULL;
	LX_TASK_TYPE task_type;
	_u8 cid[CID_SIZE] = {0};
	_u32 url_hash = 0;
	char utf8url[MAX_URL_LEN] = {0};
	_u32 utf8urllen = MAX_URL_LEN;
	char thunder_url_tmp[MAX_URL_LEN] = {0};
	
	EM_LOG_DEBUG("lx_get_task_id_by_eigenvalue:%d",p_eigenvalue->_type); 

	*task_id = 0;
	if(map_size(&g_lx_mgr._task_map)==0)
	{
		return INVALID_EIGENVALUE;
	}

	switch(p_eigenvalue->_type)
	{
		case 	TT_BT :
			task_type = LXT_BT_ALL;
			break;
		case 	TT_EMULE :
			task_type = LXT_EMULE;
			break;
		case 	TT_URL :
		case 	TT_TCID :
		case 	TT_KANKAN :
		case 	TT_FILE :
		case 	TT_LAN :
		default:
		{
			if(sd_strlen(p_eigenvalue->_eigenvalue)!=0)
				task_type = LXT_UNKNOWN;
			else
				task_type = LXT_HTTP;
		}
			break;
	}
	// Ѹ��ר���������⴦��(MAP�д洢���ǽ���������)
	if( (task_type != LXT_BT_ALL) && (NULL !=p_eigenvalue->_url) )
	{
		if(0==sd_strncmp(p_eigenvalue->_url, "thunder://",sd_strlen("thunder://")))
		{
			if( 0 != sd_base64_decode(p_eigenvalue->_url + sd_strlen("thunder://"), (_u8 *)thunder_url_tmp, &p_eigenvalue->_url_len))
				return INVALID_EIGENVALUE;
	        thunder_url_tmp[sd_strlen(thunder_url_tmp) - 2] = '\0';
	        p_eigenvalue->_url= thunder_url_tmp + 2;
	        p_eigenvalue->_url_len = sd_strlen(p_eigenvalue->_url);
		}
	}
	// ����������ĳ�ʼurlת��Ϊutf8��ʽ(���������صľ���utf8��ʽ)�����ҽ�url�е����Ľ��б���
	sd_any_format_to_utf8( p_eigenvalue->_url,  p_eigenvalue->_url_len, utf8url, &utf8urllen);
	p_eigenvalue->_url= utf8url;
	p_eigenvalue->_url_len = utf8urllen;
	
	if(task_type!=LXT_HTTP&&task_type!=LXT_EMULE)
	{
		ret_val = sd_string_to_cid(p_eigenvalue->_eigenvalue, cid);
		CHECK_VALUE(ret_val);
	}
	else
	{
		ret_val = em_get_url_hash_value( p_eigenvalue->_url,p_eigenvalue->_url_len, &url_hash);
		sd_assert(ret_val == SUCCESS);
	}
	
    for(cur_node = EM_MAP_BEGIN(g_lx_mgr._task_map);cur_node != EM_MAP_END(g_lx_mgr._task_map);cur_node = EM_MAP_NEXT(g_lx_mgr._task_map,cur_node))
    {
    	p_info_in_map = (LX_TASK_INFO_EX*)EM_MAP_VALUE(cur_node);
		// �ų��ѹ��ں���ɾ��������
		if(LXS_OVERDUE == p_info_in_map->_state || LXS_DELETED== p_info_in_map->_state)
			 	continue;
		if((p_info_in_map->_type==task_type)|| (p_info_in_map->_type==LXT_FTP) ||(task_type==LXT_UNKNOWN))		
		{
			if(task_type!=LXT_HTTP&&p_info_in_map->_type!=LXT_FTP&&task_type!=LXT_EMULE)
			{
				if(sd_is_cid_equal(p_info_in_map->_cid,cid))
				{
					*task_id = p_info_in_map->_task_id;
					return SUCCESS;
				}
			}
			else
			{
				if(p_info_in_map->_origin_url_len == p_eigenvalue->_url_len && p_info_in_map->_origin_url_hash == url_hash &&
					sd_strncmp(p_info_in_map->_origin_url, p_eigenvalue->_url, p_eigenvalue->_url_len)==0)
				{
					*task_id = p_info_in_map->_task_id;
					return SUCCESS;
				}
			}
		}
	}
	
	return LXE_TASK_ID_NOT_FOUND;
}


///// ����gicd ���Ҷ�Ӧ����������id
_int32 lx_get_task_id_by_gcid( char * p_gcid ,_u64 * task_id, _u64 * file_id)
{
 	_int32 ret_val = SUCCESS;
    MAP_ITERATOR  cur_node = NULL, bt_cur_node = NULL; 
	LX_TASK_INFO_EX *p_info_in_map = NULL;
	LX_FILE_INFO *p_file_info_in_map = NULL;
	_u8 gcid[CID_SIZE] = {0};

	EM_LOG_DEBUG("lx_get_task_id_by_gcid:%s",p_gcid); 

	*task_id = 0;
	*file_id = 0;
	
	if(map_size(&g_lx_mgr._task_map)==0)
	{
		return INVALID_EIGENVALUE;
	}

	ret_val = sd_string_to_cid(p_gcid, gcid);
	CHECK_VALUE(ret_val);

	for(cur_node = EM_MAP_BEGIN(g_lx_mgr._task_map);cur_node != EM_MAP_END(g_lx_mgr._task_map);cur_node = EM_MAP_NEXT(g_lx_mgr._task_map,cur_node))
    {
		p_info_in_map = (LX_TASK_INFO_EX*)EM_MAP_VALUE(cur_node);
		// �ų��ѹ��ں���ɾ��������
		if(LXS_OVERDUE == p_info_in_map->_state || LXS_DELETED== p_info_in_map->_state)
			continue;
		if(p_info_in_map->_type != LXT_BT_ALL)
		{
			if(sd_is_cid_equal(p_info_in_map->_gcid, gcid))		
			{
				*task_id = p_info_in_map->_task_id;
				return SUCCESS;
			}
		}
		else
		{
			if( map_size(&p_info_in_map->_bt_sub_files) != 0 )
			{
				for(bt_cur_node = EM_MAP_BEGIN(p_info_in_map->_bt_sub_files);bt_cur_node != EM_MAP_END(p_info_in_map->_bt_sub_files);bt_cur_node = EM_MAP_NEXT(p_info_in_map->_bt_sub_files, bt_cur_node))
				{
					p_file_info_in_map = (LX_FILE_INFO*)EM_MAP_VALUE(bt_cur_node);
					if(sd_is_cid_equal(p_file_info_in_map->_gcid, gcid))		
					{
						*task_id = p_info_in_map->_task_id;
						*file_id = p_file_info_in_map->_file_id;
						return SUCCESS;
					}
				}
			}
		}
    }
	return LXE_TASK_ID_NOT_FOUND;
}


#endif /* ENABLE_LIXIAN */

