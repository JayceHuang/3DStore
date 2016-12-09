#include "utility/define.h"
#ifdef ENABLE_EMULE
#include "emule_ed2k_link.h"
#include "emule_define.h"
#include "utility/utility.h"
#include "utility/base64.h"
#include "utility/string.h"
#include "utility/url.h"
#include "utility/sd_iconv.h"
#include "utility/mempool.h"
#include "utility/sd_assert.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_EMULE
#include "utility/logger.h"

_int32 emule_extract_ed2k_link(char* ed2k_link, ED2K_LINK_INFO* ed2k_info)
{
	char new_name[MAX_FILE_NAME_LEN + 1] = {0};
	_int32 ret = SUCCESS, i = 0, ret_count = SUCCESS;
	_u32 output_len = 0, count = 0;
	char* segment = NULL, *source_data = NULL, *target_char = NULL, *part_data = NULL;
	LIST link_segment_list;
	LIST source_list, part_list;
	EMULE_PEER_SOURCE* emule_source = NULL;
	LIST_ITERATOR iter = NULL;
	enum CODE_PAGE code_page;
	LOG_DEBUG("emule_extract_ed2k_link, ed2k_link = %s.", ed2k_link);
	sd_memset(ed2k_info, 0, sizeof(ED2K_LINK_INFO));
	if(sd_strchr(ed2k_link, '|', 0) == NULL)
		sd_replace_str(ed2k_link, "%7C", "|");
	list_init(&link_segment_list);
	list_init(&source_list);
	list_init(&ed2k_info->_source_list);
	list_init(&part_list);
	ret = sd_divide_str(ed2k_link, '|', &link_segment_list);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("sd_divide_str ret=%d", ret);
		return ret;
	}
	if (list_size(&link_segment_list) >= 5)
	{	
		for(iter = LIST_BEGIN(link_segment_list); iter != LIST_END(link_segment_list); iter = LIST_NEXT(iter))
		{	
			segment = (char*)LIST_VALUE(iter);
			//第一个应该是ed2k://标识
			if(count == 0 && sd_stricmp(segment, "ed2k://") != 0)
			{
				ret = -1;
				break;
			}
			//第二个应该是file标识
			if(count == 1 && sd_stricmp(segment, "file") != 0)
			{
				ret = -1;
				break;
			}
			//第三个是文件名
			if(count == 2)
			{
				ret_count = url_object_decode(segment, ed2k_info->_file_name, MAX_FILE_NAME_LEN);
              		  if(ret_count== -1)
              		  {
              		  	LOG_DEBUG("url_object_decode error");
              		  	return DT_ERR_INVALID_FILE_NAME;
              		  }
				code_page = sd_conjecture_code_page(ed2k_info->_file_name);
				switch(code_page)
				{
					case _CP_ACP:
					case _CP_GBK:
                        //sd_strncpy(ed2k_info->_file_name, new_name, MAX_FILE_NAME_LEN);
						break;	//由于编码猜测不准，所以都进行utf8转换
					case _CP_UTF8:
					{
						output_len = MAX_FILE_NAME_LEN;
						ret = sd_utf8_2_gbk(ed2k_info->_file_name, sd_strlen(ed2k_info->_file_name), new_name, &output_len);
                       			 if( ret == SUCCESS )
                       			 {
							sd_strncpy(ed2k_info->_file_name, new_name, MAX_FILE_NAME_LEN);
						}
						ret = SUCCESS;
						break;
					}
                    case _CP_BIG5:
					{
						output_len = MAX_FILE_NAME_LEN;
						ret = sd_big5_2_utf8(ed2k_info->_file_name, sd_strlen(ed2k_info->_file_name), new_name, &output_len);
                       			 if( ret == SUCCESS ) 
                       			 {
							sd_strncpy(ed2k_info->_file_name, new_name, MAX_FILE_NAME_LEN);
						}
						ret = SUCCESS;
						break;
					}
					default:
						sd_assert(FALSE);
				}
			}
			//第四个是文件大小
			if(count == 3)
			{
				sd_str_to_u64(segment, sd_strlen(segment), &ed2k_info->_file_size);
				if(ed2k_info->_file_size > MAX_EMULE_FILE_SIZE || ed2k_info->_file_size == 0)
				{
					LOG_DEBUG("ed2k_info->_file_size > MAX_EMULE_FILE_SIZE || ed2k_info->_file_size == 0");
					ret = -1;
					break;
				}
			}
			//第五个是文件hash
			if(count == 4)
			{
				// 文件哈西
				if(sd_strlen(segment) != ENCODE_LENGTH_BASE16(16))		//file_hash的长度为16
				{
					LOG_DEBUG("sd_strlen(segment)=%d, segment=%s", sd_strlen(segment), segment);
					ret = -1;
					break;
				}
				if(sd_decode_base16(segment, sd_strlen(segment), (char*)ed2k_info->_file_id, FILE_ID_SIZE) != SUCCESS)
				{
					LOG_DEBUG("decode failed ...sd_strlen(segment)=%d, segment=%s", sd_strlen(segment), segment);
					ret = -1;
					break;
				}
			}
			// 根哈西
			if(sd_strncmp(segment, "h=", sd_strlen("h=")) == 0)
			{
				if(sd_decode_base32(segment + 2, sd_strlen(segment) - 2, (char*)ed2k_info->_root_hash, CID_SIZE) != SUCCESS)
				{
					LOG_DEBUG("sd_decode_base32 failed");
					ret = -1;
					break;
				}
			}		
			else if(sd_strncmp(segment, "sources,", sd_strlen("sources,")) == 0)// 源列表
			{
				ret = sd_divide_str(segment, ',', &source_list);
				CHECK_VALUE(ret);
				list_pop(&source_list, (void**)&source_data);
				sd_free(source_data);
				source_data = NULL;
				while(list_size(&source_list) > 0)
				{
					list_pop(&source_list, (void**)&source_data);
					target_char = sd_strchr(source_data, ':', 0);
					if(target_char == NULL)
					{
						sd_free(source_data);
						source_data = NULL;
						continue;
					}
					ret = sd_malloc(sizeof(EMULE_PEER_SOURCE), (void**)&emule_source);
					CHECK_VALUE(ret);
					*target_char = '\0';
					emule_source->_ip = sd_inet_addr(source_data);
					emule_source->_port = sd_atoi(target_char + 1);
					list_push(&ed2k_info->_source_list, emule_source);
					sd_free(source_data);
					source_data = NULL;
				}
			}
			else if(sd_strncmp(segment, "p=", sd_strlen("p=")) == 0)		// 分块哈西
			{
				ret = sd_divide_str(segment + 2, ':', &part_list);
				CHECK_VALUE(ret);
				ed2k_info->_part_hash_size = PART_HASH_SIZE * list_size(&part_list);
				ret = sd_malloc(ed2k_info->_part_hash_size, (void**)&ed2k_info->_part_hash);
				CHECK_VALUE(ret);
				i = 0;
				while(list_size(&part_list) > 0)
				{
					list_pop(&part_list, (void**)&part_data);
					ret = sd_decode_base16(part_data, sd_strlen(part_data), (char*)ed2k_info->_part_hash + PART_HASH_SIZE * i, PART_HASH_SIZE);
					CHECK_VALUE(ret);
					++i;
				}
			}
			else if(sd_strncmp(segment, "s=", sd_strlen("s=")) == 0)		// http源
			{
				sd_assert(sd_strlen(segment + 2) < MAX_URL_LEN);
				sd_strncpy(ed2k_info->_http_url, segment + 2, sd_strlen(segment + 2));
			}
			++count;
		}
	}
    else
    {
    	LOG_DEBUG("else return...");
        ret = -1;
    }
	while(list_size(&link_segment_list) > 0)
	{
		list_pop(&link_segment_list, (void**)&segment);
		sd_free(segment);
		segment = NULL;
	}
	while(list_size(&source_list) > 0)
	{
		list_pop(&source_list, (void**)&source_data);
		sd_free(source_data);
		source_data = NULL;
	}
	while(list_size(&part_list) > 0)
	{
		list_pop(&part_list, (void**)&part_data);
		sd_free(part_data);
		part_data = NULL;
	}
	return ret;
}


//构造一个ed2k链接
_int32 emule_create_ed2k_link(const char* file_name, _u64 file_size, 
		const _u8 file_hash[16], char *ed2k_buf, _u32 *pbuflen)
{
	_int32 ret = SUCCESS;
	char info_hash_str[48] = {0};

	ret = str2hex( (char*)file_hash, 16, info_hash_str, 48);
	CHECK_VALUE(ret);
	ret = sd_snprintf(ed2k_buf, *pbuflen, "%s%s|%llu|%s|/", "ed2k://|file|", 
			file_name, file_size, info_hash_str);
	*pbuflen = ret;
	LOG_DEBUG("emule_create_ed2k_link,ret=%d, %s", ret, ed2k_buf);
	return SUCCESS;
}


#endif /* ENABLE_EMULE */

