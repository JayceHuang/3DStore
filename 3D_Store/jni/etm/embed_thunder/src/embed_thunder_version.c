#ifdef ENABLE_ETM_MEDIA_CENTER
#include "embed_thunder_version.h"

_int32 get_version(char *buffer, _int32 bufsize)
{
	sd_get_version_new(buffer, bufsize);
	return 0;
}
#else
#include "embed_thunder_version.h"
#include "utility/define_const_num.h"
#include "utility/string.h"
#include "utility/errcode.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_COMMON
#include "utility/logger.h"

static char g_et_version[MAX_VERSION_LEN] = {0};

_int32 get_version(char *buffer, _int32 bufsize)
{
	if (sd_strlen(g_et_version)!=0)
	{
		sd_strncpy(buffer, g_et_version, bufsize);
	}
	else
	{
		sd_strncpy(buffer, ET_INNER_VERSION, bufsize);
	}
    LOG_DEBUG("get_et_version : %s", buffer);
	return SUCCESS;
}

_int32 set_et_version(const char *version)
{
	sd_snprintf(g_et_version, MAX_VERSION_LEN-1, "%s_%s", ET_INNER_VERSION, version);
	LOG_DEBUG("set_et_version : %s", g_et_version);
	return SUCCESS;
}
#endif
