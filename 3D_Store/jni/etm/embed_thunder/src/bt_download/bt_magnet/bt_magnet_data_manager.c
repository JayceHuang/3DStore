#include "bt_magnet_data_manager.h"
#ifdef ENABLE_BT 
#include "platform/sd_fs.h"
#include "utility/utility.h"
#include "utility/errcode.h"
#include "utility/string.h"
#include "utility/mempool.h"

#include "bt_magnet_task.h"

#include "utility/logid.h"
#define	LOGID	LOGID_BT_MAGNET
#include "utility/logger.h"

_int32 bt_magnet_data_manager_init( BT_MAGNET_DATA_MANAGER *p_data_manager, struct tagBT_MAGNET_TASK *p_bt_magnet_task,
    char file_path[], char file_name[] )
{
    _int32 ret_val = SUCCESS;
    
    LOG_DEBUG("[magnet_data_manager=0x%x] bt_magnet_data_manager_init, file_path:%s, file_name:%s", p_data_manager, file_path, file_name );

    sd_strncpy( p_data_manager->_file_path, file_path, MAX_FILE_PATH_LEN );
    sd_strncpy( p_data_manager->_file_name, file_name, MAX_FILE_NAME_LEN );
    
    p_data_manager->_bt_magnet_task_ptr = p_bt_magnet_task;
    map_init( &p_data_manager->_pipe_logic_map, bt_magnet_pipe_compare );  
	
    return ret_val;
}

_int32 bt_magnet_pipe_compare(void *E1, void *E2)
{
	_u32 value1 = (_u32)E1;
	_u32 value2 = (_u32)E2;
	return value1 > value2 ? 1 : ( value1 < value2 ? -1 : 0 );  
}

_int32 bt_magnet_data_manager_uninit( BT_MAGNET_DATA_MANAGER *p_data_manager )
{
    _int32 ret_val = SUCCESS;
    BT_MAGNET_LOGIC *p_magnet_logic = NULL;
    MAP_ITERATOR cur_node = MAP_BEGIN( p_data_manager->_pipe_logic_map );
    
    LOG_DEBUG("[magnet_data_manager=0x%x] bt_magnet_data_manager_uninit", p_data_manager );

    while( cur_node != MAP_END(p_data_manager->_pipe_logic_map) )
    {
        p_magnet_logic = (BT_MAGNET_LOGIC *)MAP_VALUE(cur_node);
        bt_magnet_logic_destory( p_data_manager, p_magnet_logic );
        cur_node = MAP_NEXT( p_data_manager->_pipe_logic_map, cur_node );
    }
    map_clear( &p_data_manager->_pipe_logic_map );
    return ret_val;
}

_int32 bt_magnet_data_manager_write_data( void *pipe_data_manager, void *p_data_pipe, 
    _u32 index, char *p_data, _u32 data_len )
{
    _int32 ret_val = SUCCESS;
    BT_MAGNET_DATA_MANAGER *p_data_manager = (BT_MAGNET_DATA_MANAGER *)pipe_data_manager;
    BT_MAGNET_LOGIC *p_magnet_logic = NULL;
    char file_full_path[MAX_FULL_PATH_LEN] = {0};
    _u32 write_size = 0;
    _u32 bit_count = 0;
    _u64 file_pos = index * BT_METADATA_PIECE_SIZE;
    
    LOG_DEBUG("[magnet_data_manager=0x%x] bt_magnet_data_manager_write_data, pipe:0x%x, index:%u, data_len:%u",
        p_data_manager, p_data_pipe, index, data_len );

    ret_val = bt_magnet_data_manager_get_pipe_logic( p_data_manager, 
        p_data_pipe, &p_magnet_logic, TRUE );
    CHECK_VALUE( ret_val );

    if( p_magnet_logic->_file_id == INVALID_FILE_ID )
    {
        ret_val = bt_magnet_logic_get_file_name( p_data_manager, p_magnet_logic->_file_name_index, file_full_path );
        CHECK_VALUE( ret_val ); 
        
        ret_val = sd_open_ex( file_full_path, O_FS_CREATE, &p_magnet_logic->_file_id );
        CHECK_VALUE( ret_val ); 
    }
    
    ret_val = sd_pwrite( p_magnet_logic->_file_id, p_data, data_len, file_pos, &write_size );
    CHECK_VALUE( ret_val ); 

    sd_assert( data_len = write_size );

    bit_count = bitmap_bit_count( &p_magnet_logic->_bitmap ); 
    ret_val = bitmap_set( &p_magnet_logic->_bitmap, index, 1 );
    CHECK_VALUE( ret_val ); 
    
    if( bitmap_range_is_all_set(&p_magnet_logic->_bitmap, 0, bit_count-1) )
    {
        ret_val = bt_magnet_logic_get_file_name( p_data_manager, p_magnet_logic->_file_name_index, file_full_path );
        CHECK_VALUE( ret_val ); 
        
        bmt_notify_torrent_ok( p_data_manager->_bt_magnet_task_ptr, file_full_path);
    }
    return ret_val;
}

_int32 bt_magnet_data_manager_remove_pipe( void *pipe_data_manager, void *p_data_pipe )
{
    BT_MAGNET_DATA_MANAGER *p_data_manager = (BT_MAGNET_DATA_MANAGER *)pipe_data_manager;
    BT_MAGNET_LOGIC *p_magnet_logic = NULL;

    LOG_DEBUG("[magnet_data_manager=0x%x] bt_magnet_data_manager_remove_pipe, pipe:0x%x",
        p_data_manager, p_data_pipe );

    
    MAP_ITERATOR cur_node = MAP_BEGIN( p_data_manager->_pipe_logic_map );

    map_find_iterator( &p_data_manager->_pipe_logic_map, p_data_pipe, &cur_node );

    if( cur_node == MAP_END(p_data_manager->_pipe_logic_map) ) 
    {
        return SUCCESS;
    }

    p_magnet_logic = (BT_MAGNET_LOGIC *)MAP_VALUE( cur_node );
    
    bt_magnet_logic_destory( p_data_manager, p_magnet_logic );
    map_erase_iterator( &p_data_manager->_pipe_logic_map, cur_node );

    return SUCCESS;
}


_int32 bt_magnet_data_manager_set_metadata_size( void *pipe_data_manager, void *p_data_pipe, 
    _u32 data_size, _u32 piece_count )
{
    _int32 ret_val = SUCCESS;
    BT_MAGNET_DATA_MANAGER *p_data_manager = (BT_MAGNET_DATA_MANAGER *)pipe_data_manager;
    BT_MAGNET_LOGIC *p_magnet_logic = NULL;
    LOG_DEBUG("[magnet_data_manager=0x%x] bt_magnet_data_manager_set_metadata_size, pipe:0x%x, data_size:%u, piece_size:%u",
        p_data_manager, p_data_pipe, data_size, piece_count );
        
    ret_val = bt_magnet_data_manager_get_pipe_logic( p_data_manager, 
        p_data_pipe, &p_magnet_logic, TRUE );
    CHECK_VALUE( ret_val );

    ret_val = bitmap_resize( &p_magnet_logic->_bitmap, piece_count );
    CHECK_VALUE( ret_val );
    
    return SUCCESS;
}

_int32 bt_magnet_logic_get_metadata_type( void *pipe_data_manager, void *p_data_pipe, _u32 *p_data_type )
{
    _int32 ret_val = SUCCESS;
    BT_MAGNET_DATA_MANAGER *p_data_manager = (BT_MAGNET_DATA_MANAGER *)pipe_data_manager;
    BT_MAGNET_LOGIC *p_magnet_logic = NULL;
    

    ret_val = bt_magnet_data_manager_get_pipe_logic( p_data_manager, 
        p_data_pipe, &p_magnet_logic, FALSE );
    CHECK_VALUE( ret_val );

    *p_data_type = p_magnet_logic->_metadata_type;
    LOG_DEBUG("[magnet_data_manager=0x%x] bt_magnet_logic_get_metadata_type, pipe:0x%x, data_type:%u",
        p_data_manager, p_data_pipe, *p_data_type );
    
    return SUCCESS;
}

_int32 bt_magnet_logic_set_metadata_type( void *pipe_data_manager, void *p_data_pipe, _u32 data_type )
{
    _int32 ret_val = SUCCESS;
    BT_MAGNET_DATA_MANAGER *p_data_manager = (BT_MAGNET_DATA_MANAGER *)pipe_data_manager;
    BT_MAGNET_LOGIC *p_magnet_logic = NULL;
    
    LOG_DEBUG("[magnet_data_manager=0x%x] bt_magnet_logic_set_metadata_type, pipe:0x%x, data_type:%u",
        p_data_manager, p_data_pipe, data_type );
    
    ret_val = bt_magnet_data_manager_get_pipe_logic( p_data_manager, 
        p_data_pipe, &p_magnet_logic, FALSE );
    CHECK_VALUE( ret_val );

    p_magnet_logic->_metadata_type = data_type;
    
    return SUCCESS;
}

_int32 bt_magnet_data_manager_get_infohash( void *pipe_data_manager, unsigned char **pp_info_hash )
{
    BT_MAGNET_DATA_MANAGER *p_data_manager = (BT_MAGNET_DATA_MANAGER *)pipe_data_manager;
    *pp_info_hash = p_data_manager->_bt_magnet_task_ptr->_info_hash;
    return SUCCESS;
}


_int32 bt_magnet_data_manager_get_pipe_logic( BT_MAGNET_DATA_MANAGER *p_data_manager, void *p_data_pipe, 
    BT_MAGNET_LOGIC **pp_magnet_logic, BOOL is_create )
{
    _int32 ret_val = SUCCESS;
    PAIR map_pair;
    MAP_ITERATOR cur_node = MAP_BEGIN( p_data_manager->_pipe_logic_map );

    *pp_magnet_logic = NULL;
    
    map_find_iterator( &p_data_manager->_pipe_logic_map, p_data_pipe, &cur_node );

    if( cur_node != MAP_END(p_data_manager->_pipe_logic_map) ) 
    {
        *pp_magnet_logic = (BT_MAGNET_LOGIC *)MAP_VALUE( cur_node );
		sd_assert( *pp_magnet_logic != NULL );
        return SUCCESS;
    }

    if( !is_create ) return SUCCESS;
    
    ret_val = bt_magnet_logic_create( p_data_manager, pp_magnet_logic );
    CHECK_VALUE( ret_val );
	sd_assert( *pp_magnet_logic != NULL );

    map_pair._key = p_data_pipe;
    map_pair._value = *pp_magnet_logic;

    ret_val = map_insert_node( &p_data_manager->_pipe_logic_map, &map_pair );
    if( ret_val != SUCCESS )
    {
        bt_magnet_logic_destory( p_data_manager, *pp_magnet_logic );
        return ret_val;
    }
    LOG_DEBUG("[magnet_data_manager=0x%x] bt_magnet_data_manager_get_pipe_logic, pipe:0x%x, magnet_logic:0x%x",
        p_data_manager, p_data_pipe, *pp_magnet_logic );

    return SUCCESS;
}

_int32 bt_magnet_logic_create( BT_MAGNET_DATA_MANAGER *p_data_manager, BT_MAGNET_LOGIC **pp_magnet_logic )
{
    _int32 ret_val = SUCCESS;
    
    LOG_DEBUG("[magnet_data_manager=0x%x] bt_magnet_logic_create, magnet_logic:0x%x",
        p_data_manager, *pp_magnet_logic );

    ret_val = sd_malloc( sizeof(BT_MAGNET_LOGIC), (void**)pp_magnet_logic );
    CHECK_VALUE( ret_val );

    (*pp_magnet_logic)->_file_name_index = p_data_manager->_file_name_index++;
    (*pp_magnet_logic)->_file_id = INVALID_FILE_ID;
    (*pp_magnet_logic)->_metadata_type = 0;

    ret_val = bitmap_init( &(*pp_magnet_logic)->_bitmap );
    CHECK_VALUE( ret_val );

    return ret_val;
}

_int32 bt_magnet_logic_destory( BT_MAGNET_DATA_MANAGER *p_data_manager, BT_MAGNET_LOGIC *p_magnet_logic )
{
    _int32 ret_val = SUCCESS;
    char file_full_path[MAX_FULL_PATH_LEN] = {0};
    
    LOG_DEBUG("[magnet_data_manager=0x%x] bt_magnet_logic_destory, magnet_logic:0x%x",
        p_data_manager, p_magnet_logic );

    bitmap_uninit( &p_magnet_logic->_bitmap );
    if( p_magnet_logic->_file_id != INVALID_FILE_ID )
    {
        sd_close_ex( p_magnet_logic->_file_id );
        p_magnet_logic->_file_id = INVALID_FILE_ID;
    }
    
    ret_val = bt_magnet_logic_get_file_name( p_data_manager, p_magnet_logic->_file_name_index, file_full_path );
    sd_assert( ret_val == SUCCESS ); 
    
    sd_delete_file(file_full_path);
    sd_free( p_magnet_logic );
    return SUCCESS;
}

_int32 bt_magnet_logic_get_file_name( BT_MAGNET_DATA_MANAGER *p_data_manager, _u32 file_name_index, char file_full_path[MAX_FULL_PATH_LEN] )
{
    _int32 ret_val = SUCCESS;
    char name_index[10] = {0};
    
    ret_val = sd_u32toa( file_name_index, name_index, 10, 10 );
    CHECK_VALUE( ret_val );
    
    sd_strncpy( file_full_path, p_data_manager->_file_path, MAX_FULL_PATH_LEN );
    sd_strcat(file_full_path, name_index, MAX_FULL_PATH_LEN - sd_strlen(file_full_path) );

    LOG_DEBUG("[magnet_data_manager=0x%x] bt_magnet_logic_get_file_name, file_name_index:%u, file_name:%s",
        p_data_manager, file_name_index, file_full_path );

    return SUCCESS;
} 

#endif /*  ENABLE_BT  */

