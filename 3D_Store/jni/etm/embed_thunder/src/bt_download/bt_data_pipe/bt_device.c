#include "utility/define.h"
#ifdef ENABLE_BT 
#ifdef ENABLE_BT_PROTOCOL

#include "bt_device.h"
#include "bt_memory_slab.h"
#include "bt_pipe_impl.h"
#include "bt_cmd_define.h"
#include "socket_proxy_interface.h"
#include "utility/utility.h"
#include "utility/mempool.h"
#include "utility/logid.h"
#define	LOGID	LOGID_BT_DOWNLOAD
#include "utility/logger.h"

_int32 bt_create_device(BT_DEVICE** device, BT_DATA_PIPE* bt_pipe)
{
	_int32 ret = SUCCESS;
	ret = bt_malloc_bt_device(device);
	if(ret != SUCCESS)
		return ret;
	sd_memset(*device, 0, sizeof(BT_DEVICE));
	(*device)->_sock = INVALID_SOCKET;
	(*device)->_bt_pipe = bt_pipe;
#ifdef _DEBUG
	init_speed_calculator(&(*device)->_upload_speed_calculator, 20, 500);
#endif
	return init_speed_calculator(&(*device)->_speed_calculator, 20, 500);
}

_int32 bt_close_device(BT_DEVICE* device)
{
	_u32 count = 0;
	if(device->_sock == INVALID_SOCKET)
		return bt_destroy_device(device);
	socket_proxy_peek_op_count(device->_sock, DEVICE_SOCKET_TCP, &count);
	if(count == 0)
		return bt_destroy_device(device);
	else
		return socket_proxy_cancel(device->_sock, DEVICE_SOCKET_TCP);
}

_int32 bt_destroy_device(BT_DEVICE* device)
{
	if(device->_sock != INVALID_SOCKET)
		socket_proxy_close(device->_sock);
	uninit_speed_calculator(&device->_speed_calculator);

	uninit_speed_calculator(&device->_upload_speed_calculator);

	if(device->_recv_buffer != NULL)
	{
		sd_free(device->_recv_buffer);
		device->_recv_buffer = NULL;
		device->_recv_buffer_len = 0;
		device->_recv_buffer_offset = 0;
	}
	return bt_pipe_notify_close(device->_bt_pipe);
}

_int32 bt_device_connect(BT_DEVICE* device, _u32 ip, _u16 port)
{
	_int32 ret = SUCCESS;
	SD_SOCKADDR addr;
	ret = socket_proxy_create(&device->_sock, SD_SOCK_STREAM);
	if(ret != SUCCESS)
		return ret;
	LOG_DEBUG("bt_device_connect device->_sock = %u", device->_sock);
	addr._sin_family = AF_INET;
	addr._sin_addr = ip;
	addr._sin_port = sd_htons(port);
	return socket_proxy_connect(device->_sock, &addr, bt_device_connect_callback, device);
}

_int32 bt_device_connect_callback(_int32 errcode, _u32 pending_op_count, void* user_data)
{
	BT_DEVICE* device = (BT_DEVICE*)user_data;
	if(errcode != SUCCESS)
		return bt_device_handle_error(errcode, pending_op_count, device);
	else
		return bt_pipe_notify_connnect_success(device->_bt_pipe);
}

_int32 bt_device_send(BT_DEVICE* device, char* buffer, _u32 len)
{
	_int32 ret = SUCCESS;
	ret = socket_proxy_send(device->_sock, buffer, len, bt_device_send_callback, device);
	if(ret != SUCCESS)
	{
		sd_free(buffer);
		buffer = NULL;
	}
	return ret;
}

_int32 bt_device_send_in_speed_limit(BT_DEVICE* device, char* buffer, _u32 len)
{
	_int32 ret = SUCCESS;
	ret = socket_proxy_send_in_speed_limit(device->_sock, buffer, len
	    , bt_device_send_data_callback, device);
	if(ret != SUCCESS)
	{
		sd_free(buffer);
		buffer = NULL;
	}
	return ret;
}


_int32 bt_device_send_callback(_int32 errcode, _u32 pending_op_count, const char* buffer, _u32 had_send, void* user_data)
{
	BT_DEVICE* device = (BT_DEVICE*)user_data;
	sd_free((char*)buffer);
	buffer = NULL;
	if(errcode != SUCCESS)
		return bt_device_handle_error(errcode, pending_op_count, device);
	bt_pipe_notify_send_success(device->_bt_pipe);
	return SUCCESS;
}


_int32 bt_device_send_data(BT_DEVICE* device, char* buffer, _u32 len)
{
	_int32 ret = SUCCESS;
	ret = socket_proxy_send(device->_sock, buffer, len, bt_device_send_data_callback, device);
	if(ret != SUCCESS)
	{
		sd_free(buffer);
		buffer = NULL;
	}
	return ret;
}

_int32 bt_device_send_data_callback(_int32 errcode, _u32 pending_op_count
    , const char* buffer, _u32 had_send, void* user_data)
{
	BT_DEVICE* device = (BT_DEVICE*)user_data;
	sd_free((char*)buffer);
	buffer = NULL;
	if(errcode != SUCCESS)
		return bt_device_handle_error(errcode, pending_op_count, device);
	bt_pipe_notify_send_success(device->_bt_pipe);
	add_speed_record(&device->_upload_speed_calculator, had_send - 13);		//13个字节是协议字节，要减去后才是数据长度
	return SUCCESS;
}



_int32 bt_device_recv(BT_DEVICE* device, _u32 len)
{
	//这里首先应该检查下是否越界了
	_int32 ret = SUCCESS;
	char* buffer = NULL;
	if(len > device->_recv_buffer_len - device->_recv_buffer_offset)
	{
		ret = sd_malloc(len + device->_recv_buffer_offset, (void**)&buffer);
		CHECK_VALUE(ret);
		sd_memcpy(buffer, device->_recv_buffer, device->_recv_buffer_offset);
		SAFE_DELETE(device->_recv_buffer);
		device->_recv_buffer = buffer;
		device->_recv_buffer_len = len + device->_recv_buffer_offset;
	}
	return socket_proxy_recv(device->_sock, device->_recv_buffer + device->_recv_buffer_offset
	    , len, bt_device_recv_callback, device);
}

_int32 bt_device_recv_callback(_int32 errcode, _u32 pending_op_count, char* buffer
    , _u32 had_recv, void* user_data)
{
	_int32 ret = SUCCESS;
	_u32 package_len = 0;
	BT_DEVICE* device = (BT_DEVICE*)user_data;
	LOG_DEBUG("[bt_pipe = 0x%x]bt_device_recv_callback.", device->_bt_pipe);
	if(errcode != SUCCESS)
		return bt_device_handle_error(errcode, pending_op_count, device);
	if(device->_bt_pipe->_data_pipe._dispatch_info._pipe_state == PIPE_FAILURE)
		return SUCCESS;
	device->_recv_buffer_offset += had_recv;    
	sd_assert(device->_recv_buffer_offset >= BT_PRO_HEADER_LEN);

#ifdef _DEBUG
            LOG_DEBUG("bt_device_recv buffer, had_recv_len = %d", had_recv);
            extern void dump_buffer(char* buffer, _u32 length);
            dump_buffer(device->_recv_buffer, 100);
#endif
	
	switch(device->_recv_buffer_offset)   
	{
	case BT_PRO_HEADER_LEN:
		{
			sd_memcpy(&package_len, device->_recv_buffer, sizeof(_u32));
			package_len = sd_ntohl(package_len);
			if(package_len >= BT_MAX_PACKET_SIZE)
				return bt_device_handle_error(BT_INVALID_CMD, 0, device);
			if(package_len == 0)
			{	/*说明收到了一个长度为0的keepalive数据包*/
				ret = bt_pipe_recv_cmd_package(device->_bt_pipe, BT_KEEPALIVE);
			}
			else
			{
				return bt_device_recv(device, 1);		//接收命令码
			}
			break;
		}

	case 5:
		{
			if(device->_recv_buffer[4] == BT_PIECE)		//是否是BT_PIECE命令，是的话需要特殊处理，仅仅接收命令头
				return bt_device_recv(device, 8);
			sd_memcpy(&package_len, device->_recv_buffer, sizeof(_u32));
			package_len = sd_ntohl(package_len);
			if(package_len == 1)
				ret = bt_pipe_recv_cmd_package(device->_bt_pipe, (_u8)device->_recv_buffer[4]);
			else
				return bt_device_recv(device, package_len - 1);
			break;
		}
		
	case BT_HANDSHAKE_PRO_LEN:
		{
			if(device->_recv_buffer[0] == 19)
				ret = bt_pipe_recv_cmd_package(device->_bt_pipe, BT_HANDSHAKE);
			else
				ret = bt_pipe_recv_cmd_package(device->_bt_pipe, (_u8)device->_recv_buffer[4]);
			break;
		}

	default:
		{
		    
			ret = bt_pipe_recv_cmd_package(device->_bt_pipe, (_u8)device->_recv_buffer[4]);
		}
	}
	if(ret == SUCCESS)
	{	//接收下一个包
		device->_recv_buffer_offset = 0;
		return bt_device_recv(device, BT_PRO_HEADER_LEN);
	}
	else if(ret == BT_PIPE_GET_DATA_BUFFER_FAILED || ret == BT_PIPE_RECV_PIECE_DATA)
	{	//获取数据缓冲区失败，此时启动定时器隔一段时间再去获取内存，知道获得内存再投递接收数据请求
		return SUCCESS;
	}
	else 
	{	//出错了
		return bt_pipe_handle_error(ret, device->_bt_pipe);
	}
}

_int32 bt_device_recv_discard_data(BT_DEVICE* device, char* buffer, _u32 len)
{
	return socket_proxy_recv(device->_sock, buffer, len, bt_device_recv_discard_data_callback, device);
}

_int32 bt_device_recv_discard_data_callback(_int32 errcode, _u32 pending_op_count, char* buffer, _u32 had_recv, void* user_data)
{
	BT_DEVICE* device = (BT_DEVICE*)user_data;
	sd_free(buffer);
	buffer = NULL;
	if(errcode != SUCCESS)
		return bt_device_handle_error(errcode, pending_op_count, device);
	return SUCCESS;
}

_int32 bt_device_recv_data(BT_DEVICE* device, char* buffer, _u32 len)
{
	return socket_proxy_recv(device->_sock, buffer, len, bt_device_recv_data_callback, device);
}

_int32 bt_device_recv_data_callback(_int32 errcode, _u32 pending_op_count, char* buffer, _u32 had_recv, void* user_data)
{
	BT_DEVICE* device = (BT_DEVICE*)user_data;
	if(errcode != SUCCESS)
		return bt_device_handle_error(errcode, pending_op_count, device);
	if(device->_bt_pipe->_data_pipe._dispatch_info._pipe_state == PIPE_FAILURE)
		return SUCCESS;
	bt_pipe_notify_recv_data(device->_bt_pipe);
	add_speed_record(&device->_speed_calculator, had_recv);
	device->_recv_buffer_offset = 0;
	return bt_device_recv(device, BT_PRO_HEADER_LEN);			//接收下一个包
}

_int32 bt_device_handle_error(_int32 errcode, _u32 pending_op_count, BT_DEVICE* device)
{
	if(errcode == MSG_CANCELLED)
	{
		if(pending_op_count != 0)
			return SUCCESS;
		else
			return bt_destroy_device(device);
	}
	else
	{
		return bt_pipe_handle_error(errcode, device->_bt_pipe);
	}
}
#endif
#endif

