/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2009/04/08
-----------------------------------------------------------------------------------------------------------*/
#ifndef _BT_PIECE_CHECKER_H_
#define _BT_PIECE_CHECKER_H_

#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"
#ifdef ENABLE_BT 

#include "bt_download/bt_data_manager/bt_tmp_file.h"
#include "asyn_frame/msg.h"
#include "utility/bitmap.h"
#include "utility/sha1.h"

struct tagBT_DATA_MANAGER;
struct tagBT_PIECE_CHECKER;
struct _tag_range_data_buffer;

typedef struct tagBT_PIECE_HASH		
{
	_u32	_piece_index;
	RANGE_DATA_BUFFER _data_buffer;
	ctx_sha1 _sha1;
	BOOL	_in_tmp_file;			//这块piece是否在临时文件里面
	BOOL	_is_waiting_read;		//range正在写，等待一下再去读文件
	LIST	_read_range_info_list;
	struct tagBT_PIECE_CHECKER* _piece_checker;
}BT_PIECE_HASH;

typedef struct tagBT_PIECE_CHECKER
{
	struct tagBT_DATA_MANAGER*	_bt_data_manager;
	_u8*	_piece_hash;
	RANGE_LIST	_need_download_range_list;		//需要下载的range_list;
	RANGE_LIST	_need_check_range_list;			//需要进行piece校验的padding range
	BT_TMP_FILE _tmp_file;
	_u32		_check_timeout_id;				//定时器，定时促发校验逻辑
	LIST		_waiting_check_piece_list;		//等待进行校验的piece
	BT_PIECE_HASH* _cur_hash_piece;				//正在进行hash校验的piece	
}BT_PIECE_CHECKER;

_int32 bt_init_piece_checker(BT_PIECE_CHECKER* piece_checker, struct tagBT_DATA_MANAGER* data_manager, _u8* piece_hash, char* path, char* name);

_int32 bt_uninit_piece_checker(BT_PIECE_CHECKER* piece_checker);

_int32 bt_checker_get_need_download_range(BT_PIECE_CHECKER* piece_checker, RANGE_LIST* download_range_list);

_int32 bt_checker_put_data(BT_PIECE_CHECKER* piece_ckecker, const RANGE* padding_range, char* buffer, _u32 data_len, _u32 buffer_len);

_int32 bt_checker_recv_range(BT_PIECE_CHECKER* piece_checker, const RANGE* padding_range);

_int32 bt_checker_erase_need_check_range(BT_PIECE_CHECKER* piece_checker, RANGE* padding_range);

_int32 bt_checker_add_need_check_file(BT_PIECE_CHECKER* piece_checker, _u32 file_index);

_int32 bt_checker_del_tmp_file(BT_PIECE_CHECKER* piece_checker);

_int32 bt_checker_get_tmp_file_valid_range(BT_PIECE_CHECKER* piece_checker, RANGE_LIST* valid_padding_range_list);

_int32 bt_checker_read_tmp_file(BT_PIECE_CHECKER* piece_checker, RANGE_DATA_BUFFER* data_buffer, notify_read_result callback, void* user_data);


//--------------------------------------------------以下函数为内部调用--------------------------------------------------------//

//提交一个piece到等待校验队列里
_int32 bt_checker_commit_piece(BT_PIECE_CHECKER* piece_ckecker, _u32 piece_index);

_int32 bt_checker_handle_timeout(const MSG_INFO* msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid);

//当前正在校验的piece读取失败
_int32 bt_checker_handle_read_failed(BT_PIECE_CHECKER* piece_checker);	

_int32 bt_checker_read_data(BT_PIECE_CHECKER* piece_checker);

_int32 bt_checker_read_data_callback(struct tagTMP_FILE* p_file_struct, void *p_user, struct _tag_range_data_buffer* data_buffer, _int32 read_result, _u32 read_len);

_int32 bt_checker_verify_piece(BT_PIECE_CHECKER* piece_checker);

_int32 bt_checker_calc_hash(BT_PIECE_HASH* piece_hash, char* data, _u32 len);

//开始启动一个piece校验流程
_int32 bt_checker_start_piece_hash(BT_PIECE_CHECKER* piece_checker);

_int32 bt_checker_write_tmp_file_callback(_int32 errcode, void* user_data, RANGE* padding_range);
#endif

#ifdef __cplusplus
}
#endif
#endif

