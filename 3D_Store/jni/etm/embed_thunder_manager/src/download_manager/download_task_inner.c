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


#include "download_manager/download_task_inner.h"
#include "download_manager/download_task_data.h"
#include "download_manager/download_task_impl.h"
#include "download_manager/download_task_store.h"
#include "torrent_parser/torrent_parser_interface.h"

#include "et_interface/et_interface.h"
#include "scheduler/scheduler.h"

#include "utility/utility.h"
#include "utility/url.h"
#include "utility/base64.h"

#include "em_common/em_url.h" 
#include "em_common/em_utility.h"
#include "em_common/em_mempool.h"
#include "em_common/em_crc.h"
#include "em_interface/em_fs.h"
#include "em_common/em_string.h"
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



/*--------------------------------------------------------------------------*/
/*                Interfaces provid for external program				        */
/*--------------------------------------------------------------------------*/




/*--------------------------------------------------------------------------*/
/*                Internal functions				        */
/*--------------------------------------------------------------------------*/


_int32 dt_id_comp(void *E1, void *E2)
{
	_u32 id1 = (_u32)E1,id2 = (_u32)E2;
	_int32 ret_val = 0;
	
	if(id1>MAX_DL_TASK_ID)
		id1-=MAX_DL_TASK_ID;
	
	if(id2>MAX_DL_TASK_ID)
		id2-=MAX_DL_TASK_ID;
	ret_val =  ((_int32)id1) -((_int32)id2);
	return ret_val;
	//return ((_int32)E1) -((_int32)E2);
}

_int32 dt_eigenvalue_comp(void *E1, void *E2)
{
	_int32 i ,ret_val=0;
	_u8 *value1=(_u8*)E1,*value2=(_u8*)E2;

	if(value1 == NULL||value2==NULL)
	{
		EM_LOG_DEBUG("dt_eigenvalue_comp -1");
		return -1;
	}
#ifdef _DEBUG
{
	char buffer1[64] = {0};
	char buffer2[64] = {0};
	str2hex((const char *)value1,CID_SIZE, buffer1, 63);
	str2hex((const char *)value2,CID_SIZE, buffer2, 63);
	EM_LOG_DEBUG("dt_eigenvalue_comp:%s,%s",buffer1,buffer2);
}
#endif
	for(i=0;i<CID_SIZE;i++)
	{
		if(value1[i]!=value2 [i]) 
		{
			ret_val=value1[i]>value2 [i]? 1:(value1[i]<value2 [i]?-1:0 ); 
			EM_LOG_DEBUG("dt_eigenvalue_comp NOT equal:value1[%d]=0x%X,value2[%d]=0x%X",i,value1[i],i,value2 [i]);
			break;
		}
	}
	EM_LOG_DEBUG("dt_eigenvalue_comp:%d",ret_val);
	return ret_val;
}
_int32 dt_file_name_eigenvalue_comp(void *E1, void *E2)
{
	return ((_int32)E1) -((_int32)E2);
}


////////////////////////////////////////////////////////////////////

/*
功能: 找出保存在内存或者文件中的bt子文件列表信息，根据序号找到对应的数组索引号
参数: 
	EM_TASK: 任务信息
	file_index: 需要找的子文件序号
	*array_index: 找到的子文件序号对应的数组索引号
返回: SUCCESS表示成功，其余为错误码
*/
_int32 dt_get_bt_sub_file_array_index(EM_TASK * p_task,_u16 file_index,_u16 *array_index)
{
	EM_BT_TASK *p_bt_task=NULL;
	BOOL need_release=FALSE;
	_u16  i=0,file_num=0,*index_array=NULL;
      EM_LOG_DEBUG("dt_get_bt_sub_file_array_index:task_id=%u",p_task->_task_info->_task_id); 
	  
	file_num = p_task->_task_info->_url_len_or_need_dl_num;
	if(p_task->_task_info->_full_info)
	{
		p_bt_task = (EM_BT_TASK *)p_task->_task_info;
		index_array = p_bt_task->_dl_file_index_array;
	}
	else
	{
		if(p_task->_bt_running_files!=NULL)
		{
			index_array = p_task->_bt_running_files->_need_dl_file_index_array;
		}
		else
		{
			index_array =dt_get_task_bt_need_dl_file_index_array( p_task);
			if(index_array==NULL) return -1;
			need_release=TRUE;
		}

	}
	
	while((i<file_num)&&(index_array[i]!=file_index))
				i++;

	if(need_release==TRUE)
	{
		dt_release_task_bt_need_dl_file_index_array(index_array);
	}
			
	if(i==file_num)
		return EM_INVALID_FILE_INDEX;
	else
	{
		*array_index = i;
		return SUCCESS;
	}
}
_int32 dt_generate_eigenvalue(CREATE_TASK * p_create_param,_u8 * eigenvalue)
{
      EM_LOG_DEBUG("dt_generate_eigenvalue"); 
	switch(p_create_param->_type)
	{
		case TT_URL:
		case TT_EMULE:
			em_sd_replace_str(p_create_param->_url, "%7C", "|");
			p_create_param->_url_len = em_strlen(p_create_param->_url);
			return dt_get_url_eigenvalue(p_create_param->_url,p_create_param->_url_len,eigenvalue);
			break;
		case TT_LAN:
		case TT_TCID:
			return dt_get_cid_eigenvalue(p_create_param->_tcid,eigenvalue);
			break;
		case TT_KANKAN:
			return dt_get_cid_eigenvalue(p_create_param->_gcid,eigenvalue);
			break;
		case TT_FILE:
			return dt_get_file_eigenvalue(p_create_param,eigenvalue);
			break;
		case TT_BT:
		default:
			return INVALID_TASK_TYPE;
	}
	return SUCCESS;
}

_int32 dt_generate_file_name_eigenvalue(char * file_path,_u32 path_len,char * file_name,_u32 name_len,_u32 * eigenvalue)
{
       _u32  /*path_hash=0,*/name_hash=0,/* path_sum = 0, */ name_sum=0;
	_u16 crc_value=0xffff;
		
	EM_LOG_DEBUG( "dt_generate_file_name_eigenvalue:file_path=[%u]%s,file_name=[%u]%s", path_len,file_path,name_len,file_name);
	*eigenvalue=0;
	
	//if(em_get_url_hash_value( file_path, path_len, &path_hash )!=0)
	//	return INVALID_FILE_PATH;
#ifdef NO_CRC	
	if(em_get_url_sum( file_path, path_len, &path_sum )!=0)
		return INVALID_FILE_PATH;

	if(em_get_url_hash_value( file_name, name_len, &name_hash )!=0)
		return INVALID_FILE_NAME;
	
	if(em_get_url_sum( file_name, name_len, &name_sum )!=0)
		return INVALID_FILE_NAME;

	*eigenvalue = 0xFF000000&((path_len+name_len)<<24);
	*eigenvalue|=0x00FFF000&(name_hash<<12);
	*eigenvalue|=0x00000FFF&(path_sum+name_sum);
		
#else
	if(em_get_url_hash_value( file_name, name_len, &name_hash )!=0)
		return INVALID_FILE_NAME;

	crc_value = em_add_crc16(crc_value, file_path, path_len);
	crc_value = em_add_crc16(crc_value, file_name, name_len);
	crc_value = em_inv_crc16(crc_value);
	name_sum = crc_value;

	*eigenvalue = 0xFFFF0000&(name_hash<<16);
	*eigenvalue|=0x0000FFFF&(name_sum);
		
#endif

	EM_LOG_DEBUG( "dt_generate_file_name_eigenvalue=%X", *eigenvalue);

	return SUCCESS;
}

/*
_int32 dt_get_bt_eigenvalue(char * seed_file,_u8 * eigenvalue)
{
	_int32 ret_val = SUCCESS;
	ETI_TORRENT_SEED_INFO *p_seed_info=NULL;
	// bad idea,need optimize 
	ret_val =eti_get_torrent_seed_info( seed_file, &p_seed_info );
	CHECK_VALUE(ret_val);

	sd_memset(eigenvalue,0,CID_SIZE);
	sd_memcpy(eigenvalue,p_seed_info->_info_hash,CID_SIZE);
	
	eti_release_torrent_seed_info( p_seed_info );
	
	return SUCCESS;
}
*/
_int32 dt_get_url_eigenvalue(char * url,_u32 url_len,_u8 * eigenvalue)
{
	_int32 ret_val = SUCCESS;
	_u32 hash_value = 0,sum=0;

	if(url_len<9) return INVALID_URL;
	
	if((em_strstr(url, "http://", 0)!=url)&&(em_strstr(url, "ftp://", 0)!=url)&&(em_strstr(url, "https://", 0)!=url)
		&&(em_strstr(url, "HTTP://", 0)!=url)&&(em_strstr(url, "FTP://", 0)!=url)&&(em_strstr(url, "HTTPS://", 0)!=url))
	{
		if((em_strstr(url, "ed2k://", 0)!=url)&&(em_strstr(url, "ED2K://", 0)!=url)&&(em_strstr(url, "thunder://", 0)!=url)&&(em_strstr(url, "THUNDER://", 0)!=url))
		{
			return INVALID_URL;
		}
	}
	else
	{
		URL_OBJECT url_o;
		if(sd_url_to_object(url,url_len,&url_o)!=SUCCESS)
		{
			return INVALID_URL;
		}
	}

	if((sd_strncmp(url, "http://gdl.lixian.vip.xunlei.com/download?", sd_strlen("http://gdl.lixian.vip.xunlei.com/download?"))==0)||
		(sd_strstr(url, DEFAULT_YUNBO_URL_MARK, 0)!=NULL))
	{
		char * p_gcid = sd_strstr(url,"&g=",0);
		if(p_gcid!=NULL)
		{
			p_gcid+=3;
			ret_val = sd_string_to_cid(p_gcid,eigenvalue);
			if(ret_val == SUCCESS) 
				return SUCCESS;
		}
		//////////////////////////////
		char * p_n = sd_strstr(url,"&n=",0);
		if((p_n!=NULL)&&(p_n-url<url_len)&&(url_len-(p_n-url)<6))
		{
			/* 由于离线服务器返回的视频文件的vod url的最后面会带有一个随机参数"&n=" ,必须去掉该字段再生成特征值*/
			url_len = p_n-url;
		}
	}
	
	if(em_get_url_hash_value( url, url_len, &hash_value )!=0)
		return INVALID_EIGENVALUE;
	
	if(em_get_url_sum( url, url_len, &sum )!=0)
		return INVALID_EIGENVALUE;
	
	em_memset(eigenvalue,0,CID_SIZE);
	em_memcpy(eigenvalue,&url_len,sizeof(_u32));
	em_memcpy(eigenvalue+sizeof(_u32),&hash_value,sizeof(_u32));
	em_memcpy(eigenvalue+2*sizeof(_u32),&sum,sizeof(_u32));
	
	return SUCCESS;
}

_int32 dt_get_cid_eigenvalue(char * cid,_u8 * eigenvalue)
{
	em_memset(eigenvalue,0,CID_SIZE);
	if(em_string_to_cid(cid,eigenvalue)!=0)
		return INVALID_EIGENVALUE;
	
	return SUCCESS;
}
BOOL dt_is_final_file_exist(char * file_path,_u32 path_len,char * file_name,_u32 name_len)
{
	char full_path[MAX_FULL_PATH_BUFFER_LEN];
	
	em_memset(full_path, 0, MAX_FULL_PATH_BUFFER_LEN);
	em_strncpy(full_path, file_path, path_len);
	if(file_path[path_len-1]!='/')
	{
		em_strcat(full_path, "/", 1);
	}
	em_strcat(full_path, file_name, name_len);
	
	if( em_file_exist(full_path))
		return TRUE; 
	return FALSE;
}
 _int32 dt_get_file_eigenvalue(CREATE_TASK * p_create_param,_u8 * eigenvalue)
{
	_int32 ret_val = SUCCESS;
	_u32 file_eigenvalue=0;
	//_u64 file_size = 0;
	
	if( !dt_is_final_file_exist(p_create_param->_file_path,p_create_param->_file_path_len, p_create_param->_file_name,p_create_param->_file_name_len))
		return FILE_NOT_EXIST;
	
	//ret_val = sd_calc_file_gcid(full_path,eigenvalue, NULL,NULL,&file_size);
	ret_val = dt_generate_file_name_eigenvalue(p_create_param->_file_path,p_create_param->_file_path_len, p_create_param->_file_name,p_create_param->_file_name_len,&file_eigenvalue);
	CHECK_VALUE( ret_val );
	em_memset(eigenvalue,0,CID_SIZE);
	em_memcpy(eigenvalue,&file_eigenvalue,sizeof(_u32));
	//p_create_param->_file_size = file_size;
	
	return SUCCESS;
}

_int32 dt_check_and_sort_bt_file_index(_u32 *  src_need_dl_file_index_array,_u32 src_need_dl_file_num,_u32 file_total_num,_u16 ** pp_need_dl_file_index_array,_u16 * p_need_dl_file_num)
{
	_u32 file_index = 0;
	_u32 file_num = 0;
	_u16 *file_index_array = 0;
	_u16 count=0,i=0;
	
	_int32 ret_val = SUCCESS;
	EM_LOG_DEBUG( "dt_check_and_sort_bt_file_index." );

	
	if((src_need_dl_file_num==0)||(file_total_num==0)||( src_need_dl_file_num > file_total_num ))
	{
		return INVALID_FILE_NUM;
	}
	
	ret_val = em_malloc( file_total_num * sizeof(_u16), (void **)&file_index_array );
	CHECK_VALUE( ret_val );

	ret_val = em_memset( file_index_array, 0, file_total_num * sizeof(_u16) );
      if(ret_val != SUCCESS)
      	{
      	     EM_SAFE_DELETE( file_index_array );
	     return ret_val;		 
      	}

	for( file_num = 0; file_num < src_need_dl_file_num; file_num++ )
	{
		file_index =  src_need_dl_file_index_array[file_num];
		if(( file_index >= file_total_num )||( file_index >= MAX_FILE_INDEX) ) 
		{
			continue;
		}
		file_index_array[file_index]++;
		if(file_index_array[file_index]==1)
		{
			count++;
			sd_assert(count<MAX_FILE_NUM);
		}
	}

	ret_val = em_malloc( count * sizeof(_u16), (void **)pp_need_dl_file_index_array );
      if(ret_val != SUCCESS)
      	{
      	     EM_SAFE_DELETE( file_index_array );
	     return ret_val;		 
      	}

	em_memset( *pp_need_dl_file_index_array, 0, count * sizeof(_u16) );

	for( file_num = 0; file_num < file_total_num; file_num++ )
	{
		if(file_index_array[file_num]>0)
		{
			(*pp_need_dl_file_index_array)[i++]=file_num;
		}
	}
	sd_assert(count==i);
	*p_need_dl_file_num=count;
	EM_SAFE_DELETE( file_index_array );
	return ret_val;
	
}

_int32 dt_get_all_bt_file_index(TORRENT_SEED_INFO *p_seed_info,_u16 ** pp_need_dl_file_index_array,_u16 * p_need_dl_file_num)
{
	_u32 file_num = 0;
	_u16 *file_index_array = 0;
	_u16 dl_file_num=0;
	const char *padding_str = "_____padding_file";
	TORRENT_FILE_INFO *file_info_array_ptr;
	
	_int32 ret_val = SUCCESS;
	EM_LOG_DEBUG( "dt_get_all_bt_file_index." );
//
	//char _title_name[256-ET_MAX_TD_CFG_SUFFIX_LEN];
	//u32 _title_name_len;
	//uint64 _file_total_size;
	//u32 _file_num;
	//u32 _encoding;//种子文件编码格式，GBK = 0, UTF_8 = 1, BIG5 = 2
	//unsigned char _info_hash[20];
///
	
	if(p_seed_info->_file_num==0)
	{
		return INVALID_FILE_NUM;
	}
	
	ret_val = em_malloc( p_seed_info->_file_num * sizeof(_u16), (void **)&file_index_array );
	CHECK_VALUE( ret_val );

	ret_val = em_memset( file_index_array, 0, p_seed_info->_file_num * sizeof(_u16) );
      if(ret_val != SUCCESS)
      	{
      	     EM_SAFE_DELETE( file_index_array );
	     return ret_val;		 
      	}

	file_info_array_ptr = *(p_seed_info->_file_info_array_ptr);
	for( file_num = 0; file_num < p_seed_info->_file_num; file_num++ )
	{
		if((dl_file_num >= MAX_FILE_NUM)||(file_info_array_ptr->_file_index>=MAX_FILE_INDEX))
		{
			break;
		}

		if(( file_info_array_ptr->_file_size > MIN_BT_FILE_SIZE )
			&& em_strncmp( file_info_array_ptr->_file_name, padding_str, em_strlen(padding_str) ) )
		{
			file_index_array[dl_file_num] = file_info_array_ptr->_file_index;
			dl_file_num++;
		}
		file_info_array_ptr++;
	}

	ret_val = em_malloc( dl_file_num * sizeof(_u16), (void **)pp_need_dl_file_index_array );
      if(ret_val != SUCCESS)
      	{
      	     EM_SAFE_DELETE( file_index_array );
	     return ret_val;		 
      	}

	em_memset( *pp_need_dl_file_index_array, 0, dl_file_num * sizeof(_u16) );

	for( file_num = 0; file_num < dl_file_num; file_num++ )
	{
		(*pp_need_dl_file_index_array)[file_num]=file_index_array[file_num];
	}

	*p_need_dl_file_num=dl_file_num;
	EM_SAFE_DELETE( file_index_array );
	return ret_val;	
}


_int32 dt_init_task_info(CREATE_TASK * p_create_param,TASK_INFO ** pp_task_info ,_u32* p_task_id, BOOL create_by_cfg)
{
	_int32 ret_val = SUCCESS;
	TASK_INFO * p_task_info = NULL;
	_u8  eigenvalue[CID_SIZE];
	
      EM_LOG_DEBUG("dt_init_task_info"); 

	  /* 目前不能存放大于255的文件路径和文件名 */
	if(p_create_param->_file_path_len>255) return INVALID_FILE_PATH;
	if(p_create_param->_file_name_len>255 ) return INVALID_FILE_NAME;
	if( NULL != p_create_param->_file_name )
	{
		if( !sd_is_file_name_valid(p_create_param->_file_name) )
		{
			p_create_param->_file_name = sd_filter_valid_file_name(p_create_param->_file_name);
			p_create_param->_file_name_len = sd_strlen(p_create_param->_file_name);
		}
	}
	
	if(p_create_param->_type==TT_BT)
	{
		ret_val=dt_init_bt_task_info(p_create_param,&p_task_info,eigenvalue,p_task_id );
		if( ret_val!=SUCCESS) return ret_val;
	}
	else
	{
		ret_val=dt_init_p2sp_task_info(p_create_param,&p_task_info,eigenvalue,p_task_id, create_by_cfg);
		if( ret_val!=SUCCESS) return ret_val;
	}
	
	p_task_info->_type = p_create_param->_type;
	p_task_info->_check_data= p_create_param->_check_data;
	p_task_info->_full_info = TRUE;
	em_memcpy(p_task_info->_eigenvalue, eigenvalue, CID_SIZE);

	*pp_task_info=p_task_info;

	return SUCCESS;
	
}

_int32 dt_uninit_task_info(TASK_INFO * p_task_info)
{
      EM_LOG_DEBUG("dt_uninit_task_info"); 
	if(p_task_info->_full_info == FALSE)
	{
		dt_task_info_free(p_task_info);
	}
	else
	{
		if(p_task_info->_type==TT_BT)
		{
			dt_uninit_bt_task_info((EM_BT_TASK *) p_task_info);
		}
		else
		{
			dt_uninit_p2sp_task_info((EM_P2SP_TASK *) p_task_info);
		}
	}
	return SUCCESS;
}
_int32 dt_init_bt_task_info(CREATE_TASK * p_create_param,TASK_INFO ** pp_task_info,_u8 *eigenvalue,_u32* p_task_id )
{
#ifdef ENABLE_BT
	_int32 ret_val = SUCCESS;
	TASK_INFO * p_task_info = NULL;
	EM_BT_TASK * p_bt_task = NULL;
	char default_path[MAX_FULL_PATH_BUFFER_LEN];
	//P2SP_TASK * p_p2sp_task = NULL;
	_u16 i=0;
	TORRENT_SEED_INFO *p_seed_info;
	_u32 encoding_mode = 2;
	USER_DATA_STORE_HEAD user_data_head;
	USER_DATA_ITEM_HEAD user_data_item_head;

      EM_LOG_DEBUG("dt_init_bt_task_info"); 

	if((p_create_param->_seed_file_full_path==NULL)||(em_strlen(p_create_param->_seed_file_full_path)==0)||(p_create_param->_seed_file_full_path_len==0)||(p_create_param->_seed_file_full_path_len>=MAX_FULL_PATH_LEN))
	{
		return INVALID_SEED_FILE;
	}
	
	em_memset(default_path, 0, MAX_FULL_PATH_BUFFER_LEN);
	em_strncpy(default_path, p_create_param->_seed_file_full_path, p_create_param->_seed_file_full_path_len);
	em_settings_get_int_item("system.encoding_mode", (_int32*)&encoding_mode);
	ret_val= tp_get_seed_info( default_path,encoding_mode, &p_seed_info );
	if(ret_val!=SUCCESS) return ret_val;

	em_memset(eigenvalue,0,CID_SIZE);
	em_memcpy(eigenvalue,p_seed_info->_info_hash,CID_SIZE);	

	if(dt_is_task_exist(p_create_param->_type,eigenvalue,p_task_id)==TRUE)
	{
		ret_val = TASK_ALREADY_EXIST;
		goto ErrorHanle;
	}
	
	ret_val = dt_bt_task_malloc(&p_bt_task);
	if(ret_val!=SUCCESS) 
	{
		goto ErrorHanle;
	}
	p_task_info = (TASK_INFO *)p_bt_task;

	/*  file path */
	if((p_create_param->_file_path!=NULL)&&(em_strlen(p_create_param->_file_path)!=0)&&(p_create_param->_file_path_len!=0)&&(p_create_param->_file_path_len<MAX_FILE_PATH_LEN))
	{
		ret_val = em_malloc(p_create_param->_file_path_len+1, (void**)&p_bt_task->_file_path);
		if(ret_val!=SUCCESS)
		{
			goto ErrorHanle;
		}
		em_memset(p_bt_task->_file_path, 0, p_create_param->_file_path_len+1);
		em_strncpy(p_bt_task->_file_path, p_create_param->_file_path, p_create_param->_file_path_len);
		p_task_info->_file_path_len = em_strlen(p_bt_task->_file_path);
		if(FALSE==em_file_exist(p_bt_task->_file_path))
		{
			ret_val = INVALID_FILE_PATH;
			goto ErrorHanle;
		}
	}
	else
	if(p_create_param->_file_path==NULL)
	{
		em_memset(default_path, 0, MAX_FULL_PATH_BUFFER_LEN);
		em_strncpy(default_path, DEFAULT_PATH, em_strlen(DEFAULT_PATH));
		ret_val=em_get_download_path_imp(default_path);
		//ret_val=em_settings_get_str_item("system.download_path", default_path);
		sd_assert(ret_val==SUCCESS);
		ret_val = em_malloc(em_strlen(default_path)+1, (void**)&p_bt_task->_file_path);
		if(ret_val!=SUCCESS)
		{
			goto ErrorHanle;
		}
		em_memset(p_bt_task->_file_path, 0, em_strlen(default_path)+1);
		em_strncpy(p_bt_task->_file_path, default_path, em_strlen(default_path));
		p_task_info->_file_path_len = em_strlen(p_bt_task->_file_path);
	}
	else
	{
		ret_val = INVALID_FILE_PATH;
		goto ErrorHanle;
	}

	/*  file name*/
	if((p_create_param->_file_name!=NULL)&&(em_strlen(p_create_param->_file_name)!=0)&&(p_create_param->_file_name_len!=0)&&(p_create_param->_file_name_len<MAX_FILE_NAME_LEN))
	{
		/* bt task got a user name ... discard! */
		EM_LOG_DEBUG("bt task got a user name:%s ... discard!",p_create_param->_file_name); 
	}

	EM_LOG_DEBUG("p_seed_info->_title_name[%u]:%s",p_seed_info->_title_name_len,p_seed_info->_title_name); 
	ret_val = em_malloc(p_seed_info->_title_name_len+1, (void**)&p_bt_task->_file_name);
	if(ret_val!=SUCCESS)
	{
		goto ErrorHanle;
	}
	p_task_info->_file_name_len= p_seed_info->_title_name_len;
	em_memcpy(p_bt_task->_file_name, p_seed_info->_title_name, p_task_info->_file_name_len);
	p_bt_task->_file_name[p_task_info->_file_name_len]='\0';
	p_task_info->_have_name = TRUE;
	p_task_info->_correct_file_name= TRUE;
	p_task_info->_bt_total_file_num = p_seed_info->_file_num;

	/*  user data*/
	if(p_create_param->_user_data_len!=0)
	{
		ret_val = em_malloc(p_create_param->_user_data_len+sizeof(USER_DATA_STORE_HEAD)+sizeof(USER_DATA_ITEM_HEAD), (void**)&p_bt_task->_user_data);
		if(ret_val!=SUCCESS) 
		{
			goto ErrorHanle;
		}
		em_memset(&user_data_head,0x00,sizeof(USER_DATA_STORE_HEAD));
		em_memset(&user_data_item_head,0x00,sizeof(USER_DATA_ITEM_HEAD));
		user_data_head._PACKING_1 = PACKING_1_VALUE;
		user_data_head._PACKING_2 = PACKING_2_VALUE;
		user_data_head._ver = USER_DATA_VERSION;
		user_data_head._item_num = 1;
		user_data_item_head._type = (_u16)UDIT_USER_DATA;
		user_data_item_head._size = p_create_param->_user_data_len;
		p_task_info->_user_data_len = p_create_param->_user_data_len+sizeof(USER_DATA_STORE_HEAD)+sizeof(USER_DATA_ITEM_HEAD);
		em_memcpy(p_bt_task->_user_data, &user_data_head, sizeof(USER_DATA_STORE_HEAD));
		em_memcpy(((_u8*)p_bt_task->_user_data)+sizeof(USER_DATA_STORE_HEAD), &user_data_item_head,sizeof(USER_DATA_ITEM_HEAD));
		em_memcpy(((_u8*)p_bt_task->_user_data)+sizeof(USER_DATA_STORE_HEAD)+sizeof(USER_DATA_ITEM_HEAD), p_create_param->_user_data, p_create_param->_user_data_len);
		p_task_info->_have_user_data = TRUE;
	}
	
	/* seed file */
	ret_val = em_malloc(p_create_param->_seed_file_full_path_len+1, (void**)&p_bt_task->_seed_file_path);
	if(ret_val!=SUCCESS)
	{
		goto ErrorHanle;
	}
	p_task_info->_ref_url_len_or_seed_path_len= p_create_param->_seed_file_full_path_len;
	em_memcpy(p_bt_task->_seed_file_path, p_create_param->_seed_file_full_path, p_task_info->_ref_url_len_or_seed_path_len);
	p_bt_task->_seed_file_path[p_task_info->_ref_url_len_or_seed_path_len]='\0';

	/* file indx array*/
	if((p_create_param->_download_file_index_array!=NULL)&&(p_create_param->_file_num!=0))
	{
		ret_val = dt_check_and_sort_bt_file_index(p_create_param->_download_file_index_array,p_create_param->_file_num,p_seed_info->_file_num,&p_bt_task->_dl_file_index_array,&p_task_info->_url_len_or_need_dl_num);
		if(ret_val!=SUCCESS)
		{
			goto ErrorHanle;
		}
		
	}
	else if(p_create_param->_download_file_index_array==NULL)
	{
		ret_val = dt_get_all_bt_file_index(p_seed_info,&p_bt_task->_dl_file_index_array,&p_task_info->_url_len_or_need_dl_num);
		if(ret_val!=SUCCESS)
		{
			goto ErrorHanle;
		}
	}
	else
	{
		/* no need download file */
		ret_val = EM_INVALID_FILE_INDEX;
		goto ErrorHanle;
	}

	ret_val = em_malloc(p_task_info->_url_len_or_need_dl_num*(sizeof(BT_FILE)), (void**)&p_bt_task->_file_array);
	if(ret_val!=SUCCESS)
	{
		goto ErrorHanle;
	}
	em_memset(p_bt_task->_file_array,0,p_task_info->_url_len_or_need_dl_num*(sizeof(BT_FILE)));
		
	for(i=0;i<p_task_info->_url_len_or_need_dl_num;i++)
	{
		p_bt_task->_file_array[i]._file_index = p_bt_task->_dl_file_index_array[i];
		//p_bt_task->_file_array[i]._need_download = TRUE;
		p_bt_task->_file_array[i]._file_size = p_seed_info->_file_info_array_ptr[p_bt_task->_file_array[i]._file_index]->_file_size;
		p_task_info->_file_size+=p_bt_task->_file_array[i]._file_size ;
	}

	tp_release_seed_info( p_seed_info );
	
	*pp_task_info=p_task_info;
	return  SUCCESS;
	
ErrorHanle:
	tp_release_seed_info( p_seed_info );
	
	if(p_bt_task!=NULL)
		dt_uninit_bt_task_info( p_bt_task);
	
	//CHECK_VALUE(ret_val);
	return ret_val;
#else 
	return -1;
#endif
}
_int32 dt_uninit_bt_task_info(EM_BT_TASK * p_bt_task)
{
#ifdef ENABLE_BT
	
      EM_LOG_DEBUG("dt_uninit_bt_task_info"); 
		EM_SAFE_DELETE(p_bt_task->_file_path);
		EM_SAFE_DELETE(p_bt_task->_file_name);
		EM_SAFE_DELETE(p_bt_task->_user_data);
		EM_SAFE_DELETE(p_bt_task->_seed_file_path);
		EM_SAFE_DELETE(p_bt_task->_dl_file_index_array);
		EM_SAFE_DELETE(p_bt_task->_file_array);
		dt_remove_task_info_from_cache((TASK_INFO * )p_bt_task);
		dt_bt_task_free(p_bt_task);
#endif
	return SUCCESS;
}


char * dt_get_file_name_from_url(char * url,_u32 url_length)
{
	_int32 ret_val = SUCCESS;
	char *file_name=NULL;
	char * name=NULL;
	_u32 name_len = 0;

	file_name =em_get_file_name_from_url( url, url_length);
	if(file_name==NULL) return NULL;
	
	name_len = em_strlen(file_name);
	if(name_len==0) return NULL;
	
	ret_val = em_malloc(name_len+16, (void**)&name);
	if(ret_val!=SUCCESS)
	{
		return NULL;
	}

	em_strncpy(name, file_name, name_len);
	name[name_len]='\0';

	EM_LOG_DEBUG("dt_get_file_name_from_url:%s",name); 
	return name;
}

BOOL is_file_exist(char * file_path,_u32 path_len,char * file_name,_u32 name_len, BOOL create_by_cfg)
{
	char full_path[MAX_FULL_PATH_BUFFER_LEN];
	em_memset(full_path, 0, MAX_FULL_PATH_BUFFER_LEN);
	em_strncpy(full_path, file_path, path_len);
	if(file_path[path_len-1]!='/')
	{
		em_strcat(full_path, "/", 1);
	}
	em_strcat(full_path, file_name, name_len);
	if( em_file_exist(full_path))
	{
		return TRUE;
	}
	/* create_by_cfg: TRUE表示通过cfg创建任务，.cfg文件创建任务时不需要判断.rf文件是否存在,
	普通创建任务方式则需要判断下载路径下是否有.rf文件存在，以便去重
	*/
	if(create_by_cfg)
		return FALSE;
	else
	{
		em_strcat(full_path, ".rf", 3);
		return em_file_exist(full_path);
	}
}
static char* num[10]={"0","1","2","3","4","5","6","7","8","9"};
_int32 correct_file_name_if_exist(EM_P2SP_TASK * p_p2sp_task)
{
	_int32 ret_val = SUCCESS;
	TASK_INFO *p_task_info =&p_p2sp_task->_task_info;
	char * p_pos = NULL;
	char suffix[MAX_SUFFIX_LEN];
	_u32 count = 0;

	em_memset(suffix,0x00,MAX_SUFFIX_LEN);
CHECK_AGAIN:
	if(count==9) return FILE_ALREADY_EXIST;
	if(dt_is_file_exist(p_task_info->_file_name_eigenvalue)==TRUE)
	{
		p_pos = em_strrchr(p_p2sp_task->_file_name,'.');
		if((p_pos!=0)&&(em_strlen(p_pos)<MAX_SUFFIX_LEN-1))
		{
			if(suffix[0]=='\0')
			{
				em_strncpy(suffix,p_pos,MAX_SUFFIX_LEN-1);
			}

			if(count!=0) 
				*(p_pos-1)='\0';
			else
				*p_pos='\0';
		}
		
		em_strcat(p_p2sp_task->_file_name,num[count++],1);
		
		if(suffix[0]!='\0')
		{
			em_strcat(p_p2sp_task->_file_name,suffix,em_strlen(suffix));
		}
		p_task_info->_file_name_len = em_strlen(p_p2sp_task->_file_name);
		ret_val = dt_generate_file_name_eigenvalue(p_p2sp_task->_file_path,p_task_info->_file_path_len,p_p2sp_task->_file_name,p_task_info->_file_name_len,&p_task_info->_file_name_eigenvalue);
		CHECK_VALUE(ret_val);
		goto CHECK_AGAIN;
	}

	if(is_file_exist(p_p2sp_task->_file_path,p_task_info->_file_path_len,p_p2sp_task->_file_name,p_task_info->_file_name_len, FALSE)==TRUE)
	{
		p_pos = em_strrchr(p_p2sp_task->_file_name,'.');
		if((p_pos!=0)&&(em_strlen(p_pos)<MAX_SUFFIX_LEN-1))
		{
			if(suffix[0]=='\0')
			{
				em_strncpy(suffix,p_pos,MAX_SUFFIX_LEN-1);
			}

			if(count!=0) 
				*(p_pos-1)='\0';
			else
				*p_pos='\0';
		}
		em_strcat(p_p2sp_task->_file_name,num[count++],1);
		em_strcat(p_p2sp_task->_file_name,suffix,em_strlen(suffix));
		p_task_info->_file_name_len = em_strlen(p_p2sp_task->_file_name);
		ret_val = dt_generate_file_name_eigenvalue(p_p2sp_task->_file_path,p_task_info->_file_path_len,p_p2sp_task->_file_name,p_task_info->_file_name_len,&p_task_info->_file_name_eigenvalue);
		CHECK_VALUE(ret_val);
		goto CHECK_AGAIN;
	}
	return SUCCESS;
}

_int32 em_url_down_case(char *url)
{
	char *header = em_strstr(url,"HTTP",0);
	if(header!=NULL&&header==url)
	{
		header[0]='h';
		header[1]='t';
		header[2]='t';
		header[3]='p';
		return 0;
	}
	
	header = em_strstr(url,"THUNDER",0);
	if(header!=NULL&&header==url)
	{
		header[0]='t';
		header[1]='h';
		header[2]='u';
		header[3]='n';
		header[4]='d';
		header[5]='e';
		header[6]='r';
		return 0;
	}
	
	header = em_strstr(url,"ED2K",0);
	if(header!=NULL&&header==url)
	{
		header[0]='e';
		header[1]='d';
		header[2]='2';
		header[3]='k';
		return 0;
	}
	
	return 0;
}

/* 如果迅雷专用链是有电驴链接封装而来的话，需要转化为电驴任务 */
_int32 dt_thunder_url_to_emule_url(CREATE_TASK * p_create_param)
{
	/* thunder://QUFlZDJrOi8vfGZpbGV8dGhlLmJpZy5iYW5nLnRoZW9yeS42MDEuaGR0di1sb2wubXA0fDE2NTI5NjQxOHxDQ0VEREU1ODczMUYyQzVCRUYxODNENDAyMkMxMzMzNHxoPU9RMjNNNVRZRElETFhGRUpXUElDS0hHTElXSjZYNkRJfC9aWg== */
	if((p_create_param->_type == TT_URL)&&(0==sd_strncmp(p_create_param->_url, "thunder://", sd_strlen("thunder://"))))
	{
		char url_buffer[MAX_URL_LEN]={0};
		char *url = p_create_param->_url;
		
		/* Decode to normal URL */
		if ('/' == url[sd_strlen(url)-1])
		{
			url[sd_strlen(url)-1] = '\0';
			p_create_param->_url_len--;
		}
		
		if( sd_base64_decode(url+10,(_u8 *)url_buffer,NULL)!=0)
		{
			EM_LOG_DEBUG("dt_thunder_url_to_emule_url:ERROR:DT_ERR_INVALID_URL");
			return  INVALID_URL;
		}

		/* ed2k://|file|the.big.bang.theory.601.hdtv-lol.mp4|165296418|CCEDDE58731F2C5BEF183D4022C13334|h=OQ23M5TYDIDLXFEJWPICKHGLIWJ6X6DI|/  */
		
		if(0==sd_strncmp(url_buffer+2, "ed2k://", sd_strlen("ed2k://")))
		{
			EM_LOG_DEBUG("dt_thunder_url_to_emule_url:change task type to emule");
			url_buffer[sd_strlen(url_buffer)-2]='\0';
			sd_strncpy(url,url_buffer+2,MAX_URL_LEN);
			p_create_param->_url_len= sd_strlen(url);
			p_create_param->_type = TT_EMULE;
		}
	}
	return SUCCESS;
}
_int32 dt_init_p2sp_task_info(CREATE_TASK * p_create_param,TASK_INFO ** pp_task_info,_u8 *eigenvalue,_u32* p_task_id , BOOL create_by_cfg)
{
	_int32 ret_val = SUCCESS;
	TASK_INFO * p_task_info = NULL;
	//BT_TASK * p_bt_task = NULL;
	EM_P2SP_TASK * p_p2sp_task = NULL;
	char default_path[MAX_FILE_PATH_LEN] = {0};
	USER_DATA_STORE_HEAD user_data_head;
	USER_DATA_ITEM_HEAD user_data_item_head;

	ret_val = dt_thunder_url_to_emule_url(p_create_param);
	CHECK_VALUE(ret_val);
	
	ret_val = dt_generate_eigenvalue(p_create_param,eigenvalue);
	if(ret_val!=0) return ret_val;

	str2hex((const char *)eigenvalue, CID_SIZE, default_path, MAX_FILE_PATH_LEN-1);
  	EM_LOG_DEBUG("dt_init_p2sp_task_info:type=%d,url[%u]=%s,eigenvalue=%s",p_create_param->_type,p_create_param->_url_len,p_create_param->_url,default_path); 

	if(dt_is_task_exist(p_create_param->_type,eigenvalue,p_task_id)==TRUE)
	{
		ret_val = TASK_ALREADY_EXIST;
		return ret_val;
	}

	ret_val = dt_p2sp_task_malloc(&p_p2sp_task);
	CHECK_VALUE(ret_val);
	p_task_info = (TASK_INFO*)p_p2sp_task;

	/*  file path */
	if((p_create_param->_file_path!=NULL)&&(em_strlen(p_create_param->_file_path)!=0)&&(p_create_param->_file_path_len!=0)&&(p_create_param->_file_path_len<MAX_FILE_PATH_LEN))
	{
		ret_val = em_malloc(p_create_param->_file_path_len+1, (void**)&p_p2sp_task->_file_path);
		if(ret_val!=SUCCESS)
		{
			goto ErrorHanle;
		}
		em_memset(p_p2sp_task->_file_path, 0, p_create_param->_file_path_len+1);
		em_strncpy(p_p2sp_task->_file_path, p_create_param->_file_path, p_create_param->_file_path_len);
		p_task_info->_file_path_len = em_strlen(p_p2sp_task->_file_path);
		if(!p_create_param->_check_data && FALSE==em_file_exist(p_p2sp_task->_file_path)) //无盘点播不判断目录是否存在
		{
			ret_val = INVALID_FILE_PATH;
			goto ErrorHanle;
		}
	}
	else if(p_create_param->_file_path==NULL)
	{
		em_memset(default_path, 0, MAX_FILE_PATH_LEN);
		em_strncpy(default_path, DEFAULT_PATH, em_strlen(DEFAULT_PATH));
		ret_val=em_get_download_path_imp(default_path);
		//ret_val=em_settings_get_str_item("system.download_path", default_path);
		sd_assert(ret_val==SUCCESS);
		ret_val = em_malloc(em_strlen(default_path)+1, (void**)&p_p2sp_task->_file_path);
		if(ret_val!=SUCCESS)
		{
			goto ErrorHanle;
		}
		em_memset(p_p2sp_task->_file_path, 0, em_strlen(default_path)+1);
		em_strncpy(p_p2sp_task->_file_path, default_path, em_strlen(default_path));
		p_task_info->_file_path_len = em_strlen(p_p2sp_task->_file_path);
	}
	else
	{
		ret_val = INVALID_FILE_PATH;
		goto ErrorHanle;
	}

	/*  file name*/
	if((p_create_param->_file_name!=NULL)&&(em_strlen(p_create_param->_file_name)!=0)&&(p_create_param->_file_name_len!=0)&&(p_create_param->_file_name_len<MAX_FILE_NAME_LEN))
	{
		ret_val = em_malloc(p_create_param->_file_name_len+1, (void**)&p_p2sp_task->_file_name);
		if(ret_val!=SUCCESS)
		{
			goto ErrorHanle;
		}
		em_memset(p_p2sp_task->_file_name, 0, p_create_param->_file_name_len+1);
		em_strncpy(p_p2sp_task->_file_name, p_create_param->_file_name, p_create_param->_file_name_len);
		p_task_info->_file_name_len = em_strlen(p_p2sp_task->_file_name);
		
		ret_val = dt_generate_file_name_eigenvalue(p_p2sp_task->_file_path,p_task_info->_file_path_len,p_p2sp_task->_file_name,p_task_info->_file_name_len,&p_task_info->_file_name_eigenvalue);
		if(ret_val!=SUCCESS)
		{
			goto ErrorHanle;
		}
		
		if(dt_is_file_exist(p_task_info->_file_name_eigenvalue)==TRUE)
		{
			ret_val=FILE_ALREADY_EXIST;
			goto ErrorHanle;
		}

		if(p_create_param->_type!=TT_FILE)
		{
			if(is_file_exist(p_p2sp_task->_file_path,p_task_info->_file_path_len,p_p2sp_task->_file_name,p_task_info->_file_name_len, create_by_cfg)==TRUE)
			{
				ret_val=FILE_ALREADY_EXIST;
				goto ErrorHanle;
			}
		}
		p_task_info->_have_name = TRUE;
	}
	else if(p_create_param->_file_name==NULL)
	{
		/* creating a url task without user name! */
		EM_LOG_DEBUG("creating a  task without user name!"); 
		if((p_create_param->_type==TT_URL)||(p_create_param->_type==TT_EMULE)||(p_create_param->_type==TT_LAN))
		{
			p_p2sp_task->_file_name=dt_get_file_name_from_url(p_create_param->_url,p_create_param->_url_len);
			if(p_p2sp_task->_file_name==NULL)
			{
				ret_val = INVALID_URL;
				goto ErrorHanle;
			}
			p_task_info->_file_name_len = em_strlen(p_p2sp_task->_file_name);
		}
		else if((p_create_param->_type==TT_TCID)||(p_create_param->_type==TT_KANKAN))
		{
			ret_val = em_malloc(CID_SIZE*2+16, (void**)&p_p2sp_task->_file_name);
			if(ret_val!=SUCCESS)
			{
				goto ErrorHanle;
			}
			em_memset(p_p2sp_task->_file_name, 0x00, CID_SIZE*2+16);
			if(p_create_param->_type==TT_TCID)
			{
				em_memcpy(p_p2sp_task->_file_name, p_create_param->_tcid, CID_SIZE*2);
			}
			else
			{
				em_memcpy(p_p2sp_task->_file_name, p_create_param->_gcid, CID_SIZE*2);
			}
			
			p_task_info->_file_name_len = CID_SIZE*2;
		}
		else
		{
			ret_val = INVALID_FILE_NAME;
			goto ErrorHanle;
		}
		
		ret_val = dt_generate_file_name_eigenvalue(p_p2sp_task->_file_path,p_task_info->_file_path_len,p_p2sp_task->_file_name,p_task_info->_file_name_len,&p_task_info->_file_name_eigenvalue);
		if(ret_val!=SUCCESS)
		{
			goto ErrorHanle;
		}
		ret_val=correct_file_name_if_exist(p_p2sp_task);
		if(ret_val!=SUCCESS)
		{
			goto ErrorHanle;
		}

		p_task_info->_have_name = FALSE;
	}
	else
	{
		ret_val = INVALID_FILE_NAME;
		goto ErrorHanle;
	}

	/*  user_data */
	if(p_create_param->_user_data_len!=0)
	{
		EM_LOG_DEBUG("dt_init_p2sp_task_info : userdata_len = %d, userdata = %s", p_create_param->_user_data_len, p_create_param->_user_data); 
		ret_val = em_malloc(p_create_param->_user_data_len+sizeof(USER_DATA_STORE_HEAD)+sizeof(USER_DATA_ITEM_HEAD), (void**)&p_p2sp_task->_user_data);
		if(ret_val!=SUCCESS) 
		{
			goto ErrorHanle;
		}
		em_memset(&user_data_head,0x00,sizeof(USER_DATA_STORE_HEAD));
		em_memset(&user_data_item_head,0x00,sizeof(USER_DATA_ITEM_HEAD));
		user_data_head._PACKING_1 = PACKING_1_VALUE;
		user_data_head._PACKING_2 = PACKING_2_VALUE;
		user_data_head._ver = USER_DATA_VERSION;
		user_data_head._item_num = 1;
		user_data_item_head._type = (_u16)UDIT_USER_DATA;
		user_data_item_head._size = p_create_param->_user_data_len;
		p_task_info->_user_data_len = p_create_param->_user_data_len+sizeof(USER_DATA_STORE_HEAD)+sizeof(USER_DATA_ITEM_HEAD);
		em_memcpy(p_p2sp_task->_user_data, &user_data_head, sizeof(USER_DATA_STORE_HEAD));
		em_memcpy(((_u8*)p_p2sp_task->_user_data)+sizeof(USER_DATA_STORE_HEAD), &user_data_item_head,sizeof(USER_DATA_ITEM_HEAD));
		em_memcpy(((_u8*)p_p2sp_task->_user_data)+sizeof(USER_DATA_STORE_HEAD)+sizeof(USER_DATA_ITEM_HEAD), p_create_param->_user_data, p_create_param->_user_data_len);
		p_task_info->_have_user_data = TRUE;
	}
		
	if((p_create_param->_type==TT_URL)||(p_create_param->_type==TT_EMULE)||(p_create_param->_type==TT_LAN))
	{
		/* url */
		if((p_create_param->_url!=NULL)&&(em_strlen(p_create_param->_url)!=0)&&(p_create_param->_url_len!=0)&&(p_create_param->_url_len<MAX_URL_LEN))
		{
			ret_val = em_malloc(p_create_param->_url_len+1, (void**)&p_p2sp_task->_url);
			if(ret_val!=SUCCESS)
			{
				goto ErrorHanle;
			}
			p_task_info->_url_len_or_need_dl_num= p_create_param->_url_len;
			em_memcpy(p_p2sp_task->_url, p_create_param->_url, p_create_param->_url_len);
			p_p2sp_task->_url[p_create_param->_url_len]='\0';
			em_url_down_case(p_p2sp_task->_url);
			
			if((p_create_param->_type==TT_URL)&&(p_create_param->_file_size==0))
			{
				char file_name_tmp[MAX_FILE_NAME_BUFFER_LEN]={0};
				_u8 gcid_tmp[CID_SIZE];
				_u8 cid_tmp[CID_SIZE];
				sd_memset(gcid_tmp,0,CID_SIZE);
				sd_memset(cid_tmp,0,CID_SIZE);
				sd_memset(file_name_tmp,0,MAX_FILE_NAME_LEN);
				/* 尝试从看看url中获得文件大小 */
				if(sd_parse_kankan_vod_url(p_p2sp_task->_url,p_create_param->_url_len ,gcid_tmp,cid_tmp, &p_create_param->_file_size,file_name_tmp)==SUCCESS)
				{
					/* 获取成功 */
					EM_LOG_DEBUG("Get filesize=%llu from kankan vod url!",p_create_param->_file_size); 
				}
			}
		}
		else
		{
			/* no url */
			ret_val = INVALID_URL;
			goto ErrorHanle;
		}

		/* ref url */
		if((p_create_param->_ref_url!=NULL)&&(em_strlen(p_create_param->_ref_url)!=0)&&(p_create_param->_ref_url_len!=0)&&(p_create_param->_ref_url_len<MAX_URL_LEN))
		{
			ret_val = em_malloc(p_create_param->_ref_url_len+1, (void**)&p_p2sp_task->_ref_url);
			if(ret_val!=SUCCESS)
			{
				goto ErrorHanle;
			}
			p_task_info->_ref_url_len_or_seed_path_len= p_create_param->_ref_url_len;
			em_memcpy(p_p2sp_task->_ref_url, p_create_param->_ref_url, p_create_param->_ref_url_len);
			p_p2sp_task->_ref_url[p_create_param->_ref_url_len]='\0';
			p_task_info->_have_ref_url = TRUE;
		}

		/* with tcid */
		if(em_string_to_cid(p_create_param->_tcid,p_p2sp_task->_tcid)==0 && sd_is_cid_valid(p_p2sp_task->_tcid) )
		{
			if(dt_is_same_cid_task_exist(p_create_param->_tcid))
			{
				ret_val = TASK_ALREADY_EXIST;
				goto ErrorHanle;
			}
			p_task_info->_have_tcid = TRUE;
		}
	}
	else if(p_create_param->_type==TT_KANKAN)
	{
		/* gcid */
		if(em_string_to_cid(p_create_param->_tcid,p_p2sp_task->_tcid)!=0)
		{
			ret_val = INVALID_TCID;
			goto ErrorHanle;
		}
		p_task_info->_have_tcid = TRUE;
		
		if(p_create_param->_file_size==0)
		{
			ret_val = EM_INVALID_FILE_SIZE;
			goto ErrorHanle;
		}
	}
		
	p_task_info->_file_size= p_create_param->_file_size;
	// 从本地添加的任务和从cfg临时文件创建任务时，不需要判断是否有磁盘空间
	if(p_create_param->_type == TT_FILE || create_by_cfg)
	{
		EM_LOG_DEBUG("Do not need to check free disk!"); 
	}
	else
	{
		if(p_task_info->_file_size!=0&&!p_create_param->_check_data)		//有盘点播任务
		{
			ret_val =  sd_check_enough_free_disk(p_p2sp_task->_file_path,p_task_info->_file_size/1024);
			if(ret_val != SUCCESS )
			{	
				goto ErrorHanle;
			}
		}
	}

	
#ifdef ENABLE_ETM_MEDIA_CENTER
		if (p_create_param->_product_id != 0)
		{
			p_task_info->_groupid = p_create_param->_group_id;
			sd_memset(p_task_info->_group_name, 0, EM_MAX_GROUP_NAME_LEN);
			sd_memcpy(p_task_info->_group_name, p_create_param->_group_name, 
				MIN(EM_MAX_GROUP_NAME_LEN-1, p_create_param->_group_name_len));
			p_task_info->_group_type = p_create_param->_group_type;
			p_task_info->_productid = p_create_param->_product_id;
			p_task_info->_subid = p_create_param->_sub_id;
		}
#endif		


	*pp_task_info = p_task_info;
	return  SUCCESS;
	
ErrorHanle:
	dt_uninit_p2sp_task_info(p_p2sp_task);
//	CHECK_VALUE(ret_val);
	return ret_val;
}
_int32 dt_uninit_p2sp_task_info(EM_P2SP_TASK * p_p2sp_task)
{
      EM_LOG_DEBUG("dt_uninit_p2sp_task_info"); 
		EM_SAFE_DELETE(p_p2sp_task->_file_path);
		EM_SAFE_DELETE(p_p2sp_task->_file_name);
		EM_SAFE_DELETE(p_p2sp_task->_user_data);
		EM_SAFE_DELETE(p_p2sp_task->_url);
		EM_SAFE_DELETE(p_p2sp_task->_ref_url);
		dt_remove_task_info_from_cache((TASK_INFO * )p_p2sp_task);
		dt_p2sp_task_free(p_p2sp_task);
	return SUCCESS;
}

_int32 dt_init_task(TASK_INFO * p_task_info, EM_TASK ** pp_task)
{
	_int32 ret_val = SUCCESS;
	EM_TASK * p_task;
	
      EM_LOG_DEBUG("dt_init_task"); 
	ret_val = dt_task_malloc(&p_task);
	CHECK_VALUE(ret_val);

	p_task->_task_info = p_task_info;

	*pp_task = p_task;
	
	return SUCCESS;
}
_int32 dt_uninit_task(EM_TASK * p_task)
{
      EM_LOG_DEBUG("dt_uninit_task"); 
	dt_task_free(p_task);
	return SUCCESS;
}





BOOL dt_is_task_exist(EM_TASK_TYPE type,_u8 * eigenvalue,_u32 * task_id)
{
      EM_LOG_DEBUG("dt_is_task_exist"); 
	switch(type)
	{
		case TT_BT:
			return dt_is_bt_task_exist( eigenvalue,task_id);
			break;
		case TT_URL:
		case TT_EMULE:
			return dt_is_url_task_exist( eigenvalue,task_id);
			break;
		case TT_KANKAN:
			return dt_is_kankan_task_exist( eigenvalue,task_id);
			break;
		case TT_TCID:
		case TT_LAN:
			return dt_is_tcid_task_exist( eigenvalue,task_id);
			break;
		case TT_FILE:
			return dt_is_file_task_exist( eigenvalue,task_id);
			break;
		default:
				sd_assert(FALSE);
		}

	return FALSE;
}

_int32 dt_add_task_eigenvalue(EM_TASK_TYPE type,_u8 * eigenvalue,_u32 task_id)
{
      EM_LOG_DEBUG("dt_add_task_eigenvalue, type = %d, eigenvalue = %s, task_id = %u", type, eigenvalue, task_id); 
	switch(type)
	{
		case TT_BT:
			return dt_add_bt_task_eigenvalue( eigenvalue,task_id);
			break;
		case TT_URL:
		case TT_EMULE:
			return dt_add_url_task_eigenvalue( eigenvalue,task_id);
			break;
		case TT_KANKAN:
			return dt_add_kankan_task_eigenvalue( eigenvalue,task_id);
			break;
		case TT_TCID:
		case TT_LAN:
			return dt_add_tcid_task_eigenvalue( eigenvalue,task_id);
			break;
		case TT_FILE:
			return dt_add_file_task_eigenvalue( eigenvalue,task_id);
			break;
		default:
				sd_assert(FALSE);
		}
	return SUCCESS;
}


_int32 dt_remove_task_eigenvalue(EM_TASK_TYPE type,_u8 * eigenvalue)
{
      EM_LOG_DEBUG("dt_remove_task_eigenvalue"); 
	switch(type)
	{
		case TT_BT:
			return dt_remove_bt_task_eigenvalue( eigenvalue);
			break;
		case TT_URL:
		case TT_EMULE:
			return dt_remove_url_task_eigenvalue( eigenvalue);
			break;
		case TT_KANKAN:
			return dt_remove_kankan_task_eigenvalue( eigenvalue);
			break;
		case TT_TCID:
		case TT_LAN:
			return dt_remove_tcid_task_eigenvalue( eigenvalue);
			break;
		case TT_FILE:
			return dt_remove_file_task_eigenvalue( eigenvalue);
			break;
		default:
				sd_assert(FALSE);
		}
	return SUCCESS;
}


_int32 dt_get_task_tcid_from_et(EM_TASK * p_task)
{
	int32 ret_val=SUCCESS;
	_u8 tcid[CID_SIZE];
	
      EM_LOG_DEBUG("dt_get_task_tcid_from_et:task_id=%u",p_task->_task_info->_task_id); 
	if( p_task->_task_info->_type!=TT_URL)
		return SUCCESS;

	ret_val= eti_get_task_tcid(p_task->_inner_id,tcid);
	if(ret_val!=SUCCESS) return ret_val;

	ret_val= dt_set_p2sp_task_tcid(p_task,tcid);
	CHECK_VALUE(ret_val);

	return SUCCESS;	
}

_int32  dt_set_p2sp_task_tcid(EM_TASK * p_task,_u8 * tcid)
{
	//_int32 ret_val = SUCCESS;
	EM_P2SP_TASK * p_p2sp_task = NULL;
      EM_LOG_DEBUG("dt_set_p2sp_task_tcid:task_id=%u",p_task->_task_info->_task_id); 

	if(p_task->_task_info->_full_info)
	{
		p_p2sp_task = (EM_P2SP_TASK *)p_task->_task_info;
		em_memcpy(p_p2sp_task->_tcid,tcid,CID_SIZE);
		p_task->_task_info->_have_tcid = TRUE;
	}

	return dt_save_p2sp_task_tcid_to_file(p_task,tcid); 
}
_int32  dt_set_p2sp_task_url(EM_TASK * p_task,char * new_url,_u32 url_len)
{
	_int32 ret_val = SUCCESS;
	EM_P2SP_TASK * p_p2sp_task = NULL;
      EM_LOG_DEBUG("dt_set_p2sp_task_url:task_id=%u,%s",p_task->_task_info->_task_id,new_url); 
	  
	if(TT_LAN!=dt_get_task_type(p_task))
	{
		sd_assert(FALSE);
		return INVALID_TASK_TYPE;
	}

	if(p_task->_task_info->_full_info)
	{
		p_p2sp_task = (EM_P2SP_TASK *)p_task->_task_info;
		if(p_task->_task_info->_url_len_or_need_dl_num< url_len)
		{
			EM_SAFE_DELETE(p_p2sp_task->_url);
			p_task->_task_info->_url_len_or_need_dl_num = 0;
			ret_val = em_malloc(url_len+1,(void**)&p_p2sp_task->_url);
			CHECK_VALUE(ret_val);
			em_memset(p_p2sp_task->_url,0,url_len+1);
		}
		else
		{
			em_memset(p_p2sp_task->_url,0,p_task->_task_info->_url_len_or_need_dl_num+1);
		}

		em_memcpy(p_p2sp_task->_url,new_url,url_len);
		p_task->_task_info->_url_len_or_need_dl_num = url_len;
	}

	return dt_save_p2sp_task_url_to_file(p_task,new_url,url_len); 
}


_int32 dt_get_task_file_name_from_et(EM_TASK * p_task)
{
	int32 ret_val=SUCCESS,name_buffer_len=MAX_FILE_NAME_BUFFER_LEN;
	char name_buffer[MAX_FILE_NAME_BUFFER_LEN];
	char * point_pos = NULL;
	
      EM_LOG_DEBUG("dt_get_task_file_name_from_et:task_id=%u",p_task->_task_info->_task_id); 
	if( p_task->_task_info->_type==TT_BT)
		return SUCCESS;

	em_memset(name_buffer,0,MAX_FILE_NAME_BUFFER_LEN);
	ret_val= eti_get_task_file_name(p_task->_inner_id, name_buffer,&name_buffer_len);
	CHECK_VALUE(ret_val);

	point_pos = em_strrchr(name_buffer, '.');
	if((point_pos!=NULL)&&(em_stricmp(point_pos, ".rf")==0))
	{
		ret_val= dt_set_task_name(p_task,name_buffer,em_strlen(name_buffer)-3);
		CHECK_VALUE(ret_val);
	}
	else
	{
		ret_val= dt_set_task_name(p_task,name_buffer,em_strlen(name_buffer));
		CHECK_VALUE(ret_val);
	}

	if((p_task->_task_info->_have_name==FALSE)||(p_task->_task_info->_correct_file_name==FALSE))
	{
		_u32 name_len = p_task->_task_info->_file_name_len;
		p_task->_task_info->_have_name = TRUE;
		p_task->_task_info->_correct_file_name = TRUE;
		dt_set_task_change( p_task,CHANGE_HAVE_NAME);
		dt_notify_file_name_changed(p_task->_task_info->_task_id, name_buffer,name_len);
	}
	
	return SUCCESS;	
}
_int32  dt_set_task_name(EM_TASK * p_task,char * new_name,_u32 name_len)
{
	_int32 ret_val=SUCCESS;
	EM_TASK_TYPE type;
	char * file_name = NULL;
	_u32 file_name_eigenvalue = 0;
      EM_LOG_DEBUG("dt_set_task_name:task_id=%u,%s",p_task->_task_info->_task_id,new_name); 

	if(p_task->_task_info->_file_name_len == name_len)
	{
		file_name = dt_get_task_file_name( p_task);
		if(0==em_strncmp(file_name, new_name,name_len))
		{
			return SUCCESS;
		}
	}

      EM_LOG_DEBUG("dt_set_task_name:task_id=%u,old=%s,new=%s",p_task->_task_info->_task_id,new_name,file_name); 
	  
	if(name_len>255) 
	{
		/* 目前不能存放大于255的文件名 */
		CHECK_VALUE(INVALID_FILE_NAME);
	}
	
	ret_val = dt_generate_file_name_eigenvalue(dt_get_task_file_path( p_task),p_task->_task_info->_file_path_len,new_name,name_len,&file_name_eigenvalue);
	CHECK_VALUE(ret_val);
	
	if(dt_is_file_exist(file_name_eigenvalue)==TRUE)
	{
		ret_val=FILE_ALREADY_EXIST;
      		EM_LOG_ERROR("dt_set_task_name:FILE_ALREADY_EXIST:task_id=%u,old=%s,new=%s",p_task->_task_info->_task_id,new_name,file_name); 
		CHECK_VALUE(ret_val);
	}
	
	dt_add_file_name_eigenvalue(file_name_eigenvalue,p_task->_task_info->_task_id);

	dt_remove_file_name_eigenvalue(p_task->_task_info->_file_name_eigenvalue);

	p_task->_task_info->_file_name_eigenvalue = file_name_eigenvalue;
	
	type = dt_get_task_type(p_task);
	if(TT_BT==type)
	{
		ret_val= dt_set_bt_task_name(p_task,new_name,name_len);
	}
	else
	 {
		ret_val= dt_set_p2sp_task_name(p_task,new_name,name_len);
	 }

	dt_remove_file_name_from_cache(p_task);

	return ret_val;
}
_int32  dt_set_p2sp_task_name(EM_TASK * p_task,char * new_name,_u32 name_len)
{
	_int32 ret_val = SUCCESS;
	EM_P2SP_TASK * p_p2sp_task = NULL;
      EM_LOG_DEBUG("dt_set_p2sp_task_name:task_id=%u,%s",p_task->_task_info->_task_id,new_name); 

	if(p_task->_task_info->_full_info)
	{
		p_p2sp_task = (EM_P2SP_TASK *)p_task->_task_info;
		if(p_task->_task_info->_file_name_len < name_len)
		{
			EM_SAFE_DELETE(p_p2sp_task->_file_name);
			p_task->_task_info->_file_name_len = 0;
			ret_val = em_malloc(name_len+1,(void**)&p_p2sp_task->_file_name);
			CHECK_VALUE(ret_val);
			em_memset(p_p2sp_task->_file_name,0,name_len+1);
		}
		else
		{
			em_memset(p_p2sp_task->_file_name,0,p_task->_task_info->_file_name_len+1);
		}

		em_memcpy(p_p2sp_task->_file_name,new_name,name_len);
		p_task->_task_info->_file_name_len = name_len;
		p_task->_task_info->_have_name = TRUE;
		//p_task->_task_info->_correct_file_name= TRUE;
	}

	return dt_save_p2sp_task_name_to_file(p_task,new_name,name_len); 
}
_int32  dt_set_bt_task_name(EM_TASK * p_task,char * new_name,_u32 name_len)
{
	_int32 ret_val = SUCCESS;
	EM_BT_TASK * p_bt_task = NULL;
      EM_LOG_DEBUG("dt_set_p2sp_task_name:task_id=%u,%s",p_task->_task_info->_task_id,new_name); 
	if(p_task->_task_info->_full_info)
	{
		p_bt_task = (EM_BT_TASK *)p_task->_task_info;
		if(p_task->_task_info->_file_name_len < name_len)
		{
			EM_SAFE_DELETE(p_bt_task->_file_name);
			p_task->_task_info->_file_name_len = 0;
			ret_val = em_malloc(name_len+1,(void**)&p_bt_task->_file_name);
			CHECK_VALUE(ret_val);
			em_memset(p_bt_task->_file_name,0,name_len+1);
		}
		else
		{
			em_memset(p_bt_task->_file_name,0,p_task->_task_info->_file_name_len+1);
		}

		em_memcpy(p_bt_task->_file_name,new_name,name_len);
		p_task->_task_info->_file_name_len = name_len;
		p_task->_task_info->_have_name = TRUE;
		//p_task->_task_info->_correct_file_name= TRUE;
	}

	return dt_save_bt_task_name_to_file(p_task,new_name,name_len); 
}
char * dt_get_task_file_path(EM_TASK * p_task)
{
	EM_P2SP_TASK * p_p2sp_task = NULL;
	EM_BT_TASK * p_bt_task = NULL;
	char * path = NULL;
	
		if(p_task->_task_info->_full_info)
		{
			if(p_task->_task_info->_type==TT_BT)
			{
				p_bt_task = (EM_BT_TASK*)p_task->_task_info;
				return p_bt_task->_file_path;
			}
			else
			{
				p_p2sp_task = (EM_P2SP_TASK*)p_task->_task_info;
				return p_p2sp_task->_file_path;
			}
		}
		else
		{
			path = dt_get_file_path_from_cache(p_task);
			if(path==NULL)
			{
				path = dt_get_task_file_path_from_file(p_task);
				if(path!=NULL)
				{
					dt_add_file_path_to_cache(p_task,path,p_task->_task_info->_file_path_len);
				}
			}
		}
		return path;
}
char * dt_get_task_file_name(EM_TASK * p_task)
{
	EM_P2SP_TASK * p_p2sp_task = NULL;
	EM_BT_TASK * p_bt_task = NULL;
	char * name = NULL;
	
		if(p_task->_task_info->_full_info)
		{
			if(p_task->_task_info->_type==TT_BT)
			{
				p_bt_task = (EM_BT_TASK*)p_task->_task_info;
				return p_bt_task->_file_name;
			}
			else
			{
				p_p2sp_task = (EM_P2SP_TASK*)p_task->_task_info;
				return p_p2sp_task->_file_name;
			}
		}
		else
		{
			name = dt_get_file_name_from_cache(p_task);
			if(name==NULL)
			{
				name = dt_get_task_file_name_from_file(p_task);
				if(name!=NULL)
				{
					dt_add_file_name_to_cache(p_task,name,p_task->_task_info->_file_name_len);
				}
			}
		}
		return name;
}

_int32  dt_set_task_change(EM_TASK * p_task,_u32 change_flag)
{
      EM_LOG_DEBUG("dt_set_task_change:task_id=%u,%u",p_task->_task_info->_task_id,change_flag); 
	p_task->_change_flag|=change_flag;
	dt_have_task_changed();

	return SUCCESS;
}

_int32  dt_set_task_downloaded_size(EM_TASK * p_task,_u64 downloaded_size,BOOL save_dled_size)
{
      EM_LOG_DEBUG("dt_set_task_downloaded_size:task_id=%u,%llu",p_task->_task_info->_task_id,downloaded_size); 
	p_task->_task_info->_downloaded_data_size = downloaded_size;
	if(save_dled_size)
	{
		dt_set_task_change( p_task,CHANGE_DLED_SIZE);
	}
	return SUCCESS;
}
_int32  dt_set_task_file_size(EM_TASK * p_task,_u64 file_size)
{
      EM_LOG_DEBUG("dt_set_task_file_size:task_id=%u,%llu",p_task->_task_info->_task_id,file_size); 
	p_task->_task_info->_file_size = file_size;
	dt_set_task_change( p_task,CHANGE_FILE_SIZE);
	return SUCCESS;
}
_int32  dt_set_task_start_time(EM_TASK * p_task,_u32 start_time)
{
      EM_LOG_DEBUG("dt_set_task_start_time:task_id=%u,%u",p_task->_task_info->_task_id,start_time); 
	p_task->_task_info->_start_time= start_time;
	dt_set_task_change( p_task,CHANGE_START_TIME);
	return SUCCESS;
}
_int32  dt_set_task_finish_time(EM_TASK * p_task,_u32 finish_time)
{
      EM_LOG_DEBUG("dt_set_task_finish_time:task_id=%u,%u",p_task->_task_info->_task_id,finish_time); 
	p_task->_task_info->_finished_time= finish_time;
	dt_set_task_change( p_task,CHANGE_FINISHED_TIME);
	return SUCCESS;
}
_int32  dt_set_task_failed_code(EM_TASK * p_task,_u32 failed_code)
{
      EM_LOG_ERROR("dt_set_task_failed_code:task_id=%u,%u",p_task->_task_info->_task_id,failed_code); 
	p_task->_task_info->_failed_code= failed_code;
	dt_set_task_change( p_task,CHANGE_FAILED_CODE);
	return SUCCESS;
}


_int32 dt_set_task_state(EM_TASK * p_task,TASK_STATE new_state)
{
      EM_LOG_DEBUG("dt_set_task_state:id=%u,%d",p_task->_task_info->_task_id,new_state); 

	if(new_state==TS_TASK_DELETED)
	{
		p_task->_task_info->_is_deleted=TRUE;
		dt_set_task_change(p_task,CHANGE_DELETE);
	}
	else
	{
		p_task->_task_info->_state = new_state;
		dt_set_task_change(p_task,CHANGE_STATE);
		if(new_state==TS_TASK_WAITING)
		{
			dt_have_task_waitting();
		}
	}

	/* call the call_back_function to notify the UI */
	//dt_set_task_state_changed_callback
	
	return SUCCESS;
}

TASK_STATE dt_get_task_state(EM_TASK * p_task)
{
      EM_LOG_DEBUG("dt_get_task_state:task_id=%u,state=%d,is_deleted=%d",p_task->_task_info->_task_id,p_task->_task_info->_state,p_task->_task_info->_is_deleted); 
	if(p_task->_task_info->_is_deleted)
		return TS_TASK_DELETED;
	
	return p_task->_task_info->_state;
}


EM_TASK_TYPE dt_get_task_type(EM_TASK * p_task)
{
      EM_LOG_DEBUG("dt_get_task_type:task_id=%u,type=%d",p_task->_task_info->_task_id,p_task->_task_info->_type); 
	return p_task->_task_info->_type;
	/*
	_int32 ret_val = SUCCESS;
	TASK_INFO *p_task_info=NULL;

	if(p_task->_cache_valid)
	{
		p_task_info = p_task->_task_info;
	}
	else
	{
		// read from file 
		p_task_info = ts_get_task_info_from_file(p_task);
		sd_assert(p_task_info!=NULL);
	}
	
	return p_task_info->_task_type;
	*/
}
////////////////////////////////////

_int32 dt_update_bt_sub_file_info(EM_TASK * p_task)
{
      EM_LOG_DEBUG("dt_update_bt_sub_file_info:task_id=%u",p_task->_task_info->_task_id); 
	if(p_task->_task_info->_full_info)
	{
		dt_update_bt_running_file_in_cache( p_task);
	}
	else
	{
		if(p_task->_bt_running_files == NULL)
		{
			dt_init_bt_running_file(p_task);
		}
		else
		{
			dt_update_bt_running_file(p_task);
		}
	}
	return SUCCESS;
}
_int32  dt_update_bt_running_file_in_cache(EM_TASK * p_task)
{
	//_int32 ret_val = SUCCESS;
	_u32 inner_task_id=0,change_flag=0,file_index=0;
	EM_BT_TASK * bt_task = NULL;
	BT_FILE * bt_file_array = NULL;
	ETI_BT_FILE et_bt_file_info;
	_u16 i=0,file_num = 0;
	
      EM_LOG_DEBUG("dt_update_bt_running_file_in_cache:task_id=%u",p_task->_task_info->_task_id); 
		bt_task = (EM_BT_TASK *)p_task->_task_info;
		bt_file_array = bt_task->_file_array;
		file_num = p_task->_task_info->_url_len_or_need_dl_num;
		inner_task_id = p_task->_inner_id;
		for(i=0;i<file_num;i++)
		{
			if((bt_file_array[i]._status!=FS_SUCCESS)&&(bt_file_array[i]._status!=FS_FAILED))
			{
				/* FS_IDLE  or FS_DOWNLOADING */
				em_memset(&et_bt_file_info, 0, sizeof(ETI_BT_FILE));
				file_index = bt_file_array[i]._file_index;
      				EM_LOG_DEBUG("eti_get_bt_file_info:task_id=%u,file_index=%u",p_task->_task_info->_task_id,file_index); 
				if(eti_get_bt_file_info(inner_task_id, file_index, &et_bt_file_info)==SUCCESS) 
				{
      					EM_LOG_DEBUG("eti_get_bt_file_info 1 SUCCESS:task_id=%u,file_index=%u,file_status=%d",p_task->_task_info->_task_id,file_index,et_bt_file_info._file_status); 
					if(et_bt_file_info._file_status!=0)
					{
						/* FS_DOWNLOADING , FS_SUCCESS or FS_FAILED*/
						change_flag=0;
						if(bt_file_array[i]._file_size!= et_bt_file_info._file_size)
						{
							bt_file_array[i]._file_size = et_bt_file_info._file_size;
							change_flag|=CHANGE_BT_SUB_FILE_DLED_SIZE;
						}
				
						if(bt_file_array[i]._downloaded_data_size != et_bt_file_info._downloaded_data_size)
						{
							bt_file_array[i]._downloaded_data_size = et_bt_file_info._downloaded_data_size;
							change_flag|=CHANGE_BT_SUB_FILE_DLED_SIZE;
						}
						
						if(bt_file_array[i]._status!=(_u16)et_bt_file_info._file_status)
						{
							/* status changed !*/
							if(et_bt_file_info._file_status==3)
							{
								/* sub file failed! */
								bt_file_array[i]._failed_code = et_bt_file_info._sub_task_err_code;
								change_flag|=CHANGE_BT_SUB_FILE_FAILED_CODE;
							}
							bt_file_array[i]._status =(_u16)et_bt_file_info._file_status;
							change_flag|=CHANGE_BT_SUB_FILE_STATUS;
						}
						/* Save change to file */
						if(change_flag!=0)
						{
							dt_set_task_bt_sub_file_to_file( p_task,i,&bt_file_array[i]);
						}
					}
				}
			}
		}

		return SUCCESS;
}
_int32  dt_init_bt_running_file(EM_TASK * p_task)
{
	_int32 ret_val = SUCCESS;
	_u32 inner_task_id=0,file_index=0;
	//BT_TASK * bt_task = NULL;
	BT_FILE *bt_file = NULL,*bt_file_tmp = NULL,bt_file_tmp2;
	ETI_BT_FILE et_bt_file_info;
	_u16 i=0,j=0,file_num = 0,*file_index_array=NULL;
	
      EM_LOG_DEBUG("dt_init_bt_running_file:task_id=%u",p_task->_task_info->_task_id); 
	ret_val=dt_bt_running_file_malloc(&p_task->_bt_running_files);
	CHECK_VALUE(ret_val);
	for(j=0;j<MAX_BT_RUNNING_SUB_FILE;j++)
	{
		p_task->_bt_running_files->_sub_files[j]._file_index = MAX_FILE_INDEX;
		p_task->_bt_running_files->_sub_files[j]._file_size= 1024;
		//p_task->_bt_running_files->_sub_files[j]._need_download = TRUE;
	}
	
	p_task->_bt_running_files->_need_dl_file_index_array=dt_get_task_bt_need_dl_file_index_array(p_task);
	if(p_task->_bt_running_files->_need_dl_file_index_array==NULL) 
	{
		ret_val = INVALID_FILE_INDEX_ARRAY;
		goto ErrorHanle;
	}

	file_num = p_task->_task_info->_url_len_or_need_dl_num;
	p_task->_bt_running_files->_need_dl_file_num = file_num;
	
	file_index_array = p_task->_bt_running_files->_need_dl_file_index_array;
	inner_task_id = p_task->_inner_id;

	j=0;
	for(i=0;i<file_num;i++)
	{
		em_memset(&et_bt_file_info, 0, sizeof(ETI_BT_FILE));
		file_index = file_index_array[i];
		if(eti_get_bt_file_info(inner_task_id, file_index, &et_bt_file_info)==SUCCESS)
		{
      			EM_LOG_DEBUG("eti_get_bt_file_info 2 SUCCESS:task_id=%u,file_index=%u,file_status=%d",p_task->_task_info->_task_id,file_index,et_bt_file_info._file_status); 
			if(et_bt_file_info._file_status==1)
			{
				/* FS_DOWNLOADING */
				bt_file = &p_task->_bt_running_files->_sub_files[j++];
				bt_file->_file_index = file_index_array[i];
				bt_file->_downloaded_data_size = et_bt_file_info._downloaded_data_size;
				bt_file->_status = FS_DOWNLOADING;
				
				/* Save change to file */
				dt_set_task_bt_sub_file_to_file( p_task,i,bt_file);
				
				if(j>MAX_BT_RUNNING_SUB_FILE)
				{
					/* More than MAX_BT_RUNNING_SUB_FILE sub files are downloading in et ! */
					sd_assert(FALSE);
					break;
				}
			}
			else
			if(et_bt_file_info._file_status>1)
			{
				bt_file_tmp =  dt_get_task_bt_sub_file_from_file(p_task,i);
				sd_assert(bt_file_tmp!=NULL);
				if(bt_file_tmp!=NULL)
				{
						if(bt_file_tmp->_status!=(_u16)et_bt_file_info._file_status)
						{
							/* status changed !*/
							if(et_bt_file_info._file_status==3)
							{
								/* sub file failed! */
								bt_file_tmp2._failed_code = et_bt_file_info._sub_task_err_code;
							}
							bt_file_tmp2._status =(_u16)et_bt_file_info._file_status;
							bt_file_tmp2._file_index =bt_file_tmp->_file_index;
							bt_file_tmp2._file_size=et_bt_file_info._file_size;
							bt_file_tmp2._downloaded_data_size=et_bt_file_info._downloaded_data_size;
							/* Save change to file */
							dt_set_task_bt_sub_file_to_file( p_task,i,&bt_file_tmp2);
						}
					p_task->_bt_running_files->_finished_file_num++;
      					EM_LOG_DEBUG("bt sub file finished:task_id=%u,need_dl_file_num=%u,finished_file_num=%u,file_index=%u,file_status=%d!",
							p_task->_task_info->_task_id,p_task->_bt_running_files->_need_dl_file_num,p_task->_bt_running_files->_finished_file_num,et_bt_file_info._file_index,et_bt_file_info._file_status); 
				}
			}
		}
	}
	
	return SUCCESS;
	
ErrorHanle:
	dt_bt_running_file_safe_delete(p_task);
	CHECK_VALUE(ret_val);
	return ret_val;
}
_int32  dt_update_bt_running_file(EM_TASK * p_task)
{
//	_int32 ret_val = SUCCESS;
	_u32 inner_task_id=0,change_flag=0,file_index=0;
	//BT_TASK * bt_task = NULL;
	BT_FILE * bt_file = NULL;
	ETI_BT_FILE et_bt_file_info;
	_u16  i=0,array_index=0,erase_count=0, empty_count=0,file_num = 0,*file_index_array=NULL;

      EM_LOG_DEBUG("dt_update_bt_running_file:task_id=%u",p_task->_task_info->_task_id); 
	file_num = p_task->_bt_running_files->_need_dl_file_num;
	file_index_array = p_task->_bt_running_files->_need_dl_file_index_array;
	inner_task_id = p_task->_inner_id;
	for(i=0;i<MAX_BT_RUNNING_SUB_FILE;i++)
	{
		bt_file = &p_task->_bt_running_files->_sub_files[i];
		if((bt_file->_file_index!=MAX_FILE_INDEX)&&(bt_file->_status == FS_DOWNLOADING))
		{
			em_memset(&et_bt_file_info, 0, sizeof(ETI_BT_FILE));
			file_index= bt_file->_file_index;
			if(eti_get_bt_file_info(inner_task_id, file_index, &et_bt_file_info)==SUCCESS)
			{
      				EM_LOG_DEBUG("eti_get_bt_file_info 3 SUCCESS:task_id=%u,file_index=%u,file_status=%d",p_task->_task_info->_task_id,file_index,et_bt_file_info._file_status); 
				change_flag=0;
				if(bt_file->_file_size!= et_bt_file_info._file_size)
				{
					bt_file->_file_size = et_bt_file_info._file_size;
					change_flag|=CHANGE_BT_SUB_FILE_DLED_SIZE;
				}
				
				if(bt_file->_downloaded_data_size != et_bt_file_info._downloaded_data_size)
				{
					bt_file->_downloaded_data_size = et_bt_file_info._downloaded_data_size;
					change_flag|=CHANGE_BT_SUB_FILE_DLED_SIZE;
				}

				sd_assert(et_bt_file_info._file_status!=0);
				if(et_bt_file_info._file_status!=1)
				{
					if(et_bt_file_info._file_status==3)
					{
						/* FS_FAILED */
						bt_file->_failed_code = et_bt_file_info._sub_task_err_code;
						change_flag|=CHANGE_BT_SUB_FILE_FAILED_CODE;
					}
					bt_file->_status = (_u16)et_bt_file_info._file_status;
					change_flag|=CHANGE_BT_SUB_FILE_STATUS;
					
				}

				/* Save change to file */
				if(change_flag!=0)
				{
					array_index=0;
					while((array_index<file_num)&&(file_index_array[array_index]!=bt_file->_file_index))
						array_index++;
					
					if(array_index==file_num)
					{
						CHECK_VALUE(INVALID_FILE_INDEX_ARRAY);
					}
					
					dt_set_task_bt_sub_file_to_file( p_task,array_index,bt_file);
				}

				if(et_bt_file_info._file_status>1)
				{
					/* erase from memory */
					em_memset(bt_file, 0, sizeof(BT_FILE));
					bt_file->_file_index = MAX_FILE_INDEX;
					bt_file->_file_size= 1024;
					//bt_file->_need_download= TRUE;
					erase_count++;
					p_task->_bt_running_files->_finished_file_num++;
      					EM_LOG_DEBUG("bt sub file finished:task_id=%u,need_dl_file_num=%u,finished_file_num=%u,file_index=%u,file_status=%d!",
							p_task->_task_info->_task_id,p_task->_bt_running_files->_need_dl_file_num,p_task->_bt_running_files->_finished_file_num,et_bt_file_info._file_index,et_bt_file_info._file_status); 
				}
				
			}
		}
		else
		{
			empty_count++;
		}
	}

	sd_assert(p_task->_bt_running_files->_need_dl_file_num>=p_task->_bt_running_files->_finished_file_num);
	/* find next downloading file */
	if((p_task->_bt_running_files->_need_dl_file_num>p_task->_bt_running_files->_finished_file_num)
		&&((empty_count!=0)||(erase_count!=0))
		&&(TS_TASK_RUNNING==dt_get_task_state(p_task)))
	{
		dt_find_next_bt_running_file(p_task);
	}

	return SUCCESS;
}
_int32  dt_find_next_bt_running_file(EM_TASK * p_task)
{
//	_int32 ret_val = SUCCESS;
	_u32 inner_task_id=0,file_index=0;
	//BT_TASK * bt_task = NULL;
	BT_FILE * bt_file = NULL;
	ETI_BT_FILE et_bt_file_info;
	_u16 i=0,j=0,empty_count=0,remain_file_num=0, file_num = 0,*file_index_array=NULL;

      EM_LOG_DEBUG("dt_find_next_bt_running_file:task_id=%u,%u/%u",p_task->_task_info->_task_id,p_task->_bt_running_files->_finished_file_num,p_task->_bt_running_files->_need_dl_file_num); 
	file_num = p_task->_bt_running_files->_need_dl_file_num;
	file_index_array = p_task->_bt_running_files->_need_dl_file_index_array;
	inner_task_id = p_task->_inner_id;
	remain_file_num = p_task->_bt_running_files->_need_dl_file_num-p_task->_bt_running_files->_finished_file_num;
	
	/* find how many empty file pool */
	for(j=0;j<MAX_BT_RUNNING_SUB_FILE;j++)
	{
		if(p_task->_bt_running_files->_sub_files[j]._file_index==MAX_FILE_INDEX)
		{
			empty_count++;
		}
	}
	
	if(empty_count>remain_file_num)
	{
		empty_count = remain_file_num;
	}
	
	while((empty_count>0)&&(i<file_num))
	{
		em_memset(&et_bt_file_info, 0, sizeof(ETI_BT_FILE));
		file_index = file_index_array[i];
		if(eti_get_bt_file_info(inner_task_id, file_index, &et_bt_file_info)==SUCCESS)
		{
			if(et_bt_file_info._file_status==1)
			{
				/* FS_DOWNLOADING */
				j=0;
				while((j<MAX_BT_RUNNING_SUB_FILE)&&(p_task->_bt_running_files->_sub_files[j]._file_index!=file_index_array[i]))
						j++;

				if(j>=MAX_BT_RUNNING_SUB_FILE)
				{
					/* find the new downloading file */
					j=0;
					while((j<MAX_BT_RUNNING_SUB_FILE)&&(p_task->_bt_running_files->_sub_files[j]._file_index!=MAX_FILE_INDEX))
						j++;

					sd_assert(j<MAX_BT_RUNNING_SUB_FILE);
					bt_file= &p_task->_bt_running_files->_sub_files[j];
					bt_file->_file_index = file_index_array[i];
					bt_file->_file_size= et_bt_file_info._file_size;
					bt_file->_downloaded_data_size = et_bt_file_info._downloaded_data_size;
					bt_file->_status = FS_DOWNLOADING;
					
					/* Save change to file */
					dt_set_task_bt_sub_file_to_file( p_task,i,bt_file);
					
					empty_count--;
				}
				
			}
		}
		i++;
	}

	return SUCCESS;
}

_int32 dt_correct_task_path(EM_TASK * p_task, char * path,_u32  path_len)
{
	_int32 ret_val = SUCCESS;
	char *file_path= dt_get_vod_cache_path_impl( ); 
	char * file_name = NULL;
	char * url = NULL;
	_u32 new_path_len = 0;
	_u32 file_name_eigenvalue = 0;
	_u8  eigenvalue[CID_SIZE] = {0};

	if(file_path == NULL)
	{
		sd_assert(FALSE);
		return INVALID_FILE_PATH;
	}
	
	if(em_stricmp(file_path,path)==0)
	{
	#ifdef _DEBUG
		/* 如果URL 是"http://gdl.lixian.vip.xunlei.com/download?" 开头的，需要重新生成特征值 */
		if(p_task->_task_info->_type==TT_URL)
		{
			url = dt_get_task_url_from_file(p_task);
			if(url)
			{
				if((sd_strncmp(url, "http://gdl.lixian.vip.xunlei.com/download?", sd_strlen("http://gdl.lixian.vip.xunlei.com/download?"))==0)||
					(sd_strstr(url, DEFAULT_YUNBO_URL_MARK, 0)!=NULL))
				{
					ret_val = dt_get_url_eigenvalue(url,p_task->_task_info->_url_len_or_need_dl_num,eigenvalue);
					sd_assert(ret_val==SUCCESS);
					if(!sd_is_cid_equal(eigenvalue, p_task->_task_info->_eigenvalue))
					{
						/* remove old eigenvalue */
						dt_remove_task_eigenvalue(p_task->_task_info->_type,p_task->_task_info->_eigenvalue);

						sd_memcpy(p_task->_task_info->_eigenvalue, eigenvalue, CID_SIZE);
						/* add to new eigenvalue map */
						ret_val = dt_add_task_eigenvalue(p_task->_task_info->_type,p_task->_task_info->_eigenvalue,p_task->_task_info->_task_id);
						sd_assert(ret_val==SUCCESS);
						dt_set_task_change( p_task,CHANGE_DLED_SIZE);
						ret_val = dt_save_task_to_file(p_task);
						sd_assert(ret_val==SUCCESS);
					}
				}
			}
		}
	#endif
		return SUCCESS;
	}

	sd_assert(dt_get_task_state(p_task)!=TS_TASK_RUNNING);
  	new_path_len = em_strlen(file_path);
	file_name = dt_get_task_file_name_from_file(p_task);
	/* 如果URL 是"http://gdl.lixian.vip.xunlei.com/download?fid=" 开头的，需要重新生成特征值 */
	if(p_task->_task_info->_type==TT_URL)
	{
		url = dt_get_task_url_from_file(p_task);
	}	
	
    	EM_LOG_ERROR("dt_correct_task_path:start0:task_id=%u,old[%u]=%s",p_task->_task_info->_task_id,path_len,path); 
    	EM_LOG_ERROR("dt_correct_task_path:start1:task_id=%u,new[%u]=%s",p_task->_task_info->_task_id,new_path_len,file_path); 
    	EM_LOG_ERROR("dt_correct_task_path:start2:task_id=%u,file_name[%u]=%s",p_task->_task_info->_task_id,p_task->_task_info->_file_name_len,file_name); 
    	EM_LOG_ERROR("dt_correct_task_path:start3:task_id=%u,url[%u]=%s",p_task->_task_info->_task_id,p_task->_task_info->_url_len_or_need_dl_num,url); 
		
	em_strncpy(path,file_path,MAX_FILE_PATH_LEN);

	ret_val = dt_generate_file_name_eigenvalue(path,new_path_len,file_name,p_task->_task_info->_file_name_len,&file_name_eigenvalue);
	CHECK_VALUE(ret_val);
	
	if(dt_is_file_exist(file_name_eigenvalue)==TRUE)
	{
		ret_val=FILE_ALREADY_EXIST;
      		EM_LOG_ERROR("dt_correct_task_path:FILE_ALREADY_EXIST:task_id=%u,path=%s,file_name=%s",p_task->_task_info->_task_id,path,file_name); 
		CHECK_VALUE(ret_val);
	}
	
	dt_add_file_name_eigenvalue(file_name_eigenvalue,p_task->_task_info->_task_id);

	if(p_task->_task_info->_file_name_eigenvalue!=0)
	{
		dt_remove_file_name_eigenvalue(p_task->_task_info->_file_name_eigenvalue);
	}


	p_task->_task_info->_file_name_eigenvalue = file_name_eigenvalue;
	
	/* 更正URL 的特征值 */
	if(url)
	{
		if((sd_strncmp(url, "http://gdl.lixian.vip.xunlei.com/download?", sd_strlen("http://gdl.lixian.vip.xunlei.com/download?"))==0)||
			(sd_strstr(url, DEFAULT_YUNBO_URL_MARK, 0)!=NULL))
		{
			ret_val = dt_get_url_eigenvalue(url,p_task->_task_info->_url_len_or_need_dl_num,eigenvalue);
			sd_assert(ret_val==SUCCESS);
			if(!sd_is_cid_equal(eigenvalue, p_task->_task_info->_eigenvalue))
			{
				/* remove old eigenvalue */
				dt_remove_task_eigenvalue(p_task->_task_info->_type,p_task->_task_info->_eigenvalue);

				sd_memcpy(p_task->_task_info->_eigenvalue, eigenvalue, CID_SIZE);
				/* add to new eigenvalue map */
				ret_val = dt_add_task_eigenvalue(p_task->_task_info->_type,p_task->_task_info->_eigenvalue,p_task->_task_info->_task_id);
				sd_assert(ret_val==SUCCESS);
			}
		}
	}
	
	if(TT_BT==dt_get_task_type(p_task))
	{
		sd_assert(FALSE);
	}
	else
	 {
		sd_assert(p_task->_task_info->_full_info==FALSE);
		ret_val = dt_save_p2sp_task_path_to_file(p_task,path,new_path_len);
	 }
    	EM_LOG_ERROR("dt_correct_task_path:end:task_id=%u,ret_val=%d",p_task->_task_info->_task_id,ret_val); 
	CHECK_VALUE(ret_val);
	return SUCCESS;
}


/* 检查是否有足够的剩余空间保证让任务顺利下载完毕*/
_int32 dt_check_task_free_disk(EM_TASK * p_task,char * path)
{
	_int32 ret_val = SUCCESS;
	_u32 need_size = 0;

	/*  是否无盘*/
	if(p_task->_task_info->_check_data) return SUCCESS;
	
	sd_assert(p_task->_task_info->_file_size>= p_task->_task_info->_downloaded_data_size);
	#ifdef KANKAN_PROJ
	if((p_task->_task_info->_type == TT_LAN) && (p_task->_task_info->_failed_code != 112))
	{
		/* 由于局域网任务是用预分配空间的方式,所以它在续传时不再需要磁盘空间 */
		if( p_task->_task_info->_downloaded_data_size!=0)
		{
			/* 续传任务 */
			need_size = 0;
		}
		else
		{
			/* 新任务 */
			need_size = p_task->_task_info->_file_size ;
		}
	}
	else
	{
	need_size = p_task->_task_info->_file_size - p_task->_task_info->_downloaded_data_size;
	}
	#else
	/* 非kankan项目的任务均采用预分配空间的方式 */
	if( p_task->_task_info->_downloaded_data_size!=0)
	{
		/* 续传任务 */
		need_size = 0;
	}
	else
	{
		/* 新任务 */
		need_size = p_task->_task_info->_file_size ;
	}
	#endif /* KANKAN_PROJ */
	if(p_task->_task_info->_file_size!=0 && need_size > 0)
	{
		 ret_val = sd_check_enough_free_disk(path, need_size/1024);
	}
	EM_LOG_ERROR("dt_check_task_free_disk:task_id=%u,file_size=%llu, need_size = %u, ret_val = %d",p_task->_task_info->_task_id,p_task->_task_info->_file_size, need_size, ret_val);
	return ret_val;	
}
