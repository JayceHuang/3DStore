/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/10/14
-----------------------------------------------------------------------------------------------------------*/
#ifndef _EMULE_UTILITY_H_
#define _EMULE_UTILITY_H_

#include "utility/define.h"
#ifdef ENABLE_EMULE
#include "emule/emule_utility/emule_define.h"

#define EMULE_DEFAULT_LOW_CLIENT_ID     16777216

#define IS_LOWID(id)			(id<16777216)		// ÊÇ·ñÊÇLowID

_int32 emule_get_nick_name(char* nick_name, _u32 len);

_int32 emule_get_user_id_type(_u8 user_id[USER_ID_SIZE]);

_int32 emule_log_print_file_id(_u8 file_id[FILE_ID_SIZE], const char *desc);

_int32 emule_log_print_gcid(_u8 gcid[CID_SIZE], const char *desc);

#endif /* ENABLE_EMULE */

#endif


