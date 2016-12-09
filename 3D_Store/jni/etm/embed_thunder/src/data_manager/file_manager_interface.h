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


#if !defined(__FILE_MANAGER_INTERFACE_20080808)
#define __FILE_MANAGER_INTERFACE_20080808

#ifdef __cplusplus
extern "C" 
{
#endif

#include "data_manager/data_receive.h"

struct tagTMP_FILE;

/*****************************************************************************
* User Call Back Function
* Notes: These functions had better return SUCCESS.
*****************************************************************************/
typedef _int32 (*notify_write_result) (struct tagTMP_FILE *p_file_struct, void *p_user, RANGE_DATA_BUFFER_LIST *p_buffer_list, _int32 write_result );
typedef _int32 (*notify_read_result) (struct tagTMP_FILE *p_file_struct, void *p_user, RANGE_DATA_BUFFER *data_buffer, _int32 read_result, _u32 read_len );
typedef _int32 (*notify_file_create) (struct tagTMP_FILE *p_file_struct, void *p_user, _int32 create_result);
typedef _int32 (*notify_file_close)  (struct tagTMP_FILE *p_file_struct, void *p_user, _int32 close_result);


/*****************************************************************************
 * Interface name : init_file_manager_module
 *****************************************************************************
 * In : 
 *      NULL
 * Out :
 *      NULL
 * Return : 
 *		0 - success;  other values - error
 *****************************************************************************
 * Description :
 *		Init file manager module.
 *****************************************************************************/
_int32 init_file_manager_module(void);


/*****************************************************************************
 * Interface name : uninit_file_manager_module
 *****************************************************************************
 * In : 
 *      NULL
 * Out :
 *      NULL
 * Return : 
 *		0 - success;  other values - error
 *****************************************************************************
 * Description :
 *		Uninit file manager module.
 *****************************************************************************/
_int32 uninit_file_manager_module(void);



/*****************************************************************************
 * Interface name : fm_create_file_struct
 *****************************************************************************
 * In : 
 *		const char               *p_filename
 *		const char               *filepath
 *		void               *p_user
 *      notify_file_create call_back
 * Out :
 *		TMP_FILE           **pp_file_struct
 * Return : 
 *		0 - success;
 *      INVALID_ARGUMENT - para error;
 *      FM_CFG_FILE_ERROR - file error;
 *****************************************************************************
 * Description :
 *		Create file struct.
 *****************************************************************************/
_int32 fm_create_file_struct( const char *p_file_name, const char *p_file_path, _u64 file_size, void *p_user, notify_file_create p_call_back, struct tagTMP_FILE **pp_file_struct ,_u32 write_mode);


/* @Simple Function@ */
_int32 fm_init_file_info( struct tagTMP_FILE *p_file_struct, _u64 file_size, _u32 block_size );


/*****************************************************************************
 * Interface name : fm_change_file_size
 *****************************************************************************
 * In : 
 *		TMP_FILE               *p_file_struct
 *		_u64                   file_size
 *		void               *p_user
 * Return : 
 *		0 - success;
 *****************************************************************************
 * Description :
 *		Change file size. 
 *****************************************************************************/
_int32 fm_change_file_size( struct tagTMP_FILE *p_file_struct, _u64 file_size, void *p_user, notify_file_create p_call_back );


/*****************************************************************************
 * Interface name : fm_file_write_buffer_list
 *****************************************************************************
 * In : 
 *		TMP_FILE               *p_file_struct
 *		RANGE_DATA_BUFFER_LIST *p_buffer_list
 *		notify_write_result    call_back
 *      void                   *p_user
 * Out :
 *		None
 * Return : 
 *		0 - success;  other values - error
 *****************************************************************************
 * Description :
 *      Buffer will be free automatic.
 *		Call the call_back function when all the buffers write success or error.
 *      When the function return SUCCESS, the data buffer in the RANGE_DATA_BUFFER
 *      must be freed by the file manager whenever success or failed.
 *****************************************************************************/
_int32  fm_file_write_buffer_list( struct tagTMP_FILE *p_file_struct, RANGE_DATA_BUFFER_LIST *p_buffer_list, notify_write_result p_call_back, void *p_user );


/*****************************************************************************
 * Interface name : fm_file_asyn_read_buffer
 *****************************************************************************
 * In : 
 *		TMP_FILE           *p_file_struct
 *		RANGE_DATA_BUFFER  *data_buffer
 *		notify_read_result call_back
 *      void               *p_user
 * Out :
 *		None
 * Return : 
 *		0 - success;  other values - error
 *****************************************************************************
 * Description :
 *		Call the call_back function when the buffer asyn read success or error.
 *****************************************************************************/
_int32  fm_file_asyn_read_buffer( struct tagTMP_FILE *p_file_struct, RANGE_DATA_BUFFER *p_data_buffer, notify_read_result p_call_back, void *p_user );


/*****************************************************************************
 * Interface name : fm_file_syn_read_buffer
 *****************************************************************************
 * In : 
 *		TMP_FILE           *p_file_struct
 *		RANGE_DATA_BUFFER  *data_buffer
 * Out :
 *		None
 * Return : 
 *		0 - success;  other values - error
 *****************************************************************************
 * Description :
 *		Asyn read the data buffer.
 *****************************************************************************/
_int32  fm_file_syn_read_buffer( struct tagTMP_FILE *p_file_struct, RANGE_DATA_BUFFER *p_data_buffer );


/*****************************************************************************
 * Interface name : fm_close
 *****************************************************************************
 * In : 
 *		TMP_FILE          *p_file_struct
 *		notify_file_close call_back
 *      void              *p_user
 * Out :
 *		None
 * Return : 
 *		0 - success;  other values - error
 *****************************************************************************
 * Description :
 *		Destroy the file struct.
 *****************************************************************************/
_int32 fm_close( struct tagTMP_FILE *p_file_struct, notify_file_close p_call_back, void *p_user );//?????


/*****************************************************************************
 * Interface name : fm_file_change_name
 *****************************************************************************
 * In : 
 *		TMP_FILE          *p_file_struct
 *		char                 *p_new_file_name
 * Out :
 *		None
 * Return : 
 *		0 - success;  other values - error
 *****************************************************************************
 * Description :
 *		Change the file name.
 *****************************************************************************/
_int32 fm_file_change_name( struct tagTMP_FILE *p_file_struct, const char *p_new_file_name );


/*****************************************************************************
 * Interface name : fm_load_cfg_block_index_array
 *****************************************************************************
 * In : 
 *		TMP_FILE *p_file_struct
 *		_u32     cfg_file_id
 * Out :
 *		None
 * Return : 
 *		0 - success;  other values - error
 *****************************************************************************
 * Description :
 *		
 *****************************************************************************/
_int32 fm_load_cfg_block_index_array( struct tagTMP_FILE *p_file_struct, _u32 cfg_file_id  );


/*****************************************************************************
 * Interface name : fm_flush_cfg_block_index_array
 *****************************************************************************
 * In : 
 *		TMP_FILE *p_file_struct
 *		_u32     cfg_file_id
 * Out :
 *		None
 * Return : 
 *		0 - success;  other values - error
 *****************************************************************************
 * Description :
 *		
 *****************************************************************************/
_int32 fm_flush_cfg_block_index_array( struct tagTMP_FILE *p_file_struct, _u32 cfg_file_id );

/* @Simple Function@ */
BOOL fm_file_is_created( struct tagTMP_FILE *p_file_struct );

/* @Simple Function@ */
BOOL fm_filesize_is_valid( struct tagTMP_FILE *p_file_struct );


#ifdef __cplusplus
}
#endif

#endif // !defined(__FILE_MANAGER_INTERFACE_20080808)
