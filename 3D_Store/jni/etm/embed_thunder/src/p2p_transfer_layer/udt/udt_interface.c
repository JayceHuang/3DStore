#include "udt_interface.h"
#include "udt_device_manager.h"
#include "udt_memory_slab.h"
#include "udt_utility.h"
#include "udt_impl.h"
#include "udt_cmd_define.h"
#include "../ptl_callback_func.h"
#include "../ptl_udp_device.h"
#include "../ptl_memory_slab.h"
#include "asyn_frame/device.h"
#include "utility/time.h"
#include "utility/mempool.h"
#include "utility/utility.h"
#include "utility/bytebuffer.h"
#include "platform/sd_network.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_P2P_TRANSFER_LAYER
#include "utility/logger.h"

_int32	init_udt_modular()
{
	_int32 ret;
	ret = udt_init_memory_slab();
	CHECK_VALUE(ret);
	udt_init_utility();
	ret = udt_init_device_manager();
	CHECK_VALUE(ret);
	return udt_init_global_bitmap();
}

_int32	uninit_udt_modular()
{
    udt_uninit_global_bitmap();
    udt_uninit_device_manager();
    return udt_uninit_memory_slab();
}

_int32 udt_device_create(UDT_DEVICE** udt, _u32 source_port, _u32 target_port, _u32 peerid_hashcode, PTL_DEVICE* device)
{
#ifdef MOBILE_PHONE
	if (NT_GPRS_WAP & sd_get_net_type())
	{
		return -1;
	}
#endif

	_int32 ret = udt_malloc_udt_device(udt);
	CHECK_VALUE(ret);
	sd_memset(*udt, 0, sizeof(UDT_DEVICE));
	(*udt)->_id._virtual_source_port = source_port;
	(*udt)->_id._virtual_target_port = target_port;
	(*udt)->_id._peerid_hashcode = peerid_hashcode;
	(*udt)->_device = device;
	(*udt)->_state = INIT_STATE;
	(*udt)->_send_window = 0;
	(*udt)->_recv_window = DEFAULT_SEND_RECV_SPACE;
	(*udt)->_next_send_seq = udt_get_seq_num();;
	(*udt)->_next_recv_seq = 0;
	(*udt)->_timeout_id = INVALID_MSG_ID;
	LOG_DEBUG("[udt = 0x%x, device = 0x%x]create udt device, conn_id[%u, %u, %u].", *udt,  (*udt)->_device, source_port, target_port, peerid_hashcode);
	return ret;
}

_int32 udt_recv_buffer_comparator(void* E1, void* E2)
{
    UDT_RECV_BUFFER* left = (UDT_RECV_BUFFER*)E1;
    UDT_RECV_BUFFER* right = (UDT_RECV_BUFFER*)E2;
    return left->_seq - right->_seq;
}

_int32 udt_device_close(UDT_DEVICE* udt)
{
	SET_ITERATOR iter;
	UDT_SEND_BUFFER* send_buffer = NULL;
	UDT_RECV_BUFFER* recv_buffer = NULL;
	LOG_DEBUG("[udt = 0x%x, device = 0x%x]close udt device. conn_id = %u, udt_state = %u", udt, udt->_device, (_u32)udt->_id._virtual_source_port, udt->_state);
	udt_change_state(udt, CLOSE_STATE);
	udt_remove_device(udt);					//把相关的udt记录去掉
	if (udt->_timeout_id != INVALID_MSG_ID)
	{
		cancel_timer(udt->_timeout_id);
		udt->_timeout_id = INVALID_MSG_ID;
	}
	if (udt->_notify_recv_msgid != INVALID_MSG_ID)
	{
		cancel_timer(udt->_notify_recv_msgid);
		udt->_notify_recv_msgid = INVALID_MSG_ID;
	}
	if (udt->_notify_send_msgid != INVALID_MSG_ID)
	{
		cancel_timer(udt->_notify_send_msgid);
		udt->_notify_send_msgid = INVALID_MSG_ID;
	}
	if (udt->_cca != NULL)
	{
		sd_free(udt->_cca);
		udt->_cca = NULL;
	}
	if (udt->_rtt != NULL)
	{
		sd_free(udt->_rtt);
		udt->_rtt = NULL;
	}
	while (list_size(&udt->_waiting_send_queue) > 0)
	{
		list_pop(&udt->_waiting_send_queue, (void**)&send_buffer);
		--send_buffer->_reference_count;
		if(send_buffer->_reference_count == 0)
		{
			SAFE_DELETE(send_buffer->_buffer);
			udt_free_udt_send_buffer(send_buffer);
		}
	}
	while(list_size(&udt->_had_send_queue) > 0)
	{
		list_pop(&udt->_had_send_queue, (void**)&send_buffer);
		--send_buffer->_reference_count;
		if(send_buffer->_reference_count == 0)
		{
			SAFE_DELETE(send_buffer->_buffer);
			udt_free_udt_send_buffer(send_buffer);
		}
	}
	while(set_size(&udt->_udt_recv_buffer_set) > 0)
	{
		iter = SET_BEGIN(udt->_udt_recv_buffer_set);
		recv_buffer = (UDT_RECV_BUFFER*)iter->_data;
		set_erase_iterator(&udt->_udt_recv_buffer_set, iter);
		ptl_free_udp_buffer(recv_buffer->_buffer);
		udt_free_udt_recv_buffer(recv_buffer);
	}
	LOG_DEBUG("[udt = 0x%x, device = 0x%x]udt_send_reset. conn_id = %u.", udt, udt->_device, (_u32)udt->_id._virtual_source_port);
	udt_send_reset(udt);
	udt_free_udt_device(udt);
	return SUCCESS;			//这里一定要返回SUCCESS,上层才能正确释放内存
}

_int32 udt_device_connect(UDT_DEVICE* udt, _u32 ip, _u16 udp_port)
{
	return udt_active_connect(udt, ip, udp_port);
}

_int32 udt_device_rebuild_package_and_send(UDT_DEVICE* udt, char* buffer
    , _u32 buffer_len, BOOL is_data)
{
	UDT_SEND_BUFFER* send_buffer = NULL;
	_int32 ret = SUCCESS;
	_u32 offset = 0, buffer_offset;
	_u32 copy_len = 0;
	char* slip_buffer = NULL;
	_u32  slip_buffer_len = 0;
	offset = UDT_NEW_HEADER_SIZE;
	buffer_offset = UDT_NEW_HEADER_SIZE;
	while(buffer_offset<buffer_len)
	{
		ret = sd_malloc(udt_get_mtu_size(), (void**)&slip_buffer);
		CHECK_VALUE(ret);
		slip_buffer_len = udt_get_mtu_size();
		copy_len = MIN(slip_buffer_len - offset, buffer_len - buffer_offset);
		sd_memcpy(slip_buffer + offset, buffer + buffer_offset, copy_len);
		buffer_offset += copy_len;
		sd_assert(udt->_state == ESTABLISHED_STATE);
		ret = udt_malloc_udt_send_buffer(&send_buffer);
		CHECK_VALUE(ret);
		sd_memset(send_buffer, 0, sizeof(UDT_SEND_BUFFER));
		send_buffer->_buffer = slip_buffer;
		send_buffer->_buffer_len = copy_len + offset;
		send_buffer->_data_len = copy_len;
		send_buffer->_is_data = is_data;
		sd_assert(send_buffer->_data_len > 0);
		send_buffer->_reference_count = 0;
		list_push(&udt->_waiting_send_queue, send_buffer);
		++send_buffer->_reference_count;		//引用加1，因为push到一个队列中
		udt_update_waiting_send_queue(udt);
	}
	sd_assert(buffer_offset == buffer_len);
	sd_free(buffer);
	buffer = NULL;
	udt_notify_ptl_send_callback(udt);
	return ret;
}

_int32 udt_device_send(UDT_DEVICE* udt, char* buffer, _u32 len, BOOL is_data)
{
    LOG_DEBUG("[udt = 0x%x, device = 0x%x] udt_device_send called, len = %d, is_data = %d"
        ,udt , udt->_device, len, is_data);
	_int32 ret = SUCCESS;
	UDT_SEND_BUFFER* send_buffer = NULL;
	sd_assert(udt->_send_len_record == 0);
	udt->_send_len_record = len;
	if(len > udt_get_mtu_size() )
		return udt_device_rebuild_package_and_send(udt, buffer, len, is_data);
	sd_assert(udt->_state == ESTABLISHED_STATE);
	ret = udt_malloc_udt_send_buffer(&send_buffer);
	CHECK_VALUE(ret);
	sd_memset(send_buffer, 0, sizeof(UDT_SEND_BUFFER));
	send_buffer->_buffer = buffer;
	send_buffer->_buffer_len = len;
	send_buffer->_data_len = len - UDT_NEW_HEADER_SIZE;
	send_buffer->_is_data = is_data;
	sd_assert(send_buffer->_data_len > 0);
	send_buffer->_reference_count = 0;
	list_push(&udt->_waiting_send_queue, send_buffer);
	++send_buffer->_reference_count;		//引用加1，因为push到一个队列中
	udt_update_waiting_send_queue(udt);
	 LOG_DEBUG("[udt = 0x%x, device = 0x%x]  send 2", udt, udt->_device);
	udt_notify_ptl_send_callback(udt);
	return ret;
}

_int32 udt_device_recv(UDT_DEVICE* udt, RECV_TYPE type, char* buffer, _u32 expect_len, _u32 buffer_len)
{
	sd_assert(udt->_notify_recv_msgid == INVALID_MSG_ID);
	sd_assert(buffer != NULL && expect_len > 0 && udt->_recv_buffer == NULL);
	udt->_recv_type = type;
	udt->_recv_buffer = buffer;
	udt->_expect_recv_len = expect_len;
	udt->_had_recv_len = 0;
	udt_update_recv_buffer_set(udt);
	return SUCCESS;
}

_int32 udt_device_notify_send_data(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid)
{
	_u32 send_len = 0;
	UDT_DEVICE* udt = NULL;
	if(errcode == MSG_CANCELLED)
		return SUCCESS;
	udt = (UDT_DEVICE*)msg_info->_user_data;
	sd_assert(udt->_notify_send_msgid == msgid);
	send_len = udt->_send_len_record;
	udt->_notify_send_msgid = INVALID_MSG_ID;
	udt->_send_len_record = 0;
	//之所以使用send_len变量是因为在ptl_send_callback函数调用中会访问udt->_send_len_record变量
	ptl_send_callback(SUCCESS, udt->_device, send_len);
	return SUCCESS;
}

_int32 udt_device_notify_recv_data(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid)
{
	UDT_DEVICE* udt = NULL;
	if(errcode == MSG_CANCELLED)
		return SUCCESS;
	udt = (UDT_DEVICE*)msg_info->_device_id;
	sd_assert(udt->_notify_recv_msgid == msgid);
	udt->_notify_recv_msgid = INVALID_MSG_ID;
	ptl_udt_recv_callback(SUCCESS, udt->_recv_type, udt->_device, (_u32)msg_info->_user_data);
	return SUCCESS;
}

_int32 udt_device_notify_punch_hole_success(UDT_DEVICE* udt, _u32 remote_ip, _u16 remote_port)
{
	LOG_DEBUG("[udt = 0x%x, device = 0x%x]udt_device_notify_punch_hole_success.", udt, udt->_device);
	if(udt->_state == INIT_STATE)
	{
		return udt_device_connect(udt, remote_ip, remote_port);
	}
	else
	{
		LOG_ERROR("[udt = 0x%x, device = 0x%x]udt_device_notify_punch_hole_success, but udt state = %d is wrong.", udt, udt->_device, udt->_state);
	}
	return SUCCESS;		
}






