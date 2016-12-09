

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


#if !defined(__FILE_MANAGER_20080714)
#define __FILE_MANAGER_20080714

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
//#ifdef XUNLEI_MODE
#include "data_manager/file_manager_xl.h"
//#else
#include "utility/range.h"
#include "connect_manager/resource.h"
#include "utility/define.h"
#include "data_manager/data_receive.h"
#include "utility/mempool.h"
#include "data_manager/file_manager_interface.h"

/* Same Macro */

/**************************
 * Err Try Time
 **************************/
#define FM_ERR_TRY_TIME         (3)

/**************************
 * IO Operation Type
 **************************/
#define FM_OP_BLOCK_WRITE       (0)
#define FM_OP_BLOCK_TMP_READ	(1)
#define FM_OP_BLOCK_NORMAL_READ	(2)
#define FM_OP_BLOCK_SYN_READ	(4)

struct tagTMP_FILE;

typedef enum t_file_write_mode
{
	FWM_RANGE= 0, 				/*  用RANGE方式写 */
	FWM_BLOCK, 				/*  用BLOCK方式乱序写(自增长) */
	FWM_ORDER_BLOCK,			/*  用BLOCK方式顺序写 */
	FWM_BLOCK_NEED_ORDER	/*  用BLOCK方式乱序写,最后再调成顺序(自增长) */
}FILE_WRITE_MODE;

/********************************************************
 * Used in post create file message
 ********************************************************/
typedef struct tagMSG_FILE_CREATE_PARA
{
	struct tagTMP_FILE *_file_struct_ptr;
	void *_user_ptr;
	void *_callback_func_ptr;
}MSG_FILE_CREATE_PARA;


/********************************************************
 * Store close file parameter
 ********************************************************/
typedef struct tagMSG_FILE_CLOSE_PARA
{	
	struct tagTMP_FILE *_file_struct_ptr;
	void *_user_ptr;
	notify_file_close _callback_func_ptr;
}MSG_FILE_CLOSE_PARA;


/*********************************************************************
 * Used in write/read block data and save as a list in file manager
 *********************************************************************/
typedef struct tagBLOCK_DATA_BUFFER
{
    _u32 _block_index;
	_u32 _block_offset;
	_u32 _data_length;  
	void *_buffer_para_ptr;	
	void *_call_back_ptr;
	void *_user_ptr;	
	void *_range_buffer;
	BOOL _is_range_buffer_end;
	_u32 _try_num;	
	
   	char *_data_ptr; //block data
	_u16 _operation_type;
	BOOL _is_call_back;
	BOOL _is_tmp_block;

	BOOL _is_cancel;
}BLOCK_DATA_BUFFER;



/********************************************************
* Used in post write/read data message
********************************************************/
typedef struct tagMSG_FILE_RW_PARA
{
	struct tagTMP_FILE *_file_struct_ptr;
	void *_user_ptr;
	void *_callback_func_ptr;
	BLOCK_DATA_BUFFER *_rw_para_ptr;	
	_u16 _op_type;
	RW_DATA_BUFFER *_rw_data_buffer_ptr;
}MSG_FILE_RW_PARA;


/*********************************************************************
 * Save the relationship between the logic block and the disk block
 *********************************************************************/
typedef struct tagDATA_BLOCK_INDEX_ITEM
{
	_u32 _logic_block_index;// the block index in the logic
	BOOL _is_valid;
}DATA_BLOCK_INDEX_ITEM;


/*********************************************************************
 * Save in the cfg file, update it return the write_data result
 *********************************************************************/
typedef struct tagDATA_BLOCK_INDEX_ARRAY
{
	_u32 _total_index_num;
	_u32 _valid_index_num;
	_u32 _extra_block_index;  /* 调整时加在文件末尾用作交换空间的一块额外BLOCK 所存放的数据块的在文件中的正确序号 */
	_u32 _empty_block_index; /* 调整时数据已被移走空出来的BLOCK序号 */
	DATA_BLOCK_INDEX_ITEM *_index_array_ptr;
}DATA_BLOCK_INDEX_ARRAY;



/*********************************************************************
 * The core struct, contain the file basic information
 *********************************************************************/
typedef struct tagTMP_FILE
{
    char _file_name[MAX_FILE_NAME_LEN];
    char _file_path[MAX_FILE_PATH_LEN];
    _u32 _file_name_len;
    _u32 _file_path_len;
    _u64 _file_size;
    _u64 _new_file_size;
    _u32 _device_id;
    _u32 _block_size;
    _u32 _block_num;
    _u32 _last_block_size;
    LIST _asyn_read_block_list;  //contain the asyn read operation ranges
    LIST _syn_read_block_list;  //contain the syn read operation ranges
    LIST _write_block_list; //contain the R/W operation blocks
    
	LIST _asyn_read_range_list;  //contain the asyn read operation ranges
	LIST _write_range_list;      //contain the write operation ranges
	
	MSG_FILE_CLOSE_PARA *_close_para_ptr;
	
	DATA_BLOCK_INDEX_ARRAY _block_index_array;
	
	BOOL _is_file_size_valid;
	BOOL _is_file_created;
	BOOL _is_closing;
	BOOL _is_known_block_size;
	BOOL _is_order_mode;
	BOOL _is_writing;
	BOOL _is_asyn_reading;
////////////////////////////////////
	_u32 _open_msg_id;	
	BOOL _is_reopening;	
	BOOL _is_opening;	
	BOOL _is_sending_read_op;
	BOOL _is_sending_write_op;
	_u32 _on_writing_op_count;
	BOOL _is_sending_close_op;
	
	FILE_WRITE_MODE _write_mode;
	
	_u32 _ordering_index; 		/* 当前正在调整的数据块在 _block_index_array->_index_array_ptr中的序号(即在文件中当前存放的序号)*/
	_u32 _ordering_logic_index;   /* 当前正在调整的数据块在文件中的正确序号*/

    _u32 _write_error;
}TMP_FILE;

//tmp_file malloc/free wrap
_int32 init_tmp_file_slabs(void);
_int32 uninit_tmp_file_slabs(void);
_int32 tmp_file_malloc_wrap(TMP_FILE **pp_node);
_int32 tmp_file_free_wrap(TMP_FILE *p_node);

//msg_file_create_para malloc/free wrap
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

//block_data_buffer malloc/free wrap
_int32 init_block_data_buffer_slabs(void);
_int32 uninit_block_data_buffer_slabs(void);
_int32 block_data_buffer_malloc_wrap(BLOCK_DATA_BUFFER **pp_node);
_int32 block_data_buffer_free_wrap(BLOCK_DATA_BUFFER *p_node);

//range_data_buffer_list malloc/free wrap
_int32 init_range_data_buffer_list_slabs(void);
_int32 uninit_range_data_buffer_list_slabs(void);

_int32 range_data_buffer_list_malloc_wrap(RANGE_DATA_BUFFER_LIST **pp_node);
_int32 range_data_buffer_list_free_wrap(RANGE_DATA_BUFFER_LIST *p_node);
//#endif /* XUNLEI_MODE */

#ifdef __cplusplus
}
#endif

#endif // !defined(__FILE_MANAGER_20080714)

