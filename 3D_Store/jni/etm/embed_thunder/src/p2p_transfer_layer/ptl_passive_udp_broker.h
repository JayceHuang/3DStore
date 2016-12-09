/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/07/31
-----------------------------------------------------------------------------------------------------------*/
#ifndef _PTL_PASSIVE_UDP_BROKER_H_
#define _PTL_PASSIVE_UDP_BROKER_H_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"

typedef struct tagPASSIVE_UDP_BROKER_DATA
{
	_u32	_seq;
	_u32	_ip;
	_u16	_udp_port;
	char		_remote_peerid[PEER_ID_SIZE + 1];
	_u32  _born_time;
}PASSIVE_UDP_BROKER_DATA;

struct tagUDP_BROKER_CMD;

_int32 ptl_passive_udp_broker_data_comparator(void *E1, void *E2);

_int32 ptl_init_passive_udp_broker(void);

_int32 ptl_uninit_passive_udp_broker(void);

_int32 ptl_passive_udp_broker(struct tagUDP_BROKER_CMD* cmd);

void ptl_delete_passive_udp_broker_data(_u32 peerid_hashcode, _u32 broker_seq);

_int32 ptl_erase_passive_udp_broker_data(PASSIVE_UDP_BROKER_DATA* data);
#ifdef __cplusplus
}
#endif
#endif

