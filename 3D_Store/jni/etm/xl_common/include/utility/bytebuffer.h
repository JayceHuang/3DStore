#ifndef SD_BYTEBUFFER_H_00138F8F2E70_200806171745
#define SD_BYTEBUFFER_H_00138F8F2E70_200806171745

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"

/* @Simple Function@
 * Return : none
 */
void bytebuffer_init(void);

/* transfer to host byte-order*/
_int32 sd_get_int64_from_bg(char **buffer, _int32 *cur_buflen, _int64 *value);
_int32 sd_get_int32_from_bg(char **buffer, _int32 *cur_buflen, _int32 *value);
_int32 sd_get_int16_from_bg(char **buffer, _int32 *cur_buflen, _int16 *value);

_int32 sd_get_int64_from_lt(char **buffer, _int32 *cur_buflen, _int64 *value);
_int32 sd_get_int32_from_lt(char **buffer, _int32 *cur_buflen, _int32 *value);
_int32 sd_get_int16_from_lt(char **buffer, _int32 *cur_buflen, _int16 *value);

_int32 sd_get_int8(char **buffer, _int32 *cur_buflen, _int8 *value);


/* Transfer from host byte-order */
_int32 sd_set_int64_to_bg(char **buffer, _int32 *cur_buflen, _int64 value);
_int32 sd_set_int32_to_bg(char **buffer, _int32 *cur_buflen, _int32 value);
_int32 sd_set_int16_to_bg(char **buffer, _int32 *cur_buflen, _int16 value);

_int32 sd_set_int64_to_lt(char **buffer, _int32 *cur_buflen, _int64 value);
_int32 sd_set_int32_to_lt(char **buffer, _int32 *cur_buflen, _int32 value);
_int32 sd_set_int16_to_lt(char **buffer, _int32 *cur_buflen, _int16 value);

_int32 sd_set_int8(char **buffer, _int32 *cur_buflen, _int8 value);

_int32 sd_get_bytes(char **buffer, _int32 *cur_buflen, char *dest_buf, _int32 dest_len);
_int32 sd_set_bytes(char **buffer, _int32 *cur_buflen, char *dest_buf, _int32 dest_len);


#ifdef __cplusplus
}
#endif

#endif
