#if !defined(__P2SP_TASK_H_20090224)
#define __P2SP_TASK_H_20090224

#ifdef __cplusplus
extern "C" 
{
#endif

#include "asyn_frame/notice.h"
#include "download_task/download_task.h"
#include "p2sp_download/p2sp_query.h"
#include "data_manager/data_manager_interface.h"

#define EC_NO_RESOURCE 					0x80000000   //没有资源
#define EC_P2P_HTTP_ENCAP				0x10000000	 //迅雷p2p协议需要http封装
#define EC_QUERY_SHUB_OK 				0x08000000   //查询SHUB成功
#define EC_SHUB_HAVE_RECORD 			0x02000000   //SHUB上有该文件的记录:GCID,BCID,CID...
#define EC_SHUB_HAVE_SER_RES			0x04000000   //SHUB上有该文件的其他候选server资源
#define EC_SHUB_HAVE_MOBILE_CDN			0x06000000   //SHUB上有该文件的server类型cdn
#define EC_QUERY_PHUB_OK 				0x01000000   //查询PHUB成功
#define EC_PHUB_HAVE_PEER 				0x00800000   //PHUB上有该文件的其他候选peer资源(纯上传)
#define EC_QUERY_TRACKER_OK 			0x00400000   //查询Tracker成功
#define EC_TRACKER_HAVE_PEER 			0x00200000   //PHUB上有该文件的其他候选peer资源(边下边传)
#define EC_QUERY_NORMAL_CDN_OK			0x20000000	 //查询normal cdn manager成功
#define EC_HAVE_HTTP_PEER				0x00100000   //有支持HTTP封装的Peer资源
#define EC_NT_UNKNOWN					0x00000000	 //未知网络类型
#define EC_NT_CHINA_MOBILE				0x00010000	 //中国移动
#define EC_NT_CHINA_UNICOM				0x00020000	 //中国联通
#define EC_NT_CHINA_TELECOM				0x00030000	 //中国电信
#define EC_NT_WAP						0x00040000	 // wap网络
#define EC_NT_NET						0x00080000	 // net网络
#define EC_NT_3G						0x000C0000	 // 3g网络

#define EC_NT_WLAN						EC_NT_UNKNOWN  // 普通宽带网络,LAN或WIFI

#define EC_NT_CMWAP 		(EC_NT_WAP|EC_NT_CHINA_MOBILE)
#define EC_NT_CMNET 		(EC_NT_NET|EC_NT_CHINA_MOBILE)

#define EC_NT_CUIWAP 		(EC_NT_WAP|EC_NT_CHINA_UNICOM)
#define EC_NT_CUINET 		(EC_NT_NET|EC_NT_CHINA_UNICOM)

enum TASK_CREATE_TYPE {TASK_CREATED_BY_URL= 0, TASK_CONTINUE_BY_URL,  TASK_CREATED_BY_TCID, TASK_CONTINUE_BY_TCID, TASK_CREATED_BY_GCID};

typedef struct _tag_relation_file_info
{
	_u8   _cid[CID_SIZE];
	_u8   _gcid[CID_SIZE];
	_u64 _file_size;
	_u32  _block_info_num;
	RELATION_BLOCK_INFO* _p_block_info_arr;
}P2SP_RELATION_FILEINFO;

typedef enum t_task_dispatch_mode
{
    TASK_DISPATCH_MODE_DEFAULT = 0,                           //采用默认的方式进行下载调度
    TASK_DISPATCH_MODE_LITTLE_FILE = 1,           //小文件调度
} TASK_DISPATCH_MODE;

typedef struct  _tag_p2sp_task
{
	TASK _task;
	DATA_MANAGER _data_manager;  
	RES_QUERY_PARA _res_query_para;

	_u32  _query_shub_timer_id;
	_u32  _query_peer_timer_id;
	//_u32  _query_tracker_timer_id;
	//_u32  _res_query_timer_id;
	_u32 _last_query_stamp; /* for res_query_tracker */
	
	_int32 _query_shub_times_sn;
	_int32 _query_shub_newres_times_sn;
	_int32 _query_relation_shub_times_sn;
	_int32 _query_shub_retry_count;

	enum TASK_CREATE_TYPE _task_created_type;

	//enum TASK_STATE task_state;
	enum TASK_RES_QUERY_STATE _res_query_shub_state;
	enum TASK_RES_QUERY_STATE _res_query_phub_state;
	enum TASK_RES_QUERY_STATE _res_query_tracker_state;
	enum TASK_RES_QUERY_SHUB_STEP _res_query_shub_step;
    enum TASK_RES_QUERY_STATE _res_query_dphub_state;
	
	_int32 _shub_gcid_level; 
	_int32 _new_gcid_level; 

	BOOL   _is_cid_ok;
	BOOL   _is_gcid_ok;
	BOOL   _is_bcid_ok;
#ifdef UPLOAD_ENABLE
	BOOL   _is_add_rc_ok;  // add record to upload manager
#endif	
	/* Others need by reporter  */
	_int32 _origin_url_ip;  /* Download ip of REPORTDWSTAT */
	_int32 _download_status;  /* Download Status of INSERTSRES */
	_int32 _insert_course;  /* Insert Course of INSERTSRES */
	BOOL _is_shub_return_file_size;  /*  服务器是否返回文件大小   */
	BOOL _is_shub_ok;
	BOOL _is_phub_ok;
	BOOL _is_tracker_ok;
#ifdef ENABLE_CDN
	enum TASK_RES_QUERY_STATE _res_query_cdn_state;
	enum TASK_RES_QUERY_STATE _res_query_normal_cdn_state;
	_u32 _query_normal_cdn_timer_id;
#ifdef ENABLE_KANKAN_CDN
	enum TASK_RES_QUERY_STATE _res_query_kankan_cdn_state;
#endif
#ifdef ENABLE_HSC
	enum TASK_RES_QUERY_STATE _res_query_vip_state;
	BOOL   _is_vip_ok;
#endif /* ENABLE_HSC */
	//_u32  _query_cdn_timer_id;
#endif /* ENABLE_CDN */
	enum TASK_RES_QUERY_STATE _res_query_partner_cdn_state;
	//_u32  _query_partner_cdn_timer_id;
	
	BOOL _is_vod_task;
	BOOL _get_file_size_after_data;
	_u32 _detail_err_code;
	BOOL _is_lan;				/* IPAD KANKAN 下局域网任务的特殊处理,在长时间无下载速度时把任务置为失败状态 */
	BOOL _is_unicom;			/* 联通任务的特殊处理,在长时间无下载速度时把任务置为失败状态 */
	_u32 _nospeed_timestmp;
	
	_u32 _relation_file_num;
	P2SP_RELATION_FILEINFO* _relation_fileinfo_arr[MAX_RELATION_FILE_NUM];
	BOOL _relation_query_finished;

    DPHUB_QUERY_CONTEXT     _dpub_query_context;
    char _origion_url[MAX_URL_LEN];
    _u32 _origion_url_length;

    _u64 _filesize_to_little_file;
    TASK_DISPATCH_MODE _connect_dispatch_mode;
}P2SP_TASK;
	

/*--------------------------------------------------------------------------*/
/*                Structures for posting functions				        */
/*--------------------------------------------------------------------------*/


typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	char* url;
	_u32 url_length;
	char* ref_url;
	_u32 ref_url_length;
	char* description;
	_u32 description_len;
	char* file_path;
	_u32 file_path_len;
	char* file_name;
	_u32 file_name_length;
	_u32* task_id;
	char* origion_url;
	_u32  origion_url_length;
}TM_NEW_TASK_URL;

typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	char* url;
	_u32 url_length;
	char* ref_url;
	_u32 ref_url_length;
	char* description;
	_u32 description_len;
	char* file_path;
	_u32 file_path_len;
	char* file_name;
	_u32 file_name_length;
	_u32* task_id;
	char* origion_url;
	_u32  origion_url_length;
}TM_CON_TASK_URL;

typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_u8 *tcid;
	_u64 file_size;
	char* file_path;
	_u32 file_path_len;
	char* file_name;
	_u32 file_name_length;
	_u32* task_id;
}TM_NEW_TASK_CID;

typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_u8 *tcid;
	char* file_path;
	_u32 file_path_len;
	char* file_name;
	_u32 file_name_length;
	_u32* task_id;
}TM_CON_TASK_CID;
typedef struct   
{
	SEVENT_HANDLE _handle;
	_int32 _result;
	_u8 *tcid;
	_u64 file_size;
	_u8 *gcid;
	char* file_path;
	_u32 file_path_len;
	char* file_name;
	_u32 file_name_length;
	_u32* task_id;
}TM_NEW_TASK_GCID;


/*--------------------------------------------------------------------------*/
/*                Interfaces provid for task_manager				        */
/*--------------------------------------------------------------------------*/
_int32 init_p2sp_task_module(void);
_int32 uninit_p2sp_task_module(void);
_int32 pt_init_task(P2SP_TASK * p_task);
void pt_uninit_task(P2SP_TASK * p_task);
void pt_create_task_err_handler(P2SP_TASK * p_task);
_int32 pt_check_if_para_vaild(enum TASK_CREATE_TYPE * p_type,const char * file_path,_u32 file_path_len, const char * file_name,_u32 file_name_len, 
	const char * url,_u32 url_len,const char * ref_url,_u32 ref_url_len,_u8 * cid,_u64 file_size,_u8 * gcid);
_int32 pt_check_if_old_file_exist(const char * file_path,_u32 file_path_len, const char * file_name,_u32 file_name_len);
_int32 pt_create_new_task_by_url( TM_NEW_TASK_URL* p_param,TASK ** pp_task );
_int32 pt_create_continue_task_by_url( TM_CON_TASK_URL* p_param,TASK ** pp_task );
	//新建任务-用tcid下
_int32 pt_create_new_task_by_tcid( TM_NEW_TASK_CID* p_param,TASK ** pp_task );
	//断点续传-用tcid下
_int32 pt_create_continue_task_by_tcid(TM_CON_TASK_CID* p_param, TASK **pp_task );

_int32 pt_create_task_by_tcid_file_size_gcid( TM_NEW_TASK_GCID* p_param,TASK ** pp_task );

_int32 pt_create_task(TASK ** pp_task,enum TASK_CREATE_TYPE  type,const char * file_path,_u32 file_path_len, const char * file_name,_u32 file_name_len, 
	const char * url,_u32 url_len,const char * ref_url,_u32 ref_url_len,const char * description,_u32 description_len,_u8 * cid,_u64 file_size,_u8 * gcid, const char* origion_url, _u32 origion_url_len);
_int32 pt_start_task(TASK * p_task);
_int32 pt_stop_task(TASK * p_task);
_int32 pt_delete_task(TASK * p_task);
BOOL pt_is_task_exist_by_gcid(TASK* p_task, const _u8* gcid, BOOL include_no_disk);
BOOL pt_is_task_exist_by_url(TASK* p_task, const char* url, BOOL include_no_disk);
BOOL pt_is_task_exist_by_cid(TASK* p_task, const _u8* cid, BOOL include_no_disk);
_int32 pt_get_task_file_name(TASK* p_task, char *file_name_buffer, _int32* buffer_size);
_int32 pt_get_task_tcid(TASK* p_task, _u8 * tcid);
_int32 pt_get_task_gcid(TASK* p_task, _u8 *gcid);
_int32 pt_remove_task_tmp_file( TASK * p_task );
/* Update the task information */
_int32 pt_update_task_info(TASK * p_task);

_int32 pt_get_detail_err_code( TASK * p_task);


BOOL pt_is_shub_ok(TASK* p_task);

_int32 pt_get_file_size_after_data(TASK * p_task);
/*--------------------------------------------------------------------------*/
/*                Interfaces provid for data_manager				        */
/*--------------------------------------------------------------------------*/

/* file creating result notify,this function shuld be called by data_manager */
_int32 pt_notify_file_creating_result_event(TASK* p_task,BOOL result);

/* file closing result notify,this function shuld be called by data_manager when deleting a task */
_int32 pt_notify_file_closing_result_event(TASK* p_task,BOOL result);

/* Get the excellent file name from connect maneger,this function shuld be called by data_manager */
BOOL pt_get_excellent_filename(TASK* p_task,char *p_str_name_buffer, _u32 buffer_len );

/* Set the origin download mode to connect maneger,this function shuld be called by data_manager */
_int32 pt_set_origin_download_mode(TASK* p_task, RESOURCE *p_orgin_res );
_int32 pt_set_origin_mode(TASK* p_task, BOOL origin_mode );
BOOL pt_is_origin_mode(TASK* p_task );


/*--------------------------------------------------------------------------*/
/*                Internal functions				        */
/*--------------------------------------------------------------------------*/
#ifdef UPLOAD_ENABLE
_int32 pt_add_record_to_upload_manager( TASK * p_task );
#endif

#ifdef ENABLE_CDN
_int32 pt_set_cdn_mode( TASK * p_task,BOOL mode);
_u32 pt_get_cdn_speed( TASK * p_task );
#ifdef ENABLE_HSC
_u32 pt_get_vip_cdn_speed( TASK * p_task );
#endif /* ENABLE_HSC */
#endif /* ENABLE_CDN */

#ifdef ENABLE_LIXIAN
_u32 pt_get_lixian_speed(TASK* p_task);
#endif

_int32  pt_dm_notify_buffer_recved(TASK* task_ptr, RANGE_DATA_BUFFER*  buffer_node );

_int32 pt_dm_get_data_buffer_by_range(TASK* task_ptr, RANGE* r,  RANGE_DATA_BUFFER_LIST* ret_range_buffer);

_int32 pt_dm_notify_check_error_by_range(TASK* task_ptr, RANGE* r);

_int32 pt_set_task_dispatch_mode(TASK* p_task, TASK_DISPATCH_MODE mode, _u64 filesize_to_little_file );

_int32 pt_get_download_bytes(TASK* p_task, char* host, _u64* download);

#ifdef __cplusplus
}
#endif


#endif // !defined(__P2SP_TASK_H_20090224)
