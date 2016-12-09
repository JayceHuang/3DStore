#include "reporter_setting.h"
#include "utility/settings.h"

static REPORTER_SETTING g_reporter_setting;

REPORTER_SETTING* get_reporter_setting(void)
{
	return &g_reporter_setting;
}

_int32 init_reporter_setting(void)
{
	_int32 ret_val = SUCCESS;
	sd_memset(&g_reporter_setting,0,sizeof(REPORTER_SETTING));

	sd_memcpy(g_reporter_setting._license_server_addr, DEFAULT_LICENSE_HOST_NAME, sizeof(DEFAULT_LICENSE_HOST_NAME));
	ret_val = settings_get_str_item("license.license_server_addr", g_reporter_setting._license_server_addr);
	CHECK_VALUE(ret_val);
	g_reporter_setting._license_server_port = DEFAULT_LICENSE_PORT;
	ret_val = settings_get_int_item("license.license_server_port", (_int32*)&g_reporter_setting._license_server_port);
	CHECK_VALUE(ret_val);

	sd_memcpy(g_reporter_setting._shub_addr, DEFAULT_SHUB_HOST_NAME, sizeof(DEFAULT_SHUB_HOST_NAME));
	ret_val = settings_get_str_item("reporter_setting.shub_addr", g_reporter_setting._shub_addr);
	CHECK_VALUE(ret_val);
	g_reporter_setting._shub_port = DEFAULT_SHUB_PORT;
	ret_val = settings_get_int_item("reporter_setting.shub_port", (_int32*)&g_reporter_setting._shub_port);
	CHECK_VALUE(ret_val);

	sd_memcpy(g_reporter_setting._stat_hub_addr, DEFAULT_STAT_HUB_HOST_NAME, sizeof(DEFAULT_STAT_HUB_HOST_NAME));
	ret_val = settings_get_str_item("reporter_setting.stat_hub_addr", g_reporter_setting._stat_hub_addr);
	CHECK_VALUE(ret_val);
	g_reporter_setting._stat_hub_port = DEFAULT_STAT_HUB_PORT;
	ret_val = settings_get_int_item("reporter_setting.stat_hub_port", (_int32*)&g_reporter_setting._stat_hub_port);
	CHECK_VALUE(ret_val);
	
#ifdef ENABLE_BT
	sd_memcpy(g_reporter_setting._bt_hub_addr, DEFAULT_BT_HUB_HOST_NAME, sizeof(DEFAULT_BT_HUB_HOST_NAME));
	ret_val = settings_get_str_item("reporter_setting.bt_hub_addr", g_reporter_setting._bt_hub_addr);
	CHECK_VALUE(ret_val);
	g_reporter_setting._bt_hub_port = DEFAULT_BT_HUB_PORT;
	ret_val = settings_get_int_item("reporter_setting.bt_hub_port", (_int32*)&g_reporter_setting._bt_hub_port);
	CHECK_VALUE(ret_val);
#endif

#ifdef ENABLE_EMULE
	sd_memcpy(g_reporter_setting._emule_hub_addr, DEFAULT_EMULE_HUB_HOST_NAME, sizeof(DEFAULT_EMULE_HUB_HOST_NAME));
	ret_val = settings_get_str_item("reporter_setting.emule_hub_addr", g_reporter_setting._emule_hub_addr);
	CHECK_VALUE(ret_val);
	g_reporter_setting._emule_hub_port = DEFAULT_EMULE_HUB_PORT;
	ret_val = settings_get_int_item("reporter_setting.emule_hub_port", (_int32*)&g_reporter_setting._emule_hub_port);
	CHECK_VALUE(ret_val);
#endif

#ifdef UPLOAD_ENABLE
	sd_memcpy(g_reporter_setting._phub_addr, DEFAULT_PHUB_HOST_NAME, sizeof(DEFAULT_PHUB_HOST_NAME));
	ret_val = settings_get_str_item("reporter_setting.phub_addr", g_reporter_setting._phub_addr);
	CHECK_VALUE(ret_val);
	g_reporter_setting._phub_port = DEFAULT_PHUB_PORT;
	ret_val = settings_get_int_item("reporter_setting.phub_port", (_int32*)&g_reporter_setting._phub_port);
	CHECK_VALUE(ret_val);
#endif

#ifdef EMBED_REPORT
	g_reporter_setting._online_peer_report_interval = DEFAULT_REPORT_INTERVAL;
	ret_val = settings_get_int_item("reporter_setting._online_peer_report_interval", (_int32*)&g_reporter_setting._online_peer_report_interval);

	sd_memcpy(g_reporter_setting._emb_hub_addr, DEFAULT_EMB_HUB_HOST_NAME, sizeof(DEFAULT_EMB_HUB_HOST_NAME));
	ret_val = settings_get_str_item("reporter_setting.emb_hub_report_host", g_reporter_setting._emb_hub_addr);
	CHECK_VALUE(ret_val);
	g_reporter_setting._emb_hub_port = DEFAULT_EMB_HUB_PORT;
	ret_val = settings_get_int_item("reporter_setting.emb_hub_report_port", (_int32*)&g_reporter_setting._emb_hub_port);
	CHECK_VALUE(ret_val);

#endif

	g_reporter_setting._cmd_retry_times = DEFAULT_CMD_RETRY_TIMES;
	ret_val = settings_get_int_item("reporter_setting.cmd_retry_times", (_int32*)&g_reporter_setting._cmd_retry_times);
	CHECK_VALUE(ret_val);

	return SUCCESS;
}


