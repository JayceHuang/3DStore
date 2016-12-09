#include "utility/map.h"
#include "utility/mempool.h"
#include "utility/time.h"
#include "utility/utility.h"
#include "utility/bytebuffer.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_P2P_TRANSFER_LAYER
#include "utility/logger.h"
#include "udt_device.h"
#include "udt_memory_slab.h"
#include "udt_cmd_define.h"
#include "udt_utility.h"
#include "udt_interface.h"
#include "../ptl_utility.h"
#include "../ptl_udp_device.h"
#include "../ptl_memory_slab.h"
#include "../ptl_callback_func.h"

static SET g_udt_device_set;

_int32 conn_id_comparator(void *E1, void *E2)
{
	CONN_ID* left = (CONN_ID*)E1;
	CONN_ID* right = (CONN_ID*)E2;
	if (left->_virtual_source_port != right->_virtual_source_port)
		return (_int32)(left->_virtual_source_port - right->_virtual_source_port);
	if (left->_virtual_target_port != right->_virtual_target_port)
		return (_int32)(left->_virtual_target_port - right->_virtual_target_port); 
	return (_int32)(left->_peerid_hashcode  - right->_peerid_hashcode);
}

_int32 udt_init_device_manager(void)
{
	set_init(&g_udt_device_set, conn_id_comparator);
	return SUCCESS; 
}

_int32 udt_uninit_device_manager(void)
{
    SET_ITERATOR next_iter = NULL;
    UDT_DEVICE* data = NULL;
    SET_ITERATOR cur_iter = SET_BEGIN(g_udt_device_set);
	while (cur_iter != SET_END(g_udt_device_set))
	{
		next_iter = SET_NEXT(g_udt_device_set, cur_iter);
		data = (UDT_DEVICE*)SET_DATA(cur_iter);
		udt_device_close(data);
//		set_erase_iterator(&g_udt_device_set, cur_iter);	这里不用再erase了，因为udt_device_close里面已经erase了
		cur_iter = next_iter;
	}
	return SUCCESS;
}

_int32 udt_add_device(UDT_DEVICE* udt)
{
	_int32 ret = set_insert_node(&g_udt_device_set, udt);
	CHECK_VALUE(ret);
	LOG_DEBUG("[udt = 0x%x, device = 0x%x]conn_id[%u, %u, %u]", udt, udt->_device, (_u32)udt->_id._virtual_source_port, (_u32)udt->_id._virtual_target_port, udt->_id._peerid_hashcode);
	return SUCCESS;
}

_int32 udt_remove_device(UDT_DEVICE* udt)
{
	LOG_DEBUG("[udt = 0x%x, device = 0x%x]conn_id[%u, %u, %u]", udt, udt->_device, (_u32)udt->_id._virtual_source_port, (_u32)udt->_id._virtual_target_port, udt->_id._peerid_hashcode);
	_int32 ret = set_erase_node(&g_udt_device_set, udt);
	CHECK_VALUE(ret);
	return ret;
}

UDT_DEVICE*	udt_find_device(const CONN_ID* id)
{
	UDT_DEVICE* udt = NULL;
	set_find_node(&g_udt_device_set, (void*)id, (void**)&udt);
	return udt;
}

