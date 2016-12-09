/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/10/28
-----------------------------------------------------------------------------------------------------------*/
#ifndef _EMULE_SERVER_H_
#define _EMULE_SERVER_H_

#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL

#define	SRV_UDPFLG_EXT_GETSOURCES2	0x00000020

#define	MAX_SERVER_NAME_SIZE			32

typedef struct tagEMULE_SERVER
{
	_u32	_ip;
	_u16	_port;
	_u32	_udp_flags;		//获得与服务器的UDP通信支持参数
	_u32	_tcp_flags;
	_u16	_obf_tcp_port;
	char		_server_name[MAX_SERVER_NAME_SIZE + 1];
}EMULE_SERVER;

#endif
#endif /* ENABLE_EMULE */
#endif


