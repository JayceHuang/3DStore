#ifdef ENABLE_BT
#include "utility/define.h"
#include "torrent_parser/torrent_parser_interface.h"
#include "torrent_parser/torrent_parser.h"

#include "utility/mempool.h"
#include "utility/utility.h"

#include "utility/logid.h"
#define LOGID LOGID_BT_TORRENT_PARSER
#include "utility/logger.h"
#include "platform/sd_fs.h"

static _u32 g_seed_read_max_len;
static enum ENCODING_SWITCH_MODE g_bt_encoding_switch_mode;
static SLAB* g_torrent_parser_slab = NULL;
_int32 init_tp_module(void) {
	_int32 ret_val = SUCCESS;
	LOG_DEBUG( "init_tp_module." );

	ret_val = init_torrent_parser_slabs();
	CHECK_VALUE(ret_val);

	ret_val = init_torrent_file_info_slabs();
	if (ret_val != SUCCESS)
		goto ErrorHanle;

	//ret_val = init_torrent_seed_info_slabs();
	//if( ret_val!=SUCCESS ) goto ErrorHanle;
#if 0  /*move to et module*/
	ret_val = init_bc_slabs();
	if (ret_val != SUCCESS)
		goto ErrorHanle;
#endif
	g_bt_encoding_switch_mode = UTF_8_SWITCH;
	g_seed_read_max_len = 1024;
	LOG_DEBUG( "init_tp_module switch_mode:%d g_seed_read_max_len=%d", g_bt_encoding_switch_mode,g_seed_read_max_len );
	return SUCCESS;

	ErrorHanle: uninit_tp_module();
	return ret_val;
}

_int32 uninit_tp_module(void) {
	LOG_DEBUG( "uninit_tp_module." );
#if 0
	uninit_bc_slabs();
#endif
	//uninit_torrent_seed_info_slabs();

	uninit_torrent_file_info_slabs();

	uninit_torrent_parser_slabs();

	return SUCCESS;
}

_int32 tp_get_seed_info(char *seed_path, _u32 encoding_switch_mode,
		TORRENT_SEED_INFO **pp_seed_info) {
	TORRENT_PARSER *p_torrent = NULL;
	TORRENT_SEED_INFO* p_seed;
	int result = sd_malloc(sizeof(TORRENT_SEED_INFO), (void **) &p_seed);

	if (result != SUCCESS) {
		*pp_seed_info = NULL;
		return result;
	}

	p_torrent = hptp_torrent_init(NULL);
	if (p_torrent == NULL) {
		sd_free(p_seed);
		*pp_seed_info = NULL;
		return OUT_OF_MEMORY;
	}
	p_torrent->_switch_mode = encoding_switch_mode;
	result = tp_parse_seed(p_torrent, seed_path);

	if (result != 0) {
		sd_free(p_seed);
		*pp_seed_info = NULL;
	} else {
		int i;
		memcpy(p_seed->_info_hash, p_torrent->_info_hash, INFO_SHA1_HASH_LEN);
		p_seed->_encoding = p_torrent->_encoding;
		p_seed->_file_num = p_torrent->_file_num;
		p_seed->_file_total_size = p_torrent->_file_total_size;
		p_seed->_title_name_len = p_torrent->_title_name_len;

		*pp_seed_info = p_seed;
		sd_malloc(p_torrent->_file_num * sizeof(TORRENT_FILE_INFO*),
				(void **) &p_seed->_file_info_array_ptr);

		TORRENT_FILE_INFO *p_node = p_torrent->_file_list;
		for (i = 0; i < p_torrent->_file_num; ++i) {
			p_seed->_file_info_array_ptr[i] = p_node;
			p_node = p_node->_p_next;
		}
		p_torrent->_file_list = NULL;
		p_torrent->_list_tail = NULL;

		memcpy(p_seed->_title_name, p_torrent->_title_name,
				p_torrent->_title_name_len+1);
	}

	hptp_torrent_destroy(p_torrent);
	return result;
}

_int32 tp_get_seed_info_from_mem(char *seed_data, _u32 len, _u32 encoding_switch_mode,
	TORRENT_SEED_INFO **pp_seed_info) {
	TORRENT_PARSER *p_torrent = NULL;
	TORRENT_SEED_INFO* p_seed;
	int result = sd_malloc(sizeof(TORRENT_SEED_INFO), (void **)&p_seed);

	if (result != SUCCESS) {
		*pp_seed_info = NULL;
		return result;
	}

	p_torrent = hptp_torrent_init(NULL);
	if (p_torrent == NULL) {
		sd_free(p_seed);
		*pp_seed_info = NULL;
		return OUT_OF_MEMORY;
	}
	p_torrent->_switch_mode = encoding_switch_mode;
	result = tp_parse_seed_from_mem(p_torrent, seed_data, len);

	if (result != 0) {
		sd_free(p_seed);
		*pp_seed_info = NULL;
	}
	else {
		int i;
		memcpy(p_seed->_info_hash, p_torrent->_info_hash, INFO_SHA1_HASH_LEN);
		p_seed->_encoding = p_torrent->_encoding;
		p_seed->_file_num = p_torrent->_file_num;
		p_seed->_file_total_size = p_torrent->_file_total_size;
		p_seed->_title_name_len = p_torrent->_title_name_len;

		*pp_seed_info = p_seed;
		sd_malloc(p_torrent->_file_num * sizeof(TORRENT_FILE_INFO*),
			(void **)&p_seed->_file_info_array_ptr);

		TORRENT_FILE_INFO *p_node = p_torrent->_file_list;
		for (i = 0; i < p_torrent->_file_num; ++i) {
			p_seed->_file_info_array_ptr[i] = p_node;
			p_node = p_node->_p_next;
		}
		p_torrent->_file_list = NULL;
		p_torrent->_list_tail = NULL;

		memcpy(p_seed->_title_name, p_torrent->_title_name,
			p_torrent->_title_name_len + 1);
	}

	hptp_torrent_destroy(p_torrent);
	return result;
}

_int32 tp_parse_seed(TORRENT_PARSER *p_torrent, const char *seed_path) {
	uint64_t bytes_not_parsed = 0;
	uint32_t torrent_fd = 0;

	_int32 result = SUCCESS;

	if ((result = sd_open_ex(seed_path, O_FS_RDONLY, &torrent_fd)) != SUCCESS) {
		return result;
	}

	if ((result = sd_filesize(torrent_fd, &bytes_not_parsed)) != SUCCESS) {
		return result;
	}

	const _u32 buff_len = 81920;
	char *buf;
	result = sd_malloc(buff_len, (void **) &buf);
	if (result != SUCCESS) {
		sd_close_ex(torrent_fd);
		return result;
	}

	while (bytes_not_parsed > 0) {
		_u32 read_size =
				bytes_not_parsed >= buff_len ? buff_len : bytes_not_parsed;
		bytes_not_parsed -= read_size;
		_u32 bytes_read = read_size;

		if ((result = sd_read(torrent_fd, buf, bytes_read, &bytes_read))
				!= SUCCESS) {
			break;
		}

		if (bytes_read < read_size) {
			result = BT_SEED_FILE_NOT_EXIST;
		} else {
		}
		int ret = hptp_parse_torrent_part(p_torrent, buf, read_size);
		if (ret < read_size && p_torrent->_hptp_parsing_related._p_item != NULL) {
			result = BT_SEED_BAD_ENCODING;
			break;
		}
	}

	sd_free(buf);

	if (result == SUCCESS) {
		result = hptp_finish(p_torrent);
	} else if (result == BT_SEED_FILE_NOT_EXIST) {
	}
	sd_close_ex(torrent_fd);
	return result;
}

_int32 tp_parse_seed_from_mem(TORRENT_PARSER *p_torrent, const char *seed_data, uint32_t len) {
	int ret = hptp_parse_torrent_part(p_torrent, seed_data, len);
	return hptp_finish(p_torrent);	 
}

_int32 tp_release_seed_info(TORRENT_SEED_INFO *p_seed_info) {
	_u32 file_count = 0;
	if (p_seed_info == NULL)
		return SUCCESS;

	LOG_DEBUG( "tp_release_seed_info." );

	for (file_count = 0; file_count < p_seed_info->_file_num; file_count++) {
		TORRENT_FILE_INFO *p_file_info =
				p_seed_info->_file_info_array_ptr[file_count];
		SAFE_DELETE(p_file_info->_file_name);
		SAFE_DELETE(p_file_info->_file_path);
		SAFE_DELETE(p_file_info);
	}
	SAFE_DELETE(p_seed_info->_file_info_array_ptr);
	SAFE_DELETE(p_seed_info);
	return SUCCESS;
}

_int32 tp_create(struct tagTORRENT_PARSER **pp_torrent_parser,
		_u32 encoding_switch_mode) {
	_int32 ret_val = SUCCESS;

	struct tagTORRENT_PARSER *p_torrent_parser = hptp_torrent_init(NULL);

	LOG_DEBUG( "tp_create.encoding_switch:%d", encoding_switch_mode );

	*pp_torrent_parser = p_torrent_parser;

	if (encoding_switch_mode > DEFAULT_SWITCH)
		return BT_UNSUPPORT_SWITCH_TYPE;

	if (encoding_switch_mode == DEFAULT_SWITCH) {
		p_torrent_parser->_switch_mode = tp_get_default_encoding_mode();
	} else {
		p_torrent_parser->_switch_mode = encoding_switch_mode;
	}

	return SUCCESS;
}

_int32 tp_destroy(struct tagTORRENT_PARSER *p_torrent_parser) {
	hptp_torrent_destroy(p_torrent_parser);
	return 0;
}

_int32 tp_task_parse_seed(struct tagTORRENT_PARSER *p_torrent_parser,
		char *seed_path) {
	return tp_parse_seed(p_torrent_parser, seed_path);
}

_int32 tp_get_seed_title_name(struct tagTORRENT_PARSER *p_torrent_parser,
		char **pp_title_name) {
	*pp_title_name = p_torrent_parser->_title_name;
	LOG_DEBUG( "tp_get_seed_title_name:%s.", p_torrent_parser->_title_name );
	return SUCCESS;
}

_u32 tp_get_seed_file_num(struct tagTORRENT_PARSER *p_torrent_parser) {
	LOG_DEBUG( "tp_get_seed_file_num:%u.", p_torrent_parser->_file_num );
	return p_torrent_parser->_file_num;
}

_int32 tp_get_file_info(struct tagTORRENT_PARSER *p_torrent_parser,
		_u32 file_index, TORRENT_FILE_INFO **pp_file_info) {
	if (file_index >= p_torrent_parser->_file_num)
		return BT_ERROR_FILE_INDEX;

	TORRENT_FILE_INFO*p_node = p_torrent_parser->_file_list;
	while (p_node != NULL) {
		if (p_node->_file_index == file_index) {
			*pp_file_info = p_node;
			return 0;
		}
		p_node = p_node->_p_next;
	}

	return BT_FILE_INDEX_NOT_DOWNLOAD;
}

_int32 tp_get_file_path_and_name(struct tagTORRENT_PARSER *p_torrent_parser,
		_u32 file_index, char *p_path_buffer, _u32 *p_path_buffer_len,
		char *p_name_buffer, _u32 *p_name_len) {
	TORRENT_FILE_INFO *p_file_info = NULL;
	_int32 ret_val = tp_get_file_info(p_torrent_parser, file_index,
			&p_file_info);
	CHECK_VALUE(ret_val);

	LOG_DEBUG( "tp_get_file_path_and_name." );
	if (*p_path_buffer_len <= p_file_info->_file_path_len
			|| *p_name_len <= p_file_info->_file_name_len) {
		*p_path_buffer_len = p_file_info->_file_path_len + 1;
		*p_name_len = p_file_info->_file_name_len + 1;
		return BUFFER_OVERFLOW;
	}

	sd_strncpy(p_path_buffer, p_file_info->_file_path,
			p_file_info->_file_path_len);
	p_path_buffer[p_file_info->_file_path_len] = '\0';
	*p_path_buffer_len = p_file_info->_file_path_len;

	sd_strncpy(p_name_buffer, p_file_info->_file_name, p_file_info->_file_name_len);
	p_name_buffer[p_file_info->_file_name_len] = '\0';
	*p_name_len = p_file_info->_file_name_len;

	return SUCCESS;
}

_int32 tp_get_file_name(struct tagTORRENT_PARSER *p_torrent_parser,
		_u32 file_index, char *p_name_buffer, _u32 *p_name_len) {
	TORRENT_FILE_INFO *p_file_info = NULL;
	_int32 ret_val = tp_get_file_info(p_torrent_parser, file_index,
			&p_file_info);
	CHECK_VALUE(ret_val);

	LOG_DEBUG( "tp_get_file_name." );
	if (*p_name_len <= p_file_info->_file_name_len) {
		return BUFFER_OVERFLOW;
	}

	*p_name_len = p_file_info->_file_name_len;

	sd_strncpy(p_name_buffer, p_file_info->_file_name, *p_name_len);

	p_name_buffer[p_file_info->_file_name_len] = '\0';

	LOG_DEBUG( "tp_get_file_name:file_name:%s, name_len:%d",
			p_name_buffer, *p_name_len );

	return SUCCESS;
}

_int32 tp_get_file_name_ptr(struct tagTORRENT_PARSER *p_torrent_parser,
		_u32 file_index, char **pp_name_buffer) {
	TORRENT_FILE_INFO *p_file_info = NULL;
	_int32 ret_val = tp_get_file_info(p_torrent_parser, file_index,
			&p_file_info);
	CHECK_VALUE(ret_val);
	*pp_name_buffer = p_file_info->_file_name;

	return SUCCESS;
}

_int32 tp_get_file_info_detail(struct tagTORRENT_PARSER *p_torrent_parser,
		_u32 file_index, char *p_full_path_buffer, _u32 *p_buffer_len,
		_u64 *p_file_size) {
	TORRENT_FILE_INFO *p_file_info = NULL;
	_int32 ret_val = tp_get_file_info(p_torrent_parser, file_index,
			&p_file_info);
	CHECK_VALUE(ret_val);

	LOG_DEBUG( "tp_get_file_full_path." );
	if (*p_buffer_len
			< p_file_info->_file_path_len + p_file_info->_file_name_len+2) {
		*p_buffer_len = p_file_info->_file_path_len
				+ p_file_info->_file_name_len + 2;
		return BUFFER_OVERFLOW;
	}

	

	
	if(p_file_info->_file_path_len>0){
		*p_buffer_len = p_file_info->_file_path_len + p_file_info->_file_name_len + 1;
		ret_val = sd_strncpy(p_full_path_buffer, p_file_info->_file_path,
					p_file_info->_file_path_len);
			CHECK_VALUE(ret_val);
		p_full_path_buffer[p_file_info->_file_path_len]='/';
		ret_val = sd_strncpy(p_full_path_buffer + p_file_info->_file_path_len+1,
			p_file_info->_file_name, p_file_info->_file_name_len + 1);
		CHECK_VALUE(ret_val);
	}
	else{
		*p_buffer_len = p_file_info->_file_name_len;
		ret_val = sd_strncpy(p_full_path_buffer,
			p_file_info->_file_name, p_file_info->_file_name_len + 1);
		CHECK_VALUE(ret_val);
	}

	p_full_path_buffer[*p_buffer_len] = 0;

	*p_file_size = p_file_info->_file_size;
	LOG_DEBUG( "tp_get_file_info_detail:file_full_path:%s, path_len:%d ",
			p_full_path_buffer, *p_buffer_len );

	return SUCCESS;
}

_int32 tp_get_file_info_hash(struct tagTORRENT_PARSER *p_torrent_parser,
		unsigned char **p_file_info_hash) {
	LOG_DEBUG( "tp_get_file_info_hash." );
	*p_file_info_hash = p_torrent_parser->_info_hash;
	return SUCCESS;
}

enum ENCODING_SWITCH_MODE tp_get_default_encoding_mode(void) {
	LOG_DEBUG( "tp_get_default_encoding_mode:%d", g_bt_encoding_switch_mode );
	return g_bt_encoding_switch_mode;
}

_int32 tp_set_default_switch_mode(_u32 switch_type) {
//#ifdef _ICONV
	LOG_DEBUG( "tp_set_default_switch_mode:%d.", switch_type );

	if (switch_type > DEFAULT_SWITCH)
		return BT_UNSUPPORT_SWITCH_TYPE;
	g_bt_encoding_switch_mode = switch_type;
	return SUCCESS;
//#else
	//return BT_UNSUPPORT_SWITCH;
//  return SUCCESS;
//#endif
}

_u64 tp_get_file_total_size(struct tagTORRENT_PARSER *p_torrent_parser) {
	return p_torrent_parser->_file_total_size;
}

_int32 tp_get_sub_file_size(struct tagTORRENT_PARSER *p_torrent_parser,
		_u32 file_index, _u64 *p_file_size) {
	if (file_index >= p_torrent_parser->_file_num)
		return BT_ERROR_FILE_INDEX;

	TORRENT_FILE_INFO *p_node = p_torrent_parser->_file_list;
	while (p_node != NULL) {
		if (p_node->_file_index == file_index) {
			*p_file_size = p_node->_file_size;
			return SUCCESS;
		}
		p_node = p_node->_p_next;
	}

	return BT_ERROR_FILE_INDEX;
}

_u32 tp_get_piece_size(TORRENT_PARSER *p_torrent_parser) {
	LOG_DEBUG( "tp_get_piece_size, piece_size:%d", p_torrent_parser->_piece_length );
	return p_torrent_parser->_piece_length;
}

_u32 tp_get_piece_num(struct tagTORRENT_PARSER *p_torrent_parser) {
	_u32 piece_num = 0;
	if (p_torrent_parser->_file_total_size % p_torrent_parser->_piece_length
			== 0) {
		piece_num = p_torrent_parser->_file_total_size
				/ p_torrent_parser->_piece_length;
	} else {
		piece_num = p_torrent_parser->_file_total_size
				/ p_torrent_parser->_piece_length + 1;
	}LOG_DEBUG( "tp_get_piece_num, piece_num:%d", piece_num );
	return piece_num;
}

_int32 tp_get_piece_hash(struct tagTORRENT_PARSER *p_torrent_parser,
		_u8 **pp_piece_hash, _u32 *p_piece_hash_len) {
	*pp_piece_hash = p_torrent_parser->_piece_hash_ptr;
	*p_piece_hash_len = p_torrent_parser->_piece_hash_len;
	LOG_DEBUG( "tp_get_piece_hash, piece_hash_len:%d", p_torrent_parser->_piece_hash_len );

	return SUCCESS;
}

_int32 tp_get_tracker_url(struct tagTORRENT_PARSER *p_torrent_parser,
		LIST **pp_tracker_url_list) {
	*pp_tracker_url_list = &p_torrent_parser->_tracker_list;

	return SUCCESS;
}

BOOL tp_has_dir(struct tagTORRENT_PARSER *p_torrent_parser) {
	return p_torrent_parser->_is_dir_organized;
}

_int32 init_torrent_parser_slabs(void) {
	_int32 ret_val = SUCCESS;

	if (g_torrent_parser_slab == NULL) {
		ret_val = mpool_create_slab(sizeof(TORRENT_PARSER), MIN_TORRENT_PARSER,
				0, &g_torrent_parser_slab);
		CHECK_VALUE(ret_val);
	}

	return ret_val;
}

_int32 uninit_torrent_parser_slabs(void) {
	_int32 ret_val = SUCCESS;

	if (g_torrent_parser_slab != NULL) {
		ret_val = mpool_destory_slab(g_torrent_parser_slab);
		CHECK_VALUE(ret_val);
		g_torrent_parser_slab = NULL;
	}
	return ret_val;
}
#endif

