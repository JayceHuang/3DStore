/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/11/17
-----------------------------------------------------------------------------------------------------------*/
#ifndef _EMULE_PIPE_HANDLE_CMD_H_
#define _EMULE_PIPE_HANDLE_CMD_H_
#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL

#include "emule/emule_pipe/emule_pipe_interface.h"

struct tagEMULE_PIPE_DEVICE;

_int32 emule_handle_hello_cmd(struct tagEMULE_PIPE_DEVICE* emule_device, char* buffer, _int32 len);

_int32 emule_handle_hello_answer_cmd(EMULE_DATA_PIPE* emule_pipe, char* buffer, _int32 len);

_int32 emule_handle_sending_part_cmd(EMULE_DATA_PIPE* emule_pipe, char* buffer, _int32 len, BOOL is_64_offset);

_int32 emule_handle_aich_hash_ans_cmd(EMULE_DATA_PIPE* emule_pipe, char* buffer, _int32 len);

_int32 emule_handle_aich_hash_req_cmd(EMULE_DATA_PIPE* emule_pipe, char* buffer, _int32 len);

_int32 emule_handle_request_parts_cmd(EMULE_DATA_PIPE* emule_pipe, char* buffer, _int32 len, BOOL is_64_offset);

_int32 emule_handle_set_req_fileid_cmd(EMULE_DATA_PIPE* emule_pipe, char* buffer, _int32 len);

_int32 emule_handle_start_upload_req_cmd(EMULE_DATA_PIPE* emule_pipe, char* buffer, _int32 len);

_int32 emule_handle_accept_upload_req_cmd(EMULE_DATA_PIPE* emule_pipe);

_int32 emule_handle_cancel_transfer_cmd(EMULE_DATA_PIPE* emule_pipe);

//========================================================================================
//以下是emule扩展命令
//========================================================================================
_int32 emule_handle_answer_sources_cmd(EMULE_DATA_PIPE* emule_pipe, char* buffer, _int32 len);

_int32 emule_handle_secident_state_cmd(EMULE_DATA_PIPE* emule_pipe, char* buffer, _int32 len);

#endif
#endif /* ENABLE_EMULE */
#endif


