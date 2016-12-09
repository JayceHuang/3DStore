
#include "tree_manager/tree_store.h"
#include "tree_manager/tree_manager.h"

#include "scheduler/scheduler.h"

#include "em_interface/em_fs.h"
#include "em_common/em_logid.h"
#ifdef EM_LOGID
#undef EM_LOGID
#endif

#define EM_LOGID EM_LOGID_TREE_MANAGER

#include "em_common/em_logger.h"

////////////////////////////////////////////////////////////////////////////////////////////
#define CHECK_FILE_OPEN(p_tree) 		{em_time_ms(&p_tree->_time_stamp); if(p_tree->_file_id==0) {if(SUCCESS!=em_open_ex(trm_get_tree_file_path(p_tree),p_tree->_flag/* O_FS_CREATE*/, &p_tree->_file_id)) {sd_assert(FALSE);return NULL;}}}
#define CHECK_FILE_OPEN_INT(p_tree) 	{em_time_ms(&p_tree->_time_stamp); if(p_tree->_file_id==0) {if(SUCCESS!=em_open_ex(trm_get_tree_file_path(p_tree), p_tree->_flag/* O_FS_CREATE*/, &p_tree->_file_id)) CHECK_VALUE(-1);}}
#define CHECK_FILE_OPEN_VOID(p_tree) 		{em_time_ms(&p_tree->_time_stamp); if(p_tree->_file_id==0) {if(SUCCESS!=em_open_ex(trm_get_tree_file_path(p_tree), p_tree->_flag/* O_FS_CREATE*/, &p_tree->_file_id)) {sd_assert(FALSE);return;}}}
/////////////////////////////////////////////////////////////////////////////////////////
/*--------------------------------------------------------------------------*/
/*                Interfaces provid for external program				        */
/*--------------------------------------------------------------------------*/
char * trm_get_tree_file_path(TREE * p_tree)
{
	return p_tree->_tree_file;
}
_int32 trm_close_file(TREE * p_tree,BOOL force_close)
{
	_int32 ret_val = SUCCESS;
	_u32 cur_time=0;
	if(p_tree->_file_id==0) return SUCCESS;
	
	em_time_ms(&cur_time);
	if(force_close)
	{
     		EM_LOG_DEBUG("trm_close_file");
		trm_stop_clear_file(p_tree);
		ret_val= em_close_ex(p_tree->_file_id);
		CHECK_VALUE(ret_val);
		p_tree->_file_id = 0;
		p_tree->_time_stamp= cur_time;
		//g_pos_tree_file_end=TRM_POS_NODE_START;
	}
	else
	{
		if(TIME_SUBZ(cur_time, p_tree->_time_stamp)>MAX_FILE_IDLE_INTERVAL*2)
		{
     			EM_LOG_DEBUG("trm_close_file 2"); 
			ret_val= em_close_ex(p_tree->_file_id);
			CHECK_VALUE(ret_val);
			p_tree->_file_id = 0;
			p_tree->_time_stamp = cur_time;
		}
		/*
		else
		if((TIME_SUBZ(cur_time, g_tree_file_time_stamp)>MAX_FILE_IDLE_INTERVAL)&&g_need_clear&&!g_file_locked)
		{
			trm_clear_file();
		}
		*/
	}

	return SUCCESS;
}

_int32 trm_create_tree_file(TREE * p_tree)
{
	_int32 ret_val=SUCCESS;
	_u32 readsize=0,writesize=0;
	EM_STORE_HEAD file_head;
	char * file_path = trm_get_tree_file_path(p_tree);
	char new_file_path[MAX_FULL_PATH_BUFFER_LEN];
	_u64 _cur_file_size=0;
	
      	EM_LOG_DEBUG("trm_create_tree_file"); 
		
	p_tree->_pos_file_end= TRM_POS_NODE_START;
	
READ_AGAIN:
	if(em_file_exist(file_path))
	{
		sd_assert(p_tree->_file_id==0);

		CHECK_FILE_OPEN_INT(p_tree);
		
		ret_val=em_pread(p_tree->_file_id, (char *)&file_head,sizeof(EM_STORE_HEAD), 0, &readsize);
		if((ret_val==SUCCESS)&&(sizeof(EM_STORE_HEAD)==readsize)&&(file_head._ver == TREE_STORE_FILE_VERSION))
		{
			return SUCCESS;
		}

		if((ret_val==SUCCESS)&&(sizeof(EM_STORE_HEAD)==readsize))
		{
			if(file_head._ver < TREE_STORE_FILE_VERSION)
			{
				/* 旧版本文件 ,必须转化为本版本的文件*/
				//sd_assert(FALSE);
				ret_val= em_close_ex(p_tree->_file_id);
				CHECK_VALUE(ret_val);
				p_tree->_file_id = 0;
      				EM_LOG_ERROR("trm_create_tree_file:the version[%d] of the tree store file[%s] is old,need convert to current version[%d]! ",file_head._ver,p_tree->_tree_file,TREE_STORE_FILE_VERSION); 
				ret_val=trm_convert_old_ver_to_current( p_tree);
				CHECK_VALUE(ret_val);
				return SUCCESS;
			}
			else
			{
				/* 更新版本文件 */
				//sd_assert(FALSE);
				return SUCCESS;
			}
		}
		else
		{
			/* 读失败 */
			ret_val= em_close_ex(p_tree->_file_id);
			CHECK_VALUE(ret_val);
			p_tree->_file_id = 0;
			
			em_memset(new_file_path,0, MAX_FULL_PATH_BUFFER_LEN);
			em_strncpy(new_file_path, file_path, MAX_FULL_PATH_BUFFER_LEN-4);
			em_strcat(new_file_path, ".old", 4);
			if(em_file_exist(new_file_path))
			{
				em_delete_file(new_file_path);
			}
			
			ret_val= em_rename_file(file_path, new_file_path);
			CHECK_VALUE(ret_val);
		}
		
		if(trm_recover_file(p_tree))
		{
			goto READ_AGAIN;
		}
	}

	sd_assert(p_tree->_file_id==0);

	CHECK_FILE_OPEN_INT(p_tree);

	file_head._crc = 0;
	file_head._ver = TREE_STORE_FILE_VERSION;
	file_head._len = 0;
		
	ret_val = em_pwrite(p_tree->_file_id, (char *)&file_head, sizeof(EM_STORE_HEAD), 0 , &writesize);
	if((ret_val!=SUCCESS)||(writesize!=sizeof(EM_STORE_HEAD)))
	{
		em_close_ex(p_tree->_file_id);
		CHECK_VALUE( INVALID_WRITE_SIZE);
	}
	
	ret_val = em_enlarge_file(p_tree->_file_id, p_tree->_pos_file_end+1, &_cur_file_size);
	if(ret_val!=SUCCESS)
	{
		em_close_ex(p_tree->_file_id);
		CHECK_VALUE( ret_val);
	}
	
	/* init the file */
	trm_save_total_node_num(p_tree,1);
	
      	EM_LOG_DEBUG("trm_create_tree_file:SUCCESS"); 
	return SUCCESS;
}

BOOL trm_is_tree_file_need_clear_up(TREE * p_tree)
{
	_u32 cur_time=0;
	
	if(p_tree->_file_id!=0) return FALSE;
	
	em_time_ms(&cur_time);

	if((TIME_SUBZ(cur_time, p_tree->_time_stamp)>MAX_FILE_IDLE_INTERVAL)&&p_tree->_need_clear&&!p_tree->_file_locked)
	{
		return TRUE;
	}
	
	return FALSE;
}

_int32 trm_clear_file_impl(void* param)
{
	_int32 ret_val=SUCCESS;
	_u32 read_size=0,writesize=0,file_id=0,total_node_num=0,valid_node_num=0,invalid_node_num=0,item_len=0;
	EM_STORE_HEAD file_head;
	TREE_NODE_STORE_HEAD node_head;
	TREE_NODE_INFO_STORE node_info;
	//TREE_NODE_STORE node;
	TREE * p_tree = (TREE *)param;
	char * file_path = trm_get_tree_file_path(p_tree);
	char new_file_path[MAX_FULL_PATH_BUFFER_LEN];
	_u64 pos_file_end=TRM_POS_NODE_START,pos_node_file_end=TRM_POS_NODE_START;
	char buffer[512];
	char * user_data=NULL;
	BOOL need_release = FALSE;
	_u64 filesize = 0;
	
      	EM_LOG_DEBUG("trm_clear_file_impl"); 
	sd_assert(p_tree->_file_id==0);

	sd_assert(p_tree->_file_locked!=TRUE);

	p_tree->_file_locked=TRUE;
	em_memset(new_file_path,0, MAX_FULL_PATH_BUFFER_LEN);
	em_strncpy(new_file_path, file_path, MAX_FULL_PATH_BUFFER_LEN-4);
	em_strcat(new_file_path, ".tmp", 4);

	
	if(em_file_exist(new_file_path))
	{
		ret_val= em_delete_file(new_file_path);
		sd_assert(ret_val==SUCCESS);
		if(ret_val!=SUCCESS)goto ErrorHanle;
	}

      	/*  Open the  new  tree file to write */;
	ret_val= em_open_ex(new_file_path, O_FS_CREATE, &file_id);
	sd_assert(ret_val==SUCCESS);
	if(ret_val!=SUCCESS) goto ErrorHanle;
	
	if(!p_tree->_file_locked) goto ErrorHanle;
	
	ret_val = em_enlarge_file(file_id, TRM_POS_NODE_START+1, &filesize);
	sd_assert(ret_val==SUCCESS);
	if(ret_val!=SUCCESS) goto ErrorHanle;
	
      	EM_LOG_DEBUG("trm_clear_file_impl:file_id=%u,filesize=%llu",file_id,filesize); 
		
      	/*  Open the  original tree file to read */;
	em_time_ms(&p_tree->_time_stamp); 
	if(p_tree->_file_id==0) 
	{
		if(SUCCESS!=em_open_ex(trm_get_tree_file_path(p_tree),  O_FS_RDONLY, &p_tree->_file_id)) 
		{
			sd_assert(FALSE);
			p_tree->_file_id = 0;
			goto ErrorHanle;
		}
	}

      	/*  EM_STORE_HEAD */;
	ret_val = em_pread(p_tree->_file_id, (char*)&file_head, sizeof(EM_STORE_HEAD),0,&read_size);
	sd_assert(ret_val==SUCCESS);
	if((ret_val!=SUCCESS)||(read_size!=sizeof(EM_STORE_HEAD) ))
	{
		ret_val= INVALID_READ_SIZE;
		sd_assert(ret_val==SUCCESS);
		goto ErrorHanle;
	}
	
	if(!p_tree->_file_locked) goto ErrorHanle;
	
	ret_val = em_pwrite(file_id, (char *)&file_head, sizeof(EM_STORE_HEAD), 0 , &writesize);
	sd_assert(ret_val==SUCCESS);
	if((ret_val!=SUCCESS)||(writesize!=sizeof(EM_STORE_HEAD)))
	{
		ret_val= INVALID_WRITE_SIZE;
		sd_assert(ret_val==SUCCESS);
		goto ErrorHanle;
	}

	if(!p_tree->_file_locked) goto ErrorHanle;
	
      	/*  total_node_num */;
	ret_val= trm_get_total_node_num(p_tree,&total_node_num);
	sd_assert(ret_val==SUCCESS);
	if(ret_val!=SUCCESS) goto ErrorHanle;

	if(!p_tree->_file_locked) goto ErrorHanle;
	
	ret_val = em_pwrite(file_id, (char *)&total_node_num, sizeof(_u32), TRM_POS_TOTAL_NODE_NUM , &writesize);
	sd_assert(ret_val==SUCCESS);
	if((ret_val!=SUCCESS)||(writesize!=sizeof(_u32)))
	{
		ret_val= INVALID_WRITE_SIZE;
		sd_assert(ret_val==SUCCESS);
		goto ErrorHanle;
	}
	
      	/*  FILE_RESERVE DEFAULT_FILE_RESERVE_SIZE(64)bytes保留字段为以后扩展用 */
	ret_val = em_pread(p_tree->_file_id, (char*)buffer, DEFAULT_FILE_RESERVED_SIZE,TRM_POS_FILE_RESERVED,&read_size);
	sd_assert(ret_val==SUCCESS);
	if((ret_val!=SUCCESS)||(read_size!=DEFAULT_FILE_RESERVED_SIZE ))
	{
		ret_val= INVALID_READ_SIZE;
		sd_assert(ret_val==SUCCESS);
		goto ErrorHanle;
	}
	
	if(!p_tree->_file_locked) goto ErrorHanle;
	
	ret_val = em_pwrite(file_id, (char *)buffer, DEFAULT_FILE_RESERVED_SIZE, TRM_POS_FILE_RESERVED , &writesize);
	sd_assert(ret_val==SUCCESS);
	if((ret_val!=SUCCESS)||(writesize!=DEFAULT_FILE_RESERVED_SIZE))
	{
		ret_val= INVALID_WRITE_SIZE;
		sd_assert(ret_val==SUCCESS);
		goto ErrorHanle;
	}

      	EM_LOG_DEBUG("trm_clear_file_impl:total_node_num=%u,pos_task_file_end=%llu",total_node_num,pos_node_file_end); 
		
      	/* 开始读节点信息 */
	while(p_tree->_file_locked)
	{
      		/*  TREE_NODE_STORE_HEAD  read */;
		ret_val = em_pread(p_tree->_file_id, (char*)&node_head, sizeof(TREE_NODE_STORE_HEAD),pos_node_file_end,&read_size);
		sd_assert(ret_val==SUCCESS);
		if((ret_val!=SUCCESS)||(read_size!=sizeof(TREE_NODE_STORE_HEAD) ))
		{
			if((ret_val==0)&&(read_size==0))
			{
				/* ok,Finished! */
      				EM_LOG_DEBUG("trm_clear_file_impl:end of reading nodes, pos_node_file_end=%llu",pos_node_file_end); 
				break;
			}
			
			if((ret_val==0)&&(read_size<sizeof(TREE_NODE_STORE_HEAD)))
			{
				/* Broken file */
      				EM_LOG_DEBUG("trm_clear_file_impl:end of reading nodes, Broken file!pos_node_file_end=%llu",pos_node_file_end); 
				sd_assert(FALSE);
				break;
			}
			
			ret_val= INVALID_READ_SIZE;
			sd_assert(ret_val==SUCCESS);
			goto ErrorHanle;
		}

      		EM_LOG_DEBUG("trm_clear_file_impl:valid=%d",node_head._valid); 
			
		pos_node_file_end+=read_size;
		if(node_head._valid==0)
		{
			pos_node_file_end+=node_head._len;
			invalid_node_num++;
			continue;
		}

		if(!p_tree->_file_locked) goto ErrorHanle;
      		/*  TREE_NODE_STORE_HEAD write*/;
		ret_val = em_pwrite(file_id, (char *)&node_head,read_size, pos_file_end , &writesize);
		sd_assert(ret_val==SUCCESS);
		if((ret_val!=SUCCESS)||(writesize!=read_size))
		{
			ret_val= INVALID_WRITE_SIZE;
			sd_assert(ret_val==SUCCESS);
			goto ErrorHanle;
		}
		pos_file_end+=writesize;
		
		if(!p_tree->_file_locked) goto ErrorHanle;
      		/*  TREE_NODE_INFO_STORE  read */;
		ret_val = em_pread(p_tree->_file_id, (char*)&node_info, sizeof(TREE_NODE_INFO_STORE),pos_node_file_end,&read_size);
		sd_assert(ret_val==SUCCESS);
		if((ret_val!=SUCCESS)||(read_size!=sizeof(TREE_NODE_INFO_STORE) ))
		{
			ret_val= INVALID_READ_SIZE;
			sd_assert(ret_val==SUCCESS);
			goto ErrorHanle;
		}
		pos_node_file_end+=read_size;
		
		if(!p_tree->_file_locked) goto ErrorHanle;
      		/*  TREE_NODE_INFO_STORE  write */;
		ret_val = em_pwrite(file_id, (char *)&node_info,read_size, pos_file_end , &writesize);
		sd_assert(ret_val==SUCCESS);
		if((ret_val!=SUCCESS)||(writesize!=read_size))
		{
			ret_val= INVALID_WRITE_SIZE;
			sd_assert(ret_val==SUCCESS);
			goto ErrorHanle;
		}
		pos_file_end+=writesize;
		
		if(!p_tree->_file_locked) goto ErrorHanle;
		
		/* data */
		if(node_info._data_len!=0)
		{
			if(node_info._data_len>512)
			{
				ret_val=em_malloc(node_info._data_len, (void**)&user_data);
				sd_assert(ret_val==SUCCESS);
				if(ret_val!=SUCCESS)
				{
					goto ErrorHanle;
				}
				need_release = TRUE;
			}
			else
			{
				user_data = buffer;
			}
			ret_val=em_pread(p_tree->_file_id, (char *)user_data,node_info._data_len, pos_node_file_end, &read_size);
			sd_assert(ret_val==SUCCESS);
			if((ret_val!=SUCCESS)||(read_size!=node_info._data_len))
			{
				if(need_release)
				{
					EM_SAFE_DELETE(user_data);
					need_release=FALSE;
				}
				ret_val= INVALID_READ_SIZE;
				sd_assert(ret_val==SUCCESS);
				goto ErrorHanle;
			}
			pos_node_file_end+=read_size;

			ret_val = em_pwrite(file_id, (char *)user_data,read_size, pos_file_end , &writesize);
			if(need_release)
			{
				EM_SAFE_DELETE(user_data);
				need_release=FALSE;
			}
			sd_assert(ret_val==SUCCESS);
			if((ret_val!=SUCCESS)||(writesize!=read_size))
			{
				ret_val= INVALID_WRITE_SIZE;
				sd_assert(ret_val==SUCCESS);
				goto ErrorHanle;
			}
			pos_file_end+=writesize;
			if(!p_tree->_file_locked) goto ErrorHanle;
	      		EM_LOG_DEBUG("trm_clear_file_impl:data[%u]",writesize); 
		}
		
		
		/*  Name */
		if(node_info._name_len!=0)
		{
			em_memset(buffer,0x00,512);
			ret_val=em_pread(p_tree->_file_id, (char *)buffer,node_info._name_len, pos_node_file_end, &read_size);
			sd_assert(ret_val==SUCCESS);
			if((ret_val!=SUCCESS)||(read_size!=node_info._name_len))
			{
				ret_val= INVALID_READ_SIZE;
				sd_assert(ret_val==SUCCESS);
				goto ErrorHanle;
			}
			pos_node_file_end+=read_size;

			if(!p_tree->_file_locked) goto ErrorHanle;
			
			ret_val = em_pwrite(file_id, (char *)buffer,read_size, pos_file_end , &writesize);
			sd_assert(ret_val==SUCCESS);
			if((ret_val!=SUCCESS)||(writesize!=read_size))
			{
				ret_val= INVALID_WRITE_SIZE;
				sd_assert(ret_val==SUCCESS);
				goto ErrorHanle;
			}
			pos_file_end+=writesize;
			if(!p_tree->_file_locked) goto ErrorHanle;
      			EM_LOG_DEBUG("trm_clear_file_impl:node_name[%u]=%s",writesize,buffer); 
		}


		/*  Items */
		if(node_info._items_num!=0)
		{
			item_len = node_head._len - sizeof(TREE_NODE_INFO_STORE) - node_info._data_len - node_info._name_len;
			sd_assert(item_len == node_info._items_len);
			if(item_len>0)
			{
				if(item_len>512)
				{
					ret_val=em_malloc(item_len, (void**)&user_data);
					sd_assert(ret_val==SUCCESS);
					if(ret_val!=SUCCESS)
					{
						goto ErrorHanle;
					}
					need_release = TRUE;
				}
				else
				{
					user_data = buffer;
				}
				ret_val=em_pread(p_tree->_file_id, (char *)user_data,item_len, pos_node_file_end, &read_size);
				sd_assert(ret_val==SUCCESS);
				if((ret_val!=SUCCESS)||(read_size!=item_len))
				{
					if(need_release)
					{
						EM_SAFE_DELETE(user_data);
						need_release=FALSE;
					}
					ret_val= INVALID_READ_SIZE;
					sd_assert(ret_val==SUCCESS);
					goto ErrorHanle;
				}
				pos_node_file_end+=read_size;

				ret_val = em_pwrite(file_id, (char *)user_data,read_size, pos_file_end , &writesize);
				if(need_release)
				{
					EM_SAFE_DELETE(user_data);
					need_release=FALSE;
				}
				sd_assert(ret_val==SUCCESS);
				if((ret_val!=SUCCESS)||(writesize!=read_size))
				{
					ret_val= INVALID_WRITE_SIZE;
					sd_assert(ret_val==SUCCESS);
					goto ErrorHanle;
				}
				pos_file_end+=writesize;
	      			EM_LOG_DEBUG("trm_clear_file_impl:items[%u]",writesize); 
				if(!p_tree->_file_locked) goto ErrorHanle;
			}
			else
			{
				sd_assert(FALSE);
			}
		}
		
      		EM_LOG_DEBUG("trm_clear_file_impl:%u/%u node is ok!",++valid_node_num,invalid_node_num); 		
	}

      	EM_LOG_DEBUG("trm_clear_file_impl:all nodes(%u/%u) is done!",valid_node_num+invalid_node_num,total_node_num); 
		
	em_close_ex(p_tree->_file_id);
	p_tree->_file_id=0;
	
	em_close_ex(file_id);
	file_id = 0;
	
	ret_val= em_delete_file(file_path);
	sd_assert(ret_val==SUCCESS);
	if(ret_val!=SUCCESS) goto ErrorHanle;
	
	ret_val= em_rename_file(new_file_path,file_path);
	sd_assert(ret_val==SUCCESS);
	if(ret_val!=SUCCESS) goto ErrorHanle;

	p_tree->_pos_file_end=TRM_POS_NODE_START;
	
	//CHECK_FILE_OPEN_VOID;
      	/*  Open the  original tree file to read */;
	em_time_ms(&p_tree->_time_stamp); 
	if(p_tree->_file_id==0) 
	{
		if(SUCCESS!=em_open_ex(trm_get_tree_file_path(p_tree),  p_tree->_flag, &p_tree->_file_id)) 
		{
			sd_assert(FALSE);
			p_tree->_file_id = 0;
			goto ErrorHanle;
		}
	}

	
	ret_val = em_filesize(p_tree->_file_id, &filesize);
	sd_assert(ret_val==SUCCESS);
	if(ret_val!=SUCCESS) goto ErrorHanle;
	
	sd_assert(pos_file_end>=filesize-1);

	em_close_ex(p_tree->_file_id);
	p_tree->_file_id = 0;
	p_tree->_need_clear=FALSE;
	p_tree->_file_locked=FALSE;
	
      	EM_LOG_DEBUG("trm_clear_file_impl:SUCCESS:filesize=%llu,%llu",filesize,pos_file_end); 
	
	return SUCCESS;
ErrorHanle:
	if(file_id!=0)
	{
		em_close_ex(file_id);
		file_id = 0;
	}
	
	if(p_tree->_file_id!=0)
	{
		em_close_ex(p_tree->_file_id);
		p_tree->_file_id = 0;
	}
	
	p_tree->_pos_file_end=TRM_POS_NODE_START;
	p_tree->_file_locked=FALSE;
      	EM_LOG_DEBUG("trm_clear_file_impl:end:ret_val=%d",ret_val); 
	sd_assert(ret_val==SUCCESS);
	return ret_val;
}

_int32 trm_backup_file(TREE * p_tree)
{
	_int32 ret_val=SUCCESS;
	char * file_path = trm_get_tree_file_path(p_tree);
	char new_file_path[MAX_FULL_PATH_BUFFER_LEN];

      	EM_LOG_DEBUG("trm_backup_file:g_file_locked=%d,g_file_thread_id=%u,check_count=%u",p_tree->_file_locked,p_tree->_file_thread_id,p_tree->_backup_count); 

	if(p_tree->_file_locked) return SUCCESS;
	if(p_tree->_file_thread_id!=0) return SUCCESS;

	if(p_tree->_backup_count++>=MAX_FILE_BACKUP_COUNT)
  	{
  		p_tree->_backup_count = 0;
	}
	else
	{
		return SUCCESS;
	}
		
	em_memset(new_file_path,0, MAX_FULL_PATH_BUFFER_LEN);
	em_strncpy(new_file_path, file_path, MAX_FULL_PATH_BUFFER_LEN-4);
	em_strcat(new_file_path, ".bak", 4);

/* ipad下在排队io操作时删除文件很慢，经验证可直接copy file */
#ifndef MACOS
	if(em_file_exist(new_file_path))
	{
		em_delete_file(new_file_path);
	}
#endif

	trm_close_file(p_tree,TRUE);
	p_tree->_file_locked=TRUE;
	ret_val= em_copy_file(file_path,new_file_path);
	p_tree->_file_locked=FALSE;
	CHECK_VALUE(ret_val);
	
	return ret_val;
}
BOOL trm_recover_file(TREE * p_tree)
{
	_int32 ret_val=SUCCESS;
	char * file_path = trm_get_tree_file_path(p_tree);
	char new_file_path[MAX_FULL_PATH_BUFFER_LEN];

      	EM_LOG_DEBUG("trm_recover_file:g_file_locked=%d,g_file_thread_id=%u",p_tree->_file_locked,p_tree->_file_thread_id); 
		
	if(p_tree->_file_locked) return FALSE;
	if(p_tree->_file_thread_id!=0) return FALSE;

	em_memset(new_file_path,0, MAX_FULL_PATH_BUFFER_LEN);
	em_strncpy(new_file_path, file_path, MAX_FULL_PATH_BUFFER_LEN-4);
	em_strcat(new_file_path, ".bak", 4);
	
	trm_close_file(p_tree,TRUE);
	
	if(em_file_exist(file_path))
	{
		em_delete_file(file_path);
	}

	p_tree->_pos_file_end = TRM_POS_NODE_START;
	
	if(!em_file_exist(new_file_path))
	{
		return FALSE;
	}
	
	p_tree->_file_locked=TRUE;
	ret_val= em_rename_file(new_file_path,file_path);
	p_tree->_file_locked=FALSE;
	
	if(ret_val!=SUCCESS)
		return FALSE;
	else
		return TRUE;
}

_int32 trm_stop_clear_file(TREE * p_tree)
{
      	EM_LOG_DEBUG("trm_stop_clear_file:g_file_locked=%d,g_file_thread_id=%u",p_tree->_file_locked,p_tree->_file_thread_id); 
	if(p_tree->_file_locked)
	{
		p_tree->_file_locked=FALSE;
		em_sleep(5);
	}

	if(p_tree->_file_thread_id!=0)
	{
		em_finish_task(p_tree->_file_thread_id);
		p_tree->_file_thread_id=0;
	}
	return SUCCESS;
}

_int32 trm_get_total_node_num(TREE * p_tree,_u32 * total_node_num)
{
	_int32 ret_val=SUCCESS;
	_u32 readsize=0;
	
	CHECK_FILE_OPEN_INT(p_tree);
	
      	EM_LOG_DEBUG("trm_get_total_node_num"); 
	/* read from file */
	ret_val=em_pread(p_tree->_file_id, (char *)total_node_num,sizeof(_u32), TRM_POS_TOTAL_NODE_NUM, &readsize);
	CHECK_VALUE(ret_val);
	if((readsize!=0)&&(readsize!=sizeof(_u32)))
	{
		CHECK_VALUE( INVALID_READ_SIZE);
	}
	return SUCCESS;
}
_int32 trm_save_total_node_num(TREE * p_tree,_u32  total_node_num)
{
	_int32 ret_val=SUCCESS;
	_u32 writesize=0;
#ifdef STRONG_FILE_CHECK
	_u32 old_total_node_num=0,readsize=0;
#endif /* STRONG_FILE_CHECK */	

	trm_stop_clear_file(p_tree);
	CHECK_FILE_OPEN_INT(p_tree);
		
      	EM_LOG_DEBUG("trm_save_total_node_num"); 
#ifdef STRONG_FILE_CHECK
	/* read from file */
	ret_val=em_pread(p_tree->_file_id, (char *)&old_total_node_num,sizeof(_u32), TRM_POS_TOTAL_NODE_NUM, &readsize);
	CHECK_VALUE(ret_val);
	if((readsize!=0)&&(readsize!=sizeof(_u32)))
	{
		CHECK_VALUE( INVALID_READ_SIZE);
	}
#endif /* STRONG_FILE_CHECK */	
	ret_val = em_pwrite(p_tree->_file_id, (char *)&total_node_num, sizeof(_u32), TRM_POS_TOTAL_NODE_NUM , &writesize);
	CHECK_VALUE(ret_val);
	if(writesize!=sizeof(_u32))
	{
		CHECK_VALUE( INVALID_WRITE_SIZE);
	}
	return SUCCESS;
}

_int32 trm_get_node_crc_value(TREE_NODE* p_node,_u16 * p_crc_value,_u32 * p_data_len)
{
#if 1
	*p_data_len = sizeof(TREE_NODE_INFO_STORE) + p_node->_data_len + p_node->_name_len+p_node->_items_len;

	/* 没时间写crc校验的处理,以后有需要的话再加上 */
	*p_crc_value = 0xFFFF;
#else
	//rclist_head.crc = sd_add_crc16(rclist_head.crc, &count, sizeof(count));		
	//rclist_head.len+= sizeof(count);
	//rclist_head.crc = sd_inv_crc16(rclist_head.crc);

//// check
		//			crc = sd_add_crc16(crc, pdata, rclist_head.len);
		//			if(FALSE != sd_isvalid_crc16(rclist_head.crc, crc))
////////////////////////
	_u32 data_len=0;
	_u16 crc_value=0xffff;

      EM_LOG_DEBUG("trm_get_node_crc_value"); 
	
	crc_value = em_add_crc16(crc_value, &p_node->_node_id, sizeof(_u32));
	data_len+=sizeof(_u32);

	if(p_node->_parent!=NULL)
	{
		crc_value = em_add_crc16(crc_value, &p_node->_parent->_node_id, sizeof(_u32));
		data_len+=sizeof(_u32);
	}

	if(p_node->_pre_node!=NULL)
	{
		crc_value = em_add_crc16(crc_value, &p_node->_pre_node->_node_id, sizeof(_u32));
		data_len+=sizeof(_u32);
	}

	if(p_node->_nxt_node!=NULL)
	{
		crc_value = em_add_crc16(crc_value, &p_node->_nxt_node->_node_id, sizeof(_u32));
		data_len+=sizeof(_u32);
	}
	
	if(p_node->_first_child!=NULL)
	{
		crc_value = em_add_crc16(crc_value, &p_node->_first_child->_node_id, sizeof(_u32));
		data_len+=sizeof(_u32);
	}
	
	if(p_node->_last_child!=NULL)
	{
		crc_value = em_add_crc16(crc_value, &p_node->_last_child->_node_id, sizeof(_u32));
		data_len+=sizeof(_u32);
	}
	
	crc_value = em_add_crc16(crc_value, &p_node->_children_num, sizeof(_u32));
	data_len+=sizeof(_u32);

	crc_value = em_add_crc16(crc_value, &p_node->_data_len, sizeof(_u32));
	data_len+=sizeof(_u32);

	crc_value = em_add_crc16(crc_value, &p_node->_name_len, sizeof(_u32));
	data_len+=sizeof(_u32);

	crc_value = em_add_crc16(crc_value, p_node->_data, p_node->_data_len);
	data_len+=p_node->_data_len;

	crc_value = em_add_crc16(crc_value, p_node->_name, p_node->_name_len);
	data_len+=p_node->_name_len;

	crc_value = em_inv_crc16(crc_value);
	*p_crc_value=crc_value;
	*p_data_len=data_len;
#endif
	return SUCCESS;
}

_int32 trm_add_node_to_file(TREE * p_tree,TREE_NODE* p_node)
{
	_int32 ret_val=SUCCESS;
	_u32 writesize=0;
#ifdef STRONG_FILE_CHECK
	_u64 filesize = 0;
#endif /* STRONG_FILE_CHECK */		
	TREE_NODE_STORE tree_node_store;

	sd_assert(p_node->_full_info!=FALSE);

	trm_stop_clear_file(p_tree);

	CHECK_FILE_OPEN_INT(p_tree);

      EM_LOG_DEBUG("trm_add_node_to_file:node_id=%u",p_node->_node_id); 

	em_memset(&tree_node_store, 0, sizeof(TREE_NODE_STORE));
	tree_node_store._head._valid= 1;
	
	ret_val=trm_get_node_crc_value(p_node,&tree_node_store._head._crc,&tree_node_store._head._len);
	CHECK_VALUE(ret_val);

	trm_copy_node_to_info_store(p_node,&tree_node_store._info);
	
	if(p_node->_offset==0)
	{
		/* write to the end of the task file */
		sd_assert(p_tree->_pos_file_end>=TRM_POS_NODE_START);
#ifdef STRONG_FILE_CHECK
		ret_val = em_filesize(p_tree->_file_id, &filesize);
		CHECK_VALUE(ret_val);
		
		sd_assert(p_tree->_pos_file_end>=filesize-1);
#endif /* STRONG_FILE_CHECK */	
		p_node->_offset = p_tree->_pos_file_end;

		/* TREE_NODE_STORE */
		writesize=0;
		ret_val = em_pwrite(p_tree->_file_id, (char *)&tree_node_store, sizeof(TREE_NODE_STORE), p_node->_offset , &writesize);
		CHECK_VALUE(ret_val);
		p_tree->_pos_file_end+= writesize;
		if(writesize!=sizeof(TREE_NODE_STORE))
		{
			CHECK_VALUE( INVALID_WRITE_SIZE);
		}
		
		/* data */
		if(p_node->_data_len!=0)
		{
			writesize=0;
			ret_val = em_pwrite(p_tree->_file_id, (char *)p_node->_data, p_node->_data_len, TRM_POS_DATA , &writesize);
			CHECK_VALUE(ret_val);
			p_tree->_pos_file_end+= writesize;
			if(writesize!=p_node->_data_len)
			{
				CHECK_VALUE( INVALID_WRITE_SIZE);
			}
		}
		
		/* name */
		if(p_node->_name_len!=0)
		{
			writesize=0;
			ret_val = em_pwrite(p_tree->_file_id, (char *)p_node->_name, p_node->_name_len, TRM_POS_NAME , &writesize);
			CHECK_VALUE(ret_val);
			p_tree->_pos_file_end+= writesize;
			if(writesize!=p_node->_name_len)
			{
				CHECK_VALUE( INVALID_WRITE_SIZE);
			}
		}

		/* items */
		writesize=0;
		if(p_node->_items_len!=0)
		{
			sd_assert(p_node->_items_num!=0);
			ret_val = em_pwrite(p_tree->_file_id, (char *)p_node->_items, p_node->_items_len, TRM_POS_ITEM , &writesize);
			CHECK_VALUE(ret_val);
			p_tree->_pos_file_end+= writesize;
			if(writesize!=p_node->_items_len)
			{
				CHECK_VALUE( INVALID_WRITE_SIZE);
			}
		}
		p_node->_change_flag = 0;
	}
	else
	{
		sd_assert(FALSE);
	}
      EM_LOG_DEBUG("trm_add_node_to_file success!:node_id=%u",p_node->_node_id); 
	return SUCCESS;
}

_int32 trm_disable_node_in_file(TREE * p_tree,TREE_NODE* p_node)
{
	_int32 ret_val=SUCCESS;
	_u32 writesize=0;
	_u16 valid_flag=0;	
#ifdef STRONG_FILE_CHECK		
	_u32 readsize=0;
	TREE_NODE_STORE tree_node_store;
#endif /* STRONG_FILE_CHECK */		
	
	trm_stop_clear_file(p_tree);
	
	CHECK_FILE_OPEN_INT(p_tree);
	
		/* write to file */
		writesize=0;
		if(p_node->_offset<TRM_POS_NODE_START)
		{
	      		EM_LOG_ERROR("trm_disable_node_in_file:ERROR:p_node->_offset(%u)<TRM_POS_NODE_START(%u)",p_node->_offset,TRM_POS_NODE_START); 
			sd_assert(FALSE);
			return INVALID_NODE_ID;
		}
#ifdef STRONG_FILE_CHECK		
		/* read from file */
		/* TREE_NODE_STORE */
		ret_val=em_pread(p_tree->_file_id, (char *)&tree_node_store,sizeof(TREE_NODE_STORE), p_node->_offset, &readsize);
		if((ret_val!=SUCCESS)||(readsize!=sizeof(TREE_NODE_STORE)))
		{
	      		EM_LOG_ERROR("trm_disable_node_in_file:ERROR1:ret_val=%d,readsize(%u)<sizeof(TREE_NODE_STORE)(%u)",ret_val,readsize,sizeof(TREE_NODE_STORE)); 
			sd_assert(FALSE);
			return INVALID_READ_SIZE;
		}
		
		if(tree_node_store._head._valid==0)
		{
	      		EM_LOG_ERROR("trm_disable_node_in_file:ERROR2:tree_node_store._valid==0"); 
			sd_assert(FALSE);
			return INVALID_NODE_ID;
		}

		if(!p_node->_is_root)  //root id随机生成
		{
			if(tree_node_store._info._node_id!=p_node->_node_id)
			{
		      		EM_LOG_ERROR("trm_disable_node_in_file:ERROR3:tree_node_store._node_id(%u)!=p_node->_node_id(%u)",tree_node_store._info._node_id,p_node->_node_id); 
				sd_assert(FALSE);
				return INVALID_NODE_ID;
			}
		}
#endif /* STRONG_FILE_CHECK */		
	
	ret_val = em_pwrite(p_tree->_file_id, (char *)&valid_flag, sizeof(_u16), TRM_POS_VALID , &writesize);
	CHECK_VALUE(ret_val);
	if(writesize!=sizeof(_u16))
	{
		CHECK_VALUE( INVALID_WRITE_SIZE);
	}
	
      EM_LOG_DEBUG("trm_disable_node_in_file:node_id=%u",p_node->_node_id); 
	return SUCCESS;
}
TREE_NODE *  trm_get_node_from_file(TREE * p_tree,_int32 * err_code)
{
	static TREE_NODE node;
	_int32 ret_val=SUCCESS;
	_u32 readsize=0;
	TREE_NODE_STORE_HEAD tree_node_store_head;
	TREE_NODE_INFO_STORE tree_node_info_store;
	_u64 filesize = 0;
	//TREE_NODE_STORE tree_node_store;
	//void * data_buffer = NULL;
	//char * name_buffer = NULL;
	*err_code = SUCCESS;

	trm_stop_clear_file(p_tree);
	
      EM_LOG_DEBUG("trm_get_node_from_file:g_tree_file_id=%u,g_pos_tree_file_end=%u,TRM_POS_NODE_START=%u",p_tree->_file_id,p_tree->_pos_file_end,TRM_POS_NODE_START); 

	CHECK_FILE_OPEN(p_tree);
	
	ret_val = em_filesize(p_tree->_file_id, &filesize);
	if(ret_val!=SUCCESS)
	{
		*err_code = ret_val;
		return NULL;
	}
	
	em_memset(&node,0,sizeof(TREE_NODE));

READ_NEXT:	
	sd_assert(p_tree->_pos_file_end>=TRM_POS_NODE_START);
	em_memset(&tree_node_store_head,0,sizeof(TREE_NODE_STORE_HEAD));
	/* read TREE_NODE_STORE_HEAD from file */
	readsize = 0;
	ret_val=em_pread(p_tree->_file_id, (char *)&tree_node_store_head,sizeof(TREE_NODE_STORE_HEAD), p_tree->_pos_file_end, &readsize);
	if((ret_val!=SUCCESS)||(readsize!=sizeof(TREE_NODE_STORE_HEAD)))
	{
		if(p_tree->_invalid_node_count>=MAX_INVALID_NODE_NUM)
		{
			p_tree->_need_clear=TRUE;
		}
      		EM_LOG_ERROR("trm_get_node_from_file:ret_val=%d,readsize=%u,sizeof(TREE_NODE_STORE_HEAD)=%d,invalid_node_count=%d,g_pos_tree_file_end=%u",ret_val,readsize,sizeof(TREE_NODE_STORE_HEAD),p_tree->_invalid_node_count,p_tree->_pos_file_end); 
		p_tree->_invalid_node_count = 0;
		return NULL;
	}

	EM_LOG_DEBUG("get_node[%u]:valid=%u,crc=%u,len=%u",p_tree->_pos_file_end,	tree_node_store_head._valid,tree_node_store_head._crc,tree_node_store_head._len);

	p_tree->_pos_file_end+=readsize;
	if(tree_node_store_head._valid==0)
	{
		p_tree->_pos_file_end+=tree_node_store_head._len;
		p_tree->_invalid_node_count++;
		goto READ_NEXT;
	}
	
	sd_assert(tree_node_store_head._valid==1);
	if(tree_node_store_head._valid!=1)
	{
		/* 文件已经被破坏，不可再用 */
		EM_LOG_ERROR("trm_get_node_from_file:FATAL ERROR:valid=%d,invalid_node_count=%d,g_pos_tree_file_end=%u",tree_node_store_head._valid,p_tree->_invalid_node_count,p_tree->_pos_file_end); 
		p_tree->_invalid_node_count = 0;
		p_tree->_pos_file_end-=sizeof(TREE_NODE_STORE_HEAD);
		*err_code = READ_TREE_FILE_ERR;
		return NULL;
	}
	
	  if(filesize<p_tree->_pos_file_end+tree_node_store_head._len)
	  {
		/* 文件已经被破坏，不可再用 */
		EM_LOG_ERROR("trm_get_node_from_file:FATAL ERROR2:invalid_node_count=%d,g_pos_tree_file_end=%u,filesize=%llu",p_tree->_invalid_node_count,p_tree->_pos_file_end,filesize); 
		p_tree->_invalid_node_count = 0;
		p_tree->_pos_file_end-=sizeof(TREE_NODE_STORE_HEAD);
		*err_code = READ_TREE_FILE_ERR;
		return NULL;
	  }
	  
	em_memset(&tree_node_info_store,0,sizeof(TREE_NODE_INFO_STORE));
	/* read TREE_NODE_INFO_STORE from file */
	readsize = 0;
	ret_val=em_pread(p_tree->_file_id, (char *)&tree_node_info_store,sizeof(TREE_NODE_INFO_STORE), p_tree->_pos_file_end, &readsize);
	if((ret_val!=SUCCESS)||(readsize!=sizeof(TREE_NODE_INFO_STORE)))
	{
		sd_assert(FALSE);
		EM_LOG_ERROR("trm_get_node_from_file:FATAL ERROR3:ret_val=%d,readsize=%u,sizeof(TREE_NODE_INFO_STORE)=%d,invalid_node_count=%d,g_pos_tree_file_end=%u",ret_val,readsize,sizeof(TREE_NODE_INFO_STORE),p_tree->_invalid_node_count,p_tree->_pos_file_end); 
		p_tree->_invalid_node_count = 0;
		p_tree->_pos_file_end-=sizeof(TREE_NODE_STORE_HEAD);
		*err_code = READ_TREE_FILE_ERR;
		return NULL;
	}

	EM_LOG_DEBUG("get_valid_node:is_root=%d,id=%u,parent=%u,pre=%u,nxt=%u,1st=%u,last=%u,ch_num=%u,data_len=%u,name_len=%u,items_num=%u,items_len=%u",
				tree_node_info_store._is_root,tree_node_info_store._node_id,tree_node_info_store._parent_id,tree_node_info_store._pre_node_id,tree_node_info_store._nxt_node_id,tree_node_info_store._first_child_id,tree_node_info_store._last_child_id,tree_node_info_store._children_num,tree_node_info_store._data_len,tree_node_info_store._name_len,tree_node_info_store._items_num,tree_node_info_store._items_len);

	trm_copy_info_store_to_node(&tree_node_info_store,&node);

	node._full_info = FALSE;
	node._change_flag = 0;
	node._tree = (void*)p_tree;

	node._offset= p_tree->_pos_file_end-sizeof(TREE_NODE_STORE_HEAD);
	p_tree->_pos_file_end+=readsize;
#if 1
	p_tree->_pos_file_end+=tree_node_store_head._len-sizeof(TREE_NODE_INFO_STORE);
      EM_LOG_DEBUG("trm_get_node_from_file:node_id=%u,pos_file_end=%u,filesize=%llu",node._node_id,p_tree->_pos_file_end,filesize); 
	  
	return &node;
#else
	if(node._data_len!=0)
	{
		ret_val = em_malloc(node._data_len, (void**)&data_buffer);
		if(ret_val!=SUCCESS) goto ErrorHanle;
		
		/* read from file */
		readsize = 0;
		ret_val=em_pread(g_tree_file_id, (char *)data_buffer,node._data_len, g_pos_tree_file_end, &readsize);
		//sd_assert(ret_val==SUCCESS);
		if((ret_val!=SUCCESS)||(readsize!=node._data_len))
		{
      			EM_LOG_ERROR("trm_get_node_from_file:ERROR:ret_val=%d,readsize=%u,node._data_len=%u,g_pos_tree_file_end=%u",ret_val,readsize,node._data_len,g_pos_tree_file_end); 
			sd_assert(FALSE);
			goto ErrorHanle;
		}
		g_pos_tree_file_end+=readsize;
		
	}	

	if(node._name_len!=0)
	{
		ret_val= trm_node_name_malloc(&name_buffer);
		//ret_val = em_sd_malloc(node._name_len+1, (void**)&name_buffer);
		if(ret_val!=SUCCESS) goto ErrorHanle;

		//sd_memset(name_buffer,0,node._name_len+1);
		
		/* read from file */
		readsize = 0;
		ret_val=em_pread(g_tree_file_id, (char *)name_buffer,node._name_len, g_pos_tree_file_end, &readsize);
		//sd_assert(ret_val==SUCCESS);
		if((ret_val!=SUCCESS)||(readsize!=node._name_len))
		{
      			EM_LOG_ERROR("trm_get_node_from_file:ERROR 2:ret_val=%d,readsize=%u,node._name_len=%u,g_pos_tree_file_end=%u",ret_val,readsize,node._name_len,g_pos_tree_file_end); 
			sd_assert(FALSE);
			goto ErrorHanle;
		}
		g_pos_tree_file_end+=readsize;
		
	}	

	node._data = data_buffer;
	node._name = name_buffer;

	/* need check the crc value .... */
	
      EM_LOG_DEBUG("trm_get_node_from_file:node_id=%u",node._node_id); 
	return &node;

ErrorHanle:
		EM_SAFE_DELETE(data_buffer);
		trm_node_name_free(name_buffer);
		if(p_tree->_invalid_node_count>=MAX_INVALID_NODE_NUM)
		{
			g_need_clear=TRUE;
		}
		sd_assert(FALSE);
		p_tree->_invalid_node_count = 0;
		return NULL;
#endif	
}

_int32 trm_copy_node_to_info_store(TREE_NODE * p_node,TREE_NODE_INFO_STORE* p_tree_node_info_store)
{
	p_tree_node_info_store->_node_id = p_node->_node_id;
	p_tree_node_info_store->_parent_id = 0;
	if(p_node->_parent!=NULL)
	{
		p_tree_node_info_store->_parent_id=p_node->_parent->_node_id;
	}
	
	p_tree_node_info_store->_pre_node_id = 0;
	if(p_node->_pre_node!=NULL)
	{
		p_tree_node_info_store->_pre_node_id=p_node->_pre_node->_node_id;
	}

	p_tree_node_info_store->_nxt_node_id = 0;
	if(p_node->_nxt_node!=NULL)
	{
		p_tree_node_info_store->_nxt_node_id=p_node->_nxt_node->_node_id;
	}

	p_tree_node_info_store->_first_child_id = 0;
	if(p_node->_first_child!=NULL)
	{
		p_tree_node_info_store->_first_child_id=p_node->_first_child->_node_id;
	}

	p_tree_node_info_store->_last_child_id = 0;
	if(p_node->_last_child!=NULL)
	{
		p_tree_node_info_store->_last_child_id=p_node->_last_child->_node_id;
	}
	
	p_tree_node_info_store->_children_num=p_node->_children_num;
	p_tree_node_info_store->_data_len=p_node->_data_len;
	p_tree_node_info_store->_name_len=p_node->_name_len;
	/* new */
	p_tree_node_info_store->_is_root=p_node->_is_root;
	p_tree_node_info_store->_data_hash=p_node->_data_hash;
	p_tree_node_info_store->_name_hash=p_node->_name_hash;
	p_tree_node_info_store->_create_time= p_node->_create_time;
	p_tree_node_info_store->_modify_time= p_node->_modify_time;
	p_tree_node_info_store->_children_sort_mode=p_node->_children_sort_mode;
	p_tree_node_info_store->_reserved=p_node->_reserved;
	p_tree_node_info_store->_items_num=p_node->_items_num;
	p_tree_node_info_store->_items_len=p_node->_items_len;
	return SUCCESS;
}
_int32 trm_copy_info_store_to_node(TREE_NODE_INFO_STORE* p_tree_node_info_store,TREE_NODE * p_node)
{
	p_node->_node_id = p_tree_node_info_store->_node_id;
	if(p_tree_node_info_store->_parent_id!=0)
	{
		p_node->_parent= (TREE_NODE*)p_tree_node_info_store->_parent_id;
	}
	if(p_tree_node_info_store->_pre_node_id!=0)
	{
		p_node->_pre_node= (TREE_NODE*)p_tree_node_info_store->_pre_node_id;
	}
	if(p_tree_node_info_store->_nxt_node_id!=0)
	{
		p_node->_nxt_node= (TREE_NODE*)p_tree_node_info_store->_nxt_node_id;
	}
	if(p_tree_node_info_store->_first_child_id!=0)
	{
		p_node->_first_child= (TREE_NODE*)p_tree_node_info_store->_first_child_id;
	}
	if(p_tree_node_info_store->_last_child_id!=0)
	{
		p_node->_last_child= (TREE_NODE*)p_tree_node_info_store->_last_child_id;
	}
	p_node->_children_num= p_tree_node_info_store->_children_num;
	p_node->_data_len= p_tree_node_info_store->_data_len;
	p_node->_name_len= p_tree_node_info_store->_name_len;
	/* new */
	p_node->_is_root= p_tree_node_info_store->_is_root;
	p_node->_data_hash= p_tree_node_info_store->_data_hash;
	p_node->_name_hash= p_tree_node_info_store->_name_hash;
	p_node->_create_time= p_tree_node_info_store->_create_time;
	p_node->_modify_time= p_tree_node_info_store->_modify_time;
	p_node->_children_sort_mode= p_tree_node_info_store->_children_sort_mode;
	p_node->_reserved= p_tree_node_info_store->_reserved;
	p_node->_items_num= p_tree_node_info_store->_items_num;
	p_node->_items_len= p_tree_node_info_store->_items_len;
	return SUCCESS;
}
_int32 trm_save_len_changed_node_to_file(TREE * p_tree,TREE_NODE * p_node)
{
	_int32 ret_val=SUCCESS;
	_u32 readsize=0;
	TREE_NODE node_tmp;
	char *data=NULL,*name=NULL,*item=NULL;
	BOOL need_release_data = FALSE,need_release_name=FALSE,need_release_item=FALSE;
	
		if(p_node->_full_info)
		{
			ret_val =  trm_disable_node_in_file(p_tree,p_node);
			CHECK_VALUE(ret_val);
			
			p_node->_offset = 0;
			ret_val =  trm_add_node_to_file(p_tree,p_node);
			CHECK_VALUE(ret_val);
		}
		else
		{
			em_memcpy(&node_tmp,p_node,sizeof(TREE_NODE));
			if((p_node->_change_flag&TRN_CHANGE_DATA)==0)
			{
				/* data没有改动,从原来的位置读出来 */
				if((p_node->_data==NULL)&&(p_node->_data_len!=0))
				{
					ret_val=em_malloc(p_node->_data_len, (void**)&data);
					CHECK_VALUE( ret_val);
					need_release_data = TRUE;
					readsize = 0;
					ret_val=em_pread(p_tree->_file_id, (char *)data,p_node->_data_len, TRM_POS_DATA, &readsize);
					sd_assert(ret_val==SUCCESS);
					if((ret_val!=SUCCESS)||(readsize!=p_node->_data_len))
					{
						ret_val = INVALID_READ_SIZE;
						goto ErrorHanle;
					}
					node_tmp._data = (void*)data;
				}
			}

			if((p_node->_change_flag&TRN_CHANGE_NAME)==0)
			{
				/* name没有改动,从原来的位置读出来 */
				if((p_node->_name==NULL)&&(p_node->_name_len!=0))
				{
					ret_val=em_malloc(p_node->_name_len, (void**)&name);
					if( ret_val!=SUCCESS)
					{
						goto ErrorHanle;
					}
					need_release_name = TRUE;
					readsize = 0;
					ret_val=em_pread(p_tree->_file_id, (char *)name,p_node->_name_len, TRM_POS_NAME, &readsize);
					sd_assert(ret_val==SUCCESS);
					if((ret_val!=SUCCESS)||(readsize!=p_node->_name_len))
					{
						ret_val = INVALID_READ_SIZE;
						goto ErrorHanle;
					}
					node_tmp._name = name;
				}
			}
			
			if((p_node->_change_flag&TRN_CHANGE_ITEMS)==0)
			{
				/* items没有改动,从原来的位置读出来 */
				if((p_node->_items==NULL)&&(p_node->_items_len!=0))
				{
					sd_assert(p_node->_items_num!=0);
					ret_val=em_malloc(p_node->_items_len, (void**)&item);
					if( ret_val!=SUCCESS)
					{
						goto ErrorHanle;
					}
					need_release_item = TRUE;
					readsize = 0;
					ret_val=em_pread(p_tree->_file_id, (char *)item,p_node->_items_len, TRM_POS_ITEM, &readsize);
					sd_assert(ret_val==SUCCESS);
					if((ret_val!=SUCCESS)||(readsize!=p_node->_items_len))
					{
						ret_val = INVALID_READ_SIZE;
						goto ErrorHanle;
					}
					node_tmp._items = (void*)item;
				}
			}
			ret_val =  trm_disable_node_in_file(p_tree,p_node);
			if(ret_val!=SUCCESS)
			{
				goto ErrorHanle;
			}
			
			node_tmp._full_info = TRUE;
			node_tmp._offset = 0;
			ret_val =  trm_add_node_to_file(p_tree,&node_tmp);
			if(ret_val!=SUCCESS)
			{
				goto ErrorHanle;
			}
			
			p_node->_offset = node_tmp._offset;
		}
		p_node->_change_flag = 0;
	return SUCCESS;
	
ErrorHanle:
	if(need_release_data)
	{
		EM_SAFE_DELETE(data);
		need_release_data=FALSE;
	}
	if(need_release_name)
	{
		EM_SAFE_DELETE(name);
		need_release_name=FALSE;
	}
	if(need_release_item)
	{
		EM_SAFE_DELETE(item);
		need_release_item=FALSE;
	}
	CHECK_VALUE(ret_val);
	return ret_val;
}
_int32 trm_save_node_to_file(TREE * p_tree,TREE_NODE * p_node)
{
	_int32 ret_val=SUCCESS;
	_u32 writesize=0,readsize=0;
	TREE_NODE_STORE tree_node_store;

	trm_stop_clear_file(p_tree);

	CHECK_FILE_OPEN_INT(p_tree);

      EM_LOG_DEBUG("trm_save_node_to_file:node_id=%u,_change_flag=%X",p_node->_node_id,p_node->_change_flag); 

	  if(p_node->_change_flag!=0)
	  {
	  	/* 记录修改时间 */
	  	em_time_ms(&p_node->_modify_time);
	  }

	if(p_node->_change_flag&TRN_CHANGE_TOTAL_LEN)
	{
		/* 改变了长度,必须在文件中删掉旧的,把新的加到文件末尾 */
		ret_val =  trm_save_len_changed_node_to_file(p_tree,p_node);
		CHECK_VALUE(ret_val);
	}
	else
	if(p_node->_change_flag!=0)
	{
		/* 数据总长度没有改变,直接在文件的对应位置进行更新 */
		/* write to file */
		writesize=0;
		if(p_node->_offset<TRM_POS_NODE_START)
		{
	      		EM_LOG_ERROR("trm_save_node_to_file:ERROR:p_node->_offset(%u)<TRM_POS_NODE_START(%u)",p_node->_offset,TRM_POS_NODE_START); 
			sd_assert(FALSE);
			return INVALID_NODE_ID;
		}
#ifdef STRONG_FILE_CHECK		
		/* read from file */
		/* TREE_NODE_STORE */
		ret_val=em_pread(p_tree->_file_id, (char *)&tree_node_store,sizeof(TREE_NODE_STORE), p_node->_offset, &readsize);
		if((ret_val!=SUCCESS)||(readsize!=sizeof(TREE_NODE_STORE)))
		{
	      		EM_LOG_ERROR("trm_save_node_to_file:ERROR1:ret_val=%d,readsize(%u)<sizeof(TREE_NODE_STORE)(%u)",ret_val,readsize,sizeof(TREE_NODE_STORE)); 
			sd_assert(FALSE);
			return INVALID_READ_SIZE;
		}
		
		if(tree_node_store._head._valid==0)
		{
	      		EM_LOG_ERROR("trm_save_node_to_file:ERROR2:tree_node_store._valid==0"); 
			sd_assert(FALSE);
			return INVALID_NODE_ID;
		}

		if(!p_node->_is_root)
		{
			if(tree_node_store._info._node_id!=p_node->_node_id)
			{
		      		EM_LOG_ERROR("trm_save_node_to_file:ERROR3:tree_node_store._node_id(%u)!=p_node->_node_id(%u)",tree_node_store._info._node_id,p_node->_node_id); 
				sd_assert(FALSE);
				return INVALID_NODE_ID;
			}
		}
#endif /* STRONG_FILE_CHECK */	
		tree_node_store._head._valid = 1;

		ret_val=trm_get_node_crc_value(p_node,&tree_node_store._head._crc,&tree_node_store._head._len);
		CHECK_VALUE(ret_val);

		trm_copy_node_to_info_store(p_node,&tree_node_store._info);
			
		ret_val = em_pwrite(p_tree->_file_id, (char *)&tree_node_store, sizeof(TREE_NODE_STORE), p_node->_offset , &writesize);
		CHECK_VALUE(ret_val);
		if(writesize!=sizeof(TREE_NODE_STORE))
		{
			CHECK_VALUE( INVALID_WRITE_SIZE);
		}
		
		if(p_node->_change_flag&TRN_CHANGE_DATA)
		{
			/* write to file */
			writesize=0;
			ret_val = em_pwrite(p_tree->_file_id, (char *)p_node->_data, p_node->_data_len, TRM_POS_DATA, &writesize);
			CHECK_VALUE(ret_val);
			if(writesize!=p_node->_data_len)
			{
				CHECK_VALUE( INVALID_WRITE_SIZE);
			}
			
		}

		if(p_node->_change_flag&TRN_CHANGE_NAME)
		{
			/* write to file */
			writesize=0;
			ret_val = em_pwrite(p_tree->_file_id, (char *)p_node->_name, p_node->_name_len, TRM_POS_NAME, &writesize);
			CHECK_VALUE(ret_val);
			if(writesize!=p_node->_name_len)
			{
				CHECK_VALUE( INVALID_WRITE_SIZE);
			}
			
		}
		
		if(p_node->_change_flag&TRN_CHANGE_ITEMS)
		{
			/* write to file */
			writesize=0;
			ret_val = em_pwrite(p_tree->_file_id, (char *)p_node->_items, p_node->_items_len, TRM_POS_ITEM, &writesize);
			CHECK_VALUE(ret_val);
			if(writesize!=p_node->_items_len)
			{
				CHECK_VALUE( INVALID_WRITE_SIZE);
			}
			
		}

		p_node->_change_flag = 0;
	}

	
      EM_LOG_DEBUG("trm_save_node_to_file success!:node_id=%u",p_node->_node_id); 
	return ret_val;
}
char * trm_get_name_from_file(TREE * p_tree,TREE_NODE * p_node)
{
	_int32 ret_val=SUCCESS;
	_u32 readsize=0;
	static char name_buffer[MAX_FILE_NAME_BUFFER_LEN];

	if(p_node->_name_len==0) return NULL;
	
	trm_stop_clear_file(p_tree);

	CHECK_FILE_OPEN(p_tree);
	
	em_memset(name_buffer,0,MAX_FILE_NAME_BUFFER_LEN);

	/* read from file */
	ret_val=em_pread(p_tree->_file_id, (char *)name_buffer,p_node->_name_len, TRM_POS_NAME, &readsize);
	sd_assert(ret_val==SUCCESS);
	if((ret_val!=SUCCESS)||(readsize!=p_node->_name_len))
	{
		return NULL;
	}
	
      EM_LOG_DEBUG("trm_get_name_from_file success!:node_id=%u",p_node->_node_id); 
	sd_assert(em_strlen(name_buffer)>0);
	return name_buffer;
}

_int32  trm_get_data_from_file(TREE * p_tree,TREE_NODE * p_node,void* data_buffer,_u32 * buffer_len)
{
	_int32 ret_val=SUCCESS;
	_u32 readsize=0;

	trm_stop_clear_file(p_tree);

	CHECK_FILE_OPEN_INT(p_tree);

	if(*buffer_len<p_node->_data_len)
	{
		*buffer_len=p_node->_data_len;
		return NOT_ENOUGH_BUFFER;
	}
	
	em_memset(data_buffer,0,*buffer_len);

	/* read from file */
	ret_val=em_pread(p_tree->_file_id, (char *)data_buffer,p_node->_data_len, TRM_POS_DATA, &readsize);
	CHECK_VALUE(ret_val);
	if(readsize!=p_node->_data_len)
	{
		return INVALID_READ_SIZE;
	}

	*buffer_len=p_node->_data_len;
	return SUCCESS;
}
_int32  trm_get_items_from_file(TREE * p_tree,TREE_NODE * p_node,void* item_buffer,_u32 * buffer_len)
{
	_int32 ret_val=SUCCESS;
	_u32 readsize=0;

	trm_stop_clear_file(p_tree);

	CHECK_FILE_OPEN_INT(p_tree);

	if(*buffer_len<p_node->_items_len)
	{
		*buffer_len=p_node->_items_len;
		return NOT_ENOUGH_BUFFER;
	}
	
	em_memset(item_buffer,0,*buffer_len);

	/* read from file */
	ret_val=em_pread(p_tree->_file_id, (char *)item_buffer,p_node->_items_len, TRM_POS_ITEM, &readsize);
	CHECK_VALUE(ret_val);
	if(readsize!=p_node->_items_len)
	{
		return INVALID_READ_SIZE;
	}

	*buffer_len=p_node->_items_len;
	return SUCCESS;
}


_int32  trm_check_node_file(TREE * p_tree,_u32 node_num)
{
#ifdef STRONG_FILE_CHECK
	_u32 valid_node_count = 0,invalid_node_count=0;
	_int32 ret_val=SUCCESS;
	_u32 readsize=0,pos_tree_file_end=TRM_POS_NODE_START;
	TREE_NODE_STORE tree_node_store;
	TREE_NODE * p_node = NULL;
	_u64 filesize=0;

      EM_LOG_DEBUG("trm_check_node_file:g_tree_file_id=%u,node_num=%u,g_pos_tree_file_end=%u,TRM_POS_NODE_START=%u,check_count=%u",p_tree->_file_id,node_num,p_tree->_pos_file_end,TRM_POS_NODE_START,p_tree->_check_count); 

	if(p_tree->_check_count++>=MAX_FILE_CHECK_COUNT)
  	{
  		p_tree->_check_count = 0;
	}
	else
	{
		return SUCCESS;
	}
	  
	trm_stop_clear_file(p_tree);
	
	CHECK_FILE_OPEN_INT(p_tree);

	ret_val = em_filesize(p_tree->_file_id, &filesize);
	CHECK_VALUE(ret_val);
	
	while(TRUE)
	{
		sd_assert(pos_tree_file_end>=TRM_POS_NODE_START);
		if(pos_tree_file_end>=filesize-1)
		{
			if(node_num!=valid_node_count)
			{
		      		EM_LOG_ERROR("trm_check_node_file:ERROR:node_num(%u)!=valid_node_count(%u)",node_num,valid_node_count); 
				CHECK_VALUE(INVALID_NODE_ID);
			}
	      		EM_LOG_DEBUG("trm_check_node_file end:pos_tree_file_end=%u,filesize=%llu,valid_node_count=%u,invalid_node_count=%u",pos_tree_file_end,filesize,valid_node_count,invalid_node_count); 
			return SUCCESS;
		}
		
		em_memset(&tree_node_store,0,sizeof(TREE_NODE_STORE));
		/* read from file */
		readsize=0;
		ret_val=em_pread(p_tree->_file_id, (char *)&tree_node_store,sizeof(TREE_NODE_STORE), pos_tree_file_end, &readsize);
		//sd_assert(ret_val==SUCCESS);
		if((ret_val!=SUCCESS)||(readsize!=sizeof(TREE_NODE_STORE)))
		{
	      		EM_LOG_ERROR("trm_check_node_file:ERROR0:ret_val=%d,readsize=%u,sizeof(TREE_NODE_STORE)=%d,valid_node_count=%d,pos_tree_file_end=%u",ret_val,readsize,sizeof(TREE_NODE_STORE),valid_node_count,pos_tree_file_end); 
			CHECK_VALUE(INVALID_READ_SIZE);
		}

		if(tree_node_store._head._valid==0)
		{
			pos_tree_file_end+=readsize;
			pos_tree_file_end+=tree_node_store._head._len - sizeof(TREE_NODE_INFO_STORE);
			invalid_node_count++;
			continue;
		}
		
		sd_assert(tree_node_store._head._valid==1);
		if(tree_node_store._head._valid!=1)
		{
			/* 文件已经被破坏，不可再用 */
			EM_LOG_ERROR("trm_check_node_file:FATAL ERROR:valid=%d,invalid_node_count=%d,g_pos_tree_file_end=%u",tree_node_store._head._valid,p_tree->_invalid_node_count,p_tree->_pos_file_end); 
			CHECK_VALUE(NODE_NOT_FOUND);
		}
	
		p_node = trm_get_node_from_map(p_tree,tree_node_store._info._node_id);
		if(p_node==NULL) 
		{
	      		EM_LOG_ERROR("trm_check_node_file:ERROR2:_node_id=%u can not find in the map!",tree_node_store._info._node_id); 
			CHECK_VALUE(INVALID_NODE_ID);
		}
		
		if(p_node->_offset != pos_tree_file_end)
		{
	      		EM_LOG_ERROR("trm_check_node_file:ERROR3:_node_id=%u ,p_node->_offset(%u) != pos_task_file_end(%u)!",tree_node_store._info._node_id,p_node->_offset ,pos_tree_file_end); 
			CHECK_VALUE(INVALID_NODE_ID);
		}
		//////////////////////////////
		if(tree_node_store._info._parent_id!=0)
		{
			p_node = trm_get_node_from_map(p_tree,tree_node_store._info._parent_id);
			if(p_node==NULL) 
			{
		      		EM_LOG_ERROR("trm_check_node_file:ERROR4:node_id=%u 's parent_id(%u) can not find in the map!",tree_node_store._info._node_id,tree_node_store._info._parent_id); 
				CHECK_VALUE(INVALID_NODE_ID);
			}
		}
		
		if(tree_node_store._info._pre_node_id!=0)
		{
			p_node = trm_get_node_from_map(p_tree,tree_node_store._info._pre_node_id);
			if(p_node==NULL) 
			{
		      		EM_LOG_ERROR("trm_check_node_file:ERROR5:node_id=%u 's pre_node_id(%u) can not find in the map!",tree_node_store._info._node_id,tree_node_store._info._pre_node_id); 
				CHECK_VALUE(INVALID_NODE_ID);
			}
		}
		
		if(tree_node_store._info._nxt_node_id!=0)
		{
			p_node = trm_get_node_from_map(p_tree,tree_node_store._info._nxt_node_id);
			if(p_node==NULL) 
			{
		      		EM_LOG_ERROR("trm_check_node_file:ERROR6:node_id=%u 's nxt_node_id(%u) can not find in the map!",tree_node_store._info._node_id,tree_node_store._info._nxt_node_id); 
				CHECK_VALUE(INVALID_NODE_ID);
			}
		}
		
		if(tree_node_store._info._first_child_id!=0)
		{
			p_node = trm_get_node_from_map(p_tree,tree_node_store._info._first_child_id);
			if(p_node==NULL) 
			{
		      		EM_LOG_ERROR("trm_check_node_file:ERROR7:node_id=%u 's first_child_id(%u) can not find in the map!",tree_node_store._info._node_id,tree_node_store._info._first_child_id); 
				CHECK_VALUE(INVALID_NODE_ID);
			}
		}
		
		if(tree_node_store._info._last_child_id!=0)
		{
			p_node = trm_get_node_from_map(p_tree,tree_node_store._info._last_child_id);
			if(p_node==NULL) 
			{
		      		EM_LOG_ERROR("trm_check_node_file:ERROR8:node_id=%u 's last_child_id(%u) can not find in the map!",tree_node_store._info._node_id,tree_node_store._info._last_child_id); 
				CHECK_VALUE(INVALID_NODE_ID);
			}
		}

		/////////////////////////////////
	      EM_LOG_DEBUG("trm_check_node_file ok:_node_id=%u",tree_node_store._info._node_id); 
		valid_node_count++;

		pos_tree_file_end+=readsize;
		pos_tree_file_end+=tree_node_store._head._len - sizeof(TREE_NODE_INFO_STORE);;

	}
#endif /* STRONG_FILE_CHECK */	
	return SUCCESS;
}

_int32  trm_convert_old_ver_to_current(TREE * p_tree)
{
	_int32 ret_val=SUCCESS;
	_u32 read_size=0,writesize=0,file_id=0,total_node_num=0,valid_node_num=0,invalid_node_num=0;
	EM_STORE_HEAD file_head;
	TREE_NODE_STORE_HEAD node_head;
	TREE_NODE_INFO_STORE node_info;
	TREE_NODE_STORE_OLD node_old;
	//TREE_NODE_STORE node;
	char * file_path = trm_get_tree_file_path(p_tree);
	char new_file_path[MAX_FULL_PATH_BUFFER_LEN];
	_u64 pos_file_end=TRM_POS_NODE_START,pos_node_file_end=TRM_POS_NODE_START_OLD;
	char buffer[512];
	char buffer_name[MAX_FILE_NAME_BUFFER_LEN];
	char * user_data=NULL;
	BOOL need_release = FALSE;
	_u64 filesize = 0;
	
      	EM_LOG_DEBUG("trm_convert_old_ver_to_current"); 
	sd_assert(p_tree->_file_id==0);

	sd_assert(p_tree->_file_locked!=TRUE);

	p_tree->_file_locked=TRUE;
	em_memset(new_file_path,0, MAX_FULL_PATH_BUFFER_LEN);
	em_strncpy(new_file_path, file_path, MAX_FULL_PATH_BUFFER_LEN-4);
	em_strcat(new_file_path, ".cur", 4);

	
	if(em_file_exist(new_file_path))
	{
		ret_val= em_delete_file(new_file_path);
		CHECK_VALUE(ret_val);
	}

      	/*  Open the  new  tree file to write */;
	ret_val= em_open_ex(new_file_path, O_FS_CREATE, &file_id);
	CHECK_VALUE(ret_val);
	
	if(!p_tree->_file_locked) goto ErrorHanle;
	
	ret_val = em_enlarge_file(file_id, TRM_POS_NODE_START+1, &filesize);
	sd_assert(ret_val==SUCCESS);
	if(ret_val!=SUCCESS) goto ErrorHanle;
	
      	EM_LOG_DEBUG("trm_convert_old_ver_to_current:file_id=%u,filesize=%llu",file_id,filesize); 
		
      	/*  Open the  original tree file to read */;
	em_time_ms(&p_tree->_time_stamp); 
	if(p_tree->_file_id==0) 
	{
		ret_val = em_open_ex(trm_get_tree_file_path(p_tree),  O_FS_RDONLY, &p_tree->_file_id);
		sd_assert(ret_val==SUCCESS);
		if(SUCCESS!=ret_val) 
		{
			p_tree->_file_id = 0;
			goto ErrorHanle;
		}
	}

      	/*  EM_STORE_HEAD */;
	ret_val = em_pread(p_tree->_file_id, (char*)&file_head, sizeof(EM_STORE_HEAD),0,&read_size);
	sd_assert(ret_val==SUCCESS);
	if((ret_val!=SUCCESS)||(read_size!=sizeof(EM_STORE_HEAD) ))
	{
		ret_val= INVALID_READ_SIZE;
		sd_assert(ret_val==SUCCESS);
		goto ErrorHanle;
	}
	
	if(!p_tree->_file_locked) goto ErrorHanle;

	sd_assert(file_head._ver == 1);
	if(file_head._ver != 1) 
	{
		ret_val= -1;
		goto ErrorHanle;
	}

	file_head._ver = TREE_STORE_FILE_VERSION;
	
	ret_val = em_pwrite(file_id, (char *)&file_head, sizeof(EM_STORE_HEAD), 0 , &writesize);
	sd_assert(ret_val==SUCCESS);
	if((ret_val!=SUCCESS)||(writesize!=sizeof(EM_STORE_HEAD)))
	{
		ret_val= INVALID_WRITE_SIZE;
		sd_assert(ret_val==SUCCESS);
		goto ErrorHanle;
	}

	if(!p_tree->_file_locked) goto ErrorHanle;
	
      	/*  total_node_num */;
	ret_val= trm_get_total_node_num(p_tree,&total_node_num);
	sd_assert(ret_val==SUCCESS);
	if(ret_val!=SUCCESS) goto ErrorHanle;

	if(!p_tree->_file_locked) goto ErrorHanle;
	
	ret_val = em_pwrite(file_id, (char *)&total_node_num, sizeof(_u32), TRM_POS_TOTAL_NODE_NUM , &writesize);
	sd_assert(ret_val==SUCCESS);
	if((ret_val!=SUCCESS)||(writesize!=sizeof(_u32)))
	{
		ret_val= INVALID_WRITE_SIZE;
		sd_assert(ret_val==SUCCESS);
		goto ErrorHanle;
	}
	
      	EM_LOG_DEBUG("trm_convert_old_ver_to_current:total_node_num=%u,pos_task_file_end=%llu",total_node_num,pos_node_file_end); 
		
      	/* 开始读节点信息 */
	while(p_tree->_file_locked)
	{
      		/*  TREE_NODE_STORE_HEAD  read */;
		ret_val = em_pread(p_tree->_file_id, (char*)&node_old, sizeof(TREE_NODE_STORE_OLD),pos_node_file_end,&read_size);
		sd_assert(ret_val==SUCCESS);
		if((ret_val!=SUCCESS)||(read_size!=sizeof(TREE_NODE_STORE_OLD) ))
		{
			if((ret_val==0)&&(read_size==0))
			{
				/* ok,Finished! */
      				EM_LOG_DEBUG("trm_convert_old_ver_to_current:end of reading nodes, pos_node_file_end=%llu",pos_node_file_end); 
				break;
			}
			
			if((ret_val==0)&&(read_size<sizeof(TREE_NODE_STORE_OLD)))
			{
				/* Broken file */
      				EM_LOG_DEBUG("trm_convert_old_ver_to_current:end of reading nodes, Broken file!pos_node_file_end=%llu",pos_node_file_end); 
				sd_assert(FALSE);
				break;
			}
			
			ret_val= INVALID_READ_SIZE;
			sd_assert(ret_val==SUCCESS);
			goto ErrorHanle;
		}

      		EM_LOG_DEBUG("trm_convert_old_ver_to_current:valid=%d",node_old._valid); 
			
		if(node_old._valid==0)
		{
			pos_node_file_end+=read_size;
			pos_node_file_end+=node_old._data_len;
			pos_node_file_end+=node_old._name_len;
			invalid_node_num++;
			continue;
		}

		pos_node_file_end+=read_size;

		/* data read */
		if(node_old._data_len!=0)
		{
			if(!p_tree->_file_locked) goto ErrorHanle;
			if(node_old._data_len>512)
			{
				ret_val=em_malloc(node_old._data_len, (void**)&user_data);
				sd_assert(ret_val==SUCCESS);
				if(ret_val!=SUCCESS)
				{
					goto ErrorHanle;
				}
				need_release = TRUE;
			}
			else
			{
				user_data = buffer;
			}
			read_size = 0;
			ret_val=em_pread(p_tree->_file_id, (char *)user_data,node_old._data_len, pos_node_file_end, &read_size);
			sd_assert(ret_val==SUCCESS);
			if((ret_val!=SUCCESS)||(read_size!=node_old._data_len))
			{
				if(need_release)
				{
					EM_SAFE_DELETE(user_data);
					need_release=FALSE;
				}
				ret_val= INVALID_READ_SIZE;
				sd_assert(ret_val==SUCCESS);
				goto ErrorHanle;
			}
			pos_node_file_end+=read_size;
		}
		
		
		/*  Name read */
		if(node_old._name_len!=0)
		{
			if(!p_tree->_file_locked) goto ErrorHanle;
			em_memset(buffer_name,0x00,MAX_FILE_NAME_BUFFER_LEN);
			ret_val=em_pread(p_tree->_file_id, (char *)buffer_name,node_old._name_len, pos_node_file_end, &read_size);
			sd_assert(ret_val==SUCCESS);
			if((ret_val!=SUCCESS)||(read_size!=node_old._name_len))
			{
				ret_val= INVALID_READ_SIZE;
				sd_assert(ret_val==SUCCESS);
				goto ErrorHanle;
			}
			pos_node_file_end+=read_size;
		}

		node_head._crc = 0xFFFF;
		node_head._valid = 1;
		node_head._len = sizeof(TREE_NODE_INFO_STORE) + node_old._data_len + node_old._name_len;

		if(!p_tree->_file_locked) goto ErrorHanle;
      		/*  TREE_NODE_STORE_HEAD write*/;
		ret_val = em_pwrite(file_id, (char *)&node_head,sizeof(TREE_NODE_STORE_HEAD), pos_file_end , &writesize);
		sd_assert(ret_val==SUCCESS);
		if((ret_val!=SUCCESS)||(writesize!=sizeof(TREE_NODE_STORE_HEAD)))
		{
			ret_val= INVALID_WRITE_SIZE;
			sd_assert(ret_val==SUCCESS);
			goto ErrorHanle;
		}
		pos_file_end+=writesize;
		
		if(!p_tree->_file_locked) goto ErrorHanle;
		em_memset(&node_info,0x00,sizeof(TREE_NODE_INFO_STORE));
		node_info._node_id = node_old._node_id;
		node_info._parent_id = node_old._parent_id;
		node_info._pre_node_id = node_old._pre_node_id;
		node_info._nxt_node_id = node_old._nxt_node_id;
		node_info._first_child_id = node_old._first_child_id;
		node_info._last_child_id = node_old._last_child_id;
		node_info._children_num = node_old._children_num;
		node_info._data_len = node_old._data_len;
		node_info._name_len = node_old._name_len;
		
		if(node_info._node_id == TRM_ROOT_NODE_ID)
		{
			node_info._is_root = TRUE;
		}
		else
		{
			node_info._is_root = FALSE;
		}
		
		if(node_info._parent_id == TRM_ROOT_NODE_ID)
		{
			node_info._parent_id = MAX_NODE_ID+TRM_ROOT_NODE_ID;
		}
		
		if(node_info._data_len!=0)
		{
			node_info._data_hash = trm_generate_data_hash(user_data,node_info._data_len);
		}
		
		if(node_info._name_len!=0)
		{
			node_info._name_hash = trm_generate_name_hash(buffer_name,node_info._name_len);
		}
		node_info._create_time = p_tree->_time_stamp;
		node_info._modify_time = p_tree->_time_stamp;
		
		if(!p_tree->_file_locked) goto ErrorHanle;
      		/*  TREE_NODE_INFO_STORE  write */;
		ret_val = em_pwrite(file_id, (char *)&node_info,sizeof(TREE_NODE_INFO_STORE), pos_file_end , &writesize);
		sd_assert(ret_val==SUCCESS);
		if((ret_val!=SUCCESS)||(writesize!=sizeof(TREE_NODE_INFO_STORE)))
		{
			ret_val= INVALID_WRITE_SIZE;
			sd_assert(ret_val==SUCCESS);
			goto ErrorHanle;
		}
		pos_file_end+=writesize;
		
		/* data write */
		if(node_info._data_len!=0)
		{
			if(!p_tree->_file_locked) goto ErrorHanle;
			ret_val = em_pwrite(file_id, (char *)user_data,node_info._data_len, pos_file_end , &writesize);
			if(need_release)
			{
				EM_SAFE_DELETE(user_data);
				need_release=FALSE;
			}
			sd_assert(ret_val==SUCCESS);
			if((ret_val!=SUCCESS)||(writesize!=node_info._data_len))
			{
				ret_val= INVALID_WRITE_SIZE;
				sd_assert(ret_val==SUCCESS);
				goto ErrorHanle;
			}
			pos_file_end+=writesize;
	      		EM_LOG_DEBUG("trm_convert_old_ver_to_current:data[%u]",writesize); 
		}
		
		
		/*  Name write */
		if(node_info._name_len!=0)
		{
			if(!p_tree->_file_locked) goto ErrorHanle;
			ret_val = em_pwrite(file_id, (char *)buffer_name,node_info._name_len, pos_file_end , &writesize);
			sd_assert(ret_val==SUCCESS);
			if((ret_val!=SUCCESS)||(writesize!=node_info._name_len))
			{
				ret_val= INVALID_WRITE_SIZE;
				sd_assert(ret_val==SUCCESS);
				goto ErrorHanle;
			}
			pos_file_end+=writesize;
      			EM_LOG_DEBUG("trm_convert_old_ver_to_current:node_name[%u]=%s",writesize,buffer); 
		}

      		EM_LOG_DEBUG("trm_convert_old_ver_to_current:%u/%u node is ok!",++valid_node_num,invalid_node_num); 		
	}

      	EM_LOG_DEBUG("trm_convert_old_ver_to_current:all nodes(%u/%u) is done!",valid_node_num+invalid_node_num,total_node_num); 
		
	em_close_ex(p_tree->_file_id);
	p_tree->_file_id=0;
	
	em_close_ex(file_id);
	file_id = 0;
	
	ret_val= em_delete_file(file_path);
	sd_assert(ret_val==SUCCESS);
	if(ret_val!=SUCCESS) goto ErrorHanle;
	
	ret_val= em_rename_file(new_file_path,file_path);
	sd_assert(ret_val==SUCCESS);
	if(ret_val!=SUCCESS) goto ErrorHanle;

	p_tree->_pos_file_end=TRM_POS_NODE_START;
	
	//CHECK_FILE_OPEN_VOID;
      	/*  Open the  original tree file to read */;
	em_time_ms(&p_tree->_time_stamp); 
	if(p_tree->_file_id==0) 
	{
		ret_val= em_open_ex(trm_get_tree_file_path(p_tree),  p_tree->_flag, &p_tree->_file_id);
		sd_assert(ret_val==SUCCESS);
		if(SUCCESS!=ret_val) 
		{
			p_tree->_file_id = 0;
			goto ErrorHanle;
		}
	}

	
	ret_val = em_filesize(p_tree->_file_id, &filesize);
	sd_assert(ret_val==SUCCESS);
	if(ret_val!=SUCCESS) goto ErrorHanle;
	
	sd_assert(pos_file_end>=filesize-1);

	p_tree->_need_clear=FALSE;
	p_tree->_file_locked=FALSE;
  	p_tree->_invalid_node_count=0;
	
      	EM_LOG_DEBUG("trm_convert_old_ver_to_current:SUCCESS:filesize=%llu,%llu",filesize,pos_file_end); 
	
	return SUCCESS;
ErrorHanle:
	if(need_release)
	{
		EM_SAFE_DELETE(user_data);
		need_release=FALSE;
	}
	if(file_id!=0)
	{
		em_close_ex(file_id);
		file_id = 0;
	}
	
	if(p_tree->_file_id!=0)
	{
		em_close_ex(p_tree->_file_id);
		p_tree->_file_id = 0;
	}
	
	em_delete_file(new_file_path);

	em_delete_file(trm_get_tree_file_path(p_tree));
	
	p_tree->_pos_file_end=TRM_POS_NODE_START;
	p_tree->_need_clear=FALSE;
	p_tree->_file_locked=FALSE;
  	p_tree->_invalid_node_count=0;
    	EM_LOG_DEBUG("trm_convert_old_ver_to_current:end:ret_val=%d",ret_val); 
	CHECK_VALUE(ret_val);
	return ret_val;
}

