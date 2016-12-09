/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2008/08/14
-----------------------------------------------------------------------------------------------------------*/
#ifndef _LOCAL_IP_H_
#define _LOCAL_IP_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"


/* @Simple Function@
 * Return : the local ip in network-byte-order
 */
_u32 sd_get_local_ip(void);

void sd_set_local_ip(_u32 ip);

/* @Simple Function@
 * Return : whether if the ip is a nat-ip
 * ip is network byte order
 */
BOOL sd_is_in_nat(_u32 ip);	

_u32 sd_get_local_netmask(void);

#ifdef __cplusplus
}
#endif

#endif
