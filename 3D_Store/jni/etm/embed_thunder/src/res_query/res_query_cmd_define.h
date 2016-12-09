#ifndef _RES_QUERY_CMD_DEFINE_H_
#define _RES_QUERY_CMD_DEFINE_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#include "utility/list.h"
#include "res_query_interface.h"

#define SHUB_PROTOCOL_VER 54
#define NEW_SHUB_PROTOCOL_VER 61
#define PHUB_PROTOCOL_VER 65
#ifdef ENABLE_HSC
#define VIP_HUB_PROTOCOL_VER 65
#endif
#define	TRACKER_PROTOCOL_VER 65


/*command type*/
#define	ENROLLSP1 1005
#define ENROLLSP1_RESP 1006
#define QUERY_SERVER_RES 2001
#define SERVER_RES_RESP 2002
#define QUERY_RES_INFO_CMD_ID 2021
#define QUERY_RES_INFO_RESP_ID 2022
#define QUERY_NEW_RES_CMD_ID 2023
#define QUERY_NEW_RES_RESP_ID 2024
#define QUERY_FILE_RELATION 2041
#define QUERY_FILE_RELATION_RESP 2042
#define QUERY_PEER_RES 59
#define PEER_RES_RESP 60
#define QUERY_TRACKER_RES 22
#define TRACKER_RES_RESP 236
#define QUERY_BT_INFO 4001
#define QUERY_BT_INFO_RESP 4002
#define QUERY_CONFIG 148
#define QUERY_CONFIG_RESP 149

#ifdef ENABLE_CDN
#define QUERY_CDN_MANAGER_INFO 5001
#define QUERY_CDN_MANAGER_INFO_RESP 5002
#endif

/*by what type*/
#define QUERY_BY_CID 0x01  
#define QUERY_NO_BCID 0x02
#define QUERY_WITH_GCID 0x04
#ifdef ENABLE_CDN
#define MAX_CDN_MANAGER_RESP_LEN 1024
#endif

/*command struct*/

#ifdef ENABLE_HSC
#define VIP_HUB_ENCODE_PADDING_LEN 16
#endif
#define HTTP_HEADER_LEN 1024

typedef struct tagQUERY_SERVER_RES_CMD		/*p2s protocol version is 50*/
{
	HUB_CMD_HEADER _header;
	_u32 _client_version;
	_u16 _compress;
	_u16 _cmd_type;
	_u8 _by_what;
	_u32 _url_or_gcid_size;
	char _url_or_gcid[URL_MAX_LEN];
	_u32 _cid_size;						
	_u8 _cid[CID_SIZE];				/*cid are not use when query by url*/
	_u64 _file_size;					/*file size are not use when query by url*/
	_u32 _origin_url_size;
	char _origin_url[URL_MAX_LEN];
	_u32 _max_server_res;
	_u8 _bonus_res_num;
	_u32 _peerid_size;
	char _peerid[PEER_ID_SIZE + 1];
	_u32 _peer_report_ip;
	_u32 _peer_capability;
	_u32 _query_times_sn;
	_u8 _cid_query_type;			/*cid_query_type are not use when query by url*/
	_u32 _refer_url_size;
	char _refer_url[URL_MAX_LEN];
	_u32 _partner_id_size;
	char _partner_id[32];	//合作伙伴ID，最长长度为32
	_u32 _product_flag;
}QUERY_SERVER_RES_CMD;

typedef struct tagSERVER_RES			/*version is 50*/
{
    _u32 _url_size;
	char _url[MAX_URL_LEN];
	_u32 _refer_url_size;
	char _refer_url[MAX_URL_LEN];
	_u32 _url_speed;
	_u8 _fetch_hint;
	_u32 _connect_time;
	_u8 _gcid_type;
	_u8 _max_connection;
	_u8 _min_retry_interval;
	_u32 _control_flag;
	_u8 _url_quality;
	_u32 _user_agent;
	_u8 _res_level;
	_u8 _res_priority;
}SERVER_RES;

typedef struct tagSERVER_RES_RESP_CMD		/*p2s protocol version is 50*/
{
	HUB_CMD_HEADER _header;
	_u32	_client_version;
	_u16	_compress;
	_u16	_cmd_type;
	_u8		_result;
	_u64	_file_size;
	_u32	_cid_size;
	_u8		_cid[CID_SIZE];
	_u64	_origin_url_id;
	_u32	_server_res_num;
	_u32	_server_res_buffer_len;
	char*	_server_res_buffer;
	_u8		_bonus_res_num;
	_u32	_gcid_size;
	_u8		_gcid[CID_SIZE];
	_u32	_block_size;
	_u32	_gcid_level;
	_u8		_use_policy;
	_u32	_bcid_size;
	_u8*	_bcid;							/*buffer is malloc*/
	_u32	_tracker_ip;
	_u32	_tracker_port;
	_u32	_control_flag;
	_u32	_svr_speed_threshold;
	_u32	_put_file_size_threshold;
	_u32	_origin_url_speed;
	_u8		_origin_fetch_hint;
	_u32	_origin_connect_time;
	_u8		_max_connection;
	_u8		_min_retry_interval;
	_u8		_res_type;
	_u8		_dspider_control_flag;
	_u8		_orig_url_quality;
	_u32	_file_suffix_size;
	char		_file_suffix[16];
	_u32	_user_agent;
	_u8		_orig_res_level;
	_u8		_orig_res_priority;
}SERVER_RES_RESP_CMD;

typedef struct tagFILE_RELATION_SERVER_RES
{
	_u32 _gcid_size;
	_u8   _gcid[CID_SIZE];
	_u32  _cid_size;
	_u8    _cid[CID_SIZE];
	_u32 _file_name_size;
	_u8   _file_name[MAX_FILE_NAME_LEN];
	_u32 _file_suffix_length;
	char  _file_suffix[17];
	_u64 _file_size;
	_u32  _block_num;
	_u64  _block_total_size;
	_u32  _block_info_num;
	RELATION_BLOCK_INFO* _p_block_info;
}FILE_RELATION_SERVER_RES;

typedef struct tagRELATION_RESOURCE_INFO
{
	_u32 _url_size;
	char  _url[URL_MAX_LEN];
	_u32 _url_codepage;
	_u32 _refurl_size;
	char  _refurl[URL_MAX_LEN];
	_u32 _refurl_codepage;	
	_u8  _res_level;
	_u8  _res_priority;
}RELATION_RESOURCE_INFO;

typedef struct tagQUERY_FILE_RELATION_RES_CMD
{
	HUB_CMD_HEADER _header;
	_u32	_client_version;
	_u16	_compress;
	_u16	_cmd_type;

	_u32	_cid_size;						
	_u8		_cid[CID_SIZE];		
	_u64	_file_size;		
	_u32       _gcid_size;
	_u8         _gcid[CID_SIZE];
	_u32       _gcid_level;
	_u32       _assist_url_length;
	char		_assist_url[URL_MAX_LEN];
	_u32       _assist_url_code_page;
	_u32       _original_url_length;
	char         _original_url[URL_MAX_LEN];
	_u32       _original_url_code_page;
	_u32       _ref_url_length;
	char         _ref_url[URL_MAX_LEN];
	_u32       _ref_url_code_page;
	_u8 		_cid_query_type;
	_u32       _max_relation_resource_num;
	_u32       _peerid_length;
	char		_peerid[PEER_ID_SIZE + 1];
	_u32       _local_ip;
	_u32       _query_sn;
	_u32       _file_suffix_length;
	char        _file_suffilx[17];	
}QUERY_FILE_RELATION_RES_CMD;

typedef struct tagQUERY_FILE_RELATION_RES_RESP
{
	HUB_CMD_HEADER _header;
	_u32	_client_version;
	_u16	_compress;
	_u16	_cmd_type;
	_u8		_result;
	_u32       _cid_size;
	_u8		_cid[CID_SIZE];
	_u64       _file_size;
	_u32       _gcid_size;
	_u8         _gcid[CID_SIZE];
	_u32	_relation_server_res_num;
	_u32	_relation_server_res_buffer_len;	
	char*      _relation_server_res_buffer;
}QUERY_FILE_RELATION_RES_RESP;




typedef struct _tagQUERY_RES_INFO_CMD
{
	HUB_CMD_HEADER _header;
	_u32	_client_version;
	_u16	_compress;

	_u32  	_reserver_length;
	char*      _reserver_buffer;
	
	_u16	_cmd_type;	
	_u8 		_by_what;

	/*****by url****/
	_u32  _query_url_size;
	char   _query_url[URL_MAX_LEN];
	_u32  _query_url_code_page;

	_u32  _origion_url_size;
	char   _origion_url[URL_MAX_LEN];
	_u32  _origion_url_code_page;

	_u32  _ref_url_size;
	char   _ref_url[URL_MAX_LEN];
	_u32  _ref_url_code_page;
	/*******************/

	/****by cid*****/
	_u32 _cid_size;
	_u8	 _cid[CID_SIZE];
	_u64 _file_size;
	_u32 _cid_assist_url_size;
	char  _cid_assist_url[URL_MAX_LEN];
	_u32 _cid_assist_url_code_page;

	_u32  _cid_origion_url_size;
	char  _cid_origion_url[URL_MAX_LEN];
	_u32 _cid_origion_url_code_page;

	_u32  _cid_ref_url_size;
	char   _cid_ref_url[URL_MAX_LEN];
	_u32  _cid_ref_url_code_page;

	_u8    _cid_query_type;
	
	/**************************/

	_u32       _peerid_length;
	char		_peerid[PEER_ID_SIZE + 1];
	_u32       _local_ip;
	_u32       _query_sn;	
	
	_u32       _bcid_range;
}QUERY_RES_INFO_CMD;


typedef struct tagQUERY_RES_INFO_RESP
{
	HUB_CMD_HEADER _header;
	_u32	_client_version;
	_u16	_compress;

	_u32  	_reserver_length;
	char*      _reserver_buffer;
	
	_u16	_cmd_type;	

	_u8		_result;
	_u32 	_cid_size;
	_u8         _cid[CID_SIZE];

	_u64       _file_size;
	_u32       _gcid_size;
	_u8         _gcid[CID_SIZE];

	_u32        _gcid_part_size;
	_u32        _gcid_level;

	_u32        _bcid_size;
	_u8*	_bcid;	

	_u32       _control_flag;
	_u32       _svr_speed_threshold;
	_u32       _pub_file_size_threshold;

	_u8 		_res_type;
	_u8         _dspider_control_flag;

	_u32       _file_suffix_size;
	char  	_file_suffix[17];
	_u32       _download_strategy;

	_u8 		_is_bcid_exist;
		
}QUERY_RES_INFO_RESP;

typedef struct tagNEWQUERY_SERVER_RES_CMD
{
	HUB_CMD_HEADER _header;
	_u32	_client_version;
	_u16	_compress;

	_u32  	_reserver_length;
	char*      _reserver_buffer;
	
	_u16	_cmd_type;		

	_u32 	_cid_size;
	_u8		_cid[CID_SIZE];
	_u64       _file_size;
	_u32       _gcid_size;
	_u8         _gcid[CID_SIZE];

	_u32 	_gcid_level;
	_u32	_assist_url_size;
	char  	_assist_url[MAX_URL_LEN];
	_u32 	_assist_url_code_page;
	_u32 	_origion_url_size;
	char   	_origion_url[MAX_URL_LEN];
	_u32  	_origion_url_code_page;
	_u32 	_ref_url_size;
	char 	_ref_url[MAX_URL_LEN];
	_u32 	_ref_url_code_page;

	_u8 		_cid_query_type;
	_u32 	_max_server_res;
	_u8  	_bonus_res_num;
	
	_u32       _peerid_length;
	char		_peerid[PEER_ID_SIZE + 1];
	_u32       _local_ip;
	_u32       _query_sn;	

	_u32	_file_suffix_size;
	char		_file_suffix[17];
	
}NEWQUERY_SERVER_RES_CMD;


typedef struct tagNEWQUERY_SERVER_RES_RESP
{
	HUB_CMD_HEADER _header;
	_u32	_client_version;
	_u16	_compress;

	_u32  	_reserver_length;
	char*      _reserver_buffer;

	_u16	_cmd_type;	

	_u8 		_result;
	_u32       _cid_size;
	_u8 		_cid[CID_SIZE];
	_u64 	_file_size;
	_u32	_gcid_size;
	_u8 		_gcid[CID_SIZE];
	_u32	_server_res_num;
	_u32	_server_res_buffer_len;
	char*	_server_res_buffer;
	_u8		_bonus_res_num;
}NEWQUERY_SERVER_RES_RESP;

typedef struct tagNEWSERVER_RES
{
	_u32   _url_size;
	char   _url[MAX_URL_LEN];
	_u32   _url_code_page;
	_u32   _ref_url_size;
	char   _ref_url[MAX_URL_LEN];
	_u32   _ref_url_code_page;

	_u32  _url_speed;
	_u8 	  _fetch_hint;
	_u32  _connect_time;

	_u8 	 _gcid_type;
	_u8  _max_connection;
	_u8 _min_retry_interval;

	_u32 _control_flag;
	_u8  _url_quality;
	_u32 _user_agent;
	_u8  _resource_level;
	_u8 	_resource_priority;

	
}NEWSERVER_RES;



typedef struct tagQUERY_PEER_RES_CMD
{
	_u32	_protocol_version;
	_u32	_seq;
	_u32	_cmd_len;
	_u8		_cmd_type;
	_u32	_peerid_size;
	char		_peerid[PEER_ID_SIZE + 1];
	_u32	_cid_size;
	_u8		_cid[CID_SIZE];
	_u64	_file_size;
	_u32	_gcid_size;
	_u8		_gcid[CID_SIZE];
	_u32	_peer_capability;
	_u32	_internal_ip;
	_u32	_nat_type;
	_u8		_level_res_num;
	_u8		_resource_capability;	/* 0:普通查询	1:仅需要发行Server	 2:仅需要外网资源	3:仅需要内网资源	4. 需要所有种类资源(与0的差别在于如果有cdn资源必须返回) */
	_int32	_server_res_num;
	_u32	_query_times;
	_u32	_p2p_capability;
	_u32	_upnp_ip;
	_u16	_upnp_port;
	_u8		_rsc_type;	/* 1（普通cid查询，即客户端算出来的cid查询），2（社区博客来的cid查询），3（bt来的cid查询）, 4（emule来的cid查询），5 （点播任务的查询）*/
	_u32	_partner_id_size;	
	char		_partner_id[32];	//合作伙伴ID，最长长度为32
	_u32	_product_flag;	
}QUERY_PEER_RES_CMD;

typedef struct tagPEER_RES
{
	_u32	_peerid_size;
	char		_peerid[PEER_ID_SIZE + 1];
	/*
	_u32	_file_name_size;		//just use for old p2p version
	char	_file_name[256];
	*/
	_u32	_internal_ip;
	_u16	_tcp_port;
	_u16	_udp_port;
	_u8		_resource_level;
	_u8		_resource_priority;
	_u32	_peer_capability;
}PEER_RES;

typedef struct tagPEER_RES_RESP_CMD
{
	HUB_CMD_HEADER _header;
	_u8		_cmd_type;
	_u8		_result;
	_u32	_cid_size;
	_u8 		_cid[CID_SIZE];
	_u64	_file_size;
	_u32	_gcid_size;
	_u8		_gcid[CID_SIZE];
	_u8		_bonus_res_num;
	_u32	_peer_res_num;
	_u32	_peer_res_buffer_len;
	char*	_peer_res_buffer;
	_u32    	_resource_count;
	_u16    	_query_interval;
}PEER_RES_RESP_CMD;

typedef struct tagQUERY_TRACKER_RES_CMD
{
	_u32	_protocol_version;
	_u32	_seq;
	_u32	_cmd_len;
	_u8		_cmd_type;
	_u32	_last_query_stamp;
	_u8		_by_what;
	_u32	_gcid_size;
	_u8		_gcid[CID_SIZE];
	_u64	_file_size;
	_u32	_peerid_size;
	char		_peerid[PEER_ID_SIZE + 1];
	_u32	_local_ip;
	_u16	_tcp_port;
	_u8		_max_res;
	_u32	_nat_type;
	_u8		_download_ratio;
	_u64	_download_map;
	_u32	_peer_capability;	
	_u32	_release_id;
	_u32	_upnp_ip;
	_u16	_upnp_port;
	_u32	_p2p_capability;
	_u16	_udp_port;
	_u8		_rsc_type;		//查询的类型：1（普通cid查询，即客户端算出来的cid查询），2（社区博客来的cid查询），3（bt来的cid查询）, 4（emule来的cid查询），5 （点播任务的查询）
	_u32	_partner_id_size;
	char		_partner_id[32];
	_u32	_product_flag;
}QUERY_TRACKER_RES_CMD;

typedef struct tagTRACKER_RES
{
	_u32	_peerid_size;
	char		_peerid[PEER_ID_SIZE + 1];
	_u32	_internal_ip;
	_u16	_tcp_port;
	_u16	_udp_port;
	_u8		_resource_level;
	_u8		_resource_priority;
	_u32	_peer_capability;
}TRACKER_RES;

typedef struct tagTRACKER_RES_RESP_CMD
{
	HUB_CMD_HEADER _header;		/*version must greate than 58*/
	_u8		_cmd_type;
	_u8		_result;
	_u32	_tracker_res_num;
	_u32	_tracker_res_buffer_len;
	char*	_tracker_res_buffer;
	_u32	_reserve_size;			/*must be 0*/
	_u32	_query_stamp;
	_u16	_query_interval;
}TRACKER_RES_RESP_CMD;

typedef struct tagQUERY_BT_INFO_CMD
{
    HUB_CMD_HEADER _header;
    _u32 _client_version;
    _u16 _compress;
    _u16 _cmd_type;
    _u32 _peerid_len;
    char _peerid[PEER_ID_SIZE + 1];
    _u32 _info_id_len;
    char _info_id[CID_SIZE];
    _u32 _file_index;
    _u32 _query_times;
    _u8 _need_bcid;
}QUERY_BT_INFO_CMD;

typedef struct tagQUERY_BT_INFO_RESP_CMD
{
	HUB_CMD_HEADER _header;
	_u32 _client_version;
	_u16 _compress;
	_u16 _cmd_type;
	_u8 _result;
	_u32 _has_record;
	_u64 _file_size;
	_u32 _cid_size;
	_u8 _cid[CID_SIZE];
	_u32 _gcid_size;
	_u8 _gcid[CID_SIZE];
	_u32 _gcid_part_size;
	_u32 _gcid_level;
	_u32 _control_flag;
	_u32 _bcid_size;
	_u8* _bcid;
}QUERY_BT_INFO_RESP_CMD;

typedef struct tagENROLLSP1_CMD
{
	HUB_CMD_HEADER	_header;
	_u32	_client_version;
	_u16	_compress;
	_u16	_cmd_type;
	_u32	_local_peerid_len;
	char		_local_peerid[PEER_ID_SIZE + 1];
	_u32	_internal_ip_len;
	char		_internal_ip[24];
	_u32	_product_flag;
	_u32	_thunder_version_len;
	char		_thunder_version;
	_u32	_network_type;
	_u32	_os_type;
	_u32	_os_details_len;
	char		_os_details[32];
	_u32	_user_details_len;
	char		_user_details[32];
	_u32	_plugin_size;
	LIST		_plugin_list;
	_u32	_enable_login_control;
	_u32	_filter_peerid_version;
	_u32	_user_agent_version;
	_u32	_partner_id_len;
	char		_partner_id[32];	//合作伙伴ID，最长长度为32
	_u32	_file_head_suffix_map_version;
}ENROLLSP1_CMD;

typedef struct tagCONF_SETTING_NODE
{
	_u32	_section_size;
	char		_section[128];
	_u32	_name_size;
	char		_name[128];
	_u32	_value_size;
	char		_value[128];
}CONF_SETTING_NODE;

typedef struct tagSERVERS_NODE
{
	_u32	_server_size;
	char		_server[MAX_URL_LEN];
	_u32	_server_ip;
	_u32	_server_port;
}SERVERS_NODE;

typedef struct tagENROLLSP1_RESP_CMD
{
	HUB_CMD_HEADER	_header;
	_u32	_client_version;
	_u16	_compress;
	_u16	_cmd_type;
	_u8		_result;
	_u32	_latest_version_size;
	char		_latest_version[24];
	_u8		_login_hint;
	_u32	_login_desc_size;
	char		_login_desc[256];
	_u32	_login_associate_size;
	char		_login_associate[256];
	_u32	_client_conf_setting_size;
	LIST		_client_conf_setting_list;	/*list node is CONF_SETTING_NODE*/
	_u32	_servers_list_size;
	LIST		_servers_list;				/*list node is SERVERS_NODE*/
	_u32	_external_ip;
}ENROLLSP1_RESP_CMD;

#ifdef ENABLE_CDN
typedef struct tagCDNManagerQuery
{
	HUB_CMD_HEADER _header;
	_int32 _version;
	_u8 _gcid[CID_SIZE];
}CDNMANAGERQUERY;

typedef struct tagQUERY_PEER_RES_CMD QUERY_NOMAL_CDN_CMD;

#endif /* ENABLE_CDN */

typedef struct tagQUERY_CONFIG_CMD
{
    HUB_CMD_HEADER _header;
    _u8	_cmd_type;
    _u32 _local_peerid_len;
    char _local_peerid[PEER_ID_SIZE + 1];
    _u32 _peer_capability;
    _u32 _product_flag;
    _u32 _os_type;
    _u32 _os_details_len;
    char _os_details[MAX_OS_LEN];
    _u32 _ui_version_len;
    char _ui_version[64];
    _u32 _thunder_version_len;
    char _thunder_version[64];
    _u32 _network_type;
    _u8 _enable_bt;
    _u8 _enable_emule;
    _u8 _storage_type;
    _u32 _timestamp;
    _u32 _config_section_filter_len;
    char _config_section_filter[256];
}QUERY_CONFIG_CMD;

typedef struct tagQUERY_CONFIG_RESP_CMD
{
	HUB_CMD_HEADER	_header;
	_u8	       _cmd_type;
	_u8         _result;
	_u32       _timestamp;
	_u32	_setting_size;
	LIST		_setting_list;	/*list node is CONF_SETTING_NODE*/
}QUERY_CONFIG_RESP_CMD;


#ifdef __cplusplus
}
#endif
#endif
