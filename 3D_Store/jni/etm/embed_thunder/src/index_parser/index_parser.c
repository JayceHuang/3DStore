
#include "utility/string.h"
#include "utility/utility.h"
#include "utility/url.h"

#include "utility/logid.h"
#define	 LOGID	LOGID_INDEX_PARSER
#include "utility/logger.h"


#include "index_parser.h"
#include "wmv_parser.h"
#include "rmvb_parser.h"
#include "flv_parser.h"



	
/*内部函数*/
_int32 ip_get_index_range_list_rmvb_by_file_head(char* file_head, _int32 file_head_length, _u64 file_size,  RANGE_LIST* index_range_list, _u32* bits_per_second)
{
	_int32 ret;
	RMFF_FILE_T rmffile;
	RANGE  range;

	sd_memset(&rmffile, 0, sizeof(RMFF_FILE_T));

	LOG_DEBUG("ip_get_index_range_list_rmvb_by_file_head...file_size = %llu ", file_size );
	ret = rmff_read_headers(file_head, file_head_length, &rmffile);
	LOG_DEBUG("ip_get_index_range_list_rmvb_by_file_head... rmff_read_headers return %d", ret);
	if(RMFF_ERR_OK != ret)
		return ret;
       *bits_per_second = rmffile.prop_header.avg_bit_rate;
	/*get index range from rmffile here*/
	LOG_DEBUG("ip_get_index_range_list_rmvb_by_file_head... got index pos,len[%llu, %llu], *bits_per_second = %d", (_u64)(rmffile.prop_header.index_offset),  (_u64)(file_size-rmffile.prop_header.index_offset) , *bits_per_second);
	range = pos_length_to_range(rmffile.prop_header.index_offset,  file_size-rmffile.prop_header.index_offset ,    file_size);
	LOG_DEBUG("ip_get_index_range_list_rmvb_by_file_head... got index range[%lu, %lu]", range._index, range._num);
	ret = range_list_add_range(index_range_list, &range, NULL, NULL);
	LOG_DEBUG("ip_get_index_range_list_rmvb_by_file_head... range_list_add_range  offset=%llu return %d", rmffile.prop_header.index_offset, ret);
	return ret;
}
 
_int32 ip_get_index_range_list_wmv_by_file_head(char* file_head, _int32 file_head_length, _u64 file_size,  RANGE_LIST* index_range_list, _u32* bits_per_second)
{
	_int32 ret;
	AsfFormatInfo asffile;
	RANGE  range;

	LOG_DEBUG("ip_get_index_range_list_wmv_by_file_head...");
	ret = asf_read_header(file_head, file_head_length, &asffile);
	LOG_DEBUG("ip_get_index_range_list_wmv_by_file_head... return %d", ret);
       if(RMFF_ERR_OK != ret)
		return ret;
       *bits_per_second = asffile.file_prop_object.max_bitrate;
	/*get index range from rmffile here*/
	LOG_DEBUG("ip_get_index_range_list_wmv_by_file_head... got index pos, len[%llu, %llu], *bits_per_second=%d", (_u64)(asffile.offset_of_data_object + asffile.data_object_header.object_size),  (_u64)asffile.file_prop_object.file_size-(_u64)(asffile.offset_of_data_object + asffile.data_object_header.object_size), *bits_per_second);
	range = pos_length_to_range((_u64)(asffile.offset_of_data_object + asffile.data_object_header.object_size),  (_u64)asffile.file_prop_object.file_size-(_u64)(asffile.offset_of_data_object + asffile.data_object_header.object_size), (_u64)asffile.file_prop_object.file_size   );
	LOG_DEBUG("ip_get_index_range_list_wmv_by_file_head... got index range[%lu, %lu]", range._index, range._num);
	ret = range_list_add_range(index_range_list, &range, NULL, NULL);
	LOG_DEBUG("ip_get_index_range_list_wmv_by_file_head... range_list_add_range return %d", ret);
	return RMFF_ERR_OK;

}

_int32 ip_get_index_range_list_flv_by_file_head(char* file_head, _int32 file_head_length,  _u64 file_size, RANGE_LIST* index_range_list, _u32* bits_per_second,_u32 * flv_header_to_end_of_first_tag_len)
{
	_int32 ret_val = SUCCESS;
	FLVFileInfo flvfile;
	RANGE range;

	LOG_DEBUG("ip_get_index_range_list_flv_by_file(size=%llu)_head[%d]=%s",file_size,file_head_length,file_head);
	
	sd_memset(&flvfile,0x00,sizeof(FLVFileInfo));
	ret_val = flv_read_headers((_u8*)file_head, file_head_length,  &flvfile);
	LOG_DEBUG("ip_get_index_range_list_flv_by_file_head... return %d", ret_val);
#if 0 //def _DEBUG
	printf("\n @@@@@@ip_get_index_range_list_flv_by_file_head:ret_val=%d,file_head_length=%d,file_size=%llu \n",ret_val,file_head_length,file_size);
#endif

#ifdef ENABLE_FLASH_PLAYER	
	if(ret_val!=SUCCESS) return ret_val;
#endif /* ENABLE_FLASH_PLAYER */

	*bits_per_second = flvfile.audiodatarate + flvfile.videodatarate;
	*flv_header_to_end_of_first_tag_len = (flvfile.end_of_first_audio_tag>flvfile.end_of_first_video_tag)?flvfile.end_of_first_audio_tag:flvfile.end_of_first_video_tag;
	if(*flv_header_to_end_of_first_tag_len!=0) 
	{
		*flv_header_to_end_of_first_tag_len+=4; /* 加上后续的四位tag长度 */
	}
	/*get index range from flvfile here*/
	LOG_DEBUG("ip_get_index_range_list_flv_by_file_head:metadata_offset=%u, metadata_size=%u, *bits_per_second = %u,end_of_first_audio_tag=%u,end_of_first_video_tag=%u,flv_header_to_end_of_first_tag_len=%u",  flvfile.metadata_offset, flvfile.metadata_size, *bits_per_second,flvfile.end_of_first_audio_tag,flvfile.end_of_first_video_tag,*flv_header_to_end_of_first_tag_len);	
#if 0 //def _DEBUG
	printf("\n @@@@@@ip_get_index_range_list_flv_by_file_head:metadata_offset=%u, metadata_size=%u, *bits_per_second = %u,end_of_first_audio_tag=%u,end_of_first_video_tag=%u,flv_header_to_end_of_first_tag_len=%u\n",  flvfile.metadata_offset, flvfile.metadata_size, *bits_per_second,flvfile.end_of_first_audio_tag,flvfile.end_of_first_video_tag,*flv_header_to_end_of_first_tag_len);	
#endif
	range = pos_length_to_range( flvfile.metadata_offset, flvfile.metadata_size, file_size);
	LOG_DEBUG("ip_get_index_range_list_flv_by_file_head... got index range[%lu, %lu]", range._index, range._num);
	ret_val = range_list_add_range(index_range_list, &range, NULL, NULL);
	LOG_DEBUG("ip_get_index_range_list_flv_by_file_head... range_list_add_range return %d", ret_val);
	return ret_val;
	
}


_int32 ip_get_index_range_list_avi_by_file_head(char* file_head, _int32 file_head_length,  _u64 file_size, RANGE_LIST* index_range_list, _u32* bits_per_second)
{
     return SUCCESS;
}
_int32 ip_get_index_range_list_unknown_by_file_head(char* file_head, _int32 file_head_length, _u64 file_size,  RANGE_LIST* index_range_list, _u32* bits_per_second)
{
	_int32 ret = SUCCESS;
	RANGE  range;

	LOG_DEBUG("ip_get_index_range_list_unknown_by_file_head...");

       *bits_per_second = 256*1024*8;
	   
	range._index = 0;
	range._num = 1;
	
	ret = range_list_add_range(index_range_list, &range, NULL, NULL);

	return ret;
}

