/*****************************************************************************
 *
 * Filename: 			file_manager_interface.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2008/08/08
 *	
 * Purpose:				Contains the platforms provided by file manager.
 *
 *****************************************************************************/


#if !defined(__FILE_MANAGER_INTERFACE_XL_20101224)
#define __FILE_MANAGER_INTERFACE_XL_20101224

#ifdef __cplusplus
extern "C" 
{
#endif

#include "data_manager/data_receive.h"
#include "data_manager/file_manager_interface.h"


/*********************************************************************
 * Functions in "data_manager/file_manager_interface_xl.c"
 *********************************************************************/
_int32 init_file_manager_module_xl();
_int32 uninit_file_manager_module_xl();
_int32 fm_create_file_struct_xl( const char *p_file_name, const char *p_file_path, _u64 file_size, void *p_user, notify_file_create p_call_back, struct tagTMP_FILE **pp_file_struct ,_u32 write_mode);
_int32 fm_init_file_info_xl( struct tagTMP_FILE *p_file_struct, _u64 file_size, _u32 block_size );
_int32 fm_change_file_size_xl( struct tagTMP_FILE *p_file_struct, _u64 file_size, void *p_user, notify_file_create p_call_back );
_int32 fm_file_write_buffer_list_xl( struct tagTMP_FILE *p_file_struct, RANGE_DATA_BUFFER_LIST *p_buffer_list, notify_write_result p_call_back, void *p_user );
_int32 fm_file_asyn_read_buffer_xl( struct tagTMP_FILE *p_file_struct, RANGE_DATA_BUFFER *p_data_buffer, notify_read_result p_call_back, void *p_user );
_int32  fm_file_syn_read_buffer_xl( struct tagTMP_FILE *p_file_struct, RANGE_DATA_BUFFER *p_data_buffer );
_int32 fm_close_xl( struct tagTMP_FILE *p_file_struct , notify_file_close p_call_back, void *p_user );
_int32 fm_file_change_name_xl( struct tagTMP_FILE *p_file_struct, const char *p_new_file_name );
_int32 fm_load_cfg_block_index_array_xl( struct tagTMP_FILE *p_file_struct, _u32 cfg_file_id );
_int32 fm_flush_cfg_block_index_array_xl( struct tagTMP_FILE *p_file_struct, _u32 cfg_file_id );


#ifdef __cplusplus
}
#endif

#endif // !defined(__FILE_MANAGER_INTERFACE_XL_20101224)
