#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL
#include "emule_udt_cmd.h"
#include "emule_udt_device_pool.h"
#include "emule_udt_impl.h"
#include "emule_nat_server.h"
#include "../emule_impl.h"
#include "../emule_utility/emule_opcodes.h"
#include "../emule_utility/emule_udp_device.h"
#include "utility/mempool.h"
#include "utility/local_ip.h"
#include "utility/utility.h"
#include "utility/time.h"
#include "utility/bytebuffer.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_EMULE
#include "utility/logger.h"

#define	MAX_BUF_SIZE		8192

_int32 emule_udt_send_syn(EMULE_UDT_DEVICE* udt_device)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL, *tmp_buf = NULL;
	_int32 len = 32, tmp_len = 0;
	EMULE_PEER* local_peer = emule_get_local_peer();
	LOG_DEBUG("[udt_device = 0x%x]emule_udt_send_syn.", udt_device);
	ret = sd_malloc((_u32)len, (void**)&buffer);
	if(ret != SUCCESS)
		return ret;
	tmp_buf = buffer;
	tmp_len = len;
	sd_set_int8(&tmp_buf, &tmp_len, OP_VC_NAT_HEADER);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, len - EMULE_HEADER_SIZE);
	sd_set_int8(&tmp_buf, &tmp_len, OP_NAT_SYNC);
	//这里要填充的是外网的ip,目前测试仅仅填本机内网ip
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)sd_get_local_ip());
	sd_set_int16_to_lt(&tmp_buf, &tmp_len, (_int16)sd_htons(local_peer->_udp_port));
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)local_peer->_user_id, USER_ID_SIZE);
	ret = sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)udt_device->_conn_num);
	sd_assert(ret == SUCCESS && tmp_len == 0); 	
	return emule_udp_sendto(buffer, len, udt_device->_ip, udt_device->_udp_port, TRUE);
}

_int32 emule_udt_send_ping(EMULE_UDT_DEVICE* udt_device, BOOL keepalive)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL, *tmp_buf = NULL;
	_int32 len = 0, tmp_len = 0;
	EMULE_PEER* local_peer = emule_get_local_peer();
	if(keepalive == FALSE)
	{
		LOG_DEBUG("[emule_udt_device = 0x%x]emule_udt_device send PING.", udt_device);
		len = 26;
	}
	else
	{
		LOG_DEBUG("[emule_udt_device = 0x%x]emule_udt_device send KEEPALIVE.", udt_device);
		len = 6;
	}
	ret = sd_malloc((_u32)len, (void**)&buffer);
	if(ret != SUCCESS)
		return ret;
	tmp_buf = buffer;
	tmp_len = len;
	sd_set_int8(&tmp_buf, &tmp_len, OP_VC_NAT_HEADER);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, len - EMULE_HEADER_SIZE);
	ret = sd_set_int8(&tmp_buf, &tmp_len, OP_NAT_PING);
	if(keepalive == FALSE)
	{
		sd_set_bytes(&tmp_buf, &tmp_len, (char*)local_peer->_user_id, USER_ID_SIZE);
		ret = sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)udt_device->_conn_num);
	}
	sd_assert(ret == SUCCESS && tmp_len == 0);
	return emule_udp_sendto(buffer, len, udt_device->_ip, udt_device->_udp_port, TRUE);
}

_int32 emule_udt_send_data(EMULE_UDT_DEVICE* udt_device, EMULE_UDT_SEND_BUFFER* send_buffer)
{
	_int32 ret = SUCCESS;
	char* tmp_buf = NULL;
	_int32 tmp_len = 0;
	LOG_DEBUG("[emule_udt_device = 0x%x]emule_udt_device send data, seq = %u, data_len = %u.", udt_device, send_buffer->_seq_num, send_buffer->_data_len);
	if(send_buffer->_send_count == 0)
	{
		//第一次发送才需要填充头部
		tmp_buf = send_buffer->_buffer;
		tmp_len = DATA_SEGMENT_HEADER_SIZE;
		sd_set_int8(&tmp_buf, &tmp_len, OP_VC_NAT_HEADER);
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, send_buffer->_data_len + DATA_SEGMENT_HEADER_SIZE - EMULE_HEADER_SIZE);
		sd_set_int8(&tmp_buf, &tmp_len, OP_NAT_DATA);
		sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)udt_device->_conn_num);
		ret = sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)send_buffer->_seq_num);
		sd_assert(ret == SUCCESS && tmp_len == 0);
	}
	sd_time_ms(&send_buffer->_send_time);		//更新最后发送时间
	++send_buffer->_send_count;

    if(send_buffer->_is_data)
    {
	    return emule_udp_sendto_in_speed_limit(send_buffer->_buffer
        	, send_buffer->_data_len + DATA_SEGMENT_HEADER_SIZE
        	, udt_device->_ip
        	, udt_device->_udp_port
        	, FALSE);
    }
    else
    {
        return emule_udp_sendto(send_buffer->_buffer
            , send_buffer->_data_len + DATA_SEGMENT_HEADER_SIZE
            , udt_device->_ip
            , udt_device->_udp_port
            , FALSE);
    }
}

_int32 emule_udt_send_ack(EMULE_UDT_DEVICE* udt_device, _u32 seq)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL, *tmp_buf = NULL;
	_int32 len = 18, tmp_len = 0;
	_u32 wnd = 0;
	if(3 * MAX_BUF_SIZE > udt_device->_recv_queue->_recv_data_size)
	{
		wnd = 3 * MAX_BUF_SIZE- udt_device->_recv_queue->_recv_data_size;
	}
/*
	电驴源代码中有这个重新通知窗口大小的调用，不知效果怎样
	if (wnd< MAXFRAGSIZE)
	{
		m_bReNotifyWindowSize = true;
	}
*/
	LOG_DEBUG("[emule_udt_device = 0x%x]emule_udt_device send ack seq[%u], wnd[%u].", udt_device, seq, wnd);
	ret = sd_malloc((_u32)len, (void**)&buffer);
	if(ret != SUCCESS)
		return ret;
	tmp_buf = buffer;
	tmp_len = len;
	sd_set_int8(&tmp_buf, &tmp_len, OP_VC_NAT_HEADER);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, len - EMULE_HEADER_SIZE);
	sd_set_int8(&tmp_buf, &tmp_len, OP_NAT_ACK);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)udt_device->_conn_num);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)seq);
	ret = sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)wnd);
	sd_assert(ret == SUCCESS && tmp_len == 0);
	return emule_udp_sendto(buffer, len, udt_device->_ip, udt_device->_udp_port, TRUE);
}

_int32 emule_udt_send_reping(EMULE_UDT_DEVICE* udt_device)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL, *tmp_buf = NULL;
	_int32 len = 26, tmp_len = 0;
	EMULE_PEER* local_peer = emule_get_local_peer();
	LOG_DEBUG("[emule_udt_device = 0x%x]emule_udt_send_reping.", udt_device);
	ret = sd_malloc((_u32)len, (void**)&buffer);
	if(ret != SUCCESS)
		return ret;
	tmp_buf = buffer;
	tmp_len = len;
	sd_set_int8(&tmp_buf, &tmp_len, OP_VC_NAT_HEADER);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, len - EMULE_HEADER_SIZE);
	sd_set_int8(&tmp_buf, &tmp_len, OP_NAT_REPING);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)local_peer->_user_id, USER_ID_SIZE);
	ret = sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)udt_device->_conn_num);
	sd_assert(ret == SUCCESS && tmp_len == 0);
	return emule_udp_sendto(buffer, len, udt_device->_ip, udt_device->_udp_port, TRUE);
}

_int32 emule_recv_udt_cmd(char** buffer, _int32 len, _u32 ip, _u16 port)
{
	_int32 cmd_len = 0;
	_u8 protocol = 0, opcode = 0;
	char* tmp_buf = *buffer;
	_int32 tmp_len = len;
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&protocol);
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, &cmd_len);
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&opcode);
	switch(opcode)
	{
		case OP_NAT_SYNC:
			sd_assert(FALSE);
		case OP_NAT_PING:
			emule_udt_recv_ping(tmp_buf, tmp_len, ip, port);
			break;
		case OP_NAT_REGISTER:
			emule_udt_recv_nat_register(tmp_buf, tmp_len, ip, port);
			break;
		case OP_NAT_FAILED:
			emule_udt_recv_nat_failed(tmp_buf, tmp_len, ip, port);
			break;
		case OP_NAT_REPING:
			emule_udt_recv_nat_reping(tmp_buf, tmp_len, ip, port);
			break;
		case OP_NAT_SYNC2:
			emule_udt_recv_nat_sync2(tmp_buf, tmp_len);
			break;
		case OP_NAT_DATA:
			emule_udt_recv_data(buffer, len, ip, port);
			break;
		case OP_NAT_ACK:
			emule_udt_recv_ack(tmp_buf, tmp_len, ip, port);
			break;
		case OP_NAT_RST:
			emule_udt_recv_reset(tmp_buf, tmp_len, ip, port);
			break;
		default:
			sd_assert(FALSE);
	}
	return SUCCESS;
}

_int32 emule_udt_recv_ping(char* buffer, _int32 len, _u32 ip, _u16 port)
{
	_int32 ret = SUCCESS;
	EMULE_UDT_DEVICE* udt_device = NULL;
	_u8 conn[CONN_ID_SIZE] = {0};
	_u32 conn_num = 0;
	if(len == 0)
	{
		return emule_udt_on_keepalive(ip, port);
	}
	// ping报文
	sd_get_bytes(&buffer, &len, (char*)conn, CONN_ID_SIZE);
	ret = sd_get_int32_from_lt(&buffer, &len, (_int32*)&conn_num);
	sd_assert(ret == SUCCESS);		
	udt_device = emule_udt_device_find_by_conn_id(conn);
	if(udt_device == NULL)
		return SUCCESS;
	udt_device->_ip = ip;
	udt_device->_udp_port = port;
	return emule_udt_on_ping(udt_device, conn_num);
}

_int32 emule_udt_recv_nat_register(char* buffer, _int32 len, _u32 ip, _u16 port)
{
	_u32 server_ip = 0;
	_u16 server_port = 0;
	if(len >= 2)
	{
		sd_assert(len == 2);
		sd_get_int16_from_lt(&buffer, &len, (_int16*)&server_port);
	}
	LOG_DEBUG("emule_udt_recv_nat_register.");
	//这个地方特意将server_ip设置为0，目的是使用域名去发送数据包
	return emule_nat_server_notify_register(server_ip, server_port);
}

_int32 emule_udt_recv_nat_failed(char* buffer, _int32 len, _u32 ip, _u16 port)
{
	_int32 ret = SUCCESS;
	EMULE_UDT_DEVICE* udt_device = NULL;
	_u8 flag = 0;
	_u8 conn_id[CONN_ID_SIZE] = {0};
	sd_get_int8(&buffer, &len, (_int8*)&flag);
	sd_assert(flag ==  1);
	sd_get_bytes(&buffer, &len, (char*)conn_id, CONN_ID_SIZE);
	udt_device = emule_udt_device_find_by_conn_id(conn_id);
	sd_assert(udt_device != NULL);
	if(udt_device != NULL)
		ret = emule_udt_on_nat_failed(udt_device);
	return ret;
}

_int32 emule_udt_recv_nat_reping(char* buffer, _int32 len, _u32 ip, _u16 port)
{
	_int32 ret = SUCCESS;
	EMULE_UDT_DEVICE* udt_device = NULL;
	_u8 conn_id[CONN_ID_SIZE] = {0};
	ret = sd_get_bytes(&buffer, &len, (char*)conn_id, CONN_ID_SIZE);
	sd_assert(ret == SUCCESS && len == 0);
	udt_device = emule_udt_device_find_by_conn_id(conn_id);
	sd_assert(udt_device != NULL);
	if(udt_device != NULL)
	{
		sd_assert(ip == udt_device->_ip && port == udt_device->_udp_port);
		ret = emule_udt_on_nat_reping(udt_device);
	}
	return ret;
}

_int32 emule_udt_recv_nat_sync2(char* buffer, _int32 len)
{
	_int32 ret = SUCCESS;
	EMULE_UDT_DEVICE* udt_device = NULL;
	_u32 ip = 0, conn_num = 0;
	_u16 port = 0;
	_u8 conn_id[CONN_ID_SIZE] = {0};
	sd_get_int32_from_lt(&buffer, &len, (_int32*)&ip);
	//这里获得的端口号是网络序，要转为主机序
	sd_get_int16_from_lt(&buffer, &len, (_int16*)&port);
	port = sd_ntohs(port);
	sd_assert(ip != 0 && port != 0);
	sd_get_bytes(&buffer, &len, (char*)conn_id, CONN_ID_SIZE);
	ret = sd_get_int32_from_lt(&buffer, &len, (_int32*)&conn_num);
	sd_assert(ret == SUCCESS && len == 0);
	udt_device = emule_udt_device_find_by_conn_id(conn_id);
	if(udt_device != NULL)
		ret = emule_udt_on_nat_sync2(udt_device, ip, port);
	else
		LOG_ERROR("emule udt recv nat_sync2, but no udt_device.");
	return ret;
}

_int32 emule_udt_recv_data(char** buffer, _int32 len, _u32 ip, _u16 port)
{
	_int32 ret = SUCCESS;
	_u32 conn_num = 0;
	EMULE_UDT_DEVICE* udt_device = NULL;
	char* tmp_buf = *buffer + 6;
	_int32 tmp_len = len - 6;
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&conn_num);
	udt_device = emule_udt_device_find(ip, port, conn_num);
	if(udt_device != NULL)
		ret = emule_udt_on_data(udt_device, buffer, len);
	return ret;
}


_int32 emule_udt_recv_ack(char* buffer, _int32 len, _u32 ip, _u16 port)
{
	_int32 ret = SUCCESS;
	_u32 conn_num = 0, ack_num = 0, win_size = 0;
	EMULE_UDT_DEVICE* udt_device = NULL;
	sd_get_int32_from_lt(&buffer, &len, (_int32*)&conn_num);
	sd_get_int32_from_lt(&buffer, &len, (_int32*)&ack_num);
	ret = sd_get_int32_from_lt(&buffer, &len, (_int32*)&win_size);
	sd_assert(ret == SUCCESS && len == 0);
	udt_device = emule_udt_device_find(ip, port, conn_num);
	if(udt_device != NULL)
		ret = emule_udt_on_ack(udt_device, ack_num, win_size);
	return ret;
}

_int32 emule_udt_recv_reset(char* buffer, _int32 len, _u32 ip, _u16 port)
{
	_int32 ret = SUCCESS;
	LOG_ERROR("warning : emule_udt_recv_reset, but not process, must fix when have time.");
	return ret;
}


#endif /* ENABLE_EMULE */
#endif /* ENABLE_EMULE */       

