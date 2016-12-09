/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2009/02/12
-----------------------------------------------------------------------------------------------------------*/


#ifndef _AL_UTILITY_H_
#define	_AL_UTILITY_H_
#ifdef __cplusplus
extern "C" 
{
#endif

#ifdef AUTO_LAN

#include "utility/define.h"

_int32 al_build_icmp_packet(char** buffer, _int32* len, _u16 seq);


BOOL al_is_lan_ip(_u32 ip);

#endif

#ifdef __cplusplus
}
#endif

#endif


