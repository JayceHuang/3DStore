
/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/11/16
-----------------------------------------------------------------------------------------------------------*/
#ifndef _EMULE_ACCEPT_H_
#define _EMULE_ACCEPT_H_

#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL
#include "platform/sd_socket.h"

_int32 emule_create_acceptor(void);

_int32 emule_close_acceptor(void);

_int32 emule_notify_accept(_int32 errcode, _u32 pending_op_count, SOCKET conn_sock, void* user_data);
#endif /* ENABLE_EMULE */
#endif /* ENABLE_EMULE */

#endif


