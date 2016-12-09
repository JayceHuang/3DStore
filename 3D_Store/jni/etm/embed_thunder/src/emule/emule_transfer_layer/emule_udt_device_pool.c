#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL
#include "emule_udt_device_pool.h"
#include "utility/map.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_EMULE
#include "utility/logger.h"

static	SET		g_emule_udt_device_set;

_int32 emule_conn_id_comparator(void *E1, void *E2)
{
	return sd_memcmp(E1, E2, CONN_ID_SIZE);
}

_int32 emule_udt_device_pool_init(void)
{
	set_init(&g_emule_udt_device_set, emule_conn_id_comparator);
	return SUCCESS;
}

_int32 emule_udt_device_pool_uninit(void)
{
	sd_assert(set_size(&g_emule_udt_device_set) == 0);
	return SUCCESS;
}

_int32 emule_udt_device_add(EMULE_UDT_DEVICE* udt_device)
{
	LOG_DEBUG("[emule_udt_device = 0x%x]emule_udt_device add to pool.", udt_device);
	return set_insert_node(&g_emule_udt_device_set, udt_device);
}

_int32 emule_udt_device_remove(EMULE_UDT_DEVICE* udt_device)
{
	LOG_DEBUG("[emule_udt_device = 0x%x]emule_udt_device remove from pool.", udt_device);
	return set_erase_node(&g_emule_udt_device_set, udt_device);
}

EMULE_UDT_DEVICE* emule_udt_device_find_by_conn_id(_u8 conn_id[CONN_ID_SIZE])
{
	EMULE_UDT_DEVICE* result = NULL;
	set_find_node(&g_emule_udt_device_set, conn_id, (void**)&result);
	return result;
}

//如果conn_num为0的话，那么表示仅根据ip和port来查找
EMULE_UDT_DEVICE* emule_udt_device_find(_u32 ip, _u16 port, _u32 conn_num)
{
	EMULE_UDT_DEVICE* udt_device = NULL;
	SET_ITERATOR iter = NULL;
	for(iter = SET_BEGIN(g_emule_udt_device_set); iter != SET_END(g_emule_udt_device_set); iter = SET_NEXT(g_emule_udt_device_set, iter))
	{
		udt_device = (EMULE_UDT_DEVICE*)SET_DATA(iter);
		if(udt_device->_ip == ip && udt_device->_udp_port == port)
		{
			if(conn_num == 0)
				return udt_device;
			else if(conn_num == udt_device->_conn_num)
				return udt_device;
		}
	}
	return NULL;
}

#endif /* ENABLE_EMULE */
#endif /* ENABLE_EMULE */

