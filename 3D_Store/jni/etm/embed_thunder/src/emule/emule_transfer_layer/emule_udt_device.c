#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL
#include "emule_udt_device.h"
#include "emule_udt_device_pool.h"
#include "emule_socket_device.h"
#include "emule_udt_cmd.h"
#include "emule_udt_impl.h"
#include "../emule_utility/emule_memory_slab.h"
#include "asyn_frame/msg.h"
#include "utility/utility.h"
#include "utility/mempool.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_EMULE
#include "utility/logger.h"

#define		DEF_WIN_SIZE			(64*1024)

_int32 emule_udt_device_create(EMULE_UDT_DEVICE** udt_device)
{
	_int32 ret = SUCCESS;
	ret = emule_get_emule_udt_device_slip(udt_device);
	CHECK_VALUE(ret);
	LOG_DEBUG("[emule_udt_device = 0x%x]emule_udt_device_create.", *udt_device);
	sd_memset(*udt_device, 0, sizeof(EMULE_UDT_DEVICE));	
	emule_udt_device_add(*udt_device);
	return ret;
}

_int32 emule_udt_device_close(EMULE_UDT_DEVICE* udt_device)
{
	LOG_DEBUG("[emule_udt_device = 0x%x]emule_udt_device_close.", udt_device);
	emule_udt_device_remove(udt_device);
	if(udt_device->_transfer != NULL)
	{
		emule_transfer_close(udt_device->_transfer);
		udt_device->_transfer = NULL;
	}
	if(udt_device->_send_queue != NULL)
	{
		emule_udt_send_queue_close(udt_device->_send_queue);
		udt_device->_send_queue = NULL;
	}
	if(udt_device->_recv_queue != NULL)
	{
		emule_udt_recv_queue_close(udt_device->_recv_queue);
		udt_device->_recv_queue = NULL;
	}
	return emule_socket_device_close_callback(udt_device->_socket_device);
}

_int32 emule_udt_device_destroy(EMULE_UDT_DEVICE* udt_device)
{
	LOG_DEBUG("[emule_udt_device = 0x%x]emule_udt_device_destroy.", udt_device);
	return emule_free_emule_udt_device_slip(udt_device);
}

_int32 emule_udt_device_connect(EMULE_UDT_DEVICE* udt_device, _u32 ip, _u16 port, _u8 conn_id[USER_ID_SIZE])
{
	_int32 ret = SUCCESS;
	udt_device->_state = UDT_TRAVERSE;
	udt_device->_conn_num = sd_rand();
	sd_memcpy(udt_device->_conn_id, conn_id, USER_ID_SIZE);
	sd_assert(udt_device->_transfer == NULL);
	ret = emule_transfer_create(&udt_device->_transfer, TRANSFER_BY_SERVER, udt_device);
	CHECK_VALUE(ret);
	return emule_transfer_start(udt_device->_transfer);
}

_int32 emule_udt_device_notify_traverse(EMULE_UDT_DEVICE* udt_device, _int32 errcode)
{
	LOG_DEBUG("[emule_udt_device = 0x%x]emule_udt_device_notify_traverse, errcode = %d.", udt_device, errcode);
	emule_transfer_close(udt_device->_transfer);
	udt_device->_transfer = NULL;
	if(errcode != SUCCESS)
		return emule_socket_device_connect_callback(udt_device->_socket_device, errcode);
	sd_assert(FALSE);
	return SUCCESS;
}

_int32 emule_udt_device_send(EMULE_UDT_DEVICE* udt_device, char* buffer, _int32 len, BOOL is_data)
{
	return emule_udt_send_queue_add(udt_device->_send_queue, buffer, len, is_data);
}

_int32 emule_udt_device_recv(EMULE_UDT_DEVICE* udt_device, char* buffer, _int32 len, BOOL is_data)
{
	sd_assert(udt_device->_recv_queue->_buffer == NULL && udt_device->_recv_queue->_len == 0 && udt_device->_recv_queue->_recv_msg_id == 0);
	udt_device->_recv_queue->_buffer = buffer;
	udt_device->_recv_queue->_len = len;
	udt_device->_recv_queue->_offset = 0;
	udt_device->_recv_queue->_is_recv_data = is_data;
	return emule_udt_read_data(udt_device);
}

#endif /* ENABLE_EMULE */
#endif /* ENABLE_EMULE */

