/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/10/13
-----------------------------------------------------------------------------------------------------------*/
#ifndef _EMULE_PIPE_IMPL_H_
#define _EMULE_PIPE_IMPL_H_
#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL

#include "emule/emule_pipe/emule_pipe_interface.h"
#include "asyn_frame/msg.h"

struct tagBITMAP;
struct tagEMULE_PIPE_DEVICE;

void emule_pipe_attach_emule_device(EMULE_DATA_PIPE* emule_pipe, struct tagEMULE_PIPE_DEVICE* pipe_device);

_int32 emule_pipe_notify_connnect(struct tagEMULE_PIPE_DEVICE* emule_device);

//通知双方握手成功
_int32 emule_pipe_notify_handshake(EMULE_DATA_PIPE* emule_pipe);

_int32 emule_pipe_notify_recv_part_data(EMULE_DATA_PIPE* emule_pipe, const RANGE* down_range);

void emule_pipe_change_state(EMULE_DATA_PIPE* emule_pipe, PIPE_STATE state);

_int32 emule_pipe_request_data(EMULE_DATA_PIPE* emule_pipe);

_int32 emule_pipe_handle_error(EMULE_DATA_PIPE* emule_pipe, _int32 errcode);

_int32 emule_handle_recv_edonkey_cmd(struct tagEMULE_PIPE_DEVICE* emule_device, char* buffer, _int32 len);

_int32 emule_handle_recv_emule_cmd(EMULE_DATA_PIPE* emule_pipe, char* buffer, _int32 len);

_int32 emule_handle_recv_udp_cmd(char* buffer, _int32 len, _u32 ip, _u16 port);

_int32 emule_pipe_part_bitmap_to_can_download_ranges(EMULE_DATA_PIPE* emule_pipe, char** buffer, _int32* buffer_len);

_int32 emule_handle_file_not_found_cmd(EMULE_DATA_PIPE* emule_pipe, char* buffer, _int32 len);

_int32 emule_handle_req_filename_answer_cmd(EMULE_DATA_PIPE* emule_pipe, char* buffer, _int32 len);

_int32 emule_handle_file_status_cmd(EMULE_DATA_PIPE* emule_pipe, char* buffer, _int32 len);

_int32 emule_handle_part_hash_request_cmd(EMULE_DATA_PIPE* emule_pipe, char* buffer, _int32 len);
_int32 emule_handle_part_hash_answer_cmd(EMULE_DATA_PIPE* emule_pipe, char* buffer, _int32 len);

_int32 emule_handle_out_of_part_req_cmd(EMULE_DATA_PIPE* emule_pipe);

_int32 emule_handle_request_filename_cmd(EMULE_DATA_PIPE* emule_pipe, char* buffer, _int32 len);

_int32 emule_handle_queue_rank_cmd(EMULE_DATA_PIPE* emule_pipe, char* buffer, _int32 len);

_int32 emule_handle_emule_info_cmd(EMULE_DATA_PIPE* emule_pipe, char* buffer, _int32 len);

_int32 emule_handle_queue_ranking_cmd(EMULE_DATA_PIPE* emule_pipe, char* buffer, _int32 len);

_int32 emule_handle_request_sources_cmd(EMULE_DATA_PIPE* emule_pipe, char* buffer, _int32 len);

_int32 emule_pipe_handle_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid);


//_int32 emule_handle_compressed_part_cmd(EMULE_DATA_PIPE* emule_pipe, char* buffer, _int32 len, BOOL large_file);

#endif
#endif /* ENABLE_EMULE */
#endif


