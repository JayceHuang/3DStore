
#if !defined(__DATA_MANAGER_INTERFACE_20080604)
#define __DATA_MANAGER_INTERFACE_20080604

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#include "utility/range.h"
#include "connect_manager/resource.h"
#include "connect_manager/data_pipe.h"
#include "data_manager/correct_manager.h"
#include "data_manager/file_info.h"

#include "data_manager/range_manager.h"
#include "data_manager/data_receive.h"
#include "connect_manager/pipe_interface.h"
//#include "task_manager/task_manager_interface.h"
#include "dispatcher/dispatcher_interface_info.h"
#include "embed_thunder.h"


/*#define   DATA_UNINITED    99
#define   DATA_SUCCESS    100
#define   DATA_DOWNING    101
#define   DATA_CANNOT_CORRECT_ERROR    102
#define   DATA_CAN_NOT_GET_CID    103
#define   DATA_CAN_NOT_GET_GCID    104
#define   DATA_CHECK_CID_ERROR    105
#define   DATA_CHECK_GCID_ERROR    106
#define   DATA_CREAT_FILE_ERROR    107
#define   DATA_WRITE_FILE_ERROR    108
#define   DATA_READ_FILE_ERROR      109
#define   DATA_ALLOC_BCID_BUFFER_ERROR      110
#define   DATA_ALLOC_READ_BUFFER_ERROR      111

#define   DATA_NO_SPACE_TO_CREAT_FILE      112
#define   DATA_CANOT_CHECK_CID_READ_ERROR    113
#define   DATA_CANOT_CHECK_CID_NOBUFFER    114
*/
struct _tag_task;

typedef struct   _tag_data_manager 
{
    FILEINFO _file_info;  
	RANGE_MANAGER _range_record;
    CORRECT_MANAGER _correct_manage;

    RANGE_LIST  part_3_cid_range;	   
	RESOURCE* _origin_res;

    _int32  _data_status_code;	

	BOOL _only_use_orgin;	

	BOOL _has_bub_info;     // shub是否已经查询完成，不管是否有索引

	BOOL _waited_success_close_file;
	BOOL _need_call_back;

    char _file_suffix[MAX_SUFFIX_LEN];
	
	BOOL _IS_VOD_MOD;
	_u32  _start_pos_index;
	_u32  _conrinue_times;

#ifdef _VOD_NO_DISK   
	BOOL _is_no_disk;
#endif

    BOOL _is_check_data;

	/*
		recv_bytes为 接收字节数
		download_bytes为 有效接收字节数
			（经过 之前没接收过的，整块的，可以写入文件等条件过来后，剩下的接收字节数）
        注：维持老逻辑，离线cdn和其他cdn分开统计
	*/
	_u64 _server_dl_bytes;
	_u64 _peer_dl_bytes;
	_u64 _cdn_dl_bytes;
	_u64 _lixian_dl_bytes;
	_u64 _origin_dl_bytes;
	_u64 _assist_peer_dl_bytes;
	_u64 _appacc_dl_bytes;
	_u64 _normal_cdn_dl_bytes;

    _u64 _server_recv_bytes;
	_u64 _peer_recv_bytes;
	_u64 _cdn_recv_bytes;
    _u64 _lixian_recv_bytes;
	
	struct _tag_task* _task_ptr;
	
	_u32 _cur_range_step;

	BOOL  _is_origion_res_changed;
}DATA_MANAGER ;

/* platform of data manager module, must  be invoke in the initial state of the download program*/
_int32  init_data_manager_module(void);
_int32  uninit_data_manager_module(void);

/* platform of data manager module*/
_int32 get_data_manager_cfg_parameter(void);
_int32  dm_create_slabs_and_init_data_buffer(void);
_int32  dm_destroy_slabs_and_unit_data_buffer(void);


/* platform of data manager struct*/
_int32  dm_init_data_manager(DATA_MANAGER* p_data_manager_interface, struct _tag_task* p_task);
_int32  dm_unit_data_manager(DATA_MANAGER* p_data_manager_interface);


_int32  data_manager_notify_failure(DATA_MANAGER* p_data_manager_interface,  _int32 error_code);

_int32  data_manager_notify_success(DATA_MANAGER* p_data_manager_interface);

void  data_manager_notify_file_create_info(DATA_MANAGER* p_data_manager_interface, BOOL creat_success);

void  data_manager_notify_file_close_info(DATA_MANAGER* p_data_manager_interface, _int32 close_result);

_int32 dm_set_file_info( DATA_MANAGER* p_data_manager_interface,char* filename, char*filedir, char* originurl, char* originrefurl);

_int32 dm_set_origin_url_info( DATA_MANAGER* p_data_manager_interface, char* originurl, char* originrefurl);

_int32 dm_set_cookie_info( DATA_MANAGER* p_data_manager_interface, char* cookie);

BOOL dm_get_origin_url( DATA_MANAGER* p_data_manager_interface,char** origin_url);

BOOL dm_get_origin_ref_url( DATA_MANAGER* p_data_manager_interface,char** origin_ref_url);

BOOL dm_get_file_path( DATA_MANAGER* p_data_manager_interface,char** file_path);

BOOL dm_get_file_name( DATA_MANAGER* p_data_manager_interface,char** filename);

_int32 dm_set_file_suffix(DATA_MANAGER* p_data_manager_interface, char *file_suffix);

char* dm_get_file_suffix(DATA_MANAGER* p_data_manager_interface);

 PIPE_FILE_TYPE dm_get_file_type( DATA_MANAGER* p_data_manager_interface ); 

BOOL dm_get_filnal_file_name( DATA_MANAGER* p_data_manager_interface,char** filename);

_int32 dm_set_decision_filename_policy(ET_FILENAME_POLICY namepolicy);

_int32 dm_decide_filename( DATA_MANAGER* p_data_manager_interface);

_int32 dm_set_file_size( DATA_MANAGER* p_data_manager_interface,_u64 filesize, BOOL force);

_u64  dm_get_file_size( DATA_MANAGER* p_data_manager_interface);

_int32 dm_get_block_size( DATA_MANAGER* p_data_manager_interface);

BOOL  dm_has_recv_data( DATA_MANAGER* p_data_manager_interface);

RANGE  dm_pos_len_to_valid_range( DATA_MANAGER* p_data_manager_interface, _u64 pos, _u64 length);

_u32  dm_get_file_percent( DATA_MANAGER* p_data_manager_interface);

_u64  dm_get_download_data_size(DATA_MANAGER* p_data_manager_interface);

_u64  dm_get_writed_data_size( DATA_MANAGER* p_data_manager_interface);

_int32 dm_set_status_code( DATA_MANAGER* p_data_manager_interface,_int32 status);	

_int32 dm_get_status_code( DATA_MANAGER* p_data_manager_interface);

_int32 dm_handle_extra_things( DATA_MANAGER* p_data_manager_interface);

_int32 dm_set_cid( DATA_MANAGER* p_data_manager_interface, _u8 cid[CID_SIZE]);

_int32 dm_set_gcid( DATA_MANAGER* p_data_manager_interface, _u8 gcid[CID_SIZE]);

_int32 dm_set_user_data(DATA_MANAGER* dm_interface, const _u8 *user_data, _u32 len);

//当数据文件发生文件大小变化与hub返回的文件大小不一致时，hub返回的bcid等应该失效。
_int32 dm_set_hub_return_info( DATA_MANAGER* p_data_manager_interface,_int32 gcid_type, 
                        unsigned char block_cid[], 	_u32 block_count, _u32 block_size, _u64 filesize  );

_int32 dm_shub_no_result( DATA_MANAGER* p_data_manager_interface);

BOOL dm_get_cid( DATA_MANAGER* p_data_manager_interface, _u8 cid[CID_SIZE]);

BOOL dm_get_shub_gcid( DATA_MANAGER* p_data_manager_interface, _u8 gcid[CID_SIZE]);

BOOL dm_get_calc_gcid( DATA_MANAGER* p_data_manager_interface, _u8 gcid[CID_SIZE]);

BOOL dm_bcid_is_valid( DATA_MANAGER* p_data_manager_interface);

/*获得任务的bcid 块数和buffer，注意只能在delete之前调用，并且不能修改bcidbuffer的内容，
    buffer长度为 bcid_num*CID_SIZE, 没有\0结束符，因为不是字符串!*/
BOOL dm_get_bcids( DATA_MANAGER* p_data_manager_interface, _u32* bcid_num, _u8** bcid_buffer);

BOOL dm_only_use_origin( DATA_MANAGER* p_data_manager_interface);

BOOL dm_check_gcid(DATA_MANAGER* p_data_manager_interface);

BOOL dm_check_cid(DATA_MANAGER* p_data_manager_interface, _int32* err_code);

BOOL dm_3_part_cid_is_ok( DATA_MANAGER* p_data_manager_interface);

_int32 dm_set_origin_resource( DATA_MANAGER* p_data_manager_interface, RESOURCE*  origin_res);

BOOL dm_is_origin_resource( DATA_MANAGER* p_data_manager_interface, RESOURCE*  res);

BOOL dm_origin_resource_is_useable( DATA_MANAGER* p_data_manager_interface);

_int32 dm_put_data(DATA_MANAGER* p_data_manager_interface , const RANGE *r, char **data_ptr, _u32 data_len,_u32 buffer_len,
                   DATA_PIPE *p_data_pipe, RESOURCE *resource_ptr);

_int32 dm_notify_dispatch_data_finish(DATA_MANAGER* p_data_manager_interface ,DATA_PIPE *ptr_data_pipe);

_int32 dm_flush_cached_buffer(DATA_MANAGER* p_data_manager_interface);

_int32 dm_check_block_success(DATA_MANAGER* p_data_manager_interface, RANGE* p_block_range);

_int32 dm_check_block_failure(DATA_MANAGER* p_data_manager_interface, RANGE* p_block_range);

BOOL dm_ia_all_received(DATA_MANAGER* p_data_manager_interface);

_int32 dm_erase_data_except_origin(DATA_MANAGER* p_data_manager_interface);

_int32 dm_get_data_buffer(DATA_MANAGER* p_data_manager_interface ,  char **data_ptr, _u32* data_len);

_int32 dm_free_data_buffer(DATA_MANAGER* p_data_manager_interface ,  char **data_ptr, _u32 data_len);

BOOL dm_range_is_recv(DATA_MANAGER* p_data_manager_interface, RANGE* r);

BOOL dm_range_is_write(DATA_MANAGER* p_data_manager_interface, RANGE* r);

BOOL dm_range_is_check(DATA_MANAGER* p_data_manager_interface, RANGE* r);

BOOL dm_range_is_cached(DATA_MANAGER* p_data_manager_interface, RANGE* r);

/*   以下函数得到的range_list 只能读不能写!!!!*/
 RANGE_LIST* dm_get_recved_range_list(DATA_MANAGER* p_data_manager_interface);
 
 RANGE_LIST* dm_get_writed_range_list(DATA_MANAGER* p_data_manager_interface);
 
 RANGE_LIST* dm_get_checked_range_list(DATA_MANAGER* p_data_manager_interface);

BOOL dm_is_only_using_origin_res(DATA_MANAGER* p_data_manager_interface);

_int32 dm_get_uncomplete_range(DATA_MANAGER* p_data_manager_interface, RANGE_LIST*  un_complete_range_list);

_int32 dm_get_priority_range(DATA_MANAGER* p_data_manager_interface, RANGE_LIST*  priority_range_list);

_int32 dm_get_error_range_block_list(DATA_MANAGER* p_data_manager_interface, ERROR_BLOCKLIST**  pp_error_block_list);

/*删除下载任务中的临时文件，包括数据文件和cfg文件，一定要在unit data manager 之后才能调用!*/
_int32 dm_delete_tmp_file(DATA_MANAGER* p_data_manager_interface);
 
_int32 dm_open_old_file(DATA_MANAGER* p_data_manager_interface, char* cfg_file_path);

_u64 dm_get_downsize_from_cfg_file(char* cfg_file_path);

void dm_set_vod_mode(DATA_MANAGER* p_data_manager_interface,BOOL  is_vod_mod);

BOOL dm_is_vod_mode(DATA_MANAGER* p_data_manager_interface);

_int32 dm_set_emerge_rangelist( DATA_MANAGER* p_data_manager_interface, RANGE_LIST* em_range_list);

_int32 dm_get_range_data_buffer( DATA_MANAGER* p_data_manager_interface, RANGE need_r,  RANGE_DATA_BUFFER_LIST* ret_range_buffer);

#ifdef _VOD_NO_DISK   

_int32 dm_vod_set_no_disk_mode( DATA_MANAGER* p_data_manager_interface);

BOOL dm_vod_is_no_disk_mode(DATA_MANAGER* p_data_manager_interface);

_int32 dm_vod_no_disk_notify_free_data_buffer( DATA_MANAGER* p_data_manager_interface,   RANGE_DATA_BUFFER_LIST* del_range_buffer);

_int32 dm_notify_free_data_buffer( DATA_MANAGER* p_data_manager_interface,   RANGE_LIST* del_range_list);

_int32 dm_notify_only_free_data_buffer( DATA_MANAGER* p_data_manager_interface,   RANGE_LIST* del_range_list);

_int32 dm_drop_buffer_not_in_emerge_windows( DATA_MANAGER* p_data_manager_interface, RANGE_LIST* em_range_windows);

_int32 dm_flush_data_to_vod_cache( DATA_MANAGER* p_data_manager_interface);

#endif

_int32 dm_vod_set_check_data_mode( DATA_MANAGER* p_data_manager_interface);

BOOL dm_vod_is_check_data_mode(DATA_MANAGER* p_data_manager_interface);

_int32  dm_asyn_read_data_buffer( DATA_MANAGER* p_data_manager_interface, RANGE_DATA_BUFFER *p_data_buffer, notify_read_result p_call_back, void *p_user );

void   dm_notify_file_create_result (void* p_user_data, FILEINFO* p_file_infor, _int32 creat_result);

void  dm_notify_file_close_result  (void* p_user_data, FILEINFO* p_file_infor, _int32 close_result);

void  dm_notify_file_block_check_result(void* p_user_data, FILEINFO* p_file_infor, RANGE* p_range, BOOL  check_result);

void  dm_notify_file_result  (void* p_user_data, FILEINFO* p_file_infor, _u32 file_result);

void  dm_get_buffer_list_from_cache(void* p_user_data, RANGE* r,  RANGE_DATA_BUFFER_LIST* ret_range_buffer);

_int32 dm_get_dispatcher_data_info(DATA_MANAGER* p_data_manager_interface, DS_DATA_INTEREFACE* p_data_info);

 _u64  dm_ds_intereface_get_file_size( void* p_owner_data_manager);
BOOL dm_ds_intereface_is_only_using_origin_res(void* p_owner_data_manager);
BOOL dm_ds_intereface_is_origin_resource( void* p_owner_data_manager, RESOURCE*  res);
 _int32 dm_ds_intereface_get_priority_range(void* p_owner_data_manager, RANGE_LIST*  priority_range_list);
 _int32 dm_ds_intereface_get_uncomplete_range(void* p_owner_data_manager, RANGE_LIST*  un_complete_range_list);
_int32 dm_ds_intereface_get_error_range_block_list(void* p_owner_data_manager, ERROR_BLOCKLIST**  pp_error_block_list);	
BOOL dm_ds_is_vod_mod(void* p_owner_data_manager);

BOOL dm_abandon_resource( void* p_data_manager_interface, RESOURCE*  abdon_res);

_u32 dm_get_valid_data_speed(DATA_MANAGER* p_data_manager_interface);

void dm_get_dl_bytes(DATA_MANAGER* p_data_manager_interface, _u64 *p_server_dl_bytes, _u64 *p_peer_dl_bytes, _u64 *p_cdn_dl_bytes, _u64 *p_lixian_dl_bytes , _u64* p_appacc_dl_bytes);

_u64 dm_get_normal_cdn_down_bytes(DATA_MANAGER *data_manager);

#ifdef EMBED_REPORT
struct tagFILE_INFO_REPORT_PARA *dm_get_report_para( DATA_MANAGER* p_data_manager_interface );
#endif

BOOL dm_check_kankan_lan_file_finished(DATA_MANAGER* p_data_manager_interface);

void dm_get_origin_resource_dl_bytes(DATA_MANAGER* p_data_manager_interface, _u64 *p_origin_resource_dl_bytes);


void dm_get_assist_peer_dl_bytes(DATA_MANAGER* p_data_manager_interface, _u64 *p_assist_peer_dl_bytes);


typedef _int32 ( *DM_NOTIFY_FILE_NAME_CHANGED)(_u32 task_id, const char* file_name,_u32 length);
_int32 dm_set_file_name_changed_callback(void * callback_ptr);
_int32 dm_get_file_name_changed_callback(DM_NOTIFY_FILE_NAME_CHANGED *call_ptr);


typedef _int32 ( *DM_NOTIFY_EVENT)(_u32 task_id, _u32 event);
_int32 dm_set_notify_event_callback(void * callback_ptr);

typedef _int32 (*EM_SCHEDULER)(void);
_int32 dm_set_notify_etm_scheduler(void* callback_ptr);


_int32 dm_set_check_mode(DATA_MANAGER* p_data_manager_interface, BOOL _is_need_calc_bcid);

_int32 dm_get_download_bytes(DATA_MANAGER* p_data_manager_interface, char* host, _u64* download);

_int32 dm_get_info_from_cfg_file(char* cfg_file_path, char *origin_url, BOOL *cid_is_valid, _u8 *cid, _u64* file_size, char* file_name,char *lixian_url, char *cookie, char *user_data);

#ifdef __cplusplus
}
#endif

#endif /* !defined(__DATA_MANAGER_INTERFACE_20080604)*/



