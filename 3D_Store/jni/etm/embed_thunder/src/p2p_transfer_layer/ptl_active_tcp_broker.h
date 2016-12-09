/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/07/30
-----------------------------------------------------------------------------------------------------------*/
#ifndef _PTL_ACTIVE_TCP_BROKER_H_
#define _PTL_ACTIVE_TCP_BROKER_H_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "asyn_frame/msg.h"
#include "utility/define.h"

struct tagPTL_DEVICE;

typedef struct tagACTIVE_TCP_BROKER_DATA
{
	struct tagPTL_DEVICE*	_device;
	_u32				_seq_num;
	_u32				_timeout_id;
	_u32 				_retry_time;
	_u32				_sn_ip;
	_u16				_sn_port;
	char					_remote_peerid[PEER_ID_SIZE + 1];
}ACTIVE_TCP_BROKER_DATA;

_int32 ptl_active_tcp_broker_data_comparator(void *E1, void *E2);

_int32 ptl_init_active_tcp_broker(void);

_int32 ptl_uninit_active_tcp_broker(void);

_int32 ptl_active_tcp_broker(struct tagPTL_DEVICE* device, const char* remote_peerid);

_int32 ptl_active_tcp_broker_get_peersn_callback(_int32 errcode, _u32 sn_ip, _u16 sn_port, void* user_data);

_int32 ptl_handle_active_tcp_broker_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid);

ACTIVE_TCP_BROKER_DATA* ptl_find_active_tcp_broker_data(_u32 seq_num);

_int32 ptl_erase_active_tcp_broker_data(ACTIVE_TCP_BROKER_DATA* data);

_int32 ptl_cancel_active_tcp_broker_req(struct tagPTL_DEVICE* device);
#ifdef __cplusplus
}
#endif
#endif

