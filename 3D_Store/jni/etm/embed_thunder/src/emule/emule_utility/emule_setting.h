/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/11/9
-----------------------------------------------------------------------------------------------------------*/
#ifndef _EMULE_SETTING_H_
#define _EMULE_SETTING_H_
#include "utility/define.h"
#ifdef ENABLE_EMULE

#include "emule/emule_utility/emule_define.h"

_int32 emule_get_user_id(_u8 user_id[USER_ID_SIZE]);

_int32 emule_set_user_id(_u8 user_id[USER_ID_SIZE]);

void emule_log_print_user_id(_u8 user_id[USER_ID_SIZE]);

BOOL emule_enable_p2sp(void);

BOOL emule_enable_server(void);

BOOL emule_enable_obfuscation(void);

BOOL emule_enable_compress(void);

BOOL emule_enable_source_exchange(void);

BOOL emule_designate_server(_u32* ip, _u16* port);

BOOL emule_enable_kad(void);

BOOL emule_enable_thunder_tag(void);
#endif /* ENABLE_EMULE */

#endif


