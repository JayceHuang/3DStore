#if !defined(__VOD_TASK_H_20090921)
#define __VOD_TASK_H_20090921

#ifdef __cplusplus
extern "C" 
{
#endif

#include "em_common/em_define.h"
#include "download_manager/download_task_interface.h"


/* Structures for creating vod task */
typedef struct t_em_create_vod_task
{
	EM_TASK_TYPE _type;
	char* _url;
	_u32 _url_len;
	char* _ref_url;
	_u32 _ref_url_len;
	char* _seed_file_full_path; 
	_u32 _seed_file_full_path_len; 
	_u32* _download_file_index_array; 
	_u32 _file_num;
	char *_tcid;
	_u64 _file_size;
	_u64 * _segment_file_size_array;		//在Kan Kan 任务中这里可以是多个子文件(一部电影有可能被分成n个文件) file_size 的数组
	char *_gcid;
	BOOL _check_data;
	BOOL _is_no_disk;	
	void * _user_data;					/* 用于存放与该任务相关的用户附加信息，如任务描述，海报，cookie等 。如果是cookie，coockie部分必须以"Cookie:"开头，并以"\r\n" 结束 */
	_u32  _user_data_len;					/* 用户数据的长度,必须小于65535 */
	char* _file_name;
	_u32  _file_name_len;
} EM_CREATE_VOD_TASK;

#if 1
typedef struct t_vod_task
{
	_u32  _task_id;		
	//TASK_STATE  _state;   
	EM_TASK_TYPE _type;
	//char* _url_or_seed_path;
	//_u32 _url_len;
	//char* _ref_url;
	//_u32 _ref_url_len;
	//char* _seed_file_full_path; 
	//_u32 _seed_file_full_path_len; 
	//_u32* _download_file_index_array; 
	_u32 _file_num;
	_u8 *_tcid;
	_u8 *_gcid;
	_u64 _file_size;
	_u64 * _segment_file_size_array;	
	_u32 _pre_id;
	_u32 _inner_id;
	_u32 _nxt_id;
	_u32 _pre_file_index;
	_u32 _current_file_index;
	_u32 _nxt_file_index;
	BOOL _check_data;
	BOOL _is_multi_file;
	BOOL _is_no_disk;	
	_u8  _eigenvalue[CID_SIZE];
} VOD_TASK;

/* Structures for vod manager*/
typedef struct t_vod_manager
{
	MAP _all_tasks;				//所有任务与任务id的对应关系,用于快速通过task_id找到对应的TASK  (key=task_id -> data=TASK)
	
	_u32 _total_task_num;		//这个主要用来累加生成新的task_id
	_u32 _max_cache_size;
	char _cache_path[MAX_FILE_PATH_LEN];
}VOD_MANAGER;


/*--------------------------------------------------------------------------*/
/*                Interfaces  				        */
/*--------------------------------------------------------------------------*/
_int32 init_vod_module(void);
_int32 uninit_vod_module(void);
_int32 vod_task_malloc(VOD_TASK **pp_slip);
_int32 vod_task_free(VOD_TASK * p_slip);
_int32 vod_id_comp(void *E1, void *E2);
_u32 vod_get_task_num( void );
VOD_TASK * vod_get_task_from_map(_u32 task_id);
_int32 vod_add_task_to_map(VOD_TASK * p_task);
_int32 vod_remove_task_from_map(VOD_TASK * p_task);
_int32 vod_clear_tasks(void);
_int32 vod_create_task( void * p_param );
_int32 vod_stop_task( void * p_param );
_int32 vod_read_file( void * p_param );
_int32 vod_read_bt_file( void * p_param );
_int32 vod_get_buffer_percent( void * p_param );
_int32 vod_is_download_finished( void * p_param );
_int32 vod_get_bitrate( void * p_param );
_int32 vod_get_download_position( void * p_param );
_int32 vod_report( void * p_param );
_int32 vod_final_server_host( void * p_param );
_int32 vod_ui_notify_start_play( void * p_param );
_int32 vod_get_task_info(void * p_param);
_int32 vod_get_task_running_status(_u32 task_id,RUNNING_STATUS *p_status);
_int32 vod_get_bt_file_info( void * p_param );

_int32 vod_get_all_bt_file_index(char  * seed_file_full_path, _u32 ** pp_need_dl_file_index_array,_u32 * p_need_dl_file_num);
_int32 vod_create_bt_task( EM_CREATE_VOD_TASK * p_create_param ,_u32 * inner_id);
_int32 vod_create_p2sp_task( EM_CREATE_VOD_TASK * p_create_param ,_u32 * inner_id);
_int32 vod_generate_eigenvalue(EM_EIGENVALUE * p_eigenvalue,_u8 * eigenvalue);
_int32 vod_get_task_id_by_eigenvalue( EM_EIGENVALUE * p_eigenvalue ,_u32 * task_id);
_int32 vod_report_impl(_u32 task_id);

#if 0
/////2.8 设置有盘点播的默认临时文件缓存路径,必须已存在且可写,path_len 必须小于ETM_MAX_FILE_PATH_LEN
_int32 vod_set_cache_path( void * p_param );
_int32 vod_get_cache_path( void * p_param );

/////2.9 设置有盘点播的缓存区大小，单位kB，即接口etm_set_vod_cache_path 所设进来的目录的最大可写空间，建议3GB 以上
_int32 vod_set_cache_size( void * p_param );
_int32  vod_get_cache_size( void * p_param );

/////2.10 设置点播的专用内存大小,单位kB，建议2MB 以上
_int32 vod_set_buffer_size( void * p_param );
_int32 vod_get_buffer_size( void * p_param );

/////2.11 查询vod 专用内存是否已经分配
_int32 vod_is_buffer_allocated( void * p_param );

/////2.12 手工释放vod 专用内存,ETM 本身在反初始化时会自动释放这块内存，但如果界面程序想尽早腾出这块内存的话，可调用此接口，注意调用之前要确认无点播任务在运行
_int32  vod_free_buffer( void * p_param );
#endif
#endif

#ifdef __cplusplus
}
#endif


#endif // !defined(__VOD_TASK_H_20090921)
