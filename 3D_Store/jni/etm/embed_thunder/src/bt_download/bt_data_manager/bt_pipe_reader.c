#include "utility/define.h"
#ifdef ENABLE_BT 


#include "../bt_task/bt_task_interface.h"
#include "connect_manager/pipe_interface.h"
#include "bt_pipe_reader.h"
#include "bt_data_manager_interface.h"
#include "bt_data_manager.h"
#include "bt_data_manager_imp.h"
#include "utility/mempool.h"

#include "utility/logid.h"
#define LOGID LOGID_BT_DOWNLOAD
#include "utility/logger.h"

//#ifdef UPLOAD_DEBUG
//#include "platform/sd_fs.h"
//#endif

static SLAB* g_bt_pipe_reader_mgr_slab = NULL;
//static SLAB* g_bt_pipe_reader_slab = NULL;

_int32 init_bpr( void )
{
	_int32 ret_val = SUCCESS;

	LOG_DEBUG( " init_bpr");

	if( g_bt_pipe_reader_mgr_slab == NULL )
	{              
		ret_val = mpool_create_slab( sizeof( BT_PIPE_READER_MANAGER ), MAX_NUM_OF_PIPE_READER, 0, &g_bt_pipe_reader_mgr_slab );
		CHECK_VALUE( ret_val );
	}
	/*
	if( g_bt_pipe_reader_slab == NULL )
	{              
		ret_val = mpool_create_slab( sizeof( BT_PIPE_READER ), MAX_NUM_OF_PIPE_READER, 0, &g_bt_pipe_reader_slab );
		CHECK_VALUE( ret_val );
	}
	*/
	return SUCCESS;
}

_int32 uninit_bpr( void )
{
	_int32 ret_val = SUCCESS;

	LOG_DEBUG( " uninit_bpr");

	if( g_bt_pipe_reader_mgr_slab != NULL )
	{              
		ret_val = mpool_destory_slab( g_bt_pipe_reader_mgr_slab );
		CHECK_VALUE( ret_val );
		g_bt_pipe_reader_mgr_slab = NULL;
	}
/*
	if( g_bt_pipe_reader_slab != NULL )
	{              
		ret_val = mpool_destory_slab( g_bt_pipe_reader_slab );
		CHECK_VALUE( ret_val );
		g_bt_pipe_reader_slab = NULL;
	}
*/
	 return SUCCESS;
}
_int32 bpr_pipe_reader_mgr_malloc_wrap(BT_PIPE_READER_MANAGER **pp_node)
{
	return mpool_get_slip( g_bt_pipe_reader_mgr_slab, (void**)pp_node );
}

_int32 bpr_pipe_reader_mgr_free_wrap(BT_PIPE_READER_MANAGER *p_node)
{
	sd_assert( p_node != NULL );
	if( p_node == NULL ) return SUCCESS;
	return mpool_free_slip( g_bt_pipe_reader_mgr_slab, (void*)p_node );
}
/*
_int32 bpr_pipe_reader_malloc_wrap(BT_PIPE_READER **pp_node)
{
	return mpool_get_slip( g_bt_pipe_reader_slab, (void**)pp_node );
}

_int32 bpr_pipe_reader_free_wrap(BT_PIPE_READER *p_node)
{
	sd_assert( p_node != NULL );
	if( p_node == NULL ) return SUCCESS;
	return mpool_free_slip( g_bt_pipe_reader_slab, (void*)p_node );
}
*/
_int32 bprm_init_list( LIST *p_pipe_reader_mgr_list )
{
	LOG_DEBUG( " bprm_init_list");

	 list_init(p_pipe_reader_mgr_list);
	 return SUCCESS;
}
_int32 bprm_uninit_list( LIST *p_pipe_reader_mgr_list )
{
	BT_PIPE_READER_MANAGER *p_pipe_reader_mgr=NULL;
    	LIST_ITERATOR  cur_node = NULL;
	LOG_DEBUG( " bprm_uninit_list=%u",list_size( p_pipe_reader_mgr_list));

	while(0!=list_size( p_pipe_reader_mgr_list))
	{
	    	cur_node = LIST_BEGIN( *p_pipe_reader_mgr_list ) ; 
		sd_assert(cur_node != LIST_END(*p_pipe_reader_mgr_list ));
	       {
		        p_pipe_reader_mgr = (BT_PIPE_READER_MANAGER *)LIST_VALUE( cur_node );
				
			 if(p_pipe_reader_mgr->is_clear!=TRUE)
			 	p_pipe_reader_mgr->is_clear=TRUE;
			 
			 bprm_finished(p_pipe_reader_mgr, -1, 0 );
	       }
	}

	return SUCCESS;
}


_int32 bprm_init_struct( BT_PIPE_READER_MANAGER *p_pipe_reader_mgr ,struct tagBT_DATA_MANAGER *p_bt_data_mgr,const BT_RANGE *p_bt_range, char *p_range_data_buffer,_u32 total_data_len,_u32 data_len,
	void* p_call_back, void *p_user,LIST * read_range_info_list_for_file)
{
	//_int32 ret_val = SUCCESS;

	LOG_DEBUG( " bprm_init_struct");

	sd_memset(p_pipe_reader_mgr,0,sizeof(BT_PIPE_READER_MANAGER));
	
	//list_init(&(p_pipe_reader_mgr->_bt_data_reader_list));
	p_pipe_reader_mgr->_bt_data_mgr_ptr=p_bt_data_mgr;
	p_pipe_reader_mgr->_range_data_buffer=p_range_data_buffer;
	p_pipe_reader_mgr->_total_data_len=total_data_len;
	p_pipe_reader_mgr->_data_len=data_len;
	p_pipe_reader_mgr->_p_call_back=p_call_back;
	p_pipe_reader_mgr->_p_user=p_user;
	p_pipe_reader_mgr->_bt_range_ptr=(BT_RANGE *)p_bt_range;
	p_pipe_reader_mgr->read_range_info_list_for_file=read_range_info_list_for_file;
	
	return SUCCESS;

}
_int32 bprm_uninit_struct(BT_PIPE_READER_MANAGER *p_pipe_reader_mgr)
{
	//_int32 ret_val = SUCCESS;
	//LIST_ITERATOR p_list_node=NULL ;
	BT_PIPE_READER * p_pipe_reader=&(p_pipe_reader_mgr->_pipe_reader);
	LIST *read_range_info_list_for_file=p_pipe_reader_mgr->read_range_info_list_for_file;
	READ_RANGE_INFO_FOR_FILE * p_read_range_info_for_file=NULL;
	LIST_ITERATOR p_list_node=NULL;
	
	
	LOG_DEBUG( " bprm_uninit_struct");

	if(p_pipe_reader_mgr->_add_range_timer_id!=0)
	{
		/* Stop timer */
		LOG_DEBUG("cancel_timer(p_pipe_reader_mgr->_add_range_timer_id)");
		cancel_timer(p_pipe_reader_mgr->_add_range_timer_id);
		p_pipe_reader_mgr->_add_range_timer_id=0;
	}
	

	if(0!=list_size( read_range_info_list_for_file))
	{
		for(p_list_node = LIST_BEGIN(*read_range_info_list_for_file); p_list_node != LIST_END(*read_range_info_list_for_file); p_list_node = LIST_NEXT(p_list_node))
		{
			p_read_range_info_for_file = (READ_RANGE_INFO_FOR_FILE * )LIST_VALUE(p_list_node);
			SAFE_DELETE(p_read_range_info_for_file);
		}
		list_clear( read_range_info_list_for_file);
	}	
	SAFE_DELETE(read_range_info_list_for_file);
	p_pipe_reader_mgr->read_range_info_list_for_file=NULL;

	bpr_uninit_struct( p_pipe_reader );
	
	
	return SUCCESS;

}
_int32 bprm_failure_exit(BT_PIPE_READER_MANAGER *p_pipe_reader_mgr ,_int32 err_code )
{
	//_int32 ret_val = SUCCESS;
	//LIST_ITERATOR p_list_node=NULL ;
	//BT_PIPE_READER * p_pipe_reader=NULL;
	_u32 total_read_len=p_pipe_reader_mgr->_receved_data_len+(p_pipe_reader_mgr->_total_data_len-p_pipe_reader_mgr->_data_len);
	
	LOG_DEBUG( " bprm_failure_exit:err_code=%d",err_code);

	bprm_finished(p_pipe_reader_mgr,  err_code, total_read_len );	
	return SUCCESS;

}
_int32 bprm_add_read_range(BT_PIPE_READER_MANAGER *p_pipe_reader_mgr  )
{
	_int32 ret_val = SUCCESS;
	//LIST_ITERATOR p_list_node=NULL ;
	BT_PIPE_READER * p_pipe_reader=&(p_pipe_reader_mgr->_pipe_reader);
	READ_RANGE_INFO_FOR_FILE * p_read_range_info_for_file=NULL;
	BOOL need_timer=FALSE;
	LIST * p_read_range_info_list=p_pipe_reader_mgr->read_range_info_list_for_file;
	//struct tagBT_DATA_MANAGER *p_bt_data_manager = p_pipe_reader_mgr->_bt_data_mgr_ptr;
	
	LOG_DEBUG( " bprm_add_read_range:listsize=%u",list_size( p_read_range_info_list));

	sd_assert(list_size( p_read_range_info_list)!=0);

	ret_val =list_pop(p_read_range_info_list, (void **)&p_read_range_info_for_file);
	CHECK_VALUE(ret_val);
	
	LOG_DEBUG( " bprm_add_read_ranges:_file_index=%u,_padding_range(%u,%u),bt_range(%llu,%llu)",
			p_read_range_info_for_file->_file_index,p_read_range_info_for_file->_padding_range._index,p_read_range_info_for_file->_padding_range._num,p_read_range_info_for_file->bt_range._pos,p_read_range_info_for_file->bt_range._length);
			
	ret_val = bpr_init_struct( p_pipe_reader,p_pipe_reader_mgr,p_read_range_info_for_file->data_buffer,p_read_range_info_for_file->bt_range._length,p_read_range_info_for_file->_file_index,&(p_read_range_info_for_file->_padding_range));
	if(ret_val!=SUCCESS) 
	{
		goto ErrorHanle;
	}

	ret_val = bpr_add_read_bt_range( p_pipe_reader, &(p_read_range_info_for_file->bt_range));
	if(ret_val!=SUCCESS) 
	{
		if((ret_val==BT_RANGE_NOT_IN_DISK)||(ret_val==BT_READ_GET_BUFFER_FAIL))
		{
			need_timer=TRUE;
			ret_val=SUCCESS;
		}
		else
		{
			bpr_uninit_struct( p_pipe_reader );
			goto ErrorHanle;
		}
	}
	
	if(need_timer==TRUE)
	{
		sd_assert(p_pipe_reader_mgr->_add_range_timer_id==0);
		if(p_pipe_reader_mgr->_add_range_timer_id==0)
		{
			/* Start timer */
			LOG_DEBUG("start_timer(bprm_handle_add_range_timeout)");
			ret_val = start_timer(bprm_handle_add_range_timeout, NOTICE_INFINITE,500, 0,(void*)(p_pipe_reader_mgr),&(p_pipe_reader_mgr->_add_range_timer_id));  
			CHECK_VALUE(ret_val);
		}
	}
	
	SAFE_DELETE(p_read_range_info_for_file);
	return SUCCESS;

ErrorHanle:
	SAFE_DELETE(p_read_range_info_for_file);
	return ret_val;

}
/*
_int32 bprm_add_read_ranges(BT_PIPE_READER_MANAGER *p_pipe_reader_mgr ,LIST * p_read_range_info_list )
{
	_int32 ret_val = SUCCESS;
	LIST_ITERATOR p_list_node=NULL ;
	BT_PIPE_READER * p_pipe_reader=NULL;
	READ_RANGE_INFO_FOR_FILE * p_read_range_info_for_file=NULL;
	BOOL need_timer=FALSE;
	_u32 listsize=list_size( p_read_range_info_list);
	struct tagBT_DATA_MANAGER *p_bt_data_manager = p_pipe_reader_mgr->_bt_data_mgr_ptr;
	
	LOG_DEBUG( " bprm_add_read_ranges:listsize=%u",listsize);

	sd_assert(listsize==1);
	
	if(0!=listsize)
	{
		if(listsize>1)
		{
			return -1;
		}
		
		for(p_list_node = LIST_BEGIN(*p_read_range_info_list); p_list_node != LIST_END(*p_read_range_info_list); p_list_node = LIST_NEXT(p_list_node))
		{
			p_read_range_info_for_file = (READ_RANGE_INFO_FOR_FILE * )LIST_VALUE(p_list_node);

			//init_pos=p_read_range_info->_padding_exact_range._pos;
			//data_len=p_read_range_info->_padding_exact_range._length
			LOG_DEBUG( " bprm_add_read_ranges:_file_index=%u,_padding_range(%u,%u),bt_range(%llu,%llu)",
			p_read_range_info_for_file->_file_index,p_read_range_info_for_file->_padding_range._index,p_read_range_info_for_file->_padding_range._num,p_read_range_info_for_file->bt_range._pos,p_read_range_info_for_file->bt_range._length);
			if(FALSE==bdm_range_is_write_in_disk(p_bt_data_manager,p_read_range_info_for_file->_file_index,&(p_read_range_info_for_file->_padding_range)))
			{
				LOG_DEBUG( " bprm_add_read_ranges:bdm_range_is_write_in_disk return FALSE!");
				ret_val =  -1;
				goto ErrorHanle;
			}
			
			ret_val = bpr_pipe_reader_malloc_wrap(&p_pipe_reader);
			if(ret_val!=SUCCESS) goto ErrorHanle;
			
			ret_val = bpr_init_struct( p_pipe_reader,p_pipe_reader_mgr,p_read_range_info_for_file->data_buffer,p_read_range_info_for_file->bt_range._length);
			if(ret_val!=SUCCESS) 
			{
				bpr_pipe_reader_free_wrap(p_pipe_reader);
				goto ErrorHanle;
			}

			ret_val = bpr_add_read_bt_range( p_pipe_reader, &(p_read_range_info_for_file->bt_range));
			if(ret_val!=SUCCESS) 
			{
				if(ret_val==BT_READ_GET_BUFFER_FAIL)
				{
					need_timer=TRUE;
					ret_val=SUCCESS;
				}
				else
				{
					bpr_uninit_struct( p_pipe_reader );
					bpr_pipe_reader_free_wrap(p_pipe_reader);
					goto ErrorHanle;
				}
			}

			ret_val = list_push(&(p_pipe_reader_mgr->_bt_data_reader_list),(void*)p_pipe_reader);
			if(ret_val!=SUCCESS) 
			{
				bpr_uninit_struct( p_pipe_reader );
				bpr_pipe_reader_free_wrap(p_pipe_reader);
				goto ErrorHanle;
			}

		}
	}
	else
	{
		return -1;
	}
	
	if(need_timer==TRUE)
	{
		sd_assert(p_pipe_reader_mgr->_add_range_timer_id==0);
		if(p_pipe_reader_mgr->_add_range_timer_id==0)
		{
			// Start timer
			LOG_DEBUG("start_timer(bprm_handle_add_range_timeout)");
			ret_val = start_timer(bprm_handle_add_range_timeout, NOTICE_INFINITE,1000, 0,(void*)(p_pipe_reader_mgr),&(p_pipe_reader_mgr->_add_range_timer_id));  
			CHECK_VALUE(ret_val);
		}
	}
	
	return SUCCESS;

ErrorHanle:
	
	return ret_val;
}
*/
_int32 bprm_clear(BT_PIPE_READER_MANAGER *p_pipe_reader_mgr)
{
	
	LOG_DEBUG( " bprm_clear..." );

	p_pipe_reader_mgr->is_clear=TRUE;
	if(p_pipe_reader_mgr->_add_range_timer_id!=0)
	{
		/* Stop timer */
		LOG_DEBUG("cancel_timer(p_pipe_reader_mgr->_add_range_timer_id)");
		cancel_timer(p_pipe_reader_mgr->_add_range_timer_id);
		p_pipe_reader_mgr->_add_range_timer_id=0;
		bprm_finished(p_pipe_reader_mgr, -1, 0 );
	}
	return SUCCESS;

}

_int32 bprm_finished(BT_PIPE_READER_MANAGER *p_pipe_reader_mgr, _int32 read_result, _u32 read_len )
{
	bt_pipe_read_result _p_call_back;
	BT_RANGE *p_bt_range=NULL;
	void *_p_user;
	char* data_buffer=NULL;
    	LIST_ITERATOR  cur_node = NULL,next_node;
	BT_PIPE_READER_MANAGER * p_pipe_reader_mgr_val=NULL;
	struct tagBT_DATA_MANAGER *p_bt_data_manager = p_pipe_reader_mgr->_bt_data_mgr_ptr;
	_int32 ret_val=SUCCESS;
	BOOL is_clear=FALSE;
	
	//char tmp_buffer[MAX_FILE_NAME_LEN];
	//_u32 file_id = 0;
	//_u32 write_size = 0;
	
	LOG_DEBUG( " bprm_finished(result=%d,read_len=%u,is_clear=%d)..." ,read_result,read_len,p_pipe_reader_mgr->is_clear);

		_p_call_back=(bt_pipe_read_result) p_pipe_reader_mgr->_p_call_back;
		_p_user=p_pipe_reader_mgr->_p_user;
		p_bt_range=p_pipe_reader_mgr->_bt_range_ptr;
		data_buffer=p_pipe_reader_mgr->_range_data_buffer;
		is_clear = p_pipe_reader_mgr->is_clear;
		bprm_uninit_struct(p_pipe_reader_mgr  );
	    	for(cur_node = LIST_BEGIN( p_bt_data_manager->_bt_pipe_reader_mgr_list ) ; cur_node != LIST_END( p_bt_data_manager->_bt_pipe_reader_mgr_list );  )
	       {
		        p_pipe_reader_mgr_val = (BT_PIPE_READER_MANAGER *)LIST_VALUE( cur_node );
		        next_node = LIST_NEXT( cur_node );
		        if( p_pipe_reader_mgr_val == p_pipe_reader_mgr )
		        {
		            ret_val = list_erase( &(p_bt_data_manager->_bt_pipe_reader_mgr_list), cur_node );
#ifdef _DEBUG
				CHECK_VALUE(ret_val);
#endif		            
		            break;
		        }
			cur_node = next_node;
	       }
		bpr_pipe_reader_mgr_free_wrap(p_pipe_reader_mgr);
		//typedef _int32 (*bt_pipe_read_result) ( void *p_user, BT_RANGE *p_bt_range, char *p_data_buffer, _int32 read_result, _u32 read_len );
		if(is_clear==TRUE)
		{
			sd_free(data_buffer);
			data_buffer = NULL;
			sd_free(p_bt_range);
			p_bt_range = NULL;
		}
		else
		{
#ifdef UPLOAD_DEBUG
		//sd_snprintf( tmp_buffer, MAX_FILE_NAME_LEN, "%s%llu.test", p_pipe_reader_mgr->_bt_data_mgr_ptr->_data_path, p_bt_range->_pos );

		//sd_open_ex( tmp_buffer, 1, &file_id );
		//sd_write( file_id, data_buffer, read_len, &write_size );
#endif				
			_p_call_back(_p_user,p_bt_range,data_buffer,read_result,read_len);
		}
		
	return SUCCESS;

}

_int32 bprm_read_data_result_handler(BT_PIPE_READER_MANAGER *p_pipe_reader_mgr,BT_PIPE_READER *p_pipe_reader,  _int32 read_result, _u32 read_len )
{
	_int32 ret_val = SUCCESS;
	//LIST_ITERATOR p_list_node=NULL ;
	_u32 total_read_len=0;
	
	LOG_DEBUG( " bprm_read_data_result_handler(result=%d)..." ,read_result);

	sd_assert(&(p_pipe_reader_mgr->_pipe_reader)==p_pipe_reader);
	
	bpr_uninit_struct( p_pipe_reader );
	
	p_pipe_reader_mgr->_receved_data_len+=read_len;
	
	if(read_result!=SUCCESS)
	{
		total_read_len = p_pipe_reader_mgr->_receved_data_len+(p_pipe_reader_mgr->_total_data_len-p_pipe_reader_mgr->_data_len);
		bprm_finished(p_pipe_reader_mgr,  read_result, total_read_len );
	}
	else
	{
		if(0==list_size( p_pipe_reader_mgr->read_range_info_list_for_file))
		{
			sd_assert(p_pipe_reader_mgr->_receved_data_len==p_pipe_reader_mgr->_data_len);
			total_read_len = p_pipe_reader_mgr->_total_data_len;
			bprm_finished(p_pipe_reader_mgr,  read_result, total_read_len );
		}
		else
		{
			if(p_pipe_reader_mgr->is_clear==FALSE)
			{
				ret_val = bprm_add_read_range(p_pipe_reader_mgr );
				if(ret_val!=SUCCESS)
				{
					total_read_len = p_pipe_reader_mgr->_receved_data_len+(p_pipe_reader_mgr->_total_data_len-p_pipe_reader_mgr->_data_len);
					bprm_finished(p_pipe_reader_mgr,  ret_val, total_read_len );
				}
			}
			else
			{
				total_read_len = p_pipe_reader_mgr->_receved_data_len+(p_pipe_reader_mgr->_total_data_len-p_pipe_reader_mgr->_data_len);
				bprm_finished(p_pipe_reader_mgr,  read_result, total_read_len );
			}
		}
	}
	return SUCCESS;

//ErrorHanle:
	
//	CHECK_VALUE( ret_val );
	
//	return ret_val;
}
_int32 bprm_handle_add_range_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid)
{
	_int32 ret_val = SUCCESS;
	//LIST_ITERATOR p_list_node=NULL ;
	BT_PIPE_READER_MANAGER *p_pipe_reader_mgr = (BT_PIPE_READER_MANAGER *)(msg_info->_user_data);
	BT_PIPE_READER *p_pipe_reader=&(p_pipe_reader_mgr->_pipe_reader);
	BOOL need_timer=FALSE;
	 
	LOG_DEBUG("bprm_handle_add_range_timeout:errcode=%d,notice_count_left=%u,expired=%u,msgid=%u",errcode,notice_count_left,expired,msgid);

	if(errcode == MSG_TIMEOUT)
	{
		sd_assert(p_pipe_reader_mgr!=NULL);
		if(p_pipe_reader_mgr==NULL) return INVALID_ARGUMENT;	
		
		if(/*msg_info->_device_id*/msgid == p_pipe_reader_mgr->_add_range_timer_id)
		{
		
			//p_pipe_reader_mgr->_add_range_timer_id=0;
			

					if(p_pipe_reader->is_set_range==FALSE)
					{
						ret_val = bpr_add_read_bt_range( p_pipe_reader, &(p_pipe_reader->_bt_range));
						if(ret_val!=SUCCESS) 
						{
							if((ret_val==BT_RANGE_NOT_IN_DISK)||(ret_val==BT_READ_GET_BUFFER_FAIL))
							{
								need_timer=TRUE;
							}
							else
							{
								goto ErrorHanle;
							}
						}
					}
					else
					{
						sd_assert(FALSE);
					}
			
			if(need_timer==FALSE)
			{
				sd_assert(p_pipe_reader_mgr->_add_range_timer_id!=0);
				if(p_pipe_reader_mgr->_add_range_timer_id!=0)
				{
					/* Stop timer */
					LOG_DEBUG("cancel_timer(p_pipe_reader_mgr->_add_range_timer_id)");
					cancel_timer(p_pipe_reader_mgr->_add_range_timer_id);
					p_pipe_reader_mgr->_add_range_timer_id=0;
				}
			}
		}
		else
		{
			sd_assert(FALSE);
			goto ErrorHanle;
		}
	}
		
	LOG_DEBUG( "bprm_handle_add_range_timeout:SUCCESS" );
	return SUCCESS;

ErrorHanle:

	bprm_failure_exit(p_pipe_reader_mgr , ret_val );
	CHECK_VALUE( ret_val );
	return SUCCESS;


}

////////////////////////////////////////////////////

_int32 bpr_init_struct( BT_PIPE_READER *p_pipe_reader,BT_PIPE_READER_MANAGER *p_bt_data_reader_mgr,char* data_buffer,_u32 data_len,_u32 file_index,RANGE *_padding_range)
{
	_int32 ret_val = SUCCESS;
	BT_DATA_MANAGER *p_bt_data_manager = p_bt_data_reader_mgr->_bt_data_mgr_ptr;
	BT_RANGE_SWITCH *p_bt_range_switch=&(p_bt_data_manager->_bt_range_switch);

	LOG_DEBUG( " bpr_init_struct");

	sd_memset(p_pipe_reader,0,sizeof(BT_PIPE_READER));

	p_pipe_reader->_bt_data_reader_mgr_ptr = p_bt_data_reader_mgr;
	p_pipe_reader->_data_buffer_ptr= data_buffer;
	p_pipe_reader->_buffer_len= data_len;

	p_pipe_reader->_file_index = file_index;
	p_pipe_reader->_padding_range._index=_padding_range->_index;
	p_pipe_reader->_padding_range._num=_padding_range->_num;

	ret_val = bdr_init_struct( &(p_pipe_reader->_bt_data_reader), p_bt_range_switch,bpr_read_data, 
				bpr_add_data, bpr_read_data_result_handler, (void*)p_pipe_reader );
	CHECK_VALUE( ret_val );
	
	return SUCCESS;
}

_int32 bpr_uninit_struct( BT_PIPE_READER *p_pipe_reader )
{
	LOG_DEBUG( " bpr_uninit_struct");

	bdr_uninit_struct( &(p_pipe_reader->_bt_data_reader));
 
	 return SUCCESS;
}
_int32 bpr_add_read_bt_range( BT_PIPE_READER *p_pipe_reader,BT_RANGE *p_bt_range )
{
	_int32 ret_val = SUCCESS;
	BT_DATA_MANAGER *p_bt_data_manager = p_pipe_reader->_bt_data_reader_mgr_ptr->_bt_data_mgr_ptr;

	LOG_DEBUG( " bpr_add_read_bt_range");

	if(FALSE==bdm_range_is_write_in_disk(p_bt_data_manager,p_pipe_reader->_file_index,&(p_pipe_reader->_padding_range)))
	{
		LOG_DEBUG( " bpr_add_read_bt_range:bdm_range_is_write_in_disk return FALSE!");
		ret_val =  BT_RANGE_NOT_IN_DISK;
		goto ErrorHanle;
	}
	
	ret_val = bdr_read_bt_range( &(p_pipe_reader->_bt_data_reader), p_bt_range);
	if( ret_val !=SUCCESS)
	{
		goto ErrorHanle;
	}

	p_pipe_reader->is_set_range=TRUE;
	
	return SUCCESS;
	
ErrorHanle:
	
		if((ret_val==BT_RANGE_NOT_IN_DISK)||(ret_val==BT_READ_GET_BUFFER_FAIL))
		{
			p_pipe_reader->_bt_range._pos= p_bt_range->_pos;
			p_pipe_reader->_bt_range._length= p_bt_range->_length;
			p_pipe_reader->is_set_range=FALSE;
			return ret_val;
		}
		else
		{
			LOG_DEBUG( "!!! bpr_add_read_bt_range error:ret_val=%d",ret_val);
			//CHECK_VALUE(ret_val);
			return ret_val;
		}
	
}

_int32 bpr_read_data( void *p_user, _u32 file_index, RANGE_DATA_BUFFER *p_range_data_buffer, notify_read_result p_call_back )
{
	BT_PIPE_READER *p_pipe_reader = ( BT_PIPE_READER *)p_user;
	struct tagBT_DATA_MANAGER *p_bt_data_manager = p_pipe_reader->_bt_data_reader_mgr_ptr->_bt_data_mgr_ptr;
	LOG_DEBUG( " bpr_read_data");

	return bdm_read_data( p_bt_data_manager, file_index, p_range_data_buffer, p_call_back, p_user );
}

_int32 bpr_add_data( void *p_user, _u32 offset, const char *p_buffer_data, _u32 data_len )
{
	BT_PIPE_READER *p_pipe_reader = ( BT_PIPE_READER *)p_user;
	LOG_DEBUG( " bpr_add_data:offset =%u,data_len=%u, p_pipe_reader->_buffer_len=%u",offset ,data_len, p_pipe_reader->_buffer_len);

	sd_assert( offset +data_len<= p_pipe_reader->_buffer_len);
	sd_memcpy(p_pipe_reader->_data_buffer_ptr+offset, p_buffer_data, data_len);
	return SUCCESS;
}

_int32 bpr_read_data_result_handler( void *p_user, _int32 read_result, _u32 read_len )
{
	_int32 ret_val = SUCCESS;
	BT_PIPE_READER *p_pipe_reader = ( BT_PIPE_READER *)p_user;
	
	LOG_DEBUG( " bpr_read_data_result_handler(read_result=%d,read_len=%u,_buffer_len=%u)..." ,read_result ,read_len,p_pipe_reader->_buffer_len);
	LOG_DEBUG( " bpr_read_data_result_handler:_data_buffer_ptr=0x%X,_buffer_len=%u,_bt_range(%llu,%llu),_file_index=%u,_padding_range(%u,%u)" ,
	p_pipe_reader->_data_buffer_ptr ,p_pipe_reader->_buffer_len,p_pipe_reader->_bt_range._pos,p_pipe_reader->_bt_range._length,p_pipe_reader->_file_index,p_pipe_reader->_padding_range._index,p_pipe_reader->_padding_range._num);

	if((read_result==SUCCESS)&&(read_len==p_pipe_reader->_buffer_len))
	ret_val=bprm_read_data_result_handler(p_pipe_reader->_bt_data_reader_mgr_ptr,p_pipe_reader,  read_result, read_len);
	else
	ret_val=bprm_read_data_result_handler(p_pipe_reader->_bt_data_reader_mgr_ptr,p_pipe_reader,  -1,read_len );
	
	CHECK_VALUE(ret_val);
	
	return SUCCESS;
}




#endif /* #ifdef ENABLE_BT */


