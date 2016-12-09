
#include "xv_dec.h"

#ifdef ENABLE_XV_FILE_DEC

#include "utility/mempool.h"
#include "utility/errcode.h"
#include "utility/string.h"

#include <stdio.h>

XV_DEC_CONTEXT* create_xv_decoder() 
{
	XV_DEC_CONTEXT* ctx = NULL;
	int ret_val = sd_malloc(sizeof(XV_DEC_CONTEXT), (void**)&ctx);
	if(ret_val==SUCCESS)
		return ctx;
	else
		return NULL;
}

void close_xv_decoder(XV_DEC_CONTEXT* ctx) 
{
	SAFE_DELETE(ctx);
}

static long long  get_bytes_value(unsigned char* buf,int len)
{
	long  long   ret = 0;
	int i = 0;
	for(i =0;i<len;i++)
	{
		ret += *(buf+i)<<(8*i);
	}
	return ret;
}

static unsigned char get_xv_decrypt_keyvalue(unsigned char* buf,int len) 
{
	unsigned char ret = 0;
	int value = 0;
	char tmp[3] ={0};
	int i = 0;	
	if(len%2)
	{
		len = len-1;
	}
	for(i = 0;i<len;i=i+2)
	{
		tmp[0] = *(buf+i);
		tmp[1] = *(buf+i+1);
		sscanf(tmp,"%x",&value);		
		ret += value;
	}
	return ret;
}

int analyze_xv_head(XV_DEC_CONTEXT* ctx,unsigned char* head_buf,unsigned int len_of_head_buf, unsigned int* decode_data_offset) 
{
	int ret = -1;
	unsigned char* ptr =(unsigned char*)head_buf;
	unsigned long file_version;
	long long mediasize;
	unsigned long ref_url_len;
	unsigned long peer_id_len;
	unsigned long original_url_len;
	char* sgcid;
	char* psub;
	char* ptmp;
	int hasgcid;
	unsigned long movie_name_len;
	unsigned long movie_desc_len;

	unsigned int encrypt_size_len;
	unsigned int range;
	unsigned char keyvalue;
	//check file head is xv head
	if(*ptr == 'X' && *(ptr+1) == 'L' &&*(ptr+2) == 'V' &&*(ptr+3) == 'F')
	{
		//head info
		
		//int headsize = sizeof(xmp_xv_file_head);

		ptr += 12;
		// 读取版本号 4 bytes
		file_version = *ptr;
		ptr += sizeof(unsigned long);
		//xv2 与 xv3 没什么区别除了Version 
		if(file_version == 2 || file_version == 3)
		{
			// 媒体数据开始的文件位置 64 bytes
			mediasize = get_bytes_value(ptr,8);
			*decode_data_offset = mediasize;
			ptr += sizeof(long long);
			
			// 播放影片的应用页 //首先 str len 后 str
			ref_url_len = get_bytes_value(ptr,4);

			ptr += sizeof(unsigned long);
			if( ref_url_len > 0 && ref_url_len <= MAX_XV_FILE_HEAD_SIZE)
			{
				ptr += ref_url_len;
			}

			//unsigned long movie_id; // 影片ID
			ptr += sizeof(unsigned long);

			//unsigned long sub_id; // 
			ptr += sizeof(unsigned long);

			//std::string peer_id; // 下载影片的peerid
			peer_id_len = get_bytes_value(ptr,4);

			ptr += sizeof(unsigned long);
			if( peer_id_len > 0 && peer_id_len <= MAX_XV_FILE_HEAD_SIZE)
			{
				//char mpid[16] = {0};
				//memcpy(mpid,ptr,16);
				ptr += peer_id_len;
			}
			// URL xv 文件url 含 gcid 分析出该gcid做密锁用
			original_url_len = get_bytes_value(ptr,4);

			ptr += sizeof(unsigned long);
			if( original_url_len > 0 && original_url_len <= MAX_XV_FILE_HEAD_SIZE)
			{
				//sgcid =(char*)malloc(original_url_len);
				ret = sd_malloc(original_url_len, (void**)&sgcid);
				CHECK_VALUE(ret);
				sd_memcpy(sgcid,ptr,original_url_len);
				psub = 0;
				ptmp = sgcid;
				hasgcid = 0;
				psub = sd_strchr(ptmp,'/',0);
				while(psub)
				{
					//gcid 40  hex char 
					if(psub - ptmp == 40)
					{
						hasgcid = 1;
						break;
					}
					ptmp = psub+1;
					psub = sd_strchr(ptmp,'/',0);
					
				}				
				keyvalue = get_xv_decrypt_keyvalue((unsigned char*)ptmp,40);
				sd_free(sgcid);
				sgcid = 0;
				ptr += original_url_len;
			}
			// 节目名字 in >> movie_info->movie_name;
			movie_name_len = get_bytes_value(ptr,4);

			ptr += sizeof(unsigned long);
			if( movie_name_len > 0 && movie_name_len <= MAX_XV_FILE_HEAD_SIZE)
			{
				//char* movie_name =(char*)malloc(movie_name_len);
				//memcpy(movie_name,ptr,movie_name_len);
				ptr += movie_name_len;
			}	
			// 节目描述 in >> movie_info->movie_desc;
			movie_desc_len = get_bytes_value(ptr,4);

			ptr += sizeof(unsigned long);
			if( movie_desc_len > 0 && movie_desc_len <= MAX_XV_FILE_HEAD_SIZE)
			{
				//char* movie_desc =(char*)malloc(movie_desc_len);
				//memcpy(movie_desc,ptr,movie_desc_len);
				ptr += movie_desc_len;
			}	
			// 发布时间 in >> movie_info->issuetime; 
			ptr += sizeof(unsigned long);

			// 加密数据块的大小 in >> *encrypt_size;
			encrypt_size_len = get_bytes_value(ptr,4);
			range = encrypt_size_len;
			ptr += sizeof(unsigned long);
			ret = 0;	
		}
		

	}
	//设置上下文
	ctx->encoded_data_offset = mediasize;
	ctx->encoded_data_len = range;
	ctx->key = keyvalue;
	return ret;
}


int decode_xv_data(XV_DEC_CONTEXT* ctx, unsigned int  data_offset,unsigned char* input_buffer,unsigned int len_of_input_buf) 
{
	int i;
	if (data_offset > ctx->encoded_data_offset + ctx->encoded_data_len || data_offset < ctx->encoded_data_offset)
		return 1;//数据无需解密
	for(i = 0;(i < ctx->encoded_data_len - (data_offset - ctx->encoded_data_offset)) && (i < len_of_input_buf); i++ )
	{
		*(input_buffer+i) = *(input_buffer+i) - ctx->key;
	}
	return 0;
}

#endif /* ENABLE_XV_FILE_DEC */

