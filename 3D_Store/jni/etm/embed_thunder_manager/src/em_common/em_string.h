#ifndef EM_STRING_H_00138F8F2E70_200806121335
#define EM_STRING_H_00138F8F2E70_200806121335

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/string.h"
#define em_strncpy sd_strncpy
#define em_vsnprintf sd_vsnprintf
#define em_snprintf sd_snprintf
#define em_vfprintf sd_vfprintf
#define em_fprintf sd_fprintf
#define em_printf sd_printf
#define em_strlen sd_strlen
#define em_strcat sd_strcat
#define em_strcmp sd_strcmp
#define em_strncmp sd_strncmp
#define em_stricmp sd_stricmp
#define em_strnicmp sd_strnicmp
#define em_strchr sd_strchr
#define em_strstr sd_strstr
#define em_strrchr sd_strrchr
#define em_memset sd_memset
#define em_memcpy sd_memcpy
#define em_memmove sd_memmove
#define em_memcmp sd_memcmp
#define em_trim_prefix_lws sd_trim_prefix_lws
#define em_trim_postfix_lws sd_trim_postfix_lws
#define em_sd_replace_str sd_replace_str


#ifdef __cplusplus
}
#endif

#endif
