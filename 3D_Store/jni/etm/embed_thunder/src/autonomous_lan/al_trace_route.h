/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/06/17
-----------------------------------------------------------------------------------------------------------*/

#ifndef _AL_TRACE_ROUTE_H_
#define _AL_TRACE_ROUTE_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#ifdef AUTO_LAN

#include "utility/define.h"
#include "platform/sd_socket.h"

_int32 al_start_trace(SOCKET sock, _u32* external_ip);

#endif

#ifdef __cplusplus
}
#endif

#endif


