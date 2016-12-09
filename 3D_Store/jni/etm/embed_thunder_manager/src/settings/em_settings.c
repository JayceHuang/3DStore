/*--------------------------------------------------------------------------*/
/*                               IDENTIFICATION                             */
/*--------------------------------------------------------------------------*/
/*     Filename  : settings.c                                         */
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



#include "settings/em_settings.h"
#include "em_common/em_string.h"
#include "em_interface/em_fs.h"
#include "em_common/em_mempool.h"
#include "em_common/em_utility.h"
//#include "common/peerid.h"
//#include "common/local_ip.h"
#include "em_common/em_list.h"
//#include "em_interface/em_mem.h"
#include "em_interface/em_fs.h"
#include "scheduler/scheduler.h"

//#include "task_manager/task_manager.h"

#include "em_common/em_logid.h"
#ifdef EM_LOGID
#undef EM_LOGID
#endif

#define EM_LOGID EM_LOGID_SETTINGS

#include "em_common/em_logger.h"


/*--------------------------------------------------------------------------*/
/*                Global declarations				        */
/*--------------------------------------------------------------------------*/
/* Pointer of the task_manager->settings,only one settings when program is running */
static SETTINGS  g_em_settings ;
static SLAB *gp_em_settings_item_slab = NULL;
static TASK_LOCK  g_global_em_settings_lock ;

/*--------------------------------------------------------------------------*/
/*                Interfaces provid for other modules				        */
/*--------------------------------------------------------------------------*/
/* *_item_value 为一个in/out put 参数，若找不到与_item_name 对应的项则以*_item_value 为默认值生成新项 */
/*
	The type of _item_value is string
*/
_int32 em_settings_get_str_item(char * _item_name,char *_item_value)
{
	SETTINGS * _p_settings = &g_em_settings;
	LIST * _p_settings_item_list = NULL;
	_u32 _list_size = 0; 
	pLIST_NODE _p_list_node=NULL ;
	CONFIG_ITEM * _p_cfg_item = NULL;
	_int32 result = 0;

	EM_LOG_DEBUG("em_settings_get_str_item");
	

	if(_p_settings == NULL) return SETTINGS_ERR_UNKNOWN;
	if(_item_name == NULL) return SETTINGS_ERR_INVALID_ITEM_NAME;
	if(_item_value == NULL) return SETTINGS_ERR_INVALID_ITEM_VALUE;

     result = sd_task_lock ( &g_global_em_settings_lock );
     CHECK_VALUE(result);
	 
	 _p_settings_item_list = (LIST *)&( _p_settings->_item_list);
	 _list_size = em_list_size(_p_settings_item_list); 
		
	if(_list_size == 0)
	{
	 	goto ADD_AS_DEFAULT;
	}
	
	_p_list_node = LIST_BEGIN(*_p_settings_item_list);
	while(_list_size)
	{
		_p_cfg_item = (CONFIG_ITEM * )LIST_VALUE(_p_list_node);
		if(em_strcmp(_item_name, _p_cfg_item->_name) == 0)
		{
			em_strncpy(_item_value, _p_cfg_item->_value, MAX_CFG_VALUE_LEN-1);
			break;
		}
		
			_p_list_node = LIST_NEXT(_p_list_node);
			_list_size--;
	}
	
	if(_list_size == 0)
	{
	 	goto ADD_AS_DEFAULT;
	}
	
       sd_task_unlock ( &g_global_em_settings_lock );
	
	return SUCCESS;
	
ADD_AS_DEFAULT:
	_p_cfg_item = NULL;

	result = em_mpool_get_slip(gp_em_settings_item_slab, (void**)&_p_cfg_item);
	//result = sd_malloc(sizeof(CONFIG_ITEM) ,(void**)&_p_cfg_item);
	if(result!=SUCCESS)
	{
       	sd_task_unlock ( &g_global_em_settings_lock );
		CHECK_VALUE(result);   
	}

	em_memset(_p_cfg_item,0,sizeof(CONFIG_ITEM));
	sd_assert(em_strlen(_item_name)<MAX_CFG_NAME_LEN);
	em_strncpy(_p_cfg_item->_name,_item_name,MAX_CFG_NAME_LEN-1);
	em_trim_prefix_lws(_p_cfg_item->_name);
	em_trim_postfix_lws( _p_cfg_item->_name);
							
	sd_assert(em_strlen(_item_value)<MAX_CFG_VALUE_LEN);
	em_strncpy(_p_cfg_item->_value,_item_value,MAX_CFG_VALUE_LEN-1);
	em_trim_prefix_lws(_p_cfg_item->_value);
	em_trim_postfix_lws( _p_cfg_item->_value);
	sd_assert(em_strlen(_p_cfg_item->_name)!=0);
	result =em_list_push(_p_settings_item_list, _p_cfg_item);
       sd_task_unlock ( &g_global_em_settings_lock );
	if(result !=SUCCESS)
	{
		em_mpool_free_slip(gp_em_settings_item_slab,(void*) _p_cfg_item);
		//SAFE_DELETE(_p_cfg_item);
		CHECK_VALUE(result);   
	}
	/* Save to config file */
	return em_settings_config_save();
	
}
/*
	The type of _item_value is int
*/
_int32 em_settings_get_int_item(char * _item_name,_int32 *_item_value)
{
	char str_value[MAX_CFG_VALUE_LEN];
	_int32 result = 0;

	EM_LOG_DEBUG("em_settings_get_int_item");
	
	
	em_snprintf(str_value,MAX_CFG_VALUE_LEN,"%d",*_item_value);
	
	result =em_settings_get_str_item( _item_name,str_value);
	CHECK_VALUE(result);
	
	*_item_value = em_atoi(str_value);
	
	return SUCCESS;
	
}

_int32 em_settings_get_bool_item(char * _item_name,BOOL *_item_value)
{
	_int32 _int_value = 0;
	_int32 result = SUCCESS;

	EM_LOG_DEBUG("em_settings_get_bool_item");
	
	
	if(*_item_value)
	   _int_value = 1;

	result =em_settings_get_int_item( _item_name,&_int_value);
	CHECK_VALUE(result);

	if(_int_value == 0)
	*_item_value = FALSE;
	else
	*_item_value = TRUE;
	
	return SUCCESS;
	
}


/*
	The type of _item_value is string
*/
_int32 em_settings_set_str_item(char * _item_name,char *_item_value)
{
	SETTINGS * _p_settings = &g_em_settings;
	LIST * _p_settings_item_list = NULL;
	_u32 _list_size = 0; 
	pLIST_NODE _p_list_node=NULL ;
	CONFIG_ITEM * _p_cfg_item = NULL;
	_int32 result =SUCCESS;

	EM_LOG_DEBUG("em_settings_set_str_item");
	

	if(_p_settings == NULL) return SETTINGS_ERR_UNKNOWN;
	if(_item_name == NULL) return SETTINGS_ERR_INVALID_ITEM_NAME;
	if(_item_value == NULL) return SETTINGS_ERR_INVALID_ITEM_VALUE;

     result = sd_task_lock ( &g_global_em_settings_lock );
     CHECK_VALUE(result);
	 
	_p_settings_item_list = (LIST *)&( _p_settings->_item_list);
	_list_size = em_list_size(_p_settings_item_list); 
	if(_list_size == 0)
	{
	 	goto ADD_AS_DEFAULT;
	}
	
	_p_list_node = LIST_BEGIN(*_p_settings_item_list);

	while(_list_size)
	{
		_p_cfg_item = (CONFIG_ITEM * )LIST_VALUE(_p_list_node);
		if(em_strcmp(_item_name, _p_cfg_item->_name) == 0)
		{
			em_strncpy(_p_cfg_item->_value,_item_value,  MAX_CFG_VALUE_LEN-1);
			break;
		}
		
			_p_list_node = LIST_NEXT(_p_list_node);
			_list_size--;
	}
	
	
	if(_list_size == 0)
	{
	 	goto ADD_AS_DEFAULT;
	}
	
	
	em_settings_config_save();
       sd_task_unlock ( &g_global_em_settings_lock );

	return SUCCESS;

ADD_AS_DEFAULT:
	_p_cfg_item = NULL;

	result = em_mpool_get_slip(gp_em_settings_item_slab, (void**)&_p_cfg_item);
	//result = sd_malloc(sizeof(CONFIG_ITEM) ,(void**)&_p_cfg_item);
	if(result!=SUCCESS)
	{
	       sd_task_unlock ( &g_global_em_settings_lock );
		CHECK_VALUE(result);   
	}

	em_memset(_p_cfg_item,0,sizeof(CONFIG_ITEM));
	sd_assert(em_strlen(_item_name)<MAX_CFG_NAME_LEN);
	em_strncpy(_p_cfg_item->_name,_item_name,MAX_CFG_NAME_LEN-1);
	em_trim_prefix_lws(_p_cfg_item->_name);
	em_trim_postfix_lws( _p_cfg_item->_name);
							
	sd_assert(em_strlen(_item_value)<MAX_CFG_VALUE_LEN);
	em_strncpy(_p_cfg_item->_value,_item_value,MAX_CFG_VALUE_LEN-1);
	em_trim_prefix_lws(_p_cfg_item->_value);
	em_trim_postfix_lws( _p_cfg_item->_value);

	sd_assert(em_strlen(_p_cfg_item->_name)!=0);
	result =em_list_push(_p_settings_item_list, _p_cfg_item);
	if(result !=SUCCESS)
	{
		em_mpool_free_slip(gp_em_settings_item_slab,(void*) _p_cfg_item);
		//SAFE_DELETE(_p_cfg_item);
		sd_task_unlock ( &g_global_em_settings_lock );
		CHECK_VALUE(result);   
	}
	
	/* Save to config file */
	result = em_settings_config_save();
	sd_task_unlock ( &g_global_em_settings_lock );

	return result;

	
	
}
/*
	The type of _item_value is string
*/
_int32 em_settings_set_int_item(char * _item_name,_int32 _item_value)
{
	char str_value[MAX_CFG_VALUE_LEN];

	EM_LOG_DEBUG("em_settings_set_int_item");
	

	em_snprintf(str_value,MAX_CFG_VALUE_LEN,"%d",_item_value);

	return em_settings_set_str_item( _item_name,str_value);
	
}
/*
	The type of _item_value is bool
*/
_int32 em_settings_set_bool_item(char * _item_name,BOOL _item_value)
{
	_int32 int_value = 0;

	EM_LOG_DEBUG("em_settings_set_bool_item");
	

	if(_item_value)
		int_value = 1;
	
	return em_settings_set_int_item( _item_name,int_value);
	
}
_int32 em_settings_del_item(char * _item_name)
{
	SETTINGS * _p_settings = &g_em_settings;
	LIST * _p_settings_item_list = NULL;
	_u32 _list_size = 0; 
	LIST_NODE * _p_list_node=NULL ;
	CONFIG_ITEM * _p_cfg_item = NULL;
	_int32 result = SUCCESS;

	EM_LOG_DEBUG("em_settings_del_item");
	

	if(_p_settings == NULL) return SETTINGS_ERR_UNKNOWN;
	if(_item_name == NULL) return SETTINGS_ERR_INVALID_ITEM_NAME;

	_p_settings_item_list = (LIST *)&( _p_settings->_item_list);
	_list_size = em_list_size(_p_settings_item_list); 

	if(_list_size == 0)
	{
	 	CHECK_VALUE(SETTINGS_ERR_LIST_EMPTY);
	}
	
	_p_list_node = LIST_BEGIN(*_p_settings_item_list);
	while(_list_size)
	{
		_p_cfg_item = (CONFIG_ITEM * )LIST_VALUE(_p_list_node);
		if(em_strcmp(_item_name, _p_cfg_item->_name) == 0)
		{
			result = em_list_erase(_p_settings_item_list, _p_list_node);
			CHECK_VALUE(result);
			em_mpool_free_slip(gp_em_settings_item_slab,(void*) _p_cfg_item);
			//SAFE_DELETE(_p_cfg_item);
			return SUCCESS;
		}
		
			_p_list_node = LIST_NEXT(_p_list_node);
			_list_size--;
	}

	CHECK_VALUE(SETTINGS_ERR_ITEM_NOT_FOUND);
	return SUCCESS;
}



/*--------------------------------------------------------------------------*/
/*                Interfaces provid just for task_manager				        */
/*--------------------------------------------------------------------------*/


_int32 em_settings_initialize(void)
{
	_int32 result = SUCCESS;
	SETTINGS * _p_settings = &g_em_settings;
	//_int32 _list_size = 0;
	//CONFIG_ITEM * _p_cfg_item = NULL;

	EM_LOG_DEBUG("em_settings_initialize");
	
	if(gp_em_settings_item_slab==NULL)
	{
		result = em_mpool_create_slab(sizeof(CONFIG_ITEM), EM_MIN_SETTINGS_ITEM_MEMORY, 0, &gp_em_settings_item_slab);
		CHECK_VALUE(result);
	}
	else
	{
		return SUCCESS;
	}

       result = sd_init_task_lock(&g_global_em_settings_lock);	   
	CHECK_VALUE(result);

	em_memset(_p_settings,0,sizeof(SETTINGS));
	
//#ifdef ENABLE_CFG_FILE
	_p_settings->_enable_cfg_file=TRUE;
//#endif

	/* Get tht settings items from the config file */
	if(_p_settings->_enable_cfg_file==TRUE)
	{
		em_settings_config_load(ETM_CONFIG_FILE,&(_p_settings->_item_list));
	}
	else
	{
		em_list_init(&(_p_settings->_item_list));
	}
	
	return SUCCESS;

}

_int32 em_settings_uninitialize( void)
{
	_int32 _list_size = 0;
	_int32 _result = SUCCESS;
	SETTINGS * _p_settings = &g_em_settings;
	CONFIG_ITEM * _p_cfg_item = NULL;
	
	/* Save the settings items to the config file */
	em_settings_config_save();
	
	_list_size =  em_list_size(&(_p_settings->_item_list)); 
	while(_list_size)
	{
		_result = em_list_pop(&(_p_settings->_item_list), (void**)&_p_cfg_item);
 		CHECK_VALUE(_result);
		em_mpool_free_slip(gp_em_settings_item_slab,(void*) _p_cfg_item);
		//SAFE_DELETE(_p_cfg_item);
		_list_size--;
	}
	
       sd_uninit_task_lock(&g_global_em_settings_lock);	   
	   
	if(gp_em_settings_item_slab!=NULL)
	{
		_result =em_mpool_destory_slab(gp_em_settings_item_slab); 
		CHECK_VALUE(_result);
		gp_em_settings_item_slab=NULL;
	}

	return SUCCESS;
}
/*
_cfg_file_name -> full path name of the config file; e.g. root/downloder/config.cfg
*/
_int32 em_settings_config_load(char* _cfg_file_name,LIST * _p_settings_item_list)
{
	CONFIG_ITEM * _p_cfg_item = NULL;
	char Rbuffer[MAX_CFG_LINE_LEN],tbuffer[MAX_CFG_LINE_LEN*2];
	char * p_eq = NULL,*p_LF = NULL;
	char * system_path = em_get_system_path();
	_u32 read_size = 0, file_id = 0;
	_int32 result = SUCCESS,_len = 0,_dispose=0;

	em_list_init(_p_settings_item_list);
	
	if(em_strlen(system_path)==0)
	{
		return INVALID_FILE_PATH;
	}
	
	em_memset(tbuffer,0,MAX_CFG_LINE_LEN*2);
	em_snprintf(tbuffer, MAX_CFG_LINE_LEN*2, "%s/%s", system_path, _cfg_file_name);
	
	EM_LOG_DEBUG("em_settings_config_load:%s",tbuffer);
	if(em_strlen(tbuffer) < 1)
	{
		/* File name error */
		CHECK_VALUE(SETTINGS_ERR_INVALID_FILE_NAME);
	}

	if(em_file_exist(tbuffer) == FALSE)
	{
		/* Config File is not exist */
		//return SETTINGS_ERR_CFG_FILE_NOT_EXIST;
	}

	result = em_open_ex(tbuffer, O_FS_CREATE, &file_id);
	if(result != SUCCESS)
	{
		/* Opening File error */
		CHECK_VALUE(result);
	}

	em_memset(tbuffer,0,MAX_CFG_LINE_LEN*2);
	
	while(TRUE) 
	{
		em_memset(Rbuffer,0,MAX_CFG_LINE_LEN);
		result = em_read(file_id, Rbuffer, MAX_CFG_LINE_LEN-1, &read_size);
		if(result != SUCCESS)
		{
			EM_LOG_DEBUG("em_settings_config_load,em_read error:%d,file_id=%u,read_size=%u",result,file_id,read_size);
			break;
		}
		if(read_size )
		{
			p_eq = NULL;
			p_LF = NULL;
			_dispose=0;
			em_strcat(tbuffer, Rbuffer,read_size);

			while(TRUE) 
			{
			 	p_eq = NULL;
			 	p_LF = NULL;
				p_LF = em_strchr(tbuffer+_dispose,'\n', 0);
				if(p_LF!=NULL)
				{
					p_eq = em_strchr(tbuffer+_dispose,'=', 0) ;
 					if(p_eq!=NULL)
					{
						if(p_eq<p_LF)
						{
							result = em_mpool_get_slip(gp_em_settings_item_slab, (void**)&_p_cfg_item);
			  				//result = sd_malloc(sizeof(CONFIG_ITEM) ,(void**)&_p_cfg_item);
		 					if(result!=SUCCESS)
		 					{
		 						em_close_ex(file_id);	
								CHECK_VALUE(result);
		 						//return result;   
		 					}

							em_memset(_p_cfg_item,0,sizeof(CONFIG_ITEM));
							if(p_eq-tbuffer-_dispose>MAX_CFG_NAME_LEN-1)
							em_strncpy(_p_cfg_item->_name,tbuffer+_dispose,MAX_CFG_NAME_LEN-1);
							else
							em_memcpy(_p_cfg_item->_name,tbuffer+_dispose,p_eq-tbuffer-_dispose);
							em_trim_prefix_lws(_p_cfg_item->_name);
							em_trim_postfix_lws( _p_cfg_item->_name);
							
							if(p_LF-p_eq-1>MAX_CFG_VALUE_LEN-1)
							em_strncpy(_p_cfg_item->_value,p_eq+1,MAX_CFG_VALUE_LEN-1);
							else
							em_memcpy(_p_cfg_item->_value,p_eq+1,p_LF-p_eq-1);
							em_trim_prefix_lws(_p_cfg_item->_value);
							em_trim_postfix_lws( _p_cfg_item->_value);

							sd_assert(em_strlen(_p_cfg_item->_name)!=0);
							EM_LOG_DEBUG("em_settings_config_load[%u],%s=%s",em_list_size(_p_settings_item_list),_p_cfg_item->_name, _p_cfg_item->_value);
							result = em_list_push(_p_settings_item_list, _p_cfg_item);
							if(result != SUCCESS)
		 					{
		 						em_mpool_free_slip(gp_em_settings_item_slab,(void*) _p_cfg_item);
		 						em_close_ex(file_id);	
								CHECK_VALUE(result);
		 						//return result;   
		 					}
					
						}
					}
						
					_dispose=p_LF-tbuffer+1;
					if(_dispose>em_strlen(tbuffer)) 
					{
		 				em_close_ex(file_id);	 						
		 				CHECK_VALUE(SETTINGS_ERR_INVALID_LINE);   
					}
				}
				else
				{
					_len = em_strlen(tbuffer+_dispose);
					if(_len != 0)
					{
						if(_len>MAX_CFG_LINE_LEN*2-1)
						em_strncpy(tbuffer, tbuffer+_dispose, MAX_CFG_LINE_LEN*2-1);
						else
						em_memcpy(tbuffer, tbuffer+_dispose, _len);
					}
					
					tbuffer[_len] = '\0';
					
					break;
				}
			}
		}
		else
		{
			break;   
		}

	}
	em_close_ex(file_id);	 

	CHECK_VALUE( result);
	
	return SUCCESS;

}

_int32 em_settings_config_save(void)
{
	_u32 file_id = 0,writesize = 0;
	pLIST_NODE _p_list_node=NULL ;
	CONFIG_ITEM * _p_cfg_item = NULL;
	char line_buffer[MAX_CFG_LINE_LEN];
	char path_buffer[MAX_FULL_PATH_BUFFER_LEN];
	char* _cfg_file_name=ETM_CONFIG_FILE;
	char* system_path = em_get_system_path();
	SETTINGS * _p_settings = &g_em_settings;
	LIST * _p_settings_item_list = NULL;
	_u32 _list_size = 0;
	_int32 result = 0;
	char  buffer[2048];
	_u32 buffer_pos=0,buffer_len=2048;

	EM_LOG_DEBUG("em_settings_config_save:enable_cfg_file=%d,list_size=%u",_p_settings->_enable_cfg_file,em_list_size(_p_settings_item_list));
	

	if(_p_settings == NULL) return SETTINGS_ERR_UNKNOWN;

	/* Check if config file is enable */
	if(_p_settings->_enable_cfg_file!=TRUE)
	{
		return SUCCESS;
	}
	
	_p_settings_item_list = (LIST *)&( _p_settings->_item_list);
	 _list_size = em_list_size(_p_settings_item_list); 

	if(_list_size == 0)
	{
	 	return SETTINGS_ERR_LIST_EMPTY;
	}

	if(em_strlen(system_path)==0)
	{
		return INVALID_FILE_PATH;
	}
	
	em_memset(path_buffer,0,MAX_FULL_PATH_BUFFER_LEN);
	em_snprintf(path_buffer, MAX_FULL_PATH_BUFFER_LEN, "%s/%s", system_path, _cfg_file_name);
	
	if(em_strlen(path_buffer) < 1)
	{
		/* File name error */
		CHECK_VALUE( SETTINGS_ERR_INVALID_FILE_NAME);
	}

	if(em_file_exist(path_buffer) == TRUE)
	{
		/* Config File is exist,delete it first */
		result = em_delete_file(path_buffer);
		CHECK_VALUE( result);
	}

	result = em_open_ex(path_buffer, O_FS_CREATE, &file_id);
	EM_LOG_DEBUG("em_settings_config_save:result=%d,file_id=%u,%s",result,file_id,path_buffer);
	CHECK_VALUE( result);
	
	_p_list_node = LIST_BEGIN(*_p_settings_item_list);
	while(_list_size)
	{
		_p_cfg_item = (CONFIG_ITEM * )LIST_VALUE(_p_list_node);
		
		EM_LOG_DEBUG("em_settings_config_save[%u]:%s=%s",_list_size,_p_cfg_item->_name,_p_cfg_item->_value);
		if(em_strlen(_p_cfg_item->_name)+em_strlen(_p_cfg_item->_value)+2>MAX_CFG_LINE_LEN)
			break;
		
		em_snprintf(line_buffer,MAX_CFG_LINE_LEN,"%s=%s\n",_p_cfg_item->_name,_p_cfg_item->_value);

		result = em_write_save_to_buffer(file_id, buffer, buffer_len,&buffer_pos, line_buffer, em_strlen(line_buffer));
		if(result!=SUCCESS)
		{
			em_close_ex(file_id);	 
			CHECK_VALUE( result);
		}
			
			_p_list_node = LIST_NEXT(_p_list_node);
			_list_size--;
	}
	
	if(buffer_pos!=0)
	{
		result =em_write(file_id,buffer, buffer_pos, &writesize);
		sd_assert(buffer_pos==writesize);
	}

	em_close_ex(file_id);	 
	return SUCCESS;
	
}

