/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/11/4
-----------------------------------------------------------------------------------------------------------*/
#ifndef _EMULE_UDP_DEVICE_H_
#define _EMULE_UDP_DEVICE_H_

#include "utility/define.h"
#ifdef ENABLE_EMULE

struct tagSD_SOCKADDR;

_int32 emule_create_udp_device(void);

_int32 emule_close_udp_device(void);

_int32 emule_force_close_udp_socket(void);

_int32 emule_udp_sendto(const char* buffer, _u32 len, _u32 ip, _u16 port, BOOL need_free);
_int32 emule_udp_sendto_in_speed_limit(const char* buffer
        , _u32 len, _u32 ip, _u16 port, BOOL need_free);


_int32 emule_udp_sendto_by_domain(const char* buffer, _u32 len, const char* host, _u16 port, BOOL need_free);

_int32 emule_udp_sendto_callback(_int32 errcode, _u32 pending_op_count, const char* buffer, _u32 had_send, void* user_data);

_int32 emule_udp_recvfrom(void);

_int32 emule_udp_recvfrom_callback(_int32 errcode, _u32 pending_op_count, char* buffer, _u32 had_recv, struct tagSD_SOCKADDR* from, void* user_data);
#endif /* ENABLE_EMULE */

#endif


