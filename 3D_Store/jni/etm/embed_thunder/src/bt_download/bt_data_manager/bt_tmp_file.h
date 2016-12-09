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

//�ֱ�����ļ������ڿ���״̬�����ڴ�״̬���ɶ�д״̬�����ڹر�״̬
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
	RANGE_LIST	_tmp_padding_range_list;		//��ʱ�ļ������padding_range	
	RANGE_LIST	_valid_padding_range_list;		//��Ч������padding_range,���Ѿ�д�ڴ����е�
	_u32		_total_range_num;				//��ʱ�ļ�Ŀǰһ���ж��ٸ�range
	SET			_range_change_set;				//padding_range��file_range��ת��
	struct tagTMP_FILE*	_base_file;				//�ļ�
	LIST		_read_request_list;				//������
	LIST		_write_request_list;
	BT_TMP_FILE_READ_REQ*	_cur_read_req;
	BT_TMP_FILE_WRITE_REQ*	_cur_write_req;
	BT_TMP_FILE_STATE	_state;
	RANGE_DATA_BUFFER _read_data_buffer;		//����ṹ��������ʱ�ļ�
	RANGE_DATA_BUFFER_LIST _write_data_buffer_list;	//����ṹ����д��ʱ�ļ�
	char	_file_path[MAX_FILE_PATH_LEN];
	char	_file_name[MAX_FILE_NAME_LEN];
	BOOL	_del_flag;							//�Ƿ�ɾ����ʱ�ļ���ʶ
}BT_TMP_FILE;

_int32 bt_init_tmp_file(BT_TMP_FILE* tmp_file, char* tmp_file_path, char* tmp_file_name);

_int32 bt_uninit_tmp_file(BT_TMP_FILE* tmp_file);

_int32 bt_tmp_file_set_del_flag(BT_TMP_FILE* tmp_file, BOOL flag);

_int32 bt_add_tmp_file(BT_TMP_FILE* tmp_file, _u32 file_index, struct tagBT_RANGE_SWITCH* range_switch);

BOOL bt_is_tmp_file_range(BT_TMP_FILE* tmp_file, const RANGE* padding_range);

//��ʱ�ļ���ĳ��padding_range�Ƿ��Ѿ���Ч����д�ڴ�����
BOOL bt_is_tmp_range_valid(BT_TMP_FILE* tmp_file, const RANGE* padding_range);

_int32 bt_read_tmp_file(BT_TMP_FILE* tmp_file, const RANGE* padding_range, struct _tag_range_data_buffer* data_buffer, notify_read_result callback, void* user_data);

_int32 bt_write_tmp_file(BT_TMP_FILE* tmp_file, const RANGE* padding_range, char* buffer, _u32 buffer_len, _u32 data_len, bt_tmp_file_write_callback callback, void* user_data);

_int32 bt_get_tmp_file_need_donwload_range_list(BT_TMP_FILE* tmp_file, RANGE_LIST* tmp_file_need_download_range_list);

_int32 bt_tmp_file_get_valid_range_list(BT_TMP_FILE* tmp_file, RANGE_LIST* valid_range_list);

//================================�ڲ�����=======================================

_int32 bt_tmp_file_save_to_cfg_file(BT_TMP_FILE* tmp_file);		//���л�

_int32 bt_tmp_file_load_from_cfg_file(BT_TMP_FILE* tmp_file);	//�����л�

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

