

#if !defined(__FILE_INFO_20080730)
#define __FILE_INFO_20080730

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#include "utility/range.h"
#include "utility/mempool.h"
#include "utility/sha1.h"
#include "utility/speed_calculator.h"


#include "asyn_frame/msg.h"
#include "asyn_frame/device.h"

#include "data_manager/file_manager_interface.h"

#include "connect_manager/resource.h"
//#include "data_manager/data_manager_interface.h"
#include "data_manager/data_buffer.h"
#include "data_manager/data_receive.h"
#include "data_manager/file_manager.h"

struct tag_HASH_CALCULATOR;


#define CID_SIZE 20

#define TCID_SAMPLE_SIZE     61440
#define TCID_SAMPLE_UNITSIZE  20480
#define BCID_DEF_MIN_BLOCK_COUNT 512
#define BCID_DEF_MAX_BLOCK_SIZE  2097152
#define BCID_DEF_BLOCK_SIZE	   262144

typedef enum  _tag_check_file_type{ UNKOWN_FILE_TYPE= 1, MEDIA_FILE_TYPE , OTHER_FILE_TYPE }CHECK_FILE_TYPE;

typedef enum  _tag_bcid_level{ BLV_PEER = 1, BLV_SERVERS = 10, BLV_ONESERVER = 90 }BCID_LEVEL;

typedef struct _tag_bcid_pair
{
	_u32 _block_no; /*块号*/
	_u8   _bcid[CID_SIZE];   /*本块的cid*/
}bcid_pair;

typedef struct _tag_bcids_info /*存储/访问bcid列表信息: 块号是从0开始的*/
{  
    BCID_LEVEL  _check_level; 
     _u32  _bcid_num;
     _u8*  _bcid_buffer;
     _u8*  _bcid_bitmap;	 
     _u8*  _bcid_checking_bitmap;
	 _u64  _last_bg_bno;
	 _u64  _last_end_bno;
}bcid_info;

typedef struct  _tag_block_caculator
{
     LIST _need_calc_blocks;    
     //ctx_sha1 _hash_ptr;
     _u8* _data_buffer;
     RANGE_DATA_BUFFER* _p_buffer_data; 
     _u32  _cal_length;
    _u32  _cal_blockno;
    _u32  _expect_length;
}BLOCK_CACULATOR;

 struct _tag_file_info;

typedef  void  (* notify_file_create_result)(void* p_user_data, struct _tag_file_info* p_file_infor, _int32 creat_result);
typedef void  (*notify_file_close_result)(void* p_user_data, struct _tag_file_info* p_file_infor, _int32 close_result);
typedef void  (*notify_file_block_check_result)(void* p_user_data, struct _tag_file_info* p_file_infor, RANGE* p_range, BOOL  check_result);
typedef void  (*notify_file_result)(void* p_user_data, struct _tag_file_info* p_file_infor, _u32 file_result);
typedef void  (*fm_get_buffer_list_from_cache)(void* p_user_data, RANGE* r,  RANGE_DATA_BUFFER_LIST* ret_range_buffer);

typedef struct _tag_file_info_callback
{
       notify_file_create_result _p_notify_create_event;
       notify_file_close_result _p_notify_close_event;
	notify_file_block_check_result  _p_notify_check_event;	
	notify_file_result  _p_notify_file_result_event;
	fm_get_buffer_list_from_cache _p_get_buffer_list_from_cache;
	
} FILE_INFO_CALLBACK;

#ifdef EMBED_REPORT

typedef struct tagFILE_INFO_REPORT_PARA
{
	_u64  _down_exist;					 //断点续传前数据量 																
	_u64  _overlap_bytes;				 //重叠数据大小 	
	
	_u64  _down_n2i_no; 				 //内网从外网非边下边传下载字节数													
	_u64  _down_n2i_yes;				 //内网从外网边下边传下载字节数 													
	_u64  _down_n2n_no; 				 //内网从内网非边下边传下载字节数													
	_u64  _down_n2n_yes;				 //内网从内网边下边传下载字节数 													
	_u64  _down_n2s_no; 				 //同子网内网非边下边传下载字节数													
	_u64  _down_n2s_yes;				 //同子网内网边下边传下载字节数 													
	_u64  _down_i2i_no; 				 //外网从外网非边下边传下载字节数													
	_u64  _down_i2i_yes;				 //外网从外网边下边传下载字节数 													
	_u64  _down_i2n_no; 				 //外网从内网非边下边传下载字节数													
	_u64  _down_i2n_yes;				 //外网从内网边下边传下载字节数 	
	
	_u64  _down_by_partner_cdn; 		//partner_Cdn纠错字节		
	
	_u64  _down_err_by_cdn; 			 //Cdn纠错字节		
	_u64  _down_err_by_partner_cdn; 	//partner_Cdn纠错字节		
	_u64  _down_err_by_peer;			 //Peer纠错字节 																	
	_u64  _down_err_by_svr; 			 //Server纠错字节	
	_u64  _down_err_by_appacc;			//应用市场加速纠错字节

} FILE_INFO_REPORT_PARA;
#endif

typedef struct _tag_file_info
{
	char _file_name[MAX_FILE_NAME_BUFFER_LEN];
		
	char _path[MAX_FILE_PATH_LEN];

	char _user_file_name[MAX_FILE_NAME_BUFFER_LEN];
	BOOL _has_postfix;
	_u32  _cfg_file;

	_u64 _file_size;
    _u32 _block_size;  
	_u32 _unit_num_per_block;   

    BOOL  _cid_is_valid;  
	_u8 _cid[CID_SIZE];

	BOOL  _gcid_is_valid;  
	_u8 _gcid[CID_SIZE];

    _u8 *_user_data;
    _int32 _user_data_len;

	BOOL _bcid_enable;       // emule 不关心file_info里面的bcid校验情况。
	BOOL _bcid_is_valid;
    BOOL _is_calc_bcid;//对于emule来说 不关心bcid
	 bcid_info  _bcid_info;
	 BOOL _is_backup_bcid_valid;
	 bcid_info  _bcid_info_backup;
	 BOOL  _bcid_map_from_cfg;
	 BOOL _has_history_check_info;

	char _origin_url[MAX_URL_LEN];
	char _origin_ref_url[MAX_URL_LEN];
	char _cookie[MAX_COOKIE_LEN];

	/*当前已校验的range列表，列表包括还在主缓冲区和纠错缓冲区中的已接收块*/
	RANGE_LIST _done_ranges;
	/*当前已写入的range列表，列表包括校验和未校验的*/
	RANGE_LIST _writed_range_list;  

	DATA_RECEIVER _data_receiver;

	RANGE_DATA_BUFFER_LIST _flush_buffer_list;

       BLOCK_CACULATOR _block_caculator;

	struct tagTMP_FILE*  _tmp_file_struct;

	FILE_INFO_CALLBACK  _call_back_func_table;
	void*  _p_user_ptr;

	 RANGE_LIST _no_need_check_range;  // 视频文件允许有些块不校验
	 CHECK_FILE_TYPE _check_file_type;

	 BOOL   _wait_for_close;

#ifdef _VOD_NO_DISK
       BOOL _is_no_disk;
#endif

         SPEED_CALCULATOR _cur_data_speed;

#ifdef EMBED_REPORT
	FILE_INFO_REPORT_PARA _report_para;
#endif
    BOOL _is_reset_bcid;
	FILE_WRITE_MODE _write_mode;
	BOOL _finished;

	BOOL _is_need_calc_bcid;

    struct tag_HASH_CALCULATOR* _hash_calculator;

}FILEINFO;


char*  get_file_suffix( const char* filename );
BOOL is_excellent_filename( char* filename );

_int32 file_info_adjust_max_read_length(void);

_int32 init_file_info_slabs(void);
_int32 uninit_file_info_slabs(void);
_int32 file_info_alloc_node(FILEINFO** pp_node);
_int32 file_info_free_node(FILEINFO* p_node);

_int32  init_file_info(FILEINFO* p_file_info, void* owner_ptr);
/*  file_info_close_tmp_file must be reference before and the close callback must return */
_int32  unit_file_info(FILEINFO* p_file_info);

BOOL  file_info_has_tmp_file(FILEINFO* p_file_info);

_int32  file_info_register_callbacks(FILEINFO* p_file_info, FILE_INFO_CALLBACK* _callback_funcs);

_int32  file_info_set_td_flag(FILEINFO* p_file_info);

//_int32  file_info_erase_error_range(FILEINFO* p_file_info, RANGE* r);

_int32  file_info_add_done_range(FILEINFO* p_file_info, RANGE* r);
_int32	file_info_add_check_done_range(FILEINFO* p_file_info, RANGE* r);
_int32	file_info_sync_bcid_bitmap(FILEINFO *p_file_info, const RANGE r);
_int32  file_info_add_erase_range(FILEINFO* p_file_info, RANGE* r);

_int32  file_info_set_user_name(FILEINFO* p_file_info, char* userfilename, char* filepath);


_int32  file_info_set_final_file_name(FILEINFO* p_file_info, char*  finalname);
_int32  file_info_set_origin_url(FILEINFO* p_file_info, char* url, char* origin_ref_url);

_int32  file_info_set_cookie(FILEINFO* p_file_info, char* cookie);

BOOL file_info_get_origin_url( FILEINFO* p_file_info,char** origin_url);
	
BOOL file_info_get_origin_ref_url( FILEINFO* p_file_info,char** origin_ref_url);

BOOL file_info_get_file_path( FILEINFO* p_file_info,char** file_path);

BOOL   file_info_get_file_name(FILEINFO* p_file_info, char** file_name);

BOOL   file_info_get_user_file_name(FILEINFO* p_file_info, char** file_name);

void  file_info_decide_finish_filename(FILEINFO* p_file_info);

//_int32  file_info_set_filesize(FILEINFO* p_file_info, _u64 filesize);
_u64  file_info_get_filesize(FILEINFO* p_file_info);

_u32  file_info_get_blocksize(FILEINFO* p_file_info);

BOOL  file_info_filesize_is_valid(FILEINFO* p_file_info);

//_int32  file_info_set_down_mode(FILEINFO* p_file_info, BOOL  use_url_down);
BOOL  file_info_filesize_is_use_url_down(FILEINFO* p_file_info);


_int32  file_info_set_cid(FILEINFO* p_file_info, _u8 cid[CID_SIZE]);
_int32  file_info_set_gcid(FILEINFO* p_file_info, _u8 gcid[CID_SIZE]);
_int32 file_info_set_user_data(FILEINFO* fi, const _u8 *user_data, _int32 len);

_int32  file_info_invalid_cid(FILEINFO* p_file_info);
_int32  file_info_invalid_gcid(FILEINFO* p_file_info);

BOOL  file_info_get_shub_cid(FILEINFO* p_file_info, _u8 cid[CID_SIZE]);
BOOL  file_info_get_shub_gcid(FILEINFO* p_file_info, _u8 gcid[CID_SIZE]);

BOOL file_info_cid_is_valid(FILEINFO* p_file_info);

BOOL file_info_gcid_is_valid(FILEINFO* p_file_info);

BOOL file_info_check_cid(FILEINFO* p_file_info, _int32* err_code, BOOL* read_file_error, char* file_calc_cid);

BOOL file_info_check_gcid(FILEINFO* p_file_info);

//_int32  file_info_add_range(FILEINFO* p_file_info, RANGE* r);
//_int32  file_info_add_rangelist(FILEINFO* p_file_info, RANGE_LIST* r_list);
_int32  file_info_delete_range(FILEINFO* p_file_info, RANGE* r);
_int32 file_info_reset_bcid_info(FILEINFO* p_file_info);

_int32  file_info_delete_buffer_by_range(FILEINFO* p_file_info, RANGE* r);

BOOL  file_info_add_no_need_check_range(FILEINFO* p_file_info, RANGE* r);
//_int32  file_info_delete_rangelist(FILEINFO* p_file_info, RANGE_LIST* r_list);

_int32  file_info_open_cfg_file(FILEINFO* p_file_info,char * cfg_file_path);

_int32  file_info_close_cfg_file(FILEINFO* p_file_info);

_int32  file_info_delete_cfg_file(FILEINFO* p_file_info);

_int32  file_info_delete_tmp_file(FILEINFO* p_file_info);

_int32 file_info_set_filesize(FILEINFO* p_file_info, _u64 filesize);

_u32 file_info_set_hub_return_info(FILEINFO* p_file_info,_u64 filesize,_u32 gcid_type,  _u8 block_cid[], _u32 block_count, _u32 block_size );

_int32 file_info_filesize_change(FILEINFO* p_file_info, _u64 filesize);

_u32 start_check_blocks(FILEINFO* p_file_info);

_u32 clear_check_blocks(FILEINFO* p_file_info);

_int32 file_info_put_data(FILEINFO* p_file_info , const RANGE *r, char **data_ptr, _u32 data_len,_u32 buffer_len);
_int32 file_info_put_safe_data(FILEINFO* p_file_info , const RANGE *r, char **data_ptr, _u32 data_len,_u32 buffer_len);

_int32  file_info_flush_data_to_file(FILEINFO* p_file_info, BOOL force);

RANGE_DATA_BUFFER_LIST* file_info_get_cache_range_buffer_list(FILEINFO* p_file_info);

_int32 file_info_flush_cached_buffer(FILEINFO* p_file_info);

BOOL file_info_ia_all_received(FILEINFO* p_file_info);

BOOL file_info_range_is_recv(FILEINFO* p_file_info, const RANGE* r);

BOOL file_info_range_is_write(FILEINFO* p_file_info, const RANGE* r);

BOOL file_info_range_is_check(FILEINFO* p_file_info, const RANGE* r);

BOOL file_info_range_is_cached(FILEINFO* p_file_info, const RANGE* r);

BOOL file_info_rangelist_is_write(FILEINFO* p_file_info, RANGE_LIST* r_list);


/*   以下函数得到的range_list 只能读不能写!!!!*/
RANGE_LIST* file_info_get_recved_range_list(FILEINFO* p_file_info);

RANGE_LIST* file_info_get_writed_range_list(FILEINFO* p_file_info);

RANGE_LIST* file_info_get_checked_range_list(FILEINFO* p_file_info);

void  file_info_clear_all_recv_data(FILEINFO* p_file_info);

void  file_info_asyn_all_recv_data(FILEINFO* p_file_info);

BOOL  file_info_has_recved_data(FILEINFO* p_file_info);

_u32  file_info_get_file_percent(FILEINFO* p_file_info);

_u64  file_info_get_download_data_size(FILEINFO* p_file_info);

_u64  file_info_get_writed_data_size(FILEINFO* p_file_info);

_int32  file_info_set_write_mode(FILEINFO* p_file_info,FILE_WRITE_MODE write_mode);
_int32  file_info_get_write_mode(FILEINFO* p_file_info,FILE_WRITE_MODE * write_mode);
	
_int32 file_info_get_range_data_buffer( FILEINFO* p_file_info, RANGE need_r,  RANGE_DATA_BUFFER_LIST* ret_range_buffer);

#ifdef _VOD_NO_DISK   

_int32 file_info_vod_set_no_disk_mode( FILEINFO* p_file_info);
_int32 file_info_vod_no_disk_notify_free_data_buffer( FILEINFO* p_file_info,   RANGE_DATA_BUFFER_LIST* del_range_buffer);
#endif

_int32  file_info_asyn_read_data_buffer( FILEINFO* p_file_info, RANGE_DATA_BUFFER *p_data_buffer, notify_read_result p_call_back, void *p_user );

//如果要读的内容在cache中，则返回SUCCESS,读取成功，否则返回ERR_WAIT_NOTIFY,等待回调通知
_int32 file_info_read_data(FILEINFO* file_info, RANGE_DATA_BUFFER* data_buffer, notify_read_result callback, void* user_data);

_int32 file_info_get_flush_data(FILEINFO* p_file_info, RANGE_DATA_BUFFER_LIST* need_flush_data, BOOL force_flush);

_u32 compute_block_size(_u64 file_size);

_int32 compute_3part_range_list(_u64 file_size, RANGE_LIST*t_list);

_u32 get_bitmap_index(_u32 blockno);

_u32 get_bitmap_off(_u32 blockno);

_u32 get_read_length(void);

_u32 get_read_length_for_bcid_check(void);

_u32 get_data_unit_index(_u32 block_no, _u32 unit_num_per_block);

 RANGE to_range(_u32 block_no, _u32 block_size, _u64 file_size);

 RANGE range_to_block_range(RANGE* r, _u32 block_size, _u64 file_size);

BOOL range_list_length_is_enough(RANGE_LIST* range_list, _u64 filesize);

_u32 to_blockno(_u64 pos, _u32 block_size);

void clear_list(LIST* l);

_u32 prepare_for_bcid_info(bcid_info* p_bcid_info, _u32 bcid_num);

void file_info_close_tmp_file(FILEINFO* p_file_info);

BOOL file_info_file_is_create(FILEINFO* p_file_info);

_int32 file_info_try_create_file(FILEINFO* p_file_info);

BOOL  file_info_bcid_valid(FILEINFO* p_file_info);

_int32  file_info_invalid_bcid(FILEINFO* p_file_info);

_u32  file_info_get_bcid_num(FILEINFO* p_file_info);

_u8*  file_info_get_bcid_buffer(FILEINFO* p_file_info);

_u32 prepare_for_shahash(FILEINFO* p_file_info);

_u32 check_block(FILEINFO* p_file_info ,_u32 blockno, RANGE_DATA_BUFFER_LIST* buffer, BOOL drop_over_data);

_u32 set_block_cid(bcid_info*  p_bcids ,_u32 blockno, _u8 cal_bcid[CID_SIZE] );

_u32 verify_block_cid(bcid_info*  p_bcids ,_u32 blockno, _u8 cal_bcid[CID_SIZE]);

_u32 set_blockmap(bcid_info*  p_bcids, _u32 blockno);

_u32 clear_blockmap(bcid_info*  p_bcids, _u32 blockno);

BOOL blockno_is_set(bcid_info*  p_bcids, _u32 blockno);

BOOL blockno_map_is_set(_u8*  p_bcid_map, _u32 blockno);

BOOL file_info_is_all_checked(FILEINFO* p_file_info);

BOOL blockno_is_all_checked(bcid_info*  p_bcids);

_u32 calc_block_cid(FILEINFO* p_file_info ,_u32 blockno, RANGE_DATA_BUFFER_LIST* buffer, _u8 ret_bcid[20], BOOL drop_over_data);

_u32 add_check_block(FILEINFO* p_file_info ,_u32 blockno);

_u32 prepare_for_readbuffer(FILEINFO* p_file_info, _u32 blockno );

BOOL file_info_has_block_need_check(FILEINFO* p_file_info);

_u32 start_check_blocks(FILEINFO* p_file_info);

_u32 clear_check_blocks(FILEINFO* p_file_info);

_u32 handle_read_failure(FILEINFO* p_file_info, _int32 error);

_u32 handle_write_failure(FILEINFO* p_file_info, _int32 error);

_u32 handle_create_failure(FILEINFO* p_file_info, _int32 error);

_u32 notify_read_data_result(struct tagTMP_FILE*  file_struct, void* user_ptr,RANGE_DATA_BUFFER* data_buffer, _int32 read_result, _int32 read_len);

_u32 file_info_flush_data(FILEINFO* p_file_info, RANGE_DATA_BUFFER_LIST* buffer);

_u32 notify_write_data_result(struct tagTMP_FILE*  file_struct, void* user_ptr,RANGE_DATA_BUFFER_LIST* buffer_list, _int32 write_result);

_u32 file_info_cal_need_check_block(FILEINFO* p_file_info, RANGE_DATA_BUFFER_LIST* buffer);

BOOL get_file_3_part_cid(FILEINFO* p_file_info, _u8 cid[CID_SIZE] , _int32* err_code);

BOOL get_file_gcid_helper(bcid_info *_bcid_info, _u8 gcid[CID_SIZE], BOOL is_bakup);

BOOL get_file_gcid_backup(FILEINFO* p_file_info, _u8 gcid[CID_SIZE]);

BOOL get_file_gcid(FILEINFO* p_file_info, _u8 gcid[CID_SIZE]);

_int32 file_info_notify_file_create(struct tagTMP_FILE* file_struct, void* user_ptr, _int32 create_result);

_int32 file_info_notify_file_close(struct tagTMP_FILE* file_struct, void* user_ptr, _int32 close_result);

_int32  file_info_change_finish_filename(FILEINFO* p_file_info, char* p_final_name);

_int32 file_info_notify_fm_can_close(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid);

_int32 file_info_notify_filesize_change(struct tagTMP_FILE* file_struct, void* user_ptr, _int32 create_result);

_int32 file_info_change_td_name( FILEINFO* p_file_info );

BOOL file_info_save_to_cfg_file(FILEINFO* p_file_info);

BOOL file_info_load_from_cfg_file(FILEINFO* p_file_info, char* cfg_file_path);

_u64 file_info_get_downsize_from_cfg_file(char* cfg_file_path);
_u32 file_info_get_valid_data_speed(FILEINFO* p_file_info );

_int32 file_info_set_bcid_enable(FILEINFO* p_file_info, BOOL enable);
_int32 file_info_set_is_calc_bcid(FILEINFO* p_file_info, BOOL is_calc);

_int32 file_info_set_check_mode(FILEINFO* p_file_info, BOOL _is_need_calc_bcid);

_u32 file_info_set_hub_return_info2(FILEINFO* p_file_info
									, _u32 gcid_type
									, _u8 block_cid[]
									, _u32 block_count
									, _u32 block_size );

_int32 file_info_get_info_from_cfg_file(char* cfg_file_path, char *origin_url, BOOL *cid_is_valid, _u8 *cid, _u64* file_size, char* file_name, char *lixian_url, char *cookie, char *user_data);

#ifdef EMBED_REPORT

FILE_INFO_REPORT_PARA *file_info_get_report_para( FILEINFO* p_file_info );
void file_info_handle_valid_data_report(FILEINFO* p_file_info, RESOURCE *resource_ptr, _u32 data_len );
void file_info_handle_err_data_report(FILEINFO* p_file_info, RESOURCE *resource_ptr, _u32 data_len );
void file_info_add_overloap_date(FILEINFO* p_file_info, _u32 date_len);

#endif

_int32 dm_get_enable_fast_stop();
_int32 dm_donot_fast_stop_hook(const _int32 down_task);

#ifdef __cplusplus
}
#endif

#endif /* !defined(__FILE_INFO_20080730)*/

