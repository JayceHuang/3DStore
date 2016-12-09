

/*****************************************************************************
 *
 * Filename: 			bt_data_manager.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2009/02/24
 *	
 * Purpose:				Basic bt_data_manager struct.
 *
 *****************************************************************************/

#if !defined(__BT_DATA_MANAGER_20090224)
#define __BT_DATA_MANAGER_20090224

#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"

#ifdef ENABLE_BT
#include "bt_download/bt_data_manager/bt_range_download_info.h"

#include "bt_download/bt_data_manager/bt_file_manager.h"
#include "bt_download/bt_data_manager/bt_range_switch.h"
#include "bt_download/bt_data_manager/bt_pipe_reader.h"
#include "bt_download/bt_data_manager/bt_piece_checker.h"

#include "data_manager/correct_manager.h"
#include "data_manager/range_manager.h"
#include "utility/bitmap.h"

struct _tag_task;


typedef struct tagBT_DATA_MANAGER
{
	BT_FILE_MANAGER _bt_file_manager;
	CORRECT_MANAGER _correct_manager;
	RANGE_MANAGER _range_manager;
	
	BT_RANGE_DOWNLOAD_INFO _range_download_info;
	
	BT_RANGE_SWITCH _bt_range_switch;
	BT_PIECE_CHECKER _bt_piece_check;
	
	LIST _bt_pipe_reader_mgr_list; /* BT_PIPE_READER_MANAGER */
	
	char _data_path[MAX_FILE_PATH_LEN];
	_u32 _data_path_len;
	_int32 _data_status_code;	
	struct _tag_task *_task_ptr;
	BOOL _IS_VOD_MOD;
	_u32 _dap_file_index;
	_u32 _start_pos_index;
	_u32 _conrinue_times;
	BITMAP _piece_bitmap;
	RANGE_LIST _need_read_tmp_range_list;
	BOOL _is_reading_tmp_range;
} BT_DATA_MANAGER;

#endif /* #ifdef ENABLE_BT */

#ifdef __cplusplus
}
#endif

#endif // !defined(__BT_DATA_MANAGER_20090224)


