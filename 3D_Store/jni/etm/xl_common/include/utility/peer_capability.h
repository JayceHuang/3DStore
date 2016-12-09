/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2008/08/14
-----------------------------------------------------------------------------------------------------------*/
#ifndef _PEER_CAPABILITY_H_
#define _PEER_CAPABILITY_H_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"

_u32	get_peer_capability(void);

/*
Peer所具备的功能标志，用于P2P传输层：
第0位：是否在内网
第1位：是否支持内网直连
第2位：是否与本地Peer位于相同局域网（用于资源查询返回）
第3位：是否支持边下边传
第4位：是否为发行server
第5位：是否支持P2P传输层
第6位：是否支持新udt（426版开始）
第7位：为UDT协议保留
*/
void set_peer_capability(_u32* peer_capability, BOOL nated, BOOL support_intranet , BOOL same_nat, BOOL support_new_p2p, BOOL is_cdn,
						 BOOL support_ptl, BOOL support_new_udt);

BOOL is_nated(_u32 peer_capability);

BOOL is_support_intranet(_u32 peer_capability);

BOOL is_same_nat(_u32 peer_capability);

BOOL is_support_new_p2p(_u32 peer_capability);

BOOL is_cdn(_u32 peer_capability);

BOOL is_support_ptl(_u32 peer_capability);

BOOL is_support_new_udt(_u32 peer_capability);

BOOL is_support_mhxy_version1(_u32 peer_capability);
#ifdef __cplusplus
}
#endif
#endif
