#include "utility/define.h"
#ifdef _DK_QUERY

#include "dk_setting.h"
#include "utility/settings.h"
#include "utility/mempool.h"

#ifdef ENABLE_ETM_MEDIA_CENTER
#define DHT_CFG_FULL_PATH EMBED_THUNDER_PERMANENT_DIR"dht.cfg"
#define KAD_CFG_FULL_PATH EMBED_THUNDER_PERMANENT_DIR"kad.cfg"
#else
#define DHT_CFG_FULL_PATH ("./dht.cfg")
#define KAD_CFG_FULL_PATH ("./kad.cfg")
#endif

#define DHT_BOOT_HOST ("btrouter.sandai.net:6881;router.bittorrent.com:6881;router.utorrent.com:6881;")

static DK_SETTING *g_dk_setting_ptr = NULL;

_int32 dk_setting_init( void )
{
	_int32 ret_val = SUCCESS;
	
	ret_val = sd_malloc( sizeof(DK_SETTING), (void **)&g_dk_setting_ptr );
	CHECK_VALUE( ret_val );
	
	g_dk_setting_ptr->_time_out_interval = 5000;
	settings_get_int_item( "dk_setting._time_out_interval", (_int32*)&(g_dk_setting_ptr->_time_out_interval) );

	g_dk_setting_ptr->_ping_tick_cycle = 12;
	settings_get_int_item( "dk_setting._ping_tick_cycle", (_int32*)&(g_dk_setting_ptr->_ping_tick_cycle) );

	g_dk_setting_ptr->_find_empty_bucket_cycle = 5;
	settings_get_int_item( "dk_setting._find_empty_bucket_cycle", (_int32*)&(g_dk_setting_ptr->_find_empty_bucket_cycle) );

	g_dk_setting_ptr->_empty_bucket_find_node_max_num = 3;
	settings_get_int_item( "dk_setting._empty_bucket_find_node_max_num", (_int32*)&(g_dk_setting_ptr->_empty_bucket_find_node_max_num) );

	g_dk_setting_ptr->_node_max_old_times = 3;
	settings_get_int_item( "dk_setting._node_max_old_time", (_int32*)&(g_dk_setting_ptr->_node_max_old_times) );

	g_dk_setting_ptr->_node_old_time = 2 * g_dk_setting_ptr->_ping_tick_cycle * g_dk_setting_ptr->_time_out_interval / 1000;
	//settings_get_int_item( "dk_setting._node_old_span_time", (_int32*)&(g_dk_setting_ptr->_node_old_time) );

	g_dk_setting_ptr->_once_ping_num = 20;
	settings_get_int_item( "dk_setting._once_ping_num", (_int32*)&(g_dk_setting_ptr->_once_ping_num) );

    g_dk_setting_ptr->_max_wait_ping_num = 150;
	settings_get_int_item( "dk_setting._max_wait_ping_num", (_int32*)&(g_dk_setting_ptr->_max_wait_ping_num) );

	g_dk_setting_ptr->_can_split_max_distance = 1;
	settings_get_int_item( "dk_setting._can_split_max_distance", (_int32*)&(g_dk_setting_ptr->_can_split_max_distance) );

	g_dk_setting_ptr->_min_node_num = 2;
	settings_get_int_item( "dk_setting._ping_inited_node_num", (_int32*)&(g_dk_setting_ptr->_min_node_num) );

	g_dk_setting_ptr->_res_query_interval = 360;
	settings_get_int_item( "dk_setting._res_query_interval", (_int32*)&(g_dk_setting_ptr->_res_query_interval) );

	g_dk_setting_ptr->_once_find_node_num = 30;
	settings_get_int_item( "dk_setting._once_find_node_num", (_int32*)&(g_dk_setting_ptr->_once_find_node_num) );

    g_dk_setting_ptr->_find_node_from_rt_low_limit = 16;
	settings_get_int_item( "dk_setting._find_node_from_rt_low_limit", (_int32*)&(g_dk_setting_ptr->_find_node_from_rt_low_limit) );

	g_dk_setting_ptr->_find_node_idle_count = 12;
	settings_get_int_item( "dk_setting._find_node_idle_count", (_int32*)&(g_dk_setting_ptr->_find_node_idle_count) );

    g_dk_setting_ptr->_find_node_retry_times = 4;
	settings_get_int_item( "dk_setting._find_node_retry_times", (_int32*)&(g_dk_setting_ptr->_find_node_retry_times) );

    g_dk_setting_ptr->_manager_idle_count = 60;
	settings_get_int_item( "dk_setting._manager_idle_count", (_int32*)&(g_dk_setting_ptr->_manager_idle_count) );

    g_dk_setting_ptr->_root_node_interval = 24;
	settings_get_int_item( "dk_setting._root_node_interval", (_int32*)&(g_dk_setting_ptr->_root_node_interval) );

    g_dk_setting_ptr->_ping_root_node_max_times = 6;
	settings_get_int_item( "dk_setting._ping_root_node_max_times", (_int32*)&(g_dk_setting_ptr->_ping_root_node_max_times) );

    g_dk_setting_ptr->_avg_node_num = 128;
	settings_get_int_item( "dk_setting._avg_node_num", (_int32*)&(g_dk_setting_ptr->_avg_node_num) );

    g_dk_setting_ptr->_socket_packet_max_num = 50;
	settings_get_int_item( "dk_setting._socket_packet_max_num", (_int32*)&(g_dk_setting_ptr->_socket_packet_max_num) );

    g_dk_setting_ptr->_find_node_max_num = 600;
     settings_get_int_item( "dk_setting._find_node_max_num", (_int32*)&(g_dk_setting_ptr->_find_node_max_num) );

    g_dk_setting_ptr->_filter_cycle = 40;
    settings_get_int_item( "dk_setting._filter_cycle", (_int32*)&(g_dk_setting_ptr->_filter_cycle) );

	//for dht
	g_dk_setting_ptr->_dht_bucket_k = 8;
	settings_get_int_item( "dk_setting._dht_bucket_k", (_int32*)&(g_dk_setting_ptr->_dht_bucket_k) );
	
	g_dk_setting_ptr->_dht_bucket_max_level = 160;
	settings_get_int_item( "dk_setting._dht_bucket_max_level", (_int32*)&(g_dk_setting_ptr->_dht_bucket_max_level) );

    g_dk_setting_ptr->_dht_bucket_min_level = 4;
	settings_get_int_item( "dk_setting._dht_bucket_min_level", (_int32*)&(g_dk_setting_ptr->_dht_bucket_min_level) );

	sd_memset( g_dk_setting_ptr->_dht_cfg_path, 0, MAX_FILE_PATH_LEN );
	sd_memcpy( g_dk_setting_ptr->_dht_cfg_path, DHT_CFG_FULL_PATH, MAX_FILE_PATH_LEN );
	settings_get_str_item( "dk_setting._dht_cfg_path", g_dk_setting_ptr->_dht_cfg_path );

	g_dk_setting_ptr->_dht_peer_save_num = 512;
	settings_get_int_item( "dk_setting._dht_peer_save_num", (_int32*)&(g_dk_setting_ptr->_dht_peer_save_num) );

	g_dk_setting_ptr->_dht_udp_port = 666;
	settings_get_int_item( "dk_setting._dht_udp_port", (_int32*)&(g_dk_setting_ptr->_dht_udp_port) );

	sd_memset( g_dk_setting_ptr->_dht_boot_host, 0, MAX_URL_LEN );
	sd_memcpy( g_dk_setting_ptr->_dht_boot_host, DHT_BOOT_HOST, MAX_URL_LEN );

    g_dk_setting_ptr->_dht_get_nearest_node_max_num = 8;
	settings_get_int_item( "dk_setting._dht_get_nearest_node_max_num", (_int32*)&(g_dk_setting_ptr->_dht_get_nearest_node_max_num) );

    g_dk_setting_ptr->_dht_node_filter_low_limit = 16;
	settings_get_int_item( "dk_setting._dht_node_filter_low_limit", (_int32*)&(g_dk_setting_ptr->_dht_node_filter_low_limit) );

	//for kad
	g_dk_setting_ptr->_kad_bucket_k = 20;
	settings_get_int_item( "dk_setting._kad_bucket_k", (_int32*)&(g_dk_setting_ptr->_kad_bucket_k) );

    g_dk_setting_ptr->_kad_bucket_max_level = 128;
	settings_get_int_item( "dk_setting._kad_bucket_max_level", (_int32*)&(g_dk_setting_ptr->_kad_bucket_max_level) );

    g_dk_setting_ptr->_kad_bucket_min_level = 4;
	settings_get_int_item( "dk_setting._kad_bucket_min_level", (_int32*)&(g_dk_setting_ptr->_kad_bucket_min_level) );

	sd_memset( g_dk_setting_ptr->_kad_cfg_path, 0, MAX_FILE_PATH_LEN );
	sd_memcpy( g_dk_setting_ptr->_kad_cfg_path, KAD_CFG_FULL_PATH, MAX_FILE_PATH_LEN );
	settings_get_str_item( "dk_setting._kad_cfg_path", g_dk_setting_ptr->_kad_cfg_path );

	g_dk_setting_ptr->_kad_peer_save_num = 512;
	settings_get_int_item( "dk_setting._kad_peer_save_num", (_int32*)&(g_dk_setting_ptr->_kad_peer_save_num) );

	g_dk_setting_ptr->_kad_udp_port = 888;
	settings_get_int_item( "dk_setting._kad_udp_port", (_int32*)&(g_dk_setting_ptr->_kad_udp_port) );

    g_dk_setting_ptr->_kad_get_nearest_node_max_num = 50;
	settings_get_int_item( "dk_setting._kad_get_nearest_node_max_num", (_int32*)&(g_dk_setting_ptr->_kad_get_nearest_node_max_num) );

    g_dk_setting_ptr->_kad_node_filter_low_limit = 40;
	settings_get_int_item( "dk_setting._kad_node_filter_low_limit", (_int32*)&(g_dk_setting_ptr->_kad_node_filter_low_limit) );

	return SUCCESS;
}

_int32 dk_setting_uninit( void )
{
	SAFE_DELETE( g_dk_setting_ptr );
	return SUCCESS;
}

_u32 dk_time_out_interval( void )
{
	return g_dk_setting_ptr->_time_out_interval;
}	

_u32 dk_get_ping_tick_cycle( void )
{
	return g_dk_setting_ptr->_ping_tick_cycle;
}	

_u32 dk_find_empty_bucket_cycle( void )
{
	return g_dk_setting_ptr->_find_empty_bucket_cycle;
}


_u32 dk_empty_bucket_find_node_max_num( void )
{
	return g_dk_setting_ptr->_empty_bucket_find_node_max_num;
}

_u32 dk_node_max_old_times( void )
{
	return g_dk_setting_ptr->_node_max_old_times;
}	

_u32 dk_node_old_time( void )
{
	return g_dk_setting_ptr->_node_old_time;
}	

_u32 dk_once_ping_num( void )
{
	return g_dk_setting_ptr->_once_ping_num;
}	

_u32 dk_max_wait_ping_num( void )
{
	return g_dk_setting_ptr->_max_wait_ping_num;
}	

_u32 dk_can_split_max_distance( void )
{
	return g_dk_setting_ptr->_can_split_max_distance;
}	

_u32 dk_min_node_num( void )
{
	return g_dk_setting_ptr->_min_node_num;
}	

_u32 dk_res_query_interval( void )
{
	return g_dk_setting_ptr->_res_query_interval;
}	

_u32 dk_once_find_node_num( void )
{
	return g_dk_setting_ptr->_once_find_node_num;
}	

_u32 dk_find_node_from_rt_low_limit( void )
{
	return g_dk_setting_ptr->_find_node_from_rt_low_limit;
}	

_u32 dk_find_node_idle_count( void )
{
	return g_dk_setting_ptr->_find_node_idle_count;
}	

_u32 dk_find_node_retry_times( void )
{
	return g_dk_setting_ptr->_find_node_retry_times;
}	

_u32 dk_manager_idle_count( void )
{
	return g_dk_setting_ptr->_manager_idle_count;
}	

_u32 dk_root_node_interval( void )
{
	return g_dk_setting_ptr->_root_node_interval;
}	

_u32 dk_ping_root_node_max_times( void )
{
	return g_dk_setting_ptr->_ping_root_node_max_times;
}	

_u32 dk_avg_node_num( void )
{
	return g_dk_setting_ptr->_avg_node_num;
}

_u32 dk_socket_packet_max_num( void )
{
	return g_dk_setting_ptr->_socket_packet_max_num;
}

_u32 dk_find_node_max_num( void )
{
	return g_dk_setting_ptr->_find_node_max_num;
}

_u32 dk_filter_cycle( void )
{
	return g_dk_setting_ptr->_filter_cycle;
}

//for dht
_u32 dht_bucket_k( void )
{
	return g_dk_setting_ptr->_dht_bucket_k;
}	

_u32 dht_bucket_max_level( void )
{
	return g_dk_setting_ptr->_dht_bucket_max_level;
}

_u32 dht_bucket_min_level( void )
{
	return g_dk_setting_ptr->_dht_bucket_min_level;
}	

char *dht_cfg_path( void )
{
	return g_dk_setting_ptr->_dht_cfg_path;
}	
_int32 dht_set_cfg_path(const char * path )
{
	if(MAX_FILE_PATH_LEN<=sd_strlen(path)) return -1;
	if(!g_dk_setting_ptr) return -1;
	sd_strncpy(g_dk_setting_ptr->_dht_cfg_path,path, MAX_FILE_PATH_LEN-1);
	sd_strcat(g_dk_setting_ptr->_dht_cfg_path,"/dht.cfg", sd_strlen("/dht.cfg"));
	settings_set_str_item( "dk_setting._dht_cfg_path", g_dk_setting_ptr->_dht_cfg_path );
	return SUCCESS;
}	

_u32 dht_peer_save_num( void )
{
	return g_dk_setting_ptr->_dht_peer_save_num;
}	

_u16 dht_udp_port( void )
{
	return (_u16)g_dk_setting_ptr->_dht_udp_port;
}	

char *dht_boot_host( void )
{
	return g_dk_setting_ptr->_dht_boot_host;
}	

_u32 dht_get_nearest_node_max_num( void )
{
	return g_dk_setting_ptr->_dht_get_nearest_node_max_num;
}	

_u32 dht_node_filter_low_limit( void )
{
	return g_dk_setting_ptr->_dht_node_filter_low_limit;
}	

//for kad
_u32 kad_bucket_k( void )
{
	return g_dk_setting_ptr->_kad_bucket_k;
}	

_u32 kad_bucket_max_level( void )
{
	return g_dk_setting_ptr->_kad_bucket_max_level;
}

_u32 kad_bucket_min_level( void )
{
	return g_dk_setting_ptr->_kad_bucket_min_level;
}	

char *kad_cfg_path( void )
{
	return g_dk_setting_ptr->_kad_cfg_path;
}	
_int32 kad_set_cfg_path(const char * path )
{
	if(MAX_FILE_PATH_LEN<=sd_strlen(path)) return -1;
	if(!g_dk_setting_ptr) return -1;
	sd_strncpy(g_dk_setting_ptr->_kad_cfg_path,path, MAX_FILE_PATH_LEN-1);
	sd_strcat(g_dk_setting_ptr->_kad_cfg_path,"/kad.cfg", sd_strlen("/kad.cfg"));
	settings_set_str_item( "dk_setting._kad_cfg_path", g_dk_setting_ptr->_kad_cfg_path );
	return SUCCESS;
}	

_u32 kad_peer_save_num( void )
{
	return g_dk_setting_ptr->_kad_peer_save_num;
}	

_u16 kad_udp_port( void )
{
	return (_u16)g_dk_setting_ptr->_kad_udp_port;
}	

_u32 kad_get_nearest_node_max_num( void )
{
	return g_dk_setting_ptr->_kad_get_nearest_node_max_num;
}	

_u32 kad_node_filter_low_limit( void )
{
	return g_dk_setting_ptr->_kad_node_filter_low_limit;
}	

#endif

