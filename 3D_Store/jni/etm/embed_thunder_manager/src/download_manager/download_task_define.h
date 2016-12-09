#ifndef DOWNLOAD_TASK_DEFINE_H
#define DOWNLOAD_TASK_DEFINE_H

#ifdef __cplusplus
extern "C" 
{
#endif

#include "em_common/em_define.h"
#include "em_common/em_errcode.h"
#include "utility/url.h"
#include "utility/list.h"
#include "utility/map.h"
#include "em_common/em_utility.h"
#include "em_asyn_frame/em_asyn_frame.h"

#include "settings/em_settings.h"


/************************************************************************/
/*                    STRUCT DEFINE                                     */
/************************************************************************/
#define ETM_MAX_TASK_TAG_LEN (512)		/* �ļ�����󳤶� */

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
	TT_LAN ,
	TT_BT_MAGNET
} EM_TASK_TYPE;


typedef enum t_vod_task_type
{
    VTT_NON_VOD = 0,
    VTT_VOD = 1,
    VTT_VOD_NO_DISK = 2,
} EM_VOD_TASK_TYPE;


typedef enum t_em_encoding_mode
{ 
	EM_ENCODING_NONE = 0, /* ����ԭʼ�ֶ�(����et_release_torrent_seed_info��_encoding�ֶξ��������ʽ)  */
	EM_ENCODING_GBK = 1,/*  ����GBK��ʽ���� */
	EM_ENCODING_UTF8 = 2,/* ����UTF-8��ʽ���� */
	EM_ENCODING_BIG5 = 3,/* ����BIG5��ʽ����  */
	EM_ENCODING_UTF8_PROTO_MODE = 4,/* ���������ļ��е�utf-8�ֶ�  */
	EM_ENCODING_DEFAULT = 5/* δ���������ʽ(ʹ��et_set_seed_switch_type��ȫ���������)  */
}EM_ENCODING_MODE ;

/* Structures for running task status */
typedef struct t_runing_status
{
	EM_TASK_TYPE _type;   
	TASK_STATE  _state;   
	_u64 _file_size;  /*�����ļ���С*/  
	_u64 _downloaded_data_size; 		 /*�����ص����ݴ�С*/  
	_u32 _dl_speed;  					/* ʵʱ��������*/  
	_u32 _ul_speed;  					/* ʵʱ�ϴ�����*/  
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
	_u64 _file_size;  /*�����ļ���С*/  
	_u64 _downloaded_data_size; 		 /*�����ص����ݴ�С*/  
	 
	_u32 _start_time;	
	_u32 _finished_time;

	_u32  _failed_code;
	//_u32  _inner_id;		//�������������ؿ�������ʱ�����_inner_idΪ���ؿ��task_id
	_u32  _bt_total_file_num; // just for bt task
	BOOL _is_deleted;	//�����񱻷���deleted_tasks������ʱ,����_state����ΪETS_TASK_DELETED�����Ǳ���ԭ����ֵ�Ա������Ա���ȷ��ԭ���������ʾ�����ʱ״̬ΪETS_TASK_DELETED
	BOOL _check_data;	//���ޱ�Ҫ?
	BOOL _is_no_disk;					/* �Ƿ�Ϊ���̵㲥  */

	char _task_tag[ETM_MAX_TASK_TAG_LEN];
}EM_TASK_INFO;

typedef struct t_file_downstat_info
{	/*��file_size,stat_time��,�����ۻ�ֵ*/
	_u64 _orisrv_recv_size;	/*ԭʼ·�������ֽ���*/
	_u64 _orisrv_download_size;

	_u64 _altsrv_recv_size;	/*��ѡurl�����ֽ���*/
	_u64 _altsrv_download_size;

	_u64 _hsc_recv_size;	/*����ͨ�������ֽ���*/
	_u64 _hsc_download_size;

	_u64 _p2p_recv_size;	/*p2p�����ֽ���*/
	_u64 _p2p_download_size;

	_u64 _lixian_recv_size;	/*���������ֽ���*/
	_u64 _lixian_download_size;

	_u64 _total_recv_size;      //�������ݴ�С
	_u64 _total_download_size;  //��Ч�������ش�С
	
	_u64 _file_size;

	_u32 _first_byte_arrive_time;//����ʱ��, ����ʼ���յ���һ���ֽڵ�ʱ��,��λ��	
	_u32 _download_time;           //������ʱ
	_u32 _zero_speed_time;	//����ʱ��,ÿ��ͳ��һ��, ����ʼ��(������),�ڸ�����û���κ�������,����Ϊ��������0�ٶ�,����ֵ+1
	_u32 _checksum_time;		//У��ʱ��

	_u32 _stat_time;	/*ͳ��ʱ��*/
}EM_FILE_DOWNSTAT_INFO;

typedef enum t_em_hsc_stat
{
	EM_HSC_IDLE, EM_HSC_ENTERING, EM_HSC_SUCCESS, EM_HSC_FAILED, EM_HSC_STOP
} EM_HSC_STAT;
typedef struct t_em_high_speed_channel_info
{
	_u64				_product_id;
	_u64				_sub_id;
	_int32				_channel_type;// ͨ������: 0=�̳����3�� 1=�̳���ɫͨ�� 2=vip����ͨ�� -1=������
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
	BOOL _full_info	: 1;	//_task_infoָ����������ȫ��Ϣ���ǻ�����Ϣ 
	BOOL _correct_file_name	: 1;  // �Ƿ��Ѿ��������ļ���
	
	_u8 _file_path_len;
	_u8 _file_name_len;
	_u16 _url_len_or_need_dl_num; 
	_u16 _ref_url_len_or_seed_path_len; 
	_u32  _user_data_len;
	_u8 _eigenvalue[CID_SIZE];

	_u64 _file_size;  /*�����ļ���С*/  
	_u64 _downloaded_data_size; 		 /*�����ص����ݴ�С*/  
	 
	_u32 _start_time;	
	_u32 _finished_time;

	_u32  _failed_code;

	_u32  _bt_total_file_num; // just for bt task
	_u32  _file_name_eigenvalue; 
 
	
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
	_u64 _downloaded_data_size; 		 /*�����ص����ݴ�С*/  
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
	_u64 _downloaded_data_size; 		 /*��д�����̵����ݴ�С*/  
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
	BOOL _is_download;			//vod ����ģʽ
	_u32 _set_timestamp;		//vod ����ģʽ����ʱ��
	_u32 _file_retain_time;		//vod �����ļ�����ʱ�䣬��λΪ��
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

#define CHANGE_DOWNLOAD_MODE 					(0x00002000)
/* Structures for task */
typedef struct t_task
{
	TASK_INFO * _task_info;// ָ�����������Ϣ

	_u32  _inner_id;
	_u32 _offset;		 //������洢��task_store.dat�е�ƫ��,���ڶ�ȡ�͸�д�ýڵ���Ϣ(sd_setfilepos)
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
	_u64 _last_origin_data_size;

	_u32 _create_time;			/*����ʱ��*/
    EM_FILE_DOWNSTAT_INFO _file_downstat_info;/*�ϴ�stopʱ���ۼ�ֵ,��ǰet �� ��ͳ�����ݲ��� */
	EM_FILE_DOWNSTAT_INFO * _p_et_file_downstat_info;/*��ǰet����ͳ������*/
    EM_FILE_DOWNSTAT_INFO _file_downstat_info_report;/*�ϴ�reportʱ���ۼ�ֵ*/
}EM_TASK; //80 + 120  bytes

/* Structures for running task */
typedef struct t_running_task
{
	RUNNING_STATUS _status;
	_u32  _task_id;		
	_u32  _inner_id;		//���ؿ��task_id
	_u32  _dled_size_fresh_count;		
	EM_TASK * _task;    // ָ�����������Ϣ
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

    _u32 _task_tag_len;
	char *_task_tag; 
}EM_P2SP_TASK;

/* Structures for BT task */
typedef struct t_bt_task_info
{
	TASK_INFO _task_info;

	char *_file_path; 
	char * _file_name;

	char * _seed_file_path;
	void * _user_data;
	_u16* _dl_file_index_array;//array[_need_dl_num]  ָ�����ڴ洢������Ҫ���ص����ļ����ļ����
	BT_FILE * _file_array; //array[_need_dl_num]  ָ�����ڴ洢������Ҫ���ص����ļ�����Ϣ����

    _u32 _task_tag_len;
	char *_task_tag;
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
	LIST _full_task_infos;		//������Ϣcache (full TASK_INFO)
	_u32 _max_cache_num;
	
	LIST  _dling_task_order;			//���ڼ�¼�����������ȼ�
	BOOL _dling_order_changed;		// �б��Ƿ���Ĺ�	
	
	MAP _all_tasks;				//��������������id�Ķ�Ӧ��ϵ,���ڿ���ͨ��task_id�ҵ���Ӧ��TASK  (key=task_id -> data=TASK)
	
	MAP _eigenvalue_url;				//����ȥ����:����ETT_URL �����url hash ֵ������id�Ķ�Ӧ��ϵ, (key=url_hash -> data=task_id)
	MAP _eigenvalue_tcid;			//����ȥ����:����ETT_TCID ��ETT_LAN�����tcid ֵ������id�Ķ�Ӧ��ϵ, (key=tcid -> data=task_id)
	MAP _eigenvalue_gcid;			//����ȥ����:����ETT_KANKAN �����gcid ֵ������id�Ķ�Ӧ��ϵ, (key=gcid -> data=task_id)
	MAP _eigenvalue_bt;				//����ȥ����:����ETT_BT �����info hash ֵ������id�Ķ�Ӧ��ϵ, (key=info_hash -> data=task_id)
	MAP _eigenvalue_file;				//����ȥ����:����ETT_FILE ����� hash ֵ������id�Ķ�Ӧ��ϵ, (key=info_hash -> data=task_id)
	MAP _file_name_eigenvalue;		//����ȥ����:���� ������ļ������� ֵ������id�Ķ�Ӧ��ϵ, (key=info_hash -> data=task_id)

	RUNNING_TASK _running_task[MAX_ALOW_TASKS];
	_u32  _running_task_num;
	_u32  _max_running_task_num;
	_u32 _total_task_num;		//�����Ҫ�����ۼ������µ�task_id
	BOOL _running_task_loaded;		// �Ƿ��Ѿ�������Ҫ��������������
	BOOL _need_notify_state_changed;		// �Ƿ���Ҫ֪ͨ����
	BOOL _running_task_changed;		// �Ƿ���Ҫ�����������е�����
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

	_u32 _last_save_stat_time;//�ϴα��������б�ͳ����Ϣ��ʱ��
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
	_u32 _encoding;//�����ļ������ʽ��GBK = 0, UTF_8 = 1, BIG5 = 2
	_u8 _info_hash[20];
	EMD_TORRENT_FILE_INFO *file_info_array_ptr;
} EMD_TORRENT_SEED_INFO;

/* ������������ʱ��������� */
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
	EM_URL_ENCODING_MODE _url_encoding_mode;
	
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

	char * _task_tag;					/* �����ʶ�����ڽ��й����һ��������ͬһ���ֶα�ʶ���� */
	_u32 _task_tag_len;					/* �����ʶ���� */
} EM_CREATE_TASK;

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
	EM_URL_ENCODING_MODE _url_encoding_mode;
	
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
    EM_VOD_TASK_TYPE _vod_task_type;

	char * _task_tag;					/* �����ʶ�����ڽ��й����һ��������ͬһ���ֶα�ʶ���� */
    _u32 _task_tag_len;					/* �����ʶ���� */

	
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
	EM_TASK_TYPE _type;				/* �������ͣ�����׼ȷ��д*/
	
	char* _url;							/* _type== ETT_URL ����_type== ETT_EMULE ʱ��������������URL */
	_u32 _url_len;						/* URL �ĳ��ȣ�����С��512 */
	char _eigenvalue[40];					/* _type== ETT_TCID��ETT_LAN ʱ,�����CID,_type== ETT_KANKAN ʱ,�����GCID, _type== ETT_BT ʱ,�����info_hash   */
}EM_EIGENVALUE;


/* ����ͨ�������Ϣ */
typedef enum em_hsc_state{HS_IDLE=0, HS_RUNNING, HS_FAILED}EM_HSC_STATE;
typedef struct t_em_hsc_info
{
	EM_HSC_STATE 		_state;
	_u32 					_res_num;	//��Դ����
	_u64 				_dl_bytes;	//ͨ������ͨ�����ص�������
	_u32 					_speed;		
	_int32	 			_errcode;
}EM_HSC_INFO;

typedef struct t_lixian_id
{
	_u32 _file_index; 	
	_u64 _lixian_id;	
}DT_LIXIAN_ID;

typedef struct t_origin_info
{
	BOOL _is_origin_mode; 	
	_u64 _origin_download_data_size;	
}DT_ORIGIN_INFO;


#define DT_URL_BT_MAGNET_PREFIX "http://www.etm.runmit.com/downloadmagnet.asp?"
#define DT_URL_BT_MAGNET_PREFIX_LEN strlen(DT_URL_BT_MAGNET_PREFIX)	//should precalculate it .
#ifdef __cplusplus
}
#endif


#endif // !defined(__DOWNLOAD_TASK_H_20080918)

