/*****************************************************************************
 *
 * Filename: 			bt_data_read.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2009/03/19
 *	
 * Purpose:				bt_data_reader.
 *
 *****************************************************************************/



#if !defined(__BT_DATA_READER_20090319)
#define __BT_DATA_READER_20090319

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"

#ifdef ENABLE_BT 
///////////////////
#include "data_manager/file_manager_interface.h"
#include "data_manager/data_receive.h"

struct tagTMP_FILE;

//////////////////


struct tagBT_RANGE_SWITCH;
struct tagREAD_RANGE_INFO;


typedef _int32 (*input_data_handler)( void *p_user, _u32 file_index, RANGE_DATA_BUFFER *p_range_data_buffer, notify_read_result p_call_back );
typedef _int32 (*output_data_handler)( void *p_user, _u32 offset, const char *p_buffer_data, _u32 data_len );

typedef _int32 (*read_data_result_handler)( void *p_user, _int32 read_result, _u32 read_len );

typedef struct tagBT_DATA_READER
{
	input_data_handler _input_hander_ptr;
	output_data_handler _output_hander_ptr;
	read_data_result_handler _read_data_call_back_ptr;
	void *_call_back_user_ptr;
	RANGE_DATA_BUFFER *_range_data_buffer_ptr;
	
	struct tagBT_RANGE_SWITCH *_range_switcher_ptr;

	LIST _read_range_info_list;
	struct tagREAD_RANGE_INFO *_cur_read_range_info_ptr;
	_u32 _cur_read_times;
	_u32 _finished_len;
	
} BT_DATA_READER;



_int32 bdr_init_struct( BT_DATA_READER *p_bt_data_reader, struct tagBT_RANGE_SWITCH *p_range_switcher,
	input_data_handler p_input_hander, output_data_handler p_out_put_hander, 
	read_data_result_handler p_read_data_call_back, void *p_call_back_user );

_int32 bdr_uninit_struct( BT_DATA_READER *p_bt_data_reader );

_int32 bdr_read_bt_range( BT_DATA_READER *p_bt_data_reader, BT_RANGE *p_bt_range );

_int32 bdr_handle_new_read_range_info( BT_DATA_READER *p_bt_data_reader );

_int32 bdr_handle_uncomplete_range_info( BT_DATA_READER *p_bt_data_reader );

_int32 bdr_notify_read_result( struct tagTMP_FILE *p_file_struct, void *p_user, RANGE_DATA_BUFFER *p_range_data_buffer, _int32 read_result, _u32 read_len );

_int32 bdr_read_success( BT_DATA_READER *p_bt_data_reader );

_int32 bdr_read_failed( BT_DATA_READER *p_bt_data_reader );

void bdr_read_clear( BT_DATA_READER *p_bt_data_reader );

_int32 bdr_cancel( BT_DATA_READER *p_bt_data_reader, BT_RANGE bt_range );

_u32 bdr_get_max_read_num(void);
_u32 bdr_get_read_length(void);


//bt_data_reader malloc/free
_int32 init_bt_data_reader_slabs(void);

_int32 uninit_bt_data_reader_slabs(void);

_int32 bt_data_reader_malloc_wrap(struct tagBT_DATA_READER **pp_node);

_int32 bt_data_reader_free_wrap(struct tagBT_DATA_READER *p_node);


#endif /* #ifdef ENABLE_BT */

#ifdef __cplusplus
}
#endif

#endif // !defined(__BT_DATA_READER_20090319)


