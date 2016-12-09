#include "utility/errcode.h"
#include "utility/mempool.h"
#include "utility/settings.h"
#include "utility/define.h"
#include "utility/list.h"

#include "utility/logid.h"
#define LOGID LOGID_DATA_BUFFER
#include "utility/logger.h"

#include "data_buffer.h"

static _u32 MIN_BLOCK = 1;
static _u32 MAX_BLOCK = 5;
static _u32 MAX_CACHE_BUFFER_SIZE  = DATA_BUFFER_MAX_CACHE_BUFFER;
static _u32 MAX_ALLOC_BUFFER_SIZE =  DATA_BUFFER_MAX_ALLOC_BUFFER*4;

static DATA_BUFFER_POOL g_data_buffer_pool;

#ifdef _MEDIA_CENTER_TEST
static LIST g_deleted_data_buffer_list;
#endif

_int32 get_data_buffer_cfg_parameter()
{
    _int32 ret_val = SUCCESS;
    
    ret_val = settings_get_int_item("data_buffer.min_block", (_int32*)&MIN_BLOCK);
	LOG_DEBUG("get_data_buffer_cfg_parameter,  data_buffer.min_block: %u .", MIN_BLOCK);

	ret_val = settings_get_int_item("data_buffer.max_block", (_int32*)&MAX_BLOCK);
	LOG_DEBUG("get_data_buffer_cfg_parameter,  data_buffer.max_block : %u .", MAX_BLOCK);

	ret_val = settings_get_int_item("data_buffer.max_cache_buffer_size", (_int32*)&MAX_CACHE_BUFFER_SIZE);
	LOG_DEBUG("get_data_buffer_cfg_parameter,  data_buffer.max_cache_buffer_size : %u .", MAX_CACHE_BUFFER_SIZE);

	ret_val = settings_get_int_item("data_buffer.max_alloc_buffer_size", (_int32*)&MAX_ALLOC_BUFFER_SIZE);
	LOG_DEBUG("get_data_buffer_cfg_parameter,  data_buffer.max_alloc_buffer_size: %u .", MAX_ALLOC_BUFFER_SIZE);

	return ret_val;
}

_int32 initialize_data_buffer_pool(DATA_BUFFER_POOL* p_data_buffer_pool,_u32 block_size,
	          _u32 min_block, _u32 max_block, _u32 max_alloc_size, _u32 max_cache_size)
{
    _int32 block_array_num = max_block - min_block +1;
    _int32 ret_val = SUCCESS;
    _int32 i = 0;

	LOG_DEBUG("initialize_data_buffer_pool, DATA_BUFFER_POOL:0x%x,block_size:%u, min_block:%u, max_block:%u,max_alloc_size:%u, max_cache_size:%u .", p_data_buffer_pool,block_size, min_block, max_block, max_alloc_size, max_cache_size);   

    p_data_buffer_pool->m_block_size = block_size;
	p_data_buffer_pool->m_min_block = min_block;
	p_data_buffer_pool->m_max_block = max_block;
	p_data_buffer_pool->m_max_alloc_size= max_alloc_size;
	p_data_buffer_pool->m_max_cache_size= max_cache_size;
	p_data_buffer_pool->m_alloc_size = 0;	
	p_data_buffer_pool->buffer_array = 0;

    ret_val = sd_malloc(block_array_num*sizeof(BUFFER_NODE), (void **)&p_data_buffer_pool->buffer_array);   
	CHECK_VALUE(ret_val);

	for(i =0; i<block_array_num; i++)
	{
	         p_data_buffer_pool->buffer_array[i]._next = NULL;			 
	}
#ifdef _MEDIA_CENTER_TEST
	list_init(&g_deleted_data_buffer_list);
#endif

	return SUCCESS;
	    	
}


_int32 get_data_block_num(_u32 data_size, _u32 block_size)
{
       return (data_size+block_size-1)/block_size;
}

_int32 inc_alloc_size(DATA_BUFFER_POOL* p_data_buffer_pool, _u32 new_alloc_size)
{
       p_data_buffer_pool->m_alloc_size += new_alloc_size;

	return SUCCESS;
}

_int32 dec_alloc_size(DATA_BUFFER_POOL* p_data_buffer_pool, _u32 new_alloc_size)
{
       if(p_data_buffer_pool->m_alloc_size < new_alloc_size)
       {
	      p_data_buffer_pool->m_alloc_size = 0;
       }
	else   
       {
             p_data_buffer_pool->m_alloc_size -= new_alloc_size;
	}
    
	return SUCCESS;
}

BOOL buffer_is_full(DATA_BUFFER_POOL* p_data_buffer_pool)
{
       if(p_data_buffer_pool->m_max_alloc_size > p_data_buffer_pool->m_alloc_size)
       {
               LOG_DEBUG("buffer_is_full: false.  max_alloc_size:%u,alloc_size:%u",
			   	p_data_buffer_pool->m_max_alloc_size, p_data_buffer_pool->m_alloc_size );
	        return FALSE;
       }
	else
	{
	        LOG_DEBUG("buffer_is_full: true.  max_alloc_size:%u,alloc_size:%u",
			   	p_data_buffer_pool->m_max_alloc_size, p_data_buffer_pool->m_alloc_size);
	        return TRUE;
	}
}

BOOL cache_is_full(DATA_BUFFER_POOL* p_data_buffer_pool)
{
       if(p_data_buffer_pool->m_max_cache_size > p_data_buffer_pool->m_alloc_size)
       {
               LOG_DEBUG("cache_is_full: false. max_cache_size:%u, alloc_size:%u",
			   	p_data_buffer_pool->m_max_cache_size, p_data_buffer_pool->m_alloc_size ); 
	        return FALSE;
       }
	else
	{
	        LOG_DEBUG("cache_is_full: max_cache_size:%u, alloc_size:%u",
			   	p_data_buffer_pool->m_max_cache_size, p_data_buffer_pool->m_alloc_size ); 
	        return TRUE;
	}
}


_int32 unintialize_data_buffer_pool(DATA_BUFFER_POOL* p_data_buffer_pool)
{
        _int32 i = 0;
	BUFFER_NODE* p_cur_node = NULL;	
       BUFFER_NODE* p_tmp_node  = NULL;
	_int32 block_array_num = p_data_buffer_pool->m_max_block - p_data_buffer_pool->m_min_block +1;

	LOG_DEBUG("initialize_data_buffer_pool, DATA_BUFFER_POOL:0x%x .", p_data_buffer_pool);   
   
		  
	for(i =0; i<block_array_num; i++)
	{
              p_cur_node = p_data_buffer_pool->buffer_array[i]._next ;
              while(p_cur_node != NULL)
              {
                     p_tmp_node = p_cur_node;
			p_cur_node= p_cur_node->_next;
                     sd_free(p_tmp_node);	
					 p_tmp_node = NULL;
              }
			   	
		p_data_buffer_pool->buffer_array[i]._next = NULL;	 
	}

	sd_free(p_data_buffer_pool->buffer_array);
	p_data_buffer_pool->buffer_array = NULL;
#ifdef _MEDIA_CENTER_TEST
	list_clear(&g_deleted_data_buffer_list);
#endif

	return SUCCESS;
}


_int32 dm_data_buffer_init()
{
       _int32 ret_val = SUCCESS;

	LOG_DEBUG("dm_data_buffer_init.");    
       ret_val = initialize_data_buffer_pool(&g_data_buffer_pool, get_data_unit_size(), MIN_BLOCK, MAX_BLOCK
	   	                                                     , MAX_ALLOC_BUFFER_SIZE, MAX_CACHE_BUFFER_SIZE);	   
      return ret_val;

}
_int32 dm_data_buffer_unint()
{
      LOG_DEBUG("dm_data_buffer_unint.");    
      return unintialize_data_buffer_pool(&g_data_buffer_pool);
}

_u32 dm_get_buffer_from_data_buffer( _u32*  min_alloc_size, _int8** ptr_tobuffer)
{
        _u32  min_alloc_len = *min_alloc_size;
        _u32 index = 0; 

	_int32 ret_val = SUCCESS;
	 BUFFER_NODE* p_cur_node = NULL;   
	 
        _u32 block_num = get_data_block_num(min_alloc_len, g_data_buffer_pool.m_block_size);
#ifdef _MEDIA_CENTER_TEST
		void *head_buffer = NULL;
#endif
	*ptr_tobuffer = NULL;	

	 LOG_DEBUG("dm_get_buffer_from_data_buffer.");   

	 if(block_num<g_data_buffer_pool.m_min_block || block_num > g_data_buffer_pool.m_max_block)
	 {
	       LOG_ERROR("dm_get_buffer_from_data_buffer, block_num:%u is invalid.", block_num); 
	 	return ALLOC_INVALID_SIZE;
	 }
	 
	  index = block_num - g_data_buffer_pool.m_min_block;

	  p_cur_node = g_data_buffer_pool.buffer_array[index]._next ;

	  if(p_cur_node != NULL)
	  {
	         g_data_buffer_pool.buffer_array[index]._next  = p_cur_node->_next;
               
		 *ptr_tobuffer = (_int8*)p_cur_node;

		  LOG_DEBUG("dm_get_buffer_from_data_buffer.ret cached buffer:0x%x .",p_cur_node);   
#ifdef _MEDIA_CENTER_TEST
		list_pop(&g_deleted_data_buffer_list, &head_buffer );
		sd_assert(head_buffer ==(void*)*ptr_tobuffer );
		if(g_data_buffer_pool.buffer_array[index]._next== NULL)
			sd_assert(list_size(&g_deleted_data_buffer_list)==0);
#endif
	  } 
	  else
	  {
	           if(buffer_is_full(&g_data_buffer_pool) == TRUE)
	           {
	                  LOG_ERROR("dm_get_buffer_from_data_buffer, cache is full block_num:%u can not alloc.", block_num); 
					  
	                 return DATA_BUFFER_IS_FULL;
	           }
			  
                  ret_val = sd_malloc(block_num*(g_data_buffer_pool.m_block_size), (void**)ptr_tobuffer);   
                  if(ret_val != SUCCESS)
                  {
                        LOG_ERROR("dm_get_buffer_from_data_buffer, block_num:%u blocks alloc failure.", block_num); 
			   return ret_val;	  
                  }
		   LOG_DEBUG("dm_get_buffer_from_data_buffer.malloc buffer:0x%x, block_num:%u .", *ptr_tobuffer, block_num); 			  

		     inc_alloc_size(&g_data_buffer_pool, (block_num*(g_data_buffer_pool.m_block_size)));			  
	  }

	 *min_alloc_size = block_num*g_data_buffer_pool.m_block_size;

	LOG_DEBUG("dm_get_buffer_from_data_buffer.ret buffer:0x%x, length:%u .", *ptr_tobuffer, *min_alloc_size);   
	
        return SUCCESS;   		
}

_u32 dm_free_buffer_to_data_buffer(_u32 data_size,_int8**  ptr_to_buffer )
{
        _u32 index = 0; 
	BUFFER_NODE* p_cur_node  = NULL;	
        _u32 block_num = get_data_block_num(data_size, g_data_buffer_pool.m_block_size);	
#ifdef _MEDIA_CENTER_TEST
		LIST_ITERATOR cur_node = NULL;
#endif

        LOG_DEBUG("dm_free_buffer_to_data_buffer.");   
 
	 if(block_num<g_data_buffer_pool.m_min_block || block_num > g_data_buffer_pool.m_max_block)
	 {
	       LOG_ERROR("dm_free_buffer_to_data_buffer, block_num:%u is invalid.", block_num); 
	 	return ALLOC_INVALID_SIZE;
	 }
	 if(cache_is_full(&g_data_buffer_pool) == TRUE)
	 {
	        LOG_DEBUG("dm_free_buffer_to_data_buffer.  buffer:0x%x  length:%u block_num:%u is free os..",*ptr_to_buffer, data_size, block_num);   
 
	        sd_free(*ptr_to_buffer);
			*ptr_to_buffer = NULL;
	
		 dec_alloc_size(&g_data_buffer_pool, (block_num*(g_data_buffer_pool.m_block_size)));	
	 }
	 else
	 {
	        index = block_num - g_data_buffer_pool.m_min_block;

	        p_cur_node = g_data_buffer_pool.buffer_array[index]._next ;

	        ((BUFFER_NODE* )(*ptr_to_buffer))->_next =  p_cur_node; 
		 g_data_buffer_pool.buffer_array[index]._next  = (BUFFER_NODE* )(*ptr_to_buffer);	
#ifdef _MEDIA_CENTER_TEST
    	cur_node = LIST_BEGIN(g_deleted_data_buffer_list);
		list_insert(&g_deleted_data_buffer_list, *ptr_to_buffer, cur_node);
#endif

		  LOG_DEBUG("dm_free_buffer_to_data_buffer.  buffer:0x%x  length:%u block_num:%u is free to cache..",*ptr_to_buffer, data_size, block_num);   
	 }

	 *ptr_to_buffer = NULL;	 
        return SUCCESS;   
}


