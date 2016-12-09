#ifndef _PTL_INTERFACE_H_
#define _PTL_INTERFACE_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include "asyn_frame/msg.h"
#include "utility/define.h"


typedef enum tagPTL_DEVICE_TYPE
{
    UNKNOWN_TYPE = 0,
    TCP_TYPE,
    UDT_TYPE
}PTL_DEVICE_TYPE;

//奇数都是主动连接，偶数都是被动连接
typedef enum tagPTL_CONNECT_TYPE
{
	INVALID_CONN = 0, SAME_NAT_CONN,
	PASSIVE_TCP_CONN, ACTIVE_TCP_CONN,
	PASSIVE_UDT_CONN, ACTIVE_UDT_CONN,
	PASSIVE_PUNCH_HOLE_CONN,	ACTIVE_PUNCH_HOLE_CONN,
	PASSIVE_UDP_BROKER_CONN, ACTIVE_UDP_BROKER_CONN,
	PASSIVE_TCP_BROKER_CONN,	ACTIVE_TCP_BROKER_CONN
}PTL_CONNECT_TYPE;

typedef struct tagPTL_DEVICE
{
	PTL_DEVICE_TYPE _type;
	void* _device;
	void* _user_data;
	void** _table;
	PTL_CONNECT_TYPE _connect_type;
}PTL_DEVICE;

_int32 init_ptl_modular(void);
_int32 uninit_ptl_modular(void);

_int32 ptl_exit(void);		//用于在反初始化之前做一些退出操作

_int32 ptl_create_device(PTL_DEVICE** device, void* user_data, void** table);
_int32 ptl_close_device(PTL_DEVICE* device);
_int32 ptl_destroy_device(PTL_DEVICE* device);

_int32 force_close_ptl_modular_res(void);

//该函数供p2p层调用，用于分配合适长度的命令缓冲区
_int32 ptl_malloc_cmd_buffer(char** cmd_buffer, _u32* len, _u32* offset, PTL_DEVICE* device_base);
_int32 ptl_free_cmd_buffer(char* cmd_buffer);

PTL_CONNECT_TYPE ptl_get_connect_type(_u32 peer_capability);

_int32 ptl_connect(PTL_DEVICE* device, _u32 peer_capability, const _u8* gcid, _u64 file_size, const char* peerid, _u32 ip, _u16 tcp_port, _u16 udp_port);

_int32 ptl_recv_cmd(PTL_DEVICE* device, char* buffer, _u32 expect_len, _u32 buffer_len);
_int32 ptl_recv_data(PTL_DEVICE* device, char* buffer, _u32 expect_len, _u32 buffer_len);

_int32 ptl_recv_discard_data(PTL_DEVICE* device, char* buffer, _u32 expect_len, _u32 buffer_len);

_int32 ptl_send(PTL_DEVICE* device, char* buffer, _u32 len);
_int32 ptl_send_in_speed_limit(PTL_DEVICE* device, char* buffer, _u32 len);

_int32 ptl_handle_ping_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 expired, _u32 msgid);

_int32 ptl_start_ping();
_int32 ptl_stop_ping();

_int32 ptl_ping_sn_notask_hook( const _int32 down_task);
#ifdef __cplusplus
}
#endif
#endif

