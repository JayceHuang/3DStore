#ifndef _SD_DNS_REACTOR_H_00138F8F2E70_200807231430
#define _SD_DNS_REACTOR_H_00138F8F2E70_200807231430

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#include "asyn_frame/msg.h"

_int32 init_dns_reactor(_int32 *waitable_handle);
_int32 uninit_dns_reactor(void);

_int32 register_dns_event(MSG *msg);
_int32 unregister_dns(MSG *msg, _int32 reason);

_int32 get_complete_dns_msg(MSG **msg);


#ifdef __cplusplus
}
#endif

#endif
