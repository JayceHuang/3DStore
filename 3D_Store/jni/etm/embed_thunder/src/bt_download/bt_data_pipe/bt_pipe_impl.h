/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2009/02/26
-----------------------------------------------------------------------------------------------------------*/

#ifndef _BT_PIPE_IMPL_H_
#define _BT_PIPE_IMPL_H_

#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"
#ifdef ENABLE_BT 
#ifdef ENABLE_BT_PROTOCOL

#include "bt_download/bt_data_pipe/bt_data_pipe.h"
#include "bt_download/bt_data_pipe/bt_device.h"
#include "connect_manager/resource.h"
#include "asyn_frame/msg.h"
#include "utility/bitmap.h"

struct _tag_range_data_buffer;
struct tagTMP_FILE;

_int32 bt_pipe_notify_connnect_success(BT_DATA_PIPE* bt_pipe);

_int32 bt_pipe_notify_send_success(BT_DATA_PIPE* bt_pipe);

_int32 bt_pipe_notify_recv_success(BT_DATA_PIPE* bt_pipe);

_int32 bt_pipe_notify_recv_data(BT_DATA_PIPE* bt_pipe);

_int32 bt_pipe_notify_close(BT_DATA_PIPE* bt_pipe);

_int32 bt_pipe_handle_error(_int32 errcode, BT_DATA_PIPE* bt_pipe);

_int32 bt_pipe_handle_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid);

_int32 bt_pipe_get_data_buffer_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid);

_int32 bt_pipe_request_data(BT_DATA_PIPE* bt_pipe);

_int32 bt_pipe_read_data_callback(void* user_data, BT_RANGE* bt_range, char* data_buffer, _int32 read_result, _u32 read_len);

_int32 bt_pipe_upload_piece_data(BT_DATA_PIPE* bt_pipe);

void bt_pipe_change_state(BT_DATA_PIPE* bt_pipe, PIPE_STATE state);

/*发送数据包*/
_int32 bt_pipe_send_handshake_cmd(BT_DATA_PIPE* bt_pipe);

_int32 bt_pipe_send_ex_handshake_cmd(BT_DATA_PIPE* bt_pipe);

_int32 bt_pipe_send_metadata_request_cmd(BT_DATA_PIPE* bt_pipe, _u32 piece_index);

_int32 bt_pipe_send_choke_cmd(BT_DATA_PIPE* bt_pipe, BOOL is_choke);

_int32 bt_pipe_send_bitfield_cmd(BT_DATA_PIPE* bt_pipe, const BITMAP* bitmap_i_have);

_int32 bt_pipe_send_interested_cmd(BT_DATA_PIPE* bt_pipe, BOOL is_interested);

_int32 bt_pipe_send_have_cmd(BT_DATA_PIPE* bt_pipe, _u32 piece_index);

_int32 bt_pipe_send_request_cmd(BT_DATA_PIPE* bt_pipe, BT_PIECE_DATA* piece);

_int32 bt_pipe_send_cancel_cmd(BT_DATA_PIPE* bt_pipe, BT_PIECE_DATA* piece);

_int32 bt_pipe_send_piece_cmd(BT_DATA_PIPE* bt_pipe, BT_PIECE_DATA* piece, char* data);

_int32 bt_pipe_send_a0_cmd(BT_DATA_PIPE* bt_pipe);

_int32 bt_pipe_send_a1_cmd(BT_DATA_PIPE* bt_pipe);

_int32 bt_pipe_send_a2_cmd(BT_DATA_PIPE* bt_pipe);

_int32 bt_pipe_send_keepalive_cmd(BT_DATA_PIPE* bt_pipe);

/*处理收到的数据包*/
_int32 bt_pipe_recv_cmd_package(BT_DATA_PIPE* bt_pipe, _u8 cmd_type);

_int32 bt_pipe_handle_choke_cmd(BT_DATA_PIPE* bt_pipe);

_int32 bt_pipe_handle_unchoke_cmd(BT_DATA_PIPE* bt_pipe);

_int32 bt_pipe_handle_interested_cmd(BT_DATA_PIPE* bt_pipe);

_int32 bt_pipe_handle_not_interested_cmd(BT_DATA_PIPE* bt_pipe);

_int32 bt_pipe_handle_have_cmd(BT_DATA_PIPE* bt_pipe);
	
_int32 bt_pipe_handle_bitfield_cmd(BT_DATA_PIPE* bt_pipe);

_int32 bt_pipe_handle_request_cmd(BT_DATA_PIPE* bt_pipe);

_int32 bt_pipe_handle_piece_cmd(BT_DATA_PIPE* bt_pipe);

_int32 bt_pipe_handle_cancel_cmd(BT_DATA_PIPE* bt_pipe);

_int32 bt_pipe_handle_port_cmd(BT_DATA_PIPE* bt_pipe);

_int32 bt_pipe_handle_handshake_cmd(BT_DATA_PIPE* bt_pipe);

_int32 bt_pipe_handle_ex_handshake_cmd(BT_DATA_PIPE* bt_pipe,
	char *p_data_buffer, _u32 data_len );

_int32 bt_pipe_handle_magnet_cmd(BT_DATA_PIPE* bt_pipe);

_int32 bt_pipe_handle_metadata_cmd(BT_DATA_PIPE* bt_pipe, 
	char *p_data_buffer, _u32 data_len );

_int32 bt_pipe_handle_a0_cmd(BT_DATA_PIPE* bt_pipe);

_int32 bt_pipe_handle_a1_cmd(BT_DATA_PIPE* bt_pipe);

_int32 bt_pipe_handle_a2_cmd(BT_DATA_PIPE* bt_pipe);

_int32 bt_pipe_handle_a3_cmd(BT_DATA_PIPE* bt_pipe);

_int32 bt_pipe_handle_ac_cmd(BT_DATA_PIPE* bt_pipe);

_int32 bt_pipe_handle_keepalive_cmd(BT_DATA_PIPE* bt_pipe);

/*辅助函数*/
BOOL bt_pipe_is_piece_valid(BT_DATA_PIPE* bt_pipe, BT_PIECE_DATA* piece);

_int32 bt_range_to_piece_data(BT_DATA_PIPE* bt_pipe, const BT_RANGE* down_range, LIST* piece_list);

#ifdef UPLOAD_ENABLE

enum tagPEER_UPLOAD_STATE;

void bt_pipe_change_upload_state(BT_DATA_PIPE* bt_pipe, enum tagPEER_UPLOAD_STATE state);
#endif

#endif
#endif

#ifdef __cplusplus
}
#endif

#endif

