#include "download_manager/download_task_store.h"
#include "download_manager/download_task_data.h"
#include "em_common/em_utility.h"
#include "scheduler/scheduler.h"
#include "em_interface/em_fs.h"
#include "em_common/em_logid.h"
#ifdef EM_LOGID
#undef EM_LOGID
#endif

#define EM_LOGID EM_LOGID_DOWNLOAD_TASK

#include "em_common/em_logger.h"

static _u32 g_task_file_id=0;
static _u32 g_task_file_time_stamp=0;
static _u32 g_pos_task_file_end=POS_TASK_START;
static BOOL g_need_clear=FALSE;
static BOOL g_file_locked=FALSE;
static _int32	g_file_thread_id = 0;
static BOOL running_task_loaded=FALSE;
static _u32 *running_task_ids =NULL;
#ifdef DT_STROE_DEBUG
static _u32 g_task_test_file_id=0;
#endif
////////////////////////////////////////////////////////////////////////////////////////////
#define CHECK_FILE_OPEN 		{em_time_ms(&g_task_file_time_stamp); if(g_task_file_id==0) {if(SUCCESS!=em_open_ex(dt_get_task_store_file_path(), O_FS_CREATE, &g_task_file_id)) {sd_assert(FALSE);return NULL;}}}
#define CHECK_FILE_OPEN_INT 	{em_time_ms(&g_task_file_time_stamp); if(g_task_file_id==0) {if(SUCCESS!=em_open_ex(dt_get_task_store_file_path(), O_FS_CREATE, &g_task_file_id)) CHECK_VALUE(-1);}}
#define CHECK_FILE_OPEN_VOID 		{em_time_ms(&g_task_file_time_stamp); if(g_task_file_id==0) {if(SUCCESS!=em_open_ex(dt_get_task_store_file_path(), O_FS_CREATE, &g_task_file_id)) {sd_assert(FALSE);return;}}}

/////////////////////////////////////////////////////////////////////////////////////////
char * dt_get_task_store_file_path(void)
{
	static char file_path[MAX_FULL_PATH_BUFFER_LEN];
	_int32 ret_val = SUCCESS;
	em_memset(file_path,0, MAX_FULL_PATH_BUFFER_LEN);
	ret_val = em_snprintf(file_path, MAX_FULL_PATH_BUFFER_LEN, "%s/%s", em_get_system_path(), TASK_STORE_FILE);
	sd_assert(em_strlen(file_path)>0);
	return file_path;
}
_int32 dt_close_task_file(BOOL force_close)
{
	_int32 ret_val=SUCCESS;
	_u32 cur_time=0;
	if(g_task_file_id==0) return SUCCESS;
	
	em_time_ms(&cur_time);
	if(force_close)
	{
     		EM_LOG_DEBUG("dt_close_task_file"); 
		dt_stop_clear_task_file();
		ret_val= em_close_ex(g_task_file_id);
		CHECK_VALUE(ret_val);
		g_task_file_id = 0;
		g_task_file_time_stamp = cur_time;
		//g_pos_task_file_end=POS_TASK_START;
	}
	else
	{
		if(TIME_SUBZ(cur_time, g_task_file_time_stamp)>MAX_FILE_IDLE_INTERVAL*2)
		{
     			EM_LOG_DEBUG("dt_close_task_file 2"); 
			ret_val= em_close_ex(g_task_file_id);
			CHECK_VALUE(ret_val);
			g_task_file_id = 0;
			g_task_file_time_stamp = cur_time;
		}
		/*
		else
		if((TIME_SUBZ(cur_time, g_task_file_time_stamp)>MAX_FILE_IDLE_INTERVAL)&&g_need_clear&&!g_file_locked)
		{
			dt_clear_task_file();
		}
		*/
	}

	return SUCCESS;
}
BOOL dt_is_task_file_need_clear_up(void)
{
	//_int32 ret_val=SUCCESS;
	_u32 cur_time=0;
	
	if(g_task_file_id!=0) return FALSE;
	
	em_time_ms(&cur_time);

	if((TIME_SUBZ(cur_time, g_task_file_time_stamp)>MAX_FILE_IDLE_INTERVAL)&&g_need_clear&&!g_file_locked)
	{
		return TRUE;
	}
	
	return FALSE;
}
_int32 dt_create_task_file(void)
{
	_int32 ret_val=SUCCESS;
	_u32 readsize=0,writesize=0;
	EM_STORE_HEAD file_head;
	char * file_path = dt_get_task_store_file_path();
	char new_file_path[MAX_FULL_PATH_BUFFER_LEN];
	_u32 running_task_ids[MAX_ALOW_TASKS];
	_u64 _cur_file_size=0;
	
      	EM_LOG_DEBUG("dt_create_task_file"); 

	g_pos_task_file_end = POS_TASK_START;
	
READ_AGAIN:		
	if(em_file_exist(file_path))
	{
		sd_assert(g_task_file_id==0);

		CHECK_FILE_OPEN_INT;
		
		ret_val=em_pread(g_task_file_id, (char *)&file_head,sizeof(EM_STORE_HEAD), 0, &readsize);
		if((ret_val==SUCCESS)&&(sizeof(EM_STORE_HEAD)==readsize)&&(file_head._ver == TASK_STORE_FILE_VERSION))
		{
			return SUCCESS;
		}

      		EM_LOG_ERROR("dt_create_task_file:ERROR:ret_val=%d,readsize=%u,sizeof(EM_STORE_HEAD)=%d,file_head._ver=%d",ret_val,readsize,sizeof(EM_STORE_HEAD),file_head._ver); 
		ret_val= em_close_ex(g_task_file_id);
		CHECK_VALUE(ret_val);
		g_task_file_id = 0;
		
		em_memset(new_file_path,0, MAX_FULL_PATH_BUFFER_LEN);
		em_strncpy(new_file_path, file_path, MAX_FULL_PATH_BUFFER_LEN-4);
		em_strcat(new_file_path, ".old", 4);
		if(em_file_exist(new_file_path))
		{
			em_delete_file(new_file_path);
		}
		
		ret_val= em_rename_file(file_path, new_file_path);
		CHECK_VALUE(ret_val);

		if(dt_recover_file())
		{
			goto READ_AGAIN;
		}
	}

	sd_assert(g_task_file_id==0);

	CHECK_FILE_OPEN_INT;

	file_head._crc = 0;
	file_head._ver = TASK_STORE_FILE_VERSION;
	file_head._len = 0;
		
	ret_val = em_pwrite(g_task_file_id, (char *)&file_head, sizeof(EM_STORE_HEAD), 0 , &writesize);
	if((ret_val!=SUCCESS)||(writesize!=sizeof(EM_STORE_HEAD)))
	{
		em_close_ex(g_task_file_id);
		CHECK_VALUE( INVALID_WRITE_SIZE);
	}
	
	ret_val = em_enlarge_file(g_task_file_id, g_pos_task_file_end+1, &_cur_file_size);
	if(ret_val!=SUCCESS)
	{
		em_close_ex(g_task_file_id);
		CHECK_VALUE( ret_val);
	}
	
	/* init the file */
	dt_save_total_task_num_to_file(0);
	em_memset(&running_task_ids,0,MAX_ALOW_TASKS*sizeof(_u32));
	dt_save_running_tasks_to_file(running_task_ids);
	dt_save_order_list_to_file(0,NULL);
	
      	EM_LOG_DEBUG("dt_create_task_file:SUCCESS"); 
	return SUCCESS;
}
_int32 dt_clear_task_file_impl(void* param)
{
	_int32 ret_val=SUCCESS;
	_u32 read_size=0,writesize=0,file_id=0,total_task_num=0,valid_task_num=0,invalid_task_num=0;
	EM_STORE_HEAD file_head;
	char * file_path = dt_get_task_store_file_path();
	char new_file_path[MAX_FULL_PATH_BUFFER_LEN];
	_u32 *running_task_ids=NULL;
	_u32 dling_task_num;
	_u32 *dling_task_order=NULL;	
	DT_TASK_STORE task_store;
	DT_TASK_STORE_HEAD task_head;
	_u64 pos_file_end=POS_TASK_START,pos_task_file_end=POS_TASK_START;
	char buffer[512];
	char * user_data=NULL;
	BOOL need_release = FALSE;
	_u16* dl_file_index_array=NULL;
	BT_FILE * file_array=NULL; 
	_u64 filesize = 0;

      	EM_LOG_DEBUG("dt_clear_task_file_impl"); 
	sd_assert(g_task_file_id==0);

	sd_assert(g_file_locked!=TRUE);

	g_file_locked=TRUE;
	em_memset(new_file_path,0, MAX_FULL_PATH_BUFFER_LEN);
	em_strncpy(new_file_path, file_path, MAX_FULL_PATH_BUFFER_LEN-4);
	em_strcat(new_file_path, ".tmp", 4);

	
	if(em_file_exist(new_file_path))
	{
		ret_val= em_delete_file(new_file_path);
		sd_assert(ret_val==SUCCESS);
		if(ret_val!=SUCCESS)goto ErrorHanle;
	}

	ret_val= em_open_ex(new_file_path, O_FS_CREATE, &file_id);
	sd_assert(ret_val==SUCCESS);
	if(ret_val!=SUCCESS) goto ErrorHanle;
	
	if(!g_file_locked) goto ErrorHanle;

	ret_val = em_enlarge_file(file_id, POS_TASK_START+1, &filesize);
	sd_assert(ret_val==SUCCESS);
	if(ret_val!=SUCCESS) goto ErrorHanle;
	
      	EM_LOG_DEBUG("dt_clear_task_file_impl:file_id=%u,filesize=%llu",file_id,filesize); 
		
      	CHECK_FILE_OPEN_INT;
	
	ret_val = em_pread(g_task_file_id, (char*)&file_head, sizeof(EM_STORE_HEAD),0,&read_size);
	sd_assert(ret_val==SUCCESS);
	if((ret_val!=SUCCESS)||(read_size!=sizeof(EM_STORE_HEAD) ))
	{
		ret_val= INVALID_READ_SIZE;
		sd_assert(ret_val==SUCCESS);
		goto ErrorHanle;
	}
	
	if(!g_file_locked) goto ErrorHanle;
	
	ret_val = em_pwrite(file_id, (char *)&file_head, sizeof(EM_STORE_HEAD), 0 , &writesize);
	sd_assert(ret_val==SUCCESS);
	if((ret_val!=SUCCESS)||(writesize!=sizeof(EM_STORE_HEAD)))
	{
		ret_val= INVALID_WRITE_SIZE;
		sd_assert(ret_val==SUCCESS);
		goto ErrorHanle;
	}

	if(!g_file_locked) goto ErrorHanle;
	
	ret_val= dt_get_total_task_num_from_file(&total_task_num);
	sd_assert(ret_val==SUCCESS);
	if(ret_val!=SUCCESS) goto ErrorHanle;

	if(!g_file_locked) goto ErrorHanle;
	
	ret_val = em_pwrite(file_id, (char *)&total_task_num, sizeof(_u32), POS_TOTAL_TASK_NUM , &writesize);
	sd_assert(ret_val==SUCCESS);
	if((ret_val!=SUCCESS)||(writesize!=sizeof(_u32)))
	{
		ret_val= INVALID_WRITE_SIZE;
		sd_assert(ret_val==SUCCESS);
		goto ErrorHanle;
	}

      	EM_LOG_DEBUG("dt_clear_task_file_impl:total_task_num=%u",total_task_num); 
	if(!g_file_locked) goto ErrorHanle;
	
	running_task_ids= dt_get_running_tasks_from_file();
	if(running_task_ids==NULL) goto ErrorHanle;

	if(!g_file_locked) goto ErrorHanle;
	
	ret_val = em_pwrite(file_id, (char *)running_task_ids, sizeof(_u32)*MAX_ALOW_TASKS, POS_RUNNING_TASK_ARRAY , &writesize);
	sd_assert(ret_val==SUCCESS);
	if((ret_val!=SUCCESS)||(writesize!=sizeof(_u32)*MAX_ALOW_TASKS))
	{
		ret_val= INVALID_WRITE_SIZE;
		sd_assert(ret_val==SUCCESS);
		goto ErrorHanle;
	}

	if(!g_file_locked) goto ErrorHanle;
	
	dling_task_num= dt_get_order_list_size_from_file();

	if(!g_file_locked) goto ErrorHanle;
	
	ret_val = em_pwrite(file_id, (char *)&dling_task_num, sizeof(_u32), POS_ORDER_LIST_SIZE , &writesize);
	sd_assert(ret_val==SUCCESS);
	if((ret_val!=SUCCESS)||(writesize!=sizeof(_u32)))
	{
		ret_val= INVALID_WRITE_SIZE;
		sd_assert(ret_val==SUCCESS);
		goto ErrorHanle;
	}

	if(!g_file_locked) goto ErrorHanle;
	
      	EM_LOG_DEBUG("dt_clear_task_file_impl:dling_task_num=%u",dling_task_num); 
	if(dling_task_num!=0)
	{
		if(dling_task_num*sizeof(_u32)>512)
		{
			ret_val=em_malloc(dling_task_num*sizeof(_u32), (void**)&dling_task_order);
			sd_assert(ret_val==SUCCESS);
			if(ret_val!=SUCCESS)
			{
				goto ErrorHanle;
			}
			need_release=TRUE;
		}
		else
		{
			dling_task_order=(_u32*)buffer;
		}

		ret_val=dt_get_order_list_from_file(dling_task_order,dling_task_num*sizeof(_u32));
		sd_assert(ret_val==SUCCESS);
		if((ret_val!=SUCCESS)||(!g_file_locked))
		{
			if(need_release)
			{
				EM_SAFE_DELETE(dling_task_order);
				need_release=FALSE;
			}
			sd_assert(ret_val==SUCCESS);
			goto ErrorHanle;
		}


		ret_val = em_pwrite(file_id, (char *)dling_task_order, dling_task_num*sizeof(_u32), POS_ORDER_LIST_ARRAY , &writesize);
		if(need_release)
		{
			EM_SAFE_DELETE(dling_task_order);
			need_release=FALSE;
		}
		sd_assert(ret_val==SUCCESS);
		if((ret_val!=SUCCESS)||(writesize!= dling_task_num*sizeof(_u32)))
		{
			ret_val= INVALID_WRITE_SIZE;
			sd_assert(ret_val==SUCCESS);
			goto ErrorHanle;
		}
	}


      	EM_LOG_DEBUG("dt_clear_task_file_impl:start reading tasks pos_task_file_end=%llu",pos_task_file_end); 
	while(g_file_locked)
	{
		ret_val = em_pread(g_task_file_id, (char*)&task_head, sizeof(DT_TASK_STORE_HEAD),pos_task_file_end,&read_size);
		sd_assert(ret_val==SUCCESS);
		if((ret_val!=SUCCESS)||(read_size!=sizeof(DT_TASK_STORE_HEAD) ))
		{
			if((ret_val==0)&&(read_size==0))
			{
      				EM_LOG_DEBUG("dt_clear_task_file_impl:end of reading tasks pos_task_file_end=%llu",pos_task_file_end); 
				/* Finished! */
				break;
			}
			
			ret_val= INVALID_READ_SIZE;
			sd_assert(ret_val==SUCCESS);
			goto ErrorHanle;
		}
		if(task_head._len>=MAX_P2SP_TASK_FULL_INFO_LEN)
		{
			/* 读到一个长度不正常的任务信息，该文件已经被破坏! */
			ret_val= INVALID_TASK_INFO; 
			goto ErrorHanle;
		}
      		EM_LOG_DEBUG("dt_clear_task_file_impl:task_head._valid=%d",task_head._valid); 
		if(task_head._valid==0)
		{
			pos_task_file_end+=read_size;
			pos_task_file_end+=task_head._len;
			invalid_task_num++;
			continue;
		}
		else
		if(task_head._valid!=1)
		{
			ret_val= INVALID_TASK_INFO; 
			goto ErrorHanle;
		}

		if(!g_file_locked) goto ErrorHanle;
		
		/* DT_TASK_STORE */
		ret_val=em_pread(g_task_file_id, (char *)&task_store,sizeof(DT_TASK_STORE), pos_task_file_end, &read_size);
		sd_assert(ret_val==SUCCESS);
		if((ret_val!=SUCCESS)||(read_size!=sizeof(DT_TASK_STORE)))
		{
			ret_val= INVALID_READ_SIZE;
			sd_assert(ret_val==SUCCESS);
			goto ErrorHanle;
		}

		pos_task_file_end+=read_size;
		
		if(!g_file_locked) goto ErrorHanle;
		
		ret_val = em_pwrite(file_id, (char *)&task_store,read_size, pos_file_end , &writesize);
		sd_assert(ret_val==SUCCESS);
		if((ret_val!=SUCCESS)||(writesize!=read_size))
		{
			ret_val= INVALID_WRITE_SIZE;
			sd_assert(ret_val==SUCCESS);
			goto ErrorHanle;
		}
		pos_file_end+=writesize;
		
      		EM_LOG_DEBUG("dt_clear_task_file_impl:task_id=%d,type=%d,state=%d",task_store._task_info._task_id,task_store._task_info._type,task_store._task_info._state); 
		if(!g_file_locked) goto ErrorHanle;
		
		/* File Path */
		em_memset(buffer,0x00,512);
		ret_val=em_pread(g_task_file_id, (char *)buffer,task_store._task_info._file_path_len, pos_task_file_end, &read_size);
		sd_assert(ret_val==SUCCESS);
		if((ret_val!=SUCCESS)||(read_size!=task_store._task_info._file_path_len))
		{
			ret_val= INVALID_READ_SIZE;
			sd_assert(ret_val==SUCCESS);
			goto ErrorHanle;
		}
		pos_task_file_end+=read_size;
		
		if(!g_file_locked) goto ErrorHanle;

		ret_val = em_pwrite(file_id, (char *)buffer,read_size, pos_file_end , &writesize);
		sd_assert(ret_val==SUCCESS);
		if((ret_val!=SUCCESS)||(writesize!=read_size))
		{
			ret_val= INVALID_WRITE_SIZE;
			sd_assert(ret_val==SUCCESS);
			goto ErrorHanle;
		}
		pos_file_end+=writesize;
		
      		EM_LOG_DEBUG("dt_clear_task_file_impl:file_path=%s",buffer); 
		if(!g_file_locked) goto ErrorHanle;
		
		/* File Name */
		em_memset(buffer,0x00,512);
		ret_val=em_pread(g_task_file_id, (char *)buffer,task_store._task_info._file_name_len, pos_task_file_end, &read_size);
		sd_assert(ret_val==SUCCESS);
		if((ret_val!=SUCCESS)||(read_size!=task_store._task_info._file_name_len))
		{
			ret_val= INVALID_READ_SIZE;
			sd_assert(ret_val==SUCCESS);
			goto ErrorHanle;
		}
		pos_task_file_end+=read_size;

		if(!g_file_locked) goto ErrorHanle;
		
		ret_val = em_pwrite(file_id, (char *)buffer,read_size, pos_file_end , &writesize);
		sd_assert(ret_val==SUCCESS);
		if((ret_val!=SUCCESS)||(writesize!=read_size))
		{
			ret_val= INVALID_WRITE_SIZE;
			sd_assert(ret_val==SUCCESS);
			goto ErrorHanle;
		}
		pos_file_end+=writesize;
		
      		EM_LOG_DEBUG("dt_clear_task_file_impl:file_name=%s",buffer); 
		if(!g_file_locked) goto ErrorHanle;
		
		if(task_store._task_info._type == TT_BT)
		{
			em_memset(buffer,0x00,512);
			ret_val=em_pread(g_task_file_id, (char *)buffer,task_store._task_info._ref_url_len_or_seed_path_len, pos_task_file_end, &read_size);
			sd_assert(ret_val==SUCCESS);
			if((ret_val!=SUCCESS)||(read_size!=task_store._task_info._ref_url_len_or_seed_path_len))
			{
				ret_val= INVALID_READ_SIZE;
				sd_assert(ret_val==SUCCESS);
				goto ErrorHanle;
			}
			pos_task_file_end+=read_size;

			if(!g_file_locked) goto ErrorHanle;
		
			ret_val = em_pwrite(file_id, (char *)buffer,read_size, pos_file_end , &writesize);
			sd_assert(ret_val==SUCCESS);
			if((ret_val!=SUCCESS)||(writesize!=read_size))
			{
				ret_val= INVALID_WRITE_SIZE;
				sd_assert(ret_val==SUCCESS);
				goto ErrorHanle;
			}
			pos_file_end+=writesize;
		
      			EM_LOG_DEBUG("dt_clear_task_file_impl:seed_file=%s",buffer); 
			if(!g_file_locked) goto ErrorHanle;
		
			if(task_store._task_info._user_data_len!=0)
			{
				if(task_store._task_info._user_data_len>512)
				{
					ret_val=em_malloc(task_store._task_info._user_data_len, (void**)&user_data);
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
				em_memset(user_data,0x00,512);
				ret_val=em_pread(g_task_file_id, (char *)user_data,task_store._task_info._user_data_len, pos_task_file_end, &read_size);
				sd_assert(ret_val==SUCCESS);
				if((ret_val!=SUCCESS)||(read_size!=task_store._task_info._user_data_len))
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
				pos_task_file_end+=read_size;

				ret_val = em_pwrite(file_id, (char *)user_data,read_size, pos_file_end , &writesize);
				sd_assert(ret_val==SUCCESS);
				if(need_release)
				{
					EM_SAFE_DELETE(user_data);
					need_release=FALSE;
				}
				if((ret_val!=SUCCESS)||(writesize!=read_size))
				{
					ret_val= INVALID_WRITE_SIZE;
					sd_assert(ret_val==SUCCESS);
					goto ErrorHanle;
				}
				pos_file_end+=writesize;
      				EM_LOG_DEBUG("dt_clear_task_file_impl:user_data=%s",user_data); 
			}

			if(!g_file_locked) goto ErrorHanle;
		
			/* dl_file_index_array */
			if(task_store._task_info._url_len_or_need_dl_num*sizeof(_u16)>512)
			{
				ret_val=em_malloc(task_store._task_info._url_len_or_need_dl_num*sizeof(_u16), (void**)&dl_file_index_array);
				sd_assert(ret_val==SUCCESS);
				if(ret_val!=SUCCESS)
				{
					goto ErrorHanle;
				}
				need_release = TRUE;
			}
			else
			{
				dl_file_index_array = (_u16*)buffer;
			}
			ret_val=em_pread(g_task_file_id, (char *)dl_file_index_array,sizeof(_u16)*task_store._task_info._url_len_or_need_dl_num, pos_task_file_end, &read_size);
			sd_assert(ret_val==SUCCESS);
			if((ret_val!=SUCCESS)||(read_size!=sizeof(_u16)*task_store._task_info._url_len_or_need_dl_num))
			{
				if(need_release)
				{
					EM_SAFE_DELETE(dl_file_index_array);
					need_release=FALSE;
				}
				ret_val= INVALID_READ_SIZE;
				sd_assert(ret_val==SUCCESS);
				goto ErrorHanle;
			}
			pos_task_file_end+=read_size;

			ret_val = em_pwrite(file_id, (char *)dl_file_index_array,read_size, pos_file_end , &writesize);
			sd_assert(ret_val==SUCCESS);
			if(need_release)
			{
				EM_SAFE_DELETE(dl_file_index_array);
				need_release=FALSE;
			}
			if((ret_val!=SUCCESS)||(writesize!=read_size))
			{
				ret_val= INVALID_WRITE_SIZE;
				sd_assert(ret_val==SUCCESS);
				goto ErrorHanle;
			}
			pos_file_end+=writesize;
			
      			EM_LOG_DEBUG("dt_clear_task_file_impl:need_dl_num=%u",task_store._task_info._url_len_or_need_dl_num); 
			if(!g_file_locked) goto ErrorHanle;
		
			/* dl_file_index_array */
			if(task_store._task_info._url_len_or_need_dl_num*sizeof(BT_FILE)>512)
			{
				ret_val=em_malloc(task_store._task_info._url_len_or_need_dl_num*sizeof(BT_FILE), (void**)&file_array);
				sd_assert(ret_val==SUCCESS);
				if(ret_val!=SUCCESS)
				{
					goto ErrorHanle;
				}
				need_release = TRUE;
			}
			else
			{
				file_array =(BT_FILE*)buffer;
			}
			ret_val=em_pread(g_task_file_id, (char *)file_array,sizeof(BT_FILE)*task_store._task_info._url_len_or_need_dl_num, pos_task_file_end, &read_size);
			sd_assert(ret_val==SUCCESS);
			if((ret_val!=SUCCESS)||(read_size!=sizeof(BT_FILE)*task_store._task_info._url_len_or_need_dl_num))
			{
				if(need_release)
				{
					EM_SAFE_DELETE(file_array);
					need_release=FALSE;
				}
				ret_val= INVALID_READ_SIZE;
				sd_assert(ret_val==SUCCESS);
				goto ErrorHanle;
			}
			
			pos_task_file_end+=read_size;

			ret_val = em_pwrite(file_id, (char *)file_array,read_size, pos_file_end , &writesize);
			sd_assert(ret_val==SUCCESS);
			if(need_release)
			{
				EM_SAFE_DELETE(file_array);
				need_release=FALSE;
			}
			if((ret_val!=SUCCESS)||(writesize!=read_size))
			{
				ret_val= INVALID_WRITE_SIZE;
				sd_assert(ret_val==SUCCESS);
				goto ErrorHanle;
			}
			pos_file_end+=writesize;
		}
		else
		{
			if(task_store._task_info._url_len_or_need_dl_num!=0)
			{
				em_memset(buffer,0x00,512);
				ret_val=em_pread(g_task_file_id, (char *)buffer,task_store._task_info._url_len_or_need_dl_num, pos_task_file_end, &read_size);
				sd_assert(ret_val==SUCCESS);
				if((ret_val!=SUCCESS)||(read_size!=task_store._task_info._url_len_or_need_dl_num))
				{
					ret_val= INVALID_READ_SIZE;
					sd_assert(ret_val==SUCCESS);
					goto ErrorHanle;
				}
				pos_task_file_end+=read_size;
				
				ret_val = em_pwrite(file_id, (char *)buffer,read_size, pos_file_end , &writesize);
				sd_assert(ret_val==SUCCESS);
				if((ret_val!=SUCCESS)||(writesize!=read_size))
				{
					ret_val= INVALID_WRITE_SIZE;
					sd_assert(ret_val==SUCCESS);
					goto ErrorHanle;
				}
				pos_file_end+=writesize;
      				EM_LOG_DEBUG("dt_clear_task_file_impl:url=%s",buffer); 
			}
			
			if(!g_file_locked) goto ErrorHanle;
		
			if(task_store._task_info._ref_url_len_or_seed_path_len!=0)
			{
				em_memset(buffer,0x00,512);
				ret_val=em_pread(g_task_file_id, (char *)buffer,task_store._task_info._ref_url_len_or_seed_path_len, pos_task_file_end, &read_size);
				sd_assert(ret_val==SUCCESS);
				if((ret_val!=SUCCESS)||(read_size!=task_store._task_info._ref_url_len_or_seed_path_len))
				{
					ret_val= INVALID_READ_SIZE;
					sd_assert(ret_val==SUCCESS);
					goto ErrorHanle;
				}
				pos_task_file_end+=read_size;
				
				ret_val = em_pwrite(file_id, (char *)buffer,read_size, pos_file_end , &writesize);
				sd_assert(ret_val==SUCCESS);
				if((ret_val!=SUCCESS)||(writesize!=read_size))
				{
					ret_val= INVALID_WRITE_SIZE;
					sd_assert(ret_val==SUCCESS);
					goto ErrorHanle;
				}
				pos_file_end+=writesize;
      				EM_LOG_DEBUG("dt_clear_task_file_impl:ref_url=%s",buffer); 
			}
			
			if(!g_file_locked) goto ErrorHanle;
		
			if(task_store._task_info._user_data_len!=0)
			{
				if(task_store._task_info._user_data_len>512)
				{
					ret_val=em_malloc(task_store._task_info._user_data_len, (void**)&user_data);
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
				em_memset(user_data,0x00,512);
				ret_val=em_pread(g_task_file_id, (char *)user_data,task_store._task_info._user_data_len, pos_task_file_end, &read_size);
				sd_assert(ret_val==SUCCESS);
				if((ret_val!=SUCCESS)||(read_size!=task_store._task_info._user_data_len))
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
				pos_task_file_end+=read_size;
				
				ret_val = em_pwrite(file_id, (char *)user_data,read_size, pos_file_end , &writesize);
				sd_assert(ret_val==SUCCESS);
				if(need_release)
				{
					EM_SAFE_DELETE(user_data);
					need_release=FALSE;
				}
				if((ret_val!=SUCCESS)||(writesize!=read_size))
				{
					ret_val= INVALID_WRITE_SIZE;
					sd_assert(ret_val==SUCCESS);
					goto ErrorHanle;
				}
				pos_file_end+=writesize;
      				EM_LOG_DEBUG("dt_clear_task_file_impl:user_data=%s",user_data); 
			}
			
			if(!g_file_locked) goto ErrorHanle;
		
      			EM_LOG_DEBUG("dt_clear_task_file_impl:have_tcid=%d",task_store._task_info._have_tcid); 
			if(task_store._task_info._have_tcid!=0)
			{
				ret_val=em_pread(g_task_file_id, (char *)buffer,CID_SIZE, pos_task_file_end, &read_size);
				sd_assert(ret_val==SUCCESS);
				if((ret_val!=SUCCESS)||(read_size!=CID_SIZE))
				{
					ret_val= INVALID_READ_SIZE;
					sd_assert(ret_val==SUCCESS);
					goto ErrorHanle;
				}
				pos_task_file_end+=read_size;
				
				ret_val = em_pwrite(file_id, (char *)buffer,read_size, pos_file_end , &writesize);
				sd_assert(ret_val==SUCCESS);
				if((ret_val!=SUCCESS)||(writesize!=read_size))
				{
					ret_val= INVALID_WRITE_SIZE;
					sd_assert(ret_val==SUCCESS);
					goto ErrorHanle;
				}
				pos_file_end+=writesize;
			}
		}
      		EM_LOG_DEBUG("dt_clear_task_file_impl:%u/%u task is ok!",++valid_task_num,invalid_task_num); 
	}

      	EM_LOG_DEBUG("dt_clear_task_file_impl:all tasks(%u/%u) is done!",valid_task_num+invalid_task_num,total_task_num); 
	em_close_ex(g_task_file_id);
	g_task_file_id=0;
	
	em_close_ex(file_id);
	file_id = 0;
	
	ret_val= em_delete_file(file_path);
	sd_assert(ret_val==SUCCESS);
	if(ret_val!=SUCCESS) goto ErrorHanle;
	
	ret_val= em_rename_file(new_file_path,file_path);
	sd_assert(ret_val==SUCCESS);
	if(ret_val!=SUCCESS) goto ErrorHanle;

	g_pos_task_file_end=POS_TASK_START;
	
	CHECK_FILE_OPEN_INT;
	
	ret_val = em_filesize(g_task_file_id, &filesize);
	sd_assert(ret_val==SUCCESS);
	if(ret_val!=SUCCESS) goto ErrorHanle;
	
	sd_assert(pos_file_end>=filesize-1);
		
	g_need_clear=FALSE;
	g_file_locked=FALSE;
	
      	EM_LOG_DEBUG("dt_clear_task_file_impl:SUCCESS:filesize=%llu,%llu",filesize,pos_file_end); 
	
	return SUCCESS;
ErrorHanle:
	if(file_id!=0)
	{
		em_close_ex(file_id);
	}
	g_file_locked=FALSE;
      	EM_LOG_DEBUG("dt_clear_task_file_impl:end:ret_val=%d",ret_val); 
	sd_assert(FALSE);
	return ret_val;
}
_int32 dt_stop_clear_task_file(void)
{
      	EM_LOG_DEBUG("dt_stop_clear_task_file:g_file_locked=%d,g_file_thread_id=%u",g_file_locked,g_file_thread_id); 
	if(g_file_locked)
	{
		g_file_locked=FALSE;
		em_sleep(5);
	}

	if(g_file_thread_id!=0)
	{
		em_finish_task(g_file_thread_id);
		g_file_thread_id=0;
	}
	return SUCCESS;
}

_int32 dt_backup_file(void)
{
	_int32 ret_val=SUCCESS;
	char * file_path = dt_get_task_store_file_path();
	static char new_file_path[MAX_FULL_PATH_BUFFER_LEN];
	static _u32 check_count = 0;


      	EM_LOG_DEBUG("dt_backup_file:g_file_locked=%d,g_file_thread_id=%u,check_count=%u",g_file_locked,g_file_thread_id,check_count); 
		
	if(g_file_locked) return SUCCESS;
	if(g_file_thread_id!=0) return SUCCESS;

	if(check_count++>=MAX_FILE_BACKUP_COUNT)
  	{
  		check_count = 0;
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

	dt_close_task_file(TRUE);
	g_file_locked=TRUE;
	ret_val= em_copy_file(file_path,new_file_path);
	g_file_locked=FALSE;
	CHECK_VALUE(ret_val);
	
	return ret_val;
}
BOOL dt_recover_file(void)
{
	_int32 ret_val=SUCCESS;
	char * file_path = dt_get_task_store_file_path();
	char new_file_path[MAX_FULL_PATH_BUFFER_LEN];

      	EM_LOG_DEBUG("dt_recover_file:g_file_locked=%d,g_file_thread_id=%u",g_file_locked,g_file_thread_id); 
		
	if(g_file_locked) return FALSE;
	if(g_file_thread_id!=0) return FALSE;

	em_memset(new_file_path,0, MAX_FULL_PATH_BUFFER_LEN);
	em_strncpy(new_file_path, file_path, MAX_FULL_PATH_BUFFER_LEN-4);
	em_strcat(new_file_path, ".bak", 4);
	
	dt_close_task_file(TRUE);
	
	if(em_file_exist(file_path))
	{
		em_delete_file(file_path);
	}
	
	if(!em_file_exist(new_file_path))
	{
		return FALSE;
	}
	g_pos_task_file_end = POS_TASK_START;
	
	g_file_locked=TRUE;
	ret_val= em_rename_file(new_file_path,file_path);
	sd_assert(ret_val==SUCCESS);
	g_file_locked=FALSE;
	
	if(ret_val!=SUCCESS)
		return FALSE;
	else
		return TRUE;
}


_int32 dt_get_total_task_num_from_file(_u32 * total_task_num)
{
	_int32 ret_val=SUCCESS;
	_u32 readsize=0;
	
	CHECK_FILE_OPEN_INT;
	
      	EM_LOG_DEBUG("dt_get_total_task_num_from_file"); 
	/* read from file */
	ret_val=em_pread(g_task_file_id, (char *)total_task_num,sizeof(_u32), POS_TOTAL_TASK_NUM, &readsize);
	CHECK_VALUE(ret_val);
	if(readsize!=sizeof(_u32))
	{
		*total_task_num=0;
		//CHECK_VALUE( INVALID_READ_SIZE);
	}
#ifdef DT_STROE_DEBUG
	if(g_task_test_file_id==0) 
	{
		if(SUCCESS==em_open_ex("./etm_task_test.dat", O_FS_CREATE, &g_task_test_file_id))
		{
			ret_val=em_fprintf(g_task_test_file_id, "\nopen etm_task_test.dat:g_task_test_file_id=%u [%u,%u] \nget total_task_num:%u\n ", g_task_test_file_id, POS_TOTAL_TASK_NUM,g_pos_task_file_end,*total_task_num);
			sd_assert(ret_val==SUCCESS);
		}
		else
		{
			sd_assert(FALSE);
		}
	}
#endif
      	EM_LOG_DEBUG("dt_get_total_task_num_from_file:%u",*total_task_num); 
	return SUCCESS;
}
_int32 dt_save_total_task_num_to_file(_u32  total_task_num)
{
	_int32 ret_val=SUCCESS;
	_u32 writesize=0;
#ifdef STRONG_FILE_CHECK
	_u32 old_total_task_num=0,readsize=0;
#endif /* STRONG_FILE_CHECK */

	dt_stop_clear_task_file();

	CHECK_FILE_OPEN_INT
		
      	EM_LOG_DEBUG("dt_save_total_task_num_to_file"); 
#ifdef STRONG_FILE_CHECK
	/* read from file */
	ret_val=em_pread(g_task_file_id, (char *)&old_total_task_num,sizeof(_u32), POS_TOTAL_TASK_NUM, &readsize);
	CHECK_VALUE(ret_val);
	if(readsize!=sizeof(_u32))
	{
		CHECK_VALUE( INVALID_READ_SIZE);
	}
#endif /* STRONG_FILE_CHECK */
		ret_val = em_pwrite(g_task_file_id, (char *)&total_task_num, sizeof(_u32), POS_TOTAL_TASK_NUM , &writesize);
		CHECK_VALUE(ret_val);
		if(writesize!=sizeof(_u32))
		{
			CHECK_VALUE( INVALID_WRITE_SIZE);
		}
#ifdef DT_STROE_DEBUG
	if(g_task_test_file_id!=0) 
	{
		ret_val=em_fprintf(g_task_test_file_id, "\n save total_task_num[%u,%u]:%u\n ", POS_TOTAL_TASK_NUM,g_pos_task_file_end, total_task_num);
		sd_assert(ret_val==SUCCESS);
	}
	else
	{
		sd_assert(FALSE);
	}
#endif
	return SUCCESS;
}

_u32 * dt_get_running_tasks_from_file(void)
{
	static _u32 running_tasks_array[MAX_ALOW_TASKS];
	_int32 ret_val=SUCCESS;
	_u32 readsize=0;
	
	CHECK_FILE_OPEN;

      	EM_LOG_DEBUG("dt_get_running_tasks_from_file"); 
	em_memset(&running_tasks_array, 0, MAX_ALOW_TASKS*sizeof(_u32));
	/* read from file */
	ret_val=em_pread(g_task_file_id, (char *)&running_tasks_array,MAX_ALOW_TASKS*sizeof(_u32), POS_RUNNING_TASK_ARRAY, &readsize);
	sd_assert(ret_val==SUCCESS);
	if((ret_val!=SUCCESS)||(readsize!=MAX_ALOW_TASKS*sizeof(_u32)))
	{
		return NULL;
	}
	
#ifdef DT_STROE_DEBUG
	if(g_task_test_file_id!=0) 
	{
		ret_val=em_fprintf(g_task_test_file_id, "\n get running_tasks[%u,%u]:%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n ", POS_RUNNING_TASK_ARRAY,g_pos_task_file_end, 
			running_tasks_array[0],running_tasks_array[1],running_tasks_array[2],running_tasks_array[3],running_tasks_array[4],running_tasks_array[5],running_tasks_array[6],running_tasks_array[7],
			running_tasks_array[8],	running_tasks_array[9],running_tasks_array[10],running_tasks_array[11],running_tasks_array[12],running_tasks_array[13],running_tasks_array[14],running_tasks_array[15]);
		sd_assert(ret_val==SUCCESS);
	}
	else
	{
		sd_assert(FALSE);
	}
#endif
	return running_tasks_array;
}
_int32 dt_save_running_tasks_to_file(_u32 *running_tasks_array)
{
	_int32 ret_val=SUCCESS;
	_u32 writesize=0;

	dt_stop_clear_task_file();

	CHECK_FILE_OPEN_INT
		
      	EM_LOG_DEBUG("dt_save_running_tasks_to_file"); 

#ifdef STRONG_FILE_CHECK
	if(dt_get_running_tasks_from_file()==NULL)
	{
		CHECK_VALUE( INVALID_READ_SIZE);
	}
#endif /* STRONG_FILE_CHECK */
	
		ret_val = em_pwrite(g_task_file_id, (char *)running_tasks_array, MAX_ALOW_TASKS*sizeof(_u32), POS_RUNNING_TASK_ARRAY , &writesize);
		CHECK_VALUE(ret_val);
		if(writesize!=MAX_ALOW_TASKS*sizeof(_u32))
		{
			CHECK_VALUE( INVALID_WRITE_SIZE);
		}
		
#ifdef DT_STROE_DEBUG
	if(g_task_test_file_id!=0) 
	{
		ret_val=em_fprintf(g_task_test_file_id, "\n dt_save_running_tasks[%u,%u]:%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n ", POS_RUNNING_TASK_ARRAY,g_pos_task_file_end,
			running_tasks_array[0],running_tasks_array[1],running_tasks_array[2],running_tasks_array[3],running_tasks_array[4],running_tasks_array[5],running_tasks_array[6],running_tasks_array[7],
			running_tasks_array[8],	running_tasks_array[9],running_tasks_array[10],running_tasks_array[11],running_tasks_array[12],running_tasks_array[13],running_tasks_array[14],running_tasks_array[15]);
		sd_assert(ret_val==SUCCESS);
	}
	else
	{
		sd_assert(FALSE);
	}
#endif
	running_task_loaded = FALSE;
	return SUCCESS;
}

_u32 dt_get_order_list_size_from_file(void)
{
	_int32 ret_val=SUCCESS;
	_u32 order_list_size=0,readsize=0;
	
	CHECK_FILE_OPEN_INT;
	
      	EM_LOG_DEBUG("dt_get_order_list_size_from_file"); 
	/* read from file */
	ret_val=em_pread(g_task_file_id, (char *)&order_list_size,sizeof(_u32), POS_ORDER_LIST_SIZE, &readsize);
	sd_assert(ret_val==SUCCESS);
	if((ret_val!=SUCCESS)||(readsize!=sizeof(_u32)))
	{
		return 0;
	}
	
#ifdef DT_STROE_DEBUG
	if(g_task_test_file_id!=0) 
	{
		ret_val=em_fprintf(g_task_test_file_id, "\n get_order_list_size[%u,%u]: %u\n",POS_ORDER_LIST_SIZE,g_pos_task_file_end,order_list_size);
		sd_assert(ret_val==SUCCESS);
	}
	else
	{
		sd_assert(FALSE);
	}
#endif
	return order_list_size;
}
_int32 dt_get_order_list_from_file(_u32 * task_id_array_buffer,_u32 buffer_len)
{
	_int32 ret_val=SUCCESS;
	_u32 readsize=0;
	
	CHECK_FILE_OPEN_INT;
	
      	EM_LOG_DEBUG("dt_get_order_list_from_file"); 
	/* read from file */
	ret_val=em_pread(g_task_file_id, (char *)task_id_array_buffer,buffer_len, POS_ORDER_LIST_ARRAY, &readsize);
	CHECK_VALUE( ret_val);
	if(readsize!=buffer_len)
	{
		CHECK_VALUE( INVALID_READ_SIZE);
	}
	
#ifdef DT_STROE_DEBUG
	if(g_task_test_file_id!=0) 
	{
		ret_val=em_fprintf(g_task_test_file_id, "\n get_order_list[%u,%u]: ",POS_ORDER_LIST_ARRAY,g_pos_task_file_end);
		sd_assert(ret_val==SUCCESS);
		for(readsize=0;readsize<(buffer_len/sizeof(_u32));readsize++)
		{
			if((readsize%16)==0)
			{
				ret_val=em_fprintf(g_task_test_file_id, "\n%u,",task_id_array_buffer[readsize]);
			}
			else
			{
				ret_val=em_fprintf(g_task_test_file_id, "%u,",task_id_array_buffer[readsize]);
			}
			sd_assert(ret_val==SUCCESS);
		}
		ret_val=em_fprintf(g_task_test_file_id, "\n get_order_list end!\n");
		sd_assert(ret_val==SUCCESS);
	}
	else
	{
		sd_assert(FALSE);
	}
#endif
	return SUCCESS;
}
_int32 dt_save_order_list_to_file(_u32 order_list_size,_u32 *task_id_array)
{
	_int32 ret_val=SUCCESS;
	_u32 writesize=0;
#ifdef STRONG_FILE_CHECK
	_u32 old_list_size = 0;
#endif /* STRONG_FILE_CHECK */	

	dt_stop_clear_task_file();

	CHECK_FILE_OPEN_INT
		
      	EM_LOG_DEBUG("dt_save_order_list_to_file"); 
#ifdef STRONG_FILE_CHECK
	old_list_size = dt_get_order_list_size_from_file();
#endif /* STRONG_FILE_CHECK */	
		ret_val = em_pwrite(g_task_file_id, (char *)&order_list_size, sizeof(_u32), POS_ORDER_LIST_SIZE , &writesize);
		CHECK_VALUE(ret_val);
		if(writesize!=sizeof(_u32))
		{
			CHECK_VALUE( INVALID_WRITE_SIZE);
		}

		if(task_id_array!=NULL)
		{
			writesize=0;
			ret_val = em_pwrite(g_task_file_id, (char *)task_id_array, order_list_size*sizeof(_u32), POS_ORDER_LIST_ARRAY , &writesize);
			CHECK_VALUE(ret_val);
			if(writesize!=order_list_size*sizeof(_u32))
			{
				CHECK_VALUE( INVALID_WRITE_SIZE);
			}
		}
#ifdef DT_STROE_DEBUG
	if(g_task_test_file_id!=0) 
	{
		ret_val=em_fprintf(g_task_test_file_id, "\n save_order_list,size:%u[%u,%u],list[%u]:",order_list_size,POS_ORDER_LIST_SIZE,g_pos_task_file_end,POS_ORDER_LIST_ARRAY);
		sd_assert(ret_val==SUCCESS);
		for(writesize=0;writesize<order_list_size;writesize++)
		{
			if((writesize%16)==0)
			{
				ret_val=em_fprintf(g_task_test_file_id, "\n%u,",task_id_array[writesize]);
			}
			else
			{
				ret_val=em_fprintf(g_task_test_file_id, "%u,",task_id_array[writesize]);
			}
			sd_assert(ret_val==SUCCESS);
		}
		ret_val=em_fprintf(g_task_test_file_id, "\n save_order_list end!\n");
		sd_assert(ret_val==SUCCESS);
	}
	else
	{
		sd_assert(FALSE);
	}
#endif
	return SUCCESS;
}
////////////////////////////////////////////////////////////////////
_int32 dt_get_task_crc_value(EM_TASK * p_task,_u16 * p_crc_value,_u32 * p_data_len)
{
	//rclist_head.crc = sd_add_crc16(rclist_head.crc, &count, sizeof(count));		
	//rclist_head.len+= sizeof(count);
	//rclist_head.crc = sd_inv_crc16(rclist_head.crc);

//// check
		//			crc = sd_add_crc16(crc, pdata, rclist_head.len);
		//			if(FALSE != sd_isvalid_crc16(rclist_head.crc, crc))
////////////////////////
	_u32 data_len=0;
	_u16 crc_value=0xffff;
	EM_BT_TASK * p_bt_task=NULL;
	EM_P2SP_TASK * p_p2sp_task=NULL;
      EM_LOG_DEBUG("dt_get_task_crc_value:task_id=%u",p_task->_task_info->_task_id); 
	
	crc_value = em_add_crc16(crc_value, p_task->_task_info, sizeof(TASK_INFO));
	data_len+=sizeof(TASK_INFO);
	
	if(p_task->_task_info->_type == TT_BT)
	{
		p_bt_task = (EM_BT_TASK * )p_task->_task_info;
		crc_value = em_add_crc16(crc_value, p_bt_task->_file_path, p_task->_task_info->_file_path_len);
		data_len+=p_task->_task_info->_file_path_len;

		crc_value = em_add_crc16(crc_value, p_bt_task->_file_name, p_task->_task_info->_file_name_len);
		data_len+=p_task->_task_info->_file_name_len;
		
		crc_value = em_add_crc16(crc_value, p_bt_task->_seed_file_path,  p_task->_task_info->_ref_url_len_or_seed_path_len);
		data_len+=p_task->_task_info->_ref_url_len_or_seed_path_len;
		
		crc_value = em_add_crc16(crc_value, p_bt_task->_user_data, p_task->_task_info->_user_data_len);
		data_len+=p_task->_task_info->_user_data_len;

		crc_value = em_add_crc16(crc_value, p_bt_task->_dl_file_index_array, p_task->_task_info->_url_len_or_need_dl_num*sizeof(_u16));
		data_len+=p_task->_task_info->_url_len_or_need_dl_num*sizeof(_u16);
		
		crc_value = em_add_crc16(crc_value, p_bt_task->_file_array, p_task->_task_info->_url_len_or_need_dl_num*sizeof(BT_FILE));
		data_len+=p_task->_task_info->_url_len_or_need_dl_num*sizeof(BT_FILE);
	}
	else
	{
		p_p2sp_task = (EM_P2SP_TASK * )p_task->_task_info;
		
		if(p_task->_task_info->_file_path_len > 255)
		{
			CHECK_VALUE(INVALID_FILE_PATH);
		}
		crc_value = em_add_crc16(crc_value, p_p2sp_task->_file_path, p_task->_task_info->_file_path_len);
		data_len+=p_task->_task_info->_file_path_len;

		if(p_task->_task_info->_file_name_len>255)
		{
			CHECK_VALUE(INVALID_FILE_NAME);
		}
		crc_value = em_add_crc16(crc_value, p_p2sp_task->_file_name, p_task->_task_info->_file_name_len);
		data_len+=p_task->_task_info->_file_name_len;
		
		if(p_task->_task_info->_url_len_or_need_dl_num>=MAX_URL_LEN)
		{
			CHECK_VALUE(INVALID_URL);
		}
		crc_value = em_add_crc16(crc_value, p_p2sp_task->_url, p_task->_task_info->_url_len_or_need_dl_num);
		data_len+=p_task->_task_info->_url_len_or_need_dl_num;
		
		if(p_task->_task_info->_ref_url_len_or_seed_path_len>=MAX_URL_LEN)
		{
			CHECK_VALUE(INVALID_URL);
		}
		if(p_task->_task_info->_ref_url_len_or_seed_path_len!=0) 
		{
			sd_assert(p_task->_task_info->_have_ref_url);
		}
		crc_value = em_add_crc16(crc_value, p_p2sp_task->_ref_url, p_task->_task_info->_ref_url_len_or_seed_path_len);
		data_len+=p_task->_task_info->_ref_url_len_or_seed_path_len;
		
		if(p_task->_task_info->_user_data_len>=MAX_USER_DATA_LEN)
		{
			CHECK_VALUE(INVALID_USER_DATA);
		}
		crc_value = em_add_crc16(crc_value, p_p2sp_task->_user_data, p_task->_task_info->_user_data_len);
		data_len+=p_task->_task_info->_user_data_len;

		if(p_task->_task_info->_have_tcid)
		{
			crc_value = em_add_crc16(crc_value, p_p2sp_task->_tcid, CID_SIZE);
			data_len+=CID_SIZE;
		}

		if(data_len>=MAX_P2SP_TASK_FULL_INFO_LEN)
		{
			CHECK_VALUE(INVALID_TASK_INFO);
		}
	}

	crc_value = em_inv_crc16(crc_value);
	*p_crc_value=crc_value;
	*p_data_len=data_len;
	
	return SUCCESS;
}

_int32 dt_add_task_to_file(EM_TASK * p_task)
{
	_int32 ret_val=SUCCESS;
	_u32 writesize=0;
#if 0 //def STRONG_FILE_CHECK
	_u64 filesize = 0;
#endif /* STRONG_FILE_CHECK */		
	//_u32 need_dl_num=0,seed_path_len=0,url_len=0,ref_url_len=0;
	DT_TASK_STORE dt_task_store;
	//BT_TASK * p_bt_task=NULL;
	//P2SP_TASK * p_p2sp_task=NULL;

	dt_stop_clear_task_file();

	CHECK_FILE_OPEN_INT;

      EM_LOG_DEBUG("dt_add_task_to_file:task_id=%u",p_task->_task_info->_task_id); 

	sd_assert((p_task->_task_info!=NULL)&&(p_task->_task_info->_full_info!=FALSE));

	em_memset(&dt_task_store, 0, sizeof(DT_TASK_STORE));
	dt_task_store._valid = 1;
	
	ret_val=dt_get_task_crc_value(p_task,&dt_task_store._crc,&dt_task_store._len);
	CHECK_VALUE(ret_val);
	
	em_memcpy(&dt_task_store._task_info, p_task->_task_info, sizeof(TASK_INFO));
	
	if(p_task->_offset==0)
	{
		/* write to the end of the task file */
		sd_assert(g_pos_task_file_end>=POS_TASK_START);
#if 0 //def STRONG_FILE_CHECK
		ret_val = em_filesize(g_task_file_id, &filesize);
		CHECK_VALUE(ret_val);
		
		sd_assert(g_pos_task_file_end>=filesize-1);
#endif /* STRONG_FILE_CHECK */	
		if(g_pos_task_file_end==POS_TASK_START)
		{
			if(dt_get_task_num_in_map()!=0)
			{
				/* g_pos_task_file_end 出错! */
				CHECK_VALUE(WRITE_TASK_FILE_ERR);
			}
		}
		p_task->_offset = g_pos_task_file_end;

		writesize=0;
		ret_val = em_pwrite(g_task_file_id, (char *)&dt_task_store, sizeof(DT_TASK_STORE), p_task->_offset , &writesize);
		CHECK_VALUE(ret_val);
		g_pos_task_file_end+= writesize;
		if(writesize!=sizeof(DT_TASK_STORE))
		{
			CHECK_VALUE( INVALID_WRITE_SIZE);
		}
		
#ifdef DT_STROE_DEBUG
		if(g_task_test_file_id!=0) 
		{
			ret_val=em_fprintf(g_task_test_file_id, "\n add_task[%u,%u]:valid=%u,crc=%u,len=%u,task_id=%u,type=%u,state=%u,\ndeleted=%d,have_name=%d,check_data=%d,have_tcid=%d,have_ref_url=%d,have_user_data=%d,full_info=%d,_correct_file_name=%d,\nfile_path_len=%u,file_name_len=%u,url_len_or_need_dl_num=%u,ref_url_len_or_seed_path_len=%u,user_data_len=%u,\neigenvalue=%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,\nfile_size=%llu,dl_size=%llu,start_time=%u,finished_time=%u,failed_code=%u,bt_total_file_num=%u\n",
				p_task->_offset,g_pos_task_file_end,
				dt_task_store._valid,dt_task_store._crc,dt_task_store._len,dt_task_store._task_info._task_id,dt_task_store._task_info._type,dt_task_store._task_info._state,dt_task_store._task_info._is_deleted,
				dt_task_store._task_info._have_name,dt_task_store._task_info._check_data,dt_task_store._task_info._have_tcid,dt_task_store._task_info._have_ref_url,dt_task_store._task_info._have_user_data,dt_task_store._task_info._full_info,
				dt_task_store._task_info._correct_file_name,dt_task_store._task_info._file_path_len,dt_task_store._task_info._file_name_len,dt_task_store._task_info._url_len_or_need_dl_num,dt_task_store._task_info._ref_url_len_or_seed_path_len,dt_task_store._task_info._user_data_len,
				dt_task_store._task_info._eigenvalue[0],dt_task_store._task_info._eigenvalue[1],dt_task_store._task_info._eigenvalue[2],dt_task_store._task_info._eigenvalue[3],dt_task_store._task_info._eigenvalue[4],dt_task_store._task_info._eigenvalue[5],
				dt_task_store._task_info._eigenvalue[6],dt_task_store._task_info._eigenvalue[7],dt_task_store._task_info._eigenvalue[8],dt_task_store._task_info._eigenvalue[9],dt_task_store._task_info._eigenvalue[10],dt_task_store._task_info._eigenvalue[11],
				dt_task_store._task_info._eigenvalue[12],dt_task_store._task_info._eigenvalue[13],dt_task_store._task_info._eigenvalue[14],dt_task_store._task_info._eigenvalue[15],dt_task_store._task_info._eigenvalue[16],dt_task_store._task_info._eigenvalue[17],
				dt_task_store._task_info._eigenvalue[18],dt_task_store._task_info._eigenvalue[19],dt_task_store._task_info._file_size,dt_task_store._task_info._downloaded_data_size,dt_task_store._task_info._start_time,dt_task_store._task_info._finished_time
				,dt_task_store._task_info._failed_code,dt_task_store._task_info._bt_total_file_num);
			sd_assert(ret_val==SUCCESS);
		}
		else
		{
			sd_assert(FALSE);
		}
#endif

		if(p_task->_task_info->_type== TT_BT)
		{
			ret_val = dt_add_bt_task_part_to_file(p_task);
			CHECK_VALUE(ret_val);
		}
		else
		{
	 		ret_val = dt_add_p2sp_task_part_to_file( p_task);
			CHECK_VALUE(ret_val);
 		}
	
	}
	else
	{
		sd_assert(FALSE);
		//if(change_len)
		{
			/* disable the current section and write to the end of the task file */
		}
		//else
		{
			/* modify the current section */
		}
	}
      EM_LOG_DEBUG("dt_add_task_to_file success!:task_id=%u",p_task->_task_info->_task_id); 
	p_task->_change_flag = 0;
	return SUCCESS;
}
_int32 dt_add_bt_task_part_to_file(EM_TASK * p_task)
{
	_int32 ret_val=SUCCESS;
	_u32 writesize=0;
	EM_BT_TASK * p_bt_task=NULL;
	
      EM_LOG_DEBUG("dt_add_bt_task_part_to_file:task_id=%u",p_task->_task_info->_task_id); 
			p_bt_task = (EM_BT_TASK * )p_task->_task_info;
			
			writesize=0;
			ret_val = em_pwrite(g_task_file_id, (char *)p_bt_task->_file_path, p_task->_task_info->_file_path_len,POS_PATH , &writesize);
			CHECK_VALUE(ret_val);
			g_pos_task_file_end+= writesize;
			if(writesize!=p_task->_task_info->_file_path_len)
			{
				CHECK_VALUE( INVALID_WRITE_SIZE);
			}
			
			writesize=0;
			ret_val = em_pwrite(g_task_file_id, (char *)p_bt_task->_file_name, p_task->_task_info->_file_name_len, POS_NAME , &writesize);
			CHECK_VALUE(ret_val);
			g_pos_task_file_end+= writesize;
			if(writesize!=p_task->_task_info->_file_name_len)
			{
				CHECK_VALUE( INVALID_WRITE_SIZE);
			}
			
			writesize=0;
			ret_val = em_pwrite(g_task_file_id, (char *)p_bt_task->_seed_file_path, p_task->_task_info->_ref_url_len_or_seed_path_len, POS_SEED_PATH , &writesize);
			CHECK_VALUE(ret_val);
			g_pos_task_file_end+= writesize;
			if(writesize!=p_task->_task_info->_ref_url_len_or_seed_path_len)
			{
				CHECK_VALUE( INVALID_WRITE_SIZE);
			}
			
			writesize=0;
			ret_val = em_pwrite(g_task_file_id, (char *)p_bt_task->_user_data, p_task->_task_info->_user_data_len, POS_BT_USER_DATA , &writesize);
			CHECK_VALUE(ret_val);
			g_pos_task_file_end+= writesize;
			if(writesize!=p_task->_task_info->_user_data_len)
			{
				CHECK_VALUE( INVALID_WRITE_SIZE);
			}
			
			writesize=0;
			ret_val = em_pwrite(g_task_file_id, (char *)p_bt_task->_dl_file_index_array, p_task->_task_info->_url_len_or_need_dl_num*sizeof(_u16), POS_INDEX_ARRAY , &writesize);
			CHECK_VALUE(ret_val);
			g_pos_task_file_end+= writesize;
			if(writesize!=p_task->_task_info->_url_len_or_need_dl_num*sizeof(_u16))
			{
				CHECK_VALUE( INVALID_WRITE_SIZE);
			}
			
			writesize=0;
			ret_val = em_pwrite(g_task_file_id, (char *)p_bt_task->_file_array, p_task->_task_info->_url_len_or_need_dl_num*sizeof(BT_FILE), POS_FILE_ARRAY , &writesize);
			CHECK_VALUE(ret_val);
			g_pos_task_file_end+= writesize;
			if(writesize!=p_task->_task_info->_url_len_or_need_dl_num*sizeof(BT_FILE))
			{
				CHECK_VALUE( INVALID_WRITE_SIZE);
			}
			
	return SUCCESS;
}
_int32 dt_add_p2sp_task_part_to_file(EM_TASK * p_task)
{
	_int32 ret_val=SUCCESS;
	_u32 writesize=0;
	EM_P2SP_TASK * p_p2sp_task=NULL;

      EM_LOG_DEBUG("dt_add_p2sp_task_part_to_file:task_id=%u",p_task->_task_info->_task_id); 
			p_p2sp_task = (EM_P2SP_TASK * )p_task->_task_info;
			
			writesize=0;
			ret_val = em_pwrite(g_task_file_id, (char *)p_p2sp_task->_file_path, p_task->_task_info->_file_path_len,POS_PATH , &writesize);
			CHECK_VALUE(ret_val);
			g_pos_task_file_end+= writesize;
			if(writesize!=p_task->_task_info->_file_path_len)
			{
				CHECK_VALUE( INVALID_WRITE_SIZE);
			}

			if(p_task->_task_info->_file_name_len!=0)
			{
				writesize=0;
				ret_val = em_pwrite(g_task_file_id, (char *)p_p2sp_task->_file_name, p_task->_task_info->_file_name_len, POS_NAME , &writesize);
				CHECK_VALUE(ret_val);
				g_pos_task_file_end+= writesize;
				if(writesize!=p_task->_task_info->_file_name_len)
				{
					CHECK_VALUE( INVALID_WRITE_SIZE);
				}
			}
////
			if(p_task->_task_info->_url_len_or_need_dl_num!=0)
			{
				writesize=0;
				ret_val = em_pwrite(g_task_file_id, (char *)p_p2sp_task->_url, p_task->_task_info->_url_len_or_need_dl_num, POS_URL , &writesize);
				CHECK_VALUE(ret_val);
				g_pos_task_file_end+= writesize;
				if(writesize!=p_task->_task_info->_url_len_or_need_dl_num)
				{
					CHECK_VALUE( INVALID_WRITE_SIZE);
				}
			}		

			if(p_task->_task_info->_ref_url_len_or_seed_path_len!=0)
			{
				writesize=0;
				ret_val = em_pwrite(g_task_file_id, (char *)p_p2sp_task->_ref_url, p_task->_task_info->_ref_url_len_or_seed_path_len, POS_REF_URL , &writesize);
				CHECK_VALUE(ret_val);
				g_pos_task_file_end+= writesize;
				if(writesize!=p_task->_task_info->_ref_url_len_or_seed_path_len)
				{
					CHECK_VALUE( INVALID_WRITE_SIZE);
				}
			}

			if(p_task->_task_info->_user_data_len!=0)
			{
				writesize=0;
				ret_val = em_pwrite(g_task_file_id, (char *)p_p2sp_task->_user_data, p_task->_task_info->_user_data_len, POS_P2SP_USER_DATA , &writesize);
				CHECK_VALUE(ret_val);
				g_pos_task_file_end+= writesize;
				if(writesize!=p_task->_task_info->_user_data_len)
				{
					CHECK_VALUE( INVALID_WRITE_SIZE);
				}
			}
			
			if(p_task->_task_info->_have_tcid != FALSE)
			{
				writesize=0;
				ret_val = em_pwrite(g_task_file_id, (char *)p_p2sp_task->_tcid, CID_SIZE, POS_TCID , &writesize);
				CHECK_VALUE(ret_val);
				g_pos_task_file_end+= writesize;
				if(writesize!=CID_SIZE)
				{
					CHECK_VALUE( INVALID_WRITE_SIZE);
				}
			}
			
	return SUCCESS;
}

_int32 dt_disable_task_in_file(EM_TASK * p_task)
{
	_int32 ret_val=SUCCESS;
	_u32 writesize=0;
	_u16 valid_flag=0;	
#ifdef STRONG_FILE_CHECK
	_u32 readsize=0;
	DT_TASK_STORE task_store;
#endif /* STRONG_FILE_CHECK */		
	
	dt_stop_clear_task_file();

	CHECK_FILE_OPEN_INT;

		/* write to file */
		writesize=0;
		if(p_task->_offset<POS_TASK_START)
		{
	      		EM_LOG_ERROR("dt_disable_task_in_file:ERROR:p_task->_offset(%u)<POS_TASK_START(%u)",p_task->_offset,POS_TASK_START); 
			sd_assert(FALSE);
			return INVALID_TASK;
		}
		
#ifdef STRONG_FILE_CHECK
		/* read from file */
		/* DT_TASK_STORE */
		ret_val=em_pread(g_task_file_id, (char *)&task_store,sizeof(DT_TASK_STORE), p_task->_offset, &readsize);
		if((ret_val!=SUCCESS)||(readsize!=sizeof(DT_TASK_STORE)))
		{
	      		EM_LOG_ERROR("dt_disable_task_in_file:ERROR1:ret_val=%d,readsize(%u)<sizeof(DT_TASK_STORE)(%u)",ret_val,readsize,sizeof(DT_TASK_STORE)); 
			sd_assert(FALSE);
			return INVALID_READ_SIZE;
		}
		
		if(task_store._valid==0)
		{
	      		EM_LOG_ERROR("dt_disable_task_in_file:ERROR2:task_store._valid==0"); 
			sd_assert(FALSE);
			return INVALID_TASK;
		}
		
		if(task_store._task_info._task_id!=p_task->_task_info->_task_id)
		{
	      		EM_LOG_ERROR("dt_disable_task_in_file:ERROR3:task_store._task_info._task_id(%u)!=p_task->_task_id(%u)",task_store._task_info._task_id,p_task->_task_info->_task_id); 
			sd_assert(FALSE);
			return INVALID_TASK;
		}
#endif /* STRONG_FILE_CHECK */		
	
	ret_val = em_pwrite(g_task_file_id, (char *)&valid_flag, sizeof(_u16), POS_VALID , &writesize);
	CHECK_VALUE(ret_val);
	if(writesize!=sizeof(_u16))
	{
		CHECK_VALUE( INVALID_WRITE_SIZE);
	}
	
      EM_LOG_DEBUG("dt_disable_task_in_file:task_id=%u",p_task->_task_info->_task_id); 
	//g_need_clear=TRUE;
	return SUCCESS;
}

EM_TASK *  dt_get_task_from_file(void)
{
	static EM_TASK task;
	static TASK_INFO task_info;
	static _u32 invalid_task_count=0;
	_int32 ret_val=SUCCESS,i=0;
	_u32 readsize=0;
	DT_TASK_STORE_HEAD dt_task_store_head;

      EM_LOG_DEBUG("dt_get_task_from_file:g_task_file_id=%u,g_pos_task_file_end=%u,POS_TASK_START=%u",g_task_file_id,g_pos_task_file_end,POS_TASK_START); 
	  
	CHECK_FILE_OPEN;

	em_memset(&task,0,sizeof(EM_TASK));
READ_NEXT:	
	sd_assert(g_pos_task_file_end>=POS_TASK_START);
	em_memset(&dt_task_store_head,0,sizeof(DT_TASK_STORE_HEAD));
	/* read from file */
	readsize=0;
	ret_val=em_pread(g_task_file_id, (char *)&dt_task_store_head,sizeof(DT_TASK_STORE_HEAD), g_pos_task_file_end, &readsize);
	//sd_assert(ret_val==SUCCESS);
	if((ret_val!=SUCCESS)||(readsize!=sizeof(DT_TASK_STORE_HEAD)))
	{
		if(invalid_task_count>=MAX_INVALID_TASK_NUM)
		{
			g_need_clear=TRUE;
		}
      		EM_LOG_DEBUG("dt_get_task_from_file:ret_val=%d,readsize=%u,sizeof(DT_TASK_STORE_HEAD)=%d,invalid_task_count=%d,g_pos_task_file_end=%u",ret_val,readsize,sizeof(DT_TASK_STORE_HEAD),invalid_task_count,g_pos_task_file_end); 
		invalid_task_count = 0;
		return NULL;
	}

			EM_LOG_DEBUG("get_task[%u]:valid=%u,crc=%u,len=%u",g_pos_task_file_end,
				dt_task_store_head._valid,dt_task_store_head._crc,dt_task_store_head._len);

	if(dt_task_store_head._len>=MAX_P2SP_TASK_FULL_INFO_LEN)
	{
		/* 读到一个长度不正常的任务信息，该文件已经被破坏! */
		if(invalid_task_count>=MAX_INVALID_TASK_NUM)
		{
			g_need_clear=TRUE;
		}
      		EM_LOG_ERROR("dt_get_task_from_file:get a big task!dt_task_store_head._len=%u,invalid_task_count=%d,g_pos_task_file_end=%u",dt_task_store_head._len,invalid_task_count,g_pos_task_file_end); 
		invalid_task_count = 0;
		return NULL;
	}

	if(dt_task_store_head._valid==0)
	{
		g_pos_task_file_end+=readsize;
		g_pos_task_file_end+=dt_task_store_head._len;
		invalid_task_count++;
		goto READ_NEXT;
	}
	else
	if(dt_task_store_head._valid!=1)
	{
		/* 读到一个不正常的任务信息，该文件已经被破坏! */
		if(invalid_task_count>=MAX_INVALID_TASK_NUM)
		{
			g_need_clear=TRUE;
		}
      		EM_LOG_ERROR("dt_get_task_from_file:get bad task!dt_task_store_head._len=%u,invalid_task_count=%d,g_pos_task_file_end=%u",dt_task_store_head._len,invalid_task_count,g_pos_task_file_end); 
		invalid_task_count = 0;
		return NULL;
	}
	
	sd_assert(dt_task_store_head._valid==1);
	
	/* read from file */
	readsize=0;
	ret_val=em_pread(g_task_file_id, (char *)&task_info,sizeof(TASK_INFO), g_pos_task_file_end+sizeof(DT_TASK_STORE_HEAD), &readsize);
	//sd_assert(ret_val==SUCCESS);
	if((ret_val!=SUCCESS)||(readsize!=sizeof(TASK_INFO)))
	{
		if(invalid_task_count>=MAX_INVALID_TASK_NUM)
		{
			g_need_clear=TRUE;
		}
      		EM_LOG_ERROR("dt_get_task_from_file:ERROR2:ret_val=%d,readsize=%u,sizeof(TASK_INFO)=%d,invalid_task_count=%d,g_pos_task_file_end=%u",ret_val,readsize,sizeof(TASK_INFO),invalid_task_count,g_pos_task_file_end); 
		invalid_task_count = 0;
		sd_assert(FALSE);
		return NULL;
	}

			EM_LOG_DEBUG("info[%u]:id=%u,t=%u,s=%u,\nd=%d,ha_n=%d,check=%d,hav_cid=%d,hav_rurl=%d,hav_ud=%d,full_info=%d,correct_name=%d,\npath_len=%u,name_len=%u,url_len=%u,rurl_len=%u,ud_len=%u,\neigenvalue=%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,\nsize=%llu,dl_size=%llu,stime=%u,ftime=%u,fcode=%u,bt_fn=%u\n",g_pos_task_file_end,
				task_info._task_id,task_info._type,task_info._state,task_info._is_deleted,
				task_info._have_name,task_info._check_data,task_info._have_tcid,task_info._have_ref_url,task_info._have_user_data,task_info._full_info,
				task_info._correct_file_name,task_info._file_path_len,task_info._file_name_len,task_info._url_len_or_need_dl_num,task_info._ref_url_len_or_seed_path_len,task_info._user_data_len,
				task_info._eigenvalue[0],task_info._eigenvalue[1],task_info._eigenvalue[2],task_info._eigenvalue[3],task_info._eigenvalue[4],task_info._eigenvalue[5],
				task_info._eigenvalue[6],task_info._eigenvalue[7],task_info._eigenvalue[8],task_info._eigenvalue[9],task_info._eigenvalue[10],task_info._eigenvalue[11],
				task_info._eigenvalue[12],task_info._eigenvalue[13],task_info._eigenvalue[14],task_info._eigenvalue[15],task_info._eigenvalue[16],task_info._eigenvalue[17],
				task_info._eigenvalue[18],task_info._eigenvalue[19],task_info._file_size,task_info._downloaded_data_size,task_info._start_time,task_info._finished_time
				,task_info._failed_code,task_info._bt_total_file_num);
#if 0
	if((task_info._task_id>MAX_DL_TASK_ID)&&task_info._check_data)
	{
		_u16 valid_flag=0;
		_u32 writesize = 0;
		/* 无盘点播任务,直接废弃 */
		ret_val = em_pwrite(g_task_file_id, (char *)&valid_flag, sizeof(_u16), g_pos_task_file_end , &writesize);
		CHECK_VALUE(ret_val);
		if((ret_val != SUCCESS)||(writesize!=sizeof(_u16)))
		{
			sd_assert(FALSE);
			return NULL;
		}
		g_pos_task_file_end+=sizeof(DT_TASK_STORE_HEAD);
		g_pos_task_file_end+=dt_task_store_head._len;
		invalid_task_count++;
		goto READ_NEXT;
	}
#endif	
	task._task_info= &task_info;
	task._task_info->_full_info= FALSE;
	if((task._task_info->_state ==TS_TASK_RUNNING)||(task._task_info->_state ==TS_TASK_WAITING))
	{
		if(em_is_task_auto_start()==TRUE)
		{
			if(dt_is_vod_task(&task))
			{
				task._task_info->_state =TS_TASK_PAUSED;
				task._change_flag|=CHANGE_STATE;
			}else
			if(task._task_info->_state !=TS_TASK_WAITING)
			{
				task._task_info->_state =TS_TASK_WAITING;
				dt_have_task_waitting();
				task._change_flag|=CHANGE_STATE;
			}
		}
		else
		{
			task._task_info->_state =TS_TASK_PAUSED;
			task._change_flag|=CHANGE_STATE;
		}
	}
	else
	if((em_is_task_auto_start())&&(task._task_info->_state ==TS_TASK_PAUSED))
	{
		if(!running_task_loaded)
		{
	 		 running_task_ids =dt_get_running_tasks_from_file();
			 running_task_loaded=TRUE;
		}
	
		if(running_task_ids!=NULL)
		{
			for(i=0;i<MAX_ALOW_TASKS;i++)
			{
				if(running_task_ids[i]==task._task_info->_task_id)
				{
					task._task_info->_state =TS_TASK_WAITING;
					dt_have_task_waitting();
					task._change_flag|=CHANGE_STATE;
				}
			}
		}
	}

	if((task._task_info->_state ==TS_TASK_PAUSED) && (task._task_info->_file_size!=0&&task._task_info->_downloaded_data_size == task._task_info->_file_size))
	{
		task._task_info->_state = TS_TASK_SUCCESS;
	}

	task._offset= g_pos_task_file_end;
		
	g_pos_task_file_end+=sizeof(DT_TASK_STORE_HEAD);
	g_pos_task_file_end+=dt_task_store_head._len;

	/* need check the crc value .... */
	
      EM_LOG_DEBUG("dt_get_task_from_file:task_id=%u",task_info._task_id); 
	return &task;
}

_int32 dt_save_task_to_file(EM_TASK * p_task)
{
	_int32 ret_val=SUCCESS;
	_u32 writesize=0;
#ifdef STRONG_FILE_CHECK		
	_u32 readsize=0;
	DT_TASK_STORE task_store;
#endif /* STRONG_FILE_CHECK */		

	dt_stop_clear_task_file();

	CHECK_FILE_OPEN_INT;

	if(p_task->_change_flag&CHANGE_TOTAL_LEN)
	{
		sd_assert(FALSE);
		return -1;
	}

      EM_LOG_DEBUG("dt_save_task_to_file:task_id=%u",p_task->_task_info->_task_id); 
	////////////////////////// TASK INFO  /////////////////////////////////////
#if 1 //defined(__SYMBIAN32__)&&defined(__WINSCW__)
	if(p_task->_change_flag!=0)
	{
		/* write to file */
		writesize=0;
		if(p_task->_offset<POS_TASK_START)
		{
	      		EM_LOG_ERROR("dt_save_task_to_file:ERROR:p_task->_offset(%u)<POS_TASK_START(%u)",p_task->_offset,POS_TASK_START); 
			sd_assert(FALSE);
			return INVALID_TASK;
		}
#ifdef STRONG_FILE_CHECK		
		/* read from file */
		/* DT_TASK_STORE */
		ret_val=em_pread(g_task_file_id, (char *)&task_store,sizeof(DT_TASK_STORE), p_task->_offset, &readsize);
		if((ret_val!=SUCCESS)||(readsize!=sizeof(DT_TASK_STORE)))
		{
	      		EM_LOG_ERROR("dt_save_task_to_file:ERROR1:ret_val=%d,readsize(%u)<sizeof(DT_TASK_STORE)(%u)",ret_val,readsize,sizeof(DT_TASK_STORE)); 
			sd_assert(FALSE);
			return INVALID_READ_SIZE;
		}
		
		if(task_store._valid==0)
		{
	      		EM_LOG_ERROR("dt_save_task_to_file:ERROR2:task_store._valid==0"); 
			sd_assert(FALSE);
			return INVALID_TASK;
		}
		
		if(task_store._task_info._task_id!=p_task->_task_info->_task_id)
		{
	      		EM_LOG_ERROR("dt_save_task_to_file:ERROR3:task_store._task_info._task_id(%u)!=p_task->_task_id(%u)",task_store._task_info._task_id,p_task->_task_info->_task_id); 
			sd_assert(FALSE);
			return INVALID_TASK;
		}
#endif /* STRONG_FILE_CHECK */		
		ret_val = em_pwrite(g_task_file_id, (char *)(p_task->_task_info), sizeof(TASK_INFO), POS_TASK_INFO , &writesize);
		CHECK_VALUE(ret_val);
		if(writesize!=sizeof(TASK_INFO))
		{
			CHECK_VALUE( INVALID_WRITE_SIZE);
		}
	}
#else
	if(p_task->_change_flag&CHANGE_STATE)
	{
		/* write to file */
		writesize=0;
		ret_val = em_pwrite(g_task_file_id, (char *)(((_u8*)(p_task->_task_info))+sizeof(_u32)), sizeof(_u8), POS_STATE , &writesize);
		CHECK_VALUE(ret_val);
		if(writesize!=sizeof(_u8))
		{
			CHECK_VALUE( INVALID_WRITE_SIZE);
		}
	}

	if(((p_task->_change_flag&CHANGE_DELETE))||(p_task->_change_flag&CHANGE_HAVE_NAME))
	{
		/* write to file */
		writesize=0;
		ret_val = em_pwrite(g_task_file_id, (char *)(((_u8*)(p_task->_task_info))+sizeof(_u32)+1), sizeof(_u8), POS_IS_DELETED , &writesize);
		CHECK_VALUE(ret_val);
		if(writesize!=sizeof(_u8))
		{
			CHECK_VALUE( INVALID_WRITE_SIZE);
		}
	}

	if(p_task->_change_flag&CHANGE_FILE_SIZE)
	{
		/* write to file */
		writesize=0;
		ret_val = em_pwrite(g_task_file_id, (char *)&p_task->_task_info->_file_size, sizeof(_u64), POS_FILE_SIZE, &writesize);
		CHECK_VALUE(ret_val);
		if(writesize!=sizeof(_u64))
		{
			CHECK_VALUE( INVALID_WRITE_SIZE);
		}
	}

	if(p_task->_change_flag&CHANGE_DLED_SIZE)
	{
		/* write to file */
		writesize=0;
		ret_val = em_pwrite(g_task_file_id, (char *)&p_task->_task_info->_downloaded_data_size, sizeof(_u64), POS_DLED_SIZE, &writesize);
		CHECK_VALUE(ret_val);
		if(writesize!=sizeof(_u64))
		{
			CHECK_VALUE( INVALID_WRITE_SIZE);
		}
	}

	if(p_task->_change_flag&CHANGE_START_TIME)
	{
		/* write to file */
		writesize=0;
		ret_val = em_pwrite(g_task_file_id, (char *)&p_task->_task_info->_start_time, sizeof(_u32), POS_START_TIME, &writesize);
		CHECK_VALUE(ret_val);
		if(writesize!=sizeof(_u32))
		{
			CHECK_VALUE( INVALID_WRITE_SIZE);
		}
	}

	if(p_task->_change_flag&CHANGE_FINISHED_TIME)
	{
		/* write to file */
		writesize=0;
		ret_val = em_pwrite(g_task_file_id, (char *)&p_task->_task_info->_finished_time, sizeof(_u32), POS_FINISH_TIME, &writesize);
		CHECK_VALUE(ret_val);
		if(writesize!=sizeof(_u32))
		{
			CHECK_VALUE( INVALID_WRITE_SIZE);
		}
	}

	if(p_task->_change_flag&CHANGE_FAILED_CODE)
	{
		/* write to file */
		writesize=0;
		ret_val = em_pwrite(g_task_file_id, (char *)&p_task->_task_info->_failed_code, sizeof(_u32), POS_FAILED_CODE, &writesize);
		CHECK_VALUE(ret_val);
		if(writesize!=sizeof(_u32))
		{
			CHECK_VALUE( INVALID_WRITE_SIZE);
		}
	}
#endif
	////////////////////////// BT SUB FILE /////////////////////////////////////

	if(p_task->_change_flag&CHANGE_BT_NEED_DL_FILE)
	{
		sd_assert(FALSE);
	}

	if(p_task->_change_flag&CHANGE_BT_SUB_FILE_DLED_SIZE)
	{
		sd_assert(FALSE);
	}

	if(p_task->_change_flag&CHANGE_BT_SUB_FILE_STATUS)
	{
		sd_assert(FALSE);
	}

	if(p_task->_change_flag&CHANGE_BT_SUB_FILE_FAILED_CODE)
	{
		sd_assert(FALSE);
	}
	
	p_task->_change_flag = 0;
      EM_LOG_DEBUG("dt_save_task_to_file success!:task_id=%u",p_task->_task_info->_task_id); 
	return SUCCESS;
}
_int32 dt_save_p2sp_task_url_to_file(EM_TASK * p_task,char* new_url,_u32  url_len)
{
	_int32 ret_val=SUCCESS;
	_u32 readsize=0;
	EM_P2SP_TASK p2sp_task;
	EM_TASK task_tmp;
	char path_buffer[MAX_FILE_PATH_LEN],name_buffer[MAX_FILE_PATH_LEN],ref_url_buffer[MAX_URL_LEN];
	void * user_data= NULL;

	if(TT_LAN!=p_task->_task_info->_type)
	{
		sd_assert(FALSE);
		return INVALID_TASK_TYPE;
	}
	
	dt_stop_clear_task_file();

	CHECK_FILE_OPEN_INT;
		
	if(p_task->_task_info->_full_info )
	{
		ret_val = dt_disable_task_in_file(p_task);
		CHECK_VALUE(ret_val);
		
		p_task->_offset = 0;
		/* write to file */
		ret_val = dt_add_task_to_file(p_task);
		//CHECK_VALUE(ret_val);
	}
	else
	{

		em_memset(&p2sp_task,0,sizeof(EM_P2SP_TASK));
		em_memset(&task_tmp,0,sizeof(EM_TASK));

		task_tmp._task_info = (TASK_INFO*)&p2sp_task;
		em_memcpy(task_tmp._task_info,p_task->_task_info,sizeof(TASK_INFO));

		task_tmp._task_info->_url_len_or_need_dl_num= url_len;
		task_tmp._task_info->_full_info = TRUE;
		p2sp_task._url = new_url;
	
		/* read file_path from file */
		readsize = 0;
		em_memset(path_buffer,0,MAX_FILE_PATH_LEN);
		ret_val=em_pread(g_task_file_id, (char *)path_buffer,p_task->_task_info->_file_path_len, POS_PATH, &readsize);
		CHECK_VALUE(ret_val);
		if(readsize!=p_task->_task_info->_file_path_len)
		{
			CHECK_VALUE(INVALID_READ_SIZE);
		}
		p2sp_task._file_path = path_buffer;

		readsize = 0;
		em_memset(name_buffer,0,MAX_FILE_NAME_BUFFER_LEN);
		ret_val=em_pread(g_task_file_id, (char *)name_buffer,p_task->_task_info->_file_name_len, POS_NAME, &readsize);
		CHECK_VALUE(ret_val);
		if(readsize!=p_task->_task_info->_file_name_len)
		{
			CHECK_VALUE(INVALID_READ_SIZE);
		}
		p2sp_task._file_name = name_buffer;

		if(p_task->_task_info->_ref_url_len_or_seed_path_len!=0)
		{
			/* read ref_url from file */
			readsize = 0;
			em_memset(ref_url_buffer,0,MAX_URL_LEN);
			ret_val=em_pread(g_task_file_id, (char *)ref_url_buffer,p_task->_task_info->_ref_url_len_or_seed_path_len, POS_REF_URL, &readsize);
			CHECK_VALUE(ret_val);
			if(readsize!=p_task->_task_info->_ref_url_len_or_seed_path_len)
			{
				CHECK_VALUE(INVALID_READ_SIZE);
			}
			p2sp_task._ref_url= ref_url_buffer;
		}
			
		if(p_task->_task_info->_have_user_data!=FALSE)
		{
			/* read user_data from file */
			ret_val = em_malloc(p_task->_task_info->_user_data_len,(void**)&user_data);
			CHECK_VALUE(ret_val);

			readsize = 0;
			//sd_memset(user_data,0,p_task->_task_info->_user_data_len);
			ret_val=em_pread(g_task_file_id, (char *)user_data,p_task->_task_info->_user_data_len, POS_P2SP_USER_DATA, &readsize);
			if(ret_val!=SUCCESS)
			{
				EM_SAFE_DELETE(user_data);
				CHECK_VALUE(ret_val);
			}

			if(readsize!=p_task->_task_info->_user_data_len)
			{
				EM_SAFE_DELETE(user_data);
				CHECK_VALUE(INVALID_READ_SIZE);
			}
			p2sp_task._user_data= user_data;
		}
	
	
		if(p_task->_task_info->_have_tcid!=FALSE)
		{
			/* read tcid from file */
			readsize = 0;
			em_memset(&p2sp_task._tcid,0,CID_SIZE);
			ret_val=em_pread(g_task_file_id, (char *)&p2sp_task._tcid,CID_SIZE, POS_TCID, &readsize);
			if(ret_val!=SUCCESS)
			{
				EM_SAFE_DELETE(user_data);
				CHECK_VALUE(ret_val);
			}

			if(readsize!=CID_SIZE)
			{
				EM_SAFE_DELETE(user_data);
				CHECK_VALUE(INVALID_READ_SIZE);
			}
		}
		
		
		/* write to file */
		ret_val = dt_add_task_to_file(&task_tmp);
		EM_SAFE_DELETE(user_data);
		CHECK_VALUE(ret_val);

		ret_val = dt_disable_task_in_file(p_task);
		sd_assert(ret_val==SUCCESS);
		if(ret_val==SUCCESS)
		{
			p_task->_offset = task_tmp._offset;
			p_task->_task_info->_url_len_or_need_dl_num = url_len;
		}

	}
		
	return ret_val;
}

_int32 dt_save_p2sp_task_tcid_to_file(EM_TASK * p_task,_u8  * tcid)
{
	_int32 ret_val=SUCCESS;
	_u32 readsize=0;
	EM_P2SP_TASK p2sp_task;
	EM_TASK task_tmp;
	char path_buffer[MAX_FILE_PATH_LEN],name_buffer[MAX_FILE_NAME_BUFFER_LEN],url_buffer[MAX_URL_LEN],ref_url_buffer[MAX_URL_LEN];
	void * user_data= NULL;
	//BT_TASK * p_bt_task=NULL;

	dt_stop_clear_task_file();

	CHECK_FILE_OPEN_INT;
		
	if(p_task->_task_info->_full_info )
	{
		ret_val = dt_disable_task_in_file(p_task);
		CHECK_VALUE(ret_val);
		
		p_task->_offset = 0;
		/* write to file */
		ret_val = dt_add_task_to_file(p_task);
		//CHECK_VALUE(ret_val);
	}
	else
	{

		em_memset(&p2sp_task,0,sizeof(EM_P2SP_TASK));
		em_memset(&task_tmp,0,sizeof(EM_TASK));

		task_tmp._task_info = (TASK_INFO*)&p2sp_task;
		em_memcpy(task_tmp._task_info,p_task->_task_info,sizeof(TASK_INFO));

		sd_assert(p_task->_task_info->_have_tcid==FALSE);
		
		task_tmp._task_info->_have_tcid = TRUE;
		em_memcpy(p2sp_task._tcid,tcid,CID_SIZE);
	
		task_tmp._task_info->_full_info = TRUE;
		
		/* read file_path from file */
		readsize = 0;
		em_memset(path_buffer,0,MAX_FILE_PATH_LEN);
		ret_val=em_pread(g_task_file_id, (char *)path_buffer,p_task->_task_info->_file_path_len, POS_PATH, &readsize);
		CHECK_VALUE(ret_val);
		if(readsize!=p_task->_task_info->_file_path_len)
		{
			CHECK_VALUE(INVALID_READ_SIZE);
		}
		p2sp_task._file_path = path_buffer;

		/* read file_name from file */
		readsize = 0;
		em_memset(name_buffer,0,MAX_FILE_NAME_BUFFER_LEN);
		ret_val=em_pread(g_task_file_id, (char *)name_buffer,p_task->_task_info->_file_name_len, POS_NAME, &readsize);
		CHECK_VALUE(ret_val);
		if(readsize!=p_task->_task_info->_file_name_len)
		{
			CHECK_VALUE(INVALID_READ_SIZE);
		}
		p2sp_task._file_name = name_buffer;

		if(p_task->_task_info->_url_len_or_need_dl_num!=0)
		{
			/* read url from file */
			readsize = 0;
			em_memset(url_buffer,0,MAX_URL_LEN);
			ret_val=em_pread(g_task_file_id, (char *)url_buffer,p_task->_task_info->_url_len_or_need_dl_num, POS_URL, &readsize);
			CHECK_VALUE(ret_val);
			if(readsize!=p_task->_task_info->_url_len_or_need_dl_num)
			{
				CHECK_VALUE(INVALID_READ_SIZE);
			}
			p2sp_task._url= url_buffer;
		}

		if(p_task->_task_info->_ref_url_len_or_seed_path_len!=0)
		{
			/* read ref_url from file */
			readsize = 0;
			em_memset(ref_url_buffer,0,MAX_URL_LEN);
			ret_val=em_pread(g_task_file_id, (char *)ref_url_buffer,p_task->_task_info->_ref_url_len_or_seed_path_len, POS_REF_URL, &readsize);
			CHECK_VALUE(ret_val);
			if(readsize!=p_task->_task_info->_ref_url_len_or_seed_path_len)
			{
				CHECK_VALUE(INVALID_READ_SIZE);
			}
			p2sp_task._ref_url= ref_url_buffer;
		}
			
		if(p_task->_task_info->_have_user_data!=FALSE)
		{
			/* read user_data from file */
			ret_val = em_malloc(p_task->_task_info->_user_data_len,(void**)&user_data);
			CHECK_VALUE(ret_val);

			readsize = 0;
			//sd_memset(user_data,0,p_task->_task_info->_user_data_len);
			ret_val=em_pread(g_task_file_id, (char *)user_data,p_task->_task_info->_user_data_len, POS_P2SP_USER_DATA, &readsize);
			if(ret_val!=SUCCESS)
			{
				EM_SAFE_DELETE(user_data);
				CHECK_VALUE(ret_val);
			}

			if(readsize!=p_task->_task_info->_user_data_len)
			{
				EM_SAFE_DELETE(user_data);
				CHECK_VALUE(INVALID_READ_SIZE);
			}
			p2sp_task._user_data= user_data;
		}
	
		/*
		if(p_task->_task_info->_have_tcid!=FALSE)
		{
			// read tcid from file 
			readsize = 0;
			em_memset(&p2sp_task._tcid,0,CID_SIZE);
			ret_val=em_pread(g_task_file_id, (char *)&p2sp_task._tcid,CID_SIZE, POS_TCID, &readsize);
			if(ret_val!=SUCCESS)
			{
				EM_SAFE_DELETE(user_data);
				CHECK_VALUE(ret_val);
			}

			if(readsize!=CID_SIZE)
			{
				EM_SAFE_DELETE(user_data);
				CHECK_VALUE(INVALID_READ_SIZE);
			}
		}
		*/
		
		/* write to file */
		ret_val = dt_add_task_to_file(&task_tmp);
		EM_SAFE_DELETE(user_data);
		CHECK_VALUE(ret_val);

		ret_val = dt_disable_task_in_file(p_task);
		sd_assert(ret_val==SUCCESS);
		if(ret_val==SUCCESS)
		{
			p_task->_offset = task_tmp._offset;
			p_task->_task_info->_have_tcid = TRUE;
		}

	}
		
	return ret_val;
}
_int32 dt_save_p2sp_task_path_to_file(EM_TASK * p_task,char* new_path,_u32  path_len)
{
	_int32 ret_val=SUCCESS;
	_u32 readsize=0;
	EM_P2SP_TASK p2sp_task;
	EM_TASK task_tmp;
	char name_buffer[MAX_FILE_NAME_BUFFER_LEN],url_buffer[MAX_URL_LEN],ref_url_buffer[MAX_URL_LEN];
	void * user_data= NULL;
	//BT_TASK * p_bt_task=NULL;

	dt_stop_clear_task_file();

	CHECK_FILE_OPEN_INT;
		
	if(p_task->_task_info->_full_info )
	{
		ret_val = dt_disable_task_in_file(p_task);
		CHECK_VALUE(ret_val);
		
		p_task->_offset = 0;
		/* write to file */
		ret_val = dt_add_task_to_file(p_task);
		//CHECK_VALUE(ret_val);
	}
	else
	{

		em_memset(&p2sp_task,0,sizeof(EM_P2SP_TASK));
		em_memset(&task_tmp,0,sizeof(EM_TASK));

		task_tmp._task_info = (TASK_INFO*)&p2sp_task;
		em_memcpy(task_tmp._task_info,p_task->_task_info,sizeof(TASK_INFO));

		task_tmp._task_info->_file_path_len = path_len;
		task_tmp._task_info->_full_info = TRUE;
		p2sp_task._file_path = new_path;
	
		/* read file_path from file */
		readsize = 0;
		em_memset(name_buffer,0,MAX_FILE_NAME_BUFFER_LEN);
		ret_val=em_pread(g_task_file_id, (char *)name_buffer,p_task->_task_info->_file_name_len, POS_NAME, &readsize);
		CHECK_VALUE(ret_val);
		if(readsize!=p_task->_task_info->_file_name_len)
		{
			CHECK_VALUE(INVALID_READ_SIZE);
		}
		p2sp_task._file_name = name_buffer;

		if(p_task->_task_info->_url_len_or_need_dl_num!=0)
		{
			/* read url from file */
			readsize = 0;
			em_memset(url_buffer,0,MAX_URL_LEN);
			ret_val=em_pread(g_task_file_id, (char *)url_buffer,p_task->_task_info->_url_len_or_need_dl_num, POS_URL, &readsize);
			CHECK_VALUE(ret_val);
			if(readsize!=p_task->_task_info->_url_len_or_need_dl_num)
			{
				CHECK_VALUE(INVALID_READ_SIZE);
			}
			p2sp_task._url= url_buffer;
		}

		if(p_task->_task_info->_ref_url_len_or_seed_path_len!=0)
		{
			/* read ref_url from file */
			readsize = 0;
			em_memset(ref_url_buffer,0,MAX_URL_LEN);
			ret_val=em_pread(g_task_file_id, (char *)ref_url_buffer,p_task->_task_info->_ref_url_len_or_seed_path_len, POS_REF_URL, &readsize);
			CHECK_VALUE(ret_val);
			if(readsize!=p_task->_task_info->_ref_url_len_or_seed_path_len)
			{
				CHECK_VALUE(INVALID_READ_SIZE);
			}
			p2sp_task._ref_url= ref_url_buffer;
		}
			
		if(p_task->_task_info->_have_user_data!=FALSE)
		{
			/* read user_data from file */
			ret_val = em_malloc(p_task->_task_info->_user_data_len,(void**)&user_data);
			CHECK_VALUE(ret_val);

			readsize = 0;
			//sd_memset(user_data,0,p_task->_task_info->_user_data_len);
			ret_val=em_pread(g_task_file_id, (char *)user_data,p_task->_task_info->_user_data_len, POS_P2SP_USER_DATA, &readsize);
			if(ret_val!=SUCCESS)
			{
				EM_SAFE_DELETE(user_data);
				CHECK_VALUE(ret_val);
			}

			if(readsize!=p_task->_task_info->_user_data_len)
			{
				EM_SAFE_DELETE(user_data);
				CHECK_VALUE(INVALID_READ_SIZE);
			}
			p2sp_task._user_data= user_data;
		}
	
	
		if(p_task->_task_info->_have_tcid!=FALSE)
		{
			/* read tcid from file */
			readsize = 0;
			em_memset(&p2sp_task._tcid,0,CID_SIZE);
			ret_val=em_pread(g_task_file_id, (char *)&p2sp_task._tcid,CID_SIZE, POS_TCID, &readsize);
			if(ret_val!=SUCCESS)
			{
				EM_SAFE_DELETE(user_data);
				CHECK_VALUE(ret_val);
			}

			if(readsize!=CID_SIZE)
			{
				EM_SAFE_DELETE(user_data);
				CHECK_VALUE(INVALID_READ_SIZE);
			}
		}
		
		
		/* write to file */
		ret_val = dt_add_task_to_file(&task_tmp);
		EM_SAFE_DELETE(user_data);
		CHECK_VALUE(ret_val);

		ret_val = dt_disable_task_in_file(p_task);
		sd_assert(ret_val==SUCCESS);
		if(ret_val==SUCCESS)
		{
			p_task->_offset = task_tmp._offset;
			p_task->_task_info->_file_path_len = path_len;
		}

	}
		
	return ret_val;
}

_int32 dt_save_p2sp_task_name_to_file(EM_TASK * p_task,char* new_name,_u32  name_len)
{
	_int32 ret_val=SUCCESS;
	_u32 readsize=0;
	EM_P2SP_TASK p2sp_task;
	EM_TASK task_tmp;
	char path_buffer[MAX_FILE_PATH_LEN],url_buffer[MAX_URL_LEN],ref_url_buffer[MAX_URL_LEN];
	void * user_data= NULL;
	//BT_TASK * p_bt_task=NULL;

	dt_stop_clear_task_file();

	CHECK_FILE_OPEN_INT;
		
	if(p_task->_task_info->_full_info )
	{
		ret_val = dt_disable_task_in_file(p_task);
		CHECK_VALUE(ret_val);
		
		p_task->_offset = 0;
		/* write to file */
		ret_val = dt_add_task_to_file(p_task);
		//CHECK_VALUE(ret_val);
	}
	else
	{

		em_memset(&p2sp_task,0,sizeof(EM_P2SP_TASK));
		em_memset(&task_tmp,0,sizeof(EM_TASK));

		task_tmp._task_info = (TASK_INFO*)&p2sp_task;
		em_memcpy(task_tmp._task_info,p_task->_task_info,sizeof(TASK_INFO));

		task_tmp._task_info->_have_name = TRUE;
		//task_tmp._task_info->_correct_file_name= TRUE;
		task_tmp._task_info->_file_name_len = name_len;
		task_tmp._task_info->_full_info = TRUE;
		p2sp_task._file_name = new_name;
	
		/* read file_path from file */
		readsize = 0;
		em_memset(path_buffer,0,MAX_FILE_PATH_LEN);
		ret_val=em_pread(g_task_file_id, (char *)path_buffer,p_task->_task_info->_file_path_len, POS_PATH, &readsize);
		CHECK_VALUE(ret_val);
		if(readsize!=p_task->_task_info->_file_path_len)
		{
			CHECK_VALUE(INVALID_READ_SIZE);
		}
		p2sp_task._file_path = path_buffer;

		if(p_task->_task_info->_url_len_or_need_dl_num!=0)
		{
			/* read url from file */
			readsize = 0;
			em_memset(url_buffer,0,MAX_URL_LEN);
			ret_val=em_pread(g_task_file_id, (char *)url_buffer,p_task->_task_info->_url_len_or_need_dl_num, POS_URL, &readsize);
			CHECK_VALUE(ret_val);
			if(readsize!=p_task->_task_info->_url_len_or_need_dl_num)
			{
				CHECK_VALUE(INVALID_READ_SIZE);
			}
			p2sp_task._url= url_buffer;
		}

		if(p_task->_task_info->_ref_url_len_or_seed_path_len!=0)
		{
			/* read ref_url from file */
			readsize = 0;
			em_memset(ref_url_buffer,0,MAX_URL_LEN);
			ret_val=em_pread(g_task_file_id, (char *)ref_url_buffer,p_task->_task_info->_ref_url_len_or_seed_path_len, POS_REF_URL, &readsize);
			CHECK_VALUE(ret_val);
			if(readsize!=p_task->_task_info->_ref_url_len_or_seed_path_len)
			{
				CHECK_VALUE(INVALID_READ_SIZE);
			}
			p2sp_task._ref_url= ref_url_buffer;
		}
			
		if(p_task->_task_info->_have_user_data!=FALSE)
		{
			/* read user_data from file */
			ret_val = em_malloc(p_task->_task_info->_user_data_len,(void**)&user_data);
			CHECK_VALUE(ret_val);

			readsize = 0;
			//sd_memset(user_data,0,p_task->_task_info->_user_data_len);
			ret_val=em_pread(g_task_file_id, (char *)user_data,p_task->_task_info->_user_data_len, POS_P2SP_USER_DATA, &readsize);
			if(ret_val!=SUCCESS)
			{
				EM_SAFE_DELETE(user_data);
				CHECK_VALUE(ret_val);
			}

			if(readsize!=p_task->_task_info->_user_data_len)
			{
				EM_SAFE_DELETE(user_data);
				CHECK_VALUE(INVALID_READ_SIZE);
			}
			p2sp_task._user_data= user_data;
		}
	
	
		if(p_task->_task_info->_have_tcid!=FALSE)
		{
			/* read tcid from file */
			readsize = 0;
			em_memset(&p2sp_task._tcid,0,CID_SIZE);
			ret_val=em_pread(g_task_file_id, (char *)&p2sp_task._tcid,CID_SIZE, POS_TCID, &readsize);
			if(ret_val!=SUCCESS)
			{
				EM_SAFE_DELETE(user_data);
				CHECK_VALUE(ret_val);
			}

			if(readsize!=CID_SIZE)
			{
				EM_SAFE_DELETE(user_data);
				CHECK_VALUE(INVALID_READ_SIZE);
			}
		}
		
		
		/* write to file */
		ret_val = dt_add_task_to_file(&task_tmp);
		EM_SAFE_DELETE(user_data);
		CHECK_VALUE(ret_val);

		ret_val = dt_disable_task_in_file(p_task);
		sd_assert(ret_val==SUCCESS);
		if(ret_val==SUCCESS)
		{
			p_task->_offset = task_tmp._offset;
			p_task->_task_info->_have_name = TRUE;
			//p_task->_task_info->_correct_file_name= TRUE;
			p_task->_task_info->_file_name_len = name_len;
		}

	}
		
	return ret_val;
}
_int32 dt_save_bt_task_name_to_file(EM_TASK * p_task,char* new_name,_u32  name_len)
{
	_int32 ret_val=SUCCESS;
	_u32 readsize=0;
	EM_BT_TASK bt_task,*p_bt_task=NULL;
	EM_TASK task_tmp;
	char path_buffer[MAX_FILE_PATH_LEN],seed_path_buffer[MAX_FULL_PATH_BUFFER_LEN];
	void * user_data= NULL;

	dt_stop_clear_task_file();

	CHECK_FILE_OPEN_INT;

	if(p_task->_task_info->_full_info)
	{
		p_bt_task=(EM_BT_TASK *)p_task->_task_info;
		
		ret_val = dt_disable_task_in_file(p_task);
		CHECK_VALUE(ret_val);

		p_task->_offset = 0;
		/* write to file */
		ret_val = dt_add_task_to_file(p_task);
		//CHECK_VALUE(ret_val);
	}
	else
	{
		em_memset(&bt_task,0,sizeof(EM_BT_TASK));
		em_memset(&task_tmp,0,sizeof(EM_TASK));

		task_tmp._task_info = (TASK_INFO*)&bt_task;
		em_memcpy(task_tmp._task_info,p_task->_task_info,sizeof(TASK_INFO));

		task_tmp._task_info->_have_name = TRUE;
		//task_tmp._task_info->_correct_file_name= TRUE;
		task_tmp._task_info->_file_name_len = name_len;
		task_tmp._task_info->_full_info = TRUE;
		bt_task._file_name = new_name;
		
		/* read file_path from file */
		readsize = 0;
		em_memset(path_buffer,0,MAX_FILE_PATH_LEN);
		ret_val=em_pread(g_task_file_id, (char *)path_buffer,p_task->_task_info->_file_path_len, POS_PATH, &readsize);
		CHECK_VALUE(ret_val);
		if(readsize!=p_task->_task_info->_file_path_len)
		{
			CHECK_VALUE(INVALID_READ_SIZE);
		}
		bt_task._file_path = path_buffer;

		/* read seed path from file */
		readsize = 0;
		em_memset(seed_path_buffer,0,MAX_FULL_PATH_BUFFER_LEN);
		ret_val=em_pread(g_task_file_id, (char *)seed_path_buffer,p_task->_task_info->_ref_url_len_or_seed_path_len, POS_SEED_PATH, &readsize);
		CHECK_VALUE(ret_val);
		if(readsize!=p_task->_task_info->_ref_url_len_or_seed_path_len)
		{
			CHECK_VALUE(INVALID_READ_SIZE);
		}
		bt_task._seed_file_path= seed_path_buffer;
		
		if(p_task->_task_info->_have_user_data!=FALSE)
		{
			/* read user_data from file */
			ret_val = em_malloc(p_task->_task_info->_user_data_len,(void**)&user_data);
			CHECK_VALUE(ret_val);

			readsize = 0;
			//sd_memset(user_data,0,p_task->_task_info->_user_data_len);
			ret_val=em_pread(g_task_file_id, (char *)user_data,p_task->_task_info->_user_data_len, POS_BT_USER_DATA, &readsize);
			if(ret_val!=SUCCESS) 
			{
				EM_SAFE_DELETE(user_data);
				CHECK_VALUE(ret_val);
			}

			if(readsize!=p_task->_task_info->_user_data_len)
			{
				EM_SAFE_DELETE(user_data);
				CHECK_VALUE(INVALID_READ_SIZE);
			}
			bt_task._user_data= user_data;
		}

		bt_task._dl_file_index_array = dt_get_task_bt_need_dl_file_index_array(p_task);
		if(bt_task._dl_file_index_array==NULL)
		{
			EM_SAFE_DELETE(user_data);
			CHECK_VALUE(INVALID_FILE_INDEX_ARRAY);
		}
		
		/* read bt file array from file */
		ret_val = em_malloc(p_task->_task_info->_url_len_or_need_dl_num*sizeof(BT_FILE),(void**)&bt_task._file_array);
		if(ret_val!=SUCCESS)
		{
			EM_SAFE_DELETE(user_data);
			dt_release_task_bt_need_dl_file_index_array(bt_task._dl_file_index_array);
			CHECK_VALUE(ret_val);
		}
		
		readsize = 0;
		//sd_memset(seed_path_buffer,0,MAX_FULL_PATH_BUFFER_LEN);
		ret_val=em_pread(g_task_file_id, (char *)bt_task._file_array,p_task->_task_info->_url_len_or_need_dl_num*sizeof(BT_FILE), POS_FILE_ARRAY, &readsize);
		if(ret_val!=SUCCESS)
		{
			EM_SAFE_DELETE(user_data);
			dt_release_task_bt_need_dl_file_index_array(bt_task._dl_file_index_array);
			EM_SAFE_DELETE(bt_task._file_array);
			CHECK_VALUE(ret_val);
		}
		if(readsize!=p_task->_task_info->_url_len_or_need_dl_num*sizeof(BT_FILE))
		{
			EM_SAFE_DELETE(user_data);
			dt_release_task_bt_need_dl_file_index_array(bt_task._dl_file_index_array);
			EM_SAFE_DELETE(bt_task._file_array);
			CHECK_VALUE(INVALID_READ_SIZE);
		}
		
		/* write to file */
		ret_val = dt_add_task_to_file(&task_tmp);
		EM_SAFE_DELETE(user_data);
		dt_release_task_bt_need_dl_file_index_array(bt_task._dl_file_index_array);
		EM_SAFE_DELETE(bt_task._file_array);
		CHECK_VALUE(ret_val);

		ret_val = dt_disable_task_in_file(p_task);
		sd_assert(ret_val==SUCCESS);
		if(ret_val==SUCCESS)
		{
			p_task->_offset = task_tmp._offset;
			p_task->_task_info->_have_name = TRUE;
			//p_task->_task_info->_correct_file_name= TRUE;
			p_task->_task_info->_file_name_len = name_len;
		}

	}
	return ret_val;
}

 char *  dt_get_task_file_path_from_file(EM_TASK * p_task)
{
	_int32 ret_val=SUCCESS;
	_u32 readsize=0;
	static char file_path[MAX_FILE_PATH_LEN];
	
	dt_stop_clear_task_file();

	CHECK_FILE_OPEN;
	
	em_memset(file_path,0,MAX_FILE_PATH_LEN);

	/* read from file */
	ret_val=em_pread(g_task_file_id, (char *)file_path,p_task->_task_info->_file_path_len, POS_PATH, &readsize);
	sd_assert(ret_val==SUCCESS);
	if((ret_val!=SUCCESS)||(readsize!=p_task->_task_info->_file_path_len))
	{
		return NULL;
	}
	
      EM_LOG_DEBUG("dt_get_task_file_path_from_file :task_id=%u,path=%s",p_task->_task_info->_task_id,file_path); 
	sd_assert(em_strlen(file_path)>0);
	return file_path;
}
 char *  dt_get_task_file_name_from_file(EM_TASK * p_task)
{
	_int32 ret_val=SUCCESS;
	_u32 readsize=0;
	static char file_name[MAX_FILE_NAME_BUFFER_LEN];

	dt_stop_clear_task_file();

	//if(p_task->_task_info->_have_name==FALSE) return NULL;
	
	CHECK_FILE_OPEN;
	
	em_memset(file_name,0,MAX_FILE_NAME_BUFFER_LEN);

	/* read from file */
	ret_val=em_pread(g_task_file_id, (char *)file_name,p_task->_task_info->_file_name_len, POS_NAME, &readsize);
	sd_assert(ret_val==SUCCESS);
	if((ret_val!=SUCCESS)||(readsize!=p_task->_task_info->_file_name_len))
	{
		return NULL;
	}
	
      EM_LOG_DEBUG("dt_get_task_file_name_from_file :task_id=%u,name=%s",p_task->_task_info->_task_id,file_name); 
	sd_assert(em_strlen(file_name)>0);
	return file_name;
}

 char * dt_get_task_url_from_file(EM_TASK * p_task)
{
	_int32 ret_val=SUCCESS;
	_u32 readsize=0;
	static char url[MAX_URL_LEN];
	
	dt_stop_clear_task_file();

	CHECK_FILE_OPEN;

	//sd_assert(p_task->_task_info->_type == TT_URL);
	em_memset(url,0,MAX_URL_LEN);

	/* read from file */
	ret_val=em_pread(g_task_file_id, (char *)url,p_task->_task_info->_url_len_or_need_dl_num, POS_URL, &readsize);
	sd_assert(ret_val==SUCCESS);
	if((ret_val!=SUCCESS)||(readsize!=p_task->_task_info->_url_len_or_need_dl_num))
	{
		return NULL;
	}
	
      EM_LOG_DEBUG("dt_get_task_url_from_file :task_id=%u:%s",p_task->_task_info->_task_id,url); 
	sd_assert(em_strlen(url)>0);
	return url;
}
 char *  dt_get_task_ref_url_from_file(EM_TASK * p_task)
{
	_int32 ret_val=SUCCESS;
	_u32 readsize=0;
	static char url[MAX_URL_LEN];
	
	if(p_task->_task_info->_have_ref_url==FALSE) return NULL;
	
	dt_stop_clear_task_file();

	CHECK_FILE_OPEN;
	
	em_memset(url,0,MAX_URL_LEN);

	/* read from file */
	ret_val=em_pread(g_task_file_id, (char *)url,p_task->_task_info->_ref_url_len_or_seed_path_len, POS_REF_URL, &readsize);
	sd_assert(ret_val==SUCCESS);
	if((ret_val!=SUCCESS)||(readsize!=p_task->_task_info->_ref_url_len_or_seed_path_len))
	{
		return NULL;
	}
	
      EM_LOG_DEBUG("dt_get_task_ref_url_from_file :task_id=%u",p_task->_task_info->_task_id); 
	sd_assert(em_strlen(url)>0);
	return url;
}

_u8 *  dt_get_task_tcid_from_file(EM_TASK * p_task)
{
	_int32 ret_val=SUCCESS;
	_u32 readsize=0;
	static _u8 tcid[CID_SIZE];
	
	if(p_task->_task_info->_have_tcid==FALSE) return NULL;
	
	dt_stop_clear_task_file();

	CHECK_FILE_OPEN;
	
	//sd_assert(p_task->_task_info->_type == TT_KANKAN);
	sd_assert(p_task->_task_info->_have_tcid!= FALSE);
	em_memset(tcid,0,CID_SIZE);

	/* read from file */
	ret_val=em_pread(g_task_file_id, (char *)tcid,CID_SIZE, POS_TCID, &readsize);
	sd_assert(ret_val==SUCCESS);
	if((ret_val!=SUCCESS)||(readsize!=CID_SIZE))
	{
		return NULL;
	}
	
      EM_LOG_DEBUG("dt_get_task_tcid_from_file :task_id=%u",p_task->_task_info->_task_id); 
	  sd_assert(em_is_cid_valid(tcid)==TRUE);
	return tcid;
}

_int32  dt_get_p2sp_task_user_data_from_file(EM_TASK * p_task,void* data_buffer,_u32 * buffer_len)
{
	_int32 ret_val=SUCCESS;
	_u32 readsize=0;
	
	if(p_task->_task_info->_have_user_data==FALSE) return INVALID_USER_DATA;
	
	dt_stop_clear_task_file();

	CHECK_FILE_OPEN_INT;
	
	sd_assert(p_task->_task_info->_type != TT_BT);
	sd_assert(p_task->_task_info->_have_user_data!= FALSE);
	sd_assert(p_task->_task_info->_user_data_len <= *buffer_len);

	/* read from file */
	ret_val=em_pread(g_task_file_id, (char *)data_buffer,p_task->_task_info->_user_data_len, POS_P2SP_USER_DATA, &readsize);
	CHECK_VALUE(ret_val);
	if(readsize!=p_task->_task_info->_user_data_len)
	{
		CHECK_VALUE(INVALID_READ_SIZE);
	}
	
      EM_LOG_DEBUG("dt_get_task_ref_url_from_file :task_id=%u",p_task->_task_info->_task_id); 
	*buffer_len =p_task->_task_info->_user_data_len;
	return SUCCESS;
}
_int32  dt_save_p2sp_task_user_data_to_file(EM_TASK * p_task,_u8 * new_user_data,_u32  new_user_data_len)
{
	_int32 ret_val=SUCCESS;
	_u32 readsize=0;
	EM_P2SP_TASK p2sp_task;
	EM_TASK task_tmp;
	char path_buffer[MAX_FILE_PATH_LEN],name_buffer[MAX_FILE_NAME_BUFFER_LEN],url_buffer[MAX_URL_LEN],ref_url_buffer[MAX_URL_LEN];

	dt_stop_clear_task_file();

	CHECK_FILE_OPEN_INT;
		
	if(p_task->_task_info->_full_info )
	{
		ret_val = dt_disable_task_in_file(p_task);
		CHECK_VALUE(ret_val);
		
		p_task->_offset = 0;
		/* write to file */
		ret_val = dt_add_task_to_file(p_task);
		//CHECK_VALUE(ret_val);
	}
	else
	{

		em_memset(&p2sp_task,0,sizeof(EM_P2SP_TASK));
		em_memset(&task_tmp,0,sizeof(EM_TASK));

		task_tmp._task_info = (TASK_INFO*)&p2sp_task;
		em_memcpy(task_tmp._task_info,p_task->_task_info,sizeof(TASK_INFO));

		task_tmp._task_info->_have_user_data= TRUE;
		task_tmp._task_info->_user_data_len = new_user_data_len;
		task_tmp._task_info->_full_info = TRUE;
		p2sp_task._user_data= new_user_data;
	
		/* read file_path from file */
		readsize = 0;
		em_memset(path_buffer,0,MAX_FILE_PATH_LEN);
		ret_val=em_pread(g_task_file_id, (char *)path_buffer,p_task->_task_info->_file_path_len, POS_PATH, &readsize);
		CHECK_VALUE(ret_val);
		if(readsize!=p_task->_task_info->_file_path_len)
		{
			CHECK_VALUE(INVALID_READ_SIZE);
		}
		p2sp_task._file_path = path_buffer;

		/* read file_name from file */
		readsize = 0;
		em_memset(name_buffer,0,MAX_FILE_NAME_BUFFER_LEN);
		ret_val=em_pread(g_task_file_id, (char *)name_buffer,p_task->_task_info->_file_name_len, POS_NAME, &readsize);
		CHECK_VALUE(ret_val);
		if(readsize!=p_task->_task_info->_file_name_len)
		{
			CHECK_VALUE(INVALID_READ_SIZE);
		}
		p2sp_task._file_name= name_buffer;

		if(p_task->_task_info->_url_len_or_need_dl_num!=0)
		{
			/* read url from file */
			readsize = 0;
			em_memset(url_buffer,0,MAX_URL_LEN);
			ret_val=em_pread(g_task_file_id, (char *)url_buffer,p_task->_task_info->_url_len_or_need_dl_num, POS_URL, &readsize);
			CHECK_VALUE(ret_val);
			if(readsize!=p_task->_task_info->_url_len_or_need_dl_num)
			{
				CHECK_VALUE(INVALID_READ_SIZE);
			}
			p2sp_task._url= url_buffer;
		}

		if(p_task->_task_info->_ref_url_len_or_seed_path_len!=0)
		{
			/* read ref_url from file */
			readsize = 0;
			em_memset(ref_url_buffer,0,MAX_URL_LEN);
			ret_val=em_pread(g_task_file_id, (char *)ref_url_buffer,p_task->_task_info->_ref_url_len_or_seed_path_len, POS_REF_URL, &readsize);
			CHECK_VALUE(ret_val);
			if(readsize!=p_task->_task_info->_ref_url_len_or_seed_path_len)
			{
				CHECK_VALUE(INVALID_READ_SIZE);
			}
			p2sp_task._ref_url= ref_url_buffer;
		}
			
	
		if(p_task->_task_info->_have_tcid!=FALSE)
		{
			/* read tcid from file */
			readsize = 0;
			em_memset(&p2sp_task._tcid,0,CID_SIZE);
			ret_val=em_pread(g_task_file_id, (char *)&p2sp_task._tcid,CID_SIZE, POS_TCID, &readsize);
			if(ret_val!=SUCCESS)
			{
				CHECK_VALUE(ret_val);
			}

			if(readsize!=CID_SIZE)
			{
				CHECK_VALUE(INVALID_READ_SIZE);
			}
		}
		
		
		/* write to file */
		ret_val = dt_add_task_to_file(&task_tmp);
		CHECK_VALUE(ret_val);

		ret_val = dt_disable_task_in_file(p_task);
		sd_assert(ret_val==SUCCESS);
		if(ret_val==SUCCESS)
		{
			p_task->_offset = task_tmp._offset;
			p_task->_task_info->_have_user_data = TRUE;
			p_task->_task_info->_user_data_len = new_user_data_len;
		}

	}
		
	return ret_val;
}
_int32  dt_get_bt_task_user_data_from_file(EM_TASK * p_task,void* data_buffer,_u32 * buffer_len)
{
	_int32 ret_val=SUCCESS;
	_u32 readsize=0;
	
	if(p_task->_task_info->_have_user_data==FALSE) return INVALID_USER_DATA;
	
	dt_stop_clear_task_file();

	CHECK_FILE_OPEN_INT;
	
	sd_assert(p_task->_task_info->_type == TT_BT);
	sd_assert(p_task->_task_info->_have_user_data!= FALSE);
	sd_assert(p_task->_task_info->_user_data_len <= *buffer_len);

	/* read from file */
	ret_val=em_pread(g_task_file_id, (char *)data_buffer,p_task->_task_info->_user_data_len, POS_BT_USER_DATA, &readsize);
	CHECK_VALUE(ret_val);
	if(readsize!=p_task->_task_info->_user_data_len)
	{
		CHECK_VALUE(INVALID_READ_SIZE);
	}
	
      EM_LOG_DEBUG("dt_get_task_ref_url_from_file :task_id=%u",p_task->_task_info->_task_id); 
	*buffer_len =p_task->_task_info->_user_data_len;
	return SUCCESS;
}
_int32 dt_save_bt_task_user_data_to_file(EM_TASK * p_task,_u8 * new_user_data,_u32  new_user_data_len)
{
	_int32 ret_val=SUCCESS;
	_u32 readsize=0;
	EM_BT_TASK bt_task,*p_bt_task=NULL;
	EM_TASK task_tmp;
	char path_buffer[MAX_FILE_PATH_LEN],name_buffer[MAX_FILE_NAME_BUFFER_LEN],seed_path_buffer[MAX_FULL_PATH_BUFFER_LEN];

	dt_stop_clear_task_file();

	CHECK_FILE_OPEN_INT;

	if(p_task->_task_info->_full_info)
	{
		p_bt_task=(EM_BT_TASK *)p_task->_task_info;
		
		ret_val = dt_disable_task_in_file(p_task);
		CHECK_VALUE(ret_val);

		p_task->_offset = 0;
		/* write to file */
		ret_val = dt_add_task_to_file(p_task);
		//CHECK_VALUE(ret_val);
	}
	else
	{
		em_memset(&bt_task,0,sizeof(EM_BT_TASK));
		em_memset(&task_tmp,0,sizeof(EM_TASK));

		task_tmp._task_info = (TASK_INFO*)&bt_task;
		em_memcpy(task_tmp._task_info,p_task->_task_info,sizeof(TASK_INFO));

		task_tmp._task_info->_have_user_data = TRUE;
		task_tmp._task_info->_user_data_len = new_user_data_len;
		task_tmp._task_info->_full_info = TRUE;
		bt_task._user_data = new_user_data;
		
		/* read file_path from file */
		readsize = 0;
		em_memset(path_buffer,0,MAX_FILE_PATH_LEN);
		ret_val=em_pread(g_task_file_id, (char *)path_buffer,p_task->_task_info->_file_path_len, POS_PATH, &readsize);
		CHECK_VALUE(ret_val);
		if(readsize!=p_task->_task_info->_file_path_len)
		{
			CHECK_VALUE(INVALID_READ_SIZE);
		}
		bt_task._file_path = path_buffer;

		/* read file_name from file */
		readsize = 0;
		em_memset(name_buffer,0,MAX_FILE_NAME_BUFFER_LEN);
		ret_val=em_pread(g_task_file_id, (char *)name_buffer,p_task->_task_info->_file_name_len, POS_NAME, &readsize);
		CHECK_VALUE(ret_val);
		if(readsize!=p_task->_task_info->_file_name_len)
		{
			CHECK_VALUE(INVALID_READ_SIZE);
		}
		bt_task._file_name= name_buffer;

		/* read seed path from file */
		readsize = 0;
		em_memset(seed_path_buffer,0,MAX_FULL_PATH_BUFFER_LEN);
		ret_val=em_pread(g_task_file_id, (char *)seed_path_buffer,p_task->_task_info->_ref_url_len_or_seed_path_len, POS_SEED_PATH, &readsize);
		CHECK_VALUE(ret_val);
		if(readsize!=p_task->_task_info->_ref_url_len_or_seed_path_len)
		{
			CHECK_VALUE(INVALID_READ_SIZE);
		}
		bt_task._seed_file_path= seed_path_buffer;
		
		bt_task._dl_file_index_array = dt_get_task_bt_need_dl_file_index_array(p_task);
		if(bt_task._dl_file_index_array==NULL)
		{
			CHECK_VALUE(INVALID_FILE_INDEX_ARRAY);
		}
		
		/* read bt file array from file */
		ret_val = em_malloc(p_task->_task_info->_url_len_or_need_dl_num*sizeof(BT_FILE),(void**)&bt_task._file_array);
		if(ret_val!=SUCCESS)
		{
			dt_release_task_bt_need_dl_file_index_array(bt_task._dl_file_index_array);
			CHECK_VALUE(ret_val);
		}
		
		readsize = 0;
		//sd_memset(seed_path_buffer,0,MAX_FULL_PATH_BUFFER_LEN);
		ret_val=em_pread(g_task_file_id, (char *)bt_task._file_array,p_task->_task_info->_url_len_or_need_dl_num*sizeof(BT_FILE), POS_FILE_ARRAY, &readsize);
		if(ret_val!=SUCCESS)
		{
			dt_release_task_bt_need_dl_file_index_array(bt_task._dl_file_index_array);
			EM_SAFE_DELETE(bt_task._file_array);
			CHECK_VALUE(ret_val);
		}
		if(readsize!=p_task->_task_info->_url_len_or_need_dl_num*sizeof(BT_FILE))
		{
			dt_release_task_bt_need_dl_file_index_array(bt_task._dl_file_index_array);
			EM_SAFE_DELETE(bt_task._file_array);
			CHECK_VALUE(INVALID_READ_SIZE);
		}
		
		/* write to file */
		ret_val = dt_add_task_to_file(&task_tmp);
		dt_release_task_bt_need_dl_file_index_array(bt_task._dl_file_index_array);
		EM_SAFE_DELETE(bt_task._file_array);
		CHECK_VALUE(ret_val);

		ret_val = dt_disable_task_in_file(p_task);
		sd_assert(ret_val==SUCCESS);
		if(ret_val==SUCCESS)
		{
			p_task->_offset = task_tmp._offset;
			p_task->_task_info->_have_user_data = TRUE;
			p_task->_task_info->_user_data_len = new_user_data_len;
		}

	}
	return ret_val;
}

 char *  dt_get_task_seed_file_from_file(EM_TASK * p_task)
{
	_int32 ret_val=SUCCESS;
	_u32 readsize=0;
	static char seed_file[MAX_FULL_PATH_BUFFER_LEN];
	
	dt_stop_clear_task_file();

	CHECK_FILE_OPEN;
	
	em_memset(seed_file,0,MAX_FULL_PATH_BUFFER_LEN);

	/* read from file */
	ret_val=em_pread(g_task_file_id, (char *)seed_file,p_task->_task_info->_ref_url_len_or_seed_path_len, POS_SEED_PATH, &readsize);
	sd_assert(ret_val==SUCCESS);
	if((ret_val!=SUCCESS)||(readsize!=p_task->_task_info->_ref_url_len_or_seed_path_len))
	{
		return NULL;
	}
	
      EM_LOG_DEBUG("dt_get_task_seed_file_from_file :task_id=%u",p_task->_task_info->_task_id); 
	sd_assert(em_strlen(seed_file)>0);
	return seed_file;
}
 
/*功能:从文件中读取bt子文件的序号列表
参数: EM_TASK
返回: _u16的数组指针，函数内部分配空间，外部调用者负责释放
*/
_u16 *  dt_get_task_bt_need_dl_file_index_array(EM_TASK * p_task)
{
	_int32 ret_val=SUCCESS;
	_u32 readsize=0;
	_u16 * index_array=NULL;
	
	dt_stop_clear_task_file();

	CHECK_FILE_OPEN;

	ret_val=em_malloc(p_task->_task_info->_url_len_or_need_dl_num*sizeof(_u16), (void**)&index_array);
	if(ret_val!=SUCCESS)
	{
		return NULL;
	}
	
	em_memset(index_array, 0, p_task->_task_info->_url_len_or_need_dl_num*sizeof(_u16));

	/* read from file */
	ret_val=em_pread(g_task_file_id, (char *)index_array,p_task->_task_info->_url_len_or_need_dl_num*sizeof(_u16), POS_INDEX_ARRAY, &readsize);
	sd_assert(ret_val==SUCCESS);
	if((ret_val!=SUCCESS)||(readsize!=p_task->_task_info->_url_len_or_need_dl_num*sizeof(_u16)))
	{
		EM_SAFE_DELETE(index_array);
		return NULL;
	}
	
      EM_LOG_DEBUG("dt_get_task_bt_need_dl_file_index_array :task_id=%u",p_task->_task_info->_task_id); 
	sd_assert(index_array!=NULL);
	return index_array;
}
_int32  dt_release_task_bt_need_dl_file_index_array(_u16 * file_index_array)
{
	EM_SAFE_DELETE(file_index_array);
	
	return SUCCESS;
}

BT_FILE *  dt_get_task_bt_sub_file_from_file(EM_TASK * p_task,_u16 array_index)
{
	static BT_FILE bt_file;
	_int32 ret_val=SUCCESS;
	_u32 readsize=0;
	
	dt_stop_clear_task_file();

	CHECK_FILE_OPEN;
	
	em_memset(&bt_file, 0, sizeof(BT_FILE));
	bt_file._file_index=MAX_FILE_INDEX;
	
	/* read from file */
	ret_val=em_pread(g_task_file_id, (char *)&bt_file,sizeof(BT_FILE), POS_FILE(array_index), &readsize);
	sd_assert(ret_val==SUCCESS);
	if((ret_val!=SUCCESS)||(readsize!=sizeof(BT_FILE)))
	{
		return NULL;
	}
	
      EM_LOG_DEBUG("dt_get_task_bt_sub_file_from_file :task_id=%u",p_task->_task_info->_task_id); 
	sd_assert(bt_file._file_index!=MAX_FILE_INDEX);
	return &bt_file;
}

_int32  dt_set_task_bt_sub_file_to_file(EM_TASK * p_task,_u16 array_index,BT_FILE * p_bt_file)
{
	_int32 ret_val=SUCCESS;
	_u32 writesize=0;
#ifdef STRONG_FILE_CHECK		
	_u32 readsize=0;
	DT_TASK_STORE task_store;
#endif /* STRONG_FILE_CHECK */		
	
	dt_stop_clear_task_file();

	CHECK_FILE_OPEN_INT;
	
		if(p_task->_offset<POS_TASK_START)
		{
	      		EM_LOG_ERROR("dt_set_task_bt_sub_file_to_file:ERROR:p_task->_offset(%u)<POS_TASK_START(%u)",p_task->_offset,POS_TASK_START); 
			sd_assert(FALSE);
			return INVALID_TASK;
		}
#ifdef STRONG_FILE_CHECK		
		/* read from file */
		/* DT_TASK_STORE */
		ret_val=em_pread(g_task_file_id, (char *)&task_store,sizeof(DT_TASK_STORE), p_task->_offset, &readsize);
		if((ret_val!=SUCCESS)||(readsize!=sizeof(DT_TASK_STORE)))
		{
	      		EM_LOG_ERROR("dt_set_task_bt_sub_file_to_file:ERROR1:ret_val=%d,readsize(%u)<sizeof(DT_TASK_STORE)(%u)",ret_val,readsize,sizeof(DT_TASK_STORE)); 
			sd_assert(FALSE);
			return INVALID_READ_SIZE;
		}
		
		if(task_store._valid==0)
		{
	      		EM_LOG_ERROR("dt_set_task_bt_sub_file_to_file:ERROR2:task_store._valid==0"); 
			sd_assert(FALSE);
			return INVALID_TASK;
		}
		
		if(task_store._task_info._task_id!=p_task->_task_info->_task_id)
		{
	      		EM_LOG_ERROR("dt_set_task_bt_sub_file_to_file:ERROR3:task_store._task_info._task_id(%u)!=p_task->_task_id(%u)",task_store._task_info._task_id,p_task->_task_info->_task_id); 
			sd_assert(FALSE);
			return INVALID_TASK;
		}
#endif /* STRONG_FILE_CHECK */		
	/* write to file */
	ret_val = em_pwrite(g_task_file_id, (char *)p_bt_file, sizeof(BT_FILE), POS_FILE(array_index) , &writesize);
	CHECK_VALUE(ret_val);
	if(writesize!=sizeof(BT_FILE))
	{
		CHECK_VALUE( INVALID_WRITE_SIZE);
	}
	
      EM_LOG_DEBUG("dt_set_task_bt_sub_file_to_file :task_id=%u",p_task->_task_info->_task_id); 
	return SUCCESS;
}
_int32 dt_save_bt_task_need_dl_file_change_to_file(EM_TASK * p_task,_u16 * need_dl_file_index_array,_u16 need_dl_file_num)
{
	_int32 ret_val=SUCCESS;
	_u32 readsize=0;
	EM_BT_TASK bt_task;
	EM_TASK task_tmp;
	BT_FILE * p_bt_file=NULL;
	char path_buffer[MAX_FILE_PATH_LEN],name_buffer[MAX_FILE_NAME_BUFFER_LEN],seed_path_buffer[MAX_FULL_PATH_BUFFER_LEN];
	void * user_data= NULL;
	//_u16 * old_dl_file_index_array=NULL;
	_u16 i=0,j=0;

	dt_stop_clear_task_file();

	CHECK_FILE_OPEN_INT;

	if(p_task->_task_info->_full_info != FALSE)
	{
		ret_val = dt_disable_task_in_file(p_task);
		CHECK_VALUE(ret_val);

		p_task->_offset = 0;
		/* write to file */
		ret_val = dt_add_task_to_file(p_task);
		//CHECK_VALUE(ret_val);
	}
	else
	{
		em_memset(&bt_task,0,sizeof(EM_BT_TASK));
		em_memset(&task_tmp,0,sizeof(EM_TASK));

		task_tmp._task_info = (TASK_INFO*)&bt_task;
		em_memcpy(task_tmp._task_info,p_task->_task_info,sizeof(TASK_INFO));

		task_tmp._task_info->_full_info = TRUE;
		
		/* read file_path from file */
		readsize = 0;
		em_memset(path_buffer,0,MAX_FILE_PATH_LEN);
		ret_val=em_pread(g_task_file_id, (char *)path_buffer,p_task->_task_info->_file_path_len, POS_PATH, &readsize);
		CHECK_VALUE(ret_val);
		if(readsize!=p_task->_task_info->_file_path_len)
		{
			CHECK_VALUE(INVALID_READ_SIZE);
		}
		bt_task._file_path = path_buffer;

		/* read file_name from file */
		readsize = 0;
		em_memset(name_buffer,0,MAX_FILE_NAME_BUFFER_LEN);
		ret_val=em_pread(g_task_file_id, (char *)name_buffer,p_task->_task_info->_file_name_len, POS_NAME, &readsize);
		CHECK_VALUE(ret_val);
		if(readsize!=p_task->_task_info->_file_name_len)
		{
			CHECK_VALUE(INVALID_READ_SIZE);
		}
		bt_task._file_name= name_buffer;

		/* read seed path from file */
		readsize = 0;
		em_memset(seed_path_buffer,0,MAX_FULL_PATH_BUFFER_LEN);
		ret_val=em_pread(g_task_file_id, (char *)seed_path_buffer,p_task->_task_info->_ref_url_len_or_seed_path_len, POS_SEED_PATH, &readsize);
		CHECK_VALUE(ret_val);
		if(readsize!=p_task->_task_info->_ref_url_len_or_seed_path_len)
		{
			CHECK_VALUE(INVALID_READ_SIZE);
		}
		bt_task._seed_file_path= seed_path_buffer;
		
		if(p_task->_task_info->_have_user_data!=FALSE)
		{
			/* read user_data from file */
			ret_val = em_malloc(p_task->_task_info->_user_data_len,(void**)&user_data);
			CHECK_VALUE(ret_val);

			readsize = 0;
			//sd_memset(user_data,0,p_task->_task_info->_user_data_len);
			ret_val=em_pread(g_task_file_id, (char *)user_data,p_task->_task_info->_user_data_len, POS_BT_USER_DATA, &readsize);
			if(ret_val!=SUCCESS) 
			{
				EM_SAFE_DELETE(user_data);
				CHECK_VALUE(ret_val);
			}

			if(readsize!=p_task->_task_info->_user_data_len)
			{
				EM_SAFE_DELETE(user_data);
				CHECK_VALUE(INVALID_READ_SIZE);
			}
			bt_task._user_data= user_data;
		}

		
		bt_task._dl_file_index_array = dt_get_task_bt_need_dl_file_index_array(p_task);
		if(bt_task._dl_file_index_array==NULL)
		{
			EM_SAFE_DELETE(user_data);
			CHECK_VALUE(INVALID_FILE_INDEX_ARRAY);
		}

		/* bt file array  */
		ret_val = em_malloc(need_dl_file_num*sizeof(BT_FILE),(void**)&bt_task._file_array);
		if(ret_val!=SUCCESS)
		{
			EM_SAFE_DELETE(user_data);
			dt_release_task_bt_need_dl_file_index_array(bt_task._dl_file_index_array);
			CHECK_VALUE(ret_val);
		}
		em_memset(bt_task._file_array,0,need_dl_file_num*(sizeof(BT_FILE)));
		for(i=0;i<need_dl_file_num;i++)
		{
			bt_task._file_array[i]._file_index = need_dl_file_index_array[i];
			for(j=0;j<task_tmp._task_info->_url_len_or_need_dl_num;j++)
			{
				if(bt_task._file_array[i]._file_index==bt_task._dl_file_index_array[j])
				{
					p_bt_file = dt_get_task_bt_sub_file_from_file(p_task,j);
					if(p_bt_file!=NULL)
					{
						em_memcpy(&bt_task._file_array[i],p_bt_file,sizeof(BT_FILE));
					}
				}
			}
		}
		
		dt_release_task_bt_need_dl_file_index_array(bt_task._dl_file_index_array);
		
		task_tmp._task_info->_url_len_or_need_dl_num= need_dl_file_num;
		bt_task._dl_file_index_array= need_dl_file_index_array;
		/* write to file */
		ret_val = dt_add_task_to_file(&task_tmp);
		EM_SAFE_DELETE(user_data);
		EM_SAFE_DELETE(bt_task._file_array);
		CHECK_VALUE(ret_val);

		ret_val = dt_disable_task_in_file(p_task);
		sd_assert(ret_val==SUCCESS);
		if(ret_val==SUCCESS)
		{
			p_task->_offset = task_tmp._offset;
			p_task->_task_info->_url_len_or_need_dl_num = need_dl_file_num;
		}
	}
	return ret_val;
}

_int32  dt_check_task_file(_u32 task_num)
{
#ifdef STRONG_FILE_CHECK
	TASK_INFO task_info;
	_u32 valid_task_count = 0,invalid_task_count=0;
	_int32 ret_val=SUCCESS;
	_u32 readsize=0,pos_task_file_end=POS_TASK_START;
	DT_TASK_STORE_HEAD dt_task_store_head;
	EM_TASK * p_task = NULL;
	_u64 filesize=0;
	static _u32 check_count = 0;

      EM_LOG_DEBUG("dt_check_task_file:g_task_file_id=%u,task_num=%u,g_pos_task_file_end=%u,POS_TASK_START=%u,check_count=%u",g_task_file_id,task_num,g_pos_task_file_end,POS_TASK_START,check_count); 

	if(check_count++>=MAX_FILE_CHECK_COUNT)
  	{
  		check_count = 0;
	}
	else
	{
		return SUCCESS;
	}
	
	dt_stop_clear_task_file();
	
	CHECK_FILE_OPEN_INT;

	ret_val = em_filesize(g_task_file_id, &filesize);
	CHECK_VALUE(ret_val);
	
while(TRUE)
{
	sd_assert(pos_task_file_end>=POS_TASK_START);
	if(pos_task_file_end>=filesize-1)
	{
		if(task_num!=valid_task_count)
		{
	      		EM_LOG_ERROR("dt_check_task_file:ERROR:task_num(%u)!=valid_task_count(%u)",task_num,valid_task_count); 
			CHECK_VALUE(INVALID_TASK);
		}
      		EM_LOG_DEBUG("dt_check_task_file end:pos_task_file_end=%u,filesize=%llu,valid_task_count=%u,invalid_task_count=%u",pos_task_file_end,filesize,valid_task_count,invalid_task_count); 
		return SUCCESS;
	}
	
	em_memset(&dt_task_store_head,0,sizeof(DT_TASK_STORE_HEAD));
	/* read from file */
	readsize=0;
	ret_val=em_pread(g_task_file_id, (char *)&dt_task_store_head,sizeof(DT_TASK_STORE_HEAD), pos_task_file_end, &readsize);
	//sd_assert(ret_val==SUCCESS);
	if((ret_val!=SUCCESS)||(readsize!=sizeof(DT_TASK_STORE_HEAD)))
	{
      		EM_LOG_ERROR("dt_check_task_file:ERROR0:ret_val=%d,readsize=%u,sizeof(DT_TASK_STORE_HEAD)=%d,invalid_task_count=%d,pos_task_file_end=%u",ret_val,readsize,sizeof(DT_TASK_STORE_HEAD),invalid_task_count,pos_task_file_end); 
		CHECK_VALUE(INVALID_READ_SIZE);
	}

	if(dt_task_store_head._len>=MAX_P2SP_TASK_FULL_INFO_LEN)
	{
		/* 读到一个长度不正常的任务信息，该文件已经被破坏! */
      		EM_LOG_ERROR("dt_check_task_file:get a big task!dt_task_store_head._len=%u,invalid_task_count=%d,g_pos_task_file_end=%u",dt_task_store_head._len,invalid_task_count,pos_task_file_end); 
		CHECK_VALUE(INVALID_TASK_INFO);
	}

	if(dt_task_store_head._valid==0)
	{
		pos_task_file_end+=readsize;
		pos_task_file_end+=dt_task_store_head._len;
		invalid_task_count++;
		continue;
	}
	else
	if(dt_task_store_head._valid!=1)
	{
		/* 读到一个不正常的任务信息，该文件已经被破坏! */
      		EM_LOG_ERROR("dt_check_task_file:get a bad task!dt_task_store_head._len=%u,invalid_task_count=%d,g_pos_task_file_end=%u",dt_task_store_head._len,invalid_task_count,pos_task_file_end); 
		CHECK_VALUE(INVALID_TASK_INFO);
	}
	
	/* read from file */
	readsize=0;
	em_memset(&task_info,0,sizeof(TASK_INFO));
	ret_val=em_pread(g_task_file_id, (char *)&task_info,sizeof(TASK_INFO), pos_task_file_end+sizeof(DT_TASK_STORE_HEAD), &readsize);
	//sd_assert(ret_val==SUCCESS);
	if((ret_val!=SUCCESS)||(readsize!=sizeof(TASK_INFO)))
	{
      		EM_LOG_ERROR("dt_check_task_file:ERROR1:ret_val=%d,readsize=%u,sizeof(TASK_INFO)=%d,invalid_task_count=%d,pos_task_file_end=%u",ret_val,readsize,sizeof(TASK_INFO),invalid_task_count,pos_task_file_end); 
		CHECK_VALUE(INVALID_READ_SIZE);
	}

	p_task = dt_get_task_from_map(task_info._task_id);
	if(p_task==NULL) 
	{
      		EM_LOG_ERROR("dt_check_task_file:ERROR2:task_info._task_id=%u can not find in the map!",task_info._task_id); 
		sd_assert(FALSE);
		return INVALID_TASK_ID;
	}
	
	if(p_task->_offset != pos_task_file_end)
	{
      		EM_LOG_ERROR("dt_check_task_file:ERROR3:task_info._task_id=%u ,p_task->_offset(%u) != pos_task_file_end(%u)!",task_info._task_id,p_task->_offset ,pos_task_file_end); 
		CHECK_VALUE(INVALID_TASK);
	}
		
      EM_LOG_DEBUG("dt_check_task_file ok:task_id=%u",task_info._task_id); 
	valid_task_count++;

	pos_task_file_end+=sizeof(DT_TASK_STORE_HEAD);
	pos_task_file_end+=dt_task_store_head._len;

}
#endif /* STRONG_FILE_CHECK */	
	return SUCCESS;
}



