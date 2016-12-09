
#if !defined(__INDEX_PARSER_20090201)
#define __INDEX_PARSER_20090201

#ifdef __cplusplus
extern "C" 
{
#endif
	
#include "utility/define.h"
#include "utility/range.h"
	
	/** \brief No error has occured. */
#define INDEX_PARSER_ERR_OK                                    0
	/** \brief The file is not a valid RealMedia file. */
#define INDEX_PARSER_ERR_NOT_RMFF                             -1
	/** \brief The structures/data read from the file were invalid. */
#define INDEX_PARSER_ERR_DATA                                 -2
	/** \brief The end of the file has been reached. */
#define INDEX_PARSER_ERR_EOF                                  -3
	/** \brief An error occured during file I/O. */
#define INDEX_PARSER_ERR_IO                                   -4
	/** \brief The parameters were invalid. */
#define INDEX_PARSER_ERR_PARAMETERS                           -5
	/** \brief An error has occured for which \c errno should be consulted. */
#define INDEX_PARSER_ERR_CHECK_ERRNO                          -6
	
	
/*ÄÚ²¿º¯Êý*/
_int32 ip_get_index_range_list_rmvb_by_file_head(char* file_head, _int32 file_head_length,  _u64 file_size, RANGE_LIST* index_range_list, _u32* bits_per_second);
_int32 ip_get_index_range_list_wmv_by_file_head(char* file_head, _int32 file_head_length,  _u64 file_size, RANGE_LIST* index_range_list, _u32* bits_per_second);
_int32 ip_get_index_range_list_flv_by_file_head(char* file_head, _int32 file_head_length,  _u64 file_size, RANGE_LIST* index_range_list, _u32* bits_per_second,_u32 * flv_header_to_end_of_first_tag_len);
_int32 ip_get_index_range_list_avi_by_file_head(char* file_head, _int32 file_head_length,  _u64 file_size, RANGE_LIST* index_range_list, _u32* bits_per_second);
_int32 ip_get_index_range_list_unknown_by_file_head(char* file_head, _int32 file_head_length, _u64 file_size,  RANGE_LIST* index_range_list, _u32* bits_per_second);
	
#ifdef __cplusplus
}
#endif

#endif /* !defined(__INDEX_PARSER_20090201)*/
