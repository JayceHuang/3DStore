
#ifdef AUTO_LAN
#include "utility/define.h"
#include "utility/errcode.h"
#include "utility/logid.h"
#define LOGID LOGID_AUTO_LAN
#include "utility/logger.h"

#include "al_interface.h"
#include "al_impl.h"

/************************inter function******************************/
BOOL is_al_enable(void)
{
	return al_is_enable();
}

/************************out function******************************/
_int32 init_al_modular(void)
{
	LOG_DEBUG("init_al_modular.");
	return al_start();
}

_int32 uninit_al_modular(void)
{
	LOG_DEBUG("uninit_al_modular.");
	al_stop();
	return SUCCESS;
}

void al_set_speed_limit(_u32 download_limit_speed, _u32 upload_limit_speed)
{
	al_set_max_speed_limit( download_limit_speed,  upload_limit_speed);
}
#else
#if defined(MACOS)&&(!defined(MOBILE_PHONE))
_int32 al_empty(void)
{
	return -1;
}
#endif

#endif

