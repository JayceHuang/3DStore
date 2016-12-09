/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/10/29
-----------------------------------------------------------------------------------------------------------*/
#ifndef _EMULE_SERVER_CMD_H_
#define _EMULE_SERVER_CMD_H_
#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL

#include "emule/emule_server/emule_server.h"
#include "emule/emule_utility/emule_define.h"

struct tagEMULE_SERVER_DEVICE;

_int32 emule_send_tcp_query_source_cmd(struct tagEMULE_SERVER_DEVICE* device, _u8 file_id[FILE_ID_SIZE], _u64 file_size);

_int32 emule_send_udp_query_source_cmd(EMULE_SERVER* server, _u8 file_id[FILE_ID_SIZE], _u64 file_size);

_int32 emule_send_login_req_cmd(struct tagEMULE_SERVER_DEVICE* device);

_int32 emule_send_request_callback_cmd(struct tagEMULE_SERVER_DEVICE* device, _u32 client_id);

#endif
#endif /* ENABLE_EMULE */
#endif


