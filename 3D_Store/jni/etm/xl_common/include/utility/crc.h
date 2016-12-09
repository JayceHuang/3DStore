#if !defined(__CRC16_H_20081105)
#define __CRC16_H_20081105

#ifdef __cplusplus
extern "C" 
{
#endif


#include "utility/define.h"

BOOL sd_isvalid_crc16(_u16 old_fcs, _u16 crc);
_u16 sd_add_crc16(_u16 fcs, const void *pdata, _int32 len);
_u16 sd_get_crc16(const void *pdata, _int32 len);
_u16 sd_inv_crc16(_u16 fcs);


#ifdef __cplusplus
}
#endif

#endif /* __CRC16_H_20081105 */
