/*******************************************************************************
 * File Name:   torrent_parser.h
 * Author:      WANG CHANGQING
 * Date:        2011-07-11
 * Notes:       Used to parse torrent.
 ******************************************************************************/
#ifndef	___TORRENT_PARSER__H___
#define	___TORRENT_PARSER__H___

#include "torrent_parser/torrent_parser_struct_define.h"
#include <stdint.h>
#include "utility/sha1.h"

#include <string.h>
#include <stdlib.h>
#include "utility/list.h"

#define INFO_SHA1_HASH_LEN 20




/*******************************************************************************
 * User interface of torrent_parser_module.
 */
TORRENT_PARSER* hptp_torrent_init(TORRENT_PARSER *p_torrent);
void hptp_torrent_destroy(TORRENT_PARSER *p_torrent);
int hptp_parse_torrent_part(TORRENT_PARSER *p_torrent, const char * src,
		uint64_t len);
int hptp_finish(TORRENT_PARSER *p_torrent);

int tp_store_result(TORRENT_PARSER *p_torrent, bencoding_dict* p_dict);
int tp_store_list(TORRENT_PARSER *p_torrent, bencoding_list* p_list);

int hp_malloc(size_t len, char **pp_ptr);
int hp_free(void *p);

_int32 init_torrent_file_info_slabs(void) ;

_int32 uninit_torrent_file_info_slabs(void) ;

TORRENT_FILE_INFO* tp_find_incompleted_file(TORRENT_PARSER* p_torrent);

int tp_add_announce(TORRENT_PARSER *p_torrent, bencoding_string *p_ann);
bencoding_int* bencoding_integer_create(bencoding_item_base *p_parent);
bencoding_string* bencoding_string_create(bencoding_item_base *p_parent);
bencoding_list* bencoding_list_create(bencoding_item_base *p_parent);
bencoding_dict* bencoding_dict_create(bencoding_item_base *p_parent);

void bencoding_dict_destroy(bencoding_dict *p_dict) ;
void bencoding_list_destroy(bencoding_list *p_list) ;
void bencoding_string_destroy(bencoding_string *p_string) ;

int bencoding_list_add_item(bencoding_list *p_list, bencoding_item_base *p_item);
void hptp_store_data(TORRENT_PARSER *p_torrent, bencoding_dict* p_dict);
TORRENT_FILE_INFO* hptp_torrent_referred_file_create(uint32_t index);

int tp_convert_charset(const char *src, unsigned src_len, char *dst,
		unsigned *dst_len, enum ENCODING_MODE src_encoding,
		enum ENCODING_SWITCH_MODE convert_mode);
_int32 tp_gbk_2_utf8(const char *p_input, _u32 input_len, char *p_output,
		_u32 *output_len) ;

_int32 tp_utf8_2_gbk(const char *p_input, _u32 input_len, char *p_output,
		_u32 *output_len) ;

_int32 tp_utf8_2_big5(const char *p_input, _u32 input_len, char *p_output,
		_u32 *output_len) ;

_int32 tp_big5_2_utf8(const char *p_input, _u32 input_len, char *p_output,
		_u32 *output_len);


#endif	//___TORRENT_PARSER__H___
