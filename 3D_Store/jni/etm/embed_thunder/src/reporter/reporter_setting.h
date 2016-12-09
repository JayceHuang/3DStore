/*----------------------------------------------------------------------------------------------------------
author:		ZengYuqing
created:	2008/09/20
-----------------------------------------------------------------------------------------------------------*/
#ifndef _REPORTER_SETTING_H_
#define _REPORTER_SETTING_H_

#ifdef __cplusplus
extern "C" 
{
#endif


#include "utility/define.h"


typedef struct tagREPORTER_SETTING
{
	char	_license_server_addr[MAX_HOST_NAME_LEN/2];
	_u32	_license_server_port;
	char	_shub_addr[MAX_HOST_NAME_LEN/2];
	_u32	_shub_port;
	char	_stat_hub_addr[MAX_HOST_NAME_LEN/2];
	_u32	_stat_hub_port;
	char	_basic_stat_hub_addr[MAX_HOST_NAME_LEN/2];
	_u32	_basic_stat_hub_port;
#ifdef ENABLE_BT
	char	_bt_hub_addr[MAX_HOST_NAME_LEN/2];
	_u32	_bt_hub_port;
#endif

#ifdef ENABLE_EMULE
	char	_emule_hub_addr[MAX_HOST_NAME_LEN/2];
	_u32	_emule_hub_port;
#endif

#ifdef UPLOAD_ENABLE
	char	_phub_addr[MAX_HOST_NAME_LEN/2];
	_u32	_phub_port;
#endif

#ifdef EMBED_REPORT
	_u32	_online_peer_report_interval;
	char	_emb_hub_addr[MAX_HOST_NAME_LEN/2];
	_u32	_emb_hub_port;
#endif

	_u32	_cmd_retry_times;			/*execute command failed and it will retry 3 times*/		

}REPORTER_SETTING;

REPORTER_SETTING* get_reporter_setting(void);

_int32 init_reporter_setting(void);

#ifdef __cplusplus
}
#endif


#endif  /*  _REPORTER_SETTING_H_  */
