
#ifndef SD_DEFINE_CONST_NUM_H_20090107
#define SD_DEFINE_CONST_NUM_H_20090107

#ifdef __cplusplus
extern "C" 
{
#endif

/***************************************************************************/
/*  asynframe*/
/***************************************************************************/

#define MIN_MSG_COUNT				(512)
#define MIN_THREAD_MSG_COUNT		(16)

#define MIN_TIMER_COUNT				(256)

#define MIN_BT_FILE_SIZE (5*1024)

#define MIN_DNS_COUNT				(256)
#define MIN_ACCEPT_COUNT			(2)
#define MIN_CONN_SOCKET_RW_COUNT	(64)
#define MIN_NOCONN_SOCKET_RW_COUNT	(64)
#define MIN_FS_OPEN_COUNT			(16)
#define MIN_FS_RW_COUNT				(16)
#define MIN_SOCKADDR_COUNT			(64)

#define MIN_SOCKET_COUNT	(64)

#define DNS_SERVER_IP_COUNT_MAX (3)
#define DNS_CONTENT_IP_COUNT_MAX (12)

#define MAX_FILE_SIZE_USING_ORIGIN_MODE (10*1024*1024)   /* 10MB */
#define MAX_FILE_SIZE_USING_LIMITED_PIPES (MAX_FILE_SIZE_USING_ORIGIN_MODE)   /* 10MB */
#define LITTLE_FILE_MAX_PIPES_EACH_SERVER (1)  

#ifdef MOBILE_PHONE
#define MAX_DNS_VALID_TIME (30*1000)       // 30s
#else
#define MAX_DNS_VALID_TIME (3600*1000)       // 1小时
#endif
/***************************************************************************/
/*  common*/
/***************************************************************************/
#if defined(_EXTENT_MEM_FROM_OS) && defined(MEMPOOL_1M) 	
#define MIN_LIST_MEMORY	(2048)
#define MIN_MAP_MEMORY	(1024)
#define MIN_SET_MEMORY	(1024 + MIN_MAP_MEMORY)
#define MIN_CROSSLINK_MEMORY (128)

#define MIN_QUEUE_MEMORY	(256)

#else
#define MIN_LIST_MEMORY	(4096)
#define MIN_MAP_MEMORY	(2048)
#define MIN_SET_MEMORY	(2048 + MIN_MAP_MEMORY)
#define MIN_CROSSLINK_MEMORY (512)

#define MIN_QUEUE_MEMORY	(256)

#endif

/************** _COMPANY_NUM   ************************/
#if defined(MOBILE_PHONE)
#ifndef  _COMPANY_NUM
#ifdef MACOS
#define PEERID_SUFFIX           ("003V")
#elif defined(NEIGHBOR_PROJ)
#define PEERID_SUFFIX           ("006V")
#elif defined(_ANDROID_LINUX)
#define PEERID_SUFFIX           ("004V")
#else
#define PEERID_SUFFIX           ("000V")
#endif
#else
#define PEERID_SUFFIX__(A) ""#A"V"
#define PEERID_SUFFIX_(A) PEERID_SUFFIX__(A)
#define PEERID_SUFFIX PEERID_SUFFIX_(_COMPANY_NUM)
#endif
#elif  defined(_TEST_FOR_XUNLEI)
#ifndef  _COMPANY_NUM
#define PEERID_SUFFIX           ("000Z")
#else
#define PEERID_SUFFIX__(A) ""#A"Z"
#define PEERID_SUFFIX_(A) PEERID_SUFFIX__(A)
#define PEERID_SUFFIX PEERID_SUFFIX_(_COMPANY_NUM)
#endif
#else
#ifndef  _COMPANY_NUM
		#ifdef MACOS
			#define PEERID_SUFFIX           ("005V")			// MAC迅雷
		#else
#define PEERID_SUFFIX           ("000X")
		#endif
#else
#define PEERID_SUFFIX__(A) ""#A"X"
#define PEERID_SUFFIX_(A) PEERID_SUFFIX__(A)
#define PEERID_SUFFIX PEERID_SUFFIX_(_COMPANY_NUM)
#endif

#endif
/************** End of _COMPANY_NUM   ************************/

/************** download.cfg   ************************/
 /*conf-file mgr*/
#ifdef _DEBUG
#define ENABLE_CFG_FILE  
#endif /* _DEBUG */
/************** End of download.cfg   ************************/

#ifndef MACOS
#define MAX_CFG_NAME_LEN 256
#define MAX_CFG_VALUE_LEN 256
#define MAX_CFG_LINE_LEN (MAX_CFG_NAME_LEN+MAX_CFG_VALUE_LEN+4)
#else
#define MAX_CFG_LINE_LEN 512
#define MAX_CFG_NAME_LEN 256
#define MAX_CFG_VALUE_LEN 256
#endif

#if defined(LINUX)
#if defined(_ANDROID_LINUX)
#define CONFIG_FILE_PATH "/sdcard/"
#elif defined(_TEST_RC)
#define CONFIG_FILE_PATH "/tmp/THUNDERMETA/"
#elif defined(MACOS) && defined(MOBILE_PHONE)
#define CONFIG_FILE_PATH "./Library/.config/"
#else
#define CONFIG_FILE_PATH "./"
#endif
#elif defined(AMLOS)
#define CONFIG_FILE_PATH "/mnt/UDISK/usba0/cfg/"
#elif defined(MSTAR)
#define CONFIG_FILE_PATH "/usb/cfg/"
#elif defined(SUNPLUS)
#define CONFIG_FILE_PATH "/usba0/"
#elif defined(WINCE)
#define CONFIG_FILE_PATH "./"
#endif	

#define CONFIG_FILE CONFIG_FILE_PATH"download.cfg"

#define MIN_SETTINGS_ITEM_MEMORY 128
#define RANGE_DATA_UNIT_SIZE (16*1024)

#define MIN_VALID_RELATION_BLOCK_LENGTH (2*RANGE_DATA_UNIT_SIZE)
#if defined(_EXTENT_MEM_FROM_OS) && defined(MEMPOOL_1M) 

#define RANGE_MIN_RANGE_LIST_NODE  1024

#define RANGE_MIN_RANGE_LIST  128

#else

#define RANGE_MIN_RANGE_LIST_NODE  4096

#define RANGE_MIN_RANGE_LIST  256

#endif

#ifdef _VOD_NO_DISK_EXTEND   
 #define VOD_EXTEND_BUFFER   2048  //  vod_extent buffer  4k  for one
#else
 #define VOD_EXTEND_BUFFER   0  //  vod_extent buffer  4k  for one
 #endif
 
#if  defined(_VOD_MEM_6M)
 #define VOD_MEM_BUFFER   1536   // 6M
#elif defined(_VOD_MEM_5M)
 #define VOD_MEM_BUFFER   1280   // 5M
#elif defined(_VOD_MEM_8M)
#define VOD_MEM_BUFFER   2048    // 8M
#elif defined(_VOD_MEM_10M)
#define VOD_MEM_BUFFER   2560   // 10M
#elif defined(_VOD_MEM_11M)
 #define VOD_MEM_BUFFER   2816   // 11M
#elif defined(_VOD_MEM_20M)
 #define VOD_MEM_BUFFER   5120   // 11M
#else
#define VOD_MEM_BUFFER   2560   // 10M
#endif

#define IP_REVERSE(ip)  (_u32)((0xFF000000&(ip<<24))|(0x00FF0000&(ip<<8))|(0x0000FF00&(ip>>8))|(0x000000FF&(ip>>24)))

#define MAX_CMWAP_RANGE 		(18)
#define MAX_WAP_403_COUNT		(30)
#define MAX_IAP_NAME_LEN 		128

#define IMEI_LEN (15)

/***************************************************************************/
/*  platform*/
/***************************************************************************/


/***************************************************************************/
/*  bt download*/
/***************************************************************************/

#if defined(_EXTENT_MEM_FROM_OS) && defined(MEMPOOL_1M) 
#define MIN_BT_TASK               1
#define MIN_BT_FILE_INFO     1
#define MIN_BT_FILE_TASK_INFO     1
#define MIN_BT_QUERY_PARA         2

#define MIN_TORRENT_FILE_INFO  1
//#define MIN_TORRENT_SEED_INFO  1
#define MIN_TORRENT_PARSER     2

#define BT_RESOURCE_SLAB_SIZE		4
#define	BT_DATA_PIPE_SLAB_SIZE		4
#define	BT_DEVICE_SLAB_SIZE			4
#define BT_PIECE_SLAB_SIZE			4
#define	RANGE_CHANGE_NODE_SLAB_SIZE  4

#define MIN_PIECE_RANGE_INFO  3
#define MIN_READ_RANGE_INFO  3
#define MIN_SUB_FILE_PADDING_RANGE_INFO  3


#define DEFAULT_BT_ACCELERATE_MIN_SIZE  (0)   /* 1 KB */

#define MIN_BT_DATA_READER  1

#define MIN_BC_READER_BUFFER 15
#define BT_METADATA_PIECE_SIZE (16*1024)


#else
#define MIN_BT_TASK               3
#define MIN_BT_FILE_INFO     5
#define MIN_BT_FILE_TASK_INFO     5
#define MIN_BT_QUERY_PARA         2

#define MIN_TORRENT_FILE_INFO  5
//#define MIN_TORRENT_SEED_INFO  1
#define MIN_TORRENT_PARSER     2

#define BT_RESOURCE_SLAB_SIZE		256
#define	BT_DATA_PIPE_SLAB_SIZE		64
#define	BT_DEVICE_SLAB_SIZE			64
#define BT_PIECE_SLAB_SIZE			128
#define	RANGE_CHANGE_NODE_SLAB_SIZE 512

#define MIN_PIECE_RANGE_INFO  3
#define MIN_READ_RANGE_INFO  3
#define MIN_SUB_FILE_PADDING_RANGE_INFO  3

#define DEFAULT_BT_ACCELERATE_MIN_SIZE  (0)   /* 1 KB */

#define MIN_BT_DATA_READER  5

#define MIN_BC_READER_BUFFER 15
#define BT_METADATA_PIECE_SIZE (16*1024)

#endif



#ifdef  MEMPOOL_1M	 

#ifdef _EXTENT_MEM_FROM_OS	
#define MAX_CUR_DOWNLOADING_SIZE (300 * 1024 * 1024)
#else
#define MAX_CUR_DOWNLOADING_SIZE (100 * 1024 * 1024)
#endif

#endif

#ifdef  MEMPOOL_3M	 
#define MAX_CUR_DOWNLOADING_SIZE (300 * 1024 * 1024)
#endif

#ifdef  MEMPOOL_5M	 
#define MAX_CUR_DOWNLOADING_SIZE (500 * 1024 * 1024)
#endif

#ifdef  MEMPOOL_8M	 
#define MAX_CUR_DOWNLOADING_SIZE (800 * 1024 * 1024) 
#endif

#ifdef  MEMPOOL_10M	 
#define MAX_CUR_DOWNLOADING_SIZE (1024 * 1024 * 1024) 
#endif
    
/***************************************************************************/
/*  get user info*/
/***************************************************************************/
    
#define MAX_DATE_LEN 64
    
/***************************************************************************/
/*  connect manager*/
/***************************************************************************/

#ifdef  MEMPOOL_1M	 
#ifdef _EXTENT_MEM_FROM_OS	
 #define CONNECT_MANAGE_DEFAULT_MAX_CONNECTIONS 20
#else
 #define CONNECT_MANAGE_DEFAULT_MAX_CONNECTIONS 20
#endif
#endif

#ifdef  MEMPOOL_3M	 
#define CONNECT_MANAGE_DEFAULT_MAX_CONNECTIONS 32 
#endif

#ifdef  MEMPOOL_5M	 
#define CONNECT_MANAGE_DEFAULT_MAX_CONNECTIONS 64 
#endif

#ifdef  MEMPOOL_8M	 
#define CONNECT_MANAGE_DEFAULT_MAX_CONNECTIONS 100 
#endif

#ifdef  MEMPOOL_10M	 
#define CONNECT_MANAGE_DEFAULT_MAX_CONNECTIONS 128
#endif

 #define CONNECT_MANAGE_DEFAULT_MAX_CONNECTIONS_IN_GPRS 10

#ifdef ENABLE_CDN
#define DEFAULT_CDN_RES_NUM 6
#endif /* ENABLE_CDN */

#define INVALID_FILE_INDEX (0xFFFFFFFF)

#define MAX_RES_LEVEL (0xFF)
#define MAX_PRIORITY (0xFF)

#define DEFAULE_CDN_MODE (TRUE)
#define DEFAULE_DISABLE_CDN_SPEED (20) 	//任务速度减去CDN速度大于该值时关闭CDN,单位KB
#define DEFAULE_ENABLE_CDN_SPEED (10) 	//任务下载速度小于该值时启动CDN,单位KB

#define MAX_HTTP_ENCAP_P2P_TEST_COUNT (10)

/***************************************************************************/
/*  data_manager*/

#define   DATA_UNINITED    99
#define   DATA_SUCCESS    100
#define   DATA_DOWNING    101
#define   DATA_CANNOT_CORRECT_ERROR    102
#define   DATA_CAN_NOT_GET_CID    103
#define   DATA_CAN_NOT_GET_GCID    104
#define   DATA_CHECK_CID_ERROR    105
#define   DATA_CHECK_GCID_ERROR    106
#define   DATA_CREAT_FILE_ERROR    107
#define   DATA_WRITE_FILE_ERROR    108
#define   DATA_READ_FILE_ERROR      109
#define   DATA_ALLOC_BCID_BUFFER_ERROR      110
#define   DATA_ALLOC_READ_BUFFER_ERROR      111

#define   DATA_NO_SPACE_TO_CREAT_FILE      112
#define   DATA_CANOT_CHECK_CID_READ_ERROR    113
#define   DATA_CANOT_CHECK_CID_NOBUFFER    114

#define   DATA_FILE_BIGGER_THAN_4G   115

#define  VOD_DATA_FINISHED    150

#define CID_SIZE 20

#define TCID_SAMPLE_SIZE     61440
#define TCID_SAMPLE_UNITSIZE  20480
#define BCID_DEF_MIN_BLOCK_COUNT 512
#define BCID_DEF_MAX_BLOCK_SIZE  2097152
#define BCID_DEF_BLOCK_SIZE	   262144

#define MIN_TMP_FILE             		8
#define MIN_MSG_FILE_CREATE_PARA 		8
#define MIN_MSG_FILE_RW_PARA     		16
#define MIN_MSG_FILE_CLOSE_PARA  		8
#define MIN_RW_DATA_BUFFER    		    32
#define MIN_RANGE_DATA_BUFFER_LIST      32
#define MIN_BLOCK_DATA_BUFFER    		20

#define CID_IS_OK 1
#define SHUB_NO_RESULT 2

#define USE_ONE_PIPE_FILE_UPPER_LIMIT 5*1024*1024

#define VOD_DATA_BUFFER_MAX_CACHE_BUFFER  (2*1024*1024)
#define VOD_DATA_BUFFER_MAX_ALLOC_BUFFER  ( VOD_MEM_BUFFER*4*1024)

 // 停止任务的时候如果文件大小小于这个值，则把已在内存里的数据写到磁盘中,否则直接丢弃以加快停止任务的速度
 #ifdef ASSISTANT_PROJ
#define MAX_FILE_SIZE_NEED_FLUSH_DATA_B4_CLOSE	(16*1024)
#else
#define MAX_FILE_SIZE_NEED_FLUSH_DATA_B4_CLOSE	(10*1024*1024)
#endif

#ifdef  MEMPOOL_1M
/*
#ifdef _VOD_NO_DISK
#define DATA_BUFFER_MAX_CACHE_BUFFER  (768*1024 + 4*1024*1024)
 #define DATA_BUFFER_MAX_ALLOC_BUFFER  (1*1024*1024 + 4*1024*1024  + VOD_EXTEND_BUFFER*4*1024)
#else
#define DATA_BUFFER_MAX_CACHE_BUFFER  (768*1024)
#define DATA_BUFFER_MAX_ALLOC_BUFFER  (1*1024*1024)		
#endif
*/
#ifdef _EXTENT_MEM_FROM_OS	
#define DATA_BUFFER_MAX_CACHE_BUFFER  (512*1024)
#define DATA_BUFFER_MAX_ALLOC_BUFFER  (2*1024*1024)	

#define  DATA_RECEIVE_MAX_FLUSH_UNIT_NUM  (2*1024*1024/RANGE_DATA_UNIT_SIZE)
#define  DATA_CHECK_MAX_READ_LENGTH   (16384)
#else
#define DATA_BUFFER_MAX_CACHE_BUFFER  (512*1024)
#define DATA_BUFFER_MAX_ALLOC_BUFFER  (768*1024*1024)	

#define  DATA_RECEIVE_MAX_FLUSH_UNIT_NUM  (0.5*1024*1024/RANGE_DATA_UNIT_SIZE)
#define  DATA_CHECK_MAX_READ_LENGTH   (16384)
#endif

#endif

#ifdef  MEMPOOL_3M
/*
#ifdef _VOD_NO_DISK
#define DATA_BUFFER_MAX_CACHE_BUFFER  (768*1024 + 4*1024*1024)
 #define DATA_BUFFER_MAX_ALLOC_BUFFER  (1*1024*1024 + 4*1024*1024  + VOD_EXTEND_BUFFER*4*1024)
#else
#define DATA_BUFFER_MAX_CACHE_BUFFER  (768*1024)
#define DATA_BUFFER_MAX_ALLOC_BUFFER  (1*1024*1024)		
#endif
*/
#define DATA_BUFFER_MAX_CACHE_BUFFER  (768*1024)
#define DATA_BUFFER_MAX_ALLOC_BUFFER  (1*1024*1024)	

#define  DATA_RECEIVE_MAX_FLUSH_UNIT_NUM  (0.75*1024*1024/RANGE_DATA_UNIT_SIZE)
#define  DATA_CHECK_MAX_READ_LENGTH   (16384)
#endif

#ifdef  MEMPOOL_5M
/*
#ifdef _VOD_NO_DISK
#define DATA_BUFFER_MAX_CACHE_BUFFER  (1*1024*1024 + 4*1024*1024)
 #define DATA_BUFFER_MAX_ALLOC_BUFFER  (2*1024*1024 + 4*1024*1024 + VOD_EXTEND_BUFFER*4*1024)
#else
#define DATA_BUFFER_MAX_CACHE_BUFFER  (1*1024*1024)
#define DATA_BUFFER_MAX_ALLOC_BUFFER  (2*1024*1024)		
#endif
*/
#define DATA_BUFFER_MAX_CACHE_BUFFER  (1*1024*1024)
#define DATA_BUFFER_MAX_ALLOC_BUFFER  (2*1024*1024)	
#define  DATA_RECEIVE_MAX_FLUSH_UNIT_NUM  (1*1024*1024/RANGE_DATA_UNIT_SIZE)

#define  DATA_CHECK_MAX_READ_LENGTH   (16384)
#endif

#ifdef  MEMPOOL_8M	 
/*
#ifdef _VOD_NO_DISK
#define DATA_BUFFER_MAX_CACHE_BUFFER  (1.5*1024*1024 + 4*1024*1024)
 #define DATA_BUFFER_MAX_ALLOC_BUFFER  (3*1024*1024 + 4*1024*1024 + VOD_EXTEND_BUFFER*4*1024)
#else
#define DATA_BUFFER_MAX_CACHE_BUFFER  (1.5*1024*1024)
#define DATA_BUFFER_MAX_ALLOC_BUFFER  ( 3*1024*1024)
#endif
*/
#define DATA_BUFFER_MAX_CACHE_BUFFER  (1.5*1024*1024)
#define DATA_BUFFER_MAX_ALLOC_BUFFER  ( 3*1024*1024)
#define  DATA_RECEIVE_MAX_FLUSH_UNIT_NUM  (1.5*1024*1024/RANGE_DATA_UNIT_SIZE)
 #define  DATA_CHECK_MAX_READ_LENGTH   (16384)
#endif

#ifdef  MEMPOOL_10M	 
/*
#ifdef _VOD_NO_DISK
#define DATA_BUFFER_MAX_CACHE_BUFFER  (2*1024*1024 + 4*1024*1024)
 #define DATA_BUFFER_MAX_ALLOC_BUFFER  ( 4*1024*1024 + 4*1024*1024 + VOD_EXTEND_BUFFER*4*1024)
#else
#define DATA_BUFFER_MAX_CACHE_BUFFER  (2*1024*1024)
#define DATA_BUFFER_MAX_ALLOC_BUFFER  ( 4*1024*1024)
#endif
*/
#define DATA_BUFFER_MAX_CACHE_BUFFER  (2*1024*1024)
#define DATA_BUFFER_MAX_ALLOC_BUFFER  ( 4*1024*1024)
#define  DATA_RECEIVE_MAX_FLUSH_UNIT_NUM  (2*1024*1024/RANGE_DATA_UNIT_SIZE)	 
#define  DATA_CHECK_MAX_READ_LENGTH   (16384)	 
#endif


#define   CORRECT_MANAGER_MIN_ERROR_BLOCK_NODE  (5)

#define DATA_RECEIVE_MIN_RANGE_DATA_BUFFER_NODE  (128)


/***************************************************************************/
/*  dispatcher*/
/***************************************************************************/

#define DISPATCH_MIN_DISPATCH_ITEM_NODE  (128)

/***************************************************************************/
/*  download task*/
/***************************************************************************/
#define MAX_PEER 20
#define MAX_SERVER 20
#define REQ_RESOURCE_DEFAULT_TIMEOUT (2*60*1000)  /* 2 minutes */

#define MAX_FILE_SIZE -1
#define MIN_FILE_SIZE 0
#define DEFAULT_BONUS_RES_NUM 20
#define MAX_QUERY_SHUB_RETRY_TIMES 2

#define DEFAULT_LOCAL_URL "http://127.0.0.1"
#define DEFAULT_LOCAL_IP "127.0.0.1"

#define MIN_TASK_MEMORY 5
#ifdef ENABLE_CDN
#define CDN_VERSION 1
#endif /*  */
/***************************************************************************/
/*  ftp data pipe*/
/***************************************************************************/
//#define FTP_REC_TIMEOUT_M_SEC 5000
//#define FTP_SND_TIMEOUT_M_SEC 5000

#define FTP_RESPN_DEFAULT_LEN 4096 //1024
#define FTP_DEFAULT_PORT 21
#define MAX_FTP_REQ_HEADER_LEN 64
#define FTP_DEFAULT_RANGE_LEN ((_u64)get_data_unit_size()) //(64*1024)=65535

#ifdef  MEMPOOL_1M	
#define FTP_DEFAULT_RANGE_NUM 1
#endif

#ifdef  MEMPOOL_3M	
#define FTP_DEFAULT_RANGE_NUM 1
#endif

#ifdef  MEMPOOL_5M	
#define FTP_DEFAULT_RANGE_NUM 1
#endif

#ifdef MEMPOOL_8M
#define FTP_DEFAULT_RANGE_NUM 1
#endif

#ifdef MEMPOOL_10M
#define FTP_DEFAULT_RANGE_NUM 1
#endif


//#define FTP_DEFAULT_RECEIVE_BLOCK_LEN_KB 8   /* k bytes */

#define MAX_RETRY_CONNECT_TIMES 0
#define RETRY_CONNECT_WAIT_M_SEC 1000
#define PUT_DATA_WAIT_M_SEC 1000
#define SET_FILE_SIZE_WAIT_M_SEC 1000
#define MAX_NUM_RANGES -1
//#define MAX_RANGES_LEN -1

#define MIN_FTP_RES_MEMORY 32
#define MIN_FTP_PIPE_MEMORY 32

/***************************************************************************/
/*  http data pipe*/
/***************************************************************************/
#define MAX_NUM_FIELD 15
//#define HTTP_REC_TIMEOUT_M_SEC 5000
//#define HTTP_SND_TIMEOUT_M_SEC 5000
#define HTTP_1024_LEN 1024
#define HTTP_HEADER_DEFAULT_LEN HTTP_1024_LEN
#define HTTP_DEFAULT_PORT 80
#define HTTPS_DEFAULT_PORT 443
#define HTTP_SMALL_FILE_DEFAULT_LEN (1024*10)
#define MAX_HTTP_REQ_HEADER_LEN 512
#define MAX_FIELD_NAME_LEN 32
#define MAX_STATUS_LINE_LEN 32
#define HTTP_HTML_HEADER_UP_CASE "<!DOCTYPE HTML"
#define HTTP_HTML_HEADER_LOW_CASE "<!DOCTYPE html"

#define HTTP_DEFAULT_RANGE_LEN ((_u64)get_data_unit_size()) //16*1024=16384

#define MIN_MINI_HTTP_MEMORY 1
#define DEFAULT_MINI_HTTP_TASK_TIMEOUT 180
#define MINI_HTTP_TIMEOUT_INSTANCE		100
#define MINI_TIMEOUT_INSTANCE		MINI_HTTP_TIMEOUT_INSTANCE

#define MINI_HTTP_CLOSE_NORMAL	100//http请求完毕，正常关闭
#define MINI_HTTP_CLOSE_TIMEOUT	101//http请求超时
#define MINI_HTTP_CLOSE_FORCE		102//强制关闭
#define MINI_HTTP_CLOSE_ERROR		103//错误

#ifdef  MEMPOOL_1M	
#define HTTP_DEFAULT_RANGE_NUM 1
#ifdef _EXTENT_MEM_FROM_OS	
#define MIN_HTTP_RES_MEMORY 10
#else
#define MIN_HTTP_RES_MEMORY 20
#endif
#endif

#ifdef  MEMPOOL_3M	
#define HTTP_DEFAULT_RANGE_NUM 1
#define MIN_HTTP_RES_MEMORY 32
#endif

#ifdef  MEMPOOL_5M	
#define HTTP_DEFAULT_RANGE_NUM 1
#define MIN_HTTP_RES_MEMORY 64
#endif

#ifdef MEMPOOL_8M
#define HTTP_DEFAULT_RANGE_NUM 1
#define MIN_HTTP_RES_MEMORY  96
#endif

#ifdef MEMPOOL_10M
#define HTTP_DEFAULT_RANGE_NUM 1
#define MIN_HTTP_RES_MEMORY 96
#endif


//#define HTTP_DEFAULT_RECEIVE_BLOCK_LEN_KB 8   /* k bytes */

//#define HTTP_DEFAULT_FILE_NAME "index.html"

//#define DEBUG_LEN 256
//#define MAX_RETRY_TIMES 0
//#define RETRY_CONNECT_WAIT_M_SEC 5000
//#define PUT_DATA_WAIT_M_SEC 1000
//#define SET_FILE_SIZE_WAIT_M_SEC 1000
//#define MAX_NUM_RANGES -1
//#define MAX_RANGES_LEN -1

#if defined(_EXTENT_MEM_FROM_OS) && defined(MEMPOOL_1M) 
#define MIN_HTTP_PIPE_MEMORY 16
#define MIN_HTTP_1024_MEMORY 16
#else
#define MIN_HTTP_PIPE_MEMORY 32
#define MIN_HTTP_1024_MEMORY 32
#endif


/***************************************************************************/
/*  p2p data pipe*/
/***************************************************************************/

#if defined(_EXTENT_MEM_FROM_OS) && defined(MEMPOOL_1M) 
#define		P2P_DATA_PIPE_SLAB_SIZE			16
#define		SOCKET_DEVICE_SLAB_SIZE			16
#define		RECV_CMD_BUFFER_LEN_SLAB_SIZE	16
#define		RANGE_SLAB_SIZE					128
#define		P2P_SENDING_QUEUE_SLAB_SIZE		32
#define		P2P_SENDING_CMD_SLAB_SIZE		16
#define		P2P_UPLOAD_DATA_SLAB_SIZE		4

#else
#define		P2P_DATA_PIPE_SLAB_SIZE			64
#define		SOCKET_DEVICE_SLAB_SIZE			64
#define		RECV_CMD_BUFFER_LEN_SLAB_SIZE	64
#define		RANGE_SLAB_SIZE					384
#define		P2P_SENDING_QUEUE_SLAB_SIZE		64
#define		P2P_SENDING_CMD_SLAB_SIZE		32
#define		P2P_UPLOAD_DATA_SLAB_SIZE		64

#endif

#define		P2P_FROM_TRACKER	            0
#define		P2P_FROM_HUB					1
#define		P2P_FROM_CDN					2
#define		P2P_FROM_PARTNER_CDN			3
#define		P2P_FROM_PASSIVE				4
#define		P2P_FROM_UNKNOWN				5
#define		P2P_FROM_VIP_HUB				6
#define		P2P_FROM_LIXIAN				    7
#define		P2P_FROM_ASSIST_DOWNLOAD		8
#define     P2P_FROM_DPHUB                  9
#define     P2P_FROM_EMULE_TRACKER          10
#define		P2P_FROM_NORMAL_CDN				11


#ifdef  MEMPOOL_1M	
#ifdef _EXTENT_MEM_FROM_OS	
#define		P2P_RESOURCE_SLAB_SIZE			32	
#else
#define		P2P_RESOURCE_SLAB_SIZE			64	
#endif
#endif

#ifdef  MEMPOOL_3M	
#define		P2P_RESOURCE_SLAB_SIZE			128	
#endif

#ifdef  MEMPOOL_5M	
#define		P2P_RESOURCE_SLAB_SIZE			256	
#endif

#ifdef MEMPOOL_8M
#define		P2P_RESOURCE_SLAB_SIZE			384	
#endif

#ifdef MEMPOOL_10M
#define		P2P_RESOURCE_SLAB_SIZE			512	
#endif


/***************************************************************************/
/*  p2p transfer layer*/
/***************************************************************************/

#if defined(_EXTENT_MEM_FROM_OS) && defined(MEMPOOL_1M) 
#define		UDT_SYN_DATA_SLAB_SIZE			32
#define		UDT_SEND_BUFFER_SLAB_SIZE		32
#define		UDT_RECV_BUFFER_SLAB_SIZE		32
#define		ICALLSOMEONE_DATA_SLAB_SIZE		32
#else
#define		UDT_SYN_DATA_SLAB_SIZE			128
#define		UDT_SEND_BUFFER_SLAB_SIZE		128
#define		UDT_RECV_BUFFER_SLAB_SIZE		128
#define		ICALLSOMEONE_DATA_SLAB_SIZE		128
#endif

#ifdef  MEMPOOL_1M	

#define		UDP_BUFFER_SIZE					1600

#ifdef _EXTENT_MEM_FROM_OS	
#define		TCP_DEVICE_SLAB_SIZE			16
#define		UDT_DEVICE_SLAB_SIZE			12
#define		GET_PEERSN_DATA_SLAB_SIZE		12
#define		GET_PEERSN_FUNC_SLAB_SIZE		12
#define		PEERSN_CACHE_DATA_SLAB_SIZE		12
#define		UDP_BUFFER_SLAB_SIZE			32
#else
#define		TCP_DEVICE_SLAB_SIZE			32
#define		UDT_DEVICE_SLAB_SIZE			24
#define		GET_PEERSN_DATA_SLAB_SIZE		24
#define		GET_PEERSN_FUNC_SLAB_SIZE		24
#define		PEERSN_CACHE_DATA_SLAB_SIZE		24
#define		UDP_BUFFER_SLAB_SIZE			64
#endif
#endif

#ifdef  MEMPOOL_3M	
#define		TCP_DEVICE_SLAB_SIZE			48
#define		UDT_DEVICE_SLAB_SIZE			48
#define		GET_PEERSN_DATA_SLAB_SIZE		48
#define		GET_PEERSN_FUNC_SLAB_SIZE		48
#define		PEERSN_CACHE_DATA_SLAB_SIZE		48
#define		UDP_BUFFER_SIZE					1600
#define		UDP_BUFFER_SLAB_SIZE			128
#endif

#ifdef  MEMPOOL_5M	
#define		TCP_DEVICE_SLAB_SIZE			48
#define		UDT_DEVICE_SLAB_SIZE			48
#define		GET_PEERSN_DATA_SLAB_SIZE		48
#define		GET_PEERSN_FUNC_SLAB_SIZE		48
#define		PEERSN_CACHE_DATA_SLAB_SIZE		48
#define		UDP_BUFFER_SIZE					1600
#define		UDP_BUFFER_SLAB_SIZE			192
#endif

#ifdef MEMPOOL_8M
#define		TCP_DEVICE_SLAB_SIZE			64
#define		UDT_DEVICE_SLAB_SIZE			64
#define		GET_PEERSN_DATA_SLAB_SIZE		64
#define		GET_PEERSN_FUNC_SLAB_SIZE		64
#define		PEERSN_CACHE_DATA_SLAB_SIZE		64
#define		UDP_BUFFER_SIZE					2048
#define		UDP_BUFFER_SLAB_SIZE			214
#endif

#ifdef MEMPOOL_10M
#define		TCP_DEVICE_SLAB_SIZE			64
#define		UDT_DEVICE_SLAB_SIZE			64
#define		GET_PEERSN_DATA_SLAB_SIZE		64
#define		GET_PEERSN_FUNC_SLAB_SIZE		64
#define		PEERSN_CACHE_DATA_SLAB_SIZE		64
#define		UDP_BUFFER_SIZE					2048
#define		UDP_BUFFER_SLAB_SIZE			256

#endif


/***************************************************************************/
/*  drm */
/***************************************************************************/

#define DRM_VERIFY_SERVER_HOST DEFAULT_LOCAL_IP   //"auth.shop.xunlei.com"


/***************************************************************************/
/*  report*/
/***************************************************************************/
#define MAX_LICENSE_REPORT_RETRY_TIMES 0
#define FIRST_LICENSE_REPORT_INTERVAL (5*60)
#define DEFAULT_LICENSE_REPORT_INTERVAL (60*60)
#define DEFAULT_LICENSE_REPORT_FAILED_INTERVAL (10*60)
#define DEFAULT_LICENSE_EXPIRE_TIME (60*60)
#define DEFAULT_LICENSE 	"0810100001099a951a5fcd4ad593a129815438ef39"

#define MAX_SUFFIX_LEN 16
#define MAX_VERSION_LEN 64
#define MAX_PARTNER_ID_LEN 16
#define MAX_LICENSE_LEN 256
#define MAX_OS_LEN 64

#define MIN_REPORT_FILE_SIZE (10*1024)

#define	PARTNER_ID_LEN					8

#if defined(MOBILE_PHONE)

#ifndef  _COMPANY_NUM
#define PARTNER_ID                      "20010000"
#else
#define PARTNER_ID__(A)   "20010"#A
#define PARTNER_ID_(A) PARTNER_ID__(A)
#define PARTNER_ID PARTNER_ID_(_COMPANY_NUM)
#endif


#else

#ifndef  _COMPANY_NUM
#define PARTNER_ID                      "20000000"
#else
#define PARTNER_ID__(A)   "20000"#A
#define PARTNER_ID_(A) PARTNER_ID__(A)
#define PARTNER_ID PARTNER_ID_(_COMPANY_NUM)
#endif

#endif

/********* 随身盘相关域名*********/
#define DEFAULT_WALKBOX_HOST_NAME DEFAULT_LOCAL_IP   //"wireless.walkbox.vip.xunlei.com" //"wireless.svr.xlpan.com" //121.10.137.164
#define DEFAULT_WALKBOX_PORT 80   //(8889)  //
	
//#define DEFAULT_WALKBOX_HOST_NAME "121.10.137.164"
//#define DEFAULT_WALKBOX_PORT 8889  //
/****************************************/

/********* 离线空间相关域名*********/
#define DEFAULT_LIXIAN_HOST_NAME DEFAULT_LOCAL_IP   //"pad.i.vod.xunlei.com"// "10.10.199.26" //"127.0.0.1"  /* xml 协议服务器 */
#define DEFAULT_LIXIAN_PORT 80//21011 //80   //(8889)  //

#define DEFAULT_LIXIAN_SERVER_HOST_NAME DEFAULT_LOCAL_IP   //"service.lixian.vip.xunlei.com"   /* 二进制协议服务器 */
#define DEFAULT_LIXIAN_SERVER_PORT 80
	
/********* 网络邻居相关域名*********/
#define DEFAULT_LOCATION_HOST_NAME  DEFAULT_LOCAL_IP   //"nb.q.xunlei.com"  //"119.147.41.70"  //
#define DEFAULT_LOCATION_PORT (8686)  //

#define DEFAULT_NEIGHBOR_HOST_NAME   DEFAULT_LOCAL_IP   //"platformc.q.xunlei.com"  //"119.147.41.70" //
#define DEFAULT_NEIGHBOR_PORT (80)  //

/********* 无线数据中心域名*********/
#define DEFAULT_HTTP_REPORT_HOST_NAME DEFAULT_LOCAL_IP   //"pgv.m.xunlei.com"
#define DEFAULT_HTTP_REPORT_PORT (80)
	


#define DEFAULT_LICENSE_HOST_NAME  DEFAULT_LOCAL_IP   // "license.em.xunlei.com"    /* no use */
#define DEFAULT_LICENSE_PORT  80

#define DEFAULT_SHUB_HOST_NAME DEFAULT_LOCAL_IP   //"hub5sr.wap.sandai.net"
#define DEFAULT_SHUB_PORT 3076 //80  //

#define DEFAULT_PHUB_HOST_NAME   DEFAULT_LOCAL_IP   // "hub5pr.wap.sandai.net"  
#define DEFAULT_PHUB_PORT 3076 // 80  

#define DEFAULT_TRCKER_HOST_NAME  DEFAULT_LOCAL_IP   //"hub5p.wap.sandai.net" 
#define DEFAULT_TRACKER_PORT  3076 //80

#define DEFAULT_PARTERN_HUB_HOST_NAME   DEFAULT_LOCAL_IP   //"cprovider.wap.sandai.net"    /* no use */
#define DEFAULT_PARTERN_HUB_PORT  80

#define DEFAULT_KANKAN_CDN_MANAGER_HOST_NAME   DEFAULT_LOCAL_IP   //"cl.wap.sandai.net"  // new cdn manager cl.mp4.m.xunlei.com
#define DEFAULT_KANKAN_CDN_MANAGER_PORT  80

#define DEFAULT_NORMAL_CDN_MANAGER_HOST_NAME	DEFAULT_LOCAL_IP   //"cdnmgr.phub.sandai.net"  // normal cdn manage
#define DEFAULT_NORMAL_CDN_MANAGER_PORT			80

#define DEFAULT_KANKAN_HTTP_CDN_MANAGER_HOST_NAME DEFAULT_LOCAL_IP   //"web.cl.kankan.xunlei.com"
#define DEFAULT_KANKAN_HTTP_CDN_MANAGER_PORT  80

#define DEFAULT_BT_HUB_HOST_NAME DEFAULT_LOCAL_IP   //"hub5btmain.wap.sandai.net"
#define DEFAULT_BT_HUB_PORT 3076 //80

#define DEFAULT_EMULE_HUB_HOST_NAME DEFAULT_LOCAL_IP   //"hub5emu.wap.sandai.net" //"hub5emu.sandai.net"
#define DEFAULT_EMULE_HUB_PORT 3076  //80

#define DEFAULT_EMULE_TRACKER_HOST_NAME DEFAULT_LOCAL_IP   //"hub5emutr.wap.sandai.net"
#define DEFAULT_EMULE_TRACKER_PORT 8000
#define DEFAULT_STAT_HUB_HOST_NAME DEFAULT_LOCAL_IP   //"hubstat.wap.sandai.net"
#define DEFAULT_STAT_HUB_PORT 3076 //80 //

#define DEFAULT_EMB_HUB_HOST_NAME DEFAULT_LOCAL_IP   //"wapstat.wap.sandai.net"
#define DEFAULT_EMB_HUB_PORT  83

#define DEFAULT_NET_SERVER_HOST_NAME  DEFAULT_LOCAL_IP   //"hub5pn.wap.sandai.net"
#define DEFAULT_NET_SERVER_PORT 8000

#define DEFAULT_PING_SERVER_HOST_NAME  DEFAULT_LOCAL_IP   //"hub5u.wap.sandai.net"
#define DEFAULT_PING_SERVERB_PORT 8000

#define DEFAULT_MOVIE_SERVER_HOST_NAME DEFAULT_LOCAL_IP   //"movie.xunlei.com"
#define DEFAULT_MOVIE_SERVER_PORT 80

#define DEFAULT_HSC_PERM_QUERY_HOST_NAME DEFAULT_LOCAL_IP   //"boxdown.shop.xunlei.com" //高速通道权限查询服务器地址

#define DEFAULT_MAIN_MEMBER_SERVER_HOST_NAME DEFAULT_LOCAL_IP   //"phonelogin.reg2t.sandai.net"		//"login3.reg2t.sandai.net"
#define DEFAULT_PORTAL_MEMBER_SERVER_HOST_NAME DEFAULT_LOCAL_IP   //"phoneportal.i.xunlei.com"		//"portal.i.xunlei.com"
#define DEFAULT_MAIN_VIP_SERVER_HOST_NAME DEFAULT_LOCAL_IP   //"phonecache.vip.xunlei.com"			//"cache2.vip.xunlei.com"
#define DEFAULT_PORTAL_VIP_SERVER_HOST_NAME DEFAULT_LOCAL_IP   //"phoneportal.i.xunlei.com"			//"portal.i.xunlei.com"

#define DEFAULT_VIP_HUB_HOST_NAME DEFAULT_LOCAL_IP   //"viphub5pr.phub.sandai.net"
#define DEFAULT_VIP_HUB_PORT 80

#define DEFAULT_CONFIG_HUB_HOST_NAME DEFAULT_LOCAL_IP   //"wap.pmap.sandai.net"
#define DEFAULT_CONFIG_HUB_PORT 80

#define DEFAULT_REPORT_INTERVAL  600	// 每10分钟上报一次wapstat.wap.sandai.net:83
#define	REPORTER_TIMEOUT		5000
#define DEFAULT_CMD_RETRY_TIMES 2

// 默认的父节点域名和端口
#define DEFAULT_DPHUB_PARENT_NODE_HOST_NAME DEFAULT_LOCAL_IP   //"dfnode.wap.dphub.sandai.net"
#define DEFAULT_DPHUB_PARENT_NODE_PORT 80

#define DEFAULT_DPHUB_HOST_NAME DEFAULT_LOCAL_IP   //"master.wap.dphub.sandai.net"
//#define DEFAULT_DPHUB_HOST_NAME "10.10.25.89"
#define DEFAULT_DPHUB_PORT  80


/***************************************************************************/
/*  res_query*/
/***************************************************************************/
#define DEFAULT_RES_QUERY_SHUB_HOST  DEFAULT_SHUB_HOST_NAME
#define DEFAULT_RES_QUERY_SHUB_PORT DEFAULT_SHUB_PORT

#define DEFAULT_RES_QUERY_PHUB_HOST  DEFAULT_PHUB_HOST_NAME
#define DEFAULT_RES_QUERY_PHUB_PORT DEFAULT_PHUB_PORT

#define DEFAULT_RES_QUERY_VIP_HUB_HOST DEFAULT_VIP_HUB_HOST_NAME
#define DEFAULT_RES_QUERY_VIP_HUB_PORT DEFAULT_VIP_HUB_PORT

#define DEFAULT_RES_QUERY_PARTNER_CDN_HOST  DEFAULT_PARTERN_HUB_HOST_NAME
#define DEFAULT_RES_QUERY_PARTNER_CDN_PORT DEFAULT_PARTERN_HUB_PORT

#define DEFAULT_RES_QUERY_TRACKER_HOST  DEFAULT_TRCKER_HOST_NAME
#define DEFAULT_RES_QUERY_TRACKER_PORT DEFAULT_TRACKER_PORT

#define DEFAULT_RES_QUERY_BTHUB_HOST  DEFAULT_BT_HUB_HOST_NAME
#define DEFAULT_RES_QUERY_BTHUB_PORT DEFAULT_BT_HUB_PORT

#define DEFAULT_RES_QUERY_CDN_MANAGER_HOST  DEFAULT_KANKAN_CDN_MANAGER_HOST_NAME
#define DEFAULT_RES_QUERY_CDN_MANAGER_PORT DEFAULT_KANKAN_CDN_MANAGER_PORT

#define DEFAULT_RES_QUERY_NORMAL_CDN_MANAGER_HOST	DEFAULT_NORMAL_CDN_MANAGER_HOST_NAME
#define DEFAULT_RES_QUERY_NORMAL_CDN_MANAGER_PORT	DEFAULT_NORMAL_CDN_MANAGER_PORT

#define DEFAULT_RES_QUERY_KANKAN_CDN_MANAGER_HOST  DEFAULT_KANKAN_HTTP_CDN_MANAGER_HOST_NAME
#define DEFAULT_RES_QUERY_KANKAN_CDN_MANAGER_PORT DEFAULT_KANKAN_HTTP_CDN_MANAGER_PORT

#define DEFAULT_RES_QUERY_EMULEHUB_HOST  DEFAULT_EMULE_HUB_HOST_NAME
#define DEFAULT_RES_QUERY_EMULEHUB_PORT DEFAULT_EMULE_HUB_PORT
#define DEFAULT_RES_QUERY_MAGNET_URL  DEFAULT_LOCAL_IP   //"bt.box.n0808.com"

#define DEFAULT_RES_QUERY_CONFIG_HUB_HOST  DEFAULT_CONFIG_HUB_HOST_NAME
#define DEFAULT_RES_QUERY_CONFIG_HUB_PORT  DEFAULT_CONFIG_HUB_PORT
#define DEFAULT_RES_QUERY_DPHUB_ROOT_HOST   DEFAULT_DPHUB_HOST_NAME
#define DEFAULT_RES_QUERY_DPHUB_ROOT_PORT   DEFAULT_DPHUB_PORT

#define DEFAULT_RES_QUERY_DPHUB_PARENT_NODE_HOST    DEFAULT_DPHUB_PARENT_NODE_HOST_NAME
#define DEFAULT_RES_QUERY_DPHUB_PARENT_NODE_PORT    DEFAULT_DPHUB_PARENT_NODE_PORT

#define DEFAULT_RES_QUERY_EMULETRACKER_HOST DEFAULT_EMULE_TRACKER_HOST_NAME
#define DEFAULT_RES_QUERY_EMULETRACKER_PORT DEFAULT_EMULE_TRACKER_PORT

#define WAIT_FOR_DPHUB_ROOT_RETURN_TIMEOUT  2*1000      // 2s

/***************************************************************************/
/*  socket proxy*/
/***************************************************************************/
#if defined(_EXTENT_MEM_FROM_OS) && defined(MEMPOOL_1M) 
#define		PROXY_DATA_SLAB_SIZE			32
#define		PROXY_CONNECT_DNS_SLAB_SIZE		8
#define		PROXY_SENDTO_DNS_SLAB_SIZE		64
#define		FD_MSGID_PAIR_SLAB_SIZE			16
#define		SOCKET_RECV_REQUEST_SLAB_SIZE	16
#define		SOCKET_SEND_REQUEST_SLAB_SIZE	16
#else
#define		PROXY_DATA_SLAB_SIZE			128
#define		PROXY_CONNECT_DNS_SLAB_SIZE		32
#define		PROXY_SENDTO_DNS_SLAB_SIZE		256
#define		FD_MSGID_PAIR_SLAB_SIZE			32
#define		SOCKET_RECV_REQUEST_SLAB_SIZE	32
#define		SOCKET_SEND_REQUEST_SLAB_SIZE	32
#endif

/***************************************************************************/
/*  task manager*/
/***************************************************************************/

/* 长文件名支持:文件名和路径最长支持1024字节，文件全路径最长支持2048字节 */
#define MAX_LONG_FILE_PATH_LEN 1024
#define MAX_LONG_FILE_NAME_LEN (1024-MAX_TD_CFG_SUFFIX_LEN)
#define MAX_LONG_FILE_NAME_BUFFER_LEN (1024)
#define MAX_LONG_FULL_PATH_LEN (MAX_LONG_FILE_PATH_LEN+MAX_LONG_FILE_NAME_LEN)
#define MAX_LONG_FULL_PATH_BUFFER_LEN (MAX_LONG_FILE_PATH_LEN+MAX_LONG_FILE_NAME_BUFFER_LEN)


#define MAX_TD_CFG_SUFFIX_LEN (8) /*.td.cfg*/

#define MAX_URL_LEN 1024
#define MAX_FILE_PATH_LEN 512
#define MAX_FILE_NAME_LEN (512-MAX_TD_CFG_SUFFIX_LEN)
#define MAX_FILE_NAME_BUFFER_LEN (512)
#define MAX_COOKIE_LEN 256

#define MIN_URL_LEN 9
#define MAX_DES_LEN 512
#define MAX_FULL_PATH_LEN (MAX_FILE_PATH_LEN+MAX_FILE_NAME_LEN)
#define MAX_FULL_PATH_BUFFER_LEN (MAX_FILE_PATH_LEN+MAX_FILE_NAME_BUFFER_LEN)
#define MAX_ED2K_LEN	2048
#define MAX_BT_MAGNET_LEN 2048
#define CID_SIZE 20
#define PEER_ID_SIZE 16
#define IMEI_SIZE 15
#define MAX_USER_NAME_LEN 64
#define MAX_PASSWORD_LEN 64
#define MAX_HOST_NAME_LEN 128
#define MAX_USER_DATA_LEN (5120)  //5KB
#define MAX_P2SP_TASK_FULL_INFO_LEN (10000*64 + MAX_USER_DATA_LEN+2*MAX_URL_LEN+MAX_FILE_PATH_LEN+MAX_FILE_NAME_LEN+CID_SIZE+128)  

#define MAX_LAST_MODIFIED_LEN 64

#define UPDATE_INFO_WAIT_M_SEC 1000

#define HEX_INFO_HASH_LEN 40

#ifdef KANKAN_PROJ
#define MIN_FREE_DISK_SIZE (100*1024)  //单位邋KB,最小剩余空间100MB，IPAD下预留100MB
#else
#define MIN_FREE_DISK_SIZE (10*1024)  //单位邋KB,最小剩余空间10MB
#endif


#define  MAX_ALOW_TASKS 16
#if defined(ENABLE_WALKBOX ) && defined(MACOS)
#define  MAX_ET_ALOW_TASKS 64
#else
#define  MAX_ET_ALOW_TASKS MAX_ALOW_TASKS
#endif /* ENABLE_WALKBOX   MACOS */
#define MAX_NUM_TASKS 5
#define MAX_RUNNING_LAN_TASKS 1


#if defined(_ANDROID_LINUX)
//#define LOG_CONFIG_FILE "/storage/emulated/0/log.conf"
#define LOG_CONFIG_FILE "/storage/emulated/0/slog.config"
#else
#define LOG_CONFIG_FILE "log.conf"
#endif



/***************************************************************************/
/*  upload manager*/
/***************************************************************************/
#ifdef UPLOAD_ENABLE
#define ULM_SCHEDULER_TIME_OUT 2000
//#define ULM_MAX_PIPES_NUM 	5
#define ULM_MAX_UNCHOKE_PIPES_NUM 	5
#define ULM_MAX_CHOKE_PIPES_NUM 	50
#define ULM_MAX_FILES_NUM 	5
#define ULM_MAX_IDLE_INTERVAL 2000

#define NORMAL_HUB (0)	//普通任务下载的数据
#define ISSUE_HUB (1)	//软件升级包下载的数据

#define MIN_UPLOAD_FILE_SIZE (10*1024)

#define RC_LIST_FILE_PATH CONFIG_FILE_PATH

#define RC_LIST_FILE "cid_store.dat"

/* rc_list_manager */
#define MAX_RC_INFO_KEY_LEN CID_SIZE
#define STORE_VERSION_HI (1)
#define STORE_VERSION_LO (0)
#define STORE_VERSION    (STORE_VERSION_LO + (STORE_VERSION_HI << 8))
#define MAX_RC_LIST_NUMBER 1000

 #define MIN_RC_LIST_ITEM_MEMORY 20
 #define MIN_RC_INFO_KEY_MEMORY 20

 #define MIN_ULM_FILE_ITEM_MEMORY 5
 #define MIN_ULM_READ_ITEM_MEMORY 5
 #define MIN_ULM_MAP_KEY_ITEM_MEMORY 5

 #define MIN_UPM_PIPE_INFO_ITEM_MEMORY 5
#endif

/***************************************************************************/
/*  dk query*/
/***************************************************************************/


#define MIN_FIND_NODE_HANDLER 5
#define MIN_NODE_INFO 15
#define MIN_KAD_NODE_INFO 15
#define MIN_K_NODE 50
#define MIN_KAD_NODE 50
#define MIN_DK_REQUEST_NODE 10
#define MIN_K_BUCKET 128


#define DHT_TYPE 0
#define KAD_TYPE 1
#define DK_TYPE_COUNT 2

#define DHT_ID_LEN 20
#define DK_PACKET_MAX_LEN 1024

#define KAD_ID_LEN 16

#define	FILE_ID_SIZE				16
#define	AICH_HASH_SIZE			20

#if defined(MSTAR)
extern unsigned int g_log_asynframe;
extern unsigned int g_log_asynframe_step;
extern unsigned int g_log_socket;
extern unsigned int g_log_socket_step;
extern unsigned int g_log_dns;
extern unsigned int g_log_dns_step;

extern unsigned int g_log_fs1;
extern unsigned int g_log_fs2;


extern unsigned int g_log_enlarge;
extern unsigned int g_log_enlarge_step;


extern unsigned int g_log_sd;
extern unsigned int g_log_sd_step;

extern unsigned int g_log_fd;
extern unsigned int g_log_fd_step;




extern unsigned int g_log_read;
extern unsigned int g_log_read_step;
extern unsigned int g_log_pread;
extern unsigned int g_log_pread_step;

extern unsigned int g_log_operation_type;
extern unsigned int g_log_msg_errcode;
extern unsigned int g_log_msg_elapsed;
#endif


#ifdef VVVV_VOD_DEBUG
extern _u64 gloabal_recv_data ;

extern _u64 gloabal_invalid_data ;

extern _u64 gloabal_discard_data ;

extern _u32 gloabal_task_speed ;

extern _u32 gloabal_vv_speed ;
#endif

#define MAX_PIPE_NUM_WHEN_ASSISTANT_ON	5

#ifdef __cplusplus
}
#endif
#endif 


