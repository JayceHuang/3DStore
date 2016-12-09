#ifndef RES_QUERY_SETTING_H_
#define RES_QUERY_SETTING_H_
#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"

typedef struct tagRES_QUERY_SETTING
{
	char	_shub_addr[64];
	_u32	_shub_port;
	char	_phub_addr[64];
	_u32	_phub_port;
	char	_partner_cdn_addr[64];
	_u32	_partner_cdn_port;
	char	_tracker_addr[64];
	_u32	_tracker_port;
	char	_bt_hub_addr[64];
	_u32	_bt_hub_port;
#ifdef ENABLE_CDN
	char	_cdn_manager_addr[64];
	_u32	_cdn_manager_port;
	char	_normal_cdn_manager_addr[64];
	_u32	_normal_cdn_manager_port;
#ifdef ENABLE_KANKAN_CDN
	char _kankan_cdn_manager_addr[64];
	_u32	_kankan_cdn_manager_port;
#endif /* ENABLE_KANKAN_CDN */
#ifdef ENABLE_HSC
	char	_vip_hub_addr[64];
	_u32	_vip_hub_port;
#endif /* ENABLE_HSC */
#endif
#ifdef ENABLE_EMULE
	char _emule_hub_addr[64];
	_u32 _emule_hub_port;
#endif

    char _dphub_root_addr[64];
    _u32 _dphub_root_port;

//≈‰÷√≤Œ ˝
	char _config_hub_addr[64];
	_u32 _config_hub_port;
	_u32	_cmd_retry_times;			/*execute command failed and it will retry 3 times*/			
	
    char    _emule_trakcer_addr[64];
    _u32    _emule_tracker_port;

}RES_QUERY_SETTING;

RES_QUERY_SETTING* get_res_query_setting(void);
_int32 init_res_query_setting(void);

#ifdef __cplusplus
}
#endif
#endif
