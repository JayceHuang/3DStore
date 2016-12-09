#if !defined(__RCLIST_MANAGER_C_20081105)
#define __RCLIST_MANAGER_C_20081105
/*--------------------------------------------------------------------------*/
/*                               IDENTIFICATION                             */
/*--------------------------------------------------------------------------*/
/*     Filename  : rclist_manager.c                                         */
/*     Author     : ZengYuqing                                              */
/*     Project    : EmbedThunder                                        */
/*     Module     : upload_manager													*/
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
/* This file contains the procedure of rclist manager                       */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*                              HISTORY                                     */
/*--------------------------------------------------------------------------*/
/*   Date     |    Author   | Modification                                  */
/*--------------------------------------------------------------------------*/
/* 2008.11.05 | ZengYuqing  | Creation                                      */
/*--------------------------------------------------------------------------*/
#include "rclist_manager.h"
#include "ulm_file_manager.h"
#include "ulm_pipe_manager.h"

#ifdef UPLOAD_ENABLE

#include "utility/mempool.h"
#include "utility/crc.h"
#include "utility/utility.h"
#include "utility/string.h"
#include "platform/sd_fs.h"
#include "utility/settings.h"
#include "utility/time.h"

#include "utility/logid.h"
#ifdef LOGID
#undef LOGID
#endif

#define LOGID LOGID_UPLOAD_MANAGER

#include "utility/logger.h"

   
/*--------------------------------------------------------------------------*/
/*                Global declarations				        */
/*--------------------------------------------------------------------------*/
/* Pointer of the rclist_manager,only one rclist_manager when program is running */
static RCLIST_MANAGER * g_pRclist_manager = NULL;
static SLAB * gp_id_rc_info_slab = NULL;
static SLAB * gp_id_rc_key_slab = NULL;


/*--------------------------------------------------------------------------*/
/*                Interfaces provid for other module				        */
/*--------------------------------------------------------------------------*/

_int32 init_rclist_manager(void)
{
	_int32 ret_val = SUCCESS;
	
	LOG_DEBUG("init_rclist_manager");
	
	if(g_pRclist_manager != NULL)
		return RCLM_ERR_RCLIST_MGR_EXIST;

	if(gp_id_rc_info_slab!=NULL)
		return RCLM_ERR_RCLIST_SLAB_EXIST;

	ret_val = sd_malloc(sizeof(RCLIST_MANAGER) ,(void**)&g_pRclist_manager);
	if(ret_val!=SUCCESS) return ret_val;

	sd_memset(g_pRclist_manager,0,sizeof(RCLIST_MANAGER));

	ret_val = mpool_create_slab(sizeof(ID_RC_INFO), MIN_RC_LIST_ITEM_MEMORY, 0, &gp_id_rc_info_slab);
	if(ret_val!=SUCCESS) goto ErrorHanle;
	
	ret_val = mpool_create_slab(MAX_RC_INFO_KEY_LEN, MIN_RC_INFO_KEY_MEMORY, 0, &gp_id_rc_key_slab);
	if(ret_val!=SUCCESS) goto ErrorHanle;
	
	list_init(&(g_pRclist_manager->_list_rc));

	map_init(&(g_pRclist_manager->_index_gcid), rclist_index_gcid_map_compare_fun);

	sd_strncpy(g_pRclist_manager->_rc_list_file_path, RC_LIST_FILE_PATH, MAX_FULL_PATH_BUFFER_LEN);
	LOG_DEBUG("RC_LIST_FILE_PATH:%s.", RC_LIST_FILE_PATH);

#if defined(MOBILE_PHONE)
	char config_file_path[MAX_CFG_LINE_LEN + MAX_FILE_PATH_LEN] = {0};
	char *tmp_config = NULL;

#if defined(MACOS)
	tmp_config = malloc_settings_file_path();
#else
	tmp_config = setting_get_cfg_dir();
#endif

	if (!tmp_config || sd_strlen(tmp_config) == 0) {
		tmp_config = CONFIG_FILE;
	}

	sd_strncpy(config_file_path, tmp_config, sd_strlen(tmp_config));

	LOG_DEBUG("config_file_path:%s.", config_file_path);

	sd_assert(sd_strlen(config_file_path) != 0);

	_int32 config_len = sd_strlen(config_file_path);
	_int32 config_file_len = sd_strlen(sd_strrchr(config_file_path, DIR_SPLIT_CHAR));

	config_file_path[config_len-config_file_len+1] = '\0';

	sd_strncpy(g_pRclist_manager->_rc_list_file_path, config_file_path, sd_strlen(config_file_path));

#if defined(MACOS)
	if (config_file_path != NULL) {
		free(config_file_path);
	}
#endif

#endif

	ret_val = rclist_set_rc_list_path(g_pRclist_manager->_rc_list_file_path, MAX_FULL_PATH_BUFFER_LEN);
	CHECK_VALUE(ret_val);

	return SUCCESS;
	
ErrorHanle:
	
	if(gp_id_rc_info_slab!=NULL)
	{
		 mpool_destory_slab(gp_id_rc_info_slab);
		 gp_id_rc_info_slab=NULL;
	}

	if(gp_id_rc_key_slab!=NULL)
	{
		 mpool_destory_slab(gp_id_rc_key_slab);
		 gp_id_rc_key_slab=NULL;
	}

	SAFE_DELETE(g_pRclist_manager);
	g_pRclist_manager=NULL;
	
	return ret_val;
}

_int32 uninit_rclist_manager(void)
{
	
	LOG_DEBUG("uninit_rclist_manager");
	
	if(g_pRclist_manager == NULL)
		return RCLM_ERR_RCLIST_MGR_NOT_EXIST;

	rclist_flush();

	rclist_clear();

	if(gp_id_rc_key_slab!=NULL)
	{
		 mpool_destory_slab(gp_id_rc_key_slab);
		 gp_id_rc_key_slab = NULL;
	}

	if(gp_id_rc_info_slab!=NULL)
	{
		 mpool_destory_slab(gp_id_rc_info_slab);
		 gp_id_rc_info_slab = NULL;
	}

	SAFE_DELETE(g_pRclist_manager);

	return SUCCESS;
}
	

////////////////////////////////////////////////////////////////////////
// 添加资源
_int32 rclist_add_rc(_u64 size, const _u8 *tcid, const _u8 *gcid, _u8 chub, const char *path)
{
	_int32 ret_val = SUCCESS;
		ID_RC_INFO info,*p_rc_info = NULL;
	
		LOG_DEBUG("rclist_add_rc");
	
		p_rc_info = rclist_query(gcid );
	
	
		if ( p_rc_info!=NULL )
		{
			LOG_DEBUG( "Record already in the list:path= %s" ,path);
			return RCLM_ERR_RC_ALREADY_EXIST; //表示已经在列表中
		}
		else
		{
			sd_memset(&info,0,sizeof(ID_RC_INFO));
			info._chub = chub;
			info._size = size;
			sd_memcpy(info._tcid, tcid, CID_SIZE);
			sd_memcpy(info._gcid, gcid, CID_SIZE);
			
			if(sd_strlen(path)>MAX_FILE_PATH_LEN)
				return RCLM_ERR_INVALID_FILE_PATH;
			
			sd_memcpy(info._path, path, sd_strlen(path));
	
			if(list_size(&(g_pRclist_manager->_list_rc))>=MAX_RC_LIST_NUMBER)
			{
				rclist_del_oldest_rc();
			}
			
			LOG_DEBUG( "Record adding to the list:path= %s" ,path);
			ret_val = rclist_insert( &info );
			CHECK_VALUE(ret_val);
			if(g_pRclist_manager->_list_modified_flag == FALSE)
				g_pRclist_manager->_list_modified_flag = TRUE;
			
			rclist_flush();
			
			return SUCCESS; //表示添加到列表中
		}
	
		// 设置列表更改标志位
	}


// 删除资源
_int32 rclist_del_rc(const _u8 *gcid)
{
	_int32 ret_val = SUCCESS;
	ID_RC_INFO * p_rc_info = NULL;

	LOG_DEBUG("rclist_del_rc");
	
	
	p_rc_info = rclist_query( gcid );


	if ( p_rc_info!=NULL )
	{
		LOG_DEBUG( "Record deteling from the list:path= %s" ,p_rc_info->_path);
		ret_val = rclist_remove(p_rc_info );
		CHECK_VALUE(ret_val);
		if(g_pRclist_manager->_list_modified_flag == FALSE)
			g_pRclist_manager->_list_modified_flag = TRUE;

		rclist_flush();
		
		/* Close the file if opend  */
		ufm_close_file(gcid);

		/* Delete all the correlative upload pipes which are just added to upm but not reading data */
 		//upm_del_pipes(gcid);
		
		return SUCCESS; 
	}
	else
	{
		return RCLM_ERR_RC_NOT_FOUND;
	}

}
// 修改资源
_int32 rclist_change_rc_path( const _u8 *gcid,const char *new_path,_u32  new_path_len)
{
	ID_RC_INFO * p_rc_info = NULL;

	LOG_DEBUG("rclist_change_rc_path");

	if((new_path==NULL)||(new_path_len==0)||(new_path_len>=MAX_FILE_PATH_LEN)||(sd_strlen(new_path)!=new_path_len)||(!sd_file_exist(new_path)))
	{
		return RCLM_ERR_INVALID_FILE_PATH;
	}
	
	p_rc_info = rclist_query( gcid );


	if ( p_rc_info!=NULL )
	{
		LOG_DEBUG( "Record change path from old path:%s\n to new path: %s" ,p_rc_info->_path,new_path);
		sd_memset(p_rc_info->_path, 0, MAX_FILE_PATH_LEN);
		sd_memcpy(p_rc_info->_path, new_path, new_path_len);
		if(g_pRclist_manager->_list_modified_flag == FALSE)
			g_pRclist_manager->_list_modified_flag = TRUE;

		rclist_flush();
		
		/* Close the file if opend  */
		ufm_close_file(gcid);
		
		return SUCCESS; 
	}
	else
	{
		return RCLM_ERR_RC_NOT_FOUND;
	}

}

_int32 rclist_change_file_path(_u64 size, const char *old_path, const char *new_path)
{
	_int32 ret_val = RCLM_ERR_INVALID_FILE_PATH;
	ID_RC_INFO *p_rc_info = NULL;
	LIST_ITERATOR cur_node;
	
	LOG_DEBUG("rclist_change_file_path");
	
	for( cur_node = LIST_BEGIN( g_pRclist_manager->_list_rc);
		cur_node != LIST_END( g_pRclist_manager->_list_rc);
		cur_node = LIST_NEXT(cur_node))
	{
		p_rc_info = (ID_RC_INFO*)LIST_VALUE( cur_node );
		if( p_rc_info->_size == size && sd_strcmp(old_path, p_rc_info->_path)==0  )
		{
			LOG_DEBUG("rclist_change_file_path:old_path=%s,new_path=%s", old_path, new_path );

			ret_val = sd_strlen(new_path);
			sd_memcpy(p_rc_info->_path, new_path, ret_val);
			p_rc_info->_path[ret_val] = '\0';

			if(g_pRclist_manager->_list_modified_flag == FALSE)
				g_pRclist_manager->_list_modified_flag = TRUE;
			
			rclist_flush();
			
			/* Close the file if opend  */
			ufm_close_file(p_rc_info->_gcid);
			
			ret_val=SUCCESS;
			break;
		}
	}
		
	return ret_val;

}


ID_RC_INFO *	rclist_query( const _u8 * gcid )
{
	//_int32 ret_val = SUCCESS;
	//char  key_buffer[MAX_RC_INFO_KEY_LEN];
	ID_RC_INFO *p_rc_info = NULL;

	LOG_DEBUG("rclist_query");

	//sd_memset(key_buffer,0,MAX_RC_INFO_KEY_LEN);

	//if(str2hex((char*)gcid, CID_SIZE, key_buffer, MAX_RC_INFO_KEY_LEN)!=SUCCESS)
	//	return NULL;

#ifdef _DEBUG
	{
		_u8 hex_cid[CID_SIZE*2 + 1] = {0};
		sd_cid_to_hex_string(gcid, CID_SIZE, hex_cid, CID_SIZE*2 + 1);
		LOG_DEBUG("prepare to query gcid:%s.", hex_cid);
	}
#endif

	if(map_find_node(&(g_pRclist_manager->_index_gcid), (void*)gcid, (void**)&p_rc_info)!=SUCCESS)
		return NULL;
	return p_rc_info;
}

_int32 compare_size_and_path(void *E1, void *E2)
{
	ID_RC_INFO *p_rc_info = (ID_RC_INFO *)E1;
	PAIR *pair = (PAIR *)E2;
	ID_RC_INFO *p_rc_info2 = (ID_RC_INFO *)pair->_value;
	if (p_rc_info->_size == p_rc_info2->_size && p_rc_info->_path && p_rc_info2->_path && 
		sd_stricmp(p_rc_info->_path, p_rc_info2->_path)==0)
	{
		return 0;
	}
	return -1;
}

ID_RC_INFO *	rclist_query_by_size_and_path(_u64 size, const char *path)
{
	//_int32 ret_val = SUCCESS;
	//char  key_buffer[MAX_RC_INFO_KEY_LEN];
	ID_RC_INFO *p_rc_info = NULL;

	LOG_DEBUG("rclist_query");

	//sd_memset(key_buffer,0,MAX_RC_INFO_KEY_LEN);

	//if(str2hex((char*)gcid, CID_SIZE, key_buffer, MAX_RC_INFO_KEY_LEN)!=SUCCESS)
	//	return NULL;
	ID_RC_INFO findData;
	findData._size = size;
	sd_strncpy(findData._path, path, strlen(path)+1);
	if(map_find_node_by_custom_compare_function(compare_size_and_path, &(g_pRclist_manager->_index_gcid), (void*)&findData, (void**)&p_rc_info)!=SUCCESS)
		return NULL;
	return p_rc_info;
}

_int32 rclist_set_rc_list_path( const char *path, _u32 path_len)

{
	_int32 ret_val = SUCCESS;
	_u32 file_id=0,cur_time=0;
	char file_buffer[MAX_FULL_PATH_BUFFER_LEN]={0};

	LOG_DEBUG("rclist_set_rc_list_path, path:%s.", path);
		
	sd_time_ms(&(cur_time));
	sd_snprintf(file_buffer, MAX_FULL_PATH_BUFFER_LEN, "%s/%u", path, cur_time);
	ret_val = sd_open_ex(file_buffer, O_FS_CREATE, &file_id);
	CHECK_VALUE(ret_val);

	sd_close_ex(file_id);
	sd_delete_file(file_buffer);

	sd_assert(sd_strlen(g_pRclist_manager->_rc_list_file_path) != 0);

	if(g_pRclist_manager->_rc_list_file_path[path_len-1]!='/')
	{
		g_pRclist_manager->_rc_list_file_path[path_len]='/';
	}

	sd_strcat(g_pRclist_manager->_rc_list_file_path, RC_LIST_FILE, sd_strlen(RC_LIST_FILE));

	LOG_DEBUG("g_pRclist_manager->_rc_list_file_path:%s.", g_pRclist_manager->_rc_list_file_path);

	ret_val=rclist_load_data(TRUE);

	if((ret_val!=SUCCESS)&&(ret_val!=RCLM_ERR_RC_FILE_NOT_EXIST))
	{
		return ret_val;
	}
	
	return SUCCESS;
}

LIST *  rclist_get_rc_list(void)
{

	return &(g_pRclist_manager->_list_rc);
}


/*
 * Return: 
 *    >0: E1 > E2	   ==0: E1 == E2		<0: E1 < E2
 */
_int32 rclist_index_gcid_map_compare_fun( void *E1, void *E2 )
{
	_int32 i ,ret_val=0;
	_u8 *gcid1=(_u8*)E1,*gcid2=(_u8*)E2;

	if(gcid1 == NULL||gcid2==NULL)
	{
		LOG_DEBUG("rclist_gcid_compare -1");
		return -1;
	}

	for(i=0;i<CID_SIZE;i++)
	{
		ret_val = gcid1[i]-gcid2 [i];
		if(ret_val!=0) 
		{
			break;
		}
	}
	LOG_DEBUG("rclist_gcid_compare:%d",ret_val);
	return ret_val;
}


////////////////////////////////////////////////////////////////////////
/*	校 验 码(2) 数据版本(2) 数据长度(4) 
	记 录 数(4)
	文件大小(8) 全文cid(20) HUB标识(1) 三段cid(20) 路径长度(4) 路径(?)*/
//	#include <sys/stat.h>
_int32 rclist_load_data(BOOL need_check_file)
{
	_int32 ret_val = SUCCESS;
/*
  struct stat {
        dev_t      st_dev;        //设备号码
        ino_t      st_ino;        //inode节点号
        mode_t     st_mode;       //文件对应的模式，文件，目录等
        nlink_t    st_nlink;      //文件的连接数
        uid_t      st_uid;        //文件所有者
        gid_t      st_gid;        //文件所有者对应的组
        dev_t      st_rdev;       //特殊设备号码
        off_t      st_size;       //普通文件，对应的文件字节数
        blksize_t st_blksize;     //文件内容对应的块大小
        blkcnt_t   st_blocks;     //文件内容对应的块数量
        time_t     st_atime;      //文件最后被访问的时间
        time_t     st_mtime;      //文件内容最后被修改的时间
        time_t     st_ctime;      //文件状态改变时间
      };

 */ 

	_u32 count = 0,read_size = 0, file_id = 0,path_size = 0,last_modified_time=0;
	ID_RC_INFO rc_info;
	RCLIST_STORE_HEAD rclist_head;
	_u8 *pdata = NULL, *ptr = NULL;
	_u16 crc = 0xffff;
	_int32 m =0;
	_u64 file_size = 0;
	LIST * p_rc_list = &(g_pRclist_manager->_list_rc);
	
	LOG_DEBUG("rclist_load_data");


	if(FALSE == sd_file_exist(g_pRclist_manager->_rc_list_file_path))
	{
		LOG_DEBUG("The rc file not exist:%s",g_pRclist_manager->_rc_list_file_path);
		return RCLM_ERR_RC_FILE_NOT_EXIST;
	}
	
	/*获取文件最后修改时间*/
	ret_val = sd_get_file_size_and_modified_time(g_pRclist_manager->_rc_list_file_path,NULL,&last_modified_time);

	// 如果获取成功
	if(ret_val==0)
	{
		if(0==g_pRclist_manager->_file_modified_time)
		{
			g_pRclist_manager->_file_modified_time=last_modified_time;
		}
		else
		if(last_modified_time==g_pRclist_manager->_file_modified_time)
		{
			/*列表文件内容没有变动过，那么就没有必要重新读入数据*/
			LOG_DEBUG("CompareFileTime return 0, file has not been modified. no need to reload data." );
			return SUCCESS;
		}
	
	}

	/*文件内容有更新,需要重新读入数据*/
	rclist_clear();

	/*文件修改标志位清零*/
	g_pRclist_manager->_list_modified_flag = FALSE;


	ret_val = sd_open_ex(g_pRclist_manager->_rc_list_file_path, O_FS_RDONLY, &file_id);
	if(ret_val != SUCCESS)
	{
		/* Opening File error */
		LOG_DEBUG("Opening File error :%s",g_pRclist_manager->_rc_list_file_path);
		CHECK_VALUE(ret_val);
	}

	list_init(p_rc_list);
	sd_memset(&rclist_head,0,sizeof(RCLIST_STORE_HEAD));
	ret_val = sd_read(file_id, (char*)&rclist_head, sizeof(RCLIST_STORE_HEAD), &read_size);
	if(ret_val != SUCCESS)
	{
		/* Reading File error */
		LOG_DEBUG("Reading File error :%s",g_pRclist_manager->_rc_list_file_path);
		goto ErrHandler;
	}
	
	if(read_size==sizeof(RCLIST_STORE_HEAD) )
	{
			if( rclist_head.len <sizeof(count)) goto ErrHandler;
			ret_val = sd_malloc(rclist_head.len + 1 ,(void**)&pdata);
			if(ret_val!=SUCCESS) goto ErrHandler;

			ptr = pdata;
			ret_val = sd_read(file_id,(char*) pdata,  rclist_head.len, &read_size);
			if(ret_val != SUCCESS) goto ErrHandler;

			if(rclist_head.len == read_size)
			{
					
					crc = sd_add_crc16(crc, pdata, rclist_head.len);
					if(FALSE != sd_isvalid_crc16(rclist_head.crc, crc))
					{
						sd_memcpy(&count, ptr, sizeof(count));
						ptr += sizeof(count);
						LOG_DEBUG( "rclist_load_data:rclist_head.crc=%u,ver=%u, len=%u, num=%u" ,rclist_head.crc,rclist_head.ver  ,rclist_head.len , count);

						for( m = 0; m < count; m ++)
						{
							sd_memset(&rc_info, 0, sizeof(ID_RC_INFO));
							
							sd_memcpy(&(rc_info._size), ptr, sizeof(rc_info._size));
							ptr += sizeof(rc_info._size);

							sd_memcpy(rc_info._gcid, ptr, sizeof(rc_info._gcid));
							ptr += sizeof(rc_info._gcid);

							sd_memcpy(&rc_info._chub, ptr, sizeof(rc_info._chub));
							ptr += sizeof(rc_info._chub);

							sd_memcpy(rc_info._tcid, ptr, sizeof(rc_info._tcid));
							ptr += sizeof(rc_info._tcid);
							
							sd_memcpy(&path_size, ptr, sizeof(path_size));
							ptr += sizeof(path_size);
							
							if(path_size >= MAX_FILE_PATH_LEN) 
							{
								ptr += path_size;
								LOG_DEBUG("path_size too long :number=%d,path_size=%u",m,path_size);
								continue;
							}
							
							sd_memcpy(rc_info._path, ptr, path_size);
							ptr += path_size;
#ifdef _DEBUG
							{
								char gcid_buffer[48],cid_buffer[48];
								sd_memset(gcid_buffer,0,48);
								str2hex((char*)(rc_info._gcid), CID_SIZE, gcid_buffer, 48);
								sd_memset(cid_buffer,0,48);
								str2hex((char*)(rc_info._tcid), CID_SIZE, cid_buffer, 48);
								LOG_DEBUG( "load the record from file: file_size=%llu, gcid=%s, tcid=%s, chub=%d, path=%s" 
											,rc_info._size ,gcid_buffer,cid_buffer,rc_info._chub ,rc_info._path);
							}
#endif /* _DEBUG */

							if( need_check_file )
							{
								ret_val = sd_get_file_size_and_modified_time(rc_info._path,&file_size,NULL);

								if((ret_val!=0)||((ret_val==0)&&(file_size!=rc_info._size))||
									(!sd_is_file_readable(rc_info._path)))
								{
									LOG_DEBUG("rclist_load_data:check_file :ret_val=%d,file_size=%llu,rc_info._size=%llu,rc_info._path=%s",ret_val,file_size,rc_info._size,rc_info._path);
									/*列表文件内容有变动过*/
									g_pRclist_manager->_list_modified_flag = TRUE; // 有记录没有读入，列表与文件相比有变动，需要flush
									ret_val = SUCCESS;
								}
								else
								{
									rclist_insert(&rc_info);
								}
							}
							else
							{
								rclist_insert(&rc_info);
							}
						}
					}
					else
					{
						LOG_DEBUG("RCLM_ERR_INVALID_RC_FILE_CRC");
						ret_val =RCLM_ERR_INVALID_RC_FILE_CRC;
					}
				}
				else
				{
					LOG_DEBUG("RCLM_ERR_RC_FILE_SIZE");
					ret_val =RCLM_ERR_RC_FILE_SIZE;
				}
	}
	else
	{
		LOG_DEBUG("RCLM_ERR_READING_RC_HEAD");
		ret_val =RCLM_ERR_READING_RC_HEAD;
	}
	
ErrHandler:
	
	SAFE_DELETE(pdata);
	sd_close_ex(file_id);	 

	LOG_ERROR("rclist_load_data end!ret_val=%d",ret_val);

	//CHECK_VALUE(ret_val);
	return SUCCESS;
	
}
	


_int32 rclist_flush(void)
{
	_int32 ret_val = SUCCESS;
	_u32 file_id = 0,count = 0,last_modified_time=0,write_size=0;
	RCLIST_STORE_HEAD rclist_head;
	LIST_NODE * p_list_node = NULL;
	ID_RC_INFO * p_rc_info = NULL;
	char  buffer[2048];
	_u32 buffer_pos=0,buffer_len=2048;

	LOG_DEBUG (  "rclist_flush" );


	if ( FALSE == g_pRclist_manager->_list_modified_flag )
	{
		/*列表从文件读入后，内容没有更新过，所以不需要写入到文件*/
		LOG_DEBUG (  "_rc_list never been modified. so no need to flush to file." );
		return SUCCESS;
	}

	if(STORE_VERSION < rclist_get_version())
	{
		LOG_DEBUG (  "rclist_flush:RCLM_ERR_RC_STORE_VERSION" );
		//return RCLM_ERR_RC_STORE_VERSION;
	}
	
	ret_val = sd_delete_file(g_pRclist_manager->_rc_list_file_path);
	if(ret_val != SUCCESS)
	{
		/* Opening File error */
		LOG_DEBUG (  "rclist_flush:eleting File error " );
		//CHECK_VALUE(ret_val);
	}
	
	ret_val = sd_open_ex(g_pRclist_manager->_rc_list_file_path, O_FS_CREATE, &file_id);
	if(ret_val != SUCCESS)
	{
		/* Opening File error */
		LOG_DEBUG (  "rclist_flush:Opening File error " );
		CHECK_VALUE(ret_val);
	}

	rclist_head.crc = 0xffff;
	rclist_head.len = 0;
	rclist_head.ver = STORE_VERSION;

	ret_val = sd_setfilepos(file_id, sizeof(RCLIST_STORE_HEAD));
	if(ret_val != SUCCESS) goto ErrHandler;

	count = list_size(&(g_pRclist_manager->_list_rc));

	LOG_DEBUG (  "rclist_flush:count=%u " ,count);
	 
	ret_val = sd_write_save_to_buffer(file_id, buffer, buffer_len,&buffer_pos, (char*) &count, sizeof(count));
	if(ret_val != SUCCESS) goto ErrHandler;

	rclist_head.crc = sd_add_crc16(rclist_head.crc, &count, sizeof(count));		
	rclist_head.len+= sizeof(count);


	for( p_list_node =  LIST_BEGIN(g_pRclist_manager->_list_rc);	p_list_node !=  LIST_END(g_pRclist_manager->_list_rc); p_list_node =  LIST_NEXT(p_list_node))
	{
		p_rc_info = (ID_RC_INFO *)(p_list_node->_data);
		ret_val = sd_write_save_to_buffer(file_id, buffer, buffer_len,&buffer_pos, (char*)&(p_rc_info->_size), sizeof(p_rc_info->_size));
		if(ret_val != SUCCESS) goto ErrHandler;
		rclist_head.crc = sd_add_crc16(rclist_head.crc, &(p_rc_info->_size), sizeof(p_rc_info->_size));		
		rclist_head.len+= sizeof(p_rc_info->_size);

		ret_val = sd_write_save_to_buffer(file_id, buffer, buffer_len,&buffer_pos, (char*)(p_rc_info->_gcid), CID_SIZE);
		if(ret_val != SUCCESS) goto ErrHandler;
		rclist_head.crc = sd_add_crc16(rclist_head.crc, p_rc_info->_gcid, CID_SIZE);		
		rclist_head.len+= CID_SIZE;

		ret_val = sd_write_save_to_buffer(file_id, buffer, buffer_len,&buffer_pos, (char*) &(p_rc_info->_chub), sizeof(p_rc_info->_chub));
		if(ret_val != SUCCESS) goto ErrHandler;
		rclist_head.crc = sd_add_crc16(rclist_head.crc, &(p_rc_info->_chub), sizeof(p_rc_info->_chub));		
		rclist_head.len+= sizeof(p_rc_info->_chub);

		ret_val = sd_write_save_to_buffer(file_id, buffer, buffer_len,&buffer_pos, (char*)(p_rc_info->_tcid), CID_SIZE);
		if(ret_val != SUCCESS) goto ErrHandler;
		rclist_head.crc = sd_add_crc16(rclist_head.crc, p_rc_info->_tcid, CID_SIZE);		
		rclist_head.len+= CID_SIZE;

		count = sd_strlen(p_rc_info->_path);	 
		ret_val = sd_write_save_to_buffer(file_id, buffer, buffer_len,&buffer_pos,  (char*)&count, sizeof(count));
		if(ret_val != SUCCESS) goto ErrHandler;
		rclist_head.crc = sd_add_crc16(rclist_head.crc, &count, sizeof(count));		
		rclist_head.len+= sizeof(count);

		ret_val = sd_write_save_to_buffer(file_id, buffer, buffer_len,&buffer_pos,  p_rc_info->_path, count);
		if(ret_val != SUCCESS) goto ErrHandler;
		rclist_head.crc = sd_add_crc16(rclist_head.crc, p_rc_info->_path, count);		
		rclist_head.len+= count;
#ifdef _DEBUG
		{
			char gcid_buffer[48],cid_buffer[48];
			sd_memset(gcid_buffer,0,48);
			str2hex((char*)(p_rc_info->_gcid), CID_SIZE, gcid_buffer, 48);
			sd_memset(cid_buffer,0,48);
			str2hex((char*)(p_rc_info->_tcid), CID_SIZE, cid_buffer, 48);
			LOG_DEBUG( "Save the record to file: file_size=%llu, gcid=%s, tcid=%s, chub=%d, path=%s" 
						,p_rc_info->_size ,gcid_buffer,cid_buffer,p_rc_info->_chub ,p_rc_info->_path);
		}
#endif /* _DEBUG */
	}

	if(buffer_pos!=0)
	{
		ret_val =sd_write(file_id,buffer, buffer_pos, &write_size);
		if(ret_val != SUCCESS) goto ErrHandler;
		sd_assert(buffer_pos==write_size);
	}

	ret_val = sd_setfilepos(file_id, 0);
	if(ret_val != SUCCESS) goto ErrHandler;

	rclist_head.crc = sd_inv_crc16(rclist_head.crc);
	
	LOG_DEBUG (  "rclist_flush:rclist_head.crc=%u ,ver=%u,len=%u" ,rclist_head.crc,rclist_head.ver,rclist_head.len);
	
	ret_val = sd_write(file_id, (char*)&rclist_head, sizeof(rclist_head), &write_size);
	if((ret_val != SUCCESS)||(write_size!=sizeof(rclist_head))) goto ErrHandler;
	
	//fflush(fcb);
	sd_close_ex(file_id);

	/*文件修改标志位清零*/
	g_pRclist_manager->_list_modified_flag = FALSE;

	/*获取文件最后修改时间*/
	ret_val =  sd_get_file_size_and_modified_time(g_pRclist_manager->_rc_list_file_path,NULL,&last_modified_time);

	// 如果获取成功
	if(ret_val==0)
	{
		/*更新文件最后一次更改时间*/
		g_pRclist_manager->_file_modified_time = last_modified_time;	
		LOG_DEBUG (  "rclist_flush:last_modified_time=%u" ,last_modified_time);
	}
	

	return SUCCESS;
	
ErrHandler:
	sd_close_ex(file_id);
	if(ret_val == SUCCESS)
		return RCLM_ERR_WRITING_FILE;
	return ret_val;
}
	

//////////////////////////// id_rc_list_info //////////////////////////
/*
_int32 rclist_get_rc_info(ID_RC_INFO * p_info, enum INFO_KEY key )
{

	id_rc_info * info_ptr = _rc_list.query(info, key);

	// 如果列表中存在该资源
	if ( info_ptr ) // && info_ptr->_size == file_ex::file_size(info_ptr->_path) ) // 因为界面是先修改文件名，后修改资源列表，所以不能判断文件大小
	{
		info = *info_ptr;
		return info._chub;
	}
	
	return 0xff;
}
*/


_int32		rclist_insert( const ID_RC_INFO * p_info )
{
	char * map_key = NULL;
	ID_RC_INFO* p_rc_info = NULL;
	PAIR node;
	_int32 ret_val = SUCCESS;
	LIST_NODE * p_list_node=NULL;
	
	LOG_DEBUG("rclist_insert");

	
	ret_val = mpool_get_slip(gp_id_rc_info_slab, (void**)&p_rc_info);
	CHECK_VALUE(ret_val);

	sd_memset(p_rc_info,0,sizeof(ID_RC_INFO));
	sd_memcpy(p_rc_info,p_info,sizeof(ID_RC_INFO));
	

	/* store in the list: _list_rc */
	ret_val =  list_push(&(g_pRclist_manager->_list_rc), (void*)p_rc_info);
	if(ret_val!=SUCCESS)
	{
		mpool_free_slip(gp_id_rc_info_slab,(void*) p_rc_info);
		CHECK_VALUE(ret_val);
	}

	/* store in the map: _index_gcid */
	map_key = NULL;
	ret_val = mpool_get_slip(gp_id_rc_key_slab, (void**)&map_key);
	if(ret_val!=SUCCESS) goto ErrHandle;

	sd_memset(map_key,0,MAX_RC_INFO_KEY_LEN);
	sd_memcpy(map_key,p_info->_gcid,CID_SIZE);
	//ret_val = str2hex((char*)(p_info->_gcid), CID_SIZE, map_key, MAX_RC_INFO_KEY_LEN);
	//if(ret_val!=SUCCESS) goto ErrHandle;

#ifdef _DEBUG
	{
		_u8 hex_cid[CID_SIZE*2 + 1] = {0};
		sd_cid_to_hex_string(p_info->_gcid, CID_SIZE, hex_cid, CID_SIZE*2 + 1);
		LOG_DEBUG("prepare to insert gcid:%s.", hex_cid);
	}
#endif

	node._key = (void*)map_key;
	node._value = (void*)p_rc_info;
	ret_val = map_insert_node(&(g_pRclist_manager->_index_gcid), &node);
	if(ret_val!=SUCCESS) goto ErrHandle;

	return SUCCESS;
	
ErrHandle:
	p_list_node=LIST_RBEGIN(g_pRclist_manager->_list_rc);
	sd_assert(p_rc_info==LIST_VALUE(p_list_node));
	list_erase(&(g_pRclist_manager->_list_rc), p_list_node);
	mpool_free_slip(gp_id_rc_info_slab,(void*) p_rc_info);

	if(map_key!=NULL)
	{
		mpool_free_slip(gp_id_rc_key_slab,(void*) map_key);
	}
	
	CHECK_VALUE(ret_val);
	
	return SUCCESS;
}


_int32			rclist_remove(ID_RC_INFO * p_info)
{
	_int32 ret_val = SUCCESS;
	_u8  key_buffer[MAX_RC_INFO_KEY_LEN],*map_key=NULL;
	ID_RC_INFO *p_rc_info = NULL;
	MAP_ITERATOR iterator ;
	LIST_ITERATOR cur_node;
	
	LOG_DEBUG("rclist_remove");

	sd_memset(key_buffer,0,MAX_RC_INFO_KEY_LEN);
	sd_memcpy(key_buffer,p_info->_gcid,CID_SIZE);
	//ret_val = str2hex((char*)(p_info->_gcid), CID_SIZE, key_buffer, MAX_RC_INFO_KEY_LEN);
	//CHECK_VALUE(ret_val);
	ret_val = map_find_iterator(&(g_pRclist_manager->_index_gcid), key_buffer, &iterator);
	CHECK_VALUE(ret_val);
	sd_assert( iterator != MAP_END( g_pRclist_manager->_index_gcid ) );
	map_key = (_u8*) MAP_KEY(iterator);
	p_rc_info =  (ID_RC_INFO *)MAP_VALUE(iterator);
	mpool_free_slip(gp_id_rc_key_slab,(void*) map_key);
	map_erase_iterator(&(g_pRclist_manager->_index_gcid), iterator);

	ret_val =-1;
	
	for( cur_node = LIST_BEGIN( g_pRclist_manager->_list_rc);
		cur_node != LIST_END( g_pRclist_manager->_list_rc);
		cur_node = LIST_NEXT(cur_node))
	{
		if(p_rc_info == (ID_RC_INFO*)LIST_VALUE( cur_node ))
		{
			list_erase(&(g_pRclist_manager->_list_rc), cur_node);
			ret_val=SUCCESS;
			break;
		}

	}
		
	mpool_free_slip(gp_id_rc_info_slab,(void*) p_rc_info);
	
	CHECK_VALUE(ret_val);
	return SUCCESS;

}

_int32			rclist_clear(void)
{
	_u32 _list_size = 0;
	ID_RC_INFO * p_rc_info = NULL;
	MAP_ITERATOR cur_node , next_node;
	char * map_key = NULL;
	
	LOG_DEBUG("rclist_clear");
	
	cur_node = MAP_BEGIN( g_pRclist_manager->_index_gcid );

	while( cur_node != MAP_END( g_pRclist_manager->_index_gcid ) )
	{
		map_key = (char *)MAP_KEY( cur_node );
		next_node = MAP_NEXT( g_pRclist_manager->_index_gcid, cur_node );
		map_erase_iterator(&(g_pRclist_manager->_index_gcid), cur_node );
		mpool_free_slip(gp_id_rc_key_slab,(void*) map_key);
		cur_node = next_node;
	}

	_list_size =  list_size(&(g_pRclist_manager->_list_rc)); 
	while(_list_size)
	{
 		if(list_pop(&(g_pRclist_manager->_list_rc), (void**)&p_rc_info)==SUCCESS) 
			mpool_free_slip(gp_id_rc_info_slab,(void*) p_rc_info);
		_list_size--;
	}


	return SUCCESS;
}

_int32 rclist_del_oldest_rc(void)
{
	_int32 ret_val = SUCCESS;
	ID_RC_INFO * p_rc_info = NULL;
	_u8  key_buffer[MAX_RC_INFO_KEY_LEN],*map_key=NULL;
	MAP_ITERATOR iterator ;

	LOG_DEBUG("rclist_del_oldest_rc");
	
	
	ret_val = list_pop(&(g_pRclist_manager->_list_rc), (void **)&p_rc_info);
	CHECK_VALUE(ret_val);

	LOG_DEBUG( "Oldest Record deteling from the list:path= %s" ,p_rc_info->_path);

	sd_memset(key_buffer,0,MAX_RC_INFO_KEY_LEN);
	sd_memcpy(key_buffer,p_rc_info->_gcid,CID_SIZE);

	ret_val = map_find_iterator(&(g_pRclist_manager->_index_gcid), key_buffer, &iterator);
	CHECK_VALUE(ret_val);
	sd_assert( iterator != MAP_END( g_pRclist_manager->_index_gcid ) );
	map_key = (_u8*) MAP_KEY(iterator);
	mpool_free_slip(gp_id_rc_key_slab,(void*) map_key);
	map_erase_iterator(&(g_pRclist_manager->_index_gcid), iterator);

	mpool_free_slip(gp_id_rc_info_slab,(void*) p_rc_info);
	
	if(g_pRclist_manager->_list_modified_flag == FALSE)
		g_pRclist_manager->_list_modified_flag = TRUE;

	/* Close the file if opend  */
	ufm_close_file(key_buffer);

	/* Delete all the correlative upload pipes which are just added to upm but not reading data */
 	//upm_del_pipes(key_buffer);
		
	if(list_size(&(g_pRclist_manager->_list_rc))>=MAX_RC_LIST_NUMBER)
	{
		rclist_del_oldest_rc();
	}
	return SUCCESS; 

}

////////////////////////////////////////////////////////////////////////


_int32 rclist_get_version(void)
{
	_int32 ret_val = SUCCESS;
	_u32 file_id = 0,read_size = 0;
	RCLIST_STORE_HEAD rclist_head;
	
	LOG_DEBUG( "rclist_get_version" );
	
	ret_val = sd_open_ex(g_pRclist_manager->_rc_list_file_path, O_FS_RDONLY, &file_id);
	if(ret_val != SUCCESS)
	{
		/* Opening File error */
		return STORE_VERSION;
	}

	sd_memset(&rclist_head,0,sizeof(RCLIST_STORE_HEAD));
	ret_val = sd_read(file_id,(char*) &rclist_head, sizeof(RCLIST_STORE_HEAD), &read_size);
	sd_close_ex(file_id);	 
	
	if(ret_val != SUCCESS)
	{
		/* Reading File error */
		return STORE_VERSION;
	}
	
	if(read_size==sizeof(RCLIST_STORE_HEAD) )
	{
		return (_int32)(rclist_head.ver);
	}
	
	return STORE_VERSION;
}

#endif /* UPLOAD_ENABLE */

#endif /* __RCLIST_MANAGER_C_20081105 */

