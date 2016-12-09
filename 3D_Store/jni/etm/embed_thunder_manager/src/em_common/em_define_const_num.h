#ifndef EM_DEFINE_CONST_NUM_H_20090107
#define EM_DEFINE_CONST_NUM_H_20090107


/*  无线产品id，由易鹏分配
19.安卓离线下载；
39.安卓云播；
33.iPad迅雷云HD
42.iPhone迅雷云
48.云播iPhone
49.云播iPad        
51.会员手机支付中心 for Android  
70.安卓云播TV版  */
#define PRODUCT_ID_ANDROID_LIXIAN (19)
#define PRODUCT_ID_ANDROID_YUNBO (39)
#define PRODUCT_ID_YUN_HD (33)
#define PRODUCT_ID_YUN_IPHONE (42)
#define PRODUCT_ID_YUNBO_IPHONE (48)
#define PRODUCT_ID_YUNBO_HD (49)
#define PRODUCT_ID_PAY_CENTRE (51)
#define PRODUCT_ID_YUNBO_TV (70)

/* bussinesstype	由帐号体系陈泉分配,同一bussinesstype 的产品之间会互踢
19.安卓离线下载；
20.安卓云播；
21.iPad迅雷云HD
22.iPhone迅雷云
27.云播iPhone
28.云播iPad        
29.会员手机支付中心 for Android     
42.安卓云播TV版  */
#define BUSSINESS_TYPE_ANDROID_LIXIAN (19)
#define BUSSINESS_TYPE_ANDROID_YUNBO (20)
#define BUSSINESS_TYPE_YUN_HD (21)
#define BUSSINESS_TYPE_YUN_IPHONE (22)
#define BUSSINESS_TYPE_YUNBO_IPHONE (27)
#define BUSSINESS_TYPE_YUNBO_HD (28)
#define BUSSINESS_TYPE_PAY_CENTRE (29)  
#define BUSSINESS_TYPE_YUNBO_TV (42)  

#define DEFAULT_LOCAL_HTTP_SERVER_PORT (26122)

#define DEFAULT_YUNBO_URL_MARK "&from=ipad_interface"  /* 云播专用URL 标识符 */
#define ETM_YUNBO_REPORT_PROTOCOL "xl_cloudplay_etmvod"		/* 下载库关于云播的上报协议 */
#define EM_TEST_LOGIN_REPORT_PROTOCOL "xl_cloudplay_testwork"	/* 下载库测试网络的上报协议 */
#define EM_PROGRESS_BACK_REPORT_PROTOCOL "xl_cloudplay_progressback"	/* 下载库下载回退上报协议 */
/********* 无线数据中心域名*********/
#define NEW_HTTP_REPORT_HOST_NAME 	DEFAULT_LOCAL_IP	 //"pgv.m.vip.xunlei.com"	/* 会员部门的统计服务器 */
#define NEW_HTTP_REPORT_PORT (8090)

#ifdef CLOUD_PLAY_PROJ
#define DEFAULT_HTTP_REPORT_HOST_NAME NEW_HTTP_REPORT_HOST_NAME
#define DEFAULT_HTTP_REPORT_PORT NEW_HTTP_REPORT_PORT
#else
#define DEFAULT_HTTP_REPORT_HOST_NAME 	DEFAULT_LOCAL_IP	 //"pgv.m.xunlei.com"	/* 无线部门的统计服务器 */
#define DEFAULT_HTTP_REPORT_PORT (80)
#endif /* CLOUD_PLAY_PROJ */

#ifdef	ENABLE_ETM_MEDIA_CENTER
#define EM_SAVE_DLED_SIZE_INTERVAL	180
#else
#define EM_SAVE_DLED_SIZE_INTERVAL 15
#endif
#define MAX_ET_IDLE_INTERVAL 	(120*1000)    //120 seconds

#ifdef EM_MEMPOOL_64K
#define EM_MIN_LIST_MEMORY 128

#define EM_MIN_SET_MEMORY 128
#define EM_MIN_MAP_MEMORY 64

#define EM_MIN_MSG_COUNT 64
#define EM_MIN_QUEUE_MEMORY 64
#define EM_MIN_SETTINGS_ITEM_MEMORY 16
#define EM_MIN_TIMER_COUNT 8

#define MIN_TREE_NODE_MEMORY 32
#define MIN_NODE_NAME_MEMORY 32
#define MIN_TREE_MEMORY 3

#define MIN_VOD_TASK_MEMORY 1

#define MIN_TASK_INFO_MEMORY 16
#define MIN_DL_TASK_MEMORY 16
#define MIN_DT_EIGENVALUE_MEMORY 16
#define MAX_CACHE_NUM 8
#define MIN_BT_TASK_MEMORY 1
#define MIN_P2SP_TASK_MEMORY 3
#define MIN_BT_RUNNING_FILE_MEMORY 1

#define MIN_MINI_TASK_MEMORY 1
#else
#define EM_MIN_LIST_MEMORY 256

#define EM_MIN_SET_MEMORY 256
#define EM_MIN_MAP_MEMORY 128

#define EM_MIN_MSG_COUNT 64
#define EM_MIN_QUEUE_MEMORY 64
#define EM_MIN_SETTINGS_ITEM_MEMORY 16
#define EM_MIN_TIMER_COUNT 16

#define MIN_TREE_MEMORY 1
#define MIN_TREE_NODE_MEMORY 32
#define MIN_NODE_NAME_MEMORY 32


#define MIN_VOD_TASK_MEMORY 2

#define MIN_TASK_INFO_MEMORY 32
#define MIN_DL_TASK_MEMORY 32
#define MIN_DT_EIGENVALUE_MEMORY 32
#define MAX_CACHE_NUM 16
#define MIN_BT_TASK_MEMORY 4
#define MIN_P2SP_TASK_MEMORY 4
#define MIN_BT_RUNNING_FILE_MEMORY 3

#define MIN_MINI_TASK_MEMORY 1
#endif

#define DEFAULT_PATH "./"


#define ETM_CONFIG_FILE "runmit_dle.cfg"

#define LICENSE_LEN 42


#define MAX_TASK_NUM (0xFFFF)

#define  MAX_FILE_INDEX		(0xFFFF)
#define  MAX_FILE_NUM			(0xFFFF)
#define MAX_DL_TASK_ID 0x80000000
#define VOD_NODISK_TASK_MASK  0xA0000000
#define MAX_LOCAL_DL_TASK_ID (MAX_DL_TASK_ID/2)
#define MAX_RUNNING_TASK_NUM 3

#define EM_LOG_CONFIG_FILE LOG_CONFIG_FILE //"log_etm.conf"

#define RESTART_ET_INTERVAL (1*1000)

#define MAX_TREE_ID 0x80000000
#define MAX_NODE_ID 0x80000000


#define ETM_DEFAULT_VOD_CACHE_SIZE (1024*1024)  // 1M*KB
#define ETM_EMPTY_DISK_FILE "runmit_cache_file.dat"
#define ETM_EMPTY_DISK_FILE_SIZE (1024*1024)

/* 登录默认超时时间 */
#define MEMBER_DEFAULT_LOGIN_TIMEOUT (10)
//注册消息服务器及端口
#define MEMBER_REGISTER_SERVER_HOST	DEFAULT_LOCAL_IP	 //"http://accord.pad.sandai.net/accordServer"

//会员VIP http服务器地址及端口
#define MEMBER_VIP_HTTP_SERVER_HOST	DEFAULT_LOCAL_IP	 //"cache2.vip.xunlei.com"
#define MEMBER_VIP_HTTP_SERVER_PORT	(8001)

#define DEFAULT_VIP_HUB_HOST_NAME 	DEFAULT_LOCAL_IP	 //"viphub5pr.phub.sandai.net"
#define DEFAULT_VIP_HUB_PORT 80

#define DEFAULT_REPORT_INTERVAL  600	// 每10分钟上报一次wapstat.wap.sandai.net:83
#define	REPORTER_TIMEOUT		5000
#define DEFAULT_CMD_RETRY_TIMES 2

/********* 离线空间相关域名*********/
#define DEFAULT_LIXIAN_HOST_NAME 	DEFAULT_LOCAL_IP	 //"pad.i.vod.xunlei.com"// "10.10.199.26" //"127.0.0.1"  /* xml 协议服务器 */
#define DEFAULT_LIXIAN_PORT 80//21011 //80   //(8889)  //

#define DEFAULT_LIXIAN_SERVER_HOST_NAME 	DEFAULT_LOCAL_IP	 //"service.lixian.vip.xunlei.com"   /* 二进制协议服务器 */
#define DEFAULT_LIXIAN_SERVER_PORT 80

#define DEFAULT_LIXIAN_GDL_HOST_NAME 	DEFAULT_LOCAL_IP	 //"gdl.lixian.vip.xunlei.com"  /* 云播调度服务器 */
#define DEFAULT_LIXIAN_GDL_PORT 80

#define DEFAULT_VOD10_HOST_NAME 	DEFAULT_LOCAL_IP	 //"vod10.t17.lixian.vip.xunlei.com"  /* 云播CDN 服务器 */

#define DEFAULT_HIGH_SPEED_SERVER_HOST_NAME 	DEFAULT_LOCAL_IP	 //"service.cdn.vip.xunlei.com"   /* 高速扣费协议服务器 */
#define DEFAULT_HIGH_SPEED_SERVER_PORT 80

/***************************************************************************/
/*  remote control*/
/***************************************************************************/

#define EM_MAX_LICENSE_LEN 256
#define EM_MAX_RT_PATH_NUM 3
#define EM_MAX_GROUP_NAME_LEN 48
#define EM_MAX_MSG_LEN 32
#define EM_MAX_ACTIVE_KEY_LEN 16
#define EM_MAX_USERID_LEN 20
#define EM_MAX_USERNAME_LEN 32
#define EM_MAX_PATH_NUM 3


#endif 


