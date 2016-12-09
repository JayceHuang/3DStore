
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


#if !defined(__FILE_MANAGER_IMP_XL_20080808)
#define __FILE_MANAGER_IMP_XL_20080808

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
//#ifdef XUNLEI_MODE
#include "data_manager/file_manager.h"
#include "asyn_frame/msg.h"
#include "asyn_frame/device.h"


/*********************************************************************
 * The relation function about create file.
 *********************************************************************/
_int32 fm_create_and_init_struct_xl( const char *p_filename, const char *p_filepath, _u64 file_size, TMP_FILE **pp_file_struct );

_int32 fm_handle_create_file_xl( TMP_FILE *p_file_struct, void *p_user, notify_file_create p_call_back );

_int32 fm_op_open_xl( OP_PARA_FS_OPEN *p_fso_para, MSG_FILE_CREATE_PARA *p_user_para, _u32 *msg_id );

_int32 fm_cancel_open_msg_xl( TMP_FILE *p_file_struct );

/*********************************************************************
 * The relation function about R/W file.
 *********************************************************************/
 
 /* generates ranges from range. Return: SUCCESS or errors from malloc. */
#ifdef _WRITE_BUFFER_MERGE
void fm_merge_write_range_list( struct tagTMP_FILE *p_file_struct, LIST_ITERATOR *list_node );
#endif

_int32 fm_generate_range_list( struct tagTMP_FILE *p_file_struct, RANGE_DATA_BUFFER *p_range_buffer, void *p_call_back_buffer, void *p_call_back, void *p_user, _u16 op_type );

/* try to handle the fist range data in the write range list. */
_int32 fm_handle_write_range_list(  TMP_FILE *p_file_struct  );
_int32 fm_syn_close_and_delete_file( struct tagTMP_FILE *p_file_struct );

_int32 fm_handle_asyn_read_range_list( TMP_FILE *p_file_struct );

/* Asyn Write or Read the range data from the disk */
_int32 fm_asyn_handle_range_data( TMP_FILE *p_file_struct, RW_DATA_BUFFER *p_rw_data_buffer );

_int32 fm_op_rw_xl( _u32 device_id, OP_PARA_FS_RW *p_frw_para, MSG_FILE_RW_PARA *p_user_para );


/*********************************************************************
 * The relation function close file.
 *********************************************************************/
_int32 fm_handle_close_file_xl( TMP_FILE *p_file_struct );

void fm_cancel_list_rw_op_xl( LIST *p_list );

_int32 fm_handle_cancel_rw_buffer( struct tagTMP_FILE *p_file_struct, RW_DATA_BUFFER *p_rw_data_buffer );

_int32 fm_op_close_xl( _u32 device_id , MSG_FILE_CLOSE_PARA *p_close_rara );


/*********************************************************************
 * The function called by the asyn_frame, which calls the user.
 *********************************************************************/
_int32 fm_callback_xl( const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid);

_int32 fm_reopen_callback( const MSG_INFO *msg_info, _int32 errcode );

_int32 fm_create_callback_xl( const MSG_INFO *msg_info, _int32 errcode );

_int32 fm_write_callback_xl( const MSG_INFO *msg_info, _int32 errcode );

_int32 fm_read_callback_xl( const MSG_INFO *msg_info, _int32 errcode );

_int32 fm_close_callback_xl( const MSG_INFO *msg_info, _int32 errcode );



/*********************************************************************
 * The other functions.
 *********************************************************************/

/* Get file full path. */
_int32 fm_get_file_full_path( TMP_FILE *p_file_struct, char *file_full_path, _u32 buffer_size );
//#endif  /* XUNLEI_MODE */


#ifdef __cplusplus
}
#endif

#endif // !defined(__FILE_MANAGER_IMP_XL_20080808)

