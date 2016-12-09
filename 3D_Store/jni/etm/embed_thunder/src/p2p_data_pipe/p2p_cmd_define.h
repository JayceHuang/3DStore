/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2008/07/02
-----------------------------------------------------------------------------------------------------------*/

#ifndef _P2P_CMD_DEFINE_H_
#define _P2P_CMD_DEFINE_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"

/*p2p protocol version define*/
#define		P2P_PROTOCOL_VERSION		62		/*this p2p modular version*/
#define		P2P_PROTOCOL_VERSION_50		50
#define		P2P_PROTOCOL_VERSION_51		51
#define		P2P_PROTOCOL_VERSION_54		54	
#define		P2P_PROTOCOL_VERSION_57		57
#define		P2P_PROTOCOL_VERSION_58		58
#define		P2P_PROTOCOL_VERSION_60		60

/*p2p response result*/
#define		P2P_RESULT_NONE				0xff
#define		P2P_RESULT_SUCC				0
#define		P2P_RESULT_PROTOCOL_INVALID	1		//不支持该协议版本
#define		P2P_RESULT_FAIL				2		//处理失败(旧版P2P也表示文件不存在)
#define		P2P_RESULT_INVALID_PARAM	101		//参数非法
#define		P2P_RESULT_FILE_NOT_EXIST	102		//文件不存在
#define		P2P_RESULT_BOTH_UPLOAD		103		//双方都是纯上传
#define		P2P_RESULT_READ_DATA_ERROR	104		//从文件读取数据失败
#define		P2P_RESULT_ADD_TASK_ERROR	105		//添加任务失败
#define		P2P_RESULT_UPLOAD_OVER_MAX	106		//上传超过上限
#define		P2P_RESULT_INVALID_CMD		107		//命令字对方不认识


#define		P2P_CMD_HEADER_LENGTH		9	/* version(4) + length(4) + command_type(1) */

/*p2p command type*/
#define		HANDSHAKE					100
#define		HANDSHAKE_RESP				101
#define		INTERESTED					102
#define		INTERESTED_RESP				103
#define		NOT_INTERESTED				104
#define		KEEPALIVE					105
#define		REQUEST						106
#define		REQUEST_RESP				107
#define		CANCEL						108
#define		CANCEL_RESP					109
#define		UNKNOWN_COMMAND				112		/*remote peer unknown your command*/
#define		CHOKE						113
#define		UNCHOKE						114
#define		FIN							115
#define		FIN_RESP					116

_int32 is_p2p_cmd_valid(_u8 cmd_type);

/*p2p tcp command struct define*/
typedef struct tagP2P_TCP_CMD_HEADER
{
	_u32	_version;
	_u32	_command_len;
	_u8		_command_type;
}P2P_TCP_CMD_HEADER;

/*p2p file status*/
#define P2P_FILE_STATUS_NONE				0xff
#define P2P_FILE_STATUS_NOT_EXIST			0x00 
#define P2P_FILE_STATUS_DOWNLOADING			0x01 
#define P2P_FILE_STATUS_ALREADY_EXIST		0x02 

typedef struct tagHANDSHAKE_CMD
{
	P2P_TCP_CMD_HEADER	_header;
	_u32				_handshake_connect_id;
	_u8					_by_what;
	_u32				_gcid_len;
	_u8					_gcid[CID_SIZE];
	_u64				_file_size;
	_u8					_file_status;
	_u32				_peerid_len;
	char					_peerid[PEER_ID_SIZE + 1];
	_u32				_internal_addr_len;
	char					_internal_addr[24];
	_u32				_tcp_listen_port;
	_u32				_product_ver;
	_u64				_downbytes_in_history;
	_u64				_upbytes_in_history;
	_u8					_not_in_nat;		/* 1 -- in wan;	0 -- in nat */
	_u32				_upload_speed_limit;
	_u32				_same_nat_tcp_speed_max;
	_u32				_diff_nat_tcp_speed_max;
	_u32				_udp_speed_max;
	_u32				_p2p_capability;
	_u32				_phub_res_count;		//该参数很重要，小于5的话，在对端的上传调度中，会加10分优先级
	_u8					_load_level;
	_u32				_home_location_len;
	char					_home_location[256];
       _u32                            _res_type;
}HANDSHAKE_CMD;

typedef struct tagHANDSHAKE_RESP_CMD
{
	P2P_TCP_CMD_HEADER	_header;
	_u8					_result;
	_u32				_peerid_len;
	char					_peerid[PEER_ID_SIZE + 1];
	_u32				_product_ver;
	_u64				_downbytes_in_history;
	_u64				_upbytes_in_history;
	_u8					_not_in_nat;
	_u32				_upload_speed_limit;
	_u32				_same_nat_tcp_speed_max;
	_u32				_diff_nat_tcp_speed_max;
	_u32				_udp_speed_max;
	_u32				_p2p_capablility;
	_u32				_phub_res_count;
	_u8					_load_level;
	_u32				_home_location_len;
	char					_home_location[256];
}HANDSHAKE_RESP_CMD;

typedef struct tagINTERESTED_CMD
{
	P2P_TCP_CMD_HEADER _header;
	_u8				   _by_what;
	_u32			   _min_block_size;
	_u32			   _max_block_count;
}INTERESTED_CMD;

typedef struct tagBLOCK
{
	_u8		_store_type;
	_u64	_file_pos;
	_u64	_file_len;
}BLOCK;
typedef struct tagINTERESTED_RESP_CMD
{
	P2P_TCP_CMD_HEADER _header;
	_u8				   _download_ratio;
	_u32			   _block_num;
	_int32			   _block_data_len;
	char*			   _block_data;
}INTERESTED_RESP_CMD;

typedef struct tagCANCEL_CMD
{
	P2P_TCP_CMD_HEADER _header;
}CANCEL_CMD;

typedef struct tagCANCEL_RESP_CMD
{
	P2P_TCP_CMD_HEADER _header;
}CANCEL_RESP_CMD;

typedef struct tagREQUEST_CMD
{
	P2P_TCP_CMD_HEADER	_header;
	_u8					_by_what;
	_u64				_file_pos;
	_u64				_file_len;
	_u32				_max_package_size;
	_u8					_priority;
	_u32				_upload_speed;
	_u32				_unchoke_num;
	_u32				_pipe_num;
	_u32				_task_num;
	_u32				_local_requested;
	_u32				_remote_requested;
	_u8					_file_ratio;
}REQUEST_CMD;

typedef struct tagREQUEST_RESP_CMD
{
	P2P_TCP_CMD_HEADER _header;
	_u8				   _result;
	_u64			   _data_pos;
	_u32			   _data_len;
	char*			   _data;
	_u32			   _upload_speed;
	_u32			   _unchoke_num;
	_u32			   _pipe_num;
	_u32			   _task_num;
	_u32			   _local_request;
	_u32			   _remote_request;
	_u8				   _file_ratio;
}REQUEST_RESP_CMD;

typedef struct tagKEEPALIVE_CMD
{
	P2P_TCP_CMD_HEADER _header;
}KEEPALIVE_CMD;

typedef struct tagCHOKE_CMD
{
	_u32	_version;
	_u32	_cmd_len;
	_u8		_cmd_type;
}CHOKE_CMD;

typedef struct tagUNCHOKE_CMD
{
	_u32	_version;
	_u32	_cmd_len;
	_u8		_cmd_type;
}UNCHOKE_CMD;

typedef struct tagFIN_RESP_CMD
{
	_u32	_version;
	_u32	_cmd_len;
	_u8		_cmd_type;
}FIN_RESP_CMD;

#ifdef __cplusplus
}
#endif

#endif

