/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2008/07/21
-----------------------------------------------------------------------------------------------------------*/
#ifndef _P2P_ACCEPTOR_H_
#define _P2P_ACCEPTOR_H_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"
#include "platform/sd_socket.h"

struct tagPTL_DEVICE;

_int32 p2p_handle_passive_accept(struct tagPTL_DEVICE** device, char* buffer, _u32 len);
#ifdef __cplusplus
}
#endif
#endif


