#if !defined(__P2SP_TASK_C_20090224)
#define __P2SP_TASK_C_20090224
/*--------------------------------------------------------------------------*/
/*                               IDENTIFICATION                             */
/*--------------------------------------------------------------------------*/
/*     Filename  : p2sp_task.c                                         */
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
/* This file contains the procedure of p2sp_task                       */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*                              HISTORY                                     */
/*--------------------------------------------------------------------------*/
/*   Date     |    Author   | Modification                                  */
/*--------------------------------------------------------------------------*/
/* 2009.02.24 | ZengYuqing  | Creation                                      */
/*--------------------------------------------------------------------------*/


#include "p2sp_task.h"
#include "task_manager/task_manager.h"
#include "http_data_pipe/http_resource.h"



#include "res_query/res_query_interface.h"
#include "utility/time.h"
#include "utility/utility.h"
#include "data_manager/data_manager_interface.h"
#include "reporter/reporter_interface.h"
#include "utility/base64.h"
#include "utility/url.h"
#include "utility/sd_iconv.h"
#include "utility/md5.h"
#include "platform/sd_fs.h"
#include "vod_data_manager/vod_data_manager_interface.h"
#include "high_speed_channel/hsc_info.h"

#ifdef UPLOAD_ENABLE
#include "upload_manager/upload_manager.h"
#endif
#include "platform/sd_network.h"
#include "connect_manager/connect_manager_interface.h"

#include "utility/logid.h"
#ifdef LOGID
#undef LOGID
#endif

#define LOGID LOGID_P2SP_TASK

#include "utility/logger.h"

#ifdef VVVV_VOD_DEBUG
extern _u32 gloabal_task_speed = 0 ;

extern _u32 gloabal_vv_speed = 0 ;
#endif

/*--------------------------------------------------------------------------*/
/*                Global declarations				        */
/*--------------------------------------------------------------------------*/

/* Pointer of the task slab */
static SLAB *gp_p2sp_task_slab = NULL;
    


/*--------------------------------------------------------------------------*/
/*                Interfaces provid for external program				        */
/*--------------------------------------------------------------------------*/

_int32 init_p2sp_task_module(void)
{
	_int32 _result = SUCCESS;
	LOG_DEBUG( "init_p2sp_task_module" );
	_result = init_download_task_module();
	CHECK_VALUE(_result);
	
	if(gp_p2sp_task_slab==NULL)
	{
		_result = mpool_create_slab(sizeof(P2SP_TASK), MIN_TASK_MEMORY, 0, &gp_p2sp_task_slab);
		if(_result!=SUCCESS)
		{
			uninit_download_task_module();
			CHECK_VALUE(_result);
		}
	}
	return SUCCESS;
}



_int32 uninit_p2sp_task_module(void)
{
	_int32 _result = SUCCESS;
	LOG_DEBUG( "uninit_p2sp_task_module" );

	uninit_download_task_module();
	
	if(gp_p2sp_task_slab!=NULL)
	{
		_result = mpool_destory_slab(gp_p2sp_task_slab);
		CHECK_VALUE(_result);
		gp_p2sp_task_slab=NULL;
	}
	return SUCCESS;
}

_int32 pt_init_task(P2SP_TASK * p_task)
{
    _int32 ret_val = SUCCESS;
    DS_DATA_INTEREFACE _ds_data_intere;
    LOG_DEBUG( "pt_init_task" );

    // TODO 用download.cfg中的项控制use_one_pipe变量

    ret_val = cm_init_connect_manager(  &(p_task->_task._connect_manager),&(p_task->_data_manager),dm_abandon_resource);
    p_task->_task._connect_manager._main_task_ptr = &p_task->_task;
    CHECK_VALUE(ret_val);


    ret_val = dm_init_data_manager(  &(p_task->_data_manager),(TASK *)p_task);
    CHECK_VALUE(ret_val);

    sd_memset(&_ds_data_intere, 0 , sizeof(DS_DATA_INTEREFACE));

    ret_val = dm_get_dispatcher_data_info(&(p_task->_data_manager), &_ds_data_intere);
    CHECK_VALUE(ret_val);

    ret_val = ds_init_dispatcher(  &(p_task->_task._dispatcher),&_ds_data_intere,&(p_task->_task._connect_manager));
	CHECK_VALUE(ret_val);

	p_task->_task.task_status = TASK_S_IDLE;

    //save the etm task id
    p_task->_task._extern_id = -1;
    p_task->_task._create_time = 0;
    p_task->_task._is_continue = FALSE;

#ifdef EMBED_REPORT
	sd_memset( &p_task->_task._res_query_report_para, 0, sizeof(RES_QUERY_REPORT_PARA) );
#endif	

#if defined(ENABLE_HSC) && !defined(ENABLE_ETM_MEDIA_CENTER)
	hsc_init_info(&p_task->_task._hsc_info);
#endif  /* ENABLE_HSC */
	return SUCCESS;
}
void pt_uninit_task(P2SP_TASK * p_task)
{
	LOG_DEBUG( "pt_uninit_task" );

#ifdef _LOGGER
    out_put_res_recv_records(&p_task->_data_manager._range_record);
#endif	

	cm_uninit_connect_manager(&(p_task->_task._connect_manager));

	ds_unit_dispatcher(&(p_task->_task._dispatcher));

	dm_unit_data_manager(&(p_task->_data_manager) );

	_u32 i = 0;
	for(i = 0; i < p_task->_relation_file_num; i++)
	{	
		sd_assert(NULL != p_task->_relation_fileinfo_arr[i]);
		
		SAFE_DELETE(p_task->_relation_fileinfo_arr[i]->_p_block_info_arr);
		SAFE_DELETE(p_task->_relation_fileinfo_arr[i]);
		
	}
}

void pt_create_task_err_handler(P2SP_TASK * p_task)
{
		cm_uninit_connect_manager(&(p_task->_task._connect_manager));
		ds_unit_dispatcher( &(p_task->_task._dispatcher));
}

_int32 pt_check_if_para_vaild(enum TASK_CREATE_TYPE * p_type,const char * file_path,_u32 file_path_len, const char * file_name,_u32 file_name_len, 
	const char * url,_u32 url_len,const char * ref_url,_u32 ref_url_len,_u8 * cid,_u64 file_size,_u8 * gcid)
{
	_int32 ret_val=SUCCESS;
	
	LOG_DEBUG("pt_check_if_para_vaild:type=%d",*p_type);

	/* Check if the file_path is valid */
	if((file_path==NULL)||file_path_len==0||(sd_strlen(file_path)<file_path_len)||(file_path_len>=MAX_FILE_PATH_LEN)/*||!sd_file_exist(file_path)*/)
	{
		LOG_DEBUG("pt_check_if_para_vaild:ERROR:DT_ERR_INVALID_FILE_PATH");
		return DT_ERR_INVALID_FILE_PATH;
	}

	/* if the file name is vaild ,then check if the very file is exist or the .rf.cfg file is exist */
	if((file_name!=NULL)&&(sd_strlen(file_name)!=0))
	{
		/* Check if the file_name is valid */
		if((sd_strlen(file_name)!=file_name_len)||(file_name_len>=MAX_FILE_NAME_LEN)||!sd_is_file_name_valid(file_name))
		{
			LOG_DEBUG("pt_check_if_para_vaild:ERROR:DT_ERR_INVALID_FILE_NAME");
			return DT_ERR_INVALID_FILE_NAME;
		}

		/* Check if the very file is exist or the .rf.cfg file is exist */
		ret_val=pt_check_if_old_file_exist(file_path,file_path_len,  file_name, file_name_len);
		if(ret_val==0)
		{
			/* The .rf.cfg file is not found! */
			switch(*p_type)
			{
				case TASK_CONTINUE_BY_URL:
					//*p_type=TASK_CREATED_BY_URL;	
					//LOG_DEBUG("pt_check_if_para_vaild:change TASK_CONTINUE_BY_URL to TASK_CREATED_BY_URL!");
					LOG_DEBUG("pt_check_if_para_vaild:ERROR:DT_ERR_CFG_FILE_NOT_EXIST");
					return DT_ERR_CFG_FILE_NOT_EXIST;
					break;
				case TASK_CONTINUE_BY_TCID:
					//*p_type=TASK_CREATED_BY_TCID;					
					//LOG_DEBUG("pt_check_if_para_vaild:change TASK_CONTINUE_BY_TCID to TASK_CREATED_BY_TCID!");
					LOG_DEBUG("pt_check_if_para_vaild:ERROR:DT_ERR_CFG_FILE_NOT_EXIST");
					return DT_ERR_CFG_FILE_NOT_EXIST;
					break;
				case TASK_CREATED_BY_URL:
				case TASK_CREATED_BY_TCID:
				case TASK_CREATED_BY_GCID:
					break;
				default:
					LOG_DEBUG("pt_check_if_para_vaild:ERROR:DT_ERR_INVALID_DOWNLOAD_TASK");
					return DT_ERR_INVALID_DOWNLOAD_TASK; 
					break;
			}
		}
		else
		if(ret_val==1)
		{
			/* The .rf.cfg file is  found! */
			switch(*p_type)
			{
				case TASK_CREATED_BY_URL:
					//*p_type=TASK_CONTINUE_BY_URL;					
					//LOG_DEBUG("pt_check_if_para_vaild:change TASK_CREATED_BY_URL to TASK_CONTINUE_BY_URL!");
					break;
				case TASK_CREATED_BY_TCID:
					break;
				case TASK_CREATED_BY_GCID:
					*p_type=TASK_CONTINUE_BY_TCID;					
					LOG_DEBUG("pt_check_if_para_vaild:change  TASK_CREATED_BY_GCID to TASK_CONTINUE_BY_TCID!");
					break;
				case TASK_CONTINUE_BY_URL:
					if((url==NULL)||(sd_strlen(url)==0)||(sd_strlen(url)!=url_len)||(url_len>=MAX_URL_LEN)||(ref_url_len>=MAX_URL_LEN))
					{
						LOG_DEBUG("pt_check_if_para_vaild:ERROR:DT_ERR_INVALID_URL!");
						return  DT_ERR_INVALID_URL;
					}
				case TASK_CONTINUE_BY_TCID:
					break;
				default:
					LOG_DEBUG("pt_check_if_para_vaild:ERROR:DT_ERR_INVALID_DOWNLOAD_TASK");
					return DT_ERR_INVALID_DOWNLOAD_TASK;
					break;
			}
		}
		else
		{
			/* The file already exist */
			LOG_DEBUG("pt_check_if_para_vaild:ERROR:The file already exist!");
			return ret_val;
		}
	}

	/* Check if other parameters are vaild */
	switch(*p_type)
	{
		case TASK_CREATED_BY_URL:
			if((url==NULL)||(sd_strlen(url)==0)||(sd_strlen(url)!=url_len)||(url_len>=MAX_URL_LEN)||(ref_url_len>=MAX_URL_LEN))
			{
				LOG_DEBUG("pt_check_if_para_vaild:ERROR:DT_ERR_INVALID_URL!");
				return  DT_ERR_INVALID_URL;
			}
			
			break;
			
			
		case TASK_CREATED_BY_TCID:
		case TASK_CREATED_BY_GCID:
			if(FALSE==sd_is_cid_valid(cid))
			{
				LOG_DEBUG("pt_check_if_para_vaild:ERROR:DT_ERR_INVALID_TCID!");
				return DT_ERR_INVALID_TCID;
			}
			
		case TASK_CONTINUE_BY_URL:
		case TASK_CONTINUE_BY_TCID:
			if((file_name==NULL)||(sd_strlen(file_name)==0))
			{
				LOG_DEBUG("pt_check_if_para_vaild:ERROR:DT_ERR_INVALID_FILE_NAME!");
				return DT_ERR_INVALID_FILE_NAME;
			}
			break;
			
			
		default:
			LOG_DEBUG("pt_check_if_para_vaild:ERROR:DT_ERR_INVALID_DOWNLOAD_TASK!");
			return DT_ERR_INVALID_DOWNLOAD_TASK;
			break;
	}
	
	LOG_DEBUG("pt_check_if_para_vaild:all parameters are ok!");	
	return SUCCESS;
}

_int32 pt_check_if_old_file_exist(const char * file_path,_u32 file_path_len, const char * file_name,_u32 file_name_len)
{
	char cfg_file_path[MAX_FULL_PATH_BUFFER_LEN];
	char td_file_path[MAX_FULL_PATH_BUFFER_LEN];
	char* cfg_pos_str = ".rf.cfg";
	_u32 file_path_len_tmp= file_path_len;
	BOOL have_cfg = FALSE;
	BOOL have_td = FALSE;
	
	LOG_DEBUG("pt_check_if_old_file_exist");
       /*create full file path*/
	sd_memset(cfg_file_path,0,MAX_FULL_PATH_BUFFER_LEN);
	sd_strncpy(cfg_file_path,file_path,file_path_len);
	sd_memset(td_file_path, 0, MAX_FULL_PATH_BUFFER_LEN);

	/* Make sure the file path is end with a '/'  */
	if(cfg_file_path[file_path_len_tmp-1]!='/')
	{
		if(file_path_len_tmp+1==MAX_FILE_PATH_LEN)
		{
			LOG_DEBUG("pt_check_if_old_file_exist:DT_ERR_INVALID_FILE_PATH");
			return DT_ERR_INVALID_FILE_PATH;
		}
		else
		{
			cfg_file_path[file_path_len_tmp]='/';
			file_path_len_tmp++;
			cfg_file_path[file_path_len_tmp]='\0';
		}
	}
	
	sd_strncpy(cfg_file_path +file_path_len_tmp ,file_name,MAX_FULL_PATH_BUFFER_LEN-file_path_len_tmp);

	/* Check if the file already exist */
	if(TRUE==sd_file_exist(cfg_file_path))
	{
		LOG_DEBUG("pt_check_if_old_file_exist:DT_ERR_FILE_EXIST:%s",cfg_file_path);
		return DT_ERR_FILE_EXIST;
	}
	
       /*create fullfilename.rf.cfg */
       sd_strncpy(cfg_file_path +file_path_len_tmp+file_name_len ,cfg_pos_str,7);

	sd_strncpy(td_file_path, cfg_file_path, sd_strlen(cfg_file_path)-4);
	  
	/* Check if the .rf.cfg file and .td file already exist */
	have_cfg = sd_file_exist(cfg_file_path);
	have_td = sd_file_exist(td_file_path);
	if(have_cfg && have_td)
	{
		/*  .rf.cfg file and .td file both exist*/
		return 1;
	}
	else
	{
		if(have_cfg)
		{
			LOG_ERROR("pt_check_if_old_file_exist:the rf file is not exist, the .rf.cfg file need deleted!:%s",cfg_file_path);		
			sd_delete_file(cfg_file_path);
		}
		if(have_td)
		{
			LOG_ERROR("pt_check_if_old_file_exist:the td file is exist,need deleted!:%s",cfg_file_path);
			sd_delete_file(td_file_path);
		}		
	}
	   
	/* No file */
	LOG_DEBUG("pt_check_if_old_file_exist:No file!");
	return 0;
}

_int32 pt_create_new_task_by_url( TM_NEW_TASK_URL* p_param,TASK ** pp_task )
{
	_int32 ret_val = SUCCESS;
	enum TASK_CREATE_TYPE type = TASK_CREATED_BY_URL;

	LOG_DEBUG("pt_create_new_task_by_url");
	

	ret_val=pt_check_if_para_vaild(&type,p_param->file_path,p_param->file_path_len, p_param->file_name,p_param->file_name_length, 
	p_param->url,p_param->url_length,p_param->ref_url, p_param->ref_url_length,NULL,0,NULL);
	if(ret_val!=SUCCESS) return ret_val;

	ret_val=pt_create_task(pp_task,type,p_param->file_path,p_param->file_path_len, p_param->file_name,p_param->file_name_length, 
	p_param->url,p_param->url_length,p_param->ref_url, p_param->ref_url_length,p_param->description,p_param->description_len,NULL,0,NULL, p_param->origion_url, p_param->origion_url_length);
	if(ret_val!=SUCCESS) return ret_val;
	
	return SUCCESS;
}

_int32 pt_create_continue_task_by_url( TM_CON_TASK_URL* p_param,TASK ** pp_task )
{
	_int32 ret_val = SUCCESS;
	enum TASK_CREATE_TYPE type = TASK_CONTINUE_BY_URL;

	LOG_DEBUG("pt_create_continue_task_by_url");
	

	ret_val=pt_check_if_para_vaild(&type,p_param->file_path,p_param->file_path_len, p_param->file_name,p_param->file_name_length, 
	p_param->url,p_param->url_length,p_param->ref_url, p_param->ref_url_length,NULL,0,NULL);
	if(ret_val!=SUCCESS) return ret_val;

	ret_val=pt_create_task(pp_task,type,p_param->file_path,p_param->file_path_len, p_param->file_name,p_param->file_name_length, 
	p_param->url,p_param->url_length,p_param->ref_url, p_param->ref_url_length,p_param->description,p_param->description_len,NULL,0,NULL
	,p_param->origion_url, p_param->origion_url_length);
	if(ret_val!=SUCCESS) return ret_val;
	
	return SUCCESS;


}


	//新建任务-用tcid下
_int32 pt_create_new_task_by_tcid( TM_NEW_TASK_CID* p_param,TASK ** pp_task )
{
	_int32 ret_val = SUCCESS;
	enum TASK_CREATE_TYPE type = TASK_CREATED_BY_TCID;

	LOG_DEBUG("pt_create_new_task_by_tcid");
	

	ret_val=pt_check_if_para_vaild(&type,p_param->file_path,p_param->file_path_len, p_param->file_name,p_param->file_name_length, 
	NULL,0,NULL, 0,p_param->tcid,p_param->file_size,NULL);
	if(ret_val!=SUCCESS) return ret_val;

	ret_val=pt_create_task(pp_task,type,p_param->file_path,p_param->file_path_len, p_param->file_name,p_param->file_name_length, 
	NULL,0,NULL, 0,NULL, 0,p_param->tcid,p_param->file_size,NULL, "", 0);
	if(ret_val!=SUCCESS) return ret_val;
	
	return SUCCESS;


}


	//断点续传-用tcid下
_int32 pt_create_continue_task_by_tcid(TM_CON_TASK_CID* p_param, TASK **pp_task )
{
	_int32 ret_val = SUCCESS;
	enum TASK_CREATE_TYPE type = TASK_CONTINUE_BY_TCID;

	LOG_DEBUG("pt_create_continue_task_by_tcid");
	

	ret_val=pt_check_if_para_vaild(&type,p_param->file_path,p_param->file_path_len, p_param->file_name,p_param->file_name_length, 
	NULL,0,NULL, 0,p_param->tcid,0,NULL);
	if(ret_val!=SUCCESS) return ret_val;

	ret_val=pt_create_task(pp_task,type,p_param->file_path,p_param->file_path_len, p_param->file_name,p_param->file_name_length, 
	NULL,0,NULL, 0,NULL, 0,p_param->tcid,0,NULL, "", 0);
	if(ret_val!=SUCCESS) return ret_val;
	
	return SUCCESS;


}
_int32 pt_create_task_by_tcid_file_size_gcid( TM_NEW_TASK_GCID* p_param,TASK ** pp_task )
{
	_int32 ret_val = SUCCESS;
	enum TASK_CREATE_TYPE type = TASK_CREATED_BY_GCID;

	LOG_DEBUG("pt_create_task_by_tcid_file_size_gcid");
	

	ret_val=pt_check_if_para_vaild(&type,p_param->file_path,p_param->file_path_len, p_param->file_name,p_param->file_name_length, 
	NULL,0,NULL, 0,p_param->tcid,p_param->file_size,p_param->gcid);
	if(ret_val!=SUCCESS) return ret_val;

	ret_val=pt_create_task(pp_task,type,p_param->file_path,p_param->file_path_len, p_param->file_name,p_param->file_name_length, 
	NULL,0,NULL, 0,NULL, 0,p_param->tcid,p_param->file_size,p_param->gcid, "", 0);
	if(ret_val!=SUCCESS) return ret_val;

#ifdef ENABLE_VOD
       dm_set_vod_mode(&((P2SP_TASK*)*pp_task)->_data_manager, TRUE);
       pt_set_cdn_mode( *pp_task,TRUE);		 
#endif

	return SUCCESS;
}

_int32 pt_create_task(TASK ** pp_task,enum TASK_CREATE_TYPE  type,const char * file_path,_u32 file_path_len, const char * file_name,_u32 file_name_len, 
	const char * url,_u32 url_len,const char * ref_url,_u32 ref_url_len,const char * description,_u32 description_len,_u8 * cid,_u64 file_size,_u8 * gcid, const char* origion_url, _u32 origion_url_length)
{
	char file_path_tmp[MAX_FILE_PATH_LEN]={0};
	char url_buffer[MAX_URL_LEN]={0};
	char url_fix[MAX_URL_LEN]={0};
	char *fixed_url;
	char file_name_tmp[MAX_FILE_NAME_BUFFER_LEN]={0};
	_u8 gcid_tmp[CID_SIZE]={0};
	_u8 cid_tmp[CID_SIZE]={0};
	_u8 gcid_tmp2[CID_SIZE]={0};
	_u8 cid_tmp2[CID_SIZE]={0};
	_u8 *p_cid=cid;
	_u8 *p_gcid=gcid;
	_u32 file_path_len_tmp=file_path_len;
	_u32 url_length=0;
	_u32 file_name_len_tmp=file_name_len;
	_u64 file_size_tmp=file_size;
	_int32 ret_val=SUCCESS;
	P2SP_TASK * p_task = NULL;
	BOOL is_data_manager_inited = FALSE;
	char * p_file_name=(char *)file_name;
	RESOURCE *p_orgin_res=NULL;
	char cfg_file_path[MAX_FULL_PATH_BUFFER_LEN]={0};
	char* cfg_pos_str = ".rf.cfg";
       char* origin_url = NULL;
	char* ref_origin_url = NULL;   
	BOOL b_ret = FALSE,is_vod_task=FALSE;
	BOOL is_cdn_mode = FALSE;
	enum TASK_CREATE_TYPE  create_type=type;
	BOOL is_lan = FALSE;				/* IPAD KANKAN 下局域网任务的特殊处理 */
	BOOL is_unicom_url = FALSE;		/* 联通url特殊处理 */
	
	LOG_DEBUG("pt_create_task:create_type=%d, file_name=%s",create_type, (file_name!=NULL?file_name:""));
	
	sd_strncpy(file_path_tmp, file_path, file_path_len);
	
	/* Make sure the file path is end with a '/'  */
	if(file_path_tmp[file_path_len_tmp-1]!='/')
	{
		if(file_path_len_tmp+1==MAX_FILE_PATH_LEN)
		{
			LOG_DEBUG("pt_create_task:ERROR:DT_ERR_INVALID_FILE_PATH");
			return DT_ERR_INVALID_FILE_PATH;
		}
		else
		{
			file_path_tmp[file_path_len_tmp]='/';
			file_path_len_tmp++;
			file_path_tmp[file_path_len_tmp]='\0';
		}
	}

	/* Precheck ... */
	switch(create_type)
	{
		case TASK_CREATED_BY_URL:
		case TASK_CONTINUE_BY_URL:
			sd_memset(url_buffer,0,MAX_URL_LEN);

			 /* Check if the URL is thunder special URL */
			if(sd_strstr((char*) url, "thunder://", 0 )== url)
			{
				/* Decode to normal URL */
				fixed_url = (char*)url;
				if ('/' == url[sd_strlen(url)-1])
				{
					sd_strncpy(url_fix, url, MAX_URL_LEN);
					url_fix[sd_strlen(url)-1] = '\0';
					fixed_url = url_fix;
				}
				if( sd_base64_decode(fixed_url+10,(_u8 *)url_buffer,NULL)!=0)
				{
					LOG_DEBUG("pt_create_task:ERROR:DT_ERR_INVALID_URL");
					return  DT_ERR_INVALID_URL;
				}

				url_buffer[sd_strlen(url_buffer)-2]='\0';
				sd_strncpy(url_buffer,url_buffer+2,MAX_URL_LEN);
				url_length = sd_strlen(url_buffer);
			}
			else
			{
				sd_strncpy(url_buffer,url,MAX_URL_LEN);
				url_length = url_len;
			}

			/* Check if the url is xunlei kankan url  */
			sd_memset(gcid_tmp,0,CID_SIZE);
			sd_memset(cid_tmp,0,CID_SIZE);
			sd_memset(file_name_tmp,0,MAX_FILE_NAME_LEN);
			if(sd_parse_kankan_vod_url(url_buffer,url_length ,gcid_tmp,cid_tmp, &file_size_tmp,file_name_tmp)==SUCCESS)
			{
				is_vod_task=TRUE;
				
				if(create_type==TASK_CREATED_BY_URL)
				{
					create_type=TASK_CREATED_BY_GCID;
					p_cid = cid_tmp;
					p_gcid = gcid_tmp;
					if((p_file_name==NULL)||(sd_strlen(p_file_name)==0))
					{
						p_file_name=file_name_tmp;
					}
					LOG_DEBUG("pt_create_task:change TASK_CREATED_BY_URL to TASK_CREATED_BY_GCID");
					goto CREATE_NEW_VOD_TASK;
				}
				else
				{
					p_cid = cid_tmp;
					p_gcid = gcid_tmp;
					if((p_file_name==NULL)||(sd_strlen(p_file_name)==0))
					{
						LOG_DEBUG("pt_create_task:ERROR:DT_ERR_INVALID_FILE_NAME");
						return DT_ERR_INVALID_FILE_NAME;
					}
					
					create_type=TASK_CONTINUE_BY_TCID;
					LOG_DEBUG("pt_create_task:change TASK_CONTINUE_BY_URL to TASK_CONTINUE_BY_TCID");
					goto CREATE_CON_VOD_TASK;
				}
			}

			/*
			sd_memset(gcid_tmp,0,CID_SIZE);
			sd_memset(cid_tmp,0,CID_SIZE);
			sd_memset(file_name_tmp,0,MAX_FILE_NAME_LEN);
			if(sd_parse_lixian_url(url_buffer,url_length ,gcid_tmp,cid_tmp, &file_size_tmp,file_name_tmp)==SUCCESS)
			{
				is_vod_task=TRUE;
				is_cdn_mode=TRUE;
				if(create_type==TASK_CREATED_BY_URL)
				{
					create_type=TASK_CREATED_BY_GCID;
					p_cid = cid_tmp;
					p_gcid = gcid_tmp;
					if((p_file_name==NULL)||(sd_strlen(p_file_name)==0))
					{
						p_file_name=file_name_tmp;
					}
					LOG_DEBUG("pt_create_task:change TASK_CREATED_BY_URL to TASK_CREATED_BY_GCID");
					goto CREATE_NEW_VOD_TASK;
				}
				else
				{
					p_cid = cid_tmp;
					p_gcid = gcid_tmp;
					if((p_file_name==NULL)||(sd_strlen(p_file_name)==0))
					{
						LOG_DEBUG("pt_create_task:ERROR:DT_ERR_INVALID_FILE_NAME");
						return DT_ERR_INVALID_FILE_NAME;
					}

					create_type=TASK_CONTINUE_BY_TCID;
					LOG_DEBUG("pt_create_task:change TASK_CONTINUE_BY_URL to TASK_CONTINUE_BY_TCID");
					goto CREATE_CON_VOD_TASK;
				}
			}
			*/

           if (tm_is_task_exist_by_url(url_buffer, FALSE) == TRUE)
	       {
	       	    LOG_DEBUG("pt_create_task:1:ERROR:DT_ERR_ORIGIN_RESOURCE_EXIST");
	            return DT_ERR_ORIGIN_RESOURCE_EXIST;
	       }

			if(sd_check_if_in_nat_url(url_buffer))
			{
				 is_lan = TRUE;	
			}
			
			// 检查是否联通url
			if( NULL != sd_strstr(url_buffer, "thunder_url", 0) )
			{
				LOG_DEBUG("pt_create_task: 3g_unicom task");
				 is_unicom_url = TRUE;	
			}   
			break;
			
		case TASK_CREATED_BY_GCID:
			
			is_vod_task=TRUE;
			
CREATE_NEW_VOD_TASK:
			
            if ((TRUE==sd_is_cid_valid(p_gcid))&&(tm_is_task_exist(p_gcid, FALSE) == TRUE))
			{
				LOG_DEBUG("pt_create_task:2:ERROR:DT_ERR_ORIGIN_RESOURCE_EXIST");
				return DT_ERR_ORIGIN_RESOURCE_EXIST;
			}
			
		case TASK_CREATED_BY_TCID:
		case TASK_CONTINUE_BY_TCID:
CREATE_CON_VOD_TASK:
			/* is_vod_task此标志用于判断是否查询tracker_cdn: 查询完shub获取到gcid后查询pt_start_query_phub_tracker_cdn--tracker_cdn(看看资源)，
			下载xv格式必须查tracker_cdn。(导入未下载完成xv任务下载不动原因) */
			if(TASK_CONTINUE_BY_TCID == create_type && ( (file_name != NULL) && (NULL !=sd_strstr(file_name, ".xv", 0))) )
			{
				LOG_DEBUG("pt_create_task: type = TASK_CONTINUE_BY_TCID, file_name = %s", file_name);
				is_vod_task = TRUE;
			}
            if (tm_is_task_exist_by_cid(p_cid, FALSE) == TRUE)
		    {
	       	    LOG_DEBUG("pt_create_task:3:ERROR:DT_ERR_ORIGIN_RESOURCE_EXIST");
	            return  DT_ERR_ORIGIN_RESOURCE_EXIST;
		    }
			break;
			
		default:
			LOG_DEBUG("pt_create_task:ERROR:DT_ERR_INVALID_DOWNLOAD_TASK");
			return DT_ERR_INVALID_DOWNLOAD_TASK;
			break;
	}

	/* OK, get the task memory */
	ret_val = mpool_get_slip(gp_p2sp_task_slab, (void**)&p_task);
	if(ret_val !=SUCCESS) return ret_val;

	sd_memset(p_task,0,sizeof(P2SP_TASK));

	/* Initiate the task parameters */
	ret_val = pt_init_task( p_task);
	if(ret_val !=SUCCESS) goto ErrorHanle;
	
    sd_memset(p_task->_task._eigenvalue, 0, CID_SIZE);
    ctx_md5 p_ctx;
    md5_initialize(&p_ctx);
    md5_update(&p_ctx, (const _u8*)url, url_len);
    md5_finish(&p_ctx, p_task->_task._eigenvalue);

	is_data_manager_inited = TRUE;

    p_task->_task_created_type = create_type;

        if (origion_url )
        {
           sd_strncpy(p_task->_origion_url, origion_url, MAX_URL_LEN);
           p_task->_origion_url_length = MIN(MAX_URL_LEN - 1, origion_url_length);
        }
        else
        {
            p_task->_origion_url[0] = '\0';
            p_task->_origion_url_length = 0;
        }
	/* Initiate the task resources */
	switch(create_type)
	{
		case TASK_CREATED_BY_URL:
			ret_val = dm_set_file_info(  &(p_task->_data_manager),p_file_name, file_path_tmp, url_buffer, (char*)ref_url);
			if(ret_val !=SUCCESS) goto ErrorHanle;

			ret_val = cm_add_origin_server_resource( &(p_task->_task._connect_manager),INVALID_FILE_INDEX,url_buffer,  url_length,(char*) ref_url, ref_url_len, &p_orgin_res );
			if(ret_val !=SUCCESS) goto ErrorHanle;

 #ifdef ENABLE_COOKIES
            enum SCHEMA_TYPE url_type = sd_get_url_type(url_buffer, sd_strlen(url_buffer));
            if (HTTP == url_type || HTTPS == url_type)
            {
                if(description&&description_len!=0)
                {
                    if(sd_strncmp(description,"Cookie:", 7)==0 && p_orgin_res->_resource_type == HTTP_RESOURCE)
                    {
                        http_resource_set_cookies((HTTP_SERVER_RESOURCE * )p_orgin_res,description,description_len);
                    }
                }
            }
 			
 #endif /* ENABLE_COOKIES */
			ret_val = dm_set_origin_resource(  &(p_task->_data_manager),p_orgin_res );
			if(ret_val !=SUCCESS) goto ErrorHanle;

			// 设置userdata到cfg文件保存
			//LOG_DEBUG("pt_create_task:change TASK_CREATED_BY_URL : userdata_len = %d", description_len);
			//dm_set_user_data(&(p_task->_data_manager), (const _u8*)description, description_len);
			
			break;
			
		case TASK_CONTINUE_BY_URL:
			ret_val = dm_set_file_info(  &(p_task->_data_manager),p_file_name, file_path_tmp, url_buffer, (char*)ref_url);
			if(ret_val !=SUCCESS) goto ErrorHanle;

		       /*create full file path*/
			sd_memset(cfg_file_path,0,MAX_FULL_PATH_BUFFER_LEN);
			sd_strncpy(cfg_file_path,file_path_tmp,MAX_FULL_PATH_BUFFER_LEN);

			sd_strncpy(cfg_file_path +file_path_len_tmp ,p_file_name,MAX_FULL_PATH_BUFFER_LEN-file_path_len_tmp);

		       /*create fullfilename.rf.cfg */
		       sd_strncpy(cfg_file_path +file_path_len_tmp+file_name_len_tmp ,cfg_pos_str,7);

			ret_val = dm_open_old_file(&(p_task->_data_manager), cfg_file_path);
			if(ret_val !=SUCCESS) goto ErrorHanle;

            p_task->_task._is_continue = TRUE;

			if ((dm_get_cid( &(p_task->_data_manager),  cid_tmp)==TRUE)&&
				(tm_is_task_exist_by_cid(cid_tmp, FALSE) == TRUE))
		       {
		            ret_val =  DT_ERR_ORIGIN_RESOURCE_EXIST;
			     goto ErrorHanle; 
		       }

			if((dm_get_shub_gcid( &(p_task->_data_manager),  gcid_tmp)==TRUE)&&
				(tm_is_task_exist(gcid_tmp, FALSE) == TRUE))
		       {
		            ret_val =  DT_ERR_ORIGIN_RESOURCE_EXIST;
			     goto ErrorHanle; 
		       }

			b_ret = dm_get_origin_url(&(p_task->_data_manager), &origin_url);
			if((b_ret == TRUE) &&(sd_strcmp(origin_url, DEFAULT_LOCAL_URL) != 0))  
			{
				   
				if(is_lan && (sd_strcmp(origin_url, url) != 0))
				{
					/* 替换旧的原始URL */
                    if (tm_is_task_exist_by_url((char*)url, FALSE) == TRUE)
			        {
			            ret_val =  DT_ERR_ORIGIN_RESOURCE_EXIST;
				        goto ErrorHanle; 
			        }
					   
					ret_val = dm_set_origin_url_info( &(p_task->_data_manager),(char*)url, (char*)ref_url );
					CHECK_VALUE(ret_val);

					ret_val = cm_add_origin_server_resource( &(p_task->_task._connect_manager),INVALID_FILE_INDEX,  (char*)url,   sd_strlen(url),(char*)ref_url,  sd_strlen(ref_url), &p_orgin_res );
					CHECK_VALUE(ret_val);
				}
				else
				{
                    if (tm_is_task_exist_by_url(origin_url, FALSE) == TRUE)
			        {
			            ret_val =  DT_ERR_ORIGIN_RESOURCE_EXIST;
				        goto ErrorHanle; 
			        }
				dm_get_origin_ref_url(&(p_task->_data_manager), &ref_origin_url);

				ret_val = cm_add_origin_server_resource( &(p_task->_task._connect_manager),INVALID_FILE_INDEX,origin_url,  sd_strlen(origin_url), ref_origin_url, sd_strlen(ref_origin_url), &p_orgin_res );
				if(ret_val !=SUCCESS) goto ErrorHanle;
				}
 #ifdef ENABLE_COOKIES
                enum SCHEMA_TYPE url_type = sd_get_url_type(origin_url, sd_strlen(origin_url));
                if (HTTP == url_type || HTTPS == url_type)
                {
                    if(description&&description_len!=0)
                    {
                        if(sd_strncmp(description,"Cookie:", 7)==0)
                        {
                            http_resource_set_cookies((HTTP_SERVER_RESOURCE * )p_orgin_res,description,description_len);
                        }
                    }
                }
	 			
 #endif /* ENABLE_COOKIES */
 
				ret_val = dm_set_origin_resource(  &(p_task->_data_manager),p_orgin_res );
				if(ret_val !=SUCCESS) goto ErrorHanle;
			}
			else
			{
				if((dm_get_cid( &(p_task->_data_manager),  cid_tmp)==TRUE)&&
					(sd_is_cid_valid(cid_tmp)))
				{
					p_task->_is_cid_ok = TRUE;
					p_task->_task_created_type = TASK_CONTINUE_BY_TCID;
					LOG_DEBUG("pt_create_task:change TASK_CONTINUE_BY_URL to TASK_CONTINUE_BY_TCID");
				}
				else
				{
					ret_val = DT_ERR_GETTING_ORIGIN_URL;
					goto ErrorHanle; 
				}
			}

			break;
			
		case TASK_CREATED_BY_TCID:
		case TASK_CREATED_BY_GCID:
			
			p_task->_task_created_type = TASK_CREATED_BY_TCID;
			
            ret_val = dm_set_file_info(  &(p_task->_data_manager),p_file_name,file_path_tmp, 
                ((type == TASK_CREATED_BY_URL) ? (char *)url: DEFAULT_LOCAL_URL), NULL);
			if (ret_val !=SUCCESS) goto ErrorHanle;

			ret_val = dm_set_cid( &(p_task->_data_manager), p_cid);
			if(ret_val !=SUCCESS) goto ErrorHanle;
			
			p_task->_is_cid_ok = TRUE;

			if(TRUE==sd_is_cid_valid(p_gcid))
			{
				ret_val = dm_set_gcid( &(p_task->_data_manager), p_gcid);
				if(ret_val!=SUCCESS) goto ErrorHanle;
				p_task->_is_gcid_ok = TRUE;
			}
	
			if(file_size_tmp!=0)
			{
				ret_val = dm_set_file_size( &(p_task->_data_manager), file_size_tmp, FALSE);
				if(ret_val !=SUCCESS) goto ErrorHanle;
			}

            dm_set_user_data(&(p_task->_data_manager), (const _u8*)description, description_len);
			break;
			
		case TASK_CONTINUE_BY_TCID:
            ret_val = dm_set_file_info(  &(p_task->_data_manager),p_file_name, file_path_tmp, 
                ((type == TASK_CONTINUE_BY_URL) ? (char *)url: DEFAULT_LOCAL_URL), NULL);
			if (ret_val !=SUCCESS) goto ErrorHanle;

		       /*create full file path*/
			sd_memset(cfg_file_path,0,MAX_FULL_PATH_BUFFER_LEN);
			sd_strncpy(cfg_file_path,file_path_tmp,MAX_FULL_PATH_BUFFER_LEN);

			sd_strncpy(cfg_file_path +file_path_len_tmp ,p_file_name,MAX_FULL_PATH_BUFFER_LEN-file_path_len_tmp);

		       /*create fullfilename.rf.cfg */
		       sd_strncpy(cfg_file_path +file_path_len_tmp+file_name_len_tmp ,cfg_pos_str,7);

			ret_val = dm_open_old_file(&(p_task->_data_manager),cfg_file_path);
			if(ret_val !=SUCCESS) goto ErrorHanle;
            p_task->_task._is_continue = TRUE;

			sd_memset(cid_tmp2,0,CID_SIZE);
			if((dm_get_cid( &(p_task->_data_manager),  cid_tmp2)==TRUE)
				&&(sd_is_cid_valid(cid_tmp2)))
			{
                if (tm_is_task_exist_by_cid(cid_tmp2, FALSE) == TRUE)
			    {
	                 ret_val =  DT_ERR_ORIGIN_RESOURCE_EXIST;
		             goto ErrorHanle; 
			    }

				if(FALSE==sd_is_cid_equal(cid_tmp2,p_cid))
				{
					ret_val =DT_ERR_GETTING_CID;
					goto ErrorHanle;
				}
					
				p_task->_is_cid_ok = TRUE;
			}
			else
			{
				ret_val =DT_ERR_GETTING_CID;
				goto ErrorHanle;
			}

			if(dm_get_shub_gcid( &(p_task->_data_manager),  gcid_tmp2)==TRUE)
		       {
                   if (tm_is_task_exist(gcid_tmp2, FALSE) == TRUE)
		       	   {
			           ret_val =  DT_ERR_ORIGIN_RESOURCE_EXIST;
				       goto ErrorHanle; 
		       	   }
				p_task->_is_gcid_ok = TRUE;
		       }
			else if (TRUE==sd_is_cid_valid(p_gcid))
			{
				ret_val = dm_set_gcid( &(p_task->_data_manager), p_gcid);
				if(ret_val!=SUCCESS) goto ErrorHanle;
				p_task->_is_gcid_ok = TRUE;
			}
            dm_set_user_data(&(p_task->_data_manager), (const _u8*)description, description_len);
			break;
		default:
			ret_val =  DT_ERR_INVALID_DOWNLOAD_TASK;
			goto ErrorHanle; 
			break;
	}

    init_dphub_query_context(&p_task->_dpub_query_context);

	p_task->_is_vod_task = is_vod_task;
	p_task->_task._task_type=P2SP_TASK_TYPE;
	if(is_lan)
	{
	#ifdef KANKAN_PROJ
		p_task->_is_lan = TRUE;
	#endif /* KANKAN_PROJ */
	}

	/* Always set to lan task cause Runmit file do not neet to query xunlei server !   Modified by ZYQ @ 20141103  */
	p_task->_is_lan = TRUE;
	
	if(is_unicom_url)
		p_task->_is_unicom = TRUE;

	*pp_task =(TASK*)p_task;
	LOG_DEBUG("pt_create_task:SUCCESS:_task_created_type=%d",p_task->_task_created_type);
	return SUCCESS;
	
ErrorHanle:

	LOG_DEBUG("pt_create_task:ERROR:ret_val=%d,is_data_manager_inited=%d",ret_val,is_data_manager_inited);
	pt_create_task_err_handler(p_task);
	if(is_data_manager_inited == TRUE)
	{
		dm_unit_data_manager(&(p_task->_data_manager));
		/* Return but do not release the _p_download_task until the dt_notify_file_closing_result_event is called */
#ifndef IPAD_KANKAN
		*pp_task =(TASK*)p_task;
#endif /* IPAD_KANKAN */
	}
	else
	{
		mpool_free_slip(gp_p2sp_task_slab,(void*) p_task);
	}

	return ret_val;

}

_int32 pt_start_task(TASK * p_task)
{
	P2SP_TASK * p_p2sp_task = (P2SP_TASK *)p_task;
	_int32 ret_val = SUCCESS;
	_u64 filesize = dm_get_file_size(& (p_p2sp_task->_data_manager));
	_u64 downloaded_size = dm_get_download_data_size(&(p_p2sp_task->_data_manager));
    BOOL current_mode = FALSE;
	LOG_DEBUG("pt_start_task");
	

	if(p_task->task_status != TASK_S_IDLE)
	{
		return DT_ERR_TASK_IS_NOT_READY;
	}
	
	ret_val = sd_time(&(p_task->_start_time));
	CHECK_VALUE(ret_val);
	ret_val = sd_time(&p_p2sp_task->_nospeed_timestmp);
	CHECK_VALUE(ret_val);
	
	/*  kankan 中已经下载完的文件不再向服务器查询资源 */
	if(filesize==0||filesize!=downloaded_size)		
	{
		//p_task->_dispatcher._dispatch_mode = PLAY;
		ret_val = ds_start_dispatcher(&(p_task->_dispatcher));
		CHECK_VALUE(ret_val);
		
        current_mode = cm_get_origin_mode( &(p_task->_connect_manager) );
        LOG_DEBUG("pt_start_task, current_mode = %d", current_mode);
        if(current_mode == FALSE )
        {
		    ret_val = pt_start_query( (TASK *)p_p2sp_task);
		    if(ret_val!=SUCCESS) goto ErrorHanle;
        }
	}
	p_task->task_status = TASK_S_RUNNING;

ErrorHanle:
	
	if(ret_val ==SUCCESS)
	{
		LOG_DEBUG("The task is started!_task_id=%u,_start_time=%u",p_p2sp_task->_task._task_id,p_p2sp_task->_task._start_time);
	}
	else
	{
		ds_stop_dispatcher(&(p_task->_dispatcher));
	}

		return ret_val;

}
_int32 pt_stop_task(TASK * p_task)
{
	P2SP_TASK * p_p2sp_task = (P2SP_TASK *)p_task;
	//_int32 ret_val = SUCCESS;
	_u32 cur_time = 0;
#ifdef EMBED_REPORT
	VOD_REPORT_PARA vod_para;
#endif

#ifndef ENABLE_CDN
	LOG_DEBUG("pt_stop_task:task_status=%d,_res_query_shub_state=%d,_res_query_phub_state=%d,_res_query_tracker_state=%d,partner_cdn_state=%d"
		,p_task->task_status,p_p2sp_task->_res_query_shub_state,p_p2sp_task->_res_query_phub_state,p_p2sp_task->_res_query_tracker_state,p_p2sp_task->_res_query_partner_cdn_state);
#else
	LOG_DEBUG("pt_stop_task:task_status=%d,_res_query_shub_state=%d,_res_query_phub_state=%d,_res_query_tracker_state=%d,_res_query_cdn_state=%d,partner_cdn_state=%d"
		,p_task->task_status,p_p2sp_task->_res_query_shub_state,p_p2sp_task->_res_query_phub_state,p_p2sp_task->_res_query_tracker_state,p_p2sp_task->_res_query_cdn_state,p_p2sp_task->_res_query_partner_cdn_state);
#endif

	if(p_task->task_status == TASK_S_STOPPED)
	{
		CHECK_VALUE(DT_ERR_TASK_NOT_RUNNING);
	}

	sd_time(&cur_time);
#ifdef EMBED_REPORT
	if( !dm_is_vod_mode(&(p_p2sp_task->_data_manager))&&(TIME_SUBZ(cur_time,p_task->_stop_vod_time)>10) )
	{
		sd_memset( &vod_para, 0, sizeof( VOD_REPORT_PARA ) );
		emb_reporter_common_stop_task(p_task,&vod_para );
	}

    reporter_task_p2sp_stat(p_task);

#endif
	
	pt_stop_query((TASK *)p_p2sp_task);
    ds_stop_dispatcher(&(p_task->_dispatcher));
    
	/* report the task information to shub */
	if( p_task->task_status==TASK_S_SUCCESS || p_task->task_status==TASK_S_VOD )
	{
		LOG_DEBUG("pt_stop_task is_shub_ok:%d,_get_file_size_after_data:%d, _is_shub_cid_error:%d, _is_origion_res_changed:%d", p_p2sp_task->_is_shub_ok,
			p_p2sp_task->_get_file_size_after_data , p_task->_is_shub_cid_error, p_p2sp_task->_data_manager._is_origion_res_changed);
		if( ((p_p2sp_task->_is_shub_ok == FALSE)&&(p_p2sp_task->_get_file_size_after_data==FALSE)) || p_task->_is_shub_cid_error 
			|| p_p2sp_task->_data_manager._is_origion_res_changed)	
		{
			if(p_p2sp_task->_new_gcid_level==0)
				p_p2sp_task->_new_gcid_level = 90;
			
			p_p2sp_task->_insert_course = IC_SHUB_NO_CID;

			// 如果cid校验错误，则设置上报特征为cid发生变化
			if( p_task->_is_shub_cid_error)
			{
				p_p2sp_task->_insert_course = IC_CID_CHANGE;
			}

			if(p_p2sp_task->_data_manager._is_origion_res_changed)
			{
				p_p2sp_task->_insert_course = IC_GCID_CHANGE;
			}
			#ifdef EMBED_REPORT
			reporter_insertsres(p_task);
			#endif
		}
		#ifdef EMBED_REPORT
		reporter_dw_stat(p_task);
		#endif

#ifdef UPLOAD_ENABLE
		pt_add_record_to_upload_manager( p_task );
#endif

#ifdef EMBED_REPORT
		emb_reporter_common_task_download_stat(p_task);
#endif

	}
	else if(p_task->task_status==TASK_S_FAILED)
	{
	
#ifdef EMBED_REPORT
		reporter_dw_fail(p_task);
		emb_reporter_common_task_download_fail(p_task);
#endif
	}

    uninit_dphub_query_context(&p_p2sp_task->_dpub_query_context);

	p_task->task_status = TASK_S_STOPPED;

	LOG_DEBUG("The task is stoped!_task_id=%u,_start_time=%u,_stop_time=%u",p_task->_task_id,p_task->_start_time,p_task->_finished_time);

	return SUCCESS;
}

_int32 pt_delete_task(TASK * p_task)
{
	P2SP_TASK * p_p2sp_task = (P2SP_TASK *)p_task;

	LOG_DEBUG("pt_delete_task:task_status=%d,_res_query_shub_state=%d,_res_query_phub_state=%d,_res_query_tracker_state=%d"
	,p_task->task_status,p_p2sp_task->_res_query_shub_state,p_p2sp_task->_res_query_phub_state,p_p2sp_task->_res_query_tracker_state);
	

	pt_uninit_task(p_p2sp_task);
	/* Return but do not release the _p_download_task until the dt_notify_file_closing_result_event is called */
/*	
	_result = mpool_free_slip(gp_task_slab,(void*) _p_download_task);
	if(_result!=SUCCESS)
	{
		LOG_DEBUG("Fatal Error calling: mpool_free_slip(gp_task_slab,(void*) _p_download_task)");
		CHECK_VALUE(_result);

	}
*/	

	return TM_ERR_WAIT_FOR_SIGNAL;
}



BOOL pt_is_task_exist_by_gcid(TASK* p_task,const _u8* gcid, BOOL include_no_disk)
{
	P2SP_TASK * p_p2sp_task = (P2SP_TASK *)p_task;
	 _u8 gcid_t[CID_SIZE];
	LOG_DEBUG("pt_is_task_exist");
	
    if(p_p2sp_task->_is_gcid_ok==TRUE && (include_no_disk 
#ifdef _VOD_NO_DISK  
        ||!(p_p2sp_task->_data_manager._is_no_disk)
#endif
        ))
	{
		if(FALSE==dm_get_shub_gcid(&(p_p2sp_task->_data_manager),gcid_t))
		{
			if(FALSE==dm_get_calc_gcid(&(p_p2sp_task->_data_manager),gcid_t))
			{
				return FALSE;
			}			
		}

		
		return sd_is_cid_equal(gcid,gcid_t);
	}
	
	return FALSE;
}

BOOL pt_is_task_exist_by_url(TASK* p_task,const char* url, BOOL include_no_disk)
{
	P2SP_TASK * p_p2sp_task = (P2SP_TASK *)p_task;
	char* origin_url = NULL;
	LOG_DEBUG("pt_is_task_exist_by_url");
	
	if((p_p2sp_task->_task_created_type==TASK_CREATED_BY_URL)||(p_p2sp_task->_task_created_type== TASK_CONTINUE_BY_URL))
	{
        if((include_no_disk 
#ifdef _VOD_NO_DISK  
            || !(p_p2sp_task->_data_manager._is_no_disk)
#endif
            ) && TRUE==dm_get_origin_url(&(p_p2sp_task->_data_manager), &origin_url))
		{
			if(0==sd_stricmp(url,origin_url))
			{
				return TRUE;
			}			
		}
	}
	
	
	return FALSE;
}

BOOL pt_is_task_exist_by_cid(TASK* p_task,const _u8* cid, BOOL include_no_disk)
{
    P2SP_TASK * p_p2sp_task = (P2SP_TASK *)p_task;
    _u8 cid_t[CID_SIZE] = {0};
    LOG_DEBUG("pt_is_task_exist_by_cid");

    if((include_no_disk 
#ifdef _VOD_NO_DISK  
        || !(p_p2sp_task->_data_manager._is_no_disk) 
#endif
        )&& (p_p2sp_task->_is_cid_ok==TRUE)&&(TRUE==dm_get_cid(&(p_p2sp_task->_data_manager),cid_t)))
    {
        return sd_is_cid_equal(cid, cid_t);
    }
    return FALSE;
}



_int32 pt_get_task_file_name(TASK* p_task, char *file_name_buffer, _int32* buffer_size)
{
	P2SP_TASK * p_p2sp_task = (P2SP_TASK *)p_task;
	char * file_name = NULL;
	LOG_DEBUG("pt_get_task_file_name");
	
	if(TRUE == dm_get_filnal_file_name( &(p_p2sp_task->_data_manager),&file_name))
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
			return SUCCESS;
		}
	}
	return DT_ERR_INVALID_FILE_NAME;
}
_int32 pt_get_task_tcid(TASK* p_task, _u8 * tcid)
{
	P2SP_TASK * p_p2sp_task = (P2SP_TASK *)p_task;
	

	if(TRUE == dm_get_cid( &(p_p2sp_task->_data_manager),tcid))
	{
#ifdef _DEBUG
		char tid_str[CID_SIZE*2+4] = {0};
		str2hex((const char*)tcid, CID_SIZE, tid_str, CID_SIZE*2+4);
		LOG_DEBUG("pt_get_task_tcid,tcid=%s", tid_str);
#endif
		return SUCCESS;
	}
	return DT_ERR_INVALID_TCID;
}

_int32 pt_get_task_gcid(TASK * p_task, _u8 * gcid)
{
	P2SP_TASK * p_p2sp_task = (P2SP_TASK *)p_task;
	_u8 cid[CID_SIZE];
	LOG_DEBUG("pt_get_task_gcid");

	if(TRUE == dm_get_shub_gcid(&(p_p2sp_task->_data_manager), cid))
	{
		LOG_DEBUG("pt_get_task_gcid, get shub gcid");
		sd_memcpy(gcid, cid, CID_SIZE);
		return SUCCESS;
	}
	else if(TRUE == dm_get_calc_gcid(&(p_p2sp_task->_data_manager), cid))
	{
		LOG_DEBUG("pt_get_task_gcid, get calc gcid");
		sd_memcpy(gcid, cid, CID_SIZE);
		return SUCCESS;
	}
	return DT_ERR_INVALID_GCID;
		
}

_int32 pt_remove_task_tmp_file( TASK * p_task )
{
#ifdef UPLOAD_ENABLE
	P2SP_TASK * p_p2sp_task = (P2SP_TASK *)p_task;
	 _u8 cid[CID_SIZE],gcid[CID_SIZE];
#endif

	LOG_DEBUG("dt_remove_task_tmp_file");
	
#ifdef UPLOAD_ENABLE
	if((p_p2sp_task->_is_add_rc_ok == TRUE)&&
		(dm_get_cid( &(p_p2sp_task->_data_manager),  cid)==TRUE)&&(dm_get_shub_gcid( &(p_p2sp_task->_data_manager),  gcid)==TRUE))
	{
		ulm_del_record(p_task->file_size,cid, gcid);
	}
#endif

	dt_remove_task_tmp_file(  p_task );

	return SUCCESS;
}

_int32 pt_get_detail_err_code( TASK * p_task)
{
	//_int32 ret_val = SUCCESS;
	_int32 type = 0;
	_u32 net_type = 0;
	BOOL have_origin = FALSE /*,have_recode = FALSE,have_ser_res=FALSE,have_peer_res=FALSE,have_cdn=FALSE*/;
	_int32 ser_num=0,peer_num=0,origin_err_code = 0;
	_int32 http_encap_state = 0;
	P2SP_TASK * p_p2sp_task = (P2SP_TASK *)p_task;

	type = (_int32)p_p2sp_task->_task_created_type;

	p_p2sp_task->_detail_err_code|=EC_NO_RESOURCE;

	/*  0-未知,1-迅雷p2p协议不需要http封装,2-迅雷p2p协议需要http封装  */
	settings_get_int_item( "p2p.http_encap_state",&http_encap_state );
	if(http_encap_state == 2)
	{
		p_p2sp_task->_detail_err_code |= EC_P2P_HTTP_ENCAP;
	}

	ser_num = list_size(&(p_task->_connect_manager._idle_server_res_list._res_list))+
			list_size(&(p_task->_connect_manager._using_server_res_list._res_list))+
 			list_size(&(p_task->_connect_manager._candidate_server_res_list._res_list))+
 			list_size(&(p_task->_connect_manager._retry_server_res_list._res_list))+
 			list_size(&(p_task->_connect_manager._discard_server_res_list._res_list));

	peer_num = list_size(&(p_task->_connect_manager._idle_peer_res_list._res_list))+
			list_size(&(p_task->_connect_manager._using_peer_res_list._res_list))+
 			list_size(&(p_task->_connect_manager._candidate_peer_res_list._res_list))+
 			list_size(&(p_task->_connect_manager._retry_peer_res_list._res_list))+
 			list_size(&(p_task->_connect_manager._discard_peer_res_list._res_list));


	if(p_task->_connect_manager._origin_res_ptr!=NULL)
	{
		have_origin = TRUE;
		origin_err_code = (_int32)p_task->_connect_manager._origin_res_ptr->_err_code;
		origin_err_code &= 0x0000FFFF;
		p_p2sp_task->_detail_err_code|=origin_err_code;
		
		if(ser_num>1)
		{
			p_p2sp_task->_detail_err_code|=EC_SHUB_HAVE_SER_RES;
		}
	}
	else
	{
		if(ser_num>0)
		{
			p_p2sp_task->_detail_err_code|=EC_SHUB_HAVE_SER_RES;
		}
	}

#ifdef ENABLE_CDN
	if(list_size(&p_task->_connect_manager._cdn_res_list._res_list)>0)
	{
		p_p2sp_task->_detail_err_code|=EC_SHUB_HAVE_MOBILE_CDN;
	}
#endif

	if(peer_num>0)
	{
		p_p2sp_task->_detail_err_code|=EC_PHUB_HAVE_PEER;
	}

	/* net type */
#if defined(MOBILE_PHONE)
	net_type = sd_get_net_type();
	switch(net_type)
	{
		case NT_CMWAP:
			net_type = EC_NT_CMWAP;
			break;
		case NT_CMNET:
			net_type = EC_NT_CMNET;
			break;
		case NT_CUIWAP:
			net_type = EC_NT_CUIWAP;
			break;
		case NT_CUINET:
			net_type = EC_NT_CUINET;
			break;
		default:
			if(net_type&NT_3G)
			{
				if(net_type&CHINA_MOBILE)
				{
					net_type =EC_NT_3G;
					net_type |=EC_NT_CHINA_MOBILE;
				}
				else
				if(net_type&CHINA_UNICOM)
				{
					net_type =EC_NT_3G;
					net_type |=EC_NT_CHINA_UNICOM;
				}
				else
				if(net_type&CHINA_TELECOM)
				{
					net_type =EC_NT_3G;
					net_type |=EC_NT_CHINA_TELECOM;
				}
				else
				{
					net_type =EC_NT_3G;
				}
			}
			else
			{
				net_type = EC_NT_WLAN;
			}
	}
#else
	net_type = EC_NT_WLAN;
#endif /* MOBILE_PHONE */

	p_p2sp_task->_detail_err_code|=net_type;

	if(p_p2sp_task->_detail_err_code==0)
	{
		p_p2sp_task->_detail_err_code = 130;
	}
	
	LOG_DEBUG("pt_get_detail_err_code:task_id=%u,err_code=0x%X, net_type =%d",p_task->_task_id,p_p2sp_task->_detail_err_code, net_type);
	return p_p2sp_task->_detail_err_code;
}


/* Update the task information */
_int32 pt_update_task_info(TASK * p_task)
{
	P2SP_TASK * p_p2sp_task = (P2SP_TASK *)p_task;
	_int32 ret_val = SUCCESS;
	_int32 _data_status_code = 0;
	#ifdef MOBILE_PHONE
	BOOL enable_cdn_mode = DEFAULE_CDN_MODE;
	_int32 disable_cdn_speed = DEFAULE_DISABLE_CDN_SPEED;
	_int32 enable_cdn_speed = DEFAULE_ENABLE_CDN_SPEED;
	#endif

	LOG_DEBUG("pt_update_task_info, task_ptr = 0x%x", p_p2sp_task);
	
	if((p_task->task_status!=TASK_S_RUNNING)&&(p_task->task_status!=TASK_S_VOD))
	{
		return DT_ERR_TASK_NOT_RUNNING;
	}
	
	/* Get the downloading status */
	_data_status_code = dm_get_status_code(& (p_p2sp_task->_data_manager));
	if(_data_status_code ==DATA_DOWNING) 
	{
		p_task->task_status = TASK_S_RUNNING;
		p_task->failed_code = 0;
	}else
	if(_data_status_code ==DATA_SUCCESS) 
	{
		p_task->task_status = TASK_S_SUCCESS;
		p_task->failed_code = 0;
	}else
	if(_data_status_code ==VOD_DATA_FINISHED) 
	{
		p_task->task_status = TASK_S_VOD;
		p_task->failed_code = 0;
	}else
	{
		p_task->task_status = TASK_S_FAILED;
		p_task->failed_code = _data_status_code;
	}

	/* Get the downloading progress */
	p_task->progress = dm_get_file_percent(& (p_p2sp_task->_data_manager));
	p_task->file_size = dm_get_file_size(& (p_p2sp_task->_data_manager));
	p_task->_downloaded_data_size= dm_get_download_data_size(& (p_p2sp_task->_data_manager));
	p_task->_written_data_size= dm_get_writed_data_size(& (p_p2sp_task->_data_manager));
	p_task->ul_speed= cm_get_upload_speed(& (p_task->_connect_manager));

#ifdef _CONNECT_DETAIL
       p_task->valid_data_speed = dm_get_valid_data_speed(& (p_p2sp_task->_data_manager));
#endif	

	if(p_task->task_status == TASK_S_RUNNING)
	{
		if( (p_task->_file_create_status == FILE_NOT_CREATED && p_task->_downloaded_data_size > 0 )
			|| p_task->file_size == p_task->_written_data_size )
		{
			cm_set_not_idle_status(&(p_task->_connect_manager) ,INVALID_FILE_INDEX);
		}
		else if( cm_is_idle_status( & (p_task->_connect_manager) ,INVALID_FILE_INDEX)==TRUE)
		{
			if( p_task->file_size > p_task->_written_data_size//防止断点续传时文件校验用太多时间任务失败
				|| p_task->file_size==0 )//防止shub没返回数据,长时间任务不结束
			{
				LOG_DEBUG("idle_status,TASK_S_FAILED");
				p_task->task_status = TASK_S_FAILED;
				p_task->failed_code = 130;
			}
		}
		
		/* 局域网任务和3g联通url的特殊处理,在长时间无下载速度时把任务置为失败状态 */
		if( (p_task->task_status == TASK_S_RUNNING) && p_p2sp_task->_is_lan )
		{
			_u32 now_time_stamp = 0;
			sd_time(&now_time_stamp);
			if( (p_task->speed == 0) && (vdm_get_vod_task_num() == 0) )
			{
				if(TIME_SUBZ(now_time_stamp, p_p2sp_task->_nospeed_timestmp)>=300)
				{
					if((p_task->_connect_manager._origin_res_ptr!=NULL)&&(p_task->_connect_manager._origin_res_ptr->_err_code!=SUCCESS))
					{
						p_task->task_status = TASK_S_FAILED;
						p_task->failed_code = (_int32)p_task->_connect_manager._origin_res_ptr->_err_code;
					}
				}
			}
			else
			{
				p_p2sp_task->_nospeed_timestmp = now_time_stamp;
			}
		}
		LOG_DEBUG("pt_update_task_info: p_task->task_status =%d, p_p2sp_task->_is_unicom =%d",p_task->task_status, p_p2sp_task->_is_unicom);
		/* 3g联通url的特殊处理,在长时间无下载速度时把任务置为失败状态 */
		if( (p_task->task_status == TASK_S_RUNNING) &&  p_p2sp_task->_is_unicom )
		{
			_u32 now_time_stamp = 0;
			sd_time(&now_time_stamp);
			if( p_task->speed == 0 )
			{
				LOG_DEBUG("pt_update_task_info: p_p2sp_task->_nospeed_timestmp =%u, now_time_stamp = %u", p_p2sp_task->_nospeed_timestmp, now_time_stamp);
				if(TIME_SUBZ(now_time_stamp, p_p2sp_task->_nospeed_timestmp) >= 300)
				{
					LOG_DEBUG("pt_update_task_info: unicom_3g task failed");
					p_task->task_status = TASK_S_FAILED;
					p_task->failed_code = 130;
				}
			}
			else
			{
				p_p2sp_task->_nospeed_timestmp = now_time_stamp;
			}
		}
	}
	
	ret_val = dt_update_task_info( p_task  );

#ifdef VVVV_VOD_DEBUG
        gloabal_task_speed  = p_task->speed;

        gloabal_vv_speed  = dm_get_valid_data_speed(& (p_p2sp_task->_data_manager));
#endif	
	if(ret_val!=SUCCESS) return ret_val;
	
#if defined( MACOS )&&(!defined(MOBILE_PHONE)) /* zyq @ 20111021 */
       p_task->speed= dm_get_valid_data_speed(& (p_p2sp_task->_data_manager));
#endif

	/* Get other informations which reporting need */
	if((p_task->task_status == TASK_S_SUCCESS)||(p_task->task_status == TASK_S_FAILED))
	{
		//_p_download_task->redirect_url = ;
		//_p_download_task->redirect_url_length = sd_strlen(_p_download_task->redirect_url);
		p_p2sp_task->_origin_url_ip = 0;
		p_p2sp_task->_download_status =  0;   //下载行为状态字(每一bit代表一状态) b0: 原始URL下载的字节数为0
		p_p2sp_task->_insert_course = 0;
		//p_p2sp_task->_is_shub_return_file_size = 0;

		if((p_task->peer_speed!=0)&&(p_p2sp_task->_new_gcid_level!=1))
		{
			p_p2sp_task->_new_gcid_level = 1;
		}

		if(p_task->failed_code == 130)
		{
			p_task->failed_code = pt_get_detail_err_code(  p_task);
		}
	}
	#ifdef MOBILE_PHONE
	else if(p_task->task_status == TASK_S_RUNNING&&!dm_is_vod_mode(&(p_p2sp_task->_data_manager)))
	{
		/* 下载任务的CDN 使用策略 */
		settings_get_bool_item("system.enable_cdn_mode",&enable_cdn_mode);
		if(enable_cdn_mode)
		{
			settings_get_int_item("system.disable_cdn_speed",&disable_cdn_speed);
			settings_get_int_item("system.enable_cdn_speed", &enable_cdn_speed);
			if(p_task->speed<enable_cdn_speed*1024)
			{
				pt_set_cdn_mode(p_task, TRUE);
			}
			else
			if(p_task->speed-pt_get_cdn_speed(p_task)>disable_cdn_speed*1024)
			{
				//pt_set_cdn_mode(p_task, FALSE);
				pt_set_cdn_mode(p_task, TRUE);
			}
		}
		else
		{
				//pt_set_cdn_mode(p_task, FALSE);
				pt_set_cdn_mode(p_task, TRUE);
		}
	}
	#endif /* MOBILE_PHONE */

	/* 确保任务正在运行的时候,_downloaded_data_size 不会大于 file_size */
	if(p_task->task_status == TASK_S_RUNNING||p_task->task_status == TASK_S_VOD)
	{
		if((p_task->file_size != 0) && (p_task->_downloaded_data_size >= p_task->file_size))
		{
			p_task->_downloaded_data_size = p_task->file_size -1;
			if(p_task->_written_data_size>=p_task->file_size)
			{
				if(p_task->task_status == TASK_S_VOD)
				{
					/* 该任务是一个点播任务,且已经下载完毕 */
					p_task->_downloaded_data_size = p_task->file_size;
				}
				else
				{
					if(p_p2sp_task->_data_manager._file_info._finished 
                        && dm_check_kankan_lan_file_finished(&(p_p2sp_task->_data_manager)))
					{
						p_task->_downloaded_data_size = p_task->file_size;
					}
				}
			}
		}
	}

	return SUCCESS;

}

BOOL pt_is_shub_ok(TASK* p_task)
{
	P2SP_TASK * p_p2sp_task = (P2SP_TASK *)p_task;
	
	if(p_task==NULL) return TRUE;
	
	if((p_p2sp_task->_is_shub_ok==TRUE)||(p_p2sp_task->_res_query_shub_state>RES_QUERY_REQING))
	{
		LOG_DEBUG("pt_is_shub_ok:TRUE");
		return TRUE;
	}

	LOG_DEBUG("pt_is_shub_ok:FALSE");
	return FALSE;
}

_int32 pt_get_file_size_after_data(TASK * p_task)
{
	P2SP_TASK * p_p2sp_task = (P2SP_TASK *)p_task;
	LOG_DEBUG("pt_get_file_size_after_data");
	
	if(p_task==NULL) return SUCCESS;
	
	if(p_p2sp_task->_get_file_size_after_data==FALSE)
	{
		p_p2sp_task->_get_file_size_after_data=TRUE;
	}

	return SUCCESS;
}
	
/*--------------------------------------------------------------------------*/
/*                Interfaces provid for data_manager				        */
/*--------------------------------------------------------------------------*/

/* file creating result notify,this function shuld be called by data_manager */
_int32 pt_notify_file_creating_result_event(TASK* p_task,BOOL result)
{
	
	LOG_DEBUG("pt_notify_file_creating_result_event");

	if(p_task==NULL) return DT_ERR_INVALID_DOWNLOAD_TASK;

	if(result==TRUE)
		p_task->_file_create_status= FILE_CREATED_SUCCESS;
	else
		p_task->_file_create_status= FILE_CREATED_FAILED;

	return SUCCESS;
	

}

/* file closing result notify,this function shuld be called by data_manager when deleting a task */
_int32 pt_notify_file_closing_result_event(TASK* p_task,BOOL result)
{
	_int32 ret_val = SUCCESS;
	P2SP_TASK * p_p2sp_task = (P2SP_TASK *)p_task;
	LOG_DEBUG("pt_notify_file_closing_result_event(_p_download_task=%X,_result=%d)",p_task,result);

	if(p_task==NULL) return DT_ERR_INVALID_DOWNLOAD_TASK;

	/* Check if need to remove the temp files */
	if(p_task->need_remove_tmp_file == TRUE)
		dm_delete_tmp_file(&(p_p2sp_task->_data_manager));


	ret_val = mpool_free_slip(gp_p2sp_task_slab,(void*) p_task);
	if(ret_val!=SUCCESS)
	{
		LOG_DEBUG("Fatal Error calling: mpool_free_slip(gp_p2sp_task_slab,(void*) p_task)");
	}

	/* Release the signal sevent  */
	tm_signal_sevent_handle();

	return SUCCESS;
}


/* Get the excellent file name from connect maneger,this function shuld be called by data_manager */
BOOL pt_get_excellent_filename(TASK* p_task,char *p_str_name_buffer, _u32 buffer_len )
{
	return cm_get_excellent_filename(&(p_task->_connect_manager), p_str_name_buffer, buffer_len );
}

/* Set the origin download mode to connect maneger,this function shuld be called by data_manager */
_int32 pt_set_origin_download_mode(TASK* p_task, RESOURCE *p_orgin_res )
{
	return cm_set_origin_download_mode(&(p_task->_connect_manager), p_orgin_res );
}

/* Set the origin download mode to connect maneger,this function shuld be called by server pipe */
_int32 pt_set_origin_mode(TASK* p_task, BOOL origin_mode )
{
	P2SP_TASK * p_p2sp_task = NULL;
	BOOL current_mode = FALSE;
  
	if(!p_task) return SUCCESS;

	current_mode = cm_get_origin_mode( &(p_task->_connect_manager) );
        //如果当前为多资源调度模式，不允许再设置回原始资源调度
        if (FALSE == current_mode && TRUE == origin_mode)
        {
            return DP_ERR_MODE_SWITCH_NOT_SUPPORT;
        }

	LOG_ERROR( "oooo pt_set_origin_mode : task_id=%u, set mode=%d, current_mode=%d!! ",p_task->_task_id,origin_mode,current_mode);
	cm_set_origin_mode(&(p_task->_connect_manager), origin_mode );

	if(p_task->_task_type !=P2SP_TASK_TYPE ) 
        return SUCCESS;

	p_p2sp_task = (P2SP_TASK *)p_task;

    //pt_start_query(p_task); // 强制查hub得到文件大小等信息

	//if(p_p2sp_task->_task_created_type!=TASK_CREATED_BY_URL) return SUCCESS;
	if( origin_mode == FALSE && current_mode == TRUE )
	{
		LOG_ERROR( "oooo pt_set_origin_mode start_query: task_id=%u!! ",p_task->_task_id);
		pt_start_query(p_task);
	}
	return SUCCESS;
}
BOOL pt_is_origin_mode(TASK* p_task )
{
	BOOL current_mode = FALSE;

	if(!p_task) return FALSE;
	if(p_task->_task_type!=P2SP_TASK_TYPE) return FALSE;
	
	current_mode = cm_get_origin_mode( &(p_task->_connect_manager) );

	return current_mode;
}


/*--------------------------------------------------------------------------*/
/*                Internal functions				        */
/*--------------------------------------------------------------------------*/
#ifdef UPLOAD_ENABLE
_int32 pt_add_record_to_upload_manager( TASK * p_task )
{
	P2SP_TASK * p_p2sp_task = (P2SP_TASK *)p_task;
	char file_name_buffer[MAX_FILE_PATH_LEN],*file_path=NULL,*file_name=NULL;
	 _u8 cid[CID_SIZE],gcid[CID_SIZE];

	LOG_DEBUG("pt_add_record_to_upload_manager");
	
	if(p_task->file_size<MIN_UPLOAD_FILE_SIZE) return SUCCESS;
	if(PIPE_FILE_HTML_TYPE==dm_get_file_type( &(p_p2sp_task->_data_manager )))
	{
		return SUCCESS;
	}
	
	if(/*(p_p2sp_task->_is_cid_ok == TRUE)&&(p_p2sp_task->_is_gcid_ok == TRUE)&&*/
		(TRUE == dm_get_filnal_file_name( &(p_p2sp_task->_data_manager),&file_name))
		&&(TRUE == dm_get_file_path(  &(p_p2sp_task->_data_manager),&file_path)))
	{
		if(sd_strlen(file_path)+sd_strlen(file_name)<MAX_FILE_PATH_LEN) 
		{
			sd_memset(file_name_buffer,0,MAX_FILE_PATH_LEN);
			sd_snprintf(file_name_buffer, MAX_FILE_PATH_LEN,"%s%s", file_path,file_name);
			if((dm_get_cid( &(p_p2sp_task->_data_manager),  cid)==TRUE)&&(dm_get_shub_gcid( &(p_p2sp_task->_data_manager),  gcid)==TRUE))
			{
				if(SUCCESS ==ulm_add_record(p_task->file_size, cid, gcid, NORMAL_HUB, file_name_buffer))
				{
					p_p2sp_task->_is_add_rc_ok = TRUE;
				}
			}
		}
	}
		
	return SUCCESS;
}
#endif



#ifdef ENABLE_CDN
_int32 pt_set_cdn_mode( TASK * p_task,BOOL mode)
{
	return cm_set_cdn_mode(&(p_task->_connect_manager), INVALID_FILE_INDEX,mode);

}
_u32 pt_get_cdn_speed( TASK * p_task )
{
	return cm_get_cdn_speed( &(p_task->_connect_manager), TRUE );

}
#ifdef ENABLE_HSC
_u32 pt_get_vip_cdn_speed( TASK * p_task )
{
	return cm_get_vip_cdn_speed( &(p_task->_connect_manager) );
}
#endif /* ENABLE_HSC */
#endif /* ENABLE_CDN */

#ifdef ENABLE_LIXIAN
_u32 pt_get_lixian_speed(TASK* p_task)
{
	return cm_get_lixian_speed( &(p_task->_connect_manager) );
}
#endif


_int32  pt_dm_notify_buffer_recved(TASK* task_ptr, RANGE_DATA_BUFFER*  buffer_node )
{

	return vdm_dm_notify_buffer_recved(task_ptr, buffer_node);
}

_int32 pt_dm_get_data_buffer_by_range(TASK* task_ptr, RANGE* r,  RANGE_DATA_BUFFER_LIST* ret_range_buffer)
{
      return vdm_dm_get_data_buffer_by_range(task_ptr, r, ret_range_buffer);
}

_int32 pt_dm_notify_check_error_by_range(TASK* task_ptr, RANGE* r)
{
    return vdm_dm_notify_check_error_by_range(task_ptr, r);
}

_int32 pt_origion_pos_to_relation_pos(RELATION_BLOCK_INFO* p_block_info_arr, _u32 block_num, _u64 origion_pos, _u64* relation_pos)
{

	_int32 ret = -1;

	LOG_DEBUG("pt_origion_pos_to_relation_pos enter. origion_pos:%llu", origion_pos);
	 
	sd_assert(block_num > 0);
	
	RELATION_BLOCK_INFO* p_find_block = NULL;

	_u32 start_idx = 0,end_idx = block_num - 1;

	while(start_idx <= end_idx)
	{
		_u32 mid = (start_idx + end_idx )  / 2;

		RELATION_BLOCK_INFO* p_block_info = &p_block_info_arr[mid];

		if(p_block_info->_origion_start_pos <= origion_pos && (p_block_info->_origion_start_pos + p_block_info->_length) >= origion_pos)
		{
			p_find_block = p_block_info;
			break;
		}

		if(p_block_info->_origion_start_pos < origion_pos)
		{
			start_idx = mid + 1;
		}
		else
		{
			if(mid == 0)
			{	
				break;
			}
			else
			{
				end_idx = mid - 1;
			}
		}
	}


	if(NULL == p_find_block)
	{
		LOG_DEBUG("pt_origion_pos_to_relation_pos origion_pos cant fine block...");
		return ret;
	}


	ret = 0;

	_u64 greaten = origion_pos - p_find_block->_origion_start_pos;
	_u64 dest_pos = p_find_block->_relation_start_pos + greaten;


	LOG_DEBUG("pt_origion_pos_to_relation_pos origion_pos:%llu, dest_pos:%llu, block_length:%llu", 
		origion_pos, dest_pos, p_find_block->_length);

	*relation_pos = dest_pos;

	return ret;
	
}

void dump_relation_file_info(char* pos,_u32 i, P2SP_RELATION_FILEINFO* p_relation_info)
{
#ifdef _DEBUG
	char cidbuff[41],gcidbuff[41];
	cidbuff[40] = 0; gcidbuff[40] = 0;
	_u32 j = 0;

	str2hex((const char*)p_relation_info->_cid, CID_SIZE, cidbuff, 40);
	str2hex((const char*)p_relation_info->_gcid, CID_SIZE, gcidbuff, 40);
	LOG_DEBUG("%s file[%u] cid=%s,gcid=%s, filesize=%llu",pos,  i, cidbuff, gcidbuff, p_relation_info->_file_size);

	for(j = 0; j < p_relation_info->_block_info_num; j++)
	{
		RELATION_BLOCK_INFO* p_block = &p_relation_info->_p_block_info_arr[j];

		LOG_DEBUG("dt_add_relation_fileinfo file[%u] blockinfo(%d, %llu, %llu, %llu)", i, p_block->_block_info_valid, 
			p_block->_origion_start_pos, p_block->_relation_start_pos, p_block->_length);
	}
#endif
}

_int32 pt_set_task_dispatch_mode(TASK* p_task, TASK_DISPATCH_MODE mode, _u64 filesize_to_little_file )
{
    LOG_DEBUG("pt_set_task_dispatch_mode task:%u, mode:%d, filesize:%u", p_task->_task_id, mode, filesize_to_little_file );

    _int32 ret_val = SUCCESS;
    P2SP_TASK *pP2spTask = NULL;
  
    if(!p_task) return SUCCESS;

    if (P2SP_TASK_TYPE == p_task->_task_type)
    {
        if (TASK_DISPATCH_MODE_LITTLE_FILE==mode)
        {
            ret_val = pt_set_origin_mode(p_task, FALSE);
            if (SUCCESS!=ret_val) return ret_val;
        }
        pP2spTask = (P2SP_TASK *)p_task;
        pP2spTask->_filesize_to_little_file = filesize_to_little_file;
        pP2spTask->_connect_dispatch_mode = mode;
    }
    else
    {
        ret_val = DT_ERR_INVALID_TASK_TYPE;
    }

    return ret_val;

}


_int32 pt_get_download_bytes(TASK* p_task, char* host, _u64* download)
{
    LOG_DEBUG("pt_get_download_bytes task_id : %d, host = %s", p_task->_task_id, host);
    _int32 ret_val = SUCCESS;
    P2SP_TASK* p2sp_task = NULL;
    if (!p_task)
    {
        return SUCCESS;
    }
    else
    {
        p2sp_task = (P2SP_TASK*)p_task;
        dm_get_download_bytes(&p2sp_task->_data_manager, host, download);        
    }
    return ret_val;
}

#endif /* __P2SP_TASK_C_20090224 */
