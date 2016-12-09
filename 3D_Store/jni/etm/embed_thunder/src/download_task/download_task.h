#if !defined(__DOWNLOAD_TASK_H_20080918)
#define __DOWNLOAD_TASK_H_20080918

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#include "dispatcher/dispatcher_interface.h"
#include "connect_manager/connect_manager_interface.h"
#include "high_speed_channel/hsc_info.h"
#include "download_task_struct_define.h"


typedef struct _tag_main_task_lixian_info
{
	_u64 				_dl_bytes;
	_u32 				_speed;	
}MAIN_TASK_LIXIAN_INFO;

typedef struct _tag_task
{
	enum TASK_TYPE _task_type;
	enum TASK_STATUS  task_status;
	enum TASK_FILE_CREATE_STATUS _file_create_status;
	_u32 _task_id;
    _u8 _eigenvalue[CID_SIZE];
	_u32   failed_code;
	DISPATCHER _dispatcher;
	CONNECT_MANAGER   _connect_manager;
	
	/* Time information  */
	_u32 _start_time;	
	_u32 _finished_time;

	/* Speed information  */
	_u32 speed;
	_u32 server_speed;
	_u32 peer_speed;
	_u32 ul_speed;


	/* Progress information  */
	_u32 progress;

	/* Data information  */
	_u64 file_size;// 0 表示还不知道文件大小
	_u64 _downloaded_data_size; 		 /*已下载的数据大小*/  
	_u64 _written_data_size; 			 /*已写进磁盘的数据大小*/  

	BOOL need_remove_tmp_file;  /* 在删除任务时是否也要删除临时文件？*/
	_int32 _control_flag;
	
	/* Resource  information */
	//_u64 sum_res_count; /* _total_server_res_count + _total_peer_res_count */
	//_u64 using_res_count; /* _using_server_res_count + _using_peer_res_count */

	/* Server resource  information */
	_u32 _total_server_res_count; 
	_u32 _valid_server_res_count;
	//_u32 _using_server_res_count;
	//_u32 _used_server_res_count;
	//_u32 _discard_server_res_count;
	//_u32 _idle_server_res_count;	
	//_u32 _connect_server_res_count;
	
	/* Peer resource  information   */
	_u32 _total_peer_res_count;
	//_u32 _using_peer_res_count;	
	//_u32 _used_peer_res_count;	
	//_u32 _discard_peer_res_count;
	//_u32 _idle_peer_res_count;	
	//_u32 _connect_peer_res_count;	
	
	/* Server pipe information  */
	_u32 dowing_server_pipe_num; //低8位表示CDN资源数，高24未表示Server资源数
	_u32 connecting_server_pipe_num;

	/* Peer pipe information  */
	_u32 dowing_peer_pipe_num;
	_u32 connecting_peer_pipe_num;
	
	//_u64 recv_bytes; //表示这个任务收到的字节数，这个值用来计算速度，不能用来计算进度
	//_u64 recv_valid_bytes; //表示这个任务收到的合法字节数，这个值用来计算进度
	//_u64 send_bytes; //这个任务发送的字节	
	//_u64 _origin_server_download_size;
	//_u64 _other_server_download_size;	
	//_u64 _server_download_size;	
	//_u64 _peer_download_size;
	//_u64 _cdn_download_size;  /* _cdn_download_size + _dcdn_download_size */
	//_u64 _cdn_download_size;
	//_u64 _dcdn_download_size;
	_int32 _cdn_num;
	_int32 _fault_block_size;

	/* Others need by reporter  */
	_int32	download_stat;
	_int32 dw_comefrom;	
	BOOL is_nated;  /* Peer是否为内网？   */

#ifdef _CONNECT_DETAIL
       _u32  valid_data_speed;
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
    void *_peer_pipe_info_array;
    void *_server_pipe_info_array;
#endif
	
#ifdef EMBED_REPORT
	RES_QUERY_REPORT_PARA _res_query_report_para;
#endif

#ifdef ENABLE_HSC
	HIGH_SPEED_CHANNEL_INFO _hsc_info;
#endif


	MAIN_TASK_LIXIAN_INFO  _main_task_lixian_info;


	_u32 _stop_vod_time;	//如果是点播任务,这里记录停止点播时间
	//struct _tag_task * parent_task_ptr;  /* 如果该任务是一个BT小任务（task_type = SUB_TASK_TYPE），这个指针指向它所附属的BT大任务  */

    _u32 _working_mode;  //0标明从原始资源下载, 第0位为1表明可多资源下载，第1位为1标明可使用PC加速，后面可扩展
	_int32 _extern_id;	// etm设置的任务id
    _u32 _create_time;  // etm设置进来的任务创建时间
    BOOL _is_continue;

	//PC assist info
	BOOL 				_use_assist_peer;
	struct t_edt_res*	_p_assist_res;
	DATA_PIPE* 			_p_assist_pipe;

	BOOL				_use_assist_pc_res_only;
	char				_gcid[CID_SIZE];
	char				_cid[CID_SIZE];
	_u64				_file_size;

	BOOL  _is_shub_cid_error;
	
	_u32 _first_byte_arrive_time;/*启速时长,单位毫秒*/
	_u32 _zero_speed_time_ms;/*零速时长,单位毫秒*/
    _u32 _last_download_stat_time_ms;/*上次统计的时间(启动后的毫秒数)*/
    _u64 _last_download_bytes; //上次统计时,已下载字节数
    _u32 _download_finish_time; //数据全部下载完的时间，即_downloaded_data_size==file_size的时间点
	_u32 _last_open_normal_cdn_time; // 上一次开启normal cdn pipe的时刻
	_u32 _last_close_normal_cdn_time; // 上一次关闭normal cdn pipe的时刻
	_u32 _query_normal_cdn_cnt;	// 查询cdn失败的次数，供定时器限制次数使用
}TASK;

typedef struct _tagRES_QUERY_PARA
{
	TASK *_task_ptr;
	_u32 _file_index;
}RES_QUERY_PARA;

struct _tag_data_manager;

/*--------------------------------------------------------------------------*/
/*                Interfaces provid for task_manager				        */
/*--------------------------------------------------------------------------*/
_int32 init_download_task_module(void);
_int32 uninit_download_task_module(void);
_int32 dt_remove_task_tmp_file( TASK * p_task );
_int32 dt_update_task_info( TASK* p_task  );

/*--------------------------------------------------------------------------*/
/*                Interfaces provid for data_manager				        */
/*--------------------------------------------------------------------------*/
_int32 dt_dispatch_at_pipe( TASK * p_task , DATA_PIPE*  ptr_data_pipe);
_int32 dt_start_dispatcher_immediate( TASK * p_task );

#ifdef _VOD_NO_DISK
BOOL dt_is_no_disk_task( TASK * p_task);
#endif

BOOL dt_is_vod_check_data_task( TASK * p_task);

/*--------------------------------------------------------------------------*/
/*                Internal functions				        */
/*--------------------------------------------------------------------------*/
_int32 dt_failure_exit(TASK* p_task,_int32 err_code );

/*--------------------------------------------------------------------------*/
/*                Interfaces provid for res_query				        */
/*--------------------------------------------------------------------------*/

/* add resource,these functions shuld be called by res_query */
_int32 dt_add_server_resource(void* user_data, char *url, _u32 url_size, char* ref_url, _u32 ref_url_size, BOOL is_origin,_u8 resource_priority);
_int32 dt_add_server_resource_ex(void* user_data, char *url, _u32 url_size, char* ref_url, _u32 ref_url_size, BOOL is_origin,_u8 resource_priority, BOOL relation_res, _u32 relation_index);
_int32 dt_add_peer_resource(void* user_data, char *peer_id, _u8 *gcid
    , _u64 file_size, _u32 peer_capability, _u32 host_ip, _u16 tcp_port, _u16 udp_port
    ,  _u8 res_level, _u8 res_from, _u8 res_priority);
_int32 dt_add_relation_fileinfo(void* user_data,  _u8 gcid[CID_SIZE],  _u8 cid[CID_SIZE], _u64 file_size,  _u32 block_num,  RELATION_BLOCK_INFO** p_block_info_arr);

#ifdef EMBED_REPORT
struct tagRES_QUERY_REPORT_PARA *dt_get_res_query_report_para( TASK * p_task );
#endif

struct _tag_data_manager;

_int32 init_task(TASK* task, enum TASK_TYPE type);

#ifdef ENABLE_EMULE
_int32 start_task(TASK* task, struct _tag_data_manager* data_manager, can_destory_resource_call_back func, void** pipe_function_table);

_int32 stop_task(TASK* task);

_int32 delete_task(TASK* task);
#endif
#ifdef ENABLE_LIXIAN
_int32 dt_add_task_res(TASK* p_task, EDT_RES * p_resource);
_int32 dt_get_lixian_info( TASK * p_task , _u32 file_index ,void * p_lx_info);
#endif /* ENABLE_LIXIAN */

_int32 dt_get_origin_resource_info( TASK * p_task, void * p_resource );

_int32 dt_add_assist_task_res(TASK* p_task, EDT_RES * p_resource);
_int32 dt_get_assist_task_res_info( TASK * p_task, _u32 file_index,void * p_lx_info );
_int32 dt_get_task_downloading_res_info( TASK * p_task, void *p_res_info );

_int32 dt_get_assist_task_info_impl(TASK* p_task, void * p_assist_task_info);

_int32 dt_set_recv_data_from_assist_pc_only_impl(TASK* p_task, EDT_RES * p_resource, ASSIST_TASK_PROPERTY * p_task_prop);

#ifdef __cplusplus
}
#endif


#endif // !defined(__DOWNLOAD_TASK_H_20080918)
