/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/07/31
-----------------------------------------------------------------------------------------------------------*/
#ifndef _PTL_PASSIVE_TCP_BROKER_H_
#define _PTL_PASSIVE_TCP_BROKER_H_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "platform/sd_socket.h"
#include "utility/define.h"

#define	PASSIVE_TCP_BROKER_BUFFER_LEN		13

typedef struct tagPASSIVE_TCP_BROKER_DATA
{
	SOCKET		_sock;
	_u32		_broker_seq;
	_u32		_ip;
	_u16		_port;
	char			_buffer[PASSIVE_TCP_BROKER_BUFFER_LEN];
}PASSIVE_TCP_BROKER_DATA;

struct tagBROKER2_CMD;

_int32 ptl_passive_tcp_broker_comparator(void *E1, void *E2);

_int32 ptl_init_passive_tcp_broker(void);

_int32 ptl_uninit_passive_tcp_broker(void);

_int32 ptl_passive_tcp_broker(struct tagBROKER2_CMD* cmd);

_int32 ptl_passive_tcp_broker_connect_callback(_int32 errcode, _u32 pending_op_count, void* user_data);

_int32 ptl_passive_tcp_broker_send_callback(_int32 errcode, _u32 pending_op_count, const char* buffer, _u32 had_send, void* user_data);

_int32 ptl_passive_tcp_broker_recv_callback(_int32 errcode, _u32 pending_op_count, char* buffer, _u32 had_recv, void* user_data);

_int32 ptl_erase_passive_tcp_broker_data(PASSIVE_TCP_BROKER_DATA* data);
#ifdef __cplusplus
}
#endif
#endif

