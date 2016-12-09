/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2008/10/23
-----------------------------------------------------------------------------------------------------------*/
#ifndef _P2P_SOCKET_DEVICE_H_
#define _P2P_SOCKET_DEVICE_H_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "p2p_transfer_layer/ptl_interface.h"
#include "p2p_data_pipe/p2p_sending_queue.h"
#include "p2p_data_pipe/p2p_data_pipe.h"
#include "platform/sd_socket.h"
#include "utility/speed_calculator.h"

struct tagP2P_DATA_PIPE;

typedef struct tagSOCKET_DEVICE
{
	char*				_cmd_buffer;			
	_u32				_cmd_buffer_len;		//如果该值等于RECV_CMD_BUFFER_LEN,说明_cmd_buffer是slab得到的；否则，则是malloc得到的，在释放_cmd_buffer时，需注意类型。
	_u32				_cmd_buffer_offset;
	/*data buffer allocate from data manager, this data is corporation with first element of pipe->_request_range_list LIST*/
	BOOL				_is_valid_data;			//收到的是否有效的数据
	char*				_data_buffer;			/*data buffer which is allocate by dm_get_data_buffer()*/
	_u32				_data_buffer_len;		/*length of _data_buffer, this length is equal to the first request range size in pipe->_request_list LIST*/
	_u32				_data_buffer_offset;	/*have recv data*/
	_u32				_request_data_len;		/*p2p request data length, must less or equal than _data_buffer_len*/
	P2P_SENDING_QUEUE*	_p2p_sending_queue;
	P2P_SENDING_CMD*	_p2p_sending_cmd;		
	SPEED_CALCULATOR	_speed_calculator;		/*calculate pipe speed*/
	_u32				_get_data_buffer_timeout_id;
	_u32				_keepalive_timeout_id;
      _u8                               _mhxy_read_head;
}SOCKET_DEVICE;

_int32 p2p_create_socket_device(P2P_DATA_PIPE* p2p_pipe);

_int32 p2p_close_socket_device(P2P_DATA_PIPE* p2p_pipe);

_int32 p2p_destroy_socket_device(P2P_DATA_PIPE* p2p_pipe);

_int32 p2p_socket_device_connect(P2P_DATA_PIPE* p2p_pipe, const P2P_RESOURCE* res);

_int32 p2p_socket_device_send(P2P_DATA_PIPE* p2p_pipe, _u8 command_type, char* cmd_buffer, _u32 len);

_int32 p2p_socket_device_recv_cmd(P2P_DATA_PIPE* p2p_pipe, _u32 cmd_len);

_int32 p2p_socket_device_recv_data(P2P_DATA_PIPE* p2p_pipe, _u32 data_len);

_int32 p2p_socket_device_recv_discard_data(P2P_DATA_PIPE* p2p_pipe, _u32 discard_len);

_int32 p2p_socket_device_free_data_buffer(P2P_DATA_PIPE* p2p_pipe);

//以下部分是回调函数，由ptl层回调回来
_int32 p2p_socket_device_connect_callback(_int32 errcode, PTL_DEVICE* device);

_int32 p2p_socket_device_send_callback(_int32 errcode, PTL_DEVICE* device, _u32 had_send);

_int32 p2p_socket_device_recv_cmd_callback(_int32 errcode, PTL_DEVICE* device, _u32 had_recv);

_int32 p2p_socket_device_recv_data_callback(_int32 errcode, PTL_DEVICE* device, _u32 had_recv);

_int32 p2p_socket_device_recv_diacard_data_callback(_int32 errcode, PTL_DEVICE* device, _u32 had_recv);

_int32 p2p_notify_close_socket_device(PTL_DEVICE* device);		/*由ptl层回调通知可以关闭SOCKET_DEVICE*/

_int32 get_data_buffer_timeout_handler(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid);
#ifdef __cplusplus
}
#endif
#endif

