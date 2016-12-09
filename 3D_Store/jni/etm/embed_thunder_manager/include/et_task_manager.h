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
#define ETM_MAX_FILE_NAME_LEN (512)		/* 文件名最大长度 */
#define ETM_MAX_FILE_PATH_LEN (512)		/* 文件路径(不包括文件名)最大长度 */
#define ETM_MAX_TD_CFG_SUFFIX_LEN (8)		/* 迅雷下载临时文件的后缀最大长度 */
#define ETM_MAX_URL_LEN (1024)		/* URL 最大长度 */
#define ETM_MAX_COOKIE_LEN ETM_MAX_URL_LEN

/************************************************************************/

/* 任务类型*/
typedef enum t_etm_task_type
{
	ETT_URL = 0, 				/* 用URL 创建的任务,支持协议: http://,https://,ftp://,thunder:// */
	ETT_BT, 					/* 用.torrent 文件创建的任务 */
	ETT_TCID,  					/* 用迅雷的TCID 创建的任务  */
	ETT_KANKAN, 				/* 用迅雷的TCID,File_Size,和GCID一起创建的任务 ,注意 这种类型的任务不会自动停止，需要调用etm_stop_vod_task才会停止*/
	ETT_EMULE, 					/* 用电驴链接创建的任务  								 */
	ETT_FILE ,					/* 用文件路径直接创建一个本地任务,目的是把现成的文件转为一个任务由ETM管理  */
	ETT_LAN					/* 用tcid,[gcid],filesize,filepath,filename,url创建一个局域网任务,这种任务的特点是只从指定的URL下载数据 */
} ETM_TASK_TYPE;

/* 任务状态 */
typedef enum t_etm_task_state
{
	ETS_TASK_WAITING =0, 
	ETS_TASK_RUNNING , 
	ETS_TASK_PAUSED  ,
	ETS_TASK_SUCCESS , 
	ETS_TASK_FAILED  , 
	ETS_TASK_DELETED 
} ETM_TASK_STATE;

/* BT 任务的字符编码设置 */
typedef enum t_etm_encoding_mode
{ 
	EEM_ENCODING_NONE = 0, 				/*  返回原始字段*/
	EEM_ENCODING_GBK = 1,					/*  返回GBK格式编码 */
	EEM_ENCODING_UTF8 = 2,				/*  返回UTF-8格式编码 */
	EEM_ENCODING_BIG5 = 3,				/*  返回BIG5格式编码  */
	EEM_ENCODING_UTF8_PROTO_MODE = 4,	/*  返回种子文件中的utf-8字段  */
	EEM_ENCODING_DEFAULT = 5				/*  未设置输出格式(使用etm_set_default_encoding_mode的全局输出设置)  */
}ETM_ENCODING_MODE ;

/* 正在运行中的任务的实时状况 */
typedef struct t_etm_running_status
{
	ETM_TASK_TYPE  _type;   
	ETM_TASK_STATE  _state;   
	uint64 _file_size; 					/* 任务文件大小*/  
	uint64 _downloaded_data_size; 	/* 已下载的数据大小*/  
	u32 _dl_speed;  					/* 实时下载速率*/  
	u32 _ul_speed;  					/* 实时上传速率*/  
	u32 _downloading_pipe_num; 			/* 正在下载数据的socket 连接数 */
	u32 _connecting_pipe_num; 		/* 正在连接中的socket 连接数 */
}ETM_RUNNING_STATUS;

/* 任务信息 */
typedef struct t_etm_task_info
{
	u32  _task_id;		
	ETM_TASK_STATE  _state;   
	ETM_TASK_TYPE  _type;   

	char _file_name[ETM_MAX_FILE_NAME_LEN];
	char _file_path[ETM_MAX_FILE_PATH_LEN]; 
	uint64 _file_size; 
	uint64 _downloaded_data_size; 		
	 
	u32 _start_time;				/* 任务开始下载的时间，自1970年1月1日开始的秒数 */
	u32 _finished_time;			/* 任务完成(成功或失败)的时间，自1970年1月1日开始的秒数 */

	 /*任务失败原因
              102  无法纠错
              103  无法获取cid
              104  无法获取gcid
              105  cid 校验错误
              106  gcid校验错误
              107  创建文件失败
              108  写文件失败
              109  读文件失败
              112  空间不足无法创建文件
              113 校验cid时读文件错误 
              130  无资源下载失败

              15400 子文件下载失败(bt任务)         */  	  
	u32  _failed_code;			/* 如果任务失败的话,这里会有失败码 */
	 
	u32  _bt_total_file_num; 		/* 如果该任务的_type== ETT_BT, 这里是torrent文件中包含的子文件总个数*/
	Bool _is_deleted;			/* 当任务被成功调用etm_delete_task 接口后,它的_state不会为ETS_TASK_DELETED，还是保持原来的值以便它可以被正确还原(etm_recover_task)，用这个值标识任务此时状态为ETS_TASK_DELETED */
	Bool _check_data;			/* 点播时是否需要验证数据 */	
	Bool _is_no_disk;			/* 是否为无盘点播  */
}ETM_TASK_INFO;

/* BT 任务子文件的下载状态 */
typedef enum t_etm_file_status
{
	EFS_IDLE = 0, 
	EFS_DOWNLOADING, 
	EFS_SUCCESS, 
	EFS_FAILED 
} ETM_FILE_STATUS;

/* BT 任务子文件的下载信息 */
typedef struct  t_etm_bt_file
{
	u32 _file_index;
	Bool _need_download;			/* 是否需要下载 */
	ETM_FILE_STATUS _status;
	uint64 _file_size;	
	uint64 _downloaded_data_size;
	u32 _failed_code;
}ETM_BT_FILE;

/* BT 任务子文件的信息 */
typedef struct t_etm_torrent_file_info
{
	u32 _file_index;
	char *_file_name;
	u32 _file_name_len;
	char *_file_path;
	u32 _file_path_len;
	uint64 _file_offset;		/* 该子文件在整个BT 文件中的偏移 */
	uint64 _file_size;
} ETM_TORRENT_FILE_INFO;

/* torrent 文件的信息 */
typedef struct t_etm_torrent_seed_info
{
	char _title_name[ETM_MAX_FILE_NAME_LEN-ETM_MAX_TD_CFG_SUFFIX_LEN];
	u32 _title_name_len;
	uint64 _file_total_size;						/* 所有子文件的总大小*/
	u32 _file_num;								/* 所有子文件的总个数*/
	u32 _encoding;								/* 种子文件编码格式，GBK = 0, UTF_8 = 1, BIG5 = 2 */
	unsigned char _info_hash[20];					/* 种子文件的特征值 */
	ETM_TORRENT_FILE_INFO *file_info_array_ptr;	/* 子文件信息列表 */
} ETM_TORRENT_SEED_INFO;


/* E-Mule链接中包含的文件信息 */
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


/* 创建下载任务时的输入参数 */
typedef struct t_etm_create_task
{
	ETM_TASK_TYPE _type;				/* 任务类型，必须准确填写*/
	
	char* _file_path; 						/* 任务文件存放路径，必须已存在且可写；如果为NULL ，则表示存放于默认路径中(由etm_set_download_path事先设置) */
	u32 _file_path_len;					/* 任务文件存放路径的长度，必须小于 ETM_MAX_FILE_PATH_LEN */
	char* _file_name;					/* 任务文件名字 */
	u32 _file_name_len; 					/* 任务文件名字的长度，必须小于 ETM_MAX_FILE_NAME_LEN-ETM_MAX_TD_CFG_SUFFIX_LEN */
	
	char* _url;							/* _type== ETT_URL ,ETT_LAN或者_type== ETT_EMULE 时，这里就是任务的URL */
	u32 _url_len;						/* URL 的长度，必须小于512 */
	char* _ref_url;						/* _type== ETT_URL 时，这里就是任务的引用页面URL ,可为NULL */
	u32 _ref_url_len;						/* 引用页面URL 的长度，必须小于512 */
	
	char* _seed_file_full_path; 			/* _type== ETT_BT 时，这里就是任务的种子文件(*.torrent) 全路径 */
	u32 _seed_file_full_path_len; 		/* 种子文件全路径的长度，必须小于ETM_MAX_FILE_PATH_LEN+ETM_MAX_FILE_NAME_LEN */
	u32* _download_file_index_array; 	/* _type== ETT_BT 时，这里就是需要下载的子文件序号列表，为NULL 表示下载所有有效的子文件*/
	u32 _file_num;						/* _type== ETT_BT 时，这里就是需要下载的子文件个数*/
	
	char *_tcid;							/* _type== ETT_TCID ,ETT_LAN或ETT_KANKAN  时，这里就是任务文件的迅雷TCID ,为40字节的十六进制数字字符串*/
	uint64 _file_size;						/* _type== ETT_TCID ,ETT_LAN或ETT_KANKAN  时，这里就是任务文件的大小 */
	char *_gcid;							/* _type== ETT_KANKAN  时，这里就是任务文件的迅雷GCID ,为40字节的十六进制数字字符串*/
	
	Bool _check_data;					/* 任务边下边播时是否需要校验数据  */
	Bool _manual_start;					/* 是否手动开始下载?  TRUE 为用户手动开始下载,FALSE为ETM自动开始下载*/
	
	void * _user_data;					/* 用于存放与该任务相关的用户附加信息，如任务描述，海报，cookie等 。如果是cookie，coockie部分必须以"Cookie:"开头，并以"\r\n" 结束 */
	u32  _user_data_len;					/* 用户数据的长度,必须小于65535 */
} ETM_CREATE_TASK;

/* 创建点播任务时的输入参数 */
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
	u32 _file_num;						/* _type== ETT_BT 或_type== ETT_KANKAN  时，这里就是需要点播的子文件个数										[ 暂不支持ETT_KANKAN 多个文件] */

	char *_tcid;							/* _type== ETT_KANKAN  时，这里是一个或多个子文件(一部电影有可能被分成n个文件) TCID 的数组		[ 暂不支持ETT_KANKAN 多个文件] */
	uint64 _file_size;						/* _type== ETT_KANKAN  时，这里是所有(一个或多个)需要点播的文件的总大小								[ 暂不支持ETT_KANKAN 多个文件] */
	uint64 * _segment_file_size_array;	/* _type== ETT_KANKAN  时，这里可以是多个子文件(一部电影有可能被分成n个文件) file_size 的数组		[ 暂不支持ETT_KANKAN 多个文件] */
	char *_gcid;							/* _type== ETT_KANKAN  时，这里是一个或多个子文件(一部电影有可能被分成n个文件) GCID 的数组		[ 暂不支持ETT_KANKAN 多个文件] */

	Bool _check_data;					/* 是否需要校验数据  */
	Bool _is_no_disk;					/* 是否为无盘点播  */

	void * _user_data;					/* 用于存放与该任务相关的用户附加信息，如任务描述，海报，cookie等 。如果是cookie，coockie部分必须以"Cookie:"开头，并以"\r\n" 结束 */
	u32  _user_data_len;					/* 用户数据的长度,必须小于65535 */

	char* _file_name;
	u32   _file_name_len;
} ETM_CREATE_VOD_TASK;

/* 创建微型任务时的输入参数 (比如上传或下载几KB 的一块数据，一个小文件，一张小图片或网页等)*/
typedef struct t_etm_mini_task
{
	char* _url;					/* 只支持"http://" 开头的url  */
	u32 _url_len;
	
	Bool _is_file;					/* 是否为文件，TRUE为_file_path,_file_path_len,_file_name,_file_name_len有效,_data和_data_len无效；FALSE则相反*/
	
	char _file_path[ETM_MAX_FILE_PATH_LEN]; 				/* _is_file=TRUE时, 需要上传或下载的文件存储路径，必须真实存在 */
	u32 _file_path_len;			/* _is_file=TRUE时, 需要上传或下载的文件存储路径的长度*/
	char _file_name[ETM_MAX_FILE_NAME_LEN];			/* _is_file=TRUE时, 需要上传或下载的文件名*/
	u32 _file_name_len; 			/* _is_file=TRUE时, 需要上传或下载的文件名长度*/
	
	u8 * _send_data;			/* _is_file=FALSE时,指向需要上传的数据*/
	u32  _send_data_len;			/* _is_file=FALSE时,需要上传的数据长度 */
	
	void * _recv_buffer;			/* 用于接收服务器返回的响应数据的缓存，下载时如果_is_file=TRUE,此参数无效*/
	u32  _recv_buffer_size;		/* _recv_buffer 缓存大小,至少要16384 byte !下载时如果_is_file=TRUE,此参数无效 */
	
	char * _send_header;			/* 用于发送http头部 */
	u32  _send_header_len;		/* _send_header的大小	 */
	
	void *_callback_fun;			/* 任务完成后的回调函数 : typedef int32 ( *ETM_NOTIFY_MINI_TASK_FINISHED)(void * user_data, int32 result,void * recv_buffer,u32  recvd_len,void * p_header,u8 * send_data); */
	void * _user_data;			/* 用户参数 */
	u32  _timeout;				/* 超时,单位秒,等于0时180秒超时*/
	u32  _mini_id;				/* id */
	Bool  _gzip;					/* 是否接受或发送压缩文件 */
} ETM_MINI_TASK;

/* 创建商城任务时的输入参数 */
typedef struct t_etm_create_shop_task
{
	char* _url;							/* URL 的长度，必须小于512 */
	u32 _url_len;
	char* _ref_url;
	u32 _ref_url_len;					/* 引用页面URL 的长度，必须小于512 */	
	
	char* _file_path; 					/* 任务文件存放路径，必须已存在且可写；如果为NULL ，则表示存放于默认路径中(由etm_set_download_path事先设置) */
	u32 _file_path_len;					/* 任务文件存放路径的长度，必须小于 ETM_MAX_FILE_PATH_LEN */
	char* _file_name;					/* 任务文件名字 */
	u32 _file_name_len; 				/* 任务文件名字的长度，必须小于 ETM_MAX_FILE_NAME_LEN-ETM_MAX_TD_CFG_SUFFIX_LEN */

	char *_tcid;
	uint64 _file_size;
	char *_gcid;

	uint64 _group_id;
	char *_group_name;
	u32 _group_name_len;				/* group的名字长度，必须小于32 */
	u8 _group_type;
	uint64 _product_id;
	uint64 _sub_id;
} ETM_CREATE_SHOP_TASK;

/************************************************************************/
/*  Interfaces provided by EmbedThunderTaskManager	*/
/************************************************************************/
/////1 初始化与反初始化相关接口

/////1.1 初始化，etm_system_path 为ETM 用于存放系统数据的目录，必须已存在且可写，最小要有1M 的空间，不能在可移动的存储设备中；path_len 必须小于ETM_MAX_FILE_PATH_LEN
ETDLL_API int32 etm_init(const char *etm_system_path,u32  path_len);

/////1.2 反初始化
ETDLL_API int32 etm_uninit(void);

/////1.1 有外部设备插入时可调用该接口初始化下载任务，etm_system_path 为ETM 用于存放系统数据的目录，必须已存在且可写，最小要有1M 的空间，不能在可移动的存储设备中；path_len 必须小于ETM_MAX_FILE_PATH_LEN
ETDLL_API int32 etm_load_tasks(const char *etm_system_path,u32  path_len);

/////U盘拔出时调用该接口卸载掉所有任务，但是保留无盘点播任务
ETDLL_API int32 etm_unload_tasks(void);


/////1.3 网络相关接口
/*
网络连接类型：
(低16位) */
#define UNKNOWN_NET   	0x00000000
#define CHINA_MOBILE  	0x00000001
#define CHINA_UNICOM  	0x00000002
#define CHINA_TELECOM 	0x00000004
#define OTHER_NET     	0x00008000
/* (高16位) */
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
/* peerid的长度不能小于16，否则会返回参数错误 */
ETDLL_API int32 etm_set_peerid(const char* peerid, u32 peerid_len);
ETDLL_API const char* etm_get_peerid(void);
ETDLL_API int32 etm_user_set_network(Bool is_ok);

ETDLL_API int32 etm_set_net_type(u32 net_type);

///////////////////////////////////////////////////////////////
/////2 系统设置相关接口

/////2.1  设置用户自定义的底层接口,这个接口必须在调用etm_init 之前调用!
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

/* 参数列表说明：
 * 	int32 fun_idx    接口函数的序号
 * 	void *fun_ptr    接口函数指针
 *
 *
 *  目前支持的接口函数序号以及相对应的函数类型说明：
 *------------------------------------------------------------------------------------------------------------------
 *  序号：		0 (ET_FS_IDX_OPEN)      
 *  类型定义：	typedef int32 (*et_fs_open)(char *filepath, int32 flag, u32 *file_id);
 *  说明：		打开文件，需要以读写方式打开。成功时返回0，否则返回错误码
 *  参数说明：
 *	 filepath：需要打开文件的全路径； 
 *	 flag：    当(flag & 0x01) == 0x01时，文件不存在则创建文件，否则文件不存在时打开失败
                                                                       如果文件存在则以读写权限打开文件
                      当(flag & 0x02) == 0x02时，表示打开只读文件
                      当(flag & 0x04) == 0x04时，表示打开只写
                      当(flag ) = 0x 0时，表示读写权限打开文件
 *	 file_id： 文件打开成功，返回文件句柄
 *------------------------------------------------------------------------------------------------------------------
 *  序号：		1 (ET_FS_IDX_ENLARGE_FILE)   
 *  类型定义：	typedef int32 (*et_fs_enlarge_file)(u32 file_id, uint64 expect_filesize, uint64 *cur_filesize);
 *  说明：		重新改变文件大小（目前只需要变大）。一般用于打开文件后，进行预创建文件。 成功返回0，否则返回错误码
 *  参数说明：
 *   file_id：需要更改大小的文件句柄
 *   expect_filesize： 希望更改到的文件大小
 *   cur_filesize： 实际更改后的文件大小（注意：当设置大小成功后，一定要正确设置此参数的值!）
 *------------------------------------------------------------------------------------------------------------------
 *  序号：		2 (ET_FS_IDX_CLOSE)   
 *  类型定义：	typedef int32 (*et_fs_close)(u32 file_id);
 *  说明：		关闭文件。成功返回0，否则返回错误码
 *  参数说明：
 *   file_id：需要关闭的文件句柄
 *------------------------------------------------------------------------------------------------------------------
 *  序号：		3 (ET_FS_IDX_READ)   
 *  类型定义：	typedef int32 (*et_fs_read)(u32 file_id, char *buffer, int32 size, u32 *readsize);
 *  说明：		读取当前文件指针文件内容。成功返回0，否则返回错误码
 *  参数说明：
 *   file_id： 需要读取的文件句柄
 *   buffer：  存放读取内容的buffer指针
 *   size：    需要读取的数据大小（调用者可以保证不会超过buffer的大小）
 *   readsize：实际读取的文件大小（注意：文件读取成功后，一定要正确设置此参数的值!）
 *------------------------------------------------------------------------------------------------------------------
 *  序号：		4 (ET_FS_IDX_WRITE)   
 *  类型定义：	typedef int32 (*et_fs_write)(u32 file_id, char *buffer, int32 size, u32 *writesize);
 *  说明：		从当前文件指针处写入内容。成功返回0，否则返回错误码
 *  参数说明：
 *   file_id：  需要写入的文件句柄
 *   buffer：   存放写入内容的buffer指针
 *   size：     需要写入的数据大小（调用者可以保证不会超过buffer的大小）
 *   writesize：实际写入的文件大小（注意：文件写入成功后，一定要正确设置此参数的值!）
 *------------------------------------------------------------------------------------------------------------------
 *  序号：		5 (ET_FS_IDX_PREAD)   
 *  类型定义：	typedef int32 (*et_fs_pread)(u32 file_id, char *buffer, int32 size, uint64 filepos, u32 *readsize);
 *  说明：		读取指定偏移处的文件内容。成功返回0，否则返回错误码
 *  参数说明：
 *   file_id： 需要读取的文件句柄
 *   buffer：  存放读取内容的buffer指针
 *   size：    需要读取的数据大小（调用者可以保证不会超过buffer的大小）
 *   filepos： 需要读取的文件偏移
 *   readsize：实际读取的文件大小（注意：文件读取成功后，一定要正确设置此参数的值!）
 *------------------------------------------------------------------------------------------------------------------
 *  序号：		6 (ET_FS_IDX_PWRITE)   
 *  类型定义：	typedef int32 (*et_fs_pwrite)(u32 file_id, char *buffer, int32 size, uint64 filepos, u32 *writesize);
 *  说明：		从指定偏移处写入内容。成功返回0，否则返回错误码
 *  参数说明：
 *   file_id：  需要写入的文件句柄
 *   buffer：   存放写入内容的buffer指针
 *   size：     需要写入的数据大小（调用者可以保证不会超过buffer的大小）
 *   filepos：  需要读取的文件偏移
 *   writesize：实际写入的文件大小（注意：文件写入成功后，一定要正确设置此参数的值!）
 *------------------------------------------------------------------------------------------------------------------
 *  序号：		7 (ET_FS_IDX_FILEPOS)   
 *  类型定义：	typedef int32 (*et_fs_filepos)(u32 file_id, uint64 *filepos);
 *  说明：		获得当前文件指针位置。成功返回0，否则返回错误码
 *  参数说明：
 *   file_id： 文件句柄
 *   filepos： 从文件头开始计算的文件偏移
 *------------------------------------------------------------------------------------------------------------------
 *  序号：		8 (ET_FS_IDX_SETFILEPOS)   
 *  类型定义：	typedef int32 (*et_fs_setfilepos)(u32 file_id, uint64 filepos);
 *  说明：		设置当前文件指针位置。成功返回0，否则返回错误码
 *  参数说明：
 *   file_id： 文件句柄
 *   filepos： 从文件头开始计算的文件偏移
 *------------------------------------------------------------------------------------------------------------------
 *  序号：		9 (ET_FS_IDX_FILESIZE)   
 *  类型定义：	typedef int32 (*et_fs_filesize)(u32 file_id, uint64 *filesize);
 *  说明：		获得当前文件大小。成功返回0，否则返回错误码
 *  参数说明：
 *   file_id： 文件句柄
 *   filepos： 当前文件大小
 *------------------------------------------------------------------------------------------------------------------
 *  序号：		10 (ET_FS_IDX_FREE_DISK)   
 *  类型定义：	typedef int32 (*et_fs_get_free_disk)(const char *path, u32 *free_size);
 *  说明：		获得path路径所在磁盘的剩余空间，一般用作是否可以创建文件的判断依据。成功返回0，否则返回错误码
 *  参数说明：
 *   path：     需要获取剩余磁盘空间磁盘上的任意路径
 *   free_size：指定路径所在磁盘的当前剩余磁盘空间（注意：此参数值单位是 KB(1024 bytes) !）
 *------------------------------------------------------------------------------------------------------------------
 *  序号：		11 (ET_SOCKET_IDX_SET_SOCKOPT)   
 *  类型定义：	typedef int32 (*et_socket_set_sockopt)(u32 socket, int32 socket_type); 
 *  说明：		设置socket的相关参数，目前只支持协议簇PF_INET。成功返回0，否则返回错误码
 *  参数说明：
 *   socket：     需要设置的socket句柄
 *   socket_type：此socket的类型，目前有效的值有2个：SOCK_STREAM  SOCK_DGRAM。这2个宏的取值和所在的OS一致。
 *------------------------------------------------------------------------------------------------------------------
 *  序号：		12 (ET_MEM_IDX_GET_MEM)  
 *  类型定义：	typedef int32 (*et_mem_get_mem)(u32 memsize, void **mem);
 *  说明：		从操作系统分配固定大小的连续内存块。成功返回0，否则返回错误码
 *  参数说明：
 *   memsize：     需要分配的内存大小
 *   mem： 成功分配之后，内存块首地址放在*mem中返回。
 *------------------------------------------------------------------------------------------------------------------
 *  序号：		13 (ET_MEM_IDX_FREE_MEM)   
 *  类型定义：	typedef int32 (*et_mem_free_mem)(void* mem, u32 memsize);
 *  说明：		释放指定内存块给操作系统。成功返回0，否则返回错误码
 *  参数说明：
 *   mem：     需要释放的内存块首地址
 *   memsize：需要释放的内存块的大小
 *------------------------------------------------------------------------------------------------------------------ 
 *  序号：		14 (ET_ZLIB_UNCOMPRESS)   
 *  类型定义：	typedef _int32 (*et_zlib_uncompress)( unsigned char *p_out_buffer, int *p_out_len, const unsigned char *p_in_buffer, int in_len );
 *  说明：		指定zlib库的解压缩函数进来,便于kad网络中找源包的解压,提高emule找源数量
 *  参数说明：
 *   p_out_buffer：解压后数据缓冲区
 *   p_out_len：   解压后数据长度
 *   p_in_buffer： 待解压数据缓冲区
 *   in_len：      待解压数据长度
 *------------------------------------------------------------------------------------------------------------------ */
ETDLL_API int32 etm_set_customed_interface(int32 fun_idx, void *fun_ptr);
	
/////2.2 获取版本号,其中前部分为ETM 的版本号，后一部分为下载库ET 的版本号
ETDLL_API const char* etm_get_version(void);

/////2.3 注册license
/*目前提供仅用于测试的免费license:
*				License						    |  序号 |		有效期		| 状态
*	09072400010000000000009y4us41bxk5nsno35569 | 0000000 |2009-07-24~2010-07-24	| 00
*/
ETDLL_API int32 	etm_set_license(const char *license, int32 license_size);
ETDLL_API const char* etm_get_license(void);

/////2.4 获取license 的验证结果,接口返回码和接口参数*result 均等于0 为验证通过,其他值解释如下

/*  如果该接口返回值为102406，表示ETM正在与迅雷服务器交互中，需要在稍后再调用此接口。
*    在接口返回值为0的前提下,接口参数*result的值解释如下:
*    4096   表示通讯错误，原因可能是网络问题或服务器坏掉了。
*    4118   表示ETM在尝试了12次（每次间隔1小时）license上报之后，均告失败，原因同4096。
*    21000 表示 LICENSE 验证未通过，原因是 LICENSE 被冻结。
*    21001 表示 LICENSE 验证未通过，原因是 LICENSE 过期。
*    21002 表示 LICENSE 验证未通过，原因是 LICENSE 被回收。
*    21003 表示 LICENSE 验证未通协，原因是 LICENSE 被重复使用。
*    21004 表示 LICENSE 验证未通协，原因是 LICENSE 是虚假的。
*    21005 表示 服务器繁忙，ETM 会在一小时后自动重试,界面程序不必理会。
*/
ETDLL_API int32 	etm_get_license_result(u32 * result);

/////2.5 设置解释.torrent文件时系统默认字符编码格式，encoding_mode 不能为EEM_ENCODING_DEFAULT
ETDLL_API int32 etm_set_default_encoding_mode( ETM_ENCODING_MODE encoding_mode);
ETDLL_API ETM_ENCODING_MODE etm_get_default_encoding_mode(void);

/////2.6 设置默认下载存储路径,必须已存在且可写,path_len 必须小于ETM_MAX_FILE_PATH_LEN
ETDLL_API int32 etm_set_download_path(const char *path,u32  path_len);
ETDLL_API const char* etm_get_download_path(void);

/////2.7 设置任务状态转换通知回调函数,此函数必须在创建任务之前调用
// 注意回调函数的代码应尽量简洁，且万不可以在回调函数中调用任何ETM的接口函数，否则会导致死锁!
typedef int32 ( *ETM_NOTIFY_TASK_STATE_CHANGED)(u32 task_id, ETM_TASK_INFO * p_task_info);
ETDLL_API int32 	etm_set_task_state_changed_callback( ETM_NOTIFY_TASK_STATE_CHANGED callback_function_ptr);

/* 设置任务文件名改变通知回调函数,此函数必须在创建任务之前调用*/
// 注意回调函数的代码应尽量简洁，且万不可以在回调函数中调用任何ETM的接口函数，否则会导致死锁!
typedef int32 ( *ETM_NOTIFY_FILE_NAME_CHANGED)(u32 task_id, const char* file_name,u32 length);
ETDLL_API int32 etm_set_file_name_changed_callback(ETM_NOTIFY_FILE_NAME_CHANGED callback_function_ptr);
#define etm_set_task_file_name_changed_callback etm_set_file_name_changed_callback

/////2.8 设置有盘点播的默认临时文件缓存路径,必须已存在且可写,path_len 要小于ETM_MAX_FILE_PATH_LEN,此函数必须在创建有盘点播任务之前调用
ETDLL_API int32 etm_set_vod_cache_path(const char *path,u32  path_len);
ETDLL_API const char* etm_get_vod_cache_path(void);

/////2.9 设置有盘点播的缓存区大小，单位KB，即接口etm_set_vod_cache_path 所设进来的目录的最大可写空间，建议3GB 以上,此函数必须在调用etm_set_vod_cache_path之后,创建有盘点播任务之前调用
ETDLL_API int32 etm_set_vod_cache_size(u32  cache_size);
ETDLL_API u32  etm_get_vod_cache_size(void);

///// 强制清空vod 缓存
ETDLL_API int32  etm_clear_vod_cache(void);

/////设置缓冲时间，单位秒,默认为30秒缓冲,不建议设置该值,以保证播放流畅
ETDLL_API int32 etm_set_vod_buffer_time(u32 buffer_time);

/////2.10 设置点播的专用内存大小,单位KB，建议2MB 以上,此函数必须在创建点播任务之前调用
ETDLL_API int32 etm_set_vod_buffer_size(u32  buffer_size);
ETDLL_API u32  etm_get_vod_buffer_size(void);

/////2.11 查询vod 专用内存是否已经分配
ETDLL_API Bool etm_is_vod_buffer_allocated(void);

/////2.12 手工释放vod 专用内存,ETM 本身在反初始化时会自动释放这块内存，但如果界面程序想尽早腾出这块内存的话，可调用此接口，注意调用之前要确认无点播任务在运行
ETDLL_API int32  etm_free_vod_buffer(void);

/////2.13 重置以下各系统参数:(10M内存版本时)max_tasks=5,download_limit_speed=-1,upload_limit_speed=-1,auto_limit_speed=FALSE,max_task_connection=128,task_auto_start=FALSE
ETDLL_API int32 	etm_load_default_settings(void);

/////2.14 设置最大同时运行的任务个数,最小为1,最大为15
ETDLL_API int32 etm_set_max_tasks(u32 task_num);
ETDLL_API u32 etm_get_max_tasks(void);

/////2.15 设置下载限速,以KB 为单位,等于-1为不限速,不能为0
ETDLL_API int32 etm_set_download_limit_speed(u32 max_speed);
ETDLL_API u32 etm_get_download_limit_speed(void);

/////2.16 设置上传限速,以KB 为单位,等于-1为不限速,不能为0,建议max_download_limit_speed:max_upload_limit_speed=3:1
ETDLL_API int32 etm_set_upload_limit_speed(u32 max_speed);
ETDLL_API u32 etm_get_upload_limit_speed(void);

/////2.17 设置是否启动智能限速(auto_limit=TRUE为启动)，linux下必须用root 权限运行ETM 的时候才能生效
ETDLL_API int32 etm_set_auto_limit_speed(Bool auto_limit);
ETDLL_API Bool etm_get_auto_limit_speed(void);

/////2.18 设置最大连接数,10M 内存版本取值范围为[1~200]
ETDLL_API int32 etm_set_max_task_connection(u32 connection_num);
ETDLL_API u32 etm_get_max_task_connection(void);

/////2.19 设置ETM 启动后是否自动开始下载未完成的任务(auto_start=TRUE为是)
ETDLL_API int32 etm_set_task_auto_start(Bool auto_start);
ETDLL_API Bool etm_get_task_auto_start(void);

/////2.20 获取当前全局下载总速度,以Byte 为单位
ETDLL_API u32 etm_get_current_download_speed(void);

/////2.21 获取当前全局上传总速度,以Byte 为单位
ETDLL_API u32 etm_get_current_upload_speed(void);

/////2.22 设置下载分段大小,[100~1000]，单位KB
ETDLL_API int32 etm_set_download_piece_size(u32 piece_size);
ETDLL_API u32 etm_get_download_piece_size(void);

/////2.23 设置下载完成提示音开关类型:0 -关闭,1-打开,3-震动,4-根据情景模式
ETDLL_API int32 etm_set_prompt_tone_mode(u32 mode);
ETDLL_API u32 etm_get_prompt_tone_mode(void);

/////2.24 设置p2p 下载模式:0 -打开,1-在WIFI网络下打开,2-关闭
ETDLL_API int32 etm_set_p2p_mode(u32 mode);
ETDLL_API u32 etm_get_p2p_mode(void);

/////2.25 设置下载任务的CDN 使用策略:
// enable - 使该策略生效,enable_cdn_speed - 任务下载速度小于该值时启动CDN(单位KB),
// disable_cdn_speed -- 任务速度减去CDN速度大于该值时关闭CDN(单位KB)
ETDLL_API int32 etm_set_cdn_mode(Bool enable,u32 enable_cdn_speed,u32 disable_cdn_speed);

ETDLL_API int32 etm_set_ui_version(const char *ui_version, int32 product,int32 partner_id);

ETDLL_API int32 etm_get_business_type_by_product_id(int32 product_id);
////////////////////////////////////////////////////////////

// 用户登录行为。对应值为:
// 1-安装应用程序
// 2-打开应用程序
// 3-退出应用程序
#define REPORTER_USER_ACTION_TYPE_LOGIN (0)

// 播放器行为。对应值为:
// 1-打开
// 2-退出
#define REPORTER_USER_ACTION_TYPE_PLAYER (1)

// TEXT行为。对应值为:
// 1-mov
// 2-text
// 3-software
// 9-other
#define REPORTER_USER_ACTION_TYPE_DOWNLOAD_FILE (2)
#define REPORTER_USER_ACTION_TYPE_UPLOAD_FILE (4)

//看看播放器打开行为
// 1-表示进入看看播放器
#define REPORTER_USER_ACTION_ENTER_KANKAN_PLAYER   (3)

/*  101-迅雷手机助手相关 */
#define REPORTER_USER_ACTION_XUNLEI_ASSISTANT   (101)  

/* 201-IPAD看看（3.0以上） */
#define REPORTER_USER_ACTION_IPAD_KANKAN   (201)


//看看播放器播放信息
//11001-表示remark(data)字段存放的是KANKAN_PLAY_INFO_STAT数据协议包
//12001-表示remark(data)字段存放的是KANKAN_USER_ACTION_STAT数据协议包
#define REPORTER_USER_ACTION_PLAY_INFO    (999)

//用户普通操作
//action_type = REPORTER_COMMON_USER_ACTION,action_value 总为0
//详细信息放在data中,data_len的单位是byte.
#define REPORTER_COMMON_USER_ACTION    (1000)

ETDLL_API int32 etm_reporter_mobile_user_action_to_file(u32 action_type, u32 action_value, void *data, u32 data_len);

/* http 方式上报，上报data内容为键值对格式:
	u=aaaa&u1=bbbb&u2=cccc&u3=dddd&u4=eeeee
例子: u=mh_chanel&u1={手机端版本号}&u2={peerid}&u3={手机端IMEI}&u4={时间unix时间}&u5={频道名称}
*/
ETDLL_API int32 etm_http_report(char *data, u32 data_len);
ETDLL_API int32 etm_http_report_by_url(char *data, u32 data_len);
//定时上报用户行为记录文件的开关，默认开启
ETDLL_API int32 etm_enable_user_action_report(Bool is_enable);

/* 以下三组接口用于界面保存和获取配置项
	item_name 和item_value最长为63字节
	调用get接口时,若找不到与item_name 对应的项则以item_value 为初始值生成新项 
*/
ETDLL_API int32  etm_settings_set_str_item(char * item_name,char *item_value);
ETDLL_API int32  etm_settings_get_str_item(char * item_name,char *item_value);

ETDLL_API int32  etm_settings_set_int_item(char * item_name,int32 item_value);
ETDLL_API int32  etm_settings_get_int_item(char * item_name,int32 *item_value);

ETDLL_API int32  etm_settings_set_bool_item(char * item_name,Bool item_value);
ETDLL_API int32  etm_settings_get_bool_item(char * item_name,Bool *item_value);

/*添加 强制替换域名及其对应的ip地址,下载库不再对该域名查dns, 随机使用对应的ip地址(若有多个)*/
ETDLL_API int32  etm_set_host_ip(const char * host_name,const char *ip);
/* 清除所配置的域名ip */
ETDLL_API int32  etm_clean_host(void);
///////////////////////////////////////////////////////////////
/////3下载任务(包括纯下载和边下边播)相关接口

/////3.1 创建下载任务
ETDLL_API int32 etm_create_task(ETM_CREATE_TASK * p_param,u32* p_task_id );

/////3.1.1 创建下载任务
ETDLL_API int32 etm_create_shop_task(ETM_CREATE_SHOP_TASK * p_param,u32* p_task_id);

/////3.2 暂停任务
ETDLL_API int32 etm_pause_task (u32 task_id);

/////3.3 开始被暂停的任务
ETDLL_API int32 etm_resume_task(u32 task_id);

/////提供给IPAD 播放器的恢复任务接口
ETDLL_API int32 etm_vod_resume_task(u32 task_id);

/////3.4 将任务放入回收站
ETDLL_API int32 etm_delete_task(u32 task_id);

/////3.5 还原已删除的任务
ETDLL_API int32 etm_recover_task(u32 task_id);

/////强制开始某个任务，慎用
ETDLL_API int32 etm_force_run_task(u32 task_id);

/////3.6 永久删除任务,当delete_file为TRUE时表示删除任务所有信息的同时也删除任务对应的文件
ETDLL_API int32 etm_destroy_task(u32 task_id,Bool delete_file);

/////3.7 获取任务下载优先级task_id 列表,任务状态为ETS_TASK_WAITING,ETS_TASK_RUNNING,ETS_TASK_PAUSED,ETS_TASK_FAILED的都会在这个列表中,*buffer_len的单位是sizeof(u32)
ETDLL_API int32 etm_get_task_pri_id_list (u32 * id_array_buffer,u32 *buffer_len);

/////3.8 将任务下载优先级上移
ETDLL_API int32 etm_task_pri_level_up(u32 task_id,u32 steps);
	
/////3.9 将任务下载优先级下移
ETDLL_API int32 etm_task_pri_level_down (u32 task_id,u32 steps);

/////3.10 重命名任务文件，任务状态必须为ETS_TASK_SUCCESS，且不能用于bt任务，new_name_len必须小于ETM_MAX_FILE_NAME_LEN-ETM_MAX_TD_CFG_SUFFIX_LEN
ETDLL_API int32 etm_rename_task(u32 task_id,const char * new_name,u32 new_name_len);

/////3.11 获取任务的基本信息,由etm_create_vod_task接口创建的任务也可以调用这个接口
ETDLL_API int32 etm_get_task_info(u32 task_id,ETM_TASK_INFO *p_task_info);

/////ipad kankan 获取下载任务的下载信息，将任务的状态与点播状态分离
ETDLL_API int32 etm_get_task_download_info(u32 task_id,ETM_TASK_INFO *p_task_info);

/////3.12 获取正在运行的 任务的运行状况,由etm_create_vod_task接口创建的任务也可以调用这个接口
ETDLL_API int32 etm_get_task_running_status(u32 task_id,ETM_RUNNING_STATUS *p_status);

/////3.13 获取类型为ETT_URL ,ETT_LAN或者ETT_EMULE的 任务的URL
ETDLL_API const char* etm_get_task_url(u32 task_id);
/////替换类型为ETT_LAN的 任务的URL
ETDLL_API int32 etm_set_task_url(u32 task_id,const char * url);

/////3.14 获取类型为ETT_URL 的 任务的引用页面URL(如果有的话)
ETDLL_API const char* etm_get_task_ref_url(u32 task_id);

/////3.15 获取任务的TCID
ETDLL_API const char* etm_get_task_tcid(u32 task_id);
ETDLL_API const char* etm_get_bt_task_sub_file_tcid(u32 task_id, u32 file_index);
ETDLL_API const char* etm_get_file_tcid(const char * file_path);

/////3.16 获取类型不为ETT_BT 的 任务的GCID
ETDLL_API const char* etm_get_task_gcid(u32 task_id);

/////3.17 获取类型为ETT_BT 的 任务的种子文件全路径
ETDLL_API const char* etm_get_bt_task_seed_file(u32 task_id);

/////3.18 获取BT任务中某个子文件的信息,file_index取值范围为>=0。由etm_create_vod_task接口创建的bt 任务也可以调用这个接口
ETDLL_API int32 etm_get_bt_file_info(u32 task_id, u32 file_index, ETM_BT_FILE *file_info);

/* 获取bt任务子文件名 */
ETDLL_API const char* etm_get_bt_task_sub_file_name(u32 task_id, u32 file_index);

/////3.19 修改BT任务中要下载的文件序号列表,如果之前需要下的文件仍然需要下载的话，也要放在new_file_index_array里
ETDLL_API int32 etm_set_bt_need_download_file_index(u32 task_id, u32* new_file_index_array,	u32 new_file_num);
ETDLL_API int32 etm_get_bt_need_download_file_index(u32 task_id, u32* id_array_buffer,	u32 * buffer_len); /* *buffer_len的单位是sizeof(u32) */

/////3.20 获取任务的用户附加数据,*buffer_len的单位是byte
ETDLL_API int32 etm_get_task_user_data(u32 task_id,void * data_buffer,u32 * buffer_len);

/////3.21 根据任务状态获取任务id 列表(包括远程控制创建的任务和本地创建的任务),*buffer_len的单位是sizeof(u32)
ETDLL_API int32 	etm_get_task_id_by_state(ETM_TASK_STATE state,u32 * id_array_buffer,u32 *buffer_len);

/////3.22 根据任务状态获取本地创建的任务id 列表(不包括远程控制创建的任务),*buffer_len的单位是sizeof(u32)
ETDLL_API int32 	etm_get_local_task_id_by_state(ETM_TASK_STATE state,u32 * id_array_buffer,u32 *buffer_len);

/////3.23 获取所有的任务id 列表,*buffer_len的单位是sizeof(u32)，不包括无盘和有盘点播任务(包括有盘点播任务转为下载模式后的任务)
ETDLL_API int32 	etm_get_all_task_ids(u32 * id_array_buffer,u32 *buffer_len);

//// 获取所有任务文件的总大小(包含etm_get_all_task_need_space),单位Byte
ETDLL_API uint64 etm_get_all_task_total_file_size(void);
	
//// 获取所有任务完成下载仍需占用的空间大小,单位Byte
ETDLL_API uint64 etm_get_all_task_need_space(void);

///// 获取任务本地点播URL,只有启动了本地http 点播服务器时才能调用
ETDLL_API const char* etm_get_task_local_vod_url(u32 task_id);

/* 功能: 通过td.cfg文件路径创建任务
   参数: 
   		cfg_file_path: cfg文件的路径。
   		p_task_id: 传出的通过cfg创建的任务id。
*/
ETDLL_API int32 etm_create_task_by_cfg_file(char* cfg_file_path, u32* p_task_id);

/* 功能: 替换已存在未下载完成任务的原始url。 
	1. 先删除掉原有任务(不删除文件)；2. 修改对应cfg文件存储的原始url。 3. 用新的url创建任务。
参数: 
	new_origin_url: 需替换的新的原始url。
	p_task_id: 传入的是指针，必须填值，值对应的是需要替换url的原始任务的任务id。 重新创建之后该值接收到新的任务id。
*/
ETDLL_API int32 etm_use_new_url_create_task(char *new_origin_url, u32* p_task_id);

///////////////////////////////////////////////////////////////
/////离线下载相关接口
/////3.23 根据URL ,cid,gicd 或BT种子的info_hash 查找对应的任务id
typedef struct t_etm_eigenvalue
{
	ETM_TASK_TYPE _type;				/* 任务类型，必须准确填写*/
	
	char* _url;							/* _type== ETT_URL 或者_type== ETT_EMULE 时，这里就是任务的URL */
	u32 _url_len;						/* URL 的长度，必须小于512 */
	char _eigenvalue[40];					/* _type== ETT_TCID 或者ETT_LAN 时,这里放CID,_type== ETT_KANKAN 时,这里放GCID, _type== ETT_BT 时,这里放info_hash   */
}ETM_EIGENVALUE;
ETDLL_API int32 	etm_get_task_id_by_eigenvalue(ETM_EIGENVALUE * p_eigenvalue,u32 * task_id);

/////3.24 把任务设置为离线任务
ETDLL_API int32 etm_set_task_lixian_mode(u32 task_id,Bool is_lixian);

/////3.25 查询任务是否为离线任务
ETDLL_API Bool etm_is_lixian_task(u32 task_id);

/////3.26 增加server 资源,file_index只在BT任务中用到,普通任务file_index可忽略
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

/////3.27 增加peer 资源,file_index只在BT任务中用到,普通任务file_index可忽略
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

/////获取类型为ETT_LAN的 任务的peer相关信息
ETDLL_API int32 etm_get_peer_resource( u32 task_id, ETM_PEER_RES  * p_resource );

typedef  enum etm_lixian_state {ELS_IDLE=0, ELS_RUNNING, ELS_FAILED } ETM_LIXIAN_STATE;
typedef struct t_etm_lixian_info
{
	ETM_LIXIAN_STATE 	_state;
	u32 					_res_num;	//资源个数
	uint64 				_dl_bytes;	//通过离线资源下载到的数据
	u32 					_speed;		
	int32	 			_errcode;
}ETM_LIXIAN_INFO;
/* 获取离线下载信息 */
ETDLL_API int32 etm_get_lixian_info(u32 task_id, u32 file_index,ETM_LIXIAN_INFO * p_lx_info);

/* 设置或获取离线任务id ,lixian_task_id 等于0为无效id */
ETDLL_API int32 etm_set_lixian_task_id(u32 task_id, u32 file_index,uint64 lixian_task_id);
ETDLL_API int32 etm_get_lixian_task_id(u32 task_id, u32 file_index,uint64 * p_lixian_task_id);

///////////////////////////////////////////////////////////////
////广告任务相关接口

///// 把任务设置为广告任务，需要在创建任务时把任务设置为手动开始之后调用 (顺序下载)
ETDLL_API int32 etm_set_task_ad_mode(u32 task_id,Bool is_ad);

///// 查询任务是否为广告任务
ETDLL_API Bool etm_is_ad_task(u32 task_id);

///////////////////////////////////////////////////////////////
/////高速通道相关接口

typedef enum etm_hsc_state{EHS_IDLE=0, EHS_RUNNING, EHS_FAILED}ETM_HSC_STATE;
typedef struct t_etm_hsc_info
{
	ETM_HSC_STATE 		_state;
	u32 					_res_num;	//资源个数
	uint64 				_dl_bytes;	//通过高速通道下载到的数据
	u32 					_speed;		
	int32	 			_errcode;
}ETM_HSC_INFO;
/* 获取高速通道信息 */
ETDLL_API int32 etm_get_hsc_info(u32 task_id, u32 file_index, ETM_HSC_INFO * p_hsc_info);

/* 查询高速通道是否可用 */
ETDLL_API int32 etm_is_high_speed_channel_usable( u32 task_id , u32 file_index,Bool * p_usable);

/* 启用高速通道 */
ETDLL_API int32 etm_open_high_speed_channel( u32 task_id , u32 file_index);


///////////////////////////////////////////////////////////////
/////4 torrent 文件解析和E-Mule链接解析相关接口,其中torrent文件解析的两个接口必须成对使用!

/////4.1 从种子文件解析出种子信息
ETDLL_API int32 etm_get_torrent_seed_info (const char *seed_path, ETM_ENCODING_MODE encoding_mode,ETM_TORRENT_SEED_INFO **pp_seed_info );

/////4.2 释放种子信息
ETDLL_API int32 etm_release_torrent_seed_info( ETM_TORRENT_SEED_INFO *p_seed_info );

/////4.3 从E-Mule链接中解析出文件信息
ETDLL_API int32 etm_extract_ed2k_url(char* ed2k, ETM_ED2K_LINK_INFO* info);

/////4.4 从磁力链接中解析出文件信息,目前只能解析xt为bt info hash 的磁力链
typedef struct tagETM_MAGNET_INFO
{
	char		_file_name[ETM_MAX_FILE_NAME_LEN];
	uint64		_file_size;
	u8		_file_hash[20];
}ETM_MAGNET_INFO;
ETDLL_API int32 etm_parse_magnet_url(char* magnet, ETM_MAGNET_INFO* info);

/* 用bt任务的info_hash 生成磁力链*/
ETDLL_API int32 etm_generate_magnet_url(const char * info_hash, const char * display_name,uint64 total_size,char * url_buffer ,u32 buffer_len);

/*	功能: 对src中的数据进行AES(128Bytes)加密，加密后数据保存在des中
 *	aes_key:用于加密的key (注意，这个key内部进行了MD5运算，用算出的MD5值作为加密解密的密钥!)
 *	src:	待加密数据
 *	src_len:待加密数据长度
 *	des:	加密后的数据
 *	des_len:输入输出参数，传进去的的是des的内存最大长度,传出来的是加密后的数据长度
*/
ETDLL_API int32 etm_aes128_encrypt(const char* aes_key,const u8 * src, u32 src_len, u8 * des, u32 * des_len);
/*	功能: 对src中的数据进行AES(128Bytes)解密，解密后数据保存在des中
 *	aes_key:用于解密的key (注意，这个key内部进行了MD5运算，用算出的MD5值作为加密解密的密钥!)
 *	src:	待解密数据
 *	src_len:待解密数据长度
 *	des:	解密后的数据
 *	des_len:输入输出参数，传进去的的是des的内存最大长度,传出来的是解密后的数据长度
*/
ETDLL_API int32 etm_aes128_decrypt(const char* aes_key,const u8 * src, u32 src_len, u8 * des, u32 * des_len);
/**
 * 使用gz对buffer编码
 * @para: 	src     -输入buffer
 *			src_len -输入buffer的长度
 * 		  	des     -输出buffer
 *			des_len -输出buffer的长度
 * 			encode_len -压缩后的长度
 * @return: 0-SUCCESS  else-FAIL
 */
ETDLL_API int32 etm_gzip_encode(u8 * src, u32 src_len, u8 * des, u32 des_len, u32 *encode_len);
/**
 * 使用gz对buffer解码，注意，如果传入des不够长度，内部会重新分配长度为des_len的内存
 * @para: 	src     -输入buffer
 *			src_len -输入buffer的长度
 * 		  	des     -输出buffer
 *			des_len -输出buffer的总长度
 * 			encode_len -解压后有效数据的长度
 * @return: 0-SUCCESS  else-FAIL
 */
ETDLL_API int32 etm_gzip_decode(u8 * src, u32 src_len, u8 **des, u32 *des_len, u32 *decode_len);

///////////////////////////////////////////////////////////////
/////5 VOD点播相关接口

/////5.1 创建点播任务
ETDLL_API int32 etm_create_vod_task(ETM_CREATE_VOD_TASK * p_param,u32* p_task_id );

/////5.2 停止点播,如果这个任务是由etm_create_vod_task接口创建的，那么这个任务将被删除,如果任务由etm_create_task接口创建，则只停止点播
ETDLL_API int32 etm_stop_vod_task (u32 task_id);

/////5.3 读取文件数据，由etm_create_task接口创建的任务也可调用这个接口以实现边下载边播放
///// 建议len(一次获取数据的长度,单位Byte)为16*1024 以达到最优获取速度；block_time 单位为毫秒,  取值500,1000均可
ETDLL_API int32 etm_read_vod_file(u32 task_id, uint64 start_pos, uint64 len, char *buf, u32 block_time );
ETDLL_API int32 etm_read_bt_vod_file(u32 task_id, u32 file_index, uint64 start_pos, uint64 len, char *buf, u32 block_time );

/////5.4 获取缓冲(码率乘与30秒的数据长度)百分比，由etm_create_task接口创建的任务在边下载边播放时也可调用这个接口
ETDLL_API int32 etm_vod_get_buffer_percent(u32 task_id,  u32 * percent );

/////5.5 查询点播任务的剩余数据是否已经下载完成，一般用于判断是否应该开始下载下一个电影或同一部电影的下一片段。由etm_create_task接口创建的任务在边下载边播放时也可调用这个接口
ETDLL_API int32 etm_vod_is_download_finished(u32 task_id, Bool* finished );

/////5.6 获取点播任务文件的码率
ETDLL_API int32 etm_vod_get_bitrate(u32 task_id, u32 file_index, u32* bitrate );

/////5.7 获取当前下载到的连续位置
ETDLL_API int32 etm_vod_get_download_position(u32 task_id, uint64* position );

/////5.8 将点播任务转为下载任务,file_retain_time为任务文件在磁盘的保留时间(等于0为永久保存),单位秒
ETDLL_API int32 etm_vod_set_download_mode(u32 task_id, Bool download,u32 file_retain_time );

/////5.9 查询点播任务是否转为下载任务
ETDLL_API int32 etm_vod_get_download_mode(u32 task_id, Bool * download,u32 * file_retain_time );

/////5.10 点播统计上报
typedef struct t_etm_vod_report
{
       u32	_vod_play_time_len;//点播时长,单位均为秒
       u32  _vod_play_begin_time;//开始点播时间
	u32  _vod_first_buffer_time_len; //首缓冲时长
	u32  _vod_play_drag_num;//拖动次数
	u32  _vod_play_total_drag_wait_time;//拖动后等待总时长
	u32  _vod_play_max_drag_wait_time;//最大拖动等待时间
	u32  _vod_play_min_drag_wait_time;//最小拖动等待时间
	u32  _vod_play_interrupt_times;//中断次数
	u32  _vod_play_total_buffer_time_len;//中断等待总时长
	u32  _vod_play_max_buffer_time_len;//最大中断等待时间
	u32  _vod_play_min_buffer_time_len;//最小中断等到时间
	
	u32 _play_interrupt_1;         // 1分钟内中断
	u32 _play_interrupt_2;	        // 2分钟内中断
	u32 _play_interrupt_3;		    //6分钟内中断
	u32 _play_interrupt_4;		    //10分钟内中断
	u32 _play_interrupt_5;		    //15分钟内中断
	u32 _play_interrupt_6;		    //15分钟以上中断
	Bool _is_ad_type;
}ETM_VOD_REPORT;
ETDLL_API int32 etm_vod_report(u32 task_id, ETM_VOD_REPORT * p_report );

/////5.11 查询点播任务最终获取数据的服务器域名(只适用于云点播任务)
ETDLL_API int32 etm_vod_get_final_server_host(u32 task_id, char  server_host[256] );

/////5.12 界面通知下载库开始点播(只适用于云点播任务)
ETDLL_API int32 etm_vod_ui_notify_start_play(u32 task_id );

///////////////////////////////////////////////////////////////
/// 6. 任务存储树相关接口,注意这棵树与下载任务或点播任务无任何关系，一般仅供界面程序存放树状菜单用

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


// 创建或打开一棵树(flag=ETM_TF_CREATE|ETM_TF_RDWR),如果成功*p_tree_id返回树根节点id
ETDLL_API int32 	etm_open_tree(const char * file_path,int32 flag ,u32 * p_tree_id); 

// 关闭一棵树
ETDLL_API int32 	etm_close_tree(u32 tree_id); 

// 删除一棵树
ETDLL_API int32 	etm_destroy_tree(const char * file_path); 

// 判断数是否存在
ETDLL_API Bool 	etm_tree_exist(const char * file_path); 

// 复制一棵树
ETDLL_API int32 	etm_copy_tree(const char * file_path,const char * new_file_path); 


// 创建一个节点,data_len(单位Byte)大小无限制,但建议不大于256
ETDLL_API int32 	etm_create_node(u32 tree_id,u32 parent_id,const char * name, u32 name_len,void * data, u32 data_len,u32 * p_node_id); 

// 删除一个节点。注意:如果这个node_id是树枝，会把里面的所有叶节点同时删除;node_id不能等于tree_id,删除树要用etm_destroy_tree
ETDLL_API int32 	etm_delete_node(u32 tree_id,u32 node_id);

// 设置节点名字(或重命名),name_len必须小于256 bytes;node_id不能等于tree_id
ETDLL_API int32 	etm_set_node_name(u32 tree_id,u32 node_id,const char * name, u32 name_len);

// 获取节点名字;node_id等于tree_id时返回这棵树的名字
ETDLL_API  const char * etm_get_node_name(u32 tree_id,u32 node_id);

// 设置或修改节点数据,new_data_len(单位Byte)大小无限制,但建议不大于256;
ETDLL_API int32 	etm_set_node_data(u32 tree_id,u32 node_id,void * new_data, u32 new_data_len);

// 获取节点数据,*buffer_len的单位是Byte;
ETDLL_API int32	etm_get_node_data(u32 tree_id,u32 node_id,void * data_buffer,u32 * buffer_len);

// 移动节点
///移往不同分类;node_id不能等于tree_id
ETDLL_API int32 	etm_set_node_parent(u32 tree_id,u32 node_id,u32 parent_id);

///同一分类中移动;node_id不能等于tree_id
ETDLL_API int32 	etm_node_move_up(u32 tree_id,u32 node_id,u32 steps);
ETDLL_API int32 	etm_node_move_down(u32 tree_id,u32 node_id,u32 steps);


// 获取父节点id;node_id不能等于tree_id
ETDLL_API u32 	etm_get_node_parent(u32 tree_id,u32 node_id);

// 获取所有子节点id 列表,*buffer_len的单位是sizeof(u32)
ETDLL_API int32	etm_get_node_children(u32 tree_id,u32 node_id,u32 * id_buffer,u32 * buffer_len);

// 获取第一个子节点id,找不到返回ETM_INVALID_NODE_ID
ETDLL_API u32	etm_get_first_child(u32 tree_id,u32 parent_id);

// 获取兄弟节点id;node_id不能等于tree_id,找不到返回ETM_INVALID_NODE_ID
ETDLL_API u32	etm_get_next_brother(u32 tree_id,u32 node_id);
ETDLL_API u32	etm_get_prev_brother(u32 tree_id,u32 node_id);

// 根据名字查找节点id，【名字暂不支持分层式的写法，如aaa.bbb.ccc】
// 返回第一个匹配的节点id，找不到返回ETM_INVALID_NODE_ID
ETDLL_API u32	etm_find_first_node_by_name(u32 tree_id,u32 parent_id,const char * name, u32 name_len);

// 返回下一个匹配的节点id，找不到返回ETM_INVALID_NODE_ID;node_id不能等于tree_id
ETDLL_API u32	etm_find_next_node_by_name(u32 tree_id,u32 parent_id,u32 node_id,const char * name, u32 name_len); //注意node_id与parent_id的对应关系
// 返回上一个匹配的节点id，找不到返回ETM_INVALID_NODE_ID;node_id不能等于tree_id
ETDLL_API u32	etm_find_prev_node_by_name(u32 tree_id,u32 parent_id,u32 node_id,const char * name, u32 name_len); //注意node_id与parent_id的对应关系


// 根据节点数据查找节点,data_len单位Byte.

// 返回第一个匹配的节点id，找不到返回ETM_INVALID_NODE_ID
ETDLL_API u32	etm_find_first_node_by_data(u32 tree_id,u32 parent_id,void * data, u32 data_len);

// 返回下一个匹配的节点id，找不到返回ETM_INVALID_NODE_ID;node_id不能等于tree_id
ETDLL_API u32	etm_find_next_node_by_data(u32 tree_id,u32 parent_id,u32 node_id,void * data, u32 data_len);
// 返回上一个匹配的节点id，找不到返回ETM_INVALID_NODE_ID;node_id不能等于tree_id
ETDLL_API u32	etm_find_prev_node_by_data(u32 tree_id,u32 parent_id,u32 node_id,void * data, u32 data_len);

// 根据节点名字和数据查找合适的节点,data_len单位Byte.
// 返回第一个匹配的节点id，找不到返回ETM_INVALID_NODE_ID
ETDLL_API u32	etm_find_first_node(u32 tree_id,u32 parent_id,const char * name, u32 name_len,void * data, u32 data_len);

// 返回下一个匹配的节点id，找不到返回ETM_INVALID_NODE_ID;node_id不能等于tree_id
ETDLL_API u32	etm_find_next_node(u32 tree_id,u32 parent_id,u32 node_id,const char * name, u32 name_len,void * data, u32 data_len);//注意node_id与parent_id的对应关系
// 返回上一个匹配的节点id，找不到返回ETM_INVALID_NODE_ID;node_id不能等于tree_id
ETDLL_API u32	etm_find_prev_node(u32 tree_id,u32 parent_id,u32 node_id,const char * name, u32 name_len,void * data, u32 data_len);//注意node_id与parent_id的对应关系


//////////////////////////////////////////////////////////////
/////7 其它

/////7.1 从URL 中获得文件名和文件大小，如果不能从URL 中直接解析得到，ETM 将会查询迅雷服务器获得		[ 暂不支持获取文件大小] 
/// 注意: 目前只支持http://,https://,ftp://和thunder://,ed2k://开头的URL, url_len必须小于512
///               如果只想得到文件名，file_size可为 NULL ；如果只想得到文件大小，name_buffer可为 NULL
///               由于该接口为同步(线程阻塞)接口,因此需要设置block_time,单位为毫秒
ETDLL_API int32 etm_get_file_name_and_size_from_url(const char* url,u32 url_len,char *name_buffer,u32 *name_buffer_len,uint64 *file_size, int32 block_time );

/*--------------------------------------------------------------------------*/
/*           根据URL或cid和filesize 查询文件信息接口
----------------------------------------------------------------------------*/
typedef struct t_etm_query_shub_result
{
	void* _user_data;
	int32 _result;				/* 0:成功; 其他值为失败 */
	uint64 _file_size;
	u8 _cid[20];
	u8 _gcid[20];
	char _file_suffix[16];			/* 文件后缀 */		
} ETM_QUERY_SHUB_RESULT;
typedef int32 (*ETM_QUERY_SHUB_CALLBACK)(ETM_QUERY_SHUB_RESULT * p_query_result);

typedef struct t_etm_query_shub
{
	const char* _url;						/* 当URL为 NULL 时,下面的cid必须有效*/
	const char* _refer_url;
	
	u8 _cid[20];							/* 增加用cid 和filesize参数查询gcid的功能 */
	uint64 _file_size;
	
	ETM_QUERY_SHUB_CALLBACK _cb_ptr;	/* 不可以为 NULL */
	void* _user_data;					
} ETM_QUERY_SHUB;
#define etm_query_shub_by_url_or_cid etm_query_shub_by_url
ETDLL_API int32 etm_query_shub_by_url(ETM_QUERY_SHUB * p_param,u32 * p_action_id);
ETDLL_API int32 etm_cancel_query_shub(u32 action_id);

/* 用GCID ,CID,filesize,filename 拼凑出一个以http://pubnet.sandai.net:8080/0/ 开头的看看专用URL 
	注意:
	1.gcid和cid必须是长度为40 bytes的16进制数字字符串;
	2.filename必须为UTF8编码,中间如果有空格,必须提前以%20替换;
	3. url_buffer是用来存放即将生成的URL的内存,*url_buffer_len不得小于1024bytes.	

	生成的点播url格式：以/分割字段
	版本：0
	http://host:port/version/gcid/cid/file_size/head_size/index_size/bitrate/packet_size/packet_count/tail_pos/url_mid/filename
*/
ETDLL_API int32 etm_generate_kankan_url(const char* gcid,const char* cid,uint64 file_size,const char* file_name, char *url_buffer,u32 *url_buffer_len );

/////7.2 创建微型下载任务,回调函数中result为0表示成功【此接口已被etm_http_get 和etm_http_get_file替代,请别再使用!】
// typedef int32 ( *ETM_NOTIFY_MINI_TASK_FINISHED)(void * user_data, int32 result,void * recv_buffer,u32  recvd_len,void * p_header,u8 * send_data);
ETDLL_API int32 etm_get_mini_file_from_url(ETM_MINI_TASK * p_mini_param );

/////7.3 创建微型上传任务,回调函数中result为0表示成功【此接口已被etm_http_post替代,请别再使用!】
// typedef int32 ( *ETM_NOTIFY_MINI_TASK_FINISHED)(void * user_data, int32 result,void * recv_buffer,u32  recvd_len,void * p_header,u8 * send_data);
ETDLL_API int32 etm_post_mini_file_to_url(ETM_MINI_TASK * p_mini_param );

/////7.4 强制取消微型任务【此接口已被etm_http_cancel替代,请别再使用!】
ETDLL_API int32 etm_cancel_mini_task(u32 mini_id );

//////////////////////////////////////////////////////////////
/* HTTP会话接口的回调函数参数*/
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
	void * _user_data;			/* 用户参数 */
	ETM_HTTP_CB_TYPE _type;
	void * _header;				/* _type==EHCT_NOTIFY_RESPN时有效,指向http响应头,可用etm_get_http_header_value获取里面的详细信息*/
	
	u8 ** _send_data;			/* _type==EHCT_GET_SEND_DATA时有效, 数据分步上传时,指向需要上传的数据*/
	u32  * _send_data_len;		/* 需要上传的数据长度 */
	
	u8 * _sent_data;			/* _type==EHCT_NOTIFY_SENT_DATA时有效, 数据分步上传时,指向已经上传的数据,切记无论_sent_data_len是否为零,界面都要负责释放该内存*/
	u32   _sent_data_len;		/* 已经上传的数据长度 */
	
	void ** _recv_buffer;			/* _type==EHCT_GET_RECV_BUFFER时有效, 数据分步下载时,指向用于接收数据的缓存*/
	u32  * _recv_buffer_len;		/* 缓存大小 ,至少要16384 byte !*/
	
	u8 * _recved_data;			/* _type==EHCT_PUT_RECVED_DATA时有效, 数据分步下载时,指向已经收到的数据,切记无论_recved_data_len是否为零,界面都要负责释放该内存*/
	u32  _recved_data_len;		/* 已经收到的数据长度 */

	int32 _result;					/* _type==EHCT_NOTIFY_FINISHED时有效, 0为成功*/
} ETM_HTTP_CALL_BACK;
typedef int32 ( *ETM_HTTP_CALL_BACK_FUNCTION)(ETM_HTTP_CALL_BACK * p_http_call_back_param); 
/* HTTP会话接口的输入参数 */
typedef struct t_etm_http_get
{
	char* _url;					/* 只支持"http://" 开头的url  */
	u32 _url_len;
	
	char * _ref_url;				/* 引用页面URL*/
	u32  _ref_url_len;		
	
	char * _cookie;			
	u32  _cookie_len;		
	
	uint64  _range_from;			/* RANGE 起始位置,【暂不支持】*/
	uint64  _range_to;			/* RANGE 结束位置,【暂不支持】*/
	
	Bool  _accept_gzip;			/* 是否接受压缩文件 */
	
	void * _recv_buffer;			/* 用于接收服务器返回的响应数据的缓存*/
	u32  _recv_buffer_size;		/* _recv_buffer 缓存大小,至少要16384 byte !*/
	
	ETM_HTTP_CALL_BACK_FUNCTION _callback_fun;			/* 任务完成后的回调函数 : typedef int32 ( *ETM_HTTP_CALL_BACK_FUNCTION)(ETM_HTTP_CALL_BACK * p_http_call_back_param); */
	void * _user_data;			/* 用户参数 */
	u32  _timeout;				/* 超时,单位秒,等于0时180秒超时*/
} ETM_HTTP_GET;
typedef struct t_etm_http_post
{
	char* _url;					/* 只支持"http://" 开头的url  */
	u32 _url_len;
	
	char * _ref_url;				/* 引用页面URL*/
	u32  _ref_url_len;		
	
	char * _cookie;			
	u32  _cookie_len;		
	
	uint64  _content_len;			/* Content-Length:*/
	
	Bool  _send_gzip;			/* 是否发送压缩数据*/
	Bool  _accept_gzip;			/* 是否接受压缩数据*/
	
	u8 * _send_data;				/* 指向需要上传的数据*/
	u32  _send_data_len;			/* 需要上传的数据大小*/
	
	void * _recv_buffer;			/* 用于接收服务器返回的响应数据的缓存*/
	u32  _recv_buffer_size;		/* _recv_buffer 缓存大小,至少要16384 byte !*/

	ETM_HTTP_CALL_BACK_FUNCTION _callback_fun;			/* 任务完成后的回调函数 : typedef int32 ( *ETM_HTTP_CALL_BACK_FUNCTION)(ETM_HTTP_CALL_BACK * p_http_call_back_param); */
	void * _user_data;			/* 用户参数 */
	u32  _timeout;				/* 超时,单位秒,等于0时180秒超时*/
} ETM_HTTP_POST;
typedef struct t_etm_http_get_file
{
	char* _url;					/* 只支持"http://" 开头的url  */
	u32 _url_len;
	
	char * _ref_url;				/* 引用页面URL*/
	u32  _ref_url_len;		
	
	char * _cookie;			
	u32  _cookie_len;		
	
	uint64  _range_from;			/* RANGE 起始位置,【暂不支持】*/
	uint64  _range_to;			/* RANGE 结束位置,【暂不支持】*/
	
	Bool  _accept_gzip;			/* 是否接受压缩文件 */
	
	char _file_path[ETM_MAX_FILE_PATH_LEN]; 				/* _is_file=TRUE时, 需要上传或下载的文件存储路径，必须真实存在 */
	u32 _file_path_len;			/* _is_file=TRUE时, 需要上传或下载的文件存储路径的长度*/
	char _file_name[ETM_MAX_FILE_NAME_LEN];			/* _is_file=TRUE时, 需要上传或下载的文件名*/
	u32 _file_name_len; 			/* _is_file=TRUE时, 需要上传或下载的文件名长度*/
	
	ETM_HTTP_CALL_BACK_FUNCTION _callback_fun;			/* 任务完成后的回调函数 : typedef int32 ( *ETM_HTTP_CALL_BACK_FUNCTION)(ETM_HTTP_CALL_BACK * p_http_call_back_param); */
	void * _user_data;			/* 用户参数 */
	u32  _timeout;				/* 超时,单位秒,等于0时180秒超时*/
} ETM_HTTP_GET_FILE;

typedef struct t_etm_http_post_file
{
	char* _url;					/* 只支持"http://" 开头的url  */
	u32 _url_len;
	
	char * _ref_url;				/* 引用页面URL*/
	u32  _ref_url_len;		
	
	char * _cookie;			
	u32  _cookie_len;		
	
	uint64  _range_from;			/* RANGE 起始位置,【暂不支持】*/
	uint64  _range_to;			/* RANGE 结束位置,【暂不支持】*/
	
	Bool  _accept_gzip;			/* 是否接受压缩文件 */
	
	char _file_path[ETM_MAX_FILE_PATH_LEN]; 				/*  需要上传或下载的文件存储路径，必须真实存在 */
	u32 _file_path_len;			/* 需要上传或下载的文件存储路径的长度*/
	char _file_name[ETM_MAX_FILE_NAME_LEN];			/*  需要上传或下载的文件名*/
	u32 _file_name_len; 			/*  需要上传或下载的文件名长度*/
	
	ETM_HTTP_CALL_BACK_FUNCTION _callback_fun;			/* 任务完成后的回调函数 : typedef int32 ( *ETM_HTTP_CALL_BACK_FUNCTION)(ETM_HTTP_CALL_BACK * p_http_call_back_param); */
	void * _user_data;			/* 用户参数 */
	u32  _timeout;				/* 超时,单位秒,等于0时180秒超时*/
} ETM_HTTP_POST_FILE;
/////7.6 创建http get 会话
// typedef int32 ( *ETM_HTTP_CALL_BACK_FUNCTION)(ETM_HTTP_CALL_BACK * p_http_call_back_param);
ETDLL_API int32 etm_http_get(ETM_HTTP_GET * p_http_get ,u32 * http_id);
ETDLL_API int32 etm_http_get_file(ETM_HTTP_GET_FILE * p_http_get_file ,u32 * http_id);

/////7.7 创建http post 会话
// typedef int32 ( *ETM_HTTP_CALL_BACK_FUNCTION)(ETM_HTTP_CALL_BACK * p_http_call_back_param);
ETDLL_API int32 etm_http_post(ETM_HTTP_POST * p_http_post ,u32 * http_id );
ETDLL_API int32 etm_http_post_file(ETM_HTTP_POST_FILE * p_http_post_file ,u32 * http_id );

/////7.8 强制取消http会话,这个函数不是必须的，只有UI想在回调函数被执行之前取消该会话时才调用
ETDLL_API int32 etm_http_cancel(u32 http_id );

/////7.9 获取http头的字段值,该函数可以直接在回调函数里调用
/* http头字段*/
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
 *  设置drm证书的存放路径
 *  certificate_path:  drm证书路径
 * 返回值: 0     成功
           4201  certificate_path非法(路径不存在或路径长度大于255)
 */
ETDLL_API int32 etm_set_certificate_path(const char *certificate_path);


/* 
 *  打开xlmv文件,获取操作文件的drm_id
 *  p_file_full_path:  xlmv文件全路径
 *  p_drm_id: 返回的drm_id(一个文件对应一个,且不会重复)
 *  p_origin_file_size: 返回的drm原始文件的大小
 * 返回值: 0     成功
           22529 drm文件不存在.
           22530 drm文件打开失败.
           22531 drm文件读取失败.
           22532 drm文件格式错误.
           22537 drm证书格式错误,或者非法证书文件.
           22538 不支持openssl库,无法解密文件
 */
ETDLL_API int32 etm_open_drm_file(const char *p_file_full_path, u32 *p_drm_id, uint64 *p_origin_file_size);


/* 
 *  xlmv文件对应证书文件是否合法可用
 *  drm_id:  xlmv文件对应的drm_id
 *  p_is_ok: 是否证书文件合法可用
 * 返回值: 0     成功
           22534 无效的drm_id.
           22535 证书下载失败.
           22536 drm内部逻辑错误.
           22537 drm证书格式错误,或者非法证书文件.
           22538 不支持openssl库,无法解密文件
           
           (以下错误码需要返回给界面让用户知道)
           22628 盒子的pid绑定的用户名不存在
           22629 用户没有购买指定的影片
           22630 商品已经下架，用户将不能再购买；如果是在已经购买之后下架的，用户还是可以播放
           22631 绑定的盒子数超出限制
           22632 密钥未找到，或者是证书生成过程中出现了异常
           22633 服务器忙，一般是系统压力过大，导致查询或者绑定存在异常。如果出现这个异常，客户端可以考虑重试。
 */

ETDLL_API int32 etm_is_certificate_ok(u32 drm_id, Bool *p_is_ok);


/* 
 *  xlmv文件读取数据
 *  drm_id:      xlmv文件对应的drm_id
 *  p_buffer:    读取数据buffer
 *  size:        读取数据buffer的大小
 *  file_pos:    读取数据的起始位置
 *  p_read_size: 实际读取数据大小
 * 返回值: 0     成功
           22533 证书尚未成功
           22534 无效的drm_id.
           22536 drm内部逻辑错误.
           22537 drm证书格式错误,或者非法证书文件.
 */
ETDLL_API int32 etm_read_drm_file(u32 drm_id, char *p_buffer, u32 size, 
    uint64 file_pos, u32 *p_read_size );


/* 
 *  关闭xlmv文件
 *  drm_id:      xlmv文件对应的drm_id
 * 返回值: 0     成功
           22534 无效的drm_id.
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
 *  drm需要用到openssl库里面rsa解密相关的函数,需要外界设置对应的回调函数进来.(必须在下载库启动前调用)
 *  fun_count:    需要的回调函数个数,必须等于ET_OPENSSL_IDX_COUNT
 *  fun_ptr_table:    回调函数数组,可以是一个void*的数组
 * 返回值: 0     成功
           3273  参数不对,fun_count不为ET_OPENSSL_IDX_COUNT或fun_ptr_table为NULL或数组元素为NULL

 *  序号：0 (ET_OPENSSL_D2I_RSAPUBLICKEY_IDX)  函数原型：typedef RSA * (*et_func_d2i_RSAPublicKey)(RSA **a, _u8 **pp, _int32 length);
 *  说明：对应openssl库的d2i_RSAPublicKey函数

 *  序号：1 (ET_OPENSSL_RSA_SIZE_IDX)  函数原型：typedef _int32 (*et_func_openssli_RSA_size)(const RSA *rsa);
 *  说明：对应openssl库的RSA_size函数

 *  序号：2 (ET_OPENSSL_BIO_NEW_MEM_BUF_IDX)  函数原型：typedef BIO *(*et_func_BIO_new_mem_buf)(void *buf, _int32 len);
 *  说明：对应openssl库的BIO_new_mem_buf函数

 *  序号：3 (ET_OPENSSL_D2I_RSA_PUBKEY_BIO_IDX)  函数原型：typedef RSA *(*et_func_d2i_RSA_PUBKEY_bio)(BIO *bp,RSA **rsa);
 *  说明：对应openssl库的d2i_RSA_PUBKEY_bio函数

 *  序号：4 (ET_OPENSSL_RSA_PUBLIC_DECRYPT_IDX)  函数原型：typedef _int32(*et_func_RSA_public_decrypt)(_int32 flen, const _u8 *from, _u8 *to, RSA *rsa, _int32 padding);
 *  说明：对应openssl库的RSA_public_decrypt函数

 *  序号：5 (ET_OPENSSL_RSA_FREE_IDX)  函数原型：typedef void(*et_func_RSA_free)( RSA *r );
 *  说明：对应openssl库的RSA_free函数

 *  序号：6 (ET_OPENSSL_BIO_FREE_IDX)  函数原型：typedef void(*et_func_BIO_free)( BIO *a );
 *  说明：对应openssl库的BIO_free函数

 */

ETDLL_API int32 etm_set_openssl_rsa_interface(int32 fun_count, void *fun_ptr_table);

//////////////////////////////////////////////////////////////
/////10 查询错误码对应的简要描述。注意1~1024的描述在不同的平台可能有所不同
ETDLL_API const char * etm_get_error_code_description(int32 error_code);


//////////////////////////////////////////////////////////////
/////11 json接口
/* json数据通道，输入一个json调用，在输出buffer上得到一个json语句
 *p_input_json  输入json调用
 *p_output_json_buffer  输出json的buffer，由外界维护buffer
 *p_buffer_len  开始赋值为buffer的大小，如果函数返回105476,  p_buffer_len返回需要的buffer长度
 * 返回值: 0     成功
 *                    105476    输出buffer长度不够
 */
ETDLL_API int32 etm_mc_json_call( const char *p_input_json, char *p_output_json_buffer, u32 *p_buffer_len );



//////////////////////////////////////////////////////////////
/////12 指定网段内搜索服务器的相关接口
//服务器协议类型
typedef enum t_etm_server_type
{
         EST_HTTP=0, 
         EST_FTP            //暂不支持，主要用于以后扩展用
} ETM_SERVER_TYPE;

//服务器信息
typedef struct t_etm_server
{
         ETM_SERVER_TYPE _type;
         u32 _ip;
         u32 _port;
		 u32 _file_num;
		 Bool _has_password;
		 char _ip_addr[16];
		 char _pc_name[64];
         char _description[256];  // 服务器描述，比如"version=xl7.xxx&pc_name; file_num=999; peerid=864046003239850V; ip=192.168.x.x; tcp_port=8080; udp_port=10373; peer_capability=61546"
} ETM_SERVER;

//找到服务器的回调函数类型定义
typedef int32 ( *ETM_FIND_SERVER_CALL_BACK_FUNCTION)(ETM_SERVER * p_server);

//搜索完毕通知回调函数类型定义，result等于0为成功，其他值为失败
typedef int32 ( *ETM_SEARCH_FINISHED_CALL_BACK_FUNCTION)(int32 result);

//搜索输入参数
typedef struct t_etm_search_server
{
         ETM_SERVER_TYPE _type;
         u32 _ip_from;           /* 初始ip，本地序，如"192.168.1.1"为3232235777，填0表示只扫描与本地ip同一子网的主机 */
         u32 _ip_to;               /* 结束ip，本地序，如"192.168.1.254"为3232236030，填0表示只扫描与本地ip同一子网的主机*/
         int32 _ip_step;         /* 扫描步进*/
         
         u32 _port_from;         /*初始端口，本地序*/
         u32 _port_to;           /* 结束端口，本地序*/
         int32 _port_step;    /* 端口扫描步进*/
         
         ETM_FIND_SERVER_CALL_BACK_FUNCTION _find_server_callback_fun;             /* 每找到一个服务器，etm就回调一下该函数，将该服务器的信息传给UI */
         ETM_SEARCH_FINISHED_CALL_BACK_FUNCTION _search_finished_callback_fun;     /* 搜索完毕 */
} ETM_SEARCH_SERVER;

//开始搜索
ETDLL_API int32 etm_start_search_server(ETM_SEARCH_SERVER * p_search);

//停止搜索,如果该接口在ETM回调_search_finished_callback_fun之前调用，则ETM不会再回调_search_finished_callback_fun
ETDLL_API int32 etm_stop_search_server(void);
ETDLL_API int32 etm_restart_search_server(void);

//////////////////////////////////////////////////////////////
/////14 离线空间相关接口
//	注意:所有char *字符串均为UTF8格式!

/* 向下载库设置迅雷用户帐号信息,new_user_name必须是数字串  */
ETDLL_API int32 etm_lixian_set_user_info(uint64 user_id,const char * new_user_name,const char * old_user_name,int32 vip_level,const char * session_id,char* jumpkey, u32 jumpkey_len);

/* 用户调用etm_member_relogin重新登录之后需将更新的sessionid重新设置到离线模块  */
ETDLL_API int32 etm_lixian_set_sessionid(const char * session_id);

/* 向下载库设置jump key*/
ETDLL_API int32 etm_lixian_set_jumpkey(char* jumpkey, u32 jumpkey_len);

/* 获取用户离线空间信息,单位Byte */
ETDLL_API int32 etm_lixian_get_space(uint64 * p_max_space,uint64 * p_available_space);

/* 获取点播时的cookie */
ETDLL_API const char * etm_lixian_get_cookie(void);

/* 取消某个异步操作 */
ETDLL_API int32 etm_lixian_cancel(u32 action_id);

/* 退出离线空间 */
ETDLL_API int32 etm_lixian_logout(void);

/* 创建离线任务返回相关信息*/
typedef struct t_elx_login_result
{
	u32 _action_id;
	void * _user_data;							/* 用户参数 */
	int32 _result;								/*	0:成功；其他值:失败*/

	/* 用户相关*/
	u8 _user_type;								/* 用户类型 */
	u8 _vip_level;								/* 用户vip等级 */
	uint64 _available_space;					/* 可用空间 */
	uint64 _max_space;							/* 用户空间 */
	uint64 _max_task_num;						/* 用户最大任务数 */
	uint64 _current_task_num;					/* 当前任务数 */

} ELX_LOGIN_RESULT;
typedef int32 ( *ELX_LIXIAN_LOGIN_CALLBACK)(ELX_LOGIN_RESULT * p_result);
/* 离线登录参数*/
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

/* 功能:
	1. 向下载库设置迅雷用户帐号信息,new_user_name必须是数字串。
	2. 向下载库设置jump key。
	3. 登录离线空间。
   参数: ELX_LOGIN_INFO
	用户登录之后获取到的信息传递过来进行离线登录。
*/
ETDLL_API int32 etm_lixian_login(ELX_LOGIN_INFO* p_param, u32 * p_action_id);
/* 功能: 获取离线列表是否有缓存(主要用于判断是否第一次拉取离线列表)
   参数: 
   		userid: 用户id
   		is_cache: 是否有任务列表缓存
*/
ETDLL_API int32 etm_lixian_has_list_cache(uint64 userid, BOOL *is_cache);

/* 向离线空间提交下载任务*/
/* 离线任务状态*/

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

/* 离线任务类型 */
typedef enum t_elx_task_type
{
	ELXT_UNKNOWN = 0, 				
	ELXT_HTTP , 					
	ELXT_FTP,				
	ELXT_BT,			// 无用	
	ELXT_EMULE,				
	ELXT_BT_ALL,				
	ELXT_BT_FILE,				
	ELXT_SHOP				
} ELX_TASK_TYPE;
/* 创建离线任务返回相关信息*/
typedef struct t_elx_create_task_result
{
	u32 _action_id;
	void * _user_data;							/* 用户参数 */
	int32 _result;									/*	0:成功；其他值:失败*/

	/* 用户相关*/
	uint64 _available_space;						/* 可用空间 */
	uint64 _max_space;							/* 用户空间 */
	uint64 _max_task_num;							/* 用户最大任务数 */
	uint64 _current_task_num;						/* 当前任务数 */

	/* 扣费相关*/
	BOOL _is_goldbean_converted;					/* 是否转换了金豆*/
	uint64 _goldbean_total_num;					/* 金豆总额*/
	uint64 _goldbean_need_num;					/* 需要的金豆数量*/
	uint64 _goldbean_get_space;					/* 转换豆豆获得的空间大小*/
	uint64 _silverbean_total_num;					/* 银豆数量*/
	uint64 _silverbaen_need_num;					/* 需要的银豆数量*/

	/* 任务相关*/
	uint64 _task_id;
	uint64 _file_size; 
	ELX_TASK_STATE _status;
	ELX_TASK_TYPE  _type;
	u32 _progress;								/*任务进度，除以100即为百分比，10000即100%*/
} ELX_CREATE_TASK_RESULT;

typedef int32 ( *ELX_CREATE_TASK_CALLBACK)(ELX_CREATE_TASK_RESULT * p_result);

/* 创建下载任务参数*/
typedef struct t_elx_create_task_info
{
	char _url[ETM_MAX_URL_LEN];
	char _ref_url[ETM_MAX_URL_LEN];
	char _task_name[ETM_MAX_FILE_NAME_LEN];
	u8 _cid[20];			
	u8 _gcid[20];
	uint64 _file_size;
	BOOL _is_auto_charge;					/* 当空间不足且有金豆时是否自动扣金豆 */
	
	void * _user_data;		
	ELX_CREATE_TASK_CALLBACK _callback_fun;
} ELX_CREATE_TASK_INFO;

/* 向离线空间提交下载任务,使用该接口前需设置过jump key*/
ETDLL_API int32 etm_lixian_create_task(ELX_CREATE_TASK_INFO * p_param,u32 * p_action_id);


/* 用Torrent种子文件或磁力链创建bt离线任务参数*/
typedef struct t_elx_create_bt_task_info
{
	char* _title;							/* 任务的标题,utf8编码,最长255字节,填NULL表示直接用torrent里面的title 或用磁力链里的infohash做标题*/
	char* _seed_file_full_path; 			/* 用torrent创建任务时，这里就是任务的种子文件(*.torrent) 全路径 */
	char* _magnet_url;					/* 用磁力链创建任务时，这里就是磁力链*/
	u32 _file_num;						/* 需要下载的子文件个数*/
	u32* _download_file_index_array; 	/* 需要下载的子文件序号列表，为NULL 表示下载所有有效的子文件*/

	BOOL _is_auto_charge;				/* 当空间不足且有金豆时是否自动扣金豆 */
	
	void * _user_data;		
	ELX_CREATE_TASK_CALLBACK _callback_fun;
} ELX_CREATE_BT_TASK_INFO;

/* 向离线空间提交bt 离线任务,使用该接口前需设置过jump key*/
ETDLL_API int32 etm_lixian_create_bt_task(ELX_CREATE_BT_TASK_INFO * p_param,u32 * p_action_id);



/* 离线任务文件类型 */
typedef enum t_elx_file_type
{
	ELXF_UNKNOWN = 0, 				
	ELXF_VEDIO , 					
	ELXF_PIC , 					
	ELXF_AUDIO , 					
	ELXF_ZIP , 		/* 各种压缩文档,如zip,rar,tar.gz... */			
	ELXF_TXT , 					
	ELXF_APP,		/* 各种软件,如exe,apk,dmg,rpm... */			
	ELXF_OTHER					
} ELX_FILE_TYPE;


/* 离线任务信息 */
typedef struct t_elx_task_info
{
	uint64 _task_id;
	ELX_TASK_TYPE _type; 						
	ELX_TASK_STATE _state;
	char _name[ETM_MAX_FILE_NAME_LEN];		/* 任务名称,utf8编码*/
	uint64 _size;								/* 任务文件的大小,BT任务时为所有子文件的总大小,单位byte*/
	int32 _progress;							/* 如果任务正在下载中,此为下载进度*/

	char _file_suffix[16]; 						/* 非BT任务的文件后缀名,如: txt,exe,flv,avi,mp3...*/ 
	u8 _cid[20];								/* 非BT任务的文件cid ,bt任务为info hash */
	u8 _gcid[20];								/* 非BT任务的文件gcid */
	Bool _vod;								/* 非BT任务的文件是否可点播 */
	char _url[ETM_MAX_URL_LEN];				/* 非BT任务的下载URL,utf8编码*/
	char _cookie[ETM_MAX_URL_LEN];			/* 非BT任务的下载URL 对应的cookie */

	u32 _sub_file_num;						/* BT任务中表示该任务子文件个数(离线BT任务无子文件夹) */
	u32 _finished_file_num;					/* BT任务中表示该任务里已完成(失败或成功)的子文件个数*/

	u32 _left_live_time;						/* 剩余天数 */
	char _origin_url[ETM_MAX_URL_LEN];				/* 非BT任务的原始下载URL*/

	u32 _origin_url_len;						/* 非BT任务的原始下载URL 长度*/
	u32 _origin_url_hash;					/* 非BT任务的原始下载URL 的hash值*/
} ELX_TASK_INFO;

/* 获取列表响应,内存由下载库释放,UI需要拷贝一份再切换到UI线程中处理 */
typedef struct t_elx_task_list
{
	u32 _action_id;
	void * _user_data;				/* 用户参数 */
	int32 _result;						/*	0:成功,其他值:失败	*/
	u32 _total_task_num;			/* 该用户离线空间内的总任务个数*/
	u32 _task_num;					/* 此次交互获得的任务个数 */
	uint64 _total_space;			/* 用户离线总空间，单位字节 */
	uint64 _available_space;			/* 用户离线可用空间，单位字节 */
	ELX_TASK_INFO * _task_array;		/* 注意:此参数已作废! 任务信息必须用etm_lixian_get_task_info接口获取 */
} ELX_TASK_LIST;
typedef int32 ( *ELX_GET_TASK_LIST_CALLBACK)(ELX_TASK_LIST * p_task_list);

/* 获取离线空间任务列表的输入参数*/
typedef struct t_elx_get_task_ls
{
	ELX_FILE_TYPE _file_type;			/*  为指定获取具有特定类型的文件的任务列表*/
	int32 _file_status;				/*  为指定具有特定状态的文件的任务列表,0:全部；1:查询正在下载中任务；2:完成*/
	int32 _offset;						/*  表示在任务个数太多时分批获取的偏移值,从0开始*/
	int32 _max_task_num;			/*  为此次获取最大任务个数   */
	
	void * _user_data;				/* 用户参数 */
	ELX_GET_TASK_LIST_CALLBACK _callback;
} ELX_GET_TASK_LIST;

/* 获取离线空间任务列表
	p_action_id 返回该异步操作的id，用于中途取消该操作*/
ETDLL_API int32 etm_lixian_get_task_list(ELX_GET_TASK_LIST * p_param,u32 * p_action_id);


typedef struct t_elx_od_or_del_task_list
{
	u32 _action_id;
	void * _user_data;				/* 用户参数 */
	int32 _result;						/*	0:成功,其他值:失败	*/
	u32 _task_num;					/* 此次交互获得的任务个数 */
} ELX_OD_OR_DEL_TASK_LIST;
typedef int32 ( *ELX_GET_OD_OR_DEL_TASK_LIST_CALLBACK)(ELX_OD_OR_DEL_TASK_LIST * p_task_list);
/* 获取离线空间过期或者已删除任务列表的输入参数*/
typedef struct t_elx_od_or_del_get_task_ls
{
	int32 _task_type;				/*  0:已删除；1:过期*/
	int32 _page_offset;			/*  表示在任务个数太多时分批获取的偏移值,从0开始*/
	int32 _max_task_num;			/*  每页的任务数 */
	
	void * _user_data;				/* 用户参数 */
	ELX_GET_OD_OR_DEL_TASK_LIST_CALLBACK _callback;
} ELX_GET_OD_OR_DEL_TASK_LIST;

/* 获取离线空间过期或者已删除任务的列表
	p_action_id 返回该异步操作的id，用于中途取消该操作*/
ETDLL_API int32 etm_lixian_get_overdue_or_deleted_task_list(ELX_GET_OD_OR_DEL_TASK_LIST * p_param,u32 * p_action_id);

/* 功能: 根据任务的不同状态获取离线任务id 列表。
   参数: 
     state: 范围 0-127, 根据如下几个状态值与运算取需要获取的几个状态值	
     state = 0表示获取所有状态id列表
	    	ELXS_WAITTING =1, 
			ELXS_RUNNING  =2, 
			ELXS_PAUSED   =4,
			ELXS_SUCCESS  =8, 
			ELXS_FAILED   =16, 
			ELXS_OVERDUE  =32,
			ELXS_DELETED  =64
     id_array_buffer: 不知道某状态对应的id数目时，第一次调用传入NULL，根据返回的buffer_len，分配空间第二次传入获取id列表
     buffer_len: 需要获取的id列表数目，第一次id_array_buffer为NULL时，该值填0即可。
*/
ETDLL_API int32 etm_lixian_get_task_ids_by_state(int32 state,uint64 * id_array_buffer,u32 *buffer_len);

/* 获取离线任务信息 */
ETDLL_API int32 etm_lixian_get_task_info(uint64 task_id,ELX_TASK_INFO * p_info);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* 获取离线空间BT 任务的子文件列表*/


/* BT任务子文件信息 */
typedef struct t_elx_file_info
{
	uint64 _file_id; 						
	ELX_TASK_STATE _state;
	char _name[ETM_MAX_FILE_NAME_LEN];		/* 文件名,utf8编码*/
	
	uint64 _size;								/* 文件大小,单位byte*/
	int32 _progress;							/* 如果文件正在下载中,此为下载进度*/

	char _file_suffix[16]; 						/* 文件后缀名,如: txt,exe,flv,avi,mp3...*/ 
	u8 _cid[20];								/* 文件cid */
	u8 _gcid[20];								/* 文件gcid */
	Bool _vod;								/* 文件是否可点播 */
	char _url[ETM_MAX_URL_LEN];				/* 文件的下载URL,utf8编码*/
	char _cookie[ETM_MAX_URL_LEN];			/* 文件的下载URL 对应的cookie */
	u32 _file_index;							/* BT 子文件在bt任务中的文件序号,从0开始 */
} ELX_FILE_INFO;

/* 获取列表响应,内存由下载库释放,UI需要拷贝一份再切换到UI线程中处理 */
typedef struct t_elx_file_list
{
	u32 _action_id;
	uint64 _task_id;
	void * _user_data;				/* 用户参数 */
	int32 _result;						/*	0:成功,其他值:失败	*/
	u32 _total_file_num;				/* 该BT 任务内的总文件个数*/
	u32 _file_num;					/* 此次交互获得的文件个数 */
	ELX_FILE_INFO * _file_array;		/* 注意:此参数已作废! 子文件信息必须用etm_lixian_get_bt_sub_file_info接口获取 */
} ELX_FILE_LIST;
typedef int32 ( *ELX_GET_FILE_LIST_CALLBACK)(ELX_FILE_LIST * p_file_list);

/* 获取离线空间BT 任务的子文件列表的输入参数*/
typedef struct t_elx_get_bt_file_ls
{
	uint64 _task_id;					/* BT 任务id */
	ELX_FILE_TYPE _file_type;			/*  为指定获取bt任务中特定类型的文件*/
	int32 _file_status;				/*  为指定bt任务中特定状态的文件,0:全部；1:查询正在下载中任务；2:完成*/
	int32 _offset;						/*  表示在子文件个数太多时分批获取的偏移值,从0开始*/
	int32 _max_file_num;				/*  为此次获取最大文件个数   */
	
	void * _user_data;				/* 用户参数 */
	ELX_GET_FILE_LIST_CALLBACK _callback;
} ELX_GET_BT_FILE_LIST;

/* 获取离线空间BT 任务的子文件列表
	p_action_id 返回该异步操作的id，用于中途取消该操作*/
ETDLL_API int32 etm_lixian_get_bt_task_file_list(ELX_GET_BT_FILE_LIST * p_param,u32 * p_action_id);

/* 功能: 根据任务的不同状态获取离线任务id 列表。
   参数: 
     state: 范围 0-127, 根据如下几个状态值与运算取需要获取的几个状态值	
     state = 0表示获取所有状态id列表
	    	ELXS_WAITTING =1, 
			ELXS_RUNNING  =2, 
			ELXS_PAUSED   =4,
			ELXS_SUCCESS  =8, 
			ELXS_FAILED   =16, 
			ELXS_OVERDUE  =32,
			ELXS_DELETED  =64
     id_array_buffer: 不知道某状态对应的id数目时，第一次调用传入NULL，根据返回的buffer_len，分配空间第二次传入获取id列表
     buffer_len: 需要获取的id列表数目，第一次id_array_buffer为NULL时，该值填0即可。
*/
ETDLL_API int32 etm_lixian_get_bt_sub_file_ids_by_state(uint64 task_id,int32 state,uint64 * id_array_buffer,u32 *buffer_len);

/////获取离线BT任务中某个子文件的信息
ETDLL_API int32 etm_lixian_get_bt_sub_file_info(uint64 task_id, uint64 file_id, ELX_FILE_INFO *p_file_info);
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/* 获取视频截图,截图为jpg格式 */
typedef struct t_elx_screenshot_result
{
	u32 _action_id;
	void * _user_data;							/* 用户参数 */
	char  _store_path[ETM_MAX_FILE_PATH_LEN]; 	/* 依样传回 截图存放路径*/
	Bool _is_big;
	u32 _file_num;
	u8 * _gcid_array;
	int32 * _result_array;							/*	各文件的截图获取结果:0 成功,其他值:失败	*/
} ELX_SCREENSHOT_RESUTL;
typedef int32 ( *ELX_GET_SCREENSHOT_CALLBACK)(ELX_SCREENSHOT_RESUTL * p_screenshot_result);

/* 获取视频截图的输入参数*/
typedef struct t_elx_get_screenshot
{
	u32 _file_num;								/* file_num 需要获取截图的文件个数 */
	u8 * _gcid_array;							/* gcid_array 存放各个需要获取截图的文件gcid,均为20byte的数字串 */
	
	Bool _is_big;								/* is_big 是否获取大图，TRUE为获取 256*192的截图,FALSE为获取128*96	的截图 */
	const char * _store_path;						/* store_path为截图存放路径,必须可写,下载库将把各gcid对应的文件截图下载到该目录中,并以gcid作为文件名,截图为jpg后缀 */
	Bool _overwrite;
	
	void * _user_data;							/* 用户参数 */
	ELX_GET_SCREENSHOT_CALLBACK _callback;
} ELX_GET_SCREENSHOT;

/* 获取视频截图,截图为jpg格式 
	p_action_id 返回该异步操作的id，用于中途取消该操作*/
ETDLL_API int32 etm_lixian_get_screenshot(ELX_GET_SCREENSHOT * p_param,u32 * p_action_id);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* 获取文件的云点播的URL */

/* 用"视频指纹"技术查询到的同一部电影的其他候选点播URL 的相关信息*/
typedef struct t_elx_fp_info
{
	uint64 _size;								/* 候选点播资源的文件大小,单位byte*/
	u8 _cid[20];								/* 候选点播资源的文件cid */			
	u8 _gcid[20];								/* 候选点播资源的文件gcid */		
	u32 _video_time;							/* 候选点播资源的视频时间长度,单位: s*/
	u32 _bit_rate;							/* 候选点播资源的码率【服务器暂不提供该参数】*/
	char _vod_url[ETM_MAX_URL_LEN];    		/* 候选点播资源的点播URL*/
	char _screenshot_url[ETM_MAX_URL_LEN];    	/* 候选点播资源的截图URL*/
} ELX_FP_INFO;

/*云点播相关信息*/
typedef struct t_elx_get_vod_result
{
	u32 _action_id;
	void * _user_data;							/* 用户参数 */
       int32 _result;									/*	0:成功；其他值:失败*/

	//uint64 _size;							
	u8 _cid[20];							
	u8 _gcid[20];							
	//char _sheshou_cid[64];    						/*射手cid,用于查询字幕*/
	u32 _video_time;								/*视频时间长度,单位: s*/

	u32 _normal_bit_rate;						/* 普清码率【服务器暂不提供该参数】*/
	uint64 _normal_size;							/* 普清视频文件的大小 */						
	char _normal_vod_url[ETM_MAX_URL_LEN];    	/* 普清点播URL*/
	
	u32 _high_bit_rate;							/* 高清码率【服务器暂不提供该参数】*/
	uint64 _high_size;							/* 高清视频文件的大小 */				
	char _high_vod_url[ETM_MAX_URL_LEN];    		/* 高清点播URL*/

	/* 两种不同码率格式 */
	u32 _fluency_bit_rate_1;							/* 流畅码率【服务器暂不提供该参数】*/
	uint64 _fluency_size_1;							/* 流畅视频文件的大小 */				
	char _fluency_vod_url_1[ETM_MAX_URL_LEN];    		/* 流畅点播URL*/

	u32 _fluency_bit_rate_2;							/* 流畅码率【服务器暂不提供该参数】*/
	uint64 _fluency_size_2;							/* 流畅视频文件的大小 */				
	char _fluency_vod_url_2[ETM_MAX_URL_LEN];    		/* 流畅点播URL*/
	
	u32 _fpfile_num;								/* 用"视频指纹"技术查询到的同一部电影的其他候选点播URL 的个数*/
	ELX_FP_INFO * _fp_array;						/* 其他候选点播URL 数组*/
} ELX_GET_VOD_RESULT;
typedef int32 ( *ELX_GET_VOD_URL_CALLBACK)(ELX_GET_VOD_RESULT * p_result);

/* 获取视频文件的云点播URL 的输入参数*/
typedef struct t_elx_get_vod
{
	u32 _device_width;
	u32 _device_height;
	u32 _video_type;							/*	视频格式: 0:flv(兼容旧版本);  1:flv;  2:ts;  3:保留;  4: mp4  */
		
	uint64 _size;								/* 文件大小,单位byte*/
	u8 _cid[20];								
	u8 _gcid[20];								
	//Bool _is_after_trans;					/* 是否是转码后的文件*/
	u32 _max_fpfile_num;					/* 用"视频指纹"技术查询同一部电影的其他候选点播URL 的最大个数*/

	void * _user_data;						/* 用户参数 */
	ELX_GET_VOD_URL_CALLBACK _callback_fun;
} ELX_GET_VOD;

/* 获取视频文件的云点播URL 
	p_action_id 返回该异步操作的id，用于中途取消该操作*/
ETDLL_API int32 etm_lixian_get_vod_url(ELX_GET_VOD * p_param,u32 * p_action_id);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* 删除离线任务返回相关信息*/
typedef struct t_elx_delete_task_result
{
	u32 _action_id;
	void * _user_data;							/* 用户参数 */
	int32 _result;									/*	0:成功；其他值:失败*/
} ELX_DELETE_TASK_RESULT;

typedef int32 ( *ELX_DELETE_TASK_CALLBACK)(ELX_DELETE_TASK_RESULT * p_result);

/* 向离线空间删除下载任务,使用该接口前需设置过jump key*/
ETDLL_API int32 etm_lixian_delete_task(uint64 task_id, void* user_data, ELX_DELETE_TASK_CALLBACK callback_fun, u32 * p_action_id);
/* 删除过期任务*/
ETDLL_API int32 etm_lixian_delete_overdue_task(uint64 task_id, void* user_data, ELX_DELETE_TASK_CALLBACK callback_fun, u32 * p_action_id);

/* 批量删除离线任务返回相关信息*/
typedef struct t_elx_delete_tasks_result
{
	u32 _action_id;
	void * _user_data;							/* 用户参数 */
	u32 _task_num;								/* 批量删除任务个数 */
	uint64 * _p_task_ids;							/* 要删除的任务id列表(下载库分配的内存，界面不能删除) */
	int32 *_p_results;							/*	任务id对应的操作结果,0:成功；其他值:失败(下载库分配的内存，界面不能删除) */
} ELX_DELETE_TASKS_RESULT;

typedef int32 ( *ELX_DELETE_TASKS_CALLBACK)(ELX_DELETE_TASKS_RESULT * p_result);
/* 向离线空间批量删除下载任务,使用该接口前需设置过jump key*/
ETDLL_API int32 etm_lixian_delete_tasks(u32 task_num,uint64 * p_task_ids,BOOL is_overdue, void* user_data, ELX_DELETE_TASKS_CALLBACK callback_fun, u32 * p_action_id);


/* 查询离线任务信息返回的数据 (目前只返回部分UI需要数据，如还需其他数据，改造该结构体并在内部将数据返回即可) */
typedef struct t_elx_query_task_info_result
{
	u32 _action_id;
	void * _user_data;							/* 用户参数 */
	int32 _result;								/*	0:成功；其他值:失败*/
	//ELX_TASK_STATE _download_status;			/*	下载状态*/
	//int32 _progress;							/*	下载进度*/
} ELX_QUERY_TASK_INFO_RESULT;
typedef int32 ( *ELX_QUERY_TASK_INFO_CALLBACK)(ELX_QUERY_TASK_INFO_RESULT * p_result);
/* 查询任务信息(异步接口)，目前只支持单个任务信息查询(需要支持多个任务，则需改造该接口) task_id 为需要查询的任务id数组，task_num为需要查询的任务数目
   callback_fun为该异步操作的回调函数, p_action_id 返回该异步操作的id，用于中途取消该操作*/
ETDLL_API int32 etm_lixian_query_task_info(uint64 *task_id, u32 task_num, void* user_data, ELX_GET_TASK_LIST_CALLBACK callback_fun, u32 * p_action_id);

/* 查询BT主任务信息，可支持单个或者多个任务的任务信息查询 
   参数: task_id 为需要查询的任务id数组;
   		 task_num为需要查询的任务数目;
   		 callback_fun为该异步操作的回调函数;
   		 p_action_id 返回该异步操作的id，用于中途取消该操作
*/
ETDLL_API int32 etm_lixian_query_bt_task_info(uint64 *task_id, u32 task_num, void* user_data, ELX_QUERY_TASK_INFO_CALLBACK callback_fun, u32 * p_action_id);

/* 函数名称: 已过期任务重新下载接口(异步接口)
   函数功能: 对已过期任务重新下载操作。(不会删除已过期任务列表中的文件，需删除调用其他接口)
   函数参数: 
     task_id 为需要重新下载的已过期任务id; (对已过期任务重新下载后的返回任务id与原来的id是不一样的)
     user_data 用户参数，回调函数中传出;
     callback 为已过期任务的重新创建后的回调函数
     p_action_id 返回该异步操作的id，用于中途取消该操作
*/
ETDLL_API int32 etm_lixian_create_task_again(uint64 task_id, void* user_data, ELX_CREATE_TASK_CALLBACK callback, u32 * p_action_id);


/* 续期离线任务返回相关信息*/
typedef struct t_elx_delay_task_result
{
	u32 _action_id;
	void * _user_data;							/* 用户参数 */
	int32 _result;									/*	0:成功；其他值:失败*/
	u32 _left_live_time;							/*  剩余天数 */	
} ELX_DELAY_TASK_RESULT;

typedef int32 ( *ELX_DELAY_TASK_CALLBACK)(ELX_DELAY_TASK_RESULT * p_result);

/* 向离线空间续期下载任务,使用该接口前需设置过jump key*/
ETDLL_API int32 etm_lixian_delay_task(uint64 task_id, void* user_data, ELX_DELAY_TASK_CALLBACK callback_fun, u32 * p_action_id);	

/* 精简查询离线任务返回相关信息*/
typedef struct t_elx_miniquery_task_result
{
	u32 _action_id;
	void * _user_data;							/* 用户参数 */
	int32 _result;									/*	0:成功；其他值:失败*/
	u32 _state;									/*	0:任务存在；3: 任务不存在 无其他值*/
	uint64 _commit_time;									/*  提交时间 */	
	u32 _left_live_time;	   					/* 剩余天数 */	
} ELX_MINIQUERY_TASK_RESULT;

typedef int32 ( *ELX_MINIQUERY_TASK_CALLBACK)(ELX_MINIQUERY_TASK_RESULT * p_result);

ETDLL_API int32 etm_lixian_miniquery_task(uint64 task_id, void* user_data, ELX_MINIQUERY_TASK_CALLBACK callback_fun, u32 * p_action_id);
	
///// 根据URL ,cid,gicd 或BT种子的info_hash 查找对应的离线任务id
ETDLL_API int32 	etm_lixian_get_task_id_by_eigenvalue(ETM_EIGENVALUE * p_eigenvalue,uint64 * task_id);

/* 函数名称: etm_lixian_get_task_id_by_gcid(同步接口)
   函数功能: 通过gcid获取离线任务的id, 如果是bt子任务，则还可获取到bt子任务的文件id。(通过该函数获取到id之后即可得到离线任务的相关信息)
   函数参数: 
     p_gcid  : 传入的40字节的gcid值。
     task_id : 函数返回的离线任务id保存于此;
     file_id : 如果是bt子任务，则子任务的文件id保存于此，否则该值为0。
*/
ETDLL_API int32 etm_lixian_get_task_id_by_gcid(char * p_gcid, uint64 * task_id, uint64 * file_id);

/* 从网页中嗅探可供下载的url  */
typedef struct t_elx_downloadable_url_result
{
	u32 _action_id;
	void * _user_data;						/* 用户参数 */
	char _url[ETM_MAX_URL_LEN];				/* 请求时传进下载库被嗅探到URL */
	int32 _result;								/*	0:成功；其他值:失败,后面的字段无效*/
	u32 _url_num;							/*  发现可供下载的URL 个数 */
} ELX_DOWNLOADABLE_URL_RESULT;

typedef int32 ( *ELX_DOWNLOADABLE_URL_CALLBACK)(ELX_DOWNLOADABLE_URL_RESULT * p_result);

ETDLL_API int32 etm_lixian_get_downloadable_url_from_webpage(const char* url,void* user_data,ELX_DOWNLOADABLE_URL_CALLBACK callback_fun , u32 * p_action_id);

/* 从已经拉取到的网页源文件中嗅探可供下载的url ,注意,这个是同步接口，不需要网络交互  */
ETDLL_API int32 etm_lixian_get_downloadable_url_from_webpage_file(const char* url,const char * source_file_path,u32 * p_url_num);

///// 获取网页中可供下载URL的id 列表,*buffer_len的单位是sizeof(u32)
ETDLL_API int32 etm_lixian_get_downloadable_url_ids(const char* url,u32 * id_array_buffer,u32 *buffer_len);

/* 获取可供下载URL 信息 */
typedef enum t_elx_url_type
{
	ELUT_ERROR = -1,
	ELUT_HTTP = 0, 				
	ELUT_FTP, 
	ELUT_THUNDER, 
	ELUT_EMULE, 
	ELUT_MAGNET, 				/* 磁力链 */
	ELUT_BT, 					/* 指向torrent 文件的url */
} ELX_URL_TYPE;
typedef enum t_elx_url_status
{
	ELUS_UNTREATED = 0, 		/* 未处理 */			
	ELUS_IN_LOCAL, 				/* 已经下载到本地 */
	ELUS_IN_LIXIAN				/* 已经在离线空间里了 */
} ELX_URL_STATUS;
typedef struct t_elx_downloadable_url
{
	u32 _url_id;
	ELX_URL_TYPE _url_type;				/* URL 类型*/
	ELX_FILE_TYPE _file_type;				/*  URL 指向的文件类型*/
	ELX_URL_STATUS _status;				/* 该URL 的处理状态*/
	uint64 _file_size;
	char _name[ETM_MAX_FILE_NAME_LEN];
	char _url[ETM_MAX_URL_LEN];
} ELX_DOWNLOADABLE_URL;

ETDLL_API int32 etm_lixian_get_downloadable_url_info(u32 url_id,ELX_DOWNLOADABLE_URL * p_info);

/* 判断该url 是否可供下载 */
ETDLL_API BOOL etm_lixian_is_url_downloadable(const char* url);

/* 获取url的类型 */
ETDLL_API  ELX_URL_TYPE etm_lixian_get_url_type(const char* url);

typedef struct t_elx_take_off_flux_result
{
	u32 _action_id;
	void * _user_data;						/* 用户参数 */
	int32 _result;							/*	0:成功；其他值:失败*/
	uint64 _all_flux;						/*  总流量 */	
	uint64 _remain_flux;					/*  剩余流量 */	
	uint64 _lixian_taskid;				/*  离线任务id */
	uint64 _lixian_fileid;				/*  离线任务id */	
} ELX_TAKE_OFF_FLUX_RESULT;

typedef int32 ( *ELX_TAKE_OFF_FLUX_CALLBACK)(ELX_TAKE_OFF_FLUX_RESULT * p_result);
/* 函数名称: etm_lixian_take_off_flux_from_high_speed(异步接口)
   函数功能: 从高速服务器扣除流量，以使免费用户的离线url能够作为加速url使用。
   函数参数: 
     task_id  :   离线任务对应的task_id;
     file_id  :   离线BT任务对应的子任务file_id,不是bt任务填0即可;
     user_data :  用户携带的数据，回调函数里返回;
     callback_fun: 获取相关信息的回调函数;
     p_action_id:  异步操作的id; 
*/
ETDLL_API int32 etm_lixian_take_off_flux_from_high_speed(uint64 task_id, uint64 file_id, void* user_data, ELX_TAKE_OFF_FLUX_CALLBACK callback_fun, u32 * p_action_id);

typedef struct t_free_strategy_result
{
	u32 _action_id;
	void * _user_data;						/* 用户参数 */
	int32 _result;							/*	0:成功；-1: 超时; 其他值:失败*/
	uint64 _user_id;						/*  用户id */
	BOOL _is_active_stage;				/*  是否在活动期内 */	
	u32 _free_play_time;					/*  试播时间 */
} ETM_FREE_STRATEGY_RESULT;
typedef int32 ( *ETM_FREE_STRATEGY_CALLBACK)(ETM_FREE_STRATEGY_RESULT * p_param);
/* 函数名称: etm_get_free_strategy(异步接口)
   函数功能: 从服务器获取免费策略相关的信息。
   函数参数: 
     user_id  :   用户id;
     session_id : 用户登录的session_id;
     user_data :  用户携带的数据，回调函数里返回;
     callback_fun: 获取相关信息的回调函数;
     p_action_id:  异步操作的id;
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
	void * _user_data;				/* 用户参数 */
	int32 _result;					/*	0:成功；-1: 超时; 其他值:失败*/
	uint64 _user_id;				/*  用户id */
	ETM_CS_AC_TYPE _type;			/*  响应类型 */
	u32 _free_time;				/*  体验天数 */
	BOOL _is_first_use;			/*  是否第一次使用 */
} ETM_FREE_EXPERIENCE_MEMBER_RESULT;
typedef int32 ( *ETM_FREE_EXPERIENCE_MEMBER_CALLBACK)(ETM_FREE_EXPERIENCE_MEMBER_RESULT * p_param);
/* 函数名称: etm_get_free_experience_member(异步接口)
   函数功能: 从服务器获取免费策略相关的信息。
   函数参数: 
     user_id  :   用户id;
     user_data :  用户携带的数据，回调函数里返回;
     callback_fun: 获取相关信息的回调函数;
     p_action_id:  异步操作的id;
*/
ETDLL_API int32 etm_get_free_experience_member(uint64 user_id, void* user_data, ETM_FREE_EXPERIENCE_MEMBER_CALLBACK callback_fun, u32 * p_action_id);

/* 函数名称: etm_get_free_experience_member(异步接口)
   函数功能: 从服务器获取免费策略相关的信息。
   函数参数: 
     user_id  :   用户id;
     user_data :  用户携带的数据，回调函数里返回;
     callback_fun: 获取相关信息的回调函数;
     p_action_id:  异步操作的id;
*/
ETDLL_API int32 etm_get_experience_member_remain_time(uint64 user_id, void* user_data, ETM_FREE_EXPERIENCE_MEMBER_CALLBACK callback_fun, u32 * p_action_id);

typedef struct t_get_high_speed_flux_result
{
	u32 _action_id;
	void * _user_data;						/* 用户参数 */
	int32 _result;							/*	0:成功；-1: 超时; 其他值:失败*/
} ETM_GET_HIGH_SPEED_FLUX_RESULT;
typedef int32 ( *ETM_GET_HIGH_SPEED_FLUX_CALLBACK)(ETM_GET_HIGH_SPEED_FLUX_RESULT * p_param);

typedef struct t_get_high_speed_flux
{
	uint64 _user_id;     /* 用户id */
	uint64 _flux;		  /* 需要领取的流量数 */			
	u32    _valid_time; /* 领取流量的有效时间 */
	u32    _property;   /* 0为普通流量卡，1为会员流量卡 */
	void* _user_data;   /* 用户数据 */
	ETM_GET_HIGH_SPEED_FLUX_CALLBACK _callback_fun;  /* 接口回调函数 */
} ETM_GET_HIGH_SPEED_FLUX;
/* 函数名称: etm_get_high_speed_flux(异步接口)
   函数功能: 从服务器(转发)领取免费流量卡。
*/
ETDLL_API int32 etm_get_high_speed_flux(ETM_GET_HIGH_SPEED_FLUX *flux_param, u32 * p_action_id);

//////////////////////////////////////////////////////////////
/////16 登录模块相关接口
#define MEMBER_SESSION_ID_SIZE 64
#define MEMBER_PROTOCAL_JUMPKEY_MAX_SIZE 512
#define MEMBER_MAX_USERNAME_LEN 64
#define MEMBER_MAX_PASSWORD_LEN 64
#define MEMBER_MAX_DATE_SIZE 32
#define MEMBER_MAX_PAYID_NAME_LEN   64
	
typedef enum tag_ETM_MEMBER_EVENT
{
	ETM_LOGIN_LOGINED_EVENT = 0,		/* Logined事件回调时, errcode为0表示login成功,为(-200)表示登录失败但是可以用cache信息,为(-201)为登录失败,但是可以用cache,并且cache里的sessionid已经更新.其他值为彻底失败! */
	ETM_LOGIN_FAILED_EVENT,
//	LOGOUT_SUCCESS_EVENT,
//	LOGOUT_FAILED_EVENT,
//	PING_FAILED_EVENT,
	ETM_UPDATE_PICTURE_EVENT,
	ETM_KICK_OUT_EVENT,			//账户在别处登录，被踢了
	ETM_NEED_RELOGIN_EVENT		//账户需要重新登录
//	MEMBER_LOGOUT_EVENT,
//	REFRESH_MEMBER_EVENT,
//	MEMBER_CANCEL_EVENT
}ETM_MEMBER_EVENT;

/* 登录信息 */
typedef struct tagETM_MEMBER_INFO
{
	Bool _is_vip;
	Bool _is_year;		//是否年费会员
	Bool _is_platinum;
	uint64 _userid;		//全局唯一id
	Bool _is_new;		//username 是否新帐号，新帐号username为纯数字
	char _username[MEMBER_MAX_USERNAME_LEN];		//账户名
	char _nickname[MEMBER_MAX_USERNAME_LEN];
	char _military_title[16];
	u16 _level;
	u16 _vip_rank;
	char _expire_date[MEMBER_MAX_DATE_SIZE];	// VIP过期日期
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
	Bool _is_son_account;    //是否子账号
	u32 _vas_type;	          //会员类型: 2-普通会员 3-白金会员 4--钻石会员
}ETM_MEMBER_INFO; 

/* 登录状态 */
typedef enum tagETM_MEMBER_STATE
{
	ETM_MEMBER_INIT_STATE = 0,
	ETM_MEMBER_LOGINING_STATE,		//正在登陆的状态
	ETM_MEMBER_LOGINED_STATE,		//登陆成功的状态
	ETM_MEMBER_LOGOUT_STATE,		//正在退出登陆的状态
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

//设置登录回调事件函数
ETDLL_API int32 etm_member_set_callback_func(ETM_MEMBER_EVENT_NOTIFY notify_func);

// 用户登录，参数需用utf8格式，密码不加密。
ETDLL_API int32 etm_member_login(const char* username, const char* password);
// sessionid和userid登录,(用于联合登录)。
ETDLL_API int32 etm_member_sessionid_login(const char* sessionid, uint64 userid);

// 加密方式登录，用于使用本地保存的MD5加密后的密码，参数需用utf8格式
ETDLL_API int32 etm_member_encode_login(const char* username, const char* md5_pw);

// 重新登录: 清除缓存并用缓存的用户名密码重新登录
ETDLL_API int32 etm_member_relogin(void);

// 刷新用户信息，不重新登录的情况下重新获取用户的信息。回调成功后etm_member_get_info重新获取信息
ETDLL_API int32 etm_member_refresh_user_info(ETM_MEMBER_REFRESH_NOTIFY notify_func);

// 用户注销。
ETDLL_API int32 etm_member_logout(void);

// 用户发送ping包，确保用户sessionid在有效期内。在需要sessionid的操作之前都需要发送一次该请求(eg: 云播操作)
ETDLL_API int32 etm_member_keepalive(void);
// 上报下载文件。
//ETDLL_API int32 etm_member_report_download_file(uint64 filesize, const char cid[40], char* url, u32 url_len);

//如果没有登录成功调用该函数会返回失败
ETDLL_API int32 etm_member_get_info(ETM_MEMBER_INFO* info);

//获取用户头像图片全路径文件名，若无图片返回NULL
ETDLL_API char * etm_member_get_picture(void);

//获取登录状态
ETDLL_API ETM_MEMBER_STATE etm_member_get_state(void);

//获取错误码解释。
//const char* etm_get_error_code_description(int32 errcode);

//用户注册，查看id是否存在
ETDLL_API int32 etm_member_register_check_id(ETM_REGISTER_INFO* register_info);

//用户注册，参数需用utf8格式。
ETDLL_API int32 etm_member_register_user(ETM_REGISTER_INFO* register_info);

ETDLL_API int32 etm_member_register_check_id_cancel(void);

ETDLL_API int32 etm_member_register_cancel(void);


#ifdef __cplusplus
}
#endif
#endif /* ET_TASK_MANAGER_H_200909081912 */

