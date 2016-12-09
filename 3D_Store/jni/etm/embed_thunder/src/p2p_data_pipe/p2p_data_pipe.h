/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2008/07/02
-----------------------------------------------------------------------------------------------------------*/
#ifndef _P2P_DATA_PIPE_
#define _P2P_DATA_PIPE_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "connect_manager/resource.h"
#include "connect_manager/data_pipe.h"
#include "utility/speed_calculator.h"

struct tagSOCKET_DEVICE;
struct _tag_range_data_buffer;
struct tagPTL_DEVICE;

typedef struct tagP2P_RESOURCE
{
	RESOURCE				_res_pub;
	_u8						_gcid[CID_SIZE];
	_u64					_file_size;
	_u32					_ip;
	_u32					_tcp_port;
	_u32					_udp_port;	
	char					_peer_id[PEER_ID_SIZE + 1];
	_u32						_peer_capability; 
	_u8						_res_level;
	_u8                     _from;
	_u8                     _res_priority;
}P2P_RESOURCE;

#ifdef UPLOAD_ENABLE
typedef struct tagP2P_UPLOAD_DATA
{
	_u64	_data_pos;
	_u32	_data_len;
	char*	_buffer;
	_u32	_buffer_offset;
	_u32	_buffer_len;
}P2P_UPLOAD_DATA;
#endif

typedef struct tagP2P_DATA_PIPE
{	
	DATA_PIPE					_data_pipe;
	struct tagSOCKET_DEVICE*		_socket_device;
	struct tagPTL_DEVICE*			_device;
	/*p2p property*/
	_u32				_handshake_connect_id;		/*must greater 0x80000000*/
	BOOL				_remote_not_in_nat;
	_u32				_remote_protocol_ver;		/*remote peer's protocol version*/
	_u32					_remote_peer_capability;
	/*request helper*/
	BOOL				_cancel_flag;				/*if true, all request_response command are discard*/
	BOOL				_is_ever_unchoke;			//曾经被unchoke过
	LIST					_request_data_queue;		/*请求数据队列*/

#ifdef UPLOAD_ENABLE
	LIST					_upload_data_queue;			/*上传数据队列*/
	struct _tag_range_data_buffer* _range_data_buffer;			
	P2P_UPLOAD_DATA*	_cur_upload;				/*当前正在处理的上传*/
	_u32				_get_upload_buffer_timeout_id;	
	SPEED_CALCULATOR	_upload_speed;				/*当前的上传速度*/
#endif
	BOOL _last_cmd_is_handshake;
}P2P_DATA_PIPE;
#ifdef __cplusplus
}
#endif
#endif

