#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL
#include "emule_tcp_device.h"
#include "emule_socket_device.h"
#include "../emule_utility/emule_memory_slab.h"
#include "socket_proxy_interface.h"
#include "utility/utility.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_EMULE
#include "utility/logger.h"

_int32 emule_tcp_device_create(EMULE_TCP_DEVICE** tcp_device)
{
	_int32 ret = SUCCESS;
	ret = emule_get_emule_tcp_device_slip(tcp_device);
	CHECK_VALUE(ret);
	(*tcp_device)->_sock = INVALID_SOCKET;
	(*tcp_device)->_socket_device = NULL;
	LOG_DEBUG("[emule_tcp_device = 0x%x]emule_tcp_device_create.", *tcp_device);
	return ret;
}

_int32 emule_tcp_device_close(EMULE_TCP_DEVICE* tcp_device)
{
	_int32 ret = SUCCESS;
	_u32 count = 0;
	LOG_DEBUG("[emule_pipe_device = 0x%x]emule_tcp_device_close.", tcp_device->_socket_device->_user_data);
	ret = socket_proxy_peek_op_count(tcp_device->_sock, DEVICE_SOCKET_TCP, &count);
	CHECK_VALUE(ret);
	if(count > 0)
	{
		LOG_DEBUG("[emule_pipe_device = 0x%x]emule_tcp_device peek op count = %u, cancel op.", tcp_device->_socket_device->_user_data, count);
		return socket_proxy_cancel(tcp_device->_sock, DEVICE_SOCKET_TCP);
	}
	socket_proxy_close(tcp_device->_sock);
	return emule_socket_device_close_callback(tcp_device->_socket_device);
}

_int32 emule_tcp_device_destroy(EMULE_TCP_DEVICE* tcp_device)
{
	LOG_DEBUG("[emule_tcp_device = 0x%x]emule_tcp_device_destroy.", tcp_device);
	return emule_free_emule_tcp_device_slip(tcp_device);
}

_int32 emule_tcp_device_connect(EMULE_TCP_DEVICE* tcp_device, _u32 ip, _u16 port)
{
	_int32 ret = SUCCESS;
	SD_SOCKADDR addr;
	ret = socket_proxy_create(&tcp_device->_sock, SD_SOCK_STREAM);
	CHECK_VALUE(ret);
	addr._sin_family = AF_INET;
	addr._sin_addr = ip;
	addr._sin_port = sd_htons(port);
	return socket_proxy_connect(tcp_device->_sock, &addr, emule_tcp_device_connect_callback, tcp_device);
}

_int32 emule_tcp_device_connect_callback(_int32 errcode, _u32 pending_op_count, void* user_data)
{
	EMULE_TCP_DEVICE* tcp_device = (EMULE_TCP_DEVICE*)user_data;
	emule_socket_device_connect_callback(tcp_device->_socket_device, errcode);
	if(errcode == MSG_CANCELLED && pending_op_count == 0)
	{
		socket_proxy_close(tcp_device->_sock);
		emule_socket_device_close_callback(tcp_device->_socket_device);
	}
	return SUCCESS;
}

_int32 emule_tcp_device_send(EMULE_TCP_DEVICE* tcp_device, char* buffer, _int32 len)
{
    LOG_DEBUG("emule_tcp_device_send.");
	return socket_proxy_send(tcp_device->_sock, buffer, (_u32)len, emule_tcp_device_send_callback, tcp_device);
}

_int32 emule_tcp_device_send_in_speed_limit(EMULE_TCP_DEVICE* tcp_device, char* buffer, _int32 len)
{
	return socket_proxy_send_in_speed_limit(tcp_device->_sock, buffer, (_u32)len
	    , emule_tcp_device_send_callback, tcp_device);
}


_int32 emule_tcp_device_send_callback(_int32 errcode, _u32 pending_op_count, const char* buffer, _u32 had_send, void* user_data)
{
	EMULE_TCP_DEVICE* tcp_device= (EMULE_TCP_DEVICE*)user_data;
    LOG_DEBUG("emule_tcp_device_send_callback, errcode = %d, pending_op_count = %u, had_send = %u, user_data = 0x%x buffer = 0x%x",
        errcode, pending_op_count, had_send, user_data, buffer);

	emule_socket_device_send_callback(tcp_device->_socket_device, (char*)buffer, (_int32)had_send, errcode);
	if(errcode == MSG_CANCELLED && pending_op_count == 0)
	{
		socket_proxy_close(tcp_device->_sock);
		emule_socket_device_close_callback(tcp_device->_socket_device);
	}
	return SUCCESS;
}

_int32 emule_tcp_device_recv(EMULE_TCP_DEVICE* tcp_device, char* buffer, _int32 len, BOOL is_data)
{
	if(is_data == FALSE)
		return socket_proxy_recv(tcp_device->_sock, buffer, (_u32)len, emule_tcp_device_recv_cmd_callback, tcp_device);
	else
		return socket_proxy_recv(tcp_device->_sock, buffer, (_u32)len, emule_tcp_device_recv_data_callback, tcp_device);
}

_int32 emule_tcp_device_recv_cmd_callback(_int32 errcode, _u32 pending_op_count, char* buffer, _u32 had_recv, void* user_data)
{
	EMULE_TCP_DEVICE* tcp_device = (EMULE_TCP_DEVICE*)user_data;
    LOG_DEBUG("emule_tcp_device_recv_cmd_callback, tcp_device = 0x%x, errcode = %d, pending_op_count = %u "
        "buffer = 0x%x, had_recv = %u.", tcp_device, errcode, pending_op_count, buffer, had_recv);

	emule_socket_device_recv_callback(tcp_device->_socket_device, buffer, (_int32)had_recv, FALSE, errcode);
	if(errcode == MSG_CANCELLED && pending_op_count == 0)
	{
		socket_proxy_close(tcp_device->_sock);
		emule_socket_device_close_callback(tcp_device->_socket_device);
	}
	return SUCCESS;
}

_int32 emule_tcp_device_recv_data_callback(_int32 errcode, _u32 pending_op_count, char* buffer, _u32 had_recv, void* user_data)
{
	EMULE_TCP_DEVICE* tcp_device = (EMULE_TCP_DEVICE*)user_data;
	emule_socket_device_recv_callback(tcp_device->_socket_device, buffer, (_int32)had_recv, TRUE, errcode);
	if(errcode == MSG_CANCELLED && pending_op_count == 0)
	{
		socket_proxy_close(tcp_device->_sock);
		emule_socket_device_close_callback(tcp_device->_socket_device);
	}
	return SUCCESS;
}



#endif /* ENABLE_EMULE */

#endif /* ENABLE_EMULE */

