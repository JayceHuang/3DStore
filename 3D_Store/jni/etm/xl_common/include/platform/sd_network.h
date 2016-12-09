#ifndef SD_NETEORK_H_20100827
#define SD_NETEORK_H_20100827
#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"
#include "utility/errcode.h"

/*
网络连接类型：
(低16位) */
#define UNKNOWN_NET   	0x00000000
#define CHINA_MOBILE  	0x00000001
#define CHINA_UNICOM  	0x00000002
#define CHINA_TELECOM 	0x00000004
#define OTHER_NET     	0x00008000
#define NO_NET_WORK     0xF0000000

/* (高16位) */
#define NT_GPRS_WAP   	0x00010000
#define NT_GPRS_NET   	0x00020000
#define NT_3G         	0x00040000
#define NT_WLAN       	0x00080000   // wifi and lan ...

#define NT_CMWAP 	(NT_GPRS_WAP|CHINA_MOBILE)
#define NT_CMNET 	(NT_GPRS_NET|CHINA_MOBILE)

#define NT_CUIWAP 	(NT_GPRS_WAP|CHINA_UNICOM)
#define NT_CUINET 	(NT_GPRS_NET|CHINA_UNICOM)

#define NT_CTWAP 	(NT_GPRS_WAP|CHINA_TELECOM)
#define NT_CTNET 	(NT_GPRS_NET|CHINA_TELECOM)

/////1.3 网络相关接口
typedef _int32 ( *SD_NOTIFY_NET_CONNECT_RESULT)(_u32 iap_id,_int32 result,_u32 net_type);
typedef enum t_sd_net_status
{
	SNS_DISCNT = 0, 
	SNS_CNTING, 
	SNS_CNTED 
} SD_NET_STATUS;

IMPORT_C _int32 sd_init_network(_u32 iap_id,SD_NOTIFY_NET_CONNECT_RESULT call_back_func);
IMPORT_C _int32 sd_uninit_network(void);
IMPORT_C SD_NET_STATUS sd_get_network_status(void);
IMPORT_C _int32 sd_get_network_iap(_u32 *iap_id);
IMPORT_C const char* sd_get_network_iap_name(void);
IMPORT_C void sd_check_net_connection_result(void);
IMPORT_C _u32 sd_get_net_type(void);
IMPORT_C _int32 sd_set_net_type(_u32 net_type);
IMPORT_C BOOL sd_is_net_ok(void);

IMPORT_C _int32 sd_set_proxy(_u32 ip,_u16 port);
IMPORT_C _int32 sd_get_proxy(_u32 *p_ip,_u16 *p_port);

#if defined(_ANDROID_LINUX) || defined(LINUX)
IMPORT_C _u32 sd_get_global_net_type(void);
IMPORT_C _int32 sd_set_global_net_type(_u32 net_type);
#endif


#ifdef __cplusplus
}
#endif
#endif /* SD_NETEORK_H_20100827 */
