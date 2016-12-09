#include "et_utility.h"
#include "utility/base64.h"
#include "utility/define_const_num.h"
#include "utility/url.h"
#include "utility/sd_iconv.h"
#include "platform/sd_fs.h"
#include <stdlib.h>

#ifdef LINUX
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#elif defined(WINCE)
#include <winsock2.h>
#endif

/* Convert BASE64  to char string
 * Make sure the length of bufer:d larger than 3/4 length of source string:s
 * Returns: 0:success, -1:failed
 */
_int32 et_base64_decode(const char *s, _u32 str_len, char *d)
{
	char base64_table[255], s_buffer[MAX_URL_LEN * 2 + 1], *p_s = s_buffer; //因为一些磁力链的url长度会达到2048字节，所以调一下处理缓冲区长度
	_int32 clen, len, i, j;

	sd_memset(base64_table, 0, 255);
	sd_memset(p_s, 0, MAX_URL_LEN * 2 + 1);

	for (j = 0, i = 'A'; i <= 'Z'; i++)
		base64_table[i] = j++;
	for (i = 'a'; i <= 'z'; i++)
		base64_table[i] = j++;
	for (i = '0'; i <= '9'; i++)
		base64_table[i] = j++;
	base64_table['+'] = j++;
	base64_table['/'] = j++;
	base64_table['='] = j;

	sd_strncpy(p_s, s, str_len);
	len = sd_strlen(p_s);
	len = len % 4;
	if (len != 0)
	{
		len = 4 - (len % 4);
		while (len--)
			sd_strcat(p_s, "=", 2);
	}
	len = sd_strlen(p_s);
	clen = len / 4;

	for (; clen--;)
	{
		*d = base64_table[(int)(*p_s++)] << 2 & 0xfc;
		*d++ |= base64_table[(int)(*p_s)] >> 4;
		*d = base64_table[(int)(*p_s++)] << 4 & 0xf0;
		*d++ |= base64_table[(int)(*p_s)] >> 2 & 0x0f;
		*d = base64_table[(int)(*p_s++)] << 6;
		if (*p_s != '=')
			*d++ |= base64_table[(int)(*p_s++)];
	}

	return 0;
}

_int32 et_parse_magnet_url(const char * url, _u8 info_hash[INFO_HASH_LEN], char * name_buffer, _u64 * file_size)
{
	_int32 ret_val = SUCCESS;
	char * point_pos = NULL;
	char tmp_buffer[MAX_FILE_NAME_BUFFER_LEN] = { 0 };
	_u32 buffer_len = MAX_FILE_NAME_BUFFER_LEN, xt_len = 40;;
	BOOL is_base32 = FALSE;

	/* 只嗅探用BT infohash 做urn 的磁力链 */
	point_pos = sd_strstr(url, "btih:", 0);
	if ((point_pos != NULL) && (sd_strlen(point_pos + 5) >= 32))
	{
		char *p_xl = NULL, *p_dn = NULL, *p_end = NULL;

		if ((sd_strlen(point_pos + 5) < 40) || sd_string_to_cid(point_pos + 5, info_hash) != SUCCESS)
		{
			ret_val = sd_decode_base32(point_pos + 5, 32, (char*)info_hash, INFO_HASH_LEN);
			CHECK_VALUE(ret_val);

			is_base32 = TRUE;
			xt_len = 32;
		}

		/* 文件大小 */
		p_xl = sd_strstr(url, "xl=", 0);
		if (p_xl != NULL&&file_size != NULL)
		{
			p_xl += 3;
			p_end = sd_strstr(p_xl, "&", 1);
			if (p_end != NULL)
			{
				sd_strncpy(tmp_buffer, p_xl, p_end - p_xl);
				p_xl = tmp_buffer;
			}
			ret_val = sd_str_to_u64(p_xl, sd_strlen(p_xl), file_size);
			sd_assert(ret_val == SUCCESS);

		}

		/* 文件名 */
		p_dn = sd_strstr(url, "dn=", 0);
		if (p_dn != NULL)
		{
			p_dn += 3;
			p_end = sd_strstr(p_dn, "&", 1);
			if (p_end != NULL)
			{
				sd_strncpy(name_buffer, p_dn, p_end - p_dn);
			}
			else
			{
				sd_strncpy(name_buffer, p_dn, MAX_FILE_NAME_BUFFER_LEN - 1);
			}

			/* remove all the '%' */
			sd_decode_file_name(name_buffer, NULL, MAX_FILE_NAME_BUFFER_LEN);

			/* Check if Bad file name ! */
			sd_get_valid_name(name_buffer, NULL);

			ret_val = sd_any_format_to_utf8(name_buffer, sd_strlen(name_buffer), tmp_buffer, &buffer_len);
			sd_assert(ret_val != -1);
			if (ret_val > 0)
			{
				sd_strncpy(name_buffer, tmp_buffer, MAX_FILE_NAME_BUFFER_LEN - 1);
			}
		}
		else
		{
			/* 用btih 字段做文件名*/
			sd_strncpy(name_buffer, point_pos + 5, xt_len);
		}
	}
	else
	{
		return -80;
	}
	return ret_val;
}

_int32 et_replace_7c(char * str)
{
	sd_replace_str(str, "%7C", "|");
	sd_replace_str(str, "%7c", "|");
	return SUCCESS;
}

char * et_get_file_name_from_url(char * url, _u32 url_length, char *out_name, _u32 out_len)
{
	if (!out_name || out_len < MAX_FILE_NAME_BUFFER_LEN) return NULL;
	char name[MAX_FILE_NAME_BUFFER_LEN * 2] = { 0 };
	_int32 ret_val = SUCCESS;
	char url_tmp[MAX_URL_LEN * 2] = { 0 };
	char *pos1 = NULL, *pos2 = NULL;
	_u32 name_len = MAX_FILE_NAME_BUFFER_LEN * 2;

	if ((url == NULL) || (url_length < 9))
	{
		return NULL;
	}

	sd_memset(name, 0, name_len);
	sd_memset(url_tmp, 0, MAX_URL_LEN);

	/* Check if the URL is thunder special URL */
	if (sd_strncmp((char*)url, "magnet:?", sd_strlen("magnet:?")) == 0)
	{
		/* Decode to normal URL */
		_u8 info_hash[INFO_HASH_LEN] = { 0 };
		if (url_length >= MAX_URL_LEN)
		{
			/* 超长磁力链截断处理 */
			sd_strncpy(url_tmp, url, MAX_URL_LEN - 1);
			url = url_tmp;
			pos1 = sd_strrchr(url, '&');
			if (pos1 != NULL)
			{
				*pos1 = '\0';
			}
		}
		if (et_parse_magnet_url(url, info_hash, name, NULL) != SUCCESS)
		{
			return  NULL;
		}

		if (sd_strlen(name) == 0)
			return NULL;

	}
	else if (sd_strncmp((char*)url, "ed2k://", sd_strlen("ed2k://")) == 0)
	{
		if (sd_strncmp(url, "ed2k://%7", sd_strlen("ed2k://%7")) == 0)
		{
			sd_strncpy(url_tmp, url, MAX_URL_LEN - 1);
			et_replace_7c(url_tmp);
			url = url_tmp;
		}
		/* E-Mule URL */
		pos2 = sd_strstr(url, "|file|", 0);
		if (pos2 != NULL)
		{
			pos2 += 6;
		}
		else
		{
			return NULL;
		}

		pos1 = sd_strchr(pos2, '|', 0);
		if (pos1 != NULL&&pos1 - pos2 > 0)
		{
			sd_strncpy(name, pos2, pos1 - pos2);
			/* remove all the '%' */
			sd_decode_file_name(name, NULL, sizeof(name));

			/* check is Bad file name ! */
			sd_get_valid_name(name, NULL);

			ret_val = sd_any_format_to_utf8(name, sd_strlen(name), url_tmp, &name_len);
			sd_assert(ret_val != -1);
			if (ret_val > 0)
			{
				sd_strncpy(name, url_tmp, MAX_FILE_NAME_BUFFER_LEN * 2 - 1);
			}

			if (sd_strlen(name) == 0)
				return NULL;

		}
		else
		{
			return NULL;
		}
	}
	else
	{
		if (sd_strstr((char*)url, "thunder://", 0) == url)
		{
			/* Decode to normal URL */
			if (sd_base64_decode(url + 10, (_u8*)url_tmp, NULL) != 0)
			{
				return  NULL;
			}

			url_tmp[sd_strlen(url_tmp) - 2] = '\0';
			sd_strncpy(url_tmp, url_tmp + 2, MAX_URL_LEN - 1);
		}
		else
		{
			sd_strncpy(url_tmp, url, MAX_URL_LEN - 1);
		}

		ret_val = sd_get_file_name_from_url(url_tmp, sd_strlen(url_tmp), name, sizeof(name));
		sd_assert(ret_val == SUCCESS);
		if (ret_val == SUCCESS)
		{
			ret_val = sd_any_format_to_utf8(name, sd_strlen(name), url_tmp, &name_len);
			sd_assert(ret_val != -1);
			if (ret_val > 0)
			{
				sd_strncpy(name, url_tmp, MAX_FILE_NAME_BUFFER_LEN * 2 - 1);
			}
			if (sd_strlen(name) == 0)
				return NULL;
		}
		else
		{
			return NULL;
		}
	}
	name_len = sd_strlen(name);
	if (name_len > 0)
	{
		//当文件名的长度大于输出缓冲区的长度时，按字符截断
		if (name_len > out_len - 1)
		{
			_u32 maxLen = out_len;
			_u32 _unicode_name_len = sizeof(url_tmp) / 2;
			ret_val = sd_any_format_to_unicode(name, sd_strlen(name), (_u16 *)url_tmp, &_unicode_name_len);
			sd_assert(SUCCESS == ret_val);
			if (SUCCESS == ret_val)
			{
				_int32 remain_len = _unicode_name_len;
				char *name_data = name + sizeof(name)-1;
				char *name_end = name_data + 1;
				*name_data = '\0';

				_u16 *convert_begin = (_u16 *)url_tmp;
				convert_begin += _unicode_name_len;
				_int32 convert_len = maxLen / 3;

				while (name_end - name_data < maxLen)
				{
					if (remain_len - convert_len > 0)
					{
						convert_begin -= convert_len;
						remain_len -= convert_len;
					}
					else
					{
						convert_len = remain_len;
						remain_len = 0;
						convert_begin = (_u16*)url_tmp;
					}
					name_len = sizeof(name)-1 - (name_end - name_data);
					ret_val = sd_unicode_2_utf8(convert_begin, convert_len, (_u8*)name, &name_len);
					if (SUCCESS == ret_val && (name_end - name_data + name_len <= maxLen))
					{
						name_data -= name_len;
						sd_memcpy(name_data, name, name_len);
					}
					else
					{
						break;
					}
					convert_len = 10;
				}
				if (name != name_data && name_end - name_data > 1)
				{
					sd_memmove(name, name_data, name_end - name_data);
				}
			}

		}
		sd_strncpy(out_name, name, out_len - 1);
		out_name[out_len - 1] = 0;
		return out_name;
	}
	return NULL;

}

//获得某个目录下唯一的文件名，如果成功，会返回SUCCESS, 并直接会修改_file_name变量

_int32 get_unique_file_name_in_path(const char *_path, char *_file_name, _u32 _file_name_buf_len)
{
	if (!_path || !_file_name || sd_strlen(_file_name) + 2 >= _file_name_buf_len) return -1;

	char new_full_name[MAX_FILE_NAME_BUFFER_LEN] = { 0 };
	char szNum[8] = { 0 };
	sd_strcat(new_full_name, _path, sd_strlen(_path));
	sd_strcat(new_full_name, _file_name, sd_strlen(_file_name));
	char *file_ext = sd_strrchr(_file_name, '.');
	BOOL bExist = sd_file_exist(new_full_name);
	int i = 0;
	while (bExist && i < 1000)
	{
		sprintf(szNum, "_%u", i);
		sd_strcat(new_full_name, _path, sd_strlen(_path));
		if (file_ext)
		{
			sd_strcat(new_full_name, _file_name, file_ext - _file_name);
			sd_strcat(new_full_name, szNum, sd_strlen(szNum));
			sd_strcat(new_full_name, file_ext, sd_strlen(file_ext));
		}
		else
		{
			sd_strcat(new_full_name, _file_name, sd_strlen(_file_name));
			sd_strcat(new_full_name, szNum, sd_strlen(szNum));
		}
		bExist = sd_file_exist(new_full_name);
		i++;

	}
	if (!bExist)
	{
		char buf[64] = { 0 };
		if (file_ext)
		{
			sd_memcpy(buf, file_ext, sd_strlen(file_ext));
			_file_name[file_ext - _file_name] = '\0';
		}
		if (sd_strlen(_file_name) + sd_strlen(buf) + sd_strlen(szNum) + 1 > _file_name_buf_len)
		{
			return -3;//缓冲区不够
		}
		sd_strcat(_file_name, szNum, sd_strlen(szNum));
		sd_strcat(_file_name, buf, sd_strlen(buf));

		return SUCCESS;
	}
	return -2;
}
