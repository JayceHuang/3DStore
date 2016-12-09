#ifndef SD_UTILITY_H_00138F8F2E70_200806182142
#define SD_UTILITY_H_00138F8F2E70_200806182142
#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"
#include "utility/time.h"

#define SD_RAND_MAX	(2147483647)

#if defined(LINUX)
#ifndef MIN
#define MIN(n1, n2)  ((n1) < (n2) ? (n1) : (n2))
#endif
#ifndef MAX
#define MAX(n1, n2)  ((n1) > (n2) ? (n1) : (n2))
#endif

#elif defined(WINCE)
#ifndef MIN
#define MIN(n1, n2)  ((n1) < (n2) ? (n1) : (n2))
#endif
#ifndef MAX
#define MAX(n1, n2)  ((n1) > (n2) ? (n1) : (n2))
#endif

#endif

#define IS_DIGIT(chr)  ((((chr) >= '0')&&((chr) <= '9'))? TRUE : FALSE)
#define IS_ALPHA(chr)  ((((chr) >= 'a')&&((chr) <= 'z'))||(((chr) >= 'A')&&((chr) <= 'Z')))? TRUE : FALSE)
#define IS_HEX(chr)  (((((chr) >= '0')&&((chr) <= '9'))|| (((chr) >= 'a')&&((chr) <= 'f'))||(((chr) >= 'A')&&((chr) <= 'F')))? TRUE : FALSE)

/* @Simple Function@
 * Return : the binary data in network byte-order of cp
 */
_u32 sd_inet_addr(const char *cp);

/* same as sd_inet_addr, but can be return errcode */
_int32 sd_inet_aton(const char *cp, _u32 *nip);

/* inet_ntoa */
_int32 sd_inet_ntoa(_u32 inp, char *out_buf, _int32 bufsize);

/* @Simple Function@
 * Return : network byte-order of h
 */
_u16 sd_htons(_u16 h);

/* @Simple Function@
 * Return : network byte-order of h
 */
_u32 sd_htonl(_u32 h);

/* @Simple Function@
 * Return : host byte-order of h
 */
_u16 sd_ntohs(_u16 n);

/* @Simple Function@
 * Return : host byte-order of h
 */
_u32 sd_ntohl(_u32 n);

/* e.g. 'A' to 41 */
_int32 char2hex( unsigned char c , char *out_buf, _int32 out_bufsize);
/* hex */
_int32 str2hex(const char *in_buf, _int32 in_bufsize, char *out_buf, _int32 out_bufsize);

//将Hex字符串转换成普通字符串
_int32 hex2str(char* hex, _int32 hex_len, char* str, _int32 str_len);


//将十六制字符表示为十进制整数,如'A'转换为10
_int32 sd_hex_2_int(char chr);
//将十六进制字符串转换为十进制整数,不考虑大数
_int32 sd_hexstr_2_int(const char * in_buf, _int32 in_bufsize);
/* @Simple Function@
 * Return : the number described by nptr
 * 
 */
_int32 sd_atoi(const char* nptr);

_int32 sd_srand(_u32 seeds);

/* @Simple Function@
 * Return : random integer between 0 - SD_RAND_MAX
 *
 */
_int32 sd_rand(void);

/* @Simple Function@
 * Return : TRUE if equal,or FALSE
 *
 */
BOOL sd_is_bcid_equal(const _u8* _bcid1,_u32 bcid1_len,const _u8* _bcid2,_u32 bcid2_len);

/* @Simple Function@
 * Return : TRUE if equal,or FALSE
 *
 */
BOOL sd_is_cid_equal(const _u8* _cid1,const _u8* _cid2);

/* @Simple Function@
 * Return : TRUE if valid,or FALSE
 *
 */
BOOL sd_is_cid_valid(const _u8* _cid);

BOOL sd_is_bcid_valid(_u64 filesize, _u32 bcid_size, _u32 block_size);


/* @Simple Function@
 * Return : the hashvalue(elf) of str
 *
 */
_u32 sd_elf_hashvalue(const char *str, _u32 hash_value);
_u64 sd_generate_hash_from_size_crc_hashvalue(const _u8 * data ,_u32 data_size);

/* str to val */
_int32 sd_str_to_u64( const char *str, _u32 strlen, _u64 *val );

/* val to str */
_int32 sd_u64_to_str( _u64 val, char *str, _u32 strlen );
_int32 sd_u32_to_str( _u32 val, char *str, _u32 strlen );

_int32 sd_abs(_int32 val);

BOOL sd_data_cmp(_u8* i_data, _u8* d_data, _int32 count);
/*
	Convert the string(40 chars hex ) to content id(20 bytes in hex)
*/
_int32 sd_string_to_cid(char * str,_u8 *cid);
/*
	Convert the string(chars hex ) to  bytes in hex
*/
_int32 sd_string_to_hex(char * str,_u8 * hex);


_int32 sd_i32toa(_int32 value, char *buffer, _int32 buf_len, _int32 radix);
_int32 sd_i64toa(_int64 value, char *buffer, _int32 buf_len, _int32 radix);

_int32 sd_u32toa(_u32 value, char *buffer, _int32 buf_len, _int32 radix);
_int32 sd_u64toa(_u64 value, char *buffer, _int32 buf_len, _int32 radix);

_u32 sd_digit_bit_count( _u64 value );
BOOL is_cmwap_prompt_page(const char * http_header,_u32 header_len);

_int32 sd_calc_file_cid(const char* file_path, _u8 * hex_cid);
_int32 sd_calc_buf_cid(const char *buf, int buf_len, _u8 *p_cid);
_u32 sd_calc_gcid_part_size(_u64 file_size);
_int32 sd_calc_file_gcid(const char* file_path, _u8* gcid, _int32 * bcid_num, _u8 ** bcid_buf,_u64 * file_size);

/*xml字符串中的实体引用替换*/
_int32 sd_xml_entity_ref_replace(char* xml_str,_u32 total_buffer_len);

_int32 sd_get_file_type_from_name(const char *p_file_name);

void sd_exit(_int32 errcode);

_int32 sd_cid_to_hex_string(_u8* cid, _int32 cid_len,  _u8* hex_cid_str, _int32 hex_cid_len);

_int32 sd_gm_time(_u32 time_sec, TIME_t *p_time);

_int32 sd_str_to_i64_v2(const char* str, _u32 len, _int64* val);

_int32 sd_getaddrinfo(const char *host, char *ip, int ip_len);

#ifdef __cplusplus
}
#endif
#endif

