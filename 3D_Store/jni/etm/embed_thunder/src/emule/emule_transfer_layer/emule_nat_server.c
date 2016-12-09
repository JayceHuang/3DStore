#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL
#include "emule_nat_server.h"
#include "../emule_utility/emule_opcodes.h"
#include "../emule_utility/emule_udp_device.h"
#include "../emule_impl.h"
#include "utility/bytebuffer.h"
#include "utility/mempool.h"
#include "utility/time.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_EMULE
#include "utility/logger.h"

static	EMULE_NAT_SERVER		g_nat_server;
extern _int32 emule_nat_server_notask_hook(const _int32 task_state);

_int32 emule_nat_server_init(void)
{
	g_nat_server._keepalive_time = 0;
	g_nat_server._register_time = 0;
	g_nat_server._is_register = FALSE;
	g_nat_server._port = 2004;
	start_timer(emule_nat_server_timeout, NOTICE_INFINITE, 4000, 0, NULL, &g_nat_server._timeout_id);
	return emule_nat_server_register();
}

_int32 emule_nat_server_uninit(void)
{
	g_nat_server._keepalive_time = 0;
	g_nat_server._register_time = 0;
	g_nat_server._is_register = FALSE;
	g_nat_server._port = 2004;
	cancel_timer(g_nat_server._timeout_id);
	g_nat_server._timeout_id = INVALID_MSG_ID;
	return SUCCESS;
}

_int32 emule_nat_server_register(void)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL, *tmp_buf = NULL;
	_int32 len = 22, tmp_len = 0;
	EMULE_PEER* local_peer = emule_get_local_peer();
	_u32 cur_time = 0;
	ret = sd_malloc(len, (void**)&buffer);
	if(ret != SUCCESS)
		return ret;
	tmp_buf = buffer;
	tmp_len = len;
	sd_set_int8(&tmp_buf, &tmp_len, OP_VC_NAT_HEADER);
	sd_set_int32_to_lt(&tmp_buf, &tmp_len, len - EMULE_HEADER_SIZE);
	sd_set_int8(&tmp_buf, &tmp_len, OP_NAT_REGISTER);
	ret = sd_set_bytes(&tmp_buf, &tmp_len, (char*)local_peer->_user_id, USER_ID_SIZE);
	sd_assert(ret == SUCCESS && tmp_len == 0);
	sd_time(&cur_time);
	g_nat_server._keepalive_time = g_nat_server._register_time = cur_time;
	return emule_nat_server_send(buffer, len);
}

BOOL emule_nat_server_enable(void)
{
	return g_nat_server._is_register;
}

_int32 emule_nat_server_notify_register(_u32 ip, _u16 port)
{
	g_nat_server._is_register = TRUE;
	g_nat_server._ip = ip;
	g_nat_server._port = port;
	return SUCCESS;
}

_int32 emule_nat_server_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid)
{
	_u32 cur_time = 0;
	if(errcode == MSG_CANCELLED)
		return SUCCESS;
	if(g_nat_server._is_register == FALSE)
		return emule_nat_server_register();
	sd_time(&cur_time);
	if(cur_time - g_nat_server._register_time > 90)
		return emule_nat_server_register();
	return SUCCESS;
}

_int32 emule_nat_server_send(const char* buffer, _int32 len)
{
	if(g_nat_server._ip == 0)
		return emule_udp_sendto_by_domain(buffer, (_u32)len, "nat.emule.org.cn", g_nat_server._port, TRUE);
	else
		return emule_udp_sendto(buffer, (_u32)len, g_nat_server._ip, g_nat_server._port, TRUE);
}

_int32 emule_nat_server_notask_hook(const _int32 task_state)
{
	if(task_state == 1) {
		if(g_nat_server._timeout_id == INVALID_MSG_ID)
			emule_nat_server_init();
	}
	if(task_state == 0) {
		if(g_nat_server._timeout_id != INVALID_MSG_ID)
			emule_nat_server_uninit();
	}
	return SUCCESS;
}

#endif /* ENABLE_EMULE */
#endif /* ENABLE_EMULE */

