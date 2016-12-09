#ifndef _RES_QUERY_IMPL_H_
#define _RES_QUERY_IMPL_H_

#ifdef __cplusplus
extern "C" 
{
#endif
#include "asyn_frame/msg.h"
#include "res_query_interface_imp.h"

/*调用该函数，那么buffer将被接管*/
// extra_data 是用于附加在命令后面的私货，区别于user_data
_int32 res_query_commit_cmd(HUB_DEVICE* device, _u16 cmd_type, char* buffer, _u32 len, void* callback, void* user_data, 
	_u32 cmd_seq,BOOL not_add_res, void *extra_data, _u8* aes_key);

_int32 res_query_execute_cmd(HUB_DEVICE* device);

/*connect event notify*/
_int32 res_query_handle_connect_callback(_int32 errcode, _u32 pending_op_count, void* user_data);

/*send event notify*/
_int32 res_query_handle_send_callback(_int32 errcode, _u32 pending_op_count, const char* buffer, _u32 had_send, void* user_data);

/*recv event notify*/
_int32 res_query_handle_recv_callback(_int32 errcode, _u32 pending_op_count, char* buffer, _u32 had_recv, void* user_data);

/*handle network error*/
_int32 res_query_handle_network_error(HUB_DEVICE* device, _int32 error_code);

_int32 res_query_extend_recv_buffer(HUB_DEVICE* device, _u32 total_len);

_int32 res_query_handle_cancel_msg(HUB_DEVICE* device);

_int32 res_query_notify_execute_cmd_failed(HUB_DEVICE* device);

_int32 res_query_retry_handler(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid);

_int32 res_query_free_last_cmd(HUB_DEVICE* device);

void res_query_update_last_cmd(HUB_DEVICE* device);

_int32 res_query_handle_rsa_encountered_resp(HUB_DEVICE *p_device, _u32 http_header_len);

#ifdef __cplusplus
}
#endif
#endif
