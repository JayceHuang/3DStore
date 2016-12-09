
#include "utility/string.h"
#include "utility/utility.h"
#include "utility/url.h"

#include "utility/logid.h"
#define	 LOGID	LOGID_INDEX_PARSER
#include "utility/logger.h"


#include "index_parser_interface.h"
#include "index_parser.h"



/*获取索引位置*/
_int32 ip_get_index_range_list_by_file_head(char* file_head, _int32 file_head_length, _u64 file_size, _int32 file_type,  RANGE_LIST* index_range_list, _u32* bits_per_second,_u32 * flv_header_to_end_of_first_tag_len)
{
	   LOG_DEBUG("ip_get_index_range_list_by_file_head...file_size = %llu ", file_size );
          switch(file_type)
          {
          case INDEX_PARSER_FILETYPE_RMVB:
                       return ip_get_index_range_list_rmvb_by_file_head(file_head, file_head_length,file_size,  index_range_list, bits_per_second);
			  break;
          case INDEX_PARSER_FILETYPE_WMV:
                       return ip_get_index_range_list_wmv_by_file_head(file_head, file_head_length, file_size, index_range_list, bits_per_second);
			  break;
		case INDEX_PARSER_FILETYPE_FLV:
	   		  return ip_get_index_range_list_flv_by_file_head(file_head, file_head_length, file_size, index_range_list, bits_per_second,flv_header_to_end_of_first_tag_len);
			  break;
	  default:
                       return ip_get_index_range_list_unknown_by_file_head(file_head, file_head_length, file_size, index_range_list, bits_per_second);
          }
}


