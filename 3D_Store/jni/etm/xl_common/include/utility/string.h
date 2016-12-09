#ifndef SD_STRING_H_00138F8F2E70_200806121335
#define SD_STRING_H_00138F8F2E70_200806121335

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#include "utility/arg.h"
struct tagLIST;

_int32 sd_strncpy(char *dest, const char *src, _int32 size);

_int32 sd_vsnprintf(char *buffer, _int32 bufsize, const char *fmt, sd_va_list ap);
_int32 sd_snprintf(char *buffer, _int32 bufsize, const char *fmt, ...);

_int32 sd_vfprintf(_int32 fd, const char *fmt, sd_va_list ap);
_int32 sd_fprintf(_int32 fd, const char *fmt, ...);

_int32 sd_printf(const char *fmt, ...);


/* @Simple Function@
 * Return : length of str
 */
_int32 sd_strlen(const char *str);

_int32 sd_strcat(char *dest, const char *src, _int32 n);


/* @Simple Function@
 * Return :   -1: s1<s2  0: s1==s2     1: s1>s2
 */
_int32 sd_strcmp(const char *s1, const char *s2);

_int32 sd_strncmp(const char *s1, const char *s2, _int32 n);

/* @Simple Function@
 * Return :   -1: s1<s2  0: s1==s2     1: s1>s2
 */
_int32 sd_stricmp(const char *s1, const char *s2);

/* @Simple Function@
 * only compares the first n characters of s1
 * Return :   -1: s1<s2  0: s1==s2     1: s1>s2
 */
_int32 sd_strnicmp(const char *s1, const char *s2, _int32 n);

/* @Simple Function@
 * Return : a pointer to the first occurrence of the character ch in the string dest
 *          NULL when not found
 */
char* sd_strchr(char *dest, char ch, _int32 from);

/* @Simple Function@
 * 忽略大小写的查找。
 * Return : a pointer to the first occurrence of the character ch in the string dest
 *          NULL when not found
 */
char* sd_strichr(char *dest, char ch, _int32 from);

/* @Simple Function@
 * Return : a pointer to the beginning of the sub-string
 *          NULL when not found
 */
char* sd_strstr(const char *dest, const char *search_str, _int32 from);

/* @Simple Function@
 * 忽略大小写的查找。
 * Return : a pointer to the beginning of the sub-string
 *          NULL when not found
 */
char* sd_stristr(const char *dest, const char *search_str, _int32 from);

/* @Simple Function@
 * Return : a pointer to the last occurrence of the character ch in the string dest
 *          NULL when not found
 */
char* sd_strrchr(const char *dest, char ch);

/* @Simple Function@
 * 忽略大小写的查找。
 * Return : a pointer to the last occurrence of the character ch in the string dest
 *          NULL when not found
 */
char* sd_strirchr(char *dest, char ch);

_int32 sd_memset(void *dest, _int32 c, _int32 count);

/*  The memory areas should not overlap*/
_int32 sd_memcpy(void *dest, const void *src, _int32 n);

/*  The memory areas may overlap*/
_int32 sd_memmove(void *dest, const void *src, _int32 n);
_int32 sd_memcmp(const void* src, const void* dest, _u32 len);

/* @Simple Function@
 * Description: remove all the ' ','\t','\r' and '\n' on the head of the string.
 * Return :     0: SUCCESS     -1: ERROR
 */
_int32 sd_trim_prefix_lws( char * str );

/* @Simple Function@
 * Description: remove all the ' ','\t','\r' and '\n' on the tail of the string.
 * Return :     0: SUCCESS     -1: ERROR
 */
_int32 sd_trim_postfix_lws( char * str );

/*切割字符串，把结果保存在result列表中，注意LIST中的字段类型是char*,内容是malloc出来的，记得回收内存*/
_int32 sd_divide_str(const char* str, char ch, struct tagLIST* result);

/*替换字符串，把str中出现的source字符串替换为target字符串，要求target字符串的长度小于等于source字符串长度*/
_int32 sd_replace_str(char* str, const char* source, const char* target);

char sd_toupper(char ch);
char sd_tolower(char ch);
void sd_strtolower(char * str);

/*  指定最大字符的长度，返回其截断的子串的长度，utf-8编码 */
_u32 sd_get_sub_utf8_str_len(char *utf8_str, _u32 max_len);

_int32 sd_replace_7c(char * str);

#ifdef __cplusplus
}
#endif

#endif
