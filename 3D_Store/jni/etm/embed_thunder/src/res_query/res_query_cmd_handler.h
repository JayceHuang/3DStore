#ifndef _RES_QUERY_CMD_HANDLER_H_
#define _RES_QUERY_CMD_HANDLER_H_
#ifdef __cplusplus
extern "C" 
{
#endif

#include "res_query_interface_imp.h"

_int32 set_add_resource_func(res_query_add_server_res_handler add_server_res, res_query_add_peer_res_handler add_peer_res, res_query_add_relation_fileinfo add_relation_fileinfo);

_int32 handle_recv_resp_cmd(char* buffer, _u32 len, HUB_DEVICE* device);

_int32 handle_server_res_resp(char* buffer, _u32 len, RES_QUERY_CMD* last_cmd);

_int32 handle_peer_res_resp(char* buffer, _u32 len, RES_QUERY_CMD* last_cmd, _u8 res_from);

_int32 handle_tracker_res_resp(char* buffer, _u32 len, RES_QUERY_CMD* last_cmd);

_int32 handle_enrollsp1_resp(char* buffer, _u32 len, RES_QUERY_CMD* last_cmd);

_int32 handle_query_config_resp(char* buffer, _u32 len, RES_QUERY_CMD* last_cmd);

_int32 handle_emule_tracker_res_resp(char *buffer, _int32 len, RES_QUERY_CMD *last_cmd);

_int32 xl_aes_decrypt(char* pDataBuff, _u32* nBuffLen);

_int32 handle_server_res_info(char* buffer, _u32 len, RES_QUERY_CMD* last_cmd);

_int32 handle_newserver_res_resp(char* buffer, _u32 len, RES_QUERY_CMD* last_cmd);

_int32 handle_query_relation_server_res_resp(char* buffer, _u32 len, RES_QUERY_CMD* last_cmd);

#ifdef ENABLE_BT
_int32 handle_bt_info_resp(char* buffer, _u32 len, RES_QUERY_CMD* last_cmd);
#endif

_int32 handle_recv_resp_cmd_with_aes_key(char* buffer, _u32 len, HUB_DEVICE* device, _u8 *p_aeskey);

#ifdef __cplusplus
}
#endif
#endif
