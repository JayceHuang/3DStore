/*******************************************************************************
 * File Name:   hp_torrent_parser.h
 * Author:      WANG CHANGQING
 * Date:        2011-07-11
 * Notes:       Used to parse torrent.
 ******************************************************************************/

#include "torrent_parser/torrent_parser.h"

#include <string.h>
#include "utility/mempool.h"
#include "utility/sd_iconv.h"

const int BENCODING_STRING_MAX_LEN = 64 * 1024 * 1024;

const int BENCODING_STRING_TOO_LONG = 300;
const int BAD_MALLOC = 3002;
const int PARSING_FINISHED = 0;
const int ENCODING_CONVERT_BUFF_LEN = 640;
static SLAB* g_torrent_file_info_slab = NULL;
//static SLAB* g_torrent_seed_info_slab = NULL;

TORRENT_PARSER* hptp_torrent_init(TORRENT_PARSER *p_torrent) {
	if (p_torrent == NULL) {
		sd_malloc(sizeof(TORRENT_PARSER), (void **) &p_torrent);
	}

	if (p_torrent != NULL) {
		p_torrent->_hptp_parsing_related._torrent_dict._key = NULL;
		p_torrent->_hptp_parsing_related._torrent_dict._item = NULL;
		p_torrent->_hptp_parsing_related._torrent_dict._base._len = 0;
		p_torrent->_hptp_parsing_related._torrent_dict._base._p_parent = NULL;
		p_torrent->_hptp_parsing_related._torrent_dict._base._state =
				dict_waiting_prefix;
		p_torrent->_hptp_parsing_related._torrent_dict._base._type =
				BENCODING_ITEM_DICT;
		p_torrent->_hptp_parsing_related._p_item =
				(bencoding_item_base *) &p_torrent->_hptp_parsing_related._torrent_dict;
		p_torrent->_hptp_parsing_related._torrent_size = 0;
		p_torrent->_hptp_parsing_related._bytes_parsed = 0;

		p_torrent->_title_name_len = 0;
		p_torrent->_title_name = NULL;
		p_torrent->_hptp_parsing_related._update_info_sha1 = 0;

		p_torrent->_encoding = ENCODING_UNDEFINED;
		p_torrent->_switch_mode = DEFAULT_SWITCH;

		p_torrent->_ann_list = NULL;
		p_torrent->_file_list = NULL;
		p_torrent->_list_tail = NULL;

		p_torrent->_info_hash_valid = 0;
		p_torrent->_piece_hash_len = 0;
		p_torrent->_piece_hash_ptr = NULL;
		p_torrent->_piece_length = 0;
		p_torrent->_file_num = 0;

		list_init(&p_torrent->_tracker_list);
		sha1_initialize(&p_torrent->_hptp_parsing_related._sha1_ctx);
	}
	return p_torrent;
}

//if str_prefix is not null, copy str_prefix to the result buffer first
 void hp_realloc_convert_encoding(const char *str_prefix,int pre_len,char **str, unsigned *len,
		enum ENCODING_MODE src_encoding, enum ENCODING_MODE dst_encoding) {
	unsigned dst_buff_len = 8 * *len;
	char *dst_str_buff = NULL;
	if (SUCCESS != sd_malloc(dst_buff_len, (void **)&dst_str_buff))
		return;

	int result = tp_convert_charset(*str, *len, dst_str_buff, &dst_buff_len,
			src_encoding, dst_encoding);
	if (result == SUCCESS) {
		sd_free(*str);
		*len = (str_prefix!=NULL)?(dst_buff_len+pre_len+1):dst_buff_len;//one more byte for '/'
		sd_malloc(*len + 1, (void **) str);//one more byte for '\0'
		if(str_prefix!=NULL){
			sd_memcpy(*str, str_prefix, pre_len);
			(*str)[pre_len]='/';
			++pre_len;
		}
		sd_memcpy(*str+pre_len, dst_str_buff, dst_buff_len);
		*(*str + *len) = '\0';
	}
	else if(str_prefix!=NULL){
		dst_buff_len = *len;
		sd_memcpy(dst_str_buff,*str,*len);
		
		sd_free(*str);
		*len = dst_buff_len+pre_len+1;//one more byte for '/'
		sd_malloc(*len + 1, (void **) str);//one more byte for '\0'
		sd_memcpy(*str, str_prefix, pre_len);
		(*str)[pre_len]='/';
		++pre_len;
		sd_memcpy(*str+pre_len, dst_str_buff, dst_buff_len);
		*(*str + *len) = '\0';
	}

	sd_free(dst_str_buff);
}

int hptp_finish(TORRENT_PARSER *p_torrent) {
	int parsing_result = 0;

	if (p_torrent->_piece_hash_ptr == NULL || p_torrent->_piece_length <= 0
			|| p_torrent->_piece_hash_len == 0) {
		return BT_SEED_BAD_ENCODING;
	}

	if (p_torrent->_file_num == 0) {
		TORRENT_FILE_INFO *p_file = tp_find_incompleted_file(p_torrent);
		if (p_file == NULL) {
			return BT_SEED_PARSE_FAILED;
		}
		p_torrent->_file_list = p_file;
		p_torrent->_list_tail = p_file;
		sd_malloc(p_torrent->_title_name_len + 1,
				(void **) &p_file->_file_name);
		sd_malloc(1u, (void **) &p_file->_file_path);
		if (p_file->_file_name == NULL || p_file->_file_path == NULL) {
			
			return BT_SEED_PARSE_FAILED;
		} else {
			p_file->_file_size = p_torrent->_file_total_size;
			sd_memcpy(p_file->_file_name, p_torrent->_title_name,
					p_torrent->_title_name_len + 1);
			p_file->_file_name_len = p_torrent->_title_name_len;
			p_file->_file_path[0] = 0;
		}
		p_torrent->_file_num = 1;
		p_file->_file_size = p_torrent->_file_total_size;
	} else {
		p_torrent->_file_total_size = p_torrent->_list_tail->_file_offset
				+ p_torrent->_list_tail->_file_size;
	}

	if (p_torrent->_file_num == 0 || p_torrent->_file_total_size == 0) {
		
		return BT_SEED_PARSE_FAILED;
	}
	if (p_torrent->_title_name == NULL || p_torrent->_title_name_len <= 0) {
		
		return BT_SEED_BAD_ENCODING;
	}

	if (p_torrent->_hptp_parsing_related._bytes_parsed
			< p_torrent->_hptp_parsing_related._torrent_size&& p_torrent->_hptp_parsing_related._p_item!=NULL) {
			
		return BT_SEED_BAD_ENCODING;
	}

	if (p_torrent->_file_list == NULL
			|| p_torrent->_list_tail->_file_size
					< 0|| p_torrent->_list_tail->_file_path==NULL
					|| p_torrent->_list_tail->_file_name==NULL) {
					
		return BT_SEED_PARSE_FAILED;
	}

	bencoding_string *p_node = p_torrent->_ann_list;
	while (p_node) {
		list_push(&p_torrent->_tracker_list, p_node->_str);
		p_node = (bencoding_string *) p_node->_base._p_next;
	}


	hp_realloc_convert_encoding(NULL,0,&p_torrent->_title_name,
			(unsigned *) &p_torrent->_title_name_len, p_torrent->_encoding,
			p_torrent->_switch_mode);

	TORRENT_FILE_INFO *p_file = p_torrent->_file_list;
	p_torrent->_is_dir_organized = 0;
	while (p_file) {
		hp_realloc_convert_encoding(NULL,0,&p_file->_file_name,
				&p_file->_file_name_len, p_torrent->_encoding,
				p_torrent->_switch_mode);
		if (p_file->_file_path_len > 0){
			hp_realloc_convert_encoding(p_torrent->_title_name,p_torrent->_title_name_len,&p_file->_file_path,
					&p_file->_file_path_len, p_torrent->_encoding,
					p_torrent->_switch_mode);
			p_torrent->_is_dir_organized = 1;
		}
		else if(p_torrent->_file_num > 1){
			sd_free(p_file->_file_path);
			sd_malloc(p_torrent->_title_name_len+2,(void **)&p_file->_file_path);
			//allocate 2 more bytes, the first one is for path splitter '/' and another for '\0'

			p_file->_file_path_len = p_torrent->_title_name_len+1;
			sd_memcpy(p_file->_file_path,p_torrent->_title_name,p_torrent->_title_name_len);
			p_file->_file_path[p_torrent->_title_name_len]='/';
			p_file->_file_path[p_file->_file_path_len]='\0';
			p_torrent->_is_dir_organized = 1;
		}
		p_file = p_file->_p_next;
	}

	return 0;
}

int hptp_parse_torrent_part(TORRENT_PARSER *p_torrent, const char * src,
		uint64_t len) {
	bencoding_item_base* p_item = p_torrent->_hptp_parsing_related._p_item;
	uint64_t parse_cur = 0;
	const int bad_bencoding_occurred = -1;
	const int bad_nesting = 2123;
	long long sha1_start = -1;
	long long sha1_end = -1;

	int parsing_result = 0; //1 stands for illegal char in integer

	for (parse_cur = 0; parse_cur < len; ++parse_cur) {
		const char input_char = src[parse_cur];

		if (p_item == NULL) {
			break;
		} else if (p_item->_type == BENCODING_ITEM_INTEGER
				|| p_item->_type == BENCODING_ITEM_NEGINT) {
			bencoding_int *p_int = (bencoding_int *) p_item;
			if (input_char >= '0' && input_char <= '9') {
				++p_int->_base._len;
				p_int->_value = p_int->_value * 10 + input_char - '0';
			} else if (input_char == '-') {
				p_int->_base._type = BENCODING_ITEM_NEGINT;
			} else if (input_char == 'e') {
				if (p_item->_type == BENCODING_ITEM_NEGINT)
					p_int->_value = 0 - p_int->_value;

				p_item = p_item->_p_parent;

				if (p_item->_type == BENCODING_ITEM_DICT) {
					p_item->_state = dict_waiting_key;
				}
			} else {
				parsing_result = bad_bencoding_occurred;
				break;
			}
		} else if (p_item->_type == BENCODING_ITEM_STRING) {
			bencoding_string *p_str = (bencoding_string *) p_item;

			if (p_item->_state == string_parsing_len) {
				if (input_char >= '0' && input_char <= '9') {
					p_item->_len = p_item->_len * 10 + input_char - '0';
				} else if (input_char == ':') {
					if (p_item->_len > BENCODING_STRING_MAX_LEN) {
						parsing_result = BENCODING_STRING_TOO_LONG;
						break;
					}

					p_item->_state = string_parsing_buf;
					p_str->_valid_len = 0;

					sd_malloc(p_str->_base._len + 1, (void **) &p_str->_str);
					if (p_str->_str == NULL) {
						parsing_result = BAD_MALLOC;
						break;
					}
				} else {
					parsing_result = bad_bencoding_occurred;
					break;
				}
			} else {
				//p_str->_str[p_str->_valid_len++]=input_char;
				long long copy_len = p_str->_base._len - p_str->_valid_len;

				if (len - parse_cur >= copy_len) {
					memcpy(p_str->_str + p_str->_valid_len, src + parse_cur,
							copy_len);
					p_str->_valid_len = p_str->_base._len;
					parse_cur += copy_len - 1;
					p_torrent->_hptp_parsing_related._bytes_parsed += copy_len
							- 1;
				} else {
					memcpy(p_str->_str + p_str->_valid_len, src + parse_cur,
							len - parse_cur);
					p_str->_valid_len += len - parse_cur;
					parse_cur = len - 1;
					p_torrent->_hptp_parsing_related._bytes_parsed += len
							- parse_cur - 1;
				}
			}

			if (p_str->_valid_len == p_str->_base._len) {
				p_str->_base._state = string_parsing_buf;
				p_str->_str[p_str->_valid_len] = '\0';
				p_item = p_item->_p_parent;

				if (p_item->_type == BENCODING_ITEM_DICT) {
					p_item->_state =
							(p_item->_state == dict_key_parsing) ?
									dict_waiting_val : dict_waiting_key;
				}
				/***
				 * Check info hash location,the BENCODING dictionary referred
				 *  by key "info" in the root dictionary of the torrent
				 */
				if (p_str->_valid_len == 4
						&& p_str->_base._p_parent->_type == BENCODING_ITEM_DICT
						&& p_str->_base._p_parent->_p_parent == NULL
						&& strncmp("info", p_str->_str, 4) == 0) {
					p_torrent->_hptp_parsing_related._update_info_sha1 = 1;
					sha1_start = parse_cur + 1;
				}
			}
		} else if (p_item->_type == BENCODING_ITEM_DICT) {
			if (p_item->_state == dict_waiting_key) {
				bencoding_dict *p_dict = (bencoding_dict *) p_item;
				if (p_dict->_key != NULL) {
					parsing_result = tp_store_result(p_torrent, p_dict);
					if (parsing_result != 0) {
						break;
					}
				}
				if (input_char >= '0' && input_char <= '9') {
					bencoding_string *p_str = bencoding_string_create(p_item);
					if (p_str == NULL) {
						parsing_result = BAD_MALLOC;
						break;
					}
					p_str->_base._len = input_char - '0';
					p_dict->_base._state = dict_key_parsing;
					p_dict->_key = p_str;
					p_item = (bencoding_item_base *) p_str;
				} else if (input_char == 'e') {
					p_item = p_item->_p_parent;

					if (p_item == NULL) {
						++parse_cur;
						++p_torrent->_hptp_parsing_related._bytes_parsed;
						break;
					} else if (p_item->_type == BENCODING_ITEM_DICT) {
						p_item->_state = dict_waiting_key;
					}
					if (p_item != NULL && p_item->_p_parent == NULL 
					&& p_item->_type == BENCODING_ITEM_DICT
					&& strncmp("info", ((bencoding_dict *)p_item)->_key->_str, 4) == 0) {
						sha1_end = parse_cur + 1;
					}
				} else {
					parsing_result = bad_bencoding_occurred;
					break;
				}
			} else if (p_item->_state == dict_waiting_val) {
				bencoding_dict *p_dict = (bencoding_dict *) p_item;
				if (input_char >= '0' && input_char <= '9') {
					bencoding_string *p_str = bencoding_string_create(p_item);
					if (p_str == NULL) {
						parsing_result = BAD_MALLOC;
						break;
					}
					p_str->_base._len = input_char - '0';
					p_item->_state = dict_val_parsing;
					p_dict->_item = (bencoding_item_base *) p_str;
					p_item = (bencoding_item_base *) p_str;
				} else if (input_char == 'd') {
					bencoding_dict *p_new_dict = bencoding_dict_create(p_item);
					if (p_new_dict == NULL) {
						parsing_result = BAD_MALLOC;
						break;
					}
					p_item->_state = dict_val_parsing;
					((bencoding_dict *) p_item)->_item =
							(bencoding_item_base *) p_new_dict;
					p_item = (bencoding_item_base *) p_new_dict;
				} else if (input_char == 'l') {
					bencoding_list *p_list = bencoding_list_create(p_item);
					if (p_list == NULL) {
						parsing_result = BAD_MALLOC;
						break;
					}
					p_item->_state = dict_val_parsing;
					((bencoding_dict *) p_item)->_item =
							(bencoding_item_base *) p_list;
					p_item = (bencoding_item_base *) p_list;
				} else if (input_char == 'i') {
					bencoding_int *p_int = bencoding_integer_create(p_item);
					if (p_int == NULL) {
						parsing_result = BAD_MALLOC;
						break;
					}
					p_item->_state = dict_val_parsing;
					((bencoding_dict *) p_item)->_item =
							(bencoding_item_base *) p_int;
					p_item = (bencoding_item_base *) p_int;
				} else {
					parsing_result = bad_bencoding_occurred;
					break;
				}
			} else {
				//p_item->_state is HPTP_STATE_WAIT_DICT_FEATURE
				if (input_char == 'd') {
					p_item->_state = dict_waiting_key;
				} else {
					parsing_result = bad_bencoding_occurred;
					break;
				}
			}
		} else if (p_item->_type == BENCODING_ITEM_LIST) {
			bencoding_list *p_list = (bencoding_list *) p_item;
			if (p_list->_item != NULL
					&& p_list->_item->_type != BENCODING_ITEM_STRING) {
				parsing_result = tp_store_list(p_torrent, p_list);
				if (parsing_result != 0)
					break;
			}
			if (input_char >= '0' && input_char <= '9') {
				if (p_list->_item != NULL
						&& p_list->_item->_type != BENCODING_ITEM_STRING) {
					parsing_result = bad_nesting;
					break;
				}
				bencoding_string *p_str = bencoding_string_create(p_item);
				if (p_str == NULL) {
					parsing_result = BAD_MALLOC;
					break;
				}

				p_str->_base._len = input_char - '0';
				if (p_list->_item == NULL) {
					p_list->_item = (bencoding_item_base *) p_str;
				} else {
					bencoding_item_base *p_node = p_list->_item;
					while (p_node->_p_next != NULL)
						p_node = p_node->_p_next;
					p_node->_p_next = (bencoding_item_base *) p_str;
				}
				p_item = (bencoding_item_base *) p_str;
			} else if (input_char == 'd') {
				bencoding_dict *p_dict = bencoding_dict_create(p_item);
				if (p_dict == NULL) {
					parsing_result = BAD_MALLOC;
					break;
				}
				p_list->_item = (bencoding_item_base *) p_dict;
				p_item = (bencoding_item_base *) p_dict;
			} else if (input_char == 'i') {
				bencoding_int *p_new = bencoding_integer_create(p_item);
				if (p_new == NULL) {
					parsing_result = BAD_MALLOC;
					break;
				}
				bencoding_list_add_item(p_list, (bencoding_item_base *) p_new);
				p_item = (bencoding_item_base *) p_new;
			} else if (input_char == 'l') {
				bencoding_list *p_new = bencoding_list_create(p_item);
				if (p_new == NULL) {
					parsing_result = BAD_MALLOC;
					break;
				}
				p_list->_item = (bencoding_item_base *) p_new;
				p_item = (bencoding_item_base *) p_new;
			} else if (input_char == 'e') {
				if (p_list->_item != NULL) {
					parsing_result = tp_store_list(p_torrent, p_list);
					if (parsing_result != PARSING_FINISHED)
						break;
				}

				p_item = p_item->_p_parent;
				if (p_item->_type == BENCODING_ITEM_DICT) {
					p_item->_state = dict_waiting_key;
				}
			} else {
				parsing_result = bad_bencoding_occurred;
				break;
			}
		}
		++p_torrent->_hptp_parsing_related._bytes_parsed;
	}

	p_torrent->_hptp_parsing_related._p_item = p_item;

	if (parsing_result == PARSING_FINISHED
			&& p_torrent->_hptp_parsing_related._update_info_sha1 == 1) {
		if (sha1_start >= 0 && sha1_end >= 0) {
			sha1_update(&p_torrent->_hptp_parsing_related._sha1_ctx,
					(unsigned char *) src + sha1_start, sha1_end - sha1_start);
			sha1_finish(&p_torrent->_hptp_parsing_related._sha1_ctx,
					p_torrent->_info_hash);
			p_torrent->_info_hash_valid = 1;
		} else if (sha1_start >= 0) {
			sha1_update(&p_torrent->_hptp_parsing_related._sha1_ctx,
					(unsigned char *) src + sha1_start, len - sha1_start);
		} else if (sha1_end >= 0) {
			sha1_update(&p_torrent->_hptp_parsing_related._sha1_ctx,
					(unsigned char *) src, sha1_end);
			sha1_finish(&p_torrent->_hptp_parsing_related._sha1_ctx,
					p_torrent->_info_hash);
			p_torrent->_info_hash_valid = 1;
		} else {
			sha1_update(&p_torrent->_hptp_parsing_related._sha1_ctx,
					(unsigned char *) src, len);
		}
	} else {
	}

	return parse_cur;
}

/**
 * return : null stands for error occurred
 */
TORRENT_FILE_INFO* tp_find_incompleted_file(TORRENT_PARSER* p_torrent) {
	TORRENT_FILE_INFO *p_file = NULL;
	if (p_torrent->_file_list == NULL) {
		p_file = hptp_torrent_referred_file_create(0);

		if (p_file == NULL)
			return p_file;

		p_torrent->_list_tail = p_file;
		p_torrent->_file_list = p_file;
		p_torrent->_file_num++;
	} else {
		if (p_torrent->_list_tail->_file_name == NULL
				|| p_torrent->_list_tail->_file_path == NULL
				|| p_torrent->_list_tail->_file_size == -1) {
			p_file = p_torrent->_list_tail;
		} else {
			p_file = hptp_torrent_referred_file_create(
					p_torrent->_list_tail->_file_index + 1);
			if (p_file != NULL) {
				p_file->_file_offset = p_torrent->_list_tail->_file_offset
						+ p_torrent->_list_tail->_file_size;
				p_torrent->_list_tail->_p_next = p_file;
				p_torrent->_list_tail = p_file;
				p_torrent->_file_num++;
			}
		}
	}

	return p_file;
}

/**
 * prerequisites: p_torrent != NULL &&p_dict != NULL && p_dict->_key != NULL && p_dict->_item != NULL
 */
int tp_store_result(TORRENT_PARSER *p_torrent, bencoding_dict* p_dict) {
	const int dict_key_len = p_dict->_key->_valid_len;
	const char* dict_key = p_dict->_key->_str;

	bencoding_item_base *p_item = p_dict->_item;
	int store_result = 0;

	if (p_item->_type == BENCODING_ITEM_INTEGER
			|| p_item->_type == BENCODING_ITEM_NEGINT) {
		if (p_dict->_base._p_parent != NULL) {
			bencoding_int *p_int = (bencoding_int *) p_item;
			//p_dict is not the root dictionary
			if (p_dict->_base._p_parent->_type == BENCODING_ITEM_LIST) {
				//p_dict's parent is a list
				bencoding_list *p_list =
						(bencoding_list *) p_dict->_base._p_parent;
				if (p_list->_base._p_parent->_type == BENCODING_ITEM_DICT) {
					//expecting p_list's parent is the dict referred by key "info" in the root dict
					bencoding_dict *p_info_dict =
							(bencoding_dict *) p_list->_base._p_parent;
					if (strcmp(p_info_dict->_key->_str, "files") == 0) {
						if (strcmp(dict_key, "length") == 0) {
							TORRENT_FILE_INFO* p_file =
									tp_find_incompleted_file(p_torrent);
							if (p_file == NULL) {
								store_result = -1;
							} else {
								p_file->_file_size = p_int->_value;
							}
						} else {
							//ignore the item
						}
					} else {
						//ignore the item.
					}
				} else {
					store_result = -1;
				}
			} else {
				//p_dict's parent is a BENCODING dictionary
				if (strcmp(dict_key, "piece length") == 0) {
					p_torrent->_piece_length = p_int->_value;
				}
				//expecting it's a single file torrent.
				if (p_dict->_base._p_parent->_p_parent == NULL
						&& strcmp(
								((bencoding_dict *) p_dict->_base._p_parent)->_key->_str,
								"info") == 0) {
					if (strcmp(dict_key, "length") == 0) {
						p_torrent->_file_total_size = p_int->_value;
					}
				}
			}

			sd_free(p_int);
		} else {
			//in this branch p_dict->_base._p_parent == NULL

			bencoding_int *p_int = (bencoding_int *) p_item;
			//p_dict is a root dictionary item
			if (strcmp(dict_key, "piece length") == 0) {
				p_torrent->_piece_length = p_int->_value;
			}
			sd_free(p_int);
		}
	} else if (p_item->_type == BENCODING_ITEM_STRING) {
		if (p_dict->_base._p_parent != NULL) {
			bencoding_string *p_str = (bencoding_string *) p_item;
			if (p_dict->_base._p_parent->_type == BENCODING_ITEM_LIST) {
				sd_free(p_str->_str);
			} else {
				//p_dict's parent is a bencoding dictionary
				if (strcmp(dict_key, "name") == 0) {
					p_torrent->_title_name = p_str->_str;
					p_torrent->_title_name_len = p_str->_valid_len;
				} else if (strcmp(dict_key, "pieces") == 0) {
					p_torrent->_piece_hash_ptr = (uint8_t *) p_str->_str;
					p_torrent->_piece_hash_len = p_str->_valid_len;
				}
			}
			sd_free(p_str);
		} else {
			bencoding_string *p_str = (bencoding_string *) p_item;
			//p_dict is the root dict of the torrent
			if (strcmp(dict_key, "announce") == 0) {
				p_str->_base._p_next = NULL;
				tp_add_announce(p_torrent, p_str);
				p_str = NULL;
			} else if (strcmp(dict_key, "encoding") == 0) {
				if (strcasecmp(p_str->_str, "utf-8") == 0) {
					p_torrent->_encoding = UTF_8;
				} else if (strcasecmp(p_str->_str, "gbk") == 0) {
					p_torrent->_encoding = GBK;
				} else if (strcasecmp(p_str->_str, "big5") == 0) {
					p_torrent->_encoding = BIG5;
				} else {
					p_torrent->_encoding = ENCODING_UNDEFINED;
				}
			} else {
			}

			if (p_str != NULL) {
				sd_free(p_str->_str);
				sd_free(p_str);
			}
		}
	} else if (p_item->_type == BENCODING_ITEM_DICT) {
		sd_free(p_item);
	} 

	sd_free(p_dict->_key->_str);
	sd_free(p_dict->_key);
	p_dict->_key = NULL;
	p_dict->_item = NULL;
	return store_result;
}

int tp_store_list(TORRENT_PARSER *p_torrent, bencoding_list* p_list) {
	int store_result = 0;

	if (p_list->_base._p_parent->_type == BENCODING_ITEM_LIST) {
		bencoding_item_base *p_parent_list = p_list->_base._p_parent;
		if (p_parent_list->_p_parent->_type
				== BENCODING_ITEM_DICT&& p_parent_list->_p_parent->_p_parent == NULL) {
			bencoding_dict *p_dict = (bencoding_dict *) p_parent_list->_p_parent;
			if (strcmp(p_dict->_key->_str, "announce-list") == 0) {
				bencoding_string *p_str = (bencoding_string *) p_list->_item;
				p_str->_base._p_next = NULL;
				tp_add_announce(p_torrent, p_str);
				p_list->_item = NULL;
			} else {
				//dht nodes is possible here.
				bencoding_item_base *p_node = p_list->_item;
				while (p_node != NULL) {
					if (p_node->_type == BENCODING_ITEM_STRING) {
						sd_free(((bencoding_string *) p_node)->_str);
					}
					bencoding_item_base *p_del = p_node;
					p_node = (bencoding_item_base *) p_node->_p_next;
					sd_free(p_del);
				}
				p_list->_item = NULL;
			}
		} else {
			store_result = -1;
		}
	} else {
		//p_list's parent is a dictionary
		bencoding_dict *p_dict = (bencoding_dict *) p_list->_base._p_parent;
		//expecting path
		if (p_dict->_base._p_parent != NULL
				&& strcmp(p_dict->_key->_str, "path") == 0) {
			bencoding_string *p_str = (bencoding_string *) p_list->_item;
			int path_length = 2;//at least, append a '/' and '\0'
			while (p_str->_base._p_next != NULL) {
				path_length += p_str->_valid_len+1;
				p_str = (bencoding_string *) p_str->_base._p_next;
			}

			do {
				TORRENT_FILE_INFO *p_file = tp_find_incompleted_file(p_torrent);
				if (p_file == NULL) {
					store_result = BAD_MALLOC;
					break;
				}

				sd_malloc(path_length, (void **) &p_file->_file_path);
				if (p_file->_file_path == NULL) {
					store_result = BAD_MALLOC;
					break;
				}

				p_str = (bencoding_string *) p_list->_item;
				bencoding_string *p_del;
				while (p_str->_base._p_next != NULL) {
					memcpy(p_file->_file_path + p_file->_file_path_len,
							p_str->_str, p_str->_valid_len);
					p_file->_file_path_len += p_str->_valid_len;
					p_file->_file_path[p_file->_file_path_len]='/';
					p_file->_file_path_len++;
					sd_free(p_str->_str);
					p_del = p_str;
					p_str = (bencoding_string *) p_str->_base._p_next;
					sd_free(p_del);
				}
				p_file->_file_path[p_file->_file_path_len]='/';
				p_file->_file_path_len++;
				p_file->_file_path[p_file->_file_path_len] = 0;
				p_file->_file_name = p_str->_str;
				p_file->_file_name_len = p_str->_valid_len;
				sd_free(p_str);
			} while (0);

			if (store_result == BAD_MALLOC) {
				bencoding_item_base *p_node = p_list->_item;
				bencoding_item_base *p_del;
				while (p_node != NULL) {
					if (p_node->_type == BENCODING_ITEM_STRING) {
						sd_free(((bencoding_string *) p_node)->_str);
					}
					p_del = p_node;
					p_node = p_node->_p_next;
					sd_free(p_del);
				}
			}

			p_list->_item = NULL;
		} else {
			bencoding_item_base *p_node = p_list->_item;
			bencoding_item_base *p_del;

			while (p_node != NULL) {
				if (p_node->_type == BENCODING_ITEM_STRING) {
					sd_free(((bencoding_string *) p_node)->_str);
				}
				p_del = p_node;
				p_node = p_node->_p_next;
				sd_free(p_del);
			}
		}
	}

	p_list->_item = NULL;
	return store_result;
}

bencoding_int* bencoding_integer_create(bencoding_item_base *p_parent) {
	bencoding_int* p_int;
	sd_malloc(sizeof(bencoding_int), (void **) &p_int);
	if (p_int != NULL) {
		p_int->_base._type = BENCODING_ITEM_INTEGER;
		p_int->_base._p_parent = p_parent;
		p_int->_value = 0;
		p_int->_base._len = 0;
		p_int->_base._p_next = NULL;
	}

	return p_int;
}

bencoding_string* bencoding_string_create(bencoding_item_base *p_parent) {
	bencoding_string* p_str;
	sd_malloc(sizeof(bencoding_string), (void **) &p_str);
	if (p_str != NULL) {
		p_str->_base._type = BENCODING_ITEM_STRING;
		p_str->_base._p_parent = p_parent;
		p_str->_base._state = string_parsing_len;
		p_str->_base._len = 0;
		p_str->_str = NULL;
		p_str->_valid_len = 0;
		p_str->_base._p_next = NULL;
	}

	return p_str;
}

bencoding_list* bencoding_list_create(bencoding_item_base *p_parent) {
	bencoding_list* p_list;
	sd_malloc(sizeof(bencoding_list), (void **) &p_list);

	if (p_list != NULL) {
		p_list->_base._type = BENCODING_ITEM_LIST;
		p_list->_base._p_parent = p_parent;
		p_list->_base._len = 0;
		p_list->_item = NULL;
		p_list->_base._p_next = NULL;
	}

	return p_list;
}

bencoding_dict* bencoding_dict_create(bencoding_item_base *p_parent) {
	bencoding_dict* p_dict;
	sd_malloc(sizeof(bencoding_dict), (void **) &p_dict);
	if (p_dict != NULL) {
		p_dict->_base._type = BENCODING_ITEM_DICT;
		p_dict->_base._p_parent = p_parent;
		p_dict->_base._state = dict_waiting_key;
		p_dict->_base._len = 0;
		p_dict->_key = NULL;
		p_dict->_item = NULL;
		p_dict->_base._p_next = NULL;
	}

	return p_dict;
}
void bencoding_string_destroy(bencoding_string *p_str) {
	if (p_str->_str != NULL) {
		sd_free(p_str->_str);
	}
	sd_free(p_str);
}

void bencoding_list_destroy(bencoding_list *p_list) {
	bencoding_item_base *p_node = p_list->_item;
	while (p_node != NULL) {
		bencoding_item_base *p_del = p_node;
		p_node = p_node->_p_next;
		if (p_del->_type == BENCODING_ITEM_NEGINT
				|| p_del->_type == BENCODING_ITEM_INTEGER) {
			sd_free(p_del);
		} else if (p_del->_type == BENCODING_ITEM_STRING) {
			bencoding_string_destroy((bencoding_string *)p_del);
		} else if (p_del->_type == BENCODING_ITEM_LIST) {
			bencoding_list_destroy((bencoding_list *)p_del);
		} else {
			bencoding_dict_destroy((bencoding_dict *)p_del);
		}
	}
}

void bencoding_dict_destroy(bencoding_dict *p_dict) {
	if (p_dict->_key != NULL) {
		bencoding_string_destroy(p_dict->_key);
	}

	if (p_dict->_item != NULL) {
		if (p_dict->_item->_type == BENCODING_ITEM_INTEGER
				|| p_dict->_item->_type == BENCODING_ITEM_NEGINT) {
			sd_free(p_dict->_item);
		} else if (p_dict->_item->_type == BENCODING_ITEM_STRING) {
			bencoding_string_destroy((bencoding_string *)p_dict->_item);
		} else if (p_dict->_item->_type == BENCODING_ITEM_LIST) {
			bencoding_list_destroy((bencoding_list *)p_dict->_item);
		} else {
			bencoding_dict_destroy((bencoding_dict *)p_dict->_item);
		}
	}
}

int hptp_parsing_related_add_file(TORRENT_PARSER *p_torrent,
		TORRENT_FILE_INFO *p_file) {
	p_file->_p_next = NULL;

	if (p_torrent->_file_list == NULL) {
		p_torrent->_file_list = p_file;
		p_torrent->_list_tail = p_file;
	} else {
		p_torrent->_list_tail->_p_next = p_file;
		p_torrent->_list_tail = p_file;
	}

	++p_torrent->_file_num;
	return 0;
}

TORRENT_FILE_INFO* hptp_torrent_referred_file_create(uint32_t index) {
	TORRENT_FILE_INFO* p_file;
	sd_malloc(sizeof(TORRENT_FILE_INFO), (void **) &p_file);
	if (p_file != NULL) {
		p_file->_file_index = index;

		p_file->_p_next = NULL;

		p_file->_file_offset = 0;
		p_file->_file_size = -1;

		p_file->_file_name = NULL;
		p_file->_file_path = NULL;

		p_file->_file_name_len = 0;
		p_file->_file_path_len = 0;
	}
	return p_file;
}

int bencoding_list_add_item(bencoding_list *p_list, bencoding_item_base *p_item) {
	if (p_list->_item == NULL) {
		p_list->_item = p_item;
	} else {
		bencoding_item_base *p_node = p_list->_item;
		while (p_node->_p_next)
			p_node = p_node->_p_next;
		p_node->_p_next = p_item;
	}
	return 1;
}

int tp_add_announce(TORRENT_PARSER *p_torrent, bencoding_string *p_ann) {
	p_ann->_base._p_next = NULL;
	if (p_torrent->_ann_list == NULL) {
		p_torrent->_ann_list = p_ann;
	} else {
		bencoding_string *p_node = p_torrent->_ann_list;
		while (p_node->_base._p_next)
			p_node = (bencoding_string *) p_node->_base._p_next;
		p_node->_base._p_next = (bencoding_item_base *) p_ann;
	}
	return 1;
}
void hptp_torrent_destroy(TORRENT_PARSER *p_torrent) {
	if (p_torrent->_title_name != NULL) {
		sd_free(p_torrent->_title_name);
		p_torrent->_title_name = NULL;
	}
	if (p_torrent->_piece_hash_ptr != NULL) {
		sd_free(p_torrent->_piece_hash_ptr);
		p_torrent->_piece_hash_ptr = NULL;
	}

	bencoding_string *p_node = p_torrent->_ann_list;
	while (p_node != NULL) {
		sd_free(p_node->_str);
		bencoding_string *p_del = p_node;
		p_node = (bencoding_string *) p_node->_base._p_next;
		sd_free((void *) p_del);
	}
	p_torrent->_ann_list = NULL;

	list_clear(&p_torrent->_tracker_list);

	TORRENT_FILE_INFO *p_file = p_torrent->_file_list;
	while (p_file != NULL) {
		if (p_file->_file_name)
			sd_free(p_file->_file_name);
		if (p_file->_file_path)
			sd_free(p_file->_file_path);
		TORRENT_FILE_INFO*p_del = p_file;
		p_file = p_file->_p_next;
		sd_free(p_del);
	}

	p_torrent->_list_tail = NULL;
	p_torrent->_file_list = NULL;

	bencoding_dict_destroy(&p_torrent->_hptp_parsing_related._torrent_dict);
}

//torrent_file_info malloc/free
_int32 init_torrent_file_info_slabs(void) {
	_int32 ret_val = SUCCESS;

	if (g_torrent_file_info_slab == NULL) {
		ret_val = mpool_create_slab(sizeof(TORRENT_FILE_INFO),
		MIN_TORRENT_FILE_INFO, 0, &g_torrent_file_info_slab);
		CHECK_VALUE(ret_val);
	}
	return ret_val;
}

_int32 uninit_torrent_file_info_slabs(void) {
	_int32 ret_val = SUCCESS;

	if (g_torrent_file_info_slab != NULL) {
		ret_val = mpool_destory_slab(g_torrent_file_info_slab);
		CHECK_VALUE(ret_val);
		g_torrent_file_info_slab = NULL;
	}
	return ret_val;
}

_int32 torrent_file_info_malloc_wrap(struct tagTORRENT_FILE_INFO **pp_node) {
	_int32 ret_val = SUCCESS;
	ret_val = mpool_get_slip(g_torrent_file_info_slab, (void**) pp_node);
	CHECK_VALUE(ret_val);

	ret_val = sd_memset(*pp_node, 0, sizeof(TORRENT_FILE_INFO));
	return SUCCESS;
}

_int32 torrent_file_info_free_wrap(struct tagTORRENT_FILE_INFO *p_node) {
	sd_assert( p_node != NULL );
	if (p_node == NULL)
		return SUCCESS;
	return mpool_free_slip(g_torrent_file_info_slab, (void*) p_node);
}

int tp_convert_charset(const char *src, unsigned src_len, char *dst,
		unsigned *dst_len, enum ENCODING_MODE src_encoding,
		enum ENCODING_SWITCH_MODE convert_mode) {
	int convert_result = SUCCESS;

	if (convert_mode ==UTF8_PROTO_SWITCH || convert_mode >= DEFAULT_SWITCH ||
		convert_mode <= PROTO_SWITCH){
		if(src_encoding!=UTF_8){
			convert_result = sd_any_format_to_utf8(src, src_len, dst, dst_len);
		}
		else
			return -1;
	}
	else if (src_encoding == UTF_8) {
		if (convert_mode == GBK_SWITCH) {
			convert_result = tp_utf8_2_gbk(src, src_len, dst, dst_len);
		} else if (convert_mode == BIG5_SWITCH) {
			convert_result = tp_utf8_2_big5(src, src_len, dst, dst_len);
		} else {
			convert_result = -1;
		}
	} else if (src_encoding == GBK) {
		if (convert_mode == UTF_8_SWITCH) {
			convert_result = tp_gbk_2_utf8(src, src_len, dst, dst_len);
		} else if (convert_mode == BIG5_SWITCH) {
			convert_result = -1;
			//convert_result =tp_gbk_2_big5(src,src_len,dst,dst_len);
		} else {
			convert_result = -1;
		}
	} else if (src_encoding == BIG5) {
		if (convert_mode == GBK_SWITCH) {
			convert_result = -1;
			//convert_result =tp_big5_2_gbk(src,src_len,dst,dst_len);
		} else if (convert_mode == UTF_8_SWITCH) {
			convert_result = tp_big5_2_utf8(src, src_len, dst, dst_len);
		} else {
			convert_result = -1;
		}
	} else {
		convert_result = sd_any_format_to_utf8(src, src_len, dst, dst_len);
	}

	return convert_result;
}

_int32 tp_gbk_2_utf8(const char *p_input, _u32 input_len, char *p_output,
		_u32 *output_len) {
	_int32 ret_val = SUCCESS;

	ret_val = sd_any_format_to_utf8(p_input, input_len, p_output, output_len);
	//LOG_DEBUG("tp_gbk_2_utf8 return:%d.", ret_val);
	if (ret_val != SUCCESS)
		return BT_UNSUPPORT_GBK_2_UTF8;
	return ret_val;
}

_int32 tp_utf8_2_gbk(const char *p_input, _u32 input_len, char *p_output,
		_u32 *output_len) {
	_int32 ret_val = SUCCESS;
	ret_val = sd_utf8_2_gbk(p_input, input_len, p_output, output_len);
	//LOG_DEBUG("tp_utf8_2_gbk return:%d.", ret_val);
	if (ret_val != SUCCESS)
		return BT_UNSUPPORT_UTF8_2_GBK;
	return ret_val;
}

_int32 tp_utf8_2_big5(const char *p_input, _u32 input_len, char *p_output,
		_u32 *output_len) {
	_int32 ret_val = SUCCESS;
	ret_val = sd_utf8_2_big5(p_input, input_len, p_output, output_len);
	//LOG_DEBUG("tp_utf8_2_big5 return:%d.", ret_val);
	if (ret_val != SUCCESS)
		return BT_UNSUPPORT_UTF8_2_BIG5;
	return ret_val;
}

_int32 tp_big5_2_utf8(const char *p_input, _u32 input_len, char *p_output,
		_u32 *output_len) {
	_int32 ret_val = SUCCESS;
	ret_val = sd_big5_2_utf8(p_input, input_len, p_output, output_len);
	//LOG_DEBUG("tp_big5_2_utf8 return:%d.", ret_val);
	if (ret_val != SUCCESS)
		return BT_UNSUPPORT_BIG5_2_UTF8;
	return ret_val;
}

