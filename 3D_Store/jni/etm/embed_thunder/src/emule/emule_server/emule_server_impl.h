/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/10/30
-----------------------------------------------------------------------------------------------------------*/
#ifndef _EMULE_SERVER_IMPL_H_
#define _EMULE_SERVER_IMPL_H_
#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL

#include "emule/emule_utility/emule_define.h"

struct tagEMULE_SERVER_DEVICE;

_int32 emule_server_init(void);

_int32 emule_server_uninit(void);

_int32 emule_server_save_impl(void);

_int32 emule_server_load_impl(void);

//该函数创建emule的优先连接服务器列表
_int32 emule_build_priority_server_list(void);

_int32 emule_try_connect_server(void);

_int32 emule_server_notify_connect(struct tagEMULE_SERVER_DEVICE* device);

_int32 emule_server_notify_close(void);

_int32 emule_server_handle_error(_int32 errcode, struct tagEMULE_SERVER_DEVICE* device);

_int32 emule_server_recv_cmd(struct tagEMULE_SERVER_DEVICE* device, char* buffer, _int32 len);

_int32 emule_server_recv_udp_cmd(char* buffer, _int32 len, _u32 ip, _u16 port);

_int32 emule_handle_server_status_cmd(struct tagEMULE_SERVER_DEVICE* device, char* buffer, _int32 len);

_int32 emule_handle_id_change_cmd(struct tagEMULE_SERVER_DEVICE* device, char* buffer, _int32 len);

_int32 emule_handle_server_message_cmd(struct tagEMULE_SERVER_DEVICE* device, char* buffer, _int32 len);

_int32 emule_handle_udp_found_sources_cmd(char* buffer, _int32 len);

_int32 emule_server_query_source_impl(_u8 file_id[FILE_ID_SIZE], _u64 file_size);

_int32 emule_handle_found_sources_cmd(struct tagEMULE_SERVER_DEVICE* device, char* buffer, _int32 len);

_int32 emule_handle_found_obfu_sources_cmd(struct tagEMULE_SERVER_DEVICE* device, char* buffer, _int32 len);

_int32 emule_server_request_callback(_u32 client_id);

BOOL emule_is_local_server_impl(_u32 ip, _u16 port);

_int32 emule_get_local_server(_u32* server_ip, _u16* server_port);

#endif
#endif /* ENABLE_EMULE */

#endif

