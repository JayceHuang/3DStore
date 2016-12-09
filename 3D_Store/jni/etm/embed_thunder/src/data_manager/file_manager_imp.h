

/*****************************************************************************
 *
 * Filename: 			file_manager_imp.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2008/08/08
 *	
 * Purpose:				Implement the file manager's functions.
 *
 *****************************************************************************/


#if !defined(__FILE_MANAGER_IMP_20080808)
#define __FILE_MANAGER_IMP_20080808

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
//#ifndef XUNLEI_MODE
#include "data_manager/file_manager_xl.h"
#include "data_manager/file_manager.h"
#include "asyn_frame/msg.h"
#include "asyn_frame/device.h"

/*********************************************************************
 * The relation function about create file.
 *********************************************************************/
_int32 fm_create_and_init_struct( const char *p_filename, const char *p_filepath, _u64 file_size, TMP_FILE **pp_file_struct );

_int32 fm_handle_create_file( TMP_FILE *p_file_struct, void *p_user, notify_file_create p_call_back );

_int32 fm_op_open( OP_PARA_FS_OPEN *p_fso_para, MSG_FILE_CREATE_PARA *p_user_para, _u32 *msg_id );

_int32 fm_cancel_open_msg( TMP_FILE *p_file_struct );

/*********************************************************************
 * The relation function about R/W file.
 *********************************************************************/
 
 /* generates blocks from range. Return: SUCCESS or errors from malloc. */
_int32 fm_generate_block_list( TMP_FILE *p_file_struct, RANGE_DATA_BUFFER *p_range_buffer, _u16 op_type, void *buffer_para, void *p_call_back, void *p_user );

/* try to handle the fist block data in the write block list. */
_int32 fm_handle_write_block_list(  TMP_FILE *p_file_struct  );

/* handle the fist block data in the order write block list. */
_int32 fm_handle_order_write_block_list( TMP_FILE *p_file_struct );

/* Write the block data. Do the real write or change the write_block_list .*/
_int32 fm_try_to_write_block_data( TMP_FILE *p_file_struct, BLOCK_DATA_BUFFER *p_block_data_buffer );

/* Asyn Write or Read the block data from the disk */
_int32 fm_asyn_handle_block_data( TMP_FILE *p_file_struct, BLOCK_DATA_BUFFER *p_block_data_buffer, _u32 block_disk_index );

_int32 fm_op_rw( _u32 device_id, _u16 op, OP_PARA_FS_RW *p_frw_para, MSG_FILE_RW_PARA *p_user_para );

/* @Simple Function@,  Notes: reset the block_index_array. */
_int32 fm_set_block_index_array( DATA_BLOCK_INDEX_ARRAY *block_index_array, _u32 disk_block_index, _u32 logic_block_index );

/* when write block data, maybe cause some block R/W operations, so create the BLOCK_DATA_BUFFER struct. */
_int32 fm_generate_tmp_write_block(TMP_FILE *p_file_struct, _u32 disk_block_index, BOOL is_write, RANGE_DATA_BUFFER_LIST *p_buffer_list, char *p_data_buffer, BLOCK_DATA_BUFFER **pp_block_data_buffer );

_int32 fm_handle_asyn_read_block_list( TMP_FILE *p_file_struct );



/*********************************************************************
 * The relation function about syn read file.
 *********************************************************************/
 
_int32 fm_handle_syn_read_block_list( TMP_FILE *p_file_struct );

/* Syn Write or Read the block data from the disk */
_int32 fm_syn_handle_block_data( TMP_FILE *p_file_struct, BLOCK_DATA_BUFFER *p_block_data_buffer, _u32 block_disk_index, _u32 file_id );

/* Used in syn read file. */
_int32 get_syn_op_file_id( TMP_FILE *p_file_struct, _u32 *file_id );


/*********************************************************************
 * The function called by the asyn_frame, which calls the user.
 *********************************************************************/
_int32 fm_callback( const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid);

_int32 fm_write_callback( const MSG_INFO *msg_info, _int32 errcode );

_int32 fm_read_callback( const MSG_INFO *msg_info, _int32 errcode );

_int32 fm_create_callback( const MSG_INFO *msg_info, _int32 errcode );

_int32 fm_close_callback( const MSG_INFO *msg_info, _int32 errcode );


/*********************************************************************
 * The relation function close file.
 *********************************************************************/
_int32 fm_handle_close_file( TMP_FILE *p_file_struct );

_int32 fm_op_close( _u32 device_id , MSG_FILE_CLOSE_PARA *p_close_rara );


/*********************************************************************
 * The relation function about cfg file.
 *********************************************************************/

BOOL fm_check_cfg_block_index_array( TMP_FILE *p_file_struct );

/* @Simple Function@,  Return: the virtual block's disk index. */
BOOL fm_get_cfg_disk_block_index( DATA_BLOCK_INDEX_ARRAY *block_index_array , _u32 logic_block_index, _u32 *disk_block_index  );

_u64 fm_get_tmp_filesize( TMP_FILE *p_file_struct );

//#endif  /* !XUNLEI_MODE */

#ifdef __cplusplus
}
#endif


#endif // !defined(__FILE_MANAGER_IMP_20080808)

