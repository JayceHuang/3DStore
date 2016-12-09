#include "utility/bytebuffer.h"
#include "utility/errcode.h"
#include "utility/string.h"

#ifdef BIG_ENDIAN
#undef BIG_ENDIAN
#endif
#define BIG_ENDIAN		(0)
#ifdef LITTLE_ENDIAN
#undef LITTLE_ENDIAN
#endif
#define LITTLE_ENDIAN	(1)

static _int32 g_test_endian = 1;
static _int32 g_host_endian = LITTLE_ENDIAN;

#define TEST_ENDIAN	{g_host_endian = (*((char*)&(g_test_endian)) == 1);}

void bytebuffer_init(void)
{
	TEST_ENDIAN;
}

_int32 sd_get_int64_from_bg(char **buffer, _int32 *cur_buflen, _int64 *value)
{
	_int32 sizes = sizeof(_int64), index = 0;
	char *pvalue = (char*)value;

       #ifdef  NOT_SUPPORT_LARGE_INT_64
             if(*cur_buflen < sizes + sizeof(_int32))
		    return BUFFER_OVERFLOW;

	        *cur_buflen -= (sizes + sizeof(_int32));
	#else
	if(*cur_buflen < sizes)
		return BUFFER_OVERFLOW;

	*cur_buflen -= sizes;
	#endif
	#ifdef  NOT_SUPPORT_LARGE_INT_64
              *buffer += ( sizeof(_int32));
	#endif

	if(g_host_endian == BIG_ENDIAN)
	{
		sd_memcpy(value, *buffer, sizeof(_int64));
	}
	else
	{
		for(index = sizes; index > 0; index--)
		{
			*(pvalue++) = *(*buffer + index - 1);
		}
	}

	*buffer += sizes;

	return SUCCESS;
}

_int32 sd_get_int32_from_bg(char **buffer, _int32 *cur_buflen, _int32 *value)
{
	_int32 sizes = sizeof(_int32), index = 0;
	char *pvalue = (char*)value;

	if(*cur_buflen < sizes)
		return BUFFER_OVERFLOW;

	*cur_buflen -= sizes;

	if(g_host_endian == BIG_ENDIAN)
	{
		sd_memcpy(value, *buffer, sizeof(_int32));
	}
	else
	{
		for(index = sizes; index > 0; index--)
		{
			*(pvalue++) = *(*buffer + index - 1);
		}
	}

	*buffer += sizes;

	return SUCCESS;
}

_int32 sd_get_int16_from_bg(char **buffer, _int32 *cur_buflen, _int16 *value)
{
	_int32 sizes = sizeof(_int16), index = 0;
	char *pvalue = (char*)value;

	if(*cur_buflen < sizes)
		return BUFFER_OVERFLOW;

	*cur_buflen -= sizes;

	if(g_host_endian == BIG_ENDIAN)
	{
		sd_memcpy(value, *buffer, sizeof(_int16));
	}
	else
	{
		for(index = sizes; index > 0; index--)
		{
			*(pvalue++) = *(*buffer + index - 1);
		}
	}

	*buffer += sizes;

	return SUCCESS;
}

_int32 sd_get_int64_from_lt(char **buffer, _int32 *cur_buflen, _int64 *value)
{
	_int32 sizes = sizeof(_int64), index = 0;
	char *pvalue = (char*)value;

	 #ifdef  NOT_SUPPORT_LARGE_INT_64
             if(*cur_buflen < sizes + sizeof(_int32))
		    return BUFFER_OVERFLOW;

	        *cur_buflen -= (sizes + sizeof(_int32));
	#else
	if(*cur_buflen < sizes)
		return BUFFER_OVERFLOW;

	*cur_buflen -= sizes;
	#endif

	if(g_host_endian == LITTLE_ENDIAN)
	{
		sd_memcpy(value, *buffer, sizeof(_int64));
	}
	else
	{
		for(index = sizes; index > 0; index--)
		{
			*(pvalue++) = *(*buffer + index - 1);
		}
	}

	*buffer += sizes;
	#ifdef  NOT_SUPPORT_LARGE_INT_64
              *buffer += ( sizeof(_int32));
	#endif

	return SUCCESS;
}

_int32 sd_get_int32_from_lt(char **buffer, _int32 *cur_buflen, _int32 *value)
{
	_int32 sizes = sizeof(_int32), index = 0;
	char *pvalue = (char*)value;

	if(*cur_buflen < sizes)
		return BUFFER_OVERFLOW;

	*cur_buflen -= sizes;

	if(g_host_endian == LITTLE_ENDIAN)
	{
		sd_memcpy(value, *buffer, sizeof(_int32));
	}
	else
	{
		for(index = sizes; index > 0; index--)
		{
			*(pvalue++) = *(*buffer + index - 1);
		}
	}

	*buffer += sizes;

	return SUCCESS;
}

_int32 sd_get_int16_from_lt(char **buffer, _int32 *cur_buflen, _int16 *value)
{
	_int32 sizes = sizeof(_int16), index = 0;
	char *pvalue = (char*)value;

	if(*cur_buflen < sizes)
		return BUFFER_OVERFLOW;

	*cur_buflen -= sizes;

	if(g_host_endian == LITTLE_ENDIAN)
	{
		sd_memcpy(value, *buffer, sizeof(_int16));
	}
	else
	{
		for(index = sizes; index > 0; index--)
		{
			*(pvalue++) = *(*buffer + index - 1);
		}
	}

	*buffer += sizes;

	return SUCCESS;
}

_int32 sd_get_int8(char **buffer, _int32 *cur_buflen, _int8 *value)
{
	if(*cur_buflen < 1)
		return BUFFER_OVERFLOW;

	*cur_buflen -= 1;

	*value = *((*buffer)++);

	return SUCCESS;
}

_int32 sd_set_int64_to_bg(char **buffer, _int32 *cur_buflen, _int64 value)
{
	_int32 sizes = sizeof(_int64), index = 0;
	char *pvalue = (char*)&value;

	 #ifdef  NOT_SUPPORT_LARGE_INT_64
             if(*cur_buflen < sizes + sizeof(_int32))
		    return BUFFER_OVERFLOW;

	        *cur_buflen -= (sizes + sizeof(_int32));
	#else
	if(*cur_buflen < sizes)
		return BUFFER_OVERFLOW;

	*cur_buflen -= sizes;
	#endif
	#ifdef  NOT_SUPPORT_LARGE_INT_64
	       sd_memset(*buffer, 0 , sizeof(_int32));
              *buffer += ( sizeof(_int32));
	#endif

	if(g_host_endian == BIG_ENDIAN)
	{
		sd_memcpy(*buffer, &value, sizeof(_int64));
	}
	else
	{
		for(index = sizes; index > 0; index--)
		{
			*(*buffer + index - 1) = *(pvalue++);
		}
	}

	*buffer += sizes;

	return SUCCESS;
}

_int32 sd_set_int32_to_bg(char **buffer, _int32 *cur_buflen, _int32 value)
{
	_int32 sizes = sizeof(_int32), index = 0;
	char *pvalue = (char*)&value;

	if(*cur_buflen < sizes)
		return BUFFER_OVERFLOW;

	*cur_buflen -= sizes;

	if(g_host_endian == BIG_ENDIAN)
	{
		sd_memcpy(*buffer, &value, sizeof(_int32));
	}
	else
	{
		for(index = sizes; index > 0; index--)
		{
			*(*buffer + index - 1) = *(pvalue++);
		}
	}

	*buffer += sizes;

	return SUCCESS;
}

_int32 sd_set_int16_to_bg(char **buffer, _int32 *cur_buflen, _int16 value)
{
	_int32 sizes = sizeof(_int16), index = 0;
	char *pvalue = (char*)&value;

	if(*cur_buflen < sizes)
		return BUFFER_OVERFLOW;

	*cur_buflen -= sizes;

	if(g_host_endian == BIG_ENDIAN)
	{
		sd_memcpy(*buffer, &value, sizeof(_int16));
	}
	else
	{
		for(index = sizes; index > 0; index--)
		{
			*(*buffer + index - 1) = *(pvalue++);
		}
	}

	*buffer += sizes;

	return SUCCESS;
}

_int32 sd_set_int64_to_lt(char **buffer, _int32 *cur_buflen, _int64 value)
{
	_int32 sizes = sizeof(_int64), index = 0;
	char *pvalue = (char*)&value;

	 #ifdef  NOT_SUPPORT_LARGE_INT_64
             if(*cur_buflen < sizes + sizeof(_int32))
		    return BUFFER_OVERFLOW;

	        *cur_buflen -= (sizes + sizeof(_int32));
	#else
	if(*cur_buflen < sizes)
		return BUFFER_OVERFLOW;

	*cur_buflen -= sizes;
	#endif

	if(g_host_endian == LITTLE_ENDIAN)
	{
		sd_memcpy(*buffer, &value, sizeof(_int64));
	}
	else
	{
		for(index = sizes; index > 0; index--)
		{
			*(*buffer + index - 1) = *(pvalue++);
		}
	}

	*buffer += sizes;
	#ifdef  NOT_SUPPORT_LARGE_INT_64
	       sd_memset(*buffer, 0 , sizeof(_int32));
              *buffer += ( sizeof(_int32));
	#endif

	return SUCCESS;
}

_int32 sd_set_int32_to_lt(char **buffer, _int32 *cur_buflen, _int32 value)
{
	_int32 sizes = sizeof(_int32), index = 0;
	char *pvalue = (char*)&value;

	if(*cur_buflen < sizes)
		return BUFFER_OVERFLOW;

	*cur_buflen -= sizes;

	if(g_host_endian == LITTLE_ENDIAN)
	{
		sd_memcpy(*buffer, &value, sizeof(_int32));
	}
	else
	{
		for(index = sizes; index > 0; index--)
		{
			*(*buffer + index - 1) = *(pvalue++);
		}
	}

	*buffer += sizes;

	return SUCCESS;
}

_int32 sd_set_int16_to_lt(char **buffer, _int32 *cur_buflen, _int16 value)
{
	_int32 sizes = sizeof(_int16), index = 0;
	char *pvalue = (char*)&value;

	if(*cur_buflen < sizes)
		return BUFFER_OVERFLOW;

	*cur_buflen -= sizes;

	if(g_host_endian == LITTLE_ENDIAN)
	{
		sd_memcpy(*buffer, &value, sizeof(_int16));
	}
	else
	{
		for(index = sizes; index > 0; index--)
		{
			*(*buffer + index - 1) = *(pvalue++);
		}
	}

	*buffer += sizes;

	return SUCCESS;
}

_int32 sd_set_int8(char **buffer, _int32 *cur_buflen, _int8 value)
{
	if(*cur_buflen < 1)
		return BUFFER_OVERFLOW;

	*cur_buflen -= 1;

	*((*buffer)++) = value;

	return SUCCESS;
}

_int32 sd_get_bytes(char **buffer, _int32 *cur_buflen, char *dest_buf, _int32 dest_len)
{
	if(*cur_buflen < dest_len)
		return BUFFER_OVERFLOW;

	sd_memcpy(dest_buf, *buffer, dest_len);

	*cur_buflen -= dest_len;
	*buffer += dest_len;

	return SUCCESS;
}

_int32 sd_set_bytes(char **buffer, _int32 *cur_buflen, char *dest_buf, _int32 dest_len)
{
	if(dest_len==0)
		return INVALID_ARGUMENT;
	
	if(*cur_buflen < dest_len)
		return BUFFER_OVERFLOW;

	sd_memcpy(*buffer, dest_buf, dest_len);

	*cur_buflen -= dest_len;
	*buffer += dest_len;

	return SUCCESS;
}
