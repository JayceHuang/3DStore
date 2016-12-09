#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL
#include "emule_udp_device.h"
#include "emule_opcodes.h"
#include "emule_memory_slab.h"
#include "../emule_transfer_layer/emule_udt_cmd.h"
#include "../emule_server/emule_server_impl.h"
#include "../emule_pipe/emule_pipe_impl.h"
#include "../emule_impl.h"
#include "platform/sd_socket.h"
#include "socket_proxy_interface.h"
#include "utility/utility.h"
#include "utility/mempool.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_EMULE
#include "utility/logger.h"
#include "utility/settings.h"

static	SOCKET		g_emule_udp_socket;

_int32 emule_create_udp_device(void)
{
	_int32 ret = SUCCESS;
	_u32 port = 4661;
	SD_SOCKADDR addr;
	EMULE_PEER* local_peer = emule_get_local_peer();
	ret = socket_proxy_create(&g_emule_udp_socket, SD_SOCK_DGRAM);
	if(ret != SUCCESS)
		return ret;
	settings_get_int_item("emule.udp_socket_port", (_int32*)&port);
	addr._sin_family = SD_AF_INET;
	addr._sin_addr = ANY_ADDRESS;
	addr._sin_port = sd_htons(port);
	ret = socket_proxy_bind(g_emule_udp_socket, &addr);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("emule_create_udp_device bind port failed, port = %u", sd_ntohs(addr._sin_port));
		socket_proxy_close(g_emule_udp_socket);
	}
	local_peer->_udp_port = sd_ntohs(addr._sin_port);
	return ret;
}

_int32 emule_close_udp_device(void)
{
	_u32 count = 0;
	_int32 ret = SUCCESS;
	if(g_emule_udp_socket == INVALID_SOCKET)
		return SUCCESS;
	socket_proxy_peek_op_count(g_emule_udp_socket, DEVICE_SOCKET_UDP, &count);
	if(count > 0)
	{
		return socket_proxy_cancel(g_emule_udp_socket, DEVICE_SOCKET_UDP);
	}
	else
	{
	       ret = socket_proxy_close(g_emule_udp_socket);
		g_emule_udp_socket = INVALID_SOCKET; 
		return ret; 
	}
}

_int32 emule_force_close_udp_socket(void)
{
    if (g_emule_udp_socket != INVALID_SOCKET)
    {
        g_emule_udp_socket = INVALID_SOCKET;
    }
    return SUCCESS;
}

_int32 emule_udp_sendto(const char* buffer, _u32 len, _u32 ip, _u16 port, BOOL need_free)
{
	_int32 ret = SUCCESS;
	SD_SOCKADDR	addr;
	addr._sin_family = SD_AF_INET;
	addr._sin_addr = ip;
	addr._sin_port = sd_htons(port);
	ret = socket_proxy_sendto(g_emule_udp_socket, buffer, len, &addr, emule_udp_sendto_callback, (void*)need_free);
	if(ret != SUCCESS)
	{
		LOG_ERROR("emule_udp_sendto failed, errcode = %d.", ret);
		if(need_free == TRUE)
			sd_free((char*)buffer);		/*发送失败，记得把接管的内存释放掉*/
	}
	return ret;
}

_int32 emule_udp_sendto_in_speed_limit(const char* buffer
        , _u32 len, _u32 ip, _u16 port, BOOL need_free)
{
	_int32 ret = SUCCESS;
	SD_SOCKADDR	addr;
	addr._sin_family = SD_AF_INET;
	addr._sin_addr = ip;
	addr._sin_port = sd_htons(port);
	ret = socket_proxy_sendto_in_speed_limit(g_emule_udp_socket, buffer, len
	    , &addr, emule_udp_sendto_callback, (void*)need_free);
	if(ret != SUCCESS)
	{
		LOG_ERROR("emule_udp_sendto failed, errcode = %d.", ret);
		if(need_free == TRUE)
			sd_free((char*)buffer);		/*发送失败，记得把接管的内存释放掉*/
	}
	return ret;    
}

_int32 emule_udp_sendto_by_domain(const char* buffer, _u32 len, const char* host, _u16 port, BOOL need_free)
{
	_int32 ret = SUCCESS;
	ret = socket_proxy_sendto_by_domain(g_emule_udp_socket, buffer, len, host, port, emule_udp_sendto_callback, (void*)need_free);
	if(ret != SUCCESS)
	{
		LOG_ERROR("emule_udp_sendto_by_domain failed, errcode = %d.", ret);
		if(need_free == TRUE)
			sd_free((char*)buffer);		/*发送失败，记得把接管的内存释放掉*/
	}
	return ret;
}



_int32 emule_udp_sendto_callback(_int32 errcode, _u32 pending_op_count, const char* buffer, _u32 had_send, void* user_data)
{
	BOOL need_free = (BOOL)user_data;
	if(need_free == TRUE)
		sd_free((char*)buffer);
	if(errcode == MSG_CANCELLED && pending_op_count == 0)
	{
		socket_proxy_close(g_emule_udp_socket);
		g_emule_udp_socket = INVALID_SOCKET;
	}
	if(errcode != SUCCESS)
	{
		LOG_DEBUG("emule_udp_sendto_callback, errcode =%d.", errcode);
	}
	return SUCCESS;
}

_int32 emule_udp_recvfrom(void)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL;
	ret = emule_get_udp_buffer_slip(&buffer);
	CHECK_VALUE(ret);
	ret = socket_proxy_recvfrom(g_emule_udp_socket, buffer, UDP_BUFFER_SIZE, emule_udp_recvfrom_callback, NULL);
	CHECK_VALUE(ret);
	return ret;
}

_int32 emule_udp_recvfrom_callback(_int32 errcode, _u32 pending_op_count, char* buffer, _u32 had_recv, SD_SOCKADDR* from, void* user_data)
{
    _int32 ret = SUCCESS;  
//	LOG_DEBUG("emule_udp_recvfrom_callback, had_recv = %u, errcode = %d", had_recv, errcode);
	if(errcode == MSG_CANCELLED)
	{
		emule_free_udp_buffer_slip(buffer);
		if(pending_op_count == 0)
		{    
		       ret = socket_proxy_close(g_emule_udp_socket);
		       g_emule_udp_socket = INVALID_SOCKET; 
		}
		return ret;
	}
	if(errcode == SUCCESS && had_recv > 0)		
	{
		switch((_u8)buffer[0])
		{
			case OP_EDONKEYPROT:
				emule_server_recv_udp_cmd(buffer + 1, had_recv - 1, from->_sin_addr, sd_ntohs(from->_sin_port));
				break;
			case OP_EMULEPROT:
				emule_handle_recv_udp_cmd(buffer + 1, had_recv - 1, from->_sin_addr, sd_ntohs(from->_sin_port));
				break;
			case OP_VC_NAT_HEADER:
				emule_recv_udt_cmd(&buffer, had_recv, from->_sin_addr, sd_ntohs(from->_sin_port));
				break;
			default:
				break;
		}
	}
	if(buffer != NULL)
		emule_free_udp_buffer_slip(buffer);
	return emule_udp_recvfrom();
}

#endif /* ENABLE_EMULE */

#endif /* ENABLE_EMULE */

