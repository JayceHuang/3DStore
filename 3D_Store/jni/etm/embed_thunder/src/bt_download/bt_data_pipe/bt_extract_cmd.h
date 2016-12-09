/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2009/02/27
-----------------------------------------------------------------------------------------------------------*/
#ifndef _BT_EXTRACT_CMD_H_
#define _BT_EXTRACT_CMD_H_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"
#ifdef ENABLE_BT 
#ifdef ENABLE_BT_PROTOCOL

#include "bt_download/bt_utility/bt_define.h"
#include "bt_download/bt_data_pipe/bt_data_pipe.h"
#include "utility/define.h"

_int32 bt_extract_handshake_cmd(char* buffer, _int32 len, _u8 info_hash[BT_INFO_HASH_LEN], 
    char peerid[BT_PEERID_LEN + 1], char resevered[8] );

_int32 bt_extract_bitfield_cmd(char* buffer, _int32 len, char** data, _u32* data_len);

_int32 bt_extract_have_cmd(char* buffer, _int32 len, _u32* index);

_int32 bt_extract_request_cmd(char* buffer, _int32 len, BT_PIECE_DATA* piece);

_int32 bt_extract_piece_cmd(char* buffer, _int32 len, BT_PIECE_DATA* piece);

_int32 bt_extract_cancel_cmd(char* buffer, _int32 len, BT_PIECE_DATA* piece);

_int32 bt_extract_port_cmd(char* buffer, _int32 len, _u16* port);

_int32 bt_extract_ex_handshake_cmd(char* buffer, _int32 len, _u32 *p_metadata_id, _u32 *p_metadata_size );

_int32 bt_extract_metadata(char* buffer, _int32 len, _u32 *p_piece_index, char **p_data, _u32 *p_data_len );

#endif
#endif

#ifdef __cplusplus
}
#endif

#endif
