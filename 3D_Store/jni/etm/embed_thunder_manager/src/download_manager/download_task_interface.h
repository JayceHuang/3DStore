#if !defined(__DOWNLOAD_TASK_H_20080918)
#define __DOWNLOAD_TASK_H_20080918

#ifdef __cplusplus
extern "C" 
{
#endif

#include "em_common/em_define.h"
#include "em_common/em_errcode.h"
#include "em_asyn_frame/em_asyn_frame.h"

#include "em_common/em_list.h"
#include "em_common/em_map.h"
#include "settings/em_settings.h"
//#include "connect_manager/resource.h"
//#include "data_manager/data_manager_interface.h"
//#include "asyn_frame/msg.h"
//#include "asyn_frame/notice.h"


/************************************************************************/
/*                    STRUCT DEFINE                                     */
/************************************************************************/

typedef enum t_task_state
{
	TS_TASK_WAITING=0, 
	TS_TASK_RUNNING, 
	TS_TASK_PAUSED,
	TS_TASK_SUCCESS, 
	TS_TASK_FAILED, 
	TS_TASK_DELETED 
} TASK_STATE;

typedef enum t_task_type
{
	TT_URL = 0, 
	TT_BT, 
	TT_TCID,  
	TT_KANKAN, 
	TT_EMULE, 
	TT_FILE ,
	TT_LAN 
} EM_TASK_TYPE;

typedef enum t_em_encoding_mode
{ 
	EM_ENCODING_NONE = 0, /* 返回原始字段(根据et_release_torrent_seed_info的_encoding字段决定编码格式)  */
	EM_ENCODING_GBK = 1,/*  返回GBK格式编码 */
	EM_ENCODING_UTF8 = 2,/* 返回UTF-8格式编码 */
	EM_ENCODING_BIG5 = 3,/* 返回BIG5格式编码  */
	EM_ENCODING_UTF8_PROTO_MODE = 4,/* 返回种子文件中的utf-8字段  */
	EM_ENCODING_DEFAULT = 5/* 未设置输出格式(使用et_set_seed_switch_type的全局输出设置)  */
}EM_ENCODING_MODE ;

/* Structures for running task status */
typedef struct t_runing_status
{
	EM_TASK_TYPE _type;   
	TASK_STATE  _state;   
	_u64 _file_size;  /*任务文件大小*/  
	_u64 _downloaded_data_size; 		 /*已下载的数据大小*/  
	_u32 _dl_speed;  					/* 实时下载速率*/  
	_u32 _ul_speed;  					/* 实时上传速率*/  
	_u32 _downloading_pipe_num; 
	_u32 _connecting_pipe_num; 
}RUNNING_STATUS;

/* Structures for  task info */
typedef struct t_em_task_info
{
	_u32  _task_id;		
	TASK_STATE  _state;   
	EM_TASK_TYPE  _type;   

	char _file_name[MAX_FILE_NAME_BUFFER_LEN];
	char _file_path[MAX_FILE_PATH_LEN]; 
	_u64 _file_size;  /*任务文件大小*/  
	_u64 _downloaded_data_size; 		 /*已下载的数据大小*/  
	 
	_u32 _start_time;	
	_u32 _finished_time;

	_u32  _failed_code;
	//_u32  _inner_id;		//当任务正在下载库中运行时，这个_inner_id为下载库的task_id
	_u32  _bt_total_file_num; // just for bt task
	BOOL _is_deleted;	//当任务被放入deleted_tasks子树中时,它的_state不会为ETS_TASK_DELETED，还是保持原来的值以便它可以被正确还原，用这个标示任务此时状态为ETS_TASK_DELETED
	BOOL _check_data;	//有无必要?
	BOOL _is_no_disk;					/* 是否为无盘点播  */
}EM_TASK_INFO;
typedef enum t_em_hsc_stat
{
	EM_HSC_IDLE, EM_HSC_ENTERING, EM_HSC_SUCCESS, EM_HSC_FAILED, EM_HSC_STOP
} EM_HSC_STAT;
typedef struct t_em_high_speed_channel_info
{
	_u64				_product_id;
	_u64				_sub_id;
	_int32				_channel_type;// 通道类型: 0=商城免费3次 1=商城绿色通道 2=vip高速通道 -1=不可用
	EM_HSC_STAT		 	_stat;
	_u32 				_res_num;
	_u64 				_dl_bytes;
	_u32 				_speed;
	_int32		 		_errcode;
	BOOL 				_can_use;
	_u64 				_cur_cost_flow;
	_u64 				_remaining_flow;
	BOOL				_used_hsc_before;
} EM_HIGH_SPEED_CHANNEL_INFO;
#ifdef ENABLE_ETM_MEDIA_CENTER
typedef struct t_em_task_shop_info
{
	_u64 _groupid;
	_u8 _group_name[EM_MAX_GROUP_NAME_LEN];
	_u8 _group_type;
	_u64 _productid;
	_u64 _subid;	
	_u64 _created_file_size;
} EM_TASK_SHOP_INFO;
#endif
/* Structures for  task info */
typedef struct t_task_info
{	_u32  _task_id;	
	EM_TASK_TYPE  _type : 4; 
	TASK_STATE  _state : 4;   
	
	BOOL _is_deleted : 1;	
	BOOL _have_name	: 1;
	BOOL _check_data : 1;	
	BOOL _have_tcid	: 1;
	BOOL _have_ref_url	: 1;
	BOOL _have_user_data	: 1;
	BOOL _full_info	: 1;	//_task_info指向的是任务的全信息还是基本信息 
	BOOL _correct_file_name	: 1;  // 是否已经纠正了文件名
	
	_u8 _file_path_len;
	_u8 _file_name_len;
	_u16 _url_len_or_need_dl_num; 
	_u16 _ref_url_len_or_seed_path_len; 
	_u32  _user_data_len;
	_u8 _eigenvalue[CID_SIZE];

	_u64 _file_size;  /*任务文件大小*/  
	_u64 _downloaded_data_size; 		 /*已下载的数据大小*/  
	 
	_u32 _start_time;	
	_u32 _finished_time;

	_u32  _failed_code;

	_u32  _bt_total_file_num; // just for bt task
	_u32  _file_name_eigenvalue; 
	
#ifdef ENABLE_ETM_MEDIA_CENTER
	_u64 _groupid;
	_u8 _group_name[EM_MAX_GROUP_NAME_LEN];
	_u8 _group_type;
	_u64 _productid;
	_u64 _subid;
	_u64 _green_channel_dld;
	_u8 _green_channel_opened;
	_u64 _created_file_size;
#endif
	
}TASK_INFO; //68 bytes

#define EMEC_NO_RESOURCE	0x80000000

/* Structures for bt sub files */
typedef enum t_em_file_status
{
	EMFS_IDLE = 0, 
	EMFS_DOWNLOADING, 
	EMFS_SUCCESS, 
	EMFS_FAILED 
} EM_FILE_STATUS;

typedef struct  t_em_bt_file
{
	_u32 _file_index;
	BOOL _need_download;
	EM_FILE_STATUS _status;
	_u64 _file_size;	
	_u64 _downloaded_data_size; 		 /*已下载的数据大小*/  
	_u32 _failed_code;
}EM_BT_FILE;

#define  FS_IDLE				0
#define  FS_DOWNLOADING	1
#define  FS_SUCCESS			2
#define  FS_FAILED			3
typedef struct  t_bt_file
{
	_u16 _file_index;
	_u16 _status;
	_u64 _file_size;	
	_u64 _downloaded_data_size; 		 /*已写进磁盘的数据大小*/  
	_u32 _failed_code;
}BT_FILE;

#define  MAX_BT_RUNNING_SUB_FILE		3

typedef struct  t_bt_running_file
{
	_u16 _need_dl_file_num;
	_u16 _finished_file_num;
	_u16 * _need_dl_file_index_array;
	//_u32 *_sub_file_change_flag;
	BT_FILE _sub_files[MAX_BT_RUNNING_SUB_FILE];
}BT_RUNNING_FILE;

//vod download mode
typedef struct t_vod_download_mode
{
	BOOL _is_download;			//vod 下载模式
	_u32 _set_timestamp;		//vod 下载模式设置时间
	_u32 _file_retain_time;		//vod 下载文件保存时间，单位为秒
}VOD_DOWNLOAD_MODE;

#define CHANGE_STATE 						(0x00000001)
#define CHANGE_HAVE_NAME 					(0x00000002)
#define CHANGE_FILE_SIZE					(0x00000004)
#define CHANGE_DLED_SIZE 					(0x00000008)
#define CHANGE_START_TIME 					(0x00000010)
#define CHANGE_FINISHED_TIME 				(0x00000020)
#define CHANGE_FAILED_CODE 				(0x00000040)
#define CHANGE_DELETE 						(0x00000080)

#define CHANGE_BT_SUB_FILE_DLED_SIZE 		(0x00000100)
#define CHANGE_BT_SUB_FILE_STATUS 		(0x00000200)
#define CHANGE_BT_SUB_FILE_FAILED_CODE	(0x00000400)

#define  CHANGE_BT_NEED_DL_FILE			(0x00000800)

#define CHANGE_TOTAL_LEN 					(0x00001000)

/* Structures for task */
typedef struct t_task
{
	TASK_INFO * _task_info;// 指向基本任务信息

	_u32  _inner_id;
	_u32 _offset;		 //该任务存储在task_store.dat中的偏移,用于读取和改写该节点信息(sd_setfilepos)
	_u32 _change_flag;
	BOOL _waiting_stop;
	BOOL _vod_stop;
	BOOL _need_pause;
	BT_RUNNING_FILE * _bt_running_files; // just for bt running task while _full_info is FALSE
	VOD_DOWNLOAD_MODE  _download_mode;
	BOOL _is_ad;
#ifdef ENABLE_HSC
	EM_HIGH_SPEED_CHANNEL_INFO _hsc_info;
	BOOL _use_hsc;
#endif
}EM_TASK; //80 bytes

/* Structures for running task */
typedef struct t_running_task
{
	RUNNING_STATUS _status;
	_u32  _task_id;		
	_u32  _inner_id;		//下载库的task_id
	_u32  _dled_size_fresh_count;		
	EM_TASK * _task;    // 指向基本任务信息
}RUNNING_TASK;

/* Structures for URL task */
typedef struct t_url_task_info
{
	TASK_INFO _task_info;
	char *_file_path; 
	char * _file_name;

	char * _url;
	char * _ref_url;
	void * _user_data;
	_u8  _tcid[CID_SIZE];			
}EM_P2SP_TASK;

/* Structures for BT task */
typedef struct t_bt_task_info
{
	TASK_INFO _task_info;

	char *_file_path; 
	char * _file_name;

	char * _seed_file_path;
	void * _user_data;
	_u16* _dl_file_index_array;//array[_need_dl_num]  指向用于存储所有需要下载的子文件的文件序号
	BT_FILE * _file_array; //array[_need_dl_num]  指向用于存储所有需要下载的子文件的信息数组
}EM_BT_TASK;

typedef struct  t_em_file_cache
{
	_u32 _task_id;
	char * _str;
	//_u32 _time_stamp;
}EM_FILE_CACHE;

/* Structures for task manager*/
typedef struct t_download_task_manager
{
	LIST _full_task_infos;		//任务信息cache (full TASK_INFO)
	_u32 _max_cache_num;
	//char  _task_file[MAX_FILE_NAME_BUFFER_LEN];		//存储所有任务信息的文件名:task_store.dat
	//_u32  _task_file_size;
	//_u32  _task_count;
	
	LIST  _dling_task_order;			//用于记录任务下载优先级
	BOOL _dling_order_changed;		// 列表是否更改过	
	
	MAP _all_tasks;				//所有任务与任务id的对应关系,用于快速通过task_id找到对应的TASK  (key=task_id -> data=TASK)
	
	MAP _eigenvalue_url;				//任务去重用:所有ETT_URL 任务的url hash 值与任务id的对应关系, (key=url_hash -> data=task_id)
	MAP _eigenvalue_tcid;			//任务去重用:所有ETT_TCID 或ETT_LAN任务的tcid 值与任务id的对应关系, (key=tcid -> data=task_id)
	MAP _eigenvalue_gcid;			//任务去重用:所有ETT_KANKAN 任务的gcid 值与任务id的对应关系, (key=gcid -> data=task_id)
	MAP _eigenvalue_bt;				//任务去重用:所有ETT_BT 任务的info hash 值与任务id的对应关系, (key=info_hash -> data=task_id)
	MAP _eigenvalue_file;				//任务去重用:所有ETT_FILE 任务的 hash 值与任务id的对应关系, (key=info_hash -> data=task_id)
	MAP _file_name_eigenvalue;		//任务去重用:所有 任务的文件名特征 值与任务id的对应关系, (key=info_hash -> data=task_id)

	RUNNING_TASK _running_task[MAX_ALOW_TASKS];
	_u32  _running_task_num;
	_u32  _max_running_task_num;
	_u32 _total_task_num;		//这个主要用来累加生成新的task_id
	BOOL _running_task_loaded;		// 是否已经载入需要马上启动的任务
	BOOL _need_notify_state_changed;		// 是否需要通知界面
	BOOL _running_task_changed;		// 是否需要保存正在运行的任务
	//SEVENT_HANDLE * p_sevent_handle;
	LIST _file_path_cache;	//EM_FILE_CACHE
	LIST _file_name_cache;	//EM_FILE_CACHE

	///////// vod   ///////////////////////
	_u32  _vod_task_num;
	_u32  _vod_running_task_num;
	_u32  _used_vod_cache_size;  //KB
	_u32  _max_vod_cache_size;  //KB

	_u32 _current_vod_task_id;

	BOOL _is_reading_success_file;
	///////// lan   ///////////////////////	
	_u32 _lan_running_task_num;
	_u32 _lan_waiting_task_num;
}DOWNLOAD_TASK_MANAGER;


/* Structures for bt seed file */
#define MAX_TD_CFG_SUFFIX_LEN (8)

typedef struct t_emd_torrent_file_info
{
	_u32 _file_index;
	char *_file_name;
	_u32 _file_name_len;
	char *_file_path;
	_u32 _file_path_len;
	_u64 _file_offset;
	_u64 _file_size;
} EMD_TORRENT_FILE_INFO;

typedef struct t_emd_torrent_seed_info
{
	char _title_name[256-MAX_TD_CFG_SUFFIX_LEN];
	_u32 _title_name_len;
	_u64 _file_total_size;
	_u32 _file_num;
	_u32 _encoding;//种子文件编码格式，GBK = 0, UTF_8 = 1, BIG5 = 2
	_u8 _info_hash[20];
	EMD_TORRENT_FILE_INFO *file_info_array_ptr;
} EMD_TORRENT_SEED_INFO;

/* 创建下载任务时的输入参数 */
typedef struct t_em_create_task
{
	EM_TASK_TYPE _type;
	
	char* _file_path;
	_u32 _file_path_len;
	char* _file_name;
	_u32 _file_name_len;
	
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
	char *_gcid;
	
	BOOL _check_data;
	BOOL _manual_start;
	
	void * _user_data;
	_u32  _user_data_len;
} EM_CREATE_TASK;

/* 创建商城任务时的输入参数 */
typedef struct t_em_create_shop_task
{
	char* _url;
	_u32 _url_len;
	char* _ref_url;
	_u32 _ref_url_len;
	
	char* _file_path;
	_u32 _file_path_len;
	char* _file_name;
	_u32 _file_name_len;

	char *_tcid;
	_u64 _file_size;
	char *_gcid;

	_u64 _group_id;
	char *_group_name;
	_u32 _group_name_len;
	_u8 _group_type;
	_u64 _product_id;
	_u64 _sub_id;
} EM_CREATE_SHOP_TASK;

/* Structures for creating download task */
typedef struct t_create_task
{
	EM_TASK_TYPE _type;
	char* _file_path; 
	_u32 _file_path_len;
	char* _file_name;
	_u32 _file_name_len; 
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
	char *_gcid;
	BOOL _check_data;
	BOOL _manual_start;					
	void * _user_data;
	_u32  _user_data_len;
#ifdef ENABLE_ETM_MEDIA_CENTER
	_u64 _group_id;
	char *_group_name;
	_u32 _group_name_len;
	_u8 _group_type;
	_u64 _product_id;
	_u64 _sub_id;
#endif
} CREATE_TASK;
typedef struct t_em_server_res
{
	_u32 _file_index;
	_u32 _resource_priority;
	char* _url;							
	_u32 _url_len;					
	char* _ref_url;				
	_u32 _ref_url_len;		
	char* _cookie;				
	_u32 _cookie_len;		
}EM_SERVER_RES;
typedef struct t_em_peer_res
{
	_u32 _file_index;
	char _peer_id[PEER_ID_SIZE];							
	_u32 _peer_capability;					
	_u32 _res_level;
	_u32 _host_ip;
	_u16 _tcp_port;
	_u16 _udp_port;
}EM_PEER_RES;

typedef enum t_em_res_type
{
	ERT_SERVER = 0, 
	ERT_PEER
} EM_RES_TYPE;
typedef struct t_em_res 
{
	EM_RES_TYPE _type;
        union 
	{
                EM_SERVER_RES _s_res;
		  EM_PEER_RES _p_res;
        } I_RES;
#define s_res  I_RES._s_res
#define p_res  I_RES._p_res
}EM_RES;

typedef struct t_em_eigenvalue
{
	EM_TASK_TYPE _type;				/* 任务类型，必须准确填写*/
	
	char* _url;							/* _type== ETT_URL 或者_type== ETT_EMULE 时，这里就是任务的URL */
	_u32 _url_len;						/* URL 的长度，必须小于512 */
	char _eigenvalue[40];					/* _type== ETT_TCID或ETT_LAN 时,这里放CID,_type== ETT_KANKAN 时,这里放GCID, _type== ETT_BT 时,这里放info_hash   */
}EM_EIGENVALUE;


/* 高速通道相关信息 */
typedef enum em_hsc_state{HS_IDLE=0, HS_RUNNING, HS_FAILED}EM_HSC_STATE;
typedef struct t_em_hsc_info
{
	EM_HSC_STATE 		_state;
	_u32 					_res_num;	//资源个数
	_u64 				_dl_bytes;	//通过高速通道下载到的数据
	_u32 					_speed;		
	_int32	 			_errcode;
}EM_HSC_INFO;

typedef struct t_lixian_id
{
	_u32 _file_index; 	
	_u64 _lixian_id;	
}DT_LIXIAN_ID;

/*--------------------------------------------------------------------------*/
/*                Interfaces provid for external program				        */
/*--------------------------------------------------------------------------*/

_int32 init_download_manager_module(void);
_int32 uninit_download_manager_module(void);
_int32 dt_load_tasks(void);
_int32 dt_start_tasks(void);
_int32 dt_restart_tasks(void);
_int32 dt_clear(void);
_int32 dt_clear_running_tasks_before_restart_et(void);
_int32 dt_stop_all_waiting_tasks(void);
void dt_scheduler(void);
_int32 dt_create_task( void * p_param );
#ifdef ENABLE_ETM_MEDIA_CENTER
_int32 dt_create_shop_task( void * p_param );
#endif
_int32 dt_start_task( void * p_param );
_int32 dt_vod_start_task( void * p_param );
_int32 dt_run_task( void * p_param );
_int32 dt_stop_task( void * p_param );
_int32 dt_delete_task( void * p_param );
_int32 dt_recover_task( void * p_param );
_int32 dt_destroy_task( void * p_param );
_int32 dt_get_pri_id_list( void * p_param );
_int32 dt_pri_level_up( void * p_param );
_int32 dt_pri_level_down( void * p_param );
_int32 dt_rename_task( void * p_param );
_int32 dt_get_task_info(void * p_param);
_int32 dt_get_task_download_info(void * p_param);
_int32 dt_get_task_running_status(_u32 task_id,RUNNING_STATUS *p_status);
_int32 dt_set_task_url( void * p_param );
_int32 dt_get_task_url( void * p_param );
_int32 dt_get_task_ref_url( void * p_param );
_int32 dt_get_task_tcid_impl(EM_TASK * p_task, char * cid_buffer);
_int32 dt_get_task_tcid( void * p_param );
_int32 dt_get_bt_task_sub_file_tcid( void * p_param );
_int32 dt_get_task_gcid( void * p_param );
_int32 dt_get_bt_task_seed_file( void * p_param );
_int32 dt_get_bt_file_info( void * p_param );
_int32 dt_get_bt_task_sub_file_name( void * p_param );
_int32 dt_set_bt_need_download_file_index( void * p_param );
_int32 dt_get_bt_need_download_file_index( void * p_param );
_int32 dt_get_task_user_data( void * p_param );
_int32 dt_get_task_id_by_state( void * p_param );
_int32 dt_get_all_task_ids( void * p_param );
_int32 dt_get_all_task_total_file_size( void * p_param );
_int32 dt_get_all_task_need_space( void * p_param );
_int32 em_add_server_resource( void * p_param );
_int32 em_add_peer_resource( void * p_param );
_int32 em_get_peer_resource( void * p_param );
#ifdef ENABLE_LIXIAN
_int32 em_get_lixian_info( _u32 task_id,_u32 file_index, void * p_lx_info );
_int32 dt_set_lixian_task_id( void * p_param );
_int32 dt_get_lixian_task_id( void * p_param );
_int32 dt_set_task_lixian_mode( void * p_param );
_int32 dt_is_lixian_task( void * p_param );
#endif /* ENABLE_LIXIAN */
_int32 dt_get_task_id_by_eigenvalue_impl( EM_EIGENVALUE * p_eigenvalue ,_u32 * task_id);
_int32 dt_get_task_id_by_eigenvalue( void * p_param );

////ad////
_int32 dt_set_task_ad_mode( void * p_param );
_int32 dt_is_ad_task( void * p_param );

_u32 dt_get_task_inner_id( _u32 task_id );
#ifdef ENABLE_HSC
_int32 dt_get_task_hsc_info( _u32 task_id, EM_HIGH_SPEED_CHANNEL_INFO *p_hsc_info );
#endif /* ENABLE_HSC */
#ifdef ENABLE_ETM_MEDIA_CENTER
_int32 dt_get_task_shop_info( _u32 task_id, EM_TASK_SHOP_INFO *p_shop_info );
#endif
_int32 dt_set_max_running_tasks(_u32 max_tasks);
BOOL dt_is_task_locked(void);
_int32 dt_get_current_download_speed( void * p_param );
_int32 dt_get_current_upload_speed( void * p_param );



_int32 dt_delete_urgent_task_param( void );
_int32 dt_create_urgent_task_impl( void );
_int32 dt_create_urgent_task( CREATE_TASK * p_create_param,_u32* p_task_id,BOOL is_remote );
_int32 dt_create_task_impl( CREATE_TASK * p_create_param,_u32* p_task_id,BOOL is_vod_task ,BOOL is_remote,BOOL create_by_cfg);

BOOL dt_have_running_task(void );

_int32 dt_force_scheduler(void);
_int32 dt_post_next(void);
_int32 dt_do_next(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid);
////////////////////  vod ///////////////////////////////
_int32 dt_create_vod_task( void * p_param );
_int32 dt_stop_vod_task( void * p_param );
_int32 dt_vod_read_file( void * p_param );
_int32 dt_force_start_task(EM_TASK * p_task);
_int32 dt_vod_read_bt_file( void * p_param );
_int32 dt_vod_get_buffer_percent( void * p_param );
_int32 dt_vod_get_download_position(void * p_param);
_int32 dt_vod_set_download_mode(void * p_param);
_int32 dt_vod_get_download_mode(void * p_param);
_int32 dt_vod_is_download_finished( void * p_param );
_int32 dt_vod_get_bitrate( void * p_param );
_int32 dt_vod_report(void * p_param);
_int32 dt_vod_final_server_host(void * p_param);
_int32 dt_vod_ui_notify_start_play(void * p_param);
_int32 dt_set_vod_cache_path( void * p_param );
_int32 dt_get_vod_cache_path( void * p_param );
_int32 dt_set_vod_cache_size( void * p_param );
_int32  dt_get_vod_cache_size( void * p_param );
_int32  dt_clear_vod_cache( void * p_param );
_int32 dt_set_vod_buffer_time( void * p_param );
_int32 dt_set_vod_buffer_size( void * p_param );
_int32 dt_get_vod_buffer_size( void * p_param );
_int32 dt_is_vod_buffer_allocated( void * p_param );
_int32  dt_free_vod_buffer( void * p_param );

#if defined(ENABLE_NULL_PATH)
_int32 dt_et_get_task_info(void * p_param);
#endif
#ifdef ENABLE_HSC
_int32 dt_get_hsc_info( _u32 task_id, _u32 file_index,EM_HSC_INFO * p_hsc_info );
_int32 dt_is_hsc_usable( _u32 task_id, _u32 file_index,BOOL * p_usable);
_int32 dt_open_high_speed_channel( void * p_param );
#endif /* ENABLE_HSC */


typedef _int32 ( *DT_NOTIFY_FILE_NAME_CHANGED)(_u32 task_id, const char* file_name,_u32 length);
_int32 dt_set_file_name_changed_callback( void * p_param );
_int32 dt_notify_file_name_changed( _u32 task_id, const char* file_name,_u32 length );

_int32 dt_create_task_by_cfg_file(void * p_param);

_int32 dt_use_new_url_create_task(void * p_param);

#ifdef __cplusplus
}
#endif


#endif // !defined(__DOWNLOAD_TASK_H_20080918)
