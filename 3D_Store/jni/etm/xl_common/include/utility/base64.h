#if !defined(__BASE64_H_20081021)
#define __BASE64_H_20081021

#ifdef __cplusplus
extern "C" 
{
#endif


#include "utility/define.h"

// BASE16解码后的长度
#define    DECODE_LENGTH_BASE16(len)		(len/2)
// BASE16编码后的长度
#define    ENCODE_LENGTH_BASE16(len)		(len*2)
// BASE32解码后的长度
#define    DECODE_LENGTH_BASE32(len)		((len*5)/8)
// BASE32编码后的长度
#define    ENCODE_LENGTH_BASE32(len)		((len*8)%5>0 ? (len*8)/5+1 : (len*8)/5)

_int32 sd_encode_base16(const _u8* input, _u32 input_len, char* output, _u32 output_len);

_int32 sd_decode_base16(const char* input, _u32 input_len, char* output, _u32 output_len);

_int32 sd_decode_base32(const char* input, _u32 input_len, char* output, _u32 output_len);
/* Convert BASE64  to char string
 * Make sure the length of bufer:d larger than 3/4 length of source string:s 
 * Returns: 0:success, -1:failed
 */
_int32 sd_base64_decode(const char *s,_u8 *d,int * output_size);

_int32 sd_base64_decode_v2(const _u8* in, _int32 in_len, char* out);
/* Convert char string to BASE64
 * Make sure the length of bufer:d larger than 4/3 length of source string:s 
 * Returns: 0:success, -1:failed
 */

_int32 sd_base64_encode(const _u8 *s,int input_size,char *d);

//_int32 sd_encode_base64(const char* input, _u32 input_len, char* output, _u32 output_len);

#ifdef __cplusplus
}
#endif

#endif /* __BASE64_H_20081021 */
