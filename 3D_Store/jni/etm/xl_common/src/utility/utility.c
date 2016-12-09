#include "utility/utility.h"
#include "utility/errcode.h"
#include "utility/string.h"
#include "utility/crc.h"

#include "platform/sd_fs.h"

#include <stdlib.h>
#include <string.h>

#if defined(LINUX)
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#elif defined(WINCE)
#include <winsock2.h>
#endif

#define INT2HEX(ch)	((ch) < 10 ? (ch) + '0' : (ch) - 10 + 'A')

_u32 sd_inet_addr(const char *cp)
{

#if defined(LINUX)	
	return inet_addr(cp);
#elif defined(WINCE)
	return inet_addr(cp);
#endif

}

_int32 sd_inet_aton(const char *cp, _u32 *nip)
{
	_int32 ret_val = SUCCESS;

#if defined(LINUX)	
	ret_val = inet_addr(cp);

	if(ret_val == INADDR_NONE)
		ret_val = ERROR_INVALID_INADDR;
	else
	{
		*nip = ret_val;
		ret_val = SUCCESS;
	}

#elif defined(WINCE)
	ret_val = inet_addr(cp);
	
	if(ret_val == INADDR_NONE)
		ret_val = ERROR_INVALID_INADDR;
	else
	{
		*nip = ret_val;
		ret_val = SUCCESS;
	}
#endif

	return ret_val;
}


_int32 sd_inet_ntoa(_u32 inp, char *out_buf, _int32 bufsize)
{

#if defined(LINUX)	
	char *out = NULL;
	struct in_addr addr;
	sd_memset(&addr, 0, sizeof(addr));
	addr.s_addr = inp;
	out = inet_ntoa(addr);
	sd_strncpy(out_buf, out, bufsize);

#elif defined(WINCE)
	char *out = NULL;
	struct in_addr addr;
	addr.s_addr = inp;
	
	out = inet_ntoa(addr);
	sd_strncpy(out_buf, out, bufsize);
#endif

	return SUCCESS;
}

_int32 sd_atoi(const char* nptr)
{
	return atoi(nptr);
}

_int32 sd_srand(_u32 seeds)
{
	srand(seeds);

	return SUCCESS;
}

_int32 sd_rand(void)
{
	return rand();
}

_u16 sd_htons(_u16 h)
{
#if defined(LINUX)	
	return htons(h);
#elif defined(WINCE)
	return htons(h);
#endif
}

_u32 sd_htonl(_u32 h)
{
#if defined(LINUX)	
	return htonl(h);
#elif defined(WINCE)
	return htonl(h);
#endif
}

_u16 sd_ntohs(_u16 n)
{
#if defined(LINUX)	
	return ntohs(n);
#elif defined(WINCE)
	return ntohs(n);
#endif
}

_u32 sd_ntohl(_u32 n)
{
#if defined(LINUX)	
	return ntohl(n);
#elif defined(WINCE)
	return ntohl(n);
#endif
}
_int32 char2hex( unsigned char c , char *out_buf, _int32 out_bufsize)
{
	char hex[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	if(out_bufsize<3) return -1;
	out_buf[0] = hex[c/16];
	out_buf[1] = hex[c%16];
	out_buf[2] = '\0';
	return SUCCESS;
}
_int32 str2hex(const char *in_buf, _int32 in_bufsize, char *out_buf, _int32 out_bufsize)
{
	_int32 ret_val = SUCCESS;
	_int32 idx = 0;
	_u8 *inp = (_u8*)in_buf, ch;

	for(idx = 0; idx < in_bufsize; idx++)
	{
		if(idx * 2 >= out_bufsize)
			break;

		ch = inp[idx] >> 4;
		out_buf[idx * 2] = INT2HEX(ch);
		ch = inp[idx] & 0x0F;
		out_buf[idx * 2 + 1] = INT2HEX(ch);
	}
	return ret_val;
}

//将Hex字符串转换成普通字符串
_int32 hex2str(char* hex, _int32 hex_len, char* str, _int32 str_len)
{
    int i=0;
	int j=0;
    for(j = 0; j < hex_len -1; )
    {
		if(i >= str_len) break;

        _int32 a =  sd_hex_2_int(hex[j++]);
        _int32 b =  sd_hex_2_int(hex[j++]);
        str[i++] = (char)(a *16 + b);
    }
	
	return SUCCESS;
}

_int32 sd_hex_2_int(char chr)
{
	if( chr >= '0' && chr <= '9' )
	{
		return chr - '0';
	}
	if( chr >= 'a' && chr <= 'f' )
	{
		return chr - 'a' + 10;
	}
	if( chr >= 'A' && chr <= 'F' )
	{
		return chr - 'A' + 10;
	}
	else
	{
		return 0;
	}
}

_int32 sd_hexstr_2_int(const char * in_buf, _int32 in_bufsize)
{
	char * temp = (char*)in_buf;
	_int32 result = 0;
	
	if(in_bufsize > 10)
	{
		return 0;
	}
	if(in_bufsize > 8 && (*temp != '0' || (*(temp+1) != 'x' && *(temp+1) != 'X')))
	{
		return 0;
	}
	if(*temp == '0' && (*(temp+1) == 'x' || *(temp+1) == 'X'))
	{
		temp += 2;
		in_bufsize -= 2;
	}
	while(in_bufsize-- > 0)
	{
		result = result*16 + sd_hex_2_int(*temp++);		
	}
	return result;
}

BOOL sd_is_bcid_equal(const _u8* _bcid1,_u32 bcid1_len,const _u8* _bcid2,_u32 bcid2_len)
{
	_int32 i ;

	//LOG_DEBUG("sd_is_bcid_equal");
	if(_bcid1 == NULL||_bcid2==NULL)
	{
		return FALSE;
	}
	
	if(bcid1_len!=bcid2_len)
		return FALSE;
	
	for(i=0;i<bcid1_len;i++)
		if(_bcid1[i]!=_bcid2[i]) 
		{
			return FALSE;
		}

	return TRUE;
}

BOOL sd_is_cid_equal(const _u8* _cid1,const _u8* _cid2)
{
	_int32 i ;

	//LOG_DEBUG("tm_is_cid_equal");
	if(_cid1 == NULL||_cid2==NULL)
	{
		return FALSE;
	}

	for(i=0;i<CID_SIZE;i++)
		if(_cid1[i]!=_cid2[i]) 
		{
			return FALSE;
		}

	return TRUE;
}

/* @Simple Function@
 * Return : TRUE if valid,or FALSE
 *
 */
BOOL sd_is_cid_valid(const _u8* _cid)
{
	_u8 _cid_t[CID_SIZE];

	if(_cid == NULL)
	{
		return FALSE;
	}

	sd_memset(_cid_t,0,CID_SIZE);
	
	if(sd_is_cid_equal(_cid,_cid_t)) 
	{
		return FALSE;
	}

	return TRUE;
}

BOOL sd_is_bcid_valid(_u64 filesize, _u32 bcid_size, _u32 block_size)
{
    _u32 bcid_num = (filesize + block_size-1)/(block_size );
    return (bcid_num==bcid_size) ? TRUE : FALSE;
}

_u32 sd_elf_hashvalue(const char *str, _u32 hash_value)
{
	_u32 x = 0;

	if(!str)
		return 0;

	while(*str)
	{
		hash_value  =  (hash_value << 4 ) + (*(str++));
		if((x  =  hash_value & 0xF0000000L) != 0)
		{
			hash_value  ^=  (x  >> 24 );
			hash_value  &=   ~ x;
		}
	}
	return hash_value;
}

_u64 sd_generate_hash_from_size_crc_hashvalue(const _u8 * data ,_u32 data_size)
{
	_u32 hash = MAX_U32,x = 0,len = data_size;
	const _u8 * p_data = data;
	_u16 crc_value=0xffff;
	_u64 final_hash = 0;
	/* final_hash =  hash<<32|crc_value<<16|data_size&0x0000FFFF */
	if(data==NULL||data_size==0) return 0;

         while  ( len!=0)
          {
                hash  =  (hash  <<   4 )  +  ( * p_data  );
                 if  ((x  =  hash  &   0xF0000000L )  !=   0 )
                  {
                        hash  ^=  (x  >>   24 );
                        hash  &=   ~ x;
                } 
		p_data++;
		len--;
        } 
		 
	crc_value = sd_add_crc16(crc_value, (const void*)data, (_int32)data_size);
	crc_value = sd_inv_crc16(crc_value);

	len = 0x0000FFFF&data_size;

	final_hash = hash;
	final_hash = (final_hash<<16)|crc_value;
	final_hash = (final_hash<<16)|len;
	return final_hash;
}


_int32 sd_str_to_u64( const char *str, _u32 strlen, _u64 *val )
{
#ifdef  NOT_SUPPORT_LARGE_INT_64
	_u32 _result = 0;
	int j = 0;
	if( str == NULL || strlen == 0 )
		return INVALID_ARGUMENT;

	for(j=0;j<strlen;j++)
	{
		if((str[j]<'0')||(str[j]>'9'))
		{
			return INVALID_ARGUMENT;
		}
		else
		{
			_result *= 10;  
			_result += str[j] - '0';
		}

		if(_result > (_u32)0xffffffff)
			//return BT_UNSUPPORT_64BIT_FILE_SIZE;
			return INVALID_ARGUMENT; 
	}
	*val = _result;
	
#else
	_u64 _result = 0;
	int j = 0;
	if( str == NULL || strlen == 0 )
		return INVALID_ARGUMENT;

	for(j=0;j<strlen;j++)
	{
		if((str[j]<'0')||(str[j]>'9'))
		{
			return INVALID_ARGUMENT;
		}
		else
		{
			_result *= 10;   
			_result += str[j] - '0';   
		}
	}
	*val = _result;
#endif
	return SUCCESS;
}

_int32 sd_u64_to_str( _u64 val, char *str, _u32 strlen )
{
#ifdef  NOT_SUPPORT_LARGE_INT_64
	return sd_snprintf( str, strlen, "%u", val);
#else
	return sd_snprintf( str, strlen, "%llu", val);
#endif
}

_int32 sd_u32_to_str( _u32 val, char *str, _u32 strlen )
{
	return sd_snprintf( str, strlen, "%u", val );
}

_int32 sd_abs(_int32 val)
{
	return abs(val);

}

BOOL sd_data_cmp(_u8* i_data, _u8* d_data, _int32 count)
{
     while(count-->0)
     {
           if(*i_data++ != *d_data++)
		   return FALSE;	
     }

     return TRUE;	 
}

/*
	Convert the string(40 chars hex ) to content id(20 bytes in hex)
*/
_int32 sd_string_to_cid(char * str,_u8 *cid)
{
	int i=0,is_cid_ok=0,cid_byte=0;

	if((str==NULL)||(sd_strlen(str)<40)||(cid==NULL))
		return -1;

	for(i=0;i<20;i++)
	{
		if(((str[i*2])>='0')&&((str[i*2])<='9'))
		{
			cid_byte=(str[i*2]-'0')*16;
		}
		else
		if(((str[i*2])>='A')&&((str[i*2])<='F'))
		{
			cid_byte=((str[i*2]-'A')+10)*16;
		}
		else
		if(((str[i*2])>='a')&&((str[i*2])<='f'))
		{
			cid_byte=((str[i*2]-'a')+10)*16;
		}
		else
			return -1;

		if(((str[i*2+1])>='0')&&((str[i*2+1])<='9'))
		{
			cid_byte+=(str[i*2+1]-'0');
		}
		else
		if(((str[i*2+1])>='A')&&((str[i*2+1])<='F'))
		{
			cid_byte+=(str[i*2+1]-'A')+10;
		}
		else
		if(((str[i*2+1])>='a')&&((str[i*2+1])<='f'))
		{
			cid_byte+=(str[i*2+1]-'a')+10;
		}
		else
			return -1;

		cid[i]=cid_byte;
		if((is_cid_ok==0)&&(cid_byte!=0))
			is_cid_ok=1;
	}

	if(is_cid_ok==1)
	return SUCCESS;
	else
	return 1;
		

}

/*
	Convert the string(chars hex ) to  bytes in hex
*/
_int32 sd_string_to_hex(char * str,_u8 * hex)
{
	int i=0,is_hex_ok=0,hex_byte=0;

	if((str==NULL)||(sd_strlen(str)==0)||(hex==NULL))
		return -1;

	while(str[i*2]!='\0')
	{
		if(((str[i*2])>='0')&&((str[i*2])<='9'))
		{
			hex_byte=(str[i*2]-'0')*16;
		}
		else
		if(((str[i*2])>='A')&&((str[i*2])<='F'))
		{
			hex_byte=((str[i*2]-'A')+10)*16;
		}
		else
		if(((str[i*2])>='a')&&((str[i*2])<='f'))
		{
			hex_byte=((str[i*2]-'a')+10)*16;
		}
		else
			return -1;

		if(((str[i*2+1])>='0')&&((str[i*2+1])<='9'))
		{
			hex_byte+=(str[i*2+1]-'0');
		}
		else
		if(((str[i*2+1])>='A')&&((str[i*2+1])<='F'))
		{
			hex_byte+=(str[i*2+1]-'A')+10;
		}
		else
		if(((str[i*2+1])>='a')&&((str[i*2+1])<='f'))
		{
			hex_byte+=(str[i*2+1]-'a')+10;
		}
		else
			return -1;

		hex[i++]=hex_byte;
		if((is_hex_ok==0)&&(hex_byte!=0))
			is_hex_ok=1;
	}

	if(is_hex_ok==1)
	return SUCCESS;
	else
	return 1;
}



_int32 sd_i32toa(_int32 value, char *buffer, _int32 buf_len, _int32 radix)
{
    return sd_i64toa((_int64)value, buffer, buf_len, radix);
}

_int32 sd_i64toa(_int64 value, char *buffer, _int32 buf_len, _int32 radix)
{
    if(value < 0)
    {
        if(buf_len < 3) /* "-1\0" */
        {
            sd_memset(buffer, 0, buf_len);
            return SUCCESS;
        }
        *buffer = '-';
        return sd_u64toa(-value, buffer + 1, buf_len - 1, radix);
    }
    else
    {
        return sd_u64toa(value, buffer, buf_len, radix);
    }
}

_int32 sd_u32toa(_u32 value, char *buffer, _int32 buf_len, _int32 radix)
{
    return sd_u64toa((_u64)value, buffer, buf_len, radix);
}

_int32 sd_u64toa(_u64 value, char *buffer, _int32 buf_len, _int32 radix)
{
    char hex[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    _int32 idx = 0, last_idx = 0, half_idx = 0;
    _u64 div_value = 0;
    char swap_tmp;

    if(radix > 16 || radix <= 0)
        return INVALID_ARGUMENT;

    idx = 0;
    if(value == 0 && idx < buf_len - 1)
    {
        buffer[idx++] = '0';
    }
    else
    {
        for(div_value = value, idx = 0; div_value > 0 && idx < buf_len - 1; div_value /= radix, idx++)
        {
            buffer[idx] = hex[div_value % radix];
        }
    }

    buffer[idx] = 0;

    /* inverse */

    last_idx = idx - 1;
    half_idx = idx / 2; 
    for(idx = 0; idx < half_idx; idx ++)
    {
        swap_tmp = buffer[idx];
        buffer[idx] = buffer[last_idx - idx];
        buffer[last_idx - idx] = swap_tmp;
    }

    return SUCCESS;
}

_u32 sd_digit_bit_count( _u64 value )
{
	_u32 bit_count = 0;

	if( value == 0 ) return 1;
	
	while( value > 0 )
	{
		value = value / 10;
		bit_count++;
	}
	return bit_count;
}

BOOL is_cmwap_prompt_page(const char * http_header,_u32 header_len)
{
	_int32 len=0;
	char buffer[64];
	char *p1=NULL, *p2=NULL;

	p1 = sd_strstr( (char *)http_header, "HTTP/1.1 200 ",0 ) ;
	if((p1==NULL)||(p1-http_header>=header_len))
	{
		return FALSE;
	}	
	
	p1 = sd_strstr( (char *)http_header, "Content-Type:",0 ) ;
	if((p1!=NULL)&&(p1-http_header<header_len))
	{
		p1+=13;
		p2 = sd_strstr( p1, "\r\n",0 ) ;
		if((p2!=NULL)&&(p2-http_header<header_len))
		{
			len = p2-p1;
			if(len>=64) len = 63;
			sd_memset(buffer, 0, 64);
			sd_memcpy(buffer,p1,len);
			if(NULL!=sd_strstr( buffer, "vnd.wap.",0 ) )
			{
				return TRUE;
			}
		}
	}
	return FALSE;
}

/*******************************************************************************
* 函数名称	: calc_file_cid
* 功能描述	: 计算一个文件的cid
* 参　　数	: const tchar* file_path	文件对应的路径
* 参　　数	:  _u8 * p_cid				保存cid的缓存指针
* 返 回 值	: _int32					=0表示成功，<0表示出错
* 作　　者	: 曾雨青
* 设计日期	: 2011年5月9日
* 修改日期		   修改人		  修改内容
*******************************************************************************/
#include "platform/sd_fs.h"
#include "utility/mempool.h"
#include "utility/sha1.h"
_int32 sd_calc_file_cid(const char* file_path, _u8 * hex_cid)
{
	_int32 ret_val = SUCCESS;
    _u8 cid[CID_SIZE] = {0};
	_u32 file_id = 0,read_len = 0;
	_u64 file_size = 0;
	char * p_buffer = NULL;
	ctx_sha1 cid_hash_ptr;
	
	if(NULL == file_path || '\0' == file_path[0] || NULL == hex_cid )
	{
		return -1;
	}	

	ret_val = sd_open_ex(file_path, O_FS_RDONLY, &file_id);
	CHECK_VALUE(ret_val);

	ret_val = sd_filesize(file_id,&file_size);
	if(ret_val != SUCCESS)
	{
		sd_close_ex( file_id);
		return ret_val;
	}
	

	if(file_size < TCID_SAMPLE_SIZE) // 小尺寸文件计算cid	
	{
		if(0 == file_size)
		{
			sd_close_ex( file_id);
			return -2;
		}

		ret_val = sd_malloc(file_size, (void**)&p_buffer);
		if(ret_val  != SUCCESS)
		{
			sd_close_ex( file_id);
			return ret_val;
		}

		/* 用这个文件的内容计算CID */
		ret_val =  sd_pread( file_id, p_buffer,(_int32) file_size,0, &read_len);
		if(ret_val!=SUCCESS||read_len != file_size)
		{
			sd_close_ex( file_id);
			SAFE_DELETE(p_buffer);
			return -3;
		}

		sha1_initialize(&cid_hash_ptr);	   
		sha1_update(&cid_hash_ptr, (unsigned char*)p_buffer, read_len);
        sha1_finish(&cid_hash_ptr,cid);
        sd_cid_to_hex_string(cid, CID_SIZE, hex_cid, CID_SIZE * 2 );

		sd_close_ex( file_id);
		SAFE_DELETE(p_buffer);
		return SUCCESS;		
	}

	/* 大文件计算CID，读取首尾和中间三段数据进行计算 */
	ret_val = sd_malloc(TCID_SAMPLE_UNITSIZE, (void**)&p_buffer);
	if(ret_val  != SUCCESS)
	{
		sd_close_ex( file_id);
		return ret_val;
	}

	/* 读取首段数据 */
	ret_val =  sd_pread( file_id, p_buffer, TCID_SAMPLE_UNITSIZE,0, &read_len);
	if(ret_val!=SUCCESS||read_len != TCID_SAMPLE_UNITSIZE)
	{
		sd_close_ex( file_id);
		SAFE_DELETE(p_buffer);
		return -4;
	}

	 sha1_initialize(&cid_hash_ptr);	   
	 sha1_update(&cid_hash_ptr, (unsigned char*)p_buffer, read_len);



	/* 读取中间段数据 */
	ret_val =  sd_pread( file_id, p_buffer, TCID_SAMPLE_UNITSIZE,file_size/3, &read_len);
	if(ret_val!=SUCCESS||read_len != TCID_SAMPLE_UNITSIZE)
	{
		sd_close_ex( file_id);
		SAFE_DELETE(p_buffer);
		return -5;
	}

	 sha1_update(&cid_hash_ptr, (unsigned char*)p_buffer, read_len);

	/* 读取末段数据 */
	ret_val =  sd_pread( file_id, p_buffer, TCID_SAMPLE_UNITSIZE,file_size-TCID_SAMPLE_UNITSIZE, &read_len);
	if(ret_val!=SUCCESS||read_len != TCID_SAMPLE_UNITSIZE)
	{
		sd_close_ex( file_id);
		SAFE_DELETE(p_buffer);
		return -6;
	}

	sha1_update(&cid_hash_ptr, (unsigned char*)p_buffer, read_len);
    sha1_finish(&cid_hash_ptr, cid);
    sd_cid_to_hex_string(cid, CID_SIZE, hex_cid, CID_SIZE * 2 );

	sd_close_ex( file_id);
	SAFE_DELETE(p_buffer);

	return SUCCESS;
}

/*******************************************************************************
* 函数名称	: calc_buf_cid
* 功能描述	: 计算一字符串的cid
* 参　　数	: const char *buf 
* 参    数    : buf_len
* 参　　数	:  _u8 * p_cid				保存cid的缓存指针
* 返 回 值	: _int32					=0表示成功，<0表示出错
* 作　　者	: 钟炳鑫
* 设计日期	: 2012年9月19日
* 修改日期		   修改人		  修改内容
*******************************************************************************/
_int32 sd_calc_buf_cid(const char* buf,int buf_len, _u8 * p_cid)
{
    _int32 ret_val = SUCCESS;
	_u32 read_len = 0;
	_u64 file_size = 0;
	char * p_buffer = NULL;
	ctx_sha1 cid_hash_ptr;
	

    
    file_size = buf_len;
    
    
	if(file_size < TCID_SAMPLE_SIZE)	
	{

        
		p_buffer = (char*)malloc(file_size);
		if(p_buffer  == NULL)
		{

            free(p_buffer);
            p_buffer = NULL;
			return -3;
		}
        
        read_len = file_size;
        memcpy((void *)p_buffer, (void *)buf, file_size);
        
        sha1_initialize(&cid_hash_ptr);	   
        sha1_update(&cid_hash_ptr, (unsigned char*)p_buffer, read_len);
        sha1_finish(&cid_hash_ptr,p_cid);        

		free(p_buffer);
        p_buffer = NULL;
		return SUCCESS;		
	}
    
    p_buffer = (char*)malloc(TCID_SAMPLE_UNITSIZE);
    if(p_buffer  == NULL)
    {

        free(p_buffer);
        p_buffer = NULL;
        return -5;
    }
    read_len = TCID_SAMPLE_UNITSIZE;
    memcpy(p_buffer,buf, read_len);
    
    sha1_initialize(&cid_hash_ptr);	   
    sha1_update(&cid_hash_ptr, (unsigned char*)p_buffer, read_len);
    
 
    read_len = TCID_SAMPLE_UNITSIZE;
    memcpy(p_buffer,buf+file_size/3, read_len);
    
    sha1_update(&cid_hash_ptr, (unsigned char*)p_buffer, read_len);    

    read_len = TCID_SAMPLE_UNITSIZE;
    memcpy(p_buffer,buf+file_size-TCID_SAMPLE_UNITSIZE, read_len);
    sha1_update(&cid_hash_ptr, (unsigned char*)p_buffer, read_len);
    sha1_finish(&cid_hash_ptr,p_cid);
    

    free(p_buffer);
    p_buffer = NULL;
    
    
	return SUCCESS; 
    
}



/*******************************************************************************
* 函数名称	: cid_utility::calc_file_gcid
* 功能描述	: 计算文件的gcid
* 参　　数	: const tchar* file_path	文件的路径
* 参　　数	: byte* gcid_buf			保存gcid的缓存指针
* 参　　数	: uint32& buffer_len		保存gcid的缓存大小
* 参　　数	: sint32& bcid_num			块数目
* 参　　数	: byte** bcid_buf			保存块的缓存(注意：如果bcid_buf!=NULL,则本函数为bcid_buf分配了内存，调用者有义务释放这部分内存)
* 返 回 值	: sint32					=0表示成功，<0表示出错
* 作　　者	: 王卫华
* 设计日期	: 2009年3月12日
* 修改日期		   修改人		  修改内容
*******************************************************************************/
_u32 sd_calc_gcid_part_size(_u64 file_size)
{
	const _u32 max_block_size = (2 << 20); // 2M
	_u32 block_size = (_u32)(256 << 10);
	const _u32 max_count = 512;

	_u32 count = (_u32)((file_size + block_size - 1) / block_size);
	
	if(0 == file_size)
	{
		return 0;
	}

	while((count > max_count) && (block_size < max_block_size))
	{
		block_size <<= 1; // 块的大小加倍
		count = (_u32)((file_size + block_size - 1) / block_size);
	}

	return block_size;
}

_int32 sd_calc_file_gcid(const char* file_path, _u8* gcid, _int32 * bcid_num, _u8 ** bcid_buf,_u64 * file_size)
{
	_int32 ret_val = SUCCESS;
	_u32 file_id = 0, block_size = 0,block_count = 0,i = 0;
	_u64 tmp_file_size = 0;
	const _u32 max_temp_data_size = (16 << 10); // 16K
	_u8 temp_data[16 << 10];
	_u8 sha_buf[CID_SIZE],*tmp_bcid = NULL;
       ctx_sha1 bctx, gctx;
	
	if(NULL == file_path || '\0' == file_path[0] || NULL == gcid)
	{
		return -1;
	}

	ret_val = sd_open_ex((char*)file_path, O_FS_RDONLY, &file_id);
	CHECK_VALUE(ret_val);

	ret_val = sd_filesize(file_id,&tmp_file_size);
	if(ret_val != SUCCESS)
	{
		sd_close_ex( file_id);
		return ret_val;
	}
	
	if(file_size!=NULL)
	{
		*file_size = tmp_file_size;
	}
	
	// 计算分片的大小及个数
	block_size = sd_calc_gcid_part_size(tmp_file_size);

	if(block_size <= 0)
	{
		sd_close_ex( file_id);
		return -4;
	}

	block_count = (_u32)((tmp_file_size + block_size - 1) / block_size);
	if(bcid_num!=NULL)
	{
		*bcid_num = block_count;
	}

	if(bcid_buf != NULL)
	{
		ret_val = sd_malloc(CID_SIZE*block_count, (void**)bcid_buf);
		if(ret_val != SUCCESS)
		{
			sd_close_ex( file_id);
			return ret_val;
		}
		tmp_bcid = *bcid_buf;
	}

	sha1_initialize(&gctx);

	// read each part of the file
	for( i = 0; i < block_count; i++)
	{
		_u32 blksize = block_size;
		_u32 left_bytes = 0;
		
		// 一块文件的内容计算成为bcid
		sha1_initialize(&bctx);

		if(i == block_count - 1) // 最后一块
		{
			blksize = (_u32)(tmp_file_size - i * block_size);
		}

		left_bytes = blksize;
		while(left_bytes > 0)
		{
			_u32 readed = 0;
			_u32 read_bytes = (left_bytes < max_temp_data_size ? left_bytes : max_temp_data_size);
			ret_val = sd_read( file_id,(char *) temp_data,read_bytes, &readed);
			if(ret_val != SUCCESS)
			{
				sd_close_ex( file_id);
				SAFE_DELETE(tmp_bcid);
				return ret_val;
			}
			if(readed != read_bytes)
			{
				sd_close_ex( file_id);
				SAFE_DELETE(tmp_bcid);
				return -1;
			}

			sha1_update(&bctx, temp_data, readed);
			left_bytes -= (_u32)readed;
		}
       	sha1_finish(&bctx,sha_buf); 
		sha1_update(&gctx,sha_buf, CID_SIZE);// 根据bcid计算gcid		
#if 0 //def _DEBUG
{
      		char my_bcid[41];
		str2hex((char*)sha_buf, CID_SIZE, my_bcid, 40);
		my_bcid[40] = '\0';
		LOG_ERROR("sd_calc_file_gcid,bcid[%u] = %s",  i,my_bcid);
}
#endif

		if(bcid_buf != NULL)
		{
			sd_memcpy(*bcid_buf + i * CID_SIZE, sha_buf, CID_SIZE);
		}
	}

	sd_close_ex( file_id);

      	sha1_finish(&gctx,gcid); 
		
#if 0 //def _DEBUG
{
      		char my_gcid[41];
		str2hex((char*)gcid, CID_SIZE, my_gcid, 40);
		my_gcid[40] = '\0';
		LOG_ERROR("sd_calc_file_gcid,file_path=%s, gcid = %s,tmp_file_size = %llu",  file_path,my_gcid, tmp_file_size);
}
#endif
	return SUCCESS;
}


/*xml字符串中的实体引用替换*/
_int32 sd_xml_entity_ref_replace(char* xml_str,_u32 total_buffer_len)
{
	char *pos=NULL,*pos1=NULL,*pos2=NULL;
	char *tmp=NULL;
	_int32 len=0,ret=0;
	
	len=sd_strlen(xml_str);
	ret=sd_malloc(len, (void**)&tmp);
	CHECK_VALUE(ret);

	pos = xml_str;

	do
	{
		sd_memset(tmp, 0, len);
		switch (*pos)
		{
		case '&':
			pos1=pos+1;
			sd_memcpy(tmp,pos1,sd_strlen(pos1)+1);
			pos2=pos+5;
			if (sd_strlen(xml_str)+5<total_buffer_len)
			{
				sd_strncpy(pos,"&amp;",5);
				sd_strncpy(pos2,tmp,sd_strlen(tmp)+1);
				pos+=5;
			}
			else
			{
				SAFE_DELETE(tmp);
				return -1;
			}
			break;
		case '\'':
			pos1=pos+1;
			sd_memcpy(tmp,pos1,sd_strlen(pos1)+1);
			pos2=pos+6;
			if (sd_strlen(xml_str)+6<total_buffer_len)
			{
				sd_strncpy(pos,"&apos;",6);
				sd_strncpy(pos2,tmp,sd_strlen(tmp)+1);
				pos+=6;
			}
			else
			{
				SAFE_DELETE(tmp);
				return -1;
			}
			break;
		case '\"':
			pos1=pos+1;
			sd_memcpy(tmp,pos1,sd_strlen(pos1)+1);
			pos2=pos+6;
			if (sd_strlen(xml_str)+6<total_buffer_len)
			{
				sd_strncpy(pos,"&quot;",6);
				sd_strncpy(pos2,tmp,sd_strlen(tmp)+1);
				pos+=6;
			}
			else
			{
				SAFE_DELETE(tmp);
				return -1;
			}
			break;
		case '<':
			pos1=pos+1;
			sd_memcpy(tmp,pos1,sd_strlen(pos1)+1);
			pos2=pos+4;
			if (sd_strlen(xml_str)+4<total_buffer_len)
			{
				sd_strncpy(pos,"&lt;",4);
				sd_strncpy(pos2,tmp,sd_strlen(tmp)+1);
				pos+=4;
			}
			else
			{
				SAFE_DELETE(tmp);
				return -1;
			}
			break;
		case '>':
			pos1=pos+1;
			sd_memcpy(tmp,pos1,sd_strlen(pos1)+1);
			pos2=pos+4;
			if (sd_strlen(xml_str)+4<total_buffer_len)
			{
				sd_strncpy(pos,"&gt;",4);
				sd_strncpy(pos2,tmp,sd_strlen(tmp)+1);
				pos+=4;
			}
			else
			{
				SAFE_DELETE(tmp);
				return -1;
			}
			break;
		default:
			pos++;
		}
	}while(*pos);

	SAFE_DELETE(tmp);
	return SUCCESS;
}

/*******************************************************************************
* 函数名称	: sd_get_file_type_from_name
* 功能描述	: 从文件名中得出文件的类型
* 参　　数	: const tchar* file_name	文件名
* 返 回 值	: _int32				文件类型，<0表示出错
* 作　　者	: 曹文锋
* 设计日期	: 2011年12月14日
* 修改日期		   修改人		  修改内容
*******************************************************************************/
enum {
    FILE_TYPE_UNKNOWN = 0,
    FILE_TYPE_TXT, 
    FILE_TYPE_PIC ,
    FILE_TYPE_AUDIO ,
    FILE_TYPE_VEDIO ,
    FILE_TYPE_APP ,      // 5
    FILE_TYPE_DOC , 
    FILE_TYPE_PDF , 
    FILE_TYPE_XLS ,
    FILE_TYPE_ZIP ,
    FILE_TYPE_PPT	// 10
};

_int32 sd_get_file_type_from_name(const char *p_file_name)
{
	_int32 file_type = 0;
	if(*p_file_name == '\0')
		return -1;

	char *p_point = sd_strrchr((char*)p_file_name, '.');
	if(p_point == NULL)
	{
		return FILE_TYPE_UNKNOWN;
	}

	char *p_extended = p_point + 1;

	if(0==sd_stricmp(p_extended, "txt"))
		file_type=FILE_TYPE_TXT;
	else if(0==sd_stricmp(p_extended, "chm"))	
		file_type=FILE_TYPE_TXT;
	else if(0==sd_stricmp(p_extended, "doc"))	
		file_type=FILE_TYPE_DOC;
 	else if(0==sd_stricmp(p_extended, "docx"))	
		file_type=FILE_TYPE_DOC;
	else if(0==sd_stricmp(p_extended, "pdf"))	
		file_type=FILE_TYPE_PDF;
   	else if(0==sd_stricmp(p_extended, "xls"))	
		file_type=FILE_TYPE_XLS;
   	else if(0==sd_stricmp(p_extended, "xlsx"))	
		file_type=FILE_TYPE_XLS;
   	else if(0==sd_stricmp(p_extended, "ppt"))	
		file_type=FILE_TYPE_PPT;
   	else if(0==sd_stricmp(p_extended, "pptx"))	
		file_type=FILE_TYPE_PPT;
	//图像格式即图像文件存放的格式，通常有JPEG、TIFF、RAW、BMP、GIF、PNG等。
	else if(0==sd_stricmp(p_extended, "jpg"))	
		file_type=FILE_TYPE_PIC;
	else if(0==sd_stricmp(p_extended, "bmp"))			
		file_type=FILE_TYPE_PIC;
	else if(0==sd_stricmp(p_extended, "png"))			
		file_type=FILE_TYPE_PIC;
	else if(0==sd_stricmp(p_extended, "gif"))			
		file_type=FILE_TYPE_PIC;
	else if(0==sd_stricmp(p_extended, "jpeg"))			
		file_type=FILE_TYPE_PIC;
	else if(0==sd_stricmp(p_extended, "tiff"))			
		file_type=FILE_TYPE_PIC;
	else if(0==sd_stricmp(p_extended, "raw"))			
		file_type=FILE_TYPE_PIC;
	//音频格式日新月异，常用音频格式包括：MP3、WAV、WMA、AIFF、MIDI、AMR、VQF、ASF、AAC。
	else if(0==sd_stricmp(p_extended, "mp3"))	
		file_type=FILE_TYPE_AUDIO;
	else if(0==sd_stricmp(p_extended, "wav"))	
		file_type=FILE_TYPE_AUDIO;
	else if(0==sd_stricmp(p_extended, "wma"))	
		file_type=FILE_TYPE_AUDIO;
	else if(0==sd_stricmp(p_extended, "aiff"))	
		file_type=FILE_TYPE_AUDIO;
	else if(0==sd_stricmp(p_extended, "midi"))	
		file_type=FILE_TYPE_AUDIO;
	else if(0==sd_stricmp(p_extended, "amr"))	
		file_type=FILE_TYPE_AUDIO;
	else if(0==sd_stricmp(p_extended, "vqf"))	
		file_type=FILE_TYPE_AUDIO;
	else if(0==sd_stricmp(p_extended, "asf"))	
		file_type=FILE_TYPE_AUDIO;
	else if(0==sd_stricmp(p_extended, "aac"))	
		file_type=FILE_TYPE_AUDIO;
	else if(0==sd_stricmp(p_extended, "ape"))	
		file_type=FILE_TYPE_AUDIO;
	else if(0==sd_stricmp(p_extended, "ogg"))	
		file_type=FILE_TYPE_AUDIO;
	//视频格式：RM、RMVB、AVI、MPG 、MPEG、FLV、3GP、MP4、SWF、MKV、MOV
	else if(0==sd_stricmp(p_extended, "avi"))
		file_type=FILE_TYPE_VEDIO;
    else if(0==sd_stricmp(p_extended, "xv"))
		file_type=FILE_TYPE_VEDIO;
	else if(0==sd_stricmp(p_extended, "rm"))
		file_type=FILE_TYPE_VEDIO;
	else if(0==sd_stricmp(p_extended, "rmvb"))
		file_type=FILE_TYPE_VEDIO;
	else if(0==sd_stricmp(p_extended, "mpg"))
		file_type=FILE_TYPE_VEDIO;
	else if(0==sd_stricmp(p_extended, "mpeg"))
		file_type=FILE_TYPE_VEDIO;
	else if(0==sd_stricmp(p_extended, "flv"))
		file_type=FILE_TYPE_VEDIO;
	else if(0==sd_stricmp(p_extended, "3gp"))
		file_type=FILE_TYPE_VEDIO;
	else if(0==sd_stricmp(p_extended, "mp4"))
		file_type=FILE_TYPE_VEDIO;
	else if(0==sd_stricmp(p_extended, "swf"))
		file_type=FILE_TYPE_VEDIO;
	else if(0==sd_stricmp(p_extended, "mkv"))
		file_type=FILE_TYPE_VEDIO;
	else if(0==sd_stricmp(p_extended, "mov"))
		file_type=FILE_TYPE_VEDIO;
	else if(0==sd_stricmp(p_extended, "f4v"))
		file_type=FILE_TYPE_VEDIO;
    else if(0==sd_stricmp(p_extended, "wmv"))
		file_type=FILE_TYPE_VEDIO;
    else if(0==sd_stricmp(p_extended, "asf"))
		file_type=FILE_TYPE_VEDIO;
    else if(0==sd_stricmp(p_extended, "mpga"))
		file_type=FILE_TYPE_VEDIO;
    else if(0==sd_stricmp(p_extended, "m3u"))
		file_type=FILE_TYPE_VEDIO;
    else if(0==sd_stricmp(p_extended, "dat"))
		file_type=FILE_TYPE_VEDIO;
	//可执行程序格式: EXE、IMG、IPA、APK
	else if(0==sd_stricmp(p_extended, "exe"))
		file_type=FILE_TYPE_APP;
	else if(0==sd_stricmp(p_extended, "img"))
		file_type=FILE_TYPE_APP;
	else if(0==sd_stricmp(p_extended, "ipa"))
		file_type=FILE_TYPE_APP;
	else if(0==sd_stricmp(p_extended, "apk"))
		file_type=FILE_TYPE_APP;
 	else if(0==sd_stricmp(p_extended, "msi"))
		file_type=FILE_TYPE_APP;
 	else if(0==sd_stricmp(p_extended, "jar"))
		file_type=FILE_TYPE_APP;
   //压缩包格式: rar, zip, 7z
	else if(0==sd_stricmp(p_extended, "rar"))
		file_type=FILE_TYPE_ZIP;
	else if(0==sd_stricmp(p_extended, "zip"))
		file_type=FILE_TYPE_ZIP;
	else if(0==sd_stricmp(p_extended, "7z"))
		file_type=FILE_TYPE_ZIP;
	else if(0==sd_stricmp(p_extended, "iso"))
		file_type=FILE_TYPE_ZIP;
	else if(0==sd_stricmp(p_extended, "bz2"))
		file_type=FILE_TYPE_ZIP;
	else if(0==sd_stricmp(p_extended, "tar"))
		file_type=FILE_TYPE_ZIP;
	else if(0==sd_stricmp(p_extended, "ra"))
		file_type=FILE_TYPE_ZIP;
	else if(0==sd_stricmp(p_extended, "gz"))
		file_type=FILE_TYPE_ZIP;
	//未知文件类型返回0
	else
		file_type=FILE_TYPE_UNKNOWN;

	return file_type;
}

void sd_exit(_int32 errcode)
{
    exit(errcode);
}

_int32 sd_cid_to_hex_string(_u8* cid, _int32 cid_len,  _u8* hex_cid_str, _int32 hex_cid_len)
{
    _int32 i = 0;
    _int32 l = 0;
    _int32 r = 0;
    _u8 tmp = 0;
    char hex_array[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    if (cid_len * 2 > hex_cid_len)
    {
        return SD_ERROR;
    }
    for( i = 0; i < cid_len; ++i)
    {
        tmp = cid[i];
        l = (tmp>>4) & (0x0F);
        hex_cid_str[i*2] = hex_array[l];
        r = tmp & 0x0F;
        hex_cid_str[i*2+1] = hex_array[r];
    }
    return SUCCESS;
}

_int32 sd_gm_time(_u32 time_sec, TIME_t *p_time)
{
    struct tm *tm_time = NULL;
    time_t _time_sec = (time_t)time_sec;
    if (p_time)
    {
        sd_memset(p_time, 0, sizeof(*p_time));
        if ((_u32)-1 != _time_sec)
        {
            tm_time = gmtime(&_time_sec);
            if (tm_time)
            {
                p_time->sec = tm_time->tm_sec;
                p_time->min = tm_time->tm_min;
                p_time->hour = tm_time->tm_hour;
                p_time->mday = tm_time->tm_mday;
                p_time->mon = tm_time->tm_mon;
                p_time->year = tm_time->tm_year + 1900;
                p_time->wday = tm_time->tm_wday;
                p_time->yday = tm_time->tm_yday;
//              p_time->isdst = tm_time->tm_isdst;
                return 0;
            }
        }
    }
    return 1;
}

/** convert first len of str to int64, *val will be 0 if not success*/
_int32 sd_str_to_i64_v2(const char* str, _u32 len, _int64* val)
{
#ifdef  NOT_SUPPORT_LARGE_INT_64
#error "Not work well without int64 !!"
#else

	*val=0;
	if( str == NULL || len == 0 ) return INVALID_ARGUMENT;

	BOOL sign = FALSE;
	const char* p_cur = str;
	const char* p_end = str+len;

	if(*p_cur=='-')sign=TRUE,++p_cur;
	else if(*p_cur=='+') ++p_cur;

	for( ; p_cur!=p_end ; ++p_cur)
	{
		if(*p_cur>='0' && *p_cur<='9')
			*val = *val*10 + (*p_cur&0xf);
		else return INVALID_ARGUMENT;
	}

	if(sign) *val = -*val;

#endif
	return SUCCESS;
}

#if defined(LINUX)      
_int32 sd_getaddrinfo(const char *host, char *ip, int ip_len)
{
    struct addrinfo hints;
    struct addrinfo *result = NULL;
    struct sockaddr_in *sin_p = NULL;
    int ret_val = 0;

    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_family = AF_INET; /* only for ipv4 */

    ret_val = getaddrinfo(host, NULL, &hints, &result);
    if(ret_val != 0 || !result) {
        printf("getaddrinfo: %s, return: %d(%s).\n", host, ret_val,  gai_strerror(ret_val));
        return ret_val;
    }

    sin_p = (struct sockaddr_in *)result->ai_addr;
    inet_ntop(AF_INET, &sin_p->sin_addr, ip, ip_len);

    if (result)
        freeaddrinfo(result);

    return SUCCESS;
}
#endif

