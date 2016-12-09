#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL
#include "emule_server_device.h"
#include "emule_server_cmd.h"
#include "emule_server_impl.h"
#include "../emule_utility/emule_opcodes.h"
#include "../emule_utility/emule_define.h"
#include "socket_proxy_interface.h"
#include "utility/mempool.h"
#include "utility/bytebuffer.h"
#include "utility/utility.h"
#include "utility/string.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_EMULE
#include "utility/logger.h"

_int32 emule_server_device_connect(EMULE_SERVER_DEVICE* server_device)
{
	return emule_socket_device_connect(server_device->_socket_device, server_device->_server->_ip, server_device->_server->_port, NULL);
}

_int32 emule_server_device_connect_callback(void* user_data, _int32 errcode)
{
	EMULE_SERVER_DEVICE* server_device = (EMULE_SERVER_DEVICE*)user_data;
	if(errcode != SUCCESS)
		return SUCCESS;
	return emule_server_notify_connect(server_device);
}

_int32 emule_server_device_extend_recv_buffer(EMULE_SERVER_DEVICE* server_device, _u32 len)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL;
	LOG_DEBUG("[emule_server] extend recv buffer, extend len = %u.", len);
	ret = sd_malloc(len, (void**)&buffer);
	if(ret != SUCCESS)
		return ret;
	if(server_device->_buffer_offset > 0)
		sd_memcpy(buffer, server_device->_buffer, server_device->_buffer_offset);
	server_device->_buffer_len = len;
	if(server_device->_buffer != NULL)
		sd_free(server_device->_buffer);
	server_device->_buffer = buffer;
	return ret;
}

_int32 emule_server_device_recv(EMULE_SERVER_DEVICE* server_device, _int32 len)
{
	sd_assert(server_device->_buffer_len >= server_device->_buffer_offset);
	if(len > server_device->_buffer_len - server_device->_buffer_offset)
	{
		emule_server_device_extend_recv_buffer(server_device, MAX(len, 1024));
	}
	return emule_socket_device_recv(server_device->_socket_device, server_device->_buffer + server_device->_buffer_offset, len, FALSE);
}

_int32 emule_server_device_recv_callback(void* user_data, char* buffer, _int32 len, _int32 errcode)
{
	_int32 ret = SUCCESS;
	char* tmp_buf = NULL;
	_int32 tmp_len = 0;
	_u8 protocol = 0;
	_u32 cmd_len = 0, total_len = 0;
	EMULE_SERVER_DEVICE* server_device = (EMULE_SERVER_DEVICE*)user_data;
	if(errcode != SUCCESS)
		return SUCCESS;
	server_device->_buffer_offset += len;
	tmp_buf = server_device->_buffer;
	tmp_len = (_int32)server_device->_buffer_offset;
	sd_get_int8(&tmp_buf, &tmp_len, (_int8*)&protocol);
	if(protocol != OP_EDONKEYPROT && protocol != OP_EMULEPROT && protocol != OP_PACKEDPROT)
	{
		LOG_ERROR("[emule_server] recv unknown protocol = %d.", protocol);
		return emule_server_handle_error(-1, server_device);
	}
	sd_get_int32_from_lt(&tmp_buf, &tmp_len, (_int32*)&cmd_len);
	total_len = cmd_len + EMULE_HEADER_SIZE;
	if(total_len == server_device->_buffer_offset)
	{
		ret = emule_server_recv_cmd(server_device, server_device->_buffer + EMULE_HEADER_SIZE, server_device->_buffer_offset - EMULE_HEADER_SIZE);
		if(ret != SUCCESS)
			return emule_server_handle_error(ret, server_device);
		server_device->_buffer_offset = 0;	
		return emule_server_device_recv(server_device, EMULE_HEADER_SIZE);
	}
	sd_assert(total_len > server_device->_buffer_offset);
	return emule_server_device_recv(server_device, total_len - server_device->_buffer_offset);
}

_int32 emule_server_device_send(EMULE_SERVER_DEVICE* server_device, char* buffer, _int32 len)
{
	return emule_socket_device_send(server_device->_socket_device, buffer, len);
}

_int32 emule_server_device_send_callback(void* user_data, char* buffer, _int32 len, _int32 errcode)
{
	return sd_free(buffer);
}

_int32 emule_server_device_close(EMULE_SERVER_DEVICE* server_device)
{
	return emule_socket_device_close(server_device->_socket_device);
}

_int32 emule_server_device_close_callback(void* user_data)
{
	EMULE_SERVER_DEVICE* server_device = (EMULE_SERVER_DEVICE*)user_data;
	if(server_device->_buffer != NULL)
	{
		sd_free(server_device->_buffer);
		server_device->_buffer = NULL;
		server_device->_buffer_len = 0;
		server_device->_buffer_offset = 0;
	}
	emule_socket_device_destroy(&server_device->_socket_device);
	emule_server_notify_close();
	return SUCCESS;
}

_int32 emule_server_device_notify_error(void* user_data, _int32 errcode)
{
	EMULE_SERVER_DEVICE* server_device = (EMULE_SERVER_DEVICE*)user_data;
	LOG_DEBUG("[emule_server_device = 0x%x]emule_server_device_notify_error.", server_device);
	emule_server_handle_error(errcode, server_device);
	return SUCCESS;
}

#endif /* ENABLE_EMULE */

#endif /* ENABLE_EMULE */

