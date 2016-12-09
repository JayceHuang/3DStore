#if !defined(__HTTP_DATA_PROCESS_C_20090305)
#define __HTTP_DATA_PROCESS_C_20090305

#include "platform/sd_socket.h"
#include "utility/settings.h"
#include "utility/base64.h"
#include "utility/sd_iconv.h"
#include "p2sp_download/p2sp_task.h"
#include "platform/sd_network.h"
#include "utility/logid.h"
#define LOGID LOGID_HTTP_PIPE
#include "utility/logger.h"

#include "http_data_pipe.h"
#include "download_task/download_task.h"
#include "socket_proxy_interface.h"

 _int32 http_pipe_get_request_range( struct tag_HTTP_DATA_PIPE * p_http_pipe ,_u64 * p_start_pos,_u64 * p_end_pos)
{
	RANGE   uncom_head_range,download_range;
	_u64 _start_pos ,_end_pos= 0,_file_size=0;
	_int32 ret_val=SUCCESS;
	LOG_DEBUG( " enter[0x%X] http_pipe_get_request_range()...",p_http_pipe );
	
	ret_val=dp_get_uncomplete_ranges_head_range( &(p_http_pipe->_data_pipe), &uncom_head_range );
	CHECK_VALUE(ret_val);
	
	LOG_DEBUG( " [0x%X] uncom_head_range._index=%u,uncom_head_range._num=%u",p_http_pipe,uncom_head_range._index,uncom_head_range._num );
	if( 0!=uncom_head_range._num)
	{
			/* Save a copy of uncom_head_range */
			p_http_pipe->_data._request_range._index = uncom_head_range._index;
			p_http_pipe->_data._request_range._num = uncom_head_range._num;
			
			/* Get the start index of this request */
			download_range._index=uncom_head_range._index;

			/* Get the start point of this request */
			_start_pos = get_data_pos_from_index(download_range._index);

			/* Get the total file size */
			_file_size =http_pipe_get_file_size(p_http_pipe);

			LOG_DEBUG( " [0x%X] _start_pos=%llu,_file_size=%llu",p_http_pipe,_start_pos,_file_size );
			/* Check if the start point is valid */
			if((_file_size!=0)&&(_file_size<=_start_pos) && !p_http_pipe->_p_http_server_resource->_relation_res)
			{
				LOG_DEBUG( " [0x%X] HTTP_ERR_INVALID_RANGES" );
				return  HTTP_ERR_INVALID_RANGES;
			}

			/* Get the end point of this request */
			if((uncom_head_range._index==0)&&(uncom_head_range._num==MAX_U32))
			{
				/* Download the whole file  */
				_end_pos = -1;
			}
			else
			{
				/* Not whole file,get the correct end point  */
				_end_pos=_start_pos+range_to_length(&uncom_head_range,  _file_size);

				/* Check if the end point is valid */
				if(_file_size<=_end_pos && !p_http_pipe->_p_http_server_resource->_relation_res)
				{
					/* Download the whole file  */
					_end_pos = -1;
				}

			}

			if(HTTP_RESOURCE_IS_SUPPORT_RANGE(p_http_pipe->_p_http_server_resource)==FALSE)
				_end_pos = -1;
			if(p_http_pipe->_p_http_server_resource->_relation_res)
			{
				
				//sd_assert(_end_pos != -1);

				if(_end_pos == -1)
				{
					LOG_DEBUG( " [0x%X] HTTP_ERR_INVALID_RANGES _end_pos == -1 but pipe is relation pipe", p_http_pipe);
					return HTTP_ERR_INVALID_RANGES;
				}
			}

			/* Get the length of data need to download of this request */
			p_http_pipe->_data._content_length = range_to_length(&uncom_head_range,  _file_size);
			p_http_pipe->_b_ranges_changed =FALSE;
			p_http_pipe->_data._recv_end_pos = 0;

#if defined(MOBILE_PHONE)
			if(NT_GPRS_WAP & sd_get_net_type())
			{
				_u32 max_cmwap_range = MAX_CMWAP_RANGE;
				settings_get_int_item( "system.max_cmwap_range",(_int32*)&max_cmwap_range);
				max_cmwap_range=max_cmwap_range*(get_data_unit_size());
				if((_end_pos==-1)||(_end_pos-_start_pos>max_cmwap_range))
				{
					_end_pos = _start_pos+max_cmwap_range;
					if((_file_size!=0)&&(_end_pos>_file_size))
					{
						_end_pos = _file_size;
					}
						
					if(p_http_pipe->_data._content_length>_end_pos - _start_pos)
					{
						p_http_pipe->_data._content_length = _end_pos - _start_pos ;
					}
				}
			}
#endif		

			p_http_pipe->_req_end_pos = _end_pos;
			
			/* Get the first piece of range number to receive of this request */
			if(uncom_head_range._num>http_get_every_time_reveive_range_num())
				download_range._num= http_get_every_time_reveive_range_num();
			else
				download_range._num = uncom_head_range._num;
			
		LOG_DEBUG( "_p_http_pipe=0x%X: uncom_head_range->_index=%u,uncom_head_range->_num=%u,_down_range._index=%u,_down_range._unm=%u,_start_pos=%llu,_end_pos=%llu,_file_size=%llu,_data._content_length=%llu,_data._recv_end_pos=%llu,_req_end_pos=%llu"
			,p_http_pipe,uncom_head_range._index,uncom_head_range._num,download_range._index,download_range._num,_start_pos,_end_pos,_file_size,p_http_pipe->_data._content_length,p_http_pipe->_data._recv_end_pos,p_http_pipe->_req_end_pos);

			*p_start_pos=_start_pos;
			*p_end_pos	=_end_pos;
			if(p_http_pipe->_p_http_server_resource->_relation_res)
			{	
				_u64 relation_pos = 0;
				_int32 ret =pt_origion_pos_to_relation_pos(p_http_pipe->_p_http_server_resource->_p_block_info_arr, 
					p_http_pipe->_p_http_server_resource->_block_info_num, _start_pos, &relation_pos);

				if(ret != SUCCESS)
				{
					return HTTP_ERR_RELATION_RES_NOT_SUPPORT_RANGE;
				}

				*p_start_pos = 	relation_pos;
				*p_end_pos  = relation_pos + _end_pos - _start_pos;
			}
			ret_val=dp_set_download_range( &(p_http_pipe->_data_pipe), &download_range );
			CHECK_VALUE(ret_val);
	}
	else
	{
		/* Error */
		LOG_DEBUG( "_p_http_pipe=0x%X:HTTP_ERR_INVALID_RANGES",p_http_pipe);
		return HTTP_ERR_INVALID_RANGES;
	}
	return SUCCESS;
}
 _int32 http_pipe_get_down_range( struct tag_HTTP_DATA_PIPE * p_http_pipe )
{
	RANGE   uncom_head_range,download_range;
	_u64 _start_pos ,_file_size=0;
	_int32 ret_val=SUCCESS;
    RANGE_LIST_NODE *p_can_rlst_node = NULL;
    RANGE cmplte_rge;

	LOG_DEBUG( "_p_http_pipe=0x%X:http_pipe_get_down_range",p_http_pipe);
	
	ret_val=dp_get_uncomplete_ranges_head_range( &(p_http_pipe->_data_pipe), &uncom_head_range );
	CHECK_VALUE(ret_val);
	
	if( 0 != uncom_head_range._num)
	{
		/* Get the start index of this request */
		download_range._index = uncom_head_range._index;

		/* Get the start point of this request */
		_start_pos = get_data_pos_from_index(download_range._index);

		/* Get the total file size */
		_file_size =http_pipe_get_file_size(p_http_pipe);

		/* Check if the start point is valid */
		if((_file_size!=0)&&(_file_size<_start_pos))
		{
			LOG_DEBUG( "_p_http_pipe=0x%X:HTTP_ERR_INVALID_RANGES:_file_size(%llu)<_start_pos(%llu)",p_http_pipe,_file_size,_start_pos);
			return  HTTP_ERR_INVALID_RANGES;
		}

		LOG_DEBUG( "_p_http_pipe=0x%X: uncom_head_range->_index=%u,\
                   uncom_head_range->_num=%u,http_get_every_time_reveive_range_num()=%u"
			      , p_http_pipe
                  , uncom_head_range._index
                  , uncom_head_range._num
                  , http_get_every_time_reveive_range_num());

		/* Get the first piece of range number to receive of this request */
		if(uncom_head_range._num > http_get_every_time_reveive_range_num())
		{
			download_range._num = http_get_every_time_reveive_range_num();
		}
		else
		{
			download_range._num = uncom_head_range._num;

            // 判断如果这个pipe不支持range，不要再请求分块调度了，直接返回后面的range让它去下载
            // 否则，可能会导致这个pipe重头下载
            // 2012.11.30 lgq
            if (NULL != p_http_pipe) {
                if (HTTP_RESOURCE_IS_SUPPORT_RANGE(p_http_pipe->_p_http_server_resource)
                    == TRUE) {
                    LOG_DEBUG("_p_http_pipe=0x%x: should request to dispatch again."
                        , p_http_pipe);
                    pi_notify_dispatch_data_finish((DATA_PIPE *)p_http_pipe);
                } else {
                    LOG_DEBUG("_p_http_pipe=0x%x: don't support range so give all \
                              can download range to it.", p_http_pipe);

                    LOG_DEBUG("_p_http_pipe = 0x%x, can_download_ranges is: ", p_http_pipe);
                    //my_out_put_range_list(
                    //    &p_http_pipe->_data_pipe._dispatch_info._can_download_ranges);

                    LOG_DEBUG("_p_http_pipe = 0x%x, uncomplete_ranges before add is: ", p_http_pipe);
                    //my_out_put_range_list(
                    //    &p_http_pipe->_data_pipe._dispatch_info._uncomplete_ranges);

                    RANGE_LIST tmp_can_rglst;
                    range_list_init(&tmp_can_rglst);

                    range_list_cp_range_list(&p_http_pipe->_data_pipe._dispatch_info._can_download_ranges
                        , &tmp_can_rglst);

                    // 需要减去已经下载完成的                    
                    cmplte_rge._index = 0;
                    cmplte_rge._num = download_range._index;
                    //my_out_put_range_list(&tmp_can_rglst);
                    range_list_delete_range(&tmp_can_rglst, &cmplte_rge, NULL, NULL);
                    //my_out_put_range_list(&tmp_can_rglst);

                    // 加入到uncomplete range中去
                    p_can_rlst_node 
                        = tmp_can_rglst._head_node;
                    while (p_can_rlst_node != NULL) {
                        dp_add_uncomplete_ranges((DATA_PIPE *)p_http_pipe, 
                            &p_can_rlst_node->_range);
                        p_can_rlst_node = p_can_rlst_node->_next_node;
                    }

                    LOG_DEBUG("_p_http_pipe = 0x%x, uncomplete_ranges after add is: ", p_http_pipe);
                    //my_out_put_range_list(
                    //    &p_http_pipe->_data_pipe._dispatch_info._uncomplete_ranges);

                    range_list_clear(&tmp_can_rglst);
                }
            }
		}
			
		LOG_DEBUG( "_p_http_pipe=0x%X: uncom_head_range->_index=%u,\
                   uncom_head_range->_num=%u,_down_range._index=%u,\
                   _down_range._unm=%u,_start_pos=%llu,_file_size=%llu,\
                   _data._content_length=%llu,_data._recv_end_pos=%llu,_req_end_pos=%llu"
			      , p_http_pipe
                  , uncom_head_range._index
                  , uncom_head_range._num
                  , download_range._index
                  , download_range._num
                  , _start_pos
                  , _file_size
                  , p_http_pipe->_data._content_length
                  , p_http_pipe->_data._recv_end_pos
                  , p_http_pipe->_req_end_pos);

		ret_val=dp_set_download_range( &(p_http_pipe->_data_pipe), &download_range );
		CHECK_VALUE(ret_val);
	}
	else
	{
		/* Error */
		download_range._index=0;
		download_range._num=0;
		LOG_DEBUG( "_p_http_pipe=0x%X:http_pipe_get_down_range, but no range any more!!",p_http_pipe);
		ret_val=dp_set_download_range( &(p_http_pipe->_data_pipe), &download_range );
		CHECK_VALUE(ret_val);
	}

	return SUCCESS;
}


  _int32 http_pipe_parse_file_content( struct tag_HTTP_DATA_PIPE * p_http_pipe )
{
	char body_header_buffer[255];
	LOG_DEBUG( "enter[0x%X] http_pipe_parse_file_content...,header._is_binary_file=%d,_parse_file_content=%d",p_http_pipe,p_http_pipe->_header._is_binary_file,p_http_pipe->_data._parse_file_content);

	sd_assert(p_http_pipe->_data._data_buffer!=NULL);

	if((p_http_pipe->_data._parse_file_content==FALSE)&&(p_http_pipe->_header._is_binary_file==FALSE)&&
		(http_pipe_is_binary_file( p_http_pipe )==TRUE))
	{
		LOG_DEBUG( "http_pipe[0x%X] The first range,parse the file content to check if the file is a html file ",p_http_pipe);
		sd_memset(body_header_buffer,0,255);
		if(p_http_pipe->_data._data_buffer_end_pos>254)
		{
			sd_memcpy(body_header_buffer,p_http_pipe->_data._data_buffer,254);
		}
		else
		{
			sd_memcpy(body_header_buffer,p_http_pipe->_data._data_buffer,p_http_pipe->_data._data_buffer_end_pos);
		}
		
		if((NULL!=sd_strstr(body_header_buffer, HTTP_HTML_HEADER_UP_CASE, 0))
		||(NULL!=sd_strstr(body_header_buffer, HTTP_HTML_HEADER_LOW_CASE, 0))
		||(NULL!=sd_strstr(body_header_buffer, "<html>\r\n<head>\r\n", 0)))
		{
			/* Wrong file content */
			LOG_DEBUG( " http_pipe[0x%X] Wrong file content:html file:\n%s",p_http_pipe,body_header_buffer);
			return HTTP_ERR_INVALID_SERVER_FILE_SIZE;
		}
		
		p_http_pipe->_data._parse_file_content=TRUE;
	}
	
	LOG_DEBUG( " http_pipe[0x%X]Parse body..._data_buffer_end_pos=%u,_data_length=%u",p_http_pipe,p_http_pipe->_data._data_buffer_end_pos,p_http_pipe->_data._data_length);
	LOG_DEBUG( " http_pipe[0x%X]Parse body..._is_chunked=%d,_has_content_length=%d,_b_received_0_byte=%d",p_http_pipe,p_http_pipe->_header._is_chunked,p_http_pipe->_header._has_content_length,p_http_pipe->_data._b_received_0_byte);
	if(p_http_pipe->_header._is_chunked == TRUE)
	{
		return http_pipe_parse_chunked_data( p_http_pipe );
	}
	else
	if((p_http_pipe->_data._data_buffer_end_pos == p_http_pipe->_data._data_length)
		||(p_http_pipe->_data._recv_end_pos==p_http_pipe->_data._content_length)
		||((p_http_pipe->_data._b_received_0_byte ==TRUE)&&(p_http_pipe->_header._has_content_length==FALSE)))
	{
		/* range succss */
		return http_pipe_down_range_success( p_http_pipe);
	}
	
	return SUCCESS;

}

  _int32 http_pipe_parse_body( struct tag_HTTP_DATA_PIPE * p_http_pipe )
{
	_int32 ret_val=SUCCESS;
	LOG_DEBUG( "enter[0x%X] http_pipe_parse_body..._temp_data_buffer_length=%u",p_http_pipe,p_http_pipe->_data._temp_data_buffer_length);
	if( p_http_pipe->_data._temp_data_buffer_length !=0 )
	{
	
		ret_val =  http_pipe_get_buffer(  p_http_pipe );
		 CHECK_VALUE(ret_val);

		if(TRUE!=p_http_pipe->_b_data_full)
		{
		 	ret_val =  http_pipe_store_temp_data_buffer( p_http_pipe );
			if(ret_val!=SUCCESS) return ret_val;
		}
		else
			return SUCCESS;
	}

	if(p_http_pipe->_data._data_buffer!=NULL)
	{
		return http_pipe_parse_file_content(  p_http_pipe );
	}
	
	return SUCCESS;
}
 
_int32 http_pipe_set_file_size( struct tag_HTTP_DATA_PIPE * p_http_pipe ,_u64 result )
{
	_u64 resource_file_size = 0,file_size_from_data_manager =0;
	_int32 ret_val=SUCCESS;
	char body_header_buffer[255];
	LOG_DEBUG( "enter[0x%X] http_pipe_set_file_size=%llu,is_origin=%d",p_http_pipe,result,HTTP_RESOURCE_IS_ORIGIN(p_http_pipe->_p_http_server_resource));


	if(result == 0 && !HTTP_RESOURCE_IS_ORIGIN(p_http_pipe->_p_http_server_resource))
	{
		return HTTP_ERR_INVALID_SERVER_FILE_SIZE;
	}

    // 这里返回文件大小0非法
    if( result == 0 && HTTP_RESOURCE_IS_ORIGIN(p_http_pipe->_p_http_server_resource)
        && !p_http_pipe->_p_http_server_resource->_is_chunked )
	{
	    ret_val = pi_set_file_size( (DATA_PIPE *)p_http_pipe,result);
		return HTTP_ERR_INVALID_SERVER_FILE_SIZE;
	}
	
	resource_file_size = HTTP_RESOURCE_GET_FILE_SIZE(p_http_pipe->_p_http_server_resource);
	
	if((resource_file_size == 0 ) || (resource_file_size!=result) )
	{
		//HTTP_RESOURCE_SET_FILE_SIZE(p_http_pipe->_p_http_server_resource, result);

		if(p_http_pipe->_p_http_server_resource->_relation_res)
		{
			LOG_DEBUG( "enter[0x%X] http_pipe_set_file_size=%llu,iis a relation resource , cant set orgion filesize",p_http_pipe);
			return SUCCESS;
		}
    		
		file_size_from_data_manager = pi_get_file_size( (DATA_PIPE *)p_http_pipe);
								
		LOG_DEBUG( " http_pipe[0x%X] resource->_file_size == 0>>>>>result=%llu,_file_size_from_data_manager=%llu,_b_is_origin=%d",p_http_pipe,result,file_size_from_data_manager,HTTP_RESOURCE_IS_ORIGIN(p_http_pipe->_p_http_server_resource));

#ifdef _DEBUG
		LOG_DEBUG( " http_pipe[0x%X] +++ Get file size=%llu from resource:%s,is_origin=%d,lixian=%d",p_http_pipe,result,p_http_pipe->_p_http_server_resource->_o_url,HTTP_RESOURCE_IS_ORIGIN(p_http_pipe->_p_http_server_resource),p_http_pipe->_p_http_server_resource->_lixian);
#endif

        if(file_size_from_data_manager != result)
        {
            if( HTTP_RESOURCE_IS_ORIGIN(p_http_pipe->_p_http_server_resource) || p_http_pipe->_p_http_server_resource->_lixian)
            {
                HTTP_RESOURCE_SET_FILE_SIZE(p_http_pipe->_p_http_server_resource, result);
                /* Check the file type*/
                if(http_pipe_is_binary_file( p_http_pipe )==TRUE)	
                {
                    if(p_http_pipe->_header._is_binary_file==FALSE)
                    {
                        sd_memset(body_header_buffer,0,255);
                        if((p_http_pipe->_data._temp_data_buffer!=NULL)&&(p_http_pipe->_data._temp_data_buffer_length>sd_strlen(HTTP_HTML_HEADER_UP_CASE)))
                        {
                            sd_strncpy(body_header_buffer,p_http_pipe->_data._temp_data_buffer,254);
                        }
                        else if((p_http_pipe->_data._data_buffer!=NULL)&&(p_http_pipe->_data._data_buffer_end_pos>sd_strlen(HTTP_HTML_HEADER_UP_CASE)))
                        {
                            sd_strncpy(body_header_buffer,p_http_pipe->_data._data_buffer,254);
                        }

                        /* Check if the file is a html file */
                        if(body_header_buffer[0]!='\0')
                        {
                            if((NULL!=sd_strstr(body_header_buffer, HTTP_HTML_HEADER_UP_CASE, 0))
                                ||(NULL!=sd_strstr(body_header_buffer, HTTP_HTML_HEADER_LOW_CASE, 0))
                                ||(NULL!=sd_strstr(body_header_buffer, "<html>\r\n<head>\r\n", 0)))
                            {
                                /* Wrong file content */
                                LOG_DEBUG( " http_pipe[0x%X] Wrong file content:html file:\n%s",p_http_pipe,body_header_buffer);
                                return HTTP_ERR_INVALID_SERVER_FILE_SIZE;
                            }
                        }

                    }
                }

                /* Check if the file size is ok */
                if((file_size_from_data_manager>10*1024)&&(result<1024))
                {
                    LOG_DEBUG( " http_pipe[0x%X] the file size=%llu from origin resource is too small !file_size_from_data_manager=%llu,UNACCEPTABLE !!\n",p_http_pipe,result,file_size_from_data_manager);
                    return HTTP_ERR_INVALID_SERVER_FILE_SIZE;
                }

                LOG_DEBUG( " http_pipe[0x%X] call dm_set_file_size:%llu",p_http_pipe,result);
                ret_val = pi_set_file_size( (DATA_PIPE *)p_http_pipe, result);
                if(ret_val!=SUCCESS)
                {
                    if(ret_val==FILE_SIZE_NOT_BELIEVE)
                    {
                        /* Start timer to wait dm_set_file_size */
                        p_http_pipe->_b_retry_set_file_size = TRUE;
                        /* Start timer */
                        LOG_DEBUG( " http_pipe[0x%X] FILE_SIZE_NOT_BELIEVE ;call START_TIMER to wait dm_set_file_size",p_http_pipe);
                        ret_val = start_timer(http_pipe_handle_retry_set_file_size_timeout, NOTICE_ONCE,SET_FILE_SIZE_WAIT_M_SEC,0,  p_http_pipe,&(p_http_pipe->_retry_set_file_size_timer_id));
                        CHECK_VALUE(ret_val);
                        return SUCCESS;
                    }
                    else
                    {
                        LOG_DEBUG( " http_pipe[0x%X] HTTP_ERR_INVALID_SERVER_FILE_SIZE" ,p_http_pipe);
                        return HTTP_ERR_INVALID_SERVER_FILE_SIZE;
                    }
                }
                else
                {
                    if((p_http_pipe->_b_get_file_size_after_data==TRUE)&&(p_http_pipe->_b_set_request==FALSE))
                    {
                        pt_get_file_size_after_data((TASK *) dp_get_task_ptr( (DATA_PIPE *)p_http_pipe ));
                    }
                }

            }
            else
            {
                LOG_DEBUG( " http_pipe[0x%X] HTTP_ERR_INVALID_SERVER_FILE_SIZE",p_http_pipe);
                return HTTP_ERR_INVALID_SERVER_FILE_SIZE;
            }
        }
        else
        {
            HTTP_RESOURCE_SET_FILE_SIZE(p_http_pipe->_p_http_server_resource, result);
        }

	}

	if(p_http_pipe->_p_http_server_resource->_relation_res)
	{
		LOG_DEBUG( "enter[0x%X] http_pipe_set_file_size=%llu,iis a relation resource22 , cant set orgion filesize",p_http_pipe);
		return SUCCESS;
	}

	/* Set the _data_pipe._dispatch_info._can_download_ranges ! */
	ret_val =http_pipe_set_can_download_ranges( p_http_pipe,result );
	
	return ret_val;
}

_u64 http_pipe_get_file_size( struct tag_HTTP_DATA_PIPE * p_http_pipe )
{
	_u64 file_size=0;
	if(!p_http_pipe->_p_http_server_resource->_relation_res && HTTP_RESOURCE_GET_FILE_SIZE(p_http_pipe->_p_http_server_resource)!=0)
	{
		file_size= HTTP_RESOURCE_GET_FILE_SIZE(p_http_pipe->_p_http_server_resource);
		LOG_DEBUG( " enter[0x%X] http_pipe_get_file_size from server resource =%llu",p_http_pipe,file_size );
	}
	else
	{
		file_size= pi_get_file_size( (DATA_PIPE *)p_http_pipe);
		LOG_DEBUG( " enter[0x%X] http_pipe_get_file_size from data_manager =%llu",p_http_pipe,file_size );
	}
	return file_size;
}


 _int32 http_pipe_get_buffer( struct tag_HTTP_DATA_PIPE * p_http_pipe )
{
	_u64 file_size =0;
	_int32 ret_val=SUCCESS;
	RANGE download_range;
	LOG_DEBUG( " enter[0x%X] http_pipe_get_buffer(_content_length=%llu,_recv_end_pos=%llu)...",p_http_pipe,p_http_pipe->_data._content_length,p_http_pipe->_data._recv_end_pos );

	sd_assert(p_http_pipe->_data._content_length != 0);
	
	file_size = http_pipe_get_file_size( p_http_pipe );
	ret_val=dp_get_download_range( &(p_http_pipe->_data_pipe), &download_range);
	CHECK_VALUE(ret_val);
	
	sd_assert(download_range._num!= 0);
	
	p_http_pipe->_data._data_length = range_to_length(&download_range,  file_size);
		
	if((p_http_pipe->_data._content_length-p_http_pipe->_data._recv_end_pos) < p_http_pipe->_data._data_length)
	{
		LOG_DEBUG( "  http_pipe[0x%X]The last ranges ",p_http_pipe);
		p_http_pipe->_data._data_length = p_http_pipe->_data._content_length-p_http_pipe->_data._recv_end_pos;
	}

	/* The length of the data buffer should be getten from data_manager */
	p_http_pipe->_data._data_buffer_length =p_http_pipe->_data._data_length   ;
	p_http_pipe->_data._data_buffer_end_pos = 0;

	/* Get buffer from data manager,maybe the _data_buffer_length will be longer than _data_length */
	LOG_DEBUG( "  http_pipe[0x%X] DM_GET_DATA_BUFFER(_p_data_manager=0x%X,_buffer=0x%X,&(_buffer)=0x%X,_buffer_length[_index=%u,_num=%u]=%u)..." ,
			p_http_pipe,p_http_pipe->_data_pipe._p_data_manager,p_http_pipe->_data._data_buffer,&(p_http_pipe->_data._data_buffer),download_range._index,download_range._num,p_http_pipe->_data._data_buffer_length);

	sd_assert(p_http_pipe->_data._data_buffer == NULL);

	ret_val=pi_get_data_buffer( (DATA_PIPE *) p_http_pipe, &(p_http_pipe->_data._data_buffer),&( p_http_pipe->_data._data_buffer_length));
	if(ret_val!=SUCCESS)
	{
		sd_assert(p_http_pipe->_b_data_full!=TRUE);

		p_http_pipe->_b_data_full = TRUE;
		pipe_set_err_get_buffer( &(p_http_pipe->_data_pipe), TRUE );
		p_http_pipe->_get_buffer_try_count = 0;
		/* Start timer */
		LOG_DEBUG( "  http_pipe[0x%X] get buffer failed!call START_retry_get_buffer_timer_TIMER",p_http_pipe);
		ret_val= start_timer(http_pipe_handle_retry_get_buffer_timeout, NOTICE_ONCE,http_pipe_get_buffer_wait_timeout(p_http_pipe), 0, p_http_pipe,&(p_http_pipe->_retry_get_buffer_timer_id)); 
		CHECK_VALUE(ret_val);
	}
		
	return SUCCESS;
		
}
 _int32 http_pipe_store_temp_data_buffer( struct tag_HTTP_DATA_PIPE * p_http_pipe )
{
	_int32 ret_val=SUCCESS;
	LOG_DEBUG( "  http_pipe[0x%X]enter http_pipe_store_temp_data_buffer(_is_chunked=%d)..." ,p_http_pipe,p_http_pipe->_header._is_chunked);

	LOG_DEBUG( "  http_pipe[0x%X]:_recv_end_pos=%llu,_temp_data_buffer_length =%u ,_content_length=%llu,_data_buffer_end_pos=%u,_data_length =%u ",
		p_http_pipe,p_http_pipe->_data._recv_end_pos,p_http_pipe->_data._temp_data_buffer_length,p_http_pipe->_data._content_length,p_http_pipe->_data._data_buffer_end_pos,p_http_pipe->_data._data_length );

	if(p_http_pipe->_header._is_chunked!=TRUE)
	{

		if(( (p_http_pipe->_data._recv_end_pos+p_http_pipe->_data._temp_data_buffer_length) <=p_http_pipe->_data._content_length)
			&&((p_http_pipe->_data._temp_data_buffer_length+p_http_pipe->_data._data_buffer_end_pos)<=p_http_pipe->_data._data_length))
		{
			if((p_http_pipe->_data._temp_data_buffer!=NULL )&&(p_http_pipe->_data._data_buffer!=NULL))
			{
				sd_memcpy(p_http_pipe->_data._data_buffer+p_http_pipe->_data._data_buffer_end_pos,p_http_pipe->_data._temp_data_buffer,p_http_pipe->_data._temp_data_buffer_length);
				p_http_pipe->_data._data_buffer_end_pos+=p_http_pipe->_data._temp_data_buffer_length;

				sd_assert(p_http_pipe->_data._recv_end_pos == 0);
				
				if(p_http_pipe->_data._recv_end_pos == 0)
					p_http_pipe->_data._recv_end_pos+=p_http_pipe->_data._temp_data_buffer_length;
				
				p_http_pipe->_data._temp_data_buffer_length = 0;

				return SUCCESS;
			}
		}
		
		return HTTP_ERR_BAD_TEMP_BUFFER;
	}
	else
	{
		ret_val = http_pipe_parse_chunked_header( p_http_pipe ,p_http_pipe->_data._temp_data_buffer,p_http_pipe->_data._temp_data_buffer_length);
		if(ret_val!=SUCCESS) return ret_val;
		p_http_pipe->_data._temp_data_buffer_length = 0;
	}
	return SUCCESS;
}
 BOOL http_pipe_check_can_put_data( struct tag_HTTP_DATA_PIPE * p_http_pipe )
{
	LOG_DEBUG( " enter[0x%X]  http_pipe_check_can_put_data...",p_http_pipe);
	if(FALSE==HTTP_RESOURCE_IS_ORIGIN(p_http_pipe->_p_http_server_resource)) return TRUE;

	if(TRUE==p_http_pipe->_b_set_request) return TRUE;
	
	if(COMMON_PIPE_INTERFACE_TYPE!=pi_get_pipe_interface_type( (DATA_PIPE *)p_http_pipe ))
	{
		sd_assert(FALSE);
		return TRUE;
	}

	if(TRUE== pt_is_shub_ok((TASK *) dp_get_task_ptr( (DATA_PIPE *)p_http_pipe ))) return TRUE;

	if(p_http_pipe->_data._data_length>=HTTP_DEFAULT_RANGE_LEN) return TRUE;

	LOG_DEBUG( " enter[0x%X]  http_pipe_check_can_put_data...return FALSE!",p_http_pipe);
	return FALSE;
}
 
 _int32 http_pipe_abort_put_data( struct tag_HTTP_DATA_PIPE * p_http_pipe )
{
	LOG_DEBUG( " enter[0x%X]  http_pipe_abort_put_data...reconnect the server until the shub return the real file size!",p_http_pipe);
	HTTP_SET_STATE(HTTP_PIPE_STATE_RANGE_CHANGED_NEED_RECONNECT);  /* 权宜之计... */
	http_reset_respn_header( &(p_http_pipe->_header) );
	http_pipe_reset_respn_data(p_http_pipe, &(p_http_pipe->_data) );	
	return SUCCESS;
}

_int32 http_pipe_down_range_success( struct tag_HTTP_DATA_PIPE * p_http_pipe)
{
	_int32 ret_val = SUCCESS;
	_u64 received_pos = 0;
	RANGE download_range;
	ret_val=dp_get_download_range( &(p_http_pipe->_data_pipe), &download_range);
	CHECK_VALUE(ret_val);
	LOG_DEBUG( " enter[0x%X]  http_pipe_down_range_success(_index=%u,_num=%u)...",p_http_pipe,download_range._index,download_range._num);

	if(p_http_pipe->_data._data_buffer == NULL) return HTTP_ERR_DATA_FULL;
	sd_assert(p_http_pipe->_data._data_buffer != NULL);
	
	LOG_DEBUG( "  http_pipe[0x%X]_b_received_0_byte=%d,_has_content_length=%d,_data._content_length=%llu,_data._recv_end_pos=%llu,_is_chunked=%d",
		p_http_pipe,p_http_pipe->_data._b_received_0_byte,p_http_pipe->_header._has_content_length,p_http_pipe->_data._content_length , p_http_pipe->_data._recv_end_pos,p_http_pipe->_header._is_chunked);

	if((p_http_pipe->_data._b_received_0_byte ==TRUE)&&(p_http_pipe->_header._has_content_length==FALSE))
	{
		LOG_DEBUG( "  http_pipe[0x%X] _data_length=%u,_data_buffer_end_pos=%u",p_http_pipe,p_http_pipe->_data._data_length, p_http_pipe->_data._data_buffer_end_pos);
		
		p_http_pipe->_data._data_length = p_http_pipe->_data._data_buffer_end_pos;

		if((p_http_pipe->_data._data_length/HTTP_DEFAULT_RANGE_LEN)<download_range._num)
		{
			download_range._num=(p_http_pipe->_data._data_length+(HTTP_DEFAULT_RANGE_LEN-(p_http_pipe->_data._data_length%HTTP_DEFAULT_RANGE_LEN)))/HTTP_DEFAULT_RANGE_LEN;
		}
				
		 p_http_pipe->_data._content_length = p_http_pipe->_data._recv_end_pos;

	}
	else if(p_http_pipe->_data._content_length == p_http_pipe->_data._recv_end_pos)
	{
		LOG_DEBUG( "  http_pipe[0x%X] _data_length=%u,_data_buffer_end_pos=%u",p_http_pipe,p_http_pipe->_data._data_length, p_http_pipe->_data._data_buffer_end_pos);
		
		p_http_pipe->_data._data_length = p_http_pipe->_data._data_buffer_end_pos;

		if((p_http_pipe->_data._data_length/HTTP_DEFAULT_RANGE_LEN)<download_range._num)
		{
			download_range._num=(p_http_pipe->_data._data_length+(HTTP_DEFAULT_RANGE_LEN-(p_http_pipe->_data._data_length%HTTP_DEFAULT_RANGE_LEN)))/HTTP_DEFAULT_RANGE_LEN;
		}
	}
	else if(p_http_pipe->_header._is_chunked==TRUE)
	{
		if(p_http_pipe->_data.p_chunked->_state == HTTP_CHUNKED_STATE_END)
		{
			LOG_DEBUG( "  http_pipe[0x%X] _data_length=%u,_data_buffer_end_pos=%u",p_http_pipe,p_http_pipe->_data._data_length, p_http_pipe->_data._data_buffer_end_pos);
			
			p_http_pipe->_data._data_length = p_http_pipe->_data._data_buffer_end_pos;

			if((p_http_pipe->_data._data_length/HTTP_DEFAULT_RANGE_LEN)<download_range._num)
			{
				download_range._num=(p_http_pipe->_data._data_length+(HTTP_DEFAULT_RANGE_LEN-(p_http_pipe->_data._data_length%HTTP_DEFAULT_RANGE_LEN)))/HTTP_DEFAULT_RANGE_LEN;
			}
					
			 p_http_pipe->_data._content_length = p_http_pipe->_data._recv_end_pos;
		}
	}

	/* Check if pipe can put data to data manager */
	if((download_range._index==0)&&(FALSE ==http_pipe_check_can_put_data(p_http_pipe)))
	{
		return http_pipe_abort_put_data( p_http_pipe );
	}
	
	 /* Put the data to data manager */
	LOG_DEBUG( "  http_pipe[0x%X] DM_PUT_DATA:_p_data_manager=0x%X,_p_http_pipe=0x%X,_buffer=0x%X,&(_buffer)=0x%X, _data_length=%u,_buffer_length=%u,range._index=%u,range._unm=%u,resource=0x%X",
					p_http_pipe,p_http_pipe->_data_pipe._p_data_manager,p_http_pipe,p_http_pipe->_data._data_buffer,&(p_http_pipe->_data._data_buffer),p_http_pipe->_data._data_length, p_http_pipe->_data._data_buffer_length,download_range._index,download_range._num,p_http_pipe->_data_pipe._p_resource);

	ret_val = pi_put_data((DATA_PIPE *) p_http_pipe, &download_range, 
						&(p_http_pipe->_data._data_buffer), p_http_pipe->_data._data_length, p_http_pipe->_data._data_buffer_length,p_http_pipe->_data_pipe._p_resource);
					
	CHECK_VALUE(ret_val);
		
	received_pos =get_data_pos_from_index(download_range._index)+p_http_pipe->_data._data_length;
	
	p_http_pipe->_data._data_buffer_length = 0;
	p_http_pipe->_data._data_buffer_end_pos = 0;
	p_http_pipe->_data._data_length = 0;

	if(!p_http_pipe->_p_http_server_resource->_relation_res)
	{
		ret_val = http_pipe_update_down_range(p_http_pipe,received_pos);
	}
	else
	{
		_u64 relation_pos = 0;
		_int32 ret = pt_origion_pos_to_relation_pos(p_http_pipe->_p_http_server_resource->_p_block_info_arr, 
			p_http_pipe->_p_http_server_resource->_block_info_num, received_pos, &relation_pos);

		if(ret != SUCCESS)
		{
			return HTTP_ERR_RELATION_RES_NOT_SUPPORT_RANGE;
		}
			
		ret_val = http_pipe_update_down_range(p_http_pipe,relation_pos);
	}
	
	return ret_val;

}

/* The  _p_http_pipe->_data_pipe._dispatch_info._down_range is downloading success!  */
_int32 http_pipe_update_down_range( struct tag_HTTP_DATA_PIPE * p_http_pipe,_u64 received_pos)
{
	_int32 ret_val = SUCCESS;
	_u64 new_start_pos = 0,file_size=0;
	_u32 new_data_len = 0;
	BOOL need_close_connection=TRUE;
	RANGE download_range;

	ret_val=dp_get_download_range( &(p_http_pipe->_data_pipe), &download_range);
	CHECK_VALUE(ret_val);
	
	LOG_DEBUG( "_p_http_pipe=0x%X: enter http_pipe_update_down_range\
               (_down_range._index=%u,_down_range._unm=%u,_data._content_length=%llu,\
               _data._recv_end_pos=%llu,_req_end_pos=%llu,_b_ranges_changed=%d,\
               received_pos=%llu)..."
               , p_http_pipe
               , download_range._index
               , download_range._num
               , p_http_pipe->_data._content_length
               , p_http_pipe->_data._recv_end_pos
               , p_http_pipe->_req_end_pos
               , p_http_pipe->_b_ranges_changed
               , received_pos);

	/* Delete the current range from the uncomplete range list */
	ret_val =  dp_delete_uncomplete_ranges( &(p_http_pipe->_data_pipe), &download_range );
	CHECK_VALUE(ret_val);

	/* reset the _down_range */
	download_range._index= 0;
	download_range._num= 0;
	ret_val=dp_set_download_range( &(p_http_pipe->_data_pipe), &download_range);
	CHECK_VALUE(ret_val);

			
	file_size = http_pipe_get_file_size( p_http_pipe );

	LOG_DEBUG( "_p_http_pipe=0x%X:_file_size=%llu,_is_chunked=%d,_has_content_length=%d\
               ,_b_received_0_byte=%d"
               , p_http_pipe
               , file_size
               , p_http_pipe->_header._is_chunked
               , p_http_pipe->_header._has_content_length
               , p_http_pipe->_data._b_received_0_byte);

	if(p_http_pipe->_data._recv_end_pos == p_http_pipe->_data._content_length)
	{
        // 判断是否已经下载完成了
		if(((p_http_pipe->_header._is_chunked == TRUE)
            &&(p_http_pipe->_header._has_content_length == FALSE)
            &&(p_http_pipe->_header._has_entity_length == FALSE)) || 
           ((p_http_pipe->_data._b_received_0_byte == TRUE)
            &&(p_http_pipe->_header._has_content_length == FALSE)))
		{
			p_http_pipe->_b_get_file_size_after_data=TRUE;
			ret_val = http_pipe_set_file_size(  p_http_pipe ,received_pos );
			if(ret_val!=SUCCESS) return ret_val;
			need_close_connection=FALSE;
			LOG_DEBUG( "_p_http_pipe=0x%X:!!!!!!OK!! the downloading is completed!",p_http_pipe);
			goto DOWNLOAD_COMPLETED;
		}
	}

	ret_val =  http_pipe_get_down_range(p_http_pipe);
	if(ret_val!=SUCCESS) return ret_val;

	ret_val=dp_get_download_range(&(p_http_pipe->_data_pipe), &download_range);
	CHECK_VALUE(ret_val);
	
	if(download_range._num != 0)
	{
		new_start_pos = get_data_pos_from_index(download_range._index);
		new_data_len = range_to_length(&download_range,  file_size);
		LOG_DEBUG( "_p_http_pipe=0x%X:new_start_pos=%llu,new_data_len=%u,\
                   received_pos=%llu,_req_end_pos=%llu,_data._content_length=%llu,\
                   _data._recv_end_pos=%llu"
			       , p_http_pipe
                   , new_start_pos
                   , new_data_len
                   , received_pos
                   , p_http_pipe->_req_end_pos
                   , p_http_pipe->_data._content_length
                   , p_http_pipe->_data._recv_end_pos);

		if(p_http_pipe->_p_http_server_resource->_relation_res)
		{	
			_u64 relation_pos = 0;
			_int32 ret = pt_origion_pos_to_relation_pos(p_http_pipe->_p_http_server_resource->_p_block_info_arr, 
			p_http_pipe->_p_http_server_resource->_block_info_num, new_start_pos, &relation_pos);

			if(ret != SUCCESS)
			{
				LOG_DEBUG( "_p_http_pipe=0x%X is not support ranges 1",p_http_pipe);
				return HTTP_ERR_RELATION_RES_NOT_SUPPORT_RANGE;			
			}
			
			new_start_pos   = relation_pos;
		}                   
		/* Get the nes state to decide what to do next ! */
		if(new_start_pos==received_pos)
		{
			if(new_start_pos+new_data_len<=p_http_pipe->_req_end_pos)
			{
				if(p_http_pipe->_data._recv_end_pos==p_http_pipe->_data._content_length)
				{
					p_http_pipe->_data._content_length+=new_data_len;
					/* Continue to downloading... */
					LOG_DEBUG( "_p_http_pipe=0x%X Continue to downloading...  1",p_http_pipe);
					if(p_http_pipe->_b_ranges_changed ==TRUE)
						p_http_pipe->_b_ranges_changed =FALSE;
					
					return SUCCESS;
				}
				else if((p_http_pipe->_data._content_length-p_http_pipe->_data._recv_end_pos)>=new_data_len)
				{
					/* Continue to downloading... */
					LOG_DEBUG( "_p_http_pipe=0x%X Continue to downloading...  2",p_http_pipe);
					if(p_http_pipe->_b_ranges_changed ==TRUE)
						p_http_pipe->_b_ranges_changed =FALSE;
					
					return SUCCESS;
				}
				else
				{
					p_http_pipe->_data._content_length+=(new_data_len-(p_http_pipe->_data._content_length-p_http_pipe->_data._recv_end_pos));
					LOG_DEBUG( "_p_http_pipe=0x%X Continue to downloading...  3",p_http_pipe);
					/* Continue to downloading... */
					if(p_http_pipe->_b_ranges_changed ==TRUE)
						p_http_pipe->_b_ranges_changed =FALSE;
					
					return SUCCESS;
				}
					
			}
			else //if(new_start_pos+new_data_len>p_http_pipe->_req_end_pos)
			{
				if(received_pos==p_http_pipe->_req_end_pos)
				{
					if(HTTP_RESOURCE_IS_SUPPORT_RANGE(p_http_pipe->_p_http_server_resource)==TRUE)
					{
						LOG_DEBUG( "_p_http_pipe=0x%X Ready to download next ranges ,but  do not need to reconnect ,and need send new request:HTTP_PIPE_STATE_RANGE_CHANGED 1",p_http_pipe);
						HTTP_SET_STATE(HTTP_PIPE_STATE_RANGE_CHANGED) ;
						p_http_pipe->_b_ranges_changed =TRUE;
						return SUCCESS;
					}
					else
					{
						LOG_DEBUG( "_p_http_pipe=0x%X is not support ranges 1",p_http_pipe);
						return HTTP_ERR_RANGE_NOT_SUPPORT;
					}
				}
			}
		}
		else if((new_start_pos>received_pos)&&(received_pos==p_http_pipe->_req_end_pos))
		{
			if(HTTP_RESOURCE_IS_SUPPORT_RANGE(p_http_pipe->_p_http_server_resource)==TRUE)
			{
				LOG_DEBUG( "_p_http_pipe=0x%X Ready to download next ranges ,but  do not need to reconnect ,and need send new request:HTTP_PIPE_STATE_RANGE_CHANGED 2",p_http_pipe);
				HTTP_SET_STATE(HTTP_PIPE_STATE_RANGE_CHANGED) ;
				p_http_pipe->_b_ranges_changed =TRUE;
				return SUCCESS;
			}
			else
			{
				LOG_DEBUG( "_p_http_pipe=0x%X is not support ranges 2",p_http_pipe);
				return HTTP_ERR_RANGE_NOT_SUPPORT;
			}
		}

		/* All others,can not continue to download,must disconnect and the reconnect !*/
		LOG_DEBUG( "_p_http_pipe=0x%X Can not continue to download ,must disconnect and the reconnect [HTTP_PIPE_STATE_RANGE_CHANGED_NEED_RECONNECT]! ",p_http_pipe);
		HTTP_SET_STATE(HTTP_PIPE_STATE_RANGE_CHANGED_NEED_RECONNECT);
		p_http_pipe->_b_ranges_changed =TRUE;
		
		http_pipe_correct_uncomplete_head_range_for_not_support_range( p_http_pipe);
		
		return SUCCESS;
    }		
		
DOWNLOAD_COMPLETED:
	/* All ranges in _uncomplete_ranges are downloaded ! */
	p_http_pipe->_http_state = HTTP_PIPE_STATE_DOWNLOAD_COMPLETED;
	LOG_DEBUG( "_p_http_pipe=0x%X HTTP_PIPE_STATE_DOWNLOAD_COMPLETED...._b_retry_set_file_size=%d,_is_connected=%d",p_http_pipe ,p_http_pipe->_b_retry_set_file_size,p_http_pipe->_is_connected);
				
	//http_reset_respn_header( &(p_http_pipe->_header) );
	//http_pipe_reset_respn_data(p_http_pipe, &(p_http_pipe->_data) );		
	//return SOCKET_PROXY_CLOSE(_p_http_pipe->_socket_fd, http_pipe_handle_close, _p_http_pipe);
	if(p_http_pipe->_is_connected == TRUE)
	{
		LOG_DEBUG( "_p_http_pipe=0x%X _req_end_pos=%llu,_received_pos=%llu,_file_size=%llu,_wait_delete=%u",p_http_pipe,p_http_pipe->_req_end_pos, received_pos,file_size,p_http_pipe->_wait_delete);

		if(p_http_pipe->_wait_delete==TRUE)
		{
			/* Fatal error!*/
			LOG_DEBUG( "_p_http_pipe=0x%X Fatal error! should halt up the program!",p_http_pipe);
			CHECK_VALUE(p_http_pipe->_wait_delete);
						
		}

		if((need_close_connection==TRUE)&&((p_http_pipe->_req_end_pos>received_pos)||(file_size>received_pos)))
		{
			ret_val = http_pipe_close_connection(p_http_pipe);
			CHECK_VALUE(ret_val);
		}
	}
	
	if(p_http_pipe->_b_retry_set_file_size==FALSE)			
		pi_notify_dispatch_data_finish((DATA_PIPE *)p_http_pipe);
	else
	{
		LOG_DEBUG( "_p_http_pipe=0x%X do not dm_notify_dispatch_data_finish, because _b_retry_set_file_size==TRUE!",p_http_pipe);
		//sd_assert(FALSE);
	}

	return SUCCESS;
}


/*************************/
/* structrue reset      */
/*************************/

_int32 http_pipe_reset_respn_data(struct tag_HTTP_DATA_PIPE * p_http_pipe, HTTP_RESPN_DATA * p_respn_data )
{
	LOG_DEBUG( " enter http_pipe_reset_respn_data(0x%X,%u)..." ,p_http_pipe,p_http_pipe->_data_pipe._id);

	if(p_respn_data->_data_buffer!=NULL)
		pi_free_data_buffer( (DATA_PIPE *)p_http_pipe,  &(p_respn_data->_data_buffer),p_respn_data->_data_buffer_length);	

	if(p_respn_data->_temp_data_buffer!=NULL)
		http_release_1024_buffer((void*) (p_respn_data->_temp_data_buffer));
	p_respn_data->_temp_data_buffer=NULL;

	if(p_respn_data->p_chunked!=NULL)
		http_pipe_delete_chunked( p_respn_data);
	p_respn_data->p_chunked=NULL;

	sd_memset(p_respn_data,0,sizeof(HTTP_RESPN_DATA));

	 return SUCCESS;

}


_int32 http_pipe_create_chunked(HTTP_RESPN_DATA * p_respn_data)
{
	_int32 ret_val = SUCCESS;
	
	sd_assert(p_respn_data->p_chunked==NULL);
	
	ret_val = sd_malloc(sizeof(HTTP_RESPN_DATA_CHUNKED) ,(void**)&(p_respn_data->p_chunked));
	CHECK_VALUE(ret_val);

	sd_memset(p_respn_data->p_chunked,0,sizeof(HTTP_RESPN_DATA_CHUNKED));
	return SUCCESS;
}
_int32 http_pipe_delete_chunked(HTTP_RESPN_DATA * p_respn_data)
{
	SAFE_DELETE(p_respn_data->p_chunked);
	return SUCCESS;
}
_int32 http_pipe_parse_chunked_header( struct tag_HTTP_DATA_PIPE * p_http_pipe ,char* header,_u32 header_len)
{
	//_int32 ret_val=SUCCESS;
	char * tmp=header,*crlf=NULL;
	_u32 len=header_len,pack_len=0,remainder_len=0;

	LOG_DEBUG( " enter[0x%X] http_pipe_parse_chunked_header(header[%u]=%s)...",p_http_pipe,header_len,header );
	if(p_http_pipe == NULL ) return -1;
	
	/* Transfer-Encoding: chunked\r\n\r\n20df\r\n<html><head><meta http-equiv=\"content-type\" content= */
	p_http_pipe->_data.p_chunked->_pack_len=0;
	p_http_pipe->_data.p_chunked->_pack_received_len=0;
	p_http_pipe->_data.p_chunked->_temp_buffer_end_pos=0;
ParseHeader:	
	if((tmp!=NULL)&&(len>=CHUNKED_HEADER_DEFAULT_LEN))
	{
		if((tmp[0]==0x0d)&&(tmp[1]==0x0a))
		{
			tmp+=2;
		}
		else
		if(tmp[0]==0x0a)
		{
			tmp+=1;
		}
	
		crlf = sd_strstr(tmp, "\r\n", 0);

		if((crlf!=NULL)&&(crlf-tmp<CHUNKED_HEADER_BUFFER_LEN))
		{
			/* Got the header */
			pack_len=0;
			while(IS_HEX(tmp[0]))
			{
				pack_len=pack_len*16+sd_hex_2_int(tmp[0]);
				tmp++;
			}
			/* Escape all ' '  */
			while((tmp[0]==' ')&&(tmp<crlf))
			{
				tmp++;
			}
			
			LOG_DEBUG( " [0x%X] get  pack_len=%u",p_http_pipe,pack_len );
			p_http_pipe->_data.p_chunked->_pack_len=pack_len;
			if(pack_len==0)
			{
				LOG_DEBUG( " [0x%X] ok,HTTP_CHUNKED_STATE_END,get remainder_len=%u",p_http_pipe,tmp+2-header );
				/* received 0  */
				p_http_pipe->_data.p_chunked->_state = HTTP_CHUNKED_STATE_END;
				//sd_assert((tmp+2-header)==header_len);
			}
			else
			{
				p_http_pipe->_data.p_chunked->_state = HTTP_CHUNKED_STATE_RECEIVE_DATA;
				sd_assert(tmp==crlf);
				tmp+=2;
					
				remainder_len = header_len-(tmp-header);
				LOG_DEBUG( " [0x%X],ok,HTTP_CHUNKED_STATE_RECEIVE_DATA, get  remainder_len=%u",p_http_pipe,remainder_len );
				
				if(remainder_len!=0)
				{
					
					if(( (p_http_pipe->_data._recv_end_pos+remainder_len) <=p_http_pipe->_data._content_length)
						&&((remainder_len+p_http_pipe->_data._data_buffer_end_pos)<=p_http_pipe->_data._data_length))
					{
						if(p_http_pipe->_data._data_buffer!=NULL)
						{
							if(remainder_len>=pack_len)
							{
								LOG_DEBUG( " [0x%X],remainder_len>=pack_len ",p_http_pipe );
								sd_memmove(p_http_pipe->_data._data_buffer+p_http_pipe->_data._data_buffer_end_pos,tmp,pack_len);
								p_http_pipe->_data._data_buffer_end_pos+=pack_len;
								
								p_http_pipe->_data._recv_end_pos+=pack_len;

								remainder_len-=pack_len;
								 
								if(remainder_len>2)
								{
									tmp+=pack_len;
									len=header_len-(tmp-header);
									LOG_DEBUG( " [0x%X],ParseHeaderagain :tmp[%u]=%s ",p_http_pipe ,len,tmp);
									goto ParseHeader;
									//CHECK_VALUE(HTTP_ERR_BAD_TEMP_BUFFER);
								}
								else
								{
									p_http_pipe->_data.p_chunked->_state = HTTP_CHUNKED_STATE_RECEIVE_HEAD;
									remainder_len=0;
								}
							}
							else
							{
								sd_memmove(p_http_pipe->_data._data_buffer+p_http_pipe->_data._data_buffer_end_pos,tmp,remainder_len);
								p_http_pipe->_data._data_buffer_end_pos+=remainder_len;
								
								p_http_pipe->_data._recv_end_pos+=remainder_len;
							}
						}
						else
						{
							CHECK_VALUE(HTTP_ERR_BAD_TEMP_BUFFER);
						}
					}
					else
					{
						//CHECK_VALUE(HTTP_ERR_BAD_TEMP_BUFFER);
						LOG_DEBUG( " [0x%X],no data buffer,need store in the _temp_buffer",p_http_pipe );
						
						if(remainder_len>CHUNKED_TEMP_BUFFER_LEN)
						{
							if(( (p_http_pipe->_data._recv_end_pos+remainder_len) >p_http_pipe->_data._content_length)
								||((remainder_len+p_http_pipe->_data._data_buffer_end_pos)>p_http_pipe->_data._data_length))
							{
								LOG_DEBUG( " http_pipe[0x%X] HTTP_ERR_INVALID_SERVER_FILE_SIZE!!",p_http_pipe);	
								return HTTP_ERR_INVALID_SERVER_FILE_SIZE;
							}
							
						}
						
						sd_assert(remainder_len<=CHUNKED_TEMP_BUFFER_LEN);
						sd_memset(p_http_pipe->_data.p_chunked->_temp_buffer,0,CHUNKED_TEMP_BUFFER_LEN);
						if(remainder_len>=pack_len)
						{
							LOG_DEBUG( " [0x%X],remainder_len>=pack_len",p_http_pipe );
							sd_memmove(p_http_pipe->_data.p_chunked->_temp_buffer+p_http_pipe->_data.p_chunked->_temp_buffer_end_pos,tmp,pack_len);
							p_http_pipe->_data.p_chunked->_temp_buffer_end_pos+=pack_len;
								
							remainder_len-=pack_len;
								 
							if(remainder_len>2)
							{
								tmp+=pack_len;
								len=header_len-(tmp-header);
								goto ParseHeader;
								//CHECK_VALUE(HTTP_ERR_BAD_TEMP_BUFFER);
							}
							else
							{
								p_http_pipe->_data.p_chunked->_state = HTTP_CHUNKED_STATE_RECEIVE_HEAD;
								remainder_len=0;
							}
						}
						else
						{
							sd_memmove(p_http_pipe->_data.p_chunked->_temp_buffer+p_http_pipe->_data.p_chunked->_temp_buffer_end_pos,tmp,remainder_len);
							p_http_pipe->_data.p_chunked->_temp_buffer_end_pos+= remainder_len;
						}
					}
				}
				p_http_pipe->_data.p_chunked->_pack_received_len=remainder_len;
			}
			p_http_pipe->_data.p_chunked->_head_buffer_end_pos=0;
		}
		else
		{
			if(p_http_pipe->_data.p_chunked->_head_buffer_end_pos!=header_len)
			{
				/* Call by http_pipe_store_temp_data_buffer */
				sd_assert(p_http_pipe->_data.p_chunked->_head_buffer_end_pos==0);
				p_http_pipe->_data.p_chunked->_head_buffer_end_pos=header_len-(tmp-header);
				if(p_http_pipe->_data.p_chunked->_head_buffer_end_pos>=CHUNKED_HEADER_BUFFER_LEN-1)
					CHECK_VALUE(HTTP_ERR_CHUNKED_HEAD_NO_BUFFER);
				
				p_http_pipe->_data.p_chunked->_state = HTTP_CHUNKED_STATE_RECEIVE_HEAD;
				sd_memmove(p_http_pipe->_data.p_chunked->_head_buffer, tmp, p_http_pipe->_data.p_chunked->_head_buffer_end_pos);
			}
			else
			if(header_len>=CHUNKED_HEADER_BUFFER_LEN-1)
			{
			    return HTTP_ERR_CHUNKED_HEAD_NO_BUFFER;
			}
				//CHECK_VALUE(HTTP_ERR_CHUNKED_HEAD_NO_BUFFER);
		}
	}
	else
	{
		return SUCCESS; //HTTP_ERR_CHUNKED_HEAD_NO_BUFFER;
	}
	
	
	 return SUCCESS;

}
_int32 http_pipe_parse_chunked_data( struct tag_HTTP_DATA_PIPE * p_http_pipe )
{
	//_int32 ret_val=SUCCESS;
	_int32 ret_val=SUCCESS;

	LOG_DEBUG( " enter[0x%X] http_pipe_parse_chunked_data(_data_buffer_end_pos=%u,_data_length=%u,_pack_received_len=%u,_pack_len=%u,_head_buffer_end_pos=%u)...",
		p_http_pipe ,p_http_pipe->_data._data_buffer_end_pos,p_http_pipe->_data._data_length,p_http_pipe->_data.p_chunked->_pack_received_len,p_http_pipe->_data.p_chunked->_pack_len,p_http_pipe->_data.p_chunked->_head_buffer_end_pos);

	if(p_http_pipe->_data.p_chunked->_head_buffer_end_pos!=0)
		p_http_pipe->_data.p_chunked->_head_buffer_end_pos=0;
	
	if(p_http_pipe->_data._data_buffer_end_pos==p_http_pipe->_data._data_length)
	{
		if(p_http_pipe->_data.p_chunked->_pack_received_len==p_http_pipe->_data.p_chunked->_pack_len)
		{
			p_http_pipe->_data.p_chunked->_state = HTTP_CHUNKED_STATE_RECEIVE_HEAD;
			LOG_DEBUG( " enter [0x%X] http_pipe_parse_chunked_data()...HTTP_CHUNKED_STATE_RECEIVE_HEAD 1",p_http_pipe );
		}
		/* down range success !*/
		ret_val = http_pipe_down_range_success( p_http_pipe);
		if(ret_val!=SUCCESS) return ret_val;
	}
	else
	if((p_http_pipe->_data._b_received_0_byte ==TRUE)||(p_http_pipe->_data.p_chunked->_state == HTTP_CHUNKED_STATE_END))
	{
		
		/* down range success !*/
		ret_val = http_pipe_down_range_success( p_http_pipe);
		if(ret_val!=SUCCESS) return ret_val;
	}
	else
	if(p_http_pipe->_data.p_chunked->_pack_received_len==p_http_pipe->_data.p_chunked->_pack_len)
	{
		p_http_pipe->_data.p_chunked->_state = HTTP_CHUNKED_STATE_RECEIVE_HEAD;
		LOG_DEBUG( " enter [0x%X] http_pipe_parse_chunked_data()...HTTP_CHUNKED_STATE_RECEIVE_HEAD 2",p_http_pipe );
	}
	else
	{
		//CHECK_VALUE(HTTP_ERR_CHUNKED_HEAD_NO_BUFFER);
		LOG_DEBUG( " enter [0x%X] http_pipe_parse_chunked_data()...do nothing",p_http_pipe );
	}
	
	
	 return SUCCESS;

}
 _int32 http_pipe_store_chunked_temp_data_buffer( struct tag_HTTP_DATA_PIPE * p_http_pipe )
{
	_int32 cpy_data_len = 0,left_len = 0;
	LOG_DEBUG( "  http_pipe[0x%X] enter http_pipe_store_chunked_temp_data_buffer:_recv_end_pos=%llu,_temp_buffer_end_pos =%u ,_content_length=%llu,_data_buffer_end_pos=%u,_data_length =%u ",
		p_http_pipe,p_http_pipe->_data._recv_end_pos,p_http_pipe->_data.p_chunked->_temp_buffer_end_pos,p_http_pipe->_data._content_length,p_http_pipe->_data._data_buffer_end_pos,p_http_pipe->_data._data_length );


		if(( (p_http_pipe->_data._recv_end_pos+p_http_pipe->_data.p_chunked->_temp_buffer_end_pos) <=p_http_pipe->_data._content_length)
			/*&&((p_http_pipe->_data.p_chunked->_temp_buffer_end_pos+p_http_pipe->_data._data_buffer_end_pos)<=p_http_pipe->_data._data_length)*/)
		{
			if((p_http_pipe->_data.p_chunked->_temp_buffer_end_pos+p_http_pipe->_data._data_buffer_end_pos)<=p_http_pipe->_data._data_length)
			{
				cpy_data_len = p_http_pipe->_data.p_chunked->_temp_buffer_end_pos;
				left_len = 0;
			}
			else
			{
				cpy_data_len = p_http_pipe->_data._data_length -p_http_pipe->_data._data_buffer_end_pos;
				left_len = p_http_pipe->_data.p_chunked->_temp_buffer_end_pos+p_http_pipe->_data._data_buffer_end_pos-p_http_pipe->_data._data_length;
			}
			
			if((p_http_pipe->_data.p_chunked->_temp_buffer!=NULL )&&(p_http_pipe->_data._data_buffer!=NULL))
			{
				sd_memcpy(p_http_pipe->_data._data_buffer+p_http_pipe->_data._data_buffer_end_pos,p_http_pipe->_data.p_chunked->_temp_buffer,cpy_data_len);
				p_http_pipe->_data._data_buffer_end_pos+=cpy_data_len;

				p_http_pipe->_data._recv_end_pos+=cpy_data_len;

				if(left_len == 0)
				{
					p_http_pipe->_data.p_chunked->_temp_buffer_end_pos = 0;
				}
				else
				{
					sd_memcpy(p_http_pipe->_data.p_chunked->_temp_buffer,p_http_pipe->_data.p_chunked->_temp_buffer+cpy_data_len,left_len);
					p_http_pipe->_data.p_chunked->_temp_buffer_end_pos = left_len;
				}

				return SUCCESS;
			}
		}
		
		CHECK_VALUE(HTTP_ERR_BAD_TEMP_BUFFER);
		
	return SUCCESS;
}

_u32 http_pipe_get_receive_len(struct tag_HTTP_DATA_PIPE * p_http_pipe)
{
	_u32 receive_len =0;
	_u32 _block_len = socket_proxy_get_p2s_recv_block_size();

	if(p_http_pipe->_header._is_chunked==TRUE)
	{
		LOG_DEBUG( "[0x%X]http_pipe_get_receive_len, _is_chunked=%d,_pack_len=%u,_pack_received_len=%u",
		p_http_pipe,p_http_pipe->_header._is_chunked ,p_http_pipe->_data.p_chunked->_pack_len,p_http_pipe->_data.p_chunked->_pack_received_len);
		receive_len =p_http_pipe->_data.p_chunked->_pack_len-p_http_pipe->_data.p_chunked->_pack_received_len;
	}
	else
		receive_len = _block_len;
	
	if(receive_len>p_http_pipe->_data._content_length-p_http_pipe->_data._recv_end_pos)
		receive_len = p_http_pipe->_data._content_length-p_http_pipe->_data._recv_end_pos;
	
	if(receive_len>p_http_pipe->_data._data_length-p_http_pipe->_data._data_buffer_end_pos)
		receive_len = p_http_pipe->_data._data_length-p_http_pipe->_data._data_buffer_end_pos;
	
	if(receive_len>_block_len)
		receive_len=_block_len ;
	
	LOG_DEBUG( "[0x%X]http_pipe_get_receive_len=%u, _is_chunked=%d,_has_content_length=%d,_content_length=%llu,_recv_end_pos=%llu,_data_length=%u,_data_buffer_end_pos=%u,_block_len=%u",
		p_http_pipe,receive_len,p_http_pipe->_header._is_chunked ,p_http_pipe->_header._has_content_length,p_http_pipe->_data._content_length,p_http_pipe->_data._recv_end_pos, p_http_pipe->_data._data_length,p_http_pipe->_data._data_buffer_end_pos,_block_len);
	
	return receive_len;
}


_u32 http_pipe_get_download_range_index(struct tag_HTTP_DATA_PIPE * p_http_pipe)
{
	RANGE download_range;
	_int32 ret_val = SUCCESS;
	ret_val=dp_get_download_range( &(p_http_pipe->_data_pipe), &download_range);
	CHECK_VALUE(ret_val);
	return download_range._index;
}
_u32 http_pipe_get_download_range_num(struct tag_HTTP_DATA_PIPE * p_http_pipe)
{
	RANGE download_range;
	_int32 ret_val = SUCCESS;
	ret_val=dp_get_download_range( &(p_http_pipe->_data_pipe), &download_range);
	CHECK_VALUE(ret_val);
	return download_range._num;
}

/*******************************************************************/

#endif /* __HTTP_DATA_PROCESS_C_20090305 */
