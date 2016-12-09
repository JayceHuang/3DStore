#include "utility/string.h"
#include "utility/settings.h"
#include "utility/utility.h"
#include "utility/errcode.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_P2P_TRANSFER_LAYER
#include "utility/logger.h"
#include "platform/sd_network.h"

#include "tcp_interface.h"

#include "tcp_memory_slab.h"
#include "../ptl_callback_func.h"
#include "../ptl_interface.h"
#include "socket_proxy_interface.h"

_int32 init_tcp_modular()
{
	return tcp_init_memory_slab();
}

_int32 uninit_tcp_modular()
{
	return tcp_uninit_memory_slab();
}

_int32 tcp_device_create(TCP_DEVICE** tcp, SOCKET sock, struct tagPTL_DEVICE* device)
{
	_int32 ret = tcp_malloc_tcp_device(tcp);
	CHECK_VALUE(ret);
	sd_memset(*tcp, 0, sizeof(TCP_DEVICE));
	(*tcp)->_device = device;
	if (sock == INVALID_SOCKET)
	{
	    _int32 http_encap_state = 0; /*  0-未知,1-迅雷p2p协议不需要http封装,2-迅雷p2p协议需要http封装  */
		settings_get_int_item("p2p.http_encap_state", &http_encap_state );
		if (http_encap_state == 2
#if defined(MOBILE_PHONE)
			||(NT_GPRS_WAP & sd_get_net_type())
#endif
		)
		{
			LOG_DEBUG("tcp_device_create:using xunlei http encapsulated p2p protocol!");
			ret = socket_proxy_create_http_client((&(*tcp)->_socket), SD_SOCK_STREAM);
		}
		else
		{
			LOG_DEBUG("tcp_device_create:using xunlei original p2p protocol");
			ret = socket_proxy_create((&(*tcp)->_socket), SD_SOCK_STREAM);
		}
	}
	else
	{
		(*tcp)->_socket = sock;
	}
	
	LOG_DEBUG("tcp_device_create,*tcp=0x%X socket_device = 0x%x,_socket=%u", (*tcp),(*tcp)->_device,(*tcp)->_socket);
	return ret;
}

_int32 tcp_device_close(TCP_DEVICE* tcp)
{
	_u32 count = 0;
	sd_assert(tcp != NULL);
	if(tcp->_socket != INVALID_SOCKET)
		socket_proxy_peek_op_count(tcp->_socket, DEVICE_SOCKET_TCP, &count);
	if(count > 0)
	{
		socket_proxy_cancel(tcp->_socket, DEVICE_SOCKET_TCP);
		return ERR_P2P_WAITING_CLOSE;
	}
	LOG_DEBUG("tcp_device_close,tcp=0x%X, socket_device = 0x%x,_socket=%u", tcp,tcp->_device,tcp->_socket);
	/*no pending operation on socket*/
	socket_proxy_close(tcp->_socket);
#ifdef _SUPPORT_MHXY
	if (tcp->_mhxy)
	{
	    mhxy_tcp_destroy( &tcp->_mhxy );
	}
#endif
	tcp_free_tcp_device(tcp);
	return SUCCESS;
}

_int32 tcp_device_connect(TCP_DEVICE* tcp, _u32 ip, _u16 port)
{
	SD_SOCKADDR addr;
	addr._sin_family = SD_AF_INET;
	addr._sin_addr = ip;
	addr._sin_port = sd_htons((_u16)port);
	LOG_DEBUG("tcp_device_connect, tcp=0x%X,socket_device = 0x%x,_socket=%u,ip=%u,port=%u",tcp, tcp->_device,tcp->_socket,ip,port);
	return socket_proxy_connect(tcp->_socket, &addr, tcp_device_connect_callback, tcp);
}

_int32	tcp_device_connect_callback(_int32 errcode, _u32 pending_op_count, void* user_data)
{
	TCP_DEVICE* tcp = (TCP_DEVICE*)user_data;
	LOG_DEBUG("tcp_device_connect_callback, tcp=0x%X,socket_device = 0x%x,_socket=%u,errcode=%d", tcp,tcp->_device,tcp->_socket,errcode);
	if(errcode == MSG_CANCELLED)
	{
		return tcp_device_handle_cancel_msg(tcp, pending_op_count);
	}
	if(errcode != SUCCESS)
	{
		errcode = ERR_PTL_TRY_UDT_CONNECT;
	}
#ifdef _SUPPORT_MHXY
	if (SUCCESS==errcode)
	{
	    mhxy_tcp_new( &tcp->_mhxy );
	}
#endif
	return ptl_connect_callback(errcode, tcp->_device);
}

_int32 tcp_device_send_in_speed_limit(TCP_DEVICE* tcp, char* cmd_buffer, _u32 len)
{
	LOG_DEBUG("tcp_device_send_in_speed_limit, socket_device = 0x%x,_socket=%u,len=%u", tcp->_device,tcp->_socket,len);
	return socket_proxy_send_in_speed_limit(tcp->_socket, cmd_buffer, len, tcp_device_send_callback, tcp);
}

_int32 tcp_device_send(TCP_DEVICE* tcp, char* cmd_buffer, _u32 len)
{
	LOG_DEBUG("tcp_device_send, socket_device = 0x%x,_socket=%u,len=%u", tcp->_device,tcp->_socket,len);
	return socket_proxy_send(tcp->_socket, cmd_buffer, len, tcp_device_send_callback, tcp);
}


_int32 tcp_device_send_callback(_int32 errcode, _u32 pending_op_count, const char* cmd_buffer, _u32 had_send, void* user_data)
{
	TCP_DEVICE* tcp = (TCP_DEVICE*)user_data;
	LOG_DEBUG("tcp_device_send_callback, socket_device = 0x%x,_socket=%u,errcode=%d,had_send=%u", tcp->_device,tcp->_socket,errcode,had_send);
	ptl_free_cmd_buffer((char*)cmd_buffer);
	if(errcode == MSG_CANCELLED)
		return tcp_device_handle_cancel_msg(tcp, pending_op_count);
	return ptl_send_callback(errcode, tcp->_device, had_send);
}

_int32	tcp_device_recv_cmd(TCP_DEVICE* tcp, char* buffer, _u32 len)
{
	LOG_DEBUG("tcp_device_recv_cmd, socket_device = 0x%x,_socket=%u,len=%u", tcp->_device,tcp->_socket,len);
	return socket_proxy_recv(tcp->_socket, buffer, len, tcp_device_recv_cmd_callback, tcp);
}

_int32	tcp_device_recv_cmd_callback(_int32 errcode, _u32 pending_op_count, char* tmp_buffer, _u32 had_recv, void* user_data)
{
	TCP_DEVICE* tcp = (TCP_DEVICE*)user_data;
	LOG_DEBUG("tcp_device_recv_cmd_callback, socket_device = 0x%x,_socket=%u,errcode=%d,had_recv=%u", tcp->_device,tcp->_socket,errcode,had_recv);
	if(errcode == MSG_CANCELLED)
		return tcp_device_handle_cancel_msg(tcp, pending_op_count);
	return ptl_recv_cmd_callback(errcode, tcp->_device, had_recv);
}

_int32	tcp_device_recv_data(TCP_DEVICE* tcp, char* buffer, _u32 len)
{
	LOG_DEBUG("tcp_device_recv_data, socket_device = 0x%x,_socket=%u,len=%u", tcp->_device,tcp->_socket,len);
	return socket_proxy_recv(tcp->_socket, buffer, len, tcp_device_recv_data_callback, tcp);
}

_int32	tcp_device_recv_data_callback(_int32 errcode, _u32 pending_op_count, char* buffer, _u32 had_recv, void* user_data)
{
	TCP_DEVICE* tcp = (TCP_DEVICE*)user_data;
	LOG_DEBUG("tcp_device_recv_data_callback, socket_device = 0x%x,_socket=%u,errcode=%d,had_recv=%u", tcp->_device,tcp->_socket,errcode,had_recv);
	if(errcode == MSG_CANCELLED)
		return tcp_device_handle_cancel_msg(tcp, pending_op_count);
	return ptl_recv_data_callback(errcode, tcp->_device, had_recv);
}

_int32	tcp_device_recv_discard_data(TCP_DEVICE* tcp, char* buffer, _u32 len)
{
	LOG_DEBUG("tcp_device_recv_discard_data, socket_device = 0x%x,_socket=%u,len=%u", tcp->_device,tcp->_socket,len);
	return socket_proxy_recv(tcp->_socket, buffer, len, tcp_device_recv_discard_data_callback, tcp);
}

_int32	tcp_device_recv_discard_data_callback(_int32 errcode, _u32 pending_op_count, char* buffer, _u32 len, void* user_data)
{
	TCP_DEVICE* tcp = (TCP_DEVICE*)user_data;
	LOG_DEBUG("tcp_device_recv_discard_data_callback, socket_device = 0x%x,_socket=%u,errcode=%d,len=%u", tcp->_device,tcp->_socket,errcode,len);
	if(errcode == MSG_CANCELLED)
		return tcp_device_handle_cancel_msg(tcp, pending_op_count);
	return ptl_recv_discard_data_callback(errcode, tcp->_device, len);	
}

_int32 tcp_device_handle_cancel_msg(TCP_DEVICE* tcp, _u32 pending_op_count)
{
	PTL_DEVICE* device = NULL;
	LOG_DEBUG("tcp_device_handle_cancel_msg, socket_device = 0x%x,_socket=%u,pending_op_count=%u", tcp->_device,tcp->_socket,pending_op_count);
	sd_assert(tcp->_socket != INVALID_SOCKET);
	if(pending_op_count > 0)
		return SUCCESS;
	device = tcp->_device;
	socket_proxy_close(tcp->_socket);
	tcp_free_tcp_device(tcp);
	device->_device= NULL;
	return ptl_notify_close_device(device);	
}
