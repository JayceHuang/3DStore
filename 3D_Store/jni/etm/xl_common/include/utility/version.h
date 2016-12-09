#ifndef SD_VERSION_H_00138F8F2E70_2008007261031
#define SD_VERSION_H_00138F8F2E70_2008007261031
#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"


#if defined(_DEBUG)
#define DEBUG_RELEASE_FLAG 1
#elif defined(_USE_INTERNAL_LICENSE)
#define DEBUG_RELEASE_FLAG 3
#else
#define DEBUG_RELEASE_FLAG 2
#endif

#define MKSTR(s) # s
#define __VERSION(__v) MKSTR(__v)
#define _VERSION(v, c, dr, b) __VERSION(v ## . ## c ## . ## dr ## . ## b)
#define VERSION(v, c, dr, b) _VERSION(v, c, dr, b)

/* @Simple Function@
 * Return : product id
 */
_int32 get_product_flag(void);

/* @Simple Function@
 * Return : product id
 */
_u32 get_product_id(void);

_int32 set_partner_id(_int32 partner_id);
_int32 get_partner_id(char *buffer, _int32 bufsize);

/* Get the operation system:system_name-release 
*   Return value:0 -success ,or failed
*/
_int32 sd_get_os(char *buffer, _int32 bufsize);
_int32 sd_get_device_name(char *buffer, _int32 bufsize);
_int32 sd_get_screen_size(_int32 *x, _int32 *y);
_int32 sd_get_hardware_version(char *buffer, _int32 bufsize);
#if defined(MACOS)&&defined(MOBILE_PHONE)
BOOL sd_is_ipad3(void);
#endif

_u32 sd_get_version_num();
_int32 sd_get_version_new(char *buff, _u32 buflen);
#ifdef __cplusplus
}
#endif
#endif
