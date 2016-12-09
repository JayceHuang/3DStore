/*****************************************************************************
 *
 * Filename: 			torrent_parser_struct_define.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2008/09/16
 *	
 * Purpose:				Parser torrent file.
 *
 *****************************************************************************/

#ifndef TORRENT_PARSER_STRUCT_DEFINE_H
#define TORRENT_PARSER_STRUCT_DEFINE_H

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#include "utility/map.h"
#include "utility/list.h"
#include "utility/define_const_num.h"
#include "utility/sha1.h"

#ifdef _DK_QUERY
#include "utility/bitmap.h"

#define DHT_NODE_BITS_NUM 160
#define KAD_NODE_BITS_NUM 128

struct tagK_NODE;
struct tagNODE_INFO;

#define INFO_SHA1_HASH_LEN 20

typedef _int32 (*k_node_destory_handler)( struct tagK_NODE *p_k_node );

typedef _int32 (*node_info_creater)( _u32 ip, _u16 port, _u8 version, _u8 *p_id, _u32 id_len, struct tagNODE_INFO **pp_node_info );
typedef _int32 (*node_info_destoryer)( struct tagNODE_INFO *p_node_info );

typedef struct tagK_DISTANCE
{
	BITMAP _bit_map;

}K_DISTANCE;

typedef struct tagK_PEER_ADDR
{
	_u32 _ip;
	_u16 _port;
}K_PEER_ADDR;

typedef K_DISTANCE K_NODE_ID;

typedef struct tagK_NODE
{
	K_NODE_ID _node_id;
	K_PEER_ADDR _peer_addr;
	_u32 _last_active_time;
	_u32 _old_times;
} K_NODE;

typedef struct tagKAD_NODE
{
	K_NODE _k_node;
    //EMULE_TAG_LIST _tag_list;
    _u16 _tcp_port;
	_u8 _version;
} KAD_NODE;

typedef struct tagNODE_INFO
{
	K_PEER_ADDR _peer_addr;
    _u32 _try_times;
} NODE_INFO;

typedef struct tagKAD_NODE_INFO
{
	NODE_INFO _node_info;
    _u8 *_node_id_ptr;
	_u8 _version;
} KAD_NODE_INFO;
#endif

struct tagBC_OBJ;
struct tagBC_READER;
struct tagBC_DICT;
struct tagTORRENT_PARSER;

typedef _int32 (*bc_from_str)( struct tagBC_OBJ *p_bc_obj );
typedef _int32 (*bc_to_str)( struct tagBC_OBJ *p_bc_obj, char *p_bc_str, _u32 str_len, _u32 *p_used_len );
typedef _int32 (*bc_uninit_struct) ( struct tagBC_OBJ *p_bc_obj );
typedef _int32 (*bc_dict_handler) ( struct tagTORRENT_PARSER *p_torrent_parser, void *p_user, struct tagBC_DICT *p_bc_dict );
typedef _int32 (*bc_seed_title_handler) ( struct tagTORRENT_PARSER *p_torrent_parser, struct tagBC_DICT *p_bc_dict );


typedef struct tagBC_READER
{
	char *_bc_buffer;
	_u32 _max_read_len;
	_u32 _read_size;
	_u32 _used_str_len;
	_u32 _used_buffer_str_len;

	_u32 _file_id;
	_u32 _file_size;

	BOOL _is_calc_info_hash;
	ctx_sha1 *_sha1_ptr;
	BOOL _is_start_sha1;

	_u32 _seed_file_count;
	BOOL _is_pre_parser_seed;
	
	BOOL _is_parse_piece_hash;
	bc_dict_handler _file_dict_handler;
	void *_user_ptr;
	struct tagTORRENT_PARSER *_torrent_parser_ptr;
	BOOL _is_stop_fill_str;
} BC_PARSER;

typedef enum tagBC_TYPE
{
	INT_TYPE,
	STR_TYPE,
	LIST_TYPE,
	DICT_TYPE
} BC_TYPE;

typedef struct tagBC_OBJ
{
	BC_TYPE _bc_type;
	bc_from_str p_from_str_call_back;
	bc_to_str p_to_str_call_back;
	bc_uninit_struct p_uninit_call_back;
	BC_PARSER *_bc_parser_ptr;
} BC_OBJ;

typedef struct tagBC_INT
{
	BC_OBJ _bc_obj;
	_u64 _value;
	_u32 _bc_str_len;
} BC_INT;

typedef struct tagBC_STR
{
	BC_OBJ _bc_obj;
	char *_str_ptr;
	_u32 _str_len;
	_u32 _bc_str_len;
} BC_STR;

typedef struct tagBC_LIST
{
	BC_OBJ _bc_obj;
	LIST _list;
} BC_LIST;

typedef struct tagBC_DICT
{
	BC_OBJ _bc_obj;
	MAP _map;
	BOOL _is_file_dict;
} BC_DICT;

struct tagTORRENT_FILE_INFO;
struct tagTORRENT_SEED_INFO;

enum ENCODING_MODE {
	GBK = 0,
	UTF_8 = 1,
	BIG5 = 2,
	ENCODING_UNDEFINED,
};

enum ENCODING_SWITCH_MODE 
{ 
	PROTO_SWITCH = 0,         /* 返回原始字段(根据et_release_torrent_seed_info的_encoding字段决定编码格式)  */
	GBK_SWITCH = 1,           /*  返回GBK格式编码 */
	UTF_8_SWITCH = 2,         /* 返回UTF-8格式编码 */
	BIG5_SWITCH = 3,          /* 返回BIG5格式编码  */
	UTF8_PROTO_SWITCH = 4,    /* 返回种子文件中的utf-8字段  */
	DEFAULT_SWITCH = 5        /* 未设置输出格式(使用et_set_seed_switch_type的全局输出设置)  */
};

typedef struct tagTORRENT_FILE_INFO {
	unsigned _file_index;
	char *_file_name;
	unsigned _file_name_len;
	char *_file_path;
	unsigned _file_path_len;
	int64_t _file_offset;
	int64_t _file_size;
	struct tagTORRENT_FILE_INFO *_p_next;
} TORRENT_FILE_INFO;

typedef enum __tag_bencoding_item_type {
	BENCODING_ITEM_INTEGER,
	BENCODING_ITEM_NEGINT,
	BENCODING_ITEM_STRING,
	BENCODING_ITEM_LIST,
	BENCODING_ITEM_DICT,
} bencoding_item_type;

typedef enum __tag_hptp_parsing_state {
	dict_waiting_prefix,
	dict_waiting_key,
	dict_waiting_val,
	dict_key_parsing,
	dict_val_parsing,
	string_parsing_len,
	string_parsing_buf,
} hptp_parsing_state;

typedef struct __tag_bencoding_item_base {
	bencoding_item_type _type;
	hptp_parsing_state _state;
	unsigned _len;
	struct __tag_bencoding_item_base *_p_next;
	struct __tag_bencoding_item_base *_p_parent;
} bencoding_item_base;

typedef struct __tag_bencoding_string {
	bencoding_item_base _base;
	char *_str;
	unsigned _valid_len;
} bencoding_string;

typedef struct __tag_bencoding_dict {
	bencoding_item_base _base;
	bencoding_string *_key;
	bencoding_item_base *_item;
} bencoding_dict;

typedef struct __tag_bencoding_list {
	bencoding_item_base _base;
	bencoding_item_base *_item;
} bencoding_list;

typedef struct __tag_bencoding_integer {
	bencoding_item_base _base;
	long long _value;
} bencoding_int;

typedef struct __tag_hptp_parsing_related {
	bencoding_dict _torrent_dict;
	bencoding_item_base *_p_item;
	ctx_sha1 _sha1_ctx;
	uint64_t _torrent_size;
	uint64_t _bytes_parsed;
	uint8_t _update_info_sha1;
} hptp_parsing_related;
typedef struct tagTORRENT_PARSER {
	hptp_parsing_related _hptp_parsing_related; //It's discouraged to access _hptp_parsing_related.

	LIST _tracker_list;
	bencoding_string *_ann_list;

	TORRENT_FILE_INFO* _file_list;
	TORRENT_FILE_INFO* _list_tail;
	uint32_t _file_num;
	uint64_t _file_total_size;
	uint8_t _multi_file_refered;
	uint8_t _info_hash[20];
	uint8_t _info_hash_valid;

	char *_title_name;
	int _title_name_len;
	enum ENCODING_MODE _encoding;
	enum ENCODING_SWITCH_MODE _switch_mode;
	char _is_dir_organized;
	long long _piece_length;
	uint8_t* _piece_hash_ptr;
	uint32_t _piece_hash_len;
} TORRENT_PARSER;

typedef struct tagTORRENT_SEED_INFO
{
	char _title_name[MAX_FILE_NAME_LEN];
	_u32 _title_name_len;
	_u64 _file_total_size;
	_u32 _file_num;
	_u32 _encoding;
	unsigned char _info_hash[INFO_HASH_LEN];
	TORRENT_FILE_INFO **_file_info_array_ptr;
} TORRENT_SEED_INFO;


struct tagTORRENT_PARSER;


#ifdef __cplusplus
}
#endif

#endif// TORRENT_PARSER_STRUCT_DEFINE_H

