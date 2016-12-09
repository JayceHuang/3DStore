#include "utility/settings.h"
#include "utility/define_const_num.h"

#include "res_query_setting.h"

RES_QUERY_SETTING g_res_query_setting;

RES_QUERY_SETTING* get_res_query_setting()
{
	return &g_res_query_setting;
}

_int32 init_res_query_setting()
{
	sd_memcpy(g_res_query_setting._shub_addr, DEFAULT_RES_QUERY_SHUB_HOST, sizeof(DEFAULT_RES_QUERY_SHUB_HOST));
	settings_get_str_item("res_query_setting.shub_addr", g_res_query_setting._shub_addr);
	g_res_query_setting._shub_port = DEFAULT_RES_QUERY_SHUB_PORT;
	settings_get_int_item("res_query_setting.shub_port", (_int32*)&g_res_query_setting._shub_port);

	sd_memcpy(g_res_query_setting._phub_addr, DEFAULT_RES_QUERY_PHUB_HOST, sizeof(DEFAULT_RES_QUERY_PHUB_HOST));
	settings_get_str_item("res_query_setting.phub_addr", g_res_query_setting._phub_addr);
	g_res_query_setting._phub_port = DEFAULT_RES_QUERY_PHUB_PORT;
	settings_get_int_item("res_query_setting.phub_port", (_int32*)&g_res_query_setting._phub_port);

	sd_memcpy(g_res_query_setting._partner_cdn_addr, DEFAULT_RES_QUERY_PARTNER_CDN_HOST, sizeof(DEFAULT_RES_QUERY_PARTNER_CDN_HOST));
	settings_get_str_item("res_query_setting.partner_cdn_addr", g_res_query_setting._partner_cdn_addr);
	g_res_query_setting._partner_cdn_port = DEFAULT_RES_QUERY_PARTNER_CDN_PORT;
	settings_get_int_item("res_query_setting.partner_cdn_port", (_int32*)&g_res_query_setting._partner_cdn_port);

	sd_memcpy(g_res_query_setting._tracker_addr, DEFAULT_RES_QUERY_TRACKER_HOST, sizeof(DEFAULT_RES_QUERY_TRACKER_HOST));
	settings_get_str_item("res_query_setting.tracker_addr", g_res_query_setting._tracker_addr);
	g_res_query_setting._tracker_port = DEFAULT_RES_QUERY_TRACKER_PORT;
	settings_get_int_item("res_query_setting.tracker_port", (_int32*)&g_res_query_setting._tracker_port);

	sd_memcpy(g_res_query_setting._bt_hub_addr, DEFAULT_RES_QUERY_BTHUB_HOST, sizeof(DEFAULT_RES_QUERY_BTHUB_HOST));
	settings_get_str_item("res_query_setting.bt_hub_addr", g_res_query_setting._bt_hub_addr);
	g_res_query_setting._bt_hub_port = DEFAULT_RES_QUERY_BTHUB_PORT;
	settings_get_int_item("res_query_setting.bt_hub_port", (_int32*)&g_res_query_setting._bt_hub_port);

#ifdef ENABLE_CDN
	sd_memcpy(g_res_query_setting._cdn_manager_addr, DEFAULT_RES_QUERY_CDN_MANAGER_HOST, sizeof(DEFAULT_RES_QUERY_CDN_MANAGER_HOST));
	settings_get_str_item("res_query_setting.cdn_manager_addr", g_res_query_setting._cdn_manager_addr);
	g_res_query_setting._cdn_manager_port = DEFAULT_RES_QUERY_CDN_MANAGER_PORT;
	settings_get_int_item("res_query_setting.cdn_manager_port", (_int32*)&g_res_query_setting._cdn_manager_port);
	sd_memcpy(g_res_query_setting._normal_cdn_manager_addr, DEFAULT_RES_QUERY_NORMAL_CDN_MANAGER_HOST, sizeof(DEFAULT_RES_QUERY_NORMAL_CDN_MANAGER_HOST));
	settings_get_str_item("res_query_setting.normal_cdn_manager_addr", g_res_query_setting._normal_cdn_manager_addr);
	g_res_query_setting._normal_cdn_manager_port = DEFAULT_RES_QUERY_NORMAL_CDN_MANAGER_PORT;
	settings_get_int_item("res_query_setting.normal_cdn_manager_port", (_int32*)&g_res_query_setting._normal_cdn_manager_port);
#ifdef ENABLE_KANKAN_CDN
	sd_memcpy(g_res_query_setting._kankan_cdn_manager_addr, DEFAULT_RES_QUERY_KANKAN_CDN_MANAGER_HOST, sizeof(DEFAULT_RES_QUERY_KANKAN_CDN_MANAGER_HOST));
	g_res_query_setting._kankan_cdn_manager_port = DEFAULT_RES_QUERY_KANKAN_CDN_MANAGER_PORT;
#endif /* ENABLE_KANKAN_CDN */	
#ifdef ENABLE_HSC
	sd_memcpy(g_res_query_setting._vip_hub_addr, DEFAULT_RES_QUERY_VIP_HUB_HOST, sizeof(DEFAULT_RES_QUERY_VIP_HUB_HOST));
	settings_get_str_item("res_query_setting.vip_hub_addr", g_res_query_setting._vip_hub_addr);
	g_res_query_setting._vip_hub_port = DEFAULT_RES_QUERY_VIP_HUB_PORT;
	settings_get_int_item("res_query_setting.vip_hub_port", (_int32*)&g_res_query_setting._vip_hub_port);
#endif /* ENABLE_HSC */
#endif

#ifdef ENABLE_EMULE
	sd_memcpy(g_res_query_setting._emule_hub_addr, DEFAULT_RES_QUERY_EMULEHUB_HOST, sizeof(DEFAULT_RES_QUERY_EMULEHUB_HOST));
	settings_get_str_item("res_query_setting.emule_hub_addr", g_res_query_setting._emule_hub_addr);
	g_res_query_setting._emule_hub_port = DEFAULT_RES_QUERY_EMULEHUB_PORT;
	settings_get_int_item("res_query_setting.emule_hub_port", (_int32*)&g_res_query_setting._emule_hub_port);
#endif

    sd_memcpy(g_res_query_setting._dphub_root_addr, DEFAULT_RES_QUERY_DPHUB_ROOT_HOST, sizeof(DEFAULT_RES_QUERY_DPHUB_ROOT_HOST));
    settings_get_str_item("res_query_setting.dphub_root_addr", g_res_query_setting._dphub_root_addr);
    g_res_query_setting._dphub_root_port = DEFAULT_RES_QUERY_DPHUB_ROOT_PORT;
    settings_get_int_item("res_query_setting.dphub_root_port", (_int32*)&g_res_query_setting._dphub_root_port);

	sd_memcpy(g_res_query_setting._config_hub_addr, DEFAULT_RES_QUERY_CONFIG_HUB_HOST, sizeof(DEFAULT_RES_QUERY_CONFIG_HUB_HOST));
	settings_get_str_item("res_query_setting.config_hub_addr", g_res_query_setting._config_hub_addr);
	g_res_query_setting._config_hub_port = DEFAULT_RES_QUERY_CONFIG_HUB_PORT;
	settings_get_int_item("res_query_setting.config_hub_port", (_int32*)&g_res_query_setting._config_hub_port);

    sd_memcpy(g_res_query_setting._emule_trakcer_addr, DEFAULT_RES_QUERY_EMULETRACKER_HOST, sizeof(DEFAULT_RES_QUERY_EMULETRACKER_HOST));
    settings_get_str_item("res_query_setting.emule_tracker_addr", g_res_query_setting._emule_trakcer_addr);
    g_res_query_setting._emule_tracker_port= DEFAULT_RES_QUERY_EMULETRACKER_PORT;
    settings_get_int_item("res_query_setting.emule_tracker_port", (_int32*)&g_res_query_setting._emule_tracker_port);

	g_res_query_setting._cmd_retry_times = 3;
	settings_get_int_item("res_query_setting.cmd_retry_times", (_int32*)&g_res_query_setting._cmd_retry_times);
	return SUCCESS;
}
