#include "bt_magnet_task.h"
#ifdef ENABLE_BT 

#include "utility/sha1.h"
#include "p2sp_download/p2sp_task.h"
#include "connect_manager/connect_manager_interface.h"
#include "utility/base64.h"
#include "utility/utility.h"
#include "res_query/res_query_interface.h"
#include "../torrent_parser/bencode.h"
#include "utility/settings.h"
#include "platform/sd_fs.h"

#include "utility/logid.h"
#define	LOGID	LOGID_BT_MAGNET
#include "utility/logger.h"
#include "utility/utility.h"
#include "torrent_parser/torrent_parser_interface.h"

#include "task_manager/task_manager.h"

#define BT_MAGNET_TASK_DEFAULT_TIMEOUT (4*1000)  /* 4 s */

_int32 bm_create_task( char *url, char *file_path, char * file_name, BOOL bManual, BT_MAGNET_TASK **pp_task , TASK* task)
{
    _int32 ret_val = SUCCESS;
    BT_MAGNET_TASK *p_task = NULL;
	char file_path_tmp[MAX_FILE_PATH_LEN] = {0};
    char file_name_tmp[MAX_FILE_NAME_LEN] = {0};
    _u8 info_hash[INFO_HASH_LEN] = {0};
    const char *torrent_sign = ".torrent";
	_u32 url_len = sd_strlen(url);
	_u32 file_path_len = sd_strlen(file_path);
    
    *pp_task = NULL;
    
	LOG_DEBUG("bm_create_task, url:%s, file_path:%s, file_name:%s, bManual:%d", url, file_path, file_name, bManual);
	
    while(url[url_len - 1] == ' ')
    {
		url[url_len - 1] = 0;
		url_len--;
    }
	sd_strncpy(file_path_tmp, file_path, file_path_len);
    if( url_len > MAX_BT_MAGNET_LEN 
        || file_path_len > MAX_FILE_PATH_LEN )
    {
        return TM_ERR_INVALID_PARAMETER;
    }
	
	if (file_path_tmp[file_path_len - 1] != '/')
	{
		if (file_path_len + 1 == file_path_len) return TM_ERR_INVALID_PARAMETER;
		file_path_tmp[file_path_len++] = '/';
	}
	
    ret_val = bm_parse_url( url, url_len, info_hash, INFO_HASH_LEN, NULL );
    CHECK_VALUE( ret_val );

	if(0==sd_strlen(file_name))
	{
	    str2hex( (char*)info_hash, INFO_HASH_LEN, file_name_tmp, MAX_FILE_NAME_LEN );
	    sd_strcat( file_name_tmp, torrent_sign, sd_strlen(torrent_sign) );
		file_name = file_name_tmp;
	}

    ret_val = sd_malloc( sizeof(BT_MAGNET_TASK), (void**)&p_task );
    CHECK_VALUE( ret_val );
    
    ret_val = bm_init_task( p_task, url, file_path_tmp, file_name ,bManual);
    if( ret_val != SUCCESS )
    {
        sd_free( p_task );
        return ret_val;
    }

    ret_val = bmt_start( p_task, task );
    if( ret_val != SUCCESS )
    {
        bm_uninit_task( p_task );
        sd_free( p_task );
        return ret_val;
    }
    *pp_task = p_task;

    return SUCCESS;
}

_int32 bm_destory_task( BT_MAGNET_TASK *p_task )
{
	LOG_DEBUG("[task=0x%x] bm_destory_task", p_task );
	if (!p_task) return SUCCESS;
    bmt_stop( p_task );
    bm_uninit_task( p_task );
    sd_free( p_task );
    
    return SUCCESS;
}

_int32 bm_get_task_status( BT_MAGNET_TASK *p_task, MAGNET_TASK_STATUS *p_task_status )
{
	LOG_DEBUG("[task=0x%x] bm_get_task_status, status:%d", p_task, p_task->_task_status );

    *p_task_status = p_task->_task_status;
    return SUCCESS;
}

_int32 bm_get_seed_full_path( BT_MAGNET_TASK *p_task, char *p_file_full_path_buffer, _u32 buffer_len )
{
    _u32 path_len = 0;

    sd_strncpy( p_file_full_path_buffer, p_task->_data_manager._file_path, buffer_len );

    path_len = sd_strlen(p_task->_data_manager._file_path);

    if(path_len >= buffer_len) return -1;
    if(sd_strlen(p_task->_data_manager._file_name) > 0
		&& p_task->_data_manager._file_path[path_len - 1] != '/')
    {
		sd_strcat(p_file_full_path_buffer, "/", 1);
    }
    sd_strcat( p_file_full_path_buffer, p_task->_data_manager._file_name, buffer_len - path_len);

    LOG_DEBUG("[task=0x%x] bm_get_seed_full_path, fill_path:%s", p_task, p_file_full_path_buffer );
    
    return SUCCESS;
}

_int32 bm_get_data_path( BT_MAGNET_TASK *p_task, char *p_data_path_buffer, _u32 buffer_len )
{

    sd_strncpy( p_data_path_buffer, p_task->_data_manager._file_path, buffer_len );
    LOG_DEBUG("[task=0x%x] bm_get_data_path, fill_path:%s", p_task, p_data_path_buffer );

    return SUCCESS;
}

_int32 bm_init_task( BT_MAGNET_TASK *p_task, char url[], char file_path[], char file_name[] ,BOOL bManual)
{
	_int32 ret_val = SUCCESS;
	sd_memset( p_task, 0, sizeof(BT_MAGNET_TASK) );

	LOG_DEBUG("[task=0x%x] bm_init_task, fill_name:%s, bManual=%d", p_task, file_name ,bManual);

	list_init( &p_task->_tracker_url );
	p_task->_task_type = BT_MAGNET_TASK_TYPE;
	p_task->_bManual = bManual;

	ret_val = bm_parse_url( url, sd_strlen(url), p_task->_info_hash, INFO_HASH_LEN, &p_task->_tracker_url );
	if( ret_val != SUCCESS ) return ret_val;

	ret_val = bt_magnet_data_manager_init( &p_task->_data_manager, p_task, file_path, file_name );
	CHECK_VALUE( ret_val );

	p_task->_connect_manager_ptr = NULL;

	p_task->_res_query_bt_tracker_state = RES_QUERY_IDLE;
	p_task->_res_query_bt_dht_state = RES_QUERY_IDLE;
	p_task->_task_update_timer_id = INVALID_MSG_ID;
	p_task->_p2sp_task_ptr = NULL;

	return SUCCESS;

}

_int32 bm_uninit_task( BT_MAGNET_TASK *p_task )
{
    char *p_track_url = NULL;

    LOG_DEBUG("[task=0x%x] bm_uninit_task", p_task );

    LIST_ITERATOR cur_node = LIST_BEGIN( p_task->_tracker_url );
    while( cur_node != LIST_END(p_task->_tracker_url) )
    {
        p_track_url = (char*)LIST_VALUE( cur_node );
        sd_free( p_track_url );
        cur_node = LIST_NEXT( cur_node );
    }
    
    bt_magnet_data_manager_uninit( &p_task->_data_manager );
    return SUCCESS;
}

_int32 bmt_start( BT_MAGNET_TASK *p_bt_magnet_task, TASK* task )
{
    _int32 ret_val = SUCCESS;

    LOG_DEBUG("[task=0x%x] bmt_start", p_bt_magnet_task );

    ret_val = start_timer( bmt_update, NOTICE_INFINITE, BT_MAGNET_TASK_DEFAULT_TIMEOUT, 0,(void*)p_bt_magnet_task,&(p_bt_magnet_task->_task_update_timer_id));  
    CHECK_VALUE( ret_val );
    
    ret_val = sd_malloc( sizeof(CONNECT_MANAGER), (void **)&p_bt_magnet_task->_connect_manager_ptr );
    CHECK_VALUE( ret_val );

    ret_val = cm_init_bt_magnet_connect_manager( p_bt_magnet_task->_connect_manager_ptr, &p_bt_magnet_task->_data_manager );

    p_bt_magnet_task->_connect_manager_ptr->_main_task_ptr = task;
    // 避免获取资源信息时connect_manager未初始化引起的崩溃
    cm_init_struct( &task->_connect_manager, (void *)NULL, NULL );
      	
    	
    
    if( ret_val != SUCCESS )
    {
        bt_magnet_data_manager_uninit( &p_bt_magnet_task->_data_manager );
        SAFE_DELETE( p_bt_magnet_task->_connect_manager_ptr );
        return ret_val;
    }

    if( bmt_verify_is_torrent_ok( p_bt_magnet_task, p_bt_magnet_task->_data_manager._file_name ) )
    {
        p_bt_magnet_task->_task_status = MAGNET_TASK_SUCCESS;
        bmt_stop( p_bt_magnet_task );
        return SUCCESS;
    }

    bmt_res_query_bt_track( p_bt_magnet_task );
    bmt_res_query_dht( p_bt_magnet_task );
    bmt_start_p2sp_download( p_bt_magnet_task );
    
    return SUCCESS;
}

_int32 bmt_stop( BT_MAGNET_TASK *p_bt_magnet_task )
{
    LOG_DEBUG("[task=0x%x] bmt_stop", p_bt_magnet_task );
    
    bmt_stop_track( p_bt_magnet_task );
    bmt_stop_dht( p_bt_magnet_task );
    bmt_stop_p2sp_download( p_bt_magnet_task );

    if( p_bt_magnet_task->_task_update_timer_id != INVALID_MSG_ID )
    {
        cancel_timer( p_bt_magnet_task->_task_update_timer_id );
        p_bt_magnet_task->_task_update_timer_id = INVALID_MSG_ID;
    }

    if( p_bt_magnet_task->_connect_manager_ptr != NULL )    
    {
        cm_uninit_bt_magnet_connect_manager( p_bt_magnet_task->_connect_manager_ptr );
        SAFE_DELETE( p_bt_magnet_task->_connect_manager_ptr );    
    }

    return SUCCESS;
}

_int32 bmt_update(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid)
{
	BT_MAGNET_TASK *p_task = (BT_MAGNET_TASK *)(msg_info->_user_data);
	if(errcode == MSG_CANCELLED) return SUCCESS;
    LOG_DEBUG("[task=0x%x] bmt_update", p_task );

    if( p_task->_task_status != MAGNET_TASK_RUNNING ) return SUCCESS;
    if (p_task->_p2sp_task_ptr)
    {
	    pt_update_task_info( &p_task->_p2sp_task_ptr->_task );
	    if( p_task->_p2sp_task_ptr->_task.task_status == TASK_S_SUCCESS )
	    {
	        p_task->_task_status = MAGNET_TASK_SUCCESS;
	        LOG_ERROR("p_task->_task_status == MAGNET_TASK_SUCCESS");
			return SUCCESS;
	    }
    }

	if (cm_is_idle_status(p_task->_connect_manager_ptr, INVALID_FILE_INDEX))
	{
		p_task->_task_status = MAGNET_TASK_FAIL;
		return SUCCESS;
	}

    bmt_start_create_pipes( p_task );
    return SUCCESS;
}

_int32 bm_parse_url( char url[], _u32 url_len, _u8 infohash_buffer[], _u32 buffer_len,
    LIST *p_tracker_list )
{
    _int32 ret_val = SUCCESS;
	LIST url_segment_list;
    const char *url_head = "magnet:?";
    char *p_str = NULL, *p_segment = NULL, *p_segment_sign = NULL, *p_tracker_url = NULL;
    LIST_ITERATOR cur_node = NULL;
    char track_url[MAX_URL_LEN] = {0};

	LOG_DEBUG("bm_parse_url, url:%s", url );

    if( buffer_len != INFO_HASH_LEN
        || infohash_buffer == NULL
        || url == NULL
        || url_len > MAX_BT_MAGNET_LEN ) 
    {
        return TM_ERR_INVALID_PARAMETER;
    }
    if( sd_strncmp( url, url_head, sd_strlen(url_head )) )
    {
        return BT_MAGNET_INVALID_URL;
    }
    p_str = url + sd_strlen( url_head );
    list_init( &url_segment_list );
    
    ret_val = sd_divide_str( p_str, '&', &url_segment_list );
    if( ret_val != SUCCESS ) return ret_val;
    cur_node = LIST_BEGIN( url_segment_list );
    while( cur_node != LIST_END( url_segment_list) )
    {
        p_segment = (char *)LIST_VALUE( cur_node );
        p_segment_sign = NULL;
        p_segment_sign = sd_strstr( p_segment, "=", 0 );
        if( p_segment_sign == NULL )
        {
            ret_val = BT_MAGNET_INVALID_URL;
            goto ERR_HANDLE;
        }
        // *p_segment_sign = '\0';
        if( sd_strncmp( p_segment, "xt", 2)== 0 )
        {
            p_segment_sign = NULL;
            p_segment_sign = sd_strstr( p_segment, "urn:btih:", 3 );
            if( p_segment_sign == NULL ) 
            {
                ret_val = BT_MAGNET_INVALID_URL;
                goto ERR_HANDLE;
            }
            p_segment_sign += sd_strlen( "urn:btih:" );
            if( sd_strlen(p_segment_sign) == 32 )
            {
                ret_val = sd_decode_base32( p_segment_sign, 32, (char*)infohash_buffer, buffer_len );
                if( ret_val != SUCCESS ) goto ERR_HANDLE;
            }
            else if( sd_strlen(p_segment_sign) == 40 )
            {
                ret_val = sd_string_to_cid( p_segment_sign, infohash_buffer );
                if( ret_val != SUCCESS ) goto ERR_HANDLE;
            }
        }
        else if( p_tracker_list != NULL && sd_strncmp( p_segment, "tr", 2)== 0 )
        {
            sd_memset( track_url, 0, MAX_URL_LEN );
            ret_val = url_object_decode( p_segment_sign+1, track_url, MAX_URL_LEN );
            if( ret_val != SUCCESS ) goto ERR_HANDLE;
            ret_val = sd_malloc( sd_strlen(track_url)+1, (void**)&p_tracker_url );
            if( ret_val != SUCCESS ) goto ERR_HANDLE;
            sd_strncpy( p_tracker_url, track_url, sd_strlen(track_url) );
            p_tracker_url[sd_strlen(track_url)] = '\0';
            LOG_DEBUG("bm_parse_url, tracker:%s", p_tracker_url );

            
            ret_val = list_push( p_tracker_list, p_tracker_url );
            if( ret_val != SUCCESS )
            {
               sd_assert(FALSE);
               goto ERR_HANDLE; 
            }
        }
        cur_node = LIST_NEXT( cur_node );
    }
    
ERR_HANDLE:
    while( list_size(&url_segment_list) != 0 )
    {
        list_pop( &url_segment_list, (void**)&p_segment );
        sd_free( p_segment );
    }
    if( ret_val != SUCCESS && p_tracker_list != NULL )
    {
        while( list_size(p_tracker_list) != 0 )
        {
            list_pop( p_tracker_list, (void**)&p_tracker_url );
            sd_free( p_tracker_url );
        }
    }
    return ret_val;
}
 
_int32 bmt_res_query_bt_track( BT_MAGNET_TASK *p_bt_magnet_task )
{
#ifdef ENABLE_BT_PROTOCOL
    _int32 ret_val = SUCCESS;
    char *p_track_url = NULL;
    BOOL is_all_failed = TRUE;
    LIST_ITERATOR cur_node = LIST_BEGIN( p_bt_magnet_task->_tracker_url );
#ifdef _DEBUG
	BOOL is_enable = TRUE;
	settings_get_bool_item( "bmt.query_tracker_enable", (BOOL *)&is_enable );
	if( !is_enable) return SUCCESS;
#endif

	LOG_DEBUG("bmt_res_query_bt_track, p_bt_magnet_task:0x%X", p_bt_magnet_task);
	
    while( cur_node != LIST_END(p_bt_magnet_task->_tracker_url) )
    {
        p_track_url = (char*)LIST_VALUE( cur_node );
        ret_val = res_query_bt_tracker( p_bt_magnet_task, bmt_bt_tracker_callback, 
            p_track_url, p_bt_magnet_task->_info_hash );
        if( ret_val == SUCCESS ) is_all_failed = TRUE;
        cur_node = LIST_NEXT( cur_node );
    }
    if( is_all_failed )
#endif /* ENABLE_BT_PROTOCOL */
    {
        p_bt_magnet_task->_res_query_bt_tracker_state = RES_QUERY_END; 
    }
    return SUCCESS;
}

_int32 bmt_bt_tracker_callback( void* user_data,_int32 errcode )
{
	_int32 ret_val = SUCCESS;
	BT_MAGNET_TASK *p_task = (BT_MAGNET_TASK *)user_data;
    
	if( errcode == SUCCESS )
	{
		LOG_DEBUG("bmt_bt_tracker_callback, SUCCESS!! user_data:0x%x",user_data);
		p_task->_res_query_bt_tracker_state = RES_QUERY_SUCCESS;
		bmt_start_create_pipes( p_task );
	}
	else if( p_task->_res_query_bt_tracker_state != RES_QUERY_SUCCESS )
	{
		LOG_DEBUG("bmt_bt_tracker_callback, failed!! user_data:0x%x",user_data);
		p_task->_res_query_bt_tracker_state = RES_QUERY_FAILED;
	}

	/* Start timer */
	if( p_task->_query_bt_tracker_timer_id==INVALID_MSG_ID 
#ifdef ENABLE_BT_PROTOCOL        
	&& res_query_bt_tracker_exist(p_task) == FALSE
#endif
	)
	{
		LOG_DEBUG("start_timer(bmt_query_bt_tracker_timeout)");
		ret_val = start_timer(bmt_query_bt_tracker_timeout, NOTICE_ONCE,REQ_RESOURCE_DEFAULT_TIMEOUT, 0,(void*)p_task,&(p_task->_query_bt_tracker_timer_id));  
		if(ret_val!=SUCCESS)
        {
           p_task->_res_query_bt_tracker_state = RES_QUERY_END; 
        }
	}

	return SUCCESS;
}

_int32 bmt_query_bt_tracker_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid)
{
	BT_MAGNET_TASK *p_task = (BT_MAGNET_TASK *)(msg_info->_user_data);
    
    LOG_DEBUG("[task=0x%x] bmt_query_bt_tracker_timeout", p_task );
    
	if(errcode == MSG_TIMEOUT)
	{
        sd_assert( msgid == p_task->_query_bt_tracker_timer_id );
        bmt_res_query_bt_track( p_task );
        p_task->_query_bt_tracker_timer_id = INVALID_MSG_ID;
	}
    return SUCCESS;
}

_int32 bmt_stop_track( BT_MAGNET_TASK *p_bt_magnet_task )
{
#ifdef ENABLE_BT_PROTOCOL
    if( !res_query_bt_tracker_exist(p_bt_magnet_task) )
#endif /* ENABLE_BT_PROTOCOL */
        return SUCCESS;
	res_query_cancel(p_bt_magnet_task,BT_TRACKER);
	p_bt_magnet_task->_res_query_bt_tracker_state = RES_QUERY_END;
    
    LOG_DEBUG("[task=0x%x] bmt_stop_track", p_bt_magnet_task );

	if(p_bt_magnet_task->_query_bt_tracker_timer_id!=0)
	{
		cancel_timer(p_bt_magnet_task->_query_bt_tracker_timer_id);
		p_bt_magnet_task->_query_bt_tracker_timer_id = INVALID_MSG_ID;
	}
    return SUCCESS;
}

_int32 bmt_res_query_dht( BT_MAGNET_TASK *p_bt_magnet_task )
{
    _int32 ret_val = SUCCESS;
#ifdef _DEBUG
	BOOL is_enable = TRUE;
	settings_get_bool_item( "bmt.query_dht_enable", (BOOL *)&is_enable );
	if( !is_enable) return SUCCESS;
#endif

    LOG_DEBUG("[task=0x%x] bmt_res_query_dht", p_bt_magnet_task );
    
#ifdef _DK_QUERY
    ret_val = res_query_dht( p_bt_magnet_task, p_bt_magnet_task->_info_hash );
    if( ret_val != SUCCESS )
    {
        p_bt_magnet_task->_res_query_bt_dht_state = RES_QUERY_FAILED;
    }
    else
    {
        p_bt_magnet_task->_res_query_bt_dht_state = RES_QUERY_REQING;
    }

#endif

    return SUCCESS;
}

_int32 bmt_stop_dht( BT_MAGNET_TASK *p_bt_magnet_task )
{
    LOG_DEBUG("[task=0x%x] bmt_stop_dht", p_bt_magnet_task );

    
    if(p_bt_magnet_task->_res_query_bt_dht_state == RES_QUERY_REQING)
    {
#ifdef _DK_QUERY
        res_query_cancel(p_bt_magnet_task,BT_DHT);
#endif	
        p_bt_magnet_task->_res_query_bt_dht_state = RES_QUERY_END;
    }

    return SUCCESS;
}

_int32 bmt_add_bt_resource( void* user_data, _u32 host_ip, _u16 port )
{
	BT_MAGNET_TASK *p_task = (BT_MAGNET_TASK *)user_data;
    
    LOG_DEBUG("[task=0x%x] bmt_add_bt_resource", p_task );

    if( p_task->_connect_manager_ptr != NULL )
    {
#ifdef ENABLE_BT_PROTOCOL
        cm_add_bt_resource( p_task->_connect_manager_ptr, host_ip, port, p_task->_info_hash );
#endif /* ENABLE_BT_PROTOCOL */
    }

    return SUCCESS;
}

_int32 bmt_start_p2sp_download( BT_MAGNET_TASK *p_bt_magnet_task )
{
    _int32 ret_val = SUCCESS;
    char gougou_domain[MAX_URL_LEN] = {0}, gougou_url[MAX_URL_LEN] = {0};
    char infohash_head[3] = {0}, infohash_tail[3] = {0};
    char infohash_hex[INFO_HASH_LEN*2+1] = {0};
	TM_NEW_TASK_URL _param;
#ifdef _DEBUG
	BOOL is_enable = TRUE;
	settings_get_bool_item( "bmt.p2sp_enable", (BOOL *)&is_enable );
	if( !is_enable) return SUCCESS;
#endif

	sd_strncpy( gougou_domain, DEFAULT_RES_QUERY_MAGNET_URL, sizeof(DEFAULT_RES_QUERY_MAGNET_URL));
	settings_get_str_item("bt_magnet.torrent_host", gougou_domain );

    ret_val = char2hex( p_bt_magnet_task->_info_hash[0], infohash_head, 3 );
    CHECK_VALUE( ret_val );
    
    ret_val = char2hex( p_bt_magnet_task->_info_hash[INFO_HASH_LEN-1], infohash_tail, 3 );
    CHECK_VALUE( ret_val );

    ret_val = str2hex( (char*)p_bt_magnet_task->_info_hash, INFO_HASH_LEN, 
        infohash_hex, INFO_HASH_LEN*2+1 );
    CHECK_VALUE( ret_val );    

    sd_snprintf( gougou_url, MAX_URL_LEN, "http://%s/%s/%s/%s.torrent", 
        gougou_domain, infohash_head, infohash_tail, infohash_hex );
    LOG_DEBUG("[task=0x%x] bmt_start_p2sp_download, gougou_url:%s", 
        p_bt_magnet_task, gougou_url );

	sd_memset(&_param,0,sizeof(TM_NEW_TASK_URL));
	_param.url = gougou_url;
	_param.url_length = sd_strlen(gougou_url);
	_param.ref_url = "";
	_param.ref_url_length = 0;
	_param.description = NULL;
	_param.description_len = 0;
	_param.file_path = p_bt_magnet_task->_data_manager._file_path;
	_param.file_path_len =  sd_strlen(p_bt_magnet_task->_data_manager._file_path);
	_param.file_name = p_bt_magnet_task->_data_manager._file_name;
	_param.file_name_length = sd_strlen(p_bt_magnet_task->_data_manager._file_name);;
	_param.task_id = NULL;

    ret_val = pt_create_new_task_by_url( &_param, (TASK **)&p_bt_magnet_task->_p2sp_task_ptr );
	if (ret_val == DT_ERR_FILE_EXIST)
	{
		ret_val = pt_create_continue_task_by_url( (TM_CON_TASK_URL*)&_param, (TASK **)&p_bt_magnet_task->_p2sp_task_ptr );
	}
	if (ret_val != SUCCESS)
	{
		return ret_val;
	}

    ret_val = pt_start_task( (TASK *)p_bt_magnet_task->_p2sp_task_ptr );
    pt_set_origin_mode((TASK *)p_bt_magnet_task->_p2sp_task_ptr, 0);
    if( ret_val != SUCCESS )
    {
        pt_delete_task( (TASK *)p_bt_magnet_task->_p2sp_task_ptr );
    }

    return ret_val;
}

_int32 bmt_stop_p2sp_download( BT_MAGNET_TASK *p_bt_magnet_task )
{
    LOG_DEBUG("[task=0x%x] bmt_stop_p2sp_download", p_bt_magnet_task );

    if(p_bt_magnet_task->_p2sp_task_ptr== NULL) return SUCCESS;
    pt_stop_task( (TASK *)p_bt_magnet_task->_p2sp_task_ptr );
    pt_delete_task( (TASK *)p_bt_magnet_task->_p2sp_task_ptr );
    p_bt_magnet_task->_p2sp_task_ptr = NULL;
    return SUCCESS; 
}

_int32 bmt_start_create_pipes( BT_MAGNET_TASK *p_bt_magnet_task )
{
    _int32 ret_val = SUCCESS;
    LOG_DEBUG("[task=0x%x] bmt_start_create_pipes", p_bt_magnet_task );

    if( p_bt_magnet_task->_connect_manager_ptr != NULL )
    {
        cm_create_pipes( p_bt_magnet_task->_connect_manager_ptr );
    }
    return ret_val;
}

_int32 bmt_notify_torrent_ok( BT_MAGNET_TASK *p_bt_magnet_task, char *p_file_name )
{
    _int32 ret_val = SUCCESS;    
    LOG_DEBUG("[task=0x%x] bmt_notify_torrent_ok, file_name:%s", p_bt_magnet_task, p_file_name );
	ret_val = bmt_generate_torrent_file( p_bt_magnet_task, p_file_name );
	if( ret_val != SUCCESS ) return SUCCESS; 

    if( bmt_verify_is_torrent_ok(p_bt_magnet_task, p_file_name) )
    {
        p_bt_magnet_task->_task_status = MAGNET_TASK_SUCCESS;
    }
	sd_delete_file(p_file_name);
    return ret_val;
}

BOOL bmt_verify_is_torrent_ok( BT_MAGNET_TASK *p_bt_magnet_task, char *p_file_name  )
{
    _int32 ret_val = SUCCESS;
    char file_full_path[MAX_FULL_PATH_LEN] = {0};
    BOOL is_ok = FALSE;
    TORRENT_SEED_INFO *p_seed_info = NULL;

    TASK *et_task = NULL;            

    LOG_DEBUG("[task=0x%x] bmt_verify_is_torrent_ok, file_name:%s", p_bt_magnet_task, p_file_name );

    sd_strncpy(file_full_path, p_bt_magnet_task->_data_manager._file_path, MAX_FULL_PATH_LEN);
	sd_strcat(file_full_path, p_file_name, MAX_FULL_PATH_LEN - sd_strlen(file_full_path));

	ret_val = tp_get_seed_info( file_full_path, DEFAULT_SWITCH, &p_seed_info );
    if( ret_val != SUCCESS )
    {
        is_ok = FALSE;
        return is_ok;
    }
        
    is_ok = sd_is_cid_equal( p_seed_info->_info_hash, p_bt_magnet_task->_info_hash );
    tp_release_seed_info(p_seed_info);
    return is_ok;
}

_int32 bmt_generate_torrent_file( BT_MAGNET_TASK *p_bt_magnet_task, char *p_file_name )
{
    _int32 ret_val = SUCCESS;
    BC_DICT *p_dict = NULL;
    char *p_announce_key = "announce";
    BC_STR *p_bc_announce = NULL;
    char new_torrent_full_path[ MAX_FULL_PATH_LEN ] = { 0 };
    char old_torrent_full_path[ MAX_FULL_PATH_LEN ] = { 0 };
    LIST_ITERATOR cur_node = NULL;
    char *p_tracker_url = NULL;
    _u32 torrent_len = 0;
    _u32 new_file_id = INVALID_FILE_ID, old_file_id = INVALID_FILE_ID;
    _int32 flag = O_FS_CREATE;
    _u32 write_size = 0, read_size = 0, read_old_file_size = 0;
    char *p_buffer = NULL;
    _int32 buffer_len = 16 * 1024;
    _u64 torrent_file_pos = 0, old_file_size = 0;
    
    LOG_DEBUG("[task=0x%x] bmt_generate_torrent_file, file_name:%s", p_bt_magnet_task, p_file_name );

    sd_strncpy( new_torrent_full_path, p_bt_magnet_task->_data_manager._file_path, MAX_FULL_PATH_LEN );
    sd_strcat( new_torrent_full_path, p_bt_magnet_task->_data_manager._file_name, MAX_FULL_PATH_LEN - sd_strlen(new_torrent_full_path));

    sd_strcat( old_torrent_full_path, p_file_name, MAX_FULL_PATH_LEN - sd_strlen(old_torrent_full_path));

    if( sd_file_exist( new_torrent_full_path ) )
    {
        //sd_assert( FALSE );
        sd_memcpy(p_file_name, p_bt_magnet_task->_data_manager._file_name, 
        		sd_strlen(p_bt_magnet_task->_data_manager._file_name) + 1);
        return SUCCESS;
    }

    if( list_size(&p_bt_magnet_task->_tracker_url) == 0 )
    {
        ret_val = sd_rename_file( old_torrent_full_path, new_torrent_full_path );
        CHECK_VALUE( ret_val ); 
    }

    ret_val = sd_malloc( buffer_len, (void**)&p_buffer );
    CHECK_VALUE( ret_val ); 

	sd_memset(p_buffer, 0, buffer_len);

    cur_node = LIST_BEGIN( p_bt_magnet_task->_tracker_url );
    while( cur_node != LIST_END( p_bt_magnet_task->_tracker_url ) )
    {
        p_tracker_url = (char *)LIST_VALUE( cur_node );
        sd_strcat( p_buffer, p_tracker_url, buffer_len - sd_strlen(p_buffer) );
        p_buffer[ sd_strlen(p_buffer) ] = ';';
        cur_node = LIST_NEXT( cur_node );
    }
	
    ret_val = bc_str_create_with_value( p_buffer, sd_strlen(p_buffer), &p_bc_announce );
    if( ret_val != SUCCESS )
    {
        bc_str_uninit( (BC_OBJ*)p_bc_announce );
        goto ErrHandler;
    }
    
    ret_val = bc_dict_create( &p_dict );
    if( ret_val != SUCCESS )
    {
        bc_str_uninit( (BC_OBJ*)p_bc_announce );
        goto ErrHandler;
    }

    ret_val = bc_dict_set_value( p_dict, p_announce_key, (BC_OBJ*)p_bc_announce );
    if( ret_val != SUCCESS )
    {
        bc_str_uninit( (BC_OBJ*)p_bc_announce );
        bc_dict_uninit( (BC_OBJ*)p_dict );
        goto ErrHandler;
    }

    sd_memset( p_buffer, 0, buffer_len );

    ret_val = bc_dict_to_str( (BC_OBJ*)p_dict, p_buffer, buffer_len - 7, &torrent_len );
    if( ret_val != SUCCESS )
    {
        bc_dict_uninit( (BC_OBJ*)p_dict );
        goto ErrHandler;
    }

    bc_dict_uninit( (BC_OBJ*)p_dict );

	p_buffer[torrent_len - 1] = 0;
	sd_strcat(p_buffer, "4:info", 7);
	torrent_len += 6;

    ret_val = sd_open_ex( new_torrent_full_path, flag, &new_file_id );
    if( ret_val != SUCCESS )
    {
        goto ErrHandler;
    }

    ret_val = sd_pwrite( new_file_id, p_buffer, torrent_len, torrent_file_pos, &write_size );
    if( ret_val != SUCCESS )
    {
        goto ErrHandler1;
    }

    sd_assert( torrent_len == write_size );
    torrent_file_pos += write_size - 1;

    ret_val = sd_open_ex( old_torrent_full_path, O_FS_RDONLY, &old_file_id );
	if( ret_val != SUCCESS )
    {
        goto ErrHandler1;
    }
	ret_val = sd_filesize( old_file_id, &old_file_size );
	if( ret_val != SUCCESS )
    {
        goto ErrHandler1;
    } 
	
    do
    {
        ret_val = sd_read( old_file_id, p_buffer, buffer_len, &read_size );
        if( ret_val != SUCCESS )
        {
          goto ErrHandler2;
        }

        ret_val = sd_pwrite( new_file_id, p_buffer, read_size, torrent_file_pos, &write_size );
        if( ret_val != SUCCESS )
        {
          goto ErrHandler2;
        }     
        sd_assert( read_size == write_size );
        torrent_file_pos += read_size;
        read_old_file_size += read_size;
        
    } while( read_old_file_size < old_file_size );

    p_buffer[0] = 'e';
    ret_val = sd_pwrite( new_file_id, p_buffer, 1, torrent_file_pos, &write_size );
    sd_assert( 1 == write_size );

	sd_memcpy(p_file_name, p_bt_magnet_task->_data_manager._file_name, 
			sd_strlen(p_bt_magnet_task->_data_manager._file_name) + 1);
	sd_delete_file(old_torrent_full_path);
    
ErrHandler2:
    sd_close_ex( old_file_id );
ErrHandler1:
    sd_close_ex( new_file_id );
ErrHandler:
    SAFE_DELETE( p_buffer );
    return ret_val;
}

#endif /*  ENABLE_BT  */

