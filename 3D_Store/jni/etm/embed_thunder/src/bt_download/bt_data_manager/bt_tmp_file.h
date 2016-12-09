/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2009/03/28
-----------------------------------------------------------------------------------------------------------*/
#ifndef _BT_TMP_FILE_H_
#define	_BT_TMP_FILE_H_

#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"
#ifdef ENABLE_BT 

#include "data_manager/file_manager_interface.h"
#include "utility/map.h"

struct tagTMP_FILE;
struct tagBT_RANGE_SWITCH;
struct _tag_range_list;
struct _tag_range_data_buffer;

typedef _int32 (*bt_tmp_file_req_callback)(_int32 errcode, void* user_data, RANGE* pending_range, char* buffer, _u32 len);
typedef _int32 (*bt_tmp_file_write_callback)(_int32 errcode, void* user_data, RANGE* padding_range);

//分别代表文件正处于空闲状态，正在打开状态，可读写状态，正在关闭状态
typedef enum {IDLE_STATE = 0, OPENING_STATE, READ_WRITE_STATE, CLOSING_STATE, UNINIT_STATE} BT_TMP_FILE_STATE;

typedef struct tagBT_TMP_FILE_READ_REQ
{
	RANGE	_padding_range;
	struct _tag_range_data_buffer* _data_buffer;
	notify_read_result _callback;
	void*	_user_data;
}BT_TMP_FILE_READ_REQ;

typedef struct tagBT_TMP_FILE_WRITE_REQ
{
	RANGE	_pending_range;
	char*	_buffer;
	_u32	_buffer_len;
	bt_tmp_file_write_callback _callback;
	void*	_user_data;
}BT_TMP_FILE_WRITE_REQ;

typedef struct tagRANGE_CHANGE_NODE
{
	RANGE	_padding_range;
	_u32	_file_range_index;
}RANGE_CHANGE_NODE;

typedef struct tagBT_TMP_FILE
{
	RANGE_LIST	_tmp_padding_range_list;		//临时文件保存的padding_range	
	RANGE_LIST	_valid_padding_range_list;		//有效的数据padding_range,即已经写在磁盘中的
	_u32		_total_range_num;				//临时文件目前一共有多少个range
	SET			_range_change_set;				//padding_range与file_range的转换
	struct tagTMP_FILE*	_base_file;				//文件
	LIST		_read_request_list;				//读请求
	LIST		_write_request_list;
	BT_TMP_FILE_READ_REQ*	_cur_read_req;
	BT_TMP_FILE_WRITE_REQ*	_cur_write_req;
	BT_TMP_FILE_STATE	_state;
	RANGE_DATA_BUFFER _read_data_buffer;		//这个结构用来读临时文件
	RANGE_DATA_BUFFER_LIST _write_data_buffer_list;	//这个结构用来写临时文件
	char	_file_path[MAX_FILE_PATH_LEN];
	char	_file_name[MAX_FILE_NAME_LEN];
	BOOL	_del_flag;							//是否删除临时文件标识
}BT_TMP_FILE;

_int32 bt_init_tmp_file(BT_TMP_FILE* tmp_file, char* tmp_file_path, char* tmp_file_name);

_int32 bt_uninit_tmp_file(BT_TMP_FILE* tmp_file);

_int32 bt_tmp_file_set_del_flag(BT_TMP_FILE* tmp_file, BOOL flag);

_int32 bt_add_tmp_file(BT_TMP_FILE* tmp_file, _u32 file_index, struct tagBT_RANGE_SWITCH* range_switch);

BOOL bt_is_tmp_file_range(BT_TMP_FILE* tmp_file, const RANGE* padding_range);

//临时文件中某个padding_range是否已经有效，即写在磁盘中
BOOL bt_is_tmp_range_valid(BT_TMP_FILE* tmp_file, const RANGE* padding_range);

_int32 bt_read_tmp_file(BT_TMP_FILE* tmp_file, const RANGE* padding_range, struct _tag_range_data_buffer* data_buffer, notify_read_result callback, void* user_data);

_int32 bt_write_tmp_file(BT_TMP_FILE* tmp_file, const RANGE* padding_range, char* buffer, _u32 buffer_len, _u32 data_len, bt_tmp_file_write_callback callback, void* user_data);

_int32 bt_get_tmp_file_need_donwload_range_list(BT_TMP_FILE* tmp_file, RANGE_LIST* tmp_file_need_download_range_list);

_int32 bt_tmp_file_get_valid_range_list(BT_TMP_FILE* tmp_file, RANGE_LIST* valid_range_list);

//================================内部函数=======================================

_int32 bt_tmp_file_save_to_cfg_file(BT_TMP_FILE* tmp_file);		//序列化

_int32 bt_tmp_file_load_from_cfg_file(BT_TMP_FILE* tmp_file);	//反序列化

_int32 bt_update_tmp_file(BT_TMP_FILE* tmp_file);

_int32 bt_send_read_request(BT_TMP_FILE* tmp_file);

_int32 bt_send_write_request(BT_TMP_FILE* tmp_file);

_int32 notify_bt_tmp_file_read(struct tagTMP_FILE *p_file_struct, void* user_data, RANGE_DATA_BUFFER *data_buffer, _int32 read_result, _u32 read_len);

_int32 notify_bt_tmp_file_write(struct tagTMP_FILE *p_file_struct, void* user_data, RANGE_DATA_BUFFER_LIST* data_buffer_list, _int32 write_result);

_int32 notify_bt_tmp_file_create(struct tagTMP_FILE *p_file_struct, void* user_data, _int32 create_result);

_int32 notify_bt_tmp_file_close(struct tagTMP_FILE *p_file_struct, void* user_data, _int32 close_result);
#endif

#ifdef __cplusplus
}
#endif
#endif

