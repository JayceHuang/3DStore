#if !defined(__FTP_DATA_PIPE_C_20080614)
#define __FTP_DATA_PIPE_C_20080614

#include "ftp_data_pipe.h"


#include "platform/sd_socket.h"
#include "utility/settings.h"
#include "utility/utility.h"
#include "platform/sd_fs.h"
#include "p2sp_download/p2sp_task.h"

#include "utility/logid.h"
#define LOGID LOGID_FTP_PIPE

#include "utility/logger.h"

#include "socket_proxy_interface.h"

#ifdef FTP_TRACE
#undef FTP_TRACE
#endif
#define FTP_TRACE LOG_DEBUG("(_p_ftp_pipe=0x%X)",_p_ftp_pipe);LOG_DEBUG


/*--------------------------------------------------------------------------*/
/*                Global declarations                       */
/*--------------------------------------------------------------------------*/
/* Pointer of the ftp_resource and ftp_pipe slab */
static SLAB *gp_ftp_res_slab = NULL;
static SLAB *gp_ftp_pipe_slab = NULL;


/*--------------------------------------------------------------------------*/
/*                Interfaces provid for other modules                       */
/*--------------------------------------------------------------------------*/
_int32 init_ftp_pipe_module(  void)
{
    _int32 _result = SUCCESS;
    LOG_DEBUG( "init_ftp_pipe_module" );

    if(gp_ftp_res_slab==NULL)
    {
        _result = mpool_create_slab(sizeof(FTP_SERVER_RESOURCE), MIN_FTP_RES_MEMORY, 0, &gp_ftp_res_slab);
        CHECK_VALUE(_result);
    }

    if(gp_ftp_pipe_slab==NULL)
    {
        _result = mpool_create_slab(sizeof(FTP_DATA_PIPE), MIN_FTP_PIPE_MEMORY, 0, &gp_ftp_pipe_slab);
        if(_result!=SUCCESS)
        {
            mpool_destory_slab(gp_ftp_res_slab);
            gp_ftp_res_slab = NULL;
            CHECK_VALUE(_result);
        }
    }

    return _result;

}


_int32 uninit_ftp_pipe_module(  void)
{
    _int32 _result = SUCCESS;
    LOG_DEBUG( "uninit_ftp_pipe_module" );

    if(gp_ftp_res_slab!=NULL)
    {
        _result = mpool_destory_slab(gp_ftp_res_slab);
        CHECK_VALUE(_result);
        gp_ftp_res_slab = NULL;
    }

    if(gp_ftp_pipe_slab!=NULL)
    {
        _result = mpool_destory_slab(gp_ftp_pipe_slab);
        CHECK_VALUE(_result);
        gp_ftp_pipe_slab = NULL;
    }

    return _result;

}


_int32 ftp_resource_create(  char *_url,_u32 _url_size,BOOL _is_origin,RESOURCE ** _pp_ftp_resouce)
{
    FTP_SERVER_RESOURCE * _p_ftp_resource = NULL;
    _int32 _result = SUCCESS;

    LOG_DEBUG( "ftp_resource_create(_is_origin=%d,_url_size=%d,url=%s)",_is_origin,_url_size ,_url);

    if((_url == NULL)||(_url_size==0)||(_url_size>MAX_URL_LEN-1))
        return FTP_ERR_INVALID_URL;

    _result = mpool_get_slip(gp_ftp_res_slab, (void**)&_p_ftp_resource);
    CHECK_VALUE(_result);

    sd_memset(_p_ftp_resource,0,sizeof(FTP_SERVER_RESOURCE));

#ifdef _DEBUG
    sd_memcpy(_p_ftp_resource->_o_url,_url,_url_size);
#endif

    if(sd_url_to_object(_url,_url_size,&(_p_ftp_resource->_url))!=SUCCESS)
    {
        _result = mpool_free_slip(gp_ftp_res_slab,(void*) _p_ftp_resource);
        CHECK_VALUE(_result);
        return FTP_ERR_INVALID_URL;
    }

    if((_p_ftp_resource->_url._schema_type !=FTP)||(_p_ftp_resource->_url._is_dynamic_url ==TRUE)||(_p_ftp_resource->_url._full_path[0]=='\0')||(_p_ftp_resource->_url._file_name==NULL)||(_p_ftp_resource->_url._file_name_len==0))
    {
        _result = mpool_free_slip(gp_ftp_res_slab,(void*) _p_ftp_resource);
        CHECK_VALUE(_result);
        return FTP_ERR_INVALID_URL;
    }

    /***************?????????????????????????????????????????????????????????????????***************************/
    /*
        if(sd_strcmp(_p_ftp_resource->_url._host,"127.0.0.1")==0)
            {
                _result = mpool_free_slip(gp_ftp_res_slab,(void*) _p_ftp_resource);
                CHECK_VALUE(_result);
                return FTP_ERR_INVALID_URL;
            }
    */
    /***************?????????????????????????????????????????????????????????????????***************************/


    init_resource_info( &(_p_ftp_resource->_resource) , FTP_RESOURCE);

    _p_ftp_resource->_b_is_origin = _is_origin;

    if(_is_origin)
    {
        set_resource_level( (RESOURCE* )_p_ftp_resource, MAX_RES_LEVEL);
    }

    *_pp_ftp_resouce = (RESOURCE *) (_p_ftp_resource);

    LOG_DEBUG( "++++ftp_resource_create(_is_origin=%d,_url_size=%d,url=%s) SUCCESS!!",_is_origin,_url_size ,_url);
    return SUCCESS;

}

_int32 ftp_resource_destroy(RESOURCE ** _pp_ftp_resouce)
{
    FTP_SERVER_RESOURCE * _p_ftp_resource = NULL;
    _int32 _result = SUCCESS;
    LOG_DEBUG( "ftp_resource_destroy" );

    if((* _pp_ftp_resouce) == NULL)
        return FTP_ERR_INVALID_RESOURCE;
    if((* _pp_ftp_resouce)->_resource_type!= FTP_RESOURCE )
        return FTP_ERR_INVALID_RESOURCE;

    _p_ftp_resource = (FTP_SERVER_RESOURCE *)(* _pp_ftp_resouce);


    uninit_resource_info( &(_p_ftp_resource->_resource));

    _result = mpool_free_slip(gp_ftp_res_slab,(void*) _p_ftp_resource);
    CHECK_VALUE(_result);
    *_pp_ftp_resouce=NULL;

    LOG_DEBUG( "++++ ftp_resource_destroy SUCCESS!" );

    return SUCCESS;
}

/* Interface for connect_manager getting file suffix  */
char* ftp_resource_get_file_suffix(RESOURCE * _p_resource,BOOL *_is_binary_file)
{
    FTP_SERVER_RESOURCE * _p_ftp_resource = (FTP_SERVER_RESOURCE * )_p_resource;
    LOG_DEBUG( " enter ftp_resource_get_file_suffix()..." );


    if(_p_ftp_resource->_url._file_suffix[0]!='\0')
    {
        LOG_DEBUG( "_file_suffix=%s" ,_p_ftp_resource->_url._file_suffix);
        *_is_binary_file = _p_ftp_resource->_url._is_binary_file;
        return _p_ftp_resource->_url._file_suffix;
    }
    else
    {
        *_is_binary_file = FALSE;
        return NULL;
    }
}
/* Interface for connect_manager getting file name from resource */
BOOL ftp_resource_get_file_name(RESOURCE * _p_resource
    , char * _file_name
    , _u32 buffer_len
    , BOOL _compellent)
{
    char *suffix_pos=NULL;
	char suffix_pos_string[128] = {0};
    FTP_SERVER_RESOURCE * _p_ftp_resource = (FTP_SERVER_RESOURCE * )_p_resource;

#ifdef _DEBUG
    LOG_DEBUG( " enter ftp_resource_get_file_name(is_origin=%d,_compellent=%d,url=%s)...",_p_ftp_resource->_b_is_origin,_compellent,_p_ftp_resource->_o_url);
#else
    LOG_DEBUG( " enter ftp_resource_get_file_name(is_origin=%d,_compellent=%d)...",_p_ftp_resource->_b_is_origin,_compellent );
#endif

    sd_memset(_file_name,0,buffer_len);

    suffix_pos = sd_strrchr(_p_ftp_resource->_url._file_name, '.');
	if(suffix_pos != NULL)
	{
		strcpy(suffix_pos_string, suffix_pos);
	}
	
    if((suffix_pos!=NULL)&&(sd_is_binary_file(suffix_pos_string,NULL)==TRUE))
    {
        if(_p_ftp_resource->_url._file_name_len>=buffer_len) return FALSE;
        sd_memcpy(_file_name,_p_ftp_resource->_url._file_name,_p_ftp_resource->_url._file_name_len);
        /* remove all the '%' */
        if( sd_decode_file_name(_file_name, suffix_pos, buffer_len)==TRUE)
        {
            if(sd_is_file_name_valid(_file_name)==TRUE)
            {
                LOG_DEBUG( "Get file name from ftp_resource_get_file_name=%s" ,_file_name);
                return TRUE;
            }
        }
    }
    else if(_compellent == TRUE)
    {
        if(_p_ftp_resource->_url._file_name_len>=buffer_len) return FALSE;
        sd_memcpy(_file_name,_p_ftp_resource->_url._file_name,_p_ftp_resource->_url._file_name_len);
        /* remove all the '%' */
        if( sd_decode_file_name(_file_name,NULL, buffer_len)==TRUE)
        {
            if(sd_is_file_name_valid(_file_name)==TRUE)
            {
                LOG_DEBUG( "ftp_resource_get_file_name=%s" ,_file_name);
                return TRUE;
            }
        }
    }

    return FALSE;

}

BOOL ftp_resource_is_support_range(RESOURCE * p_resource)
{
    FTP_SERVER_RESOURCE * p_ftp_resource = (FTP_SERVER_RESOURCE * )p_resource;

    LOG_DEBUG( "[0x%X]ftp_resource_is_support_range:%d" ,p_ftp_resource,p_ftp_resource->_is_support_range);

    return p_ftp_resource->_is_support_range;
}


_int32 ftp_pipe_create(  void*  _p_data_manager,RESOURCE*  _p_resource,DATA_PIPE ** _pp_pipe)
{
    FTP_DATA_PIPE * _p_ftp_pipe = NULL;
    FTP_SERVER_RESOURCE * _p_ftp_resource = NULL;
    _int32 _result = SUCCESS;

    FTP_TRACE( " enter ftp_pipe_create()..." );

    if(* _pp_pipe)  return FTP_ERR_PIPE_NOT_EMPTY;
    if(_p_resource == NULL) return FTP_ERR_RESOURCE_EMPTY;
    if(_p_resource->_resource_type!= FTP_RESOURCE)  return FTP_ERR_RESOURCE_EMPTY;
    if(_p_data_manager == NULL) return FTP_ERR_DATA_MANAGER_EMPTY;

    _p_ftp_resource = (FTP_SERVER_RESOURCE *)_p_resource;
    if(_p_ftp_resource->_url._host[0]=='\0')    return FTP_ERR_INVALID_RESOURCE;

    _result = mpool_get_slip(gp_ftp_pipe_slab, (void**)&_p_ftp_pipe);
    CHECK_VALUE(_result);

    sd_memset(_p_ftp_pipe,0,sizeof(FTP_DATA_PIPE));

    init_pipe_info( &(_p_ftp_pipe->_data_pipe), FTP_PIPE, _p_resource,(void *)_p_data_manager );

    init_speed_calculator(&(_p_ftp_pipe->_speed_calculator), 20, 500);

    _p_ftp_pipe->_ftp_state = FTP_PIPE_STATE_IDLE;
    _p_ftp_pipe->_p_ftp_server_resource = (FTP_SERVER_RESOURCE *)_p_resource;

    _p_ftp_pipe->_reveive_range_num = FTP_DEFAULT_RANGE_NUM;
    settings_get_int_item( "ftp_data_pipe.receive_ranges_number",(_int32*)&(_p_ftp_pipe->_reveive_range_num));

    *_pp_pipe = (DATA_PIPE *)(_p_ftp_pipe);
    FTP_TRACE( "++++ftp pipe[0x%X] is created success!++++" ,_p_ftp_pipe);
    return SUCCESS;


}



_int32 ftp_pipe_open(DATA_PIPE * _p_pipe)
{
    FTP_DATA_PIPE * _p_ftp_pipe = (FTP_DATA_PIPE *)_p_pipe;
    FTP_TRACE( "FTP:enter ftp_pipe_open()..." );

    if ((!_p_ftp_pipe)||(_p_pipe->_data_pipe_type !=FTP_PIPE)) return FTP_ERR_INVALID_PIPE;

    if (_p_ftp_pipe->_wait_delete) return FTP_ERR_WAITING_DELETE;

    FTP_TRACE( "  _pipe_state=%d,_ftp_state=%d ",_p_ftp_pipe->_data_pipe._dispatch_info._pipe_state,_p_ftp_pipe->_ftp_state);

    if((_p_ftp_pipe->_data_pipe._dispatch_info._pipe_state != PIPE_IDLE)
       &&(_p_ftp_pipe->_data_pipe._dispatch_info._pipe_state != PIPE_DOWNLOADING))
        return FTP_ERR_OPEN_WRONG_STATE;

    if((_p_ftp_pipe->_ftp_state != FTP_PIPE_STATE_IDLE)
       &&(_p_ftp_pipe->_ftp_state != FTP_PIPE_STATE_RANGE_CHANGED)
       &&(_p_ftp_pipe->_ftp_state != FTP_PIPE_STATE_DOWNLOAD_COMPLETED))
        return FTP_ERR_OPEN_WRONG_STATE;

    if(_p_ftp_pipe->_is_connected == TRUE)
        return FTP_ERR_OPEN_WRONG_STATE;

    _p_ftp_pipe->_error_code = SOCKET_PROXY_CREATE(&(_p_ftp_pipe->_socket_fd),SD_SOCK_STREAM);
    if((_p_ftp_pipe->_error_code)||(_p_ftp_pipe->_socket_fd == 0))
    {
        /* Creating socket error ! */
        goto ErrorHanle;
    }

    LOG_DEBUG("ftp_pipe_open _p_ftp_pipe->_socket_fd = %u", _p_ftp_pipe->_socket_fd);
    _p_ftp_pipe->_data_pipe._dispatch_info._last_recv_data_time=MAX_U32;
    //_p_ftp_pipe->_data_pipe._dispatch_info._pipe_state = PIPE_CONNECTING;
    dp_set_state(&_p_ftp_pipe->_data_pipe, PIPE_CONNECTING);
    _p_ftp_pipe->_ftp_state = FTP_PIPE_STATE_CONNECTING;

    FTP_TRACE( "call SOCKET_PROXY_CONNECT(fd=%u,_host=%s,port=%u)" ,_p_ftp_pipe->_socket_fd,_p_ftp_pipe->_p_ftp_server_resource->_url._host,_p_ftp_pipe->_p_ftp_server_resource->_url._port);
    _p_ftp_pipe->_error_code = SOCKET_PROXY_CONNECT(_p_ftp_pipe->_socket_fd,_p_ftp_pipe->_p_ftp_server_resource->_url._host,
                               _p_ftp_pipe->_p_ftp_server_resource->_url._port,ftp_pipe_handle_connect,(void*)_p_ftp_pipe);
    if(_p_ftp_pipe->_error_code)
    {
        /* Opening socket error ! */
        goto ErrorHanle;
    }


    return SUCCESS;

ErrorHanle:
    ftp_pipe_failure_exit( _p_ftp_pipe);
    return _p_ftp_pipe->_error_code;


}
_int32 ftp_pipe_close(DATA_PIPE * _p_pipe)
{
    FTP_DATA_PIPE * _p_ftp_pipe = (FTP_DATA_PIPE *)_p_pipe;
    FTP_TRACE( " enter ftp_pipe_close()..." );

    if ((!_p_ftp_pipe)||(_p_pipe->_data_pipe_type !=FTP_PIPE)) return FTP_ERR_INVALID_PIPE;

    _p_ftp_pipe->_wait_delete = TRUE;

    FTP_TRACE( " _retry_connect_timer_id=%u,_pasv._retry_timer_id=%u,_retry_get_buffer_timer_id=%u,_is_connected=%d,_ftp_state=%d,_pasv._is_connected=%d,_pasv._state=%d,_b_ranges_changed=%d",
               _p_ftp_pipe->_retry_connect_timer_id,_p_ftp_pipe->_pasv._retry_timer_id,_p_ftp_pipe->_retry_get_buffer_timer_id,_p_ftp_pipe->_is_connected,_p_ftp_pipe->_ftp_state,_p_ftp_pipe->_pasv._is_connected,_p_ftp_pipe->_pasv._state,_p_ftp_pipe->_b_ranges_changed);

    _p_ftp_pipe->_b_ranges_changed = FALSE;
    dp_set_pipe_interface(_p_pipe, NULL );
#ifdef _DEBUG
    FTP_TRACE( " ++++ _p_ftp_pipe[0x%X,url=%s]->_sum_recv_bytes=%llu,LargerThanRange=%d,_ftp_state=%d,_pipe_state=%d,ErrCode=%d",_p_ftp_pipe,_p_ftp_pipe->_p_ftp_server_resource->_o_url,_p_ftp_pipe->_sum_recv_bytes,((_p_ftp_pipe->_sum_recv_bytes>FTP_DEFAULT_RANGE_LEN)?1:0),_p_ftp_pipe->_ftp_state,_p_ftp_pipe->_data_pipe._dispatch_info._pipe_state,_p_ftp_pipe->_error_code);
#endif
    /* Check if the timer is running */
    if(_p_ftp_pipe->_retry_connect_timer_id != 0)
    {
        CANCEL_TIMER(_p_ftp_pipe->_retry_connect_timer_id);
        return SUCCESS;

    }
    else if(_p_ftp_pipe->_pasv._retry_timer_id != 0)
    {
        CANCEL_TIMER(_p_ftp_pipe->_pasv._retry_timer_id);
        return SUCCESS;

    }
    else if(_p_ftp_pipe->_retry_get_buffer_timer_id != 0)
    {
        CANCEL_TIMER(_p_ftp_pipe->_retry_get_buffer_timer_id);
        return SUCCESS;

    }
    else if (_p_ftp_pipe->_retry_set_file_size_timer_id!=0)
    {
        CANCEL_TIMER(_p_ftp_pipe->_retry_set_file_size_timer_id);
        return SUCCESS;
    }

    uninit_speed_calculator(&(_p_ftp_pipe->_speed_calculator));

    /* Close the FTP data connection */
    if(_p_ftp_pipe->_pasv._socket_fd != 0)
    {
        ftp_pipe_close_pasv( _p_ftp_pipe);
        return SUCCESS;
    }

    /* Close the FTP command connection */
    if(((_p_ftp_pipe->_is_connected == TRUE)||(_p_ftp_pipe->_ftp_state == FTP_PIPE_STATE_CONNECTING))
       &&(_p_ftp_pipe->_ftp_state != FTP_PIPE_STATE_CLOSING))
    {
        ftp_pipe_close_connection( _p_ftp_pipe);

    }
    else if(_p_ftp_pipe->_ftp_state == FTP_PIPE_STATE_CLOSING)
    {
        return FTP_ERR_OPEN_WRONG_STATE;
    }
    else
    {
        //_p_ftp_pipe->_data_pipe._dispatch_info._pipe_state = PIPE_IDLE;
        dp_set_state(&_p_ftp_pipe->_data_pipe, PIPE_IDLE);
        _p_ftp_pipe->_is_connected = FALSE;
        _p_ftp_pipe->_data_pipe._dispatch_info._time_stamp = 0;   ///????
        //_p_ftp_pipe->_sum_recv_bytes = 0;
        _p_ftp_pipe->_ftp_state = FTP_PIPE_STATE_IDLE;
        _p_ftp_pipe->_pasv._state = FTP_PIPE_STATE_IDLE;
        ftp_pipe_destroy(_p_ftp_pipe);

    }


    return SUCCESS;

}


_int32 ftp_pipe_change_ranges(DATA_PIPE * _p_pipe,const RANGE *_alloc_range)
{
    FTP_DATA_PIPE * _p_ftp_pipe = (FTP_DATA_PIPE *)_p_pipe;

    FTP_TRACE( " enter ftp_pipe_change_ranges(_index=%u,_num=%u)..." ,_alloc_range->_index,_alloc_range->_num);

    if ((!_p_ftp_pipe)||(_p_pipe->_data_pipe_type !=FTP_PIPE)) return FTP_ERR_INVALID_PIPE;

    if (_p_ftp_pipe->_wait_delete) return FTP_ERR_WAITING_DELETE;

    if (_alloc_range->_num == 0) return FTP_ERR_INVALID_RANGES;

    if(_p_ftp_pipe->_data_pipe._dispatch_info._pipe_state == PIPE_FAILURE)
        return FTP_ERR_WRONG_STATE;

    if((_p_ftp_pipe->_data_pipe._dispatch_info._is_support_range == FALSE)&&(_alloc_range->_index!=0))
        return FTP_ERR_RANGE_NOT_SUPPORT;

    if(0==dp_get_uncomplete_ranges_list_size(&(_p_ftp_pipe->_data_pipe)))
    {
        _p_ftp_pipe->_error_code = dp_add_uncomplete_ranges(&(_p_ftp_pipe->_data_pipe), _alloc_range);
        if(_p_ftp_pipe->_error_code)        goto ErrorHanle;
    }
    else
    {
        _p_ftp_pipe->_error_code = dp_clear_uncomplete_ranges_list(&(_p_ftp_pipe->_data_pipe));
        if(_p_ftp_pipe->_error_code)        goto ErrorHanle;
        _p_ftp_pipe->_error_code = dp_add_uncomplete_ranges(&(_p_ftp_pipe->_data_pipe), _alloc_range);
        if(_p_ftp_pipe->_error_code)        goto ErrorHanle;
    }

    if(!_p_ftp_pipe->_b_ranges_changed )
        _p_ftp_pipe->_b_ranges_changed =TRUE;

    FTP_TRACE( "  _pipe_state=%d,_ftp_state=%d _b_ranges_changed=%d ",_p_ftp_pipe->_data_pipe._dispatch_info._pipe_state,_p_ftp_pipe->_ftp_state,_p_ftp_pipe->_b_ranges_changed);
    if((_p_ftp_pipe->_data_pipe._dispatch_info._pipe_state == PIPE_DOWNLOADING)&&(_p_ftp_pipe->_ftp_state == FTP_PIPE_STATE_CONNECTED))
    {
        /* change to the work drectory */
        _p_ftp_pipe->_error_code = ftp_pipe_send_command(_p_ftp_pipe,FTP_COMMAND_CWD);

        if(_p_ftp_pipe->_error_code)        goto ErrorHanle;

    }
    else if(((_p_ftp_pipe->_data_pipe._dispatch_info._pipe_state == PIPE_IDLE)||(_p_ftp_pipe->_data_pipe._dispatch_info._pipe_state == PIPE_DOWNLOADING))
            &&((_p_ftp_pipe->_ftp_state == FTP_PIPE_STATE_DOWNLOAD_COMPLETED)||(_p_ftp_pipe->_ftp_state == FTP_PIPE_STATE_IDLE)))
    {
        if(_p_ftp_pipe->_is_connected == TRUE)
        {
            if(_p_ftp_pipe->_data_pipe._dispatch_info._is_support_long_connect == TRUE)
            {
                /* ready to enter passive mode */
                _p_ftp_pipe->_error_code = ftp_pipe_send_command(_p_ftp_pipe,FTP_COMMAND_PASV);

            }
            else
            {
                /* Reopen the socket */
                _p_ftp_pipe->_error_code = ftp_pipe_open((DATA_PIPE *) _p_ftp_pipe);
            }

            if(_p_ftp_pipe->_error_code)        goto ErrorHanle;
        }
    }


    return SUCCESS;

ErrorHanle:
    ftp_pipe_failure_exit( _p_ftp_pipe);
    return _p_ftp_pipe->_error_code;


} /* End of ftp_pipe_change_ranges */

_int32 ftp_pipe_destroy(FTP_DATA_PIPE * _p_ftp_pipe)
{
    _int32 ret_val = SUCCESS;
    FTP_TRACE( " enter ftp_pipe_destroy(0x%X)...",_p_ftp_pipe );

    if ((!_p_ftp_pipe)||(_p_ftp_pipe->_data_pipe._data_pipe_type !=FTP_PIPE)) return FTP_ERR_INVALID_PIPE;

    ftp_pipe_reset_pipe( _p_ftp_pipe);
    ret_val = mpool_free_slip(gp_ftp_pipe_slab,(void*) _p_ftp_pipe);
    CHECK_VALUE(ret_val);

    LOG_DEBUG( "++++ftp_pipe_destroy SUCCESS!" );

    return SUCCESS;

}



/* Interface for connect_manager getting the pipe speed */
_int32 ftp_pipe_get_speed(DATA_PIPE * _p_pipe)
{
    _int32 ret_val = SUCCESS;
    FTP_DATA_PIPE * _p_ftp_pipe = (FTP_DATA_PIPE *)_p_pipe;
    FTP_TRACE( " enter ftp_pipe_get_speed()..." );

    if ((!_p_ftp_pipe)||(_p_pipe->_data_pipe_type !=FTP_PIPE)) return FTP_ERR_INVALID_PIPE;

    //if(_p_ftp_pipe->_b_data_full != TRUE)
    {
        ret_val =calculate_speed(&(_p_ftp_pipe->_speed_calculator), &(_p_ftp_pipe->_data_pipe._dispatch_info._speed));
        CHECK_VALUE(ret_val);

    }
    FTP_TRACE( " enter ftp_pipe_get_speed=%u" ,_p_ftp_pipe->_data_pipe._dispatch_info._speed);
    return SUCCESS;
}

///////////////////////////////////////////////////////////

//            Callback function
///////////////////////////////////////////////////////////



_int32 ftp_pipe_handle_connect( _int32 _errcode, _u32 notice_count_left,void *user_data)
{
    FTP_DATA_PIPE * _p_ftp_pipe = (FTP_DATA_PIPE *)user_data;
    RANGE all_range;

    FTP_TRACE( " enter ftp_pipe_handle_connect(%d)...",_errcode );


    if (!_p_ftp_pipe) return FTP_ERR_INVALID_PIPE;

    if((_p_ftp_pipe->_ftp_state != FTP_PIPE_STATE_CONNECTING)&&(_p_ftp_pipe->_ftp_state != FTP_PIPE_STATE_CLOSING))
        return FTP_ERR_WRONG_STATE;

    if(_errcode == SUCCESS)
    {
        /* FTP Connecting success,wait for welcome msg */
        FTP_TRACE( "Connecting SUCCESS" );
        _p_ftp_pipe->_ftp_state = FTP_PIPE_STATE_READING;
        _p_ftp_pipe->_cur_command_id = 0;

        _p_ftp_pipe->_is_connected = TRUE;
        _p_ftp_pipe->_data_pipe._dispatch_info._time_stamp = 0;   ///????
        //_p_ftp_pipe->_sum_recv_bytes = 0;
        _p_ftp_pipe->_error_code = sd_time_ms(&(_p_ftp_pipe->_data_pipe._dispatch_info._time_stamp));
        if(_p_ftp_pipe->_error_code== SUCCESS)
        {

            /* Init the response struct */
            ftp_pipe_reset_response( &(_p_ftp_pipe->_response) );

            /* reset the speed */
            //uninit_speed_calculator(&(_p_ftp_pipe->_speed_calculator));
            //init_speed_calculator(&(_p_ftp_pipe->_speed_calculator), 10, 1000);
            //calculate_speed(&(_p_ftp_pipe->_speed_calculator), &(_p_ftp_pipe->_data_pipe._dispatch_info._speed));
            if(dp_get_can_download_ranges_list_size( &(_p_ftp_pipe->_data_pipe) ) ==0)
            {
                all_range._index=0;
                all_range._num=MAX_U32;
                dp_add_can_download_ranges( &(_p_ftp_pipe->_data_pipe), &all_range );
            }


            /* malloc memory for reading the welcome msg */
            _p_ftp_pipe->_response._buffer_length = FTP_RESPN_DEFAULT_LEN;

            _p_ftp_pipe->_error_code = sd_malloc(_p_ftp_pipe->_response._buffer_length+2,  (void **)& (_p_ftp_pipe->_response._buffer));
            if(_p_ftp_pipe->_error_code==SUCCESS)
            {
                //sd_memset(_p_ftp_pipe->_response._buffer,0,_p_ftp_pipe->_response._buffer_length+2);
                /* Read the welcome msg from socket */
                /*
                FTP_TRACE( "call  SOCKET_PROXY_UNCOMPLETE_RECV(_socket_fd=%d,_buffer_length=%u)" ,_p_ftp_pipe->_socket_fd,_p_ftp_pipe->_response._buffer_length);
                _p_ftp_pipe->_error_code = SOCKET_PROXY_UNCOMPLETE_RECV(_p_ftp_pipe->_socket_fd, _p_ftp_pipe->_response._buffer,
                                            _p_ftp_pipe->_response._buffer_length, ftp_pipe_handle_uncomplete_recv,(void*)_p_ftp_pipe);
                */
                _p_ftp_pipe->_error_code=ftp_pipe_send_command(_p_ftp_pipe,FTP_COMMAND_USER);

            }
        }
        if(_p_ftp_pipe->_error_code!=SUCCESS)
            ftp_pipe_failure_exit( _p_ftp_pipe);
    }
    else if((_errcode == MSG_CANCELLED)&&(_p_ftp_pipe->_ftp_state == FTP_PIPE_STATE_CLOSING))
    {

        FTP_TRACE( "Connecting MSG_CANCELLED" );
        ftp_pipe_close_connection( _p_ftp_pipe);
    }
    else
    {
        FTP_TRACE( "Connecting FAILED" );
        if((_int32)_p_ftp_pipe->_already_try_count >= MAX_RETRY_CONNECT_TIMES)
        {
            /* Failed */
            FTP_TRACE( "call SOCKET_PROXY_CLOSE(fd=%u)",_p_ftp_pipe->_socket_fd );
            SOCKET_PROXY_CLOSE(_p_ftp_pipe->_socket_fd);
            _p_ftp_pipe->_socket_fd=0;
            _p_ftp_pipe->_error_code = _errcode;

        }
        else
        {
            FTP_TRACE( "call SOCKET_PROXY_CLOSE(fd=%u)",_p_ftp_pipe->_socket_fd );
            _p_ftp_pipe->_error_code = SOCKET_PROXY_CLOSE(_p_ftp_pipe->_socket_fd);
            _p_ftp_pipe->_socket_fd=0;
            if(_p_ftp_pipe->_error_code == SUCCESS)
            {
                /* Try again later... */
                _p_ftp_pipe->_ftp_state = FTP_PIPE_STATE_RETRY_WAITING;
                /* Start timer */
                FTP_TRACE( "call START_TIMER" );
                _p_ftp_pipe->_error_code =START_TIMER(ftp_pipe_handle_timeout, NOTICE_ONCE,RETRY_CONNECT_WAIT_M_SEC,0, _p_ftp_pipe,&(_p_ftp_pipe->_retry_connect_timer_id));
            }
        }
        if(_p_ftp_pipe->_error_code !=SUCCESS)
            ftp_pipe_failure_exit( _p_ftp_pipe);
    }

    return SUCCESS;

}
_int32 ftp_pipe_handle_pasv_connect( _int32 _errcode, _u32 notice_count_left,void *user_data)
{
    FTP_DATA_PIPE * _p_ftp_pipe = (FTP_DATA_PIPE *)user_data;

    FTP_TRACE( " enter ftp_pipe_handle_pasv_connect(%d)...",_errcode );


    if (!_p_ftp_pipe) return FTP_ERR_INVALID_PIPE;

    if((_p_ftp_pipe->_pasv._state != FTP_PIPE_STATE_CONNECTING)&&(_p_ftp_pipe->_pasv._state != FTP_PIPE_STATE_CLOSING))
        return FTP_ERR_WRONG_STATE;

    if(_errcode == SUCCESS)
    {
        FTP_TRACE( "Passive Mode Connecting SUCCESS" );
        /* FTP data channel Connecting success,start receiving file data */
        _p_ftp_pipe->_pasv._is_connected = TRUE;

        /* Start download the file data  */
        _p_ftp_pipe->_error_code = ftp_pipe_send_command(_p_ftp_pipe,FTP_COMMAND_RETR);

        if(_p_ftp_pipe->_error_code!=SUCCESS)
            ftp_pipe_failure_exit( _p_ftp_pipe);

    }
    else if((_errcode == MSG_CANCELLED)&&(_p_ftp_pipe->_pasv._state == FTP_PIPE_STATE_CLOSING))
    {
        FTP_TRACE( "Passive Mode Connecting MSG_CANCELLED" );
        ftp_pipe_close_pasv(_p_ftp_pipe);
    }
    else
    {
        FTP_TRACE( "Passive Mode Connecting FAILED" );
        if((_int32)_p_ftp_pipe->_pasv._try_count >= MAX_RETRY_CONNECT_TIMES)
        {
            /* Failed */
            FTP_TRACE( "call SOCKET_PROXY_CLOSE(_pasv.fd=%u)",_p_ftp_pipe->_pasv._socket_fd );
            SOCKET_PROXY_CLOSE(_p_ftp_pipe->_pasv._socket_fd);
            _p_ftp_pipe->_pasv._socket_fd=0;
            _p_ftp_pipe->_error_code = _errcode;

        }
        else
        {
            FTP_TRACE( "call SOCKET_PROXY_CLOSE(_pasv.fd=%u)",_p_ftp_pipe->_pasv._socket_fd );
            _p_ftp_pipe->_error_code = SOCKET_PROXY_CLOSE(_p_ftp_pipe->_pasv._socket_fd);
            _p_ftp_pipe->_pasv._socket_fd=0;
            if(_p_ftp_pipe->_error_code == SUCCESS)
            {
                /* Try again later... */
                _p_ftp_pipe->_pasv._state = FTP_PIPE_STATE_RETRY_WAITING;
                /* Start timer */
                FTP_TRACE( "call START_TIMER" );
                _p_ftp_pipe->_error_code =START_TIMER(ftp_pipe_handle_timeout, NOTICE_ONCE,RETRY_CONNECT_WAIT_M_SEC,0,  _p_ftp_pipe,&(_p_ftp_pipe->_pasv._retry_timer_id));
            }
        }
        if(_p_ftp_pipe->_error_code!=SUCCESS )
            ftp_pipe_failure_exit( _p_ftp_pipe);
    }

    return SUCCESS;

}

_int32 ftp_pipe_handle_send( _int32 _errcode, _u32 notice_count_left,const char* buffer, _u32 had_send,void *user_data)
{
    FTP_DATA_PIPE * _p_ftp_pipe = (FTP_DATA_PIPE *)user_data;

    FTP_TRACE( " enter ftp_pipe_handle_send(_errcode=%d,had_send=%u)..." ,_errcode ,had_send);

    if (!_p_ftp_pipe) return FTP_ERR_INVALID_PIPE;

    if((_p_ftp_pipe->_ftp_state != FTP_PIPE_STATE_REQUESTING)&&(_p_ftp_pipe->_ftp_state != FTP_PIPE_STATE_CLOSING))
        return FTP_ERR_WRONG_STATE;

    if(_errcode == SUCCESS)
    {
        FTP_TRACE( "Sending SUCCESS" );
        /* Sending command success,wait for the response from FTP server */
        _p_ftp_pipe->_ftp_state = FTP_PIPE_STATE_READING;

        if(_p_ftp_pipe->_response._buffer != NULL)
        {
            /* Read the response from socket */
            _p_ftp_pipe->_response._buffer_end_pos = 0;
            _p_ftp_pipe->_response._code = 0;
            SAFE_DELETE(_p_ftp_pipe->_response._description);
            sd_memset(_p_ftp_pipe->_response._buffer,0,_p_ftp_pipe->_response._buffer_length+2);

            FTP_TRACE( "call  SOCKET_PROXY_UNCOMPLETE_RECV(_socket_fd=%d,_buffer_length=%u)" ,_p_ftp_pipe->_socket_fd,_p_ftp_pipe->_response._buffer_length);
            _p_ftp_pipe->_error_code = SOCKET_PROXY_UNCOMPLETE_RECV(_p_ftp_pipe->_socket_fd, _p_ftp_pipe->_response._buffer,
                                       _p_ftp_pipe->_response._buffer_length, ftp_pipe_handle_uncomplete_recv,(void*)_p_ftp_pipe);


        }
        else
        {
            _p_ftp_pipe->_error_code = FTP_ERR_UNKNOWN;
#ifdef _DEBUG
            CHECK_VALUE(1);
#endif
        }

        if(_p_ftp_pipe->_error_code!=SUCCESS)
            ftp_pipe_failure_exit( _p_ftp_pipe);
    }
    else if((_errcode == MSG_CANCELLED)&&(_p_ftp_pipe->_ftp_state == FTP_PIPE_STATE_CLOSING))
    {
        FTP_TRACE( "Sending MSG_CANCELLED" );
        ftp_pipe_close_connection( _p_ftp_pipe);

    }
    else
    {
        /* Failed */
        FTP_TRACE( "Sending FAILED" );
        _p_ftp_pipe->_error_code = _errcode;
        ftp_pipe_failure_exit( _p_ftp_pipe);
    }

    return SUCCESS;

}
_int32 ftp_pipe_handle_recv( _int32 _errcode, _u32 notice_count_left,char* buffer, _u32 had_recv,void *user_data)
{
    //_u32 _cur_time =0;
    FTP_DATA_PIPE * _p_ftp_pipe = (FTP_DATA_PIPE *)user_data;

    FTP_TRACE( " enter ftp_pipe_handle_recv(_errcode=%d,had_recv=%u)..." ,_errcode ,had_recv);

    if (!_p_ftp_pipe) return FTP_ERR_INVALID_PIPE;

    if((_p_ftp_pipe->_pasv._state != FTP_PIPE_STATE_READING)&&(_p_ftp_pipe->_pasv._state != FTP_PIPE_STATE_CLOSING))
        return FTP_ERR_WRONG_STATE;

    if(_errcode == SUCCESS)
    {
        FTP_TRACE( "Receiving SUCCESS" );
        sd_time_ms(&(_p_ftp_pipe->_data_pipe._dispatch_info._last_recv_data_time));
        if(had_recv != 0)
        {
            /* Receiving data success from server */
            _p_ftp_pipe->_sum_recv_bytes+=had_recv;
            _p_ftp_pipe->_data._buffer_end_pos += had_recv;
            _p_ftp_pipe->_data._recv_end_pos += had_recv;

            add_speed_record(&(_p_ftp_pipe->_speed_calculator), had_recv);
            //_p_ftp_pipe->_error_code =calculate_speed(&(_p_ftp_pipe->_speed_calculator), &(_p_ftp_pipe->_data_pipe._dispatch_info._speed));

            FTP_TRACE( "_connected_time=%u,_speed=%u",_p_ftp_pipe->_data_pipe._dispatch_info._time_stamp ,_p_ftp_pipe->_data_pipe._dispatch_info._speed);

            if(_p_ftp_pipe->_error_code ==SUCCESS)
            {
                FTP_TRACE( "_buffer_end_pos=%u,_data_length=%u",_p_ftp_pipe->_data._buffer_end_pos ,_p_ftp_pipe->_data._data_length);
                if(_p_ftp_pipe->_data._buffer_end_pos == _p_ftp_pipe->_data._data_length)
                {
                    /* range succss */
                    _p_ftp_pipe->_error_code = ftp_pipe_recving_range_success( _p_ftp_pipe);
                }
                /*
                else
                {
                    _p_ftp_pipe->_error_code = FTP_ERR_UNKNOWN;

                }
                */
            }

        }
        else
        {
            /* connectiong closed by server */
            FTP_TRACE( " received 0 byte meaning remote server closed the conection!" );
            _p_ftp_pipe->_error_code = FTP_ERR_SERVER_DISCONNECTED;
            _p_ftp_pipe->_is_connected = FALSE;
            _p_ftp_pipe->_data_pipe._dispatch_info._time_stamp = 0;   ///????
            //_p_ftp_pipe->_sum_recv_bytes = 0;
        }

        if(_p_ftp_pipe->_error_code!=SUCCESS)
            ftp_pipe_failure_exit( _p_ftp_pipe);
    }
    else if((_errcode == MSG_CANCELLED)&&(_p_ftp_pipe->_pasv._state == FTP_PIPE_STATE_CLOSING))
    {
        FTP_TRACE( "Receiving MSG_CANCELLED:_wait_delete%d ",_p_ftp_pipe->_wait_delete );
        if(_p_ftp_pipe->_wait_delete==TRUE)
        {
            ftp_pipe_close_pasv(_p_ftp_pipe);
            return SUCCESS;
        }
        else
        {
            _p_ftp_pipe->_error_code = ftp_pipe_close_pasv(_p_ftp_pipe);
            if(_p_ftp_pipe->_error_code!=SUCCESS)
                ftp_pipe_failure_exit( _p_ftp_pipe);
        }
    }
    else
    {
        FTP_TRACE( "Receiving FAILED" );
        /* Failed */
        _p_ftp_pipe->_error_code = _errcode;
        ftp_pipe_failure_exit( _p_ftp_pipe);
    }

    FTP_TRACE( "pipe_state=%d,_ftp_state=%d,_pasv._state=%d",_p_ftp_pipe->_data_pipe._dispatch_info._pipe_state,_p_ftp_pipe->_ftp_state,_p_ftp_pipe->_pasv._state );

    /************************************************************************/

    if(_p_ftp_pipe->_pasv._state == FTP_PIPE_STATE_READING)
    {
        FTP_TRACE( "FTP_PIPE_STATE_READING:_b_data_full=%d,_data_length=%u,_buffer_end_pos=%u",_p_ftp_pipe->_b_data_full,_p_ftp_pipe->_data._data_length ,_p_ftp_pipe->_data._buffer_end_pos );
        /* continue */
        if(_p_ftp_pipe->_b_data_full!=TRUE)
        {
            if(_p_ftp_pipe->_data._data_length > _p_ftp_pipe->_data._buffer_end_pos)
            {
                _u32 _block_len = socket_proxy_get_p2s_recv_block_size();//_p_ftp_pipe->_reveive_block_len;

                if(_p_ftp_pipe->_data._data_length-_p_ftp_pipe->_data._buffer_end_pos<_block_len)
                    _block_len = _p_ftp_pipe->_data._data_length-_p_ftp_pipe->_data._buffer_end_pos;

                FTP_TRACE( "call SOCKET_PROXY_RECV(_pasv._socket_fd=%u)",_p_ftp_pipe->_pasv._socket_fd );
                _p_ftp_pipe->_error_code = SOCKET_PROXY_RECV(_p_ftp_pipe->_pasv._socket_fd,
                                           _p_ftp_pipe->_data._buffer+_p_ftp_pipe->_data._buffer_end_pos,
                                           _block_len,
                                           ftp_pipe_handle_recv,(void*)_p_ftp_pipe);
            }
            else
            {
                FTP_TRACE( "FTP_ERR_UNKNOWN");
#ifdef _DEBUG
                CHECK_VALUE(1);
#endif
                _p_ftp_pipe->_error_code = FTP_ERR_UNKNOWN;
            }
        }

        if(_p_ftp_pipe->_error_code!=SUCCESS)
            ftp_pipe_failure_exit( _p_ftp_pipe);


    }
    else if(_p_ftp_pipe->_pasv._state == FTP_PIPE_STATE_RANGE_CHANGED)
    {
        FTP_TRACE( "FTP_PIPE_STATE_RANGE_CHANGED:_is_support_long_connect=%d",_p_ftp_pipe->_data_pipe._dispatch_info._is_support_long_connect );
        /* download the next range */
        if(_p_ftp_pipe->_data_pipe._dispatch_info._is_support_long_connect==TRUE)
            _p_ftp_pipe->_error_code = ftp_pipe_send_command(_p_ftp_pipe,FTP_COMMAND_ABOR);
        else
            _p_ftp_pipe->_error_code =  ftp_pipe_send_command(_p_ftp_pipe,FTP_COMMAND_QUIT);

        if(_p_ftp_pipe->_error_code!=SUCCESS)
            ftp_pipe_failure_exit( _p_ftp_pipe);

    }
    else if(_p_ftp_pipe->_pasv._state == FTP_PIPE_STATE_DOWNLOAD_COMPLETED)
    {
        FTP_TRACE( "FTP_PIPE_STATE_DOWNLOAD_COMPLETED:_is_support_long_connect=%d",_p_ftp_pipe->_data_pipe._dispatch_info._is_support_long_connect );
        /* stop transfering */
        if(_p_ftp_pipe->_data_pipe._dispatch_info._is_support_long_connect==TRUE)
            _p_ftp_pipe->_error_code = ftp_pipe_send_command(_p_ftp_pipe,FTP_COMMAND_ABOR);
        else
            _p_ftp_pipe->_error_code =  ftp_pipe_send_command(_p_ftp_pipe,FTP_COMMAND_QUIT);

        if(_p_ftp_pipe->_error_code!=SUCCESS)
            ftp_pipe_failure_exit( _p_ftp_pipe);
    }


    return SUCCESS;
}
_int32 ftp_pipe_handle_uncomplete_recv( _int32 _errcode, _u32 notice_count_left,char* buffer, _u32 had_recv,void *user_data)
{
    //_u32 _cur_time=0;
    FTP_DATA_PIPE * _p_ftp_pipe = (FTP_DATA_PIPE *)user_data;

    FTP_TRACE( " enter ftp_pipe_handle_uncomplete_recv(_errcode=%d,had_recv=%u)..." ,_errcode ,had_recv);

    if (!_p_ftp_pipe) return FTP_ERR_INVALID_PIPE;

    if((_p_ftp_pipe->_ftp_state != FTP_PIPE_STATE_READING)&&(_p_ftp_pipe->_ftp_state != FTP_PIPE_STATE_CLOSING))
        return FTP_ERR_WRONG_STATE;

    if(_errcode == SUCCESS)
    {
        FTP_TRACE( "Uncomplete Receiving SUCCESS" );
        sd_time_ms(&(_p_ftp_pipe->_data_pipe._dispatch_info._last_recv_data_time));
        if(had_recv != 0)
        {
            _p_ftp_pipe->_sum_recv_bytes+=had_recv;
            _p_ftp_pipe->_response._buffer_end_pos += had_recv;

            //_p_ftp_pipe->_error_code =add_speed_record(&(_p_ftp_pipe->_speed_calculator), had_recv);
            //_p_ftp_pipe->_error_code =calculate_speed(&(_p_ftp_pipe->_speed_calculator), &(_p_ftp_pipe->_data_pipe._dispatch_info._speed));

            FTP_TRACE( "_connected_time=%u,_speed=%u",_p_ftp_pipe->_data_pipe._dispatch_info._time_stamp ,_p_ftp_pipe->_data_pipe._dispatch_info._speed);

            if(_p_ftp_pipe->_error_code ==SUCCESS)
            {
                /* Reading success,start parse the ftp header and data */
                _p_ftp_pipe->_error_code = ftp_pipe_parse_response( _p_ftp_pipe);
            }


            if((_p_ftp_pipe->_error_code != SUCCESS)&&(_p_ftp_pipe->_error_code != FTP_ERR_RESPONSE_CODE_NOT_FOUNDE))  //?????
                ftp_pipe_failure_exit( _p_ftp_pipe);
        }
        else
        {
            /* Error */
            FTP_TRACE( " received 0 byte meaning remote server closed the conection!" );
            _p_ftp_pipe->_error_code = FTP_ERR_SERVER_DISCONNECTED;
            _p_ftp_pipe->_is_connected = FALSE;
            _p_ftp_pipe->_data_pipe._dispatch_info._time_stamp = 0;   ///????
            //_p_ftp_pipe->_sum_recv_bytes = 0;
            ftp_pipe_failure_exit( _p_ftp_pipe);
        }

    }
    else if((_errcode == MSG_CANCELLED)&&(_p_ftp_pipe->_ftp_state == FTP_PIPE_STATE_CLOSING))
    {
        FTP_TRACE( "Uncomplete Receiving MSG_CANCELLED:_wait_delete%d ",_p_ftp_pipe->_wait_delete );
        if(_p_ftp_pipe->_wait_delete==TRUE)
        {
            ftp_pipe_close_connection( _p_ftp_pipe);
            return SUCCESS;
        }
        else
        {
            _p_ftp_pipe->_error_code = ftp_pipe_close_connection( _p_ftp_pipe);
            if(_p_ftp_pipe->_error_code!=SUCCESS)
                ftp_pipe_failure_exit( _p_ftp_pipe);
        }
    }
    else
    {
        FTP_TRACE( "Uncomplete Receiving FAILED" );
        /* Failed */
        _p_ftp_pipe->_error_code = _errcode;
        ftp_pipe_failure_exit( _p_ftp_pipe);
    }


    FTP_TRACE( "pipe_state=%d,_ftp_state=%d,_pasv._state=%d",_p_ftp_pipe->_data_pipe._dispatch_info._pipe_state,_p_ftp_pipe->_ftp_state,_p_ftp_pipe->_pasv._state );

    /************************************************************************/

    if(_p_ftp_pipe->_ftp_state == FTP_PIPE_STATE_READING)
    {
        FTP_TRACE( "FTP_PIPE_STATE_READING:_buffer_length=%u,_buffer_end_pos=%u",_p_ftp_pipe->_response._buffer_length ,_p_ftp_pipe->_response._buffer_end_pos );

        if(_p_ftp_pipe->_b_retry_set_file_size != TRUE)
        {
            /* continue */
            if(_p_ftp_pipe->_response._buffer_length > _p_ftp_pipe->_response._buffer_end_pos)
            {
                FTP_TRACE( "call SOCKET_PROXY_UNCOMPLETE_RECV(_socket_fd=%u)",_p_ftp_pipe->_socket_fd );
                _p_ftp_pipe->_error_code = SOCKET_PROXY_UNCOMPLETE_RECV(_p_ftp_pipe->_socket_fd,
                                           _p_ftp_pipe->_response._buffer+_p_ftp_pipe->_response._buffer_end_pos,
                                           _p_ftp_pipe->_response._buffer_length-_p_ftp_pipe->_response._buffer_end_pos,
                                           ftp_pipe_handle_uncomplete_recv,(void*)_p_ftp_pipe);
            }
            else
            {
                FTP_TRACE( "FTP_ERR_UNKNOWN" );
//#ifdef _DEBUG
//                  CHECK_VALUE(1);
//#endif
                _p_ftp_pipe->_error_code = FTP_ERR_UNKNOWN;
            }

            if(_p_ftp_pipe->_error_code!=SUCCESS)
                ftp_pipe_failure_exit( _p_ftp_pipe);

        }


    }
    else if(_p_ftp_pipe->_ftp_state == FTP_PIPE_STATE_RANGE_CHANGED)
    {
        FTP_TRACE( "FTP_PIPE_STATE_RANGE_CHANGED" );
        /* download the next range */
        _p_ftp_pipe->_error_code = ftp_pipe_send_command(_p_ftp_pipe,FTP_COMMAND_PASV);
        if(_p_ftp_pipe->_error_code!=SUCCESS)
            ftp_pipe_failure_exit( _p_ftp_pipe);

    }
    else if(_p_ftp_pipe->_ftp_state == FTP_PIPE_STATE_DOWNLOAD_COMPLETED)
    {
        FTP_TRACE( "_p_ftp_pipe->_ftp_state == FTP_PIPE_STATE_DOWNLOAD_COMPLETED!" );
    }

    return SUCCESS;
}
_u32 ftp_pipe_get_buffer_wait_timeout( FTP_DATA_PIPE * p_ftp_pipe )
{
    _u32 time_out=0;
    if(p_ftp_pipe->_data_pipe._dispatch_info._speed>100000)
    {
        time_out= 50;
    }
    else if(p_ftp_pipe->_data_pipe._dispatch_info._speed>50000)
    {
        time_out= 100;
    }
    else if(p_ftp_pipe->_data_pipe._dispatch_info._speed>20000)
    {
        time_out= 200;
    }
    else
    {
        time_out= 300;
    }

    LOG_DEBUG( " enter[0x%X] ftp_pipe_get_buffer_wait_timeout(FTP_SPEED=%u,time_out=%u)...",p_ftp_pipe,p_ftp_pipe->_data_pipe._dispatch_info._speed,time_out );
    return time_out;
}

_int32 ftp_pipe_handle_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid)
{
    FTP_DATA_PIPE * _p_ftp_pipe = (FTP_DATA_PIPE *)(msg_info->_user_data);
    RANGE all_range;
    _u64 _file_size_from_data_manager=0;
    //_int32 ret_val=SUCCESS;

    FTP_TRACE( " enter ftp_pipe_handle_timeout(errcode=%d,left=%u,expired=%u,msgid=%u)..." ,errcode ,notice_count_left,expired,msgid);

    if (!_p_ftp_pipe) return FTP_ERR_INVALID_PIPE;


    if(errcode == MSG_TIMEOUT)
    {
        if((_p_ftp_pipe->_ftp_state == FTP_PIPE_STATE_RETRY_WAITING))
        {
            /* retry connect time out */
            _p_ftp_pipe->_already_try_count++;
            FTP_TRACE( " _retry_connect_timer_id =%u,_already_try_connect_count=%u", _p_ftp_pipe->_retry_connect_timer_id,_p_ftp_pipe->_already_try_count );
            _p_ftp_pipe->_retry_connect_timer_id = 0;
            /* connect again */
            _p_ftp_pipe->_error_code = SOCKET_PROXY_CREATE(&(_p_ftp_pipe->_socket_fd),SD_SOCK_STREAM);
            if((_p_ftp_pipe->_error_code==SUCCESS)&&(_p_ftp_pipe->_socket_fd != 0))
            {

                LOG_DEBUG("ftp_pipe_handle_timeout _p_ftp_pipe->_socket_fd = %u", _p_ftp_pipe->_socket_fd);
                _p_ftp_pipe->_ftp_state = FTP_PIPE_STATE_CONNECTING;

                FTP_TRACE( "call SOCKET_PROXY_CONNECT(fd=%u,_host=%s,port=%u)" ,_p_ftp_pipe->_socket_fd,_p_ftp_pipe->_p_ftp_server_resource->_url._host,_p_ftp_pipe->_p_ftp_server_resource->_url._port);
                _p_ftp_pipe->_error_code = SOCKET_PROXY_CONNECT(_p_ftp_pipe->_socket_fd,_p_ftp_pipe->_p_ftp_server_resource->_url._host,
                                           _p_ftp_pipe->_p_ftp_server_resource->_url._port,ftp_pipe_handle_connect,(void*)_p_ftp_pipe);
            }
        }
        else if((_p_ftp_pipe->_pasv._state == FTP_PIPE_STATE_RETRY_WAITING))
        {
            /* retry connect time out */
            _p_ftp_pipe->_pasv._try_count++;
            FTP_TRACE( " _pasv._retry_connect_timer_id =%u,_pasv._already_try_connect_count=%u",_p_ftp_pipe->_pasv._retry_timer_id,_p_ftp_pipe->_pasv._try_count  );
            _p_ftp_pipe->_pasv._retry_timer_id = 0;
            /* connect again */
            _p_ftp_pipe->_error_code = SOCKET_PROXY_CREATE(&(_p_ftp_pipe->_pasv._socket_fd),SD_SOCK_STREAM);
            if((_p_ftp_pipe->_error_code==SUCCESS)&&(_p_ftp_pipe->_pasv._socket_fd != 0))
            {
                LOG_DEBUG("ftp_pipe_handle_timeout 2 _p_ftp_pipe->_pasv._socket_fd = %u", _p_ftp_pipe->_pasv._socket_fd);
                _p_ftp_pipe->_pasv._state = FTP_PIPE_STATE_CONNECTING;

                FTP_TRACE( "call SOCKET_PROXY_CONNECT(_pasv.fd=%u,_pasv._host=%s,_pasv.port=%u)" ,_p_ftp_pipe->_pasv._socket_fd,_p_ftp_pipe->_pasv._PASV_ip,_p_ftp_pipe->_pasv._PASV_port);
                _p_ftp_pipe->_error_code = SOCKET_PROXY_CONNECT(_p_ftp_pipe->_pasv._socket_fd,_p_ftp_pipe->_pasv._PASV_ip,
                                           _p_ftp_pipe->_pasv._PASV_port,ftp_pipe_handle_pasv_connect,(void*)_p_ftp_pipe);
            }
        }
        else if((_p_ftp_pipe->_pasv._state == FTP_PIPE_STATE_READING)&&(_p_ftp_pipe->_b_data_full == TRUE))
        {
            /* get buffer time out */
            FTP_TRACE( " _retry_get_buffer_timer_id =%u,_already_try_get_data_count=%u", _p_ftp_pipe->_retry_get_buffer_timer_id,_p_ftp_pipe->_get_buffer_try_count+1 );
            _p_ftp_pipe->_retry_get_buffer_timer_id = 0;
            _p_ftp_pipe->_b_data_full = FALSE;
            pipe_set_err_get_buffer( &(_p_ftp_pipe->_data_pipe), FALSE );

            FTP_TRACE( " DM_GET_DATA_BUFFER(_p_data_manager=0x%X,_p_ftp_pipe=0x%X,_buffer=0x%X,&(_data._buffer)=0x%X,_buffer_length=%u)..." ,
                       _p_ftp_pipe->_data_pipe._p_data_manager,_p_ftp_pipe,_p_ftp_pipe->_data._buffer,&(_p_ftp_pipe->_data._buffer),_p_ftp_pipe->_data._buffer_length);
            sd_assert(_p_ftp_pipe->_data._buffer == NULL);
            _p_ftp_pipe->_error_code =pi_get_data_buffer( (DATA_PIPE *) _p_ftp_pipe ,  &(_p_ftp_pipe->_data._buffer),&( _p_ftp_pipe->_data._buffer_length));
            if(_p_ftp_pipe->_error_code!=SUCCESS)
            {
                //if(_p_ftp_pipe->_get_buffer_try_count++ < MAX_RETRY_TIMES)
                {
                    _p_ftp_pipe->_b_data_full = TRUE;
                    pipe_set_err_get_buffer( &(_p_ftp_pipe->_data_pipe), TRUE );
                    /* Start timer */
                    FTP_TRACE( "call START_TIMER" );
                    _p_ftp_pipe->_error_code =START_TIMER(ftp_pipe_handle_timeout, NOTICE_ONCE,ftp_pipe_get_buffer_wait_timeout(_p_ftp_pipe),0,  _p_ftp_pipe,&(_p_ftp_pipe->_retry_get_buffer_timer_id));
                }//else
                //{
                /* Error */
                //  _p_ftp_pipe->_error_code =  FTP_ERR_GETING_BUFFER;
                //  return ftp_pipe_failure_exit( _p_ftp_pipe);
                //}
            }
            else            /* continue reading data from socket */
                if(_p_ftp_pipe->_data._data_length > _p_ftp_pipe->_data._buffer_end_pos)
                {
                    _u32 _block_len = socket_proxy_get_p2s_recv_block_size();//_p_ftp_pipe->_reveive_block_len;

                    if(_p_ftp_pipe->_data._data_length-_p_ftp_pipe->_data._buffer_end_pos<_block_len)
                        _block_len = _p_ftp_pipe->_data._data_length-_p_ftp_pipe->_data._buffer_end_pos;


                    FTP_TRACE( "call SOCKET_PROXY_RECV(_pasv._socket_fd=%u)", _p_ftp_pipe->_pasv._socket_fd);
                    _p_ftp_pipe->_error_code = SOCKET_PROXY_RECV(_p_ftp_pipe->_pasv._socket_fd,
                                               _p_ftp_pipe->_data._buffer+_p_ftp_pipe->_data._buffer_end_pos,
                                               _block_len,
                                               ftp_pipe_handle_recv,(void*)_p_ftp_pipe);
                }
                else
                {
                    FTP_TRACE( "FTP_ERR_UNKNOWN" );
#ifdef _DEBUG
                    CHECK_VALUE(1);
#endif
                    _p_ftp_pipe->_error_code = FTP_ERR_UNKNOWN;
                }

        }
        else if((_p_ftp_pipe->_b_retry_set_file_size == TRUE)&&(msgid==_p_ftp_pipe->_retry_set_file_size_timer_id))
        {
            _file_size_from_data_manager =pi_get_file_size( (DATA_PIPE *)_p_ftp_pipe);

            /* retry set file time out */
            FTP_TRACE( "_retry_set_file_size_timer_id =%u,_file_size_from_data_manager=%llu,server_resource->_file_size=%llu",_p_ftp_pipe->_retry_set_file_size_timer_id,_file_size_from_data_manager ,_p_ftp_pipe->_p_ftp_server_resource->_file_size );
            _p_ftp_pipe->_retry_set_file_size_timer_id = 0;
            _p_ftp_pipe->_b_retry_set_file_size = FALSE;

            /* Get and set the file size */
            if(_file_size_from_data_manager!=_p_ftp_pipe->_p_ftp_server_resource->_file_size)
            {
                _p_ftp_pipe->_error_code =pi_set_file_size( (DATA_PIPE *)_p_ftp_pipe, _p_ftp_pipe->_p_ftp_server_resource->_file_size);
                if(_p_ftp_pipe->_error_code !=SUCCESS)
                {
                    if(_p_ftp_pipe->_error_code ==FILE_SIZE_NOT_BELIEVE)
                    {
                        /* Start timer to wait dm_set_file_size */
                        _p_ftp_pipe->_b_retry_set_file_size = TRUE;
                        /* Start timer */
                        FTP_TRACE("call START_TIMER to wait dm_set_file_size");
                        _p_ftp_pipe->_error_code =START_TIMER(ftp_pipe_handle_timeout, NOTICE_ONCE,SET_FILE_SIZE_WAIT_M_SEC,0,  _p_ftp_pipe,&(_p_ftp_pipe->_retry_set_file_size_timer_id));
                    }
                    else
                    {
                        FTP_TRACE( "FTP_ERR_INVALID_SERVER_FILE_SIZE" );
                        _p_ftp_pipe->_error_code = FTP_ERR_INVALID_SERVER_FILE_SIZE;
                    }
                }
            }

            if((_p_ftp_pipe->_error_code==SUCCESS)&&(_p_ftp_pipe->_b_retry_set_file_size != TRUE))
            {
                /* Set the _data_pipe._dispatch_info._can_download_ranges ! */
                all_range = pos_length_to_range(0,_p_ftp_pipe->_p_ftp_server_resource->_file_size, _p_ftp_pipe->_p_ftp_server_resource->_file_size);
                //_p_ftp_pipe->_error_code =pi_pos_len_to_valid_range((DATA_PIPE *) _p_ftp_pipe, 0, _p_ftp_pipe->_p_ftp_server_resource->_file_size, &all_range );
                //if(_p_ftp_pipe->_error_code==SUCCESS)
                {

                    _p_ftp_pipe->_error_code =dp_add_can_download_ranges( &(_p_ftp_pipe->_data_pipe), &all_range );
                    if(_p_ftp_pipe->_error_code==SUCCESS)
                    {
                        /* request for passive mode */
                        _p_ftp_pipe->_error_code= ftp_pipe_send_command(_p_ftp_pipe,FTP_COMMAND_PASV);
                    }
                }

            }



        }
        else if((_p_ftp_pipe->_ftp_state != FTP_PIPE_STATE_CLOSING)
                ||(_p_ftp_pipe->_pasv._state != FTP_PIPE_STATE_CLOSING))
        {
            FTP_TRACE( "FTP_ERR_WRONG_TIME_OUT" );
            _p_ftp_pipe->_error_code = FTP_ERR_WRONG_TIME_OUT;
        }


        if(_p_ftp_pipe->_error_code!=SUCCESS)
            ftp_pipe_failure_exit( _p_ftp_pipe);

    }
    else if(errcode == MSG_CANCELLED)
    {
        FTP_TRACE( "Timer MSG_CANCELLED:_retry_connect_timer_id=%u,_pasv._retry_timer_id=%u,_retry_get_buffer_timer_id=%u,_b_retry_set_file_size=%u,_wait_delete=%d",_p_ftp_pipe->_retry_connect_timer_id,_p_ftp_pipe->_pasv._retry_timer_id,_p_ftp_pipe->_retry_get_buffer_timer_id,_p_ftp_pipe->_b_retry_set_file_size, _p_ftp_pipe->_wait_delete);
        if(_p_ftp_pipe->_retry_connect_timer_id ==msgid)
            _p_ftp_pipe->_retry_connect_timer_id = 0;
        else if(_p_ftp_pipe->_pasv._retry_timer_id ==msgid)
            _p_ftp_pipe->_pasv._retry_timer_id = 0;
        else if(_p_ftp_pipe->_retry_get_buffer_timer_id ==msgid)
            _p_ftp_pipe->_retry_get_buffer_timer_id = 0;
        else if(_p_ftp_pipe->_retry_set_file_size_timer_id ==msgid)
            _p_ftp_pipe->_retry_set_file_size_timer_id = 0;

        if(_p_ftp_pipe->_wait_delete)
        {
            ftp_pipe_close((DATA_PIPE *) _p_ftp_pipe);
        }
    }
    else
    {
        FTP_TRACE( "Timer Error" );
        _p_ftp_pipe->_error_code = errcode;
        ftp_pipe_failure_exit( _p_ftp_pipe);
    }


    return SUCCESS;
}


///////////////////////////////////////////////////////////

//            internal function
///////////////////////////////////////////////////////////

_int32 ftp_pipe_failure_exit(FTP_DATA_PIPE * _p_ftp_pipe)
{

    FTP_TRACE( " enter ftp_pipe_failure_exit(pipe state = %d,ftp_state = %d,_pasv._state = %d,error code = %d)...",_p_ftp_pipe->_data_pipe._dispatch_info._pipe_state,
               _p_ftp_pipe->_ftp_state,_p_ftp_pipe->_pasv._state,_p_ftp_pipe->_error_code );

#ifdef _DEBUG
    if( _p_ftp_pipe->_p_ftp_server_resource->_b_is_origin==TRUE )
    {
        LOG_ERROR( "+++ [_p_ftp_pipe=0x%X]The origin resource:%s, ERROR=%d !! ",_p_ftp_pipe,_p_ftp_pipe->_p_ftp_server_resource->_o_url,_p_ftp_pipe->_error_code);
    }
    else
    {
        LOG_ERROR( "+++ [_p_ftp_pipe=0x%X]The inquire resource:%s, ERROR=%d !!",_p_ftp_pipe,_p_ftp_pipe->_p_ftp_server_resource->_o_url,_p_ftp_pipe->_error_code );
    }
#endif


    /* Set resource error code */
    set_resource_err_code(  (RESOURCE*)(_p_ftp_pipe->_p_ftp_server_resource), (_u32) _p_ftp_pipe->_error_code );

    /* halt the program for debug */
    //CHECK_VALUE(_p_ftp_pipe->_error_code);
    if(_p_ftp_pipe->_error_code==FTP_ERR_INVALID_SERVER_FILE_SIZE
        || _p_ftp_pipe->_error_code==FTP_ERR_FILE_NOT_FOUND )
    {
        FTP_TRACE( "+++ The resource:%s  is abandoned !!",_p_ftp_pipe->_p_ftp_server_resource->_url._host);
        set_resource_state( _p_ftp_pipe->_data_pipe._p_resource, ABANDON );
    }

    if(_p_ftp_pipe->_p_ftp_server_resource->_b_is_origin==TRUE)
    {
        if(_p_ftp_pipe->_data_pipe._p_resource->_retry_times + 1>= _p_ftp_pipe->_data_pipe._p_resource->_max_retry_time  )
        {
            //LOG_ERROR( "oooo [_p_ftp_pipe=0x%X]The origin resource:%s, ERROR=%d  ,retry_times=%u,max_retry_time=%u!! ",_p_ftp_pipe,_p_ftp_pipe->_p_ftp_server_resource->_o_url,_p_ftp_pipe->_error_code,_p_ftp_pipe->_data_pipe._p_resource->_retry_times, _p_ftp_pipe->_data_pipe._p_resource->_max_retry_time);
            //pt_set_origin_mode((TASK *) dp_get_task_ptr( (DATA_PIPE *)_p_ftp_pipe ),FALSE);
        }
    }

    //_p_ftp_pipe->_data_pipe._dispatch_info._pipe_state = PIPE_FAILURE;
    dp_set_state(&_p_ftp_pipe->_data_pipe, PIPE_FAILURE);
    _p_ftp_pipe->_ftp_state = FTP_PIPE_STATE_UNKNOW;
    _p_ftp_pipe->_pasv._state = FTP_PIPE_STATE_UNKNOW;

    return SUCCESS;

} /* End of ftp_pipe_failure_exit */



_int32 ftp_pipe_send_command(FTP_DATA_PIPE * _p_ftp_pipe,_int32 _command_id)
{
    _u64 _start_pos=0,_remain_len=0 ;
    char *pos1 =NULL,*pos2=NULL;
    char command[MAX_FULL_PATH_BUFFER_LEN+256 ] = {0};
    char tmp_buffer[MAX_URL_LEN]= {0};
    _int32 ret_val=SUCCESS;
    _int32 length = 0;
    RANGE uncom_head_range,down_range;

    FTP_TRACE( " enter ftp_pipe_send_command(%d)...",_command_id );

    sd_memset(command,0,MAX_FULL_PATH_BUFFER_LEN+256);

    switch(_command_id)
    {
        case FTP_COMMAND_USER:
        {
            /* send the user name */
            sd_strcat(command,"USER ",sd_strlen("USER "));
            if(_p_ftp_pipe->_p_ftp_server_resource->_url._user[0]!= '\0')
            {
                sd_strcat(command,_p_ftp_pipe->_p_ftp_server_resource->_url._user,sd_strlen(_p_ftp_pipe->_p_ftp_server_resource->_url._user));
            }
            else
            {
                sd_strcat(command,"anonymous",sd_strlen("anonymous"));
            }

            break;
        }

        case FTP_COMMAND_PASWD:
        {
            /* send the pass word */
            sd_strcat(command,"PASS ",sd_strlen("PASS "));
            if(_p_ftp_pipe->_p_ftp_server_resource->_url._password[0]!= '\0')
            {
                sd_strcat(command,_p_ftp_pipe->_p_ftp_server_resource->_url._password,sd_strlen(_p_ftp_pipe->_p_ftp_server_resource->_url._password));
            }
            else
            {
                sd_strcat(command,"yourname@yourcompany.com",sd_strlen("yourname@yourcompany.com"));
            }
            break;
        }
        case FTP_COMMAND_FEAT:
        {
            /* send the FEAT */
            sd_strcat(command,"FEAT",sd_strlen("FEAT"));
            break;
        }

        case FTP_COMMAND_CWD:
        {
            /* send the change work directory */
            sd_strcat(command,"CWD ",sd_strlen("CWD "));
            if (_p_ftp_pipe->_p_ftp_server_resource->_url._path) sd_trim_postfix_lws(_p_ftp_pipe->_p_ftp_server_resource->_url._path);
            if(_p_ftp_pipe->_p_ftp_server_resource->_url._path!= NULL && 
                        sd_strlen(_p_ftp_pipe->_p_ftp_server_resource->_url._path)>0 && 
                        sd_strncmp(_p_ftp_pipe->_p_ftp_server_resource->_url._path, "/", _p_ftp_pipe->_p_ftp_server_resource->_url._path_len)!=0)
            {
                if(_p_ftp_pipe->_b_retry_request == TRUE)
                {
                    _p_ftp_pipe->_b_retry_request =FALSE;
                    /* Convert the path format */
                    ret_val = url_convert_format( _p_ftp_pipe->_p_ftp_server_resource->_url._path,_p_ftp_pipe->_p_ftp_server_resource->_url._path_len,tmp_buffer ,MAX_URL_LEN ,&_p_ftp_pipe->_request_step);
                    if(ret_val!=SUCCESS) return ret_val;
                    sd_strcat(command,tmp_buffer,sd_strlen(tmp_buffer));
                }
                else
                {
                    if((_p_ftp_pipe->_p_ftp_server_resource->_url._path_valid_step!=UCS_ORIGIN)&&(_p_ftp_pipe->_p_ftp_server_resource->_url._path_valid_step!=UCS_END))
                    {
                        /* Convert the path format */
                        ret_val = url_convert_format_to_step( _p_ftp_pipe->_p_ftp_server_resource->_url._path,_p_ftp_pipe->_p_ftp_server_resource->_url._path_len,tmp_buffer ,MAX_URL_LEN ,_p_ftp_pipe->_p_ftp_server_resource->_url._path_valid_step);
                        CHECK_VALUE(ret_val);
                        sd_strcat(command,tmp_buffer,sd_strlen(tmp_buffer));
                    }
                    else
                    {
                        sd_strcat(command,_p_ftp_pipe->_p_ftp_server_resource->_url._path,_p_ftp_pipe->_p_ftp_server_resource->_url._path_len);
                    }
                }

                if(command[sd_strlen(command)-1] == '/')
                    command[sd_strlen(command)-1] = '\0';
                pos1 = sd_strrchr(command,'/');
                pos2 = sd_strchr(command,'/',0);
                if((pos1!=NULL)&&(pos2!=NULL)&&(pos1==pos2))
                    sd_strncpy(pos1, pos1+1, sd_strlen(pos1+1)+1);

            }
            else
            {
                FTP_TRACE( "ftp_pipe_send_command: path is root");
                sd_strcat(command,"./",sd_strlen("./"));
            }
            break;
        }
        case FTP_COMMAND_TYPE_I:
        {
            /* send the type I */
            sd_strcat(command,"TYPE I",sd_strlen("TYPE I"));
            break;
        }
        case FTP_COMMAND_MDTM:
        {
            /* send the command to get the last modify time of the file  */
            sd_strcat(command,"MDTM ",sd_strlen("MDTM "));
            if(_p_ftp_pipe->_p_ftp_server_resource->_url._file_name!= NULL)
            {
                sd_strcat(command,_p_ftp_pipe->_p_ftp_server_resource->_url._file_name,_p_ftp_pipe->_p_ftp_server_resource->_url._file_name_len);
            }
            else
            {
                return FTP_ERR_INVALID_FILE_NAME;
            }
            break;
        }

        case FTP_COMMAND_SIZE:
        {
            /* send the size of the file  */
            sd_strcat(command,"SIZE ",sd_strlen("SIZE "));
            if(_p_ftp_pipe->_p_ftp_server_resource->_url._file_name!=NULL)
            {
                sd_strcat(command,_p_ftp_pipe->_p_ftp_server_resource->_url._file_name,_p_ftp_pipe->_p_ftp_server_resource->_url._file_name_len);
            }
            else
            {
                return FTP_ERR_INVALID_FILE_NAME;
            }
            break;
        }
        case FTP_COMMAND_PASV:
        {
            /* send the chnage to passive mode */
            sd_strcat(command,"PASV",sd_strlen("PASV"));
            break;
        }
        case FTP_COMMAND_REST:
        {
            /* send the restart from XXXX */
            _p_ftp_pipe->_error_code = ftp_pipe_reset_data( _p_ftp_pipe,&(_p_ftp_pipe->_data) );
            if(_p_ftp_pipe->_error_code)
            {
                return _p_ftp_pipe->_error_code;
            }

            /*--------------- Range ----------------------*/
            if( 0!=dp_get_uncomplete_ranges_list_size(&(_p_ftp_pipe->_data_pipe)))
            {
                _p_ftp_pipe->_error_code = dp_get_uncomplete_ranges_head_range(&(_p_ftp_pipe->_data_pipe), &uncom_head_range);
                if(_p_ftp_pipe->_error_code )
                {
                    /* Error */
                    return _p_ftp_pipe->_error_code  ;
                }
                else
                {

                    down_range._index = uncom_head_range._index;

                    if(uncom_head_range._num>_p_ftp_pipe->_reveive_range_num)
                        down_range._num = _p_ftp_pipe->_reveive_range_num;
                    else
                        down_range._num = uncom_head_range._num;

                    _p_ftp_pipe->_error_code = dp_set_download_range( (DATA_PIPE *)_p_ftp_pipe, &down_range );
                    if(_p_ftp_pipe->_error_code )
                    {
                        /* Error */
                        return _p_ftp_pipe->_error_code  ;
                    }
                    _start_pos = (down_range._index)*FTP_DEFAULT_RANGE_LEN;


                    FTP_TRACE( "Sending _start_pos=%llu  FTP_DEFAULT_RANGE_LEN=%llu, down_range._index=%d, size=%llu", _start_pos,  FTP_DEFAULT_RANGE_LEN, down_range._index, _p_ftp_pipe->_p_ftp_server_resource->_file_size);

                    _remain_len =(down_range._index+down_range._num)*FTP_DEFAULT_RANGE_LEN;

                    if(_p_ftp_pipe->_p_ftp_server_resource->_file_size==0)
                    {
                        _p_ftp_pipe->_p_ftp_server_resource->_file_size = pi_get_file_size( (DATA_PIPE *)_p_ftp_pipe);
                        if(_p_ftp_pipe->_p_ftp_server_resource->_file_size==0)
                            _p_ftp_pipe->_p_ftp_server_resource->_file_size = -1;

                    }

                    if(_p_ftp_pipe->_p_ftp_server_resource->_file_size<_start_pos)
                        return FTP_ERR_RANGE_OUT_OF_SIZE;
                    else
                    {
                        if(_p_ftp_pipe->_p_ftp_server_resource->_file_size<_remain_len)
                            _p_ftp_pipe->_data._content_length = _p_ftp_pipe->_p_ftp_server_resource->_file_size-_start_pos;
                        else
                            _p_ftp_pipe->_data._content_length = (down_range._num)*FTP_DEFAULT_RANGE_LEN;
                    }
                    _p_ftp_pipe->_data._recv_end_pos    =0;
////////////////////////////////////////////////////////////////
                    /*
                    //if(_p_ftp_pipe->_data._current_recving_range._num!=FTP_DEFAULT_RANGE_NUM)
                    {
                        _int32 result_t = 0,size_t = 0;
                        size_t=range_list_size(&(_p_ftp_pipe->_data_pipe._dispatch_info._uncomplete_ranges));

                        FTP_TRACE( "******* TESTING:range_list_size=%d" ,size_t);

                        if(size_t !=0)
                        {
                            RANGE_LIST_ITEROATOR  Range_node = NULL;
                            result_t = range_list_get_head_node(&(_p_ftp_pipe->_data_pipe._dispatch_info._uncomplete_ranges), &Range_node);
                        FTP_TRACE( "******* *********************************************************");

                            FTP_TRACE( " ******* TESTING:result_t=%d,_uncomplete_ranges._index=%u,_uncomplete_ranges._num=%u)" ,result_t,Range_node->_range._index,Range_node->_range._num);
                        }
                            FTP_TRACE( " ******* TESTING:_data._content_length=%llu,_data._recv_end_pos=%llu,_data._data_length=%u)" ,_p_ftp_pipe->_data._content_length,_p_ftp_pipe->_data._recv_end_pos,_p_ftp_pipe->_data._data_length);

                            FTP_TRACE( " ******* TESTING:_down_range._index=%u,_down_range._num=%u,_current_recving_range._index=%u,_current_recving_range._num=%u)" ,
                                _p_ftp_pipe->_data_pipe._dispatch_info._down_range._index,_p_ftp_pipe->_data_pipe._dispatch_info._down_range._num,_p_ftp_pipe->_data._current_recving_range._index ,_p_ftp_pipe->_data._current_recving_range._num );
                        FTP_TRACE( "******* *********************************************************");

                    }
                    */
////////////////////////////////////////////////////////////////
                    FTP_TRACE( "FTP_ERR_COMMAND_NOT_NEEDED: _start_pos ==%llu", _start_pos );
                    sd_snprintf(command,256,"REST %llu",_start_pos);

                    FTP_TRACE( "FTP_ERR_COMMAND_NOT_NEEDED command= %s", command);
                    _p_ftp_pipe->_b_ranges_changed =FALSE;
                    /* if the start point is 0,this command is not needed! */
                    if(_start_pos == 0)
                    {
                        FTP_TRACE( "FTP_ERR_COMMAND_NOT_NEEDED: _start_pos == 0" );
                        return FTP_ERR_COMMAND_NOT_NEEDED;
                    }
                }
            }
            else
            {
                return FTP_ERR_INVALID_RANGES  ;
            }

            break;
        }
        case FTP_COMMAND_RETR:
        {
            /* send the download file .... */
            sd_strcat(command,"RETR ",sd_strlen("RETR "));
            if(_p_ftp_pipe->_p_ftp_server_resource->_url._file_name != NULL)
            {
                if(_p_ftp_pipe->_b_retry_request == TRUE)
                {
                    _p_ftp_pipe->_b_retry_request =FALSE;
                    /* Convert the path format */
                    ret_val = url_convert_format( _p_ftp_pipe->_p_ftp_server_resource->_url._file_name,_p_ftp_pipe->_p_ftp_server_resource->_url._file_name_len,tmp_buffer ,MAX_URL_LEN ,&_p_ftp_pipe->_request_step);
                    if(ret_val!=SUCCESS) return ret_val;
                    sd_strcat(command,tmp_buffer,sd_strlen(tmp_buffer));
                }
                else
                {
                    if((_p_ftp_pipe->_p_ftp_server_resource->_url._filename_valid_step!=UCS_ORIGIN)&&(_p_ftp_pipe->_p_ftp_server_resource->_url._filename_valid_step!=UCS_END))
                    {
                        /* Convert the path format */
                        ret_val = url_convert_format_to_step( _p_ftp_pipe->_p_ftp_server_resource->_url._file_name,_p_ftp_pipe->_p_ftp_server_resource->_url._file_name_len,tmp_buffer ,MAX_URL_LEN ,_p_ftp_pipe->_p_ftp_server_resource->_url._filename_valid_step);
                        CHECK_VALUE(ret_val);
                        sd_strcat(command,tmp_buffer,sd_strlen(tmp_buffer));
                    }
                    else
                    {
                        sd_strcat(command,_p_ftp_pipe->_p_ftp_server_resource->_url._file_name,_p_ftp_pipe->_p_ftp_server_resource->_url._file_name_len);
                    }
                }
            }
            else
            {
                return FTP_ERR_INVALID_FILE_NAME;
            }
            break;
        }

        case FTP_COMMAND_ABOR:
        {
            /* send the abort the current task */
            sd_strcat(command,"ABOR",sd_strlen("ABOR"));
            break;
        }
        case FTP_COMMAND_QUIT:
        {
            /* send the quit from the server  */
            sd_strcat(command,"QUIT",sd_strlen("QUIT"));
            break;
        }
        default:
        {
            //SAFE_DELETE(command);
            return FTP_ERR_INVALID_COMMAND;
        }
    }

    sd_strcat(command, "\r\n",sd_strlen("\r\n"));

    length = sd_strlen(command);

    if(length+1>_p_ftp_pipe->_ftp_request_buffer_len)
    {
        if(_p_ftp_pipe->_ftp_request_buffer_len==0)
        {
            if(length+1>MAX_FTP_REQ_HEADER_LEN)
                _p_ftp_pipe->_ftp_request_buffer_len = length+1;
            else
                _p_ftp_pipe->_ftp_request_buffer_len = MAX_FTP_REQ_HEADER_LEN;
        }
        else
        {
            SAFE_DELETE(_p_ftp_pipe->_ftp_request);
            _p_ftp_pipe->_ftp_request_buffer_len = length+1;
        }

        _p_ftp_pipe->_error_code = sd_malloc(_p_ftp_pipe->_ftp_request_buffer_len, (void**)&(_p_ftp_pipe->_ftp_request));
        if(_p_ftp_pipe->_error_code) return _p_ftp_pipe->_error_code;

    }


    sd_memset(_p_ftp_pipe->_ftp_request,0,_p_ftp_pipe->_ftp_request_buffer_len);
    sd_memcpy(_p_ftp_pipe->_ftp_request,command,length);

    FTP_TRACE( "Sending[%d]:%s",length,_p_ftp_pipe->_ftp_request );

    _p_ftp_pipe->_cur_command_id = _command_id;
    _p_ftp_pipe->_ftp_state = FTP_PIPE_STATE_REQUESTING;
    _p_ftp_pipe->_error_code = SOCKET_PROXY_SEND(_p_ftp_pipe->_socket_fd, _p_ftp_pipe->_ftp_request, length, ftp_pipe_handle_send,(void*)_p_ftp_pipe);

    if(_p_ftp_pipe->_error_code)
    {
        return _p_ftp_pipe->_error_code;
    }

    sd_time_ms(&(_p_ftp_pipe->_data_pipe._dispatch_info._last_recv_data_time));


    return SUCCESS;

} /* End of ftp_pipe_send_command */


_int32 ftp_pipe_close_connection(FTP_DATA_PIPE * _p_ftp_pipe)
{
    _u32 _op_count = 0;
    FTP_TRACE( "FTP:enter ftp_pipe_close_connection()..." );

    if (!_p_ftp_pipe) return FTP_ERR_INVALID_PIPE;

    FTP_TRACE( "_is_connected=%d,_ftp_state=%d",_p_ftp_pipe->_is_connected,_p_ftp_pipe->_ftp_state );

    if((_p_ftp_pipe->_is_connected == TRUE)||
       (_p_ftp_pipe->_ftp_state == FTP_PIPE_STATE_CONNECTING)||(_p_ftp_pipe->_ftp_state == FTP_PIPE_STATE_CLOSING))
    {
        _p_ftp_pipe->_error_code = SOCKET_PROXY_GET_OP_COUNT(_p_ftp_pipe->_socket_fd, DEVICE_SOCKET_TCP,&_op_count);
        if(_p_ftp_pipe->_error_code)  goto ErrorHanle;

        /* Close the FTP command connection */
        if(_op_count!=0)
        {
            _p_ftp_pipe->_ftp_state = FTP_PIPE_STATE_CLOSING;
            FTP_TRACE( "call SOCKET_PROXY_CANCEL(fd=%u)",_p_ftp_pipe->_socket_fd );
            _p_ftp_pipe->_error_code = SOCKET_PROXY_CANCEL(_p_ftp_pipe->_socket_fd,DEVICE_SOCKET_TCP);
            if(_p_ftp_pipe->_error_code)  goto ErrorHanle;

            return SUCCESS;

        }
        else
        {

            _p_ftp_pipe->_ftp_state = FTP_PIPE_STATE_CLOSING;
            FTP_TRACE( "call SOCKET_PROXY_CLOSE(fd=%u)",_p_ftp_pipe->_socket_fd );
            _p_ftp_pipe->_error_code = SOCKET_PROXY_CLOSE(_p_ftp_pipe->_socket_fd);
            if(_p_ftp_pipe->_error_code)
                /* closing socket error ! */
                goto ErrorHanle;
        }
    }

    FTP_TRACE( "Colseing OK:range_list_size=%d,_pipe_state=%d,_wait_delete=%d",dp_get_uncomplete_ranges_list_size(&(_p_ftp_pipe->_data_pipe)),_p_ftp_pipe->_data_pipe._dispatch_info._pipe_state,_p_ftp_pipe->_wait_delete );

    _p_ftp_pipe->_is_connected = FALSE;
    _p_ftp_pipe->_data_pipe._dispatch_info._time_stamp = 0;   ///????
    //_p_ftp_pipe->_sum_recv_bytes = 0;
    _p_ftp_pipe->_socket_fd = 0;
    _p_ftp_pipe->_ftp_state = FTP_PIPE_STATE_IDLE;

    if((dp_get_uncomplete_ranges_list_size(&(_p_ftp_pipe->_data_pipe))!=0)
       &&(_p_ftp_pipe->_data_pipe._dispatch_info._pipe_state != PIPE_FAILURE)&&(_p_ftp_pipe->_wait_delete!=TRUE))
    {
        return ftp_pipe_open((DATA_PIPE*) _p_ftp_pipe);
    }
    else if(_p_ftp_pipe->_wait_delete)
        ftp_pipe_destroy(_p_ftp_pipe);  ////??????



    return SUCCESS;
ErrorHanle:
    FTP_TRACE( "ErrorHanle:_wait_delete=%d",_p_ftp_pipe->_wait_delete );
    if(_p_ftp_pipe->_wait_delete)
    {
        ftp_pipe_destroy(_p_ftp_pipe);  ////??????
        return SUCCESS;
    }
    else
    {
        ftp_pipe_failure_exit( _p_ftp_pipe);
        return _p_ftp_pipe->_error_code;
    }



}

_int32 ftp_pipe_open_pasv(FTP_DATA_PIPE * _p_ftp_pipe)
{
    FTP_TRACE( "FTP:enter ftp_pipe_open_pasv()..." );

    if (!_p_ftp_pipe) return FTP_ERR_INVALID_PIPE;

    /* Connect to passive mode port... */
    /* Check if the FTP data connection is closed */
    if(_p_ftp_pipe->_pasv._is_connected == TRUE)
    {

        return FTP_ERR_ENTER_PASV_MODE;
    }

    _p_ftp_pipe->_error_code = SOCKET_PROXY_CREATE(&(_p_ftp_pipe->_pasv._socket_fd),SD_SOCK_STREAM);
    if((_p_ftp_pipe->_error_code)||(_p_ftp_pipe->_pasv._socket_fd == 0))
    {
        /* Create socket error ! */
        return _p_ftp_pipe->_error_code;
    }

    LOG_DEBUG("ftp_pipe_open_pasv  _p_ftp_pipe->_pasv._socket_fd = %u", _p_ftp_pipe->_pasv._socket_fd);

    _p_ftp_pipe->_pasv._state = FTP_PIPE_STATE_CONNECTING;
    _p_ftp_pipe->_pasv._try_count = 0;
    FTP_TRACE( "call SOCKET_PROXY_CONNECT(_pasv.fd=%u,_pasv._host=%s,_pasv.port=%u)" ,_p_ftp_pipe->_pasv._socket_fd,_p_ftp_pipe->_pasv._PASV_ip,_p_ftp_pipe->_pasv._PASV_port);
    return SOCKET_PROXY_CONNECT(_p_ftp_pipe->_pasv._socket_fd,_p_ftp_pipe->_pasv._PASV_ip,
                                _p_ftp_pipe->_pasv._PASV_port,ftp_pipe_handle_pasv_connect,(void*)_p_ftp_pipe);

}
_int32 ftp_pipe_close_pasv(FTP_DATA_PIPE * _p_ftp_pipe)
{
    _u32 _op_count = 0;
    FTP_TRACE( "FTP:enter ftp_pipe_close_pasv()..." );

    if (!_p_ftp_pipe) return FTP_ERR_INVALID_PIPE;

    FTP_TRACE( "_pasv._is_connected=%d,_pasv._state=%d,_wait_delete=%d",_p_ftp_pipe->_pasv._is_connected,_p_ftp_pipe->_pasv._state,_p_ftp_pipe->_wait_delete );

    if((_p_ftp_pipe->_pasv._is_connected == TRUE)||(_p_ftp_pipe->_pasv._state == FTP_PIPE_STATE_CLOSING)||(_p_ftp_pipe->_pasv._state == FTP_PIPE_STATE_CONNECTING))
    {
        _p_ftp_pipe->_error_code = SOCKET_PROXY_GET_OP_COUNT(_p_ftp_pipe->_pasv._socket_fd, DEVICE_SOCKET_TCP,&_op_count);
        if(_p_ftp_pipe->_error_code)  return _p_ftp_pipe->_error_code;

        if(_op_count!=0)
        {
            _p_ftp_pipe->_pasv._state = FTP_PIPE_STATE_CLOSING;
            FTP_TRACE( "call SOCKET_PROXY_CANCEL(_pasv.fd=%u)",_p_ftp_pipe->_pasv._socket_fd );
            _p_ftp_pipe->_error_code = SOCKET_PROXY_CANCEL(_p_ftp_pipe->_pasv._socket_fd,DEVICE_SOCKET_TCP);
            if(_p_ftp_pipe->_error_code)  return _p_ftp_pipe->_error_code;

            return SUCCESS;

        }
        else
        {

            _p_ftp_pipe->_pasv._state = FTP_PIPE_STATE_CLOSING;
            FTP_TRACE( "call SOCKET_PROXY_CLOSE(_pasv.fd=%u)",_p_ftp_pipe->_pasv._socket_fd );
            _p_ftp_pipe->_error_code = SOCKET_PROXY_CLOSE(_p_ftp_pipe->_pasv._socket_fd);
            if(_p_ftp_pipe->_error_code)
                /* closing socket error ! */
                return _p_ftp_pipe->_error_code;

            _p_ftp_pipe->_pasv._is_connected = FALSE;
            _p_ftp_pipe->_pasv._socket_fd = 0;
            _p_ftp_pipe->_pasv._state = FTP_PIPE_STATE_IDLE;

            if(_p_ftp_pipe->_wait_delete)
            {
                ftp_pipe_close((DATA_PIPE *) _p_ftp_pipe);
            }

        }
    }

    return SUCCESS;
}


_int32 ftp_pipe_get_line( const char* ptr, _u32 str_len, _u32 * line_len )
{
    _u32 pos=0;

    //FTP_TRACE( "ftp_pipe_get_line[%d]: %s",str_len ,ptr);

    while( pos < str_len )
    {
        if( ( *(ptr+pos) == '\r' )
            && ( (pos+1) < str_len )
            && ( *(ptr+pos+1) == '\n' )
          )
        {
            *line_len = pos;
            return SUCCESS;
        }
        else
            ++pos;
    }
    return -1;
}
_int32 ftp_pipe_parse_line( const char* line_ptr, _u32 line_len, _int32 * code, char * text_line )
{
    _int32 i,result=0;
    char  first_3_ch[4];

    //FTP_TRACE( "ftp_pipe_parse_line");

    sd_memset(first_3_ch,0,4);

    if( line_len < 3 )
        return -1;
    else if( line_len == 3 )
    {
        sd_strncpy(first_3_ch,line_ptr,3);
        text_line = "";
    }
    else if( line_ptr[3] == ' ' )
    {
        sd_strncpy(first_3_ch,line_ptr,3);
        sd_strncpy(text_line,line_ptr+4,line_len-4);
    }
    else
    {
        return -1;
    }

    for( i=0; i<sd_strlen(first_3_ch); i++)
    {
        if((first_3_ch[i]<'0')||(first_3_ch[i]>'9'))
        {
            return -1;
        }
        else
        {
            result   *=   10;
            result   +=   first_3_ch[i]   -   '0';

        }
    }
    *code = result;
    return SUCCESS;
}

_int32 ftp_pipe_parse_response(FTP_DATA_PIPE * _p_ftp_pipe)
{
    char response_line[FTP_RESPN_DEFAULT_LEN],text_line[FTP_RESPN_DEFAULT_LEN];
    char hostBuffer[MAX_HOST_NAME_LEN],p1_buffer[MAX_HOST_NAME_LEN],p2_buffer[MAX_HOST_NAME_LEN];
    char PathBuffer1[MAX_FULL_PATH_BUFFER_LEN]= {0},PathBuffer2[MAX_FULL_PATH_BUFFER_LEN]= {0};
    char* cur_ptr = NULL,* pos = NULL,*  pos1 = NULL,*pos2=NULL,*pos3=NULL,*pos4=NULL,*colon_pos=NULL;
    char * respn_buffer = _p_ftp_pipe->_response._buffer;
    _u32 respn_len = _p_ftp_pipe->_response._buffer_end_pos;
    _u32 dispose_data_len = 0,line_len=0;
    _int32 code = 0,i,p1=0,p2=0,ret_val=SUCCESS;
    _u64 result=0,_file_size_from_data_manager=0;
    RANGE all_range,     down_range,uncom_head_range;

    FTP_TRACE( " enter ftp_pipe_parse_response()..." );

    //respn_buffer[respn_len] = '\0';
    //FTP_TRACE("********************************************" );
    //FTP_TRACE( respn_buffer );
    //FTP_TRACE("********************************************" );

    while( TRUE )
    {

        if( dispose_data_len < respn_len )
        {
            cur_ptr= respn_buffer+dispose_data_len;
            if( ftp_pipe_get_line( cur_ptr, respn_len-dispose_data_len,&line_len ) == SUCCESS )
            {
                //std::string response_line(cur_ptr, line_len);
                sd_memset(response_line,0,FTP_RESPN_DEFAULT_LEN);
                sd_memcpy(response_line,cur_ptr,line_len);

                FTP_TRACE("Get a line:%s",response_line);

                dispose_data_len += (line_len + 2) ;

                sd_memset(text_line,0,FTP_RESPN_DEFAULT_LEN);

                if( ftp_pipe_parse_line( cur_ptr, line_len,&code, text_line ) == SUCCESS )
                {
                    FTP_TRACE("Response line is found:code=%d,text=%s",code,text_line);

                    if((code != 451)&&(code != 220))
                    {
                        if((code == 226)&&(_p_ftp_pipe->_cur_command_id == FTP_COMMAND_PASV)&&
                           (NULL!=sd_strstr(_p_ftp_pipe->_response._buffer+dispose_data_len,"227", 0)))
                        {
                            FTP_TRACE("Need continue to parse: %s",_p_ftp_pipe->_response._buffer+dispose_data_len);
                            //CHECK_VALUE(777);
                        }
                        else
                        {
                            _p_ftp_pipe->_response._code =  code;
                            if(text_line[0] != '\0')
                            {

                                if( sd_malloc(sd_strlen(text_line)+1, (void **)& (_p_ftp_pipe->_response._description)))
                                    return OUT_OF_MEMORY;

                                sd_memset(_p_ftp_pipe->_response._description,0,sd_strlen(text_line)+1);
                                sd_memcpy(_p_ftp_pipe->_response._description, text_line, sd_strlen(text_line));

                            }
                            break;
                        }
                    }
                    else if(code == 220)
                    {
                        FTP_TRACE("Got 220:Welcome msg ");
                        /* Discard this string...*/
                        sd_memmove(_p_ftp_pipe->_response._buffer,_p_ftp_pipe->_response._buffer+dispose_data_len,respn_len-dispose_data_len);
                        _p_ftp_pipe->_response._buffer[respn_len-dispose_data_len]='\0';
                        respn_buffer = _p_ftp_pipe->_response._buffer;
                        _p_ftp_pipe->_response._buffer_end_pos=respn_len-dispose_data_len;
                        respn_len = _p_ftp_pipe->_response._buffer_end_pos;
                        dispose_data_len=0;
                    }
                    else if(code == 451)
                    {
                        FTP_TRACE("Got 451");
                    }
                }
                //LOG4CPLUS_THIS_INFO( _logger, "not recved last line, continue" );
            }
            else
            {
                FTP_TRACE( "not recv one complete line" );
                return FTP_ERR_RESPONSE_CODE_NOT_FOUNDE;
            }
        }
        else
            return FTP_ERR_RESPONSE_CODE_NOT_FOUNDE;
    }

    switch(code)
    {
        case 220:
        {
            /* Welcome msg ok! now send the user name  */
            FTP_TRACE( "220:Welcome msg ok! now send the user name " );

            //_p_ftp_pipe->_ftp_state = FTP_PIPE_STATE_CONNECTED;
            //_p_ftp_pipe->_data_pipe._p_resource->_cur_connections++;
            return ftp_pipe_send_command(_p_ftp_pipe,FTP_COMMAND_USER);

            break;
        }
        case 331: //User name okay, need password.
            FTP_TRACE( "331:User name okay, need password " );
            return ftp_pipe_send_command(_p_ftp_pipe,FTP_COMMAND_PASWD);

            break;
        case 230: //User logged in, proceed.
            /* Get the Extension supported of the FTP server */
            FTP_TRACE( "230:User logged in, proceed. Get the Extension supported of the FTP server " );

            if(_p_ftp_pipe->_p_ftp_server_resource->_b_is_origin==TRUE)
            {
                FTP_TRACE( "+++ The origin resource:%s [%s] is connected!!",_p_ftp_pipe->_p_ftp_server_resource->_url._host,_p_ftp_pipe->_p_ftp_server_resource->_url._full_path);
            }
            else
            {
                FTP_TRACE( "+++ The inquire resource:%s [%s] is connected!!",_p_ftp_pipe->_p_ftp_server_resource->_url._host,_p_ftp_pipe->_p_ftp_server_resource->_url._full_path);
            }

            return ftp_pipe_send_command(_p_ftp_pipe,FTP_COMMAND_FEAT);
            //return ftp_pipe_send_command(_p_ftp_pipe,FTP_COMMAND_CWD);

            break;
        case 211: //Extension supported
        {
            FTP_TRACE( "211:Extension supported " );
            /* Connecting success ! */
            //_p_ftp_pipe->_data_pipe._dispatch_info._pipe_state = PIPE_DOWNLOADING;
            dp_set_state(&_p_ftp_pipe->_data_pipe, PIPE_DOWNLOADING);
            _p_ftp_pipe->_ftp_state = FTP_PIPE_STATE_CONNECTED;
            pos = sd_strstr(_p_ftp_pipe->_response._buffer,"REST", 0);
            if(pos!=NULL)
            {
                _p_ftp_pipe->_data_pipe._dispatch_info._is_support_range = TRUE;
                _p_ftp_pipe->_p_ftp_server_resource->_is_support_range=TRUE;
            }

            _p_ftp_pipe->_data_pipe._dispatch_info._is_support_long_connect = TRUE; /* ???? */

            /* change to the work drectory */
            if(_p_ftp_pipe->_b_ranges_changed == TRUE)
            {
                return ftp_pipe_send_command(_p_ftp_pipe,FTP_COMMAND_CWD);
            }
            else
            {
                LOG_DEBUG( "_p_ftp_pipe=0x%X:dm_notify_dispatch_data_finish ",_p_ftp_pipe);
                return pi_notify_dispatch_data_finish((DATA_PIPE *)_p_ftp_pipe);
            }

            break;
        }
        case 250: //Directory changed to...
            FTP_TRACE( "250:Directory changed to... Now change to tpye I" );
            if((_p_ftp_pipe->_p_ftp_server_resource->_url._path_valid_step==UCS_ORIGIN)&&
               (_p_ftp_pipe->_request_step!=UCS_ORIGIN)&&(_p_ftp_pipe->_request_step!=UCS_END))
            {
                _p_ftp_pipe->_p_ftp_server_resource->_url._path_valid_step=_p_ftp_pipe->_request_step;
            }
            _p_ftp_pipe->_request_step=UCS_ORIGIN;
            /* change to tpye I */
            return ftp_pipe_send_command(_p_ftp_pipe,FTP_COMMAND_TYPE_I);

            break;
        case 200: //Type set to I
            FTP_TRACE( "200:Type set to I OK... Now Get the last modify time of the file " );
            /* Get the last modify time of the file */
            return ftp_pipe_send_command(_p_ftp_pipe,FTP_COMMAND_MDTM);

            break;
        case 213:
            FTP_TRACE( "213:Last modify time ... or  size of file ..." );
            if(_p_ftp_pipe->_response._description != NULL)
            {
                sd_trim_prefix_lws(_p_ftp_pipe->_response._description);
                sd_trim_postfix_lws( _p_ftp_pipe->_response._description );

                if(_p_ftp_pipe->_cur_command_id == FTP_COMMAND_MDTM)
                {
                    // Last modify time ...
                    FTP_TRACE( "Last modify time ... " );

                    //sd_memset(_p_ftp_pipe->_p_ftp_server_resource->_lastmodified_from_server,0,MAX_LAST_MODIFIED_LEN);
                    //sd_strncpy(_p_ftp_pipe->_p_ftp_server_resource->_lastmodified_from_server, _p_ftp_pipe->_response._description,MAX_LAST_MODIFIED_LEN-1);

                    /* Get the file size */
                    return ftp_pipe_send_command(_p_ftp_pipe,FTP_COMMAND_SIZE);

                }
                else if(_p_ftp_pipe->_cur_command_id == FTP_COMMAND_SIZE)
                {
                    // size of file ...
                    FTP_TRACE( "size of file ..." );
                    result=0;
                    for( i=0; i<sd_strlen(_p_ftp_pipe->_response._description); i++)
                    {
                        if((_p_ftp_pipe->_response._description[i]<'0')||(_p_ftp_pipe->_response._description[i]>'9'))
                        {
                            return FTP_ERR_INVALID_FILE_SIZE;
                        }
                        else
                        {
                            result   *=   10;
                            result   +=   _p_ftp_pipe->_response._description[i]   -   '0';

                        }
                    }
                    /* Geting file size success ! */
                    if(_p_ftp_pipe->_p_ftp_server_resource->_file_size != result )
                    {
                        _file_size_from_data_manager = pi_get_file_size( (DATA_PIPE *)_p_ftp_pipe);
                        _p_ftp_pipe->_p_ftp_server_resource->_file_size = result;
                        _p_ftp_pipe->_p_ftp_server_resource->_b_file_exist = TRUE;

                        FTP_TRACE("+++ Get file size=%llu from resource:%s,full path=%s,is_origin=%d",result,_p_ftp_pipe->_p_ftp_server_resource->_url._host,_p_ftp_pipe->_p_ftp_server_resource->_url._full_path,_p_ftp_pipe->_p_ftp_server_resource->_b_is_origin);
                        if(0 == result 
                            && _p_ftp_pipe->_p_ftp_server_resource->_b_is_origin)
                        {
                            pi_set_file_size(&(_p_ftp_pipe->_data_pipe), 0);
                            return FTP_ERR_INVALID_SERVER_FILE_SIZE;
                        }

                        if((_p_ftp_pipe->_p_ftp_server_resource->_b_is_origin==TRUE) && (_file_size_from_data_manager!=result))
                        {
                            if(_p_ftp_pipe->_b_retry_set_file_size != TRUE)
                            {
                                ret_val=pi_set_file_size( (DATA_PIPE *)_p_ftp_pipe, result);
                                if(ret_val!=SUCCESS)
                                {
                                    if(ret_val==FILE_SIZE_NOT_BELIEVE)
                                    {
                                        /* Start timer to wait dm_set_file_size */
                                        _p_ftp_pipe->_b_retry_set_file_size = TRUE;
                                        /* Start timer */
                                        FTP_TRACE("call START_TIMER to wait dm_set_file_size");
                                        return START_TIMER(ftp_pipe_handle_timeout, NOTICE_ONCE,SET_FILE_SIZE_WAIT_M_SEC,0,  _p_ftp_pipe,&(_p_ftp_pipe->_retry_set_file_size_timer_id));
                                    }
                                    else
                                    {
                                        FTP_TRACE( "FTP_ERR_INVALID_SERVER_FILE_SIZE" );
                                        return FTP_ERR_INVALID_SERVER_FILE_SIZE;
                                    }
                                }
                            }
                            else
                            {
                                return SUCCESS;
                            }
                        }
                        else if(_file_size_from_data_manager!=result)
                        {
                            //if(_file_size_from_data_manager==0)
                            //{
                            //dm_set_file_size( _p_ftp_pipe->_data_pipe._p_data_manager,result);
                            FTP_TRACE( "FTP_ERR_INVALID_SERVER_FILE_SIZE" );
                            //}else
                            return FTP_ERR_INVALID_SERVER_FILE_SIZE;

                        }
                    }

                    /* Set the _data_pipe._dispatch_info._can_download_ranges ! */
                    {
                        all_range = pos_length_to_range(0,result, result);
                        //_p_ftp_pipe->_error_code =pi_pos_len_to_valid_range( (DATA_PIPE *) _p_ftp_pipe, 0, result, &all_range );
                        //if(_p_ftp_pipe->_error_code) return _p_ftp_pipe->_error_code;

                        _p_ftp_pipe->_error_code =dp_add_can_download_ranges( &(_p_ftp_pipe->_data_pipe), &all_range );
                        if(_p_ftp_pipe->_error_code) return _p_ftp_pipe->_error_code;


                        _p_ftp_pipe->_error_code = dp_get_download_range( (DATA_PIPE *)_p_ftp_pipe, &down_range );
                        if(_p_ftp_pipe->_error_code!=SUCCESS) return _p_ftp_pipe->_error_code;

                        if((down_range._index == 0 )&&(down_range._num == MAX_NUM_RANGES))
                        {

                            if(all_range._num>=_p_ftp_pipe->_reveive_range_num)
                                down_range._num=_p_ftp_pipe->_reveive_range_num;
                            else
                                down_range._num=all_range._num;

                            _p_ftp_pipe->_error_code = dp_set_download_range( (DATA_PIPE *)_p_ftp_pipe, &down_range );
                            if(_p_ftp_pipe->_error_code!=SUCCESS) return _p_ftp_pipe->_error_code;
                        }

                    }

                    /* request for passive mode */
                    return ftp_pipe_send_command(_p_ftp_pipe,FTP_COMMAND_PASV);
                }



            }
            return FTP_ERR_INVALID_RESPONSE;

            break;
        case 227: // Entering Passive Mode (xxx,xxx,xxx,xxx,XXX,XXX):
            FTP_TRACE( "227:Entering Passive Mode (xxx,xxx,xxx,xxx,XXX,XXX)" );
            if(_p_ftp_pipe->_response._description != NULL)
            {
                pos1 = NULL;
                pos2=NULL;
                pos3=NULL;
                pos4=NULL;
                pos1 = sd_strchr(_p_ftp_pipe->_response._description,'(', 0);
                if( pos1!=NULL)
                {
                    pos2 = sd_strchr(_p_ftp_pipe->_response._description,')', 0);
                    if( pos2!=NULL)
                    {
                        pos3 = sd_strchr(pos1,',', 0);
                        for(i=0; (i<4)&&( pos3!=NULL); i++)
                        {
                            pos3++;
                            pos4 = pos3;
                            pos3 = sd_strchr(pos4,',', 0);
                        }


                        //for(i=0;(i<4)&&( sd_strchr(_p_ftp_pipe->_response._description+pos1+pos4,',', 0, &pos3)==SUCCESS);i++) pos4+=pos3+1;

                        if((i == 4)&&(pos1!=NULL)&&(pos2!=NULL)&&(pos3!=NULL)&&(pos4!=NULL)&&(pos1<pos3)&&(pos3<pos2))
                        {
                            sd_memset(hostBuffer,0,MAX_HOST_NAME_LEN);
                            /* Get the server ip address */
                            if(pos4-pos1-2>MAX_HOST_NAME_LEN-1)
                                return FTP_ERR_INVALID_PASV_MODE_HOST;

                            sd_memcpy(hostBuffer,pos1+1,pos4-pos1-2);  ///????

                            pos3 = sd_strchr(hostBuffer,',', 0);
                            for(i=0; (i<3)&&( pos3!=NULL); i++)
                            {
                                pos3[0]='.';
                                pos3 = sd_strchr(pos3,',', 0);
                            }


                            if(sd_stricmp(hostBuffer,_p_ftp_pipe->_p_ftp_server_resource->_url._host) != 0)
                            {
                                /* the passive mode ip address is different from the host ip address */
                                /* To be completed.... */
                                FTP_TRACE(" The passive mode ip address is different from the host ip address ");

                            }

                            {
                                p1=0;
                                p2=0;
                                sd_strncpy(_p_ftp_pipe->_pasv._PASV_ip,hostBuffer,MAX_HOST_NAME_LEN-1);

                                sd_memset(p1_buffer,0,MAX_HOST_NAME_LEN);
                                sd_memset(p2_buffer,0,MAX_HOST_NAME_LEN);
                                /* Get the server pasv port */ // p1*256+p2
                                sd_memset(hostBuffer,0,MAX_HOST_NAME_LEN);
                                if(pos2-pos4>MAX_HOST_NAME_LEN-1)
                                    return FTP_ERR_INVALID_PASV_MODE_HOST;
                                sd_memcpy(hostBuffer,pos4,pos2-pos4);
                                pos1 = sd_strchr(hostBuffer,',', 0);
                                if( pos1!=NULL)
                                {
                                    sd_memcpy(p1_buffer,hostBuffer,pos1-hostBuffer);
                                    sd_strncpy(p2_buffer,pos1+1,MAX_HOST_NAME_LEN-1);
                                    /* Get the high part of the port */
                                    for( i=0; i<sd_strlen(p1_buffer); i++)
                                    {
                                        if((p1_buffer[i]<'0')||(p1_buffer[i]>'9'))
                                        {
                                            return FTP_ERR_ENTER_PASV_MODE;
                                        }
                                        else
                                        {
                                            p1   *=   10;
                                            p1   +=   p1_buffer[i]   -   '0';

                                        }
                                    }
                                    /* Get the low part of the port */
                                    for( i=0; i<sd_strlen(p2_buffer); i++)
                                    {
                                        if((p2_buffer[i]<'0')||(p2_buffer[i]>'9'))
                                        {
                                            return FTP_ERR_ENTER_PASV_MODE;
                                        }
                                        else
                                        {
                                            p2   *=   10;
                                            p2   +=   p2_buffer[i]   -   '0';

                                        }
                                    }

                                    _p_ftp_pipe->_pasv._PASV_port = p1*256+p2;
                                    /* Jump to the start downloading position of the file */
                                    ret_val = ftp_pipe_send_command(_p_ftp_pipe,FTP_COMMAND_REST);
                                    if(ret_val==FTP_ERR_COMMAND_NOT_NEEDED)
                                    {
                                        /* Connect to passive mode port... */
                                        _p_ftp_pipe->_error_code = ftp_pipe_open_pasv(_p_ftp_pipe);
                                        if(_p_ftp_pipe->_error_code)
                                        {
                                            /* Open socket error ! */
                                            return _p_ftp_pipe->_error_code;
                                        }
                                        //    /* Start download the file data  */
                                        _p_ftp_pipe->_ftp_state = FTP_PIPE_STATE_DOWNLOADING;
                                        return SUCCESS;
                                    }
                                    else
                                    {
                                        return ret_val;
                                    }

                                }
                            }

                        }
                    }
                }
            }

            return FTP_ERR_ENTER_PASV_MODE;


            break;
        case 350: // Restarting at XXXX. Send STORE or RETRIEVE
            FTP_TRACE( "350:Restarting at XXXX. Send STORE or RETRIEVE,Now Connect to passive mode port... " );

            if(_p_ftp_pipe->_data_pipe._dispatch_info._is_support_range != TRUE)
            {
                _p_ftp_pipe->_data_pipe._dispatch_info._is_support_range = TRUE;
                _p_ftp_pipe->_p_ftp_server_resource->_is_support_range=TRUE;
            }

            /* Connect to passive mode port... */
            _p_ftp_pipe->_error_code = ftp_pipe_open_pasv(_p_ftp_pipe);
            if(_p_ftp_pipe->_error_code)
            {
                /* Open socket error ! */
                return _p_ftp_pipe->_error_code;
            }
            //  /* Start download the file data  */
            //  return ftp_pipe_send_command(_p_ftp_pipe,FTP_COMMAND_RETR);
            _p_ftp_pipe->_ftp_state = FTP_PIPE_STATE_DOWNLOADING;

            break;
        case 125: // Data connection already open; Transfer starting.
        case 150: // Opening BINARY mode data connection for xxx (xxx Bytes).
            FTP_TRACE( "150:Opening BINARY mode data connection for xxx,filesize =%llu",_p_ftp_pipe->_p_ftp_server_resource->_file_size );

            /* if the SIZE command is not suppoeted by the server ,we must get the file size from the text of this response */
            if((_p_ftp_pipe->_p_ftp_server_resource->_file_size == 0)||(_p_ftp_pipe->_p_ftp_server_resource->_file_size == -1))
            {
                pos1 = NULL;
                pos2=NULL;
                pos3=NULL;
                result=0;


                pos1 = sd_strrchr(_p_ftp_pipe->_response._description,'(');
                pos2 = sd_strrchr(_p_ftp_pipe->_response._description,')');
                if(( pos1!=NULL)&&( pos2!=NULL)&&( pos1<pos2))
                {
                    pos3 = sd_strchr(pos1+1,' ',0);
                    if((pos3==NULL)||(pos3>pos2)||(pos3-pos1-1<1)||(pos3-pos1-1>20))
                    {
                        FTP_TRACE( "FTP_ERR_INVALID_SERVER_FILE_SIZE" );
                        return FTP_ERR_INVALID_SERVER_FILE_SIZE;
                    }

                    sd_memset(p1_buffer,0,MAX_HOST_NAME_LEN);
                    sd_memcpy(p1_buffer,pos1+1,pos3-pos1-1);
                    if(sd_str_to_u64( p1_buffer, pos3-pos1-1, &result )==SUCCESS)
                    {
                        _file_size_from_data_manager = pi_get_file_size( (DATA_PIPE *)_p_ftp_pipe);
                        _p_ftp_pipe->_p_ftp_server_resource->_file_size = result;
                        _p_ftp_pipe->_p_ftp_server_resource->_b_file_exist = TRUE;

                        FTP_TRACE("+++ Get file size=%llu from resource:%s,full path=%s,is_origin=%d",result,_p_ftp_pipe->_p_ftp_server_resource->_url._host,_p_ftp_pipe->_p_ftp_server_resource->_url._full_path,_p_ftp_pipe->_p_ftp_server_resource->_b_is_origin);
                        if(0 == result 
                            && _p_ftp_pipe->_p_ftp_server_resource->_b_is_origin)
                        {
                            pi_set_file_size(&(_p_ftp_pipe->_data_pipe), 0);
                            return FTP_ERR_INVALID_SERVER_FILE_SIZE;
                        }

                        if((_p_ftp_pipe->_p_ftp_server_resource->_b_is_origin==TRUE) && (_file_size_from_data_manager!=result))
                        {
                            if((_p_ftp_pipe->_b_retry_set_file_size == TRUE)||(pi_set_file_size( (DATA_PIPE *)_p_ftp_pipe,result)!=SUCCESS))
                            {
                                FTP_TRACE( "FTP_ERR_INVALID_SERVER_FILE_SIZE" );
                                return FTP_ERR_INVALID_SERVER_FILE_SIZE;
                            }
                        }
                        else if(_file_size_from_data_manager!=result)
                        {
                            FTP_TRACE( "FTP_ERR_INVALID_SERVER_FILE_SIZE" );
                            return FTP_ERR_INVALID_SERVER_FILE_SIZE;
                        }

                        /* Set the _data_pipe._dispatch_info._can_download_ranges ! */
                        {
                            all_range = pos_length_to_range(0,result, result);
                            //_p_ftp_pipe->_error_code =pi_pos_len_to_valid_range( (DATA_PIPE *) _p_ftp_pipe, 0, result, &all_range );
                            //if(_p_ftp_pipe->_error_code) return _p_ftp_pipe->_error_code;

                            _p_ftp_pipe->_error_code =dp_add_can_download_ranges( &(_p_ftp_pipe->_data_pipe), &all_range );
                            if(_p_ftp_pipe->_error_code) return _p_ftp_pipe->_error_code;

                            _p_ftp_pipe->_error_code = dp_get_download_range( (DATA_PIPE *)_p_ftp_pipe, &down_range );
                            if(_p_ftp_pipe->_error_code!=SUCCESS) return _p_ftp_pipe->_error_code;

                            if((down_range._index == 0 )&&(down_range._num == MAX_NUM_RANGES))
                            {

                                if(all_range._num>=_p_ftp_pipe->_reveive_range_num)
                                    down_range._num=_p_ftp_pipe->_reveive_range_num;
                                else
                                    down_range._num=all_range._num;

                                _p_ftp_pipe->_error_code = dp_set_download_range( (DATA_PIPE *)_p_ftp_pipe, &down_range );
                                if(_p_ftp_pipe->_error_code!=SUCCESS) return _p_ftp_pipe->_error_code;

                            }

                        }
                    }
                    else
                    {
                        FTP_TRACE( "FTP_ERR_INVALID_SERVER_FILE_SIZE" );
                        return FTP_ERR_INVALID_SERVER_FILE_SIZE;
                    }


                }
            }

            if(_p_ftp_pipe->_p_ftp_server_resource->_b_is_origin==TRUE)
            {
                FTP_TRACE( "+++ The origin resource:%s [%s] starts to transfer data!!",_p_ftp_pipe->_p_ftp_server_resource->_url._host,_p_ftp_pipe->_p_ftp_server_resource->_url._full_path);
            }
            else
            {
                FTP_TRACE( "+++ The inquire resource:%s [%s] starts to transfer data!!",_p_ftp_pipe->_p_ftp_server_resource->_url._host,_p_ftp_pipe->_p_ftp_server_resource->_url._full_path);
            }

            if(_p_ftp_pipe->_p_ftp_server_resource->_b_file_exist != TRUE)
                _p_ftp_pipe->_p_ftp_server_resource->_b_file_exist = TRUE;

            /* Downloading....FTP command connection do nothing */
            _p_ftp_pipe->_ftp_state = FTP_PIPE_STATE_DOWNLOADING;

            /* Start receiving file data */
            _p_ftp_pipe->_pasv._state = FTP_PIPE_STATE_READING;

            if((_p_ftp_pipe->_p_ftp_server_resource->_url._filename_valid_step==UCS_ORIGIN)&&
               (_p_ftp_pipe->_request_step!=UCS_ORIGIN)&&(_p_ftp_pipe->_request_step!=UCS_END))
            {
                FTP_TRACE( "Get p_ftp_pipe[0x%X]->_p_ftp_server_resource->_url._filename_valid_step=%d!",_p_ftp_pipe,_p_ftp_pipe->_request_step);
                _p_ftp_pipe->_p_ftp_server_resource->_url._filename_valid_step=_p_ftp_pipe->_request_step;
                _p_ftp_pipe->_request_step=UCS_ORIGIN;
            }

            /* Get buffer from data manager */
            _p_ftp_pipe->_error_code =  ftp_pipe_get_buffer(_p_ftp_pipe );
            if(_p_ftp_pipe->_error_code == SUCCESS)
            {
                /* read file data from socket */
                if(!_p_ftp_pipe->_b_data_full)
                {
                    _u32 _block_len = socket_proxy_get_p2s_recv_block_size();//_p_ftp_pipe->_reveive_block_len;

                    if(_p_ftp_pipe->_data._data_length-_p_ftp_pipe->_data._buffer_end_pos<_block_len)
                        _block_len = _p_ftp_pipe->_data._data_length-_p_ftp_pipe->_data._buffer_end_pos;


                    _p_ftp_pipe->_error_code = SOCKET_PROXY_RECV(_p_ftp_pipe->_pasv._socket_fd,
                                               _p_ftp_pipe->_data._buffer,_block_len,
                                               ftp_pipe_handle_recv,(void*)_p_ftp_pipe);
                }

            }
            return _p_ftp_pipe->_error_code;
            break;
        case 426: // Data connection closed, file transfer xxx aborted by client
            FTP_TRACE( "426:Data connection closed, file transfer xxx aborted by client" );
            /* Close the FTP data connection */
            _p_ftp_pipe->_error_code =  ftp_pipe_close_pasv( _p_ftp_pipe);
            if(_p_ftp_pipe->_error_code)
                /* closing socket error ! */
                return _p_ftp_pipe->_error_code;
            FTP_TRACE("!!!!!_p_ftp_pipe->_b_ranges_changed=%d,_is_support_long_connect=%d",_p_ftp_pipe->_b_ranges_changed,_p_ftp_pipe->_data_pipe._dispatch_info._is_support_long_connect);
            /* Download the next range ? */
            if((_p_ftp_pipe->_b_ranges_changed == TRUE) &&(_p_ftp_pipe->_data_pipe._dispatch_info._is_support_long_connect != TRUE))
            {
                return ftp_pipe_close_connection( _p_ftp_pipe);
            }
            else if((_p_ftp_pipe->_b_ranges_changed == TRUE) &&(_p_ftp_pipe->_data_pipe._dispatch_info._is_support_long_connect == TRUE))
            {
                FTP_TRACE("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@_p_ftp_pipe=0x%X,FTP_COMMAND_PASV",_p_ftp_pipe);
                return ftp_pipe_send_command(_p_ftp_pipe,FTP_COMMAND_PASV);
            }
            else
                /* Downloading is completed! */
                _p_ftp_pipe->_ftp_state = FTP_PIPE_STATE_DOWNLOAD_COMPLETED;

            break;
        case 226: // ABOR command successful.
        {
            FTP_TRACE( "226:ABOR command successful." );
            /* Close the FTP data connection */
            _p_ftp_pipe->_error_code =  ftp_pipe_close_pasv( _p_ftp_pipe);
            if(_p_ftp_pipe->_error_code)
                /* closing socket error ! */
                return _p_ftp_pipe->_error_code;

            if(_p_ftp_pipe->_cur_command_id == FTP_COMMAND_ABOR)
            {
                /* Check if the "451" is also received */
                //FTP_TRACE(" if 451 :%s",_p_ftp_pipe->_response._buffer+dispose_data_len);

                /* Download the next range ? */
                if(_p_ftp_pipe->_b_ranges_changed == TRUE)
                    return ftp_pipe_send_command(_p_ftp_pipe,FTP_COMMAND_PASV);
                else
                    _p_ftp_pipe->_ftp_state = FTP_PIPE_STATE_DOWNLOAD_COMPLETED;

            }
            else if(_p_ftp_pipe->_cur_command_id == FTP_COMMAND_QUIT)
            {
                pos = sd_strstr(_p_ftp_pipe->_response._buffer+dispose_data_len,"221", 0);
                if(( dispose_data_len < respn_len )&&(pos!=NULL))
                {
                    /* Download the next range ? */
                    if(_p_ftp_pipe->_b_ranges_changed == TRUE)
                    {
                        return ftp_pipe_close_connection( _p_ftp_pipe);
                    }
                    else
                        _p_ftp_pipe->_ftp_state = FTP_PIPE_STATE_DOWNLOAD_COMPLETED;

                }
                else
                    /*Need continue to read the "221 Goodbye, closing session." */
                {
                    _p_ftp_pipe->_ftp_state = FTP_PIPE_STATE_READING;
                    _p_ftp_pipe->_response._buffer_end_pos = 0;
                    _p_ftp_pipe->_response._code = 0;
                    SAFE_DELETE(_p_ftp_pipe->_response._description);
                }

            }
            else
            {
                _p_ftp_pipe->_ftp_state = FTP_PIPE_STATE_READING;
                _p_ftp_pipe->_response._buffer_end_pos = 0;
                _p_ftp_pipe->_response._code = 0;
                SAFE_DELETE(_p_ftp_pipe->_response._description);
            }
            //return FTP_ERR_INVALID_RESPONSE;


        }
        break;
        case 451: // Operation aborted due to ABOR command.
            FTP_TRACE( "451:Operation aborted due to ABOR command." );
            /* Download the next range ? */
            if(_p_ftp_pipe->_b_ranges_changed == TRUE)
                return ftp_pipe_send_command(_p_ftp_pipe,FTP_COMMAND_PASV);
            else
                _p_ftp_pipe->_ftp_state = FTP_PIPE_STATE_DOWNLOAD_COMPLETED;
            break;
        case 221: // Goodbye...
            FTP_TRACE( "221:Goodbye... " );
            /* Close the FTP data connection */
            _p_ftp_pipe->_error_code =  ftp_pipe_close_pasv( _p_ftp_pipe);
            if(_p_ftp_pipe->_error_code)
                /* closing socket error ! */
                return _p_ftp_pipe->_error_code;
            /* Download the next range ? */
            if(_p_ftp_pipe->_b_ranges_changed == TRUE)
            {
                return ftp_pipe_close_connection( _p_ftp_pipe);
            }
            else
                _p_ftp_pipe->_ftp_state = FTP_PIPE_STATE_DOWNLOAD_COMPLETED;

            break;
        case 530: //Not logged in.
            FTP_TRACE( "530:Not logged in. " );
            if((_p_ftp_pipe->_cur_command_id == FTP_COMMAND_USER)
               ||(_p_ftp_pipe->_cur_command_id == FTP_COMMAND_PASWD))
            {
                return FTP_ERR_INVALID_USER;
            }
            else
                return FTP_ERR_INVALID_RESPONSE;
            break;
        case 550:
            FTP_TRACE( "550:._cur_command_id=%d" ,_p_ftp_pipe->_cur_command_id);

            if((_p_ftp_pipe->_cur_command_id == FTP_COMMAND_USER)
               ||(_p_ftp_pipe->_cur_command_id == FTP_COMMAND_PASWD))
            {
                // Permission denied.
                FTP_TRACE( "550:Permission denied. " );
                return FTP_ERR_INVALID_USER;
            }
            else if(_p_ftp_pipe->_cur_command_id == FTP_COMMAND_FEAT)
            {
                // Permission denied.
                FTP_TRACE( "550:Extension is not supported " );
                /* Connecting success ! */
                //_p_ftp_pipe->_data_pipe._dispatch_info._pipe_state = PIPE_DOWNLOADING;
                dp_set_state(&_p_ftp_pipe->_data_pipe, PIPE_DOWNLOADING);
                _p_ftp_pipe->_ftp_state = FTP_PIPE_STATE_CONNECTED;

                _p_ftp_pipe->_data_pipe._dispatch_info._is_support_long_connect = TRUE; /* ???? */

                /* change to the work drectory */
                if(_p_ftp_pipe->_b_ranges_changed == TRUE)
                {
                    return ftp_pipe_send_command(_p_ftp_pipe,FTP_COMMAND_CWD);
                }
                else
                {
                    LOG_DEBUG( "_p_ftp_pipe=0x%X:dm_notify_dispatch_data_finish ",_p_ftp_pipe);
                    return pi_notify_dispatch_data_finish((DATA_PIPE *)_p_ftp_pipe);
                }

            }
            else if(_p_ftp_pipe->_cur_command_id == FTP_COMMAND_MDTM)
            {
                // Last modify time ...
                FTP_TRACE( "550:No such file or Last modify time  is not supported " );

                //sd_memset(_p_ftp_pipe->_p_ftp_server_resource->_lastmodified_from_server,0,MAX_LAST_MODIFIED_LEN);
                //sd_strncpy(_p_ftp_pipe->_p_ftp_server_resource->_lastmodified_from_server, _p_ftp_pipe->_response._description,MAX_LAST_MODIFIED_LEN-1);

                /* Get the file size */
                return ftp_pipe_send_command(_p_ftp_pipe,FTP_COMMAND_SIZE);

            }
            else if(_p_ftp_pipe->_cur_command_id == FTP_COMMAND_SIZE)
            {
                //No such file
                FTP_TRACE( "550:No such file or FTP_COMMAND_SIZE  is not supported " );
                //_p_ftp_pipe->_p_ftp_server_resource->_b_file_exist = FALSE;
                /* request for passive mode */
                return ftp_pipe_send_command(_p_ftp_pipe,FTP_COMMAND_PASV);
            }
            else if(_p_ftp_pipe->_cur_command_id == FTP_COMMAND_RETR)
            {
                FTP_TRACE( "550:No such file or Cannot RESTart beyond end-of-file " );
                /* Maybe the filename is in wrong format */
                if(_p_ftp_pipe->_p_ftp_server_resource->_url._filename_encode_mode!=UEM_ASCII)
                {
                    if(_p_ftp_pipe->_p_ftp_server_resource->_url._filename_valid_step==UCS_ORIGIN)
                    {
                        _p_ftp_pipe->_b_retry_request = TRUE;
                        /* Close the FTP data connection */
                        _p_ftp_pipe->_error_code =  ftp_pipe_close_pasv( _p_ftp_pipe);
                        if(_p_ftp_pipe->_error_code)
                            /* closing socket error ! */
                            return _p_ftp_pipe->_error_code;
                        FTP_TRACE("p_ftp_pipe=0x%X,FTP_COMMAND_PASV to retry request! _request_step=%d",_p_ftp_pipe,_p_ftp_pipe->_request_step);
                        ret_val =ftp_pipe_send_command(_p_ftp_pipe,FTP_COMMAND_PASV);
                        if(ret_val==SUCCESS)
                        {
                            return ret_val;
                        }
                    }
                }

                _p_ftp_pipe->_p_ftp_server_resource->_url._filename_valid_step=UCS_END;
				ftp_pipe_close_connection( _p_ftp_pipe);
                //Cannot RESTart beyond end-of-file.
                return FTP_ERR_FILE_NOT_FOUND;
            }
            else if(_p_ftp_pipe->_cur_command_id == FTP_COMMAND_CWD)
            {
                FTP_TRACE( "550:No such directory " );

                /* Maybe the path is in wrong format */
                if(_p_ftp_pipe->_p_ftp_server_resource->_url._path_encode_mode!=UEM_ASCII)
                {
                    if(_p_ftp_pipe->_p_ftp_server_resource->_url._path_valid_step==UCS_ORIGIN)
                    {
                        _p_ftp_pipe->_b_retry_request = TRUE;
                        ret_val =ftp_pipe_send_command(_p_ftp_pipe,FTP_COMMAND_CWD);
                        if(ret_val==SUCCESS)
                        {
                            return ret_val;
                        }
                    }
                }

                if(_p_ftp_pipe->_response._description!=NULL)
                {
                    colon_pos= sd_strrchr(_p_ftp_pipe->_response._description, ':');
                    if((colon_pos!=NULL)&&(colon_pos-_p_ftp_pipe->_response._description>0)&&(colon_pos-_p_ftp_pipe->_response._description<MAX_FULL_PATH_BUFFER_LEN-2)&&(_p_ftp_pipe->_p_ftp_server_resource->_b_path_changed==FALSE))
                    {
                        //sd_memset(PathBuffer1,0,MAX_FILE_PATH_LEN);
                        sd_memcpy(PathBuffer1,_p_ftp_pipe->_response._description,colon_pos-_p_ftp_pipe->_response._description);
                        if(sd_strlen(sd_strrchr(PathBuffer1, '/'))>1)
                        {
                            sd_strcat(PathBuffer1, "/",2);
                        }

                        //sd_memset(PathBuffer2,0,MAX_FILE_PATH_LEN);
                        sd_memcpy(PathBuffer2,_p_ftp_pipe->_p_ftp_server_resource->_url._path,_p_ftp_pipe->_p_ftp_server_resource->_url._path_len);
                        if(sd_strcmp(PathBuffer1, PathBuffer2)!=0)
                        {
                            FTP_TRACE( "Get new path:%s -> %s ", PathBuffer2,PathBuffer1);
                            sd_memset(PathBuffer2,0,MAX_FULL_PATH_BUFFER_LEN);
                            sd_memcpy(PathBuffer2,_p_ftp_pipe->_p_ftp_server_resource->_url._file_name,_p_ftp_pipe->_p_ftp_server_resource->_url._file_name_len);
                            sd_memset(_p_ftp_pipe->_p_ftp_server_resource->_url._full_path,0,MAX_FULL_PATH_BUFFER_LEN);
                            sd_strncpy(_p_ftp_pipe->_p_ftp_server_resource->_url._full_path,PathBuffer1,MAX_FULL_PATH_BUFFER_LEN);

                            _p_ftp_pipe->_p_ftp_server_resource->_url._path=_p_ftp_pipe->_p_ftp_server_resource->_url._full_path;
                            _p_ftp_pipe->_p_ftp_server_resource->_url._path_len=sd_strlen(_p_ftp_pipe->_p_ftp_server_resource->_url._full_path);
                            _p_ftp_pipe->_p_ftp_server_resource->_url._path_encode_mode = url_get_encode_mode( _p_ftp_pipe->_p_ftp_server_resource->_url._path ,_p_ftp_pipe->_p_ftp_server_resource->_url._path_len );
                            _p_ftp_pipe->_p_ftp_server_resource->_url._path_valid_step = 0;

                            sd_strcat(_p_ftp_pipe->_p_ftp_server_resource->_url._full_path, PathBuffer2,_p_ftp_pipe->_p_ftp_server_resource->_url._file_name_len);

                            _p_ftp_pipe->_p_ftp_server_resource->_url._file_name=_p_ftp_pipe->_p_ftp_server_resource->_url._full_path+_p_ftp_pipe->_p_ftp_server_resource->_url._path_len;
                            _p_ftp_pipe->_p_ftp_server_resource->_url._file_name_len=sd_strlen(_p_ftp_pipe->_p_ftp_server_resource->_url._file_name);
                            _p_ftp_pipe->_p_ftp_server_resource->_url._filename_encode_mode = url_get_encode_mode( _p_ftp_pipe->_p_ftp_server_resource->_url._file_name ,_p_ftp_pipe->_p_ftp_server_resource->_url._file_name_len );
                            _p_ftp_pipe->_p_ftp_server_resource->_url._filename_valid_step = 0;
                            _p_ftp_pipe->_p_ftp_server_resource->_b_path_changed = TRUE;
                            _p_ftp_pipe->_request_step=0;
                            /* change to the work drectory */
                            return ftp_pipe_send_command(_p_ftp_pipe,FTP_COMMAND_CWD);

                        }

                    }
                }
                //No such directory.
                _p_ftp_pipe->_p_ftp_server_resource->_url._path_valid_step=UCS_END;
                return FTP_ERR_NO_SUCH_DIR;
            }
            else
            {
                FTP_TRACE( "550:FTP_ERR_INVALID_RESPONSE" );
                return FTP_ERR_INVALID_RESPONSE;
            }

            break;

        case 502: //Command not implemented.
            FTP_TRACE( "502:Command not implemented._cur_command_id=%d" ,_p_ftp_pipe->_cur_command_id);
            if(_p_ftp_pipe->_cur_command_id == FTP_COMMAND_FEAT)
            {
                /* Connecting success ! */
                //_p_ftp_pipe->_data_pipe._dispatch_info._pipe_state = PIPE_DOWNLOADING;
                dp_set_state(&_p_ftp_pipe->_data_pipe, PIPE_DOWNLOADING);
                _p_ftp_pipe->_ftp_state = FTP_PIPE_STATE_CONNECTED;

                _p_ftp_pipe->_data_pipe._dispatch_info._is_support_long_connect = TRUE; /* ???? */

                /* change to the work drectory */
                if(_p_ftp_pipe->_b_ranges_changed == TRUE)
                {
                    return ftp_pipe_send_command(_p_ftp_pipe,FTP_COMMAND_CWD);
                }
                else
                {
                    LOG_DEBUG( "_p_ftp_pipe=0x%X:dm_notify_dispatch_data_finish ",_p_ftp_pipe);
                    return pi_notify_dispatch_data_finish((DATA_PIPE *)_p_ftp_pipe);
                }
            }
            else if(_p_ftp_pipe->_cur_command_id == FTP_COMMAND_MDTM)
            {
                /* Get the file size */
                return ftp_pipe_send_command(_p_ftp_pipe,FTP_COMMAND_SIZE);
            }
            else if(_p_ftp_pipe->_cur_command_id == FTP_COMMAND_SIZE)
            {
                /* request for passive mode */
                return ftp_pipe_send_command(_p_ftp_pipe,FTP_COMMAND_PASV);
            }
            else if(_p_ftp_pipe->_cur_command_id == FTP_COMMAND_REST)
            {
                _p_ftp_pipe->_data_pipe._dispatch_info._is_support_range = FALSE;
                _p_ftp_pipe->_p_ftp_server_resource->_is_support_range=FALSE;
                _p_ftp_pipe->_error_code = dp_get_uncomplete_ranges_head_range(&(_p_ftp_pipe->_data_pipe), &uncom_head_range);
                if(_p_ftp_pipe->_error_code ) return _p_ftp_pipe->_error_code  ;

                if(uncom_head_range._index == 0)
                {
                    /* send the restart from XXXX */
                    _p_ftp_pipe->_error_code = ftp_pipe_reset_data( _p_ftp_pipe,&(_p_ftp_pipe->_data) );
                    if(_p_ftp_pipe->_error_code)
                        return _p_ftp_pipe->_error_code;
                    /*--------------- Range ----------------------*/
                    if(uncom_head_range._num!=0)
                    {

                        down_range._index = uncom_head_range._index;
                        down_range._num = uncom_head_range._num;

                        _p_ftp_pipe->_error_code = dp_set_download_range( (DATA_PIPE *)_p_ftp_pipe, &down_range );
                        if(_p_ftp_pipe->_error_code!=SUCCESS) return _p_ftp_pipe->_error_code;

                        if(_p_ftp_pipe->_p_ftp_server_resource->_file_size<
                           (down_range._index+down_range._num)*FTP_DEFAULT_RANGE_LEN)
                            _p_ftp_pipe->_data._content_length = _p_ftp_pipe->_p_ftp_server_resource->_file_size-
                                                                 (down_range._index*FTP_DEFAULT_RANGE_LEN);
                        else
                            _p_ftp_pipe->_data._content_length = (down_range._num)*FTP_DEFAULT_RANGE_LEN;
                    }
                    else
                    {
                        /* Error */
                        return FTP_ERR_INVALID_RANGES;
                    }

                    _p_ftp_pipe->_b_ranges_changed =FALSE;

                    /* Connect to passive mode port... */
                    _p_ftp_pipe->_error_code = ftp_pipe_open_pasv(_p_ftp_pipe);
                    if(_p_ftp_pipe->_error_code)
                    {
                        /* Open socket error ! */
                        return _p_ftp_pipe->_error_code;
                    }
                    /* Start download the file data  */
                    return ftp_pipe_send_command(_p_ftp_pipe,FTP_COMMAND_RETR);
                }
                else
                    return FTP_ERR_NO_SUPPORT_RANGE;

            }
            else if(_p_ftp_pipe->_cur_command_id == FTP_COMMAND_ABOR)
            {
                _p_ftp_pipe->_data_pipe._dispatch_info._is_support_long_connect = FALSE;

                /* Download the next range ? */
                if(_p_ftp_pipe->_b_ranges_changed == TRUE)
                    return ftp_pipe_send_command(_p_ftp_pipe,FTP_COMMAND_QUIT);
                else
                    _p_ftp_pipe->_ftp_state = FTP_PIPE_STATE_DOWNLOAD_COMPLETED;
            }
            else
            {
                return FTP_ERR_INVALID_COMMAND;
            }
            break;


        default:
        {
            /* Failed */
			if(_p_ftp_pipe->_cur_command_id == FTP_COMMAND_FEAT)
            {
                /* Connecting success ! */
                //_p_ftp_pipe->_data_pipe._dispatch_info._pipe_state = PIPE_DOWNLOADING;
                dp_set_state(&_p_ftp_pipe->_data_pipe, PIPE_DOWNLOADING);
                _p_ftp_pipe->_ftp_state = FTP_PIPE_STATE_CONNECTED;

                _p_ftp_pipe->_data_pipe._dispatch_info._is_support_long_connect = TRUE; /* ???? */

                /* change to the work drectory */
                if(_p_ftp_pipe->_b_ranges_changed == TRUE)
                {
                    return ftp_pipe_send_command(_p_ftp_pipe,FTP_COMMAND_CWD);
                }
                else
                {
                    LOG_DEBUG( "_p_ftp_pipe=0x%X:dm_notify_dispatch_data_finish ",_p_ftp_pipe);
                    return pi_notify_dispatch_data_finish((DATA_PIPE *)_p_ftp_pipe);
                }
            }
            FTP_TRACE(" Invalid response received:%d",code);
            return FTP_ERR_INVALID_RESPONSE;
        }
    }
    return SUCCESS;
} /* End of ftp_pipe_parse_response */

_int32 ftp_pipe_get_buffer( FTP_DATA_PIPE * _p_ftp_pipe )
{
    RANGE down_range;
    FTP_TRACE( " enter ftp_pipe_get_buffer(_content_length=%llu,_recv_end_pos=%llu)..." ,_p_ftp_pipe->_data._content_length,_p_ftp_pipe->_data._recv_end_pos);

    _p_ftp_pipe->_data._buffer_length = 0;
    _p_ftp_pipe->_data._buffer_end_pos = 0;
    _p_ftp_pipe->_data._data_length = 0;

    if(_p_ftp_pipe->_data._content_length != 0)
    {
        /* Change the reveiving range number */
        if((_p_ftp_pipe->_data._content_length-_p_ftp_pipe->_data._recv_end_pos) >= (FTP_DEFAULT_RANGE_LEN*_p_ftp_pipe->_reveive_range_num))
        {
            /* receive 4 range every time */
            _p_ftp_pipe->_data._data_length = FTP_DEFAULT_RANGE_LEN*_p_ftp_pipe->_reveive_range_num;
            _p_ftp_pipe->_data._current_recving_range._num = _p_ftp_pipe->_reveive_range_num;
        }
        else
        {
            /* receive the last range */
            _p_ftp_pipe->_data._data_length = _p_ftp_pipe->_data._content_length-_p_ftp_pipe->_data._recv_end_pos;
            if((_p_ftp_pipe->_data._data_length)%(FTP_DEFAULT_RANGE_LEN)!=0)
                _p_ftp_pipe->_data._current_recving_range._num = (_p_ftp_pipe->_data._data_length+(FTP_DEFAULT_RANGE_LEN-
                        ((_p_ftp_pipe->_data._data_length)%(FTP_DEFAULT_RANGE_LEN))))/(FTP_DEFAULT_RANGE_LEN);
            else
                _p_ftp_pipe->_data._current_recving_range._num = (_p_ftp_pipe->_data._data_length)/(FTP_DEFAULT_RANGE_LEN);

        }

        /* Change the reveiving range index */
        if(_p_ftp_pipe->_data._recv_end_pos != 0)
        {
            _p_ftp_pipe->_data._current_recving_range._index+= _p_ftp_pipe->_reveive_range_num;
        }
        else
        {
            _p_ftp_pipe->_error_code = dp_get_download_range( (DATA_PIPE *)_p_ftp_pipe, &down_range );
            if(_p_ftp_pipe->_error_code!=SUCCESS) return _p_ftp_pipe->_error_code;

            _p_ftp_pipe->_data._current_recving_range._index = down_range._index;
        }

        _p_ftp_pipe->_data._buffer_length = _p_ftp_pipe->_data._data_length;

        /* Get buffer from data manager */
        FTP_TRACE( " DM_GET_DATA_BUFFER(_p_data_manager=0x%X,_p_ftp_pipe=0x%X,_buffer=0x%X,&(_buffer)=0x%X,_buffer_length=%u)..." ,
                   _p_ftp_pipe->_data_pipe._p_data_manager,_p_ftp_pipe,_p_ftp_pipe->_data._buffer,&(_p_ftp_pipe->_data._buffer),_p_ftp_pipe->_data._buffer_length);
        sd_assert(_p_ftp_pipe->_data._buffer == NULL);
        _p_ftp_pipe->_error_code =pi_get_data_buffer(  (DATA_PIPE *) _p_ftp_pipe,  &(_p_ftp_pipe->_data._buffer),&( _p_ftp_pipe->_data._buffer_length));
        if(_p_ftp_pipe->_error_code)
        {
            /* The data manager if full,need get buffer later,wait... */
            if(!_p_ftp_pipe->_b_data_full)
            {
                _p_ftp_pipe->_b_data_full = TRUE;
                pipe_set_err_get_buffer( &(_p_ftp_pipe->_data_pipe), TRUE );
                _p_ftp_pipe->_get_buffer_try_count = 0;
                /* Start timer */
                FTP_TRACE("call START_TIMER");
                return START_TIMER(ftp_pipe_handle_timeout, NOTICE_ONCE,ftp_pipe_get_buffer_wait_timeout(_p_ftp_pipe),0,  _p_ftp_pipe,&(_p_ftp_pipe->_retry_get_buffer_timer_id));
            }
            else
            {
                /* Error */
                FTP_TRACE("FTP_ERR_GETING_BUFFER");
                return FTP_ERR_GETING_BUFFER;

            }
        }
    }
    else
    {
        FTP_TRACE("FTP_ERR_NO_CONTENT_LEN");
        return FTP_ERR_NO_CONTENT_LEN;
    }

    return SUCCESS;

}


_int32 ftp_pipe_recving_range_success( FTP_DATA_PIPE * _p_ftp_pipe)
{
    FTP_TRACE( " enter ftp_pipe_recving_range_success(_index=%u,_num=%u)..." ,_p_ftp_pipe->_data._current_recving_range._index,_p_ftp_pipe->_data._current_recving_range._num);


    if(_p_ftp_pipe->_data._buffer != NULL)
    {
        /* Updata the pipe speed */
        //_p_ftp_pipe->_error_code =add_speed_record(&(_p_ftp_pipe->_speed_calculator), _p_ftp_pipe->_data._data_length);
        //if(_p_ftp_pipe->_error_code!=SUCCESS)
        //  return _p_ftp_pipe->_error_code;
        /* Put the data to data manager */
        FTP_TRACE( " DM_PUT_DATA:_p_data_manager=0x%X,_p_ftp_pipe=0x%X,_buffer=0x%X,&(_buffer)=0x%X, _data_length=%d,_buffer_length=%d",
                   _p_ftp_pipe->_data_pipe._p_data_manager,_p_ftp_pipe,_p_ftp_pipe->_data._buffer,&(_p_ftp_pipe->_data._buffer),_p_ftp_pipe->_data._data_length, _p_ftp_pipe->_data._buffer_length);
        _p_ftp_pipe->_error_code = pi_put_data( (DATA_PIPE *) _p_ftp_pipe, &(_p_ftp_pipe->_data._current_recving_range),
                                                &(_p_ftp_pipe->_data._buffer),  _p_ftp_pipe->_data._data_length,_p_ftp_pipe->_data._buffer_length,_p_ftp_pipe->_data_pipe._p_resource );

        if(_p_ftp_pipe->_error_code)
            return _p_ftp_pipe->_error_code;

        _p_ftp_pipe->_data._buffer_length = 0;
        _p_ftp_pipe->_data._buffer_end_pos = 0;
        _p_ftp_pipe->_data._data_length = 0;
    }
    FTP_TRACE( " _recv_end_pos=%u,_content_length=%u",_p_ftp_pipe->_data._recv_end_pos, _p_ftp_pipe->_data._content_length);
    /* if the current range is download success ? */
    if(_p_ftp_pipe->_data._recv_end_pos != _p_ftp_pipe->_data._content_length)
        /* Continue to downloading... */
        return ftp_pipe_get_buffer(_p_ftp_pipe );
    else
        return ftp_pipe_range_success(  _p_ftp_pipe);

}
_int32 ftp_pipe_range_success( FTP_DATA_PIPE * _p_ftp_pipe)
{
    _int32 result_t = SUCCESS;
    RANGE uncom_head_range,down_range;
    _u64 _start_pos = 0,_file_size=0,_received_pos=0;

    _p_ftp_pipe->_error_code = dp_get_download_range( (DATA_PIPE *)_p_ftp_pipe, &down_range );
    if(_p_ftp_pipe->_error_code!=SUCCESS) return _p_ftp_pipe->_error_code;

    /* Receiveing current ranges successed ! */
    FTP_TRACE( "_p_ftp_pipe=0x%X: enter ftp_pipe_range_success(_down_range._index=%u,_down_range._unm=%u,_data._content_length=%llu,_data._recv_end_pos=%llu,_b_ranges_changed=%d)..."
               ,_p_ftp_pipe,down_range._index,down_range._num,_p_ftp_pipe->_data._content_length,_p_ftp_pipe->_data._recv_end_pos,_p_ftp_pipe->_b_ranges_changed);

    /* Delete the current range the from the uncomplete range list */
    _p_ftp_pipe->_error_code =  dp_delete_uncomplete_ranges(&(_p_ftp_pipe->_data_pipe),&down_range);
    if(_p_ftp_pipe->_error_code) return _p_ftp_pipe->_error_code;

    /* reset the _down_range */
    down_range._index = 0;
    down_range._num = 0;
    _p_ftp_pipe->_error_code = dp_set_download_range( (DATA_PIPE *)_p_ftp_pipe, &down_range );
    if(_p_ftp_pipe->_error_code!=SUCCESS) return _p_ftp_pipe->_error_code;

    _received_pos = (_p_ftp_pipe->_data._current_recving_range._index *FTP_DEFAULT_RANGE_LEN)+_p_ftp_pipe->_data._content_length;

    if(_p_ftp_pipe->_p_ftp_server_resource->_file_size!=0)
        _file_size = _p_ftp_pipe->_p_ftp_server_resource->_file_size;
    else
        _file_size = pi_get_file_size( (DATA_PIPE *)_p_ftp_pipe);

    if(dp_get_uncomplete_ranges_list_size(&(_p_ftp_pipe->_data_pipe))!=0)
    {

        result_t = dp_get_uncomplete_ranges_head_range(&(_p_ftp_pipe->_data_pipe), &uncom_head_range);
        CHECK_VALUE(result_t);

        down_range._index =uncom_head_range._index;

        if(uncom_head_range._num>_p_ftp_pipe->_reveive_range_num)
            down_range._num = _p_ftp_pipe->_reveive_range_num;
        else
            down_range._num = uncom_head_range._num;

        _start_pos = down_range._index*FTP_DEFAULT_RANGE_LEN;

        _p_ftp_pipe->_error_code = dp_set_download_range( (DATA_PIPE *)_p_ftp_pipe, &down_range );
        if(_p_ftp_pipe->_error_code!=SUCCESS) return _p_ftp_pipe->_error_code;

        if((_file_size==0)||(_file_size<_start_pos))
            return FTP_ERR_INVALID_RANGES;

        _p_ftp_pipe->_data._content_length = (down_range._num)*FTP_DEFAULT_RANGE_LEN;
        if(_p_ftp_pipe->_data._content_length+_start_pos>_file_size)
        {
            _p_ftp_pipe->_data._content_length = _file_size-_start_pos;
        }

        FTP_TRACE( "_p_ftp_pipe=0x%X: range_node->_range._index=%u,range_node->_range._num=%u,_down_range._index=%u,_down_range._unm=%u,_received_pos=%llu,_start_pos=%llu,_file_size=%llu,_data._content_length=%llu,_data._recv_end_pos=%llu,_b_ranges_changed=%d"
                   ,_p_ftp_pipe,uncom_head_range._index,uncom_head_range._num,down_range._index,down_range._num,_received_pos,_start_pos,_file_size,_p_ftp_pipe->_data._content_length,_p_ftp_pipe->_data._recv_end_pos,_p_ftp_pipe->_b_ranges_changed);

        if(_received_pos==_start_pos)
        {
            /* Á¬ÐøµÄ¿é*/
            _p_ftp_pipe->_data._recv_end_pos    =0;
            _p_ftp_pipe->_b_ranges_changed =FALSE;

            if(uncom_head_range._num<=_p_ftp_pipe->_reveive_range_num)
                pi_notify_dispatch_data_finish((DATA_PIPE *)_p_ftp_pipe);

            FTP_TRACE( "OK, continue to downloading...");

            /* Continue to downloading... */
            return ftp_pipe_get_buffer(_p_ftp_pipe );

        }
        else
        {
            /* Ready to download next ranges */
            _p_ftp_pipe->_ftp_state = FTP_PIPE_STATE_RANGE_CHANGED;
            _p_ftp_pipe->_pasv._state = FTP_PIPE_STATE_RANGE_CHANGED;
            _p_ftp_pipe->_b_ranges_changed =TRUE;
            FTP_TRACE( " FTP_PIPE_STATE_RANGE_CHANGED..." );
        }
    }
    else
    {
        /* All ranges in _uncomplete_ranges are downloaded ! */
        _p_ftp_pipe->_pasv._state = FTP_PIPE_STATE_DOWNLOAD_COMPLETED;
        /* reset the speed */
        //uninit_speed_calculator(&(_p_ftp_pipe->_speed_calculator));
        //init_speed_calculator(&(_p_ftp_pipe->_speed_calculator), 10, 1000);
        //calculate_speed(&(_p_ftp_pipe->_speed_calculator), &(_p_ftp_pipe->_data_pipe._dispatch_info._speed));

        FTP_TRACE( " FTP_PIPE_STATE_DOWNLOAD_COMPLETED" );

        if(_p_ftp_pipe->_is_connected == TRUE)
        {
            FTP_TRACE( "_received_pos=%llu,_file_size=%llu,_is_support_long_connect=%d", _received_pos,_file_size,_p_ftp_pipe->_data_pipe._dispatch_info._is_support_long_connect);
            if(_file_size>_received_pos)
            {
                if(_p_ftp_pipe->_wait_delete==TRUE)
                {
                    /* Fatal error!*/
                    FTP_TRACE("Fatal error! should halt up the program!");
                    CHECK_VALUE(_p_ftp_pipe->_wait_delete);

                }
                /* stop transfering */
                FTP_TRACE( " Should stop the transfering!" );
                /*
                if(_p_ftp_pipe->_data_pipe._dispatch_info._is_support_long_connect==TRUE)
                    _p_ftp_pipe->_error_code = ftp_pipe_send_command(_p_ftp_pipe,FTP_COMMAND_ABOR);
                else
                    _p_ftp_pipe->_error_code =  ftp_pipe_send_command(_p_ftp_pipe,FTP_COMMAND_QUIT);

                if(_p_ftp_pipe->_error_code!=SUCCESS) return _p_ftp_pipe->_error_code;
                */
            }
        }

        pi_notify_dispatch_data_finish((DATA_PIPE *)_p_ftp_pipe);

    }


    return SUCCESS;

}



/*************************/
/* structrue reset      */
/*************************/

_int32 ftp_pipe_reset_data(FTP_DATA_PIPE * _p_ftp_pipe, FTP_DATA * _p_data )
{
    LOG_DEBUG( " enter ftp_pipe_reset_data()..." );

    if(_p_data->_buffer)
        pi_free_data_buffer( (DATA_PIPE *)_p_ftp_pipe,&(_p_data->_buffer),_p_data->_buffer_length);

    sd_memset(_p_data,0,sizeof(FTP_DATA));

    return SUCCESS;

}


_int32 ftp_pipe_reset_response( FTP_RESPONSE * _p_respn )
{
    LOG_DEBUG( " enter ftp_pipe_reset_response()..." );

    if(_p_respn == NULL ) return -1;

    SAFE_DELETE(_p_respn->_description);
    SAFE_DELETE(_p_respn->_buffer);
    sd_memset(_p_respn,0,sizeof(FTP_RESPONSE));

    return SUCCESS;

}






_int32 ftp_pipe_reset_pipe( FTP_DATA_PIPE * _p_ftp_pipe )
{

    LOG_DEBUG( " enter ftp_pipe_reset_pipe()..." );
    if(_p_ftp_pipe == NULL ) return -1;

    uninit_pipe_info( &(_p_ftp_pipe->_data_pipe) );


    ftp_pipe_reset_response( &(_p_ftp_pipe->_response));

    ftp_pipe_reset_data(_p_ftp_pipe, &(_p_ftp_pipe->_data) );

    SAFE_DELETE(_p_ftp_pipe->_ftp_request);

    sd_memset(_p_ftp_pipe,0,sizeof(FTP_DATA_PIPE));

    return SUCCESS;

}




#endif /* __FTP_DATA_PIPE_C_20080614 */

