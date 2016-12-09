

/*****************************************************************************
 *
 * Filename: 			bt_pipe_reader.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2009/03/19
 *	
 * Purpose:				Basic bt_pipe_reader struct.
 *
 *****************************************************************************/

#if !defined(__BT_PIPE_READER_20090319)
#define __BT_PIPE_READER_20090319

#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"
#ifdef ENABLE_BT 

#include "bt_download/bt_data_manager/bt_data_reader.h"
#include "bt_download/bt_data_manager/bt_range_switch.h"
#include "data_manager/file_manager_interface.h"
#include "data_manager/data_receive.h"
#include "asyn_frame/msg.h"
#include "utility/list.h"
#include "utility/range.h"
//#include "bt_download/bt_data_manager/bt_data_manager_interface.h"
//struct tagREAD_RANGE_INFO;

#define MAX_NUM_OF_PIPE_READER 5

typedef struct tagREAD_RANGE_INFO_FOR_FILE
{
	char * data_buffer;
	//READ_RANGE_INFO* _read_range_info;
	BT_RANGE bt_range;
	_u32 _file_index;
	RANGE _padding_range;//piece在padding后所处子文件中的range
} READ_RANGE_INFO_FOR_FILE;

typedef struct tagBT_PIPE_READER
{
	BT_DATA_READER _bt_data_reader;
	char *_data_buffer_ptr;
	_u32 _buffer_len;
	BT_RANGE _bt_range;
	BOOL is_set_range;
	_u32 _file_index;
	RANGE _padding_range;//piece在padding后所处子文件中的range
	struct tagBT_PIPE_READER_MANAGER *_bt_data_reader_mgr_ptr;
} BT_PIPE_READER;

typedef struct tagBT_PIPE_READER_MANAGER
{
	//LIST _bt_data_reader_list;
	BT_PIPE_READER _pipe_reader;
	char* _range_data_buffer;
	//_u32 _buffer_len;
	_u32 _data_len;
	_u32 _total_data_len;
	_u32 _receved_data_len;

	BT_RANGE * _bt_range_ptr;

	struct tagBT_DATA_MANAGER *_bt_data_mgr_ptr;
	void* _p_call_back;
	void *_p_user;
	
	_u32  _add_range_timer_id;
	LIST * read_range_info_list_for_file;

	BOOL is_clear;

} BT_PIPE_READER_MANAGER;

_int32 init_bpr( void );
_int32 uninit_bpr( void );
_int32 bpr_pipe_reader_mgr_malloc_wrap(BT_PIPE_READER_MANAGER **pp_node);
_int32 bpr_pipe_reader_mgr_free_wrap(BT_PIPE_READER_MANAGER *p_node);
_int32 bprm_init_list( LIST *p_pipe_reader_mgr_list );
_int32 bprm_uninit_list( LIST *p_pipe_reader_mgr_list );

_int32 bprm_init_struct( BT_PIPE_READER_MANAGER *p_pipe_reader_mgr ,struct tagBT_DATA_MANAGER *p_bt_data_mgr,const BT_RANGE *p_bt_range, char *p_range_data_buffer,_u32 total_data_len,_u32 data_len,
	void* p_call_back, void *p_user,LIST * read_range_info_list_for_file);
_int32 bprm_uninit_struct(BT_PIPE_READER_MANAGER *p_pipe_reader_mgr );
_int32 bprm_add_read_range(BT_PIPE_READER_MANAGER *p_pipe_reader_mgr  );
//_int32 bprm_add_read_ranges(BT_PIPE_READER_MANAGER *p_pipe_reader_mgr ,LIST * p_read_range_info_list );
_int32 bprm_read_data_result_handler(BT_PIPE_READER_MANAGER *p_pipe_reader_mgr,BT_PIPE_READER *p_pipe_reader,  _int32 read_result, _u32 read_len );
_int32 bprm_handle_add_range_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid);
_int32 bprm_failure_exit(BT_PIPE_READER_MANAGER *p_pipe_reader_mgr ,_int32 err_code );
_int32 bprm_finished(BT_PIPE_READER_MANAGER *p_pipe_reader_mgr, _int32 read_result, _u32 read_len );
_int32 bprm_clear(BT_PIPE_READER_MANAGER *p_pipe_reader_mgr);

//_int32 bpr_pipe_reader_malloc_wrap(BT_PIPE_READER **pp_node);
//_int32 bpr_pipe_reader_free_wrap(BT_PIPE_READER *p_node);
_int32 bpr_init_struct( BT_PIPE_READER *p_pipe_reader,BT_PIPE_READER_MANAGER *p_bt_data_reader_mgr,char* data_buffer,_u32 data_len,_u32 file_index,RANGE *_padding_range);
_int32 bpr_uninit_struct( BT_PIPE_READER *p_pipe_reader );
_int32 bpr_add_read_bt_range( BT_PIPE_READER *p_pipe_reader,BT_RANGE *p_bt_range );
_int32 bpr_read_data( void *p_user, _u32 file_index, RANGE_DATA_BUFFER *p_range_data_buffer, notify_read_result p_call_back );
_int32 bpr_add_data( void *p_user, _u32 offset, const char *p_buffer_data, _u32 data_len );
_int32 bpr_read_data_result_handler( void *p_user, _int32 read_result, _u32 read_len );



#endif /* #ifdef ENABLE_BT */




#ifdef __cplusplus
}
#endif

#endif // !defined(__BT_PIPE_READER_20090319)


