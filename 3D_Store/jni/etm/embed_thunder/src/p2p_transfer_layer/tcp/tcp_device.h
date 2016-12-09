/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2008/11/6
-----------------------------------------------------------------------------------------------------------*/
#ifndef _TCP_DEVICE_H_
#define	_TCP_DEVICE_H_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "platform/sd_socket.h"
#include "p2p_transfer_layer/mhxy/mhxy_tcp.h"

struct tagPTL_DEVICE;

typedef struct tagTCP_DEVICE
{
	struct tagPTL_DEVICE*	_device;
	SOCKET				_socket;
#ifdef _SUPPORT_MHXY
	MHXY_TCP * _mhxy;
#endif
}TCP_DEVICE;
#ifdef __cplusplus
}
#endif
#endif

