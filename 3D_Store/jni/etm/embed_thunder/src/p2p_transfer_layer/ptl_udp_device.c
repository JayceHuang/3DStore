#include "utility/utility.h"
#include "utility/settings.h"
#include "utility/bytebuffer.h"
#include "utility/mempool.h"
#include "utility/logid.h"
#define  LOGID	LOGID_P2P_TRANSFER_LAYER
#include "utility/logger.h"
#include "asyn_frame/device.h"

#include "ptl_udp_device.h"

#include "ptl_cmd_define.h"
#include "ptl_memory_slab.h"
#include "ptl_utility.h"
#include "ptl_cmd_handler.h"
#include "socket_proxy_interface.h"

static SOCKET g_ptl_udp_socket;

_int32 ptl_create_udp_device()
{
    _int32 ret = socket_proxy_create(&g_ptl_udp_socket, SD_SOCK_DGRAM);
	if (ret != SUCCESS)
	{
	    return ret;
	}
	LOG_DEBUG("ptl_create_udp_device g_ptl_udp_socket = %u", g_ptl_udp_socket);
	
	_u32 udp_port = 3027;
	settings_get_int_item("ptl_setting.udp_port", (_int32*)&udp_port);
    SD_SOCKADDR addr;
	addr._sin_family = SD_AF_INET;
	addr._sin_addr = ANY_ADDRESS;
	addr._sin_port = sd_htons((_u16)udp_port);
	ret = socket_proxy_bind(g_ptl_udp_socket, &addr);
	if (ret != SUCCESS)
	{
		LOG_DEBUG("ptl_create_udp_device bind port failed, port = %u", sd_ntohs(addr._sin_port));
		socket_proxy_close(g_ptl_udp_socket);
	}
	ptl_set_local_udp_port(sd_ntohs(addr._sin_port));
	return ret;
}

_int32 ptl_close_udp_device()
{
    _int32 ret = SUCCESS;
	if (g_ptl_udp_socket == INVALID_SOCKET)
	{
	    return ret;
	}
	
	_u32 count = 0;
	socket_proxy_peek_op_count(g_ptl_udp_socket, DEVICE_SOCKET_UDP, &count);
	if (count > 0)
	{
	    ret = socket_proxy_cancel(g_ptl_udp_socket, DEVICE_SOCKET_UDP);
	}
	else
	{
	    ret = socket_proxy_close(g_ptl_udp_socket);
	    g_ptl_udp_socket = INVALID_SOCKET; 
	}
	return ret;
}

//NOTE:jieouy这个函数貌似有句柄泄漏，需要好好review一下
_int32 ptl_force_close_udp_device_socket(void)
{
    if (g_ptl_udp_socket != INVALID_SOCKET)
    {
        //socket_proxy_close(g_ptl_udp_socket);
        g_ptl_udp_socket = INVALID_SOCKET;
    }
    return SUCCESS;
}

SOCKET ptl_get_global_udp_socket()
{
	return g_ptl_udp_socket;
}

_int32 ptl_udp_sendto(const char* buffer, _u32 len, _u32 ip, _u16 port)
{
	_int32 ret = SUCCESS;
	SD_SOCKADDR	addr;
	addr._sin_family = SD_AF_INET;
	addr._sin_addr = ip;
	addr._sin_port = sd_htons(port);
	ret = socket_proxy_sendto(g_ptl_udp_socket, buffer, len, &addr, ptl_udp_sendto_callback, NULL);
	if (ret != SUCCESS)
	{
		LOG_ERROR("ptl_udp_sendto failed, errcode = %d.", ret);
		sd_free((char*)buffer);		/*发送失败，记得把接管的内存释放掉*/
		buffer = NULL;
	}
	return ret;
}

_int32 ptl_udp_sendto_by_domain(const char* buffer, _u32 len, const char* host, _u32 port)
{
	_int32 ret = socket_proxy_sendto_by_domain(g_ptl_udp_socket, buffer, len, host, port, ptl_udp_sendto_callback, NULL);
	if (ret != SUCCESS)
	{
		LOG_ERROR("ptl_udp_sendto_by_domain failed, errcode = %d.", ret);
		sd_free((char*)buffer);		/*发送失败，记得把接管的内存释放掉*/
		buffer = NULL;
	}
	return ret;
}

_int32 ptl_udp_sendto_callback(_int32 errcode, _u32 pending_op_count, const char* buffer, _u32 had_send, void* user_data)
{
	sd_free((char*)buffer);
	buffer = NULL;
	if(errcode == MSG_CANCELLED && pending_op_count == 0)
	{
		socket_proxy_close(g_ptl_udp_socket);
		g_ptl_udp_socket = INVALID_SOCKET;
	}
	if(errcode != SUCCESS)
	{
		LOG_DEBUG("ptl_udp_sendto_callback, errcode =%d.", errcode);
	}
	return SUCCESS;
}

_int32 ptl_udp_recvfrom(void)
{
    char* buffer = NULL;
    _int32 ret = ptl_malloc_udp_buffer(&buffer);
    if (ret != SUCCESS)
	{
		LOG_DEBUG("ptl_udp_recvfrom, but ptl_malloc_udp_buffer failed.");
		ptl_set_recv_udp_package();		//通知底层有空余的buffer时，重新启动recvfrom
		return ret;			
	}
	ret = socket_proxy_recvfrom(g_ptl_udp_socket, buffer, UDP_BUFFER_SIZE, ptl_udp_recvfrom_callback, NULL);
	CHECK_VALUE(ret);
	return ret;
}

/*receive udp package*/
_int32 ptl_udp_recvfrom_callback(_int32 errcode, _u32 pending_op_count, char* buffer, _u32 had_recv, SD_SOCKADDR* from, void* user_data)
{
       _int32 ret;  
	LOG_DEBUG("ptl_udp_recvfrom_callback, had_recv = %u, errcode = %d", had_recv, errcode);
	if(errcode == MSG_CANCELLED)
	{
		ptl_free_udp_buffer(buffer);
		if(pending_op_count == 0)
		{    
		       ret = socket_proxy_close(g_ptl_udp_socket);
		       g_ptl_udp_socket = INVALID_SOCKET; 
			return ret;
		}
		else
			return SUCCESS;
	}
	if(errcode == SUCCESS && had_recv >= 5)		
	{
		ptl_handle_recv_cmd(&buffer, had_recv, from->_sin_addr, sd_ntohs(from->_sin_port));
	}
	if(buffer != NULL)
		ptl_free_udp_buffer(buffer);
	return ptl_udp_recvfrom();
}
