/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2008/11/4
-----------------------------------------------------------------------------------------------------------*/
#ifndef	_UDT_UTILITY_H_
#define	_UDT_UTILITY_H_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "p2p_transfer_layer/udt/udt_device.h"

void udt_init_utility(void);

_u32 udt_generate_source_port(void);

_u32 udt_local_peerid_hashcode(void);

_u32 udt_hash_peerid(const char* peerid);

_u32 udt_get_mtu_size(void);

_u32 udt_get_seq_num(void);

_u32 udt_get_seq_num();
#ifdef __cplusplus
}
#endif
#endif

