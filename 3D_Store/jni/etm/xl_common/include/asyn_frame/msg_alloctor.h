#ifndef _SD_MSG_ALLOCTOR_H_00138F8F2E70_200806252159
#define _SD_MSG_ALLOCTOR_H_00138F8F2E70_200806252159

#ifdef __cplusplus
extern "C" 
{
#endif

#include "asyn_frame/device.h"

_int32 msg_alloctor_init(void);
_int32 msg_alloctor_uninit(void);

_int32 msg_alloc(MSG **msg);
_int32 msg_dealloc(MSG *msg);


/* thread msg */
_int32 msg_thread_alloc(THREAD_MSG **ppmsg);
_int32 msg_thread_dealloc(THREAD_MSG *pmsg);

/* OP-Parameter */

_int32 para_dns_alloc(void **para_dns);
_int32 para_dns_dealloc(void *para_dns);

_int32 para_accept_alloc(void **para_accept);
_int32 para_accept_dealloc(void *para_accept);

_int32 para_connsock_rw_alloc(void **para_cs_rw);
_int32 para_connsock_rw_dealloc(void *para_cs_rw);

_int32 para_noconnsock_rw_alloc(void **para_ncs_rw);
_int32 para_noconnsock_rw_dealloc(void *para_ncs_rw);

_int32 para_fsopen_alloc(void **para_fsopen);
_int32 para_fsopen_dealloc(void *para_fsopen);

_int32 para_fsrw_alloc(void **para_fsrw);
_int32 para_fsrw_dealloc(void *para_fsrw);

_int32 para_sockaddr_alloc(void **para_sockaddr);
_int32 para_sockaddr_dealloc(void *para_sockaddr);

_int32 para_calc_hash_alloc(void **para_calc_hash);
_int32 para_calc_hash_dealloc(void *para_calc_hash);

#ifdef __cplusplus
}
#endif

#endif
