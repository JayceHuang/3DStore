#ifndef EMBED_THUNDER_VERSION_H
#define EMBED_THUNDER_VERSION_H

#include "utility/define.h"
#include "utility/version.h"

_int32 get_version(char *buffer, _int32 bufsize);
_int32 set_et_version(const char *version);

#ifdef MACOS

#ifdef MOBILE_PHONE
#define ET_INNER_VERSION ("1.12.2.0")		/* 手机迅雷下载库为2,iPad 看看为3,android 为4,MAC迅雷为5 */
#else
#define ET_INNER_VERSION ("1.12.2.0")
#endif // MOBILE_PHONE

#elif defined(_ANDROID_LINUX)

#define ET_INNER_VERSION ("1.12.2.0")
//#define ET_INNER_VERSION VERSION(VERSION_NUM, _PACKAGE_NUM, DEBUG_RELEASE_FLAG, BUILD_NUM)
#else
#define ET_INNER_VERSION ("1.12.2.0")

#endif


#endif

