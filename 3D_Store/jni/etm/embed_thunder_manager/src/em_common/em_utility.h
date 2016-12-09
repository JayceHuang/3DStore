#ifndef EM_UTILITY_H_00138F8F2E70_200806182142
#define EM_UTILITY_H_00138F8F2E70_200806182142

#include "em_common/em_define.h"
#include "utility/utility.h"
#include "utility/list.h"

#define em_str2hex str2hex
#define em_atoi sd_atoi
#define em_is_cid_equal sd_is_cid_equal
#define em_is_cid_valid sd_is_cid_valid
#define em_string_to_cid sd_string_to_cid
#define em_i32toa sd_i32toa
#define em_i64toa sd_i64toa
#define em_u32toa sd_u32toa
#define em_u64toa sd_u64toa

#define em_digit_bit_count sd_digit_bit_count
#define em_u32_to_str sd_u32_to_str
#define em_u64_to_str sd_u64_to_str
#define em_str_to_u64 sd_str_to_u64


/* Convert BASE64  to char string
 * Make sure the length of bufer:d larger than 3/4 length of source string:s 
 * Returns: 0:success, -1:failed
 */
_int32 em_base64_decode(const char *s,_u32 str_len,char *d);
_int32 em_replace_7c(char * str);
char * em_get_file_name_from_url( char * url,_u32 url_length);
/* 用bt任务的info_hash 生成磁力链*/
_int32 em_generate_magnet_url(_u8 info_hash[INFO_HASH_LEN], const char * display_name,_u64 total_size,char * url_buffer ,_u32 buffer_len);
_int32 em_parse_magnet_url(const char * url ,_u8 info_hash[INFO_HASH_LEN], char * name_buffer,_u64 * file_size);

_int32 em_hex_2_int(char chr);
//将所有的带有'%' 的字符去除'%'，包括保留字符'/','?','@',':'...
_int32 em_decode_ex(const char* src_str, char * str_buffer ,_u32 buffer_len);
BOOL em_decode_file_name(char * _file_name,char * _file_suffix);
_int32 em_get_valid_name(char * _file_name,char * file_suffix);
_u32 em_digit_bit_count( _u64 value );
_int32 em_u32_to_str( _u32 val, char *str, _u32 strlen );
_int32 em_u64_to_str( _u64 val, char *str, _u32 strlen );
_int32 em_str_to_u64( const char *str, _u32 strlen, _u64 *val );

_int32 em_license_encode(const char * license,char * buffer);
_int32 em_license_decode(const char * license_encoded,char * license_buffer);

/* 从html文件中获得可供下载的url  */
typedef enum t_em_url_type
{
	UT_NORMAL = -1,		/*不可下载的url*/
	UT_HTTP = 0, 				
	UT_FTP, 
	UT_THUNDER, 
	UT_EMULE, 
	UT_MAGNET, 				/* 磁力链 */
	UT_BT, 					/* 指向torrent 文件的url */
	UT_HTTP_PARTIAL,		/*只有相对路径的http 链接*/
	UT_BT_PARTIAL			/*只有相对路径的bt 链接*/
} EM_URL_TYPE;

/* 文件类型 */
typedef enum t_em_file_type
{
	EFT_UNKNOWN = 0, 				
	EFT_VEDIO , 					
	EFT_PIC , 					
	EFT_AUDIO , 					
	EFT_ZIP , 		/* 各种压缩文档,如zip,rar,tar.gz... */			
	EFT_TXT , 					
	EFT_APP,		/* 各种软件,如exe,apk,dmg,rpm... */			
	EFT_OTHER					
} EM_FILE_TYPE;

typedef enum t_em_url_status
{
	EUS_UNTREATED = 0, 		/* 未处理 */			
	EUS_IN_LOCAL, 				/* 已经下载到本地 */
	EUS_IN_LIXIAN				/* 已经在离线空间里了 */
} EM_URL_STATUS;

typedef struct t_em_downloadable_url
{
	_u32 _url_id;
	EM_URL_TYPE _type;
	EM_FILE_TYPE _file_type;				/*  URL 指向的文件类型*/
	EM_URL_STATUS _status;				/* 该URL 的处理状态*/
	_u64 _file_size;
	char _name[MAX_FILE_NAME_BUFFER_LEN];
	char _url[MAX_URL_LEN];
} EM_DOWNLOADABLE_URL;
_int32 em_get_downloadable_url_from_webpage(const char* webpage_file_path,char* url,_u32 * p_downloadable_num,EM_DOWNLOADABLE_URL ** pp_downloadable );

/* 判断url是否可以下载 */
BOOL em_is_url_downloadable( const char * url);
// 获取url的类型，如果是不可下载的url，则返回-1
EM_URL_TYPE em_get_url_type(const char* url);


/*通过product_id设置全局的嗅探扩展名*/
void set_detect_file_suffixs_by_product_id(_int32 product_id);

/*嗅探正则表达式*/
#define EM_REGEX_LEN 128  //尚不熟悉一般正则表达式的长度，暂定128，以后调整
typedef struct t_em_detect_regex
{
    _int32 _matching_scheme;             /*嗅探时所采用的匹配方案*/
    char _start_string[EM_REGEX_LEN];   /*用于限定区间的开始字符串*/
    char _end_string[EM_REGEX_LEN];     /*用于限定区间的结束字符串*/
	char _download_url[EM_REGEX_LEN];		/*下载链接表达式*/
	char _name[EM_REGEX_LEN];			/*文件名表达式*/
	char _name_append[EM_REGEX_LEN];	/*文件名附加描述表达式*/
	char _suffix[EM_REGEX_LEN];			/*后缀名表达式*/
	char _file_size[EM_REGEX_LEN];			/*文件大小表达式*/
}EM_DETECT_REGEX;

/*嗅探正则表达式*/
#define EM_DETECT_STRING_LEN 64  //尚不熟悉一般正则表达式的长度，暂定64，以后调整
typedef struct t_em_detect_string
{
       _int32 _matching_scheme;             /*嗅探时所采用的匹配方案*/
       char _string_start[EM_DETECT_STRING_LEN];   /*用于限定区间的开始字符串*/
	_int32 _string_start_include;  		/*是否包含开始字符串，1为包含*/
       char _string_end[EM_DETECT_STRING_LEN];     /*用于限定区间的结束字符串*/
	_int32 _string_end_include;		/*是否包含结束字符串，1为包含*/
	char _download_url_start[EM_DETECT_STRING_LEN];		/*下载链接表达式*/
	_int32 _download_url_start_include;		
	char _download_url_end[EM_DETECT_STRING_LEN];
	_int32 _download_url_end_include;
	char _name_start[EM_DETECT_STRING_LEN];			/*文件名表达式*/
	_int32 _name_start_include;	
	char _name_end[EM_DETECT_STRING_LEN];	
	_int32 _name_end_include;	
	char _name_append_start[EM_DETECT_STRING_LEN];	/*文件名附加描述表达式*/
	_int32 _name_append_start_include;
	char _name_append_end[EM_DETECT_STRING_LEN];
	_int32 _name_append_end_include;
	char _suffix_start[EM_DETECT_STRING_LEN];			/*后缀名表达式*/
	_int32 _suffix_start_include;		
	char _suffix_end[EM_DETECT_STRING_LEN];	
	_int32 _suffix_end_include;		
	char _file_size_start[EM_DETECT_STRING_LEN];			/*文件大小表达式*/
	_int32 _file_size_start_include;		
	char _file_size_end[EM_DETECT_STRING_LEN];	
	_int32 _file_size_end_include;		
}EM_DETECT_STRING;

/*需要通过配置规则(正则表达式或定位字符串)嗅探的特殊网站*/
typedef struct t_em_detect_special_web
{
	char _website[MAX_URL_LEN];			/*网站域名*/
	LIST * _rule_list;						/*规则列表*/
}EM_DETECT_SPECIAL_WEB;

_int32 em_set_detect_regex(LIST* regex_list);
_int32 em_set_detect_string(LIST* detect_string_list);

#endif

