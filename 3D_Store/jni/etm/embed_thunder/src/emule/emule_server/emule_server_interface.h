/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/10/28
-----------------------------------------------------------------------------------------------------------*/
#ifndef _EMULE_SERVER_INTERFACE_H_
#define _EMULE_SERVER_INTERFACE_H_
#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL

#include "emule/emule_server/emule_server.h"
#include "emule/emule_utility/emule_define.h"

_int32 emule_server_start(void);

_int32 emule_server_stop(void);

_int32 emule_server_close(void);

_int32 emule_server_query_source(_u8 file_id[FILE_ID_SIZE], _u64 file_size);

BOOL emule_is_local_server(_u32 ip, _u16 port);

#endif
#endif /* ENABLE_EMULE */
#endif


