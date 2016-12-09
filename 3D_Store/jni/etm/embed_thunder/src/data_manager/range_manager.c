
#include "utility/mempool.h"
#include "range_manager.h"

#include "http_data_pipe/http_resource.h"
#include "ftp_data_pipe/ftp_data_pipe.h"

#include "utility/logid.h"
#define LOGID LOGID_RANGE_MANAGER
#include "utility/logger.h"

 _int32  range_map_compare(void *E1, void *E2)
{
       _u32 L = (_u32)E1;
       _u32 R = (_u32)E2;

	return L>R? 1:(L<R?-1:0 );  
}

_u32 init_range_manager(RANGE_MANAGER*  range_manage)
{
      _int32 ret_val =  SUCCESS;

      LOG_DEBUG("init_range_manager ."); 
      map_init(&range_manage->res_range_map, range_map_compare);

      return ret_val;	
}

_u32 unit_range_manager(RANGE_MANAGER*  range_manage)
{
     _int32 ret_val =  SUCCESS;
     RESOURCE* cur_resource = NULL;
     RANGE_LIST* cur_list = NULL;	 

	 MAP_ITERATOR cur_node =NULL;

     LOG_DEBUG("unit_range_manager ."); 
	   
     cur_node =  MAP_BEGIN(range_manage->res_range_map);	 
     while(cur_node != MAP_END(range_manage->res_range_map))
     {
            cur_resource = (RESOURCE*) MAP_KEY(cur_node);
	     cur_list = (RANGE_LIST*)MAP_VALUE(cur_node);	   

	     range_list_clear(cur_list);
	    // sd_free(cur_list);	 
	     free_range_list(cur_list);
		
	     map_erase_iterator(&range_manage->res_range_map, cur_node);
	     cur_node =  MAP_BEGIN(range_manage->res_range_map);	 
     }
	 
      return ret_val;		 
}


_u32 put_range_record(RANGE_MANAGER*  range_manage, RESOURCE *resource_ptr, const RANGE *r)
{
      _int32 ret_val =  SUCCESS;
	   
      MAP_ITERATOR cur_node =  NULL;
      RANGE_LIST* cur_list =  NULL;  
      RANGE_LIST* new_list = NULL;	 
      PAIR new_node;	  

	LOG_DEBUG("put_range_record res:0x%x, rang(%u,%u).", resource_ptr, r->_index, r->_num);   

      map_find_iterator(&range_manage->res_range_map, resource_ptr, &cur_node);  

       if(cur_node != MAP_END(range_manage->res_range_map))
       {
              cur_list = (RANGE_LIST*)MAP_VALUE(cur_node);	

		ret_val = range_list_add_range(cur_list, r, NULL,NULL);

		CHECK_VALUE(ret_val);
       }
	else
	{	      
//		 ret_val = sd_malloc(sizeof(RANGE_LIST), (void**)&new_list);
		 ret_val =   alloc_range_list(&new_list);
		 CHECK_VALUE(ret_val);

               range_list_init(new_list);
		 range_list_add_range(new_list, r, NULL,NULL);
		
		 new_node._key = resource_ptr;
		 new_node._value = new_list;
		 
		 ret_val = map_insert_node(&range_manage->res_range_map, &new_node);
		 
	}
	return ret_val;	
}

_u32 get_res_from_range(RANGE_MANAGER*  range_manage,  const RANGE *r, LIST* res_list)
{
         
     _int32 ret_val =  SUCCESS;
     RESOURCE* cur_resource = NULL;
     RANGE_LIST* cur_list =  NULL;	 
	     
     MAP_ITERATOR cur_node =  MAP_BEGIN(range_manage->res_range_map);  	 

     LOG_DEBUG("get_res_from_range, get range(%u,%u) reses.", r->_index, r->_num); 
	 
     while(cur_node !=  MAP_END(range_manage->res_range_map))
     {
            cur_resource = (RESOURCE*) MAP_KEY(cur_node);
	     cur_list = (RANGE_LIST*)MAP_VALUE(cur_node);	   

            if(TRUE == range_list_is_relevant(cur_list, r))
            {
                   list_push(res_list, cur_resource);   
            } 

	    cur_node =  MAP_NEXT(range_manage->res_range_map, cur_node);
     }
	 
      return ret_val;		

}

BOOL  range_is_all_from_res(RANGE_MANAGER*  range_manage,  RESOURCE *res, RANGE* r)
{      
	   
      MAP_ITERATOR cur_node =  NULL;
      RANGE_LIST* cur_list = NULL;	  

      LOG_DEBUG("range_is_all_from_res,  res:0x%x , range (%u,%u).", res, r->_index, r->_num); 

       if(r->_num == 0 || res == NULL)
       {
             return FALSE;
       }
		
      map_find_iterator(&range_manage->res_range_map, res, &cur_node);  

       if(cur_node != MAP_END(range_manage->res_range_map))
       {
              cur_list = (RANGE_LIST*)MAP_VALUE(cur_node);	

		 if(range_list_is_include(cur_list,  r) == TRUE)
		 {
		       return TRUE;
		 }
		 else
		 {
		       return FALSE;
		 }
       }	

	return FALSE;
    
}

_u32 get_range_from_res(RANGE_MANAGER*  range_manage,  RESOURCE *res, RANGE_LIST*  r_list)
{
      _int32 ret_val =  SUCCESS;
	   
      MAP_ITERATOR cur_node =  NULL;
      RANGE_LIST* cur_list = NULL;	  

      LOG_DEBUG("get_res_from_range, get res:0x%x range list.", res); 
		
      map_find_iterator(&range_manage->res_range_map, res, &cur_node);  

       if(cur_node != MAP_END(range_manage->res_range_map))
       {
              cur_list = (RANGE_LIST*)MAP_VALUE(cur_node);	

		ret_val = range_list_add_range_list(r_list, cur_list);	 
       }	

	return SUCCESS;
    
}


_u32 range_manager_erase_resource(RANGE_MANAGER*  range_manage, RESOURCE *resource_ptr)
{
        _int32 ret_val =  SUCCESS;
	   
        MAP_ITERATOR cur_node =  NULL;
	 RESOURCE* cur_resource = NULL;
	 RANGE_LIST* cur_list = NULL;

	 LOG_DEBUG("range_manager_erase_resource, erase res:0x%x .", resource_ptr); 

        map_find_iterator(&range_manage->res_range_map, resource_ptr, &cur_node);  

        if(cur_node != MAP_END(range_manage->res_range_map))
        {
             	cur_resource = (RESOURCE*) MAP_KEY(cur_node);
	       cur_list = (RANGE_LIST*)MAP_VALUE(cur_node);	   

	       range_list_clear(cur_list);

	       //sd_free(cur_list);
		free_range_list(cur_list);   

	       ret_val = map_erase_iterator(&range_manage->res_range_map, cur_node);	   

		CHECK_VALUE(ret_val);
       }

	 return ret_val;		
}

_u32 range_manager_erase_range(RANGE_MANAGER*  range_manage, const RANGE* r, RESOURCE* origin_res)
{
      _int32 ret_val =  SUCCESS;
	   
     RESOURCE* cur_resource = NULL;
     RANGE_LIST* cur_list = NULL; 
     MAP_ITERATOR cur_node =  NULL;	
     MAP_ITERATOR erase_node =  NULL;		 

     LOG_DEBUG("range_manager_erase_range, erase range(%u,%u) .", r->_index, r->_num); 		 
	 
     cur_node =  MAP_BEGIN(range_manage->res_range_map);  		 
     while(cur_node != MAP_END(range_manage->res_range_map))
     {
            cur_resource = (RESOURCE*) MAP_KEY(cur_node);
	     cur_list = (RANGE_LIST*)MAP_VALUE(cur_node);	

	      if(origin_res != NULL)
	      {
	             if(cur_resource == origin_res)
	             	{
	             	      cur_node =  MAP_NEXT(range_manage->res_range_map, cur_node);		
			      continue;			  
	             	}
	      }

	     range_list_delete_range(cur_list, r, NULL,NULL);	

	     if(cur_list->_node_size == 0)
	     {
	           range_list_clear(cur_list);

		    free_range_list(cur_list);   

		    erase_node = cur_node;

		    cur_node =  MAP_NEXT(range_manage->res_range_map, cur_node);	
			
		    map_erase_iterator(&range_manage->res_range_map, erase_node);
		   
	     }
	     else
	     {
	           cur_node =  MAP_NEXT(range_manage->res_range_map, cur_node);			           
	     }
     }
	 
      return ret_val;	
}

BOOL range_manager_has_resource(RANGE_MANAGER*  range_manage, RESOURCE *resource_ptr)
{
       
	   
        MAP_ITERATOR cur_node =  NULL;	
	 RANGE_LIST* cur_list = NULL;

	 LOG_DEBUG("range_manager_has_resource, has res:0x%x .", resource_ptr); 

        map_find_iterator(&range_manage->res_range_map, resource_ptr, &cur_node);  

        if(cur_node != MAP_END(range_manage->res_range_map))
        {
	       cur_list = (RANGE_LIST*)MAP_VALUE(cur_node);	   

              if(cur_list->_node_size == 0)
              {
                    return FALSE;
              }
		else
		{
		      return TRUE;
		}
       }
	else
	{
	      return FALSE;;		
	}
}


_int32 out_put_res_list(LIST* res_list)
{            
#ifdef _LOGGER  

         RESOURCE* res = NULL; 
	  LIST_ITERATOR cur_res = LIST_BEGIN(*res_list);
       		
	  while(cur_res != LIST_END(*res_list))
	  {
               res = LIST_VALUE(cur_res); 
               LOG_DEBUG("out_put_res_list, res: 0x%x .", res ); 
		 cur_res = LIST_NEXT(cur_res);	 
	  }
	  
#endif
	   return SUCCESS;
     
}


static _u64 range_list_get_total_length(RANGE_LIST* range_list, _u64 filesize)
{
    _u64 total_len = 0;
    RANGE_LIST_NODE*  cur_node = range_list->_head_node;

    while(cur_node != NULL)
    {
        total_len += range_to_length(&cur_node->_range, filesize);
        cur_node = cur_node->_next_node;
    }
    LOG_DEBUG("total_len = %llu", total_len);
    return total_len;
}

void range_manager_get_download_bytes(RANGE_MANAGER* range_manage
                                      , char* host
                                      , _u64* download
                                      , _u64 filesize)
{
    LOG_DEBUG("range_manager_get_download_bytes enter, host = %s, filesize = %llu", host, filesize);
    MAP_ITERATOR iter_begin = MAP_BEGIN(range_manage->res_range_map);
    MAP_ITERATOR iter_end = MAP_END(range_manage->res_range_map);
    MAP_ITERATOR iter = iter_begin;
    RESOURCE* res = NULL;
    RANGE_LIST* range_list = NULL; 
    _u64 range_bytes = 0;
    while ( iter != iter_end )
    {
        res = (RESOURCE*)MAP_KEY(iter);
        char* url_host_ptr = NULL;
        LOG_DEBUG("res_ptr = 0x%x, resource_type = %d", res, res->_resource_type);
        if(res->_resource_type == HTTP_RESOURCE)
        {
            HTTP_SERVER_RESOURCE* http_res = (HTTP_SERVER_RESOURCE*)res;
            url_host_ptr = http_res->_url._host;

        }
        else if(res->_resource_type == FTP_RESOURCE)
        {
            FTP_SERVER_RESOURCE* ftp_res = (FTP_SERVER_RESOURCE*)res;
            url_host_ptr = ftp_res->_url._host;
        }
        if(url_host_ptr != NULL)
        { 
            LOG_DEBUG("find server res, url.host = %s", url_host_ptr);
            if (sd_stristr(url_host_ptr, host, 0) != NULL)
            {
                range_list = (RANGE_LIST*)MAP_VALUE(iter);
                range_bytes += range_list_get_total_length(range_list, filesize);
            }
        }
        iter = MAP_NEXT(range_manage->res_range_map, iter);
    }    
    *download = range_bytes;
}

void out_put_res_recv_records(RANGE_MANAGER*  range_manage)
{
    LOG_DEBUG("out_put_res_recv_records enter, range_manage len = %d", map_size(&range_manage->res_range_map));
    MAP_ITERATOR iter_begin = MAP_BEGIN(range_manage->res_range_map);
    MAP_ITERATOR iter_end = MAP_END(range_manage->res_range_map);
    MAP_ITERATOR iter = iter_begin;
    RESOURCE* res = NULL;
    RANGE_LIST* range_list = NULL; 
    while ( iter != iter_end )
    {
        res = (RESOURCE*)MAP_KEY(iter);
        range_list = (RANGE_LIST*)MAP_VALUE(iter);
        LOG_DEBUG("res_ptr = 0x%x, resource_type = %d", res, res->_resource_type);
        if(res->_resource_type == HTTP_RESOURCE)
        {
            HTTP_SERVER_RESOURCE* http_res = (HTTP_SERVER_RESOURCE*)res;
            LOG_DEBUG("out_put_res_recv_records, http res.host = %s, range_list ===>", http_res->_url._host);
            out_put_range_list(range_list);
        }
        iter = MAP_NEXT(range_manage->res_range_map, iter);
    }
    
}
