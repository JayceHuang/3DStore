/*****************************************************************************
 *
 * Filename: 			torrent_parser_interface.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2008/09/16
 *	
 * Purpose:				Parser torrent file interface.
 *
 *****************************************************************************/

#if !defined(__TORRENT_PARSER_INTERFACE_20080916)
#define __TORRENT_PARSER_INTERFACE_20080916

#ifdef __cplusplus
extern "C" {
#endif

#include "utility/define.h"
#include "utility/list.h"
#include "torrent_parser/torrent_parser.h"
#include "torrent_parser/torrent_parser_struct_define.h"


struct tagTORRENT_PARSER;

#ifdef ENABLE_BT
_int32 init_tp_module(void);
_int32 uninit_tp_module(void);

_int32 tp_get_seed_info( char *seed_path,enum ENCODING_SWITCH_MODE encoding_switch_mode, TORRENT_SEED_INFO **pp_seed_info );
_int32 tp_get_seed_info_from_mem(char *seed_data, _u32 len, _u32 encoding_switch_mode,
	TORRENT_SEED_INFO **pp_seed_info);
_int32 tp_release_seed_info( TORRENT_SEED_INFO *p_seed_info );

_int32 tp_create( struct tagTORRENT_PARSER **pp_torrent_parser, _u32 encoding_switch_mode);
_int32 tp_destroy( struct tagTORRENT_PARSER *p_torrent_parser );
_int32 tp_task_parse_seed( struct tagTORRENT_PARSER *p_torrent_parser, char *seed_path );


_int32 tp_parse_seed( TORRENT_PARSER *p_torrent, const char *seed_path);
_int32 tp_parse_seed_from_mem(TORRENT_PARSER *p_torrent, const char *seed_data, uint32_t len);
_int32 tp_get_seed_title_name( struct tagTORRENT_PARSER *p_torrent_parser, char **pp_title_name );
_u32 tp_get_seed_file_num( struct tagTORRENT_PARSER *p_torrent_parser );
_int32 tp_get_file_info( struct tagTORRENT_PARSER *p_torrent_parser, _u32 file_index, TORRENT_FILE_INFO **pp_file_info );

_int32 tp_get_file_path_and_name( struct tagTORRENT_PARSER *p_torrent_parser, _u32 file_index,
		char *p_path_buffer, _u32 *p_buffer_len,
		char *p_name_buffer, _u32 *p_name_len );

_int32 tp_get_file_name( struct tagTORRENT_PARSER *p_torrent_parser, _u32 file_index,
		char *p_name_buffer, _u32 *p_name_len );
_int32 tp_get_file_name_ptr( struct tagTORRENT_PARSER *p_torrent_parser, _u32 file_index, char **pp_name_buffer );

_int32 tp_get_file_info_detail( struct tagTORRENT_PARSER *p_torrent_parser, _u32 file_index,
		char *p_full_path_buffer, _u32 *p_buffer_len, _u64 *p_file_size );

_int32 tp_get_file_info_hash( struct tagTORRENT_PARSER *p_torrent_parser, unsigned char **p_file_info_hash );
enum ENCODING_SWITCH_MODE tp_get_default_encoding_mode(void);
_int32 tp_set_default_switch_mode( _u32 switch_type );

_u64 tp_get_file_total_size( struct tagTORRENT_PARSER *p_torrent_parser );
_int32 tp_get_sub_file_size( struct tagTORRENT_PARSER *p_torrent_parser, _u32 file_index, _u64 *p_file_size );

_u32 tp_get_piece_size( struct tagTORRENT_PARSER *p_torrent_parser );
_u32 tp_get_piece_num( struct tagTORRENT_PARSER *p_torrent_parser );

_int32 tp_get_piece_hash( struct tagTORRENT_PARSER *p_torrent_parser, _u8 **pp_piece_hash, _u32 *p_piece_hash_len );

_int32 tp_get_tracker_url( struct tagTORRENT_PARSER *p_torrent_parser, LIST **pp_tracker_url_list );


BOOL tp_has_dir( struct tagTORRENT_PARSER *p_torrent_parser );


_int32 init_torrent_parser_slabs(void) ;

_int32 uninit_torrent_parser_slabs(void) ;

//declared here ,defined in bencoding in bencoding.c

_int32 init_bc_slabs(void);
_int32 uninit_bc_slabs(void);

#endif /*  #ifdef ENABLE_BT */
#ifdef __cplusplus
}
#endif

#endif // !defined(__TORRENT_PARSER_INTERFACE_20080916)
