#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL
#include "emule_socket_device.h"
#include "emule_tcp_device.h"
#include "emule_udt_device.h"
#include "../emule_utility/emule_memory_slab.h"
#include "asyn_frame/msg.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_EMULE
#include "utility/logger.h"

void emule_socket_device_attach_tcp_device(EMULE_SOCKET_DEVICE* socket_device, EMULE_TCP_DEVICE* tcp_device)
{
	socket_device->_device = tcp_device;
	tcp_device->_socket_device = socket_device;
}

void emule_socket_device_attach_udt_device(EMULE_SOCKET_DEVICE* socket_device, EMULE_UDT_DEVICE* udt_device)
{
	socket_device->_device = udt_device;
	udt_device->_socket_device = socket_device;
}

_int32 emule_socket_device_create(EMULE_SOCKET_DEVICE** socket_device, EMULE_SOCKET_TYPE type,  EMULE_SERVER_ENCRYPTOR* encryptor, 
									   void** callback_table, void* user_data)
{
	_int32 ret = SUCCESS;
	EMULE_TCP_DEVICE* tcp_device = NULL;
	EMULE_UDT_DEVICE* udt_device = NULL;
	ret = emule_get_emule_socket_device_slip(socket_device);
	CHECK_VALUE(ret);
	sd_memset(*socket_device, 0, sizeof(EMULE_SOCKET_DEVICE));
	(*socket_device)->_type = type;
	if(type == EMULE_TCP_TYPE)
	{
		emule_tcp_device_create(&tcp_device);
		emule_socket_device_attach_tcp_device(*socket_device, tcp_device);
	}
	else
	{
		emule_udt_device_create(&udt_device);
		emule_socket_device_attach_udt_device(*socket_device, udt_device);
	}
	(*socket_device)->_callback_table = callback_table;
	(*socket_device)->_encryptor = encryptor;
	(*socket_device)->_user_data = user_data;
	LOG_DEBUG("[emule_socket_device = 0x%x]emule socket device create.", *socket_device);
	return ret;
}

_int32 emule_socket_device_connect(EMULE_SOCKET_DEVICE* socket_device, _u32 ip, _u16 port, _u8 conn_id[USER_ID_SIZE])
{
	if(socket_device->_type == EMULE_TCP_TYPE)
		return emule_tcp_device_connect((EMULE_TCP_DEVICE*)socket_device->_device, ip, port);
	else
	{
		return emule_udt_device_connect((EMULE_UDT_DEVICE*)socket_device->_device, ip, port, conn_id);
	}
}

_int32 emule_socket_device_connect_callback(EMULE_SOCKET_DEVICE* socket_device, _int32 errcode)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL;
	_int32 len = 0;
	if(socket_device->_encryptor != NULL && errcode == SUCCESS)
	{
		//需要加密传输，那么先不通知上层，三次握手成功后才告诉上层连接成功
		ret = emule_server_encryptor_build_handshake(socket_device->_encryptor, &buffer, &len);
		CHECK_VALUE(ret);
		socket_device->_encryptor->_state = ENCRYPT_HANDSHAKE_START;
		ret = emule_socket_device_send(socket_device, buffer, len);
		CHECK_VALUE(ret);
		//接收对方的握手消息
		return emule_socket_device_recv(socket_device, socket_device->_encryptor->_buffer, 103, FALSE);
	}
	((emule_socket_device_notify_connect)(socket_device->_callback_table[0]))(socket_device->_user_data, errcode);
	//如果出错了，那么需要通知上层,MSG_CANCELLED不是出错，而是上层要求关闭底层返回取消操作
	if(errcode != SUCCESS && errcode != MSG_CANCELLED)
		emule_socket_device_error(socket_device, errcode);
	return ret;
}

_int32 emule_socket_device_send(EMULE_SOCKET_DEVICE* socket_device, char* buffer, _int32 len)
{
    LOG_DEBUG("emule_socket_device_send, len = %d.", len);

	if(socket_device->_encryptor != NULL && socket_device->_encryptor->_state == ENCRYPT_HANDSHAKE_OK)
		emule_server_encryptor_encode_data(socket_device->_encryptor, buffer, len);
	if(socket_device->_type == EMULE_TCP_TYPE)
		return emule_tcp_device_send((EMULE_TCP_DEVICE*)socket_device->_device, buffer, len);
	else
		return emule_udt_device_send((EMULE_UDT_DEVICE*)socket_device->_device, buffer, len, FALSE);
}

_int32 emule_socket_device_send_in_speed_limit(EMULE_SOCKET_DEVICE* socket_device, char* buffer, _int32 len)
{
	if(socket_device->_encryptor != NULL && socket_device->_encryptor->_state == ENCRYPT_HANDSHAKE_OK)
		emule_server_encryptor_encode_data(socket_device->_encryptor, buffer, len);
	if(socket_device->_type == EMULE_TCP_TYPE)
		return emule_tcp_device_send_in_speed_limit((EMULE_TCP_DEVICE*)socket_device->_device, buffer, len);
	else
		return emule_udt_device_send((EMULE_UDT_DEVICE*)socket_device->_device, buffer, len, TRUE);
}


_int32 emule_socket_device_send_callback(EMULE_SOCKET_DEVICE* socket_device, char* buffer, _int32 len, _int32 errcode)
{
    LOG_DEBUG("emule_socket_device_send_callback.");

    //如果出错了，那么需要通知上层,MSG_CANCELLED不是出错，而是上层要求关闭底层返回取消操作
    if(errcode != SUCCESS && errcode != MSG_CANCELLED)
    {
        emule_socket_device_error(socket_device, errcode);
    }
    else 
    {
        //通知上层发送成功
	    ((emule_socket_device_notify_send)(socket_device->_callback_table[1]))(socket_device->_user_data, buffer, len, errcode);
    }

	return SUCCESS;
}

_int32 emule_socket_device_recv(EMULE_SOCKET_DEVICE* socket_device, char* buffer, _int32 len, BOOL is_data)
{
    LOG_DEBUG("emule_socket_device_recv, type = %d, TCP(UDT)_device = 0x%x, is_data = %d.", socket_device->_type, 
        socket_device->_device, is_data);

	if(socket_device->_type == EMULE_TCP_TYPE)
		return emule_tcp_device_recv((EMULE_TCP_DEVICE*)socket_device->_device, buffer, len, is_data);
	else
		return emule_udt_device_recv((EMULE_UDT_DEVICE*)socket_device->_device, buffer, len, is_data);
}

_int32 emule_socket_device_recv_callback(EMULE_SOCKET_DEVICE* socket_device, char* buffer, _int32 len, BOOL is_data, _int32 errcode)
{
	_int32 ret = SUCCESS;
	_u8 padding = 0;
	char* resp_buf = NULL;
	_int32 resp_len = 0;

    LOG_DEBUG("emule_socket_device_recv_callback, socket_device = 0x%x, len = %d, is_data = %d, errcode = %d", 
        socket_device, len, is_data, errcode);

	if(socket_device->_encryptor != NULL && errcode == SUCCESS)
	{
        LOG_DEBUG("socket_device->_encryptor != NULL, state = %d.", socket_device->_encryptor->_state);
		switch(socket_device->_encryptor->_state)
		{
			case ENCRYPT_HANDSHAKE_OK:
				emule_server_encryptor_decode_data(socket_device->_encryptor, buffer, len);
				break;
			case ENCRYPT_HANDSHAKE_START:
				sd_assert(len == 103);
				ret = emule_server_encryptor_recv_key(socket_device->_encryptor, buffer, len, &padding);
				if(ret != SUCCESS)
					return emule_socket_device_error(socket_device, ret);
				socket_device->_encryptor->_state = ENCRYPT_HANDSHAKE_RECV_PADDING;
				return emule_socket_device_recv(socket_device, socket_device->_encryptor->_buffer + len, padding, FALSE);
			case ENCRYPT_HANDSHAKE_RECV_PADDING:
				emule_server_encryptor_decode_data(socket_device->_encryptor, buffer, len);
				ret = emule_server_encryptor_build_handshake_resp(socket_device->_encryptor, &resp_buf, &resp_len);
				if(ret != SUCCESS)
					return emule_socket_device_error(socket_device, ret);
				ret = emule_socket_device_send(socket_device, resp_buf, resp_len);
				if(ret != SUCCESS)
					return emule_socket_device_error(socket_device, ret);
				socket_device->_encryptor->_state = ENCRYPT_HANDSHAKE_OK;
				return ((emule_socket_device_notify_connect)(socket_device->_callback_table[0]))(socket_device->_user_data, SUCCESS);
			default:
				sd_assert(FALSE);
		}
	}

	//通知上层接收成功
	if(is_data == FALSE)
		((emule_socket_device_notify_recv_cmd)(socket_device->_callback_table[2]))(socket_device->_user_data, buffer, len, errcode);
	else
		((emule_socket_device_notify_recv_data)(socket_device->_callback_table[3]))(socket_device->_user_data, buffer, len, errcode);
	
    //如果出错了，那么需要通知上层,MSG_CANCELLED不是出错，而是上层要求关闭底层返回取消操作
	if(errcode != SUCCESS && errcode != MSG_CANCELLED)
		emule_socket_device_error(socket_device, errcode);
	
    return errcode;	
}

_int32 emule_socket_device_close(EMULE_SOCKET_DEVICE* socket_device)
{
	LOG_DEBUG("[emule_socket_device = 0x%x]emule socket device close.", socket_device);
	if(socket_device->_type == EMULE_TCP_TYPE)
		return emule_tcp_device_close((EMULE_TCP_DEVICE*)socket_device->_device);
	else
		return emule_udt_device_close((EMULE_UDT_DEVICE*)socket_device->_device);
}

_int32 emule_socket_device_close_callback(EMULE_SOCKET_DEVICE* socket_device)
{
	((emule_socket_device_notify_close)(socket_device->_callback_table[4]))(socket_device->_user_data);
	return SUCCESS;
}

_int32 emule_socket_device_destroy(EMULE_SOCKET_DEVICE** socket_device)
{
	LOG_DEBUG("[emule_socket_device = 0x%x]emule socket device destroy.", *socket_device);
	if((*socket_device)->_type == EMULE_TCP_TYPE)
		emule_tcp_device_destroy((EMULE_TCP_DEVICE*)(*socket_device)->_device);
	else
		emule_udt_device_destroy((EMULE_UDT_DEVICE*)(*socket_device)->_device);
	emule_free_emule_socket_device_slip(*socket_device);
	*socket_device = NULL;
	return SUCCESS;
}

//出错了，需要通知上层
_int32 emule_socket_device_error(EMULE_SOCKET_DEVICE* socket_device, _int32 errcode)
{
	LOG_DEBUG("[emule_socket_device = 0x%x]emule_socket_device_error, errcode = %d.", socket_device, errcode);
	((emule_socket_device_notify_error)(socket_device->_callback_table[5]))(socket_device->_user_data, errcode);
	return SUCCESS;
}

#endif /* ENABLE_EMULE */

#endif /* ENABLE_EMULE */

