/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2008/10/6
-----------------------------------------------------------------------------------------------------------*/
#ifndef _PTL_CMD_DEFINE_H_
#define _PTL_CMD_DEFINE_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#include "utility/list.h"

/*ptl protocol version define*/
#define		PTL_PROTOCOL_VERSION			59		/*目前使用的ptl版本是59*/
#define		PTL_PROTOCOL_VERSION_50		50
#define		PTL_PROTOCOL_VERSION_51		51
#define		PTL_PROTOCOL_VERSION_58		58
#define		PTL_PROTOCOL_VERSION_59		59

/*ptl command define*/
#define		GET_PEERSN						0
#define		GET_MYSN						1
#define		PING_SN						2
#define		ICALLSOMEONE					3
#define		SOMEONECALLYOU				4
#define		PUNCH_HOLE					5
#define		SYN								6
#define		NN2SN_LOGOUT					10
#define		SN2NN_LOGOUT					11
#define		PING							12
#define		LOGOUT							13
#define		BROKER1						111	
#define		BROKER2_REQ					130
#define		BROKER2						131
#define		TRANSFER_LAYER_CONTROL		132
#define		TRANSFER_LAYER_CONTROL_RESP	133
#define		UDP_BROKER_REQ				134
#define		UDP_BROKER						135
#define		ICALLSOMEONE_RESP				252
#define		PING_SN_RESP					253
#define		GET_MYSN_RESP					254
#define		GET_PEERSN_RESP				255

/*ptl command define*/
#define GET_PEERSN_CMD_LEN				25

typedef struct tagGET_PEERSN_CMD
{
	_u32	_version;
	_u8		_cmd_type;
	_u32	_target_peerid_len;
	char	_target_peerid[PEER_ID_SIZE + 1];
}GET_PEERSN_CMD;

typedef struct tagGET_MYSN_CMD
{
	_u32	_version;
	_u8		_cmd_type;
	_u32	_local_peerid_len;
	char		_local_peerid[PEER_ID_SIZE + 1];
	//失效的SN的peerID数组，目前新版只有一个my_sn
	_u32	_invalid_sn_num;
	_u32	_invalid_sn_peerid_len;
	char		_invalid_sn_peerid[PEER_ID_SIZE + 1];
}GET_MYSN_CMD;

typedef struct tagICALLSOMEONE_CMD	
{
	_u32	_version;
	_u8		_cmd_type;
	_u32	_local_peerid_size;
	char		_local_peerid[PEER_ID_SIZE + 1];
	_u32	_remote_peerid_size;
	char		_remote_peerid[PEER_ID_SIZE + 1];
	_u16	_virtual_port;
	_u32	_nat_type;
	_u16	_latest_extenal_port;
	_u32	_time_elapsed;
	_int32	_delta_port;
}ICALLSOMEONE_CMD;

typedef struct tagGET_MYSN_RESP_CMD
{
	_u32	_version;
	_u8		_cmd_type;
	_u8		_result;
	_u16	_max_sn;
	_u32	_mysn_array_num;
	_u32	_mysn_peerid_len;
	char		_mysn_peerid[PEER_ID_SIZE + 1];
	_u32	_mysn_ip;
	_u16	_mysn_port;
}GET_MYSN_RESP_CMD;

typedef struct tagPING_SN_CMD
{
	_u32	_version;
	_u8		_cmd_type;
	_u32	_local_peerid_len;
	char		_local_peerid[PEER_ID_SIZE + 1];
	_u32	_nat_type;
	_u16	_latest_port;
	_u32	_time_elapsed;
	_u32	_delta_port;
}PING_SN_CMD;

typedef struct tagPING_SN_RESP_CMD
{
	_u32	_version;
	_u8		_cmd_type;
	_u32	_sn_peerid_len;
	char		_sn_peerid[PEER_ID_SIZE + 1];
}PING_SN_RESP_CMD;

typedef struct tagSN2NN_LOGOUT_CMD
{
	_u32	_version;
	_u8		_cmd_type;
	_u32	_sn_peerid_len;
	char		_sn_peerid[PEER_ID_SIZE + 1];
}SN2NN_LOGOUT_CMD;

typedef struct tagNN2SN_LOGOUT_CMD
{
	_u32	_version;
	_u8		_cmd_type;
	_u32	_local_peerid_len;
	char		_local_peerid[PEER_ID_SIZE + 1];
}NN2SN_LOGOUT_CMD;

typedef struct tagSOMEONECALLYOU_CMD
{
	_u32	_version;
	_u8		_cmd_type;
	_u32	_source_peerid_len;
	char		_source_peerid[PEER_ID_SIZE + 1];
	_u32	_source_ip;
	_u16	_source_port;
	_u16	_virtual_port;
	_u32	_nat_type;
	_u16	_latest_remote_port;
	_u16	_guess_remote_port;	
	_u16	_udt_version;
	_u8		_mhxy_version;
}SOMEONECALLYOU_CMD;

typedef struct tagGET_PEERSN_RESP_CMD
{
	_u32	_version;
	_u8		_cmd_type;
	_u8		_result;
	_u32	_sn_num;
	_u32	_sn_peerid_len;
	char		_sn_peerid[PEER_ID_SIZE + 1];
	_u32	_sn_ip;
	_u16	_sn_port;
	_int32	_peerid_len;
	char		_peerid[PEER_ID_SIZE + 1];
	_int32	_nat_type;
}GET_PEERSN_RESP_CMD;

typedef struct tagPING_CMD
{
	_u32	_version;
	_u8		_cmd_type;
	_u32	_local_peerid_len;
	char		_local_peerid[PEER_ID_SIZE + 1];
	_u32	_internal_ip;
	_u32	_natwork_mask;
	_u16	_listen_port;
	_u32	_product_flag;
	_u32	_product_version;
	_u32	_sn_list_size;
	LIST		_sn_list;
	_u32	_network_type;
	_u32	_upnp_ip;
	_u16	_upnp_port;
	_u32	_online_time;
	_u32	_download_bytes;
	_u32	_upload_bytes;
	_u16	_upload_resource_number;
	_u8		_cur_uploading_number;
	_u8		_cur_download_task_number;
	_u8		_cur_uploading_connect_num;
	_u16	_cur_download_speed;
	_u16	_cur_upload_speed;
	_u16	_max_download_speed;
	_u16	_max_upload_speed;
	_u16	_auto_upload_speed_limit;
	_u16	_user_upload_speed_limit;
	_u16	_download_speed_limit;
	_u16	_peer_status_flag;
}PING_CMD;

typedef struct tagLOGOUT_CMD
{
	_u32	_version;
	_u8		_cmd_type;
	_u32	_ip;
	_u32	_local_peerid_len;
	char	_local_peerid[PEER_ID_SIZE + 1];
}LOGOUT_CMD;

typedef struct tagPUNCH_HOLE_CMD
{
	_u32	_version;
	_u8		_cmd_type;
	_u32	_peerid_len;
	char		_peerid[PEER_ID_SIZE + 1];		/*本客户端的PeerID*/
	_u16	_source_port;
	_u16	_target_port;
}PUNCH_HOLE_CMD;

typedef struct tagBROKER2_REQ_CMD
{
	_u32	_version;
	_u8		_cmd_type;
	_u32	_seq_num;
	_u32	_requestor_ip;
	_u16	_requestor_port;
	_u32	_remote_peerid_len;
	char		_remote_peerid[PEER_ID_SIZE + 1];
}BROKER2_REQ_CMD;

typedef struct tagBROKER2_CMD
{
	_u32	_version;
	_u8		_cmd_type;
	_u32	_seq_num;
	_u32	_ip;
	_u16	_tcp_port;
}BROKER2_CMD;

typedef struct tagUDP_BROKER_REQ_CMD
{
	_u32	_version;
	_u8		_cmd_type;
	_u32	_seq_num;
	_u32	_requestor_ip;
	_u16	_requestor_udp_port;
    	_u32	_remote_peerid_len;
	char		_remote_peerid[PEER_ID_SIZE + 1];
	_u32	_requestor_peerid_len;
	char		_requestor_peerid[PEER_ID_SIZE + 1];
}UDP_BROKER_REQ_CMD;

typedef struct tagUDP_BROKER_CMD
{
	_u32	_version;
	_u8		_cmd_type;
	_u32	_seq_num;
	_u32	_ip;
	_u16	_udp_port;
	_u32	_peerid_len;
	char		_peerid[PEER_ID_SIZE + 1];
}UDP_BROKER_CMD;

typedef struct tagTRANSFER_LAYER_CONTROL_CMD
{
	_u32	_version;
	_u32	_cmd_len;
	_u8		_cmd_type;
	_u32	_broker_seq;
	_u64	_header_hash;
}TRANSFER_LAYER_CONTROL_CMD;

typedef struct tagTRANSFER_LAYER_CONTROL_RESP_CMD
{
	_u32	_version;
	_u32	_cmd_len;
	_u8		_cmd_type;
	_u32	_result;		//	0 ：未知命令		1 ：broker成功		2 ：broker失败
}TRANSFER_LAYER_CONTROL_RESP_CMD;

typedef struct tagICALLSOMEONE_RESP_CMD
{
	_u32	_version;
	_u8		_cmd_type;
	_u32	_sn_peerid_len;
	char		_sn_peerid[PEER_ID_SIZE + 1];
	_u32	_remote_peerid_len;
	char		_remote_peerid[PEER_ID_SIZE + 1];
	_u8		_is_on_line;
	_u32	_remote_ip;
	_u16	_remote_port;		//本地字节序
	_u16	_virtual_port;		//请求方虚端口
	_u32	_nat_type;			//远方NAT类型
	_u16	_latest_ex_port;		//最近的公网端口
	_u16	_guess_ex_port;		//猜测的公网端口
}ICALLSOMEONE_RESP_CMD;

#ifdef __cplusplus
}
#endif
#endif

