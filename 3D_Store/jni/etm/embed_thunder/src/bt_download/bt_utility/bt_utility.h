/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2009/02/24
-----------------------------------------------------------------------------------------------------------*/
#ifndef _BT_UTILITY_H_
#define _BT_UTILITY_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#ifdef ENABLE_BT 

/*注意dest_len的长度至少是src_len长度的三倍，否则可能会失败*/
_int32 bt_escape_string(const char* src, _int32 src_len, char* dest, _int32* dest_len);

_int32 bt_get_local_peerid(char* bt_peerid, _u32 bt_peerid_len);

BOOL bt_is_bitcomet_peer(char* peerid);

BOOL bt_is_UT_peer(char* peerid);

void bt_get_peer_hash_value(_u32 ip, _u16 port, _u8* info_hash, _u32* result);
#endif

#ifdef __cplusplus
}
#endif
#endif

