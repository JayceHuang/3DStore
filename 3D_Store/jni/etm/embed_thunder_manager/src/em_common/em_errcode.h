#ifndef EM_ERRCODE_H_00138F8F2E70_200806111932
#define EM_ERRCODE_H_00138F8F2E70_200806111932

#include "utility/errcode.h"

_int32 em_set_critical_error(_int32 errcode);
_int32 em_get_critical_error(void);

#define EM_CHECK_CRITICAL_ERROR  {CHECK_VALUE(em_get_critical_error());}


/***************************************************************************/
/*ETM*/
/***************************************************************************/
#define	ETM_ERRCODE_BASE			  (1024 * 100)	//102400
#define	DT_ERRCODE_BASE			  ETM_ERRCODE_BASE	
typedef enum t_etm_err_code
{
ET_WRONG_VERSION=ETM_ERRCODE_BASE+1, //102401
ETM_OUT_OF_MEMORY,
ETM_BUSY,
INVALID_SYSTEM_PATH,
INVALID_LICENSE_LEN,		//102405
LICENSE_VERIFYING,
TOO_MANY_TASKS,				 
TOO_MANY_RUNNING_TASKS,
TASK_ALREADY_EXIST,
RUNNING_TASK_LOCK_CRASH,				 //102410
NOT_ENOUGH_BUFFER,
NOT_ENOUGH_CACHE,
INVALID_WRITE_SIZE,
INVALID_READ_SIZE,				 
ET_RESTART_CRASH,		//102415
FILE_ALREADY_EXIST,
MSG_ALREADY_EXIST,
ERR_TASK_PLAYING,
WRITE_TASK_FILE_ERR,
ETM_BROKEN_SIGNAL,		//102420
ETM_NOT_DEFINED9,
ETM_NOT_DEFINED10,
ETM_NOT_DEFINED11,
ETM_NOT_DEFINED12,
ETM_NOT_DEFINED13,		//102425
ETM_NOT_DEFINED14,
ETM_NOT_DEFINED15,
ETM_NOT_DEFINED16,
ETM_NOT_DEFINED17,
ETM_NOT_DEFINED18,		//102430
ETM_NOT_DEFINED19,
ETM_NOT_DEFINED20,
INVALID_TASK,
INVALID_TASK_ID,
INVALID_TASK_TYPE,				 //102435
INVALID_TASK_STATE,
INVALID_TASK_INFO,
INVALID_FILE_PATH,
INVALID_URL,
INVALID_SEED_FILE,				 //102440
EM_INVALID_FILE_SIZE,
INVALID_TCID,
INVALID_GCID,
INVALID_FILE_NAME,
INVALID_FILE_NUM,				 //102445
INVALID_FILE_INDEX_ARRAY,
INVALID_USER_DATA,
INVALID_EIGENVALUE,
INVALID_ORDER_LIST_SIZE,
NOT_RUNNINT_TASK,				 //102450
HIGH_SPEED_ALREADY_OPENED,
HIGH_SPEED_OPENING,
PARSE_DNS_GET_ERROR,				//102453
PARSE_DNS_TYPE_ERROR,				//102454
CONNECT_ERROR						//102455
}ETM_ERR_CODE;
#define	EM_INVALID_FILE_INDEX			  (15364)	

/***************************************************************************/
/*tree_manager*/
/***************************************************************************/
#define	TRM_ERRCODE_BASE			  (1024 * 101)	
typedef enum t_trm_err_code
{
INVALID_NODE_ID=TRM_ERRCODE_BASE+1,		// 103425
INVALID_PARENT_ID,
INVALID_CHILD,
CHILD_NOT_EMPTY,
NAME_TOO_LONG,
/*********** new add *******************/
INVALID_TREE_ID,		//103430
INVALID_NODE_NAME,
INVALID_NODE_DATA,
INVALID_NAME_HASH,
INVALID_DATA_HASH,
NODE_NOT_FOUND,		//103435
WRITE_TREE_FILE_ERR,
READ_TREE_FILE_ERR,
}TRM_ERR_CODE;

/***************************************************************************/
/*remote control*/
/***************************************************************************/
#define	RT_CTRL_BASE			        (1024 * 102)	 //104448
#define RT_CTRL_START_WRONG_TIME        (RT_CTRL_BASE + 1) 
#define RT_CTRL_STOP_WRONG_TIME         (RT_CTRL_BASE + 2) 
#define RT_PATH_NUM_ERR          (RT_CTRL_BASE + 3) //104451
#define RT_PATH_LENGTH_ERR                (RT_CTRL_BASE + 4) 
#define RT_DOWNLOAD_PATH_ERR            (RT_CTRL_BASE + 5) 

/***************************************************************************/
/*json*/
/***************************************************************************/
#define	JSOE_BASE			        (1024 * 103)	 //105472
#define    JSON_TOKEN_PARSE_ERR        (JSOE_BASE + 1) 
#define    JSON_INVALID_FUNC        	(JSOE_BASE + 2) 
#define    JSON_LIB_ERR        		(JSOE_BASE + 3) 
#define    JSON_BUFFER_NOT_ENOUGH  	(JSOE_BASE + 4) 
#define    JSON_ERR_PARA       		(JSOE_BASE + 5) 

/***************************************************************************/
/* walk_box */
/***************************************************************************/
#define	WBE_BASE			        (1024 * 104)	 //106496
typedef enum t_wb_err_code
{
WBE_WRONG_STATE=WBE_BASE+1,		// 106497
WBE_ACTION_NOT_FOUND,
WBE_PATH_TOO_LONG,
WBE_PATH_IS_DIR,			//106500
WBE_PATH_NOT_DIR,
WBE_WRONG_RESPONE,
WBE_STATUS_NOT_FOUND,
WBE_KEYWORD_NOT_FOUND,
WBE_PATH_ALREADY_EXIST,	//106505
WBE_WRONG_RESPONE_PATH,
WBE_WRONG_FILE_NUM,
WBE_NO_THUMBNAIL, 
WBE_GZED_FAILED,
WBE_DEGZED_FAILED,	//106510
WBE_UNCOMPRESS_FAILED,
WBE_XML_ENTITY_REPLACE_FAILED,
WBE_SAME_ACTION_EXIST,	
WBE_WRONG_REQUEST,
WBE_NOT_SUPPORT_LIST_OFFSET,
WBE_OTHER_ACTION_EXIST,
WBE_ENCRYPT_FAILED,
WBE_DECRYPT_FAILED,
}WB_ERR_CODE;

/***************************************************************************/
/* lixian */
/***************************************************************************/
#define	LXE_BASE			        (1024 * 105)	 //107520
typedef enum t_lx_err_code
{
LXE_ACTION_NOT_FOUND=LXE_BASE+1,		// 107521
LXE_WRONG_REQUEST,
LXE_WRONG_RESPONE,
LXE_STATUS_NOT_FOUND,
LXE_KEYWORD_NOT_FOUND,
LXE_WRONG_GCID_LIST_NUM,
LXE_WRONG_GCID,
LXE_DOWNLOAD_PIC,
LXE_SPACE_SIZE_NULL,     //107529
LXE_READ_FILE_WRONG,
LXE_FILE_NOT_EXIST,
LXE_TASK_ID_NOT_FOUND,  //107532
LXE_TASK_INFO_NOT_IN_MAP,
LXE_OUT_OF_RANGE,
LXE_NOT_LOGIN,
LXE_TASK_EMPTY,
LXE_BAD_TASK,        		//��Ӧ���������صĴ�����1  107537
LXE_LACK_SPACE,      		//��Ӧ���������صĴ�����2  107538
LXE_TASK_NOT_EXIST, 		//��Ӧ���������صĴ�����3  107539
LXE_PERMISSION_DENIED,   //��Ӧ���������صĴ�����8  107540

LXE_HS_LACK_FLUX,        	     //��Ӧ����ͨ�����������صĴ�����101  107541
LXE_HS_SERVER_INNER_ERROR,   //��Ӧ����ͨ�����������صĴ�����500  107542
LXE_HS_FILE_NOT_EXIST,       //��Ӧ����ͨ�����������صĴ�����501  107543
LXE_HS_PARAMETER_ERROR,     //��Ӧ����ͨ�����������صĴ�����502  107544
LXE_HS_TASK_NOT_EXIST,      //��Ӧ����ͨ�����������صĴ�����503  107545

LXE_GET_VOD_URL_NULL,      //��ȡ�㲥url���������ؽ��Ϊ�� 107546

LXE_GZ_ENCODE_ERROR,      //��ѹʧ��107547
LXE_GZ_DECODE_ERROR,      //��ѹʧ��107548
LXE_AES_ENCRYPT_ERROR,    //����ʧ��107549
LXE_AES_DECRYPT_ERROR,    //����ʧ��107550
}LX_ERR_CODE;

/***************************************************************************/
/* remote_control */
/***************************************************************************/
#define	RC_BASE			        107548
typedef enum t_rc_err_code
{
RCE_NOT_LOGIN = RC_BASE + 1,  // 107549
RCE_CALLBACK_NULL,	        // 107550
RCE_URL_TOO_LONG,	            // 107551
RCE_ACTION_NOT_FOUND,	        // 107552
RCE_DEVICE_INFO_NOT_FOUND,	// 107553
RCE_PATH_LIST_EMPTY,  // 107554
RCE_PATH_NOT_FOUND,	// 107555
RCE_PATH_TOO_LONG,	// 107556
RCE_BUFFER_OVERFLOW, // 107557
RCE_DEVICE_IS_BUSY,  // 107558
RCE_FILE_NAME_TOO_LONG, // 107559
RCE_TASK_INFO_NOT_FOUND,// 107560
RCE_NOT_BT_TASK,         // 107561
RCE_URL_IS_NULL,		   // 107562
RCE_FILE_PATH_IS_NULL,  // 107563
RCE_FILE_NAME_IS_NULL,  // 107564
RCE_RECV_BUFFER_NULL,   // 107565
RCE_COOKIES_TOO_LONG,   // 107566
RCE_PEERID_TOO_LONG,    // 107567
RCE_FILE_NOT_EXIST,     // 107568
RCE_READ_FILE_DATA_FAILED,  // 107569
RCE_PEERID_IS_NULL,          // 107570
RCE_FILE_PATH_TOO_LONG,     // 107571
RCE_VIP_CHANNEL_DISABLED,   // 107572
RCE_VIP_CHANNEL_USING,      // 107573
RCE_LIXIAN_CHANNEL_USING,  // 107574
RCE_ID_STATUS_TOO_LONG,    // 107575
RCE_BT_FILE_INFO_NOT_FOUND,// 107576
RCE_DEVICE_NAME_TOO_LONG,  //107577
RCE_TASK_STATUS_SAME,      //107578
RCE_TASK_LIST_NULL,        //107579
REMOTE_CONTROL_BUSY,       //107580
RCE_NOT_LOGIN_TO_DEVICE, // 107581
RCE_SYSTEM_PATH_IS_NULL,     // 107582
RCE_JSON_NO_U64_TO_CHANGE,     // 107583

}RC_ERR_CODE;
/***************************************************************************/
/* neighbor */
/***************************************************************************/
#define	NBE_BASE			        (1024 * 106)	 //108544
typedef enum t_nb_err_code
{
NBE_ACTION_NOT_FOUND=NBE_BASE+1,		// 108545
NBE_WRONG_REQUEST,
NBE_WRONG_RESPONE,
NBE_STATUS_NOT_FOUND,
NBE_KEYWORD_NOT_FOUND,
NBE_NEIGHBOR_NOT_FOUND,  //108550
NBE_RESOURCE_NOT_FOUND,
NBE_MSG_NOT_FOUND,
NBE_NOT_ENOUGH_BUFFER,
NBE_PICTURE_NOT_FOUND,
NBE_LOCATION_NOT_FOUND,	//108555
NBE_ZONE_NOT_FOUND,
NBE_SAME_ZONE,
NBE_QUERYING_NEIGHBOR,
NBE_CHANGING_ZONE,
NBE_INVALID_ZONE_NAME,
NBE_RESOURCE_NOT_CHANGED,
NBE_SEARCH_RS_CANCEL,
NBE_RESOURCE_INVALID,
NBE_RECVER_INVALID,
}NB_ERR_CODE;


#endif
