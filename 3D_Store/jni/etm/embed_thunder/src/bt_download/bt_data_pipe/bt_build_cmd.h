/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2009/02/26
-----------------------------------------------------------------------------------------------------------*/
#ifndef _BT_BUILD_CMD_H_
#define _BT_BUILD_CMD_H_

#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"
#ifdef ENABLE_BT 
#ifdef ENABLE_BT_PROTOCOL

#include "bt_download/bt_utility/bt_define.h"

struct tagBITMAP;
struct tagBT_PIECE_DATA;

_int32 bt_build_handshake_cmd(char** buffer, _u32* len, 
    _u8 info_hash[BT_INFO_HASH_LEN], BOOL is_magnet);

_int32 bt_build_ex_handshake_cmd(char** buffer, _u32* len);

_int32 bt_build_metadata_request_cmd(char** buffer, _u32* len, _u32 metadata_type, _u32 piece_index );

_int32 bt_build_choke_cmd(char** buffer, _u32* len, BOOL is_choke);

_int32 bt_build_bitfield_cmd(char** buffer, _u32* len, const struct tagBITMAP* bitmap_i_have);

_int32 bt_build_interested_cmd(char** buffer, _u32* len, BOOL is_interest);

_int32 bt_build_have_cmd(char** buffer, _u32* len, _u32 index);

_int32 bt_build_request_cmd(char** buffer, _u32* len, struct tagBT_PIECE_DATA* piece);

_int32 bt_build_piece_cmd(char** buffer, _u32* len, struct tagBT_PIECE_DATA* piece, char* data);

_int32 bt_build_cancel_cmd(char** buffer, _u32* len, struct tagBT_PIECE_DATA* piece);

_int32 bt_build_a0_cmd(char** buffer, _u32* len);

_int32 bt_build_a1_cmd(char** buffer, _u32* len);

_int32 bt_build_a2_cmd(char** buffer, _u32* len, char* bt_peerid, _u32 ip, _u16 port);
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif

