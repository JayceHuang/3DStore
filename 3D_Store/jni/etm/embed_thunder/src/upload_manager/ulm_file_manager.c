#if !defined(__ULM_FILE_MANAGER_C_20090604)
#define __ULM_FILE_MANAGER_C_20090604
/*--------------------------------------------------------------------------*/
/*                               IDENTIFICATION                             */
/*--------------------------------------------------------------------------*/
/*     Filename  : ulm_file_manager.c                                         */
/*     Author     : ZengYuqing                                              */
/*     Project    : EmbedThunder                                        */
/*     Module     : upload_manager													*/
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
/* This file contains the procedure of upload manager                       */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*                              HISTORY                                     */
/*--------------------------------------------------------------------------*/
/*   Date     |    Author   | Modification                                  */
/*--------------------------------------------------------------------------*/
/* 2009.06.04 | ZengYuqing  | Creation                                      */
/*--------------------------------------------------------------------------*/


#include "ulm_file_manager.h"
#include "ulm_pipe_manager.h"
#include "upload_manager.h"
#ifdef UPLOAD_ENABLE

#include "utility/settings.h"
#include "utility/mempool.h"
#include "utility/string.h"
#include "utility/utility.h"
#include "utility/time.h"
#include "data_manager/data_manager_interface.h"
#ifdef ENABLE_BT
#include "bt_download/bt_data_manager/bt_data_manager_interface.h"
#include "bt_download/bt_data_manager/bt_data_manager_imp.h"
#endif
#ifdef ENABLE_EMULE
#include "emule/emule_interface.h"
#endif
#include "utility/logid.h"
#ifdef LOGID
#undef LOGID
#endif

#define LOGID LOGID_UPLOAD_MANAGER

#include "utility/logger.h"

    
/*--------------------------------------------------------------------------*/
/*                Global declarations				        */
/*--------------------------------------------------------------------------*/
/* Pointer of the ulm_file_manager,only one ulm_file_manager when program is running */
static ULM_FILE_MANAGER * gp_ulm_file_manager = NULL;
static SLAB * gp_ulm_file_struct_slab = NULL;
static SLAB * gp_ulm_file_read_slab = NULL;
static SLAB * gp_ulm_map_key_slab = NULL;

/*--------------------------------------------------------------------------*/
/*                Interfaces provid for upload_manager				        */
/*--------------------------------------------------------------------------*/
_int32 init_upload_file_manager(void)
{
	_int32 ret_val = SUCCESS;
	
	LOG_DEBUG("init_upload_file_manager");
	
	ret_val = sd_malloc(sizeof(ULM_FILE_MANAGER) ,(void**)&gp_ulm_file_manager);
	if(ret_val!=SUCCESS) return ret_val;

	//sd_memset(gp_ulm_file_manager,0,sizeof(ULM_FILE_MANAGER));

	ret_val = mpool_create_slab(sizeof(UFM_FILE_STRUCT), MIN_ULM_FILE_ITEM_MEMORY, 0, &gp_ulm_file_struct_slab);
	if(ret_val!=SUCCESS) goto ErrorHanle;
	
	ret_val = mpool_create_slab(sizeof(UFM_FILE_READ), MIN_ULM_READ_ITEM_MEMORY, 0, &gp_ulm_file_read_slab);
	if(ret_val!=SUCCESS) goto ErrorHanle;
	
	ret_val = mpool_create_slab(CID_SIZE, MIN_ULM_MAP_KEY_ITEM_MEMORY, 0, &gp_ulm_map_key_slab);
	if(ret_val!=SUCCESS) goto ErrorHanle;
	
	map_init(&(gp_ulm_file_manager->_file_structs), ufm_map_compare_fun);
	map_init(&(gp_ulm_file_manager->_dm_file_read), ufm_dm_file_read_map_compare_fun);
	gp_ulm_file_manager->_max_files =ULM_MAX_FILES_NUM;
	gp_ulm_file_manager->_max_idle_interval =ULM_MAX_IDLE_INTERVAL;
		
	settings_get_int_item( "upload_manager.max_files",(_int32*)&(gp_ulm_file_manager->_max_files));
	settings_get_int_item( "upload_manager.max_idle_interval",(_int32*)&(gp_ulm_file_manager->_max_idle_interval));
	
	gp_ulm_file_manager->_event_handle=NULL;
	
	LOG_DEBUG("init_upload_file_manager:SUCCESS");
	return SUCCESS;
	
ErrorHanle:
	
	if(gp_ulm_file_read_slab!=NULL)
	{
		 mpool_destory_slab(gp_ulm_file_read_slab);
		 gp_ulm_file_read_slab=NULL;
	}

	if(gp_ulm_file_struct_slab!=NULL)
	{
		 mpool_destory_slab(gp_ulm_file_struct_slab);
		 gp_ulm_file_struct_slab=NULL;
	}

	SAFE_DELETE(gp_ulm_file_manager);
	
	LOG_DEBUG("init_upload_file_manager:FAILED!ret_val=%d ",ret_val);
	return ret_val;
}
_int32 uninit_upload_file_manager(void)
{
	
	LOG_DEBUG("uninit_upload_file_manager");

	if(gp_ulm_file_manager==NULL) return SUCCESS;
	
	//ufm_clear();

	if(gp_ulm_map_key_slab!=NULL)
	{
		 mpool_destory_slab(gp_ulm_map_key_slab);
		 gp_ulm_map_key_slab = NULL;
	}
	 
	if(gp_ulm_file_read_slab!=NULL)
	{
		 mpool_destory_slab(gp_ulm_file_read_slab);
		 gp_ulm_file_read_slab = NULL;


	}
	 
	if(gp_ulm_file_struct_slab!=NULL)
	{
		 mpool_destory_slab(gp_ulm_file_struct_slab);
		 gp_ulm_file_struct_slab = NULL;
	}

	SAFE_DELETE(gp_ulm_file_manager);

	LOG_DEBUG("uninit_upload_file_manager:SUCCESS");
	return SUCCESS;
}



/*--------------------------------------------------------------------------*/
/*                Interfaces provid for p2p pipe				        */
/*--------------------------------------------------------------------------*/
_int32 ufm_read_file_from_common_data_manager(void * data_manager,RANGE_DATA_BUFFER * p_data_buffer, void* callback, DATA_PIPE * p_pipe)
{
	_int32 ret_val=SUCCESS;
	UFM_FILE_READ * p_file_read=NULL;
	PAIR node;
      RANGE_DATA_BUFFER_LIST range_buffer_list;
	RANGE_DATA_BUFFER* cur_buffer = NULL;
	LIST_ITERATOR  cur_node ;
	
	LOG_DEBUG( "ufm_read_file_from_common_data_manager:0x%X",p_pipe );

	if(TRUE==ufm_is_pipe_exist_in_dm_map(p_pipe))
	{
		return -1; //BT_PIPE_READER_FULL;
	}

	sd_assert(1== p_data_buffer->_range._num) ;
	if(TRUE== dm_range_is_cached((DATA_MANAGER*)data_manager, &(p_data_buffer->_range)))
	{
		/* Read data from cache */
		LOG_DEBUG( "ufm_read_file_from_common_data_manager:0x%X Read data from cache",p_pipe );
		list_init( &(range_buffer_list._data_list));
		ret_val = dm_get_range_data_buffer( (DATA_MANAGER*)data_manager, p_data_buffer->_range,  &range_buffer_list);
		CHECK_VALUE(ret_val);

		sd_assert(1 == list_size(&(range_buffer_list._data_list))) ;
			
		cur_node = LIST_BEGIN(range_buffer_list._data_list);
			
		sd_assert(cur_node != LIST_END(range_buffer_list._data_list)) ;
	
		cur_buffer = (RANGE_DATA_BUFFER*)LIST_VALUE(cur_node);
		sd_assert(cur_buffer->_range._index== p_data_buffer->_range._index) ;
		sd_assert(cur_buffer->_range._num== p_data_buffer->_range._num) ;
		sd_assert(cur_buffer->_data_length== p_data_buffer->_data_length) ;
		sd_assert(cur_buffer->_data_length<= p_data_buffer->_buffer_length) ;
		sd_memcpy(p_data_buffer->_data_ptr,cur_buffer->_data_ptr,p_data_buffer->_buffer_length) ;
		list_clear( &(range_buffer_list._data_list));
		((read_data_callback_func)callback)(0,p_data_buffer,p_pipe); 
		return SUCCESS;
	}

	/* The data is not in cache ... */
	ret_val = ufm_create_file_read(p_data_buffer, callback, (void*)p_pipe,&p_file_read);
	CHECK_VALUE(ret_val);

	
	node._key = (void*)p_pipe;
	node._value = (void*)p_file_read;
	ret_val = map_insert_node(&(gp_ulm_file_manager->_dm_file_read), &node);
	if(ret_val!=SUCCESS)
	{
		ufm_delete_file_read(p_file_read);
		CHECK_VALUE(ret_val);
	}

	if(TRUE== dm_range_is_write((DATA_MANAGER*)data_manager, &(p_data_buffer->_range)))
	{
		/* Read data from disk */
		LOG_DEBUG( "ufm_read_file_from_common_data_manager:0x%X Read data from disk",p_pipe );
		ret_val =   dm_asyn_read_data_buffer( (DATA_MANAGER*) data_manager, p_data_buffer, ufm_data_manager_notify_file_read_result, (void *)p_pipe );
	}
	else
	{
		/* The data is being written,wait... */
		LOG_DEBUG( "ufm_read_file_from_common_data_manager:0x%X The data is being written,wait...",p_pipe );
		p_file_read->_data_manager_ptr=data_manager;
		ret_val = sd_time_ms(&(p_file_read->_time_stamp));
		if((ret_val==SUCCESS)&&(p_file_read->_time_stamp==0))
		{
			ret_val = sd_time_ms(&(p_file_read->_time_stamp));
		}
	}
	
	if(ret_val!=SUCCESS)
	{
		LOG_DEBUG( "ufm_read_file_from_common_data_manager:0x%X error,ret_val=%d",p_pipe,ret_val );
		map_erase_node(&(gp_ulm_file_manager->_dm_file_read), (void*)p_pipe);
		ufm_delete_file_read(p_file_read);
		return ret_val;
	}

	return SUCCESS;
}


/*--------------------------------------------------------------------------*/
/*                Interfaces provid for speedup pipe				        */
/*--------------------------------------------------------------------------*/
#ifdef ENABLE_BT
_int32 ufm_speedup_pipe_read_file_from_bt_data_manager(void * data_manager,RANGE_DATA_BUFFER * p_data_buffer, void* callback, DATA_PIPE * p_pipe)
{
	_int32 ret_val=SUCCESS;
	UFM_FILE_READ * p_file_read=NULL;
	PAIR node;
	_u32 file_index = 0;
      RANGE_DATA_BUFFER_LIST range_buffer_list;
	RANGE_DATA_BUFFER* cur_buffer = NULL;
	LIST_ITERATOR  cur_node ;
	
	LOG_DEBUG( "ufm_speedup_pipe_read_file_from_bt_data_manager:0x%X",p_pipe );

	if(TRUE==ufm_is_pipe_exist_in_dm_map(p_pipe))
	{
		return -1; //BT_PIPE_READER_FULL;
	}

	file_index = dp_get_pipe_file_index(p_pipe);
	LOG_DEBUG( "ufm_speedup_pipe_read_file_from_bt_data_manager:0x%X, file_index:%u",
		p_pipe, file_index );	
	if( file_index == MAX_U32 )
	{
		CHECK_VALUE( ULM_ERR_FILE_INDEX );
	}
	
	sd_assert(1== p_data_buffer->_range._num) ;
	
	if(TRUE== bdm_range_is_cache((struct tagBT_DATA_MANAGER*)data_manager, file_index,&(p_data_buffer->_range)))
	{
		/* Read data from cache */
		LOG_DEBUG( "ufm_speedup_pipe_read_file_from_bt_data_manager:0x%X Read data from cache",p_pipe );
		list_init( &(range_buffer_list._data_list));
		ret_val = bdm_get_cache_data_buffer( (struct tagBT_DATA_MANAGER*)data_manager, file_index, &(p_data_buffer->_range), &range_buffer_list);
		CHECK_VALUE(ret_val);

		sd_assert(1 == list_size(&(range_buffer_list._data_list))) ;
			
		cur_node = LIST_BEGIN(range_buffer_list._data_list);
			
		sd_assert(cur_node != LIST_END(range_buffer_list._data_list)) ;
	
		cur_buffer = (RANGE_DATA_BUFFER*)LIST_VALUE(cur_node);
		sd_assert(cur_buffer->_range._index== p_data_buffer->_range._index) ;
		sd_assert(cur_buffer->_range._num== p_data_buffer->_range._num) ;
		sd_assert(cur_buffer->_data_length== p_data_buffer->_data_length) ;
		sd_assert(cur_buffer->_data_length<= p_data_buffer->_buffer_length) ;
		sd_memcpy(p_data_buffer->_data_ptr,cur_buffer->_data_ptr,p_data_buffer->_buffer_length) ;
		list_clear( &(range_buffer_list._data_list));
		((read_data_callback_func)callback)(0,p_data_buffer,p_pipe); 
		return SUCCESS;
	}

	/* The data is not in cache ... */
	ret_val = ufm_create_file_read(p_data_buffer, callback, (void*)p_pipe,&p_file_read);
	CHECK_VALUE(ret_val);

	node._key = (void*)p_pipe;
	node._value = (void*)p_file_read;
	ret_val = map_insert_node(&(gp_ulm_file_manager->_dm_file_read), &node);
	if(ret_val!=SUCCESS)
	{
		ufm_delete_file_read(p_file_read);
		CHECK_VALUE(ret_val);
	}

	if(TRUE== bdm_range_is_write((struct tagBT_DATA_MANAGER*)data_manager,file_index, &(p_data_buffer->_range)))
	{
		/* Read data from disk */
		LOG_DEBUG( "ufm_speedup_pipe_read_file_from_bt_data_manager:0x%X Read data from disk",p_pipe );
		ret_val =   bdm_asyn_read_data_buffer( (struct tagBT_DATA_MANAGER *) data_manager, p_data_buffer, ufm_data_manager_notify_file_read_result, (void *)p_pipe, file_index);
	}
	else
	{
		/* The data is being written,wait... */
		LOG_DEBUG( "ufm_speedup_pipe_read_file_from_bt_data_manager:0x%X The data is being written,wait...",p_pipe );
		p_file_read->_data_manager_ptr=data_manager;
		p_file_read->_file_index=file_index;
		ret_val = sd_time_ms(&(p_file_read->_time_stamp));
		if((ret_val==SUCCESS)&&(p_file_read->_time_stamp==0))
		{
			ret_val = sd_time_ms(&(p_file_read->_time_stamp));
		}
	}
	
	if(ret_val!=SUCCESS)
	{
		LOG_DEBUG( "ufm_speedup_pipe_read_file_from_bt_data_manager:0x%X error,ret_val=%d",p_pipe,ret_val );
		map_erase_node(&(gp_ulm_file_manager->_dm_file_read), (void*)p_pipe);
		ufm_delete_file_read(p_file_read);
		CHECK_VALUE(ret_val);
	}

	return SUCCESS;
}
#endif

#ifdef ENABLE_EMULE
_int32 ufm_speedup_pipe_read_file_from_emule_data_manager(void * data_manager,RANGE_DATA_BUFFER * p_data_buffer, void* callback, DATA_PIPE * p_pipe)
{
	_int32 ret_val=SUCCESS;
	UFM_FILE_READ * p_file_read=NULL;
	PAIR node;

	
	LOG_DEBUG( "ufm_speedup_pipe_read_file_from_emule_data_manager:0x%x",p_pipe );

	if(TRUE==ufm_is_pipe_exist_in_dm_map(p_pipe))
	{
		return -1; //BT_PIPE_READER_FULL;
	}
	sd_assert(1== p_data_buffer->_range._num) ;
	ret_val = ufm_create_file_read(p_data_buffer, callback, (void*)p_pipe,&p_file_read);
	CHECK_VALUE(ret_val);
	node._key = (void*)p_pipe;
	node._value = (void*)p_file_read;
	ret_val = map_insert_node(&(gp_ulm_file_manager->_dm_file_read), &node);
	if(ret_val!=SUCCESS)
	{
		ufm_delete_file_read(p_file_read);
		CHECK_VALUE(ret_val);
	}
	ret_val = emule_read_data((EMULE_DATA_MANAGER*)data_manager, p_data_buffer, ufm_data_manager_notify_file_read_result, (void *)p_pipe);
    if(ret_val == ERROR_WAIT_NOTIFY || ret_val == FM_READ_CLOSE_EVENT)
        return SUCCESS;
	if(ret_val!=SUCCESS)
	{
		LOG_DEBUG( "ufm_speedup_pipe_read_file_from_emule_data_manager:0x%X error,ret_val=%d",p_pipe,ret_val );
		map_erase_node(&(gp_ulm_file_manager->_dm_file_read), (void*)p_pipe);
		ufm_delete_file_read(p_file_read);
		return ret_val;
	}

	return SUCCESS;

}
#endif

/*--------------------------------------------------------------------------*/
/*                Interfaces provid for bt pipe				        */
/*--------------------------------------------------------------------------*/

#ifdef ENABLE_BT
_int32 ufm_bt_pipe_read_file_from_bt_data_manager(void * data_manager,const BT_RANGE *p_bt_range, 
	char *p_data_buffer, _u32 buffer_len, void *callback, 
	DATA_PIPE * p_pipe)
{
	_int32 ret_val=SUCCESS;
	UFM_FILE_READ * p_file_read=NULL;
	PAIR node;
	
	LOG_DEBUG( "ufm_bt_pipe_read_file_from_bt_data_manager:0x%X",p_pipe );
	
	if(TRUE==ufm_is_pipe_exist_in_dm_map(p_pipe))
	{
		return BT_PIPE_READER_FULL; //BT_PIPE_READER_FULL;
	}

	ret_val = ufm_create_file_read(NULL, callback, (void*)p_pipe,&p_file_read);
	CHECK_VALUE(ret_val);



	node._key = (void*)p_pipe;
	node._value = (void*)p_file_read;
	ret_val = map_insert_node(&(gp_ulm_file_manager->_dm_file_read), &node);
	if(ret_val!=SUCCESS)
	{
		ufm_delete_file_read(p_file_read);
		CHECK_VALUE(ret_val);
	}

	ret_val =   bdm_bt_pipe_read_data_buffer( (struct tagBT_DATA_MANAGER *) data_manager, p_bt_range,
		p_data_buffer, buffer_len, ufm_bt_data_manager_notify_bt_range_read_result, (void *)p_pipe);
	if(ret_val!=SUCCESS)
	{
		map_erase_node(&(gp_ulm_file_manager->_dm_file_read), (void*)p_pipe);
		ufm_delete_file_read(p_file_read);
		//CHECK_VALUE(ret_val);
	}

	return ret_val;
}
#endif

#ifdef ENABLE_EMULE
_int32 ufm_read_file_from_emule_data_manager(void * data_manager,RANGE_DATA_BUFFER * p_data_buffer, void* callback, DATA_PIPE * p_pipe)
{
	_int32 ret_val=SUCCESS;
	UFM_FILE_READ * p_file_read=NULL;
	PAIR node;
	
	LOG_DEBUG( "ufm_read_file_from_emule_data_manager:0x%X",p_pipe );

	if(TRUE==ufm_is_pipe_exist_in_dm_map(p_pipe))
	{
		return -1; //emule_PIPE_READER_FULL;
	}

	/* The data is not in cache ... */
	ret_val = ufm_create_file_read(p_data_buffer, callback, (void*)p_pipe,&p_file_read);
	CHECK_VALUE(ret_val);

	
	node._key = (void*)p_pipe;
	node._value = (void*)p_file_read;
	ret_val = map_insert_node(&(gp_ulm_file_manager->_dm_file_read), &node);
	if(ret_val!=SUCCESS)
	{
		ufm_delete_file_read(p_file_read);
		CHECK_VALUE(ret_val);
	}
    
    ret_val = emule_read_data((EMULE_DATA_MANAGER*) data_manager, p_data_buffer, ufm_data_manager_notify_file_read_result, (void *)p_pipe);
    if(ret_val == SUCCESS )
    {
		LOG_DEBUG( "ufm_read_file_from_emule_data_manager:0x%x SUCCESS",p_pipe );
		map_erase_node(&(gp_ulm_file_manager->_dm_file_read), (void*)p_pipe);
		ufm_delete_file_read(p_file_read);
        return SUCCESS;
    }
    else if(ret_val != ERROR_WAIT_NOTIFY)
    {
		map_erase_node(&(gp_ulm_file_manager->_dm_file_read), (void*)p_pipe);
		ufm_delete_file_read(p_file_read);
    }
    LOG_DEBUG( "ufm_read_file_from_emule_data_manager:0x%x, ret:%d",p_pipe,ret_val );
	return ret_val;
}

#endif

_int32 ufm_read_file(ID_RC_INFO *p_file_info,RANGE_DATA_BUFFER * p_data_buffer, void* callback, void* user_data)
{
	_int32 ret_val=SUCCESS;
	UFM_FILE_STRUCT * p_file_struct=NULL; 
	UFM_FILE_READ * p_file_read=NULL;
	LOG_DEBUG( "ufm_read_file:0x%X",user_data );

	p_file_struct= ufm_get_file_struct(p_file_info->_gcid);
	if(NULL==p_file_struct)
	{
		ret_val = ufm_create_file_struct(&p_file_struct);
		CHECK_VALUE(ret_val);
		/* add file struct to map */
		ret_val = ufm_add_file_struct_to_map(p_file_struct,p_file_info->_gcid);
		if(ret_val!=SUCCESS)
		{
			ufm_delete_file_struct(p_file_struct);
			CHECK_VALUE(ret_val);
		}
		
		ret_val = ufm_open_file_struct(p_file_info,p_file_struct);
		if(ret_val!=SUCCESS)
		{
			ufm_del_file_struct_from_map(p_file_info->_gcid);
			ufm_delete_file_struct(p_file_struct);
			return ret_val;
		}
		
		
	}
	else
	{
		if(p_file_struct->_state==UFM_CLOSING) 
		{
			LOG_DEBUG( "ufm_read_file:0x%X,file is closing...",user_data );
			return -1;
		}
	}

	ret_val = ufm_create_file_read(p_data_buffer, callback, user_data,&p_file_read);
	CHECK_VALUE(ret_val);

	ret_val = ufm_commit_file_read(p_file_struct,p_file_read);
	if(ret_val!=SUCCESS)
	{
		ufm_delete_file_read(p_file_read);
		CHECK_VALUE(ret_val);
	}
	
	return SUCCESS;
}

BOOL ufm_check_need_cancel_read_file(const _u8 *gcid, void* user_data)
{
	_int32 ret_val = SUCCESS;
	UFM_FILE_STRUCT *p_file_struct = NULL;
	UFM_FILE_READ *p_file_read = NULL;
	LIST_ITERATOR cur_node;
	
	LOG_DEBUG( "ufm_cancel_read_file:0x%X",user_data );
	
	if(gcid!=NULL)
	{
		ret_val=map_find_node(&(gp_ulm_file_manager->_file_structs), (void*)gcid, (void **)&p_file_struct);
		sd_assert(ret_val==SUCCESS);
			
		if(NULL==p_file_struct) return FALSE;
		if(p_file_struct->_cur_read!= NULL)
		{
			p_file_read=p_file_struct->_cur_read;
			if(p_file_read->_user_data==user_data)
			{
				if(p_file_read->_is_cancel==FALSE)
					return TRUE;
			}
		}

	
		for(cur_node = LIST_BEGIN( p_file_struct->_read_list); cur_node != LIST_END( p_file_struct->_read_list);
			cur_node = LIST_NEXT(cur_node))
		{
			p_file_read = (UFM_FILE_READ *)LIST_VALUE( cur_node );
			if(p_file_read->_user_data==user_data)
			{
				if(p_file_read->_is_cancel==FALSE)
					return TRUE;
			}
		}

	}
	else
	{
		ret_val=map_find_node(&(gp_ulm_file_manager->_dm_file_read), (void*)user_data, (void **)&p_file_read);
		CHECK_VALUE(ret_val);
		if(NULL==p_file_read) return FALSE;
		if(p_file_read->_is_cancel==FALSE) return TRUE;
 	}

	return FALSE;
}
_int32 ufm_cancel_read_file(const _u8 *gcid, void* user_data)
{
	_int32 ret_val = SUCCESS;
	UFM_FILE_STRUCT *p_file_struct = NULL;
	
	LOG_DEBUG( "ufm_cancel_read_file:0x%X",user_data );
	
	ret_val=map_find_node(&(gp_ulm_file_manager->_file_structs), (void*)gcid, (void **)&p_file_struct);
	CHECK_VALUE(ret_val);
		
	if(NULL==p_file_struct) return UFM_ERR_FILE_STRUCT_NOT_FOUND;

	ret_val=ufm_cancel_file_read(p_file_struct,user_data);
	//CHECK_VALUE(ret_val);

	return ret_val;
}

_int32 ufm_cancel_read_from_data_manager(void* user_data)
{
	_int32 ret_val = SUCCESS;
	UFM_FILE_READ *p_file_read = NULL;
	read_data_callback_func p2p_read_callback; 
	RANGE_DATA_BUFFER * p_data_buffer=NULL;	
	ULM_PIPE_TYPE pipe_type;
	
	ret_val=map_find_node(&(gp_ulm_file_manager->_dm_file_read), (void*)user_data, (void **)&p_file_read);
	CHECK_VALUE(ret_val);
		
	LOG_DEBUG( "ufm_cancel_read_from_data_manager:0x%X,p_file_read=0x%X",user_data,p_file_read );

	if(NULL==p_file_read) return UFM_ERR_READ_OP_NOT_FOUND;

	p_file_read->_is_cancel=TRUE;

	if(p_file_read->_time_stamp!=0)
	{
		pipe_type = upm_get_pipe_type((DATA_PIPE*)user_data);
		if((pipe_type==ULM_D_AND_U_P2P_PIPE)
#ifdef ENABLE_BT
			||(pipe_type==ULM_D_AND_U_BT_P2P_PIPE)
#endif
			)
		{
			LOG_DEBUG( "ufm_cancel_read_from_data_manager:0x%X is waiting but canceled!",user_data );
			p2p_read_callback=(read_data_callback_func)(p_file_read->_callback);
			p_data_buffer = p_file_read->_p_data_buffer;
			map_erase_node(&(gp_ulm_file_manager->_dm_file_read), (void*)user_data);
			ufm_delete_file_read(p_file_read);
			p2p_read_callback(UFM_ERR_READ_CANCELED,p_data_buffer,user_data); 		
		}
		else
		{
			/* ULM_D_AND_U_BT_PIPE */
			sd_assert(FALSE);
		}
	}
	return SUCCESS;
}


/*--------------------------------------------------------------------------*/
/*                Interfaces provid for rclist_manager and ulm_pipe_manager				        */
/*--------------------------------------------------------------------------*/
_int32 ufm_close_file(const _u8 *gcid)
{
	_int32 ret_val = SUCCESS;
	UFM_FILE_STRUCT *p_file_struct = NULL;
	
	LOG_DEBUG( "ufm_close_file" );

	sd_assert(gcid!=NULL);
	/* del pipe操作移到closefile里面，防止删pipe时map找不到对应的file_struct,  导致异常逻辑  */
 	upm_del_pipes(gcid);
	
	ret_val=map_find_node(&(gp_ulm_file_manager->_file_structs), (void*)gcid, (void **)&p_file_struct);
	CHECK_VALUE(ret_val);
		
	if(NULL==p_file_struct) return SUCCESS;

	ret_val=ufm_del_file_struct_from_map(gcid);
	CHECK_VALUE(ret_val);
	
	ufm_close_file_struct(p_file_struct);
	//ufm_delete_file_struct( p_file_struct);
 	//upm_del_pipes(gcid);
	
	return SUCCESS;
}

BOOL ufm_is_file_full(void)
{
	BOOL ret_bol=(map_size(&(gp_ulm_file_manager->_file_structs))<gp_ulm_file_manager->_max_files)?FALSE:TRUE;
	
	LOG_DEBUG( "ufm_is_file_full:%d",ret_bol );
	return ret_bol;
}

/* 文件分数算法：
	等于在该文件上正在读取数据的上传pipe的个数
*/
_u32 ufm_get_file_score(const _u8 *gcid)
{
	//_int32 ret_val = SUCCESS;
	UFM_FILE_STRUCT *p_file_struct = NULL;
	_u32 score=0;
	
	LOG_DEBUG( "ufm_get_file_score" );
	
	if(NULL==gcid) return 0;
	
	p_file_struct=ufm_get_file_struct(gcid);
		
	if(NULL==p_file_struct) return 0;

	if(p_file_struct->_state==UFM_CLOSING)  return 0;
	
	score=1;
	
	LOG_DEBUG( "ufm_get_file_score=%u",score );
	return score;
}

/*--------------------------------------------------------------------------*/
/*                Interfaces provid for upload_read_file				        */
/*--------------------------------------------------------------------------*/
_int32 ufm_notify_file_create (struct tagTMP_FILE *p_file_struct, void *p_user, _int32 create_result)
{
	_int32 ret_val=SUCCESS;
	UFM_FILE_STRUCT *p_ufm_file_struct = (UFM_FILE_STRUCT *)p_user;
	_u8 * gcid=NULL;
	
	LOG_DEBUG( "ufm_notify_file_create:create_result=%d,%s",create_result ,p_file_struct->_file_name);
	
	sd_assert(p_ufm_file_struct!=NULL);
	
	if(p_ufm_file_struct->_state==UFM_CLOSING)  return SUCCESS;

	if(create_result!=SUCCESS)
	{
		gcid=ufm_get_file_gcid(p_ufm_file_struct);
		sd_assert(gcid!=NULL);
		if(FALSE==ulm_handle_invalid_record( gcid,create_result))
		{
			ufm_close_file(gcid);
		}
		return SUCCESS;
	}
	
	sd_assert(p_file_struct!=NULL);  

	sd_assert(p_ufm_file_struct->_cur_read==NULL);

	p_ufm_file_struct->_state= UFM_OPENED;
	
	ret_val =  ufm_execute_file_read(p_ufm_file_struct);
	CHECK_VALUE(ret_val);
	
	
	return SUCCESS;
}


_int32 ufm_notify_file_read_result (struct tagTMP_FILE *p_file_struct
    , void *p_user
    , RANGE_DATA_BUFFER *data_buffer
    , _int32 read_result
    , _u32 read_len )
{
	_int32 ret_val=SUCCESS;
	UFM_FILE_STRUCT *p_ufm_file_struct = (UFM_FILE_STRUCT *)p_user;
	read_data_callback_func p2p_read_callback; 
	_u8 * gcid=NULL;
	
	LOG_DEBUG( "ufm_notify_file_read_result:result=%d,len=%u,range[%u,%u],data_len=%u,buffer_len=%u,%s",
		read_result ,read_len,data_buffer->_range._index,data_buffer->_range._num,data_buffer->_data_length,data_buffer->_buffer_length,p_file_struct->_file_name);
	
	
	sd_assert(p_ufm_file_struct!=NULL);
	sd_assert(p_file_struct!=NULL);  
	
	if(read_result==0)
	{
		sd_assert(data_buffer->_data_length==read_len);
	}

	sd_assert(p_ufm_file_struct->_cur_read!=NULL);
	//sd_assert(p_ufm_file_struct->_state==UFM_READING);

	LOG_DEBUG( "ufm_notify_file_read_result:pipe=0x%X,_is_cancel=%d",p_ufm_file_struct->_cur_read->_user_data ,p_ufm_file_struct->_cur_read->_is_cancel);
	p2p_read_callback=(read_data_callback_func)(p_ufm_file_struct->_cur_read->_callback);
	if(FALSE==p_ufm_file_struct->_cur_read->_is_cancel)
	{
		p2p_read_callback(read_result, data_buffer,p_ufm_file_struct->_cur_read->_user_data);
	}
	else
	{
		p2p_read_callback(UFM_ERR_READ_CANCELED,data_buffer,p_ufm_file_struct->_cur_read->_user_data);
	}

	
	ufm_delete_file_read(p_ufm_file_struct->_cur_read);
	p_ufm_file_struct->_cur_read=NULL;
	
	if(p_ufm_file_struct->_state==UFM_CLOSING) 
	{
		LOG_DEBUG( "ufm_notify_file_read_result:the file is closing..." );
		return SUCCESS;
	}
	
	p_ufm_file_struct->_state= UFM_OPENED;
	
	if(read_result!=SUCCESS)
	{
		gcid=ufm_get_file_gcid(p_ufm_file_struct);
		sd_assert(gcid!=NULL);
		if(TRUE==ulm_handle_invalid_record( gcid,read_result))
		{
			return SUCCESS;
		}
	}
	
	ret_val =  ufm_execute_file_read(p_ufm_file_struct);
	CHECK_VALUE(ret_val);
	
	
	return SUCCESS;
}


_int32 ufm_notify_file_close  (struct tagTMP_FILE *p_file_struct, void *p_user, _int32 close_result)
{
	UFM_FILE_STRUCT *p_ufm_file_struct = (UFM_FILE_STRUCT *)p_user;
	
	LOG_DEBUG( "ufm_notify_file_close:close_result=%d,%s",close_result ,p_file_struct->_file_name);
	
	sd_assert(p_ufm_file_struct!=NULL);
	sd_assert(p_ufm_file_struct->_cur_read==NULL);
	sd_assert(list_size(&(p_ufm_file_struct->_read_list))==0);
	sd_assert(p_ufm_file_struct->_state==UFM_CLOSING);

	sd_assert(p_file_struct!=NULL);  // Who release this momery ????!!!

	ufm_delete_file_struct(p_ufm_file_struct);

	if(gp_ulm_file_manager->_event_handle!=NULL)
	{
		gp_ulm_file_manager->_file_num_wait_close--;
		if(gp_ulm_file_manager->_file_num_wait_close==0)
		{
			ulm_close_upload_pipes_notify(gp_ulm_file_manager->_event_handle );
			gp_ulm_file_manager->_event_handle=NULL;
		}
	}
	
	return SUCCESS;
}

_int32 ufm_data_manager_notify_file_read_result (struct tagTMP_FILE *p_file_struct
        , void *p_user
        , RANGE_DATA_BUFFER *data_buffer
        , _int32 read_result
        , _u32 read_len )
{
	_int32 ret_val=SUCCESS;
	UFM_FILE_READ * p_file_read=NULL;
	DATA_PIPE *p_pipe= (DATA_PIPE *)p_user;
	read_data_callback_func p2p_read_callback; 
	
	LOG_DEBUG( "ufm_data_manager_notify_file_read_result:pipe=0x%X,result=%d,len=%u,range[%u,%u],data_len=%u,buffer_len=%u,%s",
		p_pipe,read_result ,read_len,data_buffer->_range._index,data_buffer->_range._num,data_buffer->_data_length,data_buffer->_buffer_length,p_file_struct->_file_name);
	
	sd_assert(p_pipe!=NULL);
	sd_assert(p_file_struct!=NULL);  
	
	if(read_result==0)
	{
		sd_assert(data_buffer->_data_length==read_len);
	}
	
	ret_val = map_find_node(&(gp_ulm_file_manager->_dm_file_read),p_user,(void*)&p_file_read);
	sd_assert(ret_val==SUCCESS);
	sd_assert(p_file_read!=NULL);  

	map_erase_node(&(gp_ulm_file_manager->_dm_file_read),p_user);

	LOG_DEBUG( "ufm_data_manager_notify_file_read_result:pipe=0x%X,_is_cancel=%d",p_pipe,p_file_read->_is_cancel);
	p2p_read_callback=(read_data_callback_func)(p_file_read->_callback); 
	if(p_file_read->_is_cancel==FALSE)
	{
		p2p_read_callback(read_result,data_buffer,p_pipe);
	}
	else
	{
		p2p_read_callback(UFM_ERR_READ_CANCELED,data_buffer,p_pipe);
	}

	ufm_delete_file_read(p_file_read);

	return SUCCESS;
}

#ifdef ENABLE_BT
_int32 ufm_bt_data_manager_notify_bt_range_read_result (void *p_user, BT_RANGE *p_bt_range, char *p_data_buffer, _int32 read_result, _u32 read_len )
{
	_int32 ret_val=SUCCESS;
	UFM_FILE_READ * p_file_read=NULL;
	//DATA_PIPE *p_pipe= (DATA_PIPE *)p_user;
	bt_pipe_read_result bt_read_callback; 
	
	LOG_DEBUG( "ufm_bt_data_manager_notify_bt_range_read_result:pipe=0x%X,result=%d,len=%u,bt_range[%llu,%llu]",
		p_user,read_result ,read_len,p_bt_range->_pos,p_bt_range->_length);
	
	sd_assert(p_user!=NULL);
	sd_assert(p_bt_range!=NULL);  
	sd_assert(p_data_buffer!=NULL);  

	if(read_result==0)
	{
		sd_assert(p_bt_range->_length==read_len);
	}
	
	ret_val = map_find_node(&(gp_ulm_file_manager->_dm_file_read),p_user,(void*)&p_file_read);
	sd_assert(ret_val==SUCCESS);
	sd_assert(p_file_read!=NULL);  

	map_erase_node(&(gp_ulm_file_manager->_dm_file_read),p_user);

	LOG_DEBUG( "ufm_bt_data_manager_notify_bt_range_read_result:pipe=0x%X,_is_cancel=%d",p_user,p_file_read->_is_cancel);
	bt_read_callback=(bt_pipe_read_result)(p_file_read->_callback); 
	if(p_file_read->_is_cancel==FALSE)
	{
		bt_read_callback(p_user, p_bt_range, p_data_buffer,  read_result,  read_len);
	}
	else
	{
		bt_read_callback(p_user, p_bt_range, p_data_buffer, UFM_ERR_READ_CANCELED,  0 );
	}

	ufm_delete_file_read(p_file_read);

	return SUCCESS;
}
#endif
/*--------------------------------------------------------------------------*/
/*                Internal functions				        */
/*--------------------------------------------------------------------------*/
_int32 ufm_map_compare_fun( void *E1, void *E2 )
{
	_int32 i=0;
	_u8 *gcid1,*gcid2;
	//LOG_DEBUG( "ufm_map_compare_fun" );
	gcid1=(_u8 *)E1;
	gcid2=(_u8 *)E2;
	while(i<CID_SIZE)
	{
		if(gcid1[i]-gcid2[i]==0)
			i++;
		else
			return (_int32)(gcid1[i]-gcid2[i]);
	}
		
	
	//if( sd_is_cid_equal(E1,E2)==TRUE)
		return 0;
	
}
_int32 ufm_dm_file_read_map_compare_fun( void *E1, void *E2 )
{
	//LOG_DEBUG( "ufm_dm_file_read_map_compare_fun" );

	return ((_int32)E1-(_int32)E2);
	//LOG_DEBUG( "E1 =%s,E2=%s",(char*)E1, (char*)E2);
	//return sd_strcmp((char*)E1, (char*)E2 );
}

BOOL ufm_is_file_open(const _u8 *gcid)
{
	_int32 ret_val = SUCCESS;
	UFM_FILE_STRUCT *p_file_struct=NULL;
	
	LOG_DEBUG("ufm_is_file_open");
	ret_val=map_find_node(&(gp_ulm_file_manager->_file_structs), (void*)gcid, (void **)&p_file_struct);
	CHECK_VALUE(ret_val);
		
	if( p_file_struct!=NULL)
	{
		if((p_file_struct->_state==UFM_OPENED)||(p_file_struct->_state==UFM_READING))
			return TRUE;
	}

	return FALSE;
}
BOOL ufm_is_pipe_exist_in_dm_map(DATA_PIPE * p_pipe)
{
	_int32 ret_val = SUCCESS;
      MAP_ITERATOR map_node =  NULL;
	BOOL ret_b=FALSE;

	sd_assert(p_pipe!=NULL);
	
	ret_val = map_find_iterator(&(gp_ulm_file_manager->_dm_file_read),p_pipe, &map_node);
	if((ret_val==SUCCESS)&&(map_node!=MAP_END(gp_ulm_file_manager->_dm_file_read)))
	{
		ret_b= TRUE;
	}

	LOG_DEBUG("ufm_is_pipe_exist_in_dm_map:0x%X,ret_b=%d",p_pipe,ret_b);

	return ret_b;
	
}


UFM_FILE_STRUCT * ufm_get_file_struct(const _u8 *gcid)
{
	_int32 ret_val = SUCCESS;
	UFM_FILE_STRUCT *p_file_struct=NULL;
	
	LOG_DEBUG("ufm_get_file_struct");
	
	if(NULL==gcid) return NULL;
	
	ret_val=map_find_node(&(gp_ulm_file_manager->_file_structs), (void*)gcid, (void **)&p_file_struct);
	sd_assert(ret_val==SUCCESS);
	if(ret_val!=SUCCESS)	return NULL;
	return p_file_struct;
}

_u8 * ufm_get_file_gcid(UFM_FILE_STRUCT *p_file_struct)
{
	UFM_FILE_STRUCT *p_file_struct_tmp=NULL;
	MAP_ITERATOR cur_node ;
	
	LOG_DEBUG("ufm_get_file_gcid:file_num=%u",map_size(&( gp_ulm_file_manager->_file_structs)));
	
	cur_node = MAP_BEGIN( gp_ulm_file_manager->_file_structs);

	while( cur_node != MAP_END( gp_ulm_file_manager->_file_structs ) )
	{
		p_file_struct_tmp = (UFM_FILE_STRUCT *)MAP_VALUE( cur_node );
		if(p_file_struct_tmp==p_file_struct)
		{
			return ( (_u8 *)MAP_KEY( cur_node ));
		}
		cur_node = MAP_NEXT( gp_ulm_file_manager->_file_structs, cur_node );
	}

	return NULL;
}

_int32 ufm_failure_exit(_int32 err_code)
{
	LOG_DEBUG( "ufm_failure_exit" );

	return -1;
}

_int32	ufm_clear(void * event_handle)
{
	UFM_FILE_STRUCT * p_file_struct = NULL;
	UFM_FILE_READ * p_file_read = NULL;
	MAP_ITERATOR cur_node , next_node;
	_u8 * map_key = NULL;
	_u32 file_num = map_size(&( gp_ulm_file_manager->_file_structs));
	
	LOG_DEBUG("ufm_clear:file_num=%u",file_num);

	if(file_num!=0)
	{
		sd_assert(gp_ulm_file_manager->_event_handle==NULL);
		gp_ulm_file_manager->_event_handle = event_handle;
		gp_ulm_file_manager->_file_num_wait_close=file_num;
	}
	
	cur_node = MAP_BEGIN( gp_ulm_file_manager->_file_structs);

	while( cur_node != MAP_END( gp_ulm_file_manager->_file_structs ) )
	{
		p_file_struct = (UFM_FILE_STRUCT *)MAP_VALUE( cur_node );
		map_key = (_u8 *)MAP_KEY( cur_node );
		next_node = MAP_NEXT( gp_ulm_file_manager->_file_structs, cur_node );
		map_erase_iterator(&(gp_ulm_file_manager->_file_structs), cur_node );
		
		mpool_free_slip(gp_ulm_map_key_slab,(void*) map_key);
		
		ufm_close_file_struct(p_file_struct);
		
		cur_node = next_node;
	}

	LOG_DEBUG("ufm_clear:_dm_file_read_num=%u",map_size(&( gp_ulm_file_manager->_dm_file_read)));
	
	cur_node = MAP_BEGIN( gp_ulm_file_manager->_dm_file_read);

	while( cur_node != MAP_END( gp_ulm_file_manager->_dm_file_read ) )
	{
		p_file_read = (UFM_FILE_READ*)MAP_VALUE( cur_node );
		next_node = MAP_NEXT( gp_ulm_file_manager->_dm_file_read, cur_node );
		map_erase_iterator(&(gp_ulm_file_manager->_dm_file_read), cur_node );
		
		
		ufm_stop_file_read(p_file_read);
		ufm_delete_file_read( p_file_read);
		
		cur_node = next_node;
	}

	return SUCCESS;
}

_int32 ufm_add_file_struct_to_map(UFM_FILE_STRUCT *p_file_struct,const _u8* gcid)
{
	_int32 ret_val = SUCCESS;
	_u8 * map_key = NULL;
	PAIR node;
	
	LOG_DEBUG("ufm_add_file_struct_to_map");
	
	map_key = NULL;
	ret_val = mpool_get_slip(gp_ulm_map_key_slab, (void**)&map_key);
	CHECK_VALUE(ret_val);
	sd_memcpy(map_key,gcid,CID_SIZE);

	node._key = (void*)map_key;
	node._value = (void*)p_file_struct;
	ret_val = map_insert_node(&(gp_ulm_file_manager->_file_structs), &node);
	if(ret_val!=SUCCESS)
	{
		mpool_free_slip(gp_ulm_map_key_slab,(void*) map_key);
		CHECK_VALUE(ret_val);
	}


	return SUCCESS;
	
}

_int32 ufm_del_file_struct_from_map(const _u8* gcid)
{
	_int32 ret_val = SUCCESS;
	MAP_ITERATOR result_iterator=NULL;
	_u8 * map_key = NULL;
	
	LOG_DEBUG("ufm_del_file_struct_from_map");
	ret_val=map_find_iterator(&(gp_ulm_file_manager->_file_structs), (void*)gcid, &result_iterator);
	CHECK_VALUE(ret_val);
	sd_assert( result_iterator != MAP_END( gp_ulm_file_manager->_file_structs ) );
	
	map_key = (_u8 *)MAP_KEY( result_iterator );

	map_erase_iterator(&(gp_ulm_file_manager->_file_structs), result_iterator );
		
	mpool_free_slip(gp_ulm_map_key_slab,(void*) map_key);
		
	return SUCCESS;
	
}

_int32 ufm_create_file_struct(UFM_FILE_STRUCT ** pp_file_struct)
{
	_int32 ret_val = SUCCESS;
	UFM_FILE_STRUCT * p_file_struct=NULL;
	
	LOG_DEBUG("ufm_create_file_struct");
	
	ret_val = mpool_get_slip(gp_ulm_file_struct_slab, (void**)&p_file_struct);
	CHECK_VALUE(ret_val);
	list_init(&(p_file_struct->_read_list));
	p_file_struct->_cur_read=NULL;
	p_file_struct->_file_struct=NULL;
	p_file_struct->_state=UFM_OPENING;
	*pp_file_struct=p_file_struct;
	return SUCCESS;
	
}
_int32 ufm_open_file_struct(ID_RC_INFO *p_file_info,UFM_FILE_STRUCT * p_file_struct)
{
	_int32 ret_val = SUCCESS;
	char *file_name=NULL,file_path[MAX_FILE_PATH_LEN]={0};

	LOG_DEBUG("ufm_open_file_struct");

	file_name = sd_strrchr(p_file_info->_path, '/');
	if((file_name==NULL)||(sd_strlen(file_name+1)==0))
	{
		LOG_DEBUG("ufm_open_file_struct:file path too long!");
		return -1;
	}

	file_name = file_name + 1;

	//sd_memset(file_path,0,MAX_FILE_PATH_LEN);
	sd_memcpy(file_path,p_file_info->_path,file_name-p_file_info->_path);
	p_file_struct->_state=UFM_OPENING;
	LOG_DEBUG("ufm_open_file_struct:start open file:%s",p_file_info->_path);
	ret_val = up_create_file_struct( file_name, file_path, p_file_info->_size, (void *)p_file_struct, ufm_notify_file_create, &p_file_struct->_file_struct);
	//CHECK_VALUE(ret_val);

	return ret_val;
}
_int32 ufm_close_file_struct(UFM_FILE_STRUCT *p_file_struct)
{
	//_int32 ret_val=SUCCESS;
	UFM_FILE_READ * p_file_read = NULL;
	LIST_ITERATOR cur_node , next_node;

	LOG_DEBUG( "ufm_close_file_struct:%s",p_file_struct->_file_struct->_file_name);

	if(p_file_struct->_state==UFM_CLOSING)  return SUCCESS;
	
	cur_node = LIST_BEGIN( p_file_struct->_read_list);

	while( cur_node != LIST_END( p_file_struct->_read_list ) )
	{
		p_file_read = (UFM_FILE_READ *)LIST_VALUE( cur_node );
		next_node = LIST_NEXT( cur_node );
		list_erase(&(p_file_struct->_read_list), cur_node );

		ufm_stop_file_read( p_file_read);
		ufm_delete_file_read( p_file_read);
		cur_node = next_node;
	}

	if(p_file_struct->_cur_read!=NULL)
	{
		p_file_struct->_cur_read->_is_cancel=TRUE;
	}

	p_file_struct->_state=UFM_CLOSING;
	return up_file_close( p_file_struct->_file_struct, ufm_notify_file_close, p_file_struct );

}
_int32 ufm_delete_file_struct(UFM_FILE_STRUCT *p_file_struct)
{
	//_int32 ret_val = SUCCESS;
	
	LOG_DEBUG("ufm_delete_file_struct");
	
	sd_assert(p_file_struct!=NULL);
	sd_assert(p_file_struct->_cur_read==NULL);
	sd_assert(list_size(&(p_file_struct->_read_list))==0);
	//sd_assert(p_file_struct->_state==UFM_CLOSING);

	mpool_free_slip(gp_ulm_file_struct_slab,(void*) p_file_struct);
	
	return SUCCESS;
	
}

_int32 ufm_create_file_read(RANGE_DATA_BUFFER * p_data_buffer, void* callback, void* user_data,UFM_FILE_READ ** pp_file_read)
{
	_int32 ret_val = SUCCESS;
	UFM_FILE_READ * p_file_read=NULL;
	
	LOG_DEBUG("ufm_create_file_read:0x%X",user_data);
	
		ret_val = mpool_get_slip(gp_ulm_file_read_slab, (void**)&p_file_read);
		CHECK_VALUE(ret_val);
		p_file_read->_p_data_buffer =p_data_buffer;
		p_file_read->_callback = callback;
		p_file_read->_user_data = user_data;
		p_file_read->_is_cancel=FALSE;
		p_file_read->_time_stamp=0;
		p_file_read->_data_manager_ptr=NULL;
		p_file_read->_file_index=-1;
		*pp_file_read=p_file_read;

	return SUCCESS;
	
}
_int32 ufm_commit_file_read(UFM_FILE_STRUCT *p_file_struct,UFM_FILE_READ *p_file_read)
{
	_int32 ret_val = SUCCESS;
	
	LOG_DEBUG("ufm_commit_file_read:0x%X",p_file_read->_user_data);
	
	ret_val =  list_push(&(p_file_struct->_read_list), (void*)p_file_read);
	CHECK_VALUE(ret_val);
	
	if((p_file_struct->_state!=UFM_OPENING) &&(p_file_struct->_cur_read== NULL))
 
	{
		ret_val =  ufm_execute_file_read(p_file_struct);
		CHECK_VALUE(ret_val);
	}

	return SUCCESS;
	
}
_int32 ufm_execute_file_read(UFM_FILE_STRUCT *p_file_struct)
{
	_int32 ret_val = SUCCESS;
	//UFM_FILE_READ * p_file_read=NULL;
	
	LOG_DEBUG("ufm_execute_file_read");

	sd_assert(p_file_struct->_cur_read== NULL);
	sd_assert(p_file_struct->_state== UFM_OPENED);
	
	if(list_size(&(p_file_struct->_read_list))==0)
	{
		LOG_DEBUG("ufm_execute_file_read:no read command any more!");
		ret_val = sd_time_ms(&(p_file_struct->_time_stamp));
		CHECK_VALUE(ret_val);
		return SUCCESS;
	}

	ret_val =  list_pop(&(p_file_struct->_read_list), (void**)&p_file_struct->_cur_read);
	CHECK_VALUE(ret_val);
	sd_assert(p_file_struct->_cur_read!= NULL);

	p_file_struct->_state= UFM_READING;
	p_file_struct->_time_stamp=0;
	LOG_DEBUG("ufm_execute_file_read,start reading:0x%X",p_file_struct->_cur_read->_user_data);
	ret_val =  up_file_asyn_read_buffer( p_file_struct->_file_struct
	        , p_file_struct->_cur_read->_p_data_buffer
	        , ufm_notify_file_read_result
	        , p_file_struct );
	CHECK_VALUE(ret_val);

	return SUCCESS;
	
}

_int32 ufm_cancel_file_read(UFM_FILE_STRUCT *p_file_struct,void* user_data)
{
	BOOL canceled=FALSE;
	UFM_FILE_READ * p_file_read=NULL;
	LIST_ITERATOR cur_node,tmp_node;
	
	LOG_DEBUG("ufm_cancel_file_read:0x%X",user_data);

	if(p_file_struct->_cur_read!= NULL)
	{
		p_file_read=p_file_struct->_cur_read;
		if(p_file_read->_user_data==user_data)
		{
			p_file_read->_is_cancel=TRUE;
			canceled=TRUE;
		}
	}

	cur_node = LIST_BEGIN( p_file_struct->_read_list);
	while( cur_node != LIST_END( p_file_struct->_read_list))
	{
		p_file_read = (UFM_FILE_READ *)LIST_VALUE( cur_node );
		tmp_node =cur_node;
		cur_node = LIST_NEXT(cur_node);
		if(p_file_read->_user_data==user_data)
		{
			list_erase(&(p_file_struct->_read_list), tmp_node);
			ufm_stop_file_read(p_file_read);
			ufm_delete_file_read(p_file_read);
			canceled=TRUE;
		}
	}


	if(canceled==TRUE)
		return SUCCESS;
	else
		return UFM_ERR_READ_OP_NOT_FOUND;
	
}

_int32 ufm_stop_file_read(UFM_FILE_READ * p_file_read)
{
	//_int32 ret_val = SUCCESS;
	read_data_callback_func p2p_read_callback; 

	if(p_file_read==NULL) return SUCCESS;
	
	LOG_DEBUG("ufm_stop_file_read:0x%X",p_file_read->_user_data);
	
	p2p_read_callback=(read_data_callback_func)(p_file_read->_callback); 
	p2p_read_callback(UFM_ERR_READ_CANCELED,p_file_read->_p_data_buffer,p_file_read->_user_data);

	return SUCCESS;
	
}

_int32 ufm_delete_file_read(UFM_FILE_READ * p_file_read)
{
	//_int32 ret_val = SUCCESS;

	if(p_file_read==NULL) return SUCCESS;
	
	LOG_DEBUG("ufm_delete_file_read:0x%X",p_file_read->_user_data);
	/* Shall I release the momory ? */
	//SAFE_DELETE(p_file_read->_p_data_buffer->_data_ptr);
	//SAFE_DELETE(p_file_read->_p_data_buffer);
	mpool_free_slip(gp_ulm_file_read_slab,(void*) p_file_read);

	return SUCCESS;
	
}
void ufm_scheduler(void)
{
	UFM_FILE_STRUCT * p_file_struct = NULL;
	UFM_FILE_READ * p_file_read=NULL;
	MAP_ITERATOR cur_node , next_node;
	_u8 * map_key = NULL;
	_u32 cur_time=0;
	DATA_PIPE * p_pipe=NULL;
	
	LOG_DEBUG("ufm_scheduler:file_num=%u,dm_file_read num=%u",map_size(&(gp_ulm_file_manager->_file_structs)),map_size(&(gp_ulm_file_manager->_dm_file_read)));
	
	cur_node = MAP_BEGIN( gp_ulm_file_manager->_file_structs);

	while( cur_node != MAP_END( gp_ulm_file_manager->_file_structs ) )
	{
		p_file_struct = (UFM_FILE_STRUCT *)MAP_VALUE( cur_node );
		next_node = MAP_NEXT( gp_ulm_file_manager->_file_structs, cur_node );
		sd_time_ms(&(cur_time));
		
		if((p_file_struct->_state==UFM_OPENED)&&
			(TIME_SUBZ(cur_time, p_file_struct->_time_stamp)>gp_ulm_file_manager->_max_idle_interval))
		{
			map_key = (_u8 *)MAP_KEY( cur_node );
			map_erase_iterator(&(gp_ulm_file_manager->_file_structs), cur_node );
			
			mpool_free_slip(gp_ulm_map_key_slab,(void*) map_key);
			
			ufm_close_file_struct(p_file_struct);
		}
		cur_node = next_node;
	}

	
	cur_node = MAP_BEGIN( gp_ulm_file_manager->_dm_file_read);

	while( cur_node != MAP_END( gp_ulm_file_manager->_dm_file_read ) )
	{
		p_file_read = (UFM_FILE_READ *)MAP_VALUE( cur_node );
		next_node = MAP_NEXT( gp_ulm_file_manager->_dm_file_read, cur_node );
		sd_time_ms(&(cur_time));
		
		if((p_file_read->_time_stamp!=0)&&
			(TIME_SUBZ(cur_time, p_file_read->_time_stamp)>500))
		{
			p_pipe = (DATA_PIPE *)MAP_KEY( cur_node );
			ufm_execute_dm_file_read( p_file_read,p_pipe);
		}
		cur_node = next_node;
	}
	
	LOG_DEBUG("ufm_scheduler:success!");
}

_int32 ufm_execute_dm_file_read(UFM_FILE_READ * p_file_read,DATA_PIPE * p_pipe)
{
	_int32 ret_val = SUCCESS;
	ULM_PIPE_TYPE pipe_type;
	read_data_callback_func p2p_read_callback; 
	RANGE_DATA_BUFFER * p_data_buffer=NULL;	
		
	LOG_DEBUG("ufm_execute_dm_file_read:0x%X",p_pipe);

	sd_assert(p_file_read->_is_cancel==FALSE);
	sd_assert(p_file_read->_user_data==p_pipe);

	pipe_type = upm_get_pipe_type(p_pipe);
	if(pipe_type==ULM_D_AND_U_P2P_PIPE)
	{
		if(TRUE== dm_range_is_write((DATA_MANAGER*)(p_file_read->_data_manager_ptr),  &(p_file_read->_p_data_buffer->_range)))
		{
			/* Read data from disk */
			ret_val =   dm_asyn_read_data_buffer( (DATA_MANAGER*) (p_file_read->_data_manager_ptr), p_file_read->_p_data_buffer, ufm_data_manager_notify_file_read_result, (void *)p_pipe );
			if(ret_val!=SUCCESS)
			{
				LOG_DEBUG("ufm_execute_dm_file_read 1:0x%X error,ret_val=%d",p_pipe,ret_val);
				p2p_read_callback=(read_data_callback_func)(p_file_read->_callback);
				p_data_buffer = p_file_read->_p_data_buffer;
				map_erase_node(&(gp_ulm_file_manager->_dm_file_read), (void*)p_pipe);
				ufm_delete_file_read(p_file_read);
				p2p_read_callback(ret_val,p_data_buffer,p_pipe); 
			}
			else
			{
				LOG_DEBUG("ufm_execute_dm_file_read 2:0x%X ",p_pipe);
				p_file_read->_time_stamp=0;
			}
		}
	}
#ifdef ENABLE_BT
	else
	if(pipe_type==ULM_D_AND_U_BT_P2P_PIPE)
	{
		if(TRUE== bdm_range_is_write((struct tagBT_DATA_MANAGER*)(p_file_read->_data_manager_ptr),p_file_read->_file_index, &(p_file_read->_p_data_buffer->_range)))
		{
			/* Read data from disk */
			ret_val =   bdm_asyn_read_data_buffer( (struct tagBT_DATA_MANAGER *) (p_file_read->_data_manager_ptr), p_file_read->_p_data_buffer, ufm_data_manager_notify_file_read_result, (void *)p_pipe, p_file_read->_file_index);
			if(ret_val!=SUCCESS)
			{
				LOG_DEBUG("ufm_execute_dm_file_read 3:0x%X error,ret_val=%d",p_pipe,ret_val);
				p2p_read_callback=(read_data_callback_func)(p_file_read->_callback);
				p_data_buffer = p_file_read->_p_data_buffer;
				map_erase_node(&(gp_ulm_file_manager->_dm_file_read), (void*)p_pipe);
				ufm_delete_file_read(p_file_read);
				p2p_read_callback(ret_val,p_data_buffer,p_pipe); 
			}
			else
			{
				LOG_DEBUG("ufm_execute_dm_file_read 4:0x%X ",p_pipe);
				p_file_read->_time_stamp=0;
			}
		}
	}
#endif
	else
	{
		/* ULM_D_AND_U_BT_PIPE */
		sd_assert(FALSE);
	}

	return SUCCESS;
	
}


#endif /* UPLOAD_ENABLE */

#endif /* __ULM_FILE_MANAGER_C_20090604 */

