#if !defined(__TASK_MANAGER_H_20080917)
#define __TASK_MANAGER_H_20080917

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#include "utility/list.h"
#include "utility/errcode.h"
#include "utility/settings.h"
#include "asyn_frame/msg.h"
#include "asyn_frame/notice.h"
#include "download_task/download_task.h"


typedef struct 
{
	_u32 _sys_start_time;	
	_u32 _last_update_time;
	_u32 _cur_downloadtask_num;  
	_u32 _cur_download_speed;
	_u32 _cur_pipe_num;	
	_u32 _cur_mem_used;

	_u32 _total_task_num;  /* all tasks created,including runing tasks, waiting tasks,stoped tasks,completed tasks and deleted tasks*/

	_u32  _update_info_timer_id;
	//SEVENT_HANDLE _cancel_timer_event_handle;
}GLOBAL_STATISTIC;

typedef struct
{
	GLOBAL_STATISTIC _global_statistic;
	LIST _task_list;
	
	_u64 max_download_filesize; //最大可下载的文件大小, 单位M
	_u32 min_download_filesize; //最小可下载的文件大小, 单位B
	_u32 max_tasks;
	_int32 max_query_shub_retry_count;
	
	_u32 license_report_interval; //second
	_u32 license_expire_time;//second
	void * license_callback_ptr;
	_u32  _license_report_timer_id;
	_u32  _license_report_retry_count;
	SEVENT_HANDLE *_p_sevent_handle;  /* for waiting file closing call back */
}TASK_MANAGER;



typedef struct
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_u32 _task_id;
	char *_orignal_url;
	char *_ref_url;
} TM_ORIGINAL_URL_INFO;


#ifdef _CONNECT_DETAIL
typedef struct et_server_pipe_info_t
{
    char _id_str[64];
    _u32 _port;
    _u32 _speed;
    _u32 _pipe_type;
} SERVER_PIPE_INFO_T;

typedef struct et_server_pipe_info_array_t
{
    SERVER_PIPE_INFO_T _pipe_info_list[ SERVER_INFO_NUM ];
    _u32 pipe_info_num;
} SERVER_PIPE_INFO_ARRAY_T;

#endif

typedef struct t_et_task_info
{
	_u32 _task_id;
	
	_u32 _speed;    /*任务的下载速度*/
	_u32 _server_speed;   /*任务server 资源的下载速度*/  
	_u32 _peer_speed;   /*任务peer 资源的下载速度*/  
	_u32 _ul_speed;    /*任务的上传速度*/
	_u32 _progress;  /*任务进度*/  
	_u32 _dowing_server_pipe_num; /*任务server 连接数*/  
	_u32 _connecting_server_pipe_num;  /*任务server 正在连接数*/  
	_u32 _dowing_peer_pipe_num;  /*任务peer 连接数*/  
	_u32 _connecting_peer_pipe_num; /*任务pipe 正在连接数*/  
	
#ifdef _CONNECT_DETAIL
    _u32 _valid_data_speed;
    _u32 _bt_dl_speed;   /*任务BT 资源的下载速度（只对BT任务有效）*/  
    _u32 _bt_ul_speed;   /*任务BT 资源的上传速度（只对BT任务有效）*/  
    _u32 downloading_tcp_peer_pipe_num;
    _u32 downloading_udp_peer_pipe_num;
    
	_u32 downloading_tcp_peer_pipe_speed;
	_u32 downloading_udp_peer_pipe_speed;
    

	/* Server resource information */
	_u32 idle_server_res_num;
	_u32 using_server_res_num;
	_u32 candidate_server_res_num;
	_u32 retry_server_res_num;
	_u32 discard_server_res_num;
	_u32 destroy_server_res_num;

	/* Peer resource information */
	_u32 idle_peer_res_num;
	_u32 using_peer_res_num;

	_u32 candidate_peer_res_num;
	_u32 retry_peer_res_num;
	_u32 discard_peer_res_num;
	_u32 destroy_peer_res_num;
	_u32 cm_level;
	PEER_PIPE_INFO_ARRAY _peer_pipe_info_array;
	//void *_server_pipe_info_array;
#endif

	_u64 _file_size;  /*任务文件大小*/  

	enum TASK_STATUS  _task_status;   
	_u32  _failed_code;
	 
	/* Time information  */
	_u32 _start_time;	
	_u32 _finished_time;
	
	enum TASK_FILE_CREATE_STATUS _file_create_status;
	_u64 _downloaded_data_size; 		 /*已下载的数据大小*/  
	_u64 _written_data_size; 			 /*已写进磁盘的数据大小*/  

        //下面几个属性是是任务生命周期的统计数据
	/*以下几个值除file_size,first_byte_arrive_time外, 都在stop(或重启)后累计计算,时长单位默认是秒, xxx_recv_size 是未校验过的,xxx_download_size 是已校验过的*/
	_u64 _orisrv_recv_size;	/*原始路径下载字节数*/
	_u64 _orisrv_download_size;

	_u64 _altsrv_recv_size;	/*候选url下载字节数*/
	_u64 _altsrv_download_size;

	_u64 _hsc_recv_size;	/*高速通道下载字节数*/
	_u64 _hsc_download_size;

	_u64 _p2p_recv_size;	/*p2p下载字节数*/
	_u64 _p2p_download_size;

	_u64 _lixian_recv_size;	/*离线下载字节数*/
	_u64 _lixian_download_size;

	_u64 _total_recv_size;      //接收数据大小
	//uint64 total_download_size;  //有效数据下载大小
	
	_u32 _download_time;           //下载用时

	_u32 _first_byte_arrive_time;//起速时长, 任务开始后收到第一个字节的时长,单位秒	
	_u32 _zero_speed_time;	//零速时长,每秒统计一次, 任务开始后(已起速),在该秒内没收任何新数据,则认为该秒属于0速段,将该值+1
	_u32 _checksum_time;		//校验时长
    

}ET_TASK_INFO;

typedef struct proxy_info
{
	char server[MAX_HOST_NAME_LEN];
	char user[MAX_USER_NAME_LEN];
	char password[MAX_PASSWORD_LEN];
	
	_u32 port;
	
	_int32 proxy_type; //0 direct connect type, 1 socks5 proxy type, 	//2 http_proxy type, 3 ftp_proxy type
	_int32 server_len;
	_int32 user_len;
	_int32 password_len;
}PROXY_INFO;

#ifdef ENABLE_BT
typedef struct tag_BT_FILE_INFO_POOL
{
	_u32 _task_id;
	_u32 _file_num;
	void*_brd_ptr;
	void * _file_info_for_user;
}BT_FILE_INFO_POOL;
#endif  /* ENABLE_BT */
/*--------------------------------------------------------------------------*/
/*                Structures for posting functions				        */
/*--------------------------------------------------------------------------*/

typedef struct
{
	SEVENT_HANDLE _handle;
	_int32	_result;
	_u32	_download_limit_speed;
	_u32	_upload_limit_speed;
}TM_SET_SPEED;

typedef struct
{
	SEVENT_HANDLE _handle;
	_int32	_result;
	_u32*	_download_limit_speed;
	_u32*	_upload_limit_speed;
}TM_GET_SPEED;

typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_u32 *task_connection;
}TM_GET_CONNECTION;

typedef struct
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_u32 task_connection;
}TM_SET_CONNECTION;


#ifdef UPLOAD_ENABLE
typedef struct
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	const char *path;
	_u32 path_len;
}TM_SET_RC_PATH;
typedef struct
{
	SEVENT_HANDLE _handle;
	_int32 _result;
}TM_CLOSE_PIPES;
#endif

typedef struct
{
	SEVENT_HANDLE _handle;
	_int32 _result;
}TM_LOGOUT;

typedef struct
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_u32 max_tasks;
	//_u32 max_running_tasks;
}TM_SET_TASK_NUM;


typedef struct
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_u32* max_tasks;
	//_u32* max_running_tasks;
}TM_GET_TASK_NUM;


typedef struct
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_u32 task_id;
}TM_START_TASK;


typedef struct
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_u32 task_id;
}TM_STOP_TASK;

typedef struct
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_u32 task_id;
}TM_DEL_TASK;

typedef struct
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_u32 task_id;
	void *info;
}TM_GET_TASK_INFO;

typedef struct
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_u32 _task_id;
	void *_info;
}TM_GET_TASK_HSC_INFO;

typedef struct
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_u32 task_id;
	char *file_name_buffer;
	_int32 * buffer_size;
	_u32 file_index;
}TM_GET_TASK_FILE_NAME;

typedef struct
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_u32 task_id;
	_u8 * tcid;
	_u32 file_index;
}TM_GET_TASK_TCID;

typedef struct
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_u32 task_id;
	_u8 * gcid;
}TM_GET_TASK_GCID;

typedef struct
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_u32 task_id;
	void * _res;
}TM_ADD_TASK_RES;

#ifdef ENABLE_BT
typedef struct
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_u32 task_id;
	void *info;
	_u32 file_index;
}TM_GET_BT_FILE_INFO;
#endif /* #ifdef ENABLE_BT */

#ifdef ENABLE_BT

typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	char *seed_path;
	_int32 encoding_switch_mode; 
	void **pp_seed_info;
}TM_GET_TORRENT_SEED_INFO;

typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_u32 task_id;
	BOOL *_seed_download;
}TM_GET_TORRENT_SEED_FILE_PATH;

typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	void *p_seed_info;
}TM_RELEASE_TORRENT_SEED_INFO;

typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_u32 _switch_type;
}TM_SEED_SWITCH_TYPE;

typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_u32 _task_id;
	_u32 *_buffer_len;
	_u32 *_file_index_buffer;
}TM_GET_BT_FILE_INDEX;

typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_u32 task_id;
	_u32 file_index;
	char *file_path_buffer;
	_int32 *file_path_buffer_size;
	char *file_name_buffer;
	_int32 * file_name_buffer_size;
}TM_GET_BT_TASK_FILE_NAME;


typedef struct 
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_u32 task_id;
	char* tmp_file_path;
	char* tmp_file_name;
}TM_GET_BT_TMP_FILE_PATH;

#endif /*  #ifdef ENABLE_BT */

typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_int32 license_size;
	char *license;
}TM_SET_LICENSE;

typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	void * license_callback_ptr;
}TM_SET_LICENSE_CALLBACK;


typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_u32 task_id;
}TM_REMOVE_TMP_FILE;

typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_u32 *_buffer_len;
	_u32 *_task_id_buffer;
}TM_GET_ALL_TASK_ID;

typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	void*  _param;
	_u32 * _id;
}TM_MINI_HTTP;

typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_u32  _id;
}TM_MINI_HTTP_CLOSE;


typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32 _result;
    const char *_certificate_path_ptr;
}TM_SET_CERTIFICATE_PATH;

typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32 _result;
    const char *_file_full_path_ptr;
    _u32 *_drm_id_ptr;
    _u64 *_origin_file_size_ptr;
}TM_OPEN_DRM_FILE;

typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32 _result;
    _u32 _drm_id;
    BOOL *_is_ok;
}TM_IS_CERTIFICATE_OK;

typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32 _result;
    _u32 _drm_id;
    char *_buffer_ptr;
	_u32 _size;
	_u64 _file_pos;
    _u32 *_read_size_ptr;
    
}TM_READ_DRM_FILE;

typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32 _result;
    _u32 _drm_id;
    
}TM_CLOSE_DRM_FILE;

typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_u32 _task_id;
}TM_HIGH_SPEED_CHANNEL_SWITCH;
/*
typedef struct
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_u32 _task_id;
	HIGH_SPEED_CHANNEL_INFO *_p_hsc_info;
}TM_HIGH_SPEED_CHANNEL_INFO;
*/
typedef struct
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_u32 _task_id;
	_u64 _product_id;
	_u64 _sub_id;
}TM_HSC_SET_PRODUCT_SUB_ID_INFO;

typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32 _result;
}TM_POST_PARA_0;

typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	void *	_para1;
}TM_POST_PARA_1;

typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	void *	_para1;
	void *	_para2;
}TM_POST_PARA_2;

typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	void *	_para1;
	void *	_para2;
	void *	_para3;
}TM_POST_PARA_3;

typedef struct
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	void *	_para1;
	void *	_para2;
	void *	_para3;
	void *	_para4;
}TM_POST_PARA_4;


typedef struct
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	void *	_para1;
	void *	_para2;
	void *	_para3;
	void *	_para4;
	void *	_para5;
	void *	_para6;
	void *	_para7;
	void *	_para8;
	void *	_para9;
}TM_POST_PARA_9;

typedef struct
{
       SEVENT_HANDLE _handle;
       _int32  _result;
       _u32    _task_id;
       _u32    _file_index;
       char    *_url;
       _u32    _url_size;
       char    *_ref_url;
       _u32    _ref_url_size;
       char    *_cookie;
       _u32    _cookie_size;
       _u32    _resource_priority;
} TM_ADD_LX_RESOURCE;

typedef struct
{
       SEVENT_HANDLE _handle;
       _int32  _result;
       _u32    _task_id;
       _u32    _file_index;
       char    *_peer_id;
       _u8     *_gcid;
       _u64    _file_size;
       _u32     _peer_capability;
       _u32    _host_ip;
       _u16    _tcp_port;
       _u16    _udp_port;
       _u8     _res_level;
       _u8     _res_from;
} TM_ADD_CDN_PEER_RESOURCE;

typedef struct
{
       SEVENT_HANDLE _handle;
       _int32 _result;
       _u32  _task_id;
       _u32 *_hsc_speed;
       _u64 *_hsc_download_size;
} TM_HSC_INFO;

typedef struct
{
       SEVENT_HANDLE _handle;
       _int32 _result;
       _u32  _task_id;
       _u32 *_lixian_speed;
       _u64 *_lixian_download_size;
} TM_LIXIAN_INFO;

typedef struct
{
        SEVENT_HANDLE _handle;
        _int32 _result;
        _u32 _task_id;
        LIST *_file_index;
}TM_BT_ACC_FILE_INDEX;

typedef struct t_tm_lx_torrent_seed_info
{
        char _title_name[MAX_FILE_NAME_LEN];
        _u32 _title_name_len;
        _u64 _file_total_size;
        _u32 _file_num;
        _u8  _encoding;
        _u8 _info_hash[INFO_HASH_LEN];
        _u32 _piece_size;
        _u8 *_piece_hash_ptr;
        _u32 _piece_hash_len;
        _u32 _file_info_num;
        _u32 *_file_info_buf;
} TM_LX_TORRENT_SEED_INFO;

typedef struct
{
        SEVENT_HANDLE _handle;
        _int32 _result;
        _u32 task_id;
        void *torrent_info;
} TM_GET_BT_TASK_TORRENT_INFO;

typedef struct
{
        SEVENT_HANDLE _handle;
        _int32 _result;
        _u32 _task_id;
        _u32 _file_index;
        char *_cid;  //CID_SIZE = 20
        char *_gcid;
        _u64 *_file_size;
} TM_BT_FILE_INDEX_INFO;


typedef void (*deletet_task_notice_proc)(TASK *task_ptr);

/*--------------------------------------------------------------------------*/
/*                Internal functions				        */
/*--------------------------------------------------------------------------*/
_int32 tm_init( void * _param );
_int32 tm_uninit( void * _param );

/* 下载库全局基础模块的初始化 */
_int32 tm_basic_init( void );
_int32 tm_basic_uninit(void);

/* 下载库功能模块的初始化 */
_int32 tm_sub_module_init( void  );
_int32 tm_sub_module_uninit(void);

/* 下载库独立模块的初始化 */
_int32 tm_other_module_init( void  );
_int32 tm_other_module_uninit(void);

/* task_manager 模块的初始化 */
_int32 init_task_manager( void * _param );
_int32 uninit_task_manager(void);

_int32 tm_force_close_resource(void);

_int32 tm_set_speed_limit( void * _param );
_int32 tm_get_speed_limit( void * _param );

_int32 tm_get_task_connection_limit( void * _param );



_int32 tm_set_task_connection_limit( void * _param );

#ifdef UPLOAD_ENABLE
_int32 tm_set_download_record_file_path( void * _param );
_int32 tm_close_upload_pipes( void * _param );
_int32 tm_close_upload_pipes_notify( void *event_handle );
#endif

_int32 tm_set_task_num( void * _param );

_int32 tm_get_task_num( void * _param );

_int32 tm_create_task_precheck( SEVENT_HANDLE * p_sevent_handle);
_int32 tm_create_task_tag(TASK * p_task);

_int32 tm_create_new_task_by_url( void * _param );

_int32 tm_create_continue_task_by_url( void * _param );

_int32 tm_create_new_task_by_tcid( void * _param );

_int32 tm_create_continue_task_by_tcid( void * _param );

_int32 tm_create_task_by_tcid_file_size_gcid( void * _param );

#ifdef ENABLE_BT
_int32 tm_create_bt_task( void * _param );
_int32 tm_create_bt_magnet_task( void * _param );
#endif /*  #ifdef ENABLE_BT */

#ifdef ENABLE_EMULE
_int32 tm_create_emule_task(void* param);
#endif

#ifdef _VOD_NO_DISK   
_int32 tm_set_task_no_disk( void * _param );
#endif

_int32 tm_set_task_check_data( void * _param );
_int32 tm_set_task_write_mode( void * _param );
_int32 tm_get_task_write_mode( void * _param );

_int32 tm_start_task(void * _param);
_int32 tm_stop_task(void * _param);

//_int32 tm_clear_tasks(void *_param);


_int32 tm_delete_task( void * _param );

_int32 tm_get_task_info(void * _param);
_int32 tm_get_task_info_ex(void * _param);

_int32 tm_register_deletet_task_notice(deletet_task_notice_proc callback);
_int32 tm_unregister_deletet_task_notice(deletet_task_notice_proc callback);

#ifdef ENABLE_CDN
_int32 tm_pause_download_except_task(TASK * p_task,BOOL pause);
#ifdef ENABLE_HSC
_int32 tm_init_task_hsc_info(void);
_int32 tm_uninit_task_hsc_info(void);
_int32 tm_get_task_hsc_info(void * _param);
_int32 tm_update_task_hsc_info(void);
_int32 tm_delete_task_hsc_info(_u32 task_id);
#endif /* ENABLE_HSC */
#endif
_int32 tm_delete_task_info(_u32 task_id);
	
_int32 tm_get_task_by_id(_u32 task_id,TASK **_pp_task);

BOOL tm_is_task_exist_by_url( char* url, BOOL include_no_disk);

BOOL tm_is_task_exist_by_cid( const _u8* cid, BOOL include_no_disk);

#ifdef ENABLE_BT
BOOL tm_is_task_exist_by_info_hash( const _u8* info_hash);
#endif /* #ifdef ENABLE_BT */

_int32 tm_logout( void * _param );
_int32 tm_http_clear( void * _param );
_int32 tm_http_get( void * _param );
_int32 tm_http_post( void * _param );
_int32 tm_http_cancel( void * _param );
_int32 tm_http_close( void * _param );
/* 取消 global_pipes */
_int32 tm_resume_global_pipes_after_close_mini_http( void  );

_int32 tm_init_post_msg(void);
_int32 tm_uninit_post_msg(void);
_int32 tm_post_function(thread_msg_handle handler, void *args, SEVENT_HANDLE *_handle, _int32 * _result);
_int32 tm_post_function_unblock(thread_msg_handle handler, void *args, _int32 * _result);
_int32 tm_try_post_function(thread_msg_handle handler, void *args, SEVENT_HANDLE *_handle, _int32 * _result);

/* Release the signal sevent 
	Notice: this function is only called by dt_notify_file_closing_result_event from download_task when deleting a task */
void  tm_signal_sevent_handle(void);

#ifdef ENABLE_BT
_int32 tm_init_bt_file_info_pool(void);
_int32 tm_uninit_bt_file_info_pool(void);
_int32 tm_add_bt_file_info_to_pool(TASK* p_task);
_int32 tm_delete_bt_file_info_to_pool(_u32 task_id);
_int32 tm_get_bt_file_info(void * _param);
_int32 tm_get_bt_file_path_and_name(void * _param);
_int32 tm_get_bt_tmp_path(void * _param);

//判断磁力连任务种子文件是否下载完成
_int32 tm_get_bt_magnet_torrent_seed_downloaded(void * _param);

/* Get the torrent seed file information from seed_path  */
_int32 	tm_get_torrent_seed_info( void * _param);

/* Release the memory which malloc by  et_get_torrent_seed_info */
_int32 	tm_release_torrent_seed_info( void * _param);

/* Set whether switch bt utf8 mode to gdk moke */
_int32 	tm_set_seed_switch_type( void * _param );

_int32 	tm_get_bt_download_file_index( void * _param );

#endif /*  #ifdef ENABLE_BT */

/* set_license */
_int32 	tm_set_license(void * _param);

/* set_license_callback */
_int32 	tm_set_license_callback( void * _param);

/* tm_notify_license_report_result */
_int32 	tm_notify_license_report_result(_u32 result, _int32  report_interval,_int32  expire_time);

/* Remove the temp file(s) of task */
_int32 	tm_remove_task_tmp_file( void * _param);

/* Get the file name of task */
_int32 tm_get_task_file_name(void * _param);
_int32 tm_get_bt_task_sub_file_name(void * _param);
_int32 tm_get_task_tcid(void * _param);
_int32 tm_get_bt_task_sub_file_tcid(void * _param);
_int32 tm_get_bt_task_sub_file_gcid(void * _param);
_int32 tm_get_task_gcid(void * _param);
_int32 tm_add_task_resource(void * _param);
/* Get all the id of current exist tasks */
_int32 	tm_get_all_task_id( void * _param );

/*--------------------------------------------------------------------------*/
/*                Interfaces provid for p2p module				        */
/*--------------------------------------------------------------------------*/

BOOL tm_is_task_exist(const _u8* gcid, BOOL include_no_disk);

CONNECT_MANAGER * tm_get_task_connect_manager(const _u8* gcid,_u32 * file_index, _u8 file_id[FILE_ID_SIZE]);


 /*--------------------------------------------------------------------------*/
/*                Callback function for timer				        */
/*--------------------------------------------------------------------------*/
_int32 tm_handle_update_info_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid);
 _int32 tm_handle_license_report_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid);


/*--------------------------------------------------------------------------*/
/*                Interfaces provid for drm				        */
/*--------------------------------------------------------------------------*/
#ifdef ENABLE_DRM 

_int32 tm_set_certificate_path(void * _param);

_int32 tm_open_drm_file(void * _param);

_int32 tm_is_certificate_ok(void * _param);

_int32 tm_read_drm_file(void * _param);

_int32 tm_close_drm_file(void * _param);

#endif  /*  #ifdef ENABLE_DRM */

_int32 tm_get_task_list(LIST **pp_task_list);

#if 0//ENABLE_CDN
_int32 tm_enable_high_speed_channel(void *_param);
_int32 tm_disable_high_speed_channel(void *_param);
_int32 tm_get_hsc_info(void *_param);
_int32 tm_set_product_sub_id(void *_param);
#endif /* ENABLE_CDN */
#ifdef ENABLE_LIXIAN
_int32 tm_init_task_lixian_info(void);
_int32 tm_uninit_task_lixian_info(void);
_int32 tm_update_task_lixian_info(void);
_int32 tm_delete_task_lixian_info(_u32 task_id);
_int32 tm_get_lixian_info(_u32 task_id,_u32 file_index,void * p_lx_info);
_int32 tm_get_bt_lixian_info(void * _param);
#endif /* ENABLE_LIXIAN */

_int32 tm_add_cdn_peer_resource(void* _param);
_int32 tm_add_lixian_server_resource(void* _param);
_int32 tm_get_bt_acc_file_index(void *param);
_int32  tm_get_bt_task_torrent_info( void * _param);
_int32 tm_get_bt_file_index_info(void *_param);

/* 设置一个可读写的路径，用于下载库存放临时数据*/
_int32 	tm_set_system_path( void * _param);

/***************************************************************************/
/*******   网络状态监测	  ********************/
_int32 tm_start_check_network(void);
_int32 tm_stop_check_network(void);
_int32 tm_check_network_status(void);
_int32 tm_handle_connect( _int32 errcode, _u32 notice_count_left,void *user_data);
_int32 tm_check_network_server_start(void);
_int32 tm_check_network_server_stop(void);

typedef struct t_tm_query_shub_page
{
	RES_QUERY_PARA _query_para;
	void * _cb_ptr;
	void* _user_data;
	_u64 _file_size;
	char _file_suffix[16];
} TM_QUERY_SHUB_PAGE;
_int32 tm_res_query_notify_shub_handler(void* user_data,_int32 errcode, _u8 result, _u64 file_size, _u8* cid, _u8* gcid, _u32 gcid_level, _u8* bcid, _u32 bcid_size, _u32 block_size,_u32 retry_interval,char * file_suffix,_int32 control_flag);

typedef struct t_tm_query_shub_result
{
	void* _user_data;
	_int32 _result;				/* 0:成功; 其他值为失败 */
	_u64 _file_size;
	_u8 _cid[20];
	_u8 _gcid[20];
	char _file_suffix[16];			/* 文件后缀 */		
} TM_QUERY_SHUB_RESULT;
typedef _int32 (*tm_query_notify_shub_cb)(TM_QUERY_SHUB_RESULT * p_query_result);

typedef struct t_tm_query_shub
{
	const char* _url;
	const char* _refer_url;

	_u8 _cid[20];							/* 增加用cid 和filesize参数查询gcid的功能 */
	_u64 _file_size;

	void * _cb_ptr;
	void* _user_data;
} TM_QUERY_SHUB;
_int32 tm_query_shub_by_url(void * _param);
_int32 tm_query_shub_cancel(void * _param);

_int32 tm_set_origin_mode( void * _param );
_int32 tm_is_origin_mode( void * _param );
_int32 tm_get_origin_resource_info( void * _param );
_int32 tm_get_origin_download_data( void * _param );

_int32 tm_add_assist_peer_resource(void * _param);
_int32 tm_get_assist_peer_info(void * _param);
_int32 tm_get_task_downloading_info(void * _param);

_int32 tm_set_extern_id( void * _param );
_int32 tm_set_extern_info( void * _param );

_int32 	tm_set_notify_event_callback( void * _param);

_int32 tm_get_assist_task_info(void * _param);


_int32 tm_set_etm_scheduler_event(void* _param);

_int32 tm_update_task_list(void *_param);

_int32 tm_set_recv_data_from_assist_pc_only(void * _param);

BOOL tm_in_downloading();

_int32 tm_get_network_flow(void *_param);


#ifdef ENABLE_HSC
_int32 tm_hsc_set_user_info(void * _param);
#endif

#ifdef UPLOAD_ENABLE
_int32 tm_ulm_del_record_by_full_file_path(void *_param);
_int32 tm_ulm_change_file_path(void *_param);
#endif

_int32 tm_start_http_server(void *_param);
_int32 tm_stop_http_server(void *_param);

_int32 tm_set_host_ip(void *_param);
_int32 tm_clear_host_ip(void *_param);
_int32 tm_set_original_url(void *param);

//检测下载库是否在正常工作,返回SUCCESS即为正常，如果返回其它值或不返回则表示不正常
_int32 tm_check_alive(void *_param);

_int32 tm_set_task_dispatch_mode( void * _param );

_int32 tm_get_download_bytes(void* param);

_int32 tm_set_module_option(void* param);
_int32 tm_get_module_option(void* param);

_int32 tm_get_info_from_cfg_file(void * _param);

#ifdef __cplusplus
}
#endif


#endif // !defined(__TASK_MANAGER_H_20080917)
