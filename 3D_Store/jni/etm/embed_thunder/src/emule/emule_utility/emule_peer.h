/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/10/13
-----------------------------------------------------------------------------------------------------------*/
#ifndef _EMULE_PEER_H_
#define _EMULE_PEER_H_

#include "utility/define.h"
#ifdef ENABLE_EMULE
#include "emule/emule_utility/emule_tag_list.h"
#include "emule/emule_utility/emule_define.h"

typedef struct tagEMULE_PEER
{
	_u32	_client_id;
	_u8		_user_id[USER_ID_SIZE];
	_u16	_tcp_port;
	_u16	_udp_port;
	_u32	_server_ip;
	_u16	_server_port;
	EMULE_TAG_LIST	_tag_list;
}EMULE_PEER;

void emule_peer_init(EMULE_PEER* peer);

void emule_peer_uninit(EMULE_PEER* peer);

_u8 emule_peer_get_extended_requests_option(EMULE_PEER* peer);

_u8 emule_peer_get_source_exchange_option(EMULE_PEER* peer);

#endif /* ENABLE_EMULE */
#endif

