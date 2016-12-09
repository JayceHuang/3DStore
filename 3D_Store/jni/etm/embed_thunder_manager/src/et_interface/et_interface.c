
#include "et_interface/et_interface.h"

#include "scheduler/scheduler.h"

//#include "common/time.h"

_int32 iet_init(ETI_PROXY_INFO* proxy_for_hub)
{
#ifdef ET_ENABLE
return et_init(proxy_for_hub);
#else
return -1;
#endif
}

_int32 iet_uninit(void)
{
#ifdef ET_ENABLE
return et_uninit();
#else
return -1;
#endif
}


_int32 iet_set_license(char *license, int32 license_size)
{
#ifdef ET_ENABLE
return et_set_license(license,  license_size);
#else
return -1;
#endif
}
_int32 iet_set_license_callback( ETI_NOTIFY_LICENSE_RESULT license_callback_ptr)
{
#ifdef ET_ENABLE
return et_set_license_callback(  license_callback_ptr);
#else
return -1;
#endif
}

const char*  iet_get_version(void)
{
#ifdef ET_ENABLE
return et_get_version();
#else
return "1.3.3.2";
#endif	
}

_int32 iet_create_new_task_by_url(char* url, u32 url_length, 
							  char* ref_url, u32 ref_url_length,
							  char* description, u32 description_len,
							  char* file_path, u32 file_path_len,
							  char* file_name, u32 file_name_length, 
							  u32* task_id)
{
#ifdef ET_ENABLE
	_int32 ret_val = SUCCESS;
	if(FALSE==em_is_et_running())
	{
		ret_val = em_start_et(  );
		CHECK_VALUE(ret_val);
	}
	
	return et_create_new_task_by_url(url,  url_length, 
							  ref_url,  ref_url_length,
							  description, description_len,
							   file_path,  file_path_len,
							  file_name,  file_name_length, 
							  task_id);

#else
return -1;
#endif	
}

_int32 iet_try_create_new_task_by_url(char* url, u32 url_length, 
							  char* ref_url, u32 ref_url_length,
							  char* description, u32 description_len,
							  char* file_path, u32 file_path_len,
							  char* file_name, u32 file_name_length, 
							  u32* task_id)
{
#ifdef ET_ENABLE
	_int32 ret_val = SUCCESS;
	if(FALSE==em_is_et_running())
	{
		ret_val = em_start_et(  );
		CHECK_VALUE(ret_val);
	}
	
	return et_try_create_new_task_by_url(url,  url_length, 
							  ref_url,  ref_url_length,
							  description, description_len,
							   file_path,  file_path_len,
							  file_name,  file_name_length, 
							  task_id);

#else
return -1;
#endif	
}


_int32 iet_create_continue_task_by_url(char* url, u32 url_length, 
								   char* ref_url, u32 ref_url_length,
								   char* description, u32 description_len,
								   char* file_path, u32 file_path_len,
								   char* file_name, u32 file_name_length,
								   u32* task_id)
{
#ifdef ET_ENABLE
	_int32 ret_val = SUCCESS;
	if(FALSE==em_is_et_running())
	{
		ret_val = em_start_et(  );
		CHECK_VALUE(ret_val);
	}
	
	return et_create_continue_task_by_url(url,  url_length, 
								   ref_url,  ref_url_length,
								   description,  description_len,
								   file_path,  file_path_len,
								   file_name,  file_name_length,
								   task_id);
#else
return -1;
#endif	
}



_int32 iet_create_new_task_by_tcid(u8 *tcid, uint64 file_size, char *file_name, u32 file_name_length, char *file_path, u32 file_path_len, u32* task_id )
{
#ifdef ET_ENABLE
	_int32 ret_val = SUCCESS;
	if(FALSE==em_is_et_running())
	{
		ret_val = em_start_et(  );
		CHECK_VALUE(ret_val);
	}
	
	return et_create_new_task_by_tcid(tcid,  file_size, file_name,  file_name_length, file_path,  file_path_len,  task_id );
#else
return -1;
#endif	
}

_int32 iet_create_continue_task_by_tcid(u8 *tcid, char*file_name, u32 file_name_length, char*file_path, u32 file_path_len, u32* task_id )
{
#ifdef ET_ENABLE
	_int32 ret_val = SUCCESS;
	if(FALSE==em_is_et_running())
	{
		ret_val = em_start_et(  );
		CHECK_VALUE(ret_val);
	}
	
	return et_create_continue_task_by_tcid(tcid, file_name,  file_name_length, file_path,  file_path_len,  task_id );
#else
return -1;
#endif	
}
_int32 iet_create_task_by_tcid_file_size_gcid(u8 *tcid, uint64 file_size, u8 *gcid,char *file_name, u32 file_name_length, char *file_path, u32 file_path_len, u32* task_id )
{
#ifdef ET_ENABLE
	_int32 ret_val = SUCCESS;
	if(FALSE==em_is_et_running())
	{
		ret_val = em_start_et(  );
		CHECK_VALUE(ret_val);
	}
	
	return et_create_task_by_tcid_file_size_gcid(tcid,  file_size, gcid,file_name,file_name_length, file_path,  file_path_len,  task_id );
#else
return -1;
#endif	
}
_int32 iet_create_bt_task(char* seed_file_full_path, u32 seed_file_full_path_len, 
								char* file_path, u32 file_path_len,
								u32* download_file_index_array, u32 file_num,
								enum ETI_ENCODING_SWITCH_MODE encoding_switch_mode, u32* task_id)
{
#ifdef ET_ENABLE
	_int32 ret_val = SUCCESS;
	if(FALSE==em_is_et_running())
	{
		ret_val = em_start_et(  );
		CHECK_VALUE(ret_val);
	}
	
	return et_create_bt_task(seed_file_full_path,  seed_file_full_path_len, 
								 file_path,  file_path_len,
								 download_file_index_array,  file_num,
								 encoding_switch_mode, task_id);
#else
return -1;
#endif	
}
_int32 iet_create_emule_task(const char* ed2k_link, u32 ed2k_link_len, char* path, u32 path_len, char* file_name, u32 file_name_length, u32* task_id)
{
#ifdef ET_ENABLE
	_int32 ret_val = SUCCESS;
	if(FALSE==em_is_et_running())
	{
		ret_val = em_start_et(  );
		CHECK_VALUE(ret_val);
	}
	
	return et_create_emule_task(ed2k_link,  ed2k_link_len, path,  path_len, file_name,file_name_length, task_id);
#else
return -1;
#endif	
}
_int32 iet_create_bt_magnet_task(char * url, u32 url_len, char * file_path, u32 file_path_len, char * file_name,u32 file_name_len,BOOL bManual, enum ET_ENCODING_SWITCH_MODE encoding_switch_mode, u32 * p_et_taskid)
{
#ifdef ET_ENABLE
	_int32 ret_val = SUCCESS;
	if(FALSE==em_is_et_running())
	{
		ret_val = em_start_et(  );
		CHECK_VALUE(ret_val);
	}
	
	return et_create_bt_magnet_task(url, url_len, file_path, file_path_len, file_name,file_name_len,bManual,encoding_switch_mode, p_et_taskid);
#else
return -1;
#endif
}


_int32 iet_set_task_no_disk(u32 task_id)
{
#ifdef ET_ENABLE
	return et_set_task_no_disk( task_id);
#else
return -1;
#endif	
}

_int32 iet_vod_set_task_check_data(u32 task_id)
{
#ifdef ET_ENABLE
	return et_vod_set_task_check_data( task_id);
#else
return -1;
#endif	
}
_int32 iet_start_task(u32 task_id)
{
#ifdef ET_ENABLE
	return et_start_task( task_id);
#else
return -1;
#endif	
}


_int32 iet_stop_task(u32 task_id)
{
#ifdef ET_ENABLE
	return et_stop_task( task_id);
#else
return -1;
#endif	
}

_int32 iet_delete_task(u32 task_id)
{
#ifdef ET_ENABLE
	return et_delete_task( task_id);
#else
return -1;
#endif	
}


_int32 iet_get_task_info(u32 task_id, ETI_TASK *info)
{
#ifdef ET_ENABLE
	return  et_get_task_info( task_id, info);
#else
return -1;
#endif	
}

_int32 iet_get_task_file_name(u32 task_id, char *file_name_buffer, _int32* buffer_size)
{
#ifdef ET_ENABLE
	return et_get_task_file_name( task_id, file_name_buffer,  buffer_size);
#else
return -1;
#endif	
}

_int32 iet_get_task_tcid(u32 task_id, _u8 *tcid)
{
#ifdef ET_ENABLE
	return et_get_task_tcid( task_id, tcid);
#else
return -1;
#endif	
}



_int32 iet_get_torrent_seed_info( char *seed_path, ETI_TORRENT_SEED_INFO **pp_seed_info )
{
#ifdef ET_ENABLE
	_int32 ret_val = SUCCESS;
	if(FALSE==em_is_et_running())
	{
		ret_val = em_start_et(  );
		if(ret_val!=SUCCESS) return ret_val;
	}
	
	ret_val = et_get_torrent_seed_info( seed_path, pp_seed_info );
	return ret_val;
#else
return -1;
#endif	
}

_int32 iet_get_torrent_seed_info_with_encoding_mode( char *seed_path, enum ETI_ENCODING_SWITCH_MODE encoding_switch_mode, ETI_TORRENT_SEED_INFO **pp_seed_info )
{
#ifdef ET_ENABLE
	_int32 ret_val = SUCCESS;
	if(FALSE==em_is_et_running())
	{
		ret_val = em_start_et(  );
		CHECK_VALUE(ret_val);
	}
	
	ret_val = et_get_torrent_seed_info_with_encoding_mode( seed_path,  encoding_switch_mode, pp_seed_info );
	return ret_val;
#else
return -1;
#endif	
}


_int32 iet_release_torrent_seed_info( ETI_TORRENT_SEED_INFO *p_seed_info )
{
#ifdef ET_ENABLE
	return et_release_torrent_seed_info( p_seed_info );
#else
return -1;
#endif	
}


_int32 iet_set_seed_switch_type( enum ETI_ENCODING_SWITCH_MODE switch_type )
{
#ifdef ET_ENABLE
	return et_set_seed_switch_type(  switch_type );
#else
return -1;
#endif	
}


_int32 iet_get_bt_download_file_index( u32 task_id, u32 *buffer_len, u32 *file_index_buffer )
{
#ifdef ET_ENABLE
	return et_get_bt_download_file_index(  task_id, buffer_len, file_index_buffer );
#else
return -1;
#endif	
}

_int32 iet_get_bt_file_info(u32 task_id, u32 file_index, ETI_BT_FILE *info)
{
#ifdef ET_ENABLE
	return et_get_bt_file_info( task_id, file_index, info);
#else
return -1;
#endif	
}
_int32 iet_extract_ed2k_url(char* ed2k, ET_ED2K_LINK_INFO* info)
{
#ifdef ENABLE_EMULE
	_int32 ret_val = SUCCESS;
	if(FALSE==em_is_et_running())
	{
		ret_val = em_start_et(  );
		CHECK_VALUE(ret_val);
	}
	
	return et_extract_ed2k_url(ed2k,  info);

#else /* ENABLE_EMULE */
return -1;
#endif	 /* ENABLE_EMULE */
}

_int32 iet_remove_tmp_file(char* file_path, char* file_name)
{
#ifdef ET_ENABLE
	return et_remove_tmp_file( file_path, file_name);
#else
return -1;
#endif	
}

_int32 iet_set_customed_interface(_int32 fun_idx, void *fun_ptr)
{
#ifdef ET_ENABLE
	return et_set_customed_interface( fun_idx, fun_ptr);
#else
return -1;
#endif	
}

_int32 iet_set_download_record_file_path(const char *path,_u32  path_len)
{
#ifdef ET_ENABLE
	return et_set_download_record_file_path(path,  path_len);
#else
return -1;
#endif	
}

_int32 iet_set_max_tasks(_u32 task_num)
{
#ifdef ET_ENABLE
	return et_set_max_tasks( task_num);
#else
return -1;
#endif	
}


_u32 iet_get_max_tasks(void)
{
#ifdef ET_ENABLE
	return et_get_max_tasks();
#else
return -1;
#endif	
}

_int32 iet_set_limit_speed(_u32 download_limit_speed, _u32 upload_limit_speed)
{
#ifdef ET_ENABLE
	return et_set_limit_speed( download_limit_speed,  upload_limit_speed);
#else
return -1;
#endif	
}


_int32 iet_get_limit_speed(_u32* download_limit_speed, _u32* upload_limit_speed)
{
#ifdef ET_ENABLE
	return et_get_limit_speed( download_limit_speed,  upload_limit_speed);
#else
return -1;
#endif	
}

_int32 iet_set_max_task_connection(_u32 connection_num)
{
#ifdef ET_ENABLE
	return et_set_max_task_connection( connection_num);
#else
return -1;
#endif	
}

_u32 iet_get_max_task_connection(void)
{
#ifdef ET_ENABLE
	return et_get_max_task_connection();
#else
return -1;
#endif	
}

_u32 iet_get_current_download_speed(void)
{
#ifdef ET_ENABLE
	return et_get_current_download_speed();
#else
return -1;
#endif	
}

_u32 iet_get_current_upload_speed(void)
{
#ifdef ET_ENABLE
	return et_get_current_upload_speed();
#else
return -1;
#endif	
}

_int32 iet_get_all_task_id(  _u32 *buffer_len, _u32 *task_id_buffer )
{
#ifdef ET_ENABLE
	return et_get_all_task_id( buffer_len, task_id_buffer );
#else
return -1;
#endif	
}

_int32 iet_remove_task_tmp_file(_u32 task_id)
{
#ifdef ET_ENABLE
	return et_remove_task_tmp_file( task_id);
#else
return -1;
#endif	
}

_int32 iet_get_bt_file_path_and_name(_u32 task_id, _u32 file_index,char *file_path_buffer, _int32 *file_path_buffer_size, char *file_name_buffer, _int32* file_name_buffer_size)
{
#ifdef ET_ENABLE
	return et_get_bt_file_path_and_name( task_id,  file_index,file_path_buffer, file_path_buffer_size, file_name_buffer,  file_name_buffer_size);
#else
return -1;
#endif	
}

_int32 iet_vod_read_file(_int32 task_id, _u64 start_pos, _u64 len, char *buf, _int32 block_time )
{
#ifdef ET_ENABLE
	return et_vod_read_file( task_id,  start_pos,  len, buf,  block_time );
#else
return -1;
#endif	
}

_int32 iet_vod_bt_read_file(_int32 task_id, _u32 file_index, _u64 start_pos, _u64 len, char *buf, _int32 block_time )
{
#ifdef ET_ENABLE
	return et_vod_bt_read_file( task_id,  file_index,  start_pos,  len, buf,  block_time );
#else
return -1;
#endif	
}

_int32 iet_vod_set_buffer_time(_int32 buffer_time )
{
#ifdef ET_ENABLE
	return et_vod_set_buffer_time( buffer_time );
#else
return -1;
#endif	
}

_int32 iet_vod_get_buffer_percent(_int32 task_id,  int32* percent )
{
#ifdef ET_ENABLE
	return et_vod_get_buffer_percent( task_id,   percent );
#else
return -1;
#endif	
}

_int32 iet_vod_get_download_position(_int32 task_id,  _u64* position )
{
#ifdef ET_ENABLE
	return et_vod_get_download_position( task_id,   position );
#else
return -1;
#endif	
}

_int32 iet_set_customed_interface_mem(_int32 fun_idx, void *fun_ptr)
{
#ifdef ET_ENABLE
	return et_set_customed_interface_mem( fun_idx, fun_ptr);
#else
return -1;
#endif	
}


_int32 iet_vod_set_vod_buffer_size(int32 buffer_size)
{
#ifdef ET_ENABLE
	return et_vod_set_vod_buffer_size( buffer_size);
#else
return -1;
#endif	
}


_int32 iet_vod_get_vod_buffer_size(int32* buffer_size)
{
#ifdef ET_ENABLE
	return et_vod_get_vod_buffer_size(buffer_size);
#else
return -1;
#endif	
}

_int32 iet_vod_is_vod_buffer_allocated(int32* allocated)
{
#ifdef ET_ENABLE
	return et_vod_is_vod_buffer_allocated( allocated);
#else
return -1;
#endif	
}


_int32 iet_vod_free_vod_buffer(void)
{
#ifdef ET_ENABLE
	return et_vod_free_vod_buffer();
#else
return -1;
#endif	
}

_int32 iet_vod_get_bitrate(int32 task_id, u32 file_index, u32* bitrate)
{
#ifdef ET_ENABLE
	return et_vod_get_bitrate(task_id, file_index, bitrate);
#else
return -1;
#endif	
}

_int32 iet_vod_is_download_finished(int32 task_id, BOOL* finished )
{
#ifdef ET_ENABLE
	return et_vod_is_download_finished(task_id,finished);
#else
return -1;
#endif	
}
_int32 iet_reporter_mobile_user_action_to_file(u32 action_type, u32 action_value, void* data, u32 data_len)
{
#ifdef ET_ENABLE
	_int32 ret_val = SUCCESS;
	if(FALSE==em_is_et_running())
	{
		ret_val = em_start_et(  );
		CHECK_VALUE(ret_val);
	}
	return et_reporter_mobile_user_action_to_file( action_type,  action_value,  data,  data_len);
#else
return -1;
#endif	
}
_int32 iet_http_get(ET_HTTP_PARAM * param,u32 * http_id)
{
#ifdef ET_ENABLE
	_int32 ret_val = SUCCESS;
	if(FALSE==em_is_et_running())
	{
		ret_val = em_start_et(  );
		CHECK_VALUE(ret_val);
	}
	return et_http_get( param, http_id);
#else
return -1;
#endif	
}

_int32 iet_http_post(ET_HTTP_PARAM * param,u32 * http_id)
{
#ifdef ET_ENABLE
	_int32 ret_val = SUCCESS;
	if(FALSE==em_is_et_running())
	{
		ret_val = em_start_et(  );
		CHECK_VALUE(ret_val);
	}
	return et_http_post( param, http_id);
#else
return -1;
#endif	
}
_int32 iet_http_close(u32 http_id)
{
#ifdef ET_ENABLE
	return et_http_close( http_id);
#else
return -1;
#endif	
}

_int32 iet_create_cmd_proxy( u32 ip, u16 port, u32 attribute, u32 *p_cmd_proxy_id )
{
#ifdef ET_ENABLE
	return et_create_cmd_proxy(ip, port, attribute, p_cmd_proxy_id );
#else
return -1;
#endif	
}

_int32 iet_create_cmd_proxy_by_domain( const char* host, u16 port, u32 attribute, u32 *p_cmd_proxy_id )
{
#ifdef ET_ENABLE
	return et_create_cmd_proxy_by_domain(host, port, attribute, p_cmd_proxy_id );
#else
return -1;
#endif	
}

_int32 iet_cmd_proxy_get_info( u32 cmd_proxy_id, int32 *p_errcode, u32 *p_recv_cmd_id, u32 *p_data_len )
{
#ifdef ET_ENABLE
	return et_cmd_proxy_get_info(cmd_proxy_id, p_errcode, p_recv_cmd_id, p_data_len);
#else
return -1;
#endif	
}

_int32 iet_cmd_proxy_send( u32 cmd_proxy_id, const char* buffer, u32 len )
{
#ifdef ET_ENABLE
	return et_cmd_proxy_send(cmd_proxy_id, buffer, len);
#else
return -1;
#endif	
}

_int32 iet_cmd_proxy_get_recv_info( u32 cmd_proxy_id, u32 recv_cmd_id, char *data_buffer, u32 data_buffer_len )
{
#ifdef ET_ENABLE
	return et_cmd_proxy_get_recv_info(cmd_proxy_id, recv_cmd_id, data_buffer, data_buffer_len);
#else
return -1;
#endif	
}

_int32 iet_cmd_proxy_close( u32 cmd_proxy_id )
{
#ifdef ET_ENABLE
	return et_cmd_proxy_close(cmd_proxy_id);
#else
return -1;
#endif	
}

_int32 iet_get_peerid( char *buffer, u32 buffer_size )
{
#ifdef ET_ENABLE

    _int32 ret_val = SUCCESS;
    if(FALSE==em_is_et_running())
    {
        ret_val = em_start_et(  );
        CHECK_VALUE(ret_val);
    }

    return et_get_peerid( buffer, buffer_size );
#else
return -1;
#endif	
}

_int32  iet_get_free_disk(const char *path, _u64 *free_size)
{
#ifdef ET_ENABLE
return et_get_free_disk(path, free_size);
#else
return 0;
#endif	
}

_int32  iet_is_drm_enable( BOOL *p_is_enable )
{
#ifdef ET_ENABLE
return et_is_drm_enable(p_is_enable);
#else
return 0;
#endif	
}

_int32  iet_set_certificate_path( const char *certificate_path )
{
#ifdef ET_ENABLE
return et_set_certificate_path(certificate_path);
#else
return 0;
#endif	
}


_int32  iet_open_drm_file( const char *p_file_full_path, u32 *p_drm_id, uint64 *p_origin_file_size  )
{
#ifdef ET_ENABLE
return et_open_drm_file(p_file_full_path, p_drm_id, p_origin_file_size );
#else
return 0;
#endif	
}

_int32  iet_is_certificate_ok( u32 drm_id, BOOL *p_is_ok )
{
#ifdef ET_ENABLE
return et_is_certificate_ok(drm_id, p_is_ok);
#else
return 0;
#endif	
}

_int32  iet_read_drm_file( u32 drm_id, char *p_buffer, u32 size, 
    uint64 file_pos, u32 *p_read_size )
{
#ifdef ET_ENABLE
return et_read_drm_file(drm_id, p_buffer, size, file_pos, p_read_size);
#else
return 0;
#endif	
}

_int32  iet_close_drm_file( u32 drm_id )
{
#ifdef ET_ENABLE
return et_close_drm_file(drm_id);
#else
return 0;
#endif	
}

_int32  iet_set_openssl_rsa_interface( _int32 fun_count, void *fun_ptr_table )
{
#ifdef ET_ENABLE
return et_set_openssl_rsa_interface(fun_count, fun_ptr_table);
#else
return 0;
#endif	
}

_int32 iet_start_http_server(_u16 * port)
{
#if defined( ENABLE_VOD)||defined(ENABLE_ASSISTANT)||defined(ENABLE_HTTP_SERVER)
	_int32 ret_val = SUCCESS;
	if(FALSE==em_is_et_running())
	{
		ret_val = em_start_et(  );
		CHECK_VALUE(ret_val);
	}
	return et_start_http_server(  port);
#else
return -1;
#endif	
}

_int32 iet_start_search_server(ET_SEARCH_SERVER * p_search)
{
	_int32 ret_val = SUCCESS;
	if(FALSE==em_is_et_running())
	{
		ret_val = em_start_et(  );
		CHECK_VALUE(ret_val);
	}
	return et_start_search_server(p_search);
}

_int32 iet_assistant_set_callback_func(ET_ASSISTANT_INCOM_REQ_CALLBACK req_callback,ET_ASSISTANT_SEND_RESP_CALLBACK resp_callback)
{
#if defined(ENABLE_ASSISTANT)
	_int32 ret_val = SUCCESS;
	if(FALSE==em_is_et_running())
	{
		ret_val = em_start_et(  );
		CHECK_VALUE(ret_val);
	}
	return et_assistant_set_callback_func(  req_callback,resp_callback);
#else
return -1;
#endif	
}

_int32 iet_set_system_path(const char * system_path)
{
	_int32 ret_val = SUCCESS;
	if(FALSE==em_is_et_running())
	{
		ret_val = em_start_et(  );
		CHECK_VALUE(ret_val);
	}
	return et_set_system_path(  system_path);
}

_int32 iet_set_host_ip(const char * host_name, const char *ip)
{
	_int32 ret_val = SUCCESS;
	if(FALSE==em_is_et_running())
	{
		ret_val = em_start_et(  );
		CHECK_VALUE(ret_val);
	}
	return et_set_host_ip(host_name, ip);
}

_int32 iet_clear_host_ip(void)
{
	_int32 ret_val = SUCCESS;
	if(FALSE==em_is_et_running())
	{
		ret_val = em_start_et(  );
		CHECK_VALUE(ret_val);
	}
	return et_clear_host_ip();
}

_int32 iet_query_shub_by_url(ET_QUERY_SHUB * p_param,u32 * p_action_id)
{
	_int32 ret_val = SUCCESS;
	if(FALSE==em_is_et_running())
	{
		ret_val = em_start_et(  );
		CHECK_VALUE(ret_val);
	}
	return et_query_shub_by_url(p_param, p_action_id);
}

_int32 iet_get_info_from_cfg_file(char* cfg_path, char* url, BOOL* cid_is_valid, u8* cid, uint64* file_size, char* file_name, char *lixian_url, char *cookie, char *user_data)
{
	_int32 ret_val = SUCCESS;
	if(FALSE==em_is_et_running())
	{
		ret_val = em_start_et(  );
		CHECK_VALUE(ret_val);
	}
	return et_get_info_from_cfg_file(cfg_path, url, cid_is_valid, cid, file_size, file_name, lixian_url, cookie, user_data);
}

