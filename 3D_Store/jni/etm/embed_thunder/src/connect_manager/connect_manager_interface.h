/*****************************************************************************
 *
 * Filename: 			connect_manager_interface.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2008/08/08
 *	
 * Purpose:				Contains the platforms provided by connect manager.
 *
 *****************************************************************************/


#if !defined(__CONNECT_MANAGER_INTERFACE_20080617)
#define __CONNECT_MANAGER_INTERFACE_20080617

#ifdef __cplusplus
extern "C" 
{
#endif

#include "connect_manager/connect_manager_imp.h"


struct _tag_data_manager;

#ifdef ENABLE_BT
struct tagBT_DATA_MANAGER;
#endif /* #ifdef ENABLE_BT  */

_int32 init_connect_manager_module(void);
_int32 uninit_connect_manager_module(void);

_int32 cm_init_connect_manager( CONNECT_MANAGER* p_connect_manager, struct _tag_data_manager *p_data_manager, can_destory_resource_call_back p_call_back );
_int32 cm_uninit_connect_manager( CONNECT_MANAGER *p_connect_manager );

#ifdef ENABLE_BT
_int32 cm_init_bt_magnet_connect_manager( CONNECT_MANAGER* p_connect_manager, void *p_data_manager );
_int32 cm_uninit_bt_magnet_connect_manager(CONNECT_MANAGER* p_connect_manager );
_int32 cm_init_bt_connect_manager( CONNECT_MANAGER* p_connect_manager, struct tagBT_DATA_MANAGER *p_bt_data_manager, can_destory_resource_call_back p_call_back );
_int32 cm_uninit_bt_connect_manager(CONNECT_MANAGER* p_connect_manager );
_int32 cm_create_sub_connect_manager( CONNECT_MANAGER *p_connect_manager, _u32 file_index );

#ifdef ENABLE_BT_PROTOCOL
_int32 cm_add_bt_resource( CONNECT_MANAGER *p_connect_manager, _u32 host_ip, _u16 port, _u8 info_hash[20] );
_int32 cm_notify_have_piece( CONNECT_MANAGER *p_connect_manager, _u32 piece_index );
_int32 cm_update_pipe_can_download_range( CONNECT_MANAGER *p_connect_manager);
_u32 cm_get_sub_manager_upload_speed( CONNECT_MANAGER *p_connect_manager );
#endif 
_int32 cm_init_bt_magnet_connect_manager( CONNECT_MANAGER* p_connect_manager, void *p_data_manager );
_int32 cm_uninit_bt_magnet_connect_manager(CONNECT_MANAGER* p_connect_manager );
#endif /* #ifdef ENABLE_BT  */

_int32 cm_delete_sub_connect_manager( CONNECT_MANAGER *p_connect_manager, _u32 file_index );

#ifdef ENABLE_EMULE
_int32 cm_init_emule_connect_manager( CONNECT_MANAGER* p_connect_manager, void* p_data_manager, can_destory_resource_call_back p_call_back,
											void** function_table);
_int32 cm_uninit_emule_connect_manager(CONNECT_MANAGER* p_connect_manager );
_int32 cm_add_emule_resource(CONNECT_MANAGER* connect_manager, _u32 ip, _u16 port, _u32 server_ip, _u16 server_port);
#endif

_int32 cm_pause_pipes(CONNECT_MANAGER *p_connect_manager);
_int32 cm_resume_pipes(CONNECT_MANAGER *p_connect_manager);
BOOL cm_is_pause_pipes(CONNECT_MANAGER *p_connect_manager);
BOOL cm_is_cdn_peer_res_exist_by_peerid(void * v_connect_manager, const  char* peerid);

_int32 cm_add_server_resource( CONNECT_MANAGER* p_connect_manager, _u32 file_index, char *url, _u32 url_size, char* ref_url, _u32 ref_url_size ,_u8 resource_priority);
_int32 cm_add_server_resource_ex( CONNECT_MANAGER* p_connect_manager, _u32 file_index, char *url, _u32 url_size, char* ref_url, _u32 ref_url_size,_u8 resource_priority, void* p_task, BOOL relation_res, _u32 relation_index );
#ifdef ENABLE_LIXIAN
_int32 cm_add_lixian_server_resource( CONNECT_MANAGER* p_connect_manager, _u32 file_index, char *url, _u32 url_size, char* ref_url, _u32 ref_url_size, char* cookie, _u32 cookie_size,_u32 resource_priority );
#endif /* ENABLE_LIXIAN */
_int32 cm_add_origin_server_resource( CONNECT_MANAGER* p_connect_manager, _u32 file_index, char *url, _u32 url_size, char* ref_url, _u32 ref_url_size, RESOURCE **pp_orgin_res );
_int32 cm_add_active_peer_resource( CONNECT_MANAGER *p_connect_manager
    , _u32 file_index, char *peer_id, _u8 *gcid, _u64 file_size
    , _u32 peer_capability, _u32 host_ip, _u16 tcp_port, _u16 udp_port
    ,  _u8 res_level, _u8 res_from, _u8 res_priority);
_int32 cm_add_passive_peer( CONNECT_MANAGER* p_connect_manager, _u32 file_index, RESOURCE *p_peer_res, DATA_PIPE *p_peer_data_pipe );

_int32 cm_create_pipes( CONNECT_MANAGER *p_connect_manager );
_int32 cm_create_sub_manager_pipes( CONNECT_MANAGER *p_connect_manager, _u32 file_index );
_int32 cm_get_connect_manager_list( CONNECT_MANAGER *p_connect_manager, LIST *p_connect_manager_list );
_int32 cm_get_idle_server_res_list( CONNECT_MANAGER *p_connect_manager, RESOURCE_LIST **pp_res_list );
_int32 cm_get_using_server_res_list( CONNECT_MANAGER *p_connect_manager, RESOURCE_LIST **pp_res_list );

_int32 cm_get_idle_peer_res_list( CONNECT_MANAGER *p_connect_manager, RESOURCE_LIST **pp_res_list );
_int32 cm_get_using_peer_res_list( CONNECT_MANAGER *p_connect_manager, RESOURCE_LIST **pp_res_list );

_int32 cm_get_connecting_server_pipe_list( CONNECT_MANAGER *p_connect_manager, PIPE_LIST **pp_pipe_list );
_int32 cm_get_connecting_server_pipe_num( CONNECT_MANAGER *p_connect_manager);

_int32 cm_get_working_server_pipe_list( CONNECT_MANAGER *p_connect_manager, PIPE_LIST **pp_pipe_list );
_int32 cm_get_working_server_pipe_num( CONNECT_MANAGER *p_connect_manager);

_int32 cm_get_connecting_peer_pipe_list( CONNECT_MANAGER *p_connect_manager, PIPE_LIST **pp_pipe_list );
_int32 cm_get_connecting_peer_pipe_num( CONNECT_MANAGER *p_connect_manager);

_int32 cm_get_working_peer_pipe_list( CONNECT_MANAGER *p_connect_manager, PIPE_LIST **pp_pipe_list );
_int32 cm_get_working_peer_pipe_num( CONNECT_MANAGER *p_connect_manager );

_u32 cm_get_upload_speed( CONNECT_MANAGER *p_connect_manager );

_u32 cm_get_task_bt_download_speed( CONNECT_MANAGER *p_connect_manager );

#ifdef _CONNECT_DETAIL
void cm_get_working_peer_type_info( CONNECT_MANAGER *p_connect_manager, _u32 *p_tcp_pipe_num, _u32 *p_udp_pipe_num, _u32 *p_tcp_pipe_speed, _u32 *p_udp_pipe_speed );

_int32 cm_get_idle_server_res_num( CONNECT_MANAGER *p_connect_manager );
_int32 cm_get_using_server_res_num( CONNECT_MANAGER *p_connect_manager );
_int32 cm_get_candidate_server_res_num( CONNECT_MANAGER *p_connect_manager );
_int32 cm_get_retry_server_res_num( CONNECT_MANAGER *p_connect_manager );
_int32 cm_get_discard_server_res_num( CONNECT_MANAGER *p_connect_manager );
_int32 cm_get_destroy_server_res_num( CONNECT_MANAGER *p_connect_manager );

_int32 cm_get_idle_peer_res_num( CONNECT_MANAGER *p_connect_manager );
_int32 cm_get_using_peer_res_num( CONNECT_MANAGER *p_connect_manager );

_int32 cm_get_candidate_peer_res_num( CONNECT_MANAGER *p_connect_manager );
_int32 cm_get_retry_peer_res_num( CONNECT_MANAGER *p_connect_manager );
_int32 cm_get_discard_peer_res_num( CONNECT_MANAGER *p_connect_manager );
_int32 cm_get_destroy_peer_res_num( CONNECT_MANAGER *p_connect_manager );

_u32 cm_get_cm_level( CONNECT_MANAGER *p_connect_manager );
void *cm_get_peer_pipe_info_array( CONNECT_MANAGER *p_connect_manager );
void *cm_get_server_pipe_info_array( CONNECT_MANAGER *p_connect_manager );
#endif

BOOL cm_get_excellent_filename( CONNECT_MANAGER *p_connect_manager, char *p_str_name_buffer, _u32 buffer_len );
_u32 cm_get_current_task_speed( CONNECT_MANAGER *p_connect_manager );

_u32 cm_get_current_task_server_speed( CONNECT_MANAGER *p_connect_manager );
_u32 cm_get_current_task_peer_speed( CONNECT_MANAGER *p_connect_manager );	

_int32 cm_set_origin_download_mode( CONNECT_MANAGER *p_connect_manager, RESOURCE *p_orgin_res );
_int32 cm_set_origin_mode( CONNECT_MANAGER *p_connect_manager, BOOL origin_mode );
BOOL cm_get_origin_mode( CONNECT_MANAGER *p_connect_manager );

BOOL cm_is_need_more_server_res( CONNECT_MANAGER *p_connect_manager, _u32 file_index );
BOOL cm_is_need_more_peer_res( CONNECT_MANAGER *p_connect_manager, _u32 file_index );
BOOL cm_is_global_need_more_res(void);

BOOL cm_is_idle_status( CONNECT_MANAGER *p_connect_manager, _u32 file_index );
void cm_set_not_idle_status( CONNECT_MANAGER *p_connect_manager, _u32 file_index );

void cm_p2s_set_enable( BOOL is_enable );
void cm_p2p_set_enable( BOOL is_enable );

void cm_set_task_max_pipe( _u32 pipe_num  );
_u32 cm_get_task_max_pipe(void);

/*****************************************************************************
 * Interface for global dispatch
 *****************************************************************************/
_int32 cm_set_global_strategy( BOOL is_using );
_int32 cm_set_global_max_pipe_num( _u32 max_pipe_num );
_int32 cm_set_global_max_connecting_pipe_num( _u32 max_connecting_pipe_num );
_int32 cm_do_global_connect_dispatch(void);
_int32 cm_get_global_res_num(void);
_int32 cm_get_global_pipe_num(void);
_int32 cm_pause_all_global_pipes(void);
_int32 cm_resume_all_global_pipes(void);
_int32 cm_pause_global_pipes(void);
_int32 cm_resume_global_pipes(void);

#ifdef ENABLE_CDN
/***************************************************************************************************/
/*   CDN Interfaces for other modules   */
/***************************************************************************************************/
BOOL cm_is_need_more_cdn_res(CONNECT_MANAGER *p_connect_manager, _u32 file_index);
_int32 cm_get_cdn_peer_count(RESOURCE_LIST * p_res_list,_u8 res_from );
_int32 cm_add_cdn_peer_resource( CONNECT_MANAGER *p_connect_manager, _u32 file_index, char *peer_id, _u8 *gcid, _u64 file_size, _u32 peer_capability,  _u32 host_ip, _u16 tcp_port , _u16 udp_port, _u8 res_level, _u8 res_from );
_int32 cm_add_cdn_server_resource( CONNECT_MANAGER* p_connect_manager, _u32 file_index, char *url, _u32 url_size, char* ref_url, _u32 ref_url_size );
_int32 cm_set_cdn_mode(CONNECT_MANAGER *p_connect_manager, _u32 file_index,BOOL mode);
_int32 cm_get_cdn_pipe_list( CONNECT_MANAGER *p_connect_manager, PIPE_LIST **pp_pipe_list );
_u32 cm_get_cdn_speed( CONNECT_MANAGER *p_connect_manager, BOOL is_contain_normal_cdn );
_u32 cm_get_current_connect_manager_cdn_speed(CONNECT_MANAGER *p_connect_manager, BOOL is_contain_normal_cdn);
// 根据条件判断是否可以在normal cdn上开启链接
struct _tag_task;
_int32 cm_can_open_normal_cdn_pipe(CONNECT_MANAGER *p_connect_manager);
_int32 cm_can_close_normal_cdn_pipe(CONNECT_MANAGER *p_connect_manager);

//
// 取normal cdn的速度
//
_u32 cm_get_normal_cdn_speed(CONNECT_MANAGER *p_connect_manager);
_u32 cm_get_current_connect_manager_normal_cdn_speed(CONNECT_MANAGER *p_connect_manager);

//
// 取当前任务中无normal cdn资源的速度
//
_u32 cm_get_current_task_speed_without_normal_cdn(CONNECT_MANAGER *p_connect_manager);

//
// 取normal cdn上的pipe建立时间
//
_u32 cm_get_normal_cdn_connect_time(CONNECT_MANAGER *p_connect_manager);

// 
// 取normal cdn上的使用时间
//
_u32 cm_get_normal_cdn_stat_para(CONNECT_MANAGER *p_connect_manager, BOOL is_using_sub_conn,
								_u32 *use_time, _u32 *cdn_cnt, _u32 *cdn_valid_cnt, _u32 *first_zero_dura);

#ifdef ENABLE_HSC
_u32 cm_get_vip_cdn_speed( CONNECT_MANAGER *p_connect_manager );
_u32 cm_get_current_connect_manager_vip_cdn_speed( CONNECT_MANAGER *p_connect_manager );
_int32 cm_enable_high_speed_channel(CONNECT_MANAGER *p_connect_manager, _u32 file_index, _int32 value);
_int32 cm_disable_high_speed_channel(CONNECT_MANAGER *p_connect_manager);

_int32 cm_check_high_speed_channel_flag(const CONNECT_MANAGER *p_connect_manager, const _u32 file_index);
#endif /* ENABLE_HSC */
_int32 cm_get_working_cdn_pipe_num( CONNECT_MANAGER *p_connect_manager);
_int32 cm_get_cdn_res_num( CONNECT_MANAGER *p_connect_manager);
#ifdef ENABLE_LIXIAN
/* 离线下载相关信息 */
typedef  enum lixian_state {LS_IDLE=0, LS_RUNNING, LS_FAILED } LIXIAN_STATE;
typedef struct t_lixian_info
{
	LIXIAN_STATE 	_state;
	_u32 					_res_num;	//资源个数
	_u64 				_dl_bytes;	//通过离线资源下载到的数据
	_u32 					_speed;		
	_int32	 			_errcode;
}LIXIAN_INFO;
typedef struct tagLIXIAN_INFO_BRD
{
	_u32						_task_id;
	LIXIAN_INFO		_lixian_info;
} LIXIAN_INFO_BRD;

_u32 cm_get_current_connect_manager_lixian_speed( CONNECT_MANAGER *p_connect_manager );
_u32 cm_get_current_connect_manager_lixian_res_num( CONNECT_MANAGER *p_connect_manager );
_u32 cm_get_lixian_info(CONNECT_MANAGER *p_connect_manager, _u32 file_index,LIXIAN_INFO *p_lx_info);
_u32 cm_get_lixian_speed(CONNECT_MANAGER *p_connect_manager);
#endif /* ENABLE_LIXIAN */
#endif /* ENABLE_CDN */

#ifdef EMBED_REPORT
struct tagCM_REPORT_PARA *cm_get_report_para( CONNECT_MANAGER *p_connect_manager, _u32 file_index );
#endif

_int32 cm_get_bt_acc_file_idx_list(CONNECT_MANAGER *p_connect_manager, LIST *p_list);

// 获取原始资源下载的信息
typedef struct t_origin_resource_info
{
	_int32	 			_errcode;
	_u64 				_dl_bytes;	
	_u32 				_speed;		
}ORIGIN_RESOURCE_INFO;

_u32 cm_get_origin_resource_speed(CONNECT_MANAGER *p_connect_manager);

typedef struct t_assist_peer_info
{
	_u32 				_res_num;	//资源个数
	_u64 				_dl_bytes;	//通过离线资源下载到的数据
	_u32 				_speed;		
	_int32	 			_errcode;
}ASSIST_PEER_INFO;

_u32 cm_get_current_connect_manager_assist_peer_speed( CONNECT_MANAGER *p_connect_manager );
_u32 cm_get_current_connect_manager_assist_peer_res_num( CONNECT_MANAGER *p_connect_manager );
_u32 cm_get_assist_peer_info(CONNECT_MANAGER *p_connect_manager, _u32 file_index, ASSIST_PEER_INFO *p_lx_info);


BOOL cm_is_p2sptask(CONNECT_MANAGER *p_connect_manager);
BOOL cm_is_bttask(CONNECT_MANAGER * p_connect_manager);

// 必须为 p2sp任务才调用该接口，否则返回FALSE
BOOL cm_shubinfo_valid(CONNECT_MANAGER *p_connect_manager);

typedef struct t_task_downloading_info
{
	_u32 		_orgin_res_speed;	        //原始资源下载速度
	_u64 		_orgin_res_dl_bytes;	    //原始资源下载数据量
	_u32 		_all_download_speed;	    //总速度
	_u64 		_downloaded_data_size;   //总下载数据量
	_u32 		_pc_assist_res_num;	    //协同加速资源数
	_u32 		_pc_assist_res_speed;	    //协同加速下载速度
	_u64	 	_pc_assist_res_dl_bytes; //协同加速下载数据量

	_u32 		_idle_server_res_num;       //下载资源的空闲服务资源数
	_u32 		_idle_peer_res_num;         //下载资源的空闲peer资源数
	_u32 		_using_server_res_num;      //下载资源的正使用服务资源数
	_u32 		_using_peer_res_num;        //下载资源的正使用peer资源数
	_u32 		_candidate_server_res_num; //下载资源的候选服务资源数
	_u32 		_candidate_peer_res_num;   //下载资源的候选peer资源数
	_u32 		_retry_server_res_num;     //下载资源的重试服务资源数
	_u32 		_retry_peer_res_num;       //下载资源的重试peer资源数
	_u32 		_discard_server_res_num;   //下载资源的废弃服务资源数
	_u32 		_discard_peer_res_num;     //下载资源的废弃peer资源数


	_int32 		_state;
	_u32 		_res_num;	//资源个数
	_u64 		_dl_bytes;	//通过高速通道下载到的数据
	_u32 		_speed;		
	_int32	 	_errcode;
}TASK_DOWNLOADING_INFO;

//BOOL cm_is_small_file_mode( CONNECT_MANAGER* p_connect_manager );


#ifdef __cplusplus
}
#endif

#endif // !defined(__CONNECT_MANAGER_INTERFACE_20080617)

