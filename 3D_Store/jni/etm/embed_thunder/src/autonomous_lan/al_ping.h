/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2009/06/17
-----------------------------------------------------------------------------------------------------------*/

#ifndef _AL_PING_H_
#define	_AL_PING_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#ifdef AUTO_LAN

#include "utility/define.h"
#include "platform/sd_socket.h"

#define	PING_NUM	5
#define	PING_TIMEOUT	4000

_int32 al_start_ping(SOCKET sock, _u32 dest_ip);

BOOL al_is_ping_valid(SOCKET sock, _u32 dest_ip);

#endif

#ifdef __cplusplus
}
#endif

#endif


