#if !defined(__BASE64_C_20081021)
#define __BASE64_C_20081021

#include "utility/base64.h"
#include "utility/string.h"

#include "utility/logid.h"
#ifdef LOGID
#undef LOGID
#endif

#define LOGID LOGID_COMMON

#include "utility/logger.h"

#define 	BASE16_LOOKUP_MAX 	23
static _u8 base16_lookup[BASE16_LOOKUP_MAX][2] = {
    { '0', 0x0 },
    { '1', 0x1 },
    { '2', 0x2 },
    { '3', 0x3 },
    { '4', 0x4 },
    { '5', 0x5 },
    { '6', 0x6 },
    { '7', 0x7 },
    { '8', 0x8 },
    { '9', 0x9 },
    { ':', 0x9 },
    { ';', 0x9 },
    { '<', 0x9 },
    { '=', 0x9 },
    { '>', 0x9 },
    { '?', 0x9 },
    { '@', 0x9 },
    { 'A', 0xA },
    { 'B', 0xB },
    { 'C', 0xC },
    { 'D', 0xD },
    { 'E', 0xE },
    { 'F', 0xF }
};
    
_int32 sd_decode_base16(const char* input, _u32 input_len, char* output, _u32 output_len)
{
	_u32 i = 0;
	_int32 lookup = 0;
	_u8 word = 0;
	if(output_len < DECODE_LENGTH_BASE16(input_len))
		return -1;		//output长度不够
	for(i = 0; i < input_len; ++i)
	{
		lookup = sd_toupper(input[i]) - '0';
		//Check to make sure that the given word falls inside a valid range
		if(lookup < 0 || lookup >= BASE16_LOOKUP_MAX)
			return -1;
		else
			word = base16_lookup[lookup][1];
		if(i % 2 == 0)
			output[i / 2] = word << 4;
		else
			output[(i - 1) / 2] |= word;
	}
	return SUCCESS;
}

_int32 sd_decode_base32(const char* input, _u32 input_len, char* output, _u32 output_len)
{
	_u32 bits = 0;
	_int32 count = 0, i = 0;
	_int32 chars = 0;
	if(output_len < DECODE_LENGTH_BASE32(input_len))
		return -1;
	for(chars = input_len; chars--; input++)
	{
		if(*input >= 'A' && *input <= 'Z')
			bits |= ( *input - 'A' );
		else if ( *input >= 'a' && *input <= 'z' )
			bits |= ( *input - 'a' );
		else if ( *input >= '2' && *input <= '7' )
			bits |= ( *input - '2' + 26 );
		else
			return -1;		
		count += 5;
		if(count >= 8)
		{
			output[i] = (char)(bits >> ( count - 8 ));
			count -= 8;
			++i;
		}
		bits <<= 5;
	}
	if(DECODE_LENGTH_BASE32(input_len) == i)
		return SUCCESS;
	else
		return -1;
}

/* Convert BASE64  to char string
 * Make sure the length of bufer:d larger than 3/4 length of source string:s 
 * Returns: 0:success, -1:failed
 */
_int32 sd_base64_decode(const char *s,_u8 *d,int * output_size)
{
        char base64_table[255],s_buffer[MAX_URL_LEN],*p_s=s_buffer;
        _int32 clen,len,i,j;
	_u8 *d_base = d;
 	LOG_DEBUG( "eui_sd_base64_decode" );
	
       if((s==NULL)||(sd_strlen(s)>=MAX_URL_LEN))
                return -1;

	 sd_memset(base64_table,0,255);
	 sd_memset(p_s,0,MAX_URL_LEN);
	 
        for(j=0,i='A';i<='Z';i++) 
                base64_table[i]=j++;
        for(i='a';i<='z';i++)
                base64_table[i]=j++;
        for(i='0';i<='9';i++)
                base64_table[i]=j++;
        base64_table['+']=j++;
        base64_table['/']=j++;
        base64_table['=']=j;

	sd_strncpy(p_s,s,MAX_URL_LEN);
        len=sd_strlen(p_s);
        if(p_s[len-1]=='/')
        {
                p_s[len-1] ='\0';
                len--;
        }
	 len =len%4;
        if(len!=0) 
	{
	 	len =4-(len%4);
		while(len--)
			sd_strcat(p_s,"=",2);
	}
        len=sd_strlen(p_s);
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
	   
	   if(output_size!=NULL)
	   *output_size = d-d_base+1;
        return 0;
}

/* Convert char string to BASE64
 * Make sure the length of bufer:d larger than 4/3 length of source string:s 
 * Returns: 0:success, -1:failed
 */
_int32 sd_base64_encode(const _u8 *s,int input_size,char *d)
{
	char CharSet[64]={
	'A','B','C','D','E','F','G','H',
	'I','J','K','L','M','N','O','P',
	'Q','R','S','T','U','V','W','X',
	'Y','Z','a','b','c','d','e','f',
	'g','h','i','j','k','l','m','n',
	'o','p','q','r','s','t','u','v',
	'w','x','y','z','0','1','2','3',
	'4','5','6','7','8','9','+','/'};
	unsigned char In[3];
	unsigned char Out[4];
	int /*cnt=0,*/left_len = input_size;

 	LOG_DEBUG( "eui_sd_base64_decode" );
	
	if(!s||!d) return -1;


	for(;left_len>0;)
	{
		//if(cnt+4>76)
		//{
		//*d++='\n';
		//cnt=0;
		//}

		if(left_len>=3)
		{
			In[0]=*s;
			In[1]=*(s+1);
			In[2]=*(s+2);
			Out[0]=In[0]>>2;
			Out[1]=(In[0]&0x03)<<4|(In[1]&0xf0)>>4;
			Out[2]=(In[1]&0x0f)<<2|(In[2]&0xc0)>>6;
			Out[3]=In[2]&0x3f;
			*d=CharSet[Out[0]];
			*(d+1)=CharSet[Out[1]];
			*(d+2)=CharSet[Out[2]];
			*(d+3)=CharSet[Out[3]];
			s+=3;
			left_len-=3;
			d+=4;
		}
		else if(left_len==1)
		{
			In[0]=*s;
			Out[0]=In[0]>>2;
			Out[1]=(In[0]&0x03)<<4|0;
			*d=CharSet[Out[0]];
			*(d+1)=CharSet[Out[1]];
			*(d+2)='=';
			*(d+3)='=';
			s+=1;
			left_len-=1;
			d+=4;
		}
		else if(left_len==2)
		{
			In[0]=*s;
			In[1]=*(s+1);
			Out[0]=In[0]>>2;
			Out[1]=(In[0]&0x03)<<4|(In[1]&0xf0)>>4;
			Out[2]=(In[1]&0x0f)<<2|0;
			*d=CharSet[Out[0]];
			*(d+1)=CharSet[Out[1]];
			*(d+2)=CharSet[Out[2]];
			*(d+3)='=';
			s+=2;
			left_len-=2;
			d+=4;
		}
//		cnt+=4;
		}
	*d='\0';
	return 0;

}

#define np 0
static const unsigned char base64_decode_table[] =
{
    np,np,np,np,np,np,np,np,np,np,  // 0 - 9
    np,np,np,np,np,np,np,np,np,np,  //10 -19
    np,np,np,np,np,np,np,np,np,np,  //20 -29
    np,np,np,np,np,np,np,np,np,np,  //30 -39
    np,np,np,62,np,np,np,63,52,53,  //40 -49
    54,55,56,57,58,59,60,61,np,np,  //50 -59
    np,np,np,np,np, 0, 1, 2, 3, 4,  //60 -69
    5, 6, 7, 8, 9,10,11,12,13,14,  //70 -79
    15,16,17,18,19,20,21,22,23,24,  //80 -89
    25,np,np,np,np,np,np,26,27,28,  //90 -99
    29,30,31,32,33,34,35,36,37,38,  //100 -109
    39,40,41,42,43,44,45,46,47,48,  //110 -119
    49,50,51,np,np,np,np,np,np,np,  //120 -129
    np,np,np,np,np,np,np,np,np,np,  //130 -139
    np,np,np,np,np,np,np,np,np,np,  //140 -149
    np,np,np,np,np,np,np,np,np,np,  //150 -159
    np,np,np,np,np,np,np,np,np,np,  //160 -169
    np,np,np,np,np,np,np,np,np,np,  //170 -179
    np,np,np,np,np,np,np,np,np,np,  //180 -189
    np,np,np,np,np,np,np,np,np,np,  //190 -199
    np,np,np,np,np,np,np,np,np,np,  //200 -209
    np,np,np,np,np,np,np,np,np,np,  //210 -219
    np,np,np,np,np,np,np,np,np,np,  //220 -229
    np,np,np,np,np,np,np,np,np,np,  //230 -239
    np,np,np,np,np,np,np,np,np,np,  //240 -249
    np,np,np,np,np,np               //250 -255
};
#undef np

/** @Simple Function@
* 部分迅雷专用链末尾会被附加一个 / 乃至更多字符，此时无法判断迅雷专用链的结尾
* 如果长度不是4的整数倍，该链接为错误链接，为增加对迅雷专用链的解析成功率，处理方式如下:
* 解析过程中遇到 = ，终止解析
* 剩余一个或两个字符，忽略
* 剩余三个字符，正常解析(也即可以容忍末尾缺少一个 =)
* 目前仍不能成功解析末尾尾随了太多太多无关内容的迅雷专用链
* @param in, in_len 待解析字符串及其长度
* @param out 输出字符串的缓冲区，请自行确保足够大
* @return 成功解析出的长度
* 不检测输入参数的有效性，也不检测输入串中的无效字符
**/
_int32 sd_base64_decode_v2(const _u8* in, _int32 in_len, char* out)
{
    char* p = out;
    for( ; in_len>2 ; in_len-=4)
    {
        *p = base64_decode_table[*in++] << 2;
        *p++ |= base64_decode_table[*in] >> 4;
        *p = base64_decode_table[*in++] << 4;
        if(*in == '=') break;
        *p++ |= base64_decode_table[*in] >> 2;
        *p = base64_decode_table[*in++] << 6;
        if(*in == '=' || *in==0) break;
        *p++ |= base64_decode_table[*in++];
    }
    *p = 0;

    return p - out;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
/*
_int32 sd_encode_base64(const char* input, _u32 input_len, char* output, _u32 output_len)
{
	char CharSet[64]={
	'A','B','C','D','E','F','G','H',
	'I','J','K','L','M','N','O','P',
	'Q','R','S','T','U','V','W','X',
	'Y','Z','a','b','c','d','e','f',
	'g','h','i','j','k','l','m','n',
	'o','p','q','r','s','t','u','v',
	'w','x','y','z','0','1','2','3',
	'4','5','6','7','8','9','+','/'};
	unsigned char In[3];
	unsigned char Out[4];
	int cnt=0;
	if(!input||!output) return -1;
	while(input_len > 0)
	{
		if(cnt+4>76)
		{
		*output++='\n';
		cnt=0;
		}

		if(input_len>=3)
		{
			In[0]=*input;
			In[1]=*(input+1);
			In[2]=*(input+2);
			Out[0]=In[0]>>2;
			Out[1]=(In[0]&0x03)<<4|(In[1]&0xf0)>>4;
			Out[2]=(In[1]&0x0f)<<2|(In[2]&0xc0)>>6;
			Out[3]=In[2]&0x3f;
			*output=CharSet[Out[0]];
			*(output+1)=CharSet[Out[1]];
			*(output+2)=CharSet[Out[2]];
			*(output+3)=CharSet[Out[3]];
			input+=3;
			output+=4;
		}
		else if(input_len==1)
		{
			In[0]=*input;
			Out[0]=In[0]>>2;
			Out[1]=(In[0]&0x03)<<4|0;
			*output=CharSet[Out[0]];
			*(output+1)=CharSet[Out[1]];
			*(output+2)='=';
			*(output+3)='=';
			input+=1;
			output+=4;
		}
		else if(input_len==2)
		{
			In[0]=*input_len;
			In[1]=*(input_len+1);
			Out[0]=In[0]>>2;
			Out[1]=(In[0]&0x03)<<4|(In[1]&0xf0)>>4;
			Out[2]=(In[1]&0x0f)<<2|0;
			*output=CharSet[Out[0]];
			*(output+1)=CharSet[Out[1]];
			*(output+2)=CharSet[Out[2]];
			*(output+3)='=';
			input+=2;
			output+=4;
		}
		cnt+=4;
		}
	*output='\0';
	return 0;

}



*/

	
#endif /* __BASE64_C_20081021 */

