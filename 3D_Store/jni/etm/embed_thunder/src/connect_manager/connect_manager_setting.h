/*****************************************************************************
 *
 * Filename: 			connect_manager_setting.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2008/08/28
 *	
 * Purpose:				Contain the global control parameters of connect manager.
 *
 *****************************************************************************/

#if !defined(__CONNECT_MANAGER_SETTING_20080828)
#define __CONNECT_MANAGER_SETTING_20080828

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"

/************************************************************************
 * The struct contains global control parameters, init them when modul init.
 ************************************************************************/
typedef struct tagCONNECT_MANAGER_SETTING 
{
	_u32 _max_pipe_num;	
	_u32 _max_server_pipe_num;
	_u32 _max_peer_pipe_num;
	
	_u32 _max_pipe_num_each_server;
	
	_u32 _max_connecting_pipe_num;
	_u32 _max_connecting_server_pipe_num;
	_u32 _max_connecting_peer_pipe_num;

	_u32 _max_res_retry_times;
	_u32 _max_orgin_res_retry_times;
	_u32 _status_idle_ticks;
	_u32 _retry_res_init_score;
	_u32 _retry_res_score_ratio;

	BOOL _is_enable_peer_download;
	BOOL _is_enable_server_download;
	BOOL _is_enable_http_download;
	BOOL _is_enable_ftp_download;
	BOOL _is_enable_p2p_download;
	
	BOOL _is_enable_bt_download;
	BOOL _is_enable_p2p_tcp;
	BOOL _is_enable_p2p_same_nat;
	BOOL _is_enable_p2p_tcp_broker;
	BOOL _is_enable_p2p_udp_broker;
	BOOL _is_enable_p2p_udt;
	BOOL _is_enable_punch_hole;

	BOOL _is_only_using_origin_server;
	
	_u32 _clear_hash_map_speed_ratio;//when speed descend, clear the destoryed res hash map
	_u32 _clear_hash_map_ticks;
	_u32 _discard_res_max_use_num;
	_u32 _discard_res_use_limit;//when res num is too little, using discard res
	
	//for connect filter manager
	_u32 _filt_max_speed;
	_u32 _filt_speed;
	_u32 _filt_max_speed_time;
    _u32 _pipe_speed_test_time;
    _u32 _pipe_retry_interval_time;
    

    _u32 _task_speed_filter_ratio;//the speed low bar used in filter low speed resource(refer to total speed)
	_u32 _pipe_low_speed_filter;//if system has idle resource, filter the low speed pipe.

    _u32 _pipes_num_low_limit;
    
	_u32 _need_idle_server_res_num;
	_u32 _need_idle_peer_res_num;	
	_u32 _need_retry_peer_res_num;	
	
	_u32 _refuse_more_res_speed_limit;//when speed attack 2M/s, refuse more speed.

	//for global connect manager
	_u32 _global_normal_speed_ratio;// 20%(+-)
	_u32 _global_task_test_time;
	_u32 _global_dispatch_period;
	_u32 _global_max_pipe_num;
	
	_u32 _global_test_speed_pipe_num;
	
	_u32 _global_max_filter_pipe_num;
	_u32 _global_pipes_num_low_limit;	
	_u32 _global_max_connecting_pipe_num;
	BOOL _is_use_global_strategy;

	//for choke pipe
	_u32 _choke_res_level_standard;
	_u32 _choke_res_time_span;
	_u32 _excellent_choke_res_speed;
	_u32 _choke_res_speed_span;	
	
	_u32 _max_res_num;

	_u32 _min_res_num;
	BOOL _is_slow_speed_core;
	_u32 _max_idle_core_ticks;
	_u32 _magnet_max_pipe_num;
}CONNECT_MANAGER_SETTING;

void cm_init_connect_manager_setting(void);
void cm_uninit_connect_manager_setting(void);

_u32 cm_magnet_max_pipe_num(void); 

_u32 cm_max_pipe_num(void); 
_u32 cm_max_server_pipe_num(void); 
_u32 cm_max_peer_pipe_num(void); 
_u32 cm_max_pipe_num_each_server(void); 
_u32 cm_max_connecting_pipe_num(void); 
_u32 cm_max_connecting_server_pipe_num(void); 
_u32 cm_max_connecting_peer_pipe_num(void); 
_u32 cm_max_res_retry_times(void);
_u32 cm_retry_res_init_score(void);  
_u32 cm_retry_res_score_ratio(void);  

_u32 cm_max_orgin_res_retry_times(void);
_u32 cm_status_idle_ticks(void); 
BOOL cm_is_enable_peer_download(void); 
BOOL cm_is_enable_server_download(void); 
BOOL cm_is_enable_http_download(void); 
BOOL cm_is_enable_ftp_download(void); 
BOOL cm_is_enable_p2p_download(void); 
BOOL cm_is_enable_bt_download(void);  

BOOL cm_is_enable_p2p_tcp(void);
BOOL cm_is_enable_p2p_same_nat(void);
BOOL cm_is_enable_p2p_tcp_broker(void);
BOOL cm_is_enable_p2p_udp_broker(void);

BOOL cm_is_enable_p2p_udt(void);
BOOL cm_is_enable_punch_hole(void);  

BOOL cm_is_only_using_origin_server(void);
_u32 cm_clear_hash_map_speed_ratio(void);  
_u32 cm_clear_hash_map_ticks(void);  
_u32 cm_discard_res_max_use_num(void); 
_u32 cm_discard_res_use_limit(void); 


//for connect filter manager
_u32 cm_filt_max_speed(void);
_u32 cm_filt_speed(void);

_u32 cm_filt_max_speed_time(void);
_u32 cm_pipe_speed_test_time(void);
_u32 cm_pipe_retry_interval_time(void);

_u32 cm_task_speed_filter_ratio(void); 
_u32 cm_pipe_low_speed_filter(void);
_u32 cm_pipes_num_low_limit(void); 
_u32 cm_need_idle_server_res_num(void);
_u32 cm_need_idle_peer_res_num(void);
_u32 cm_need_retry_peer_res_num(void);
_u32 cm_refuse_more_res_speed_limit(void);
_u32 cm_global_normal_speed_ratio(void);
_u32 cm_global_task_test_time(void); 
_u32 cm_global_dispatch_period(void); 
_u32 cm_global_max_pipe_num(void);  
_u32 cm_global_test_speed_pipe_num(void);  
_u32 cm_global_max_filter_pipe_num(void);  

_u32 cm_global_pipes_num_low_limit(void); 

_u32 cm_global_max_connecting_pipe_num(void);  
BOOL cm_is_use_global_strategy(void);  
_u32 cm_choke_res_level_standard(void);  
_u32 cm_choke_res_time_span(void); 
_u32 cm_choke_res_speed_span(void);  
_u32 cm_excellent_choke_res_speed(void);  

void cm_enable_p2s_download( BOOL is_enable );
void cm_enable_p2p_download( BOOL is_enable );
void cm_set_settings_max_pipe( _u32 pipe_num  );

_u32 cm_get_settings_max_pipe(void);

void cm_set_normal_dispatch_pipe( _u32 pipe_num  );

_u32 cm_max_res_num(void); 
_u32 cm_min_res_num(void);  

BOOL cm_is_slow_speed_core(void);  
_u32 cm_max_idle_core_ticks(void);  

#ifdef __cplusplus
}
#endif

#endif // !defined(__CONNECT_MANAGER_SETTING_20080828)

