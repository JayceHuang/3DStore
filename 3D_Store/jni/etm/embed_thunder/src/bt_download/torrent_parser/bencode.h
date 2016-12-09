/*****************************************************************************
 *
 * Filename: 			bencode.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2008/09/16
 *	
 * Purpose:				Basic bencode struct.
 *
 *****************************************************************************/

#if !defined(__BENCODE_20080916)
#define __BENCODE_20080916

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#include "utility/map.h"
#include "utility/list.h"
#include "utility/define_const_num.h"
#include "utility/sha1.h"

#include "torrent_parser/torrent_parser_struct_define.h"

/* public function */

#ifdef ENABLE_BT
_int32 init_bc_slabs(void);
_int32 uninit_bc_slabs(void);

_int32 bc_parser_create( char *p_buffer, _u32 buffer_len, _u32 read_size, BC_PARSER **pp_bc_parser );

_int32 bc_parser_init_file_para( BC_PARSER *p_bc_parser, char *seed_path );

_int32 bc_parser_init_sha1_para( BC_PARSER *p_bc_parser );

void bc_parser_set_pre_parser( BC_PARSER *p_bc_parser, BOOL is_only_get_file_count );

void bc_parser_piece_hash_set( BC_PARSER *p_bc_parser, BOOL is_parse_piece_hash );

void bc_parser_set_file_dict_handler( BC_PARSER *p_bc_parser, bc_dict_handler dict_handler, struct tagTORRENT_PARSER *p_torrent_parser, void *p_user );

_int32 bc_parser_destory( BC_PARSER *p_bc_parser );

void bc_parser_get_info_hash( BC_PARSER *p_bc_parser, _u8 info_hash[20] );

_u32 bc_parser_get_seed_file_count( BC_PARSER *p_bc_parser );

_int32 bc_parser_str( BC_PARSER *p_bc_parser, BC_OBJ **pp_bc_obj );

_int32 bc_parser_get_used_str_len(BC_PARSER *p_bc_parser, _u32 *p_len);

BC_TYPE bc_get_type( BC_OBJ *p_bc_obj );

/* public function */
_int32 bc_int_create_with_value( _u64 value, BC_INT **pp_bc_int );
_int32 bc_str_create_with_value( const char *p_str, _u32 str_len, BC_STR **pp_bc_str );
_int32 bc_dict_create( BC_DICT **pp_bc_obj );
_int32 bc_dict_set_value( BC_DICT *p_bc_dict, const char *p_key, BC_OBJ *p_value );


/* private function */

void bc_parser_updata_sha1( BC_PARSER *p_bc_parser, _u8 *p_data, _u32 data_len );

_int32 bc_parser_try_to_update_buffer( BC_PARSER *p_bc_parser, _u32 low_limit );

_int32 bc_parser_file_handler( BC_PARSER *p_bc_parser, _u32 valid_strlen );

//int function
_int32 bc_int_init( BC_OBJ *p_bc_obj, BC_PARSER *p_bc_parser );
_int32 bc_int_uninit( BC_OBJ *p_bc_obj );
_int32 bc_int_from_str( BC_OBJ *p_bc_obj );
_int32 bc_int_to_str( BC_OBJ *p_bc_obj, char *p_bc_str, _u32 str_len, _u32 *p_used_len );

//str function
_int32 bc_str_init( BC_OBJ *p_bc_obj, BC_PARSER *p_bc_parser );
_int32 bc_str_uninit( BC_OBJ *p_bc_obj );
_int32 bc_str_from_str( BC_OBJ *p_bc_obj );
_int32 bc_str_to_str( BC_OBJ *p_bc_obj, char *p_bc_str, _u32 str_len, _u32 *p_used_len );

//list function
_int32 bc_list_init( BC_OBJ *p_bc_obj, BC_PARSER *p_bc_parser );
_int32 bc_list_uninit( BC_OBJ *p_bc_obj );
_int32 bc_list_from_str( BC_OBJ *p_bc_obj );
_int32 bc_list_to_str( BC_OBJ *p_bc_obj, char *p_bc_str, _u32 str_len, _u32 *p_used_len );

//dict function
_int32 bc_dict_init( BC_OBJ *p_bc_obj, BC_PARSER *p_bc_parser );
_int32 bc_dict_uninit( BC_OBJ *p_bc_obj );
_int32 bc_dict_from_str( BC_OBJ *p_bc_obj );
_int32 bc_dict_to_str( BC_OBJ *p_bc_obj, char *p_bc_str, _u32 str_len, _u32 *p_used_len );
_int32 bc_dict_get_value( BC_DICT *p_bc_obj, const char *p_key, BC_OBJ **pp_value );
_int32 bc_map_compare_fun( void *E1, void *E2 );


//int malloc/free
_int32 init_bc_int_slabs(void);
_int32 uninit_bc_int_slabs(void);
_int32 bc_int_malloc_wrap(BC_INT **pp_node);
_int32 bc_int_free_wrap(BC_INT *p_node);

//str malloc/free
_int32 init_bc_str_slabs(void);
_int32 uninit_bc_str_slabs(void);
_int32 bc_str_malloc_wrap(BC_STR **pp_node);
_int32 bc_str_free_wrap(BC_STR *p_node);

//list malloc/free
_int32 init_bc_list_slabs(void);
_int32 uninit_bc_list_slabs(void);
_int32 bc_list_malloc_wrap(BC_LIST **pp_node);
_int32 bc_list_free_wrap(BC_LIST *p_node);

//dict malloc/free
_int32 init_bc_dict_slabs(void);
_int32 uninit_bc_dict_slabs(void);
_int32 bc_dict_malloc_wrap(BC_DICT **pp_node);
_int32 bc_dict_free_wrap(BC_DICT *p_node);
#endif /* #ifdef ENABLE_BT */


#ifdef __cplusplus
}
#endif

#endif // !defined(__BENCODE_20080916)



