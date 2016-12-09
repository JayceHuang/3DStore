#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL
#include "emule_traverse_by_svr.h"
#include "emule_nat_server.h"
#include "emule_udt_device.h"
#include "../emule_utility/emule_opcodes.h"
#include "../emule_utility/emule_udp_device.h"
#include "../emule_impl.h"
#include "utility/mempool.h"
#include "utility/bytebuffer.h"
#include "utility/string.h"
#include "utility/utility.h"
#include "utility/time.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_EMULE
#include "utility/logger.h"

#define		ATTEMPT_INTERVAL				4
#define		SYNC2_ATTEMPT_TIMES			8
#define		PING_ATTEMPT_TIMES			15

_int32 emule_traverse_by_svr_create(EMULE_TRANSFER_BY_SERVER** traverse, void* user_data)
{
	_int32 ret = SUCCESS;
	ret = sd_malloc(sizeof(EMULE_TRANSFER_BY_SERVER), (void**)traverse);
	if(ret != SUCCESS)
		return ret;
	sd_memset(*traverse, 0, sizeof(EMULE_TRANSFER_BY_SERVER));
	(*traverse)->_user_data = user_data;
	(*traverse)->_timeout_id = INVALID_MSG_ID;
	return ret;
}

_int32 emule_traverse_by_svr_close(EMULE_TRANSFER_BY_SERVER* traverse)
{
	_int32 ret = SUCCESS;
	ret = cancel_timer(traverse->_timeout_id);
	CHECK_VALUE(ret);
	return sd_free(traverse);
}

_int32 emule_traverse_by_svr_start(EMULE_TRANSFER_BY_SERVER* traverse)
{
/*
	if(emule_nat_server_enable() == FALSE)
	{
		sd_assert(FALSE);
		return -1;
	}
*/
	traverse->_state = NAT_IDLE_STATE;
	start_timer(emule_traverse_by_svr_timeout, NOTICE_INFINITE, ATTEMPT_INTERVAL * 1000, 0, traverse, &traverse->_timeout_id);
	return emule_traverse_by_svr_send_sync2(traverse);	
}

_int32 emule_traverse_by_svr_send_ping(EMULE_TRANSFER_BY_SERVER* traverse)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL, *tmp_buf = NULL;
	_int32 len = 26, tmp_len = 0;
	EMULE_UDT_DEVICE* udt_device = (EMULE_UDT_DEVICE* )traverse->_user_data;
	EMULE_PEER* local_peer = emule_get_local_peer();
	ret = sd_malloc(len, (void**)&buffer);
	if(ret != SUCCESS)
		return ret;
	tmp_buf = buffer;
	tmp_len = len;
	sd_set_int8(&tmp_buf, &tmp_len, OP_VC_NAT_HEADER);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, len - EMULE_HEADER_SIZE);
	sd_set_int8(&tmp_buf, &tmp_len, OP_NAT_PING);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)local_peer->_user_id, USER_ID_SIZE);
	ret = sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)udt_device->_conn_num);
	sd_assert(ret == SUCCESS && tmp_len == 0);
	++traverse->_ping_count;
	sd_time(&traverse->_last_ping_time);
	LOG_DEBUG("[emule_udt_device = 0x%x]emule_traverse_by_svr_send_ping, ip = %u, port = %u.", udt_device, traverse->_client_ip, (_u32)traverse->_client_port);
	return emule_udp_sendto(buffer, len, traverse->_client_ip, traverse->_client_port, TRUE);
}

_int32 emule_traverse_by_svr_send_sync2(EMULE_TRANSFER_BY_SERVER* traverse)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL, *tmp_buf = NULL;
	_int32 len = 42, tmp_len = 0;
	EMULE_UDT_DEVICE* udt_device = (EMULE_UDT_DEVICE* )traverse->_user_data;
	EMULE_PEER* local_peer = emule_get_local_peer();
	LOG_DEBUG("[emule_udt_device = 0x%x]emule_traverse_by_svr_send_sync2.", udt_device);
	ret = sd_malloc(len, (void**)&buffer);
	if(ret != SUCCESS)
		return ret;
	tmp_buf = buffer;
	tmp_len = len;
	sd_set_int8(&tmp_buf, &tmp_len, OP_VC_NAT_HEADER);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, len - EMULE_HEADER_SIZE);
	sd_set_int8(&tmp_buf, &tmp_len, OP_NAT_SYNC2);
	sd_set_bytes(&tmp_buf, &tmp_len, (char*)local_peer->_user_id, USER_ID_SIZE);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, (_int32)udt_device->_conn_num);
	ret = sd_set_bytes(&tmp_buf, &tmp_len, (char*)udt_device->_conn_id, USER_ID_SIZE);
	sd_assert(ret == SUCCESS && tmp_len == 0);
	sd_time(&traverse->_last_sync2_time);
	++traverse->_sync2_count;
	traverse->_next_sync2_interval = MIN(64, 2 << traverse->_sync2_count);
	return emule_nat_server_send(buffer, len);
}


_int32 emule_traverse_by_svr_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid)
{
	_u32 cur_time = 0;
	EMULE_TRANSFER_BY_SERVER* traverse = (EMULE_TRANSFER_BY_SERVER*)msg_info->_user_data;
	if(errcode == MSG_CANCELLED)
		return SUCCESS;
	sd_time(&cur_time);
	switch(traverse->_state)
	{
		case NAT_IDLE_STATE:
//			if(cur_time - traverse->_last_sync2_time >= MIN(64, 2 << traverse->_sync2_count))
			if(cur_time - traverse->_last_sync2_time >= ATTEMPT_INTERVAL)
			{
				if(traverse->_sync2_count >= SYNC2_ATTEMPT_TIMES)
				{
					traverse->_state |= NAT_TIMEOUT_STATE;
					emule_traverse_by_svr_failed(traverse);
				}
				else
					emule_traverse_by_svr_send_sync2(traverse);
			}
			break;
		case NAT_SYNC_STATE:
			if(cur_time - traverse->_last_ping_time > ATTEMPT_INTERVAL)
			{
				if(traverse->_ping_count > PING_ATTEMPT_TIMES)
				{
					traverse->_state |= NAT_TIMEOUT_STATE;
					emule_traverse_by_svr_failed(traverse);
				}
				else
					emule_traverse_by_svr_send_ping(traverse);
			}
			break;
		case NAT_PING_STATE:
			if(traverse->_ping_count > PING_ATTEMPT_TIMES)
				return emule_traverse_by_svr_failed(traverse);
			if(cur_time - traverse->_last_ping_time > 4)
				return emule_traverse_by_svr_send_ping(traverse);
		
		default:
			sd_assert(FALSE);
	}
	return SUCCESS;
}

_int32 emule_traverse_by_svr_failed(EMULE_TRANSFER_BY_SERVER* traverse)
{
	emule_udt_device_notify_traverse((EMULE_UDT_DEVICE*)traverse->_user_data, -1);
	return SUCCESS;
}

_int32 emule_traverse_by_svr_recv(EMULE_TRANSFER_BY_SERVER* traverse, _u8 opcode, _u32 ip, _u16 port)
{
	_int32 ret = SUCCESS;
	switch(opcode)
	{
		case OP_NAT_FAILED:
			traverse->_state |= NAT_NOPEER_STATE;
			ret = emule_traverse_by_svr_failed(traverse);
			break;
		case OP_NAT_SYNC2:
			traverse->_client_ip = ip;
			traverse->_client_port = port;
			traverse->_state |= NAT_SYNC_STATE;
			ret = emule_traverse_by_svr_send_ping(traverse);
			break;
		default:
			sd_assert(FALSE);
	}
	return ret;
}



#endif /* ENABLE_EMULE */

#endif /* ENABLE_EMULE */

