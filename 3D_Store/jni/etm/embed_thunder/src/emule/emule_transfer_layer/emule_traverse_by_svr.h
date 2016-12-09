/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/12/14
-----------------------------------------------------------------------------------------------------------*/
#ifndef _EMULE_TRAVERSE_BY_SVR_H_
#define _EMULE_TRAVERSE_BY_SVR_H_
#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL

#include "asyn_frame/msg.h"

typedef enum tagEMULE_NAT_STATE
{
	NAT_IDLE_STATE = 0, NAT_SYNC_STATE, NAT_PING_STATE, NAT_TIMEOUT_STATE, NAT_NOPEER_STATE
}EMULE_NAT_STATE;

typedef struct tagEMULE_TRANSFER_BY_SERVER
{
	EMULE_NAT_STATE		_state;
	void*				_user_data;
	_u32				_sync2_count;
	_u32				_last_sync2_time;
	_u32				_next_sync2_interval;
	_u32				_ping_count;
	_u32				_last_ping_time;
	_u32				_timeout_id;
	_u32				_client_ip;
	_u16				_client_port;
}EMULE_TRANSFER_BY_SERVER;

_int32 emule_traverse_by_svr_create(EMULE_TRANSFER_BY_SERVER** traverse, void* user_data);

_int32 emule_traverse_by_svr_close(EMULE_TRANSFER_BY_SERVER* traverse);

_int32 emule_traverse_by_svr_start(EMULE_TRANSFER_BY_SERVER* traverse);

_int32 emule_traverse_by_svr_send_ping(EMULE_TRANSFER_BY_SERVER* traverse);

_int32 emule_traverse_by_svr_send_sync2(EMULE_TRANSFER_BY_SERVER* traverse);

_int32 emule_traverse_by_svr_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid);

_int32 emule_traverse_by_svr_failed(EMULE_TRANSFER_BY_SERVER* traverse);

_int32 emule_traverse_by_svr_recv(EMULE_TRANSFER_BY_SERVER* traverse, _u8 opcode, _u32 ip, _u16 port);
#endif
#endif /* ENABLE_EMULE */
#endif

