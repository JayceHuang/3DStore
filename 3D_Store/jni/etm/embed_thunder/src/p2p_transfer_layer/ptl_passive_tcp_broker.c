#include "ptl_passive_tcp_broker.h"
#include "ptl_cmd_define.h"
#include "ptl_cmd_extractor.h"
#include "ptl_cmd_builder.h"
#include "ptl_passive_connect.h"
#include "socket_proxy_interface.h"
#include "utility/map.h"
#include "utility/utility.h"
#include "utility/mempool.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_P2P_TRANSFER_LAYER
#include "utility/logger.h"

static	SET		g_passive_tcp_broker_data_set;

_int32 ptl_passive_tcp_broker_comparator(void *E1, void *E2)
{
	PASSIVE_TCP_BROKER_DATA* left = (PASSIVE_TCP_BROKER_DATA*)E1;
	PASSIVE_TCP_BROKER_DATA* right = (PASSIVE_TCP_BROKER_DATA*)E2;
	if(left->_broker_seq != right->_broker_seq)
		return (_int32)(left->_broker_seq - right->_broker_seq);
	if(left->_ip != right->_ip)
		return (_int32)(left->_ip - right->_ip);
	if(left->_port != right->_port)
		return (_int32)(left->_port - right->_port);
	return 0;
}

_int32 ptl_init_passive_tcp_broker(void)
{
	LOG_DEBUG("ptl_init_passive_tcp_broker.");
	set_init(&g_passive_tcp_broker_data_set, ptl_passive_tcp_broker_comparator);
	return SUCCESS;
}

_int32 ptl_uninit_passive_tcp_broker(void)
{
	PASSIVE_TCP_BROKER_DATA* data = NULL;
	SET_ITERATOR cur_iter, next_iter; 
	LOG_DEBUG("ptl_uninit_passive_tcp_broker.");
	cur_iter = SET_BEGIN(g_passive_tcp_broker_data_set);
	while(cur_iter != SET_END(g_passive_tcp_broker_data_set))
	{
		next_iter = SET_NEXT(g_passive_tcp_broker_data_set, cur_iter);
		data = (PASSIVE_TCP_BROKER_DATA*)SET_DATA(cur_iter);
		ptl_erase_passive_tcp_broker_data(data);
		cur_iter = next_iter;
		data = NULL;
	}
	return SUCCESS;
}

_int32 ptl_passive_tcp_broker(BROKER2_CMD* cmd)
{
	_int32 ret = SUCCESS;
	SD_SOCKADDR addr;
	PASSIVE_TCP_BROKER_DATA* data = NULL, *result = NULL;
	LOG_DEBUG("ptl_passive_tcp_broker, broker_seq = %u, ip = %u, port = %u.", cmd->_seq_num, cmd->_ip, cmd->_tcp_port);
	ret = sd_malloc(sizeof(PASSIVE_TCP_BROKER_DATA), (void**)&data);
	if(ret != SUCCESS)
		return ret;
	data->_sock = INVALID_SOCKET;
	data->_broker_seq = cmd->_seq_num;
	data->_ip = cmd->_ip;
	data->_port = cmd->_tcp_port;
	set_find_node(&g_passive_tcp_broker_data_set, data, (void**)&result);
	if(result != NULL)
	{
		LOG_DEBUG("ptl_passive_tcp_broker, but passive tcp broker is exist.");
		return sd_free(data);
	}
	ret = socket_proxy_create(&data->_sock, SD_SOCK_STREAM);
	if(ret!=SUCCESS) 
	{
		sd_free(data);
		data = NULL;
		return ret;
	}
	LOG_DEBUG("ptl_passive_tcp_broker data->_sock = %u", data->_sock);
	addr._sin_family = AF_INET;
	addr._sin_addr = data->_ip;
	addr._sin_port = sd_htons(data->_port);
	ret = socket_proxy_connect(data->_sock, &addr, ptl_passive_tcp_broker_connect_callback, data);
	if(ret!=SUCCESS) 
	{
		sd_free(data);
		data = NULL;
		return ret;
	}
	ret = set_insert_node(&g_passive_tcp_broker_data_set, data);
	if(ret!=SUCCESS) 
	{
		sd_free(data);
		data = NULL;
	}
	return ret;
}

_int32 ptl_passive_tcp_broker_connect_callback(_int32 errcode, _u32 pending_op_count, void* user_data)
{
	_int32 ret = SUCCESS;
	char* buffer = NULL;
	_u32 len = 0;
	TRANSFER_LAYER_CONTROL_CMD cmd;
	PASSIVE_TCP_BROKER_DATA* data = (PASSIVE_TCP_BROKER_DATA*)user_data;
	if(errcode != SUCCESS)
		return ptl_erase_passive_tcp_broker_data(data);
	LOG_DEBUG("ptl_passive_tcp_broker_connect_callback, send transfer_layer_control_cmd, seq = %u.", data->_broker_seq);
	sd_memset(&cmd, 0, sizeof(TRANSFER_LAYER_CONTROL_CMD));
	cmd._broker_seq = data->_broker_seq;
	ret = ptl_build_transfer_layer_control_cmd(&buffer, &len, &cmd);
	if(ret != SUCCESS)
	{
		LOG_ERROR("ptl_send_transfer_layer_control_cmd failed, ret = %d.", ret);
		return ret;
	}
	return socket_proxy_send(data->_sock, buffer, len, ptl_passive_tcp_broker_send_callback, data);
}

_int32 ptl_passive_tcp_broker_send_callback(_int32 errcode, _u32 pending_op_count, const char* buffer, _u32 had_send, void* user_data)
{
	PASSIVE_TCP_BROKER_DATA* data = (PASSIVE_TCP_BROKER_DATA*)user_data;
	sd_free((char*)buffer);
	buffer = NULL;
	if(errcode != SUCCESS)
		return ptl_erase_passive_tcp_broker_data(data);
	return socket_proxy_recv(data->_sock, data->_buffer, PASSIVE_TCP_BROKER_BUFFER_LEN, ptl_passive_tcp_broker_recv_callback, data);
}

_int32 ptl_passive_tcp_broker_recv_callback(_int32 errcode, _u32 pending_op_count, char* buffer, _u32 had_recv, void* user_data)
{
	_int32 ret = SUCCESS;
	TRANSFER_LAYER_CONTROL_RESP_CMD cmd;
	PASSIVE_TCP_BROKER_DATA* data = (PASSIVE_TCP_BROKER_DATA*)user_data;
	if(errcode != SUCCESS)
		return ptl_erase_passive_tcp_broker_data(data);
	ret = ptl_extract_transfer_layer_control_resp_cmd(data->_buffer, had_recv, &cmd);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("ptl_passive_tcp_broker_recv_cmd_callback, but ptl_extract_transfer_layer_control_resp_cmd failed, errcode = %d.", ret);
		return ptl_erase_passive_tcp_broker_data(data);
	}
	if(cmd._result != 1)
	{	
		LOG_DEBUG("ptl_passive_tcp_broker_recv_cmd_callback, but transfer_layer_control_resp result is %u, failed.", cmd._result);
		return ptl_erase_passive_tcp_broker_data(data);
	}
	ptl_accept_passive_tcp_broker_connect(data);
	return ptl_erase_passive_tcp_broker_data(data);
}

_int32 ptl_erase_passive_tcp_broker_data(PASSIVE_TCP_BROKER_DATA* data)
{
	_int32 ret = SUCCESS;
	_u32 op_count = 0;
	LOG_DEBUG("ptl_erase_passive_tcp_broker_data, broker_seq = %u, ip = %u, port = %u.", data->_broker_seq, data->_ip, data->_port);
	ret = set_erase_node(&g_passive_tcp_broker_data_set, data);
	CHECK_VALUE(ret);
	if(data->_sock != INVALID_SOCKET)
	{
		socket_proxy_peek_op_count(data->_sock, DEVICE_SOCKET_TCP, &op_count);
		if(op_count > 0)
			return socket_proxy_cancel(data->_sock, DEVICE_SOCKET_TCP);
		socket_proxy_close(data->_sock);
	}
	return sd_free(data);
}

