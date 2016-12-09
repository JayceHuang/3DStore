
/*****************************************************************************
 *
 * Filename: 			bt_magnet_data_manager.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2010/08/09
 *	
 * Purpose:				bt_magnet_data_manager logic.
 *
 *****************************************************************************/

#if !defined(__BT_MAGNET_DATA_MANAGER_20100809)
#define __BT_MAGNET_DATA_MANAGER_20100809

#ifdef _cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#ifdef ENABLE_BT 
#include "utility/map.h"
#include "utility/bitmap.h"

#include "download_task/download_task.h"

typedef struct tagBT_MAGNET_LOGIC
{
    _u32 _file_name_index;
    _u32 _file_id;
    BITMAP _bitmap;
    _u32  _metadata_type;
    _u32 _metadata_size;
} BT_MAGNET_LOGIC;

typedef struct tagBT_MAGNET_DATA_MANAGER
{
    struct tagBT_MAGNET_TASK *_bt_magnet_task_ptr;
    char _file_path[MAX_FILE_PATH_LEN];
    char _file_name[MAX_FILE_NAME_LEN];
    MAP _pipe_logic_map;
    _u32 _file_name_index;
} BT_MAGNET_DATA_MANAGER;

/* public platform */
_int32 bt_magnet_data_manager_init( BT_MAGNET_DATA_MANAGER *p_data_manager, struct tagBT_MAGNET_TASK *p_bt_magnet_task,
    char file_path[], char file_name[] );

_int32 bt_magnet_pipe_compare(void *E1, void *E2);

_int32 bt_magnet_data_manager_uninit( BT_MAGNET_DATA_MANAGER *p_data_manager );

_int32 bt_magnet_data_manager_write_data( void *pipe_data_manager, void *p_data_pipe, 
    _u32 index, char *p_data, _u32 data_len );

_int32 bt_magnet_data_manager_remove_pipe( void *pipe_data_manager, void *p_data_pipe );

_int32 bt_magnet_data_manager_set_metadata_size( void *pipe_data_manager, void *p_data_pipe, 
    _u32 data_size, _u32 piece_count );

_int32 bt_magnet_logic_get_metadata_type( void *pipe_data_manager, void *p_data_pipe, _u32 *p_data_type );

_int32 bt_magnet_logic_set_metadata_type( void *pipe_data_manager, void *p_data_pipe, _u32 data_type );

_int32 bt_magnet_data_manager_get_infohash( void *pipe_data_manager, unsigned char **pp_info_hash );

/* private platform */
_int32 bt_magnet_data_manager_get_pipe_logic( BT_MAGNET_DATA_MANAGER *p_data_manager, void *p_data_pipe, 
    BT_MAGNET_LOGIC **pp_magnet_logic, BOOL is_create );

_int32 bt_magnet_logic_create( BT_MAGNET_DATA_MANAGER *p_data_manager, BT_MAGNET_LOGIC **pp_magnet_logic );

_int32 bt_magnet_logic_destory( BT_MAGNET_DATA_MANAGER *p_data_manager, BT_MAGNET_LOGIC *p_magnet_logic );

_int32 bt_magnet_logic_get_file_name( BT_MAGNET_DATA_MANAGER *p_data_manager, _u32 file_name_index, char file_full_path[MAX_FULL_PATH_LEN] );

#endif /* #ifdef ENABLE_BT */

#ifdef _cplusplus
}
#endif

#endif // !defined(__BT_MAGNET_DATA_MANAGER_20100809)


