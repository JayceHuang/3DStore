
/*****************************************************************************
 *
 * Filename: 			file_manager.h
 *
 * Author:				PENGXIANGQIAN
 *
 * Created Data:		2008/08/08
 *	
 * Purpose:				Define the basic structs of file_manager.
 *
 *****************************************************************************/


#if !defined(__FILE_MANAGER_XL_20080714)
#define __FILE_MANAGER_XL_20080714

#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"

//#ifdef XUNLEI_MODE
#include "utility/range.h"
#include "connect_manager/resource.h"
#include "data_manager/data_receive.h"
#include "utility/mempool.h"
#include "data_manager/file_manager_interface.h"

/* Same Macro */

/**************************
 * Err Try Time
 **************************/
//#define FM_ERR_TRY_TIME         (3)

struct tagTMP_FILE;

/********************************************************
 * Used in post create file message
 ********************************************************/
//typedef struct tagMSG_FILE_CREATE_PARA
//{
//	struct tagTMP_FILE *_file_struct_ptr;
//	void *_user_ptr;
//	notify_file_create _callback_func_ptr;
//}MSG_FILE_CREATE_PARA;


/********************************************************
 * Store close file parameter
 ********************************************************/
//typedef struct tagMSG_FILE_CLOSE_PARA
//{	
//	struct tagTMP_FILE *_file_struct_ptr;
//	void *_user_ptr;
//	notify_file_close _callback_func_ptr;
//}MSG_FILE_CLOSE_PARA;


/*********************************************************************
 * Used in write/read range data and save as a list in file manager
 *********************************************************************/
typedef struct tagRW_DATA_BUFFER
{
	RANGE_DATA_BUFFER *_range_buffer_ptr;
	void *_call_back_buffer_ptr;
	void *_call_back_func_ptr;
	void *_user_ptr;
	_u16 _try_num;	
	_u16 _operation_type;
	BOOL _is_call_back;	
	BOOL _is_cancel;
#ifdef _WRITE_BUFFER_MERGE
    BOOL _is_merge;
#endif
    _int32 _error_code;
}RW_DATA_BUFFER;


/********************************************************
* Used in post write/read data message
********************************************************/
//typedef struct tagMSG_FILE_RW_PARA
//{
//	struct tagTMP_FILE *_file_struct_ptr;
//	RW_DATA_BUFFER *_rw_data_buffer_ptr;
//}MSG_FILE_RW_PARA;


/*********************************************************************
 * The core struct, contain the file basic information
 *********************************************************************/
/*
typedef struct tagTMP_FILE
{
    char _file_name[MAX_FILE_NAME_LEN];
    char _file_path[MAX_FILE_PATH_LEN];
	_u32 _file_name_len;
	_u32 _file_path_len;
    _u64 _file_size;
    _u64 _new_file_size;
	_u32 _device_id;	

	LIST _asyn_read_range_list;  //contain the asyn read operation ranges
	LIST _write_range_list;      //contain the write operation ranges
	
	MSG_FILE_CLOSE_PARA *_close_para_ptr;
	_u32 _open_msg_id;	
	
	BOOL _is_file_size_valid;	
	BOOL _is_file_created;	
	BOOL _is_closing;
	BOOL _is_reopening;	
	BOOL _is_opening;	
	BOOL _is_sending_read_op;
	BOOL _is_sending_write_op;
	BOOL _is_sending_close_op;
	
}TMP_FILE;
*/
//tmp_file malloc/free wrap
//_int32 init_tmp_file_slabs(void);
//_int32 uninit_tmp_file_slabs(void);
//_int32 tmp_file_malloc_wrap(TMP_FILE **pp_node);
//_int32 tmp_file_free_wrap(TMP_FILE *p_node);

//msg_file_create_para malloc/free wrap
#if 0
_int32 init_msg_file_create_para_slabs(void);
_int32 uninit_msg_file_create_para_slabs(void);
_int32 msg_file_create_para_malloc_wrap(MSG_FILE_CREATE_PARA **pp_node);
_int32 msg_file_create_para_free_wrap(MSG_FILE_CREATE_PARA *p_node);

//msg_file_rw_para malloc/free wrap
_int32 init_msg_file_rw_para_slabs(void);
_int32 uninit_msg_file_rw_para_slabs(void);
_int32 msg_file_rw_para_slab_malloc_wrap(MSG_FILE_RW_PARA **pp_node);
_int32 msg_file_rw_para_slab_free_wrap(MSG_FILE_RW_PARA *p_node);

//msg_file_close_para malloc/free wrap
_int32 init_msg_file_close_para_slabs(void);
_int32 uninit_msg_file_close_para_slabs(void);
_int32 msg_file_close_para_malloc_wrap(MSG_FILE_CLOSE_PARA **pp_node);
_int32 msg_file_close_para_free_wrap(MSG_FILE_CLOSE_PARA *p_node);
#endif
//rw_data_buffer malloc/free wrap
_int32 init_rw_data_buffer_slabs(void);
_int32 uninit_rw_data_buffer_slabs(void);
_int32 rw_data_buffer_malloc_wrap(RW_DATA_BUFFER **pp_node);
_int32 rw_data_buffer_free_wrap(RW_DATA_BUFFER *p_node);
#if 0
//range_data_buffer_list malloc/free wrap
_int32 init_range_data_buffer_list_slabs(void);
_int32 uninit_range_data_buffer_list_slabs(void);
_int32 range_data_buffer_list_malloc_wrap(RANGE_DATA_BUFFER_LIST **pp_node);
_int32 range_data_buffer_list_free_wrap(RANGE_DATA_BUFFER_LIST *p_node);
#endif
void fm_init_setting(void);
_u32 fm_open_file_timeout(void);
_u32 fm_rw_file_timeout(void);
_u32 fm_close_file_timeout(void);
_u32 fm_max_merge_range_num(void);  

//#endif  /* XUNLEI_MODE */

#ifdef __cplusplus
}
#endif

#endif // !defined(__FILE_MANAGER_XL_20080714)

