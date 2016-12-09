#ifndef ET_UTILITY_H_00138F8F2E70_200806182142
#define ET_UTILITY_H_00138F8F2E70_200806182142

#include "utility/utility.h"
#include "utility/list.h"

_int32 et_replace_7c(char * str);
_int32 et_base64_decode(const char *s,_u32 str_len,char *d);

char * et_get_file_name_from_url( char * url,_u32 url_length, char *out_name, _u32 out_len);
_int32 et_parse_magnet_url(const char * url ,_u8 info_hash[INFO_HASH_LEN], char * name_buffer,_u64 * file_size);

//获得某个目录下唯一的文件名，如果成功，会返回SUCCESS, 并直接会修改_file_name变量
_int32 get_unique_file_name_in_path(const char *_path, char *_file_name, _u32 _file_name_buf_len);
#endif //ET_UTILITY_H_00138F8F2E70_200806182142

