/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2008/10/9
-----------------------------------------------------------------------------------------------------------*/
#ifndef _PTL_UTILITY_H_
#define _PTL_UTILITY_H_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"

void ptl_set_local_tcp_port(_u16 tcp_port);

_u16 ptl_get_local_tcp_port(void);

void ptl_set_local_udp_port(_u16 udp_port);

_u16 ptl_get_local_udp_port(void);

#ifdef __cplusplus
}
#endif
#endif
