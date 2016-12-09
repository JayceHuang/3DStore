#ifndef SD_ICONV_H_20081229
#define SD_ICONV_H_20081229

#include "utility/define.h"

#ifdef __cplusplus
extern "C" 
{
#endif

enum CODE_PAGE { _CP_ACP = 0, _CP_GBK , _CP_UTF8 ,_CP_BIG5 };

enum GBK_CHAR_TYPE
{
    _STAND_COMMON_CHAR=1,
    _STAND_CHAR=3,
    _STAND_CHAR_EXTEND_1=4,
    _STAND_CHAR_EXTEND_2=8,
    _STAND_FLAG=16,
    STAND_FLAG_EXTEND=32,
    _UNKNOWN_TYPE = 0x80
};
    
/*	猜测字符串的编码格式:CP_ACP=0, CP_GBK , CP_UTF8
 
Notice:
   该函数用于猜测字符串的编码格式，只能猜测以下四种编码的字符串：
	纯ASCII编码返回CP_ACP；中文简体GBK编码返回CP_GBK；UTF8编码返回CP_UTF8；中文繁体BIG5返回CP_BIG5。
   由于该函数不一定100%能猜正确，所以，根据不同的返回值，有以下情况需要注意：
	1.如果返回值为CP_ACP，则该字符串一定是纯ASCII编码，不会是其他格式的编码；
	2.如果返回值为UTF8，则该字符串一定是UTF8编码，不会是其他格式的编码；
	3.如果返回值为CP_BIG5，则该字符串也有可能是UTF8编码，但绝不会是CP_ACP或CP_GBK格式的编码；这种情况需要多加注意。
	4.如果返回值为CP_GBK，则该字符串也有可能是CP_BIG5或UTF8编码，但绝不会是纯ASCII编码；这种情况也需要多加注意。

*/
enum CODE_PAGE sd_conjecture_code_page( const char* src_str );

enum GBK_CHAR_TYPE sd_get_gbk_type( _u16  char_value );
BOOL sd_is_gbk_type(enum GBK_CHAR_TYPE src, enum GBK_CHAR_TYPE dst);
    
_int32 sd_utf8_2_gbk(const char *p_input, _u32 input_len, char *p_output, _u32 *output_len);
_int32  sd_gbk_2_utf8(const char *p_input, _u32 input_len, char *p_output, _u32 *output_len);
_int32 sd_utf8_2_big5(const char *p_input, _u32 input_len, char *p_output, _u32 *output_len);
_int32  sd_big5_2_utf8(const char *p_input, _u32 input_len, char *p_output, _u32 *output_len);

_int32 sd_big5_2_gbk(const char *p_input, _u32 input_len, char *p_output, _u32 *output_len);

_int32 sd_unicode_2_gbk(const _u16 *p_input, _u32 input_len, char *p_output, _u32 *output_len);
_int32  sd_gbk_2_unicode(const char *p_input, _u32 input_len, _u16 *p_output, _u32 *output_len);

_int32 sd_utf8_2_unicode(_u8* utf8, _u32 input_len, _u16* unicode,_u32 *output_len);
_int32 sd_unicode_2_utf8(_u16* unicode,_u32 input_len, _u8* utf8,_u32 *output_len);

/* 如果成功, *output_len返回占用内存长度(byte)*/
_int32 sd_any_format_to_gbk( const char *input,_u32 input_len, _u8* gbk,_u32 *output_len);
/* 如果成功, *output_len返回占用内存长度(byte)*/
_int32 sd_any_format_to_utf8( const char *input,_u32 input_len, char * p_utf8,_u32 *output_len);
/* 如果成功, *output_len返回unicode字符(两字节)个数*/
_int32 sd_any_format_to_unicode( const char *input,_u32 input_len, _u16* p_unicode,_u32 *output_len);

BOOL sd_is_acp_page_code(const char* str, _int32 len);
#ifdef __cplusplus
}
#endif


#endif  /* SD_ICONV_H_20081229 */

