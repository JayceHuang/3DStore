/*--------------------------------------------------------------------------*/
/*                               IDENTIFICATION                             */
/*--------------------------------------------------------------------------*/
/*     Filename  : lixian_interface.c                                         */
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
#include "lixian/lixian_interface.h"
#include "lixian/lixian_impl.h"
#include "lixian/lixian_protocol.h"

#include "utility/sd_iconv.h"
#include "utility/version.h"
#include "utility/url.h"

#include "platform/sd_fs.h"
#include "platform/sd_network.h"
#include "platform/sd_customed_interface.h"

#include "scheduler/scheduler.h"
#include "et_interface/et_interface.h"

#include "em_interface/em_task.h"
#include "em_asyn_frame/em_msg.h"
#include "em_asyn_frame/em_notice.h"
#include "em_common/em_utility.h"
#include "em_common/em_mempool.h"
#include "em_common/em_string.h"
#include "em_common/em_time.h"
#include "em_common/em_logid.h"

#ifdef EM_LOGID
#undef EM_LOGID
#endif
#define EM_LOGID LOGID_LIXIAN

#include "em_common/em_logger.h"


/*--------------------------------------------------------------------------*/
/*                Global declarations				        */
/*--------------------------------------------------------------------------*/

static BOOL g_inited = FALSE;
static _int32 g_first_get_task_list = 0;
static BOOL g_geting_task_list = FALSE;
static BOOL g_geting_user_info = FALSE;
static LX_PT_GET_DL_URL  * gp_current_sniff_action = NULL;	
static _u32 g_update_regex_timer_id = 0;
static _u32 g_update_detect_string_timer_id = 0;
static BOOL g_geting_overdue_task_list = FALSE;
/*--------------------------------------------------------------------------*/
/*                Interfaces provid for other module				        */
/*--------------------------------------------------------------------------*/

_int32 lx_zlib_uncompress( _u8 *p_out_buffer, _int32 *p_out_len, const _u8 *p_in_buffer, _int32 in_len )
{
    int ret = 0;
    ret = uncompress( (unsigned char *)p_out_buffer, p_out_len, p_in_buffer, in_len );
    return ret ;
}
_int32 init_lixian_module(void)
{
	_int32 ret_val = SUCCESS;
	EM_LOG_DEBUG( "init_lixian_module" );

	if(g_inited) return SUCCESS;
	
	ret_val = lx_init_mgr();
	if(ret_val!=SUCCESS) goto  ErrHandler;

	ret_val = lx_make_xml_file_store_dir();
	if(ret_val!=SUCCESS) goto  ErrHandler;

#ifdef ENABLE_SNIFF_URL
	#ifdef ENABLE_REGEX_DETECT
	/* Start the update_regex */  // 
	ret_val = em_start_timer(em_update_regex_timeout, NOTICE_ONCE,5*1000, 0,NULL,&g_update_regex_timer_id);  
	sd_assert(ret_val==SUCCESS);
	#endif/*ENABLE_REGEX_DETECT*/
	#ifdef ENABLE_STRING_DETECT
	/* Start the update_detect_string */  // 
	ret_val = em_start_timer(em_update_detect_string_timeout, NOTICE_ONCE,5*1000, 0,NULL,&g_update_detect_string_timer_id);  
	sd_assert(ret_val==SUCCESS);
	#endif/*ENABLE_STRING_DETECT*/
	
#endif /* ENABLE_SNIFF_URL */
	g_inited = TRUE;
	g_first_get_task_list = 0;
	g_geting_task_list = FALSE;
	g_geting_user_info = FALSE;
	gp_current_sniff_action = NULL;
	
	set_customed_interface(ET_ZLIB_UNCOMPRESS,lx_zlib_uncompress);
		
	return SUCCESS;
	
ErrHandler:
	uninit_lixian_module();
	CHECK_VALUE(ret_val);
	return ret_val;
}
_int32 uninit_lixian_module(void)
{
	_int32 ret_val = SUCCESS;
	EM_LOG_DEBUG( "uninit_lixian_module" );

	if(!g_inited) return SUCCESS;

#ifdef ENABLE_SNIFF_URL
	em_cancel_timer(g_update_regex_timer_id);
	g_update_regex_timer_id=0;
	em_set_detect_regex(NULL);// ����ΪNULLʱ�ͷ�������ʽ�б���ռ�ڴ�
#endif /* ENABLE_SNIFF_URL */

	lx_uninit_mgr();

	gp_current_sniff_action=NULL;
	g_inited = FALSE;
	
	return ret_val;
}

/////14 ���߿ռ���ؽӿ�
//	ע��:����char *�ַ�����ΪUTF8��ʽ!

/* �����ؿ�����Ѹ���û��ʺ���Ϣ */
_int32 lx_set_user_info(_u64 user_id,const char * user_name,const char * old_user_name,_int32 vip_level,const char * session_id)
{
	_int32 ret_val = SUCCESS;
	EM_LOG_DEBUG( "lx_set_user_info" );
	
	ret_val = init_lixian_module();
	CHECK_VALUE(ret_val);
	
	lx_clear_action_list_except_sniff();
	lx_clear_task_map();
	
	ret_val = lx_set_base(user_id, user_name,old_user_name, vip_level,session_id);
	if(ret_val!=SUCCESS) goto  ErrHandler;

	/* ���������б����ػ��� */
	//g_first_get_task_list = 10;
	//lx_get_task_list(NULL,NULL,0);
    
	return SUCCESS;

ErrHandler:
	uninit_lixian_module();
	CHECK_VALUE(ret_val);
	return ret_val;
}

/* �����ؿ�����Ѹ���û��ʺ���Ϣ */
_int32 lx_set_sessionid_info(const char * session_id)
{
	_int32 ret_val = SUCCESS;
	EM_LOG_DEBUG( "lx_set_sessionid_info" );
	
	ret_val = lx_set_sessionid(session_id);

	return ret_val;
}

_int32 lx_login(void* user_data, LX_LOGIN_CALLBACK callback_fun, _u32 * p_action_id)
{
	_int32 ret_val = SUCCESS;
	EM_LOG_DEBUG( "lx_login" );

	#ifdef LX_LIST_CACHE
	if(em_is_net_ok(TRUE)&&(sd_get_net_type()!=UNKNOWN_NET))
	{
		ret_val = SUCCESS;
		EM_LOG_DEBUG("lx_login:net_type=%u!!",sd_get_net_type());
	}
	else
	{
		_u32 max_task_num = 0 ,current_task_num = 0;
		LX_LOGIN_RESULT resp = {0};
		/* No network �ص�֪ͨ */
		EM_LOG_DEBUG("lx_login:No network net_type=%u!!",sd_get_net_type());
		
		ret_val = lx_load_task_list_info(lx_get_base()->_userid);
		if(ret_val==SUCCESS)
		{
			/* �����ɹ� */
			resp._result = SUCCESS;
		}
		else
		{
			/* ���粻���� */
			resp._result = NETWORK_NOT_READY;
		}
		resp._action_id = MAX_U32 -1;
		resp._user_data = user_data;
		lx_get_task_num(&max_task_num,&current_task_num);
		resp._max_task_num = max_task_num;
		resp._current_task_num = current_task_num;
		
		lx_get_space(&resp._max_space,&resp._available_space);
		
		*p_action_id = resp._action_id;

		if( NULL != callback_fun )
			callback_fun(&resp);
		
		/* �ӿ���Ȼ���سɹ� */
		return SUCCESS;
	}
	#endif
	
	ret_val = lx_get_user_info_req(user_data, callback_fun, p_action_id);

	return ret_val;
}

_int32 lx_has_list_cache(_u64 userid, BOOL * is_cache)
{
	BOOL ret_val = FALSE;
	char file_path[MAX_FULL_PATH_BUFFER_LEN]={0};
	char *system_path = NULL;
	
	system_path = em_get_system_path();
	sd_snprintf(file_path, MAX_FULL_PATH_BUFFER_LEN-1, "%s/%llu.lxt",system_path,userid);

	ret_val = sd_file_exist(file_path);
	if(ret_val != TRUE) 
		*is_cache = FALSE;
	else
		*is_cache = TRUE;
	return SUCCESS;
}


/* ��ȡ���߿ռ������б�
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
_int32 lx_get_task_list(LX_GET_TASK_LIST * p_param,_u32 * p_action_id,_int32 priority)
{
 	_int32 ret_val = SUCCESS;
	
	EM_LOG_DEBUG( "lx_get_task_list,g_geting_task_list=%d,p_param=0x%X",g_geting_task_list ,p_param);
	
	if(!lx_is_logined()) return -1;

	if(g_geting_task_list)
	{
		return ETM_BUSY;
	}

	// ��������Ĵ���
	#ifdef LX_LIST_CACHE
	if(em_is_net_ok(TRUE)&&(sd_get_net_type()!=UNKNOWN_NET))
	{
		ret_val = SUCCESS;
		EM_LOG_DEBUG("lx_get_task_list:net_type=%u!!",sd_get_net_type());
	}
	else
	{
		LX_TASK_LIST task_list_resp = {0};
		/* No network �ص�֪ͨ */
		EM_LOG_DEBUG("lx_get_task_list:No network net_type=%u!!",sd_get_net_type());
		
		lx_load_task_list_info(lx_get_base()->_userid);
		
		task_list_resp._action_id = MAX_U32 -1;
		task_list_resp._user_data = p_param->_user_data;
		task_list_resp._result = NETWORK_NOT_READY;
		lx_get_task_num(&task_list_resp._total_task_num,&task_list_resp._task_num);
		lx_get_space(&task_list_resp._total_space,&task_list_resp._available_space);
		task_list_resp._task_array = NULL;
		
		*p_action_id = task_list_resp._action_id;
		
		p_param->_callback(&task_list_resp);
		return SUCCESS;
	}
	#endif
			
	if(p_param!=NULL)
	{
		#ifdef LX_GET_TASK_LIST_NEW
		ret_val = lx_get_task_list_new_req(p_param, p_action_id,priority, 1);
		#else
		ret_val = lx_get_task_list_req(p_param, p_action_id,priority, 1);
		#endif
	}
	else
	{
		#ifdef ENABLE_LIXIAN_CACHE
		_u32 action_id = 0;
		LX_GET_TASK_LIST create_param;
		#ifndef KANKAN_PROJ
		create_param._file_type = LXF_UNKNOWN;
		create_param._file_status = 0;
		#else
		create_param._file_type = LXF_VEDIO;
		create_param._file_status = 2;
		#endif
		create_param._offset = 0;
		create_param._max_task_num = LX_MAX_TASK_NUM_EACH_REQ;
		create_param._user_data = NULL;
		create_param._callback = NULL;

		#ifdef LX_GET_TASK_LIST_NEW
		ret_val = lx_get_task_list_new_req(p_param, p_action_id,priority, 1);
		#else
		ret_val = lx_get_task_list_req(&create_param, &action_id,priority, 1);
		#endif
		
		//ret_val = lx_get_task_list_req(&create_param,&action_id,priority);
		#else
		ret_val = INVALID_ARGUMENT;
		#endif /* ENABLE_LIXIAN_CACHE */
	}
	
 	return ret_val;

}

_int32 lx_batch_query_task_info(_u64 *task_id, _u32 num, void* user_data, LX_GET_TASK_LIST_CALLBACK callback_fun, _u64 commit_time, _u64 last_task_id, _u32 offset,_u32 * p_action_id)
{
 	_int32 ret_val = SUCCESS;
	LX_PT_BATCH_QUERY_TASK_INFO * p_action =NULL;
	_u32 http_id = 0;
	char jumpkey[LX_JUMPKEY_MAX_SIZE] = {0};
	_u32 jumpkey_len = 0;
	
	if(!lx_is_logined()) return LXE_NOT_LOGIN;
	
	ret_val = sd_malloc(sizeof(LX_PT_BATCH_QUERY_TASK_INFO),(void**) &p_action);
	CHECK_VALUE(ret_val);
	sd_memset(p_action,0x00,sizeof(LX_PT_BATCH_QUERY_TASK_INFO));

	p_action->_action._type = LPT_BATCH_QUERY_TASK_INFO;

	/* ------------- ��������cmd ------------ */
	/* ͷ�� */
	p_action->_req._cmd_header._version = LX_CMD_PROTOCAL_VERSION;
	p_action->_req._cmd_header._seq = lx_get_cmd_protocal_seq();
	// p_action._req._cmd_header._len �������
	p_action->_req._cmd_header._thunder_flag = get_product_flag();
	p_action->_req._cmd_header._compress_flag = 0;
	p_action->_req._cmd_header._cmd_type = LX_CMD_TYPE_QUERYTASK_REQ;
    
	if(SUCCESS != lx_get_jumpkey(jumpkey, &jumpkey_len))
	{
		SAFE_DELETE(p_action);
		return LXE_WRONG_REQUEST;
	}	
	p_action->_req._jump_key_len = jumpkey_len;
	sd_memcpy(p_action->_req._jump_key, jumpkey, jumpkey_len);
    
	/* �û���Ϣ*/
	p_action->_req._userid = lx_get_base()->_userid;
	/* ------------------------------------------- */
	p_action->_req._task_id = task_id;
	p_action->_req._task_num = num;
	p_action->_req._last_task_id = last_task_id;
	p_action->_req._last_commit_time = commit_time;
	p_action->_req._offset = offset;
    p_action->_req._user_data = user_data;
	p_action->_req._callback_fun = callback_fun;
	
	p_action->_action._req_buffer_len = MAX_POST_REQ_BUFFER_LEN;
	p_action->_action._req_data_len = p_action->_action._req_buffer_len;
	
	ret_val = lx_build_req_batch_query_task_info(p_action);
	
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}
    
	ret_val = lx_get_xml_file_store_path(p_action->_action._file_path);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}
    
	ret_val = lx_post_req((LX_PT *) p_action,&http_id,0);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}
	p_action->_action._action_id = http_id;
	p_action->_action._state = LPS_RUNNING;

	*p_action_id = http_id;
	
	lx_add_action_to_list((LX_PT * )p_action);

 	return ret_val;

}

_int32 lx_batch_query_task_info_resp(LX_PT * p_action)
{
    _int32 ret_val = SUCCESS;
    LX_PT_BATCH_QUERY_TASK_INFO* p_action_ls = (LX_PT_BATCH_QUERY_TASK_INFO *)p_action;
    LX_TASK_LIST* p_result = &p_action_ls->_resp;
     
    EM_LOG_DEBUG( "lx_batch_query_task_info_resp" );
    p_result->_action_id = p_action->_action_id;
    p_result->_user_data = p_action_ls->_req._user_data;
    p_result->_result = p_action->_error_code;
     
    if(p_action->_error_code == SUCCESS)
    {
        if(p_action->_file_id != 0)
        {
            sd_close_ex(p_action->_file_id);
            p_action->_file_id = 0;
        }
        if( (p_action->_resp_status == 0) || (p_action->_resp_status == 200))
        {
        	p_action_ls->_resp._task_num = p_action_ls->_req._task_num;
			p_action_ls->_resp._total_task_num = p_action_ls->_req._task_num;
			ret_val = lx_get_space(&p_action_ls->_resp._total_space, &p_action_ls->_resp._available_space);
	        ret_val = lx_parse_resp_batch_query_task_info(p_action_ls);
	        if(ret_val != SUCCESS) 
	        {
	            p_result->_result = ret_val;
	            goto ErrHandler;
	        }
        }
        else
        {
            p_result->_result = p_action->_resp_status;
            sd_assert( p_result->_result!=SUCCESS);
        }
    }
     
ErrHandler:
	//if(p_action_ls->_req._last_task_id != 0)
		//g_geting_task_list = FALSE;
     /* �ص�֪ͨUI */
	if(p_action_ls->_req._callback_fun != NULL)
	{
		if(p_action_ls->_req._offset != 0)
		{	
			p_result->_task_num += MAX_GET_TASK_NUM;
			p_result->_total_task_num += MAX_GET_TASK_NUM;
		}
		p_action_ls->_req._callback_fun(p_result);

		if(p_action_ls->_req._offset == 0)
		{
			LX_GET_TASK_LIST lx_list = {0};
			lx_list._max_task_num = 10000;
			lx_list._offset = MAX_GET_TASK_NUM;
			lx_list._user_data = p_action_ls->_req._user_data;
			lx_list._callback = p_action_ls->_req._callback_fun;
			ret_val = lx_get_task_list_binary_req(&lx_list, &p_action_ls->_resp._action_id, 0, p_action_ls->_req._last_commit_time, p_action_ls->_req._last_task_id);
			if(ret_val != SUCCESS)
			{	
				p_result->_result = ret_val;
				goto ErrHandler;
			}
			if(p_action->_file_id != 0)
			{
				sd_close_ex(p_action->_file_id);
				p_action->_file_id = 0;
			}
			sd_delete_file(p_action->_file_path);	
			SAFE_DELETE(p_action);
			return SUCCESS;
		}
	}
     
    if(p_action->_file_id!=0)
    {
        sd_close_ex(p_action->_file_id);
        p_action->_file_id = 0;
    }
     
    sd_delete_file(p_action->_file_path);
     
    SAFE_DELETE(p_action);
	return ret_val;
}


_int32 lx_get_task_list_binary_req(LX_GET_TASK_LIST * p_param,_u32 * p_action_id,_int32 priority, _u64 commit_time, _u64 task_id)
{
 	_int32 ret_val = SUCCESS;
	LX_PT_TASK_ID_LS * p_action =NULL;
	_u32 http_id = 0;
	char jumpkey[LX_JUMPKEY_MAX_SIZE] = {0};
	_u32 jumpkey_len = 0;
	
	EM_LOG_DEBUG( "lx_get_task_list_req:file_type=%d,file_status=%d,callback=0x%X",p_param->_file_type,p_param->_file_status,p_param->_callback);
	
	if(!lx_is_logined()) return -1;

	ret_val = sd_malloc(sizeof(LX_PT_TASK_ID_LS),(void**) &p_action);
	CHECK_VALUE(ret_val);
	sd_memset(p_action,0x00,sizeof(LX_PT_TASK_ID_LS));

	p_action->_action._type = LPT_BINARY_TASK_LS;
	sd_memcpy(&p_action->_req._task_list, p_param, sizeof(LX_GET_TASK_LIST));

	/* ------------- ��������cmd ------------ */
	/* ͷ�� */
	p_action->_req._cmd_header._version = LX_CMD_PROTOCAL_VERSION;
	p_action->_req._cmd_header._seq = lx_get_cmd_protocal_seq();
	// p_action._req._cmd_header._len �������
	p_action->_req._cmd_header._thunder_flag = get_product_flag();
	p_action->_req._cmd_header._compress_flag = 0;
	p_action->_req._cmd_header._cmd_type = LX_CMD_TYPE_QUERY_SOME_TASKID_REQ;

	if(SUCCESS != lx_get_jumpkey(jumpkey, &jumpkey_len))
	{
		SAFE_DELETE(p_action);
		return LXE_WRONG_REQUEST;
	}	
	p_action->_req._jump_key_len = jumpkey_len;
	sd_memcpy(p_action->_req._jump_key, jumpkey, jumpkey_len);

	/* �û���Ϣ*/
	p_action->_req._userid = lx_get_base()->_userid;
	p_action->_req._vip_level = lx_get_base()->_vip_level;

	p_action->_req._commit_time = commit_time;
	p_action->_req._task_id = task_id;
	p_action->_req._flag = 0;
	p_action->_req._retry_count = 0;
	
	//if(task_id == 0)
	//	p_action->_req._task_list._max_task_num = MAX_GET_TASK_NUM;	
	/* ------------------------------------------- */
	p_action->_action._req_buffer_len = MAX_POST_REQ_BUFFER_LEN;
	p_action->_action._req_data_len = p_action->_action._req_buffer_len;
	
	ret_val = lx_build_req_query_task_id_list(p_action);
	
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}

	ret_val = lx_get_xml_file_store_path(p_action->_action._file_path);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}

	ret_val = lx_post_req((LX_PT *) p_action,&http_id,0);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}
	p_action->_action._action_id = http_id;
	p_action->_action._state = LPS_RUNNING;
	
	lx_add_action_to_list((LX_PT * )p_action);

	*p_action_id = http_id;
	
 	return ret_val;

}

 _int32 lx_get_task_list_binary_resp(LX_PT * p_action)
 {
 	_int32 ret_val = SUCCESS;
	LX_PT_TASK_ID_LS * p_action_ls = (LX_PT_TASK_ID_LS *)p_action;
	LX_TASK_LIST * p_result = &p_action_ls->_resp;
	_u64 *all_task_id = NULL;
	_u32 task_id_num = 0;
	_u64 last_commit_time = 0;
	_u64 last_task_id = 0;
	
	p_result->_action_id = p_action->_action_id;
	p_result->_user_data = p_action_ls->_req._task_list._user_data;

	p_result->_result = p_action->_error_code;

	EM_LOG_DEBUG( "lx_get_task_list_binary_resp:file_type=%d,file_status=%d,error_code=%d",p_action_ls->_req._task_list._file_type,p_action_ls->_req._task_list._file_status,p_action->_error_code);
	if(p_action->_error_code == SUCCESS)
	{
		if(p_action->_file_id!=0)
		{
			sd_close_ex(p_action->_file_id);
			p_action->_file_id = 0;
		}
		/* �ֶβ�ѯʱ��һ�δ�MAP ��������������� offset��¼ÿ����ȡ������ƫ��ֵ*/
		if(p_action_ls->_req._task_list._offset == 0)
		{
			lx_clear_task_map();
		}
		
		ret_val = lx_parse_resp_query_task_id_list(p_action_ls, &last_task_id, &last_commit_time);
		if(ret_val != SUCCESS) 
		{
			p_result->_result = ret_val;
			goto ErrHandler;
		}

		EM_LOG_DEBUG( "lx_get_task_list_binary_resp:resp_status=%d,error_code=%d,task_num=%u",p_action->_resp_status,p_action->_error_code,p_action_ls->_resp._task_num);
		if( p_action->_resp_status == 0 || p_action->_resp_status == 200)
		{
			/* ls �ɹ� */
			p_result->_result = 0;
			/* �ֶβ�ѯ */
			#if 0
			if( p_action_ls->_resp._task_num == MAX_GET_TASK_NUM )
			{	
				p_action_ls->_req._task_list._offset += p_action_ls->_resp._task_num;
				ret_val = lx_get_task_list_binary_req(&p_action_ls->_req._task_list, &p_action_ls->_resp._action_id, 0, p_action_ls->_req._commit_time, p_action_ls->_req._task_id);
				if(ret_val != SUCCESS)
				{	
					p_result->_result = ret_val;
					goto ErrHandler;
				}
				if(p_action->_file_id != 0)
				{
					sd_close_ex(p_action->_file_id);
					p_action->_file_id = 0;
				}
				sd_delete_file(p_action->_file_path);	
				SAFE_DELETE(p_action);
				return SUCCESS;
			}
			#endif

			if(p_action_ls->_resp._task_num != 0 )//|| p_action_ls->_req._task_list._offset != 0)
			{
				// ������ѯ��ȡ��������id�б��������Ϣ(���б����ã���Ϊÿ�ηֶ���ȡʱ����һ���ظ�����)
				//task_id_num = lx_num_of_task_in_list();
				task_id_num = p_action_ls->_resp._task_num;
				ret_val = sd_malloc(task_id_num*sizeof(_u64), (void*)&all_task_id);
				sd_assert(ret_val==SUCCESS);
				if(ret_val != SUCCESS)
				{	
					p_result->_result = ret_val;
					goto ErrHandler;
				}
				ret_val = sd_memset(all_task_id, 0x00, task_id_num*sizeof(_u64));
				sd_assert(ret_val==SUCCESS);
				lx_get_all_task_id(all_task_id, p_action_ls->_req._task_id);
				ret_val = lx_batch_query_task_info(all_task_id, task_id_num, p_action_ls->_req._task_list._user_data, p_action_ls->_req._task_list._callback, last_commit_time, last_task_id, p_action_ls->_req._task_id, &p_action_ls->_action._action_id);
				if(ret_val != SUCCESS)
				{	
					p_result->_result = ret_val;
					goto ErrHandler;
				}
				SAFE_DELETE(all_task_id);
				return SUCCESS;
			}
		}
		else
		{
			p_result->_result = p_action->_resp_status;
			sd_assert( p_result->_result!=SUCCESS);
		}
	}


ErrHandler:
	g_geting_task_list = FALSE;
	/* �ص�֪ͨUI */
	if(p_action_ls->_req._task_list._callback!=NULL)
	{
		p_action_ls->_req._task_list._callback(p_result);
		SAFE_DELETE(all_task_id);
	}
	
	if(p_action->_file_id!=0)
	{
		sd_close_ex(p_action->_file_id);
		p_action->_file_id = 0;
	}

	sd_delete_file(p_action->_file_path);
	SAFE_DELETE(p_action);
	
	return SUCCESS;
 }

_int32 lx_get_task_list_req(LX_GET_TASK_LIST * p_param,_u32 * p_action_id,_int32 priority, _u32 retry_num)
{
 	_int32 ret_val = SUCCESS;
	LX_PT_TASK_LS * p_action =NULL;
	_u32 http_id = 0;
	
	EM_LOG_DEBUG( "lx_get_task_list_req:file_type=%d,file_status=%d,callback=0x%X",p_param->_file_type,p_param->_file_status,p_param->_callback);
	
	if(!lx_is_logined()) return -1;

	ret_val = sd_malloc(sizeof(LX_PT_TASK_LS),(void**) &p_action);
	CHECK_VALUE(ret_val);
	sd_memset(p_action,0x00,sizeof(LX_PT_TASK_LS));

	p_action->_action._type = LPT_TASK_LS;
	sd_memcpy(&p_action->_req, p_param,sizeof(LX_GET_TASK_LIST));

	#ifdef ENABLE_LX_XML_ZIP
	p_action->_action._is_compress=TRUE;
	#endif/* ENABLE_LX_XML_ZIP */

	#ifdef ENABLE_LX_XML_AES
	p_action->_action._is_aes=TRUE;
	#endif/* ENABLE_LX_XML_AES */

	if(p_action->_action._is_aes)
	{
		lx_get_aes_key( p_action->_action._aes_key);
	}

	if(p_action->_req._offset == 0)
		p_action->_req._max_task_num = MAX_GET_TASK_NUM;

	p_action->_action._retry_num = retry_num;
	p_action->_action._req_buffer_len = MAX_POST_REQ_BUFFER_LEN;
	p_action->_action._req_data_len = p_action->_action._req_buffer_len;
	ret_val = lx_build_req_task_ls(lx_get_base(),p_action);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}

	ret_val = lx_get_xml_file_store_path(p_action->_action._file_path);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}

	ret_val = lx_post_req((LX_PT *) p_action,&http_id,priority);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}
	p_action->_action._action_id = http_id;
	p_action->_action._state = LPS_RUNNING;
	
	lx_add_action_to_list((LX_PT * )p_action);

	*p_action_id = http_id;
	g_geting_task_list = TRUE;
 	return ret_val;

}

/* ���������б��ش洢���� by hx @ 20130107 */

_int32 lx_save_task_list_info(_u64 userid, LIST *task_list)
{
	_int32 ret_val = SUCCESS;
	//_u32 tree_id = 0,node_id = 0;
	_u32 file_id = 0, writesize = 0;
	_u32 task_num = 0;
	_u64 fileops = 0;
	char file_path[MAX_FULL_PATH_BUFFER_LEN]={0};
	char *system_path = NULL;
	pLIST_NODE cur_node = NULL;
	LX_TASK_INFO_EX *p_task_in_map = NULL;
	_u64 max_space=0,available_space=0;

	EM_LOG_DEBUG("lx_save_task_list_info:userid=%llu", userid);
	if( 0 == userid || NULL == task_list)
		return INVALID_ARGUMENT;
	
	task_num = list_size(task_list);
	
	system_path = em_get_system_path();
	sd_snprintf(file_path, MAX_FULL_PATH_BUFFER_LEN-1, "%s/%llu.lxt",system_path,userid);
	/* ɾ�����ļ� */
	sd_delete_file(file_path);
	ret_val = sd_open_ex(file_path, O_FS_CREATE, &file_id);
	CHECK_VALUE(ret_val);

	lx_get_space(&max_space,&available_space);

	writesize = 0;
	/* �û����߿ռ��ܴ�С */
	ret_val = sd_write(file_id, (char *)&max_space, sizeof(_u64), &writesize);
	if(ret_val != SUCCESS || writesize != sizeof(_u64))
	{
		ret_val = WRITE_TASK_FILE_ERR;
		goto ErrHandler;
	}
	fileops += writesize;
	
	writesize = 0;
	/* �û����߿ռ�ʣ���С */
	ret_val = sd_write(file_id, (char *)&available_space, sizeof(_u64), &writesize);
	if(ret_val != SUCCESS || writesize != sizeof(_u64))
	{
		ret_val = WRITE_TASK_FILE_ERR;
		goto ErrHandler;
	}
	fileops += writesize;
	
	writesize = 0;
	/* �û����߿ռ�������� */
	ret_val = sd_write(file_id, (char *)&task_num, sizeof(_u32), &writesize);
	if(ret_val != SUCCESS || writesize != sizeof(_u32))
	{
		ret_val = WRITE_TASK_FILE_ERR;
		goto ErrHandler;
	}
	fileops += writesize;
	
	writesize = 0;
	/* ���� MAX_FULL_PATH_BUFFER_LEN ���ֽ�Ϊ�Ժ���չ��*/
	sd_memset(file_path,0x00,MAX_FULL_PATH_BUFFER_LEN);
	ret_val = sd_write(file_id, file_path, MAX_FULL_PATH_BUFFER_LEN, &writesize);
	if(ret_val != SUCCESS || writesize != MAX_FULL_PATH_BUFFER_LEN)
	{
		ret_val = WRITE_TASK_FILE_ERR;
		goto ErrHandler;
	}
	fileops += writesize;
	
	writesize = 0;
	
	cur_node = LIST_BEGIN(*task_list);
	while(cur_node != LIST_END(*task_list))
	{
		p_task_in_map = (LX_TASK_INFO_EX *)(LIST_VALUE(cur_node));
		if( NULL == p_task_in_map )
		{
			cur_node = LIST_NEXT(cur_node);
			continue;
		}
		writesize = 0;
		/* ��������Ϣ����д�������� */
		ret_val = sd_write(file_id, (char *)p_task_in_map,sizeof(LX_TASK_INFO_EX), &writesize);
		if(ret_val != SUCCESS || writesize != sizeof(LX_TASK_INFO_EX))
		{
			ret_val = WRITE_TASK_FILE_ERR;
			goto ErrHandler;
		}
		fileops += writesize;
		cur_node = LIST_NEXT(cur_node);
	}

	ret_val = sd_close_ex(file_id);
	CHECK_VALUE(ret_val);

	
	/* �ɹ����� */
	return SUCCESS;
	
	//trm_destroy_tree_impl(file_path);

	/* ���´��� */
	/*
	ret_val = trm_open_tree_impl(file_path, 0x1, &tree_id);
	CHECK_VALUE(ret_val);

	cur_node = LIST_BEGIN(*task_list);
	while(cur_node != LIST_END(*task_list))
	{
		p_task_in_map = (LX_TASK_INFO_EX *)(LIST_VALUE(cur_node));
		if( NULL == p_task_in_map )
		{
			cur_node = LIST_NEXT(cur_node);
			continue;
		}
		ret_val = trm_create_node_impl(tree_id,tree_id, (char *)&(p_task_in_map->_task_id), sizeof(_u64), p_task_in_map,sizeof(LX_TASK_INFO_EX),&node_id);
		cur_node = LIST_NEXT(cur_node);
	}

	trm_close_tree_by_id(tree_id);
	
	if(ret_val!=SUCCESS)
	{
		trm_destroy_tree_impl(file_path);
		CHECK_VALUE(ret_val);
	}
	*/
	
ErrHandler:
	/* �����ˣ��������ļ� */
	sd_close_ex(file_id);
	sd_delete_file(file_path);
	return ret_val;
}
_int32 lx_load_task_list_info(_u64 userid)
{
	_int32 ret_val = SUCCESS;
	LX_TASK_INFO_EX *p_task_in_map = NULL;
	//_u32 tree_id = 0, first_node_id = 0, next_node_id = 0;
	_u32 file_id = 0, readsize = 0;
	_u64 fileops = 0;
	_u32 task_num = 0;
	_int32 i = 0;
	_u32 buffer_len = sizeof(LX_TASK_INFO_EX);
	char file_path[MAX_FULL_PATH_BUFFER_LEN]={0};
	char *system_path = NULL;
	_u64 max_space=0,available_space=0;

	EM_LOG_DEBUG("lx_load_task_list_info:userid=%llu", userid);
	if( 0 == userid)
		return LXE_NOT_LOGIN;
	system_path = em_get_system_path();
	sd_snprintf(file_path, MAX_FULL_PATH_BUFFER_LEN-1, "%s/%llu.lxt",system_path,userid);
	
	ret_val = sd_open_ex(file_path, O_FS_RDONLY, &file_id);
	if(ret_val!=SUCCESS) return ret_val;

	readsize = 0;
	/* �û����߿ռ��ܴ�С */
	ret_val = sd_read(file_id, (char *)&max_space, sizeof(_u64), &readsize);
	if(ret_val != SUCCESS || readsize != sizeof(_u64))
	{
		ret_val = INVALID_READ_SIZE;
		goto ErrHandler;
	}
	fileops += readsize;

	readsize = 0;
	/* �û����߿ռ�ʣ���С */
	ret_val = sd_read(file_id, (char *)&available_space, sizeof(_u64), &readsize);
	if(ret_val != SUCCESS || readsize != sizeof(_u64))
	{
		ret_val = INVALID_READ_SIZE;
		goto ErrHandler;
	}
	fileops += readsize;

	readsize = 0;
	/* �û����߿ռ�������� */
	ret_val = sd_read(file_id, (char *)&task_num, sizeof(_u32), &readsize);
	if(ret_val != SUCCESS || readsize != sizeof(_u32))
	{
		ret_val = INVALID_READ_SIZE;
		goto ErrHandler;
	}
	fileops += readsize;

	lx_set_user_lixian_info(task_num,task_num,max_space,available_space);
	
	if(0 == task_num)
	{
		sd_close_ex(file_id);
		return LXE_TASK_EMPTY;
	}

	/* �������� ��MAX_FULL_PATH_BUFFER_LEN ���ֽ�Ϊ�Ժ���չ��*/
	fileops += MAX_FULL_PATH_BUFFER_LEN;
	ret_val = sd_setfilepos( file_id, fileops);
	if(ret_val != SUCCESS )
	{
		goto ErrHandler;
	}

	/* ��վ��б� */
	//ret_val = lx_clear_task_map();
	
	//ret_val = sd_pread(file_id, (char *)&task_num, sizeof(_u32), fileops, &readsize);
	
	for(i = 0; i < task_num; i++)
	{
		ret_val = lx_malloc_ex_task(&p_task_in_map);
		if(ret_val != SUCCESS )
		{
			goto ErrHandler;
		}
		//ret_val = sd_pread(file_id, (char *)p_task_in_map,sizeof(LX_TASK_INFO_EX), fileops, &readsize);
		readsize = 0;
		ret_val = sd_read(file_id, (char *)p_task_in_map,sizeof(LX_TASK_INFO_EX), &readsize);
		if(ret_val != SUCCESS || readsize != sizeof(LX_TASK_INFO_EX))
		{
			ret_val = READ_FILE_ERR;
			goto ErrHandler;
		}
		fileops += readsize;

		/* �����ٴγ�ʼ����MAP�� ��������! */
		map_init(&(p_task_in_map->_bt_sub_files),lx_task_id_comp);
		
		ret_val = lx_add_task_to_map(p_task_in_map);
		if(  ret_val != SUCCESS)
		{
			SAFE_DELETE(p_task_in_map);
		}
	}

	ret_val = sd_close_ex(file_id);
	CHECK_VALUE(ret_val);
	
	/* �ɹ����� */
	return SUCCESS;
	
	/*
	ret_val = trm_open_tree_impl(file_path, 0x1, &tree_id);
	CHECK_VALUE(ret_val);

	ret_val = lx_clear_task_map();

	ret_val = trm_get_first_child_impl( tree_id, tree_id, &first_node_id);
	if(ret_val!=SUCCESS||first_node_id==0)
	{
		trm_close_tree_by_id(tree_id);
		trm_destroy_tree_impl(file_path);
		if(ret_val!=SUCCESS)
			return ret_val;
		else
			return -1;
	}
	ret_val = lx_malloc_ex_task(&p_task_in_map);
	if( SUCCESS != ret_val)
	{	
		trm_close_tree_by_id(tree_id);
		return ret_val;
	}
	ret_val = trm_get_data_impl( tree_id, first_node_id,p_task_in_map,&buffer_len);
	if(ret_val == SUCCESS && p_task_in_map != NULL)
	{
		ret_val = lx_add_task_to_map(p_task_in_map);
		if(MAP_DUPLICATE_KEY == ret_val)
			SAFE_DELETE(p_task_in_map);
	}

	while(1)
	{
		ret_val = trm_get_next_brother_impl(tree_id,first_node_id,&next_node_id);
		if(ret_val!=SUCCESS||next_node_id==0)
		{
			trm_close_tree_by_id(tree_id);
			break;
		}
		ret_val = lx_malloc_ex_task(&p_task_in_map);
		ret_val = trm_get_data_impl( tree_id, next_node_id,p_task_in_map,&buffer_len);
		if(ret_val == SUCCESS && p_task_in_map != NULL)
		{
			ret_val = lx_add_task_to_map(p_task_in_map);
			if(MAP_DUPLICATE_KEY == ret_val)
				SAFE_DELETE(p_task_in_map);
		}
		first_node_id = next_node_id;
	}
	*/
	
ErrHandler:
	/* �����ˣ��������ļ� */
	sd_close_ex(file_id);
	sd_delete_file(file_path);
	return ret_val;
}
/*
	����: ������Ĵ�����һ�ηֶ���ȡ���б�󣬶�ʣ����б�ͬʱ���з�����ȡ���󣬲��Բ��з��͵����������Ӧ����
	��̬����:
	sequence: ���ڼ�¼��������ķ�����Ϣ��(����ֵ���������ƫ����offset����׷��)
	resp_num: ���ڼ�¼�Ѿ�����ķ�����������Ϣ������(��ʼ��Ϊ1���ڵ�һ�η��ش����м�¼����Ҫ����ķ����������)
*/
_u32 lx_get_req_num(_u32 total_num)
{
	int req_num = MAX_GET_TASK_NUM;
// ���ڷ��������ܲ���һ����ȡ���ٸ��������Կ���ÿ����ȡ�������Ϊ100����2014-1-17 by hexiang
/*	while(1)
	{
		if(total_num / req_num > MAX_GET_TASK_LIST_REQ_NUM)
			req_num += MAX_GET_TASK_NUM;
		else
			break;
	}
*/
	return req_num;
}

 _int32 lx_get_task_list_resp(LX_PT * p_action)
 {
 	_int32 ret_val = SUCCESS;
	LX_PT_TASK_LS * p_action_ls = (LX_PT_TASK_LS *)p_action;
	LX_TASK_LIST * p_result = &p_action_ls->_resp;
	unsigned int offset = 0;
	static unsigned int sequence = 0; /*��һ�������յ����������к�(��offset��Ϊ����ֵ)*/
	static unsigned int resp_num = 0; /*�ڵ�һ�η���100�������ͬʱ���͵�������Ŀ*/
	static unsigned int get_list_num = MAX_GET_TASK_NUM; /*��֮��ͬʱ���͵������������Ŀ*/
	static BOOL error_callback_flag = FALSE; /*����������ص�һ�εı�־*/
	int i = 0;
	LX_GET_TASK_LIST p_param = {0};
	LX_PT_TASK_LS * p_action_in_map = NULL;

	//struct timeval start, end;
	
	p_result->_action_id = p_action->_action_id;
	p_result->_user_data = p_action_ls->_req._user_data;

	p_result->_result = p_action->_error_code;

	EM_LOG_DEBUG( "lx_get_task_list_resp:file_type=%d,file_status=%d,error_code=%d",p_action_ls->_req._file_type,p_action_ls->_req._file_status,p_action->_error_code);
	if(p_action->_error_code == SUCCESS)
	{
		if(p_action->_file_id!=0)
		{
			sd_close_ex(p_action->_file_id);
			p_action->_file_id = 0;
		}
		// ��һ����ȡ�б�ʱ�ķ��ش���: ���MAP-->��������-->��������ʣ������������
		if(p_action_ls->_req._file_status == 0 && p_action_ls->_req._offset == 0)
		{
			/* ��MAP ��������������� */
			lx_clear_task_map();
			// ��Ӧͷ���ɹ���ȥ����
			if( p_action->_resp_status == 0 || p_action->_resp_status == 200 )
			{
				ret_val = lx_parse_resp_task_list(p_action_ls);
				if(ret_val != SUCCESS) 
				{
					p_result->_result = ret_val;
					goto ErrHandler;
				}
				if(p_action_ls->_action._error_code == SUCCESS)
				{
					p_result->_result = 0;
					lx_set_user_lixian_info(p_result->_total_task_num+1,p_result->_total_task_num,p_result->_total_space,p_result->_available_space);
					// һ���Է���ʣ����Ҫ�ֶλ�ȡ����������,����ÿ�����¶�static����ֵ
					if( p_result->_total_task_num > MAX_GET_TASK_NUM)
					{
						get_list_num = lx_get_req_num(p_result->_total_task_num);
						if( (p_result->_total_task_num - MAX_GET_TASK_NUM) % get_list_num == 0)
							resp_num = (p_result->_total_task_num - MAX_GET_TASK_NUM) / get_list_num ;
						else
							resp_num = ( (p_result->_total_task_num - MAX_GET_TASK_NUM) / get_list_num ) + 1; 
						sd_memcpy(&p_param, &p_action_ls->_req, sizeof(LX_GET_TASK_LIST));
						p_param._max_task_num = get_list_num;
						for(i = 0; i < resp_num; i++)
						{
							p_param._offset = i * get_list_num + MAX_GET_TASK_NUM;
							ret_val = lx_get_task_list_req(&p_param, &p_action_ls->_resp._action_id, 0, 1);
							if(ret_val != SUCCESS)
							{	
								p_result->_result = ret_val;
								break;
							}
						}
						sequence = MAX_GET_TASK_NUM;
						error_callback_flag = FALSE;
					}
					else
					{
					#ifdef LX_LIST_CACHE
						EM_LOG_DEBUG( "lx_get_task_list_resp: lx_save_task_list_info,user_id = %llu, task_num = %u", lx_get_base()->_userid, lx_num_of_task_in_map());
						lx_save_task_list_info(lx_get_base()->_userid, &(lx_get_manager()->_task_list));
					#endif
					}
				}
				// ���ý���ʧ�ܴ�����
				else
					p_result->_result = p_action_ls->_action._error_code;
			}
			else
			{
				// ��Ӧʧ�ܴ�����
				p_result->_result = p_action->_resp_status;
				#ifdef LX_LIST_CACHE
				//gettimeofday(&start, NULL);
				lx_load_task_list_info(lx_get_base()->_userid);
				EM_LOG_DEBUG( "lx_get_task_list_resp errcode = %d: lx_load_task_list_info,task_num = %u", p_action->_resp_status, lx_num_of_task_in_map());
				//gettimeofday(&end, NULL);
				//printf("lx_get_task_list: task_num = %u, time = %lu\n", p_result->_total_task_num, (1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec)/1000 );
				#endif
			}
			
			if(resp_num == 0)
			{
				g_geting_task_list = FALSE;
				EM_LOG_DEBUG( "lx_get_task_list_resp set g_geting_task_list = %d FALSE", g_geting_task_list);
			}
			if(p_action_ls->_req._callback!=NULL)
			{
				p_action_ls->_req._callback(p_result);
				lx_clear_task_list_action_map();
			}
			if(p_action->_file_id!=0)
			{
				sd_close_ex(p_action->_file_id);
				p_action->_file_id = 0;
			}
			sd_delete_file(p_action->_file_path);
			SAFE_DELETE(p_action);
			return SUCCESS;
		}
		else
		{
			// ��������Ļظ���Ų�������ֵ�����MAP�Ժ���
			if(sequence != p_action_ls->_req._offset)
			{
				ret_val = lx_add_task_list_action_to_map(p_action_ls);
				sd_assert(ret_val == SUCCESS);
				if(ret_val != SUCCESS)
				{	
					p_result->_result = ret_val;
					goto ErrHandler;
				}
				return SUCCESS;
			}
			else
			{
				// ���������ص���Ϣ����������ȡ�����ֵ�������������MAP�л������Ϣ
				ret_val = lx_parse_resp_task_list(p_action_ls);
				if(ret_val != SUCCESS) 
				{
					p_result->_result = ret_val;
					goto ErrHandler;
				}
				resp_num--;
				sequence = p_action_ls->_req._offset + get_list_num;
				// ѭ������MAP�еĻ�����Ϣ
				while( 0 != lx_num_of_task_list_action_in_map())
				{
					// �������ֵ��MAP����˳����
					p_action_in_map = lx_get_task_list_action_from_map(sequence);
					if(p_action_in_map != NULL)
					{	
						ret_val = lx_parse_resp_task_list(p_action_in_map);
						if(ret_val != SUCCESS) 
						{
							p_result->_result = ret_val;
							goto ErrHandler;
						}
						resp_num--;
						p_result->_task_num = p_action_in_map->_resp._task_num;
						offset = p_action_in_map->_req._offset;
						// remove����ɾ����p_action_in_map�����Ժ��治������
						ret_val = lx_remove_task_list_action_from_map(sequence);
						sequence = offset + get_list_num;
					}
					// �����������һ��action����MAP��(��ʾ��û���յ�����������)��������MAP�����ȴ�������Ϣ
					else
						break;
				}
				// ʵ����ȡ���б���Ŀ = ���һ�������յ��������� + ƫ����
				p_result->_task_num += sequence - get_list_num;
				/* ������������ϲŻص�֪ͨUI */
				if(resp_num == 0)
				{
					g_geting_task_list = FALSE;
					if(p_action_ls->_req._callback!=NULL)
					{
						p_action_ls->_req._callback(p_result);
						lx_clear_task_list_action_map();
						// ��ȡ�����б�ɹ�֮�󱣴��б�����
						if(p_result->_result == SUCCESS)
						{
							#ifdef LX_LIST_CACHE
							EM_LOG_DEBUG( "lx_get_task_list_resp: lx_save_task_list_info,user_id = %llu, task_num = %u", lx_get_base()->_userid, lx_num_of_task_in_map());
							lx_save_task_list_info(lx_get_base()->_userid, &(lx_get_manager()->_task_list));
							#endif
						}
					}
				}
			}
		}
	}
	else   
	{
		EM_LOG_DEBUG( "lx_get_task_list_resp errcode = %d", p_action->_error_code);
				//gettimeofday(&end, NULL);
		/*��ʱ����,���Ի���*/
		if(p_action->_error_code == -1 || p_action->_error_code == 250)
		{
			if(p_action->_retry_num > 0)
			{
				EM_LOG_DEBUG( "error_code = %d, lx_get_task_list_req  try again",p_action->_error_code);
				
				ret_val = lx_get_task_list_req(&p_action_ls->_req, &p_action_ls->_resp._action_id, 0, 0);
				if(ret_val != SUCCESS)
				{	
					p_result->_result = ret_val;
					goto ErrHandler;
				}

				if(p_action->_file_id != 0)
				{
					sd_close_ex(p_action->_file_id);
					p_action->_file_id = 0;
				}
				sd_delete_file(p_action->_file_path);	
				SAFE_DELETE(p_action);
				return SUCCESS;
			}
			
		}
		// ��һ������100������ʱʧ�ܻص�
		if( 0 == p_action_ls->_req._offset)
		{
			if(p_action->_error_code == 500)
			{	
				ret_val = lx_get_task_list_binary_req(&p_action_ls->_req, &p_action_ls->_resp._action_id,0, 0, 0);
				if(ret_val != SUCCESS)
				{	
					return ret_val;
				}
			}
			// ����������������һ��֮���ٴγ�ʱ װ�ػ����������Ϣ��ֱ�ӻص�֪ͨ����
			#ifdef LX_LIST_CACHE
			lx_load_task_list_info(lx_get_base()->_userid);
			EM_LOG_DEBUG( "lx_get_task_list_resp errcode = %d: lx_load_task_list_info,task_num = %u", p_action->_error_code, lx_num_of_task_in_map());
			#endif
			g_geting_task_list = FALSE;
			if(p_action_ls->_req._callback!=NULL)
			{
				p_action_ls->_req._callback(p_result);
			}
		}
		// ����100������֮���͵�����ʧ��ʱֻ�ص�һ��
		else
		{
			if(!error_callback_flag)
			{
				#ifdef LX_LIST_CACHE
				lx_load_task_list_info(lx_get_base()->_userid);
				EM_LOG_DEBUG( "lx_get_task_list_resp errcode = %d: lx_load_task_list_info,task_num = %u", p_action->_error_code, lx_num_of_task_in_map());
				#endif
				g_geting_task_list = FALSE;
				if(p_action_ls->_req._callback!=NULL)
				{
					p_action_ls->_req._callback(p_result);
				}
				error_callback_flag = TRUE;
			}
		}
		lx_clear_task_list_action_map();
		if(p_action->_file_id!=0)
		{
			sd_close_ex(p_action->_file_id);
			p_action->_file_id = 0;
		}
		sd_delete_file(p_action->_file_path);
		SAFE_DELETE(p_action);
		return SUCCESS;
	}
ErrHandler:
	EM_LOG_DEBUG( "lx_get_task_list_new_resp: ErrHandler g_geting_task_list = %d", g_geting_task_list);
	g_geting_task_list = FALSE;
	
	if(p_action->_file_id!=0)
	{
		sd_close_ex(p_action->_file_id);
		p_action->_file_id = 0;
	}
	sd_delete_file(p_action->_file_path);
	SAFE_DELETE(p_action);

	return SUCCESS;
}


#if 0
 _int32 lx_get_task_list_resp(LX_PT * p_action)
 {
 	_int32 ret_val = SUCCESS;
	LX_PT_TASK_LS * p_action_ls = (LX_PT_TASK_LS *)p_action;
	LX_TASK_LIST * p_result = &p_action_ls->_resp;

	p_result->_action_id = p_action->_action_id;
	p_result->_user_data = p_action_ls->_req._user_data;

	p_result->_result = p_action->_error_code;

	g_geting_task_list = FALSE;
	
	EM_LOG_DEBUG( "lx_get_task_list_resp:file_type=%d,file_status=%d,error_code=%d",p_action_ls->_req._file_type,p_action_ls->_req._file_status,p_action->_error_code);
	if(p_action->_error_code == SUCCESS)
	{

		if(p_action->_file_id!=0)
		{
			sd_close_ex(p_action->_file_id);
			p_action->_file_id = 0;
		}
#ifndef KANKAN_PROJ
		if(p_action_ls->_req._file_status==0 && p_action_ls->_req._offset == 0)
#endif
		{
			/* ��MAP ��������������� */
			lx_clear_task_map();
		}
		
		ret_val = lx_parse_resp_task_list(p_action_ls);
		if(ret_val != SUCCESS) 
		{
			p_result->_result = ret_val;
			/*   ��Ӧxml ����ʧ��*/
			goto ErrHandler;
		}

		EM_LOG_DEBUG( "lx_get_task_list_resp:resp_status=%d,error_code=%d,task_num=%u",p_action->_resp_status,p_action->_error_code,p_action_ls->_resp._task_num);
		if( (p_action->_resp_status == 0 || p_action->_resp_status == 200)&& p_action->_error_code == SUCCESS)
		{
			if(p_action_ls->_resp._task_num!=0)
			{
				if(p_action_ls->_req._callback!=NULL)
				{
					ret_val = lx_get_task_array_from_map(p_action_ls->_req._file_type,p_action_ls->_req._file_status,p_action_ls->_req._offset,p_action_ls->_resp._task_num,&p_action_ls->_resp._task_array);
					if(ret_val!=SUCCESS)
					{
						p_result->_result = ret_val;
						/*   �ڴ�����ʧ��*/
						p_result->_total_task_num = 0;
						p_result->_task_num = 0;
						goto ErrHandler;
					}
				}
				else
				{
					lx_get_bt_task_lists(p_action_ls->_req._file_status);
				}
			}

			/* ls �ɹ� */
			p_result->_result = 0;
			g_geting_user_info = FALSE;
			lx_set_user_lixian_info(p_result->_total_task_num+1,p_result->_total_task_num,p_result->_total_space,p_result->_available_space);
		}
		else
		{
			p_result->_result = p_action->_resp_status+p_action->_error_code;
			sd_assert( p_result->_result!=SUCCESS);
		}
		#ifdef LIXIAN_SECTION_GET_LIST
		if((p_action_ls->_resp._task_num != 0) && p_action_ls->_resp._task_num + p_action_ls->_req._offset < p_action_ls->_resp._total_task_num)
		{
			LX_GET_TASK_LIST p_param = {0};
			sd_memcpy(&p_param, &p_action_ls->_req, sizeof(LX_GET_TASK_LIST));
			p_param._offset += p_action_ls->_resp._task_num;
			p_param._max_task_num = MAX_GET_TASK_NUM;
			ret_val = lx_get_task_list_req(&p_param, &p_action_ls->_resp._action_id, 0);
			if(ret_val != SUCCESS)
			{	
				p_result->_result = ret_val;
				goto ErrHandler;
			}
			if(p_action->_file_id != 0)
			{
				sd_close_ex(p_action->_file_id);
				p_action->_file_id = 0;
			}
			sd_delete_file(p_action->_file_path);	
			SAFE_DELETE(p_action);
			return SUCCESS;
		}
		#endif
	}
	else if(p_action->_error_code == -1)  
	{
		/*��ʱ����*/
		ret_val = lx_get_task_list_req(&p_action_ls->_req, &p_action_ls->_resp._action_id, 0);
		if(ret_val != SUCCESS)
		{	
			p_result->_result = ret_val;
			goto ErrHandler;
		}
		if(p_action->_file_id != 0)
		{
			sd_close_ex(p_action->_file_id);
			p_action->_file_id = 0;
		}
		sd_delete_file(p_action->_file_path);	
		SAFE_DELETE(p_action);
		return SUCCESS;
	}
	
ErrHandler:
	
#ifdef LIXIAN_SECTION_GET_LIST
	p_result->_task_num += p_action_ls->_req._offset;
#endif
	/* �ص�֪ͨUI */
	if(p_action_ls->_req._callback!=NULL)
	{
		p_action_ls->_req._callback(p_result);
		SAFE_DELETE(p_result->_task_array);
	}
	
	if(p_action->_file_id!=0)
	{
		sd_close_ex(p_action->_file_id);
		p_action->_file_id = 0;
	}

	sd_delete_file(p_action->_file_path);

	ret_val = p_result->_result;
	
	SAFE_DELETE(p_action);

	if(ret_val!=SUCCESS)
	{
		if(ret_val==16)
		{
			/* �ǻ�Ա,�������� */
			g_first_get_task_list = 0;
			return SUCCESS;
		}
		
		if(g_first_get_task_list>0)
		{
			/* ����һ�����������б����ػ��� */
			g_first_get_task_list--;
			lx_get_task_list(NULL,NULL,0);
		}
	}
	else
	{
		g_first_get_task_list = 0;
	}
	return SUCCESS;
 }
#endif

_int32 lx_get_task_list_new_req(LX_GET_TASK_LIST * p_param,_u32 * p_action_id,_int32 priority, _u32 retry_num)
{
 	_int32 ret_val = SUCCESS;
	LX_PT_TASK_LS_NEW * p_action =NULL;
	_u32 http_id = 0;
	char jumpkey[LX_JUMPKEY_MAX_SIZE] = {0};
	_u32 jumpkey_len = 0;

	EM_LOG_DEBUG( "lx_get_task_list_new_req:offset = %d,file_type=%d,file_status=%d,callback=0x%X",p_param->_offset, p_param->_file_type,p_param->_file_status,p_param->_callback);
	
	if(!lx_is_logined()) return -1;

	ret_val = sd_malloc(sizeof(LX_PT_TASK_LS_NEW),(void**) &p_action);
	CHECK_VALUE(ret_val);
	sd_memset(p_action,0x00,sizeof(LX_PT_TASK_LS_NEW));

	p_action->_action._type = LPT_TASK_LS_NEW;
	/* ------------- ��������cmd ------------ */
	/* ͷ�� */
	p_action->_req._cmd_header._version = LX_CMD_PROTOCAL_VERSION;
	p_action->_req._cmd_header._seq = lx_get_cmd_protocal_seq();
	// p_action._req._cmd_header._len �������
	p_action->_req._cmd_header._thunder_flag = 0x7000000;//get_product_flag();
	p_action->_req._cmd_header._compress_flag = 0;
	p_action->_req._cmd_header._cmd_type = LX_CMD_TYPE_GET_TASK_LIST_REQ;

	if(SUCCESS != lx_get_jumpkey(jumpkey, &jumpkey_len))
	{
		SAFE_DELETE(p_action);
		return LXE_WRONG_REQUEST;
	}	
	p_action->_req._jump_key_len = jumpkey_len;
	sd_memcpy(p_action->_req._jump_key, jumpkey, jumpkey_len);

	/* �û���Ϣ*/
	p_action->_req._userid = lx_get_base()->_userid;
	p_action->_req._vip_level = lx_get_base()->_vip_level;
	/* ------------------------------------------- */
	sd_memcpy(&p_action->_req._task_list, p_param, sizeof(LX_GET_TASK_LIST));

	if(p_action->_req._task_list._offset == 0)
		p_action->_req._task_list._max_task_num = MAX_GET_TASK_NUM;
	p_action->_action._retry_num = retry_num;
	p_action->_action._req_buffer_len = MAX_POST_REQ_BUFFER_LEN;
	p_action->_action._req_data_len = p_action->_action._req_buffer_len;
	
	ret_val = lx_build_req_get_task_list_new(p_action);
	
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}

	ret_val = lx_get_xml_file_store_path(p_action->_action._file_path);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}

	ret_val = lx_post_req((LX_PT *) p_action,&http_id,0);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}
	p_action->_action._action_id = http_id;
	p_action->_action._state = LPS_RUNNING;
	
	lx_add_action_to_list((LX_PT * )p_action);

	*p_action_id = http_id;
	g_geting_task_list = TRUE;
 	return ret_val;

}


 _int32 lx_get_task_list_new_resp(LX_PT * p_action)
 {
 	_int32 ret_val = SUCCESS;
	LX_PT_TASK_LS_NEW * p_action_ls = (LX_PT_TASK_LS_NEW *)p_action;
	LX_TASK_LIST * p_result = &p_action_ls->_resp;
	unsigned int offset = 0;
	static unsigned int sequence = 0; /*��һ�������յ����������к�(��offset��Ϊ����ֵ)*/
	static unsigned int resp_num = 0; /*�ڵ�һ�η���100�������ͬʱ���͵�������Ŀ*/
	static unsigned int get_list_num = MAX_GET_TASK_NUM; /*��֮��ͬʱ���͵������������Ŀ*/
	static BOOL error_callback_flag = FALSE; /*����������ص�һ�εı�־*/
	int i = 0;
	LX_GET_TASK_LIST p_param = {0};
	LX_PT_TASK_LS_NEW * p_action_in_map = NULL;
	LX_GET_TASK_LIST_CALLBACK callback = p_action_ls->_req._task_list._callback;
	//struct timeval start, end;
	
	p_result->_action_id = p_action->_action_id;
	p_result->_user_data = p_action_ls->_req._task_list._user_data;

	p_result->_result = p_action->_error_code;

	EM_LOG_DEBUG( "lx_get_task_list_new_resp:file_type=%d,file_status=%d,error_code=%d",p_action_ls->_req._task_list._file_type,p_action_ls->_req._task_list._file_status,p_action->_error_code);
	if(p_action->_error_code == SUCCESS)
	{
		if(p_action->_file_id!=0)
		{
			sd_close_ex(p_action->_file_id);
			p_action->_file_id = 0;
		}
		// ��һ����ȡ�б�ʱ�ķ��ش���: ���MAP-->��������-->��������ʣ������������
		//if(p_action_ls->_req._task_list._file_status == 0 && p_action_ls->_req._task_list._offset == 0)
		/* ��ȡ��ͬ���͵Ļ�����������б���� */
		if(p_action_ls->_req._task_list._offset == 0)
		{
			// ��Ӧͷ���ɹ���ȥ����
			if( p_action->_resp_status == 0 || p_action->_resp_status == 200 )
			{
				/* ��MAP ��������������� */
				lx_clear_task_map();
				ret_val = lx_parse_resp_get_task_list_new(p_action_ls);
				if(ret_val != SUCCESS) 
				{
					p_result->_result = ret_val;
				}
				else
				{
					p_result->_result = 0;
					lx_set_user_lixian_info(p_result->_total_task_num+1,p_result->_total_task_num,p_result->_total_space,p_result->_available_space);
					// һ���Է���ʣ����Ҫ�ֶλ�ȡ����������,����ÿ�����¶�static����ֵ
					if( p_result->_total_task_num > MAX_GET_TASK_NUM)
					{
						get_list_num = lx_get_req_num(p_result->_total_task_num);
						if( (p_result->_total_task_num - MAX_GET_TASK_NUM) % get_list_num == 0)
							resp_num = (p_result->_total_task_num - MAX_GET_TASK_NUM) / get_list_num ;
						else
							resp_num = ( (p_result->_total_task_num - MAX_GET_TASK_NUM) / get_list_num ) + 1; 
						sd_memcpy(&p_param, &p_action_ls->_req._task_list, sizeof(LX_GET_TASK_LIST));
						p_param._max_task_num = get_list_num;
						for(i = 0; i < resp_num; i++)
						{
							p_param._offset = i * get_list_num + MAX_GET_TASK_NUM;
							ret_val = lx_get_task_list_new_req(&p_param, &p_action_ls->_resp._action_id, 0, 1);
							if(ret_val != SUCCESS)
							{	
								p_result->_result = ret_val;
								break;
							}
						}
						sequence = MAX_GET_TASK_NUM;
						error_callback_flag = FALSE;
					}
					else
					{
					#ifdef LX_LIST_CACHE
						EM_LOG_DEBUG( "lx_get_task_list_new_resp: lx_save_task_list_info,user_id = %llu, task_num = %u", lx_get_base()->_userid, lx_num_of_task_in_map());
						lx_save_task_list_info(lx_get_base()->_userid, &(lx_get_manager()->_task_list));
					#endif
					}
				}
			}
			else
			{
				// ��Ӧʧ�ܴ�����
				p_result->_result = p_action->_resp_status;
				#if 0
				lx_load_task_list_info(lx_get_base()->_userid);
				EM_LOG_DEBUG( "lx_get_task_list_new_resp errcode = %d: lx_load_task_list_info,task_num = %u", p_action->_resp_status, lx_num_of_task_in_map());
				#endif
			}
			// ֻ��һ�������ʱ����Ҫ���ûظ�ȫ�ֲ�������������������ظ�֮��������
			if(resp_num == 0)
			{
				g_geting_task_list = FALSE;
				EM_LOG_DEBUG( "lx_get_task_list_new_resp set g_geting_task_list = %d FALSE", g_geting_task_list);
			}
			if(callback!=NULL)
			{
				EM_LOG_DEBUG( "lx_get_task_list_new_resp first: callback to ui , result = %d", p_result->_result);
				callback(p_result);
				lx_clear_task_list_action_map_new();
			}
			if(p_action->_file_id!=0)
			{
				sd_close_ex(p_action->_file_id);
				p_action->_file_id = 0;
			}
			sd_delete_file(p_action->_file_path);
			SAFE_DELETE(p_action);
			return SUCCESS;
		}
		else
		{
			// ��������Ļظ���Ų�������ֵ�����MAP�Ժ���
			if(sequence != p_action_ls->_req._task_list._offset)
			{
				ret_val = lx_add_task_list_action_to_map_new(p_action_ls);
				sd_assert(ret_val == SUCCESS);
				if(ret_val != SUCCESS)
				{	
					p_result->_result = ret_val;
					goto ErrHandler;
				}
				return SUCCESS;
			}
			else
			{
				// ���������ص���Ϣ����������ȡ�����ֵ�������������MAP�л������Ϣ
				ret_val = lx_parse_resp_get_task_list_new(p_action_ls);
				if(ret_val != SUCCESS) 
				{
					p_result->_result = ret_val;
					goto ErrHandler;
				}
				resp_num--;
				sequence = p_action_ls->_req._task_list._offset + get_list_num;
				// ѭ������MAP�еĻ�����Ϣ
				while( 0 != lx_num_of_task_list_action_in_map_new())
				{
					// �������ֵ��MAP����˳����
					p_action_in_map = lx_get_task_list_action_from_map_new(sequence);
					if(p_action_in_map != NULL)
					{	
						ret_val = lx_parse_resp_get_task_list_new(p_action_in_map);
						if(ret_val != SUCCESS) 
						{
							p_result->_result = ret_val;
							goto ErrHandler;
						}
						resp_num--;
						p_result->_task_num = p_action_in_map->_resp._task_num;
						offset = p_action_in_map->_req._task_list._offset;
						// remove����ɾ����p_action_in_map�����Ժ��治������
						ret_val = lx_remove_task_list_action_from_map_new(sequence);
						sequence = offset + get_list_num;
					}
					// �����������һ��action����MAP��(��ʾ��û���յ�����������)��������MAP�����ȴ�������Ϣ
					else
						break;
				}
				// ʵ����ȡ���б���Ŀ = ���һ�������յ��������� + ƫ����
				p_result->_task_num += sequence - get_list_num;
				/* ������������ϲŻص�֪ͨUI */
				if(resp_num == 0)
				{
					g_geting_task_list = FALSE;
					if(callback!=NULL)
					{
						EM_LOG_DEBUG( "lx_get_task_list_new_resp all request success: callback to ui, result = %d", p_result->_result);
						callback(p_result);
						lx_clear_task_list_action_map_new();
						// ��ȡ�����б�ɹ�֮�󱣴��б�����
						if(p_result->_result == SUCCESS)
						{
							#ifdef LX_LIST_CACHE
							EM_LOG_DEBUG( "lx_get_task_list_new_resp: lx_save_task_list_info,user_id = %llu, task_num = %u", lx_get_base()->_userid, lx_num_of_task_in_map());
							lx_save_task_list_info(lx_get_base()->_userid, &(lx_get_manager()->_task_list));
							#endif
						}
					}
				}

				if(p_action->_file_id!=0)
				{
					sd_close_ex(p_action->_file_id);
					p_action->_file_id = 0;
				}
				sd_delete_file(p_action->_file_path);
				SAFE_DELETE(p_action);
				return SUCCESS;
			}
		}
	}
	else   
	{
		EM_LOG_DEBUG( "lx_get_task_list_new_resp errcode = %d", p_action->_error_code);
		/*��ʱ����,���Ի���*/
		if(p_action->_error_code == -1 || p_action->_error_code == 250)
		{
			if(p_action->_retry_num > 0)
			{
				EM_LOG_DEBUG( "error_code = %d, lx_get_task_list_new_req  try again",p_action->_error_code);
				
				ret_val = lx_get_task_list_new_req(&p_action_ls->_req._task_list, &p_action_ls->_resp._action_id, 0, 0);
				if(ret_val != SUCCESS)
				{	
					p_result->_result = ret_val;
					goto ErrHandler;
				}

				if(p_action->_file_id != 0)
				{
					sd_close_ex(p_action->_file_id);
					p_action->_file_id = 0;
				}
				sd_delete_file(p_action->_file_path);	
				SAFE_DELETE(p_action);
				return SUCCESS;
			}
			
		}
		// ��һ������100������ʱʧ�ܻص�
		if( 0 == p_action_ls->_req._task_list._offset)
		{
			// ����������������һ��֮���ٴγ�ʱ װ�ػ����������Ϣ��ֱ�ӻص�֪ͨ����
			#ifdef LX_LIST_CACHE
			lx_load_task_list_info(lx_get_base()->_userid);
			EM_LOG_DEBUG( "lx_get_task_list_new_resp errcode = %d: lx_load_task_list_info,task_num = %u", p_action->_error_code, lx_num_of_task_in_map());
			#endif
			g_geting_task_list = FALSE;
			if(callback!=NULL)
			{
				EM_LOG_DEBUG( "lx_get_task_list_new_resp failed to first: callback to ui, result = %d", p_result->_result);
				callback(p_result);
			}
			if(p_action->_file_id!=0)
			{
				sd_close_ex(p_action->_file_id);
				p_action->_file_id = 0;
			}
			sd_delete_file(p_action->_file_path);
			SAFE_DELETE(p_action);
			return SUCCESS;
		}
		// ����100������֮���͵�����ʧ��ʱֻ�ص�һ��
		else
			goto ErrHandler;
	}
ErrHandler:
	EM_LOG_DEBUG( "lx_get_task_list_new_resp: ErrHandler g_geting_task_list = %d", g_geting_task_list);
	g_geting_task_list = FALSE;
	if(!error_callback_flag)
	{
		if(callback!=NULL)
		{
			EM_LOG_DEBUG( "lx_get_task_list_new_resp ErrHandler: callback to ui, result = %d", p_result->_result);
			callback(p_result);
		}
		error_callback_flag = TRUE;
	}
	lx_clear_task_list_action_map_new();
	if(p_action->_file_id!=0)
	{
		sd_close_ex(p_action->_file_id);
		p_action->_file_id = 0;
	}
	sd_delete_file(p_action->_file_path);
	SAFE_DELETE(p_action);

	return SUCCESS;
}

 _int32 lx_cancel_get_task_list(LX_PT * p_action)
 {
 	EM_LOG_DEBUG("lx_cancel_get_task_list:action_id=%u", p_action->_action_id);
 	g_geting_task_list = FALSE;
 	return lx_cancel_action(p_action);
}
 
////////////////////////////////////////////////////////////////////////////////////////////////

 
////////////////////////////////////////////////////////////////////////////////////////////////
/* ��ȡ���߿ռ���ڻ�����ɾ��������б�
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
_int32 lx_get_overdue_or_deleted_task_list(LX_GET_OD_OR_DEL_TASK_LIST * p_param,_u32 * p_action_id,_int32 priority)
{
	_int32 ret_val = SUCCESS;
	
	EM_LOG_DEBUG( "lx_get_overdue_or_deleted_task_list,g_geting_overdue_task_list=%d,p_param=0x%X",g_geting_overdue_task_list ,p_param);
	
	if(!lx_is_logined()) return -1;
	if(!em_is_net_ok(TRUE)||(sd_get_net_type()==UNKNOWN_NET)) 
	{
		EM_LOG_DEBUG("lx_get_task_list:No network net_type=%u!!",sd_get_net_type());
		return NETWORK_NOT_READY;   /* �����������ɾ���������Ƿ�Ҳ���Ի���?  */
	}
	
	if(g_geting_overdue_task_list)
	{
		return ETM_BUSY;
	}
	
	if(p_param != NULL)
	{
		ret_val = lx_get_overdue_or_deleted_task_list_req(p_param, p_action_id,priority);
	}
	else
		return INVALID_ARGUMENT;

 	return ret_val;
}

/* ��ȡ���߿ռ���ڻ�����ɾ��������б�
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
_int32 lx_get_overdue_or_deleted_task_list_req(LX_GET_OD_OR_DEL_TASK_LIST * p_param,_u32 * p_action_id,_int32 priority)
{
	_int32 ret_val = SUCCESS;
	
	LX_PT_OD_OR_DEL_TASK* p_action = NULL;
	_u32 http_id = 0;
	char jumpkey[LX_JUMPKEY_MAX_SIZE] = {0};
	_u32 jumpkey_len = 0;
	EM_LOG_DEBUG( "lx_get_overdue_or_deleted_task_list:task_type=%d,offset = %d,max_task_num=%d,callback=0x%X",p_param->_task_type, p_param->_page_offset,p_param->_max_task_num,p_param->_callback);

	if(!lx_is_logined()) return -1;

	ret_val = sd_malloc(sizeof(LX_PT_OD_OR_DEL_TASK), (void**) &p_action);
	CHECK_VALUE(ret_val);
	sd_memset(p_action,0x00,sizeof(LX_PT_OD_OR_DEL_TASK));

	p_action->_action._type = LPT_GET_OD_OR_DEL_LS;

	/* ------------- ��������cmd ------------ */
	/* ͷ�� */
	p_action->_req._cmd_header._version = LX_CMD_PROTOCAL_VERSION;
	p_action->_req._cmd_header._seq = lx_get_cmd_protocal_seq();
	// p_action._req._cmd_header._len �������
	p_action->_req._cmd_header._thunder_flag = get_product_flag();
	p_action->_req._cmd_header._compress_flag = 0;
	p_action->_req._cmd_header._cmd_type = LX_CMD_TYPE_HISTORYTASKLIST_REQ;

	if(SUCCESS != lx_get_jumpkey(jumpkey, &jumpkey_len))
	{
		SAFE_DELETE(p_action);
		return LXE_WRONG_REQUEST;
	}	
	p_action->_req._jump_key_len = jumpkey_len;
	sd_memcpy(p_action->_req._jump_key, jumpkey, jumpkey_len);

	/* �û���Ϣ*/
	p_action->_req._userid = lx_get_base()->_userid;
	p_action->_req._vip_level = lx_get_base()->_vip_level;
	/* ------------------------------------------- */

	sd_memcpy(&p_action->_req._list_info, p_param, sizeof(LX_GET_OD_OR_DEL_TASK_LIST));
	
	p_action->_action._req_buffer_len = MAX_POST_REQ_BUFFER_LEN;
	p_action->_action._req_data_len = p_action->_action._req_buffer_len;
	
	ret_val = lx_build_req_get_overdue_or_deleted_task_list(p_action);
	
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}

	ret_val = lx_get_xml_file_store_path(p_action->_action._file_path);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}

	ret_val = lx_post_req((LX_PT *) p_action,&http_id,0);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}
	p_action->_action._action_id = http_id;
	p_action->_action._state = LPS_RUNNING;
	
	lx_add_action_to_list((LX_PT * )p_action);

	*p_action_id = http_id;

	g_geting_overdue_task_list = TRUE;
 	return ret_val;

}

_int32 lx_get_overdue_or_deleted_task_list_resp(LX_PT * p_action)
 {
	_int32 ret_val = SUCCESS;
	LX_PT_OD_OR_DEL_TASK* p_action_ls = (LX_PT_OD_OR_DEL_TASK *)p_action;
	LX_OD_OR_DEL_TASK_LIST_RESULT* p_result = &p_action_ls->_resp;

	EM_LOG_DEBUG( "lx_get_overdue_or_deleted_task_list_resp: errcode = %d ", p_action->_error_code);

	p_result->_action_id = p_action->_action_id;
	p_result->_user_data = p_action_ls->_req._list_info._user_data;
	p_result->_result = p_action->_error_code;
	
	if(p_action->_error_code == SUCCESS)
	{
		if(p_action->_file_id != 0)
		{
			//�ر��ļ�
			sd_close_ex(p_action->_file_id);
			p_action->_file_id = 0;
		}
		ret_val = lx_parse_resp_get_overdue_or_deleted_task_list(p_action_ls);
		EM_LOG_DEBUG( "lx_parse_resp_get_overdue_or_deleted_task_list, ret_val = %d, resp_status = %u", ret_val, p_action->_resp_status);
		ret_val = lx_pt_server_errcode_to_em_errcode(ret_val);
		if(ret_val != SUCCESS) 
		{
			EM_LOG_DEBUG( "lx_pt_server_errcode_to_em_errcode, ret_val = %d", ret_val);
			p_result->_result = ret_val;
			goto ErrHandler;
		}
		
		if( ((p_action->_resp_status == 0)|| (p_action->_resp_status == 200)) && (p_action->_error_code == SUCCESS))
		{
			/*  �ɹ� */
			p_result->_result = 0;
		}
		else
		{
			p_result->_result = p_action->_resp_status+p_action->_error_code;
			sd_assert( p_result->_result!=SUCCESS);
		}
	}
	
ErrHandler:
	
	/* �ص�֪ͨUI */
	p_action_ls->_req._list_info._callback(p_result);
	
	if(p_action->_file_id!=0)
	{
		sd_close_ex(p_action->_file_id);
		p_action->_file_id = 0;
	}

	sd_delete_file(p_action->_file_path);
	
	SAFE_DELETE(p_action);
	
	g_geting_overdue_task_list = FALSE;
	
	return SUCCESS;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* ��ȡ���߿ռ�BT ��������ļ��б�
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
_int32 lx_get_bt_task_file_list(LX_GET_BT_FILE_LIST * p_param,_u32 * p_action_id,_int32 priority)
{
	_int32 ret_val = SUCCESS;
	LX_TASK_INFO_EX * p_task_in_map =  NULL;
	
	EM_LOG_DEBUG( "lx_get_bt_task_file_list,file_type=%d,file_status=%d" ,p_param->_file_type,p_param->_file_status);
	
	if(!lx_is_logined()) return -1;
	if(!em_is_net_ok(TRUE)||(sd_get_net_type()==UNKNOWN_NET)) return NETWORK_NOT_READY;

	p_task_in_map =  lx_get_task_from_map( p_param->_task_id);
	sd_assert(p_task_in_map!=NULL);
	if(p_task_in_map==NULL)
	{
		return INVALID_TASK_ID;
	}
	
	//if(p_info_in_map->_type == LXT_BT_ALL && p_info_in_map->_state ==LXS_RUNNING && map_size(&p_info_in_map->_bt_sub_files)!=0)
	if(p_param->_callback!=NULL&&map_size(&p_task_in_map->_bt_sub_files)!=0)
	{
		LX_FILE_LIST  result;
		LX_FILE_LIST * p_result = &result;
		p_result->_action_id = MAX_U32/2;
		p_result->_task_id = p_param->_task_id;
		p_result->_user_data = p_param->_user_data;
		p_result->_result = 0;
		p_result->_total_file_num = map_size(&p_task_in_map->_bt_sub_files);
		p_result->_file_num = p_result->_total_file_num;

		p_result->_result = lx_get_file_array_from_map(&p_task_in_map->_bt_sub_files,p_param->_file_type,p_param->_file_status,p_param->_offset,p_result->_file_num,&p_result->_file_array);
			
		/* �ص�֪ͨUI */
		p_param->_callback(p_result);
		
		SAFE_DELETE(p_result->_file_array);
	}
	else
	{
		ret_val =  lx_get_bt_task_file_list_req(p_param,p_action_id,priority);
	}
 	return ret_val;
}
_int32 lx_get_bt_task_file_list_req(LX_GET_BT_FILE_LIST * p_param,_u32 * p_action_id,_int32 priority)
{
	_int32 ret_val = SUCCESS;
	LX_PT_BT_LS * p_action =NULL;
	_u32 http_id = 0;
	
	EM_LOG_DEBUG( "lx_get_bt_task_file_list_req,file_type=%d,file_status=%d" ,p_param->_file_type,p_param->_file_status);
	
	if(!lx_is_logined()) return -1;

	
	ret_val = sd_malloc(sizeof(LX_PT_BT_LS),(void**) &p_action);
	CHECK_VALUE(ret_val);
	sd_memset(p_action,0x00,sizeof(LX_PT_BT_LS));

	p_action->_action._type = LPT_BT_LS;
	sd_memcpy(&p_action->_req, p_param,sizeof(LX_GET_BT_FILE_LIST));

	#ifdef ENABLE_LX_XML_ZIP
	p_action->_action._is_compress=TRUE;
	#endif/* ENABLE_LX_XML_ZIP */

	#ifdef ENABLE_LX_XML_AES
	p_action->_action._is_aes=TRUE;
	#endif/* ENABLE_LX_XML_AES */

	if(p_action->_action._is_aes)
	{
		lx_get_aes_key( p_action->_action._aes_key);
	}
	
	//WB_CHECK_SAME_ACTION(p_action);		
	
	p_action->_action._req_buffer_len = MAX_POST_REQ_BUFFER_LEN;
	p_action->_action._req_data_len = p_action->_action._req_buffer_len;
	ret_val = lx_build_req_bt_task_ls(lx_get_base(),p_action);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}

	ret_val = lx_get_xml_file_store_path(p_action->_action._file_path);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}

	ret_val = lx_post_req((LX_PT *) p_action,&http_id,priority);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}
	p_action->_action._action_id = http_id;
	p_action->_action._state = LPS_RUNNING;
	
	lx_add_action_to_list((LX_PT * )p_action);

	*p_action_id = http_id;
	
 	return ret_val;
}
 _int32 lx_get_bt_task_file_list_resp(LX_PT * p_action)
 {
 	_int32 ret_val = SUCCESS;
	LX_PT_BT_LS * p_action_ls = (LX_PT_BT_LS *)p_action;
	LX_FILE_LIST * p_result = &p_action_ls->_resp;
	LX_TASK_INFO_EX * p_task_in_map =  lx_get_task_from_map( p_action_ls->_req._task_id);

	EM_LOG_DEBUG( "lx_get_bt_task_file_list_resp,file_type=%d,file_status=%d,error_code=%d" ,p_action_ls->_req._file_type,p_action_ls->_req._file_status,p_action->_error_code);

	sd_assert(p_task_in_map!=NULL);
	sd_assert(p_task_in_map->_type==LXT_BT_ALL);
	
	p_result->_action_id = p_action->_action_id;
	p_result->_user_data = p_action_ls->_req._user_data;
	p_result->_task_id = p_action_ls->_req._task_id;
	p_result->_result = p_action->_error_code;

	if(p_action->_error_code == SUCCESS)
	{

		if(p_action->_file_id!=0)
		{
			sd_close_ex(p_action->_file_id);
			p_action->_file_id = 0;
		}

		if((p_action_ls->_req._callback==NULL)&&(p_action_ls->_req._file_status==0))
		{
			/* ��MAP ��������������� */
			lx_clear_file_map(&p_task_in_map->_bt_sub_files);
		}
		
		ret_val = lx_parse_resp_bt_task_list(p_action_ls);
		if(ret_val != SUCCESS) 
		{
			EM_LOG_DEBUG( "lx_parse_resp_bt_task_list, ret_val = %d, resp_status = %u", ret_val, p_action->_resp_status);
			p_result->_result = ret_val;
			/*   ��Ӧxml ����ʧ��*/
			goto ErrHandler;
		}
		
		EM_LOG_DEBUG( "lx_get_bt_task_file_list_resp,resp_status=%d,error_code=%d,file_num=%u" ,p_action->_resp_status,p_action->_error_code,p_action_ls->_resp._file_num);
		if( (p_action->_resp_status == 0) || (p_action->_resp_status == 200) )
		{
			/* ����xml �ɹ� */
			if( p_action->_error_code == SUCCESS )
			{
				if((p_action_ls->_resp._file_num!=0)&&(p_action_ls->_req._callback!=NULL))
				{
					ret_val = lx_get_file_array_from_map(&p_task_in_map->_bt_sub_files,p_action_ls->_req._file_type,p_action_ls->_req._file_status,p_action_ls->_req._offset,p_action_ls->_resp._file_num,&p_action_ls->_resp._file_array);
					if(ret_val!=SUCCESS)
					{
						p_result->_result = ret_val;
						/*   �ڴ�����ʧ��*/
						goto ErrHandler;
					}
				
				}
				/* ls �ɹ� */
				p_result->_result = 0;
			}
			else
				p_result->_result = p_action->_error_code;
		}	
		else
		{
			// ���������صĽ��
			p_result->_result = p_action->_resp_status;
			sd_assert( p_result->_result!=SUCCESS);
		}

	}
	else
	{
#ifdef ENABLE_TEST_NETWORK
		em_set_uid(lx_get_manager()->_base._userid);
		//���������ϱ�
		sd_create_task(em_ipad_interface_server_report_handler, 0, (void*)p_action->_error_code, NULL);
#endif
	}
	
ErrHandler:

	/* �ص�֪ͨUI */
	if(p_action_ls->_req._callback!=NULL)
	{
		p_action_ls->_req._callback(p_result);
	}
	
	SAFE_DELETE(p_result->_file_array);
	
	if(p_action->_file_id!=0)
	{
		sd_close_ex(p_action->_file_id);
		p_action->_file_id = 0;
	}

	sd_delete_file(p_action->_file_path);
	
	SAFE_DELETE(p_action);

	return SUCCESS;
}

 _int32 lx_cancel_get_bt_task_file_list(LX_PT * p_action)
 {
 	_int32 ret_val = SUCCESS;
	EM_LOG_DEBUG( "lx_cancel_get_bt_task_file_list" );

	ret_val = lx_cancel_action(p_action);
	return ret_val;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* ��ȡ��Ƶ��ͼ,��ͼΪjpg��ʽ
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
_int32 lx_get_screenshot(LX_GET_SCREENSHOT * p_param,_u32 * p_action_id)
{
 	_int32 ret_val = SUCCESS;
	LX_PT_SS * p_action =NULL;
	_u32 http_id = 0;
	
	EM_LOG_DEBUG( "lx_get_screenshot" );
	
	if(!lx_is_logined()) return -1;
	if(!em_is_net_ok(TRUE)||(sd_get_net_type()==UNKNOWN_NET)) return NETWORK_NOT_READY;

	ret_val = sd_malloc(sizeof(LX_PT_SS),(void**) &p_action);
	CHECK_VALUE(ret_val);
	sd_memset(p_action,0x00,sizeof(LX_PT_SS));

	p_action->_action._type = LPT_SCREEN_SHOT;
	sd_memcpy(&p_action->_req, p_param,sizeof(LX_GET_SCREENSHOT));

	ret_val = sd_malloc(p_param->_file_num*CID_SIZE,(void**) &p_action->_req._gcid_array);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}
	sd_memset(p_action->_req._gcid_array,0x00,p_param->_file_num*CID_SIZE);
	sd_memcpy(p_action->_req._gcid_array,p_param->_gcid_array,p_param->_file_num*CID_SIZE);

	p_action->_resp._file_num = p_param->_file_num;
	p_action->_resp._gcid_array = p_action->_req._gcid_array;
	p_action->_resp._is_big = p_param->_is_big;

	sd_strncpy(p_action->_resp._store_path, p_param->_store_path,MAX_FILE_PATH_LEN-1);
	
	ret_val = sd_test_path_writable(p_action->_resp._store_path);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action->_req._gcid_array);
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}

	p_action->_req._store_path = p_action->_resp._store_path;
	
	ret_val = sd_malloc(p_param->_file_num*sizeof(_int32),(void**) &p_action->_resp._result_array);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action->_req._gcid_array);
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}
	/* ȫ����Ϊ-1 */
	sd_memset(p_action->_resp._result_array,0xFF,p_param->_file_num*sizeof(_int32));

	p_action->_need_dl_num = p_param->_file_num;

	/* ���Ŀ���ļ��Ѵ��ڣ�ֱ�ӳɹ�! */
	if(!p_action->_req._overwrite)
	{
		//Error # 17: File exists
		ret_val = lx_check_if_pic_exist((LX_PT * )p_action);
		if(ret_val!=SUCCESS)
		{
			SAFE_DELETE(p_action->_req._gcid_array);
			SAFE_DELETE(p_action);
			return ret_val;
		}
	}
	sd_assert(p_action->_need_dl_num!=0);
	sd_assert(p_action->_need_dl_num<=p_param->_file_num);
	
	#ifdef ENABLE_LX_XML_ZIP
	p_action->_action._is_compress=TRUE;
	#endif/* ENABLE_LX_XML_ZIP */

	#ifdef ENABLE_LX_XML_AES
	p_action->_action._is_aes=TRUE;
	#endif/* ENABLE_LX_XML_AES */

	if(p_action->_action._is_aes)
	{
		lx_get_aes_key( p_action->_action._aes_key);
	}
	
	//WB_CHECK_SAME_ACTION(p_action);
		
	
	p_action->_action._req_buffer_len = MAX_POST_REQ_BUFFER_LEN;
	p_action->_action._req_data_len = p_action->_action._req_buffer_len;
	ret_val = lx_build_req_screenshot(lx_get_base(),p_action);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action->_resp._result_array);
		SAFE_DELETE(p_action->_req._gcid_array);
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}

	ret_val = lx_get_xml_file_store_path(p_action->_action._file_path);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action->_resp._result_array);
		SAFE_DELETE(p_action->_req._gcid_array);
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}

	ret_val = lx_post_req((LX_PT *) p_action,&http_id,-1);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action->_resp._result_array);
		SAFE_DELETE(p_action->_req._gcid_array);
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}
	p_action->_action._action_id = http_id;
	p_action->_action._state = LPS_RUNNING;
	
	lx_add_action_to_list((LX_PT * )p_action);

	*p_action_id = http_id;
	
 	return ret_val;
}
 _int32 lx_get_screenshot_resp(LX_PT * p_action)
 {
 	_int32 ret_val = SUCCESS;
	LX_PT_SS * p_action_ls = (LX_PT_SS *)p_action;
	//LX_GET_SCREENSHOT * p_req = &p_action_ls->_req;			
	LX_SCREENSHOT_RESUTL * p_result = &p_action_ls->_resp;
	
	EM_LOG_DEBUG( "lx_get_screenshot_resp: errcode= %d", p_action->_error_code);
	
	p_result->_action_id = p_action->_action_id;
	p_result->_user_data = p_action_ls->_req._user_data;

	if(p_action->_error_code == SUCCESS)
	{

		if(p_action->_file_id!=0)
		{
			sd_close_ex(p_action->_file_id);
			p_action->_file_id = 0;
		}

		ret_val = lx_parse_resp_screenshot(p_action_ls);
		if(ret_val != SUCCESS) 
		{
			EM_LOG_DEBUG( "lx_parse_resp_screenshot: ret_val= %d, p_action->_error_code", ret_val, p_action->_error_code);
			lx_set_screenshot_result(p_action,ret_val);
			/*   ��Ӧxml ����ʧ��*/
			goto ErrHandler;
		}
		
		if( (p_action->_resp_status == 0) || (p_action->_resp_status == 200) )
		{
			if( p_action->_error_code == SUCCESS )
			{
				/* �Ƿ����е�gcid �ֶζ���������? */
				if(lx_check_is_all_gcid_parse_failed(p_action))
				{
					goto ErrHandler;
				}
				
				/* �����߷��������� �ɹ���������Ҫ��ͼƬ���ػ���! */
				ret_val = lx_dowanload_pic(p_action);
				if(ret_val != SUCCESS) 
				{
					lx_set_screenshot_result(p_action,ret_val);
					/*   ��Ӧxml ����ʧ��*/
					goto ErrHandler;
				}
				
				sd_delete_file(p_action->_file_path);
				
				/* �ݲ��ص�֪ͨ */
				return SUCCESS;
			}
			else
				lx_set_screenshot_result(p_action, p_action->_error_code);
		}
		else
		{
			lx_set_screenshot_result(p_action,p_action->_resp_status);
		}
	}
	else
	{
		lx_set_screenshot_result(p_action, p_action->_error_code);
	}
	
ErrHandler:

	/* �ص�֪ͨUI */
	//typedef _int32 ( *LX_GET_SCREENSHOT_CALLBACK)(LX_SCREENSHOT_RESUTL * p_screenshot_result);
	p_action_ls->_req._callback(p_result);

	if(p_action->_file_id!=0)
	{
		sd_close_ex(p_action->_file_id);
		p_action->_file_id = 0;
	}

	sd_delete_file(p_action->_file_path);
	
	SAFE_DELETE(p_action_ls->_dl_array);
	SAFE_DELETE(p_action_ls->_resp._result_array);
	SAFE_DELETE(p_action_ls->_req._gcid_array);
	SAFE_DELETE(p_action);

	return SUCCESS;
 }
_int32 lx_cancel_screenshot(LX_PT * p_action)
{
 	_int32 ret_val = SUCCESS,index = 0;
	LX_PT_SS * p_action_ss = (LX_PT_SS *)p_action;
	LX_DL_FILE * p_dl = NULL;

	EM_LOG_DEBUG( "lx_cancel_download_pic" );

	ret_val = eti_http_close(p_action->_action_id);
	
	lx_remove_action_from_list(p_action);
	
	if(p_action->_file_id!=0)
	{
		sd_close_ex(p_action->_file_id);
		p_action->_file_id = 0;
	}

	/* ɾ�����Ʒ */
	sd_delete_file(p_action->_file_path);
	
	if(0!=p_action_ss->_dling_num)
	{
		/* ɾ����������ͼƬ�Ĳ��� */
		p_dl = p_action_ss->_dl_array;
		for(index=0;index<p_action_ss->_need_dl_num;index++)
		{
			if(p_dl->_action._action_id!=0)
			{
				lx_cancel_download_pic((LX_PT * )p_dl,FALSE);
			}
			p_dl++;
		}
	}
	SAFE_DELETE(p_action_ss->_dl_array);
	SAFE_DELETE(p_action_ss->_resp._result_array);
	SAFE_DELETE(p_action_ss->_req._gcid_array);
	SAFE_DELETE(p_action_ss);
	
	return SUCCESS;
}

_int32 lx_dowanload_pic(LX_PT * p_action)
{
 	_int32 ret_val = SUCCESS,i = 0;
	LX_PT_SS * p_action_ss = (LX_PT_SS *)p_action;
	LX_DL_FILE * p_dl = p_action_ss->_dl_array;

	for(i=0;i<p_action_ss->_need_dl_num;i++)
	{
		if(p_dl->_action._error_code == SUCCESS)
		{
			p_dl->_action._type = LPT_DL_PIC;
			p_dl->_ss_action = p_action;
			lx_set_section_type(p_dl->_section_type);
			ret_val = lx_get_file_req((LX_PT*)p_dl);
			if(ret_val == SUCCESS)
			{
				lx_add_action_to_list((LX_PT * )p_dl);
				p_action_ss->_dling_num++;
			}
		}
		p_dl++;
	}

	if(p_action_ss->_dling_num==0)
	{
		return LXE_DOWNLOAD_PIC;
	}
	
	return SUCCESS;
}

 _int32 lx_dowanload_pic_resp(LX_PT * p_action)
 {
	LX_DL_FILE * p_action_dl = (LX_DL_FILE *)p_action;
	LX_PT_SS * p_action_ss = (LX_PT_SS *)p_action_dl->_ss_action;
	//LX_GET_SCREENSHOT * p_req = &p_action_ss->_req;			
	LX_SCREENSHOT_RESUTL * p_result = &p_action_ss->_resp;

	p_action_ss->_dled_num++;
	
	if(p_action->_file_id!=0)
	{
		sd_close_ex(p_action->_file_id);
		p_action->_file_id = 0;
	}
	
	if(p_action->_error_code == SUCCESS)
	{
		/* ͼƬ���سɹ� */
		*(p_result->_result_array+p_action_dl->_gcid_index) = SUCCESS;
	}
	else
	{
		/* ͼƬ����ʧ�� */
		*(p_result->_result_array+p_action_dl->_gcid_index) = p_action->_error_code;
		sd_delete_file(p_action->_file_path);
	}
	
	if(p_action_ss->_dled_num==p_action_ss->_dling_num)
	{
		/* �ص�֪ͨUI */
		//typedef _int32 ( *LX_GET_SCREENSHOT_CALLBACK)(LX_SCREENSHOT_RESUTL * p_screenshot_result);
		p_action_ss->_req._callback(p_result);
		
		SAFE_DELETE(p_action_ss->_dl_array);
		SAFE_DELETE(p_action_ss->_resp._result_array);
		SAFE_DELETE(p_action_ss->_req._gcid_array);
		SAFE_DELETE(p_action_ss);
	}
	return SUCCESS;
 }

_int32 lx_cancel_download_pic(LX_PT * p_action,BOOL need_cancel_ss)
{
 	_int32 ret_val = SUCCESS;
	LX_DL_FILE * p_action_dl = (LX_DL_FILE *)p_action;
	LX_PT_SS * p_action_ss = (LX_PT_SS *)p_action_dl->_ss_action;

	EM_LOG_DEBUG( "lx_cancel_download_pic" );

	p_action_ss->_dled_num++;
	
	ret_val = eti_http_close(p_action->_action_id);
	
	lx_remove_action_from_list(p_action);
	
	if(p_action->_file_id!=0)
	{
		sd_close_ex(p_action->_file_id);
		p_action->_file_id = 0;
	}

	/* ɾ�����Ʒ */
	sd_delete_file(p_action->_file_path);

	if(need_cancel_ss && p_action_ss->_dled_num==p_action_ss->_dling_num)
	{
		SAFE_DELETE(p_action_ss->_dl_array);
		SAFE_DELETE(p_action_ss->_resp._result_array);
		SAFE_DELETE(p_action_ss->_req._gcid_array);
		SAFE_DELETE(p_action_ss);
	}
	return SUCCESS;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* ��ȡ��Ƶ�ļ����Ƶ㲥URL
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
_int32 lx_get_vod_url(LX_GET_VOD * p_param,_u32 * p_action_id)
{
 	_int32 ret_val = SUCCESS;
	LX_PT_VOD_URL * p_action =NULL;
	_u32 http_id = 0;

	EM_LOG_DEBUG( "lx_get_vod_url" );

	if(!lx_is_logined()) return -1;
	if(!em_is_net_ok(TRUE)||(sd_get_net_type()==UNKNOWN_NET)) return NETWORK_NOT_READY;

	ret_val = sd_malloc(sizeof(LX_PT_VOD_URL),(void**) &p_action);
	CHECK_VALUE(ret_val);
	sd_memset(p_action,0x00,sizeof(LX_PT_VOD_URL));

	p_action->_action._type = LPT_VOD_URL;
	sd_memcpy(&p_action->_req, p_param,sizeof(LX_GET_VOD));
	if(p_action->_req._video_type!=2&&p_action->_req._video_type!=4)
	{
		p_action->_req._video_type = 1;
	}

	#ifdef ENABLE_LX_XML_ZIP
	p_action->_action._is_compress=TRUE;
	#endif/* ENABLE_LX_XML_ZIP */

	#ifdef ENABLE_LX_XML_AES
	p_action->_action._is_aes=TRUE;
	#endif/* ENABLE_LX_XML_AES */

	if(p_action->_action._is_aes)
	{
		lx_get_aes_key( p_action->_action._aes_key);
	}
	
	//WB_CHECK_SAME_ACTION(p_action);		
	
	p_action->_action._req_buffer_len = MAX_POST_REQ_BUFFER_LEN;
	p_action->_action._req_data_len = p_action->_action._req_buffer_len;
	ret_val = lx_build_req_vod_url(lx_get_base(),p_action);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}

	ret_val = lx_get_xml_file_store_path(p_action->_action._file_path);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}

	ret_val = lx_post_req((LX_PT *) p_action,&http_id,-1);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}
	p_action->_action._action_id = http_id;
	p_action->_action._state = LPS_RUNNING;
	
	lx_add_action_to_list((LX_PT * )p_action);

	*p_action_id = http_id;
	
 	return ret_val;
}
 _int32 lx_get_vod_url_resp(LX_PT * p_action)
 {
	_int32 ret_val = SUCCESS;
	LX_PT_VOD_URL* p_action_ls = (LX_PT_VOD_URL *)p_action;
	LX_GET_VOD_RESULT* p_result = &p_action_ls->_resp;

	EM_LOG_DEBUG( "lx_get_vod_url_resp: errcode= %d", p_action->_error_code);
	
	p_result->_action_id = p_action->_action_id;
	p_result->_user_data = p_action_ls->_req._user_data;

	p_result->_result = p_action->_error_code;

	if(p_action->_error_code == SUCCESS)
	{

		if(p_action->_file_id!=0)
		{
			sd_close_ex(p_action->_file_id);
			p_action->_file_id = 0;
		}
		ret_val = lx_parse_resp_vod_url(p_action_ls);
		if(ret_val != SUCCESS) 
		{
			EM_LOG_DEBUG( "lx_parse_resp_vod_url: ret_val= %d, p_action->_error_code", ret_val, p_action->_error_code);
			p_result->_result = ret_val;
			/*   ��Ӧxml ����ʧ��*/
			goto ErrHandler;
		}
		
		if( (p_action->_resp_status == 0) || (p_action->_resp_status == 200) )
		{
			if(p_action->_error_code == SUCCESS )
			{
				/* ����xml �ɹ� */
				p_result->_result = 0;
				/* ����ɹ�������CID������ */
				sd_memcpy(p_result->_cid, p_action_ls->_req._cid, 20);
				sd_memcpy(p_result->_gcid, p_action_ls->_req._gcid, 20);
				
				EM_LOG_DEBUG( "get_normal_url: normal_size = %llu, url = %s", p_result->_normal_size, p_result->_normal_vod_url);
				EM_LOG_DEBUG( "get_high_url: high_size = %llu, url = %s ", p_result->_high_size, p_result->_high_vod_url);
				EM_LOG_DEBUG( "get_fluency_url1: fluency_size_1 = %llu, url = %s", p_result->_fluency_size_1, p_result->_fluency_vod_url_1);
				EM_LOG_DEBUG( "get_fluency_url2: fluency_size_2 = %llu, url = %s", p_result->_fluency_size_2, p_result->_fluency_vod_url_2);

				if( (sd_strlen(p_result->_high_vod_url) == 0) && (sd_strlen(p_result->_normal_vod_url) == 0) &&
					(sd_strlen(p_result->_fluency_vod_url_1) == 0) && (sd_strlen(p_result->_fluency_vod_url_2) == 0))
				{
					p_result->_result = LXE_GET_VOD_URL_NULL;
				}
			}
			else
				p_result->_result = p_action->_error_code;
		}
		else
		{
			// ���������صĽ��
			p_result->_result = p_action->_resp_status;
			sd_assert( p_result->_result!=SUCCESS);
		}
	}
	else
	{
#ifdef ENABLE_TEST_NETWORK
		em_set_uid(lx_get_manager()->_base._userid);
		//���������ϱ�
		sd_create_task(em_ipad_interface_server_report_handler, 0, (void*)p_action->_error_code, NULL);
#endif
	}
ErrHandler:

	/* �ص�֪ͨUI */
	if( NULL != p_action_ls->_req._callback_fun)
		p_action_ls->_req._callback_fun(p_result);
	EM_LOG_DEBUG( "lx_get_vod_url_resp end: _result= %d", p_result->_result);
	
	SAFE_DELETE(p_result->_fp_array);
	
	if(p_action->_file_id!=0)
	{
		sd_close_ex(p_action->_file_id);
		p_action->_file_id = 0;
	}

	sd_delete_file(p_action->_file_path);
	
	SAFE_DELETE(p_action);

	return SUCCESS;

	return ret_val;
}

 _int32 lx_cancel_get_vod_url(LX_PT * p_action)
 {
 	_int32 ret_val = SUCCESS;
	EM_LOG_DEBUG( "lx_cancel_get_vod_url" );

	ret_val = lx_cancel_action(p_action);

	return ret_val;
}

_int32 lx_create_task(LX_CREATE_TASK_INFO * p_param, _u32 * p_action_id, _u32 retry_num)
{
	_int32 ret_val = SUCCESS;
	LX_PT_COMMIT_TASK* p_action = NULL;
	_u32 http_id = 0;
	char jumpkey[LX_JUMPKEY_MAX_SIZE] = {0};
	_u32 jumpkey_len = 0;
	
	EM_LOG_DEBUG( "lx_create_task" );

	if(!lx_is_logined()) return -1;
	if(!em_is_net_ok(TRUE)||(sd_get_net_type()==UNKNOWN_NET)) return NETWORK_NOT_READY;
	
	sd_assert(sd_strncmp(p_param->_url, "magnet:?", sd_strlen("magnet:?"))!=0);

	/* ��������Ч�� */
	if(sd_strlen(p_param->_url)!=0)
	{
		EM_DOWNLOADABLE_URL dl_url;
		if(sd_strncmp(p_param->_url, "ed2k://", sd_strlen("ed2k://"))==0)
		{
			ET_ED2K_LINK_INFO info;
			if(sd_strncmp(p_param->_url, "ed2k://%7", sd_strlen("ed2k://%7"))==0)
			{
				em_replace_7c(p_param->_url);
			}
			
			if(p_param->_url[sd_strlen(p_param->_url)-1]!=0x2F)
			{
				/* ��¿���ӹ���,��Ҫ�ض� */
				char * end_pos = sd_strrchr(p_param->_url, '|');
				if(end_pos==NULL) return INVALID_URL;
				*(end_pos+1) = 0x2F;
				*(end_pos+2) = '\0';
			}
			
			ret_val = eti_extract_ed2k_url(p_param->_url, &info);
			if(ret_val!=SUCCESS) return ret_val;
			if(sd_strlen(p_param->_task_name)==0)
			{
				char str_buffer[MAX_FILE_NAME_BUFFER_LEN]={0};
				_u32 buffer_len = MAX_FILE_NAME_BUFFER_LEN;
				sd_strncpy(p_param->_task_name,info._file_name, MAX_FILE_PATH_LEN-1);
				ret_val = sd_any_format_to_utf8(p_param->_task_name, sd_strlen(p_param->_task_name), str_buffer,&buffer_len);
				sd_assert(ret_val!=-1);
				if(ret_val>0)
				{
					sd_strncpy(p_param->_task_name, str_buffer, MAX_FILE_PATH_LEN-1);
				}
			}
			dl_url._type = UT_EMULE;
		}
		else
		if(sd_strncmp(p_param->_url, "thunder://", sd_strlen("thunder://"))==0)
		{
			ret_val = SUCCESS;
			dl_url._type = UT_THUNDER;
		}
		else
		{
			URL_OBJECT url_o;
			ret_val = sd_url_to_object(p_param->_url,sd_strlen(p_param->_url),&url_o);
			if(ret_val!=SUCCESS) return ret_val;
			dl_url._type = UT_HTTP;
		}

		sd_strncpy(dl_url._url,p_param->_url,MAX_URL_LEN-1);
		/*
		ret_val = lx_get_downloadable_url_status_in_lixian(&dl_url);
		if(ret_val==SUCCESS&&dl_url._status==EUS_IN_LIXIAN)
		{
			// �����Ѵ���
			EM_LOG_DEBUG( "lx_create_task by url, TASK_ALREADY_EXIST, url = %s", p_param->_url);
			return TASK_ALREADY_EXIST;
		}
		*/
	}
	else
	{
		EM_EIGENVALUE cid_eigenvalue = {0} ;
		_u64 old_task_id = 0;
		
		if(!sd_is_cid_valid(p_param->_cid))
			return INVALID_ARGUMENT;

		cid_eigenvalue._type = TT_TCID;
		str2hex((char*)p_param->_cid, CID_SIZE, cid_eigenvalue._eigenvalue, CID_SIZE*2);
		/* //��������������ͬ���񣬲���Ҫ���ع�����ͬ����������
		ret_val = lx_get_task_id_by_eigenvalue( &cid_eigenvalue ,&old_task_id);
	 	if(ret_val==SUCCESS&&old_task_id!=0)
	 	{
			// �����Ѵ���
			EM_LOG_DEBUG( "lx_create_task by cid, TASK_ALREADY_EXIST, cid_eigenvalue = %s, task_id = %llu", cid_eigenvalue._eigenvalue, old_task_id);
			return TASK_ALREADY_EXIST;
	 	}
		 */
	}

	if(sd_strlen(p_param->_task_name)==0)
	{
		return INVALID_ARGUMENT;
	}
	
	ret_val = sd_malloc(sizeof(LX_PT_COMMIT_TASK), (void**) &p_action);
	CHECK_VALUE(ret_val);
	sd_memset(p_action,0x00,sizeof(LX_PT_COMMIT_TASK));

	p_action->_action._type = LPT_CRE_TASK;

	/* ------------- ��������cmd ------------ */
	/* ͷ�� */
	p_action->_req._cmd_header._version = LX_CMD_PROTOCAL_VERSION;
	p_action->_req._cmd_header._seq = lx_get_cmd_protocal_seq();
	// p_action._req._cmd_header._len �������
	p_action->_req._cmd_header._thunder_flag = 0x7000000;//get_product_flag();
	p_action->_req._cmd_header._compress_flag = 0;
	p_action->_req._cmd_header._cmd_type = LX_CMD_TYPE_NEW_COMMIT_REQ;

	if(SUCCESS != lx_get_jumpkey(jumpkey, &jumpkey_len))
	{
		SAFE_DELETE(p_action);
		return LXE_WRONG_REQUEST;
	}	
	p_action->_req._jump_key_len = jumpkey_len;
	sd_memcpy(p_action->_req._jump_key, jumpkey, jumpkey_len);

	/* �û���Ϣ*/
	p_action->_req._userid = lx_get_base()->_userid;
	p_action->_req._vip_level = lx_get_base()->_vip_level;
	/* ------------------------------------------- */

	sd_memcpy(&p_action->_req._task_info, p_param, sizeof(LX_CREATE_TASK_INFO));
	p_action->_action._retry_num = retry_num;
	p_action->_action._req_buffer_len = MAX_POST_REQ_BUFFER_LEN;
	p_action->_action._req_data_len = p_action->_action._req_buffer_len;
	
	ret_val = lx_build_req_commit_task(p_action);
	
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}

	ret_val = lx_get_xml_file_store_path(p_action->_action._file_path);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}

	ret_val = lx_post_req((LX_PT *) p_action,&http_id,0);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}
	p_action->_action._action_id = http_id;
	p_action->_action._state = LPS_RUNNING;
	
	lx_add_action_to_list((LX_PT * )p_action);

	*p_action_id = http_id;
	
 	return ret_val;
	
}

_int32 lx_create_task_resp(LX_PT * p_action)
 {
	_int32 ret_val = SUCCESS;
	LX_PT_COMMIT_TASK* p_action_ls = (LX_PT_COMMIT_TASK *)p_action;
	LX_CREATE_TASK_RESULT* p_result = &p_action_ls->_resp;

	EM_LOG_DEBUG( "lx_create_task_resp, error_code = %d", p_action->_error_code);

	p_result->_action_id = p_action->_action_id;
	p_result->_user_data = p_action_ls->_req._task_info._user_data;
	p_result->_result = p_action->_error_code;
	
	if(p_action->_error_code == SUCCESS)
	{
		if(p_action->_file_id!=0)
		{
			//�ر��ļ�
			sd_close_ex(p_action->_file_id);
			p_action->_file_id = 0;
		}
	#ifdef LX_CREATE_LIXIAN_TASK
		LX_TASK_INFO_EX *task_info = NULL;
		lx_malloc_ex_task(&task_info);
		ret_val = lx_parse_resp_commit_task_info(p_action_ls, task_info);
		EM_LOG_DEBUG( "lx_parse_resp_commit_task_info, ret_val = %d, resp_status = %u", ret_val, p_action->_resp_status);
	#else
		ret_val = lx_parse_resp_commit_task(p_action_ls);
		EM_LOG_DEBUG( "lx_parse_resp_commit_task, ret_val = %d, resp_status = %u", ret_val, p_action->_resp_status);
	#endif
		ret_val = lx_pt_server_errcode_to_em_errcode(ret_val);
		if(ret_val != SUCCESS) 
		{
			EM_LOG_DEBUG( "lx_create_task_resp: lx_pt_server_errcode_to_em_errcode: ret_val= %d", ret_val);
			p_result->_result = ret_val;
			goto ErrHandler;
		}
		
		if( ((p_action->_resp_status == 0)|| (p_action->_resp_status == 200)) && (p_action->_error_code == SUCCESS))
		{
			/* ls �ɹ� */
			p_result->_result = 0;
			lx_set_user_lixian_info((_u32)p_result->_max_task_num,(_u32)p_result->_current_task_num,p_result->_max_space,p_result->_available_space);
			
			/* ����URL ��״̬ */
			if(sd_strlen(p_action_ls->_req._task_info._url)!=0)
			{
				lx_set_downloadable_url_status_to_lixian(p_action_ls->_req._task_info._url);
			}
			
			/* ���������б� */
		#ifdef LX_CREATE_LIXIAN_TASK
			lx_add_new_create_task_to_map(task_info);
			//lx_add_task_to_map(task_info);
		#endif
			
		}
		else
		{
			p_result->_result = p_action->_resp_status;
			sd_assert( p_result->_result!=SUCCESS);
		}
	}
	else
	{
		/*���Ի��� (������ʱ����)*/
		if(p_action->_retry_num > 0)
		{
			EM_LOG_DEBUG( "error_code = %d, lx_create_task  try again",p_action->_error_code);
			
			ret_val = lx_create_task(&p_action_ls->_req._task_info, &p_action_ls->_resp._action_id, 0);
			if(ret_val != SUCCESS)
			{	
				p_result->_result = ret_val;
				goto ErrHandler;
			}

			if(p_action->_file_id != 0)
			{
				sd_close_ex(p_action->_file_id);
				p_action->_file_id = 0;
			}
			sd_delete_file(p_action->_file_path);	
			SAFE_DELETE(p_action);
			return SUCCESS;
		}
			
	}
ErrHandler:

	/* �ص�֪ͨUI */
	p_action_ls->_req._task_info._callback_fun(p_result);
	
	if(p_action->_file_id!=0)
	{
		sd_close_ex(p_action->_file_id);
		p_action->_file_id = 0;
	}

	sd_delete_file(p_action->_file_path);
	
	SAFE_DELETE(p_action);

	return SUCCESS;
}
_int32 lx_correct_kankan_url(LX_CREATE_TASK_INFO * p_create_task_info)
{
	EM_LOG_DEBUG( "lx_correct_kankan_url,%s",p_create_task_info->_url);
	if(sd_strlen(p_create_task_info->_url)!=0&& 
		(p_create_task_info->_url == sd_strstr(p_create_task_info->_url,"http://pubnet.sandai.net:8080/0/",0)))
	{
		char * p_mark1 = sd_strstr(p_create_task_info->_url,"/200000/0/4afb9/0/0/5e812e6/0/",0);
		char * p_mark2 = sd_strstr(p_create_task_info->_url,"/0/0/0/0/0/0/",0);
		if(p_mark1!=NULL ||p_mark2!=NULL)
		{
			_int32 ret_val = SUCCESS;
			char file_name[MAX_FILE_NAME_LEN] = {0};
			EM_LOG_ERROR( "lx_correct_kankan_url,find wrong URL!fs=%llu,fname=%s",p_create_task_info->_file_size,p_create_task_info->_task_name);
			
#ifdef _LOGGER
			//if(sd_is_cid_valid(p_create_task_info->_gcid))
			{
				str2hex((char*)p_create_task_info->_gcid, CID_SIZE, file_name, CID_SIZE*2);
				EM_LOG_ERROR("lx_correct_kankan_url:gcid=%s",file_name);
			}
			
			//if(sd_is_cid_valid(p_create_task_info->_cid))
			{
				str2hex((char*)p_create_task_info->_cid, CID_SIZE, file_name, CID_SIZE*2);
				EM_LOG_ERROR("lx_correct_kankan_url:cid=%s",file_name);
			}
#endif /* _LOGGER */
			/*  ����ʱƴ���㷨�����URL ,����ؼ����� */
			ret_val = sd_parse_kankan_vod_url(p_create_task_info->_url,sd_strlen(p_create_task_info->_url) ,p_create_task_info->_gcid,p_create_task_info->_cid, &p_create_task_info->_file_size,file_name);
			CHECK_VALUE(ret_val);

			/* url ���� */
			p_create_task_info->_url[0] = '\0';
		}
		
	}
	return SUCCESS;
}

_int32 lx_create_task_again(_u64 task_id, void* user_data ,LX_CREATE_TASK_CALLBACK callback, _u32 * p_action_id)
{
	_int32 ret_val = SUCCESS;

	LX_TASK_INFO_EX* p_task_info = NULL;
	EM_LOG_DEBUG( "lx_create_task_again" );

	if(!lx_is_logined()) return -1;
	if(!em_is_net_ok(TRUE)||(sd_get_net_type()==UNKNOWN_NET)) return NETWORK_NOT_READY;

	p_task_info = lx_get_task_from_map(task_id);
	if(p_task_info == NULL)
	{
		EM_LOG_DEBUG( "lx_create_task_again, INVALID_TASK_ID, task_id = %llu", task_id);
		return INVALID_TASK_ID;
	}
	EM_LOG_DEBUG( "lx_create_task_again, task_type = %d", p_task_info->_type);
	switch(p_task_info->_type)
	{
		// ����url����
		case LXT_HTTP:
		case LXT_FTP:
		case LXT_UNKNOWN:
		{	
			LX_CREATE_TASK_INFO url_create_task = {0};
			URL_OBJECT url_o = {0};
			// ���ȼ���Ƿ���_origin_url
			if( sd_strlen(p_task_info->_origin_url) > 0)
			{	
				// ���_origin_url�Ƿ����
				ret_val = sd_url_to_object(p_task_info->_origin_url, sd_strlen(p_task_info->_origin_url), &url_o);
				if(ret_val == SUCCESS)
					sd_strncpy(url_create_task._url, p_task_info->_origin_url, MAX_URL_LEN-1);
				else
				{
					// ���lixian_url�Ƿ����
					if( sd_strlen(p_task_info->_url) > 0)
					{
						ret_val = sd_url_to_object(p_task_info->_url, sd_strlen(p_task_info->_url), &url_o);
						if(ret_val == SUCCESS)
							sd_strncpy(url_create_task._url, p_task_info->_url, MAX_URL_LEN-1);
						else
						{
							return INVALID_URL;
						}	
					}
				}
			}
			else
			{
				// ���lixian_url�Ƿ����
				if( sd_strlen(p_task_info->_url) > 0)
				{
					ret_val = sd_url_to_object(p_task_info->_url, sd_strlen(p_task_info->_url), &url_o);
					if(ret_val == SUCCESS)
						sd_strncpy(url_create_task._url, p_task_info->_url, MAX_URL_LEN-1);
					else
					{
						return INVALID_URL;
					}	
				}
				else
				{
					return INVALID_URL;
				}		
			}
			
			sd_strncpy(url_create_task._task_name, p_task_info->_name, sd_strlen(p_task_info->_name));
			url_create_task._is_auto_charge = TRUE;
			url_create_task._user_data = user_data;
			url_create_task._callback_fun = callback;

			/* �ɰ汾��ƴ��kankan url��ʱ���㷨����,�ᵼ���������������޽���,��Ҫ������URL ! By ZYQ @ 20130517 */
			lx_correct_kankan_url(&url_create_task);
			
			ret_val = lx_create_task(&url_create_task, p_action_id, 1);
			if(SUCCESS != ret_val)
			{	
				return ret_val;
			}	
			break;
		}
		
		// ����emule����
		case LXT_EMULE:
		{
			LX_CREATE_TASK_INFO emule_create_task = {0};
			sd_strncpy(emule_create_task._task_name, p_task_info->_name, MAX_FILE_PATH_LEN - 1);
			emule_create_task._is_auto_charge = TRUE;
			emule_create_task._user_data = user_data;
			emule_create_task._callback_fun = callback;	
			if(sd_strlen(p_task_info->_origin_url) > 0)
				sd_strncpy(emule_create_task._url, p_task_info->_origin_url, MAX_URL_LEN-1);
			else
				return INVALID_URL;
			ret_val = lx_create_task(&emule_create_task, p_action_id, 1);
			if(SUCCESS != ret_val)
			{	
				return ret_val;
			}
			break;
		}
		
		// ����BT����
		case LXT_BT_ALL:	
		{
			LX_CREATE_BT_TASK_INFO bt_create_task = {0};
			bt_create_task._title = p_task_info->_name;
			// �ѹ��������б��ص�_origin_url�Ǹ������ʽ(bt://infohash)�����ڴ���BT����ʱ���⴦��(lx_build_bt_req_commit_task)
			if( sd_strlen(p_task_info->_origin_url) > 0)
				bt_create_task._magnet_url = p_task_info->_origin_url;
			else
			{
				return INVALID_URL;
			}
			bt_create_task._is_auto_charge = TRUE;
			bt_create_task._user_data = user_data;
			bt_create_task._callback_fun = callback;
						
			ret_val = lx_create_bt_task(&bt_create_task, p_action_id, 1);
			if(SUCCESS != ret_val)
			{	
				return ret_val;
			}			
			break;
		}	
		case LXT_BT:
		case LXT_BT_FILE:
		case LXT_SHOP:	
		default:
			return INVALID_TASK_TYPE;
			break;
	}
 	return SUCCESS;
}

_int32 lx_create_bt_task(LX_CREATE_BT_TASK_INFO * p_param, _u32 * p_action_id, _u32 retry_num)
{
	_int32 ret_val = SUCCESS;
	LX_PT_COMMIT_BT_TASK* p_action = NULL;
	_u32 http_id = 0;
	char jumpkey[LX_JUMPKEY_MAX_SIZE] = {0};
	_u32 jumpkey_len = 0;
	
	EM_LOG_DEBUG( "lx_create_bt_task" );

	if(!lx_is_logined()) return -1;
	if(!em_is_net_ok(TRUE)||(sd_get_net_type()==UNKNOWN_NET)) return NETWORK_NOT_READY;

	ret_val = sd_malloc(sizeof(LX_PT_COMMIT_BT_TASK), (void**) &p_action);
	CHECK_VALUE(ret_val);
	sd_memset(p_action,0x00,sizeof(LX_PT_COMMIT_BT_TASK));

	p_action->_action._type = LPT_CRE_BT_TASK;

	/* ------------- ��������cmd ------------ */
	/* ͷ�� */
	p_action->_req._cmd_header._version = LX_CMD_PROTOCAL_VERSION;
	p_action->_req._cmd_header._seq = lx_get_cmd_protocal_seq();
	// p_action._req._cmd_header._len �������
	p_action->_req._cmd_header._thunder_flag = 0x7000000;//get_product_flag();
	p_action->_req._cmd_header._compress_flag = 0;
	p_action->_req._cmd_header._cmd_type = LX_CMD_TYPE_NEW_BT_COMMIT_REQ;

	if(SUCCESS != lx_get_jumpkey(jumpkey, &jumpkey_len))
	{
		SAFE_DELETE(p_action);
		return LXE_WRONG_REQUEST;
	}	
	p_action->_req._jump_key_len = jumpkey_len;
	sd_memcpy(p_action->_req._jump_key, jumpkey, jumpkey_len);

	/* �û���Ϣ*/
	p_action->_req._userid = lx_get_base()->_userid;
	p_action->_req._vip_level = lx_get_base()->_vip_level;
	/* ------------------------------------------- */

	sd_memcpy(&p_action->_req._task_info, p_param, sizeof(LX_CREATE_BT_TASK_INFO));
	p_action->_action._retry_num = retry_num;
	p_action->_action._req_buffer_len = MAX_POST_REQ_BUFFER_LEN;
	p_action->_action._req_data_len = p_action->_action._req_buffer_len;
	
	ret_val = lx_build_bt_req_commit_task(p_action);
	
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		return ret_val;
	}

	ret_val = lx_get_xml_file_store_path(p_action->_action._file_path);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}

	ret_val = lx_post_req((LX_PT *) p_action,&http_id,0);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}
	p_action->_action._action_id = http_id;
	p_action->_action._state = LPS_RUNNING;
	
	lx_add_action_to_list((LX_PT * )p_action);

	*p_action_id = http_id;

	return ret_val;
}
_int32 lx_create_bt_task_resp(LX_PT * p_action)
 {
	_int32 ret_val = SUCCESS;
	LX_PT_COMMIT_BT_TASK* p_action_ls = (LX_PT_COMMIT_BT_TASK *)p_action;
	LX_CREATE_TASK_RESULT* p_result = &p_action_ls->_resp;

	EM_LOG_DEBUG( "lx_create_bt_task_resp, error_code = %d", p_action->_error_code );

	p_result->_action_id = p_action->_action_id;
	p_result->_user_data = p_action_ls->_req._task_info._user_data;
	p_result->_result = p_action->_error_code;

	if(p_action->_error_code == SUCCESS)
	{
		if(p_action->_file_id != 0) 
		{
			//�ر��ļ�
			sd_close_ex(p_action->_file_id);
			p_action->_file_id = 0;
		}
	#ifdef LX_CREATE_LIXIAN_TASK
		LX_TASK_INFO_EX *task_info = NULL;
		lx_malloc_ex_task(&task_info);
		ret_val = lx_parse_bt_resp_commit_task_info(p_action_ls, task_info);
		EM_LOG_DEBUG( "lx_parse_bt_resp_commit_task_info, ret_val = %d, resp_status = %u", ret_val, p_action->_resp_status);
	#else
		ret_val = lx_parse_bt_resp_commit_task(p_action_ls);
		EM_LOG_DEBUG( "lx_parse_bt_resp_commit_task, ret_val = %d, resp_status = %u", ret_val, p_action->_resp_status);
	#endif
		ret_val = lx_pt_server_errcode_to_em_errcode(ret_val);
		if(ret_val != SUCCESS) 
		{
			EM_LOG_DEBUG( "lx_create_bt_task_resp: lx_pt_server_errcode_to_em_errcode: ret_val= %d", ret_val);
			p_result->_result = ret_val;
			goto ErrHandler;
		}
		
		if( ((p_action->_resp_status == 0)|| (p_action->_resp_status == 200)) && (p_action->_error_code == SUCCESS))
		{
			/* ls �ɹ� */
			p_result->_result = 0;
			lx_set_user_lixian_info((_u32)p_result->_max_task_num,(_u32)p_result->_current_task_num,p_result->_max_space,p_result->_available_space);
			/* ���������б� */
		#ifdef LX_CREATE_LIXIAN_TASK
			//lx_add_task_to_map(task_info);
			lx_add_new_create_task_to_map(task_info);
		#endif
		}
		else
		{
			p_result->_result = p_action->_resp_status+p_action->_error_code;
			sd_assert( p_result->_result!=SUCCESS);
		}
	}
	else
	{
		/*���Ի��� (������ʱ����)*/
		if(p_action->_retry_num > 0)
		{
			EM_LOG_DEBUG( "error_code = %d, lx_create_bt_task  try again",p_action->_error_code);
			
			ret_val = lx_create_bt_task(&p_action_ls->_req._task_info, &p_action_ls->_resp._action_id, 0);
			if(ret_val != SUCCESS)
			{	
				p_result->_result = ret_val;
				goto ErrHandler;
			}

			if(p_action->_file_id != 0)
			{
				sd_close_ex(p_action->_file_id);
				p_action->_file_id = 0;
			}
			sd_delete_file(p_action->_file_path);	
			SAFE_DELETE(p_action);
			return SUCCESS;
		}
	}
	
ErrHandler:

	/* �ص�֪ͨUI */

	p_action_ls->_req._task_info._callback_fun(p_result);

	/*�ͷŽ��շ�����Ϣ�Ŀռ�*/

	if(p_action->_file_id!=0)
	{
		sd_close_ex(p_action->_file_id);
		p_action->_file_id = 0;
	}

	sd_delete_file(p_action->_file_path);
	
	SAFE_DELETE(p_action);

	return SUCCESS;

}


_int32 lx_delete_task(_u64 task_id, void* user_data, LX_DELETE_TASK_CALLBACK callback_fun, _u32 * p_action_id, _u32 retry_num)
{
	_int32 ret_val = SUCCESS;
	LX_PT_DELETE_TASK* p_action = NULL;
	_u32 http_id = 0;
	char jumpkey[LX_JUMPKEY_MAX_SIZE] = {0};
	_u32 jumpkey_len = 0;

	EM_LOG_DEBUG( "lx_delete_task: type = 2" );

	if(!lx_is_logined()) return -1;
	if(!em_is_net_ok(TRUE)||(sd_get_net_type()==UNKNOWN_NET)) return NETWORK_NOT_READY;

	ret_val = sd_malloc(sizeof(LX_PT_DELETE_TASK), (void**) &p_action);
	CHECK_VALUE(ret_val);
	sd_memset(p_action,0x00,sizeof(LX_PT_DELETE_TASK));

	p_action->_action._type = LPT_DEL_TASK;

	/* --- ---------��������cmd -------------- */
	/* ͷ�� */
	p_action->_req._cmd_header._version = LX_CMD_PROTOCAL_VERSION;
	p_action->_req._cmd_header._seq = lx_get_cmd_protocal_seq();
	// p_action._req._cmd_header._len �������
	p_action->_req._cmd_header._thunder_flag = get_product_flag();
	p_action->_req._cmd_header._compress_flag = 0;
	p_action->_req._cmd_header._cmd_type = LX_CMD_TYPE_DELETE_REQ;

	if(SUCCESS != lx_get_jumpkey(jumpkey, &jumpkey_len))
	{
		SAFE_DELETE(p_action);
		return LXE_WRONG_REQUEST;
	}
	p_action->_req._jump_key_len = jumpkey_len;
	sd_memcpy(p_action->_req._jump_key, jumpkey, jumpkey_len);
	
	/* �û���Ϣ*/
	p_action->_req._userid = lx_get_base()->_userid;
	p_action->_req._vip_level = lx_get_base()->_vip_level;
	/* ------------------------------------------- */
	
	p_action->_req._flag = 2;
	p_action->_req._task_id = task_id;
	p_action->_req._user_data = user_data;
	p_action->_req._callback_fun = callback_fun;

	p_action->_action._retry_num = retry_num;
	p_action->_action._req_buffer_len = MAX_POST_REQ_BUFFER_LEN;
	p_action->_action._req_data_len = p_action->_action._req_buffer_len;
	
	ret_val = lx_build_req_delete_task(p_action);
	
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}

	ret_val = lx_get_xml_file_store_path(p_action->_action._file_path);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}
	
	ret_val = lx_post_req((LX_PT *) p_action,&http_id,-1);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}
	p_action->_action._action_id = http_id;
	p_action->_action._state = LPS_RUNNING;
	
	lx_add_action_to_list((LX_PT * )p_action);

	*p_action_id = http_id;
	
 	return ret_val;
}

_int32 lx_delete_overdue_task(_u64 task_id, void* user_data, LX_DELETE_TASK_CALLBACK callback_fun, _u32 * p_action_id, _u32 retry_num)
{
	_int32 ret_val = SUCCESS;
	LX_PT_DELETE_TASK* p_action = NULL;
	_u32 http_id = 0;
	char jumpkey[LX_JUMPKEY_MAX_SIZE] = {0};
	_u32 jumpkey_len = 0;

	EM_LOG_DEBUG( "lx_delete_overdue_task: type = 5" );

	if(!lx_is_logined()) return -1;
	if(!em_is_net_ok(TRUE)||(sd_get_net_type()==UNKNOWN_NET)) return NETWORK_NOT_READY;

	ret_val = sd_malloc(sizeof(LX_PT_DELETE_TASK), (void**) &p_action);
	CHECK_VALUE(ret_val);
	sd_memset(p_action,0x00,sizeof(LX_PT_DELETE_TASK));

	p_action->_action._type = LPT_DEL_TASK;

	/* --- ---------��������cmd -------------- */
	/* ͷ�� */
	p_action->_req._cmd_header._version = LX_CMD_PROTOCAL_VERSION;
	p_action->_req._cmd_header._seq = lx_get_cmd_protocal_seq();
	// p_action._req._cmd_header._len �������
	p_action->_req._cmd_header._thunder_flag = get_product_flag();
	p_action->_req._cmd_header._compress_flag = 0;
	p_action->_req._cmd_header._cmd_type = LX_CMD_TYPE_DELETE_REQ;

	if(SUCCESS != lx_get_jumpkey(jumpkey, &jumpkey_len))
	{
		SAFE_DELETE(p_action);
		return LXE_WRONG_REQUEST;
	}
	p_action->_req._jump_key_len = jumpkey_len;
	sd_memcpy(p_action->_req._jump_key, jumpkey, jumpkey_len);
	
	/* �û���Ϣ*/
	p_action->_req._userid = lx_get_base()->_userid;
	p_action->_req._vip_level = lx_get_base()->_vip_level;
	/* ------------------------------------------- */
	
	p_action->_req._flag = 5;
	p_action->_req._task_id = task_id;
	p_action->_req._user_data = user_data;
	p_action->_req._callback_fun = callback_fun;

	p_action->_action._retry_num = retry_num;
	p_action->_action._req_buffer_len = MAX_POST_REQ_BUFFER_LEN;
	p_action->_action._req_data_len = p_action->_action._req_buffer_len;
	
	ret_val = lx_build_req_delete_task(p_action);
	
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}

	ret_val = lx_get_xml_file_store_path(p_action->_action._file_path);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}
	
	ret_val = lx_post_req((LX_PT *) p_action,&http_id,-1);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}
	p_action->_action._action_id = http_id;
	p_action->_action._state = LPS_RUNNING;
	
	lx_add_action_to_list((LX_PT * )p_action);

	*p_action_id = http_id;
	
 	return ret_val;
}


_int32 lx_delete_task_resp(LX_PT * p_action)
 {
	_int32 ret_val = SUCCESS;
	LX_PT_DELETE_TASK* p_action_ls = (LX_PT_DELETE_TASK *)p_action;
	LX_DELETE_TASK_RESULT* p_result = &p_action_ls->_resp;

	EM_LOG_DEBUG( "lx_delete_task_resp, type = %u, error_code = %d", p_action_ls->_req._flag, p_action->_error_code);

	p_result->_action_id = p_action->_action_id;
	p_result->_user_data = p_action_ls->_req._user_data;
	p_result->_result = p_action->_error_code;
	
	if(p_action->_error_code == SUCCESS)
	{

		if(p_action->_file_id!=0)
		{
			sd_close_ex(p_action->_file_id);
			p_action->_file_id = 0;
		}
		ret_val = lx_parse_resp_delete_task(p_action_ls);
		EM_LOG_DEBUG( "lx_parse_resp_delete_task, ret_val = %u, resp_status = %d",ret_val, p_action->_resp_status);
		ret_val = lx_pt_server_errcode_to_em_errcode(ret_val);
		if(ret_val != SUCCESS) 
		{
			p_result->_result = ret_val;
			goto ErrHandler;
		}
		
		if( ((p_action->_resp_status == 0)|| (p_action->_resp_status == 200)) && (p_action->_error_code == SUCCESS))
		{
			/* ls �ɹ� */
			p_result->_result = 0;
			lx_remove_task_from_map(p_action_ls->_req._task_id);
		}
		else
		{
			p_result->_result = p_action->_resp_status;
			sd_assert( p_result->_result!=SUCCESS);
		}
	}
	else
	{
		/*���Ի��� (������ʱ����)*/
		if(p_action->_retry_num > 0)
		{
			if( 2 == p_action_ls->_req._flag)
			{
				EM_LOG_DEBUG( "error_code = %d, lx_delete_task  try again",p_action->_error_code);
				ret_val = lx_delete_task(p_action_ls->_req._task_id, p_action_ls->_req._user_data, p_action_ls->_req._callback_fun, &p_action_ls->_resp._action_id, 0);
			}
			else if(5 == p_action_ls->_req._flag)
			{
				EM_LOG_DEBUG( "error_code = %d, lx_delete_overdue_task  try again",p_action->_error_code);
				ret_val = lx_delete_overdue_task(p_action_ls->_req._task_id, p_action_ls->_req._user_data, p_action_ls->_req._callback_fun, &p_action_ls->_resp._action_id, 0);
			}
			if(ret_val != SUCCESS)
			{	
				p_result->_result = ret_val;
				goto ErrHandler;
			}

			if(p_action->_file_id != 0)
			{
				sd_close_ex(p_action->_file_id);
				p_action->_file_id = 0;
			}
			sd_delete_file(p_action->_file_path);	
			SAFE_DELETE(p_action);
			return SUCCESS;
		}
	}
	
ErrHandler:

	/* �ص�֪ͨUI */
	p_action_ls->_req._callback_fun(p_result);
	
	if(p_action->_file_id!=0)
	{
		sd_close_ex(p_action->_file_id);
		p_action->_file_id = 0;
	}

	sd_delete_file(p_action->_file_path);
	
	SAFE_DELETE(p_action);

	return SUCCESS;
}

/* ����ɾ������ */
_int32 lx_delete_tasks(_u32 task_num,_u64 * p_task_ids,BOOL is_overdue, void* user_data, LX_DELETE_TASKS_CALLBACK callback_fun, _u32 * p_action_id)
{
	_int32 ret_val = SUCCESS;
	LX_PT_DELETE_TASKS* p_action = NULL;
	_u32 http_id = 0;
	char jumpkey[LX_JUMPKEY_MAX_SIZE] = {0};
	_u32 jumpkey_len = 0;

	EM_LOG_DEBUG( "lx_delete_tasks" );

	if(!lx_is_logined()) return -1;
	if(!em_is_net_ok(TRUE)||(sd_get_net_type()==UNKNOWN_NET)) return NETWORK_NOT_READY;

	ret_val = sd_malloc(sizeof(LX_PT_DELETE_TASKS), (void**) &p_action);
	CHECK_VALUE(ret_val);
	sd_memset(p_action,0x00,sizeof(LX_PT_DELETE_TASKS));

	p_action->_action._type = LPT_DEL_TASKS;

	/* --- ---------��������cmd -------------- */
	/* ͷ�� */
	p_action->_req._cmd_header._version = LX_CMD_PROTOCAL_VERSION;
	p_action->_req._cmd_header._seq = lx_get_cmd_protocal_seq();
	// p_action._req._cmd_header._len �������
	p_action->_req._cmd_header._thunder_flag = get_product_flag();
	p_action->_req._cmd_header._compress_flag = 0;
	p_action->_req._cmd_header._cmd_type = LX_CMD_TYPE_DELETE_REQ;

	if(SUCCESS != lx_get_jumpkey(jumpkey, &jumpkey_len))
	{
		SAFE_DELETE(p_action);
		return LXE_WRONG_REQUEST;
	}
	p_action->_req._jump_key_len = jumpkey_len;
	sd_memcpy(p_action->_req._jump_key, jumpkey, jumpkey_len);
	
	/* �û���Ϣ*/
	p_action->_req._userid = lx_get_base()->_userid;
	p_action->_req._vip_level = lx_get_base()->_vip_level;
	/* ------------------------------------------- */
	
	p_action->_req._flag = 2;
	if(is_overdue)
	{
		p_action->_req._flag = 5;
	}

	sd_assert(task_num!=0);

	p_action->_req._task_num = task_num;
	p_action->_resp._task_num = task_num;
	ret_val = sd_malloc(task_num*sizeof(_u64), (void**) &p_action->_req._p_task_ids);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}
	sd_memcpy(p_action->_req._p_task_ids,p_task_ids,task_num*sizeof(_u64));
	p_action->_resp._p_task_ids = p_action->_req._p_task_ids;
	
	ret_val = sd_malloc(task_num*sizeof(_int32), (void**) &p_action->_resp._p_results);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action->_req._p_task_ids);
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}
	sd_memset(p_action->_resp._p_results,0,task_num*sizeof(_int32));
	
	p_action->_req._user_data = user_data;
	p_action->_req._callback_fun = callback_fun;
	
	p_action->_action._req_buffer_len = MAX_POST_REQ_BUFFER_LEN;
	p_action->_action._req_data_len = p_action->_action._req_buffer_len;
	
	ret_val = lx_build_req_delete_tasks(p_action);
	
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action->_resp._p_results);
		SAFE_DELETE(p_action->_req._p_task_ids);
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}

	ret_val = lx_get_xml_file_store_path(p_action->_action._file_path);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action->_resp._p_results);
		SAFE_DELETE(p_action->_req._p_task_ids);
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}
	
	ret_val = lx_post_req((LX_PT *) p_action,&http_id,-1);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action->_resp._p_results);
		SAFE_DELETE(p_action->_req._p_task_ids);
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}
	p_action->_action._action_id = http_id;
	p_action->_action._state = LPS_RUNNING;
	
	lx_add_action_to_list((LX_PT * )p_action);

	*p_action_id = http_id;
	
 	return ret_val;
}
_int32 lx_delete_tasks_resp(LX_PT * p_action)
 {
	_int32 ret_val = SUCCESS,i = 0;
	LX_PT_DELETE_TASKS* p_action_ls = (LX_PT_DELETE_TASKS *)p_action;
	LX_DELETE_TASKS_RESULT* p_result = &p_action_ls->_resp;
	_int32 * p_task_result = NULL;
	_u64 * p_task_id = NULL;

	EM_LOG_DEBUG( "lx_delete_tasks_resp: error_code = %d,task_num=%u",p_action->_error_code ,p_action_ls->_req._task_num);

	p_result->_action_id = p_action->_action_id;
	p_result->_user_data = p_action_ls->_req._user_data;
	p_result->_task_num = p_action_ls->_req._task_num;
	p_result->_p_task_ids = p_action_ls->_req._p_task_ids;
	p_task_result = p_result->_p_results;
	
	if(p_action->_error_code == SUCCESS)
	{

		if(p_action->_file_id!=0)
		{
			sd_close_ex(p_action->_file_id);
			p_action->_file_id = 0;
		}
		
		ret_val = lx_parse_resp_delete_tasks(p_action_ls);
		EM_LOG_DEBUG( "lx_parse_resp_delete_tasks, ret_val = %u, resp_status = %d",ret_val, p_action->_resp_status);
		if(ret_val != SUCCESS) 
		{
			for(i=0;i<p_result->_task_num;i++)
			{
				*p_task_result = p_action->_error_code;
				p_task_result++;
			}
			goto ErrHandler;
		}

		p_task_id = p_action_ls->_req._p_task_ids;
		for(i=0;i<p_result->_task_num;i++)
		{
			lx_remove_task_from_map(*p_task_id);
			p_task_id++;
		}
	}
	else
	{
		for(i=0;i<p_result->_task_num;i++)
		{
			*p_task_result = p_action->_error_code;
			p_task_result++;
		}
	}
	
ErrHandler:

	/* �ص�֪ͨUI */
	//typedef _int32 ( *LX_GET_TASK_LIST_CALLBACK)(LX_TASK_LIST * p_task_list);
	p_action_ls->_req._callback_fun(p_result);
	
	if(p_action->_file_id!=0)
	{
		sd_close_ex(p_action->_file_id);
		p_action->_file_id = 0;
	}

	sd_delete_file(p_action->_file_path);
	
	SAFE_DELETE(p_action_ls->_resp._p_results);
	SAFE_DELETE(p_action_ls->_req._p_task_ids);
	SAFE_DELETE(p_action);

	return SUCCESS;
}

_int32 lx_delay_task(_u64 task_id, void* user_data, LX_DELAY_TASK_CALLBACK callback_fun, _u32 * p_action_id, _u32 retry_num)
{
	_int32 ret_val = SUCCESS;
	LX_PT_DELAY_TASK* p_action = NULL;
	_u32 http_id = 0;
	char jumpkey[LX_JUMPKEY_MAX_SIZE] = {0};
	_u32 jumpkey_len = 0;
	LX_TASK_INFO_EX * p_task_info = NULL;
	
	EM_LOG_DEBUG( "lx_delay_task:%llu",task_id );

	if(!lx_is_logined()) return -1;
	if(!em_is_net_ok(TRUE)||(sd_get_net_type()==UNKNOWN_NET)) return NETWORK_NOT_READY;

	p_task_info = lx_get_task_from_map(task_id);
	sd_assert(p_task_info!=NULL);
	if(p_task_info==NULL)
	{
		return INVALID_TASK_ID;
	}
	
	ret_val = sd_malloc(sizeof(LX_PT_DELAY_TASK), (void**) &p_action);
	CHECK_VALUE(ret_val);
	sd_memset(p_action,0x00,sizeof(LX_PT_DELAY_TASK));

	p_action->_action._type = LPT_DELAY_TASK;

	/* ------------- ��������cmd ------------ */
	/* ͷ�� */
	p_action->_req._cmd_header._version = LX_CMD_PROTOCAL_VERSION;
	p_action->_req._cmd_header._seq = lx_get_cmd_protocal_seq();
	// p_action._req._cmd_header._len �������
	p_action->_req._cmd_header._thunder_flag = get_product_flag();
	p_action->_req._cmd_header._compress_flag = 0;
	p_action->_req._cmd_header._cmd_type = LX_CMD_TYPE_DELAYTASK_REQ;

	if(SUCCESS != lx_get_jumpkey(jumpkey, &jumpkey_len))
	{
		SAFE_DELETE(p_action);
		return LXE_WRONG_REQUEST;
	}	
	p_action->_req._jump_key_len = jumpkey_len;
	sd_memcpy(p_action->_req._jump_key, jumpkey, jumpkey_len);

	/* �û���Ϣ*/
	p_action->_req._userid = lx_get_base()->_userid;
	p_action->_req._vip_level = lx_get_base()->_vip_level;
	/* ------------------------------------------- */

	p_action->_req._task_id = task_id;
	p_action->_req._user_data = user_data;
	p_action->_req._callback_fun = callback_fun;
	
	p_action->_action._retry_num = retry_num;
	p_action->_action._req_buffer_len = MAX_POST_REQ_BUFFER_LEN;
	p_action->_action._req_data_len = p_action->_action._req_buffer_len;
	
	ret_val = lx_build_req_delay_task(p_action);
	
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}

	ret_val = lx_get_xml_file_store_path(p_action->_action._file_path);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}

	ret_val = lx_post_req((LX_PT *) p_action,&http_id,-1);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}
	p_action->_action._action_id = http_id;
	p_action->_action._state = LPS_RUNNING;
	
	lx_add_action_to_list((LX_PT * )p_action);

	*p_action_id = http_id;
	
 	return ret_val;
}

_int32 lx_delay_task_resp(LX_PT * p_action)
 {
	_int32 ret_val = SUCCESS;
	LX_PT_DELAY_TASK* p_action_ls = (LX_PT_DELAY_TASK *)p_action;
	LX_DELAY_TASK_RESULT* p_result = &p_action_ls->_resp;

	EM_LOG_DEBUG( "lx_delay_task_resp: task_id = %llu, errcode= %d",p_action_ls->_req._task_id, p_action->_error_code);

	p_result->_action_id = p_action->_action_id;
	p_result->_user_data = p_action_ls->_req._user_data;
	p_result->_result = p_action->_error_code;
	
	if(p_action->_error_code == SUCCESS)
	{
		if(p_action->_file_id!=0)
		{
			sd_close_ex(p_action->_file_id);
			p_action->_file_id = 0;
		}
		ret_val = lx_parse_resp_delay_task(p_action_ls);
		EM_LOG_DEBUG( "lx_delay_task_resp, ret_val = %u, resp_status = %d",ret_val, p_action->_resp_status);
		ret_val = lx_pt_server_errcode_to_em_errcode(ret_val);
		if(ret_val != SUCCESS) 
		{
			p_result->_result = ret_val;
			goto ErrHandler;
		}
		
		if( ((p_action->_resp_status == 0)|| (p_action->_resp_status == 200)) && (p_action->_error_code == SUCCESS))
		{
			/* ���»����е����� */
			LX_TASK_INFO_EX * p_task_info = lx_get_task_from_map(p_action_ls->_req._task_id);
			sd_assert(p_task_info!=NULL);
			p_task_info->_left_live_time = p_result->_left_live_time;
			
			/* delay �ɹ� */
			p_result->_result = 0;
		}
		else
		{
			p_result->_result = p_action->_resp_status+p_action->_error_code;
			sd_assert( p_result->_result!=SUCCESS);
		}
	}
	else
	{
		/*���Ի��� (������ʱ����)*/
		if(p_action->_retry_num > 0)
		{
			EM_LOG_DEBUG( "error_code = %d, lx_delay_task  try again",p_action->_error_code);
			ret_val = lx_delay_task(p_action_ls->_req._task_id, p_action_ls->_req._user_data, p_action_ls->_req._callback_fun, &p_action_ls->_resp._action_id, 0);
			if(ret_val != SUCCESS)
			{	
				p_result->_result = ret_val;
				goto ErrHandler;
			}

			if(p_action->_file_id != 0)
			{
				sd_close_ex(p_action->_file_id);
				p_action->_file_id = 0;
			}
			sd_delete_file(p_action->_file_path);	
			SAFE_DELETE(p_action);
			return SUCCESS;
		}
	}
ErrHandler:

	/* �ص�֪ͨUI */
	p_action_ls->_req._callback_fun(p_result);
	
	if(p_action->_file_id!=0)
	{
		sd_close_ex(p_action->_file_id);
		p_action->_file_id = 0;
	}

	sd_delete_file(p_action->_file_path);
	
	SAFE_DELETE(p_action);

	return ret_val;
}

////////////////////////////////////////////////////////////////////////////////////////////////
/* ��ѯ���߿ռ�������Ϣ
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
#if 0
_int32 lx_query_task_info(_u64 task_id, void* user_data, LX_QUERY_TASK_INFO_CALLBACK callback_fun, _u32 * p_action_id)
{
 	_int32 ret_val = SUCCESS;
	LX_PT_QUERY_TASK_INFO * p_action =NULL;
	_u32 http_id = 0;
	char jumpkey[LX_JUMPKEY_MAX_SIZE] = {0};
	_u32 jumpkey_len = 0;
	LX_TASK_INFO_EX * p_task_info = NULL;
	
	EM_LOG_DEBUG( "lx_query_task_info:%llu",task_id );
	
	if(!lx_is_logined()) return -1;
	p_task_info = lx_get_task_from_map(task_id);
	sd_assert(p_task_info!=NULL);
	if(p_task_info==NULL)
	{
		return INVALID_TASK_ID;
	}
	
	ret_val = sd_malloc(sizeof(LX_PT_QUERY_TASK_INFO),(void**) &p_action);
	CHECK_VALUE(ret_val);
	sd_memset(p_action,0x00,sizeof(LX_PT_QUERY_TASK_INFO));

	p_action->_action._type = LPT_QUERY_TASK_INFO;

	/* ------------- ��������cmd ------------ */
	/* ͷ�� */
	p_action->_req._cmd_header._version = LX_CMD_PROTOCAL_VERSION;
	p_action->_req._cmd_header._seq = lx_get_cmd_protocal_seq();
	// p_action._req._cmd_header._len �������
	p_action->_req._cmd_header._thunder_flag = get_product_flag();
	p_action->_req._cmd_header._compress_flag = 0;
	p_action->_req._cmd_header._cmd_type = LX_CMD_TYPE_QUERYTASK_REQ;
    
	if(SUCCESS != lx_get_jumpkey(jumpkey, &jumpkey_len))
	{
		SAFE_DELETE(p_action);
		return LXE_WRONG_REQUEST;
	}	
	p_action->_req._jump_key_len = jumpkey_len;
	sd_memcpy(p_action->_req._jump_key, jumpkey, jumpkey_len);
    
	/* �û���Ϣ*/
	p_action->_req._userid = lx_get_base()->_userid;
	/* ------------------------------------------- */
	p_action->_req._task_id = task_id;
    p_action->_req._user_data = user_data;
	p_action->_req._callback_fun = callback_fun;
	
	p_action->_action._req_buffer_len = MAX_POST_REQ_BUFFER_LEN;
	p_action->_action._req_data_len = p_action->_action._req_buffer_len;
	
	ret_val = lx_build_req_query_task_info(p_action);
	
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}
    
	ret_val = lx_get_xml_file_store_path(p_action->_action._file_path);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}
    
	ret_val = lx_post_req((LX_PT *) p_action,&http_id,0);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}
	p_action->_action._action_id = http_id;
	p_action->_action._state = LPS_RUNNING;
	
	lx_add_action_to_list((LX_PT * )p_action);

 	return ret_val;

}

_int32 lx_query_task_info_resp(LX_PT * p_action)
{
    _int32 ret_val = SUCCESS;
    LX_PT_QUERY_TASK_INFO* p_action_ls = (LX_PT_QUERY_TASK_INFO *)p_action;
    LX_QUERY_TASK_INFO_RESULT* p_result = &p_action_ls->_resp;
     
    EM_LOG_DEBUG( "lx_query_task_info_resp" );
    //g_geting_user_info = FALSE;
    p_result->_action_id = p_action->_action_id;
    p_result->_user_data = p_action_ls->_req._user_data;
    p_result->_result = p_action->_error_code;
     
    if(p_action->_error_code == SUCCESS)
    {
        if(p_action->_file_id != 0)
        {
            sd_close_ex(p_action->_file_id);
            p_action->_file_id = 0;
        }
        ret_val = lx_parse_resp_query_task_info(p_action_ls);
        if(ret_val != SUCCESS) 
        {
            p_result->_result = ret_val;
            goto ErrHandler;
        }
         
        if( ((p_action->_resp_status == 0)|| (p_action->_resp_status == 200)) && (p_action->_error_code == SUCCESS))
        {
            /* ���»����е�����״̬ */
			LX_TASK_INFO_EX * p_task_info = lx_get_task_from_map(p_action_ls->_req._task_id);
			if(p_task_info == NULL)
			{
				p_result->_result = LXE_TASK_INFO_NOT_IN_MAP;
			}
			else
			{	
				p_task_info->_state = p_result->_download_status;
				/* query task info �ɹ� */
				p_result->_result = 0;
			}
        }
        else
        {
            p_result->_result = p_action->_resp_status + p_action->_error_code;
            sd_assert( p_result->_result!=SUCCESS);
        }
    }
     
ErrHandler:
     /* �ص�֪ͨUI */
    p_action_ls->_req._callback_fun(p_result);
     
    if(p_action->_file_id!=0)
    {
        sd_close_ex(p_action->_file_id);
        p_action->_file_id = 0;
    }
     
    sd_delete_file(p_action->_file_path);
     
    SAFE_DELETE(p_action);
	return ret_val;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////
/* ��ѯ���߿ռ�������Ϣ
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
_int32 lx_query_bt_task_info(_u64 *task_id, _u32 num, void* user_data, LX_QUERY_TASK_INFO_CALLBACK callback_fun, _u32 * p_action_id)
{
 	_int32 ret_val = SUCCESS;
	LX_PT_QUERY_TASK_INFO * p_action =NULL;
	_u32 http_id = 0;
	char jumpkey[LX_JUMPKEY_MAX_SIZE] = {0};
	_u32 jumpkey_len = 0;
	
	EM_LOG_DEBUG( "lx_query_bt_task_info: task_num = %u",num);
	
	if(!lx_is_logined()) return -1;
	if(!em_is_net_ok(TRUE)||(sd_get_net_type()==UNKNOWN_NET)) return NETWORK_NOT_READY;
	
	ret_val = sd_malloc(sizeof(LX_PT_QUERY_TASK_INFO),(void**) &p_action);
	CHECK_VALUE(ret_val);
	sd_memset(p_action,0x00,sizeof(LX_PT_QUERY_TASK_INFO));

	p_action->_action._type = LPT_QUERY_BT_TASK_INFO;

	/* ------------- ��������cmd ------------ */
	/* ͷ�� */
	p_action->_req._cmd_header._version = LX_CMD_PROTOCAL_VERSION;
	p_action->_req._cmd_header._seq = lx_get_cmd_protocal_seq();
	// p_action._req._cmd_header._len �������
	p_action->_req._cmd_header._thunder_flag = get_product_flag();
	p_action->_req._cmd_header._compress_flag = 0;
	p_action->_req._cmd_header._cmd_type = LX_CMD_TYPE_BT_QUERYTASK_REQ;
    
	if(SUCCESS != lx_get_jumpkey(jumpkey, &jumpkey_len))
	{
		SAFE_DELETE(p_action);
		return LXE_WRONG_REQUEST;
	}	
	p_action->_req._jump_key_len = jumpkey_len;
	sd_memcpy(p_action->_req._jump_key, jumpkey, jumpkey_len);
    
	/* �û���Ϣ*/
	p_action->_req._userid = lx_get_base()->_userid;
	/* ------------------------------------------- */
	p_action->_req._task_id = task_id;
	p_action->_req._task_num = num;
    p_action->_req._user_data = user_data;
	p_action->_req._callback_fun = callback_fun;
	
	p_action->_action._req_buffer_len = MAX_POST_REQ_BUFFER_LEN;
	p_action->_action._req_data_len = p_action->_action._req_buffer_len;
	
	ret_val = lx_build_req_query_bt_task_info(p_action);
	
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}
    
	ret_val = lx_get_xml_file_store_path(p_action->_action._file_path);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}
    
	ret_val = lx_post_req((LX_PT *) p_action,&http_id,-1);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}
	p_action->_action._action_id = http_id;
	p_action->_action._state = LPS_RUNNING;
	
	lx_add_action_to_list((LX_PT * )p_action);
	
	*p_action_id = http_id;
 	return ret_val;

}

_int32 lx_query_bt_task_info_resp(LX_PT * p_action)
 {
	_int32 ret_val = SUCCESS;
	LX_PT_QUERY_TASK_INFO* p_action_ls = (LX_PT_QUERY_TASK_INFO *)p_action;
	LX_QUERY_TASK_INFO_RESULT* p_result = &p_action_ls->_resp;

	EM_LOG_DEBUG( "lx_query_bt_task_info_resp: task_num = %llu, errcode =%d",p_action_ls->_req._task_num, p_action->_error_code);

	p_result->_action_id = p_action->_action_id;
	p_result->_user_data = p_action_ls->_req._user_data;
	p_result->_result = p_action->_error_code;
	
	if(p_action->_error_code == SUCCESS)
	{
		if(p_action->_file_id!=0)
		{
			sd_close_ex(p_action->_file_id);
			p_action->_file_id = 0;
		}
		ret_val = lx_parse_resp_query_bt_task_info(p_action_ls);
		EM_LOG_DEBUG( "lx_query_bt_task_info_resp, ret_val = %u, resp_status = %d",ret_val, p_action->_resp_status);
		ret_val = lx_pt_server_errcode_to_em_errcode(ret_val);
		if(ret_val != SUCCESS) 
		{
			p_result->_result = ret_val;
			goto ErrHandler;
		}
		
		if( ((p_action->_resp_status == 0)|| (p_action->_resp_status == 200)) && (p_action_ls->_action._error_code == SUCCESS))
		{
			/*  �ɹ� */
			p_result->_result = 0;
		}
		else
		{
			p_result->_result = p_action->_resp_status+p_action->_error_code;
			sd_assert( p_result->_result!=SUCCESS);
		}
	}
	
ErrHandler:

	/* �ص�֪ͨUI */
	if(NULL != p_action_ls->_req._callback_fun)
		p_action_ls->_req._callback_fun(p_result);
	
	if(p_action->_file_id!=0)
	{
		sd_close_ex(p_action->_file_id);
		p_action->_file_id = 0;
	}

	sd_delete_file(p_action->_file_path);
	
	SAFE_DELETE(p_action);

	return ret_val;
}

_int32 lx_miniquery_task(_u64 task_id, void* user_data, LX_MINIQUERY_TASK_CALLBACK callback_fun, _u32 * p_action_id)
{
	_int32 ret_val = SUCCESS;
	LX_PT_MINIQUERY_TASK* p_action = NULL;
	_u32 http_id = 0;
	char jumpkey[LX_JUMPKEY_MAX_SIZE] = {0};
	_u32 jumpkey_len = 0;
	LX_TASK_INFO_EX * p_task_info = NULL;
	
	EM_LOG_DEBUG( "lx_miniquery_task:%llu",task_id );

	if(!lx_is_logined()) return -1;
	if(!em_is_net_ok(TRUE)||(sd_get_net_type()==UNKNOWN_NET)) return NETWORK_NOT_READY;

	p_task_info = lx_get_task_from_map(task_id);
	sd_assert(p_task_info!=NULL);
	if(p_task_info==NULL)
	{
		return INVALID_TASK_ID;
	}
	
	ret_val = sd_malloc(sizeof(LX_PT_MINIQUERY_TASK), (void**) &p_action);
	CHECK_VALUE(ret_val);
	sd_memset(p_action,0x00,sizeof(LX_PT_MINIQUERY_TASK));

	p_action->_action._type = LPT_MINIQUERY_TASK;

	/* ------------- ��������cmd ------------ */
	/* ͷ�� */
	p_action->_req._cmd_header._version = LX_CMD_PROTOCAL_VERSION;
	p_action->_req._cmd_header._seq = lx_get_cmd_protocal_seq();
	// p_action._req._cmd_header._len �������
	p_action->_req._cmd_header._thunder_flag = get_product_flag();
	p_action->_req._cmd_header._compress_flag = 0;
	p_action->_req._cmd_header._cmd_type = LX_CMD_TYPE_TASKMINI_REQ;

	if(SUCCESS != lx_get_jumpkey(jumpkey, &jumpkey_len))
	{
		SAFE_DELETE(p_action);
		return LXE_WRONG_REQUEST;
	}	
	p_action->_req._jump_key_len = jumpkey_len;
	sd_memcpy(p_action->_req._jump_key, jumpkey, jumpkey_len);

	/* �û���Ϣ*/
	p_action->_req._userid = lx_get_base()->_userid;
	p_action->_req._vip_level = lx_get_base()->_vip_level;
	/* ------------------------------------------- */

	p_action->_req._task_id = task_id;
	p_action->_req._user_data = user_data;
	p_action->_req._callback_fun = callback_fun;
	
	p_action->_action._req_buffer_len = MAX_POST_REQ_BUFFER_LEN;
	p_action->_action._req_data_len = p_action->_action._req_buffer_len;
	
	ret_val = lx_build_req_miniquery_task(p_action);
	
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}

	ret_val = lx_get_xml_file_store_path(p_action->_action._file_path);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}

	ret_val = lx_post_req((LX_PT *) p_action,&http_id,-1);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}
	p_action->_action._action_id = http_id;
	p_action->_action._state = LPS_RUNNING;
	
	lx_add_action_to_list((LX_PT * )p_action);

	*p_action_id = http_id;
	
 	return ret_val;
}

_int32 lx_miniquery_task_resp(LX_PT * p_action)
 {
	_int32 ret_val = SUCCESS;
	LX_PT_MINIQUERY_TASK* p_action_ls = (LX_PT_MINIQUERY_TASK *)p_action;
	LX_MINIQUERY_TASK_RESULT* p_result = &p_action_ls->_resp;

	EM_LOG_DEBUG( "lx_miniquery_task_resp:%llu",p_action_ls->_req._task_id  );

	p_result->_action_id = p_action->_action_id;
	p_result->_user_data = p_action_ls->_req._user_data;
	p_result->_result = p_action->_error_code;
	
	if(p_action->_error_code == SUCCESS)
	{
		if(p_action->_file_id!=0)
		{
			sd_close_ex(p_action->_file_id);
			p_action->_file_id = 0;
		}
		ret_val = lx_parse_resp_miniquery_task(p_action_ls);
		if(ret_val != SUCCESS) 
		{
			p_result->_result = ret_val;
			goto ErrHandler;
		}
		
		if( ((p_action->_resp_status == 0)|| (p_action->_resp_status == 200)) && (p_action->_error_code == SUCCESS))
		{
			/* miniquery �ɹ� */
			p_result->_result = 0;
		}
		else
		{
			p_result->_result = p_action->_resp_status+p_action->_error_code;
			sd_assert( p_result->_result!=SUCCESS);
		}
	}
	
ErrHandler:

	/* �ص�֪ͨUI */
	p_action_ls->_req._callback_fun(p_result);
	
	if(p_action->_file_id!=0)
	{
		sd_close_ex(p_action->_file_id);
		p_action->_file_id = 0;
	}

	sd_delete_file(p_action->_file_path);
	
	SAFE_DELETE(p_action);

	return ret_val;
}


/* ��ȡ�û�������Ϣ */
_int32 lx_get_user_info_req(void* user_data, LX_LOGIN_CALLBACK callback_fun, _u32* p_action_id)
{
	_int32 ret_val = SUCCESS;
	LX_PT_GET_USER_INFO_TASK* p_action = NULL;
	_u32 http_id = 0;
	char jumpkey[LX_JUMPKEY_MAX_SIZE] = {0};
	_u32 jumpkey_len = 0;
	
	EM_LOG_DEBUG( "lx_get_user_info_req" );
    
	if(!lx_is_logined()) return -1;
    
	ret_val = sd_malloc(sizeof(LX_PT_GET_USER_INFO_TASK), (void**) &p_action);
	CHECK_VALUE(ret_val);
	sd_memset(p_action, 0x00, sizeof(LX_PT_GET_USER_INFO_TASK));
    
	p_action->_action._type = LPT_GET_USER_INFO;
    
	/* ------------- ��������cmd ------------ */
	/* ͷ�� */
	p_action->_req._cmd_header._version = LX_CMD_PROTOCAL_VERSION;
	p_action->_req._cmd_header._seq = lx_get_cmd_protocal_seq();
	// p_action._req._cmd_header._len �������
	p_action->_req._cmd_header._thunder_flag = get_product_flag();
	p_action->_req._cmd_header._compress_flag = 0;
	p_action->_req._cmd_header._cmd_type = LX_CMD_TYPE_GETUSERINFO_REQ;
    
	if(SUCCESS != lx_get_jumpkey(jumpkey, &jumpkey_len))
	{
		SAFE_DELETE(p_action);
		return LXE_WRONG_REQUEST;
	}	
	p_action->_req._jump_key_len = jumpkey_len;
	sd_memcpy(p_action->_req._jump_key, jumpkey, jumpkey_len);
    
	/* �û���Ϣ*/
	p_action->_req._userid = lx_get_base()->_userid;
	p_action->_req._vip_level = lx_get_base()->_vip_level;
	/* ------------------------------------------- */
    p_action->_req._user_data = user_data;
	p_action->_req._callback_fun = callback_fun;
	
	p_action->_action._req_buffer_len = MAX_POST_REQ_BUFFER_LEN;
	p_action->_action._req_data_len = p_action->_action._req_buffer_len;
	
	ret_val = lx_build_req_get_user_info_task(p_action);
	
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}
    
	ret_val = lx_get_xml_file_store_path(p_action->_action._file_path);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}
    
	ret_val = lx_post_req((LX_PT *) p_action,&http_id,0);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}
	p_action->_action._action_id = http_id;
	p_action->_action._state = LPS_RUNNING;
	
	lx_add_action_to_list((LX_PT * )p_action);
    g_geting_user_info = TRUE;

	*p_action_id = http_id;
	
 	return ret_val;
}

_int32 lx_get_user_info_resp(LX_PT * p_action)
{
    _int32 ret_val = SUCCESS;
    LX_PT_GET_USER_INFO_TASK* p_action_ls = (LX_PT_GET_USER_INFO_TASK *)p_action;
    LX_LOGIN_RESULT* p_result = &p_action_ls->_resp;
     
    EM_LOG_DEBUG( "lx_get_user_info_resp: errcode = %d", p_action->_error_code );
    
    p_result->_action_id = p_action->_action_id;
    p_result->_user_data = p_action_ls->_req._user_data;
    p_result->_result = p_action->_error_code;
     
    if(p_action->_error_code == SUCCESS)
    {
        if(p_action->_file_id!=0)
        {
            sd_close_ex(p_action->_file_id);
            p_action->_file_id = 0;
        }
        ret_val = lx_parse_resp_get_user_info_task(p_action_ls);
		EM_LOG_DEBUG( "lx_get_user_info_resp, ret_val = %u, resp_status = %d",ret_val, p_action->_resp_status);
		ret_val = lx_pt_server_errcode_to_em_errcode(ret_val);
        if(ret_val != SUCCESS) 
        {
            p_result->_result = ret_val;
            goto ErrHandler;
        }
         
        if( ((p_action->_resp_status == 0)|| (p_action->_resp_status == 200)) && (p_action->_error_code == SUCCESS))
        {
             /* lx_get_user_info_resp  success*/
            p_result->_result = 0;
            lx_set_user_lixian_info((_u32)p_result->_max_task_num, (_u32)p_result->_current_task_num, p_result->_max_space, p_result->_available_space);
        }
        else
        {
            p_result->_result = p_action->_resp_status + p_action->_error_code;
            sd_assert( p_result->_result!=SUCCESS);
        }
    }
     
ErrHandler:
	
	g_geting_user_info = FALSE;
	#ifdef LX_LIST_CACHE
	if(p_result->_result!=0)
	{
		_u32 max_task_num = 0 ,current_task_num = 0;
		/* ��ȡ�������� */
		ret_val = lx_load_task_list_info(lx_get_base()->_userid);
		if(ret_val==SUCCESS)
		{
			/* �����ɹ� */
			p_result->_result = SUCCESS;
		}

		lx_get_task_num(&max_task_num,&current_task_num);
		p_result->_max_task_num = max_task_num;
		p_result->_current_task_num = current_task_num;
		
		lx_get_space(&p_result->_max_space,&p_result->_available_space);
	}
	#endif /* LX_LIST_CACHE */
     /* �ص�֪ͨUI */
	if(NULL != p_action_ls->_req._callback_fun)
    	p_action_ls->_req._callback_fun(p_result);
     
    if(p_action->_file_id!=0)
    {
        sd_close_ex(p_action->_file_id);
        p_action->_file_id = 0;
    }
     
    sd_delete_file(p_action->_file_path);
     
    SAFE_DELETE(p_action);
	return ret_val;
}
/* ��ȡ�㲥ʱ��cookie */
_int32 lx_get_cookie(char * cookie_buffer)
{
	_int32 ret_val = SUCCESS;
	EM_LOG_DEBUG( "lx_get_cookie" );

	if(!lx_is_logined()) return -1;

	ret_val = lx_get_cookie_impl(cookie_buffer);

	return ret_val;
}

/* ȡ��ĳ���첽���� */
_int32 lx_cancel(_u32 action_id)
{
	_int32 ret_val = SUCCESS;
	LX_PT* p_action = NULL;
	EM_LOG_DEBUG( "lx_cancel" );

	if(!g_inited) return -1;

	p_action = lx_get_action_from_list(action_id);
	//sd_assert(p_action!=NULL);
	if(p_action)	
	{
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
			//ret_val = lx_cancel_download_pic(p_action);
			sd_assert(FALSE);
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
	}
	return ret_val;
}

_int32 lx_get_free_strategy(_u64 user_id, const char *session_id, void* user_data, LX_FREE_STRATEGY_CALLBACK callback_fun, _u32 * p_action_id)
{
	_int32 ret_val = SUCCESS;
	LX_PT_FREE_STRATEGY* p_action = NULL;
	_u32 http_id = 0;

	if(!em_is_net_ok(TRUE)||(sd_get_net_type()==UNKNOWN_NET)) return NETWORK_NOT_READY;

	ret_val = sd_malloc(sizeof(LX_PT_FREE_STRATEGY), (void**) &p_action);
	CHECK_VALUE(ret_val);
	sd_memset(p_action,0x00,sizeof(LX_PT_FREE_STRATEGY));

	/* ------------- ��������cmd ------------ */
	p_action->_action._type = LPT_FREE_STRATEGY;
	p_action->_action._is_compress = TRUE;
	p_action->_action._is_aes = TRUE;
	p_action->_action._req_buffer_len = MAX_POST_REQ_BUFFER_LEN;
	p_action->_action._req_data_len = p_action->_action._req_buffer_len;
	/* �û���Ϣ*/
	p_action->_req._jump_key_len = sd_strlen(session_id);
	sd_memcpy(p_action->_req._jump_key, session_id, p_action->_req._jump_key_len);

	p_action->_req._userid = user_id;
	p_action->_req._user_data = user_data;
	p_action->_req._callback_fun = callback_fun;
	
	ret_val = lx_build_req_free_strategy(p_action);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
		goto ErrHandle;
	}

	ret_val = lx_get_xml_file_store_path(p_action->_action._file_path);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
		goto ErrHandle;
	}

	ret_val = lx_post_req((LX_PT *) p_action,&http_id,0);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
		goto ErrHandle;
	}
	/*
	p_action->_action._action_id = http_id;
	p_action->_action._state = LPS_RUNNING;
	
	lx_add_action_to_list((LX_PT * )p_action);
	*/
	*p_action_id = http_id;
	
 	return SUCCESS;
ErrHandle:
	return ret_val;
}

_int32 lx_get_free_strategy_resp(LX_PT * p_action)
 {
	_int32 ret_val = SUCCESS;
	LX_PT_FREE_STRATEGY* p_action_ls = (LX_PT_FREE_STRATEGY *)p_action;
	LX_FREE_STRATEGY_RESULT* p_result = &p_action_ls->_resp;

	p_result->_action_id = p_action->_action_id;
	p_result->_user_data = p_action_ls->_req._user_data;
	p_result->_result = p_action->_error_code;
	
	if(p_action->_error_code == SUCCESS)
	{
		if(p_action->_file_id!=0)
		{
			sd_close_ex(p_action->_file_id);
			p_action->_file_id = 0;
		}
		ret_val = lx_parse_resp_is_free_strategy(p_action_ls);
		if(ret_val != SUCCESS) 
		{
			p_result->_result = ret_val;
			goto ErrHandler;
		}
	}
	
ErrHandler:

	/* �ص�֪ͨUI */
	if(NULL != p_action_ls->_req._callback_fun)
		p_action_ls->_req._callback_fun(p_result);
	
	if(p_action->_file_id!=0)
	{
		sd_close_ex(p_action->_file_id);
		p_action->_file_id = 0;
	}

	sd_delete_file(p_action->_file_path);
	
	SAFE_DELETE(p_action);

	return ret_val;
}

_int32 lx_get_free_experience_member(_u32 type, _u64 user_id, void* user_data, LX_FREE_EXPERIENCE_MEMBER_CALLBACK callback_fun, _u32 * p_action_id)
{
	_int32 ret_val = SUCCESS;
	LX_PT_FREE_EXPERIENCE_MEMBER* p_action = NULL;
	_u32 http_id = 0;

	if(!em_is_net_ok(TRUE)||(sd_get_net_type()==UNKNOWN_NET)) return NETWORK_NOT_READY;

	ret_val = sd_malloc(sizeof(LX_PT_FREE_EXPERIENCE_MEMBER), (void**) &p_action);
	CHECK_VALUE(ret_val);
	sd_memset(p_action,0x00,sizeof(LX_PT_FREE_EXPERIENCE_MEMBER));

	/* ------------- ��������cmd ------------ */
	p_action->_action._is_compress = TRUE;
	p_action->_action._is_aes = TRUE;
	p_action->_action._req_buffer_len = MAX_POST_REQ_BUFFER_LEN;
	p_action->_action._req_data_len = p_action->_action._req_buffer_len;
	/* �û���Ϣ*/
	p_action->_req._userid = user_id;
	p_action->_req._user_data = user_data;
	p_action->_req._callback_fun = callback_fun;
	p_action->_req._type = type;
	switch(type)
	{	
		case LX_CS_RESP_GET_FREE_DAY:
			p_action->_action._type = LPT_FREE_EXPERIENCE_MEMBER;
			ret_val = lx_build_req_free_experience_member(p_action);
			break;
		case LX_CS_RESP_GET_REMAIN_DAY:
			p_action->_action._type = LPT_GET_EXPERIENCE_MEMBER_REMAIN_TIME;
			ret_val = lx_build_req_get_experience_member_remain_time(p_action);
			break;
		default:
			return ret_val;
	}
	
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}

	ret_val = lx_get_xml_file_store_path(p_action->_action._file_path);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}

	ret_val = lx_post_req((LX_PT *) p_action,&http_id,0);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}
	// http�����id
	p_action->_action._action_id = http_id;
	*p_action_id = http_id;
	
 	return ret_val;
}

_int32 lx_get_free_experience_member_resp(LX_PT * p_action)
 {
	_int32 ret_val = SUCCESS;
	LX_PT_FREE_EXPERIENCE_MEMBER* p_action_ls = (LX_PT_FREE_EXPERIENCE_MEMBER *)p_action;
	LX_FREE_EXPERIENCE_MEMBER_RESULT* p_result = &p_action_ls->_resp;

	p_result->_action_id = p_action->_action_id;
	p_result->_user_data = p_action_ls->_req._user_data;
	p_result->_result = p_action->_error_code;
	p_result->_user_id = p_action_ls->_req._userid;
	p_result->_type = p_action_ls->_req._type;
	
	if(p_action->_error_code == SUCCESS)
	{
		if(p_action->_file_id != 0)
		{
			sd_close_ex(p_action->_file_id);
			p_action->_file_id = 0;
		}
		ret_val = lx_parse_resp_data_free_experience_member(p_action_ls);
		if(ret_val != SUCCESS) 
		{
			p_result->_result = ret_val;
			goto ErrHandler;
		}
	}
	
ErrHandler:

	/* �ص�֪ͨUI */
	if(NULL != p_action_ls->_req._callback_fun)
		p_action_ls->_req._callback_fun(p_result);
	
	if(p_action->_file_id!=0)
	{
		sd_close_ex(p_action->_file_id);
		p_action->_file_id = 0;
	}

	sd_delete_file(p_action->_file_path);
	
	SAFE_DELETE(p_action);

	return ret_val;
}

_int32 lx_get_high_speed_flux(LX_GET_HIGH_SPEED_FLUX* param, _u32 * p_action_id)
{
	_int32 ret_val = SUCCESS;
	LX_PT_GET_HIGH_SPEED_FLUX* p_action = NULL;
	_u32 http_id = 0;

	if(!em_is_net_ok(TRUE)||(sd_get_net_type()==UNKNOWN_NET)) return NETWORK_NOT_READY;

	ret_val = sd_malloc(sizeof(LX_PT_GET_HIGH_SPEED_FLUX), (void**) &p_action);
	CHECK_VALUE(ret_val);
	sd_memset(p_action,0x00,sizeof(LX_PT_GET_HIGH_SPEED_FLUX));

	/* ------------- ��������cmd ------------ */
	p_action->_action._type = LPT_GET_HIGH_SPEED_FLUX;
	p_action->_action._is_compress = TRUE;
	p_action->_action._is_aes = TRUE;
	p_action->_action._req_buffer_len = MAX_POST_REQ_BUFFER_LEN;
	p_action->_action._req_data_len = p_action->_action._req_buffer_len;
	/* �û���Ϣ*/
	sd_memcpy(&p_action->_req, param, sizeof(p_action->_req));
	
	ret_val = lx_build_req_get_high_speed_flux(p_action);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
		goto ErrHandle;
	}

	ret_val = lx_get_xml_file_store_path(p_action->_action._file_path);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
		goto ErrHandle;
	}

	ret_val = lx_post_req((LX_PT *) p_action,&http_id,0);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
		goto ErrHandle;
	}
	*p_action_id = http_id;
	
 	return SUCCESS;
ErrHandle:
	return ret_val;
}

_int32 lx_get_high_speed_flux_resp(LX_PT * p_action)
 {
	_int32 ret_val = SUCCESS;
	LX_PT_GET_HIGH_SPEED_FLUX* p_action_ls = (LX_PT_GET_HIGH_SPEED_FLUX *)p_action;
	LX_GET_HIGH_SPEED_FLUX_RESULT* p_result = &p_action_ls->_resp;

	p_result->_action_id = p_action->_action_id;
	p_result->_user_data = p_action_ls->_req._user_data;
	p_result->_result = p_action->_error_code;
	
	if(p_action->_error_code == SUCCESS)
	{
		if(p_action->_file_id!=0)
		{
			sd_close_ex(p_action->_file_id);
			p_action->_file_id = 0;
		}
		ret_val = lx_parse_resp_get_high_speed_flux(p_action_ls);
		if(ret_val != SUCCESS) 
		{
			p_result->_result = ret_val;
			goto ErrHandler;
		}
	}
	
ErrHandler:

	/* �ص�֪ͨUI */
	if(NULL != p_action_ls->_req._callback_fun)
		p_action_ls->_req._callback_fun(p_result);
	
	if(p_action->_file_id!=0)
	{
		sd_close_ex(p_action->_file_id);
		p_action->_file_id = 0;
	}

	sd_delete_file(p_action->_file_path);
	
	SAFE_DELETE(p_action);

	return ret_val;
}

_int32 lx_take_off_flux_from_high_speed(_u64 task_id, _u64 file_id, void* user_data, LX_TAKE_OFF_FLUX_CALLBACK callback_fun, _u32 * p_action_id)
{
	_int32 ret_val = SUCCESS;
	LX_PT_TAKE_OFF_FLUX* p_action = NULL;
	_u32 http_id = 0;
	LX_TASK_INFO_EX * p_task_info = NULL;
	LX_FILE_INFO* p_file_info = NULL;
	char jumpkey[LX_JUMPKEY_MAX_SIZE] = {0};
	_u32 jumpkey_len = 0;
	
	EM_LOG_DEBUG( "lx_task_off_flux_from_high_speed:%llu",task_id );

	if(!lx_is_logined()) return -1;
	if(!em_is_net_ok(TRUE)||(sd_get_net_type()==UNKNOWN_NET)) return NETWORK_NOT_READY;

	p_task_info = lx_get_task_from_map(task_id);
	sd_assert(p_task_info!=NULL);
	if(p_task_info==NULL)
	{
		return INVALID_TASK_ID;
	}
	if( LXT_BT_ALL == p_task_info->_type )
	{
		if( 0 == file_id )
			return INVALID_FILE_ID;
		p_file_info = lx_get_file_from_map(&p_task_info->_bt_sub_files, file_id);
		if( NULL == p_file_info )
			return INVALID_FILE_ID;
	}
	
	ret_val = sd_malloc(sizeof(LX_PT_TAKE_OFF_FLUX), (void**) &p_action);
	CHECK_VALUE(ret_val);
	sd_memset(p_action,0x00,sizeof(LX_PT_TAKE_OFF_FLUX));

	p_action->_action._type = LPT_TAKE_OFF_FLUX;
	/* ------------- ��������cmd ------------ */
	/* ͷ�� */
	p_action->_req._cmd_header._version = LX_CMD_PROTOCAL_VERSION;
	p_action->_req._cmd_header._seq = lx_get_cmd_protocal_seq();
	// p_action._req._cmd_header._len �������
	p_action->_req._cmd_header._thunder_flag = 0x7000000;//get_product_flag();
	p_action->_req._cmd_header._compress_flag = 0;
	p_action->_req._cmd_header._cmd_type = LX_HS_CMD_TYPE_TASK_OFF_FLUX_REQ;
	/* �û���Ϣ*/
	p_action->_req._userid = (_u32)lx_get_base()->_userid;
	p_action->_req._main_taskid = task_id;
	p_action->_req._file_id = file_id;
	//sd_strncpy(p_action->_req._session_id, lx_get_base()->_session_id, MAX_SESSION_ID_LEN - 1 );
	ret_val = sd_get_os(p_action->_req._peer_id, MAX_OS_LEN);
	p_action->_req._business_flag = LX_BUSINESS_TYPE;
	p_action->_req._user_data = user_data;
	p_action->_req._callback_fun = callback_fun;
	if(SUCCESS != lx_get_jumpkey(jumpkey, &jumpkey_len))
	{
		SAFE_DELETE(p_action);
		return LXE_WRONG_REQUEST;
	}	
	p_action->_req._jump_key_len = jumpkey_len;
	sd_memcpy(p_action->_req._jump_key, jumpkey, jumpkey_len);
	
	p_action->_action._retry_num = 1;
	p_action->_action._req_buffer_len = MAX_POST_REQ_BUFFER_LEN;
	p_action->_action._req_data_len = p_action->_action._req_buffer_len;

	if( LXT_BT_ALL == p_task_info->_type )
		ret_val = lx_build_req_bt_file_take_off_flux_from_high_speed(p_action, p_file_info);
	else
		ret_val = lx_build_req_take_off_flux_from_high_speed(p_action, p_task_info);
	
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
		goto ErrHandle;
	}

	ret_val = lx_get_xml_file_store_path(p_action->_action._file_path);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}

	ret_val = lx_post_req((LX_PT *) p_action,&http_id,-1);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}
	p_action->_action._action_id = http_id;
	p_action->_action._state = LPS_RUNNING;
	
	lx_add_action_to_list((LX_PT * )p_action);

	*p_action_id = http_id;
	
 	return SUCCESS;
ErrHandle:
	return ret_val;
}

_int32 lx_take_off_flux_from_high_speed_resp(LX_PT * p_action)
 {
	_int32 ret_val = SUCCESS;
	LX_PT_TAKE_OFF_FLUX* p_action_ls = (LX_PT_TAKE_OFF_FLUX *)p_action;
	LX_TAKE_OFF_FLUX_RESULT* p_result = &p_action_ls->_resp;

	p_result->_action_id = p_action->_action_id;
	p_result->_user_data = p_action_ls->_req._user_data;
	p_result->_result = p_action->_error_code;
	p_result->_task_id = p_action_ls->_req._main_taskid;
	p_result->_file_id = p_action_ls->_req._file_id;
	
	EM_LOG_DEBUG( "lx_take_off_flux_from_high_speed_resp: errcode= %d",p_action->_error_code);
	if(p_action->_error_code == SUCCESS)
	{
		if(p_action->_file_id!=0)
		{
			sd_close_ex(p_action->_file_id);
			p_action->_file_id = 0;
		}
		ret_val = lx_parse_resp_take_off_flux_from_high_speed(p_action_ls);
		if(ret_val != SUCCESS) 
		{
			ret_val = lx_pt_high_speed_errcode_to_em_errcode(ret_val);
			EM_LOG_DEBUG( "lx_take_off_flux_from_high_speed_resp: result = %d", ret_val);
			p_result->_result = ret_val;
			goto ErrHandler;
		}
		
		if( (p_action->_resp_status == 0)|| (p_action->_resp_status == 200) )
		{
			/*  �ɹ� */
			p_result->_result = 0;
		}
		else
		{
			p_result->_result = p_action->_resp_status+p_action->_error_code;
			sd_assert( p_result->_result!=SUCCESS);
		}
	}
	
ErrHandler:

	/* �ص�֪ͨUI */
	if(NULL != p_action_ls->_req._callback_fun)
		p_action_ls->_req._callback_fun(p_result);
	
	if(p_action->_file_id!=0)
	{
		sd_close_ex(p_action->_file_id);
		p_action->_file_id = 0;
	}

	sd_delete_file(p_action->_file_path);
	
	SAFE_DELETE(p_action);

	return ret_val;
}

/* ����ҳ�л�ÿɹ����ص�url  */
_int32 lx_get_downloadable_url_from_webpage(const char* url,void* user_data,LX_DOWNLOADABLE_URL_CALLBACK callback_fun , _u32 * p_action_id)
{
	_int32 ret_val = SUCCESS;
	LX_DL_URL * p_dl_url_in_map = NULL;
	LX_DOWNLOADABLE_URL_RESULT *p_result = NULL,result ;
	//EM_DOWNLOADABLE_URL * p_dl_url = NULL;
	
	EM_LOG_DEBUG( "lx_get_downloadable_url_from_webpage" );

	if(!g_inited) return -1;

	if(sd_strlen(url)>=MAX_URL_LEN) return INVALID_URL;

	if(sd_strncmp(url, "http://",sd_strlen("http://"))!=0) return INVALID_URL;

	p_dl_url_in_map = lx_get_url_from_map( url);
	if(p_dl_url_in_map!=NULL)
	{
		/* ���ڱ��ػ����� */
		sd_memset(&result,0x00,sizeof(LX_DOWNLOADABLE_URL_RESULT));
		p_result = &result;

		p_result->_action_id = MAX_U32;
		p_result->_user_data = user_data;
		p_result->_result = 0;
		p_result->_url_num = p_dl_url_in_map->_url_num;
		sd_strncpy(p_result->_url,url,MAX_URL_LEN);

		callback_fun(p_result);
		*p_action_id = p_result->_action_id;
	}
	else
	{
		if(gp_current_sniff_action!=NULL)
		{
			if(sd_strcmp(gp_current_sniff_action->_url, url)==0)
			{
				/* ����ҳ������̽�� */
				ret_val = ETM_BUSY;
			}
			else
			{
				/* ȡ��֮ǰ����̽ */
				gp_current_sniff_action->_is_canceled = TRUE;

				/* ��̽����ҳ */
				ret_val = lx_get_downloadable_url_from_webpage_req(url,user_data,callback_fun , p_action_id);
			}
		}
		else
		{
			ret_val = lx_get_downloadable_url_from_webpage_req(url,user_data,callback_fun , p_action_id);
		}
	}
	
	return ret_val;
}
_int32 lx_get_downloadable_url_from_webpage_req(const char* url,void* user_data,LX_DOWNLOADABLE_URL_CALLBACK callback_fun , _u32 * p_action_id)
{
	_int32 ret_val = SUCCESS;
	LX_PT_GET_DL_URL * p_action = NULL;
	
	if(!em_is_net_ok(TRUE)||(sd_get_net_type()==UNKNOWN_NET)) return NETWORK_NOT_READY;
	
	EM_LOG_DEBUG( "lx_get_downloadable_url_from_webpage_req" );

	ret_val = sd_malloc(sizeof(LX_PT_GET_DL_URL), (void**) &p_action);
	CHECK_VALUE(ret_val);
	sd_memset(p_action,0x00,sizeof(LX_PT_GET_DL_URL));

	p_action->_action._type = LPT_GET_DL_URL;
	p_action->_user_data = user_data;
	p_action->_callback_fun = (void*)callback_fun;
	
	sd_strncpy(p_action->_url, url, MAX_URL_LEN-1);
	
	//p_action->_action._req_buffer_len = MAX_POST_REQ_BUFFER_LEN;
	//p_action->_action._req_data_len = p_action->_action._req_buffer_len;

	ret_val = lx_get_xml_file_store_path(p_action->_action._file_path);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}
	
	ret_val = lx_get_file_req((LX_PT *) p_action);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}

	lx_add_action_to_list((LX_PT * )p_action);

	*p_action_id = p_action->_action._action_id;
	
	gp_current_sniff_action = p_action;
	
	return SUCCESS;
}

 _int32 lx_get_downloadable_url_from_webpage_resp(LX_PT * p_action)
 {
 	_int32 ret_val = SUCCESS;
	LX_PT_GET_DL_URL * p_action_dl = (LX_PT_GET_DL_URL *)p_action;
	LX_DOWNLOADABLE_URL_RESULT *p_result = NULL,result ;
	LX_DOWNLOADABLE_URL_CALLBACK callback_fun = (LX_DOWNLOADABLE_URL_CALLBACK)p_action_dl->_callback_fun;
	EM_DOWNLOADABLE_URL * p_dl_url_array = NULL;

	EM_LOG_DEBUG( "lx_get_downloadable_url_from_webpage_resp" );
	
	gp_current_sniff_action = NULL;

	sd_memset(&result,0x00,sizeof(LX_DOWNLOADABLE_URL_RESULT));
	p_result = &result;

	p_result->_action_id = p_action->_action_id;
	p_result->_user_data = p_action_dl->_user_data;
	p_result->_result = p_action->_error_code;
	sd_strncpy(p_result->_url,p_action_dl->_url,MAX_URL_LEN);
	
	if(p_action->_error_code == SUCCESS)
	{

		ret_val = em_get_downloadable_url_from_webpage(p_action->_file_path,p_action_dl->_url,&p_result->_url_num,&p_dl_url_array);
		if(ret_val != SUCCESS) 
		{
			p_result->_result = ret_val;
			goto ErrHandler;
		}

	}
	
ErrHandler:

	/* �ص�֪ͨUI */
	//typedef _int32 ( *EM_DOWNLOADABLE_URL_CALLBACK)(EM_DOWNLOADABLE_URL_RESULT * p_result);
	if(!p_action_dl->_is_canceled)
	{
		callback_fun(p_result);
	}
	
	if(p_action->_file_id!=0)
	{
		sd_close_ex(p_action->_file_id);
		p_action->_file_id = 0;
	}

	sd_delete_file(p_action->_file_path);

	if(p_result->_result==SUCCESS)
	{
		/* �ӵ�map �� */
		ret_val = lx_add_url_to_map(p_action_dl->_url,p_result->_url_num,p_dl_url_array);
		if(ret_val!=SUCCESS)
		{
			SAFE_DELETE(p_dl_url_array);
		}
	}
	else
	{
		SAFE_DELETE(p_dl_url_array);
	}
	
	SAFE_DELETE(p_action);

	return SUCCESS;
 }

 _int32 lx_cancel_get_downloadable_url_from_webpage(LX_PT * p_action)
 {
 	gp_current_sniff_action = NULL;
 	return lx_cancel_action(p_action);
}

 _int32 lx_get_downloadable_url_from_webpage_file(const char* url,const char * source_file_path,_u32 * p_url_num)
 {
 	_int32 ret_val = SUCCESS;
	EM_DOWNLOADABLE_URL * p_dl_url_array = NULL;
	LX_DL_URL * p_dl_url_in_map = NULL;
	
	EM_LOG_DEBUG( "lx_get_downloadable_url_from_webpage_file:%s",url );
	
	if(!g_inited) return -1;

	if(sd_strlen(url)>=MAX_URL_LEN) return INVALID_URL;

	if(sd_strncmp(url, "http://",sd_strlen("http://"))!=0) return INVALID_URL;

	p_dl_url_in_map = lx_get_url_from_map( url);
	if(p_dl_url_in_map!=NULL)
	{
		/* ���ڱ��ػ����� */
		*p_url_num = p_dl_url_in_map->_url_num;
		EM_LOG_DEBUG( "lx_get_downloadable_url_from_webpage_file:already in map,resource num=%u" ,*p_url_num);
	}
	else
	{
	ret_val = em_get_downloadable_url_from_webpage(source_file_path,url,p_url_num,&p_dl_url_array);
	CHECK_VALUE(ret_val);

		EM_LOG_DEBUG( "lx_get_downloadable_url_from_webpage_file:get resource num=%u" ,*p_url_num);

	/* �ӵ�map �� */
	ret_val = lx_add_url_to_map(url,*p_url_num,p_dl_url_array);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_dl_url_array);
	}
	}
	return ret_val;
 }

 /* ��ȡ������̽������ҳ������ʽ*/
_int32 lx_get_detect_regex(void* user_data)
 {
 	_int32 ret_val = SUCCESS;
	LX_PT_GET_REGEX * p_action = NULL;
	
	EM_LOG_DEBUG( "lx_get_detect_regex" );

	// Ϊaction��Ϣ����ռ�
	ret_val = sd_malloc(sizeof(LX_PT_GET_REGEX), (void**) &p_action);
	CHECK_VALUE(ret_val);
	sd_memset(p_action,0x00,sizeof(LX_PT_GET_REGEX));

	p_action->_action._type = LPT_GET_REGEX;// action����
	p_action->_user_data = user_data;//�û���Ϣ
	
	sd_strncpy(p_action->_url, LX_DETECT_REGEX_CONFIG, MAX_URL_LEN-1);// LX_DETECT_REGEX_CONFIG
	
	//p_action->_action._req_buffer_len = MAX_POST_REQ_BUFFER_LEN;
	//p_action->_action._req_data_len = p_action->_action._req_buffer_len;

	// ��ȡ�ļ�����·��
	ret_val = lx_get_xml_file_store_path(p_action->_action._file_path);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}

	// ��ȡ�ļ�
	ret_val = lx_get_file_req((LX_PT *) p_action);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}

	lx_add_action_to_list((LX_PT * )p_action);

	//*p_action_id = p_action->_action._action_id;
	
	//gp_current_sniff_action = p_action;
	
	return SUCCESS;

 }
 
_int32 lx_get_detect_regex_resp(LX_PT * p_action)
{
 	_int32 ret_val = SUCCESS;
	LX_PT_GET_REGEX* p_resp = (LX_PT_GET_REGEX *)p_action;

	EM_LOG_DEBUG( "lx_get_detect_regex_resp" );
		
	if(p_action->_error_code == SUCCESS)
	{
		ret_val = lx_parse_detect_regex_xml(p_resp);

		if(ret_val != SUCCESS) 
		{
			goto ErrHandler;
		}

		ret_val = em_set_detect_regex(p_resp->_website_list);
	}
	
ErrHandler:

	// sd_delete_file(p_action->_file_path);//����xml�ļ���ɾ��

	SAFE_DELETE(p_action);

	return SUCCESS;

}

/* ��ȡ������̽������ҳ�Ķ�λ�ַ���*/
_int32 lx_get_detect_string(void* user_data)
 {
 	_int32 ret_val = SUCCESS;
	LX_PT_GET_DETECT_STRING* p_action = NULL;
	
	EM_LOG_DEBUG( "lx_get_detect_string" );

	// Ϊaction��Ϣ����ռ�
	ret_val = sd_malloc(sizeof(LX_PT_GET_DETECT_STRING), (void**) &p_action);
	CHECK_VALUE(ret_val);
	sd_memset(p_action,0x00,sizeof(LX_PT_GET_DETECT_STRING));

	p_action->_action._type = LPT_GET_DETECT_STRING;// action����
	p_action->_user_data = user_data;//�û���Ϣ
	
	sd_strncpy(p_action->_url, LX_DETECT_STRING_CONFIG, MAX_URL_LEN-1);// LX_DETECT_STRING_CONFIG
	
	//p_action->_action._req_buffer_len = MAX_POST_REQ_BUFFER_LEN;
	//p_action->_action._req_data_len = p_action->_action._req_buffer_len;

	// ��ȡ�ļ�����·��
	ret_val = lx_get_xml_file_store_path(p_action->_action._file_path);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}

	// ��ȡ�ļ�
	ret_val = lx_get_file_req((LX_PT *) p_action);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(p_action);
		CHECK_VALUE(ret_val);
	}

	lx_add_action_to_list((LX_PT * )p_action);

	//*p_action_id = p_action->_action._action_id;
	
	//gp_current_sniff_action = p_action;
	
	return SUCCESS;

 }

/*�÷��������ڽ����������ļ�����������̽����*/
 _int32 lx_add_detect_string(LX_PT_GET_DETECT_STRING *p_detect_string_after_parse)
 {
 	_int32 ret_val = SUCCESS;
 	EM_DETECT_STRING *p_detect_rule = NULL;
	EM_DETECT_SPECIAL_WEB *p_detect_website = NULL;

	/*��ʼ����̽����ṹ��*/
	ret_val = sd_malloc(sizeof(EM_DETECT_STRING), (void**) &p_detect_rule);
       sd_assert(ret_val == SUCCESS);
	sd_memset(p_detect_rule,0x00,sizeof(EM_DETECT_STRING));

	ret_val = sd_malloc(sizeof(EM_DETECT_SPECIAL_WEB), (void**) &p_detect_website);
       sd_assert(ret_val == SUCCESS);
       sd_memset(p_detect_website,0x00,sizeof(EM_DETECT_SPECIAL_WEB));
       ret_val = sd_malloc(sizeof(LIST), (void**)&p_detect_website->_rule_list);
       sd_assert(ret_val == SUCCESS);
       list_init(p_detect_website->_rule_list);

	/*������̽����*/
       p_detect_rule->_matching_scheme = 1;
       sd_strncpy(p_detect_rule->_string_start, "<td class=\"ltext ttitle\">", EM_DETECT_STRING_LEN);
       p_detect_rule->_string_start_include = 1;
       sd_strncpy(p_detect_rule->_string_end, "</tr>", EM_DETECT_STRING_LEN);
       p_detect_rule->_string_end_include = 1;
       sd_strncpy(p_detect_rule->_download_url_start, "ttitle\"><a href=\"", EM_DETECT_STRING_LEN);
       p_detect_rule->_download_url_start_include = 0;
       sd_strncpy(p_detect_rule->_download_url_end, "\"", EM_DETECT_STRING_LEN);
       p_detect_rule->_download_url_end_include = 0;
       sd_strncpy(p_detect_rule->_name_start, "target=\"_blank\">", EM_DETECT_STRING_LEN);
       p_detect_rule->_name_start_include = 0;
       sd_strncpy(p_detect_rule->_name_end, "</a>", EM_DETECT_STRING_LEN);
       p_detect_rule->_name_end_include = 0;
       sd_strncpy(p_detect_rule->_name_append_start, "", EM_DETECT_STRING_LEN);
       p_detect_rule->_name_append_start_include = 0;
       sd_strncpy(p_detect_rule->_name_append_end, "", EM_DETECT_STRING_LEN);
       p_detect_rule->_name_append_end_include = 0;
       sd_strncpy(p_detect_rule->_suffix_start, "", EM_DETECT_STRING_LEN);
       p_detect_rule->_suffix_start_include = 0;
       sd_strncpy(p_detect_rule->_suffix_end, "", EM_DETECT_STRING_LEN);
       p_detect_rule->_suffix_end_include = 0;
       sd_strncpy(p_detect_rule->_file_size_start, "<td>", EM_DETECT_STRING_LEN);
       p_detect_rule->_file_size_start_include = 0;
       sd_strncpy(p_detect_rule->_file_size_end, "</td>", EM_DETECT_STRING_LEN);
       p_detect_rule->_file_size_end_include = 0;
       
       /*������̽������̽��վ*/
	sd_strncpy(p_detect_website->_website, "bt.ktxp.com", EM_DETECT_STRING_LEN);
	list_push(p_detect_website->_rule_list, p_detect_rule);

	/*������̽��վ��ӵ��б�*/
	if( p_detect_string_after_parse->_website_list == NULL)
	{
		ret_val = sd_malloc(sizeof(LIST), (void**)&p_detect_string_after_parse->_website_list);
        sd_assert(ret_val == SUCCESS);
        list_init(p_detect_string_after_parse->_website_list);
	}
	list_push(p_detect_string_after_parse->_website_list, p_detect_website);

	/*�����ͷţ������̽��վ�б�ʱ���ͷ���������*/
	
	return ret_val;
 }
 
_int32 lx_get_detect_string_resp(LX_PT * p_action)
{
 	_int32 ret_val = SUCCESS;
	char * system_path = NULL;
	LX_PT_GET_DETECT_STRING* p_resp = (LX_PT_GET_DETECT_STRING *)p_action;

	EM_LOG_DEBUG( "lx_get_detect_string_resp" );
		
	if(p_action->_error_code == SUCCESS)
	{
		ret_val = lx_parse_detect_string_xml(p_resp);

		if(ret_val != SUCCESS) 
		{
			goto ErrHandler;
		}

		/*�����һЩû��д�������ļ��е���վ*/
		ret_val = lx_add_detect_string(p_resp);
		if(ret_val != SUCCESS) 
		{
			goto ErrHandler;
		}
				
		ret_val = em_set_detect_string(p_resp->_website_list);
	}
	else
	{
		system_path = em_get_system_path();
		sd_memset(p_resp->_action._file_path, 0x00, sizeof(p_resp->_action._file_path));
		ret_val = sd_snprintf(p_resp->_action._file_path, MAX_FULL_PATH_BUFFER_LEN-1, "%s/%s",system_path, LX_DEFAULT_DETECT_XML_NAME);
		if( sd_file_exist(p_resp->_action._file_path) )
		{
			ret_val = lx_parse_detect_string_xml(p_resp);
			if(ret_val != SUCCESS) 
			{
				goto ErrHandler;
			}

			/*�����һЩû��д�������ļ��е���վ*/
			ret_val = lx_add_detect_string(p_resp);
			if(ret_val != SUCCESS) 
			{
				goto ErrHandler;
			}
					
			ret_val = em_set_detect_string(p_resp->_website_list);
		}
		
	}
	
ErrHandler:

	// sd_delete_file(p_action->_file_path);//����xml�ļ���ɾ��

	SAFE_DELETE(p_action);

	return SUCCESS;

}

/*--------------------------------------------------------------------------*/
/*                Interfaces provid for external program				        */
/*--------------------------------------------------------------------------*/

/////14 ���߿ռ���ؽӿ�
//	ע��:����char *�ַ�����ΪUTF8��ʽ!

/* �����ؿ�����Ѹ���û��ʺ���Ϣ */
_int32 lixian_set_user_info(void * p_param)
{
	POST_PARA_7* _p_param = (POST_PARA_7*)p_param;
	_u64 * p_user_id = (_u64 * )_p_param->_para1;
	const char * user_name  = (const char * )_p_param->_para2;
	const char * old_user_name = (const char * )_p_param->_para3;
	_int32 vip_level = (_int32)_p_param->_para4;
	const char * session_id = (const char * )_p_param->_para5;
	char * jumpkey = (char * )_p_param->_para6;
	_u32 jumpkey_len  = (_u32)_p_param->_para7;
	
	EM_LOG_DEBUG("lixian_set_user_info:%llu,user_name=%s,old_user_name=%s,vip_level=%d,session_id=%s",*p_user_id,user_name,old_user_name,vip_level,session_id);

	_p_param->_result=lx_set_user_info( *p_user_id,user_name,old_user_name,vip_level,session_id);

	EM_LOG_DEBUG("lixian_set_jumpkey:%s", jumpkey);
	_p_param->_result=lx_set_jumpkey(jumpkey, jumpkey_len);
	
	EM_LOG_DEBUG("lixian_set_user_info, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 lixian_set_sessionid(void * p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	const char * session_id = (const char * )_p_param->_para1;
	
	EM_LOG_DEBUG("lixian_set_sessionid:session_id=%s", session_id);

	_p_param->_result = lx_set_sessionid_info( session_id);
	
	EM_LOG_DEBUG("lixian_set_sessionid, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

/* ��ȡ�û���Ϣ������¼���߿ռ� */
_int32 lixian_login(void * p_param)
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	LX_LOGIN_INFO* login_info = (LX_LOGIN_INFO*)_p_param->_para1;
	_u32* p_action_id = (_u32*)_p_param->_para2;
	
	EM_LOG_DEBUG("lixian_login");

	if(login_info->_user_id != 0)
		_p_param->_result = lx_set_user_info( login_info->_user_id,login_info->_new_user_name,login_info->_old_user_name,login_info->_vip_level,login_info->_session_id);
	else
		_p_param->_result = INVALID_ARGUMENT;

	if(login_info->_jumpkey != NULL && login_info->_jumpkey_len != 0)
		_p_param->_result = lx_set_jumpkey(login_info->_jumpkey, login_info->_jumpkey_len);
	else
		_p_param->_result = INVALID_ARGUMENT;

	if(lx_is_logined())
		_p_param->_result = lx_login(login_info->_user_data, login_info->_callback_fun, p_action_id);
	else
		_p_param->_result = LXE_NOT_LOGIN;
	
	EM_LOG_DEBUG("lixian_login, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}


/* ��ȡ�û���Ϣ������¼���߿ռ� */
_int32 lixian_has_list_cache(void * p_param)
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	_u64* user_id = (_u64*)_p_param->_para1;
	BOOL* is_cache = (BOOL*)_p_param->_para2;
	
	EM_LOG_DEBUG("lixian_has_list_cache");

	if(!lx_is_logined())
		_p_param->_result = -1;
	else
		_p_param->_result = lx_has_list_cache(*user_id, is_cache);
	
	EM_LOG_DEBUG("lixian_has_list_cache, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}


/* ��ȡ���߿ռ������б�
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
_int32 lixian_get_task_list(void * p_param)
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	LX_GET_TASK_LIST * param = (LX_GET_TASK_LIST * )_p_param->_para1;
	_u32 * p_action_id = (_u32 * )_p_param->_para2;
	//static _u32 time_stamp = 0;
	//_u32 cur_time = 0;
	
	EM_LOG_DEBUG("lixian_get_task_list");
#if 0
	em_time_ms(&cur_time);
	if(time_stamp==0)
	{
		time_stamp = cur_time;
	}
	else
	{
		if(TIME_SUBZ(cur_time,time_stamp)<3000)
		{
			/* Ϊ�˱���Ƶ�������������ڲ����ظ�ˢ�������б� */
			_p_param->_result= ETM_BUSY;
			goto ErrHandler;
		}

		time_stamp = cur_time;
	}
#endif
	_p_param->_result=lx_get_task_list( param,p_action_id,0);
	
//ErrHandler:	
	EM_LOG_DEBUG("lixian_get_task_list, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 lixian_get_overdue_or_deleted_task_list(void * p_param)
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	LX_GET_OD_OR_DEL_TASK_LIST * param = (LX_GET_OD_OR_DEL_TASK_LIST * )_p_param->_para1;
	_u32 * p_action_id = (_u32 * )_p_param->_para2;

	EM_LOG_DEBUG("lixian_get_overdue_or_deleted_task_list");

	_p_param->_result = lx_get_overdue_or_deleted_task_list( param,p_action_id, 0);
	
	EM_LOG_DEBUG("lixian_get_overdue_or_deleted_task_list, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 lixian_get_task_ids_by_state(void * p_param)
{
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	_int32 state = (_int32 )_p_param->_para1;
	_u64 * id_array_buffer = (_u64 * )_p_param->_para2;
	_u32 *buffer_len = (_u32 * )_p_param->_para3;
	
	EM_LOG_DEBUG("lixian_get_task_ids_by_state: state = %d",state);

	if(!lx_is_logined())
	{
		_p_param->_result = -1;
	}
	else
	if((g_geting_task_list )&&(lx_num_of_task_in_map()==0))
	{
		_p_param->_result = ETM_BUSY;
	}
	else
	{
		_p_param->_result=lx_get_task_ids_by_state( state,id_array_buffer,buffer_len);
	}
	
	EM_LOG_DEBUG("lixian_get_task_ids_by_state, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

/* ��ȡ����������Ϣ */
_int32 lixian_get_task_info(void * p_param)
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	_u64 * p_task_id = (_u64 * )_p_param->_para1;
	LX_TASK_INFO * p_info = (LX_TASK_INFO * )_p_param->_para2;
	
	EM_LOG_DEBUG("lixian_get_task_info:%llu",*p_task_id);

	if(!lx_is_logined())
	{
		_p_param->_result = -1;
	}
	else
	{
		_p_param->_result=lx_get_task_info( *p_task_id,p_info);
		EM_LOG_DEBUG("lixian_get_task_info: task_id = %llu, lixian_url = %s, origin_url = %s",p_info->_task_id, p_info->_url, p_info->_origin_url);
	}
	
	EM_LOG_DEBUG("lixian_get_task_info, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}


_int32 lixian_query_task_info(void * p_param)
{
	POST_PARA_5* _p_param = (POST_PARA_5*)p_param;
	_u64 *p_task_id = (_u64 * )_p_param->_para1;
	_u32 task_num = (_u32)_p_param->_para2;
	void* user_data = (void *)_p_param->_para3;
	LX_GET_TASK_LIST_CALLBACK callback_fun =  (LX_GET_TASK_LIST_CALLBACK)_p_param->_para4;
	_u32* p_action_id = (_u32*)_p_param->_para5;

	EM_LOG_DEBUG("lixian_query_task_info:%llu",*p_task_id);
	if(!lx_is_logined())
	{
		_p_param->_result = -1;
	}
	else
	{
		//_p_param->_result = lx_batch_query_task_info( p_task_id, task_num, user_data, callback_fun, p_action_id);
	}
	
	EM_LOG_DEBUG("lixian_query_task_info, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 lixian_query_bt_task_info(void * p_param)
{
	POST_PARA_5* _p_param = (POST_PARA_5*)p_param;
	_u64 *p_task_id = (_u64 * )_p_param->_para1;
	_u32 task_num = (_u32)_p_param->_para2;
	void* user_data = (void *)_p_param->_para3;
	LX_QUERY_TASK_INFO_CALLBACK callback_fun =  (LX_QUERY_TASK_INFO_CALLBACK)_p_param->_para4;
	_u32* p_action_id = (_u32*)_p_param->_para5;

	EM_LOG_DEBUG("lixian_query_task_info:%llu",*p_task_id);
	if(!lx_is_logined())
	{
		_p_param->_result = -1;
	}
	else
	{
		_p_param->_result = lx_query_bt_task_info( p_task_id, task_num, user_data, callback_fun, p_action_id);
	}
	
	EM_LOG_DEBUG("lixian_query_bt_task_info, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* ��ȡ���߿ռ�BT ��������ļ��б�
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
_int32 lixian_get_bt_task_file_list(void * p_param)
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	LX_GET_BT_FILE_LIST * param = (LX_GET_BT_FILE_LIST * )_p_param->_para1;
	_u32 * p_action_id = (_u32 * )_p_param->_para2;
	
	EM_LOG_DEBUG("lixian_get_bt_task_file_list");

	_p_param->_result=lx_get_bt_task_file_list( param,p_action_id,-1);
	
	EM_LOG_DEBUG("lixian_get_bt_task_file_list, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

///// ��ȡbt���������ļ�id �б�,*buffer_len�ĵ�λ��sizeof(u32)
_int32 lixian_get_bt_sub_file_ids(void * p_param)
{
	POST_PARA_4* _p_param = (POST_PARA_4*)p_param;
	_u64 * p_task_id = (_u64* )_p_param->_para1;
	_int32 state = (_int32 )_p_param->_para2;
	_u64 * id_array_buffer = (_u64 * )_p_param->_para3;
	_u32 *buffer_len = (_u32 * )_p_param->_para4;
	
	EM_LOG_DEBUG("lixian_get_bt_sub_file_ids:%llu",*p_task_id);

	if(!lx_is_logined())
	{
		_p_param->_result = -1;
	}
	else
	{
		_p_param->_result=lx_get_bt_sub_file_ids( *p_task_id,state,id_array_buffer,buffer_len);
	}
	
	EM_LOG_DEBUG("lixian_get_bt_sub_file_ids, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

/////��ȡ����BT������ĳ�����ļ�����Ϣ
_int32 lixian_get_bt_sub_file_info(void * p_param)
{
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	_u64 * p_task_id = (_u64 * )_p_param->_para1;
	_u64 * file_id = (_u64*  )_p_param->_para2;
	LX_FILE_INFO * p_file_info = (LX_FILE_INFO * )_p_param->_para3;
	
	EM_LOG_DEBUG("lixian_get_bt_sub_file_info:%llu,%llu",*p_task_id,file_id);

	if(!lx_is_logined())
	{
		_p_param->_result = -1;
	}
	else
	{
		_p_param->_result=lx_get_bt_sub_file_info( *p_task_id,*file_id,p_file_info);
	}
	
	EM_LOG_DEBUG("lixian_get_bt_sub_file_info, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* ��ȡ��Ƶ��ͼ,��ͼΪjpg��ʽ
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
_int32 lixian_get_screenshot(void * p_param)
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	LX_GET_SCREENSHOT * param = (LX_GET_SCREENSHOT * )_p_param->_para1;
	_u32 * p_action_id = (_u32 * )_p_param->_para2;
	
	EM_LOG_DEBUG("lixian_get_screenshot");

	_p_param->_result=lx_get_screenshot( param,p_action_id);
	
	EM_LOG_DEBUG("lixian_get_screenshot, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* ��ȡ��Ƶ�ļ����Ƶ㲥URL
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
_int32 lixian_get_vod_url(void * p_param)
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	LX_GET_VOD * param = (LX_GET_VOD * )_p_param->_para1;
	_u32 * p_action_id = (_u32 * )_p_param->_para2;
	
	EM_LOG_DEBUG("lixian_get_vod_url");

	_p_param->_result=lx_get_vod_url( param,p_action_id);
	
	EM_LOG_DEBUG("lixian_get_vod_url, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* �����߿ռ��ύ��������
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
_int32 lixian_create_task(void * p_param)
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	LX_CREATE_TASK_INFO* param = (LX_CREATE_TASK_INFO* )_p_param->_para1;
	_u32 * p_action_id = (_u32 * )_p_param->_para2;
	
	EM_LOG_DEBUG("lixian_create_task");

	_p_param->_result=lx_create_task( param, p_action_id, 1);
	
	EM_LOG_DEBUG("lixian_create_task, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

/* ���ѹ��ڵ����������߿ռ������ύ��������
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
_int32 lixian_create_task_again(void * p_param)
{
	POST_PARA_4* _p_param = (POST_PARA_4*)p_param;
	_u64* p_task_id = (_u64*)_p_param->_para1;
	void* p_userdata = (void *)_p_param->_para2;
	LX_CREATE_TASK_CALLBACK p_callback = (LX_CREATE_TASK_CALLBACK)_p_param->_para3;
	_u32 * p_action_id = (_u32 * )_p_param->_para4;
	
	EM_LOG_DEBUG("lixian_create_task_again");

	_p_param->_result = lx_create_task_again(*p_task_id, p_userdata, p_callback, p_action_id);
	
	EM_LOG_DEBUG("lixian_create_task_again, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

/* �����߿ռ��ύbt��������
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
_int32 lixian_create_bt_task(void * p_param)
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	LX_CREATE_BT_TASK_INFO* param = (LX_CREATE_BT_TASK_INFO* )_p_param->_para1;
	_u32 * p_action_id = (_u32 * )_p_param->_para2;
	
	EM_LOG_DEBUG("lixian_create_bt_task");

	_p_param->_result=lx_create_bt_task( param, p_action_id, 1);
	
	EM_LOG_DEBUG("lixian_create_bt_task, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* �����߿ռ�ɾ����������
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
_int32 lixian_delete_task(void * p_param)
{
	POST_PARA_4* _p_param = (POST_PARA_4*)p_param;
	_u64* p_task_id = (_u64*)_p_param->_para1;
	void* user_data = _p_param->_para2;
	LX_DELETE_TASK_CALLBACK callback_fun =  (LX_DELETE_TASK_CALLBACK)_p_param->_para3;
	_u32* p_action_id = (_u32*)_p_param->_para4;
	
	EM_LOG_DEBUG("lixian_delete_task:%llu",*p_task_id);

	_p_param->_result=lx_delete_task(*p_task_id, user_data, callback_fun, p_action_id, 1);
	
	EM_LOG_DEBUG("lixian_delete_task, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 lixian_delete_overdue_task(void * p_param)
{
	POST_PARA_4* _p_param = (POST_PARA_4*)p_param;
	_u64* p_task_id = (_u64*)_p_param->_para1;
	void* user_data = _p_param->_para2;
	LX_DELETE_TASK_CALLBACK callback_fun =  (LX_DELETE_TASK_CALLBACK)_p_param->_para3;
	_u32* p_action_id = (_u32*)_p_param->_para4;
	
	EM_LOG_DEBUG("lixian_delete_overdue_task:%llu",*p_task_id);

	_p_param->_result = lx_delete_overdue_task(*p_task_id, user_data, callback_fun, p_action_id, 1);
	
	EM_LOG_DEBUG("lixian_delete_overdue_task, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

/* �����߿ռ�����ɾ����������
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
_int32 lixian_delete_tasks(void * p_param)
{
	POST_PARA_6* _p_param = (POST_PARA_6*)p_param;
	_u32 task_num = (_u32)_p_param->_para1;
	_u64 * p_task_ids = (_u64 *) _p_param->_para2;
	BOOL is_overdue = (BOOL)_p_param->_para3;
	void * user_data = (void *)_p_param->_para4;
	LX_DELETE_TASKS_CALLBACK callback_fun =  (LX_DELETE_TASKS_CALLBACK)_p_param->_para5;
	_u32* p_action_id = (_u32*)_p_param->_para6;
	
	EM_LOG_DEBUG("lixian_delete_task:%u,is_overdue=%d",task_num,is_overdue);

	_p_param->_result=lx_delete_tasks(task_num,p_task_ids, is_overdue,user_data, callback_fun, p_action_id);
	
	EM_LOG_DEBUG("lixian_delete_tasks, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}


/* �����߿ռ�������������
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
_int32 lixian_delay_task(void * p_param)
{
	POST_PARA_4* _p_param = (POST_PARA_4*)p_param;
	_u64* p_task_id = (_u64*)_p_param->_para1;
	void* user_data = _p_param->_para2;
	LX_DELAY_TASK_CALLBACK callback_fun =  (LX_DELAY_TASK_CALLBACK)_p_param->_para3;
	_u32* p_action_id = (_u32*)_p_param->_para4;
	
	EM_LOG_DEBUG("lixian_delay_task:%llu",*p_task_id);

	_p_param->_result=lx_delay_task(*p_task_id, user_data, callback_fun, p_action_id, 1);
	
	EM_LOG_DEBUG("lixian_delay_task, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

/* ���߿ռ侫�������ѯ
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
_int32 lixian_miniquery_task(void * p_param)
{
	POST_PARA_4* _p_param = (POST_PARA_4*)p_param;
	_u64* p_task_id = (_u64*)_p_param->_para1;
	void* user_data = _p_param->_para2;
	LX_MINIQUERY_TASK_CALLBACK callback_fun =  (LX_MINIQUERY_TASK_CALLBACK)_p_param->_para3;
	_u32* p_action_id = (_u32*)_p_param->_para4;
	
	EM_LOG_DEBUG("lixian_miniquery_task:%llu",*p_task_id);

	_p_param->_result = lx_miniquery_task(*p_task_id, user_data, callback_fun, p_action_id);
	
	EM_LOG_DEBUG("lixian_miniquery_task, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

///// ����URL ,cid,gicd ��BT���ӵ�info_hash ���Ҷ�Ӧ����������id
_int32 lixian_get_task_id_by_eigenvalue( void * p_param )
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	EM_EIGENVALUE * p_eigenvalue = (EM_EIGENVALUE * )_p_param->_para1;
	_u64 * task_id = (_u64 * )_p_param->_para2;

	if(!lx_is_logined())
	{
		_p_param->_result = -1;
	}
	else
	{
		_p_param->_result = lx_get_task_id_by_eigenvalue( p_eigenvalue ,task_id);
	}

	EM_LOG_DEBUG("lixian_get_task_id_by_eigenvalue, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 lixian_get_task_id_by_gcid( void * p_param )
{
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	char * p_gcid = (char * )_p_param->_para1;
	_u64 * task_id = (_u64 * )_p_param->_para2;
	_u64 * file_id = (_u64 * )_p_param->_para3;

	if(!lx_is_logined())
	{
		_p_param->_result = -1;
	}
	else
	{
		_p_param->_result = lx_get_task_id_by_gcid( p_gcid ,task_id, file_id);
	}

	EM_LOG_DEBUG("lixian_get_task_id_by_gcid, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}
/* �����ؿ�����jump key*/
_int32 lixian_set_jumpkey(void * p_param)
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	char * jumpkey = (char * )_p_param->_para1;
	_u32 jumpkey_len  = (_u32)_p_param->_para2;
	
	EM_LOG_DEBUG("lixian_set_jumpkey:%s", jumpkey);

	_p_param->_result=lx_set_jumpkey(jumpkey, jumpkey_len);
	
	EM_LOG_DEBUG("lixian_set_jumpkey, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

/* ��ȡ�㲥ʱ��cookie */
_int32 lixian_get_cookie(void * p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	char * cookie_buffer = (char *)_p_param->_para1;
	
	EM_LOG_DEBUG("lixian_get_cookie");

	_p_param->_result=lx_get_cookie( cookie_buffer);
	
	EM_LOG_DEBUG("lixian_get_cookie, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

/* ��ȡ�û����߿ռ���Ϣ*/
_int32 lixian_get_space(void * p_param)
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	_u64 * p_max_space = (_u64 * )_p_param->_para1;
	_u64 * p_available_space  = (_u64*)_p_param->_para2;
	
	EM_LOG_DEBUG("lixian_get_space");

	if(g_geting_user_info)
	{
		_p_param->_result = ETM_BUSY;
	}
	else
	{
		_p_param->_result = lx_get_space(p_max_space, p_available_space);
	}
	
	EM_LOG_DEBUG("lixian_get_space, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}


/* ȡ��ĳ���첽���� */
_int32 lixian_cancel(void * p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	_u32 action_id = (_u32)_p_param->_para1;
	
	EM_LOG_DEBUG("lixian_cancel:%u",action_id);

	_p_param->_result=lx_cancel( action_id);
	
	EM_LOG_DEBUG("lixian_cancel, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

/* �˳����߿ռ� */
_int32 lixian_logout(void * p_param)
{
	POST_PARA_0* _p_param = (POST_PARA_0*)p_param;
	
	EM_LOG_DEBUG("lixian_logout");

	if(lx_is_logined())
	{
		_p_param->_result=lx_logout( );
	}
	
	EM_LOG_DEBUG("lixian_logout, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 lixian_get_free_strategy(void * p_param)
{
	POST_PARA_5* _p_param = (POST_PARA_5*)p_param;
	_u64* user_id = (_u64 * )_p_param->_para1;
	const char * session_id = (const char * )_p_param->_para2;
	void * user_data = (void * )_p_param->_para3;
	LX_FREE_STRATEGY_CALLBACK * callback_fun = (LX_FREE_STRATEGY_CALLBACK * )_p_param->_para4;
	_u32 * action_id = (_u32 * )_p_param->_para5;

	_p_param->_result = lx_get_free_strategy(*user_id, session_id, user_data ,callback_fun, action_id);

	EM_LOG_DEBUG("lixian_get_free_strategy, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 lixian_get_free_experience_member(void * p_param)
{
	POST_PARA_4* _p_param = (POST_PARA_4*)p_param;
	_u64 *user_id = (_u64 * )_p_param->_para1;
	void *user_data = (void * )_p_param->_para2;
	LX_FREE_EXPERIENCE_MEMBER_CALLBACK* callback_fun = (LX_FREE_EXPERIENCE_MEMBER_CALLBACK * )_p_param->_para3;
	_u32 *action_id = (_u32 * )_p_param->_para4;
	_u32 type = LX_CS_RESP_GET_FREE_DAY;
	
	_p_param->_result = lx_get_free_experience_member(type, *user_id, user_data ,callback_fun, action_id);

	EM_LOG_DEBUG("lixian_get_free_experience_member, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 lixian_get_experience_member_remain_time(void * p_param)
{
	POST_PARA_4* _p_param = (POST_PARA_4*)p_param;
	_u64 *user_id = (_u64 * )_p_param->_para1;
	void *user_data = (void * )_p_param->_para2;
	LX_FREE_EXPERIENCE_MEMBER_CALLBACK* callback_fun = (LX_FREE_EXPERIENCE_MEMBER_CALLBACK * )_p_param->_para3;
	_u32 *action_id = (_u32 * )_p_param->_para4;
	_u32 type = LX_CS_RESP_GET_REMAIN_DAY;
	
	_p_param->_result = lx_get_free_experience_member(type, *user_id, user_data ,callback_fun, action_id);

	EM_LOG_DEBUG("lixian_get_experience_member_remain_time, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 lixian_get_high_speed_flux(void * p_param)
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	LX_GET_HIGH_SPEED_FLUX* param = (LX_GET_HIGH_SPEED_FLUX* )_p_param->_para1;
	_u32 * action_id = (_u32 * )_p_param->_para2;

	_p_param->_result = lx_get_high_speed_flux(param, action_id);

	EM_LOG_DEBUG("lixian_get_high_speed_flux, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 lixian_take_off_flux_from_high_speed(void * p_param)
{
	POST_PARA_5* _p_param = (POST_PARA_5*)p_param;
	_u64* p_task_id = (_u64*)_p_param->_para1;
	_u64* p_file_id = (_u64*)_p_param->_para2;
	void* user_data = _p_param->_para3;
	LX_TAKE_OFF_FLUX_CALLBACK callback_fun =  (LX_TAKE_OFF_FLUX_CALLBACK)_p_param->_para4;
	_u32* p_action_id = (_u32*)_p_param->_para5;
	
	EM_LOG_DEBUG("lixian_task_off_flux_from_high_speed:%llu",*p_task_id);

	_p_param->_result = lx_take_off_flux_from_high_speed(*p_task_id, *p_file_id, user_data, callback_fun, p_action_id);
	
	EM_LOG_DEBUG("lixian_task_off_flux_from_high_speed, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

/* ��html�ļ��л�ÿɹ����ص�url  */
_int32 lixian_get_downloadable_url_from_webpage(void * p_param)
{
	POST_PARA_4* _p_param = (POST_PARA_4*)p_param;
	const char* url = (const char*)_p_param->_para1;
	void* user_data = (void*)_p_param->_para2;
	LX_DOWNLOADABLE_URL_CALLBACK callback_fun = (LX_DOWNLOADABLE_URL_CALLBACK)_p_param->_para3;
	_u32 * p_action_id = (_u32 *)_p_param->_para4;
	
	EM_LOG_DEBUG("lixian_get_downloadable_url_from_webpage:%s",url);

	_p_param->_result=lx_get_downloadable_url_from_webpage(url,user_data, callback_fun,p_action_id);
	
	EM_LOG_DEBUG("lixian_get_downloadable_url_from_webpage, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}
/* ���Ѿ���ȡ������ҳԴ�ļ�����̽�ɹ����ص�url ,ע��,�����ͬ���ӿڣ�����Ҫ���罻��  */
_int32 lixian_get_downloadable_url_from_webpage_file(void * p_param)
{
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	const char* url = (const char*)_p_param->_para1;
	const char * source_file_path = (const char*)_p_param->_para2;
	_u32 * p_url_num = (_u32 *)_p_param->_para3;

	EM_LOG_DEBUG("lixian_get_downloadable_url_from_webpage_file:%s",url);

	_p_param->_result=lx_get_downloadable_url_from_webpage_file(url,source_file_path, p_url_num);
	
	EM_LOG_DEBUG("lixian_get_downloadable_url_from_webpage_file, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

///// ��ȡ��ҳ�пɹ�����URL��id �б�,*buffer_len�ĵ�λ��sizeof(u32)
_int32 lixian_get_downloadable_url_ids(void * p_param)
{
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	const char* url = (const char* )_p_param->_para1;
	_u32 * id_array_buffer = (_u32 * )_p_param->_para2;
	_u32 *buffer_len = (_u32 * )_p_param->_para3;
	
	EM_LOG_DEBUG("lixian_get_downloadable_url_ids:%s",url);

	if(!g_inited) 
	{
		_p_param->_result= -1;
	}
	else
	{
		_p_param->_result=lx_get_downloadable_url_ids( url,id_array_buffer,buffer_len);
	}
	
	EM_LOG_DEBUG("lixian_get_downloadable_url_ids, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

/* ��ȡ�ɹ�����URL ��Ϣ */
_int32 lixian_get_downloadable_url_info(void * p_param)
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	_u32 url_id = (_u32 )_p_param->_para1;
	EM_DOWNLOADABLE_URL * p_info = (EM_DOWNLOADABLE_URL * )_p_param->_para2;
	
	EM_LOG_DEBUG("lixian_get_downloadable_url_info:%u",url_id);

	if(!g_inited) 
	{
		_p_param->_result= -1;
	}
	else
	{
		_p_param->_result=lx_get_downloadable_url_info(url_id,p_info);
	}
	
	EM_LOG_DEBUG("lixian_get_downloadable_url_info, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}


/*--------------------------------------------------------------------------*/
/*                Internal functions				        */
/*--------------------------------------------------------------------------*/
_int32 lx_check_if_pic_exist(LX_PT * p_action)
{
	_int32 ret_val = SUCCESS,index = 0,exist_pic_num = 0;
	LX_PT_SS * p_action_ss = (LX_PT_SS *)p_action;
	char path_buffer[MAX_FULL_PATH_BUFFER_LEN] = {0},*p_pos = NULL;
	char gcid_str[64]={0};
	_u8 * p_gcid = NULL;

	sd_strncpy(path_buffer, p_action_ss->_resp._store_path, MAX_FULL_PATH_BUFFER_LEN-1);
	p_pos = path_buffer+sd_strlen(p_action_ss->_resp._store_path);
	
	p_gcid = p_action_ss->_req._gcid_array;
	for(index = 0; index<p_action_ss->_req._file_num;index++)
	{
		str2hex((char*)p_gcid, CID_SIZE, gcid_str, 64);
		gcid_str[40] = '\0';
		sd_snprintf(p_pos, LX_PT_GCID_NODE_LEN, "/%s.jpg", gcid_str);
		if(sd_file_exist(path_buffer))
		{
			exist_pic_num++;
			*(p_action_ss->_resp._result_array+index) = SUCCESS;
		}
		p_gcid+=CID_SIZE;
	}

	if(exist_pic_num == p_action_ss->_req._file_num)
	{
		//������,����Ҫ�ٴ�������
		//Error # 17: File exists
		return 17;
	}
	else
	{
		//��ͼƬ������,����ȥ���������ػ���
		sd_assert(exist_pic_num < p_action_ss->_req._file_num);
		p_action_ss->_need_dl_num = p_action_ss->_req._file_num-exist_pic_num;
	}
	return ret_val;
}

_int32 lx_set_screenshot_result(LX_PT * p_action,_int32 result)
{
	_int32 index = 0;
	LX_PT_SS * p_ss = (LX_PT_SS *)p_action;

	for(index = 0;index<p_ss->_req._file_num;index++)
	{
		if(*(p_ss->_resp._result_array+index)!=SUCCESS)
		{
			*(p_ss->_resp._result_array+index) = result;
		}
	}

	return SUCCESS;
}

BOOL lx_check_is_all_gcid_parse_failed(LX_PT * p_action)
{
	_int32 index = 0,failed_num = 0;
	LX_PT_SS * p_ss = (LX_PT_SS *)p_action;
	LX_DL_FILE * p_dl = p_ss->_dl_array;

	for(index = 0;index<p_ss->_need_dl_num;index++)
	{
		if(p_dl->_action._error_code!=SUCCESS)
		{
			*(p_ss->_resp._result_array+p_dl->_gcid_index) = p_dl->_action._error_code;
			failed_num++;
		}
		p_dl++;
	}

	if(failed_num==p_ss->_need_dl_num)
		return TRUE;

	return FALSE;
}

 _int32 em_update_regex_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid)
 {
	EM_LOG_DEBUG("em_update_regex_timeout");

	if(errcode == MSG_TIMEOUT)
	{
		if(/*msg_info->_device_id*/msgid == g_update_regex_timer_id)
		{
#ifdef ENABLE_REGEX_DETECT
			_int32 ret_val = lx_get_detect_regex(NULL);
			if(ret_val!=SUCCESS)
			{
			    em_set_detect_regex(NULL);
                /* Start the update_regex */  // 
                ret_val = em_start_timer(em_update_regex_timeout, NOTICE_ONCE,5*1000, 0,NULL,&g_update_regex_timer_id);  
                sd_assert(ret_val==SUCCESS);
                return SUCCESS;
			}

			/* Start the update_regex */  // 
			ret_val = em_start_timer(em_update_regex_timeout, NOTICE_ONCE,2*3600*1000, 0,NULL,&g_update_regex_timer_id);  
			sd_assert(ret_val==SUCCESS);

#endif /* ENABLE_REGEX_DETECT */    
		}	
	}
		
	EM_LOG_DEBUG("em_update_regex_timeout end!");
	return SUCCESS;
 }


 _int32 em_update_detect_string_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid)
 {
	EM_LOG_DEBUG("em_update_detect_string_timeout");

	if(errcode == MSG_TIMEOUT)
	{
		if(/*msg_info->_device_id*/msgid == g_update_detect_string_timer_id)
		{
#ifdef ENABLE_STRING_DETECT
			_int32 ret_val = lx_get_detect_string(NULL);
			if(ret_val!=SUCCESS)
			{
			    em_set_detect_string(NULL);
                /* Start the update_detect_string */  // 
                ret_val = em_start_timer(em_update_detect_string_timeout, NOTICE_ONCE,5*1000, 0,NULL,&g_update_detect_string_timer_id);  
                sd_assert(ret_val==SUCCESS);
                return SUCCESS;
			}

			/* Start the update_detect_string */  // 
			ret_val = em_start_timer(em_update_detect_string_timeout, NOTICE_ONCE,2*3600*1000, 0,NULL,&g_update_detect_string_timer_id);  
			sd_assert(ret_val==SUCCESS);

#endif /* ENABLE_STRING_DETECT */    
		}	
	}
		
	EM_LOG_DEBUG("em_update_detect_string_timeout end!");
	return SUCCESS;
 }



#else
/* ������Ϊ����MACOS����˳������ */
#if defined(MACOS)&&(!defined(MOBILE_PHONE))
void lx_test(void)
{
	return ;
}
#endif /* defined(MACOS)&&(!defined(MOBILE_PHONE)) */
#endif /* ENABLE_LIXIAN */

