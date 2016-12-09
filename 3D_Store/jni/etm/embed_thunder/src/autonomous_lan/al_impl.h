#ifndef _AL_IMPL_H_
#define _AL_IMPL_H_
#ifdef __cplusplus
extern "C" 
{
#endif

#ifdef AUTO_LAN

#include "utility/define.h"

_int32 al_start(void);

_int32 al_stop(void);

BOOL al_is_enable(void);

void al_set_max_speed_limit(_u32 download_limit_speed, _u32 upload_limit_speed);

_int32 al_on_ping_result(BOOL result, _u32 delay);

#endif

#ifdef __cplusplus
}
#endif

#endif


