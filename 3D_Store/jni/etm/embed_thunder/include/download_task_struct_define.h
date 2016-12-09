#ifndef DOWNLOAD_TASK_STRUCT_DEFINE_H
#define DOWNLOAD_TASK_STRUCT_DEFINE_H

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"

/* bit of  Control Flag of QUERYSRESRESP */
#define CF_BCID_RETRY 			(0x01)
#define CF_VOTE 					(0x02)
#define CF_REPORTHOST 			(0x04)
#define CF_REPORTDWFAIL 		(0x08)
#define CF_URL_LEVEL	 		(0x10)

/* bit of  Insert Course of INSERTSRES */
#define IC_SHUB_NO_CID 			(0x01)
#define IC_GCID_LEVEL_UPGRADE 	(0x02)
#define IC_GCID_CHANGE 			(0x04)
#define IC_SHUB_NO_BCID 		(0x08)
#define IC_SHUB_NO_SUFFIX	 	(0x10)
#define IC_CID_CHANGE			(0x20)


enum TASK_TYPE {P2SP_TASK_TYPE= 0, BT_TASK_TYPE, EMULE_TASK_TYPE, BT_MAGNET_TASK_TYPE};
enum TASK_STATUS {TASK_S_IDLE = 0, TASK_S_RUNNING, TASK_S_VOD, TASK_S_SUCCESS, TASK_S_FAILED,TASK_S_STOPPED};
enum TASK_FILE_CREATE_STATUS {FILE_NOT_CREATED = 0, FILE_CREATED_SUCCESS,  FILE_CREATED_FAILED};
enum TASK_RES_QUERY_STATE {RES_QUERY_IDLE= 0, RES_QUERY_REQING,  RES_QUERY_SUCCESS, RES_QUERY_FAILED, RES_QUERY_END};

enum TASK_SHUB_QUERY_STEP{QUERY_SHUB_RES_INFO_STEP = 0, QUERY_SHUB_SERVER_RES_STEP};
const static _u32 S_USE_NORMAL_MULTI_RES = 1;
const static _u32 S_USE_PC_RES = 2;
const static _u32 S_SMALL_FILE_MODE = 4;

#ifdef EMBED_REPORT
typedef struct tagRES_QUERY_REPORT_PARA
{
	_u32  _shub_query_time;//ms
	_u32  _phub_query_time;//ms
	_u32  _tracker_query_time;//ms
	
	_u32  _hub_s_max;					 //Shub查询的最大时间(ms)															
	_u32  _hub_s_min;					 //Shub查询的最小时间(ms)															
	_u32  _hub_s_avg;					 //Shub查询的平均时间(ms)															
	_u32  _hub_s_succ;					 //Shub查询成功次数 																
	_u32  _hub_s_fail;					 //Shub查询失败次数 	
	
	_u32  _hub_p_max;					 //Phub查询的最大时间(ms)															
	_u32  _hub_p_min;					 //Phub查询的最小时间(ms)															
	_u32  _hub_p_avg;					 //Phub查询的平均时间(ms)															
	_u32  _hub_p_succ;					 //Phub查询成功次数 																
	_u32  _hub_p_fail;					 //Phub查询失败次数 	
	
	_u32  _hub_t_max;					 //Tracker查询的最大时间(ms) 														
	_u32  _hub_t_min;					 //Tracker查询的最小时间(ms) 														
	_u32  _hub_t_avg;					 //Tracker查询的平均时间(ms) 														
	_u32  _hub_t_succ;					 //Tracker查询成功次数																
	_u32  _hub_t_fail;					 //Tracker查询失败次数		

	_u32  _normal_cdn_fail;			     //普通cdn查询的失败次数

} RES_QUERY_REPORT_PARA;
#endif


typedef struct _tagDPHUB_QUERY_CONTEXT
{
    LIST        _to_query_node_list;    // 本任务所有待查的dphub节点
    LIST        _exist_node_list;       // 曾经查询过的dphub节点
    BOOL        _is_query_root_finish;  // 是否已经查询完root节点了
    _u32        _current_peer_rc_num;   // 当前已经查询到的peer_rc的个数
    _u32        _max_res;               // 服务器返回的需要最大资源数
    _u32        _wait_dphub_root_timer_id;  // 等待root查询回来的定时器
} DPHUB_QUERY_CONTEXT;
_int32 init_dphub_query_context(DPHUB_QUERY_CONTEXT *context);
_int32 uninit_dphub_query_context(DPHUB_QUERY_CONTEXT *context);

typedef struct t_edt_server_res
{
	_u32 _file_index;
	_u32 _resource_priority;
	char* _url;							
	_u32 _url_len;					
	char* _ref_url;				
	_u32 _ref_url_len;		
	char* _cookie;				
	_u32 _cookie_len;		
}EDT_SERVER_RES;
typedef struct t_edt_peer_res
{
	_u32 _file_index;
	char _peer_id[PEER_ID_SIZE];							
	_u32 _peer_capability;					
	_u32 _res_level;
	_u32 _host_ip;
	_u16 _tcp_port;
	_u16 _udp_port;
}EDT_PEER_RES;

typedef enum t_edt_res_type
{
	EDT_SERVER = 0, 
	EDT_PEER
} EDT_RES_TYPE;
typedef struct t_edt_res 
{
	EDT_RES_TYPE _type;
        union 
	{
                EDT_SERVER_RES _s_res;
		  EDT_PEER_RES _p_res;
        } I_RES;
#define edt_s_res  I_RES._s_res
#define edt_p_res  I_RES._p_res
}EDT_RES;

typedef struct t_assit_task_info
{
	char   	_gcid[CID_SIZE*2 + 1];
	char   	_cid[CID_SIZE*2  + 1];
	_u64 	_file_size;
	char  	_info_id[CID_SIZE*2 + 1];
	_u32 	_file_index;
	_u64 	_download_size;
}ASSIST_TASK_INFO;

typedef struct t_assist_task_property
{
	char   	_gcid[CID_SIZE*2 + 1];
	char   	_cid[CID_SIZE*2  + 1];
	_u64 	_file_size;
}ASSIST_TASK_PROPERTY;

#ifdef __cplusplus
}
#endif


#endif // !defined(__DOWNLOAD_TASK_H_20080918)
