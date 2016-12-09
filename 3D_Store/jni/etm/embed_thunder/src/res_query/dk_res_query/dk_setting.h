/*****************************************************************************
 *
 * Filename: 			dk_setting.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2009/10/18
 *	
 * Purpose:				Contain the dht and kad control parameters
 *
 *****************************************************************************/
	 

#if !defined(__DK_SETTING_20091018)
#define __DK_SETTING_20091018

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#ifdef _DK_QUERY
#include "utility/define_const_num.h"

typedef struct tagDK_SETTING 
{
	//time
	
	_u32 _time_out_interval;
	_u32 _ping_tick_cycle;
	
	_u32 _find_empty_bucket_cycle;
	_u32 _empty_bucket_find_node_max_num;
	
	_u32 _node_max_old_times;
	_u32 _node_old_time;
	_u32 _once_ping_num;
	_u32 _max_wait_ping_num;
	_u32 _can_split_max_distance;

	_u32 _min_node_num;
	_u32 _res_query_interval;
	_u32 _once_find_node_num;
	_u32 _find_node_from_rt_low_limit;
	_u32 _find_node_idle_count;
	_u32 _find_node_retry_times;
    _u32 _manager_idle_count;
    _u32 _root_node_interval;
    _u32 _ping_root_node_max_times;
    
    _u32 _avg_node_num;
    
    _u32 _socket_packet_max_num;
    _u32 _find_node_max_num;
    _u32 _filter_cycle;


	//for dht
	_u32 _dht_bucket_k;
	_u32 _dht_bucket_max_level;
	_u32 _dht_bucket_min_level;
	char _dht_cfg_path[MAX_FILE_PATH_LEN];
	_u32 _dht_peer_save_num;
	_u32 _dht_udp_port;
	char _dht_boot_host[MAX_URL_LEN];
	_u32 _dht_get_nearest_node_max_num;
	_u32 _dht_node_filter_low_limit;
    
	//for kad
	_u32 _kad_bucket_k;
	_u32 _kad_bucket_max_level;	
	_u32 _kad_bucket_min_level;	
	char _kad_cfg_path[MAX_FILE_PATH_LEN];
	_u32 _kad_peer_save_num;
	_u32 _kad_udp_port;
	_u32 _kad_get_nearest_node_max_num;
	_u32 _kad_node_filter_low_limit;
}DK_SETTING ;

_int32 dk_setting_init( void );
_int32 dk_setting_uninit( void );

_u32 dk_time_out_interval( void );
_u32 dk_get_ping_tick_cycle( void );
_u32 dk_find_empty_bucket_cycle( void );
_u32 dk_empty_bucket_find_node_max_num( void );

_u32 dk_node_old_time( void );
_u32 dk_node_max_old_times( void );

_u32 dk_once_ping_num( void );
_u32 dk_max_wait_ping_num( void );
_u32 dk_once_get_peers_num( void );
_u32 dk_can_split_max_distance( void );
_u32 dk_min_node_num( void );

_u32 dk_res_query_interval( void );
_u32 dk_once_find_node_num( void );
_u32 dk_find_node_from_rt_low_limit( void );
_u32 dk_find_node_idle_count( void );
_u32 dk_find_node_retry_times( void );
_u32 dk_manager_idle_count( void );
_u32 dk_root_node_interval( void );
_u32 dk_ping_root_node_max_times( void );
_u32 dk_avg_node_num( void );
_u32 dk_socket_packet_max_num( void );
_u32 dk_find_node_max_num( void );
_u32 dk_filter_cycle( void );

//for dht
_u32 dht_bucket_k( void );
_u32 dht_bucket_max_level( void );
_u32 dht_bucket_min_level( void );
char *dht_cfg_path( void );
_int32 dht_set_cfg_path(const char * path );
_u32 dht_peer_save_num( void );
_u16 dht_udp_port( void );
char *dht_boot_host( void );
_u32 dht_get_nearest_node_max_num( void );
_u32 dht_node_filter_low_limit( void );

//for kad
_u32 kad_bucket_k( void );
_u32 kad_bucket_max_level( void );
_u32 kad_bucket_min_level( void );
char *kad_cfg_path( void );
_int32 kad_set_cfg_path(const char * path );
_u32 kad_peer_save_num( void );
_u16 kad_udp_port( void );
_u32 kad_get_nearest_node_max_num( void );
_u32 kad_node_filter_low_limit( void );
#endif


#ifdef __cplusplus
}
#endif

#endif // !defined(__DK_SETTING_20091018)


