
#if !defined(__VOD_DATA_MANAGER_INTERFACE_20090201)
#define __VOD_DATA_MANAGER_INTERFACE_20090201

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#include "utility/range.h"


typedef _int32 (*vdm_recv_handler)(_int32 errcode, _u32 pending_op_count, char* buffer, _u32 had_recv, void* user_data);

struct _tag_task;

typedef struct _tag_vod_range_data_buffer
{
       RANGE_DATA_BUFFER _range_data_buffer;
	_int32                        _dead_time;
	_int32                        _is_recved;
}VOD_RANGE_DATA_BUFFER;

typedef struct _tag_vod_range_data_buffer_list
{
      LIST   _data_list;  
	/*list for VOD_RANGE_DATA_BUFFER*/

}VOD_RANGE_DATA_BUFFER_LIST;

typedef void(*NotifyCanReadDataChange)(void *args, _u64 pos);

typedef enum _VOD_LAST_TIME_TYPE{VOD_LAST_FIRST_BUFFERING=1, VOD_LAST_INTERRUPT, VOD_LAST_DRAG, VOD_LAST_NONE}VOD_LAST_TIME_TYPE;
typedef struct   _tag_vod_data_manager 
{
	_u64  _current_postion;
	_u32  _vdm_timer;
	VOD_RANGE_DATA_BUFFER_LIST _buffer_list;
	RANGE_LIST             _range_buffer_list;
	RANGE_LIST             _range_index_list;
	RANGE_LIST             _range_waiting_list;
	RANGE_LIST             _range_last_em_list;
	RANGE                  _current_download_window;
	RANGE                  _current_buffering_window;
	_u64                   _current_play_pos;
    NotifyCanReadDataChange _can_read_data_notify;
    void *_can_read_data_notify_args;
    _u64 _last_notify_read_pos;
    struct _tag_task* _task_ptr;
    _u32 _task_id;
    _u8 _eigenvalue[CID_SIZE];

	_u32   _is_fetching;
	_u64   _start_pos;
	_u64   _fetch_length;
	_u32   _bit_per_seconds;
	char*  _user_buffer;
	vdm_recv_handler  _callback_recv_handler;
	_int32  _block_time;
	void*   _user_data;
	_u32   _last_fetch_time;
	_u32   _last_fetch_time_for_same_pos;
	BOOL  _send_pause;
	_u32  _file_index;
	BOOL _cdn_using;
	_u32  _last_begin_play_time;
	_u64  _last_read_pos;
#ifdef EMBED_REPORT
       VOD_LAST_TIME_TYPE _vod_last_time_type;
       _u32       _vod_last_time;
       _u32	_vod_play_times;
       _u32       _vod_play_begin_time;
	_u32       _vod_first_buffer_time_len; 
	_u32       _vod_failed_times;
	_u32       _vod_play_drag_num;
	_u32       _vod_play_total_drag_wait_time;
	_u32       _vod_play_max_drag_wait_time;
	_u32       _vod_play_min_drag_wait_time;
	_u32       _vod_play_interrupt_times;
	_u32       _vod_play_total_buffer_time_len;
	_u32       _vod_play_max_buffer_time_len;
	_u32       _vod_play_min_buffer_time_len;
	
	_u32 _play_interrupt_1;         // 1分钟内中断
	_u32 _play_interrupt_2;	        // 2分钟内中断
	_u32 _play_interrupt_3;		    //6分钟内中断
	_u32 _play_interrupt_4;		    //10分钟内中断
	_u32 _play_interrupt_5;		    //15分钟内中断
	_u32 _play_interrupt_6;		    //15分钟以上中断
	_u32 _cdn_stop_times;		    //关闭cdn次数
	BOOL _is_ad_type;			//是否广告任务
#endif
	_u32   _flv_header_to_end_of_first_tag_len;
}VOD_DATA_MANAGER ;
typedef struct t_vod_report
{
       _u32	_vod_play_time_len;//点播时长,单位均为秒
       _u32  _vod_play_begin_time;//开始点播时间
	_u32  _vod_first_buffer_time_len; //首缓冲时长
	_u32  _vod_play_drag_num;//拖动次数
	_u32  _vod_play_total_drag_wait_time;//拖动后等待总时长
	_u32  _vod_play_max_drag_wait_time;//最大拖动等待时间
	_u32  _vod_play_min_drag_wait_time;//最小拖动等待时间
	_u32  _vod_play_interrupt_times;//中断次数
	_u32  _vod_play_total_buffer_time_len;//中断等待总时长
	_u32  _vod_play_max_buffer_time_len;//最大中断等待时间
	_u32  _vod_play_min_buffer_time_len;//最小中断等到时间
	
	_u32 _play_interrupt_1;         // 1分钟内中断
	_u32 _play_interrupt_2;	        // 2分钟内中断
	_u32 _play_interrupt_3;		    //6分钟内中断
	_u32 _play_interrupt_4;		    //10分钟内中断
	_u32 _play_interrupt_5;		    //15分钟内中断
	_u32 _play_interrupt_6;		    //15分钟以上中断
	BOOL _is_ad_type;			//是否广告任务
}VOD_REPORT;

typedef struct _tag_vod_data_manager_list
{
	LIST   _data_list;  
	
}VOD_DATA_MANAGER_LIST;


typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_int32  _task_id;
       _int32  _file_index;
	_u64   _start_pos;
	_u64   _length;
	char*  _buffer;
	_int32 _blocktime;
}VOD_DATA_MANAGER_READ;

typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_int32  _task_id;
       _int32  _file_index;
}VOD_DATA_MANAGER_STOP;

typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_int32  _task_id;
       _int32  _buffer_time;
}VOD_DATA_MANAGER_SET_BUFFER_TIME;

typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_int32  _task_id;
       _int32  _percent;
}VOD_DATA_MANAGER_GET_BUFFER_PERCENT;

typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_int32  _task_id;
    _u64  _position;
}VOD_DATA_MANAGER_GET_DOWNLOAD_POSITION;

typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32 _result;
}VOD_DATA_MANAGER_FREE_VOD_BUFFER;

typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_int32 _buffer_size;
}VOD_DATA_MANAGER_GET_SET_VOD_BUFFER_SIZE;

typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_int32 _allocated;
}VOD_DATA_MANAGER_IS_VOD_BUFFER_ALLOCATED;

typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_int32  _task_id;
	BOOL  _finished;
}VOD_DATA_MANAGER_IS_DOWNLOAD_FINISHED;

typedef struct
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_int32 _task_id;
	_u32 _file_index;
	_u32 _bitrate;
}VOD_DATA_MANAGER_GET_BITRATE;

typedef struct _tag_vod_dl_pos
{
       _u32 _task_id;
	_u64  _start_pos;
	_u64  _file_size;
	RANGE_LIST * _range_total_recved;
}VOD_DL_POS;
#define VDM_MAX_DL_POS_NUM 2
#define MIN_VDM_INFO  10
#define VDM_TIMER      100
#define VDM_TIMER_INTERVAL 30   /* 300 -> 30  不要等太久,以免导致界面卡死  modified by zyq @ 20120712  */
#define FORCE_SET_CDN_MODE_COUNT (10*1000/VDM_TIMER_INTERVAL)  //10s/300ms->33
#define VDM_MIN_WINDOW_TIME_LEN  10
#define VDM_EM_WINDOW_TIME_LEN  35
#define VDM_DRAG_WINDOW_TIME_LEN  10		//按需求设置
#define VDM_BUFFERING_WINDOW_TIME_LEN  60
#define VDM_MIN_HD_FILE_SIZE  (200*1024*1024)   //200MB
#define VDM_MAX_BYTES_RATE  (256*1024)
#define VDM_DEFAULT_BYTES_RATE  (64*1024)
#define VDM_DEFAULT_FETCH_TIMEOUT  30

/* platform of vod data manager module, must  be invoke in the initial state of the download program*/
_int32  init_vod_data_manager_module(void);
_int32  uninit_vod_data_manager_module(void);

/* platform of vod data manager module*/
_int32 get_vod_data_manager_cfg_parameter(void);
_int32  vdm_create_slabs_and_init_data_buffer(void);
_int32  vdm_destroy_slabs_and_unit_data_buffer(void);


/* platform of vod data manager struct*/
_int32  vdm_init_data_manager(VOD_DATA_MANAGER* p_vod_data_manager_interface, struct _tag_task* p_task, _u32 file_index);
_int32  vdm_uninit_data_manager(VOD_DATA_MANAGER* p_vod_data_manager_interface, BOOL keep_data);

_int32 vdm_get_the_other_vod_task(VOD_DATA_MANAGER_LIST* vod_data_manager_list, _u32 cur_task_id,_u32* p_task_id);

_int32 vdm_vod_async_read_file(_int32 task_id, _u32 file_index,  _u64 start_pos, _u64 len, char *buf, _int32 block_time, vdm_recv_handler callback_handler, void* user_data);
_int32 vdm_dm_notify_flush_finished(_u32 task_id);

/*api */
_int64 vdm_vod_read_file (_int32 task_id, _u64 start_pos, _u64 len, char *buf, _int32 block_time);
_int64 vdm_vod_bt_read_file (_int32 task_id, _int32 file_index, _u64 start_pos, _u64 len, char *buf, _int32 block_time);
_int32 vdm_vod_set_index_data (_int32 task_id, _u64 start_pos, _u64 len);

_int32 vdm_set_data_change_notify(void *param);

_int32 vdm_get_free_vod_data_manager(struct _tag_task* ptr_task, _u32 file_index, VOD_DATA_MANAGER **pp_vdm);

_int32 vdm_vod_read_file_handle( void * _param );

_int32 vdm_stop_vod_handle( void * _param );

_int32 vdm_vod_stop_task(_int32 task_id, BOOL keep_data);

#ifdef ENABLE_VOD	
_int32 vdm_set_vod_buffer_time_handle( void * _param );
#endif

_int32 vdm_get_vod_buffer_percent_handle( void * _param );

_int32 vdm_get_vod_download_position_handle(void * _param);

_int32     vdm_dm_notify_buffer_recved(TASK* task_ptr, RANGE_DATA_BUFFER* range_data );

#ifdef ENABLE_VOD	
_int32 vdm_free_vod_buffer_handle( void * _param );
#endif

_int32 vdm_get_vod_buffer_size_handle( void * _param );

_int32 vdm_set_vod_buffer_size_handle( void * _param );

_int32 vdm_is_vod_buffer_allocated_handle( void * _param );

_int32 vdm_get_vod_bitrate_handle(void * _param);

_int32 vdm_vod_report_handle( void * _param );

_int32 vdm_vod_is_download_finished_handle( void * _param );

_int32 vdm_vod_free_vod_buffer(void);

_int32 vdm_vod_malloc_vod_buffer(void);

_int32 vdm_dm_get_data_buffer_by_range(TASK* task_ptr, RANGE* r,  RANGE_DATA_BUFFER_LIST* ret_range_buffer);

_int32 vdm_dm_notify_check_error_by_range(TASK* task_ptr, RANGE* r);

_int32 vdm_enable_vod (BOOL b);

_int32 vdm_init_dlpos(_u32 task_id,_u64  start_pos,_u64  file_size,RANGE_LIST * range_total_recved);
_int32 vdm_get_dlpos(_u32 task_id,_u64 *  dl_pos);
_int32 vdm_update_dlpos(_u32 task_id,_u64  start_pos);
_int32 vdm_uninit_dlpos(_u32 task_id);

_int32 vdm_vod_get_download_position(_int32 task_id, _u64* position);

_int32 vdm_get_vod_task_num (void);
/* 同一时间允许一个点播任务存在! */
_u32 vdm_get_current_vod_task_id(void);

_int32     vdm_get_flv_header_to_first_tag(_u32 task_id,_u32 file_index, char ** pp_buffer,_u64 * data_len);

_int32 vdm_process_index_parser(VOD_DATA_MANAGER* ptr_vod_data_manager, _u64 file_size, char* file_name);

_int32 vdm_drop_buffer_by_range(VOD_DATA_MANAGER* ptr_vod_data_manager, RANGE* r);
#ifdef __cplusplus
}
#endif

#endif /* !defined(__VOD_DATA_MANAGER_INTERFACE_20090201)*/

