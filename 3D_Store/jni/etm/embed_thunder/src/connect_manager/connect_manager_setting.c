#include "utility/settings.h"

#include "utility/logid.h"
#define LOGID LOGID_CONNECT_MANAGER
#include "utility/logger.h"
#include "utility/utility.h"
#include "platform/sd_network.h"

#include "connect_manager_setting.h"

static CONNECT_MANAGER_SETTING g_connect_manager_setting;

//NOTE:jieouy这里有大量的magic number和可以服用的字符串。
void cm_init_connect_manager_setting(void)
{
#ifdef MOBILE_PHONE
	_int32 p2p_mode = 0;
#endif /* MOBILE_PHONE */

	LOG_DEBUG("cm_init_connect_manager_setting.");
	
	g_connect_manager_setting._max_pipe_num = 15;
	settings_get_int_item("connect_manager.max_pipe_num", (_int32*)&(g_connect_manager_setting._max_pipe_num));

	g_connect_manager_setting._max_server_pipe_num = 15;
	settings_get_int_item("connect_manager.max_server_pipe_num", (_int32*)&(g_connect_manager_setting._max_server_pipe_num));

	g_connect_manager_setting._max_peer_pipe_num = 15;
	settings_get_int_item("connect_manager.max_peer_pipe_num", (_int32*)&(g_connect_manager_setting._max_peer_pipe_num));

	g_connect_manager_setting._max_pipe_num_each_server = 5;
	settings_get_int_item("connect_manager.max_pipe_num_each_server", (_int32*)&(g_connect_manager_setting._max_pipe_num_each_server));

	g_connect_manager_setting._max_connecting_pipe_num = 15;
	settings_get_int_item("connect_manager.max_connecting_num", (_int32*)&(g_connect_manager_setting._max_connecting_pipe_num));

	g_connect_manager_setting._max_connecting_server_pipe_num = 15;
	settings_get_int_item("connect_manager.max_connecting_server_pipe_num", (_int32*)&(g_connect_manager_setting._max_connecting_server_pipe_num));

	g_connect_manager_setting._max_connecting_peer_pipe_num = 15;
	settings_get_int_item("connect_manager.max_connecting_peer_pipe_num", (_int32*)&(g_connect_manager_setting._max_connecting_peer_pipe_num));

	g_connect_manager_setting._max_res_retry_times = 3;
	settings_get_int_item("connect_manager.max_res_retry_times", (_int32*)&(g_connect_manager_setting._max_res_retry_times));

	g_connect_manager_setting._retry_res_init_score = 2000;
	settings_get_int_item("connect_manager.retry_res_init_score", (_int32*)&(g_connect_manager_setting._retry_res_init_score));

	g_connect_manager_setting._retry_res_score_ratio = 2 * 1024;
	settings_get_int_item("connect_manager.retry_res_score_ratio", (_int32*)&(g_connect_manager_setting._retry_res_score_ratio));

	g_connect_manager_setting._max_orgin_res_retry_times = 10;
	settings_get_int_item("connect_manager.max_orgin_res_retry_times", (_int32*)&(g_connect_manager_setting._max_orgin_res_retry_times));

	g_connect_manager_setting._status_idle_ticks = 60;
	settings_get_int_item("connect_manager.status_idle_ticks", (_int32*)&(g_connect_manager_setting._status_idle_ticks));

	g_connect_manager_setting._is_enable_peer_download = FALSE;
	settings_get_bool_item("connect_manager.enable_peer_download", &(g_connect_manager_setting._is_enable_peer_download));

	g_connect_manager_setting._is_enable_server_download = TRUE;
	settings_get_bool_item("connect_manager.enable_server_download", &(g_connect_manager_setting._is_enable_server_download));

	g_connect_manager_setting._is_enable_http_download = TRUE;
	settings_get_bool_item("connect_manager.enable_http_download", &(g_connect_manager_setting._is_enable_http_download));

	g_connect_manager_setting._is_enable_ftp_download = TRUE;
	settings_get_bool_item("connect_manager.enable_ftp_download", &(g_connect_manager_setting._is_enable_ftp_download));

	g_connect_manager_setting._is_enable_p2p_download = FALSE;
	settings_get_bool_item("connect_manager.enable_p2p_download", &(g_connect_manager_setting._is_enable_p2p_download));
	
#ifdef MOBILE_PHONE
	settings_get_int_item("system.p2p_mode", (_int32*)&p2p_mode);
	if (p2p_mode == 1)
	{
		/* 只在WIFI网络下打开 */
		if (!(NT_WLAN & sd_get_net_type()))
		{
			g_connect_manager_setting._is_enable_p2p_download = FALSE;
		}
	}
	else if (p2p_mode == 2)
	{
		g_connect_manager_setting._is_enable_p2p_download = FALSE;
	}
#endif /* MOBILE_PHONE */

	g_connect_manager_setting._is_enable_bt_download = FALSE;
	settings_get_bool_item("connect_manager.enable_bt_download", &(g_connect_manager_setting._is_enable_bt_download));

	g_connect_manager_setting._is_enable_p2p_tcp = FALSE;
	settings_get_bool_item("connect_manager.enable_p2p_tcp", &(g_connect_manager_setting._is_enable_p2p_tcp));

	g_connect_manager_setting._is_enable_p2p_same_nat = FALSE;
	settings_get_bool_item("connect_manager.enable_p2p_same_nat", &(g_connect_manager_setting._is_enable_p2p_same_nat));

	g_connect_manager_setting._is_enable_p2p_tcp_broker = FALSE;
	settings_get_bool_item("connect_manager.enable_p2p_tcp_broker", &(g_connect_manager_setting._is_enable_p2p_tcp_broker));

	g_connect_manager_setting._is_enable_p2p_udp_broker = FALSE;
	settings_get_bool_item("connect_manager.enable_p2p_udp_broker", &(g_connect_manager_setting._is_enable_p2p_udp_broker));

	g_connect_manager_setting._is_enable_p2p_udt = FALSE;
	settings_get_bool_item("connect_manager.enable_p2p_udt", &(g_connect_manager_setting._is_enable_p2p_udt));

	g_connect_manager_setting._is_enable_punch_hole = FALSE;
	settings_get_bool_item("connect_manager.enable_p2p_punch_hole", &(g_connect_manager_setting._is_enable_punch_hole));

	g_connect_manager_setting._is_only_using_origin_server = FALSE;
	settings_get_bool_item( "connect_manager.is_only_using_origin_server", &(g_connect_manager_setting._is_only_using_origin_server) );

	g_connect_manager_setting._clear_hash_map_speed_ratio = 4;
	settings_get_int_item( "connect_manager.clear_hash_map_speed_ratio", (_int32*)&(g_connect_manager_setting._clear_hash_map_speed_ratio) );

	g_connect_manager_setting._clear_hash_map_ticks = 10;
	settings_get_int_item( "connect_manager.clear_hash_map_ticks", (_int32*)&(g_connect_manager_setting._clear_hash_map_ticks) );

	g_connect_manager_setting._discard_res_max_use_num = 3;
	settings_get_int_item( "connect_manager.discard_res_max_use_num", (_int32*)&(g_connect_manager_setting._discard_res_max_use_num) );

	g_connect_manager_setting._discard_res_use_limit = 10;
	settings_get_int_item( "connect_manager.discard_res_use_limit", (_int32*)&(g_connect_manager_setting._discard_res_use_limit) );

	g_connect_manager_setting._filt_max_speed = 10 * 1024;
	settings_get_int_item( "connect_manager.filt_max_speed", (_int32*)&(g_connect_manager_setting._filt_max_speed) );

	g_connect_manager_setting._filt_speed = 40 * 1024;
	settings_get_int_item( "connect_manager.filt_speed", (_int32*)&(g_connect_manager_setting._filt_speed) );

	g_connect_manager_setting._filt_max_speed_time = 3;
	settings_get_int_item( "connect_manager.filt_max_speed_time", (_int32*)&(g_connect_manager_setting._filt_max_speed_time) );

    g_connect_manager_setting._pipe_speed_test_time = 6 * 1000;
	settings_get_int_item( "connect_manager.pipe_speed_test_time", (_int32*)&(g_connect_manager_setting._pipe_speed_test_time) );

    g_connect_manager_setting._pipe_retry_interval_time = 2; //s
	settings_get_int_item( "connect_manager.pipe_retry_interval", (_int32*)&(g_connect_manager_setting._pipe_retry_interval_time) );

	g_connect_manager_setting._task_speed_filter_ratio = 80;
	settings_get_int_item( "connect_manager.task_speed_filter_ratio", (_int32*)&(g_connect_manager_setting._task_speed_filter_ratio) );

	g_connect_manager_setting._pipe_low_speed_filter = 1024 * 4;
	settings_get_int_item( "connect_manager.pipe_low_speed_filter", (_int32*)&(g_connect_manager_setting._pipe_low_speed_filter) );

	g_connect_manager_setting._pipes_num_low_limit = 5;
	settings_get_int_item( "connect_manager.pipes_num_low_limit", (_int32*)&(g_connect_manager_setting._pipes_num_low_limit) );

	//auto adjust the filter
	g_connect_manager_setting._pipes_num_low_limit = MIN( g_connect_manager_setting._pipes_num_low_limit, g_connect_manager_setting._max_pipe_num/3 );
	g_connect_manager_setting._pipes_num_low_limit = MIN( g_connect_manager_setting._pipes_num_low_limit, g_connect_manager_setting._max_connecting_pipe_num/3 );

	g_connect_manager_setting._need_idle_server_res_num = 25;
	settings_get_int_item( "connect_manager.need_idle_server_res_num", (_int32*)&(g_connect_manager_setting._need_idle_server_res_num) );

	g_connect_manager_setting._need_idle_peer_res_num = 25;
	settings_get_int_item( "connect_manager.need_idle_peer_res_num", (_int32*)&(g_connect_manager_setting._need_idle_peer_res_num) );

	g_connect_manager_setting._need_retry_peer_res_num = 50;
	settings_get_int_item( "connect_manager.need_retry_peer_res_num", (_int32*)&(g_connect_manager_setting._need_retry_peer_res_num) );

	g_connect_manager_setting._refuse_more_res_speed_limit = 2 * 1024 * 1024;
	settings_get_int_item( "connect_manager.refuse_more_res_speed_limit", (_int32*)&(g_connect_manager_setting._refuse_more_res_speed_limit) );

	g_connect_manager_setting._global_normal_speed_ratio = 20;
	settings_get_int_item( "connect_manager.global_normal_speed_ratio", (_int32*)&(g_connect_manager_setting._global_normal_speed_ratio) );

	g_connect_manager_setting._global_task_test_time = 60 * 1000;
	settings_get_int_item( "connect_manager.global_task_test_time", (_int32*)&(g_connect_manager_setting._global_task_test_time) );

	g_connect_manager_setting._global_dispatch_period = 4;
	settings_get_int_item( "connect_manager.global_dispatch_period", (_int32*)&(g_connect_manager_setting._global_dispatch_period) );

	g_connect_manager_setting._global_max_pipe_num = CONNECT_MANAGE_DEFAULT_MAX_CONNECTIONS;
	settings_get_int_item( "connect_manager.global_max_pipe_num", (_int32*)&(g_connect_manager_setting._global_max_pipe_num) );

	g_connect_manager_setting._global_test_speed_pipe_num = 5;
	settings_get_int_item( "connect_manager.global_test_speed_pipe_num", (_int32*)&(g_connect_manager_setting._global_test_speed_pipe_num) );

	g_connect_manager_setting._global_max_filter_pipe_num = 15;
	settings_get_int_item( "connect_manager.global_max_filter_pipe_num", (_int32*)&(g_connect_manager_setting._global_max_filter_pipe_num) );
	g_connect_manager_setting._global_max_filter_pipe_num = MIN( g_connect_manager_setting._global_max_filter_pipe_num, g_connect_manager_setting._global_max_pipe_num /3 );


    g_connect_manager_setting._global_pipes_num_low_limit = 2;
	settings_get_int_item( "connect_manager.global_pipes_num_low_limit", (_int32*)&(g_connect_manager_setting._global_pipes_num_low_limit) );

	g_connect_manager_setting._global_max_connecting_pipe_num = 10;
	settings_get_int_item( "connect_manager.global_max_connecting_pipe_num", (_int32*)&(g_connect_manager_setting._global_max_connecting_pipe_num) );
#ifdef MACOS
	/* MAC迅雷不用全局调度 */
	g_connect_manager_setting._is_use_global_strategy = FALSE;
#else
	g_connect_manager_setting._is_use_global_strategy = TRUE;
#endif /* MACOS */
	settings_get_bool_item( "connect_manager.is_use_global_strategy", &g_connect_manager_setting._is_use_global_strategy );

	g_connect_manager_setting._choke_res_level_standard = 8;
	settings_get_int_item( "connect_manager.choke_res_level_standard", (_int32*)&g_connect_manager_setting._choke_res_level_standard );

	g_connect_manager_setting._choke_res_time_span = 20 * 1000;//20S
	settings_get_int_item( "connect_manager.choke_res_time_span", (_int32*)&g_connect_manager_setting._choke_res_time_span );

	g_connect_manager_setting._choke_res_speed_span = 10 * 1024;//10K
	settings_get_int_item( "connect_manager.choke_res_speed_span", (_int32*)&g_connect_manager_setting._choke_res_speed_span );

	g_connect_manager_setting._max_res_num = 400;
	settings_get_int_item( "connect_manager.max_res_num", (_int32*)&g_connect_manager_setting._max_res_num );

	g_connect_manager_setting._min_res_num = 20;
	settings_get_int_item( "connect_manager.min_res_num", (_int32*)&g_connect_manager_setting._min_res_num );

	g_connect_manager_setting._excellent_choke_res_speed = 30 * 1024;//30K
	settings_get_int_item( "connect_manager.excellent_choke_res_speed", (_int32*)&g_connect_manager_setting._excellent_choke_res_speed );

	g_connect_manager_setting._magnet_max_pipe_num = 3;
	settings_get_int_item( "connect_manager.magnet_max_pipe_num", (_int32*)&g_connect_manager_setting._magnet_max_pipe_num );


LOG_DEBUG("000 get_net_type = 0x%X, enable_punch_hole = %d"
    , sd_get_net_type(), g_connect_manager_setting._is_enable_punch_hole);

	
#if defined(MOBILE_PHONE)
	if(sd_get_net_type()<NT_3G)
	{

    
	LOG_DEBUG("111 get_net_type = 0x%X, enable_punch_hole = %d"
	    , sd_get_net_type(), g_connect_manager_setting._is_enable_punch_hole);
 
		g_connect_manager_setting._max_pipe_num = 6;

		g_connect_manager_setting._max_server_pipe_num = 6;

		g_connect_manager_setting._max_peer_pipe_num = 6;

		g_connect_manager_setting._is_use_global_strategy = FALSE;

		g_connect_manager_setting._max_connecting_pipe_num = 6;

		g_connect_manager_setting._max_connecting_server_pipe_num = 6;

		g_connect_manager_setting._max_connecting_peer_pipe_num = 6;

		g_connect_manager_setting._global_max_pipe_num = CONNECT_MANAGE_DEFAULT_MAX_CONNECTIONS_IN_GPRS;

		g_connect_manager_setting._global_test_speed_pipe_num = 1;

		g_connect_manager_setting._global_max_filter_pipe_num = CONNECT_MANAGE_DEFAULT_MAX_CONNECTIONS_IN_GPRS;

		g_connect_manager_setting._global_task_test_time = 600 * 1000;

        g_connect_manager_setting._pipe_speed_test_time = 200 * 1000;

		g_connect_manager_setting._filt_max_speed = 10 * 1024;

		g_connect_manager_setting._filt_speed = 2 * 1024;
		
		g_connect_manager_setting._refuse_more_res_speed_limit = 15 * 1024;
		
		g_connect_manager_setting._excellent_choke_res_speed = 6 * 1024;//6K
#ifdef KANKAN_PROJ
		g_connect_manager_setting._status_idle_ticks = 9999999;  // Kankan 永不退出 ??
#else
		g_connect_manager_setting._status_idle_ticks = 150;
#endif	
		g_connect_manager_setting._global_max_filter_pipe_num = MIN( g_connect_manager_setting._global_max_filter_pipe_num, g_connect_manager_setting._global_max_pipe_num /3 );
		g_connect_manager_setting._pipes_num_low_limit = MIN( g_connect_manager_setting._pipes_num_low_limit, g_connect_manager_setting._max_pipe_num/3 );
		g_connect_manager_setting._pipes_num_low_limit = MIN( g_connect_manager_setting._pipes_num_low_limit, g_connect_manager_setting._max_connecting_pipe_num/3 );


	LOG_DEBUG("222 get_net_type = 0x%X, enable_punch_hole = %d"
	    , sd_get_net_type(), g_connect_manager_setting._is_enable_punch_hole);

	
	}
	else
	{
		g_connect_manager_setting._max_pipe_num = 6;

		g_connect_manager_setting._max_server_pipe_num = 6;

		g_connect_manager_setting._max_peer_pipe_num = 6;

		g_connect_manager_setting._is_use_global_strategy = FALSE;

		g_connect_manager_setting._max_connecting_pipe_num = 6;

		g_connect_manager_setting._max_connecting_server_pipe_num = 6;

		g_connect_manager_setting._max_connecting_peer_pipe_num = 6;

		g_connect_manager_setting._global_max_pipe_num = CONNECT_MANAGE_DEFAULT_MAX_CONNECTIONS;

		g_connect_manager_setting._global_test_speed_pipe_num = 1;

		g_connect_manager_setting._global_max_filter_pipe_num = CONNECT_MANAGE_DEFAULT_MAX_CONNECTIONS;

#ifdef KANKAN_PROJ
    		g_connect_manager_setting._pipe_speed_test_time = 8*1000;
#endif

#ifdef KANKAN_PROJ
		g_connect_manager_setting._status_idle_ticks = 999999;  //iPan Kankan 永不退出 ??
#else
		g_connect_manager_setting._status_idle_ticks = 150;
#endif	
	
		g_connect_manager_setting._global_max_filter_pipe_num = MIN( g_connect_manager_setting._global_max_filter_pipe_num, g_connect_manager_setting._global_max_pipe_num /3 );
		g_connect_manager_setting._pipes_num_low_limit = MIN( g_connect_manager_setting._pipes_num_low_limit, g_connect_manager_setting._max_pipe_num/3 );
		g_connect_manager_setting._pipes_num_low_limit = MIN( g_connect_manager_setting._pipes_num_low_limit, g_connect_manager_setting._max_connecting_pipe_num/3 );
	}
#endif

	g_connect_manager_setting._is_slow_speed_core = FALSE;


	settings_get_bool_item( "connect_manager.is_slow_speed_core", &g_connect_manager_setting._is_slow_speed_core );

	g_connect_manager_setting._max_idle_core_ticks = 180;
	settings_get_int_item( "connect_manager.max_idle_core_ticks", (_int32*)&g_connect_manager_setting._max_idle_core_ticks );

	LOG_DEBUG("get_net_type = 0x%X, enable_punch_hole = %d"
	    , sd_get_net_type(), g_connect_manager_setting._is_enable_punch_hole);

}

void cm_uninit_connect_manager_setting(void)
{
	sd_memset(&g_connect_manager_setting, 0, sizeof(CONNECT_MANAGER_SETTING));
}

_u32 cm_max_pipe_num(void) 
{
	return g_connect_manager_setting._max_pipe_num;
}

_u32 cm_magnet_max_pipe_num(void) 
{
	return g_connect_manager_setting._magnet_max_pipe_num;
}

_u32 cm_max_server_pipe_num(void)  
{
	return g_connect_manager_setting._max_server_pipe_num;
}

_u32 cm_max_peer_pipe_num(void)  
{
	return g_connect_manager_setting._max_peer_pipe_num;
}

_u32 cm_max_pipe_num_each_server(void)  
{
	return g_connect_manager_setting._max_pipe_num_each_server;
}

_u32 cm_max_connecting_pipe_num(void)  
{
	return g_connect_manager_setting._max_connecting_pipe_num;
}

_u32 cm_max_connecting_server_pipe_num(void)  
{
	return g_connect_manager_setting._max_connecting_server_pipe_num;
}

_u32 cm_max_connecting_peer_pipe_num(void)  
{
	return g_connect_manager_setting._max_connecting_peer_pipe_num;
}

_u32 cm_max_res_retry_times(void)  
{
	return g_connect_manager_setting._max_res_retry_times;
}

_u32 cm_retry_res_init_score(void)  
{
	return g_connect_manager_setting._retry_res_init_score;
}

_u32 cm_retry_res_score_ratio(void)  
{
	return g_connect_manager_setting._retry_res_score_ratio;
}

_u32 cm_max_orgin_res_retry_times(void)  
{
	return g_connect_manager_setting._max_orgin_res_retry_times;
}


_u32 cm_status_idle_ticks(void)  
{
	return g_connect_manager_setting._status_idle_ticks;
}

BOOL cm_is_enable_peer_download(void)  
{
	return g_connect_manager_setting._is_enable_peer_download;
}

BOOL cm_is_enable_server_download(void)  
{
	return g_connect_manager_setting._is_enable_server_download;
}

BOOL cm_is_enable_http_download(void)  
{
	return g_connect_manager_setting._is_enable_http_download;
}

BOOL cm_is_enable_ftp_download(void)  
{
	return g_connect_manager_setting._is_enable_ftp_download;
}

BOOL cm_is_enable_p2p_download(void)  
{
	return g_connect_manager_setting._is_enable_p2p_download;
}

BOOL cm_is_enable_bt_download(void)  
{
	return g_connect_manager_setting._is_enable_bt_download;
}

BOOL cm_is_enable_p2p_tcp(void)  
{
	return g_connect_manager_setting._is_enable_p2p_tcp;
}
BOOL cm_is_enable_p2p_same_nat(void)  
{
	return g_connect_manager_setting._is_enable_p2p_same_nat;
}

BOOL cm_is_enable_p2p_tcp_broker(void)  
{
	return g_connect_manager_setting._is_enable_p2p_tcp_broker;
}

BOOL cm_is_enable_p2p_udp_broker(void)  
{
	return g_connect_manager_setting._is_enable_p2p_udp_broker;
}

BOOL cm_is_enable_p2p_udt(void)  
{
	return g_connect_manager_setting._is_enable_p2p_udt;
}

BOOL cm_is_enable_punch_hole(void)  
{
	return g_connect_manager_setting._is_enable_punch_hole;
}

BOOL cm_is_only_using_origin_server(void)  
{
	return g_connect_manager_setting._is_only_using_origin_server;
}

_u32 cm_clear_hash_map_speed_ratio(void)  
{
	return g_connect_manager_setting._clear_hash_map_speed_ratio;
}

_u32 cm_clear_hash_map_ticks(void)  
{
	return g_connect_manager_setting._clear_hash_map_ticks;
}

_u32 cm_discard_res_max_use_num(void)  
{
	return g_connect_manager_setting._discard_res_max_use_num;
}

_u32 cm_discard_res_use_limit(void)  
{
	return g_connect_manager_setting._discard_res_use_limit;
}

_u32 cm_filt_max_speed(void)
{
	return g_connect_manager_setting._filt_max_speed;
}

_u32 cm_filt_speed(void)
{
	return g_connect_manager_setting._filt_speed;
}

_u32 cm_filt_max_speed_time(void)
{
	return g_connect_manager_setting._filt_max_speed_time;
}
	
_u32 cm_pipe_speed_test_time(void)
{
    return g_connect_manager_setting._pipe_speed_test_time;
}

_u32 cm_pipe_retry_interval_time(void)
{
    return g_connect_manager_setting._pipe_retry_interval_time;
}

_u32 cm_task_speed_filter_ratio(void)  
{
	return g_connect_manager_setting._task_speed_filter_ratio;
}

_u32 cm_pipe_low_speed_filter(void)  
{
	return g_connect_manager_setting._pipe_low_speed_filter;
}

_u32 cm_pipes_num_low_limit(void)  
{
	return g_connect_manager_setting._pipes_num_low_limit;
}

_u32 cm_global_pipes_num_low_limit(void)  
{
	return g_connect_manager_setting._global_pipes_num_low_limit;
}

_u32 cm_need_idle_server_res_num(void)  
{
	return g_connect_manager_setting._need_idle_server_res_num;
}

_u32 cm_need_idle_peer_res_num(void)  
{
	return g_connect_manager_setting._need_idle_peer_res_num;
}

_u32 cm_need_retry_peer_res_num(void)  
{
	return g_connect_manager_setting._need_retry_peer_res_num;
}

_u32 cm_refuse_more_res_speed_limit(void)  
{
	return g_connect_manager_setting._refuse_more_res_speed_limit;
}

_u32 cm_global_normal_speed_ratio(void)  
{
	return g_connect_manager_setting._global_normal_speed_ratio;
}

_u32 cm_global_task_test_time(void)  
{
	return g_connect_manager_setting._global_task_test_time;
}

_u32 cm_global_dispatch_period(void)  
{
	return g_connect_manager_setting._global_dispatch_period;
}

_u32 cm_global_max_pipe_num(void)  
{
	return g_connect_manager_setting._global_max_pipe_num;
}

_u32 cm_global_test_speed_pipe_num(void)  
{
	return g_connect_manager_setting._global_test_speed_pipe_num;
}

_u32 cm_global_max_filter_pipe_num(void)  
{
	return g_connect_manager_setting._global_max_filter_pipe_num;
}

_u32 cm_global_max_connecting_pipe_num(void)  
{
	return g_connect_manager_setting._global_max_connecting_pipe_num;
}

BOOL cm_is_use_global_strategy(void)  
{
	return g_connect_manager_setting._is_use_global_strategy;
}

_u32 cm_choke_res_level_standard(void)  
{
	return g_connect_manager_setting._choke_res_level_standard;
}

_u32 cm_choke_res_time_span(void)  
{
	return g_connect_manager_setting._choke_res_time_span;
}

_u32 cm_max_res_num(void)  
{
	return g_connect_manager_setting._max_res_num;
}

_u32 cm_min_res_num(void)  
{
	return g_connect_manager_setting._min_res_num;
}

_u32 cm_choke_res_speed_span(void)  
{
	return g_connect_manager_setting._choke_res_speed_span;
}

_u32 cm_excellent_choke_res_speed(void)  
{
	return g_connect_manager_setting._excellent_choke_res_speed;
}


void cm_enable_p2s_download( BOOL is_enable )
{
	g_connect_manager_setting._is_enable_server_download = is_enable;
	settings_set_bool_item( "connect_manager.enable_server_download", is_enable );
}

void cm_enable_p2p_download( BOOL is_enable )
{
	g_connect_manager_setting._is_enable_p2p_download = is_enable;
	settings_set_bool_item( "connect_manager.enable_p2p_download", is_enable );
}

void cm_set_settings_max_pipe( _u32 pipe_num  )
{
	LOG_DEBUG( "cm_set_settings_max_pipe.pipe_num:%d", pipe_num );
	g_connect_manager_setting._max_pipe_num = pipe_num;
	settings_set_int_item( "connect_manager.max_pipe_num", (_int32)pipe_num );

	g_connect_manager_setting._max_server_pipe_num = pipe_num;
	settings_set_int_item( "connect_manager.max_server_pipe_num", (_int32)pipe_num );
	
	g_connect_manager_setting._max_peer_pipe_num = pipe_num;
	settings_set_int_item( "connect_manager.max_peer_pipe_num", (_int32)pipe_num );
	
	//auto adjust the filter
	g_connect_manager_setting._pipes_num_low_limit = MIN( g_connect_manager_setting._pipes_num_low_limit, g_connect_manager_setting._max_pipe_num/3 );
	g_connect_manager_setting._pipes_num_low_limit = MIN( g_connect_manager_setting._pipes_num_low_limit, g_connect_manager_setting._max_connecting_pipe_num/3 );

}

_u32 cm_get_settings_max_pipe(void)
{
	return g_connect_manager_setting._max_pipe_num;
}

void cm_set_normal_dispatch_pipe( _u32 pipe_num  )
{
	LOG_DEBUG( "cm_set_normal_dispatch_pipe.pipe_num:%d", pipe_num );
    
	g_connect_manager_setting._max_pipe_num = pipe_num;
	//settings_set_int_item( "connect_manager.max_pipe_num", (_int32)pipe_num );

	g_connect_manager_setting._max_server_pipe_num = pipe_num;
	//settings_set_int_item( "connect_manager.max_server_pipe_num", (_int32)pipe_num );
	
	g_connect_manager_setting._max_peer_pipe_num = pipe_num;
	//settings_set_int_item( "connect_manager.max_peer_pipe_num", (_int32)pipe_num );


    //connecting pipe num
	g_connect_manager_setting._max_connecting_pipe_num = pipe_num;
	//settings_set_int_item( "connect_manager.max_connecting_pipe_num", (_int32)pipe_num );

	g_connect_manager_setting._max_connecting_server_pipe_num = pipe_num;
	//settings_set_int_item( "connect_manager.max_connecting_server_pipe_num", (_int32)pipe_num );
	
	g_connect_manager_setting._max_connecting_peer_pipe_num = pipe_num;
	//settings_set_int_item( "connect_manager.max_connecting_peer_pipe_num", (_int32)pipe_num );
	

	//auto adjust the filter
	g_connect_manager_setting._pipes_num_low_limit = MIN( g_connect_manager_setting._pipes_num_low_limit, g_connect_manager_setting._max_pipe_num/3 );
	g_connect_manager_setting._pipes_num_low_limit = MIN( g_connect_manager_setting._pipes_num_low_limit, g_connect_manager_setting._max_connecting_pipe_num/3 );

}

BOOL cm_is_slow_speed_core(void)  
{
	return g_connect_manager_setting._is_slow_speed_core;
}

_u32 cm_max_idle_core_ticks(void)  
{
	return g_connect_manager_setting._max_idle_core_ticks;
}

