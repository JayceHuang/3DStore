#include "utility/define.h"
#ifdef ENABLE_BT 
#include "bt_utility.h"
#include "bt_define.h"
#include "utility/bytebuffer.h"
#include "utility/errcode.h"
#include "utility/sd_assert.h"
#include "utility/sha1.h"
#include "utility/string.h"
#include "utility/time.h"
#include "utility/utility.h"

static char	g_bt_peerid[BT_PEERID_LEN] = {0};

_int32 bt_escape_string(const char* src, _int32 src_len, char* dest, _int32* dest_len)
{
	// http://www.ietf.org/rfc/rfc2396.txt    section 2.3
	// some trackers seems to require that ' is escaped
	_int32 ret = SUCCESS;
	char unreserved_chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	_u32 index = 0;
	_int32 i = 0, len = *dest_len;
	for(i = 0; i < src_len; ++i)
	{
		if((src[i] >= 'A' && src[i] <= 'Z') || (src[i] >= 'a' && src[i] <= 'z') || (src[i] >= '0' && src[i] <= '9'))
		{
			ret = sd_set_int8(&dest, &len, src[i]);
		}
		else
		{
			sd_set_int8(&dest, &len, '%');
			index = ((_u8)src[i]) >> 4;		
			sd_set_int8(&dest, &len, unreserved_chars[index]);
			index = src[i] & 0x0F;
			ret = sd_set_int8(&dest, &len, unreserved_chars[index]);
		}
	}
	*dest_len = *dest_len - len;
	return ret;
}

_int32 bt_get_local_peerid(char* bt_peerid, _u32 bt_peerid_len)
{
	_u32 times = 0;
	ctx_sha1 sha1;
	_u8 result[20];
	if(bt_peerid_len < BT_PEERID_LEN)
		return -1;
	if(sd_strlen(g_bt_peerid) == BT_PEERID_LEN)
		return sd_memcpy(bt_peerid, g_bt_peerid, BT_PEERID_LEN);
	/*生成一个bt_peerid*/
	sha1_initialize(&sha1);
	sd_time(&times);
	sd_srand(times);
	sd_snprintf((char*)result, 20, "%s%u%d", BT_PEERID_HEADER, times, sd_rand());		/*这里省略了一些参数*/
	sha1_update(&sha1, result, 20);
	sha1_finish(&sha1, result);
	sd_memcpy(result, BT_PEERID_HEADER, sd_strlen(BT_PEERID_HEADER));
	sd_memcpy(g_bt_peerid, result, BT_PEERID_LEN);
	return sd_memcpy(bt_peerid, g_bt_peerid, BT_PEERID_LEN);
}

BOOL bt_is_bitcomet_peer(char* peerid)
{
	if(sd_strncmp(peerid, "-BC", 3) == 0 || sd_strncmp(peerid, "exbc", 4) == 0)
		return TRUE;
	else
		return FALSE;
}

BOOL bt_is_UT_peer(char* peerid)
{
	if(sd_strncmp(peerid, "-UT", 3) == 0)
		return TRUE;
	else
		return FALSE;
}

void bt_get_peer_hash_value(_u32 ip, _u16 port, _u8* info_hash, _u32* result)
{
	char* buffer;
	_u32 hash = 0, x = 0, i = 0;
	for(i = 0; i < BT_INFO_HASH_LEN; ++i)
	{
		hash = (hash << 4) + info_hash[i];
		if((x = hash & 0xF0000000L) != 0)
		{
			hash ^= (x >> 24);
			hash &= ~x;
		} 
	} 
	buffer = (char*)&ip;
	for(i = 1; i < 7; ++i)
	{
		hash = (hash << 4) + (*buffer);
		if((x = hash & 0xF0000000L) != 0)
		{
			hash ^= (x >> 24);
			hash &= ~x;
		}
		++buffer;
		if(i % 5 == 0)
			buffer = (char*)&port;
	}
	*result = hash & 0x7FFFFFFF;
}
#endif

