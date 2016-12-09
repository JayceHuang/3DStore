/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/10/13
-----------------------------------------------------------------------------------------------------------*/
#ifndef _EMULE_PIPE_CMD_H_
#define _EMULE_PIPE_CMD_H_

#include "utility/define.h"

#ifdef ENABLE_EMULE

#ifdef ENABLE_EMULE_PROTOCOL
#include "utility/range.h"


struct tagEMULE_DATA_PIPE;
struct tagEMULE_TAG_LIST;

_int32 emule_pipe_send_hello_cmd(struct tagEMULE_DATA_PIPE* emule_pipe);

_int32 emule_pipe_send_hello_answer_cmd(struct tagEMULE_DATA_PIPE* emule_pipe);

_int32 emule_pipe_send_req_filename_cmd(struct tagEMULE_DATA_PIPE* emule_pipe);

_int32 emule_pipe_send_req_filename_answer_cmd(struct tagEMULE_DATA_PIPE* emule_pipe);

_int32 emule_pipe_send_req_file_id_cmd(struct tagEMULE_DATA_PIPE* emule_pipe);

_int32 emule_pipe_send_part_hash_req_cmd(struct tagEMULE_DATA_PIPE* emule_pipe);

_int32 emule_pipe_send_arch_hash_req_cmd(struct tagEMULE_DATA_PIPE* emule_pipe);

_int32 emule_pipe_send_part_hash_answer_cmd(struct tagEMULE_DATA_PIPE* emule_pipe);

_int32 emule_pipe_send_start_upload_req_cmd(struct tagEMULE_DATA_PIPE* emule_pipe);

_int32 emule_pipe_send_req_part_cmd(struct tagEMULE_DATA_PIPE* emule_pipe, RANGE* req_range);

_int32 emule_pipe_send_req_source_cmd(struct tagEMULE_DATA_PIPE* emule_pipe);

_int32 emule_pipe_send_emule_info_answer_cmd(struct tagEMULE_DATA_PIPE* emule_pipe);

_int32 emule_pipe_send_file_state_cmd(struct tagEMULE_DATA_PIPE* emule_pipe);

_int32 emule_pipe_send_accept_upload_req_cmd(struct tagEMULE_DATA_PIPE* emule_pipe, BOOL is_accept);

_int32 emule_pipe_send_reask_cmd(struct tagEMULE_DATA_PIPE* emule_pipe);

_int32 emule_pipe_send_secident_state(struct tagEMULE_DATA_PIPE* emule_pipe);

_int32 emule_pipe_send_aich_answer_cmd(struct tagEMULE_DATA_PIPE* emule_pipe);

#endif
#endif

#endif
