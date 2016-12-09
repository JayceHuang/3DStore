#ifndef _RES_QUERY_CMD_BUILDER_H_
#define _RES_QUERY_CMD_BUILDER_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include "res_query_interface_imp.h"
#include "res_query_cmd_define.h"

_int32 xl_aes_encrypt(char* buffer,_u32* len);

_int32 build_query_server_res_cmd(HUB_DEVICE* device,char** buffer, _u32* len, QUERY_SERVER_RES_CMD* cmd);

_int32 build_query_peer_res_cmd(HUB_DEVICE* device,char** buffer, _u32* len, QUERY_PEER_RES_CMD* cmd);

_int32 build_query_tracker_res_cmd(HUB_DEVICE* device,char** buffer, _u32* len, QUERY_TRACKER_RES_CMD* cmd);

_int32 build_query_bt_info_cmd(HUB_DEVICE* device,char** buffer, _u32* len, QUERY_BT_INFO_CMD* cmd);

_int32 build_enrollsp1_cmd(HUB_DEVICE* device,char** buffer, _u32* len, ENROLLSP1_CMD* cmd);

_int32 build_query_config_cmd(HUB_DEVICE* device,char** buffer, _u32* len, QUERY_CONFIG_CMD* cmd);

#ifdef ENABLE_CDN
_int32 build_query_cdn_manager_info_cmd(HUB_DEVICE* device,char** buffer, _u32* len, CDNMANAGERQUERY* cmd);
#ifdef ENABLE_KANKAN_CDN
_int32 build_query_kankan_cdn_manager_info_cmd(HUB_DEVICE* device,char** buffer, _u32* len, CDNMANAGERQUERY* cmd);
#endif

#endif

_int32 res_query_build_http_header(char* buffer, _u32* len, _u32 data_len,HUB_TYPE hub_type, const char *device_addr, _u16 device_port);

_int32 build_reservce_60ver(char** buffer, _u32* len );

_int32 build_query_newserver_res_cmd(HUB_DEVICE* device,char** buffer, _u32* len, NEWQUERY_SERVER_RES_CMD* cmd);

_int32 build_relation_query_server_res_cmd(HUB_DEVICE* device,char** buffer, _u32* len, QUERY_FILE_RELATION_RES_CMD* cmd);

_int32 build_query_info_cmd(HUB_DEVICE* device,char** buffer, _u32* len, QUERY_RES_INFO_CMD* cmd);

/////////////////////////////////// RSA ////////////////////////////////
_int32 build_rsa_encrypt_header(char **ppbuf, _int32 *p_buflen, _int32 pubkey_ver,
	_u8 *aes_key, _int32 data_len);
_int32 build_query_newserver_res_cmd_rsa(HUB_DEVICE* device,char** buffer, _u32* len, NEWQUERY_SERVER_RES_CMD* cmd, 
	_u8* aes_key, _int32 pubkey_version);
_int32 build_query_peer_res_cmd_rsa(HUB_DEVICE* device,char** buffer, _u32* len, QUERY_PEER_RES_CMD* cmd,
	_u8* aes_key, _int32 pubkey_version);
_int32 build_query_info_cmd_rsa(HUB_DEVICE* device,char** buffer, _u32* len, QUERY_RES_INFO_CMD* cmd,
	_u8* aes_key, _int32 pubkey_version);
_int32 build_query_server_res_cmd_rsa(HUB_DEVICE* device,char** buffer, _u32* len, QUERY_SERVER_RES_CMD* cmd,
	_u8* aes_key, _int32 pubkey_version);
_int32 build_enrollsp1_cmd_rsa(HUB_DEVICE* device,char** buffer, _u32* len, ENROLLSP1_CMD* cmd,
	_u8* aes_key, _int32 pubkey_version);
_int32 build_query_bt_info_cmd_rsa(HUB_DEVICE* device, char** buffer, _u32* len, QUERY_BT_INFO_CMD* cmd,
	_u8* aes_key, _int32 pubkey_version);
_int32 build_query_tracker_res_cmd_rsa(HUB_DEVICE* device,char** buffer, _u32* len, QUERY_TRACKER_RES_CMD* cmd, 
	_u8* aes_key, _int32 pubkey_version);
#ifdef __cplusplus
}
#endif
#endif
