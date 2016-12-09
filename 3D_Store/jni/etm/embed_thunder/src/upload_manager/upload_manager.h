#if !defined(__UPLOAD_MANAGER_H_20081111)
#define __UPLOAD_MANAGER_H_20081111


#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"

#ifdef UPLOAD_ENABLE

#include "utility/list.h"
#include "utility/errcode.h"
#include "connect_manager/data_pipe.h"
#include "asyn_frame/msg.h"
#include "upload_manager/ulm_file_manager.h"
#include "upload_manager/ulm_pipe_manager.h"

#include "data_manager/file_manager_interface.h"
#ifdef ENABLE_BT
#include "bt_download/bt_data_manager/bt_data_manager_interface.h"
#endif

/*--------------------------------------------------------------------------*/
/*                Interfaces provid for task_manager				        */
/*--------------------------------------------------------------------------*/
_int32 init_upload_manager(void);
_int32 uninit_upload_manager(void);
_int32 ulm_close_upload_pipes(void *event_handle );
_int32 ulm_close_upload_pipes_notify(void *event_handle );
_int32 ulm_set_record_list_file_path( const char *path,_u32  path_len);


_u32 ulm_get_cur_upload_speed( void);

_u32 ulm_get_max_upload_speed( void);

/*--------------------------------------------------------------------------*/
/*                Interfaces provid for download_task				        */
/*--------------------------------------------------------------------------*/
// 添加资源
_int32 ulm_add_record(_u64 size, const _u8 *tcid, const _u8 *gcid, _u8 chub, const char *path);
// 删除资源
_int32 ulm_del_record(_u64 size, const _u8 *tcid,const _u8 *gcid);
//比较文件大小和文件路径名，如果相同就可以删除
_int32 ulm_del_record_by_full_file_path(_u64 size,  const char *path);

// 文件重名后，更新记录
_int32 ulm_change_file_path(_u64 size, const char *old_path, const char *new_path);

/*--------------------------------------------------------------------------*/
/*                Interfaces provid for data_manager				        */
/*--------------------------------------------------------------------------*/
//
_int32 ulm_notify_have_block( const _u8 *gcid);
/*--------------------------------------------------------------------------*/
/*                Interfaces provid for p2p/bt pipe				        */
/*--------------------------------------------------------------------------*/
// 查询资源
BOOL ulm_is_file_exist(const _u8* gcid, _u64 file_size);


BOOL ulm_is_pipe_full(void);
//Add pipe
_int32 ulm_add_pipe_by_gcid( const _u8 *gcid  ,DATA_PIPE * p_pipe);

_int32 ulm_remove_pipe( DATA_PIPE * p_pipe);


_int32 ulm_read_data(void * data_manager,RANGE_DATA_BUFFER * p_data_buffer, read_data_callback_func callback, DATA_PIPE * p_pipe);

_int32 ulm_cancel_read_data( DATA_PIPE * p_pipe);

_int32 ulm_get_file_size( DATA_PIPE * p_pipe,_u64 * file_size);

#ifdef ENABLE_BT
_int32 ulm_add_pipe_by_info_hash( const _u8 *info_hash ,DATA_PIPE * p_pipe);
_int32 ulm_bt_pipe_read_data(void * data_manager,const BT_RANGE *p_bt_range, 
	char *p_data_buffer, _u32 buffer_len, bt_pipe_read_result p_call_back, DATA_PIPE * p_pipe );
#endif

#ifdef ENABLE_EMULE
_int32 ulm_add_pipe_by_file_hash( const _u8 *file_hash, DATA_PIPE * p_pipe);
_int32 ulm_emule_pipe_read_data(void * data_manager,RANGE_DATA_BUFFER * p_data_buffer, read_data_callback_func callback, DATA_PIPE * p_pipe);
#endif

/*统计上报*/
void ulm_stat_add_up_bytes(_u32 bytes);
void ulm_stat_handle_report_response(_u32 seqno , _int32 result);


/*--------------------------------------------------------------------------*/
/*                Interfaces provid for ulm_file_manager				        */
/*--------------------------------------------------------------------------*/
//
BOOL ulm_handle_invalid_record( const _u8 *gcid,_int32 err_code);

/*--------------------------------------------------------------------------*/
/*                Interfaces provid for reporter				        */
/*--------------------------------------------------------------------------*/
_int32  ulm_isrc_online_resp( BOOL result, _int32 should_report_rcList );
_int32  ulm_report_rclist_resp( BOOL result );
_int32  ulm_insert_rc_resp( BOOL result);
_int32  ulm_delete_rc_resp(BOOL result);



#ifdef _CONNECT_DETAIL
void ulm_update_pipe_info(void);
_int32 ulm_get_pipe_info_array(PEER_PIPE_INFO_ARRAY * p_pipe_info_array);
#endif

/*--------------------------------------------------------------------------*/
/*                Callback function for timer				        */
/*--------------------------------------------------------------------------*/
_int32 ulm_handle_scheduler_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid);
 _int32 ulm_handle_isrc_online_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired,_u32 msgid);

/*--------------------------------------------------------------------------*/
/*                Internal functions				        */
/*--------------------------------------------------------------------------*/
BOOL  ulm_is_record_exist_by_gcid( const _u8 *gcid );

//_int32 ulm_add_pipe_by_size_tcid( _u64 size, const _u8 *tcid ,DATA_PIPE * p_pipe,notify_file_create p_call_back);
//_int32 ulm_add_pipe_by_gcid( const _u8 *gcid  ,DATA_PIPE * p_pipe,notify_file_create p_call_back);
//_int32 ulm_add_pipe_by_path( const char *path,DATA_PIPE * p_pipe,notify_file_create p_call_back);

//BOOL  ulm_is_pipes_full( void );


//_int32 ulm_add_pipe(DATA_PIPE * p_pipe);
_int32 ulm_scheduler(void);
//_int32 ulm_clean_pipes(void);
/* Check if the file is not modified */
BOOL ulm_check_is_file_modified(_u64 filesize,const char * filepath);
_int32 ulm_failure_exit(_int32 err_code);



#endif /* UPLOAD_ENABLE */

#ifdef __cplusplus
}
#endif

#endif // !defined(__UPLOAD_MANAGER_H_20081111)
