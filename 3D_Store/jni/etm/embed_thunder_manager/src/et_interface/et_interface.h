#if !defined(__ET_INTERFACE_H_20090924)
#define __ET_INTERFACE_H_20090924

#include "em_common/em_define.h"
#define ET_ENABLE  
#include "embed_thunder.h"

#define ETI_TASK_STATUS 					ET_TASK_STATUS
#define ETI_TASK_FILE_CREATE_STATUS 		ET_TASK_FILE_CREATE_STATUS
#define ETI_ENCODING_SWITCH_MODE 		ET_ENCODING_SWITCH_MODE

typedef ET_TASK								ETI_TASK;
typedef ET_BT_FILE							ETI_BT_FILE;
typedef ET_PROXY_INFO						ETI_PROXY_INFO;
typedef ET_TORRENT_FILE_INFO					ETI_TORRENT_FILE_INFO;
typedef ET_TORRENT_SEED_INFO				ETI_TORRENT_SEED_INFO;

#define ETI_MAX_TD_CFG_SUFFIX_LEN 		ET_MAX_TD_CFG_SUFFIX_LEN

typedef int32 ( *ETI_NOTIFY_LICENSE_RESULT)(u32 result, u32 expire_time);


#define eti_init 											iet_init
#define eti_uninit 										iet_uninit
#define eti_set_license 									iet_set_license
#define eti_set_license_callback 							iet_set_license_callback
#define eti_get_version 									iet_get_version
#define eti_create_new_task_by_url 						iet_create_new_task_by_url
#define eti_try_create_new_task_by_url 					iet_try_create_new_task_by_url
#define eti_create_continue_task_by_url 					iet_create_continue_task_by_url
#define eti_create_new_task_by_tcid 						iet_create_new_task_by_tcid
#define eti_create_continue_task_by_tcid 					iet_create_continue_task_by_tcid
#define eti_create_task_by_tcid_file_size_gcid 			iet_create_task_by_tcid_file_size_gcid
#define eti_create_bt_task 								iet_create_bt_task
#define eti_create_emule_task 							iet_create_emule_task
#define eti_create_bt_magnet_task						iet_create_bt_magnet_task
#define eti_set_task_no_disk 								iet_set_task_no_disk
#define eti_vod_set_task_check_data 						iet_vod_set_task_check_data
#define eti_start_task 									iet_start_task
#define eti_stop_task 									iet_stop_task
#define eti_delete_task 									iet_delete_task
#define eti_get_task_info 									iet_get_task_info
#define eti_get_task_file_name 							iet_get_task_file_name
#define eti_get_task_tcid 									iet_get_task_tcid
#define eti_add_task_resource 							et_add_task_resource
#define eti_get_torrent_seed_info 						iet_get_torrent_seed_info
#define eti_get_torrent_seed_info_with_encoding_mode 	iet_get_torrent_seed_info_with_encoding_mode
#define eti_release_torrent_seed_info 					iet_release_torrent_seed_info
#define eti_set_seed_switch_type 							iet_set_seed_switch_type
#define eti_get_bt_download_file_index 					iet_get_bt_download_file_index
#define eti_get_bt_file_info 								iet_get_bt_file_info
#define eti_extract_ed2k_url 								iet_extract_ed2k_url

#define eti_remove_tmp_file 								iet_remove_tmp_file

#define eti_set_customed_interface 								iet_set_customed_interface
#define eti_set_download_record_file_path 								iet_set_download_record_file_path
#define eti_set_max_tasks 								iet_set_max_tasks
#define eti_get_max_tasks 								iet_get_max_tasks
#define eti_set_limit_speed 								iet_set_limit_speed
#define eti_get_limit_speed 								iet_get_limit_speed
#define eti_set_max_task_connection 								iet_set_max_task_connection
#define eti_get_max_task_connection 								iet_get_max_task_connection
#define eti_get_current_download_speed 								iet_get_current_download_speed
#define eti_get_current_upload_speed 								iet_get_current_upload_speed
#define eti_get_all_task_id 								iet_get_all_task_id
#define eti_remove_task_tmp_file 								iet_remove_task_tmp_file
#define eti_get_bt_file_path_and_name 								iet_get_bt_file_path_and_name
#define eti_vod_read_file 								iet_vod_read_file
#define eti_vod_bt_read_file 								iet_vod_bt_read_file
#define eti_vod_set_buffer_time 								iet_vod_set_buffer_time
#define eti_vod_get_buffer_percent 								iet_vod_get_buffer_percent
#define eti_vod_get_download_position 								iet_vod_get_download_position
#define eti_set_customed_interface_mem 								iet_set_customed_interface_mem

#define eti_vod_set_vod_buffer_size 								iet_vod_set_vod_buffer_size
#define eti_vod_get_vod_buffer_size 								iet_vod_get_vod_buffer_size
#define eti_vod_is_vod_buffer_allocated 								iet_vod_is_vod_buffer_allocated
#define eti_vod_free_vod_buffer 								iet_vod_free_vod_buffer
#define eti_vod_is_download_finished 								iet_vod_is_download_finished
#define eti_vod_get_bitrate										iet_vod_get_bitrate

#define eti_reporter_mobile_user_action_to_file 								iet_reporter_mobile_user_action_to_file

//ip port 均为主机字节序

#define eti_create_cmd_proxy 								iet_create_cmd_proxy
#define eti_create_cmd_proxy_by_domain 						iet_create_cmd_proxy_by_domain
#define eti_cmd_proxy_get_info 						iet_cmd_proxy_get_info

#define eti_cmd_proxy_send 								    iet_cmd_proxy_send
#define eti_cmd_proxy_get_recv_info 						iet_cmd_proxy_get_recv_info
#define eti_cmd_proxy_close 								iet_cmd_proxy_close
#define eti_get_peerid 									    iet_get_peerid
#define eti_get_free_disk 									iet_get_free_disk



#define eti_http_get 												iet_http_get
#define eti_http_post												iet_http_post
#define eti_http_close 												iet_http_close

#define eti_is_drm_enable 							iet_is_drm_enable

#define eti_set_certificate_path 							iet_set_certificate_path

#define eti_open_drm_file 							iet_open_drm_file
#define eti_is_certificate_ok 						iet_is_certificate_ok
#define eti_read_drm_file 							iet_read_drm_file
#define eti_close_drm_file 							iet_close_drm_file
#define eti_set_openssl_rsa_interface 				iet_set_openssl_rsa_interface

#define eti_start_http_server 				iet_start_http_server
#define eti_start_search_server				iet_start_search_server
#define eti_assistant_set_callback_func 				iet_assistant_set_callback_func
#define eti_set_system_path 				iet_set_system_path
#define eti_set_host_ip						iet_set_host_ip
#define eti_clear_host_ip					iet_clear_host_ip

#define eti_query_shub_by_url		iet_query_shub_by_url
#define eti_get_info_from_cfg_file		iet_get_info_from_cfg_file
#define eti_set_origin_url_to_cfg_file	iet_set_origin_url_to_cfg_file

_int32 iet_init(ETI_PROXY_INFO* proxy_for_hub);
_int32 iet_uninit(void);


_int32 iet_set_license(char *license, int32 license_size);
_int32 iet_set_license_callback( ETI_NOTIFY_LICENSE_RESULT license_callback_ptr);
const char*  iet_get_version(void);

_int32 iet_create_new_task_by_url(char* url, u32 url_length, 
							  char* ref_url, u32 ref_url_length,
							  char* description, u32 description_len,
							  char* file_path, u32 file_path_len,
							  char* file_name, u32 file_name_length, 
							  u32* task_id);

_int32 iet_try_create_new_task_by_url(char* url, u32 url_length, 
							  char* ref_url, u32 ref_url_length,
							  char* description, u32 description_len,
							  char* file_path, u32 file_path_len,
							  char* file_name, u32 file_name_length, 
							  u32* task_id);

_int32 iet_create_continue_task_by_url(char* url, u32 url_length, 
								   char* ref_url, u32 ref_url_length,
								   char* description, u32 description_len,
								   char* file_path, u32 file_path_len,
								   char* file_name, u32 file_name_length,
								   u32* task_id);


_int32 iet_create_new_task_by_tcid(u8 *tcid, uint64 file_size, char *file_name, u32 file_name_length, char *file_path, u32 file_path_len, u32* task_id );

_int32 iet_create_continue_task_by_tcid(u8 *tcid, char*file_name, u32 file_name_length, char*file_path, u32 file_path_len, u32* task_id );
_int32 iet_create_task_by_tcid_file_size_gcid(u8 *tcid, uint64 file_size, u8 *gcid,char *file_name, u32 file_name_length, char *file_path, u32 file_path_len, u32* task_id );
_int32 iet_create_bt_task(char* seed_file_full_path, u32 seed_file_full_path_len, 
								char* file_path, u32 file_path_len,
								u32* download_file_index_array, u32 file_num,
								enum ETI_ENCODING_SWITCH_MODE encoding_switch_mode, u32* task_id);

_int32 iet_create_emule_task(const char* ed2k_link, u32 ed2k_link_len, char* path, u32 path_len,char* file_name, u32 file_name_length,  u32* task_id);

_int32 iet_create_bt_magnet_task(char * url, u32 url_len, char * file_path, u32 file_path_len,  char * file_name,u32 file_name_len,BOOL bManual, enum ET_ENCODING_SWITCH_MODE encoding_switch_mode, u32 * p_et_taskid);
_int32 iet_set_task_no_disk(u32 task_id);

_int32 iet_vod_set_task_check_data(u32 task_id);
_int32 iet_start_task(u32 task_id);


_int32 iet_stop_task(u32 task_id);

_int32 iet_delete_task(u32 task_id);


_int32 iet_get_task_info(u32 task_id, ETI_TASK *info);

_int32 iet_get_task_file_name(u32 task_id, char *file_name_buffer, _int32* buffer_size);
_int32 iet_get_task_tcid(u32 task_id, _u8 *tcid);




_int32 iet_get_torrent_seed_info( char *seed_path, ETI_TORRENT_SEED_INFO **pp_seed_info );

_int32 iet_get_torrent_seed_info_with_encoding_mode( char *seed_path, enum ETI_ENCODING_SWITCH_MODE encoding_switch_mode, ETI_TORRENT_SEED_INFO **pp_seed_info );


_int32 iet_release_torrent_seed_info( ETI_TORRENT_SEED_INFO *p_seed_info );


_int32 iet_set_seed_switch_type( enum ETI_ENCODING_SWITCH_MODE switch_type );

_int32 iet_get_bt_download_file_index( u32 task_id, u32 *buffer_len, u32 *file_index_buffer );

_int32 iet_get_bt_file_info(u32 task_id, u32 file_index, ETI_BT_FILE *info);

_int32 iet_extract_ed2k_url(char* ed2k, ET_ED2K_LINK_INFO* info);


_int32 iet_remove_tmp_file(char* file_path, char* file_name);
_int32 iet_set_customed_interface(_int32 fun_idx, void *fun_ptr);
_int32 iet_set_download_record_file_path(const char *path,_u32  path_len);
_int32 iet_set_max_tasks(_u32 task_num);

_u32 iet_get_max_tasks(void);
_int32 iet_set_limit_speed(_u32 download_limit_speed, _u32 upload_limit_speed);

_int32 iet_get_limit_speed(_u32* download_limit_speed, _u32* upload_limit_speed);
_int32 iet_set_max_task_connection(_u32 connection_num);
_u32 iet_get_max_task_connection(void);
_u32 iet_get_current_download_speed(void);
_u32 iet_get_current_upload_speed(void);
_int32 iet_get_all_task_id(  _u32 *buffer_len, _u32 *task_id_buffer );
_int32 iet_remove_task_tmp_file(_u32 task_id);
_int32 iet_get_bt_file_path_and_name(_u32 task_id, _u32 file_index,char *file_path_buffer, _int32 *file_path_buffer_size, char *file_name_buffer, _int32* file_name_buffer_size);
_int32 iet_vod_read_file(_int32 task_id, _u64 start_pos, _u64 len, char *buf, _int32 block_time );
_int32 iet_vod_bt_read_file(_int32 task_id, _u32 file_index, _u64 start_pos, _u64 len, char *buf, _int32 block_time );
_int32 iet_vod_set_buffer_time(_int32 buffer_time );
_int32 iet_vod_get_buffer_percent(_int32 task_id,  int32* percent );
_int32 iet_vod_get_download_position(_int32 task_id,  _u64* position );
_int32 iet_set_customed_interface_mem(_int32 fun_idx, void *fun_ptr);

_int32 iet_vod_set_vod_buffer_size(_int32 buffer_size);
_int32 iet_vod_get_vod_buffer_size(_int32* buffer_size);
_int32 iet_vod_is_vod_buffer_allocated(_int32* allocated);
_int32 iet_vod_free_vod_buffer(void);
_int32 iet_vod_get_bitrate(int32 task_id, u32 file_index, u32* bitrate);
_int32 iet_vod_is_download_finished(int32 task_id, BOOL* finished);

_int32 iet_reporter_mobile_user_action_to_file(u32 action_type, u32 action_value, void* data, u32 data_len);

//ip port 均为主机字节序

_int32 iet_create_cmd_proxy( u32 ip, u16 port, u32 attribute, u32 *p_cmd_proxy_id );
_int32 iet_create_cmd_proxy_by_domain( const char* host, u16 port, u32 attribute, u32 *p_cmd_proxy_id );
_int32 iet_cmd_proxy_get_info( u32 cmd_proxy_id, int32 *p_errcode, u32 *p_recv_cmd_id, u32 *p_data_len );

_int32 iet_cmd_proxy_send( u32 cmd_proxy_id, const char* buffer, u32 len );

_int32 iet_cmd_proxy_get_recv_info( u32 cmd_proxy_id, u32 recv_cmd_id, char *data_buffer, u32 data_buffer_len );

_int32 iet_cmd_proxy_close( u32 cmd_proxy_id );

_int32 iet_get_peerid( char *buffer, u32 buffer_size );

_int32  iet_get_free_disk(const char *path, _u64 *free_size);

_int32 iet_http_get(ET_HTTP_PARAM * param,u32 * http_id);
_int32 iet_http_post(ET_HTTP_PARAM * param,u32 * http_id);
_int32 iet_http_close(u32 http_id);
_int32  iet_is_drm_enable( BOOL *p_is_enable );

_int32  iet_set_certificate_path( const char *certificate_path );

_int32  iet_open_drm_file( const char *p_file_full_path, u32 *p_drm_id, uint64 *p_origin_file_size );
_int32  iet_is_certificate_ok( u32 drm_id, BOOL *p_is_ok );
_int32  iet_read_drm_file( u32 drm_id, char *p_buffer, u32 size, 
    uint64 file_pos, u32 *p_read_size );
_int32  iet_close_drm_file( u32 drm_id );
_int32  iet_set_openssl_rsa_interface( _int32 fun_count, void *fun_ptr_table );

_int32 iet_start_http_server(_u16 * port);
_int32 iet_start_search_server(ET_SEARCH_SERVER * p_search);
_int32 iet_assistant_set_callback_func(ET_ASSISTANT_INCOM_REQ_CALLBACK req_callback,ET_ASSISTANT_SEND_RESP_CALLBACK resp_callback);

_int32 iet_set_system_path(const char * system_path);

_int32 iet_set_host_ip(const char * host_name, const char *ip);
_int32 iet_clear_host_ip(void);

_int32 iet_query_shub_by_url(ET_QUERY_SHUB * p_param,u32 * p_action_id);

_int32 iet_get_info_from_cfg_file(char* cfg_path, char* url, BOOL* cid_is_valid, u8* cid, uint64* file_size, char* file_name, char *lixian_url, char *cookie, char *user_data);


#endif /* __ET_INTERFACE_H_20090924 */
