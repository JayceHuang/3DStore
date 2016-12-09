#ifndef ET_TASK_MANAGER_H_200909081912
#define ET_TASK_MANAGER_H_200909081912
/*-------------------------------------------------------*/
/*                       IDENTIFICATION                             	*/
/*-------------------------------------------------------*/
/*     Filename  : et_task_manager.h                                	*/
/*     Author    : Zeng Yuqing                                      	*/
/*     Project   : EmbedThunderManager                              	*/
/*     Version   : 1.8 								    	*/
/*-------------------------------------------------------*/
/*                  Shenzhen XunLei Networks			          	*/
/*-------------------------------------------------------*/
/*                  - C (copyright) - www.xunlei.com -		    	*/
/*                                                                          		      	*/
/*      This document is the proprietary of XunLei                  	*/
/*                                                                          			*/
/*      All rights reserved. Integral or partial reproduction         */
/*      prohibited without a written authorization from the           */
/*      permanent of the author rights.                               */
/*                                                                          			*/
/*-------------------------------------------------------*/
/*-------------------------------------------------------*/
/*                  FUNCTIONAL DESCRIPTION                           */
/*-------------------------------------------------------*/
/* This file contains the interfaces of EmbedThunderTaskManager       */
/*-------------------------------------------------------*/

/*-------------------------------------------------------*/
/*                       HISTORY                                     */
/*-------------------------------------------------------*/
/*   Date     |    Author   | Modification                            */
/*-------------------------------------------------------*/
/* 2009.09.08 |ZengYuqing  | Creation                                 */
/* 2010.11.11 |ZengYuqing  | Update to 1.5                                 */
/* 2011.06.29 |ZengYuqing  | Update to 1.7                                 */
/* 2011.09.01 |ZengYuqing  | Update to 1.8 for walkbox                */
/*-------------------------------------------------------*/

#ifdef __cplusplus
extern "C" 
{
#endif

/************************************************************************/
/*                    TYPE DEFINE                    */
/************************************************************************/
typedef unsigned char		u8;
typedef unsigned short		u16;
typedef unsigned int		u32;
typedef char				int8;
typedef short				int16;
typedef int					int32;
typedef unsigned long long	uint64;
typedef long long			int64;

#ifndef NULL
#define NULL	((void*)(0))
#endif

#define ETDLL_API


#ifndef TRUE
typedef int32 Bool;
typedef int32 BOOL;
#define TRUE	(1)
#define FALSE	(0)
#else
typedef int32 Bool;
#endif


/************************************************************************/
/*                    STRUCT DEFINE                 */
/************************************************************************/
#define ETM_MAX_FILE_NAME_LEN (512)		/* �ļ�����󳤶� */
#define ETM_MAX_FILE_PATH_LEN (512)		/* �ļ�·��(�������ļ���)��󳤶� */
#define ETM_MAX_TD_CFG_SUFFIX_LEN (8)		/* Ѹ��������ʱ�ļ��ĺ�׺��󳤶� */
#define ETM_MAX_URL_LEN (1024)		/* URL ��󳤶� */
#define ETM_MAX_COOKIE_LEN ETM_MAX_URL_LEN

/************************************************************************/

/* ��������*/
typedef enum t_etm_task_type
{
	ETT_URL = 0, 				/* ��URL ����������,֧��Э��: http://,https://,ftp://,thunder:// */
	ETT_BT, 					/* ��.torrent �ļ����������� */
	ETT_TCID,  					/* ��Ѹ�׵�TCID ����������  */
	ETT_KANKAN, 				/* ��Ѹ�׵�TCID,File_Size,��GCIDһ�𴴽������� ,ע�� �������͵����񲻻��Զ�ֹͣ����Ҫ����etm_stop_vod_task�Ż�ֹͣ*/
	ETT_EMULE, 					/* �õ�¿���Ӵ���������  								 */
	ETT_FILE ,					/* ���ļ�·��ֱ�Ӵ���һ����������,Ŀ���ǰ��ֳɵ��ļ�תΪһ��������ETM����  */
	ETT_LAN					/* ��tcid,[gcid],filesize,filepath,filename,url����һ������������,����������ص���ֻ��ָ����URL�������� */
} ETM_TASK_TYPE;

/* ����״̬ */
typedef enum t_etm_task_state
{
	ETS_TASK_WAITING =0, 
	ETS_TASK_RUNNING , 
	ETS_TASK_PAUSED  ,
	ETS_TASK_SUCCESS , 
	ETS_TASK_FAILED  , 
	ETS_TASK_DELETED 
} ETM_TASK_STATE;

/* BT ������ַ��������� */
typedef enum t_etm_encoding_mode
{ 
	EEM_ENCODING_NONE = 0, 				/*  ����ԭʼ�ֶ�*/
	EEM_ENCODING_GBK = 1,					/*  ����GBK��ʽ���� */
	EEM_ENCODING_UTF8 = 2,				/*  ����UTF-8��ʽ���� */
	EEM_ENCODING_BIG5 = 3,				/*  ����BIG5��ʽ����  */
	EEM_ENCODING_UTF8_PROTO_MODE = 4,	/*  ���������ļ��е�utf-8�ֶ�  */
	EEM_ENCODING_DEFAULT = 5				/*  δ���������ʽ(ʹ��etm_set_default_encoding_mode��ȫ���������)  */
}ETM_ENCODING_MODE ;

/* ���������е������ʵʱ״�� */
typedef struct t_etm_running_status
{
	ETM_TASK_TYPE  _type;   
	ETM_TASK_STATE  _state;   
	uint64 _file_size; 					/* �����ļ���С*/  
	uint64 _downloaded_data_size; 	/* �����ص����ݴ�С*/  
	u32 _dl_speed;  					/* ʵʱ��������*/  
	u32 _ul_speed;  					/* ʵʱ�ϴ�����*/  
	u32 _downloading_pipe_num; 			/* �����������ݵ�socket ������ */
	u32 _connecting_pipe_num; 		/* ���������е�socket ������ */
}ETM_RUNNING_STATUS;

/* ������Ϣ */
typedef struct t_etm_task_info
{
	u32  _task_id;		
	ETM_TASK_STATE  _state;   
	ETM_TASK_TYPE  _type;   

	char _file_name[ETM_MAX_FILE_NAME_LEN];
	char _file_path[ETM_MAX_FILE_PATH_LEN]; 
	uint64 _file_size; 
	uint64 _downloaded_data_size; 		
	 
	u32 _start_time;				/* ����ʼ���ص�ʱ�䣬��1970��1��1�տ�ʼ������ */
	u32 _finished_time;			/* �������(�ɹ���ʧ��)��ʱ�䣬��1970��1��1�տ�ʼ������ */

	 /*����ʧ��ԭ��
              102  �޷�����
              103  �޷���ȡcid
              104  �޷���ȡgcid
              105  cid У�����
              106  gcidУ�����
              107  �����ļ�ʧ��
              108  д�ļ�ʧ��
              109  ���ļ�ʧ��
              112  �ռ䲻���޷������ļ�
              113 У��cidʱ���ļ����� 
              130  ����Դ����ʧ��

              15400 ���ļ�����ʧ��(bt����)         */  	  
	u32  _failed_code;			/* �������ʧ�ܵĻ�,�������ʧ���� */
	 
	u32  _bt_total_file_num; 		/* ����������_type== ETT_BT, ������torrent�ļ��а��������ļ��ܸ���*/
	Bool _is_deleted;			/* �����񱻳ɹ�����etm_delete_task �ӿں�,����_state����ΪETS_TASK_DELETED�����Ǳ���ԭ����ֵ�Ա������Ա���ȷ��ԭ(etm_recover_task)�������ֵ��ʶ�����ʱ״̬ΪETS_TASK_DELETED */
	Bool _check_data;			/* �㲥ʱ�Ƿ���Ҫ��֤���� */	
	Bool _is_no_disk;			/* �Ƿ�Ϊ���̵㲥  */
}ETM_TASK_INFO;

/* BT �������ļ�������״̬ */
typedef enum t_etm_file_status
{
	EFS_IDLE = 0, 
	EFS_DOWNLOADING, 
	EFS_SUCCESS, 
	EFS_FAILED 
} ETM_FILE_STATUS;

/* BT �������ļ���������Ϣ */
typedef struct  t_etm_bt_file
{
	u32 _file_index;
	Bool _need_download;			/* �Ƿ���Ҫ���� */
	ETM_FILE_STATUS _status;
	uint64 _file_size;	
	uint64 _downloaded_data_size;
	u32 _failed_code;
}ETM_BT_FILE;

/* BT �������ļ�����Ϣ */
typedef struct t_etm_torrent_file_info
{
	u32 _file_index;
	char *_file_name;
	u32 _file_name_len;
	char *_file_path;
	u32 _file_path_len;
	uint64 _file_offset;		/* �����ļ�������BT �ļ��е�ƫ�� */
	uint64 _file_size;
} ETM_TORRENT_FILE_INFO;

/* torrent �ļ�����Ϣ */
typedef struct t_etm_torrent_seed_info
{
	char _title_name[ETM_MAX_FILE_NAME_LEN-ETM_MAX_TD_CFG_SUFFIX_LEN];
	u32 _title_name_len;
	uint64 _file_total_size;						/* �������ļ����ܴ�С*/
	u32 _file_num;								/* �������ļ����ܸ���*/
	u32 _encoding;								/* �����ļ������ʽ��GBK = 0, UTF_8 = 1, BIG5 = 2 */
	unsigned char _info_hash[20];					/* �����ļ�������ֵ */
	ETM_TORRENT_FILE_INFO *file_info_array_ptr;	/* ���ļ���Ϣ�б� */
} ETM_TORRENT_SEED_INFO;


/* E-Mule�����а������ļ���Ϣ */
typedef struct tagETM_ED2K_LINK_INFO
{
	char		_file_name[256];
	uint64		_file_size;
	u8		_file_id[16];
	u8		_root_hash[20];
	char		_http_url[512];
}ETM_ED2K_LINK_INFO;


/************************************************************************/

#define	ETM_INVALID_TASK_ID			0


/* ������������ʱ��������� */
typedef struct t_etm_create_task
{
	ETM_TASK_TYPE _type;				/* �������ͣ�����׼ȷ��д*/
	
	char* _file_path; 						/* �����ļ����·���������Ѵ����ҿ�д�����ΪNULL �����ʾ�����Ĭ��·����(��etm_set_download_path��������) */
	u32 _file_path_len;					/* �����ļ����·���ĳ��ȣ�����С�� ETM_MAX_FILE_PATH_LEN */
	char* _file_name;					/* �����ļ����� */
	u32 _file_name_len; 					/* �����ļ����ֵĳ��ȣ�����С�� ETM_MAX_FILE_NAME_LEN-ETM_MAX_TD_CFG_SUFFIX_LEN */
	
	char* _url;							/* _type== ETT_URL ,ETT_LAN����_type== ETT_EMULE ʱ��������������URL */
	u32 _url_len;						/* URL �ĳ��ȣ�����С��512 */
	char* _ref_url;						/* _type== ETT_URL ʱ������������������ҳ��URL ,��ΪNULL */
	u32 _ref_url_len;						/* ����ҳ��URL �ĳ��ȣ�����С��512 */
	
	char* _seed_file_full_path; 			/* _type== ETT_BT ʱ�������������������ļ�(*.torrent) ȫ·�� */
	u32 _seed_file_full_path_len; 		/* �����ļ�ȫ·���ĳ��ȣ�����С��ETM_MAX_FILE_PATH_LEN+ETM_MAX_FILE_NAME_LEN */
	u32* _download_file_index_array; 	/* _type== ETT_BT ʱ�����������Ҫ���ص����ļ�����б�ΪNULL ��ʾ����������Ч�����ļ�*/
	u32 _file_num;						/* _type== ETT_BT ʱ�����������Ҫ���ص����ļ�����*/
	
	char *_tcid;							/* _type== ETT_TCID ,ETT_LAN��ETT_KANKAN  ʱ��������������ļ���Ѹ��TCID ,Ϊ40�ֽڵ�ʮ�����������ַ���*/
	uint64 _file_size;						/* _type== ETT_TCID ,ETT_LAN��ETT_KANKAN  ʱ��������������ļ��Ĵ�С */
	char *_gcid;							/* _type== ETT_KANKAN  ʱ��������������ļ���Ѹ��GCID ,Ϊ40�ֽڵ�ʮ�����������ַ���*/
	
	Bool _check_data;					/* ������±߲�ʱ�Ƿ���ҪУ������  */
	Bool _manual_start;					/* �Ƿ��ֶ���ʼ����?  TRUE Ϊ�û��ֶ���ʼ����,FALSEΪETM�Զ���ʼ����*/
	
	void * _user_data;					/* ���ڴ�����������ص��û�������Ϣ��������������������cookie�� �������cookie��coockie���ֱ�����"Cookie:"��ͷ������"\r\n" ���� */
	u32  _user_data_len;					/* �û����ݵĳ���,����С��65535 */
} ETM_CREATE_TASK;

/* �����㲥����ʱ��������� */
typedef struct t_etm_create_vod_task
{
	ETM_TASK_TYPE _type;

	char* _url;
	u32 _url_len;
	char* _ref_url;
	u32 _ref_url_len;

	char* _seed_file_full_path; 
	u32 _seed_file_full_path_len; 
	u32* _download_file_index_array; 	
	u32 _file_num;						/* _type== ETT_BT ��_type== ETT_KANKAN  ʱ�����������Ҫ�㲥�����ļ�����										[ �ݲ�֧��ETT_KANKAN ����ļ�] */

	char *_tcid;							/* _type== ETT_KANKAN  ʱ��������һ���������ļ�(һ����Ӱ�п��ܱ��ֳ�n���ļ�) TCID ������		[ �ݲ�֧��ETT_KANKAN ����ļ�] */
	uint64 _file_size;						/* _type== ETT_KANKAN  ʱ������������(һ������)��Ҫ�㲥���ļ����ܴ�С								[ �ݲ�֧��ETT_KANKAN ����ļ�] */
	uint64 * _segment_file_size_array;	/* _type== ETT_KANKAN  ʱ����������Ƕ�����ļ�(һ����Ӱ�п��ܱ��ֳ�n���ļ�) file_size ������		[ �ݲ�֧��ETT_KANKAN ����ļ�] */
	char *_gcid;							/* _type== ETT_KANKAN  ʱ��������һ���������ļ�(һ����Ӱ�п��ܱ��ֳ�n���ļ�) GCID ������		[ �ݲ�֧��ETT_KANKAN ����ļ�] */

	Bool _check_data;					/* �Ƿ���ҪУ������  */
	Bool _is_no_disk;					/* �Ƿ�Ϊ���̵㲥  */

	void * _user_data;					/* ���ڴ�����������ص��û�������Ϣ��������������������cookie�� �������cookie��coockie���ֱ�����"Cookie:"��ͷ������"\r\n" ���� */
	u32  _user_data_len;					/* �û����ݵĳ���,����С��65535 */

	char* _file_name;
	u32   _file_name_len;
} ETM_CREATE_VOD_TASK;

/* ����΢������ʱ��������� (�����ϴ������ؼ�KB ��һ�����ݣ�һ��С�ļ���һ��СͼƬ����ҳ��)*/
typedef struct t_etm_mini_task
{
	char* _url;					/* ֻ֧��"http://" ��ͷ��url  */
	u32 _url_len;
	
	Bool _is_file;					/* �Ƿ�Ϊ�ļ���TRUEΪ_file_path,_file_path_len,_file_name,_file_name_len��Ч,_data��_data_len��Ч��FALSE���෴*/
	
	char _file_path[ETM_MAX_FILE_PATH_LEN]; 				/* _is_file=TRUEʱ, ��Ҫ�ϴ������ص��ļ��洢·����������ʵ���� */
	u32 _file_path_len;			/* _is_file=TRUEʱ, ��Ҫ�ϴ������ص��ļ��洢·���ĳ���*/
	char _file_name[ETM_MAX_FILE_NAME_LEN];			/* _is_file=TRUEʱ, ��Ҫ�ϴ������ص��ļ���*/
	u32 _file_name_len; 			/* _is_file=TRUEʱ, ��Ҫ�ϴ������ص��ļ�������*/
	
	u8 * _send_data;			/* _is_file=FALSEʱ,ָ����Ҫ�ϴ�������*/
	u32  _send_data_len;			/* _is_file=FALSEʱ,��Ҫ�ϴ������ݳ��� */
	
	void * _recv_buffer;			/* ���ڽ��շ��������ص���Ӧ���ݵĻ��棬����ʱ���_is_file=TRUE,�˲�����Ч*/
	u32  _recv_buffer_size;		/* _recv_buffer �����С,����Ҫ16384 byte !����ʱ���_is_file=TRUE,�˲�����Ч */
	
	char * _send_header;			/* ���ڷ���httpͷ�� */
	u32  _send_header_len;		/* _send_header�Ĵ�С	 */
	
	void *_callback_fun;			/* ������ɺ�Ļص����� : typedef int32 ( *ETM_NOTIFY_MINI_TASK_FINISHED)(void * user_data, int32 result,void * recv_buffer,u32  recvd_len,void * p_header,u8 * send_data); */
	void * _user_data;			/* �û����� */
	u32  _timeout;				/* ��ʱ,��λ��,����0ʱ180�볬ʱ*/
	u32  _mini_id;				/* id */
	Bool  _gzip;					/* �Ƿ���ܻ���ѹ���ļ� */
} ETM_MINI_TASK;

/* �����̳�����ʱ��������� */
typedef struct t_etm_create_shop_task
{
	char* _url;							/* URL �ĳ��ȣ�����С��512 */
	u32 _url_len;
	char* _ref_url;
	u32 _ref_url_len;					/* ����ҳ��URL �ĳ��ȣ�����С��512 */	
	
	char* _file_path; 					/* �����ļ����·���������Ѵ����ҿ�д�����ΪNULL �����ʾ�����Ĭ��·����(��etm_set_download_path��������) */
	u32 _file_path_len;					/* �����ļ����·���ĳ��ȣ�����С�� ETM_MAX_FILE_PATH_LEN */
	char* _file_name;					/* �����ļ����� */
	u32 _file_name_len; 				/* �����ļ����ֵĳ��ȣ�����С�� ETM_MAX_FILE_NAME_LEN-ETM_MAX_TD_CFG_SUFFIX_LEN */

	char *_tcid;
	uint64 _file_size;
	char *_gcid;

	uint64 _group_id;
	char *_group_name;
	u32 _group_name_len;				/* group�����ֳ��ȣ�����С��32 */
	u8 _group_type;
	uint64 _product_id;
	uint64 _sub_id;
} ETM_CREATE_SHOP_TASK;

/************************************************************************/
/*  Interfaces provided by EmbedThunderTaskManager	*/
/************************************************************************/
/////1 ��ʼ���뷴��ʼ����ؽӿ�

/////1.1 ��ʼ����etm_system_path ΪETM ���ڴ��ϵͳ���ݵ�Ŀ¼�������Ѵ����ҿ�д����СҪ��1M �Ŀռ䣬�����ڿ��ƶ��Ĵ洢�豸�У�path_len ����С��ETM_MAX_FILE_PATH_LEN
ETDLL_API int32 etm_init(const char *etm_system_path,u32  path_len);

/////1.2 ����ʼ��
ETDLL_API int32 etm_uninit(void);

/////1.1 ���ⲿ�豸����ʱ�ɵ��øýӿڳ�ʼ����������etm_system_path ΪETM ���ڴ��ϵͳ���ݵ�Ŀ¼�������Ѵ����ҿ�д����СҪ��1M �Ŀռ䣬�����ڿ��ƶ��Ĵ洢�豸�У�path_len ����С��ETM_MAX_FILE_PATH_LEN
ETDLL_API int32 etm_load_tasks(const char *etm_system_path,u32  path_len);

/////U�̰γ�ʱ���øýӿ�ж�ص��������񣬵��Ǳ������̵㲥����
ETDLL_API int32 etm_unload_tasks(void);


/////1.3 ������ؽӿ�
/*
�����������ͣ�
(��16λ) */
#define UNKNOWN_NET   	0x00000000
#define CHINA_MOBILE  	0x00000001
#define CHINA_UNICOM  	0x00000002
#define CHINA_TELECOM 	0x00000004
#define OTHER_NET     	0x00008000
/* (��16λ) */
#define NT_GPRS_WAP   	0x00010000
#define NT_GPRS_NET   	0x00020000
#define NT_3G         		0x00040000
#define NT_WLAN       		0x00080000   // wifi and lan ...

#define NT_CMWAP 		(NT_GPRS_WAP|CHINA_MOBILE)
#define NT_CMNET 		(NT_GPRS_NET|CHINA_MOBILE)

#define NT_CUIWAP 		(NT_GPRS_WAP|CHINA_UNICOM)
#define NT_CUINET 		(NT_GPRS_NET|CHINA_UNICOM)

typedef int32 ( *ETM_NOTIFY_NET_CONNECT_RESULT)(u32 iap_id,int32 result,u32 net_type);
typedef enum t_etm_net_status
{
	ENS_DISCNT = 0, 
	ENS_CNTING, 
	ENS_CNTED 
} ETM_NET_STATUS;

ETDLL_API int32 etm_set_network_cnt_notify_callback( ETM_NOTIFY_NET_CONNECT_RESULT callback_function_ptr);
ETDLL_API int32 etm_init_network(u32 iap_id);
ETDLL_API int32 etm_uninit_network(void);
ETDLL_API ETM_NET_STATUS etm_get_network_status(void);
ETDLL_API int32 etm_get_network_iap(u32 *iap_id);
ETDLL_API int32 etm_get_network_iap_from_ui(u32 *iap_id);
ETDLL_API const char* etm_get_network_iap_name(void);
ETDLL_API int32 etm_get_network_flow(u32 * download,u32 * upload);
/* peerid�ĳ��Ȳ���С��16������᷵�ز������� */
ETDLL_API int32 etm_set_peerid(const char* peerid, u32 peerid_len);
ETDLL_API const char* etm_get_peerid(void);
ETDLL_API int32 etm_user_set_network(Bool is_ok);

ETDLL_API int32 etm_set_net_type(u32 net_type);

///////////////////////////////////////////////////////////////
/////2 ϵͳ������ؽӿ�

/////2.1  �����û��Զ���ĵײ�ӿ�,����ӿڱ����ڵ���etm_init ֮ǰ����!
#define ET_FS_IDX_OPEN           (0)
#define ET_FS_IDX_ENLARGE_FILE   (1)
#define ET_FS_IDX_CLOSE          (2)
#define ET_FS_IDX_READ           (3)
#define ET_FS_IDX_WRITE          (4)
#define ET_FS_IDX_PREAD          (5)
#define ET_FS_IDX_PWRITE         (6)
#define ET_FS_IDX_FILEPOS        (7)
#define ET_FS_IDX_SETFILEPOS     (8)
#define ET_FS_IDX_FILESIZE       (9)
#define ET_FS_IDX_FREE_DISK      (10)
#define ET_SOCKET_IDX_SET_SOCKOPT (11)
#define ET_MEM_IDX_GET_MEM           (12)
#define ET_MEM_IDX_FREE_MEM          (13)
#define ET_ZLIB_UNCOMPRESS          (14)

/* �����б�˵����
 * 	int32 fun_idx    �ӿں��������
 * 	void *fun_ptr    �ӿں���ָ��
 *
 *
 *  Ŀǰ֧�ֵĽӿں�������Լ����Ӧ�ĺ�������˵����
 *------------------------------------------------------------------------------------------------------------------
 *  ��ţ�		0 (ET_FS_IDX_OPEN)      
 *  ���Ͷ��壺	typedef int32 (*et_fs_open)(char *filepath, int32 flag, u32 *file_id);
 *  ˵����		���ļ�����Ҫ�Զ�д��ʽ�򿪡��ɹ�ʱ����0�����򷵻ش�����
 *  ����˵����
 *	 filepath����Ҫ���ļ���ȫ·���� 
 *	 flag��    ��(flag & 0x01) == 0x01ʱ���ļ��������򴴽��ļ��������ļ�������ʱ��ʧ��
                                                                       ����ļ��������Զ�дȨ�޴��ļ�
                      ��(flag & 0x02) == 0x02ʱ����ʾ��ֻ���ļ�
                      ��(flag & 0x04) == 0x04ʱ����ʾ��ֻд
                      ��(flag ) = 0x 0ʱ����ʾ��дȨ�޴��ļ�
 *	 file_id�� �ļ��򿪳ɹ��������ļ����
 *------------------------------------------------------------------------------------------------------------------
 *  ��ţ�		1 (ET_FS_IDX_ENLARGE_FILE)   
 *  ���Ͷ��壺	typedef int32 (*et_fs_enlarge_file)(u32 file_id, uint64 expect_filesize, uint64 *cur_filesize);
 *  ˵����		���¸ı��ļ���С��Ŀǰֻ��Ҫ��󣩡�һ�����ڴ��ļ��󣬽���Ԥ�����ļ��� �ɹ�����0�����򷵻ش�����
 *  ����˵����
 *   file_id����Ҫ���Ĵ�С���ļ����
 *   expect_filesize�� ϣ�����ĵ����ļ���С
 *   cur_filesize�� ʵ�ʸ��ĺ���ļ���С��ע�⣺�����ô�С�ɹ���һ��Ҫ��ȷ���ô˲�����ֵ!��
 *------------------------------------------------------------------------------------------------------------------
 *  ��ţ�		2 (ET_FS_IDX_CLOSE)   
 *  ���Ͷ��壺	typedef int32 (*et_fs_close)(u32 file_id);
 *  ˵����		�ر��ļ����ɹ�����0�����򷵻ش�����
 *  ����˵����
 *   file_id����Ҫ�رյ��ļ����
 *------------------------------------------------------------------------------------------------------------------
 *  ��ţ�		3 (ET_FS_IDX_READ)   
 *  ���Ͷ��壺	typedef int32 (*et_fs_read)(u32 file_id, char *buffer, int32 size, u32 *readsize);
 *  ˵����		��ȡ��ǰ�ļ�ָ���ļ����ݡ��ɹ�����0�����򷵻ش�����
 *  ����˵����
 *   file_id�� ��Ҫ��ȡ���ļ����
 *   buffer��  ��Ŷ�ȡ���ݵ�bufferָ��
 *   size��    ��Ҫ��ȡ�����ݴ�С�������߿��Ա�֤���ᳬ��buffer�Ĵ�С��
 *   readsize��ʵ�ʶ�ȡ���ļ���С��ע�⣺�ļ���ȡ�ɹ���һ��Ҫ��ȷ���ô˲�����ֵ!��
 *------------------------------------------------------------------------------------------------------------------
 *  ��ţ�		4 (ET_FS_IDX_WRITE)   
 *  ���Ͷ��壺	typedef int32 (*et_fs_write)(u32 file_id, char *buffer, int32 size, u32 *writesize);
 *  ˵����		�ӵ�ǰ�ļ�ָ�봦д�����ݡ��ɹ�����0�����򷵻ش�����
 *  ����˵����
 *   file_id��  ��Ҫд����ļ����
 *   buffer��   ���д�����ݵ�bufferָ��
 *   size��     ��Ҫд������ݴ�С�������߿��Ա�֤���ᳬ��buffer�Ĵ�С��
 *   writesize��ʵ��д����ļ���С��ע�⣺�ļ�д��ɹ���һ��Ҫ��ȷ���ô˲�����ֵ!��
 *------------------------------------------------------------------------------------------------------------------
 *  ��ţ�		5 (ET_FS_IDX_PREAD)   
 *  ���Ͷ��壺	typedef int32 (*et_fs_pread)(u32 file_id, char *buffer, int32 size, uint64 filepos, u32 *readsize);
 *  ˵����		��ȡָ��ƫ�ƴ����ļ����ݡ��ɹ�����0�����򷵻ش�����
 *  ����˵����
 *   file_id�� ��Ҫ��ȡ���ļ����
 *   buffer��  ��Ŷ�ȡ���ݵ�bufferָ��
 *   size��    ��Ҫ��ȡ�����ݴ�С�������߿��Ա�֤���ᳬ��buffer�Ĵ�С��
 *   filepos�� ��Ҫ��ȡ���ļ�ƫ��
 *   readsize��ʵ�ʶ�ȡ���ļ���С��ע�⣺�ļ���ȡ�ɹ���һ��Ҫ��ȷ���ô˲�����ֵ!��
 *------------------------------------------------------------------------------------------------------------------
 *  ��ţ�		6 (ET_FS_IDX_PWRITE)   
 *  ���Ͷ��壺	typedef int32 (*et_fs_pwrite)(u32 file_id, char *buffer, int32 size, uint64 filepos, u32 *writesize);
 *  ˵����		��ָ��ƫ�ƴ�д�����ݡ��ɹ�����0�����򷵻ش�����
 *  ����˵����
 *   file_id��  ��Ҫд����ļ����
 *   buffer��   ���д�����ݵ�bufferָ��
 *   size��     ��Ҫд������ݴ�С�������߿��Ա�֤���ᳬ��buffer�Ĵ�С��
 *   filepos��  ��Ҫ��ȡ���ļ�ƫ��
 *   writesize��ʵ��д����ļ���С��ע�⣺�ļ�д��ɹ���һ��Ҫ��ȷ���ô˲�����ֵ!��
 *------------------------------------------------------------------------------------------------------------------
 *  ��ţ�		7 (ET_FS_IDX_FILEPOS)   
 *  ���Ͷ��壺	typedef int32 (*et_fs_filepos)(u32 file_id, uint64 *filepos);
 *  ˵����		��õ�ǰ�ļ�ָ��λ�á��ɹ�����0�����򷵻ش�����
 *  ����˵����
 *   file_id�� �ļ����
 *   filepos�� ���ļ�ͷ��ʼ������ļ�ƫ��
 *------------------------------------------------------------------------------------------------------------------
 *  ��ţ�		8 (ET_FS_IDX_SETFILEPOS)   
 *  ���Ͷ��壺	typedef int32 (*et_fs_setfilepos)(u32 file_id, uint64 filepos);
 *  ˵����		���õ�ǰ�ļ�ָ��λ�á��ɹ�����0�����򷵻ش�����
 *  ����˵����
 *   file_id�� �ļ����
 *   filepos�� ���ļ�ͷ��ʼ������ļ�ƫ��
 *------------------------------------------------------------------------------------------------------------------
 *  ��ţ�		9 (ET_FS_IDX_FILESIZE)   
 *  ���Ͷ��壺	typedef int32 (*et_fs_filesize)(u32 file_id, uint64 *filesize);
 *  ˵����		��õ�ǰ�ļ���С���ɹ�����0�����򷵻ش�����
 *  ����˵����
 *   file_id�� �ļ����
 *   filepos�� ��ǰ�ļ���С
 *------------------------------------------------------------------------------------------------------------------
 *  ��ţ�		10 (ET_FS_IDX_FREE_DISK)   
 *  ���Ͷ��壺	typedef int32 (*et_fs_get_free_disk)(const char *path, u32 *free_size);
 *  ˵����		���path·�����ڴ��̵�ʣ��ռ䣬һ�������Ƿ���Դ����ļ����ж����ݡ��ɹ�����0�����򷵻ش�����
 *  ����˵����
 *   path��     ��Ҫ��ȡʣ����̿ռ�����ϵ�����·��
 *   free_size��ָ��·�����ڴ��̵ĵ�ǰʣ����̿ռ䣨ע�⣺�˲���ֵ��λ�� KB(1024 bytes) !��
 *------------------------------------------------------------------------------------------------------------------
 *  ��ţ�		11 (ET_SOCKET_IDX_SET_SOCKOPT)   
 *  ���Ͷ��壺	typedef int32 (*et_socket_set_sockopt)(u32 socket, int32 socket_type); 
 *  ˵����		����socket����ز�����Ŀǰֻ֧��Э���PF_INET���ɹ�����0�����򷵻ش�����
 *  ����˵����
 *   socket��     ��Ҫ���õ�socket���
 *   socket_type����socket�����ͣ�Ŀǰ��Ч��ֵ��2����SOCK_STREAM  SOCK_DGRAM����2�����ȡֵ�����ڵ�OSһ�¡�
 *------------------------------------------------------------------------------------------------------------------
 *  ��ţ�		12 (ET_MEM_IDX_GET_MEM)  
 *  ���Ͷ��壺	typedef int32 (*et_mem_get_mem)(u32 memsize, void **mem);
 *  ˵����		�Ӳ���ϵͳ����̶���С�������ڴ�顣�ɹ�����0�����򷵻ش�����
 *  ����˵����
 *   memsize��     ��Ҫ������ڴ��С
 *   mem�� �ɹ�����֮���ڴ���׵�ַ����*mem�з��ء�
 *------------------------------------------------------------------------------------------------------------------
 *  ��ţ�		13 (ET_MEM_IDX_FREE_MEM)   
 *  ���Ͷ��壺	typedef int32 (*et_mem_free_mem)(void* mem, u32 memsize);
 *  ˵����		�ͷ�ָ���ڴ�������ϵͳ���ɹ�����0�����򷵻ش�����
 *  ����˵����
 *   mem��     ��Ҫ�ͷŵ��ڴ���׵�ַ
 *   memsize����Ҫ�ͷŵ��ڴ��Ĵ�С
 *------------------------------------------------------------------------------------------------------------------ 
 *  ��ţ�		14 (ET_ZLIB_UNCOMPRESS)   
 *  ���Ͷ��壺	typedef _int32 (*et_zlib_uncompress)( unsigned char *p_out_buffer, int *p_out_len, const unsigned char *p_in_buffer, int in_len );
 *  ˵����		ָ��zlib��Ľ�ѹ����������,����kad��������Դ���Ľ�ѹ,���emule��Դ����
 *  ����˵����
 *   p_out_buffer����ѹ�����ݻ�����
 *   p_out_len��   ��ѹ�����ݳ���
 *   p_in_buffer�� ����ѹ���ݻ�����
 *   in_len��      ����ѹ���ݳ���
 *------------------------------------------------------------------------------------------------------------------ */
ETDLL_API int32 etm_set_customed_interface(int32 fun_idx, void *fun_ptr);
	
/////2.2 ��ȡ�汾��,����ǰ����ΪETM �İ汾�ţ���һ����Ϊ���ؿ�ET �İ汾��
ETDLL_API const char* etm_get_version(void);

/////2.3 ע��license
/*Ŀǰ�ṩ�����ڲ��Ե����license:
*				License						    |  ��� |		��Ч��		| ״̬
*	09072400010000000000009y4us41bxk5nsno35569 | 0000000 |2009-07-24~2010-07-24	| 00
*/
ETDLL_API int32 	etm_set_license(const char *license, int32 license_size);
ETDLL_API const char* etm_get_license(void);

/////2.4 ��ȡlicense ����֤���,�ӿڷ�����ͽӿڲ���*result ������0 Ϊ��֤ͨ��,����ֵ��������

/*  ����ýӿڷ���ֵΪ102406����ʾETM������Ѹ�׷����������У���Ҫ���Ժ��ٵ��ô˽ӿڡ�
*    �ڽӿڷ���ֵΪ0��ǰ����,�ӿڲ���*result��ֵ��������:
*    4096   ��ʾͨѶ����ԭ��������������������������ˡ�
*    4118   ��ʾETM�ڳ�����12�Σ�ÿ�μ��1Сʱ��license�ϱ�֮�󣬾���ʧ�ܣ�ԭ��ͬ4096��
*    21000 ��ʾ LICENSE ��֤δͨ����ԭ���� LICENSE �����ᡣ
*    21001 ��ʾ LICENSE ��֤δͨ����ԭ���� LICENSE ���ڡ�
*    21002 ��ʾ LICENSE ��֤δͨ����ԭ���� LICENSE �����ա�
*    21003 ��ʾ LICENSE ��֤δͨЭ��ԭ���� LICENSE ���ظ�ʹ�á�
*    21004 ��ʾ LICENSE ��֤δͨЭ��ԭ���� LICENSE ����ٵġ�
*    21005 ��ʾ ��������æ��ETM ����һСʱ���Զ�����,������򲻱���ᡣ
*/
ETDLL_API int32 	etm_get_license_result(u32 * result);

/////2.5 ���ý���.torrent�ļ�ʱϵͳĬ���ַ������ʽ��encoding_mode ����ΪEEM_ENCODING_DEFAULT
ETDLL_API int32 etm_set_default_encoding_mode( ETM_ENCODING_MODE encoding_mode);
ETDLL_API ETM_ENCODING_MODE etm_get_default_encoding_mode(void);

/////2.6 ����Ĭ�����ش洢·��,�����Ѵ����ҿ�д,path_len ����С��ETM_MAX_FILE_PATH_LEN
ETDLL_API int32 etm_set_download_path(const char *path,u32  path_len);
ETDLL_API const char* etm_get_download_path(void);

/////2.7 ��������״̬ת��֪ͨ�ص�����,�˺��������ڴ�������֮ǰ����
// ע��ص������Ĵ���Ӧ������࣬���򲻿����ڻص������е����κ�ETM�Ľӿں���������ᵼ������!
typedef int32 ( *ETM_NOTIFY_TASK_STATE_CHANGED)(u32 task_id, ETM_TASK_INFO * p_task_info);
ETDLL_API int32 	etm_set_task_state_changed_callback( ETM_NOTIFY_TASK_STATE_CHANGED callback_function_ptr);

/* ���������ļ����ı�֪ͨ�ص�����,�˺��������ڴ�������֮ǰ����*/
// ע��ص������Ĵ���Ӧ������࣬���򲻿����ڻص������е����κ�ETM�Ľӿں���������ᵼ������!
typedef int32 ( *ETM_NOTIFY_FILE_NAME_CHANGED)(u32 task_id, const char* file_name,u32 length);
ETDLL_API int32 etm_set_file_name_changed_callback(ETM_NOTIFY_FILE_NAME_CHANGED callback_function_ptr);
#define etm_set_task_file_name_changed_callback etm_set_file_name_changed_callback

/////2.8 �������̵㲥��Ĭ����ʱ�ļ�����·��,�����Ѵ����ҿ�д,path_len ҪС��ETM_MAX_FILE_PATH_LEN,�˺��������ڴ������̵㲥����֮ǰ����
ETDLL_API int32 etm_set_vod_cache_path(const char *path,u32  path_len);
ETDLL_API const char* etm_get_vod_cache_path(void);

/////2.9 �������̵㲥�Ļ�������С����λKB�����ӿ�etm_set_vod_cache_path ���������Ŀ¼������д�ռ䣬����3GB ����,�˺��������ڵ���etm_set_vod_cache_path֮��,�������̵㲥����֮ǰ����
ETDLL_API int32 etm_set_vod_cache_size(u32  cache_size);
ETDLL_API u32  etm_get_vod_cache_size(void);

///// ǿ�����vod ����
ETDLL_API int32  etm_clear_vod_cache(void);

/////���û���ʱ�䣬��λ��,Ĭ��Ϊ30�뻺��,���������ø�ֵ,�Ա�֤��������
ETDLL_API int32 etm_set_vod_buffer_time(u32 buffer_time);

/////2.10 ���õ㲥��ר���ڴ��С,��λKB������2MB ����,�˺��������ڴ����㲥����֮ǰ����
ETDLL_API int32 etm_set_vod_buffer_size(u32  buffer_size);
ETDLL_API u32  etm_get_vod_buffer_size(void);

/////2.11 ��ѯvod ר���ڴ��Ƿ��Ѿ�����
ETDLL_API Bool etm_is_vod_buffer_allocated(void);

/////2.12 �ֹ��ͷ�vod ר���ڴ�,ETM �����ڷ���ʼ��ʱ���Զ��ͷ�����ڴ棬�������������뾡���ڳ�����ڴ�Ļ����ɵ��ô˽ӿڣ�ע�����֮ǰҪȷ���޵㲥����������
ETDLL_API int32  etm_free_vod_buffer(void);

/////2.13 �������¸�ϵͳ����:(10M�ڴ�汾ʱ)max_tasks=5,download_limit_speed=-1,upload_limit_speed=-1,auto_limit_speed=FALSE,max_task_connection=128,task_auto_start=FALSE
ETDLL_API int32 	etm_load_default_settings(void);

/////2.14 �������ͬʱ���е��������,��СΪ1,���Ϊ15
ETDLL_API int32 etm_set_max_tasks(u32 task_num);
ETDLL_API u32 etm_get_max_tasks(void);

/////2.15 ������������,��KB Ϊ��λ,����-1Ϊ������,����Ϊ0
ETDLL_API int32 etm_set_download_limit_speed(u32 max_speed);
ETDLL_API u32 etm_get_download_limit_speed(void);

/////2.16 �����ϴ�����,��KB Ϊ��λ,����-1Ϊ������,����Ϊ0,����max_download_limit_speed:max_upload_limit_speed=3:1
ETDLL_API int32 etm_set_upload_limit_speed(u32 max_speed);
ETDLL_API u32 etm_get_upload_limit_speed(void);

/////2.17 �����Ƿ�������������(auto_limit=TRUEΪ����)��linux�±�����root Ȩ������ETM ��ʱ�������Ч
ETDLL_API int32 etm_set_auto_limit_speed(Bool auto_limit);
ETDLL_API Bool etm_get_auto_limit_speed(void);

/////2.18 �������������,10M �ڴ�汾ȡֵ��ΧΪ[1~200]
ETDLL_API int32 etm_set_max_task_connection(u32 connection_num);
ETDLL_API u32 etm_get_max_task_connection(void);

/////2.19 ����ETM �������Ƿ��Զ���ʼ����δ��ɵ�����(auto_start=TRUEΪ��)
ETDLL_API int32 etm_set_task_auto_start(Bool auto_start);
ETDLL_API Bool etm_get_task_auto_start(void);

/////2.20 ��ȡ��ǰȫ���������ٶ�,��Byte Ϊ��λ
ETDLL_API u32 etm_get_current_download_speed(void);

/////2.21 ��ȡ��ǰȫ���ϴ����ٶ�,��Byte Ϊ��λ
ETDLL_API u32 etm_get_current_upload_speed(void);

/////2.22 �������طֶδ�С,[100~1000]����λKB
ETDLL_API int32 etm_set_download_piece_size(u32 piece_size);
ETDLL_API u32 etm_get_download_piece_size(void);

/////2.23 �������������ʾ����������:0 -�ر�,1-��,3-��,4-�����龰ģʽ
ETDLL_API int32 etm_set_prompt_tone_mode(u32 mode);
ETDLL_API u32 etm_get_prompt_tone_mode(void);

/////2.24 ����p2p ����ģʽ:0 -��,1-��WIFI�����´�,2-�ر�
ETDLL_API int32 etm_set_p2p_mode(u32 mode);
ETDLL_API u32 etm_get_p2p_mode(void);

/////2.25 �������������CDN ʹ�ò���:
// enable - ʹ�ò�����Ч,enable_cdn_speed - ���������ٶ�С�ڸ�ֵʱ����CDN(��λKB),
// disable_cdn_speed -- �����ٶȼ�ȥCDN�ٶȴ��ڸ�ֵʱ�ر�CDN(��λKB)
ETDLL_API int32 etm_set_cdn_mode(Bool enable,u32 enable_cdn_speed,u32 disable_cdn_speed);

ETDLL_API int32 etm_set_ui_version(const char *ui_version, int32 product,int32 partner_id);

ETDLL_API int32 etm_get_business_type_by_product_id(int32 product_id);
////////////////////////////////////////////////////////////

// �û���¼��Ϊ����ӦֵΪ:
// 1-��װӦ�ó���
// 2-��Ӧ�ó���
// 3-�˳�Ӧ�ó���
#define REPORTER_USER_ACTION_TYPE_LOGIN (0)

// ��������Ϊ����ӦֵΪ:
// 1-��
// 2-�˳�
#define REPORTER_USER_ACTION_TYPE_PLAYER (1)

// TEXT��Ϊ����ӦֵΪ:
// 1-mov
// 2-text
// 3-software
// 9-other
#define REPORTER_USER_ACTION_TYPE_DOWNLOAD_FILE (2)
#define REPORTER_USER_ACTION_TYPE_UPLOAD_FILE (4)

//��������������Ϊ
// 1-��ʾ���뿴��������
#define REPORTER_USER_ACTION_ENTER_KANKAN_PLAYER   (3)

/*  101-Ѹ���ֻ�������� */
#define REPORTER_USER_ACTION_XUNLEI_ASSISTANT   (101)  

/* 201-IPAD������3.0���ϣ� */
#define REPORTER_USER_ACTION_IPAD_KANKAN   (201)


//����������������Ϣ
//11001-��ʾremark(data)�ֶδ�ŵ���KANKAN_PLAY_INFO_STAT����Э���
//12001-��ʾremark(data)�ֶδ�ŵ���KANKAN_USER_ACTION_STAT����Э���
#define REPORTER_USER_ACTION_PLAY_INFO    (999)

//�û���ͨ����
//action_type = REPORTER_COMMON_USER_ACTION,action_value ��Ϊ0
//��ϸ��Ϣ����data��,data_len�ĵ�λ��byte.
#define REPORTER_COMMON_USER_ACTION    (1000)

ETDLL_API int32 etm_reporter_mobile_user_action_to_file(u32 action_type, u32 action_value, void *data, u32 data_len);

/* http ��ʽ�ϱ����ϱ�data����Ϊ��ֵ�Ը�ʽ:
	u=aaaa&u1=bbbb&u2=cccc&u3=dddd&u4=eeeee
����: u=mh_chanel&u1={�ֻ��˰汾��}&u2={peerid}&u3={�ֻ���IMEI}&u4={ʱ��unixʱ��}&u5={Ƶ������}
*/
ETDLL_API int32 etm_http_report(char *data, u32 data_len);
ETDLL_API int32 etm_http_report_by_url(char *data, u32 data_len);
//��ʱ�ϱ��û���Ϊ��¼�ļ��Ŀ��أ�Ĭ�Ͽ���
ETDLL_API int32 etm_enable_user_action_report(Bool is_enable);

/* ��������ӿ����ڽ��汣��ͻ�ȡ������
	item_name ��item_value�Ϊ63�ֽ�
	����get�ӿ�ʱ,���Ҳ�����item_name ��Ӧ��������item_value Ϊ��ʼֵ�������� 
*/
ETDLL_API int32  etm_settings_set_str_item(char * item_name,char *item_value);
ETDLL_API int32  etm_settings_get_str_item(char * item_name,char *item_value);

ETDLL_API int32  etm_settings_set_int_item(char * item_name,int32 item_value);
ETDLL_API int32  etm_settings_get_int_item(char * item_name,int32 *item_value);

ETDLL_API int32  etm_settings_set_bool_item(char * item_name,Bool item_value);
ETDLL_API int32  etm_settings_get_bool_item(char * item_name,Bool *item_value);

/*��� ǿ���滻���������Ӧ��ip��ַ,���ؿⲻ�ٶԸ�������dns, ���ʹ�ö�Ӧ��ip��ַ(���ж��)*/
ETDLL_API int32  etm_set_host_ip(const char * host_name,const char *ip);
/* ��������õ�����ip */
ETDLL_API int32  etm_clean_host(void);
///////////////////////////////////////////////////////////////
/////3��������(���������غͱ��±߲�)��ؽӿ�

/////3.1 ������������
ETDLL_API int32 etm_create_task(ETM_CREATE_TASK * p_param,u32* p_task_id );

/////3.1.1 ������������
ETDLL_API int32 etm_create_shop_task(ETM_CREATE_SHOP_TASK * p_param,u32* p_task_id);

/////3.2 ��ͣ����
ETDLL_API int32 etm_pause_task (u32 task_id);

/////3.3 ��ʼ����ͣ������
ETDLL_API int32 etm_resume_task(u32 task_id);

/////�ṩ��IPAD �������Ļָ�����ӿ�
ETDLL_API int32 etm_vod_resume_task(u32 task_id);

/////3.4 ������������վ
ETDLL_API int32 etm_delete_task(u32 task_id);

/////3.5 ��ԭ��ɾ��������
ETDLL_API int32 etm_recover_task(u32 task_id);

/////ǿ�ƿ�ʼĳ����������
ETDLL_API int32 etm_force_run_task(u32 task_id);

/////3.6 ����ɾ������,��delete_fileΪTRUEʱ��ʾɾ������������Ϣ��ͬʱҲɾ�������Ӧ���ļ�
ETDLL_API int32 etm_destroy_task(u32 task_id,Bool delete_file);

/////3.7 ��ȡ�����������ȼ�task_id �б�,����״̬ΪETS_TASK_WAITING,ETS_TASK_RUNNING,ETS_TASK_PAUSED,ETS_TASK_FAILED�Ķ���������б���,*buffer_len�ĵ�λ��sizeof(u32)
ETDLL_API int32 etm_get_task_pri_id_list (u32 * id_array_buffer,u32 *buffer_len);

/////3.8 �������������ȼ�����
ETDLL_API int32 etm_task_pri_level_up(u32 task_id,u32 steps);
	
/////3.9 �������������ȼ�����
ETDLL_API int32 etm_task_pri_level_down (u32 task_id,u32 steps);

/////3.10 �����������ļ�������״̬����ΪETS_TASK_SUCCESS���Ҳ�������bt����new_name_len����С��ETM_MAX_FILE_NAME_LEN-ETM_MAX_TD_CFG_SUFFIX_LEN
ETDLL_API int32 etm_rename_task(u32 task_id,const char * new_name,u32 new_name_len);

/////3.11 ��ȡ����Ļ�����Ϣ,��etm_create_vod_task�ӿڴ���������Ҳ���Ե�������ӿ�
ETDLL_API int32 etm_get_task_info(u32 task_id,ETM_TASK_INFO *p_task_info);

/////ipad kankan ��ȡ���������������Ϣ���������״̬��㲥״̬����
ETDLL_API int32 etm_get_task_download_info(u32 task_id,ETM_TASK_INFO *p_task_info);

/////3.12 ��ȡ�������е� ���������״��,��etm_create_vod_task�ӿڴ���������Ҳ���Ե�������ӿ�
ETDLL_API int32 etm_get_task_running_status(u32 task_id,ETM_RUNNING_STATUS *p_status);

/////3.13 ��ȡ����ΪETT_URL ,ETT_LAN����ETT_EMULE�� �����URL
ETDLL_API const char* etm_get_task_url(u32 task_id);
/////�滻����ΪETT_LAN�� �����URL
ETDLL_API int32 etm_set_task_url(u32 task_id,const char * url);

/////3.14 ��ȡ����ΪETT_URL �� ���������ҳ��URL(����еĻ�)
ETDLL_API const char* etm_get_task_ref_url(u32 task_id);

/////3.15 ��ȡ�����TCID
ETDLL_API const char* etm_get_task_tcid(u32 task_id);
ETDLL_API const char* etm_get_bt_task_sub_file_tcid(u32 task_id, u32 file_index);
ETDLL_API const char* etm_get_file_tcid(const char * file_path);

/////3.16 ��ȡ���Ͳ�ΪETT_BT �� �����GCID
ETDLL_API const char* etm_get_task_gcid(u32 task_id);

/////3.17 ��ȡ����ΪETT_BT �� ����������ļ�ȫ·��
ETDLL_API const char* etm_get_bt_task_seed_file(u32 task_id);

/////3.18 ��ȡBT������ĳ�����ļ�����Ϣ,file_indexȡֵ��ΧΪ>=0����etm_create_vod_task�ӿڴ�����bt ����Ҳ���Ե�������ӿ�
ETDLL_API int32 etm_get_bt_file_info(u32 task_id, u32 file_index, ETM_BT_FILE *file_info);

/* ��ȡbt�������ļ��� */
ETDLL_API const char* etm_get_bt_task_sub_file_name(u32 task_id, u32 file_index);

/////3.19 �޸�BT������Ҫ���ص��ļ�����б�,���֮ǰ��Ҫ�µ��ļ���Ȼ��Ҫ���صĻ���ҲҪ����new_file_index_array��
ETDLL_API int32 etm_set_bt_need_download_file_index(u32 task_id, u32* new_file_index_array,	u32 new_file_num);
ETDLL_API int32 etm_get_bt_need_download_file_index(u32 task_id, u32* id_array_buffer,	u32 * buffer_len); /* *buffer_len�ĵ�λ��sizeof(u32) */

/////3.20 ��ȡ������û���������,*buffer_len�ĵ�λ��byte
ETDLL_API int32 etm_get_task_user_data(u32 task_id,void * data_buffer,u32 * buffer_len);

/////3.21 ��������״̬��ȡ����id �б�(����Զ�̿��ƴ���������ͱ��ش���������),*buffer_len�ĵ�λ��sizeof(u32)
ETDLL_API int32 	etm_get_task_id_by_state(ETM_TASK_STATE state,u32 * id_array_buffer,u32 *buffer_len);

/////3.22 ��������״̬��ȡ���ش���������id �б�(������Զ�̿��ƴ���������),*buffer_len�ĵ�λ��sizeof(u32)
ETDLL_API int32 	etm_get_local_task_id_by_state(ETM_TASK_STATE state,u32 * id_array_buffer,u32 *buffer_len);

/////3.23 ��ȡ���е�����id �б�,*buffer_len�ĵ�λ��sizeof(u32)�����������̺����̵㲥����(�������̵㲥����תΪ����ģʽ�������)
ETDLL_API int32 	etm_get_all_task_ids(u32 * id_array_buffer,u32 *buffer_len);

//// ��ȡ���������ļ����ܴ�С(����etm_get_all_task_need_space),��λByte
ETDLL_API uint64 etm_get_all_task_total_file_size(void);
	
//// ��ȡ�������������������ռ�õĿռ��С,��λByte
ETDLL_API uint64 etm_get_all_task_need_space(void);

///// ��ȡ���񱾵ص㲥URL,ֻ�������˱���http �㲥������ʱ���ܵ���
ETDLL_API const char* etm_get_task_local_vod_url(u32 task_id);

/* ����: ͨ��td.cfg�ļ�·����������
   ����: 
   		cfg_file_path: cfg�ļ���·����
   		p_task_id: ������ͨ��cfg����������id��
*/
ETDLL_API int32 etm_create_task_by_cfg_file(char* cfg_file_path, u32* p_task_id);

/* ����: �滻�Ѵ���δ������������ԭʼurl�� 
	1. ��ɾ����ԭ������(��ɾ���ļ�)��2. �޸Ķ�Ӧcfg�ļ��洢��ԭʼurl�� 3. ���µ�url��������
����: 
	new_origin_url: ���滻���µ�ԭʼurl��
	p_task_id: �������ָ�룬������ֵ��ֵ��Ӧ������Ҫ�滻url��ԭʼ���������id�� ���´���֮���ֵ���յ��µ�����id��
*/
ETDLL_API int32 etm_use_new_url_create_task(char *new_origin_url, u32* p_task_id);

///////////////////////////////////////////////////////////////
/////����������ؽӿ�
/////3.23 ����URL ,cid,gicd ��BT���ӵ�info_hash ���Ҷ�Ӧ������id
typedef struct t_etm_eigenvalue
{
	ETM_TASK_TYPE _type;				/* �������ͣ�����׼ȷ��д*/
	
	char* _url;							/* _type== ETT_URL ����_type== ETT_EMULE ʱ��������������URL */
	u32 _url_len;						/* URL �ĳ��ȣ�����С��512 */
	char _eigenvalue[40];					/* _type== ETT_TCID ����ETT_LAN ʱ,�����CID,_type== ETT_KANKAN ʱ,�����GCID, _type== ETT_BT ʱ,�����info_hash   */
}ETM_EIGENVALUE;
ETDLL_API int32 	etm_get_task_id_by_eigenvalue(ETM_EIGENVALUE * p_eigenvalue,u32 * task_id);

/////3.24 ����������Ϊ��������
ETDLL_API int32 etm_set_task_lixian_mode(u32 task_id,Bool is_lixian);

/////3.25 ��ѯ�����Ƿ�Ϊ��������
ETDLL_API Bool etm_is_lixian_task(u32 task_id);

/////3.26 ����server ��Դ,file_indexֻ��BT�������õ�,��ͨ����file_index�ɺ���
typedef struct t_etm_server_res
{
	u32 _file_index;
	u32 _resource_priority;
	char* _url;							
	u32 _url_len;					
	char* _ref_url;				
	u32 _ref_url_len;		
	char* _cookie;				
	u32 _cookie_len;		
}ETM_SERVER_RES;
ETDLL_API int32 etm_add_server_resource( u32 task_id, ETM_SERVER_RES  * p_resource );

/////3.27 ����peer ��Դ,file_indexֻ��BT�������õ�,��ͨ����file_index�ɺ���
#define ETM_PEER_ID_SIZE 16
typedef struct t_etm_peer_res
{
	u32 _file_index;
	char _peer_id[ETM_PEER_ID_SIZE];							
	u32 _peer_capability;					
	u32 _res_level;
	u32 _host_ip;
	u16 _tcp_port;
	u16 _udp_port;
}ETM_PEER_RES;
ETDLL_API int32 etm_add_peer_resource( u32 task_id, ETM_PEER_RES  * p_resource );

/////��ȡ����ΪETT_LAN�� �����peer�����Ϣ
ETDLL_API int32 etm_get_peer_resource( u32 task_id, ETM_PEER_RES  * p_resource );

typedef  enum etm_lixian_state {ELS_IDLE=0, ELS_RUNNING, ELS_FAILED } ETM_LIXIAN_STATE;
typedef struct t_etm_lixian_info
{
	ETM_LIXIAN_STATE 	_state;
	u32 					_res_num;	//��Դ����
	uint64 				_dl_bytes;	//ͨ��������Դ���ص�������
	u32 					_speed;		
	int32	 			_errcode;
}ETM_LIXIAN_INFO;
/* ��ȡ����������Ϣ */
ETDLL_API int32 etm_get_lixian_info(u32 task_id, u32 file_index,ETM_LIXIAN_INFO * p_lx_info);

/* ���û��ȡ��������id ,lixian_task_id ����0Ϊ��Чid */
ETDLL_API int32 etm_set_lixian_task_id(u32 task_id, u32 file_index,uint64 lixian_task_id);
ETDLL_API int32 etm_get_lixian_task_id(u32 task_id, u32 file_index,uint64 * p_lixian_task_id);

///////////////////////////////////////////////////////////////
////���������ؽӿ�

///// ����������Ϊ���������Ҫ�ڴ�������ʱ����������Ϊ�ֶ���ʼ֮����� (˳������)
ETDLL_API int32 etm_set_task_ad_mode(u32 task_id,Bool is_ad);

///// ��ѯ�����Ƿ�Ϊ�������
ETDLL_API Bool etm_is_ad_task(u32 task_id);

///////////////////////////////////////////////////////////////
/////����ͨ����ؽӿ�

typedef enum etm_hsc_state{EHS_IDLE=0, EHS_RUNNING, EHS_FAILED}ETM_HSC_STATE;
typedef struct t_etm_hsc_info
{
	ETM_HSC_STATE 		_state;
	u32 					_res_num;	//��Դ����
	uint64 				_dl_bytes;	//ͨ������ͨ�����ص�������
	u32 					_speed;		
	int32	 			_errcode;
}ETM_HSC_INFO;
/* ��ȡ����ͨ����Ϣ */
ETDLL_API int32 etm_get_hsc_info(u32 task_id, u32 file_index, ETM_HSC_INFO * p_hsc_info);

/* ��ѯ����ͨ���Ƿ���� */
ETDLL_API int32 etm_is_high_speed_channel_usable( u32 task_id , u32 file_index,Bool * p_usable);

/* ���ø���ͨ�� */
ETDLL_API int32 etm_open_high_speed_channel( u32 task_id , u32 file_index);


///////////////////////////////////////////////////////////////
/////4 torrent �ļ�������E-Mule���ӽ�����ؽӿ�,����torrent�ļ������������ӿڱ���ɶ�ʹ��!

/////4.1 �������ļ�������������Ϣ
ETDLL_API int32 etm_get_torrent_seed_info (const char *seed_path, ETM_ENCODING_MODE encoding_mode,ETM_TORRENT_SEED_INFO **pp_seed_info );

/////4.2 �ͷ�������Ϣ
ETDLL_API int32 etm_release_torrent_seed_info( ETM_TORRENT_SEED_INFO *p_seed_info );

/////4.3 ��E-Mule�����н������ļ���Ϣ
ETDLL_API int32 etm_extract_ed2k_url(char* ed2k, ETM_ED2K_LINK_INFO* info);

/////4.4 �Ӵ��������н������ļ���Ϣ,Ŀǰֻ�ܽ���xtΪbt info hash �Ĵ�����
typedef struct tagETM_MAGNET_INFO
{
	char		_file_name[ETM_MAX_FILE_NAME_LEN];
	uint64		_file_size;
	u8		_file_hash[20];
}ETM_MAGNET_INFO;
ETDLL_API int32 etm_parse_magnet_url(char* magnet, ETM_MAGNET_INFO* info);

/* ��bt�����info_hash ���ɴ�����*/
ETDLL_API int32 etm_generate_magnet_url(const char * info_hash, const char * display_name,uint64 total_size,char * url_buffer ,u32 buffer_len);

/*	����: ��src�е����ݽ���AES(128Bytes)���ܣ����ܺ����ݱ�����des��
 *	aes_key:���ڼ��ܵ�key (ע�⣬���key�ڲ�������MD5���㣬�������MD5ֵ��Ϊ���ܽ��ܵ���Կ!)
 *	src:	����������
 *	src_len:���������ݳ���
 *	des:	���ܺ������
 *	des_len:�����������������ȥ�ĵ���des���ڴ���󳤶�,���������Ǽ��ܺ�����ݳ���
*/
ETDLL_API int32 etm_aes128_encrypt(const char* aes_key,const u8 * src, u32 src_len, u8 * des, u32 * des_len);
/*	����: ��src�е����ݽ���AES(128Bytes)���ܣ����ܺ����ݱ�����des��
 *	aes_key:���ڽ��ܵ�key (ע�⣬���key�ڲ�������MD5���㣬�������MD5ֵ��Ϊ���ܽ��ܵ���Կ!)
 *	src:	����������
 *	src_len:���������ݳ���
 *	des:	���ܺ������
 *	des_len:�����������������ȥ�ĵ���des���ڴ���󳤶�,���������ǽ��ܺ�����ݳ���
*/
ETDLL_API int32 etm_aes128_decrypt(const char* aes_key,const u8 * src, u32 src_len, u8 * des, u32 * des_len);
/**
 * ʹ��gz��buffer����
 * @para: 	src     -����buffer
 *			src_len -����buffer�ĳ���
 * 		  	des     -���buffer
 *			des_len -���buffer�ĳ���
 * 			encode_len -ѹ����ĳ���
 * @return: 0-SUCCESS  else-FAIL
 */
ETDLL_API int32 etm_gzip_encode(u8 * src, u32 src_len, u8 * des, u32 des_len, u32 *encode_len);
/**
 * ʹ��gz��buffer���룬ע�⣬�������des�������ȣ��ڲ������·��䳤��Ϊdes_len���ڴ�
 * @para: 	src     -����buffer
 *			src_len -����buffer�ĳ���
 * 		  	des     -���buffer
 *			des_len -���buffer���ܳ���
 * 			encode_len -��ѹ����Ч���ݵĳ���
 * @return: 0-SUCCESS  else-FAIL
 */
ETDLL_API int32 etm_gzip_decode(u8 * src, u32 src_len, u8 **des, u32 *des_len, u32 *decode_len);

///////////////////////////////////////////////////////////////
/////5 VOD�㲥��ؽӿ�

/////5.1 �����㲥����
ETDLL_API int32 etm_create_vod_task(ETM_CREATE_VOD_TASK * p_param,u32* p_task_id );

/////5.2 ֹͣ�㲥,��������������etm_create_vod_task�ӿڴ����ģ���ô������񽫱�ɾ��,���������etm_create_task�ӿڴ�������ֹֻͣ�㲥
ETDLL_API int32 etm_stop_vod_task (u32 task_id);

/////5.3 ��ȡ�ļ����ݣ���etm_create_task�ӿڴ���������Ҳ�ɵ�������ӿ���ʵ�ֱ����ر߲���
///// ����len(һ�λ�ȡ���ݵĳ���,��λByte)Ϊ16*1024 �Դﵽ���Ż�ȡ�ٶȣ�block_time ��λΪ����,  ȡֵ500,1000����
ETDLL_API int32 etm_read_vod_file(u32 task_id, uint64 start_pos, uint64 len, char *buf, u32 block_time );
ETDLL_API int32 etm_read_bt_vod_file(u32 task_id, u32 file_index, uint64 start_pos, uint64 len, char *buf, u32 block_time );

/////5.4 ��ȡ����(���ʳ���30������ݳ���)�ٷֱȣ���etm_create_task�ӿڴ����������ڱ����ر߲���ʱҲ�ɵ�������ӿ�
ETDLL_API int32 etm_vod_get_buffer_percent(u32 task_id,  u32 * percent );

/////5.5 ��ѯ�㲥�����ʣ�������Ƿ��Ѿ�������ɣ�һ�������ж��Ƿ�Ӧ�ÿ�ʼ������һ����Ӱ��ͬһ����Ӱ����һƬ�Ρ���etm_create_task�ӿڴ����������ڱ����ر߲���ʱҲ�ɵ�������ӿ�
ETDLL_API int32 etm_vod_is_download_finished(u32 task_id, Bool* finished );

/////5.6 ��ȡ�㲥�����ļ�������
ETDLL_API int32 etm_vod_get_bitrate(u32 task_id, u32 file_index, u32* bitrate );

/////5.7 ��ȡ��ǰ���ص�������λ��
ETDLL_API int32 etm_vod_get_download_position(u32 task_id, uint64* position );

/////5.8 ���㲥����תΪ��������,file_retain_timeΪ�����ļ��ڴ��̵ı���ʱ��(����0Ϊ���ñ���),��λ��
ETDLL_API int32 etm_vod_set_download_mode(u32 task_id, Bool download,u32 file_retain_time );

/////5.9 ��ѯ�㲥�����Ƿ�תΪ��������
ETDLL_API int32 etm_vod_get_download_mode(u32 task_id, Bool * download,u32 * file_retain_time );

/////5.10 �㲥ͳ���ϱ�
typedef struct t_etm_vod_report
{
       u32	_vod_play_time_len;//�㲥ʱ��,��λ��Ϊ��
       u32  _vod_play_begin_time;//��ʼ�㲥ʱ��
	u32  _vod_first_buffer_time_len; //�׻���ʱ��
	u32  _vod_play_drag_num;//�϶�����
	u32  _vod_play_total_drag_wait_time;//�϶���ȴ���ʱ��
	u32  _vod_play_max_drag_wait_time;//����϶��ȴ�ʱ��
	u32  _vod_play_min_drag_wait_time;//��С�϶��ȴ�ʱ��
	u32  _vod_play_interrupt_times;//�жϴ���
	u32  _vod_play_total_buffer_time_len;//�жϵȴ���ʱ��
	u32  _vod_play_max_buffer_time_len;//����жϵȴ�ʱ��
	u32  _vod_play_min_buffer_time_len;//��С�жϵȵ�ʱ��
	
	u32 _play_interrupt_1;         // 1�������ж�
	u32 _play_interrupt_2;	        // 2�������ж�
	u32 _play_interrupt_3;		    //6�������ж�
	u32 _play_interrupt_4;		    //10�������ж�
	u32 _play_interrupt_5;		    //15�������ж�
	u32 _play_interrupt_6;		    //15���������ж�
	Bool _is_ad_type;
}ETM_VOD_REPORT;
ETDLL_API int32 etm_vod_report(u32 task_id, ETM_VOD_REPORT * p_report );

/////5.11 ��ѯ�㲥�������ջ�ȡ���ݵķ���������(ֻ�������Ƶ㲥����)
ETDLL_API int32 etm_vod_get_final_server_host(u32 task_id, char  server_host[256] );

/////5.12 ����֪ͨ���ؿ⿪ʼ�㲥(ֻ�������Ƶ㲥����)
ETDLL_API int32 etm_vod_ui_notify_start_play(u32 task_id );

///////////////////////////////////////////////////////////////
/// 6. ����洢����ؽӿ�,ע������������������㲥�������κι�ϵ��һ����������������״�˵���

#define ETM_INVALID_NODE_ID	0

/* whether if create tree while it not exist. */
#define ETM_TF_CREATE		(0x1)
/* read and write (default) */
#define ETM_TF_RDWR		(0x0)
/* read only. */
#define ETM_TF_RDONLY		(0x2)
/* write only. */
#define ETM_TF_WRONLY		(0x4)
#define ETM_TF_MASK       (0xFF)


// �������һ����(flag=ETM_TF_CREATE|ETM_TF_RDWR),����ɹ�*p_tree_id���������ڵ�id
ETDLL_API int32 	etm_open_tree(const char * file_path,int32 flag ,u32 * p_tree_id); 

// �ر�һ����
ETDLL_API int32 	etm_close_tree(u32 tree_id); 

// ɾ��һ����
ETDLL_API int32 	etm_destroy_tree(const char * file_path); 

// �ж����Ƿ����
ETDLL_API Bool 	etm_tree_exist(const char * file_path); 

// ����һ����
ETDLL_API int32 	etm_copy_tree(const char * file_path,const char * new_file_path); 


// ����һ���ڵ�,data_len(��λByte)��С������,�����鲻����256
ETDLL_API int32 	etm_create_node(u32 tree_id,u32 parent_id,const char * name, u32 name_len,void * data, u32 data_len,u32 * p_node_id); 

// ɾ��һ���ڵ㡣ע��:������node_id����֦��������������Ҷ�ڵ�ͬʱɾ��;node_id���ܵ���tree_id,ɾ����Ҫ��etm_destroy_tree
ETDLL_API int32 	etm_delete_node(u32 tree_id,u32 node_id);

// ���ýڵ�����(��������),name_len����С��256 bytes;node_id���ܵ���tree_id
ETDLL_API int32 	etm_set_node_name(u32 tree_id,u32 node_id,const char * name, u32 name_len);

// ��ȡ�ڵ�����;node_id����tree_idʱ���������������
ETDLL_API  const char * etm_get_node_name(u32 tree_id,u32 node_id);

// ���û��޸Ľڵ�����,new_data_len(��λByte)��С������,�����鲻����256;
ETDLL_API int32 	etm_set_node_data(u32 tree_id,u32 node_id,void * new_data, u32 new_data_len);

// ��ȡ�ڵ�����,*buffer_len�ĵ�λ��Byte;
ETDLL_API int32	etm_get_node_data(u32 tree_id,u32 node_id,void * data_buffer,u32 * buffer_len);

// �ƶ��ڵ�
///������ͬ����;node_id���ܵ���tree_id
ETDLL_API int32 	etm_set_node_parent(u32 tree_id,u32 node_id,u32 parent_id);

///ͬһ�������ƶ�;node_id���ܵ���tree_id
ETDLL_API int32 	etm_node_move_up(u32 tree_id,u32 node_id,u32 steps);
ETDLL_API int32 	etm_node_move_down(u32 tree_id,u32 node_id,u32 steps);


// ��ȡ���ڵ�id;node_id���ܵ���tree_id
ETDLL_API u32 	etm_get_node_parent(u32 tree_id,u32 node_id);

// ��ȡ�����ӽڵ�id �б�,*buffer_len�ĵ�λ��sizeof(u32)
ETDLL_API int32	etm_get_node_children(u32 tree_id,u32 node_id,u32 * id_buffer,u32 * buffer_len);

// ��ȡ��һ���ӽڵ�id,�Ҳ�������ETM_INVALID_NODE_ID
ETDLL_API u32	etm_get_first_child(u32 tree_id,u32 parent_id);

// ��ȡ�ֵܽڵ�id;node_id���ܵ���tree_id,�Ҳ�������ETM_INVALID_NODE_ID
ETDLL_API u32	etm_get_next_brother(u32 tree_id,u32 node_id);
ETDLL_API u32	etm_get_prev_brother(u32 tree_id,u32 node_id);

// �������ֲ��ҽڵ�id���������ݲ�֧�ֲַ�ʽ��д������aaa.bbb.ccc��
// ���ص�һ��ƥ��Ľڵ�id���Ҳ�������ETM_INVALID_NODE_ID
ETDLL_API u32	etm_find_first_node_by_name(u32 tree_id,u32 parent_id,const char * name, u32 name_len);

// ������һ��ƥ��Ľڵ�id���Ҳ�������ETM_INVALID_NODE_ID;node_id���ܵ���tree_id
ETDLL_API u32	etm_find_next_node_by_name(u32 tree_id,u32 parent_id,u32 node_id,const char * name, u32 name_len); //ע��node_id��parent_id�Ķ�Ӧ��ϵ
// ������һ��ƥ��Ľڵ�id���Ҳ�������ETM_INVALID_NODE_ID;node_id���ܵ���tree_id
ETDLL_API u32	etm_find_prev_node_by_name(u32 tree_id,u32 parent_id,u32 node_id,const char * name, u32 name_len); //ע��node_id��parent_id�Ķ�Ӧ��ϵ


// ���ݽڵ����ݲ��ҽڵ�,data_len��λByte.

// ���ص�һ��ƥ��Ľڵ�id���Ҳ�������ETM_INVALID_NODE_ID
ETDLL_API u32	etm_find_first_node_by_data(u32 tree_id,u32 parent_id,void * data, u32 data_len);

// ������һ��ƥ��Ľڵ�id���Ҳ�������ETM_INVALID_NODE_ID;node_id���ܵ���tree_id
ETDLL_API u32	etm_find_next_node_by_data(u32 tree_id,u32 parent_id,u32 node_id,void * data, u32 data_len);
// ������һ��ƥ��Ľڵ�id���Ҳ�������ETM_INVALID_NODE_ID;node_id���ܵ���tree_id
ETDLL_API u32	etm_find_prev_node_by_data(u32 tree_id,u32 parent_id,u32 node_id,void * data, u32 data_len);

// ���ݽڵ����ֺ����ݲ��Һ��ʵĽڵ�,data_len��λByte.
// ���ص�һ��ƥ��Ľڵ�id���Ҳ�������ETM_INVALID_NODE_ID
ETDLL_API u32	etm_find_first_node(u32 tree_id,u32 parent_id,const char * name, u32 name_len,void * data, u32 data_len);

// ������һ��ƥ��Ľڵ�id���Ҳ�������ETM_INVALID_NODE_ID;node_id���ܵ���tree_id
ETDLL_API u32	etm_find_next_node(u32 tree_id,u32 parent_id,u32 node_id,const char * name, u32 name_len,void * data, u32 data_len);//ע��node_id��parent_id�Ķ�Ӧ��ϵ
// ������һ��ƥ��Ľڵ�id���Ҳ�������ETM_INVALID_NODE_ID;node_id���ܵ���tree_id
ETDLL_API u32	etm_find_prev_node(u32 tree_id,u32 parent_id,u32 node_id,const char * name, u32 name_len,void * data, u32 data_len);//ע��node_id��parent_id�Ķ�Ӧ��ϵ


//////////////////////////////////////////////////////////////
/////7 ����

/////7.1 ��URL �л���ļ������ļ���С��������ܴ�URL ��ֱ�ӽ����õ���ETM �����ѯѸ�׷��������		[ �ݲ�֧�ֻ�ȡ�ļ���С] 
/// ע��: Ŀǰֻ֧��http://,https://,ftp://��thunder://,ed2k://��ͷ��URL, url_len����С��512
///               ���ֻ��õ��ļ�����file_size��Ϊ NULL �����ֻ��õ��ļ���С��name_buffer��Ϊ NULL
///               ���ڸýӿ�Ϊͬ��(�߳�����)�ӿ�,�����Ҫ����block_time,��λΪ����
ETDLL_API int32 etm_get_file_name_and_size_from_url(const char* url,u32 url_len,char *name_buffer,u32 *name_buffer_len,uint64 *file_size, int32 block_time );

/*--------------------------------------------------------------------------*/
/*           ����URL��cid��filesize ��ѯ�ļ���Ϣ�ӿ�
----------------------------------------------------------------------------*/
typedef struct t_etm_query_shub_result
{
	void* _user_data;
	int32 _result;				/* 0:�ɹ�; ����ֵΪʧ�� */
	uint64 _file_size;
	u8 _cid[20];
	u8 _gcid[20];
	char _file_suffix[16];			/* �ļ���׺ */		
} ETM_QUERY_SHUB_RESULT;
typedef int32 (*ETM_QUERY_SHUB_CALLBACK)(ETM_QUERY_SHUB_RESULT * p_query_result);

typedef struct t_etm_query_shub
{
	const char* _url;						/* ��URLΪ NULL ʱ,�����cid������Ч*/
	const char* _refer_url;
	
	u8 _cid[20];							/* ������cid ��filesize������ѯgcid�Ĺ��� */
	uint64 _file_size;
	
	ETM_QUERY_SHUB_CALLBACK _cb_ptr;	/* ������Ϊ NULL */
	void* _user_data;					
} ETM_QUERY_SHUB;
#define etm_query_shub_by_url_or_cid etm_query_shub_by_url
ETDLL_API int32 etm_query_shub_by_url(ETM_QUERY_SHUB * p_param,u32 * p_action_id);
ETDLL_API int32 etm_cancel_query_shub(u32 action_id);

/* ��GCID ,CID,filesize,filename ƴ�ճ�һ����http://pubnet.sandai.net:8080/0/ ��ͷ�Ŀ���ר��URL 
	ע��:
	1.gcid��cid�����ǳ���Ϊ40 bytes��16���������ַ���;
	2.filename����ΪUTF8����,�м�����пո�,������ǰ��%20�滻;
	3. url_buffer��������ż������ɵ�URL���ڴ�,*url_buffer_len����С��1024bytes.	

	���ɵĵ㲥url��ʽ����/�ָ��ֶ�
	�汾��0
	http://host:port/version/gcid/cid/file_size/head_size/index_size/bitrate/packet_size/packet_count/tail_pos/url_mid/filename
*/
ETDLL_API int32 etm_generate_kankan_url(const char* gcid,const char* cid,uint64 file_size,const char* file_name, char *url_buffer,u32 *url_buffer_len );

/////7.2 ����΢����������,�ص�������resultΪ0��ʾ�ɹ����˽ӿ��ѱ�etm_http_get ��etm_http_get_file���,�����ʹ��!��
// typedef int32 ( *ETM_NOTIFY_MINI_TASK_FINISHED)(void * user_data, int32 result,void * recv_buffer,u32  recvd_len,void * p_header,u8 * send_data);
ETDLL_API int32 etm_get_mini_file_from_url(ETM_MINI_TASK * p_mini_param );

/////7.3 ����΢���ϴ�����,�ص�������resultΪ0��ʾ�ɹ����˽ӿ��ѱ�etm_http_post���,�����ʹ��!��
// typedef int32 ( *ETM_NOTIFY_MINI_TASK_FINISHED)(void * user_data, int32 result,void * recv_buffer,u32  recvd_len,void * p_header,u8 * send_data);
ETDLL_API int32 etm_post_mini_file_to_url(ETM_MINI_TASK * p_mini_param );

/////7.4 ǿ��ȡ��΢�����񡾴˽ӿ��ѱ�etm_http_cancel���,�����ʹ��!��
ETDLL_API int32 etm_cancel_mini_task(u32 mini_id );

//////////////////////////////////////////////////////////////
/* HTTP�Ự�ӿڵĻص���������*/
typedef enum t_etm_http_cb_type
{
	EHCT_NOTIFY_RESPN=0, 
	EHCT_GET_SEND_DATA, 	// Just for POST
	EHCT_NOTIFY_SENT_DATA, 	// Just for POST
	EHCT_GET_RECV_BUFFER,
	EHCT_PUT_RECVED_DATA,
	EHCT_NOTIFY_FINISHED
} ETM_HTTP_CB_TYPE;
typedef struct t_etm_http_call_back
{
	u32 _http_id;
	void * _user_data;			/* �û����� */
	ETM_HTTP_CB_TYPE _type;
	void * _header;				/* _type==EHCT_NOTIFY_RESPNʱ��Ч,ָ��http��Ӧͷ,����etm_get_http_header_value��ȡ�������ϸ��Ϣ*/
	
	u8 ** _send_data;			/* _type==EHCT_GET_SEND_DATAʱ��Ч, ���ݷֲ��ϴ�ʱ,ָ����Ҫ�ϴ�������*/
	u32  * _send_data_len;		/* ��Ҫ�ϴ������ݳ��� */
	
	u8 * _sent_data;			/* _type==EHCT_NOTIFY_SENT_DATAʱ��Ч, ���ݷֲ��ϴ�ʱ,ָ���Ѿ��ϴ�������,�м�����_sent_data_len�Ƿ�Ϊ��,���涼Ҫ�����ͷŸ��ڴ�*/
	u32   _sent_data_len;		/* �Ѿ��ϴ������ݳ��� */
	
	void ** _recv_buffer;			/* _type==EHCT_GET_RECV_BUFFERʱ��Ч, ���ݷֲ�����ʱ,ָ�����ڽ������ݵĻ���*/
	u32  * _recv_buffer_len;		/* �����С ,����Ҫ16384 byte !*/
	
	u8 * _recved_data;			/* _type==EHCT_PUT_RECVED_DATAʱ��Ч, ���ݷֲ�����ʱ,ָ���Ѿ��յ�������,�м�����_recved_data_len�Ƿ�Ϊ��,���涼Ҫ�����ͷŸ��ڴ�*/
	u32  _recved_data_len;		/* �Ѿ��յ������ݳ��� */

	int32 _result;					/* _type==EHCT_NOTIFY_FINISHEDʱ��Ч, 0Ϊ�ɹ�*/
} ETM_HTTP_CALL_BACK;
typedef int32 ( *ETM_HTTP_CALL_BACK_FUNCTION)(ETM_HTTP_CALL_BACK * p_http_call_back_param); 
/* HTTP�Ự�ӿڵ�������� */
typedef struct t_etm_http_get
{
	char* _url;					/* ֻ֧��"http://" ��ͷ��url  */
	u32 _url_len;
	
	char * _ref_url;				/* ����ҳ��URL*/
	u32  _ref_url_len;		
	
	char * _cookie;			
	u32  _cookie_len;		
	
	uint64  _range_from;			/* RANGE ��ʼλ��,���ݲ�֧�֡�*/
	uint64  _range_to;			/* RANGE ����λ��,���ݲ�֧�֡�*/
	
	Bool  _accept_gzip;			/* �Ƿ����ѹ���ļ� */
	
	void * _recv_buffer;			/* ���ڽ��շ��������ص���Ӧ���ݵĻ���*/
	u32  _recv_buffer_size;		/* _recv_buffer �����С,����Ҫ16384 byte !*/
	
	ETM_HTTP_CALL_BACK_FUNCTION _callback_fun;			/* ������ɺ�Ļص����� : typedef int32 ( *ETM_HTTP_CALL_BACK_FUNCTION)(ETM_HTTP_CALL_BACK * p_http_call_back_param); */
	void * _user_data;			/* �û����� */
	u32  _timeout;				/* ��ʱ,��λ��,����0ʱ180�볬ʱ*/
} ETM_HTTP_GET;
typedef struct t_etm_http_post
{
	char* _url;					/* ֻ֧��"http://" ��ͷ��url  */
	u32 _url_len;
	
	char * _ref_url;				/* ����ҳ��URL*/
	u32  _ref_url_len;		
	
	char * _cookie;			
	u32  _cookie_len;		
	
	uint64  _content_len;			/* Content-Length:*/
	
	Bool  _send_gzip;			/* �Ƿ���ѹ������*/
	Bool  _accept_gzip;			/* �Ƿ����ѹ������*/
	
	u8 * _send_data;				/* ָ����Ҫ�ϴ�������*/
	u32  _send_data_len;			/* ��Ҫ�ϴ������ݴ�С*/
	
	void * _recv_buffer;			/* ���ڽ��շ��������ص���Ӧ���ݵĻ���*/
	u32  _recv_buffer_size;		/* _recv_buffer �����С,����Ҫ16384 byte !*/

	ETM_HTTP_CALL_BACK_FUNCTION _callback_fun;			/* ������ɺ�Ļص����� : typedef int32 ( *ETM_HTTP_CALL_BACK_FUNCTION)(ETM_HTTP_CALL_BACK * p_http_call_back_param); */
	void * _user_data;			/* �û����� */
	u32  _timeout;				/* ��ʱ,��λ��,����0ʱ180�볬ʱ*/
} ETM_HTTP_POST;
typedef struct t_etm_http_get_file
{
	char* _url;					/* ֻ֧��"http://" ��ͷ��url  */
	u32 _url_len;
	
	char * _ref_url;				/* ����ҳ��URL*/
	u32  _ref_url_len;		
	
	char * _cookie;			
	u32  _cookie_len;		
	
	uint64  _range_from;			/* RANGE ��ʼλ��,���ݲ�֧�֡�*/
	uint64  _range_to;			/* RANGE ����λ��,���ݲ�֧�֡�*/
	
	Bool  _accept_gzip;			/* �Ƿ����ѹ���ļ� */
	
	char _file_path[ETM_MAX_FILE_PATH_LEN]; 				/* _is_file=TRUEʱ, ��Ҫ�ϴ������ص��ļ��洢·����������ʵ���� */
	u32 _file_path_len;			/* _is_file=TRUEʱ, ��Ҫ�ϴ������ص��ļ��洢·���ĳ���*/
	char _file_name[ETM_MAX_FILE_NAME_LEN];			/* _is_file=TRUEʱ, ��Ҫ�ϴ������ص��ļ���*/
	u32 _file_name_len; 			/* _is_file=TRUEʱ, ��Ҫ�ϴ������ص��ļ�������*/
	
	ETM_HTTP_CALL_BACK_FUNCTION _callback_fun;			/* ������ɺ�Ļص����� : typedef int32 ( *ETM_HTTP_CALL_BACK_FUNCTION)(ETM_HTTP_CALL_BACK * p_http_call_back_param); */
	void * _user_data;			/* �û����� */
	u32  _timeout;				/* ��ʱ,��λ��,����0ʱ180�볬ʱ*/
} ETM_HTTP_GET_FILE;

typedef struct t_etm_http_post_file
{
	char* _url;					/* ֻ֧��"http://" ��ͷ��url  */
	u32 _url_len;
	
	char * _ref_url;				/* ����ҳ��URL*/
	u32  _ref_url_len;		
	
	char * _cookie;			
	u32  _cookie_len;		
	
	uint64  _range_from;			/* RANGE ��ʼλ��,���ݲ�֧�֡�*/
	uint64  _range_to;			/* RANGE ����λ��,���ݲ�֧�֡�*/
	
	Bool  _accept_gzip;			/* �Ƿ����ѹ���ļ� */
	
	char _file_path[ETM_MAX_FILE_PATH_LEN]; 				/*  ��Ҫ�ϴ������ص��ļ��洢·����������ʵ���� */
	u32 _file_path_len;			/* ��Ҫ�ϴ������ص��ļ��洢·���ĳ���*/
	char _file_name[ETM_MAX_FILE_NAME_LEN];			/*  ��Ҫ�ϴ������ص��ļ���*/
	u32 _file_name_len; 			/*  ��Ҫ�ϴ������ص��ļ�������*/
	
	ETM_HTTP_CALL_BACK_FUNCTION _callback_fun;			/* ������ɺ�Ļص����� : typedef int32 ( *ETM_HTTP_CALL_BACK_FUNCTION)(ETM_HTTP_CALL_BACK * p_http_call_back_param); */
	void * _user_data;			/* �û����� */
	u32  _timeout;				/* ��ʱ,��λ��,����0ʱ180�볬ʱ*/
} ETM_HTTP_POST_FILE;
/////7.6 ����http get �Ự
// typedef int32 ( *ETM_HTTP_CALL_BACK_FUNCTION)(ETM_HTTP_CALL_BACK * p_http_call_back_param);
ETDLL_API int32 etm_http_get(ETM_HTTP_GET * p_http_get ,u32 * http_id);
ETDLL_API int32 etm_http_get_file(ETM_HTTP_GET_FILE * p_http_get_file ,u32 * http_id);

/////7.7 ����http post �Ự
// typedef int32 ( *ETM_HTTP_CALL_BACK_FUNCTION)(ETM_HTTP_CALL_BACK * p_http_call_back_param);
ETDLL_API int32 etm_http_post(ETM_HTTP_POST * p_http_post ,u32 * http_id );
ETDLL_API int32 etm_http_post_file(ETM_HTTP_POST_FILE * p_http_post_file ,u32 * http_id );

/////7.8 ǿ��ȡ��http�Ự,����������Ǳ���ģ�ֻ��UI���ڻص�������ִ��֮ǰȡ���ûỰʱ�ŵ���
ETDLL_API int32 etm_http_cancel(u32 http_id );

/////7.9 ��ȡhttpͷ���ֶ�ֵ,�ú�������ֱ���ڻص����������
/* httpͷ�ֶ�*/
typedef enum t_etm_http_header_field
{
	EHHV_STATUS_CODE=0, 
	EHHV_LAST_MODIFY_TIME, 
	EHHV_COOKIE,
	EHHV_CONTENT_ENCODING,
	EHHV_CONTENT_LENGTH
} ETM_HTTP_HEADER_FIELD;
ETDLL_API char * etm_get_http_header_value(void * p_header,ETM_HTTP_HEADER_FIELD header_field );
//////////////////////////////////////////////////////////////
/////8 drm


/* 
 *  ����drm֤��Ĵ��·��
 *  certificate_path:  drm֤��·��
 * ����ֵ: 0     �ɹ�
           4201  certificate_path�Ƿ�(·�������ڻ�·�����ȴ���255)
 */
ETDLL_API int32 etm_set_certificate_path(const char *certificate_path);


/* 
 *  ��xlmv�ļ�,��ȡ�����ļ���drm_id
 *  p_file_full_path:  xlmv�ļ�ȫ·��
 *  p_drm_id: ���ص�drm_id(һ���ļ���Ӧһ��,�Ҳ����ظ�)
 *  p_origin_file_size: ���ص�drmԭʼ�ļ��Ĵ�С
 * ����ֵ: 0     �ɹ�
           22529 drm�ļ�������.
           22530 drm�ļ���ʧ��.
           22531 drm�ļ���ȡʧ��.
           22532 drm�ļ���ʽ����.
           22537 drm֤���ʽ����,���߷Ƿ�֤���ļ�.
           22538 ��֧��openssl��,�޷������ļ�
 */
ETDLL_API int32 etm_open_drm_file(const char *p_file_full_path, u32 *p_drm_id, uint64 *p_origin_file_size);


/* 
 *  xlmv�ļ���Ӧ֤���ļ��Ƿ�Ϸ�����
 *  drm_id:  xlmv�ļ���Ӧ��drm_id
 *  p_is_ok: �Ƿ�֤���ļ��Ϸ�����
 * ����ֵ: 0     �ɹ�
           22534 ��Ч��drm_id.
           22535 ֤������ʧ��.
           22536 drm�ڲ��߼�����.
           22537 drm֤���ʽ����,���߷Ƿ�֤���ļ�.
           22538 ��֧��openssl��,�޷������ļ�
           
           (���´�������Ҫ���ظ��������û�֪��)
           22628 ���ӵ�pid�󶨵��û���������
           22629 �û�û�й���ָ����ӰƬ
           22630 ��Ʒ�Ѿ��¼ܣ��û��������ٹ�����������Ѿ�����֮���¼ܵģ��û����ǿ��Բ���
           22631 �󶨵ĺ�������������
           22632 ��Կδ�ҵ���������֤�����ɹ����г������쳣
           22633 ������æ��һ����ϵͳѹ�����󣬵��²�ѯ���߰󶨴����쳣�������������쳣���ͻ��˿��Կ������ԡ�
 */

ETDLL_API int32 etm_is_certificate_ok(u32 drm_id, Bool *p_is_ok);


/* 
 *  xlmv�ļ���ȡ����
 *  drm_id:      xlmv�ļ���Ӧ��drm_id
 *  p_buffer:    ��ȡ����buffer
 *  size:        ��ȡ����buffer�Ĵ�С
 *  file_pos:    ��ȡ���ݵ���ʼλ��
 *  p_read_size: ʵ�ʶ�ȡ���ݴ�С
 * ����ֵ: 0     �ɹ�
           22533 ֤����δ�ɹ�
           22534 ��Ч��drm_id.
           22536 drm�ڲ��߼�����.
           22537 drm֤���ʽ����,���߷Ƿ�֤���ļ�.
 */
ETDLL_API int32 etm_read_drm_file(u32 drm_id, char *p_buffer, u32 size, 
    uint64 file_pos, u32 *p_read_size );


/* 
 *  �ر�xlmv�ļ�
 *  drm_id:      xlmv�ļ���Ӧ��drm_id
 * ����ֵ: 0     �ɹ�
           22534 ��Ч��drm_id.
 */
ETDLL_API int32 etm_close_drm_file(u32 drm_id);


#define ETM_OPENSSL_IDX_COUNT			    (7)

/* function index */
#define ETM_OPENSSL_D2I_RSAPUBLICKEY_IDX    (0)
#define ETM_OPENSSL_RSA_SIZE_IDX            (1)
#define ETM_OPENSSL_BIO_NEW_MEM_BUF_IDX     (2)
#define ETM_OPENSSL_D2I_RSA_PUBKEY_BIO_IDX  (3)
#define ETM_OPENSSL_RSA_PUBLIC_DECRYPT_IDX  (4)
#define ETM_OPENSSL_RSA_FREE_IDX            (5)
#define ETM_OPENSSL_BIO_FREE_IDX            (6)

/* 
 *  drm��Ҫ�õ�openssl������rsa������صĺ���,��Ҫ������ö�Ӧ�Ļص���������.(���������ؿ�����ǰ����)
 *  fun_count:    ��Ҫ�Ļص���������,�������ET_OPENSSL_IDX_COUNT
 *  fun_ptr_table:    �ص���������,������һ��void*������
 * ����ֵ: 0     �ɹ�
           3273  ��������,fun_count��ΪET_OPENSSL_IDX_COUNT��fun_ptr_tableΪNULL������Ԫ��ΪNULL

 *  ��ţ�0 (ET_OPENSSL_D2I_RSAPUBLICKEY_IDX)  ����ԭ�ͣ�typedef RSA * (*et_func_d2i_RSAPublicKey)(RSA **a, _u8 **pp, _int32 length);
 *  ˵������Ӧopenssl���d2i_RSAPublicKey����

 *  ��ţ�1 (ET_OPENSSL_RSA_SIZE_IDX)  ����ԭ�ͣ�typedef _int32 (*et_func_openssli_RSA_size)(const RSA *rsa);
 *  ˵������Ӧopenssl���RSA_size����

 *  ��ţ�2 (ET_OPENSSL_BIO_NEW_MEM_BUF_IDX)  ����ԭ�ͣ�typedef BIO *(*et_func_BIO_new_mem_buf)(void *buf, _int32 len);
 *  ˵������Ӧopenssl���BIO_new_mem_buf����

 *  ��ţ�3 (ET_OPENSSL_D2I_RSA_PUBKEY_BIO_IDX)  ����ԭ�ͣ�typedef RSA *(*et_func_d2i_RSA_PUBKEY_bio)(BIO *bp,RSA **rsa);
 *  ˵������Ӧopenssl���d2i_RSA_PUBKEY_bio����

 *  ��ţ�4 (ET_OPENSSL_RSA_PUBLIC_DECRYPT_IDX)  ����ԭ�ͣ�typedef _int32(*et_func_RSA_public_decrypt)(_int32 flen, const _u8 *from, _u8 *to, RSA *rsa, _int32 padding);
 *  ˵������Ӧopenssl���RSA_public_decrypt����

 *  ��ţ�5 (ET_OPENSSL_RSA_FREE_IDX)  ����ԭ�ͣ�typedef void(*et_func_RSA_free)( RSA *r );
 *  ˵������Ӧopenssl���RSA_free����

 *  ��ţ�6 (ET_OPENSSL_BIO_FREE_IDX)  ����ԭ�ͣ�typedef void(*et_func_BIO_free)( BIO *a );
 *  ˵������Ӧopenssl���BIO_free����

 */

ETDLL_API int32 etm_set_openssl_rsa_interface(int32 fun_count, void *fun_ptr_table);

//////////////////////////////////////////////////////////////
/////10 ��ѯ�������Ӧ�ļ�Ҫ������ע��1~1024�������ڲ�ͬ��ƽ̨����������ͬ
ETDLL_API const char * etm_get_error_code_description(int32 error_code);


//////////////////////////////////////////////////////////////
/////11 json�ӿ�
/* json����ͨ��������һ��json���ã������buffer�ϵõ�һ��json���
 *p_input_json  ����json����
 *p_output_json_buffer  ���json��buffer�������ά��buffer
 *p_buffer_len  ��ʼ��ֵΪbuffer�Ĵ�С�������������105476,  p_buffer_len������Ҫ��buffer����
 * ����ֵ: 0     �ɹ�
 *                    105476    ���buffer���Ȳ���
 */
ETDLL_API int32 etm_mc_json_call( const char *p_input_json, char *p_output_json_buffer, u32 *p_buffer_len );



//////////////////////////////////////////////////////////////
/////12 ָ����������������������ؽӿ�
//������Э������
typedef enum t_etm_server_type
{
         EST_HTTP=0, 
         EST_FTP            //�ݲ�֧�֣���Ҫ�����Ժ���չ��
} ETM_SERVER_TYPE;

//��������Ϣ
typedef struct t_etm_server
{
         ETM_SERVER_TYPE _type;
         u32 _ip;
         u32 _port;
		 u32 _file_num;
		 Bool _has_password;
		 char _ip_addr[16];
		 char _pc_name[64];
         char _description[256];  // ����������������"version=xl7.xxx&pc_name; file_num=999; peerid=864046003239850V; ip=192.168.x.x; tcp_port=8080; udp_port=10373; peer_capability=61546"
} ETM_SERVER;

//�ҵ��������Ļص��������Ͷ���
typedef int32 ( *ETM_FIND_SERVER_CALL_BACK_FUNCTION)(ETM_SERVER * p_server);

//�������֪ͨ�ص��������Ͷ��壬result����0Ϊ�ɹ�������ֵΪʧ��
typedef int32 ( *ETM_SEARCH_FINISHED_CALL_BACK_FUNCTION)(int32 result);

//�����������
typedef struct t_etm_search_server
{
         ETM_SERVER_TYPE _type;
         u32 _ip_from;           /* ��ʼip����������"192.168.1.1"Ϊ3232235777����0��ʾֻɨ���뱾��ipͬһ���������� */
         u32 _ip_to;               /* ����ip����������"192.168.1.254"Ϊ3232236030����0��ʾֻɨ���뱾��ipͬһ����������*/
         int32 _ip_step;         /* ɨ�貽��*/
         
         u32 _port_from;         /*��ʼ�˿ڣ�������*/
         u32 _port_to;           /* �����˿ڣ�������*/
         int32 _port_step;    /* �˿�ɨ�貽��*/
         
         ETM_FIND_SERVER_CALL_BACK_FUNCTION _find_server_callback_fun;             /* ÿ�ҵ�һ����������etm�ͻص�һ�¸ú��������÷���������Ϣ����UI */
         ETM_SEARCH_FINISHED_CALL_BACK_FUNCTION _search_finished_callback_fun;     /* ������� */
} ETM_SEARCH_SERVER;

//��ʼ����
ETDLL_API int32 etm_start_search_server(ETM_SEARCH_SERVER * p_search);

//ֹͣ����,����ýӿ���ETM�ص�_search_finished_callback_fun֮ǰ���ã���ETM�����ٻص�_search_finished_callback_fun
ETDLL_API int32 etm_stop_search_server(void);
ETDLL_API int32 etm_restart_search_server(void);

//////////////////////////////////////////////////////////////
/////14 ���߿ռ���ؽӿ�
//	ע��:����char *�ַ�����ΪUTF8��ʽ!

/* �����ؿ�����Ѹ���û��ʺ���Ϣ,new_user_name���������ִ�  */
ETDLL_API int32 etm_lixian_set_user_info(uint64 user_id,const char * new_user_name,const char * old_user_name,int32 vip_level,const char * session_id,char* jumpkey, u32 jumpkey_len);

/* �û�����etm_member_relogin���µ�¼֮���轫���µ�sessionid�������õ�����ģ��  */
ETDLL_API int32 etm_lixian_set_sessionid(const char * session_id);

/* �����ؿ�����jump key*/
ETDLL_API int32 etm_lixian_set_jumpkey(char* jumpkey, u32 jumpkey_len);

/* ��ȡ�û����߿ռ���Ϣ,��λByte */
ETDLL_API int32 etm_lixian_get_space(uint64 * p_max_space,uint64 * p_available_space);

/* ��ȡ�㲥ʱ��cookie */
ETDLL_API const char * etm_lixian_get_cookie(void);

/* ȡ��ĳ���첽���� */
ETDLL_API int32 etm_lixian_cancel(u32 action_id);

/* �˳����߿ռ� */
ETDLL_API int32 etm_lixian_logout(void);

/* �����������񷵻������Ϣ*/
typedef struct t_elx_login_result
{
	u32 _action_id;
	void * _user_data;							/* �û����� */
	int32 _result;								/*	0:�ɹ�������ֵ:ʧ��*/

	/* �û����*/
	u8 _user_type;								/* �û����� */
	u8 _vip_level;								/* �û�vip�ȼ� */
	uint64 _available_space;					/* ���ÿռ� */
	uint64 _max_space;							/* �û��ռ� */
	uint64 _max_task_num;						/* �û���������� */
	uint64 _current_task_num;					/* ��ǰ������ */

} ELX_LOGIN_RESULT;
typedef int32 ( *ELX_LIXIAN_LOGIN_CALLBACK)(ELX_LOGIN_RESULT * p_result);
/* ���ߵ�¼����*/
typedef struct t_elx_login_info
{
	uint64 _user_id;
	char * _new_user_name;
	char * _old_user_name;
	int32 _vip_level;
	char * _session_id;

	char* _jumpkey;
	u32   _jumpkey_len;
	
	void * _user_data;		
	ELX_LIXIAN_LOGIN_CALLBACK _callback_fun;
} ELX_LOGIN_INFO;

/* ����:
	1. �����ؿ�����Ѹ���û��ʺ���Ϣ,new_user_name���������ִ���
	2. �����ؿ�����jump key��
	3. ��¼���߿ռ䡣
   ����: ELX_LOGIN_INFO
	�û���¼֮���ȡ������Ϣ���ݹ����������ߵ�¼��
*/
ETDLL_API int32 etm_lixian_login(ELX_LOGIN_INFO* p_param, u32 * p_action_id);
/* ����: ��ȡ�����б��Ƿ��л���(��Ҫ�����ж��Ƿ��һ����ȡ�����б�)
   ����: 
   		userid: �û�id
   		is_cache: �Ƿ��������б���
*/
ETDLL_API int32 etm_lixian_has_list_cache(uint64 userid, BOOL *is_cache);

/* �����߿ռ��ύ��������*/
/* ��������״̬*/

typedef enum t_etm_lx_task_state
{
	ELXS_WAITTING =1, 
	ELXS_RUNNING  =2, 
	ELXS_PAUSED   =4,
	ELXS_SUCCESS  =8, 
	ELXS_FAILED   =16, 
	ELXS_OVERDUE  =32,
	ELXS_DELETED  =64
} ELX_TASK_STATE;

/* ������������ */
typedef enum t_elx_task_type
{
	ELXT_UNKNOWN = 0, 				
	ELXT_HTTP , 					
	ELXT_FTP,				
	ELXT_BT,			// ����	
	ELXT_EMULE,				
	ELXT_BT_ALL,				
	ELXT_BT_FILE,				
	ELXT_SHOP				
} ELX_TASK_TYPE;
/* �����������񷵻������Ϣ*/
typedef struct t_elx_create_task_result
{
	u32 _action_id;
	void * _user_data;							/* �û����� */
	int32 _result;									/*	0:�ɹ�������ֵ:ʧ��*/

	/* �û����*/
	uint64 _available_space;						/* ���ÿռ� */
	uint64 _max_space;							/* �û��ռ� */
	uint64 _max_task_num;							/* �û���������� */
	uint64 _current_task_num;						/* ��ǰ������ */

	/* �۷����*/
	BOOL _is_goldbean_converted;					/* �Ƿ�ת���˽�*/
	uint64 _goldbean_total_num;					/* ���ܶ�*/
	uint64 _goldbean_need_num;					/* ��Ҫ�Ľ�����*/
	uint64 _goldbean_get_space;					/* ת��������õĿռ��С*/
	uint64 _silverbean_total_num;					/* ��������*/
	uint64 _silverbaen_need_num;					/* ��Ҫ����������*/

	/* �������*/
	uint64 _task_id;
	uint64 _file_size; 
	ELX_TASK_STATE _status;
	ELX_TASK_TYPE  _type;
	u32 _progress;								/*������ȣ�����100��Ϊ�ٷֱȣ�10000��100%*/
} ELX_CREATE_TASK_RESULT;

typedef int32 ( *ELX_CREATE_TASK_CALLBACK)(ELX_CREATE_TASK_RESULT * p_result);

/* ���������������*/
typedef struct t_elx_create_task_info
{
	char _url[ETM_MAX_URL_LEN];
	char _ref_url[ETM_MAX_URL_LEN];
	char _task_name[ETM_MAX_FILE_NAME_LEN];
	u8 _cid[20];			
	u8 _gcid[20];
	uint64 _file_size;
	BOOL _is_auto_charge;					/* ���ռ䲻�����н�ʱ�Ƿ��Զ��۽� */
	
	void * _user_data;		
	ELX_CREATE_TASK_CALLBACK _callback_fun;
} ELX_CREATE_TASK_INFO;

/* �����߿ռ��ύ��������,ʹ�øýӿ�ǰ�����ù�jump key*/
ETDLL_API int32 etm_lixian_create_task(ELX_CREATE_TASK_INFO * p_param,u32 * p_action_id);


/* ��Torrent�����ļ������������bt�����������*/
typedef struct t_elx_create_bt_task_info
{
	char* _title;							/* ����ı���,utf8����,�255�ֽ�,��NULL��ʾֱ����torrent�����title ���ô��������infohash������*/
	char* _seed_file_full_path; 			/* ��torrent��������ʱ�������������������ļ�(*.torrent) ȫ·�� */
	char* _magnet_url;					/* �ô�������������ʱ��������Ǵ�����*/
	u32 _file_num;						/* ��Ҫ���ص����ļ�����*/
	u32* _download_file_index_array; 	/* ��Ҫ���ص����ļ�����б�ΪNULL ��ʾ����������Ч�����ļ�*/

	BOOL _is_auto_charge;				/* ���ռ䲻�����н�ʱ�Ƿ��Զ��۽� */
	
	void * _user_data;		
	ELX_CREATE_TASK_CALLBACK _callback_fun;
} ELX_CREATE_BT_TASK_INFO;

/* �����߿ռ��ύbt ��������,ʹ�øýӿ�ǰ�����ù�jump key*/
ETDLL_API int32 etm_lixian_create_bt_task(ELX_CREATE_BT_TASK_INFO * p_param,u32 * p_action_id);



/* ���������ļ����� */
typedef enum t_elx_file_type
{
	ELXF_UNKNOWN = 0, 				
	ELXF_VEDIO , 					
	ELXF_PIC , 					
	ELXF_AUDIO , 					
	ELXF_ZIP , 		/* ����ѹ���ĵ�,��zip,rar,tar.gz... */			
	ELXF_TXT , 					
	ELXF_APP,		/* �������,��exe,apk,dmg,rpm... */			
	ELXF_OTHER					
} ELX_FILE_TYPE;


/* ����������Ϣ */
typedef struct t_elx_task_info
{
	uint64 _task_id;
	ELX_TASK_TYPE _type; 						
	ELX_TASK_STATE _state;
	char _name[ETM_MAX_FILE_NAME_LEN];		/* ��������,utf8����*/
	uint64 _size;								/* �����ļ��Ĵ�С,BT����ʱΪ�������ļ����ܴ�С,��λbyte*/
	int32 _progress;							/* �����������������,��Ϊ���ؽ���*/

	char _file_suffix[16]; 						/* ��BT������ļ���׺��,��: txt,exe,flv,avi,mp3...*/ 
	u8 _cid[20];								/* ��BT������ļ�cid ,bt����Ϊinfo hash */
	u8 _gcid[20];								/* ��BT������ļ�gcid */
	Bool _vod;								/* ��BT������ļ��Ƿ�ɵ㲥 */
	char _url[ETM_MAX_URL_LEN];				/* ��BT���������URL,utf8����*/
	char _cookie[ETM_MAX_URL_LEN];			/* ��BT���������URL ��Ӧ��cookie */

	u32 _sub_file_num;						/* BT�����б�ʾ���������ļ�����(����BT���������ļ���) */
	u32 _finished_file_num;					/* BT�����б�ʾ�������������(ʧ�ܻ�ɹ�)�����ļ�����*/

	u32 _left_live_time;						/* ʣ������ */
	char _origin_url[ETM_MAX_URL_LEN];				/* ��BT�����ԭʼ����URL*/

	u32 _origin_url_len;						/* ��BT�����ԭʼ����URL ����*/
	u32 _origin_url_hash;					/* ��BT�����ԭʼ����URL ��hashֵ*/
} ELX_TASK_INFO;

/* ��ȡ�б���Ӧ,�ڴ������ؿ��ͷ�,UI��Ҫ����һ�����л���UI�߳��д��� */
typedef struct t_elx_task_list
{
	u32 _action_id;
	void * _user_data;				/* �û����� */
	int32 _result;						/*	0:�ɹ�,����ֵ:ʧ��	*/
	u32 _total_task_num;			/* ���û����߿ռ��ڵ����������*/
	u32 _task_num;					/* �˴ν�����õ�������� */
	uint64 _total_space;			/* �û������ܿռ䣬��λ�ֽ� */
	uint64 _available_space;			/* �û����߿��ÿռ䣬��λ�ֽ� */
	ELX_TASK_INFO * _task_array;		/* ע��:�˲���������! ������Ϣ������etm_lixian_get_task_info�ӿڻ�ȡ */
} ELX_TASK_LIST;
typedef int32 ( *ELX_GET_TASK_LIST_CALLBACK)(ELX_TASK_LIST * p_task_list);

/* ��ȡ���߿ռ������б���������*/
typedef struct t_elx_get_task_ls
{
	ELX_FILE_TYPE _file_type;			/*  Ϊָ����ȡ�����ض����͵��ļ��������б�*/
	int32 _file_status;				/*  Ϊָ�������ض�״̬���ļ��������б�,0:ȫ����1:��ѯ��������������2:���*/
	int32 _offset;						/*  ��ʾ���������̫��ʱ������ȡ��ƫ��ֵ,��0��ʼ*/
	int32 _max_task_num;			/*  Ϊ�˴λ�ȡ����������   */
	
	void * _user_data;				/* �û����� */
	ELX_GET_TASK_LIST_CALLBACK _callback;
} ELX_GET_TASK_LIST;

/* ��ȡ���߿ռ������б�
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
ETDLL_API int32 etm_lixian_get_task_list(ELX_GET_TASK_LIST * p_param,u32 * p_action_id);


typedef struct t_elx_od_or_del_task_list
{
	u32 _action_id;
	void * _user_data;				/* �û����� */
	int32 _result;						/*	0:�ɹ�,����ֵ:ʧ��	*/
	u32 _task_num;					/* �˴ν�����õ�������� */
} ELX_OD_OR_DEL_TASK_LIST;
typedef int32 ( *ELX_GET_OD_OR_DEL_TASK_LIST_CALLBACK)(ELX_OD_OR_DEL_TASK_LIST * p_task_list);
/* ��ȡ���߿ռ���ڻ�����ɾ�������б���������*/
typedef struct t_elx_od_or_del_get_task_ls
{
	int32 _task_type;				/*  0:��ɾ����1:����*/
	int32 _page_offset;			/*  ��ʾ���������̫��ʱ������ȡ��ƫ��ֵ,��0��ʼ*/
	int32 _max_task_num;			/*  ÿҳ�������� */
	
	void * _user_data;				/* �û����� */
	ELX_GET_OD_OR_DEL_TASK_LIST_CALLBACK _callback;
} ELX_GET_OD_OR_DEL_TASK_LIST;

/* ��ȡ���߿ռ���ڻ�����ɾ��������б�
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
ETDLL_API int32 etm_lixian_get_overdue_or_deleted_task_list(ELX_GET_OD_OR_DEL_TASK_LIST * p_param,u32 * p_action_id);

/* ����: ��������Ĳ�ͬ״̬��ȡ��������id �б�
   ����: 
     state: ��Χ 0-127, �������¼���״ֵ̬������ȡ��Ҫ��ȡ�ļ���״ֵ̬	
     state = 0��ʾ��ȡ����״̬id�б�
	    	ELXS_WAITTING =1, 
			ELXS_RUNNING  =2, 
			ELXS_PAUSED   =4,
			ELXS_SUCCESS  =8, 
			ELXS_FAILED   =16, 
			ELXS_OVERDUE  =32,
			ELXS_DELETED  =64
     id_array_buffer: ��֪��ĳ״̬��Ӧ��id��Ŀʱ����һ�ε��ô���NULL�����ݷ��ص�buffer_len������ռ�ڶ��δ����ȡid�б�
     buffer_len: ��Ҫ��ȡ��id�б���Ŀ����һ��id_array_bufferΪNULLʱ����ֵ��0���ɡ�
*/
ETDLL_API int32 etm_lixian_get_task_ids_by_state(int32 state,uint64 * id_array_buffer,u32 *buffer_len);

/* ��ȡ����������Ϣ */
ETDLL_API int32 etm_lixian_get_task_info(uint64 task_id,ELX_TASK_INFO * p_info);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* ��ȡ���߿ռ�BT ��������ļ��б�*/


/* BT�������ļ���Ϣ */
typedef struct t_elx_file_info
{
	uint64 _file_id; 						
	ELX_TASK_STATE _state;
	char _name[ETM_MAX_FILE_NAME_LEN];		/* �ļ���,utf8����*/
	
	uint64 _size;								/* �ļ���С,��λbyte*/
	int32 _progress;							/* ����ļ�����������,��Ϊ���ؽ���*/

	char _file_suffix[16]; 						/* �ļ���׺��,��: txt,exe,flv,avi,mp3...*/ 
	u8 _cid[20];								/* �ļ�cid */
	u8 _gcid[20];								/* �ļ�gcid */
	Bool _vod;								/* �ļ��Ƿ�ɵ㲥 */
	char _url[ETM_MAX_URL_LEN];				/* �ļ�������URL,utf8����*/
	char _cookie[ETM_MAX_URL_LEN];			/* �ļ�������URL ��Ӧ��cookie */
	u32 _file_index;							/* BT ���ļ���bt�����е��ļ����,��0��ʼ */
} ELX_FILE_INFO;

/* ��ȡ�б���Ӧ,�ڴ������ؿ��ͷ�,UI��Ҫ����һ�����л���UI�߳��д��� */
typedef struct t_elx_file_list
{
	u32 _action_id;
	uint64 _task_id;
	void * _user_data;				/* �û����� */
	int32 _result;						/*	0:�ɹ�,����ֵ:ʧ��	*/
	u32 _total_file_num;				/* ��BT �����ڵ����ļ�����*/
	u32 _file_num;					/* �˴ν�����õ��ļ����� */
	ELX_FILE_INFO * _file_array;		/* ע��:�˲���������! ���ļ���Ϣ������etm_lixian_get_bt_sub_file_info�ӿڻ�ȡ */
} ELX_FILE_LIST;
typedef int32 ( *ELX_GET_FILE_LIST_CALLBACK)(ELX_FILE_LIST * p_file_list);

/* ��ȡ���߿ռ�BT ��������ļ��б���������*/
typedef struct t_elx_get_bt_file_ls
{
	uint64 _task_id;					/* BT ����id */
	ELX_FILE_TYPE _file_type;			/*  Ϊָ����ȡbt�������ض����͵��ļ�*/
	int32 _file_status;				/*  Ϊָ��bt�������ض�״̬���ļ�,0:ȫ����1:��ѯ��������������2:���*/
	int32 _offset;						/*  ��ʾ�����ļ�����̫��ʱ������ȡ��ƫ��ֵ,��0��ʼ*/
	int32 _max_file_num;				/*  Ϊ�˴λ�ȡ����ļ�����   */
	
	void * _user_data;				/* �û����� */
	ELX_GET_FILE_LIST_CALLBACK _callback;
} ELX_GET_BT_FILE_LIST;

/* ��ȡ���߿ռ�BT ��������ļ��б�
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
ETDLL_API int32 etm_lixian_get_bt_task_file_list(ELX_GET_BT_FILE_LIST * p_param,u32 * p_action_id);

/* ����: ��������Ĳ�ͬ״̬��ȡ��������id �б�
   ����: 
     state: ��Χ 0-127, �������¼���״ֵ̬������ȡ��Ҫ��ȡ�ļ���״ֵ̬	
     state = 0��ʾ��ȡ����״̬id�б�
	    	ELXS_WAITTING =1, 
			ELXS_RUNNING  =2, 
			ELXS_PAUSED   =4,
			ELXS_SUCCESS  =8, 
			ELXS_FAILED   =16, 
			ELXS_OVERDUE  =32,
			ELXS_DELETED  =64
     id_array_buffer: ��֪��ĳ״̬��Ӧ��id��Ŀʱ����һ�ε��ô���NULL�����ݷ��ص�buffer_len������ռ�ڶ��δ����ȡid�б�
     buffer_len: ��Ҫ��ȡ��id�б���Ŀ����һ��id_array_bufferΪNULLʱ����ֵ��0���ɡ�
*/
ETDLL_API int32 etm_lixian_get_bt_sub_file_ids_by_state(uint64 task_id,int32 state,uint64 * id_array_buffer,u32 *buffer_len);

/////��ȡ����BT������ĳ�����ļ�����Ϣ
ETDLL_API int32 etm_lixian_get_bt_sub_file_info(uint64 task_id, uint64 file_id, ELX_FILE_INFO *p_file_info);
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/* ��ȡ��Ƶ��ͼ,��ͼΪjpg��ʽ */
typedef struct t_elx_screenshot_result
{
	u32 _action_id;
	void * _user_data;							/* �û����� */
	char  _store_path[ETM_MAX_FILE_PATH_LEN]; 	/* �������� ��ͼ���·��*/
	Bool _is_big;
	u32 _file_num;
	u8 * _gcid_array;
	int32 * _result_array;							/*	���ļ��Ľ�ͼ��ȡ���:0 �ɹ�,����ֵ:ʧ��	*/
} ELX_SCREENSHOT_RESUTL;
typedef int32 ( *ELX_GET_SCREENSHOT_CALLBACK)(ELX_SCREENSHOT_RESUTL * p_screenshot_result);

/* ��ȡ��Ƶ��ͼ���������*/
typedef struct t_elx_get_screenshot
{
	u32 _file_num;								/* file_num ��Ҫ��ȡ��ͼ���ļ����� */
	u8 * _gcid_array;							/* gcid_array ��Ÿ�����Ҫ��ȡ��ͼ���ļ�gcid,��Ϊ20byte�����ִ� */
	
	Bool _is_big;								/* is_big �Ƿ��ȡ��ͼ��TRUEΪ��ȡ 256*192�Ľ�ͼ,FALSEΪ��ȡ128*96	�Ľ�ͼ */
	const char * _store_path;						/* store_pathΪ��ͼ���·��,�����д,���ؿ⽫�Ѹ�gcid��Ӧ���ļ���ͼ���ص���Ŀ¼��,����gcid��Ϊ�ļ���,��ͼΪjpg��׺ */
	Bool _overwrite;
	
	void * _user_data;							/* �û����� */
	ELX_GET_SCREENSHOT_CALLBACK _callback;
} ELX_GET_SCREENSHOT;

/* ��ȡ��Ƶ��ͼ,��ͼΪjpg��ʽ 
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
ETDLL_API int32 etm_lixian_get_screenshot(ELX_GET_SCREENSHOT * p_param,u32 * p_action_id);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* ��ȡ�ļ����Ƶ㲥��URL */

/* ��"��Ƶָ��"������ѯ����ͬһ����Ӱ��������ѡ�㲥URL �������Ϣ*/
typedef struct t_elx_fp_info
{
	uint64 _size;								/* ��ѡ�㲥��Դ���ļ���С,��λbyte*/
	u8 _cid[20];								/* ��ѡ�㲥��Դ���ļ�cid */			
	u8 _gcid[20];								/* ��ѡ�㲥��Դ���ļ�gcid */		
	u32 _video_time;							/* ��ѡ�㲥��Դ����Ƶʱ�䳤��,��λ: s*/
	u32 _bit_rate;							/* ��ѡ�㲥��Դ�����ʡ��������ݲ��ṩ�ò�����*/
	char _vod_url[ETM_MAX_URL_LEN];    		/* ��ѡ�㲥��Դ�ĵ㲥URL*/
	char _screenshot_url[ETM_MAX_URL_LEN];    	/* ��ѡ�㲥��Դ�Ľ�ͼURL*/
} ELX_FP_INFO;

/*�Ƶ㲥�����Ϣ*/
typedef struct t_elx_get_vod_result
{
	u32 _action_id;
	void * _user_data;							/* �û����� */
       int32 _result;									/*	0:�ɹ�������ֵ:ʧ��*/

	//uint64 _size;							
	u8 _cid[20];							
	u8 _gcid[20];							
	//char _sheshou_cid[64];    						/*����cid,���ڲ�ѯ��Ļ*/
	u32 _video_time;								/*��Ƶʱ�䳤��,��λ: s*/

	u32 _normal_bit_rate;						/* �������ʡ��������ݲ��ṩ�ò�����*/
	uint64 _normal_size;							/* ������Ƶ�ļ��Ĵ�С */						
	char _normal_vod_url[ETM_MAX_URL_LEN];    	/* ����㲥URL*/
	
	u32 _high_bit_rate;							/* �������ʡ��������ݲ��ṩ�ò�����*/
	uint64 _high_size;							/* ������Ƶ�ļ��Ĵ�С */				
	char _high_vod_url[ETM_MAX_URL_LEN];    		/* ����㲥URL*/

	/* ���ֲ�ͬ���ʸ�ʽ */
	u32 _fluency_bit_rate_1;							/* �������ʡ��������ݲ��ṩ�ò�����*/
	uint64 _fluency_size_1;							/* ������Ƶ�ļ��Ĵ�С */				
	char _fluency_vod_url_1[ETM_MAX_URL_LEN];    		/* �����㲥URL*/

	u32 _fluency_bit_rate_2;							/* �������ʡ��������ݲ��ṩ�ò�����*/
	uint64 _fluency_size_2;							/* ������Ƶ�ļ��Ĵ�С */				
	char _fluency_vod_url_2[ETM_MAX_URL_LEN];    		/* �����㲥URL*/
	
	u32 _fpfile_num;								/* ��"��Ƶָ��"������ѯ����ͬһ����Ӱ��������ѡ�㲥URL �ĸ���*/
	ELX_FP_INFO * _fp_array;						/* ������ѡ�㲥URL ����*/
} ELX_GET_VOD_RESULT;
typedef int32 ( *ELX_GET_VOD_URL_CALLBACK)(ELX_GET_VOD_RESULT * p_result);

/* ��ȡ��Ƶ�ļ����Ƶ㲥URL ���������*/
typedef struct t_elx_get_vod
{
	u32 _device_width;
	u32 _device_height;
	u32 _video_type;							/*	��Ƶ��ʽ: 0:flv(���ݾɰ汾);  1:flv;  2:ts;  3:����;  4: mp4  */
		
	uint64 _size;								/* �ļ���С,��λbyte*/
	u8 _cid[20];								
	u8 _gcid[20];								
	//Bool _is_after_trans;					/* �Ƿ���ת�����ļ�*/
	u32 _max_fpfile_num;					/* ��"��Ƶָ��"������ѯͬһ����Ӱ��������ѡ�㲥URL ��������*/

	void * _user_data;						/* �û����� */
	ELX_GET_VOD_URL_CALLBACK _callback_fun;
} ELX_GET_VOD;

/* ��ȡ��Ƶ�ļ����Ƶ㲥URL 
	p_action_id ���ظ��첽������id��������;ȡ���ò���*/
ETDLL_API int32 etm_lixian_get_vod_url(ELX_GET_VOD * p_param,u32 * p_action_id);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* ɾ���������񷵻������Ϣ*/
typedef struct t_elx_delete_task_result
{
	u32 _action_id;
	void * _user_data;							/* �û����� */
	int32 _result;									/*	0:�ɹ�������ֵ:ʧ��*/
} ELX_DELETE_TASK_RESULT;

typedef int32 ( *ELX_DELETE_TASK_CALLBACK)(ELX_DELETE_TASK_RESULT * p_result);

/* �����߿ռ�ɾ����������,ʹ�øýӿ�ǰ�����ù�jump key*/
ETDLL_API int32 etm_lixian_delete_task(uint64 task_id, void* user_data, ELX_DELETE_TASK_CALLBACK callback_fun, u32 * p_action_id);
/* ɾ����������*/
ETDLL_API int32 etm_lixian_delete_overdue_task(uint64 task_id, void* user_data, ELX_DELETE_TASK_CALLBACK callback_fun, u32 * p_action_id);

/* ����ɾ���������񷵻������Ϣ*/
typedef struct t_elx_delete_tasks_result
{
	u32 _action_id;
	void * _user_data;							/* �û����� */
	u32 _task_num;								/* ����ɾ��������� */
	uint64 * _p_task_ids;							/* Ҫɾ��������id�б�(���ؿ������ڴ棬���治��ɾ��) */
	int32 *_p_results;							/*	����id��Ӧ�Ĳ������,0:�ɹ�������ֵ:ʧ��(���ؿ������ڴ棬���治��ɾ��) */
} ELX_DELETE_TASKS_RESULT;

typedef int32 ( *ELX_DELETE_TASKS_CALLBACK)(ELX_DELETE_TASKS_RESULT * p_result);
/* �����߿ռ�����ɾ����������,ʹ�øýӿ�ǰ�����ù�jump key*/
ETDLL_API int32 etm_lixian_delete_tasks(u32 task_num,uint64 * p_task_ids,BOOL is_overdue, void* user_data, ELX_DELETE_TASKS_CALLBACK callback_fun, u32 * p_action_id);


/* ��ѯ����������Ϣ���ص����� (Ŀǰֻ���ز���UI��Ҫ���ݣ��绹���������ݣ�����ýṹ�岢���ڲ������ݷ��ؼ���) */
typedef struct t_elx_query_task_info_result
{
	u32 _action_id;
	void * _user_data;							/* �û����� */
	int32 _result;								/*	0:�ɹ�������ֵ:ʧ��*/
	//ELX_TASK_STATE _download_status;			/*	����״̬*/
	//int32 _progress;							/*	���ؽ���*/
} ELX_QUERY_TASK_INFO_RESULT;
typedef int32 ( *ELX_QUERY_TASK_INFO_CALLBACK)(ELX_QUERY_TASK_INFO_RESULT * p_result);
/* ��ѯ������Ϣ(�첽�ӿ�)��Ŀǰֻ֧�ֵ���������Ϣ��ѯ(��Ҫ֧�ֶ�������������ýӿ�) task_id Ϊ��Ҫ��ѯ������id���飬task_numΪ��Ҫ��ѯ��������Ŀ
   callback_funΪ���첽�����Ļص�����, p_action_id ���ظ��첽������id��������;ȡ���ò���*/
ETDLL_API int32 etm_lixian_query_task_info(uint64 *task_id, u32 task_num, void* user_data, ELX_GET_TASK_LIST_CALLBACK callback_fun, u32 * p_action_id);

/* ��ѯBT��������Ϣ����֧�ֵ������߶�������������Ϣ��ѯ 
   ����: task_id Ϊ��Ҫ��ѯ������id����;
   		 task_numΪ��Ҫ��ѯ��������Ŀ;
   		 callback_funΪ���첽�����Ļص�����;
   		 p_action_id ���ظ��첽������id��������;ȡ���ò���
*/
ETDLL_API int32 etm_lixian_query_bt_task_info(uint64 *task_id, u32 task_num, void* user_data, ELX_QUERY_TASK_INFO_CALLBACK callback_fun, u32 * p_action_id);

/* ��������: �ѹ��������������ؽӿ�(�첽�ӿ�)
   ��������: ���ѹ��������������ز�����(����ɾ���ѹ��������б��е��ļ�����ɾ�����������ӿ�)
   ��������: 
     task_id Ϊ��Ҫ�������ص��ѹ�������id; (���ѹ��������������غ�ķ�������id��ԭ����id�ǲ�һ����)
     user_data �û��������ص������д���;
     callback Ϊ�ѹ�����������´�����Ļص�����
     p_action_id ���ظ��첽������id��������;ȡ���ò���
*/
ETDLL_API int32 etm_lixian_create_task_again(uint64 task_id, void* user_data, ELX_CREATE_TASK_CALLBACK callback, u32 * p_action_id);


/* �����������񷵻������Ϣ*/
typedef struct t_elx_delay_task_result
{
	u32 _action_id;
	void * _user_data;							/* �û����� */
	int32 _result;									/*	0:�ɹ�������ֵ:ʧ��*/
	u32 _left_live_time;							/*  ʣ������ */	
} ELX_DELAY_TASK_RESULT;

typedef int32 ( *ELX_DELAY_TASK_CALLBACK)(ELX_DELAY_TASK_RESULT * p_result);

/* �����߿ռ�������������,ʹ�øýӿ�ǰ�����ù�jump key*/
ETDLL_API int32 etm_lixian_delay_task(uint64 task_id, void* user_data, ELX_DELAY_TASK_CALLBACK callback_fun, u32 * p_action_id);	

/* �����ѯ�������񷵻������Ϣ*/
typedef struct t_elx_miniquery_task_result
{
	u32 _action_id;
	void * _user_data;							/* �û����� */
	int32 _result;									/*	0:�ɹ�������ֵ:ʧ��*/
	u32 _state;									/*	0:������ڣ�3: ���񲻴��� ������ֵ*/
	uint64 _commit_time;									/*  �ύʱ�� */	
	u32 _left_live_time;	   					/* ʣ������ */	
} ELX_MINIQUERY_TASK_RESULT;

typedef int32 ( *ELX_MINIQUERY_TASK_CALLBACK)(ELX_MINIQUERY_TASK_RESULT * p_result);

ETDLL_API int32 etm_lixian_miniquery_task(uint64 task_id, void* user_data, ELX_MINIQUERY_TASK_CALLBACK callback_fun, u32 * p_action_id);
	
///// ����URL ,cid,gicd ��BT���ӵ�info_hash ���Ҷ�Ӧ����������id
ETDLL_API int32 	etm_lixian_get_task_id_by_eigenvalue(ETM_EIGENVALUE * p_eigenvalue,uint64 * task_id);

/* ��������: etm_lixian_get_task_id_by_gcid(ͬ���ӿ�)
   ��������: ͨ��gcid��ȡ���������id, �����bt�������򻹿ɻ�ȡ��bt��������ļ�id��(ͨ���ú�����ȡ��id֮�󼴿ɵõ���������������Ϣ)
   ��������: 
     p_gcid  : �����40�ֽڵ�gcidֵ��
     task_id : �������ص���������id�����ڴ�;
     file_id : �����bt����������������ļ�id�����ڴˣ������ֵΪ0��
*/
ETDLL_API int32 etm_lixian_get_task_id_by_gcid(char * p_gcid, uint64 * task_id, uint64 * file_id);

/* ����ҳ����̽�ɹ����ص�url  */
typedef struct t_elx_downloadable_url_result
{
	u32 _action_id;
	void * _user_data;						/* �û����� */
	char _url[ETM_MAX_URL_LEN];				/* ����ʱ�������ؿⱻ��̽��URL */
	int32 _result;								/*	0:�ɹ�������ֵ:ʧ��,������ֶ���Ч*/
	u32 _url_num;							/*  ���ֿɹ����ص�URL ���� */
} ELX_DOWNLOADABLE_URL_RESULT;

typedef int32 ( *ELX_DOWNLOADABLE_URL_CALLBACK)(ELX_DOWNLOADABLE_URL_RESULT * p_result);

ETDLL_API int32 etm_lixian_get_downloadable_url_from_webpage(const char* url,void* user_data,ELX_DOWNLOADABLE_URL_CALLBACK callback_fun , u32 * p_action_id);

/* ���Ѿ���ȡ������ҳԴ�ļ�����̽�ɹ����ص�url ,ע��,�����ͬ���ӿڣ�����Ҫ���罻��  */
ETDLL_API int32 etm_lixian_get_downloadable_url_from_webpage_file(const char* url,const char * source_file_path,u32 * p_url_num);

///// ��ȡ��ҳ�пɹ�����URL��id �б�,*buffer_len�ĵ�λ��sizeof(u32)
ETDLL_API int32 etm_lixian_get_downloadable_url_ids(const char* url,u32 * id_array_buffer,u32 *buffer_len);

/* ��ȡ�ɹ�����URL ��Ϣ */
typedef enum t_elx_url_type
{
	ELUT_ERROR = -1,
	ELUT_HTTP = 0, 				
	ELUT_FTP, 
	ELUT_THUNDER, 
	ELUT_EMULE, 
	ELUT_MAGNET, 				/* ������ */
	ELUT_BT, 					/* ָ��torrent �ļ���url */
} ELX_URL_TYPE;
typedef enum t_elx_url_status
{
	ELUS_UNTREATED = 0, 		/* δ���� */			
	ELUS_IN_LOCAL, 				/* �Ѿ����ص����� */
	ELUS_IN_LIXIAN				/* �Ѿ������߿ռ����� */
} ELX_URL_STATUS;
typedef struct t_elx_downloadable_url
{
	u32 _url_id;
	ELX_URL_TYPE _url_type;				/* URL ����*/
	ELX_FILE_TYPE _file_type;				/*  URL ָ����ļ�����*/
	ELX_URL_STATUS _status;				/* ��URL �Ĵ���״̬*/
	uint64 _file_size;
	char _name[ETM_MAX_FILE_NAME_LEN];
	char _url[ETM_MAX_URL_LEN];
} ELX_DOWNLOADABLE_URL;

ETDLL_API int32 etm_lixian_get_downloadable_url_info(u32 url_id,ELX_DOWNLOADABLE_URL * p_info);

/* �жϸ�url �Ƿ�ɹ����� */
ETDLL_API BOOL etm_lixian_is_url_downloadable(const char* url);

/* ��ȡurl������ */
ETDLL_API  ELX_URL_TYPE etm_lixian_get_url_type(const char* url);

typedef struct t_elx_take_off_flux_result
{
	u32 _action_id;
	void * _user_data;						/* �û����� */
	int32 _result;							/*	0:�ɹ�������ֵ:ʧ��*/
	uint64 _all_flux;						/*  ������ */	
	uint64 _remain_flux;					/*  ʣ������ */	
	uint64 _lixian_taskid;				/*  ��������id */
	uint64 _lixian_fileid;				/*  ��������id */	
} ELX_TAKE_OFF_FLUX_RESULT;

typedef int32 ( *ELX_TAKE_OFF_FLUX_CALLBACK)(ELX_TAKE_OFF_FLUX_RESULT * p_result);
/* ��������: etm_lixian_take_off_flux_from_high_speed(�첽�ӿ�)
   ��������: �Ӹ��ٷ������۳���������ʹ����û�������url�ܹ���Ϊ����urlʹ�á�
   ��������: 
     task_id  :   ���������Ӧ��task_id;
     file_id  :   ����BT�����Ӧ��������file_id,����bt������0����;
     user_data :  �û�Я�������ݣ��ص������ﷵ��;
     callback_fun: ��ȡ�����Ϣ�Ļص�����;
     p_action_id:  �첽������id; 
*/
ETDLL_API int32 etm_lixian_take_off_flux_from_high_speed(uint64 task_id, uint64 file_id, void* user_data, ELX_TAKE_OFF_FLUX_CALLBACK callback_fun, u32 * p_action_id);

typedef struct t_free_strategy_result
{
	u32 _action_id;
	void * _user_data;						/* �û����� */
	int32 _result;							/*	0:�ɹ���-1: ��ʱ; ����ֵ:ʧ��*/
	uint64 _user_id;						/*  �û�id */
	BOOL _is_active_stage;				/*  �Ƿ��ڻ���� */	
	u32 _free_play_time;					/*  �Բ�ʱ�� */
} ETM_FREE_STRATEGY_RESULT;
typedef int32 ( *ETM_FREE_STRATEGY_CALLBACK)(ETM_FREE_STRATEGY_RESULT * p_param);
/* ��������: etm_get_free_strategy(�첽�ӿ�)
   ��������: �ӷ�������ȡ��Ѳ�����ص���Ϣ��
   ��������: 
     user_id  :   �û�id;
     session_id : �û���¼��session_id;
     user_data :  �û�Я�������ݣ��ص������ﷵ��;
     callback_fun: ��ȡ�����Ϣ�Ļص�����;
     p_action_id:  �첽������id;
*/
ETDLL_API int32 etm_get_free_strategy(uint64 user_id, const char * session_id, void* user_data, ETM_FREE_STRATEGY_CALLBACK callback_fun, u32 * p_action_id);

typedef enum t_etm_cloudserver_type
{
	ETM_CS_RESP_GET_FREE_DAY = 1800, 	
	ETM_CS_RESP_GET_REMAIN_DAY
} ETM_CS_AC_TYPE;
typedef struct t_free_experience_member_result
{
	u32 _action_id;
	void * _user_data;				/* �û����� */
	int32 _result;					/*	0:�ɹ���-1: ��ʱ; ����ֵ:ʧ��*/
	uint64 _user_id;				/*  �û�id */
	ETM_CS_AC_TYPE _type;			/*  ��Ӧ���� */
	u32 _free_time;				/*  �������� */
	BOOL _is_first_use;			/*  �Ƿ��һ��ʹ�� */
} ETM_FREE_EXPERIENCE_MEMBER_RESULT;
typedef int32 ( *ETM_FREE_EXPERIENCE_MEMBER_CALLBACK)(ETM_FREE_EXPERIENCE_MEMBER_RESULT * p_param);
/* ��������: etm_get_free_experience_member(�첽�ӿ�)
   ��������: �ӷ�������ȡ��Ѳ�����ص���Ϣ��
   ��������: 
     user_id  :   �û�id;
     user_data :  �û�Я�������ݣ��ص������ﷵ��;
     callback_fun: ��ȡ�����Ϣ�Ļص�����;
     p_action_id:  �첽������id;
*/
ETDLL_API int32 etm_get_free_experience_member(uint64 user_id, void* user_data, ETM_FREE_EXPERIENCE_MEMBER_CALLBACK callback_fun, u32 * p_action_id);

/* ��������: etm_get_free_experience_member(�첽�ӿ�)
   ��������: �ӷ�������ȡ��Ѳ�����ص���Ϣ��
   ��������: 
     user_id  :   �û�id;
     user_data :  �û�Я�������ݣ��ص������ﷵ��;
     callback_fun: ��ȡ�����Ϣ�Ļص�����;
     p_action_id:  �첽������id;
*/
ETDLL_API int32 etm_get_experience_member_remain_time(uint64 user_id, void* user_data, ETM_FREE_EXPERIENCE_MEMBER_CALLBACK callback_fun, u32 * p_action_id);

typedef struct t_get_high_speed_flux_result
{
	u32 _action_id;
	void * _user_data;						/* �û����� */
	int32 _result;							/*	0:�ɹ���-1: ��ʱ; ����ֵ:ʧ��*/
} ETM_GET_HIGH_SPEED_FLUX_RESULT;
typedef int32 ( *ETM_GET_HIGH_SPEED_FLUX_CALLBACK)(ETM_GET_HIGH_SPEED_FLUX_RESULT * p_param);

typedef struct t_get_high_speed_flux
{
	uint64 _user_id;     /* �û�id */
	uint64 _flux;		  /* ��Ҫ��ȡ�������� */			
	u32    _valid_time; /* ��ȡ��������Чʱ�� */
	u32    _property;   /* 0Ϊ��ͨ��������1Ϊ��Ա������ */
	void* _user_data;   /* �û����� */
	ETM_GET_HIGH_SPEED_FLUX_CALLBACK _callback_fun;  /* �ӿڻص����� */
} ETM_GET_HIGH_SPEED_FLUX;
/* ��������: etm_get_high_speed_flux(�첽�ӿ�)
   ��������: �ӷ�����(ת��)��ȡ�����������
*/
ETDLL_API int32 etm_get_high_speed_flux(ETM_GET_HIGH_SPEED_FLUX *flux_param, u32 * p_action_id);

//////////////////////////////////////////////////////////////
/////16 ��¼ģ����ؽӿ�
#define MEMBER_SESSION_ID_SIZE 64
#define MEMBER_PROTOCAL_JUMPKEY_MAX_SIZE 512
#define MEMBER_MAX_USERNAME_LEN 64
#define MEMBER_MAX_PASSWORD_LEN 64
#define MEMBER_MAX_DATE_SIZE 32
#define MEMBER_MAX_PAYID_NAME_LEN   64
	
typedef enum tag_ETM_MEMBER_EVENT
{
	ETM_LOGIN_LOGINED_EVENT = 0,		/* Logined�¼��ص�ʱ, errcodeΪ0��ʾlogin�ɹ�,Ϊ(-200)��ʾ��¼ʧ�ܵ��ǿ�����cache��Ϣ,Ϊ(-201)Ϊ��¼ʧ��,���ǿ�����cache,����cache���sessionid�Ѿ�����.����ֵΪ����ʧ��! */
	ETM_LOGIN_FAILED_EVENT,
//	LOGOUT_SUCCESS_EVENT,
//	LOGOUT_FAILED_EVENT,
//	PING_FAILED_EVENT,
	ETM_UPDATE_PICTURE_EVENT,
	ETM_KICK_OUT_EVENT,			//�˻��ڱ𴦵�¼��������
	ETM_NEED_RELOGIN_EVENT		//�˻���Ҫ���µ�¼
//	MEMBER_LOGOUT_EVENT,
//	REFRESH_MEMBER_EVENT,
//	MEMBER_CANCEL_EVENT
}ETM_MEMBER_EVENT;

/* ��¼��Ϣ */
typedef struct tagETM_MEMBER_INFO
{
	Bool _is_vip;
	Bool _is_year;		//�Ƿ���ѻ�Ա
	Bool _is_platinum;
	uint64 _userid;		//ȫ��Ψһid
	Bool _is_new;		//username �Ƿ����ʺţ����ʺ�usernameΪ������
	char _username[MEMBER_MAX_USERNAME_LEN];		//�˻���
	char _nickname[MEMBER_MAX_USERNAME_LEN];
	char _military_title[16];
	u16 _level;
	u16 _vip_rank;
	char _expire_date[MEMBER_MAX_DATE_SIZE];	// VIP��������
	u32 _current_account;
	u32 _total_account;
	u32 _order;
	u32 _update_days;
	char _picture_filename[1024];
	char _session_id[MEMBER_SESSION_ID_SIZE];
	u32 _session_id_len;
	char _jumpkey[MEMBER_PROTOCAL_JUMPKEY_MAX_SIZE];
	u32 _jumpkey_len;
	u32 _payid;
	char _payname[MEMBER_MAX_PAYID_NAME_LEN];
	Bool _is_son_account;    //�Ƿ����˺�
	u32 _vas_type;	          //��Ա����: 2-��ͨ��Ա 3-�׽��Ա 4--��ʯ��Ա
}ETM_MEMBER_INFO; 

/* ��¼״̬ */
typedef enum tagETM_MEMBER_STATE
{
	ETM_MEMBER_INIT_STATE = 0,
	ETM_MEMBER_LOGINING_STATE,		//���ڵ�½��״̬
	ETM_MEMBER_LOGINED_STATE,		//��½�ɹ���״̬
	ETM_MEMBER_LOGOUT_STATE,		//�����˳���½��״̬
	ETM_MEMBER_FAILED_STATE
}ETM_MEMBER_STATE;

typedef int32 (*ETM_MEMBER_EVENT_NOTIFY)(ETM_MEMBER_EVENT event, int32 errcode);

typedef int32 (*ETM_MEMBER_REGISTER_CALLBACK)(int32 errcode, void *user_data);

typedef int32 (*ETM_MEMBER_REFRESH_NOTIFY)(int32 result);

typedef struct tagETM_REGISTER_INFO
{
	char _username[MEMBER_MAX_USERNAME_LEN];
	char _password[MEMBER_MAX_PASSWORD_LEN];
	char _nickname[MEMBER_MAX_USERNAME_LEN];
	void* _user_data;
	ETM_MEMBER_REGISTER_CALLBACK _callback_func;
}ETM_REGISTER_INFO;

//���õ�¼�ص��¼�����
ETDLL_API int32 etm_member_set_callback_func(ETM_MEMBER_EVENT_NOTIFY notify_func);

// �û���¼����������utf8��ʽ�����벻���ܡ�
ETDLL_API int32 etm_member_login(const char* username, const char* password);
// sessionid��userid��¼,(�������ϵ�¼)��
ETDLL_API int32 etm_member_sessionid_login(const char* sessionid, uint64 userid);

// ���ܷ�ʽ��¼������ʹ�ñ��ر����MD5���ܺ�����룬��������utf8��ʽ
ETDLL_API int32 etm_member_encode_login(const char* username, const char* md5_pw);

// ���µ�¼: ������沢�û�����û����������µ�¼
ETDLL_API int32 etm_member_relogin(void);

// ˢ���û���Ϣ�������µ�¼����������»�ȡ�û�����Ϣ���ص��ɹ���etm_member_get_info���»�ȡ��Ϣ
ETDLL_API int32 etm_member_refresh_user_info(ETM_MEMBER_REFRESH_NOTIFY notify_func);

// �û�ע����
ETDLL_API int32 etm_member_logout(void);

// �û�����ping����ȷ���û�sessionid����Ч���ڡ�����Ҫsessionid�Ĳ���֮ǰ����Ҫ����һ�θ�����(eg: �Ʋ�����)
ETDLL_API int32 etm_member_keepalive(void);
// �ϱ������ļ���
//ETDLL_API int32 etm_member_report_download_file(uint64 filesize, const char cid[40], char* url, u32 url_len);

//���û�е�¼�ɹ����øú����᷵��ʧ��
ETDLL_API int32 etm_member_get_info(ETM_MEMBER_INFO* info);

//��ȡ�û�ͷ��ͼƬȫ·���ļ���������ͼƬ����NULL
ETDLL_API char * etm_member_get_picture(void);

//��ȡ��¼״̬
ETDLL_API ETM_MEMBER_STATE etm_member_get_state(void);

//��ȡ��������͡�
//const char* etm_get_error_code_description(int32 errcode);

//�û�ע�ᣬ�鿴id�Ƿ����
ETDLL_API int32 etm_member_register_check_id(ETM_REGISTER_INFO* register_info);

//�û�ע�ᣬ��������utf8��ʽ��
ETDLL_API int32 etm_member_register_user(ETM_REGISTER_INFO* register_info);

ETDLL_API int32 etm_member_register_check_id_cancel(void);

ETDLL_API int32 etm_member_register_cancel(void);


#ifdef __cplusplus
}
#endif
#endif /* ET_TASK_MANAGER_H_200909081912 */

