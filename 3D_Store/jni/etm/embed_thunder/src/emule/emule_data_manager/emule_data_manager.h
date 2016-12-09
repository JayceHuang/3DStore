#ifndef _EMULE_DATA_MANAGER_H_
#define _EMULE_DATA_MANAGER_H_

#include "utility/define.h"
#ifdef ENABLE_EMULE
#include "emule/emule_utility/emule_define.h"
#include "emule/emule_data_manager/emule_part_checker.h"
#include "data_manager/correct_manager.h"
#include "data_manager/range_manager.h"
#include "data_manager/file_info.h"
#include "utility/bitmap.h"
#include "utility/range.h"

struct tagEMULE_TASK;
struct tagRESOURCE;
struct _tag_range_list;
struct _tag_error_block_list;
struct _tag_ds_data_intereface;
struct tagED2K_LINK_INFO;

#define CROSS_DATA_VALIED_MAGIC  (0x12345678)
#define CROSS_DATA_MAGIC_LENGTH (4)

typedef struct tagEMULE_DATA_MANAGER
{
	FILEINFO 				_file_info;
	struct tagEMULE_TASK*	_emule_task;
	BOOL					_is_cid_and_gcid_valid;
	_u8						_cid[CID_SIZE];
	_u8						_gcid[CID_SIZE];
	_u8						_file_id[FILE_ID_SIZE];
	_u64					_file_size;
	BITMAP					_part_bitmap;
	RANGE_LIST				_priority_range_list;
	RANGE_LIST				_recv_range_list;
	RANGE_LIST				_uncomplete_range_list;
	BOOL					_file_success_flag;			//文件正确下载成功标志，若为TRUE，则关闭文件时要改名并删临时文件
	EMULE_PART_CHECKER		_part_checker;				//emule校验，每块大小为9.28M
	RANGE_MANAGER 			_range_manager;
	CORRECT_MANAGER			_correct_manager;
    BOOL                    _is_task_deleting;
    BOOL                    _is_file_exist;
    _u64                    _next_start_download_pos;
    _u64                    _server_dl_bytes;
	_u64                    _peer_dl_bytes;
	_u64                    _emule_dl_bytes;
	_u64                    _cdn_dl_bytes;
	_u64					_normal_cdn_dl_bytes;
	_u64 					_lixian_dl_bytes;
	_u64 					_origin_dl_bytes;
	_u64 					_assist_peer_dl_bytes;
	RANGE_LIST              _3part_cid_list;
	char _cross_file_path[MAX_FILE_NAME_BUFFER_LEN * 2];
}EMULE_DATA_MANAGER;

_int32 emule_create_data_manager(EMULE_DATA_MANAGER** data_manager, struct tagED2K_LINK_INFO* ed2k_info, char* file_path, struct tagEMULE_TASK* emule_task);

_int32 emule_date_manager_adjust_uncomplete_range(EMULE_DATA_MANAGER* data_manager);

_int32 emule_close_data_manager(EMULE_DATA_MANAGER* data_manager);

_int32 emule_destroy_data_manager(EMULE_DATA_MANAGER* data_manager);

_int32 emule_set_part_hash(EMULE_DATA_MANAGER* data_manager, const _u8* part_hash, _u32 part_hash_len);

_int32 emule_set_aich_hash(EMULE_DATA_MANAGER* data_manager, const _u8* aich_hash, _u32 aich_hash_len);

BOOL emule_is_part_hash_valid(EMULE_DATA_MANAGER* data_manager);

BOOL emule_is_aich_hash_valid(EMULE_DATA_MANAGER* data_manager);

const RANGE_LIST* emule_get_recved_range_list(EMULE_DATA_MANAGER* data_manager);

//所有的数据读取都通过该接口
_int32 emule_read_data(EMULE_DATA_MANAGER* data_manager, RANGE_DATA_BUFFER* data_buffer, notify_read_result callback, void* user_data);

_int32 emule_write_data(EMULE_DATA_MANAGER* data_manager, const RANGE* range, char** data_buffer, _u32 data_len,_u32 buffer_len, RESOURCE* res);

void  emule_merge_cross_range(EMULE_DATA_MANAGER* data_manager, const RANGE* range, char** data_buffer, _u32 data_len,_u32 buffer_len);


_u32 emule_get_part_count(EMULE_DATA_MANAGER* data_manager);

_u32 emule_get_part_size(EMULE_DATA_MANAGER* data_manager, _u32 part_index);

_u64 emule_get_range_size(EMULE_DATA_MANAGER* data_manager, const RANGE* range);

_int32 emule_notify_check_part_hash_result(EMULE_DATA_MANAGER* data_manager, _u32 part_index, _int32 result);


//以下是dispatcher模块的调用接口
_int32 emule_set_dispatch_interface(EMULE_DATA_MANAGER* data_manager, struct _tag_ds_data_intereface* platform);

_u64 emule_get_file_size(void* data_manager);

BOOL emule_get_gcid(void* data_manager, _u8 gcid[CID_SIZE]);

BOOL emule_is_only_using_origin_res(void* data_manager);

BOOL emule_is_origin_resource(void* data_manager, struct tagRESOURCE* res);

_int32 emule_get_priority_range(void* data_manager, struct _tag_range_list* priority_range_list);

_int32 emule_get_uncomplete_range(void* data_manager, struct _tag_range_list* uncomplete_range_list);

_int32 emule_get_error_range_block_list(void* data_manager, struct _tag_error_block_list** error_block_list);		

BOOL emule_is_vod_mod(void* data_manager);	

//以下是file_info的回调函数
void emule_notify_file_create_result(void* user_data, FILEINFO* file_info, _int32 creat_result);

void emule_notify_file_block_check_result(void* user_data, FILEINFO* file_info, RANGE* range, BOOL result);

void emule_notify_file_result(void* user_data, FILEINFO* file_info, _u32 result);

void emule_notify_file_close_result(void* user_data, FILEINFO* file_info, _int32 result);


///////////////// for report
void emule_get_dl_bytes(void* data_manager, _u64 *p_server_dl_bytes, _u64 *p_peer_dl_bytes, _u64 *p_emule_dl_bytes, _u64 *p_cdn_dl_bytes, _u64 *p_lixian_dl_bytes );
void emule_get_file_id(void* data_manager, _u8 file_id[FILE_ID_SIZE]);
void emule_get_part_hash(void* data_manager, _u8** pp_part_hash, _u32 *p_hash_len);
void emule_get_aich_hash(void* data_manager, _u8** pp_aich_hash, _u32 *p_aich_hash_len);

BOOL emule_get_shub_cid( void* data_manager, _u8 cid[CID_SIZE]);
BOOL emule_get_cid( void* data_manager, _u8 cid[CID_SIZE]);
BOOL emule_get_calc_gcid( void* data_manager, _u8 gcid[CID_SIZE]);
_int32 emule_get_block_size( void* data_manager);
BOOL emule_get_bcids( void* data_manager, _u32* bcid_num, _u8** bcid_buffer);
void emule_get_normal_cdn_down_bytes(void *data_manager,_u64 * p_normal_cdn_dl_bytes);
void emule_get_origin_resource_dl_bytes( void* data_manager, _u64 *p_origin_resource_dl_bytes);
void emule_get_assist_peer_dl_bytes( void* data_manager, _u64 *p_assist_peer_dl_bytes);
BOOL emule_get_shub_gcid(void* data_manager, _u8 gcid[CID_SIZE]);
#ifdef EMBED_REPORT
struct tagFILE_INFO_REPORT_PARA *emule_get_report_para( void* data_manager );
#endif
#endif /* ENABLE_EMULE */

#endif


