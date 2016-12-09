

/*****************************************************************************
 *
 * Filename: 			bt_range_switch.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2009/02/23
 *	
 * Purpose:				Basic bt_range_switch struct.
 *
 *****************************************************************************/

#if !defined(__BT_RANGE_SWITCH_20090223)
#define __BT_RANGE_SWITCH_20090223

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#ifdef ENABLE_BT 
#include "utility/range.h"
#include "utility/list.h"

struct tagTORRENT_PARSER;

typedef struct tagFILE_RANGE_INFO
{
	_u32 _file_index;
	_u64 _file_offset;
	_u64 _file_size;
	_u32 _pos_p2sp_view;
	_u32 _right_padding_size;
	_u32 _file_range_num;
	BOOL _is_need_download;
} FILE_RANGE_INFO;

typedef struct tagBT_RANGE_SWITCH
{
	FILE_RANGE_INFO* _file_range_info_array;
	_u32 _file_num;
	_u32 _piece_size;
	_u32 _last_piece_size;
	_u32 _piece_num;
	_u64 _file_size;
} BT_RANGE_SWITCH;

typedef struct tagPIECE_RANGE_INFO
{
	_u32 _piece_index;//这里的piece为非跨文件的边界块
	_u32 _file_index;
	RANGE _padding_range;//piece在padding后所处子文件中的range
	EXACT_RANGE _padding_exact_range;//piece在padding后所处的在子文件内的精确range
} PIECE_RANGE_INFO;

typedef struct tagSUB_FILE_PADDING_RANGE_INFO
{
	_u32 _file_index;
	RANGE _padding_range;//相当大文件的range
} SUB_FILE_PADDING_RANGE_INFO;


typedef struct tagREAD_RANGE_INFO
{
	_u32 _file_index;
	_u32 _range_length;
	RANGE _padding_range;//piece在padding后所处子文件中的range
	EXACT_RANGE _padding_exact_range;//piece在padding后所处的在子文件内的精确range
	BOOL _is_tmp_file;
} READ_RANGE_INFO;

typedef _u64 (*brs_get_file_pos)( FILE_RANGE_INFO *p_file_range_info );

_int32 brs_init_module( void );
_int32 brs_uninit_module( void );

_int32 brs_init_struct( BT_RANGE_SWITCH *p_bt_range_switch, struct tagTORRENT_PARSER *p_torrent_parser,
	_u32 *p_need_download_file_array, _u32 need_download_file_num );
_int32 brs_uninit_struct( BT_RANGE_SWITCH *p_bt_range_switch );

_int32 brs_padding_range_to_bt_range( const BT_RANGE_SWITCH *p_bt_range_switch, const RANGE *p_padding_range, BT_RANGE *p_bt_range );
_int32 brs_range_to_extra_piece( const BT_RANGE_SWITCH *p_bt_range_switch, const RANGE *p_padding_range, _u32 *p_first_piece, _u32 *p_last_piece );

_int32 brs_padding_range_to_file_range( const BT_RANGE_SWITCH *p_bt_range_switch, _u32 file_index, const RANGE *p_padding_range, RANGE *p_file_range );


_int32 brs_bt_range_to_padding_range( const BT_RANGE_SWITCH *p_bt_range_switch,
	const BT_RANGE *p_bt_range, RANGE *p_padding_range );

_int32 brs_piece_to_extra_padding_range( const BT_RANGE_SWITCH *p_bt_range_switch, _u32 piece_index, RANGE *p_padding_range );

//将bt_range转成包含bt_range的最小padding_range
_int32 brs_bt_range_to_extra_padding_range( const BT_RANGE_SWITCH *p_bt_range_switch,
	const BT_RANGE *p_bt_range, RANGE *p_padding_range );

_int32 brs_file_range_to_padding_range( const BT_RANGE_SWITCH *p_bt_range_switch, _u32 file_index, 
    const RANGE *p_file_range, RANGE *p_padding_range );

_int32 brs_search_file_index( const BT_RANGE_SWITCH *p_bt_range_switch, _u64 range_pos, _u64 range_length, brs_get_file_pos p_get_file_pos, _u32 *p_range_first_file_index, _u32 *p_range_last_file_index );

_int32 brs_piece_search_file_index( const BT_RANGE_SWITCH *p_bt_range_switch, _u32 piece_index, _u32 *p_first_file, _u32 *p_last_file );
//_int32 brs_padding_range_search_file_index( const BT_RANGE_SWITCH *p_bt_range_switch, RANGE *p_padding_range, _u32 *p_first_file, _u32 *p_last_file );
_int32 brs_get_padding_range_file_index_list( const BT_RANGE_SWITCH *p_bt_range_switch, RANGE *p_padding_range, LIST *p_list );
void brs_release_padding_range_file_index_list( LIST *p_list );

_u64 brs_get_file_padding_pos( FILE_RANGE_INFO *p_file_range_info );

_u64 brs_get_file_bt_pos( FILE_RANGE_INFO *p_file_range_info );

_int32 brs_padding_range_to_sub_file_range( const BT_RANGE_SWITCH *p_bt_range_switch, const RANGE *p_padding_range, _u32 *p_file_index, RANGE *p_sub_file_range );


/*****************************************************************************
 * 通过range得到子文件序号,子文件内部的range,还有piece信息列表.
 * 列表里面的piece为此range所关联的所有非跨文件的piece.
 ****************************************************************************/
//p_range不能跨文件!!!
_int32 brs_pipe_put_range_get_info( const BT_RANGE_SWITCH *p_bt_range_switch, const RANGE *p_padding_range, _u32 *p_file_index, BOOL *p_is_tmp_file, RANGE *p_sub_file_range, LIST *p_piece_info_list );
void brs_release_piece_range_info_list( LIST *p_piece_info_list );


_int32 brs_bt_range_to_read_range_info_list( const BT_RANGE_SWITCH *p_bt_range_switch, BT_RANGE *p_bt_range, LIST *p_read_range_info_list );
_int32 brs_piece_to_range_info_list( const BT_RANGE_SWITCH *p_bt_range_switch, _u32 piece_index, RANGE *p_padding_range, LIST *p_read_range_info_list );

void brs_release_read_range_info_list( LIST *p_read_range_info_list );

void brs_release_read_range_info( READ_RANGE_INFO *p_read_range_info );


_int32 brs_get_padding_range_from_file_index( const BT_RANGE_SWITCH *p_bt_range_switch, _u32 file_index, RANGE *p_range );
_int32 brs_get_inner_piece_range_from_file_index( const BT_RANGE_SWITCH *p_bt_range_switch, _u32 file_index, _u32 *p_begin_piece_index, _u32 *p_end_piece_index );

_int32 brs_get_piece_range_from_file_index( const BT_RANGE_SWITCH *p_bt_range_switch, _u32 file_index, _u32 *p_begin_piece_index, _u32 *p_end_piece_index );

_u32 brs_get_piece_num( const BT_RANGE_SWITCH *p_bt_range_switch );

_u32 brs_get_piece_size( const BT_RANGE_SWITCH *p_bt_range_switch );

_u32 brs_get_target_piece_size( const BT_RANGE_SWITCH *p_bt_range_switch, _u32 piece_index );
_int32 brs_sub_file_exact_range_to_bt_range( const BT_RANGE_SWITCH *p_bt_range_switch, _u32 file_index, const EXACT_RANGE *p_exact_range, BT_RANGE *p_bt_range );

_int32 brs_get_file_p2sp_pos( const BT_RANGE_SWITCH *p_bt_range_switch, _u32 file_index, _u32 *p_p2sp_pos );
_int32 brs_get_file_size( const BT_RANGE_SWITCH *p_bt_range_switch, _u32 file_index, _u64 *p_file_size );


//piece_range_info malloc/free
_int32 init_piece_range_info_slabs(void);

_int32 uninit_piece_range_info_slabs(void);

_int32 piece_range_info_malloc_wrap(struct tagPIECE_RANGE_INFO **pp_node);

_int32 piece_range_info_free_wrap(struct tagPIECE_RANGE_INFO *p_node);



//read_range_info malloc/free
_int32 init_read_range_info_slabs(void);

_int32 uninit_read_range_info_slabs(void);

_int32 read_range_info_malloc_wrap(struct tagREAD_RANGE_INFO **pp_node);

_int32 read_range_info_free_wrap(struct tagREAD_RANGE_INFO *p_node);


//sub_file_padding_range_info malloc/free
_int32 init_sub_file_padding_range_info_slabs(void);

_int32 uninit_sub_file_padding_range_info_slabs(void);

_int32 sub_file_padding_range_info_malloc_wrap(struct tagSUB_FILE_PADDING_RANGE_INFO **pp_node);

_int32 sub_file_padding_range_info_free_wrap(struct tagSUB_FILE_PADDING_RANGE_INFO *p_node);
#endif /* #ifdef ENABLE_BT */

#ifdef __cplusplus
}
#endif

#endif // !defined(__BT_RANGE_SWITCH_20090223)



