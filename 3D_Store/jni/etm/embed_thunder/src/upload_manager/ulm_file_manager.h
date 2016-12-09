#if !defined(__ULM_FILE_MANAGER_H_20090604)
#define __ULM_FILE_MANAGER_H_20090604


#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"

#ifdef UPLOAD_ENABLE

#include "utility/list.h"
#include "utility/map.h"
#include "utility/errcode.h"
#include "connect_manager/data_pipe.h"
#include "asyn_frame/msg.h"
#include "upload_manager/rclist_manager.h"

#include"data_manager/upload_read_file.h"
#include "data_manager/file_manager.h"

enum UFM_FILE_STATE {UFM_OPENING= 0, UFM_OPENED,UFM_READING,UFM_CLOSING};

typedef _int32 (*read_data_callback_func) (_int32 errcode,RANGE_DATA_BUFFER * p_data_buffer, void* user_data);


typedef struct 
{
	RANGE_DATA_BUFFER  * _p_data_buffer; 
	void* _callback; 
	void* _user_data;  //p2p_pipe
	BOOL _is_cancel;
	_u32 _time_stamp;	/* Wait if the data is writing to the disk */
	void* _data_manager_ptr; 
	_u32 _file_index;	
}UFM_FILE_READ;

typedef struct 
{
	TMP_FILE *_file_struct;
	enum UFM_FILE_STATE _state;
	LIST			_read_list;			/*read  queue */ 
	UFM_FILE_READ*	_cur_read;			/*last read which is processing*/
	//_u8  _gcid[CID_SIZE];
	_u32 _time_stamp;	/* Wait 2 second before closing the file when no read command on this file */
}UFM_FILE_STRUCT;


typedef struct 
{
	MAP	_file_structs;		// GCIDË÷Òý //UFM_FILE_STRUCT
	MAP	_dm_file_read;		// data_pipeË÷Òý UFM_FILE_READ
	_u32 	_max_files;
	_u32 	_max_idle_interval;
	void * _event_handle;
	_u32 	_file_num_wait_close;
}ULM_FILE_MANAGER;


/*--------------------------------------------------------------------------*/
/*                Interfaces provid for upload_manager				        */
/*--------------------------------------------------------------------------*/
_int32 init_upload_file_manager(void);
_int32 uninit_upload_file_manager(void);

_int32 ufm_read_file(ID_RC_INFO *p_file_info,RANGE_DATA_BUFFER * p_data_buffer, void* callback, void* user_data);
_int32 ufm_read_file_from_common_data_manager(void * data_manager,RANGE_DATA_BUFFER * p_data_buffer, void* callback, DATA_PIPE * p_pipe);
#ifdef ENABLE_BT
_int32 ufm_speedup_pipe_read_file_from_bt_data_manager(void * data_manager,RANGE_DATA_BUFFER * p_data_buffer, void* callback, DATA_PIPE * p_pipe);
_int32 ufm_bt_pipe_read_file_from_bt_data_manager(void * data_manager,const BT_RANGE *p_bt_range, 
	char *p_data_buffer, _u32 buffer_len, void *callback, 	DATA_PIPE * p_pipe);
#endif

#ifdef ENABLE_EMULE
_int32 ufm_speedup_pipe_read_file_from_emule_data_manager(void * data_manager,RANGE_DATA_BUFFER * p_data_buffer, void* callback, DATA_PIPE * p_pipe);
_int32 ufm_read_file_from_emule_data_manager(void * data_manager,RANGE_DATA_BUFFER * p_data_buffer, void* callback, DATA_PIPE * p_pipe);

#endif
void ufm_scheduler(void);

/*--------------------------------------------------------------------------*/
/*                Interfaces provid for ulm_pipe_manager				        */
/*--------------------------------------------------------------------------*/
BOOL ufm_check_need_cancel_read_file(const _u8 *gcid, void* user_data);
_int32 ufm_cancel_read_file(const _u8 *gcid, void* user_data);
_int32 ufm_cancel_read_from_data_manager(void* user_data);

BOOL ufm_is_file_full(void);
_u32 ufm_get_file_score(const _u8 *gcid);

/*--------------------------------------------------------------------------*/
/*                Interfaces provid for rclist_manager 				        */
/*--------------------------------------------------------------------------*/
_int32 ufm_close_file(const _u8 *gcid);

/*--------------------------------------------------------------------------*/
/*                Interfaces provid for upload_read_file				        */
/*--------------------------------------------------------------------------*/
_int32 ufm_notify_file_create (struct tagTMP_FILE *p_file_struct, void *p_user, _int32 create_result);
_int32 ufm_notify_file_read_result (struct tagTMP_FILE *p_file_struct, void *p_user, RANGE_DATA_BUFFER *data_buffer, _int32 read_result, _u32 read_len );
_int32 ufm_notify_file_close  (struct tagTMP_FILE *p_file_struct, void *p_user, _int32 close_result);
_int32 ufm_data_manager_notify_file_read_result (struct tagTMP_FILE *p_file_struct, void *p_user, RANGE_DATA_BUFFER *data_buffer, _int32 read_result, _u32 read_len );
#ifdef ENABLE_BT
_int32 ufm_bt_data_manager_notify_bt_range_read_result (void *p_user, BT_RANGE *p_bt_range, char *p_data_buffer, _int32 read_result, _u32 read_len );
#endif

/*--------------------------------------------------------------------------*/
/*                Internal functions				        */
/*--------------------------------------------------------------------------*/
_int32 ufm_map_compare_fun( void *E1, void *E2 );
_int32 ufm_dm_file_read_map_compare_fun( void *E1, void *E2 );
BOOL ufm_is_file_open(const _u8 *gcid);
BOOL ufm_is_pipe_exist_in_dm_map(DATA_PIPE * p_pipe);
UFM_FILE_STRUCT * ufm_get_file_struct(const _u8 *gcid);
_u8 * ufm_get_file_gcid(UFM_FILE_STRUCT *p_file_struct);
_int32 ufm_failure_exit(_int32 err_code);
_int32	ufm_clear(void * event_handle);

_int32 ufm_add_file_struct_to_map(UFM_FILE_STRUCT *p_file_struct,const _u8* gcid);
_int32 ufm_del_file_struct_from_map(const _u8* gcid);

_int32 ufm_create_file_struct(UFM_FILE_STRUCT ** pp_file_struct);
_int32 ufm_open_file_struct(ID_RC_INFO *p_file_info,UFM_FILE_STRUCT * p_file_struct);
_int32 ufm_close_file_struct(UFM_FILE_STRUCT *p_file_struct);
_int32 ufm_delete_file_struct(UFM_FILE_STRUCT *p_file_struct);

_int32 ufm_create_file_read(RANGE_DATA_BUFFER * p_data_buffer, void* callback, void* user_data,UFM_FILE_READ ** pp_file_read);
_int32 ufm_commit_file_read(UFM_FILE_STRUCT *p_file_struct,UFM_FILE_READ *p_file_read);
_int32 ufm_execute_file_read(UFM_FILE_STRUCT *p_file_struct);
_int32 ufm_cancel_file_read(UFM_FILE_STRUCT *p_file_struct,void* user_data);
_int32 ufm_stop_file_read(UFM_FILE_READ * p_file_read);
_int32 ufm_delete_file_read(UFM_FILE_READ * p_file_read);
_int32 ufm_execute_dm_file_read(UFM_FILE_READ * p_file_read,DATA_PIPE * p_pipe);


#endif /* UPLOAD_ENABLE */

#ifdef __cplusplus
}
#endif

#endif // !defined(__ULM_FILE_MANAGER_H_20090604)
