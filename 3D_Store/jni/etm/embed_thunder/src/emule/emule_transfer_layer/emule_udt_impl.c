#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL
#include "emule_udt_impl.h"
#include "emule_udt_device_pool.h"
#include "emule_udt_cmd.h"
#include "emule_socket_device.h"
#include "../emule_utility/emule_opcodes.h"
#include "asyn_frame/device.h"
#include "utility/time.h"
#include "utility/utility.h"
#include "utility/bytebuffer.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_EMULE
#include "utility/logger.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//�������ӵĹ���:(A ������B)
//1.����A ������syn��B,����һ��������ɵĲ�Ϊ0��conn_num��B
//2.B�յ��󣬷���һ��PING��A,��PING����conn_numΪ0
//3.A���յ�PING���󣬱�ʾ�����Ѿ��ɹ�������������һ��PING��B,��PING��conn_num
//   ΪA ֮ǰ������ɵĲ�Ϊ0��ֵ��B�յ������������Լ���conn_num.�Ժ��ͨ�ž�
//   ����(ip, port, conn_num)����ʾ����һ��udt�豸
/////////////////////////////////////////////////////////////////////////////////////////////////////////

#define		TM_KEEPALIVE			20		// 20 seconds

_int32 emule_udt_on_ping(EMULE_UDT_DEVICE* udt_device, _u32 conn_num)
{
	_int32 ret = SUCCESS;
	LOG_DEBUG("[emule_udt_device = 0x%x]emule_udt_on_ping, conn_num = %u.", udt_device, conn_num);
	sd_time(&udt_device->_last_recv_time);
	if(udt_device->_state == UDT_TRAVERSE)
	{
		sd_assert(udt_device->_transfer != NULL);
		emule_transfer_close(udt_device->_transfer);
		udt_device->_transfer = NULL;
		udt_device->_state = UDT_ESTABLISHED;
		// SYN_SENT���͵�_conn_num�Ƿ�0.
		// �Է����յ���,ʹ��_conn_num������
		// ��������յ��Է���ping,���ڶԷ����ĵ�һ�ζ���conn_num����0,
		//����0�����õ�ǰ����,�����ᵼ�� verycd�޷��ҵ���Ӧ����
//		if (conn_num != 0 && udt_device->_conn_num == 0)
//			udt_device->_conn_num = conn_num;
//		emule_udt_send_ping(udt_device, FALSE);
		ret = emule_udt_recv_queue_create(&udt_device->_recv_queue, udt_device);
		CHECK_VALUE(ret);
		ret = emule_udt_send_queue_create(&udt_device->_send_queue, udt_device);
		CHECK_VALUE(ret);
		start_timer(emule_udt_on_timeout, NOTICE_INFINITE, 30, 0, udt_device, &udt_device->_timeout_id);
		emule_socket_device_connect_callback(udt_device->_socket_device, SUCCESS);
	}
	return ret;
}

_int32 emule_udt_on_keepalive(_u32 ip, _u16 port)
{
	EMULE_UDT_DEVICE* udt_device = NULL;
	udt_device = emule_udt_device_find(ip, port, 0);
	if(udt_device == NULL)
	{
		LOG_ERROR("emule_udt_on_keepalive, ip = %u, port = %u, but can't find udt_device.", ip, (_u32)port);
		return SUCCESS;
	}
	LOG_DEBUG("[emule_udt_device = 0x%x]emule_udt_on_keepalive.", udt_device);
	sd_time(&udt_device->_last_recv_time);
	return SUCCESS;
}

_int32 emule_udt_on_ack(EMULE_UDT_DEVICE* udt_device, _u32 ack_num, _u32 wnd_size)
{
	LOG_DEBUG("[emule_udt_device = 0x%x]emule udt ack seq[%u], wnd[%u].", udt_device, ack_num, wnd_size);
	sd_time(&udt_device->_last_recv_time);
	return emule_udt_send_queue_remove(udt_device->_send_queue, ack_num, wnd_size);
}

_int32 emule_udt_on_reset(EMULE_UDT_DEVICE* udt_device)
{
	LOG_DEBUG("[emule_udt_device = 0x%x]emule_udt_on_reset, udt_state = %d.", udt_device, udt_device->_state);
	if(udt_device->_state >= UDT_RESET)
		return SUCCESS;
	udt_device->_state = UDT_RESET;
	return emule_socket_device_error(udt_device->_socket_device, -1);
}

_int32 emule_udt_on_nat_failed(EMULE_UDT_DEVICE* udt_device)
{
	LOG_DEBUG("[emule_udt_device = 0x%x]emule_udt_on_nat_failed.", udt_device);
	sd_assert(udt_device->_transfer != NULL);
	return emule_transfer_recv(udt_device->_transfer, OP_NAT_FAILED, 0, 0);
}

_int32 emule_udt_on_nat_reping(EMULE_UDT_DEVICE* udt_device)
{
	LOG_DEBUG("[emule_udt_device = 0x%x]emule_udt_on_nat_reping.", udt_device);
	return emule_udt_send_reping(udt_device);
}

_int32 emule_udt_on_nat_sync2(EMULE_UDT_DEVICE* udt_device, _u32 ip, _u16 port)
{
	LOG_DEBUG("[emule_udt_device = 0x%x]emule_udt_on_nat_sync2.", udt_device);
	sd_assert(udt_device->_transfer != NULL);
	return emule_transfer_recv(udt_device->_transfer, OP_NAT_SYNC2, ip, port);
}

_int32 emule_udt_on_data(EMULE_UDT_DEVICE* udt_device, char** buffer, _int32 len)
{
	_u32 seq = 0;
	char* tmp_buf = *buffer + 10;
	_int32 tmp_len = len - 10;
	sd_time(&udt_device->_last_recv_time);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&seq);
	sd_assert(seq != 0);
	if(seq < udt_device->_recv_queue->_last_read_segment || emule_udt_recv_queue_lookup(udt_device->_recv_queue, seq))
	{
		//�ظ��İ������֮ǰ��ack�������ˣ���ô���п����յ��ظ��İ�
		return emule_udt_send_ack(udt_device, seq);
	}
	if(emule_udt_recv_queue_size(udt_device->_recv_queue) > 64 && seq > udt_device->_recv_queue->_last_read_segment + 25)
	{
		//û���㹻�Ŀռ������ɸ����ݰ��ˣ�����
		return SUCCESS;
	}
	//����µ����ݰ���recv_queue����
	if(emule_udt_recv_queue_add(udt_device->_recv_queue, seq, buffer, len) == FALSE)
		return SUCCESS;			//����°�ʧ��
	//��ӳɹ�����ack��֪�Է��Ѿ��յ��ð�
	emule_udt_send_ack(udt_device, seq);
	//����յ��ĸð�������ĿǰҪ��ȡ�ģ�����֪ͨ�ϲ�����ȡ����
	if(seq == udt_device->_recv_queue->_last_read_segment && udt_device->_recv_queue->_recv_msg_id == INVALID_MSG_ID)
		emule_udt_read_data(udt_device);
	return SUCCESS;
}

_int32 emule_udt_on_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid)
{
	_u32 now = 0;
    EMULE_UDT_DEVICE* udt_device = NULL;
	if(errcode == MSG_CANCELLED)
		return SUCCESS;
	udt_device = (EMULE_UDT_DEVICE*)msg_info->_user_data;
	sd_time(&now);
	if(now - udt_device->_last_send_keepalive_time > TM_KEEPALIVE)
	{
		emule_udt_send_ping(udt_device, TRUE);
		sd_time(&udt_device->_last_send_keepalive_time);
	}
	return emule_udt_send_queue_timeout(udt_device->_send_queue);	
}

_int32 emule_udt_read_data(EMULE_UDT_DEVICE* udt_device)
{
	sd_assert(udt_device->_recv_queue->_recv_msg_id == INVALID_MSG_ID);
	return emule_udt_recv_queue_read(udt_device->_recv_queue);
}


#endif /* ENABLE_EMULE */
#endif /* ENABLE_EMULE */

