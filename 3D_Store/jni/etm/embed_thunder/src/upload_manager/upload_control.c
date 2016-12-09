#include "upload_control.h"
#ifdef UPLOAD_ENABLE

#include "utility/settings.h"
#include "utility/mempool.h"
#include "utility/string.h"
#include "utility/utility.h"
#include "utility/time.h"
#include "platform/sd_network.h"
#include "utility/sd_os.h"
#include "utility/logid.h"
#define LOGID LOGID_UPLOAD_MANAGER
#include "utility/logger.h"

#include "task_manager/task_manager.h"
#include "socket_proxy_interface.h"
#include "ulm_pipe_manager.h"
#include "socket_proxy_interface.h"
#include "p2p_transfer_layer/ptl_interface.h"

static ULM_CONTROLLER* g_ulm_controller = NULL;
static _u32 g_controller_timer = INVALID_MSG_ID;

static _int32 ulc_start_upload();
static _int32 ulc_stop_upload();
static _int32 ulc_handle_timeout(const MSG_INFO *msg_info
    , _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid);
static _int32 ulc_handle_score();

_int32 init_ulc()
{
    LOG_DEBUG("ulm_control, init_ulc enter");
    _int32 ret = SUCCESS;
    ret = sd_malloc(sizeof(ULM_CONTROLLER), (void**)&g_ulm_controller);
    CHECK_VALUE(ret);
    ret = sd_memset(g_ulm_controller, 0, sizeof(ULM_CONTROLLER) );
    CHECK_VALUE(ret);

#ifndef	MOBILE_PHONE 
	g_ulm_controller->_policy=1;
	
	g_ulm_controller->_speed_limit_in_charge = 40;
	g_ulm_controller->_speed_limit_in_lock = 0;
	g_ulm_controller->_speed_limit_in_download = 0;
	g_ulm_controller->_lower_limit_charge_from_cfg=0;

	g_ulm_controller->_is_in_charge = 1;
	g_ulm_controller->_now_charge = 100;
	g_ulm_controller->_is_in_lock = 0;

	settings_get_int_item("upload_manager_box.enable_upload", &g_ulm_controller->_enable_upload_from_cfg);
	settings_get_int_item("upload_manager_box.enable_upload_when_download", &g_ulm_controller->_enable_upload_when_download_from_cfg );
	settings_get_int_item("upload_manager_box.up_speed_limit_max", (_int32*)&g_ulm_controller->_speed_limit_in_charge);

	LOG_DEBUG("ulm_control, get_config enable_upload = %d, enable_upload_when_download = %d, \
		upload_manager_box.up_speed_limit_max = %d"
		, g_ulm_controller->_enable_upload_from_cfg 
		, g_ulm_controller->_enable_upload_when_download_from_cfg
		, g_ulm_controller->_speed_limit_in_charge
		);
#else	
	settings_get_int_item("upload_manager.enable_upload"
	            , &g_ulm_controller->_enable_upload_from_cfg );
    settings_get_int_item("upload_manager.enable_upload_when_download"
            , &g_ulm_controller->_enable_upload_when_download_from_cfg );

	g_ulm_controller->_policy=0;

    settings_get_int_item("upload_manager.speed_limit_in_charge"
            , (_int32*)&g_ulm_controller->_speed_limit_in_charge );
    settings_get_int_item("upload_manager.speed_limit_in_lock"
            , (_int32*)&g_ulm_controller->_speed_limit_in_lock );
    settings_get_int_item("upload_manager.speed_limit_in_download"
            , (_int32*)&g_ulm_controller->_speed_limit_in_download );
    settings_get_int_item("upload_manager.lower_limit_charge"
            , &g_ulm_controller->_lower_limit_charge_from_cfg);

    if(g_ulm_controller->_lower_limit_charge_from_cfg == 0)
    {
        g_ulm_controller->_lower_limit_charge_from_cfg = 10;
    }
    LOG_DEBUG("ulm_control, get_config enable_upload = %d, enable_upload_when_download = %d, \
        speed_limit_in_charge = %d, speed_limit_in_lock = %d, speed_limit_in_download = %d, \
        lower_limit_charge = %d"
        , g_ulm_controller->_enable_upload_from_cfg 
        , g_ulm_controller->_enable_upload_when_download_from_cfg
        , g_ulm_controller->_speed_limit_in_charge
        , g_ulm_controller->_speed_limit_in_lock
        , g_ulm_controller->_speed_limit_in_download
        , g_ulm_controller->_lower_limit_charge_from_cfg );
#endif
    if(g_controller_timer == INVALID_MSG_ID)
    {
        start_timer(ulc_handle_timeout, NOTICE_INFINITE, 5*1000, 0, NULL, &g_controller_timer);
    }
        
    return ret;
}

_int32 uninit_ulc()
{
    LOG_DEBUG("ulm_control, uninit_ulc enter");
    if(g_controller_timer != INVALID_MSG_ID)
    {
        cancel_timer(g_controller_timer);
        g_controller_timer = INVALID_MSG_ID;
    }
    return SUCCESS;
}

_int32 ulc_handle_timeout(const MSG_INFO *msg_info
    , _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid)
{
    _int32 ret = SUCCESS;
    BOOL is_downloading = tm_in_downloading();
    STRATEGY next_strategy = UPLOAD_CONTROL_STRATEGY_INIT;
    if(errcode == MSG_CANCELLED)
        return ret;
	if(0 == g_ulm_controller->_policy )
	{
	    g_ulm_controller->_is_in_charge = sd_get_charge_status();
	    g_ulm_controller->_now_charge = sd_get_now_charge();
	    g_ulm_controller->_is_in_lock = sd_get_lock_status();
	    LOG_DEBUG("ulm_control, now_charge = %d, is_in_charge = %d, is_in_lock = %d, is_downloading = %d"
	        , g_ulm_controller->_now_charge, g_ulm_controller->_is_in_charge
	        , g_ulm_controller->_is_in_lock, is_downloading);
	}
	else
	{	
		_int32 old_enable_upload_from_cfg,old_enable_upload_when_download_from_cfg,old_speed_limit_max;
		old_enable_upload_from_cfg=g_ulm_controller->_enable_upload_from_cfg;
		old_enable_upload_when_download_from_cfg=g_ulm_controller->_enable_upload_when_download_from_cfg;
		old_speed_limit_max=g_ulm_controller->_speed_limit_in_charge;
		
		settings_get_int_item("upload_manager_box.enable_upload", &g_ulm_controller->_enable_upload_from_cfg);
		settings_get_int_item("upload_manager_box.enable_upload_when_download", &g_ulm_controller->_enable_upload_when_download_from_cfg );
		settings_get_int_item("upload_manager_box.up_speed_limit_max", (_int32*)&g_ulm_controller->_speed_limit_in_charge);
		
		if( (old_enable_upload_from_cfg != g_ulm_controller->_enable_upload_from_cfg)||
			(old_enable_upload_when_download_from_cfg != g_ulm_controller->_enable_upload_when_download_from_cfg)||
			(old_speed_limit_max != g_ulm_controller->_speed_limit_in_charge)
			)
		{

			LOG_DEBUG("ulc_handle_timeout, change config  old_enable_upload = %d , old_enable_upload_when_download = %d , old_up_speed_limit_max = %d\
				enable_upload = %d , enable_upload_when_download = %d , up_speed_limit_max = %d"
				,old_enable_upload_from_cfg, old_enable_upload_when_download_from_cfg,old_speed_limit_max,
				g_ulm_controller->_enable_upload_from_cfg,g_ulm_controller->_enable_upload_when_download_from_cfg, g_ulm_controller->_speed_limit_in_charge);
		}
	}
	
	do
	{
	    if( 0 == g_ulm_controller->_enable_upload_from_cfg )
	    {
	        LOG_DEBUG("ulm_control, 0 == g_ulm_controller->_enable_upload_from_cfg, next_strategy = NEED_STOP");
	        next_strategy = NEED_STOP;        
			break;
	    }
    	if(0 == g_ulm_controller->_policy )
    	{
			if(!g_ulm_controller->_is_in_charge
		        && g_ulm_controller->_now_charge < g_ulm_controller->_lower_limit_charge_from_cfg)
		    {
		        LOG_DEBUG("ulm_control, charge too low, next_strategy = NEED_STOP");
		        next_strategy = NEED_STOP; 
				break;
		    }
		    else if(sd_get_global_net_type() != NT_WLAN) // 非wifi，直接停止上传
		    {
		        LOG_DEBUG("ulm_control, global_net_type != WIFI, next_strategy = NEED_STOP");
		        next_strategy = NEED_STOP;
				break;
		    }
    	}
        if(g_ulm_controller->_enable_upload_when_download_from_cfg) // 下载的时候上传
        {
            if(is_downloading)
            {
                LOG_DEBUG("ulm_control, enable_upload_when_download == 1, now in_downloading, next_strategy = NEED_START");
                next_strategy = NEED_START;
            }
            else
            {
                LOG_DEBUG("ulm_control, enable_upload_when_download == 1, now no_downloading, next_strategy = NEED_STOP");
                next_strategy = NEED_STOP;          
            }
        }
        else
        {
            LOG_DEBUG("ulm_control, enable_upload_when_download == 0, next_strategy = NEED_START");
            next_strategy = NEED_START;
        }
	}while(FALSE);
    LOG_DEBUG("ulm_control, next_strategy = %d, now_state = %d"
            , next_strategy, g_ulm_controller->_enable_upload );
    
    if( next_strategy == NEED_STOP)
    {
        if(g_ulm_controller->_enable_upload == 1)
        {
            g_ulm_controller->_enable_upload = 0;
            //todo: 发多次logout命令， 防止丢包
            ulc_stop_upload();
        }           
    }
    else if( next_strategy == NEED_START)
    {
        if(g_ulm_controller->_enable_upload == 0)
        {
            g_ulm_controller->_enable_upload = 1;
            // todo 开始ping服务器
            ulc_start_upload();
        }              
    }

    ulc_handle_score();
    
    return ret;
}


_int32 ulc_handle_score()
{
    LOG_DEBUG("ulc_hanlde_score enter ... ");
    _int32 ret = SUCCESS;
    BOOL is_downloading = tm_in_downloading();
    _u32 upload_speed_limit =10 ,_system_up_speed_limit;  // 单位K, 只限制数据包，协议包不限制
    _u64 upload_speed_limit_64;
    if( g_ulm_controller->_enable_upload == 1 )
    {
    	if(0 == g_ulm_controller->_policy )
    	{
	    	upload_speed_limit_64=upload_speed_limit;
	        if(g_ulm_controller->_is_in_charge)
	        {
	            upload_speed_limit_64 += g_ulm_controller->_speed_limit_in_charge;
	        }
	        if(g_ulm_controller->_is_in_lock)
	        {
	            upload_speed_limit_64 += g_ulm_controller->_speed_limit_in_lock;
	        }
	        if(is_downloading)
	        {
	            upload_speed_limit_64 += g_ulm_controller->_speed_limit_in_download;
	        }

	    	upload_speed_limit = -1;
			if(upload_speed_limit_64 < upload_speed_limit)
			{
				upload_speed_limit = (_u32)upload_speed_limit_64;
			}
    	}
		else
		{
			upload_speed_limit = g_ulm_controller->_speed_limit_in_charge;
			if(upload_speed_limit<10)
			{
				upload_speed_limit = 10;
			}
		}
    }
	_system_up_speed_limit = upload_speed_limit;
	settings_get_int_item("system.upload_limit_speed", (_int32*)&_system_up_speed_limit);
	if(upload_speed_limit > _system_up_speed_limit)
	{
		LOG_DEBUG("ulm_control, ulc_handle_score upload_speed_limit=%d < system.upload_limit_speed =%d",
			upload_speed_limit, _system_up_speed_limit);
		upload_speed_limit = _system_up_speed_limit;
	}
    _u32 last_download_limit = -1;
    _u32 last_upload_limit = 10;
    sl_get_speed_limit(&last_download_limit, &last_upload_limit);
    LOG_DEBUG("ulm_control, ulc_handle_score,  upload_speed_limit = %d, last download speed limit:%d, last upload limit:%d"
        ,  upload_speed_limit, last_download_limit, last_upload_limit);
    socket_proxy_set_speed_limit(last_download_limit, upload_speed_limit);
    return ret;
}

_int32 ulc_enable_upload()
{
    LOG_DEBUG("ulm_control, enable_upload = %d", g_ulm_controller->_enable_upload);
    return g_ulm_controller->_enable_upload;
}

_int32 ulc_stop_upload()
{
    // todo 停止ping服务器， 关闭所有正在上传的pipe
    LOG_DEBUG("ulm_control, ulc_stop_upload enter");
    ptl_stop_ping();
    upm_close_pure_upload_pipes();
    return SUCCESS;
}

_int32 ulc_start_upload()
{
    LOG_DEBUG("ulm_control, ulc_start_upload enter");
    ptl_start_ping();
    return SUCCESS;
}

#endif

