#include "utility/peer_capability.h"
#include "utility/sd_assert.h"
#include "utility/local_ip.h"

_u32 g_peer_capability = 0;

_u32	get_peer_capability(void)
{
	_u32 local_ip;
	if(g_peer_capability == 0)
	{
		local_ip = sd_get_local_ip();
		set_peer_capability(&g_peer_capability, sd_is_in_nat(local_ip), TRUE, FALSE, TRUE, FALSE, TRUE, TRUE);
	}
	return g_peer_capability;	
}

void set_peer_capability(_u32* peer_capability, BOOL nated, BOOL support_intranet , BOOL same_nat, BOOL support_new_p2p, BOOL is_cdn,
						 BOOL support_ptl, BOOL support_new_udt)
{
	*peer_capability = 0;
	if(nated)
		*peer_capability = (*peer_capability) | 0x01;
	if(support_intranet)
		*peer_capability = (*peer_capability) | 0x02;
	if(same_nat)
		*peer_capability = (*peer_capability) | 0x04;
	if(support_new_p2p)
		*peer_capability = (*peer_capability) | 0x08;
	if(is_cdn)
		*peer_capability = (*peer_capability) | 0x10;
	if(support_ptl)
		*peer_capability = (*peer_capability) | 0x20;
	if(support_new_udt)
		*peer_capability = (*peer_capability) | 0x40;
}

BOOL is_nated(_u32 peer_capability)
{
	return (0x01 & peer_capability) != 0;
}

BOOL is_support_intranet(_u32 peer_capability)
{
	return (0x02 & peer_capability) != 0;
}

BOOL is_same_nat(_u32 peer_capability)
{
	return (0x04 & peer_capability) != 0;
}

BOOL is_support_new_p2p(_u32 peer_capability)
{
	return (0x08 & peer_capability) != 0;
}
	
BOOL is_cdn(_u32 peer_capability)
{
	return (0x10 & peer_capability) != 0;
}

BOOL is_support_ptl(_u32 peer_capability)
{
	return (0x20 & peer_capability) != 0;
}

BOOL is_support_new_udt(_u32 peer_capability)
{
	return (0x40 & peer_capability) != 0;
}

BOOL is_support_mhxy_version1(_u32 peer_capability)
{
	return (0x0800 & peer_capability) != 0;
}
