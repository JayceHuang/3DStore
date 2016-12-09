#if !defined(__SETTINGS_C_20080731)
#define __SETTINGS_C_20080731
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



#include "utility/settings.h"
#include "utility/string.h"
#include "platform/sd_fs.h"
#include "utility/mempool.h"
#include "utility/utility.h"
#include "utility/peerid.h"
#include "utility/local_ip.h"
#include "utility/list.h"
#include "platform/sd_mem.h"
#include "platform/sd_task.h"

#if defined(MACOS)&&defined(MOBILE_PHONE)
//由于IOS的应用沙盒原理，所以这个东西是为了解决文件读写权限的问题
#include "sd_ios_device_info.h"
#endif

#include "utility/logid.h"
#ifdef LOGID
#undef LOGID
#endif

#define LOGID LOGID_SETTINGS

#include "utility/logger.h"


/*--------------------------------------------------------------------------*/
/*                Global declarations				        */
/*--------------------------------------------------------------------------*/
/* Pointer of the task_manager->settings,only one settings when program is running */
static SETTINGS  g_settings ;
static SLAB *gp_settings_item_slab = NULL;
static TASK_LOCK  g_global_settings_lock ;

/*--------------------------------------------------------------------------*/
/*                Interfaces provid for other modules				        */
/*--------------------------------------------------------------------------*/
/* *_item_value 为一个in/out put 参数，若找不到与_item_name 对应的项则以*_item_value 为默认值生成新项 */
/*
	The type of _item_value is string
*/

static char g_system_path[MAX_CFG_LINE_LEN + 50] = {0};

_int32 settings_get_str_item(char * _item_name, char *_item_value)
{
	SETTINGS * _p_settings = &g_settings;
	LIST * _p_settings_item_list = NULL;
	_u32 _list_size = 0; 
	pLIST_NODE _p_list_node=NULL ;
	CONFIG_ITEM * _p_cfg_item = NULL;
	_int32 result = 0;

    LOG_DEBUG("settings_get_str_item 000, item_name = %s, item_value = %s"
	    , _item_name, _item_value);

	if (_p_settings == NULL)
	{
	     LOG_DEBUG("settings_get_str_item 000.0, item_name = %s, item_value = %s"
	    , _item_name, _item_value);   
	    return SETTINGS_ERR_UNKNOWN;
	}
	if (_item_name == NULL) 
	{
	    LOG_DEBUG("settings_get_str_item 000.1, item_name = %s, item_value = %s"
	    , _item_name, _item_value);
	    return SETTINGS_ERR_INVALID_ITEM_NAME;
	}
	if (_item_value == NULL) 
	{
	    LOG_DEBUG("settings_get_str_item 000.2, item_name = %s, item_value = %s"
	    , _item_name, _item_value);
	    return SETTINGS_ERR_INVALID_ITEM_VALUE;
    }

     result = sd_task_lock(&g_global_settings_lock);
     CHECK_VALUE(result);
	 
	 _p_settings_item_list = (LIST *)&(_p_settings->_item_list);
	 _list_size = list_size(_p_settings_item_list);
	if (_list_size == 0)
	{
	 	goto ADD_AS_DEFAULT;
	}
	
	_p_list_node = LIST_BEGIN(*_p_settings_item_list);
	while (_list_size)
	{
		_p_cfg_item = (CONFIG_ITEM *)LIST_VALUE(_p_list_node);
		if (sd_strcmp(_item_name, _p_cfg_item->_name) == 0)
		{
			sd_strncpy(_item_value, _p_cfg_item->_value, (MAX_CFG_VALUE_LEN - 1));
			break;
		}
		
        _p_list_node = LIST_NEXT(_p_list_node);
        _list_size--;
	}
	
	if(_list_size == 0)
	{
	 	goto ADD_AS_DEFAULT;
	}

	sd_task_unlock ( &g_global_settings_lock );

    LOG_DEBUG("settings_get_str_item 111, item_name = %s, item_value = %s"
	    , _item_name, _item_value);
	return SUCCESS;
	
ADD_AS_DEFAULT:
	_p_cfg_item = NULL;

	result = mpool_get_slip(gp_settings_item_slab, (void**)&_p_cfg_item);
	if (result != SUCCESS)
	{
       	sd_task_unlock ( &g_global_settings_lock );
		CHECK_VALUE(result);   
	}

	sd_memset(_p_cfg_item, 0, sizeof(CONFIG_ITEM));

	sd_assert(sd_strlen(_item_name) < MAX_CFG_NAME_LEN);	
	sd_strncpy(_p_cfg_item->_name, _item_name, MAX_CFG_NAME_LEN-1);
	sd_trim_prefix_lws(_p_cfg_item->_name);
	sd_trim_postfix_lws( _p_cfg_item->_name);
							
	sd_assert(sd_strlen(_item_value) < MAX_CFG_VALUE_LEN);
	sd_strncpy(_p_cfg_item->_value, _item_value, MAX_CFG_VALUE_LEN-1);
	sd_trim_prefix_lws(_p_cfg_item->_value);
	sd_trim_postfix_lws( _p_cfg_item->_value);

	result =list_push(_p_settings_item_list, _p_cfg_item);

	sd_task_unlock (&g_global_settings_lock);
	   
	if (result !=SUCCESS)
	{
		mpool_free_slip(gp_settings_item_slab,(void*) _p_cfg_item);
		CHECK_VALUE(result);
	}

    LOG_DEBUG("settings_get_str_item 111, item_name = %s, item_value = %s"
	    , _item_name, _item_value);
	
	/* Save to config file */
	return SUCCESS;//settings_config_save();
	
}
/*
	The type of _item_value is int
*/
_int32 settings_get_int_item(char * _item_name, _int32 *_item_value)
{
	char str_value[MAX_CFG_VALUE_LEN] = {0};
	_int32 result = 0;
	
	sd_snprintf(str_value, MAX_CFG_VALUE_LEN, "%d", *_item_value);
	
	result = settings_get_str_item(_item_name, str_value);
	CHECK_VALUE(result);
	
	*_item_value = sd_atoi(str_value);

	LOG_DEBUG("settings_get_int_item, item_name = %s, item_value = %d"
	    , _item_name, *_item_value);
	return SUCCESS;
}

_int32 settings_get_bool_item(char * _item_name, BOOL *_item_value)
{
	_int32 _int_value = 0;
	_int32 result = SUCCESS;
	
	if (*_item_value)
	{
	    _int_value = 1;
	}

	result =settings_get_int_item(_item_name, &_int_value);
	CHECK_VALUE(result);

	*_item_value = (0 == _int_value) ? FALSE : TRUE;

	LOG_DEBUG("settings_get_bool_item, item_name = %s, item_value = %d"
	    , _item_name, *_item_value);
	
	return SUCCESS;
}


/*
	The type of _item_value is string
*/
_int32 settings_set_str_item(char * _item_name,char *_item_value)
{
	SETTINGS * _p_settings = &g_settings;
	LIST * _p_settings_item_list = NULL;
	_u32 _list_size = 0; 
	pLIST_NODE _p_list_node=NULL ;
	CONFIG_ITEM * _p_cfg_item = NULL;
	_int32 result =SUCCESS;

	LOG_DEBUG("settings_set_bool_item, item_name = %s, item_value = %s"
	    , _item_name, _item_value);

	

	if(_p_settings == NULL) return SETTINGS_ERR_UNKNOWN;
	if(_item_name == NULL) return SETTINGS_ERR_INVALID_ITEM_NAME;
	if(_item_value == NULL) return SETTINGS_ERR_INVALID_ITEM_VALUE;

     result = sd_task_lock ( &g_global_settings_lock );
     CHECK_VALUE(result);
	 
	_p_settings_item_list = (LIST *)&( _p_settings->_item_list);
	_list_size = list_size(_p_settings_item_list); 
	if(_list_size == 0)
	{
	 	goto ADD_AS_DEFAULT;
	}
	
	_p_list_node = LIST_BEGIN(*_p_settings_item_list);

	while(_list_size)
	{
		_p_cfg_item = (CONFIG_ITEM * )LIST_VALUE(_p_list_node);
		if(sd_strcmp(_item_name, _p_cfg_item->_name) == 0)
		{
			sd_strncpy(_p_cfg_item->_value,_item_value,  MAX_CFG_VALUE_LEN-1);
			break;
		}
		
			_p_list_node = LIST_NEXT(_p_list_node);
			_list_size--;
	}
	
	
	if(_list_size == 0)
	{
	 	goto ADD_AS_DEFAULT;
	}
	
       sd_task_unlock ( &g_global_settings_lock );

	return SUCCESS;
	
ADD_AS_DEFAULT:
	_p_cfg_item = NULL;

	result = mpool_get_slip(gp_settings_item_slab, (void**)&_p_cfg_item);
	//result = sd_malloc(sizeof(CONFIG_ITEM) ,(void**)&_p_cfg_item);
	if(result!=SUCCESS)
	{
	       sd_task_unlock ( &g_global_settings_lock );
		CHECK_VALUE(result);   
	}

	sd_memset(_p_cfg_item,0,sizeof(CONFIG_ITEM));
	sd_strncpy(_p_cfg_item->_name,_item_name,MAX_CFG_NAME_LEN-1);
	sd_trim_prefix_lws(_p_cfg_item->_name);
	sd_trim_postfix_lws( _p_cfg_item->_name);
							
	sd_strncpy(_p_cfg_item->_value,_item_value,MAX_CFG_VALUE_LEN-1);
	sd_trim_prefix_lws(_p_cfg_item->_value);
	sd_trim_postfix_lws( _p_cfg_item->_value);

	result =list_push(_p_settings_item_list, _p_cfg_item);
	if(result !=SUCCESS)
	{
		mpool_free_slip(gp_settings_item_slab,(void*) _p_cfg_item);
		//SAFE_DELETE(_p_cfg_item);
		sd_task_unlock ( &g_global_settings_lock );
		CHECK_VALUE(result);   
	}
	
	/* Save to config file */
	result = settings_config_save();
	sd_task_unlock ( &g_global_settings_lock );
	
	return result;
}
/*
	The type of _item_value is string
*/
_int32 settings_set_int_item(char * _item_name,_int32 _item_value)
{
	char str_value[MAX_CFG_VALUE_LEN];

	LOG_DEBUG("settings_set_int_item, item_name = %s, item_value = %d"
	    , _item_name, _item_value);

	

	sd_snprintf(str_value,MAX_CFG_VALUE_LEN,"%d",_item_value);

	return settings_set_str_item( _item_name,str_value);
	
}
/*
	The type of _item_value is bool
*/
_int32 settings_set_bool_item(char * _item_name,BOOL _item_value)
{
	_int32 int_value = 0;

	LOG_DEBUG("settings_set_bool_item, item_name = %s, item_value = %d"
	    , _item_name, _item_value);
	

	if(_item_value)
		int_value = 1;
	
	return settings_set_int_item( _item_name,int_value);
	
}
/*
_int32 settings_del_item(char * _item_name)
{
	SETTINGS * _p_settings = &g_settings;
	LIST * _p_settings_item_list = NULL;
	_u32 _list_size = 0; 
	LIST_NODE * _p_list_node=NULL ;
	CONFIG_ITEM * _p_cfg_item = NULL;
	_int32 result = SUCCESS;

	LOG_DEBUG("settings_del_item");
	

	if(_p_settings == NULL) return SETTINGS_ERR_UNKNOWN;
	if(_item_name == NULL) return SETTINGS_ERR_INVALID_ITEM_NAME;

	_p_settings_item_list = (LIST *)&( _p_settings->_item_list);
	_list_size = list_size(_p_settings_item_list); 

	if(_list_size == 0)
	{
	 	CHECK_VALUE(SETTINGS_ERR_LIST_EMPTY);
	}
	
	_p_list_node = LIST_BEGIN(*_p_settings_item_list);
	while(_list_size)
	{
		_p_cfg_item = (CONFIG_ITEM * )LIST_VALUE(_p_list_node);
		if(sd_strcmp(_item_name, _p_cfg_item->_name) == 0)
		{
			result = list_erase(_p_settings_item_list, _p_list_node);
			CHECK_VALUE(result);
			mpool_free_slip(gp_settings_item_slab,(void*) _p_cfg_item);
			//SAFE_DELETE(_p_cfg_item);
			return SUCCESS;
		}
		
			_p_list_node = LIST_NEXT(_p_list_node);
			_list_size--;
	}

	CHECK_VALUE(SETTINGS_ERR_ITEM_NOT_FOUND);
	return SUCCESS;
}
*/


/*--------------------------------------------------------------------------*/
/*                Interfaces provid just for task_manager				        */
/*--------------------------------------------------------------------------*/

#if defined(MACOS)&&defined(MOBILE_PHONE)
char* malloc_settings_file_path(void)
{
    char* config_file_path = (char*)malloc(MAX_FILE_PATH_LEN);
    sd_memset(config_file_path, 0, MAX_FILE_PATH_LEN);
    
//目前决定越狱版和非越狱的APP都先放到程序目录里，统一
//#if defined(JAILBREAK)
//    sd_snprintf(config_file_path, MAX_FILE_PATH_LEN, "%s",CONFIG_FILE);
//#else
    sd_snprintf(config_file_path, MAX_FILE_PATH_LEN, "%s/Library/.config/",get_app_home_path());
    if (!sd_file_exist(config_file_path))
    {
        sd_mkdir(config_file_path);
    }
    sd_strcat(config_file_path, "download.cfg", sd_strlen("download.cfg"));
//#endif 
    LOG_DEBUG("malloc_settings_file_path: %s",config_file_path);

    return config_file_path;
}
#endif

_int32 settings_initialize(void)
{
	_int32 result = SUCCESS;
	SETTINGS * _p_settings = &g_settings;
	_int32 _list_size = 0;
	CONFIG_ITEM * _p_cfg_item = NULL;

	LOG_DEBUG("settings_initialize");
	
	if(gp_settings_item_slab==NULL)
	{
		result = mpool_create_slab(sizeof(CONFIG_ITEM), MIN_SETTINGS_ITEM_MEMORY, 0, &gp_settings_item_slab);
		CHECK_VALUE(result);
	}
	else
	{
		return SUCCESS;
	}

       result = sd_init_task_lock(&g_global_settings_lock);	   
	CHECK_VALUE(result);
	
	sd_memset(_p_settings,0,sizeof(SETTINGS));
	
#if( defined(ENABLE_CFG_FILE) || defined(_TEST_RC) )
	_p_settings->_enable_cfg_file=TRUE;
#endif

	/* Get tht settings items from the config file */
	if(_p_settings->_enable_cfg_file==TRUE)
	{
#if defined(MACOS)&&defined(MOBILE_PHONE)
        char* config_file_path = malloc_settings_file_path();
        
        result = settings_config_load(config_file_path,&(_p_settings->_item_list));
        free(config_file_path);
#else
    if( g_system_path[0] != 0 )
    {
        result = settings_config_load(g_system_path, &(_p_settings->_item_list));
    }
    else
    {
        result = settings_config_load(CONFIG_FILE, &(_p_settings->_item_list));
    }
#endif
		if(result!=SUCCESS) goto ErrorHanle;
	}
	else
	{
		list_init(&(_p_settings->_item_list));
	}
	
	return SUCCESS;

ErrorHanle:
	_list_size =  list_size(&(_p_settings->_item_list)); 
	while(_list_size)
	{
		if(list_pop(&(_p_settings->_item_list), (void**)&_p_cfg_item)!=SUCCESS)
		{
			LOG_DEBUG("Fatal Error!");
			CHECK_VALUE(-1);
		}
		mpool_free_slip(gp_settings_item_slab,(void*) _p_cfg_item);
		_list_size--;
	}
	
       sd_uninit_task_lock(&g_global_settings_lock);	   

	if(gp_settings_item_slab!=NULL)
	{
		mpool_destory_slab(gp_settings_item_slab); 
		gp_settings_item_slab=NULL;
	}
	return result;
}

_int32 settings_uninitialize( void)
{
	_int32 _list_size = 0;
	_int32 _result = SUCCESS;
	SETTINGS * _p_settings = &g_settings;
	CONFIG_ITEM * _p_cfg_item = NULL;
	
	/* Save the settings items to the config file */
	_result = settings_config_save();
 	CHECK_VALUE(_result);
	
	_list_size =  list_size(&(_p_settings->_item_list)); 
	while(_list_size)
	{
		_result = list_pop(&(_p_settings->_item_list), (void**)&_p_cfg_item);
 		CHECK_VALUE(_result);
		mpool_free_slip(gp_settings_item_slab,(void*) _p_cfg_item);
		//SAFE_DELETE(_p_cfg_item);
		_list_size--;
	}
	
       sd_uninit_task_lock(&g_global_settings_lock);	   
	   
	if(gp_settings_item_slab!=NULL)
	{
		_result =mpool_destory_slab(gp_settings_item_slab); 
		CHECK_VALUE(_result);
		gp_settings_item_slab=NULL;
	}

	return SUCCESS;
}
/*
_cfg_file_name -> full path name of the config file; e.g. root/downloder/config.cfg
*/
_int32 settings_config_load(char* _cfg_file_name,LIST * _p_settings_item_list)
{
	CONFIG_ITEM * _p_cfg_item = NULL;
	char Rbuffer[MAX_CFG_LINE_LEN],tbuffer[MAX_CFG_LINE_LEN*2];
	char * p_eq = NULL,*p_LF = NULL;
	_u32 read_size = 0, file_id = 0;
	_int32 result = SUCCESS,_len = 0,_dispose=0;

	LOG_DEBUG("settings_config_load, cfg_file_name = %s", _cfg_file_name);
    printf("settings_config_load, cfg_file_name = %s\n", _cfg_file_name);
	
	
	if(sd_strlen(_cfg_file_name) < 1)
	{
		/* File name error */
		CHECK_VALUE(SETTINGS_ERR_INVALID_FILE_NAME);
	}

	if(sd_file_exist(_cfg_file_name) == FALSE)
	{
        LOG_DEBUG("cfg_file : %s is not exist", _cfg_file_name);
	}

	result = sd_open_ex(_cfg_file_name, O_FS_CREATE, &file_id);
	if(result != SUCCESS)
	{
		/* Opening File error */
#ifdef ENABLE_NULL_PATH
		list_init(_p_settings_item_list);
		return SUCCESS;
#else
		CHECK_VALUE(result);
#endif
	}

	list_init(_p_settings_item_list);
	sd_memset(tbuffer,0,MAX_CFG_LINE_LEN*2);
	
	while(TRUE) 
	{
		sd_memset(Rbuffer,0,MAX_CFG_LINE_LEN);
		result = sd_read(file_id, Rbuffer, MAX_CFG_LINE_LEN-1, &read_size);
		if(result != SUCCESS)
		{
			break;
		}
		if(read_size )
		{
			p_eq = NULL;
			p_LF = NULL;
			_dispose=0;
			sd_strcat(tbuffer, Rbuffer,read_size);

			while(TRUE) 
			{
			 	p_eq = NULL;
			 	p_LF = NULL;
				p_LF = sd_strchr(tbuffer+_dispose,'\n', 0);
				if(p_LF!=NULL)
				{
					p_eq = sd_strchr(tbuffer+_dispose,'=', 0) ;
 					if(p_eq!=NULL)
					{
						if(p_eq<p_LF)
						{
							result = mpool_get_slip(gp_settings_item_slab, (void**)&_p_cfg_item);
			  				
		 					if(result!=SUCCESS)
		 					{
		 						sd_close_ex(file_id);	
								CHECK_VALUE(result);
		 						 
		 					}

							sd_memset(_p_cfg_item,0,sizeof(CONFIG_ITEM));
							if(p_eq-tbuffer-_dispose>MAX_CFG_NAME_LEN-1)
							sd_strncpy(_p_cfg_item->_name,tbuffer+_dispose,MAX_CFG_NAME_LEN-1);
							else
							sd_memcpy(_p_cfg_item->_name,tbuffer+_dispose,p_eq-tbuffer-_dispose);
							sd_trim_prefix_lws(_p_cfg_item->_name);
							sd_trim_postfix_lws( _p_cfg_item->_name);
							
							if(p_LF-p_eq-1>MAX_CFG_NAME_LEN-1)
							sd_strncpy(_p_cfg_item->_value,p_eq+1,MAX_CFG_VALUE_LEN-1);
							else
							sd_memcpy(_p_cfg_item->_value,p_eq+1,p_LF-p_eq-1);
							sd_trim_prefix_lws(_p_cfg_item->_value);
							sd_trim_postfix_lws( _p_cfg_item->_value);

							result = list_push(_p_settings_item_list, _p_cfg_item);
							LOG_DEBUG("load cfg, item_key = %s, item_value = %s"
							    , _p_cfg_item->_name
							    , _p_cfg_item->_value);
							if(result != SUCCESS)
		 					{
		 						mpool_free_slip(gp_settings_item_slab,(void*) _p_cfg_item);
		 						sd_close_ex(file_id);	
								CHECK_VALUE(result);
		 						 
		 					}
					
						}
					}
						
					_dispose=p_LF-tbuffer+1;
					if(_dispose>sd_strlen(tbuffer)) 
					{
		 				sd_close_ex(file_id);	 						
		 				CHECK_VALUE(SETTINGS_ERR_INVALID_LINE);   
					}
				}
				else
				{
					_len = sd_strlen(tbuffer+_dispose);
					if(_len != 0)
					{
						if(_len>MAX_CFG_LINE_LEN*2-1)
						sd_strncpy(tbuffer, tbuffer+_dispose, MAX_CFG_LINE_LEN*2-1);
						else
						sd_memmove(tbuffer, tbuffer+_dispose, _len);
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
	sd_close_ex(file_id);	 

	CHECK_VALUE( result);
	
	return SUCCESS;

}

_int32 settings_config_save(void)
{
	_u32 file_id = 0,writesize = 0;
	pLIST_NODE _p_list_node=NULL ;
	CONFIG_ITEM * _p_cfg_item = NULL;
	char line_buffer[MAX_CFG_LINE_LEN];
#if defined(MACOS)&&defined(MOBILE_PHONE)
    char* _cfg_file_name = malloc_settings_file_path();
#else
    char* _cfg_file_name = CONFIG_FILE;
    if(g_system_path[0] != 0)
    {
        _cfg_file_name = g_system_path;
    }
#endif
    
	SETTINGS * _p_settings = &g_settings;
	LIST * _p_settings_item_list = NULL;
	_u32 _list_size = 0;
	_int32 result = 0;
	char  buffer[2048];
	_u32 buffer_pos=0,buffer_len=2048;

	LOG_DEBUG("settings_config_save");
	

	if(_p_settings == NULL) return SETTINGS_ERR_UNKNOWN;

	/* Check if config file is enable */
	if(_p_settings->_enable_cfg_file!=TRUE)
	{
#if defined(MACOS)&&defined(MOBILE_PHONE)
        free(_cfg_file_name);
#endif
		return SUCCESS;
	}
	
	_p_settings_item_list = (LIST *)&( _p_settings->_item_list);
	 _list_size = list_size(_p_settings_item_list); 

	if(_list_size == 0)
	{
	 	CHECK_VALUE( SETTINGS_ERR_LIST_EMPTY);
	}
	
	if(sd_strlen(_cfg_file_name) < 1)
	{
		/* File name error */
		CHECK_VALUE( SETTINGS_ERR_INVALID_FILE_NAME);
	}

	if(sd_file_exist(_cfg_file_name) == TRUE)
	{
		/* Config File is exist,delete it first */
		result = sd_delete_file(_cfg_file_name);
		CHECK_VALUE( result);
	}

	result = sd_open_ex(_cfg_file_name, O_FS_CREATE, &file_id);
	if (result != SUCCESS)
	{
#ifdef ENABLE_NULL_PATH
#if defined(MACOS)&&defined(MOBILE_PHONE)
        free(_cfg_file_name);
#endif
		return SUCCESS;
#else
	CHECK_VALUE( result);
#endif
	}
	
	_p_list_node = LIST_BEGIN(*_p_settings_item_list);
	while(_list_size)
	{
		_p_cfg_item = (CONFIG_ITEM * )LIST_VALUE(_p_list_node);
		
		if(sd_strlen(_p_cfg_item->_name)+sd_strlen(_p_cfg_item->_value)+2>MAX_CFG_LINE_LEN)
			break;
		
		sd_snprintf(line_buffer,MAX_CFG_LINE_LEN,"%s=%s\n",_p_cfg_item->_name,_p_cfg_item->_value);

		result = sd_write_save_to_buffer(file_id, buffer, buffer_len,&buffer_pos, line_buffer, sd_strlen(line_buffer));
		if(result!=SUCCESS)
		{
			sd_close_ex(file_id);	 
			CHECK_VALUE( result);
		}
			
			_p_list_node = LIST_NEXT(_p_list_node);
			_list_size--;
	}
	
	if(buffer_pos!=0)
	{
		result =sd_write(file_id,buffer, buffer_pos, &writesize);
		sd_assert(buffer_pos==writesize);
	}

	sd_close_ex(file_id);
#if defined(MACOS)&&defined(MOBILE_PHONE)
    free(_cfg_file_name);
#endif
	return SUCCESS;
	
}

_int32 setting_cfg_dir(char* system_dir, _int32 system_dir_len)
{
    LOG_DEBUG("setting_cfg_dir, system_dir = %d", system_dir);

    sd_snprintf(g_system_path, MAX_CFG_LINE_LEN + 50, "%s/%s", system_dir, "download.cfg");
    LOG_DEBUG("setting_cfg_dir, g_system_path : %s", g_system_path);
    
    return SUCCESS;
}

char * setting_get_cfg_dir()
{
	return g_system_path;
}



#endif /* __SETTINGS_C_20080731 */
