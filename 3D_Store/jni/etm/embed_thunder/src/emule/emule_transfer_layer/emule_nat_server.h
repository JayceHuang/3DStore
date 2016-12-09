/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/12/14
-----------------------------------------------------------------------------------------------------------*/
#ifndef	_EMULE_NAT_SERVER_H_
#define	_EMULE_NAT_SERVER_H_

#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL
#include "asyn_frame/msg.h"

typedef struct tagEMULE_NAT_SERVER
{
	_u32	_register_time;
	_u32	_keepalive_time;
	BOOL	_is_register;
	_u32	_timeout_id;
	_u32	_ip;
	_u16	_port;
}EMULE_NAT_SERVER;

_int32 emule_nat_server_init(void);

_int32 emule_nat_server_register(void);

BOOL emule_nat_server_enable(void);

_int32 emule_nat_server_notify_register(_u32 ip, _u16 port);

_int32 emule_nat_server_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid);

_int32 emule_nat_server_send(const char* buffer, _int32 len);

#endif
#endif /* ENABLE_EMULE */

#endif

