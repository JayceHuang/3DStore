/*--------------------------------------------------------------------------*/
/*                               IDENTIFICATION                             */
/*--------------------------------------------------------------------------*/
/*     Filename  : scheduler.c                                         */
/*     Author     : ZengYuqing                                              */
/*     Project    : EmbedThunder                                        */
/*     Module     : task_manager													*/
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
/* This file contains the procedure of task manager                       */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*                              HISTORY                                     */
/*--------------------------------------------------------------------------*/
/*   Date     |    Author   | Modification                                  */
/*--------------------------------------------------------------------------*/
/* 2008.07.31 | ZengYuqing  | Creation                                      */
/*--------------------------------------------------------------------------*/


#include "scheduler/scheduler.h"

#include "em_asyn_frame/em_asyn_frame.h"
#include "em_common/em_queue.h"
#include "em_common/em_time.h"
#include "em_common/em_utility.h"
#include "em_common/em_bytebuffer.h"

#include "utility/peerid.h"
#include "utility/local_ip.h"
#include "utility/url.h"

#include "settings/em_settings.h"

#include "download_manager/download_task_interface.h"
#include "download_manager/mini_task_interface.h"
#include "tree_manager/tree_interface.h"
#include "vod_task/vod_task.h"
#include "tree_manager/tree_impl.h"

#include "em_common/em_version.h"
#include "em_interface/em_customed_interface.h"
#include "em_interface/em_fs.h"

#include "et_interface/et_interface.h"

//#include "em_torrent_parser/em_torrent_parser_interface.h"
#include "torrent_parser/torrent_parser_interface.h"

#include "platform/sd_network.h"

#ifdef ENABLE_LIXIAN_ETM
#include "lixian/lixian_interface.h"
#endif /* ENABLE_LIXIAN_ETM */

#ifdef ENABLE_MEMBER
#include "member/member_interface.h"
#endif /* ENABLE_MEMBER */

#include "em_common/em_logid.h"
#ifdef EM_LOGID
#undef EM_LOGID
#endif

#define EM_LOGID EM_LOGID_SCHEDULER
#include "em_common/em_logger.h"

extern  _int32 et_get_network_flow(_u32 * download,_u32 * upload);

#define EM_SCHEDULER_TIME_OUT 1000    
#define EM_WAIT_MAC_TIME_OUT 5000    
#define  MAX_REPORT_AGAIN_NUM 1
/*--------------------------------------------------------------------------*/
/*                Global declarations				        */
/*--------------------------------------------------------------------------*/
static EM_NOTIFY_TASK_STATE_CHANGED em_task_state_changed_callback_ptr=NULL;

static EM_NOTIFY_NET_CONNECT_RESULT em_notify_net_callback_ptr=NULL;

static _u32 g_scheduler_timer_id=0;
static _u32 g_do_next_msg_id=0;
static char g_system_path[MAX_FILE_PATH_LEN];
static BOOL g_et_running = FALSE;
static BOOL g_et_wait_mac = FALSE;
static _u32 g_et_wait_mac_timer_id=0;
static BOOL g_is_license_verified = FALSE;
static _u32  g_license_result=-1;
static _u32  g_license_expire_time=0;

static BOOL g_task_auto_start = FALSE;
static BOOL g_need_start_et = FALSE;
static BOOL g_need_notify_net = FALSE;
static BOOL g_need_init_net = FALSE;

static _u32  g_et_idle_time_stamp=0;
static _u32  g_uinit_net_time_stamp=0;

static _int32  g_network_state=0;  // 0 - not init, 1- initiating ,2 - ok ,3 - user abort , other -failed
static BOOL g_user_set_netok = FALSE;
static BOOL g_is_install = FALSE;
static BOOL g_is_ui_reported = FALSE;
static _u32 g_http_report_tree_id = 0;
static LIST g_http_report_list;

#ifdef _LOGGER
static _int32 g_reload_log_interval=60;
static _u32 g_log_reload_timer_id=0;
#endif

#ifdef ENABLE_ETM_MEDIA_CENTER
static _int32 g_upgrade_check_interval=36000;
static _u32 g_upgrade_check_timer_id=0;
static EM_SYS_VOLUME_INFO *g_default_sys_volume_info = NULL;
static EM_SYS_VOLUME_INFO *g_default_download_volume_info = NULL;
static LIST g_sys_volume_info_list;
static BOOL g_is_enable_dynmic_volume = FALSE;
static BOOL g_is_multi_disk = FALSE;
#endif
/*--------------------------------------------------------------------------*/
/*                Internal functions				        */
/*--------------------------------------------------------------------------*/
_int32 em_init( void * _param )
{
	_int32 ret_val = SUCCESS;
	char version_buffer[64];
	char * system_path = (char *)_param;

	em_get_version(version_buffer, sizeof(version_buffer));
#ifdef _DEBUG
	URGENT_TO_FILE("em_init:version=%s,system_path=%s",version_buffer,system_path);
#endif
	/* Ԥռ1MB �Ŀռ�����ȷ��������˳������ */
	ret_val = em_ensure_free_disk( system_path);
	CHECK_VALUE(ret_val);

	ret_val=em_test_path_writable(system_path);
	CHECK_VALUE(ret_val);
	em_memset(g_system_path,0,MAX_FILE_PATH_LEN);
	em_strncpy(g_system_path, system_path, MAX_FILE_PATH_LEN);
	
	g_network_state = 0;
	g_need_start_et = FALSE;
	em_notify_net_callback_ptr = NULL;
	g_need_notify_net = FALSE;
	g_need_init_net = FALSE;
	g_uinit_net_time_stamp = 0;
	list_init(&g_http_report_list);
	
	/* ���ؿ�ȫ�ֻ���ģ��ĳ�ʼ�� */
	ret_val=em_basic_init();
	CHECK_VALUE(ret_val);

	/* ���ؿ⹦��ģ��ĳ�ʼ�� */
	ret_val=em_sub_module_init();
	if(ret_val!=SUCCESS) goto ErrorHanle_sub;
	
	/* ���ؿ����ģ��ĳ�ʼ�� */
	ret_val=em_other_module_init();
	if(ret_val!=SUCCESS) goto ErrorHanle_other;
	
	/* task_manager ģ��ĳ�ʼ�� */
	ret_val=em_init_task_manager(_param );
	if(ret_val!=SUCCESS) goto ErrorHanle_tm;

	g_do_next_msg_id=0;
			
#if defined(__SYMBIAN32__)
	ret_val=dt_load_tasks();
	if(ret_val==SUCCESS)
	{
		ret_val=trm_load_default_tree(	);
	}
#endif

	if(ret_val==SUCCESS)
	{
		ret_val=em_post_next(em_do_next,0);
	}
	if(ret_val!=SUCCESS) goto ErrorHanle_next;
	
	EM_LOG_DEBUG("tm_init SUCCESS ");
	return SUCCESS;
	
ErrorHanle_next:
	em_uninit_task_manager();
ErrorHanle_tm:
	em_other_module_uninit();
ErrorHanle_other:
	em_sub_module_uninit();
ErrorHanle_sub:
	em_basic_uninit();
	
	EM_LOG_DEBUG("em_init ERROR:%d ",ret_val);
	return ret_val;
}

_int32 em_uninit(void* _param)
{
	_int32 ret_val = SUCCESS;

	EM_LOG_DEBUG("em_uninit...");

	if(g_http_report_tree_id != 0)
	{
		trm_close_tree_by_id(g_http_report_tree_id);
	}
	
	em_http_report_clear_action_list();

	if(g_do_next_msg_id!=0)
	{
		em_cancel_message_by_msgid(g_do_next_msg_id);
		g_do_next_msg_id=0;
	}
	
	ret_val=em_uninit_task_manager();
	CHECK_VALUE(ret_val);
	
	em_other_module_uninit();

	em_sub_module_uninit();
#if defined(ENABLE_NULL_PATH)
      //
      	em_settings_uninitialize( );
#else
	em_basic_uninit();
#endif	
#ifdef _DEBUG
	EM_LOG_URGENT("em_uninit SUCCESS Bye-bye!");
#endif	
	em_uninit_sys_path_info();

	return SUCCESS;
}

#ifdef ENABLE_ETM_MEDIA_CENTER
static _int32 _push_volume(LIST *list, EM_SYS_VOLUME_INFO *p_volume_info)
{
	LIST_ITERATOR cur_node;
	EM_SYS_VOLUME_INFO *p_data;
	int32 ret_val=SUCCESS;

	for (cur_node = LIST_BEGIN(*list); 
		 cur_node != LIST_END(*list);
		 cur_node = LIST_NEXT(cur_node))
	{
		p_data = (EM_SYS_VOLUME_INFO*)cur_node->_data;
		if (p_data->_volume_index > p_volume_info->_volume_index)
		{
			ret_val = list_insert(list, p_volume_info, cur_node);
			CHECK_VALUE(ret_val);
			break;
		}
	}

	if (cur_node == LIST_END(*list))
	{
		ret_val = list_push(list, p_volume_info);
		CHECK_VALUE(ret_val);
	}

	return ret_val;
}
#endif


_int32 em_init_sys_path_info()
{
	_int32 ret_val = SUCCESS;
	
#ifdef ENABLE_ETM_MEDIA_CENTER
	_u32 file_num = 0;
	FILE_ATTRIB *sub_files = NULL;
	u32 i = 0, volume_index = 1, volume_setting = 0;
	char system_disk[MAX_FILE_PATH_LEN] = {0};
	char link_path[MAX_FILE_PATH_LEN] = {0};
	char download_path[MAX_FILE_PATH_LEN] = {0};
	char sub_file_full_path[MAX_FULL_PATH_BUFFER_LEN] = {0};
	char remount_cmd[MAX_FULL_PATH_BUFFER_LEN * 2] = {0};
	_u32 path_len = 0;
	char *p_tmp = NULL;
	LIST_NODE *list_node = NULL;
	EM_SYS_VOLUME_INFO *p_volume_info = NULL;
	
	EM_LOG_DEBUG("em_init_sys_path_info");
	ret_val = sd_get_sub_files(EM_SYSTEM_VOLUME, NULL, 0, &file_num);
	ret_val = sd_malloc(sizeof(FILE_ATTRIB) * file_num, (void **)&sub_files);
	CHECK_VALUE(ret_val);

	em_list_init( &g_sys_volume_info_list );
	ret_val = sd_get_sub_files(EM_SYSTEM_VOLUME, sub_files, file_num, &file_num);
	if( ret_val != SUCCESS )
	{
		ret_val = INVALID_SYSTEM_PATH;
		goto ErrHandle;
	}
	
	if( file_num > EM_SYSTEM_VOLUME_MAX_NUM )
	{
		ret_val = NOT_ENOUGH_BUFFER;
		goto ErrHandle;
	}

	// ���ҵ�ϵͳĿ¼���ڵ�Ӳ��
	for (i=0;i<file_num;i++)
	{
		p_tmp = NULL;
		if (sub_files[i]._is_dir)
			continue;
		if ('.' == sub_files[i]._name[0])
			continue;
		
		em_memset( link_path, 0, MAX_FILE_PATH_LEN );
		
		em_strncpy(sub_file_full_path, EM_SYSTEM_VOLUME, MAX_FULL_PATH_BUFFER_LEN);
		em_strcat(sub_file_full_path, sub_files[i]._name, MAX_FULL_PATH_BUFFER_LEN - em_strlen(EM_SYSTEM_VOLUME) - 1);
		path_len = readlink(sub_file_full_path, link_path, MAX_FILE_PATH_LEN);

		if (path_len == -1)
		{
			EM_LOG_DEBUG("file %s is not a soft link", sub_file_full_path);
			continue;
		}

		EM_LOG_DEBUG("find label[%s], link to %s", sub_files[i]._name, link_path);
		
		p_tmp = em_strstr( g_system_path, link_path, 0 );
		if( p_tmp == g_system_path )
		{
			em_strncpy( system_disk, link_path, MAX_FILE_PATH_LEN );
			if (system_disk[path_len-1] == '/')
			{
				system_disk[path_len-1]='\0';
				path_len --;
			}
			
			if ('0' > system_disk[path_len-1] || system_disk[path_len-1] > '9')
			{
				ret_val = INVALID_SYSTEM_PATH;
				EM_LOG_ERROR("link %s is an invalid volume path", link_path);
				goto ErrHandle;
			}
			else
			{
				system_disk[path_len-1] = '\0';
			}
			break;
		}
	}

	// ��û���ҵ�Ӳ��?
	if( em_strlen(system_disk)==0 )
	{
		EM_LOG_ERROR("can not find system path(%s) in any volumes", g_system_path);
		ret_val = INVALID_SYSTEM_PATH;
		goto ErrHandle;
	}
	
	em_settings_get_int_item("system.download_volume_index", (_int32*)&volume_setting );

	//  ��ʼ�����е��б�
	for (i=0;i<file_num;i++)
	{
		p_tmp = NULL;
		if (sub_files[i]._is_dir)
			continue;
		if ('.' == sub_files[i]._name[0])
			continue;
		
		em_memset( link_path, 0, MAX_FILE_PATH_LEN );
		
		em_strncpy(sub_file_full_path, EM_SYSTEM_VOLUME, MAX_FULL_PATH_BUFFER_LEN);
		em_strcat(sub_file_full_path, sub_files[i]._name, MAX_FULL_PATH_BUFFER_LEN - em_strlen(EM_SYSTEM_VOLUME) - 1);
		path_len = readlink(sub_file_full_path, link_path, MAX_FILE_PATH_LEN);

		if (path_len == -1)
		{
			EM_LOG_DEBUG("file %s is not a soft link", sub_file_full_path);
			continue;
		}

		EM_LOG_DEBUG("find label[%s], link to %s", sub_files[i]._name, link_path);

		p_tmp = em_strstr(link_path, system_disk, 0);
		if (p_tmp == NULL || em_strlen(link_path) <= em_strlen(system_disk))
		{
			EM_LOG_DEBUG("label[%s] link to %s is not in system_disk %s", 
				sub_files[i]._name, link_path, system_disk);
			g_is_multi_disk = TRUE;
			continue;
		}

		// �ҵ�һ������
		ret_val = sd_malloc( sizeof(EM_SYS_VOLUME_INFO), (void **)&p_volume_info );
		if( ret_val != SUCCESS )
		{
			goto ErrHandle;
		}

		volume_index = link_path[sd_strlen(system_disk)] - '0';
		p_volume_info->_volume_index = volume_index;
		
		em_strncpy( p_volume_info->_volume_name, sub_files[i]._name, MAX_SYS_VOLUME_LEN );
		p_tmp = em_strchr( p_volume_info->_volume_name, ':', 0 );
		if( p_tmp != NULL )
		{
			*p_tmp = '\0';
		}
		
		em_strncpy( p_volume_info->_volume_path,link_path, MAX_FILE_PATH_LEN );
		path_len = em_strlen(p_volume_info->_volume_path);

		ret_val = _push_volume( &g_sys_volume_info_list, p_volume_info );
		if( ret_val != SUCCESS )
		{
			sd_free(p_volume_info);
			p_volume_info = NULL;
			goto ErrHandle;
		}
		EM_LOG_DEBUG("em_init_sys_path_info, index:%u, volume:%s, path:%s", 
			p_volume_info->_volume_index, p_volume_info->_volume_name, p_volume_info->_volume_path);

		// ����mount�ɿɶ�д����
		em_snprintf(remount_cmd, sizeof(remount_cmd), "/bin/mount -o remount,rw %s", p_volume_info->_volume_path); 
		ret_val = system(remount_cmd);
		EM_LOG_DEBUG("system(%s) return %d", remount_cmd, ret_val);		
		
		p_tmp = em_strstr( g_system_path, link_path, 0 );
		if( p_tmp == g_system_path )
		{
			g_default_sys_volume_info = p_volume_info;
			EM_LOG_DEBUG("system volume_index is %d", volume_index);

			if (volume_setting==0)
			{
				volume_setting = volume_index;
				em_settings_set_int_item("system.download_volume_index", volume_index );
				g_default_download_volume_info = p_volume_info;
			}
		}
		
		if( volume_setting==volume_index )
		{
			g_default_download_volume_info = p_volume_info;
		}
	}

	if (g_default_download_volume_info == NULL)
	{
		EM_LOG_ERROR("cannot find default download volume:%d",  volume_setting);
		list_node = LIST_BEGIN(g_sys_volume_info_list);
		g_default_download_volume_info = (EM_SYS_VOLUME_INFO *) LIST_VALUE(list_node);
		em_settings_set_int_item("system.download_volume_index", g_default_download_volume_info->_volume_index);
	}

	g_is_enable_dynmic_volume = TRUE;
	em_get_download_path_imp(download_path);

	if( !em_dir_exist(download_path) )
	{
		ret_val = em_mkdir(download_path);
		if( ret_val != SUCCESS )
		{
			sd_assert( FALSE );
			goto ErrHandle;
		}
	}
	
	EM_LOG_DEBUG("em_init_sys_path_info, sys_volume:%s, sys_path:%s, download_path_volume:%s, download_path_path:%s", 
		g_default_sys_volume_info->_volume_name, g_default_sys_volume_info->_volume_path,
		g_default_download_volume_info->_volume_name, g_default_download_volume_info->_volume_path);

	SAFE_DELETE(sub_files);
	return SUCCESS;		
ErrHandle:
	SAFE_DELETE(sub_files);
	if( ret_val != SUCCESS )
	{
		g_is_enable_dynmic_volume = FALSE;
		em_uninit_sys_path_info();
	}
#endif
	return ret_val;
}


_int32 em_uninit_sys_path_info()
{
#ifdef ENABLE_ETM_MEDIA_CENTER
	EM_LOG_DEBUG("em_uninit_sys_path_info");
	LIST_ITERATOR cur_node = LIST_BEGIN(g_sys_volume_info_list);
	EM_SYS_VOLUME_INFO *p_volume_info = NULL;

	while( cur_node != LIST_END(g_sys_volume_info_list) )
	{
		p_volume_info = (EM_SYS_VOLUME_INFO *)LIST_VALUE(cur_node);
		SAFE_DELETE(p_volume_info);
		cur_node = LIST_NEXT( cur_node );
	}
	g_default_sys_volume_info = NULL;
	g_default_download_volume_info = NULL;

#endif
	return SUCCESS;
}


_int32 em_post_next(msg_handler handler,_u32 timeout)
{
	_int32 ret_val = SUCCESS;
	MSG_INFO msg_info;

	if(g_do_next_msg_id!=0) return MSG_ALREADY_EXIST;
	
	msg_info._device_id = 0;
	msg_info._device_type = DEVICE_COMMON;
	msg_info._msg_priority = MSG_PRIORITY_NORMAL;
	msg_info._operation_parameter = NULL;
	msg_info._operation_type = OP_COMMON;
	msg_info._pending_op_count = 0;
	msg_info._user_data = NULL;

	ret_val = em_post_message(&msg_info, handler, NOTICE_ONCE, timeout, &g_do_next_msg_id);

	return ret_val;
}
_int32 em_do_next(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid)
{
	_u32 iap_id = 0;
	EM_LOG_DEBUG("em_do_next");

	//if(errcode == MSG_TIMEOUT)
	{
		if(/*msg_info->_device_id*/msgid == g_do_next_msg_id)
		{
//#if defined( MOBILE_PHONE)
//		#if defined(_ANDROID_LINUX)||defined(MACOS)
//		g_need_init_net = TRUE;
//		#endif
//#endif
			g_do_next_msg_id=0;
			if(g_need_init_net)
			{
				g_need_init_net=FALSE;
				if(sd_get_network_status()==SNS_DISCNT)
				{
					em_settings_get_int_item("system.ui_iap_id", (_int32*)&iap_id);
					em_init_network_impl(iap_id,em_notify_init_network);
				}
			}
			
#if (defined(MOBILE_PHONE)&&(!defined(__SYMBIAN32__)))
	if(g_network_state==1)
	{
		sd_check_net_connection_result();
	}
#endif

#if defined(__SYMBIAN32__)
			em_scheduler();
#else
			dt_load_tasks();
			trm_load_default_tree(  );
#endif
		}	
	}
		
	EM_LOG_DEBUG("em_do_next end!");
	return SUCCESS;

}
_int32 em_do_net_connection(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid)
{
	_u32 iap_id = 0;
	EM_LOG_ERROR("em_do_net_connection");

		if(/*msg_info->_device_id*/msgid == g_do_next_msg_id)
		{
			g_do_next_msg_id=0;
			if(g_need_init_net)
			{
				g_need_init_net=FALSE;
			}
			if(sd_get_network_status()==SNS_DISCNT)
			{
				em_settings_get_int_item("system.ui_iap_id", (_int32*)&iap_id);
				em_init_network_impl(iap_id,em_notify_init_network);
			}
		}	
		
	EM_LOG_ERROR("em_do_net_connection end!");
	return SUCCESS;

}


_int32 em_clear(void* p_param)
{
	POST_PARA_0* _p_param = (POST_PARA_0*)p_param;
//	_int32 ret_val = SUCCESS;

	EM_LOG_DEBUG("em_clear...");

	if(g_do_next_msg_id!=0)
	{
		em_cancel_message_by_msgid(g_do_next_msg_id);
		g_do_next_msg_id=0;
	}

	if(g_scheduler_timer_id!=0)
	{
		em_cancel_timer(g_scheduler_timer_id);
		g_scheduler_timer_id = 0;
	}
	
	trm_clear();
	
	dt_clear();

	mini_clear();
	//es_close_file(TRUE);
	
	return em_signal_sevent_handle(&(_p_param->_handle));

}

/* ���ؿ�ȫ�ֻ���ģ��ĳ�ʼ�� */
_int32 em_basic_init( void  )
{
	_int32 ret_val = SUCCESS;

	EM_LOG_DEBUG("em_basic_init...");

       //bytebuffer_init();

	 /* Get tht settings items from the config file */
	ret_val = em_settings_initialize();
	CHECK_VALUE(ret_val);

	ret_val = em_init_sys_path_info();
	CHECK_VALUE(ret_val);
	
#ifdef _LOGGER
	ret_val = em_log_reload_initialize();
#endif

#if defined(ENABLE_ETM_MEDIA_CENTER) && !defined(DVERSION_FOR_VALGRIND)
	ret_val = em_upgrade_initialze();
#endif
/*  	
	ret_val = em_start_et(  );
	if(ret_val!=SUCCESS)
	{
		em_settings_uninitialize( );
		CHECK_VALUE(ret_val);
	}
*/
	EM_LOG_DEBUG("em_basic_init SUCCESS ");
	return SUCCESS;
	
}
_int32 em_basic_uninit(void)
{
	EM_LOG_DEBUG("em_basic_uninit...");

	em_stop_et(  );
	
	em_settings_uninitialize( );
	
	EM_LOG_DEBUG("em_basic_uninit SUCCESS ");

	return SUCCESS;
}

/* ���ؿ⹦��ģ��ĳ�ʼ�� */
_int32 em_sub_module_init( void  )
{
	_int32 ret_val = SUCCESS;

	EM_LOG_DEBUG("em_sub_module_init...");

	ret_val =  init_download_manager_module();
	CHECK_VALUE(ret_val);

	ret_val =  init_vod_module();
	if(ret_val!=SUCCESS)
	{
		uninit_download_manager_module();
		CHECK_VALUE(ret_val);
	}

	ret_val =  init_tree_manager_module();
	if(ret_val!=SUCCESS)
	{
		uninit_vod_module();
		uninit_download_manager_module();
		CHECK_VALUE(ret_val);
	}

	ret_val =  init_mini_task_module();
	if(ret_val!=SUCCESS)
	{
		uninit_tree_manager_module();
		uninit_vod_module();
		uninit_download_manager_module();
		CHECK_VALUE(ret_val);
	}

	EM_LOG_DEBUG("em_sub_module_init SUCCESS ");
	return SUCCESS;
}
_int32 em_sub_module_uninit(void)
{
	EM_LOG_DEBUG("em_sub_module_uninit...");

	uninit_mini_task_module();

	uninit_tree_manager_module();

	uninit_vod_module();
	
	uninit_download_manager_module();
	
	EM_LOG_DEBUG("em_sub_module_uninit SUCCESS ");

	return SUCCESS;
}

/* ���ؿ����ģ��ĳ�ʼ�� */
_int32 em_other_module_init( void  )
{
	_int32 ret_val = SUCCESS;

	EM_LOG_DEBUG("em_other_module_init...");

#ifdef ENABLE_BT
	ret_val = init_tp_module();
	CHECK_VALUE(ret_val);
#endif	
#ifdef ENABLE_MEMBER
	ret_val = init_member_modular();
	if(ret_val != SUCCESS)
	{
		EM_LOG_ERROR("em_other_module_init init member failed.");
	}
#endif /* ENABLE_MEMBER */

#ifdef ENABLE_LIXIAN_ETM
	init_lixian_module();
#endif /* ENABLE_LIXIAN_ETM */

	EM_LOG_DEBUG("em_other_module_initinit SUCCESS ");
	return SUCCESS;

}
_int32 em_other_module_uninit(void)
{
#if 0 //defined( __SYMBIAN32__)
	_int32 ret_val = SUCCESS;
	_u32 iap_id=MAX_U32;
#endif

	EM_LOG_DEBUG("em_other_module_uninit...");

#ifdef ENABLE_LIXIAN_ETM
	uninit_lixian_module();
#endif /* ENABLE_LIXIAN_ETM */

#ifdef ENABLE_BT
	uninit_tp_module();
#endif
#ifdef ENABLE_MEMBER
	uninit_member_modular();
#endif /* ENABLE_MEMBER */
	
#ifdef ENABLE_RC
	uninit_rc_module();
#endif /* ENABLE_RC */
	EM_LOG_DEBUG("em_other_module_uninit SUCCESS ");

	return SUCCESS;
}

/* task_manager ģ��ĳ�ʼ�� */
_int32 em_init_task_manager( void * _param )
{
	_int32 ret_val = SUCCESS;

	EM_LOG_DEBUG("em_init_task_manager... ");

	sd_assert(g_scheduler_timer_id==0);
#if defined(ENABLE_NULL_PATH)
       //
#else
	sd_assert(g_et_running==FALSE);
#endif

#if defined(ENABLE_NULL_PATH)
	g_is_license_verified=FALSE;
	g_license_result = -1;
	g_license_expire_time = 0;
#else
	sd_assert(g_is_license_verified==FALSE);
	sd_assert(g_license_result==-1);
	sd_assert(g_license_expire_time==0);
#endif
#ifdef ENABLE_ETM_AUTO_START
	g_task_auto_start = TRUE;
#else
	sd_assert(g_task_auto_start==FALSE);
#endif
	
	em_settings_get_bool_item("system.task_auto_start", &g_task_auto_start);
	
#if defined(MOBILE_PHONE)
	g_is_license_verified=TRUE;
	g_license_result = 0;
	g_license_expire_time = MAX_U32;
#endif

	/* Start the scheduler_timer */  // 
	ret_val = em_start_timer(em_scheduler_timeout, NOTICE_INFINITE,EM_SCHEDULER_TIME_OUT, 0,NULL,&g_scheduler_timer_id);  
	if(ret_val!=SUCCESS)
	{
		/* FATAL ERROR! */
		EM_LOG_ERROR("FATAL ERROR! should stop the program!ret_val= %d",ret_val);
		#ifdef _DEBUG
		CHECK_VALUE(ret_val);
		#endif
	}

	EM_LOG_DEBUG("em_init_task_manager SUCCESS ");
	return ret_val;
	
}

_int32 em_uninit_task_manager(void)
{
	EM_LOG_DEBUG("em_uninit_task_manager...");

	
	if(g_scheduler_timer_id!=0)
	{
		em_cancel_timer(g_scheduler_timer_id);
		g_scheduler_timer_id = 0;
	}
	
	//g_et_running = FALSE;
	g_is_license_verified = FALSE;
	g_license_result=-1;
	g_license_expire_time=0;
	g_task_auto_start = FALSE;
	
	EM_LOG_DEBUG("em_uninit_task_manager SUCCESS ");

	return SUCCESS;
}

/////1.3 ������ؽӿ�

_int32 em_set_network_cnt_notify_callback(void* p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	em_notify_net_callback_ptr = (EM_NOTIFY_NET_CONNECT_RESULT)_p_param->_para1;
	
	return SUCCESS;

}

_int32 em_init_default_network(void)
{
#if defined(MOBILE_PHONE)
	_int32 ret_val = SUCCESS;
	_u32 iap_id = 0;
	EM_LOG_ERROR("em_init_network_impl:%d",g_network_state);
	
	if(g_network_state==1) return SUCCESS;
	
	ret_val=em_settings_get_int_item("system.iap_id", (_int32*)&iap_id);
	CHECK_VALUE(ret_val);

	ret_val=em_init_network_impl( iap_id,em_notify_init_network);
	CHECK_VALUE(ret_val);

	g_network_state=1;
	g_need_start_et=TRUE;
	EM_LOG_ERROR("em_init_default_network end!");
	return SUCCESS;		
#else
	return NOT_IMPLEMENT;
#endif
}
_int32 em_init_network_impl(_u32 iap_id,EM_NOTIFY_NET_CONNECT_RESULT call_back_func)
{
#if defined(MOBILE_PHONE)
	_int32 ret_val = SUCCESS;
	EM_LOG_ERROR("em_init_network_impl:%u",iap_id);
	
	mini_clear();
	
	if(g_et_running)
	{
		em_stop_et_sub_step( );
		g_need_start_et=TRUE;
	}

	if(sd_get_network_status()!=SNS_DISCNT)
	{
		sd_uninit_network();
	}
		

	g_need_notify_net=FALSE;	
	ret_val = sd_init_network( iap_id, call_back_func);
	CHECK_VALUE(ret_val);
		
	g_network_state = 1;
	EM_LOG_ERROR("em_init_network_impl end!");
	return SUCCESS;		
#else
	return NOT_IMPLEMENT;
#endif
}
_int32 em_init_network(void* p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
#if defined(MOBILE_PHONE)
	_u32 iap_id = (_u32)_p_param->_para1;

	EM_LOG_ERROR("em_init_network:%u",iap_id);
	if(sd_get_network_status()==SNS_DISCNT)
	{
		_u32 timeout = 0;
		em_settings_set_int_item("system.iap_id", (_int32)iap_id);
		em_settings_set_int_item("system.ui_iap_id", (_int32)iap_id);
		if(g_uinit_net_time_stamp!=0)
		{
			em_time(&timeout);
			if(TIME_SUBZ(timeout,g_uinit_net_time_stamp)<1000)
			{
				timeout = 1000;
			}
			else
			{
				timeout = 0;
			}
		}
		EM_LOG_ERROR("em_init_network:timeout =%u",timeout);
		_p_param->_result=em_post_next(em_do_net_connection,timeout);
		if(_p_param->_result == MSG_ALREADY_EXIST)
		{
			g_need_init_net = TRUE;
			_p_param->_result = SUCCESS;
		}
	}
	else
	{
		_p_param->_result=-1;
	}
#else
	_p_param->_result= NOT_IMPLEMENT;
#endif
	
	EM_LOG_ERROR("em_init_network end!");
	return em_signal_sevent_handle(&(_p_param->_handle));

}

_int32 em_uninit_network_impl(BOOL user_abort)
{
	//_int32 ret_val = SUCCESS;
	EM_LOG_ERROR("em_uninit_network_impl:%d",user_abort);
	
	mini_clear();
	
	if(g_et_running)
	{
		em_stop_et_sub_step( );
		if(!user_abort)
		{
			g_need_start_et=TRUE;
		}
		else
		{
			dt_stop_all_waiting_tasks();
		}
	}
	
#if defined(MOBILE_PHONE)
	if(sd_get_network_status()!=SNS_DISCNT)
	{
		em_time(&g_uinit_net_time_stamp);
		EM_LOG_ERROR("em_uninit_network_impl:g_uinit_net_time_stamp =%u",g_uinit_net_time_stamp);
	}
	
	sd_uninit_network();
#else
	return NOT_IMPLEMENT;
#endif
	g_need_notify_net=FALSE;
	g_network_state = 0;
	EM_LOG_ERROR("em_uninit_network_impl end!");
	return SUCCESS;
}

_int32 em_uninit_network(void* p_param)
{
	POST_PARA_0* _p_param = (POST_PARA_0*)p_param;
//	_int32 ret_val = SUCCESS;

	EM_LOG_DEBUG("em_uninit_network...");

	_p_param->_result=em_uninit_network_impl(FALSE);
	
	return em_signal_sevent_handle(&(_p_param->_handle));

}
_int32 	em_get_network_status(void * p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
#if defined(MOBILE_PHONE)
	SD_NET_STATUS *status= (SD_NET_STATUS * )_p_param->_para1;

	EM_LOG_DEBUG("em_get_network_status");
	
	*status = sd_get_network_status();
	
#else
	_p_param->_result = NOT_IMPLEMENT;
#endif
	EM_LOG_DEBUG("em_get_network_status, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 	em_get_network_iap(void * p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	_u32 *iap_id= (_u32 * )_p_param->_para1;

	EM_LOG_DEBUG("em_get_network_iap");
	
	*iap_id = MAX_U32;
	if(g_network_state==2)
	{
		_p_param->_result=em_settings_get_int_item("system.iap_id", (_int32 *)iap_id);
	}
	else
	{
		_p_param->_result=em_settings_get_int_item("system.ui_iap_id", (_int32 *)iap_id);
	}
	
	if(*iap_id==0) *iap_id=MAX_U32;
	
	EM_LOG_DEBUG("em_get_network_iap, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 	em_get_network_iap_from_ui(void * p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	_u32 *iap_id= (_u32 * )_p_param->_para1;

	EM_LOG_DEBUG("em_get_network_iap_from_ui");
	
	*iap_id = MAX_U32;
	
	_p_param->_result=em_settings_get_int_item("system.ui_iap_id", (_int32 *)iap_id);

	if(*iap_id==0) *iap_id=MAX_U32;
	
	EM_LOG_DEBUG("em_get_network_iap_from_ui, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 	em_get_network_iap_name(void * p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
#if defined(MOBILE_PHONE)
	char *iap_name= (char * )_p_param->_para1;
	const char * curnt_name = sd_get_network_iap_name();

	EM_LOG_DEBUG("em_get_network_iap_name");

	if(curnt_name)
	{
		sd_memcpy(iap_name, curnt_name,MAX_IAP_NAME_LEN);
	}
	
#else
	_p_param->_result = NOT_IMPLEMENT;
#endif
	EM_LOG_DEBUG("em_get_network_iap_name, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 em_get_network_flow(u32 * download,u32 * upload)
{
#if defined(MOBILE_PHONE)
	return et_get_network_flow(download,upload);
#else
	return NOT_IMPLEMENT;
#endif
}

_int32 	em_set_peerid(void * p_param)
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	char *peerid= (char * )_p_param->_para1;
	_u32 peerid_len = (_u32)_p_param->_para2;

	EM_LOG_DEBUG("em_set_peerid");

	// 
	set_peerid(peerid, peerid_len);

	return SUCCESS;
}

_int32 	em_get_peerid(void * p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
//#if defined(MOBILE_PHONE)
	char *peerid= (char * )_p_param->_para1;

	EM_LOG_DEBUG("em_get_peerid");

	_p_param->_result = em_settings_get_str_item("system.peer_id", peerid);
	if(em_strlen(peerid)==0||em_strcmp(peerid, "020000000000003V")==0)
	{
		_p_param->_result = get_peerid(peerid,PEER_ID_SIZE);
		if(_p_param->_result == SUCCESS && sd_strlen(peerid)!=0)
		{
			em_settings_set_str_item("system.peer_id", peerid);
			_p_param->_result = SUCCESS;
		}
		else
		{
			_p_param->_result = ERROR_INVALID_PEER_ID;
		}
	}
//#else
//	_p_param->_result = NOT_IMPLEMENT;
//#endif
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 em_set_net_type(void* p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	_u32 net_type = (_u32)_p_param->_para1;

	EM_LOG_DEBUG("em_set_net_type:0x%X",net_type);
	if(sd_get_net_type()!=net_type)
	{
		sd_set_net_type(net_type);
		if(em_is_et_running())
		{
			/* �л�������,����Ѹ��p2pЭ���Ƿ�����Ϊδ֪״̬:  0-δ֪,1-Ѹ��p2pЭ�鲻��Ҫhttp��װ,2-Ѹ��p2pЭ����Ҫhttp��װ  */
			settings_set_int_item( "p2p.http_encap_state",0);
			settings_set_int_item( "p2p.http_encap_test_count",0);
		}
	}

	sd_set_local_ip(0);

	return SUCCESS;
}
_int32 em_notify_init_network(_u32 iap_id,_int32 result,_u32 net_type)
{
	//_u32 iap_id_setting = MAX_U32;
	EM_LOG_ERROR("em_notify_init_network:iap_id=%u,result=%d,net_type=0x%X",iap_id,result,net_type);

	
	g_need_notify_net=TRUE;
	if(result==0)
	{
		g_network_state = 2;
		
		//em_settings_get_int_item("system.iap_id", (_int32*)&iap_id_setting);

		//if((iap_id_setting!=MAX_U32)&&(iap_id_setting!=iap_id))
		//{
			/* Save to cfg file */	            
			em_settings_set_int_item("system.iap_id", (_int32)iap_id);
		//}
		//em_settings_get_int_item("system.max_running_tasks", (_int32 *)&max_tasks);
		//em_set_max_tasks_impl( max_tasks);
	}
	else
	if(result==USER_ABORT_NETWORK)
	{
		g_network_state = 3;
	}
	else
	{
		g_network_state = 0;
	}
	return SUCCESS;
}

// 0 - not init, 1- initiating ,2 - ok ,3 - user abort , other -failed
BOOL em_is_net_ok(BOOL start_connect)
{
#if defined(MOBILE_PHONE)
	_int32 ret_val = SUCCESS;
	_u32 iap_id = MAX_U32;
	
	EM_LOG_DEBUG("em_is_net_ok:start_connect=%d,g_network_state=%d",start_connect,g_network_state);

	if(g_network_state==1)
	{
		sd_check_net_connection_result();
	}
	
	if(g_network_state==2)
	{
		return TRUE;
	}

	if(g_network_state != 1&&start_connect)
	{
		em_settings_get_int_item("system.ui_iap_id", (_int32*)&iap_id);
		ret_val = sd_init_network(iap_id,em_notify_init_network);
		if(ret_val==SUCCESS)
		{
			g_network_state = 1;
		}
	}
	return FALSE;
#else
	return TRUE;
#endif
	
}
BOOL em_is_et_idle(void)
{
	_u32 cur_time=0;
	
	if(!dt_have_running_task()/*&&(0==dt_get_running_vod_task_num( ))*/)
	{
		if(g_et_idle_time_stamp==0)
		{
			em_time_ms(&g_et_idle_time_stamp);
		}
		else
		{
			em_time_ms(&cur_time);
			if(TIME_SUBZ(cur_time, g_et_idle_time_stamp)>MAX_ET_IDLE_INTERVAL)
			{
	     			return TRUE;
			}
		}
	}
	else
	{
		if(g_et_idle_time_stamp!=0)
			g_et_idle_time_stamp=0;
	}
	
	return FALSE;
}
_int32 em_check_watch_dog()
{
	static _u32 old_time = 0;
	_u32 now_time = 0;
	_int32 ret_val = SUCCESS;
	ret_val = em_time(&now_time);
	CHECK_VALUE(ret_val);
	if(old_time!=0)
	{
		EM_LOG_ERROR("em_check_watch_dog start:D-value=%u",now_time-old_time);
		if(now_time-old_time>5)
		{
			EM_LOG_ERROR("em_check_watch_dog start 2:old_time=%u,now_time=%u,D-value=%u",old_time,now_time,now_time-old_time);
			/*
			em_uninit_network_impl(FALSE);
			if(g_network_state!=1)
			{
				ret_val=em_init_default_network();
				CHECK_VALUE(ret_val);
			}
			*/
		}

	}
	old_time = now_time;
	return SUCCESS;
}

_int32 em_scheduler(void)
{
	_u32 iap_id = 0;
	_int32 et_error = SUCCESS;
	EM_LOG_DEBUG("em_scheduler start:g_et_running=%d,g_need_notify_net=%d",g_et_running,g_need_notify_net);

	//em_check_watch_dog();
	
	#if (defined(MOBILE_PHONE)&&(!defined(__SYMBIAN32__)))
	if(g_network_state==1)
	{
		sd_check_net_connection_result();
	}
	#endif
	if(g_et_running)
	{
		et_error = et_check_critical_error();
		if(et_error!=SUCCESS)
		{
			if(et_error==USER_ABORT_NETWORK)
			{
				sd_assert(FALSE);
				em_uninit_network_impl(TRUE);
			}
			else
			if(et_error==NETWORK_NOT_READY)
			{
				em_uninit_network_impl(FALSE);
				if(g_network_state!=1)
				{
					em_init_network_impl( -1,em_notify_init_network);
				}
				
			}
			else
			{
				 em_restart_et();
			}
		}
	}

	if(g_need_notify_net)
	{
		g_need_notify_net = FALSE;
		
		if(g_network_state==2&&g_need_start_et)
		{
			em_start_et_sub_step( );
		}
		
		if(em_notify_net_callback_ptr)
		{
			em_settings_get_int_item("system.ui_iap_id", (_int32*)&iap_id);
			if(g_network_state==2)
			{
#if defined(MOBILE_PHONE)
				em_notify_net_callback_ptr(iap_id,0,sd_get_net_type());
#else
				em_notify_net_callback_ptr(iap_id,0,-1);
#endif
			}
			else
			{
				em_notify_net_callback_ptr(iap_id,-1,0);
			}
		}
	}

	dt_scheduler();
	trm_scheduler();

	em_http_report_from_file();

#ifdef ENABLE_ETM_MEDIA_CENTER
	if (em_is_license_ok())
	{
		em_start_remote_control();
	}
#endif
	//mini_scheduler();
	//es_close_file(FALSE);
	/*
	if((g_et_running)&&em_is_et_idle())
	{
		em_stop_et();
	}
	*/
	EM_LOG_DEBUG("em_scheduler end!");
	return SUCCESS;
}

char * em_get_system_path(void)
{
	return g_system_path;
}

BOOL em_is_et_running(void)
{
	_int32 ret_val = SUCCESS;
	if(g_et_running)
	{
	    if(!et_check_running())
	    {
	    	/* critical error !!! */
      		EM_LOG_URGENT("em_is_et_running:critical error !!! "); 
#if defined(MACOS)&&defined(MOBILE_PHONE)
      		printf("\n\n ***** em_is_et_running:critical error !!! **** \n\n"); 
#endif
		sd_assert(FALSE);
		ret_val = em_restart_et();
		if(ret_val != SUCCESS)
		{
			g_et_running = FALSE;
		}
	    }
	}
	return g_et_running;
}
BOOL em_is_license_ok(void)
{
#if defined( ENABLE_LICENSE)
      EM_LOG_DEBUG("em_is_license_ok:g_is_license_verified=%d,g_license_result=%d",g_is_license_verified,g_license_result); 
	if(g_is_license_verified==TRUE)
	{
		if(g_license_result == 0)
		{
			return TRUE;
		}
	}
	return FALSE;
#else
	return TRUE;
#endif /* ENABLE_LICENSE */
}

#ifdef ENABLE_ETM_MEDIA_CENTER

BOOL em_is_multi_disk(void)
{
	return g_is_multi_disk;
}

#endif

BOOL em_is_task_auto_start(void)
{
	return g_task_auto_start;
}

void em_user_set_netok(BOOL is_net_ok)
{
	g_user_set_netok = is_net_ok;
}

BOOL em_is_user_netok(void)
{
	return g_user_set_netok;
}

_int32 em_set_customed_interfaces(_int32 fun_idx, void *fun_ptr)
{
	_int32 ret_val = SUCCESS;
	
	ret_val = eti_set_customed_interface(fun_idx, fun_ptr);
	if(ret_val==SUCCESS)
	{
		ret_val = em_set_customed_interface(fun_idx, fun_ptr);
	}
	return ret_val;
}

/*  This is the call back function of license report result
	Please make sure keep it in this type:
	typedef int32 ( *ET_NOTIFY_LICENSE_RESULT)(u32 result, u32 expire_time);
*/
_int32 em_notify_license_result(_u32  result, _u32  expire_time)
{
	EM_LOG_DEBUG("em_notify_license_result:result=%u,expire_time=%u",result,expire_time);
	/* Add your code here ! */
	/* �ر�ע��:������ص���������,����Ӧ�þ�����࣬�����в����������������������κ����ؿ�Ľӿ�!��Ϊ�����ᵼ�����ؿ�����!����!!! */
	if((result==0)||(result>=21000))
	{
		g_is_license_verified = TRUE;
	}
	else
	{
		g_is_license_verified = FALSE;
	}

	g_license_result = result;
	g_license_expire_time = expire_time;

	return SUCCESS;
}

/* set_license */
_int32 	em_set_license(void * p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	char *license= (char * )_p_param->_para1;

	EM_LOG_DEBUG("em_set_license");
	
	
	_p_param->_result=em_save_license(license);
	if(_p_param->_result==SUCCESS)
	{
		if(g_et_running == FALSE)
		{
			_p_param->_result=em_start_et(  );	
		}
		else
		{
			_p_param->_result=eti_set_license(license, LICENSE_LEN);	
		}
	}
	
	EM_LOG_DEBUG("em_set_license, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 	em_get_license(void * p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	char *license= (char * )_p_param->_para1;

	EM_LOG_DEBUG("em_get_license");
	
	
	_p_param->_result=em_load_license( license);
	
	EM_LOG_DEBUG("em_get_license, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 	em_get_license_result(void * p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	_u32 *result= (_u32 * )_p_param->_para1;

	EM_LOG_DEBUG("em_get_license_result");
	
#if defined( ENABLE_LICENSE)
	*result = g_license_result;
	if(g_is_license_verified == FALSE)
	{
		_p_param->_result=LICENSE_VERIFYING;
	}
#else
	*result = 0;
#endif /* ENABLE_LICENSE */
	EM_LOG_DEBUG("em_get_license_result, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 	em_set_default_encoding_mode(void * p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	EM_ENCODING_MODE encoding_mode= (EM_ENCODING_MODE )_p_param->_para1;

	EM_LOG_DEBUG("em_set_default_encoding_mode");
#ifdef ENABLE_BT
	
	_p_param->_result=em_settings_set_int_item("system.encoding_mode", (_int32)encoding_mode);
	if((_p_param->_result==SUCCESS)&&(g_et_running == TRUE))
	{
		_p_param->_result=eti_set_seed_switch_type(encoding_mode);	
	}
	
	tp_set_default_switch_mode(( _u32 )encoding_mode );
	
	EM_LOG_DEBUG("em_set_default_encoding_mode, em_signal_sevent_handle:_result=%d",_p_param->_result);
#endif
	return SUCCESS;
}
_int32 	em_get_default_encoding_mode(void * p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	EM_ENCODING_MODE * encoding_mode= (EM_ENCODING_MODE * )_p_param->_para1;

	EM_LOG_DEBUG("em_get_default_encoding_mode");
	
	_p_param->_result=em_settings_get_int_item("system.encoding_mode", (_int32*)encoding_mode);
	
	EM_LOG_DEBUG("em_get_default_encoding_mode, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 	em_set_download_path(void * p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	char *download_path= (char * )_p_param->_para1;

	EM_LOG_DEBUG("em_set_download_path");
#ifdef ENABLE_ETM_MEDIA_CENTER
 	if(g_is_enable_dynmic_volume)
 	{
		_p_param->_result = -1;
 	}
	else
#endif /* ENABLE_ETM_MEDIA_CENTER */
	{
		_p_param->_result=em_settings_set_str_item("system.download_path", download_path);
	}
	
	
	EM_LOG_DEBUG("em_set_download_path, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 	em_get_download_path(void * p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	char *download_path= (char * )_p_param->_para1;

	EM_LOG_DEBUG("em_get_download_path");
	
	_p_param->_result=em_get_download_path_imp(  download_path);
	
	EM_LOG_DEBUG("em_get_download_path, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32  em_get_download_path_imp(char *download_path)
{

#ifdef ENABLE_ETM_MEDIA_CENTER
	if(!g_is_enable_dynmic_volume)
#endif /* ENABLE_ETM_MEDIA_CENTER  */
	{
		em_settings_get_str_item("system.download_path", download_path);
	}
#ifdef ENABLE_ETM_MEDIA_CENTER
	else
	{
		em_strncpy(download_path, g_default_download_volume_info->_volume_path, MAX_FILE_PATH_LEN );
		em_strcat( download_path, "/", 1 );
		em_strcat( download_path, EM_DOWNLOAD_PATH, em_strlen(EM_DOWNLOAD_PATH) );
	}
#endif /* ENABLE_ETM_MEDIA_CENTER  */

	EM_LOG_DEBUG("em_get_download_path_imp:%s", download_path);

	return SUCCESS;
}

#ifdef ENABLE_ETM_MEDIA_CENTER
_int32  em_get_download_volume_path(char *download_path)
{

 	if(!g_is_enable_dynmic_volume) return -1;
	
	em_strncpy(download_path, g_default_download_volume_info->_volume_name, MAX_FILE_PATH_LEN );
	em_strcat( download_path, ":/", 2 );
	em_strcat( download_path, EM_DOWNLOAD_PATH, em_strlen(EM_DOWNLOAD_PATH) );
	
	EM_LOG_DEBUG("em_get_download_volume_path:%s", download_path);
	return SUCCESS;
}

_int32  em_set_download_path_volume(char *download_volume)
{

	LIST_ITERATOR cur_node = LIST_BEGIN(g_sys_volume_info_list);
	EM_SYS_VOLUME_INFO *p_volume_info = NULL;
	char *p_tmp = NULL;
	_int32 ret_val = SUCCESS;
	char remount_cmd[MAX_FULL_PATH_BUFFER_LEN * 2] = {0};
	EM_LOG_DEBUG("em_set_download_path_volume:%s", download_volume);

	while( cur_node != LIST_END(g_sys_volume_info_list) )
	{
		p_volume_info = (EM_SYS_VOLUME_INFO *)LIST_VALUE(cur_node);
		p_tmp = em_strstr( download_volume, p_volume_info->_volume_name, 0 );
		if( p_tmp == download_volume )
		{
			g_default_download_volume_info = p_volume_info;
			em_settings_set_int_item("system.download_volume_index", p_volume_info->_volume_index );

			// ��mount������
			em_snprintf(remount_cmd, sizeof(remount_cmd), "/bin/mount -o remount,rw %s", p_volume_info->_volume_path); 
			ret_val = system(remount_cmd);
			EM_LOG_DEBUG("system(%s) return %d", remount_cmd, ret_val);

			return SUCCESS;
		}
		cur_node = LIST_NEXT( cur_node );
	}
	return INVALID_SYSTEM_PATH;
}

_int32  em_get_all_disk_volume( LIST *p_volume_list )
{

	LIST_ITERATOR cur_node = LIST_BEGIN(g_sys_volume_info_list);
	EM_SYS_VOLUME_INFO *p_volume_info = NULL;

	while( cur_node != LIST_END(g_sys_volume_info_list) )
	{
		p_volume_info = (EM_SYS_VOLUME_INFO *)LIST_VALUE(cur_node);
		EM_LOG_DEBUG("em_get_all_disk_volume:%s", p_volume_info->_volume_name);
		em_list_push(p_volume_list, &p_volume_info->_volume_name );
		cur_node = LIST_NEXT( cur_node );
	}
	return SUCCESS;
}

_int32  em_get_default_path_space( _u32 *p_size )
{

	return em_get_free_disk(g_default_download_volume_info->_volume_path, p_size );
}

_int32  em_get_disk_download_path( char *p_path )
{
	char *p_colon = NULL;
	LIST_ITERATOR cur_node = LIST_BEGIN(g_sys_volume_info_list);
	EM_SYS_VOLUME_INFO *p_volume_info = NULL;
	u32 movsize=0;
	u32 path_len=0;

	EM_LOG_DEBUG("em_get_disk_download_path:%s", p_path);
	
 	if(!g_is_enable_dynmic_volume) return -1;

	p_colon = em_strstr(p_path, ":/", 0);
	if (p_colon == NULL)
	{
		EM_LOG_ERROR("em_get_disk_download_path: %s do not have :/", p_path);
		return -1;
	}
	
	while( cur_node != LIST_END(g_sys_volume_info_list) )
	{
		p_volume_info = (EM_SYS_VOLUME_INFO *)LIST_VALUE(cur_node);
		if (em_strncmp(p_volume_info->_volume_name, p_path, p_colon-p_path) == 0)
		{
			path_len = em_strlen(g_default_download_volume_info->_volume_path);
			movsize = em_strlen(p_colon + 1);
			if (movsize + path_len > MAX_FILE_PATH_LEN)
			{
				movsize = MAX_FILE_PATH_LEN - path_len;
			}
			
			em_memmove(p_path+path_len,	p_colon + 1, movsize);
			em_memcpy(p_path, g_default_download_volume_info->_volume_path, path_len); 

			EM_LOG_DEBUG("em_get_disk_download_path: new path %s", p_path);
			return 0;
		}
		cur_node = LIST_NEXT( cur_node );
	}

	EM_LOG_ERROR("em_get_disk_download_path: %s have not found volume", p_path);
	return -1;	
}
//���д洢�ļ�·����Ҫ�ĳ������ϵͳ·��δ���
#endif

_int32 em_set_task_state_changed_callback(void* p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;

	EM_LOG_DEBUG("em_set_task_state_changed_callback");
	
 	em_task_state_changed_callback_ptr = (EM_NOTIFY_TASK_STATE_CHANGED)_p_param->_para1;

	EM_LOG_DEBUG("em_set_task_state_changed_callback, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

#ifdef ENABLE_ETM_MEDIA_CENTER
void em_set_task_state_changed_callback_ptr(EM_NOTIFY_TASK_STATE_CHANGED callback_function_ptr)
{
	em_task_state_changed_callback_ptr = callback_function_ptr;
}
#endif

//typedef int32 ( *ETM_NOTIFY_TASK_STATE_CHANGED)(u32 task_id, ETM_TASK_STATE new_state);

_int32 em_notify_task_state_changed(_u32 task_id, EM_TASK_INFO * p_task_info)
{
	_int32 ret_val = SUCCESS;
	EM_LOG_DEBUG("em_notify_task_state_changed:task_id=%u,new_state=%d",task_id,p_task_info ? p_task_info->_state : 0);
	if(em_task_state_changed_callback_ptr!=NULL)
		ret_val = em_task_state_changed_callback_ptr(task_id,p_task_info);
	EM_LOG_DEBUG("em_notify_task_state_changed end:task_id=%u,ret_val=%d",task_id,ret_val);
#ifdef LITTLE_FILE_TEST2
	if(p_task_info->_state ==TS_TASK_SUCCESS)
	{
      		EM_LOG_URGENT( "******* etm notify to jni, task download success!");
	}
#endif /* LITTLE_FILE_TEST2 */
	return ret_val;
}

/////2.8 ����2.9 ~2.14 ��Ĭ��ϵͳ����: max_tasks=5,download_limit_speed=-1,upload_limit_speed=-1,auto_limit_speed=FALSE,max_task_connection=128,task_auto_start=FALSE
_int32 em_load_default_settings(void* p_param)
{
	POST_PARA_0* _p_param = (POST_PARA_0*)p_param;
	_u32 max_tasks = MAX_RUNNING_TASK_NUM;
	_u32 download_limit_speed = -1;
	_u32 upload_limit_speed = -1;
	_u32 max_task_connection = CONNECT_MANAGE_DEFAULT_MAX_CONNECTIONS;
	BOOL auto_limit_speed = FALSE;
	BOOL task_auto_start = FALSE;
	_int32 piece_size = 300;

	EM_LOG_DEBUG("em_load_default_settings");
	
	_p_param->_result=em_settings_set_int_item("system.max_running_tasks", (_int32)max_tasks);
	if(_p_param->_result==SUCCESS)
	{
		_p_param->_result=dt_set_max_running_tasks(max_tasks);	
	}
	
	if((_p_param->_result==SUCCESS)&&(g_et_running == TRUE))
	{
		_p_param->_result=eti_set_max_tasks(max_tasks);	
	}
	
	em_settings_set_int_item("system.download_limit_speed", -1);
	em_settings_set_int_item("system.upload_limit_speed", -1);
	if((_p_param->_result==SUCCESS)&&(g_et_running == TRUE))
	{
		_p_param->_result=eti_set_limit_speed(download_limit_speed, upload_limit_speed);	
	}
	
	em_settings_set_int_item("system.max_task_connection", (_int32)max_task_connection);
	if((_p_param->_result==SUCCESS)&&(g_et_running == TRUE))
	{
		_p_param->_result=eti_set_max_task_connection(max_task_connection);	
	}
	
	if(_p_param->_result==SUCCESS)
	{
		em_settings_set_bool_item("system.auto_limit_speed", auto_limit_speed);
		//g_task_auto_start = task_auto_start;
		em_settings_set_bool_item("system.task_auto_start", task_auto_start);
		em_settings_set_int_item("system.prompt_tone_mode", 1);
	}
	
	_p_param->_result=em_settings_set_int_item("system.download_piece_size", piece_size);
	if((_p_param->_result==SUCCESS)&&(g_et_running == TRUE))
	{
#if defined(MOBILE_PHONE)
		_p_param->_result=settings_set_int_item("system.max_cmwap_range", piece_size/16);
#endif
	}
	EM_LOG_DEBUG("em_load_default_settings, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 em_set_max_tasks_impl(_u32 max_tasks)
{
	_int32 ret_val = SUCCESS;
	EM_LOG_DEBUG("em_set_max_tasks_impl");

#if 0 //defined(__SYMBIAN32__)||defined(WIN32)
	if(em_is_net_ok(FALSE)&&(sd_get_net_type()<NT_3G))
	{
		max_tasks = 1;
	}
#endif

	ret_val = dt_set_max_running_tasks(max_tasks);	
	CHECK_VALUE(ret_val);
	
	if(g_et_running == TRUE)
	{
		ret_val = eti_set_max_tasks(max_tasks);	
		CHECK_VALUE(ret_val);
	}
	
	return ret_val;
}

_int32 em_set_max_tasks(void* p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	_u32 max_tasks= (_u32 )_p_param->_para1;

	EM_LOG_DEBUG("em_set_max_tasks");
	
	_p_param->_result=em_settings_set_int_item("system.max_running_tasks", (_int32)max_tasks);
	if(_p_param->_result==SUCCESS)
	{
		_p_param->_result=em_set_max_tasks_impl(max_tasks);	
	}
	
	EM_LOG_DEBUG("em_set_max_tasks, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 em_get_max_tasks(void* p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	_u32 * max_tasks= (_u32 * )_p_param->_para1;

	EM_LOG_DEBUG("em_get_max_tasks");

	*max_tasks = MAX_RUNNING_TASK_NUM;
	//*max_tasks = eti_get_max_tasks();
	
	_p_param->_result=em_settings_get_int_item("system.max_running_tasks", (_int32 *)max_tasks);
	
	if(*max_tasks==0) *max_tasks = MAX_RUNNING_TASK_NUM;
	
	EM_LOG_DEBUG("em_get_max_tasks, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

//����ֵ: 0  success
//value�ĵ�λ:kbytes/second��Ϊ0 ��ʾ������
_int32 em_set_download_limit_speed(void* p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	_u32 download_limit_speed= (_u32 )_p_param->_para1;
	_u32 upload_limit_speed = -1;

	EM_LOG_DEBUG("em_set_download_limit_speed");
	
	_p_param->_result=em_settings_set_int_item("system.download_limit_speed", (_int32)download_limit_speed);
	if((_p_param->_result==SUCCESS)&&(g_et_running == TRUE))
	{
		em_settings_get_int_item("system.upload_limit_speed",(_int32*) &upload_limit_speed);
		eti_set_limit_speed(download_limit_speed, upload_limit_speed);	
	}
	
	return SUCCESS;
}

//����ֵ: 0  success
//value�ĵ�λ:kbytes/second��Ϊ0 ��ʾ������
_int32 em_get_download_limit_speed(void* p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	_u32 * download_limit_speed= (_u32 * )_p_param->_para1;
	//_u32 upload_limit_speed = 0;

	EM_LOG_DEBUG("em_get_download_limit_speed");

	*download_limit_speed = -1;
	//_p_param->_result=eti_get_limit_speed(download_limit_speed, &upload_limit_speed);
	
	_p_param->_result=em_settings_get_int_item("system.download_limit_speed", (_int32 *)download_limit_speed);
	
	if(*download_limit_speed==0) *download_limit_speed = -1;
	
	EM_LOG_DEBUG("em_get_download_limit_speed, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 em_set_upload_limit_speed(void* p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	_u32 upload_limit_speed= (_u32 )_p_param->_para1;
	_u32 download_limit_speed = -1;

	EM_LOG_DEBUG("em_set_upload_limit_speed");
	
	_p_param->_result=em_settings_set_int_item("system.upload_limit_speed", (_int32)upload_limit_speed);
	if((_p_param->_result==SUCCESS)&&(g_et_running == TRUE))
	{
		em_settings_get_int_item("system.download_limit_speed",(_int32*) &download_limit_speed);
		eti_set_limit_speed(download_limit_speed, upload_limit_speed);	
	}
	
		return SUCCESS;
}

//����ֵ: 0  success
//value�ĵ�λ:kbytes/second��Ϊ0 ��ʾ������
_int32 em_get_upload_limit_speed(void* p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	_u32 * upload_limit_speed= (_u32 * )_p_param->_para1;
	//_u32 download_limit_speed = 0;

	EM_LOG_DEBUG("em_get_upload_limit_speed");

	*upload_limit_speed = -1;
	//_p_param->_result=eti_get_limit_speed(&download_limit_speed, upload_limit_speed);
	
	_p_param->_result=em_settings_get_int_item("system.upload_limit_speed", (_int32 *)upload_limit_speed);
	
	if(*upload_limit_speed==0) *upload_limit_speed = -1;
	
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}


_int32 em_set_auto_limit_speed(void* p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	BOOL auto_limit_speed= (BOOL )_p_param->_para1;

	EM_LOG_DEBUG("em_set_auto_limit_speed");
	
	_p_param->_result=em_settings_set_bool_item("system.auto_limit_speed", auto_limit_speed);
	
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 em_get_auto_limit_speed(void* p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	BOOL * auto_limit_speed= (BOOL * )_p_param->_para1;

	EM_LOG_DEBUG("em_get_auto_limit_speed");

	*auto_limit_speed = FALSE;
	_p_param->_result=em_settings_get_bool_item("system.auto_limit_speed", auto_limit_speed);
	
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 em_set_max_task_connection(void* p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	_u32 max_task_connection= (_u32 )_p_param->_para1;

	EM_LOG_DEBUG("em_set_max_task_connection");
	
	_p_param->_result=em_settings_set_int_item("system.max_task_connection", (_int32)max_task_connection);
	if((_p_param->_result==SUCCESS)&&(g_et_running == TRUE))
	{
		_p_param->_result=eti_set_max_task_connection(max_task_connection);	
	}
	
		EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
		return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 em_get_max_task_connection(void* p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	_u32 * max_task_connection= (_u32 * )_p_param->_para1;
	_u32 default_task_connection = CONNECT_MANAGE_DEFAULT_MAX_CONNECTIONS;

	EM_LOG_DEBUG("em_get_max_task_connection");
	
	*max_task_connection = default_task_connection;
	//*max_task_connection=eti_get_max_task_connection();
	
	_p_param->_result=em_settings_get_int_item("system.max_task_connection", (_int32 *)max_task_connection);
	
	if(*max_task_connection==0) *max_task_connection = default_task_connection;
	
	EM_LOG_DEBUG("em_get_max_task_connection, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 em_set_task_auto_start(void* p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	BOOL task_auto_start= (BOOL )_p_param->_para1;

	EM_LOG_DEBUG("em_set_task_auto_start");
	
	//g_task_auto_start = task_auto_start;
	_p_param->_result=em_settings_set_bool_item("system.task_auto_start", task_auto_start);
	
	EM_LOG_DEBUG("em_set_task_auto_start, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 em_get_task_auto_start(void* p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	BOOL * task_auto_start= (BOOL * )_p_param->_para1;

	EM_LOG_DEBUG("em_get_task_auto_start");

	*task_auto_start = g_task_auto_start;
	_p_param->_result=em_settings_get_bool_item("system.task_auto_start", task_auto_start);
	
	
	EM_LOG_DEBUG("em_get_task_auto_start, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}




_int32 em_set_download_piece_size(void* p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	_u32 piece_size= (_u32 )_p_param->_para1;
#if defined(MOBILE_PHONE)
	_u32 wap_range = piece_size/16;
#endif

	EM_LOG_DEBUG("em_set_download_piece_size");
	
	_p_param->_result=em_settings_set_int_item("system.download_piece_size", (_int32)piece_size);
	if((_p_param->_result==SUCCESS)&&(g_et_running == TRUE))
	{
#if defined(MOBILE_PHONE)
		/*
		100 : 98304 = 6 * (16*1024)
		200 : 196608 = 12 * (16*1024)
		300 : 294912 = 18 * (16*1024)
		400 : 409600 = 25 * (16*1024)
		500 : 507904 = 31 * (16*1024)
		600 : 606208 = 37 * (16*1024)
		700 : 704512 = 43 * (16*1024)
		800 : 819200 = 50 * (16*1024)
		900 : 917504 = 56 * (16*1024)
		1000 : 1015808 = 62 * (16*1024)
		*/
		_p_param->_result=settings_set_int_item("system.max_cmwap_range", (_int32)wap_range);
#endif
	}
	
	EM_LOG_DEBUG("em_set_download_piece_size, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 em_get_download_piece_size(void* p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	_u32 * piece_size= (_u32 * )_p_param->_para1;

	EM_LOG_DEBUG("em_get_download_piece_size");
	
	*piece_size = 300;
	//*piece_size=eti_get_max_task_connection();
	
	_p_param->_result=em_settings_get_int_item("system.download_piece_size", (_int32 *)piece_size);
	
	if(*piece_size==0) *piece_size = 300;
	
	EM_LOG_DEBUG("em_get_download_piece_size, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 em_set_ui_version(void* p_param)
{
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	char * ui_version= (char * )_p_param->_para1;
	_int32 product_id = (_int32)_p_param->_para2;
	_int32 partner_id = (_int32)_p_param->_para3;
	_int32 ret_1 = SUCCESS;
	_int32 ret_2 = SUCCESS;
	char old_ui_version[256] = {0};
	_int32 old_product = 0;
	_int32 bussiness_type = 0;
	
	EM_LOG_DEBUG("em_set_ui_version:%s,%d",ui_version,product_id);

	switch(product_id)
	{
		case PRODUCT_ID_ANDROID_LIXIAN: /* ���߰�׿ */
			bussiness_type = BUSSINESS_TYPE_ANDROID_LIXIAN ;
			break;
		case PRODUCT_ID_ANDROID_YUNBO : /* �Ʋ���׿ */
			bussiness_type = BUSSINESS_TYPE_ANDROID_YUNBO ;
			break;
		case PRODUCT_ID_YUN_HD : /* Ѹ����HD */
			bussiness_type = BUSSINESS_TYPE_YUN_HD ;
			break;
		case PRODUCT_ID_YUN_IPHONE : /* Ѹ����for iPhone */
			bussiness_type = BUSSINESS_TYPE_YUN_IPHONE ;
			break;
		case PRODUCT_ID_YUNBO_IPHONE : /* �Ʋ�for iPhone */
			bussiness_type = BUSSINESS_TYPE_YUNBO_IPHONE ;
			break;
		case PRODUCT_ID_YUNBO_HD : /* �Ʋ�HD */
			bussiness_type = BUSSINESS_TYPE_YUNBO_HD ;
			break;
		case PRODUCT_ID_PAY_CENTRE : /* ��Ա�ֻ�֧������ for Android*/
			bussiness_type = BUSSINESS_TYPE_PAY_CENTRE ;
			break;
		case PRODUCT_ID_YUNBO_TV : /* ��׿�Ʋ�TV��*/
			bussiness_type = BUSSINESS_TYPE_YUNBO_TV ;
			break;
		default:
			sd_assert(FALSE);
			{
				_u32 * crash = NULL;
				/* �������úϷ�product_id,������������! */
				*crash = 1;
			}
	}
	
#ifdef ENABLE_MEMBER
	/* ���¼ģ������ bussiness_type */
	member_set_bussiness_type( bussiness_type );
#endif/*ENABLE_MEMBER*/
    
#ifdef ENABLE_LIXIAN_ETM

    set_detect_file_suffixs_by_product_id(product_id);
    
#endif/*ENABLE_LIXIAN_ETM*/

	//��ȡ�ɰ汾��Ϣ
	ret_1 = em_settings_get_str_item("system.ui_version", old_ui_version);
	ret_2 = em_settings_get_int_item("system.ui_product", &old_product);
	if ((ret_1 | ret_2) == SUCCESS) 
	{
		if(sd_strcmp(ui_version, old_ui_version))
		{
			//���°汾��Ϣ��һ��
			EM_LOG_DEBUG("em_set_ui_version: set g_is_install = TRUE");
			g_is_install= TRUE;
		}
	}
	g_is_ui_reported = TRUE;
	
	ret_1 = em_settings_set_str_item("system.ui_version", (char*)ui_version);
	ret_2 = em_settings_set_int_item("system.ui_product", product_id);
	em_settings_set_int_item("system.ui_partner_id", partner_id);

	_p_param->_result = ret_1 | ret_2;
	
	if((_p_param->_result == SUCCESS) && (g_et_running == TRUE))
	{
#if defined(MOBILE_PHONE)
		ret_1 = settings_set_str_item((char*)"system.ui_version",(char*)ui_version);
		ret_2 = settings_set_int_item((char*)"system.ui_product", product_id);
		settings_set_int_item((char*)"system.ui_partner_id", partner_id);
#endif
		_p_param->_result = ret_1 | ret_2;
		et_reporter_set_version(ui_version, product_id,partner_id);
		et_reporter_new_install(g_is_install);
		g_is_ui_reported = FALSE;
	}
	
	EM_LOG_DEBUG("em_set_ui_version, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 em_get_business_type(_int32 product_id)
{
	_int32 business_type = 0;
	switch(product_id)
	{
		case PRODUCT_ID_ANDROID_LIXIAN: /* ���߰�׿ */
			business_type = BUSSINESS_TYPE_ANDROID_LIXIAN ;
			break;
		case PRODUCT_ID_ANDROID_YUNBO : /* �Ʋ���׿ */
			business_type = BUSSINESS_TYPE_ANDROID_YUNBO ;
			break;
		case PRODUCT_ID_YUN_HD : /* Ѹ����HD */
			business_type = BUSSINESS_TYPE_YUN_HD ;
			break;
		case PRODUCT_ID_YUN_IPHONE : /* Ѹ����for iPhone */
			business_type = BUSSINESS_TYPE_YUN_IPHONE ;
			break;
		case PRODUCT_ID_YUNBO_IPHONE : /* �Ʋ�for iPhone */
			business_type = BUSSINESS_TYPE_YUNBO_IPHONE ;
			break;
		case PRODUCT_ID_YUNBO_HD : /* �Ʋ�HD */
			business_type = BUSSINESS_TYPE_YUNBO_HD ;
			break;
		case PRODUCT_ID_PAY_CENTRE : /* ��Ա�ֻ�֧������ for Android*/
			business_type = BUSSINESS_TYPE_PAY_CENTRE ;
			break;
		case PRODUCT_ID_YUNBO_TV : /* ��׿�Ʋ�TV��*/
			business_type = BUSSINESS_TYPE_YUNBO_TV ;
			break;
		default:
			business_type = 0 ;
	}
	return business_type;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/* �ϱ���� */
_int32 em_http_etm_inner_report(const char* first_u, const char* str_begin_from_u1)
{
	_int32 ret_val = SUCCESS;
	char url_buffer[MAX_URL_LEN] = {0};

	if(sd_strlen(str_begin_from_u1) >= MAX_URL_LEN)
	{
		return INVALID_ARGUMENT;
	}

	sd_snprintf(url_buffer, MAX_URL_LEN-1, "u=%s&%s", first_u,str_begin_from_u1);

	if(sd_strlen(url_buffer) >= MAX_URL_LEN)
	{
		return INVALID_ARGUMENT;
	}
	
	ret_val = em_http_report_impl(url_buffer,sd_strlen(url_buffer));
	return ret_val;
}

// �ṩ�����ϱ�
static char g_username[EM_MAX_USERNAME_LEN];
static char g_password[EM_MAX_USERNAME_LEN];
static _u64 g_userid  = 0;
_int32 em_set_username_password(char *name, char *password)
{
	if(NULL != name && (0 != sd_strlen(name)) )
		sd_strncpy(g_username, name, EM_MAX_USERNAME_LEN - 1);
	else
		sd_strncpy(g_username, "test", EM_MAX_USERNAME_LEN - 1);

	if(NULL != password && (0 != sd_strlen(password)) )
		sd_strncpy(g_password, password, EM_MAX_USERNAME_LEN - 1);
	else
		sd_strncpy(g_password, "test", EM_MAX_USERNAME_LEN - 1);

	return SUCCESS;
}

_int32 em_set_uid(_u64 uid)
{
	g_userid = uid;
	return SUCCESS;
}

#if 0 //def ENABLE_TEST_NETWORK
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<unistd.h>
#include 	<fcntl.h>
#include    <time.h>
#include	<netdb.h>
#include	<errno.h>
#include	<netinet/in.h>

#define EM_HOST_NAME_LEN  64
#define EM_IP_LEN  32
#define EM_PORT  80
#define EM_CONNECT_TIME_OUT 5

typedef enum tag_EM_PROTOCOL_TYPE
{
	EM_MAIN_LOGIN = 1,
	EM_VIP_SERVER,
	EM_MAIN_PORTAL,

	EM_IPAD_INTERFACE,
	EM_LIXIAN_INTERFACE,
	EM_VOD_INTERFACE,
}EM_PROTOCOL_TYPE;


int em_parse_DNS(char *name, int *family, char **ip)
{
	_int32 ret = SUCCESS;
	struct hostent *hptr = NULL;
	char tempname[EM_HOST_NAME_LEN];
	char str[EM_IP_LEN];
	
	sd_memset(tempname, 0, sizeof(tempname));
	if(NULL == name)
		return -1;
	sd_strncpy(tempname, name, EM_HOST_NAME_LEN);

	hptr = gethostbyname(tempname);
	if( hptr == NULL )
	{
		return PARSE_DNS_GET_ERROR;
	}
	else
	{
		*family = hptr->h_addrtype;
		switch(hptr->h_addrtype)
		{
		case AF_INET:
			inet_ntop(hptr->h_addrtype, *(hptr->h_addr_list), str, sizeof(str));
			sd_strncpy(*ip, str, EM_IP_LEN);
			break;
		default:
			ret = PARSE_DNS_TYPE_ERROR;
			break;
		}
	}
	return ret;
}

int em_select(int fd, fd_set *rset, fd_set *wset, fd_set *eset, struct timeval *timeout)
{
	int ret;
	ret = select(fd, rset, wset, eset, timeout);
	if( 0 == ret )
	{
		//printf("select timeout\n");
	}
	else if( ret < 0)
	{
		//perror("select");
		ret = errno;
	}
	return ret;	
}

int em_connect_with_timeout(int family, char *seraddr, int port)
{
	_int32 ret = SUCCESS;
	int flags, n;
	socklen_t len;
	fd_set rset, wset;
	struct timeval tval;
	int sockfd;
	struct sockaddr_in stServerAddress;
	int nsec = EM_CONNECT_TIME_OUT;
	int error = 0;
	
	sockfd = socket(family, SOCK_STREAM, 0);
	if (-1 == sockfd)
	{
		//perror("socket error");
		ret = errno;
		return ret;
	}
	// ����sokcet��־
	flags = fcntl(sockfd, F_GETFL, 0);
	if(flags < 0)
	{
		//perror("fcntl F_GETFL");
		ret = errno;
		goto ErrHandle;
	}
	if( fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0 )
	{
		//perror("fcntl F_SETFL");
		ret = errno;
		goto ErrHandle;
	}
	
	stServerAddress.sin_family = AF_INET;
	stServerAddress.sin_port = sd_htons(port);
	//stServerAddress.sin_addr.s_addr = inet_addr(serAddr);
	if (inet_pton(AF_INET, seraddr, (struct in_addrv *) &stServerAddress.sin_addr.s_addr) <= 0)
    {
        //perror("dest addr error");
		ret = errno;
       	goto ErrHandle;
    }

	if( (n = connect(sockfd, (struct sockaddr*)&stServerAddress, sizeof(stServerAddress))) < 0 )
	{
		if(errno != EINPROGRESS )
			goto ErrHandle;
	}
	if( 0 != n )
	{
		FD_ZERO(&rset);
		FD_SET(sockfd, &rset);
		wset = rset;
		
		tval.tv_sec = nsec;
		tval.tv_usec = 0;
		if( 0 == (n = em_select(sockfd+1, &rset, &wset, NULL, nsec?&tval:NULL)) )
		{
			errno = ETIMEDOUT;
			ret = errno;
			goto ErrHandle;
		} 
		if( FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset) )
		{
			// ����׽ӿ��Ƿ���ڴ�����Ĵ���
			len = sizeof(error);
			if( getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0 )
			{	
				ret = errno;
				goto ErrHandle;
			}
		}
		else
		{	
			ret = CONNECT_ERROR;
			//printf("select error: sockfd not set\n");
			goto ErrHandle;
		}
	}	
	//�ظ��׽����ļ�״̬��־
	if( fcntl(sockfd, F_SETFL, flags ) < 0 )
	{
		//perror("fcntl");
		ret = errno;
		goto ErrHandle;
	}
	if( error )
	{
		errno = error;
		ret = errno;
		goto ErrHandle;
	}

ErrHandle:
	//printf("goto ErrHandle\n");
	close(sockfd);
	return ret;

}

void em_main_login_server_report_handler(void* arg)
{
	_int32 errcode = 0;
	errcode = (_int32)arg;
	
	EM_LOG_DEBUG("em_main_login_server_report_handler begin: errcode = %d\n", errcode);

	em_http_net_status_report(EM_MAIN_LOGIN, DEFAULT_MAIN_MEMBER_SERVER_HOST_NAME, 80, errcode);

}

void em_vip_login_server_report_handler(void* arg)
{
	_int32 errcode = 0;
	errcode = (_int32)arg;
	
	EM_LOG_DEBUG("em_vip_login_server_report_handler begin: errcode = %d\n", errcode);

	em_http_net_status_report(EM_VIP_SERVER, DEFAULT_MAIN_VIP_SERVER_HOST_NAME, 80, errcode);

}

void em_portal_server_report_handler(void* arg)
{
	_int32 errcode = 0;
	errcode = (_int32)arg;
	
	EM_LOG_DEBUG("em_portal_server_report_handler begin: errcode = %d\n", errcode);

	em_http_net_status_report(EM_MAIN_PORTAL, DEFAULT_PORTAL_MEMBER_SERVER_HOST_NAME, 80, errcode);
}

void em_ipad_interface_server_report_handler(void* arg)
{
	_int32 errcode = 0;
	errcode = (_int32)arg;
	EM_LOG_DEBUG("em_ipad_interface_server_report_handler begin: errcode = %d\n", errcode);

	em_http_net_status_report(EM_IPAD_INTERFACE, DEFAULT_LIXIAN_HOST_NAME, DEFAULT_LIXIAN_PORT, errcode);

	//em_http_net_status_report(EM_VOD_INTERFACE, "i.vod.xunlei.com", 80);

	//em_http_net_status_report(EM_LIXIAN_INTERFACE, DEFAULT_LIXIAN_SERVER_HOST_NAME, DEFAULT_LIXIAN_SERVER_PORT);

}

char * em_get_net_status_report(EM_PROTOCOL_TYPE type, _int32 result, char *ip, _int32 errcode)
{
	static char str_buffer[1024] = {0};
	char os[MAX_OS_LEN]={0},os_encoded[MAX_OS_LEN]={0},device_type[64]={0};
	char ui_version[64]={0},peer_id[PEER_ID_SIZE+4]={0};
	_u32 cur_time = 0;
	_u32 net_type = sd_get_net_type();
	const char * p_imei = get_imei();
	_int32 product_id = 0;

	/*
	Ϊ�˶������ݽ����е�u1��u9����Ϊ������Ϣ��
	u1,�汾��
	u2,�ֻ�peerid
	u3,�ֻ�imei����
	u4,�û�id��û������0��¼�ɹ���Ϊ���ص��ڲ�id
	u5,unixʱ��
	u6,androidϵͳ��
	u7,��������
	u8,�ֻ��ͺ�
	u9,��Ʒid.
	*/
	sd_time(&cur_time);
	
	sd_get_os(os, MAX_OS_LEN);
	url_object_encode_ex(os, os_encoded ,63);
	
	settings_get_str_item("system.ui_version",ui_version);
	
	get_peerid(peer_id, PEER_ID_SIZE);

	settings_get_int_item("system.ui_product", &product_id);
	sd_assert(product_id!=0);

	sd_get_hardware_version(device_type, 64); 

	sd_snprintf(str_buffer, 1023, "u1=%s&u2=%s&u3=%s&u4=%llu&u5=%u&u6=%s&u7=0x%X&u8=%s&u9=%d&u10=%d&u11=%u&u12=%s&u13=%s&u14=%s&u15=%d",
	ui_version, peer_id, p_imei, g_userid, cur_time, os_encoded, net_type, device_type, product_id, result, type, ip, g_username, g_password, errcode);

	return str_buffer;
}

/* �ϱ���� */
_int32 em_http_net_status_report(EM_PROTOCOL_TYPE type, char *domain, _u32 port, _int32 errcode)
{
	_int32 ret_val = SUCCESS;
	char url_buffer[MAX_URL_LEN] = {0};
	char *ip_addr = NULL;
	int  family;
	char *report = NULL;

	EM_LOG_DEBUG("em_http_net_status_report: type = %d, domain = %s, port = %d\n", type, domain, port);
	if(NULL == domain)
	{
		ret_val = INVALID_ARGUMENT;
		goto ErrHandle;
	}
	ip_addr = malloc(EM_IP_LEN);
	if(NULL != ip_addr)
	{	
		sd_memset(ip_addr, 0x00, EM_IP_LEN);
		if( SUCCESS == (ret_val = em_parse_DNS(domain, &family, &ip_addr)) )
		{
			ret_val = em_connect_with_timeout(family, ip_addr, port);
		}
	}
	else
	{	
		ret_val = INVALID_MEMORY;
	}

	report = em_get_net_status_report(type, ret_val, ip_addr, errcode);
	
	sd_snprintf(url_buffer, MAX_URL_LEN-1, "u=%s&%s", EM_TEST_LOGIN_REPORT_PROTOCOL, report);

	if(sd_strlen(url_buffer) >= MAX_URL_LEN)
	{
		EM_LOG_DEBUG("***em_http_net_status_report, ERROR: url_buffer too long");
		return INVALID_ARGUMENT;
	}
	EM_LOG_DEBUG("em_http_net_status_report, url_buffer = %s", url_buffer);
	
	ret_val = em_http_report_impl(url_buffer,sd_strlen(url_buffer));

ErrHandle:

	SAFE_DELETE(ip_addr);
	return ret_val;
}
#endif


#if 0 //def ENABLE_PROGRESS_BACK_REPORT

_int32 em_download_progress_back_report(char* data,_u32 data_len)
{
	_int32 ret_val = SUCCESS;
	EM_HTTP_GET http_para = {0};
	EM_HTTP_REPORT_INFO * p_info = NULL;
	_u32 seed = 0;
	_u32 random_num = 0;

	if(data_len >= MAX_URL_LEN)
	{
		ret_val = INVALID_ARGUMENT;
		sd_assert(FALSE);
		goto ErrHandler;
	}

	//����С��1000�������
	sd_time(&seed);
	sd_srand(seed);
	random_num = sd_rand()%1000;

	//ƴ���ϱ�url
	ret_val = sd_malloc(sizeof(EM_HTTP_REPORT_INFO), (void*)&p_info);
	if(ret_val!=SUCCESS) goto ErrHandler;
	sd_memset(p_info, 0, sizeof(EM_HTTP_REPORT_INFO));

	if(sd_strstr(data,EM_PROGRESS_BACK_REPORT_PROTOCOL,0) != NULL)
	{
		/* ���ؿ�����Ʋ����ϱ���Ҫ�����µĻ�Աͳ�Ʒ����� */
		sd_snprintf(p_info->_url, MAX_URL_LEN-1, "http://%s:%u/?%s&rd=%u", NEW_HTTP_REPORT_HOST_NAME, NEW_HTTP_REPORT_PORT, data, random_num);
	}
	p_info->_retry_num = MAX_REPORT_AGAIN_NUM;
	http_para._url = p_info->_url;
	http_para._url_len = sd_strlen(http_para._url);
	http_para._user_data = (void*)p_info;
	http_para._callback_fun = em_http_report_resp;
	http_para._recv_buffer = p_info->_recv_buffer;
	http_para._recv_buffer_size = 16384;
	http_para._timeout = 10;
	
	p_info->_state = EHRS_RUNNING;
	/* http �������ȼ�:-1:�����ȼ�,������ͣ��������; 0:��ͨ���ȼ�,����ͣ��������*/
	/* �����ϱ������Ƚ�Ƶ�������ȼ����߻�����Ӱ�����������ٶȣ��ʵ������ȼ�--- by zyq @ 20120419 */
	ret_val = em_http_get_impl(&http_para, &p_info->_http_id,-1);

	/* ������η��سɹ������� */
	ret_val = SUCCESS;
ErrHandler:

	return ret_val;

}

static char progress_report_url[MAX_URL_LEN] = {0};
char * em_set_download_progress_back_info_to_report(char *url, _u64 file_size, _u64 back_len, _u32 task_status, _u32 failed_code)
{
	_int32 ret_val = SUCCESS;
	char str_buffer[MAX_URL_LEN] = {0};
	char os[MAX_OS_LEN]={0},os_encoded[MAX_OS_LEN]={0},device_type[64]={0};
	char ui_version[64]={0},peer_id[PEER_ID_SIZE+4]={0};
	_u32 cur_time = 0;
	_u32 net_type = sd_get_net_type();
	const char * p_imei = get_imei();
	_int32 product_id = 0;
	char url_buffer[2*MAX_URL_LEN] = {0};

	EM_LOG_DEBUG("em_set_download_progress_back_info, file_size = %llu, back_len = %llu, task_status = %u, failed_code = %u", file_size, back_len, task_status, failed_code);
	/*
	Ϊ�˶������ݽ����е�u1��u9����Ϊ������Ϣ��
	u1,�汾��
	u2,�ֻ�peerid
	u3,�ֻ�imei����
	u4,�û�id��û������0��¼�ɹ���Ϊ���ص��ڲ�id
	u5,unixʱ��
	u6,androidϵͳ��
	u7,��������
	u8,�ֻ��ͺ�
	u9,��Ʒid.
	*/
	sd_time(&cur_time);
	
	sd_get_os(os, MAX_OS_LEN);
	url_object_encode_ex(os, os_encoded ,63);
	
	settings_get_str_item("system.ui_version",ui_version);
	
	get_peerid(peer_id, PEER_ID_SIZE);

	settings_get_int_item("system.ui_product", &product_id);
	sd_assert(product_id!=0);

	sd_get_hardware_version(device_type, 64); 

	sd_snprintf(str_buffer, 1023, "u1=%s&u2=%s&u3=%s&u4=%llu&u5=%u&u6=%s&u7=0x%X&u8=%s&u9=%d&u10=%llu&u11=%llu&u12=%u&u13=%u",
	ui_version, peer_id, p_imei, g_userid, cur_time, os_encoded, net_type, device_type, product_id, back_len, file_size, task_status, failed_code);

	sd_snprintf(progress_report_url, 2*MAX_URL_LEN - 1, "u=%s&%s", EM_PROGRESS_BACK_REPORT_PROTOCOL, str_buffer);

	if(sd_strlen(progress_report_url) >= 2*MAX_URL_LEN)
	{
		EM_LOG_ERROR("***em_set_download_progress_back_info, ERROR: url_buffer too long");
		return INVALID_ARGUMENT;
	}
	EM_LOG_DEBUG("em_set_download_progress_back_info, url_buffer = %s", progress_report_url);

}

void em_download_progress_back_report_handler(void* arg)
{
	EM_LOG_DEBUG("em_main_login_server_report_handler begin: url = %s\n", progress_report_url);

	_int32 ret_val = SUCCESS;

	ret_val = em_download_progress_back_report(progress_report_url,sd_strlen(progress_report_url));

	return ret_val;

}


#endif

_int32 em_http_report_impl(char* data,_u32 data_len)
{
#if 1
				return 0;
#else
	_int32 ret_val = SUCCESS;
	EM_HTTP_GET http_para = {0};
	EM_HTTP_REPORT_INFO * p_info = NULL;
	_u32 seed = 0;
	_u32 random_num = 0;

	if(data_len >= MAX_URL_LEN)
	{
		ret_val = INVALID_ARGUMENT;
		sd_assert(FALSE);
		goto ErrHandler;
	}

	//����С��1000�������
	sd_time(&seed);
	sd_srand(seed);
	random_num = sd_rand()%1000;

	//ƴ���ϱ�url
	ret_val = sd_malloc(sizeof(EM_HTTP_REPORT_INFO), (void*)&p_info);
	if(ret_val!=SUCCESS) goto ErrHandler;
	sd_memset(p_info, 0, sizeof(EM_HTTP_REPORT_INFO));

	if(sd_strstr(data,ETM_YUNBO_REPORT_PROTOCOL,0)!=NULL || sd_strstr(data,EM_TEST_LOGIN_REPORT_PROTOCOL,0)!=NULL)
	{
		/* ���ؿ�����Ʋ����ϱ���Ҫ�����µĻ�Աͳ�Ʒ����� */
		sd_snprintf(p_info->_url, MAX_URL_LEN-1, "http://%s:%u/?%s&rd=%u", NEW_HTTP_REPORT_HOST_NAME, NEW_HTTP_REPORT_PORT, data, random_num);
		//sd_snprintf(p_info->_url, MAX_URL_LEN-1, "http://%s:%u/?%s&rd=%u", "pgv.m.xunlei.com", 80, data, random_num);
	}
	else
	{
		sd_snprintf(p_info->_url, MAX_URL_LEN-1, "http://%s:%u/?%s&rd=%u", DEFAULT_HTTP_REPORT_HOST_NAME, DEFAULT_HTTP_REPORT_PORT, data, random_num);
	}
	p_info->_retry_num = MAX_REPORT_AGAIN_NUM;
	http_para._url = p_info->_url;
	http_para._url_len = sd_strlen(http_para._url);
	http_para._user_data = (void*)p_info;
	http_para._callback_fun = em_http_report_resp;
	http_para._recv_buffer = p_info->_recv_buffer;
	http_para._recv_buffer_size = 16384;
	http_para._timeout = 10;
	
	p_info->_state = EHRS_RUNNING;
	/* http �������ȼ�:-1:�����ȼ�,������ͣ��������; 0:��ͨ���ȼ�,����ͣ��������*/
	/* �����ϱ������Ƚ�Ƶ�������ȼ����߻�����Ӱ�����������ٶȣ��ʵ������ȼ�--- by zyq @ 20120419 */
	ret_val = em_http_get_impl(&http_para, &p_info->_http_id,-1);

	if(ret_val != SUCCESS)
	{
		sd_assert(FALSE);
		ret_val = em_http_report_save_to_file(http_para._url, http_para._url_len);	
		sd_assert(ret_val==SUCCESS);
		SAFE_DELETE(p_info);
	}
	else
	{
		ret_val = em_http_report_add_action_to_list(p_info);
		sd_assert(ret_val==SUCCESS);
	}

	/* ������η��سɹ������� */
	ret_val = SUCCESS;
	
ErrHandler:

	return ret_val;
#endif
}


_int32 em_http_report(void* p_param)
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	char* data = _p_param->_para1;
	_u32 data_len = (_u32)_p_param->_para2;
	_int32 ret_val = 0;//em_http_report_impl(data, data_len);

	_p_param->_result = ret_val;
	EM_LOG_DEBUG("em_http_report, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));

}

_int32 em_http_report_by_url(void* p_param)
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	char* data = _p_param->_para1;
	_u32 data_len = (_u32)_p_param->_para2;

	_int32 ret_val = SUCCESS;
	#if 0
	EM_HTTP_GET http_para = {0};
	EM_HTTP_REPORT_INFO * p_info = NULL;
	_u32 seed = 0;
	_u32 random_num = 0;

	if(data_len >= MAX_URL_LEN)
	{
		ret_val = INVALID_ARGUMENT;
		sd_assert(FALSE);
		goto ErrHandler;
	}

	//����С��1000�������
	sd_time(&seed);
	sd_srand(seed);
	random_num = sd_rand()%1000;

	//ƴ���ϱ�url
	ret_val = sd_malloc(sizeof(EM_HTTP_REPORT_INFO), (void*)&p_info);
	if(ret_val!=SUCCESS) goto ErrHandler;
	sd_memset(p_info, 0, sizeof(EM_HTTP_REPORT_INFO));

	sd_memcpy(p_info->_url, data, data_len);
	
	p_info->_retry_num = MAX_REPORT_AGAIN_NUM;
	http_para._url = p_info->_url;
	http_para._url_len = sd_strlen(http_para._url);
	http_para._user_data = (void*)p_info;
	http_para._callback_fun = em_http_report_resp;
	http_para._recv_buffer = p_info->_recv_buffer;
	http_para._recv_buffer_size = 16384;
	http_para._timeout = 10;
	
	p_info->_state = EHRS_RUNNING;
	/* http �������ȼ�:-1:�����ȼ�,������ͣ��������; 0:��ͨ���ȼ�,����ͣ��������*/
	/* �����ϱ������Ƚ�Ƶ�������ȼ����߻�����Ӱ�����������ٶȣ��ʵ������ȼ�--- by zyq @ 20120419 */
	ret_val = em_http_get_impl(&http_para, &p_info->_http_id,-1);

	if(ret_val != SUCCESS)
	{
		sd_assert(FALSE);
		ret_val = em_http_report_save_to_file(http_para._url, http_para._url_len);	
		sd_assert(ret_val==SUCCESS);
		SAFE_DELETE(p_info);
	}
	else
	{
		ret_val = em_http_report_add_action_to_list(p_info);
		sd_assert(ret_val==SUCCESS);
	}

	/* ������η��سɹ������� */
	ret_val = SUCCESS;
	
ErrHandler:
	#endif
	_p_param->_result = ret_val;
	EM_LOG_DEBUG("em_http_report_by_url, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));

}


_int32 em_http_report_add_action_to_list(EM_HTTP_REPORT_INFO * p_action)
{
	_int32 ret_val = SUCCESS;
	ret_val = list_push(&g_http_report_list,( void * )p_action);
	CHECK_VALUE(ret_val);
	return SUCCESS;
}

_int32 em_http_report_handle_action_list(BOOL clear_all)
{
	_int32 ret_val = SUCCESS;
	pLIST_NODE cur_node = NULL ,nxt_node = NULL ;
	EM_HTTP_REPORT_INFO * p_action = NULL;
	EM_HTTP_GET http_para = {0};
	_u32 hash_value = 0;
	
	if(list_size(&g_http_report_list)==0)	
	{
		EM_LOG_DEBUG("em_http_report_handle_action_list: list_size(&g_http_report_list) = %d, ", list_size(&g_http_report_list));
		return SUCCESS;
	}

	cur_node = LIST_BEGIN(g_http_report_list);
	while(cur_node!=LIST_END(g_http_report_list))
	{
		nxt_node = LIST_NEXT(cur_node);
		
		p_action = (EM_HTTP_REPORT_INFO * )LIST_VALUE(cur_node);

		if( clear_all || p_action->_state == EHRS_SUCCESS )
		{
			eti_http_close(p_action->_http_id);
			//sd_http_close(p_action->_http_id);	
			list_erase(&g_http_report_list, cur_node);
			sd_assert(p_action->_node_id == 0);
			SAFE_DELETE(p_action);
		}
		else if(p_action->_state == EHRS_FAILED)
		{
			sd_get_url_hash_value(p_action->_url, sd_strlen(p_action->_url), &hash_value);
			EM_LOG_DEBUG("em_http_report_handle_action_list: EHRS_FAILED--retry_num = %d, errcode = %d, url_hash = %s, url = %s",p_action->_retry_num, p_action->_errcode, hash_value, p_action->_url);
			// ��ʱ����д���ļ��ȴ��´�ѭ���Ĵ���
			/*if(-1 == p_action->_errcode)
			{
				eti_http_close(p_action->_http_id);
				//sd_http_close(p_action->_http_id);	
				list_erase(&g_http_report_list, cur_node);
				if(p_action->_node_id==0)
				{
					ret_val = em_http_report_save_to_file(p_action->_url, sd_strlen(p_action->_url));	
					sd_assert(ret_val == SUCCESS);
				}
				sd_assert(ret_val==SUCCESS);
				SAFE_DELETE(p_action);
			}
			// ����������;�ֻ����һ��
			else
			{
			*/
			if(p_action->_retry_num > 0)
			{
				p_action->_retry_num = 0;
				http_para._url = p_action->_url;
				http_para._url_len = sd_strlen(http_para._url);
				http_para._user_data = (void*)p_action;
				http_para._callback_fun = em_http_report_resp;
				http_para._recv_buffer = p_action->_recv_buffer;
				http_para._recv_buffer_size = 16384;	
				http_para._timeout = 10;

				p_action->_state = EHRS_RUNNING;
				ret_val = em_http_get_impl(&http_para, &p_action->_http_id, -1);
				if(ret_val != SUCCESS)
				{
					eti_http_close(p_action->_http_id);
					//sd_http_close(p_action->_http_id);	
					list_erase(&g_http_report_list, cur_node);
					SAFE_DELETE(p_action);
				}
			}
			else
			{
				eti_http_close(p_action->_http_id);
				//sd_http_close(p_action->_http_id);	
				list_erase(&g_http_report_list, cur_node);
				SAFE_DELETE(p_action);
			}

		}

		cur_node = nxt_node;
	}

	return ret_val;
}
_int32 em_http_report_clear_action_list(void)
{
	return em_http_report_handle_action_list( TRUE);
}

_int32 em_http_report_resp(EM_HTTP_CALL_BACK* param)
{
	EM_HTTP_REPORT_INFO* p_info = (EM_HTTP_REPORT_INFO*)(param->_user_data);

	EM_LOG_DEBUG("em_http_report_resp, param->_type = %d, result = %d", param->_type, param->_result);

	if(param->_type == EMHC_NOTIFY_FINISHED)
	{
		p_info->_errcode = param->_result;
		
		 /* ���߷�����Ϊ�������ӣ�ÿ���ϱ����ӻ�ֱ�ӱ��������ص������Ի᷵��ECONNRESET  Connection reset by peer  ����Զ�˷������ر�
			�Ʋ��ϱ�����������Ӧ����204״̬��Ҳ�ǵ��������ϱ�����
		 */
		if (param->_result == 0 || param->_result == 204 || param->_result == 304 || param->_result == HTTP_ERR_INVALID_SERVER_FILE_SIZE 
#if defined(LINUX)
#ifdef MACOS
			 || param->_result == 54
#else
			 || param->_result == 104
#endif
#endif
		)
		{
			/* �ϱ��ɹ� */
		        EM_LOG_DEBUG("report success!, result=%d", param->_result);
			if(p_info!=NULL&&p_info->_http_id == param->_http_id)
			{
				p_info->_state = EHRS_SUCCESS;
			}
			else
			{
				sd_assert(FALSE);
			}
		}
		else
		{
			/* �ϱ�ʧ��*/
			if(p_info!=NULL&&p_info->_http_id == param->_http_id)
			{
				p_info->_state = EHRS_FAILED;
			}
			else
			{
				sd_assert(FALSE);
			}
		}
	}
	return SUCCESS;
}

_u32 em_http_report_get_tree_id(void)
{
	_int32 ret_val = SUCCESS;
	if(g_http_report_tree_id == 0)
	{
		char * system_path = em_get_system_path();
		char report_file_store_path[MAX_FULL_PATH_BUFFER_LEN] = {0};
		
		sd_snprintf(report_file_store_path, MAX_FULL_PATH_BUFFER_LEN-1, "%s/%s",system_path, "runmit_report.data");

		ret_val = trm_open_tree_impl( report_file_store_path, O_FS_CREATE|O_FS_RDWR ,&g_http_report_tree_id);
		sd_assert(ret_val==SUCCESS);
	}
	return g_http_report_tree_id;
}

_int32 em_http_report_save_to_file(char* data, _u32 data_len)
{
	_int32 ret_val = SUCCESS;
	_u32 node_id = 0,tree_id = em_http_report_get_tree_id();

	EM_LOG_DEBUG("em_http_report_save_to_file: data = %s", data);
	ret_val = trm_create_node_impl(tree_id, tree_id, NULL, 0, data, data_len,&node_id);
	sd_assert(ret_val==SUCCESS);
	
	return ret_val;
}

_int32 em_http_report_from_file(void)
{
#if 1
				return 0;
#else
	_int32 ret_val = SUCCESS;
	_u32 node_id = 0,buf_len = 0,count = 0,tree_id = em_http_report_get_tree_id();
	_u32 tmp_id = 0;
	EM_HTTP_GET http_para = {0};
	EM_HTTP_REPORT_INFO* p_info = NULL;

	em_http_report_handle_action_list(FALSE);

	if(em_is_net_ok(TRUE)&&(sd_get_net_type()!=UNKNOWN_NET))
	{
		EM_LOG_DEBUG("em_http_report_from_file:net_type=%u!!",sd_get_net_type());

		ret_val= trm_get_first_child_impl(tree_id, tree_id, &node_id);
		CHECK_VALUE(ret_val);

		while(node_id!=TRM_INVALID_NODE_ID)
		{
			ret_val = sd_malloc(sizeof(EM_HTTP_REPORT_INFO), (void*)&p_info);
			CHECK_VALUE(ret_val);
			sd_memset(p_info, 0, sizeof(EM_HTTP_REPORT_INFO));

			buf_len = MAX_URL_LEN;
			ret_val= trm_get_data_impl( tree_id, node_id, (void*)p_info->_url,&buf_len);
			sd_assert(ret_val==SUCCESS);
			
			if(ret_val==SUCCESS)
			{
				/*��ȡ���ϱ�url */
				EM_LOG_DEBUG("report url:%s", p_info->_url);

				http_para._url = p_info->_url;
				http_para._url_len = sd_strlen(http_para._url);
				http_para._user_data = (void*)p_info;
				http_para._callback_fun = em_http_report_resp;
				http_para._recv_buffer = p_info->_recv_buffer;
				http_para._recv_buffer_size = 16384;	
				http_para._timeout = 10;

				//p_info->_node_id = node_id;
				p_info->_state = EHRS_RUNNING;
				/* http �������ȼ�:-1:�����ȼ�,������ͣ��������; 0:��ͨ���ȼ�,����ͣ��������*/
				/* �����ϱ������Ƚ�Ƶ�������ȼ����߻�����Ӱ�����������ٶȣ��ʵ������ȼ�--- by zyq @ 20120419 */
				ret_val=em_http_get_impl(&http_para, &p_info->_http_id,-1);
				sd_assert(ret_val==SUCCESS);
				if(ret_val != SUCCESS)
				{
					SAFE_DELETE(p_info);

					goto ErrHandler;
					/*
					ret_val= trm_get_next_brother_impl( tree_id, node_id,&node_id);
					sd_assert(ret_val == SUCCESS);
					if(ret_val!=SUCCESS) goto ErrHandler; 
					*/
				}
				else
				{
					ret_val = em_http_report_add_action_to_list(p_info);
					sd_assert(ret_val==SUCCESS);

					/* �ϱ�ʱɾ���ýڵ� */
					tmp_id = node_id;
					ret_val= trm_get_next_brother_impl( tree_id, tmp_id, &node_id);
					sd_assert(ret_val == SUCCESS);
					ret_val = trm_delete_node_impl(tree_id, tmp_id);
					sd_assert(ret_val == SUCCESS);

					count++;
				}
			}
			else
			{
				_u32 node_id_tmp = node_id;
				/* �ڵ������⣬ֱ�Ӷ��� */
				SAFE_DELETE(p_info);
				ret_val= trm_get_next_brother_impl( tree_id, node_id_tmp,&node_id);
				sd_assert(ret_val == SUCCESS);
				ret_val = trm_delete_node_impl(tree_id,node_id_tmp);
				sd_assert(ret_val == SUCCESS);
				continue;
			}
			
			/* �����ۼƹ����ϱ�����ͬʱ���ͣ�һ����෢��15�� */
			if(count ++ > 14) break;
		}
	}
ErrHandler:
	
	return ret_val;
#endif
}
/////////////////////////////////////////////////////////////////////////////////////////////////////

_int32 em_reporter_mobile_user_action_to_file(void* p_param)
{
	POST_PARA_4* _p_param = (POST_PARA_4*)p_param;
	_u32 action_type = (_u32)_p_param->_para1;
	_u32 action_value = (_u32)_p_param->_para2;
	void* data = _p_param->_para3;
	_u32 data_len = (_u32)_p_param->_para4;

	EM_LOG_DEBUG("em_reporter_mobile_user_action_to_file:%u", action_type);

	_p_param->_result = 0;//eti_reporter_mobile_user_action_to_file(action_type, action_value, data, data_len);

	EM_LOG_DEBUG("em_reporter_mobile_user_action_to_file, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}
extern int32 et_reporter_enable_user_action_report(BOOL is_enable);
_int32 em_reporter_mobile_enable_user_action_report(void* p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	BOOL is_enable = (_u32 )_p_param->_para1;

	EM_LOG_DEBUG("em_reporter_mobile_enable_user_action_report");
	
	_p_param->_result= 0;//et_reporter_enable_user_action_report(is_enable);
	
	EM_LOG_DEBUG("em_reporter_mobile_enable_user_action_report, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 em_set_prompt_tone_mode(void* p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	_u32 mode= (_u32 )_p_param->_para1;

	EM_LOG_DEBUG("em_set_prompt_tone_mode");
	
	_p_param->_result=em_settings_set_int_item("system.prompt_tone_mode", (_int32)mode);
	
	EM_LOG_DEBUG("em_set_prompt_tone_mode, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 em_get_prompt_tone_mode(void* p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	_u32 * mode= (_u32 * )_p_param->_para1;

	EM_LOG_DEBUG("em_get_prompt_tone_mode");
	
	*mode = 1;
	
	_p_param->_result=em_settings_get_int_item("system.prompt_tone_mode", (_int32 *)mode);
	

	EM_LOG_DEBUG("em_get_prompt_tone_mode, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 em_set_p2p_mode(void* p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	_u32 mode= (_u32 )_p_param->_para1;

	EM_LOG_DEBUG("em_set_p2p_mode");
	
	_p_param->_result=em_settings_set_int_item("system.p2p_mode", (_int32)mode);
	if((_p_param->_result==SUCCESS)&&(g_et_running == TRUE))
	{
#if defined(MOBILE_PHONE)
		_p_param->_result=settings_set_int_item((char*)"system.p2p_mode", (_int32)mode);
#endif
	}
	
	EM_LOG_DEBUG("em_set_p2p_mode, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 em_get_p2p_mode(void* p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	_u32 * mode= (_u32 * )_p_param->_para1;

	EM_LOG_DEBUG("em_get_p2p_mode");
	
	*mode = 0;
	
	_p_param->_result=em_settings_get_int_item("system.p2p_mode", (_int32 *)mode);
	

	EM_LOG_DEBUG("em_get_p2p_mode, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 em_set_cdn_mode(void* p_param)
{
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	BOOL enable= (BOOL )_p_param->_para1;
	_u32 enable_cdn_speed = (_u32 )_p_param->_para2;
	_u32 disable_cdn_speed = (_u32 )_p_param->_para3;

	EM_LOG_DEBUG("em_set_p2p_mode");
	
	em_settings_set_bool_item("system.enable_cdn_mode", enable);
	em_settings_set_int_item("system.disable_cdn_speed", (_int32)disable_cdn_speed);
	em_settings_set_int_item("system.enable_cdn_speed", (_int32)enable_cdn_speed);
	if(g_et_running == TRUE)
	{
#if defined(MOBILE_PHONE)
		settings_set_bool_item((char*)"system.enable_cdn_mode", enable);
		settings_set_int_item((char*)"system.disable_cdn_speed", (_int32)disable_cdn_speed);
		settings_set_int_item((char*)"system.enable_cdn_speed", (_int32)enable_cdn_speed);
#endif
	}
	
	EM_LOG_DEBUG("em_set_cdn_mode, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 em_set_host_ip(void* p_param)
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	const char* host_name = _p_param->_para1;
	const char* ip_str = _p_param->_para2;

	EM_LOG_DEBUG("em_set_host_ip");
	
	_p_param->_result=eti_set_host_ip(host_name, ip_str);
	
	EM_LOG_DEBUG("em_set_host_ip, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 em_clear_host_ip(void* p_param)
{
	POST_PARA_0* _p_param = (POST_PARA_0*)p_param;

	EM_LOG_DEBUG("em_clear_host_ip");
	
	_p_param->_result=eti_clear_host_ip();
	
	EM_LOG_DEBUG("em_set_host_ip, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

/* Get the torrent seed file information  */
_int32 	em_get_torrent_seed_info( void * p_param)
{
#ifdef ENABLE_BT
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	char * seed_path = (char * )_p_param->_para1;
	EM_ENCODING_MODE encoding_mode = (EM_ENCODING_MODE)_p_param->_para2;
	TORRENT_SEED_INFO **pp_seed_info = (TORRENT_SEED_INFO ** )_p_param->_para3;

	EM_LOG_DEBUG("em_get_torrent_seed_info");

	_p_param->_result= tp_get_seed_info(seed_path,encoding_mode, (TORRENT_SEED_INFO**) pp_seed_info);

	EM_LOG_DEBUG("em_get_torrent_seed_info, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
#else
	return -1;
#endif
}


/* Release the memory which malloc by  tm_get_torrent_seed_info */
_int32 	em_release_torrent_seed_info( void * p_param)
{
#ifdef ENABLE_BT
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	TORRENT_SEED_INFO *p_seed_info= (TORRENT_SEED_INFO * )_p_param->_para1;

	EM_LOG_DEBUG("em_release_torrent_seed_info");
	_p_param->_result = tp_release_seed_info( p_seed_info);

	EM_LOG_DEBUG("em_release_torrent_seed_info, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
#else
	return -1;
#endif
}


_int32 	em_extract_ed2k_url( void * p_param)
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	char* ed2k= (char * )_p_param->_para1;
	ET_ED2K_LINK_INFO* info = (ET_ED2K_LINK_INFO * )_p_param->_para2;
	
	EM_LOG_DEBUG("em_extract_ed2k_url");
	
	if(em_is_net_ok(TRUE)==FALSE)
	{
		EM_LOG_DEBUG("em_extract_ed2k_url:Warnning:The network is NOT ok !");
		_p_param->_result = NETWORK_NOT_READY;
	}
	else
	//if(g_et_running == TRUE)
	//{
		_p_param->_result=eti_extract_ed2k_url(ed2k,info);	
	//}
	//else
	//	_p_param->_result=-1;
	
	EM_LOG_DEBUG("em_extract_ed2k_url, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

 _int32 em_scheduler_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid)
 {
	//EM_LOG_DEBUG("em_scheduler_timeout");

	if(errcode == MSG_TIMEOUT)
	{
		if(/*msg_info->_device_id*/msgid == g_scheduler_timer_id)
		{
 			em_scheduler();
		}	
	}
		
	//EM_LOG_DEBUG("em_scheduler_timeout end!");
	return SUCCESS;
 }

_int32 	em_start_et( void )
{
	_int32 ret_val = SUCCESS;
	_u32 len = 0;
#ifdef ENABLE_ETM_MEDIA_CENTER
	BOOL is_auto_green_channel = TRUE;
#endif
	
	sd_assert(g_et_running==FALSE);
	if(g_et_wait_mac) return 1925;
	
	EM_LOG_DEBUG("em_start_et");

	if(g_need_start_et) 
		g_need_start_et = FALSE;
	if(em_is_net_ok(TRUE)==FALSE)
	{
		EM_LOG_ERROR("em_start_et:Warnning:The network is NOT ok !");

		sd_assert(FALSE);
		g_need_start_et = TRUE;
		return SUCCESS;
	}
	
	ret_val = eti_init(NULL);
#if defined(ENABLE_NULL_PATH)
	if(ret_val == ALREADY_ET_INIT)
	{
		ret_val = SUCCESS;
	}
#else
	if(ret_val == ALREADY_ET_INIT)
	{
		eti_uninit( );
		ret_val = eti_init(NULL);
	}
#endif
	
	CHECK_VALUE(ret_val);

	len = em_strlen(g_system_path);
	if(len!=0)
	{
		 ret_val = eti_set_download_record_file_path(g_system_path,len);
	        if((ret_val != 0)&&(ret_val != 2058))
	        {
			eti_uninit( );
			CHECK_VALUE(ret_val);
	        }
	}
	
	ret_val = et_set_system_path(g_system_path);
	sd_assert(ret_val == SUCCESS);
	
#if defined( ENABLE_LICENSE)
	/* Set license callback function to  Embed Thunder download library */
	ret_val=eti_set_license_callback( em_notify_license_result);
	if(ret_val!=SUCCESS)
	{
		eti_uninit( );
		CHECK_VALUE(ret_val);
	}

	 ret_val = em_set_et_license();	
        if(ret_val != 0)
        {
        	if(ret_val==1925)
        	{
			/* Start the em_set_et_license_timeout */  // 
			ret_val = em_start_timer(em_set_et_license_timeout, NOTICE_INFINITE,EM_WAIT_MAC_TIME_OUT, 0,NULL,&g_et_wait_mac_timer_id);  
			if(ret_val!=SUCCESS)
			{
				/* FATAL ERROR! */
				EM_LOG_ERROR("FATAL ERROR! should stop the program!ret_val= %d",ret_val);
				//eti_uninit( );
				CHECK_VALUE(ret_val);
			}
			else
			{
        			g_et_wait_mac=TRUE;
			}
        	}
		else
		{
			eti_uninit( );
			CHECK_VALUE(ret_val);
		}
        }
	 else
#endif /* ENABLE_LICENSE */
	 {
		 ret_val=em_set_et_config(  );
		 CHECK_VALUE(ret_val);
		g_et_running = TRUE;
	 }
	 
#ifdef ENABLE_ETM_MEDIA_CENTER
	em_settings_get_bool_item("system.auto_green_channel", &is_auto_green_channel);
	ret_val = et_auto_hsc_sw(is_auto_green_channel);
	CHECK_VALUE(ret_val);
#endif
	 
	EM_LOG_DEBUG("em_start_et end:g_et_wait_mac_timer_id=%u,g_et_wait_mac=%d,g_et_running=%d",g_et_wait_mac_timer_id,g_et_wait_mac,g_et_running);
	return ret_val;	
	
}
_int32 	em_set_et_license( void )
{
	_int32 ret_val = SUCCESS;
	char buffer[64];
	_u32 len = 0;

	EM_LOG_DEBUG("em_set_et_license");
	em_memset(buffer, 0, 64);
	ret_val=em_load_license( buffer);
	if(ret_val!=SUCCESS)
	{
		CHECK_VALUE(ret_val);
	}
	
	len = em_strlen(buffer);
	if(len==0)
	{
		CHECK_VALUE(INVALID_LICENSE_LEN);
	}
	
	 ret_val = eti_set_license(buffer,len);
	
	EM_LOG_DEBUG("eti_set_license:ret_val=%d",ret_val);
	return ret_val;	
	
}
 _int32 em_set_et_license_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid)
 {
	_int32 ret_val = SUCCESS;
	EM_LOG_DEBUG("em_set_et_license_timeout");

	if(errcode == MSG_TIMEOUT)
	{
		if(/*msg_info->_device_id*/msgid == g_et_wait_mac_timer_id)
		{
			 ret_val = em_set_et_license();	
		        if(ret_val == 0)
		        {
				em_cancel_timer(g_et_wait_mac_timer_id);
				g_et_wait_mac_timer_id = 0;

		        	g_et_wait_mac=FALSE;
				em_set_et_config(  );
				g_et_running = TRUE;
				EM_LOG_DEBUG("em_set_et_license_timeout em_set_et_license success!");
		        }
		}	
	}
		
	EM_LOG_DEBUG("em_set_et_license_timeout end!");
	return SUCCESS;
 }
_int32 	em_set_et_config( void )
{
	_int32 ret_val = SUCCESS;
	EM_ENCODING_MODE switch_type=2;
	_u32 max_tasks = MAX_RUNNING_TASK_NUM;
	_u32 download_limit_speed = -1;
	_u32 upload_limit_speed = -1;
	_u32 max_task_connection = CONNECT_MANAGE_DEFAULT_MAX_CONNECTIONS;
	_u32 vod_buffer_size = 0;
	_u32 vod_buffer_time = 0;
#if defined(MOBILE_PHONE)
	_u32 piece_size = 0;
	char ui_version[64];
	_int32 ui_product = 0,partner_id = 0;
	_int32 p2p_mode = 0;
	BOOL enable_cdn_mode = DEFAULE_CDN_MODE;
	_int32 disable_cdn_speed = DEFAULE_DISABLE_CDN_SPEED;
	_int32 enable_cdn_speed = DEFAULE_ENABLE_CDN_SPEED;
#endif
	#if defined(ENABLE_HTTP_VOD)&&(!defined(ENABLE_ASSISTANT))
	_u16 port = DEFAULT_LOCAL_HTTP_SERVER_PORT;
	#endif /* ENABLE_HTTP_VOD */

	EM_LOG_DEBUG("em_set_et_config");
	
	#if defined(ENABLE_HTTP_VOD)&&(!defined(ENABLE_ASSISTANT))
	 ret_val = et_start_http_server(&port);
        if((ret_val != 0)&&(ret_val != 2058))
        {
              CHECK_VALUE(ret_val);
        }
	EM_LOG_DEBUG("em_set_et_config:et_start_http_server,port=%u",port);
	em_settings_set_int_item("system.http_server_port", (_int32)port);
	#endif /* ENABLE_HTTP_VOD */
	
	em_settings_get_int_item("system.encoding_mode", (_int32*)&switch_type);
	 ret_val = eti_set_seed_switch_type(switch_type);	
        //CHECK_VALUE(ret_val);

	em_settings_get_int_item("system.max_running_tasks", (_int32*)&max_tasks);
	 ret_val = eti_set_max_tasks(max_tasks);	
        CHECK_VALUE(ret_val);

	em_settings_get_int_item("system.download_limit_speed", (_int32*)&download_limit_speed);
	em_settings_get_int_item("system.upload_limit_speed", (_int32*)&upload_limit_speed);
	eti_set_limit_speed(download_limit_speed, upload_limit_speed);	
        //CHECK_VALUE(ret_val);

	
	em_settings_get_int_item("system.max_task_connection", (_int32*)&max_task_connection);
	 ret_val = eti_set_max_task_connection(max_task_connection);	
        CHECK_VALUE(ret_val);

	em_settings_get_int_item("system.vod_buffer_size", (_int32*)&vod_buffer_size);
	if(vod_buffer_size != 0)
	{
		ret_val = eti_vod_set_vod_buffer_size(vod_buffer_size * 1024);
		if(ret_val == NOT_IMPLEMENT)
			ret_val = SUCCESS;
		else
			CHECK_VALUE(ret_val);
	}

	em_settings_get_int_item("system.vod_buffer_time", (_int32*)&vod_buffer_time);
	if(vod_buffer_time != 0)
	{
		ret_val = eti_vod_set_buffer_time(vod_buffer_time);
		if(ret_val == NOT_IMPLEMENT)
			ret_val = SUCCESS;
		else
			CHECK_VALUE(ret_val);
	}


#if defined(MOBILE_PHONE)
	em_settings_get_int_item("system.download_piece_size", (_int32*)&piece_size);
	if(piece_size!=0)
	{
		settings_set_int_item("system.max_cmwap_range", piece_size/16);
	}

	em_memset(ui_version,0x00,64);
	em_settings_get_str_item("system.ui_version", ui_version);
	em_settings_get_int_item("system.ui_product", &ui_product);
	em_settings_get_int_item("system.ui_partner_id", &partner_id);
	if(em_strlen(ui_version)!=0)
	{
		settings_set_str_item("system.ui_version", ui_version);
		settings_set_int_item("system.ui_product", ui_product);
		settings_set_int_item((char*)"system.ui_partner_id", partner_id);
		et_reporter_set_version(ui_version, ui_product,partner_id);
		if(g_is_ui_reported == TRUE)
		{
			et_reporter_new_install(g_is_install);
		}
	}

	em_settings_get_int_item("system.p2p_mode", (_int32*)&p2p_mode);
	settings_set_int_item("system.p2p_mode", (_int32)p2p_mode);
	
	em_settings_get_bool_item("system.enable_cdn_mode",&enable_cdn_mode);
	settings_set_bool_item("system.enable_cdn_mode", enable_cdn_mode);
	
	em_settings_get_int_item("system.disable_cdn_speed", &disable_cdn_speed);
	settings_set_int_item("system.disable_cdn_speed", disable_cdn_speed);
	
	em_settings_get_int_item("system.enable_cdn_speed", &enable_cdn_speed);
	settings_set_int_item("system.enable_cdn_speed", enable_cdn_speed);
#endif

	/* ����et֪ͨetmǿ�Ƶ��ȵ�ִ�к��� */
	/* ����ʹ��:���޲��������etm��Ϣ����,������治�ϴ���������,��ʹ��etm��Ϣ�ڴ�����������et��etm���࿨��! */
	#ifdef _ANDROID_LINUX
	//et_set_etm_scheduler_functions(em_post_function_unlock,em_scheduler_by_et);
	#endif /* _ANDROID_LINUX */
	return ret_val;	
	
}

_int32 	em_stop_et( void )
{

	EM_LOG_ERROR("em_stop_et:g_et_wait_mac_timer_id=%u,g_et_running=%d,g_et_wait_mac=%d",g_et_wait_mac_timer_id,g_et_running,g_et_wait_mac);
	if(g_et_wait_mac_timer_id!=0)
	{
		em_cancel_timer(g_et_wait_mac_timer_id);
		g_et_wait_mac_timer_id = 0;
	}

	if((g_et_running==TRUE)||(g_et_wait_mac==TRUE))
	{
		eti_uninit( );
	}
	
	g_et_running = FALSE;
	g_et_wait_mac=FALSE;
	
	return SUCCESS;	
	
}

#if defined(ENABLE_NULL_PATH)
_int32 	em_force_stop_et( void )
{
	if(g_et_wait_mac_timer_id!=0)
	{
		em_cancel_timer(g_et_wait_mac_timer_id);
		g_et_wait_mac_timer_id = 0;
	}

	eti_uninit( );
	
	g_et_running = FALSE;
	g_et_wait_mac=FALSE;
	
	return SUCCESS;	
}

#endif
_int32 	em_restart_et( void )
{
	_int32 ret_val = SUCCESS,et_err=SUCCESS;
	//_u32 iap_id = 0;
	et_err = et_check_critical_error();//==USER_ABORT_NETWORK)

	ret_val=em_stop_et_sub_step( );
	CHECK_VALUE(ret_val);

	if(et_err==NEED_RECONNECT_NETWORK)
	{
		em_uninit_network_impl(FALSE);
		//em_sleep(60*1000);
		ret_val=em_init_default_network();
		CHECK_VALUE(ret_val);
	}
	else
	{
		ret_val=em_start_et_sub_step( );
	}
	
	return ret_val;	
	
}
_int32 	em_stop_et_sub_step( void )
{
	_int32 ret_val = SUCCESS;

	EM_LOG_DEBUG("em_stop_et_sub_step");

	
	ret_val = dt_clear_running_tasks_before_restart_et();
	CHECK_VALUE(ret_val);
	
	//ret_val = vod_clear_tasks();
	//CHECK_VALUE(ret_val);
	
	ret_val = em_stop_et();
	CHECK_VALUE(ret_val);

	return SUCCESS;	
	
}
_int32 	em_start_et_sub_step( void )
{
	static _u32 time_stamp = 0;
	_u32 cur_time = 0;
	_int32 ret_val = SUCCESS;

	EM_LOG_DEBUG("em_start_et_sub_step");

	
	if(time_stamp==0)
	{
		ret_val = em_time_ms(&time_stamp);
		CHECK_VALUE(ret_val);
	}
	else
	{
		ret_val = em_time_ms(&cur_time);
		CHECK_VALUE(ret_val);

		if(TIME_SUBZ(cur_time, time_stamp)<RESTART_ET_INTERVAL)
		{
			CHECK_VALUE(ET_RESTART_CRASH);
		}
		else
		{
			time_stamp = cur_time;
		}
	}

	em_sleep(50);

	ret_val = em_start_et();
	CHECK_VALUE(ret_val);

	ret_val = dt_restart_tasks();
	CHECK_VALUE(ret_val);   // ????
	
	return SUCCESS;	
	
}

_int32 em_save_license(const char * license)
{
	_int32 ret_val=SUCCESS;
	char buffer[64];
	em_memset(buffer,0,64);

	EM_LOG_DEBUG("em_save_license:%s",license);

	ret_val=em_license_encode(license,buffer);
	CHECK_VALUE(ret_val);
	
	ret_val=em_settings_set_str_item("system.license", buffer);
	CHECK_VALUE(ret_val);
	
	return SUCCESS;
}

_int32 em_load_license(char * license_buffer )
{
	_int32 ret_val=SUCCESS;
	char buffer[64];
	
	em_memset(buffer,0,64);
	
	ret_val=em_settings_get_str_item("system.license", buffer);
	CHECK_VALUE(ret_val);

	ret_val=em_license_decode(buffer,license_buffer);
	CHECK_VALUE(ret_val);

	EM_LOG_DEBUG("em_load_license:%s",license_buffer);
	
	return SUCCESS;
}

_int32 	em_set_certificate_path(void * p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	const char *certificate_path= (char * )_p_param->_para1;

	EM_LOG_DEBUG("em_set_certificate_path");
	

	if(g_et_running == FALSE)
	{
		_p_param->_result=em_start_et();	
	}
	else
	{
		_p_param->_result=eti_set_certificate_path(certificate_path);	
	}

	EM_LOG_DEBUG("em_set_certificate_path, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}


_int32 	em_open_drm_file(void * p_param)
{
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	char *p_file_full_path= (char * )_p_param->_para1;
    _u32 *p_drm_id = (_u32 *)_p_param->_para2;
    _u64 *p_origin_file_size = (_u64 *)_p_param->_para3;

	EM_LOG_DEBUG("em_open_drm_file");
	
	if(g_et_running == FALSE)
	{
		_p_param->_result=em_start_et();	
	}
	else
	{
		_p_param->_result=eti_open_drm_file(p_file_full_path, p_drm_id, p_origin_file_size);	
	}

	EM_LOG_DEBUG("em_open_drm_file, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 	em_is_certificate_ok(void * p_param)
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
    
    _u32 drm_id = (_u32)_p_param->_para1;
    BOOL *p_is_ok = (BOOL *)_p_param->_para2;

	EM_LOG_DEBUG("em_is_certificate_ok");
	

	if(g_et_running == FALSE)
	{
		_p_param->_result=em_start_et();	
	}
	else
	{
		_p_param->_result=eti_is_certificate_ok(drm_id, p_is_ok);	
	}

	EM_LOG_DEBUG("em_is_certificate_ok, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

/*
_int32 	em_read_drm_file(void * p_param)
{
	POST_PARA_5* _p_param = (POST_PARA_5*)p_param;
    
    _u32 drm_id = (_u32)_p_param->_para1;
    char *p_buffer = (char *)_p_param->_para2;
    _u32 size = (_u32)_p_param->_para3;
    _u64 file_pos = *((_u64*)_p_param->_para4);
    _u32 *p_read_size = (_u32 *)_p_param->_para5;

	EM_LOG_DEBUG("em_read_drm_file");
	

	if(g_et_running == FALSE)
	{
		_p_param->_result=em_start_et();	
	}
	else
	{
		_p_param->_result=eti_read_drm_file(drm_id,p_buffer,size,file_pos,p_read_size);	
	}

	EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}
*/
_int32	em_read_drm_file(_u32 drm_id, char *p_buffer, _u32 size, 
	_u64 file_pos, _u32 *p_read_size )
{
	EM_LOG_DEBUG("em_read_drm_file");
	if(g_et_running == FALSE)
	{
		return -1;	
	}
	return eti_read_drm_file(drm_id,p_buffer,size,file_pos,p_read_size);
}

_int32 	em_close_drm_file(void * p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	u32 drm_id = (u32)_p_param->_para1;

	EM_LOG_DEBUG("em_close_drm_file");
	

	if(g_et_running == FALSE)
	{
		_p_param->_result=em_start_et();	
	}
	else
	{
		_p_param->_result=eti_close_drm_file(drm_id);	
	}

	EM_LOG_DEBUG("em_close_drm_file, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 em_set_openssl_rsa_interface(_int32 fun_count, void *fun_ptr_table)
{
	_int32 ret_val = SUCCESS;
	
	ret_val = eti_set_openssl_rsa_interface(fun_count, fun_ptr_table);
	return ret_val;
}

/* Ԥռ10MB �Ŀռ�����ȷ��������˳������ */
_int32 em_ensure_free_disk( char *system_path)
{
	_int32 ret_val = SUCCESS;
	_u64 free_size = 0;
	char file_buffer[MAX_FULL_PATH_BUFFER_LEN];
	
	EM_LOG_DEBUG( "em_ensure_free_disk" );

	if(system_path == NULL || em_strlen(system_path) == 0)
	{
		return INVALID_FILE_PATH;
	}

	ret_val = sd_get_free_disk(system_path, &free_size);
	CHECK_VALUE(ret_val);

	if(free_size>ETM_EMPTY_DISK_FILE_SIZE/1024) return SUCCESS;
	
	em_memset(file_buffer,0,MAX_FULL_PATH_BUFFER_LEN);

	em_snprintf(file_buffer, MAX_FULL_PATH_BUFFER_LEN, "%s/%s", system_path,ETM_EMPTY_DISK_FILE);
	
	if(!em_file_exist(file_buffer)) return 28; //No space left on device
	
	ret_val = sd_delete_file(file_buffer);

    	EM_LOG_ERROR("em_ensure_free_disk:free_disk=%llu,sd_delete_file=%d",free_size,ret_val); 
	return ret_val;	
}

#ifdef _LOGGER
_int32 em_log_reload_initialize()
{
	_int32 ret_val = SUCCESS;
	
#ifdef ENABLE_ETM_MEDIA_CENTER
	em_settings_get_int_item("log.reload_interval", &g_reload_log_interval);

	EM_LOG_DEBUG("em_log_reload_initialize: g_reload_log_interval=%d", g_reload_log_interval);
	
	if (0 >= g_reload_log_interval)
	{
		return 0;
	}
	
	ret_val = em_start_timer(em_log_reload_timeout, NOTICE_INFINITE,g_reload_log_interval * 1000, 0,NULL,&g_log_reload_timer_id);  
	CHECK_VALUE(ret_val);
#endif /* ENABLE_ETM_MEDIA_CENTER */
	return ret_val;
}

_int32 em_log_reload_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid)
{
	_int32 ret_val = SUCCESS;
	char path[MAX_FILE_NAME_LEN] = {0};

#ifdef _TEST_RC	
	readlink(LOG_CONFIG_FILE_PATH, path, MAX_FILE_NAME_LEN);
#endif

	EM_LOG_DEBUG("em_log_reload_timeout, path:%s", path );
	
	if (MSG_TIMEOUT == errcode)
	{
		if (g_log_reload_timer_id == msgid)
		{
			ret_val = sd_log_reload(LOG_CONFIG_FILE);
			CHECK_VALUE(ret_val);
		}
	}

	return ret_val;
}
#endif

#if 0 //def ENABLE_ETM_MEDIA_CENTER
_int32 em_upgrade_initialze()
{
	_int32 ret_val = SUCCESS;
	
	em_settings_get_int_item("upgrade.check_interval", &g_upgrade_check_interval);

	EM_LOG_DEBUG("em_upgrade_initialze: g_upgrade_check_interval=%d", g_upgrade_check_interval);
	
	if (0 >= g_upgrade_check_interval)
	{
		return 0;
	}
	
	ret_val = em_start_timer(em_upgrade_timeout, NOTICE_ONCE, FIRST_TIME_UPGRADE_CHECK_DELAY * 1000, 0,NULL,&g_upgrade_check_timer_id);  
	CHECK_VALUE(ret_val);
	
	return ret_val;
}

_int32 em_upgrade_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid)
{
	_int32 ret_val = SUCCESS;
	
	EM_LOG_DEBUG("em_upgrade_timeout");
	
	if (MSG_TIMEOUT == errcode)
	{
		if (g_upgrade_check_timer_id == msgid)
		{
			ret_val = em_down_upgrade_files(FALSE);
			CHECK_VALUE(ret_val);
		}

		em_start_timer(em_upgrade_timeout, NOTICE_ONCE, g_upgrade_check_interval * 1000, 0,NULL,&g_upgrade_check_timer_id);  
	}

	return ret_val;	
}

_int32 em_down_upgrade_files(BOOL wait)
{
	pid_t pid;

	if (wait)
	{
		char cmd[1024];
		sd_snprintf(cmd, 1024, "%s %s", MEDIA_CENTER_UPGRADE_SCRIPT, iet_get_version());
		system(cmd);
		return SUCCESS;
	}

	pid = fork();
	if (0 == pid)
	{
		EM_LOG_DEBUG("new proc forked, pid=%d, execl %s %s", MEDIA_CENTER_UPGRADE_SCRIPT, iet_get_version());
		execl(MEDIA_CENTER_UPGRADE_SCRIPT, MEDIA_CENTER_UPGRADE_SCRIPT, iet_get_version(), NULL);
		exit(1);
	}
	else if(pid > 0)
	{
		return SUCCESS;
	}
	else
	{
		return errno;
	}
}

_int32 em_start_remote_control()
{
	POST_PARA_4 param;
	EM_RC_PATH data_path;
	int32 ret_val;

	ret_val = em_get_download_path_imp(data_path._data_path);
	CHECK_VALUE(ret_val);
    
    em_memset(&param,0,sizeof(POST_PARA_4));
    param._para1= (void*)&data_path;
    param._para2= (void*)1;
    param._para3= (void*)NULL;
	param._para4= (void*)FALSE;
    
    return rc_start(&param);	
}
#endif


_int32 em_start_search_server(void* p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;

	_p_param->_result= eti_start_search_server((ET_SEARCH_SERVER *) _p_param->_para1);

	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 em_stop_search_server(void* p_param)
{
	POST_PARA_0* _p_param = (POST_PARA_0*)p_param;

	_p_param->_result= et_stop_search_server();

	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 em_restart_search_server(void* p_param)
{
	POST_PARA_0* _p_param = (POST_PARA_0*)p_param;

	_p_param->_result= et_restart_search_server();

	return em_signal_sevent_handle(&(_p_param->_handle));
}



//////////////////////////////////////////////////////////////
/////17 �ֻ�������ص�http server

/* �����ؿ����ûص����� */
_int32 em_assistant_set_callback_func(void* p_param)
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	ET_ASSISTANT_INCOM_REQ_CALLBACK req_callback = (ET_ASSISTANT_INCOM_REQ_CALLBACK)_p_param->_para1;
	ET_ASSISTANT_SEND_RESP_CALLBACK resp_callback = (ET_ASSISTANT_SEND_RESP_CALLBACK)_p_param->_para2;
	
	_p_param->_result= eti_assistant_set_callback_func(req_callback,resp_callback);

	return em_signal_sevent_handle(&(_p_param->_handle));
}

/* �����͹ر�http server */
_int32 em_start_http_server(void* p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	u16 * port = (u16 *)_p_param->_para1; 

	_p_param->_result= eti_start_http_server(port);

	return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 em_stop_http_server(void* p_param)
{
	POST_PARA_0* _p_param = (POST_PARA_0*)p_param;

	_p_param->_result= et_stop_http_server();

	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 em_http_server_add_account(void* p_param)
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	char * username = (char *)_p_param->_para1; 
	char * password = (char *)_p_param->_para2; 

	_p_param->_result= et_http_server_add_account(username, password);

	return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 em_http_server_add_path(void* p_param)
{
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	char * real_path = (char *)_p_param->_para1; 
	char * virtual_path = (char *)_p_param->_para2; 
	BOOL  need_auth = (BOOL )_p_param->_para3; 
	
	_p_param->_result= et_http_server_add_path(real_path, virtual_path, need_auth);

	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 em_http_server_get_path_list(void* p_param)
{
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	char * virtual_path = (char *)_p_param->_para1; 
	char ** path_array_buffer = (char **)_p_param->_para2; 
	u32 * path_num = (u32 *)_p_param->_para3; 
	
	_p_param->_result= et_http_server_get_path_list(virtual_path, path_array_buffer, path_num);

	return em_signal_sevent_handle(&(_p_param->_handle));
}
_int32 em_http_server_set_callback_func(void* p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	void *callback = (void *)_p_param->_para1; 

	_p_param->_result= et_http_server_set_callback_func(callback);

	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 em_http_server_send_resp(void* p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	//FB_HS_AC *fb_hs_ac = (FB_HS_AC *)_p_param->_para1; 
	void *fb_hs_ac = (void *)_p_param->_para1; 
	
	_p_param->_result= et_http_server_send_resp(fb_hs_ac);

	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 em_http_server_cancel(void* p_param)
{

	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	u32 action_id = (u32)_p_param->_para1; 

	_p_param->_result= et_http_server_cancel(action_id);

	return em_signal_sevent_handle(&(_p_param->_handle));
}


/* ������Ӧ */
_int32 em_assistant_send_resp_file(void* p_param)
{
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	_u32 ma_id = (_u32)_p_param->_para1; 
	const char * resp_file = (const char*)_p_param->_para2; 
	void * user_data = (void*)_p_param->_para3; 
	
	_p_param->_result= et_assistant_send_resp_file(ma_id,resp_file,user_data);

	return em_signal_sevent_handle(&(_p_param->_handle));
}

/* ȡ���첽���� */
_int32 em_assistant_cancel(void* p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	_u32 ma_id = (_u32)_p_param->_para1; 

	_p_param->_result= et_assistant_cancel(ma_id);

	return em_signal_sevent_handle(&(_p_param->_handle));
}

/* et ֪ͨetmǿ�Ƶ��� */
_int32 em_scheduler_by_et(void* p_param)
{

	dt_scheduler();
	 
	 return SUCCESS;
}



