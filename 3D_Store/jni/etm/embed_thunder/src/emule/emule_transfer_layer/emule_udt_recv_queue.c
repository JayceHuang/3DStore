#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL
#include "emule_udt_recv_queue.h"
#include "emule_socket_device.h"
#include "emule_udt_device.h"
#include "../emule_utility/emule_memory_slab.h"
#include "asyn_frame/device.h"
#include "utility/string.h"
#include "utility/utility.h"
#include "utility/bytebuffer.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_EMULE
#include "utility/logger.h"

_int32 emule_udt_recv_buffer_comparator(void* E1, void* E2)
{
	EMULE_UDT_RECV_BUFFER* left = (EMULE_UDT_RECV_BUFFER*)E1;
	EMULE_UDT_RECV_BUFFER* right = (EMULE_UDT_RECV_BUFFER*)E2;
	return (_int32)(left->_seq - right->_seq);
}

_int32 emule_udt_recv_queue_create(EMULE_UDT_RECV_QUEUE** recv_queue, struct tagEMULE_UDT_DEVICE* udt_device)
{
	_int32 ret = SUCCESS;
	ret = emule_get_emule_udt_recv_queue_slip(recv_queue);
	CHECK_VALUE(ret);
	LOG_DEBUG("[emule_udt_device = 0x%x]emule_udt_recv_queue_create.", udt_device);
	sd_memset(*recv_queue, 0, sizeof(EMULE_UDT_RECV_QUEUE));
	set_init(&(*recv_queue)->_recv_buffer_set, emule_udt_recv_buffer_comparator);
	(*recv_queue)->_udt_device = udt_device;
	(*recv_queue)->_last_read_segment = 1;
	return SUCCESS;
}

_int32 emule_udt_recv_queue_close(EMULE_UDT_RECV_QUEUE* recv_queue)
{
	sd_assert(FALSE);
	return SUCCESS;
}

_int32 emule_udt_recv_queue_read(EMULE_UDT_RECV_QUEUE* recv_queue)
{
	_int32 ret = SUCCESS;
	_int32 copy_len = 0;
	MSG_INFO msg = {};
	EMULE_UDT_RECV_BUFFER* recv_buffer = NULL;
	SET_ITERATOR iter = SET_BEGIN(recv_queue->_recv_buffer_set);
	while(iter != SET_END(recv_queue->_recv_buffer_set))
	{
		recv_buffer = (EMULE_UDT_RECV_BUFFER*)iter->_data;
		if(recv_buffer->_seq != recv_queue->_last_read_segment)
			break;		//跳包了
		copy_len = MIN(recv_queue->_len - recv_queue->_offset, recv_buffer->_data_len - recv_queue->_segment_offset);
		sd_assert(copy_len != 0);
		sd_memcpy(recv_queue->_buffer, recv_buffer->_data + recv_queue->_segment_offset, copy_len);
		recv_queue->_offset += copy_len;
		recv_queue->_segment_offset += copy_len;
		if(recv_queue->_segment_offset == recv_buffer->_data_len)
		{	
			//该片已经全部读取完毕，可以释放
			LOG_DEBUG("[recv_queue = 0x%x]udt_recv_queue erase seq = %u, data_len = %u.", recv_queue, recv_buffer->_seq, recv_buffer->_data_len);
			++recv_queue->_last_read_segment;
			recv_queue->_segment_offset = 0;
			recv_queue->_recv_data_size -= recv_buffer->_data_len;
			set_erase_iterator(&recv_queue->_recv_buffer_set, iter);
			emule_free_udp_buffer_slip(recv_buffer->_buffer);
			emule_free_emule_udt_recv_buffer_slip(recv_buffer);
			iter = SET_BEGIN(recv_queue->_recv_buffer_set);
		}
		if(recv_queue->_offset == recv_queue->_len)
			break;
	}
	if(recv_queue->_offset == recv_queue->_len)
	{
		sd_assert(recv_queue->_recv_msg_id == INVALID_MSG_ID);
		msg._device_id = 0;
		msg._user_data = (void*)recv_queue;
		msg._device_type = DEVICE_NONE;
		msg._operation_type = OP_NONE;
		ret = post_message(&msg, emule_recv_queue_notify_msg, NOTICE_ONCE, 0, &recv_queue->_recv_msg_id);
		CHECK_VALUE(ret);
	}
	return ret;
}

_int32 emule_recv_queue_notify_msg(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid)
{
	char* recv_buffer = NULL;
	_u32 recv_len = 0;
	EMULE_UDT_RECV_QUEUE* recv_queue = (EMULE_UDT_RECV_QUEUE*)msg_info->_user_data;
	if(errcode == MSG_CANCELLED)
		return SUCCESS;
	LOG_DEBUG("emule_recv_queue_notify_msg.");
	recv_buffer = recv_queue->_buffer;
	recv_len = recv_queue->_len;
	recv_queue->_recv_msg_id = INVALID_MSG_ID;
	recv_queue->_buffer = NULL;
	recv_queue->_len = 0;
	recv_queue->_offset = 0;
	emule_socket_device_recv_callback(recv_queue->_udt_device->_socket_device, recv_buffer, recv_len, recv_queue->_is_recv_data, SUCCESS);
	return SUCCESS;
}

BOOL emule_udt_recv_queue_lookup(EMULE_UDT_RECV_QUEUE* recv_queue, _u32 seq)
{
	EMULE_UDT_RECV_BUFFER* recv_buffer = NULL;
	set_find_node(&recv_queue->_recv_buffer_set, &seq, (void**)&recv_buffer);
	if(recv_buffer == NULL)
		return FALSE;
	else
		return TRUE;
}

BOOL emule_udt_recv_queue_add(EMULE_UDT_RECV_QUEUE* recv_queue, _u32 seq, char** buffer, _u32 len)
{
	_int32 ret = SUCCESS;
	EMULE_UDT_RECV_BUFFER* recv_buffer = NULL;	
	ret = emule_get_emule_udt_recv_buffer_slip(&recv_buffer);
	if(ret != SUCCESS)
	{
		sd_assert(FALSE);
		return FALSE;
	}
	recv_buffer->_seq = seq;
	recv_buffer->_buffer = *buffer;
	recv_buffer->_data = *buffer + 14;
	recv_buffer->_data_len = len - 14;
	ret = set_insert_node(&recv_queue->_recv_buffer_set, recv_buffer);
	if(ret != SUCCESS)
	{
		emule_free_emule_udt_recv_buffer_slip(recv_buffer);
		sd_assert(FALSE);
		return FALSE;
	}
	LOG_DEBUG("[recv_queue = 0x%x]emule recv queue add recv_buffer, seq = %u, data_len = %u.", recv_queue, recv_buffer->_seq, recv_buffer->_data_len);
	//数据包已经被接收，所以把buffer置为NULL,告诉上层不需要删除该buffer
	*buffer = NULL;
	recv_queue->_recv_data_size += recv_buffer->_data_len;
	return TRUE;
}

_u32 emule_udt_recv_queue_size(EMULE_UDT_RECV_QUEUE* recv_queue)
{
	return set_size(&recv_queue->_recv_buffer_set);
}


#endif /* ENABLE_EMULE */
#endif /* ENABLE_EMULE */

