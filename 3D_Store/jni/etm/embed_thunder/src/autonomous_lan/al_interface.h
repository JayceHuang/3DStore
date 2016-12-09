/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2009/02/10
-----------------------------------------------------------------------------------------------------------*/

#ifndef _AL_INTERFACE_H_
#define	_AL_INTERFACE_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#ifdef AUTO_LAN

#include "utility/define.h"

_int32 init_al_modular(void);

_int32 uninit_al_modular(void);

void al_set_speed_limit(_u32 download_limit_speed, _u32 upload_limit_speed);

#endif

#ifdef __cplusplus
}
#endif

#endif

