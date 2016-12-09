/*----------------------------------------------------------------------------------------------------------
author:		ZengYuqing
created:	2008/09/20
-----------------------------------------------------------------------------------------------------------*/
#ifndef _REPORTER_CMD_HANDLER_H_
#define _REPORTER_IMPL_H_

#ifdef __cplusplus
extern "C" 
{
#endif


#include "reporter/reporter_device.h"
#include "asyn_frame/msg.h"
#include "download_task/download_task.h"
/*commit shub report */
_int32 reporter_commit_cmd(REPORTER_DEVICE* device, _u16 cmd_type, const char* buffer, _u32 len, void* user_data, _u32 cmd_seq);
_int32 reporter_execute_cmd(REPORTER_DEVICE* device);
_int32 reporter_handle_connect_callback(_int32 errcode, _u32 pending_op_count, void* user_data);
_int32 reporter_handle_send_callback(_int32 errcode, _u32 pending_op_count, const char* buffer, _u32 had_send, void* user_data);
_int32 reporter_handle_recv_callback(_int32 errcode, _u32 pending_op_count, char* buffer, _u32 had_recv, void* user_data);
_int32 reporter_close_socket(REPORTER_DEVICE* device);
_int32 reporter_handle_network_error(REPORTER_DEVICE* device, _int32 errcode);
_int32 reporter_retry_handler(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid);
_int32 reporter_notify_execute_cmd_failed(REPORTER_DEVICE* device);
_int32 reporter_extend_recv_buffer(REPORTER_DEVICE* device, _u32 total_len);
_int32 reporter_failure_exit(REPORTER_DEVICE* device, _int32 errcode);

#ifdef EMBED_REPORT
_int32 reporter_init_timeout_event(BOOL from_server);
_int32 reporter_uninit_timeout_event(void);
_int32 reporter_handle_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid);
_int32 reporter_set_enable_file_report(BOOL is_enable);

#endif




#ifdef __cplusplus
}
#endif


#endif  /* _REPORTER_IMPL_H_ */
