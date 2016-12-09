#include "em_utility.h"
#include "em_errcode.h"
#include "em_string.h"
#include "em_map.h"
#include "em_url.h"
#include "em_interface/em_fs.h"

#include "utility/base64.h"
#include "utility/mempool.h"
#include "utility/url.h"
#include "utility/sd_iconv.h"
#include "utility/list.h"
#include "utility/time.h"
#include "et_interface/et_interface.h"
#include "platform/sd_gz.h"


#include <regex.h>
#include <stdlib.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>


#include "em_logid.h"

#ifdef EM_LOGID
#undef EM_LOGID
#endif

#define EM_LOGID EM_LOGID_COMMON
#include "em_logger.h"

#define INT2HEX(ch)	((ch) < 10 ? (ch) + '0' : (ch) - 10 + 'A')


LIST* g_detect_regex;
LIST* g_detect_string;

/* Convert BASE64  to char string
 * Make sure the length of bufer:d larger than 3/4 length of source string:s 
 * Returns: 0:success, -1:failed
 */
_int32 em_base64_decode(const char *s,_u32 str_len,char *d)
{
        char base64_table[255],s_buffer[MAX_URL_LEN],*p_s=s_buffer;
        _int32 clen,len,i,j;
 	//EM_LOG_DEBUG( "em_base64_decode" );
	
	 em_memset(base64_table,0,255);
	 em_memset(p_s,0,MAX_URL_LEN);
	 
        for(j=0,i='A';i<='Z';i++) 
                base64_table[i]=j++;
        for(i='a';i<='z';i++)
                base64_table[i]=j++;
        for(i='0';i<='9';i++)
                base64_table[i]=j++;
        base64_table['+']=j++;
        base64_table['/']=j++;
        base64_table['=']=j;

	em_strncpy(p_s,s,str_len);
        len=em_strlen(p_s);
	 len =len%4;
        if(len!=0) 
	{
	 	len =4-(len%4);
		while(len--)
			em_strcat(p_s,"=",2);
	}
        len=em_strlen(p_s);
	clen=len/4;
	
       for(;clen--;)
          {
                *d=base64_table[(int)(*p_s++)]<<2&0xfc;
                *d++|=base64_table[(int)(*p_s)]>>4;
		  *d=base64_table[(int)(*p_s++)]<<4&0xf0;
                *d++|=base64_table[(int)(*p_s)]>>2&0x0f;
                *d=base64_table[(int)(*p_s++)]<<6;
                if(*p_s!='=')  
                   *d++|=base64_table[(int)(*p_s++)];
          }
	   
        return 0;
}
/* 用bt任务的info_hash 生成磁力链*/
_int32 em_generate_magnet_url(_u8 info_hash[INFO_HASH_LEN], const char * display_name,_u64 total_size,char * url_buffer ,_u32 buffer_len)
{
	_int32 ret_val = SUCCESS;
	char hash_buffer[INFO_HASH_LEN*2+4] = {0};
	char name_buffer[MAX_FILE_NAME_BUFFER_LEN] = {0};

	if(sd_is_cid_valid(info_hash))
	{
		str2hex((char*)info_hash, INFO_HASH_LEN, hash_buffer, INFO_HASH_LEN*2+2);
		EM_LOG_DEBUG("em_generate_magnet_url:info_hash=%s",hash_buffer);
		sd_strtolower(hash_buffer);
	}
	else
	{
		CHECK_VALUE(INVALID_EIGENVALUE);
	}

	if(total_size==0)
	{
		ret_val = sd_snprintf(url_buffer,buffer_len, "magnet:?xt=urn:btih:%s",hash_buffer);
	}
	else
	{
		ret_val = sd_snprintf(url_buffer,buffer_len, "magnet:?xt=urn:btih:%s&xl=%llu",hash_buffer,total_size);
	}
	
	if(sd_strlen(url_buffer)<20)
	{
		return -1;
	}

	if(display_name!=NULL && sd_strlen(display_name)!=0)
	{
		ret_val = url_object_encode_ex(display_name, name_buffer ,MAX_FILE_NAME_BUFFER_LEN-1);
		if(ret_val!=-1)
		{
			sd_strcat(url_buffer, "&dn=",sd_strlen("&dn="));
			if(buffer_len-sd_strlen(url_buffer)-1<sd_strlen(name_buffer))
			{
				sd_strcat(url_buffer, name_buffer,buffer_len-sd_strlen(url_buffer)-1);
			}
			else
			{
				sd_strcat(url_buffer, name_buffer,sd_strlen(name_buffer));
			}
		}

	}
	EM_LOG_DEBUG("em_generate_magnet_url:%s",url_buffer);
	return SUCCESS;
}

_int32 em_parse_magnet_url(const char * url ,_u8 info_hash[INFO_HASH_LEN], char * name_buffer,_u64 * file_size)
{
	_int32 ret_val = SUCCESS;
	char * point_pos = NULL;
	//_u8 info_hash[INFO_HASH_LEN]={0};
	char tmp_buffer[MAX_FILE_NAME_BUFFER_LEN]={0};
	_u32 buffer_len = MAX_FILE_NAME_BUFFER_LEN,xt_len = 40;
	BOOL is_base32 = FALSE;
	
	/* 只嗅探用BT infohash 做urn 的磁力链 */
	point_pos = sd_strstr(url, "btih:",0);
	if((point_pos!=NULL)&&(sd_strlen(point_pos+5)>=32))
	{
		char *p_xl = NULL,*p_dn = NULL,*p_end = NULL;

		if((sd_strlen(point_pos+5)<40)||sd_string_to_cid(point_pos+5, info_hash)!=SUCCESS)
		{
			ret_val = sd_decode_base32(point_pos+5, 32,(char*) info_hash, INFO_HASH_LEN);
			CHECK_VALUE(ret_val);

			is_base32 = TRUE;
			xt_len = 32;
		}
		
		/* 文件大小 */
		p_xl = sd_strstr(url, "xl=",0);
		if(p_xl!=NULL&&file_size!=NULL)
		{
			p_xl+=3;
			p_end = sd_strstr(p_xl, "&", 1);
			if(p_end!=NULL)
			{
				sd_strncpy(tmp_buffer, p_xl,p_end-p_xl);
				p_xl = tmp_buffer;
			}
			ret_val = sd_str_to_u64(p_xl, sd_strlen(p_xl), file_size);
			sd_assert(ret_val==SUCCESS);
			
		}
		
		/* 文件名 */
		p_dn = sd_strstr(url, "dn=",0);
		if(p_dn!=NULL)
		{
			p_dn+=3;
			p_end = sd_strstr(p_dn, "&", 1);
			if(p_end!=NULL)
			{
				sd_strncpy(name_buffer, p_dn,p_end-p_dn);
			}
			else
			{
				sd_strncpy(name_buffer, p_dn,MAX_FILE_NAME_BUFFER_LEN-1);
			}
			
			/* remove all the '%' */
			sd_decode_file_name(name_buffer,NULL, sd_strlen(name_buffer));

			/* Check if Bad file name ! */
			sd_get_valid_name(name_buffer,NULL);
			
			ret_val = sd_any_format_to_utf8(name_buffer, sd_strlen(name_buffer), tmp_buffer,&buffer_len);
			sd_assert(ret_val!=-1);
			if(ret_val>0)
			{
				sd_strncpy(name_buffer,tmp_buffer,MAX_FILE_NAME_BUFFER_LEN-1);
			}
		}
		else
		{
			/* 用btih 字段做文件名*/
			sd_strncpy(name_buffer,point_pos+5,xt_len);
		}
	}
	else
	{
		return INVALID_URL;
	}
	return ret_val;
}
_int32 em_replace_7c(char * str)
{
	sd_replace_str(str,"%7C", "|");
	sd_replace_str(str,"%7c", "|");
	return SUCCESS;
}

_int32 em_clear_commas(char *str)
{
	char *p_comma = NULL, *p_str = NULL;
	p_str = str;
	p_comma = sd_strchr(str, ',', 0);
	while(p_comma != NULL)
	{
		p_str = p_comma;
		while(*p_str != '\0')
		{
			*p_str = *(p_str+1);
			p_str++;
		}
		*p_str = '\0';
		p_comma = sd_strchr(str, ',', 0);
	}
    return SUCCESS;
}

_u64 em_filesize_str_to_u64(char *filesize_str)
{
    _u64 scale = 1, file_size = 0;
    _int32 file_size_num = 0;
    _int32 length = 0;
    length = sd_strlen(filesize_str);
    char *file_size_str = NULL;

    sd_malloc(length+1, (void*)&file_size_str);
    sd_memset(file_size_str, 0, length+1);
    
    sd_memcpy(file_size_str, filesize_str, length);
    
    em_clear_commas(file_size_str);// 去除其中逗号
    
    char last_char = file_size_str[length -1];
    if(last_char == 'B' ||last_char == 'b')
    {
        last_char = file_size_str[length -2];
        if('0' < last_char && last_char < '9')
        {
            last_char  = file_size_str[length -1];
            file_size_str[length -1] = '\0';
        }
        else
        {
            file_size_str[length -2] = '\0';
        }
    }
    
    if(last_char == 'K' || last_char == 'k')
    {
        scale = 1024;
    }
    else if(last_char == 'M' || last_char == 'm')
    {
        scale = 1024*1024;
    }
    else if(last_char == 'G' || last_char == 'g')
    {
        scale = 1024*1024*1024;
    }
    
    file_size_num = sd_atoi(file_size_str);
    file_size = file_size_num * scale;
    
    sd_free(file_size_str);
    
    return file_size;
}

char* em_parse_ed2k_url(char*url, _u32 url_length)
{
	char name[MAX_FILE_NAME_BUFFER_LEN] = {0};
	_int32 ret_val = SUCCESS;
	char url_tmp[MAX_URL_LEN]={0};
	char *pos1=NULL;
	char *pos2=NULL;
	_u32 name_len = MAX_FILE_NAME_BUFFER_LEN;
	char *emule_url = NULL;
	char utf8_name[MAX_FILE_NAME_BUFFER_LEN]={0};

	if(sd_strncmp(url, "ed2k://%7", sd_strlen("ed2k://%7"))==0)
	{
		sd_strncpy(url_tmp, url, MAX_URL_LEN-1);
		em_replace_7c(url_tmp);
		emule_url = url_tmp;
	}
	else
		emule_url = url;
	/* E-Mule URL */
	pos2=em_strstr(emule_url, "|file|", 0);
	if(pos2!=NULL)
	{
		pos2+=6;
	}
	else
	{
		return NULL;
	}
	
	pos1=em_strchr(pos2, '|',0);
	if(pos1!=NULL&&pos1-pos2>0)
	{
		sd_strncpy(name, pos2,pos1- pos2);
		/* remove all the '%' */
		sd_decode_file_name(name,NULL, sd_strlen(name));

		/* check is Bad file name ! */
		sd_get_valid_name(name,NULL);

		ret_val = sd_any_format_to_utf8(name, sd_strlen(name), utf8_name, &name_len);
		sd_assert(ret_val!=-1);
		if( SUCCESS != ret_val)
			sd_strncpy(name, utf8_name, MAX_FILE_NAME_BUFFER_LEN-1);

		if(em_strlen(name)==0)
			return NULL;
		
		return name;
	}
	else
		return NULL;
}

char * em_get_file_name_from_url( char * url,_u32 url_length)
{
	static char name[MAX_FILE_NAME_BUFFER_LEN];
	_int32 ret_val = SUCCESS;
	char url_tmp[MAX_URL_LEN]={0};
	char *pos1=NULL,*pos2=NULL;
	_u32 name_len = MAX_FILE_NAME_BUFFER_LEN;
	char *emule_name = NULL;

	if((url==NULL)||(url_length<9))
	{	
		return NULL;
	}
	
	em_memset(name, 0, MAX_FILE_NAME_BUFFER_LEN);
	em_memset(url_tmp, 0, MAX_URL_LEN);

	/* Check if the URL is thunder special URL */
	if(em_strncmp((char*)url, "magnet:?", sd_strlen("magnet:?") )== 0)
	{
		/* Decode to normal URL */
		_u8 info_hash[INFO_HASH_LEN]={0};
		if(url_length>=MAX_URL_LEN)
		{
			/* 超长磁力链截断处理 */
			sd_strncpy(url_tmp, url, MAX_URL_LEN-1);
			url = url_tmp;
			pos1 = sd_strrchr(url, '&');
			if(pos1!=NULL) 
			{
				*pos1 = '\0';
			}
		}
		if( em_parse_magnet_url( url ,info_hash,name,NULL)!=SUCCESS)
		{
			return  NULL;
		}
		
		if(em_strlen(name)==0)
			return NULL;

		return name;
	}
	else if(em_strncmp((char*)url, "ed2k://", sd_strlen("ed2k://") )== 0)
	{
		emule_name = em_parse_ed2k_url(url, url_length);
		return emule_name;
	}
	else if(em_strstr((char*)url, "thunder://", 0 )== url)
	{
		/* Decode to normal URL */
		if( em_base64_decode(url+10,url_length-10,url_tmp)!=0)
		{
			return  NULL;
		}

		url_tmp[em_strlen(url_tmp)-2]='\0';
		em_strncpy(url_tmp,url_tmp+2,MAX_URL_LEN-1);
		// 迅雷专用链解析出来之后是电驴链接的处理
		if(em_strncmp((char*)url_tmp, "ed2k://", sd_strlen("ed2k://") )== 0)
		{
			emule_name = em_parse_ed2k_url(url_tmp, sd_strlen(url_tmp));
			return emule_name;
		}
	}
	else
	{
		em_strncpy(url_tmp, url,MAX_URL_LEN-1);
	}
	
	ret_val = sd_get_file_name_from_url(url_tmp,sd_strlen(url_tmp),name, MAX_FILE_NAME_BUFFER_LEN);
	sd_assert(ret_val==SUCCESS);
	if(ret_val==SUCCESS)
	{
		ret_val = sd_any_format_to_utf8(name, sd_strlen(name), url_tmp,&name_len);
		sd_assert(ret_val!=-1);
		if(ret_val>0)
		{
			sd_strncpy(name, url_tmp, MAX_FILE_NAME_BUFFER_LEN-1);
		}
		if(em_strlen(name)==0)
			return NULL;
		
		return name;
	}
	else
	{
		return NULL;
	}
	
}

_int32 em_hex_2_int(char chr)
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

//将所有的带有'%' 的字符去除'%'，包括保留字符'/','?','@',':'...
_int32 em_decode_ex(const char* src_str, char * str_buffer ,_u32 buffer_len)
{
	const char * p_temp=src_str;
	_u8 char_value=0;
	_int32 i=0,count=0,length=em_strlen(src_str);
	
	//LOG_DEBUG( " enter url_object_decode_ex()..." );
	
	if((src_str==NULL)||(str_buffer==NULL)||(buffer_len<length)) return -1; 
		
	em_memset(str_buffer,0,buffer_len);

	while((p_temp[0]!='\0')&&(i<buffer_len))
	{
		if(p_temp[0]!='%')
		{
			str_buffer[i] = p_temp[0];
			p_temp++;
			i++;
		}
		else
		{
			char_value=0;
			if( ( (p_temp+2-src_str)<length )
				&& IS_HEX( p_temp[1] )  
				&& IS_HEX( p_temp[2] )
				)
			{
				char_value = em_hex_2_int(p_temp[1])*16 + em_hex_2_int(p_temp[2]);
				str_buffer[i] = char_value;
				p_temp+=3;
				i++;
				count++;
			}
			else
			{
				str_buffer[i] = p_temp[0];
				p_temp++;
				i++;
			}
			
		}
	}

	//LOG_DEBUG( "\n ---------------------" );
	//LOG_DEBUG( "\n src_str[%d]=%s",length,src_str );
	//LOG_DEBUG( "\n str_buffer[%d]=%s",sd_strlen(str_buffer),str_buffer );
	//LOG_DEBUG( "\n ---------------------" );

	
	return count;
}

BOOL em_decode_file_name(char * _file_name,char * _file_suffix)
{
	char str_buffer[MAX_FILE_NAME_BUFFER_LEN],*pos=NULL,*dot_pos=NULL,*semicolon_pos=NULL;
	//LOG_DEBUG( "sd_decode_file_name" );
	if((_file_name==NULL)||(em_strlen(_file_name)<1))
		return FALSE;

		/* remove all the '%' */
		if( em_decode_ex(_file_name, str_buffer ,MAX_FILE_NAME_BUFFER_LEN)!=-1)
		{
			/* Get the new file name after decode */
			pos=em_strrchr(str_buffer, '/');
			if((pos!=NULL)&&(em_strlen(pos)>2))
			{
				em_strncpy(_file_name, pos, MAX_FILE_NAME_LEN);
				_file_name[em_strlen(pos)]='\0';
			}
			else
			{
				em_strncpy(_file_name, str_buffer, MAX_FILE_NAME_LEN);
				_file_name[em_strlen(str_buffer)]='\0';
			}
			
			/* Check is there any semicolon in the file name */
			semicolon_pos = em_strchr(_file_name, ';', 0);
			if(semicolon_pos!=NULL)
			{
				/* Cut all the chars after semicolon  */
				semicolon_pos[0]='\0';
			}

			/* Check the  file suffix  */
			dot_pos=em_strrchr(_file_name, '.');
			if((dot_pos!=NULL)&&(dot_pos!=_file_name)&&(em_strlen(dot_pos)>2))
			{
				if((_file_suffix!=NULL)&&(0==em_stricmp(dot_pos, _file_suffix)))
				{
					/* file suffix is the same as extend name */
					//LOG_DEBUG( "sd_decode_file_name:file suffix is the same as extend name,_file_name=%s",_file_name );
				}
				else
				if(_file_suffix!=NULL)
				{
					/* the file suffix is different from extend name */
					/* use the suffix as extend name */
					em_strcat(_file_name, _file_suffix, em_strlen(_file_suffix)+1);
					//LOG_DEBUG( "sd_decode_file_name:the file suffix is different from extend name,_file_name=%s",_file_name );
				}
				else
				{
					//LOG_DEBUG( "sd_decode_file_name:_file_name=%s",_file_name );
				}
			}
			else
			if(_file_suffix!=NULL)
			{
				/* use the suffix as extend name */
				em_strcat(_file_name, _file_suffix, em_strlen(_file_suffix)+1);
				//LOG_DEBUG( "sd_decode_file_name:use the suffix as extend name,_file_name=%s",_file_name );
			}
			else
			{
				/* no extend name */
				//LOG_DEBUG( "sd_decode_file_name:no extend name,_file_name=%s",_file_name );
			}
			
			
			return TRUE;
		}
		return FALSE;
}
_int32 em_get_valid_name(char * _file_name,char * file_suffix)
{
	char  chr[2],*p=NULL;
	_int32 i=0;

	chr[0]=_file_name[0];
	chr[1]='\0';

	while(chr[0]!='\0')
	{
		if(em_is_file_name_valid(chr)==FALSE)
		{
			_file_name[i]='_';
		}
		
		chr[0]=_file_name[++i];
	}

	if(file_suffix!=NULL)
	{
		i=0;
		chr[0]=file_suffix[0];

		while(chr[0]!='\0')
		{
			if(em_is_file_name_valid(chr)==FALSE)
			{
				file_suffix[i]='_';
			}
			
			chr[0]=file_suffix[++i];
		}

		i=em_strlen(file_suffix);
		p=em_strrchr(_file_name, '.');
		if((p!=NULL)&&(i>1))
		{
			if(em_stricmp(p, file_suffix)!=0)
			{
				em_strcat(_file_name, file_suffix, i);
			}
		}
		else
		if(i>1)
		{
			em_strcat(_file_name, file_suffix, i);
		}
	}	
	
	if(em_strlen(_file_name)==0)
	{
		//em_strncpy(_file_name,"UNKNOWN_FILE_NAME", em_strlen("UNKNOWN_FILE_NAME"));
		return INVALID_FILE_NAME;
	}
	
	//LOG_DEBUG( "sd_get_valid_name:%s",_file_name );
 	return SUCCESS;
}

_u16 em_get_sum(const char *str, _int32 len)
{
	_int32 i=0;
	_u32 sum = 0;
	
	while(i<len)
	{
		sum+=str[i];
		i++;
	}
	sum&=0x0000FFFF;
   return (_u16)sum;
}
_int32 em_license_encode(const char * license,char * buffer)
{
	_int32 i=0,j=0;
	char buffer_tmp[64],temp[2];
	_u16 crc = em_get_sum(license, LICENSE_LEN);
	_u16 pos = 0;

	for(i=0;i<LICENSE_LEN;i++)
	{
		buffer[i]=license[i+1];
		buffer[i+1]=license[i];
		i++;
	}

	for(i=sizeof(_u16)*2-1;i>=0;i--)
	{
		pos = (crc>>(4*i))&0x000F;
		if(pos!=0)
		{
			em_memset(buffer_tmp, 0, 64);
			em_memcpy(buffer_tmp, buffer+pos, LICENSE_LEN-pos);
			em_memcpy(buffer_tmp+(LICENSE_LEN-pos), buffer, pos);
			em_memcpy(buffer, buffer_tmp, LICENSE_LEN);
		}
	}
	
	em_memset(buffer_tmp, 0, 64);
	em_memcpy(buffer_tmp,buffer,  LICENSE_LEN);
	
	for(i=0,j=LICENSE_LEN/2;i<LICENSE_LEN/2-1;i++,j++)
	{
		em_memcpy(temp, buffer+i, 2);
		em_memcpy(buffer+i, buffer_tmp+j, 2);
		em_memcpy(buffer_tmp+j, temp, 2);
		i+=3;
		j+=3;
	}
	
	em_memcpy(buffer+LICENSE_LEN/2, buffer_tmp+LICENSE_LEN/2, LICENSE_LEN-LICENSE_LEN/2);
	
	return SUCCESS;
}

_int32 em_license_decode(const char * license_encoded,char * license_buffer)
{
	_int32 i=0,j=0;
	char buffer_tmp[64],temp[2];
	_u16 crc = em_get_sum(license_encoded, LICENSE_LEN);
	_u16 pos = 0;
	
	em_memset(buffer_tmp, 0, 64);
	em_memcpy(buffer_tmp, license_encoded, LICENSE_LEN);
	em_memcpy(license_buffer,license_encoded, LICENSE_LEN);
	
	for(i=0,j=LICENSE_LEN/2;i<LICENSE_LEN/2-1;i++,j++)
	{
		em_memcpy(temp, license_buffer+i, 2);
		em_memcpy(license_buffer+i, buffer_tmp+j, 2);
		em_memcpy(buffer_tmp+j, temp, 2);
		i+=3;
		j+=3;
	}
	em_memcpy(license_buffer+LICENSE_LEN/2, buffer_tmp+LICENSE_LEN/2, LICENSE_LEN-LICENSE_LEN/2);
	
	for(i=0;i<sizeof(_u16)*2;i++)
	{
		pos = (crc>>(4*i))&0x000F;
		if(pos!=0)
		{
			em_memset(buffer_tmp, 0, 64);
			em_memcpy(buffer_tmp, license_buffer+(LICENSE_LEN-pos), pos);
			em_memcpy(buffer_tmp+pos, license_buffer, LICENSE_LEN-pos);
			em_memcpy(license_buffer, buffer_tmp, LICENSE_LEN);
		}
	}
	
	em_memset(buffer_tmp, 0, 64);
	em_memcpy(buffer_tmp,license_buffer,  LICENSE_LEN);
	
	for(i=0;i<LICENSE_LEN;i++)
	{
		license_buffer[i]=buffer_tmp[i+1];
		license_buffer[i+1]=buffer_tmp[i];
		i++;
	}

	return SUCCESS;
}


/******************************************************************************************************************************/
/******************************************************************************************************************************/
/******************************************************************************************************************************/

char g_url_website[64]={0};
char g_url[MAX_URL_LEN]={0};
EM_DOWNLOADABLE_URL *g_dl_url = NULL;

static int g_suffixs_size = 0;
static const char** g_file_suffixs = NULL;

#define VIDEO_SUFFIXS_SIZE 18
const static char* g_video_file_suffixs[VIDEO_SUFFIXS_SIZE] =
{
	".xv",".avi",".asf",".mpeg",".mpga", ".mpg",".mp4",".m3u",".mov",".wmv",".rmvb",".rm",".3gp", ".dat",".flv",".mkv", ".ts", /* 视频 */
    ".torrent"									/* 其他 */
};

#define ALL_SUFFIXS_SIZE 50
const static char* g_all_file_suffixs[ALL_SUFFIXS_SIZE] =
{ 
	".chm",".doc",".docx",".pdf",".txt",	".xls",".xlsx",".ppt",	".pptx",				/* 文本 */
	".aac",".ape",".amr",".mp3pro",".mp3",".ogg",".wav",".wma",			/* 音频 */
	".xv",".avi",".asf",".mpeg",".mpga", ".mpg",".mp4",".m3u",".mov",".wmv",".rmvb",".rm",".3gp", ".dat",".flv",".mkv", ".ts", /* 视频 */ 
	".exe", ".msi", ".jar" , ".ipa", ".apk",".img",					/* 应用程序 */
	".rar",  ".zip",   ".iso",  ".bz2", ".tar",".ra",  ".gz" ,".7z",   		/* 压缩文档 */
	 ".bin",".torrent"									/* 其他 */
};

void set_detect_file_suffixs_by_product_id(_int32 product_id)
{
    if (PRODUCT_ID_YUNBO_IPHONE == product_id || PRODUCT_ID_YUNBO_HD == product_id || PRODUCT_ID_ANDROID_YUNBO == product_id)
    {
        g_file_suffixs = g_video_file_suffixs;
        g_suffixs_size = VIDEO_SUFFIXS_SIZE;
    }
    else
    {
        g_file_suffixs = g_all_file_suffixs;
        g_suffixs_size = ALL_SUFFIXS_SIZE;
    }
}
//#ifdef CLOUD_PLAY_PROJ
//#define SUFFIXS_SIZE 17
//const static  char* g_file_suffixs[SUFFIXS_SIZE] =
//{
//	".xv",".avi",".asf",".mpeg",".mpga", ".mpg",".mp4",".m3u",".mov",".wmv",".rmvb",".rm",".3gp", ".dat",".flv",".mkv", /* 视频 */
//    ".torrent"									/* 其他 */
//};
//
//#else/*CLOUD_PLAY_PROJ*/
//
//#define SUFFIXS_SIZE 49
//const static  char* g_file_suffixs[SUFFIXS_SIZE] = 
//{ 
//	".chm",".doc",".docx",".pdf",".txt",	".xls",".xlsx",".ppt",	".pptx",				/* 文本 */
//	".aac",".ape",".amr",".mp3pro",".mp3",".ogg",".wav",".wma",			/* 音频 */
//	".xv",".avi",".asf",".mpeg",".mpga", ".mpg",".mp4",".m3u",".mov",".wmv",".rmvb",".rm",".3gp", ".dat",".flv",".mkv", /* 视频 */ 
//	".exe", ".msi", ".jar" , ".ipa", ".apk",".img",					/* 应用程序 */
//	".rar",  ".zip",   ".iso",  ".bz2", ".tar",".ra",  ".gz" ,".7z",   		/* 压缩文档 */
//	 ".bin",".torrent"									/* 其他 */
//};
//
//#endif/*CLOUD_PLAY_PROJ*/


#define HEADERS_SIZE 5
const static  char* g_url_headers[HEADERS_SIZE] = 
{ 
	"http://",
	"ftp://",
	"thunder://",
	"ed2k://",
	"magnet:?"
};

#define SPECIAL_WEB_NUM 5

const static  char* g_special_web[SPECIAL_WEB_NUM] = 
{ 
	"http://xiazai.xunlei.com/",
	"daquan.xunlei.com/",
	"http://www.ffdy.cc/",
	"http://ishare.iask.sina.com.cn",
	"http://xiazai.zol.com.cn",
};

typedef enum t_em_special_website
{
	ESW_NOT = -1, 			
	ESW_XIAZAI_XUNLEI = 0, 			/* xiazai.xunlei.com */
	ESW_DAQUAN_XUNLEI,  			/* daquan.xunlei.com */
	ESW_FFDY,						/*http://www.ffdy.cc/*/
	ESW_ISHARE_SINA, 				/*http://ishare.iask.sina.com.cn*/
	ESW_ZOL,						/*http://xiazai.zol.com.cn*/
} EM_SPECIAL_WEB;

EM_SPECIAL_WEB em_get_special_website_id(const char * url)
{
	_int32 i = 0;
	char header_buffer[64] = {0};
	EM_SPECIAL_WEB id = ESW_NOT;
    
	sd_strncpy(header_buffer, url, 63);
	for(i=0;i<SPECIAL_WEB_NUM;i++)
	{
		if(sd_strstr(header_buffer, g_special_web[i], 0)!=NULL)
		{
			id = (EM_SPECIAL_WEB)i;
			break;
		}
	}
	return id;
}

#define DEFINITION_NUM 4
const static char* g_movie_definition[DEFINITION_NUM] = 
{
    "320p",
    "480p",
    "720p",
    "1080p"
};
typedef enum t_em_movie_definition
{
	EMD_UNKNOWN = -1, 			
	EMD_320P = 0, 
	EMD_480P,
    EMD_720P,
    EMD_1080P
} EM_MOVIE_DEFINITION;

EM_MOVIE_DEFINITION em_check_movie_definition(const char* definition_str, _int32 length)
{
    char *str_to_check = NULL;
    sd_malloc(length+1, (void**)&str_to_check);
    sd_memset(str_to_check, 0x0, length+1);
    sd_memcpy(str_to_check, definition_str, length);
    sd_string_to_low_case(str_to_check);
    int i = 0;
    for (i=0; i<DEFINITION_NUM; i++) {
        if(sd_strstr(str_to_check, g_movie_definition[i], 0)!=NULL)
        {
            sd_free(str_to_check);
            return i;
        }
    }
    sd_free(str_to_check);
    return EMD_UNKNOWN;
}

EM_FILE_TYPE em_get_file_type(char * file_suffix)
{
	_int32 props = 0;
	EM_FILE_TYPE type;
	
	file_suffix++;
	
	if(0==sd_stricmp(file_suffix, "txt"))
		props = 1;
	else if(0==sd_stricmp(file_suffix, "pdf"))	
		props=1;
	else if(0==sd_stricmp(file_suffix, "doc"))	
		props=1;
	else if(0==sd_stricmp(file_suffix, "chm"))	
		props=1;
	else if(0==sd_stricmp(file_suffix, "docx"))	
		props=1;
	else if(0==sd_stricmp(file_suffix, "xls"))
		props = 1;
	else if(0==sd_stricmp(file_suffix, "xlsx"))
		props = 1;
	else if(0==sd_stricmp(file_suffix, "ppt"))
		props = 1;
	else if(0==sd_stricmp(file_suffix, "pptx"))
		props = 1;
	/*图像格式即图像文件存放的格式，通常有JPEG、TIFF、RAW、BMP、GIF、PNG等。
	else if(0==sd_stricmp(file_suffix, "jpg"))	
		props=2;
	else if(0==sd_stricmp(file_suffix, "bmp"))			
		props=2;
	else if(0==sd_stricmp(file_suffix, "png"))			
		props=2;
	else if(0==sd_stricmp(file_suffix, "gif"))			
		props=2;
	else if(0==sd_stricmp(file_suffix, "jpeg"))			
		props=2;
	else if(0==sd_stricmp(file_suffix, "tiff"))			
		props=2;
	else if(0==sd_stricmp(file_suffix, "raw"))			
		props=2;									*/
	//音频格式日新月异，常用音频格式包括：MP3、WAV、WMA、AIFF、MIDI、AMR、VQF、ASF、AAC。
	else if(0==sd_stricmp(file_suffix, "mp3"))	
		props=3;
	else if(0==sd_stricmp(file_suffix, "wav"))	
		props=3;
	else if(0==sd_stricmp(file_suffix, "wma"))	
		props=3;
	else if(0==sd_stricmp(file_suffix, "amr"))	
		props=3;
	else if(0==sd_stricmp(file_suffix, "ogg"))	
		props=3;
	else if(0==sd_stricmp(file_suffix, "ape"))	
		props=3;
	else if(0==sd_stricmp(file_suffix, "aac"))	
		props=3;
	//视频格式：RM、RMVB、AVI、MPG 、MPEG、FLV、3GP、MP4、SWF、MKV、MOV
	else if(0==sd_stricmp(file_suffix, "xv"))
		props=4;
	else if(0==sd_stricmp(file_suffix, "avi"))
		props=4;
	else if(0==sd_stricmp(file_suffix, "rmvb"))
		props=4;
	else if(0==sd_stricmp(file_suffix, "rm"))
		props=4;
	else if(0==sd_stricmp(file_suffix, "mpg"))
		props=4;
	else if(0==sd_stricmp(file_suffix, "mpeg"))
		props=4;
	else if(0==sd_stricmp(file_suffix, "flv"))
		props=4;
	else if(0==sd_stricmp(file_suffix, "3gp"))
		props=4;
	else if(0==sd_stricmp(file_suffix, "mp4"))
		props=4;
	else if(0==sd_stricmp(file_suffix, "mpga"))
		props=4;
	else if(0==sd_stricmp(file_suffix, "mkv"))
		props=4;
	else if(0==sd_stricmp(file_suffix, "mov"))
		props=4;
	else if(0==sd_stricmp(file_suffix, "asf"))
		props=4;
	else if(0==sd_stricmp(file_suffix, "m3u"))
		props=4;
	else if(0==sd_stricmp(file_suffix, "wmv"))
		props=4;
	else if(0==sd_stricmp(file_suffix, "dat"))
		props=4;
	//应用程序:exe，apk，ipa
	else if(0==sd_stricmp(file_suffix, "exe"))
		props=5;
	else if(0==sd_stricmp(file_suffix, "msi"))
		props=5;
	else if(0==sd_stricmp(file_suffix, "jar"))
		props=5;
	else if(0==sd_stricmp(file_suffix, "ipa"))
		props=5;
	else if(0==sd_stricmp(file_suffix, "apk"))
		props=5;
	else if(0==sd_stricmp(file_suffix, "img"))
		props=5;
	//压缩文件:.rar,  .zip,   .bz2, .tar,.ra,  .gz
	else if(0==sd_stricmp(file_suffix, "rar"))
		props=6;
	else if(0==sd_stricmp(file_suffix, "zip"))
		props=6;
	else if(0==sd_stricmp(file_suffix, "bz2"))
		props=6;
	else if(0==sd_stricmp(file_suffix, "tar"))
		props=6;
	else if(0==sd_stricmp(file_suffix, "ra"))
		props=6;
	else if(0==sd_stricmp(file_suffix, "gz"))
		props=6;
	else if(0==sd_stricmp(file_suffix, "7z"))
		props=6;
	//未知文件类型
	else
		props=0;

	switch(props)
	{
		case 1:
			type= EFT_TXT;
			break;
		case 2:
			type= EFT_PIC;
			break;
		case 3:
			type= EFT_AUDIO;
			break;
		case 4:
			type= EFT_VEDIO;
			break;
		case 5:
			type= EFT_APP;
			break;
		case 6:
			type= EFT_ZIP;
			break;
		default:
			type= EFT_UNKNOWN;
	}
	return type;
}

BOOL em_is_downloadable_url( char * url,EM_URL_TYPE url_type)
{
	  _int32 i=0;
	char *point_pos = NULL,*ask_pos = NULL;

	if(url_type!=UT_HTTP) return TRUE;// 只对http类型的下载链接进行判断，若其他类型，直接返回TRUE

	point_pos = sd_strrchr(url, '.');
	if(point_pos==NULL) return FALSE;// 如果不含有"."，则不可下载
	
	sd_string_to_low_case(point_pos);// 将扩展名改为小写

	if(0==sd_strncmp(url, "http://www.txzqw.com/job-htm-action-download-",sd_strlen("http://www.txzqw.com/job-htm-action-download-")))
	{
		/* 天下足球种子文件的url */
		return TRUE;
	}
	
	if(sd_strstr(point_pos,"htm",1)!=NULL)
	{
		/*普通网页链接*/
		return FALSE;//若扩展名含有htm，则这只是一个普通链接，返回FALSE
	}

	ask_pos = sd_strchr(url, '?',0);
	if(ask_pos !=NULL&&ask_pos-url>5)
	{
		char ask_buffer[8] = {0};
		sd_strncpy(ask_buffer, ask_pos-5, 5);
		if((sd_strstr(ask_buffer,"htm",0)!=NULL)||(sd_strstr(ask_buffer,"HTM",0)!=NULL))
		{
			/* 动态html文件 */
			return FALSE;
		}
	}
	
	if(NULL!=sd_strstr(url,".sendfile.vip.xunlei.com",0))
	{
		/* 迅雷快传url */
		return TRUE;
	}

	// 判断后缀名，是否所关注的文件
	for(i=0;i<g_suffixs_size;i++)
	{
		if(0==sd_strncmp(point_pos, g_file_suffixs[i],sd_strlen(g_file_suffixs[i])))
		{
			char end_char = *(point_pos+sd_strlen(g_file_suffixs[i]));
			if(end_char=='\0'||end_char=='?'||end_char=='/')
			{
				return TRUE;
			}
			else
			{
				return FALSE;
			}
		}
	}

	return FALSE;
}

EM_URL_TYPE em_find_url_type(const char* url)
{
	EM_URL_TYPE url_type;
	char header[16] = {0};

	sd_strncpy(header, url, 15);
	sd_string_to_low_case(header);
	
	if(sd_strncmp(header, "http://", sd_strlen("http://"))==0)
	{
		url_type = UT_HTTP;
	}else
	if(sd_strncmp(header, "ftp://", sd_strlen("ftp://"))==0)
	{
		url_type = UT_FTP;
	}else
	if(sd_strncmp(header, "thunder://", sd_strlen("thunder://"))==0)
	{
		url_type = UT_THUNDER;
	}else
	if(sd_strncmp(header, "ed2k://", sd_strlen("ed2k://"))==0)
	{
		url_type = UT_EMULE;
	}else
	if(sd_strncmp(header, "magnet:?", sd_strlen("magnet:?"))==0)
	{
		url_type = UT_MAGNET;
	}else
	{
		/* 未知类型，暂时用BT 类型标识 */
		url_type = UT_BT;
	}
	
	return url_type;
}

// 获取url的类型，如果是不可下载的url，则返回-1
EM_URL_TYPE em_get_url_type(const char* url)
{
	EM_URL_TYPE url_type;
	char header[16] = {0};
	_int32 header_index = 0, suffix_index = 0;
	char *point_pos = NULL;
	char suffix[16] = {0};
    
	/*****************************************************/
	/*找url 头: 判断是否为所关注的服务类型*/
	/*****************************************************/
    
	sd_strncpy(header, url, 15);
	sd_string_to_low_case(header);
    
	// 判断是否为所关注的服务类型
	for(header_index = 0; header_index < HEADERS_SIZE; header_index++)
	{
		char *care_header = (char*)(g_url_headers[header_index]);
		if(sd_strncmp(header, care_header, sd_strlen(care_header))==0)
			break;// 找到关注的url 头，即可直接退出循环
	}
	
	// 根据服务类型，初步判断url 类型
	switch (header_index)
	{
		case 0:
			url_type = UT_HTTP;
			break;
		case 1:
			url_type = UT_FTP;
			break;
		case 2:
			url_type = UT_THUNDER;
			break;
		case 3:
			url_type = UT_EMULE;
			break;
		case 4:
			url_type = UT_MAGNET;
			break;
		case 5:
			url_type = UT_NORMAL;
			break;
		default:
			url_type = UT_NORMAL;
	}
    
	// 除此两种类型外，其他服务类型均可判定为可以下载，故直接返回
	if(url_type != UT_HTTP && url_type != UT_NORMAL)	return url_type;
    
	if (UT_HTTP == url_type)
	{
		if(0==sd_strncmp(url, "http://gdl.lixian.vip.xunlei.com/",sd_strlen("http://gdl.lixian.vip.xunlei.com/")))
		{
			/* 迅雷离线资源属于正常可下载链接 */
			return UT_HTTP;
		}
		
		if((NULL!=sd_strstr(url,".sendfile.vip.xunlei.com",0))||((NULL!=sd_strstr(url,"&fid=",0))&&(NULL!=sd_strstr(url,"&threshold=",0))&&(NULL!=sd_strstr(url,"&tid=",0))))
		{
			/******************/
			/* 迅雷快传url */
			/******************/
			return UT_HTTP;//http类型，如果按照UT_THUNDER类型来解析会出错
		}
	
		if(0==sd_strncmp(url, "http://www.txzqw.com/job-htm-action-download-",sd_strlen("http://www.txzqw.com/job-htm-action-download-")))
		{
			/* 天下足球种子文件的url */
			return UT_BT;
		}
	}
    
	/**************************************************/
	/*找url 尾: 判断是否所关注的文件类型*/
	/**************************************************/
    
	// 如果不含有"."，则不可下载
	point_pos = sd_strrchr(url, '.');
	if(point_pos==NULL) 	return UT_NORMAL;
    
	sd_strncpy(suffix, point_pos, 15);
	sd_string_to_low_case(suffix);

	// 判断后缀名，是否所关注的文件
	for(suffix_index = 0;suffix_index<g_suffixs_size;suffix_index++)
	{
		char *care_suffix = (char*)(g_file_suffixs[suffix_index]);
		if(0==sd_strncmp(suffix, care_suffix,sd_strlen(care_suffix)))
		{
			char end_char = *(point_pos+sd_strlen(care_suffix));
			if(end_char=='\0'||end_char=='?'||end_char=='/')
			{
                break;
			}
			else
			{
				return UT_NORMAL;
			}
		}
	}
    
	// 根据后缀类型，最终区分bt 链接
	if (suffix_index <g_suffixs_size - 1)
	{
		if(UT_NORMAL == url_type)
			url_type = UT_HTTP_PARTIAL;
		else
			url_type = UT_HTTP;
	}
	else if (suffix_index == g_suffixs_size -1)
	{
		if(UT_NORMAL == url_type)
			url_type = UT_BT_PARTIAL;
		else
			url_type = UT_BT;
	}
	else
	{
		url_type = UT_NORMAL;
	}
    
	return url_type;
    
}

char * em_joint_url(const char *website_url, const char * detected_url)
{
	if(sd_strlen(website_url)+sd_strlen(detected_url) >= MAX_URL_LEN) return NULL;// 超出长度，失败
	
	char whole_url[MAX_URL_LEN] = {0};

	// 找到的URL以'/'开头，则直接拼接在域名后面即可。
	if(*detected_url == '/')  
	{
		sd_snprintf(whole_url, MAX_URL_LEN-1, "%s%s", (char*)g_url_website, detected_url);
		return whole_url;
	}

	// 否则拼接在当前路径下
	char *p_end = sd_strrchr(website_url, '/');

	sd_memcpy(whole_url, website_url, p_end-website_url+1);// 包含'/'

	sd_strcat(whole_url, detected_url, sd_strlen(detected_url));

	return whole_url;
}

BOOL em_is_url_downloadable( const char * url)
{
	EM_URL_TYPE url_type;
//	char url_buffer[MAX_URL_LEN] = {0};
	
	if(sd_strlen(url)<9) return FALSE;
	
	url_type = em_get_url_type(url);
	
/*	if(url_type==UT_BT)
		return FALSE;
	else
	if(url_type!=UT_HTTP) 
		return TRUE;
	
	sd_strncpy(url_buffer, url, MAX_URL_LEN-1);
	
	return em_is_downloadable_url( url_buffer,url_type);*/

	if(url_type == UT_NORMAL)
		return FALSE;
	else
		return TRUE;
}

BOOL em_is_legal_url( char * url,EM_URL_TYPE url_type)
{

	if(sd_strlen(url)<=10) return FALSE;
	
	if(NULL!=sd_strstr(url,g_url_headers[url_type],sd_strlen(g_url_headers[url_type]))) 
		return FALSE;

	return TRUE;
}

_int32 em_url_key_comp(void *E1, void *E2)
{
	_u32 id1 = (_u32)E1,id2 = (_u32)E2;
	
	if(id1==id2)
		return 0;
	else
	if(id1>id2)
		return 1;
	else
		return -1;
}
_int32 em_add_url_to_map(MAP * p_url_map,_u32 hash_value,EM_DOWNLOADABLE_URL * p_dl_url)
{
	_int32 ret_val = SUCCESS;
	PAIR info_map_node;

	info_map_node._key = (void*)hash_value;
	info_map_node._value = (void*)p_dl_url;
	ret_val = em_map_insert_node( p_url_map, &info_map_node );
	//CHECK_VALUE(ret_val);
	return ret_val;
}

char * em_get_minimum_pos(char * pos[7])
{
	_int32 i = 0;
	char * min_pos = (char *)(~0);
	for(i=0;i<7;i++)
	{
		if(pos[i]!=NULL&&pos[i]<min_pos)
		{
			min_pos = pos[i];
		}
	}

	if(min_pos==((char *)(~0)))
		min_pos = NULL;

	return min_pos;
}
char * em_get_url_end_pos(const char* in_str,EM_URL_TYPE url_type)
{
	char *pos1 = NULL; 	/* ' 的位置 */
	char *pos2 = NULL;	/* " 的位置 */
	char *pos3 = NULL;	/* < 的位置 */
	char *pos4 = NULL;	/* > 的位置 */
	char *pos5 = NULL;	/* 空格 的位置 */	
	char *pos6 = NULL;	/* ) 的位置 */	
	char *pos7 = NULL;	/* #的位置 */	
	char *pos_end = NULL;

	char url_buffer[MAX_URL_LEN*2] = {0};

	sd_strncpy(url_buffer, in_str, MAX_URL_LEN*2-1);
	
	pos1 = sd_strchr(url_buffer,'\'', 0);
	pos2 = sd_strchr(url_buffer,'\"', 0);
	pos3 = sd_strchr(url_buffer,'<', 0);
	pos4 = sd_strchr(url_buffer,'>', 0);
	pos5 = sd_strchr(url_buffer,' ', 0);
	pos6 = sd_strchr(url_buffer,')', 0);
	if(pos6!=NULL)
	{
		char *pos_t =sd_strchr(url_buffer,'(', 0);
		if(pos_t!=NULL&& pos_t<pos6) pos6=NULL;
	}
	pos7 = sd_strchr(url_buffer,'#', 0);

	if(pos1!=NULL||pos2!=NULL||pos3!=NULL||pos4!=NULL||pos5!=NULL||pos6!=NULL||pos7!=NULL)
	{
		char * pos_array[7] = {pos1,pos2,pos3,pos4,pos5,pos6,pos7};
		
		pos_end = em_get_minimum_pos(pos_array);
	}
	
	if(pos_end==NULL) 
	{
		if(url_type==UT_MAGNET)
		{
			/* 磁力链太长的话只截取一段 */
			url_buffer[MAX_URL_LEN-5] = '\0';
			pos_end = sd_strrchr(url_buffer,'&');
			if(pos_end==NULL) return NULL;
		}
		else
		if(url_type==UT_EMULE)
		{
			/*电驴链接太长的话把片段哈希值去掉*/
			url_buffer[MAX_URL_LEN-5] = '\0';
			pos_end = sd_strstr(url_buffer,"|p=",0);
			if(pos_end==NULL) return NULL;
			pos_end++;
			*pos_end = '\0';
		}
		else
		{
			return NULL;
		}
	}
	
	return (char*)(in_str+(pos_end-url_buffer));
}

#define  em_parse_file_name_and_hash_value_from_url_new em_parse_file_name_and_hash_value_from_url
#if 0 
{
	_int32 ret_val = SUCCESS;
	char url_buffer[MAX_URL_LEN] = {0};
	char name_buffer[MAX_FILE_NAME_BUFFER_LEN]={0};
	_u32 name_len = MAX_FILE_NAME_LEN-1;
	char *point_pos = NULL;
    char *p_url = url;

	p_dl_url->_type = url_type;

	switch(url_type)
	{
		case UT_HTTP:
		case UT_FTP:
		case UT_BT:
			{
				char utf8_url_buffer[MAX_URL_LEN] = {0};
				_u32 utf8_url_len = MAX_URL_LEN-1;
				/* 首先将URL 统一转换成UTF8格式 */
				ret_val = sd_any_format_to_utf8(p_url, sd_strlen(p_url), utf8_url_buffer,&utf8_url_len);
				sd_assert(ret_val!=-1);
				if(ret_val!=-1)
				{
					/* 转换成功 */
					if(url_type==UT_THUNDER)
					{
						/* 迅雷专用链 */
						p_url = utf8_url_buffer;
					}
					else
					{
						/* 直接把原来的URL替换成UTF8格式的URL */
						sd_strncpy(p_url,utf8_url_buffer,MAX_URL_LEN-1);
					}
				}
				else
				{
					/* 转换失败,直接用原来的URL */
				}

				// 从url中解析出文件名
				ret_val = sd_get_file_name_from_url(p_url,sd_strlen(p_url),name_buffer);
				//sd_assert(ret_val==SUCCESS);
				if(ret_val==SUCCESS)
				{
					ret_val = sd_any_format_to_utf8(name_buffer, sd_strlen(name_buffer), p_dl_url->_name,&name_len);
					sd_assert(ret_val!=-1);
					if(ret_val==-1)
					{
						sd_snprintf(p_dl_url->_name, MAX_FILE_NAME_BUFFER_LEN-1, "Unknown file name");
					}
					point_pos = sd_strrchr(p_dl_url->_name, '.');
					if(point_pos!=NULL)
					{
						p_dl_url->_file_type = em_get_file_type(point_pos);
						if(0==sd_stricmp(point_pos, ".torrent"))
						{
							/* 种子文件的url */
							p_dl_url->_type = UT_BT;
						}
					}
					
					ret_val = em_get_url_hash_value( p_url,sd_strlen(p_url), p_hash_value );
					sd_assert(ret_val==SUCCESS);
					p_dl_url->_url_id = *p_hash_value;
				}
			}
		break;
		case UT_THUNDER:
			/*对于thunder的下载链接，需要先解密*/
			if ('/' == url[sd_strlen(url)-1])
			{
				url[sd_strlen(url)-1] = '\0';// 如果最后一个字符是/,需要去掉该字符 
			}
		
			/* Decode to normal URL : base64解密*/
			if( sd_base64_decode(url+10,(_u8 *)url_buffer,NULL)!=0)
			{
				CHECK_VALUE(INVALID_URL);
			}

			url_buffer[sd_strlen(url_buffer)-2]='\0';
			p_url = url_buffer+2;

			ret_val = sd_get_file_name_from_url(p_url,sd_strlen(p_url),name_buffer);
			//sd_assert(ret_val==SUCCESS);
			if(ret_val==SUCCESS)
			{
				ret_val = sd_any_format_to_utf8(name_buffer, sd_strlen(name_buffer), p_dl_url->_name, &name_len);
				sd_assert(ret_val!=-1);
				if(ret_val==-1)
				{
					sd_snprintf(p_dl_url->_name, MAX_FILE_NAME_BUFFER_LEN-1, "Unknown file name");
				}
				point_pos = sd_strrchr(p_dl_url->_name, '.');
				if(point_pos!=NULL)
				{
					p_dl_url->_file_type = em_get_file_type(point_pos);
				}
				
			}
			
			ret_val = em_get_url_hash_value( p_url,sd_strlen(p_url), p_hash_value );
			sd_assert(ret_val==SUCCESS);
			p_dl_url->_url_id = *p_hash_value;
		break;
		case UT_EMULE:
			{
				ET_ED2K_LINK_INFO info;
				ret_val = eti_extract_ed2k_url(p_url, &info);
				//sd_assert(ret_val==SUCCESS);
				if(ret_val==SUCCESS)
				{
					sd_strncpy(name_buffer,info._file_name,sd_strlen(info._file_name));
					
					/* remove all the '%' */
					sd_decode_file_name(name_buffer,NULL);

					/* Check if Bad file name ! */
					sd_get_valid_name(name_buffer,NULL);
					
					ret_val = sd_any_format_to_utf8(name_buffer, sd_strlen(name_buffer), p_dl_url->_name,&name_len);
					sd_assert(ret_val!=-1);
					if(ret_val==-1)
					{
						sd_snprintf(p_dl_url->_name, MAX_FILE_NAME_BUFFER_LEN-1, "Unknown file name");
					}
					
					point_pos = sd_strrchr(p_dl_url->_name, '.');
					if(point_pos!=NULL)
					{
						p_dl_url->_file_type = em_get_file_type(point_pos);
					}
					
					p_dl_url->_file_size = info._file_size;
					
					ret_val = em_get_url_hash_value( (char*)(info._file_id),16, p_hash_value );
					sd_assert(ret_val==SUCCESS);
					
                    ret_val = em_get_url_hash_value( p_url,sd_strlen(p_url), &p_dl_url->_url_id );
                    sd_assert(ret_val==SUCCESS);
				}
			}
		break;
		case UT_MAGNET:
		{
			_u8 info_hash[INFO_HASH_LEN]={0};
			/* 只嗅探用BT infohash 做urn 的磁力链 */
			ret_val = em_parse_magnet_url(p_url ,info_hash,p_dl_url->_name,&p_dl_url->_file_size);
			if(ret_val!=SUCCESS) return ret_val;
			
			point_pos = sd_strrchr(p_dl_url->_name, '.');
			if(point_pos!=NULL)
			{
				p_dl_url->_file_type = em_get_file_type(point_pos);
			}

			str2hex((const char*)info_hash, INFO_HASH_LEN, url_buffer, 40);	
			ret_val = em_get_url_hash_value(url_buffer,40, p_hash_value );
			sd_assert(ret_val==SUCCESS);
			ret_val = em_get_url_hash_value( p_url,sd_strlen(p_url), &p_dl_url->_url_id );
			sd_assert(ret_val==SUCCESS);
		}
		break;
		default:
			CHECK_VALUE(INVALID_URL);
	}
	return ret_val;
}
#endif

_int32 em_parse_file_name_and_hash_value_from_url( char * url,EM_URL_TYPE url_type,EM_DOWNLOADABLE_URL * p_dl_url,_u32 * p_hash_value)
{
	_int32 ret_val = SUCCESS;
	char url_buffer[MAX_URL_LEN] = {0};
	char name_buffer[MAX_FILE_NAME_BUFFER_LEN]={0};
	_u32 name_len = MAX_FILE_NAME_LEN-1;
	char *p_url = NULL,*point_pos = NULL;

	p_dl_url->_type = url_type;

	/*对于thunder的下载链接，需要先解密*/
	if(url_type==UT_THUNDER)
	{
		/* 如果最后一个字符是/,需要去掉该字符 */
		if ('/' == url[sd_strlen(url)-1])
		{
			url[sd_strlen(url)-1] = '\0';
		}
		
		/* Decode to normal URL */
		if( sd_base64_decode(url+10,(_u8 *)url_buffer,NULL)!=0)
		{
			CHECK_VALUE(INVALID_URL);
		}

		url_buffer[sd_strlen(url_buffer)-2]='\0';
		p_url = url_buffer+2;
		if(0==sd_strncmp(p_url, "ed2k://", sd_strlen("ed2k://")))
		{
			/* 电驴链接*/
			p_dl_url->_type = UT_EMULE;
			url_type = UT_EMULE;
			sd_strncpy(p_dl_url->_url,p_url,MAX_URL_LEN-1);
			if((sd_strncmp(p_dl_url->_url, "ed2k://%7", sd_strlen("ed2k://%7"))==0))
			{
				em_replace_7c(p_dl_url->_url);// 对于电驴链接，将其中的"%7c"换成"|"
			}
			p_url = p_dl_url->_url;
		}
	}
	else
	{
		p_url = url;
	}
	
	switch(url_type)
	{
		case UT_HTTP:
		case UT_FTP:
		case UT_THUNDER:
		case UT_BT:
			{
				char utf8_url_buffer[MAX_URL_LEN] = {0};
				_u32 utf8_url_len = MAX_URL_LEN-1;
				/* 首先将URL 统一转换成UTF8格式 */
				ret_val = sd_any_format_to_utf8(p_url, sd_strlen(p_url), utf8_url_buffer,&utf8_url_len);
				sd_assert(ret_val!=-1);
				if(ret_val!=-1)
				{
					/* 转换成功 */
					if(url_type==UT_THUNDER)
					{
						/* 迅雷专用链 */
						p_url = utf8_url_buffer;
					}
					else
					{
						/* 直接把原来的URL替换成UTF8格式的URL */
						sd_strncpy(p_url,utf8_url_buffer,MAX_URL_LEN-1);
					}
				}
				else
				{
					/* 转换失败,直接用原来的URL */
				}

				ret_val = sd_get_file_name_from_url(p_url,sd_strlen(p_url),name_buffer, MAX_FILE_NAME_BUFFER_LEN);
				//sd_assert(ret_val==SUCCESS);
				if(ret_val==SUCCESS)
				{
					ret_val = sd_any_format_to_utf8(name_buffer, sd_strlen(name_buffer), p_dl_url->_name,&name_len);
					sd_assert(ret_val!=-1);
					if(ret_val==-1)
					{
						sd_snprintf(p_dl_url->_name, MAX_FILE_NAME_BUFFER_LEN-1, "Unknown file name");
					}
					point_pos = sd_strrchr(p_dl_url->_name, '.');
					if(point_pos!=NULL)
					{
						p_dl_url->_file_type = em_get_file_type(point_pos);
						if(0==sd_stricmp(point_pos, ".torrent"))
						{
							/* 种子文件的url */
							p_dl_url->_type = UT_BT;
						}
						
						if(0==sd_strncmp(p_url, "http://www.txzqw.com/job-htm-action-download-",sd_strlen("http://www.txzqw.com/job-htm-action-download-")))
						{
							_u32 time_stmp = 0;
							/* 天下足球种子文件的url */
							p_dl_url->_type = UT_BT;
							sd_time_ms(&time_stmp);
							sd_snprintf(point_pos, MAX_FILE_NAME_BUFFER_LEN-sd_strlen(p_dl_url->_name), "_%X.torrent",time_stmp);
							EM_LOG_DEBUG("Get a txzq torrent url:%s,name=%s",p_dl_url->_url,p_dl_url->_name);
						}
					}
					
					ret_val = em_get_url_hash_value( p_url,sd_strlen(p_url), p_hash_value );
					sd_assert(ret_val==SUCCESS);
					p_dl_url->_url_id = *p_hash_value;
				}
			}
		break;
		case UT_EMULE:
			{
				ET_ED2K_LINK_INFO info;
				ret_val = eti_extract_ed2k_url(p_url, &info);
				//sd_assert(ret_val==SUCCESS);
				if(ret_val==SUCCESS)
				{
					sd_strncpy(name_buffer,info._file_name,sd_strlen(info._file_name));
					
					/* remove all the '%' */
					sd_decode_file_name(name_buffer,NULL, sd_strlen(name_buffer));

					/* Check if Bad file name ! */
					sd_get_valid_name(name_buffer,NULL);
					
					ret_val = sd_any_format_to_utf8(name_buffer, sd_strlen(name_buffer), p_dl_url->_name,&name_len);
					sd_assert(ret_val!=-1);
					if(ret_val==-1)
					{
						sd_snprintf(p_dl_url->_name, MAX_FILE_NAME_BUFFER_LEN-1, "Unknown file name");
					}
					
					point_pos = sd_strrchr(p_dl_url->_name, '.');
					if(point_pos!=NULL)
					{
						p_dl_url->_file_type = em_get_file_type(point_pos);
					}
					
					p_dl_url->_file_size = info._file_size;
					
					ret_val = em_get_url_hash_value( (char*)(info._file_id),16, p_hash_value );
					sd_assert(ret_val==SUCCESS);
					
					ret_val = em_get_url_hash_value( p_url,sd_strlen(p_url), &p_dl_url->_url_id );
					sd_assert(ret_val==SUCCESS);
				}
			}
		break;
		case UT_MAGNET:
		{
			_u8 info_hash[INFO_HASH_LEN]={0};
			/* 只嗅探用BT infohash 做urn 的磁力链 */
			ret_val = em_parse_magnet_url(p_url ,info_hash,p_dl_url->_name,&p_dl_url->_file_size);
			if(ret_val!=SUCCESS) return ret_val;
			
			point_pos = sd_strrchr(p_dl_url->_name, '.');
			if(point_pos!=NULL)
			{
				p_dl_url->_file_type = em_get_file_type(point_pos);
			}

			str2hex((const char*)info_hash, INFO_HASH_LEN, url_buffer, 40);	
			ret_val = em_get_url_hash_value(url_buffer,40, p_hash_value );
			sd_assert(ret_val==SUCCESS);
			ret_val = em_get_url_hash_value( p_url,sd_strlen(p_url), &p_dl_url->_url_id );
			sd_assert(ret_val==SUCCESS);
		}
		break;
		default:
			CHECK_VALUE(INVALID_URL);
	}
	return ret_val;
}
/* 获取<a href="#/URL">name</a> 里面的资源文件名 */
_int32 em_get_href_file_name(char * search_start_pos,char *name_buffer)
{
	_int32 ret_val = SUCCESS;
	char file_suffix[16]={0};
	char *title_pos = NULL,*point_pos=NULL,*mark_pos1=NULL,*mark_pos2=NULL;
	char *title_end_pos = sd_strstr(search_start_pos,"</a>",0);
	char str_buffer[MAX_FILE_NAME_BUFFER_LEN] = {0};
	_u32 buffer_len = MAX_FILE_NAME_BUFFER_LEN;

	if(title_end_pos==NULL||title_end_pos-search_start_pos>MAX_FILE_NAME_BUFFER_LEN)
	{
		return -1;
	}

	mark_pos1 = sd_strchr(search_start_pos,'>',0);
	if(mark_pos1==NULL||mark_pos1==title_end_pos+3)
	{
		return -1;
	}
	
	mark_pos2 = sd_strchr(mark_pos1,'>',1);
	if(mark_pos2==NULL||mark_pos2!=title_end_pos+3)
	{
		return -1;
	}
	
	title_pos = mark_pos1+1;
	
	if((title_end_pos>=title_pos+1)&&(title_end_pos-title_pos<MAX_FILE_NAME_BUFFER_LEN-16))
	{
		/* 找到文件名 */
		point_pos = sd_strrchr(name_buffer, '.');
		if(point_pos!=NULL &&(sd_strlen(point_pos)<15))
		{
			sd_strncpy(file_suffix, point_pos, 15);
			sd_memset(name_buffer,0x00,MAX_FILE_NAME_BUFFER_LEN);
			sd_strncpy(name_buffer, title_pos, title_end_pos-title_pos);
			
			/* remove all the '%' */
			sd_decode_file_name(name_buffer,NULL, sd_strlen(name_buffer));

			/* Check if Bad file name ! */
			sd_get_valid_name(name_buffer,NULL);
			
			ret_val = sd_any_format_to_utf8(name_buffer, sd_strlen(name_buffer), str_buffer,&buffer_len);
			sd_assert(ret_val!=-1);
			if(ret_val>0)
			{
				sd_strncpy(name_buffer, str_buffer, MAX_FILE_NAME_BUFFER_LEN-1);
			}

			if(sd_strcmp(name_buffer, "download")==0
				||sd_strcmp(name_buffer, "点击")==0
				||sd_strcmp(name_buffer, "下载")==0
				||(sd_strlen(name_buffer)>6&&sd_strcmp(name_buffer+sd_strlen(name_buffer)-6, "下载")==0))
			{
				return -1;
			}
			
			point_pos = sd_strrchr(name_buffer, '.');
			if(sd_strncmp(point_pos, file_suffix,sd_strlen(file_suffix))!=0)
			{
				/* 添加文件后缀 */
				sd_strcat(name_buffer, file_suffix, sd_strlen(file_suffix));
			}
			return SUCCESS;
		}
	}
	return -1;
}
/* 获取daquan.xunlei.com 里面的资源文件名 */
char * em_get_xunlei_daquan_file_name_start_pos(char * str)
{
	char * title_pos = NULL;

	/* 循环找到name= 后面的就是文件名 */
	do
	{
		title_pos = sd_strrchr(str,'\"');// 找到最后一个引号
		if(title_pos==NULL) return NULL;
		*title_pos = '\0';// 将此处置为\0，文件名的结尾
		title_pos = sd_strrchr(str,'\"');// 再找到一个引号
		if(title_pos==NULL||title_pos-str<6) return NULL;
		if(sd_strncmp(title_pos-5, "name=", 5)==0)//如果引号之前是name=，说明这个就是文件名
		{
			return title_pos+1;//返回文件名
		}
		*title_pos = '\0';
	}while(title_pos-str>6);//一直循环知道找到name属性
	
	return NULL;
}
_int32 em_get_xunlei_daquan_file_name(char * url_header_pos,char* url_end_pos,char *name_buffer)
{
	_int32 ret_val = SUCCESS;
	char file_suffix[16]={0};
	char *title_end_pos = NULL,*point_pos=NULL,*search_start_pos = url_header_pos-MAX_FILE_NAME_BUFFER_LEN;
	char * title_pos = NULL;
	char str_buffer[MAX_FILE_NAME_BUFFER_LEN] = {0};
	_u32 buffer_len = MAX_FILE_NAME_BUFFER_LEN;
	char *value_pos = url_header_pos-7;
	char *id_search_start_pos = NULL, *id_pos = NULL, *id_end_pos = NULL;

	if(sd_strncmp(value_pos, "value=\"", 7)==0)
	{
		sd_strncpy(str_buffer, search_start_pos,MAX_FILE_NAME_BUFFER_LEN-5);
		title_pos = em_get_xunlei_daquan_file_name_start_pos(str_buffer);
		if(title_pos==NULL||sd_strlen(title_pos)<2) return -1;
		/* 至此已找到文件名 */
		point_pos = sd_strrchr(name_buffer, '.');// 从name_buffer找到扩展名
		if(point_pos!=NULL &&(sd_strlen(point_pos)<15))
		{
			sd_strncpy(file_suffix, point_pos, 15);// 保存扩展名
			sd_memset(name_buffer,0x00,MAX_FILE_NAME_BUFFER_LEN);// 清空name_buffer
			sd_strncpy(name_buffer, title_pos, MAX_FILE_NAME_BUFFER_LEN-1);// 将name= 属性中的标题保存到name_buffer 中
		
			/* remove all the '%' */
			sd_decode_file_name(name_buffer,NULL, sd_strlen(name_buffer));

			/* Check if Bad file name ! */
			sd_get_valid_name(name_buffer,NULL);
			
			sd_memset(str_buffer,0x00,MAX_FILE_NAME_BUFFER_LEN);
			ret_val = sd_any_format_to_utf8(name_buffer, sd_strlen(name_buffer), str_buffer,&buffer_len);// 转成utf-8 格式
			sd_assert(ret_val!=-1);
			if(ret_val>0)
			{
				sd_strncpy(name_buffer, str_buffer, MAX_FILE_NAME_BUFFER_LEN-1);// 保存utf-8 格式的文件名
			}
			point_pos = sd_strrchr(name_buffer, '.');//查找后缀名
			if(point_pos!=NULL)
			{
				sd_memset(point_pos, 0x00, sd_strlen(point_pos)); //去掉原有的后缀名 
			}
			/*查找id属性中的清晰度作为文件名的一部分*/
            sd_memset(str_buffer, 0, MAX_FILE_NAME_BUFFER_LEN);
			sd_strncpy(str_buffer, search_start_pos,MAX_FILE_NAME_BUFFER_LEN-5);
            id_search_start_pos = sd_strrchr(str_buffer, '<');// 找到最右边的< 作为查找id的起点
			if(id_search_start_pos!=NULL)
			{
				id_pos=sd_strstr(id_search_start_pos, "id=\"", 1);// 查找id 属性
				if(id_pos != NULL)
				{
					id_end_pos = sd_strchr(id_pos, '\"', 4);// 
					sd_assert(id_end_pos != NULL);// id 属性应该一定能找到结束引号
                    EM_MOVIE_DEFINITION definition = em_check_movie_definition(id_pos+4, id_end_pos-id_pos-4);
                    if (definition != EMD_UNKNOWN) {
                        sd_strcat(name_buffer, g_movie_definition[definition], sd_strlen(g_movie_definition[definition]));//将清晰度接在文件名后面
                    }
				}
			}
//			if(point_pos==NULL||sd_strncmp(point_pos, file_suffix,sd_strlen(file_suffix))!=0)// 如果没有后缀名或者与最初的后缀名不同
//			{
				/* 添加文件后缀 */
				sd_strcat(name_buffer, file_suffix, sd_strlen(file_suffix));
//			}
			return SUCCESS;
		}
	}
	else
	if(sd_strncmp(url_end_pos, "', '", 4)==0)
	{
		title_pos = url_end_pos+4;
		title_end_pos = sd_strstr(title_pos,"'",0);
		if((title_end_pos!=NULL)&&(title_end_pos-title_pos>=1)&&(title_end_pos-title_pos<MAX_FILE_NAME_BUFFER_LEN-16))
		{
			/* 找到文件名 */
			point_pos = sd_strrchr(name_buffer, '.');
			if(point_pos!=NULL &&(sd_strlen(point_pos)<15))
			{
				sd_strncpy(file_suffix, point_pos, 15);
				sd_memset(name_buffer,0x00,MAX_FILE_NAME_BUFFER_LEN);
				sd_strncpy(name_buffer, title_pos, title_end_pos-title_pos);
			
				/* remove all the '%' */
				sd_decode_file_name(name_buffer,NULL, sd_strlen(name_buffer));

				/* Check if Bad file name ! */
				sd_get_valid_name(name_buffer,NULL);
				
				ret_val = sd_any_format_to_utf8(name_buffer, sd_strlen(name_buffer), str_buffer,&buffer_len);
				sd_assert(ret_val!=-1);
				if(ret_val>0)
				{
					sd_strncpy(name_buffer, str_buffer, MAX_FILE_NAME_BUFFER_LEN-1);
				}
				point_pos = sd_strrchr(name_buffer, '.');
				if(point_pos==NULL||sd_strncmp(point_pos, file_suffix,sd_strlen(file_suffix))!=0)
				{
					/* 添加文件后缀 */
					sd_strcat(name_buffer, file_suffix, sd_strlen(file_suffix));
				}
				return SUCCESS;
			}
		}
	}
	return -1;
}

/* 获取xiazai.xunlei.com 里面的资源文件名 */
_int32 em_get_xunlei_xiazai_file_name(char * search_start_pos,char *name_buffer)
{
	_int32 ret_val = SUCCESS;
	char file_suffix[16]={0};
	char *title_end_pos = NULL,*point_pos=NULL;
	char * title_pos = sd_strstr(search_start_pos,"title=\"",0);
	char str_buffer[MAX_FILE_NAME_BUFFER_LEN] = {0};
	_u32 buffer_len = MAX_FILE_NAME_BUFFER_LEN;

	if((title_pos!=NULL)&&(title_pos-search_start_pos<10))
	{
		title_pos += sd_strlen("title=\"");
		title_end_pos = sd_strstr(title_pos,"\"",0);
		if((title_end_pos!=NULL)&&(title_end_pos-title_pos>=1)&&(title_end_pos-title_pos<MAX_FILE_NAME_BUFFER_LEN-16))
		{
			/* 找到文件名 */
			point_pos = sd_strrchr(name_buffer, '.');
			if(point_pos!=NULL &&(sd_strlen(point_pos)<15))
			{
				sd_strncpy(file_suffix, point_pos, 15);
				sd_memset(name_buffer,0x00,MAX_FILE_NAME_BUFFER_LEN);
				sd_strncpy(name_buffer, title_pos, title_end_pos-title_pos);
			
				/* remove all the '%' */
				sd_decode_file_name(name_buffer,NULL, sd_strlen(name_buffer));

				/* Check if Bad file name ! */
				sd_get_valid_name(name_buffer,NULL);
				
				ret_val = sd_any_format_to_utf8(name_buffer, sd_strlen(name_buffer), str_buffer,&buffer_len);
				sd_assert(ret_val!=-1);
				if(ret_val>0)
				{
					sd_strncpy(name_buffer, str_buffer, MAX_FILE_NAME_BUFFER_LEN-1);
				}
				point_pos = sd_strrchr(name_buffer, '.');
				if(point_pos==NULL||sd_strncmp(point_pos, file_suffix,sd_strlen(file_suffix))!=0)
				{
					/* 添加文件后缀 */
					sd_strcat(name_buffer, file_suffix, sd_strlen(file_suffix));
				}
				return SUCCESS;
			}
		}
	}
	return -1;
}

/* 获取xiazai.xunlei.com 里面的资源文件名 */
_int32 em_get_zol_file_name(const char * webpage_content, char *name_buffer)
{
	_int32 ret_val = SUCCESS;
	char *start_pos = NULL, *end_pos = NULL, *mark_pos = NULL;
	char str_buffer[MAX_FILE_NAME_BUFFER_LEN] = {0};
	_u32 output_len = MAX_FILE_NAME_BUFFER_LEN;

	mark_pos = sd_strstr(webpage_content, "<h1", 0);
	if(mark_pos!=NULL)
	{
		start_pos = sd_strchr(mark_pos, '>', 0);
		start_pos += 1;

		end_pos = sd_strstr(start_pos, "</h1>", 0);
		if((end_pos == NULL)||(end_pos-start_pos>MAX_FILE_NAME_BUFFER_LEN-1)) return -1;

		sd_memcpy(str_buffer, start_pos, end_pos-start_pos);

		ret_val = sd_any_format_to_utf8(str_buffer, end_pos-start_pos, name_buffer, &output_len);
		sd_assert(ret_val!=-1);
	}
	return SUCCESS;
}

/* http://www.114bt.com/ 里面的torrent 文件下载链接需要拼凑出来,做特殊处理 */
_int32 em_get_114bt_resource(const char* webpage_content,MAP * p_url_map )
{
	_int32 ret_val = SUCCESS;
	char * torrent_pos = NULL,*mark_pos = NULL,*start_pos = NULL,*old_start_pos = (char*)webpage_content;
	char url_buffer[MAX_URL_LEN]={0};
	EM_DOWNLOADABLE_URL * p_dl_url = NULL;
	_u32 hash_value=0,header_len = sd_strlen("http://www.114bt.com/");
	
	torrent_pos = sd_strstr(old_start_pos,".torrent\"",0);
	while((torrent_pos!=NULL)&&(torrent_pos-old_start_pos>MAX_URL_LEN-header_len-10))
	{
		start_pos = torrent_pos-(MAX_URL_LEN-header_len-10);
		sd_memcpy(url_buffer, start_pos, (MAX_URL_LEN-header_len-10+sd_strlen(".torrent")));
		mark_pos = sd_strrchr(url_buffer,'\"');
		start_pos = mark_pos+1;
		
		ret_val = sd_malloc(sizeof(EM_DOWNLOADABLE_URL), (void**)&p_dl_url);
		if(ret_val!=SUCCESS) return SUCCESS;
		sd_memset(p_dl_url,0x00,sizeof(EM_DOWNLOADABLE_URL));
		sd_snprintf(p_dl_url->_url, MAX_URL_LEN-1, "http://www.114bt.com%s",start_pos);
		ret_val = em_parse_file_name_and_hash_value_from_url(p_dl_url->_url,UT_HTTP,p_dl_url, &hash_value);
		EM_LOG_DEBUG( "em_get_114bt_resource:ret=%d,type=%s,hash=%u,fsize=%llu,name=%s",ret_val,(char*)(g_url_headers[p_dl_url->_type]),hash_value,p_dl_url->_file_size,p_dl_url->_name);
		if(ret_val ==SUCCESS)
		{
			ret_val = em_add_url_to_map(p_url_map, hash_value, p_dl_url);
			if(ret_val!=SUCCESS) 
			{
					EM_LOG_ERROR( "em_get_url_from_webpage:em_add_url_to_map ERROR!ret_val=%d",ret_val);
				SAFE_DELETE(p_dl_url);
			}
		}
		else
		{
			SAFE_DELETE(p_dl_url);
		}
		
		if(sd_strlen(torrent_pos)<sd_strlen(".torrent\"")+16) return SUCCESS;
			
		old_start_pos = torrent_pos+sd_strlen(".torrent\"")+2;
		torrent_pos = sd_strstr(old_start_pos,".torrent\"",0);
	}
	return SUCCESS;
}

// 在src中找到位于start和end之间的字符串dest，返回值为dest的长度，若查找失败，返回-1
// 可以分别设置是否包含start或end
_int32 em_get_string_between_strings(const char* src, char **dest, const char* start_str, BOOL start_str_include, const char * end_str, BOOL end_str_include)
{
    _int32 length = 0;

    // 查找开始串
    char *start_pos = sd_strstr(src, start_str, 0);
    if (start_pos == NULL) {
        return -1;
    }
    // 从开始串的结尾查找结束串
    char *end_pos = sd_strstr(start_pos, end_str, sd_strlen(start_str));
    if (end_pos == NULL) {
        return -1;
    }

	if(start_str_include)
	{
		// 如果包含开始串
		 *dest = start_pos;
		length = end_pos-start_pos;
	}
	else 
	{
		// 如果不包含开始串，则需要后移开始串长度
		*dest = start_pos + sd_strlen(start_str);
		length = end_pos-start_pos - sd_strlen(start_str);
	}

	if(end_str_include)
	{
		// 如果包含结束串，则需要加上结束串长度
		length += sd_strlen(end_str);
	}

#ifdef _DEBUG
    char *find_buffer = NULL;
    sd_malloc(length+1, (void**)&find_buffer);
    sd_memset(find_buffer, 0, length+1);
    sd_memcpy(find_buffer, *dest, length);
    printf("em_get_string_between_strings result: %s\n", find_buffer);
    SAFE_DELETE(find_buffer);
#endif
    
    return length;
}

// 正则表达式嗅探
_int32 em_detect_regex_match_working(const char* src, const char * regex_string, char* match_buffer)
{
	_int32 ret_val = SUCCESS;

	_int32 cflags = REG_EXTENDED;
	char error_buffer[128] = {0};
	regex_t reg;
	regmatch_t pmatch[2];
	const size_t nmatch = 2;
	const char* p_matched = NULL;
	_int32 length = 0;

	/* 编译正则表达式*/
	ret_val = regcomp(&reg, regex_string, cflags);
    
	if(ret_val != 0)
	{
		regerror(ret_val, &reg, error_buffer, sizeof(error_buffer));
		EM_LOG_DEBUG("regex error:%s (regex:%s)\n", error_buffer, regex_string);
		return ret_val;
	}

	/* 执行正则表达式匹配*/
	ret_val = regexec(&reg, src, nmatch, pmatch, 0);

	/* 保存匹配结果*/
	if(ret_val == 0)
	{
		// 只保留第一个匹配成功的结果
		p_matched = src + pmatch[1].rm_so;
		length = pmatch[1].rm_eo - pmatch[1].rm_so;
		sd_memcpy(match_buffer, p_matched, length);
        
#ifdef _DEBUG
        printf("regex:%s match_result:%s\n", regex_string, match_buffer);  
#endif
	}
	else if (ret_val == REG_NOMATCH)
	{
		sd_memcpy(match_buffer, "\0", 1);
	}
	else
	{
		regerror(ret_val, &reg, error_buffer, sizeof(error_buffer));
		EM_LOG_DEBUG("regex error:%s (regex:%s)\n", error_buffer, regex_string);
	}

	/* 释放正则表达式*/
	regfree(&reg);
    
	return ret_val;
}

_int32 em_detect_regex_infos_from_string(const char* string, EM_DETECT_REGEX * detect_rule, MAP* p_url_map)
{
	_int32 ret_val = SUCCESS;
	char match_buffer[MAX_URL_LEN] = {0};
	EM_DOWNLOADABLE_URL * p_dl_url = NULL;
	_u32 hash_value = 0;

    if(detect_rule->_download_url[0] == '\0' ) return SUCCESS;// 如果没有下载链接regex，则直接返回
        
    // 可下载链接嗅探
    ret_val = em_detect_regex_match_working(string, detect_rule->_download_url, match_buffer);
    if(ret_val != 0) return SUCCESS;// 没有找到可下载链接，直接返回
    
    ret_val = sd_malloc(sizeof(EM_DOWNLOADABLE_URL), (void**)&p_dl_url);
    CHECK_VALUE(ret_val);
    sd_memset(p_dl_url, 0, sizeof(EM_DOWNLOADABLE_URL));

    sd_strncpy(p_dl_url->_url, match_buffer, MAX_URL_LEN - 1);
    
    EM_URL_TYPE url_type = em_get_url_type(p_dl_url->_url);
    if(url_type == UT_NORMAL) 
    {
    	// 如果嗅探错误，得到一个不可下载的链接，则直接舍去
    	SAFE_DELETE(p_dl_url);
    	return SUCCESS;
    }
    
    /*url小处理*/
    if(url_type==UT_EMULE&&(sd_strncmp(p_dl_url->_url, "ed2k://%7", sd_strlen("ed2k://%7"))==0))
    {
        em_replace_7c(p_dl_url->_url);// 对于电驴链接，将其中的"%7c"换成"|"
    }
    else if(url_type == UT_HTTP_PARTIAL || url_type == UT_BT_PARTIAL)
    {
        // 对于不完整 的链接，现将其拼接成完整链接
              char *jointed_url = em_joint_url(g_url, match_buffer);
		sd_snprintf(p_dl_url->_url, MAX_URL_LEN-1, jointed_url);
        // 相应的将链接类型也改成UT_HTTP 和UT_BT
        if (url_type == UT_HTTP_PARTIAL) {
            url_type = UT_HTTP;
        }
        else if (url_type == UT_BT_PARTIAL) {
            url_type = UT_BT;
        }
    }
    
    p_dl_url->_type = url_type;
    
    // 直接从url中解析得到文件名等信息
    ret_val = em_parse_file_name_and_hash_value_from_url_new(p_dl_url->_url,url_type,p_dl_url, &hash_value);// 解析文件名并得到url 哈希值

    sd_memset(match_buffer, 0, MAX_URL_LEN);// 清空match_buffer用于下一个匹配
	
    if(detect_rule->_name[0] != '\0')
    {
        // 文件名嗅探
        ret_val = em_detect_regex_match_working(string, detect_rule->_name, match_buffer);
        if(ret_val == 0) 
        {
            sd_memset(p_dl_url->_name, 0, MAX_FILE_NAME_BUFFER_LEN);
	     sd_strncpy(p_dl_url->_name, match_buffer, MAX_FILE_NAME_BUFFER_LEN - 1);
        }
        sd_memset(match_buffer, 0, MAX_URL_LEN);
    }
    
    // 文件名扩展标识嗅探
    if(detect_rule->_name_append[0] != '\0')
    {
        ret_val = em_detect_regex_match_working(string, detect_rule->_name_append, match_buffer);
        if(ret_val == 0) 
        {
            char name_buffer[MAX_FILE_NAME_BUFFER_LEN] = {0};
            char file_suffix[16] = {0};
            char *point_pos = NULL;
            
            sd_strncpy(name_buffer, p_dl_url->_name, sd_strlen(p_dl_url->_name));
            
            point_pos = sd_strrchr(name_buffer, '.');//查找后缀名
            if(point_pos!=NULL)
            {
                // 将扩展内容插入到后缀名之前
                sd_strncpy(file_suffix, point_pos, 16);
                sd_memset(point_pos, 0x00, sd_strlen(point_pos)); //去掉原有的后缀名 
                sd_strcat(name_buffer, match_buffer, sd_strlen(match_buffer));//将扩展内容接在文件名后面
                sd_strcat(name_buffer, file_suffix, sd_strlen(file_suffix));// 添加文件后缀 
            }
            else
            {
                //将扩展内容接在文件名后面
                sd_strcat(name_buffer, "(", 1);
                sd_strcat(name_buffer, match_buffer, sd_strlen(match_buffer));
                sd_strcat(name_buffer, ")", 1);
            }
            
            sd_strncpy(p_dl_url->_name, name_buffer, MAX_FILE_NAME_BUFFER_LEN -1);
        }
        sd_memset(match_buffer, 0, MAX_URL_LEN);// 清空match_buffer用于下一个匹配
    }
    
    // 扩展名嗅探
    if(detect_rule->_suffix[0] != '\0')
    {
        ret_val = em_detect_regex_match_working(string, detect_rule->_suffix, match_buffer);
        if(ret_val == 0)
        {
            char name_buffer[MAX_FILE_NAME_BUFFER_LEN] = {0};
            char *point_pos = NULL;
            
            sd_strncpy(name_buffer, p_dl_url->_name, MAX_FILE_NAME_BUFFER_LEN -1);
            
            point_pos = sd_strrchr(name_buffer, '.');//查找后缀名
            if(point_pos!=NULL)
            {
                sd_memset(point_pos, 0x00, sd_strlen(point_pos+1)); //去掉原有的后缀名 (不需要保留点. )
            }
            sd_strcat(name_buffer, match_buffer, sd_strlen(match_buffer));//将扩展名接在文件名后面
            
            sd_strncpy(p_dl_url->_name, name_buffer, MAX_FILE_NAME_BUFFER_LEN -1);
            p_dl_url->_file_type = em_get_file_type(match_buffer);
            
        }
        sd_memset(match_buffer, 0, MAX_URL_LEN);// 清空match_buffer用于下一个匹配
    }
    else 
    {
    		// 如果没有扩展名嗅探方式，则使用链接最后携带的扩展名
    		if(url_type == UT_HTTP || url_type == UT_HTTP_PARTIAL)//http任务才能使用url后缀
    		{
    		char * point_pos = NULL;
		point_pos = sd_strrchr(p_dl_url->_url, '.');
		if(point_pos!=NULL)
		{
			p_dl_url->_file_type = em_get_file_type(point_pos);
			sd_strcat(p_dl_url->_name, point_pos, sd_strlen(point_pos));
		}
		}
    }
    
    // 文件大小嗅探
    if(detect_rule->_file_size[0] != '\0')
    {
        ret_val = em_detect_regex_match_working(string, detect_rule->_file_size, match_buffer);
        if(ret_val == 0)
        {
            p_dl_url->_file_size = em_filesize_str_to_u64(match_buffer);
        }
        sd_memset(match_buffer, 0, MAX_URL_LEN);// 清空match_buffer用于下一个匹配
    }
    
    EM_LOG_DEBUG( "em_detect_by_regex_rule:url=%s,type=%s,hash=%u,fsize=%llu,name=%s",p_dl_url->_url,(char*)(g_url_headers[p_dl_url->_type]),hash_value,p_dl_url->_file_size,p_dl_url->_name);

#ifdef CLOUD_PLAY_PROJ
	// 云播项目只添加视频任务
	if(p_dl_url->_file_type != EFT_VEDIO && p_dl_url->_file_type != EFT_UNKNOWN)
    	{
    		return SUCCESS;
    	}
#endif/*CLOUD_PLAY_PROJ*/
    
    // 最后将嗅探结果添加到url_map 中
    ret_val = em_add_url_to_map(p_url_map, hash_value, p_dl_url);
    if(ret_val!=SUCCESS) 
    {
        EM_LOG_ERROR( "em_detect_by_regex_rule:em_add_url_to_map ERROR!ret_val=%d",ret_val);
        SAFE_DELETE(p_dl_url);
    }
    
    return ret_val;

}

_int32 em_detect_regex_infos_from_string_except_name_and_size(const char* string, EM_DETECT_REGEX * detect_rule, const char *name, const _u64 file_size, MAP* p_url_map)
{
	_int32 ret_val = SUCCESS;
	char match_buffer[MAX_URL_LEN] = {0};
	EM_DOWNLOADABLE_URL * p_dl_url = NULL;
	_u32 hash_value = 0;

    if(detect_rule->_download_url[0] == '\0' ) return SUCCESS;// 如果没有下载链接regex，则直接返回
        
    // 可下载链接嗅探
    ret_val = em_detect_regex_match_working(string, detect_rule->_download_url, match_buffer);
    if(ret_val != 0) return SUCCESS;// 没有找到可下载链接，直接返回
    
    ret_val = sd_malloc(sizeof(EM_DOWNLOADABLE_URL), (void**)&p_dl_url);
    CHECK_VALUE(ret_val);
    sd_memset(p_dl_url, 0, sizeof(EM_DOWNLOADABLE_URL));

    sd_strncpy(p_dl_url->_url, match_buffer, MAX_URL_LEN -1);
    
    sd_memset(match_buffer, 0, MAX_URL_LEN);// 清空match_buffer用于下一个匹配
    
    EM_URL_TYPE url_type = em_get_url_type(p_dl_url->_url);
    if(url_type == UT_NORMAL) 
    {
    	// 如果嗅探错误，得到一个不可下载的链接，则直接舍去
    	SAFE_DELETE(p_dl_url);
    	return SUCCESS;
    }
    
    /*url小处理*/
    if(url_type==UT_EMULE&&(sd_strncmp(p_dl_url->_url, "ed2k://%7", sd_strlen("ed2k://%7"))==0))
    {
        em_replace_7c(p_dl_url->_url);// 对于电驴链接，将其中的"%7c"换成"|"
    }
    else if(url_type == UT_HTTP_PARTIAL || url_type == UT_BT_PARTIAL)
    {
        // 对于不完整 的链接，现将其拼接成完整链接
              char *jointed_url = em_joint_url(g_url, match_buffer);
		sd_snprintf(p_dl_url->_url, MAX_URL_LEN-1, jointed_url);
        // 相应的将链接类型也改成UT_HTTP 和UT_BT
        if (url_type == UT_HTTP_PARTIAL) {
            url_type = UT_HTTP;
        }
        else if (url_type == UT_BT_PARTIAL) {
            url_type = UT_BT;
        }
    }
    
    p_dl_url->_type = url_type;
    
    // 直接从url中解析得到文件名等信息
    ret_val = em_parse_file_name_and_hash_value_from_url_new(p_dl_url->_url,url_type,p_dl_url, &hash_value);// 解析文件名并得到url 哈希值
    

	// 如果已经嗅探到文件名和文件大小，则使用嗅探到的文件名和文件大小
	if(name != NULL && *name != '\0')
	{
		sd_memset(p_dl_url->_name, 0, MAX_FILE_NAME_BUFFER_LEN);
		sd_strncpy(p_dl_url->_name, name, sd_strlen(name));

		char * point_pos = NULL;
		point_pos = sd_strrchr(p_dl_url->_name, '.');
		if(point_pos!=NULL)
		{
			p_dl_url->_file_type = em_get_file_type(point_pos);
		}
	}
	// 使用传入的文件大小
	p_dl_url->_file_size = file_size;
	
	// 文件名扩展标识嗅探
	if(detect_rule->_name_append[0] != '\0')
    	{
        	ret_val = em_detect_regex_match_working(string, detect_rule->_name_append, match_buffer);
        	if(ret_val == 0) 
        	{
            		char name_buffer[MAX_FILE_NAME_BUFFER_LEN] = {0};
            		char file_suffix[16] = {0};
            		char *point_pos = NULL;
            
            		sd_strncpy(name_buffer, p_dl_url->_name, MAX_FILE_NAME_BUFFER_LEN -1);
            
            		point_pos = sd_strrchr(name_buffer, '.');//查找后缀名
            		if(point_pos!=NULL)
            		{
                		// 将扩展内容插入到后缀名之前
                		sd_strncpy(file_suffix, point_pos, 15);
                		sd_memset(point_pos, 0x00, sd_strlen(point_pos)); //去掉原有的后缀名 
                		sd_strcat(name_buffer, match_buffer, sd_strlen(match_buffer));//将扩展内容接在文件名后面
                		sd_strcat(name_buffer, file_suffix, sd_strlen(file_suffix));// 添加文件后缀 
            		}
            		else
            		{
		                //将扩展内容接在文件名后面
		                sd_strcat(name_buffer, "(", 1);
		                sd_strcat(name_buffer, match_buffer, sd_strlen(match_buffer));
		                sd_strcat(name_buffer, ")", 1);
            		}
            
            		sd_strncpy(p_dl_url->_name, name_buffer, MAX_FILE_NAME_BUFFER_LEN -1);
        	}
        	sd_memset(match_buffer, 0, MAX_URL_LEN);// 清空match_buffer用于下一个匹配
    	}
    	// 扩展名嗅探
    	if(detect_rule->_suffix[0] != '\0')
	{
        	ret_val = em_detect_regex_match_working(string, detect_rule->_suffix, match_buffer);
        	if(ret_val == 0)
        	{
            		char name_buffer[MAX_FILE_NAME_BUFFER_LEN] = {0};
            		char *point_pos = NULL;
            
            		sd_strncpy(name_buffer, p_dl_url->_name, MAX_FILE_NAME_BUFFER_LEN -1);
            
            		point_pos = sd_strrchr(name_buffer, '.');//查找后缀名
            		if(point_pos!=NULL)
            		{
                		sd_memset(point_pos, 0x00, sd_strlen(point_pos+1)); //去掉原有的后缀名 (不需要保留点. )
            		}
            		sd_strcat(name_buffer, match_buffer, sd_strlen(match_buffer));//将扩展名接在文件名后面
            
            		sd_strncpy(p_dl_url->_name, name_buffer, MAX_FILE_NAME_BUFFER_LEN -1);
        	}
        	sd_memset(match_buffer, 0, MAX_URL_LEN);// 清空match_buffer用于下一个匹配
    	}
    else 
    {
    		// 如果没有扩展名嗅探方式，则使用链接最后携带的扩展名
    		if(url_type == UT_HTTP || url_type == UT_HTTP_PARTIAL)//http任务才能使用url后缀
    		{
    		char * point_pos = NULL;
		point_pos = sd_strrchr(p_dl_url->_url, '.');
		if(point_pos!=NULL)
		{
			p_dl_url->_file_type = em_get_file_type(point_pos);
			sd_strcat(p_dl_url->_name, point_pos, sd_strlen(point_pos));
		}
		}
    }

#ifdef CLOUD_PLAY_PROJ
	// 云播项目只添加视频任务
	if(p_dl_url->_file_type != EFT_VEDIO && p_dl_url->_file_type != EFT_UNKNOWN)
    	{
    		return SUCCESS;
    	}
#endif/*CLOUD_PLAY_PROJ*/

	EM_LOG_DEBUG( "em_detect_by_regex_rule:url=%s,type=%s,hash=%u,fsize=%llu,name=%s",p_dl_url->_url,(char*)(g_url_headers[p_dl_url->_type]),hash_value,p_dl_url->_file_size,p_dl_url->_name);
    
    // 最后将嗅探结果添加到url_map 中
    ret_val = em_add_url_to_map(p_url_map, hash_value, p_dl_url);
    if(ret_val!=SUCCESS) 
    {
        EM_LOG_ERROR( "em_detect_by_regex_rule:em_add_url_to_map ERROR!ret_val=%d",ret_val);
        SAFE_DELETE(p_dl_url);
    }
    
    return ret_val;

}

// 匹配方案1：先确定搜索区间，再在搜索区间里分别匹配url/name/size等各项信息
_int32 em_detect_with_regex_matching_scheme_1(const char* webpage_content, EM_DETECT_REGEX * detect_rule, MAP* p_url_map)
{
    _int32 ret_val = SUCCESS;
 
    // 如果没有开始串，则将整个网页作为搜索区间，执行一次匹配
    if (detect_rule->_start_string[0] == '\0' || detect_rule->_end_string[0] == '\0') {
        ret_val = em_detect_regex_infos_from_string(webpage_content, detect_rule, p_url_map);
    }
    if (detect_rule->_start_string[0] != '\0' && detect_rule->_end_string[0] != '\0') {
        char *pos = NULL;
        // 先查找搜索区间
        _int32 length = em_get_string_between_strings(webpage_content, &pos, detect_rule->_start_string, 1, detect_rule->_end_string, 1);
        if (length == -1) {//未找到搜索区间
            EM_LOG_DEBUG("em_get_string_between_strings didn't find string!");
            return -1;
        }
        while (length != -1) {
            if (length > 0) {
                char *match_area = NULL;
                ret_val = sd_malloc(length+1, (void**)&match_area);
                CHECK_VALUE(ret_val);
                sd_memcpy(match_area, pos, length);
                *(match_area+length)='\0';// 字符串结束
                
                // 在其中进行嗅探匹配
                ret_val = em_detect_regex_infos_from_string(match_area, detect_rule, p_url_map);
                
                sd_free(match_area);
                match_area = NULL;
            }
            
            // 从结尾开始找出下一个搜索区间
            length = em_get_string_between_strings(pos+length, &pos, detect_rule->_start_string, 1, detect_rule->_end_string, 1);
        }
    }
	
	return SUCCESS;
    
}


// 匹配方案2：搜索区间仅用于匹配url，name/size等各项信息通过全文匹配一次获得，适用于所有url的name等信息相同的网站
_int32 em_detect_with_regex_matching_scheme_2(const char* webpage_content, EM_DETECT_REGEX * detect_rule, MAP* p_url_map)
{
      	 _int32 ret_val = 0;
	char match_buffer[MAX_URL_LEN] = {0};
	char detected_name[MAX_FILE_NAME_BUFFER_LEN] = {0};
	_u64 file_size = 0;

	// 文件名嗅探
	 if (detect_rule->_name[0]!= '\0')
	 {
	        ret_val = em_detect_regex_match_working(webpage_content, detect_rule->_name, match_buffer);// 在整个网页中找到name
	        if(ret_val == 0) 
	        {
	            	sd_memcpy(detected_name, match_buffer, sd_strlen(match_buffer));
	        	sd_memset(match_buffer, 0, MAX_URL_LEN);
	        }
	 }
	
	// 文件大小嗅探
	if(detect_rule->_file_size[0] != '\0')
    	{
        	ret_val = em_detect_regex_match_working(webpage_content, detect_rule->_file_size, match_buffer);
        	if(ret_val == 0)
        	{
            		file_size = em_filesize_str_to_u64(match_buffer);
        	}
        	sd_memset(match_buffer, 0, MAX_URL_LEN);// 清空match_buffer用于下一个匹配
    	}

	////////////////////////////////////////////////////
	// url下载链接等其他信息嗅探
	//////////////////////////////////////////////////

	    	// 如果没有开始串，则将整个网页作为搜索区间，执行一次匹配
    		if (detect_rule->_start_string[0] == '\0' || detect_rule->_end_string[0] == '\0') {
        		ret_val = em_detect_regex_infos_from_string_except_name_and_size(webpage_content, detect_rule, detected_name, file_size, p_url_map);
    		}
    		if (detect_rule->_start_string[0] != '\0' && detect_rule->_end_string[0] != '\0') {
        		char *pos = NULL;
        		// 先查找搜索区间
        		_int32 length = em_get_string_between_strings(webpage_content, &pos, detect_rule->_start_string, 1, detect_rule->_end_string, 1);
        		if (length == -1) {//未找到搜索区间
            			EM_LOG_DEBUG("em_get_string_between_strings didn't find string!");
            			return -1;
        		}
        		while (length != -1) {
            			if (length > 0) {
                			char *match_area = NULL;
                			ret_val = sd_malloc(length+1, (void**)&match_area);
                			CHECK_VALUE(ret_val);
                			sd_memcpy(match_area, pos, length);
                			*(match_area+length)='\0';// 字符串结束
                
                			// 在其中进行嗅探匹配
        				ret_val = em_detect_regex_infos_from_string_except_name_and_size(match_area, detect_rule, detected_name, file_size, p_url_map);
                
                			sd_free(match_area);
                			match_area = NULL;
            			}
		             // 从结尾开始找出下一个搜索区间
		            length = em_get_string_between_strings(pos+length, &pos, detect_rule->_start_string, 1, detect_rule->_end_string, 1);
       		}/*while(length != -1)*/
        }

	return SUCCESS;
}

// 根据规则中的匹配方案类型，分别调用不同的函数进行嗅探
_int32 em_detect_by_regex_rule(const char* webpage_content, EM_DETECT_REGEX * detect_rule, MAP* p_url_map)
{
    _int32 ret_val = 0;
    switch (detect_rule->_matching_scheme) {
        case 1:
            em_detect_with_regex_matching_scheme_1(webpage_content, detect_rule, p_url_map);
            break;
        case 2:
            em_detect_with_regex_matching_scheme_2(webpage_content, detect_rule, p_url_map);            
            break;
        default:
            ret_val = -1;// 未找到对应匹配方案
            break;
    }
    return ret_val;
}

_int32 em_detect_string_infos_from_string(const char* string, EM_DETECT_STRING* detect_rule, MAP* p_url_map)
{
	_int32 ret_val = SUCCESS;
	char *match_buffer_pos = NULL;
	_int32 match_buffer_len = 0;
	EM_DOWNLOADABLE_URL * p_dl_url = NULL;
	_u32 hash_value = 0;

    if(detect_rule->_download_url_start[0]== '\0' || detect_rule->_download_url_end[0] == '\0') return SUCCESS;// 如果没有下载链接string，则直接返回
        
    // 可下载链接嗅探
    match_buffer_len = em_get_string_between_strings(string, &match_buffer_pos, detect_rule->_download_url_start, detect_rule->_download_url_start_include, detect_rule->_download_url_end, detect_rule->_download_url_end_include);
    if(match_buffer_len == -1) return SUCCESS;// 没有找到可下载链接，直接返回
    
    ret_val = sd_malloc(sizeof(EM_DOWNLOADABLE_URL), (void**)&p_dl_url);
    CHECK_VALUE(ret_val);
    sd_memset(p_dl_url, 0, sizeof(EM_DOWNLOADABLE_URL));

    if(match_buffer_len >= MAX_URL_LEN)
    	return SUCCESS;//超出长度，无法取得正确的URL，直接返回
    sd_memcpy(p_dl_url->_url, match_buffer_pos, match_buffer_len);
    //为种子猫"torrentkittycn.com"做的特殊处理
	if(sd_strstr(g_url, "torrentkittycn.com", 0)!=NULL)
	{
		char decoded_url[MAX_URL_LEN] = {0};
	    match_buffer_len = url_object_decode_ex(p_dl_url->_url, decoded_url, MAX_URL_LEN);
		if(match_buffer_len == -1) 
			return SUCCESS;// 解码失败，直接返回
	    sd_memset(p_dl_url->_url, 0, MAX_URL_LEN);
		sd_memcpy(p_dl_url->_url, decoded_url, MAX_URL_LEN - 1);
    }
	
			 
    EM_URL_TYPE url_type = em_get_url_type(p_dl_url->_url);
    if(url_type == UT_NORMAL) 
    {
    	// 如果嗅探错误，得到一个不可下载的链接，则直接舍去
    	SAFE_DELETE(p_dl_url);
    	return SUCCESS;
    }
    
    /*url小处理*/
    if(url_type==UT_EMULE&&(sd_strncmp(p_dl_url->_url, "ed2k://%7", sd_strlen("ed2k://%7"))==0))
    {
        em_replace_7c(p_dl_url->_url);// 对于电驴链接，将其中的"%7c"换成"|"
    }
    else if(url_type == UT_HTTP_PARTIAL || url_type == UT_BT_PARTIAL)
    {
    	char match_buffer[MAX_URL_LEN] = {0};
    	sd_memcpy(match_buffer, match_buffer_pos, match_buffer_len);
        // 对于不完整 的链接，现将其拼接成完整链接
              char *jointed_url = em_joint_url(g_url, match_buffer);
		sd_snprintf(p_dl_url->_url, MAX_URL_LEN-1, jointed_url);
        // 相应的将链接类型也改成UT_HTTP 和UT_BT
        if (url_type == UT_HTTP_PARTIAL) {
            url_type = UT_HTTP;
        }
        else if (url_type == UT_BT_PARTIAL) {
            url_type = UT_BT;
        }
    }
    
    p_dl_url->_type = url_type;
    
    // 直接从url中解析得到文件名等信息
    ret_val = em_parse_file_name_and_hash_value_from_url_new(p_dl_url->_url,url_type,p_dl_url, &hash_value);// 解析文件名并得到url 哈希值

     // 文件名嗅探
    if(detect_rule->_name_start[0] != '\0' && detect_rule->_name_end[0] != '\0')
    {
	 match_buffer_len = em_get_string_between_strings(string, &match_buffer_pos, detect_rule->_name_start, detect_rule->_name_start_include, detect_rule->_name_end, detect_rule->_name_end_include);
        if(match_buffer_len > 0) 
        {
#if 1
		char filename[MAX_URL_LEN]= {0};
		sd_memcpy(filename, match_buffer_pos, match_buffer_len);
        	 //为种子猫"torrentkittycn.com"做的特殊处理
		if(sd_strstr(g_url, "torrentkittycn.com", 0)!=NULL)
		{	
			if(match_buffer_len<MAX_FILE_NAME_BUFFER_LEN)
			{
				//过滤掉文件名中的脏字符
    	    			sd_replace_str(filename, "<em>", "");
    	    			sd_replace_str(filename, "</em>", "");
    	    		}
    		}
    		
    		if(sd_strlen(filename)<MAX_FILE_NAME_BUFFER_LEN)
    		{
	            sd_memset(p_dl_url->_name, 0, MAX_FILE_NAME_BUFFER_LEN);
	            sd_memcpy(p_dl_url->_name, filename, MAX_FILE_NAME_BUFFER_LEN);
		}
#else
        	if(match_buffer_len<MAX_FILE_NAME_BUFFER_LEN){
	            sd_memset(p_dl_url->_name, 0, MAX_FILE_NAME_BUFFER_LEN);
	            sd_memcpy(p_dl_url->_name, match_buffer_pos, match_buffer_len);
		}
 #endif/*TORRENTKITTY*/

        }
        else 
        {
            //为圣城家园"btscg.com"做的特殊处理
	    if(sd_strstr(g_url, "btscg.com", 0)!=NULL)
	    {
    	    		SAFE_DELETE(p_dl_url);
	    		return SUCCESS;
    	     }
        }
    }
    
    // 文件名扩展标识嗅探
    if(detect_rule->_name_append_start[0] != '\0' && detect_rule->_name_append_end[0] != '\0')
    {
    match_buffer_len = em_get_string_between_strings(string, &match_buffer_pos, detect_rule->_name_append_start, detect_rule->_name_append_start_include, detect_rule->_name_append_end, detect_rule->_name_append_end_include);
        if(match_buffer_len > 0) 
        {
            char name_buffer[MAX_FILE_NAME_BUFFER_LEN] = {0};
            char file_suffix[16] = {0};
            char *point_pos = NULL;
            
            sd_memcpy(name_buffer, p_dl_url->_name, sd_strlen(p_dl_url->_name));
            
            point_pos = sd_strrchr(name_buffer, '.');//查找后缀名
            if(point_pos!=NULL)
            {
                // 将扩展内容插入到后缀名之前
                sd_memcpy(file_suffix, point_pos, sd_strlen(point_pos));
                sd_memset(point_pos, 0x00, sd_strlen(point_pos)); //去掉原有的后缀名 
                sd_strcat(name_buffer, match_buffer_pos, match_buffer_len);//将扩展内容接在文件名后面
                sd_strcat(name_buffer, file_suffix, sd_strlen(file_suffix));// 添加文件后缀 
            }
            else
            {
                //将扩展内容接在文件名后面
                sd_strcat(name_buffer, "(", 1);
                sd_strcat(name_buffer, match_buffer_pos, match_buffer_len);
                sd_strcat(name_buffer, ")", 1);
            }
            
            sd_memcpy(p_dl_url->_name, name_buffer, sd_strlen(name_buffer));
        }
    }
    
    // 扩展名嗅探
    if(detect_rule->_suffix_start[0] != '\0' && detect_rule->_suffix_end[0] != '\0')
    {
    match_buffer_len = em_get_string_between_strings(string, &match_buffer_pos, detect_rule->_suffix_start, detect_rule->_suffix_start_include, detect_rule->_suffix_end, detect_rule->_suffix_end_include);
        if(match_buffer_len > 0 && match_buffer_len < 16) 
        {
            char name_buffer[MAX_FILE_NAME_BUFFER_LEN] = {0};
            char file_suffix[16] = {0};
            char *point_pos = NULL;
            
            sd_memcpy(name_buffer, p_dl_url->_name, sd_strlen(p_dl_url->_name));
	     sd_memcpy(file_suffix, match_buffer_pos, match_buffer_len);
            
            point_pos = sd_strrchr(name_buffer, '.');//查找后缀名
            if(point_pos!=NULL)
            {
                sd_memset(point_pos, 0x00, sd_strlen(point_pos+1)); //去掉原有的后缀名 (不需要保留点. )
            }
            sd_strcat(name_buffer, match_buffer_pos, match_buffer_len);//将扩展名接在文件名后面
            
            sd_memcpy(p_dl_url->_name, name_buffer, sd_strlen(name_buffer));
            p_dl_url->_file_type = em_get_file_type(file_suffix);
            
        }
    }
    else 
    {
    		// 如果没有扩展名嗅探方式，则使用链接最后携带的扩展名
    		if(url_type == UT_HTTP || url_type == UT_HTTP_PARTIAL)//http任务才能使用url后缀
    		{
    		char * point_pos = NULL;
		point_pos = sd_strrchr(p_dl_url->_url, '.');
		if(point_pos!=NULL)
		{
			p_dl_url->_file_type = em_get_file_type(point_pos);
			sd_strcat(p_dl_url->_name, point_pos, sd_strlen(point_pos));
		}
		}
    }
    
    // 文件大小嗅探
    if(detect_rule->_file_size_start[0] != '\0' && detect_rule->_file_size_end[0] != '\0')
    {
    match_buffer_len = em_get_string_between_strings(string, &match_buffer_pos, detect_rule->_file_size_start, detect_rule->_file_size_start_include, detect_rule->_file_size_end, detect_rule->_file_size_end_include);
        if(match_buffer_len > 0 && match_buffer_len < 16) 
        {
            char file_size_buffer[16] = {0};
	     sd_memcpy(file_size_buffer, match_buffer_pos, match_buffer_len);
            p_dl_url->_file_size = em_filesize_str_to_u64(file_size_buffer);
        }
    }

#ifdef CLOUD_PLAY_PROJ
	// 云播项目只添加视频任务
	if(p_dl_url->_file_type != EFT_VEDIO && p_dl_url->_file_type != EFT_UNKNOWN)
    	{
    		return SUCCESS;
    	}
#endif/*CLOUD_PLAY_PROJ*/
    
    EM_LOG_DEBUG( "em_detect_by_detect_string_rule:url=%s,type=%s,hash=%u,fsize=%llu,name=%s",p_dl_url->_url,(char*)(g_url_headers[p_dl_url->_type]),hash_value,p_dl_url->_file_size,p_dl_url->_name);
    
    // 最后将嗅探结果添加到url_map 中
    ret_val = em_add_url_to_map(p_url_map, hash_value, p_dl_url);
    if(ret_val!=SUCCESS) 
    {
        EM_LOG_ERROR( "em_detect_by_detect_string_rule:em_add_url_to_map ERROR!ret_val=%d",ret_val);
        SAFE_DELETE(p_dl_url);
    }
    
    return ret_val;

}

_int32 em_detect_string_infos_from_string_except_name(const char* string, EM_DETECT_STRING* detect_rule, const char *name, MAP* p_url_map)
{
	_int32 ret_val = SUCCESS;
	char *match_buffer_pos = NULL;
	_int32 match_buffer_len = 0;
	EM_DOWNLOADABLE_URL * p_dl_url = NULL;
	_u32 hash_value = 0;

    if(detect_rule->_download_url_start[0] == '\0' || detect_rule->_download_url_end[0] == '\0') return SUCCESS;// 如果没有下载链接string，则直接返回
        
    // 可下载链接嗅探
    match_buffer_len = em_get_string_between_strings(string, &match_buffer_pos, detect_rule->_download_url_start, detect_rule->_download_url_start_include, detect_rule->_download_url_end, detect_rule->_download_url_end_include);
    if(match_buffer_len == -1) return SUCCESS;// 没有找到可下载链接，直接返回
    
    ret_val = sd_malloc(sizeof(EM_DOWNLOADABLE_URL), (void**)&p_dl_url);
    CHECK_VALUE(ret_val);
    sd_memset(p_dl_url, 0, sizeof(EM_DOWNLOADABLE_URL));

    sd_memcpy(p_dl_url->_url, match_buffer_pos, match_buffer_len);
    
    EM_URL_TYPE url_type = em_get_url_type(p_dl_url->_url);
    if(url_type == UT_NORMAL) 
    {
    	// 如果嗅探错误，得到一个不可下载的链接，则直接舍去
    	SAFE_DELETE(p_dl_url);
    	return SUCCESS;
    }
    
    /*url小处理*/
    if(url_type==UT_EMULE&&(sd_strncmp(p_dl_url->_url, "ed2k://%7", sd_strlen("ed2k://%7"))==0))
    {
        em_replace_7c(p_dl_url->_url);// 对于电驴链接，将其中的"%7c"换成"|"
    }
    else if(url_type == UT_HTTP_PARTIAL || url_type == UT_BT_PARTIAL)
    {
    	char match_buffer[MAX_URL_LEN] = {0};
    	sd_memcpy(match_buffer, match_buffer_pos, match_buffer_len);
        // 对于不完整 的链接，现将其拼接成完整链接
              char *jointed_url = em_joint_url(g_url, match_buffer);
		sd_snprintf(p_dl_url->_url, MAX_URL_LEN-1, jointed_url);
        // 相应的将链接类型也改成UT_HTTP 和UT_BT
        if (url_type == UT_HTTP_PARTIAL) {
            url_type = UT_HTTP;
        }
        else if (url_type == UT_BT_PARTIAL) {
            url_type = UT_BT;
        }
    }
    
    p_dl_url->_type = url_type;
    
    // 直接从url中解析得到文件名等信息
    ret_val = em_parse_file_name_and_hash_value_from_url_new(p_dl_url->_url,url_type,p_dl_url, &hash_value);// 解析文件名并得到url 哈希值
    

   // 如果已经嗅探到文件名，则使用嗅探到的文件名和文件大小
   if(name != NULL && *name != '\0')
   {
	sd_memset(p_dl_url->_name, 0, MAX_FILE_NAME_BUFFER_LEN);
	sd_strncpy(p_dl_url->_name, name, sd_strlen(name));
   }

    // 文件名扩展标识嗅探
    if(detect_rule->_name_append_start[0] != '\0' && detect_rule->_name_append_end[0] != '\0')
    {
    match_buffer_len = em_get_string_between_strings(string, &match_buffer_pos, detect_rule->_name_append_start, detect_rule->_name_append_start_include, detect_rule->_name_append_end, detect_rule->_name_append_end_include);
        if(match_buffer_len > 0) 
        {
            char name_buffer[MAX_FILE_NAME_BUFFER_LEN] = {0};
            char file_suffix[16] = {0};
            char *point_pos = NULL;
            
            sd_memcpy(name_buffer, p_dl_url->_name, sd_strlen(p_dl_url->_name));
            
            point_pos = sd_strrchr(name_buffer, '.');//查找后缀名
            if(point_pos!=NULL)
            {
                // 将扩展内容插入到后缀名之前
                sd_memcpy(file_suffix, point_pos, sd_strlen(point_pos));
                sd_memset(point_pos, 0x00, sd_strlen(point_pos)); //去掉原有的后缀名 
                sd_strcat(name_buffer, match_buffer_pos, match_buffer_len);//将扩展内容接在文件名后面
                sd_strcat(name_buffer, file_suffix, sd_strlen(file_suffix));// 添加文件后缀 
            }
            else
            {
                //将扩展内容接在文件名后面
                sd_strcat(name_buffer, "(", 1);
                sd_strcat(name_buffer, match_buffer_pos, match_buffer_len);
                sd_strcat(name_buffer, ")", 1);
            }
            
            sd_memcpy(p_dl_url->_name, name_buffer, sd_strlen(name_buffer));
        }
    }

    // 扩展名嗅探
    if(detect_rule->_suffix_start[0] != '\0' && detect_rule->_suffix_end[0] != '\0')
    {
    match_buffer_len = em_get_string_between_strings(string, &match_buffer_pos, detect_rule->_suffix_start, detect_rule->_suffix_start_include, detect_rule->_suffix_end, detect_rule->_suffix_end_include);
        if(match_buffer_len > 0 && match_buffer_len < 16) 
        {
            char name_buffer[MAX_FILE_NAME_BUFFER_LEN] = {0};
            char file_suffix[16] = {0};
            char *point_pos = NULL;
            
            sd_memcpy(name_buffer, p_dl_url->_name, sd_strlen(p_dl_url->_name));
	     sd_memcpy(file_suffix, match_buffer_pos, match_buffer_len);
            
            point_pos = sd_strrchr(name_buffer, '.');//查找后缀名
            if(point_pos!=NULL)
            {
                sd_memset(point_pos, 0x00, sd_strlen(point_pos+1)); //去掉原有的后缀名 (不需要保留点. )
            }
            sd_strcat(name_buffer, match_buffer_pos, match_buffer_len);//将扩展名接在文件名后面
            
            sd_memcpy(p_dl_url->_name, name_buffer, sd_strlen(name_buffer));
            p_dl_url->_file_type = em_get_file_type(file_suffix);
            
        }
    }
    else 
    {
    		// 如果没有扩展名嗅探方式，则使用链接最后携带的扩展名
    		if(url_type == UT_HTTP || url_type == UT_HTTP_PARTIAL)//http任务才能使用url后缀
    		{
    		char * point_pos = NULL;
		point_pos = sd_strrchr(p_dl_url->_url, '.');
		if(point_pos!=NULL)
		{
			p_dl_url->_file_type = em_get_file_type(point_pos);
			sd_strcat(p_dl_url->_name, point_pos, sd_strlen(point_pos));
		}
		}
    }

    // 文件大小嗅探
    if(detect_rule->_file_size_start[0] != '\0' && detect_rule->_file_size_end[0] != '\0')
    {
    match_buffer_len = em_get_string_between_strings(string, &match_buffer_pos, detect_rule->_file_size_start, detect_rule->_file_size_start_include, detect_rule->_file_size_end, detect_rule->_file_size_end_include);
        if(match_buffer_len > 0 && match_buffer_len < 16) 
        {
            char file_size_buffer[16] = {0};
	     sd_memcpy(file_size_buffer, match_buffer_pos, match_buffer_len);
            p_dl_url->_file_size = em_filesize_str_to_u64(file_size_buffer);
        }
    }

#ifdef CLOUD_PLAY_PROJ
	// 云播项目只添加视频任务
	if(p_dl_url->_file_type != EFT_VEDIO && p_dl_url->_file_type != EFT_UNKNOWN)
    	{
    		return SUCCESS;
    	}
#endif/*CLOUD_PLAY_PROJ*/
	
    EM_LOG_DEBUG( "em_detect_by_detect_string_rule:url=%s,type=%s,hash=%u,fsize=%llu,name=%s",p_dl_url->_url,(char*)(g_url_headers[p_dl_url->_type]),hash_value,p_dl_url->_file_size,p_dl_url->_name);
    
    // 最后将嗅探结果添加到url_map 中
    ret_val = em_add_url_to_map(p_url_map, hash_value, p_dl_url);
    if(ret_val!=SUCCESS) 
    {
        EM_LOG_ERROR( "em_detect_by_detect_string_rule:em_add_url_to_map ERROR!ret_val=%d",ret_val);
        SAFE_DELETE(p_dl_url);
    }
    
    return ret_val;

}

_int32 em_detect_string_infos_from_string_except_name_and_size(const char* string, EM_DETECT_STRING* detect_rule, const char *name, const _u64 file_size, MAP* p_url_map)
{
	_int32 ret_val = SUCCESS;
	char *match_buffer_pos = NULL;
	_int32 match_buffer_len = 0;
	EM_DOWNLOADABLE_URL * p_dl_url = NULL;
	_u32 hash_value = 0;

    if(detect_rule->_download_url_start[0] == '\0' || detect_rule->_download_url_end[0] == '\0') return SUCCESS;// 如果没有下载链接string，则直接返回
        
    // 可下载链接嗅探
    match_buffer_len = em_get_string_between_strings(string, &match_buffer_pos, detect_rule->_download_url_start, detect_rule->_download_url_start_include, detect_rule->_download_url_end, detect_rule->_download_url_end_include);
    if(match_buffer_len == -1) return SUCCESS;// 没有找到可下载链接，直接返回
    
    ret_val = sd_malloc(sizeof(EM_DOWNLOADABLE_URL), (void**)&p_dl_url);
    CHECK_VALUE(ret_val);
    sd_memset(p_dl_url, 0, sizeof(EM_DOWNLOADABLE_URL));

    sd_memcpy(p_dl_url->_url, match_buffer_pos, match_buffer_len);
    
    EM_URL_TYPE url_type = em_get_url_type(p_dl_url->_url);
    if(url_type == UT_NORMAL) 
    {
    	// 如果嗅探错误，得到一个不可下载的链接，则直接舍去
    	SAFE_DELETE(p_dl_url);
    	return SUCCESS;
    }
    
    /*url小处理*/
    if(url_type==UT_EMULE&&(sd_strncmp(p_dl_url->_url, "ed2k://%7", sd_strlen("ed2k://%7"))==0))
    {
        em_replace_7c(p_dl_url->_url);// 对于电驴链接，将其中的"%7c"换成"|"
    }
    else if(url_type == UT_HTTP_PARTIAL || url_type == UT_BT_PARTIAL)
    {
    	char match_buffer[MAX_URL_LEN] = {0};
    	sd_memcpy(match_buffer, match_buffer_pos, match_buffer_len);
        // 对于不完整 的链接，现将其拼接成完整链接
              char *jointed_url = em_joint_url(g_url, match_buffer);
		sd_snprintf(p_dl_url->_url, MAX_URL_LEN-1, jointed_url);
        // 相应的将链接类型也改成UT_HTTP 和UT_BT
        if (url_type == UT_HTTP_PARTIAL) {
            url_type = UT_HTTP;
        }
        else if (url_type == UT_BT_PARTIAL) {
            url_type = UT_BT;
        }
    }
    
    p_dl_url->_type = url_type;
    
    // 直接从url中解析得到文件名等信息
    ret_val = em_parse_file_name_and_hash_value_from_url_new(p_dl_url->_url,url_type,p_dl_url, &hash_value);// 解析文件名并得到url 哈希值
    

	// 如果已经嗅探到文件名和文件大小，则使用嗅探到的文件名和文件大小
	if(name != NULL && *name != '\0')
	{
		sd_memset(p_dl_url->_name, 0, MAX_FILE_NAME_BUFFER_LEN);
		sd_strncpy(p_dl_url->_name, name, sd_strlen(name));
	}
	// 使用传入的文件大小
	p_dl_url->_file_size = file_size;
	
    // 文件名扩展标识嗅探
    if(detect_rule->_name_append_start[0] != '\0' && detect_rule->_name_append_end[0] != '\0')
    {
    match_buffer_len = em_get_string_between_strings(string, &match_buffer_pos, detect_rule->_name_append_start, detect_rule->_name_append_start_include, detect_rule->_name_append_end, detect_rule->_name_append_end_include);
        if(match_buffer_len > 0) 
        {
            char name_buffer[MAX_FILE_NAME_BUFFER_LEN] = {0};
            char file_suffix[16] = {0};
            char *point_pos = NULL;
            
            sd_memcpy(name_buffer, p_dl_url->_name, sd_strlen(p_dl_url->_name));
            
            point_pos = sd_strrchr(name_buffer, '.');//查找后缀名
            if(point_pos!=NULL)
            {
                // 将扩展内容插入到后缀名之前
                sd_memcpy(file_suffix, point_pos, sd_strlen(point_pos));
                sd_memset(point_pos, 0x00, sd_strlen(point_pos)); //去掉原有的后缀名 
                sd_strcat(name_buffer, match_buffer_pos, match_buffer_len);//将扩展内容接在文件名后面
                sd_strcat(name_buffer, file_suffix, sd_strlen(file_suffix));// 添加文件后缀 
            }
            else
            {
                //将扩展内容接在文件名后面
                sd_strcat(name_buffer, "(", 1);
                sd_strcat(name_buffer, match_buffer_pos, match_buffer_len);
                sd_strcat(name_buffer, ")", 1);
            }
            
            sd_memcpy(p_dl_url->_name, name_buffer, sd_strlen(name_buffer));
        }
    }

    // 扩展名嗅探
    if(detect_rule->_suffix_start[0] != '\0' && detect_rule->_suffix_end[0] != '\0')
    {
    match_buffer_len = em_get_string_between_strings(string, &match_buffer_pos, detect_rule->_suffix_start, detect_rule->_suffix_start_include, detect_rule->_suffix_end, detect_rule->_suffix_end_include);
        if(match_buffer_len > 0 && match_buffer_len < 16) 
        {
            char name_buffer[MAX_FILE_NAME_BUFFER_LEN] = {0};
            char file_suffix[16] = {0};
            char *point_pos = NULL;
            
            sd_memcpy(name_buffer, p_dl_url->_name, sd_strlen(p_dl_url->_name));
	     sd_memcpy(file_suffix, match_buffer_pos, match_buffer_len);
            
            point_pos = sd_strrchr(name_buffer, '.');//查找后缀名
            if(point_pos!=NULL)
            {
                sd_memset(point_pos, 0x00, sd_strlen(point_pos+1)); //去掉原有的后缀名 (不需要保留点. )
            }
            sd_strcat(name_buffer, match_buffer_pos, match_buffer_len);//将扩展名接在文件名后面
            
            sd_memcpy(p_dl_url->_name, name_buffer, sd_strlen(name_buffer));
            p_dl_url->_file_type = em_get_file_type(file_suffix);
            
        }
    }
    else 
    {
    		// 如果没有扩展名嗅探方式，则使用链接最后携带的扩展名
     		if(url_type == UT_HTTP || url_type == UT_HTTP_PARTIAL)//http任务才能使用url后缀
    		{
   		char * point_pos = NULL;
		point_pos = sd_strrchr(p_dl_url->_url, '.');
		if(point_pos!=NULL)
		{
			p_dl_url->_file_type = em_get_file_type(point_pos);
			sd_strcat(p_dl_url->_name, point_pos, sd_strlen(point_pos));
		}
		}
    }

#ifdef CLOUD_PLAY_PROJ
	// 云播项目只添加视频任务
	if(p_dl_url->_file_type != EFT_VEDIO && p_dl_url->_file_type != EFT_UNKNOWN)
    	{
    		return SUCCESS;
    	}
#endif/*CLOUD_PLAY_PROJ*/
    EM_LOG_DEBUG( "em_detect_by_detect_string_rule:url=%s,type=%s,hash=%u,fsize=%llu,name=%s",p_dl_url->_url,(char*)(g_url_headers[p_dl_url->_type]),hash_value,p_dl_url->_file_size,p_dl_url->_name);
    
    // 最后将嗅探结果添加到url_map 中
    ret_val = em_add_url_to_map(p_url_map, hash_value, p_dl_url);
    if(ret_val!=SUCCESS) 
    {
        EM_LOG_ERROR( "em_detect_by_detect_string_rule:em_add_url_to_map ERROR!ret_val=%d",ret_val);
        SAFE_DELETE(p_dl_url);
    }
    
    return ret_val;

}

// 匹配方案1：先确定搜索区间，再在搜索区间里分别匹配url/name/size等各项信息
_int32 em_detect_with_string_matching_scheme_1(const char* webpage_content, EM_DETECT_STRING* detect_rule, MAP* p_url_map)
{
    _int32 ret_val = SUCCESS;
 
    // 如果没有开始串，则将整个网页作为搜索区间，执行一次匹配
    if (detect_rule->_string_start[0] == '\0' || detect_rule->_string_end[0] == '\0') {
        ret_val = em_detect_string_infos_from_string(webpage_content, detect_rule, p_url_map);
    }
    if (detect_rule->_string_start[0] != '\0' && detect_rule->_string_end[0] != '\0') {
        char *pos = NULL;
        // 先查找搜索区间
        _int32 length = em_get_string_between_strings(webpage_content, &pos, detect_rule->_string_start, detect_rule->_string_start_include, detect_rule->_string_end, detect_rule->_string_end_include);
        if (length == -1) {//未找到搜索区间
            EM_LOG_DEBUG("em_get_string_between_strings didn't find string!");
            return -1;
        }
        while (length != -1) {
            if (length > 0) {
                char *match_area = NULL;
                ret_val = sd_malloc(length+1, (void**)&match_area);
                CHECK_VALUE(ret_val);
                sd_memcpy(match_area, pos, length);
                *(match_area+length)='\0';// 字符串结束
                
                // 在其中进行嗅探匹配
                ret_val = em_detect_string_infos_from_string(match_area, detect_rule, p_url_map);
                
                SAFE_DELETE(match_area);
                match_area = NULL;
            }
            
            // 从结尾开始找出下一个搜索区间
            length = em_get_string_between_strings(pos+length, &pos, detect_rule->_string_start, detect_rule->_string_start_include, detect_rule->_string_end, detect_rule->_string_end_include);
        }
    }
	
	return SUCCESS;
    
}

// 匹配方案2：先通过全文匹配获得一次name信息，再确定搜索区间，在搜索区间里分别匹配除name外的各项信息，所有url使用相同的name，可通过name_append和suffix进行区别
_int32 em_detect_with_string_matching_scheme_2(const char* webpage_content, EM_DETECT_STRING* detect_rule, MAP* p_url_map)
{
      	 _int32 ret_val = 0;
	char *match_buffer_pos = NULL;
	_int32 match_buffer_len = 0;
	char detected_name[MAX_FILE_NAME_BUFFER_LEN] = {0};

     // 文件名嗅探
    if(detect_rule->_name_start[0] != '\0' && detect_rule->_name_end[0] != '\0')
    {
	    match_buffer_len = em_get_string_between_strings(webpage_content, &match_buffer_pos, detect_rule->_name_start, detect_rule->_name_start_include, detect_rule->_name_end, detect_rule->_name_end_include);
        if(match_buffer_len > 0 && match_buffer_len<MAX_FILE_NAME_BUFFER_LEN) 
        {
	            sd_memcpy(detected_name, match_buffer_pos, match_buffer_len);
        }
    }
	
	////////////////////////////////////////////////////
	// url下载链接等其他信息嗅探
	//////////////////////////////////////////////////
	
	    	// 如果没有开始串，则将整个网页作为搜索区间，执行一次匹配
    		if (detect_rule->_string_start[0] == '\0' || detect_rule->_string_end[0] == '\0') {
        		ret_val = em_detect_string_infos_from_string_except_name(webpage_content, detect_rule, detected_name, p_url_map);
    		}
    		if (detect_rule->_string_start[0] != '\0' && detect_rule->_string_end[0] != '\0') {
        		char *pos = NULL;
        		// 先查找搜索区间
        		_int32 length = em_get_string_between_strings(webpage_content, &pos, detect_rule->_string_start, detect_rule->_string_start_include, detect_rule->_string_end, detect_rule->_string_end_include);
        		if (length == -1) {//未找到搜索区间
            			EM_LOG_DEBUG("em_get_string_between_strings didn't find string!");
            			return -1;
        		}
        		while (length != -1) {
            			if (length > 0) {
                			char *match_area = NULL;
                			ret_val = sd_malloc(length+1, (void**)&match_area);
                			CHECK_VALUE(ret_val);
                			sd_memcpy(match_area, pos, length);
                			*(match_area+length)='\0';// 字符串结束
                
                			// 在其中进行嗅探匹配
        				ret_val = em_detect_string_infos_from_string_except_name(match_area, detect_rule, detected_name, p_url_map);
                
                			sd_free(match_area);
                			match_area = NULL;
            			}
		            // 从结尾开始找出下一个搜索区间
		            length = em_get_string_between_strings(pos+length, &pos, detect_rule->_string_start, detect_rule->_string_start_include, detect_rule->_string_end, detect_rule->_string_end_include);
        		}/*while(length != -1)*/
        }

	return SUCCESS;
}

// 匹配方案3：搜索区间仅用于匹配url，name/size等各项信息通过全文匹配一次获得，适用于所有url的name等信息相同的网站
_int32 em_detect_with_string_matching_scheme_3(const char* webpage_content, EM_DETECT_STRING* detect_rule, MAP* p_url_map)
{
      	 _int32 ret_val = 0;
	char *match_buffer_pos = NULL;
	_int32 match_buffer_len = 0;
	char detected_name[MAX_FILE_NAME_BUFFER_LEN] = {0};
	_u64 file_size = 0;

     // 文件名嗅探
    if(detect_rule->_name_start[0] != '\0' && detect_rule->_name_end[0] != '\0')
    {
	    match_buffer_len = em_get_string_between_strings(webpage_content, &match_buffer_pos, detect_rule->_name_start, detect_rule->_name_start_include, detect_rule->_name_end, detect_rule->_name_end_include);
        if(match_buffer_len > 0 && match_buffer_len<MAX_FILE_NAME_BUFFER_LEN) 
        {
	            sd_memcpy(detected_name, match_buffer_pos, match_buffer_len);
        }
    }
	
    // 文件大小嗅探
    if(detect_rule->_file_size_start[0] != '\0' && detect_rule->_file_size_end[0] != '\0')
    {
    match_buffer_len = em_get_string_between_strings(webpage_content, &match_buffer_pos, detect_rule->_file_size_start, detect_rule->_file_size_start_include, detect_rule->_file_size_end, detect_rule->_file_size_end_include);
        if(match_buffer_len > 0 && match_buffer_len < 16) 
        {
            char file_size_buffer[16] = {0};
	     sd_memcpy(file_size_buffer, match_buffer_pos, match_buffer_len);
            file_size = em_filesize_str_to_u64(file_size_buffer);
        }
    }

	////////////////////////////////////////////////////
	// url下载链接等其他信息嗅探
	//////////////////////////////////////////////////
	
	    	// 如果没有开始串，则将整个网页作为搜索区间，执行一次匹配
    		if (detect_rule->_string_start[0] == '\0' || detect_rule->_string_end[0] == '\0') {
        		ret_val = em_detect_string_infos_from_string_except_name_and_size(webpage_content, detect_rule, detected_name, file_size, p_url_map);
    		}
    		if (detect_rule->_string_start[0] != '\0' && detect_rule->_string_end[0] != '\0') {
        		char *pos = NULL;
        		// 先查找搜索区间
        		_int32 length = em_get_string_between_strings(webpage_content, &pos, detect_rule->_string_start, detect_rule->_string_start_include, detect_rule->_string_end, detect_rule->_string_end_include);
        		if (length == -1) {//未找到搜索区间
            			EM_LOG_DEBUG("em_get_string_between_strings didn't find string!");
            			return -1;
        		}
        		while (length != -1) {
            			if (length > 0) {
                			char *match_area = NULL;
                			ret_val = sd_malloc(length+1, (void**)&match_area);
                			CHECK_VALUE(ret_val);
                			sd_memcpy(match_area, pos, length);
                			*(match_area+length)='\0';// 字符串结束
                
                			// 在其中进行嗅探匹配
        				ret_val = em_detect_string_infos_from_string_except_name_and_size(match_area, detect_rule, detected_name, file_size, p_url_map);
                
                			sd_free(match_area);
                			match_area = NULL;
            			}
		            // 从结尾开始找出下一个搜索区间
		            length = em_get_string_between_strings(pos+length, &pos, detect_rule->_string_start, detect_rule->_string_start_include, detect_rule->_string_end, detect_rule->_string_end_include);
        		}/*while(length != -1)*/
        }

	return SUCCESS;
}


// 根据规则中的匹配方案类型，分别调用不同的函数进行嗅探
_int32 em_detect_by_string_rule(const char* webpage_content, EM_DETECT_STRING* detect_rule, MAP* p_url_map)
{
    _int32 ret_val = 0;
    switch (detect_rule->_matching_scheme) {
        case 1:
            em_detect_with_string_matching_scheme_1(webpage_content, detect_rule, p_url_map);
            break;
        case 2:
            em_detect_with_string_matching_scheme_2(webpage_content, detect_rule, p_url_map);            
            break;
        case 3:
            em_detect_with_string_matching_scheme_3(webpage_content, detect_rule, p_url_map);            
            break;
        default:
            ret_val = -1;// 未找到对应匹配方案
            break;
    }
    return ret_val;
}

_int32 em_get_url_by_regex(const char* p_url_website, const char* webpage_content, LIST* regex_list, MAP * p_url_map)
{
	_int32 ret_val = -1;// 初始化为failed 作标志位使用

	// 若正则表达式列表指针为空，直接返回
	if(g_detect_regex == NULL ) return -1;

	// 若正则表达式列表内( 网站) 个数为0，则返回
	if(g_detect_regex->_list_size == 0) 
	{
		return -1;
	}

	// 否则需要先判断是否特殊嗅探的网站
	pLIST_NODE cur_node = NULL ,nxt_node = NULL ;
	pLIST_NODE cur_rule_node = NULL ,nxt_rule_node = NULL ;
	EM_DETECT_SPECIAL_WEB *p_detect_website = NULL;
	EM_DETECT_REGEX * p_detect_rule = NULL;

	cur_node = LIST_BEGIN(*g_detect_regex);
	while(cur_node!=LIST_END(*g_detect_regex))
	{
		nxt_node = LIST_NEXT(cur_node);

		p_detect_website = (EM_DETECT_SPECIAL_WEB *)LIST_VALUE(cur_node);

        char website[MAX_URL_LEN]={0};
        if(em_detect_regex_match_working(p_url_website, p_detect_website->_website, website) == 0)
		{
			// 如果找到对应的网站节点，则将返回标志置为SUCCESS，并跳出循环
			ret_val = SUCCESS;
			break;
		}

		cur_node = nxt_node;
	}

	if(ret_val != SUCCESS) return -1;// 如果ret_val不为SUCCESS，说明当前网站不在正则表达式列表中，直接返回

	// 根据正则表达式进行嗅探
	cur_rule_node = LIST_BEGIN(*(p_detect_website->_rule_list));
	while (cur_rule_node!=LIST_END(*(p_detect_website->_rule_list)))
	{
		nxt_rule_node = LIST_NEXT(cur_rule_node);

		p_detect_rule = (EM_DETECT_REGEX *)LIST_VALUE(cur_rule_node);

		// 真正的嗅探在这里!!!
		em_detect_by_regex_rule(webpage_content, p_detect_rule, p_url_map);

		cur_rule_node = nxt_rule_node;
	}
	
	
	return ret_val;
}

_int32 em_get_url_by_string(const char* p_url_website, const char* webpage_content, LIST* detect_string_list, MAP * p_url_map)
{
	_int32 ret_val = -1;// 初始化为failed 作标志位使用

	// 若正则表达式列表指针为空，直接返回
	if(g_detect_string== NULL ) return -1;

	// 若正则表达式列表内( 网站) 个数为0，则返回
	if(g_detect_string->_list_size == 0) 
	{
		return -1;
	}

	// 否则需要先判断是否特殊嗅探的网站
	pLIST_NODE cur_node = NULL ,nxt_node = NULL ;
	pLIST_NODE cur_rule_node = NULL ,nxt_rule_node = NULL ;
	EM_DETECT_SPECIAL_WEB *p_detect_website = NULL;
	EM_DETECT_STRING* p_detect_rule = NULL;

	cur_node = LIST_BEGIN(*g_detect_string);
	while(cur_node!=LIST_END(*g_detect_string))
	{
		nxt_node = LIST_NEXT(cur_node);

		p_detect_website = (EM_DETECT_SPECIAL_WEB *)LIST_VALUE(cur_node);

        	if(sd_strstr(p_url_website, p_detect_website->_website, 0) != NULL)
		{
			// 如果找到对应的网站节点，则将返回标志置为SUCCESS，并跳出循环
			ret_val = SUCCESS;
			break;
		}

		cur_node = nxt_node;
	}

	if(ret_val != SUCCESS) return -1;// 如果ret_val不为SUCCESS，说明当前网站不在正则表达式列表中，直接返回

	// 根据正则表达式进行嗅探
	cur_rule_node = LIST_BEGIN(*(p_detect_website->_rule_list));
	while (cur_rule_node!=LIST_END(*(p_detect_website->_rule_list)))
	{
		nxt_rule_node = LIST_NEXT(cur_rule_node);

		p_detect_rule = (EM_DETECT_STRING*)LIST_VALUE(cur_rule_node);

		// 真正的嗅探在这里!!!
		em_detect_by_string_rule(webpage_content, p_detect_rule, p_url_map);

		cur_rule_node = nxt_rule_node;
	}
	
	
	return ret_val;

}

_int32 em_get_url_from_html(const char* url, const char* webpage_content, EM_SPECIAL_WEB web_id, MAP * p_url_map )
{
	_int32 ret_val = SUCCESS;
	_u32 hash_value = 0;
	const char *header="ref=\"";// 避免为了区分“Href”和“href”要进行两次循环，只查找“ref”
	char *header_pos = NULL,*end_pos = NULL;
	char url_buffer[MAX_URL_LEN] = {0};
//	EM_DOWNLOADABLE_URL * p_dl_url = NULL;
	EM_URL_TYPE url_type;

	header_pos = sd_strstr((char*)webpage_content,header, 0);
	while(header_pos!=NULL)
	{
		/*找到url */
        header_pos += sd_strlen(header); 
		end_pos = sd_strchr((char*)header_pos, '\"', 0);
		if(end_pos==NULL) return SUCCESS;

		if(end_pos-header_pos>=MAX_URL_LEN) 
		{
			//url 太长了，处理不了，直接丢弃，接着处理下一个 
			header_pos = sd_strstr(end_pos,header, 0);
			continue;
		}
		
		sd_memset(url_buffer,0x00,MAX_URL_LEN);
		sd_strncpy(url_buffer, header_pos, end_pos-header_pos);// url_buffer保存了截取出来的链接
    		EM_LOG_DEBUG( "em_get_url_from_html:[%u]%s",end_pos-header_pos,url_buffer);

		/*链接类型判断*/
		url_type = em_get_url_type(url_buffer);
        EM_LOG_DEBUG("em_get_url_from_html:url_type:%d", url_type);
		if(url_type != UT_NORMAL)
		{
			// 所有发现的迅雷大全客户端EXE链接将直接被舍弃(IOS不支持该类型)
			if(sd_strncmp(url_buffer, "http://down.sandai.net/xldaquan/", sd_strlen("http://down.sandai.net/xldaquan/"))==0)
			{
				char *suffix_pos = sd_strrchr(url_buffer, '.');
				if(suffix_pos != NULL && sd_strcmp(suffix_pos, ".exe")==0)
				{
					header_pos = sd_strstr(end_pos,header, 0);
					continue;
				}
			}
			
			/*申请空间用于保存解析结果*/
			ret_val = sd_malloc(sizeof(EM_DOWNLOADABLE_URL), (void**)&g_dl_url);
			if(ret_val!=SUCCESS) return SUCCESS;
			sd_memset(g_dl_url,0x00,sizeof(EM_DOWNLOADABLE_URL));
			sd_strncpy(g_dl_url->_url, header_pos, end_pos-header_pos);

			/*url小处理*/
			if(url_type==UT_EMULE&&(sd_strncmp(g_dl_url->_url, "ed2k://%7", sd_strlen("ed2k://%7"))==0))
			{
				em_replace_7c(g_dl_url->_url);// 对于电驴链接，将其中的"%7c"换成"|"
			}
			else if(url_type == UT_HTTP_PARTIAL || url_type == UT_BT_PARTIAL)
			{
                            // 对于不完整 的链接，现将其拼接成完整链接
                            char *jointed_url = em_joint_url(url, url_buffer);
				sd_snprintf(g_dl_url->_url, MAX_URL_LEN-1, jointed_url);
                            sd_memset(url_buffer, 0, MAX_URL_LEN);
                            sd_memcpy(url_buffer, g_dl_url->_url, sd_strlen(g_dl_url->_url));
                             // 相应的将链接类型也改成UT_HTTP 和UT_BT
                            if (url_type == UT_HTTP_PARTIAL) {
                                     url_type = UT_HTTP;
                            }
                            else if (url_type == UT_BT_PARTIAL) {
                                      url_type = UT_BT;
                            }

			}

			/*解析文件名并得到url 哈希值*/
			ret_val = em_parse_file_name_and_hash_value_from_url_new(g_dl_url->_url,url_type,g_dl_url, &hash_value);
			if(ret_val==SUCCESS /* &&(p_dl_url->_type==UT_THUNDER)&&(web_id==ESW_XIAZAI_XUNLEI) */)
			{
				if(web_id==ESW_DAQUAN_XUNLEI&&header_pos-webpage_content>MAX_FILE_NAME_BUFFER_LEN)
				{
					em_get_xunlei_daquan_file_name(header_pos,end_pos,g_dl_url->_name);
				}
				else
				if(g_dl_url->_type==UT_THUNDER)
				{
					if(web_id==ESW_ZOL)
						em_get_zol_file_name(webpage_content, g_dl_url->_name);
					else
						em_get_xunlei_xiazai_file_name(end_pos,g_dl_url->_name);
				}
				else
				{
					if((header_pos-webpage_content>9)&&(sd_strncmp(header_pos-9, "<a href=\"",sd_strlen("<a href=\""))==0))
					{
						/* <a href="#/URL">name</a> */
						/*ret_val = *///em_get_href_file_name(end_pos,p_dl_url->_name);
						//sd_assert(ret_val==SUCCESS);
					}
				}
			}
 			    
                 // 文件名以.exe结尾，则过滤掉。这种资源在移动端不适用 
				char *filename_suffix_pos = sd_strrchr(g_dl_url->_name, '.');
				if(filename_suffix_pos != NULL && sd_strcmp(filename_suffix_pos, ".exe")==0)
				{
					header_pos = sd_strstr(end_pos,header, 0);
					continue;
				}

   			EM_LOG_DEBUG( "em_parse_file_name_and_hash_value_from_url:ret=%d,type=%s,hash=%u,fsize=%llu,name=%s",ret_val,(char*)(g_url_headers[g_dl_url->_type]),hash_value,g_dl_url->_file_size,g_dl_url->_name);
			if(ret_val ==SUCCESS)
			{
				ret_val = em_add_url_to_map(p_url_map, hash_value, g_dl_url);
				if(ret_val!=SUCCESS) 
				{
    					EM_LOG_ERROR( "em_get_url_from_html:em_add_url_to_map ERROR!ret_val=%d",ret_val);
					SAFE_DELETE(g_dl_url);
				}
			}
			else
			{
				SAFE_DELETE(g_dl_url);
			}
		}
		header_pos = sd_strstr(end_pos,header, 0);
	}

	
	return ret_val;
}

_int32 em_get_url_from_webpage(const char* webpage_content,EM_SPECIAL_WEB web_id,EM_URL_TYPE url_type,MAP * p_url_map )
{
	_int32 ret_val = SUCCESS;
	_u32 hash_value = 0;
	char *header=NULL,*header_pos = NULL,*end_pos = NULL;
	char url_buffer[MAX_URL_LEN] = {0};
	EM_DOWNLOADABLE_URL * p_dl_url = NULL;

	header = (char*)(g_url_headers[url_type]);

	header_pos = sd_strstr((char*)webpage_content, header, 0);
	while(header_pos!=NULL)
	{
		end_pos = em_get_url_end_pos(header_pos,url_type);
		if(end_pos==NULL) return SUCCESS;

		if(end_pos-header_pos>=MAX_URL_LEN) 
		{
			/* url 太长了，处理不了，直接丢弃，接着处理下一个 */
			header_pos = sd_strstr(end_pos,header, 0);
			continue;
		}
		sd_memset(url_buffer,0x00,MAX_URL_LEN);
		sd_strncpy(url_buffer, header_pos, end_pos-header_pos);// url_buffer保存了截取出来的链接
    		EM_LOG_DEBUG( "em_get_url_from_webpage:[%u]%s",end_pos-header_pos,url_buffer);
		if(em_is_legal_url(url_buffer,url_type)&&em_is_downloadable_url(url_buffer,url_type))
		{
			// 所有发现的迅雷大全客户端EXE链接将直接被舍弃(IOS不支持该类型)
			if(sd_strncmp(url_buffer, "http://down.sandai.net/xldaquan/", sd_strlen("http://down.sandai.net/xldaquan/"))==0)
			{
				char *suffix_pos = sd_strrchr(url_buffer, '.');
				if(suffix_pos != NULL && sd_strcmp(suffix_pos, ".exe")==0)
				{
					header_pos = sd_strstr(end_pos,header, 0);
					continue;
				}
			}
            // 发现可下载链接后，申请空间用于存储链接信息
			ret_val = sd_malloc(sizeof(EM_DOWNLOADABLE_URL), (void**)&p_dl_url);
			if(ret_val!=SUCCESS) return SUCCESS;
			sd_memset(p_dl_url,0x00,sizeof(EM_DOWNLOADABLE_URL));
			sd_strncpy(p_dl_url->_url, header_pos, end_pos-header_pos);// p_dl_url保存url
			if(url_type==UT_EMULE&&(sd_strncmp(p_dl_url->_url, "ed2k://%7", sd_strlen("ed2k://%7"))==0))
			{
				em_replace_7c(p_dl_url->_url);// 对于电驴链接，将其中的"%7c"换成"|"
			}
            // 解析文件名，p_dl_url保存name/type/file_type/url_id
			ret_val = em_parse_file_name_and_hash_value_from_url(p_dl_url->_url,url_type,p_dl_url, &hash_value);
			if(ret_val==SUCCESS /* &&(p_dl_url->_type==UT_THUNDER)&&(web_id==ESW_XIAZAI_XUNLEI) */)
			{
				if(web_id==ESW_DAQUAN_XUNLEI&&header_pos-webpage_content>MAX_FILE_NAME_BUFFER_LEN)
				{
					em_get_xunlei_daquan_file_name(header_pos,end_pos,p_dl_url->_name);
				}
				else
				if(p_dl_url->_type==UT_THUNDER)
				{
					if(web_id==ESW_ZOL)
						em_get_zol_file_name(webpage_content, p_dl_url->_name);
					else
						em_get_xunlei_xiazai_file_name(end_pos,p_dl_url->_name);
				}
				else
				{
					if((header_pos-webpage_content>9)&&(sd_strncmp(header_pos-9, "<a href=\"",sd_strlen("<a href=\""))==0))
					{
						/* <a href="#/URL">name</a> */
						/*ret_val = *///em_get_href_file_name(end_pos,p_dl_url->_name);
						//sd_assert(ret_val==SUCCESS);
					}
				}
			}
			
			    // 文件名以.exe结尾，则过滤掉。这种资源在移动端不适用 
				char *filename_suffix_pos = sd_strrchr(p_dl_url->_name, '.');
				if(filename_suffix_pos != NULL && sd_strcmp(filename_suffix_pos, ".exe")==0)
				{
					header_pos = sd_strstr(end_pos,header, 0);
					continue;
				}

    			EM_LOG_DEBUG( "em_parse_file_name_and_hash_value_from_url:ret=%d,type=%s,hash=%u,fsize=%llu,name=%s",ret_val,(char*)(g_url_headers[p_dl_url->_type]),hash_value,p_dl_url->_file_size,p_dl_url->_name);
			if(ret_val ==SUCCESS)
			{
                // 将url插入map中
				ret_val = em_add_url_to_map(p_url_map, hash_value, p_dl_url);
				if(ret_val!=SUCCESS) 
				{
    					EM_LOG_ERROR( "em_get_url_from_webpage:em_add_url_to_map ERROR!ret_val=%d",ret_val);
					SAFE_DELETE(p_dl_url);
				}
			}
			else
			{
				SAFE_DELETE(p_dl_url);
			}
		}
		header_pos = sd_strstr(end_pos,header, 0);
	}

	
	return ret_val;
}

_int32 em_get_url_of_ffdy(const char* webpage_content, MAP * p_url_map )
{
	_int32 ret_val = SUCCESS;
	_u32 hash_value = 0;
	const char *header="ftp://";// ffdy网站上的链接都是以ftp开头的
	char *header_pos = NULL,*end_pos = NULL;
	char url_buffer[MAX_URL_LEN] = {0};
//	EM_DOWNLOADABLE_URL * p_dl_url = NULL;
	EM_URL_TYPE url_type;

	header_pos = sd_strstr((char*)webpage_content,header, 0);
	while(header_pos!=NULL)
	{
		/*找到url */
		end_pos = sd_strchr((char*)header_pos, '&', 0);
		if(end_pos==NULL) return SUCCESS;

		if(end_pos-header_pos>=MAX_URL_LEN) 
		{
			//url 太长了，处理不了，直接丢弃，接着处理下一个 
			header_pos = sd_strstr(end_pos,header, 0);
			continue;
		}
		
		sd_memset(url_buffer,0x00,MAX_URL_LEN);
		sd_strncpy(url_buffer, header_pos, end_pos-header_pos);// url_buffer保存了截取出来的链接
    		EM_LOG_DEBUG( "em_get_url_of_ffdy:[%u]%s",end_pos-header_pos,url_buffer);

		/*链接类型判断*/
		url_type = em_get_url_type(url_buffer);
        	EM_LOG_DEBUG("em_get_url_of_ffdy:url_type:%d", url_type);
		if(url_type != UT_NORMAL)
		{
			/*申请空间用于保存解析结果*/
			ret_val = sd_malloc(sizeof(EM_DOWNLOADABLE_URL), (void**)&g_dl_url);
			if(ret_val!=SUCCESS) return SUCCESS;
			sd_memset(g_dl_url,0x00,sizeof(EM_DOWNLOADABLE_URL));
			sd_strncpy(g_dl_url->_url, header_pos, end_pos-header_pos);

			/*url小处理*/
			if(url_type == UT_HTTP_PARTIAL || url_type == UT_BT_PARTIAL)
			{
               		 // 对于不完整 的链接，现将其拼接成完整链接
                            char *jointed_url = em_joint_url(g_url, url_buffer);
				sd_snprintf(g_dl_url->_url, MAX_URL_LEN-1, jointed_url);
                            sd_memset(url_buffer, 0, MAX_URL_LEN);
                            sd_memcpy(url_buffer, g_dl_url->_url, sd_strlen(g_dl_url->_url));
               	 	// 相应的将链接类型也改成UT_HTTP 和UT_BT
              		if (url_type == UT_HTTP_PARTIAL) {
                    			url_type = UT_HTTP;
                		}
                		else if (url_type == UT_BT_PARTIAL) {
                    			url_type = UT_BT;
                		}
			}

			/*解析文件名并得到url 哈希值*/
			ret_val = em_parse_file_name_and_hash_value_from_url_new(g_dl_url->_url,url_type,g_dl_url, &hash_value);
    			EM_LOG_DEBUG( "em_parse_file_name_and_hash_value_from_url:ret=%d,type=%s,hash=%u,fsize=%llu,name=%s",ret_val,(char*)(g_url_headers[g_dl_url->_type]),hash_value,g_dl_url->_file_size,g_dl_url->_name);
			if(ret_val ==SUCCESS)
			{
				ret_val = em_add_url_to_map(p_url_map, hash_value, g_dl_url);
				if(ret_val!=SUCCESS) 
				{
    					EM_LOG_ERROR( "em_get_url_from_webpage:em_add_url_to_map ERROR!ret_val=%d",ret_val);
					SAFE_DELETE(g_dl_url);
				}
			}
			else
			{
				SAFE_DELETE(g_dl_url);
			}
		}
		header_pos = sd_strstr(end_pos,header, 0);
	}

	
	return ret_val;

}

_int32 em_get_digit_from_str(const char *str)
{
	_int32 ret_val = 0,len = 0;
	
	while('0'<=*str&&'9'>=*str)
	{
		ret_val=ret_val*10+(*str-'0');
		str++;
		len++;
		if(len>9)
			break;
	}

	return ret_val;
}
_int32 em_filename_cmp(const char *s1, const char *s2)
{
	while((*s1!='\0')&&(*s1==*s2))
	{
		s1++;
		s2++;
	}

	if(*s1==*s2)
		return 0;
	else
	if(*s2=='\0')
	{
		return 1;
	}
	else
	{
		/* 不相等，是否数字? */
		if('0'<=*s1&&'9'>=*s1)
		{
			/* *s1 是数字 */
			if('0'>*s2||'9'<*s2)
			{
				/* *s2 不是数字 */
				return 1;
			}
			else
			{
				/* *s2 也是数字 */
				_int32 d1 = em_get_digit_from_str(s1);
				_int32 d2 = em_get_digit_from_str(s2);
				return (d1-d2);
			}
		}
		else
		{
			/* *s1 不是数字 */
			if('0'>*s2||'9'<*s2)
			{
				/* *s2 也不是数字 */
				return (*s1-*s2);
			}
			else
			{
				/* *s2 是数字 */
				return -1;
			}
		
		}
	}
	return 0;	
}
EM_DOWNLOADABLE_URL * em_excise_biggest_value_node_from_list(LIST * p_list)
{
	pLIST_NODE cur_list_node = NULL ,p_bigest_list_node = NULL ;
	EM_DOWNLOADABLE_URL *p_dl_url = NULL;
	char * p_bigest_name = NULL;

	if(list_size(p_list)==0) return NULL;
		
	for(cur_list_node = LIST_BEGIN(*p_list);cur_list_node!=LIST_END(*p_list);cur_list_node = LIST_NEXT(cur_list_node))
	{
		p_dl_url = (EM_DOWNLOADABLE_URL *)LIST_VALUE(cur_list_node);

		if(p_bigest_name==NULL)
		{
			p_bigest_name = p_dl_url->_name;
			p_bigest_list_node = cur_list_node;
		}
		else
		{
	            _u32 len_bigest = sd_strlen(p_bigest_name);
       	     _u32 len_currnet = sd_strlen(p_dl_url->_name);
            
            		if( len_bigest == len_currnet)
            		{
            			// 如果长度相等，数字大的文件名较大
                		if(em_filename_cmp(p_bigest_name, p_dl_url->_name)<0)
                		{
                    			p_bigest_name = p_dl_url->_name;
                    			p_bigest_list_node = cur_list_node;
                		}
            		}
            		else if( len_bigest < len_currnet)
            		{
            			// 否则长度较大的文件名较大
                		p_bigest_name = p_dl_url->_name;
                		p_bigest_list_node = cur_list_node;
            		}
            }
	}

	if(p_bigest_list_node!=NULL)
	{
		p_dl_url = (EM_DOWNLOADABLE_URL *)LIST_VALUE(p_bigest_list_node);
		list_erase(p_list, p_bigest_list_node);
	}
	
	return p_dl_url;	
}
_u32 em_copy_downloadable_url_to_array(MAP * p_map,EM_DOWNLOADABLE_URL *p_array)
{
	_int32 ret_val = SUCCESS,index = 0;
	LIST list_video,list_audio,list_emule,list_magnet,list_torrent,list_txt,list_zip,list_app,list_other,*list_array[9];
      MAP_ITERATOR  cur_node = NULL; 
	EM_DOWNLOADABLE_URL *p_dl_url = NULL;
	_u32 valid_num = 0,total_num = map_size(p_map);

	list_array[0] = &list_video;
	list_array[1] = &list_audio;
	list_array[2] = &list_emule;
	list_array[3] = &list_magnet;
	list_array[4] = &list_torrent;
	list_array[5] = &list_txt;
	list_array[6] = &list_zip;
	list_array[7] = &list_app;
	list_array[8] = &list_other;

	for(index = 0;index<9;index++)
	{
		list_init(list_array[index]);
	}

      cur_node = EM_MAP_BEGIN(*p_map);
      while(cur_node != EM_MAP_END(*p_map))
      {
      		/* 按文件类型排序 */
             p_dl_url = (EM_DOWNLOADABLE_URL *)EM_MAP_VALUE(cur_node);
		switch(p_dl_url->_file_type)
		{
			case 	EFT_VEDIO:
				ret_val = list_push(&list_video, p_dl_url);
			break;
			case 	EFT_AUDIO:
				ret_val = list_push(&list_audio, p_dl_url);
			break;
			case 	EFT_TXT:
				ret_val = list_push(&list_txt, p_dl_url);
			break;
			case 	EFT_ZIP:
				ret_val = list_push(&list_zip, p_dl_url);
			break;
			case 	EFT_APP:
				ret_val = list_push(&list_app, p_dl_url);
			break;
 			case 	EFT_UNKNOWN: 				
			case 	EFT_PIC:
			case 	EFT_OTHER:
			{
				/* 不知道文件类型，按URL类型排序 */
				if(p_dl_url->_type == UT_EMULE)
					ret_val = list_push(&list_emule, p_dl_url);
				else
				if(p_dl_url->_type == UT_MAGNET)
					ret_val = list_push(&list_magnet, p_dl_url);
				else
				if(p_dl_url->_type == UT_BT)
					ret_val = list_push(&list_torrent, p_dl_url);
				else
					ret_val = list_push(&list_other, p_dl_url);
			}
			break;
		}
		sd_assert(ret_val==SUCCESS);
		if(ret_val!=SUCCESS)
		{
			SAFE_DELETE(p_dl_url);
		}
		em_map_erase_iterator(p_map, cur_node);
	      cur_node = EM_MAP_BEGIN(*p_map);
      }
	  
	sd_assert(map_size(p_map)==0);
	
	for(index = 0;index<9;index++)
	{
		while(list_size(list_array[index])!=0) 
		{
			/* 按集数由大到小排序 */
			p_dl_url = em_excise_biggest_value_node_from_list(list_array[index]);
			sd_assert(p_dl_url!=NULL);
			sd_memcpy(p_array,p_dl_url, sizeof(EM_DOWNLOADABLE_URL));
		  	p_array++;
			valid_num++;
			SAFE_DELETE(p_dl_url);
		}
	}
	
	sd_assert(valid_num==total_num);
	
	return valid_num;
}

_int32 em_get_downloadable_url_from_webpage(const char* webpage_file_path,char* url,_u32 * p_downloadable_num,EM_DOWNLOADABLE_URL ** pp_downloadable )
{
	_int32 ret_val = SUCCESS;
	_u32 file_id = 0,read_size = 0;
	_u64 filesize = 0;
	char * file_content = NULL;
	MAP url_map;
	EM_URL_TYPE url_type;
      MAP_ITERATOR  cur_node = NULL; 
	EM_DOWNLOADABLE_URL *p_dl_url = NULL,*p_dl_url_t = NULL;
	BOOL is_unziped = FALSE;
	EM_SPECIAL_WEB web_id = ESW_NOT;
	char *pos = NULL;

    	EM_LOG_DEBUG( "em_get_downloadable_url_from_webpage start:%s",webpage_file_path);
	map_init(&url_map,em_url_key_comp);
	
READ_AGAIN:
	ret_val = sd_open_ex(webpage_file_path,O_FS_RDONLY, &file_id);
	CHECK_VALUE(ret_val);

	ret_val = sd_filesize(file_id, &filesize);
	if(ret_val!=SUCCESS)
	{
		sd_close_ex(file_id);
		CHECK_VALUE(ret_val);
	}

	if(filesize==0)
	{
		sd_close_ex(file_id);
    		EM_LOG_ERROR( "em_get_downloadable_url_from_webpage end:filesize=%u",filesize);
		return EM_INVALID_FILE_SIZE;
	}
	
	ret_val = sd_malloc(filesize+1,(void**)&file_content);
	if(ret_val!=SUCCESS)
	{
		sd_close_ex(file_id);
		CHECK_VALUE(ret_val);
	}
	sd_memset(file_content,0x00,filesize+1);
	
	ret_val = sd_read(file_id,file_content,filesize,&read_size);
	sd_close_ex(file_id);
	if(ret_val!=SUCCESS)
	{
		SAFE_DELETE(file_content);
		CHECK_VALUE(ret_val);
	}

	if(NULL==sd_strstr(file_content,"<",0)) // sd_strncmp("<!",file_content, sd_strlen("<!"))!=0)
	{
    		EM_LOG_ERROR( "em_get_downloadable_url_from_webpage end:NOT a html file=%s",file_content);
		SAFE_DELETE(file_content);
		if(!is_unziped)
		{
			/* 有可能是压缩文档，尝试解压 */
#ifdef ENABLE_ZIP
			ret_val=sd_unzip_file(webpage_file_path);
#else
			ret_val = -1;
#endif
			if(ret_val!=SUCCESS) return ret_val;
			is_unziped = TRUE;
			goto READ_AGAIN;
		}
		CHECK_VALUE(INVALID_EIGENVALUE);
	}

	// 获取url 网站域名
	sd_memset(g_url_website, 0, 64);
	pos = sd_strchr(url, '/', sd_strlen("http://"));
	sd_strncpy((char*)g_url_website, url, pos-url);

	// 保存url网址
	sd_memset(g_url, 0, MAX_URL_LEN);
	sd_strncpy(g_url, url, sd_strlen(url));

	_int32 enable_detect_count = 0;// 开启的定制嗅探数量
	ret_val = 0;// 先将返回标识设为0
#ifdef ENABLE_REGEX_DETECT
       // 通过正则表达式进行嗅探
	enable_detect_count ++;
	ret_val += em_get_url_by_regex((const char*)url, file_content, g_detect_regex, &url_map);

	#ifdef _DEBUG
		printf("em_get_url_by_regex return:%d url:%s\n", ret_val, url);
	#endif

#endif/*ENABLE_REGEX_DETECT*/

#ifdef ENABLE_STRING_DETECT
 	enable_detect_count ++;
      // 通过正则表达式进行嗅探
	ret_val += em_get_url_by_string((const char*)url, file_content, g_detect_string, &url_map);

	#ifdef _DEBUG
		printf("em_get_url_by_string return:%d url:%s\n", ret_val, url);
	#endif

#endif/*ENABLE_STRING_DETECT*/

	// 由于定制嗅探都是是精确嗅探，所以嗅探成功的可以直接返回
	//(也可以通过这种方式来屏蔽一些不需要嗅探的网站)

	if((-1*ret_val) == enable_detect_count)// 所有定制嗅探都不成功(返回定制嗅探不成功的计数等于开启的定制嗅探数)，则进入常规嗅探
	{
		// 常规嗅探
		web_id = em_get_special_website_id( url);

		if((web_id == ESW_NOT)||(web_id == ESW_XIAZAI_XUNLEI)||(web_id == ESW_DAQUAN_XUNLEI)||(web_id == ESW_ZOL))
		{
			 /*普通网站和需要常规查找的特殊网站*/
			EM_LOG_DEBUG("em_get_url_from_webpage start:%s", url);
   			for(url_type=UT_HTTP;url_type<HEADERS_SIZE;url_type++)
    			{
        			em_get_url_from_webpage(file_content,web_id,url_type,&url_map );
    			}
    			EM_LOG_DEBUG("em_get_url_from_webpage end");

    			EM_LOG_DEBUG("em_get_url_from_html start:%s", url);
				em_get_url_from_html(url, file_content, web_id, &url_map);
    			EM_LOG_DEBUG("em_get_url_from_html end");
		}
		else    /*特殊网站，不经过常规查找*/
		if(web_id == ESW_FFDY)
		{
			em_get_url_of_ffdy(file_content, &url_map);
		}
		else
		if(web_id == ESW_ISHARE_SINA)
		{
			// 新浪共享由于全部采用php链接，暂时无法嗅探
		}
	}
	
	SAFE_DELETE(file_content);


	/*最后将嗅探结果复制到输出指针所指的位置*/
	*p_downloadable_num = map_size(&url_map);
	if(*p_downloadable_num==0)
	{
    		EM_LOG_ERROR( "em_get_downloadable_url_from_webpage end:downloadable url NOT found!");
		return SUCCESS;
	}

	ret_val = sd_malloc((*p_downloadable_num)*sizeof(EM_DOWNLOADABLE_URL),(void**)pp_downloadable);
	if(ret_val!=SUCCESS)
	{
		*p_downloadable_num = 0;
		CHECK_VALUE(ret_val);
	}
	sd_memset(*pp_downloadable,0x00,(*p_downloadable_num)*sizeof(EM_DOWNLOADABLE_URL));
	
	p_dl_url_t = *pp_downloadable;

	if(*p_downloadable_num==1)
	{
		cur_node = EM_MAP_BEGIN(url_map);
		p_dl_url = (EM_DOWNLOADABLE_URL *)EM_MAP_VALUE(cur_node);
		sd_memcpy(p_dl_url_t,p_dl_url, sizeof(EM_DOWNLOADABLE_URL));
	      SAFE_DELETE(p_dl_url);
		em_map_erase_iterator(&url_map, cur_node);
	}
	else
	{
		*p_downloadable_num =em_copy_downloadable_url_to_array(&url_map,p_dl_url_t);
	}
	
	sd_assert(map_size(&url_map)==0);
	
    	EM_LOG_DEBUG( "em_get_downloadable_url_from_webpage end:downloadable_num=%u",*p_downloadable_num);
	return ret_val;
}
_int32 em_free_detect_regex()
{
	// 若正则表达式列表指针为空，直接返回成功
	if(g_detect_regex == NULL ) return SUCCESS;

	// 若正则表达式列表内( 网站) 个数为0，则释放该指针并返回成功
	if(g_detect_regex->_list_size == 0) 
	{
		SAFE_DELETE(g_detect_regex);
		return SUCCESS;
	}

	// 否则( 有需要特殊嗅探的网站) 要分别释放每个网站下的嗅探规则表达式再释放网站列表
	pLIST_NODE cur_node = NULL ,nxt_node = NULL ;
	pLIST_NODE cur_rule_node = NULL ,nxt_rule_node = NULL ;
	EM_DETECT_SPECIAL_WEB *p_detect_website = NULL;
	EM_DETECT_REGEX * p_detect_rule = NULL;

	cur_node = LIST_BEGIN(*g_detect_regex);
	while(cur_node!=LIST_END(*g_detect_regex))
	{
		nxt_node = LIST_NEXT(cur_node);

		p_detect_website = (EM_DETECT_SPECIAL_WEB *)LIST_VALUE(cur_node);

		if(p_detect_website->_rule_list == NULL)
		{
			SAFE_DELETE(p_detect_website);
			continue;
		}

		if(list_size(p_detect_website->_rule_list) == 0)
		{
			SAFE_DELETE(p_detect_website->_rule_list);
			SAFE_DELETE(p_detect_website);
			continue;
		}

		cur_rule_node = LIST_BEGIN(*(p_detect_website->_rule_list));
		while (cur_rule_node!=LIST_END(*(p_detect_website->_rule_list)))
		{
			nxt_rule_node = LIST_NEXT(cur_rule_node);

			p_detect_rule = (EM_DETECT_REGEX *)LIST_VALUE(cur_rule_node);

			SAFE_DELETE(p_detect_rule);

			cur_rule_node = nxt_rule_node;
		}

		cur_node = nxt_node;
	}

	SAFE_DELETE(g_detect_regex);// 别忘了还得把正则表达式列表也释放了才返回
	
	return SUCCESS;
}

_int32 em_set_detect_regex(LIST* regex_list)
{
	//先释放之前的表达式列表
	em_free_detect_regex();

	//再保存新的列表
	if(regex_list != NULL)
		g_detect_regex = regex_list;
	
	return SUCCESS;
}

_int32 em_free_detect_string()
{
	// 若正则表达式列表指针为空，直接返回成功
	if(g_detect_string== NULL ) return SUCCESS;

	// 若正则表达式列表内( 网站) 个数为0，则释放该指针并返回成功
	if(g_detect_string->_list_size == 0) 
	{
		SAFE_DELETE(g_detect_string);
		return SUCCESS;
	}

	// 否则( 有需要特殊嗅探的网站) 要分别释放每个网站下的嗅探规则表达式再释放网站列表
	pLIST_NODE cur_node = NULL ,nxt_node = NULL ;
	pLIST_NODE cur_rule_node = NULL ,nxt_rule_node = NULL ;
	EM_DETECT_SPECIAL_WEB *p_detect_website = NULL;
	EM_DETECT_STRING* p_detect_rule = NULL;

	cur_node = LIST_BEGIN(*g_detect_string);
	while(cur_node!=LIST_END(*g_detect_string))
	{
		nxt_node = LIST_NEXT(cur_node);

		p_detect_website = (EM_DETECT_SPECIAL_WEB *)LIST_VALUE(cur_node);

		if(p_detect_website->_rule_list == NULL)
		{
			SAFE_DELETE(p_detect_website);
			continue;
		}

		if(list_size(p_detect_website->_rule_list) == 0)
		{
			SAFE_DELETE(p_detect_website->_rule_list);
			SAFE_DELETE(p_detect_website);
			continue;
		}

		cur_rule_node = LIST_BEGIN(*(p_detect_website->_rule_list));
		while (cur_rule_node!=LIST_END(*(p_detect_website->_rule_list)))
		{
			nxt_rule_node = LIST_NEXT(cur_rule_node);

			p_detect_rule = (EM_DETECT_STRING*)LIST_VALUE(cur_rule_node);

			SAFE_DELETE(p_detect_rule);

			cur_rule_node = nxt_rule_node;
		}

		cur_node = nxt_node;
	}

	SAFE_DELETE(g_detect_string);// 别忘了还得把全局指针也释放了才返回
	
	return SUCCESS;
}

_int32 em_set_detect_string(LIST* detect_string_list)
{
	//先释放之前的表达式列表
	em_free_detect_string();

	//再保存新的列表
	if(detect_string_list != NULL)
		g_detect_string = detect_string_list;
	
	return SUCCESS;
}

