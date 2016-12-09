#include "udt_utility.h"
#include "udt_cmd_define.h"
#include "utility/utility.h"
#include "utility/peerid.h"
#include "utility/time.h"
#include "utility/string.h"
#include "utility/sd_assert.h"

#define MTU_SIZE (1440)
#define SEQUENCE_NUM_DELTA (64000)

static _u16 g_virtual_source_port = 0;
static _u32 g_peerid_hashcode = 0;
static _u32 g_seq_num = 0;

void udt_init_utility()
{
	char peerid[PEER_ID_SIZE + 1] = {0};
	_u32 time;
	sd_time(&time);
	sd_srand(time);
	g_virtual_source_port = (_u16)sd_rand();
	if (get_peerid(peerid, PEER_ID_SIZE + 1) == SUCCESS)
		g_peerid_hashcode = udt_hash_peerid(peerid);
	g_seq_num = (_u32)sd_rand();
}

_u32 udt_generate_source_port()
{
	return g_virtual_source_port++;
}

_u32 udt_local_peerid_hashcode()
{
	char	peerid[PEER_ID_SIZE + 1];
	if(g_peerid_hashcode == 0)
	{
		if(get_peerid(peerid, PEER_ID_SIZE + 1) == SUCCESS)
			g_peerid_hashcode = udt_hash_peerid(peerid);
	}	
	return g_peerid_hashcode;
}

_u32 udt_hash_peerid(const char* peerid)
{
	_u32 h = 0, i = 0, g = 0;
	//sd_assert(sd_strlen(peerid) == PEER_ID_SIZE);
	//为了减少开销,强制性哈希16位
	for(i = 0; *peerid && i < PEER_ID_SIZE; ++i)
	{
		h = (h << 4) + *peerid++;
		g = h & 0xF0000000L;
		if(g)
		{
			h ^= g >> 24;
		}
		h &= ~g;
	}
	return h;
}

_u32 udt_get_mtu_size()
{
	return MTU_SIZE;
}

_u32 udt_get_seq_num()
{
	g_seq_num += SEQUENCE_NUM_DELTA;
	return g_seq_num;
}
