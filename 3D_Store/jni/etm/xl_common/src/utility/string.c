#include "utility/errcode.h"
#include "utility/string.h"
#include "utility/utility.h"
#include "utility/list.h"
#include "utility/arg.h"
#include "utility/mempool.h"
#include "platform/sd_fs.h"


#if defined(LINUX)
#include <memory.h>
 #include<ctype.h>
#endif
#include <string.h>
#include <stdio.h>


#define PRN_LBUF_SIZE  (512)

static const unsigned char g_char_map[] = {
	(unsigned char)'\000',(unsigned char)'\001',(unsigned char)'\002',(unsigned char)'\003',(unsigned char)'\004',(unsigned char)'\005',(unsigned char)'\006',(unsigned char)'\007',
	(unsigned char)'\010',(unsigned char)'\011',(unsigned char)'\012',(unsigned char)'\013',(unsigned char)'\014',(unsigned char)'\015',(unsigned char)'\016',(unsigned char)'\017',
	(unsigned char)'\020',(unsigned char)'\021',(unsigned char)'\022',(unsigned char)'\023',(unsigned char)'\024',(unsigned char)'\025',(unsigned char)'\026',(unsigned char)'\027',
	(unsigned char)'\030',(unsigned char)'\031',(unsigned char)'\032',(unsigned char)'\033',(unsigned char)'\034',(unsigned char)'\035',(unsigned char)'\036',(unsigned char)'\037',
	(unsigned char)'\040',(unsigned char)'\041',(unsigned char)'\042',(unsigned char)'\043',(unsigned char)'\044',(unsigned char)'\045',(unsigned char)'\046',(unsigned char)'\047',
	(unsigned char)'\050',(unsigned char)'\051',(unsigned char)'\052',(unsigned char)'\053',(unsigned char)'\054',(unsigned char)'\055',(unsigned char)'\056',(unsigned char)'\057',
	(unsigned char)'\060',(unsigned char)'\061',(unsigned char)'\062',(unsigned char)'\063',(unsigned char)'\064',(unsigned char)'\065',(unsigned char)'\066',(unsigned char)'\067',
	(unsigned char)'\070',(unsigned char)'\071',(unsigned char)'\072',(unsigned char)'\073',(unsigned char)'\074',(unsigned char)'\075',(unsigned char)'\076',(unsigned char)'\077',
	(unsigned char)'\100',(unsigned char)'\141',(unsigned char)'\142',(unsigned char)'\143',(unsigned char)'\144',(unsigned char)'\145',(unsigned char)'\146',(unsigned char)'\147',
	(unsigned char)'\150',(unsigned char)'\151',(unsigned char)'\152',(unsigned char)'\153',(unsigned char)'\154',(unsigned char)'\155',(unsigned char)'\156',(unsigned char)'\157',
	(unsigned char)'\160',(unsigned char)'\161',(unsigned char)'\162',(unsigned char)'\163',(unsigned char)'\164',(unsigned char)'\165',(unsigned char)'\166',(unsigned char)'\167',
	(unsigned char)'\170',(unsigned char)'\171',(unsigned char)'\172',(unsigned char)'\133',(unsigned char)'\134',(unsigned char)'\135',(unsigned char)'\136',(unsigned char)'\137',
	(unsigned char)'\140',(unsigned char)'\141',(unsigned char)'\142',(unsigned char)'\143',(unsigned char)'\144',(unsigned char)'\145',(unsigned char)'\146',(unsigned char)'\147',
	(unsigned char)'\150',(unsigned char)'\151',(unsigned char)'\152',(unsigned char)'\153',(unsigned char)'\154',(unsigned char)'\155',(unsigned char)'\156',(unsigned char)'\157',
	(unsigned char)'\160',(unsigned char)'\161',(unsigned char)'\162',(unsigned char)'\163',(unsigned char)'\164',(unsigned char)'\165',(unsigned char)'\166',(unsigned char)'\167',
	(unsigned char)'\170',(unsigned char)'\171',(unsigned char)'\172',(unsigned char)'\173',(unsigned char)'\174',(unsigned char)'\175',(unsigned char)'\176',(unsigned char)'\177',
	(unsigned char)'\200',(unsigned char)'\201',(unsigned char)'\202',(unsigned char)'\203',(unsigned char)'\204',(unsigned char)'\205',(unsigned char)'\206',(unsigned char)'\207',
	(unsigned char)'\210',(unsigned char)'\211',(unsigned char)'\212',(unsigned char)'\213',(unsigned char)'\214',(unsigned char)'\215',(unsigned char)'\216',(unsigned char)'\217',
	(unsigned char)'\220',(unsigned char)'\221',(unsigned char)'\222',(unsigned char)'\223',(unsigned char)'\224',(unsigned char)'\225',(unsigned char)'\226',(unsigned char)'\227',
	(unsigned char)'\230',(unsigned char)'\231',(unsigned char)'\232',(unsigned char)'\233',(unsigned char)'\234',(unsigned char)'\235',(unsigned char)'\236',(unsigned char)'\237',
	(unsigned char)'\240',(unsigned char)'\241',(unsigned char)'\242',(unsigned char)'\243',(unsigned char)'\244',(unsigned char)'\245',(unsigned char)'\246',(unsigned char)'\247',
	(unsigned char)'\250',(unsigned char)'\251',(unsigned char)'\252',(unsigned char)'\253',(unsigned char)'\254',(unsigned char)'\255',(unsigned char)'\256',(unsigned char)'\257',
	(unsigned char)'\260',(unsigned char)'\261',(unsigned char)'\262',(unsigned char)'\263',(unsigned char)'\264',(unsigned char)'\265',(unsigned char)'\266',(unsigned char)'\267',
	(unsigned char)'\270',(unsigned char)'\271',(unsigned char)'\272',(unsigned char)'\273',(unsigned char)'\274',(unsigned char)'\275',(unsigned char)'\276',(unsigned char)'\277',
	(unsigned char)'\300',(unsigned char)'\341',(unsigned char)'\342',(unsigned char)'\343',(unsigned char)'\344',(unsigned char)'\345',(unsigned char)'\346',(unsigned char)'\347',
	(unsigned char)'\350',(unsigned char)'\351',(unsigned char)'\352',(unsigned char)'\353',(unsigned char)'\354',(unsigned char)'\355',(unsigned char)'\356',(unsigned char)'\357',
	(unsigned char)'\360',(unsigned char)'\361',(unsigned char)'\362',(unsigned char)'\363',(unsigned char)'\364',(unsigned char)'\365',(unsigned char)'\366',(unsigned char)'\367',
	(unsigned char)'\370',(unsigned char)'\371',(unsigned char)'\372',(unsigned char)'\333',(unsigned char)'\334',(unsigned char)'\335',(unsigned char)'\336',(unsigned char)'\337',
	(unsigned char)'\340',(unsigned char)'\341',(unsigned char)'\342',(unsigned char)'\343',(unsigned char)'\344',(unsigned char)'\345',(unsigned char)'\346',(unsigned char)'\347',
	(unsigned char)'\350',(unsigned char)'\351',(unsigned char)'\352',(unsigned char)'\353',(unsigned char)'\354',(unsigned char)'\355',(unsigned char)'\356',(unsigned char)'\357',
	(unsigned char)'\360',(unsigned char)'\361',(unsigned char)'\362',(unsigned char)'\363',(unsigned char)'\364',(unsigned char)'\365',(unsigned char)'\366',(unsigned char)'\367',
	(unsigned char)'\370',(unsigned char)'\371',(unsigned char)'\372',(unsigned char)'\373',(unsigned char)'\374',(unsigned char)'\375',(unsigned char)'\376',(unsigned char)'\377',
};

_int32 sd_strncpy(char *dest, const char *src, _int32 size)
{
	_int32 i = 0;

	for(i = 0; i < size; i++)
	{
		dest[i] = src[i];
		if(!src[i])
			break;
	}

	return SUCCESS;
}

_int32 sd_vsnprintf(char *buffer, _int32 bufsize, const char *fmt, sd_va_list ap)
{
    _int32 src_idx = 0, obj_idx = 0;
    _int32 ret_val = SUCCESS;
    char *string_param = 0;

    buffer[bufsize - 1] = 0;
    for(obj_idx = 0, src_idx = 0; obj_idx < bufsize - 1 && fmt[src_idx];)
    {
        if(fmt[src_idx] == '%')
        {
            switch(fmt[src_idx + 1])
            {
            case 'd':
                ret_val = sd_i32toa(sd_va_arg(ap, _int32), buffer + obj_idx, bufsize - obj_idx, 10);
                CHECK_VALUE(ret_val);

                src_idx += 2;
                break;
            case 'u':
                ret_val = sd_u32toa(sd_va_arg(ap, _u32), buffer + obj_idx, bufsize - obj_idx, 10);
                CHECK_VALUE(ret_val);

                src_idx += 2;
                break;
            case 'x':
            case 'X':
                ret_val = sd_u32toa(sd_va_arg(ap, _u32), buffer + obj_idx, bufsize - obj_idx, 16);
                CHECK_VALUE(ret_val);

                src_idx += 2;
                break;
            case 's':
                string_param = sd_va_arg(ap, char*);
                if(string_param)
                    ret_val = sd_strncpy(buffer + obj_idx, string_param, bufsize - obj_idx - 1);
                else
                    buffer[obj_idx] = 0;

                CHECK_VALUE(ret_val);

                src_idx += 2;
                break;

            case 'l':
                if(fmt[src_idx + 2] == 'd')
                {
                    ret_val = sd_i32toa(sd_va_arg(ap, _int32), buffer + obj_idx, bufsize - obj_idx, 10);
                    CHECK_VALUE(ret_val);

                    src_idx += 3;
                }
                else if(fmt[src_idx + 2] == 'u')
                {
                    ret_val = sd_u32toa(sd_va_arg(ap, _u32), buffer + obj_idx, bufsize - obj_idx, 10);
                    CHECK_VALUE(ret_val);

                    src_idx += 3;
                }
                else if(sd_strncmp(fmt + src_idx + 2, "ld", 2) == 0)
                {
                    ret_val = sd_i64toa(sd_va_arg(ap, _int64), buffer + obj_idx, bufsize - obj_idx, 10);
                    CHECK_VALUE(ret_val);

                    src_idx += 4;
                }
                else if(sd_strncmp(fmt + src_idx + 2, "lu", 2) == 0)
                {
                    ret_val = sd_u64toa(sd_va_arg(ap, _u64), buffer + obj_idx, bufsize - obj_idx, 10);
                    CHECK_VALUE(ret_val);

                    src_idx += 4;
                }
                else
                {
                    buffer[obj_idx] = fmt[src_idx++];
                    buffer[obj_idx + 1] = 0; /* obj_idx < bufsize - 1 */
                }

                break;
            default:
                buffer[obj_idx] = fmt[src_idx++];
                buffer[obj_idx + 1] = 0; /* obj_idx < bufsize - 1 */
                break;;
            }
            obj_idx += sd_strlen(buffer + obj_idx);
        }
        else
            buffer[obj_idx++] = fmt[src_idx++];
    }

    buffer[obj_idx] = 0;

    return obj_idx;
}

_int32 sd_snprintf(char *buffer, _int32 bufsize, const char *fmt, ...)
{
	_int32 ret_val = SUCCESS;

	sd_va_list ap;
	sd_va_start(ap, fmt);

#if 0
#ifdef WINCE
	ret_val = _vsnprintf(buffer, bufsize, fmt, ap);
	buffer[bufsize - 1] = 0;
#endif
#ifdef LINUX
	ret_val = vsnprintf(buffer, bufsize, fmt, ap);
	buffer[bufsize - 1] = 0;
#endif

#else
    ret_val = sd_vsnprintf(buffer, bufsize, fmt, ap);
#endif

	sd_va_end(ap);

	return ret_val;
}

_int32 sd_vfprintf(_int32 fd, const char *fmt, sd_va_list ap)
{
    _int32 ret_val = 0;
    _u32 write_size = 0;
    char buffer[PRN_LBUF_SIZE];

    ret_val = sd_vsnprintf(buffer, sizeof(buffer), fmt, ap);
    ret_val = sd_write(fd, buffer, ret_val, &write_size);

    return ret_val;
}

_int32 sd_fprintf(_int32 fd, const char *fmt, ...)
{
    _int32 ret_val = SUCCESS;

    sd_va_list ap;
    sd_va_start(ap, fmt);

    ret_val = sd_vfprintf(fd, fmt, ap);

    sd_va_end(ap);

    return ret_val;
}

_int32 sd_printf(const char *fmt, ...)
{
    _int32 ret_val = SUCCESS;

    sd_va_list ap;
    sd_va_start(ap, fmt);

    ret_val = sd_vfprintf(1, fmt, ap);

    sd_va_end(ap);

    return ret_val;
}

_int32 sd_strlen(const char *str)
{
	if(str)
		return strlen(str);
	else
		return 0;
}

_int32 sd_strcat(char *dest, const char *src, _int32 n)
{
	strncat(dest, src, n);
	return SUCCESS;
}

_int32 sd_strcmp(const char *s1, const char *s2)
{
	return strcmp(s1, s2);
}

_int32 sd_strncmp(const char *s1, const char *s2, _int32 n)
{
	return strncmp(s1, s2, n);
}

_int32 sd_stricmp(const char *s1, const char *s2)
{
#if defined(LINUX)
	return strcasecmp(s1, s2);
#elif defined(WINCE)
	return _stricmp(s1, s2);
#endif
}

_int32 sd_strnicmp(const char *s1, const char *s2, _int32 n)
{
#if defined(LINUX)
	return strncasecmp(s1, s2, n);
#elif defined(WINCE)
	return _strnicmp(s1, s2, n);
#else
	return NOT_IMPLEMENT;
#endif
}

char* sd_strchr(char *dest, char ch, _int32 from)
{
	return strchr(dest + from, (_int32)ch);
}

char* sd_strichr(char *dest, char ch, _int32 from)
{
	char *s = (char *)(dest + from);
	for (;;)
	{
		if (g_char_map[(_u8)*s] == g_char_map[(_u8)ch])
			return (char *) s;
		if (*s == 0)
			return 0;
		s++;
	}
}

char* sd_strstr(const char *dest, const char *search_str, _int32 from)
{
	return strstr(dest + from, search_str);
}

char* sd_stristr(const char *dest, const char *search_str, _int32 from)
{
	const char *p, *q;
	char *s1 = (char *)(dest + from);
	const char *s2 = search_str;
	for (; *s1; s1++)
	{
		p = s1, q = s2;
		while (*q && *p)
		{
			if (g_char_map[(_u8)*q] != g_char_map[(_u8)*p])
				break;
			p++, q++;
		}
		if (*q == 0)
			return (char *)s1;
	}
	return NULL;
}

char* sd_strrchr(const char *dest, char ch)
{
	return strrchr(dest, (_int32)ch);
}

char* sd_strirchr(char *dest, char ch)
{
	_u32 i = 0;
	while (dest[i] != 0)
		i++;
	do
		if (g_char_map[(_u8)dest[i]] == g_char_map[(_u8)ch])
			return (char *) dest + i;
	while (i-- != 0);
	return NULL;
}

/* Nonzero if X is not aligned on a "long" boundary.  */
#define UNALIGNED_X(X) \
  ((_int32)X & (sizeof (_int32) - 1))

/* Nonzero if either X or Y is not aligned on a "long" boundary.  */
#define UNALIGNED(X, Y) \
  (((_int32)X & (sizeof (_int32) - 1)) | ((_int32)Y & (sizeof (_int32) - 1)))

/* How many bytes are copied each iteration of the 4X unrolled loop.  */
#define BIGBLOCKSIZE    (sizeof (_int32) << 2)

/* How many bytes are copied each iteration of the word copy loop.  */
#define LITTLEBLOCKSIZE (sizeof (_int32))

/* Threshhold for punting to the byte copier.  */
#define TOO_SMALL(LEN)  ((LEN) < BIGBLOCKSIZE)

_int32 sd_memset(void *dest, _int32 c, _int32 count)
{
	_int32 *aligned_dest = NULL;
	char *dstp = (char *)dest;
	_int32 aligned_c = 0;
	if (!TOO_SMALL(count) && !UNALIGNED_X(dstp))
	{
		aligned_dest = (_int32 *)dstp;
		if (c == 0)
		{
			while (count >= LITTLEBLOCKSIZE)
			{
				*aligned_dest++ = 0;
				count -= LITTLEBLOCKSIZE;
			}
		}
		else
		{
			aligned_c = ((char)c) | (((char)c)<< 8) | (((char)c)<< 16) | (((char)c)<< 24);
			while (count >= LITTLEBLOCKSIZE)
			{
				*aligned_dest++ = aligned_c;
				count -= LITTLEBLOCKSIZE;
			}
		}
		dstp = (char *)aligned_dest;
	}
	while (count-- != 0)
		*dstp++ = (char)c;
	return SUCCESS;
//#endif
}

_int32 sd_memcpy(void *dest, const void *src, _int32 n)
{
	char *d = dest;
	const char *s = src;
	_int32 *aligned_dest = NULL;
	const _int32 *aligned_src = NULL;

	if(n==0) return SUCCESS;

	if(!dest || !src )
	{
		CHECK_VALUE(INVALID_ARGUMENT);
	}


	/* If the size is small, or either SRC or DST is unaligned,
	 then punt into the byte copy loop.  This should be rare.  */
	if (!TOO_SMALL(n) && !UNALIGNED(s, d))
	{
		aligned_dest = (_int32*) d;
		aligned_src = (_int32*) s;

		/* Copy 4X long words at a time if possible.  */
		while (n >= BIGBLOCKSIZE)
		{
			*aligned_dest++ = *aligned_src++;
			*aligned_dest++ = *aligned_src++;
			*aligned_dest++ = *aligned_src++;
			*aligned_dest++ = *aligned_src++;
			n -= BIGBLOCKSIZE;
		}

		/* Copy one long word at a time if possible.  */
		while (n >= LITTLEBLOCKSIZE)
		{
			*aligned_dest++ = *aligned_src++;
			n -= LITTLEBLOCKSIZE;
		}

		/* Pick up any residual with a byte copier.  */
		d = (char*) aligned_dest;
		s = (char*) aligned_src;
	}

	while (n--)
		*d++ = *s++;

	return SUCCESS;
//#endif
}

_int32 sd_memmove(void *dest, const void *src, _int32 n)
{
	char *dstp;
	const char *srcp;

	if (dest == src || n == 0) return SUCCESS;
	
	srcp = src;
	dstp = dest;
	if (srcp < dstp)
		while (n-- != 0)
			dstp[n] = srcp[n];
	else
		while (n-- != 0)
			*dstp++ = *srcp++;
	return SUCCESS;
}

_int32 sd_memcmp(const void* src, const void* dest, _u32 len)
{
	_int32 i;
	const _int8 *src_m = src;
	const _int8 *dest_m = dest;
	
	for(i=0;i<len;i++)
		{
			if(src_m[i]!=dest_m[i])
				{
					return src_m[i]-dest_m[i];
}
		}
	return 0;
}

_int32 sd_trim_prefix_lws( char * str )
{
	char * temp = str,* temp2 = str;
	
	if(temp[0]=='\0')
		return SUCCESS;
	
	while((temp[0]!='\0')&&((temp[0]==' ')||(temp[0]=='\t')||(temp[0]=='\r')||(temp[0]=='\n')))
		temp++;

		if(temp!=str)
		{
			while(temp[0]!='\0')
			{
				temp2[0]=temp[0];
				temp++;
				temp2++;
			}
			temp2[0]='\0';
			//sd_memcpy(str, temp, strlen(temp)+1);
		}

	return SUCCESS;
}

_int32 sd_trim_postfix_lws( char * str )
{
	char * temp = NULL;

	if(str[0] == '\0')
		return SUCCESS;

	temp = str+sd_strlen(str)-1;

	while((temp[0]==' ')||(temp[0]=='\t')||(temp[0]=='\r')||(temp[0]=='\n'))
	{
		if(temp== str)
		{
			str[0] = '\0';
			return SUCCESS;
		}
		temp--;
	}

	str[temp - str+1] = '\0';
	return SUCCESS;
}
_int32 sd_divide_str(const char* str, char ch, LIST* result)
{
	_int32 ret = SUCCESS;
	char* pos_begin = (char*)str, *pos_end = NULL, *value = NULL;
	pos_end = sd_strchr(pos_begin, ch, 0);
	do
	{
		if(pos_end == NULL ) //没有分隔符，认为只有一段
		{
			if ( sd_strlen(pos_begin) <= 0) break;
			ret = sd_malloc(sd_strlen(pos_begin) + 1, (void**)&value);
			if(ret != SUCCESS)
				break;
			sd_strncpy(value, pos_begin, sd_strlen(pos_begin) + 1);
			list_push(result, value);
			break;
		}
		
		ret = sd_malloc(pos_end - pos_begin + 1, (void**)&value);
		if(ret != SUCCESS)
			break;
		sd_strncpy(value, pos_begin, pos_end - pos_begin);
		value[pos_end - pos_begin] = '\0';
		list_push(result, value);
		pos_begin = pos_end + 1;
		pos_end = sd_strchr(pos_begin, ch, 0);
		
	}while(TRUE);
	
	if(ret != SUCCESS)
	{
		while(list_size(result) > 0)
		{
			list_pop(result, (void**)&value);
			sd_free(value);
		}
	}
	return ret;
}

/* 将字符串中的source替换成target,一定要注意 target比 source长时内存是否会越界*/
_int32 sd_replace_str(char* str, const char* source, const char* target)
{
	char* head = NULL, *tail = NULL,*tmp_tail = NULL;
	_int32 pos = 0,count = 0,src_len = sd_strlen(source),tag_len = sd_strlen(target);

	head = sd_strstr(str, source, pos);
	if(head==NULL) return SUCCESS;
	
	if(tag_len <= src_len)
	{
		while(head != NULL)
		{
			tail = head + src_len;
			sd_strncpy(head, target, tag_len);
			head += tag_len;
			if(head != tail)
				sd_strncpy(head, tail, sd_strlen(tail) + 1);		//记得拷贝最后一个结束符标志\0
			pos = head - str;
			head = sd_strstr(str, source, pos);
		}
	}
	else
	{
		char tmp_buffer[1024];
		
		if(sd_strlen(str)>1023) return -1;
		
		sd_memset(tmp_buffer,0,1024);
		sd_strncpy(tmp_buffer,str, sd_strlen(str));
		
		while(head != NULL)
		{
			tail = head + src_len;
			sd_strncpy(head, target, tag_len);
			head += tag_len;
			tmp_tail = tmp_buffer+(tail-str-(tag_len-src_len)*count);
			sd_strncpy(head, tmp_tail, sd_strlen(tmp_tail) + 1);		//记得拷贝最后一个结束符标志\0
			pos = head - str;
			head = sd_strstr(str, source, pos);
			count++;
		}
	}
	return SUCCESS;
}

char sd_toupper(char ch)
{
#ifndef LINUX
      if( ch >= 'a' && ch <= 'z')
	  	return (ch + 'A' - 'a');
	  else
	return ch;
#else
	return toupper(ch);
#endif
}

char sd_tolower(char ch)
{
#ifndef LINUX
    if( ch >= 'A' && ch <= 'Z')
	  	return (ch - 'A' + 'a');
    else
        return ch;
#else
	return tolower(ch);
#endif
}

void sd_strtolower(char * str)
{
	while(str&&*str!='\0')
	{
		*str = sd_tolower(*str);
		str++;
	}
	return;
}

/*  指定最大字符的长度，返回其截断的子串的长度，utf-8编码 */
_u32 sd_get_sub_utf8_str_len(char *utf8_str, _u32 max_len)
{
	_u32 count = 0;	// 字的个数，包含英文或中文等
	_u32 index = 0;
	_u32 len = 0;
	//_u32 ret_index = 0;	// 返回的长度，在index的增长过程中可能会超过max_len，所以这个存的是每次增长前的长度
	char c = 0;

	if(utf8_str == NULL) return 0;
	
	len = strlen(utf8_str);
	
	while (index < len && index < max_len)
	{
//		ret_index = index;
		c = utf8_str[index];
		
//		if (index >= len)
//		{
//			if (index <= max_len)
//				ret_index = index;
//			
//			break;
//		}
		
		if (c & 0x80)
		{
			c <<= 1;
			count++;
			
			while (c & 0x80)
			{
				count++;
				c <<= 1;
			}
			if (count <= len && count <= max_len)
			{
				index = count;
			}
			else
			{
				break;
			}
		}
		else
		{
			count++;
			index++;
		}
	}
	
	return index;
}

_int32 sd_replace_7c(char * str)
{
	sd_replace_str(str,"%7C", "|");
	sd_replace_str(str,"%7c", "|");
	return SUCCESS;
}