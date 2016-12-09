
#if !defined(__INDEX_PARSER_INTERFACE_20090201)
#define __INDEX_PARSER_INTERFACE_20090201

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#include "utility/range.h"

#define  IP_PARSE_FILEHEAD_FAIL  101
#define  IP_PARSE_UNKNOWN_FILETYPE 102



#define INDEX_PARSER_FILETYPE_RMVB 1
#define INDEX_PARSER_FILETYPE_WMV  2
#define INDEX_PARSER_FILETYPE_AVI    3
#define INDEX_PARSER_FILETYPE_FLV	  4
#define INDEX_PARSER_FILETYPE_UNKNOWN 5


/*获取索引位置*/
_int32 ip_get_index_range_list_by_file_head(char* file_head, _int32 file_head_length, _u64 file_size, _int32 file_type,  RANGE_LIST* index_range_list, _u32* bits_per_second,_u32 * flv_header_to_end_of_first_tag_len);





#ifdef __cplusplus
}
#endif

#endif /* !defined(___INDEX_PARSER_INTERFACE_20090201)*/



